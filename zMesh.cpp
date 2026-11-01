#include "include/z3d.h"
#include "include/zTexture.h"
#include "include/zContainer.h"
#include "include/zFile3D.h"
#include "include/zPath.h"
#include "include/oGame.h"
#include "include/zRenderer.h"
#include "include/zSky.h"

#include <cmath>

extern int playerLightInt;

static zCSparseArray<void*, int> meshSparseArray(0x407);
// EXE: 0x008D8868
int zCMesh::s_autoConvertMeshes;

void zCMesh::CheckForMaterials() const
{
    for (int i = 0; i < numPoly; ++i) {
        zCPolygon* polygon = polyList[i];
        if (!(polygon->flags[1] & 3) && !polygon->material) {
            zerr->Report(zERR_TYPE_FAULT, 0,
                "D: z3d(zCMesh::CheckForMaterials): poly without material ! (" + meshName + ")",
                4, 0, 0x5DE, NULL, NULL);
        }
    }
}

int zCMesh::Render(zTRenderContext& context)
{
    int intensity = 200;
    if (context.world) {
        if (context.vob && context.vob->groundPoly && (context.vob->groundPoly->flags[0] & 8))
            intensity = 120;
        zCOLOR color = context.world->GetActiveSkyControler()->GetDaylightColorFromIntensity(intensity);
        return Render(context, &color);
    }
    zCOLOR color(200, 200, 200, 255);
    return Render(context, &color);
}

int zCMesh::Render(zTRenderContext& context, zCOLOR* color)
{
    if (context.clipFlags == -1) {
        context.clipFlags = 0x3F;
        if (zCCamera::activeCam->BBox3DInFrustum(bbox3D, context.clipFlags) == 1)
            return 0;
    }

    const unsigned int memoryMark = zCVertexTransform::s_MemMan.Mark();
    for (int polyIndex = 0; polyIndex < numPoly; ++polyIndex) {
        zCPolygon* polygon = polyList[polyIndex];
        float planeDistance = 0.0f;
        for (int axis = 0; axis < 3; ++axis)
            planeDistance += zCCamera::activeCamPos.n[axis] * polygon->polyPlane.normal.n[axis];
        if (planeDistance - polygon->polyPlane.distance <= 0.0f)
            continue;

        if (context.clipFlags <= 0)
            polygon->Unclip();
        else if (!polygon->ClipToFrustum(context.clipFlags))
            continue;

        zCMaterial* material = polygon->material;
        if (material) {
            material->ApplyTexAniMapping(polygon);
            if (material->texture) {
                if ((material->texture->cacheState & 3) != 3) {
                    material->texture->TouchTimeStampLocal();
                    material->texture->CacheIn(0.0f);
                }
                else
                    material->texture->TouchTimeStamp();
            }
        }

        for (int i = polygon->numClipVert - 1; i >= 0; --i) {
            zCVertex* vertex = polygon->clipVert[i];
            if (!vertex->transformedIndex) {
                zCVertexTransform* transform = vertex->CreateVertexTransform();
                zCCamera* camera = zCCamera::activeCam;
                for (int axis = 0; axis < 3; ++axis) {
                    transform->vertCamSpace.n[axis] =
                        camera->camMatrix.m[axis][0] * vertex->position.n[0] +
                        camera->camMatrix.m[axis][1] * vertex->position.n[1] +
                        camera->camMatrix.m[axis][2] * vertex->position.n[2] +
                        camera->camMatrix.m[axis][3];
                }
                transform->vertCamSpaceZInv = 1.0f / transform->vertCamSpace.n[2];
                transform->vertScrX = camera->viewDistanceX * transform->vertCamSpace.n[0] *
                    transform->vertCamSpaceZInv + camera->vpData.xcenter;
                transform->vertScrY = camera->vpData.ycenter - camera->viewDistanceY *
                    transform->vertCamSpace.n[1] * transform->vertCamSpaceZInv;
                if (transform->vertScrX < camera->vpData.x0)
                    transform->vertScrX = camera->vpData.x0;
                else if (transform->vertScrX > camera->vpData.x1)
                    transform->vertScrX = camera->vpData.x1;
                if (transform->vertScrY < camera->vpData.y0)
                    transform->vertScrY = camera->vpData.y0;
                else if (transform->vertScrY > camera->vpData.y1)
                    transform->vertScrY = camera->vpData.y1;
            }
            if (color) {
                polygon->clipFeat[i]->lightDyn[0] = color->b;
                polygon->clipFeat[i]->lightDyn[1] = color->g;
                polygon->clipFeat[i]->lightDyn[2] = color->r;
                polygon->clipFeat[i]->lightDyn[3] = color->a;
            }
        }
        polygon->LightDynCamSpace(zCCamera::activeCamPos, playerLightInt);
        zrenderer->DrawPoly(polygon);
    }
    zCVertexTransform::s_MemMan.Restore(memoryMark);
    return 1;
}

void zCCamera::ProjectClamp(zCVertexTransform* transform, float depthInv) const
{
    transform->vertScrX = viewDistanceX * transform->vertCamSpace.n[0] * depthInv +
        vpData.xcenter;
    transform->vertScrY = vpData.ycenter - viewDistanceY * transform->vertCamSpace.n[1] * depthInv;
    if (transform->vertScrX < vpData.x0)
        transform->vertScrX = vpData.x0;
    else if (transform->vertScrX > vpData.x1)
        transform->vertScrX = vpData.x1;
    if (transform->vertScrY < vpData.y0)
        transform->vertScrY = vpData.y0;
    else if (transform->vertScrY > vpData.y1)
        transform->vertScrY = vpData.y1;
}

int zCMesh::RenderHorizon(zTRenderContext& context, float horizonZ)
{
    const float horizonInv = 1.0f / horizonZ;
    const unsigned int memoryMark = zCVertexTransform::s_MemMan.Mark();
    for (int polyIndex = 0; polyIndex < numPoly; ++polyIndex) {
        zCPolygon* polygon = polyList[polyIndex];
        float planeDistance = 0.0f;
        for (int axis = 0; axis < 3; ++axis)
            planeDistance += zCCamera::activeCamPos.n[axis] * polygon->polyPlane.normal.n[axis];
        if (planeDistance - polygon->polyPlane.distance <= 0.0f)
            continue;
        if (context.clipFlags <= 0)
            polygon->Unclip();
        else if (!polygon->ClipToFrustum(context.clipFlags))
            continue;
        if (polygon->material)
            polygon->material->ApplyTexAniMapping(polygon);

        for (int i = polygon->numClipVert - 1; i >= 0; --i) {
            zCVertex* vertex = polygon->clipVert[i];
            if (!vertex->transformedIndex) {
                zCVertexTransform* transform = vertex->CreateVertexTransform();
                for (int axis = 0; axis < 3; ++axis) {
                    transform->vertCamSpace.n[axis] =
                        context.cam->camMatrix.m[axis][0] * vertex->position.n[0] +
                        context.cam->camMatrix.m[axis][1] * vertex->position.n[1] +
                        context.cam->camMatrix.m[axis][2] * vertex->position.n[2] +
                        context.cam->camMatrix.m[axis][3];
                }
                context.cam->ProjectClamp(transform, 1.0f / transform->vertCamSpace.n[2]);
                transform->vertCamSpace.n[2] = horizonZ;
                transform->vertCamSpaceZInv = horizonInv;
            }
        }
        zrenderer->AddAlphaPoly(polygon);
    }
    zrenderer->FlushAlphaPolys();
    zCVertexTransform::s_MemMan.Restore(memoryMark);
    return 1;
}

static void SetupMeshDecalCamera(float x, float y, float size)
{
    zCCamera* camera = zCCamera::activeCam;
    const float offsetX = (x - camera->vpData.xcenter) * camera->viewDistanceXInv * size;
    const float offsetY = (camera->vpData.ycenter - y) * camera->viewDistanceYInv * size;
    camera->camMatrix.SetRightVector(zVEC3(1.0f, 0.0f, 0.0f));
    camera->camMatrix.SetUpVector(zVEC3(0.0f, 1.0f, 0.0f));
    camera->camMatrix.SetAtVector(zVEC3(0.0f, 0.0f, 1.0f));
    camera->camMatrix.SetTranslation(zVEC3(0.0f, 0.0f, 0.0f));
    camera->camMatrixInv.SetRightVector(zVEC3(1.0f, 0.0f, 0.0f));
    camera->camMatrixInv.SetUpVector(zVEC3(0.0f, 1.0f, 0.0f));
    camera->camMatrixInv.SetAtVector(zVEC3(0.0f, 0.0f, 1.0f));
    camera->camMatrixInv.SetTranslation(zVEC3(-offsetX, -offsetY, 0.0f));
    zCCamera::activeCamPos = camera->camMatrixInv.GetTranslation();
    camera->SetUpFrustum();
}

