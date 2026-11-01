#include "include/oCollisionObjectMiscChar.h"
#include "include/oNpcMessages.h"
#include <cmath>

extern zTIMER ztimer;

int oCNpc::EV_AlignToFP(oCMsgMovement* message)
{
    if (!message->targetVob)
        return 1;
    message->SetInUse(1);
    zCVob* target = message->targetVob;
    zVEC3 position = {
        trafoObjToWorld.m[0][3] + target->trafoObjToWorld.m[0][2] * 200.0f,
        trafoObjToWorld.m[1][3] + target->trafoObjToWorld.m[1][2] * 200.0f,
        trafoObjToWorld.m[2][3] + target->trafoObjToWorld.m[2][2] * 200.0f
    };
    return Turn(position) < 1.0f;
}

int oCNpc::EV_CanSeeNpc(oCMsgMovement* message)
{
    if (message->targetMode >= 0 && CanSee(message->targetVob, 0)) {
        ClearEM();
        return 0;
    }
    return 1;
}

int oCNpc::EV_GoRoute(oCMsgMovement* message)
{
    if (!anictrl->IsWalking() && !anictrl->IsStanding())
        StandUp(0, 1);
    if (!message->IsInUse()) {
        RbtReset();
        message->SetInUse(1);
        if (!route)
            return 1;
        rbt.flags |= 8;
    }
    if ((rbt.flags & 8) && route) {
        rbt.flags &= ~8;
        return 0;
    }
    if (route && !(rbt.flags & 0x20)) {
        message->SetInUse(0);
        return 0;
    }
    SetRoute(0);
    anictrl->StopTurnAnis();
    return anictrl->IsStanding();
}

int oCNpc::EV_Jump(oCMsgMovement* message)
{
    if (message->IsInUse()) {
        if (anictrl->IsStanding()) {
            SetCollDetStat(1);
            SetCollDetDyn(1);
            return 1;
        }
    } else {
        anictrl->JumpForward();
        message->SetInUse(1);
    }
    return 0;
}

int oCNpc::EV_RobustTrace(oCMsgMovement* message)
{
    if (!message->IsInUse()) {
        RbtReset();
        RbtUpdate(message->targetPos, message->targetVob);
        rbt.flags = ((message->targetMode & 1) << 4) ^ (rbt.flags & ~0x14);
        message->SetInUse(1);
    }
    return RobustTrace();
}

int oCNpc::EV_StandUp(oCMsgMovement* message)
{
    if (!message->IsInUse()) {
        StandUp(0, message->targetMode);
        message->SetInUse(1);
    }
    if (anictrl->actionMode == 5 || anictrl->actionMode == 6 || GetTrueGuild() == 0x21)
        return 1;
    return anictrl->IsStanding();
}

int oCNpc::EV_Strafe(oCMsgMovement* message)
{
    if (!message->IsInUse()) {
        StandUp(1, 1);
        message->SetInUse(1);
        message->timer = 500.0f;
    }
    if (message->targetVob) {
        zVEC3 position = {
            message->targetVob->trafoObjToWorld.m[0][3],
            message->targetVob->trafoObjToWorld.m[1][3],
            message->targetVob->trafoObjToWorld.m[2][3]
        };
        Turn(position);
    }
    message->timer -= ztimer.frameTimeFloat;
    if (message->timer <= 0.0f || anictrl->IsStanding())
        return 1;
    return 0;
}

int oCNpc::EV_WhirlAround(oCMsgMovement* message)
{
    if (!message->targetVob)
        return 1;
    if (!message->IsInUse()) {
        StandUp(0, 1);
        anictrl->StopTurnAnis();
        message->targetPos.n[0] = message->targetVob->trafoObjToWorld.m[0][3];
        message->targetPos.n[1] = message->targetVob->trafoObjToWorld.m[1][3];
        message->targetPos.n[2] = message->targetVob->trafoObjToWorld.m[2][3];
        message->timer = 500.0f;
        message->SetInUse(1);
    }
    float remaining = Turn(message->targetPos);
    message->timer -= ztimer.frameTimeFloat;
    if (remaining < 1.0f || message->timer <= 0.0f) {
        anictrl->StopTurnAnis();
        return 1;
    }
    return 0;
}

