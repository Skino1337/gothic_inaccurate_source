#include "include/zAiPlayer.h"
#include "include/zSparseArray.h"
#include "include/zPolyStrip.h"
#include "include/zCache.h"
#include <cmath>
#include <cstring>

static int aiPlayerClassDefState = -1;
static zCClassDef zCAIPlayerClassDef("zCAIPlayer", "zCAIBase",
    zCAIPlayer::_CreateNewInstance, 1, 0x118, 0, "");
zCClassDef* zCAIPlayer::classDef = &zCAIPlayerClassDef;
static zVEC3 aiPlayerMaxPosition(0.0f, 100000.0f, 0.0f);
static zVEC3 aiPlayerMinPosition(0.0f, -100000.0f, 0.0f);
static float aiPlayerCameraTime = 180.0f;
static float aiPlayerCameraTransition = 150.0f;
static float aiPlayerCameraDelay = 150.0f;
static zCCache<zCAIPlayer*, zCAIPlayer::zTLedgeInfo> aiPlayerLedgeCache(13, 100);
static int aiPlayerState = -1;
int zCAIPlayer::s_bShowWeaponTrails = 1;
static zCSparseArray<zCAIPlayer*, zCAIPlayer*> playerArray(1031);

zCObject* zCAIPlayer::_CreateNewInstance()
{
    zCAIPlayer* player = new zCAIPlayer;
    zCClassDef::ObjectCreated(player, classDef);
    return player;
}

void zCAIPlayer::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCAIPlayer::zCAIPlayer()
{
    std::memset(&config, 0, sizeof(config));
    vob = NULL;
    model = NULL;
    world = NULL;
    centerPos = zVEC3(0.0f, 0.0f, 0.0f);
    feetY = 0.0f;
    headY = 0.0f;
    aboveFloor = 0.0f;
    waterLevel = 0;
    velocityLen2 = 0.0f;
    velocity = zVEC3(0.0f, 0.0f, 0.0f);
    fallDownDistanceY = 0.0f;
    fallDownStartY = 0.0f;
    slidePolyNormal = zVEC3(0.0f, 0.0f, 0.0f);
    checkWaterCollBodyLen = 0.0f;
    modelHeadNode = NULL;
    zMV_DO_SURFACE_ALIGN = 0;
    forceModelHalt = 0;
    autoRollEnabled = 0;
    modelAnisInPlace = 0;
    oldState = zMV_STATE_STAND;
    state = zMV_STATE_STAND;
    oldWaterLevel = 0;
    m_eCollObjectState = 0;
    m_eCollObjectStateOld = 0;
    bleedingPerc = 0.0f;
    bleedingLastPos = zVEC3(0.0f, 0.0f, 0.0f);
    bleedingNextDist = 0.0f;
    weaponTrailVob = NULL;
    waterRingVob = NULL;
    waterRingTimer = 0.0f;
    autoRollPos = 0.0f;
    autoRollPosDest = 1.0f;
    autoRollSpeed = 0.003f;
    autoRollMaxAngle = 10.0f;
    dummyLastVar = 0;
    bloodDefaultTexName = "ZBLOODSPLAT2.TGA";
    playerArray.Insert(this);
}

zCAIPlayer::~zCAIPlayer()
{
    RemoveEffects();
    playerArray.Delete(this);
    vob = NULL;
    model = NULL;
    world = NULL;
}

zCClassDef* zCAIPlayer::_GetClassDef() const
{
    return classDef;
}

void zCAIPlayer::RemoveEffects()
{
    if (waterRingVob) {
        waterRingVob->RemoveVobFromWorld();
        waterRingVob->Release();
        waterRingVob = NULL;
    }
    if (weaponTrailVob) {
        weaponTrailVob->RemoveVobFromWorld();
        weaponTrailVob->Release();
        weaponTrailVob = NULL;
    }

    for (int i = 0; i < bloodVobList.numInArray; i++) {
        if (bloodVobList.parray[i].bloodVob) {
            bloodVobList.parray[i].bloodVob->RemoveVobFromWorld();
            bloodVobList.parray[i].bloodVob->Release();
            bloodVobList.parray[i].bloodVob = NULL;
        }
    }
    bloodVobList.DeleteList();
}

void zCAIPlayer::HostVobRemovedFromWorld(zCVob* vob, zCWorld* world)
{
    RemoveEffects();
}