void zCMesh::RenderDecal(float x, float y, float size, int onTop)
{
    SetupMeshDecalCamera(x, y, size);
    zTRenderContext context = {};
    context.clipFlags = -1;
    RenderDecal(context, onTop);
}

void zCMesh::RenderDecalHorizon(float x, float y, float size, float horizonZ)
{
    SetupMeshDecalCamera(x, y, size);
    zTRenderContext context = {};
    context.cam = zCCamera::activeCam;
    context.clipFlags = -1;
    RenderHorizon(context, horizonZ);
}

void zCMesh::RenderDecal(zTRenderContext& context, int onTop)
{
    if (!numPoly || (context.vob && (context.vob->collButNoMove & 0x20)))
        return;
    if (context.clipFlags == -1) {
        context.clipFlags = 0x3F;
        if (zCCamera::activeCam->BBox3DInFrustum(bbox3D, context.clipFlags) == 1)
            return;
    }

    const unsigned int memoryMark = zCVertexTransform::s_MemMan.Mark();
    const float depthInv = -1.0f / zCCamera::activeCamPos.n[2];
    for (int polyIndex = numPoly - 1; polyIndex >= 0; --polyIndex) {
        zCPolygon* polygon = polyList[polyIndex];
        if (polygon->material)
            polygon->material->ApplyTexAniMapping(polygon);
        if (context.clipFlags <= 0)
            polygon->Unclip();
        else if (!polygon->ClipToFrustum(context.clipFlags))
            continue;

        zCVertexTransform* lastTransform = NULL;
        for (int i = polygon->numClipVert - 1; i >= 0; --i) {
            zCVertex* vertex = polygon->clipVert[i];
            if (!vertex->transformedIndex) {
                lastTransform = vertex->CreateVertexTransform();
                for (int axis = 0; axis < 3; ++axis)
                    lastTransform->vertCamSpace.n[axis] = vertex->position.n[axis] -
                        zCCamera::activeCamPos.n[axis];
                zCCamera::activeCam->ProjectClamp(lastTransform, depthInv);
                lastTransform->vertCamSpaceZInv = onTop
                    ? 1.0f / (zCCamera::activeCam->nearClipZ + 1.0f) : depthInv;
            }
        }
        if (!lastTransform && polygon->numClipVert) {
            zCVertex* vertex = polygon->clipVert[0];
            lastTransform = vertex->transformedIndex
                ? zCVertex::transformedVertexList + vertex->transformedIndex
                : vertex->CreateVertexTransform();
        }
        if (lastTransform && lastTransform->vertCamSpace.n[2] <= zCCamera::activeCam->farClipZ) {
            polygon->LightDynCamSpace(zCCamera::activeCamPos, playerLightInt);
            zrenderer->DrawPoly(polygon);
        }
    }
    zCVertexTransform::s_MemMan.Restore(memoryMark);
}

void zCMesh::CalcVertexNormals(zTCalcVertNormalMode mode, zCBspTree* bspTree)
{
    CreateListsFromArrays();
    UnshareFeatures();
    for (int i = 0; i < numPoly; ++i) {
        zCPolygon* polygon = polyList[i];
        for (int j = 0; j < polygon->polyNumVert; ++j) {
            zCVertFeature* feature = polygon->feature[j];
            feature->vertNormal = polygon->polyPlane.normal;
            if (mode == zMSH_VERTNORMAL_FACET)
                continue;
            if (mode == zMSH_VERTNORMAL_MAT) {
                const int forcedSmooth = (polygon->flags[0] & 8) != 0;
                if (!forcedSmooth && (!polygon->material || !(polygon->material->smooth & 1)))
                    continue;
            }

            for (int k = 0; k < numPoly; ++k) {
                if (k == i)
                    continue;
                zCPolygon* other = polyList[k];
                if (!other->VertPartOfPoly(polygon->vertex[j]))
                    continue;
                if (mode == zMSH_VERTNORMAL_SMOOTH) {
                    feature->vertNormal += other->polyPlane.normal;
                    continue;
                }
                if (other->material != polygon->material)
                    continue;

                const float currentAngle = (polygon->flags[0] & 8)
                    ? 60.0f : polygon->material->smoothAngle;
                const float otherAngle = (other->flags[0] & 8)
                    ? 60.0f : other->material->smoothAngle;
                const float angleLimit = currentAngle < otherAngle ? currentAngle : otherAngle;
                float dot = polygon->polyPlane.normal * other->polyPlane.normal;
                if (dot > 1.0f)
                    dot = 1.0f;
                if (dot < -1.0f)
                    dot = -1.0f;
                const float angle = std::acos(dot) * 57.2957764f;
                if (angle <= angleLimit)
                    feature->vertNormal += other->polyPlane.normal;
            }

            const float length = feature->vertNormal.Length();
            if (length != 0.0f)
                feature->vertNormal *= 1.0f / length;
            else
                feature->vertNormal = polygon->polyPlane.normal;
        }
    }
}

zCMesh* zCMesh::Load(const zSTRING& fileName, int saveGame)
{
    zSTRING sourceName = fileName;
    sourceName.Upper();
    zPATH sourcePath(sourceName);
    const zSTRING meshName = sourcePath.GetFilename() + "." + sourcePath.GetExt();

    zCMesh* mesh = SearchName(meshName);
    if (mesh) {
        mesh->AddRef();
        return mesh;
    }

    int loaded = 0;
    const zSTRING compiledName = sourcePath.GetFilename() + ".MSH";
    if (zCMaterial::s_matUsageDefault) {
        zoptions->ChangeDir(0x12);
        zFILE* file = zfactory->CreateZFile(compiledName);
        if (file->Exists()) {
            mesh = new zCMesh;
            zCClassDef::ObjectCreated(mesh, classDef);
            loaded = mesh->LoadMSH(compiledName, 0);
            if (loaded) {
                mesh->ArraysToLists();
                mesh->SortPolysByMaterial();
            }
        }
        delete file;
    }

    if (!zCMaterial::s_matUsageDefault || !loaded) {
        zoptions->ChangeDir(7);
        zFILE* file = zfactory->CreateZFile(sourceName);
        const int sourceExists = file->Exists();
        delete file;

        if (sourceExists) {
            if (!mesh) {
                mesh = new zCMesh;
                zCClassDef::ObjectCreated(mesh, classDef);
            }

            const zSTRING extension = sourcePath.GetExt();
            if (extension.Search("3DS", 0, 1) != -1) {
                zCFile3DS loader;
                loader.Load3DS(sourceName, mesh);
            }
            else if (extension.Search("BSP", 0, 1) != -1) {
                zCFileQuakeBSP loader;
                loader.LoadQuakeBSP(sourceName, mesh, NULL);
            }

            if (mesh) {
                mesh->meshName = meshName;
                if (zCMaterial::s_matUsageDefault) {
                    if (!saveGame) {
                        mesh->ArraysToLists();
                        ConvertTrisToNPolys(mesh->polyList, mesh->numPoly, mesh, 0);
                    }
                    mesh->SaveMSH(zoptions->GetDirString(0x12) + compiledName);
                }
            }
        }
    }

    return mesh;
}

static int meshState = -1;
static zCClassDef zCMeshClassDef("zCMesh", "zCVisual", zCMesh::_CreateNewInstance,
    0, 0xE4, 1, "");
static zSTRING meshBspExtension(".BSP");
int zCMesh::s_numMeshes = 0;
int zCMesh::s_usesVertexCache = 0;
zCMesh* zCMesh::s_meshRoot = NULL;
zCClassDef* zCMesh::classDef = &zCMeshClassDef;
zCArraySort<zCVertex*> zCMesh::s_vertexCache;

static int CompareMeshVertices(const void* first, const void* second)
{
    const zCVertex* left = *static_cast<zCVertex* const*>(first);
    const zCVertex* right = *static_cast<zCVertex* const*>(second);
    for (int axis = 0; axis < 3; ++axis) {
        if (left->position.n[axis] < right->position.n[axis])
            return -1;
        if (left->position.n[axis] > right->position.n[axis])
            return 1;
    }
    return 0;
}

zCObject* zCMesh::_CreateNewInstance()
{
    zCMesh* mesh = new zCMesh;
    zCClassDef::ObjectCreated(mesh, classDef);
    return mesh;
}

