#include "include/zFile3D.h"
#include "include/zObject.h"
#include "include/zPath.h"
#include "include/zTools.h"
#include <cmath>
#include <cstdio>

static int file3dsState = -1;

int zCFileBIN::BinWriteSrcFileStats(zSTRING& fileName)
{
    zPATH path;
    if (fileName.Length())
        path.SearchFile(fileName, zFILE::s_virtPathString, 1);

    zFILE_STATS stats = {};
    zFILE* source = zfactory->CreateZFile(path.GetPath());
    source->GetStats(stats);
    source->Close();
    file->Write(&stats.modifyDate, sizeof(stats.modifyDate));
    file->Write(path.GetPath() + "\n");
    delete source;
    return 1;
}

int zCFileBIN::BinReadCompareSrcFileStats()
{
    zDATE storedDate;
    file->Read(&storedDate, sizeof(storedDate));
    zSTRING sourcePath;
    file->Read(sourcePath);
    if (!sourcePath.Length())
        return 1;

    zPATH path;
    if (path.SearchFile(sourcePath, zFILE::s_virtPathString, 1))
        return 1;

    zFILE_STATS stats = {};
    zFILE* source = zfactory->CreateZFile(path.GetPath());
    source->GetStats(stats);
    delete source;
    return storedDate == stats.modifyDate || storedDate < stats.modifyDate;
}

void TPoly3ds::CheckForWrap()
{
}

struct zT3DSLoadContext {
    zCMesh* mesh;
    zCArray<zVEC3> vertices;
    zCArray<zCVertex*> mergedVertices;
};

static void Read3DSChunks(zFILE* file, long end, zT3DSLoadContext& context)
{
    while (file->Pos() + 6 <= end) {
        const long chunkStart = file->Pos();
        unsigned short id;
        unsigned long length;
        if (file->Read(&id, sizeof(id)) != sizeof(id)
            || file->Read(&length, sizeof(length)) != sizeof(length) || length < 6)
            return;
        const long chunkEnd = chunkStart + length;
        if (chunkEnd > end)
            return;
        if (id == 0x4110) {
            unsigned short count;
            file->Read(&count, sizeof(count));
            context.vertices.Clear();
            context.mergedVertices.Clear();
            for (unsigned short i = 0; i < count; ++i) {
                zVEC3 vertex;
                file->Read(&vertex, sizeof(vertex));
                context.vertices.InsertEnd(vertex);
                context.mergedVertices.InsertEnd(NULL);
            }
        }
        else if (id == 0x4120) {
            unsigned short count;
            file->Read(&count, sizeof(count));
            for (unsigned short i = 0; i < count; ++i) {
                unsigned short indices[3];
                unsigned short flags;
                file->Read(indices, sizeof(indices));
                file->Read(&flags, sizeof(flags));
                zCVertex* triangle[3];
                for (int j = 0; j < 3; ++j) {
                    const int index = indices[j];
                    if (!context.mergedVertices[index])
                        context.mergedVertices[index] = context.mesh->AddVertex(
                            context.vertices[index]);
                    triangle[j] = context.mergedVertices[index];
                }
                context.mesh->AddPoly(triangle[0], triangle[1], triangle[2]);
            }
        }
        else if (id == 0x4D4D || id == 0x3D3D || id == 0x4000 || id == 0x4100)
            Read3DSChunks(file, chunkEnd, context);
        file->Seek(chunkEnd);
    }
}

void zCFile3DS::Load3DS(const zSTRING& fileName, zCMesh* mesh)
{
    zerr.Report(zERR_TYPE_INFO, 0, "D: Loading 3DS-File: " + fileName,
        3, 1, 1061, NULL, NULL);
    const unsigned long controlWord = zfpuControler.GetCurrentControlWord();
    zfpuControler.SetPrecision_53();
    zFILE* file = zfactory->CreateZFile(fileName);
    if (!file->Open(0)) {
        zerr.Report(zERR_TYPE_FAULT, 0, "D: zFile3d(::Load3dsFile): Can't open " + fileName,
            0, 4, 981, NULL, NULL);
    }
    else {
        zT3DSLoadContext context;
        context.mesh = mesh;
        Read3DSChunks(file, file->Size(), context);
        file->Close();
    }
    delete file;
    mesh->CalcBBox3D(0);
    mesh->CalcVertexNormals();
    mesh->meshName = fileName;
    zfpuControler.SetControlWord(controlWord);
}

