#ifndef OWAYNET_H
#define OWAYNET_H

#include "oMob.h"

class oCWaypoint : public zCWaypoint {
public:

    oCWaypoint();
    virtual ~oCWaypoint();
    virtual int CanBeUsed(const zCVob* vob);
};

class oCWay : public zCWay {
public:

    oCWay();
    virtual ~oCWay();
    virtual void Init(zCWaypoint* left, zCWaypoint* right);
    virtual int CanBeUsed(const zCVob* vob);
    virtual int IsObjectOnWay(const zCVob* vob);
    oCMobLadder* GetLadder();
    oCMobDoor*   GetDoor();

    oCMobLadder* ladder;
    oCMobDoor*   door;
};

#endif
