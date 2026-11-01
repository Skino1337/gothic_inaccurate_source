#include "include/zModel.h"
#include "include/zSky.h"
#include "include/zSky_Outdoor.h"
#include "include/zVertexBuffer.h"

class zCOption {
public:

    int ChangeDir(int directory);
    int ReadBool(const zSTRING& section, const zSTRING& entry, int defaultValue);
};

extern zCOption* zoptions;
extern zCVertexBufferManager zvertexBufferMan;

static int worldState = -1;
static int worldWayNetState = -1;
static int worldContainerState = -1;
static float worldDefaultFarPlane = 25.0f;
static float worldDefaultFadeDistance = worldDefaultFarPlane * 0.5f;
static zVEC3 worldLuminanceWeights(0.299f, 0.587f, 0.114f);
// EXE: 0x009A442C
static zCArray<zCPatchMap*> worldContainerList;
static float worldFadeFarDistance = 250000.0f * 0.0166666675f;
static float worldFadeNearDistance = worldFadeFarDistance * 0.0166666675f;
static float worldFarPlaneDistance = 250000.0f * 24.0f;
static zCClassDef zCWorldClassDef("zCWorld", "zCSession", zCWorld::_CreateNewInstance,
    0, 0x6258, 1, "");
zCClassDef* zCWorld::classDef = &zCWorldClassDef;
zSTRING s_worldFileName;
static zCClassDef zCWayNetClassDef("zCWayNet", "zCObject", zCWayNet::_CreateNewInstance,
    0, 0x4C, 0, "");
zCClassDef* zCWayNet::classDef = &zCWayNetClassDef;
int zCWorld::s_worldLoadMode;
int zCWorld::s_worldSaveMode;
int zCWorld::s_worldMergeMode;
int zCWorld::s_bFadeOutFarVerts = 1;
// EXE: 0x009A4424
int zCWorld::s_bAlternateRenderOrder;
// EXE: 0x008A8AC4
int zCWorld::s_bEnvMappingEnabled = 1;
// EXE: 0x008A8AC0
int zCWorld::s_bAmbientVobsEnabled = 1;

static zCArray<zCVob*> unarcVobList;
static int arcVobCounter;
static zCTree<zCVob>* archiveRoot;
static zCVob* mergeRootVob;
static zCTree<zCVob>* mergeParentNode;
static int archiveWayNet;

zCWorld::zCWorld()
    : ownerSession(0), csPlayer(0), compiled(0), compiledEditorMode(0), traceRayIgnoreVobFlag(0),
      m_bIsInventoryWorld(0), worldRenderMode(0), wayNet(new zCWayNet(this)), masterFrameCtr(0),
      vobFarClipZ(5000.0f), vobFarClipZScalability(1.0f), renderingFirstTime(1), showWaynet(0),
      showTraceRayLines(0), progressBar(0), unarchiveFileLen(0), unarchiveStartPosVobtree(0),
      numVobsInWorld(0), skyControlerIndoor(new zCSkyControler_Indoor),
      skyControlerOutdoor(new zCSkyControler_Outdoor), activeSkyControler(0), addZonesToWorld(0),
      showZonesDebugInfo(0), cbspTree(0)
{
    globalVobTree.parent = 0;
    globalVobTree.firstChild = 0;
    globalVobTree.prev = 0;
    globalVobTree.next = 0;
    globalVobTree.data = 0;
    bspTree.bspTreeMode = 0;
    bspTree.worldRenderMode = 0;
    bspTree.bspRoot = 0;
    bspTree.mesh = 0;
    activeSkyControler = skyControlerIndoor;
    activeVobList.AllocAbs(128);
}

// EXE: 0x0061F9B0
zCObject* zCWorld::_CreateNewInstance()
{
    zCWorld* world = new zCWorld;
    zCClassDef::ObjectCreated(world, classDef);
    return world;
}

zCClassDef* zCWorld::_GetClassDef() const
{
    return classDef;
}

void zCWorld::DisposeWorld()
{
    DisposeVobs(0);
    DisposeStaticWorld();
}

zCWorld::~zCWorld()
{
    DisposeWorld();
    if (wayNet)
        wayNet->Release();
    if (csPlayer)
        csPlayer->Release();
    if (skyControlerIndoor)
        skyControlerIndoor->Release();
    if (skyControlerOutdoor)
        skyControlerOutdoor->Release();
    delete cbspTree;
    while (globalVobTree.firstChild) {
        zCTree<zCVob>* child = globalVobTree.firstChild;
        globalVobTree.firstChild = child->next;
        delete child;
    }
}

void* zCWorld::GetPlayerGroup()
{
    return 0;
}

zCSkyControler* zCWorld::GetActiveSkyControler()
{
    activeSkyControler = bspTree.bspTreeMode ? skyControlerOutdoor : skyControlerIndoor;
    return activeSkyControler;
}

void zCWorld::SetSkyControlerIndoor(zCSkyControler* sky)
{
    if (skyControlerIndoor != sky) {
        if (skyControlerIndoor)
            skyControlerIndoor->Release();
        skyControlerIndoor = sky;
    }
}

void zCWorld::SetSkyControlerOutdoor(zCSkyControler* sky)
{
    if (skyControlerOutdoor != sky) {
        if (skyControlerOutdoor)
            skyControlerOutdoor->Release();
        skyControlerOutdoor = sky;
    }
}

int zCWorld::ActiveZoneListCompare(const void* first, const void* second)
{
    zCZone* firstZone = *static_cast<zCZone* const*>(first);
    zCZone* secondZone = *static_cast<zCZone* const*>(second);
    if (firstZone->GetPriority() < secondZone->GetPriority())
        return -1;
    return firstZone->GetPriority() > secondZone->GetPriority();
}


void zCWorld::RenderWaynet(zCCamera* camera)
{
    if (showWaynet && wayNet)
        wayNet->Draw(camera);
}

void zCWorld::AddZone(zCZone* zone)
{
    if (!zone || zoneGlobalList.Search(zone) != -1)
        return;
    if (zone->_GetClassDef() == zone->GetDefaultZoneClass())
        zoneGlobalList.Insert(zone, 0);
    else {
        zoneGlobalList.InsertEnd(zone);
        zoneBoxSorter.Insert(zone);
    }
}