zCBinTree3DS::zCBinTree3DS()
    : numVerts(0), numPolys(0), vertices(NULL), vertexAlloc(0), vertexCapacity(0), polys(NULL),
      polyCapacity(0), frameCtr(0), nodes(NULL), nodeAlloc(0), nodeCount(0), minX(0.0f),
      maxX(0.0f), splitX(0.0f), center(0.0f)
{
}

zCBinTree3DS::~zCBinTree3DS()
{
    delete[] vertices;
    delete[] polys;
    delete[] nodes;
}

void zCBinTree3DS::GetMatListComplete(zCArray<zCMaterial*>& materials)
{
    for (int i = 0; i < numPolys; ++i) {
        zCMaterial* material = polys[i].material;
        if (!material)
            continue;
        int found = 0;
        for (int j = 0; j < materials.numInArray; ++j) {
            if (materials[j] == material) {
                found = 1;
                break;
            }
        }
        if (!found)
            materials.InsertEnd(material);
    }
}

void zCBinTree3DS::CalcNumPolyVertSubtree(int nodeIndex, int& numPolysValue, int& numVertsValue)
{
    int current = nodeIndex;
    int child = current * 2;
    while (child < nodeCount) {
        CalcNumPolyVertSubtree(child, numPolysValue, numVertsValue);
        current = child + 1;
        child = current * 2;
    }
    numPolysValue += nodes[current].polyList.numInArray;
    numVertsValue += nodes[current].vertList.numInArray;
}

int zCBinTree3DS::CalcNum3DSObject()
{
    int objects = 1;
    int nodeIndex = 1;
    while (nodeIndex < nodeCount) {
        int numPolysValue = 0;
        int numVertsValue = 0;
        CalcNumPolyVertSubtree(nodeIndex, numPolysValue, numVertsValue);
        if (numPolysValue > 65535 || numVertsValue > 65535) {
            nodeIndex *= 2;
            continue;
        }
        ++objects;
        nodeIndex = nodeIndex * 2 + 2;
    }
    return objects;
}

void zCBinTree3DS::GetPolyListFor3DSObj(int nodeIndex, zCArray<int>& polyIndices,
    zCArray<int>& vertIndices)
{
    int current = nodeIndex;
    int child = current * 2;
    while (child < nodeCount) {
        GetPolyListFor3DSObj(child, polyIndices, vertIndices);
        current = child + 1;
        child = current * 2;
    }
    zTBinTreeNode& node = nodes[current];
    for (int i = 0; i < node.polyList.numInArray; ++i) {
        const int polyIndex = node.polyList[i];
        polyIndices.InsertEnd(polyIndex);
        for (int j = 0; j < 3; ++j) {
            const int vertexIndex = polys[polyIndex].vertIndex[j];
            TVertex3ds& vertex = vertices[vertexIndex];
            if (vertex.frameCtr != frameCtr) {
                vertIndices.InsertEnd(vertexIndex);
                vertex.mappedIndex = vertIndices.numInArray - 1;
                vertex.frameCtr = frameCtr;
            }
            polys[polyIndex].vertIndex[j] = vertex.mappedIndex;
        }
    }
}

