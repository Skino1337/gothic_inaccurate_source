#include "zProgMesh.h"
#include "zFile3d.h"

#include <cmath>
#include <cstring>

static int progMeshState = -1;
static zVEC3 progMeshOffset;
static zSTRING progMesh3dsExtension(".3DS");
struct zTPMWedgePair {
    zTPMWedge* first;
    zTPMWedge* second;
};

// EXE: 0x0095C9A4
static zCArray<zTPMWedgePair> progMeshWedgePairs;
// EXE: 0x0095C988
static zCArray<int> progMeshTriPermut;
// EXE: 0x0095C994
static zCArray<int> progMeshWedgePermut;
// EXE: 0x0095C9EC
static zCArray<zVEC3> progMeshPosList;
// EXE: 0x0095C9A0
zCProgMeshProto* zCProgMeshBuilder::currentProto;
// EXE: 0x0095C97C
zCArray<zCMaterial*> zCProgMeshBuilder::currentMaterials;
// EXE: 0x0095C9D4
zCArray<zTPMTriangle> zCProgMeshBuilder::currentTriList;
// EXE: 0x0095C9C8
zCArray<unsigned short> zCProgMeshBuilder::currentTriMatIndexList;
// EXE: 0x0095C9E0
zCArray<zTPMWedge*> zCProgMeshBuilder::currentWedgeList;
// EXE: 0x0095C9B0
zCArray<zTPMVertexUpdate> zCProgMeshBuilder::currentVertexUpdates;
// EXE: 0x00982C04
int zCProgMeshProto::s_markMeshMaterials;

zTLazyMesh::zTLazyMesh()
{
    std::memset(this, 0, sizeof(zTLazyMesh));
}

zTLazyMesh::~zTLazyMesh()
{
}

void zCRenderLightContainer::Clear()
{
    numLights = 0;
    doPrelight = 0;
    m_fPreLightDist = 0;
    m_bSmoothPrelit = 0;
}

// EXE: 0x005C0F20
void zCProgMeshBuilder::CreatePMTriPosList()
{
    currentTriList.DeleteList();
    currentTriMatIndexList.DeleteList();
    currentWedgeList.DeleteList();
    currentVertexUpdates.DeleteList();
    for (int i = 0; i < Triangle::triangleList.numInArray; ++i) {
        Triangle* triangle = Triangle::triangleList.parray[i];
        currentTriMatIndexList.InsertEnd(triangle->materialIndex);
        for (int j = 0; j < 3; ++j) {
            zTPMTriangle triangleIndices;
            triangleIndices.wedge[0] = i * 3;
            triangleIndices.wedge[1] = i * 3 + 1;
            triangleIndices.wedge[2] = i * 3 + 2;
            currentTriList.InsertEnd(triangleIndices);
            zTPMWedge* wedge = new zTPMWedge;
            wedge->normal = triangle->normals[j];
            wedge->texUV = triangle->texCoord[j];
            wedge->position = triangle->vertices[j]->index;
            currentWedgeList.InsertEnd(wedge);
        }
    }

    if (currentProto) {
        currentProto->posList.DeleteList();
        for (int i = 0; i < Vertex::vertexList.numInArray; ++i)
            currentProto->posList.InsertEnd(Vertex::vertexList.parray[i]->position);
        if (currentProto->posList.numInArray > 0) {
            currentProto->bbox3D.mins = currentProto->posList.parray[0];
            currentProto->bbox3D.maxs = currentProto->posList.parray[0];
            for (int i = 1; i < currentProto->posList.numInArray; ++i) {
                for (int axis = 0; axis < 3; ++axis) {
                    if (currentProto->posList.parray[i].n[axis] < currentProto->bbox3D.mins.n[axis])
                        currentProto->bbox3D.mins.n[axis] = currentProto->posList.parray[i].n[axis];
                    if (currentProto->posList.parray[i].n[axis] > currentProto->bbox3D.maxs.n[axis])
                        currentProto->bbox3D.maxs.n[axis] = currentProto->posList.parray[i].n[axis];
                }
            }
        }
        float volume = 1.0f;
        for (int axis = 0; axis < 3; ++axis)
            volume *= currentProto->bbox3D.maxs.n[axis] - currentProto->bbox3D.mins.n[axis];
        currentProto->avgTriArea = volume > 0 ? currentTriList.numInArray / volume : 0;
    }
    zTPMVertexUpdate update;
    update.numNewTri = 0;
    update.numNewWedge = 0;
    currentVertexUpdates.InsertEnd(update);
}

void zCProgMeshBuilder::CalcPrelight(zTLazyMesh& source, zTLazyMesh& destination)
{
    destination.colorList.AllocAbs(destination.wedgeList.numInArray);
    destination.colorList.numInArray = destination.colorList.numAlloc;
    zCRenderLightContainer lights;
    lights.CreatePrelightConfig(currentProto->bbox3D);
    lights.Activate(zMAT4(1.0f));
    for (int i = 0; i < destination.wedgeList.numInArray; ++i) {
        const zTPMWedge& wedge = destination.wedgeList.parray[i];
        const zVEC3 color = lights.LightVertex(source.posList.parray[wedge.position], wedge.normal);
        float value = (color.n[0] * 0.299f + color.n[1] * 0.587f + color.n[2] * 0.114f) /
            255.0f;
        if (value < 0)
            value = 0;
        else if (value > 1.0f)
            value = 1.0f;
        destination.colorList.parray[i] = value;
    }
}

void zCProgMeshBuilder::CalcPosNormals(zTLazyMesh& mesh)
{
    mesh.posNormalList.AllocAbs(mesh.posList.numInArray);
    mesh.posNormalList.numInArray = mesh.posNormalList.numAlloc;
    for (int i = 0; i < mesh.wedgeList.numInArray; ++i) {
        zTPMWedge& wedge = mesh.wedgeList.parray[i];
        const float length = std::sqrt(wedge.normal.n[0] * wedge.normal.n[0] +
            wedge.normal.n[1] * wedge.normal.n[1] + wedge.normal.n[2] * wedge.normal.n[2]);
        if (length < 0.98f || length > 1.02f) {
            wedge.normal.n[0] = -0.70710678f;
            wedge.normal.n[1] = -0.70710678f;
            wedge.normal.n[2] = 0;
        }
        if (wedge.position < mesh.posNormalList.numInArray)
            mesh.posNormalList.parray[wedge.position] = wedge.normal;
    }
}

void zCProgMeshBuilder::CalcTriPlaneList(const zTLazyMesh& source, zTLazyMesh& destination)
{
    destination.triPlaneIndexList.AllocAbs(source.triList.numInArray);
    destination.triPlaneIndexList.numInArray = destination.triPlaneIndexList.numAlloc;
    destination.triPlaneList.AllocAbs(source.triList.numInArray);
    for (int i = 0; i < source.triList.numInArray; ++i) {
        const zTPMTriangle& triangle = source.triList.parray[i];
        const zVEC3& first = source.posList.parray[source.wedgeList.parray[triangle.wedge[0]].position];
        const zVEC3& second = source.posList.parray[source.wedgeList.parray[triangle.wedge[1]].position];
        const zVEC3& third = source.posList.parray[source.wedgeList.parray[triangle.wedge[2]].position];
        zVEC3 firstEdge;
        zVEC3 secondEdge;
        for (int axis = 0; axis < 3; ++axis) {
            firstEdge.n[axis] = second.n[axis] - first.n[axis];
            secondEdge.n[axis] = third.n[axis] - second.n[axis];
        }
        zTPlane plane;
        plane.normal.n[0] = firstEdge.n[1] * secondEdge.n[2] - firstEdge.n[2] * secondEdge.n[1];
        plane.normal.n[1] = firstEdge.n[2] * secondEdge.n[0] - firstEdge.n[0] * secondEdge.n[2];
        plane.normal.n[2] = firstEdge.n[0] * secondEdge.n[1] - firstEdge.n[1] * secondEdge.n[0];
        const float length = std::sqrt(plane.normal.n[0] * plane.normal.n[0] +
            plane.normal.n[1] * plane.normal.n[1] + plane.normal.n[2] * plane.normal.n[2]);
        if (length > 0) {
            for (int axis = 0; axis < 3; ++axis)
                plane.normal.n[axis] /= length;
        }
        plane.distance = plane.normal.n[0] * second.n[0] + plane.normal.n[1] * second.n[1] +
            plane.normal.n[2] * second.n[2];

        int planeIndex = -1;
        for (int j = 0; j < destination.triPlaneList.numInArray; ++j) {
            const float dot = plane.normal.n[0] * destination.triPlaneList.parray[j].normal.n[0] +
                plane.normal.n[1] * destination.triPlaneList.parray[j].normal.n[1] +
                plane.normal.n[2] * destination.triPlaneList.parray[j].normal.n[2];
            if (dot > 0.95f) {
                planeIndex = j;
                break;
            }
        }
        if (planeIndex < 0) {
            destination.triPlaneList.InsertEnd(plane);
            planeIndex = destination.triPlaneList.numInArray - 1;
        }
        destination.triPlaneIndexList.parray[i] = planeIndex;
    }
}

