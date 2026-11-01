#include "zModel.h"
#include "zPath.h"
#include "zContainer.h"

#include <cmath>

static zSTRING modelHeadNodeName = "HEAD";
static int modelState = -1;
static int modelNodeState = -1;
const zSTRING zMDL_NODE_NAME_BIP_PREFIX("BIP01");
const zSTRING zMDL_NODE_NAME_R_HAND(zMDL_NODE_NAME_BIP_PREFIX + " R HAND");
const zSTRING zMDL_NODE_NAME_L_HAND(zMDL_NODE_NAME_BIP_PREFIX + " L HAND");
const zSTRING zMDL_NODE_NAME_R_FOOT(zMDL_NODE_NAME_BIP_PREFIX + " R FOOT");
const zSTRING zMDL_NODE_NAME_L_FOOT(zMDL_NODE_NAME_BIP_PREFIX + " L FOOT");
const zSTRING zMDL_NODE_NAME_HEAD(zMDL_NODE_NAME_BIP_PREFIX + " HEAD");
const zSTRING zMDL_NODE_NAME_FOOTSTEPS(" FOOTSTEPS");
static zSTRING modelSourceExtensionMds(".MDS");
static zSTRING modelSourceExtensionAsc(".ASC");
static zTAniAttachment modelAniAttachment;
static zSTRING modelTokenWhitespace(" \t");
static zSTRING modelLineWhitespace(" \t\r");
static zSTRING modelScriptWhitespace(" \t\r");
static float modelAngleQuantization = 6.28318548f / 65535.0f;
static float modelAngleLimitQuantization = 2.0999999f / 65535.0f;
static float modelUnitQuantization = 1.0f / 65535.0f;
// EXE: 0x008D8AA0
static zCArray<zSTRING> modelAniCache;
// EXE: 0x008D8A84
int zCModel::s_drawSkeleton;
// EXE: 0x008A2D50
int zCModel::s_bSmoothRootNode = 1;
// EXE: 0x008D8A20
int* zCModel::s_iNumSoftSkinPoly = NULL;
static zSTRING modelNodeBuffer;
static zSTRING modelMeshBuffer;
static zSTRING modelAniBuffer;

zCModelAniEvent::zCModelAniEvent()
    : aniEventType(zMDL_EVENT_TAG), frameNr(0), value1(0), value2(0), value3(0), value4(0)
{
    soundList.parray = 0;
    soundList.numAlloc = 0;
    soundList.numInArray = 0;
    pfxEmitterList.parray = 0;
    pfxEmitterList.numAlloc = 0;
    pfxEmitterList.numInArray = 0;
}

zCModelAniEvent::~zCModelAniEvent()
{
    for (int i = soundList.numInArray - 1; i >= 0; --i) {
        if (soundList.parray[i])
            soundList.parray[i]->Release();
    }

    delete[] pfxEmitterList.parray;
    pfxEmitterList.parray = 0;
    delete[] soundList.parray;
    soundList.parray = 0;
}

void zCModelAniEvent::Save(zCFileBIN& binFile) const
{
    binFile.file->Write(&aniEventType, 4);
    binFile.file->Write(&frameNr, 4);
    binFile.BinWriteString(tagString);

    for (int i = 0; i < zMDL_ANIEVENT_MAXSTRING; ++i)
        binFile.BinWriteString(string[i]);

    binFile.file->Write(&value1, 4);
    binFile.file->Write(&value2, 4);
    binFile.file->Write(&value3, 4);
    binFile.file->Write(&value4, 4);
    int reserved = 0;
    binFile.file->Write(&reserved, 4);
}

void zCModelAniEvent::Load(zCFileBIN& binFile)
{
    binFile.file->Read(&aniEventType, 4);
    binFile.file->Read(&frameNr, 4);
    binFile.file->Read(tagString);

    for (int i = 0; i < zMDL_ANIEVENT_MAXSTRING; ++i)
        binFile.file->Read(string[i]);

    binFile.file->Read(&value1, 4);
    binFile.file->Read(&value2, 4);
    binFile.file->Read(&value3, 4);
    binFile.file->Read(&value4, 4);
    int reserved;
    binFile.file->Read(&reserved, 4);
}

zCModelPrototype* zCModelPrototype::s_modelRoot;

template <class T>
static void ResetArray(zCArray<T>& array)
{
    array.parray = 0;
    array.numAlloc = 0;
    array.numInArray = 0;
}

static void DeleteModelTree(zCTree<zCModelNode>* tree)
{
    delete tree->data;
    tree->data = 0;

    zCTree<zCModelNode>* child = tree->firstChild;
    while (child) {
        zCTree<zCModelNode>* next = child->next;
        DeleteModelTree(child);
        delete child;
        child = next;
    }

    tree->firstChild = 0;
    tree->lastChild = 0;
}

zCModelPrototype::zCModelPrototype()
    : next(0), prev(0), refCtr(1)
{
    Init();
    next = s_modelRoot;
    if (s_modelRoot)
        s_modelRoot->prev = this;
    s_modelRoot = this;
}

void zCModelPrototype::Init()
{
    protoAnis.array = 0;
    protoAnis.numAlloc = 0;
    protoAnis.numInArray = 0;
    protoAnis.Compare = zArraySortDefaultCompare;
    ResetArray(modelEvents);
    ResetArray(nodeList);
    ResetArray(meshSoftSkinList);
    ResetArray(sourceMeshSoftSkinList);

    meshTree.data = 0;
    meshTree.parent = 0;
    meshTree.firstChild = 0;
    meshTree.lastChild = 0;
    meshTree.prev = 0;
    meshTree.next = 0;

    for (int i = 0; i < 3; ++i) {
        bbox3D.mins.n[i] = 0;
        bbox3D.maxs.n[i] = 0;
        bbox3DCollDet.mins.n[i] = 0;
        bbox3DCollDet.maxs.n[i] = 0;
        rootNodeTrans.n[i] = 0;
    }

    nodeListChecksum = 0;
    baseModelProto = 0;
    fileSourceType = zFROM_MDS;
    modelProtoName.Sprintf("%u", reinterpret_cast<unsigned int>(this));
}

zCModelPrototype::~zCModelPrototype()
{
    --refCtr;
    Clear();

    if (this == s_modelRoot)
        s_modelRoot = next;
    else if (prev)
        prev->next = next;

    if (next)
        next->prev = prev;

    next = 0;
    prev = 0;
    baseModelProto = 0;
    delete[] sourceMeshSoftSkinList.parray;
    sourceMeshSoftSkinList.parray = 0;
}

void zCModelPrototype::Clear()
{
    ReleaseMeshSoftSkinList();

    for (int i = 0; i < nodeList.numInArray; ++i)
        nodeList.parray[i]->SetNodeVisualS(0);

    DeleteModelTree(&meshTree);

    for (int i = 0; i < protoAnis.numInArray; ++i) {
        if (protoAnis.array[i])
            protoAnis.array[i]->Release();
    }

    delete[] protoAnis.array;
    protoAnis.array = 0;
    protoAnis.numAlloc = 0;
    protoAnis.numInArray = 0;

    for (int i = 0; i < modelEvents.numInArray; ++i)
        delete modelEvents.parray[i];

    delete[] modelEvents.parray;
    ResetArray(modelEvents);
    delete[] nodeList.parray;
    ResetArray(nodeList);
    hierarchySourceASC.Clear();
    delete[] sourceMeshSoftSkinList.parray;
    ResetArray(sourceMeshSoftSkinList);
    baseModelProto = 0;
    Init();
}

void zCModelPrototype::ReleaseMeshSoftSkinList()
{
    for (int i = 0; i < meshSoftSkinList.numInArray; ++i) {
        if (meshSoftSkinList.parray[i])
            meshSoftSkinList.parray[i]->Release();
    }

    delete[] meshSoftSkinList.parray;
    ResetArray(meshSoftSkinList);
}

void zCModelPrototype::ReleaseMeshes()
{
    ReleaseMeshSoftSkinList();

    for (int i = 0; i < nodeList.numInArray; ++i)
        nodeList.parray[i]->SetNodeVisualS(0);
}

int zCModelPrototype::Release()
{
    const int result = --refCtr;
    if (result <= 0) {
        ++refCtr;
        delete this;
    }

    return result;
}

int zCModelPrototype::NumInList()
{
    int result = 0;
    for (zCModelPrototype* current = s_modelRoot; current; current = current->next)
        ++result;
    return result;
}

const zSTRING& zCModelPrototype::GetModelProtoFileName() const
{
    return modelProtoFileName;
}

void zCModelPrototype::SetModelProtoName(const zSTRING& name)
{
    modelProtoName = name;
    modelProtoName.Upper();
    modelProtoFileName = modelProtoName;
    modelProtoFileName.Append(fileSourceType == zFROM_ASC ? ".ASC" : ".MDS");
}

void zCModelPrototype::SetFileSourceType(zTFileSourceType type)
{
    fileSourceType = type;
    modelProtoFileName = modelProtoName;
    modelProtoFileName.Append(type == zFROM_ASC ? ".ASC" : ".MDS");
}

int zCModelPrototype::PrepareAsModelProtoOverlay(zCModelPrototype* base)
{
    if (!base || base->protoAnis.numInArray < protoAnis.numInArray)
        return 0;

    const int count = base->protoAnis.numInArray;
    zCModelAni** overlayAnis = new zCModelAni*[count];
    for (int i = 0; i < count; ++i)
        overlayAnis[i] = 0;

    for (int i = 0; i < protoAnis.numInArray; ++i) {
        const int index = base->SearchAniIndex(protoAnis.array[i]->aniName);
        if (index < 0) {
            delete[] overlayAnis;
            return 0;
        }

        overlayAnis[index] = protoAnis.array[i];
        overlayAnis[index]->aniID = index;
    }

    delete[] protoAnis.array;
    protoAnis.array = overlayAnis;
    protoAnis.numAlloc = count;
    protoAnis.numInArray = count;
    baseModelProto = base;
    return 1;
}

void zCModelPrototype::CalcNodeListChecksum()
{
    unsigned long checksum = 0;
    for (int i = 0; i < nodeList.numInArray; ++i) {
        zSTRING& name = nodeList.parray[i]->nodeName;
        checksum = zCChecksum::GetBufferCRC32(name.ToChar(), name.Length(), checksum);
    }

    nodeListChecksum = checksum;
}

void zCModelPrototype::CollectNodeMeshes(zCArray<zCModelNode*>& nodes)
{
    for (int i = 0; i < nodeList.numInArray; ++i) {
        zCModelNode* node = nodeList.parray[i];
        if (!node || !node->visual)
            continue;

        if (node->nodeName.Length() >= 3 && node->nodeName[0] == 'Z' &&
            node->nodeName[1] == 'S' && node->nodeName[2] == '_')
            continue;

        if (nodes.numInArray >= nodes.numAlloc) {
            const int newAlloc = nodes.numAlloc + 16;
            zCModelNode** newArray = new zCModelNode*[newAlloc];
            for (int j = 0; j < nodes.numInArray; ++j)
                newArray[j] = nodes.parray[j];
            delete[] nodes.parray;
            nodes.parray = newArray;
            nodes.numAlloc = newAlloc;
        }

        nodes.parray[nodes.numInArray++] = node;
    }
}

zCModelPrototype* zCModelPrototype::SearchName(zSTRING name)
{
    name.Upper();
    for (zCModelPrototype* current = s_modelRoot; current; current = current->next) {
        if (current->modelProtoFileName == name)
            return current;
    }

    return 0;
}

zCModelNode* zCModelPrototype::SearchNode(const zSTRING& name)
{
    for (int i = 0; i < nodeList.numInArray; ++i) {
        if (nodeList.parray[i]->nodeName == name)
            return nodeList.parray[i];
    }

    return 0;
}

int zCModelPrototype::SearchAniIndex(const zSTRING& name) const
{
    for (int i = 0; i < protoAnis.numInArray; ++i) {
        if (protoAnis.array[i] && protoAnis.array[i]->aniName == name)
            return i;
    }

    return -1;
}

zCModelAni* zCModelPrototype::SearchAni(const zSTRING& name) const
{
    const int index = SearchAniIndex(name);
    return index >= 0 ? protoAnis.array[index] : 0;
}

zCTree<zCModelNode>* zCModelPrototype::FindMeshTreeNode(const zSTRING& name,
    zCTree<zCModelNode>* tree)
{
    if (!tree)
        tree = &meshTree;

    if (tree->data && tree->data->nodeName == name)
        return tree;

    for (zCTree<zCModelNode>* child = tree->firstChild; child; child = child->next) {
        zCTree<zCModelNode>* result = FindMeshTreeNode(name, child);
        if (result)
            return result;
    }

    return 0;
}

static int FindNodeIndexRecursive(const zSTRING& name, zCTree<zCModelNode>* tree, int& index)
{
    if (tree->data) {
        if (tree->data->nodeName == name)
            return index;
        ++index;
    }

    for (zCTree<zCModelNode>* child = tree->firstChild; child; child = child->next) {
        const int result = FindNodeIndexRecursive(name, child, index);
        if (result >= 0)
            return result;
    }

    return -1;
}

int zCModelPrototype::FindMeshTreeNodeIndex(const zSTRING& name, zCTree<zCModelNode>* tree)
{
    int index = 0;
    return FindNodeIndexRecursive(name, tree ? tree : &meshTree, index);
}

int zCModelPrototype::FindNodeListIndex(const zSTRING& name)
{
    for (int i = 0; i < nodeList.numInArray; ++i) {
        if (nodeList.parray[i]->nodeName == name)
            return i;
    }

    return -1;
}

void zCModelPrototype::AddAni(zCModelAni* ani)
{
    if (SearchAniIndex(ani->aniName) >= 0)
        return;

    if (protoAnis.numInArray >= protoAnis.numAlloc) {
        const int delta = protoAnis.numAlloc >= 16 ? protoAnis.numAlloc / 2 : 16;
        const int newAlloc = protoAnis.numAlloc + delta;
        zCModelAni** newArray = new zCModelAni*[newAlloc];
        for (int i = 0; i < protoAnis.numInArray; ++i)
            newArray[i] = protoAnis.array[i];
        delete[] protoAnis.array;
        protoAnis.array = newArray;
        protoAnis.numAlloc = newAlloc;
    }

    protoAnis.array[protoAnis.numInArray++] = ani;
}

unsigned long zCModelPrototype::GetMDHFileVersion()
{
    return 3;
}

void zCModelPrototype::DescribeTree(zCTree<zCModelNode>* tree, int level)
{
    if (!tree)
        tree = &meshTree;

    zSTRING text;
    for (int i = 0; i < level * 2; ++i)
        text.Append(" ");

    text.Append(tree->data ? tree->data->nodeName : zSTRING("(null)"));
    zerr->Message(text);

    for (zCTree<zCModelNode>* child = tree->firstChild; child; child = child->next)
        DescribeTree(child, level + 1);
}

void zCModelPrototype::ConvertVec3(zVEC3& value)
{
    const float oldY = value.n[1];
    value.n[1] = value.n[2];
    value.n[2] = -oldY;
    value.n[0] = -value.n[0];
}

void zCModelPrototype::ConvertAngle(float&)
{
}

unsigned long zCModelPrototype::GetMDSFileVersion()
{
    return 1;
}

static void SetIdentity(zMAT4& matrix)
{
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x)
            matrix.m[y][x] = x == y ? 1.0f : 0.0f;
    }
}

zCModelNode::zCModelNode()
    : parentNode(0), visual(0), nodeRotAngle(0), nodeTrafoList(0), lastInstNode(0)
{
    SetIdentity(trafo);
    SetIdentity(trafoObjToWorld);
    for (int i = 0; i < 3; ++i) {
        nodeRotAxis.n[i] = 0;
        translation.n[i] = 0;
    }
}

zCModelNode::zCModelNode(const zCModelNode& other)
    : parentNode(0), nodeName(other.nodeName), visual(other.visual), trafo(other.trafo),
      nodeRotAngle(other.nodeRotAngle), trafoObjToWorld(other.trafoObjToWorld),
      nodeTrafoList(0), lastInstNode(0)
{
    if (visual)
        visual->AddRef();

    for (int i = 0; i < 3; ++i) {
        nodeRotAxis.n[i] = other.nodeRotAxis.n[i];
        translation.n[i] = other.translation.n[i];
    }
}

zCModelNode::~zCModelNode()
{
    if (visual)
        visual->Release();

    visual = 0;
    parentNode = 0;
    nodeTrafoList = 0;
    lastInstNode = 0;
}

void zCModelNode::SetNodeVisualS(zCVisual* value)
{
    if (value == visual)
        return;

    if (visual)
        visual->Release();
    if (value)
        value->AddRef();
    visual = value;
}

int zCModelPrototype::s_autoConvertAnis;

zCModelPrototype* zCModelPrototype::Load(const zSTRING& fileName, zCModelPrototype* base)
{
    zPATH path(fileName);
    zSTRING shortName = path.GetFile();
    shortName.Upper();

    zCModelPrototype* model = SearchName(shortName);
    if (model) {
        ++model->refCtr;
        return model;
    }

    model = new zCModelPrototype();
    const int loaded = fileName.Search(".ASC") >= 0
        ? model->LoadModelASC(fileName)
        : model->LoadModelScript(fileName);

    if (!loaded) {
        model->Release();
        return 0;
    }

    if (base)
        model->PrepareAsModelProtoOverlay(base);
    return model;
}

int zCModelPrototype::LoadMDH(const zSTRING& fileName)
{
    zFILE_FILE* source = new zFILE_FILE(fileName);
    if (source->Open(false)) {
        delete source;
        return 0;
    }

    zCFileBIN binFile;
    binFile.BinSetFile(source);
    const int result = LoadMDH(binFile);
    binFile.BinClose();
    source->Close();
    delete source;
    return result;
}

static void AppendNode(zCArray<zCModelNode*>& nodes, zCModelNode* node)
{
    if (nodes.numInArray >= nodes.numAlloc) {
        const int newAlloc = nodes.numAlloc + 16;
        zCModelNode** newArray = new zCModelNode*[newAlloc];
        for (int i = 0; i < nodes.numInArray; ++i)
            newArray[i] = nodes.parray[i];
        delete[] nodes.parray;
        nodes.parray = newArray;
        nodes.numAlloc = newAlloc;
    }

    nodes.parray[nodes.numInArray++] = node;
}

static void AttachNode(zCModelPrototype* model, zCModelNode* node, zCModelNode* parent)
{
    zCTree<zCModelNode>* parentTree = &model->meshTree;
    if (parent) {
        zCTree<zCModelNode>* found = model->FindMeshTreeNode(parent->nodeName);
        if (found)
            parentTree = found;
    }

    zCTree<zCModelNode>* tree = new zCTree<zCModelNode>;
    tree->data = node;
    tree->parent = parentTree;
    tree->firstChild = 0;
    tree->lastChild = 0;
    tree->prev = 0;
    tree->next = parentTree->firstChild;
    if (tree->next)
        tree->next->prev = tree;
    parentTree->firstChild = tree;
    node->parentNode = parent;
}

int zCModelPrototype::LoadMDH(zCFileBIN& binFile)
{
    SetModelProtoName(binFile.file->GetFilename());

    while (binFile.nextStart < binFile.file->Size()) {
        unsigned short chunkID = 0;
        int chunkSize = 0;
        binFile.file->Read(&chunkID, 2);
        if (binFile.file->Eof())
            break;
        binFile.file->Read(&chunkSize, 4);
        binFile.nextStart = binFile.file->Pos() + chunkSize;

        if (chunkID == 0xD100) {
            int version;
            unsigned short numNodes;
            binFile.file->Read(&version, 4);
            if (version != 3)
                return 0;
            binFile.file->Read(&numNodes, 2);
            ResetArray(nodeList);

            for (int i = 0; i < numNodes; ++i) {
                zCModelNode* node = new zCModelNode();
                short parentIndex;
                binFile.file->Read(node->nodeName);
                binFile.file->Read(&parentIndex, 2);
                binFile.file->Read(&node->trafo, 0x40);

                zCModelNode* parent = 0;
                if (parentIndex >= 0 && parentIndex < nodeList.numInArray)
                    parent = nodeList.parray[parentIndex];
                AttachNode(this, node, parent);
                AppendNode(nodeList, node);
            }

            binFile.file->Read(&bbox3D, sizeof(zTBBox3D));
            binFile.file->Read(&bbox3DCollDet, sizeof(zTBBox3D));
            binFile.file->Read(&rootNodeTrans, sizeof(zVEC3));
            binFile.file->Read(&nodeListChecksum, 4);
        }
        else if (chunkID == 0xD110) {
            if (s_autoConvertAnis && !binFile.BinReadCompareSrcFileStats())
                return 0;
        }
        else if (chunkID == 0xD120) {
            return 1;
        }

        if (binFile.nextStart < binFile.file->Size())
            binFile.file->Seek(binFile.nextStart);
    }

    return 1;
}