void zCBinTree3DS::BuildMesh3DS(int objectIndex, int nodeIndex, zCMesh3DS* mesh)
{
    mesh->meshName.Clear();
    for (int i = 0; i < mesh->matList.numInArray; ++i)
        delete mesh->matList[i];
    mesh->matList.Clear();
    mesh->polyList.Clear();
    mesh->vertList.Clear();
    char number[32];
    sprintf(number, "%d", objectIndex);
    mesh->meshName = zSTRING("zenGin") + number;
    ++frameCtr;
    GetPolyListFor3DSObj(nodeIndex, mesh->polyList, mesh->vertList);
    for (int i = 0; i < mesh->polyList.numInArray; ++i) {
        zCMaterial* material = polys[mesh->polyList[i]].material;
        if (!material)
            continue;
        zCMesh3DS::zTMatEntry* entry = NULL;
        for (int j = 0; j < mesh->matList.numInArray; ++j) {
            if (mesh->matList[j]->material == material) {
                entry = mesh->matList[j];
                break;
            }
        }
        if (!entry) {
            entry = new zCMesh3DS::zTMatEntry;
            entry->material = material;
            mesh->matList.InsertEnd(entry);
        }
        entry->polyList.InsertEnd(i);
    }
}

void zCBinTree3DS::CreateNodeList(int numVertsValue, int numPolysValue)
{
    int desired = static_cast<int>(numVertsValue * 0.0666666701f);
    int nodeSize = 1;
    while (nodeSize < desired)
        nodeSize <<= 1;
    if (nodeSize < 512)
        nodeSize = 512;
    else if (nodeSize > 8192)
        nodeSize = 8192;
    delete[] nodes;
    nodeAlloc = nodeSize * 2;
    nodeCount = nodeAlloc;
    nodes = new zTBinTreeNode[nodeAlloc];
}

int zCBinTree3DS::GetLeafIndex(const zVEC3& position)
{
    if (nodeCount <= 1)
        return 1;
    int index = 1;
    float minimum = minX;
    float maximum = maxX;
    float split = splitX;
    while (index < nodeCount) {
        const float middle = (split + minimum) * 0.5f;
        if (position.n[0] >= middle) {
            minimum = middle;
            index = index * 2 + 1;
        }
        else {
            split = middle;
            index *= 2;
        }
    }
    return index;
}

int zCBinTree3DS::GetVert3DSIndex(const zVEC3& position, const zVEC2& texCoord)
{
    const int leafIndex = GetLeafIndex(position);
    zTBinTreeNode& node = nodes[leafIndex];
    for (int i = 0; i < node.vertList.numInArray; ++i) {
        const int index = node.vertList[i];
        const TVertex3ds& vertex = vertices[index];
        if (fabs(position.n[0] - vertex.position.n[0]) < 0.1f
            && fabs(position.n[1] - vertex.position.n[1]) < 0.1f
            && fabs(position.n[2] - vertex.position.n[2]) < 0.1f
            && fabs(texCoord.n[0] - vertex.texCoord.n[0]) < 0.001f
            && fabs(texCoord.n[1] - vertex.texCoord.n[1]) < 0.001f)
            return index;
    }
    TVertex3ds& vertex = vertices[numVerts];
    vertex.position = position;
    vertex.texCoord = texCoord;
    vertex.frameCtr = 0;
    vertex.mappedIndex = 0;
    ++numVerts;
    node.vertList.InsertEnd(numVerts - 1);
    return numVerts - 1;
}

void zCBinTree3DS::DropMesh(zCMesh* mesh)
{
    numVerts = 0;
    numPolys = 0;
    frameCtr = 0;
    int maxVerts = 0;
    int maxPolys = 0;
    for (int i = 0; i < mesh->numPoly; ++i) {
        maxVerts += mesh->polyList[i]->polyNumVert;
        maxPolys += mesh->polyList[i]->polyNumVert - 2;
    }
    delete[] vertices;
    delete[] polys;
    vertexCapacity = maxVerts;
    vertexAlloc = maxVerts;
    vertices = new TVertex3ds[maxVerts];
    polyCapacity = maxPolys;
    polys = new TPoly3ds[maxPolys];
    minX = mesh->bbox3D.mins.n[0];
    maxX = mesh->bbox3D.maxs.n[0];
    splitX = maxX;
    center = (mesh->bbox3D.mins + mesh->bbox3D.maxs) * 0.5f;
    CreateNodeList(maxVerts, maxPolys);
    for (int i = 0; i < mesh->numPoly; ++i) {
        zCPolygon* polygon = mesh->polyList[i];
        for (int j = 1; j + 1 < polygon->polyNumVert; ++j) {
            const int indices[3] = { 0, j, j + 1 };
            for (int k = 0; k < 3; ++k) {
                const int vertexIndex = indices[k];
                const zVEC2 texCoord(polygon->feature[vertexIndex]->mapping.n[0],
                    polygon->feature[vertexIndex]->mapping.n[1]);
                polys[numPolys].vertIndex[k] = GetVert3DSIndex(
                    polygon->vertex[vertexIndex]->position, texCoord);
            }
            polys[numPolys].material = polygon->material;
            polys[numPolys].flags = 0;
            polys[numPolys].numVerts = 3;
            ++numPolys;
        }
    }
}

