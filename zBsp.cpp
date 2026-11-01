#include "include/zBsp.h"
#include "include/zAlphaSort.h"
#include "include/zRenderManager.h"
#include "include/zRenderer.h"
#include "include/zSky.h"
#include "include/zMath.h"
#include "include/zAlgebra.h"
#include "include/zTools.h"
#include "include/zViewBase.h"
#include "include/zCollisionDetector.h"
#include <cstdlib>
#include <cstring>
#include <cmath>

static int rayCacheState = -1;
static int cameraCollisionState = -1;
static int collisionDetectorState = -1;
static int cameraState = -1;
static int collisionReportState = -1;
static int collisionObjectState = -1;
static int collisionObjectDefState = -1;
static int collisionGeometryState = -1;
static int convexPrimitiveState = -1;
static int convexCacheState = -1;
zVEC3 zCCamera::activeCamPos;
zCMemPool<zCConvexCollisionCache> zCConvexCollisionCache::s_MemPool(0x80, 0x80);
static zCCache<zCCacheIndex, zCConvexCollisionCache> convexCollisionCache(13, 2);

extern zCRenderManager zrenderMan;
extern int bspFrameCtr;
extern int playerLightInt;

// EXE: 0x008D4960
zCArray<zTBBox2D> zCBspSector::s_activeOutdoorPortals;
// EXE: 0x008D7EB0
int zCBspTree::s_renderAllPortals;

static int   bspHorizonCells;
static float bspHorizonY;
static float bspHorizonLeft[4096];
static float bspHorizonRight[4096];

void zCBspBase::ReleaseStaticReferences()
{
    zCRndAlphaSortObject_Vob::ClearList();
}

int zCBspSector::CompareOutdoorPortalBBox2D(const void* first, const void* second)
{
    const int firstIndex = *static_cast<const int*>(first);
    const int secondIndex = *static_cast<const int*>(second);
    return int(s_activeOutdoorPortals.parray[firstIndex].maxs.n[1])
        - int(s_activeOutdoorPortals.parray[secondIndex].maxs.n[1]);
}

void zCBspSector::InitHorizonByOutdoorPortals()
{
    const int count = s_activeOutdoorPortals.numInArray;
    int* indices = count > 0 ? new int[count] : NULL;
    for (int i = 0; i < count; ++i)
        indices[i] = i;
    qsort(indices, count, sizeof(int), CompareOutdoorPortalBBox2D);

    bspHorizonCells = zCCamera::activeCam->vpData.xdim / 4;
    const int cells = zCCamera::activeCam->vpData.xdim / 4;
    std::memset(bspHorizonLeft, 0, (cells + 1) * sizeof(float));
    std::memset(bspHorizonRight, 0, (cells + 1) * sizeof(float));
    if (bspHorizonY < 0.0f)
        bspHorizonY = 0.0f;

    for (int i = 0; i < count; ++i) {
        const zTBBox2D& portal = s_activeOutdoorPortals.parray[indices[i]];
        const int left = int(portal.mins.n[0]) / 4;
        const int right = int(portal.maxs.n[0]) / 4;
        for (int cell = left; cell <= right; ++cell) {
            bspHorizonLeft[cell] = portal.maxs.n[1];
            bspHorizonRight[cell] = portal.maxs.n[1];
        }
        if (bspHorizonY < portal.maxs.n[1])
            bspHorizonY = portal.maxs.n[1];
    }
    delete[] indices;
}

void zCBspSector::CalcCenterByPortals()
{
    sectorCenter = zVEC3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < sectorPortals.numInArray; ++i)
        sectorCenter += sectorPortals.parray[i]->GetCenter();
    const float scale = 1.0f / sectorPortals.numInArray;
    sectorCenter.n[0] *= scale;
    sectorCenter.n[1] *= scale;
    sectorCenter.n[2] *= scale;
}

int zCBspSector::HasManyBigPortals() const
{
    int bigPortals = 0;
    for (int i = 0; i < sectorPortals.numInArray; ++i) {
        if ((sectorPortals.parray[i]->flags[0] & 3) >= 2) {
            ++bigPortals;
            if (bigPortals >= 7)
                return 1;
        }
    }
    return 0;
}

void zCPortal::RemoveFromBsp()
{
    if (frontBspNode)
        frontBspNode->RemovePoly(this);
    if (backBspNode)
        backBspNode->RemovePoly(this);
}

zCMaterial* zCPortal::portalDummyMaterial;

zCPortal::zCPortal(const zTPlane& plane, zCMesh* mesh)
{
    flags[0] = (flags[0] & 0xFC) | 1;
    frontBspNode = NULL;
    backBspNode = NULL;
    lastTimeCompletely = -1;

    if (!portalDummyMaterial) {
        portalDummyMaterial = new zCMaterial;
        portalDummyMaterial->objectName = "Z_PORTALDUMMYMAT";
    }
    if (material != portalDummyMaterial) {
        if (material)
            material->Release();
        portalDummyMaterial->AddRef();
        material = portalDummyMaterial;
    }

    int axis = 0;
    for (int i = 1; i < 3; ++i) {
        const float value = plane.normal.n[i] < 0.0f ? -plane.normal.n[i] : plane.normal.n[i];
        const float best = plane.normal.n[axis] < 0.0f
            ? -plane.normal.n[axis] : plane.normal.n[axis];
        if (value < best)
            axis = i;
    }

    zVEC3 reference(0.0f, 0.0f, 0.0f);
    reference.n[axis] = 1.0f;
    zVEC3 tangent = (plane.normal ^ reference).Normalize();
    const zVEC3 bitangent = plane.normal ^ tangent;
    const zVEC3 center = plane.normal * -plane.distance;

    zCVertex* vertices[4];
    vertices[0] = new zCVertex;
    vertices[1] = new zCVertex;
    vertices[2] = new zCVertex;
    vertices[3] = new zCVertex;
    vertices[0]->position = center + (tangent + bitangent) * 10000.0f;
    vertices[1]->position = center + (tangent - bitangent) * 10000.0f;
    vertices[2]->position = center + (tangent * -1.0f - bitangent) * 10000.0f;
    vertices[3]->position = center + (tangent * -1.0f + bitangent) * 10000.0f;

    AllocVerts(4);
    for (int i = 0; i < 4; ++i)
        SetVertex(i, vertices[i]);
    polyPlane = plane;
}

void zCBspBase::DescribeTree(int depth)
{
    zSTRING text = "D: BSP: " + Spaces(depth * 2);
    text = text + (nodeType == 1 ? "- Leaf" : "- Node");

    if (nodeType != 1) {
        text = text + ", numLeafs: " + zSTRING(numLeafs);
    }
    else {
        text = text + ", leafVobs: " + zSTRING(leafVobs)
            + ", leafLights: " + zSTRING(leafLights);
    }
    zerr->Message(text);

    if (nodeType != 1) {
        if (front)
            front->DescribeTree(depth + 1);
        if (back)
            back->DescribeTree(depth + 1);
    }
}

static zCVertexTransform* ProjectPolygonVertex(zCVertex* vertex)
{
    zCVertexTransform* transform = vertex->transformedIndex
        ? &zCVertex::transformedVertexList[vertex->transformedIndex]
        : vertex->CreateVertexTransform();

    if (!vertex->transformedIndex) {
        const zMAT4& matrix = zCCamera::activeCam->camMatrix;
        const zVEC3& position = vertex->position;
        for (int row = 0; row < 3; ++row) {
            transform->vertCamSpace.n[row] = matrix.m[row][0] * position.n[0]
                + matrix.m[row][1] * position.n[1] + matrix.m[row][2] * position.n[2]
                + matrix.m[row][3];
        }

        transform->vertCamSpaceZInv = 1.0f / transform->vertCamSpace.n[2];
        transform->vertScrX = zCCamera::activeCam->viewDistanceX
            * transform->vertCamSpace.n[0] * transform->vertCamSpaceZInv
            + zCCamera::activeCam->vpData.xcenter;
        transform->vertScrY = zCCamera::activeCam->vpData.ycenter
            - zCCamera::activeCam->viewDistanceY * transform->vertCamSpace.n[1]
            * transform->vertCamSpaceZInv;
    }
    return transform;
}

int zCPolygon::RenderPoly(int flags)
{
    if (lastTimeDrawn == bspFrameCtr)
        return 0;

    if (!(this->flags[0] & 3)) {
        const float distance = zCCamera::activeCamPos.n[0] * polyPlane.normal.n[0]
            + zCCamera::activeCamPos.n[1] * polyPlane.normal.n[1]
            + zCCamera::activeCamPos.n[2] * polyPlane.normal.n[2];
        if (distance < polyPlane.distance) {
            lastTimeDrawn = bspFrameCtr;
            this->flags[0] |= 0x10;
            return 0;
        }
    }

    ApplyMorphing();
    LightClippedPoly(0);
    lastTimeDrawn = bspFrameCtr;

    if (flags > 0) {
        if (this->flags[0] & 4) {
            if (!ClipToFrustum_Occluder(flags))
                return 0;
        }
        else if (!ClipToFrustum(flags)) {
            return 0;
        }
    }
    else {
        Unclip();
    }

    int outside = 0;
    for (int i = 0; i < numClipVert; ++i) {
        zCVertexTransform* transform = ProjectPolygonVertex(clipVert[i]);
        if (transform->vertCamSpace.n[2] > zCCamera::activeCam->farClipZ)
            ++outside;
    }
    if (outside == numClipVert)
        return 0;

    if (material->polyListTimeStamp != zrenderMan.frameCtr) {
        material->polyListTimeStamp = zrenderMan.frameCtr;
        zrenderMan.sceneMaterialList.InsertEnd(material);
    }
    material->polyList.InsertEnd(this);
    return 1;
}

void zCPolygon::ApplyMorphing()
{
    if (!zCWorld::s_bWaveAnisEnabled || !material->m_enuWaveMode)
        return;

    for (int i = 0; i < polyNumVert; ++i) {
        zCVertex* current = vertex[i];
        float wave = zSinApprox((current->position.n[0] + current->position.n[2])
            * material->m_fWaveGridSize);
        if (wave > 1.0f)
            wave = 1.0f;
        else if (wave < -1.0f)
            wave = -1.0f;
        const float offset = wave * material->m_fWaveMaxAmplitude;

        if (material->m_enuWaveMode < 3) {
            current->position.n[1] += offset;
        }
        else {
            current->position.n[0] += offset;
            current->position.n[2] += offset;
        }
    }
}

void zCPolygon::LightClippedPoly(int force)
{
    flags[0] &= 0xEF;
    zCSkyControler* sky = zCSkyControler::s_activeSkyControler;

    for (int i = 0; i < polyNumVert; ++i) {
        zCVertFeature* current = feature[i];
        if (sky->polyLightCLUTPtr && sky->cloudShadowScale == 0.0f) {
            const unsigned long index = current->lightStat[0]
                | (current->lightStat[1] << 8) | (current->lightStat[2] << 16)
                | (current->lightStat[3] << 24);
            const zCOLOR color = sky->polyLightCLUTPtr[index];
            current->lightDyn[0] = color.b;
            current->lightDyn[1] = color.g;
            current->lightDyn[2] = color.r;
            current->lightDyn[3] = color.a;
        }
        else {
            for (int channel = 0; channel < 4; ++channel)
                current->lightDyn[channel] = current->lightStat[channel];
        }
    }

    if (playerLightInt <= 50 || !zCCamera::activeCam->connectedVob)
        return;

    const zMAT4& trafo = zCCamera::activeCam->connectedVob->trafoObjToWorld;
    const float range2 = float(playerLightInt * playerLightInt);
    for (int i = 0; i < polyNumVert; ++i) {
        const zVEC3& position = vertex[i]->position;
        const float x = position.n[0] - trafo.m[0][3];
        const float y = position.n[1] - trafo.m[1][3];
        const float z = position.n[2] - trafo.m[2][3];
        const float distance2 = x * x + y * y + z * z;
        if (distance2 >= range2)
            continue;

        int intensity = int((1.0f - distance2 / range2) * 255.0f);
        for (int channel = 0; channel < 3; ++channel) {
            const int value = feature[i]->lightDyn[channel] + intensity;
            feature[i]->lightDyn[channel] = value > 255 ? 255 : value;
        }
    }
}

int zCPolygon::TransformProjectLight()
{
    int outside = 0;
    for (int i = 0; i < numClipVert; ++i) {
        zCVertexTransform* transform = ProjectPolygonVertex(clipVert[i]);
        if (transform->vertCamSpace.n[2] > zCCamera::activeCam->farClipZ)
            ++outside;
    }
    if (outside == numClipVert)
        return 0;

    LightDynCamSpace(zCCamera::activeCamPos, playerLightInt);
    return 1;
}

int zCPolygon::TransformProject()
{
    int outside = 0;
    for (int i = 0; i < numClipVert; ++i) {
        zCVertexTransform* transform = ProjectPolygonVertex(clipVert[i]);
        if (transform->vertCamSpace.n[2] > zCCamera::activeCam->farClipZ)
            ++outside;
    }
    return outside != numClipVert;
}

zCBspTree::zCBspTree()
{
    InitTree();
}

zCBspTree::~zCBspTree()
{
    DisposeTree();
    delete[] leafList;
    leafList = NULL;
    bspRoot = NULL;
}

void zCBspTree::InitTree()
{
    numNodes = 0;
    numLeafs = 0;
    numPolys = 0;
    nodeList = NULL;
    leafList = new zCBspLeaf[1];
    treePolyList = NULL;
    mesh = NULL;
    masterFrameCtr = 0;
    actPolyPtr = NULL;
    actNodePtr = NULL;
    actLeafPtr = NULL;
    bspRoot = leafList;
    leafList->bbox3D.SetMaximumBox();
    compiled = 0;
    drawVobBBox3D = 0;
    vobFarClipZ = 0.0f;
    vobFarPlaneSignbits = 0;
    leafsRendered = 0;
    vobsRendered = 0;
    bspTreeMode = 0;
    renderVobList.numInArray = 0;
    renderLightList.numInArray = 0;
    worldRenderMode = 0;
    m_bRenderedFirstTime = 1;
}

zCBspLeaf::zCBspLeaf()
{
    parent = NULL;
    numPolys = 0;
    polyList = NULL;
    lastTimeLighted = -1;
    lastTimeActivated = -1;
    lightPosition = zVEC3(-99.0f, -99.0f, -99.0f);
    nodeType = 1;
    sectorIndex = 0;
    polyPlueckerRefCtr = 0;
    polyPluecker = NULL;
}

void zCBspTree::DisposeTree()
{
    portalList.numInArray = 0;
    if (mesh) {
        mesh->Release();
        mesh = NULL;
    }

    delete[] nodeList;
    nodeList = NULL;
    delete[] leafList;
    leafList = NULL;
    delete[] treePolyList;
    treePolyList = NULL;
    bspRoot = NULL;

    for (int i = 0; i < sectorList.numInArray; ++i)
        delete sectorList.parray[i];
    sectorList.numInArray = 0;
    zCRndAlphaSortObject_Vob::ClearList();
    InitTree();
}

zCBspSector::~zCBspSector()
{
}

int zCCBspNode::CountNodes() const
{
    if (!front && !back)
        return 1;

    int count = 0;
    if (front)
        count += front->CountNodes();
    if (back)
        count += back->CountNodes();
    return count;
}

int zCCBspNode::CountLeafs() const
{
    return CountNodes();
}

int zCCBspNode::CountPolys() const
{
    int count = polyList.count;
    if (front)
        count += front->CountPolys();
    if (back)
        count += back->CountPolys();
    return count;
}

zCBspBase* zCBspTree::BuildRecursive(zCCBspNode* source)
{
    zCBspNode* result = actNodePtr;
    zCBspNode* parent = result != nodeList ? result - 1 : NULL;

    if (source->front || source->back) {
        result->plane = source->plane;
        result->bbox3D = source->bbox3D;
        result->parent = parent;
        result->planeSignbits = 0;
        if (result->plane.normal.n[0] == 0.0f)
            result->planeSignbits = result->plane.normal.n[1] == 0.0f ? 2 : 1;

        ++actNodePtr;
        zCBspLeaf* firstLeaf = actLeafPtr;
        if (source->front)
            result->front = BuildRecursive(source->front);
        if (source->back)
            result->back = BuildRecursive(source->back);

        result->leafList = firstLeaf;
        result->numLeafs = actLeafPtr - firstLeaf;

        if (bspTreeMode != 1) {
            result->numPolys = 0;
            if (result->front) {
                result->polyList = result->front->polyList;
                result->numPolys += result->front->numPolys;
            }
            if (result->back) {
                if (!result->front)
                    result->polyList = result->back->polyList;
                result->numPolys += result->back->numPolys;
            }
        }
        else {
            result->numPolys = source->polyList.count;
            result->polyList = source->polyList.count > 0 ? actPolyPtr : NULL;
            for (zTPolyNode* node = source->polyList.root; node; node = node->next)
                *actPolyPtr++ = node->poly;
        }
        return result;
    }

    zCBspLeaf* leaf = actLeafPtr;
    leaf->bbox3D = source->bbox3D;
    leaf->parent = parent;
    leaf->numPolys = source->polyList.count;
    leaf->polyList = source->polyList.count > 0 ? actPolyPtr : NULL;
    for (zTPolyNode* node = source->polyList.root; node; node = node->next)
        *actPolyPtr++ = node->poly;
    ++actLeafPtr;
    return leaf;
}

void zCBspTree::Build(zCCBspTree* source)
{
    bspTreeMode = source->bspTreeMode;
    compiled = 1;
    mesh = source->mesh;
    mesh->AddRef();

    numNodes = source->CountNodes();
    nodeList = new zCBspNode[numNodes];
    actNodePtr = nodeList;
    numLeafs = source->CountLeafs();
    leafList = new zCBspLeaf[numLeafs];
    actLeafPtr = leafList;
    numPolys = source->CountPolys();
    treePolyList = new zCPolygon*[numPolys];
    actPolyPtr = treePolyList;
    bspRoot = BuildRecursive(source);
    bspRenderTree = this;
    OptimizeLight();
}

zCBspNode::zCBspNode()
{
    parent = NULL;
    numPolys = 0;
    polyList = NULL;
    nodeType = 0;
    front = NULL;
    back = NULL;
    leafList = NULL;
    numLeafs = 0;
    planeSignbits = 0;
}

void zCBspTree::OptimizeLight()
{
    for (int i = 0; i < numLeafs; ++i) {
        zCBspLeaf& leaf = leafList[i];
        for (int polyIndex = 0; polyIndex < leaf.numPolys; ++polyIndex) {
            zCPolygon* polygon = leaf.polyList[polyIndex];
            if (leaf.lightVobList.numInArray > 0)
                polygon->flags[0] |= 0x80;
            else
                polygon->flags[0] &= 0x7F;
        }
    }
}

int zCBspBase::CountNodes()
{
    if (nodeType == 1)
        return 1;

    int count = 0;
    if (front)
        count += front->CountNodes();
    if (back)
        count += back->CountNodes();
    return count;
}

static void RecalcNormalsInBBox(zCBspBase* node, const zTBBox3D& box)
{
    if (!node || !node->bbox3D.IsIntersecting(box))
        return;
    for (int i = 0; i < node->numPolys; ++i)
        node->polyList[i]->CalcNormal();
    if (node->front)
        RecalcNormalsInBBox(node->front, box);
    if (node->back)
        RecalcNormalsInBBox(node->back, box);
}

void zCBspTree::UpdateVertexDependencies(zCVertex* vertex)
{
    if (!vertex)
        return;

    zTBBox3D box;
    box.mins = vertex->position - zVEC3(1.0f, 1.0f, 1.0f);
    box.maxs = vertex->position + zVEC3(1.0f, 1.0f, 1.0f);
    RecalcNormalsInBBox(bspRoot, box);
}

void zTBBox3D::AddPoint(const zVEC3& point)
{
    for (int i = 0; i < 3; ++i) {
        if (point.n[i] > maxs.n[i])
            maxs.n[i] = point.n[i];
        if (point.n[i] < mins.n[i])
            mins.n[i] = point.n[i];
    }
}

static void CollectPolyPointers(zCBspBase* node, const zTBBox3D& box,
    zCArray<zCPolygon*>& polygons)
{
    if (!node || !node->bbox3D.IsIntersecting(box))
        return;
    for (int i = 0; i < node->numPolys; ++i)
        polygons.InsertEnd(node->polyList[i]);
    if (node->front)
        CollectPolyPointers(node->front, box, polygons);
    if (node->back)
        CollectPolyPointers(node->back, box, polygons);
}

int zCBspTree::CollectPolysInBBox3DAngle(const zTBBox3D& box, const zVEC3& direction,
    float minAngle, float maxAngle, zCPolygon**& polygons, int& count) const
{
    zCArray<zCPolygon*> collected;
    CollectPolyPointers(bspRoot, box, collected);
    polygons = collected.numInArray > 0 ? new zCPolygon*[collected.numInArray] : NULL;
    count = 0;

    zVEC3 normal = direction;
    normal.Normalize();
    for (int i = 0; i < collected.numInArray; ++i) {
        zCPolygon* polygon = collected.parray[i];
        float dot = normal * polygon->polyPlane.normal;
        if (dot > 1.0f)
            dot = 1.0f;
        else if (dot < -1.0f)
            dot = -1.0f;
        const float angle = std::acos(dot) * 57.2957795f;
        if (angle < minAngle || angle > maxAngle)
            continue;
        polygons[count++] = polygon;
    }
    return count > 0;
}

void zCBspTree::RenderLightList()
{
    for (int i = 0; i < renderLightList.numInArray; ++i) {
        zCVobLight* light = renderLightList.parray[i];
        light->DoAnimation();
        if (light->lightData.lensFlareFX)
            light->lightData.lensFlareFX->RenderLensFlares(light);
        if (zCVobLight::renderLightVisuals)
            renderVobList.InsertEnd(light);
    }
}

void zCBspTree::RenderVobList()
{
    zCRndAlphaSortObject_Vob::ResetFreeList();

    for (int i = 0; i < renderVobList.numInArray; ++i) {
        zCVob* vob = renderVobList.parray[i];
        if (!vob->visual || !(vob->showVisual & 1))
            continue;

        const zVEC3 position = vob->trafoObjToWorld.GetTranslation();
        const zVEC3 cameraPosition = zCCamera::activeCam->connectedVob
            ->trafoObjToWorld.GetTranslation();
        const float distance = (position - cameraPosition).Length();
        zCVisual* visual = vob->visual;
        float alpha;
        vob->CalcRenderAlpha(distance, visual, alpha);

        if (alpha <= 0.0f || alpha < 1.0f) {
            if (alpha > 0.0f) {
                zCRndAlphaSortObject_Vob* entry = zCRndAlphaSortObject_Vob::GetFreeAlphaVob();
                if (entry->vob)
                    entry->vob->Release();
                entry->vob = vob;
                vob->AddRef();
                entry->alpha = alpha;
                entry->zvalue = distance;
                zrenderer->AddAlphaSortObject(entry);
            }
            renderVobList.parray[i] = renderVobList.parray[--renderVobList.numInArray];
            --i;
            continue;
        }

        zTRenderContext context;
        std::memset(&context, 0, sizeof(context));
        context.clipFlags = 0x3F;
        context.vob = vob;
        context.cam = zCCamera::activeCam;
        context.world = vob->GetHomeWorld();
        context.distVobToCam = distance;
        if (vob->Render(context))
            ++vobsRendered;
    }
}

zVEC3 zCVob::GetPositionWorld() const
{
    return zVEC3(trafoObjToWorld.m[0][3], trafoObjToWorld.m[1][3], trafoObjToWorld.m[2][3]);
}

zVEC3 zCVob::GetAtVectorWorld() const
{
    return zVEC3(trafoObjToWorld.m[0][2], trafoObjToWorld.m[1][2], trafoObjToWorld.m[2][2]);
}

zVEC3 zCVob::GetUpVectorWorld() const
{
    return zVEC3(trafoObjToWorld.m[0][1], trafoObjToWorld.m[1][1], trafoObjToWorld.m[2][1]);
}

zVEC3 zCVob::GetRightVectorWorld() const
{
    return zVEC3(trafoObjToWorld.m[0][0], trafoObjToWorld.m[1][0], trafoObjToWorld.m[2][0]);
}

zCPluecker::zCPluecker(const zVEC3& point, const zVEC3& direction)
{
    values[0] = direction.n[0];
    values[1] = direction.n[1];
    values[2] = direction.n[2];
    const zVEC3 moment = point ^ direction;
    values[3] = moment.n[0];
    values[4] = moment.n[1];
    values[5] = moment.n[2];
}

void zCPluecker::Normalize()
{
    const float length = std::sqrt(values[0] * values[0] + values[1] * values[1]
        + values[2] * values[2]);
    const float scale = 1.0f / length;
    for (int i = 0; i < 6; ++i)
        values[i] *= scale;
}

zCBspLeaf::~zCBspLeaf()
{
    PolyPlueckerRelease();
    for (int i = 0; i < leafVobList.numInArray; ++i)
        leafVobList.parray[i]->vobLeafList.Remove(this);
    for (int i = 0; i < lightVobList.numInArray; ++i)
        lightVobList.parray[i]->vobLeafList.Remove(this);
}

void zCBspLeaf::PolyPlueckerAddRef()
{
    if (!polyPlueckerRefCtr) {
        int edgeCount = 0;
        for (int i = 0; i < numPolys; ++i)
            edgeCount += polyList[i]->polyNumVert;
        polyPluecker = new zCPluecker[edgeCount];

        const zVEC3 center = (bbox3D.mins + bbox3D.maxs) * 0.5f;
        const zVEC3 size = bbox3D.mins - bbox3D.maxs;
        const float scale = 1.0f / size.Length();
        int edge = 0;

        for (int polyIndex = 0; polyIndex < numPolys; ++polyIndex) {
            zCPolygon* polygon = polyList[polyIndex];
            for (int i = 0; i < polygon->polyNumVert; ++i) {
                const zVEC3& first = polygon->vertex[i]->position;
                const zVEC3& second = polygon->vertex[(i + 1) % polygon->polyNumVert]->position;
                const zVEC3 point = (first - center) * scale;
                const zVEC3 direction = (second - first) * scale;
                polyPluecker[edge] = zCPluecker(point, direction);
                polyPluecker[edge].Normalize();
                ++edge;
            }
        }
    }
    ++polyPlueckerRefCtr;
}

void zCBspLeaf::PolyPlueckerRelease()
{
    --polyPlueckerRefCtr;
    if (!polyPlueckerRefCtr) {
        delete[] polyPluecker;
        polyPluecker = NULL;
    }
}

static zCBspTree* bspRenderTree;

void zCBspLeaf::AddVobsToRenderList(zTBBox2D* portal)
{
    for (int i = 0; i < leafVobList.numInArray; ++i) {
        zCVob* vob = leafVobList.parray[i];
        if (vob->lastTimeDrawn == bspFrameCtr)
            continue;

        if (portal) {
            const zTBBox2D box = vob->GetScreenBBox2D();
            if (box.maxs.n[0] < portal->mins.n[0] || box.mins.n[0] > portal->maxs.n[0]
                || box.maxs.n[1] < portal->mins.n[1] || box.mins.n[1] > portal->maxs.n[1])
                continue;
        }

        vob->lastTimeDrawn = bspFrameCtr;
        bspRenderTree->renderVobList.InsertEnd(vob);
    }

    for (int i = 0; i < lightVobList.numInArray; ++i) {
        zCVobLight* light = lightVobList.parray[i];
        if (light->lastTimeDrawn == bspFrameCtr)
            continue;
        light->lastTimeDrawn = bspFrameCtr;
        bspRenderTree->renderLightList.InsertEnd(light);
    }
}