void zCModelPrototype::SaveMDH()
{
    zSTRING fileName = modelProtoName;
    fileName.Append(".mdh");
    zCFileBIN binFile;
    binFile.BinCreate(fileName);
    SaveMDH(binFile);
    binFile.BinClose();
}

void zCModelPrototype::SaveMDH(zCFileBIN& binFile)
{
    binFile.BinStartChunk(0xD100);
    binFile.BinWriteInt(3);
    const unsigned short numNodes = nodeList.numInArray;
    binFile.file->Write(&numNodes, 2);

    for (int i = 0; i < nodeList.numInArray; ++i) {
        zCModelNode* node = nodeList.parray[i];
        binFile.BinWriteString(node->nodeName);
        const short parentIndex = node->parentNode
            ? FindNodeListIndex(node->parentNode->nodeName)
            : -1;
        binFile.file->Write(&parentIndex, 2);
        binFile.file->Write(&node->trafo, 0x40);
    }

    binFile.file->Write(&bbox3D, sizeof(zTBBox3D));
    binFile.file->Write(&bbox3DCollDet, sizeof(zTBBox3D));
    binFile.file->Write(&rootNodeTrans, sizeof(zVEC3));
    binFile.file->Write(&nodeListChecksum, 4);

    binFile.BinStartChunk(0xD110);
    binFile.BinWriteSrcFileStats(hierarchySourceASC);
    binFile.BinStartChunk(0xD120);
}

static zCFileBIN* modelParserFile;
static zSTRING modelParserLine;
static int modelSceneFirstFrame;
static int modelSceneLastFrame;
static int modelSceneFrameCount;

void zCModelPrototype::SkipBlock()
{
    int depth = 1;
    while (depth && !modelParserFile->file->Eof()) {
        modelParserFile->file->Read(modelParserLine);
        if (modelParserLine.Search("{") >= 0)
            ++depth;
        else if (modelParserLine.Search("}") >= 0)
            --depth;
    }
}

void zCModelPrototype::SkipBlockCmt()
{
    SkipBlock();
}

void zCModelPrototype::ReadComment()
{
}

void zCModelPrototype::ReadScene(float& frameSpeed)
{
    int firstFrame = 999999;
    int lastFrame = 0;
    int ticksPerFrame = 1;

    while (!modelParserFile->file->Eof()) {
        modelParserFile->file->Read(modelParserLine);
        modelParserLine.Upper();

        if (modelParserLine.Search("}") >= 0)
            break;
        if (modelParserLine.Search("*SCENE_FIRSTFRAME") >= 0)
            firstFrame = modelParserLine.PickWord_Old(3, " \t").ToInt();
        else if (modelParserLine.Search("*SCENE_LASTFRAME") >= 0)
            lastFrame = modelParserLine.PickWord_Old(3, " \t").ToInt();
        else if (modelParserLine.Search("*SCENE_FRAMESPEED") >= 0)
            frameSpeed = modelParserLine.PickWord_Old(3, " \t").ToFloat();
        else if (modelParserLine.Search("*SCENE_TICKSPERFRAME") >= 0)
            ticksPerFrame = modelParserLine.PickWord_Old(3, " \t").ToInt();
        else if (modelParserLine.Search("*SCENE_ENVMAP") >= 0)
            SkipBlock();
    }

    modelSceneFirstFrame = firstFrame;
    modelSceneLastFrame = lastFrame;
    modelSceneFrameCount = lastFrame - firstFrame + 1;
    if (ticksPerFrame > 0)
        frameSpeed /= ticksPerFrame;
}

zVEC3 zCModelPrototype::ReadTriple()
{
    zVEC3 result;
    result.n[0] = modelParserLine.PickWord(2, " \t", " \t").ToFloat();
    result.n[1] = modelParserLine.PickWord(3, " \t", " \t").ToFloat();
    result.n[2] = modelParserLine.PickWord(4, " \t", " \t").ToFloat();
    return result;
}

zCModelAni::zCModelAni()
    : aniID(0), layer(0), blendInSpeed(0), blendOutSpeed(0), collisionVolumeScale(1),
      nextAni(0), aniEvents(0), fpsRate(0), fpsRateSource(0), rootNodeIndex(-1),
      nodeList(0), aniSampleMatrix(0), samplePosRangeMin(0), samplePosScaler(0),
      numFrames(0), numNodes(0), aniType(zMDL_ANI_TYPE_NORMAL), aniDir(zMDL_ANIDIR_FORWARD),
      numAniEvents(0), aniFlags(0)
{
    combAniList.data = 0;
    combAniList.next = 0;
    ResetArray(nodeIndexList);
    for (int i = 0; i < 3; ++i) {
        aniBBox3DObjSpace.mins.n[i] = 0;
        aniBBox3DObjSpace.maxs.n[i] = 0;
    }
}

const zSTRING& zCModelAni::GetAniName() const
{
    return aniName;
}

zTMdl_AniType zCModelAni::GetAniType() const
{
    return aniType;
}

int zCModelAni::GetAniID() const
{
    return aniID;
}

void zCModelAni::SetBlendingSec(float blendIn, float blendOut)
{
    blendInSpeed = blendIn != 0 ? 1.0f / blendIn : 3.40282347e+38f;
    blendOutSpeed = blendOut != 0 ? -1.0f / blendOut : -3.40282347e+38f;
}

void zCModelAni::GetBlendingSec(float& blendIn, float& blendOut) const
{
    blendIn = blendInSpeed != 3.40282347e+38f ? 1.0f / blendInSpeed : 0;
    blendOut = blendOutSpeed != -3.40282347e+38f ? -1.0f / blendOutSpeed : 0;
}

zCModelAni::~zCModelAni()
{
    if (aniType != zMDL_ANI_TYPE_ALIAS) {
        delete[] nodeList;
        delete[] aniSampleMatrix;
        delete[] aniEvents;
    }

    nodeList = 0;
    aniSampleMatrix = 0;
    aniEvents = 0;
    nextAni = 0;
    delete[] nodeIndexList.parray;
    nodeIndexList.parray = 0;

    zCList<zCModelAni>* entry = combAniList.next;
    while (entry) {
        zCList<zCModelAni>* next = entry->next;
        delete entry;
        entry = next;
    }
    combAniList.next = 0;
}

void zCModelAni::SetFlags(const zSTRING& flags)
{
    aniFlags = 0;
    if (flags.Search("P") >= 0)
        aniFlags |= 0x02;
    if (flags.Search("R") >= 0)
        aniFlags |= 0x01;
    if (flags.Search("O") >= 0)
        aniFlags |= 0x04;
    if (flags.Search("F") >= 0)
        aniFlags |= 0x08;
    if (flags.Search("I") >= 0)
        aniFlags |= 0x30;
    if (flags.Search("Z") >= 0)
        aniFlags |= 0x20;
}

void zCModelAni::PrecacheAniEventData()
{
    for (int i = 0; i < numAniEvents; ++i) {
        zCModelAniEvent& event = aniEvents[i];
        if (event.aniEventType != zMDL_EVENT_SOUND && event.aniEventType != zMDL_EVENT_SOUND_GRND)
            continue;

        for (int j = 0; j < event.soundList.numInArray; ++j) {
            zCSoundFX* sound = event.soundList.parray[j];
            if (!sound)
                continue;

            if ((sound->cacheState & 3) != 3) {
                sound->TouchTimeStampLocal();
                sound->CacheIn(0.5f);
            }
            else {
                sound->TouchTimeStamp();
            }
        }
    }
}

zVEC3 zCModelAni::GetAniTranslation() const
{
    zVEC3 result = {{0, 0, 0}};
    if (rootNodeIndex < 0)
        return result;

    const zTMdl_AniSample& first = aniSampleMatrix[rootNodeIndex];
    const zTMdl_AniSample& last = aniSampleMatrix[numNodes * (numFrames - 1) + rootNodeIndex];
    for (int i = 0; i < 3; ++i) {
        result.n[i] = (last.position[i] - first.position[i]) * samplePosScaler;
    }
    return result;
}

float zCModelAni::GetAniVelocity() const
{
    const float duration = numFrames / fpsRate * 1000.0f;
    if (duration == 0 || rootNodeIndex < 0)
        return 0;

    const zVEC3 translation = GetAniTranslation();
    return std::sqrt(translation.n[0] * translation.n[0] +
        translation.n[1] * translation.n[1] + translation.n[2] * translation.n[2]) / duration;
}

void zCModelAni::AddNodeList(zCTree<zCModelNode>** nodes)
{
    if (numNodes <= 0) {
        nodeList = 0;
        return;
    }

    nodeList = new zCModelNode*[numNodes];
    for (int i = 0; i < numNodes; ++i)
        nodeList[i] = nodes[i]->data;
}

zCQuat zCModelAni::GetQuat(int frame, int node) const
{
    const zTMdl_AniSample& sample = aniSampleMatrix[numNodes * frame + node];
    zCQuat result;
    for (int i = 0; i < 3; ++i)
        result.q[i] = (sample.rotation[i] - 32767) / 32767.0f;

    float w = 1.0f - result.q[0] * result.q[0] - result.q[1] * result.q[1] -
        result.q[2] * result.q[2];
    if (w < 0)
        w = 0;
    result.q[3] = std::sqrt(w);
    return result;
}

zVEC3 zCModelAni::GetTrans(int frame, int node) const
{
    const zTMdl_AniSample& sample = aniSampleMatrix[numNodes * frame + node];
    zVEC3 result;
    for (int i = 0; i < 3; ++i)
        result.n[i] = sample.position[i] * samplePosScaler + samplePosRangeMin;
    return result;
}

void zCModelAni::SetTrans(int frame, int node, const zVEC3& value)
{
    zTMdl_AniSample& sample = aniSampleMatrix[numNodes * frame + node];
    for (int i = 0; i < 3; ++i) {
        sample.position[i] = static_cast<unsigned short>(
            (value.n[i] - samplePosRangeMin) / samplePosScaler);
    }
}

void zCModelAni::SetQuat(int frame, int node, const zCQuat& value)
{
    zTMdl_AniSample& sample = aniSampleMatrix[numNodes * frame + node];
    for (int i = 0; i < 3; ++i) {
        sample.rotation[i] = static_cast<unsigned short>(value.q[i] * 32767.0f) + 32767;
    }

    if (value.q[3] < 0) {
        for (int i = 0; i < 3; ++i)
            sample.rotation[i] = 65534 - sample.rotation[i];
    }
}

void zCModelAni::CorrectRootNodeIdleMovement()
{
    if (rootNodeIndex < 0 || numFrames <= 0)
        return;

    const zVEC3 translation = GetAniTranslation();
    const float distance = std::sqrt(translation.n[0] * translation.n[0] +
        translation.n[1] * translation.n[1] + translation.n[2] * translation.n[2]);
    if (distance < 1.0f)
        aniFlags |= 0x40;
}

void zCModelAni::CalcInPlaceFlag()
{
    if (aniFlags & 0x08)
        return;
    if (rootNodeIndex < 0) {
        aniFlags |= 0x20;
        return;
    }

    float maxDistance = 0;
    zVEC3 previous = GetTrans(0, rootNodeIndex);
    for (int frame = 1; frame < numFrames; ++frame) {
        const zVEC3 current = GetTrans(frame, rootNodeIndex);
        const float x = current.n[0] - previous.n[0];
        const float y = current.n[1] - previous.n[1];
        const float z = current.n[2] - previous.n[2];
        const float distance = std::sqrt(x * x + y * y + z * z);
        if (distance > maxDistance)
            maxDistance = distance;
        previous = current;
    }

    if (maxDistance < 10.0f)
        aniFlags |= 0x20;
}

int zCModelAni::ResolveAlias(zCModelPrototype* model)
{
    if (aniType != zMDL_ANI_TYPE_ALIAS || !aliasName.Length())
        return 1;

    zCModelAni* source = model->SearchAni(aliasName);
    if (!source || source->aniType != zMDL_ANI_TYPE_NORMAL)
        return 0;

    ascName = source->ascName;
    fpsRateSource = source->fpsRateSource;
    fpsRate = source->fpsRate;
    numFrames = source->numFrames;
    numNodes = source->numNodes;
    nodeList = source->nodeList;
    aniSampleMatrix = source->aniSampleMatrix;
    rootNodeIndex = source->rootNodeIndex;
    numAniEvents = source->numAniEvents;
    aniEvents = source->aniEvents;
    samplePosRangeMin = source->samplePosRangeMin;
    samplePosScaler = source->samplePosScaler;
    aniBBox3DObjSpace = source->aniBBox3DObjSpace;
    aniFlags = (aniFlags & ~0x60) | (source->aniFlags & 0x60);

    delete[] nodeIndexList.parray;
    nodeIndexList.numAlloc = source->nodeIndexList.numInArray;
    nodeIndexList.numInArray = source->nodeIndexList.numInArray;
    nodeIndexList.parray = new int[nodeIndexList.numAlloc];
    for (int i = 0; i < nodeIndexList.numInArray; ++i)
        nodeIndexList.parray[i] = source->nodeIndexList.parray[i];
    return 1;
}

int zCModelAni::ResolveComb(zCModelPrototype* model)
{
    if (aniType != zMDL_ANI_TYPE_COMB || !aliasName.Length())
        return 1;

    const int count = zSTRING(aliasName[aliasName.Length() - 1]).ToInt();
    aliasName.Delete(aliasName.Length() - 1, 1);
    if (count < 1)
        return 0;

    zCModelAni* last = 0;
    numFrames = 0;
    numNodes = 0;
    for (int i = 1; i <= count; ++i) {
        zSTRING number;
        number.Sprintf("%d", i);
        zSTRING combName = aliasName;
        combName.Append(number);
        zCModelAni* source = model->SearchAni(combName);
        if (!source)
            return 0;
        if (numFrames > 0 && (source->numFrames != numFrames || source->numNodes != numNodes))
            return 0;

        zCList<zCModelAni>* entry = new zCList<zCModelAni>;
        entry->data = source;
        entry->next = combAniList.next;
        combAniList.next = entry;
        numFrames = source->numFrames;
        numNodes = source->numNodes;
        last = source;
    }

    ascName = last->ascName;
    fpsRateSource = last->fpsRateSource;
    fpsRate = last->fpsRate;
    aniDir = last->aniDir;
    aniBBox3DObjSpace = last->aniBBox3DObjSpace;
    rootNodeIndex = last->rootNodeIndex;

    delete[] nodeList;
    nodeList = new zCModelNode*[numNodes];
    for (int i = 0; i < numNodes; ++i)
        nodeList[i] = last->nodeList[i];

    delete[] nodeIndexList.parray;
    nodeIndexList.numAlloc = last->nodeIndexList.numInArray;
    nodeIndexList.numInArray = last->nodeIndexList.numInArray;
    nodeIndexList.parray = new int[nodeIndexList.numAlloc];
    for (int i = 0; i < nodeIndexList.numInArray; ++i)
        nodeIndexList.parray[i] = last->nodeIndexList.parray[i];

    samplePosRangeMin = -800.0f;
    samplePosScaler = 1600.0f / 65535.0f;
    delete[] aniSampleMatrix;
    aniSampleMatrix = new zTMdl_AniSample[numFrames * numNodes];
    for (int frame = 0; frame < numFrames; ++frame) {
        for (int node = 0; node < numNodes; ++node) {
            SetTrans(frame, node, last->GetTrans(frame, node));
            SetQuat(frame, node, last->GetQuat(frame, node));
        }
    }

    return 1;
}

int zCModelPrototype::s_ignoreAnis;

void zCModelAni::SaveMAN(zCModelPrototype* model, const zSTRING& fileName)
{
    CorrectRootNodeIdleMovement();

    zSTRING target = fileName;
    if (!target.Length()) {
        target = model->modelProtoName;
        target.Append("-");
        target.Append(aniName);
        target.Append(".man");
    }

    zCFileBIN binFile;
    binFile.BinCreate(target);
    binFile.BinStartChunk(0xA000);
    binFile.BinStartChunk(0xA020);

    const unsigned short version = 12;
    binFile.file->Write(&version, 2);
    binFile.BinWriteString(aniName);
    binFile.BinWriteInt(layer);
    binFile.BinWriteInt(numFrames);
    binFile.BinWriteInt(numNodes);
    binFile.BinWriteFloat(fpsRate);
    binFile.BinWriteFloat(fpsRateSource);
    binFile.BinWriteFloat(samplePosRangeMin);
    binFile.BinWriteFloat(samplePosScaler);
    binFile.file->Write(&aniBBox3DObjSpace, sizeof(zTBBox3D));
    binFile.BinWriteString(nextAniName);

    binFile.BinStartChunk(0xA010);
    binFile.BinWriteSrcFileStats(ascName);

    if (numAniEvents > 0) {
        binFile.BinStartChunk(0xA030);
        binFile.BinWriteInt(numAniEvents);
        for (int i = 0; i < numAniEvents; ++i)
            aniEvents[i].Save(binFile);
    }

    binFile.BinStartChunk(0xA090);
    binFile.file->Write(&model->nodeListChecksum, 4);
    binFile.file->Write(nodeIndexList.parray, numNodes * 4);
    binFile.file->Write(aniSampleMatrix, numNodes * numFrames * sizeof(zTMdl_AniSample));
    binFile.BinClose();
}

int zCModelAni::LoadMAN(const zSTRING& fileName, zCModelPrototype* model, const zSTRING&)
{
    if (zCModelPrototype::s_ignoreAnis)
        return 1;

    zSTRING target = model->modelProtoName;
    target.Append("-");
    target.Append(fileName);
    target.Append(".man");

    zFILE_FILE* source = new zFILE_FILE(target);
    if (source->Open(false)) {
        delete source;
        return 0;
    }

    zCFileBIN binFile;
    binFile.BinSetFile(source);
    int result = 1;

    while (binFile.nextStart < source->Size()) {
        unsigned short chunkID = 0;
        int chunkSize = 0;
        source->Read(&chunkID, 2);
        if (source->Eof())
            break;
        source->Read(&chunkSize, 4);
        binFile.nextStart = source->Pos() + chunkSize;

        if (chunkID == 0xA010) {
            if (zCModelPrototype::s_autoConvertAnis && !binFile.BinReadCompareSrcFileStats()) {
                result = 0;
                break;
            }
        }
        else if (chunkID == 0xA020) {
            unsigned short version;
            source->Read(&version, 2);
            if (version != 12) {
                result = 0;
                break;
            }

            source->Read(aniName);
            source->Read(&layer, 4);
            source->Read(&numFrames, 4);
            source->Read(&numNodes, 4);
            source->Read(&fpsRate, 4);
            source->Read(&fpsRateSource, 4);
            source->Read(&samplePosRangeMin, 4);
            source->Read(&samplePosScaler, 4);
            source->Read(&aniBBox3DObjSpace, sizeof(zTBBox3D));
            source->Read(nextAniName);
        }
        else if (chunkID == 0xA030) {
            source->Read(&numAniEvents, 4);
            delete[] aniEvents;
            aniEvents = new zCModelAniEvent[numAniEvents];
            for (int i = 0; i < numAniEvents; ++i)
                aniEvents[i].Load(binFile);
        }
        else if (chunkID == 0xA090) {
            unsigned long checksum;
            source->Read(&checksum, 4);
            if (checksum != model->nodeListChecksum) {
                result = 0;
                break;
            }

            delete[] nodeIndexList.parray;
            nodeIndexList.numAlloc = numNodes;
            nodeIndexList.numInArray = numNodes;
            nodeIndexList.parray = new int[numNodes];
            source->Read(nodeIndexList.parray, numNodes * 4);

            delete[] nodeList;
            nodeList = new zCModelNode*[numNodes];
            for (int i = 0; i < numNodes; ++i) {
                const int nodeIndex = nodeIndexList.parray[i];
                nodeList[i] = nodeIndex >= 0 && nodeIndex < model->nodeList.numInArray
                    ? model->nodeList.parray[nodeIndex]
                    : 0;
                if (nodeList[i] && !nodeList[i]->parentNode)
                    rootNodeIndex = i;
            }

            delete[] aniSampleMatrix;
            const int sampleCount = numNodes * numFrames;
            aniSampleMatrix = new zTMdl_AniSample[sampleCount];
            source->Read(aniSampleMatrix, sampleCount * sizeof(zTMdl_AniSample));
        }

        if (binFile.nextStart < source->Size())
            source->Seek(binFile.nextStart);
    }

    binFile.BinClose();
    source->Close();
    delete source;
    return result;
}