zCMesh::zCMesh()
    : numPoly(0), numVert(0), numFeat(0), vertList(NULL), polyList(NULL), featList(NULL),
      vertArray(NULL), polyArray(NULL), featArray(NULL), masterFrameCtr(0), next(NULL),
      prev(NULL), hasLightmaps(0), m_bUsesAlphaTesting(0), numVertAlloc(0), numPolyAlloc(0),
      numVerts(0), vertexList(NULL)
{
    for (int axis = 0; axis < 3; ++axis) {
        bbox3D.mins.n[axis] = 0;
        bbox3D.maxs.n[axis] = 0;
        obbox3D.center.n[axis] = 0;
        obbox3D.extent.n[axis] = 0;
        for (int component = 0; component < 3; ++component)
            obbox3D.axis[axis].n[component] = 0;
    }

    ++s_numMeshes;
    next = s_meshRoot;
    if (s_meshRoot)
        s_meshRoot->prev = this;
    s_meshRoot = this;
    meshName.Sprintf("#%d", s_numMeshes);
}

zCMesh::~zCMesh()
{
    --s_numMeshes;
    DeleteAll();
    if (s_meshRoot == this)
        s_meshRoot = next;
    else if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;
    prev = NULL;
    next = NULL;
}

int zCVisual::IsBBox3DLocal()
{
    return 1;
}

void zCVisual::SetVisualUsedBy(zCVob* vob)
{
}

zCClassDef* zCMesh::_GetClassDef() const
{
    return classDef;
}

zSTRING zCMesh::GetVisualName()
{
    return meshName;
}

zTBBox3D zCMesh::GetBBox3D()
{
    return bbox3D;
}

zCOBBox3D* zCMesh::GetOBBox3D()
{
    return &obbox3D;
}

int zCMesh::CanTraceRay() const
{
    return 1;
}

int zCMesh::GetAlphaTestingEnabled() const
{
    return m_bUsesAlphaTesting;
}

void zCMesh::SetAlphaTestingEnabled(int enabled)
{
    m_bUsesAlphaTesting = enabled;
}

const zSTRING* zCMesh::GetFileExtension(int index)
{
    static zSTRING meshExtension(".MSH");
    static zSTRING threeDsExtension(".3DS");
    return index ? &meshExtension : &threeDsExtension;
}

zCVisual* zCMesh::LoadVisualVirtual(const zSTRING& fileName) const
{
    return Load(fileName, 0);
}

void zCMesh::DeleteAll()
{
    if (polyList) {
        for (int i = 0; i < numPoly; ++i) {
            if (polyList[i]) {
                if (featList || featArray) {
                    zCPolygon::S_DeletePointers(polyList[i]->polyNumVert, polyList[i]->feature);
                    polyList[i]->feature = NULL;
                }
                delete polyList[i];
            }
        }
    } else if (polyArray && (featList || featArray)) {
        for (int i = 0; i < numPoly; ++i) {
            zCPolygon::S_DeletePointers(polyArray[i].polyNumVert, polyArray[i].feature);
            polyArray[i].feature = NULL;
        }
    }

    delete[] vertList;
    delete[] polyList;
    delete[] featList;
    delete[] vertArray;
    delete[] polyArray;
    delete[] featArray;
    delete[] vertexList;

    vertList = NULL;
    polyList = NULL;
    featList = NULL;
    vertArray = NULL;
    polyArray = NULL;
    featArray = NULL;
    vertexList = NULL;
    numFeat = 0;
    numVert = 0;
    numPoly = 0;
    numVerts = 0;
    numVertAlloc = 0;
    numPolyAlloc = 0;
}

void zCMesh::AllocVerts(int count)
{
    CreateListsFromArrays();
    if (count <= 0)
        count = 1;

    zCVertex** newList = new zCVertex*[numVertAlloc + count];
    for (int i = 0; i < numVert; ++i)
        newList[i] = vertList[i];
    if (numVertAlloc)
        delete[] vertList;
    vertList = newList;
    numVertAlloc += count;
}

void zCMesh::AllocPolys(int count)
{
    CreateListsFromArrays();
    if (count <= 0)
        count = 1;

    zCPolygon** newList = new zCPolygon*[numPolyAlloc + count];
    for (int i = 0; i < numPoly; ++i)
        newList[i] = polyList[i];
    if (numPolyAlloc)
        delete[] polyList;
    polyList = newList;
    numPolyAlloc += count;
}

zCVertex* zCMesh::AddVertex(zCVertex* vertex)
{
    if (numVertAlloc < numVert + 1)
        AllocVerts(numVert / 2 > 1 ? numVert / 2 : 1);
    vertList[numVert++] = vertex;
    if (s_usesVertexCache)
        s_vertexCache.InsertSort(vertex);
    return vertex;
}

zCVertex* zCMesh::AddVertex(zVEC3& position)
{
    zCVertex* vertex = new zCVertex;
    vertex->position = position;
    return AddVertex(vertex);
}

void zCMesh::AddVertex(const zVEC3& value)
{
    zVEC3 position = value;
    AddVertex(position);
}

zCVertex* zCMesh::AddVertexSmart(zVEC3& position)
{
    CreateListsFromArrays();
    zCVertex* vertex = VertexInMesh(position);
    return vertex ? vertex : AddVertex(position);
}

zCPolygon* zCMesh::AddPoly(zVEC3& firstPosition, zVEC3& secondPosition,
    zVEC3& thirdPosition, zVEC2& firstMapping, zVEC2& secondMapping, zVEC2& thirdMapping)
{
    zCPolygon* polygon = AddPoly();
    zCVertex* first = AddVertexSmart(firstPosition);
    zCVertex* second = AddVertexSmart(secondPosition);
    zCVertex* third = AddVertexSmart(thirdPosition);
    polygon->SetMapping(0, firstMapping);
    polygon->SetMapping(1, secondMapping);
    polygon->SetMapping(2, thirdMapping);
    polygon->SetVertices(first, second, third);
    return polygon;
}

void zCMesh::DynLightVisual(const zCArray<zCVobLight*>& lights, zMAT4* trafo)
{
}

void zCMesh::DynLightScalePrelit(zCOLOR color)
{
}

void zCMesh::PrelightMesh(zMAT4* trafo)
{
    CreateListsFromArrays();
    zCArray<zCVobLight*> lights;
    zCVobLight* light = new zCVobLight;
    light->position = zVEC3(100.0f, 400.0f, -400.0f);
    light->range = 1100.0f;
    light->color[0] = 255;
    light->color[1] = 255;
    light->color[2] = 255;
    lights.InsertEnd(light);

    light = new zCVobLight;
    light->position = zVEC3(50.0f, 400.0f, 400.0f);
    light->range = 1100.0f;
    light->color[0] = 255;
    light->color[1] = 255;
    light->color[2] = 255;
    lights.InsertEnd(light);

    light = new zCVobLight;
    light->position = zVEC3(300.0f, 400.0f, -400.0f);
    light->range = 1000.0f;
    light->color[0] = 205;
    light->color[1] = 205;
    light->color[2] = 205;
    lights.InsertEnd(light);

    light = new zCVobLight;
    light->position = zVEC3(-50.0f, -400.0f, -400.0f);
    light->range = 1100.0f;
    light->color[0] = 205;
    light->color[1] = 205;
    light->color[2] = 205;
    lights.InsertEnd(light);

    ResetStaticLight();
    for (int i = 0; i < numPoly; ++i)
        SharePoly(i)->ResetLightDynToLightStat();

    zMAT4 inverse;
    zMAT4* lightTrafo = NULL;
    if (trafo) {
        inverse = trafo->InverseLinTrafo();
        lightTrafo = &inverse;
    }
    zCPolygon::LightPolyList(lights, polyList, numPoly, lightTrafo);

    for (int i = 0; i < numPoly; ++i) {
        zCPolygon* polygon = polyList[i];
        for (int j = 0; j < polygon->polyNumVert; ++j) {
            for (int channel = 0; channel < 4; ++channel)
                polygon->feature[j]->lightStat[channel] = polygon->feature[j]->color[channel];
        }
    }

    for (int i = 0; i < lights.numInArray; ++i)
        lights[i]->Release();
}

zCPolygon* zCMesh::AddPoly(zCVertex* first, zCVertex* second, zCVertex* third)
{
    zCPolygon* polygon = AddPoly();
    polygon->SetVertices(first, second, third);
    return polygon;
}