void zCSparseArrayIterator::Reset()
{
    if (registered) {
        registered = 0;
        array->numberOfIterators--;
    }

    count = array->size;
    while (count > 0) {
        count--;
        if (array->ManDatPtr(count)->occupied)
            break;
    }

    registered = 1;
    array->numberOfIterators++;
}

zCSparseArrayIterator::~zCSparseArrayIterator()
{
    if (registered) {
        registered = 0;
        array->numberOfIterators--;
    }
}

float zCAIPlayer::GetJumpUpHeight()
{
    const float mass = vob->GetRigidBody()->mass;
    return config.zMV_FORCE_JUMP_UP * config.zMV_FORCE_JUMP_UP / (mass * mass * 1962.0f);
}

void zCAIPlayer::SetJumpUpForceByHeight(float height)
{
    config.zMV_FORCE_JUMP_UP = std::sqrt(height * 1962.0f) * vob->GetRigidBody()->mass;
}

void zCAIPlayer::SetVob(zCVob* target)
{
    zMV_DO_SURFACE_ALIGN = (target->showVisual & 0x20) != 0;
    vob = target;
    world = target->homeWorld;
    model = static_cast<zCModel*>(target->visual);
    modelHeadNode = model ? model->SearchNode("BIP01 HEAD") : NULL;

    zCRigidBody* rigidBody = vob->GetRigidBody();
    rigidBody->mass = 80.0f;
    rigidBody->massInv = 0.0125f;

    float height = 1000.0f;
    if (model) {
        const zTBBox3D& box = model->bbox3DLocalFixed;
        height = ((box.maxs.n[2] - box.mins.n[2])
            + (box.maxs.n[0] - box.mins.n[0])) * 0.8f
            + box.maxs.n[1] - box.mins.n[1];
    }

    const float scale = height / 280.0f;
    config.zMV_MIN_SPACE_MOVE_FORWARD = 50.0f * scale;
    config.zMV_DETECT_CHASM_SCAN_AT = 50.0f * scale;
    config.zMV_STEP_HEIGHT = 75.0f * scale;
    config.zMV_JUMP_UP_MIN_CEIL = 50.0f * scale;
    config.zMV_WATER_DEPTH_KNEE = 50.0f * scale;
    config.zMV_WATER_DEPTH_CHEST = 151.0f * scale;
    config.zMV_YMAX_SWIM_CLIMB_OUT = 100.0f * scale;
    config.zMV_FORCE_JUMP_UP = 60000.0f * scale;
    config.zMV_YMAX_JUMPLOW = 105.0f * scale;
    config.zMV_YMAX_JUMPMID = 205.0f * scale;
    config.zMV_MAX_GROUND_ANGLE_WALK = 0.87266463f;
    config.zMV_MAX_GROUND_ANGLE_SLIDE = 1.04719758f;
    config.zMV_MAX_GROUND_ANGLE_SLIDE2 = 1.22173047f;
    config.zMV_DCUL_WALL_HEADING_ANGLE = 0.785398185f;
    config.zMV_DCUL_WALL_HORIZ_ANGLE = 0.785398185f;
    config.zMV_DCUL_GROUND_ANGLE = 0.785398185f;
}

void zCAIPlayer::DoAutoRoll()
{
    if (!autoRollEnabled || (vob->showVisual & 8))
        return;

    if (autoRollPos != autoRollPosDest) {
        const float speed = autoRollPosDest == 0.0f ? 1.0f : 0.5f;
        const float delta = speed * ztimer.frameTimeFloat * autoRollSpeed;
        if (autoRollPos < autoRollPosDest) {
            autoRollPos += delta;
            if (autoRollPos > autoRollPosDest)
                autoRollPos = autoRollPosDest;
        }
        else {
            autoRollPos -= delta;
            if (autoRollPos < autoRollPosDest)
                autoRollPos = autoRollPosDest;
        }

        float floorHeight = vob->trafoObjToWorld.m[1][0];
        if (floorHeight < -1.0f)
            floorHeight = -1.0f;
        else if (floorHeight > 1.0f)
            floorHeight = 1.0f;
        const float floorAngle = 90.0f - std::acos(floorHeight) * 57.2957764f;
        vob->RotateLocalZ(autoRollMaxAngle * autoRollPos - floorAngle);
    }

    if (autoRollPos == 0.0f && state != zMV_STATE_DIVE
            && std::fabs(vob->trafoObjToWorld.m[1][0]) < 0.0001f)
        vob->ResetXZRotationsWorld();
}

