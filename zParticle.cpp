#include "zParticle.h"

#include <cstdlib>
#include <cmath>

static int particleState = -1;
static int particleSystemState = -1;
static zCClassDef zCParticleFXClassDef("zCParticleFX", "zCVisual", NULL, 0, 0xB8, 0, "");
zCClassDef* zCParticleFX::classDef = &zCParticleFXClassDef;
zCParser* zCParticleFX::s_pfxParser;
extern zCParser* parserParticleFX;
zCArraySort<zCParticleEmitter*> zCParticleFX::s_emitterPresetList(16);
zCParticleEmitter zCParticleFX::s_emitterDummyDefault;
int zCParticleFX::s_bAmbientPFXEnabled = 1;
int zCParticleFX::s_globNumPart;
zSParticle* zCParticleFX::s_globPartList;
zSParticle* zCParticleFX::s_globFreePart;
zCMesh* zCParticleFX::s_partMeshTri;
zCMesh* zCParticleFX::s_partMeshQuad;
int zCParticleFX::s_showDebugInfo;
zCParticleFX::zCStaticPfxList zCParticleFX::s_pfxList;

struct zTIMER {
    float totalTimeFloat;
    float frameTimeFloat;
};

extern zTIMER ztimer;
static zCPFXPerFrameCallback pfxPerFrameCallback;

// EXE: 0x005AC430
zCObject* zCParticleFX::_CreateNewInstance()
{
    zCParticleFX* particle = new zCParticleFX;
    zCClassDef::ObjectCreated(particle, classDef);
    return particle;
}

void zCParticleFX::ParseParticleFXScript()
{
    if (!s_pfxParser)
        s_pfxParser = new zCParser;
    else
        s_pfxParser->Reset();
    s_pfxParser->Parse("System\\ParticleFx.src");
    s_pfxParser->CreatePCode();
    parserParticleFX = s_pfxParser;
}

void zCParticleFX::InitParticleFX()
{
    delete[] s_globPartList;
    s_globNumPart = 10000;
    s_globPartList = new zSParticle[s_globNumPart];
    for (int i = 0; i < s_globNumPart - 1; ++i)
        s_globPartList[i].next = &s_globPartList[i + 1];
    s_globPartList[s_globNumPart - 1].next = 0;
    s_globFreePart = s_globPartList;
    ParseParticleFXScript();
}

zCParticleFX::zCParticleFX()
    : firstPart(0), emitter(&s_emitterDummyDefault), connectedVob(0), bboxUpdateCtr(0),
      nextPfx(0), prevPfx(0), privateTotalTime(0), lastTimeRendered(0), timeScale(1.0f),
      localFrameTimeF(0), quadMark(0), m_BboxYRangeInv(0), m_bVisualNeverDies(0)
{
    emitterIsOwned = 0;
    dontKillPFXWhenDone = 0;
    dead = 0;
    isOneShotFX = 0;
    forceEveryFrameUpdate = 0;
    renderUnderWaterOnly = 0;
    for (int axis = 0; axis < 3; ++axis) {
        bbox3DWorld.mins.n[axis] = -10.0f;
        bbox3DWorld.maxs.n[axis] = 10.0f;
        quadMarkBBox3DWorld.mins.n[axis] = 0;
        quadMarkBBox3DWorld.maxs.n[axis] = 0;
    }
    InitEmitterVars();
}

void zCParticleFX::InitEmitterVars()
{
    emitterVars.ppsScaleKeysActFrame = 0;
    emitterVars.ppsNumParticlesFraction = 0;
    emitterVars.ppsTotalLifeTime = 0;
    emitterVars.ppsDependentEmitterCreated = 0;
    emitterVars.shpScaleKeysActFrame = 0;
    emitterVars.uniformValue = 0;
    emitterVars.uniformDelta = 0;
}

void zCParticleFX::CleanupParticleFX()
{
    if (s_partMeshTri) {
        s_partMeshTri->Release();
        s_partMeshTri = 0;
    }
    if (s_partMeshQuad) {
        s_partMeshQuad->Release();
        s_partMeshQuad = 0;
    }
    delete[] s_globPartList;
    s_globPartList = 0;
    s_globFreePart = 0;
    delete s_pfxParser;
    s_pfxParser = 0;
    parserParticleFX = 0;
    for (int i = 0; i < s_emitterPresetList.numInArray; ++i)
        delete s_emitterPresetList.array[i];
    s_emitterPresetList.numInArray = 0;
}

zCClassDef* zCParticleFX::_GetClassDef() const
{
    return classDef;
}

int zCParticleFX::IsBBox3DLocal()
{
    return 0;
}

int zCParticleFX::GetVisualDied()
{
    return dead;
}

unsigned long zCParticleFX::GetRenderSortKey() const
{
    return reinterpret_cast<unsigned long>(emitter);
}