zCPolygon* zCMesh::AddPoly(zCVertex* first, zCVertex* second, zCVertex* third,
    zCVertex* fourth)
{
    zCPolygon* polygon = AddPoly();
    polygon->SetVertices(first, second, third, fourth, 1);
    return polygon;
}

zCPolygon* zCMesh::AddPoly(zCPolygon* polygon)
{
    CreateListsFromArrays();
    UnshareFeatures();
    if (numPolyAlloc < numPoly + 1)
        AllocPolys(numPoly / 2);
    polyList[numPoly++] = polygon;
    return polygon;
}

zCPolygon* zCMesh::AddPoly()
{
    return AddPoly(new zCPolygon);
}

void zCMesh::AddPoly(const zCPolygon& value)
{
    AddPoly(new zCPolygon(value));
}

void zCMesh::DeletePoly(zCPolygon* polygon)
{
    CreateListsFromArrays();
    UnshareFeatures();
    for (int i = 0; i < numPoly; ++i) {
        if (polyList[i] != polygon)
            continue;
        delete polygon;
        for (int j = i; j < numPoly - 1; ++j)
            polyList[j] = polyList[j + 1];
        --numPoly;
        return;
    }
}

static void GetMeshCameraRay(zCCamera& camera, int x, int y, zVEC3& origin, zVEC3& direction)
{
    for (int axis = 0; axis < 3; ++axis)
        origin.n[axis] = camera.camMatrixInv.v[axis].n[3];
    const float screenX = (x - camera.vpData.xcenter) * camera.viewDistanceXInv * 60000.0f;
    const float screenY = (camera.vpData.ycenter - y) * camera.viewDistanceYInv * 60000.0f;
    for (int axis = 0; axis < 3; ++axis) {
        direction.n[axis] = screenX * camera.camMatrixInv.v[axis].n[0] +
            screenY * camera.camMatrixInv.v[axis].n[1] +
            60000.0f * camera.camMatrixInv.v[axis].n[2];
    }
}

zCPolygon* zCMesh::CheckRayPolyIntersection(const zVEC3& origin, const zVEC3& direction,
    zVEC3& intersection)
{
    for (int i = 0; i < numPoly; ++i) {
        zCPolygon* polygon = polyList[i];
        float distance;
        if (polygon->CheckRayPolyIntersection(origin, direction, intersection, distance)
            && distance < 1.0f)
            return polygon;
    }
    return NULL;
}

zCPolygon* zCMesh::CheckRayPolyIntersection(zCCamera& camera, int x, int y,
    zVEC3& intersection)
{
    zVEC3 origin;
    zVEC3 direction;
    GetMeshCameraRay(camera, x, y, origin, direction);
    return CheckRayPolyIntersection(origin, direction, intersection);
}

zCPolygon* zCMesh::PickPolygon(const zVEC3& origin, const zVEC3& direction,
    zVEC3& intersection)
{
    float nearest = 3.402823466e38F;
    zCPolygon* result = NULL;
    zVEC3 bestIntersection;
    for (int i = 0; i < numPoly; ++i) {
        zCPolygon* polygon = polyList[i];
        if (polygon->flags[1] & 3)
            continue;
        float distance;
        if (polygon->CheckRayPolyIntersection(origin, direction, intersection, distance)
            && distance < nearest) {
            nearest = distance;
            result = polygon;
            bestIntersection = intersection;
        }
    }
    if (result)
        intersection = bestIntersection;
    return result;
}

zCPolygon* zCMesh::PickPolygon(zCCamera& camera, int x, int y, zVEC3& intersection)
{
    zVEC3 origin;
    zVEC3 direction;
    GetMeshCameraRay(camera, x, y, origin, direction);
    return PickPolygon(origin, direction, intersection);
}

int zCMesh::TraceRay(const zVEC3& origin, const zVEC3& direction, int flags,
    zTTraceRayReport& report)
{
    report.foundHit = 0;
    zCPolygon* found = NULL;
    float nearest = 3.402823466e38F;

    if (flags & 0x10) {
        for (int i = 0; i < numPoly; ++i) {
            zCPolygon* polygon = polyList[i];
            float distance;
            if (polygon->CheckRayPolyIntersection2Sided(origin, direction,
                    report.foundIntersection, distance) && distance < 1.0f) {
                found = polygon;
                break;
            }
        }
    }
    else {
        zVEC3 bestIntersection;
        for (int i = 0; i < numPoly; ++i) {
            zCPolygon* polygon = polyList[i];
            if (polygon->flags[1] & 3)
                continue;
            float distance;
            if (polygon->CheckRayPolyIntersection(origin, direction,
                    report.foundIntersection, distance) && distance < nearest) {
                nearest = distance;
                found = polygon;
                bestIntersection = report.foundIntersection;
            }
        }
        if (found)
            report.foundIntersection = bestIntersection;
    }

    if (found) {
        int accept = 1;
        if ((flags & 0x100) && found->material && found->material->matGroup == 1
            && found->material->texture)
            accept = 0;
        if (accept) {
            if (flags & 0x8000)
                report.foundPolyNormal = found->polyPlane.normal;
            report.foundHit = 1;
            report.foundPoly = found;
        }
    }
    return report.foundHit;
}

zCPolygon* zCMesh::SharePoly(int index) const
{
    return polyArray ? &polyArray[index] : polyList[index];
}

void zCMesh::SetMaterial(zCMaterial* material)
{
    for (int i = 0; i < numPoly; ++i) {
        zCPolygon* polygon = SharePoly(i);
        if (polygon->material != material) {
            if (polygon->material)
                polygon->material->Release();
            if (material)
                material->AddRef();
            polygon->material = material;
        }
    }
}

void zCMesh::ScaleXZ(float scaleX, float scaleZ)
{
    for (int i = 0; i < numVert; ++i) {
        vertList[i]->position.n[0] *= scaleX;
        vertList[i]->position.n[2] *= scaleZ;
    }
    if (vertexList && !vertList) {
        for (int i = 0; i < numVerts; ++i) {
            vertexList[i].n[0] *= scaleX;
            vertexList[i].n[2] *= scaleZ;
        }
    }
    bbox3D.mins.n[0] *= scaleX;
    bbox3D.maxs.n[0] *= scaleX;
    bbox3D.mins.n[2] *= scaleZ;
    bbox3D.maxs.n[2] *= scaleZ;
}

void zCMesh::Scale(float scaleX, float scaleY, float scaleZ)
{
    const float scale[3] = { scaleX, scaleY, scaleZ };
    for (int i = 0; i < numVert; ++i) {
        for (int axis = 0; axis < 3; ++axis)
            vertList[i]->position.n[axis] *= scale[axis];
    }
    if (vertexList && !vertList) {
        for (int i = 0; i < numVerts; ++i) {
            for (int axis = 0; axis < 3; ++axis)
                vertexList[i].n[axis] *= scale[axis];
        }
    }
    for (int axis = 0; axis < 3; ++axis) {
        bbox3D.mins.n[axis] *= scale[axis];
        bbox3D.maxs.n[axis] *= scale[axis];
    }
}

void zCMesh::UnSmoothMesh()
{
    CreateListsFromArrays();
    UnshareFeatures();
    for (int i = 0; i < numPoly; ++i) {
        zCMaterial* material = SharePoly(i)->material;
        if (material)
            material->smooth &= ~1;
    }
}

void zCMesh::SetDefaultPlanarMapping(float scaleU, float scaleV)
{
    CreateListsFromArrays();
    UnshareFeatures();
    for (int i = 0; i < numPoly; ++i)
        SharePoly(i)->TexApplyPlanarMapping(scaleU, scaleV, 1);
}

void zCMesh::CalcBBox3D(int updateOBBox)
{
    CreateListsFromArrays();
    UnshareFeatures();
    for (int axis = 0; axis < 3; ++axis) {
        bbox3D.mins.n[axis] = 3.402823466e38F;
        bbox3D.maxs.n[axis] = -3.402823466e38F;
    }
    for (int i = 0; i < numVert; ++i) {
        for (int axis = 0; axis < 3; ++axis) {
            const float value = vertList[i]->position.n[axis];
            if (value < bbox3D.mins.n[axis])
                bbox3D.mins.n[axis] = value;
            if (value > bbox3D.maxs.n[axis])
                bbox3D.maxs.n[axis] = value;
        }
    }
    if (!numVert || !numPoly) {
        for (int axis = 0; axis < 3; ++axis) {
            bbox3D.mins.n[axis] = -0.1f;
            bbox3D.maxs.n[axis] = 0.1f;
        }
    }
}