static long Begin3DSChunk(zFILE* file, unsigned short id)
{
    unsigned long length = 0;
    file->Write(&id, sizeof(id));
    const long lengthPosition = file->Pos();
    file->Write(&length, sizeof(length));
    return lengthPosition;
}

static void End3DSChunk(zFILE* file, long lengthPosition)
{
    const long end = file->Pos();
    const unsigned long length = end - lengthPosition + 4;
    file->Seek(lengthPosition);
    file->Write(&length, sizeof(length));
    file->Seek(end);
}

void zCFile3DS::Save3DS(const zSTRING& fileName, zCMesh* mesh)
{
    zerr.Report(zERR_TYPE_INFO, 0, "D: 3DS-EXP: Saving 3DS-File: " + fileName,
        3, 1, 1939, NULL, NULL);
    zCPolygon::S_ResetMorphedVerts();
    const unsigned long controlWord = zfpuControler.GetCurrentControlWord();
    zfpuControler.SetPrecision_53();
    zCBinTree3DS tree;
    tree.DropMesh(mesh);
    zFILE* file = zfactory->CreateZFile(fileName);
    if (file->Create()) {
        const long mainLength = Begin3DSChunk(file, 0x4D4D);
        const long versionLength = Begin3DSChunk(file, 0x0002);
        const unsigned long version = 3;
        file->Write(&version, sizeof(version));
        End3DSChunk(file, versionLength);
        const long editorLength = Begin3DSChunk(file, 0x3D3D);
        const long objectLength = Begin3DSChunk(file, 0x4000);
        const zSTRING objectName("zenGin");
        file->Write(objectName.ToChar(), objectName.Length() + 1);
        const long meshLength = Begin3DSChunk(file, 0x4100);
        const long verticesLength = Begin3DSChunk(file, 0x4110);
        const unsigned short numVerts = tree.numVerts;
        file->Write(&numVerts, sizeof(numVerts));
        for (int i = 0; i < tree.numVerts; ++i)
            file->Write(&tree.vertices[i].position, sizeof(zVEC3));
        End3DSChunk(file, verticesLength);
        const long coordsLength = Begin3DSChunk(file, 0x4140);
        file->Write(&numVerts, sizeof(numVerts));
        for (int i = 0; i < tree.numVerts; ++i)
            file->Write(&tree.vertices[i].texCoord, sizeof(zVEC2));
        End3DSChunk(file, coordsLength);
        const long facesLength = Begin3DSChunk(file, 0x4120);
        const unsigned short numPolys = tree.numPolys;
        file->Write(&numPolys, sizeof(numPolys));
        for (int i = 0; i < tree.numPolys; ++i) {
            unsigned short indices[3];
            for (int j = 0; j < 3; ++j)
                indices[j] = tree.polys[i].vertIndex[j];
            const unsigned short flags = 0;
            file->Write(indices, sizeof(indices));
            file->Write(&flags, sizeof(flags));
        }
        End3DSChunk(file, facesLength);
        End3DSChunk(file, meshLength);
        End3DSChunk(file, objectLength);
        End3DSChunk(file, editorLength);
        End3DSChunk(file, mainLength);
        file->Close();
    }
    delete file;
    zfpuControler.SetControlWord(controlWord);
}