void zCProgMeshBuilder::CalcSubdivData(const zTLazyMesh& source, zTLazyMesh& destination)
{
    destination.triEdgeList.AllocAbs(source.triList.numInArray);
    destination.triEdgeList.numInArray = destination.triEdgeList.numAlloc;
    for (int i = 0; i < source.triList.numInArray; ++i) {
        const zTPMTriangle& triangle = source.triList.parray[i];
        for (int j = 0; j < 3; ++j) {
            const unsigned short first = triangle.wedge[j];
            const unsigned short second = triangle.wedge[(j + 1) % 3];
            int edgeIndex = -1;
            for (int k = 0; k < destination.edgeList.numInArray; ++k) {
                const zTPMEdge& edge = destination.edgeList.parray[k];
                if ((edge.wedge[0] == first && edge.wedge[1] == second) ||
                    (edge.wedge[0] == second && edge.wedge[1] == first)) {
                    edgeIndex = k;
                    break;
                }
            }
            if (edgeIndex < 0) {
                zTPMEdge edge;
                edge.wedge[0] = first;
                edge.wedge[1] = second;
                destination.edgeList.InsertEnd(edge);
                destination.edgeScoreList.InsertEnd(0);
                edgeIndex = destination.edgeList.numInArray - 1;
                const zVEC3& firstPos = source.posList.parray[source.wedgeList.parray[first].position];
                const zVEC3& secondPos = source.posList.parray[source.wedgeList.parray[second].position];
                const float dx = firstPos.n[0] - secondPos.n[0];
                const float dy = firstPos.n[1] - secondPos.n[1];
                const float dz = firstPos.n[2] - secondPos.n[2];
                destination.edgeScoreList.parray[edgeIndex] =
                    std::sqrt(dx * dx + dy * dy + dz * dz) * 0.2f;
            }
            destination.triEdgeList.parray[i].edge[j] = edgeIndex;
        }
    }
}

void zCProgMeshBuilder::CreateLazySubMesh(int flags, zTLazyMesh& source, zTLazyMesh& destination)
{
    if (destination.matList.numInArray <= 0)
        return;
    zCMaterial* material = destination.matList.parray[0];
    zCArray<int> wedgeRemap;
    wedgeRemap.AllocAbs(source.wedgeList.numInArray);
    wedgeRemap.numInArray = wedgeRemap.numAlloc;
    for (int i = 0; i < source.wedgeList.numInArray; ++i)
        wedgeRemap.parray[i] = -1;

    for (int i = 0; i < source.triList.numInArray; ++i) {
        if (source.matList.parray[source.triMatIndexList.parray[i]] != material)
            continue;
        zTPMTriangle triangle;
        for (int j = 0; j < 3; ++j) {
            const int oldWedge = source.triList.parray[i].wedge[j];
            if (wedgeRemap.parray[oldWedge] < 0) {
                wedgeRemap.parray[oldWedge] = destination.wedgeList.numInArray;
                destination.wedgeList.InsertEnd(source.wedgeList.parray[oldWedge]);
            }
            triangle.wedge[j] = wedgeRemap.parray[oldWedge];
        }
        destination.triList.InsertEnd(triangle);
    }

    destination.wedgeMap.AllocAbs(destination.wedgeList.numInArray);
    destination.wedgeMap.numInArray = destination.wedgeMap.numAlloc;
    for (int i = 0; i < destination.wedgeMap.numInArray; ++i) {
        int parent = -1;
        for (int j = 0; j < wedgeRemap.numInArray; ++j) {
            if (wedgeRemap.parray[j] == i) {
                parent = j;
                break;
            }
        }
        destination.wedgeMap.parray[i] = parent >= 0 && parent < source.wedgeMap.numInArray ?
            source.wedgeMap.parray[parent] : parent;
    }
    destination.vertexUpdates.DeleteList();
    zTPMVertexUpdate update;
    update.numNewTri = destination.triList.numInArray;
    update.numNewWedge = destination.wedgeList.numInArray;
    destination.vertexUpdates.InsertEnd(update);
    if (!(flags & 2))
        CalcTriPlaneList(source, destination);
    if (flags & 0x20)
        CalcSubdivData(source, destination);
}

void zCProgMeshBuilder::CopySubMeshData(unsigned char*& data, const zTLazyMesh& source,
    zCSubMesh* destination)
{
    destination->triList.numInArray = source.triList.numInArray;
    destination->triList.array = reinterpret_cast<zTPMTriangle*>(data);
    std::memcpy(destination->triList.array, source.triList.parray,
        source.triList.numInArray * sizeof(zTPMTriangle));
    data += source.triList.numInArray * sizeof(zTPMTriangle);
    destination->wedgeList.numInArray = source.wedgeList.numInArray;
    destination->wedgeList.array = reinterpret_cast<zTPMWedge*>(data);
    std::memcpy(destination->wedgeList.array, source.wedgeList.parray,
        source.wedgeList.numInArray * sizeof(zTPMWedge));
    data += source.wedgeList.numInArray * sizeof(zTPMWedge);
    destination->wedgeMap.numInArray = source.wedgeMap.numInArray;
    destination->wedgeMap.array = reinterpret_cast<unsigned short*>(data);
    std::memcpy(destination->wedgeMap.array, source.wedgeMap.parray,
        source.wedgeMap.numInArray * sizeof(unsigned short));
    data += source.wedgeMap.numInArray * sizeof(unsigned short);
    destination->colorList.numInArray = source.colorList.numInArray;
    destination->colorList.array = reinterpret_cast<float*>(data);
    std::memcpy(destination->colorList.array, source.colorList.parray,
        source.colorList.numInArray * sizeof(float));
    data += source.colorList.numInArray * sizeof(float);
    destination->vertexUpdates.numInArray = source.vertexUpdates.numInArray;
    destination->vertexUpdates.array = reinterpret_cast<zTPMVertexUpdate*>(data);
    std::memcpy(destination->vertexUpdates.array, source.vertexUpdates.parray,
        source.vertexUpdates.numInArray * sizeof(zTPMVertexUpdate));
    data += source.vertexUpdates.numInArray * sizeof(zTPMVertexUpdate);
    destination->triPlaneIndexList.numInArray = source.triPlaneIndexList.numInArray;
    destination->triPlaneIndexList.array = reinterpret_cast<unsigned short*>(data);
    std::memcpy(destination->triPlaneIndexList.array, source.triPlaneIndexList.parray,
        source.triPlaneIndexList.numInArray * sizeof(unsigned short));
    data += source.triPlaneIndexList.numInArray * sizeof(unsigned short);
    destination->triPlaneList.numInArray = source.triPlaneList.numInArray;
    destination->triPlaneList.array = reinterpret_cast<zTPlane*>(data);
    std::memcpy(destination->triPlaneList.array, source.triPlaneList.parray,
        source.triPlaneList.numInArray * sizeof(zTPlane));
    data += source.triPlaneList.numInArray * sizeof(zTPlane);
    destination->triEdgeList.numInArray = source.triEdgeList.numInArray;
    destination->triEdgeList.array = reinterpret_cast<zTPMTriangleEdges*>(data);
    std::memcpy(destination->triEdgeList.array, source.triEdgeList.parray,
        source.triEdgeList.numInArray * sizeof(zTPMTriangleEdges));
    data += source.triEdgeList.numInArray * sizeof(zTPMTriangleEdges);
    destination->edgeList.numInArray = source.edgeList.numInArray;
    destination->edgeList.array = reinterpret_cast<zTPMEdge*>(data);
    std::memcpy(destination->edgeList.array, source.edgeList.parray,
        source.edgeList.numInArray * sizeof(zTPMEdge));
    data += source.edgeList.numInArray * sizeof(zTPMEdge);
    destination->edgeScoreList.numInArray = source.edgeScoreList.numInArray;
    destination->edgeScoreList.array = reinterpret_cast<float*>(data);
    std::memcpy(destination->edgeScoreList.array, source.edgeScoreList.parray,
        source.edgeScoreList.numInArray * sizeof(float));
    data += source.edgeScoreList.numInArray * sizeof(float);
}

// EXE: inlined in 0x005C2B50
static void CopyBuilderInput(zTLazyMesh& destination)
{
    for (int i = 0; i < zCProgMeshBuilder::currentProto->posList.numInArray; ++i)
        destination.posList.InsertEnd(zCProgMeshBuilder::currentProto->posList.parray[i]);
    for (int i = 0; i < zCProgMeshBuilder::currentMaterials.numInArray; ++i)
        destination.matList.InsertEnd(zCProgMeshBuilder::currentMaterials.parray[i]);
    for (int i = 0; i < zCProgMeshBuilder::currentTriMatIndexList.numInArray; ++i)
        destination.triMatIndexList.InsertEnd(zCProgMeshBuilder::currentTriMatIndexList.parray[i]);
    for (int i = 0; i < zCProgMeshBuilder::currentTriList.numInArray; ++i)
        destination.triList.InsertEnd(zCProgMeshBuilder::currentTriList.parray[i]);
    for (int i = 0; i < zCProgMeshBuilder::currentWedgeList.numInArray; ++i)
        destination.wedgeList.InsertEnd(*zCProgMeshBuilder::currentWedgeList.parray[i]);
}