void zCBspLeaf::AddVobsToRenderListOutdoor()
{
    for (int i = 0; i < leafVobList.numInArray; ++i) {
        zCVob* vob = leafVobList.parray[i];
        if (vob->lastTimeDrawn == bspFrameCtr || !vob->visual || !(vob->showVisual & 1)
            || vob->visualAlpha <= 0.0f)
            continue;
        if (!zCWorld::s_bAmbientVobsEnabled && (vob->collButNoMove & 0x40))
            continue;

        vob->lastTimeDrawn = bspFrameCtr;
        bspRenderTree->renderVobList.InsertEnd(vob);
    }

    for (int i = 0; i < lightVobList.numInArray; ++i) {
        zCVobLight* light = lightVobList.parray[i];
        if (light->lastTimeDrawn == bspFrameCtr)
            continue;
        light->lastTimeDrawn = bspFrameCtr;
        bspRenderTree->renderLightList.InsertEnd(light);
    }
}

void zCBspLeaf::AddVobsToRenderListOutdoor(zCBspSector* sector, const zTBBox2D& portal)
{
    for (int i = 0; i < leafVobList.numInArray; ++i) {
        zCVob* vob = leafVobList.parray[i];
        if (vob->lastTimeDrawn == bspFrameCtr)
            continue;

        const zTBBox2D box = vob->GetScreenBBox2D();
        if (box.maxs.n[0] < portal.mins.n[0] || box.mins.n[0] > portal.maxs.n[0]
            || box.maxs.n[1] < portal.mins.n[1] || box.mins.n[1] > portal.maxs.n[1])
            continue;

        vob->lastTimeDrawn = bspFrameCtr;
        bspRenderTree->renderVobList.InsertEnd(vob);
    }

    for (int i = 0; i < lightVobList.numInArray; ++i) {
        zCVobLight* light = lightVobList.parray[i];
        if (light->lastTimeDrawn == bspFrameCtr)
            continue;
        light->lastTimeDrawn = bspFrameCtr;
        bspRenderTree->renderLightList.InsertEnd(light);
    }
}

void zCBspBase::AddVobsToRenderListSubtree(int flags)
{
    if (nodeType == 1) {
        zCBspLeaf* leaf = static_cast<zCBspLeaf*>(this);
        if (bspRenderTree->bspTreeMode == 1)
            leaf->AddVobsToRenderListOutdoor();
        else
            leaf->AddVobsToRenderList(NULL);
        return;
    }

    if (flags <= 0) {
        zCBspNode* node = static_cast<zCBspNode*>(this);
        for (int i = 0; i < node->numLeafs; ++i) {
            if (bspRenderTree->bspTreeMode == 1)
                node->leafList[i].AddVobsToRenderListOutdoor();
            else
                node->leafList[i].AddVobsToRenderList(NULL);
        }
        return;
    }

    if (front)
        front->AddVobsToRenderListSubtree(flags);
    if (back)
        back->AddVobsToRenderListSubtree(flags);
}

zTBBox2D zCBspBase::GetScreenBBox2D() const
{
    zTBBox2D result;
    result.mins = zVEC2(3.40282347e38f, 3.40282347e38f);
    result.maxs = zVEC2(-3.40282347e38f, -3.40282347e38f);

    const zMAT4& matrix = zCCamera::activeCam->camMatrix;
    for (int i = 0; i < 8; ++i) {
        const zVEC3 point(
            i & 1 ? bbox3D.maxs.n[0] : bbox3D.mins.n[0],
            i & 2 ? bbox3D.maxs.n[1] : bbox3D.mins.n[1],
            i & 4 ? bbox3D.maxs.n[2] : bbox3D.mins.n[2]);
        const float x = matrix.m[0][0] * point.n[0] + matrix.m[0][1] * point.n[1]
            + matrix.m[0][2] * point.n[2] + matrix.m[0][3];
        const float y = matrix.m[1][0] * point.n[0] + matrix.m[1][1] * point.n[1]
            + matrix.m[1][2] * point.n[2] + matrix.m[1][3];
        const float z = matrix.m[2][0] * point.n[0] + matrix.m[2][1] * point.n[1]
            + matrix.m[2][2] * point.n[2] + matrix.m[2][3];
        if (z <= 0.0f)
            continue;

        const float screenX = zCCamera::activeCam->viewDistanceX * x / z
            + zCCamera::activeCam->vpData.xcenter;
        const float screenY = zCCamera::activeCam->vpData.ycenter
            - zCCamera::activeCam->viewDistanceY * y / z;
        if (screenX < result.mins.n[0])
            result.mins.n[0] = screenX;
        if (screenX > result.maxs.n[0])
            result.maxs.n[0] = screenX;
        if (screenY < result.mins.n[1])
            result.mins.n[1] = screenY;
        if (screenY > result.maxs.n[1])
            result.maxs.n[1] = screenY;
    }

    if (result.mins.n[0] > result.maxs.n[0]) {
        result.mins = zVEC2(0.0f, 0.0f);
        result.maxs = zVEC2(0.0f, 0.0f);
    }
    return result;
}

static zCArray<zCVobLight*> bspStaticNodeLights;
static zCArray<zCVobLight*> bspDynamicNodeLights;

void zCBspBase::CollectNodeLights()
{
    bspStaticNodeLights.numInArray = 0;
    bspDynamicNodeLights.numInArray = 0;

    zCBspLeaf* leaf = static_cast<zCBspLeaf*>(this);
    for (int i = 0; i < leaf->lightVobList.numInArray; ++i) {
        zCVobLight* light = leaf->lightVobList.parray[i];
        if (light->lightData.m_bCanMove)
            bspDynamicNodeLights.InsertEnd(light);
        else
            bspStaticNodeLights.InsertEnd(light);
    }
}

void zCBspLeaf::RenderLeafIndoor(int flags)
{
    if (lastTimeActivated == bspFrameCtr)
        return;
    lastTimeActivated = bspFrameCtr;

    AddVobsToRenderList(NULL);
    CollectNodeLights();
    for (int i = 0; i < numPolys; ++i) {
        if (polyList[i]->flags[0] & 3)
            break;
        polyList[i]->RenderPoly(flags & 0x0F);
    }
    ++bspRenderTree->leafsRendered;
}

void zCBspBase::RenderTrivInIndoor()
{
    if (nodeType == 1) {
        static_cast<zCBspLeaf*>(this)->RenderLeafIndoor(0);
        return;
    }

    zCBspNode* node = static_cast<zCBspNode*>(this);
    for (int i = 0; i < node->numLeafs; ++i)
        node->leafList[i].RenderLeafIndoor(0);
}

void zCBspBase::RenderIndoor(int flags)
{
    if (nodeType == 1) {
        static_cast<zCBspLeaf*>(this)->RenderLeafIndoor(flags);
        return;
    }

    zCBspNode* node = static_cast<zCBspNode*>(this);
    for (int i = 0; i < node->numLeafs; ++i)
        node->leafList[i].RenderLeafIndoor(flags);
    if (front)
        front->RenderIndoor(flags);
    if (back)
        back->RenderIndoor(flags);
}

void zCBspBase::RenderNodeOutdoor(int flags)
{
    ++bspRenderTree->leafsRendered;
    CollectNodeLights();
    for (int i = 0; i < numPolys; ++i)
        polyList[i]->RenderPoly(flags & 0x0F);
}

void zCBspBase::RenderOutdoor(zCBspBase* node, zTBBox3D box, int flags, int collectVobs)
{
    if (!node)
        return;

    if (node->nodeType == 1) {
        if (collectVobs)
            node->AddVobsToRenderListSubtree(flags);
        node->RenderNodeOutdoor(flags);
        return;
    }

    if (node->front)
        RenderOutdoor(node->front, box, flags, collectVobs);
    if (node->back)
        RenderOutdoor(node->back, box, flags, collectVobs);
}

void zCCamera::Project(const zVEC3* point, float& x, float& y) const
{
    const float invZ = 1.0f / point->n[2];
    x = viewDistanceX * point->n[0] * invZ + vpData.xcenter;
    y = vpData.ycenter - viewDistanceY * point->n[1] * invZ;
}

void zCBspTree::Render()
{
    bspFrameCtr = masterFrameCtr;
    bspRenderTree = this;
    renderVobList.numInArray = 0;
    renderLightList.numInArray = 0;
    leafsRendered = 0;
    vobsRendered = 0;

    zCSkyControler::s_activeSkyControler->RenderSkyPre();
    if (bspTreeMode == 0) {
        if (bspRoot)
            bspRoot->RenderTrivInIndoor();
    }
    else if (bspRoot) {
        bspRoot->RenderOutdoor(bspRoot, bspRoot->bbox3D, 0x3F, 1);
    }

    zrenderMan.Flush();
    RenderLightList();
    RenderVobList();
    zCSkyControler::s_activeSkyControler->RenderSkyPost(0);
    m_bRenderedFirstTime = 0;
    ++masterFrameCtr;
}

void zCBspNode::CalcPlaneSignbits(zCBspTree* tree)
{
    planeSignbits = 0;
    if (plane.normal.n[0] == 0.0f)
        planeSignbits = plane.normal.n[1] == 0.0f ? 2 : 1;
}

void zCBspBase::AddVobToLeafs(zCVob* vob)
{
    zCBspBase* node = this;

    while (node && node->nodeType != 1) {
        const int axis = static_cast<zCBspNode*>(node)->planeSignbits;
        const float distance = static_cast<zCBspNode*>(node)->plane.distance;
        if (vob->bbox3D.maxs.n[axis] < distance) {
            node = node->front;
        }
        else if (vob->bbox3D.mins.n[axis] > distance) {
            node = node->back;
        }
        else {
            if (node->front)
                node->front->AddVobToLeafs(vob);
            node = node->back;
        }
    }

    if (node) {
        zCBspLeaf* leaf = static_cast<zCBspLeaf*>(node);
        leaf->leafVobList.InsertEnd(vob);
        vob->vobLeafList.InsertEnd(leaf);
    }
}

void zCBspBase::AddVobLightToLeafs(zCVobLight* vob)
{
    zCBspBase* node = this;

    while (node && node->nodeType != 1) {
        const zCBspNode* bspNode = static_cast<const zCBspNode*>(node);
        const zVEC3 center = vob->trafoObjToWorld.GetTranslation();
        const int axis = bspNode->planeSignbits;
        const float distance = center.n[axis] - bspNode->plane.distance;
        if (distance < -vob->lightData.range) {
            node = node->front;
        }
        else if (distance > vob->lightData.range) {
            node = node->back;
        }
        else {
            if (node->front)
                node->front->AddVobLightToLeafs(vob);
            node = node->back;
        }
    }

    if (node) {
        zCBspLeaf* leaf = static_cast<zCBspLeaf*>(node);
        leaf->lightVobList.InsertEnd(vob);
        vob->vobLeafList.InsertEnd(leaf);
    }
}

void zCBspBase::RemoveVobFromLeafs(zCVob* vob)
{
    for (int i = 0; i < vob->vobLeafList.numInArray; ++i) {
        zCBspLeaf* leaf = vob->vobLeafList.parray[i];
        if (vob->type == zVOB_TYPE_LIGHT)
            leaf->lightVobList.Remove(vob);
        else
            leaf->leafVobList.Remove(vob);
    }
    vob->vobLeafList.numInArray = 0;
}

void zCBspTree::RemoveVob(zCVob* vob)
{
    if (bspRoot)
        bspRoot->RemoveVobFromLeafs(vob);
}

void zCBspTree::AddVob(zCVob* vob)
{
    if (!bspRoot)
        return;
    bspRoot->RemoveVobFromLeafs(vob);
    if (vob->type != zVOB_TYPE_LIGHT)
        bspRoot->AddVobToLeafs(vob);
    else if ((bspFrameCtr & 1) || !vob->vobLeafList.numInArray || vob->lightData.m_bCanMove)
        bspRoot->AddVobLightToLeafs(vob);
}

void zCBspTree::AdjustVob(zCVob* vob)
{
    AddVob(vob);
}

void zCBspBase::CollectVobsSubtree(zCArray<zCVob*>& vobs) const
{
    extern int bspCollectCtr;
    ++bspCollectCtr;
    CollectVobsSubtree_I(vobs);
}

void zCBspBase::CollectVobsInBBox3D(zCArray<zCVob*>& vobs, const zTBBox3D& box) const
{
    extern int bspCollectCtr;
    ++bspCollectCtr;
    CollectVobsInBBox3D_I(vobs, box);
}

void zCBspBase::CollectVobsSubtree_I(zCArray<zCVob*>& vobs) const
{
    extern int bspCollectCtr;
    extern int tr_numLeafVobs;

    if (nodeType == 1) {
        const zCBspLeaf* leaf = static_cast<const zCBspLeaf*>(this);
        tr_numLeafVobs += leaf->leafVobList.numInArray;
        for (int i = 0; i < leaf->leafVobList.numInArray; ++i) {
            zCVob* vob = leaf->leafVobList.parray[i];
            if (vob->lastTimeDrawn != bspCollectCtr) {
                vob->lastTimeDrawn = bspCollectCtr;
                vobs.InsertEnd(vob);
            }
        }
        return;
    }

    const zCBspNode* node = static_cast<const zCBspNode*>(this);
    for (int i = 0; i < node->numLeafs; ++i)
        node->leafList[i].CollectVobsSubtree_I(vobs);
    if (front)
        front->CollectVobsSubtree_I(vobs);
    if (back)
        back->CollectVobsSubtree_I(vobs);
}

void zCBspBase::CollectLightVobsSubtree(zCArray<zCVob*>& vobs) const
{
    extern int bspCollectCtr;
    ++bspCollectCtr;
    CollectLightVobsSubtree_I(vobs);
}

void zCBspBase::CollectLightVobsSubtree_I(zCArray<zCVob*>& vobs) const
{
    extern int bspCollectCtr;
    extern int tr_numLeafVobs;

    if (nodeType == 1) {
        const zCBspLeaf* leaf = static_cast<const zCBspLeaf*>(this);
        tr_numLeafVobs += leaf->lightVobList.numInArray;
        for (int i = 0; i < leaf->lightVobList.numInArray; ++i) {
            zCVobLight* light = leaf->lightVobList.parray[i];
            if (light->lastTimeDrawn != bspCollectCtr) {
                light->lastTimeDrawn = bspCollectCtr;
                vobs.InsertEnd(light);
            }
        }
        return;
    }

    const zCBspNode* node = static_cast<const zCBspNode*>(this);
    for (int i = 0; i < node->numLeafs; ++i)
        node->leafList[i].CollectLightVobsSubtree_I(vobs);
    if (front)
        front->CollectLightVobsSubtree_I(vobs);
    if (back)
        back->CollectLightVobsSubtree_I(vobs);
}

void zCBspBase::CollectVobsInBBox3D_I(zCArray<zCVob*>& vobs, const zTBBox3D& box) const
{
    extern int bspCollectCtr;
    const zCBspBase* node = this;

    while (node && node->nodeType != 1) {
        const zCBspNode* bspNode = static_cast<const zCBspNode*>(node);
        const int axis = bspNode->planeSignbits;
        if (box.maxs.n[axis] < bspNode->plane.distance) {
            node = node->front;
        }
        else if (box.mins.n[axis] > bspNode->plane.distance) {
            node = node->back;
        }
        else {
            if (node->front)
                node->front->CollectVobsInBBox3D_I(vobs, box);
            node = node->back;
        }
    }

    if (!node)
        return;

    const zCBspLeaf* leaf = static_cast<const zCBspLeaf*>(node);
    for (int i = 0; i < leaf->leafVobList.numInArray; ++i) {
        zCVob* vob = leaf->leafVobList.parray[i];
        if (vob->lastTimeDrawn != bspCollectCtr && vob->bbox3D.IsIntersecting(box)) {
            vob->lastTimeDrawn = bspCollectCtr;
            vobs.InsertEnd(vob);
        }
    }
}

void zCBspLeaf::TraceRayCollectVobs(zCArray<zCVob*>& vobs, const zTBBox3D& box, int flags) const
{
    extern int bspCollectCtr;
    extern int tr_numLeafVobs;
    tr_numLeafVobs += leafVobList.numInArray;

    for (int i = 0; i < leafVobList.numInArray; ++i) {
        zCVob* vob = leafVobList.parray[i];
        if (vob->lastTimeDrawn == bspCollectCtr)
            continue;
        if (!(flags & 0x20) && !vob->bbox3D.IsIntersecting(box))
            continue;
        vob->lastTimeDrawn = bspCollectCtr;
        vobs.InsertEnd(vob);
    }

    if (flags & 0x20) {
        for (int i = 0; i < lightVobList.numInArray; ++i) {
            zCVobLight* light = lightVobList.parray[i];
            if (light->lastTimeDrawn != bspCollectCtr) {
                light->lastTimeDrawn = bspCollectCtr;
                vobs.InsertEnd(light);
            }
        }
    }
}

zCBspLeaf* zCBspBase::FindLeaf(const zVEC3& position)
{
    zCBspBase* node = this;
    while (node && node->nodeType != 1) {
        const zCBspNode* bspNode = static_cast<const zCBspNode*>(node);
        const float distance = bspNode->plane.normal * position - bspNode->plane.distance;
        node = distance < 0.001f ? node->front : node->back;
    }
    return static_cast<zCBspLeaf*>(node);
}

zTBBox3D zCBspTree::CalcLeafRealBBox3D(const zVEC3& position)
{
    zTBBox3D result = bspRoot->bbox3D;
    const zCBspBase* node = bspRoot;

    while (node && node->nodeType != 1) {
        const zCBspNode* bspNode = static_cast<const zCBspNode*>(node);
        const int axis = bspNode->planeSignbits;
        const float distance = bspNode->plane.normal * position - bspNode->plane.distance;
        if (distance < 0.001f) {
            result.maxs.n[axis] = bspNode->plane.distance;
            node = node->front;
        }
        else {
            result.mins.n[axis] = bspNode->plane.distance;
            node = node->back;
        }
    }
    return result;
}

static int bspRayFlags;
static zCArray<zCVob*>* bspRayVobs;
static zCRayCache* bspRayCache;

int zCBspBase::CheckRayAgainstPolys(const zVEC3& start, const zVEC3& end,
    const zVEC3& direction, zCRayTurbo* turbo) const
{
    zTTraceRayReport report;
    for (int i = 0; i < numPolys; ++i) {
        if (polyList[i]->CheckRayPolyIntersection(start, direction, bspRayFlags & 0x400, report)
            && report.foundHit)
            return 1;
    }
    return 0;
}

int zCBspBase::CheckRayAgainstPolysNearestHit(const zVEC3& start, const zVEC3& end,
    const zVEC3& direction, zCRayTurbo* turbo) const
{
    float nearest = 3.40282347e38f;
    zCPolygon* nearestPoly = NULL;

    for (int i = 0; i < numPolys; ++i) {
        zTTraceRayReport report;
        if (!polyList[i]->CheckRayPolyIntersection(start, direction, bspRayFlags & 0x400, report)
            || !report.foundHit)
            continue;
        const float distance = (report.foundIntersection - start).Length2();
        if (distance < nearest) {
            nearest = distance;
            nearestPoly = polyList[i];
        }
    }

    if (nearestPoly) {
        zTTraceRayReport report;
        nearestPoly->CheckRayPolyIntersection(start, direction, bspRayFlags & 0x400, report);
        const_cast<zVEC3&>(direction) = report.foundIntersection;
        return 1;
    }
    return 0;
}

int zCBspBase::CheckRayAgainstPolysCache(const zVEC3& start, const zVEC3& end,
    const zVEC3& direction) const
{
    return CheckRayAgainstPolysNearestHit(start, end, direction, NULL);
}

int zCBspBase::RayIntersection(const zVEC3& start, const zVEC3& end, zVEC3& direction) const
{
    direction = (end - start) * 1.00100005f;

    if (nodeType == 1) {
        const zCBspLeaf* leaf = static_cast<const zCBspLeaf*>(this);
        if (bspRayVobs)
            leaf->TraceRayCollectVobs(*bspRayVobs, leaf->bbox3D, bspRayFlags);
        return CheckRayAgainstPolysNearestHit(start, end, direction, NULL);
    }

    const zCBspNode* node = static_cast<const zCBspNode*>(this);
    const int axis = node->planeSignbits;
    const float distance = start.n[axis] - node->plane.distance;
    const zCBspBase* first = distance < 0.0f ? node->front : node->back;
    const zCBspBase* second = distance < 0.0f ? node->back : node->front;

    if (first && first->RayIntersection(start, end, direction))
        return 1;
    if (second)
        return second->RayIntersection(start, end, direction);
    return 0;
}

int zCBspTree::TraceRay(const zVEC3& start, const zVEC3& end, int flags,
    zVEC3& intersection, zCPolygon*& polygon, zCArray<zCVob*>* vobs) const
{
    if (vobs)
        vobs->numInArray = 0;
    bspRayFlags = flags;
    bspRayVobs = vobs;
    polygon = NULL;
    intersection = end;

    if (!bspRoot)
        return 0;

    zVEC3 direction = (end - start) * 1.00100005f;
    const int result = bspRoot->RayIntersection(start, end, direction);
    if (result) {
        intersection = direction;
        ++zengineStats.numTraceRay;
    }
    bspRayVobs = NULL;
    return result;
}

void zCBspTree::SetRayCache(zCRayCache* cache)
{
    bspRayCache = cache;
}

static zCPolygon* bspConstructionRayHit;

int zCCBspNode::CheckRayAgainstPolysNearestHit(const zVEC3& start, const zVEC3& end,
    const zVEC3& direction, zCRayTurbo* turbo) const
{
    float nearest = 3.40282347e38f;
    zCPolygon* nearestPoly = NULL;

    for (zTPolyNode* node = polyList.root; node; node = node->next) {
        zTTraceRayReport report;
        if (!node->poly->CheckRayPolyIntersection(start, direction, 0, report)
            || !report.foundHit)
            continue;
        const float distance = (report.foundIntersection - start).Length2();
        if (distance < nearest) {
            nearest = distance;
            nearestPoly = node->poly;
        }
    }

    if (nearestPoly) {
        bspConstructionRayHit = nearestPoly;
        zTTraceRayReport report;
        nearestPoly->CheckRayPolyIntersection(start, direction, 0, report);
        direction = report.foundIntersection;
        return 1;
    }
    return 0;
}

int zCCBspNode::RayIntersectionOctree(const zVEC3& start, const zVEC3& end, zVEC3& direction)
{
    direction = end;
    if (!front && !back)
        return CheckRayAgainstPolysNearestHit(start, end, direction, NULL);

    const float startDistance = plane.normal * start - plane.distance;
    const float endDistance = plane.normal * end - plane.distance;
    if (startDistance < 0.0f && endDistance < 0.0f) {
        if (front)
            return front->RayIntersectionOctree(start, end, direction);
    }
    else if (startDistance > 0.0f && endDistance > 0.0f) {
        if (back)
            return back->RayIntersectionOctree(start, end, direction);
    }
    else {
        if (front && front->RayIntersectionOctree(start, end, direction))
            return 1;
        if (back && back->RayIntersectionOctree(start, end, direction))
            return 1;
    }
    return 0;
}

int zCCBspNode::TraceRay(const zVEC3& start, const zVEC3& end, zCPolygon*& polygon,
    zVEC3& intersection)
{
    bspConstructionRayHit = NULL;
    const int result = RayIntersectionOctree(start, end, intersection);
    polygon = bspConstructionRayHit;
    return result;
}

int zCCBspNode::TraceRay(const zVEC3& start, const zVEC3& end, zCPolygon*& polygon)
{
    zVEC3 intersection;
    return TraceRay(start, end, polygon, intersection);
}

static zCArray<zCPolygon*> bspBoxPolys;
static zTBBox3D bspBoxPolyBounds;

void zCBspBase::CollectPolysInBBox3DRec() const
{
    if (!bbox3D.IsIntersecting(bspBoxPolyBounds))
        return;
    for (int i = 0; i < numPolys; ++i) {
        if (!(polyList[i]->flags[0] & 3))
            bspBoxPolys.InsertEnd(polyList[i]);
    }
    if (front)
        front->CollectPolysInBBox3DRec();
    if (back)
        back->CollectPolysInBBox3DRec();
}

int zCBspBase::CollectPolysInBBox3D(const zTBBox3D& box, zCPolygon**& polygons, int& count) const
{
    bspBoxPolys.numInArray = 0;
    bspBoxPolyBounds = box;
    CollectPolysInBBox3DRec();
    count = bspBoxPolys.numInArray;
    polygons = bspBoxPolys.parray;
    return count;
}

void zCCBspNode::CollectPolysInBBox3DRec()
{
    if (!bbox3D.IsIntersecting(bspBoxPolyBounds))
        return;
    for (zTPolyNode* node = polyList.root; node; node = node->next)
        bspBoxPolys.InsertEnd(node->poly);
    if (front)
        front->CollectPolysInBBox3DRec();
    if (back)
        back->CollectPolysInBBox3DRec();
}

int zCCBspNode::CollectPolysInBBox3D(const zTBBox3D& box, zCPolygon**& polygons, int& count)
{
    bspBoxPolys.numInArray = 0;
    bspBoxPolyBounds = box;
    CollectPolysInBBox3DRec();
    count = bspBoxPolys.numInArray;
    polygons = bspBoxPolys.parray;
    return count;
}

int zCBspTree::FindLeaks(zCArray<zCPolygon*>& leaks)
{
    leaks.numInArray = 0;
    int found = 0;
    for (int i = 0; i < numPolys; ++i) {
        zCPolygon* polygon = treePolyList[i];
        if (polygon->flags[0] & 3)
            continue;
        if (polygon->material && polygon->material->matGroup == 5)
            continue;

        const zVEC3 position = polygon->GetCenter() - polygon->polyPlane.normal;
        if (!bspRoot->FindLeaf(position))
            continue;

        ++found;
        int known = 0;
        for (int j = 0; j < leaks.numInArray; ++j) {
            if (leaks.parray[j] == polygon) {
                known = 1;
                break;
            }
        }
        if (!known)
            leaks.InsertEnd(polygon);
    }

    if (found > 0) {
        zCMaterial* leakMaterial = new zCMaterial;
        leakMaterial->color[0] = 0;
        leakMaterial->color[1] = 0;
        leakMaterial->color[2] = 255;
        for (int i = 0; i < leaks.numInArray; ++i) {
            zCPolygon* polygon = leaks.parray[i];
            if (polygon->material != leakMaterial) {
                if (polygon->material && polygon->material->Release() <= 0)
                    delete polygon->material;
                polygon->material = leakMaterial;
                leakMaterial->AddRef();
            }
        }
    }
    return leaks.numInArray > 0;
}

void zCBspTree::MarkOccluderPolys()
{
    for (int i = 0; i < mesh->numPoly; ++i) {
        zCPolygon* polygon = mesh->polyArray ? &mesh->polyArray[i] : mesh->polyList[i];
        if (polygon->flags[0] & 0x40) {
            polygon->flags[0] ^= 4;
            continue;
        }

        zCMaterial* material = polygon->material;
        if (material && (material->smooth & 0x10)) {
            polygon->flags[0] &= 0xFB;
            continue;
        }
        if (material && (material->smooth & 0x20))
            polygon->flags[0] |= 4;
        if (polygon->polyPlane.normal.n[1] < -0.01f) {
            polygon->flags[0] &= 0xFB;
            continue;
        }
        if (polygon->polyPlane.normal.n[1] < 0.8f || !material
            || (material->texture && material->texture->HasAlpha())
            || material->color[3] < 255 || material->rndAlphaBlendFunc > 1
            || material->matGroup == 5) {
            polygon->flags[0] &= 0xFB;
            continue;
        }

        zTBBox3D box = polygon->GetBBox3D();
        for (int axis = 0; axis < 3; ++axis) {
            if (box.mins.n[axis] == box.maxs.n[axis]) {
                box.mins.n[axis] -= 1.0f;
                box.maxs.n[axis] += 1.0f;
            }
        }
        box.Scale(1.2f);

        zCPolygon** polygons;
        int count;
        bspRoot->CollectPolysInBBox3D(box, polygons, count);
        int occluder = 1;
        for (int j = 0; j < count; ++j) {
            zCPolygon* other = polygons[j];
            if (other == polygon || (other->flags[0] & 8)
                || other->polyPlane.normal.n[1] < 0.0001f
                || !other->material || other->material->matGroup == 5)
                continue;
            const zVEC3 normalDelta = polygon->polyPlane.normal - other->polyPlane.normal;
            if (fabs(normalDelta.n[0]) < 0.001f && fabs(normalDelta.n[1]) < 0.001f
                && fabs(normalDelta.n[2]) < 0.001f)
                continue;
            const zTBBox3D otherBox = other->GetBBox3D();
            if (otherBox.maxs.n[1] < box.mins.n[1])
                continue;
            if (polygon->IsIntersectingProjection(other, zVEC3(0.0f, 1.0f, 0.0f))) {
                occluder = 0;
                break;
            }
        }
        polygon->flags[0] = (polygon->flags[0] & 0xFB) | (occluder ? 4 : 0);
    }

    for (int i = 0; i < mesh->numPoly; ++i) {
        zCPolygon* polygon = mesh->polyArray ? &mesh->polyArray[i] : mesh->polyList[i];
        if (!(polygon->flags[0] & 4) || (polygon->flags[0] & 0x40))
            continue;
        zCArray<zCPolygon*> group;
        group.InsertEnd(polygon);
        if (GetOccluderAreaRec(polygon, group) < 90000.0f) {
            for (int j = 0; j < group.numInArray; ++j)
                group.parray[j]->flags[0] &= 0xFB;
        }
    }
}