zCParticleFX::~zCParticleFX()
{
    if (emitterIsOwned)
        delete emitter;
    emitter = &s_emitterDummyDefault;
    emitterIsOwned = 0;
    while (firstPart) {
        zSParticle* particle = firstPart;
        firstPart = particle->next;
        if (particle->polyStrip)
            particle->polyStrip->Release();
        particle->next = s_globFreePart;
        s_globFreePart = particle;
    }
    quadMark = 0;
    connectedVob = 0;
    s_pfxList.RemovePfx(this);
    emitter = 0;
}

void zCPFXPerFrameCallback::DoWorldPerFrameCallback(zCWorld*, zCCamera*)
{
    zCParticleFX::s_pfxList.ProcessList();
}

void zCParticleFX::HostVobAddedToWorld(zCVob*, zCWorld* world)
{
    world->RegisterPerFrameCallback(&pfxPerFrameCallback);
}

void zCParticleFX::HostVobRemovedFromWorld(zCVob*, zCWorld*)
{
}

void zCParticleFX::zCStaticPfxList::InsertPfxHead(zCParticleFX* pfx)
{
    pfx->prevPfx = 0;
    pfx->nextPfx = pfxListHead;
    if (!pfxListTail)
        pfxListTail = pfx;
    if (pfxListHead)
        pfxListHead->prevPfx = pfx;
    pfxListHead = pfx;
    ++numInList;
}

void zCParticleFX::zCStaticPfxList::RemovePfx(zCParticleFX* pfx)
{
    if (!IsInList(pfx))
        return;
    if (pfxListTail == pfx)
        pfxListTail = pfx->prevPfx;
    if (pfxListHead == pfx)
        pfxListHead = pfx->nextPfx;
    else if (pfx->prevPfx)
        pfx->prevPfx->nextPfx = pfx->nextPfx;
    if (pfx->nextPfx)
        pfx->nextPfx->prevPfx = pfx->prevPfx;
    pfx->prevPfx = 0;
    pfx->nextPfx = 0;
    --numInList;
}

void zCParticleFX::zCStaticPfxList::TouchPfx(zCParticleFX* pfx)
{
    if (pfxListHead != pfx) {
        RemovePfx(pfx);
        InsertPfxHead(pfx);
    }
    pfx->lastTimeRendered = ztimer.totalTimeFloat;
}

void zCParticleFX::zCStaticPfxList::ProcessList()
{
    zCParticleFX* pfx = pfxListTail;
    while (pfx) {
        zCParticleFX* previous = pfx->prevPfx;
        if (!pfx->connectedVob)
            RemovePfx(pfx);
        else if (!pfx->emitter->m_bIsAmbientPFX || s_bAmbientPFXEnabled)
            pfx->UpdateParticleFX();
        pfx = previous;
    }
}

int zCParticleFX::zCStaticPfxList::IsInList(zCParticleFX* pfx)
{
    if (pfx->nextPfx == pfx->prevPfx && pfxListTail != pfx && pfxListHead != pfx)
        return 0;
    return 1;
}

zCParticleEmitter::zCParticleEmitter()
    : ppsValue(0), ppsIsLooping(0), ppsIsSmooth(0), ppsFPS(0), m_bIsAmbientPFX(0),
      ppsCreateEmitter(0), ppsCreateEmDelay(0), shpScaleFPS(0), shpScaleIsLooping(0),
      shpScaleIsSmooth(0), velAvg(0), velVar(0), lspPartAvg(0), lspPartVar(0),
      visSizeEndScale(1.0f)
{
    visSizeStart.n[0] = 0;
    visSizeStart.n[1] = 0;
    flyGravity.n[0] = 0;
    flyGravity.n[1] = 0;
    flyGravity.n[2] = 0;
    Reset();
}

zCParticleEmitter::zCParticleEmitter(const zCParticleEmitter& other)
    : ppsValue(other.ppsValue), ppsIsLooping(other.ppsIsLooping),
      ppsIsSmooth(other.ppsIsSmooth), ppsFPS(other.ppsFPS),
      particleFXName(other.particleFXName), m_bIsAmbientPFX(other.m_bIsAmbientPFX),
      ppsCreateEmitter(other.ppsCreateEmitter), ppsCreateEmDelay(other.ppsCreateEmDelay),
      shpScaleFPS(other.shpScaleFPS), shpScaleIsLooping(other.shpScaleIsLooping),
      shpScaleIsSmooth(other.shpScaleIsSmooth), velAvg(other.velAvg), velVar(other.velVar),
      lspPartAvg(other.lspPartAvg), lspPartVar(other.lspPartVar), flyGravity(other.flyGravity),
      visSizeStart(other.visSizeStart), visSizeEndScale(other.visSizeEndScale)
{
    ppsScaleKeys.AllocAbs(other.ppsScaleKeys.numInArray);
    for (int i = 0; i < other.ppsScaleKeys.numInArray; ++i)
        ppsScaleKeys.InsertEnd(other.ppsScaleKeys.parray[i]);
    shpScaleKeys.AllocAbs(other.shpScaleKeys.numInArray);
    for (int i = 0; i < other.shpScaleKeys.numInArray; ++i)
        shpScaleKeys.InsertEnd(other.shpScaleKeys.parray[i]);
}

