#include "zMorphMesh.h"

#include <cstdlib>

static int morphMeshState = -1;
static zCClassDef zCMorphMeshClassDef("zCMorphMesh", "zCVisualAnimate", NULL, 0, 0x88, 0, "");
zCClassDef* zCMorphMesh::classDef = &zCMorphMeshClassDef;
static zSTRING morphMeshWhitespace(" \t\r");
static zSTRING morphMeshLineWhitespace(" \t\r");
static zSTRING morphMeshBuffer;
int zCMorphMeshProto::autoConvertBinaryFile;
zCMorphMeshProto* zCMorphMeshProto::morphRoot;

zCProgMeshProto* zDYNAMIC_CAST(const zCObject* object)
{
    if (!object)
        return 0;
    for (zCClassDef* classDef = object->_GetClassDef(); classDef;
        classDef = classDef->baseClassDef) {
        if (classDef == zCProgMeshProto::classDef)
            return reinterpret_cast<zCProgMeshProto*>(const_cast<zCObject*>(object));
    }
    return 0;
}

// EXE: 0x005A2CC0
zCObject* zCMorphMesh::_CreateNewInstance()
{
    zCMorphMesh* mesh = new zCMorphMesh;
    zCClassDef::ObjectCreated(mesh, classDef);
    return mesh;
}

