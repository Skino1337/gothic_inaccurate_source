#include "include/zAiCamera.h"
#include <cmath>

zCMovementTracker* zCMovementTracker::GetTracker()
{
    static zCMovementTracker tracker;
    return &tracker;
}

zCMovementTracker::zCMovementTracker()
{
    zVEC3* positions[] = {
        &playerPos, &oldPlayerPos, &camPos, &playerHead, &playerHipLeft, &playerHipRight,
        &playerPosRing, &POI_Player, &oldCamPos, &oldPlayerPosMovement, &camPosRing,
        &sampledPlayerPos, &sampledPlayerPosRing, &oldSampledPlayerPosRing, &oldSampledPlayerPos,
        &playerPosMovementRing, &playerPosMovementLocal, &playerPosMovementLocalCam,
        &playerPosMovement, &playerFuturePos
    };
    for (int i = 0; i < sizeof(positions) / sizeof(positions[0]); ++i)
        *positions[i] = zVEC3(0.0f, 0.0f, 0.0f);

    movement = 0x5000;
    playerFuturePosDT = 0.5f;
    inertiaTrans = 0.1f;
    inertiaHead = 0.0f;
    inertiaRotTargetFOR = 0.1f;
    inertiaTargetTrans = 0.2f;
    inertiaTargetRot = 0.04f;
    oldDistToPlayer = 0.0f;
    actAzi = 0.0f;
    actElev = 0.0f;
    distToPlayer = 0.0f;
    playerVeloRange = 0.0f;
    oldAzi = 0.0f;
    oldElev = 0.0f;
    standing = 0;
    rotate = 1;
    numPreSamplesPlayerPos = 10;
    numSamplesCam = 2;
    numSamplesPlayer = 2;
    timeSamples = 100000.0f;
    frameTime = 0.0f;
    standTimer = 0.0f;
    bMouseUsed = 0;
    trafoTStoWS = zMAT4::s_identity;
    trafoWStoTS = zMAT4::s_identity;
    trafoWStoCS = zMAT4::s_identity;
    trafoCStoWS = zMAT4::s_identity;
    oldTrafoWStoTS = zMAT4::s_identity;
    oldTrafoWStoCS = zMAT4::s_identity;
    oldTrafoRotOffsetWStoCS = zMAT4::s_identity;
    lastValidCamTrafo = zMAT4::s_identity;
    initialPoseRotFreeze = zMAT4::s_identity;
    actRot.q[0] = 0.0f;
    actRot.q[1] = 0.0f;
    actRot.q[2] = 0.0f;
    actRot.q[3] = 1.0f;
    ResetWayPoints(zVEC3(0.0f, 0.0f, 0.0f), zWAYPOINT_PLAYER);
    ResetWayPoints(zVEC3(0.0f, 0.0f, 0.0f), zWAYPOINT_CAM);
    target = NULL;
    camVob = NULL;
    camai = NULL;
    pathSearch = zCPathSearch::GetSearch();
}

zCMovementTracker::~zCMovementTracker()
{
    while (targetKFList.numInArray)
    {
        zCPositionKey* key = targetKFList[0];
        targetKFList.RemoveOrderIndex(0);
        delete key;
    }
}

float zCMovementTracker::GetAzimuth()
{
    return GetAzimuth(camPos);
}

float zCMovementTracker::GetElevation()
{
    return GetElevation(camPos);
}

float zCMovementTracker::GetAzimuth(const zVEC3& point)
{
    zVEC3 delta = playerPos - point;
    zVEC3 right = trafoTStoWS.GetRightVector();
    zVEC3 at = trafoTStoWS.GetAtVector();
    right.NormalizeSafe();
    at.NormalizeSafe();
    return atan2(delta * right, delta * at) * 57.2957795f;
}

float zCMovementTracker::GetElevation(const zVEC3& point)
{
    zVEC3 delta = playerPos - point;
    zVEC3 right = trafoTStoWS.GetRightVector();
    zVEC3 at = trafoTStoWS.GetAtVector();
    right.NormalizeSafe();
    at.NormalizeSafe();
    float horizontal = sqrt((delta * right) * (delta * right) +
        (delta * at) * (delta * at));
    return atan2(delta.n[1], horizontal) * 57.2957795f;
}

