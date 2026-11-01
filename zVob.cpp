#include "include/zVob.h"

#include <cmath>
#include <cstdlib>

static int eventState = -1;
static int eventManagerState = -1;
static zCClassDef zCEventMessageClassDef("zCEventMessage", "zCObject",
    zCEventMessage::_CreateNewInstance, 0, 0x2C, 1, "");
zCClassDef* zCEventMessage::classDef = &zCEventMessageClassDef;
static zCClassDef zCEventCoreClassDef("zCEventCore", "zCEventMessage",
    zCEventCore::_CreateNewInstance, 0, 0x4C, 0, "");
zCClassDef* zCEventCore::classDef = &zCEventCoreClassDef;
static zCClassDef zCVobLightPresetClassDef("zCVobLightPreset", "zCObject", NULL, 0, 0x80, 0, "");
zCClassDef* zCVobLightPreset::classDef = &zCVobLightPresetClassDef;

zCEventMessage::zCEventMessage()
    : subType(0), inCutscene(0)
{
}

zCEventMessage::~zCEventMessage()
{
}

// EXE: 0x005FCEF0
zCObject* zCEventMessage::_CreateNewInstance()
{
    zCEventMessage* message = new zCEventMessage;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

void zCEventMessage::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCEventMessage::_GetClassDef() const
{
    return classDef;
}

int zCEventMessage::IsOverlay()
{
    return 0;
}

int zCEventMessage::IsNetRelevant()
{
    return 0;
}

int zCEventMessage::IsHighPriority()
{
    return 0;
}

int zCEventMessage::IsJob()
{
    return 0;
}

int zCEventMessage::GetIgnoreCutsceneMode()
{
    return 0;
}

void zCEventMessage::Delete()
{
}

int zCEventMessage::IsDeleteable()
{
    return 1;
}

int zCEventMessage::IsDeleted()
{
    return 0;
}

void zCEventMessage::SetCutsceneMode(int mode)
{
    inCutscene = mode;
}

int zCEventMessage::GetCutsceneMode()
{
    return inCutscene;
}

int zCEventMessage::MD_GetNumOfSubTypes()
{
    return 0;
}

zSTRING zCEventMessage::MD_GetSubTypeString(int)
{
    return zSTRING("");
}

zSTRING zCEventMessage::MD_GetVobRefName()
{
    return zSTRING("");
}

void zCEventMessage::MD_SetVobRefName(const zSTRING&)
{
}

void zCEventMessage::MD_SetVobParam(zCVob*)
{
}

zCEventMessage::zTTimeBehavior zCEventMessage::MD_GetTimeBehavior()
{
    return TBZero;
}

float zCEventMessage::MD_GetMinTime()
{
    return 0.0f;
}

zCEventCore::zCEventCore()
    : otherVob(0), vobInstigator(0), damage(0), damageType(0), inflictorVob(0)
{
    hitLocation.n[0] = 0;
    hitLocation.n[1] = 0;
    hitLocation.n[2] = 0;
}

// EXE: 0x005FD250
zCObject* zCEventCore::_CreateNewInstance()
{
    zCEventCore* message = new zCEventCore;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

void zCEventCore::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCEventCore::_GetClassDef() const
{
    return classDef;
}

int zCEventCore::MD_GetNumOfSubTypes()
{
    return 6;
}

int zCEventCore::IsNetRelevant()
{
    return subType < 2 || subType > 4;
}

void zCEventScreenFX::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCVobLightPreset::zCVobLightPreset()
{
}

zCVobLightPreset::~zCVobLightPreset()
{
}

// EXE: 0x005FD520
zCObject* zCVobLightPreset::_CreateNewInstance()
{
    zCVobLightPreset* preset = new zCVobLightPreset;
    zCClassDef::ObjectCreated(preset, classDef);
    return preset;
}

void zCVobLightPreset::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCVobLightPreset::_GetClassDef() const
{
    return classDef;
}

static zCClassDef zCVobClassDef("zCVob", "zCObject", zCVob::_CreateNewInstance, 0, 0x120, 5, "");
zCClassDef* zCVob::classDef = &zCVobClassDef;
static zCClassDef zCVobLevelCompoClassDef("zCVobLevelCompo", "zCVob",
    zCVobLevelCompo::_CreateNewInstance, 0, 0x120, 0, "");
zCClassDef* zCVobLevelCompo::classDef = &zCVobLevelCompoClassDef;

zCVob::zCVob()
    : visual(0), homeWorld(0), trafo(0), showVisual(1), castDynShadow(0), collButNoMove(0), m_zBias(1), type(0),
      visualAlpha(1.0f), callback_ai(0), eventManager(0), vobPresetName(0), rigidBody(0),
      m_poCollisionObject(0), m_poCollisionObjectClass(0), globalVobTreeNode(0), groundPoly(0),
      nextOnTimer(3.40282347e+38f), sleepingMode(0), isInMovementMode(0)
{
    trafoObjToWorld.m[0][0] = 1.0f;
    trafoObjToWorld.m[1][1] = 1.0f;
    trafoObjToWorld.m[2][2] = 1.0f;
    trafoObjToWorld.m[3][3] = 1.0f;
    SetCollDetStat(0);
    SetCollDetDyn(0);
}

zCVob::~zCVob()
{
    if (refCtr > 0)
        zerr.Message("D: tried to delete zCVob with refCtr>0 !");
    if (homeWorld)
        homeWorld->RemoveVob(this);

    if (globalVobTreeNode) {
        zCTree* node = globalVobTreeNode;
        if (node->parent) {
            if (node->parent->firstChild == node)
                node->parent->firstChild = node->next;
            if (node->prev)
                node->prev->next = node->next;
            if (node->next)
                node->next->prev = node->prev;
            node->next = 0;
            node->prev = 0;
        }
        while (node->firstChild) {
            zCTree* child = node->firstChild;
            node->firstChild = child->next;
            if (child->prev)
                child->prev->next = child->next;
            if (child->next)
                child->next->prev = child->prev;
            child->next = 0;
            child->prev = 0;
        }
        delete node;
        globalVobTreeNode = 0;
    }

    if (visual) {
        visual->Release();
        visual = 0;
    }
    if (callback_ai) {
        callback_ai->Release();
        callback_ai = 0;
    }
    if (eventManager) {
        eventManager->Release();
        eventManager = 0;
    }
    delete vobPresetName;
    vobPresetName = 0;
    delete rigidBody;
    rigidBody = 0;
    delete m_poCollisionObject;
    m_poCollisionObject = 0;
    m_poCollisionObjectClass = 0;
    groundPoly = 0;
    globalVobTreeNode = 0;
    homeWorld = 0;
    delete[] touchVobList.parray;
    touchVobList.parray = 0;
    delete[] vobLeafList.parray;
    vobLeafList.parray = 0;
}

// EXE: 0x005FD940
zCObject* zCVob::_CreateNewInstance()
{
    zCVob* vob = new zCVob;
    zCClassDef::ObjectCreated(vob, classDef);
    return vob;
}

zCClassDef* zCVob::_GetClassDef() const
{
    return classDef;
}

void zCVob::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

const zSTRING* zCVob::GetTriggerTarget(int) const
{
    return NULL;
}

void zCVob::OnTrigger(zCVob*, zCVob*)
{
}

void zCVob::OnUntrigger(zCVob*, zCVob*)
{
}

void zCVob::OnTouch(zCVob*)
{
}

void zCVob::OnUntouch(zCVob*)
{
}

void zCVob::OnTouchLevel()
{
}

void zCVob::OnDamage(zCVob*, zCVob*, float, int, const zVEC3&)
{
}

void zCVob::OnMessage(zCEventMessage*, zCVob*)
{
}

zCVobLevelCompo::~zCVobLevelCompo()
{
}

// EXE: 0x005FDB90
zCObject* zCVobLevelCompo::_CreateNewInstance()
{
    zCVobLevelCompo* vob = new zCVobLevelCompo;
    zCClassDef::ObjectCreated(vob, classDef);
    vob->SetCollDetStat(0);
    vob->SetCollDetDyn(0);
    return vob;
}

void zCVobLevelCompo::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCVobLevelCompo::_GetClassDef() const
{
    return classDef;
}

class zCCollisionDetector {
};

// EXE: 0x009A37E8
zCMesh* zCVob::s_shadowMesh;
// EXE: 0x009A37F8
zCCollisionDetector* zCVob::s_poCollisionDetector;
// EXE: 0x009A37E4
int zCVob::s_vobAlreadyInMovement;
// EXE: 0x009A37FC
int zCVobLevelCompo::unarcIgnoreVisuals;
unsigned long zCVob::s_nextFreeID;

void zCVob::InitVobSystem()
{
    s_shadowMesh = zCMesh::Load("_intern\\groundShadow.3ds", 0);
    if (s_shadowMesh) {
        s_shadowMesh->polyList.array[0]->material->rndAlphaBlendFunc = zRND_ALPHA_FUNC_MUL;
        s_shadowMesh->SetStaticLight(zCOLOR(255, 255, 255, 255));
        s_shadowMesh->ResetLightDynToLightStat();
    }
    s_poCollisionDetector = new zCCollisionDetector;
    zCCollObjectBase::S_RegisterCollisionTestFuncs(s_poCollisionDetector);
}

void zCVob::CleanupVobSystem()
{
    if (s_shadowMesh)
        s_shadowMesh->Release();
    delete s_poCollisionDetector;
    s_shadowMesh = 0;
    s_poCollisionDetector = 0;
}

void zCVob::ResetIDCtr()
{
    s_nextFreeID = 0;
}

unsigned long zCVob::GetNextFreeVobID()
{
    return s_nextFreeID;
}

void zCVob::SetVobID(const unsigned long&)
{
}

void zCVob::SetAI(zCAIBase* ai)
{
    if (callback_ai == ai)
        return;
    if (callback_ai)
        callback_ai->Release();
    callback_ai = ai;
    if (callback_ai)
        callback_ai->AddRef();
}

void zCVob::SetVobPresetName(const zSTRING&)
{
}

const zSTRING& zCVob::GetVobPresetName() const
{
    static zSTRING empty;
    return vobPresetName ? *vobPresetName : empty;
}

zCRigidBody* zCVob::GetRigidBody()
{
    if (!rigidBody)
        rigidBody = new zCRigidBody;
    return rigidBody;
}

void zCVob::ArchivePacked(zCArchiver& archiver)
{
    archiver.WriteInt(1);
    archiver.WriteRaw(&bbox3D, sizeof(bbox3D));
    archiver.WriteRaw(&trafoObjToWorld, sizeof(trafoObjToWorld));
    archiver.WriteFloat(m_aniModeStrength);
    archiver.WriteFloat(m_fVobFarClipZScale);

    const zSTRING& presetName = GetVobPresetName();
    if (presetName.Length())
        archiver.WriteString(presetName);
    if (GetObjectName().Length())
        archiver.WriteString(GetObjectName());
    const zSTRING visualName = visual ? visual->GetVisualName() : zSTRING();
    if (visualName.Length())
        archiver.WriteString(visualName);
    if (visual)
        archiver.WriteObject(visual);
    if (callback_ai)
        archiver.WriteObject(callback_ai);
    if (archiver.InSaveGame() && eventManager)
        archiver.WriteObject(eventManager);
}

void zCVob::ArchiveVerbose(zCArchiver& archiver)
{
    archiver.WriteString(vobPresetName ? *vobPresetName : GetVobPresetName());
    archiver.WriteRaw(&bbox3D, sizeof(bbox3D));
    archiver.WriteRaw(&trafoObjToWorld, sizeof(trafoObjToWorld));
    archiver.WriteString(GetObjectName());
    archiver.WriteString(visual ? visual->GetVisualName() : zSTRING());
    archiver.WriteBool(showVisual);
    archiver.WriteInt(m_AniMode);
    archiver.WriteFloat(m_aniModeStrength);
    archiver.WriteFloat(m_fVobFarClipZScale);
    archiver.WriteInt(castDynShadow);
    archiver.WriteInt(m_zBias);
    archiver.WriteBool(collButNoMove);
    archiver.WriteObject(visual);
    archiver.WriteObject(callback_ai);
    if (archiver.InSaveGame()) {
        archiver.WriteObject(eventManager);
        archiver.WriteBool(physicsEnabled);
    }
}

void zCVob::Archive(zCArchiver& archiver)
{
    if (archiver.InSaveGame() || archiver.InBinaryMode())
        ArchivePacked(archiver);
    else
        ArchiveVerbose(archiver);
    if (archiver.InSaveGame()) {
        archiver.WriteByte(sleepingMode);
        archiver.WriteFloat(nextOnTimer == 3.40282347e+38f ? nextOnTimer :
            nextOnTimer - ztimer.totalTimeFloat);
        if (physicsEnabled)
            GetRigidBody()->Archive(archiver);
    }
}

void zCVob::UnarchivePacked(zCArchiver& archiver, int)
{
    archiver.ReadRaw(&bbox3D, sizeof(bbox3D));
    archiver.ReadRaw(&trafoObjToWorld, sizeof(trafoObjToWorld));
    const zSTRING presetName = archiver.ReadString();
    if (presetName.Length()) {
        delete vobPresetName;
        vobPresetName = new zSTRING(presetName);
    }
    SetObjectName(archiver.ReadString());
    m_aniModeStrength = archiver.ReadFloat();
    m_fVobFarClipZScale = archiver.ReadFloat();
    archiver.ReadObject(visual);
    archiver.ReadObject(callback_ai);
    if (archiver.InSaveGame()) {
        archiver.ReadObject(eventManager);
        physicsEnabled = archiver.ReadBool();
    }
}

void zCVob::UnarchiveVerbose(zCArchiver& archiver)
{
    const zSTRING presetName = archiver.ReadString();
    if (presetName.Length()) {
        delete vobPresetName;
        vobPresetName = new zSTRING(presetName);
    }
    archiver.ReadRaw(&bbox3D, sizeof(bbox3D));
    archiver.ReadRaw(&trafoObjToWorld, sizeof(trafoObjToWorld));
    SetObjectName(archiver.ReadString());
    showVisual = archiver.ReadBool();
    m_AniMode = archiver.ReadInt();
    m_aniModeStrength = archiver.ReadFloat();
    m_fVobFarClipZScale = archiver.ReadFloat();
    m_zBias = archiver.ReadInt();
    archiver.ReadObject(visual);
    archiver.ReadObject(callback_ai);
    if (archiver.InSaveGame()) {
        archiver.ReadObject(eventManager);
        physicsEnabled = archiver.ReadBool();
    }
}

void zCVob::Unarchive(zCArchiver& archiver)
{
    nextOnTimer = 3.40282347e+38f;
    const int packed = archiver.ReadInt();
    if (packed)
        UnarchivePacked(archiver, packed);
    else
        UnarchiveVerbose(archiver);
    if (archiver.InSaveGame()) {
        sleepingMode = archiver.ReadByte();
        nextOnTimer = archiver.ReadFloat();
        if (nextOnTimer != 3.40282347e+38f)
            nextOnTimer += ztimer.totalTimeFloat;
        if (physicsEnabled)
            GetRigidBody()->Unarchive(archiver);
    }
}

void zCVob::SetVobName(const zSTRING& name)
{
    SetObjectName(name);
}

zCEventManager* zCVob::GetEM(int dontCreate)
{
    if (!eventManager && !dontCreate)
        eventManager = new zCEventManager;
    return eventManager;
}

int zCVob::TraceRay(const zVEC3& origin, const zVEC3& direction, int flags,
    zTTraceRayReport& report)
{
    if (!visual || !homeWorld)
        return 0;
    const int result = homeWorld->TraceRayNearestHit(origin, direction, this, flags);
    report = homeWorld->traceRayReport;
    return result;
}

zCOLOR zCVob::CalcLightSampleAtOrigin()
{
    if (!homeWorld) {
        const zCOLOR black = { 0, 0, 0, 255 };
        return black;
    }
    const zCOLOR stat = GetLightColorStat();
    const zCOLOR dyn = GetLightColorDyn();
    zCOLOR result;
    result.r = stat.r + dyn.r > 255 ? 255 : stat.r + dyn.r;
    result.g = stat.g + dyn.g > 255 ? 255 : stat.g + dyn.g;
    result.b = stat.b + dyn.b > 255 ? 255 : stat.b + dyn.b;
    result.a = 255;
    return result;
}

zCOLOR zCVob::GetLightColorStat()
{
    if (castDynShadow & 4) {
        castDynShadow &= ~4;
        zVEC3 color = { lightColorStat.r, lightColorStat.g, lightColorStat.b };
        CalcStaticLight(color);
        lightColorStat.r = static_cast<unsigned char>(color.n[0]);
        lightColorStat.g = static_cast<unsigned char>(color.n[1]);
        lightColorStat.b = static_cast<unsigned char>(color.n[2]);
        lightColorStat.a = 255;
    }
    if (groundPoly && groundPoly->lightmap)
        return lightColorStat;
    const int intensity = (lightColorStat.r * 150 + lightColorStat.g * 77 +
        lightColorStat.b * 29) >> 8;
    return homeWorld->GetActiveSkyControler()->GetDaylightColorFromIntensity(intensity);
}

zCOLOR zCVob::GetLightColorStatGroundPoly()
{
    if (castDynShadow & 4)
        GetLightColorStat();
    return lightColorStat;
}

zCOLOR zCVob::GetLightColorDyn()
{
    if (castDynShadow & 8) {
        castDynShadow &= ~8;
        zCArray<zCVobLight*> lights;
        for (int i = 0; i < vobLeafList.numInArray; ++i) {
            zCBspLeaf* leaf = vobLeafList.parray[i];
            for (int j = 0; j < leaf->lightVobList.numInArray; ++j) {
                zCVobLight* light = leaf->lightVobList.parray[j];
                if (!light->lightData.isStatic)
                    lights.InsertEnd(light);
            }
        }
        const zVEC3 position = {
            trafoObjToWorld.m[0][3], trafoObjToWorld.m[1][3], trafoObjToWorld.m[2][3]
        };
        lightColorDyn = zCVobLight::SumLightsAtPositionWS(lights, position, 0);
    }
    return lightColorDyn;
}

int zCVob::CalcStaticLight(zVEC3& color) const
{
    if (!homeWorld) {
        color.n[0] = 160.0f;
        color.n[1] = 160.0f;
        color.n[2] = 160.0f;
        return 0;
    }

    const zVEC3 center = {
        (bbox3D.maxs.n[0] + bbox3D.mins.n[0]) * 0.5f,
        (bbox3D.maxs.n[1] + bbox3D.mins.n[1]) * 0.5f,
        (bbox3D.maxs.n[2] + bbox3D.mins.n[2]) * 0.5f
    };
    if (groundPoly) {
        color = groundPoly->GetLightStatAtPos(center);
        return 1;
    }

    zCArray<zCVobLight*> lights;
    for (int i = 0; i < vobLeafList.numInArray; ++i) {
        zCBspLeaf* leaf = vobLeafList.parray[i];
        for (int j = 0; j < leaf->lightVobList.numInArray; ++j) {
            zCVobLight* light = leaf->lightVobList.parray[j];
            if (light->lightData.isStatic)
                lights.InsertEnd(light);
        }
    }
    const zCOLOR light = zCVobLight::SumLightsAtPositionWS(lights, center, 1);
    color.n[0] = light.r;
    color.n[1] = light.g;
    color.n[2] = light.b;
    return 1;
}

const zSTRING* zCVob::GetSectorNameVobIsIn() const
{
    return homeWorld->bspTree.GetSectorNameVobIsIn(this);
}

void zCVob::LoadGroundShadowMesh()
{
    s_shadowMesh = zCMesh::Load("_intern\\groundShadow.3ds", 0);
    if (s_shadowMesh) {
        s_shadowMesh->polyList.array[0]->material->rndAlphaBlendFunc = zRND_ALPHA_FUNC_MUL;
        s_shadowMesh->SetStaticLight(zCOLOR(255, 255, 255, 255));
        s_shadowMesh->ResetLightDynToLightStat();
    }
}

void zCVob::DeleteGroundShadowMesh()
{
    if (s_shadowMesh)
        s_shadowMesh->Release();
    s_shadowMesh = 0;
}

void zCVob::SetGroundShadowSize(const zVEC2& size)
{
    groundShadowSizePacked = (static_cast<unsigned long>(size.n[1] * 8.0f) << 16) |
        static_cast<unsigned long>(size.n[0] * 8.0f);
}

zVEC2 zCVob::GetGroundShadowSize() const
{
    zVEC2 result = {
        static_cast<float>(groundShadowSizePacked & 0xFFFF) * 0.125f,
        static_cast<float>(groundShadowSizePacked >> 16) * 0.125f
    };
    return result;
}

void zCVob::RenderGroundShadow(zTRenderContext& context)
{
    if (s_shadowMesh && groundPoly && visualAlpha < 0.7f)
        s_shadowMesh->Render(context);
}

zVEC2 operator-(const zVEC2& first, const zVEC2& second)
{
    zVEC2 result = { first.n[0] - second.n[0], first.n[1] - second.n[1] };
    return result;
}

zCVisual* zCVob::GetClassHelperVisual() const
{
    zCClassDef* objectClass = _GetClassDef();
    zCVisual* helper = zCVisual::LoadVisual(zSTRING("INVISIBLE_") + objectClass->className + ".3DS");
    if (!helper)
        helper = zCVisual::LoadVisual("INVISIBLE_DEFAULT.3DS");
    return helper;
}

void zCVob::CalcRenderAlpha(float distance, zCVisual*& renderVisual, float& alpha)
{
    if (!homeWorld) {
        alpha = 0;
        return;
    }
    if (collButNoMove & 0x20) {
        alpha = 1.0f;
        return;
    }

    const float volume = GetBBox3DLocal().GetVolume();
    const float volumeFactor = volume < 1000000000.0f
        ? (volume - 125000.0f) * 1.00012498e-09f
        : 1.0f;
    const float clipScale = (volumeFactor + 1.0f) * 0.5f;
    float farClip = zCCamera::activeCam->farClipZ * clipScale;
    const float worldClip = homeWorld->vobFarClipZScalability * homeWorld->vobFarClipZ *
        m_fVobFarClipZScale * clipScale;
    if (worldClip < farClip)
        farClip = worldClip;
    float nearFade = farClip * 0.9f;
    if (visual && visual->lodNearFadeOutDistance < nearFade)
        nearFade = visual->lodNearFadeOutDistance;

    renderVisual = visual;
    alpha = 0;
    if (distance < farClip) {
        alpha = 1.0f;
        if (distance >= nearFade && farClip != nearFade)
            alpha = (farClip - distance) / (farClip - nearFade);
    }
    if ((showVisual & 4) && visualAlpha < alpha)
        alpha = visualAlpha;
}

int zCVob::s_renderVobs = 1;
int zCVob::s_showHelperVisuals = 1;
int zCVob::s_enableAnimations = 1;

int zCVob::Render(zTRenderContext& context)
{
    if (!s_renderVobs || !homeWorld)
        return 0;
    context.vob = this;
    if (visual)
        visual->Render(context);
    else if (s_showHelperVisuals) {
        zCVisual* helper = GetClassHelperVisual();
        if (helper)
            helper->Render(context);
    }
    if (castDynShadow & 3)
        RenderGroundShadow(context);
    return 1;
}

zCOLOR zCVob::GetLightColorAtCenter() const
{
    const zCOLOR black = { 0, 0, 0, 255 };
    if (!homeWorld || homeWorld->compiled) {
        zCArray<zCVobLight*> lights;
        for (int i = 0; i < vobLeafList.numInArray; ++i) {
            zCBspLeaf* leaf = vobLeafList.parray[i];
            for (int j = 0; j < leaf->lightVobList.numInArray; ++j)
                lights.InsertEnd(leaf->lightVobList.parray[j]);
        }
        if (lights.numInArray) {
            const zVEC3 position = {
                trafoObjToWorld.m[0][3], trafoObjToWorld.m[1][3], trafoObjToWorld.m[2][3]
            };
            return zCVobLight::SumLightsAtPositionWS(lights, position, 0);
        }
    }
    return black;
}

void zCVob::RemoveVobFromWorld()
{
    if (homeWorld)
        homeWorld->RemoveVob(this);
}

void zCVob::RemoveVobSubtreeFromWorld()
{
    if (homeWorld)
        homeWorld->RemoveVobSubtree(this);
}

void zCVob::ThisVobAddedToWorld(zCWorld* world)
{
    if (visual)
        visual->HostVobAddedToWorld(this, world);
}

void zCVob::ThisVobRemovedFromWorld(zCWorld* world)
{
    if (visual)
        visual->HostVobRemovedFromWorld(this, world);
}

void zCVob::AddRefVobSubtree(zCTree* node, int)
{
    if (!node)
        node = globalVobTreeNode;
    if (!node)
        return;
    if (node->data)
        node->data->AddRef();
    for (zCTree* child = node->firstChild; child; child = child->next)
        AddRefVobSubtree(child, 0);
}

void zCVob::ReleaseVobSubtree(zCTree* node)
{
    if (!node)
        node = globalVobTreeNode;
    if (!node)
        return;
    for (zCTree* child = node->firstChild; child; child = child->next)
        ReleaseVobSubtree(child);
    if (node->data)
        node->data->Release();
}

void zCVob::RemoveWorldDependencies(int removeChildren)
{
    if (homeWorld) {
        if (callback_ai)
            callback_ai->HostVobRemovedFromWorld(this, homeWorld);
        if (visual)
            visual->HostVobRemovedFromWorld(this, homeWorld);
    }
    for (int i = 0; i < touchVobList.numInArray; ++i) {
        zCVob* other = touchVobList.parray[i];
        for (int j = 0; j < other->touchVobList.numInArray; ++j) {
            if (other->touchVobList.parray[j] == this) {
                --other->touchVobList.numInArray;
                if (j != other->touchVobList.numInArray)
                    other->touchVobList.parray[j] =
                        other->touchVobList.parray[other->touchVobList.numInArray];
                break;
            }
        }
        other->GetEM(0)->OnUntouch(this);
    }
    touchVobList.numInArray = 0;
    SetSleepingMode(1);
    if (removeChildren)
        RemoveVobFromGlobalVobTree();
    if (homeWorld)
        homeWorld->bspTree.RemoveVob(this);
    if (trafo && (!globalVobTreeNode || !globalVobTreeNode->parent ||
        !globalVobTreeNode->parent->data)) {
        delete trafo;
        trafo = 0;
    }
    groundPoly = 0;
    homeWorld = 0;
    lastTimeDrawn = -1;
}

void zCVob::RemoveVobFromGlobalVobTree()
{
    if (!globalVobTreeNode)
        return;
    zCTree* node = globalVobTreeNode;
    if (node->parent && node->parent->firstChild == node)
        node->parent->firstChild = node->next;
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
    delete node;
    globalVobTreeNode = 0;
}

void zCVob::RemoveVobFromBspTree()
{
    homeWorld = 0;
}

zSTRING zCVob::GetVobInfo()
{
    zSTRING result = _GetClassDef()->className;
    if (GetObjectName().Length())
        result += zSTRING(", name: ") + GetObjectName();
    if (visual)
        result += zSTRING(", visual: ") + visual->GetVisualName();
    if (callback_ai)
        result += zSTRING(", AI: ") + callback_ai->_GetClassDef()->className;
    result += ", leafs: ";
    result += zSTRING().Sprintf("%d", vobLeafList.numInArray);
    result += ", refCtr: ";
    result += zSTRING().Sprintf("%d", refCtr);
    return result;
}

int zCVob::s_ignoreVisuals;

void zCVob::SetVisual(zCVisual* newVisual)
{
    if (s_ignoreVisuals || visual == newVisual)
        return;
    if (visual && homeWorld)
        visual->HostVobRemovedFromWorld(this, homeWorld);
    if (visual)
        visual->Release();
    visual = newVisual;
    showVisual = visual != 0;
    if (visual) {
        visual->AddRef();
        visual->SetVisualUsedBy(this);
        if (homeWorld)
            visual->HostVobAddedToWorld(this, homeWorld);
    }
}

void zCVob::SetVisual(const zSTRING& fileName)
{
    if (!fileName.Length() || s_ignoreVisuals)
        return;
    zCVisual* newVisual = zCVisual::LoadVisual(fileName.Upper());
    if (newVisual) {
        SetVisual(newVisual);
        newVisual->Release();
    }
}

void zCVob::SetSleeping(int sleeping)
{
    SetSleepingMode(sleeping != 0);
}

void zCVob::SetSleepingMode(int mode)
{
    if ((sleepingMode & 3) == mode)
        return;
    sleepingMode = mode;
    if (!mode)
        nextOnTimer = 3.40282347e+38f;
}

void zCVob::UpdateVisualDependencies(int endFlags)
{
    if (!visual)
        return;
    BeginMovement();
    EndMovement(endFlags);
}

void zCVob::AddVobToWorld_CorrectParentDependencies()
{
    if (globalVobTreeNode && globalVobTreeNode->parent && globalVobTreeNode->parent->data)
        UpdateVisualDependencies(0);
}

void zCVob::ResetOnTimer()
{
    nextOnTimer = 3.40282347e+38f;
}

void zCVob::SetOnTimer(float seconds)
{
    nextOnTimer = ztimer.totalTimeFloat + seconds;
    if (!(sleepingMode & 3))
        SetSleepingMode(2);
}

int zCVob::IsOnTimer() const
{
    return nextOnTimer != 3.40282347e+38f;
}

void zCVob::ProcessOnTimer()
{
    if (ztimer.totalTimeFloat < nextOnTimer)
        return;
    nextOnTimer = 3.40282347e+38f;
    OnTimer();
}

void zCVob::OnTimer()
{
}

void zCVob::OnTick()
{
}

void zCVob::PostLoad()
{
}

int zCVob::GetCharacterClass()
{
    return 0;
}

int zCVob::CanThisCollideWith(zCVob*)
{
    return 1;
}

int zCVob::GetScriptInstance(zSTRING*& name, int& instance)
{
    name = 0;
    instance = 0;
    return 0;
}

int zCVob::SetByScriptInstance(const zSTRING*, int)
{
    return 0;
}

int zCVob::GetCSStateFlags()
{
    return 0;
}

void zCVob::DoFrameActivity()
{
    if (!(sleepingMode & 3))
        return;
    AddRef();
    if (callback_ai)
        callback_ai->DoAI(this);
    OnTick();
    ProcessOnTimer();
    EndMovement();
    Release();
}

zCVob::zTModelLimbColl::zTModelLimbColl()
    : hitVob(0), hitMaterial(0)
{
    hitModelNodeList.next = 0;
    hitModelNodeList.data = 0;
}

void zCVob::CheckModelLimbCollision(zCModelNodeInst*, int, zCList<zTModelLimbColl>& collisions)
{
    if (!visual || !homeWorld)
        return;
    for (int i = 0; i < touchVobList.numInArray; ++i) {
        zCVob* other = touchVobList.parray[i];
        if (!other || other == this || !other->visual || !bbox3D.IsIntersecting(other->bbox3D))
            continue;
        zTModelLimbColl* collision = new zTModelLimbColl;
        collision->hitVob = other;
        collision->approxCollisionPos.n[0] = (other->bbox3D.mins.n[0] + other->bbox3D.maxs.n[0]) * 0.5f;
        collision->approxCollisionPos.n[1] = (other->bbox3D.mins.n[1] + other->bbox3D.maxs.n[1]) * 0.5f;
        collision->approxCollisionPos.n[2] = (other->bbox3D.mins.n[2] + other->bbox3D.maxs.n[2]) * 0.5f;
        collisions.Insert(collision);
    }
}

int zTBBox3D::IsIntersecting(const zTBBox3D& other) const
{
    if (other.maxs.n[0] < mins.n[0] || other.mins.n[0] > maxs.n[0] ||
        other.maxs.n[2] < mins.n[2] || other.mins.n[2] > maxs.n[2] ||
        other.maxs.n[1] < mins.n[1] || other.mins.n[1] > maxs.n[1])
        return 0;
    return 1;
}

zCVob::zTModelLimbColl::~zTModelLimbColl()
{
}

void zCVob::CheckModelLimbCollisionLevel(zCModelNodeInst* node, int flags,
    zCList<zTModelLimbColl>& collisions)
{
    CheckModelLimbCollision(node, flags, collisions);
}

void zCVob::CheckModelLimbCollisionPolys(zCModelNodeInst*, int, zCPolygon** polygons,
    const int& numPolygons, zCList<zTModelLimbColl>& collisions)
{
    for (int i = 0; i < numPolygons; ++i) {
        zCPolygon* polygon = polygons[i];
        if (!polygon || (polygon->TFlags_flags[0] & 3) || (polygon->material->smooth & 0x10) ||
            polygon == groundPoly)
            continue;
        zTModelLimbColl* collision = new zTModelLimbColl;
        collision->hitMaterial = polygon->material;
        const zTBBox3D bounds = polygon->GetBBox3D();
        collision->approxCollisionPos.n[0] = (bounds.mins.n[0] + bounds.maxs.n[0]) * 0.5f;
        collision->approxCollisionPos.n[1] = (bounds.mins.n[1] + bounds.maxs.n[1]) * 0.5f;
        collision->approxCollisionPos.n[2] = (bounds.mins.n[2] + bounds.maxs.n[2]) * 0.5f;
        collisions.Insert(collision);
    }
}

zMAT4 zCVob::GetTrafoModelNodeToWorld(const zSTRING&)
{
    return trafoObjToWorld;
}

zMAT4 zCVob::GetTrafoModelNodeToWorld(zCModelNodeInst* node)
{
    return node ? node->trafoObjToWorld : trafoObjToWorld;
}

zTBBox2D zCVob::GetScreenBBox2D()
{
    zTBBox2D result = {};
    const zVEC3 center = {
        (bbox3D.mins.n[0] + bbox3D.maxs.n[0]) * 0.5f,
        (bbox3D.mins.n[1] + bbox3D.maxs.n[1]) * 0.5f,
        (bbox3D.mins.n[2] + bbox3D.maxs.n[2]) * 0.5f
    };
    const float deltaX = (bbox3D.maxs.n[0] - bbox3D.mins.n[0]) * 0.5f;
    const float deltaY = (bbox3D.maxs.n[1] - bbox3D.mins.n[1]) * 0.5f;
    const float deltaZ = (bbox3D.maxs.n[2] - bbox3D.mins.n[2]) * 0.5f;
    const float radius = std::sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
    const zVEC3 cameraCenter = zCCamera::activeCam->camMatrix * center;
    if (cameraCenter.n[2] < 2.0f) {
        result.mins.n[0] = 3.402823466e38f;
        result.mins.n[1] = 3.402823466e38f;
        result.maxs.n[0] = -3.402823466e38f;
        result.maxs.n[1] = -3.402823466e38f;
        return result;
    }

    zVEC3 cameraRight = cameraCenter;
    cameraRight.n[0] += radius;
    zVEC3 cameraUp = cameraCenter;
    cameraUp.n[1] += radius;
    float centerX;
    float centerY;
    float rightX;
    float rightY;
    float upX;
    float upY;
    zCCamera::activeCam->Project(&cameraCenter, 1.0f, centerX, centerY);
    zCCamera::activeCam->Project(&cameraRight, 1.0f, rightX, rightY);
    zCCamera::activeCam->Project(&cameraUp, 1.0f, upX, upY);
    const float extentX = std::fabs(rightX - centerX) * 2.0f;
    const float extentY = std::fabs(upY - centerY) * 2.0f;
    result.mins.n[0] = centerX - extentX;
    result.mins.n[1] = centerY - extentY;
    result.maxs.n[0] = centerX + extentX;
    result.maxs.n[1] = centerY + extentY;
    return result;
}

void zCCamera::Project(const zVEC3* point, float scale, float& x, float& y) const
{
    x = viewDistanceX * point->n[0] * scale + vpData.xcenter;
    y = vpData.ycenter - viewDistanceY * point->n[1] * scale;
}

void zCCamera::Project(const zVEC3* point, int& x, int& y)
{
    const float depthInv = 1.0f / point->n[2];
    x = viewDistanceX * point->n[0] * depthInv + vpData.xcenter;
    y = vpData.ycenter - viewDistanceY * point->n[1] * depthInv;
}

void zCVob::RepairIllegalFloats(zMAT4& trafo)
{
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            if (!std::isfinite(trafo.m[row][column]))
                trafo.m[row][column] = row == column ? 1.0f : 0;
            else if (column == 3 && row < 3 && std::fabs(trafo.m[row][column]) > 1000000.0f)
                trafo.m[row][column] = 0;
        }
    }
}