void zCAIPlayer::ResetAutoRoll()
{
    if (!vob || !vob->homeWorld)
        return;
    autoRollPos = 0.0f;
    autoRollPosDest = 0.0f;
    vob->ResetXZRotationsWorld();
}

void zCAIPlayer::CalcModelAnisInPlace()
{
    if (vob->showVisual & 8) {
        modelAnisInPlace = 0;
        return;
    }

    for (int i = 0; i < model->numActiveAnis; i++) {
        if (!(model->aniChannels[i]->protoAni->aniFlags & 0x20)) {
            modelAnisInPlace = 0;
            return;
        }
    }
    modelAnisInPlace = 1;
}

void zCAIPlayer::DoSurfaceAlignment()
{
    if (!zMV_DO_SURFACE_ALIGN || oldState != zMV_STATE_STAND)
        return;

    zVEC3 forward(vob->trafoObjToWorld.m[0][2],
        vob->trafoObjToWorld.m[1][2], vob->trafoObjToWorld.m[2][2]);
    if (slidePolyNormal.Length2() > 0.0f) {
        zVEC3 normal = slidePolyNormal;
        normal.Normalize();
        forward -= normal * (forward * normal);
    }
    if (forward.Length2() > 0.0f) {
        forward.Normalize();
        vob->SetHeadingAtWorld(forward);
    }
}

int zCAIPlayer::IsSliding()
{
    return (vob->showVisual & 8) && vob->GetRigidBody()->mode == zPHY_RBMODE_SLIDE;
}

int zCAIPlayer::CheckFloorSliding()
{
    if (IsSliding()) {
        if (aboveFloor > config.zMV_STEP_HEIGHT || waterLevel > 0) {
            vob->SetPhysicsEnabled(0);
            vob->GetRigidBody()->SetMode(zPHY_RBMODE_FLY);
            state = zMV_STATE_STAND;
            StartStandAni();
            return 0;
        }
        return 1;
    }

    if (waterLevel || state == zMV_STATE_SWIM || state == zMV_STATE_DIVE
            || slidePolyNormal.Length2() == 0.0f)
        return 0;

    zVEC3 normal = slidePolyNormal;
    normal.Normalize();
    float normalY = normal.n[1];
    if (normalY < -1.0f)
        normalY = -1.0f;
    else if (normalY > 1.0f)
        normalY = 1.0f;

    const float floorAngle = std::acos(normalY);
    if (floorAngle > config.zMV_MAX_GROUND_ANGLE_WALK
            && aboveFloor <= config.zMV_STEP_HEIGHT) {
        vob->SetPhysicsEnabled(1);
        vob->GetRigidBody()->SetVelocity(velocity);
        vob->GetRigidBody()->SetSliding(normal);
        model->StartAni("S_SLIDE", 0);
        return 1;
    }

    return 0;
}

void zCAIPlayer::CheckPhysics()
{
    if (aboveFloor > config.zMV_STEP_HEIGHT && !IsSliding() && state == zMV_STATE_STAND) {
        vob->SetPhysicsEnabled(1);
        vob->GetRigidBody()->SetVelocity(velocity * 0.8f);
        state = zMV_STATE_FLY;
        StartFallDownAni();
    }

    if (vob->showVisual & 8) {
        vob->GetRigidBody()->gravityScale = waterLevel == 2 ? 0.2f : 1.0f;
        if (waterLevel == 2)
            vob->GetRigidBody()->SetVelocity(vob->GetRigidBody()->v * 0.8f);
    }
}

void zCAIPlayer::PropagateCollObjectStates(zTMovementState newState)
{
    m_eCollObjectStateOld = m_eCollObjectState;
    if (IsSliding())
        m_eCollObjectState = zMV_STATE_FLY;
    else
        m_eCollObjectState = newState;
}

void zCAIPlayer::CalcForceModelHalt()
{
    if (!vob->GetCollisionObject())
        return;
    forceModelHalt = m_eCollObjectState == zMV_STATE_STAND && IsSliding();
}