int oCNpc::EV_Dodge(oCMsgMovement* message)
{
    if (!message->IsInUse()) {
        StandUp(1, 1);
        message->SetInUse(1);
        message->ani = -1;
        message->timer = 300.0f;
        return 0;
    }
    message->timer -= ztimer.frameTimeFloat;
    return message->ani == -1 || message->timer <= 0.0f;
}

int oCNpc::EV_GotoPos(oCMsgMovement* message)
{
    StandUp(1, 1);
    if (!message->IsInUse()) {
        RbtReset();
        RbtUpdate(message->targetPos, 0);
        rbt.flags = (rbt.flags & ~4) | 0x10;
        rbt.maxTargetDist = 10000.0f;
        message->SetInUse(1);
    }
    return RobustTrace();
}

int oCNpc::EV_GotoVob(oCMsgMovement* message)
{
    if (!message->targetVob)
        return 1;
    rbt.timer = 500.0f;
    rbt.targetVob = message->targetVob;
    rbt.targetPos.n[0] = message->targetVob->trafoObjToWorld.m[0][3];
    rbt.targetPos.n[1] = message->targetVob->trafoObjToWorld.m[1][3];
    rbt.targetPos.n[2] = message->targetVob->trafoObjToWorld.m[2][3];
    rbt.flags |= 0x10;
    rbt.maxTargetDist = 40000.0f;
    return RbtGotoFollowPosition();
}

int oCNpc::EV_GotoFP(oCMsgMovement* message)
{
    StandUp(1, 1);
    if (!message->targetVob)
        message->targetVob = FindSpot(message->targetName, message->targetMode, 700.0f);
    if (!message->targetVob)
        return 1;
    if (!message->IsInUse()) {
        RbtReset();
        RbtUpdate(message->targetPos, message->targetVob);
        rbt.flags = (rbt.flags & ~4) | 0x10;
        rbt.maxTargetDist = 2500.0f;
        message->SetInUse(1);
    }
    if (!RobustTrace())
        return 0;
    zCVob* target = message->targetVob;
    zVEC3 position = {
        trafoObjToWorld.m[0][3] + target->trafoObjToWorld.m[0][2] * 200.0f,
        trafoObjToWorld.m[1][3] + target->trafoObjToWorld.m[1][2] * 200.0f,
        trafoObjToWorld.m[2][3] + target->trafoObjToWorld.m[2][2] * 200.0f
    };
    return std::fabs(Turn(position)) < 3.0f;
}

int oCNpc::EV_SetWalkMode(oCMsgMovement* message)
{
    anictrl = GetAnictrl();
    if (anictrl->GetWaterLevel() > 0)
        return 1;
    if (!message->IsInUse()) {
        if (anictrl->walkmode == message->targetMode)
            return 1;
        StandUp(0, 1);
        message->SetInUse(1);
    }
    anictrl->SetWalkMode(message->targetMode);
    if (GetInteractMob() || interactItem)
        Interrupt(0, 1);
    anictrl->_Stand();
    return 1;
}

int oCNpc::EV_Turn(oCMsgMovement* message)
{
    float step = ztimer.frameTimeFloat * GetTurnSpeed();
    float rotation = message->angle;
    if (rotation > step)
        rotation = step;
    else if (rotation < -step)
        rotation = -step;
    message->angle -= rotation;
    anictrl->TurnDegrees(rotation, 1);
    if (message->angle != 0.0f)
        return 0;
    anictrl->StopTurnAnis();
    return 1;
}