void zCVob::PackStateBinary(zCBuffer& buffer) const
{
    buffer.SetMode(WRITE);
    buffer.WriteFloat(trafoObjToWorld.m[0][3], 32);
    buffer.WriteFloat(trafoObjToWorld.m[1][3], 32);
    buffer.WriteFloat(trafoObjToWorld.m[2][3], 32);
    const zVEC3 angles = trafoObjToWorld.GetEulerAngles();
    buffer.WriteFloat(angles.n[0], 32);
    buffer.WriteFloat(angles.n[1], 32);
    buffer.WriteFloat(angles.n[2], 32);
}

void zCVob::UnpackStateBinary(zCBuffer& buffer)
{
    buffer.SetMode(READ);
    zVEC3 position;
    position.n[0] = buffer.ReadFloat(32);
    position.n[1] = buffer.ReadFloat(32);
    position.n[2] = buffer.ReadFloat(32);
    zVEC3 angles;
    angles.n[0] = buffer.ReadFloat(32);
    angles.n[1] = buffer.ReadFloat(32);
    angles.n[2] = buffer.ReadFloat(32);
    BeginMovement();
    SetPositionWorld(position);
    trafoObjToWorld.SetByEulerAngles(angles);
    SetTrafoObjToWorld(trafoObjToWorld);
    EndMovement(1);
}

zCVobLevelCompo::zCVobLevelCompo()
{
    type = 3;
    SetCollDetStat(0);
    SetCollDetDyn(0);
}

int zCVobLevelCompo::HasIdentityTrafo() const
{
    const float translationLength2 = trafoObjToWorld.m[0][3] * trafoObjToWorld.m[0][3] +
        trafoObjToWorld.m[1][3] * trafoObjToWorld.m[1][3] +
        trafoObjToWorld.m[2][3] * trafoObjToWorld.m[2][3];
    if (translationLength2 > 0.001f)
        return 0;
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            if (trafoObjToWorld.m[row][column] > 1.001f)
                return 0;
        }
    }
    return 1;
}

void zCVobLevelCompo::Archive(zCArchiver& archiver)
{
    if (!HasIdentityTrafo()) {
        trafoObjToWorld = zMAT4();
        trafoObjToWorld.m[0][0] = 1.0f;
        trafoObjToWorld.m[1][1] = 1.0f;
        trafoObjToWorld.m[2][2] = 1.0f;
        trafoObjToWorld.m[3][3] = 1.0f;
    }
    zCVob::Archive(archiver);
}

void zCVobLevelCompo::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    if (!HasIdentityTrafo()) {
        trafoObjToWorld = zMAT4();
        trafoObjToWorld.m[0][0] = 1.0f;
        trafoObjToWorld.m[1][1] = 1.0f;
        trafoObjToWorld.m[2][2] = 1.0f;
        trafoObjToWorld.m[3][3] = 1.0f;
    }
}

void zCVobLevelCompo::SetVisual(zCVisual* newVisual)
{
    zCVob::SetVisual(newVisual);
    SetCollDetStat(0);
    SetCollDetDyn(0);
}

void zCVobLevelCompo::SetVisual(const zSTRING& fileName)
{
    zCVob::SetVisual(fileName);
    SetCollDetStat(0);
    SetCollDetDyn(0);
}

zCEventMessage::zCEventMessage(unsigned short eventSubType)
    : subType(eventSubType), inCutscene(0)
{
}

unsigned long zCEventMessage::GetMessageID() const
{
    return classDef->GetClassID() + (subType << 16);
}

void zCEventMessage::AnalyzeMessageID(unsigned long id, unsigned short& classID,
    unsigned short& eventSubType)
{
    classID = static_cast<unsigned short>(id);
    eventSubType = static_cast<unsigned short>(id >> 16);
}

zCEventMessage* zCEventMessage::CreateFromID(unsigned long id)
{
    zCEventMessage* message = static_cast<zCEventMessage*>(zCClassDef::CreateNewInstanceID(id));
    message->subType = static_cast<unsigned short>(id >> 16);
    return message;
}

void zCEventMessage::PackToBuffer(zCBuffer& buffer, zCEventManager* manager)
{
    const unsigned long id = GetMessageID();
    buffer.Write(&id, sizeof(id));
    Pack(buffer, manager);
}

void zCEventMessage::Pack(zCBuffer&, zCEventManager*)
{
}

void zCEventMessage::Unpack(zCBuffer&, zCEventManager*)
{
}

zCEventMessage* zCEventMessage::CreateFromBuffer(zCBuffer& buffer, zCEventManager* manager)
{
    unsigned long id;
    buffer.Read(&id, sizeof(id));
    zCEventMessage* message = CreateFromID(id);
    message->Unpack(buffer, manager);
    return message;
}

void zCEventMessage::Archive(zCArchiver& archiver)
{
    if (!archiver.InProperties()) {
        archiver.WriteEnum(subType);
        return;
    }
    zSTRING subTypeNames;
    for (int i = 0; i < MD_GetNumOfSubTypes(); ++i)
        subTypeNames += MD_GetSubTypeString(i) + ";";
    archiver.WriteEnum("", subTypeNames, subType);
}

void zCEventMessage::Unarchive(zCArchiver& archiver)
{
    subType = static_cast<unsigned short>(archiver.ReadEnum());
}

void zCEventMessage::TestClassIntegrity()
{
    for (int i = 0; i < zCClassDef::classDefList->numInArray; ++i) {
        zCClassDef* objectClass = zCClassDef::classDefList->array[i];
        if (!objectClass->createNewInstance)
            continue;
        zCEventMessage* message = static_cast<zCEventMessage*>(objectClass->createNewInstance());
        if (!message)
            continue;
        zCBuffer buffer;
        buffer.SetMode(WRITE);
        message->PackToBuffer(buffer, 0);
        buffer.SetMode(READ);
        zCEventMessage* loaded = CreateFromBuffer(buffer, 0);
        loaded->Release();
        message->Release();
    }
}

void zCEventCore::Clear()
{
    otherVob = 0;
    vobInstigator = 0;
    inflictorVob = 0;
    damage = 0;
    damageType = 0;
    hitLocation.n[0] = 0;
    hitLocation.n[1] = 0;
    hitLocation.n[2] = 0;
}

zCEventCore::~zCEventCore()
{
    otherVob = 0;
    vobInstigator = 0;
    inflictorVob = 0;
}

void zCEventCore::Pack(zCBuffer& buffer, zCEventManager*)
{
    buffer.Write(&otherVob, sizeof(otherVob));
    if (subType <= 1)
        buffer.Write(&vobInstigator, sizeof(vobInstigator));
    else if (subType == 5) {
        buffer.Write(&inflictorVob, sizeof(inflictorVob));
        buffer.WriteFloat(damage, 32);
        buffer.WriteByte(damageType, 8);
        buffer.Write(&hitLocation, sizeof(hitLocation));
    }
}

void zCEventCore::Unpack(zCBuffer& buffer, zCEventManager*)
{
    Clear();
    buffer.Read(&otherVob, sizeof(otherVob));
    if (subType <= 1)
        buffer.Read(&vobInstigator, sizeof(vobInstigator));
    else if (subType == 5) {
        buffer.Read(&inflictorVob, sizeof(inflictorVob));
        damage = buffer.ReadFloat(32);
        damageType = buffer.ReadByte(8);
        buffer.Read(&hitLocation, sizeof(hitLocation));
    }
}

zSTRING zCEventCore::MD_GetSubTypeString(int eventSubType)
{
    static const char* names[] = {
        "TRIGGER", "UNTRIGGER", "TOUCH", "UNTOUCH", "TOUCHLEVEL", "DAMAGE"
    };
    if (eventSubType < 0 || eventSubType > 5)
        return zSTRING();
    return zSTRING(names[eventSubType]);
}

void zCEventCore::Archive(zCArchiver& archiver)
{
    zCEventMessage::Archive(archiver);
    archiver.WriteFloat(damage);
    archiver.WriteByte(damageType);
    if (!archiver.InProperties())
        archiver.WriteVec3(hitLocation);
}

void zCEventCore::Unarchive(zCArchiver& archiver)
{
    subType = static_cast<unsigned short>(archiver.ReadEnum());
    Clear();
    damage = archiver.ReadFloat();
    damageType = archiver.ReadByte();
    if (!archiver.InProperties())
        hitLocation = archiver.ReadVec3();
}

int zCVob::GetIsProjectile() const
{
    return callback_ai ? callback_ai->GetIsProjectile() : 0;
}

void zCTree::RemoveSubtree()
{
    if (parent && parent->firstChild == this)
        parent->firstChild = next;
    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;
    prev = 0;
    next = 0;
}

zCTree* zCTree::AddChild(zCTree* child)
{
    child->parent = this;
    child->prev = 0;
    child->next = firstChild;
    firstChild = child;
    if (child->next)
        child->next->prev = child;
    return child;
}


static int vobLightState = -1;
static int triggerState = -1;
static zCClassDef zCVobLightClassDef("zCVobLight", "zCVob", zCVobLight::_CreateNewInstance,
    0, 0x188, 5, "");
zCArray<zCVobLightPreset*> zCVobLight::lightPresetList;
int zCVobLight::renderLightVisuals;
zCClassDef* zCVobLight::classDef = &zCVobLightClassDef;

// EXE: 0x00607C60
zCObject* zCVobLight::_CreateNewInstance()
{
    zCVobLight* light = new zCVobLight;
    zCClassDef::ObjectCreated(light, classDef);
    return light;
}

zCVobLight::zCVobLight()
{
    affectedPolyMap.InitHashTable(97);
    showVisual = 1;
    castDynShadow = 1;
    SetCollDetStat(0);
    SetCollDetDyn(0);
}

zCClassDef* zCVobLight::_GetClassDef() const
{
    return classDef;
}

zCVobLight::~zCVobLight()
{
    for (int i = 0; i < affectedPolyMap.hashTable.numInArray; ++i) {
        zTRayTurboValMap<zCPolygon*, int>::zSNode* node = affectedPolyMap.hashTable.parray[i];
        while (node) {
            zTRayTurboValMap<zCPolygon*, int>::zSNode* next = node->next;
            delete node;
            node = next;
        }
    }
    affectedPolyMap.hashTable.DeleteList();
}

void zCVobLight::CleanupVobLight()
{
    for (int i = 0; i < lightPresetList.numInArray; ++i)
        lightPresetList.parray[i]->Release();
    lightPresetList.DeleteList();
}

void zCVobLight::OnTrigger(zCVob*, zCVob*)
{
    if (!lightData.isStatic && !lightData.isTurnedOn) {
        lightData.range = lightData.rangeBackup > 0.0f ? lightData.rangeBackup : 0.0f;
        lightData.rangeInv = lightData.range != 0.0f ? 1.0f / lightData.range : 3.40282347e+38f;
        zTBBox3D box;
        for (int i = 0; i < 3; ++i) {
            box.mins.n[i] = -lightData.range;
            box.maxs.n[i] = lightData.range;
        }
        SetBBox3DLocal(box);
        lightData.isTurnedOn = 1;
    }
}

void zCVobLight::OnUntrigger(zCVob*, zCVob*)
{
    if (!lightData.isStatic && lightData.isTurnedOn) {
        lightData.range = 0.0f;
        lightData.rangeInv = 3.40282347e+38f;
        zTBBox3D box;
        for (int i = 0; i < 3; ++i) {
            box.mins.n[i] = 0.0f;
            box.maxs.n[i] = 0.0f;
        }
        SetBBox3DLocal(box);
        lightData.isTurnedOn = 0;
    }
}