void zCMorphMesh::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(reinterpret_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCMorphMesh::_GetClassDef() const
{
    return classDef;
}

int zCMorphMesh::IsBBox3DLocal()
{
    return 1;
}

zCOBBox3D* zCMorphMesh::GetOBBox3D()
{
    return 0;
}

unsigned long zCMorphMesh::GetRenderSortKey() const
{
    return reinterpret_cast<unsigned long>(morphProto) | 0xD0000000;
}

int zCMorphMesh::GetAlphaTestingEnabled() const
{
    return m_bUsesAlphaTesting;
}

void zCMorphMesh::SetAlphaTestingEnabled(int enabled)
{
    m_bUsesAlphaTesting = enabled;
}

int zCMorphMesh::CanTraceRay() const
{
    return 1;
}

void zCMorphMesh::StartAnimation(const zSTRING& name)
{
    StartAni(SearchAni(name), 1.0f, -2.0f);
}

void zCMorphMesh::StopAnimation(const zSTRING& name)
{
    zCMorphMeshAni* ani = SearchAni(name);
    if (!ani)
        return;

    for (int i = 0; i < aniChannels.numInArray; ++i) {
        if (aniChannels.array[i]->ani != ani)
            continue;
        delete aniChannels.array[i];
        for (int j = i; j < aniChannels.numInArray - 1; ++j)
            aniChannels.array[j] = aniChannels.array[j + 1];
        --aniChannels.numInArray;
        return;
    }
}

int zCMorphMesh::IsAnimationActive(const zSTRING& name)
{
    zCMorphMeshAni* ani = SearchAni(name);
    if (!ani)
        return 0;
    for (int i = 0; i < aniChannels.numInArray; ++i) {
        if (aniChannels.array[i]->ani == ani)
            return 1;
    }
    return 0;
}

zCMorphMeshAni::zCMorphMeshAni()
    : blendInSpeed(0), blendOutSpeed(0), holdTime(0), layer(0), fpsRate(0),
      numVert(0), vertIndexList(0), numFrames(0), vertPosMatrix(0)
{
    flags.discontinuity = 0;
    flags.looping = 0;
    flags.shape = 0;
    flags.refShape = 0;
}

zCMorphMeshAni::~zCMorphMeshAni()
{
    delete[] vertIndexList;
    vertIndexList = 0;
    delete[] vertPosMatrix;
    vertPosMatrix = 0;
}

void zCMorphMeshAni::SetBlending(float blendIn, float blendOut)
{
    blendInSpeed = blendIn > 0 ? 1.0f / blendIn : 3.40282347e+38f;
    blendOutSpeed = blendOut > 0 ? -1.0f / blendOut : -3.40282347e+38f;
    blendInSpeed *= 0.001f;
    blendOutSpeed *= 0.001f;
}

void zCMorphMeshAni::Load(zCFileBIN& file)
{
    file.file->Read(aniName);
    file.file->Read(&blendInSpeed, 4);
    file.file->Read(&blendOutSpeed, 4);
    file.file->Read(&holdTime, 4);
    file.file->Read(&layer, 4);
    file.file->Read(&fpsRate, 4);
    file.file->Read(&flags, 1);
    file.file->Read(&numVert, 4);
    file.file->Read(&numFrames, 4);
    delete[] vertIndexList;
    delete[] vertPosMatrix;
    vertIndexList = new int[numVert];
    vertPosMatrix = new zVEC3[numFrames * numVert];
    file.file->Read(vertIndexList, numVert * 4);
    file.file->Read(vertPosMatrix, numFrames * numVert * 12);
}

void zCMorphMeshAni::Save(zCFileBIN& file)
{
    file.BinWriteString(aniName);
    file.file->Write(&blendInSpeed, 4);
    file.file->Write(&blendOutSpeed, 4);
    file.file->Write(&holdTime, 4);
    file.file->Write(&layer, 4);
    file.file->Write(&fpsRate, 4);
    file.file->Write(&flags, 1);
    file.file->Write(&numVert, 4);
    file.file->Write(&numFrames, 4);
    file.file->Write(vertIndexList, numVert * 4);
    file.file->Write(vertPosMatrix, numFrames * numVert * 12);
}

zCMorphMeshProto::zCMorphMeshProto()
    : prev(0), refCtr(1), morphRefMesh(0), morphRefMeshVertPos(0),
      m_bAlphaTestingEnabled(0)
{
    next = morphRoot;
    if (morphRoot)
        morphRoot->prev = this;
    morphRoot = this;
    aniList.Compare = zArraySortDefaultCompare;
    morphProtoName.Sprintf("%u", reinterpret_cast<unsigned int>(this));
}

zCMorphMeshProto::~zCMorphMeshProto()
{
    --refCtr;
    for (int i = 0; i < aniList.numInArray; ++i)
        delete aniList.array[i];
    aniList.numInArray = 0;
    delete morphRefMesh;
    morphRefMesh = 0;
    delete[] morphRefMeshVertPos;
    morphRefMeshVertPos = 0;

    if (this == morphRoot)
        morphRoot = next;
    else if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;
    next = 0;
    prev = 0;
}

int zCMorphMeshProto::Release()
{
    const int result = --refCtr;
    if (result <= 0)
        delete this;
    return result;
}

zCMorphMeshProto* zCMorphMeshProto::SearchName(const zSTRING& name)
{
    for (zCMorphMeshProto* proto = morphRoot; proto; proto = proto->next) {
        if (proto->morphProtoName == name)
            return proto;
    }
    return 0;
}

zCMorphMeshProto* zCMorphMeshProto::Load(const zSTRING& fileName)
{
    zSTRING shortName = fileName;
    shortName.Upper();
    zCMorphMeshProto* proto = SearchName(shortName);
    if (proto) {
        ++proto->refCtr;
        return proto;
    }

    proto = new zCMorphMeshProto;
    proto->morphProtoName = shortName;
    zSTRING binaryName = fileName;
    binaryName.Append(".mmb");
    if (!proto->LoadMMB(binaryName)) {
        zSTRING scriptName = fileName;
        scriptName.Append(".mms");
        proto->LoadMorphScript(scriptName);
        proto->SaveMMB(binaryName);
    }
    return proto;
}

zCMorphMeshAni* zCMorphMeshProto::SearchAni(const zSTRING& name)
{
    for (int i = 0; i < aniList.numInArray; ++i) {
        if (aniList.array[i]->aniName == name)
            return aniList.array[i];
    }
    return 0;
}

int zCMorphMeshProto::LoadMMB(const zSTRING& fileName)
{
    zFILE_FILE* source = new zFILE_FILE(fileName);
    if (source->Open(false)) {
        delete source;
        return 0;
    }

    zCFileBIN file;
    file.BinSetFile(source);
    int result = 1;
    while (file.nextStart < source->Size()) {
        unsigned short chunkID = 0;
        int chunkSize = 0;
        source->Read(&chunkID, 2);
        if (source->Eof())
            break;
        source->Read(&chunkSize, 4);
        file.nextStart = source->Pos() + chunkSize;

        if (chunkID == 0xE020) {
            int version = 0;
            source->Read(&version, 4);
            if (version != 3) {
                result = 0;
                break;
            }
            source->Read(morphProtoName);
            delete morphRefMesh;
            morphRefMesh = new zCProgMeshProto;
            morphRefMesh->numSubMeshes = 0;
            morphRefMesh->subMeshList = 0;
            int numVerts = 0;
            source->Read(&numVerts, 4);
            morphRefMesh->posList.AllocAbs(numVerts);
            morphRefMesh->posList.numInArray = numVerts;
            source->Read(morphRefMesh->posList.parray, numVerts * 12);
            delete[] morphRefMeshVertPos;
            morphRefMeshVertPos = new zVEC3[numVerts];
            source->Read(morphRefMeshVertPos, numVerts * 12);
        }
        else if (chunkID == 0xE030) {
            unsigned short numAnis = 0;
            source->Read(&numAnis, 2);
            aniList.numInArray = 0;
            aniList.AllocDelta(numAnis - aniList.numAlloc);
            for (int i = 0; i < numAnis; ++i) {
                zCMorphMeshAni* ani = new zCMorphMeshAni;
                ani->Load(file);
                aniList.InsertEnd(ani);
            }
        }
        if (file.nextStart < source->Size())
            source->Seek(file.nextStart);
    }

    file.BinClose();
    source->Close();
    delete source;
    return result;
}

void zCMorphMeshProto::SaveMMB(const zSTRING& fileName)
{
    zFILE_FILE* destination = new zFILE_FILE(fileName);
    if (destination->Create()) {
        delete destination;
        return;
    }

    zCFileBIN file;
    file.BinSetFile(destination);
    file.BinStartChunk(0xE020);
    int version = 3;
    destination->Write(&version, 4);
    destination->Write(morphProtoName);
    int numVerts = morphRefMesh ? morphRefMesh->posList.numInArray : 0;
    destination->Write(&numVerts, 4);
    if (morphRefMesh)
        destination->Write(morphRefMesh->posList.parray, numVerts * 12);
    destination->Write(morphRefMeshVertPos, numVerts * 12);
    file.BinEndChunk();

    file.BinStartChunk(0xE030);
    unsigned short numAnis = aniList.numInArray;
    destination->Write(&numAnis, 2);
    for (int i = 0; i < aniList.numInArray; ++i)
        aniList.array[i]->Save(file);
    file.BinEndChunk();
    file.BinClose();
    destination->Close();
    delete destination;
}

zCMorphMeshAni* zCMorphMeshProto::GenerateMorphAni(zCMesh* mesh,
    const zCArray<int>& vertIndexList, int refShape)
{
    if (!mesh || !morphRefMesh)
        return 0;

    zCMorphMeshAni* ani = new zCMorphMeshAni;
    zCArray<int> changed;
    const int referenceVerts = morphRefMesh->posList.numInArray;
    for (int i = 0; i < mesh->numVerts && i < vertIndexList.numInArray && i < referenceVerts; ++i) {
        const int vertexIndex = vertIndexList.parray[i];
        if (vertexIndex < 0 || vertexIndex >= mesh->numVerts)
            continue;
        const zVEC3& position = mesh->vertexList[vertexIndex];
        const zVEC3& reference = morphRefMesh->posList.parray[i];
        const float dx = position.n[0] - reference.n[0];
        const float dy = position.n[1] - reference.n[1];
        const float dz = position.n[2] - reference.n[2];
        if (refShape || dx * dx + dy * dy + dz * dz > 0.000001f)
            changed.InsertEnd(i);
    }

    if (changed.numInArray == 0 && mesh->nextLODVisual == 0 && !refShape) {
        for (int i = 0; i < mesh->numVerts; ++i)
            changed.InsertEnd(i);
    }
    if (changed.numInArray == 0) {
        delete ani;
        return 0;
    }

    ani->numVert = changed.numInArray;
    ani->vertIndexList = new int[ani->numVert];
    for (int i = 0; i < ani->numVert; ++i)
        ani->vertIndexList[i] = changed.parray[i];
    for (zCVisual* lod = mesh; lod; lod = lod->nextLODVisual)
        ++ani->numFrames;
    ani->vertPosMatrix = new zVEC3[ani->numVert * ani->numFrames];

    int frame = 0;
    for (zCVisual* lod = mesh; lod; lod = lod->nextLODVisual, ++frame) {
        zCMesh* frameMesh = reinterpret_cast<zCMesh*>(lod);
        for (int i = 0; i < ani->numVert; ++i) {
            const int sourceIndex = ani->vertIndexList[i];
            const int meshIndex = vertIndexList.parray[sourceIndex];
            zVEC3& destination = ani->vertPosMatrix[frame * ani->numVert + i];
            destination = frameMesh->vertexList[meshIndex];
            if (!refShape) {
                for (int axis = 0; axis < 3; ++axis)
                    destination.n[axis] -= morphRefMesh->posList.parray[sourceIndex].n[axis];
            }
        }
    }
    return ani;
}

zCMesh* zCMorphMeshProto::Load3DSMAXAsc(const zSTRING& fileName, float& fps,
    int firstFrame, int lastFrame)
{
    zCModelPrototype model;
    model.SetModelProtoName(fileName);
    zCModelAni* ani = 0;
    model.Load3DSMAXAsc(ani, fileName, fileName, 0, firstFrame, lastFrame, -1.0f);
    zCMesh* result = 0;
    for (int i = 0; i < model.nodeList.numInArray; ++i) {
        if (model.nodeList.parray[i]->visual) {
            result = reinterpret_cast<zCMesh*>(model.nodeList.parray[i]->visual);
            result->AddRef();
            break;
        }
    }
    fps = ani ? ani->fpsRate : 1.0f;
    if (ani)
        ani->Release();
    return result;
}

void zCMorphMeshAni::SetFlagsByString(const zSTRING& flagString)
{
    flags.discontinuity = flagString.Search("D") >= 0;
    flags.looping = flagString.Search("L") >= 0;
    flags.shape = flagString.Search("S") >= 0;
    if (flags.shape)
        layer = 999;
    flags.refShape = flagString.Search("R") >= 0;
}

static zSTRING morphParserLine;

void zCMorphMeshProto::ReadMorphRef(zCArray<int>& vertIndexList)
{
    const zSTRING fileName = morphParserLine.PickWord_Old(2, " \t");
    float fps = 0;
    zCMesh* mesh = Load3DSMAXAsc(fileName, fps, 0, 0);
    if (!mesh)
        return;

    delete morphRefMesh;
    morphRefMesh = new zCProgMeshProto;
    morphRefMesh->numSubMeshes = 0;
    morphRefMesh->subMeshList = 0;
    morphRefMesh->posList.AllocAbs(mesh->numVerts);
    morphRefMesh->posList.numInArray = mesh->numVerts;
    delete[] morphRefMeshVertPos;
    morphRefMeshVertPos = new zVEC3[mesh->numVerts];
    vertIndexList.DeleteList();
    for (int i = 0; i < mesh->numVerts; ++i) {
        morphRefMesh->posList.parray[i] = mesh->vertexList[i];
        morphRefMeshVertPos[i] = mesh->vertexList[i];
        vertIndexList.InsertEnd(i);
    }
    mesh->Release();
}

void zCMorphMeshProto::ReadMorphAni(const zCArray<int>& vertIndexList)
{
    const zSTRING aniName = morphParserLine.PickWord_Old(2, " \t");
    const int layer = morphParserLine.PickWord_Old(3, " \t").ToInt();
    const float blendIn = morphParserLine.PickWord_Old(4, " \t").ToFloat();
    const float holdTime = morphParserLine.PickWord_Old(5, " \t").ToFloat();
    const float blendOut = morphParserLine.PickWord_Old(6, " \t").ToFloat();
    const zSTRING flagString = morphParserLine.PickWord_Old(7, " \t");
    const zSTRING ascName = morphParserLine.PickWord_Old(8, " \t");
    const int firstFrame = morphParserLine.PickWord_Old(9, " \t").ToInt();
    const int lastFrame = morphParserLine.PickWord_Old(10, " \t").ToInt();
    const int refShape = flagString.Search("R") >= 0;

    float fps = 0;
    zCMesh* mesh = Load3DSMAXAsc(ascName, fps, firstFrame, lastFrame);
    zCMorphMeshAni* ani = GenerateMorphAni(mesh, vertIndexList, refShape);
    if (ani) {
        ani->aniName = aniName;
        ani->ascName = ascName;
        ani->layer = layer;
        ani->holdTime = holdTime * 1000.0f;
        ani->fpsRate = fps * 0.001f;
        ani->SetBlending(blendIn, blendOut);
        ani->SetFlagsByString(flagString);
        if (ani->holdTime > 0)
            ani->flags.looping = 0;
        aniList.InsertEnd(ani);
    }
    while (mesh) {
        zCMesh* next = reinterpret_cast<zCMesh*>(mesh->nextLODVisual);
        mesh->Release();
        mesh = next;
    }
}

void zCMorphMeshProto::LoadMorphScript(const zSTRING& fileName)
{
    zFILE_FILE* source = new zFILE_FILE(fileName);
    if (source->Open(false)) {
        delete source;
        return;
    }

    morphProtoName = fileName;
    morphProtoName.Upper();
    zCArray<int> vertIndexList;
    while (!source->Eof()) {
        source->Read(morphParserLine);
        morphParserLine.Upper();
        if (morphParserLine.Search("//") >= 0)
            continue;
        if (morphParserLine.Search("MORPHREF") >= 0)
            ReadMorphRef(vertIndexList);
        else if (morphParserLine.Search("MORPHANI") >= 0)
            ReadMorphAni(vertIndexList);
    }
    vertIndexList.DeleteList();
    source->Close();
    delete source;
}

zCMorphMesh::zCMorphMesh()
{
    Init();
}

void zCMorphMesh::Init()
{
    morphProto = 0;
    morphMesh = 0;
    refShapeAni = 0;
    SetAlphaTestingEnabled(0);
    aniChannels.Compare = zArraySortDefaultCompare;
}

zCMorphMesh::zCMorphMesh(zCMorphMeshProto* proto)
{
    Init();
    morphProto = proto;
    ++morphProto->refCtr;
    morphMesh = morphProto->morphRefMesh;
    if (morphMesh)
        ++morphMesh->refCtr;
}

zCMorphMesh::~zCMorphMesh()
{
    if (morphProto) {
        morphProto->Release();
        morphProto = 0;
    }
    if (morphMesh) {
        --morphMesh->refCtr;
        if (morphMesh->refCtr <= 0)
            delete morphMesh;
        morphMesh = 0;
    }
    for (int i = 0; i < aniChannels.numInArray; ++i)
        delete aniChannels.array[i];
    aniChannels.numInArray = 0;
}

zCMorphMesh* zCMorphMesh::Load(const zSTRING& fileName)
{
    zCMorphMeshProto* proto = zCMorphMeshProto::Load(fileName);
    if (!proto)
        return 0;
    zCMorphMesh* mesh = new zCMorphMesh(proto);
    mesh->SetAlphaTestingEnabled(proto->m_bAlphaTestingEnabled);
    proto->Release();
    return mesh;
}

const zSTRING* zCMorphMesh::GetFileExtension(int index)
{
    if (index)
        return 0;
    static const zSTRING extension(".MMS");
    return &extension;
}

zCVisual* zCMorphMesh::LoadVisualVirtual(const zSTRING& fileName) const
{
    return Load(fileName);
}

zTBBox3D zCMorphMesh::GetBBox3D()
{
    zTBBox3D result;
    for (int axis = 0; axis < 3; ++axis) {
        result.mins.n[axis] = 0;
        result.maxs.n[axis] = 0;
    }
    if (!morphMesh || morphMesh->posList.numInArray <= 0)
        return result;

    result.mins = morphMesh->posList.parray[0];
    result.maxs = morphMesh->posList.parray[0];
    for (int i = 1; i < morphMesh->posList.numInArray; ++i) {
        for (int axis = 0; axis < 3; ++axis) {
            if (morphMesh->posList.parray[i].n[axis] < result.mins.n[axis])
                result.mins.n[axis] = morphMesh->posList.parray[i].n[axis];
            if (morphMesh->posList.parray[i].n[axis] > result.maxs.n[axis])
                result.maxs.n[axis] = morphMesh->posList.parray[i].n[axis];
        }
    }
    return result;
}

zSTRING zCMorphMesh::GetVisualName()
{
    zSTRING result = morphProto->morphProtoName;
    result.Append(".MMS");
    return result;
}

zCMorphMeshAni* zCMorphMesh::SearchAni(const zSTRING& name)
{
    return morphProto->SearchAni(name);
}

struct zTIMER {
    float frameTimeFloat;
};

extern zTIMER ztimer;

int zCMorphMesh::Render(zTRenderContext&)
{
    if (!morphMesh)
        return 0;
    AdvanceAnis();
    CalcVertPositions();
    texAniState.UpdateTexList();
    return 1;
}

int zCMorphMesh::Render(zTRenderContext& context, zCRenderLightContainer*, float, float)
{
    return Render(context);
}

void zCMorphMesh::CalcVertPositions()
{
    if (!morphMesh)
        return;
    for (int i = 0; i < morphMesh->posList.numInArray; ++i) {
        morphMesh->posList.parray[i].n[0] = 0;
        morphMesh->posList.parray[i].n[1] = 0;
        morphMesh->posList.parray[i].n[2] = 0;
    }

    for (int channel = 0; channel < aniChannels.numInArray; ++channel) {
        zTMorphAniEntry* entry = aniChannels.array[channel];
        zCMorphMeshAni* ani = entry->ani;
        float weight = entry->weight * entry->weight * (3.0f - 2.0f * entry->weight);
        const float baseWeight = ani->flags.shape ? 1.0f : 1.0f - weight;
        for (int i = 0; i < ani->numVert; ++i) {
            const int vertexIndex = ani->vertIndexList[i];
            if (vertexIndex < 0 || vertexIndex >= morphMesh->posList.numInArray)
                continue;
            const zVEC3& first = ani->vertPosMatrix[entry->actFrameInt * ani->numVert + i];
            zVEC3 position = first;
            if (ani->numFrames > 1) {
                const zVEC3& next = ani->vertPosMatrix[entry->nextFrameInt * ani->numVert + i];
                for (int axis = 0; axis < 3; ++axis)
                    position.n[axis] += (next.n[axis] - first.n[axis]) * entry->frac;
            }
            for (int axis = 0; axis < 3; ++axis) {
                morphMesh->posList.parray[vertexIndex].n[axis] =
                    morphMesh->posList.parray[vertexIndex].n[axis] * baseWeight +
                    position.n[axis] * weight;
            }
        }
    }

    if (refShapeAni) {
        for (int i = 0; i < refShapeAni->numVert && i < morphMesh->posList.numInArray; ++i) {
            for (int axis = 0; axis < 3; ++axis)
                morphMesh->posList.parray[i].n[axis] += refShapeAni->vertPosMatrix[i].n[axis];
        }
    }
    else {
        for (int i = 0; i < morphMesh->posList.numInArray; ++i) {
            for (int axis = 0; axis < 3; ++axis)
                morphMesh->posList.parray[i].n[axis] +=
                    morphProto->morphRefMeshVertPos[i].n[axis];
        }
    }
}

void zCMorphMesh::AdvanceAnis()
{
    for (int i = 0; i < aniChannels.numInArray; ++i) {
        zTMorphAniEntry* entry = aniChannels.array[i];
        zCMorphMeshAni* ani = entry->ani;
        bool remove = false;
        if (entry->blendState == 0) {
            entry->weight += ztimer.frameTimeFloat * entry->blendInSpeed;
            if (entry->weight >= entry->weightDest) {
                entry->weight = entry->weightDest;
                entry->blendState = 1;
            }
        }
        else if (entry->blendState == 1) {
            if (!ani->flags.looping) {
                entry->holdTime -= ztimer.frameTimeFloat;
                if (entry->holdTime <= 0)
                    entry->blendState = 2;
            }
        }
        else {
            entry->weight += ztimer.frameTimeFloat * entry->blendOutSpeed;
            if (entry->weight <= 0)
                remove = true;
        }

        if (remove) {
            delete entry;
            for (int j = i; j < aniChannels.numInArray - 1; ++j)
                aniChannels.array[j] = aniChannels.array[j + 1];
            --aniChannels.numInArray;
            --i;
            continue;
        }

        if (ani->numFrames > 1) {
            entry->actFrame += ztimer.frameTimeFloat * ani->fpsRate;
            while (entry->actFrame >= ani->numFrames)
                entry->actFrame -= ani->numFrames;
            entry->actFrameInt = static_cast<int>(entry->actFrame);
            entry->frac = entry->actFrame - entry->actFrameInt;
            if (ani->flags.discontinuity) {
                if (entry->actFrameInt != entry->nextFrameInt) {
                    int next = entry->nextFrameInt;
                    while (next == entry->nextFrameInt)
                        next = std::rand() % ani->numFrames;
                    entry->actFrameInt = next;
                }
                entry->nextFrameInt = entry->actFrameInt;
            }
            else {
                entry->nextFrameInt = entry->actFrameInt + 1;
                if (entry->nextFrameInt >= ani->numFrames)
                    entry->nextFrameInt = 0;
            }
        }
    }
    ProcessRandAnis();
}

int zCMorphMesh::IsAniActive(const zSTRING& name)
{
    return IsAniActive(SearchAni(name));
}

int zCMorphMesh::IsAniActive(zCMorphMeshAni* ani)
{
    if (!ani)
        return 0;
    for (int i = 0; i < aniChannels.numInArray; ++i) {
        if (aniChannels.array[i]->ani == ani)
            return 1;
    }
    return 0;
}

void zCMorphMesh::StopAni(const zSTRING& name)
{
    StopAni(SearchAni(name));
}

void zCMorphMesh::StopAni(zCMorphMeshAni* ani)
{
    if (!ani)
        return;
    for (int i = 0; i < aniChannels.numInArray; ++i) {
        if (aniChannels.array[i]->ani != ani)
            continue;
        delete aniChannels.array[i];
        for (int j = i; j < aniChannels.numInArray - 1; ++j)
            aniChannels.array[j] = aniChannels.array[j + 1];
        --aniChannels.numInArray;
        return;
    }
}

void zCMorphMesh::FadeOutAni(const zSTRING& name)
{
    FadeOutAni(SearchAni(name));
}

void zCMorphMesh::FadeOutAni(zCMorphMeshAni* ani)
{
    if (!ani)
        return;
    for (int i = 0; i < aniChannels.numInArray; ++i) {
        if (aniChannels.array[i]->ani == ani) {
            aniChannels.array[i]->blendState = 2;
            return;
        }
    }
}

const zSTRING* zCMorphMesh::GetAnyAnimation()
{
    if (!morphProto || morphProto->aniList.numInArray <= 0)
        return 0;
    return &morphProto->aniList.array[0]->aniName;
}

void zCMorphMesh::StartAni(const zSTRING& name, float blendIn, float blendOut)
{
    StartAni(SearchAni(name), blendIn, blendOut);
}

void zCMorphMesh::StartAni(zCMorphMeshAni* ani, float blendIn, float blendOut)
{
    if (!ani)
        return;
    if (ani->flags.refShape) {
        refShapeAni = ani;
        return;
    }

    int insertIndex = aniChannels.numInArray;
    bool found = false;
    for (int i = 0; i < aniChannels.numInArray; ++i) {
        zTMorphAniEntry* entry = aniChannels.array[i];
        if (entry->ani == ani) {
            if (entry->blendState == 2)
                entry->blendState = 0;
            found = true;
        }
        else if (entry->ani->layer == ani->layer)
            entry->blendState = 2;
        else if (entry->ani->layer > ani->layer && insertIndex == aniChannels.numInArray)
            insertIndex = i;
    }
    if (found)
        return;

    zTMorphAniEntry* entry = new zTMorphAniEntry;
    entry->ani = ani;
    entry->weight = 0;
    entry->weightDest = blendIn;
    entry->actFrame = 0;
    entry->actFrameInt = 0;
    entry->nextFrameInt = ani->numFrames > 1 ? 1 : 0;
    entry->frac = 0;
    entry->blendInSpeed = ani->blendInSpeed;
    entry->blendOutSpeed = ani->blendOutSpeed;
    entry->holdTime = blendOut == -2.0f ? ani->holdTime : blendOut;
    entry->blendState = 0;
    aniChannels.InsertEnd(entry);
    for (int i = aniChannels.numInArray - 1; i > insertIndex; --i)
        aniChannels.array[i] = aniChannels.array[i - 1];
    aniChannels.array[insertIndex] = entry;
}

void zCMorphMesh::RemoveRandAni(zCMorphMeshAni* ani)
{
    if (!ani)
        return;
    for (int i = 0; i < randAniList.numInArray; ++i) {
        if (randAniList.parray[i].randAni != ani)
            continue;
        --randAniList.numInArray;
        if (i != randAniList.numInArray)
            randAniList.parray[i] = randAniList.parray[randAniList.numInArray];
        return;
    }
}

void zCMorphMesh::AddRandAni(zCMorphMeshAni* ani, float timeMin, float timeMinVar,
    float timeMax, float timeMaxVar, float probMin)
{
    if (!ani)
        return;
    zTMorphRandAni entry;
    entry.randAniTimer = timeMax * 2000.0f;
    entry.randAni = ani;
    entry.randAniTimeMin = timeMin;
    entry.randAniTimeMinVar = timeMinVar;
    entry.randAniTimeMax = timeMax;
    entry.randAniTimeMaxVar = timeMaxVar;
    entry.randAniProbMin = probMin;
    randAniList.InsertEnd(entry);
}

void zCMorphMesh::ProcessRandAnis()
{
    for (int i = 0; i < randAniList.numInArray; ++i) {
        zTMorphRandAni& entry = randAniList.parray[i];
        entry.randAniTimer -= ztimer.frameTimeFloat;
        if (entry.randAniTimer > 0)
            continue;
        const float blendIn = std::rand() * 1.22074043e-05f + 0.6f;
        StartAni(entry.randAni, blendIn, -2.0f);
        const float randomValue = std::rand() * 3.05185094e-05f;
        const float randomSign = (std::rand() - 16383.5f) * 6.10370189e-05f;
        if (randomValue < entry.randAniProbMin)
            entry.randAniTimer = (entry.randAniTimeMin +
                randomSign * entry.randAniTimeMinVar) * 1000.0f;
        else
            entry.randAniTimer = (entry.randAniTimeMax +
                randomSign * entry.randAniTimeMaxVar) * 1000.0f;
    }
}

void zCMorphMesh::SetTextureVar(int channel, int frame, zSTRING* texture)
{
    texAniState.SetChannelVariation(channel, frame, texture);
}

int zCMorphMesh::TraceRay(const zVEC3&, const zVEC3&, int, struct zTTraceRayReport&)
{
    return morphMesh != 0;
}

void zCMorphMesh::PrintStatus(int x, int y)
{
    screen->Print(x, y, "AniName");
    screen->Print(x + 1800, y, "weight/dest");
    screen->Print(x + 3300, y, "Layer");
    screen->Print(x + 3900, y, "Frame");
    screen->Print(x + 4500, y, "FPS");
    zSTRING count;
    count.Sprintf("(#anis: %d)", morphProto->aniList.numInArray);
    screen->Print(x + 5100, y, count);

    for (int i = 0; i < aniChannels.numInArray; ++i) {
        zTMorphAniEntry* entry = aniChannels.array[i];
        const int lineY = y + (i + 1) * 250;
        zSTRING weights;
        weights.Sprintf("%.2f/%.2f", entry->weight, entry->weightDest);
        zSTRING layer;
        layer.Sprintf("%d", entry->ani->layer);
        zSTRING frame;
        frame.Sprintf("%.2f/%d", entry->actFrame, entry->ani->numFrames);
        zSTRING fps;
        fps.Sprintf("%.2f", entry->ani->fpsRate * 1000.0f);
        screen->Print(x, lineY, entry->ani->aniName);
        screen->Print(x + 1800, lineY, weights);
        screen->Print(x + 3300, lineY, layer);
        screen->Print(x + 3900, lineY, frame);
        screen->Print(x + 4500, lineY, fps);
    }
}

zCMorphMeshConvertFileHandler::zCMorphMeshConvertFileHandler()
{
    RegisterFileType("mms");
}

int zCMorphMeshConvertFileHandler::HandleFile(const zSTRING& path, const char* fileName,
    const zFINDDATA&)
{
    zSTRING fullName = path;
    fullName.Append("\\");
    fullName.Append(fileName);
    fullName.Upper();
    if (fullName.Search(".MMS") < 0)
        return 0;

    zCMorphMesh* mesh = zCMorphMesh::Load(fullName);
    if (!mesh)
        return 0;
    delete mesh;
    return 1;
}