int oCNpc::EV_TurnAway(oCMsgMovement* message)
{
    if (!message->targetVob || GetBodyState())
        return 1;
    if (!anictrl->IsStanding())
        StandUp(0, 1);
    message->targetPos.n[0] = trafoObjToWorld.m[0][3] * 2.0f
        - message->targetVob->trafoObjToWorld.m[0][3];
    message->targetPos.n[1] = trafoObjToWorld.m[1][3] * 2.0f
        - message->targetVob->trafoObjToWorld.m[1][3];
    message->targetPos.n[2] = trafoObjToWorld.m[2][3] * 2.0f
        - message->targetVob->trafoObjToWorld.m[2][3];
    float elevation;
    GetAngles(message->targetPos, message->angle, elevation);
    return EV_Turn(message);
}

int oCNpc::EV_TurnToPos(oCMsgMovement* message)
{
    float elevation;
    GetAngles(message->targetPos, message->angle, elevation);
    return EV_Turn(message);
}

int oCNpc::EV_TurnToVob(oCMsgMovement* message)
{
    if (!message->targetVob || (GetBodyState() && GetBodyState() != 3))
        return 1;
    if (!anictrl->IsStanding())
        StandUp(0, 1);
    message->targetPos.n[0] = message->targetVob->trafoObjToWorld.m[0][3];
    message->targetPos.n[1] = message->targetVob->trafoObjToWorld.m[1][3];
    message->targetPos.n[2] = message->targetVob->trafoObjToWorld.m[2][3];
    float elevation;
    GetAngles(message->targetPos, message->angle, elevation);
    return EV_Turn(message);
}

void oCNpc::ForceRotation(float angle)
{
    anictrl->TurnDegrees(angle, 1);
}

int oCNpc::RbtCheckForSolution(zVEC3 origin, zVEC3 target)
{
    float x = target.n[0] - origin.n[0];
    float y = target.n[1] - origin.n[1];
    float z = target.n[2] - origin.n[2];
    float distance = x * x + y * y + z * z;
    if (distance <= rbt.maxTargetDist)
        return 1;
    rbt.failurePossibility += ztimer.frameTimeFloat * 0.0333333351f;
    return 0;
}

void oCNpc::RbtReset()
{
    rbt.targetPos.n[0] = 0.0f;
    rbt.targetPos.n[1] = 0.0f;
    rbt.targetPos.n[2] = 0.0f;
    rbt.targetDist = 3.40282347e+38f;
    rbt.lastTargetDist = 3.40282347e+38f;
    rbt.dirFirst.n[0] = 0.0f;
    rbt.dirFirst.n[1] = 1.0f;
    rbt.dirFirst.n[2] = 0.0f;
    rbt.targetVob = 0;
    rbt.obstVob = 0;
    rbt.dirTurn = 0.0f;
    rbt.dirLastAngle = 0.0f;
    rbt.timer = 0.0f;
    rbt.flags = (rbt.flags & 0xd0) | 0x10;
    rbt.frameCtr = 0;
    rbt.targetPosCounter = 0;
    rbt.targetPosIndex = -1;
    rbt.checkVisibilityTime = 0.0f;
    rbt.positionUpdateTime = 0.0f;
    rbt.failurePossibility = 0.0f;
    rbt.maxTargetDist = 22500.0f;
}

void oCNpc::RbtInit(zVEC3& position, zCVob* target)
{
    RbtReset();
    RbtUpdate(position, target);
}

void oCNpc::RbtUpdate(zVEC3& position, zCVob* target)
{
    rbt.targetPos = position;
    rbt.targetVob = target;
    rbt.targetPosArray[0] = position;
    float x = trafoObjToWorld.m[0][3] - position.n[0];
    float y = trafoObjToWorld.m[1][3] - position.n[1];
    float z = trafoObjToWorld.m[2][3] - position.n[2];
    rbt.lastTargetDist = x * x + y * y + z * z;
    rbt.timer = std::sqrt(x * x + z * z) * (anictrl->walkmode ? 20.0f : 10.0f);
    if (rbt.lastTargetDist <= rbt.maxTargetDist)
        rbt.flags |= 8;
    else
        rbt.flags &= ~8;
    if (!(rbt.flags & 8) && (rbt.flags & 0x10))
        rbt.flags &= ~1;
}