float zCBspTree::GetOccluderAreaRec(const zCPolygon* polygon, zCArray<zCPolygon*>& visited)
{
    float area = polygon->GetArea();
    zTBBox3D box = polygon->GetBBox3D();
    for (int axis = 0; axis < 3; ++axis) {
        if (box.mins.n[axis] == box.maxs.n[axis]) {
            box.mins.n[axis] -= 1.0f;
            box.maxs.n[axis] += 1.0f;
        }
    }
    box.Scale(1.2f);

    zCPolygon** polygons;
    int count;
    bspRoot->CollectPolysInBBox3D(box, polygons, count);
    zCArray<zCPolygon*> neighbours;
    for (int i = 0; i < count; ++i) {
        zCPolygon* other = polygons[i];
        if (other == polygon || !(other->flags[0] & 4) || (other->flags[0] & 0x40))
            continue;
        int known = 0;
        for (int j = 0; j < visited.numInArray; ++j) {
            if (visited.parray[j] == other) {
                known = 1;
                break;
            }
        }
        if (!known && other->IsNeighbourOf(polygon)) {
            neighbours.InsertEnd(other);
            visited.InsertEnd(other);
        }
    }
    for (int i = 0; i < neighbours.numInArray; ++i)
        area += GetOccluderAreaRec(neighbours.parray[i], visited);
    return area;
}

static int CompareBspPolyTypes(const void* first, const void* second)
{
    const zCPolygon* firstPoly = *static_cast<zCPolygon* const*>(first);
    const zCPolygon* secondPoly = *static_cast<zCPolygon* const*>(second);
    const int firstType = (firstPoly->flags[0] & 3) ? 1 : ((firstPoly->flags[0] & 8) ? 2 : 0);
    const int secondType = (secondPoly->flags[0] & 3) ? 1 : ((secondPoly->flags[0] & 8) ? 2 : 0);
    return firstType - secondType;
}

void zCBspBase::SortNodePolys(zCBspTree* tree)
{
    zCBspBase* node = this;
    while (node) {
        if (node->numPolys > 0 && (tree->bspTreeMode != 0 || node->nodeType == 1))
            qsort(node->polyList, node->numPolys, sizeof(zCPolygon*), CompareBspPolyTypes);
        if (node->nodeType != 0)
            break;
        if (node->front)
            node->front->SortNodePolys(tree);
        node = node->back;
    }
}

int zCBspSector::s_sectorsRendered;
int zCBspSector::s_startPointOutdoor;
int zCBspSector::s_outdoorPortal;
zCArray<zCBspSector*> zCBspSector::s_activeSectorList;

void zCBspSector::RenderSector()
{
    if (rendered == bspFrameCtr)
        return;
    rendered = bspFrameCtr;
    ++s_sectorsRendered;

    for (int i = 0; i < sectorNodes.numInArray; ++i) {
        zCBspBase* node = sectorNodes.parray[i];
        if (zCCamera::activeCam->BBox3DInFrustum(node->bbox3D) == 1)
            continue;
        node->CollectNodeLights();
        for (int j = node->numPolys - 1; j >= 0; --j) {
            zCPolygon* polygon = node->polyList[j];
            if (!(polygon->flags[0] & 3) || !(polygon->flags[0] & 8))
                break;
            if (polygon->material->bspSectorFront == this)
                polygon->RenderPoly(0x3F);
        }
    }

    for (int i = 0; i < sectorPortals.numInArray; ++i) {
        const unsigned char alpha = sectorPortalInfo.parray[i].alpha;
        zCPolygon* portal = sectorPortals.parray[i];
        if (alpha > 1) {
            portal->material->color[3] = alpha;
            zrenderer->DrawPoly(portal);
        }
    }
}

static void InitSectorActivation(zCBspSector* sector, const zTBBox2D& portal)
{
    if (sector->activated == bspFrameCtr) {
        for (int axis = 0; axis < 2; ++axis) {
            if (portal.mins.n[axis] < sector->activePortal.mins.n[axis])
                sector->activePortal.mins.n[axis] = portal.mins.n[axis];
            if (portal.maxs.n[axis] > sector->activePortal.maxs.n[axis])
                sector->activePortal.maxs.n[axis] = portal.maxs.n[axis];
        }
        return;
    }

    sector->activated = bspFrameCtr;
    zCBspSector::s_activeSectorList.InsertEnd(sector);
    if (sector->sectorPortalInfo.numInArray != sector->sectorPortals.numInArray) {
        sector->sectorPortalInfo.AllocAbs(sector->sectorPortals.numInArray);
        sector->sectorPortalInfo.numInArray = sector->sectorPortals.numInArray;
        std::memset(sector->sectorPortalInfo.parray, 0,
            sector->sectorPortalInfo.numInArray * sizeof(zCBspSector::zTPortalInfo));
    }
    sector->activePortal = portal;
}

static int PrepareSectorPortal(zCPolygon* portal, unsigned char& alpha)
{
    alpha = 0;
    if (portal->polyPlane.normal * zCCamera::activeCamPos - portal->polyPlane.distance < 0.0f)
        return 0;
    if (!portal->ClipToFrustum(0x0F))
        return 0;

    const int type = portal->flags[0] & 3;
    zVEC3 position = portal->vertex[0]->position;
    if (type == 2)
        position = (portal->vertex[0]->position + portal->vertex[2]->position) * 0.5f;
    const float distance = (position - zCCamera::activeCamPos).Length2();
    if (type == 0) {
        portal->material->color[3] = 255;
        alpha = distance < 16000000.0f ? 255 : portal->material->color[3];
        return distance >= 16000000.0f;
    }

    const float nearDistance = type == 2 ? 16000000.0f : 3250000.0f;
    const float farDistance = type == 2 ? 30250000.0f : 16000000.0f;
    const float fadeScale = type == 2 ? 0.0000217f : 0.00000221f;
    if (distance <= nearDistance) {
        portal->material->color[3] = 255;
        alpha = 255;
        return 0;
    }
    if (distance >= farDistance) {
        portal->material->color[3] = 0;
        alpha = 0;
        return 1;
    }
    alpha = static_cast<unsigned char>((distance - nearDistance) * fadeScale);
    portal->material->color[3] = alpha;
    return 1;
}

static int ClipSectorPortalBBox(zTBBox2D& portalBox, const zTBBox2D& activeBox)
{
    if (portalBox.maxs.n[0] < activeBox.mins.n[0]
        || portalBox.maxs.n[1] < activeBox.mins.n[1]
        || portalBox.mins.n[0] > activeBox.maxs.n[0]
        || portalBox.mins.n[1] > activeBox.maxs.n[1])
        return 0;
    for (int axis = 0; axis < 2; ++axis) {
        if (portalBox.mins.n[axis] < activeBox.mins.n[axis])
            portalBox.mins.n[axis] = activeBox.mins.n[axis];
        if (portalBox.maxs.n[axis] > activeBox.maxs.n[axis])
            portalBox.maxs.n[axis] = activeBox.maxs.n[axis];
    }
    return 1;
}

void zCBspSector::ActivateSectorRec(const zTBBox2D& portal, zCBspSector* lastSector, int depth)
{
    if (depth > 40)
        return;
    InitSectorActivation(this, portal);

    for (int i = 0; i < sectorNodes.numInArray; ++i) {
        zCBspLeaf* leaf = static_cast<zCBspLeaf*>(sectorNodes.parray[i]);
        leaf->AddVobsToRenderListOutdoor(this, activePortal);
        leaf->lastTimeActivated = bspFrameCtr;
        leaf->sectorIndex = 0;
    }

    for (int i = 0; i < sectorPortals.numInArray; ++i) {
        zCPolygon* portalPoly = sectorPortals.parray[i];
        zCMaterial* material = portalPoly->material;
        if (!material->bspSectorFront)
            continue;

        int visible = sectorPortalInfo.parray[i].visible;
        if (lastSector) {
            visible = PrepareSectorPortal(portalPoly, sectorPortalInfo.parray[i].alpha);
            sectorPortalInfo.parray[i].visible = visible;
            if (!visible && sectorPortalInfo.parray[i].alpha == 255) {
                portalPoly->TransformProjectLight();
                portalPoly->RenderPoly(0x3F);
            }
        }
        if (!visible)
            continue;

        if (material->color[3] == 0)
            portalPoly->TransformProject();
        else
            portalPoly->TransformProjectLight();
        zTBBox2D portalBox;
        portalPoly->GetScreenBBox2D(portalBox);
        if (!ClipSectorPortalBBox(portalBox, activePortal))
            continue;

        zCBspSector* nextSector = material->bspSectorBack;
        if (!nextSector)
            s_activeOutdoorPortals.InsertEnd(portalBox);
        else if (nextSector != lastSector)
            nextSector->ActivateSectorRec(portalBox, this, depth + 1);
    }
}

void zCBspSector::ActivateSectorRecIndoor(const zTBBox2D& portal,
    zCBspSector* lastSector, int depth)
{
    if (depth > 15)
        return;
    InitSectorActivation(this, portal);

    for (int i = 0; i < sectorPortals.numInArray; ++i) {
        zCPolygon* portalPoly = sectorPortals.parray[i];
        int visible = sectorPortalInfo.parray[i].visible;
        if (lastSector) {
            visible = PrepareSectorPortal(portalPoly, sectorPortalInfo.parray[i].alpha);
            sectorPortalInfo.parray[i].visible = visible;
            if (visible || sectorPortalInfo.parray[i].alpha > 1)
                portalPoly->TransformProjectLight();
            if (sectorPortalInfo.parray[i].alpha > 1) {
                portalPoly->material->rndAlphaBlendFunc = 2;
                portalPoly->material->color[3] = sectorPortalInfo.parray[i].alpha;
                zrenderer->DrawPoly(portalPoly);
            }
        }
        if (!visible)
            continue;

        zTBBox2D portalBox;
        portalPoly->GetScreenBBox2D(portalBox);
        if (!ClipSectorPortalBBox(portalBox, activePortal))
            continue;
        zCBspSector* nextSector = bspRenderTree->sectorList.parray[portalPoly->flags[1]];
        if (nextSector != lastSector)
            nextSector->ActivateSectorRecIndoor(portalBox, this, depth + 1);
    }

    const unsigned short index = lastSector ? sectorIndex : 0xFFFF;
    for (int i = 0; i < sectorNodes.numInArray; ++i) {
        zCBspLeaf* leaf = static_cast<zCBspLeaf*>(sectorNodes.parray[i]);
        leaf->lastTimeActivated = bspFrameCtr;
        leaf->sectorIndex = index;
    }
}

void zCBspSector::RenderActiveSectorList()
{
    for (int i = 0; i < s_activeSectorList.numInArray; ++i)
        s_activeSectorList.parray[i]->RenderSector();
    s_activeSectorList.numInArray = 0;
}

zCBspSector* zCCBspTree::GetBspSectorByName(const zSTRING& name) const
{
    if (name.IsEmpty())
        return NULL;
    for (int i = 0; i < sectorList.numInArray; ++i) {
        if (sectorList.parray[i]->sectorName == name)
            return sectorList.parray[i];
    }
    return NULL;
}

const zSTRING* zCBspTree::GetSectorNameVobIsIn(const zCVob* vob) const
{
    if (!vob->groundPoly || !vob->groundPoly->material->bspSectorFront)
        return NULL;
    return &vob->groundPoly->material->bspSectorFront->sectorName;
}

void zCCBspTree::GetPolyNeighbours(zCPolygon* polygon, zCPolygon**& polygons, int& count)
{
    zTBBox3D box = polygon->GetBBox3D();
    box.Scale(1.1f);
    CollectPolysInBBox3D(box, polygons, count);

    int output = 0;
    for (int i = 0; i < count; ++i) {
        zCPolygon* other = polygons[i];
        if (other == polygon || ((other->flags[0] & 8) && !(other->flags[0] & 3)))
            continue;
        int known = 0;
        for (int j = 0; j < output; ++j) {
            if (polygons[j] == other) {
                known = 1;
                break;
            }
        }
        if (!known)
            polygons[output++] = other;
    }
    count = output;
}

void zCCBspTree::MarkGhostOccluder()
{
    for (int i = 0; i < mesh->numPoly; ++i) {
        zCPolygon* polygon = mesh->polyArray ? &mesh->polyArray[i] : mesh->polyList[i];
        zCMaterial* material = polygon->material;
        polygon->flags[0] &= 0xBF;
        if (material->objectName.Search("GHOSTOCCLUDER") == -1)
            continue;
        polygon->flags[0] = (polygon->flags[0] & 0xB8) | 0x45;
        if (material->texture)
            material->SetTexture(zSTRING());
    }
}

void zCCBspTree::CreateBspSectors()
{
    sectorList.numInArray = 0;
    for (int i = 0; i < mesh->numPoly; ++i) {
        zCPolygon* polygon = mesh->polyArray ? &mesh->polyArray[i] : mesh->polyList[i];
        zCMaterial* material = polygon->material;
        polygon->flags[0] &= 0xB4;
        if (!material || material->objectName.Search("PORTAL") == -1)
            continue;

        polygon->flags[0] |= 8;
        const int noFade = material->objectName.Search("NOFADE") != -1;
        if (noFade)
            polygon->flags[0] = (polygon->flags[0] & 0xFC) | 3;
        else if (polygon->GetArea() < 485000.0f && polygon->GetLongestEdgeLen() < 500.0f)
            polygon->flags[0] = (polygon->flags[0] & 0xFC) | 1;
        else
            polygon->flags[0] = (polygon->flags[0] & 0xFC) | 2;

        zCBspSector* sector = GetBspSectorByName(material->objectName);
        if (!sector) {
            sector = new zCBspSector;
            sector->sectorName = material->objectName;
            sector->sectorIndex = sectorList.numInArray;
            sector->activated = -1;
            sector->rendered = -1;
            sector->hasBigNoFade = noFade;
            material->bspSectorFront = NULL;
            material->bspSectorBack = NULL;
            sectorList.InsertEnd(sector);
        }
        sector->sectorPortals.InsertEnd(polygon);
        if (noFade)
            sector->hasBigNoFade = 1;
        polygon->flags[1] = static_cast<unsigned char>(sector->sectorIndex);
        if (!material->bspSectorFront)
            material->bspSectorFront = sector;
        else if (material->bspSectorFront != sector)
            material->bspSectorBack = sector;
    }

    for (int i = 0; i < sectorList.numInArray; ++i) {
        zCBspSector* sector = sectorList.parray[i];
        sector->CalcCenterByPortals();
        for (int j = 0; j < sector->sectorPortals.numInArray; ++j) {
            zCPolygon* portal = sector->sectorPortals.parray[j];
            if (!portal->material->bspSectorFront)
                portal->material->bspSectorFront = sector;
        }
    }
}

zVEC3::zVEC3(float value)
{
    n[0] = value;
    n[1] = value;
    n[2] = value;
}

zSTRING zVEC3::GetString() const
{
    zSTRING result;
    result.Sprintf(" (%g, %g, %g) ", n[0], n[1], n[2]);
    return result;
}

void zCPolygon::SetMaterial(zCMaterial* value)
{
    if (material == value)
        return;
    if (material && material->Release() <= 0)
        delete material;
    material = value;
    if (material)
        material->AddRef();
}

zTBBox2D::zTBBox2D()
{
}

zCBspSector* zCBspTree::GetBspSectorByName(const zSTRING& name) const
{
    if (name.IsEmpty())
        return NULL;
    for (int i = 0; i < sectorList.numInArray; ++i) {
        if (sectorList.parray[i]->sectorName == name)
            return sectorList.parray[i];
    }
    return NULL;
}

static void AddLeafToSector(zCBspSector* sector, zCBspLeaf* leaf, zCPolygon* portal)
{
    if (!sector)
        return;
    int known = 0;
    for (int i = 0; i < sector->sectorPortals.numInArray; ++i) {
        if (sector->sectorPortals.parray[i] == portal) {
            known = 1;
            break;
        }
    }
    if (!known)
        sector->sectorPortals.InsertEnd(portal);

    known = 0;
    for (int i = 0; i < sector->sectorNodes.numInArray; ++i) {
        if (sector->sectorNodes.parray[i] == leaf) {
            known = 1;
            break;
        }
    }
    if (!known)
        sector->sectorNodes.InsertEnd(leaf);
}

void zCBspTree::CreateBspSectors2(zCCBspTree* source)
{
    if (source->sectorList.numInArray <= 0)
        return;
    sectorList = source->sectorList;
    for (int i = 0; i < sectorList.numInArray; ++i)
        sectorList.parray[i]->sectorPortals.numInArray = 0;

    for (int i = 0; i < numLeafs; ++i) {
        zCBspLeaf* leaf = &leafList[i];
        for (int j = 0; j < leaf->numPolys; ++j) {
            zCPolygon* polygon = leaf->polyList[j];
            if (!(polygon->flags[0] & 8))
                continue;
            zCMaterial* material = polygon->material;
            if (polygon->flags[0] & 3)
                material->rndAlphaBlendFunc = 2;
            AddLeafToSector(material->bspSectorFront, leaf, polygon);
            if (material->bspSectorBack != material->bspSectorFront)
                AddLeafToSector(material->bspSectorBack, leaf, polygon);
        }
    }
    source->sectorList.numInArray = 0;
}

unsigned short zCBspTree::GetFileVersion()
{
    return 3;
}

static zCBspTree* bspFileTree;
static int bspFileNodeIndex;
static int bspFileLeafIndex;

void zCBspBase::SaveBINRec(zCFileBIN& file)
{
    zCBspBase* node = this;
    while (node) {
        file.file->Write(&node->bbox3D, sizeof(zTBBox3D));
        const int polyOffset = node->numPolys > 0
            ? static_cast<int>(node->polyList - bspFileTree->treePolyList) : 0;
        file.file->Write(&polyOffset, 4);
        file.file->Write(&node->numPolys, 4);
        if (node->nodeType != 0)
            break;

        zCBspNode* bspNode = static_cast<zCBspNode*>(node);
        unsigned char childFlags = 0;
        if (node->front)
            childFlags |= node->front->nodeType == 1 ? 5 : 1;
        if (node->back)
            childFlags |= node->back->nodeType == 1 ? 10 : 2;
        file.file->Write(&childFlags, 1);
        file.file->Write(&bspNode->plane, sizeof(zTPlane));
        if (node->front)
            node->front->SaveBINRec(file);
        node = node->back;
    }
}

void zCBspBase::LoadBINRec(zCFileBIN& file)
{
    file.file->Read(&bbox3D, sizeof(zTBBox3D));
    int polyOffset;
    file.file->Read(&polyOffset, 4);
    file.file->Read(&numPolys, 4);
    polyList = numPolys > 0 ? bspFileTree->treePolyList + polyOffset : NULL;
    if (nodeType != 0)
        return;

    unsigned char childFlags;
    zCBspNode* node = static_cast<zCBspNode*>(this);
    file.file->Read(&childFlags, 1);
    file.file->Read(&node->plane, sizeof(zTPlane));
    node->planeSignbits = node->plane.normal.n[0] == 0.0f
        ? (node->plane.normal.n[1] == 0.0f ? 2 : 1) : 0;
    front = NULL;
    back = NULL;
    zCBspLeaf* firstLeaf = bspFileTree->leafList + bspFileLeafIndex;

    if (childFlags & 1) {
        if (childFlags & 4)
            front = bspFileTree->leafList + bspFileLeafIndex++;
        else
            front = bspFileTree->nodeList + bspFileNodeIndex++;
        front->parent = node;
        front->LoadBINRec(file);
    }
    if (childFlags & 2) {
        if (childFlags & 8)
            back = bspFileTree->leafList + bspFileLeafIndex++;
        else
            back = bspFileTree->nodeList + bspFileNodeIndex++;
        back->parent = node;
        back->LoadBINRec(file);
    }
    node->leafList = firstLeaf;
    node->numLeafs = bspFileTree->leafList + bspFileLeafIndex - firstLeaf;
}

static int GetBspMeshPolyIndex(zCMesh* mesh, const zCPolygon* polygon)
{
    for (int i = 0; i < mesh->numPoly; ++i) {
        zCPolygon* candidate = mesh->polyArray ? &mesh->polyArray[i] : mesh->polyList[i];
        if (candidate == polygon)
            return i;
    }
    return 0;
}

void zCBspTree::SaveBIN(zCFileBIN& file)
{
    zCPolygon::S_ResetMorphedVerts();
    bspFileTree = this;
    mesh->SortPolysByList(treePolyList, numPolys);

    const long start = file.file->Pos();
    const unsigned long version = (zCMesh::GetFileVersion() + 0x300) << 16;
    unsigned long length = 0;
    file.file->Write(&version, 4);
    file.file->Write(&length, 4);
    mesh->SaveMSH(file);

    file.BinStartChunk(0xC000);
    const unsigned short bspVersion = GetFileVersion();
    file.file->Write(&bspVersion, 2);
    file.file->Write(&bspTreeMode, 4);
    file.BinEndChunk();

    file.BinStartChunk(0xC010);
    file.file->Write(&numPolys, 4);
    for (int i = 0; i < numPolys; ++i) {
        const int index = GetBspMeshPolyIndex(mesh, treePolyList[i]);
        file.file->Write(&index, 4);
    }
    file.BinEndChunk();

    file.BinStartChunk(0xC040);
    file.file->Write(&numNodes, 4);
    file.file->Write(&numLeafs, 4);
    bspRoot->SaveBINRec(file);
    file.BinEndChunk();

    file.BinStartChunk(0xC045);
    for (int i = 0; i < numLeafs; ++i)
        file.file->Write(&leafList[i].lightPosition, sizeof(zVEC3));
    file.BinEndChunk();

    if (sectorList.numInArray > 0) {
        file.BinStartChunk(0xC050);
        file.file->Write(&sectorList.numInArray, 4);
        for (int i = 0; i < sectorList.numInArray; ++i) {
            zCBspSector* sector = sectorList.parray[i];
            file.BinWriteString(sector->sectorName);
            file.file->Write(&sector->sectorNodes.numInArray, 4);
            file.file->Write(&sector->sectorPortals.numInArray, 4);
            for (int j = 0; j < sector->sectorNodes.numInArray; ++j) {
                const int index = static_cast<int>(
                    static_cast<zCBspLeaf*>(sector->sectorNodes.parray[j]) - leafList);
                file.file->Write(&index, 4);
            }
            for (int j = 0; j < sector->sectorPortals.numInArray; ++j) {
                const int index = GetBspMeshPolyIndex(mesh, sector->sectorPortals.parray[j]);
                file.file->Write(&index, 4);
            }
        }
        file.file->Write(&portalList.numInArray, 4);
        for (int i = 0; i < portalList.numInArray; ++i) {
            const int index = GetBspMeshPolyIndex(mesh, portalList.parray[i]);
            file.file->Write(&index, 4);
        }
        file.BinEndChunk();
    }

    file.BinStartChunk(0xC0FF);
    file.BinEndChunk();
    const unsigned char terminator = 0x42;
    file.file->Write(&terminator, 1);
    const long end = file.file->Pos();
    length = end - start - 8;
    file.file->Seek(start + 4);
    file.file->Write(&length, 4);
    file.file->Seek(end);
}

void zCBspTree::ConnectMaterialsToSectors()
{
    for (int i = 0; i < zCMaterial::classDef->objectList.numInArray; ++i) {
        zCMaterial* material = static_cast<zCMaterial*>(zCMaterial::classDef->objectList.parray[i]);
        zSTRING& name = material->objectName;
        if (name.Length() < 2 || name[1] != ':')
            continue;

        if (name[0] == 'S') {
            const zSTRING sectorName = name.Copied(2, name.Length() - 2);
            zCBspSector* sector = GetBspSectorByName(sectorName);
            material->bspSectorFront = sector;
            material->bspSectorBack = sector;
            continue;
        }
        if (name[0] != 'P')
            continue;

        material->rndAlphaBlendFunc = 2;
        int separator = -1;
        for (int j = 2; j < name.Length(); ++j) {
            if (name[j] == ':') {
                separator = j;
                break;
            }
        }
        if (separator < 0) {
            const zSTRING sectorName = name.Copied(2, name.Length() - 2);
            material->bspSectorFront = GetBspSectorByName(sectorName);
            material->bspSectorBack = material->bspSectorFront;
        }
        else {
            const zSTRING frontName = name.Copied(2, separator - 2);
            const zSTRING backName = name.Copied(separator + 1, name.Length() - separator - 1);
            material->bspSectorFront = GetBspSectorByName(frontName);
            material->bspSectorBack = GetBspSectorByName(backName);
        }
    }
}