zCParticleEmitter& zCParticleEmitter::operator=(const zCParticleEmitter& other)
{
    ppsValue = other.ppsValue;
    ppsScaleKeys_S = other.ppsScaleKeys_S;
    ppsIsLooping = other.ppsIsLooping;
    ppsIsSmooth = other.ppsIsSmooth;
    ppsFPS = other.ppsFPS;
    ppsCreateEm_S = other.ppsCreateEm_S;
    ppsCreateEmDelay = other.ppsCreateEmDelay;
    shpType_S = other.shpType_S;
    shpFOR_S = other.shpFOR_S;
    shpOffsetVec_S = other.shpOffsetVec_S;
    shpDistribType_S = other.shpDistribType_S;
    shpDistribWalkSpeed = other.shpDistribWalkSpeed;
    shpIsVolume = other.shpIsVolume;
    shpDim_S = other.shpDim_S;
    shpMesh_S = other.shpMesh_S;
    shpMeshRender_B = other.shpMeshRender_B;
    shpScaleKeys_S = other.shpScaleKeys_S;
    shpScaleIsLooping = other.shpScaleIsLooping;
    shpScaleIsSmooth = other.shpScaleIsSmooth;
    shpScaleFPS = other.shpScaleFPS;
    dirMode_S = other.dirMode_S;
    dirFOR_S = other.dirFOR_S;
    dirModeTargetFOR_S = other.dirModeTargetFOR_S;
    dirModeTargetPos_S = other.dirModeTargetPos_S;
    dirAngleHead = other.dirAngleHead;
    dirAngleHeadVar = other.dirAngleHeadVar;
    dirAngleElev = other.dirAngleElev;
    dirAngleElevVar = other.dirAngleElevVar;
    velAvg = other.velAvg;
    velVar = other.velVar;
    lspPartAvg = other.lspPartAvg;
    lspPartVar = other.lspPartVar;
    flyGravity_S = other.flyGravity_S;
    flyCollDet_B = other.flyCollDet_B;
    visName_S = other.visName_S;
    visOrientation_S = other.visOrientation_S;
    visTexIsQuadPoly = other.visTexIsQuadPoly;
    visTexAniFPS = other.visTexAniFPS;
    visTexAniIsLooping = other.visTexAniIsLooping;
    visTexColorStart_S = other.visTexColorStart_S;
    visTexColorEnd_S = other.visTexColorEnd_S;
    visSizeStart_S = other.visSizeStart_S;
    visSizeEndScale = other.visSizeEndScale;
    visAlphaFunc_S = other.visAlphaFunc_S;
    visAlphaStart = other.visAlphaStart;
    visAlphaEnd = other.visAlphaEnd;
    trlFadeSpeed = other.trlFadeSpeed;
    trlTexture_S = other.trlTexture_S;
    trlWidth = other.trlWidth;
    mrkFadeSpeed = other.mrkFadeSpeed;
    mrkTexture_S = other.mrkTexture_S;
    mrkSize = other.mrkSize;
    m_flockMode_S = other.m_flockMode_S;
    m_fFlockWeight = other.m_fFlockWeight;
    m_bSlowLocalFOR = other.m_bSlowLocalFOR;
    m_timeStartEnd_S = other.m_timeStartEnd_S;
    m_bIsAmbientPFX = other.m_bIsAmbientPFX;
    endOfDScriptPart = other.endOfDScriptPart;
    particleFXName = other.particleFXName;
    ppsScaleKeys = other.ppsScaleKeys;
    ppsCreateEmitter = other.ppsCreateEmitter;
    shpType = other.shpType;
    shpCircleSphereRadius = other.shpCircleSphereRadius;
    shpLineBoxDim = other.shpLineBoxDim;
    shpMeshLastPolyNormal = other.shpMeshLastPolyNormal;
    shpMesh = other.shpMesh;
    shpProgMesh = other.shpProgMesh;
    shpModel = other.shpModel;
    shpFOR = other.shpFOR;
    shpDistribType = other.shpDistribType;
    shpOffsetVec = other.shpOffsetVec;
    shpScaleKeys = other.shpScaleKeys;
    dirMode = other.dirMode;
    dirFOR = other.dirFOR;
    dirModeTargetFOR = other.dirModeTargetFOR;
    dirModeTargetPos = other.dirModeTargetPos;
    dirAngleBox = other.dirAngleBox;
    dirAngleBoxDim = other.dirAngleBoxDim;
    flyGravity = other.flyGravity;
    visTexture = other.visTexture;
    visOrientation = other.visOrientation;
    visSizeStart = other.visSizeStart;
    visTexColorRGBAStart = other.visTexColorRGBAStart;
    visTexColorRGBAEnd = other.visTexColorRGBAEnd;
    visAlphaFunc = other.visAlphaFunc;
    trlTexture = other.trlTexture;
    mrkTexture = other.mrkTexture;
    isOneShotFX = other.isOneShotFX;
    dirAngleHeadVarRad = other.dirAngleHeadVarRad;
    dirAngleElevVarRad = other.dirAngleElevVarRad;
    m_flockMode = other.m_flockMode;
    m_ooAlphaDist = other.m_ooAlphaDist;
    m_startTime = other.m_startTime;
    m_endTime = other.m_endTime;
    return *this;
}