// EXE: 0x005C2B50
void zCProgMeshBuilder::CreateFinalPMData(int flags)
{
    if (!currentProto)
        return;
    currentProto->posNormalList.DeleteList();
    for (int i = 0; i < currentProto->posList.numInArray; ++i) {
        zVEC3 normal;
        normal.n[0] = 0;
        normal.n[1] = 0;
        normal.n[2] = 0;
        currentProto->posNormalList.InsertEnd(normal);
    }
    currentProto->numSubMeshes = currentMaterials.numInArray;
    currentProto->subMeshList = new zCSubMesh[currentProto->numSubMeshes];
    unsigned long totalSize = 0;
    for (int i = 0; i < currentMaterials.numInArray; ++i) {
        zTLazyMesh base;
        CopyBuilderInput(base);
        zTLazyMesh lazy;
        lazy.matList.InsertEnd(currentMaterials.parray[i]);
        CreateLazySubMesh(flags, base, lazy);
        CalcPrelight(lazy, lazy);
        totalSize += lazy.triList.numInArray * sizeof(zTPMTriangle);
        totalSize += lazy.wedgeList.numInArray * sizeof(zTPMWedge);
        totalSize += lazy.wedgeMap.numInArray * sizeof(unsigned short);
        totalSize += lazy.colorList.numInArray * sizeof(float);
        totalSize += lazy.vertexUpdates.numInArray * sizeof(zTPMVertexUpdate);
        totalSize += lazy.triPlaneIndexList.numInArray * sizeof(unsigned short);
        totalSize += lazy.triPlaneList.numInArray * sizeof(zTPlane);
        totalSize += lazy.triEdgeList.numInArray * sizeof(zTPMTriangleEdges);
        totalSize += lazy.edgeList.numInArray * sizeof(zTPMEdge);
        totalSize += lazy.edgeScoreList.numInArray * sizeof(float);
    }
    currentProto->dataSize = totalSize;
    currentProto->dataPool = new unsigned char[totalSize];
    unsigned char* data = currentProto->dataPool;
    for (int i = 0; i < currentProto->numSubMeshes; ++i) {
        zTLazyMesh base;
        CopyBuilderInput(base);
        zTLazyMesh lazy;
        lazy.matList.InsertEnd(currentMaterials.parray[i]);
        CreateLazySubMesh(flags, base, lazy);
        CalcPrelight(lazy, lazy);
        currentProto->subMeshList[i].material = currentMaterials.parray[i];
        CopySubMeshData(data, lazy, &currentProto->subMeshList[i]);
        currentProto->subMeshList[i].vbStartIndex = 0;
    }
}

// EXE: 0x005C3C50
void zCProgMeshBuilder::Clear()
{
    for (int i = 0; i < Triangle::triangleList.numInArray; ++i)
        delete Triangle::triangleList.parray[i];
    Triangle::triangleList.DeleteList();
    for (int i = 0; i < Vertex::vertexList.numInArray; ++i)
        delete Vertex::vertexList.parray[i];
    Vertex::vertexList.DeleteList();
    currentTriList.DeleteList();
    currentTriMatIndexList.DeleteList();
    for (int i = 0; i < currentWedgeList.numInArray; ++i)
        delete currentWedgeList.parray[i];
    currentWedgeList.DeleteList();
    currentVertexUpdates.DeleteList();
    currentMaterials.DeleteList();
    currentProto = 0;
}

int zCProgMeshBuilder::BuildProgMeshProto(zCMesh* mesh, zCProgMeshProto* proto,
    zCArray<int>* wedgeMap, int flags)
{
    Clear();
    currentProto = proto;
    for (int i = 0; i < mesh->numVerts; ++i)
        new Vertex(mesh->vertexList[i], i);
    for (int i = 0; i < mesh->numPoly; ++i) {
        zCPolygon& polygon = mesh->polyList[i][0];
        zCMaterial* material = polygon.material;
        int materialIndex = -1;
        for (int j = 0; j < currentMaterials.numInArray; ++j) {
            if (currentMaterials.parray[j] == material) {
                materialIndex = j;
                break;
            }
        }
        if (materialIndex < 0) {
            currentMaterials.InsertEnd(material);
            material->AddRef();
            materialIndex = currentMaterials.numInArray - 1;
        }
        Vertex* vertices[3];
        for (int j = 0; j < 3; ++j) {
            int vertexIndex = 0;
            for (int k = 0; k < mesh->numVerts; ++k) {
                if (&mesh->vertexList[k] == polygon.vertex[j]) {
                    vertexIndex = k;
                    break;
                }
            }
            vertices[j] = Vertex::vertexList.parray[vertexIndex];
        }
        zVEC2 uv[3];
        for (int j = 0; j < 3; ++j) {
            uv[j].n[0] = polygon.feature[j]->mapping.n[0];
            uv[j].n[1] = polygon.feature[j]->mapping.n[1];
        }
        new Triangle(vertices[0], vertices[1], vertices[2], uv[0], uv[1], uv[2],
            polygon.feature[0]->vertNormal, polygon.feature[1]->vertNormal,
            polygon.feature[2]->vertNormal, materialIndex, 0);
    }
    if (wedgeMap) {
        for (int i = 0; i < Vertex::vertexList.numInArray && i < wedgeMap->numInArray; ++i)
            Vertex::vertexList.parray[i]->index = wedgeMap->parray[i];
    }
    CreatePMTriPosList();
    CreateFinalPMData(flags);
    Clear();
    return 1;
}

// EXE: 0x005C65A0
zCObject* zCProgMeshProto::_CreateNewInstance()
{
    zCProgMeshProto* mesh = new zCProgMeshProto;
    zCClassDef::ObjectCreated(mesh, classDef);
    return mesh;
}

// EXE: 0x005C67F0
zCObject* zCMeshSoftSkin::_CreateNewInstance()
{
    zCMeshSoftSkin* mesh = new zCMeshSoftSkin;
    zCClassDef::ObjectCreated(mesh, classDef);
    return mesh;
}

int zCProgMeshProto::IsBBox3DLocal()
{
    return 1;
}

zTBBox3D zCProgMeshProto::GetBBox3D()
{
    return bbox3D;
}

zCOBBox3D* zCProgMeshProto::GetOBBox3D()
{
    return &obbox3D;
}

void zCProgMeshProto::DynLightVisual(const zCArray<zCVobLight*>&, zMAT4*)
{
}

unsigned long zCProgMeshProto::GetRenderSortKey() const
{
    return renderSortKey;
}

int zCProgMeshProto::CanTraceRay() const
{
    return 1;
}

int zCProgMeshProto::GetAlphaTestingEnabled() const
{
    return m_bUsesAlphaTesting;
}

void zCProgMeshProto::SetAlphaTestingEnabled(int enabled)
{
    m_bUsesAlphaTesting = enabled;
}

zCClassDef* zCMeshSoftSkin::_GetClassDef() const
{
    return classDef;
}

zCSubMesh::zCSubMesh()
{
    std::memset(this, 0, sizeof(zCSubMesh));
}

zCSubMesh::~zCSubMesh()
{
    if (material)
        material->Release();
    material = 0;
    triList.array = 0;
    wedgeList.array = 0;
    colorList.array = 0;
    triPlaneIndexList.array = 0;
    triPlaneList.array = 0;
    triEdgeList.array = 0;
    edgeList.array = 0;
    edgeScoreList.array = 0;
    wedgeMap.array = 0;
    vertexUpdates.array = 0;
}

unsigned long zCProgMeshProto::s_classCtorCtr;
float zCProgMeshProto::s_lodParamStrengthModifier = 0.55f;

zCProgMeshProto::zCProgMeshProto()
    : numSubMeshes(0), subMeshList(0), vertexBufferStatic(0), avgTriArea(0),
      dataPool(0), dataSize(0), m_bUsesAlphaTesting(0), refCtr(1)
{
    std::memset(&bbox3D, 0, sizeof(bbox3D));
    std::memset(&obbox3D, 0, sizeof(obbox3D));
    lodParams.strength = 1.0f;
    lodParams.zDisplace2 = 0;
    lodParams.morphPerc = 0;
    lodParams.minVerts = 0;
    pmeshID = s_classCtorCtr++;
    lightingMode = 1;
    CalcRenderSortKey();
}

zCProgMeshProto::~zCProgMeshProto()
{
    if (vertexBufferStatic)
        zvertexBufferMan.ReleaseVertexBuffer(vertexBufferStatic);
    vertexBufferStatic = 0;
    delete[] subMeshList;
    subMeshList = 0;
    delete[] dataPool;
    dataPool = 0;
    posList.DeleteList();
    posNormalList.DeleteList();
}

zCClassDef* zCProgMeshProto::_GetClassDef() const
{
    return classDef;
}