float zCMovementTracker::GetRange(const zVEC3& point)
{
    return sqrt((playerPos - point).Length2());
}

float zCMovementTracker::GetRange()
{
    return sqrt((camPos - playerPos).Length2());
}

void zCMovementTracker::SetAzimuth(const float& value)
{
    camPos = ApplyAziToPoint(value, camPos);
}

void zCMovementTracker::SetElevation(const float& value)
{
    camPos = ApplyElevToPoint(value, camPos);
}

void zCMovementTracker::SetRange(const float& value)
{
    zVEC3 direction = camPos - playerPos;
    float length = sqrt(direction.Length2());
    if (length == 0.0f)
        length = 0.000001f;
    camPos = playerPos + direction * (value / length);
}

zVEC3 zCMovementTracker::ApplyAziToPoint(const float& value, const zVEC3& point)
{
    zVEC3 offset = point - playerPos;
    float horizontal = sqrt(offset.n[0] * offset.n[0] + offset.n[2] * offset.n[2]);
    float angle = (180.0f - value) * 0.0174532924f;
    zVEC3 right = trafoTStoWS.GetRightVector();
    zVEC3 at = trafoTStoWS.GetAtVector();
    right.NormalizeSafe();
    at.NormalizeSafe();
    return playerPos + right * (sin(angle) * horizontal) +
        at * (cos(angle) * horizontal) + zVEC3(0.0f, offset.n[1], 0.0f);
}

zVEC3 zCMovementTracker::ApplyElevToPoint(const float& value, const zVEC3& point)
{
    zVEC3 offset = point - playerPos;
    float range = sqrt(offset.Length2());
    zVEC3 horizontalPoint = ApplyAziToPoint(GetAzimuth(point), point);
    zVEC3 direction = horizontalPoint - playerPos;
    direction.n[1] = 0.0f;
    if (direction.Length2() < 0.000001f)
        direction = trafoTStoWS.GetAtVector();
    direction.NormalizeSafe();
    float elev = (value + 360.0f) * 0.0174532924f;
    return playerPos + direction * (cos(elev) * range) +
        zVEC3(0.0f, sin(elev) * range, 0.0f);
}

zVEC3 zCMovementTracker::ApplyRangeToPoint(const float& value, const zVEC3& point)
{
    zVEC3 direction = point - playerPos;
    float length = sqrt(direction.Length2());
    if (length == 0.0f)
        length = 0.000001f;
    return playerPos + direction * (value / length);
}

zVEC3 zCMovementTracker::GetRangePos(const float& value)
{
    return ApplyRangeToPoint(value, camPos);
}

zVEC3 zCMovementTracker::GetIdealRangePos()
{
    return ApplyRangeToPoint(camai->GetBestRange() * 100.0f, camPos);
}

float zCMovementTracker::GetDistToIdealRange()
{
    return sqrt((GetIdealRangePos() - camPos).Length2());
}

int zCMovementTracker::IsIdealOrientation()
{
    if (fabs(GetAzimuth(camPos) - camai->bestRotY) >= 1.5f)
        return 0;
    return fabs(GetElevation(camPos) - camai->bestRotX) < 1.5f;
}

int zCMovementTracker::IsIdealRange()
{
    return GetDistToIdealRange() < 0.1f;
}

void zCMovementTracker::UpdateCamPos(const zVEC3& point)
{
    float distance = (point - camPosRing).Length2() * 0.00001f;
    float weight = (distance * 4.0f + 1.0f) / (distance + 1.0f) * inertiaTrans /
        (numSamplesCam * 0.025f);
    camPosRing = (camPosRing + point * weight) * (1.0f / (weight + 1.0f));
    camPos = point;
}

void zCMovementTracker::UpdatePlayerPos(const zVEC3& point)
{
    float weight = frameTime / (numSamplesPlayer * 0.025f);
    float distance = (point - playerPosRing).Length2() * 0.0001f;
    weight *= (distance * 3.0f + 1.0f) / (distance + 1.0f);
    if (distance > 0.0f)
    {
        zVEC3 view = point - camPos;
        view.NormalizeSafe();
        weight -= fabs((point - playerPosRing) * view) * 0.1f;
    }
    playerPosRing = (playerPosRing + point * weight) * (1.0f / (weight + 1.0f));
    playerPos = point;
}