void zCWorld::RemoveZone(zCZone* zone)
{
    if (!zone)
        return;
    const int index = zoneGlobalList.Search(zone);
    if (index != -1) {
        --zoneGlobalList.numInArray;
        zoneGlobalList.parray[index] = zoneGlobalList.parray[zoneGlobalList.numInArray];
    }
    if (zone->_GetClassDef() != zone->GetDefaultZoneClass())
        zoneBoxSorter.Remove(zone);
    for (int i = 0; i < zoneLastClassList.numInArray; ++i) {
        if (zoneLastClassList.array[i] == zone) {
            --zoneLastClassList.numInArray;
            zoneLastClassList.array[i] = zoneLastClassList.array[zoneLastClassList.numInArray];
            break;
        }
    }
    for (int i = 0; i < zoneActiveList.numInArray; ++i) {
        if (zoneActiveList.array[i] == zone) {
            --zoneActiveList.numInArray;
            zoneActiveList.array[i] = zoneActiveList.array[zoneActiveList.numInArray];
            break;
        }
    }
}

void zCWorld::RemoveAllZones()
{
    zoneBoxSorter.Clear();
    for (int i = 0; i < zoneGlobalList.numInArray; ++i)
        zoneGlobalList.parray[i]->ThisVobRemovedFromWorld(this);
    zoneActiveList.numInArray = 0;
    zoneLastClassList.numInArray = 0;
    zoneGlobalList.numInArray = 0;
}

void zCWorld::UpdateZone(zCZone* zone)
{
    zoneBoxSorter.Update(zone);
}

void zCWorld::ProcessZones()
{
    GetActiveSkyControler();
    activeSkyControler->SetOverrideColorFlag(0);
    activeSkyControler->SetGlobalSkyScale(1.0f);
    zoneActiveList.compare_func = ActiveZoneListCompare;
    for (int i = 0; i < zoneActiveList.numInArray; ++i) {
        for (int j = i + 1; j < zoneActiveList.numInArray; ++j) {
            if (ActiveZoneListCompare(&zoneActiveList.array[i], &zoneActiveList.array[j]) > 0) {
                zCZone* swap = zoneActiveList.array[i];
                zoneActiveList.array[i] = zoneActiveList.array[j];
                zoneActiveList.array[j] = swap;
            }
        }
    }
    for (int i = 0; i < zoneGlobalList.numInArray; ++i)
        zoneGlobalList.parray[i]->ProcessZone();
}


int zCWorld::showTextureStats;
int zCWorld::s_bWaveAnisEnabled = 1;

void zCWorld::ShowZonesDebugInfo()
{
    if (!showZonesDebugInfo)
        return;
    screen->Print(0, 500, zSTRING("** Active Zones **"));
    for (int i = 0; i < zoneActiveList.numInArray; ++i)
        screen->Print(0, 500 + (i + 1) * 16, zoneActiveList.array[i]->GetObjectName());
}

void zCWorld::ShowTextureStats()
{
    if (!showTextureStats)
        return;
    zSTRING statistics;
    zrenderer->GetStatistics(statistics);
    zSTRING memory;
    memory.Sprintf("mem: %dk", zrenderer->GetTotalTextureMem() / 1024);
    screen->Print(0, 500, memory);
    screen->Print(0, 516, statistics);
}

void zCWorld::Render(zCCamera* camera)
{
    if (!camera || !camera->connectedVob || camera->connectedVob->homeWorld != this)
        return;
    const int gameWorld = m_strlevelName.Length() > 0;
    zCCacheBase::S_PurgeCaches();
    if (gameWorld && s_bWaveAnisEnabled)
        zCFFT::S_SetFrameCtr(masterFrameCtr);
    camera->PreRenderProcessing();
    camera->UpdateViewport();
    camera->Activate();
    GetActiveSkyControler();
    zCSkyControler::s_activeSkyControler = activeSkyControler;
    if (renderingFirstTime) {
        zresMan->cacheInImmediatelyMsec = 1500.0f;
        renderingFirstTime = 0;
    }
    if (gameWorld) {
        zsound->SetListener(camera->connectedVob);
        ProcessZones();
        zsound->DoSoundUpdate();
    }
    zCPolygon::PrepareRendering();
    bspTree.vobFarClipZ = vobFarClipZScalability * vobFarClipZ;
    bspTree.Render();
    camera->PostRenderProcessing();
    zrenderer->FlushPolys();
    zCPolygon::S_ResetMorphedVerts();
    if (gameWorld) {
        zlineCache.Flush(camera->targetView);
        RenderWaynet(camera);
        ShowZonesDebugInfo();
        ShowTextureStats();
    }
    camera->Activate();
    walkList = activeVobList;
    for (int i = 0; i < walkList.numInArray; ++i) {
        if (walkList.parray[i])
            walkList.parray[i]->DoFrameActivity();
    }
    walkList.numInArray = 0;
    zCEventManager::DoFrameActivity();
    zCResourceManager::DoFrameActivity(zresMan);
    if (csPlayer)
        csPlayer->Process();
    for (int i = 0; i < perFrameCallbackList.numInArray; ++i)
        perFrameCallbackList.parray[i]->DoWorldPerFrameCallback(this, camera);
    ++masterFrameCtr;
    zCVertex::ResetVertexTransforms();
    ++zCTexAniCtrl::masterFrameCtr;
}

static void ResetTraceRayReport(zTTraceRayReport& report)
{
    report.foundHit = 0;
    report.foundVob = 0;
    report.foundPoly = 0;
    report.foundVertex = 0;
    for (int i = 0; i < 3; ++i) {
        report.foundIntersection.n[i] = 0.0f;
        report.foundPolyNormal.n[i] = 0.0f;
        report.foundHitPosition.n[i] = 0.0f;
    }
}

int zCWorld::TraceRayFirstHit(const zVEC3& origin, const zVEC3& direction,
    const zCArray<zCVob*>* ignoreVobs, int flags)
{
    traceRayVobList.numInArray = 0;
    ResetTraceRayReport(traceRayReport);
    bspTree.TraceRay(origin, direction, traceRayVobList, flags, traceRayReport);
    if (traceRayReport.foundHit)
        return 1;
    for (int i = 0; i < traceRayVobList.numInArray; ++i) {
        zCVob* vob = traceRayVobList.parray[i];
        if (ignoreVobs && ignoreVobs->Search(vob) != -1)
            continue;
        if (vob->TraceRay(origin, direction, flags, traceRayReport))
            return 1;
    }
    return 0;
}

