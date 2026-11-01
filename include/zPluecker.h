#ifndef ZPLUECKER_H
#define ZPLUECKER_H

#include "zModel.h"

class zCPluecker {
public:

    zCPluecker();
    zCPluecker(const zVEC3& position, const zVEC3& direction);

    float        Length2() const;
    void         Normalize();
    zCPluecker&  operator-=(const zCPluecker& other);

    float dir[3];
    float pos[3];
};

float operator*(const zCPluecker& first, const zCPluecker& second);

#endif