zCModelAniActive::zCModelAniActive()
    : protoAni(0), nextAni(0), advanceDir(0), actFrame(0), actAniEvent(0),
      combAniX(0.5f), combAniY(0.5f), isFadingOut(0), isFirstTime(1),
      nextAniOverride(0), blendInOverride(0), blendOutOverride(0), rotFirstTime(1),
      transWeight(0), aniAttachment(0), randAniTimer(0)
{
    for (int i = 0; i < 3; ++i) {
        lastPos.n[i] = 0;
        thisPos.n[i] = 0;
    }

    for (int i = 0; i < 4; ++i) {
        lastRotQuat.q[i] = 0;
        thisRotQuat.q[i] = 0;
        freezeRotQuat.q[i] = 0;
    }
}

zCModelAniActive::~zCModelAniActive()
{
    protoAni = 0;
    nextAni = 0;
    nextAniOverride = 0;
    aniAttachment = 0;
}

void zCModelAniActive::SetDirection(zTMdl_AniDir direction)
{
    if (direction == zMDL_ANIDIR_FORWARD) {
        advanceDir = 1;
        return;
    }
    if (direction == zMDL_ANIDIR_ENDFASTEST && actFrame < protoAni->numFrames / 2) {
        advanceDir = 1;
        return;
    }

    advanceDir = -1;
}

float zCModelAniActive::GetProgressPercent() const
{
    if (advanceDir < 0)
        return 1.0f - actFrame / protoAni->numFrames;
    return actFrame / protoAni->numFrames;
}

void zCModelAniActive::SetProgressPercent(float progress)
{
    if (advanceDir < 0)
        actFrame = (protoAni->numFrames - 1) * (1.0f - progress);
    else
        actFrame = (protoAni->numFrames - 1) * progress;
}

void zCModelAniActive::SetActFrame(float frame)
{
    actFrame = frame;
    if (actFrame < 0)
        actFrame = 0;
    else if (actFrame > protoAni->numFrames - 1)
        actFrame = protoAni->numFrames - 1;
}

zCModelTexAniState::zCModelTexAniState()
    : numNodeTex(0), nodeTexList(0)
{
    for (int i = 0; i < zMDL_MAX_TEX; ++i)
        actAniFrames[0][i] = 0;
}

zCModelTexAniState::~zCModelTexAniState()
{
    DeleteTexList();
}

void zCModelTexAniState::DeleteTexList()
{
    delete[] nodeTexList;
    nodeTexList = 0;
    numNodeTex = 0;
}

void zCModelTexAniState::UpdateTexList()
{
    for (int i = 0; i < numNodeTex; ++i) {
        for (int channel = 0; channel < zMDL_MAX_ANI_CHANNELS; ++channel)
            nodeTexList[i]->actAniFrame[channel] = actAniFrames[channel][i];
    }
}

void zCModelTexAniState::SetChannelVariation(int channel, int frame, zSTRING* filter)
{
    if (channel >= zMDL_MAX_ANI_CHANNELS)
        return;

    for (int i = 0; i < numNodeTex; ++i) {
        if (!filter || nodeTexList[i]->GetObjectName().Search(*filter) >= 0)
            actAniFrames[channel][i] = frame;
    }
}

void zCModelTexAniState::BuildTexListFromMeshLib(zCModelMeshLib* meshLib)
{
    zCArray<zCTexture*> textures;
    ResetArray(textures);
    AddTexListFromMeshLib(meshLib, textures);
    FinishTexList(textures);
    delete[] textures.parray;
}

void zCModelTexAniState::BuildTexListFromMesh(zCMesh* mesh)
{
    zCArray<zCTexture*> textures;
    ResetArray(textures);
    AddTexListFromMesh(mesh, textures);
    FinishTexList(textures);
    delete[] textures.parray;
}

void zCModelTexAniState::BuildTexListFromProgMesh(zCProgMeshProto* mesh)
{
    zCArray<zCTexture*> textures;
    ResetArray(textures);
    AddTexListFromProgMesh(mesh, textures);
    FinishTexList(textures);
    delete[] textures.parray;
}

static void AddTextureUnique(zCArray<zCTexture*>& textures, zCTexture* texture)
{
    if (!texture)
        return;

    for (int i = 0; i < textures.numInArray; ++i) {
        if (textures.parray[i] == texture)
            return;
    }

    if (textures.numInArray >= textures.numAlloc) {
        const int delta = textures.numAlloc >= 16 ? textures.numAlloc / 2 : 16;
        const int newAlloc = textures.numAlloc + delta;
        zCTexture** newArray = new zCTexture*[newAlloc];
        for (int i = 0; i < textures.numInArray; ++i)
            newArray[i] = textures.parray[i];
        delete[] textures.parray;
        textures.parray = newArray;
        textures.numAlloc = newAlloc;
    }

    textures.parray[textures.numInArray++] = texture;
}

void zCModelTexAniState::AddTexListFromMeshLib(zCModelMeshLib* meshLib,
    zCArray<zCTexture*>& textures)
{
    for (int i = 0; i < meshLib->meshNodeList.numInArray; ++i) {
        zTNodeMesh& node = meshLib->meshNodeList.parray[i];
        if (node.visual)
            AddTexListFromMesh(reinterpret_cast<zCMesh*>(node.visual), textures);
    }
}

void zCModelTexAniState::AddTexListFromMesh(zCMesh* mesh, zCArray<zCTexture*>& textures)
{
    for (int i = 0; i < mesh->numPoly; ++i) {
        zCPolygon* polygon = mesh->polyArray ? &mesh->polyArray[i] : mesh->polyList[i];
        if (polygon && polygon->material)
            AddTextureUnique(textures, polygon->material->texture);
    }
}

void zCModelTexAniState::AddTexListFromProgMesh(zCProgMeshProto* mesh,
    zCArray<zCTexture*>& textures)
{
    for (int i = 0; i < mesh->numSubMeshes; ++i) {
        if (mesh->subMeshList[i].material)
            AddTextureUnique(textures, mesh->subMeshList[i].material->texture);
    }
}

void zCModelTexAniState::FinishTexList(zCArray<zCTexture*>& textures)
{
    DeleteTexList();
    while (textures.numInArray > zMDL_MAX_TEX)
        --textures.numInArray;

    numNodeTex = textures.numInArray;
    if (!numNodeTex)
        return;

    nodeTexList = new zCTexture*[numNodeTex];
    for (int i = 0; i < numNodeTex; ++i)
        nodeTexList[i] = textures.parray[i];
}

zCModelNodeInst::zCModelNodeInst()
{
    Init();
}

zCModelNodeInst::zCModelNodeInst(zCModelNode* node)
{
    InitByModelProtoNode(node);
}

zCModelNodeInst::~zCModelNodeInst()
{
    if (nodeVisual)
        nodeVisual->Release();
    nodeVisual = 0;
    parentNode = 0;
    protoNode = 0;
}

void zCModelNodeInst::Init()
{
    parentNode = 0;
    protoNode = 0;
    nodeVisual = 0;
    SetIdentity(trafo);
    SetIdentity(trafoObjToCam);
    for (int i = 0; i < 3; ++i) {
        bbox3D.mins.n[i] = 0;
        bbox3D.maxs.n[i] = 0;
    }
    numNodeAnis = 0;
    masterAni = -1;
    masterAniSpeed = 0;
}

void zCModelNodeInst::InitByModelProtoNode(zCModelNode* node)
{
    Init();
    protoNode = node;
    SetNodeVisualS(node->visual, 1);
    trafo = node->trafo;
}

void zCModelNodeInst::SetNodeVisualS(zCVisual* visual, int buildTexList)
{
    if (visual == nodeVisual)
        return;

    texAniState.DeleteTexList();
    if (nodeVisual)
        nodeVisual->Release();
    if (visual)
        visual->AddRef();
    nodeVisual = visual;

    if (nodeVisual && buildTexList)
        texAniState.BuildTexListFromMesh(reinterpret_cast<zCMesh*>(nodeVisual));
}

void zCModelNodeInst::AddNodeAni(zCModelAniActive* ani)
{
    if (numNodeAnis == zMDL_MAX_ANIS_PARALLEL)
        return;

    zTNodeAni& nodeAni = nodeAniList[numNodeAnis];
    nodeAni.modelAni = ani;
    nodeAni.weight = 0;
    nodeAni.weightSpeed = 0;
    nodeAni.blendState = zMDL_BLEND_STATE_FADEIN;

    if (!numNodeAnis) {
        nodeAni.weight = 1;
        nodeAni.blendState = zMDL_BLEND_STATE_CONST;
        masterAni = 0;
    }
    else if (masterAni == -1 || ani->protoAni->layer >= nodeAniList[masterAni].modelAni->protoAni->layer) {
        if (masterAni != -1) {
            nodeAniList[masterAni].weightSpeed = nodeAniList[masterAni].modelAni->blendOutOverride;
            nodeAniList[masterAni].blendState = zMDL_BLEND_STATE_FADEOUT;
        }
        nodeAni.weightSpeed = ani->blendInOverride;
        masterAni = numNodeAnis;
    }

    ++numNodeAnis;
}

void zCModelNodeInst::RemoveAllNodeAnis()
{
    numNodeAnis = 0;
}

void zCModelNodeInst::FindMasterAni()
{
    int highestLayer = -1;
    masterAni = -1;
    for (int i = 0; i < numNodeAnis; ++i) {
        const int layer = nodeAniList[i].modelAni->protoAni->layer;
        if (layer >= highestLayer && nodeAniList[i].blendState != zMDL_BLEND_STATE_FADEOUT) {
            highestLayer = layer;
            masterAni = i;
        }
    }

    if (masterAni != -1)
        nodeAniList[masterAni].weightSpeed = nodeAniList[masterAni].modelAni->blendInOverride;
}

void zCModelNodeInst::RemoveNodeAni(zCModelAniActive* ani)
{
    const int index = GetNodeAniListIndex(ani);
    if (index < 0)
        return;

    for (int i = index; i < numNodeAnis - 1; ++i)
        nodeAniList[i] = nodeAniList[i + 1];
    --numNodeAnis;

    if (masterAni == index)
        FindMasterAni();
    else if (masterAni > index)
        --masterAni;
}

void zCModelNodeInst::FadeOutNodeAni(zCModelAniActive* ani)
{
    const int index = GetNodeAniListIndex(ani);
    if (index < 0)
        return;

    nodeAniList[index].weightSpeed = ani->blendOutOverride;
    nodeAniList[index].blendState = zMDL_BLEND_STATE_FADEOUT;
    if (index == masterAni)
        FindMasterAni();
}

int zCModelNodeInst::GetNodeAniListIndex(const zCModelAniActive* ani) const
{
    for (int i = 0; i < numNodeAnis; ++i) {
        if (nodeAniList[i].modelAni == ani)
            return i;
    }
    return -1;
}

void zCModelNodeInst::AddToNodeAniWeight(int index, float value)
{
    zTNodeAni& nodeAni = nodeAniList[index];
    nodeAni.weight += value;
    if (nodeAni.weight > 1) {
        nodeAni.weight = 1;
        nodeAni.weightSpeed = 0;
        nodeAni.blendState = zMDL_BLEND_STATE_CONST;
    }
    else if (nodeAni.weight < 0) {
        nodeAni.weight = 0;
        nodeAni.weightSpeed = 0;
    }
}

zTAniAttachment::zTAniAttachment()
{
    ResetArray(randAniList);
}

zTAniAttachment::~zTAniAttachment()
{
    delete[] randAniList.parray;
    randAniList.parray = 0;
}

zCModel::zCModel(zCModelPrototype* model)
{
    ResetArray(modelProtoList);
    ResetArray(nodeList);
    ResetArray(meshSoftSkinList);
    ResetArray(occuredAniEvents);
    aniAttachList.array = 0;
    aniAttachList.numAlloc = 0;
    aniAttachList.numInArray = 0;
    aniAttachList.Compare = zArraySortDefaultCompare;
    Init();

    bbox3D = model->bbox3D;
    modelProtoList.numAlloc = 1;
    modelProtoList.numInArray = 1;
    modelProtoList.parray = new zCModelPrototype*[1];
    modelProtoList.parray[0] = model;

    meshSoftSkinList.numAlloc = model->meshSoftSkinList.numInArray;
    meshSoftSkinList.numInArray = model->meshSoftSkinList.numInArray;
    meshSoftSkinList.parray = new zCMeshSoftSkin*[meshSoftSkinList.numAlloc];
    for (int i = 0; i < meshSoftSkinList.numInArray; ++i) {
        meshSoftSkinList.parray[i] = model->meshSoftSkinList.parray[i];
        meshSoftSkinList.parray[i]->AddRef();
    }

    CopyProtoNodeList();
    const zVEC3 scale = {{1, 1, 1}};
    SetModelScale(scale);
}

void zCModel::Init()
{
    for (int i = 0; i < 8; ++i) {
        modelVelRing[i].n[0] = 0;
        modelVelRing[i].n[1] = 0;
        modelVelRing[i].n[2] = 0;
    }

    modelVelocity.n[0] = 0;
    modelVelocity.n[1] = 0;
    modelVelocity.n[2] = 0;
    vobTrans = modelVelocity;
    vobTransRing = modelVelocity;
    rootPosLocal = modelVelocity;
    homeVob = 0;
    numActiveAnis = 0;
    activeAniList = 0;
    lastTimeBBox3DTreeUpdate = 0;
    actVelRingPos = 0;
    relaxWeight = 0.1f;
    newAniStarted = 1;
    isVisible = 0;
    vobRot.q[0] = 0;
    vobRot.q[1] = 0;
    vobRot.q[2] = 0;
    vobRot.q[3] = 1;
    timeScale = 1;
    aniHistoryList = 0;
    fatness = 0;
    aniTransScale.n[0] = 1;
    aniTransScale.n[1] = 1;
    aniTransScale.n[2] = 1;
    modelDistanceToCam = 0;
    m_bDrawHandVisualsOnly = 0;
    n_bIsInMobInteraction = 0;
    m_bSmoothRootNode = 1;

    const zVEC3 scale = {{1, 1, 1}};
    SetModelScale(scale);

    for (int i = 0; i < numActiveAnis; ++i) {
        zCModelAniActive* ani = aniChannels[i];
        if (ani->aniAttachment && ani->aniAttachment->randAniList.numInArray > 0)
            ani->randAniTimer = ani->aniAttachment->randAniFreq * 1000.0f;
    }

    for (int i = 0; i < 3; ++i) {
        bbox3D.mins.n[i] = 0;
        bbox3D.maxs.n[i] = 0;
    }
    bbox3DLocalFixed = bbox3D;
    bbox3DCollDet = bbox3D;
}

void zCModel::CopyProtoNodeList()
{
    if (!modelProtoList.numInArray || !modelProtoList.parray[0])
        return;

    zCModelPrototype* model = modelProtoList.parray[0];
    for (int i = 0; i < model->nodeList.numInArray; ++i) {
        zCModelNode* protoNode = model->nodeList.parray[i];
        zCModelNodeInst* instance = new zCModelNodeInst(protoNode);

        if (protoNode->parentNode) {
            for (int j = 0; j < nodeList.numInArray; ++j) {
                if (nodeList.parray[j]->protoNode == protoNode->parentNode) {
                    instance->parentNode = nodeList.parray[j];
                    break;
                }
            }
        }

        protoNode->lastInstNode = instance;
        if (nodeList.numInArray >= nodeList.numAlloc) {
            const int newAlloc = nodeList.numAlloc + 16;
            zCModelNodeInst** newArray = new zCModelNodeInst*[newAlloc];
            for (int j = 0; j < nodeList.numInArray; ++j)
                newArray[j] = nodeList.parray[j];
            delete[] nodeList.parray;
            nodeList.parray = newArray;
            nodeList.numAlloc = newAlloc;
        }
        nodeList.parray[nodeList.numInArray++] = instance;
    }
}

void zCModel::SetVisualUsedBy(zCVob* vob)
{
    homeVob = vob;
    if (vob) {
        vob->castDynShadow = (vob->castDynShadow & ~3) | 1;
        vob->m_zBias = 0;
    }
}

const zSTRING* zCModel::GetFileExtension(int index)
{
    static zSTRING extensions[2] = {".MDS", ".ASC"};
    return index >= 0 && index <= 1 ? &extensions[index] : 0;
}

int zCModel::ApplyModelProtoOverlay(const zSTRING& fileName)
{
    if (!modelProtoList.numInArray)
        return 0;

    zCModelPrototype* overlay = zCModelPrototype::Load(fileName, modelProtoList.parray[0]);
    if (!overlay)
        return 0;

    const int result = ApplyModelProtoOverlay(overlay);
    overlay->Release();
    return result;
}

int zCModel::ApplyModelProtoOverlay(zCModelPrototype* model)
{
    if (!model || !modelProtoList.numInArray || HasAppliedModelProtoOverlay(model))
        return 0;
    if (model->baseModelProto != modelProtoList.parray[0])
        return 0;

    if (modelProtoList.numInArray >= modelProtoList.numAlloc) {
        const int newAlloc = modelProtoList.numAlloc + 16;
        zCModelPrototype** newArray = new zCModelPrototype*[newAlloc];
        for (int i = 0; i < modelProtoList.numInArray; ++i)
            newArray[i] = modelProtoList.parray[i];
        delete[] modelProtoList.parray;
        modelProtoList.parray = newArray;
        modelProtoList.numAlloc = newAlloc;
    }

    modelProtoList.parray[modelProtoList.numInArray++] = model;
    ++model->refCtr;
    return 1;
}

int zCModel::HasAppliedModelProtoOverlay(zCModelPrototype* model) const
{
    for (int i = 0; i < modelProtoList.numInArray; ++i) {
        if (modelProtoList.parray[i] == model)
            return 1;
    }
    return 0;
}

int zCModel::HasAppliedModelProtoOverlay(const zSTRING& fileName) const
{
    zPATH path(fileName);
    const zSTRING name = path.GetFilename();
    for (int i = 0; i < modelProtoList.numInArray; ++i) {
        if (modelProtoList.parray[i]->modelProtoName == name)
            return 1;
    }
    return 0;
}

void zCModel::RemoveModelProtoOverlay(const zSTRING& fileName)
{
    zPATH path(fileName);
    const zSTRING name = path.GetFilename();
    for (int i = 0; i < modelProtoList.numInArray; ++i) {
        if (modelProtoList.parray[i]->modelProtoName == name) {
            RemoveModelProtoOverlay(modelProtoList.parray[i]);
            return;
        }
    }
}

void zCModel::RemoveModelProtoOverlay(zCModelPrototype* model)
{
    for (int i = 1; i < modelProtoList.numInArray; ++i) {
        if (modelProtoList.parray[i] != model)
            continue;

        for (int j = i; j < modelProtoList.numInArray - 1; ++j)
            modelProtoList.parray[j] = modelProtoList.parray[j + 1];
        --modelProtoList.numInArray;
        model->Release();
        return;
    }
}

zVEC3 zCModel::GetNodePositionWorld(zCModelNodeInst* node)
{
    CalcNodeListBBoxWorld();
    zVEC3 result;
    for (int i = 0; i < 3; ++i)
        result.n[i] = node->trafoObjToCam.m[i][3];
    return result;
}