const zSTRING* zCParticleFX::GetFileExtension(int index)
{
    if (index)
        return 0;
    static const zSTRING extension(".PFX");
    return &extension;
}

zCVisual* zCParticleFX::LoadVisualVirtual(const zSTRING& fileName) const
{
    return Load(fileName);
}

void zCParticleFX::FreeParticles()
{
    while (firstPart) {
        zSParticle* particle = firstPart;
        firstPart = particle->next;
        if (particle->polyStrip)
            particle->polyStrip->Release();
        particle->next = s_globFreePart;
        s_globFreePart = particle;
    }
}

void zCParticleFX::SetVisualUsedBy(zCVob* vob)
{
    vob->showVisual &= ~0x20;
    connectedVob = vob;
    for (int axis = 0; axis < 3; ++axis) {
        bbox3DWorld.mins.n[axis] = vob->trafoObjToWorld.m[axis][3] - 1.0f;
        bbox3DWorld.maxs.n[axis] = bbox3DWorld.mins.n[axis] + 2.0f;
    }
}

zSTRING zCParticleFX::GetVisualName()
{
    zSTRING result;
    if (emitter->particleFXName.Length() > 0) {
        result = emitter->particleFXName;
        result.Append(".pfx");
    }
    return result;
}

zCParticleEmitter* zCParticleFX::SearchParticleEmitter(const zSTRING& name)
{
    for (int i = 0; i < s_emitterPresetList.numInArray; ++i) {
        if (s_emitterPresetList.array[i]->particleFXName == name)
            return s_emitterPresetList.array[i];
    }
    return 0;
}

void zCParticleFX::RemoveEmitter()
{
    if (emitterIsOwned)
        delete emitter;
    emitter = &s_emitterDummyDefault;
    emitterIsOwned = 0;
}

int zCParticleFX::SetEmitter(zCParticleEmitter* newEmitter, int copy)
{
    if (!newEmitter)
        return 0;
    RemoveEmitter();
    if (copy) {
        emitter = new zCParticleEmitter(*newEmitter);
        emitterIsOwned = 1;
    }
    else {
        emitter = newEmitter;
        emitterIsOwned = 0;
    }
    InitEmitterVars();
    isOneShotFX = !emitter->ppsIsLooping && emitter->ppsScaleKeys.numInArray == 0;
    s_pfxList.TouchPfx(this);
    return 1;
}

int zCParticleFX::SetEmitter(const zSTRING& name, int copy)
{
    zSTRING upperName = name;
    upperName.Upper();
    return SetEmitter(SearchParticleEmitter(upperName), copy);
}

int zCParticleFX::SetAndStartEmitter(zCParticleEmitter* newEmitter, int copy)
{
    const int result = SetEmitter(newEmitter, copy);
    CreateParticles();
    return result;
}

int zCParticleFX::SetAndStartEmitter(const zSTRING& name, int copy)
{
    const int result = SetEmitter(name, copy);
    CreateParticles();
    return result;
}

void zCParticleFX::StopEmitterOutput()
{
    isOneShotFX = 1;
}

void zCParticleFX::RestoreEmitterOutput()
{
    isOneShotFX = !emitter->ppsIsLooping && emitter->ppsScaleKeys.numInArray == 0;
    InitEmitterVars();
}

zCParticleFX* zCParticleFX::Load(const zSTRING& fileName)
{
    zCParticleFX* pfx = new zCParticleFX;
    zSTRING name = fileName;
    name.Upper();
    const int result = pfx->SetEmitter(name, 0);
    pfx->CreateParticles();
    if (!result) {
        delete pfx;
        return 0;
    }
    return pfx;
}