int zCWorld::TraceRayNearestHit(const zVEC3& origin, const zVEC3& direction,
    const zCArray<zCVob*>* ignoreVobs, int flags)
{
    traceRayVobList.numInArray = 0;
    ResetTraceRayReport(traceRayReport);
    bspTree.TraceRay(origin, direction, traceRayVobList, flags, traceRayReport);
    float bestDistance = 3.40282347e+38f;
    if (traceRayReport.foundHit) {
        bestDistance = 0.0f;
        for (int i = 0; i < 3; ++i) {
            const float delta = traceRayReport.foundIntersection.n[i] - origin.n[i];
            bestDistance += delta * delta;
        }
    }
    zTTraceRayReport bestReport = traceRayReport;
    for (int i = 0; i < traceRayVobList.numInArray; ++i) {
        zCVob* vob = traceRayVobList.parray[i];
        if (ignoreVobs && ignoreVobs->Search(vob) != -1)
            continue;
        zTTraceRayReport candidate;
        ResetTraceRayReport(candidate);
        if (!vob->TraceRay(origin, direction, flags, candidate))
            continue;
        float distance = 0.0f;
        for (int axis = 0; axis < 3; ++axis) {
            const float delta = candidate.foundIntersection.n[axis] - origin.n[axis];
            distance += delta * delta;
        }
        if (distance < bestDistance) {
            bestDistance = distance;
            bestReport = candidate;
        }
    }
    traceRayReport = bestReport;
    if (traceRayReport.foundHit && traceRayReport.foundPoly && !traceRayReport.foundVob)
        traceRayReport.foundPolyNormal = traceRayReport.foundPoly->polyPlane.normal;
    return traceRayReport.foundHit;
}


int zCWorld::TraceRayFirstHit(const zVEC3& origin, const zVEC3& direction, zCVob* ignoreVob,
    int flags)
{
    if (!ignoreVob)
        return TraceRayFirstHit(origin, direction, static_cast<const zCArray<zCVob*>*>(0), flags);
    traceRayTempIgnoreVobList.InsertEnd(ignoreVob);
    const int result = TraceRayFirstHit(origin, direction, &traceRayTempIgnoreVobList, flags);
    traceRayTempIgnoreVobList.numInArray = 0;
    return result;
}

int zCWorld::TraceRayNearestHit(const zVEC3& origin, const zVEC3& direction, zCVob* ignoreVob,
    int flags)
{
    if (!ignoreVob)
        return TraceRayNearestHit(origin, direction, static_cast<const zCArray<zCVob*>*>(0), flags);
    traceRayTempIgnoreVobList.InsertEnd(ignoreVob);
    const int result = TraceRayNearestHit(origin, direction, &traceRayTempIgnoreVobList, flags);
    traceRayTempIgnoreVobList.numInArray = 0;
    return result;
}

int zCWorld::TraceRayFirstHitCache(const zVEC3& origin, const zVEC3& direction,
    const zCArray<zCVob*>* ignoreVobs, int flags, zCRayCache* cache)
{
    zCBspTree::SetRayCache(cache);
    return TraceRayFirstHit(origin, direction, ignoreVobs, flags);
}

int zCWorld::TraceRayFirstHitCache(const zVEC3& origin, const zVEC3& direction, zCVob* ignoreVob,
    int flags, zCRayCache* cache)
{
    zCBspTree::SetRayCache(cache);
    return TraceRayFirstHit(origin, direction, ignoreVob, flags);
}

int zCWorld::TraceRayNearestHitCache(const zVEC3& origin, const zVEC3& direction,
    const zCArray<zCVob*>* ignoreVobs, int flags, zCRayCache* cache)
{
    zCBspTree::SetRayCache(cache);
    return TraceRayNearestHit(origin, direction, ignoreVobs, flags);
}

int zCWorld::TraceRayNearestHitCache(const zVEC3& origin, const zVEC3& direction, zCVob* ignoreVob,
    int flags, zCRayCache* cache)
{
    zCBspTree::SetRayCache(cache);
    return TraceRayNearestHit(origin, direction, ignoreVob, flags);
}


int zCWorld::PickScene(zCCamera& camera, int x, int y, float distance)
{
    camera.Activate();
    zVEC3 origin;
    zVEC3 direction;
    for (int i = 0; i < 3; ++i) {
        origin.n[i] = camera.camMatrixInv.m[i][3];
        direction.n[i] = camera.camMatrixInv.m[i][2] * 600000.0f +
            camera.camMatrixInv.m[i][0] * (x - camera.vpData.xcenter) *
                camera.viewDistanceXInv * 600000.0f +
            camera.camMatrixInv.m[i][1] * (camera.vpData.ycenter - y) *
                camera.viewDistanceYInv * 600000.0f;
    }
    float length = 0.0f;
    for (int i = 0; i < 3; ++i)
        length += direction.n[i] * direction.n[i];
    if (length > 0.0f) {
        length = std::sqrt(length);
        for (int i = 0; i < 3; ++i)
            direction.n[i] = direction.n[i] / length * distance;
    }
    ResetTraceRayReport(traceRayReport);
    traceRayIgnoreVobFlag = 1;
    const int result = TraceRayNearestHit(origin, direction,
        static_cast<const zCArray<zCVob*>*>(0), zCVob::s_showHelperVisuals ? 0x2220 : 0x220);
    traceRayIgnoreVobFlag = 0;
    if (result && traceRayReport.foundPoly && !traceRayReport.foundVob) {
        float bestDistance = 3.40282347e+38f;
        for (int i = 0; i < traceRayReport.foundPoly->polyNumVert; ++i) {
            zCVertex* vertex = traceRayReport.foundPoly->vertex[i];
            float vertexDistance = 0.0f;
            for (int axis = 0; axis < 3; ++axis) {
                const float delta = vertex->position.n[axis] - traceRayReport.foundIntersection.n[axis];
                vertexDistance += delta * delta;
            }
            if (vertexDistance < bestDistance) {
                bestDistance = vertexDistance;
                traceRayReport.foundVertex = vertex;
            }
        }
    }
    return result;
}

int zCWorld::TraverseBsp(zCCBspTree* tree, zCTree<zCVob>* node, int mode)
{
    if (!node)
        return 0;
    zCVob* vob = node->data;
    if (vob && dynamic_cast<zCVobLevelCompo*>(vob)) {
        zCMesh* mesh = dynamic_cast<zCMesh*>(vob->visual);
        if (mesh)
            tree->AddMesh(mesh, vob->trafoObjToWorld);
        vob->showVisual = 0;
    }
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        TraverseBsp(tree, child, mode);
    return 0;
}

int zCWorld::TraverseBspAddVobsToMesh(zCCBspTree* tree, zCTree<zCVob>* node)
{
    if (!node)
        return 0;
    int added = 0;
    zCVob* vob = node->data;
    if (vob && vob->_GetClassDef() == &zCVob::classDef && (vob->castDynShadow & 0x10)) {
        zCProgMeshProto* proto = dynamic_cast<zCProgMeshProto*>(vob->visual);
        if (proto && !vob->type && !vob->m_AniMode) {
            vob->CalcGroundPoly();
            if (!vob->groundPoly || !(vob->groundPoly->flags[0] & 8)) {
                zCMesh* mesh = proto->GetMesh();
                if (mesh) {
                    if (mesh->numPoly < 170) {
                        tree->AddMesh(mesh, vob->trafoObjToWorld);
                        added = 1;
                    }
                    mesh->DeleteAll();
                    mesh->Release();
                }
            }
        }
    }
    zCTree<zCVob>* child = node->firstChild;
    while (child) {
        if (TraverseBspAddVobsToMesh(tree, child) == 1)
            child = node->firstChild;
        else
            child = child->next;
    }
    if (added) {
        RemoveVob(vob);
        return 1;
    }
    return 0;
}

