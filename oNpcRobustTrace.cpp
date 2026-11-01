#include "include/oCollisionObjectMiscChar.h"
#include <cmath>

extern zTIMER ztimer;

int oCNpc::RbtMoveToExactPosition()
{
    if (rbt.targetDist >= rbt.lastTargetDist) {
        rbt.lastTargetDist = rbt.targetDist;
        return 0;
    }
    SetPositionWorld(rbt.targetPos);
    rbt.flags = (rbt.flags & ~3) | 8;
    rbt.lastDirections.numInArray = 0;
    return 1;
}

int oCNpc::RobustTrace()
{
    float x = rbt.targetPos.n[0] - trafoObjToWorld.m[0][3];
    float y = rbt.targetPos.n[1] - trafoObjToWorld.m[1][3];
    float z = rbt.targetPos.n[2] - trafoObjToWorld.m[2][3];
    rbt.targetDist = x * x + y * y + z * z;
    zVEC3 origin = {
        trafoObjToWorld.m[0][3], trafoObjToWorld.m[1][3], trafoObjToWorld.m[2][3]
    };
    RbtCheckForSolution(origin, rbt.targetPos);
    if (rbt.targetDist <= rbt.maxTargetDist)
        rbt.flags |= 1;
    if (!(rbt.flags & 0x10) && (rbt.flags & 1)) {
        rbt.flags = (rbt.flags & ~2) | 8;
        rbt.failurePossibility = 0.0f;
        return 1;
    }
    if ((rbt.flags & 4) && (rbt.flags & 1)) {
        rbt.failurePossibility = 0.0f;
        RbtMoveToExactPosition();
    } else if ((rbt.flags & 0x10) && (rbt.flags & 1)) {
        anictrl->StopTurnAnis();
        anictrl->_Stand();
        if (anictrl->IsStanding()) {
            rbt.flags = (rbt.flags & ~3) | 8;
            rbt.failurePossibility = 0.0f;
            return 1;
        }
    }
    rbt.timer -= ztimer.frameTimeFloat;
    if (rbt.timer < 0.0f) {
        anictrl->StopTurnAnis();
        anictrl->_Stand();
        if (anictrl->IsStanding()) {
            rbt.flags &= ~0x0b;
            rbt.failurePossibility = 0.0f;
            return 1;
        }
        return 0;
    }
    RbtCheckIfTargetVisible();
    RbtAvoidObstacles();
    return 0;
}

float oCNpc::RbtCalcTurnDirection(const zVEC3& direction, float distance, int action)
{
    zVEC3 targetDirection = {
        rbt.targetPos.n[0] - trafoObjToWorld.m[0][3], 0.0f,
        rbt.targetPos.n[2] - trafoObjToWorld.m[2][3]
    };
    float side = targetDirection.n[0] * direction.n[2]
        - targetDirection.n[2] * direction.n[0];
    if (side > 0.0f)
        return 1.0f;
    if (side < 0.0f)
        return -1.0f;
    return action == 4 ? -1.0f : 1.0f;
}

int oCNpc::RbtIsObjectObstacle(zCVob* vob, const zVEC3& origin, const zVEC3& direction,
    zVEC3& normal)
{
    if (!vob || dynamic_cast<oCNpc*>(vob))
        return 0;
    normal = GetVecNormalFromBBox(vob, const_cast<zVEC3&>(origin));
    float dot = normal.n[0] * direction.n[0] + normal.n[1] * direction.n[1]
        + normal.n[2] * direction.n[2];
    if (dot > 0.0f) {
        normal.n[0] = -normal.n[0];
        normal.n[1] = -normal.n[1];
        normal.n[2] = -normal.n[2];
    }
    return 1;
}