int zCBspTree::LoadBIN(zCFileBIN& file, int skip)
{
    DisposeTree();
    unsigned long version;
    unsigned long length;
    file.file->Read(&version, 4);
    file.file->Read(&length, 4);
    const long end = file.file->Pos() + length;
    const unsigned long expectedVersion = (zCMesh::GetFileVersion() + 0x300) << 16;
    if (skip || version != expectedVersion) {
        file.file->Seek(end);
        return 0;
    }

    bspFileTree = this;
    if (!mesh)
        mesh = new zCMesh;
    if (!mesh->LoadMSH(file, 1)) {
        file.file->Seek(end);
        return 0;
    }

    int result = 1;
    while (file.file->Pos() < end) {
        unsigned short chunkId;
        unsigned long chunkLength;
        if (file.file->Read(&chunkId, 2) != 2)
            break;
        file.file->Read(&chunkLength, 4);
        const long chunkEnd = file.file->Pos() + chunkLength;

        if (chunkId == 0xC000) {
            unsigned short fileVersion;
            file.file->Read(&fileVersion, 2);
            if (fileVersion != GetFileVersion()) {
                result = 0;
                file.file->Seek(end);
                break;
            }
            file.file->Read(&bspTreeMode, 4);
        }
        else if (chunkId == 0xC010) {
            file.file->Read(&numPolys, 4);
            treePolyList = new zCPolygon*[numPolys];
            for (int i = 0; i < numPolys; ++i) {
                int index;
                file.file->Read(&index, 4);
                treePolyList[i] = mesh->polyArray ? &mesh->polyArray[index] : mesh->polyList[index];
            }
        }
        else if (chunkId == 0xC040) {
            file.file->Read(&numNodes, 4);
            file.file->Read(&numLeafs, 4);
            nodeList = new zCBspNode[numNodes];
            leafList = new zCBspLeaf[numLeafs];
            bspFileNodeIndex = 1;
            bspFileLeafIndex = 0;
            actLeafPtr = leafList;
            bspRoot = nodeList;
            actNodePtr = nodeList + 1;
            nodeList->parent = NULL;
            bspRoot->LoadBINRec(file);
        }
        else if (chunkId == 0xC045) {
            for (int i = 0; i < numLeafs; ++i)
                file.file->Read(&leafList[i].lightPosition, sizeof(zVEC3));
        }
        else if (chunkId == 0xC050) {
            int numSectors;
            file.file->Read(&numSectors, 4);
            sectorList.AllocAbs(numSectors);
            for (int i = 0; i < numSectors; ++i) {
                zCBspSector* sector = new zCBspSector;
                sector->activated = -1;
                sector->rendered = -1;
                sector->sectorIndex = i;
                file.file->Read(sector->sectorName);
                int numNodesInSector;
                int numPortalsInSector;
                file.file->Read(&numNodesInSector, 4);
                file.file->Read(&numPortalsInSector, 4);
                sector->sectorNodes.AllocAbs(numNodesInSector);
                sector->sectorPortals.AllocAbs(numPortalsInSector);
                for (int j = 0; j < numNodesInSector; ++j) {
                    int index;
                    file.file->Read(&index, 4);
                    sector->sectorNodes.InsertEnd(leafList + index);
                }
                for (int j = 0; j < numPortalsInSector; ++j) {
                    int index;
                    file.file->Read(&index, 4);
                    sector->sectorPortals.InsertEnd(
                        mesh->polyArray ? &mesh->polyArray[index] : mesh->polyList[index]);
                }
                sectorList.InsertEnd(sector);
            }
            ConnectMaterialsToSectors();
            int numPortals;
            file.file->Read(&numPortals, 4);
            portalList.AllocAbs(numPortals);
            for (int i = 0; i < numPortals; ++i) {
                int index;
                file.file->Read(&index, 4);
                portalList.InsertEnd(
                    mesh->polyArray ? &mesh->polyArray[index] : mesh->polyList[index]);
            }
        }
        else if (chunkId == 0xC0FF) {
            file.file->Seek(chunkEnd);
            break;
        }
        file.file->Seek(chunkEnd);
    }
    return result;
}

void zCMesh::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

void zCBspTree::CalcStaticLeafLightData()
{
    for (int i = 0; i < numLeafs; ++i) {
        zCBspLeaf& leaf = leafList[i];
        const zVEC3 center = (leaf.bbox3D.mins + leaf.bbox3D.maxs) * 0.5f;
        float bestScore = -1.0f;
        zCVobLight* bestLight = NULL;
        for (int j = 0; j < leaf.lightVobList.numInArray; ++j) {
            zCVobLight* light = leaf.lightVobList.parray[j];
            const float distance = (light->position - center).Length();
            const float score = light->range / (distance + 1.0f);
            if (score > bestScore) {
                bestScore = score;
                bestLight = light;
            }
        }
        leaf.lightPosition = bestLight ? bestLight->position : zVEC3(-99.0f);
    }
}

void DrawBox(const zTBBox3D& box)
{
    zVEC3 corners[8];
    for (int i = 0; i < 8; ++i) {
        corners[i].n[0] = (i & 1) ? box.maxs.n[0] : box.mins.n[0];
        corners[i].n[1] = (i & 2) ? box.maxs.n[1] : box.mins.n[1];
        corners[i].n[2] = (i & 4) ? box.maxs.n[2] : box.mins.n[2];
    }
    static const int edges[12][2] = {
        { 0, 1 }, { 0, 2 }, { 1, 3 }, { 2, 3 },
        { 4, 5 }, { 4, 6 }, { 5, 7 }, { 6, 7 },
        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
    };
    for (int i = 0; i < 12; ++i)
        zlineCache.Line3D(corners[edges[i][0]], corners[edges[i][1]], 0xFFFFFF00, 0);
}

int zCRayTurboAdmin::s_iPolyTreshold = 500;

zCRayTurboAdmin::zCRayTurboAdmin()
{
    m_mapRayTurbos.AllocAbs(97);
    for (int i = 0; i < 97; ++i)
        m_mapRayTurbos.InsertEnd(NULL);
}

zCRayTurboAdmin::~zCRayTurboAdmin()
{
    for (int i = 0; i < m_mapRayTurbos.numInArray; ++i) {
        zSNode* node = m_mapRayTurbos.parray[i];
        while (node) {
            zSNode* next = node->next;
            delete node->value;
            delete node;
            node = next;
        }
    }
}

zCRayTurboAdmin& zCRayTurboAdmin::GetInstance()
{
    static zCRayTurboAdmin instance;
    return instance;
}

zCRayTurbo* zCRayTurboAdmin::GetRayTurboItem(const zCBspNode* node) const
{
    const unsigned long hash = (reinterpret_cast<unsigned long>(node) >> 4)
        % m_mapRayTurbos.numInArray;
    for (zSNode* entry = m_mapRayTurbos.parray[hash]; entry; entry = entry->next) {
        if (entry->key == node)
            return entry->value;
    }
    return NULL;
}

zCRayTurbo* zCRayTurboAdmin::CreateRayTurboItem(const zCBspNode* node)
{
    zCRayTurbo* turbo = new zCRayTurbo(*node);
    const unsigned long hash = (reinterpret_cast<unsigned long>(node) >> 4)
        % m_mapRayTurbos.numInArray;
    for (zSNode* entry = m_mapRayTurbos.parray[hash]; entry; entry = entry->next) {
        if (entry->key == node) {
            entry->value = turbo;
            return turbo;
        }
    }
    zSNode* entry = new zSNode;
    entry->key = node;
    entry->value = turbo;
    entry->hash = hash;
    entry->next = m_mapRayTurbos.parray[hash];
    m_mapRayTurbos.parray[hash] = entry;
    return turbo;
}

void zTBBox3D::Init()
{
    mins = zVEC3(3.402823466e38F);
    maxs = zVEC3(-3.402823466e38F);
}

zCRayTurbo* zCRayTurboAdmin::GetOrCreate(const zCBspNode* node)
{
    if (!CanHaveSpatialInfo(node))
        return NULL;
    zCRayTurbo* turbo = GetRayTurboItem(node);
    return turbo ? turbo : CreateRayTurboItem(node);
}

int zCRayTurboAdmin::CanHaveSpatialInfo(const zCBspNode* node)
{
    return s_iPolyTreshold > 0 && node->numPolys > s_iPolyTreshold;
}

void zCRayTurboAdmin::ReleaseSpatialDatabase()
{
    for (int i = 0; i < m_mapRayTurbos.numInArray; ++i) {
        zSNode* node = m_mapRayTurbos.parray[i];
        while (node) {
            zSNode* next = node->next;
            delete node->value;
            delete node;
            node = next;
        }
    }
    m_mapRayTurbos.numInArray = 0;
    for (int i = 0; i < 97; ++i)
        m_mapRayTurbos.InsertEnd(NULL);
}

void zCRayTurboAdmin::SetPolyTreshold(int value)
{
    s_iPolyTreshold = value;
}

int zCRayTurboAdmin::GetPolyTreshold()
{
    return s_iPolyTreshold;
}

zCRayTurbo::zCRayTurbo(const zCBspNode& node)
    : bspNode(&node)
{
    for (int i = 0; i < 8; ++i)
        meshOctreeRoot.children[i] = NULL;
    meshOctreeRoot.box.Init();
    meshOctreeRoot.flags = 1;
    zCArray<zCPolygon*> polygons;
    polygons.AllocAbs(node.numPolys);
    for (int i = 0; i < node.numPolys; ++i)
        polygons.InsertEnd(node.polyList[i]);
    meshOctreeRoot.Build(polygons, zCRayTurboAdmin::s_iPolyTreshold / 3);
}

zCRayTurbo::~zCRayTurbo()
{
    for (int i = 0; i < 8; ++i)
        delete meshOctreeRoot.children[i];
}

void zCRayTurbo::CalculateOptimizedPolyList(zCArray<zCPolygon*>& polygons)
{
    polygons.numInArray = 0;
    TraverseTree(polygons, meshOctreeRoot);
}

void zCRayTurbo::TraverseTree(zCArray<zCPolygon*>& result, const zCMeshOctreeNode& node)
{
    if ((node.flags & 4) && node.polygons.numInArray > 0) {
        for (int i = 0; i < node.polygons.numInArray; ++i)
            result.InsertEnd(node.polygons.parray[i]);
    }
    if (!(node.flags & 4)) {
        for (int i = 0; i < 8; ++i) {
            if (node.children[i])
                TraverseTree(result, *node.children[i]);
        }
    }
}

zCMeshOctreeNode::zCMeshOctreeNode(int root)
{
    for (int i = 0; i < 8; ++i)
        children[i] = NULL;
    box.Init();
    flags = root & 1;
}

zCMeshOctreeNode::~zCMeshOctreeNode()
{
    for (int i = 0; i < 8; ++i)
        delete children[i];
}

void zCMeshOctreeNode::EnlargeBox(const zCArray<zCPolygon*>& source)
{
    for (int i = 0; i < source.numInArray; ++i) {
        const zCPolygon* polygon = source.parray[i];
        for (int j = 0; j < polygon->polyNumVert; ++j) {
            const zVEC3& position = polygon->vertex[j]->position;
            for (int axis = 0; axis < 3; ++axis) {
                if (position.n[axis] - 0.01f < box.mins.n[axis])
                    box.mins.n[axis] = position.n[axis] - 0.01f;
                if (position.n[axis] + 0.01f > box.maxs.n[axis])
                    box.maxs.n[axis] = position.n[axis] + 0.01f;
            }
        }
    }
}

void zCMeshOctreeNode::CopyPolys(const zCArray<unsigned char>& mask,
    const zCArray<zCPolygon*>& source, int count, zCArray<zCPolygon*>& destination)
{
    destination.AllocAbs(count);
    for (int i = 0; i < source.numInArray; ++i) {
        if (mask.parray[i] == 1)
            destination.InsertEnd(source.parray[i]);
    }
}

void zCMeshOctreeNode::Build(const zCArray<zCPolygon*>& source, int threshold)
{
    if (flags & 1)
        EnlargeBox(source);

    zCArray<unsigned char> mask;
    zCArray<zCPolygon*> inside;
    mask.AllocAbs(source.numInArray);
    inside.AllocAbs(source.numInArray);
    for (int i = 0; i < source.numInArray; ++i) {
        const zVEC3 center = source.parray[i]->GetCenter();
        const int contained = center.n[0] >= box.mins.n[0] && center.n[0] <= box.maxs.n[0]
            && center.n[1] >= box.mins.n[1] && center.n[1] <= box.maxs.n[1]
            && center.n[2] >= box.mins.n[2] && center.n[2] <= box.maxs.n[2];
        mask.InsertEnd(contained ? 1 : 0);
        if (contained)
            inside.InsertEnd(source.parray[i]);
    }

    if (inside.numInArray <= 0) {
        flags |= 4;
        return;
    }
    if (inside.numInArray <= threshold) {
        flags |= 4;
        polygons = inside;
        for (int i = 0; i < polygons.numInArray; ++i) {
            const zCPolygon* polygon = polygons.parray[i];
            for (int j = 0; j < polygon->polyNumVert; ++j) {
                const zVEC3& position = polygon->vertex[j]->position;
                for (int axis = 0; axis < 3; ++axis) {
                    if (position.n[axis] < box.mins.n[axis])
                        box.mins.n[axis] = position.n[axis];
                    if (position.n[axis] > box.maxs.n[axis])
                        box.maxs.n[axis] = position.n[axis];
                }
            }
            for (int axis = 0; axis < 3; ++axis) {
                box.mins.n[axis] -= 0.01f;
                box.maxs.n[axis] += 0.01f;
            }
        }
        return;
    }

    flags |= 2;
    const float sizeX = box.maxs.n[0] - box.mins.n[0];
    const float sizeY = box.maxs.n[1] - box.mins.n[1];
    const float sizeZ = box.maxs.n[2] - box.mins.n[2];
    float cellSize = sizeX;
    if (sizeY > cellSize)
        cellSize = sizeY;
    if (sizeZ > cellSize)
        cellSize = sizeZ;
    cellSize *= 0.5f;

    for (int i = 0; i < 8; ++i) {
        children[i] = new zCMeshOctreeNode(0);
        for (int axis = 0; axis < 3; ++axis) {
            const int high = (i >> axis) & 1;
            children[i]->box.mins.n[axis] = box.mins.n[axis] + high * cellSize;
            children[i]->box.maxs.n[axis] = box.mins.n[axis] + (high + 1) * cellSize;
        }
        children[i]->Build(inside, threshold);
        for (int axis = 0; axis < 3; ++axis) {
            if (children[i]->box.mins.n[axis] < box.mins.n[axis])
                box.mins.n[axis] = children[i]->box.mins.n[axis];
            if (children[i]->box.maxs.n[axis] > box.maxs.n[axis])
                box.maxs.n[axis] = children[i]->box.maxs.n[axis];
        }
    }
}

zCCBspTree::zCCBspTree()
{
    front = NULL;
    back = NULL;
    parent = NULL;
    polyList.root = NULL;
    polyList.count = 0;
    mesh = new zCMesh;
    const int mode = 0;
    SetBspTreeMode(mode);
}

zCCBspNode::~zCCBspNode()
{
    zTPolyNode* node = polyList.root;
    while (node) {
        zTPolyNode* next = node->next;
        delete node;
        node = next;
    }
    polyList.root = NULL;
    polyList.count = 0;
    delete front;
    delete back;
    front = NULL;
    back = NULL;
    parent = NULL;
}

void zCCBspTree::SetBspTreeMode(const int& mode)
{
    bspTreeMode = mode;
    cfg_bspType = 0;
    cfg_spanningMode = 2;
    cfg_useAllPolys = 1;
    cfg_optimize = 0.5f;
    cfg_wbalance = 0.6f;
    cfg_wsplit = 0.2f;
    cfg_wplanar = 0.2f;
    cfg_portalize = 0;
    if (mode == 0)
        cfg_bspType = 1;
    cfg_spanningMode = 2;
    cfg_useAllPolys = 1;
    cfg_optimize = 1.0f;
}

zCCBspTree::~zCCBspTree()
{
    if (mesh && mesh->Release() <= 0)
        delete mesh;
    mesh = NULL;
    delete front;
    delete back;
    front = NULL;
    back = NULL;
    zTPolyNode* node = polyList.root;
    while (node) {
        zTPolyNode* next = node->next;
        delete node;
        node = next;
    }
    polyList.root = NULL;
    polyList.count = 0;
    parent = NULL;
}

void zCCBspTree::DeleteTree()
{
    delete front;
    delete back;
    front = NULL;
    back = NULL;
    zTPolyNode* node = polyList.root;
    while (node) {
        zTPolyNode* next = node->next;
        delete node;
        node = next;
    }
    polyList.root = NULL;
    polyList.count = 0;
}

void zCCBspTree::AddMesh(zCMesh* source)
{
    AddMesh(source, Alg_Identity3D());
}

void zCCBspTree::AddMesh(zCMesh* source, const zMAT4& trafo)
{
    if (mesh)
        mesh->MergeMesh(source, trafo);
}

void zCCBspTree::BuildTree(float optimize)
{
    for (int i = 0; i < mesh->numPoly; ++i) {
        zTPolyNode* node = new zTPolyNode;
        node->poly = mesh->polyList[i];
        node->marked = 0;
        node->isCompleteInside = 1;
        node->next = polyList.root;
        polyList.root = node;
        ++polyList.count;
    }

    cfg_optimize = optimize;
    if (cfg_optimize <= 0.0f)
        cfg_optimize = 0.00001f;
    zCCBspNode::BuildTree(0);
    CountNodes();
    CountLeafs();
    if (cfg_bspType != 1)
        CalcSubtreeBBox3D();
    else
        CalcSubtreeBBox3DCell();
    if (bspTreeMode == 1)
        OutdoorKillRedundantLeafs();
    int minDepth;
    int maxDepth;
    int avgDepth;
    CountDepth(minDepth, maxDepth, avgDepth);
    if (cfg_portalize)
        Portalize();
}

void zCCBspNode::RemovePoly(zCPolygon* polygon)
{
    zTPolyNode* previous = NULL;
    zTPolyNode* node = polyList.root;
    while (node && node->poly != polygon) {
        previous = node;
        node = node->next;
    }
    if (!node)
        return;
    if (previous)
        previous->next = node->next;
    else
        polyList.root = node->next;
    delete node;
    --polyList.count;
}

int zCCBspNode::DoesNodeContainPoly(zCPolygon* polygon)
{
    for (int i = 0; i < polygon->polyNumVert; ++i) {
        const zVEC3& position = polygon->vertex[i]->position;
        zCCBspNode* child = this;
        while (child && child->parent) {
            zCCBspNode* parentNode = child->parent;
            const float distance = parentNode->plane.normal * position - parentNode->plane.distance;
            if (parentNode->front == child && distance > 0.001f)
                return 0;
            if (parentNode->back == child && distance < -0.001f)
                return 0;
            child = parentNode;
        }
    }
    return 1;
}

int zCCBspNode::CheckPortalConsistency()
{
    int count = 0;
    for (zTPolyNode* node = polyList.root; node; node = node->next) {
        zCPolygon* polygon = node->poly;
        if (polygon && (polygon->flags[0] & 3)) {
            ++count;
            zCPortal* portal = static_cast<zCPortal*>(polygon);
            if (!portal->frontBspNode && !portal->backBspNode)
                continue;
        }
    }
    if (front)
        count += front->CheckPortalConsistency();
    if (back)
        count += back->CheckPortalConsistency();
    return count;
}

static zTPolyNode* UnlinkConstructionPoly(zCCBspNode* node, zTPolyNode* target)
{
    zTPolyNode* previous = NULL;
    zTPolyNode* current = node->polyList.root;
    while (current && current != target) {
        previous = current;
        current = current->next;
    }
    if (!current)
        return NULL;
    if (previous)
        previous->next = current->next;
    else
        node->polyList.root = current->next;
    --node->polyList.count;
    current->next = NULL;
    return current;
}

static void LinkConstructionPoly(zCCBspNode* node, zTPolyNode* polyNode)
{
    polyNode->next = node->polyList.root;
    node->polyList.root = polyNode;
    ++node->polyList.count;
}

static void AddConstructionPoly(zCCBspNode* node, zCPolygon* polygon)
{
    zTPolyNode* polyNode = new zTPolyNode;
    polyNode->poly = polygon;
    polyNode->marked = 0;
    polyNode->isCompleteInside = 1;
    LinkConstructionPoly(node, polyNode);
}

static void MovePortalLink(zCPortal* portal, zCCBspNode* oldNode, zCCBspNode* newNode)
{
    zCBspNode* castOld = reinterpret_cast<zCBspNode*>(oldNode);
    zCBspNode* castNew = reinterpret_cast<zCBspNode*>(newNode);
    if (portal->frontBspNode == castOld)
        portal->frontBspNode = castNew;
    else
        portal->backBspNode = castNew;
}

void zCCBspNode::SplitPortals()
{
    zTPolyNode* node = polyList.root;
    while (node) {
        zTPolyNode* next = node->next;
        zCPolygon* polygon = node->poly;
        if (polygon && (polygon->flags[0] & 3)) {
            zCPortal* portal = static_cast<zCPortal*>(polygon);
            const int side = polygon->ClassifyPlane(plane);
            if (side == 0 || side == 1) {
                zCCBspNode* destination = side == 0 ? front : back;
                if (!destination)
                    portal->RemoveFromBsp();
                else {
                    UnlinkConstructionPoly(this, node);
                    LinkConstructionPoly(destination, node);
                    MovePortalLink(portal, this, destination);
                }
            }
            else if (side == 2) {
                RemovePoly(polygon);
                portal->RemoveFromBsp();
                if (front)
                    AddConstructionPoly(front, polygon);
                if (back)
                    AddConstructionPoly(back, polygon);
                portal->frontBspNode = reinterpret_cast<zCBspNode*>(front);
                portal->backBspNode = reinterpret_cast<zCBspNode*>(back);
            }
            else if (side == 3) {
                zCPortal* frontPortal = new zCPortal(polygon->polyPlane, NULL);
                zCPortal* backPortal = new zCPortal(polygon->polyPlane, NULL);
                polygon->SplitPoly(plane, frontPortal, backPortal, NULL);
                RemovePoly(polygon);
                if (front && frontPortal->polyNumVert >= 3) {
                    MovePortalLink(frontPortal, this, front);
                    AddConstructionPoly(front, frontPortal);
                }
                else
                    delete frontPortal;
                if (back && backPortal->polyNumVert >= 3) {
                    MovePortalLink(backPortal, this, back);
                    AddConstructionPoly(back, backPortal);
                }
                else
                    delete backPortal;
                portal->RemoveFromBsp();
                delete portal;
            }
        }
        node = next;
    }
}

void zCPortal::operator delete(void* memory)
{
    ::operator delete(memory);
}

void zCPortal::Init()
{
    backBspNode = NULL;
    frontBspNode = NULL;
    flags[0] = (flags[0] & 0xFC) | 1;
    lastTimeCompletely = -1;
}

zCPortal* zCCBspNode::SplitPortalByAllPortals(zCPortal* portal)
{
    for (zTPolyNode* node = polyList.root; node; node = node->next) {
        zCPolygon* cutting = node->poly;
        if (!cutting || !(cutting->flags[0] & 3) || cutting == portal)
            continue;
        if (portal->ClassifyPlane(cutting->polyPlane) != 3)
            continue;

        zCPortal* frontPortal = new zCPortal(portal->polyPlane, NULL);
        zCPortal* backPortal = new zCPortal(portal->polyPlane, NULL);
        portal->SplitPoly(cutting->polyPlane, frontPortal, backPortal, NULL);
        zCPortal* cuttingPortal = static_cast<zCPortal*>(cutting);
        zCPortal* remainder = cuttingPortal->frontBspNode == reinterpret_cast<zCBspNode*>(this)
            ? frontPortal : backPortal;
        zCPortal* discarded = remainder == frontPortal ? backPortal : frontPortal;
        delete discarded;
        if (remainder->polyNumVert < 3)
            delete remainder;
        else {
            delete portal;
            portal = remainder;
        }
    }
    return portal;
}

void zCCBspNode::EnumeratePortals()
{
    if (!front && !back)
        return;
    zCPortal* portal = new zCPortal(plane, NULL);
    portal = SplitPortalByAllPortals(portal);
    SplitPortals();
    portal->frontBspNode = reinterpret_cast<zCBspNode*>(front);
    portal->backBspNode = reinterpret_cast<zCBspNode*>(back);
    if (front)
        AddConstructionPoly(front, portal);
    if (back)
        AddConstructionPoly(back, portal);
    if (front)
        front->EnumeratePortals();
    if (back)
        back->EnumeratePortals();
}

void zCCBspNode::ClassifyPortals()
{
    zCCBspNode* node = this;
    while (node) {
        if (node->front)
            node->front->ClassifyPortals();
        else if (!node->back) {
            zTPolyNode* polyNode = node->polyList.root;
            while (polyNode) {
                zTPolyNode* next = polyNode->next;
                zCPolygon* polygon = polyNode->poly;
                if ((polygon->flags[0] & 3)) {
                    zCPortal* portal = static_cast<zCPortal*>(polygon);
                    if (!portal->frontBspNode || !portal->backBspNode) {
                        portal->RemoveFromBsp();
                        delete portal;
                    }
                }
                polyNode = next;
            }
        }
        node = node->back;
    }
}

zCPortal::~zCPortal()
{
}

void zCCBspNode::Portalize()
{
    EnumeratePortals();
    CheckPortalConsistency();
    ClassifyPortals();
    CheckPortalConsistency();
}

static zCCBspTree* bspConstructionTree;

void zCCBspNode::SelectPlane(zTPlane& selectedPlane, zTPolyNode*& selected, int& valid)
{
    valid = 1;
    selected = NULL;
    zCArray<zTPolyNode*> candidates;
    for (zTPolyNode* node = polyList.root; node; node = node->next) {
        if (!node->marked)
            candidates.InsertEnd(node);
    }
    if (candidates.numInArray <= 0)
        return;

    const float optimize = bspConstructionTree->cfg_optimize;
    int stride = optimize > 0.0f ? static_cast<int>(1.0f / optimize) : 1;
    if (stride < 1)
        stride = 1;
    float bestScore = 3.402823466e38F;
    zTPlane bestPlane;
    zTPolyNode* bestNode = NULL;
    for (int i = 0; i < candidates.numInArray; i += stride) {
        zTPlane candidatePlane = candidates.parray[i]->poly->polyPlane;
        int frontCount = 0;
        int backCount = 0;
        int bothCount = 0;
        int splitCount = 0;
        for (int j = 0; j < candidates.numInArray; ++j) {
            if (i == j)
                continue;
            switch (candidates.parray[j]->poly->ClassifyPlane(candidatePlane)) {
            case 0: ++frontCount; break;
            case 1: ++backCount; break;
            case 2: ++bothCount; break;
            case 3: ++splitCount; break;
            }
        }
        if (splitCount)
            valid = 0;
        const float invCount = 1.0f / candidates.numInArray;
        const float score = fabs(static_cast<float>(frontCount - backCount)) * invCount
                * bspConstructionTree->cfg_wbalance
            + splitCount * invCount * bspConstructionTree->cfg_wsplit
            + bothCount * invCount * bspConstructionTree->cfg_wplanar;
        if (score <= 0.0f)
            break;
        if (score < bestScore) {
            bestScore = score;
            bestPlane = candidatePlane;
            bestNode = candidates.parray[i];
        }
    }
    if (bestNode) {
        selectedPlane = bestPlane;
        selected = bestNode;
    }
}

void zCCBspNode::SelectPlaneOctree(zCCBspTree* tree, zTPlane& selectedPlane, int level)
{
    if (parent) {
        bbox3DCell = parent->bbox3DCell;
        int axis = 0;
        if (parent->plane.normal.n[0] == 0.0f)
            axis = parent->plane.normal.n[1] == 0.0f ? 2 : 1;
        const float middle = (bbox3DCell.mins.n[axis] + bbox3DCell.maxs.n[axis]) * 0.5f;
        if (this == parent->front)
            bbox3DCell.mins.n[axis] = middle;
        else
            bbox3DCell.maxs.n[axis] = middle;
    }

    int axis = 0;
    const float sizeX = bbox3DCell.maxs.n[0] - bbox3DCell.mins.n[0];
    const float sizeY = bbox3DCell.maxs.n[1] - bbox3DCell.mins.n[1];
    const float sizeZ = bbox3DCell.maxs.n[2] - bbox3DCell.mins.n[2];
    if (tree->cfg_bspType == 1 && sizeY > sizeX)
        axis = 1;
    if (sizeZ > (axis == 0 ? sizeX : sizeY))
        axis = 2;
    selectedPlane.normal = zVEC3(0.0f);
    selectedPlane.normal.n[axis] = 1.0f;
    const zVEC3 point = (bbox3DCell.mins + bbox3DCell.maxs) * 0.5f;
    selectedPlane.distance = selectedPlane.normal * point;
}

int zCCBspNode::ClassifyRayToNode(const zVEC3& start, const zVEC3& end)
{
    const float startDistance = plane.normal * start - plane.distance;
    const float endDistance = plane.normal * end - plane.distance;
    if (startDistance <= 0.0f && endDistance <= 0.0f)
        return 0;
    if (startDistance >= 0.0f && endDistance >= 0.0f)
        return 1;
    return 3;
}

int zCCBspNode::ClassifyPolyToNode2(zCPolygon* polygon)
{
    return polygon->ClassifyPlane(plane);
}

int zCCBspNode::OutdoorKillRedundantLeafs()
{
    int killed = 0;
    if (front && back && !front->front && !front->back && !back->front && !back->back
        && front->polyList.count == back->polyList.count
        && front->bbox3D.mins == back->bbox3D.mins && front->bbox3D.maxs == back->bbox3D.maxs) {
        bbox3D = front->bbox3D;
        for (zTPolyNode* node = front->polyList.root; node; node = node->next)
            AddPoly(node->poly);
        delete front;
        delete back;
        front = NULL;
        back = NULL;
        killed = 1;
    }
    if (front)
        killed += front->OutdoorKillRedundantLeafs();
    if (back)
        killed += back->OutdoorKillRedundantLeafs();
    return killed;
}