zTBBox3D zCParticleFX::GetBBox3D()
{
    zTBBox3D result = bbox3DWorld;
    float scale = emitter->visSizeEndScale * 0.8f;
    if (scale > 1.0f)
        scale = 1.0f;
    const float radius = scale * (emitter->visSizeStart.n[0] + emitter->visSizeStart.n[1]) * 0.5f;
    for (int axis = 0; axis < 3; ++axis) {
        result.mins.n[axis] -= radius;
        result.maxs.n[axis] += radius;
        if (quadMark) {
            if (quadMarkBBox3DWorld.mins.n[axis] < result.mins.n[axis])
                result.mins.n[axis] = quadMarkBBox3DWorld.mins.n[axis];
            if (quadMarkBBox3DWorld.maxs.n[axis] > result.maxs.n[axis])
                result.maxs.n[axis] = quadMarkBBox3DWorld.maxs.n[axis];
        }
    }
    bbox3DWorld = result;
    return result;
}

int zCParticleFX::CalcIsDead()
{
    if (m_bVisualNeverDies || firstPart || quadMark)
        return 0;
    return isOneShotFX || !emitter->ppsIsLooping ||
        emitterVars.ppsScaleKeysActFrame >= emitter->ppsScaleKeys.numInArray;
}

float zCParticleFX::PartRand()
{
    return (std::rand() - 16383.5f) * 6.10370189e-05f;
}

void zCParticleFX::UpdateParticleFX()
{
    zSParticle* particle = firstPart;
    while (particle) {
        zSParticle* next = particle->next;
        if (particle->lifeSpan <= privateTotalTime) {
            if (firstPart == particle)
                firstPart = next;
            if (particle->polyStrip)
                particle->polyStrip->Release();
            particle->next = s_globFreePart;
            s_globFreePart = particle;
        }
        else
            UpdateParticle(particle);
        particle = next;
    }

    CreateParticlesUpdateDependencies();
    if (GetVisualDied() && connectedVob && connectedVob->homeWorld)
        connectedVob->homeWorld->RemoveVob(connectedVob);
}

void zCParticleFX::CreateParticlesUpdateDependencies()
{
    if (!isOneShotFX)
        CreateParticles();
    privateTotalTime += ztimer.frameTimeFloat;

    if (CalcIsDead()) {
        if (!dontKillPFXWhenDone)
            dead = 1;
    }
    else if (!firstPart && connectedVob) {
        for (int axis = 0; axis < 3; ++axis) {
            bbox3DWorld.mins.n[axis] = connectedVob->trafoObjToWorld.m[axis][3] - 1.0f;
            bbox3DWorld.maxs.n[axis] = connectedVob->trafoObjToWorld.m[axis][3] + 1.0f;
        }
    }
}

void zCParticleFX::UpdateParticle(zSParticle* particle)
{
    for (int axis = 0; axis < 3; ++axis) {
        particle->vel.n[axis] += emitter->flyGravity.n[axis] * localFrameTimeF;
        particle->positionWS.n[axis] += particle->vel.n[axis] * localFrameTimeF;
        particle->position.n[axis] = particle->positionWS.n[axis];
        particle->color.n[axis] += particle->colorVel.n[axis] * localFrameTimeF;
        if (particle->positionWS.n[axis] < bbox3DWorld.mins.n[axis])
            bbox3DWorld.mins.n[axis] = particle->positionWS.n[axis];
        if (particle->positionWS.n[axis] > bbox3DWorld.maxs.n[axis])
            bbox3DWorld.maxs.n[axis] = particle->positionWS.n[axis];
    }
    particle->alpha += particle->alphaVel * localFrameTimeF;
    particle->size.n[0] += particle->sizeVel.n[0] * localFrameTimeF;
    particle->size.n[1] += particle->sizeVel.n[1] * localFrameTimeF;
}

zVEC2& zVEC2::operator+=(const zVEC2& value)
{
    n[0] += value.n[0];
    n[1] += value.n[1];
    return *this;
}

float zCParticleFX::GetShapeScaleThisFrame()
{
    const int numKeys = emitter->shpScaleKeys.numInArray;
    if (emitter->shpScaleFPS <= 0 || numKeys <= 0)
        return 1.0f;

    const int frame = static_cast<int>(emitterVars.shpScaleKeysActFrame);
    if (frame >= numKeys)
        return 1.0f;
    float scale = emitter->shpScaleKeys.parray[frame];
    if (emitter->shpScaleIsSmooth) {
        int next = frame + 1;
        if (next >= numKeys)
            next = 0;
        const float fraction = emitterVars.shpScaleKeysActFrame - frame;
        scale += (emitter->shpScaleKeys.parray[next] - scale) * fraction;
    }

    emitterVars.shpScaleKeysActFrame +=
        emitter->shpScaleFPS * 0.001f * localFrameTimeF;
    if (emitterVars.shpScaleKeysActFrame >= numKeys) {
        if (emitter->shpScaleIsLooping) {
            while (emitterVars.shpScaleKeysActFrame >= numKeys)
                emitterVars.shpScaleKeysActFrame -= numKeys;
        }
        else
            emitterVars.shpScaleKeysActFrame = numKeys - 1;
    }
    return scale;
}