int oCNpc::RbtGetObstacle(const zVEC3& origin, const zVEC3& direction, zCVob*& obstacle,
    zVEC3& normal)
{
    obstacle = 0;
    if (!homeWorld || !homeWorld->TraceRayNearestHit(origin, direction, this, 0xa5))
        return 0;
    obstacle = homeWorld->traceRayReport.foundVob;
    normal = homeWorld->traceRayReport.foundPolyNormal;
    if (!obstacle)
        return 1;
    return RbtIsObjectObstacle(obstacle, origin, direction, normal);
}

int oCNpc::RbtGetSmallObstacle(const zVEC3& direction, zCVob*& obstacle, zVEC3& normal)
{
    obstacle = 0;
    zVEC3 origin = {
        trafoObjToWorld.m[0][3], trafoObjToWorld.m[1][3], trafoObjToWorld.m[2][3]
    };
    if (!homeWorld || !homeWorld->TraceRayNearestHit(origin, direction, this, 0x8a1))
        return 0;
    obstacle = homeWorld->traceRayReport.foundVob;
    if (!obstacle || obstacle == rbt.targetVob || dynamic_cast<oCNpc*>(obstacle))
        return 0;
    normal = homeWorld->traceRayReport.foundPolyNormal;
    return 1;
}

int oCNpc::RbtAvoidObstacles()
{
    float distance = (bbox3D.maxs.n[0] - bbox3D.mins.n[0]) * 0.5f + 100.0f;
    zVEC3 normal = { 0.0f, 0.0f, 0.0f };
    int action = CanGo(distance, normal);
    ++rbt.frameCtr;
    if (action == 3 || action == 9) {
        anictrl->JumpForward();
    } else if (action == 2 || action == 4) {
        if (!(rbt.flags & 2)) {
            rbt.dirTurn = RbtCalcTurnDirection(normal, distance, action);
            rbt.flags |= 2;
        }
        float rotation = GetTurnSpeed() * ztimer.frameTimeFloat;
        ForceRotation(rbt.dirTurn < 0.0f ? -rotation : rotation);
        if (RbtIsDirectionValid(normal))
            RbtInsertDirection(normal);
    } else {
        Turn(rbt.targetPos);
    }
    anictrl->_Forward();
    return 0;
}

int oCNpc::RbtIsDirectionValid(const zVEC3& direction)
{
    if (rbt.lastDirections.numInArray > 0)
        return 1;
    float dot = (rbt.targetPos.n[0] - trafoObjToWorld.m[0][3]) * direction.n[0]
        + (rbt.targetPos.n[1] - trafoObjToWorld.m[1][3]) * direction.n[1]
        + (rbt.targetPos.n[2] - trafoObjToWorld.m[2][3]) * direction.n[2];
    return dot > 0.0f;
}

oSDirectionInfo* oCNpc::RbtInsertDirection(zVEC3 direction)
{
    direction.n[1] = 0.0f;
    if (direction.n[0] == 0.0f && direction.n[2] == 0.0f)
        return 0;
    oSDirectionInfo* info = new oSDirectionInfo;
    info->direction = direction;
    info->flags = 0;
    rbt.lastDirections.InsertEnd(info);
    return info;
}

int oCNpc::RbtCheckLastDirection(float distance)
{
    int count = rbt.lastDirections.numInArray;
    if (count <= 0)
        return 0;
    oSDirectionInfo* info = rbt.lastDirections.parray[count - 1];
    zVEC3 target = {
        trafoObjToWorld.m[0][3] + info->direction.n[0] * distance,
        trafoObjToWorld.m[1][3] + info->direction.n[1] * distance,
        trafoObjToWorld.m[2][3] + info->direction.n[2] * distance
    };
    if (Turn(target) >= 1.0f)
        return 0;
    rbt.lastDirections.Remove(info);
    delete info;
    return 1;
}