void zCProgMeshProto::CalcRenderSortKey()
{
    if (!vertexBufferStatic)
        renderSortKey = pmeshID | 0xf0000000;
    else
        renderSortKey = ((vertexBufferStatic->vertexBufferID | 0xfffff000) << 16) | pmeshID;
}

void zCProgMeshProto::SetLODParamStrengthModifier(float value)
{
    s_lodParamStrengthModifier = value;
    if (value < 0)
        value = 0;
    else if (value > 1)
        value = 1;
    s_lodParamStrengthModifier = (1.0f - value) * 0.6f + 0.55f;
}

float zCProgMeshProto::GetLODParamStrengthModifier()
{
    return 1.0f - (s_lodParamStrengthModifier - 0.55f) / 0.6f;
}

void zCProgMeshProto::PackStaticVertexBuffers()
{
    int numVertices = 0;
    for (int i = 0; i < numSubMeshes; ++i)
        numVertices += subMeshList[i].wedgeList.numInArray;
    if (!vertexBufferStatic) {
        vertexBufferStatic = new zCVertexBuffer;
        vertexBufferStatic->positionList = new zVEC3[numVertices];
        vertexBufferStatic->normalList = new zVEC3[numVertices];
        vertexBufferStatic->texCoordList = new zVEC2[numVertices];
        vertexBufferStatic->arrayStride = numVertices;
    }
    int vertexIndex = 0;
    for (int i = 0; i < numSubMeshes; ++i) {
        zCSubMesh& subMesh = subMeshList[i];
        subMesh.vbStartIndex = vertexIndex;
        for (int j = 0; j < subMesh.wedgeList.numInArray; ++j) {
            const zTPMWedge& wedge = subMesh.wedgeList.array[j];
            vertexBufferStatic->positionList[vertexIndex] = posList.parray[wedge.position];
            vertexBufferStatic->normalList[vertexIndex] = wedge.normal;
            vertexBufferStatic->texCoordList[vertexIndex] = wedge.texUV;
            ++vertexIndex;
        }
    }
    CalcRenderSortKey();
}

const zSTRING* zCProgMeshProto::GetFileExtension(int index)
{
    static zSTRING extension(".3DS");
    return index ? 0 : &extension;
}

zCVisual* zCProgMeshProto::LoadVisualVirtual(const zSTRING& fileName) const
{
    return Load(fileName);
}

zSTRING zCProgMeshProto::GetVisualName()
{
    return GetObjectName();
}

int zCProgMeshProto::BuildProgMeshProto(zCMesh* mesh, zCProgMeshProto* proto,
    zCArray<int>* wedgeMap, int flags)
{
    zCProgMeshBuilder* builder = new zCProgMeshBuilder;
    const int result = builder->BuildProgMeshProto(mesh, proto, wedgeMap, flags);
    delete builder;
    return result;
}

static float DotProduct(const zVEC3& first, const zVEC3& second)
{
    return first.n[0] * second.n[0] + first.n[1] * second.n[1] + first.n[2] * second.n[2];
}

static zVEC3 SubtractVector(const zVEC3& first, const zVEC3& second)
{
    zVEC3 result;
    for (int i = 0; i < 3; ++i)
        result.n[i] = first.n[i] - second.n[i];
    return result;
}

static zVEC3 CrossProduct(const zVEC3& first, const zVEC3& second)
{
    zVEC3 result;
    result.n[0] = first.n[1] * second.n[2] - first.n[2] * second.n[1];
    result.n[1] = first.n[2] * second.n[0] - first.n[0] * second.n[2];
    result.n[2] = first.n[0] * second.n[1] - first.n[1] * second.n[0];
    return result;
}

static int CheckPMTriangle(const zCArray<zVEC3>& positions, zCSubMesh* subMesh,
    int triIndex, const zVEC3& origin, const zVEC3& direction, zVEC3& intersection,
    float& distance, int twoSided)
{
    const zTPlane& plane = subMesh->triPlaneList.array[subMesh->triPlaneIndexList.array[triIndex]];
    const float denominator = DotProduct(plane.normal, direction);
    if ((!twoSided && denominator >= 0) || denominator == 0)
        return 0;
    distance = -(DotProduct(plane.normal, origin) + plane.distance) / denominator;
    if (distance < 0 || distance > 1)
        return 0;
    for (int i = 0; i < 3; ++i)
        intersection.n[i] = origin.n[i] + direction.n[i] * distance;
    const zTPMTriangle& triangle = subMesh->triList.array[triIndex];
    const zVEC3& first = positions.parray[subMesh->wedgeList.array[triangle.wedge[0]].position];
    const zVEC3& second = positions.parray[subMesh->wedgeList.array[triangle.wedge[1]].position];
    const zVEC3& third = positions.parray[subMesh->wedgeList.array[triangle.wedge[2]].position];
    const zVEC3 edge1 = SubtractVector(second, first);
    const zVEC3 edge2 = SubtractVector(third, first);
    const zVEC3 p = CrossProduct(direction, edge2);
    const float determinant = DotProduct(edge1, p);
    if ((!twoSided && determinant <= 0) || determinant == 0)
        return 0;
    const zVEC3 t = SubtractVector(origin, first);
    const float u = DotProduct(t, p) / determinant;
    if (u < 0 || u > 1)
        return 0;
    const zVEC3 q = CrossProduct(t, edge1);
    const float v = DotProduct(direction, q) / determinant;
    return v >= 0 && u + v <= 1;
}

int zCProgMeshProto::CheckRayPolyIntersection(zCSubMesh* subMesh, int triIndex,
    const zVEC3& origin, const zVEC3& direction, zVEC3& intersection, float& distance) const
{
    return CheckPMTriangle(posList, subMesh, triIndex, origin, direction, intersection,
        distance, 0);
}

int zCProgMeshProto::CheckRayPolyIntersection2Sided(zCSubMesh* subMesh, int triIndex,
    const zVEC3& origin, const zVEC3& direction, zVEC3& intersection, float& distance) const
{
    return CheckPMTriangle(posList, subMesh, triIndex, origin, direction, intersection,
        distance, 1);
}

int zCProgMeshProto::TraceRay(const zVEC3& origin, const zVEC3& direction, int flags,
    zTTraceRayReport& report)
{
    report.foundHit = 0;
    report.foundPoly = 0;
    float nearest = 999.0f;
    zTPlane* foundPlane = 0;
    for (int i = 0; i < numSubMeshes; ++i) {
        zCSubMesh& subMesh = subMeshList[i];
        if (!subMesh.triPlaneList.numInArray)
            continue;
        for (int j = 0; j < subMesh.triList.numInArray; ++j) {
            zVEC3 intersection;
            float distance;
            const int hit = (flags & 0x400)
                ? CheckRayPolyIntersection2Sided(&subMesh, j, origin, direction, intersection, distance)
                : CheckRayPolyIntersection(&subMesh, j, origin, direction, intersection, distance);
            if (hit && distance < nearest) {
                nearest = distance;
                report.foundHit = 1;
                report.foundHitPosition = intersection;
                foundPlane = &subMesh.triPlaneList.array[subMesh.triPlaneIndexList.array[j]];
            }
        }
    }
    if (report.foundHit && foundPlane)
        report.foundPolyNormal = foundPlane->normal;
    return report.foundHit;
}

int zCProgMeshProto::GetNumTri() const
{
    int result = 0;
    for (int i = 0; i < numSubMeshes; ++i)
        result += subMeshList[i].triList.numInArray;
    return result;
}

static zCSubMesh* FindPMSubMesh(zCProgMeshProto* proto, int& triIndex)
{
    for (int i = 0; i < proto->numSubMeshes; ++i) {
        zCSubMesh& subMesh = proto->subMeshList[i];
        if (triIndex < subMesh.triList.numInArray)
            return &subMesh;
        triIndex -= subMesh.triList.numInArray;
    }
    return &proto->subMeshList[proto->numSubMeshes - 1];
}

zCPolygon* zCProgMeshProto::GetPolygon(int index)
{
    static zCPolygon* polygon;
    if (!polygon)
        polygon = new zCPolygon;
    zCSubMesh* subMesh = FindPMSubMesh(this, index);
    const zTPMTriangle& triangle = subMesh->triList.array[index];
    zCVertex* vertices[3];
    for (int i = 0; i < 3; ++i) {
        vertices[i] = new zCVertex;
        vertices[i]->position = posList.parray[subMesh->wedgeList.array[triangle.wedge[i]].position];
    }
    polygon->SetVertices(vertices[0], vertices[1], vertices[2]);
    polygon->material = subMesh->material;
    polygon->polyPlane = subMesh->triPlaneList.array[subMesh->triPlaneIndexList.array[index]];
    return polygon;
}