void zCMesh::CalcBBox3D()
{
    CalcBBox3D(0);
}

void zCMesh::Center()
{
    zVEC3 center = { 0, 0, 0 };
    for (int i = 0; i < numVert; ++i) {
        for (int axis = 0; axis < 3; ++axis)
            center.n[axis] += vertList[i]->position.n[axis];
    }
    for (int axis = 0; axis < 3; ++axis)
        center.n[axis] /= numVert;
    for (int i = 0; i < numVert; ++i) {
        for (int axis = 0; axis < 3; ++axis)
            vertList[i]->position.n[axis] -= center.n[axis];
    }
    CalcNormals();
    CalcBBox3D(0);
}

void zCMesh::TransformAllVerts(const zMAT4& trafo, int calcNormals)
{
    for (int i = 0; i < numVert; ++i)
        vertList[i]->position = trafo * vertList[i]->position;
    if (calcNormals) {
        CalcNormals();
        CalcBBox3D(0);
    }
}

void zCMesh::TransformAllVerts(const zMAT4& trafo)
{
    TransformAllVerts(trafo, 1);
}

int s_bAddVobsToMesh = 0;

void zCMesh::MergeMesh(zCMesh* source, const zMAT4& trafo)
{
    CreateListsFromArrays();
    source->CreateListsFromArrays();
    UnshareFeatures();
    AllocVerts(source->numVert);
    AllocPolys(source->numPoly);

    if (!s_bAddVobsToMesh)
        S_InitVertexMergeCache(this);

    zCVertex** vertexMap = new zCVertex*[source->numVert];
    for (int i = 0; i < source->numVert; ++i) {
        zVEC3 position = trafo * source->vertList[i]->position;
        zCVertex* vertex = NULL;
        if (!s_bAddVobsToMesh)
            vertex = VertexInMesh(position);
        if (!vertex)
            vertex = AddVertex(position);
        vertexMap[i] = vertex;
    }

    for (int i = 0; i < source->numPoly; ++i) {
        zCPolygon* sourcePolygon = source->polyList[i];
        zCPolygon* polygon = AddPoly();
        sourcePolygon->CopyPolyInto(polygon);
        for (int j = 0; j < polygon->polyNumVert; ++j) {
            for (int k = 0; k < source->numVert; ++k) {
                if (sourcePolygon->vertex[j] == source->vertList[k]) {
                    polygon->vertex[j] = vertexMap[k];
                    break;
                }
            }
        }
        polygon->CalcNormal();
    }

    delete[] vertexMap;
    if (!s_bAddVobsToMesh)
        S_DeleteVertexMergeCache();
}

void zCMesh::CalcNormals()
{
    for (int i = 0; i < numPoly; ++i)
        SharePoly(i)->CalcNormal();
}

void zCMesh::S_InitVertexMergeCache(zCMesh* mesh)
{
    s_vertexCache.DeleteList();
    s_vertexCache.Compare = CompareMeshVertices;
    for (int i = 0; i < mesh->numVert; ++i)
        s_vertexCache.InsertSort(mesh->vertList[i]);
    s_usesVertexCache = 1;
}

void zCMesh::S_DeleteVertexMergeCache()
{
    s_vertexCache.DeleteList();
    s_usesVertexCache = 0;
}

zCVertex* zCMesh::VertexInMesh(const zVEC3& position)
{
    CreateListsFromArrays();
    if (s_usesVertexCache) {
        int first = 0;
        int last = s_vertexCache.numInArray - 1;
        while (first <= last) {
            const int middle = (first + last) / 2;
            zCVertex* vertex = s_vertexCache.array[middle];
            int comparison = 0;
            for (int axis = 0; axis < 3; ++axis) {
                if (position.n[axis] < vertex->position.n[axis]) {
                    comparison = -1;
                    break;
                }
                if (position.n[axis] > vertex->position.n[axis]) {
                    comparison = 1;
                    break;
                }
            }
            if (!comparison)
                return vertex;
            if (comparison < 0)
                last = middle - 1;
            else
                first = middle + 1;
        }
        return NULL;
    }

    for (int i = 0; i < numVert; ++i) {
        zCVertex* vertex = vertList[i];
        bool equal = true;
        for (int axis = 0; axis < 3; ++axis) {
            const float delta = vertex->position.n[axis] - position.n[axis];
            if ((delta < 0 ? -delta : delta) >= 0.00999999978f) {
                equal = false;
                break;
            }
        }
        if (equal)
            return vertex;
    }
    return NULL;
}

zCMesh* zCMesh::SearchName(const zSTRING& name)
{
    for (zCMesh* mesh = s_meshRoot; mesh; mesh = mesh->next) {
        if (mesh->meshName == name)
            return mesh;
    }
    return NULL;
}

void zCMesh::ResetStaticLight()
{
    for (int i = 0; i < numPoly; ++i)
        SharePoly(i)->ResetStaticLight();
}

void zCMesh::SetStaticLight(zCOLOR color)
{
    for (int i = 0; i < numPoly; ++i)
        SharePoly(i)->SetStaticLight(color.dword);
}

void zCMesh::SetLightDyn(zCOLOR color)
{
    for (int i = 0; i < numPoly; ++i)
        SharePoly(i)->SetLightDyn(color.dword);
}

void zCMesh::SetLightStatAlpha(unsigned char alpha)
{
    for (int i = 0; i < numPoly; ++i)
        SharePoly(i)->SetLightStatAlpha(alpha);
}

void zCMesh::ResetLightDynToLightStat()
{
    for (int i = 0; i < numPoly; ++i)
        SharePoly(i)->ResetLightDynToLightStat();
}

void zCMesh::AddToLightDyn(zCOLOR color)
{
    for (int i = 0; i < numPoly; ++i)
        SharePoly(i)->AddToLightDyn(color.dword);
}

zCMesh* zCMesh::CreateQuadMesh(int twoSided)
{
    zCMesh* mesh = new zCMesh;
    zCClassDef::ObjectCreated(mesh, classDef);
    mesh->AllocVerts(4);
    zVEC3 first(-50.0f, 50.0f, 0.0f);
    zVEC3 second(50.0f, 50.0f, 0.0f);
    zVEC3 third(50.0f, -50.0f, 0.0f);
    zVEC3 fourth(-50.0f, -50.0f, 0.0f);
    zCVertex* v0 = mesh->AddVertex(first);
    zCVertex* v1 = mesh->AddVertex(second);
    zCVertex* v2 = mesh->AddVertex(third);
    zCVertex* v3 = mesh->AddVertex(fourth);

    zCPolygon* polygon = mesh->AddPoly(v0, v1, v2, v3);
    polygon->SetMapping(0, zVEC2(0.0f, 0.99f));
    polygon->SetMapping(1, zVEC2(1.0f, 0.99f));
    polygon->SetMapping(2, zVEC2(1.0f, 0.0f));
    polygon->SetMapping(3, zVEC2(0.0f, 0.0f));
    if (twoSided) {
        polygon = mesh->AddPoly(v3, v2, v1, v0);
        polygon->SetMapping(0, zVEC2(0.0f, 0.0f));
        polygon->SetMapping(1, zVEC2(1.0f, 0.0f));
        polygon->SetMapping(2, zVEC2(1.0f, 0.99f));
        polygon->SetMapping(3, zVEC2(0.0f, 0.99f));
    }
    mesh->CalcBBox3D(0);
    mesh->CalcVertexNormals(zMSH_VERTNORMAL_FACET, NULL);
    return mesh;
}

zCMesh* zCMesh::CreateTriMesh()
{
    zCMesh* mesh = new zCMesh;
    zCClassDef::ObjectCreated(mesh, classDef);
    mesh->AllocVerts(3);
    zVEC3 first(-50.0f, 50.0f, 0.0f);
    zVEC3 second(50.0f, 50.0f, 0.0f);
    zVEC3 third(50.0f, -50.0f, 0.0f);
    zCVertex* v0 = mesh->AddVertex(first);
    zCVertex* v1 = mesh->AddVertex(second);
    zCVertex* v2 = mesh->AddVertex(third);
    zCPolygon* polygon = mesh->AddPoly(v2, v1, v0);
    polygon->SetMapping(0, zVEC2(0.0f, 1.0f));
    polygon->SetMapping(1, zVEC2(1.0f, 0.0f));
    polygon->SetMapping(2, zVEC2(0.0f, 0.0f));
    mesh->CalcBBox3D(0);
    mesh->CalcVertexNormals(zMSH_VERTNORMAL_FACET, NULL);
    return mesh;
}