void zCCBspNode::AddPoly(zCPolygon* polygon)
{
    AddConstructionPoly(this, polygon);
}

void zCCBspNode::BuildTree(int level)
{
    if (!parent)
        bspConstructionTree = static_cast<zCCBspTree*>(this);
    if (polyList.count <= 1)
        return;

    zTPolyNode* selected = NULL;
    int valid = 1;
    if (bspConstructionTree->cfg_bspType == 2)
        SelectPlane(plane, selected, valid);
    else {
        SelectPlaneOctree(bspConstructionTree, plane, level);
        bbox3D = bbox3DCell;
        if (polyList.count <= 4)
            return;
    }
    if (bspConstructionTree->cfg_bspType == 2 && !selected)
        return;

    int moved = 0;
    zTPolyNode* node = polyList.root;
    while (node) {
        zTPolyNode* next = node->next;
        zCPolygon* polygon = node->poly;
        const int side = polygon->ClassifyPlane(plane);
        if (side == 0 || side == 1) {
            if (!front)
                front = new zCCBspNode(this);
            if (!back)
                back = new zCCBspNode(this);
            zCCBspNode* destination = side == 0 ? front : back;
            UnlinkConstructionPoly(this, node);
            LinkConstructionPoly(destination, node);
            moved = 1;
        }
        else if (side == 2 && bspConstructionTree->cfg_spanningMode == 2) {
            if (!front)
                front = new zCCBspNode(this);
            if (!back)
                back = new zCCBspNode(this);
            UnlinkConstructionPoly(this, node);
            LinkConstructionPoly(front, node);
            AddPoly(back, polygon);
            moved = 1;
        }
        else if (side == 3) {
            zCPolygon* frontPoly = new zCPolygon;
            zCPolygon* backPoly = new zCPolygon;
            polygon->SplitPoly(plane, frontPoly, backPoly, bspConstructionTree->mesh);
            if (frontPoly->polyNumVert >= 3 && backPoly->polyNumVert >= 3) {
                if (!front)
                    front = new zCCBspNode(this);
                if (!back)
                    back = new zCCBspNode(this);
                bspConstructionTree->mesh->AddPoly(frontPoly);
                bspConstructionTree->mesh->AddPoly(backPoly);
                AddPoly(front, frontPoly);
                AddPoly(back, backPoly);
                RemovePoly(polygon);
                moved = 1;
            }
            else {
                delete frontPoly;
                delete backPoly;
            }
        }
        node = next;
    }

    if (moved) {
        if (front)
            front->BuildTree(level + 1);
        if (back)
            back->BuildTree(level + 1);
    }
    bbox3D.Init();
    for (node = polyList.root; node; node = node->next) {
        zCPolygon* polygon = node->poly;
        if (polygon->flags[0] & 0x40)
            continue;
        for (int i = 0; i < polygon->polyNumVert; ++i)
            bbox3D.AddPoint(polygon->vertex[i]->position);
    }
}

void zCPolygon::operator delete(void* memory)
{
    ::operator delete(memory);
}

zCCBspNode::zCCBspNode(zCCBspNode* parentNode)
{
    polyList.root = NULL;
    polyList.count = 0;
    back = NULL;
    front = NULL;
    parent = parentNode;
}

void zCCBspNode::PushPolyFront(zTPolyNode* node)
{
    if (!front)
        front = new zCCBspNode(this);
    UnlinkConstructionPoly(this, node);
    LinkConstructionPoly(front, node);
}

void zCCBspNode::CalcBBox3D()
{
    bbox3D.Init();
    for (zTPolyNode* node = polyList.root; node; node = node->next) {
        zCPolygon* polygon = node->poly;
        if (polygon->flags[0] & 0x40)
            continue;
        for (int i = 0; i < polygon->polyNumVert; ++i)
            bbox3D.AddPoint(polygon->vertex[i]->position);
    }
}

static void IncludeBBox(zTBBox3D& destination, const zTBBox3D& source)
{
    for (int axis = 0; axis < 3; ++axis) {
        if (source.mins.n[axis] < destination.mins.n[axis])
            destination.mins.n[axis] = source.mins.n[axis];
        if (source.maxs.n[axis] > destination.maxs.n[axis])
            destination.maxs.n[axis] = source.maxs.n[axis];
    }
}

void zCCBspNode::CalcSubtreeBBox3D()
{
    if (front)
        front->CalcSubtreeBBox3D();
    if (back)
        back->CalcSubtreeBBox3D();
    CalcBBox3D();
    if (front)
        IncludeBBox(bbox3D, front->bbox3D);
    if (back)
        IncludeBBox(bbox3D, back->bbox3D);
}

void zCCBspNode::CalcSubtreeBBox3DCell()
{
    zCCBspNode* node = this;
    while (node) {
        if (node->parent) {
            node->bbox3DCell = node->parent->bbox3DCell;
            int axis = 0;
            if (node->parent->plane.normal.n[0] == 0.0f)
                axis = node->parent->plane.normal.n[1] == 0.0f ? 2 : 1;
            const float middle = (node->bbox3DCell.mins.n[axis]
                + node->bbox3DCell.maxs.n[axis]) * 0.5f;
            if (node == node->parent->front)
                node->bbox3DCell.mins.n[axis] = middle;
            else
                node->bbox3DCell.maxs.n[axis] = middle;
            node->bbox3D = node->bbox3DCell;
        }
        if (node->front)
            node->front->CalcSubtreeBBox3DCell();
        node = node->back;
    }
}

int zCCBspNode::CountNodes() const
{
    int count = 1;
    if (front)
        count += front->CountNodes();
    if (back)
        count += back->CountNodes();
    return count;
}

int zCCBspNode::CountLeafs() const
{
    int count = 0;
    if (front)
        count += front->CountLeafs();
    if (back)
        count += back->CountLeafs();
    if (!front && !back)
        ++count;
    return count;
}

int zCCBspNode::CountPolys() const
{
    int count = polyList.count;
    if (front)
        count += front->CountPolys();
    if (back)
        count += back->CountPolys();
    return count;
}

void zCCBspNode::CountDepthRec(int depth, int& minDepth, int& maxDepth, int& sum)
{
    if (front)
        front->CountDepthRec(depth + 1, minDepth, maxDepth, sum);
    if (back)
        back->CountDepthRec(depth + 1, minDepth, maxDepth, sum);
    if (!front && !back) {
        if (depth > maxDepth)
            maxDepth = depth;
        if (depth < minDepth)
            minDepth = depth;
        sum += depth;
    }
}

void zCCBspNode::CountDepth(int& minDepth, int& maxDepth, int& avgDepth)
{
    minDepth = 999999;
    maxDepth = -1;
    int sum = 0;
    if (front)
        front->CountDepthRec(1, minDepth, maxDepth, sum);
    if (back)
        back->CountDepthRec(1, minDepth, maxDepth, sum);
    if (!front && !back) {
        minDepth = 0;
        maxDepth = 0;
    }
    const int leafs = CountLeafs();
    avgDepth = sum / leafs;
}

void zCCBspNode::DescribeTree(int depth)
{
    zSTRING text = "D: CBSP: " + Spaces(depth * 2);
    text += (front || back) ? "- Node, " : "- Leaf, ";
    text += zSTRING(polyList.count) + " Polys";
    if (front)
        text += ", [front]";
    if (back)
        text += " [back]";
    zerr->Message(text);
    if (front)
        front->DescribeTree(depth + 1);
    if (back)
        back->DescribeTree(depth + 1);
}

zCBuildPortal::zCBuildPortal()
{
    portalPoly = NULL;
    portalPolyBack = NULL;
    sector[0] = NULL;
    sector[1] = NULL;
}

zCBuildPortal::~zCBuildPortal()
{
    portalPoly = NULL;
    portalPolyBack = NULL;
    sector[0] = NULL;
    sector[1] = NULL;
}

static void SetBuildPortalId(zCPolygon* polygon, int id)
{
    polygon->flags[1] = id & 0xFF;
    polygon->flags[2] = (id >> 8) & 0xFF;
}

static int GetBuildPortalId(const zCPolygon* polygon)
{
    return polygon->flags[1] | (polygon->flags[2] << 8);
}

void zCBspTree::PreprocessIndoorPortals(zCMesh* sourceMesh, zCArray<zCBuildPortal*>& portals)
{
    portalList.numInArray = 0;
    portalList.AllocAbs(portals.numInArray * 2);
    for (int i = 0; i < sourceMesh->numPoly; ++i) {
        zCPolygon* polygon = sourceMesh->polyList[i];
        zCMaterial* material = polygon->material;
        polygon->flags[0] &= 0xF4;
        SetBuildPortalId(polygon, 0xFFFF);
        if (!material || material->objectName.Search("PORTAL") == -1)
            continue;

        polygon->flags[0] = (polygon->flags[0] & 0xFC) | 2;
        SetBuildPortalId(polygon, 0x8000 | portals.numInArray);
        material->rndAlphaBlendFunc = 2;
        zCBuildPortal* buildPortal = new zCBuildPortal;
        buildPortal->portalPoly = polygon;
        portals.InsertEnd(buildPortal);

        zCPolygon* back = new zCPolygon;
        back->SetMaterial(material);
        back->flags[0] = (back->flags[0] & 0xFC) | (polygon->flags[0] & 3);
        back->AllocVerts(polygon->polyNumVert);
        for (int j = 0; j < polygon->polyNumVert; ++j) {
            const int reverse = polygon->polyNumVert - j - 1;
            back->SetVertex(j, polygon->vertex[reverse]);
            back->SetFeature(j, polygon->feature[reverse]);
        }
        back->CalcNormal();
        sourceMesh->AddPoly(back);
        buildPortal->portalPolyBack = back;
        portalList.InsertEnd(polygon);
        portalList.InsertEnd(back);
    }
}

void zCBspTree::GetPolyNeighbours(zCPolygon* polygon, zCPolygon**& polygons, int& count)
{
    zTBBox3D box = polygon->GetBBox3D();
    box.Scale(1.1f);
    bspRoot->CollectPolysInBBox3D(box, polygons, count);
    int output = 0;
    for (int i = 0; i < count; ++i) {
        zCPolygon* other = polygons[i];
        if (other == polygon || (other->flags[0] & 3))
            continue;
        int known = 0;
        for (int j = 0; j < output; ++j) {
            if (polygons[j] == other) {
                known = 1;
                break;
            }
        }
        if (!known)
            polygons[output++] = other;
    }
    count = output;
}

static zCBspSector* CreateIndoorSector(zCBspTree* tree)
{
    zCBspSector* sector = new zCBspSector;
    sector->activated = -1;
    sector->rendered = -1;
    sector->hasBigNoFade = 0;
    sector->sectorIndex = tree->sectorList.numInArray;
    tree->sectorList.InsertEnd(sector);
    return sector;
}

void zCBspTree::PostprocessIndoorPortals(zCArray<zCBuildPortal*>& portals)
{
    for (int i = 0; i < portals.numInArray; ++i) {
        zCBuildPortal* buildPortal = portals.parray[i];
        zCPolygon* polygon = buildPortal->portalPoly;
        zCPolygon** neighbours;
        int count;
        GetPolyNeighbours(polygon, neighbours, count);
        for (int j = 0; j < count; ++j) {
            zCPolygon* other = neighbours[j];
            int border = polygon->IsIntersecting(other) != 0;
            if (!border) {
                for (int k = 0; k < polygon->polyNumVert; ++k) {
                    if (other->VertPartOfPoly(polygon->vertex[k])) {
                        border = 1;
                        break;
                    }
                }
            }
            if (!border)
                continue;
            SetBuildPortalId(other, 0x8000 | i);
            buildPortal->borderPolyList.InsertEnd(other);
            const int side = polygon->polyPlane.normal * other->GetCenter()
                    - polygon->polyPlane.distance < 0.0f ? 0 : 1;
            buildPortal->polyBucket[side].InsertEnd(other);
        }

        for (int j = 0; j < portals.numInArray; ++j) {
            if (i == j)
                continue;
            zCBuildPortal* other = portals.parray[j];
            for (int k = 0; k < other->borderPolyList.numInArray; ++k) {
                zCPolygon* border = other->borderPolyList.parray[k];
                int sharesEdge = 0;
                for (int v = 0; v < polygon->polyNumVert; ++v) {
                    const int next = (v + 1) % polygon->polyNumVert;
                    if (border->EdgePartOfPoly(polygon->vertex[v], polygon->vertex[next])) {
                        sharesEdge = 1;
                        break;
                    }
                }
                if (sharesEdge) {
                    const int side = polygon->polyPlane.normal * border->GetCenter()
                            - polygon->polyPlane.distance < 0.0f ? 0 : 1;
                    buildPortal->portalBucket[side].InsertEnd(other);
                    break;
                }
            }
        }
    }

    for (int i = 0; i < portals.numInArray; ++i) {
        zCBuildPortal* buildPortal = portals.parray[i];
        for (int side = 0; side < 2; ++side) {
            if (buildPortal->sector[side])
                continue;
            zCBspSector* sector = CreateIndoorSector(this);
            buildPortal->sector[side] = sector;
            sector->sectorPortals.InsertEnd(side == 0
                ? buildPortal->portalPoly : buildPortal->portalPolyBack);
            for (int j = 0; j < buildPortal->polyBucket[side].numInArray; ++j)
                SetBuildPortalId(buildPortal->polyBucket[side].parray[j], sector->sectorIndex);
        }
        buildPortal->portalPoly->material->bspSectorFront = buildPortal->sector[0];
        buildPortal->portalPoly->material->bspSectorBack = buildPortal->sector[1];
    }

    for (int i = 0; i < numLeafs; ++i) {
        zCBspLeaf* leaf = &leafList[i];
        for (int j = 0; j < leaf->numPolys; ++j) {
            const int sectorIndex = GetBuildPortalId(leaf->polyList[j]);
            if ((sectorIndex & 0x8000) || sectorIndex == 0xFFFF
                || sectorIndex >= sectorList.numInArray)
                continue;
            zCBspSector* sector = sectorList.parray[sectorIndex];
            int known = 0;
            for (int k = 0; k < sector->sectorNodes.numInArray; ++k) {
                if (sector->sectorNodes.parray[k] == leaf) {
                    known = 1;
                    break;
                }
            }
            if (!known)
                sector->sectorNodes.InsertEnd(leaf);
        }
    }

    for (int i = 0; i < portals.numInArray; ++i)
        delete portals.parray[i];
    portals.numInArray = 0;
}

int zTBCylinder3D::IsIntersecting(const zTBCylinder3D& other) const
{
    if (center.n[1] - heightY > other.center.n[1] + other.heightY)
        return 0;
    if (center.n[1] + heightY < other.center.n[1] - other.heightY)
        return 0;
    const float deltaX = center.n[0] - other.center.n[0];
    const float deltaZ = center.n[2] - other.center.n[2];
    const float radiusSum = radius + other.radius;
    return deltaX * deltaX + deltaZ * deltaZ <= radiusSum * radiusSum;
}

void zTBCylinder3D::Draw(const zCOLOR& color) const
{
    zTBBox3D box;
    box.mins = zVEC3(center.n[0] - radius, center.n[1] - heightY, center.n[2] - radius);
    box.maxs = zVEC3(center.n[0] + radius, center.n[1] + heightY, center.n[2] + radius);
    box.Draw(color);
    zTBSphere3D sphere;
    sphere.center = center;
    sphere.radius = radius;
    sphere.Draw(color);
}

void zTBCylinder3D::SetByBBox3D(zTBBox3D box)
{
    center = (box.mins + box.maxs) * 0.5f;
    heightY = (box.maxs.n[1] - box.mins.n[1]) * 0.5f;
    float extent = 3.402823466e38F;
    for (int axis = 0; axis < 3; ++axis) {
        const float value = box.maxs.n[axis] - box.mins.n[axis];
        if (value < extent)
            extent = value;
    }
    radius = extent * 0.5f;
}

int zTBSphere3D::IsIntersecting(const zVEC3& start, const zVEC3& end) const
{
    const zVEC3 direction = end - start;
    const zVEC3 relative = center - start;
    const float directionLength2 = direction.Length2();
    float projection = relative * direction;
    if (directionLength2 > 0.0f)
        projection /= directionLength2;
    if (projection < 0.0f)
        projection = 0.0f;
    else if (projection > 1.0f)
        projection = 1.0f;
    const zVEC3 nearest = relative - direction * projection;
    return nearest.Length2() <= radius * radius;
}

int zTBSphere3D::IsIntersecting(const zTBSphere3D& other) const
{
    float values[3] = {
        fabs(other.center.n[0] + center.n[0]),
        fabs(other.center.n[1] + center.n[1]),
        fabs(other.center.n[2] + center.n[2])
    };
    if (values[0] < values[1]) {
        const float temporary = values[0];
        values[0] = values[1];
        values[1] = temporary;
    }
    if (values[0] < values[2]) {
        const float temporary = values[0];
        values[0] = values[2];
        values[2] = temporary;
    }
    const float remaining = values[1] + values[2];
    const float distance = values[0] - values[0] * 0.0625f
        + remaining * 0.25f + remaining * 0.125f;
    return other.radius + radius >= distance;
}

int zTBSphere3D::ClassifyPlane(zTPlane plane) const
{
    const float distance = plane.normal * center - plane.distance;
    if (distance < -radius)
        return 1;
    if (distance > radius)
        return 2;
    return 0;
}

void zTBSphere3D::Draw(const zCOLOR& color) const
{
    if (!zCCamera::activeCam)
        return;
    zCCamera::activeCam->Activate();
    const zVEC3 cameraCenter = zCCamera::activeCam->camMatrix * center;
    if (cameraCenter.n[2] <= 0.0f)
        return;
    float centerX;
    float centerY;
    zCCamera::activeCam->Project(&center, centerX, centerY);
    const float radiusPixels = radius * zCCamera::activeCam->viewDistanceX / cameraCenter.n[2];
    int segments = static_cast<int>(radiusPixels * 0.5f);
    if (segments < 12)
        segments = 12;
    float previousX = centerX + radiusPixels;
    float previousY = centerY;
    for (int i = 1; i <= segments; ++i) {
        const float angle = 6.28318548f * i / segments;
        const float x = centerX + cos(angle) * radiusPixels;
        const float y = centerY + sin(angle) * radiusPixels;
        zlineCache.Line(static_cast<int>(previousX), static_cast<int>(previousY),
            static_cast<int>(x), static_cast<int>(y), color);
        previousX = x;
        previousY = y;
    }
}

void zTBBox3D::SetMaximumBox()
{
    mins = zVEC3(-3.402823466e38F);
    maxs = zVEC3(3.402823466e38F);
}

float zTBBox3D::GetVolume() const
{
    return (maxs.n[0] - mins.n[0]) * (maxs.n[1] - mins.n[1]) * (maxs.n[2] - mins.n[2]);
}

void zTBBox3D::CalcGreaterBBox3D(const zTBBox3D& other)
{
    for (int axis = 0; axis < 3; ++axis) {
        if (other.mins.n[axis] < mins.n[axis])
            mins.n[axis] = other.mins.n[axis];
        if (other.maxs.n[axis] > maxs.n[axis])
            maxs.n[axis] = other.maxs.n[axis];
    }
}

void zTBBox3D::ClipToBBox3D(const zTBBox3D& other)
{
    for (int axis = 0; axis < 3; ++axis) {
        if (other.mins.n[axis] > mins.n[axis])
            mins.n[axis] = other.mins.n[axis];
        if (other.maxs.n[axis] < maxs.n[axis])
            maxs.n[axis] = other.maxs.n[axis];
    }
}

zSTRING zTBBox3D::GetDescription() const
{
    zSTRING result;
    result.Sprintf("%g %g %g %g %g %g", mins.n[0], mins.n[1], mins.n[2],
        maxs.n[0], maxs.n[1], maxs.n[2]);
    return result;
}

void zTBBox3D::SetByDescription(zSTRING& description)
{
    for (int i = 0; i < 6; ++i) {
        const zSTRING word = description.PickWord_Old(i + 1, " ");
        const float value = static_cast<float>(atof(word.ToChar()));
        if (i < 3)
            mins.n[i] = value;
        else
            maxs.n[i - 3] = value;
    }
}

void zTBBox3D::Transform(const zMAT4& trafo, zTBBox3D& result) const
{
    result.Init();
    for (int i = 0; i < 8; ++i) {
        const zVEC3 point((i & 1) ? maxs.n[0] : mins.n[0],
            (i & 2) ? maxs.n[1] : mins.n[1], (i & 4) ? maxs.n[2] : mins.n[2]);
        result.AddPoint(trafo * point);
    }
}

void zTBBox3D::UnTransform(const zMAT4& trafo, zTBBox3D& result) const
{
    Transform(trafo.Inverse(), result);
}

int zTBBox3D::IsIntersecting(const zVEC3& start, const zVEC3& direction,
    float& nearTime, float& farTime) const
{
    nearTime = -3.402823466e38F;
    farTime = 3.402823466e38F;
    for (int axis = 0; axis < 3; ++axis) {
        if (fabs(direction.n[axis]) < 0.0001f) {
            if (start.n[axis] < mins.n[axis] || start.n[axis] > maxs.n[axis])
                return 0;
            continue;
        }
        float first = (mins.n[axis] - start.n[axis]) / direction.n[axis];
        float second = (maxs.n[axis] - start.n[axis]) / direction.n[axis];
        if (first > second) {
            const float temporary = first;
            first = second;
            second = temporary;
        }
        if (first > nearTime)
            nearTime = first;
        if (second < farTime)
            farTime = second;
        if (nearTime > farTime)
            return 0;
    }
    return 1;
}

int zTBBox3D::IsIntersectingSweep(const zVEC3& direction, const zTBBox3D& other,
    float& time) const
{
    time = 2.0f;
    zTBBox3D expanded = other;
    const zVEC3 center = (mins + maxs) * 0.5f;
    const zVEC3 extent = (maxs - mins) * 0.5f;
    expanded.mins = expanded.mins - extent;
    expanded.maxs += extent;
    float nearTime;
    float farTime;
    if (!expanded.IsIntersecting(center, direction, nearTime, farTime)
        || farTime < 0.0f || nearTime > 1.0f)
        return 0;
    time = nearTime > 0.0f ? nearTime : 0.0f;
    return 1;
}

int zTBBox3D::IsIntersectingSweep(const zTBBox3D& endBox, const zTBBox3D& otherStart,
    const zTBBox3D& otherEnd, float& time) const
{
    const zVEC3 direction = (endBox.mins - mins) - (otherEnd.mins - otherStart.mins);
    return IsIntersectingSweep(direction, otherStart, time);
}

int zTBBox3D::IsTrivIn(const zTBBox3D& other) const
{
    for (int axis = 0; axis < 3; ++axis) {
        if (other.mins.n[axis] < mins.n[axis] || other.maxs.n[axis] > maxs.n[axis])
            return 0;
    }
    return 1;
}

int zTBBox3D::IsTrivInLine(const zVEC3& start, const zVEC3& end) const
{
    for (int axis = 0; axis < 3; ++axis) {
        if (start.n[axis] < mins.n[axis] || start.n[axis] > maxs.n[axis]
            || end.n[axis] < mins.n[axis] || end.n[axis] > maxs.n[axis])
            return 0;
    }
    return 1;
}

int zTBBox3D::Classify(const zTBBox3D& other) const
{
    if (IsTrivIn(other))
        return 0;
    for (int axis = 0; axis < 3; ++axis) {
        if (other.maxs.n[axis] < mins.n[axis] || other.mins.n[axis] > maxs.n[axis])
            return 1;
    }
    return 2;
}

zVEC3 zTBBox3D::GetNearestPoint(const zTPlane* plane) const
{
    float bestDistance = 3.402823466e38F;
    zVEC3 bestPoint;
    for (int i = 0; i < 8; ++i) {
        const zVEC3 point((i & 1) ? maxs.n[0] : mins.n[0],
            (i & 2) ? maxs.n[1] : mins.n[1], (i & 4) ? maxs.n[2] : mins.n[2]);
        const float distance = fabs(plane->normal * point - plane->distance);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestPoint = point;
        }
    }
    return bestPoint;
}

void zTBBox3D::Scale(float value)
{
    for (int axis = 0; axis < 3; ++axis) {
        const float center = (maxs.n[axis] + mins.n[axis]) * 0.5f;
        const float extent = (maxs.n[axis] - mins.n[axis]) * value * 0.5f;
        mins.n[axis] = center - extent;
        maxs.n[axis] = center + extent;
    }
}

void zTBBox3D::Scale(const zVEC3& value)
{
    for (int axis = 0; axis < 3; ++axis) {
        const float center = (maxs.n[axis] + mins.n[axis]) * 0.5f;
        const float extent = (maxs.n[axis] - mins.n[axis]) * value.n[axis] * 0.5f;
        mins.n[axis] = center - extent;
        maxs.n[axis] = center + extent;
    }
}

float zTBBox3D::GetMinExtent() const
{
    float result = 3.402823466e38F;
    for (int axis = 0; axis < 3; ++axis) {
        const float extent = maxs.n[axis] - mins.n[axis];
        if (extent < result)
            result = extent;
    }
    return result * 0.5f;
}

zTBSphere3D zTBBox3D::GetSphere3D() const
{
    zTBSphere3D result;
    result.center = (mins + maxs) * 0.5f;
    result.radius = GetMinExtent();
    return result;
}

int zTBBox3D::TraceRay(const zVEC3& start, const zVEC3& direction, zVEC3& intersection) const
{
    float nearTime;
    float farTime;
    if (!IsIntersecting(start, direction, nearTime, farTime))
        return 0;
    float time = nearTime >= 0.0f ? nearTime : farTime;
    if (time < 0.0f || time > 1.0f)
        return 0;
    intersection = start + direction * time;
    return 1;
}

void zTBBox3D::Draw(const zCOLOR& color)
{
    zVEC3 corners[8];
    for (int i = 0; i < 8; ++i) {
        corners[i].n[0] = (i & 1) ? maxs.n[0] : mins.n[0];
        corners[i].n[1] = (i & 2) ? maxs.n[1] : mins.n[1];
        corners[i].n[2] = (i & 4) ? maxs.n[2] : mins.n[2];
    }
    static const int edges[12][2] = {
        { 0, 1 }, { 0, 2 }, { 1, 3 }, { 2, 3 },
        { 4, 5 }, { 4, 6 }, { 5, 7 }, { 6, 7 },
        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
    };
    for (int i = 0; i < 12; ++i)
        zlineCache.LineCS3D(corners[edges[i][0]], corners[edges[i][1]], color);
}

void zTBBox3D::GetCornerPoints(zVEC3* points) const
{
    points[0] = zVEC3(mins.n[0], mins.n[1], mins.n[2]);
    points[1] = zVEC3(maxs.n[0], maxs.n[1], maxs.n[2]);
    points[2] = zVEC3(mins.n[0], mins.n[1], maxs.n[2]);
    points[3] = zVEC3(mins.n[0], maxs.n[1], mins.n[2]);
    points[4] = zVEC3(mins.n[0], maxs.n[1], maxs.n[2]);
    points[5] = zVEC3(maxs.n[0], mins.n[1], mins.n[2]);
    points[6] = zVEC3(maxs.n[0], mins.n[1], maxs.n[2]);
    points[7] = zVEC3(maxs.n[0], maxs.n[1], mins.n[2]);
}