void zCVobLight::DoAnimation()
{
    if (lightData.colorAniFPS == 0.0f || lightData.colorAniList.numInArray <= 0)
        return;

    const int frame = static_cast<int>(std::floor(lightData.colorAniActFrame));
    zCOLOR color = lightData.colorAniList.parray[frame];
    if (lightData.colorAniSmooth) {
        const int blend = static_cast<int>((lightData.colorAniActFrame - frame) * 255.0f);
        int nextFrame = frame + 1;
        if (nextFrame >= lightData.colorAniList.numInArray)
            nextFrame = 0;
        const zCOLOR& next = lightData.colorAniList.parray[nextFrame];
        color.r += ((next.r - color.r) * blend) >> 8;
        color.g += ((next.g - color.g) * blend) >> 8;
        color.b += ((next.b - color.b) * blend) >> 8;
    }
    lightData.lightColor = color;
    lightData.colorAniActFrame += ztimer.frameTimeFloat * lightData.colorAniFPS;
    while (std::floor(lightData.colorAniActFrame) >= lightData.colorAniList.numInArray)
        lightData.colorAniActFrame -= lightData.colorAniList.numInArray;
}

void zCVobLight::SetRange(float newRange, int resetBackup)
{
    lightData.range = newRange > 0.0f ? newRange : 0.0f;
    lightData.rangeInv = lightData.range != 0.0f ? 1.0f / lightData.range : 3.40282347e+38f;
    if (resetBackup)
        lightData.rangeBackup = lightData.range;
    zTBBox3D box;
    for (int i = 0; i < 3; ++i) {
        box.mins.n[i] = -lightData.range;
        box.maxs.n[i] = lightData.range;
    }
    SetBBox3DLocal(box);
    if (resetBackup && homeWorld)
        homeWorld->bspTree.AdjustVob(this);
}

zCOLOR zCVobLight::SumLightsAtPositionWS(const zCArray<zCVobLight*>& lights,
    const zVEC3& position, int traceRay)
{
    int red = 0;
    int green = 0;
    int blue = 0;
    for (int i = 0; i < lights.numInArray; ++i) {
        zCVobLight* light = lights.parray[i];
        zVEC3 delta;
        for (int axis = 0; axis < 3; ++axis)
            delta.n[axis] = position.n[axis] - light->trafoObjToWorld.m[axis][3];
        float x = std::fabs(delta.n[0]);
        float y = std::fabs(delta.n[1]);
        float z = std::fabs(delta.n[2]);
        if (x < y) {
            const float swap = x;
            x = y;
            y = swap;
        }
        if (x < z) {
            const float swap = x;
            x = z;
            z = swap;
        }
        const float distance = x - x * 0.0625f + (y + z) * 0.375f;
        if (distance < light->lightData.range) {
            const int intensity = static_cast<int>((1.0f - distance * light->lightData.rangeInv) * 255.0f);
            red += (light->lightData.lightColor.r * intensity) >> 8;
            green += (light->lightData.lightColor.g * intensity) >> 8;
            blue += (light->lightData.lightColor.b * intensity) >> 8;
        }
    }
    zCOLOR result;
    result.r = red > 255 ? 255 : red;
    result.g = green > 255 ? 255 : green;
    result.b = blue > 255 ? 255 : blue;
    result.a = 255;
    return result;
}


void zCVobLight::LoadLightPresets()
{
    zoptions->ChangeDir(zTOptionPaths_DIR_PRESETS);
    zSTRING fileName("lightPresets.zen");
    zCArchiver* archiver = zarcFactory->CreateArchiverRead(fileName, 0);
    if (!archiver)
        return;
    const int count = archiver->ReadWord();
    lightPresetList.DeleteList();
    lightPresetList.AllocAbs(count);
    for (int i = 0; i < count; ++i) {
        zCObject* object = 0;
        archiver->ReadObject(object);
        lightPresetList.parray[i] = static_cast<zCVobLightPreset*>(object);
    }
    archiver->Close();
    archiver->Release();
}

void zCVobLight::SaveLightPresets()
{
    zoptions->ChangeDir(zTOptionPaths_DIR_PRESETS);
    zSTRING fileName("lightPresets.zen");
    zCArchiver* archiver = zarcFactory->CreateArchiverWrite(fileName, 0, 0, 0);
    if (!archiver)
        return;
    archiver->WriteWord(lightPresetList.numInArray);
    for (int i = 0; i < lightPresetList.numInArray; ++i)
        archiver->WriteObject(lightPresetList.parray[i]);
    archiver->Close();
    archiver->Release();
}

void zCVobLight::AddThisToPresetList(const zSTRING& presetName) const
{
    if (presetName.Length() == 0)
        return;
    zCVobLightPreset* preset = new zCVobLightPreset;
    preset->presetName = presetName;
    preset->presetName.Upper();
    preset->lightData = lightData;
    preset->lightData.rangeAniScaleList = lightData.rangeAniScaleList;
    preset->lightData.colorAniList = lightData.colorAniList;
    for (int i = 0; i < lightPresetList.numInArray; ++i) {
        if (lightPresetList.parray[i]->presetName == preset->presetName) {
            lightPresetList.parray[i]->Release();
            lightPresetList.parray[i] = preset;
            return;
        }
    }
    lightPresetList.InsertEnd(preset);
}

int zCVobLight::SetByPreset(const zSTRING& presetName)
{
    if (presetName.Length() == 0)
        return 0;
    for (int i = 0; i < lightPresetList.numInArray; ++i) {
        zCVobLightPreset* preset = lightPresetList.parray[i];
        if (preset->presetName == presetName) {
            lightData = preset->lightData;
            lightData.rangeAniScaleList = preset->lightData.rangeAniScaleList;
            lightData.colorAniList = preset->lightData.colorAniList;
            lightPresetInUse = preset->presetName;
            return 1;
        }
    }
    return 0;
}

int zCVobLight::SetByPresetInUse()
{
    return SetByPreset(lightPresetInUse);
}

void zCVobLight::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteGroupBegin("VobLight");
    archiver.WriteString(lightPresetInUse);
    lightData.Archive(archiver);
    archiver.WriteGroupEnd();
}

void zCVobLight::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    lightPresetInUse = archiver.ReadString();
    lightData.Unarchive(archiver);
}

void zCVobLightPreset::Archive(zCArchiver& archiver)
{
    archiver.WriteString(presetName);
    lightData.Archive(archiver);
}

void zCVobLightPreset::Unarchive(zCArchiver& archiver)
{
    presetName = archiver.ReadString();
    lightData.Unarchive(archiver);
}


extern zSTRING zSTR_EMPTY;

int zCVobLightData::SetLensFlareFXByName(const zSTRING& name)
{
    if (name.Length() == 0)
        return 1;
    zSTRING upperName(name);
    upperName.Upper();
    lensFlareFX = static_cast<zCLensFlareFX*>(zCLensFlareFX::classDef.SearchHashTable(upperName));
    lensFlareFXNo = -1;
    for (int i = 0; i < zCLensFlareFX::classDef.objectList.numInArray; ++i) {
        if (zCLensFlareFX::classDef.objectList.parray[i] == lensFlareFX) {
            lensFlareFXNo = i;
            break;
        }
    }
    return reinterpret_cast<int>(lensFlareFX);
}

const zSTRING& zCVobLightData::GetLensFlareFXName() const
{
    return lensFlareFX ? lensFlareFX->GetObjectName() : zSTR_EMPTY;
}

zCVobLightData::zCVobLightData()
    : lensFlareFXNo(-1), lensFlareFX(0), range(0.0f), rangeInv(3.40282347e+38f),
      rangeBackup(0.0f), rangeAniActFrame(0.0f), rangeAniFPS(0.0f),
      colorAniActFrame(0.0f), colorAniFPS(0.0f), spotConeAngleDeg(0.0f)
{
    lightColor.r = 255;
    lightColor.g = 255;
    lightColor.b = 255;
    lightColor.a = 255;
    isStatic = 1;
    rangeAniSmooth = 1;
    rangeAniLoop = 0;
    colorAniSmooth = 1;
    colorAniLoop = 0;
    isTurnedOn = 1;
    lightQuality = 0;
    lightType = 0;
    m_bCanMove = 0;
}

zCVobLightData::~zCVobLightData()
{
    lensFlareFX = 0;
}

void zCVobLightData::SetRange(float newRange, int resetBackup)
{
    range = newRange > 0.0f ? newRange : 0.0f;
    rangeInv = range != 0.0f ? 1.0f / range : 3.40282347e+38f;
    if (resetBackup)
        rangeBackup = range;
}

void zCVobLightData::Archive(zCArchiver& archiver)
{
    archiver.WriteEnum(lightType);
    archiver.WriteFloat(rangeBackup);
    archiver.WriteColor(lightColor);
    archiver.WriteFloat(spotConeAngleDeg);
    archiver.WriteBool(isStatic);
    archiver.WriteEnum(lightQuality);
    archiver.WriteString(GetLensFlareFXName());
    if (archiver.InProperties() || !isStatic) {
        archiver.WriteGroupBegin("LightAnimation");
        archiver.WriteBool(isTurnedOn);
        zSTRING rangeList;
        for (int i = 0; i < rangeAniScaleList.numInArray; ++i) {
            zSTRING value;
            value.Sprintf("%f", rangeAniScaleList.parray[i]);
            rangeList.Append(value).Append(" ");
        }
        archiver.WriteString(rangeList);
        archiver.WriteFloat(rangeAniFPS * 1000.0f);
        archiver.WriteBool(rangeAniSmooth);
        zSTRING colorList;
        for (int i = 0; i < colorAniList.numInArray; ++i) {
            const zCOLOR& color = colorAniList.parray[i];
            zSTRING value;
            if (color.r == color.g && color.r == color.b)
                value.Sprintf("%d", color.r);
            else
                value = color.GetDescriptionRGB();
            colorList.Append(value).Append(" ");
        }
        archiver.WriteString(colorList);
        archiver.WriteFloat(colorAniFPS * 1000.0f);
        archiver.WriteBool(colorAniSmooth);
        archiver.WriteBool(m_bCanMove);
        archiver.WriteGroupEnd();
    }
}

void zCVobLightData::Unarchive(zCArchiver& archiver)
{
    lightType = archiver.ReadEnum();
    SetRange(archiver.ReadFloat(), 1);
    lightColor = archiver.ReadColor();
    spotConeAngleDeg = archiver.ReadFloat();
    isStatic = archiver.ReadBool();
    lightQuality = archiver.ReadEnum();
    SetLensFlareFXByName(archiver.ReadString());
    if (archiver.InProperties() || !isStatic) {
        isTurnedOn = archiver.ReadBool();
        const zSTRING rangeList = archiver.ReadString();
        rangeAniScaleList.numInArray = 0;
        for (int i = 1; ; ++i) {
            const zSTRING value = rangeList.PickWord(i, " ", "");
            if (value.Length() == 0)
                break;
            rangeAniScaleList.InsertEnd(value.ToFloat());
        }
        rangeAniActFrame = 0.0f;
        rangeAniFPS = archiver.ReadFloat() * 0.001f;
        rangeAniSmooth = archiver.ReadBool();
        const zSTRING colorList = archiver.ReadString();
        colorAniList.numInArray = 0;
        for (int i = 1; ; ++i) {
            const zSTRING value = colorList.PickWord(i, " ", "");
            if (value.Length() == 0)
                break;
            zCOLOR color;
            if (value.Search("(") != -1) {
                color.SetByDescription(value);
            }
            else {
                color.r = value.ToInt();
                color.g = color.r;
                color.b = color.r;
                color.a = 255;
            }
            colorAniList.InsertEnd(color);
        }
        colorAniActFrame = 0.0f;
        colorAniFPS = archiver.ReadFloat() * 0.001f;
        colorAniSmooth = archiver.ReadBool();
        m_bCanMove = archiver.ReadBool();
    }
}


void zCVobLight::EndMovement(int flags)
{
    if (!lightData.isStatic && flags && (GetCollisionObject()->m_bHintTranslation & 3))
        affectedPolyMap.InitHashTable(97);
    zCVob::EndMovement(flags);
}

void zCVobLight::ThisVobAddedToWorld(zCWorld* world)
{
    if (!lightData.isStatic)
        affectedPolyMap.InitHashTable(97);
    zCVob::ThisVobAddedToWorld(world);
}


static zCClassDef zCVobStairClassDef("zCVobStair", "zCVob", zCVobStair::_CreateNewInstance,
    0, 0x120, 0, "");
zCClassDef* zCVobStair::classDef = &zCVobStairClassDef;

// EXE: 0x0060B350
zCObject* zCVobStair::_CreateNewInstance()
{
    zCVobStair* vob = new zCVobStair;
    zCClassDef::ObjectCreated(vob, classDef);
    return vob;
}

void zCVobStair::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCVobStair::_GetClassDef() const
{
    return classDef;
}

zCVobStair::~zCVobStair()
{
}


static zCClassDef zCTriggerBaseClassDef("zCTriggerBase", "zCVob",
    zCTriggerBase::_CreateNewInstance, 0, 0x134, 0, "");
zCClassDef* zCTriggerBase::classDef = &zCTriggerBaseClassDef;
static zCClassDef zCTriggerUntouchClassDef("zCTriggerUntouch", "zCTriggerBase",
    zCTriggerUntouch::_CreateNewInstance, 0, 0x134, 1, "");
zCClassDef* zCTriggerUntouch::classDef = &zCTriggerUntouchClassDef;
static zCClassDef zCMessageFilterClassDef("zCMessageFilter", "zCTriggerBase",
    zCMessageFilter::_CreateNewInstance, 0, 0x138, 0, "");
zCClassDef* zCMessageFilter::classDef = &zCMessageFilterClassDef;
static zCClassDef zCCodeMasterClassDef("zCCodeMaster", "zCTriggerBase",
    zCCodeMaster::_CreateNewInstance, 0, 0x1E0, 0, "");
zCClassDef* zCCodeMaster::classDef = &zCCodeMasterClassDef;

zCTriggerBase::zCTriggerBase()
{
    SetCollisionClass(&zCCollObjectBoxPassThrough::s_oCollObjClass);
    SetCollDetStat(0);
    SetCollDetDyn(1);
    castDynShadow = (castDynShadow & 0xde) ^ 0x21;
}

zCTriggerBase::~zCTriggerBase()
{
}

zCClassDef* zCTriggerBase::_GetClassDef() const
{
    return classDef;
}

const zSTRING* zCTriggerBase::GetTriggerTarget(int index) const
{
    return index == 0 ? &triggerTarget : NULL;
}

zSTRING zCTrigger::GetTriggerTarget() const
{
    return triggerTarget;
}

// EXE: 0x0060B7F0
zCObject* zCTriggerUntouch::_CreateNewInstance()
{
    zCTriggerUntouch* trigger = new zCTriggerUntouch;
    zCClassDef::ObjectCreated(trigger, classDef);
    return trigger;
}

void zCTriggerUntouch::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCTriggerUntouch::_GetClassDef() const
{
    return classDef;
}

void zCTriggerUntouch::OnTouch(zCVob*)
{
}

void zCTriggerUntouch::OnTrigger(zCVob*, zCVob*)
{
}

void zCTriggerUntouch::OnUntrigger(zCVob*, zCVob*)
{
}

zCTriggerUntouch::~zCTriggerUntouch()
{
}

zCMessageFilter::zCMessageFilter()
    : sendWhenTriggered(1), sendWhenUntriggered(2)
{
}

// EXE: 0x0060BBC0
zCObject* zCMessageFilter::_CreateNewInstance()
{
    zCMessageFilter* filter = new zCMessageFilter;
    zCClassDef::ObjectCreated(filter, classDef);
    return filter;
}

void zCMessageFilter::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCMessageFilter::_GetClassDef() const
{
    return classDef;
}

void zCMessageFilter::OnTouch(zCVob*)
{
}

void zCMessageFilter::OnUntouch(zCVob*)
{
}

zCCodeMaster::zCCodeMaster()
    : orderRelevant(0), untriggerCancels(0), firstFalseIsFailure(0), numSlavesTriggered(0)
{
    for (int i = 0; i < 6; ++i) {
        slaveVobNameList[i].Clear();
        slaveTriggeredList[i] = 0;
    }
    triggerTargetFailure.Clear();
}

// EXE: 0x0060BEE0
zCObject* zCCodeMaster::_CreateNewInstance()
{
    zCCodeMaster* master = new zCCodeMaster;
    zCClassDef::ObjectCreated(master, classDef);
    return master;
}

void zCCodeMaster::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCCodeMaster::_GetClassDef() const
{
    return classDef;
}

void zCCodeMaster::OnUntouch(zCVob*)
{
}

void zCCodeMaster::OnTouch(zCVob*)
{
}


static zCClassDef zCMoverControlerClassDef("zCMoverControler", "zCTriggerBase",
    zCMoverControler::_CreateNewInstance, 0, 0x13C, 0, "");
zCClassDef* zCMoverControler::classDef = &zCMoverControlerClassDef;
static zCClassDef zCTriggerWorldStartClassDef("zCTriggerWorldStart", "zCTriggerBase",
    zCTriggerWorldStart::_CreateNewInstance, 0, 0x138, 0, "");
zCClassDef* zCTriggerWorldStart::classDef = &zCTriggerWorldStartClassDef;
static zCClassDef zCTriggerClassDef("zCTrigger", "zCTriggerBase", zCTrigger::_CreateNewInstance,
    0, 0x168, 2, "");
zCClassDef* zCTrigger::classDef = &zCTriggerClassDef;
static zCClassDef zCTriggerListClassDef("zCTriggerList", "zCTrigger",
    zCTriggerList::_CreateNewInstance, 0, 0x204, 0, "");
zCClassDef* zCTriggerList::classDef = &zCTriggerListClassDef;

// EXE: 0x0060C360
zCObject* zCMoverControler::_CreateNewInstance()
{
    zCMoverControler* controler = new zCMoverControler;
    zCClassDef::ObjectCreated(controler, classDef);
    return controler;
}

void zCMoverControler::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCMoverControler::_GetClassDef() const
{
    return classDef;
}

void zCMoverControler::OnUntrigger(zCVob*, zCVob*)
{
}

void zCMoverControler::OnUntouch(zCVob*)
{
}

void zCMoverControler::OnTouch(zCVob*)
{
}

// EXE: 0x0060C6A0
zCObject* zCTriggerWorldStart::_CreateNewInstance()
{
    zCTriggerWorldStart* trigger = new zCTriggerWorldStart;
    zCClassDef::ObjectCreated(trigger, classDef);
    return trigger;
}

void zCTriggerWorldStart::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCTriggerWorldStart::_GetClassDef() const
{
    return classDef;
}

void zCTriggerWorldStart::OnTouch(zCVob*)
{
}

void zCTriggerWorldStart::OnUntouch(zCVob*)
{
}

void zCTriggerWorldStart::OnUntrigger(zCVob*, zCVob*)
{
}

// EXE: 0x0060C9E0
zCObject* zCTrigger::_CreateNewInstance()
{
    zCTrigger* trigger = new zCTrigger;
    zCClassDef::ObjectCreated(trigger, classDef);
    return trigger;
}

void zCTrigger::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCTriggerList::zCTriggerList()
    : listProcess(LP_ALL), actTarget(0), sendOnTrigger(1)
{
    for (int i = 0; i < 6; ++i) {
        triggerTargetList[i].Clear();
        fireDelayList[i] = 0.0f;
    }
}

// EXE: 0x0060CC50
zCObject* zCTriggerList::_CreateNewInstance()
{
    zCTriggerList* trigger = new zCTriggerList;
    zCClassDef::ObjectCreated(trigger, classDef);
    return trigger;
}

void zCTriggerList::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCTriggerList::_GetClassDef() const
{
    return classDef;
}


static zCClassDef zCMoverClassDef("zCMover", "zCTrigger", zCMover::_CreateNewInstance,
    0, 0x270, 1, "");
zCClassDef* zCMover::classDef = &zCMoverClassDef;
static zCClassDef zCTriggerTeleportClassDef("zCTriggerTeleport", "zCTrigger",
    zCTriggerTeleport::_CreateNewInstance, 0, 0x180, 0, "");
zCClassDef* zCTriggerTeleport::classDef = &zCTriggerTeleportClassDef;
static zCClassDef zCEffectClassDef("zCEffect", "zCVob", NULL, 0, 0x120, 0, "");
zCClassDef* zCEffect::classDef = &zCEffectClassDef;
static zCClassDef zCEarthquakeClassDef("zCEarthquake", "zCEffect", zCEarthquake::_CreateNewInstance,
    0, 0x134, 0, "");
zCClassDef* zCEarthquake::classDef = &zCEarthquakeClassDef;
static zCClassDef zCPFXControlerClassDef("zCPFXControler", "zCEffect",
    zCPFXControler::_CreateNewInstance, 0, 0x138, 0, "");
zCClassDef* zCPFXControler::classDef = &zCPFXControlerClassDef;
static zCClassDef zCTouchDamageClassDef("zCTouchDamage", "zCEffect", zCTouchDamage::_CreateNewInstance,
    0, 0x134, 1, "");
zCClassDef* zCTouchDamage::classDef = &zCTouchDamageClassDef;
static zCClassDef zCTouchAnimateClassDef("zCTouchAnimate", "zCEffect", zCTouchAnimate::_CreateNewInstance,
    0, 0x138, 0, "");
zCClassDef* zCTouchAnimate::classDef = &zCTouchAnimateClassDef;

// EXE: 0x0060D080
zCObject* zCMover::_CreateNewInstance()
{
    zCMover* mover = new zCMover;
    zCClassDef::ObjectCreated(mover, classDef);
    return mover;
}

void zCMover::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCTriggerTeleport::zCTriggerTeleport()
    : teleportSound(0)
{
}

// EXE: 0x0060D2F0
zCObject* zCTriggerTeleport::_CreateNewInstance()
{
    zCTriggerTeleport* trigger = new zCTriggerTeleport;
    zCClassDef::ObjectCreated(trigger, classDef);
    return trigger;
}

void zCTriggerTeleport::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCTriggerTeleport::_GetClassDef() const
{
    return classDef;
}

zCEarthquake::zCEarthquake()
    : radiusSquare(40000.0f), timeMSec(5000.0f)
{
    amplitude.n[0] = 2.0f;
    amplitude.n[1] = 30.0f;
    amplitude.n[2] = 2.0f;
    SetCollDetStat(0);
    SetCollDetDyn(0);
    castDynShadow = (castDynShadow & 0xce) ^ 0x30;
}

// EXE: 0x0060D850
zCObject* zCEarthquake::_CreateNewInstance()
{
    zCEarthquake* earthquake = new zCEarthquake;
    zCClassDef::ObjectCreated(earthquake, classDef);
    return earthquake;
}

zCClassDef* zCEarthquake::_GetClassDef() const
{
    return classDef;
}

// EXE: 0x0060DB20
zCObject* zCPFXControler::_CreateNewInstance()
{
    zCPFXControler* controler = new zCPFXControler;
    zCClassDef::ObjectCreated(controler, classDef);
    return controler;
}

void zCPFXControler::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCPFXControler::_GetClassDef() const
{
    return classDef;
}

zCTouchDamage::zCTouchDamage()
    : damage(0.0f), damageType(1), damageRepeatDelaySec(0.0f),
      damageVolDownScale(1.0f), damageCollType(CT_BOX)
{
    SetCollisionClass(&zCCollObjectBoxPassThrough::s_oCollObjClass);
    SetCollDetStat(0);
    SetCollDetDyn(1);
    castDynShadow = (castDynShadow & 0xde) ^ 0x21;
}

// EXE: 0x0060DDF0
zCObject* zCTouchDamage::_CreateNewInstance()
{
    zCTouchDamage* touch = new zCTouchDamage;
    zCClassDef::ObjectCreated(touch, classDef);
    return touch;
}

