#ifndef ZAICAMERA_CORE_H
#define ZAICAMERA_CORE_H

#include "zAiCamera.h"

struct zSPathSearchState_Internal {
    float maxRange;
    zVEC3 foundPos;
    zMAT4 trafoCamToWorld;
    int bestLerpMode;
    float scoreLerpMode;
    float scoreDistToIdealPos;
    float scoreFoundNumNpcs;
    float scoreFoundNumBBoxes;
    float scoreFoundNumTargets;
    float scorePlayerVisibility;
    float scoreDistToLastEvasion;
    float totalScore;
    float startAzi;
    float startElev;
    float startRange;
    float endAzi;
    float endElev;
    float endRange;
    float bestAzi;
    float bestElev;
    float bestRange;
    zSTRING sphereSearchName;
    zSTRING sphereSearchSegment;
};

typedef zSPathSearchState_Internal zSPathSearchState;

class zCSphereCoordsCache {
public:

    struct zSRangeSphereCoord {
        float range;
        zSPathSearchState_Internal searchState;
    };

    struct zSElevSphereCoord {
        float elev;
        float maxRange;
        zCArraySort<zSRangeSphereCoord*> rangeCoords;
    };

    struct zSAziSphereCoord {
        float azi;
        zCArraySort<zSElevSphereCoord*> elevCoords;
    };

    zCSphereCoordsCache() {}
    ~zCSphereCoordsCache();
    void DeleteList();
    int GetSphereCoordLazy(const float& azi, const float& elev, const float& range,
        zSPathSearchState_Internal*& state);
    void GetBestScoredSearch(float& azi, float& elev, float& range,
        zSPathSearchState_Internal*& state);

    zCArraySort<zSAziSphereCoord*> aziCoords;
};

#endif
