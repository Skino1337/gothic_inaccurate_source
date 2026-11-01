#include "include/oCollisionObjectMiscChar.h"
#include <cmath>

static const float NPC_RAD_TO_DEG = 57.2957764f;
static const float NPC_HALF_PI = 1.57079637f;

static float NormalizeAngle(float angle)
{
    angle *= NPC_RAD_TO_DEG;
    if (angle < -180.0f)
        return angle + 360.0f;
    if (angle > 180.0f)
        return angle - 360.0f;
    return angle;
}

int oCNpc::CanDive()
{
    return divetime >= 0.0f || divetime >= -1000000.0f;
}

int oCNpc::CanSwim()
{
    return swimtime >= 0.0f || swimtime >= -1000000.0f;
}

float oCNpc::GetClimbRange()
{
    return 500.0f;
}

float oCNpc::GetTurnSpeed()
{
    return speedTurn;
}

float oCNpc::GetJumpRange()
{
    return 300.0f;
}

int oCNpc::RbtChooseChasmAction(const zVEC3& origin, const zVEC3& direction)
{
    float chasmDepth = direction.n[1];
    if (chasmDepth < GetFallDownHeight() * 0.5f)
        return 1;
    if (chasmDepth < GetFallDownHeight())
        return 4;
    return 9;
}

int oCNpc::CanGo(float distance, zVEC3& obstacleNormal)
{
    rbt.obstVob = 0;
    rbt.flags &= ~0x40;
    if (!anictrl)
        return 0;
    if (anictrl->actionMode == 5)
        distance *= 0.5f;
    if (rbt.obstVob) {
        obstacleNormal.n[0] = -trafoObjToWorld.m[0][2];
        obstacleNormal.n[1] = -trafoObjToWorld.m[1][2];
        obstacleNormal.n[2] = -trafoObjToWorld.m[2][2];
        return 2;
    }
    if (distance < 0.0f)
        return 2;
    return 0;
}

float oCNpc::GetAngle(zCVob* vob)
{
    if (!vob)
        return 0.0f;
    zVEC3 forward = {
        trafoObjToWorld.m[0][2], 0.0f, trafoObjToWorld.m[2][2]
    };
    zVEC3 direction = {
        vob->trafoObjToWorld.m[0][3] - trafoObjToWorld.m[0][3],
        0.0f,
        vob->trafoObjToWorld.m[2][3] - trafoObjToWorld.m[2][3]
    };
    float forwardLength = std::sqrt(forward.n[0] * forward.n[0]
        + forward.n[2] * forward.n[2]);
    float directionLength = std::sqrt(direction.n[0] * direction.n[0]
        + direction.n[2] * direction.n[2]);
    float dot = (forward.n[0] * direction.n[0] + forward.n[2] * direction.n[2])
        / (forwardLength * directionLength);
    return std::acos(dot) * NPC_RAD_TO_DEG;
}

void Alg_CalcAziElevUnit(const zVEC3& direction, float& azimuth, float& elevation)
{
    elevation = std::acos(direction.n[1]);
    float sine = std::sin(elevation);
    float inverseSine = sine == 0.0f ? 0.00001f : 1.0f / sine;
    azimuth = std::atan2(direction.n[2] * inverseSine, direction.n[0] * inverseSine);
    elevation = NPC_HALF_PI - elevation;
}

void oCNpc::GetAngles(zVEC3& position, float& azimuth, float& elevation)
{
    zVEC3 forward = {
        trafoObjToWorld.m[0][2], trafoObjToWorld.m[1][2], trafoObjToWorld.m[2][2]
    };
    if (GetBodyState() == 0x0b) {
        forward.n[0] = -forward.n[0];
        forward.n[1] = -forward.n[1];
        forward.n[2] = -forward.n[2];
    }
    zVEC3 direction = {
        position.n[0] - trafoObjToWorld.m[0][3],
        position.n[1] - trafoObjToWorld.m[1][3],
        position.n[2] - trafoObjToWorld.m[2][3]
    };
    float length = std::sqrt(direction.n[0] * direction.n[0]
        + direction.n[1] * direction.n[1] + direction.n[2] * direction.n[2]);
    direction.n[0] /= length;
    direction.n[1] /= length;
    direction.n[2] /= length;
    float targetAzimuth;
    float targetElevation;
    Alg_CalcAziElevUnit(direction, targetAzimuth, targetElevation);
    float forwardAzimuth = std::atan2(forward.n[2], forward.n[0]);
    float forwardElevation = std::acos(forward.n[1]);
    azimuth = NormalizeAngle(targetAzimuth - forwardAzimuth);
    elevation = NormalizeAngle(targetElevation - (NPC_HALF_PI - forwardElevation));
}