float zTBBox3D::GetScreenSize() const
{
    if (!zCCamera::activeCam)
        return 3.402823466e38F;
    zTBBox3D cameraBox;
    Transform(zCCamera::activeCam->camMatrix, cameraBox);
    zVEC3 points[8];
    cameraBox.GetCornerPoints(points);
    float minX = 999999.0f;
    float maxX = -999999.0f;
    float minY = 999999.0f;
    float maxY = -999999.0f;
    for (int i = 0; i < 8; ++i) {
        if (points[i].n[2] <= 0.0f)
            return 3.402823466e38F;
        const float inverseZ = 1.0f / points[i].n[2];
        const float x = points[i].n[0] * zCCamera::activeCam->viewDistanceX * inverseZ
            + zCCamera::activeCam->vpData.xcenter;
        const float y = zCCamera::activeCam->vpData.ycenter
            - points[i].n[1] * zCCamera::activeCam->viewDistanceY * inverseZ;
        if (x < minX)
            minX = x;
        if (x > maxX)
            maxX = x;
        if (y < minY)
            minY = y;
        if (y > maxY)
            maxY = y;
    }
    return (maxX - minX) * (maxY - minY);
}

int zTBBox3D::ClassifyToPlane(const zTPlane& plane) const
{
    zVEC3 negative;
    zVEC3 positive;
    for (int axis = 0; axis < 3; ++axis) {
        if (plane.normal.n[axis] >= 0.0f) {
            negative.n[axis] = mins.n[axis];
            positive.n[axis] = maxs.n[axis];
        }
        else {
            negative.n[axis] = maxs.n[axis];
            positive.n[axis] = mins.n[axis];
        }
    }
    if (plane.normal * negative - plane.distance < 0.0f)
        return 1;
    if (plane.normal * positive - plane.distance >= 0.0f)
        return 0;
    return 3;
}

int zTBBox3D::ClassifyToPlane(const zTPlane& plane, int signBits) const
{
    if (signBits > 7)
        return 0;
    zVEC3 points[8];
    GetCornerPoints(points);
    const float first = plane.normal * points[signBits] - plane.distance;
    if (first < 0.0f)
        return 1;
    const float second = plane.normal * points[7 - signBits] - plane.distance;
    if (second < 0.0f)
        return 3;
    return 0;
}

int zTBBox3D::ClassifyToPlaneSides(const zTPlane& plane) const
{
    zVEC3 negative;
    zVEC3 positive;
    for (int axis = 0; axis < 3; ++axis) {
        if (plane.normal.n[axis] >= 0.0f) {
            negative.n[axis] = mins.n[axis];
            positive.n[axis] = maxs.n[axis];
        }
        else {
            negative.n[axis] = maxs.n[axis];
            positive.n[axis] = mins.n[axis];
        }
    }
    if (plane.normal * positive - plane.distance > 0.0f)
        return 2;
    if (plane.normal * negative - plane.distance < 0.0f)
        return 3;
    return 1;
}

int zTBBox3D::ClassifyToPlaneSides(const zTPlane& plane, int signBits) const
{
    if (signBits > 7)
        return 1;
    zVEC3 points[8];
    GetCornerPoints(points);
    const float first = plane.normal * points[signBits] - plane.distance;
    if (first < 0.0f)
        return 2;
    const float second = plane.normal * points[7 - signBits] - plane.distance;
    if (second < 0.0f)
        return 3;
    return 1;
}

void zTBBox3D::SaveBIN(zCFileBIN& file)
{
    file.file->Write(this, sizeof(zTBBox3D));
}

void zTBBox3D::LoadBIN(zCFileBIN& file)
{
    file.file->Read(this, sizeof(zTBBox3D));
}

int zTBBox2D::Classify(const zTBBox2D& other) const
{
    for (int axis = 0; axis < 2; ++axis) {
        if (mins.n[axis] > other.maxs.n[axis] || maxs.n[axis] < other.mins.n[axis])
            return 1;
    }
    if (other.maxs.n[0] <= maxs.n[0] && other.mins.n[0] >= mins.n[0]
        && other.maxs.n[1] <= maxs.n[1] && other.mins.n[1] >= mins.n[1])
        return 0;
    return 2;
}

void zTBBox2D::ClipToBBox2D(const zTBBox2D& other)
{
    for (int axis = 0; axis < 2; ++axis) {
        if (other.mins.n[axis] > mins.n[axis])
            mins.n[axis] = other.mins.n[axis];
        if (other.maxs.n[axis] < maxs.n[axis])
            maxs.n[axis] = other.maxs.n[axis];
    }
}

void zTBBox2D::Draw() const
{
    const int minX = static_cast<int>(mins.n[0]);
    const int minY = static_cast<int>(mins.n[1]);
    const int maxX = static_cast<int>(maxs.n[0]);
    const int maxY = static_cast<int>(maxs.n[1]);
    zlineCache.Line(minX, minY, maxX, minY, 0xFF802A2A);
    zlineCache.Line(maxX, minY, maxX, maxY, 0xFF802A2A);
    zlineCache.Line(maxX, maxY, minX, maxY, 0xFF802A2A);
    zlineCache.Line(minX, maxY, minX, minY, 0xFF802A2A);
}

zCOBBox3D::~zCOBBox3D()
{
    for (zCList<zCOBBox3D>* entry = childs.next; entry; entry = entry->next)
        delete entry->data;
    childs.DeleteList();
}

void zCOBBox3D::Draw(int depth, zCOLOR color)
{
    zVEC3 corners[8];
    for (int i = 0; i < 8; ++i) {
        corners[i] = center;
        for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
            const float sign = (i & (1 << axisIndex)) ? 1.0f : -1.0f;
            corners[i] += axis[axisIndex] * (extent.n[axisIndex] * sign);
        }
    }
    static const int edges[12][2] = {
        { 0, 1 }, { 0, 2 }, { 1, 3 }, { 2, 3 },
        { 4, 5 }, { 4, 6 }, { 5, 7 }, { 6, 7 },
        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
    };
    for (int i = 0; i < 12; ++i)
        zlineCache.LineCS3D(corners[edges[i][0]], corners[edges[i][1]], color);
    if (depth > 1) {
        for (zCList<zCOBBox3D>* entry = childs.next; entry; entry = entry->next)
            entry->data->Draw(depth - 1, color);
    }
}

void zCOBBox3D::BuildOBB(int numPolys, zCPolygon** polygons, int useAABB)
{
    zCArray<zCVertex*> vertices;
    for (int i = 0; i < numPolys; ++i) {
        for (int j = 0; j < polygons[i]->polyNumVert; ++j) {
            zCVertex* vertex = polygons[i]->vertex[j];
            int known = 0;
            for (int k = 0; k < vertices.numInArray; ++k) {
                if (vertices.parray[k] == vertex) {
                    known = 1;
                    break;
                }
            }
            if (!known)
                vertices.InsertEnd(vertex);
        }
    }

    zTBBox3D box;
    box.Init();
    for (int i = 0; i < vertices.numInArray; ++i)
        box.AddPoint(vertices.parray[i]->position);
    center = (box.mins + box.maxs) * 0.5f;
    axis[0] = zVEC3(1.0f, 0.0f, 0.0f);
    axis[1] = zVEC3(0.0f, 1.0f, 0.0f);
    axis[2] = zVEC3(0.0f, 0.0f, 1.0f);
    extent = (box.maxs - box.mins) * 0.5f;
}

void zCOBBox3D::BuildRec(int numPolys, zCPolygon** polygons, int depth)
{
    if (depth < 1 || numPolys < 1)
        return;
    BuildOBB(numPolys, polygons, 1);
    zCPolygon** frontPolys = new zCPolygon*[numPolys];
    zCPolygon** backPolys = new zCPolygon*[numPolys];
    int numFront = 0;
    int numBack = 0;
    zTPlane splitPlane;
    splitPlane.normal = axis[2];
    splitPlane.distance = splitPlane.normal * center;
    for (int i = 0; i < numPolys; ++i) {
        const int side = polygons[i]->ClassifyPlane(splitPlane);
        if (side == 1)
            backPolys[numBack++] = polygons[i];
        else if (side == 0 || side == 2)
            frontPolys[numFront++] = polygons[i];
        else if (splitPlane.normal * polygons[i]->GetCenter() - splitPlane.distance > 0.001f)
            backPolys[numBack++] = polygons[i];
        else
            frontPolys[numFront++] = polygons[i];
    }
    if (numFront > 0 && numBack > 0) {
        zCOBBox3D* front = new zCOBBox3D;
        zCOBBox3D* back = new zCOBBox3D;
        childs.Insert(front);
        childs.Insert(back);
        front->BuildRec(numFront, frontPolys, depth - 1);
        back->BuildRec(numBack, backPolys, depth - 1);
    }
    delete[] frontPolys;
    delete[] backPolys;
}

void zCOBBox3D::BuildOBBTree(zCMesh* mesh, int depth)
{
    mesh->CreateListsFromArrays();
    BuildRec(mesh->numPoly, mesh->polyList, depth);
}

void zCOBBox3D::BuildOBBTree(int numPolys, zCPolygon** polygons, int depth)
{
    BuildRec(numPolys, polygons, depth);
}

zMAT4 zCOBBox3D::trafoB2ToThis;

static int TestOBBAxis(const zCOBBox3D& first, const zCOBBox3D& second, const zVEC3& axis)
{
    if (axis.Length2() < 0.000001f)
        return 1;
    const zVEC3 delta = second.center - first.center;
    float firstRadius = 0.0f;
    float secondRadius = 0.0f;
    for (int i = 0; i < 3; ++i) {
        firstRadius += first.extent.n[i] * fabs(first.axis[i] * axis);
        secondRadius += second.extent.n[i] * fabs(second.axis[i] * axis);
    }
    return fabs(delta * axis) <= firstRadius + secondRadius;
}

int zCOBBox3D::TestIntersectionOBB(const zCOBBox3D* other) const
{
    for (int i = 0; i < 3; ++i) {
        if (!TestOBBAxis(*this, *other, axis[i]))
            return 0;
        if (!TestOBBAxis(*this, *other, other->axis[i]))
            return 0;
    }
    for (int first = 0; first < 3; ++first) {
        for (int second = 0; second < 3; ++second) {
            if (!TestOBBAxis(*this, *other, axis[first] ^ other->axis[second]))
                return 0;
        }
    }
    return 1;
}

int zCOBBox3D::TestIntersectionTreeRec(const zCOBBox3D* other) const
{
    if (!TestIntersectionOBB(other))
        return 0;
    if (!childs.next && !other->childs.next)
        return 1;
    if (childs.next && (!other->childs.next || extent.n[0] <= other->extent.n[0])) {
        for (zCList<zCOBBox3D>* entry = childs.next; entry; entry = entry->next) {
            if (entry->data->TestIntersectionTreeRec(other))
                return 1;
        }
        return 0;
    }
    for (zCList<zCOBBox3D>* entry = other->childs.next; entry; entry = entry->next) {
        if (TestIntersectionTreeRec(entry->data))
            return 1;
    }
    return 0;
}

int zCOBBox3D::TestIntersectionTree(const zCOBBox3D* other, const zMAT4& trafo) const
{
    trafoB2ToThis = trafo;
    return TestIntersectionTreeRec(other);
}

int zCOBBox3D::TraceRay(const zVEC3& start, const zVEC3& direction, zVEC3& intersection) const
{
    const zVEC3 relative = start - center;
    zVEC3 localStart;
    zVEC3 localDirection;
    for (int i = 0; i < 3; ++i) {
        localStart.n[i] = relative * axis[i] / extent.n[i];
        localDirection.n[i] = direction * axis[i] / extent.n[i];
    }
    zTBBox3D localBox;
    localBox.mins = zVEC3(-1.0f);
    localBox.maxs = zVEC3(1.0f);
    return localBox.TraceRay(localStart, localDirection, intersection);
}

zVEC3 zMAT4::Rotate(const zVEC3& vector) const
{
    return zVEC3(m[0][0] * vector.n[0] + m[0][1] * vector.n[1] + m[0][2] * vector.n[2],
        m[1][0] * vector.n[0] + m[1][1] * vector.n[1] + m[1][2] * vector.n[2],
        m[2][0] * vector.n[0] + m[2][1] * vector.n[1] + m[2][2] * vector.n[2]);
}

int zCOBBox3D::CheckOBBPolyIntersection(zCPolygon* polygon, const zMAT4& trafo) const
{
    zCPolygon transformed;
    transformed.AllocVerts(polygon->polyNumVert);
    for (int i = 0; i < polygon->polyNumVert; ++i) {
        zCVertex* vertex = new zCVertex;
        const zVEC3 relative = trafo * polygon->vertex[i]->position - center;
        for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
            vertex->position.n[axisIndex] = relative * axis[axisIndex] / extent.n[axisIndex];
        transformed.SetVertex(i, vertex);
    }
    transformed.CalcNormal();
    zTBBox3D localBox;
    localBox.mins = zVEC3(-1.0f);
    localBox.maxs = zVEC3(1.0f);
    return transformed.CheckBBoxPolyIntersection(localBox);
}

zTBBox3D zCOBBox3D::GetBBox3D() const
{
    zTBBox3D result;
    result.Init();
    for (int i = 0; i < 8; ++i) {
        zVEC3 point = center;
        for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
            const float sign = (i & (1 << axisIndex)) ? 1.0f : -1.0f;
            point += axis[axisIndex] * (extent.n[axisIndex] * sign);
        }
        result.AddPoint(point);
    }
    return result;
}

void zCOBBox3D::SetByBBox3D(const zTBBox3D& box)
{
    center = (box.mins + box.maxs) * 0.5f;
    axis[0] = zVEC3(1.0f, 0.0f, 0.0f);
    axis[1] = zVEC3(0.0f, 1.0f, 0.0f);
    axis[2] = zVEC3(0.0f, 0.0f, 1.0f);
    extent = (box.maxs - box.mins) * 0.5f;
}

void zCOBBox3D::Transform(const zMAT4& trafo)
{
    for (int i = 0; i < 3; ++i)
        axis[i] = trafo.Rotate(axis[i]);
    center = trafo * center;
}

zCOBBox3D& zCOBBox3D::operator=(const zCOBBox3D& other)
{
    center = other.center;
    for (int i = 0; i < 3; ++i)
        axis[i] = other.axis[i];
    extent = other.extent;
    for (zCList<zCOBBox3D>* entry = other.childs.next; entry; entry = entry->next) {
        zCOBBox3D* child = new zCOBBox3D;
        childs.Insert(child);
        *child = *entry->data;
    }
    return *this;
}

void zCOBBox3D::SaveBIN(zCFileBIN& file)
{
    file.file->Write(&center, sizeof(zVEC3));
    file.file->Write(axis, sizeof(axis));
    file.file->Write(&extent, sizeof(zVEC3));
    unsigned short count = 0;
    for (zCList<zCOBBox3D>* entry = childs.next; entry; entry = entry->next)
        ++count;
    file.file->Write(&count, 2);
    for (zCList<zCOBBox3D>* entry = childs.next; entry; entry = entry->next)
        entry->data->SaveBIN(file);
}

void zCOBBox3D::LoadBIN(zCFileBIN& file)
{
    file.file->Read(&center, sizeof(zVEC3));
    file.file->Read(axis, sizeof(axis));
    file.file->Read(&extent, sizeof(zVEC3));
    unsigned short count;
    file.file->Read(&count, 2);
    if (count > 1000)
        return;
    for (int i = 0; i < count; ++i) {
        zCOBBox3D* child = new zCOBBox3D;
        childs.Insert(child);
        child->LoadBIN(file);
    }
}

zMAT4 zMAT4::s_identity = Alg_Identity3D();

zCCamera::zCCamera()
{
    trafoViewStack.pos = 0;
    trafoWorldStack.pos = 0;
    trafoWorldViewStack.pos = 0;
    nearClipZ = 0.1f;
    farClipZ = 4000.0f;
    targetView = NULL;
    connectedVob = NULL;
    trafoWorld = Alg_Identity3D();
    trafoProjection = Alg_Identity3D();
    trafoViewInv = Alg_Identity3D();
    trafoView = Alg_Identity3D();
    camMatrixInv = Alg_Identity3D();
    camMatrix = Alg_Identity3D();
    std::memset(&vpData, 0, sizeof(vpData));
    tremorAmplitude = zVEC3(0.0f);
    tremorOrigin = zVEC3(0.0f);
    tremorToggle = 0;
    tremorScale = 0.0f;
    tremorVelo = 0.0f;
    viewDistanceX = 0.0f;
    viewDistanceY = 0.0f;
    viewDistanceXInv = 0.0f;
    viewDistanceYInv = 0.0f;
    topBottomCos = 0.0f;
    topBottomSin = 0.0f;
    leftRightCos = 0.0f;
    leftRightSin = 0.0f;
    projection = 1;
    drawMode = 0;
    shadeMode = 0;
    drawWire = 0;
    screenFadeEnabled = 0;
    screenFadeTextureAniFPS = 0.0f;
    screenFadeTextureBlendFunc = 2;
    cinemaScopeEnabled = 0;
    polyMesh = zCMesh::CreateQuadMesh();
    poly = polyMesh->polyList[0];
    polyMaterial = new zCMaterial;
    polyMesh->SetStaticLight(0xFFFFFFFF);
    polyMesh->ResetLightDynToLightStat();
    SetFOV(90.0f);
}

zCCamera::~zCCamera()
{
    targetView = NULL;
    connectedVob = NULL;
    if (polyMesh && polyMesh->Release() <= 0)
        delete polyMesh;
    polyMesh = NULL;
    if (polyMaterial && polyMaterial->Release() <= 0)
        delete polyMaterial;
    polyMaterial = NULL;
    if (activeCam == this)
        activeCam = NULL;
}

void zCCamera::PushTransform(int type)
{
    if (type == 0) {
        trafoWorldStack.stack[trafoWorldStack.pos++] = trafoWorld;
    }
    else if (type == 1) {
        trafoViewStack.stack[trafoViewStack.pos++] = trafoView;
    }
    else if (type == 2) {
        trafoWorldViewStack.stack[trafoWorldViewStack.pos++] = camMatrix;
    }
}

void zCCamera::PopTransform(int type)
{
    if (type == 0) {
        SetTransform(type, trafoWorldStack.stack[--trafoWorldStack.pos]);
    }
    else if (type == 1) {
        SetTransform(type, trafoViewStack.stack[--trafoViewStack.pos]);
    }
    else if (type == 2) {
        SetTransform(type, trafoWorldViewStack.stack[--trafoWorldViewStack.pos]);
    }
}

void zCCamera::SetTransform(int type, const zMAT4& trafo)
{
    if (type == 0) {
        trafoWorld = trafo;
        camMatrix = trafoView * trafoWorld;
    }
    else if (type == 1) {
        trafoView = trafo;
        trafoViewInv = trafo.InverseLinTrafo();
        camMatrix = trafoView * trafoWorld;
    }
    else if (type == 2) {
        camMatrix = trafo;
    }
    camMatrixInv = camMatrix.InverseLinTrafo();
    if (activeCam) {
        activeCamPos.n[0] = activeCam->camMatrixInv.m[0][3];
        activeCamPos.n[1] = activeCam->camMatrixInv.m[1][3];
        activeCamPos.n[2] = activeCam->camMatrixInv.m[2][3];
    }
    SetUpFrustum();
    zrenderer->SetTransform(1, camMatrix.Transpose());
    zrenderer->SetTransform(2, trafoProjection);
}

const zMAT4& zCCamera::GetTransform(int type) const
{
    if (type == 0)
        return trafoWorld;
    if (type == 1)
        return trafoView;
    if (type == 3)
        return camMatrixInv;
    if (type == 4)
        return trafoViewInv;
    return camMatrix;
}

void zCCamera::Activate()
{
    if (!connectedVob)
        return;
    activeCam = this;
    trafoViewInv = connectedVob->trafoObjToWorld;
    camMatrixInv = trafoViewInv;
    trafoView = trafoViewInv.InverseLinTrafo();
    camMatrix = trafoView;
    trafoWorld = zMAT4::s_identity;
    activeCamPos.n[0] = trafoViewInv.m[0][3];
    activeCamPos.n[1] = trafoViewInv.m[1][3];
    activeCamPos.n[2] = trafoViewInv.m[2][3];
    SetUpFrustum();
    zrenderer->SetTransform(1, camMatrix.Transpose());
    zrenderer->SetTransform(2, trafoProjection);
    zrenderer->SetViewport(vpData.x0, vpData.y0, vpData.xdim, vpData.ydim);
}

void zCCamera::CreateProjectionMatrix(zMAT4& result, float nearClip, float farClip,
    float horizontalFOV, float verticalFOV)
{
    result = zMAT4::s_identity;
    result.m[0][0] = 1.0f / tan(horizontalFOV * 0.5f);
    result.m[1][1] = 1.0f / tan(verticalFOV * 0.5f);
    result.m[2][2] = farClip / (nearClip - farClip);
    result.m[3][2] = farClip * nearClip / (nearClip - farClip);
    result.m[2][3] = 1.0f;
    result.m[3][3] = 0.0f;
}

void zCCamera::UpdateProjectionMatrix()
{
    CreateProjectionMatrix(trafoProjection, nearClipZ, farClipZ, fovH, fovV);
    zrenderer->SetTransform(2, trafoProjection);
}

float zCCamera::GetFOV() const
{
    return fovH * 57.2957764f;
}

void zCCamera::GetFOV(float& horizontalFOV, float& verticalFOV) const
{
    horizontalFOV = fovH * 57.2957764f;
    verticalFOV = fovV * 57.2957764f;
}

void zCCamera::SetFOV(float fov)
{
    const float aspect = vpData.xdim ? static_cast<float>(vpData.ydim) / vpData.xdim : 0.750018775f;
    SetFOV(fov, aspect * fov);
}

void zCCamera::SetFOV(float horizontalFOV, float verticalFOV)
{
    fovH = horizontalFOV / 57.2957764f;
    fovV = verticalFOV / 57.2957764f;
    if (vpData.xdim) {
        viewDistanceX = 1.0f / tan(fovH * 0.5f);
        viewDistanceY = 1.0f / tan(fovV * 0.5f);
        viewDistanceXInv = 1.0f / viewDistanceX;
        viewDistanceYInv = 1.0f / viewDistanceY;
        leftRightSin = sin(fovH * 0.5f);
        leftRightCos = cos(fovH * 0.5f);
        topBottomSin = sin(fovV * 0.5f);
        topBottomCos = cos(fovV * 0.5f);
    }
    UpdateProjectionMatrix();
}

void zCCamera::UpdateViewport()
{
    if (!targetView)
        return;
    const int oldX = vpData.x0;
    const int oldY = vpData.y0;
    const int oldWidth = vpData.xdim;
    const int oldHeight = vpData.ydim;
    static_cast<zCViewBase*>(targetView)->GetViewport(vpData.x0, vpData.y0, vpData.x1, vpData.y1);
    vpData.xdim = vpData.x1 - vpData.x0 + 1;
    vpData.ydim = vpData.y1 - vpData.y0 + 1;
    if (oldX == vpData.x0 && oldY == vpData.y0
        && oldWidth == vpData.xdim && oldHeight == vpData.ydim)
        return;
    vpData.xcenter = vpData.x0 + vpData.xdim * 0.5f;
    vpData.ycenter = vpData.y0 + vpData.ydim * 0.5f;
    SetFOV(fovH * 57.2957764f, fovV * 57.2957764f);
    SetUpFrustum();
}

void zCCamera::SetRenderTarget(void* viewport)
{
    targetView = viewport;
    UpdateViewport();
}

void zCCamera::SetRenderScreenFadeTex(zSTRING fileName)
{
    if (fileName.IsEmpty()) {
        polyMaterial->SetTexture(zSTRING());
        return;
    }
    zCTexture* texture = zCTexture::Load(fileName, 0);
    polyMaterial->SetTexture(texture);
}

zMAT4* zCCamera::SetUpFrustum()
{
    const zVEC3 directions[4] = {
        zVEC3(leftRightCos, 0.0f, -leftRightSin),
        zVEC3(-leftRightCos, 0.0f, -leftRightSin),
        zVEC3(0.0f, topBottomCos, -topBottomSin),
        zVEC3(0.0f, -topBottomCos, -topBottomSin)
    };
    for (int i = 0; i < 4; ++i) {
        frustumplanes[i].normal = camMatrix.Rotate(directions[i]);
        frustumplanes[i].distance = activeCamPos * frustumplanes[i].normal - 0.0001f;
        signbits[i] = frustumplanes[i].CalcSignbits();
    }
    frustumplanes[4].normal = camMatrix.Rotate(zVEC3(0.0f, 0.0f, -1.0f));
    frustumplanes[4].distance = activeCamPos * frustumplanes[4].normal + farClipZ;
    signbits[4] = frustumplanes[4].CalcSignbits();
    frustumplanes[5].normal = -frustumplanes[4].normal;
    frustumplanes[5].distance = -activeCamPos * frustumplanes[5].normal;
    signbits[5] = frustumplanes[5].CalcSignbits();
    return &camMatrix;
}

zVEC3 operator*(const zVEC3& vector, const zMAT4& matrix)
{
    return zVEC3(matrix.m[0][0] * vector.n[0] + matrix.m[0][1] * vector.n[1]
            + matrix.m[0][2] * vector.n[2] + matrix.m[0][3],
        matrix.m[1][0] * vector.n[0] + matrix.m[1][1] * vector.n[1]
            + matrix.m[1][2] * vector.n[2] + matrix.m[1][3],
        matrix.m[2][0] * vector.n[0] + matrix.m[2][1] * vector.n[1]
            + matrix.m[2][2] * vector.n[2] + matrix.m[2][3]);
}

int zTPlane::CalcSignbits() const
{
    return (normal.n[0] >= 0.0f ? 1 : 0) | (normal.n[1] >= 0.0f ? 2 : 0)
        | (normal.n[2] >= 0.0f ? 4 : 0);
}

void zCCamera::SetFarClipZ(float farClip)
{
    farClipZ = farClip < 0.0f ? 0.0f : farClip;
    SetUpFrustum();
    UpdateProjectionMatrix();
}

int zCCamera::BBox3DInFrustum(const zTBBox3D& box)
{
    for (int i = 5; i >= 0; --i) {
        const int result = box.ClassifyToPlane(frustumplanes[i], signbits[i]);
        if (result == 1)
            return 1;
        if (result == 3)
            return 2;
    }
    return 0;
}

int zCCamera::BBox3DInFrustum(const zTBBox3D& box, int& planeMask)
{
    for (int i = 5; i >= 0; --i) {
        const int mask = 1 << i;
        if (!(planeMask & mask))
            continue;
        const int result = box.ClassifyToPlane(frustumplanes[i], signbits[i]);
        if (result == 1)
            return 1;
        if (result != 3)
            planeMask &= ~mask;
    }
    return planeMask <= 0 ? 0 : 2;
}

void zCCamera::StopTremor()
{
    tremorScale = 0.0f;
}

void zCCamera::AddTremor(const zVEC3& amplitude, float velocity, float range,
    const zVEC3& origin)
{
    const zVEC3 position(connectedVob->trafoObjToWorld.m[0][3],
        connectedVob->trafoObjToWorld.m[1][3], connectedVob->trafoObjToWorld.m[2][3]);
    const float distance2 = (origin - position).Length2();
    const float range2 = range * range;
    if (distance2 >= range2)
        return;
    const float attenuation = 1.0f - distance2 / range2;
    const zVEC3 scaled = amplitude * attenuation;
    if (tremorScale <= 0.0f) {
        tremorAmplitude = scaled;
        tremorScale = 1.0f;
        tremorVelo = 1.0f / velocity;
        return;
    }
    for (int i = 0; i < 3; ++i) {
        if (scaled.n[i] > tremorAmplitude.n[i])
            tremorAmplitude.n[i] = scaled.n[i];
    }
    tremorScale = (tremorScale + 1.0f) * 0.5f;
    const float newVelocity = 1.0f / velocity;
    if (newVelocity > tremorVelo)
        tremorVelo = newVelocity;
}