void zCTouchDamage::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCTouchDamage::_GetClassDef() const
{
    return classDef;
}

zCTouchAnimate::zCTouchAnimate()
    : touchSound(0)
{
    SetCollisionClass(&zCCollObjectBoxPassThrough::s_oCollObjClass);
    SetCollDetStat(0);
    SetCollDetDyn(1);
    castDynShadow = (castDynShadow & 0xde) ^ 0x21;
}

// EXE: 0x0060E0F0
zCObject* zCTouchAnimate::_CreateNewInstance()
{
    zCTouchAnimate* touch = new zCTouchAnimate;
    zCClassDef::ObjectCreated(touch, classDef);
    return touch;
}

void zCTouchAnimate::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCTouchAnimate::_GetClassDef() const
{
    return classDef;
}


static zCClassDef zCTouchAnimateSoundClassDef("zCTouchAnimateSound", "zCTouchAnimate",
    zCTouchAnimateSound::_CreateNewInstance, 0, 0x14C, 0, "");
zCClassDef* zCTouchAnimateSound::classDef = &zCTouchAnimateSoundClassDef;
static zCClassDef zCVobAnimateClassDef("zCVobAnimate", "zCEffect", zCVobAnimate::_CreateNewInstance,
    0, 0x124, 0, "");
zCClassDef* zCVobAnimate::classDef = &zCVobAnimateClassDef;
static zCClassDef zCVobLensFlareClassDef("zCVobLensFlare", "zCEffect",
    zCVobLensFlare::_CreateNewInstance, 0, 0x124, 1, "");
zCClassDef* zCVobLensFlare::classDef = &zCVobLensFlareClassDef;
static zCClassDef zCVobScreenFXClassDef("zCVobScreenFX", "zCEffect",
    zCVobScreenFX::_CreateNewInstance, 0, 0x1C0, 1, "");
zCClassDef* zCVobScreenFX::classDef = &zCVobScreenFXClassDef;

// EXE: 0x0060E3F0
zCObject* zCTouchAnimateSound::_CreateNewInstance()
{
    zCTouchAnimateSound* touch = new zCTouchAnimateSound;
    zCClassDef::ObjectCreated(touch, classDef);
    return touch;
}

void zCTouchAnimateSound::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCTouchAnimateSound::_GetClassDef() const
{
    return classDef;
}

zCVobAnimate::zCVobAnimate()
    : startOn(1), isRunning(0)
{
    SetCollDetStat(0);
    SetCollDetDyn(0);
    castDynShadow = (castDynShadow & 0xdf) ^ 0x20;
}

// EXE: 0x0060E7A0
zCObject* zCVobAnimate::_CreateNewInstance()
{
    zCVobAnimate* vob = new zCVobAnimate;
    zCClassDef::ObjectCreated(vob, classDef);
    return vob;
}

void zCVobAnimate::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCVobAnimate::_GetClassDef() const
{
    return classDef;
}

zCVobLensFlare::zCVobLensFlare()
    : lensFlareFX(0)
{
    SetCollDetStat(0);
    SetCollDetDyn(0);
    castDynShadow = (castDynShadow & 0xdf) ^ 0x20;
}

// EXE: 0x0060EA60
zCObject* zCVobLensFlare::_CreateNewInstance()
{
    zCVobLensFlare* vob = new zCVobLensFlare;
    zCClassDef::ObjectCreated(vob, classDef);
    return vob;
}

void zCVobLensFlare::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCVobLensFlare::_GetClassDef() const
{
    return classDef;
}

// EXE: 0x0060ED30
zCObject* zCVobScreenFX::_CreateNewInstance()
{
    zCVobScreenFX* vob = new zCVobScreenFX;
    zCClassDef::ObjectCreated(vob, classDef);
    return vob;
}


static zCClassDef zCEventCommonClassDef("zCEventCommon", "zCEventMessage",
    zCEventCommon::_CreateNewInstance, 0, 0x2C, 0, "");
zCClassDef* zCEventCommon::classDef = &zCEventCommonClassDef;
static zCClassDef zCEventMoverClassDef("zCEventMover", "zCEventMessage",
    zCEventMover::_CreateNewInstance, 0, 0x30, 0, "");
zCClassDef* zCEventMover::classDef = &zCEventMoverClassDef;
static zCClassDef zCEventScreenFXClassDef("zCEventScreenFX", "zCEventMessage",
    zCEventScreenFX::_CreateNewInstance, 0, 0x38, 0, "");
zCClassDef* zCEventScreenFX::classDef = &zCEventScreenFXClassDef;

// EXE: 0x0060EF70
zCObject* zCEventCommon::_CreateNewInstance()
{
    zCEventCommon* message = new zCEventCommon;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

void zCEventCommon::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCEventCommon::_GetClassDef() const
{
    return classDef;
}

int zCEventCommon::MD_GetNumOfSubTypes()
{
    return 4;
}

int zCEventCommon::IsNetRelevant()
{
    return 1;
}

zCEventCommon::~zCEventCommon()
{
}

zSTRING zCEventCommon::MD_GetSubTypeString(int eventSubType)
{
    if (eventSubType == zEVENT_ENABLE)
        return zSTRING("ENABLE");
    if (eventSubType == zEVENT_DISABLE)
        return zSTRING("DISBLE");
    if (eventSubType == zEVENT_TOGGLE_ENABLED)
        return zSTRING("TOGGLE_ENABLED");
    return zSTRING();
}

// EXE: 0x0060F310
zCObject* zCEventMover::_CreateNewInstance()
{
    zCEventMover* message = new zCEventMover;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

void zCEventMover::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCEventMover::_GetClassDef() const
{
    return classDef;
}

int zCEventMover::MD_GetNumOfSubTypes()
{
    return 4;
}

int zCEventMover::IsNetRelevant()
{
    return 1;
}

zCEventMover::~zCEventMover()
{
}

zSTRING zCEventMover::MD_GetSubTypeString(int eventSubType)
{
    if (eventSubType == zEVENT_GOTO_KEY_FIXED_DIRECTLY)
        return zSTRING("GOTO_KEY_FIXED_DIRECTLY");
    if (eventSubType == zEVENT_GOTO_KEY_FIXED_ORDER)
        return zSTRING("_DISABLED_");
    if (eventSubType == zEVENT_GOTO_KEY_NEXT)
        return zSTRING("GOTO_KEY_NEXT");
    if (eventSubType == zEVENT_GOTO_KEY_PREV)
        return zSTRING("GOTO_KEY_PREV");
    return zSTRING();
}

zCEventScreenFX::zCEventScreenFX()
    : duration(2.0f), fovDeg(0.0f)
{
    color.r = 0;
    color.g = 0;
    color.b = 0;
    color.a = 255;
}

// EXE: 0x0060F6B0
zCObject* zCEventScreenFX::_CreateNewInstance()
{
    zCEventScreenFX* message = new zCEventScreenFX;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

zCClassDef* zCEventScreenFX::_GetClassDef() const
{
    return classDef;
}

int zCEventScreenFX::MD_GetNumOfSubTypes()
{
    return 5;
}

zCEventScreenFX::~zCEventScreenFX()
{
}


void zCTriggerBase::OnTouch(zCVob* other)
{
    zCArray<zCVob*> targets;
    homeWorld->SearchVobListByName(triggerTarget, targets);
    for (int i = 0; i < targets.numInArray; ++i)
        targets.parray[i]->GetEM(0)->OnTrigger(other, other);
}

void zCTriggerBase::OnUntouch(zCVob* other)
{
    zCArray<zCVob*> targets;
    homeWorld->SearchVobListByName(triggerTarget, targets);
    for (int i = 0; i < targets.numInArray; ++i)
        targets.parray[i]->GetEM(0)->OnUntrigger(other, other);
}

void zCTriggerBase::OnTrigger(zCVob*, zCVob* instigator)
{
    zCArray<zCVob*> targets;
    homeWorld->SearchVobListByName(triggerTarget, targets);
    for (int i = 0; i < targets.numInArray; ++i)
        targets.parray[i]->GetEM(0)->OnTrigger(this, instigator);
}

void zCTriggerBase::OnUntrigger(zCVob*, zCVob* instigator)
{
    zCArray<zCVob*> targets;
    homeWorld->SearchVobListByName(triggerTarget, targets);
    for (int i = 0; i < targets.numInArray; ++i)
        targets.parray[i]->GetEM(0)->OnUntrigger(this, instigator);
}

void zCTriggerBase::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteString(triggerTarget);
}

void zCTriggerBase::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    triggerTarget = archiver.ReadString();
    triggerTarget.Upper();
}

zCTrigger::zCTrigger()
    : reactToOnTrigger(1), reactToOnTouch(1), reactToOnDamage(1), respondToObject(1),
      respondToPC(1), respondToNPC(1), triggerFlags(5), numCanBeActivated(-1),
      retriggerWaitSec(0.0f), damageThreshold(0.0f), fireDelaySec(0.0f),
      nextTimeTriggerable(0.0f), savedOtherVob(0), countCanBeActivated(-1)
{
    SetCollisionClass(&zCCollObjectBoxPassThrough::s_oCollObjClass);
    SetCollDetStat(0);
    SetCollDetDyn(1);
    castDynShadow = (castDynShadow & 0xde) ^ 0x21;
}

zCClassDef* zCTrigger::_GetClassDef() const
{
    return classDef;
}

void zCTrigger::ClearStateInternals()
{
    triggerFlags &= ~2;
    nextTimeTriggerable = 0.0f;
    if (savedOtherVob) {
        savedOtherVob->Release();
        savedOtherVob = 0;
    }
}

zCTrigger::~zCTrigger()
{
    if (savedOtherVob) {
        savedOtherVob->Release();
        savedOtherVob = 0;
    }
}

int zCTrigger::CanBeActivatedNow(zCVob* other)
{
    if (!(triggerFlags & 2) || countCanBeActivated == 0 || IsOnTimer())
        return 0;
    if (ztimer.totalTimeFloat < nextTimeTriggerable)
        return 0;
    if (other) {
        if (respondToObject && other->GetCharacterClass() == 0)
            return 0;
        if (respondToPC && other->GetCharacterClass() != 0)
            return 0;
        if (respondToNPC && other->GetCharacterClass() != 1)
            return 0;
        if (respondToVobName.Length() == 0 || other->GetObjectName() != respondToVobName)
            return 0;
    }
    return 1;
}

void zCTrigger::TriggerTarget(zCVob* other)
{
    if (triggerTarget.Length() == 0)
        return;
    zCArray<zCVob*> targets;
    homeWorld->SearchVobListByName(triggerTarget, targets);
    for (int i = 0; i < targets.numInArray; ++i)
        targets.parray[i]->GetEM(0)->OnTrigger(this, other);
}

void zCTrigger::UntriggerTarget(zCVob* other)
{
    if (triggerTarget.Length() == 0)
        return;
    zCArray<zCVob*> targets;
    homeWorld->SearchVobListByName(triggerTarget, targets);
    for (int i = 0; i < targets.numInArray; ++i)
        targets.parray[i]->GetEM(0)->OnUntrigger(this, other);
}

void zCTrigger::TriggerTargetPost()
{
    if (countCanBeActivated > 0)
        --countCanBeActivated;
    nextTimeTriggerable = ztimer.totalTimeFloat + retriggerWaitSec * 1000.0f;
}

void zCTrigger::ActivateTrigger(zCVob* other)
{
    if (IsOnTimer() || !CanBeActivatedNow(other) || !(triggerFlags & 2))
        return;
    if (fireDelaySec > 0.0f) {
        if (savedOtherVob) {
            savedOtherVob->Release();
            savedOtherVob = 0;
        }
        savedOtherVob = other;
        if (savedOtherVob)
            savedOtherVob->AddRef();
        SetOnTimer(fireDelaySec * 1000.0f);
        return;
    }
    TriggerTarget(other);
    TriggerTargetPost();
}

void zCTrigger::DeactivateTrigger(zCVob* other)
{
    if (!IsOnTimer() && (triggerFlags & 2) && (triggerFlags & 4))
        UntriggerTarget(other);
}

void zCTrigger::OnTrigger(zCVob*, zCVob* instigator)
{
    if (reactToOnTrigger)
        ActivateTrigger(instigator);
}

void zCTrigger::OnUntrigger(zCVob*, zCVob* instigator)
{
    if (reactToOnTrigger)
        DeactivateTrigger(instigator);
}

void zCTrigger::OnTouch(zCVob* other)
{
    if (reactToOnTouch)
        ActivateTrigger(other);
}

void zCTrigger::OnUntouch(zCVob* other)
{
    if (reactToOnTouch)
        DeactivateTrigger(other);
}

void zCTrigger::OnDamage(zCVob*, zCVob* instigator, float damage, int, const zVEC3&)
{
    if (reactToOnDamage && damage > 0.0f && damage > damageThreshold)
        ActivateTrigger(instigator);
}

void zCTrigger::OnMessage(zCEventMessage* message, zCVob*)
{
    zCEventCommon* common = dynamic_cast<zCEventCommon*>(message);
    if (!common)
        return;
    if (common->subType == zCEventCommon::zEVENT_ENABLE)
        triggerFlags |= 2;
    else if (common->subType == zCEventCommon::zEVENT_DISABLE)
        triggerFlags &= ~2;
    else if (common->subType == zCEventCommon::zEVENT_TOGGLE_ENABLED)
        triggerFlags ^= 2;
}

void zCTrigger::OnTimer()
{
    SetSleeping(1);
    if (fireDelaySec > 0.0f) {
        TriggerTarget(savedOtherVob);
        TriggerTargetPost();
        if (savedOtherVob) {
            savedOtherVob->Release();
            savedOtherVob = 0;
        }
    }
}

void zCTrigger::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteString(triggerTarget);
    archiver.WriteGroupBegin("Trigger");
    archiver.WriteGroupBegin("TriggerFilter");
    if (!archiver.InProperties()) {
        archiver.WriteRaw(&triggerFlags, 1);
        unsigned char filterFlags = reactToOnTrigger | (reactToOnTouch << 1) |
            (reactToOnDamage << 2) | (respondToObject << 3) | (respondToPC << 4) |
            (respondToNPC << 5);
        archiver.WriteRaw(&filterFlags, 1);
    }
    else {
        archiver.WriteBool(reactToOnTrigger);
        archiver.WriteBool(reactToOnTouch);
        archiver.WriteBool(reactToOnDamage);
        archiver.WriteBool(respondToObject);
        archiver.WriteBool(respondToPC);
        archiver.WriteBool(respondToNPC);
        archiver.WriteBool(triggerFlags & 1);
    }
    archiver.WriteString(respondToVobName);
    archiver.WriteInt(numCanBeActivated);
    archiver.WriteFloat(retriggerWaitSec);
    archiver.WriteFloat(damageThreshold);
    archiver.WriteGroupEnd();
    archiver.WriteGroupBegin("TriggerDelay");
    archiver.WriteFloat(fireDelaySec);
    if (archiver.InProperties())
        archiver.WriteBool(triggerFlags & 4);
    archiver.WriteGroupEnd();
    if (archiver.InSaveGame()) {
        float timeRemaining = nextTimeTriggerable - ztimer.totalTimeFloat;
        if (timeRemaining < 0.0f)
            timeRemaining = 0.0f;
        archiver.WriteFloat(timeRemaining);
        archiver.WriteObject(savedOtherVob);
        archiver.WriteInt(countCanBeActivated);
        archiver.WriteBool(triggerFlags & 2);
    }
    archiver.WriteGroupEnd();
}

void zCTrigger::Unarchive(zCArchiver& archiver)
{
    ClearStateInternals();
    zCVob::Unarchive(archiver);
    triggerTarget = archiver.ReadString();
    triggerTarget.Upper();
    if (!archiver.InProperties()) {
        archiver.ReadRaw(&triggerFlags, 1);
        unsigned char filterFlags = 0;
        archiver.ReadRaw(&filterFlags, 1);
        reactToOnTrigger = filterFlags & 1;
        reactToOnTouch = (filterFlags >> 1) & 1;
        reactToOnDamage = (filterFlags >> 2) & 1;
        respondToObject = (filterFlags >> 3) & 1;
        respondToPC = (filterFlags >> 4) & 1;
        respondToNPC = (filterFlags >> 5) & 1;
    }
    else {
        reactToOnTrigger = archiver.ReadBool();
        reactToOnTouch = archiver.ReadBool();
        reactToOnDamage = archiver.ReadBool();
        respondToObject = archiver.ReadBool();
        respondToPC = archiver.ReadBool();
        respondToNPC = archiver.ReadBool();
        triggerFlags = (triggerFlags & ~1) | archiver.ReadBool();
    }
    respondToVobName = archiver.ReadString();
    countCanBeActivated = archiver.ReadInt();
    numCanBeActivated = countCanBeActivated;
    retriggerWaitSec = archiver.ReadFloat();
    damageThreshold = archiver.ReadFloat();
    fireDelaySec = archiver.ReadFloat();
    if (archiver.InProperties())
        triggerFlags = (triggerFlags & ~4) | (archiver.ReadBool() << 2);
    if (!archiver.InSaveGame()) {
        triggerFlags &= ~2;
        return;
    }
    nextTimeTriggerable = archiver.ReadFloat();
    zCObject* object = 0;
    archiver.ReadObject(object);
    savedOtherVob = static_cast<zCVob*>(object);
    countCanBeActivated = archiver.ReadInt();
    if (nextTimeTriggerable > 0.0f)
        nextTimeTriggerable += ztimer.totalTimeFloat;
    archiver.ReadBool();
}


zCMover::zCMover()
    : sfxMoving(0), touchBlockerDamage(0.0f), moverLocked(0), autoLinkEnabled(0),
      moverAniType(MA_KEYFRAME), moverBehavior(MB_2STATE_TOGGLE), touchBehavior(TB_TOGGLE),
      posLerpType(PL_CURVE), speedType(ST_SLOW_START_END), moveSpeed(0.3f), stayOpenTimeSec(2.0f)
{
    SetCollisionClass(&zCCollObjectComplex::s_oCollObjClass);
    SetCollDetStat(0);
    castDynShadow = (castDynShadow & 0xcf) ^ 0x10;
    reactToOnTrigger = 1;
    reactToOnTouch = 0;
    reactToOnDamage = 0;
    ClearStateInternals();
}

zCClassDef* zCMover::_GetClassDef() const
{
    return classDef;
}

void zCMover::ClearStateInternals()
{
    for (int i = 0; i < 3; ++i)
        actKeyPosDelta.n[i] = 0.0f;
    actKeyframeF = 0.0f;
    actKeyframe = 0;
    nextKeyframe = 0;
    moveSpeedUnit = 0.0f;
    advanceDir = 0.0f;
    moverState = MOVER_STATE_CLOSED;
    numTriggerEvents = 0;
    stayOpenTimeDest = 0.0f;
    model = 0;
    autoRotate = 0;
    if (sfxMoving) {
        zsound->StopSound(soundMovingHandle);
        sfxMoving->Release();
        sfxMoving = 0;
    }
}

zCMover::~zCMover()
{
    model = 0;
    if (sfxMoving) {
        sfxMoving->Release();
        sfxMoving = 0;
    }
}

int zCMover::CanThisCollideWith(zCVob* other)
{
    if (dynamic_cast<zCTriggerBase*>(other))
        return 0;
    if (other->castDynShadow & 0x10)
        return 0;
    zCTree* treeNode = other->globalVobTreeNode;
    if (treeNode && treeNode->parent && treeNode->parent->data)
        return 0;
    if (autoLinkEnabled && !other->CanAutoLinkWith(this))
        return 0;
    return 1;
}

void zCMover::UpdateInternals()
{
    if (sfxMoving) {
        sfxMoving->Release();
        sfxMoving = 0;
    }
    if (soundMoving.Length() > 0)
        sfxMoving = zsound->LoadSoundFX(soundMoving);
    if (visual)
        model = dynamic_cast<zCModel*>(visual);
}

zCMover::zTMov_Keyframe zCMover::GetKeyframe(zCVob* vob)
{
    zTMov_Keyframe keyframe;
    for (int i = 0; i < 3; ++i)
        keyframe.pos.n[i] = 0.0f;
    for (int i = 0; i < 4; ++i)
        keyframe.quat.q[i] = 0.0f;
    if (vob) {
        for (int i = 0; i < 3; ++i)
            keyframe.pos.n[i] = vob->trafoObjToWorld.m[i][3];
        keyframe.quat.Matrix4ToQuat(vob->trafoObjToWorld);
    }
    else
        keyframe.quat.q[3] = 1.0f;
    return keyframe;
}

void zCMover::AdvanceKeyframe_KF()
{
    SetToKeyframe_KF(actKeyframeF + advanceDir * moveSpeedUnit * ztimer.frameTimeFloat);
}


void zCMover::SetToKeyframe_KF(float keyframe)
{
    const int count = keyframeList.numInArray;
    if (count <= 1)
        return;
    actKeyframeF = keyframe;
    if (advanceDir < 0.0f) {
        if (moverBehavior <= MB_2STATE_OPEN_TIMED && actKeyframeF <= 0.0f) {
            actKeyframeF = 0.0f;
            advanceDir = 0.0f;
        }
        else if (moverBehavior == MB_NSTATE_LOOP && actKeyframeF < 0.0f)
            actKeyframeF = count - 0.00001f;
        else if (moverBehavior == MB_NSTATE_SINGLE_KEYS && actKeyframeF <= nextKeyframe) {
            actKeyframeF = nextKeyframe;
            advanceDir = 0.0f;
        }
    }
    else if (advanceDir > 0.0f) {
        if (moverBehavior <= MB_2STATE_OPEN_TIMED && actKeyframeF >= count - 1) {
            actKeyframeF = count - 1;
            advanceDir = 0.0f;
        }
        else if (moverBehavior == MB_NSTATE_LOOP && actKeyframeF >= count)
            actKeyframeF = 0.0f;
        else if (moverBehavior == MB_NSTATE_SINGLE_KEYS && actKeyframeF >= nextKeyframe) {
            actKeyframeF = nextKeyframe;
            advanceDir = 0.0f;
        }
    }
    actKeyframe = advanceDir < 0.0f ? static_cast<int>(std::ceil(actKeyframeF)) :
        static_cast<int>(std::floor(actKeyframeF));
    if (actKeyframe < 0)
        actKeyframe = count - 1;
    else if (actKeyframe >= count)
        actKeyframe = 0;
    if (moverBehavior != MB_NSTATE_SINGLE_KEYS) {
        nextKeyframe = actKeyframe + static_cast<int>(advanceDir);
        if (nextKeyframe < 0)
            nextKeyframe = count - 1;
        else if (nextKeyframe >= count)
            nextKeyframe = 0;
    }
    const zTMov_Keyframe& current = keyframeList.parray[actKeyframe];
    const zTMov_Keyframe& next = keyframeList.parray[nextKeyframe];
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    for (int i = 0; i < 3; ++i) {
        actKeyPosDelta.n[i] = next.pos.n[i] - current.pos.n[i];
        const float value = std::fabs(actKeyPosDelta.n[i]);
        if (i == 0)
            x = value;
        else if (i == 1)
            y = value;
        else
            z = value;
    }
    if (x < y) {
        const float swap = x;
        x = y;
        y = swap;
    }
    if (x < z) {
        const float swap = x;
        x = z;
        z = swap;
    }
    float distance = x - x * 0.0625f + (y + z) * 0.375f;
    if (distance < 0.01f)
        distance = 10000.0f;
    moveSpeedUnit = moveSpeed / distance;
    if (advanceDir != 0.0f)
        return;
    nextKeyframe = actKeyframe;
    if (moverState == MOVER_STATE_OPENING) {
        SetSleeping(1);
        moverState = MOVER_STATE_OPEN;
        zsound->StopSound(soundMovingHandle);
        zsound->PlaySound3D(soundOpenEnd, this, 1, 0);
        if (moverBehavior == MB_2STATE_OPEN_TIMED) {
            stayOpenTimeDest = ztimer.totalTimeFloat + stayOpenTimeSec * 1000.0f;
            SetSleeping(0);
        }
        TriggerTarget(this);
    }
    else if (moverState == MOVER_STATE_CLOSING) {
        SetSleeping(1);
        moverState = MOVER_STATE_CLOSED;
        zsound->PlaySound3D(soundCloseEnd, this, 1, 0);
        zsound->StopSound(soundMovingHandle);
        UntriggerTarget(this);
    }
}