void oCNpc::GetAngles(zCVob* vob, float& azimuth, float& elevation)
{
    zVEC3 position = {
        vob->trafoObjToWorld.m[0][3],
        vob->trafoObjToWorld.m[1][3],
        vob->trafoObjToWorld.m[2][3]
    };
    GetAngles(position, azimuth, elevation);
}

float oCNpc::GetDistanceToPos2(zVEC3& position, int useBoundingBox)
{
    zVEC3 origin = {
        trafoObjToWorld.m[0][3], trafoObjToWorld.m[1][3], trafoObjToWorld.m[2][3]
    };
    if (useBoundingBox) {
        for (int i = 0; i < 3; ++i) {
            if (position[i] < bbox3D.mins[i])
                origin[i] = bbox3D.mins[i];
            else if (position[i] > bbox3D.maxs[i])
                origin[i] = bbox3D.maxs[i];
            else
                origin[i] = position[i];
        }
    }
    float x = position.n[0] - origin.n[0];
    float y = position.n[1] - origin.n[1];
    float z = position.n[2] - origin.n[2];
    return x * x + y * y + z * z;
}

float oCNpc::GetFallDownHeight()
{
    if (overrideFallDownHeight)
        return 0.0f;
    return fallDownHeight;
}

zCVob* oCNpc::GetRbtObstacleVob()
{
    return rbt.obstVob;
}

zVEC3 oCNpc::GetVecNormalFromBBox(zCVob* vob, zVEC3& position)
{
    zVEC3 normal = { 0.0f, 0.0f, 1.0f };
    if (!vob)
        return normal;
    float distances[6] = {
        std::fabs(position.n[0] - vob->bbox3D.mins.n[0]),
        std::fabs(position.n[0] - vob->bbox3D.maxs.n[0]),
        std::fabs(position.n[1] - vob->bbox3D.mins.n[1]),
        std::fabs(position.n[1] - vob->bbox3D.maxs.n[1]),
        std::fabs(position.n[2] - vob->bbox3D.mins.n[2]),
        std::fabs(position.n[2] - vob->bbox3D.maxs.n[2])
    };
    int side = 0;
    for (int i = 1; i < 6; ++i) {
        if (distances[i] < distances[side])
            side = i;
    }
    normal.n[0] = side == 0 ? -1.0f : side == 1 ? 1.0f : 0.0f;
    normal.n[1] = side == 2 ? -1.0f : side == 3 ? 1.0f : 0.0f;
    normal.n[2] = side == 4 ? -1.0f : side == 5 ? 1.0f : 0.0f;
    return normal;
}

void oCNpc::SetFallDownDamage(int damage)
{
    fallDownDamage = damage;
}

void oCNpc::SetFallDownHeight(float height)
{
    fallDownHeight = height;
}

void oCNpc::SetRoute(zCRoute* newRoute)
{
    delete route;
    route = newRoute;
}

extern zTIMER ztimer;

void oCNpc::CreateFallDamage(float distance)
{
    oSDamageDescriptor descriptor;
    descriptor.enuModeDamage = 0x80;
    descriptor.dwFieldsValid = 0x280;
    descriptor.fDamageMultiplier = 1.0f;
    descriptor.fDamageTotal = (distance + 50.0f - GetFallDownHeight()) * 0.01f * fallDownDamage;
    OnDamage(descriptor);
}

void oCNpc::Fleeing()
{
    ThinkNextFleeAction();
}