zVEC3& zVEC3::operator+=(const zVEC3& value)
{
    n[0] += value.n[0];
    n[1] += value.n[1];
    n[2] += value.n[2];
    return *this;
}

zVEC3& zVEC3::operator/=(float value)
{
    float inv = 1.0f / value;
    n[0] *= inv;
    n[1] *= inv;
    n[2] *= inv;
    return *this;
}

void zCMovementTracker::SetCamPos(const zVEC3& point)
{
    camPos = point;
    camPosRing = point;
}

void zCMovementTracker::CalcPlayerPose()
{
    if (!target)
        return;
    if (!target->trafoObjToWorld.IsUpper3x3Orthonormal())
        target->trafoObjToWorld.MakeOrthonormal();
    zVEC3 base = target->trafoObjToWorld.GetTranslation();
    zVEC3 right = target->trafoObjToWorld.GetRightVector();
    zVEC3 at = target->trafoObjToWorld.GetAtVector();
    right.NormalizeSafe();
    at.NormalizeSafe();
    float hipOffset = 20.0f;
    zCModel* model = dynamic_cast<zCModel*>(target->visual);
    if (model && target->bbox3D.maxs.n[1] - target->bbox3D.mins.n[1] < 40.0f)
        hipOffset = 20.0f;
    UpdateSampledPlayerPos(base);
    zVEC3 center = sampledPlayerPosRing + right * camai->focusOffsetX +
        zVEC3(0.0f, camai->focusOffsetY, 0.0f) + at * camai->focusOffsetZ;
    UpdatePlayerPos(center);
    playerPos = playerPosRing;
    pathSearch->AdjustCenterSphere(playerPos, base, CAMSPHERE_DIAMETER * 0.5f);
    float headHeight = target->bbox3D.maxs.n[1];
    if (model && model->SearchNode(zMDL_NODE_NAME_HEAD))
        headHeight = target->GetTrafoModelNodeToWorld(zMDL_NODE_NAME_HEAD).GetTranslation().n[1];
    playerHead = playerPos + zVEC3(0.0f, headHeight - playerPos.n[1] + 10.0f, 0.0f);
    playerHipLeft = playerPos - right * hipOffset;
    playerHipRight = playerPos + right * hipOffset;
    pathSearch->AdjustCenterSphere(playerHead, playerPos, CAMSPHERE_DIAMETER * 0.5f);
    pathSearch->AdjustCenterSphere(playerHipLeft, playerPos, CAMSPHERE_DIAMETER * 0.5f);
    pathSearch->AdjustCenterSphere(playerHipRight, playerPos, CAMSPHERE_DIAMETER * 0.5f);
    POI_Player = playerPos;
}

zVEC3 operator/(const zVEC3& vector, float value)
{
    return vector * (1.0f / value);
}

zMAT4 zInverseMatD(const zMAT4& matrix, const zSTRING&)
{
    float values[4][8];
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            values[row][col] = matrix.m[row][col];
            values[row][col + 4] = row == col ? 1.0f : 0.0f;
        }
    }

    for (int col = 0; col < 4; ++col)
    {
        int pivotRow = col;
        for (int row = col + 1; row < 4; ++row)
        {
            if (fabs(values[row][col]) > fabs(values[pivotRow][col]))
                pivotRow = row;
        }
        if (fabs(values[pivotRow][col]) < 0.0000001f)
            return zMAT4::s_identity;
        if (pivotRow != col)
        {
            for (int i = 0; i < 8; ++i)
            {
                float temp = values[col][i];
                values[col][i] = values[pivotRow][i];
                values[pivotRow][i] = temp;
            }
        }
        float invPivot = 1.0f / values[col][col];
        for (int i = 0; i < 8; ++i)
            values[col][i] *= invPivot;
        for (int row = 0; row < 4; ++row)
        {
            if (row == col)
                continue;
            float factor = values[row][col];
            for (int i = 0; i < 8; ++i)
                values[row][i] -= values[col][i] * factor;
        }
    }

    zMAT4 result;
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            result.m[row][col] = values[row][col + 4];
    return result;
}