void zCAIPlayer::CalcStateVars()
{
    velocity = vob->GetVelocity();
    velocityLen2 = velocity.Length2();

    zCCollisionObject* collisionObject = vob->GetCollisionObject();
    if (collisionObject) {
        centerPos = zVEC3(collisionObject->m_oTrafoNew.m[0][3],
            collisionObject->m_oTrafoNew.m[1][3], collisionObject->m_oTrafoNew.m[2][3]);
    }
    else {
        centerPos = vob->trafoObjToWorld.GetTranslation();
    }

    oldState = state;
    m_eCollObjectStateOld = m_eCollObjectState;
    oldWaterLevel = waterLevel;

    if (state == zMV_STATE_DIVE && modelHeadNode)
        headY = model->GetBBox3DNodeWorld(modelHeadNode).maxs.n[1];
    else
        headY = vob->bbox3D.maxs.n[1];

    if (model)
        feetY = vob->trafoObjToWorld.GetTranslation().n[1] - model->rootPosLocal.n[1];
    else
        feetY = vob->bbox3D.mins.n[1];

    aboveFloor = vob->groundPoly ? 0.0f : config.zMV_STEP_HEIGHT + 1.0f;
    PropagateCollObjectStates(state);
    CalcForceModelHalt();
    CheckPhysics();
    CheckFloorSliding();
}

void zCAIPlayer::DoProceduralMovement()
{
    DoAutoRoll();
    DoSurfaceAlignment();
}

void zCAIPlayer::Begin(zCVob* target)
{
    if (!target->visual || target->visual->_GetClassDef() != zCModel::classDef)
        return;

    if (vob != target)
        SetVob(target);
    CalcStateVars();
    DoProceduralMovement();

    if (state != zMV_STATE_FLY) {
        fallDownStartY = centerPos.n[1];
        fallDownDistanceY = 0.0f;
    }
    if ((vob->showVisual & 8) && !IsSliding())
        fallDownDistanceY = fallDownStartY - centerPos.n[1];
}