void zCMover::InterpolateKeyframes_KF()
{
    if (keyframeList.numInArray <= 1)
        return;
    float fraction = actKeyframeF - std::floor(actKeyframeF);
    if (speedType == ST_SEG_SLOW_START_END ||
        (speedType == ST_SLOW_START_END && moverBehavior != MB_NSTATE_LOOP))
        fraction = (std::sin(fraction * 3.14159274f - 1.57079637f) + 1.0f) * 0.5f;
    else if (speedType == ST_SEG_SLOW_START || speedType == ST_SLOW_START)
        fraction = 1.0f - std::cos(fraction * 1.57079637f);
    else if (speedType == ST_SEG_SLOW_END || speedType == ST_SLOW_END)
        fraction = std::sin(fraction * 1.57079637f);
    if (advanceDir < 0.0f)
        fraction = 1.0f - fraction;
    if (posLerpType == PL_CURVE && keyframeList.numInArray > 2)
        fraction = fraction * fraction * (3.0f - 2.0f * fraction);
    const zTMov_Keyframe& current = keyframeList.parray[actKeyframe];
    const zTMov_Keyframe& next = keyframeList.parray[nextKeyframe];
    zVEC3 position;
    zVEC3 movement;
    for (int i = 0; i < 3; ++i) {
        position.n[i] = current.pos.n[i] + actKeyPosDelta.n[i] * fraction;
        movement.n[i] = position.n[i] - trafoObjToWorld.m[i][3];
    }
    if (autoRotate && (movement.n[0] != 0.0f || movement.n[1] != 0.0f || movement.n[2] != 0.0f)) {
        SetHeadingWorld(movement);
        SetPositionWorld(position);
        return;
    }
    SetPositionWorld(position);
    zCQuat rotation;
    rotation.Slerp(fraction, current.quat, next.quat);
    SetRotationWorld(rotation);
}

void zCMover::AdvanceMover()
{
    if (advanceDir == 0.0f)
        return;
    if (moverAniType == MA_KEYFRAME) {
        if (keyframeList.numInArray > 0) {
            AdvanceKeyframe_KF();
            InterpolateKeyframes_KF();
        }
        return;
    }
    if (moverAniType == MA_MODEL_ANI && model) {
        zCModelAni* opening = model->GetAniFromAniID(model->GetAniIDFromAniName("T_CLOSED_2_OPEN"));
        zCModelAni* closing = model->GetAniFromAniID(model->GetAniIDFromAniName("T_OPEN_2_CLOSED"));
        if ((!opening || !model->IsAniActive(opening)) && (!closing || !model->IsAniActive(closing))) {
            advanceDir = 0.0f;
            if (moverState == MOVER_STATE_OPENING) {
                SetSleeping(1);
                moverState = MOVER_STATE_OPEN;
                zsound->StopSound(soundMovingHandle);
                zsound->PlaySound3D(soundOpenEnd, this, 1, 0);
                if (moverBehavior == MB_2STATE_OPEN_TIMED) {
                    stayOpenTimeDest = ztimer.totalTimeFloat + stayOpenTimeSec * 1000.0f;
                    SetSleeping(0);
                }
                TriggerTarget(this);
            }
            else if (moverState == MOVER_STATE_CLOSING) {
                SetSleeping(1);
                moverState = MOVER_STATE_CLOSED;
                zsound->PlaySound3D(soundCloseEnd, this, 1, 0);
                zsound->StopSound(soundMovingHandle);
                UntriggerTarget(this);
            }
        }
    }
}

void zCMover::SetToKeyframe(float keyframe, float direction)
{
    advanceDir = direction;
    if (moverAniType == MA_KEYFRAME) {
        SetToKeyframe_KF(keyframe);
        if (keyframe != 0.0f) {
            AdvanceKeyframe_KF();
            InterpolateKeyframes_KF();
        }
        return;
    }
    if (moverAniType != MA_MODEL_ANI || !model)
        return;
    const int index = static_cast<int>(keyframe);
    if (moverBehavior <= MB_2STATE_OPEN_TIMED) {
        if (index == 0)
            model->StartAni(direction > 0.0f ? "T_CLOSED_2_OPEN" : "S_CLOSED", 0);
        else
            model->StartAni(direction < 0.0f ? "T_OPEN_2_CLOSED" : "S_OPEN", 0);
    }
    else
        model->StartAni(index, 0);
}

void zCMover::InvertMovement()
{
    if (moverBehavior <= MB_2STATE_OPEN_TIMED) {
        if (moverState == MOVER_STATE_OPENING)
            moverState = MOVER_STATE_CLOSING;
        else if (moverState == MOVER_STATE_CLOSING)
            moverState = MOVER_STATE_OPENING;
    }
    SetToKeyframe(actKeyframeF, -advanceDir);
    if (moverAniType == MA_MODEL_ANI && model) {
        for (int i = 0; i < model->numActiveAnis; ++i)
            model->aniChannels[i]->SetDirection(model->aniChannels[i]->advanceDir == -1 ? 0 : 1);
    }
}


void zCMover::SetVisual(zCVisual* newVisual)
{
    zCVob::SetVisual(newVisual);
    model = visual ? dynamic_cast<zCModel*>(visual) : 0;
    moverAniType = model ? MA_MODEL_ANI : MA_KEYFRAME;
    if (model)
        model->StartAni(moverLocked ? "S_LOCKED" : "S_UNLOCKED", 0);
}

void zCMover::StartMovingSound()
{
    zCSoundSystem::zTSound3DParams params;
    params.obstruction = 0.0f;
    params.volume = 1.0f;
    params.radius = -1.0f;
    params.loopType = 1;
    params.coneAngleDeg = 0.0f;
    params.reverbLevel = 1.0f;
    params.isAmbient3D = 0;
    params.pitchOffset = -999999.0f;
    soundMovingHandle = zsound->PlaySound3D(sfxMoving, this, 2, &params);
}

void zCMover::MoveToKeyframe(int keyframe)
{
    if (advanceDir != 0.0f || moverBehavior != MB_NSTATE_SINGLE_KEYS)
        return;
    SetSleeping(0);
    moverState = MOVER_STATE_OPENING;
    if (keyframe < 0)
        keyframe = 0;
    else if (keyframe >= keyframeList.numInArray)
        keyframe = keyframeList.numInArray - 1;
    nextKeyframe = keyframe;
    SetToKeyframe(actKeyframeF, 1.0f);
}

void zCMover::DoOpen()
{
    SetSleeping(0);
    moverState = MOVER_STATE_OPENING;
    SetToKeyframe(0.0f, 1.0f);
    zsound->PlaySound3D(soundOpenStart, this, 1, 0);
    StartMovingSound();
}

void zCMover::FinishedOpening()
{
    SetSleeping(1);
    moverState = MOVER_STATE_OPEN;
    zsound->StopSound(soundMovingHandle);
    zsound->PlaySound3D(soundOpenEnd, this, 1, 0);
    if (moverBehavior == MB_2STATE_OPEN_TIMED) {
        stayOpenTimeDest = ztimer.totalTimeFloat + stayOpenTimeSec * 1000.0f;
        SetSleeping(0);
    }
    TriggerTarget(this);
}

void zCMover::DoClose()
{
    SetSleeping(0);
    moverState = MOVER_STATE_CLOSING;
    SetToKeyframe(keyframeList.numInArray - 1, -1.0f);
    zsound->PlaySound3D(soundCloseStart, this, 1, 0);
    StartMovingSound();
}

void zCMover::FinishedClosing()
{
    SetSleeping(1);
    moverState = MOVER_STATE_CLOSED;
    zsound->PlaySound3D(soundCloseEnd, this, 1, 0);
    zsound->StopSound(soundMovingHandle);
    UntriggerTarget(this);
}


void zCMover::Unlock(zCVob* other)
{
    if (!moverLocked || moverState != MOVER_STATE_CLOSED || !IsKeyToThisMover(other))
        return;
    zCModel* modelVisual = visual ? dynamic_cast<zCModel*>(visual) : 0;
    if (modelVisual)
        modelVisual->StartAni("T_LOCKED_2_UNLOCKED", 0);
    zsound->PlaySound3D(soundUnlock, this, 1, 0);
    moverLocked = 0;
}

void zCMover::Lock(zCVob* other)
{
    if (moverLocked || moverState != MOVER_STATE_CLOSED || !IsKeyToThisMover(other))
        return;
    zCModel* modelVisual = visual ? dynamic_cast<zCModel*>(visual) : 0;
    if (modelVisual)
        modelVisual->StartAni("T_UNLOCKED_2_LOCKED", 0);
    zsound->PlaySound3D(soundLock, this, 1, 0);
    moverLocked = 1;
}

int zCMover::IsKeyToThisMover(zCVob* other)
{
    return respondToVobName.Length() == 0 || other->GetObjectName() == respondToVobName;
}

void zCMover::TriggerMover(zCVob* other)
{
    if (moverLocked && respondToVobName.Length() > 0 && !IsKeyToThisMover(other)) {
        zsound->PlaySound3D(soundUseLocked, this, 1, 0);
        return;
    }
    if (!CanBeActivatedNow(other))
        return;
    if (moverLocked)
        Unlock(other);
    if (moverBehavior == MB_NSTATE_SINGLE_KEYS)
        return;
    if (moverBehavior == MB_NSTATE_LOOP) {
        if (advanceDir == 0.0f)
            DoOpen();
        else
            InvertMovement();
    }
    else if (moverBehavior == MB_2STATE_TRIGGER_CONTROL) {
        ++numTriggerEvents;
        if (moverState == MOVER_STATE_CLOSED)
            DoOpen();
    }
    else if (moverBehavior == MB_2STATE_TOGGLE) {
        if (moverState == MOVER_STATE_CLOSED)
            DoOpen();
        else if (moverState == MOVER_STATE_OPEN)
            DoClose();
        else
            InvertMovement();
    }
    else if (moverBehavior == MB_2STATE_OPEN_TIMED) {
        if (moverState == MOVER_STATE_CLOSED)
            DoOpen();
        else if (moverState == MOVER_STATE_OPEN) {
            if (stayOpenTimeDest != 0.0f)
                stayOpenTimeDest = ztimer.totalTimeFloat + stayOpenTimeSec * 1000.0f;
            else
                stayOpenTimeDest = 1.0f;
        }
        else
            InvertMovement();
    }
}

void zCSoundSystem::zTSound3DParams::SetDefaults()
{
    obstruction = 0.0f;
    volume = 1.0f;
    radius = -1.0f;
    loopType = 1;
    coneAngleDeg = 0.0f;
    reverbLevel = 1.0f;
    isAmbient3D = 0;
    pitchOffset = -999999.0f;
}

void zCMover::OnTick()
{
    if (moverState == MOVER_STATE_OPEN && stayOpenTimeDest != 0.0f &&
        ztimer.totalTimeFloat >= stayOpenTimeDest) {
        stayOpenTimeDest = 0.0f;
        DoClose();
    }
    AdvanceMover();
}

void zCMover::OnTrigger(zCVob*, zCVob* instigator)
{
    if ((triggerFlags & 2) && reactToOnTrigger)
        TriggerMover(instigator);
}

void zCMover::OnUntrigger(zCVob*, zCVob*)
{
    if (!(triggerFlags & 2) || moverBehavior != MB_2STATE_TRIGGER_CONTROL)
        return;
    --numTriggerEvents;
    if (numTriggerEvents > 0)
        return;
    if (moverState == MOVER_STATE_OPENING)
        InvertMovement();
    else if (moverState == MOVER_STATE_OPEN)
        DoClose();
}

void zCMover::OnTouch(zCVob* other)
{
    if (autoLinkEnabled)
        other->CheckAutoLink(this);
    if (!(triggerFlags & 2) || !other)
        return;
    if ((sleepingMode & 3) && advanceDir != 0.0f) {
        if (touchBlockerDamage > 0.0f) {
            zVEC3 hitLocation;
            hitLocation.n[0] = 0.0f;
            hitLocation.n[1] = 0.0f;
            hitLocation.n[2] = 0.0f;
            other->GetEM(0)->OnDamage(this, this, touchBlockerDamage, 1, hitLocation);
        }
    }
    else if (reactToOnTouch)
        TriggerMover(other);
}

void zCMover::OnDamage(zCVob*, zCVob* instigator, float damage, int, const zVEC3&)
{
    if ((triggerFlags & 2) && reactToOnDamage && damage > 0.0f && damage > damageThreshold)
        TriggerMover(instigator);
}

void zCMover::OnMessage(zCEventMessage* message, zCVob* sender)
{
    zCEventCommon* common = dynamic_cast<zCEventCommon*>(message);
    if (common) {
        if (common->subType == zCEventCommon::zEVENT_ENABLE)
            triggerFlags |= 2;
        else if (common->subType == zCEventCommon::zEVENT_DISABLE)
            triggerFlags &= ~2;
        else if (common->subType == zCEventCommon::zEVENT_TOGGLE_ENABLED)
            triggerFlags ^= 2;
    }
    zCEventMover* moverEvent = dynamic_cast<zCEventMover*>(message);
    if (moverEvent) {
        int keyframe = moverEvent->gotoFixedKeyframe;
        if (moverEvent->subType == zCEventMover::zEVENT_GOTO_KEY_NEXT)
            keyframe = (static_cast<int>(std::floor(actKeyframeF)) + 1) % keyframeList.numInArray;
        else if (moverEvent->subType == zCEventMover::zEVENT_GOTO_KEY_PREV) {
            keyframe = static_cast<int>(std::floor(actKeyframeF)) - 1;
            if (keyframe < 0)
                keyframe = keyframeList.numInArray - 1;
        }
        MoveToKeyframe(keyframe);
        return;
    }
    if (!common || common->subType != zCEventCommon::zEVENT_RESET)
        return;
    if (moverBehavior == MB_2STATE_TOGGLE) {
        if (moverState == MOVER_STATE_OPEN)
            DoClose();
        else
            InvertMovement();
    }
    else if (moverBehavior == MB_NSTATE_SINGLE_KEYS)
        MoveToKeyframe(0);
}

void zCMover::PostLoad()
{
}


void zCMover::Archive(zCArchiver& archiver)
{
    zCTrigger::Archive(archiver);
    archiver.WriteGroupBegin("Mover");
    archiver.WriteEnum(moverBehavior);
    archiver.WriteFloat(touchBlockerDamage);
    archiver.WriteFloat(stayOpenTimeSec);
    archiver.WriteBool(moverLocked);
    archiver.WriteBool(autoLinkEnabled);
    archiver.WriteBool(autoRotate);
    archiver.WriteGroupBegin("Keyframes");
    archiver.WriteWord(keyframeList.numInArray);
    if (keyframeList.numInArray > 0) {
        archiver.WriteFloat(moveSpeed);
        archiver.WriteEnum(posLerpType);
        archiver.WriteEnum(speedType);
        if (!archiver.InProperties())
            archiver.WriteRaw(keyframeList.parray, keyframeList.numInArray * sizeof(zTMov_Keyframe));
    }
    archiver.WriteGroupEnd();
    if (archiver.InSaveGame()) {
        archiver.WriteVec3(actKeyPosDelta);
        archiver.WriteFloat(actKeyframeF);
        archiver.WriteInt(actKeyframe);
        archiver.WriteInt(nextKeyframe);
        archiver.WriteFloat(moveSpeedUnit);
        archiver.WriteFloat(advanceDir);
        archiver.WriteEnum(moverState);
        archiver.WriteInt(numTriggerEvents);
        archiver.WriteFloat(stayOpenTimeDest);
    }
    archiver.WriteGroupBegin("Sounds");
    archiver.WriteString(soundOpenStart);
    archiver.WriteString(soundOpenEnd);
    archiver.WriteString(soundMoving);
    archiver.WriteString(soundCloseStart);
    archiver.WriteString(soundCloseEnd);
    archiver.WriteString(soundLock);
    archiver.WriteString(soundUnlock);
    archiver.WriteString(soundUseLocked);
    archiver.WriteGroupEnd();
    archiver.WriteGroupEnd();
}

void zCMover::Unarchive(zCArchiver& archiver)
{
    zCTrigger::Unarchive(archiver);
    ClearStateInternals();
    moverBehavior = static_cast<zTMoverBehavior>(archiver.ReadEnum());
    touchBlockerDamage = archiver.ReadFloat();
    stayOpenTimeSec = archiver.ReadFloat();
    moverLocked = archiver.ReadBool();
    autoLinkEnabled = archiver.ReadBool();
    autoRotate = archiver.ReadBool();
    const int numKeyframes = archiver.ReadWord();
    if (numKeyframes > 0) {
        moveSpeed = archiver.ReadFloat();
        posLerpType = static_cast<zTPosLerpType>(archiver.ReadEnum());
        speedType = static_cast<zTSpeedType>(archiver.ReadEnum());
        if (!archiver.InProperties()) {
            keyframeList.AllocAbs(numKeyframes);
            archiver.ReadRaw(keyframeList.parray, numKeyframes * sizeof(zTMov_Keyframe));
        }
    }
    if (archiver.InSaveGame()) {
        actKeyPosDelta = archiver.ReadVec3();
        actKeyframeF = archiver.ReadFloat();
        actKeyframe = archiver.ReadInt();
        nextKeyframe = archiver.ReadInt();
        moveSpeedUnit = archiver.ReadFloat();
        advanceDir = archiver.ReadFloat();
        moverState = static_cast<zTMoverState>(archiver.ReadEnum());
        numTriggerEvents = archiver.ReadInt();
        stayOpenTimeDest = archiver.ReadFloat();
    }
    soundOpenStart = archiver.ReadString();
    soundOpenEnd = archiver.ReadString();
    soundMoving = archiver.ReadString();
    soundCloseStart = archiver.ReadString();
    soundCloseEnd = archiver.ReadString();
    soundLock = archiver.ReadString();
    soundUnlock = archiver.ReadString();
    soundUseLocked = archiver.ReadString();
    UpdateInternals();
    if (!archiver.InSaveGame())
        SetToKeyframe(0.0f, 0.0f);
}

zCEarthquake::~zCEarthquake()
{
}

void zCEarthquake::OnTrigger(zCVob*, zCVob*)
{
    zVEC3 position;
    for (int i = 0; i < 3; ++i)
        position.n[i] = trafoObjToWorld.m[i][3];
    zCCamera::activeCam->AddTremor(position, radiusSquare, timeMSec, amplitude);
}

void zCEarthquake::OnUntrigger(zCVob* other, zCVob* instigator)
{
    OnTrigger(other, instigator);
}

void zCEarthquake::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteGroupBegin("Earthquake");
    archiver.WriteFloat(std::sqrt(radiusSquare));
    archiver.WriteFloat(timeMSec * 0.001f);
    archiver.WriteVec3(amplitude);
    archiver.WriteGroupEnd();
}

void zCEarthquake::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    radiusSquare = archiver.ReadFloat();
    radiusSquare *= radiusSquare;
    timeMSec = archiver.ReadFloat() * 1000.0f;
    amplitude = archiver.ReadVec3();
}


zCTriggerTeleport::~zCTriggerTeleport()
{
    if (teleportSound) {
        teleportSound->Release();
        teleportSound = 0;
    }
}

void zCTriggerTeleport::DoTeleport(zCVob* other)
{
    if (!CanBeActivatedNow(other))
        return;
    zCVob* destination = homeWorld->SearchVobByName(triggerTarget);
    if (!destination)
        return;
    destination->SetCollDetStat(0);
    destination->SetCollDetDyn(0);
    EndMovement(1);
    other->EndMovement(1);
    const unsigned long collisionFlags = other->castDynShadow;
    other->SetCollDetStat(0);
    other->SetCollDetDyn(0);
    other->SetTrafo(destination->trafoObjToWorld);
    zsound->PlaySound3D(teleportSound, other, 0, 0);
    other->SetCollDetStat((collisionFlags >> 6) & 1);
    other->SetCollDetDyn((collisionFlags >> 7) & 1);
}

void zCTriggerTeleport::OnTouch(zCVob* other)
{
    DoTeleport(other);
}

void zCTriggerTeleport::OnTrigger(zCVob* other, zCVob*)
{
    DoTeleport(other);
}

void zCTriggerTeleport::OnUntrigger(zCVob* other, zCVob*)
{
    DoTeleport(other);
}

void zCTriggerTeleport::Archive(zCArchiver& archiver)
{
    zCTrigger::Archive(archiver);
    archiver.WriteString(teleportSoundName);
}

void zCTriggerTeleport::Unarchive(zCArchiver& archiver)
{
    zCTrigger::Unarchive(archiver);
    teleportSoundName = archiver.ReadString();
    if (teleportSound)
        teleportSound->Release();
    teleportSound = zsound->LoadSoundFX(teleportSoundName);
    SetCollisionClass(&zCCollObjectBoxPassThrough::s_oCollObjClass);
}


zCPFXControler::~zCPFXControler()
{
}

zCParticleFX* zCPFXControler::GetPFX()
{
    zCParticleFX* particleFX = visual ? dynamic_cast<zCParticleFX*>(visual) : 0;
    if (!particleFX) {
        SetVisual(pfxName);
        particleFX = visual ? dynamic_cast<zCParticleFX*>(visual) : 0;
    }
    return particleFX;
}

void zCPFXControler::PostLoad()
{
    if (pfxStartOn)
        OnTrigger(0, 0);
}

void zCPFXControler::OnTrigger(zCVob*, zCVob*)
{
    zCParticleFX* particleFX = GetPFX();
    if (!particleFX)
        return;
    particleFX->dontKillPFXWhenDone = !killVobWhenDone;
    particleFX->RestoreEmitterOutput();
    particleFX->CreateParticles();
}

void zCPFXControler::OnUntrigger(zCVob*, zCVob*)
{
    zCParticleFX* particleFX = visual ? dynamic_cast<zCParticleFX*>(visual) : 0;
    if (particleFX)
        particleFX->StopEmitterOutput();
}

void zCPFXControler::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteString(pfxName);
    archiver.WriteBool(killVobWhenDone);
    archiver.WriteBool(pfxStartOn);
}

void zCPFXControler::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    pfxName = archiver.ReadString();
    killVobWhenDone = archiver.ReadBool();
    pfxStartOn = archiver.ReadBool();
    if (archiver.InSaveGame())
        pfxStartOn = 0;
    if (pfxName.Search(".PFX") == -1)
        pfxName.Append(".PFX");
}


zCTriggerList::~zCTriggerList()
{
}

void zCTriggerList::Init()
{
    listProcess = LP_ALL;
    actTarget = 0;
    sendOnTrigger = 1;
    for (int i = 0; i < 6; ++i) {
        triggerTargetList[i].Clear();
        fireDelayList[i] = 0.0f;
    }
}

const zSTRING* zCTriggerList::GetTriggerTarget(int index) const
{
    return index < 6 ? &triggerTargetList[index] : 0;
}

void zCTriggerList::TriggerActTarget(zCVob* other)
{
    zCArray<zCVob*> targets;
    homeWorld->SearchVobListByName(triggerTargetList[actTarget], targets);
    for (int i = 0; i < targets.numInArray; ++i) {
        if (sendOnTrigger)
            targets.parray[i]->GetEM(0)->OnTrigger(this, other);
        else
            targets.parray[i]->GetEM(0)->OnUntrigger(this, other);
    }
}

int zCTriggerList::ProcessActTarget(zCVob* other)
{
    if (fireDelayList[actTarget] <= 0.0f) {
        TriggerActTarget(other);
        return 1;
    }
    if (savedOtherVob) {
        savedOtherVob->Release();
        savedOtherVob = 0;
    }
    savedOtherVob = other;
    if (savedOtherVob)
        savedOtherVob->AddRef();
    SetOnTimer(fireDelayList[actTarget] * 1000.0f);
    return 0;
}