int oCNpc::RbtCheckIfTargetVisible()
{
    rbt.checkVisibilityTime += ztimer.frameTimeFloat;
    if (rbt.checkVisibilityTime >= 500.0f) {
        rbt.checkVisibilityTime -= 500.0f;
        int visible = rbt.targetVob ? FreeLineOfSight(rbt.targetVob) : 1;
        if (visible) {
            rbt.flags |= 0x80;
            if (rbt.targetVob) {
                rbt.targetPos.n[0] = rbt.targetVob->trafoObjToWorld.m[0][3];
                rbt.targetPos.n[1] = rbt.targetVob->trafoObjToWorld.m[1][3];
                rbt.targetPos.n[2] = rbt.targetVob->trafoObjToWorld.m[2][3];
                rbt.targetPosArray[0] = rbt.targetPos;
                rbt.targetPosCounter = 0;
                rbt.targetPosIndex = 0;
                rbt.positionUpdateTime = 500.0f;
            }
        } else {
            rbt.flags &= ~0x80;
        }
    }
    rbt.positionUpdateTime += ztimer.frameTimeFloat;
    if (rbt.targetVob && !(rbt.flags & 0x80) && rbt.positionUpdateTime >= 500.0f) {
        rbt.positionUpdateTime -= 500.0f;
        if (rbt.targetPosCounter < 4)
            ++rbt.targetPosCounter;
        zVEC3& position = rbt.targetPosArray[rbt.targetPosCounter];
        position.n[0] = rbt.targetVob->trafoObjToWorld.m[0][3];
        position.n[1] = rbt.targetVob->trafoObjToWorld.m[1][3];
        position.n[2] = rbt.targetVob->trafoObjToWorld.m[2][3];
    }
    return (rbt.flags & 0x80) != 0;
}

int oCNpc::RbtGotoFollowPosition()
{
    if (!rbt.targetVob)
        return 1;
    if (rbt.targetPosIndex < 0 || rbt.targetPosIndex > 4) {
        rbt.targetPos.n[0] = rbt.targetVob->trafoObjToWorld.m[0][3];
        rbt.targetPos.n[1] = rbt.targetVob->trafoObjToWorld.m[1][3];
        rbt.targetPos.n[2] = rbt.targetVob->trafoObjToWorld.m[2][3];
    } else {
        rbt.targetPos = rbt.targetPosArray[rbt.targetPosIndex];
    }
    rbt.flags &= ~8;
    if (rbt.targetPosIndex < 0 || rbt.targetPosIndex == rbt.targetPosCounter)
        rbt.flags |= 0x10;
    else
        rbt.flags &= ~0x10;
    if (rbt.targetPosIndex > 0)
        rbt.maxTargetDist = 6400.0f;
    int result = RobustTrace();
    if (result && rbt.targetPosIndex >= 0) {
        ++rbt.targetPosIndex;
        rbt.flags &= ~2;
        rbt.lastDirections.numInArray = 0;
        if (rbt.targetPosIndex > rbt.targetPosCounter) {
            rbt.targetPosIndex = 0;
            rbt.targetPosCounter = 0;
            rbt.targetPosArray[0].n[0] = rbt.targetVob->trafoObjToWorld.m[0][3];
            rbt.targetPosArray[0].n[1] = rbt.targetVob->trafoObjToWorld.m[1][3];
            rbt.targetPosArray[0].n[2] = rbt.targetVob->trafoObjToWorld.m[2][3];
        }
    }
    return result;
}

int oCNpc::CanStrafe(int right, int ani)
{
    anictrl = GetAnictrl();
    if (!anictrl)
        return 0;
    if (right)
        return anictrl->CheckEnoughSpaceMoveRight(0.0f);
    return anictrl->CheckEnoughSpaceMoveLeft(0.0f);
}

int oCNpc::CanJumpBack(int ani)
{
    anictrl = GetAnictrl();
    if (!anictrl)
        return 0;
    return anictrl->CheckEnoughSpaceMoveBackward(0.0f);
}

void oCNpc::SetWalkStopChasm(int enabled)
{
    oCAniCtrl_Human* control = GetAnictrl();
    if (control)
        control->walkStopChasm = enabled;
}

int oCNpc::GetWalkStopChasm()
{
    oCAniCtrl_Human* control = GetAnictrl();
    return control && control->walkStopChasm;
}