void zCCamera::PreRenderProcessing()
{
    if (tremorScale <= 0.0f)
        return;
    tremorToggle = !tremorToggle;
    zVEC3 randomDirection;
    randomDirection.n[0] = rand() * 6.10370189e-05f - 1.0f;
    randomDirection.n[1] = tremorToggle ? rand() * 3.05185094e-05f
        : rand() * 3.05185094e-05f - 1.0f;
    randomDirection.n[2] = rand() * 6.10370189e-05f - 1.0f;
    tremorScale -= ztimer.frameTimeFloat * tremorVelo;
    if (tremorScale < 0.0f)
        tremorScale = 0.0f;
    const zVEC3 offset = Alg_Prod(tremorAmplitude, randomDirection) * tremorScale;
    tremorOrigin = zVEC3(connectedVob->trafoObjToWorld.m[0][3],
        connectedVob->trafoObjToWorld.m[1][3], connectedVob->trafoObjToWorld.m[2][3]);
    connectedVob->Move(offset.n[0], offset.n[1], offset.n[2]);
}

void zCCamera::PostRenderProcessing()
{
    if (tremorScale > 0.0f)
        connectedVob->SetPositionWorld(tremorOrigin);
    RenderCinemaScope();
    RenderScreenFade();
}

void zCCamera::GetCamPos(zVEC3& position) const
{
    position = zVEC3(connectedVob->trafoObjToWorld.m[0][3],
        connectedVob->trafoObjToWorld.m[1][3], connectedVob->trafoObjToWorld.m[2][3]);
}

void zCCamera::InitDrawPolySimple()
{
    polyMesh = zCMesh::CreateQuadMesh();
    poly = polyMesh->polyList[0];
    polyMaterial = new zCMaterial;
    polyMesh->SetStaticLight(0xFFFFFFFF);
    polyMesh->ResetLightDynToLightStat();
}

void zCCamera::CleanupDrawPolySimple()
{
    if (polyMesh && polyMesh->Release() <= 0)
        delete polyMesh;
    polyMesh = NULL;
    if (polyMaterial && polyMaterial->Release() <= 0)
        delete polyMaterial;
    polyMaterial = NULL;
}

void zCCamera::DrawPolySimple(zTCamVertSimple* vertices, int count,
    zCMaterial* material, int flags)
{
    zrenderer->FlushPolys();
    for (int i = 0; i < count; ++i) {
        zCVertexTransform* transform = poly->vertex[i]->CreateVertexTransform();
        transform->vertScrX = vertices[i].x;
        transform->vertScrY = vertices[i].y;
        transform->vertCamSpaceZInv = vertices[i].z;
    }
    poly->polyNumVert = count;
    poly->material = material;
    poly->Unclip();
    zrenderer->SetPolySortMode(0);
    zrenderer->SetZBufferWriteEnabled(0);
    zrenderer->DrawPoly(poly);
    zrenderer->FlushPolys();
    zrenderer->SetPolySortMode(1);
    zrenderer->SetZBufferWriteEnabled(1);
    poly->polyNumVert = 4;
    poly->material = NULL;
}

void zCCamera::SetRenderScreenFade(zCOLOR color)
{
    screenFadeEnabled = 1;
    screenFadeColor = color;
}

void zCCamera::SetRenderScreenFadeTexAniFPS(float value)
{
    screenFadeTextureAniFPS = value;
    if (polyMaterial)
        polyMaterial->texAniCtrl.aniFPS = value * 0.001f;
}

void zCCamera::SetRenderScreenFadeTexBlendFunc(int value)
{
    screenFadeTextureBlendFunc = value;
}

void zCCamera::SetRenderCinemaScope(zCOLOR color)
{
    cinemaScopeEnabled = 1;
    cinemaScopeColor = color;
}

void zCCamera::RenderScreenFade()
{
    if (!screenFadeEnabled)
        return;
    screenFadeEnabled = 0;
    const float minX = vpData.x0;
    const float minY = vpData.y0;
    const float maxX = vpData.x1;
    const float maxY = vpData.y1;
    polyCamVerts[0].x = minX;
    polyCamVerts[0].y = maxY;
    polyCamVerts[1].x = maxX;
    polyCamVerts[1].y = maxY;
    polyCamVerts[2].x = maxX;
    polyCamVerts[2].y = minY;
    polyCamVerts[3].x = minX;
    polyCamVerts[3].y = minY;
    for (int i = 0; i < 4; ++i)
        polyCamVerts[i].z = 10000.0f;
    if (polyMaterial->texture) {
        polyMaterial->texAniCtrl.AdvanceAni(polyMaterial->texture);
        polyMaterial->GetAniTexture();
    }
    polyMaterial->color[0] = screenFadeColor;
    polyMaterial->rndAlphaBlendFunc = screenFadeTextureBlendFunc;
    DrawPolySimple(polyCamVerts, 4, polyMaterial, 0);
}

void zCCamera::RenderCinemaScope()
{
    if (!cinemaScopeEnabled)
        return;
    cinemaScopeEnabled = 0;
    const float alpha = cinemaScopeColor[3] * 0.00392156886f;
    const float border = alpha * vpData.ydim * 0.12f;
    polyCamVerts[0].x = vpData.x0;
    polyCamVerts[1].x = vpData.x1 + 1.0f;
    polyCamVerts[2].x = vpData.x1 + 1.0f;
    polyCamVerts[3].x = vpData.x0;
    polyCamVerts[0].y = vpData.y0 + border;
    polyCamVerts[1].y = vpData.y0 + border;
    polyCamVerts[2].y = vpData.y0;
    polyCamVerts[3].y = vpData.y0;
    for (int i = 0; i < 4; ++i)
        polyCamVerts[i].z = 10000.0f;
    polyMaterial->color[0] = cinemaScopeColor;
    polyMaterial->rndAlphaBlendFunc = 2;
    DrawPolySimple(polyCamVerts, 4, polyMaterial, 0);

    polyCamVerts[0].y = vpData.y1 + 1.0f;
    polyCamVerts[1].y = vpData.y1 + 1.0f;
    polyCamVerts[2].y = vpData.y1 - border;
    polyCamVerts[3].y = vpData.y1 - border;
    polyMaterial->color[0] = cinemaScopeColor;
    DrawPolySimple(polyCamVerts, 4, polyMaterial, 0);
}

static void ProjectPortalPoint(zCCamera* camera, const zVEC3& point, float& x, float& y)
{
    const zVEC3 cameraPoint = camera->camMatrix * point;
    const float inverseZ = 1.0f / cameraPoint.n[2];
    x = cameraPoint.n[0] * camera->viewDistanceX * inverseZ + camera->vpData.xcenter;
    y = camera->vpData.ycenter - cameraPoint.n[1] * camera->viewDistanceY * inverseZ;
}

int zCCamera::ScreenProjectionTouchesPortalRough(const zTBBox3D& box, const zTBBox2D& portal)
{
    const zVEC3 center = (box.mins + box.maxs) * 0.5f;
    if ((camMatrix * center).n[2] < 2.0f)
        return 1;
    float centerX;
    float centerY;
    ProjectPortalPoint(this, center, centerX, centerY);
    float radius = 0.0f;
    for (int i = 0; i < 8; ++i) {
        const zVEC3 point((i & 1) ? box.maxs.n[0] : box.mins.n[0],
            (i & 2) ? box.maxs.n[1] : box.mins.n[1], (i & 4) ? box.maxs.n[2] : box.mins.n[2]);
        float x;
        float y;
        ProjectPortalPoint(this, point, x, y);
        const float deltaX = x - centerX;
        const float deltaY = y - centerY;
        const float distance = deltaX * deltaX + deltaY * deltaY;
        if (distance > radius)
            radius = distance;
    }
    zTBBox2D projected;
    projected.mins = zVEC2(centerX - radius, centerY - radius);
    projected.maxs = zVEC2(centerX + radius, centerY + radius);
    return projected.Classify(portal) != 1;
}

int zCCamera::ScreenProjectionTouchesPortal(const zTBBox3D& box, const zTBBox2D& portal)
{
    zTBBox2D projected;
    projected.mins = zVEC2(999999.0f);
    projected.maxs = zVEC2(-999999.0f);
    for (int i = 0; i < 8; ++i) {
        const zVEC3 point((i & 1) ? box.maxs.n[0] : box.mins.n[0],
            (i & 2) ? box.maxs.n[1] : box.mins.n[1], (i & 4) ? box.maxs.n[2] : box.mins.n[2]);
        if ((camMatrix * point).n[2] < 2.0f)
            return 1;
        float x;
        float y;
        ProjectPortalPoint(this, point, x, y);
        if (x < projected.mins.n[0])
            projected.mins.n[0] = x;
        if (x > projected.maxs.n[0])
            projected.maxs.n[0] = x;
        if (y < projected.mins.n[1])
            projected.mins.n[1] = y;
        if (y > projected.maxs.n[1])
            projected.maxs.n[1] = y;
    }
    return projected.Classify(portal) != 1;
}

zCArray<zCCollisionObjectDef*>* zCCollisionObjectDef::s_aoCollObjClassList
    = new zCArray<zCCollisionObjectDef*>;

zCCollisionReport::zCCollisionReport()
{
    m_poCollObject1 = NULL;
    m_poCollObject0 = NULL;
}

zCCollisionReport::~zCCollisionReport()
{
    m_poCollObject1 = NULL;
    m_poCollObject0 = NULL;
}

zCCollisionDetector::zCCollisionDetector()
{
    const int count = zCCollisionObjectDef::s_aoCollObjClassList->numInArray;
    const int matrixSize = count * count;
    m_apStaticTestFuncMatrix = new zTStaticIntersectionTest*[matrixSize];
    std::memset(m_apStaticTestFuncMatrix, 0, matrixSize * sizeof(void*));
    m_apDynamicTestFuncMatrix = new zTDynamicIntersectionTest*[matrixSize];
    std::memset(m_apDynamicTestFuncMatrix, 0, matrixSize * sizeof(void*));
}

zCCollisionDetector::~zCCollisionDetector()
{
    delete[] m_apStaticTestFuncMatrix;
    m_apStaticTestFuncMatrix = NULL;
    delete[] m_apDynamicTestFuncMatrix;
    m_apDynamicTestFuncMatrix = NULL;
}

int zCCollisionDetector::GetTestFuncIndex(zCCollisionObjectDef* first, zCCollisionObjectDef* second)
{
    return zCCollisionObjectDef::s_aoCollObjClassList->numInArray * first->m_iIndex
        + second->m_iIndex;
}

zTStaticIntersectionTest* zCCollisionDetector::GetStaticTestFunc(zCCollisionObjectDef* first,
    zCCollisionObjectDef* second)
{
    return m_apStaticTestFuncMatrix[GetTestFuncIndex(first, second)];
}

zTDynamicIntersectionTest* zCCollisionDetector::GetDynamicTestFunc(zCCollisionObjectDef* first,
    zCCollisionObjectDef* second)
{
    return m_apDynamicTestFuncMatrix[GetTestFuncIndex(first, second)];
}

void zCCollisionDetector::RegisterStaticIntersectionTest(zCCollisionObjectDef* first,
    zCCollisionObjectDef* second, zTStaticIntersectionTest* function)
{
    m_apStaticTestFuncMatrix[GetTestFuncIndex(first, second)] = function;
}

void zCCollisionDetector::RegisterDynamicIntersectionTest(zCCollisionObjectDef* first,
    zCCollisionObjectDef* second, zTDynamicIntersectionTest* function)
{
    m_apDynamicTestFuncMatrix[GetTestFuncIndex(first, second)] = function;
}

int zCCollisionDetector::CanCollide(zCCollisionObjectDef* first, zCCollisionObjectDef* second)
{
    const int index = GetTestFuncIndex(first, second);
    return m_apDynamicTestFuncMatrix[index] || m_apStaticTestFuncMatrix[index];
}

unsigned long zCCollisionDetector::s_iCollisionDetectorFrameCtr;
int zCCollisionDetector::s_iCurrentCollReport;
zCCollisionReport zCCollisionDetector::s_aoCollReportList[16];
zCCollisionObjectDef zCCollisionObject::s_oCollObjClass(1, zCCollisionObject::_CreateNewInstance);

void zCCollisionDetector::S_ResetFreeCollReportList()
{
    s_iCurrentCollReport = 0;
}

zCCollisionReport* zCCollisionDetector::S_GetNextFreeCollReport()
{
    if (s_iCurrentCollReport >= 16)
        s_iCurrentCollReport = 0;
    return &s_aoCollReportList[s_iCurrentCollReport++];
}

void zCCollisionDetector::DoDynamicIntersectionTests(zCCollisionObject* object,
    zCArray<zCCollisionObject*>& objects, zCArray<zCCollisionReport*>& reports)
{
    int index = 0;
    while (index < objects.numInArray) {
        zCCollisionObject* other = objects.parray[index];
        zTDynamicIntersectionTest* test = GetDynamicTestFunc(object->GetCollObjClass(),
            other->GetCollObjClass());
        if (!test) {
            ++index;
            continue;
        }
        zCCollisionReport* report = NULL;
        if (test(object, other, &report) && report) {
            report->m_poCollObject0 = object;
            report->m_poCollObject1 = other;
            reports.InsertEnd(report);
        }
        objects.RemoveIndex(index);
    }
}

static zMAT4 InterpolateCollisionTrafo(const zMAT4& first, const zMAT4& second, float time)
{
    zMAT4 result = first;
    for (int i = 0; i < 3; ++i)
        result.m[i][3] = first.m[i][3] + (second.m[i][3] - first.m[i][3]) * time;
    return result;
}

void zCCollisionDetector::DoStaticIntersectionTests(zCCollisionObject* object,
    zCArray<zCCollisionObject*>& objects, zCArray<zCCollisionReport*>& reports)
{
    float time = reports.numInArray > 0 ? reports.parray[0]->m_fTimeStamp : 0.0f;
    const float step = object->GetAdaptiveStepSize() > 0.0f ? object->GetAdaptiveStepSize() : 0.25f;
    while (time < 1.0f) {
        const zMAT4 trafo = InterpolateCollisionTrafo(object->m_oTrafoOld,
            object->m_oTrafoNew, time);
        int found = 0;
        for (int i = 0; i < objects.numInArray; ++i) {
            zCCollisionObject* other = objects.parray[i];
            zTStaticIntersectionTest* test = GetStaticTestFunc(object->GetCollObjClass(),
                other->GetCollObjClass());
            if (!test)
                continue;
            zCCollisionReport* report = NULL;
            if (test(time, object, trafo, other, other->m_oTrafoNew, &report) && report) {
                report->m_fTimeStamp = time;
                report->m_poCollObject0 = object;
                report->m_poCollObject1 = other;
                reports.InsertEnd(report);
                found = 1;
            }
        }
        if (found)
            break;
        time += step;
        if (time > 0.99f)
            time = 1.0f;
    }
}

void zCCollisionDetector::DetectCollisions(zCCollisionObject* object,
    const zCArray<zCCollisionObject*>& objects, zCArray<zCCollisionReport*>& reports)
{
    ++s_iCollisionDetectorFrameCtr;
    reports.numInArray = 0;
    S_ResetFreeCollReportList();
    object->DetectCollisionsSelf(objects, reports);
    zCArray<zCCollisionObject*> remaining = objects;
    DoDynamicIntersectionTests(object, remaining, reports);
    if (remaining.numInArray > 0)
        DoStaticIntersectionTests(object, remaining, reports);
}

zCCollisionObject* zCCollisionObject::_CreateNewInstance()
{
    return new zCCollisionObject;
}

zCCollisionObjectDef* zCCollisionObject::GetCollObjClass() const
{
    return &s_oCollObjClass;
}

void zCCollisionObject::DetectCollisionsSelf(const zCArray<zCCollisionObject*>& objects,
    zCArray<zCCollisionReport*>& reports)
{
}

void zCCollisionObject::CollisionResponse(const zCArray<zCCollisionReport*>& reports, int& flags)
{
}

float zCCollisionObject::GetAdaptiveStepSize()
{
    return 0.0f;
}

int zCCollisionObject::SuppressCollisionResponse()
{
    return 0;
}

zCCollisionObjectDef::zCCollisionObjectDef(int volatileValue, zCCollisionObject* (*factory)())
{
    if (!s_aoCollObjClassList)
        s_aoCollObjClassList = new zCArray<zCCollisionObjectDef*>;
    m_bIsVolatile = volatileValue;
    m_createNewInstance = factory;
    m_iIndex = s_aoCollObjClassList->numInArray;
    s_aoCollObjClassList->InsertEnd(this);
}

zCCollisionObjectDef::~zCCollisionObjectDef()
{
    if (!s_aoCollObjClassList)
        return;
    s_aoCollObjClassList->Remove(this);
    if (s_aoCollObjClassList->numInArray <= 0) {
        delete s_aoCollObjClassList;
        s_aoCollObjClassList = NULL;
    }
}

void zCCollisionObject::CalcIntermediateTrafo(float time, zMAT4& result)
{
    if (time <= 0.0f) {
        result = m_oTrafoOld;
        return;
    }
    if (time >= 1.0f) {
        result = m_oTrafoNew;
        return;
    }
    result = InterpolateCollisionTrafo(m_oTrafoOld, m_oTrafoNew, time);
}

void zCCollisionObject::GetLargestBBox3DLocal(zTBBox3D& box)
{
    box.mins = zVEC3(-0.1f);
    box.maxs = zVEC3(0.1f);
}

zCCollisionObjectDef zCCollObjectUndef::s_oCollObjClass(1, zCCollObjectUndef::_CreateNewInstance);
zCCollisionObjectDef zCCollObjectLevelPolys::s_oCollObjClass(1, zCCollObjectLevelPolys::_CreateNewInstance);

zCCollisionObject* zCCollObjectUndef::_CreateNewInstance()
{
    return new zCCollObjectUndef;
}

zCCollisionObjectDef* zCCollObjectUndef::GetCollObjClass() const
{
    return &s_oCollObjClass;
}

zCCollisionObject* zCCollObjectLevelPolys::_CreateNewInstance()
{
    return new zCCollObjectLevelPolys;
}

zCCollisionObjectDef* zCCollObjectLevelPolys::GetCollObjClass() const
{
    return &s_oCollObjClass;
}

zCCollObjectLevelPolys::~zCCollObjectLevelPolys()
{
}

zCCollisionObjectDef zCCollObjectPoint::s_oCollObjClass(1, zCCollObjectPoint::_CreateNewInstance);
zCCollisionObjectDef zCCollObjectProjectile::s_oCollObjClass(1, zCCollObjectProjectile::_CreateNewInstance);
zCCollisionObjectDef zCCollObjectComplex::s_oCollObjClass(1, zCCollObjectComplex::_CreateNewInstance);

zCCollisionObject* zCCollObjectCharacter::_CreateNewInstance()
{
    return new zCCollObjectCharacter;
}

zCCollisionObject* zCCollObjectPoint::_CreateNewInstance()
{
    return new zCCollObjectPoint;
}

zCCollisionObjectDef* zCCollObjectPoint::GetCollObjClass() const
{
    return &s_oCollObjClass;
}

zCCollisionObject* zCCollObjectProjectile::_CreateNewInstance()
{
    return new zCCollObjectProjectile;
}

zCCollisionObjectDef* zCCollObjectProjectile::GetCollObjClass() const
{
    return &s_oCollObjClass;
}

zCCollisionObject* zCCollObjectComplex::_CreateNewInstance()
{
    return new zCCollObjectComplex;
}

zCCollisionObjectDef* zCCollObjectComplex::GetCollObjClass() const
{
    return &s_oCollObjClass;
}

zCCollisionObjectDef zCCollObjectBoxBlocker::s_oCollObjClass(1, zCCollObjectBoxBlocker::_CreateNewInstance);
zCCollisionObjectDef zCCollObjectBoxPassThrough::s_oCollObjClass(1, zCCollObjectBoxPassThrough::_CreateNewInstance);

zCCollisionObject* zCCollObjectBoxBlocker::_CreateNewInstance()
{
    return new zCCollObjectBoxBlocker;
}

zCCollisionObjectDef* zCCollObjectBoxBlocker::GetCollObjClass() const
{
    return &s_oCollObjClass;
}

zCCollisionObject* zCCollObjectBoxPassThrough::_CreateNewInstance()
{
    return new zCCollObjectBoxPassThrough;
}

zCCollisionObjectDef* zCCollObjectBoxPassThrough::GetCollObjClass() const
{
    return &s_oCollObjClass;
}

int zCCollObjectBoxPassThrough::SuppressCollisionResponse()
{
    return 1;
}

void zCCollObjectPoint::GetLargestBBox3DLocal(zTBBox3D& box)
{
    const zCVob* vob = static_cast<const zCVob*>(m_pvClientData);
    if (vob && vob->visual) {
        const zTBBox3D visualBox = vob->visual->GetBBox3D();
        const zVEC3 center = (visualBox.mins + visualBox.maxs) * 0.5f;
        box.mins = center;
        box.maxs = center;
    }
    else {
        box.mins = zVEC3(0.0f);
        box.maxs = box.mins;
    }
}

void zCCollObjectPoint::CollisionResponse(const zCArray<zCCollisionReport*>& reports, int& flags)
{
    if ((m_bHintTranslation & 3) && reports.numInArray > 0)
        CalcIntermediateTrafo(reports.parray[0]->m_fTimeStamp, m_oTrafoNew);
}

zMAT4& zMAT4::Translate(const zVEC3& translation)
{
    m[0][3] += translation.n[0];
    m[1][3] += translation.n[1];
    m[2][3] += translation.n[2];
    return *this;
}

zVEC3 zCCollObjectPoint::GetCenterOffsetLocal() const
{
    const zCVob* vob = static_cast<const zCVob*>(m_pvClientData);
    if (vob && vob->visual) {
        const zTBBox3D box = vob->visual->GetBBox3D();
        return (box.mins + box.maxs) * 0.5f;
    }
    return zVEC3(0.0f);
}

zVEC3 zCCollObjectPoint::GetCenterOffsetNew() const
{
    return m_oTrafoNew.Rotate(GetCenterOffsetLocal());
}

zVEC3 zTBBox3D::GetCenterFloor() const
{
    return zVEC3((mins.n[0] + maxs.n[0]) * 0.5f, mins.n[1],
        (mins.n[2] + maxs.n[2]) * 0.5f);
}

void zCCollObjectPoint::SetUpVectorNew(const zVEC3& direction)
{
    m_oTrafoNew.SetUpVector(direction);
    m_oTrafoNew.MakeOrthonormal();
    m_oTrafoNew.Translate(-GetCenterOffsetNew());
}

void zCCollObjectPoint::DetectCollisionsSelf(const zCArray<zCCollisionObject*>& objects,
    zCArray<zCCollisionReport*>& reports)
{
}

float zCCollObjectComplex::GetAdaptiveStepSize()
{
    const zCVob* vob = static_cast<const zCVob*>(m_pvClientData);
    return zCCollObjectBase::S_CalcAdaptiveCollStepFromVisualBBox(vob ? vob->visual : NULL);
}

void zCCollObjectComplex::CollisionResponse(const zCArray<zCCollisionReport*>& reports, int& flags)
{
    if ((m_bHintTranslation & 3) && reports.numInArray > 0)
        CalcIntermediateTrafo(reports.parray[0]->m_fTimeStamp, m_oTrafoNew);
}

static zVEC3 GetCollisionObjectCenter(const zCCollisionObject* object)
{
    return zVEC3(object->m_oTrafoNew.m[0][3], object->m_oTrafoNew.m[1][3],
        object->m_oTrafoNew.m[2][3]);
}

int TestDynamic_Projectile_Char(zCCollisionObject* projectile, zCCollisionObject* other,
    zCCollisionReport** report)
{
    const zVEC3 start(projectile->m_oTrafoOld.m[0][3], projectile->m_oTrafoOld.m[1][3],
        projectile->m_oTrafoOld.m[2][3]);
    const zVEC3 end = GetCollisionObjectCenter(projectile);
    const zVEC3 direction = end - start;
    const zCVob* otherVob = static_cast<const zCVob*>(other->m_pvClientData);
    if (!otherVob)
        return 0;
    zVEC3 intersection;
    if (!otherVob->bbox3D.TraceRay(start, direction, intersection))
        return 0;

    if (report) {
        zCCollisionReport* hit = zCCollisionDetector::S_GetNextFreeCollReport();
        *report = hit;
        hit->m_fTimeStamp = direction.Length() > 0.0f
            ? (intersection - start).Length() / direction.Length() * 0.95f : 0.0f;
        const zCVob* projectileVob = static_cast<const zCVob*>(projectile->m_pvClientData);
        const zVEC3 firstCenter = projectileVob ? projectileVob->bbox3D.GetCenter() : start;
        const zVEC3 secondCenter = otherVob->bbox3D.GetCenter();
        hit->m_oContactPoint = (firstCenter + secondCenter) * 0.5f;
        hit->m_oCollNormal0 = (secondCenter - firstCenter).NormalizeApprox();
        hit->m_oCollNormal1 = -hit->m_oCollNormal0;
    }
    return 1;
}

zVEC3& zVEC3::NormalizeApprox()
{
    const float inverseLength = Alg_SqrtInvApprox(Length2());
    n[0] *= inverseLength;
    n[1] *= inverseLength;
    n[2] *= inverseLength;
    return *this;
}

zVEC3 zTBBox3D::GetCenter() const
{
    return (mins + maxs) * 0.5f;
}

int TestDynamic_XXX_BoxPassThrough(zCCollisionObject* first, zCCollisionObject* second,
    zCCollisionReport** report)
{
    const zCVob* firstVob = static_cast<const zCVob*>(first->m_pvClientData);
    const zCVob* secondVob = static_cast<const zCVob*>(second->m_pvClientData);
    if (!firstVob || !secondVob)
        return 0;
    const zVEC3 start(first->m_oTrafoOld.m[0][3], first->m_oTrafoOld.m[1][3],
        first->m_oTrafoOld.m[2][3]);
    const zVEC3 direction = GetCollisionObjectCenter(first) - start;
    float time;
    if (!firstVob->bbox3D.IsIntersectingSweep(direction, secondVob->bbox3D, time))
        return 0;

    if (report) {
        zCCollisionReport* hit = zCCollisionDetector::S_GetNextFreeCollReport();
        *report = hit;
        hit->m_fTimeStamp = time * 0.95f;
        const zVEC3 firstCenter = firstVob->bbox3D.GetCenter();
        const zVEC3 secondCenter = secondVob->bbox3D.GetCenter();
        hit->m_oContactPoint = (firstCenter + secondCenter) * 0.5f;
        hit->m_oCollNormal0 = (secondCenter - firstCenter).NormalizeApprox();
        hit->m_oCollNormal1 = -hit->m_oCollNormal0;
    }
    return 1;
}

float zCCollObjectBase::S_CalcAdaptiveCollStepFromVisualBBox(zCVisual* visual)
{
    if (!visual)
        return 1.0f;
    float extent;
    zCOBBox3D* obb = visual->GetOBBox3D();
    if (obb) {
        extent = obb->extent.n[2];
        if (obb->extent.n[1] < extent)
            extent = obb->extent.n[1];
        if (obb->extent.n[0] < extent)
            extent = obb->extent.n[0];
    }
    else {
        const zTBBox3D box = visual->GetBBox3D();
        extent = box.GetMinExtent();
    }
    const float step = extent * 0.9f;
    return step < 0.1f ? 0.1f : step;
}

void zCCollObjectBase::S_CalcGeneric_CollNormal_ContactPoint(const zCVob* first,
    const zCVob* second, zCCollisionReport& report)
{
    const zVEC3 firstCenter = first->bbox3D.GetCenter();
    const zVEC3 secondCenter = second->bbox3D.GetCenter();
    report.m_oContactPoint = (firstCenter + secondCenter) * 0.5f;
    report.m_oCollNormal0 = (secondCenter - firstCenter).NormalizeApprox();
    report.m_oCollNormal1 = -report.m_oCollNormal0;
}