zCMesh* zCProgMeshProto::GetMesh(int)
{
    zCMesh* mesh = new zCMesh;
    mesh->numPoly = GetNumTri();
    mesh->numVerts = mesh->numPoly * 3;
    mesh->polyArray = new zCPolygon[mesh->numPoly];
    mesh->polyList = new zCPolygon*[mesh->numPoly];
    mesh->vertexList = new zVEC3[mesh->numVerts];
    int polyIndex = 0;
    int vertexIndex = 0;
    for (int i = 0; i < numSubMeshes; ++i) {
        zCSubMesh& subMesh = subMeshList[i];
        for (int j = 0; j < subMesh.triList.numInArray; ++j) {
            zCPolygon& polygon = mesh->polyArray[polyIndex];
            mesh->polyList[polyIndex] = &polygon;
            polygon.material = subMesh.material;
            if (subMesh.material)
                subMesh.material->AddRef();
            polygon.polyNumVert = 3;
            polygon.vertex = new zCVertex*[3];
            polygon.feature = new zCVertFeature*[3];
            for (int k = 0; k < 3; ++k) {
                const zTPMWedge& wedge = subMesh.wedgeList.array[subMesh.triList.array[j].wedge[k]];
                mesh->vertexList[vertexIndex] = posList.parray[wedge.position];
                polygon.vertex[k] = reinterpret_cast<zCVertex*>(&mesh->vertexList[vertexIndex]);
                polygon.feature[k] = new zCVertFeature;
                polygon.feature[k]->vertNormal = wedge.normal;
                polygon.feature[k]->mapping.n[0] = wedge.texUV.n[0];
                polygon.feature[k]->mapping.n[1] = wedge.texUV.n[1];
                ++vertexIndex;
            }
            polygon.polyPlane = subMesh.triPlaneList.array[subMesh.triPlaneIndexList.array[j]];
            ++polyIndex;
        }
    }
    mesh->meshName = GetObjectName();
    return mesh;
}

int zCProgMeshProto::GetLowestLODNumPolys()
{
    return GetNumTri();
}

zVEC3* zCProgMeshProto::GetLowestLODPoly(int index, zVEC3*& normal)
{
    static zVEC3 vertices[3];
    static zVEC3 defaultNormal;
    defaultNormal.n[0] = 0;
    defaultNormal.n[1] = 1.0f;
    defaultNormal.n[2] = 0;
    if (index > 0)
        --index;
    zCSubMesh* subMesh = FindPMSubMesh(this, index);
    const zTPMTriangle& triangle = subMesh->triList.array[index];
    for (int i = 0; i < 3; ++i)
        vertices[i] = posList.parray[subMesh->wedgeList.array[triangle.wedge[i]].position];
    if (subMesh->triPlaneList.numInArray)
        normal = &subMesh->triPlaneList.array[subMesh->triPlaneIndexList.array[index]].normal;
    else
        normal = &defaultNormal;
    return vertices;
}

int zCProgMeshProto::s_frameCtr;
int zCProgMeshProto::s_autoSubdivEnabled;
zCVertexBufferDyn* zCProgMeshProto::s_vertBufferNormal;
zCVertexBufferDyn* zCProgMeshProto::s_vertBufferColor;

zCVertexBufferDyn::zCVertexBufferDyn(int, int)
{
}

void zCProgMeshProto::InitProgMeshProto()
{
    s_frameCtr = 0;
    s_autoSubdivEnabled = 1;
    CreateDynamicVertexBuffer();
}

int zCProgMeshProto::CreateDynamicVertexBuffer()
{
    s_vertBufferNormal = new zCVertexBufferDyn(0x15, 0x1000);
    return 1;
}

void zCProgMeshProto::CleanupProgMeshProto()
{
    s_frameCtr = 0;
    delete s_vertBufferColor;
    s_vertBufferColor = 0;
    delete s_vertBufferNormal;
    s_vertBufferNormal = 0;
}

float zCProgMeshProto::s_lodParamStrengthOverride;
float zEFFECT_VERT_PERC = 1.0f;

float zCProgMeshProto::CalcLODRenderVertPerc(const zTRenderContext& context)
{
    if (numSubMeshes <= 0)
        return 0;
    if (context.distVobToCam <= 0)
        return 1;
    float size = bbox3D.maxs.n[0] - bbox3D.mins.n[0];
    for (int i = 1; i < 3; ++i) {
        const float extent = bbox3D.maxs.n[i] - bbox3D.mins.n[i];
        if (extent > size)
            size = extent;
    }
    float strength = s_lodParamStrengthOverride != 0
        ? s_lodParamStrengthOverride : lodParams.strength;
    const int numUpdates = subMeshList[0].vertexUpdates.numInArray;
    float result = size * 1000.0f * strength * s_lodParamStrengthModifier /
        (context.distVobToCam * numUpdates);
    if (result > 4.0f)
        result = 4.0f;
    if (result < 0)
        result = 0;
    return result;
}

static int CalcPMLODArgs(zCProgMeshProto* proto, float percent, zTLODRenderArgs& args)
{
    args.vertPerc = percent;
    if (percent > 1.0f)
        percent = 1.0f;
    const int numUpdates = proto->numSubMeshes > 0
        ? proto->subMeshList[0].vertexUpdates.numInArray : 0;
    if (proto->numSubMeshes <= 0 || numUpdates <= 1) {
        args.morphFrac = 1.0f;
        args.numPos = proto->numSubMeshes > 0 ? proto->posList.numInArray : 0;
        args.numMinMorphPos = args.numPos;
        return proto->numSubMeshes > 0;
    }
    float numPos = (numUpdates - 1) * percent;
    if (numPos < proto->lodParams.minVerts)
        numPos = proto->lodParams.minVerts;
    if (numPos < 3.0f)
        return 0;
    args.numPos = static_cast<int>(numPos);
    args.numMinMorphPos = args.numPos;
    args.morphFrac = 1.0f;
    const int morphStep = static_cast<int>(numUpdates * proto->lodParams.morphPerc);
    if (morphStep <= 1)
        return 1;
    const int upper = static_cast<int>(std::ceil(numPos / morphStep)) * morphStep;
    args.numPos = upper < numUpdates ? upper : numUpdates - 1;
    args.numMinMorphPos = args.numPos - morphStep;
    args.morphFrac = (numPos - args.numMinMorphPos) /
        (args.numPos - args.numMinMorphPos);
    if (args.morphFrac < 0)
        args.morphFrac = 0;
    else if (args.morphFrac > 1.0f)
        args.morphFrac = 1.0f;
    return 1;
}

int zCProgMeshProto::CalcLODRenderArgs(const zTRenderContext& context, zTLODRenderArgs& args)
{
    return CalcPMLODArgs(this, CalcLODRenderVertPerc(context), args);
}

int zCProgMeshProto::CalcLODRenderArgsMin(const zTRenderContext&, zTLODRenderArgs& args)
{
    return CalcPMLODArgs(this, zEFFECT_VERT_PERC, args);
}

int zCProgMeshProto::CalcLODRenderArgsSubMesh(const zTRenderContext&,
    const zTLODRenderArgs& args, const zCSubMesh* subMesh, zTLODRenderArgsSubMesh& subArgs)
{
    if (subMeshList[0].vertexUpdates.numInArray > 1) {
        subArgs.numTri = subMesh->vertexUpdates.array[args.numPos].numNewTri;
        subArgs.numWedge = subMesh->vertexUpdates.array[args.numPos].numNewWedge;
        subArgs.numMinMorphWedge = subMesh->vertexUpdates.array[args.numMinMorphPos].numNewWedge;
        return 1;
    }
    if (subMesh->vertexUpdates.numInArray <= 0) {
        subArgs.numTri = 0;
        subArgs.numWedge = 0;
        return 0;
    }
    subArgs.numTri = subMesh->vertexUpdates.array[0].numNewTri;
    subArgs.numWedge = subMesh->vertexUpdates.array[0].numNewWedge;
    subArgs.numMinMorphWedge = subArgs.numWedge;
    return 1;
}

int zCProgMeshProto::s_subMeshRenderDir = 1;
unsigned short* zCProgMeshProto::s_wedgeRemap;
zCArray<unsigned short> zCProgMeshProto::s_workIndexList(1024);

int zCProgMeshProto::Render(zTRenderContext& context)
{
    if (context.m_AniMode) {
        float strength = context.m_aniModeStrength;
        if (strength > 5.0f)
            strength = 5.0f;
        else if (strength < 0.1f)
            strength = 0.1f;
        return RenderProgMesh(context, 0xb, 0, 0, 0, strength);
    }
    return RenderProgMesh(context, 0, 0, 0, 0, 1.0f);
}

int zCProgMeshProto::RenderProgMesh(zTRenderContext& context, int flags,
    zTLODRenderArgs* args, zCRenderLightContainer* lights, float aniStrength, float lodOverride)
{
    if (posList.numInArray >= 0x800)
        return 0;
    ++s_frameCtr;
    zTLODRenderArgs localArgs;
    if (!args) {
        args = &localArgs;
        CalcLODRenderArgs(context, localArgs);
    }
    if (args->numPos < 3)
        return 0;
    zCRenderLightContainer localLights;
    if (!lights) {
        localLights.Clear();
        lights = &localLights;
    }
    if (!(flags & 2)) {
        if (numSubMeshes > 0 && subMeshList[0].vertexUpdates.numInArray > 1)
            return RenderStaticLOD(context, flags, args, lights);
        return RenderStatic(context, flags, args, lights);
    }
    if (s_autoSubdivEnabled && args->vertPerc < 1.0f && numSubMeshes > 0 &&
        subMeshList[0].triEdgeList.numInArray > 0)
        return RenderDynamicSubdiv(context, flags, args, lights, aniStrength, lodOverride);
    return RenderDynamicLOD(context, flags, args, lights, aniStrength, lodOverride);
}