void zCModel::CalcModelBBox3DWorld()
{
    if (numActiveAnis > 0) {
        for (int i = 0; i < 3; ++i) {
            bbox3D.mins.n[i] = 3.40282347e+38f;
            bbox3D.maxs.n[i] = -3.40282347e+38f;
        }

        for (int i = 0; i < numActiveAnis; ++i) {
            const zTBBox3D& other = aniChannels[i]->protoAni->aniBBox3DObjSpace;
            for (int j = 0; j < 3; ++j) {
                if (other.mins.n[j] < bbox3D.mins.n[j])
                    bbox3D.mins.n[j] = other.mins.n[j];
                if (other.maxs.n[j] > bbox3D.maxs.n[j])
                    bbox3D.maxs.n[j] = other.maxs.n[j];
            }
        }
    }
    else if (modelProtoList.numInArray > 0) {
        bbox3D = modelProtoList.parray[0]->bbox3D;
    }
    else {
        for (int i = 0; i < 3; ++i) {
            bbox3D.mins.n[i] = 0;
            bbox3D.maxs.n[i] = 0;
        }
    }

    for (int i = 0; i < 3; ++i) {
        bbox3D.mins.n[i] *= modelScale.n[i];
        bbox3D.maxs.n[i] *= modelScale.n[i];
    }
}

void zCModel::CalcNodeListBBoxWorld()
{
    for (int i = 0; i < nodeList.numInArray; ++i) {
        zCModelNodeInst* node = nodeList.parray[i];
        node->trafoObjToCam = node->trafo;
        if (!node->nodeVisual) {
            for (int j = 0; j < 3; ++j) {
                node->bbox3D.mins.n[j] = node->trafoObjToCam.m[j][3];
                node->bbox3D.maxs.n[j] = node->trafoObjToCam.m[j][3];
            }
        }
    }
}

zTBBox3D zCModel::GetBBox3DNodeWorld(zCModelNodeInst* node)
{
    CalcNodeListBBoxWorld();
    return node->bbox3D;
}

void zCModel::SetNodeVisual(zCModelNodeInst* node, zCVisual* visual, int buildTexList)
{
    node->SetNodeVisualS(visual, buildTexList);
    CalcModelBBox3DWorld();
}

void zCModel::SetDynLightMode(int mode)
{
    isVisible = (isVisible & 0x7F) | ((mode << 7) & 0x80);
}

zCModelAni* zCModel::GetAniFromAniID(int aniID) const
{
    if (aniID == -1)
        return 0;

    for (int i = modelProtoList.numInArray - 1; i >= 0; --i) {
        zCModelAni* ani = modelProtoList.parray[i]->protoAnis.array[aniID];
        if (ani)
            return ani;
    }
    return 0;
}

int zCModel::IsAniActive(zCModelAni* ani)
{
    return GetActiveAni(ani) != 0;
}

zCModelAniActive* zCModel::GetActiveAni(int aniID) const
{
    for (int i = 0; i < numActiveAnis; ++i) {
        if (aniChannels[i]->protoAni->aniID == aniID)
            return aniChannels[i];
    }
    return 0;
}

zCModelAniActive* zCModel::GetActiveAni(zCModelAni* ani) const
{
    for (int i = 0; i < numActiveAnis; ++i) {
        if (aniChannels[i]->protoAni == ani)
            return aniChannels[i];
    }
    return 0;
}

int zCModel::GetAniIDFromAniName(const zSTRING& name) const
{
    zCModelAni* ani = modelProtoList.parray[0]->SearchAni(name);
    return ani ? ani->aniID : -1;
}

void zCModel::StopAni(int aniID)
{
    if (aniID != -1)
        StopAni(GetAniFromAniID(aniID));
}

void zCModel::StopAni(zCModelAni* ani)
{
    if (ani)
        StopAni(GetActiveAni(ani));
}

void zCModel::StopAni(zCModelAniActive* ani)
{
    if (!ani)
        return;

    zCModelAni* protoAni = ani->protoAni;
    for (int i = 0; i < protoAni->numNodes; ++i) {
        const int nodeIndex = protoAni->nodeIndexList.parray[i];
        nodeList.parray[nodeIndex]->RemoveNodeAni(ani);
    }

    for (int i = 0; i < numActiveAnis; ++i) {
        if (aniChannels[i] != ani)
            continue;
        for (int j = i; j < numActiveAnis - 1; ++j)
            aniChannels[j] = aniChannels[j + 1];
        --numActiveAnis;
        break;
    }

    ani->actFrame = ani->advanceDir == 1 ? protoAni->numFrames : 0;
    ani->protoAni = 0;
}

void zCModel::StopAnisLayerRange(int minLayer, int maxLayer)
{
    for (int i = 0; i < numActiveAnis; ++i) {
        zCModelAniActive* ani = aniChannels[i];
        const int layer = ani->protoAni->layer;
        if (layer >= minLayer && layer <= maxLayer) {
            StopAni(ani);
            --i;
        }
    }
}

void zCModel::FadeOutAni(int aniID)
{
    if (aniID != -1)
        FadeOutAni(GetAniFromAniID(aniID));
}

void zCModel::FadeOutAni(zCModelAni* ani)
{
    if (ani)
        FadeOutAni(GetActiveAni(ani));
}

void zCModel::FadeOutAni(zCModelAniActive* ani)
{
    if (!ani || ani->isFadingOut || !ani->protoAni)
        return;

    for (int i = 0; i < ani->protoAni->numNodes; ++i) {
        const int nodeIndex = ani->protoAni->nodeIndexList.parray[i];
        nodeList.parray[nodeIndex]->FadeOutNodeAni(ani);
    }
    ani->isFadingOut = 1;
}

void zCModel::FadeOutAnisLayerRange(int minLayer, int maxLayer)
{
    for (int i = 0; i < numActiveAnis; ++i) {
        const int layer = aniChannels[i]->protoAni->layer;
        if (layer >= minLayer && layer <= maxLayer)
            FadeOutAni(aniChannels[i]);
    }
}

float zCModel::GetProgressPercent(const zSTRING& name) const
{
    return GetActiveAni(GetAniIDFromAniName(name))->GetProgressPercent();
}

float zCModel::GetProgressPercent(int aniID) const
{
    return GetActiveAni(aniID)->GetProgressPercent();
}

void zCModel::SetCombineAniXY(int aniID, float x, float y) const
{
    zCModelAniActive* ani = GetActiveAni(aniID);
    if (ani) {
        ani->combAniX = x;
        ani->combAniY = y;
    }
}

int zCModel::GetCombineAniXY(int aniID, float& x, float& y) const
{
    zCModelAniActive* ani = GetActiveAni(aniID);
    if (!ani) {
        x = 0;
        y = 0;
        return 0;
    }

    x = ani->combAniX;
    y = ani->combAniY;
    return 1;
}

void zCModel::StartAni(const zSTRING& name, int startMode)
{
    StartAni(GetAniIDFromAniName(name), startMode);
}

void zCModel::StartAni(int aniID, int startMode)
{
    StartAni(GetAniFromAniID(aniID), startMode);
}

void zCModel::StartAni(zCModelAni* ani, int startMode)
{
    if (!ani)
        return;

    for (int i = 0; i < numActiveAnis; ++i) {
        if (aniChannels[i]->protoAni == ani && startMode != 2)
            return;
        if (aniChannels[i]->protoAni->layer == ani->layer)
            StopAni(aniChannels[i]);
    }

    AssertActiveAniListAlloced();
    zCModelAniActive* active = 0;
    for (int i = 0; i < 6; ++i) {
        if (!activeAniList[i].protoAni) {
            active = &activeAniList[i];
            break;
        }
    }
    if (!active)
        return;

    active->protoAni = ani;
    active->nextAni = ani->nextAni;
    active->nextAniOverride = 0;
    active->blendInOverride = ani->blendInSpeed;
    active->blendOutOverride = ani->blendOutSpeed;
    active->SetDirection(ani->aniDir);
    active->actAniEvent = active->advanceDir < 0 ? ani->numAniEvents - 1 : 0;
    active->isFadingOut = 0;
    active->isFirstTime = 1;
    active->rotFirstTime = 1;
    if (startMode == 2)
        active->SetProgressPercent(0);

    active->aniAttachment = 0;
    for (int i = 0; i < aniAttachList.numInArray; ++i) {
        if (aniAttachList.array[i]->aniID == ani->aniID) {
            active->aniAttachment = aniAttachList.array[i];
            active->randAniTimer = active->aniAttachment->randAniFreq * 1000.0f;
            break;
        }
    }

    for (int i = 0; i < ani->numNodes; ++i)
        nodeList.parray[ani->nodeIndexList.parray[i]]->AddNodeAni(active);

    aniChannels[numActiveAnis++] = active;
    newAniStarted = 1;

    if (!aniHistoryList)
        aniHistoryList = new zCModelAni*[16]();
    for (int i = 1; i < 16; ++i)
        aniHistoryList[i - 1] = aniHistoryList[i];
    aniHistoryList[15] = ani;
    ani->PrecacheAniEventData();
}

void zCModel::AssertActiveAniListAlloced()
{
    if (!activeAniList)
        activeAniList = new zCModelAniActive[6];
}

static void AppendOccuredEvent(zCArray<zCModelAniEvent*>& events, zCModelAniEvent* event)
{
    if (events.numInArray >= events.numAlloc) {
        const int newAlloc = events.numAlloc + 16;
        zCModelAniEvent** newArray = new zCModelAniEvent*[newAlloc];
        for (int i = 0; i < events.numInArray; ++i)
            newArray[i] = events.parray[i];
        delete[] events.parray;
        events.parray = newArray;
        events.numAlloc = newAlloc;
    }
    events.parray[events.numInArray++] = event;
}

void zCModel::DoAniEvents(zCModelAniActive* ani)
{
    if (!homeVob || !ani->protoAni)
        return;

    const int frame = static_cast<int>(ani->actFrame);
    while (ani->actAniEvent >= 0 && ani->actAniEvent < ani->protoAni->numAniEvents) {
        zCModelAniEvent& event = ani->protoAni->aniEvents[ani->actAniEvent];
        if (ani->advanceDir > 0 && event.frameNr > frame)
            break;
        if (ani->advanceDir < 0 && event.frameNr < frame)
            break;
        if (!ani->advanceDir)
            break;

        AppendOccuredEvent(occuredAniEvents, &event);
        ani->actAniEvent += ani->advanceDir;
    }
}

void zCModel::AdvanceAni(zCModelAniActive* ani, int& frame)
{
    frame = static_cast<int>(ani->actFrame);
    DoAniEvents(ani);

    if (ani->actFrame >= 0 && ani->actFrame < ani->protoAni->numFrames)
        return;

    zCModelAni* next = ani->nextAniOverride ? ani->nextAniOverride : ani->nextAni;
    if (next)
        StartAni(next, 1);
    else
        StopAni(ani);
}

void zCModel::AdvanceAnis()
{
    struct zTIMER {
        int   frameTime;
        float frameTimeFloat;
    };
    extern zTIMER ztimer;

    occuredAniEvents.numInArray = 0;
    vobTrans.n[0] = 0;
    vobTrans.n[1] = 0;
    vobTrans.n[2] = 0;

    for (int i = 0; i < numActiveAnis;) {
        zCModelAniActive* ani = aniChannels[i];
        ani->actFrame += ani->advanceDir * ani->protoAni->fpsRate * 0.001f *
            timeScale * ztimer.frameTimeFloat;
        int frame;
        AdvanceAni(ani, frame);
        if (i < numActiveAnis && aniChannels[i] == ani)
            ++i;
    }

    CalcModelBBox3DWorld();
}

void zCModel::SetModelScale(const zVEC3& scale)
{
    modelScale = scale;
    const bool enabled = scale.n[0] != 1 || scale.n[1] != 1 || scale.n[2] != 1;
    isVisible = (isVisible & ~0x10) | (enabled ? 0x10 : 0);

    if (modelProtoList.numInArray > 0) {
        bbox3DLocalFixed = modelProtoList.parray[0]->bbox3D;
        bbox3DCollDet = modelProtoList.parray[0]->bbox3DCollDet;
        if (enabled) {
            for (int i = 0; i < 3; ++i) {
                bbox3DLocalFixed.mins.n[i] *= scale.n[i];
                bbox3DLocalFixed.maxs.n[i] *= scale.n[i];
                bbox3DCollDet.mins.n[i] *= scale.n[i];
                bbox3DCollDet.maxs.n[i] *= scale.n[i];
            }
        }
    }

    CalcModelBBox3DWorld();
}

int zCModel::IsStateActive(const zCModelAni* ani) const
{
    for (int i = 0; i < numActiveAnis; ++i) {
        if (aniChannels[i]->protoAni == ani)
            return 1;
    }
    return 0;
}

zTBBox3D zCModel::GetBBox3D()
{
    return bbox3D;
}

zSTRING zCModel::GetVisualName()
{
    if (modelProtoList.numInArray > 0 && modelProtoList.parray[0])
        return modelProtoList.parray[0]->GetModelProtoFileName();
    return zSTRING();
}

zCModelNodeInst* zCModel::SearchNode(const zSTRING& name)
{
    for (int i = 0; i < nodeList.numInArray; ++i) {
        if (nodeList.parray[i]->protoNode->nodeName == name)
            return nodeList.parray[i];
    }
    return 0;
}

zVEC3 zCModel::GetVelocityRing() const
{
    zVEC3 result = {{0, 0, 0}};
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 3; ++j)
            result.n[j] += modelVelRing[i].n[j];
    }
    for (int i = 0; i < 3; ++i)
        result.n[i] *= 0.125f;
    return result;
}

void zCModel::ResetVelocity()
{
    for (int i = 0; i < 8; ++i) {
        modelVelRing[i].n[0] = 0;
        modelVelRing[i].n[1] = 0;
        modelVelRing[i].n[2] = 0;
    }
    modelVelocity.n[0] = 0;
    modelVelocity.n[1] = 0;
    modelVelocity.n[2] = 0;
}

int zCModel::SetNodeMeshTexture(const zSTRING& nodeName, int channel, int frame, zSTRING* filter)
{
    for (int i = 0; i < nodeList.numInArray; ++i) {
        zCModelNodeInst* node = nodeList.parray[i];
        if (node->protoNode->nodeName != nodeName)
            continue;
        if (node->protoNode->nodeName.Length() >= 3 && node->protoNode->nodeName[0] == 'Z' &&
            node->protoNode->nodeName[1] == 'S' && node->protoNode->nodeName[2] == '_')
            return 0;

        node->texAniState.SetChannelVariation(channel, frame, filter);
        return 1;
    }
    return 0;
}

int zCModel::SetMeshLibTexture(const zSTRING& fileName, int channel, int frame, zSTRING* filter)
{
    zPATH path(fileName);
    zSTRING name = path.GetFilename();
    name.Upper();

    for (int i = 0; i < meshLibList.numInArray; ++i) {
        zTMeshLibEntry* entry = meshLibList.parray[i];
        zSTRING entryName = entry->meshLib->GetObjectName();
        entryName.Upper();
        if (entryName == name) {
            entry->texAniState.SetChannelVariation(channel, frame, filter);
            return 1;
        }
    }
    return 0;
}

void zCModel::UpdateMeshLibTexAniState()
{
    for (int i = 0; i < meshLibList.numInArray; ++i)
        meshLibList.parray[i]->texAniState.UpdateTexList();
}

void zCModel::RemoveMeshLibAll()
{
    for (int i = meshLibList.numInArray - 1; i >= 0; --i)
        RemoveMeshLib(meshLibList.parray[i]->meshLib->GetObjectName());
}

int zCModel::RemoveMeshLib(const zSTRING& fileName)
{
    zPATH path(fileName);
    zSTRING name = path.GetFilename();
    name.Upper();

    for (int i = 0; i < meshLibList.numInArray; ++i) {
        zTMeshLibEntry* entry = meshLibList.parray[i];
        zSTRING entryName = entry->meshLib->GetObjectName();
        entryName.Upper();
        if (entryName != name)
            continue;

        zCModelMeshLib* meshLib = entry->meshLib;
        delete entry;
        for (int j = i; j < meshLibList.numInArray - 1; ++j)
            meshLibList.parray[j] = meshLibList.parray[j + 1];
        --meshLibList.numInArray;
        meshLib->RemoveFromModel(this);
        meshLib->Release();
        return 1;
    }
    return 0;
}

int zCModel::ApplyMeshLib(const zSTRING& fileName)
{
    if (!fileName.Length())
        return 0;

    zCModelMeshLib* meshLib = 0;
    if (!zCModelMeshLib::LoadMDM(fileName, 0, this, &meshLib))
        return 0;

    meshLib->ApplyToModel(this);
    zTMeshLibEntry* entry = new zTMeshLibEntry;
    entry->meshLib = meshLib;
    entry->texAniState.BuildTexListFromMeshLib(meshLib);

    if (meshLibList.numInArray >= meshLibList.numAlloc) {
        const int newAlloc = meshLibList.numAlloc + 16;
        zTMeshLibEntry** newArray = new zTMeshLibEntry*[newAlloc];
        for (int i = 0; i < meshLibList.numInArray; ++i)
            newArray[i] = meshLibList.parray[i];
        delete[] meshLibList.parray;
        meshLibList.parray = newArray;
        meshLibList.numAlloc = newAlloc;
    }
    meshLibList.parray[meshLibList.numInArray++] = entry;
    return 1;
}

int zCModel::AniAttachmentCompare(const void* ele1, const void* ele2)
{
    const int left = **static_cast<int* const*>(ele1);
    const int right = **static_cast<int* const*>(ele2);
    return left < right ? -1 : left > right;
}

zTAniAttachment* zCModel::SearchAniAttachList(int aniID) const
{
    for (int i = 0; i < aniAttachList.numInArray; ++i) {
        if (aniAttachList.array[i]->aniID == aniID)
            return aniAttachList.array[i];
    }
    return 0;
}

void zCModel::RemoveAniAttachment(int aniID)
{
    for (int i = 0; i < aniAttachList.numInArray; ++i) {
        if (aniAttachList.array[i]->aniID != aniID)
            continue;

        delete aniAttachList.array[i];
        for (int j = i; j < aniAttachList.numInArray - 1; ++j)
            aniAttachList.array[j] = aniAttachList.array[j + 1];
        --aniAttachList.numInArray;
        return;
    }
}

void zCModel::RemoveAllAniAttachments()
{
    for (int i = 0; i < aniAttachList.numInArray; ++i)
        delete aniAttachList.array[i];
    aniAttachList.numInArray = 0;
}

zTAniAttachment* zCModel::GetCreateAniAttachment(int aniID)
{
    zTAniAttachment* attachment = SearchAniAttachList(aniID);
    if (attachment)
        return attachment;

    attachment = new zTAniAttachment;
    attachment->aniID = aniID;
    attachment->randAniFreq = 1;
    attachment->randAniProbSum = 0;

    if (aniAttachList.numInArray >= aniAttachList.numAlloc) {
        const int newAlloc = aniAttachList.numAlloc + 16;
        zTAniAttachment** newArray = new zTAniAttachment*[newAlloc];
        for (int i = 0; i < aniAttachList.numInArray; ++i)
            newArray[i] = aniAttachList.array[i];
        delete[] aniAttachList.array;
        aniAttachList.array = newArray;
        aniAttachList.numAlloc = newAlloc;
    }

    int index = aniAttachList.numInArray;
    while (index > 0 && aniAttachList.array[index - 1]->aniID > aniID) {
        aniAttachList.array[index] = aniAttachList.array[index - 1];
        --index;
    }
    aniAttachList.array[index] = attachment;
    ++aniAttachList.numInArray;
    return attachment;
}

void zCModel::DeleteRandAniList(int aniID)
{
    zTAniAttachment* attachment = SearchAniAttachList(aniID);
    if (attachment)
        attachment->randAniList.numInArray = 0;
}