void zCMovementTracker::CalcPlayerMovements()
{
    zVEC3 oldCam = trafoWStoCS * oldPlayerPos;
    zVEC3 newCam = trafoWStoCS * playerPos;
    playerPosMovement = newCam - oldCam;
    float weight = frameTime / 0.25f;
    playerPosMovementRing = (playerPosMovementRing + playerPosMovement * weight) /
        (weight + 1.0f);
    playerPosMovementLocal = oldTrafoWStoTS * playerPos - oldTrafoWStoTS * oldPlayerPos;
    playerPosMovementLocalCam = oldTrafoWStoCS * playerPos - oldTrafoWStoCS * oldPlayerPos;
}

void zCMovementTracker::SetPlayerPos(const zVEC3& point)
{
    playerPosRing = point;
    playerPos = point;
    sampledPlayerPos = point;
    sampledPlayerPosRing = point;
}

void zCMovementTracker::ReceiveMsg(int message)
{
    movement |= message;
}

void zCMovementTracker::ClearMessages()
{
    movement = 0x5000;
}

void zCMovementTracker::AutoSetMessages()
{
    if (playerPosMovementLocalCam.n[0] > 1.5f)
        movement |= 0x10;
    else if (playerPosMovementLocalCam.n[0] < -1.5f)
        movement |= 8;
    if (playerPosMovementLocalCam.n[1] > 0.5f)
        movement |= 0x20;
    else if (playerPosMovementLocalCam.n[1] < -0.5f)
        movement |= 0x40;
    if (playerPosMovementLocalCam.n[2] > 3.5f)
        movement |= 2;
    else if (playerPosMovementLocalCam.n[2] < -3.5f)
        movement |= 4;

    float rotation = atan2(oldTrafoWStoTS.m[0][2], oldTrafoWStoTS.m[2][2]) -
        atan2(trafoWStoTS.m[0][2], trafoWStoTS.m[2][2]);
    if (rotation > 0.02f)
        movement |= 0x100;
    else if (rotation < -0.02f)
        movement |= 0x80;
}

void zCMovementTracker::UpdateMessages()
{
    if (movement & 0x7e)
        movement = (movement & ~0x1000) | 0x800;
    if (movement & 0x8100)
        movement = (movement & ~0x4000) | 0x2000;
}

void zCMovementTracker::Update()
{
    if (!camVob || !target)
        return;
    frameTime = ztimer.frameTimeFloatSecs / ztimer.factorMotion;
    if (frameTime == 0.0f)
        return;

    UpdateCamPos(camVob->trafoObjToWorld.GetTranslation());
    UpdateWayPoint(zWAYPOINT_CAM);
    trafoCStoWS = camVob->trafoObjToWorld;
    trafoWStoCS = zInverseMatD(trafoCStoWS, zSTRING("Update"));
    CalcPlayerPose();
    CalcPlayerMovements();
    playerVeloRange = fabs(distToPlayer - oldDistToPlayer) / frameTime * 1000.0f;
    actAzi = GetAzimuth(camPos);
    actElev = GetElevation(camPos);
    distToPlayer = GetRange();
    if ((target->trafoObjToWorld.GetTranslation() - oldSampledPlayerPos).Length2() >
        5000.0f * 5000.0f)
        movement |= 0x8000;
    if (!(movement & 0x1000))
        standTimer = 0.0f;
    else
        standing = 1;

    float farClip = zCCamera::activeCam ? zCCamera::activeCam->farClipZ : 5000.0f;
    zVEC3 poiEnd = playerHead + trafoTStoWS.GetAtVector() * farClip;
    ++camai->raysCasted;
    if (camVob->homeWorld->TraceRayNearestHit(playerHead, poiEnd - playerHead, target, 0x1a))
    {
        zVEC3 intersection = camVob->homeWorld->traceRayReport.foundIntersection;
        POI_Player = intersection + (playerHead - intersection).NormalizeSafe();
    }
    else
        POI_Player = poiEnd;
    CalcPlayerSpline();
    CheckSpecialMoves();
    AutoSetMessages();
    UpdateMessages();
}

void zCMovementTracker::CheckKeys()
{
}