int zCProgMeshProto::RenderStatic(zTRenderContext&, int,
    zTLODRenderArgs*, zCRenderLightContainer*)
{
    if (!vertexBufferStatic)
        PackStaticVertexBuffers();
    s_subMeshRenderDir = -s_subMeshRenderDir;
    int numRenderedTriangles = 0;
    int subMeshIndex = s_subMeshRenderDir == 1 ? 0 : numSubMeshes - 1;
    for (int i = 0; i < numSubMeshes; ++i) {
        numRenderedTriangles += subMeshList[subMeshIndex].triList.numInArray;
        subMeshIndex += s_subMeshRenderDir;
    }
    return numRenderedTriangles > 0;
}

int zCProgMeshProto::RenderStaticLOD(zTRenderContext& context, int flags,
    zTLODRenderArgs* args, zCRenderLightContainer* lights)
{
    if (args->vertPerc >= 1.0f)
        return RenderStatic(context, flags, args, lights);
    if (!vertexBufferStatic)
        PackStaticVertexBuffers();
    if (!s_wedgeRemap)
        s_wedgeRemap = new unsigned short[0x1000];
    s_subMeshRenderDir = -s_subMeshRenderDir;
    int numRenderedTriangles = 0;
    int subMeshIndex = s_subMeshRenderDir == 1 ? 0 : numSubMeshes - 1;
    for (int i = 0; i < numSubMeshes; ++i) {
        zCSubMesh& subMesh = subMeshList[subMeshIndex];
        subMeshIndex += s_subMeshRenderDir;
        zTLODRenderArgsSubMesh subArgs;
        CalcLODRenderArgsSubMesh(context, *args, &subMesh, subArgs);
        if (subArgs.numTri <= 0)
            continue;
        for (int j = 0; j < subMesh.wedgeList.numInArray; ++j) {
            int mapped = j;
            while (mapped >= subArgs.numWedge)
                mapped = subMesh.wedgeMap.array[mapped];
            s_wedgeRemap[j] = mapped;
        }
        s_workIndexList.AllocAbs(subArgs.numTri * 3);
        s_workIndexList.numInArray = subArgs.numTri * 3;
        for (int j = 0; j < subArgs.numTri; ++j) {
            for (int k = 0; k < 3; ++k)
                s_workIndexList.parray[j * 3 + k] =
                    s_wedgeRemap[subMesh.triList.array[j].wedge[k]];
        }
        numRenderedTriangles += subArgs.numTri;
    }
    return numRenderedTriangles > 0;
}

zVEC3* zCProgMeshProto::s_posCacheList;
zVEC3* zCProgMeshProto::s_posNormalCSList;

void zCProgMeshProto::SoftwareTransformVerts(int flags, zTLODRenderArgs* args,
    float aniStrength, float lodOverride)
{
    if (!s_posCacheList)
        s_posCacheList = new zVEC3[0x800];
    if (!s_posNormalCSList)
        s_posNormalCSList = new zVEC3[0x800];
    for (int i = 0; i < args->numPos; ++i) {
        zVEC3 position = posList.parray[i];
        if (aniStrength != 0) {
            for (int axis = 0; axis < 3; ++axis)
                position.n[axis] += posNormalList.parray[i].n[axis] * aniStrength;
        }
        if (flags & 8)
            position.n[0] += std::sin(position.n[1] * 0.01f) * aniStrength;
        for (int axis = 0; axis < 3; ++axis)
            s_posCacheList[i].n[axis] = position.n[axis] * lodOverride;
    }
}

int zCProgMeshProto::RenderDynamicLOD(zTRenderContext& context, int flags,
    zTLODRenderArgs* args, zCRenderLightContainer*, float aniStrength, float lodOverride)
{
    if (flags & 1)
        SoftwareTransformVerts(flags, args, aniStrength, lodOverride);
    if (!s_wedgeRemap)
        s_wedgeRemap = new unsigned short[0x1000];
    s_subMeshRenderDir = -s_subMeshRenderDir;
    int numRenderedTriangles = 0;
    int subMeshIndex = s_subMeshRenderDir == 1 ? 0 : numSubMeshes - 1;
    for (int i = 0; i < numSubMeshes; ++i) {
        zCSubMesh& subMesh = subMeshList[subMeshIndex];
        subMeshIndex += s_subMeshRenderDir;
        zTLODRenderArgsSubMesh subArgs;
        CalcLODRenderArgsSubMesh(context, *args, &subMesh, subArgs);
        for (int j = 0; j < subMesh.wedgeList.numInArray; ++j) {
            int mapped = j;
            while (mapped >= subArgs.numWedge)
                mapped = subMesh.wedgeMap.array[mapped];
            s_wedgeRemap[j] = mapped;
        }
        s_workIndexList.AllocAbs(subArgs.numTri * 3);
        s_workIndexList.numInArray = subArgs.numTri * 3;
        for (int j = 0; j < subArgs.numTri; ++j) {
            for (int k = 0; k < 3; ++k)
                s_workIndexList.parray[j * 3 + k] =
                    s_wedgeRemap[subMesh.triList.array[j].wedge[k]];
        }
        numRenderedTriangles += subArgs.numTri;
    }
    return numRenderedTriangles > 0;
}

void zCProgMeshProto::CalcSubdiv(int, const zTLODRenderArgs* args)
{
    if (!s_posNormalCSList)
        s_posNormalCSList = new zVEC3[0x800];
    for (int i = 0; i < args->numPos; ++i)
        s_posNormalCSList[i] = zCCamera::activeCam->camMatrix.Rotate(posNormalList.parray[i]);
}

void zCProgMeshProto::CalcNumSubdivEdges(zCSubMesh* subMesh,
    const zTLODRenderArgs* args, zTLODRenderArgsSubMesh& subArgs)
{
    float percent = args->vertPerc * 0.25f;
    if (percent < 0)
        percent = 0;
    else if (percent > 1.0f)
        percent = 1.0f;
    subArgs.numSubdivEdges = static_cast<int>(subMesh->edgeList.numInArray * percent);
}

void zCProgMeshProto::CalcSubdivSubMesh(zCSubMesh* subMesh, zCVertexBuffer* vertexBuffer,
    const zTLODRenderArgs*, zCRenderLightContainer* lights, zTLODRenderArgsSubMesh& subArgs)
{
    int vertexIndex = subArgs.numWedge;
    for (int i = 0; i < subArgs.numSubdivEdges; ++i) {
        const zTPMEdge& edge = subMesh->edgeList.array[i];
        const zTPMWedge& firstWedge = subMesh->wedgeList.array[edge.wedge[0]];
        const zTPMWedge& secondWedge = subMesh->wedgeList.array[edge.wedge[1]];
        zVEC3 normal;
        for (int axis = 0; axis < 3; ++axis) {
            normal.n[axis] = (s_posNormalCSList[firstWedge.position].n[axis] +
                s_posNormalCSList[secondWedge.position].n[axis]) * 0.5f;
            vertexBuffer->positionList[vertexIndex].n[axis] =
                (s_posCacheList[firstWedge.position].n[axis] +
                s_posCacheList[secondWedge.position].n[axis]) * 0.5f +
                normal.n[axis] * subMesh->edgeScoreList.array[i];
        }
        vertexBuffer->normalList[vertexIndex] = lights->trafoObjToCam.Rotate(normal);
        for (int axis = 0; axis < 2; ++axis)
            vertexBuffer->texCoordList[vertexIndex].n[axis] =
                (firstWedge.texUV.n[axis] + secondWedge.texUV.n[axis]) * 0.5f;
        ++vertexIndex;
    }
}

int zCProgMeshProto::RenderDynamicSubdiv(zTRenderContext& context, int flags,
    zTLODRenderArgs* args, zCRenderLightContainer* lights, float aniStrength, float lodOverride)
{
    SoftwareTransformVerts(flags, args, aniStrength, lodOverride);
    CalcSubdiv(flags, args);
    if (!vertexBufferStatic)
        PackStaticVertexBuffers();
    int numRenderedTriangles = 0;
    for (int i = 0; i < numSubMeshes; ++i) {
        zTLODRenderArgsSubMesh subArgs;
        CalcLODRenderArgsSubMesh(context, *args, &subMeshList[i], subArgs);
        CalcNumSubdivEdges(&subMeshList[i], args, subArgs);
        CalcSubdivSubMesh(&subMeshList[i], vertexBufferStatic, args, lights, subArgs);
        numRenderedTriangles += subArgs.numTri;
    }
    return numRenderedTriangles > 0;
}