int zCMesh::CheckTexUVOnPlane(const zCArray<zCVertex*>& vertices,
    const zCArray<zCVertFeature>& features)
{
    const zVEC3 origin = vertices[0]->position;
    const zVEC3 firstEdge = vertices[1]->position - origin;
    for (int base = 2; base < vertices.numInArray; ++base) {
        const zVEC3 secondEdge = vertices[base]->position - origin;
        const float firstLength2 = firstEdge * firstEdge;
        const float secondLength2 = secondEdge * secondEdge;
        const float edgeDot = firstEdge * secondEdge;
        const float denominator = firstLength2 * secondLength2 - edgeDot * edgeDot;
        if (denominator == 0.0f)
            continue;

        for (int i = 0; i < vertices.numInArray; ++i) {
            if (i == 0 || i == 1 || i == base)
                continue;
            const zVEC3 delta = vertices[i]->position - origin;
            const float deltaFirst = delta * firstEdge;
            const float deltaSecond = delta * secondEdge;
            const float firstWeight = (deltaFirst * secondLength2 - deltaSecond * edgeDot) /
                denominator;
            const float secondWeight = (deltaSecond * firstLength2 - deltaFirst * edgeDot) /
                denominator;
            for (int channel = 0; channel < 2; ++channel) {
                const float expected = features[0].mapping.n[channel] + firstWeight *
                    (features[1].mapping.n[channel] - features[0].mapping.n[channel]) +
                    secondWeight * (features[base].mapping.n[channel] -
                        features[0].mapping.n[channel]);
                if (std::fabs(features[i].mapping.n[channel] - expected) > 0.05f)
                    return 0;
            }
        }
        return 1;
    }
    return 1;
}

zVEC2 operator+(const zVEC2& first, const zVEC2& second)
{
    return zVEC2(first.n[0] + second.n[0], first.n[1] + second.n[1]);
}

static int MeshMappingsEqual(const zCVertFeature* first, const zCVertFeature* second)
{
    return std::fabs(first->mapping.n[0] - second->mapping.n[0]) <= 0.05f &&
        std::fabs(first->mapping.n[1] - second->mapping.n[1]) <= 0.05f;
}

void zCMesh::ConvertTrisToNPolys(zCPolygon** polygons, int count, zCMesh* mesh, int flags)
{
    (void)flags;
    zCArray<zCPolygon*> remaining;
    for (int i = 0; i < count; ++i) {
        if (polygons[i] && polygons[i]->polyNumVert == 3)
            remaining.InsertEnd(polygons[i]);
    }

    int index = 0;
    while (index < remaining.numInArray) {
        zCPolygon* current = remaining[index];
        int merged = 0;
        for (int otherIndex = 0; otherIndex < remaining.numInArray && !merged; ++otherIndex) {
            zCPolygon* other = remaining[otherIndex];
            if (other == current || other->material != current->material)
                continue;
            if (std::fabs(other->polyPlane.distance - current->polyPlane.distance) > 0.1f)
                continue;
            if (current->polyPlane.normal * other->polyPlane.normal < 0.8f)
                continue;

            for (int edge = 0; edge < current->polyNumVert && !merged; ++edge) {
                const int nextEdge = (edge + 1) % current->polyNumVert;
                zCVertex* first = current->vertex[edge];
                zCVertex* second = current->vertex[nextEdge];
                for (int otherEdge = 0; otherEdge < other->polyNumVert; ++otherEdge) {
                    const int nextOtherEdge = (otherEdge + 1) % other->polyNumVert;
                    if (other->vertex[otherEdge] != second || other->vertex[nextOtherEdge] != first)
                        continue;
                    if (!MeshMappingsEqual(current->feature[edge], other->feature[nextOtherEdge])
                        || !MeshMappingsEqual(current->feature[nextEdge], other->feature[otherEdge]))
                        continue;

                    zCArray<zCVertex*> mergedVertices;
                    zCArray<zCVertFeature> mergedFeatures;
                    for (int i = 0; i < current->polyNumVert; ++i) {
                        if (i == nextEdge)
                            continue;
                        mergedVertices.InsertEnd(current->vertex[i]);
                        mergedFeatures.InsertEnd(*current->feature[i]);
                        if (i == edge) {
                            for (int j = 2; j < other->polyNumVert; ++j) {
                                const int otherVertex = (otherEdge + j) % other->polyNumVert;
                                mergedVertices.InsertEnd(other->vertex[otherVertex]);
                                mergedFeatures.InsertEnd(*other->feature[otherVertex]);
                            }
                        }
                    }
                    if (mergedVertices.numInArray > 32
                        || !CheckTexUVOnPlane(mergedVertices, mergedFeatures))
                        break;

                    current->RemoveVerticesAndFeatures();
                    current->AllocVerts(mergedVertices.numInArray);
                    for (int i = 0; i < mergedVertices.numInArray; ++i) {
                        current->vertex[i] = mergedVertices[i];
                        *current->feature[i] = mergedFeatures[i];
                    }
                    current->CalcNormal();
                    remaining.RemoveIndex(otherIndex);
                    if (mesh)
                        mesh->DeletePoly(other);
                    merged = 1;
                    break;
                }
            }
        }
        if (!merged)
            ++index;
    }
}

template <class T>
static int MeshPointerIndex(zCArray<T*>& values, T* value)
{
    for (int i = 0; i < values.numInArray; ++i) {
        if (values[i] == value)
            return i;
    }
    return -1;
}

template <class T>
static int MeshAddUniquePointer(zCArray<T*>& values, T* value)
{
    int index = MeshPointerIndex(values, value);
    if (index < 0) {
        index = values.numInArray;
        values.InsertEnd(value);
    }
    return index;
}

void zCMesh::SaveMSH(zCFileBIN& file)
{
    SaveMSH(file, polyList, numPoly, this);
}

void zCMesh::SaveMSH(const zSTRING& fileName)
{
    zCFileBIN file;
    file.file = zfactory->CreateZFile(zoptions->GetDirString(0x12) + fileName);
    file.externalFile = 0;
    file.file->Create();
    SaveMSH(file);
    file.BinClose();
}

void zCMesh::SaveMSH(zCFileBIN& file, zCPolygon** polygons, int count, zCMesh* mesh)
{
    zCArray<zCVertex*> vertices;
    zCArray<zCVertFeature*> features;
    zCArray<zCMaterial*> materials;
    zCArray<zCLightMap*> lightmaps;
    for (int i = 0; i < count; ++i) {
        zCPolygon* polygon = polygons[i];
        if (polygon->material)
            MeshAddUniquePointer(materials, polygon->material);
        if (polygon->lightmap)
            MeshAddUniquePointer(lightmaps, polygon->lightmap);
        for (int j = 0; j < polygon->polyNumVert; ++j) {
            MeshAddUniquePointer(vertices, polygon->vertex[j]);
            MeshAddUniquePointer(features, polygon->feature[j]);
        }
    }

    file.BinStartChunk(0xB000);
    const unsigned short version = GetFileVersion();
    file.file->Write(&version, sizeof(version));
    file.BinEndChunk();

    if (mesh) {
        file.BinStartChunk(0xB010);
        mesh->bbox3D.SaveBIN(file);
        mesh->obbox3D.SaveBIN(file);
        file.BinEndChunk();
    }

    file.BinStartChunk(0xB020);
    file.file->Write(&materials.numInArray, sizeof(materials.numInArray));
    for (int i = 0; i < materials.numInArray; ++i)
        file.BinWriteString(materials[i]->GetObjectName());
    file.BinEndChunk();

    if (lightmaps.numInArray) {
        file.BinStartChunk(0xB026);
        file.file->Write(&lightmaps.numInArray, sizeof(lightmaps.numInArray));
        for (int i = 0; i < lightmaps.numInArray; ++i) {
            zCLightMap* lightmap = lightmaps[i];
            file.file->Write(&lightmap->lightmapOrigin, sizeof(lightmap->lightmapOrigin));
            file.file->Write(&lightmap->lightmapUVUp, sizeof(lightmap->lightmapUVUp));
            file.file->Write(&lightmap->lightmapUVRight, sizeof(lightmap->lightmapUVRight));
            file.BinWriteString(lightmap->tex ? lightmap->tex->GetObjectName() : zSTRING());
            file.file->Write(&lightmap->xdim, sizeof(lightmap->xdim));
            file.file->Write(&lightmap->ydim, sizeof(lightmap->ydim));
        }
        file.BinEndChunk();
    }

    file.BinStartChunk(0xB030);
    file.file->Write(&vertices.numInArray, sizeof(vertices.numInArray));
    for (int i = 0; i < vertices.numInArray; ++i) {
        vertices[i]->myIndex = i;
        file.file->Write(&vertices[i]->position, sizeof(vertices[i]->position));
    }
    file.BinEndChunk();

    file.BinStartChunk(0xB040);
    file.file->Write(&features.numInArray, sizeof(features.numInArray));
    for (int i = 0; i < features.numInArray; ++i) {
        zCVertFeature* feature = features[i];
        file.file->Write(&feature->vertNormal, sizeof(feature->vertNormal));
        file.file->Write(feature->mapping.n, sizeof(float) * 2);
        file.file->Write(feature->color, sizeof(feature->color));
    }
    file.BinEndChunk();

    file.BinStartChunk(0xB050);
    file.file->Write(&count, sizeof(count));
    for (int i = 0; i < count; ++i) {
        zCPolygon* polygon = polygons[i];
        const int materialIndex = MeshPointerIndex(materials, polygon->material);
        const short lightmapIndex = static_cast<short>(MeshPointerIndex(lightmaps, polygon->lightmap));
        const unsigned short reserved = 0;
        file.file->Write(&materialIndex, sizeof(materialIndex));
        file.file->Write(&lightmapIndex, sizeof(lightmapIndex));
        file.file->Write(&reserved, sizeof(reserved));
        file.file->Write(&polygon->polyPlane, sizeof(polygon->polyPlane));
        const unsigned char polygonFlags[4] = {
            polygon->flags[0], polygon->flags[1], polygon->flags[2], polygon->polyNumVert
        };
        file.file->Write(polygonFlags, sizeof(polygonFlags));
        for (int j = 0; j < polygon->polyNumVert; ++j) {
            const int vertexIndex = MeshPointerIndex(vertices, polygon->vertex[j]);
            const int featureIndex = MeshPointerIndex(features, polygon->feature[j]);
            file.file->Write(&vertexIndex, sizeof(vertexIndex));
            file.file->Write(&featureIndex, sizeof(featureIndex));
        }
    }
    file.BinEndChunk();

    file.BinStartChunk(0xB060);
    file.BinEndChunk();
    for (int i = 0; i < vertices.numInArray; ++i)
        vertices[i]->myIndex = 0;
}