int zCParticleFX::GetNumParticlesThisFrame()
{
    float particlesPerSecond = emitter->ppsValue;
    const int numKeys = emitter->ppsScaleKeys.numInArray;
    if (emitter->ppsFPS > 0 && numKeys > 0) {
        const int frame = static_cast<int>(emitterVars.ppsScaleKeysActFrame);
        if (frame < numKeys) {
            float scale = emitter->ppsScaleKeys.parray[frame];
            if (emitter->ppsIsSmooth) {
                int next = frame + 1;
                if (next >= numKeys)
                    next = 0;
                const float fraction = emitterVars.ppsScaleKeysActFrame - frame;
                scale += (emitter->ppsScaleKeys.parray[next] - scale) * fraction;
            }
            particlesPerSecond *= scale;
        }

        emitterVars.ppsScaleKeysActFrame += emitter->ppsFPS * 0.001f * localFrameTimeF;
        if (emitterVars.ppsScaleKeysActFrame >= numKeys) {
            if (emitter->ppsIsLooping) {
                while (emitterVars.ppsScaleKeysActFrame >= numKeys)
                    emitterVars.ppsScaleKeysActFrame -= numKeys;
            }
            else {
                isOneShotFX = 1;
                emitterVars.ppsScaleKeysActFrame = numKeys - 1;
            }
        }
    }

    const float value = particlesPerSecond * 0.001f * localFrameTimeF +
        emitterVars.ppsNumParticlesFraction;
    const int result = static_cast<int>(value);
    emitterVars.ppsNumParticlesFraction = value - result;
    return result;
}

void zCParticleFX::CheckDependentEmitter()
{
    if (!emitter->ppsCreateEmitter || emitterVars.ppsDependentEmitterCreated)
        return;
    emitterVars.ppsTotalLifeTime += localFrameTimeF;
    if (emitterVars.ppsTotalLifeTime < emitter->ppsCreateEmDelay)
        return;

    zCParticleFX* child = new zCParticleFX;
    child->SetEmitter(emitter->ppsCreateEmitter, 0);
    child->CreateParticles();
    emitterVars.ppsDependentEmitterCreated = 1;
}

void zCParticleFX::CreateParticles()
{
    localFrameTimeF = ztimer.frameTimeFloat * timeScale;
    if (!connectedVob || localFrameTimeF <= 0)
        return;
    CheckDependentEmitter();
    int numParticles = GetNumParticlesThisFrame();

    for (int i = 0; i < numParticles && s_globFreePart; ++i) {
        zSParticle* particle = s_globFreePart;
        s_globFreePart = particle->next;
        particle->next = firstPart;
        firstPart = particle;
        particle->polyStrip = 0;
        particle->lifeSpan = privateTotalTime + emitter->lspPartAvg +
            emitter->lspPartVar * PartRand();
        particle->alpha = 1.0f;
        particle->alphaVel = 0;
        particle->size.n[0] = emitter->visSizeStart.n[0];
        particle->size.n[1] = emitter->visSizeStart.n[1];
        particle->sizeVel.n[0] = 0;
        particle->sizeVel.n[1] = 0;
        for (int axis = 0; axis < 3; ++axis) {
            particle->position.n[axis] = connectedVob->trafoObjToWorld.m[axis][3];
            particle->positionWS.n[axis] = particle->position.n[axis];
            particle->vel.n[axis] = emitter->velAvg + emitter->velVar * PartRand();
            particle->color.n[axis] = 1.0f;
            particle->colorVel.n[axis] = 0;
        }
    }
}

int zCParticleFX::Render(zTRenderContext&)
{
    if (emitter->m_bIsAmbientPFX && !s_bAmbientPFXEnabled)
        return 0;
    int rendered = 0;
    for (zSParticle* particle = firstPart; particle; particle = particle->next) {
        if (particle->lifeSpan > privateTotalTime)
            ++rendered;
    }
    return rendered;
}

void zCParticleEmitter::UpdateVelocity()
{
}

void zCParticleEmitter::AddCompoundReferences()
{
    if (visTexture)
        visTexture->AddRef();
    if (shpMesh)
        shpMesh->AddRef();
    if (shpProgMesh)
        ++shpProgMesh->refCtr;
    if (shpModel)
        shpModel->AddRef();
    if (trlTexture)
        trlTexture->AddRef();
    if (mrkTexture)
        mrkTexture->AddRef();
    shpMeshLastPolyNormal = 0;
}

