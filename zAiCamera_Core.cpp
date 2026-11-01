#include "include/zAiCamera_Core.h"

int zCSphereCoordsCache::GetSphereCoordLazy(const float& azi, const float& elev,
    const float& range, zSPathSearchState_Internal*& state)
{
    for (int i = 0; i < aziCoords.numInArray; ++i)
    {
        zSAziSphereCoord* aziCoord = aziCoords[i];
        if (aziCoord->azi != azi)
            continue;
        for (int j = 0; j < aziCoord->elevCoords.numInArray; ++j)
        {
            zSElevSphereCoord* elevCoord = aziCoord->elevCoords[j];
            if (elevCoord->elev != elev)
                continue;
            for (int k = 0; k < elevCoord->rangeCoords.numInArray; ++k)
            {
                zSRangeSphereCoord* rangeCoord = elevCoord->rangeCoords[k];
                if (rangeCoord->range == range)
                {
                    state = &rangeCoord->searchState;
                    return 1;
                }
            }
        }
    }

    zSAziSphereCoord* aziCoord = new zSAziSphereCoord;
    aziCoord->azi = azi;
    zSElevSphereCoord* elevCoord = new zSElevSphereCoord;
    elevCoord->elev = elev;
    elevCoord->maxRange = range;
    zSRangeSphereCoord* rangeCoord = new zSRangeSphereCoord;
    rangeCoord->range = range;
    rangeCoord->searchState.maxRange = range;
    zCMovementTracker* tracker = zCMovementTracker::GetTracker();
    rangeCoord->searchState.foundPos = tracker->CalcAziElevRangeApprox(azi, elev, range);
    zVEC3 direction = rangeCoord->searchState.foundPos - tracker->playerPos;
    float maxRange = zCAICamera::GetCurrent()->GetMaxRange() * 100.0f +
        CAMSPHERE_DIAMETER * 0.5f;
    direction.Normalize();
    direction *= maxRange;
    zCAICamera* camera = zCAICamera::GetCurrent();
    zCWorld* world = camera->camVob->homeWorld;
    ++camera->raysCasted;
    float score = maxRange * maxRange;
    if (world->TraceRayNearestHit(tracker->playerPos, direction, camera->target, 0x16) &&
        !world->traceRayReport.foundVob)
    {
        zVEC3 offset = world->traceRayReport.foundIntersection - tracker->playerPos;
        score = offset.Length2();
        if (score < 1.0f)
            score = 1.0f;
    }
    rangeCoord->searchState.totalScore = score;
    aziCoord->elevCoords.InsertSort(elevCoord);
    elevCoord->rangeCoords.InsertSort(rangeCoord);
    aziCoords.InsertSort(aziCoord);
    state = &rangeCoord->searchState;
    return 0;
}

zCSphereCoordsCache::~zCSphereCoordsCache()
{
    DeleteList();
}

void zCSphereCoordsCache::DeleteList()
{
    for (int i = 0; i < aziCoords.numInArray; ++i)
    {
        zSAziSphereCoord* aziCoord = aziCoords[i];
        for (int j = 0; j < aziCoord->elevCoords.numInArray; ++j)
        {
            zSElevSphereCoord* elevCoord = aziCoord->elevCoords[j];
            for (int k = 0; k < elevCoord->rangeCoords.numInArray; ++k)
                delete elevCoord->rangeCoords[k];
            delete elevCoord;
        }
        delete aziCoord;
    }
    aziCoords.DeleteList();
}

void zCSphereCoordsCache::GetBestScoredSearch(float& azi, float& elev, float& range,
    zSPathSearchState_Internal*& state)
{
    float bestScore = -1.0f;
    for (int i = 0; i < aziCoords.numInArray; ++i)
    {
        zSAziSphereCoord* aziCoord = aziCoords[i];
        for (int j = 0; j < aziCoord->elevCoords.numInArray; ++j)
        {
            zSElevSphereCoord* elevCoord = aziCoord->elevCoords[j];
            for (int k = 0; k < elevCoord->rangeCoords.numInArray; ++k)
            {
                zSRangeSphereCoord* rangeCoord = elevCoord->rangeCoords[k];
                if (rangeCoord->searchState.totalScore > bestScore)
                {
                    bestScore = rangeCoord->searchState.totalScore;
                    azi = aziCoord->azi;
                    elev = elevCoord->elev;
                    range = rangeCoord->range;
                    state = &rangeCoord->searchState;
                }
            }
        }
    }
}
