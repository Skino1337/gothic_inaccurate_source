#include "zPluecker.h"

#include <cmath>

zCPluecker::zCPluecker()
{
    for (int i = 0; i < 3; ++i) {
        dir[i] = 0;
        pos[i] = 0;
    }
}

zCPluecker::zCPluecker(const zVEC3& position, const zVEC3& direction)
{
    for (int i = 0; i < 3; ++i)
        dir[i] = direction.n[i];
    pos[0] = direction.n[2] * position.n[1] - position.n[2] * direction.n[1];
    pos[1] = position.n[2] * direction.n[0] - direction.n[2] * position.n[0];
    pos[2] = position.n[0] * direction.n[1] - direction.n[0] * position.n[1];
}

float zCPluecker::Length2() const
{
    float result = 0;
    for (int i = 0; i < 3; ++i)
        result += pos[i] * pos[i] + dir[i] * dir[i];
    return result;
}

void zCPluecker::Normalize()
{
    const float length2 = Length2();
    if (length2 == 0) {
        dir[0] = 1.0f;
        return;
    }
    const float scale = 1.0f / std::sqrt(length2);
    for (int i = 0; i < 3; ++i) {
        dir[i] *= scale;
        pos[i] *= scale;
    }
}