int zCMovementTracker::CheckSpecialMoves()
{
    if (!camVob || !(camVob->sleepingMode & 3) || zCCSCamera::playing ||
        !zCCamera::activeCam || !zCCamera::activeCam->connectedVob || !(movement & 0x8000))
        return 0;

    SetPlayerPos(target->trafoObjToWorld.GetTranslation());
    ResetWayPoints(playerPos, zWAYPOINT_PLAYER);
    CalcPlayerPose();
    CalcPlayerMovements();
    camai->CalcFirstPosition();
    int reason = 0;
    if (pathSearch->IsPointValid(camai->GetCamSysFirstPos(), 4, reason))
    {
        zSPathSearchResult report;
        report.foundEvasion = 1;
        report.p1 = camVob->trafoObjToWorld.GetTranslation();
        report.p2 = camai->GetCamSysFirstPos();
        report.bestLerpMode = 2;
        report.veloTrans = camai->veloTrans;
        report.veloRot = camai->veloRot;
        report.r1.Matrix4ToQuat(camVob->trafoObjToWorld);
        report.r2 = report.r1;
        InterpolateTo(&report);
    }
    else
    {
        zSPathSearchOptions options;
        options.searchFlags = 0x16;
        options.poseFailReason = 0x40000;
        options.minLerpMode = 16;
        options.minScore = 0.0f;
        options.bestScore = 1.0f;
        options.startAzi = -180.0f;
        options.startElev = -20.0f;
        options.startRange = 1.0f;
        options.endAzi = 180.0f;
        options.endElev = 20.0f;
        options.endRange = 4.0f;
        options.quality = 1.0f;
        pathSearch->FindBestEvasion(options);
        camai->evasionSearchReport->bestLerpMode =
            (camai->evasionSearchReport->bestLerpMode != 4) + 1;
        InterpolateTo(camai->evasionSearchReport);
    }
    camai->underWater = 0;
    camai->CheckUnderWaterFX();
    return 1;
}

zCQuat zCMovementTracker::GetLocalRot(const float& azi, const float& elev)
{
    zVEC3 point = playerPos - trafoTStoWS.GetAtVector();
    point = ApplyAziToPoint(azi, point);
    point = ApplyElevToPoint(elev, point);
    zVEC3 heading = playerPos - point;
    heading.NormalizeSafe();
    zMAT4 pose = GetPoseOfHeading(point, heading);
    zMAT4 local = trafoWStoTS * pose;
    zCQuat result;
    result.Matrix4ToQuat(local);
    return result;
}

zMAT4 zCMovementTracker::GetLocalRotEnd()
{
    if (movement & 0x8000)
        return camVob->trafoObjToWorld;
    if (camPos == playerPos)
        return trafoTStoWS;
    float offsetX = camai->rotOffsetX;
    float offsetY = camai->rotOffsetY;
    if (!bMouseUsed)
    {
        if (camai->bestRotX != 0.0f)
        {
            offsetX = GetElevation() / camai->bestRotX * offsetX;
            if (offsetX > fabs(camai->rotOffsetX)) offsetX = fabs(camai->rotOffsetX);
            if (offsetX < -fabs(camai->rotOffsetX)) offsetX = -fabs(camai->rotOffsetX);
        }
        if (camai->bestRotY != 0.0f)
        {
            offsetY = GetAzimuth() / camai->bestRotY * offsetY;
            if (offsetY > fabs(camai->rotOffsetY)) offsetY = fabs(camai->rotOffsetY);
            if (offsetY < -fabs(camai->rotOffsetY)) offsetY = -fabs(camai->rotOffsetY);
        }
    }
    zCQuat rotation = GetLocalRot(actAzi + offsetY, actElev + offsetX);
    zMAT4 result;
    rotation.QuatToMatrix4(result);
    return trafoWStoTS * result;
}

void zCMovementTracker::RecalcRingPositions()
{
    numSamplesCam = int(inertiaTrans * 99.0f + 1.0f);
    playerPosRing = playerPos;
    sampledPlayerPosRing = playerPos;
}

zVEC3& zCMovementTracker::GetLastValidWayPoint(const zTWayPoint& wayPoint)
{
    static zVEC3 empty(0.0f, 0.0f, 0.0f);
    zVEC3* points;
    int act;
    float time;
    switch (wayPoint)
    {
    case zWAYPOINT_PLAYER:
    case zWAYPOINT_HEAD:
        points = wayPointHead;
        act = actWayPointHead;
        time = wayTimeHead;
        break;
    case zWAYPOINT_CAM:
        points = wayPointCam;
        act = actWayPointCam;
        time = wayTimeCam;
        break;
    case zWAYPOINT_HIP_LEFT:
        points = wayPointHipLeft;
        act = actWayPointHipLeft;
        time = wayTimeHipLeft;
        break;
    case zWAYPOINT_HIP_RIGHT:
        points = wayPointHipRight;
        act = actWayPointHipRight;
        time = wayTimeHipRight;
        break;
    default:
        return empty;
    }
    int index = time < 0.15f ? (act + 2) % 3 : (act + 1) % 3;
    return points[index];
}