void zCCollObjectBase::GetLargestBBox3DLocal(zTBBox3D& box)
{
    const zCVob* vob = static_cast<const zCVob*>(m_pvClientData);
    if (vob && vob->visual)
        box = vob->visual->GetBBox3D();
    else if (vob)
        vob->bbox3D.UnTransform(vob->trafoObjToWorld.InverseLinTrafo(), box);
}

static int TestStatic_Box_Box(float time, zCCollisionObject* first, const zMAT4& firstTrafo,
    zCCollisionObject* second, const zMAT4& secondTrafo, zCCollisionReport** report)
{
    const zCVob* firstVob = static_cast<const zCVob*>(first->m_pvClientData);
    const zCVob* secondVob = static_cast<const zCVob*>(second->m_pvClientData);
    if (!firstVob || !secondVob)
        return 0;
    const zVEC3 direction(firstTrafo.m[0][3] - first->m_oTrafoOld.m[0][3],
        firstTrafo.m[1][3] - first->m_oTrafoOld.m[1][3],
        firstTrafo.m[2][3] - first->m_oTrafoOld.m[2][3]);
    float hitTime;
    if (!firstVob->bbox3D.IsIntersectingSweep(direction, secondVob->bbox3D, hitTime))
        return 0;
    if (report) {
        zCCollisionReport* hit = zCCollisionDetector::S_GetNextFreeCollReport();
        *report = hit;
        hit->m_fTimeStamp = hitTime * 0.95f;
        zCCollObjectBase::S_CalcGeneric_CollNormal_ContactPoint(firstVob, secondVob, *hit);
    }
    return 1;
}

void zCCollObjectBase::S_RegisterCollisionTestFuncs(zCCollisionDetector* detector)
{
    detector->RegisterStaticIntersectionTest(&zCCollObjectLevelPolys::s_oCollObjClass,
        &zCCollObjectLevelPolys::s_oCollObjClass, TestStatic_Box_Box);
    detector->RegisterStaticIntersectionTest(&zCCollObjectComplex::s_oCollObjClass,
        &zCCollObjectComplex::s_oCollObjClass, TestStatic_Box_Box);
    detector->RegisterStaticIntersectionTest(&zCCollObjectCharacter::s_oCollObjClass,
        &zCCollObjectCharacter::s_oCollObjClass, TestStatic_Char_Char);
    detector->RegisterDynamicIntersectionTest(&zCCollObjectLevelPolys::s_oCollObjClass,
        &zCCollObjectLevelPolys::s_oCollObjClass, TestDynamic_XXX_BoxPassThrough);
    detector->RegisterDynamicIntersectionTest(&zCCollObjectComplex::s_oCollObjClass,
        &zCCollObjectComplex::s_oCollObjClass, TestDynamic_XXX_BoxPassThrough);
    detector->RegisterDynamicIntersectionTest(&zCCollObjectBoxPassThrough::s_oCollObjClass,
        &zCCollObjectBoxPassThrough::s_oCollObjClass, TestDynamic_XXX_BoxPassThrough);
    detector->RegisterDynamicIntersectionTest(&zCCollObjectCharacter::s_oCollObjClass,
        &zCCollObjectCharacter::s_oCollObjClass, TestDynamic_Projectile_Char);
}

int zCConvexPrimitive::SymetrySphere() const
{
    return 0;
}

int zCConvexPrimitive::SymetryRotation(zVEC3& axis) const
{
    return 0;
}

void zCConvexPrimitive::UpdateBBox()
{
    bbox3Dlocal.mins = zVEC3(0.0f);
    bbox3Dlocal.maxs = zVEC3(0.0f);
}

zCConvexPrimitiveUnitSphere zCConvexPrimitiveUnitSphere::s_unitSphere;

zCConvexPrimitiveEllipsoid::zCConvexPrimitiveEllipsoid(const zVEC3& centerValue,
    const zVEC3& radiiValue)
    : zCConvexPrimitiveScaleTrans(&zCConvexPrimitiveUnitSphere::s_unitSphere,
        radiiValue, centerValue), symetric(0)
{
    UpdateBBox();
}

void zCConvexPrimitiveScaleTrans::UpdateBBox()
{
    original->UpdateBBox();
    for (int axis = 0; axis < 3; ++axis) {
        bbox3Dlocal.mins.n[axis] = original->bbox3Dlocal.mins.n[axis] * scale.n[axis]
            + translation.n[axis];
        bbox3Dlocal.maxs.n[axis] = original->bbox3Dlocal.maxs.n[axis] * scale.n[axis]
            + translation.n[axis];
    }
}

zCCollObjectCharacter::zCCollObjectCharacter()
{
    m_pvClientData = NULL;
    m_poPrimitiveCC = NULL;
    m_oUpRayCache.first = NULL;
    m_oUpRayCache.next = zCRayCache::s_anchor;
    zCRayCache::s_anchor = &m_oUpRayCache;
    m_oDownRayCache.first = NULL;
    m_oDownRayCache.next = zCRayCache::s_anchor;
    zCRayCache::s_anchor = &m_oDownRayCache;
    m_oFrontRayCache.first = NULL;
    m_oFrontRayCache.next = zCRayCache::s_anchor;
    zCRayCache::s_anchor = &m_oFrontRayCache;
    m_oBackRayCache.first = NULL;
    m_oBackRayCache.next = zCRayCache::s_anchor;
    zCRayCache::s_anchor = &m_oBackRayCache;
    m_oLeftRayCache.first = NULL;
    m_oLeftRayCache.next = zCRayCache::s_anchor;
    zCRayCache::s_anchor = &m_oLeftRayCache;
    m_oRightRayCache.first = NULL;
    m_oRightRayCache.next = zCRayCache::s_anchor;
    zCRayCache::s_anchor = &m_oRightRayCache;
    m_oSpatialState.m_bFloorIsStair = 4;
    m_oConfig.zMV_STEP_HEIGHT = 0;
    m_oConfig.zMV_JUMP_UP_MIN_CEIL = 0;
    m_fWallSlideTimer = 0.0f;
    m_fWallSlideRotDir = 0.0f;
}

zCCollObjectCharacter::~zCCollObjectCharacter()
{
    delete m_poPrimitiveCC;
    m_poPrimitiveCC = NULL;
}

zCCollisionObjectDef* zCCollObjectCharacter::GetCollObjClass() const
{
    return &s_oCollObjClass;
}

int zCCollObjectCharacter::IsDead() const
{
    return 0;
}

int zCCollObjectCharacter::DontLetHimComeNearer(const zCVob* other) const
{
    return 0;
}

float zCCollObjectCharacter::TurnSpeed() const
{
    return 1.0f;
}

void zCCollObjectCharacter::PushAround(const zVEC3& direction)
{
}

zCConvexPrimitive* zCCollObjectCharacter::GetPrimitiveCC() const
{
    if (!m_poPrimitiveCC) {
        const zCVob* vob = static_cast<const zCVob*>(m_pvClientData);
        const zTBBox3D box = vob && vob->visual ? vob->visual->GetBBox3D() : zTBBox3D();
        const zVEC3 center = (box.mins + box.maxs) * 0.5f;
        const zVEC3 radii = (box.maxs - box.mins) * 0.6f;
        const_cast<zCCollObjectCharacter*>(this)->m_poPrimitiveCC
            = new zCConvexPrimitiveEllipsoid(center, radii);
        m_poPrimitiveCC->UpdateBBox();
    }
    return m_poPrimitiveCC;
}

void zCCollObjectCharacter::GetFallbackTrafoCC(const zMAT4& oldTrafo,
    const zMAT4& currentTrafo, const zVEC3& center, int flags, zMAT4& result) const
{
    GetPrimitiveCC();
    result = currentTrafo;
}

zVEC3 zCCollObjectCharacter::S_SlideCC(const zVEC3& direction, float time,
    const zVEC3& normal)
{
    const float projection = direction * normal;
    zVEC3 result = direction - normal * projection;
    if (result.Length2() < 0.0001f)
        result = zVEC3(0.0f);
    return result;
}

int TestStatic_Char_Char(float time, zCCollisionObject* first, const zMAT4& firstTrafo,
    zCCollisionObject* second, const zMAT4& secondTrafo, zCCollisionReport** report)
{
    return TestStatic_Box_Box(time, first, firstTrafo, second, secondTrafo, report);
}

void zCCollObjectCharacter::GetCollisionPoleYDims(float& minY, float& maxY) const
{
    const zCVob* vob = static_cast<const zCVob*>(m_pvClientData);
    const zTBBox3D box = vob && vob->visual ? vob->visual->GetBBox3D() : zTBBox3D();
    minY = box.mins.n[1];
    maxY = box.maxs.n[1];
}

int zCCollObjectCharacter::CalcSlideVector(const zVEC3& direction, const zVEC3& normal,
    zVEC3& slide, zVEC3& remaining, float& projection)
{
    projection = direction * normal;
    slide = direction - normal * projection;
    remaining = direction - slide;
    return slide.Length2() > 0.000001f;
}

float Alg_AngleUnitRad(const zVEC3& first, const zVEC3& second)
{
    float dot = first * second;
    if (dot >= 1.0f)
        return 0.0f;
    if (dot <= -1.0f)
        return 3.14159274f;
    return acos(dot);
}

void zCCollObjectCharacter::DoWallSlidingLatentRotation(zMAT4& trafo)
{
    if (m_fWallSlideTimer <= 0.0f)
        return;
    float angle = ztimer.frameTimeFloat * 0.14f;
    if (m_fWallSlideRotDir < 0.0f)
        angle = -angle;
    const zVEC3 translation = trafo.GetTranslation();
    trafo.m[0][3] = 0.0f;
    trafo.m[1][3] = 0.0f;
    trafo.m[2][3] = 0.0f;
    trafo = Alg_Rotation3DN(m_oWallSlideRotAxis, angle) * trafo;
    trafo.m[0][3] = translation.n[0];
    trafo.m[1][3] = translation.n[1];
    trafo.m[2][3] = translation.n[2];
    m_fWallSlideTimer -= ztimer.frameTimeFloat;
}

void zCCollObjectCharacter::FindFloorWaterCeiling(const zVEC3& position, zTSpatialState& state)
{
    state.m_bFloorIsStair &= 0xFB;
    state.m_poCeilingPoly = NULL;
    state.m_poWaterPoly = NULL;
    const zCVob* vob = static_cast<const zCVob*>(m_pvClientData);
    zCWorld* world = vob->homeWorld;
    const zVEC3 down(0.0f, -100000.0f, 0.0f);
    world->TraceRayNearestHitCache(position, down, const_cast<zCVob*>(vob), 0x4A21,
        &m_oDownRayCache);
    if (world->traceRayReport.foundHit) {
        state.m_fLastFloorY = state.m_fFloorY;
        state.m_fFloorY = world->traceRayReport.foundIntersection.n[1] + 0.5f;
        state.m_fWaterY = state.m_fFloorY;
        state.m_poFloorPoly = world->traceRayReport.foundPoly;
        if (state.m_poFloorPoly && state.m_poFloorPoly->material
            && state.m_poFloorPoly->material->matGroup == 5) {
            state.m_poWaterPoly = state.m_poFloorPoly;
            state.m_fWaterY = world->traceRayReport.foundIntersection.n[1];
        }
    }
    else {
        state.m_fFloorY = state.m_fLastFloorY;
        state.m_fCeilingY = 3.402823466e38F;
        state.m_poFloorPoly = NULL;
    }

    const zVEC3 up(0.0f, 100000.0f, 0.0f);
    world->TraceRayNearestHitCache(position, up, const_cast<zCVob*>(vob), 0x4A21,
        &m_oUpRayCache);
    if (!world->traceRayReport.foundHit) {
        state.m_fCeilingY = 1000000.0f;
        state.m_poCeilingPoly = NULL;
        return;
    }
    state.m_fCeilingY = world->traceRayReport.foundIntersection.n[1] - 0.5f;
    state.m_poCeilingPoly = world->traceRayReport.foundPoly;
    if (state.m_poCeilingPoly && state.m_poCeilingPoly->material
        && state.m_poCeilingPoly->material->matGroup == 5) {
        state.m_poWaterPoly = state.m_poCeilingPoly;
        state.m_fWaterY = state.m_fCeilingY;
    }
}

void zCCollObjectCharacter::MarkSpatialStateAsUninited(int refresh)
{
    m_oSpatialState.m_bFloorIsStair |= 4;
    if (refresh) {
        const zCVob* vob = static_cast<const zCVob*>(m_pvClientData);
        const zVEC3 position(vob->trafoObjToWorld.m[0][3], vob->trafoObjToWorld.m[1][3],
            vob->trafoObjToWorld.m[2][3]);
        FindFloorWaterCeiling(position, m_oSpatialState);
    }
}

int zCCollObjectCharacter::S_CalcAvgNormalVariance(const zTTraceRayReport (&reports)[4],
    zVEC3& averageNormal, float& variance)
{
    averageNormal = zVEC3(0.0f);
    variance = 100.0f;
    int count = 0;
    for (int i = 0; i < 4; ++i) {
        if (reports[i].foundHit) {
            averageNormal += reports[i].foundPolyNormal;
            ++count;
        }
    }
    if (count <= 0)
        return 0;
    if (count == 1) {
        variance = 1.0f;
        return 1;
    }
    const float length = averageNormal.Length();
    if (length == 0.0f)
        return 0;
    averageNormal /= length;
    for (int i = 0; i < 4; ++i) {
        if (reports[i].foundHit) {
            const float dot = reports[i].foundPolyNormal * averageNormal;
            if (dot < variance)
                variance = dot;
        }
    }
    variance *= 100.0f;
    return 1;
}

zVEC3 zCCollObjectCharacter::GetSurfaceAlignScanOrigin(const zVEC3& origin)
{
    if (!(m_oConfig.zMV_STEP_HEIGHT & 8))
        return origin;
    const zCVob* vob = static_cast<const zCVob*>(m_pvClientData);
    const zTBBox3D box = vob->visual->GetBBox3D();
    const float height = (box.maxs.n[1] - box.mins.n[1]) * 0.65f;
    if (height <= 0.0f)
        return origin;
    const zVEC3 direction = m_oTrafoNew.Rotate(zVEC3(0.0f, height, 0.0f));
    zCWorld* world = vob->homeWorld;
    if (world->TraceRayNearestHitCache(origin, direction, const_cast<zCVob*>(vob), 0x8A1,
        &m_oUpRayCache))
        return origin + (world->traceRayReport.foundIntersection - origin) * 0.8f;
    return origin;
}

int zCCollObjectCharacter::TestHardCollisions(const zVEC3& start, const zVEC3& end,
    zVEC3& position, zTSpatialState& state, zVEC3& normal)
{
    position = end;
    const zCVob* vob = static_cast<const zCVob*>(m_pvClientData);
    zCWorld* world = vob->homeWorld;
    const zVEC3 direction = end - start;
    if (!world->TraceRayNearestHit(start, direction, const_cast<zCVob*>(vob), 0xA1)) {
        FindFloorWaterCeiling(position, state);
        return 0;
    }
    position = world->traceRayReport.foundIntersection;
    normal = world->traceRayReport.foundPolyNormal;
    const zVEC3 remaining = end - position;
    const float projection = remaining * normal;
    position += remaining - normal * projection;
    FindFloorWaterCeiling(position, state);
    return 1;
}

int zCCollObjectCharacter::TestSoftCollisions(const zVEC3& direction, const zVEC3& normal,
    zVEC3& slide, zVEC3& remaining)
{
    slide = S_SlideCC(direction, 0.0f, normal);
    remaining = direction - slide;
    return slide.Length2() > 0.000001f;
}

void zCCollObjectCharacter::DetectCollisionsSelf(const zCArray<zCCollisionObject*>& objects,
    zCArray<zCCollisionReport*>& reports)
{
    const zCVob* vob = static_cast<const zCVob*>(m_pvClientData);
    if (!vob || !vob->visual)
        return;
    DoWallSlidingLatentRotation(m_oTrafoNew);
    if (m_oSpatialState.m_bFloorIsStair & 4) {
        const zVEC3 position = m_oTrafoNew.GetTranslation();
        FindFloorWaterCeiling(position, m_oSpatialState);
    }

    for (int iteration = 0; iteration < 2; ++iteration) {
        zVEC3 slide;
        zVEC3 remaining;
        zVEC3 normal(0.0f, 1.0f, 0.0f);
        TestSoftCollisions(m_oTrafoNew.GetTranslation() - m_oTrafoOld.GetTranslation(),
            normal, slide, remaining);
        zTSpatialState state = m_oSpatialState;
        zVEC3 position;
        TestHardCollisions(m_oTrafoOld.GetTranslation(),
            m_oTrafoOld.GetTranslation() + slide, position, state, normal);
        m_oTrafoNew.m[0][3] = position.n[0];
        m_oTrafoNew.m[1][3] = position.n[1];
        m_oTrafoNew.m[2][3] = position.n[2];
        if (iteration == 0)
            m_oSpatialState = state;
    }
}

void zCCollObjectCharacter::CollisionResponse(const zCArray<zCCollisionReport*>& reports, int& flags)
{
    if (!(m_bHintTranslation & 3) || reports.numInArray <= 0)
        return;
    zCCollisionReport* report = reports.parray[0];
    zCCollisionObject* other = report->m_poCollObject0;
    if (other) {
        zMAT4 fallback;
        other->GetFallbackTrafoCC(other->m_oTrafoOld, other->m_oTrafoNew,
            report->m_oCollNormal0, reports.numInArray <= 1, fallback);
        m_oTrafoNew = fallback;
    }
    const zVEC3 slide = S_SlideCC(m_oTrafoNew.GetTranslation() - m_oTrafoOld.GetTranslation(),
        0.0f, report->m_oCollNormal0);
    m_oTrafoNew.m[0][3] = m_oTrafoOld.m[0][3] + slide.n[0];
    m_oTrafoNew.m[1][3] = m_oTrafoOld.m[1][3] + slide.n[1];
    m_oTrafoNew.m[2][3] = m_oTrafoOld.m[2][3] + slide.n[2];
    flags = 1;
}

void zCCollObjectCharacter::GetLargestBBox3DLocal(zTBBox3D& box)
{
    box = GetPrimitiveCC()->bbox3Dlocal;
}

float zCCollObjectCharacter::GetAdaptiveStepSize()
{
    const zCVob* vob = static_cast<const zCVob*>(m_pvClientData);
    if (!vob || !vob->visual)
        return 0.0f;
    const float step = vob->visual->GetBBox3D().GetMinExtent() * 1.08f;
    return step < 0.1f ? 0.1f : step;
}

void zCCollObjectCharacter::PrintDebugInfos()
{
}

void zCConvexPrimitive::PointNearestToPlane(zVEC3& result, const zVEC3& point,
    const zMAT4& trafo, const zMAT3& rotation) const
{
    zVEC3 localPoint;
    PointNearestToPlaneLocal(rotation * point, localPoint);
    result = trafo * localPoint;
}

void zCConvexPrimitive::GetCenterLocal(zVEC3& center) const
{
    center = zVEC3(0.0f);
}

static zVEC3 GetConvexSupport(const zCConvexPrimitive* primitive, const zMAT4& trafo,
    const zVEC3& direction)
{
    zVEC3 localPoint;
    primitive->PointNearestToPlaneLocal(direction, localPoint);
    return trafo * localPoint;
}

int zCConvexPrimitive::CheckCollision(const zCConvexPrimitive* first,
    const zCConvexPrimitive* second, const zMAT4& firstTrafo, const zMAT4& secondTrafo,
    zCConvexCollisionCache* cache, zVEC3& contact, float& distance, zVEC3& normal)
{
    normal = secondTrafo.GetTranslation() - firstTrafo.GetTranslation();
    if (normal.Length2() < 0.001f)
        normal = zVEC3(0.0f, 1.0f, 0.0f);
    else
        normal.Normalize();
    distance = Distance(first, second, firstTrafo, secondTrafo, normal, contact, normal);
    return distance <= 0.0f;
}

zVEC3& zVEC3::operator-=(const zVEC3& value)
{
    n[0] -= value.n[0];
    n[1] -= value.n[1];
    n[2] -= value.n[2];
    return *this;
}

zVEC3& zVEC3::operator*=(float value)
{
    n[0] *= value;
    n[1] *= value;
    n[2] *= value;
    return *this;
}

float zCConvexPrimitive::Distance(const zCConvexPrimitive* first,
    const zCConvexPrimitive* second, const zMAT4& firstTrafo, const zMAT4& secondTrafo,
    const zVEC3& direction, zVEC3& contact, zVEC3& normal)
{
    const zVEC3 firstPoint = GetConvexSupport(first, firstTrafo, -direction);
    const zVEC3 secondPoint = GetConvexSupport(second, secondTrafo, direction);
    normal = secondPoint - firstPoint;
    contact = (firstPoint + secondPoint) * 0.5f;
    return normal * direction;
}

zCConvexCollisionCache::zCConvexCollisionCache(const zCCacheIndex& index)
    : planeNormal(0.0f), invert(0)
{
}

zCConvexCollisionCache* zCConvexPrimitive::GetCache(const void* first, const void* second)
{
    struct zTCacheEntry {
        const void* first;
        const void* second;
        zCConvexCollisionCache* cache;
    };
    static zCArray<zTCacheEntry> entries;
    for (int i = 0; i < entries.numInArray; ++i) {
        if (entries.parray[i].first == first && entries.parray[i].second == second)
            return entries.parray[i].cache;
    }
    zCCacheIndex index;
    index.voba = const_cast<zCVob*>(static_cast<const zCVob*>(first));
    index.vobb = const_cast<zCVob*>(static_cast<const zCVob*>(second));
    zCConvexCollisionCache* cache = new zCConvexCollisionCache(index);
    zTCacheEntry entry = { first, second, cache };
    entries.InsertEnd(entry);
    return cache;
}

void zCConvexCollisionCache::operator delete(void* memory)
{
    ::operator delete(memory);
}

static zMAT4 convexDrawTrafo;
static zCOLOR convexDrawColor;
static zVEC3 convexDrawLastPoint;

void zCConvexPrimitive::Draw(const zMAT4& trafo, zCOLOR color, const zCCamera* camera) const
{
    convexDrawTrafo = trafo;
    convexDrawColor = color;
    DrawVirtual();
}

void zCConvexPrimitive::DrawLine(const zVEC3& first, const zVEC3& second)
{
    zlineCache.LineCS3D(first, second, convexDrawColor);
}

void zCConvexPrimitive::DrawLineStrip(const zVEC3& point)
{
    const zVEC3 transformed = convexDrawTrafo * point;
    DrawLine(convexDrawLastPoint, transformed);
    convexDrawLastPoint = transformed;
}

void zCConvexPrimitive::DrawVirtual() const
{
    DrawLine(convexDrawTrafo * zVEC3(-1.0f, 0.0f, 0.0f),
        convexDrawTrafo * zVEC3(1.0f, 0.0f, 0.0f));
    DrawLine(convexDrawTrafo * zVEC3(0.0f, -1.0f, 0.0f),
        convexDrawTrafo * zVEC3(0.0f, 1.0f, 0.0f));
    DrawLine(convexDrawTrafo * zVEC3(0.0f, 0.0f, -1.0f),
        convexDrawTrafo * zVEC3(0.0f, 0.0f, 1.0f));
}

zCConvexPrimitiveUnitSphere::zCConvexPrimitiveUnitSphere()
{
    bbox3Dlocal.mins = zVEC3(0.0f);
    bbox3Dlocal.maxs = zVEC3(0.0f);
}

zCConvexPrimitiveUnitSphere::~zCConvexPrimitiveUnitSphere()
{
}

void zCConvexPrimitiveUnitSphere::PointNearestToPlaneLocal(const zVEC3& point,
    zVEC3& result) const
{
    result = point;
}

int zCConvexPrimitiveUnitSphere::SymetrySphere() const
{
    return 1;
}

int zCConvexPrimitiveUnitSphere::SymetryRotation(zVEC3& axis) const
{
    axis = zVEC3(0.0f);
    return 1;
}

void zCConvexPrimitiveUnitSphere::UpdateBBox()
{
    bbox3Dlocal.mins = zVEC3(-1.0f);
    bbox3Dlocal.maxs = zVEC3(1.0f);
}

void zCConvexPrimitiveUnitSphere::DrawVirtual() const
{
    for (int i = 50; i >= 0; --i) {
        const float angle = i * 0.02f * 3.14159f;
        const float spiralAngle = angle * 12.0f;
        const float radius = sin(angle);
        DrawLineStrip(zVEC3(sin(spiralAngle) * radius, cos(angle),
            cos(spiralAngle) * radius));
    }
}

int zCConvexPrimitiveUnitSphere::PointIsInLocal(const zVEC3& point) const
{
    return point.Length2() <= 1.0f;
}

zCConvexPrimitiveScaleTrans::zCConvexPrimitiveScaleTrans(zCConvexPrimitive* originalValue,
    const zVEC3& scaleValue, const zVEC3& translationValue)
    : scale(scaleValue), translation(translationValue), original(originalValue)
{
    bbox3Dlocal.mins = zVEC3(0.0f);
    bbox3Dlocal.maxs = zVEC3(0.0f);
}

zCConvexPrimitiveScaleTrans::~zCConvexPrimitiveScaleTrans()
{
}

void zCConvexPrimitiveScaleTrans::PointNearestToPlaneLocal(const zVEC3& point,
    zVEC3& result) const
{
    zVEC3 scaledNormal(point.n[0] * scale.n[0], point.n[1] * scale.n[1],
        point.n[2] * scale.n[2]);
    scaledNormal.Normalize();
    original->PointNearestToPlaneLocal(scaledNormal, result);
    for (int axis = 0; axis < 3; ++axis)
        result.n[axis] = result.n[axis] * scale.n[axis] + translation.n[axis];
}

int zCConvexPrimitiveScaleTrans::PointIsInLocal(const zVEC3& point) const
{
    const zVEC3 originalPoint((point.n[0] - translation.n[0]) / scale.n[0],
        (point.n[1] - translation.n[1]) / scale.n[1],
        (point.n[2] - translation.n[2]) / scale.n[2]);
    return original->PointIsInLocal(originalPoint);
}

void zCConvexPrimitiveScaleTrans::GetCenterLocal(zVEC3& center) const
{
    original->GetCenterLocal(center);
    for (int axis = 0; axis < 3; ++axis)
        center.n[axis] = center.n[axis] * scale.n[axis] + translation.n[axis];
}

void zCConvexPrimitiveScaleTrans::DrawVirtual() const
{
    zMAT4 localTrafo = zMAT4::s_identity;
    localTrafo.m[0][0] = scale.n[0];
    localTrafo.m[1][1] = scale.n[1];
    localTrafo.m[2][2] = scale.n[2];
    localTrafo.m[0][3] = translation.n[0];
    localTrafo.m[1][3] = translation.n[1];
    localTrafo.m[2][3] = translation.n[2];
    convexDrawTrafo = convexDrawTrafo * localTrafo;
    original->DrawVirtual();
}

zCConvexPrimitiveEllipsoid::zCConvexPrimitiveEllipsoid(const zTBBox3D& box, int snap)
    : zCConvexPrimitiveScaleTrans(&zCConvexPrimitiveUnitSphere::s_unitSphere,
        (box.maxs - box.mins) * 0.5f, (box.maxs + box.mins) * 0.5f), symetric(0)
{
    if (snap)
        Snap();
}

int zCConvexPrimitiveEllipsoid::SymetryRotation(zVEC3& axis) const
{
    if (!symetric)
        return 0;
    axis = zVEC3(0.0f, 1.0f, 0.0f);
    return 1;
}

zCConvexPrimitiveEllipsoid::~zCConvexPrimitiveEllipsoid()
{
}

void zCConvexPrimitiveEllipsoid::Snap()
{
    const float sum = scale.n[0] + scale.n[2];
    if (fabs(scale.n[0] - scale.n[2]) <= sum * 0.4f) {
        scale.n[0] = sum * 0.5f;
        scale.n[2] = scale.n[0];
        UpdateBBox();
        symetric = 1;
    }
}