int zCMesh::LoadMSH(zCFileBIN& file, int saveGame)
{
    (void)saveGame;
    DeleteAll();
    zCArray<zCMaterial*> materials;
    zCArray<zCLightMap*> lightmaps;
    int hasBounds = 0;
    int finished = 0;

    while (!finished && !file.file->Eof()) {
        unsigned short chunkId;
        unsigned long chunkSize;
        if (file.file->Read(&chunkId, sizeof(chunkId)) != sizeof(chunkId))
            break;
        if (file.file->Read(&chunkSize, sizeof(chunkSize)) != sizeof(chunkSize))
            break;
        const long chunkEnd = file.file->Pos() + chunkSize;

        switch (chunkId) {
        case 0xB000: {
            unsigned short version;
            file.file->Read(&version, sizeof(version));
            if (version != GetFileVersion())
                return 0;
            break;
        }
        case 0xB010:
            bbox3D.LoadBIN(file);
            obbox3D.LoadBIN(file);
            hasBounds = 1;
            break;
        case 0xB020: {
            int materialCount;
            file.file->Read(&materialCount, sizeof(materialCount));
            for (int i = 0; i < materialCount; ++i) {
                zSTRING name;
                file.file->Read(name);
                zCMaterial* material = zCMaterial::SearchName(name);
                if (material)
                    material->AddRef();
                else {
                    material = new zCMaterial(name);
                    zCClassDef::ObjectCreated(material, zCMaterial::classDef);
                }
                materials.InsertEnd(material);
            }
            break;
        }
        case 0xB026: {
            int lightmapCount;
            file.file->Read(&lightmapCount, sizeof(lightmapCount));
            for (int i = 0; i < lightmapCount; ++i) {
                zCLightMap* lightmap = new zCLightMap;
                zCClassDef::ObjectCreated(lightmap, zCLightMap::classDef);
                file.file->Read(&lightmap->lightmapOrigin, sizeof(lightmap->lightmapOrigin));
                file.file->Read(&lightmap->lightmapUVUp, sizeof(lightmap->lightmapUVUp));
                file.file->Read(&lightmap->lightmapUVRight, sizeof(lightmap->lightmapUVRight));
                zSTRING textureName;
                file.file->Read(textureName);
                lightmap->SetTexture(zCTexture::Load(textureName, 0));
                file.file->Read(&lightmap->xdim, sizeof(lightmap->xdim));
                file.file->Read(&lightmap->ydim, sizeof(lightmap->ydim));
                lightmaps.InsertEnd(lightmap);
            }
            hasLightmaps = 1;
            break;
        }
        case 0xB030: {
            int vertexCount;
            file.file->Read(&vertexCount, sizeof(vertexCount));
            AllocVerts(vertexCount);
            for (int i = 0; i < vertexCount; ++i) {
                zVEC3 position;
                file.file->Read(&position, sizeof(position));
                AddVertex(position);
            }
            break;
        }
        case 0xB040: {
            int featureCount;
            file.file->Read(&featureCount, sizeof(featureCount));
            featArray = new zCVertFeature[featureCount];
            featList = new zCVertFeature*[featureCount];
            numFeat = featureCount;
            for (int i = 0; i < featureCount; ++i) {
                featList[i] = &featArray[i];
                file.file->Read(&featArray[i].vertNormal, sizeof(featArray[i].vertNormal));
                file.file->Read(featArray[i].mapping.n, sizeof(float) * 2);
                featArray[i].mapping.n[2] = 0.0f;
                file.file->Read(featArray[i].color, sizeof(featArray[i].color));
            }
            break;
        }
        case 0xB050: {
            int polygonCount;
            file.file->Read(&polygonCount, sizeof(polygonCount));
            AllocPolys(polygonCount);
            for (int i = 0; i < polygonCount; ++i) {
                int materialIndex;
                short lightmapIndex;
                unsigned short reserved;
                unsigned char polygonFlags[4];
                file.file->Read(&materialIndex, sizeof(materialIndex));
                file.file->Read(&lightmapIndex, sizeof(lightmapIndex));
                file.file->Read(&reserved, sizeof(reserved));

                zCPolygon* polygon = new zCPolygon;
                polyList[numPoly++] = polygon;
                file.file->Read(&polygon->polyPlane, sizeof(polygon->polyPlane));
                file.file->Read(polygonFlags, sizeof(polygonFlags));
                for (int flag = 0; flag < 3; ++flag)
                    polygon->flags[flag] = polygonFlags[flag];
                polygon->AllocVerts(polygonFlags[3]);
                if (materialIndex >= 0)
                    polygon->SetMaterial(materials[materialIndex]);
                if (lightmapIndex >= 0) {
                    polygon->lightmap = lightmaps[lightmapIndex];
                    polygon->lightmap->AddRef();
                }
                for (int j = 0; j < polygon->polyNumVert; ++j) {
                    int vertexIndex;
                    int featureIndex;
                    file.file->Read(&vertexIndex, sizeof(vertexIndex));
                    file.file->Read(&featureIndex, sizeof(featureIndex));
                    polygon->vertex[j] = vertList[vertexIndex];
                    polygon->feature[j] = featList[featureIndex];
                }
            }
            break;
        }
        case 0xB060:
            finished = 1;
            break;
        default:
            break;
        }
        file.file->Seek(chunkEnd);
    }

    for (int i = 0; i < materials.numInArray; ++i)
        materials[i]->Release();
    for (int i = 0; i < lightmaps.numInArray; ++i)
        lightmaps[i]->Release();
    if (!hasBounds)
        CalcBBox3D(0);
    for (int i = 0; i < numPoly; ++i)
        polyList[i]->ResetLightDynToLightStat();
    return 1;
}

int zCMesh::LoadMSH(const zSTRING& fileName, int saveGame)
{
    meshName = fileName;
    zCFileBIN file;
    file.file = zfactory->CreateZFile(zoptions->GetDirString(0x12) + fileName);
    file.externalFile = 0;
    if (file.file->Open(0)) {
        file.BinClose();
        return 0;
    }
    const int result = LoadMSH(file, saveGame);
    file.BinClose();
    return result;
}

unsigned short zCMesh::GetFileVersion()
{
    return (zCTextureFileFormatInternal::classDef->archiveVersion << 8) + 9;
}