void zCModel::InsertRandAni(int aniID, int randAniProtoID, int prob)
{
    if (aniID == -1)
        return;

    zTAniAttachment* attachment = GetCreateAniAttachment(aniID);
    zCArray<zTRandAni>& list = attachment->randAniList;
    if (list.numInArray >= list.numAlloc) {
        const int newAlloc = list.numAlloc + 16;
        zTRandAni* newArray = new zTRandAni[newAlloc];
        for (int i = 0; i < list.numInArray; ++i)
            newArray[i] = list.parray[i];
        delete[] list.parray;
        list.parray = newArray;
        list.numAlloc = newAlloc;
    }

    list.parray[list.numInArray].randAniProtoID = randAniProtoID;
    list.parray[list.numInArray].prob = prob;
    ++list.numInArray;
    attachment->randAniProbSum += prob;
}

float zCModel::GetRandAniFreq(int aniID) const
{
    zTAniAttachment* attachment = SearchAniAttachList(aniID);
    return attachment ? attachment->randAniFreq : 0;
}

void zCModel::SetRandAniFreq(int aniID, float)
{
    GetCreateAniAttachment(aniID)->randAniFreq = *reinterpret_cast<float*>(&aniID);
}

void zCModel::RecalcRootPosLocal(int highest)
{
    rootPosLocal.n[0] = 0;
    rootPosLocal.n[1] = 0;
    rootPosLocal.n[2] = 0;

    for (int i = 0; i < numActiveAnis; ++i) {
        zCModelAniActive* ani = aniChannels[i];
        if (ani->protoAni->rootNodeIndex < 0)
            continue;

        const int frame = static_cast<int>(std::floor(ani->actFrame));
        const zVEC3 position = ani->protoAni->GetTrans(frame, ani->protoAni->rootNodeIndex);
        if ((highest && position.n[1] >= rootPosLocal.n[1]) ||
            (!highest && position.n[1] < rootPosLocal.n[1]))
            rootPosLocal = position;
    }

    if (isVisible & 0x10) {
        for (int i = 0; i < 3; ++i)
            rootPosLocal.n[i] *= modelScale.n[i];
    }
}

void zCModelMeshLib::ApplyToModel(zCModel* model)
{
    for (int i = 0; i < meshNodeList.numInArray; ++i) {
        const zTNodeMesh& node = meshNodeList.parray[i];
        zCModelNodeInst* instance = model->nodeList.parray[node.nodeIndex];
        if (instance)
            instance->SetNodeVisualS(node.visual, 0);
    }

    for (int i = 0; i < meshSoftSkinList.numInArray; ++i) {
        zCMeshSoftSkin* skin = meshSoftSkinList.parray[i];
        skin->AddRef();
        if (model->meshSoftSkinList.numInArray >= model->meshSoftSkinList.numAlloc) {
            const int newAlloc = model->meshSoftSkinList.numAlloc + 16;
            zCMeshSoftSkin** newArray = new zCMeshSoftSkin*[newAlloc];
            for (int j = 0; j < model->meshSoftSkinList.numInArray; ++j)
                newArray[j] = model->meshSoftSkinList.parray[j];
            delete[] model->meshSoftSkinList.parray;
            model->meshSoftSkinList.parray = newArray;
            model->meshSoftSkinList.numAlloc = newAlloc;
        }
        model->meshSoftSkinList.parray[model->meshSoftSkinList.numInArray++] = skin;
    }
}

void zCModelMeshLib::RemoveFromModel(zCModel* model)
{
    for (int i = 0; i < meshNodeList.numInArray; ++i) {
        const zTNodeMesh& node = meshNodeList.parray[i];
        zCModelNodeInst* instance = model->nodeList.parray[node.nodeIndex];
        if (instance && instance->nodeVisual == node.visual)
            instance->SetNodeVisualS(0, 0);
    }

    for (int i = 0; i < meshSoftSkinList.numInArray; ++i) {
        zCMeshSoftSkin* skin = meshSoftSkinList.parray[i];
        for (int j = 0; j < model->meshSoftSkinList.numInArray; ++j) {
            if (model->meshSoftSkinList.parray[j] != skin)
                continue;
            --model->meshSoftSkinList.numInArray;
            model->meshSoftSkinList.parray[j] =
                model->meshSoftSkinList.parray[model->meshSoftSkinList.numInArray];
            skin->Release();
            break;
        }
    }
}

void zCModelMeshLib::ApplyToModel(zCModelPrototype* model)
{
    for (int i = 0; i < meshNodeList.numInArray; ++i) {
        const zTNodeMesh& node = meshNodeList.parray[i];
        model->nodeList.parray[node.nodeIndex]->SetNodeVisualS(node.visual);
    }

    for (int i = 0; i < meshSoftSkinList.numInArray; ++i) {
        zCMeshSoftSkin* skin = meshSoftSkinList.parray[i];
        skin->AddRef();
        if (model->meshSoftSkinList.numInArray >= model->meshSoftSkinList.numAlloc) {
            const int newAlloc = model->meshSoftSkinList.numAlloc + 16;
            zCMeshSoftSkin** newArray = new zCMeshSoftSkin*[newAlloc];
            for (int j = 0; j < model->meshSoftSkinList.numInArray; ++j)
                newArray[j] = model->meshSoftSkinList.parray[j];
            delete[] model->meshSoftSkinList.parray;
            model->meshSoftSkinList.parray = newArray;
            model->meshSoftSkinList.numAlloc = newAlloc;
        }
        model->meshSoftSkinList.parray[model->meshSoftSkinList.numInArray++] = skin;
    }
}

zSTRING zCModelMeshLib::GetObjectName() const
{
    return objectName;
}

void zCModelMeshLib::BuildFromModel(zCModel* model)
{
    for (int i = 0; i < meshNodeList.numInArray; ++i) {
        if (meshNodeList.parray[i].visual)
            meshNodeList.parray[i].visual->Release();
    }
    meshNodeList.numInArray = 0;
    for (int i = 0; i < meshSoftSkinList.numInArray; ++i)
        meshSoftSkinList.parray[i]->Release();
    meshSoftSkinList.numInArray = 0;
    objectName = model->modelProtoList.parray[0]->modelProtoName;

    for (int i = 0; i < model->nodeList.numInArray; ++i) {
        zCModelNodeInst* instance = model->nodeList.parray[i];
        if (!instance->nodeVisual)
            continue;
        if (instance->protoNode->nodeName.Length() >= 3 && instance->protoNode->nodeName[0] == 'Z' &&
            instance->protoNode->nodeName[1] == 'S' && instance->protoNode->nodeName[2] == '_')
            continue;

        if (meshNodeList.numInArray >= meshNodeList.numAlloc) {
            const int newAlloc = meshNodeList.numAlloc + 16;
            zTNodeMesh* newArray = new zTNodeMesh[newAlloc];
            for (int j = 0; j < meshNodeList.numInArray; ++j)
                newArray[j] = meshNodeList.parray[j];
            delete[] meshNodeList.parray;
            meshNodeList.parray = newArray;
            meshNodeList.numAlloc = newAlloc;
        }

        zTNodeMesh& node = meshNodeList.parray[meshNodeList.numInArray++];
        node.nodeIndex = i;
        node.visual = instance->nodeVisual;
        node.visual->AddRef();
    }

    for (int i = 0; i < model->meshSoftSkinList.numInArray; ++i) {
        zCMeshSoftSkin* skin = model->meshSoftSkinList.parray[i];
        skin->AddRef();
        if (meshSoftSkinList.numInArray >= meshSoftSkinList.numAlloc) {
            const int newAlloc = meshSoftSkinList.numAlloc + 16;
            zCMeshSoftSkin** newArray = new zCMeshSoftSkin*[newAlloc];
            for (int j = 0; j < meshSoftSkinList.numInArray; ++j)
                newArray[j] = meshSoftSkinList.parray[j];
            delete[] meshSoftSkinList.parray;
            meshSoftSkinList.parray = newArray;
            meshSoftSkinList.numAlloc = newAlloc;
        }
        meshSoftSkinList.parray[meshSoftSkinList.numInArray++] = skin;
    }
}

void zCModelMeshLib::AllocNumNodeVisuals(int count)
{
    if (meshNodeList.numAlloc >= count)
        return;

    zTNodeMesh* newArray = new zTNodeMesh[count];
    for (int i = 0; i < meshNodeList.numInArray; ++i)
        newArray[i] = meshNodeList.parray[i];
    delete[] meshNodeList.parray;
    meshNodeList.parray = newArray;
    meshNodeList.numAlloc = count;
}

void zCModelMeshLib::AddMeshSoftSkin(zCMeshSoftSkin* skin)
{
    skin->AddRef();
    if (meshSoftSkinList.numInArray >= meshSoftSkinList.numAlloc) {
        const int newAlloc = meshSoftSkinList.numAlloc + 16;
        zCMeshSoftSkin** newArray = new zCMeshSoftSkin*[newAlloc];
        for (int i = 0; i < meshSoftSkinList.numInArray; ++i)
            newArray[i] = meshSoftSkinList.parray[i];
        delete[] meshSoftSkinList.parray;
        meshSoftSkinList.parray = newArray;
        meshSoftSkinList.numAlloc = newAlloc;
    }
    meshSoftSkinList.parray[meshSoftSkinList.numInArray++] = skin;
}

void zCModelMeshLib::AddNodeVisual(int nodeIndex, zCVisual* visual)
{
    if (nodeIndex < 0)
        nodeIndex = 0;
    if (!visual)
        return;

    visual->AddRef();
    if (meshNodeList.numInArray >= meshNodeList.numAlloc) {
        const int newAlloc = meshNodeList.numAlloc + 16;
        zTNodeMesh* newArray = new zTNodeMesh[newAlloc];
        for (int i = 0; i < meshNodeList.numInArray; ++i)
            newArray[i] = meshNodeList.parray[i];
        delete[] meshNodeList.parray;
        meshNodeList.parray = newArray;
        meshNodeList.numAlloc = newAlloc;
    }

    zTNodeMesh& node = meshNodeList.parray[meshNodeList.numInArray++];
    node.nodeIndex = nodeIndex;
    node.visual = visual;
}

unsigned long zCModelMeshLib::GetMDMFileVersion()
{
    return ((zCMeshSoftSkin::GetFileVersion() << 16) + zCProgMeshProto::GetFileVersion()) << 8 | 6;
}

zCModelMeshLib::zCModelMeshLib()
    : refCtr(1)
{
    ResetArray(meshNodeList);
    ResetArray(meshSoftSkinList);
}

void zCModelMeshLib::ReleaseData()
{
    for (int i = 0; i < meshNodeList.numInArray; ++i) {
        if (meshNodeList.parray[i].visual)
            meshNodeList.parray[i].visual->Release();
    }
    meshNodeList.numInArray = 0;

    for (int i = 0; i < meshSoftSkinList.numInArray; ++i)
        meshSoftSkinList.parray[i]->Release();
    meshSoftSkinList.numInArray = 0;
}

void zCModelMeshLib::SaveMDM(zCModelPrototype* model)
{
    zSTRING fileName = objectName;
    fileName.Append(".mdm");
    zCFileBIN binFile;
    binFile.BinCreate(fileName);
    SaveMDM(binFile, model);
    binFile.BinClose();
}

void zCModelMeshLib::SaveMDM(zCFileBIN& binFile, zCModelPrototype* model)
{
    binFile.BinStartChunk(0xD000);
    binFile.BinWriteInt(GetMDMFileVersion());

    binFile.BinStartChunk(0xD010);
    zSTRING sourceName = objectName;
    sourceName.Append(".ASC");
    binFile.BinWriteSrcFileStats(sourceName);

    if (meshNodeList.numInArray > 0) {
        binFile.BinStartChunk(0xD020);
        const unsigned short count = meshNodeList.numInArray;
        binFile.file->Write(&count, 2);
        for (int i = 0; i < meshNodeList.numInArray; ++i) {
            const int nodeIndex = meshNodeList.parray[i].nodeIndex;
            binFile.BinWriteString(model->nodeList.parray[nodeIndex]->nodeName);
        }
        for (int i = 0; i < meshNodeList.numInArray; ++i) {
            const int nodeIndex = meshNodeList.parray[i].nodeIndex;
            model->nodeList.parray[nodeIndex]->visual->SaveBIN(binFile);
        }
    }

    if (meshSoftSkinList.numInArray > 0) {
        binFile.BinStartChunk(0xD030);
        binFile.file->Write(&model->nodeListChecksum, 4);
        const unsigned short count = meshSoftSkinList.numInArray;
        binFile.file->Write(&count, 2);
        for (int i = 0; i < meshSoftSkinList.numInArray; ++i)
            meshSoftSkinList.parray[i]->SaveBIN(binFile);
    }

    binFile.BinStartChunk(0xD040);
}

int zCModelMeshLib::LoadMDM(const zSTRING& fileName, zCModelPrototype* proto, zCModel* model,
    zCModelMeshLib** meshLib)
{
    return LoadMDM_Try(fileName, proto, model, meshLib);
}

int zCModelMeshLib::LoadMDM_Try(const zSTRING& fileName, zCModelPrototype* proto, zCModel* model,
    zCModelMeshLib** meshLib)
{
    zFILE_FILE* source = new zFILE_FILE(fileName);
    if (source->Open(false)) {
        delete source;
        return 0;
    }

    zCFileBIN binFile;
    binFile.BinSetFile(source);
    const int result = LoadMDM(binFile, proto, model, meshLib);
    binFile.BinClose();
    source->Close();
    delete source;
    return result;
}

int zCModelMeshLib::LoadMDM(zCFileBIN& binFile, zCModelPrototype* proto, zCModel* model,
    zCModelMeshLib** meshLib)
{
    if (!meshLib)
        return 0;

    *meshLib = new zCModelMeshLib();
    (*meshLib)->objectName = binFile.file->GetFilename();

    while (binFile.nextStart < binFile.file->Size()) {
        unsigned short chunkID = 0;
        int chunkSize = 0;
        binFile.file->Read(&chunkID, 2);
        if (binFile.file->Eof())
            break;
        binFile.file->Read(&chunkSize, 4);
        binFile.nextStart = binFile.file->Pos() + chunkSize;

        if (chunkID == 0xD000) {
            int version;
            binFile.file->Read(&version, 4);
            if (version != static_cast<int>(GetMDMFileVersion())) {
                (*meshLib)->Release();
                *meshLib = 0;
                return 0;
            }
        }
        else if (chunkID == 0xD010) {
            if (zCModelPrototype::s_autoConvertAnis && !binFile.BinReadCompareSrcFileStats()) {
                (*meshLib)->Release();
                *meshLib = 0;
                return 0;
            }
        }
        else if (chunkID == 0xD020) {
            unsigned short count;
            binFile.file->Read(&count, 2);
            int* nodeIndices = new int[count];
            for (int i = 0; i < count; ++i) {
                zSTRING nodeName;
                binFile.file->Read(nodeName);
                nodeIndices[i] = proto->FindNodeListIndex(nodeName);
            }
            for (int i = 0; i < count; ++i) {
                zCVisual* visual = new zCVisual();
                visual->LoadBIN(binFile);
                (*meshLib)->AddNodeVisual(nodeIndices[i], visual);
                visual->Release();
            }
            delete[] nodeIndices;
        }
        else if (chunkID == 0xD030) {
            unsigned long checksum;
            unsigned short count;
            binFile.file->Read(&checksum, 4);
            binFile.file->Read(&count, 2);
            if (checksum != proto->nodeListChecksum) {
                (*meshLib)->Release();
                *meshLib = 0;
                return 0;
            }
            for (int i = 0; i < count; ++i) {
                zCMeshSoftSkin* skin = new zCMeshSoftSkin();
                skin->LoadBIN(binFile);
                (*meshLib)->AddMeshSoftSkin(skin);
                skin->Release();
            }
        }
        else if (chunkID == 0xD040) {
            return 1;
        }

        if (binFile.nextStart < binFile.file->Size())
            binFile.file->Seek(binFile.nextStart);
    }

    return 1;
}

void zCModelMeshLib::BuildFromModel(zCModelPrototype* model)
{
    ReleaseData();
    objectName = model->modelProtoName;

    for (int i = 0; i < model->nodeList.numInArray; ++i) {
        zCModelNode* node = model->nodeList.parray[i];
        if (!node->visual)
            continue;
        if (node->nodeName.Length() >= 3 && node->nodeName[0] == 'Z' &&
            node->nodeName[1] == 'S' && node->nodeName[2] == '_')
            continue;
        AddNodeVisual(i, node->visual);
    }

    for (int i = 0; i < model->meshSoftSkinList.numInArray; ++i)
        AddMeshSoftSkin(model->meshSoftSkinList.parray[i]);
}

int zCModelMeshLib::ConvertMDM(const zSTRING&, zCModelPrototype* model)
{
    if (!model)
        return 0;

    model->ReleaseMeshes();
    zCModelMeshLib* meshLib = new zCModelMeshLib();
    meshLib->BuildFromModel(model);
    meshLib->SaveMDM(model);
    meshLib->Release();
    return 1;
}

int zCModelMeshLib::Release()
{
    const int result = --refCtr;
    if (result <= 0) {
        ReleaseData();
        delete this;
    }
    return result;
}

void zCModel::SetRandAnisEnabled(int enabled)
{
    isVisible = (isVisible & ~4) | ((enabled << 2) & 4);
    if (!enabled)
        return;

    for (int i = 0; i < numActiveAnis; ++i) {
        zCModelAniActive* ani = aniChannels[i];
        if (ani->aniAttachment && ani->aniAttachment->randAniList.numInArray > 0)
            ani->randAniTimer = ani->aniAttachment->randAniFreq * 1000.0f;
    }
}

void zCModel::GetAniMinMaxWeight(zCModelAniActive* ani, float& minWeight, float& maxWeight)
{
    minWeight = 2;
    maxWeight = -2;
    for (int i = 0; i < nodeList.numInArray; ++i) {
        const int index = nodeList.parray[i]->GetNodeAniListIndex(ani);
        if (index < 0)
            continue;

        const float weight = nodeList.parray[i]->nodeAniList[index].weight;
        if (weight < minWeight)
            minWeight = weight;
        if (weight > maxWeight)
            maxWeight = weight;
    }
}

zVEC3 zCModel::GetAniTransLerp(zCModelAni* ani, float progress, int) const
{
    const float framePosition = (ani->numFrames - 1) * progress;
    int frame = static_cast<int>(std::floor(framePosition));
    if (frame >= ani->numFrames - 1)
        frame = ani->numFrames - 2;
    const float blend = framePosition - frame;

    zVEC3 first = ani->GetTrans(frame, ani->rootNodeIndex);
    zVEC3 second = ani->GetTrans(frame + 1, ani->rootNodeIndex);
    zVEC3 result;
    for (int i = 0; i < 3; ++i) {
        first.n[i] *= modelScale.n[i];
        second.n[i] *= modelScale.n[i];
        result.n[i] = first.n[i] + (second.n[i] - first.n[i]) * blend;
    }
    return result;
}

int zCModel::IsBBox3DLocal()
{
    return 1;
}

int zCModel::CanTraceRay() const
{
    return 1;
}

unsigned long zCModel::GetRenderSortKey() const
{
    if (!modelProtoList.numInArray)
        return reinterpret_cast<unsigned long>(this);
    return reinterpret_cast<unsigned long>(modelProtoList.parray[0]) | 0xE0000000;
}

zCMaterial* zCModel::GetMaterial(int)
{
    if (!meshSoftSkinList.numInArray)
        return 0;
    const int index = meshSoftSkinList.numInArray > 1 ? 1 : 0;
    return meshSoftSkinList.parray[index]->subMeshList[0].material;
}

int zCModel::GetNumMaterials() const
{
    if (!meshSoftSkinList.numInArray)
        return 0;
    const int index = meshSoftSkinList.numInArray > 1 ? 1 : 0;
    return meshSoftSkinList.parray[index]->numSubMeshes;
}