void zCParticleEmitter::ResetStrings()
{
    ppsScaleKeys_S.Clear();
    ppsCreateEm_S.Clear();
    shpType_S.Clear();
    shpFOR_S.Clear();
    shpOffsetVec_S.Clear();
    shpDistribType_S.Clear();
    shpDim_S.Clear();
    shpMesh_S.Clear();
    shpScaleKeys_S.Clear();
    dirMode_S.Clear();
    dirFOR_S.Clear();
    dirModeTargetFOR_S.Clear();
    dirModeTargetPos_S.Clear();
    flyGravity_S.Clear();
    visName_S.Clear();
    visOrientation_S.Clear();
    visSizeStart_S.Clear();
    visTexColorStart_S.Clear();
    visTexColorEnd_S.Clear();
    visAlphaFunc_S.Clear();
    trlTexture_S.Clear();
    mrkTexture_S.Clear();
    m_flockMode_S.Clear();
    m_timeStartEnd_S.Clear();
}

void zCParticleEmitter::Reset()
{
    ppsValue = 0;
    ppsIsLooping = 0;
    ppsIsSmooth = 0;
    ppsFPS = 0;
    ppsCreateEmDelay = 0;
    shpDistribWalkSpeed = 0;
    shpIsVolume = 0;
    shpMeshRender_B = 0;
    shpScaleIsLooping = 0;
    shpScaleIsSmooth = 0;
    shpScaleFPS = 0;
    dirAngleHead = 0;
    dirAngleHeadVar = 0;
    dirAngleElev = 0;
    dirAngleElevVar = 0;
    velAvg = 0;
    velVar = 0;
    lspPartAvg = 0;
    lspPartVar = 0;
    flyCollDet_B = 0;
    visTexIsQuadPoly = 0;
    visTexAniFPS = 0;
    visTexAniIsLooping = 0;
    visSizeEndScale = 1.0f;
    visAlphaStart = 0;
    visAlphaEnd = 0;
    trlFadeSpeed = 0;
    trlWidth = 0;
    mrkFadeSpeed = 0;
    mrkSize = 0;
    m_fFlockWeight = 0;
    m_bSlowLocalFOR = 0;
    m_bIsAmbientPFX = 0;
    endOfDScriptPart = 0;
    particleFXName.Clear();
    ppsScaleKeys.DeleteList();
    shpScaleKeys.DeleteList();
    ppsCreateEmitter = 0;
    shpType = zPFX_EMITTER_SHAPE_POINT;
    shpCircleSphereRadius = 0;
    shpMeshLastPolyNormal = 0;
    shpMesh = 0;
    shpProgMesh = 0;
    shpModel = 0;
    shpFOR = 0;
    shpDistribType = 0;
    dirMode = zPFX_EMITTER_DIRMODE_NONE;
    dirFOR = 0;
    dirModeTargetFOR = 0;
    visTexture = 0;
    visOrientation = 0;
    trlTexture = 0;
    mrkTexture = 0;
    isOneShotFX = 0;
    dirAngleHeadVarRad = 0;
    dirAngleElevVarRad = 0;
    m_flockMode = 0;
    m_ooAlphaDist = 0;
    m_startTime = 0;
    m_endTime = 0;
    ResetStrings();
}

zCParticleEmitter::~zCParticleEmitter()
{
    if (visTexture)
        visTexture->Release();
    if (shpMesh)
        shpMesh->Release();
    if (shpProgMesh && --shpProgMesh->refCtr <= 0)
        delete shpProgMesh;
    if (shpModel)
        shpModel->Release();
    if (trlTexture)
        trlTexture->Release();
    if (mrkTexture)
        mrkTexture->Release();
    shpMeshLastPolyNormal = 0;
}

zVEC3 zCParticleEmitter::String2Vec3(const zSTRING& text)
{
    zVEC3 result;
    result.n[0] = text.PickWord_Old(1, " ").ToFloat();
    result.n[1] = text.PickWord_Old(2, " ").ToFloat();
    result.n[2] = text.PickWord_Old(3, " ").ToFloat();
    return result;
}

zVEC2 zCParticleEmitter::String2Vec2(const zSTRING& text)
{
    zVEC2 result;
    result.n[0] = text.PickWord_Old(1, " ").ToFloat();
    result.n[1] = text.PickWord_Old(2, " ").ToFloat();
    return result;
}