void zCMovementTracker::UpdateWayPoint(const zTWayPoint& wayPoint)
{
    zVEC3 point;
    zVEC3* points;
    int* act;
    float* time;
    int valid;
    if (wayPoint == zWAYPOINT_CAM)
    {
        point = camPos;
        points = wayPointCam;
        act = &actWayPointCam;
        time = &wayTimeCam;
        int reason = 0;
        valid = pathSearch->IsPointValid(point, 0x14, reason);
    }
    else
    {
        if (wayPoint == zWAYPOINT_HIP_LEFT)
        {
            point = playerHipLeft;
            points = wayPointHipLeft;
            act = &actWayPointHipLeft;
            time = &wayTimeHipLeft;
        }
        else if (wayPoint == zWAYPOINT_HIP_RIGHT)
        {
            point = playerHipRight;
            points = wayPointHipRight;
            act = &actWayPointHipRight;
            time = &wayTimeHipRight;
        }
        else
        {
            point = playerHead;
            points = wayPointHead;
            act = &actWayPointHead;
            time = &wayTimeHead;
        }
        ++camai->raysCasted;
        valid = !camVob->homeWorld->TraceRayFirstHit(camPos, point - camPos, target, 0);
    }

    if (!valid)
        return;
    *time += frameTime;
    if (*time < 0.3f)
        return;
    points[*act] = point;
    *act = (*act + 1) % 3;
    *time = 0.0f;
}

void zCMovementTracker::ResetWayPoints(const zVEC3& point, const zTWayPoint& wayPoint)
{
    if (wayPoint == zWAYPOINT_PLAYER)
    {
        wayTimeHead = 0.0f;
        wayTimeHipLeft = 0.0f;
        wayTimeHipRight = 0.0f;
        actWayPointHead = 0;
        actWayPointHipLeft = 0;
        actWayPointHipRight = 0;
        for (int i = 0; i < 3; ++i)
        {
            wayPointHead[i] = point;
            wayPointHipLeft[i] = point;
            wayPointHipRight[i] = point;
        }
    }
    else if (wayPoint == zWAYPOINT_CAM)
    {
        wayTimeCam = 0.0f;
        actWayPointCam = 0;
        for (int i = 0; i < 3; ++i)
            wayPointCam[i] = point;
    }
}

void zCMovementTracker::InterpolateTo(const zSPathSearchResult& result)
{
    if (!result.foundEvasion)
        return;
    oldCamPos = camPos;
    float time = result.bestLerpMode == 2 || result.bestLerpMode == 4 ? 1.0f :
        frameTime * result.veloTrans;
    if (time < 0.0f) time = 0.0f;
    if (time > 1.0f) time = 1.0f;
    zVEC3 destination = result.p2;
    if (bMouseUsed && (destination - camPos).LengthApprox() > 250.0f)
        destination = camPos + (destination - camPos).NormalizeSafe() * 250.0f;
    zCQuat destinationRot = result.r2;
    if (result.resPath.numInArray)
    {
        float pathTime = time * (result.resPath.numInArray - 1);
        int index = (int)pathTime;
        if (index >= result.resPath.numInArray - 1)
            index = result.resPath.numInArray - 1;
        destination = result.resPath[index]->p;
        destinationRot = result.resPath[index]->rot;
    }
    camPos += (destination - camPos) * time;
    actRot.Slerp(time * result.veloRot, destinationRot);
    zMAT4 trafo;
    actRot.QuatToMatrix4(trafo);
    trafo.SetTranslation(camPos);
    camVob->SetTrafoObjToWorld(trafo);
    if (time >= 1.0f)
    {
        camPosRing = camPos;
        movement &= ~0x8000;
    }
    actAzi = GetAzimuth(camPos);
    actElev = GetElevation(camPos);
}

void zMAT4::SetUpVector(const zVEC3& vector)
{
    m[0][1] = vector.n[0];
    m[1][1] = vector.n[1];
    m[2][1] = vector.n[2];
}

