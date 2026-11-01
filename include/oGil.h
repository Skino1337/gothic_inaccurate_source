#ifndef OGIL_H
#define OGIL_H

#include "zString.h"

struct oTGilValues {
    oTGilValues();
    ~oTGilValues();

    float   waterDepthKnee[66];
    float   waterDepthChest[66];
    float   jumpUpHeight[66];
    float   swimTime[66];
    float   diveTime[66];
    float   stepHeight[66];
    float   jumpLowHeight[66];
    float   jumpMidHeight[66];
    float   maxGroundAngleWalk[66];
    float   maxGroundAngleSlide2[66];
    int     disableAutoRoll[66];
    int     surfaceAlign[66];
    float   wallHeadingAngle[66];
    float   wallHorizAngle[66];
    float   groundAngle[66];
    int     fightRangeBase[66];
    int     fightRangeFist[66];
    int     fightRangeG[66];
    int     fightRange2H[66];
    int     fightRangeBow[66];
    float   fallDownHeight[66];
    int     fallDownDamage[66];
    int     bloodDisable[66];
    int     bloodMaxDistance[66];
    int     bloodAmount[66];
    int     bloodFlow[66];
    zSTRING bloodEmitter[66];
    zSTRING bloodTexture[66];
    float   turnSpeed[66];
};

extern oTGilValues TGilValues;

#endif