void zCWorld::SaveBspTreeMesh3DS(const zSTRING& fileName)
{
    zoptions->ChangeDir(zTOptionPaths_DIR_MESHES);
    zCFile3DS::Save3DS(fileName, bspTree.mesh);
}

void zCWorld::CompileWorld(int bspMode, float, int, int, zCArray<zCPolygon*>*)
{
    if (compiled)
        return;
    cbspTree = new zCCBspTree;
    cbspTree->SetBspTreeMode(bspMode);
    TraverseBsp(cbspTree, &globalVobTree, 0);
    cbspTree->Build();
    compiled = 1;
    compiledEditorMode = 1;
}


void zCWorld::DisposeStaticWorld()
{
    if (bspTree.numPolys) {
        zCSkyControler_Outdoor* outdoor = dynamic_cast<zCSkyControler_Outdoor*>(
            zCSkyControler::s_activeSkyControler);
        if (outdoor)
            outdoor->m_bLevelChanged = 1;
    }
    zCPolygon::S_DeleteMorphedVerts();
    bspTree.DisposeTree();
    compiled = 0;
    compiledEditorMode = 0;
    renderingFirstTime = 1;
    UpdateVobTreeBspDependencies(&globalVobTree);
}

int zCWorld::DisposeVobs(zCTree<zCVob>* node)
{
    zCRenderLightContainer::S_ReleaseVobLightRefs();
    if (!node) {
        if (wayNet)
            wayNet->Release();
        wayNet = new zCWayNet(this);
        if (csPlayer) {
            csPlayer->ResetCutscenePlayer();
            csPlayer->Release();
            csPlayer = 0;
        }
        if (ownerSession)
            csPlayer = ownerSession->GetCutsceneManager()->CreateCutscenePlayer(this);
        RemoveAllZones();
        renderingFirstTime = 1;
        node = &globalVobTree;
    }
    zCTree<zCVob>* child = node->firstChild;
    while (child) {
        if (DisposeVobs(child) == 1)
            child = node->firstChild;
        else
            child = child->next;
    }
    if (node->data) {
        RemoveVob(node->data);
        return 1;
    }
    return 0;
}

int zCWorld::DisposeVobsDbg(zCTree<zCVob>* node)
{
    zCRenderLightContainer::S_ReleaseVobLightRefs();
    if (!node) {
        if (wayNet)
            wayNet->Release();
        wayNet = new zCWayNet(this);
        if (csPlayer) {
            csPlayer->ResetCutscenePlayer();
            csPlayer->Release();
            csPlayer = 0;
        }
        if (ownerSession)
            csPlayer = ownerSession->GetCutsceneManager()->CreateCutscenePlayer(this);
        RemoveAllZones();
        renderingFirstTime = 1;
        node = &globalVobTree;
    }
    zCTree<zCVob>* child = node->firstChild;
    while (child) {
        if (DisposeVobsDbg(child) == 1)
            child = node->firstChild;
        else
            child = child->next;
    }
    if (node->data) {
        RemoveVob(node->data);
        return 1;
    }
    return 0;
}

zCVob* zCWorld::SearchVob(zCVob* vob, zCTree<zCVob>* node)
{
    if (!node)
        node = &globalVobTree;
    if (node->data == vob)
        return vob;
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next) {
        zCVob* result = SearchVob(vob, child);
        if (result)
            return result;
    }
    return 0;
}

zCVob* zCWorld::SearchVobByID(unsigned long vobID, zCTree<zCVob>* node)
{
    if (!node) {
        if (!vobID)
            return 0;
        node = &globalVobTree;
    }
    if (node->data && vobID == 1)
        return node->data;
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next) {
        zCVob* result = SearchVobByID(vobID, child);
        if (result)
            return result;
    }
    return 0;
}

zCVob* zCWorld::SearchVobByName(const zSTRING& name)
{
    if (!name.Length())
        return 0;
    const unsigned long hash = zCChecksum::GetBufferCRC32(name.ToChar(), name.Length(), 0) & 0x7ff;
    zCArray<zCVob*>& bucket = vobHashTable[hash];
    for (int i = 0; i < bucket.numInArray; ++i) {
        if (bucket.parray[i]->GetObjectName() == name)
            return bucket.parray[i];
    }
    return 0;
}

void zCWorld::SearchVobListByName(const zSTRING& name, zCArray<zCVob*>& vobs)
{
    if (!name.Length())
        return;
    const unsigned long hash = zCChecksum::GetBufferCRC32(name.ToChar(), name.Length(), 0) & 0x7ff;
    zCArray<zCVob*>& bucket = vobHashTable[hash];
    for (int i = 0; i < bucket.numInArray; ++i) {
        if (bucket.parray[i]->GetObjectName() == name)
            vobs.InsertEnd(bucket.parray[i]);
    }
}

void zCWorld::SearchVobListByClass(zCClassDef* classDef, zCArray<zCVob*>& vobs,
    zCTree<zCVob>* node)
{
    if (!node)
        node = &globalVobTree;
    if (node->data && node->data->_GetClassDef() == classDef)
        vobs.InsertEnd(node->data);
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        SearchVobListByClass(classDef, vobs, child);
}

void zCWorld::SearchVobListByBaseClass(zCClassDef* classDef, zCArray<zCVob*>& vobs,
    zCTree<zCVob>* node)
{
    if (!node)
        node = &globalVobTree;
    if (node->data) {
        for (zCClassDef* current = node->data->_GetClassDef(); current; current = current->baseClassDef) {
            if (current == classDef) {
                vobs.InsertEnd(node->data);
                break;
            }
        }
    }
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        SearchVobListByBaseClass(classDef, vobs, child);
}

void zCWorld::SearchZoneListByClass(zCClassDef* classDef, zCArray<zCZone*>& zones)
{
    for (int i = 0; i < zoneGlobalList.numInArray; ++i) {
        if (zoneGlobalList.parray[i]->_GetClassDef() == classDef)
            zones.InsertEnd(zoneGlobalList.parray[i]);
    }
}

zCZone* zCWorld::SearchZoneDefaultByClass(zCClassDef* classDef)
{
    for (int i = 0; i < zoneGlobalList.numInArray; ++i) {
        zCZone* zone = zoneGlobalList.parray[i];
        if (zone->_GetClassDef() == classDef)
            return zone;
        if (zone->_GetClassDef() != zone->GetDefaultZoneClass())
            break;
    }
    return 0;
}

