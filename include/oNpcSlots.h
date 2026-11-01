#ifndef ONPCSLOTS_H
#define ONPCSLOTS_H

#include "oCollisionObjectMiscChar.h"

int BitToNumber(int value);
zVEC3 RotateVec(zVEC3& vector, zVEC3& axis, float angle, float scale);

struct zTOverlayIndex {
    ~zTOverlayIndex() {}

    zSTRING overlay;
};

struct TNpcSlot {
    TNpcSlot();
    ~TNpcSlot();

    void SetVob(oCVob* value);
    void ClearVob();

    zSTRING name;
    int     inInventory;
    int     tmpLevel;
    zSTRING itemName;
    oCVob*  vob;
    int     wasVobTreeWhenInserted : 1;
};

#endif