zCModel::~zCModel()
{
    for (int i = 0; i < modelProtoList.numInArray; ++i)
        modelProtoList.parray[i]->Release();
    delete[] modelProtoList.parray;
    ResetArray(modelProtoList);

    RemoveMeshLibAll();

    for (int i = 0; i < meshSoftSkinList.numInArray; ++i)
        meshSoftSkinList.parray[i]->Release();
    delete[] meshSoftSkinList.parray;
    ResetArray(meshSoftSkinList);

    for (int i = 0; i < nodeList.numInArray; ++i) {
        nodeList.parray[i]->SetNodeVisualS(0, 0);
        delete nodeList.parray[i];
    }
    delete[] nodeList.parray;
    ResetArray(nodeList);

    RemoveAllAniAttachments();
    delete[] aniAttachList.array;
    aniAttachList.array = 0;
    aniAttachList.numAlloc = 0;

    delete[] aniHistoryList;
    aniHistoryList = 0;
    delete[] activeAniList;
    activeAniList = 0;
    delete[] occuredAniEvents.parray;
    ResetArray(occuredAniEvents);
    delete[] meshLibList.parray;
    ResetArray(meshLibList);
    homeVob = 0;
    numActiveAnis = 0;
}

static float GetModelFrameTime()
{
    struct zTIMER {
        int   frameTime;
        float frameTimeFloat;
    };
    extern zTIMER ztimer;
    return ztimer.frameTimeFloat;
}

void zCModelNodeInst::CalcWeights(zCModel* model)
{
    const float delta = model->timeScale * GetModelFrameTime();
    for (int i = 0; i < numNodeAnis; ++i) {
        zTNodeAni& nodeAni = nodeAniList[i];
        if (nodeAni.weightSpeed != 0)
            AddToNodeAniWeight(i, nodeAni.weightSpeed * 0.001f * delta);

        if (nodeAni.blendState == zMDL_BLEND_STATE_FADEOUT && nodeAni.weight <= 0) {
            model->StopAni(nodeAni.modelAni);
            --i;
        }
    }
}

void zCModelNodeInst::CalcBlending(zCModel* model)
{
    CalcWeights(model);
    if (numNodeAnis <= 1)
        return;

    zCQuat blended = {{0, 0, 0, 0}};
    float totalWeight = 0;
    for (int i = 0; i < numNodeAnis; ++i) {
        const float weight = nodeAniList[i].weight;
        for (int j = 0; j < 4; ++j)
            blended.q[j] += nodeAniList[i].quat.q[j] * weight;
        totalWeight += weight;
    }

    if (totalWeight != 0) {
        for (int i = 0; i < 4; ++i)
            blended.q[i] /= totalWeight;
    }
    blended.QuatToMatrix4(trafo);
}

void zCModel::CalcNodeListAniBlending()
{
    for (int i = 0; i < nodeList.numInArray; ++i)
        nodeList.parray[i]->CalcBlending(this);
}

void zCModel::CalcTransBlending()
{
    if (numActiveAnis <= 1)
        return;

    zVEC3 translation = {{0, 0, 0}};
    zCQuat rotation = {{0, 0, 0, 0}};
    float totalWeight = 0;
    for (int i = 0; i < numActiveAnis; ++i) {
        zCModelAniActive* ani = aniChannels[i];
        if (ani->transWeight == 0)
            continue;

        if (ani->protoAni->aniFlags & 2) {
            for (int j = 0; j < 3; ++j)
                translation.n[j] += (ani->thisPos.n[j] - ani->lastPos.n[j]) * ani->transWeight;
        }
        for (int j = 0; j < 4; ++j)
            rotation.q[j] += ani->thisRotQuat.q[j] * ani->transWeight;
        totalWeight += ani->transWeight;
    }

    if (!totalWeight)
        return;
    for (int i = 0; i < 3; ++i)
        vobTrans.n[i] = translation.n[i] / totalWeight;
    for (int i = 0; i < 4; ++i)
        vobRot.q[i] = rotation.q[i] / totalWeight;
}

void zCModelAniActive::DoCombineAni(zCModel* model, int frame, int nextFrame)
{
    zCList<zCModelAni>* entry = protoAni->combAniList.next;
    if (!entry || !entry->next)
        return;

    zCModelAni* first = entry->data;
    zCModelAni* second = entry->next->data;
    const int root = protoAni->rootNodeIndex;
    const zVEC3 firstPos = first->GetTrans(frame, root);
    const zVEC3 secondPos = second->GetTrans(nextFrame, root);
    const zCQuat firstRot = first->GetQuat(frame, root);
    const zCQuat secondRot = second->GetQuat(nextFrame, root);

    for (int i = 0; i < 3; ++i)
        thisPos.n[i] = firstPos.n[i] + (secondPos.n[i] - firstPos.n[i]) * combAniX;
    for (int i = 0; i < 4; ++i)
        thisRotQuat.q[i] = firstRot.q[i] + (secondRot.q[i] - firstRot.q[i]) * combAniX;

    float length = 0;
    for (int i = 0; i < 4; ++i)
        length += thisRotQuat.q[i] * thisRotQuat.q[i];
    length = std::sqrt(length);
    if (length != 0) {
        for (int i = 0; i < 4; ++i)
            thisRotQuat.q[i] /= length;
    }

    for (int i = 0; i < protoAni->numNodes; ++i) {
        zCModelNodeInst* node = model->nodeList.parray[protoAni->nodeIndexList.parray[i]];
        const int index = node->GetNodeAniListIndex(this);
        if (index >= 0)
            node->nodeAniList[index].quat = thisRotQuat;
    }
    transWeight = 1;
}

void zCModel::AttachChildVobToNode(zCVob* vob, zCModelNodeInst* node)
{
    if (!vob || !node)
        return;

    if (attachedVobList.numInArray >= attachedVobList.numAlloc) {
        const int newAlloc = attachedVobList.numAlloc + 16;
        zTMdl_NodeVobAttachment* newArray = new zTMdl_NodeVobAttachment[newAlloc];
        for (int i = 0; i < attachedVobList.numInArray; ++i)
            newArray[i] = attachedVobList.parray[i];
        delete[] attachedVobList.parray;
        attachedVobList.parray = newArray;
        attachedVobList.numAlloc = newAlloc;
    }

    zTMdl_NodeVobAttachment& attachment = attachedVobList.parray[attachedVobList.numInArray++];
    attachment.vob = vob;
    attachment.mnode = node;
    vob->AddRef();
    vob->SetTrafo(GetTrafoNodeToModel(node));
}

void zCModel::RemoveChildVobFromNode(zCVob* vob)
{
    if (!vob)
        return;

    for (int i = 0; i < attachedVobList.numInArray; ++i) {
        if (attachedVobList.parray[i].vob != vob)
            continue;

        attachedVobList.parray[i].vob->Release();
        --attachedVobList.numInArray;
        attachedVobList.parray[i] = attachedVobList.parray[attachedVobList.numInArray];
        return;
    }
}

zCVob* zCModel::GetAttachedNodeVob(zCModelNodeInst* node)
{
    for (int i = 0; i < attachedVobList.numInArray; ++i) {
        if (attachedVobList.parray[i].mnode == node)
            return attachedVobList.parray[i].vob;
    }
    return 0;
}

void zCModel::RemoveAllChildVobsFromNode()
{
    for (int i = 0; i < attachedVobList.numInArray; ++i)
        attachedVobList.parray[i].vob->Release();
    attachedVobList.numInArray = 0;
}

zMAT4 zCModel::GetTrafoNodeToModel(zCModelNodeInst* node)
{
    zMAT4 result = node->trafo;
    if (isVisible & 0x10) {
        for (int i = 0; i < 3; ++i)
            result.m[i][3] *= modelScale.n[i];
    }
    return result;
}

void zCModel::UpdateAttachedVobs()
{
    for (int i = 0; i < attachedVobList.numInArray; ++i) {
        zTMdl_NodeVobAttachment& attachment = attachedVobList.parray[i];
        attachment.vob->SetTrafo(GetTrafoNodeToModel(attachment.mnode));
    }
}

void zCModel::RemoveStartedVobFX(zCVob* vob)
{
    for (int i = 0; i < startedVobFX.numInArray; ++i) {
        if (startedVobFX.parray[i].vob != vob)
            continue;

        if (vob->visual)
            zCParticleFX::StopEmitterOutput(vob->visual);
        vob->Release();
        --startedVobFX.numInArray;
        startedVobFX.parray[i] = startedVobFX.parray[startedVobFX.numInArray];
        return;
    }
}

void zCModel::RemoveAllVobFX()
{
    for (int i = startedVobFX.numInArray - 1; i >= 0; --i)
        RemoveStartedVobFX(startedVobFX.parray[i].vob);
}

void zCModel::psb_WriteAniBlock(zCBuffer& buffer, int index, int) const
{
    zCModelAniActive* ani = aniChannels[index];
    buffer.WriteWord(ani->protoAni->aniID, 16);
    float frame = ani->actFrame + (ani->nextAniOverride ? 2048.0f : 0.0f);
    if (ani->advanceDir < 0)
        frame = -frame;
    buffer.WriteFloat(frame, 32);
    if (ani->nextAniOverride)
        buffer.WriteWord(ani->nextAniOverride->aniID, 16);
    buffer.WriteByte(ani->actAniEvent, 8);
    buffer.WriteByte(ani->isFadingOut, 8);
    if (ani->protoAni->aniFlags & 2) {
        for (int i = 0; i < 3; ++i)
            buffer.WriteFloat(ani->thisPos.n[i], 32);
    }
    if (ani->protoAni->aniFlags & 1) {
        for (int i = 0; i < 4; ++i)
            buffer.WriteFloat(ani->thisRotQuat.q[i], 32);
    }
    if (ani->protoAni->aniType == zMDL_ANI_TYPE_COMB) {
        buffer.WriteFloatPacked0_8(ani->combAniX);
        buffer.WriteFloatPacked0_8(ani->combAniY);
    }
}

void zCModel::psb_ReadAniBlock(zCBuffer& buffer, int index, zCModelAniActive* ani)
{
    const int aniID = buffer.ReadWord(16);
    ani->protoAni = GetAniFromAniID(aniID);
    float frame = buffer.ReadFloat(32);
    ani->advanceDir = frame < 0 ? -1 : 1;
    ani->actFrame = frame < 0 ? -frame : frame;
    ani->nextAniOverride = 0;
    if (ani->actFrame >= 2048) {
        ani->actFrame -= 2048;
        ani->nextAniOverride = GetAniFromAniID(buffer.ReadWord(16));
    }
    ani->nextAni = ani->protoAni->nextAni;
    ani->actAniEvent = buffer.ReadByte(8);
    ani->isFadingOut = buffer.ReadByte(8);
    if (ani->protoAni->aniFlags & 2) {
        for (int i = 0; i < 3; ++i)
            ani->thisPos.n[i] = buffer.ReadFloat(32);
    }
    if (ani->protoAni->aniFlags & 1) {
        for (int i = 0; i < 4; ++i)
            ani->thisRotQuat.q[i] = buffer.ReadFloat(32);
    }
    if (ani->protoAni->aniType == zMDL_ANI_TYPE_COMB) {
        ani->combAniX = buffer.ReadFloatPacked0_8();
        ani->combAniY = buffer.ReadFloatPacked0_8();
    }
    aniChannels[index] = ani;
}

void zCModel::PackStateBinary(zCBuffer& buffer)
{
    buffer.SetMode(WRITE);
    buffer.WriteByte(numActiveAnis, 8);
    for (int i = 0; i < numActiveAnis; ++i)
        psb_WriteAniBlock(buffer, i, 0);
}

void zCModel::UnpackStateBinary(zCBuffer& buffer)
{
    buffer.SetMode(READ);
    numActiveAnis = 0;
    const int count = buffer.ReadByte(8);
    AssertActiveAniListAlloced();

    for (int i = 0; i < count; ++i) {
        zCModelAniActive* ani = &activeAniList[i];
        psb_ReadAniBlock(buffer, i, ani);
        if (!ani->protoAni) {
            numActiveAnis = 0;
            return;
        }
        for (int j = 0; j < ani->protoAni->numNodes; ++j)
            nodeList.parray[ani->protoAni->nodeIndexList.parray[j]]->AddNodeAni(ani);
        ++numActiveAnis;
    }
}

class zCView {
public:

    void Print(int x, int y, const zSTRING& text);
    int  FontY() const;
};

extern zCView* screen;

int zCModel::CorrectAniFreezer()
{
    for (int i = 0; i < numActiveAnis; ++i) {
        zCModelAniActive* first = aniChannels[i];
        if (first->isFadingOut || first->protoAni->aniName[0] == 'T')
            continue;

        for (int j = i + 1; j < numActiveAnis; ++j) {
            zCModelAniActive* second = aniChannels[j];
            if (!second->isFadingOut && second->protoAni->aniName[0] != 'T' &&
                first->protoAni->layer == second->protoAni->layer) {
                StopAni(first);
                return 1;
            }
        }
    }
    return 0;
}

void zCModel::PrintStatus(int x, int y)
{
    screen->Print(x, y, zSTRING("AniName"));
    screen->Print(x + 1800, y, zSTRING("AscName"));
    screen->Print(x + 3300, y, zSTRING("Layer"));
    screen->Print(x + 3900, y, zSTRING("Frame"));

    for (int i = 0; i < numActiveAnis; ++i) {
        zCModelAniActive* ani = aniChannels[i];
        const int line = y + (i + 1) * 250;
        screen->Print(x, line, ani->protoAni->aniName);
        screen->Print(x + 1800, line, ani->protoAni->ascName);
        zSTRING value;
        value.Sprintf("%d", ani->protoAni->layer);
        screen->Print(x + 3300, line, value);
        value.Sprintf("%.2f", ani->actFrame);
        screen->Print(x + 3900, line, value);
    }
}

void zCModel::ShowAniListAdd(zCModelAni* ani)
{
    if (!aniHistoryList)
        aniHistoryList = new zCModelAni*[16]();
    if (aniHistoryList[15] == ani)
        return;

    for (int i = 1; i < 16; ++i)
        aniHistoryList[i - 1] = aniHistoryList[i];
    aniHistoryList[15] = ani;
}

void zCModel::ShowAniList(int x)
{
    if (!aniHistoryList)
        return;

    int y = 8191 - screen->FontY() * 17;
    for (int i = 0; i < 16; ++i) {
        if (aniHistoryList[i]) {
            screen->Print(x, y, aniHistoryList[i]->aniName);
            y += screen->FontY();
        }
    }
}

void zCModel::HostVobRemovedFromWorld(zCVob*, zCWorld* world)
{
    for (int i = 0; i < startedVobFX.numInArray; ++i) {
        world->RemoveVob(startedVobFX.parray[i].vob);
        startedVobFX.parray[i].vob->Release();
    }
    startedVobFX.numInArray = 0;
    RemoveAllChildVobsFromNode();
}

static int s_iNumSoftSkinPoly;

int zCModel::GetLowestLODNumPolys()
{
    return meshSoftSkinList.numInArray * s_iNumSoftSkinPoly;
}

zVEC3* zCModel::GetLowestLODPoly(int index, zVEC3*& vertices)
{
    static zVEC3 lowestPoly[3];
    if (!meshSoftSkinList.numInArray || !zCProgMeshProto::GetNumTri())
        return 0;

    meshSoftSkinList.parray[0]->GetLowestLODPoly(index, vertices);
    vertices = lowestPoly;
    return lowestPoly;
}

void zCModel::SetupLowestLOD(zTRenderContext& context)
{
    zCProgMeshProto::InitSetupLowestLOD();
    for (int i = 0; i < meshSoftSkinList.numInArray; ++i)
        meshSoftSkinList.parray[i]->SetupLowestLOD(context, &s_iNumSoftSkinPoly);
}

void zCModel::RenderNodeList(zTRenderContext& context, zCArray<zMAT4*>& trafos,
    zCRenderLightContainer&, zTPMLightingMode)
{
    for (int i = 0; i < nodeList.numInArray; ++i) {
        zCModelNodeInst* node = nodeList.parray[i];
        if (i < trafos.numInArray)
            trafos.parray[i] = &node->trafoObjToCam;
        if (node->nodeVisual)
            node->nodeVisual->Render(context);
    }
}

int zCModel::Render(zTRenderContext& context)
{
    UpdateMeshLibTexAniState();
    zCArray<zMAT4*> trafos;
    ResetArray(trafos);
    if (nodeList.numInArray) {
        trafos.numAlloc = nodeList.numInArray;
        trafos.numInArray = nodeList.numInArray;
        trafos.parray = new zMAT4*[trafos.numAlloc];
    }

    zCRenderLightContainer lights;
    RenderNodeList(context, trafos, lights, zTPM_LIGHTING_DEFAULT);
    UpdateAttachedVobs();

    if (!m_bDrawHandVisualsOnly) {
        for (int i = 0; i < meshSoftSkinList.numInArray; ++i)
            meshSoftSkinList.parray[i]->RenderSoftSkin(context, fatness);
    }

    delete[] trafos.parray;
    isVisible |= 1;
    return 1;
}

static zCList<zCModelNodeInst>* collisionNodes;

void zCModel::CheckNodeCollisionList(const zCOBBox3D& bbox, zMAT4&)
{
    for (int i = 0; i < nodeList.numInArray; ++i) {
        zCModelNodeInst* node = nodeList.parray[i];
        if (node->nodeVisual && zCOBBox3D::TestIntersectionTree(node->bbox3D))
            collisionNodes->Insert(node);
    }
}

void zCModel::CheckNodeCollision(zCModel* other, zCModelNodeInst* node, zMAT4& trafo,
    zCList<zCModelNodeInst>& results)
{
    if (!node || !node->nodeVisual)
        return;

    collisionNodes = &results;
    GetTrafoNodeToModel(node);
    CheckNodeCollisionList(*reinterpret_cast<const zCOBBox3D*>(&node->bbox3D), trafo);
}

int zCModel::TraceRay(const zVEC3&, const zVEC3&, int, zTTraceRayReport&)
{
    if (meshSoftSkinList.numInArray)
        return 0;

    zCList<zCModelNodeInst> results = {};
    zMAT4 trafo;
    SetIdentity(trafo);
    const zCOBBox3D bbox;
    CheckNodeCollisionList(bbox, trafo);
    return results.next != 0;
}

void zCModelPrototype::ResolveReferences()
{
    insertionsort(protoAnis.array, protoAnis.numInArray, sizeof(zCModelAni*),
        protoAnis.Compare, false);

    for (int i = 0; i < protoAnis.numInArray; ++i) {
        zCModelAni* ani = protoAnis.array[i];
        ani->aniID = i;
        if (ani->nextAniName.Length())
            ani->nextAni = SearchAni(ani->nextAniName);
        else
            ani->nextAni = ani;
        ani->ResolveAlias(this);
        ani->ResolveComb(this);
    }
}

void zCModelPrototype::CalcAniBBox(zCModelAni* ani)
{
    if (!ani)
        return;

    for (int i = 0; i < 3; ++i) {
        ani->aniBBox3DObjSpace.mins.n[i] = 3.40282347e+38f;
        ani->aniBBox3DObjSpace.maxs.n[i] = -3.40282347e+38f;
    }

    for (int frame = 0; frame < ani->numFrames; ++frame) {
        for (int node = 0; node < ani->numNodes; ++node) {
            const zVEC3 position = ani->GetTrans(frame, node);
            for (int i = 0; i < 3; ++i) {
                if (position.n[i] < ani->aniBBox3DObjSpace.mins.n[i])
                    ani->aniBBox3DObjSpace.mins.n[i] = position.n[i];
                if (position.n[i] > ani->aniBBox3DObjSpace.maxs.n[i])
                    ani->aniBBox3DObjSpace.maxs.n[i] = position.n[i];
            }
        }
    }
}

void zCModelPrototype::SkipBlockMDS(int depth)
{
    while (depth && !modelParserFile->file->Eof()) {
        modelParserFile->file->Read(modelParserLine);
        if (modelParserLine.Search("{") >= 0)
            ++depth;
        else if (modelParserLine.Search("}") >= 0)
            --depth;
    }
}

int zCModelPrototype::ReadMeshAndTreeMSB(int&, zCFileBIN& file)
{
    file.file->Read(hierarchySourceASC);
    hierarchySourceASC.Append(".ASC");

    zSTRING mdhName = modelProtoName;
    mdhName.Append(".MDH");
    if (!LoadMDH(mdhName)) {
        zCModelMeshLib* meshLib = new zCModelMeshLib();
        meshLib->BuildFromModel(this);
        meshLib->SaveMDM(this);
        meshLib->Release();
        SaveMDH();
    }

    return 1;
}