void zCMesh::CreateListsFromArrays()
{
    if (!vertList && vertArray) {
        vertList = new zCVertex*[numVert];
        for (int i = 0; i < numVert; ++i)
            vertList[i] = &vertArray[i];
    }
    if (!polyList && polyArray) {
        polyList = new zCPolygon*[numPoly];
        for (int i = 0; i < numPoly; ++i)
            polyList[i] = &polyArray[i];
    }
    if (!featList && featArray) {
        featList = new zCVertFeature*[numFeat];
        for (int i = 0; i < numFeat; ++i)
            featList[i] = &featArray[i];
    }
}

void zCMesh::ClearLists()
{
    if (vertList && vertArray) {
        delete[] vertList;
        vertList = NULL;
    }
    if (polyList && polyArray) {
        delete[] polyList;
        polyList = NULL;
    }
    if (featList && featArray) {
        delete[] featList;
        featList = NULL;
    }
}

void zCMesh::ArraysToLists()
{
    CreateListsFromArrays();
}

int S_FeaturesAreEqual(const zCVertFeature& first, const zCVertFeature& second)
{
    const float normalDot = first.vertNormal.n[0] * second.vertNormal.n[0] +
        first.vertNormal.n[1] * second.vertNormal.n[1] +
        first.vertNormal.n[2] * second.vertNormal.n[2];
    if (normalDot < 0.7f)
        return 0;
    const float deltaU = first.mapping.n[0] - second.mapping.n[0];
    const float deltaV = first.mapping.n[1] - second.mapping.n[1];
    return (deltaU < 0 ? -deltaU : deltaU) < 0.03f &&
        (deltaV < 0 ? -deltaV : deltaV) < 0.03f;
}

void zCMesh::ShareFeatures()
{
    CreateListsFromArrays();
    int capacity = 0;
    for (int i = 0; i < numPoly; ++i)
        capacity += polyList[i]->polyNumVert;
    if (!capacity)
        return;

    zCVertFeature* shared = new zCVertFeature[capacity];
    int sharedCount = 0;
    for (int i = 0; i < numPoly; ++i) {
        zCPolygon* polygon = polyList[i];
        for (int j = 0; j < polygon->polyNumVert; ++j) {
            zCVertFeature* feature = polygon->feature[j];
            int found = 0;
            while (found < sharedCount && !S_FeaturesAreEqual(shared[found], *feature))
                ++found;
            if (found == sharedCount) {
                shared[sharedCount] = *feature;
                ++sharedCount;
            }
            polygon->feature[j] = &shared[found];
        }
    }

    delete[] featList;
    delete[] featArray;
    featList = NULL;
    featArray = shared;
    numFeat = sharedCount;
}

void zCMesh::UnshareFeatures()
{
    CreateListsFromArrays();
    if (!featList && !featArray)
        return;

    for (int i = 0; i < numPoly; ++i) {
        zCPolygon* polygon = polyList[i];
        zCVertFeature* features = zCPolygon::S_AllocVertFeatures(polygon->polyNumVert);
        void* pointers = zCPolygon::S_AllocPointers(polygon->polyNumVert);
        for (int j = 0; j < polygon->polyNumVert; ++j) {
            features[j] = *polygon->feature[j];
            static_cast<zCVertFeature**>(pointers)[j] = &features[j];
        }
        zCPolygon::S_DeletePointers(polygon->polyNumVert, polygon->feature);
        polygon->feature = static_cast<zCVertFeature**>(pointers);
    }

    delete[] featList;
    delete[] featArray;
    featList = NULL;
    featArray = NULL;
}

void zCMesh::FlipPolys()
{
    for (int i = 0; i < numPoly; ++i)
        polyList[i]->Flip();
}

void zCMesh::SortPolysByMaterial()
{
    CreateListsFromArrays();
    for (int i = 1; i < numPoly; ++i) {
        zCPolygon* polygon = polyList[i];
        int j = i;
        while (j > 0 && polyList[j - 1]->material > polygon->material) {
            polyList[j] = polyList[j - 1];
            --j;
        }
        polyList[j] = polygon;
    }
}

void zCMesh::SortPolysByList(zCPolygon** polygons, int count)
{
    CreateListsFromArrays();
    int destination = 0;
    for (int i = 0; i < count; ++i) {
        for (int j = destination; j < numPoly; ++j) {
            if (polyList[j] == polygons[i]) {
                zCPolygon* temporary = polyList[destination];
                polyList[destination] = polyList[j];
                polyList[j] = temporary;
                ++destination;
                break;
            }
        }
    }
}

void zCMesh::DeletePolyAndVerts(zCPolygon* polygon)
{
    CreateListsFromArrays();
    for (int i = 0; i < numPoly; ++i) {
        if (polyList[i] == polygon) {
            DeletePolyAndVerts(i);
            return;
        }
    }
}

void zCMesh::DeletePolyAndVerts(unsigned long index)
{
    CreateListsFromArrays();
    if (index >= static_cast<unsigned long>(numPoly))
        return;

    zCPolygon* polygon = polyList[index];
    for (int vertexIndex = 0; vertexIndex < polygon->polyNumVert; ++vertexIndex) {
        zCVertex* vertex = polygon->vertex[vertexIndex];
        bool used = false;
        for (int i = 0; i < numPoly && !used; ++i) {
            if (polyList[i] != polygon)
                used = polyList[i]->VertPartOfPoly(vertex) != 0;
        }
        if (used)
            continue;
        for (int i = 0; i < numVert; ++i) {
            if (vertList[i] == vertex) {
                if (i != numVert - 1)
                    vertList[i] = vertList[numVert - 1];
                --numVert;
                break;
            }
        }
        delete vertex;
    }

    delete polygon;
    for (unsigned long i = index; i < static_cast<unsigned long>(numPoly - 1); ++i)
        polyList[i] = polyList[i + 1];
    --numPoly;
}

void zCMesh::RemoveDegeneratePolys()
{
    CreateListsFromArrays();
    for (int i = 0; i < numPoly; ++i) {
        if (!polyList[i]->CalcNormal()) {
            DeletePolyAndVerts(i);
            --i;
        }
    }
}

zCCombine::zCCombine()
{
    for (int y = 0; y < 258; ++y) {
        for (int x = 0; x < 258; ++x)
            occupied[y][x] = 0;
    }
}

zCCombine::~zCCombine()
{
    for (int i = 0; i < items.numInArray; ++i)
        delete items.parray[i];
    items.DeleteList();
}

int zCCombine::CombineWith(zCLightMap* lightmap)
{
    int width;
    int height;
    lightmap->tex->GetPixelSize(width, height);
    width += 2;
    height += 2;
    for (int y = 0; y + height <= 258; ++y) {
        for (int x = 0; x + width <= 258; ++x) {
            if (CombineWith(lightmap, x, y))
                return 1;
        }
    }
    return 0;
}

int zCCombine::CombineWith(zCLightMap* lightmap, int x, int y)
{
    if (x < 0 || y < 0)
        return 0;

    int width;
    int height;
    lightmap->tex->GetPixelSize(width, height);
    width += 2;
    height += 2;
    if (x + width > 258 || y + height > 258)
        return 0;

    for (int row = 0; row < height; ++row) {
        for (int column = 0; column < width; ++column) {
            if (occupied[y + row][x + column])
                return 0;
        }
    }
    for (int row = 0; row < height; ++row) {
        for (int column = 0; column < width; ++column)
            occupied[y + row][x + column] = 1;
    }

    zCCItem* item = new zCCItem;
    item->lightmap = lightmap;
    item->x = x + 1;
    item->y = y + 1;
    items.InsertEnd(item);
    return 1;
}

void zCMesh::CombineLightmaps()
{
    CreateListsFromArrays();
    zCArraySort<zCLightMap*> lightmaps;
    zCArraySort<zCTexture*> textures;
    lightmaps.Compare = zArraySortDefaultCompare;
    textures.Compare = zArraySortDefaultCompare;

    for (int i = 0; i < numPoly; ++i) {
        zCLightMap* lightmap = polyList[i]->lightmap;
        if (!lightmap)
            continue;
        if (lightmaps.Search(lightmap) < 0)
            lightmaps.InsertSort(lightmap);
        if (textures.Search(lightmap->tex) < 0)
            textures.InsertSort(lightmap->tex);
    }

    if (!lightmaps.numInArray || lightmaps.numInArray <= textures.numInArray)
        return;

    zCCombine combine;
    for (int i = 0; i < lightmaps.numInArray; ++i)
        combine.CombineWith(lightmaps.array[i]);
    hasLightmaps = 1;
}