int zCProgMeshProto::RenderStaticLODShadow(zTRenderContext& context, int flags,
    zTLODRenderArgs* args, zCRenderLightContainer* lights)
{
    return RenderStaticLOD(context, flags, args, lights);
}

void zCVertexBuffer::SetXYZ(unsigned long index, const zVEC3& position)
{
    positionList[index] = position;
}

unsigned short zCProgMeshProto::GetFileVersion()
{
    return (zCMaterial::classDef.archiveVersion << 8) + 5;
}

template <class T>
static void WritePMAdapter(zCFileBIN& file, const zCArrayAdapt<T>& adapter,
    const unsigned char* dataPool)
{
    const int offset = reinterpret_cast<const unsigned char*>(adapter.array) - dataPool;
    file.file->Write(&offset, 4);
    file.file->Write(&adapter.numInArray, 4);
}

template <class T>
static void ReadPMAdapter(zCFileBIN& file, zCArrayAdapt<T>& adapter, unsigned char* dataPool)
{
    int offset;
    file.file->Read(&offset, 4);
    file.file->Read(&adapter.numInArray, 4);
    adapter.array = reinterpret_cast<T*>(dataPool + offset);
}

int zCProgMeshProto::Save(zCFileBIN& file)
{
    file.BinStartChunk(0xb100);
    const unsigned short version = GetFileVersion();
    file.file->Write(&version, 2);
    file.file->Write(&dataSize, 4);
    file.file->Write(dataPool, dataSize);
    unsigned char count = numSubMeshes;
    file.file->Write(&count, 1);
    file.file->Write(&posList.numInArray, 4);
    int offset = reinterpret_cast<unsigned char*>(posList.parray) - dataPool;
    file.file->Write(&offset, 4);
    file.file->Write(&posNormalList.numInArray, 4);
    offset = reinterpret_cast<unsigned char*>(posNormalList.parray) - dataPool;
    file.file->Write(&offset, 4);
    for (int i = 0; i < numSubMeshes; ++i) {
        zCSubMesh& subMesh = subMeshList[i];
        WritePMAdapter(file, subMesh.triList, dataPool);
        WritePMAdapter(file, subMesh.wedgeList, dataPool);
        WritePMAdapter(file, subMesh.colorList, dataPool);
        WritePMAdapter(file, subMesh.triPlaneIndexList, dataPool);
        WritePMAdapter(file, subMesh.triPlaneList, dataPool);
        WritePMAdapter(file, subMesh.wedgeMap, dataPool);
        WritePMAdapter(file, subMesh.vertexUpdates, dataPool);
        WritePMAdapter(file, subMesh.triEdgeList, dataPool);
        WritePMAdapter(file, subMesh.edgeList, dataPool);
        WritePMAdapter(file, subMesh.edgeScoreList, dataPool);
    }
    file.BinEndChunk();
    return 1;
}

int zCProgMeshProto::Load(zCFileBIN& file)
{
    unsigned short version;
    file.file->Read(&version, 2);
    if (version != GetFileVersion())
        return 0;
    delete[] subMeshList;
    delete[] dataPool;
    file.file->Read(&dataSize, 4);
    dataPool = new unsigned char[dataSize];
    file.file->Read(dataPool, dataSize);
    unsigned char count;
    file.file->Read(&count, 1);
    numSubMeshes = count;
    subMeshList = new zCSubMesh[numSubMeshes];
    int numPositions;
    int positionOffset;
    int numNormals;
    int normalOffset;
    file.file->Read(&numPositions, 4);
    file.file->Read(&positionOffset, 4);
    file.file->Read(&numNormals, 4);
    file.file->Read(&normalOffset, 4);
    posList.DeleteList();
    posList.AllocAbs(numPositions);
    posList.numInArray = numPositions;
    std::memcpy(posList.parray, dataPool + positionOffset, numPositions * sizeof(zVEC3));
    posNormalList.DeleteList();
    posNormalList.AllocAbs(numNormals);
    posNormalList.numInArray = numNormals;
    std::memcpy(posNormalList.parray, dataPool + normalOffset, numNormals * sizeof(zVEC3));
    for (int i = 0; i < numSubMeshes; ++i) {
        zCSubMesh& subMesh = subMeshList[i];
        ReadPMAdapter(file, subMesh.triList, dataPool);
        ReadPMAdapter(file, subMesh.wedgeList, dataPool);
        ReadPMAdapter(file, subMesh.colorList, dataPool);
        ReadPMAdapter(file, subMesh.triPlaneIndexList, dataPool);
        ReadPMAdapter(file, subMesh.triPlaneList, dataPool);
        ReadPMAdapter(file, subMesh.wedgeMap, dataPool);
        ReadPMAdapter(file, subMesh.vertexUpdates, dataPool);
        ReadPMAdapter(file, subMesh.triEdgeList, dataPool);
        ReadPMAdapter(file, subMesh.edgeList, dataPool);
        ReadPMAdapter(file, subMesh.edgeScoreList, dataPool);
    }
    CalcRenderSortKey();
    return 1;
}

zCMeshSoftSkin::zCMeshSoftSkin()
    : vertWeightStream(0), iterator(0)
{
}

zCMeshSoftSkin::~zCMeshSoftSkin()
{
    DeleteVertWeightStream();
    DeleteNodeOBBList();
}

void zCMeshSoftSkin::AllocVertWeightStream(int numVertices, int numWeights)
{
    DeleteVertWeightStream();
    vertWeightStream = new unsigned char[numVertices * 4 + numWeights * 17];
    iterator = vertWeightStream;
}

void zCMeshSoftSkin::DeleteVertWeightStream()
{
    delete[] vertWeightStream;
    vertWeightStream = 0;
    iterator = 0;
}

void zCMeshSoftSkin::AddNumWeightEntry(const int& count)
{
    std::memcpy(iterator, &count, 4);
    iterator += 4;
}

void zCMeshSoftSkin::AddWeightEntry(const zTWeightEntry& entry)
{
    std::memcpy(iterator, &entry.weight, 4);
    std::memcpy(iterator + 4, entry.vertPosOS.n, 12);
    iterator[16] = entry.nodeIndex;
    iterator += 17;
}

void zCMeshSoftSkin::ResetVertWeightIterator()
{
    iterator = vertWeightStream;
}

int zCMeshSoftSkin::GetVertWeightStreamSize()
{
    ResetVertWeightIterator();
    int size = 0;
    for (int i = 0; i < posList.numInArray; ++i) {
        int numWeights;
        std::memcpy(&numWeights, iterator, 4);
        iterator += 4 + numWeights * 17;
        size += 4 + numWeights * 17;
    }
    return size;
}

void zCMeshSoftSkin::BuildNodeIndexList()
{
    ResetVertWeightIterator();
    nodeIndexList.numInArray = 0;
    for (int i = 0; i < posList.numInArray; ++i) {
        int numWeights;
        std::memcpy(&numWeights, iterator, 4);
        iterator += 4;
        for (int j = 0; j < numWeights; ++j) {
            const int nodeIndex = iterator[16];
            if (!nodeIndexList.IsInList(nodeIndex))
                nodeIndexList.InsertEnd(nodeIndex);
            iterator += 17;
        }
    }
}

void zCMeshSoftSkin::DeleteNodeOBBList()
{
    for (int i = 0; i < nodeOBBList.numInArray; ++i)
        delete nodeOBBList.parray[i];
    nodeOBBList.DeleteList();
}

void zCMeshSoftSkin::BuildNodeOBBList()
{
    DeleteNodeOBBList();
    for (int node = 0; node < nodeIndexList.numInArray; ++node) {
        zVEC3 mins;
        zVEC3 maxs;
        for (int axis = 0; axis < 3; ++axis) {
            mins.n[axis] = 3.4e38f;
            maxs.n[axis] = -3.4e38f;
        }
        ResetVertWeightIterator();
        for (int i = 0; i < posList.numInArray; ++i) {
            int numWeights;
            std::memcpy(&numWeights, iterator, 4);
            iterator += 4;
            for (int j = 0; j < numWeights; ++j) {
                if (iterator[16] == nodeIndexList.parray[node]) {
                    zVEC3 position;
                    std::memcpy(position.n, iterator + 4, 12);
                    for (int axis = 0; axis < 3; ++axis) {
                        if (position.n[axis] < mins.n[axis])
                            mins.n[axis] = position.n[axis];
                        if (position.n[axis] > maxs.n[axis])
                            maxs.n[axis] = position.n[axis];
                    }
                }
                iterator += 17;
            }
        }
        zCOBBox3D* obbox = new zCOBBox3D;
        for (int axis = 0; axis < 3; ++axis) {
            obbox->center.n[axis] = (mins.n[axis] + maxs.n[axis]) * 0.5f;
            obbox->extent.n[axis] = (maxs.n[axis] - mins.n[axis]) * 0.5f;
        }
        nodeOBBList.InsertEnd(obbox);
    }
}

unsigned short zCMeshSoftSkin::GetFileVersion()
{
    return 4;
}