int zCWorld::ShouldAddThisVobToBsp(zCVob* vob) const
{
    if (vob) {
        for (zCClassDef* classDef = vob->_GetClassDef(); classDef; classDef = classDef->baseClassDef) {
            if (classDef == &zCZone::classDef)
                return addZonesToWorld != 0;
        }
    }
    return 1;
}

zCTree<zCVob>* zCWorld::AddVobAsChild(zCVob* vob, zCTree<zCVob>* parent)
{
    if (!vob || !parent || vob->homeWorld)
        return 0;
    vob->homeWorld = this;
    if (dynamic_cast<zCZone*>(vob))
        AddZone(static_cast<zCZone*>(vob));
    if (!vob->globalVobTreeNode)
        vob->globalVobTreeNode = parent->AddChild(vob);
    else {
        if (!vob->globalVobTreeNode->parent) {
            vob->globalVobTreeNode->parent = parent;
            vob->globalVobTreeNode->prev = 0;
            vob->globalVobTreeNode->next = parent->firstChild;
            parent->firstChild = vob->globalVobTreeNode;
            if (vob->globalVobTreeNode->next)
                vob->globalVobTreeNode->next->prev = vob->globalVobTreeNode;
        }
        for (zCTree<zCVob>* child = vob->globalVobTreeNode->firstChild; child; child = child->next)
            AddVobAsChild(child->data, vob->globalVobTreeNode);
    }
    vob->AddVobToWorld_CorrectParentDependencies();
    const unsigned long collisionFlags = vob->showVisual;
    vob->SetCollDetStat(0);
    vob->SetCollDetDyn(0);
    vob->BeginMovement();
    vob->TouchMovement();
    vob->EndMovement(1);
    vob->SetCollDetDyn(collisionFlags >> 7);
    vob->SetCollDetStat((collisionFlags >> 6) & 1);
    vob->AddRef();
    if (ShouldAddThisVobToBsp(vob))
        bspTree.AddVob(vob);
    if ((vob->sleepingMode & 3) && !activeVobList.IsInList(vob))
        activeVobList.InsertEnd(vob);
    if (vob->callback_ai)
        vob->callback_ai->HostVobAddedToWorld(vob, this);
    if (vob->visual)
        vob->visual->HostVobAddedToWorld(vob, this);
    VobAddedToWorld(vob);
    return vob->globalVobTreeNode;
}

zCTree<zCVob>* zCWorld::AddVobAsChild(zCVob* vob, zCVob* parent)
{
    return parent ? AddVobAsChild(vob, parent->globalVobTreeNode) : 0;
}

zCTree<zCVob>* zCWorld::AddVob(zCVob* vob)
{
    return AddVobAsChild(vob, &globalVobTree);
}

void zCWorld::VobAddedToWorld(zCVob* vob)
{
    InsertVobHashTable(vob);
    vob->ThisVobAddedToWorld(this);
    ++numVobsInWorld;
}

void zCWorld::VobRemovedFromWorld(zCVob* vob)
{
    vob->ThisVobRemovedFromWorld(this);
    RemoveVobFromLists(vob);
    --numVobsInWorld;
}

void zCWorld::RemoveVob(zCVob* vob)
{
    if (!vob || vob->homeWorld != this)
        return;
    VobRemovedFromWorld(vob);
    vob->RemoveWorldDependencies(1);
}

void zCWorld::RemoveVob(zCTree<zCVob>* node)
{
    if (node)
        RemoveVob(node->data);
}

void zCWorld::RemoveVobSubtree_r(zCTree<zCVob>* node, int removeChildren)
{
    if (!node)
        return;
    zCVob* vob = node->data;
    if (vob) {
        VobRemovedFromWorld(vob);
        vob->AddRef();
        vob->RemoveWorldDependencies(removeChildren && !node->firstChild);
    }
    zCTree<zCVob>* child = node->firstChild;
    while (child) {
        zCTree<zCVob>* next = child->next;
        RemoveVobSubtree_r(child, 0);
        child = next;
    }
    if (vob)
        vob->Release();
}

void zCWorld::RemoveVobSubtree(zCVob* vob)
{
    if (!vob)
        return;
    MoveVobSubtreeTo(vob, static_cast<zCTree<zCVob>*>(0));
    RemoveVobSubtree_r(vob->globalVobTreeNode, 1);
}

void zCWorld::RemoveVobSubtree(zCTree<zCVob>* node)
{
    if (node)
        RemoveVobSubtree(node->data);
}

void zCWorld::MoveVobSubtreeTo(zCVob* vob, zCTree<zCVob>* parent)
{
    if (!vob || !vob->globalVobTreeNode)
        return;
    zCTree<zCVob>* node = vob->globalVobTreeNode;
    if (node->parent && node->parent->firstChild == node)
        node->parent->firstChild = node->next;
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
    node->next = 0;
    node->prev = 0;
    node->parent = parent;
    if (parent) {
        node->next = parent->firstChild;
        parent->firstChild = node;
        if (node->next)
            node->next->prev = node;
    }
    const int wasMoving = (vob->isInMovementMode & 3) != 0;
    if (wasMoving)
        vob->BeginMovement();
    vob->TouchMovement();
    vob->Move(0.0f, 0.0f, 0.0f);
    if (wasMoving)
        vob->EndMovement(1);
}

void zCWorld::MoveVobSubtreeTo(zCVob* vob, zCVob* parent)
{
    if (parent && parent->globalVobTreeNode)
        MoveVobSubtreeTo(vob, parent->globalVobTreeNode);
}

void zCWorld::MoveVobSubtreeToWorldSpace(zCVob* vob)
{
    MoveVobSubtreeTo(vob, &globalVobTree);
}

void zCWorld::RemoveVobFromLists(zCVob* vob)
{
    for (int i = 0; i < bspTree.renderVobList.numInArray; ++i) {
        if (bspTree.renderVobList.parray[i] == vob) {
            --bspTree.renderVobList.numInArray;
            if (i != bspTree.renderVobList.numInArray)
                bspTree.renderVobList.parray[i] =
                    bspTree.renderVobList.parray[bspTree.renderVobList.numInArray];
            break;
        }
    }
    for (int i = 0; i < walkList.numInArray; ++i) {
        if (walkList.parray[i] == vob) {
            walkList.parray[i] = 0;
            break;
        }
    }
    if (vob && dynamic_cast<zCZone*>(vob))
        RemoveZone(static_cast<zCZone*>(vob));
    RemoveVobHashTable(vob);
}