void zCAIPlayer::End()
{
    forceModelHalt = 0;
    PropagateCollObjectStates(state);
    if (model && (state == zMV_STATE_SWIM || state == zMV_STATE_FLY)
        model->RecalcRootPosLocal(state != zMV_STATE_SWIM);
    UpdateEffects();
}

void zCAIPlayer::DoAI(zCVob* vob, int& deleteAI)
{
}

void zCAIPlayer::UpdateEffects()
{
    if ((vob->showVisual & 8) && waterLevel > 0 && state != zMV_STATE_DIVE) {
        if (!waterRingVob) {
            zCParticleFX* particle = new zCParticleFX;
            waterRingVob = new zCVob;
            waterRingVob->SetCollDetStat(0);
            waterRingVob->SetCollDetDyn(0);
            waterRingVob->BeginMovement();
            waterRingVob->SetPositionWorld(vob->trafoObjToWorld.GetTranslation());
            waterRingVob->SetVisual(particle);
            particle->SetAndStartEmitter("ZWATERRING", 0);
            vob->homeWorld->AddVob(waterRingVob);
            waterRingVob->EndMovement(1);
        }

        if (waterRingTimer <= 0.0f || velocityLen2 > 100.0f) {
            waterRingVob->SetPositionWorld(vob->trafoObjToWorld.GetTranslation());
            waterRingTimer = 500.0f;
        }
        waterRingTimer -= ztimer.frameTimeFloat;
    }

    if (bleedingPerc > 0.0f && !waterLevel) {
        const zVEC3 position = vob->trafoObjToWorld.GetTranslation();
        const zVEC3 delta = position - bleedingLastPos;
        if (delta.Length2() >= bleedingNextDist) {
            const float size = bleedingPerc * 10.0f + std::rand() * 0.00122074038f + 5.0f;
            zSTRING texture = bloodDefaultTexName;
            AddBlood(position, zVEC3(0.0f, 0.0f, 1.0f), size, 0, &texture);
            bleedingLastPos = position;
            bleedingNextDist = (std::rand() % 501 + 900) * bleedingPerc * bleedingPerc;
        }
    }

    for (int i = bloodVobList.numInArray - 1; i >= 0; i--) {
        bloodVobList.parray[i].alpha -= ztimer.frameTimeFloat * 0.00001f;
        if (bloodVobList.parray[i].alpha <= 0.0f) {
            if (bloodVobList.parray[i].bloodVob) {
                bloodVobList.parray[i].bloodVob->RemoveVobFromWorld();
                bloodVobList.parray[i].bloodVob->Release();
            }
            bloodVobList.RemoveOrderIndex(i);
        }
    }
}

zCQuadMark* zCAIPlayer::GetBloodQuadMark()
{
    if (!vob || !vob->homeWorld)
        return NULL;

    if (bloodVobList.numInArray > 0) {
        zCVob* bloodVob = bloodVobList.parray[bloodVobList.numInArray - 1].bloodVob;
        if (bloodVob) {
            const float area = (bloodVob->bbox3D.maxs.n[2] - bloodVob->bbox3D.mins.n[2])
                * (bloodVob->bbox3D.maxs.n[0] - bloodVob->bbox3D.mins.n[0]);
            if (area < 250000.0f)
                return static_cast<zCQuadMark*>(bloodVob->visual);
        }
    }

    zCQuadMark* quadMark = new zCQuadMark;
    zCVob* bloodVob = new zCVob;
    bloodVob->SetCollDetStat(0);
    bloodVob->SetCollDetDyn(0);
    bloodVob->BeginMovement();
    bloodVob->SetPositionWorld(vob->trafoObjToWorld.GetTranslation());
    bloodVob->SetVisual(quadMark);
    vob->homeWorld->AddVob(bloodVob);
    bloodVob->EndMovement(1);
    quadMark->material->SetTexture(bloodDefaultTexName);

    zTBloodVobTrack track = { bloodVob, 1.0f };
    bloodVobList.InsertEnd(track);
    return quadMark;
}

void zCAIPlayer::SetBloodDefaultTexture(const zSTRING& texture)
{
    bloodDefaultTexName = texture;
}

void zCAIPlayer::AddBlood(const zVEC3& origin, const zVEC3& direction, float size,
    int animated, zSTRING* texture)
{
    zCQuadMark* quadMark = GetBloodQuadMark();
    if (!quadMark)
        return;

    zTQuadMarkEffectParams params = { 3, 0.8f, 0.5f, 0.0f };
    zTQuadMarkEffectParams* effectParams = animated ? &params : static_cast<zTQuadMarkEffectParams*>(NULL);
    const zVEC2 dimensions(size, size);
    if (texture)
        quadMark->CreateQuadMark(origin, direction, dimensions, *texture, effectParams);
    else
        quadMark->CreateQuadMark(origin, direction, dimensions, effectParams);
}

void zCAIPlayer::AddWeaponTrailSegment(const zVEC3& start, const zVEC3& end)
{
    if (!s_bShowWeaponTrails || ztimer.frameTimeFloat == 0.0f)
        return;

    if (!weaponTrailVob) {
        zCPolyStrip* polyStrip = new zCPolyStrip;
        weaponTrailVob = new zCVob;
        weaponTrailVob->SetCollDetStat(0);
        weaponTrailVob->SetCollDetDyn(0);
        weaponTrailVob->BeginMovement();
        weaponTrailVob->SetPositionWorld(vob->trafoObjToWorld.GetTranslation());
        weaponTrailVob->SetVisual(polyStrip);
        vob->homeWorld->AddVob(weaponTrailVob);
        weaponTrailVob->EndMovement(1);
        polyStrip->width = vob->GetCharacterClass() == 0 ? 0.6f : 1.0f;
        polyStrip->material->SetTexture("ZWEAPONTRAIL.TGA");
        polyStrip->AllocateResources(256);
    }

    zCPolyStrip* polyStrip = static_cast<zCPolyStrip*>(weaponTrailVob->visual);
    polyStrip->AddSegment(start, end);
    polyStrip->CalcBBox3D();
}

static zCCache<zCAIPlayer*, zCAIPlayer::zTLedgeInfo> ledgeCache(1031);
static zCWorld* ledgeTraceWorld;
static zCVob* ledgeTraceVob;
static zVEC3 ledgeTraceDirection;

void zCAIPlayer::CreateLedgeInfo()
{
    ledgeCache[this];
}

zCAIPlayer::zTLedgeInfo* zCAIPlayer::GetLedgeInfo() const
{
    return ledgeCache.GetDataDangerous(const_cast<zCAIPlayer*>(this));
}

int zCAIPlayer::GetFoundLedge() const
{
    return ledgeCache.GetDataDangerous(const_cast<zCAIPlayer*>(this)) != NULL;
}

void zCAIPlayer::ClearFoundLedge()
{
    ledgeCache.Delete(this);
}

zCDetectLedgeReport::zCDetectLedgeReport(const zVEC3& origin)
    : origin(origin), found(0), normal(0.0f, 0.0f, 0.0f),
      intersection(0.0f, 0.0f, 0.0f), continuation(0.0f, 0.0f, 0.0f)
{
    if (!ledgeTraceWorld || !ledgeTraceWorld->TraceRayNearestHit(origin,
            ledgeTraceDirection, ledgeTraceVob, 0x8a1))
        return;

    found = 1;
    normal = ledgeTraceWorld->traceRayReport.foundPolyNormal;
    intersection = ledgeTraceWorld->traceRayReport.foundIntersection;
    continuation = origin + ledgeTraceDirection * 20.0f;
    if ((intersection - origin).Length2() > 400.0f || normal.Length2() < 0.8f)
        found = 0;
}

int zCAIPlayer::DetectClimbUpLedge(zVEC3& point, int walkable)
{
    zVEC3 direction = point;
    if (direction.Length2() == 0.0f)
        return 0;
    direction.Normalize();

    float maxHeight = GetJumpUpHeight() * 0.95f;
    if (walkable)
        maxHeight += headY - feetY;

    ledgeTraceWorld = world;
    ledgeTraceVob = vob;
    ledgeTraceDirection = direction * 170.0f;
    if (!world->TraceRayNearestHit(centerPos, ledgeTraceDirection, vob, 0x8a1)) {
        ClearFoundLedge();
        return 0;
    }

    const zVEC3 intersection = world->traceRayReport.foundIntersection;
    if (intersection.n[1] - feetY > maxHeight) {
        ClearFoundLedge();
        return 0;
    }

    zCDetectLedgeReport report(centerPos);
    if (!report.found) {
        ClearFoundLedge();
        return 0;
    }

    zTLedgeInfo* info = GetLedgeInfo();
    if (!info) {
        CreateLedgeInfo();
        info = GetLedgeInfo();
    }
    info->point = intersection;
    info->normal = world->traceRayReport.foundPolyNormal;
    info->cont = report.continuation;
    info->maxMoveForward = (intersection - centerPos).Length();
    return 1;
}

int zCCollObjectCharacter::IsMoveInvalid(const zVEC3& oldPosition, const zVEC3& newPosition,
    zVEC3& adjustment, zTSpatialState& spatialState, zVEC3& normal)
{
    return TestHardCollisions(oldPosition, newPosition, adjustment, spatialState, normal);
}

void zCAIPlayer::AlignModelToLedge(const zSTRING& aniName)
{
    zMV_DO_SURFACE_ALIGN = 0;
    vob->SetCollDetStat(0);
    vob->SetPhysicsEnabled(0);
    model->StartAni(aniName, 2);
    model->AdvanceAnis();

    const zVEC3 handPosition = GetHandPositionWorld();
    zTLedgeInfo* info = GetLedgeInfo();
    if (info) {
        const zVEC3 delta = info->point - handPosition;
        vob->Move(delta.n[0], delta.n[1], delta.n[2]);
        vob->SetHeadingAtWorld(info->normal);
    }
}

zVEC3 zCAIPlayer::GetHandPositionWorld() const
{
    zCModelNodeInst* rightHand = model->SearchNode("BIP01 R HAND");
    zCModelNodeInst* leftHand = model->SearchNode("BIP01 L HAND");
    if (!rightHand || !leftHand)
        return centerPos;

    const zMAT4 rightTrafo = vob->trafoObjToWorld * model->GetTrafoNodeToModel(rightHand);
    const zMAT4 leftTrafo = vob->trafoObjToWorld * model->GetTrafoNodeToModel(leftHand);
    const zVEC3 rightPosition = rightTrafo.GetTranslation();
    const zVEC3 leftPosition = leftTrafo.GetTranslation();
    return (rightPosition + leftPosition) * 0.5f;
}

void zCAIPlayer::AlignToFloor(float height)
{
    zVEC3 position = centerPos;
    position.n[1] -= feetY - height;
    vob->SetPositionWorld(position);
}

float zCAIPlayer::GetModelFloorWorld()
{
    return vob->trafoObjToWorld.GetTranslation().n[1] - model->rootPosLocal.n[1];
}

int zCAIPlayer::CheckEnoughSpaceMoveDir(const zVEC3& direction, int dontMove)
{
    zVEC3 normalized = direction;
    if (normalized.Length2() == 0.0f)
        return 0;
    normalized.Normalize();

    float distance = config.zMV_MIN_SPACE_MOVE_FORWARD * 1.3f;
    if (checkWaterCollBodyLen > distance)
        distance = checkWaterCollBodyLen;
    const int flags = config.zMV_WATER_DEPTH_KNEE < 999999.0f ? 0x801 : 0xa01;
    if (world->TraceRayFirstHit(centerPos, normalized * distance, vob, flags))
        return 0;

    const zVEC3 destination = centerPos + normalized * distance;
    if (world->TraceRayNearestHit(destination, zVEC3(0.0f, -config.zMV_STEP_HEIGHT, 0.0f),
            vob, flags)) {
        const float floorDrop = destination.n[1]
            - world->traceRayReport.foundIntersection.n[1];
        if (model && floorDrop > model->bbox3DLocalFixed.maxs.n[1]
                - model->bbox3DLocalFixed.mins.n[1])
            return 0;
    }

    return 1;
}

int zCAIPlayer::CheckEnoughSpaceMoveForward(int dontMove)
{
    return CheckEnoughSpaceMoveDir(zVEC3(vob->trafoObjToWorld.m[0][2],
        vob->trafoObjToWorld.m[1][2], vob->trafoObjToWorld.m[2][2]), dontMove);
}

int zCAIPlayer::CheckEnoughSpaceMoveBackward(int dontMove)
{
    return CheckEnoughSpaceMoveDir(zVEC3(-vob->trafoObjToWorld.m[0][2],
        -vob->trafoObjToWorld.m[1][2], -vob->trafoObjToWorld.m[2][2]), dontMove);
}

int zCAIPlayer::CheckEnoughSpaceMoveRight(int dontMove)
{
    return CheckEnoughSpaceMoveDir(zVEC3(vob->trafoObjToWorld.m[0][0],
        vob->trafoObjToWorld.m[1][0], vob->trafoObjToWorld.m[2][0]), dontMove);
}

int zCAIPlayer::CheckEnoughSpaceMoveLeft(int dontMove)
{
    return CheckEnoughSpaceMoveDir(zVEC3(-vob->trafoObjToWorld.m[0][0],
        -vob->trafoObjToWorld.m[1][0], -vob->trafoObjToWorld.m[2][0]), dontMove);
}

float zCAIPlayer::GetProtoYHeight()
{
    return model->bbox3DLocalFixed.maxs.n[1] - model->bbox3DLocalFixed.mins.n[1];
}

zVEC3 zCAIPlayer::GetSurfaceAlignScanOrigin()
{
    if ((zMV_DO_SURFACE_ALIGN & 3) == 2) {
        const float height = GetProtoYHeight() * 0.65f;
        if (world->TraceRayNearestHit(centerPos, zVEC3(0.0f, height, 0.0f), vob, 0x8a1)) {
            return centerPos + (world->traceRayReport.foundIntersection - centerPos) * 0.8f;
        }
    }
    return centerPos;
}

void zCAIPlayer::DiveRotateX(const float& angle)
{
    float direction = vob->trafoObjToWorld.m[1][2];
    if (direction < -1.0f)
        direction = -1.0f;
    else if (direction > 1.0f)
        direction = 1.0f;

    const float current = std::acos(direction) * 57.2957764f;
    float target;
    if (vob->trafoObjToWorld.m[1][2] < 0.0f)
        target = current + angle < 80.0f ? current + angle : 80.0f;
    else
        target = current - angle > -80.0f ? current - angle : -80.0f;
    vob->RotateLocalX(target - current);
}

void zCAIPlayer::SetCDStat(int enabled)
{
    if (zMV_DO_SURFACE_ALIGN & 0x20)
        zMV_DO_SURFACE_ALIGN = (zMV_DO_SURFACE_ALIGN & ~0x40) | (enabled ? 0x40 : 0);
    else
        zMV_DO_SURFACE_ALIGN &= ~0x40;
    vob->SetCollDetStat((zMV_DO_SURFACE_ALIGN & 0x40) != 0);
}

void zCAIPlayer::LandAndStartAni(const zSTRING& aniName)
{
    if (!CheckFloorSliding() && aboveFloor <= config.zMV_STEP_HEIGHT) {
        model->StartAni(aniName, 0);
        vob->SetPhysicsEnabled(0);
        vob->ResetXZRotationsWorld();
        state = zMV_STATE_STAND;
        if (!waterLevel)
            zMV_DO_SURFACE_ALIGN |= 0x80;
    }
}

void zCAIPlayer::LandAndStartAni(zCModelAni* ani)
{
    if (!CheckFloorSliding() && aboveFloor <= config.zMV_STEP_HEIGHT) {
        model->StartAni(ani, 0);
        vob->SetPhysicsEnabled(0);
        vob->ResetXZRotationsWorld();
        state = zMV_STATE_STAND;
        if (!waterLevel)
            zMV_DO_SURFACE_ALIGN |= 0x80;
    }
}

int zCAIPlayer::CheckForceModelHalt(const zSTRING& aniName)
{
    if (!forceModelHalt)
        return 0;
    model->StartAni(aniName, 0);
    forceModelHalt = 0;
    return 1;
}

int zCAIPlayer::CheckForceModelHalt(zCModelAni* ani)
{
    if (!forceModelHalt)
        return 0;
    model->StartAni(ani, 0);
    forceModelHalt = 0;
    return 1;
}

int zCAIPlayer::ShouldCorrectFloorHeight()
{
    return aboveFloor <= config.zMV_STEP_HEIGHT;
}

void zCAIPlayer::BeginStateSwitch()
{
    if (state != zMV_STATE_FLY) {
        fallDownStartY = centerPos.n[1];
        fallDownDistanceY = 0.0f;
    }
    if ((vob->showVisual & 8) && !IsSliding())
        fallDownDistanceY = fallDownStartY - centerPos.n[1];
}

void zCAIPlayer::SetPhysicsEnabled(int enabled)
{
    vob->SetPhysicsEnabled(enabled);
}

void zCAIPlayer::StartPhysicsWithVel()
{
    vob->SetPhysicsEnabled(1);
    vob->GetRigidBody()->SetVelocity(velocity);
}

void zCAIPlayer::StartStandAni()
{
    model->StartAni("S_STAND", 0);
}

void zCAIPlayer::StartFallDownAni()
{
    model->StartAni("S_FALLDN", 0);
}

void zCAIPlayer::Archive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame() || !vob || !vob->GetCollisionObject())
        return;

    archiver.WriteInt("waterLevel", waterLevel);
    archiver.WriteFloat("floorY", 0.0f);
    archiver.WriteFloat("waterY", 0.0f);
    archiver.WriteFloat("ceilingY", 0.0f);
    archiver.WriteFloat("feetY", feetY);
    archiver.WriteFloat("headY", headY);
    archiver.WriteFloat("fallDownDistanceY", fallDownDistanceY);
    archiver.WriteFloat("fallDownStartY", fallDownStartY);
}