int oCNpc::ThinkNextFleeAction()
{
    if (!enemy || !FreeLineOfSight(enemy))
        return 0;
    if (!foundFleePoint) {
        vecFlee.n[0] = (enemy->trafoObjToWorld.m[0][3] - trafoObjToWorld.m[0][3]) * -2.0f;
        vecFlee.n[1] = (enemy->trafoObjToWorld.m[1][3] - trafoObjToWorld.m[1][3]) * -2.0f;
        vecFlee.n[2] = (enemy->trafoObjToWorld.m[2][3] - trafoObjToWorld.m[2][3]) * -2.0f;
        posFlee.n[0] = trafoObjToWorld.m[0][3] + vecFlee.n[0];
        posFlee.n[1] = trafoObjToWorld.m[1][3] + vecFlee.n[1];
        posFlee.n[2] = trafoObjToWorld.m[2][3] + vecFlee.n[2];
        waypointFlee = 0;
        reachedFleePoint = 0;
        foundFleePoint = 1;
        rbt.targetPos = posFlee;
        RbtUpdate();
    } else if (!reachedFleePoint) {
        if (trafoObjToWorld.m[0][3] == posFlee.n[0]
            && trafoObjToWorld.m[1][3] == posFlee.n[1]
            && trafoObjToWorld.m[2][3] == posFlee.n[2]) {
            reachedFleePoint = 1;
        } else {
            rbt.flags = (rbt.flags & ~1) | 0x10;
            rbt.maxTargetDist = 10000.0f;
            RobustTrace();
        }
    } else {
        foundFleePoint = 0;
        reachedFleePoint = 0;
    }
    return 0;
}

void oCNpc::Follow()
{
}

void oCNpc::ResetPos(zVEC3& position)
{
    Interrupt(0, 1);
    SetCollDetStat(0);
    SetCollDetDyn(0);
    SetPositionWorld(position);
    attribute[2] = attribute[3];
    attribute[0] = attribute[1];
    anictrl = GetAnictrl();
    if (anictrl) {
        anictrl->Reset();
        anictrl->SetActionMode(1);
    }
    ClearEM();
    SetBodyState(0);
    SetCollDetStat(1);
    SetCollDetDyn(1);
    SetMovLock(0);
    CheckModelOverlays();
    divectr = divetime;
}

void oCNpc::EmergencyResetPos(zVEC3& position)
{
    zVEC3 resetPosition = position;
    resetPosition.n[1] += 150.0f;
    ResetPos(resetPosition);
    ResetRotationsWorld();
}

void oCNpc::StandUp(int disableCurrent, int enableNext)
{
    anictrl = GetAnictrl();
    if (GetInteractMob() || interactItem)
        Interrupt(0, 1);
    if (anictrl) {
        if (anictrl->actionMode == 5) {
            SetBodyState(5);
        } else if (anictrl->actionMode == 6) {
            SetBodyState(7);
        } else {
            SetBodyState(0);
            anictrl->StopTurnAnis();
            if (!disableCurrent)
                anictrl->SetActionMode(enableNext);
        }
    }
    RbtReset();
}

float oCNpc::Turn(zVEC3& position)
{
    float azimuth;
    float elevation;
    GetAngles(position, azimuth, elevation);
    float step = ztimer.frameTimeFloat * GetTurnSpeed();
    if (std::fabs(azimuth) > 30.0f || (this == player && GetWeaponMode()))
        step *= 2.0f;
    float rotation = azimuth;
    if (rotation > step)
        rotation = step;
    else if (rotation < -step)
        rotation = -step;
    if (anictrl)
        anictrl->TurnDegrees(rotation, std::fabs(rotation) > 5.0f);
    return std::fabs(azimuth);
}

float oCNpc::Turning(zVEC3& position)
{
    float azimuth;
    float elevation;
    GetAngles(position, azimuth, elevation);
    float step = ztimer.frameTimeFloat * GetTurnSpeed();
    float rotation = azimuth;
    if (rotation > step)
        rotation = step;
    else if (rotation < -step)
        rotation = -step;
    if (anictrl) {
        anictrl->TurnDegrees(rotation, 1);
        if (std::fabs(azimuth) < 5.0f)
            anictrl->StopTurnAnis();
    }
    return azimuth;
}

void oCNpc::AI_Flee(oCNpc* npc)
{
}

void oCNpc::AI_Follow(oCNpc* npc)
{
}