void zMAT4::SetRightVector(const zVEC3& vector)
{
    m[0][0] = vector.n[0];
    m[1][0] = vector.n[1];
    m[2][0] = vector.n[2];
}

zVEC3 operator-(const zVEC3& vector)
{
    return zVEC3(-vector.n[0], -vector.n[1], -vector.n[2]);
}

void zCMovementTracker::CalcPlayerSpline()
{
    static int counter;
    if (counter++ & 1)
        return;
    if (targetKFList.numInArray > 50)
    {
        zCPositionKey* oldest = targetKFList[0];
        targetKFList.RemoveOrderIndex(0);
        delete oldest;
        for (int i = 0; i < targetKFList.numInArray; ++i)
            targetKFList[i]->t = float(i);
    }

    zCPositionKey* key = new zCPositionKey;
    key->t = float(targetKFList.numInArray);
    zVEC3 ideal = camai->CalcAziElevRange(0.0f, 0.0f,
        camai->npcVolumeRangeOffset + 150.0f, trafoTStoWS);
    key->p = ideal + trafoTStoWS.GetAtVector() * 200.0f + zVEC3(0.0f, 10.0f, 0.0f);
    zMAT4 pose = GetPoseOfHeading(key->p, (key->p - ideal).NormalizeSafe());
    key->rot.Matrix4ToQuat(pose);
    targetKFList.InsertEnd(key);
}

static zMAT4 MakeTrackerHeading(const zVEC3& position, const zVEC3& direction)
{
    zVEC3 at = direction;
    at.NormalizeSafe();
    zVEC3 right = zVEC3(0.0f, 1.0f, 0.0f) ^ at;
    if (right.Length2() < 0.000001f)
        right = zVEC3(1.0f, 0.0f, 0.0f);
    right.NormalizeSafe();
    zVEC3 up = at ^ right;
    up.NormalizeSafe();
    zMAT4 result = zMAT4::s_identity;
    result.SetRightVector(right);
    result.SetUpVector(up);
    result.m[0][2] = at.n[0];
    result.m[1][2] = at.n[1];
    result.m[2][2] = at.n[2];
    result.SetTranslation(position);
    return result;
}

zMAT4& zCMovementTracker::GetFirstPersonCamMat()
{
    static zMAT4 result;
    zCModel* model = dynamic_cast<zCModel*>(target->visual);
    if (model && model->SearchNode("ZS_FPCAM"))
    {
        result = target->GetTrafoModelNodeToWorld("ZS_FPCAM");
        result.PostRotateZ(-90.0f);
        zVEC3 position = result.GetTranslation();
        pathSearch->AdjustCenterSphere(position, playerPos, CAMSPHERE_DIAMETER * 0.5f);
        result.SetTranslation(position);
        return result;
    }
    zVEC3 position = playerHead;
    pathSearch->AdjustCenterSphere(position, playerPos, CAMSPHERE_DIAMETER * 0.5f);
    result = MakeTrackerHeading(position, trafoTStoWS.GetAtVector());
    return result;
}

zMAT4& zCMovementTracker::GetPanicCamMat()
{
    static zMAT4 result;
    zVEC3 direction = trafoTStoWS.GetAtVector() + GetLocalRotEnd().GetTranslation();
    direction.NormalizeSafe();
    result = MakeTrackerHeading(camPosRing, direction);
    return result;
}

zMAT4& zCMovementTracker::GetShoulderCamMat()
{
    static zMAT4 result;
    zVEC3 position = camai->CalcAziElevRange(0.0f, 0.0f,
        camai->npcVolumeRangeOffset + 150.0f, trafoTStoWS);
    position += trafoTStoWS.GetAtVector() * 200.0f + zVEC3(0.0f, 10.0f, 0.0f);
    zVEC3 direction = position - trafoTStoWS.GetAtVector();
    direction.NormalizeSafe();
    result = MakeTrackerHeading(position, direction);
    return result;
}