int zCMeshSoftSkin::Save(zCFileBIN& file)
{
    file.BinStartChunk(0xe100);
    const int version = GetFileVersion();
    file.file->Write(&version, 4);
    zCProgMeshProto::Save(file);
    const int streamSize = GetVertWeightStreamSize();
    file.file->Write(&streamSize, 4);
    file.file->Write(vertWeightStream, streamSize);
    file.file->Write(&nodeWedgeNormalList.numInArray, 4);
    file.file->Write(nodeWedgeNormalList.parray,
        nodeWedgeNormalList.numInArray * sizeof(zTNodeWedgeNormal));
    const unsigned short numNodes = nodeIndexList.numInArray;
    file.file->Write(&numNodes, 2);
    file.file->Write(nodeIndexList.parray, nodeIndexList.numInArray * sizeof(int));
    for (int i = 0; i < nodeOBBList.numInArray; ++i) {
        file.file->Write(nodeOBBList.parray[i]->center.n, 12);
        file.file->Write(nodeOBBList.parray[i]->axis, 36);
        file.file->Write(nodeOBBList.parray[i]->extent.n, 12);
    }
    file.BinEndChunk();
    file.BinStartChunk(0xe110);
    file.BinEndChunk();
    return 1;
}

int zCMeshSoftSkin::Load(zCFileBIN& file)
{
    int version;
    file.file->Read(&version, 4);
    if (version != GetFileVersion())
        return 0;
    unsigned short chunkId;
    int chunkSize;
    file.file->Read(&chunkId, 2);
    file.file->Read(&chunkSize, 4);
    if (chunkId != 0xb100 || !zCProgMeshProto::Load(file))
        return 0;
    int streamSize;
    file.file->Read(&streamSize, 4);
    delete[] vertWeightStream;
    vertWeightStream = new unsigned char[streamSize];
    iterator = vertWeightStream;
    file.file->Read(vertWeightStream, streamSize);
    int numNormals;
    file.file->Read(&numNormals, 4);
    nodeWedgeNormalList.AllocAbs(numNormals);
    nodeWedgeNormalList.numInArray = numNormals;
    file.file->Read(nodeWedgeNormalList.parray, numNormals * sizeof(zTNodeWedgeNormal));
    unsigned short numNodes;
    file.file->Read(&numNodes, 2);
    nodeIndexList.AllocAbs(numNodes);
    nodeIndexList.numInArray = numNodes;
    file.file->Read(nodeIndexList.parray, numNodes * sizeof(int));
    DeleteNodeOBBList();
    nodeOBBList.AllocAbs(numNodes);
    for (int i = 0; i < numNodes; ++i) {
        zCOBBox3D* obbox = new zCOBBox3D;
        file.file->Read(obbox->center.n, 12);
        file.file->Read(obbox->axis, 36);
        file.file->Read(obbox->extent.n, 12);
        nodeOBBList.InsertEnd(obbox);
    }
    return 1;
}

static void SkinSoftSkinVertices(zCMeshSoftSkin* skin, const zCArray<zMAT4*>& trafos,
    float fatness, int numPositions)
{
    skin->ResetVertWeightIterator();
    if (!zCProgMeshProto::s_posCacheList)
        zCProgMeshProto::s_posCacheList = new zVEC3[0x800];
    for (int i = 0; i < numPositions; ++i) {
        int numWeights;
        std::memcpy(&numWeights, skin->iterator, 4);
        skin->iterator += 4;
        zVEC3 position = { 0, 0, 0 };
        for (int j = 0; j < numWeights; ++j) {
            float weight;
            zVEC3 localPosition;
            std::memcpy(&weight, skin->iterator, 4);
            std::memcpy(localPosition.n, skin->iterator + 4, 12);
            for (int axis = 0; axis < 3; ++axis)
                localPosition.n[axis] += skin->posNormalList.parray[i].n[axis] * fatness;
            const zMAT4& trafo = *trafos.parray[skin->iterator[16]];
            for (int row = 0; row < 3; ++row)
                position.n[row] += weight * (trafo.m[row][0] * localPosition.n[0] +
                    trafo.m[row][1] * localPosition.n[1] +
                    trafo.m[row][2] * localPosition.n[2] + trafo.m[row][3]);
            skin->iterator += 17;
        }
        zCProgMeshProto::s_posCacheList[i] = position;
    }
}

int zCMeshSoftSkin::RenderSoftSkin(zTRenderContext& context,
    const zCArray<zMAT4*>& trafos, zCRenderLightContainer* lights, float fatness)
{
    zTLODRenderArgs args;
    if (!CalcLODRenderArgs(context, args))
        return 0;
    SkinSoftSkinVertices(this, trafos, fatness, args.numPos);
    return RenderDynamicLOD(context, 0, &args, lights, 0, 1.0f);
}

zVEC3* zCProgMeshProto::s_posCacheListLOD;
int* zCProgMeshProto::s_posCacheListLODIndices;
int zCProgMeshProto::s_posCacheCount;
int zCProgMeshProto::s_posCacheCountLOD;
int zCProgMeshProto::s_posCacheIndex;
int zCMeshSoftSkin::s_lowestLODNumPolys;

void zCProgMeshProto::InitSetupLowestLOD()
{
    s_posCacheIndex = 0;
    s_posCacheCount = 0;
    s_posCacheCountLOD = 0;
    zCMeshSoftSkin::s_lowestLODNumPolys = 0;
    if (!s_posCacheListLOD)
        s_posCacheListLOD = new zVEC3[0x2000];
    if (!s_posCacheListLODIndices)
        s_posCacheListLODIndices = new int[0x6000];
}

void zCMeshSoftSkin::SetupLowestLOD(zTRenderContext& context,
    const zCArray<zMAT4*>& trafos, float fatness, int& numPoly)
{
    zTLODRenderArgs args;
    if (!CalcLODRenderArgsMin(context, args) || args.numPos < 3) {
        numPoly = 0;
        return;
    }
    SkinSoftSkinVertices(this, trafos, fatness, args.numPos);
    const int positionBase = s_posCacheCountLOD;
    for (int i = 0; i < args.numPos; ++i)
        s_posCacheListLOD[s_posCacheCountLOD++] = s_posCacheList[i];
    for (int i = 0; i < numSubMeshes; ++i) {
        zCSubMesh& subMesh = subMeshList[i];
        zTLODRenderArgsSubMesh subArgs;
        CalcLODRenderArgsSubMesh(context, args, &subMesh, subArgs);
        for (int j = 0; j < subArgs.numTri; ++j) {
            for (int k = 0; k < 3; ++k) {
                const int wedge = subMesh.triList.array[j].wedge[k];
                s_posCacheListLODIndices[s_lowestLODNumPolys * 3 + k] =
                    positionBase + subMesh.wedgeList.array[wedge].position;
            }
            ++s_lowestLODNumPolys;
        }
    }
    numPoly = s_lowestLODNumPolys;
}

int zCMeshSoftSkin::GetLowestLODNumPolys()
{
    return s_lowestLODNumPolys;
}

zVEC3* zCMeshSoftSkin::GetLowestLODPoly(int index, zVEC3*& normal)
{
    static zVEC3 vertices[3];
    static zVEC3 defaultNormal = { 0, 1.0f, 0 };
    normal = &defaultNormal;
    for (int i = 0; i < 3; ++i)
        vertices[i] = s_posCacheListLOD[s_posCacheListLODIndices[index * 3 + i]];
    return vertices;
}

zCProgMeshConvertFileHandler::zCProgMeshConvertFileHandler()
{
    RegisterFileType("3ds");
}

int zCProgMeshConvertFileHandler::HandleFile(const zSTRING& path, const char* fileName,
    const zFINDDATA&)
{
    zSTRING fullName = path;
    fullName.Append("\\");
    fullName.Append(fileName);
    fullName.Upper();
    if (fullName.Search(".3DS") < 0)
        return 0;
    if (fullName.Search("\\LEVEL\\") >= 0 || fullName.Search("\\_DIETER\\") >= 0)
        return 0;
    zCVisual* visual = zCVisual::LoadVisual(fullName);
    if (!visual)
        return 0;
    visual->Release();
    return 1;
}

zVEC3 zLerp(float fraction, const zVEC3& first, const zVEC3& second)
{
    zVEC3 result;
    for (int i = 0; i < 3; ++i)
        result.n[i] = first.n[i] + (second.n[i] - first.n[i]) * fraction;
    return result;
}

zCVisual* zCProgMeshProto::Load(const zSTRING& fileName)
{
    zCProgMeshProto* proto = new zCProgMeshProto;
    proto->SetObjectName(fileName.Upper());
    zCMesh* mesh = zCMesh::Load(fileName, 0);
    if (!mesh) {
        proto->Release();
        return NULL;
    }

    zCProgMeshBuilder builder;
    if (!builder.BuildProgMeshProto(mesh, proto, NULL, 0)) {
        mesh->Release();
        proto->Release();
        return NULL;
    }
    proto->PackStaticVertexBuffers();
    mesh->Release();
    return proto;
}