void zCWorld::UpdateVobTreeBspDependencies(zCTree<zCVob>* node)
{
    if (!node)
        node = &globalVobTree;
    if (node->data) {
        node->data->groundPoly = 0;
        node->data->CalcGroundPoly();
        node->data->vobLeafList.numInArray = 0;
        node->data->RemoveVobFromBspTree();
        bspTree.AddVob(node->data);
    }
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        UpdateVobTreeBspDependencies(child);
}

void zCWorld::MoveVobs()
{
    walkList.numInArray = 0;
    if (walkList.numAlloc < activeVobList.numInArray)
        walkList.AllocDelta(activeVobList.numInArray - walkList.numAlloc);
    walkList.numInArray = activeVobList.numInArray;
    for (int i = 0; i < activeVobList.numInArray; ++i)
        walkList.parray[i] = activeVobList.parray[i];
    for (int i = 0; i < walkList.numInArray; ++i) {
        if (walkList.parray[i])
            walkList.parray[i]->DoFrameActivity();
    }
    walkList.numInArray = 0;
}

void zCWorld::AdvanceClock(float seconds)
{
    if (seconds < 0.0f)
        ztimer.ResetTimer();
    else
        ztimer.SetFrameTime(seconds * 1000.0f);
}

void zCWorld::PrintStatus()
{
    zerr->Message(zSTRING("D: *** World-Status ************************************"));
    zerr->Message(zSTRING("D: Engine, Date: Oct 22 2003"));
    zerr->Message(zSTRING("D: World:"));
    zSTRING line;
    line.Sprintf("D: - Num Vobs        : %d", numVobsInWorld);
    zerr->Message(line);
    line.Sprintf("D: - Num activeVobs  : %d", activeVobList.numInArray);
    zerr->Message(line);
    line.Sprintf("D: - vobFarClipZ     : %f", vobFarClipZ * vobFarClipZScalability);
    zerr->Message(line);
    line.Sprintf("D: - perFrameCallback: %d", perFrameCallbackList.numInArray);
    zerr->Message(line);
    if (!compiled)
        zerr->Message(zSTRING("D: uncompiled world"));
    else {
        zerr->Message(zSTRING("D: BSP: "));
        zerr->Message(zSTRING(bspTree.bspTreeMode ? "D: - Mode            : outdoor" :
            "D: - Mode            : indoor"));
        if (bspTree.mesh) {
            line.Sprintf("D: - Num meshPolys   : %d", bspTree.mesh->numPoly);
            zerr->Message(line);
        }
        line.Sprintf("D: - Num treePolys   : %d", bspTree.numPolys);
        zerr->Message(line);
    }
    zerr->Message(zSTRING("D: *****************************************************"));
}

void zCWorld::PrintGlobalVobTree(zCTree<zCVob>* node, int indentation)
{
    if (!node) {
        node = &globalVobTree;
        zerr->Message(zSTRING("D: ****** ZoneList ********"));
        for (int i = 0; i < zoneGlobalList.numInArray; ++i) {
            zSTRING line("D: ");
            line.Append(zoneGlobalList.parray[i]->GetVobInfo());
            zerr->Message(line);
        }
        zerr->Message(zSTRING("D: ****** VobTree ********"));
    }
    if (node->data) {
        zSTRING line("D: ");
        for (int i = 0; i < indentation * 2; ++i)
            line.Append(zSTRING(" "));
        line.Append(node->data->GetVobInfo());
        zerr->Message(line);
    }
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        PrintGlobalVobTree(child, indentation + 1);
}

void zCWorld::PrintActiveVobs()
{
    zerr->Message(zSTRING("D: *** World: ActiveVobList ************************************"));
    for (int i = 0; i < activeVobList.numInArray; ++i) {
        zSTRING line;
        line.Sprintf("D: %d: ", i);
        line.Append(activeVobList.parray[i]->GetVobInfo());
        zerr->Message(line);
    }
}

void zCWorld::SaveBspTree(zFILE& file)
{
    zSTRING message("D: WORLD: Saving BSP: ");
    message.Append(file.GetFilename());
    zerr->Message(message);
    zCFileBIN binaryFile;
    binaryFile.BinSetFile(&file);
    bspTree.SaveBIN(binaryFile);
}

int zCWorld::LoadBspTree(zFILE& file, int skipMesh)
{
    zCFileBIN binaryFile;
    binaryFile.BinSetFile(&file);
    const int result = bspTree.LoadBIN(binaryFile, skipMesh);
    if (!skipMesh) {
        if (bspTree.mesh) {
            worldRenderMode = bspTree.mesh->hasLightmaps ? 1 : 0;
            bspTree.worldRenderMode = worldRenderMode;
        }
        compiled = result;
    }
    return result;
}

void zCWorld::DebugMarkOccluderPolys()
{
    if (!compiled)
        return;
    zCMaterial* occluderMaterial = new zCMaterial;
    occluderMaterial->objectName = zSTRING("Occluder_Poly_Mat");
    occluderMaterial->color[0] = 50;
    occluderMaterial->color[1] = 50;
    occluderMaterial->color[2] = 255;
    bspTree.mesh->CreateListsFromArrays();
    for (int i = 0; i < bspTree.mesh->numPoly; ++i) {
        zCPolygon* polygon = bspTree.mesh->polyList[i];
        if ((polygon->flags[0] & 4) && !(polygon->flags[0] & 3)) {
            if (polygon->material != occluderMaterial) {
                if (polygon->material)
                    polygon->material->Release();
                occluderMaterial->AddRef();
                polygon->material = occluderMaterial;
            }
        }
    }
}

void zCWorld::UnarcTraverseVobs(zCArchiver& archiver, zCTree<zCVob>* parent)
{
    if (progressBar && archiver.GetFile()) {
        const long position = archiver.GetFile()->Pos();
        const int percent = static_cast<int>((position - unarchiveStartPosVobtree) /
            (unarchiveFileLen + 0.1f) * 100.0f);
        progressBar->SetPercent(percent, zSTRING());
    }
    zCTree<zCVob>* targetNode = parent ? parent : (mergeParentNode ? mergeParentNode : &globalVobTree);
    if (parent) {
        zCVob* vob = static_cast<zCVob*>(archiver.ReadObject(0));
        if (vob) {
            int replacedRoot = 0;
            if (s_worldLoadMode == 5 && s_worldMergeMode == 1 && mergeRootVob) {
                s_worldMergeMode = 0;
                mergeRootVob->SetVisual(vob->GetVisual());
                vob->Release();
                vob = mergeRootVob;
                targetNode = vob->globalVobTreeNode;
                replacedRoot = 1;
            } else {
                if (s_worldLoadMode == 5) {
                    vob->SetVobID(zCVob::GetNextFreeVobID());
                    if (!mergeRootVob)
                        mergeRootVob = vob;
                }
                zCTree<zCVob>* newNode = AddVobAsChild(vob, parent);
                if (newNode)
                    targetNode = newNode;
            }
            unarcVobList.InsertEnd(vob);
            if (!replacedRoot)
                vob->Release();
        }
    }
    const int numChildren = archiver.ReadInt();
    for (int i = 0; i < numChildren; ++i)
        UnarcTraverseVobs(archiver, targetNode);
}