void zCTriggerList::FinishActivation()
{
    SetSleeping(1);
    if (savedOtherVob) {
        savedOtherVob->Release();
        savedOtherVob = 0;
    }
    TriggerTargetPost();
}

void zCTriggerList::OnTimer()
{
    TriggerActTarget(savedOtherVob);
    if (listProcess == LP_ALL) {
        ++actTarget;
        DoTriggering(savedOtherVob);
    }
    else
        FinishActivation();
}

void zCTriggerList::DoTriggering(zCVob* other)
{
    if (!(triggerFlags & 2) || IsOnTimer())
        return;
    if (listProcess == LP_NEXT_ONE) {
        int checked = 0;
        while (triggerTargetList[actTarget].Length() == 0 && checked < 6) {
            actTarget = (actTarget + 1) % 6;
            ++checked;
        }
        if (checked >= 6)
            return;
        if (ProcessActTarget(other))
            actTarget = (actTarget + 1) % 6;
        else
            return;
    }
    else if (listProcess == LP_RAND_ONE) {
        int previous = actTarget;
        int checked = 0;
        do {
            actTarget = std::rand() % 6;
            ++checked;
        } while ((actTarget == previous || triggerTargetList[actTarget].Length() == 0) && checked < 6);
        if (triggerTargetList[actTarget].Length() == 0 || !ProcessActTarget(other))
            return;
    }
    else {
        while (actTarget < 6) {
            if (triggerTargetList[actTarget].Length() > 0 && !ProcessActTarget(other))
                return;
            ++actTarget;
        }
        actTarget = 0;
    }
    FinishActivation();
}

void zCTriggerList::TriggerTarget(zCVob* other)
{
    sendOnTrigger = 1;
    DoTriggering(other);
}

void zCTriggerList::UntriggerTarget(zCVob* other)
{
    if (triggerFlags & 4) {
        sendOnTrigger = 0;
        DoTriggering(other);
    }
}

void zCTriggerList::Archive(zCArchiver& archiver)
{
    zCTrigger::Archive(archiver);
    archiver.WriteGroupBegin("TriggerList");
    archiver.WriteEnum(listProcess);
    int count = 6;
    if (!archiver.InProperties()) {
        while (count > 0 && triggerTargetList[count - 1].Length() == 0)
            --count;
        archiver.WriteByte(count);
    }
    for (int i = 0; i < count; ++i) {
        archiver.WriteString(triggerTargetList[i]);
        archiver.WriteFloat(fireDelayList[i]);
    }
    if (archiver.InSaveGame()) {
        archiver.WriteByte(actTarget);
        archiver.WriteBool(sendOnTrigger);
    }
    archiver.WriteGroupEnd();
}

void zCTriggerList::Unarchive(zCArchiver& archiver)
{
    zCTrigger::Unarchive(archiver);
    Init();
    listProcess = static_cast<zTListProcess>(archiver.ReadEnum());
    int count = 6;
    if (!archiver.InProperties())
        count = archiver.ReadByte();
    for (int i = 0; i < count; ++i) {
        triggerTargetList[i] = archiver.ReadString();
        fireDelayList[i] = archiver.ReadFloat();
        triggerTargetList[i].Upper();
    }
    if (archiver.InSaveGame()) {
        actTarget = archiver.ReadByte();
        sendOnTrigger = archiver.ReadBool();
    }
}


zCTouchDamage::~zCTouchDamage()
{
}

void zCTouchDamage::OnTouch(zCVob* other)
{
    if (!other)
        return;
    ProcessToucher(other);
    if (damageRepeatDelaySec > 0.0f && !IsOnTimer())
        SetOnTimer(damageRepeatDelaySec * 1000.0f);
}

void zCTouchDamage::OnUntouch(zCVob*)
{
    if (touchVobList.numInArray == 0)
        SetSleeping(1);
}

void zCTouchDamage::OnTimer()
{
    if (touchVobList.numInArray <= 0) {
        SetSleeping(1);
        return;
    }
    for (int i = 0; i < touchVobList.numInArray; ++i)
        ProcessToucher(touchVobList.parray[i]);
    if (damageRepeatDelaySec > 0.0f)
        SetOnTimer(damageRepeatDelaySec * 1000.0f);
}

void zCTouchDamage::ProcessToucher(zCVob* other)
{
    if (!TestCollType(other))
        return;
    zVEC3 hitLocation;
    for (int i = 0; i < 3; ++i)
        hitLocation.n[i] = (trafoObjToWorld.m[i][3] + other->trafoObjToWorld.m[i][3]) * 0.5f;
    other->GetEM(0)->OnDamage(this, this, damage, damageType, hitLocation);
}

int zCTouchDamage::TestCollType(zCVob* other)
{
    if (damageCollType == CT_NONE)
        return 0;
    if (damageCollType == CT_BOX) {
        zTBBox3D box = bbox3D;
        box.Scale(damageVolDownScale);
        return box.IsIntersecting(other->bbox3D);
    }
    zTBBox3D box = other->bbox3D;
    box.Scale(damageVolDownScale);
    for (int i = 0; i < 3; ++i) {
        const float position = trafoObjToWorld.m[i][3];
        if (position < box.mins.n[i] || position > box.maxs.n[i])
            return 0;
    }
    return 1;
}

void zCTouchDamage::FireDamageMessage(zCVob* other)
{
    ProcessToucher(other);
}

void zCTouchDamage::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteGroupBegin("TouchDamage");
    archiver.WriteFloat(damage);
    archiver.WriteGroupBegin("DamageTypes");
    for (unsigned long i = 0; GetDamageTypeArcEnum(i); ++i)
        archiver.WriteBool((damageType & (1 << i)) != 0);
    archiver.WriteGroupEnd();
    archiver.WriteFloat(damageRepeatDelaySec);
    archiver.WriteFloat(damageVolDownScale);
    archiver.WriteEnum(damageCollType);
    archiver.WriteGroupEnd();
}

void zCTouchDamage::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    damage = archiver.ReadFloat();
    damageType = 0;
    unsigned long i = 0;
    while (GetDamageTypeArcEnum(i)) {
        damageType |= archiver.ReadBool() << i;
        ++i;
    }
    if (i == 0)
        damageType = 1;
    damageRepeatDelaySec = archiver.ReadFloat();
    damageVolDownScale = archiver.ReadFloat();
    damageCollType = static_cast<zTDamageCollType>(archiver.ReadEnum());
}

const char* zCTouchDamage::GetDamageTypeArcEnum(unsigned long index)
{
    static const char* names[] = {
        "BARRIER", "BLUNT", "EDGE", "FIRE", "FLY", "MAGIC", "POINT", "FALL"
    };
    return index < 8 ? names[index] : 0;
}

void zCTouchDamage::SetVisual(zCVisual* newVisual)
{
    zCVob::SetVisual(newVisual);
    SetVobProperties();
}

void zCTouchDamage::SetVobProperties()
{
    SetCollisionClass(&zCCollObjectBoxPassThrough::s_oCollObjClass);
    SetCollDetStat(0);
    SetCollDetDyn(1);
    castDynShadow = (castDynShadow & 0xde) ^ 0x21;
}


zCTouchAnimate::~zCTouchAnimate()
{
    if (touchSound) {
        touchSound->Release();
        touchSound = 0;
    }
}

void zCTouchAnimate::OnTouch(zCVob* other)
{
    if (!other)
        return;
    if ((sleepingMode & 3) || !zCCamera::activeCam) {
        SetSleeping(0);
        return;
    }
    const zMAT4& cameraTrafo = zCCamera::activeCam->connectedVob->trafoObjToWorld;
    float distanceSquare = 0.0f;
    for (int i = 0; i < 3; ++i) {
        const float delta = cameraTrafo.m[i][3] - trafoObjToWorld.m[i][3];
        distanceSquare += delta * delta;
    }
    if (distanceSquare <= 1500.0f * 1500.0f)
        SetSleeping(0);
}

void zCTouchAnimate::OnUntouch(zCVob*)
{
    if (touchVobList.numInArray == 0)
        SetSleeping(1);
}

void zCTouchAnimate::OnTick()
{
    if (touchVobList.numInArray <= 0) {
        if (!IsAniActive())
            SetSleeping(1);
        return;
    }
    if (IsAniActive())
        return;
    int hasCandidate = 0;
    for (int i = 0; i < touchVobList.numInArray; ++i) {
        zCVob* other = touchVobList.parray[i];
        const zVEC3 velocity = other->GetVelocity();
        const float speedSquare = velocity.n[0] * velocity.n[0] +
            velocity.n[1] * velocity.n[1] + velocity.n[2] * velocity.n[2];
        if (speedSquare <= 10.0f && IsTouching(other)) {
            StartAni();
            return;
        }
        if (!(other->castDynShadow & 8) && !(other->sleepingMode & 3))
            hasCandidate = 1;
    }
    if (!hasCandidate)
        SetSleeping(1);
}

int zCTouchAnimate::IsTouching(zCVob* other)
{
    zTBBox3D box = bbox3D;
    box.Scale(0.5f);
    return box.IsIntersecting(other->bbox3D);
}

void zCTouchAnimate::StartAni()
{
    zCVisualAnimate* animatedVisual = visual ? dynamic_cast<zCVisualAnimate*>(visual) : 0;
    if (!animatedVisual)
        return;
    if (animatedVisual->IsAniActive())
        animatedVisual->StartAni();
    if (!touchSound) {
        const zSTRING soundName = GetSoundName();
        touchSound = zsound->LoadSoundFX(soundName);
        if (!touchSound) {
            zSTRING waveName(soundName);
            waveName.Append(".wav");
            touchSound = zsound->LoadSoundFX(waveName);
        }
    }
    zsound->PlaySound3D(touchSound, this, 1, 0);
}

void zCTouchAnimate::StopAni()
{
    zCVisualAnimate* animatedVisual = visual ? dynamic_cast<zCVisualAnimate*>(visual) : 0;
    if (animatedVisual && animatedVisual->IsAniActive())
        animatedVisual->StopAni();
}

int zCTouchAnimate::IsAniActive()
{
    zCVisualAnimate* animatedVisual = visual ? dynamic_cast<zCVisualAnimate*>(visual) : 0;
    return animatedVisual ? animatedVisual->IsAniActive() : 0;
}

zSTRING zCTouchAnimate::GetSoundName()
{
    return touchSoundName;
}

void zCTouchAnimate::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
}

void zCTouchAnimate::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    if (touchSound) {
        touchSound->Release();
        touchSound = 0;
    }
    SetVobProperties();
}

void zCTouchAnimate::SetVisual(zCVisual* newVisual)
{
    zCVob::SetVisual(newVisual);
    SetVobProperties();
}

void zCTouchAnimate::SetVobProperties()
{
    SetCollisionClass(&zCCollObjectBoxPassThrough::s_oCollObjClass);
    SetCollDetStat(0);
    SetCollDetDyn(1);
    castDynShadow = (castDynShadow & 0xde) ^ 0x21;
}


zCVisual* zCVob::GetVisual() const
{
    return visual;
}


zCTouchAnimateSound::~zCTouchAnimateSound()
{
}

void zCTouchAnimateSound::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteString(touchSoundName);
}

void zCTouchAnimateSound::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    if (touchSound) {
        touchSound->Release();
        touchSound = 0;
    }
    SetVobProperties();
    touchSoundName = archiver.ReadString();
}

zSTRING zCTouchAnimateSound::GetSoundName()
{
    return touchSoundName;
}

zCVobAnimate::~zCVobAnimate()
{
}

void zCVobAnimate::SetVisual(zCVisual* newVisual)
{
    zCVob::SetVisual(newVisual);
    if (startOn || isRunning)
        StartAni();
}

void zCVobAnimate::StartAni()
{
    isRunning = 1;
    zCVisualAnimate* animatedVisual = visual ? dynamic_cast<zCVisualAnimate*>(visual) : 0;
    if (animatedVisual && animatedVisual->IsAniActive())
        animatedVisual->StartAni();
}

void zCVobAnimate::StopAni()
{
    isRunning = 0;
}

void zCVobAnimate::OnTrigger(zCVob*, zCVob*)
{
    StartAni();
}

void zCVobAnimate::OnUntrigger(zCVob*, zCVob*)
{
    StopAni();
}

void zCVobAnimate::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteBool(startOn);
    if (archiver.InSaveGame())
        archiver.WriteBool(isRunning);
}

void zCVobAnimate::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    isRunning = 0;
    startOn = archiver.ReadBool();
    if (archiver.InSaveGame())
        isRunning = archiver.ReadBool();
    if ((archiver.InSaveGame() && isRunning) || startOn)
        StartAni();
}


zCVobLensFlare::~zCVobLensFlare()
{
    lensFlareFX = 0;
}

int zCVobLensFlare::Render(zTRenderContext& context)
{
    const int result = zCVob::Render(context);
    if (lensFlareFX)
        lensFlareFX->RenderLensFlares(this);
    return result;
}

void zCVobLensFlare::SetLensFlareFXByName(const zSTRING& name)
{
    if (lensFlareFX) {
        lensFlareFX->Release();
        lensFlareFX = 0;
    }
    if (name.Length() > 0)
        lensFlareFX = static_cast<zCLensFlareFX*>(zCLensFlareFX::classDef.SearchHashTable(name));
}

void zCVobLensFlare::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteString(lensFlareFX ? lensFlareFX->GetObjectName() : zSTRING());
}

void zCVobLensFlare::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    const zSTRING name = archiver.ReadString();
    if (lensFlareFX) {
        lensFlareFX->Release();
        lensFlareFX = 0;
    }
    if (name.Length() > 0)
        lensFlareFX = static_cast<zCLensFlareFX*>(zCLensFlareFX::classDef.SearchHashTable(name));
    castDynShadow = (castDynShadow & 0xfe) ^ 1;
}

zCEventScreenFX::zCEventScreenFX(zTEventScreenFXSubType eventSubType, float eventDuration,
    zCOLOR eventColor, float eventFovDeg)
    : duration(eventDuration), color(eventColor), fovDeg(eventFovDeg)
{
    subType = eventSubType;
}

void zCEventScreenFX::Clear()
{
    duration = 2.0f;
    color.r = 0;
    color.g = 0;
    color.b = 0;
    color.a = 255;
    fovDeg = 0.0f;
}

zSTRING zCEventScreenFX::MD_GetSubTypeString(int eventSubType)
{
    static const char* names[] = {
        "BLEND_FADE_IN", "BLEND_FADE_OUT", "CINEMA_FADE_IN", "CINEMA_FADE_OUT", "FOV_MORPH"
    };
    if (eventSubType < 0 || eventSubType >= zEVENT_SCREENFX_COUNT)
        return zSTRING();
    return zSTRING(names[eventSubType]);
}

int zCEventScreenFX::MD_GetTimeBehavior()
{
    return duration >= 0.0f;
}

float zCEventScreenFX::MD_GetMinTime()
{
    return duration;
}

void zCEventScreenFX::Pack(zCBuffer& buffer, zCEventManager*)
{
    buffer.WriteFloat(duration, 32);
    if (subType != zEVENT_FOV_MORPH)
        buffer.Write(&color, sizeof(color));
    else
        buffer.WriteFloat(fovDeg, 32);
}

void zCEventScreenFX::Unpack(zCBuffer& buffer, zCEventManager*)
{
    duration = buffer.ReadFloat(32);
    if (subType != zEVENT_FOV_MORPH)
        buffer.Read(&color, sizeof(color));
    else
        fovDeg = buffer.ReadFloat(32);
}

void zCEventScreenFX::Archive(zCArchiver& archiver)
{
    zCEventMessage::Archive(archiver);
    archiver.WriteFloat(duration);
    archiver.WriteColor(color);
    archiver.WriteFloat(fovDeg);
}

void zCEventScreenFX::Unarchive(zCArchiver& archiver)
{
    zCEventMessage::Unarchive(archiver);
    duration = archiver.ReadFloat();
    color = archiver.ReadColor();
    fovDeg = archiver.ReadFloat();
}


zCVobScreenFX::zCVobScreenFX()
{
    zTScreenFXSet* effects[] = { &blend, &cinema, &fovMorph };
    for (int i = 0; i < 3; ++i) {
        effects[i]->weight = 0.0f;
        effects[i]->velo = 0.0f;
        effects[i]->color.r = 0;
        effects[i]->color.g = 0;
        effects[i]->color.b = 0;
        effects[i]->color.a = 255;
        effects[i]->fovDeg = 0.0f;
        effects[i]->visible = 0;
        effects[i]->texAniFPS = 0.0f;
        effects[i]->blendFunc = zRND_ALPHA_FUNC_BLEND;
    }
    for (int i = 0; i < 2; ++i) {
        fovSaved.n[i] = 0.0f;
        fovSavedFirst.n[i] = 0.0f;
    }
}

zCClassDef* zCVobScreenFX::_GetClassDef() const
{
    return classDef;
}

zCVobScreenFX::zTScreenFXSet::~zTScreenFXSet()
{
}

zCVobScreenFX::~zCVobScreenFX()
{
}

void zCVobScreenFX::OnMessage(zCEventMessage* message, zCVob*)
{
    zCEventScreenFX* event = dynamic_cast<zCEventScreenFX*>(message);
    if (!event)
        return;
    switch (event->subType) {
    case zCEventScreenFX::zEVENT_BLEND_FADEIN:
        blend.weight = 1.0f;
        StartEffect(event->duration, event->color, 1, blend);
        break;
    case zCEventScreenFX::zEVENT_BLEND_FADEOUT:
        blend.weight = 0.0f;
        StartEffect(event->duration, event->color, 0, blend);
        break;
    case zCEventScreenFX::zEVENT_CINEMA_FADEIN:
        cinema.weight = 1.0f;
        StartEffect(event->duration, event->color, 1, cinema);
        break;
    case zCEventScreenFX::zEVENT_CINEMA_FADEOUT:
        cinema.weight = 0.0f;
        StartEffect(event->duration, event->color, 0, cinema);
        break;
    case zCEventScreenFX::zEVENT_FOV_MORPH:
        fovMorph.weight = 0.0f;
        StartEffect(event->duration, event->color, 0, fovMorph);
        fovMorph.fovDeg = event->fovDeg;
        break;
    }
}

void zCVobScreenFX::StartEffect(float duration, zCOLOR color, int endState, zTScreenFXSet& effect)
{
    const float speed = duration > 0.0f ? 1.0f / (duration * 1000.0f) : 999999.0f;
    effect.weight = endState;
    effect.velo = endState ? -speed : speed;
    effect.color = color;
    effect.visible = 1;
    SetSleepingMode(1);
    OnTick();
}

zCOLOR zCVobScreenFX::CalcEffectColor(zTScreenFXSet& effect)
{
    effect.weight += ztimer.frameTimeFloat * effect.velo;
    if (effect.weight < 0.0f) {
        effect.weight = 0.0f;
        effect.visible = 0;
    }
    else if (effect.weight > 1.0f) {
        effect.weight = 1.0f;
        effect.visible = 1;
    }
    zCOLOR result = effect.color;
    result.a = static_cast<unsigned char>(effect.color.a * effect.weight);
    return result;
}

void zCVobScreenFX::OnTick()
{
    if (zCCamera::activeCam) {
        if (blend.visible) {
            const zCOLOR color = CalcEffectColor(blend);
            zCCamera::activeCam->SetRenderScreenFade(color);
            zCCamera::activeCam->SetRenderScreenFadeTex(blend.texName);
            zCCamera::activeCam->SetRenderScreenFadeTexAniFPS(blend.texAniFPS);
            zCCamera::activeCam->SetRenderScreenFadeTexBlendFunc(blend.blendFunc);
        }
        if (cinema.visible) {
            const zCOLOR color = CalcEffectColor(cinema);
            zCCamera::activeCam->SetRenderCinemaScope(color);
        }
        if (fovMorph.visible) {
            if (fovMorph.weight == 0.0f) {
                zCCamera::activeCam->GetFOV(fovSaved.n[0], fovSaved.n[1]);
                if (fovSavedFirst.n[0] == 0.0f)
                    fovSavedFirst = fovSaved;
            }
            if (fovMorph.fovDeg == 0.0f)
                fovMorph.fovDeg = fovSavedFirst.n[0];
            CalcEffectColor(fovMorph);
            const float fovX = fovSavedFirst.n[0] +
                (fovMorph.fovDeg - fovSavedFirst.n[0]) * fovMorph.weight;
            const float fovY = fovSavedFirst.n[1] +
                (fovSavedFirst.n[1] / fovSavedFirst.n[0] * fovMorph.fovDeg - fovSavedFirst.n[1]) *
                    fovMorph.weight;
            zCCamera::activeCam->SetFOV(fovX, fovY);
        }
    }
    if (!blend.visible && !cinema.visible && !fovMorph.visible)
        SetSleeping(1);
}

void zCVobScreenFX::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    if (!archiver.InSaveGame())
        return;
    archiver.WriteRaw(&blend, sizeof(blend));
    archiver.WriteRaw(&cinema, sizeof(cinema));
    archiver.WriteRaw(&fovMorph, sizeof(fovMorph));
    archiver.WriteRawFloat(fovSaved.n, sizeof(fovSaved));
    archiver.WriteRawFloat(fovSavedFirst.n, sizeof(fovSavedFirst));
}

void zCVobScreenFX::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    if (!archiver.InSaveGame())
        return;
    archiver.ReadRaw(&blend, sizeof(blend));
    archiver.ReadRaw(&cinema, sizeof(cinema));
    archiver.ReadRaw(&fovMorph, sizeof(fovMorph));
    archiver.ReadRawFloat(fovSaved.n, sizeof(fovSaved));
    archiver.ReadRawFloat(fovSavedFirst.n, sizeof(fovSavedFirst));
}


zCMessageFilter::~zCMessageFilter()
{
}

void zCMessageFilter::ProcessMessage(zTMessageType message, zCVob* other)
{
    if (message == MT_TRIGGER)
        return;
    zCArray<zCVob*> targets;
    homeWorld->SearchVobListByName(triggerTarget, targets);
    for (int i = 0; i < targets.numInArray; ++i) {
        zCEventManager* manager = targets.parray[i]->GetEM(0);
        if (message == MT_UNTRIGGER)
            manager->OnUntrigger(this, other);
        else {
            zCEventCommon* event = new zCEventCommon;
            if (message == MT_ENABLE)
                event->subType = zCEventCommon::zEVENT_ENABLE;
            else if (message == MT_DISABLE)
                event->subType = zCEventCommon::zEVENT_DISABLE;
            else if (message == MT_TOGGLE_ENABLED)
                event->subType = zCEventCommon::zEVENT_TOGGLE_ENABLED;
            else if (message == MT_RESET)
                event->subType = zCEventCommon::zEVENT_RESET;
            manager->OnMessage(event, this);
        }
    }
}

void zCMessageFilter::OnTrigger(zCVob*, zCVob* instigator)
{
    ProcessMessage(static_cast<zTMessageType>(sendWhenTriggered), instigator);
}

void zCMessageFilter::OnUntrigger(zCVob*, zCVob* instigator)
{
    ProcessMessage(static_cast<zTMessageType>(sendWhenUntriggered), instigator);
}

void zCMessageFilter::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteString(triggerTarget);
    archiver.WriteEnum(sendWhenTriggered);
    archiver.WriteEnum(sendWhenUntriggered);
}

void zCMessageFilter::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    triggerTarget = archiver.ReadString();
    triggerTarget.Upper();
    sendWhenTriggered = archiver.ReadEnum();
    sendWhenUntriggered = archiver.ReadEnum();
}

void zCTriggerUntouch::OnUntouch(zCVob* other)
{
    if (other->GetCharacterClass() != 0)
        return;
    int shouldTrigger = 0;
    for (int i = 0; i < touchVobList.numInArray; ++i) {
        if (touchVobList.parray[i]->GetCharacterClass() == 0 || touchVobList.parray[i]->type == 129) {
            shouldTrigger = 1;
            break;
        }
    }
    if (!shouldTrigger)
        return;
    zCArray<zCVob*> targets;
    homeWorld->SearchVobListByName(triggerTarget, targets);
    for (int i = 0; i < targets.numInArray; ++i)
        targets.parray[i]->GetEM(0)->OnTrigger(this, other);
}