zVEC3& zCMovementTracker::CalcAziElevRangeApprox(const float& azi, const float& elev,
    const float& range)
{
    static zVEC3 result;
    float aziRad = (180.0f - azi) * 0.0174532924f;
    float elevRad = (elev + 360.0f) * 0.0174532924f;
    zVEC3 right = trafoTStoWS.GetRightVector();
    zVEC3 up = trafoTStoWS.GetUpVector();
    zVEC3 at = trafoTStoWS.GetAtVector();
    right.NormalizeSafe();
    up.NormalizeSafe();
    at.NormalizeSafe();
    float horizontal = cos(elevRad) * range;
    result = playerPos + right * (cos(aziRad) * horizontal) +
        at * (sin(aziRad) * horizontal) + up * (sin(elevRad) * range);
    return result;
}

void zCMovementTracker::SetRotateEnabled(int enabled)
{
    if (rotate == enabled)
        return;
    initialPoseRotFreeze = trafoTStoWS;
    rotate = enabled;
}

void zCMovementTracker::SaveFrameStates()
{
    oldPlayerPosMovement = playerPosMovement;
    oldSampledPlayerPosRing = sampledPlayerPosRing;
    oldPlayerPos = playerPos;
    oldSampledPlayerPos = sampledPlayerPos;
    oldTrafoWStoTS = trafoWStoTS;
    oldCamPos = camPos;
    oldDistToPlayer = GetRange();
    oldElev = GetElevation(camPos);
    oldAzi = GetAzimuth(camPos);
    camPos = camVob->trafoObjToWorld.GetTranslation();
    zVEC3 direction = playerPos - camPos;
    if (direction.Length2() == 0.0f)
        oldTrafoRotOffsetWStoCS = zMAT4::s_identity;
    else
        oldTrafoRotOffsetWStoCS = zInverseMatD(MakeTrackerHeading(camPos, direction),
            zSTRING("SaveFrameStates"));
    oldTrafoWStoCS = zInverseMatD(camVob->trafoObjToWorld, zSTRING("SaveFrameStates"));

    int valid = 1;
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            float value = camVob->trafoObjToWorld.m[row][col];
            if (value != value || value > 3.402823466e38f || value < -3.402823466e38f)
                valid = 0;
        }
    }
    if (!valid)
        camVob->SetTrafoObjToWorld(lastValidCamTrafo);
    else
        lastValidCamTrafo = camVob->trafoObjToWorld;
    movement = 0x5000;
}

zVEC4 zMAT4::operator[](int index) const
{
    return zVEC4(m[index][0], m[index][1], m[index][2], m[index][3]);
}

void zCMovementTracker::TargetChanged()
{
    target = camai->target;
    playerPosMovement = zVEC3(0.0f, 0.0f, 0.0f);
    playerPosMovementRing = zVEC3(0.0f, 0.0f, 0.0f);
    playerPosMovementLocal = zVEC3(0.0f, 0.0f, 0.0f);
    playerPosMovementLocalCam = zVEC3(0.0f, 0.0f, 0.0f);
    if (target)
    {
        zVEC3 position = target->trafoObjToWorld.GetTranslation();
        SetPlayerPos(position);
        ResetWayPoints(position, zWAYPOINT_PLAYER);
        Update();
    }
}

void zCMovementTracker::CamVobChanged()
{
    camVob = camai->camVob;
    if (!camVob)
        return;
    SetCamPos(camVob->trafoObjToWorld.GetTranslation());
    if (movement & 0x8000)
        ResetWayPoints(camPos, zWAYPOINT_CAM);
    Update();
}

void zCMovementTracker::UpdateSampledPlayerPos(const zVEC3& point)
{
    float distance = (point - sampledPlayerPosRing).Length2() * 0.00001f;
    float weight = (distance * 4.0f + 1.0f) / (distance + 1.0f) * frameTime /
        (numPreSamplesPlayerPos * 0.025f);
    if ((movement & 0x8000) || camai->curcammode == "FIRSTPERSON")
        sampledPlayerPosRing = point;
    else
        sampledPlayerPosRing = (sampledPlayerPosRing + point * weight) / (weight + 1.0f);
    sampledPlayerPos = point;
}

void zCMovementTracker::SetSampledPlayerPos(const zVEC3& point)
{
    sampledPlayerPos = point;
}

zVEC3& zCMovementTracker::GetPlayerFuturePos(float seconds)
{
    static zVEC3 result;
    result = sampledPlayerPos + playerPosMovementRing * (seconds / frameTime);
    if (!pathSearch->LineOfSight(result, playerPos))
        return camVob->homeWorld->traceRayReport.foundIntersection;
    return result;
}