zCViewProgressBar* zCWorld::GetProgressBar() const
{
    return progressBar;
}

void zCWorld::Unarchive(zCArchiver& archiver)
{
    unarchiveFileLen = 0;
    unarchiveStartPosVobtree = 0;
    if (progressBar && archiver.GetFile()) {
        unarchiveFileLen = archiver.GetFile()->Size();
        progressBar->SetPercent(0, zSTRING());
    }
    const int loadDynamic = s_worldLoadMode == 1 || s_worldLoadMode == 4 || s_worldLoadMode == 5;
    const int savedStatic = s_worldLoadMode == 2;
    zSTRING chunkName;
    unsigned short version = 0;
    while (archiver.ReadChunkStart(chunkName, version)) {
        if (chunkName == zSTRING("MeshAndBsp")) {
            if (archiver.GetFile())
                LoadBspTree(*archiver.GetFile(), loadDynamic);
        } else if (chunkName == zSTRING("VobTree")) {
            if (!savedStatic) {
                if (!loadDynamic && s_worldLoadMode != 1) {
                    DisposeStaticWorld();
                    s_worldLoadMode = 4;
                }
                if (progressBar) {
                    if (archiver.GetFile())
                        unarchiveStartPosVobtree = archiver.GetFile()->Pos();
                    unarchiveFileLen -= unarchiveStartPosVobtree;
                    progressBar->SetRange(10, 80);
                }
                unarcVobList.numInArray = 0;
                UnarcTraverseVobs(archiver, 0);
                for (int i = 0; i < unarcVobList.numInArray; ++i)
                    unarcVobList.parray[i]->PostLoad();
                unarcVobList.numInArray = 0;
                if (progressBar)
                    progressBar->ResetRange();
            }
        } else if (chunkName == zSTRING("CutscenePlayer")) {
            if (csPlayer)
                csPlayer = reinterpret_cast<zCCSPlayer*>(archiver.ReadObject(csPlayer));
        } else if (chunkName == zSTRING("SkyCtrl")) {
            activeSkyControler = bspTree.bspTreeMode ? skyControlerOutdoor : skyControlerIndoor;
            activeSkyControler = static_cast<zCSkyControler*>(archiver.ReadObject(activeSkyControler));
        } else if (chunkName == zSTRING("WayNet")) {
            if (!savedStatic)
                wayNet = static_cast<zCWayNet*>(archiver.ReadObject(wayNet));
        } else if (chunkName == zSTRING("EndMarker")) {
            break;
        }
        archiver.SkipOpenChunk();
    }
    archiver.SkipOpenChunk();
    if (progressBar)
        progressBar->SetPercent(100, zSTRING());
}

int zCWorld::LoadWorld(const zSTRING& fileName, int loadMode)
{
    s_worldLoadMode = loadMode;
    DisposeVobs(0);
    zCVob::ResetIDCtr();
    if (loadMode == 0 || loadMode == 2 || loadMode == 3 || loadMode == 4)
        DisposeStaticWorld();
    zvertexBufferMan.StartChangeWorld();
    zoptions->ChangeDir(14);
    zCArchiver* archiver = zarcFactory->CreateArchiverRead(fileName, 0);
    if (archiver) {
        Unarchive(*archiver);
        archiver->Close();
        archiver->Release();
    }
    zvertexBufferMan.EndChangeWorld();
    s_bFadeOutFarVerts = zoptions->ReadBool(zSTRING("ENGINE"), zSTRING("zFarClipAlphaFade"), 1);
    if (compiled && !bspTree.bspTreeMode)
        s_bFadeOutFarVerts = 0;
    if (archiver) {
        m_strlevelName = fileName;
        bspTree.m_bRenderedFirstTime = 1;
    }
    return archiver != 0;
}

zCVob* zCWorld::MergeVobSubtree(const zSTRING& fileName, zCVob* parent, int mergeMode)
{
    s_worldMergeMode = mergeMode;
    if (mergeMode && !parent)
        return 0;
    mergeRootVob = mergeMode ? parent : 0;
    if (!mergeMode && parent && parent->globalVobTreeNode)
        mergeParentNode = parent->globalVobTreeNode;
    else
        mergeParentNode = &globalVobTree;
    s_worldLoadMode = 5;
    zoptions->ChangeDir(14);
    zCArchiver* archiver = zarcFactory->CreateArchiverRead(fileName, 0);
    if (archiver) {
        archiver->ReadObject(this);
        archiver->Close();
        archiver->Release();
    }
    zCVob* result = mergeRootVob;
    mergeRootVob = 0;
    mergeParentNode = 0;
    return result;
}

void zCWorld::ArcTraverseVobs(zCArchiver& archiver, zCTree<zCVob>* node, int)
{
    if (!node)
        return;
    if (progressBar && !(arcVobCounter & 7)) {
        const int percent = static_cast<int>(arcVobCounter / (numVobsInWorld + 0.1f) * 100.0f);
        progressBar->SetPercent(percent, zSTRING());
    }
    int numChildren = 0;
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next) {
        if (child->data && !(child->data->collButNoMove & 0x10))
            ++numChildren;
    }
    if (node->data && !(node->data->collButNoMove & 0x10)) {
        archiver.WriteObject(node->data);
        ++arcVobCounter;
    }
    archiver.WriteInt(numChildren);
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        ArcTraverseVobs(archiver, child, 0);
}

void zCWorld::Archive(zCArchiver& archiver)
{
    zCRenderLightContainer::S_ReleaseVobLightRefs();
    if (progressBar && archiver.GetFile())
        progressBar->SetPercent(0, zSTRING());
    if (archiveRoot == &globalVobTree && archiver.GetFile() &&
        (s_worldSaveMode == 1 || s_worldSaveMode == 3) && compiled) {
        archiver.WriteChunkStart("MeshAndBsp", 0);
        SaveBspTree(*archiver.GetFile());
        archiver.WriteChunkEnd();
    }
    if (s_worldSaveMode != 3 && archiveRoot &&
        (archiveRoot->data || archiveRoot->firstChild || zoneGlobalList.numInArray > 0)) {
        if (progressBar)
            progressBar->SetRange(10, 80);
        archiver.WriteChunkStart("VobTree", 0);
        arcVobCounter = 0;
        ArcTraverseVobs(archiver, archiveRoot, 1);
        archiver.WriteChunkEnd();
        if (progressBar)
            progressBar->ResetRange();
    }
    if (archiver.InSaveGame() && csPlayer) {
        archiver.WriteChunkStart("CutscenePlayer", 0);
        archiver.WriteObject(csPlayer);
        archiver.WriteChunkEnd();
    }
    if (archiver.InSaveGame()) {
        archiver.WriteChunkStart("SkyCtrl", 0);
        activeSkyControler = bspTree.bspTreeMode ? skyControlerOutdoor : skyControlerIndoor;
        archiver.WriteObject(activeSkyControler);
        archiver.WriteChunkEnd();
    }
    if (archiveWayNet) {
        archiver.WriteChunkStart("WayNet", 0);
        archiver.WriteObject(wayNet);
        archiver.WriteChunkEnd();
    }
    archiver.WriteChunkStart("EndMarker", 0);
    archiver.WriteChunkEnd();
    if (progressBar)
        progressBar->SetPercent(100, zSTRING());
}