void zCModelPrototype::ReadMeshAndTree(int& dontUseMesh, zCFileBIN& file)
{
    dontUseMesh = modelParserLine.Search("DONT_USE_MESH") >= 0;
    ReadMeshAndTreeMSB(dontUseMesh, file);
}

int zCModelPrototype::s_autoConvertMeshes;

// EXE: 0x00593C40
void zCModelPrototype::RegisterMesh(zCFileBIN&)
{
    if (!s_autoConvertMeshes)
        return;

    zSTRING source = hierarchySourceASC;
    source.Append(".ASC");
    modelAniCache.InsertEnd(source);
}

int zCModelPrototype::ReadAniEnumMSB(int, zCFileBIN&)
{
    ResolveReferences();
    return 1;
}

int zCModelPrototype::LoadModelScriptMSB(const zSTRING& fileName)
{
    fileSourceType = zFROM_MDS;
    SetModelProtoName(fileName);

    zFILE_FILE* source = new zFILE_FILE(fileName);
    if (source->Open(false)) {
        delete source;
        return 0;
    }

    zCFileBIN binFile;
    binFile.BinSetFile(source);
    int dontUseMesh = 0;
    while (binFile.nextStart < source->Size()) {
        unsigned short chunkID;
        int chunkSize;
        source->Read(&chunkID, 2);
        if (source->Eof())
            break;
        source->Read(&chunkSize, 4);
        binFile.nextStart = source->Pos() + chunkSize;

        if (chunkID == 0xF200)
            ReadMeshAndTree(dontUseMesh, binFile);
        else if (chunkID == 0xF300)
            RegisterMesh(binFile);
        else if (chunkID == 0xF400)
            ReadAniEnumMSB(0, binFile);
        else if (chunkID == 0xFFFF)
            break;

        if (binFile.nextStart < source->Size())
            source->Seek(binFile.nextStart);
    }

    ResolveReferences();
    binFile.BinClose();
    source->Close();
    delete source;
    return 1;
}

int zCModelPrototype::ReadModelMSB(zCFileBIN& file)
{
    while (file.nextStart < file.file->Size()) {
        unsigned short chunkID;
        int chunkSize;
        file.file->Read(&chunkID, 2);
        if (file.file->Eof())
            break;
        file.file->Read(&chunkSize, 4);
        file.nextStart = file.file->Pos() + chunkSize;

        int result = 1;
        if (chunkID == 0xF2FF)
            return 1;
        if (chunkID == 0xF300)
            result = ReadMeshAndTreeMSB(file.nextStart, file);
        else if (chunkID == 0xF400)
            RegisterMesh(file);
        else if (chunkID == 0xF500)
            result = ReadAniEnumMSB(chunkID, file);

        if (!result)
            return 0;
        if (file.nextStart < file.file->Size())
            file.file->Seek(file.nextStart);
    }
    return 0;
}

void zCModelPrototype::ReadModel(zCFileBIN& file)
{
    modelParserFile = &file;
    while (!file.file->Eof()) {
        file.file->Read(modelParserLine);
        modelParserLine.Upper();
        if (modelParserLine.Search("}") >= 0)
            break;
        if (modelParserLine.Search("MESHANDTREE") >= 0) {
            int dontUseMesh = 0;
            ReadMeshAndTree(dontUseMesh, file);
        }
        else if (modelParserLine.Search("ANIENUM") >= 0) {
            ReadAniEnumMSB(0, file);
        }
        else if (modelParserLine.Search("REGISTERMESH") >= 0) {
            RegisterMesh(file);
        }
    }
}

int zCModelPrototype::LoadModelScript(const zSTRING& fileName)
{
    const int result = LoadModelScriptMSB(fileName);
    if (result)
        ResolveReferences();
    return result;
}

int zCModelPrototype::LoadModelASC(const zSTRING& fileName)
{
    fileSourceType = zFROM_ASC;
    if (LoadMDH(fileName)) {
        zCModelMeshLib* meshLib = 0;
        if (zCModelMeshLib::LoadMDM(fileName, this, 0, &meshLib)) {
            meshLib->Release();
            return 1;
        }
    }

    Clear();
    fileSourceType = zFROM_ASC;
    return LoadModelScript(fileName);
}

void zCModelPrototype::Load3DSMAXAsc(zCModelAni*& ani, const zSTRING&, const zSTRING& modelName,
    int, int, int, float)
{
    LoadModelASC(modelName);
    ani = protoAnis.numInArray ? protoAnis.array[0] : 0;
}

zMAT4 zCModelPrototype::ReadNodeTM(zCModelNode* node)
{
    SetIdentity(node->trafo);
    while (!modelParserFile->file->Eof()) {
        modelParserFile->file->Read(modelParserLine);
        modelParserLine.Upper();
        if (modelParserLine.Search("}") >= 0)
            break;

        for (int row = 0; row < 4; ++row) {
            zSTRING marker;
            marker.Sprintf("*TM_ROW%d", row);
            if (modelParserLine.Search(marker) >= 0) {
                const zVEC3 values = ReadTriple();
                for (int i = 0; i < 3; ++i)
                    node->trafo.m[row][i] = values.n[i];
                break;
            }
        }
    }
    return node->trafo;
}

void zCModelPrototype::ReadVertexList(zCMesh* mesh, int count)
{
    mesh->AllocVerts(count);
    for (int i = 0; i < count; ++i) {
        modelParserFile->file->Read(modelParserLine);
        zVEC3 vertex;
        vertex.n[0] = modelParserLine.PickWord_Old(7, " \t").ToFloat();
        vertex.n[1] = modelParserLine.PickWord_Old(9, " \t").ToFloat();
        vertex.n[2] = modelParserLine.PickWord_Old(11, " \t").ToFloat();
        ConvertVec3(vertex);
        mesh->AddVertex(vertex);
    }
}

zTMatIDList* zCModelPrototype::ReadFaceList(zCMesh* mesh, int count)
{
    mesh->AllocPolys(count);
    for (int i = 0; i < count; ++i) {
        modelParserFile->file->Read(modelParserLine);
        zCPolygon polygon;
        polygon.material = 0;
        polygon.vertexIndex[0] = modelParserLine.PickWord_Old(6, " \t").ToInt();
        polygon.vertexIndex[1] = modelParserLine.PickWord_Old(9, " \t").ToInt();
        polygon.vertexIndex[2] = modelParserLine.PickWord_Old(12, " \t").ToInt();
        mesh->AddPoly(polygon);
    }

    zTMatIDList* materials = new zTMatIDList;
    materials->numMaterials = 0;
    materials->materials = 0;
    return materials;
}

zVEC2* zCModelPrototype::ReadTVertexList(int count)
{
    zVEC2* vertices = new zVEC2[count];
    for (int i = 0; i < count; ++i) {
        modelParserFile->file->Read(modelParserLine);
        vertices[i].n[0] = modelParserLine.PickWord_Old(7, " \t").ToFloat();
        vertices[i].n[1] = 1.0f - modelParserLine.PickWord_Old(9, " \t").ToFloat();
    }
    return vertices;
}

void zCModelPrototype::ReadTFaceList(zCMesh* mesh, zVEC2* vertices, int count)
{
    for (int i = 0; i < count; ++i) {
        modelParserFile->file->Read(modelParserLine);
        const int first = modelParserLine.PickWord_Old(7, " \t").ToInt();
        const int second = modelParserLine.PickWord_Old(9, " \t").ToInt();
        const int third = modelParserLine.PickWord_Old(11, " \t").ToInt();
        zCPolygon* polygon = mesh->polyArray ? &mesh->polyArray[i] : mesh->polyList[i];
        polygon->SetMapping(&vertices[first], &vertices[second], &vertices[third]);
    }
}

void zCPolygon::SetMapping(zVEC2* first, zVEC2* second, zVEC2* third)
{
    texVertex[0] = first;
    texVertex[1] = second;
    texVertex[2] = third;
}

void zCModelPrototype::ReadSoftSkinVertList()
{
    modelParserFile->file->Read(modelParserLine);
    const int count = modelParserLine.PickWord_Old(2, " \t").ToInt();
    zCMeshSoftSkin* skin = new zCMeshSoftSkin();
    ResetArray(skin->weightList);
    ResetArray(skin->nodeIndexList);

    for (int i = 0; i < count; ++i) {
        modelParserFile->file->Read(modelParserLine);
        if (skin->weightList.numInArray >= skin->weightList.numAlloc) {
            const int newAlloc = skin->weightList.numAlloc + 16;
            zTWeightEntry* newArray = new zTWeightEntry[newAlloc];
            for (int j = 0; j < skin->weightList.numInArray; ++j)
                newArray[j] = skin->weightList.parray[j];
            delete[] skin->weightList.parray;
            skin->weightList.parray = newArray;
            skin->weightList.numAlloc = newAlloc;
        }

        zTWeightEntry& entry = skin->weightList.parray[skin->weightList.numInArray++];
        entry.position = ReadTriple();
        entry.normal.n[0] = 0;
        entry.normal.n[1] = 0;
        entry.normal.n[2] = 0;
    }

    if (sourceMeshSoftSkinList.numInArray >= sourceMeshSoftSkinList.numAlloc) {
        const int newAlloc = sourceMeshSoftSkinList.numAlloc + 16;
        zCMesh** newArray = new zCMesh*[newAlloc];
        for (int i = 0; i < sourceMeshSoftSkinList.numInArray; ++i)
            newArray[i] = sourceMeshSoftSkinList.parray[i];
        delete[] sourceMeshSoftSkinList.parray;
        sourceMeshSoftSkinList.parray = newArray;
        sourceMeshSoftSkinList.numAlloc = newAlloc;
    }
    sourceMeshSoftSkinList.parray[sourceMeshSoftSkinList.numInArray++] =
        reinterpret_cast<zCMesh*>(skin);
}

zCMesh* zCModelPrototype::ReadMesh(int, zCModelNode* node, int)
{
    zCMesh* mesh = new zCMesh();
    mesh->meshName = node->nodeName;
    int numVertex = 0;
    int numFaces = 0;
    int numTVertex = 0;
    int numTFaces = 0;
    zVEC2* texVertices = 0;

    while (!modelParserFile->file->Eof()) {
        modelParserFile->file->Read(modelParserLine);
        modelParserLine.Upper();
        if (modelParserLine.Search("}") >= 0)
            break;
        if (modelParserLine.Search("*MESH_NUMVERTEX") >= 0)
            numVertex = modelParserLine.PickWord_Old(3, " \t").ToInt();
        else if (modelParserLine.Search("*MESH_NUMFACES") >= 0)
            numFaces = modelParserLine.PickWord_Old(3, " \t").ToInt();
        else if (modelParserLine.Search("*MESH_NUMTVERTEX") >= 0)
            numTVertex = modelParserLine.PickWord_Old(3, " \t").ToInt();
        else if (modelParserLine.Search("*MESH_NUMTVFACES") >= 0)
            numTFaces = modelParserLine.PickWord_Old(3, " \t").ToInt();
        else if (modelParserLine.Search("*MESH_VERTEX_LIST") >= 0)
            ReadVertexList(mesh, numVertex);
        else if (modelParserLine.Search("*MESH_FACE_LIST") >= 0)
            delete ReadFaceList(mesh, numFaces);
        else if (modelParserLine.Search("*MESH_TVERTLIST") >= 0)
            texVertices = ReadTVertexList(numTVertex);
        else if (modelParserLine.Search("*MESH_TFACELIST") >= 0)
            ReadTFaceList(mesh, texVertices, numTFaces);
    }

    return mesh;
}

void zCModelPrototype::ReadMeshAnimation(zCModelNode* node, int frame)
{
    if (node->visual)
        node->visual->Release();
    node->visual = 0;

    while (!modelParserFile->file->Eof()) {
        modelParserFile->file->Read(modelParserLine);
        modelParserLine.Upper();
        if (modelParserLine.Search("}") >= 0)
            break;
        if (modelParserLine.Search("*MESH") >= 0) {
            if (!node->visual)
                node->visual = ReadMesh(frame, node, 0);
            else
                node->visual->AddEndLODVisual(ReadMesh(frame, node, 0));
        }
    }
}

void zCModelPrototype::ReadPosTrack(zMAT4* track)
{
    while (!modelParserFile->file->Eof()) {
        modelParserFile->file->Read(modelParserLine);
        if (modelParserLine.Search("}") >= 0)
            break;

        const zVEC3 position = ReadTriple();
        track->m[0][3] = position.n[0];
        track->m[1][3] = position.n[1];
        track->m[2][3] = position.n[2];
    }
}

void zCModelPrototype::ReadRotTrack(zMAT4* track)
{
    while (!modelParserFile->file->Eof()) {
        modelParserFile->file->Read(modelParserLine);
        if (modelParserLine.Search("}") >= 0)
            break;

        zVEC3 euler;
        euler.n[0] = modelParserLine.PickWord_Old(5, " \t").ToFloat();
        euler.n[1] = modelParserLine.PickWord_Old(7, " \t").ToFloat();
        euler.n[2] = modelParserLine.PickWord_Old(9, " \t").ToFloat();
        const zCQuat rotation = zCQuat::EulerToQuat(euler);
        rotation.QuatToMatrix4(*track);
    }
}

void zCModelPrototype::ReadTMAnimation(zCModelNode*, zMAT4*& tracks)
{
    tracks = new zMAT4[modelSceneFrameCount];
    for (int i = 0; i < modelSceneFrameCount; ++i)
        SetIdentity(tracks[i]);

    while (!modelParserFile->file->Eof()) {
        modelParserFile->file->Read(modelParserLine);
        modelParserLine.Upper();
        if (modelParserLine.Search("}") >= 0)
            break;
        if (modelParserLine.Search("*CONTROL_POS_TRACK") >= 0)
            ReadPosTrack(tracks);
        else if (modelParserLine.Search("*CONTROL_ROT_TRACK") >= 0)
            ReadRotTrack(tracks);
        else if (modelParserLine.Search("*CONTROL_") >= 0)
            SkipBlock();
    }
}

void zCModelPrototype::LocalizeTrafos(zCTree<zCModelNode>* tree, zCTree<zCModelNode>* parent)
{
    for (zCTree<zCModelNode>* child = tree->firstChild; child; child = child->next)
        LocalizeTrafos(child, tree);

    if (tree->data && tree->data->nodeTrafoList) {
        delete[] tree->data->nodeTrafoList;
        tree->data->nodeTrafoList = 0;
    }
}

void zCModelPrototype::ReadMapDiffuse(zCMaterial* material)
{
    while (!modelParserFile->file->Eof()) {
        modelParserFile->file->Read(modelParserLine);
        modelParserLine.Upper();
        if (modelParserLine.Search("}") >= 0)
            break;
        if (modelParserLine.Search("*BITMAP") >= 0) {
            const zSTRING fileName = modelParserLine.PickWord_Old(2, " \t");
            material->SetTexture(fileName);
        }
        else if (modelParserLine.Search("*MAP_REFLECT") >= 0) {
            material->m_bEnvironmentalMappingStrength =
                modelParserLine.PickWord_Old(2, " \t").ToFloat();
            material->flags |= 0x40;
        }
    }
}

zCMaterial* zCModelPrototype::ReadMaterial()
{
    zCMaterial* material = new zCMaterial();
    material->texture = 0;
    material->m_bEnvironmentalMappingStrength = 0;

    while (!modelParserFile->file->Eof()) {
        modelParserFile->file->Read(modelParserLine);
        modelParserLine.Upper();
        if (modelParserLine.Search("}") >= 0)
            break;
        if (modelParserLine.Search("*MATERIAL_NAME") >= 0)
            material->objectName = modelParserLine.PickWord_Old(2, " \t");
        else if (modelParserLine.Search("*MATERIAL_DIFFUSE") >= 0) {
            for (int i = 0; i < 3; ++i)
                material->color[i] = static_cast<unsigned char>(
                    modelParserLine.PickWord_Old(i + 2, " \t").ToFloat() * 255.0f);
            material->color[3] = 255;
        }
        else if (modelParserLine.Search("*MAP_DIFFUSE") >= 0)
            ReadMapDiffuse(material);
    }

    return material;
}

zCMaterial* zCModelPrototype::ReadWireframeColor()
{
    zCMaterial* material = new zCMaterial();
    material->texture = 0;
    material->color[2] = static_cast<unsigned char>(
        modelParserLine.PickWord_Old(3, " \t").ToFloat() * 255.0f);
    material->color[1] = static_cast<unsigned char>(
        modelParserLine.PickWord_Old(5, " \t").ToFloat() * 255.0f);
    material->color[0] = static_cast<unsigned char>(
        modelParserLine.PickWord_Old(7, " \t").ToFloat() * 255.0f);
    material->color[3] = 255;
    return material;
}

void zCModelPrototype::AssignMeshNodeMaterials(zCMesh* mesh, zTMatIDList* materials, int)
{
    if (!materials || !materials->materials)
        return;

    for (int i = 0; i < mesh->numPoly; ++i) {
        zCPolygon* polygon = mesh->polyArray ? &mesh->polyArray[i] : mesh->polyList[i];
        zCMaterial* material = materials->materials[0];
        if (polygon->material)
            polygon->material->Release();
        polygon->material = material;
        if (material)
            material->AddRef();
    }
}

void zCModelPrototype::TransformNodeVisual(zCModelNode* node, const zMAT4& trafo)
{
    for (zCVisual* visual = node->visual; visual; visual = visual->nextLODVisual) {
        zCMesh* mesh = reinterpret_cast<zCMesh*>(visual);
        mesh->TransformAllVerts(trafo);
        mesh->CalcVertexNormals();
    }
}

void zCModelPrototype::ProcessMeshNode(zCModelNode* node, zCTree<zCModelNode>*, int, int,
    int& processed)
{
    processed = 0;
    if (!node || !node->visual)
        return;

    zCMesh* mesh = reinterpret_cast<zCMesh*>(node->visual);
    mesh->CalcBBox3D();
    mesh->CheckForMaterials();
    TransformNodeVisual(node, node->trafoObjToWorld);
    for (int i = 0; i < 3; ++i) {
        if (mesh->bbox3D.mins.n[i] < bbox3D.mins.n[i])
            bbox3D.mins.n[i] = mesh->bbox3D.mins.n[i];
        if (mesh->bbox3D.maxs.n[i] > bbox3D.maxs.n[i])
            bbox3D.maxs.n[i] = mesh->bbox3D.maxs.n[i];
    }
    processed = 1;
}

static zCClassDef zCProgMeshProtoClassDef("zCProgMeshProto", "zCVisual",
    zCProgMeshProto::_CreateNewInstance, 1, 0xD8, 1, "");
zCClassDef* zCProgMeshProto::classDef = &zCProgMeshProtoClassDef;

// EXE: 0x008D8AB4
static zCArray<zCArray<zCMaterial*>> modelMaterialList;
// EXE: 0x008D8AD8
static zCArray<zCMaterial*> modelMaterialBuffer;

static void DeleteModelMaterials(zCArray<zCMaterial*>& materials)
{
    for (int i = 0; i < materials.numInArray; ++i) {
        if (materials.parray[i])
            materials.parray[i]->Release();
    }
    delete[] materials.parray;
    ResetArray(materials);
}

