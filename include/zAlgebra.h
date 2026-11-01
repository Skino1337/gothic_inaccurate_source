#ifndef ZALGEBRA_H
#define ZALGEBRA_H

#include "z3d.h"
#include <iosfwd>

zVEC2 Alg_Min(const zVEC2& first, const zVEC2& second);
zVEC2 Alg_Max(const zVEC2& first, const zVEC2& second);
zVEC2 Alg_Prod(const zVEC2& first, const zVEC2& second);
void  Alg_Swap(zVEC2& first, zVEC2& second);
zVEC3 Alg_Min(const zVEC3& first, const zVEC3& second);
zVEC3 Alg_Max(const zVEC3& first, const zVEC3& second);
zVEC3 Alg_Prod(const zVEC3& first, const zVEC3& second);
void  Alg_Swap(zVEC3& first, zVEC3& second);
zVEC4 Alg_Min(const zVEC4& first, const zVEC4& second);
zVEC4 Alg_Max(const zVEC4& first, const zVEC4& second);
zVEC4 Alg_Prod(const zVEC4& first, const zVEC4& second);
void  Alg_Swap(zVEC4& first, zVEC4& second);

std::ostream& operator<<(std::ostream& output, zVEC2& value);
std::istream& operator>>(std::istream& input, zVEC2& value);
std::ostream& operator<<(std::ostream& output, zVEC3& value);
std::istream& operator>>(std::istream& input, zVEC3& value);
std::ostream& operator<<(std::ostream& output, zVEC4& value);
std::istream& operator>>(std::istream& input, zVEC4& value);

zMAT3 Alg_Identity2D();
zMAT3 Alg_Translation2D(zVEC2& translation);
zMAT3 Alg_Rotation2D(zVEC2& center, float angle);
zMAT3 Alg_Scaling2D(zVEC2& scaling);
zMAT4 Alg_Identity3D();
zMAT4 Alg_Translation3D(const zVEC3& translation);
zMAT4 Alg_Rotation3D(const zVEC3& axis, float angle);
zMAT4 Alg_Rotation3DN(const zVEC3& axis, float angle);
zMAT4 Alg_Rotation3DRad(const zVEC3& axis, float angle);
zMAT4 Alg_Rotation3DNRad(const zVEC3& axis, float angle);
zMAT4 Alg_Scaling3D(const zVEC3& scaling);
zMAT4 Alg_Perspective3D(float distance);
void  Alg_ClipAtZ0(zVEC3& first, zVEC3& second);

struct zCValueSmooth {
    zCValueSmooth();
    void Smooth(float& value);

    float fixedValue;
    float smoothedValue;
    float inertia;
    int   bSmooth;
};

struct zCVEC3Smooth {
    zCVEC3Smooth();
    void Smooth(zVEC3& value);

    zVEC3 fixedValue;
    zVEC3 smoothedValue;
    float inertia;
    int   bSmooth;
};

#endif