int zCWorld::SaveWorld(const zSTRING& fileName, int saveMode, int saveGame, int export3DS)
{
    zCPolygon::S_ResetMorphedVerts();
    zCVob* cameraVob = 0;
    if (zCCamera::activeCam && zCCamera::activeCam->connectedVob) {
        cameraVob = zCCamera::activeCam->connectedVob;
        cameraVob->AddRef();
        RemoveVob(cameraVob);
    }
    s_worldSaveMode = saveMode;
    archiveRoot = &globalVobTree;
    archiveWayNet = saveMode != 3;
    int archiveMode = saveGame * 2 + 1;
    if (saveGame && saveMode == 0 &&
        zoptions->ReadBool(zSTRING("INTERNAL"), zSTRING("zFastSaveGames"), 1))
        archiveMode = 0;
    zoptions->ChangeDir(14);
    zCArchiver* archiver = zarcFactory->CreateArchiverWrite(fileName, archiveMode, 0, saveMode == 0);
    if (archiver) {
        archiver->WriteObject(this);
        archiver->Close();
        archiver->Release();
    }
    if (cameraVob) {
        AddVobAsChild(cameraVob, &globalVobTree);
        cameraVob->Release();
    }
    archiveRoot = 0;
    return archiver != 0;
}

int zCWorld::SaveVobSubtree(const zSTRING& fileName, zCVob* vob, int saveGame, int)
{
    if (!vob || vob->homeWorld != this)
        return 0;
    s_worldSaveMode = 2;
    archiveRoot = vob->globalVobTreeNode;
    archiveWayNet = 0;
    zoptions->ChangeDir(14);
    zCArchiver* archiver = zarcFactory->CreateArchiverWrite(fileName, saveGame * 2 + 1, 0, 0);
    if (archiver) {
        archiver->WriteObject(this);
        archiver->Close();
        archiver->Release();
    }
    archiveRoot = 0;
    return archiver != 0;
}

void zCWorld::TraverseVobTree(zCVobCallback& callback, void* data, zCTree<zCVob>* node)
{
    if (!node)
        node = &globalVobTree;
    zCVob* vob = node->data;
    zCTree<zCVob>* child = node->firstChild;
    while (child) {
        zCTree<zCVob>* next = child->next;
        TraverseVobTree(callback, data, child);
        child = next;
    }
    if (vob)
        callback.HandleVob(vob, data);
}

unsigned long zCWorld::GetVobHashIndex(const zSTRING& name)
{
    return zCChecksum::GetBufferCRC32(name.ToChar(), name.Length(), 0) & 0x7ff;
}

unsigned long zCWorld::GetVobHashIndex(zCVob* vob)
{
    return GetVobHashIndex(vob->GetObjectName());
}

void zCWorld::InsertVobHashTable(zCVob* vob)
{
    if (!vob->GetObjectName().Length())
        return;
    vobHashTable[GetVobHashIndex(vob)].InsertEnd(vob);
}

void zCWorld::RemoveVobHashTable(zCVob* vob)
{
    if (!vob->GetObjectName().Length())
        return;
    zCArray<zCVob*>& bucket = vobHashTable[GetVobHashIndex(vob)];
    for (int i = 0; i < bucket.numInArray; ++i) {
        if (bucket.parray[i] == vob) {
            --bucket.numInArray;
            if (i != bucket.numInArray)
                bucket.parray[i] = bucket.parray[bucket.numInArray];
            return;
        }
    }
}

zCVob* zCWorld::SearchVobHashTable(const zSTRING& name)
{
    if (!name.Length())
        return 0;
    zCArray<zCVob*>& bucket = vobHashTable[GetVobHashIndex(name)];
    for (int i = 0; i < bucket.numInArray; ++i) {
        if (bucket.parray[i]->GetObjectName() == name)
            return bucket.parray[i];
    }
    return 0;
}

void zCWorld::SearchVobListHashTable(const zSTRING& name, zCArray<zCVob*>& vobs)
{
    if (!name.Length())
        return;
    zCArray<zCVob*>& bucket = vobHashTable[GetVobHashIndex(name)];
    for (int i = 0; i < bucket.numInArray; ++i) {
        if (bucket.parray[i]->GetObjectName() == name)
            vobs.InsertEnd(bucket.parray[i]);
    }
}

void zCWorld::SetOwnerSession(zCSession* session)
{
    ownerSession = session;
    if (session) {
        csPlayer = session->GetCutsceneManager()->CreateCutscenePlayer(this);
        return;
    }
    if (csPlayer) {
        csPlayer->Release();
        csPlayer = 0;
    }
}

zCSession* zCWorld::GetOwnerSession()
{
    return ownerSession;
}

void zCWorld::ResetCutscenePlayer()
{
    if (csPlayer) {
        csPlayer->ResetCutscenePlayer();
        csPlayer->Release();
        csPlayer = 0;
    }
    if (ownerSession)
        csPlayer = ownerSession->GetCutsceneManager()->CreateCutscenePlayer(this);
}

void zCWorld::RegisterPerFrameCallback(zCWorldPerFrameCallback* callback)
{
    if (!perFrameCallbackList.IsInList(callback))
        perFrameCallbackList.InsertEnd(callback);
}

void zCWorld::UnregisterPerFrameCallback(zCWorldPerFrameCallback* callback)
{
    for (int i = 0; i < perFrameCallbackList.numInArray; ++i) {
        if (perFrameCallbackList.parray[i] == callback) {
            --perFrameCallbackList.numInArray;
            if (i != perFrameCallbackList.numInArray)
                perFrameCallbackList.parray[i] = perFrameCallbackList.parray[perFrameCallbackList.numInArray];
            return;
        }
    }
}

zCBspTree* zCWorld::GetBspTree()
{
    return &bspTree;
}