void zCProgMeshProto::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(reinterpret_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

void zCModelPrototype::ReadGeomobject(zMAT4*& tracks, zCTree<zCModelNode>*& resultTree)
{
    zCModelNode* node = 0;
    zCModelNode* parentNode = 0;
    zCTree<zCModelNode>* parentTree = 0;
    zSTRING parentName;
    bool skipNode = false;
    bool isRoot = false;
    bool isSoftSkin = false;
    int materialRef = -1;
    int processed = 0;
    tracks = 0;
    resultTree = 0;
    ResetArray(modelMaterialBuffer);

    while (!modelParserFile->file->Eof()) {
        modelParserFile->file->Read(modelParserLine);
        modelParserLine.Upper();
        if (modelParserLine.Search("}") >= 0)
            break;

        if (modelParserLine.Search("*NODE_NAME") >= 0) {
            zSTRING nodeName = modelParserLine.PickWord_Old(2, " \t");
            while (nodeName.Length() > 0 && nodeName[0] == ' ')
                nodeName.Delete(0, 1);
            nodeName.Upper();
            parentTree = FindMeshTreeNode(nodeName);
            if (!parentTree) {
                node = new zCModelNode;
                node->nodeName = nodeName;
            }
            else {
                node = parentTree->data;
            }

            isRoot = nodeName.Search("BIP") >= 0;
            if (!isRoot && nodeName.Search("ZS_") < 0 && nodeName.Search("BIP01") < 0)
                skipNode = true;
            if (skipNode) {
                SkipBlock();
                break;
            }
        }
        else if (modelParserLine.Search("*NODE_PARENT") >= 0) {
            parentName = modelParserLine.PickWord_Old(2, " \t");
            parentName.Upper();
            parentTree = FindMeshTreeNode(parentName);
            if (parentTree)
                parentNode = parentTree->data;
        }
        else if (modelParserLine.Search("*NODE_TM") >= 0) {
            if (!node)
                continue;
            node->trafo = ReadNodeTM(node);
            if (!parentNode)
                node->trafoObjToWorld = node->trafo;
            else
                node->trafoObjToWorld = node->trafo * parentNode->trafoObjToWorld.Inverse();
            node->translation = node->trafoObjToWorld.GetTranslation();
        }
        else if (modelParserLine.Search("*MESH_ANIMATION") >= 0) {
            if (node)
                ReadMeshAnimation(node, 0);
            else
                SkipBlock();
        }
        else if (modelParserLine.Search("*MESH_SOFTSKIN") >= 0) {
            if (node) {
                node->visual = ReadMesh(0, node, 1);
                isSoftSkin = true;
            }
            else
                SkipBlock();
        }
        else if (modelParserLine.Search("*MESH") >= 0) {
            if (node)
                node->visual = ReadMesh(0, node, 0);
            else
                SkipBlock();
        }
        else if (modelParserLine.Search("*WIREFRAME_COLOR") >= 0) {
            if (modelMaterialBuffer.numInArray == 0) {
                modelMaterialBuffer.AllocAbs(1);
                modelMaterialBuffer.parray[modelMaterialBuffer.numInArray++] = ReadWireframeColor();
            }
        }
        else if (modelParserLine.Search("*TM_ANIMATION") >= 0) {
            if (node)
                ReadTMAnimation(node, tracks);
            else
                SkipBlock();
        }
        else if (modelParserLine.Search("*MATERIAL_REF") >= 0)
            materialRef = modelParserLine.PickWord_Old(2, " \t").ToInt();
        else if (modelParserLine.Search("*IK_JOINT") >= 0)
            SkipBlock();
        else if (modelParserLine.Search("{") >= 0)
            SkipBlockCmt();
    }

    if (!node || skipNode) {
        delete[] tracks;
        tracks = 0;
        delete node;
        DeleteModelMaterials(modelMaterialBuffer);
        return;
    }

    if (!parentNode) {
        AttachNode(this, node, 0);
        isRoot = true;
    }
    else {
        AttachNode(this, node, parentNode);
    }

    resultTree = FindMeshTreeNode(node->nodeName);
    if (resultTree && resultTree->parent)
        parentNode = resultTree->parent->data;
    node->parentNode = parentNode;

    if (nodeList.numInArray >= nodeList.numAlloc) {
        const int newAlloc = nodeList.numAlloc + 16;
        zCModelNode** newArray = new zCModelNode*[newAlloc];
        for (int i = 0; i < nodeList.numInArray; ++i)
            newArray[i] = nodeList.parray[i];
        delete[] nodeList.parray;
        nodeList.parray = newArray;
        nodeList.numAlloc = newAlloc;
    }
    nodeList.parray[nodeList.numInArray++] = node;

    if (node->visual) {
        if (materialRef >= 0 && materialRef < modelMaterialList.numInArray)
            AssignMeshNodeMaterials(reinterpret_cast<zCMesh*>(node->visual),
                reinterpret_cast<zTMatIDList*>(&modelMaterialList.parray[materialRef]), 0);
        ProcessMeshNode(node, resultTree, 0, isSoftSkin, processed);
    }
    DeleteModelMaterials(modelMaterialBuffer);
}

void zCModelPrototype::ReadMaterialList()
{
    for (int i = 0; i < modelMaterialList.numInArray; ++i)
        DeleteModelMaterials(modelMaterialList.parray[i]);
    delete[] modelMaterialList.parray;
    ResetArray(modelMaterialList);
    ResetArray(modelMaterialBuffer);

    while (!modelParserFile->file->Eof()) {
        modelParserFile->file->Read(modelParserLine);
        modelParserLine.Upper();
        if (modelParserLine.Search("}") >= 0)
            break;

        if (modelParserLine.Search("*MATERIAL_COUNT") >= 0) {
            const int count = modelParserLine.PickWord_Old(2, " \t").ToInt();
            if (modelMaterialList.numAlloc < count)
                modelMaterialList.AllocAbs(count);
        }
        else if (modelParserLine.Search("*MATERIAL") >= 0) {
            DeleteModelMaterials(modelMaterialBuffer);
            zCMaterial* material = ReadMaterial();
            if (modelMaterialBuffer.numInArray >= modelMaterialBuffer.numAlloc)
                modelMaterialBuffer.AllocDelta(modelMaterialBuffer.numAlloc >= 16 ?
                    modelMaterialBuffer.numAlloc / 2 : 16);
            modelMaterialBuffer.parray[modelMaterialBuffer.numInArray++] = material;

            if (modelMaterialList.numInArray >= modelMaterialList.numAlloc)
                modelMaterialList.AllocDelta(modelMaterialList.numAlloc >= 16 ?
                    modelMaterialList.numAlloc / 2 : 16);
            zCArray<zCMaterial*>& materialSet =
                modelMaterialList.parray[modelMaterialList.numInArray++];
            ResetArray(materialSet);
            materialSet.AllocAbs(modelMaterialBuffer.numInArray);
            for (int i = 0; i < modelMaterialBuffer.numInArray; ++i)
                materialSet.parray[i] = modelMaterialBuffer.parray[i];
            ResetArray(modelMaterialBuffer);
        }
    }
}

void zCModelPrototype::ReadAniEnum(int, zCFileBIN& file)
{
    modelParserFile = &file;
    float maxFps = 0;

    while (!file.file->Eof()) {
        file.file->Read(modelParserLine);
        if (modelParserLine.Length() == 0)
            continue;
        modelParserLine.Upper();
        if (modelParserLine.Search("}") >= 0)
            break;

        if (modelParserLine.Search("ANIMAXFPS") >= 0) {
            maxFps = modelParserLine.PickWord_Old(2, " \t").ToFloat();
            continue;
        }
        if (modelParserLine.Search("ANI ") < 0 || s_ignoreAnis)
            continue;

        const zSTRING aniName = modelParserLine.PickWord_Old(2, " \t");
        const int layer = modelParserLine.PickWord_Old(3, " \t").ToInt();
        const zSTRING nextName = modelParserLine.PickWord_Old(4, " \t");
        const float blendIn = modelParserLine.PickWord_Old(5, " \t").ToFloat();
        const float blendOut = modelParserLine.PickWord_Old(6, " \t").ToFloat();
        const zSTRING flags = modelParserLine.PickWord_Old(7, " \t");
        const zSTRING ascName = modelParserLine.PickWord_Old(8, " \t");
        const zSTRING direction = modelParserLine.PickWord_Old(9, " \t");
        const int firstFrame = modelParserLine.PickWord_Old(10, " \t").ToInt();
        const int lastFrame = modelParserLine.PickWord_Old(11, " \t").ToInt();

        float fps = 1.0f;
        float speed = 0;
        float collisionScale = 1.0f;
        const int fpsPos = modelParserLine.Search("FPS:");
        if (fpsPos >= 0)
            fps = modelParserLine.PickWord_Old(12, " \t").ToFloat();
        const int speedPos = modelParserLine.Search("SPD:");
        if (speedPos >= 0)
            speed = modelParserLine.PickWord_Old(13, " \t").ToFloat();
        const int scalePos = modelParserLine.Search("CVS:");
        if (scalePos >= 0) {
            collisionScale = modelParserLine.PickWord_Old(14, " \t").ToFloat();
            if (collisionScale < 0)
                collisionScale = 0;
            else if (collisionScale > 10.0f)
                collisionScale = 10.0f;
        }

        zCModelAni* ani = new zCModelAni;
        ani->aniName = aniName;
        ani->ascName = ascName;
        ani->layer = layer;
        ani->nextAniName = nextName;
        ani->blendInSpeed = blendIn;
        ani->blendOutSpeed = blendOut;
        ani->fpsRate = fps;
        ani->fpsRateSource = maxFps;
        ani->collisionVolumeScale = collisionScale;
        ani->aniDir = direction.Search("R") >= 0 ? zMDL_ANIDIR_REVERSE : zMDL_ANIDIR_FORWARD;
        ani->SetFlags(flags);

        zSTRING manName = ascName;
        manName.Append(".man");
        if (!ani->LoadMAN(manName, this, ascName)) {
            zCModelAni* loadedAni = ani;
            Load3DSMAXAsc(loadedAni, ascName, modelProtoName, firstFrame, lastFrame,
                static_cast<int>(speed), collisionScale);
            ani = loadedAni;
        }
        if (ani) {
            ani->aniName = aniName;
            ani->layer = layer;
            ani->nextAniName = nextName;
            AddAni(ani);
        }
    }

    ResolveReferences();
}

zCModelMeshLib::~zCModelMeshLib()
{
    for (int i = 0; i < meshNodeList.numInArray; ++i) {
        if (meshNodeList.parray[i].visual)
            meshNodeList.parray[i].visual->Release();
    }
    for (int i = 0; i < meshSoftSkinList.numInArray; ++i) {
        if (meshSoftSkinList.parray[i])
            meshSoftSkinList.parray[i]->Release();
    }
    delete[] meshSoftSkinList.parray;
    meshSoftSkinList.parray = 0;
    meshSoftSkinList.numInArray = 0;
    delete[] meshNodeList.parray;
    meshNodeList.parray = 0;
    meshNodeList.numInArray = 0;
}

zCModelConvertFileHandler::zCModelConvertFileHandler()
{
    RegisterFileType("mds");
    RegisterFileType("asc");
}

int zCModelConvertFileHandler::HandleFile(const zSTRING& path, const char* fileName,
    const zFINDDATA&)
{
    zSTRING fullName = path;
    fullName.Append("\\");
    fullName.Append(fileName);
    fullName.Upper();

    if (fullName.Search(".MDS") >= 0) {
        s_autoConvertMeshes = 1;
        zCModelPrototype* model = Load(fullName);
        if (model) {
            model->Release();
            s_autoConvertMeshes = 0;
            return 1;
        }
        s_autoConvertMeshes = 0;
        return 0;
    }

    if (fullName.Search(".ASC") >= 0 &&
        fullName.Search("ANIMS\\ASC_MOBSI\\MESHES") >= 0) {
        zCVisual* visual = zCVisual::LoadVisual(fullName);
        if (visual) {
            visual->Release();
            return 1;
        }
    }
    return 0;
}

static zCClassDef zCModelClassDef("zCModel", "zCVisualAnimate", NULL, 0, 0x204, 0, "");
zCClassDef* zCModel::classDef = &zCModelClassDef;

zCModel::zCModel()
{
    ResetArray(modelProtoList);
    ResetArray(nodeList);
    ResetArray(meshSoftSkinList);
    ResetArray(occuredAniEvents);
    aniAttachList.array = 0;
    aniAttachList.numAlloc = 0;
    aniAttachList.numInArray = 0;
    aniAttachList.Compare = zArraySortDefaultCompare;
    Init();
}

zCObject* zCModel::_CreateNewInstance()
{
    zCModel* model = new zCModel;
    zCClassDef::ObjectCreated(model, classDef);
    return model;
}

void zCModel::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCModel::_GetClassDef() const
{
    return classDef;
}

void zCModel::StartAnimation(const zSTRING& name)
{
    StartAni(name, 0);
}

void zCModel::StopAnimation(const zSTRING& name)
{
    zCModelAni* ani = modelProtoList.parray[0]->SearchAni(name);
    if (!ani || ani->aniID == -1)
        return;

    zCModelAni* protoAni = ani;
    for (int i = modelProtoList.numInArray - 1; i >= 0; --i) {
        zCModelAni* candidate = modelProtoList.parray[i]->protoAnis.array[ani->aniID];
        if (candidate) {
            protoAni = candidate;
            break;
        }
    }

    for (int i = 0; i < numActiveAnis; ++i) {
        if (aniChannels[i]->protoAni == protoAni) {
            StopAni(aniChannels[i]);
            return;
        }
    }
    StopAni(static_cast<zCModelAniActive*>(NULL));
}

int zCModel::IsAnimationActive(const zSTRING& name)
{
    zCModelAni* ani = modelProtoList.parray[0]->SearchAni(name);
    if (!ani || ani->aniID == -1)
        return 0;

    zCModelAni* protoAni = ani;
    for (int i = modelProtoList.numInArray - 1; i >= 0; --i) {
        zCModelAni* candidate = modelProtoList.parray[i]->protoAnis.array[ani->aniID];
        if (candidate) {
            protoAni = candidate;
            break;
        }
    }
    return IsAniActive(protoAni);
}

static zCClassDef zCModelAniClassDef("zCModelAni", "zCObject", NULL, 0, 0xE4, 0, "");
zCClassDef* zCModelAni::classDef = &zCModelAniClassDef;

zCObject* zCModelAni::_CreateNewInstance()
{
    zCModelAni* ani = new zCModelAni;
    zCClassDef::ObjectCreated(reinterpret_cast<zCObject*>(ani), classDef);
    return reinterpret_cast<zCObject*>(ani);
}

void zCModelAni::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCVisual* zCModel::LoadVisualVirtual(const zSTRING& fileName) const
{
    zCModelPrototype* prototype = zCModelPrototype::Load(fileName, NULL);
    if (!prototype)
        return NULL;
    zCModel* model = new zCModel(prototype);
    zCClassDef::ObjectCreated(model, classDef);
    return model;
}

const zSTRING* zCModel::GetAnyAnimation()
{
    if (!modelProtoList.numInArray || !modelProtoList.parray[0]->protoAnis.numInArray)
        return NULL;
    for (int i = modelProtoList.numInArray - 1; i >= 0; --i) {
        if (modelProtoList.parray[i]->protoAnis.numInArray)
            return &modelProtoList.parray[i]->protoAnis.array[0]->aniName;
    }
    return NULL;
}

static zCClassDef zCModelMeshLibClassDef("zCModelMeshLib", "zCObject",
    zCModelMeshLib::_CreateNewInstance, 1, 0x3C, 0, "");
zCClassDef* zCModelMeshLib::classDef = &zCModelMeshLibClassDef;

void zCModelMeshLib::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(reinterpret_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCModelMeshLib::_GetClassDef() const
{
    return classDef;
}

zCObject* zCModelMeshLib::_CreateNewInstance()
{
    zCModelMeshLib* meshLib = new zCModelMeshLib;
    zCClassDef::ObjectCreated(reinterpret_cast<zCObject*>(meshLib), classDef);
    return reinterpret_cast<zCObject*>(meshLib);
}

float zTMdl_AniSample::samplePosScaler = 0;
float zTMdl_AniSample::samplePosRangeMin = 0;

void zTMdl_AniSample::CalcSamplePosConstants(float minimum, float range)
{
    samplePosRangeMin = minimum;
    samplePosScaler = range / 65535.0f;
}

void zTMdl_AniSample::SetSamplePosConstants(float minimum, float scaler)
{
    samplePosRangeMin = minimum;
    samplePosScaler = scaler;
}

void zTMdl_AniSample::Pack(const zCQuat& quat, const zVEC3& positionValue)
{
    for (int axis = 0; axis < 3; ++axis) {
        rotation[axis] = static_cast<int>(quat.q[axis] * 32767.0f) + 32767;
        position[axis] = static_cast<unsigned short>(
            (positionValue.n[axis] - samplePosRangeMin) / samplePosScaler);
    }
    if (quat.q[3] < 0) {
        for (int axis = 0; axis < 3; ++axis)
            rotation[axis] = 65534 - rotation[axis];
    }
}

void zTMdl_AniSample::Unpack(zCQuat& quat, zVEC3& positionValue) const
{
    UnpackQuat(quat);
    UnpackTrans(positionValue);
}

void zTMdl_AniSample::PackTrans(const zVEC3& positionValue)
{
    for (int axis = 0; axis < 3; ++axis) {
        position[axis] = static_cast<unsigned short>(
            (positionValue.n[axis] - samplePosRangeMin) / samplePosScaler);
    }
}

void zTMdl_AniSample::UnpackTrans(zVEC3& positionValue) const
{
    for (int axis = 0; axis < 3; ++axis)
        positionValue.n[axis] = position[axis] * samplePosScaler + samplePosRangeMin;
}

void zTMdl_AniSample::PackQuat(const zCQuat& quat)
{
    for (int axis = 0; axis < 3; ++axis)
        rotation[axis] = static_cast<int>(quat.q[axis] * 32767.0f) + 32767;
    if (quat.q[3] < 0) {
        for (int axis = 0; axis < 3; ++axis)
            rotation[axis] = 65534 - rotation[axis];
    }
}

void zTMdl_AniSample::UnpackQuat(zCQuat& quat) const
{
    float length2 = 0;
    for (int axis = 0; axis < 3; ++axis) {
        quat.q[axis] = (rotation[axis] - 32767) / 32767.0f;
        length2 += quat.q[axis] * quat.q[axis];
    }
    if (length2 <= 1.0f) {
        quat.q[3] = std::sqrt(1.0f - length2);
        return;
    }

    quat.q[3] = 0;
    const float invLength = 1.0f / std::sqrt(length2);
    for (int axis = 0; axis < 3; ++axis)
        quat.q[axis] *= invLength;
}

zCClassDef* zCModelAni::_GetClassDef() const
{
    return classDef;
}

void zCModelAni::AddTrafoMatrix(zMAT4** trafos)
{
    fpsRateSource = fpsRate;
    if (numFrames * numNodes <= 0) {
        aniSampleMatrix = NULL;
        return;
    }

    float minimum = 999999.0f;
    float maximum = -999999.0f;
    for (int frame = 0; frame < numFrames; ++frame) {
        for (int node = 0; node < numNodes; ++node) {
            for (int axis = 0; axis < 3; ++axis) {
                const float value = trafos[node][frame].m[axis][3];
                if (value < minimum)
                    minimum = value;
                if (value > maximum)
                    maximum = value;
            }
        }
    }
    if (minimum == 999999.0f) {
        minimum = 0;
        maximum = 1.0f;
    }
    zTMdl_AniSample::CalcSamplePosConstants(minimum, maximum - minimum);
    samplePosRangeMin = zTMdl_AniSample::samplePosRangeMin;
    samplePosScaler = zTMdl_AniSample::samplePosScaler;

    aniSampleMatrix = new zTMdl_AniSample[numFrames * numNodes];
    for (int frame = 0; frame < numFrames; ++frame) {
        for (int node = 0; node < numNodes; ++node) {
            zCQuat quat;
            quat.Matrix4ToQuat(trafos[node][frame]);
            quat.Unit();
            const zVEC3 position = { trafos[node][frame].m[0][3],
                trafos[node][frame].m[1][3], trafos[node][frame].m[2][3] };
            aniSampleMatrix[frame * numNodes + node].Pack(quat, position);
        }
    }
}

static zCClassDef zCMeshSoftSkinClassDef("zCMeshSoftSkin", "zCProgMeshProto", NULL, 0, 0x104, 0, "");
zCClassDef* zCMeshSoftSkin::classDef = &zCMeshSoftSkinClassDef;

void zCMeshSoftSkin::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

struct zTControlerWeight {
    zTControlerWeight();

    unsigned char data[0x111];
};

zTControlerWeight::zTControlerWeight()
{
    data[0x110] = 0;
}

zTWeightEntry::zTWeightEntry()
{
}