zCCodeMaster::~zCCodeMaster()
{
    Reset();
}

void zCCodeMaster::Init()
{
    for (int i = 0; i < 6; ++i)
        slaveVobNameList[i].Clear();
    triggerTargetFailure.Clear();
    orderRelevant = 0;
    untriggerCancels = 0;
    firstFalseIsFailure = 0;
    Reset();
}

void zCCodeMaster::Reset()
{
    for (int i = 0; i < 6; ++i) {
        if (slaveTriggeredList[i]) {
            slaveTriggeredList[i]->Release();
            slaveTriggeredList[i] = 0;
        }
    }
    numSlavesTriggered = 0;
}

void zCCodeMaster::FireTriggerSuccess(zCVob* other)
{
    zCArray<zCVob*> targets;
    homeWorld->SearchVobListByName(triggerTarget, targets);
    for (int i = 0; i < targets.numInArray; ++i)
        targets.parray[i]->GetEM(0)->OnTrigger(this, other);
    Reset();
}

void zCCodeMaster::FireTriggerFailure(zCVob* other)
{
    zCArray<zCVob*> targets;
    homeWorld->SearchVobListByName(triggerTargetFailure, targets);
    for (int i = 0; i < targets.numInArray; ++i)
        targets.parray[i]->GetEM(0)->OnTrigger(this, other);
    Reset();
}

int zCCodeMaster::GetSlaveNr(zCVob* other)
{
    if (!other || other->GetObjectName().Length() == 0)
        return -1;
    for (int i = 0; i < 6; ++i) {
        if (slaveVobNameList[i] == other->GetObjectName())
            return i;
    }
    return -1;
}


void zCCodeMaster::OnTrigger(zCVob* other, zCVob*)
{
    const int slaveNr = GetSlaveNr(other);
    if (slaveNr < 0)
        return;
    if (!orderRelevant) {
        other->AddRef();
        if (slaveTriggeredList[slaveNr])
            slaveTriggeredList[slaveNr]->Release();
        slaveTriggeredList[slaveNr] = other;
        for (int i = 0; i < 6; ++i) {
            if (!slaveTriggeredList[i] && slaveVobNameList[i].Length() > 0)
                return;
        }
        FireTriggerSuccess(other);
        return;
    }
    slaveTriggeredList[numSlavesTriggered] = other;
    other->AddRef();
    int orderCorrect = 1;
    for (int i = 0; i <= numSlavesTriggered; ++i) {
        if (!slaveTriggeredList[i] || slaveTriggeredList[i]->GetObjectName() != slaveVobNameList[i]) {
            orderCorrect = 0;
            break;
        }
    }
    int numSlaves = 0;
    for (int i = 0; i < 6; ++i) {
        if (slaveVobNameList[i].Length() > 0)
            ++numSlaves;
    }
    ++numSlavesTriggered;
    if (!orderCorrect) {
        if (numSlavesTriggered == numSlaves || firstFalseIsFailure)
            FireTriggerFailure(other);
    }
    else if (numSlavesTriggered == numSlaves)
        FireTriggerSuccess(other);
    if (numSlavesTriggered >= 6)
        numSlavesTriggered = 0;
}

void zCCodeMaster::OnUntrigger(zCVob* other, zCVob*)
{
    if (orderRelevant || !untriggerCancels || !other)
        return;
    const int slaveNr = GetSlaveNr(other);
    if (slaveNr >= 0 && slaveTriggeredList[slaveNr]) {
        slaveTriggeredList[slaveNr]->Release();
        slaveTriggeredList[slaveNr] = 0;
    }
}

void zCCodeMaster::OnMessage(zCEventMessage* message, zCVob*)
{
    zCEventCommon* common = dynamic_cast<zCEventCommon*>(message);
    if (common && common->subType == zCEventCommon::zEVENT_RESET)
        Reset();
}

const zSTRING* zCCodeMaster::GetTriggerTarget(int index) const
{
    if (index == 0)
        return &triggerTarget;
    if (index == 1)
        return &triggerTargetFailure;
    return 0;
}

void zCCodeMaster::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteString(triggerTarget);
    archiver.WriteGroupBegin("CodeMaster");
    archiver.WriteBool(orderRelevant);
    archiver.WriteGroupBegin("Failure");
    archiver.WriteBool(firstFalseIsFailure);
    archiver.WriteString(triggerTargetFailure);
    archiver.WriteGroupEnd();
    archiver.WriteGroupBegin("Untrigger");
    archiver.WriteBool(untriggerCancels);
    archiver.WriteGroupEnd();
    int count = 6;
    if (!archiver.InProperties()) {
        while (count > 0 && slaveVobNameList[count - 1].Length() == 0)
            --count;
        archiver.WriteByte(count);
    }
    for (int i = 0; i < count; ++i)
        archiver.WriteString(slaveVobNameList[i]);
    if (archiver.InSaveGame()) {
        archiver.WriteByte(numSlavesTriggered);
        for (int i = 0; i < count; ++i)
            archiver.WriteObject(slaveTriggeredList[i]);
    }
    archiver.WriteGroupEnd();
}

void zCCodeMaster::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    triggerTarget = archiver.ReadString();
    triggerTarget.Upper();
    Init();
    orderRelevant = archiver.ReadBool();
    firstFalseIsFailure = archiver.ReadBool();
    triggerTargetFailure = archiver.ReadString();
    triggerTargetFailure.Upper();
    untriggerCancels = archiver.ReadBool();
    int count = 6;
    if (!archiver.InProperties())
        count = archiver.ReadByte();
    for (int i = 0; i < count; ++i) {
        slaveVobNameList[i] = archiver.ReadString();
        slaveVobNameList[i].Upper();
    }
    if (archiver.InSaveGame()) {
        numSlavesTriggered = archiver.ReadByte();
        for (int i = 0; i < count; ++i) {
            zCObject* object = 0;
            archiver.ReadObject(object);
            slaveTriggeredList[i] = static_cast<zCVob*>(object);
        }
    }
}

zCMoverControler::~zCMoverControler()
{
}

void zCMoverControler::OnTrigger(zCVob*, zCVob*)
{
    zCEventMover* event = new zCEventMover;
    event->subType = moverMessage;
    event->gotoFixedKeyframe = gotoFixedKeyframe;
    zCArray<zCVob*> targets;
    homeWorld->SearchVobListByName(triggerTarget, targets);
    for (int i = 0; i < targets.numInArray; ++i)
        targets.parray[i]->GetEM(0)->OnMessage(event, this);
}

void zCMoverControler::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteString(triggerTarget);
    if (!archiver.InProperties()) {
        archiver.WriteEnum(moverMessage);
    } else {
        zCEventMover event;
        event.subType = moverMessage;
        zSTRING subTypeNames;
        for (int i = 0; i < event.MD_GetNumOfSubTypes(); ++i) {
            if (i > 0)
                subTypeNames += ",";
            subTypeNames += event.MD_GetSubTypeString(i);
        }
        archiver.WriteString(subTypeNames);
    }
    archiver.WriteInt(gotoFixedKeyframe);
}

void zCMoverControler::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    triggerTarget = archiver.ReadString();
    triggerTarget.Upper();
    moverMessage = archiver.ReadEnum();
    gotoFixedKeyframe = archiver.ReadInt();
}


zCTriggerWorldStart::~zCTriggerWorldStart()
{
}

void zCTriggerWorldStart::PostLoad()
{
    if (!fireOnlyFirstTime || !hasFired)
        OnTrigger(this, this);
}

void zCTriggerWorldStart::OnTrigger(zCVob* other, zCVob* instigator)
{
    zCTriggerBase::OnTrigger(other, instigator);
    hasFired = 1;
}

void zCTriggerWorldStart::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteString(triggerTarget);
    archiver.WriteBool(fireOnlyFirstTime);
    if (archiver.InSaveGame())
        archiver.WriteBool(hasFired);
}

void zCTriggerWorldStart::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    triggerTarget = archiver.ReadString();
    triggerTarget.Upper();
    fireOnlyFirstTime = archiver.ReadBool();
    if (archiver.InSaveGame())
        hasFired = archiver.ReadBool();
}


zCVob* zCVob::GetAutoLinkParent(zCVob* vob)
{
    zCTree* node = vob->globalVobTreeNode;
    if (node && node->parent && node->parent->data) {
        zCMover* mover = dynamic_cast<zCMover*>(node->parent->data);
        if (mover && mover->autoLinkEnabled)
            return mover;
    }
    return 0;
}

int zCVob::CanAutoLinkWith(zCVob* vob, zCVob* other)
{
    const int canAutoLink = (vob->GetCharacterClass() != 0) != ((vob->castDynShadow & 8) != 0);
    if (!canAutoLink || GetAutoLinkParent(vob) ||
        (vob->castDynShadow & 0x80000010) != 0x80000000)
        return 0;
    zVEC3 origin;
    zVEC3 direction;
    for (int i = 0; i < 3; ++i) {
        origin.n[i] = vob->trafoObjToWorld.m[i][3];
        direction.n[i] = 0.0f;
    }
    origin.n[1] = vob->bbox3D.maxs.n[1];
    direction.n[1] = -99999.0f;
    zCVob* foundVob;
    if (other) {
        zTTraceRayReport report;
        foundVob = other->TraceRay(origin, direction, 0, report) ? report.foundVob : 0;
    } else {
        vob->homeWorld->TraceRayNearestHit(origin, direction, vob, 0x11);
        foundVob = vob->homeWorld->traceRayReport.foundVob;
    }
    if (!foundVob)
        return 1;
    zCMover* mover = dynamic_cast<zCMover*>(foundVob);
    return mover && mover->autoLinkEnabled;
}

void zCVob::CheckAutoLink(zCVob* vob, zCVob* other)
{
    if (!CanAutoLinkWith(vob, other))
        return;
    zVEC3 origin;
    zVEC3 direction;
    for (int i = 0; i < 3; ++i) {
        origin.n[i] = vob->trafoObjToWorld.m[i][3];
        direction.n[i] = 0.0f;
    }
    origin.n[1] = vob->bbox3D.maxs.n[1];
    direction.n[1] = -std::fabs(vob->bbox3D.maxs.n[1] - vob->bbox3D.mins.n[1]) * 3.0f;
    zCVob* foundVob;
    if (other) {
        zTTraceRayReport report;
        foundVob = other->TraceRay(origin, direction, 0, report) ? report.foundVob : 0;
    } else {
        vob->homeWorld->TraceRayNearestHit(origin, direction, vob, 0x11);
        foundVob = vob->homeWorld->traceRayReport.foundVob;
    }
    zCMover* mover = dynamic_cast<zCMover*>(foundVob);
    if (!mover || !mover->autoLinkEnabled)
        return;
    const unsigned long collisionFlags = vob->castDynShadow;
    vob->SetCollDetStat(0);
    vob->SetCollDetDyn(0);
    vob->homeWorld->MoveVobSubtreeTo(vob, mover);
    vob->SetCollDetDyn((collisionFlags >> 7) & 1);
    vob->SetCollDetStat((collisionFlags >> 6) & 1);
    if (!(collisionFlags & 0x10)) {
        vob->castDynShadow &= ~8;
        if (vob->rigidBody)
            vob->rigidBody->StopTransRot();
    }
}

void zCVob::CheckAutoUnlink(zCVob* vob)
{
    zCVob* parent = GetAutoLinkParent(vob);
    const int canAutoLink = (vob->GetCharacterClass() != 0) != ((vob->castDynShadow & 8) != 0);
    if (!canAutoLink || !parent || (vob->castDynShadow & 0x80000010) != 0x80000000)
        return;
    zVEC3 origin;
    zVEC3 direction;
    for (int i = 0; i < 3; ++i) {
        origin.n[i] = vob->trafoObjToWorld.m[i][3];
        direction.n[i] = 0.0f;
    }
    origin.n[1] = vob->bbox3D.maxs.n[1];
    direction.n[1] = -std::fabs(vob->bbox3D.maxs.n[1] - vob->bbox3D.mins.n[1]) * 1.5f;
    zTTraceRayReport report;
    if (parent->TraceRay(origin, direction, 0, report))
        return;
    const unsigned long collisionFlags = vob->castDynShadow;
    vob->SetCollDetStat(0);
    vob->SetCollDetDyn(0);
    vob->homeWorld->MoveVobSubtreeToWorldSpace(vob);
    vob->SetCollDetDyn((collisionFlags >> 7) & 1);
    vob->SetCollDetStat((collisionFlags >> 6) & 1);
}

void zCVob::CalcGroundPoly()
{
    zVEC3 origin;
    zVEC3 direction;
    for (int i = 0; i < 3; ++i) {
        origin.n[i] = (bbox3D.mins.n[i] + bbox3D.maxs.n[i]) * 0.5f;
        direction.n[i] = 0.0f;
    }
    direction.n[1] = -20000.0f;
    int foundGround = 0;
    if (groundPoly && groundPoly->material->matGroup != 5) {
        zVEC3 intersection;
        float distance;
        foundGround = groundPoly->CheckRayPolyIntersection(origin, direction, intersection, distance);
    }
    if (!foundGround) {
        homeWorld->TraceRayNearestHit(origin, direction, 0, 0x422);
        zCPolygon* foundPoly = homeWorld->traceRayReport.foundPoly;
        if (!foundPoly || foundPoly->polyPlane.normal.n[1] <= 0.0f) {
            origin.n[1] = bbox3D.maxs.n[1];
            homeWorld->TraceRayNearestHit(origin, direction, 0, 0x22);
            foundPoly = homeWorld->traceRayReport.foundPoly;
        }
        groundPoly = foundPoly;
    }
}

void zCVob::CalcWaterVob()
{
    if (GetCharacterClass() != 0 || (castDynShadow & 0x10))
        return;
    zVEC3 origin;
    zVEC3 direction;
    for (int i = 0; i < 3; ++i) {
        origin.n[i] = (bbox3D.mins.n[i] + bbox3D.maxs.n[i]) * 0.5f;
        direction.n[i] = 0.0f;
    }
    origin.n[1] = bbox3D.maxs.n[1];
    direction.n[1] = -20000.0f;
    homeWorld->TraceRayNearestHit(origin, direction, 0, 0x222);
    zCPolygon* foundPoly = homeWorld->traceRayReport.foundPoly;
    if (!foundPoly)
        castDynShadow &= ~0x20;
    else if (foundPoly->material->matGroup == 5)
        castDynShadow |= 0x20;
    else
        castDynShadow &= ~0x20;
}

void zCVob::SetCollTypeDefaultFromVisual()
{
    if (GetCharacterClass() == 0 || GetCharacterClass() == 1)
        return;
    const int supportedVisual = dynamic_cast<zCMesh*>(visual) || dynamic_cast<zCModel*>(visual) ||
        dynamic_cast<zCMorphMesh*>(visual) || dynamic_cast<zCProgMeshProto*>(visual);
    zCCollisionObjectClass* newClass = &zCCollObjectUndef::s_oCollObjClass;
    if (supportedVisual) {
        zTBBox3D box = visual->GetBBox3D();
        newClass = box.GetVolume() > 200000.0f ? &zCCollObjectComplex::s_oCollObjClass :
            &zCCollObjectPoint::s_oCollObjClass;
    }
    if (m_poCollisionObjectClass != newClass && m_poCollisionObject) {
        delete m_poCollisionObject;
        m_poCollisionObject = 0;
    }
    m_poCollisionObjectClass = newClass;
}


void zCVob::TouchMovement()
{
    if (isInMovementMode & 3)
        m_poCollisionObject->m_bHintTranslation |= 1;
}

const zMAT4& zCVob::GetNewTrafoObjToWorld() const
{
    return (isInMovementMode & 3) ? m_poCollisionObject->m_oTrafoNew : trafoObjToWorld;
}

zMAT4& zCVob::GetNewTrafoObjToWorld()
{
    return (isInMovementMode & 3) ? m_poCollisionObject->m_oTrafoNew : trafoObjToWorld;
}

void zCVob::SetNewTrafoObjToWorld(const zMAT4& trafo)
{
    if (isInMovementMode & 3)
        m_poCollisionObject->m_oTrafoNew = trafo;
}

void zCVob::SetBBox3DWorld(const zTBBox3D& box)
{
    if (visual)
        return;
    if (!(isInMovementMode & 3))
        BeginMovement();
    bbox3D = box;
    m_poCollisionObject->m_bHintTranslation |= 1;
    EndMovement(0);
}

void zCVob::SetBBox3DLocal(const zTBBox3D& box)
{
    if (visual)
        return;
    if (!(isInMovementMode & 3))
        BeginMovement();
    bbox3D = box;
    for (int i = 0; i < 3; ++i) {
        bbox3D.mins.n[i] += trafoObjToWorld.m[i][3];
        bbox3D.maxs.n[i] += trafoObjToWorld.m[i][3];
    }
    m_poCollisionObject->m_bHintTranslation |= 1;
    EndMovement(0);
}

zTBBox3D zCVob::GetBBox3DLocal() const
{
    if (visual && visual->IsBBox3DLocal())
        return visual->GetBBox3D();
    zTBBox3D box = bbox3D;
    for (int i = 0; i < 3; ++i) {
        box.mins.n[i] -= trafoObjToWorld.m[i][3];
        box.maxs.n[i] -= trafoObjToWorld.m[i][3];
    }
    return box;
}

void zCVob::Move(float x, float y, float z)
{
    if (!(isInMovementMode & 3))
        BeginMovement();
    m_poCollisionObject->m_oTrafoNew.m[0][3] += x;
    m_poCollisionObject->m_oTrafoNew.m[1][3] += y;
    m_poCollisionObject->m_oTrafoNew.m[2][3] += z;
    m_poCollisionObject->m_bHintTranslation |= 1;
    EndMovement(1);
}

void zCVob::MoveWorld(float x, float y, float z)
{
    Move(x, y, z);
}

void zCVob::MoveLocal(float x, float y, float z)
{
    if (!(isInMovementMode & 3))
        BeginMovement();
    const zMAT4& trafo = m_poCollisionObject->m_oTrafoNew;
    for (int i = 0; i < 3; ++i)
        m_poCollisionObject->m_oTrafoNew.m[i][3] +=
            x * trafo.m[i][0] + y * trafo.m[i][1] + z * trafo.m[i][2];
    m_poCollisionObject->m_bHintTranslation |= 1;
    EndMovement(1);
}


void zCVob::SetRotationWorld(const zCQuat& rotation)
{
    if (!(isInMovementMode & 3))
        BeginMovement();
    rotation.QuatToMatrix4(m_poCollisionObject->m_oTrafoNew);
    m_poCollisionObject->m_bHintTranslation |= 2;
    EndMovement(1);
}

void zCVob::RotateWorld(const zVEC3& axis, float angle)
{
    if (angle == 0.0f)
        return;
    if (!(isInMovementMode & 3))
        BeginMovement();
    zMAT4& trafo = m_poCollisionObject->m_oTrafoNew;
    const float x = trafo.m[0][3];
    const float y = trafo.m[1][3];
    const float z = trafo.m[2][3];
    trafo.m[0][3] = 0.0f;
    trafo.m[1][3] = 0.0f;
    trafo.m[2][3] = 0.0f;
    trafo = Alg_Rotation3D(axis, angle) * trafo;
    trafo.m[0][3] = x;
    trafo.m[1][3] = y;
    trafo.m[2][3] = z;
    m_poCollisionObject->m_bHintTranslation |= 2;
    EndMovement(1);
}

void zCVob::RotateLocal(const zVEC3& axis, float angle)
{
    if (angle == 0.0f)
        return;
    if (!(isInMovementMode & 3))
        BeginMovement();
    m_poCollisionObject->m_oTrafoNew = m_poCollisionObject->m_oTrafoNew * Alg_Rotation3D(axis, angle);
    m_poCollisionObject->m_bHintTranslation |= 2;
    EndMovement(1);
}

void zCVob::RotateLocalX(float angle)
{
    if (angle == 0.0f)
        return;
    if (!(isInMovementMode & 3))
        BeginMovement();
    m_poCollisionObject->m_oTrafoNew.PostRotateX(angle);
    m_poCollisionObject->m_bHintTranslation |= 2;
    EndMovement(1);
}

void zCVob::RotateLocalY(float angle)
{
    if (angle == 0.0f)
        return;
    if (!(isInMovementMode & 3))
        BeginMovement();
    m_poCollisionObject->m_oTrafoNew.PostRotateY(angle);
    m_poCollisionObject->m_bHintTranslation |= 2;
    EndMovement(1);
}

void zCVob::RotateLocalZ(float angle)
{
    if (angle == 0.0f)
        return;
    if (!(isInMovementMode & 3))
        BeginMovement();
    m_poCollisionObject->m_oTrafoNew.PostRotateZ(angle);
    m_poCollisionObject->m_bHintTranslation |= 2;
    EndMovement(1);
}

void zCVob::RotateWorldX(float angle)
{
    zVEC3 axis = { 1.0f, 0.0f, 0.0f };
    RotateWorld(axis, angle);
}

void zCVob::RotateWorldY(float angle)
{
    zVEC3 axis = { 0.0f, 1.0f, 0.0f };
    RotateWorld(axis, angle);
}

void zCVob::RotateWorldZ(float angle)
{
    zVEC3 axis = { 0.0f, 0.0f, 1.0f };
    RotateWorld(axis, angle);
}

void zCVob::GetPositionWorld(float& x, float& y, float& z) const
{
    x = trafoObjToWorld.m[0][3];
    y = trafoObjToWorld.m[1][3];
    z = trafoObjToWorld.m[2][3];
}

void zCVob::GetPositionLocal(float& x, float& y, float& z) const
{
    if (!trafo)
        CreateTrafoLocal();
    x = trafo->m[0][3];
    y = trafo->m[1][3];
    z = trafo->m[2][3];
}

void zCVob::CreateTrafoLocal() const
{
    if (!trafo)
        trafo = new zMAT4;
    if (globalVobTreeNode && globalVobTreeNode->parent && globalVobTreeNode->parent->data)
        *trafo = globalVobTreeNode->parent->data->trafoObjToWorld.InverseLinTrafo() * trafoObjToWorld;
    else
        *trafo = trafoObjToWorld;
}


float zCVob::GetDistanceToVob(zCVob& other) const
{
    float distance = 0.0f;
    for (int i = 0; i < 3; ++i) {
        const float delta = trafoObjToWorld.m[i][3] - other.trafoObjToWorld.m[i][3];
        distance += delta * delta;
    }
    return std::sqrt(distance);
}

float zCVob::GetDistanceToVobApprox(zCVob& other) const
{
    float x = std::fabs(trafoObjToWorld.m[0][3] - other.trafoObjToWorld.m[0][3]);
    float y = std::fabs(trafoObjToWorld.m[1][3] - other.trafoObjToWorld.m[1][3]);
    float z = std::fabs(trafoObjToWorld.m[2][3] - other.trafoObjToWorld.m[2][3]);
    if (x < y) {
        const float swap = x;
        x = y;
        y = swap;
    }
    if (x < z) {
        const float swap = x;
        x = z;
        z = swap;
    }
    return x - x * 0.0625f + (y + z) * 0.375f;
}

float zCVob::GetDistanceToVob2(zCVob& other) const
{
    float distance = 0.0f;
    for (int i = 0; i < 3; ++i) {
        const float delta = trafoObjToWorld.m[i][3] - other.trafoObjToWorld.m[i][3];
        distance += delta * delta;
    }
    return distance;
}

void zCVob::SetPositionLocal(const zVEC3& position)
{
    if (!(isInMovementMode & 3))
        BeginMovement();
    const zMAT4 inverse = m_poCollisionObject->m_oTrafoNew.Inverse();
    for (int i = 0; i < 3; ++i) {
        m_poCollisionObject->m_oTrafoNew.m[i][3] = inverse.m[i][3] +
            inverse.m[i][0] * position.n[0] + inverse.m[i][1] * position.n[1] +
            inverse.m[i][2] * position.n[2];
    }
    m_poCollisionObject->m_bHintTranslation |= 1;
    EndMovement(1);
}