void zCParticleEmitter::UpdateInternals()
{
    ppsScaleKeys.numInArray = 0;
    for (int i = 1;; ++i) {
        const zSTRING word = ppsScaleKeys_S.PickWord_Old(i, " ");
        if (word.Length() == 0)
            break;
        ppsScaleKeys.InsertEnd(word.ToFloat());
    }
    shpScaleKeys.numInArray = 0;
    for (int i = 1;; ++i) {
        const zSTRING word = shpScaleKeys_S.PickWord_Old(i, " ");
        if (word.Length() == 0)
            break;
        shpScaleKeys.InsertEnd(word.ToFloat());
    }
    ppsCreateEm_S.Upper();
    ppsCreateEmitter = zCParticleFX::SearchParticleEmitter(ppsCreateEm_S);
    shpOffsetVec = String2Vec3(shpOffsetVec_S);
    shpLineBoxDim = String2Vec3(shpDim_S);
    shpCircleSphereRadius = shpDim_S.ToFloat();
    dirModeTargetPos = String2Vec3(dirModeTargetPos_S);
    flyGravity = String2Vec3(flyGravity_S);
    visSizeStart = String2Vec2(visSizeStart_S);
    dirAngleHeadVarRad = dirAngleHeadVar * 0.0174532924f;
    dirAngleElevVarRad = dirAngleElevVar * 0.0174532924f;
    ConvertAnglesIntoBox();
    m_ooAlphaDist = visAlphaEnd != visAlphaStart ?
        1.0f / (visAlphaEnd - visAlphaStart) : 1.0f;
}

void zCParticleEmitter::SetOutputDir(const zVEC3& direction)
{
    dirAngleBox.mins = direction;
    dirAngleBox.maxs = direction;
    dirAngleBoxDim.n[0] = 0;
    dirAngleBoxDim.n[1] = 0;
    dirAngleBoxDim.n[2] = 0;
}

void zCParticleEmitter::ConvertAnglesIntoBox()
{
    const float head = dirAngleHead * 0.0174532924f;
    const float elev = dirAngleElev * 0.0174532924f;
    zVEC3 direction;
    direction.n[0] = -std::sin(head) * std::cos(elev);
    direction.n[1] = std::sin(elev);
    direction.n[2] = std::cos(head) * std::cos(elev);
    SetOutputDir(direction);
}

zVEC3 zCParticleEmitter::GetPosition()
{
    zVEC3 result;
    result.n[0] = 0;
    result.n[1] = 0;
    result.n[2] = 0;
    if (shpType == zPFX_EMITTER_SHAPE_LINE)
        result.n[0] = zCParticleFX::PartRand() * shpLineBoxDim.n[0];
    else if (shpType == zPFX_EMITTER_SHAPE_BOX) {
        for (int axis = 0; axis < 3; ++axis)
            result.n[axis] = zCParticleFX::PartRand() * shpLineBoxDim.n[axis];
    }
    else if (shpType == zPFX_EMITTER_SHAPE_CIRCLE) {
        const float angle = std::rand() * 6.10370189e-05f * 3.14159265f;
        const float radius = shpIsVolume ?
            std::rand() * shpCircleSphereRadius * 3.05185094e-05f : shpCircleSphereRadius;
        result.n[0] = -std::sin(angle) * radius;
        result.n[2] = std::cos(angle) * radius;
    }
    else if (shpType == zPFX_EMITTER_SHAPE_SPHERE) {
        for (int axis = 0; axis < 3; ++axis)
            result.n[axis] = zCParticleFX::PartRand();
        const float length = std::sqrt(result.n[0] * result.n[0] +
            result.n[1] * result.n[1] + result.n[2] * result.n[2]);
        const float radius = shpIsVolume ?
            std::rand() * shpCircleSphereRadius * 3.05185094e-05f : shpCircleSphereRadius;
        if (length > 0) {
            for (int axis = 0; axis < 3; ++axis)
                result.n[axis] = result.n[axis] / length * radius;
        }
    }
    return result;
}

zVEC3 zCParticleEmitter::GetVelocity(zSParticle* particle, zCParticleFX*)
{
    zVEC3 direction;
    if (dirMode == zPFX_EMITTER_DIRMODE_DIR) {
        for (int axis = 0; axis < 3; ++axis)
            direction.n[axis] = dirAngleBox.mins.n[axis] +
                std::rand() * dirAngleBoxDim.n[axis] * 3.05185094e-05f;
    }
    else if (dirMode == zPFX_EMITTER_DIRMODE_TARGET) {
        for (int axis = 0; axis < 3; ++axis)
            direction.n[axis] = dirModeTargetPos.n[axis] - particle->positionWS.n[axis];
    }
    else if (dirMode == zPFX_EMITTER_DIRMODE_MESH && shpMeshLastPolyNormal)
        direction = *shpMeshLastPolyNormal;
    else {
        for (int axis = 0; axis < 3; ++axis)
            direction.n[axis] = zCParticleFX::PartRand();
    }

    const float length = std::sqrt(direction.n[0] * direction.n[0] +
        direction.n[1] * direction.n[1] + direction.n[2] * direction.n[2]);
    const float speed = velAvg + zCParticleFX::PartRand() * velVar;
    if (length > 0) {
        for (int axis = 0; axis < 3; ++axis)
            direction.n[axis] = direction.n[axis] / length * speed;
    }
    return direction;
}