void zTBBox3D::InitZero()
{
    mins = zVEC3(0.0f);
    maxs = zVEC3(0.0f);
}

const zSTRING& zCMaterial::GetName() const
{
    return GetObjectName();
}

zCMesh3DS::~zCMesh3DS()
{
    meshName.Clear();
    for (int i = 0; i < matList.numInArray; ++i)
        delete matList[i];
    matList.Clear();
    polyList.Clear();
    vertList.Clear();
}

zCFileQuakeBSP::zCFileQuakeBSP()
    : file(NULL)
{
}

zCFileQuakeBSP::~zCFileQuakeBSP()
{
    delete file;
    file = NULL;
}

struct zTQuakeLump {
    int offset;
    int length;
};

struct zTQuakeVertex {
    float position[3];
    float texCoord[2];
    float lightmapCoord[2];
    float normal[3];
    unsigned char color[4];
};

struct zTQuakeFace {
    int texture;
    int effect;
    int type;
    int firstVertex;
    int numVertices;
    int firstMeshVert;
    int numMeshVerts;
    int lightmap;
    int lightmapStart[2];
    int lightmapSize[2];
    float lightmapOrigin[3];
    float lightmapVecs[2][3];
    float normal[3];
    int patchSize[2];
};

void zCFileQuakeBSP::LoadQuakeBSP(const zSTRING& fileName, zCMesh* mesh, zCWorld* world)
{
    zerr.Message("D: QBSP: Loading QuakeBSP: " + fileName);
    file = zfactory->CreateZFile(fileName);
    if (!file->Open(0))
        return;
    char magic[4];
    int version;
    zTQuakeLump lumps[17];
    file->Read(magic, sizeof(magic));
    file->Read(&version, sizeof(version));
    file->Read(lumps, sizeof(lumps));
    file->Seek(lumps[10].offset);
    const int numVertices = lumps[10].length / sizeof(zTQuakeVertex);
    zTQuakeVertex* vertices = new zTQuakeVertex[numVertices];
    file->Read(vertices, lumps[10].length);
    zCArray<zCVertex*> mergedVertices;
    for (int i = 0; i < numVertices; ++i) {
        const zVEC3 position(vertices[i].position[0] * 3.4f,
            vertices[i].position[2] * 3.4f, vertices[i].position[1] * 3.4f);
        mergedVertices.InsertEnd(mesh->AddVertex(position));
    }
    file->Seek(lumps[11].offset);
    const int numMeshVerts = lumps[11].length / 4;
    int* meshVerts = new int[numMeshVerts];
    file->Read(meshVerts, lumps[11].length);
    file->Seek(lumps[1].offset);
    const int numTextures = lumps[1].length / 72;
    zCArray<zCMaterial*> materials;
    for (int i = 0; i < numTextures; ++i) {
        char textureName[72];
        file->Read(textureName, sizeof(textureName));
        textureName[71] = 0;
        zCMaterial* material = new zCMaterial;
        material->SetTexture(zSTRING(textureName));
        materials.InsertEnd(material);
    }
    file->Seek(lumps[13].offset);
    const int numFaces = lumps[13].length / sizeof(zTQuakeFace);
    for (int i = 0; i < numFaces; ++i) {
        zTQuakeFace face;
        file->Read(&face, sizeof(face));
        if (face.type != 1 && face.type != 3)
            continue;
        for (int j = 0; j + 2 < face.numMeshVerts; j += 3) {
            zCVertex* triangle[3];
            for (int k = 0; k < 3; ++k)
                triangle[k] = mergedVertices[face.firstVertex
                    + meshVerts[face.firstMeshVert + j + k]];
            zCPolygon* polygon = mesh->AddPoly(triangle[0], triangle[1], triangle[2]);
            if (face.texture >= 0 && face.texture < materials.numInArray)
                polygon->material = materials[face.texture];
        }
    }
    delete[] meshVerts;
    delete[] vertices;
    mesh->CalcVertexNormals();
    mesh->CalcBBox3D(0);
    mesh->meshName = fileName;
    file->Close();
}