void zCVob::SetPositionWorld(const zVEC3& position)
{
    if (!(isInMovementMode & 3))
        BeginMovement();
    for (int i = 0; i < 3; ++i)
        m_poCollisionObject->m_oTrafoNew.m[i][3] = position.n[i];
    m_poCollisionObject->m_bHintTranslation |= 1;
    EndMovement(1);
}

void zCVob::SetTrafo(const zMAT4& newTrafo)
{
    if (!(isInMovementMode & 3))
        BeginMovement();
    if (globalVobTreeNode && globalVobTreeNode->parent && globalVobTreeNode->parent->data)
        m_poCollisionObject->m_oTrafoNew = globalVobTreeNode->parent->data->trafoObjToWorld * newTrafo;
    else
        m_poCollisionObject->m_oTrafoNew = newTrafo;
    m_poCollisionObject->m_bHintTranslation |= 3;
    EndMovement(1);
}

void zCVob::SetTrafoObjToWorld(const zMAT4& newTrafo)
{
    if (!(isInMovementMode & 3))
        BeginMovement();
    m_poCollisionObject->m_oTrafoNew = newTrafo;
    m_poCollisionObject->m_bHintTranslation |= 3;
    EndMovement(1);
}

void zCVob::ResetRotationsLocal()
{
    if (!(isInMovementMode & 3))
        BeginMovement();
    zMAT4& trafo = m_poCollisionObject->m_oTrafoNew;
    const float x = trafo.m[0][3];
    const float y = trafo.m[1][3];
    const float z = trafo.m[2][3];
    if (globalVobTreeNode && globalVobTreeNode->parent && globalVobTreeNode->parent->data)
        trafo = globalVobTreeNode->parent->data->trafoObjToWorld;
    else {
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column)
                trafo.m[row][column] = row == column ? 1.0f : 0.0f;
        }
    }
    trafo.m[0][3] = x;
    trafo.m[1][3] = y;
    trafo.m[2][3] = z;
    m_poCollisionObject->m_bHintTranslation |= 2;
    EndMovement(1);
}

void zCVob::ResetXZRotationsLocal()
{
    const int wasInMovement = isInMovementMode & 3;
    if (!wasInMovement)
        BeginMovement();
    if (!globalVobTreeNode || !globalVobTreeNode->parent || !globalVobTreeNode->parent->data) {
        ResetXZRotationsWorld();
    } else {
        const zMAT4 parentToLocal = globalVobTreeNode->parent->data->trafoObjToWorld.Inverse();
        zMAT4& trafo = m_poCollisionObject->m_oTrafoNew;
        zVEC3 target;
        zVEC3 position;
        for (int i = 0; i < 3; ++i) {
            target.n[i] = trafo.m[i][3] + trafo.m[i][2] * 1000.0f;
            position.n[i] = trafo.m[i][3];
        }
        target = parentToLocal * target;
        zVEC3 direction = target - position;
        const float length = direction.Length();
        if (length > 0.0f) {
            direction /= length;
            if (!(isInMovementMode & 3))
                BeginMovement();
            SetHeadingAtWorld(direction);
            EndMovement(1);
        }
    }
    if (!wasInMovement)
        EndMovement(1);
}


void zCVob::ResetRotationsWorld()
{
    if (!(isInMovementMode & 3))
        BeginMovement();
    zMAT4& trafo = m_poCollisionObject->m_oTrafoNew;
    const float x = trafo.m[0][3];
    const float y = trafo.m[1][3];
    const float z = trafo.m[2][3];
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column)
            trafo.m[row][column] = row == column ? 1.0f : 0.0f;
    }
    trafo.m[0][3] = x;
    trafo.m[1][3] = y;
    trafo.m[2][3] = z;
    m_poCollisionObject->m_bHintTranslation |= 2;
    EndMovement(1);
}

void zCVob::ResetXZRotationsWorld()
{
    if (!(isInMovementMode & 3))
        BeginMovement();
    zVEC3 direction;
    float length = 0.0f;
    for (int i = 0; i < 3; ++i) {
        direction.n[i] = m_poCollisionObject->m_oTrafoNew.m[i][2];
        length += direction.n[i] * direction.n[i];
    }
    if (length > 0.0f) {
        length = std::sqrt(length);
        for (int i = 0; i < 3; ++i)
            direction.n[i] /= length;
        SetHeadingAtWorld(direction);
        EndMovement(1);
    }
}

void zCVob::SetHeadingYLocal(const zVEC3& target)
{
    zVEC3 direction;
    float length = 0.0f;
    for (int i = 0; i < 3; ++i) {
        direction.n[i] = target.n[i] - m_poCollisionObject->m_oTrafoNew.m[i][3];
        length += direction.n[i] * direction.n[i];
    }
    if (length > 0.0f) {
        length = std::sqrt(length);
        for (int i = 0; i < 3; ++i)
            direction.n[i] /= length;
        SetHeadingAtLocal(direction);
    }
    ResetXZRotationsLocal();
}

void zCVob::SetHeadingYWorld(const zVEC3& target)
{
    zVEC3 direction;
    float length = 0.0f;
    for (int i = 0; i < 3; ++i) {
        direction.n[i] = target.n[i] - m_poCollisionObject->m_oTrafoNew.m[i][3];
        length += direction.n[i] * direction.n[i];
    }
    if (length > 0.0f) {
        length = std::sqrt(length);
        for (int i = 0; i < 3; ++i)
            direction.n[i] /= length;
        SetHeadingAtWorld(direction);
    }
}

void zCVob::SetHeadingYWorld(zCVob* target)
{
    zVEC3 position;
    for (int i = 0; i < 3; ++i)
        position.n[i] = target->trafoObjToWorld.m[i][3];
    SetHeadingYWorld(position);
}

void zCVob::SetHeadingLocal(const zVEC3& target)
{
    zVEC3 direction;
    float length = 0.0f;
    for (int i = 0; i < 3; ++i) {
        direction.n[i] = target.n[i] - m_poCollisionObject->m_oTrafoNew.m[i][3];
        length += direction.n[i] * direction.n[i];
    }
    if (length > 0.0f) {
        length = std::sqrt(length);
        for (int i = 0; i < 3; ++i)
            direction.n[i] /= length;
        SetHeadingAtLocal(direction);
    }
}

void zCVob::SetHeadingWorld(const zVEC3& target)
{
    zVEC3 direction;
    float length = 0.0f;
    for (int i = 0; i < 3; ++i) {
        direction.n[i] = target.n[i] - m_poCollisionObject->m_oTrafoNew.m[i][3];
        length += direction.n[i] * direction.n[i];
    }
    if (length > 0.0f) {
        length = std::sqrt(length);
        for (int i = 0; i < 3; ++i)
            direction.n[i] /= length;
        SetHeadingAtWorld(direction);
    }
}

void zCVob::SetHeadingWorld(zCVob* target)
{
    zVEC3 position;
    for (int i = 0; i < 3; ++i)
        position.n[i] = target->trafoObjToWorld.m[i][3];
    SetHeadingWorld(position);
}

void zCVob::SetHeadingAtLocal(const zVEC3& direction)
{
    if (HasParentVob()) {
        const zVEC3 worldDirection = globalVobTreeNode->parent->data->trafoObjToWorld.Rotate(direction);
        SetHeadingAtWorld(worldDirection);
        return;
    }
    SetHeadingAtWorld(direction);
}

void zCVob::SetHeadingAtWorld(const zVEC3& direction)
{
    if (!(isInMovementMode & 3))
        BeginMovement();
    zVEC3 right;
    right.n[0] = direction.n[2];
    right.n[1] = 0.0f;
    right.n[2] = -direction.n[0];
    float length = std::sqrt(right.n[0] * right.n[0] + right.n[2] * right.n[2]);
    if (length < 0.000001f) {
        right.n[0] = 1.0f;
        right.n[2] = 0.0f;
        length = 1.0f;
    }
    right.n[0] /= length;
    right.n[2] /= length;
    zVEC3 up;
    up.n[0] = direction.n[1] * right.n[2] - direction.n[2] * right.n[1];
    up.n[1] = direction.n[2] * right.n[0] - direction.n[0] * right.n[2];
    up.n[2] = direction.n[0] * right.n[1] - direction.n[1] * right.n[0];
    zMAT4& trafo = m_poCollisionObject->m_oTrafoNew;
    for (int i = 0; i < 3; ++i) {
        trafo.m[i][0] = right.n[i];
        trafo.m[i][1] = up.n[i];
        trafo.m[i][2] = direction.n[i];
    }
    m_poCollisionObject->m_bHintTranslation |= 2;
    EndMovement(1);
}

int zCVob::HasParentVob() const
{
    return globalVobTreeNode && globalVobTreeNode->parent && globalVobTreeNode->parent->data;
}


void zCVob::SetCollDetStat(int enabled)
{
    castDynShadow = (castDynShadow & ~0x40) | ((enabled & 1) << 6);
}

void zCVob::SetCollDetDyn(int enabled)
{
    castDynShadow = (castDynShadow & ~0x80) | ((enabled & 1) << 7);
}

void zCVob::CorrectTrafo()
{
    if (!trafo)
        return;
    if (globalVobTreeNode && globalVobTreeNode->parent && globalVobTreeNode->parent->data)
        *trafo = globalVobTreeNode->parent->data->trafoObjToWorld.InverseLinTrafo() * trafoObjToWorld;
    else
        *trafo = trafoObjToWorld;
}

void zCVob::DoneWithTrafoLocal()
{
    if (trafo && (!globalVobTreeNode || !globalVobTreeNode->parent || !globalVobTreeNode->parent->data)) {
        trafoObjToWorld = *trafo;
        delete trafo;
        trafo = 0;
    }
}

void zCVob::SetPhysicsEnabled(int enabled)
{
    if (castDynShadow & 0x10)
        return;
    castDynShadow = (castDynShadow & ~8) | ((enabled & 1) << 3);
    if (rigidBody)
        rigidBody->StopTransRot();
}

zVEC3 zCVob::GetVelocity()
{
    zVEC3 velocity;
    if ((castDynShadow & 8) && rigidBody)
        return rigidBody->v;
    zCModel* modelVisual = dynamic_cast<zCModel*>(visual);
    if (modelVisual) {
        const zVEC3* source = modelVisual->GetVelocityRing();
        for (int i = 0; i < 3; ++i) {
            velocity.n[i] = source->n[0] * trafoObjToWorld.m[i][0] +
                source->n[1] * trafoObjToWorld.m[i][1] + source->n[2] * trafoObjToWorld.m[i][2];
        }
    }
    else {
        velocity.n[0] = 0.0f;
        velocity.n[1] = 0.0f;
        velocity.n[2] = 0.0f;
    }
    return velocity;
}

void zCVob::UpdatePhysics()
{
    if (!(castDynShadow & 8) || !rigidBody)
        return;
    for (int i = 0; i < 3; ++i)
        rigidBody->xPos.n[i] = trafoObjToWorld.m[i][3];
    rigidBody->RunSimulation();
    if (!(isInMovementMode & 3))
        BeginMovement();
    for (int i = 0; i < 3; ++i)
        m_poCollisionObject->m_oTrafoNew.m[i][3] = rigidBody->xPos.n[i];
    m_poCollisionObject->m_bHintTranslation |= 1;
    EndMovement(1);
}

int zCVob::DetectCollision(zMAT4* newTrafo)
{
    const int oldPhysics = (castDynShadow >> 3) & 1;
    const int oldSleeping = sleepingMode & 3;
    SetPhysicsEnabled(1);
    SetSleepingMode(1);
    BeginMovement();
    if (newTrafo) {
        m_poCollisionObject->m_oTrafoNew = *newTrafo;
        m_poCollisionObject->m_bHintTranslation |= 3;
        EndMovement(1);
    }
    const int result = IsColliding();
    if (newTrafo && (isInMovementMode & 3)) {
        m_poCollisionObject->m_oTrafoNew = trafoObjToWorld;
        m_poCollisionObject->m_bHintTranslation &= ~3;
    }
    EndMovement(1);
    SetPhysicsEnabled(oldPhysics);
    SetSleepingMode(oldSleeping);
    return result;
}


void zCVob::SetInMovement(int movement)
{
    isInMovementMode = movement;
    if (movement != 2 && (isInMovementMode & 3) != 2)
        s_vobAlreadyInMovement = movement == 1;
}

void zCVob::SetInMovementMode(int movementMode)
{
    isInMovementMode = movementMode;
    if (movementMode != 2)
        s_vobAlreadyInMovement = movementMode == 1;
}

void zCVob::BeginMovement()
{
    if ((isInMovementMode & 3) || (sleepingMode & 8))
        return;
    CreateCollisionObject();
    ResetCollisionObjectState();
    sleepingMode &= ~4;
    SetInMovementMode(1);
}

void zCVob::ResetToOldMovementState()
{
    if (isInMovementMode & 3) {
        m_poCollisionObject->m_oTrafoNew = trafoObjToWorld;
        m_poCollisionObject->m_bHintTranslation &= ~3;
    }
}

void zCVob::UpdateTouchList(const zCArray<zCVob*>& newTouchList)
{
    for (int i = 0; i < newTouchList.numInArray; ++i) {
        zCVob* other = newTouchList.parray[i];
        if (touchVobList.Search(other) == -1) {
            GetEM(0)->OnTouch(other);
            other->GetEM(0)->OnTouch(this);
        }
    }
    for (int i = 0; i < touchVobList.numInArray; ++i) {
        zCVob* other = touchVobList.parray[i];
        if (newTouchList.Search(other) == -1) {
            GetEM(0)->OnUntouch(other);
            other->GetEM(0)->OnUntouch(this);
        }
    }
    touchVobList = newTouchList;
}

void zCVob::CalcDestinationBBox3DWorld(zTBBox3D& destination)
{
    if (visual && visual->IsBBox3DLocal()) {
        zTBBox3D localBox = visual->GetBBox3D();
        localBox.Transform(m_poCollisionObject->m_oTrafoNew, destination);
        return;
    }
    destination = bbox3D;
    for (int i = 0; i < 3; ++i) {
        const float delta = m_poCollisionObject->m_oTrafoNew.m[i][3] -
            m_poCollisionObject->m_oTrafoOld.m[i][3];
        destination.mins.n[i] += delta;
        destination.maxs.n[i] += delta;
    }
}

void zCVob::AdoptCollObjResults()
{
    if (m_poCollisionObject->m_bHintTranslation & 3) {
        trafoObjToWorld = m_poCollisionObject->m_oTrafoNew;
        if (!(sleepingMode & 4))
            CorrectTrafo();
    }
    if (m_poCollisionObjectClass && m_poCollisionObjectClass->m_bIsVolatile)
        DestroyCollisionObject(0);
}

void zCVob::EndMovement(int flags)
{
    if (!(isInMovementMode & 3))
        return;
    sleepingMode |= 8;
    if (m_poCollisionObject->m_bHintTranslation & 3) {
        CheckAndResolveCollisions();
        CalcDestinationBBox3DWorld(bbox3D);
        AdoptCollObjResults();
        DestroyCollisionObject(0);
        SetInMovementMode(0);
        if (homeWorld)
            homeWorld->bspTree.AdjustVob(this);
        if (flags) {
            CalcGroundPoly();
            if (visual)
                CalcWaterVob();
        }
        CheckAutoUnlink(this);
    }
    else {
        DestroyCollisionObject(0);
        SetInMovementMode(0);
    }
    sleepingMode &= ~8;
}

void zCVob::SetCollisionObject(zCCollisionObject* object)
{
    delete m_poCollisionObject;
    m_poCollisionObject = object;
}

void zCVob::SetCollisionClass(zCCollisionObjectClass* objectClass)
{
    if (m_poCollisionObjectClass != objectClass && m_poCollisionObjectClass)
        DestroyCollisionObject(1);
    m_poCollisionObjectClass = objectClass;
}

zCCollisionObject* zCVob::GetCollisionObject() const
{
    return m_poCollisionObject;
}

void zCVob::CreateCollisionObject()
{
    if (m_poCollisionObject)
        return;
    m_poCollisionObject = m_poCollisionObjectClass->m_createNewInstance();
    m_poCollisionObject->m_pvClientData = this;
}

void zCVob::ResetCollisionObjectState()
{
    m_poCollisionObject->m_oTrafoOld = trafoObjToWorld;
    m_poCollisionObject->m_oTrafoNew = trafoObjToWorld;
    m_poCollisionObject->m_bHintTranslation &= ~3;
}

void zCVob::DestroyCollisionObject(int force)
{
    if (!m_poCollisionObjectClass || (!force && !m_poCollisionObjectClass->m_bIsVolatile))
        return;
    delete m_poCollisionObject;
    m_poCollisionObject = 0;
}

void zCVob::CheckEmergencyPutToSleep()
{
    if (!(castDynShadow & 8))
        return;
    float distance = 0.0f;
    for (int i = 0; i < 3; ++i) {
        const float delta = m_poCollisionObject->m_oTrafoNew.m[i][3] -
            m_poCollisionObject->m_oTrafoOld.m[i][3];
        distance += delta * delta;
    }
    if (distance < 1.0f && ++collButNoMove > 8) {
        SetPhysicsEnabled(0);
        if (callback_ai)
            SetSleepingMode(2);
        else
            SetSleeping(1);
        collButNoMove = 0;
    }
}


void zCVob::CollectCollisionContext_Vobs(const zTBBox3D& box, zTCollisionContext& context)
{
    if (!(castDynShadow & 0x80000000))
        return;
    zCArray<zCVob*> vobs;
    homeWorld->bspTree.bspRoot->CollectVobsInBBox3D(vobs, box);
    for (int i = 0; i < vobs.numInArray; ++i) {
        zCVob* other = vobs.parray[i];
        if (other == this || !(other->castDynShadow & 0x80000000))
            continue;
        other->CreateCollisionObject();
        other->ResetCollisionObjectState();
        context.otherCollObjectList.InsertEnd(other->m_poCollisionObject);
        context.otherCollVobList.InsertEnd(other);
        other->AddRef();
    }
}

void zCVob::CollectCollisionContext_Static(const zTBBox3D& box, zCArray<zCCollisionObject*>& objects)
{
    if (!(castDynShadow & 0x40))
        return;
    static zCCollObjectLevelPolys levelPolys;
    levelPolys.m_aoPolyList.numInArray = 0;
    zCArray<zCPolygon*> polys;
    int numPolys = 0;
    homeWorld->bspTree.bspRoot->CollectPolysInBBox3D(box, polys, numPolys);
    const int filterBackfaces = !(m_poCollisionObject->m_bHintTranslation & 2);
    zVEC3 movement;
    if (filterBackfaces) {
        for (int axis = 0; axis < 3; ++axis) {
            movement.n[axis] = m_poCollisionObject->m_oTrafoOld.m[axis][3] -
                m_poCollisionObject->m_oTrafoNew.m[axis][3];
        }
    }
    for (int i = 0; i < numPolys; ++i) {
        zCPolygon* poly = polys.parray[i];
        if (filterBackfaces && poly->polyPlane.normal * movement <= 0.0f)
            continue;
        if (!poly->CheckBBoxPolyIntersection(box) || poly->material->matGroup == 5 ||
            (poly->material->flags & 0x10))
            continue;
        levelPolys.m_aoPolyList.InsertEnd(poly);
    }
    if (levelPolys.m_aoPolyList.numInArray > 0)
        objects.InsertEnd(&levelPolys);
}

void zCVob::CollectCollisionContext(zTCollisionContext& context)
{
    zTBBox3D box;
    CalcDestinationBBox3DWorld(box);
    context.otherCollObjectList.numInArray = 0;
    context.otherCollVobList.numInArray = 0;
    CollectCollisionContext_Vobs(box, context);
    CollectCollisionContext_Static(box, context.otherCollObjectList);
}

void zCVob::CleanupCollisionContext(const zTCollisionContext& context)
{
    for (int i = 0; i < context.otherCollVobList.numInArray; ++i) {
        zCVob* other = context.otherCollVobList.parray[i];
        if (!(other->isInMovementMode & 3) && other->m_poCollisionObjectClass &&
            other->m_poCollisionObjectClass->m_bIsVolatile)
            other->DestroyCollisionObject(0);
        other->Release();
    }
}


void zCVob::NotifyCollisionPair(zCCollisionReport* report, const zCArray<zCCollisionReport*>& reports,
    int flags)
{
    zCVob* first = static_cast<zCVob*>(report->m_poCollObject0->m_pvClientData);
    zCVob* second = static_cast<zCVob*>(report->m_poCollObject1->m_pvClientData);
    if (report->m_poCollObject1->GetCollObjClass() == &zCCollObjectLevelPolys::s_oCollObjClass && first)
        first->GetEM(0)->OnTouchLevel();
    if (first && second) {
        zCVob* responding = first == this ? second : first;
        responding->m_poCollisionObject->CollisionResponse(const_cast<zCArray<zCCollisionReport*>&>(reports), flags);
        if (!flags) {
            first->GetEM(0)->OnTouch(second);
            second->GetEM(0)->OnTouch(first);
            second->GetEM(0)->OnUntouch(first);
            first->GetEM(0)->OnUntouch(second);
        }
    }
    if (first && first->callback_ai)
        first->callback_ai->ReportCollisionToAI(report);
    if (second && second->callback_ai)
        second->callback_ai->ReportCollisionToAI(report);
}

void zCVob::ExecuteCollisionResponse(const zCArray<zCCollisionReport*>& reports, int& flags)
{
    flags = 0;
    if (reports.numInArray == 0)
        return;
    reports.parray[0]->m_poCollObject0->CollisionResponse(const_cast<zCArray<zCCollisionReport*>&>(reports), flags);
    for (int i = 0; i < reports.numInArray; ++i)
        NotifyCollisionPair(reports.parray[i], reports, flags);
    sleepingMode &= ~4;
}

void zCVob::CheckAndResolveCollisions()
{
    if (!(castDynShadow & 0xc0))
        return;
    for (int iteration = 0; iteration < 3; ++iteration) {
        zTCollisionContext context;
        zCArray<zCCollisionReport*> reports;
        CollectCollisionContext(context);
        s_poCollisionDetector->DetectCollisions(context, reports);
        if (reports.numInArray > 0) {
            int flags = 0;
            ExecuteCollisionResponse(reports, flags);
        }
        UpdateTouchList(context.otherCollVobList);
        CleanupCollisionContext(context);
        if (reports.numInArray == 0)
            break;
    }
}

int zCVob::IsColliding()
{
    if (!(castDynShadow & 0xc0))
        return 0;
    zTCollisionContext context;
    zCArray<zCCollisionReport*> reports;
    CollectCollisionContext(context);
    s_poCollisionDetector->DetectCollisions(context, reports);
    const int result = reports.numInArray > 0;
    CleanupCollisionContext(context);
    return result;
}

void zCVob::SetStaticVob(int enabled)
{
    showVisual = ((enabled << 4 ^ showVisual) & 0x10) ^ showVisual;
}

zTVobType zCVob::GetVobType() const
{
    return (zTVobType)type;
}

void zCVob::SetDrawBBox3D(int enabled)
{
    showVisual = ((enabled << 1 ^ showVisual) & 2) ^ showVisual;
}

void zCVob::SetCollDet(int enabled)
{
    SetCollDetStat(enabled);
    SetCollDetDyn(enabled);
}

zCEffect::~zCEffect()
{
}

zCClassDef* zCEffect::_GetClassDef() const
{
    return classDef;
}

static zCClassDef zCVobWaypointClassDef("zCVobWaypoint", "zCVob",
    zCVobWaypoint::_CreateNewInstance, 0, 0x120, 0, "");
zCClassDef* zCVobWaypoint::classDef = &zCVobWaypointClassDef;

void zCVobWaypoint::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}