void zCAIPlayer::Unarchive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;

    zCObject* parent = archiver.ReadObject(NULL);
    if (!parent || !zCObject::CheckInheritance(zCVob::classDef, parent->_GetClassDef()))
        return;

    zCVob* savedVob = static_cast<zCVob*>(parent);
    if (!savedVob->GetCollisionObject())
        savedVob->CreateCollisionObject();

    waterLevel = archiver.ReadInt("waterLevel");
    archiver.ReadFloat("floorY");
    archiver.ReadFloat("waterY");
    archiver.ReadFloat("ceilingY");
    feetY = archiver.ReadFloat("feetY");
    headY = archiver.ReadFloat("headY");
    fallDownDistanceY = archiver.ReadFloat("fallDownDistanceY");
    fallDownStartY = archiver.ReadFloat("fallDownStartY");
}

void zCAIPlayer::Print(const zSTRING& text) const
{
}

void zCAIPlayer::Line3D(const zVEC3& start, const zVEC3& end, int mode) const
{
}

void zCAIPlayer::PrintScreen(int x, int y, zSTRING text) const
{
}

void zCSparseArrayIterator::Next()
{
    while (count > 0) {
        count--;
        if (array->ManDatPtr(count)->occupied)
            return;
    }
    if (registered)
        DeRegister();
}

void zCSparseArrayIterator::Register()
{
    if (registered)
        return;
    registered = 1;
    array->numberOfIterators++;
}

void zCSparseArrayIterator::DeRegister()
{
    if (!registered)
        return;
    registered = 0;
    array->numberOfIterators--;
}

zCSparseArrayIterator::operator int()
{
    if (count < 0 && registered)
        DeRegister();
    return count;
}
