#include "include/zSky.h"

#include <cmath>

zMAT3 zMAT3::s_identity(zVEC3(1.0f, 0.0f, 0.0f), zVEC3(0.0f, 1.0f, 0.0f),
    zVEC3(0.0f, 0.0f, 1.0f));

void zMAT4::SetAtVector(const zVEC3& vector)
{
    m[0][2] = vector.n[0];
    m[1][2] = vector.n[1];
    m[2][2] = vector.n[2];
}

void zCOLOR::Reset()
{
    b = 0;
    g = 0;
    r = 0;
    a = 255;
}

zSTRING zCOLOR::GetDescription() const
{
    zSTRING result;
    result.Sprintf("%d %d %d %d", r, g, b, a);
    return result;
}

void zCOLOR::SetByDescription(const zSTRING& description)
{
    r = static_cast<unsigned char>(description.PickWord_Old(1, " ").ToInt());
    g = static_cast<unsigned char>(description.PickWord_Old(2, " ").ToInt());
    b = static_cast<unsigned char>(description.PickWord_Old(3, " ").ToInt());
    a = static_cast<unsigned char>(description.PickWord_Old(4, " ").ToInt());
}

zSTRING zCOLOR::GetDescriptionRGB() const
{
    zSTRING result;
    result.Sprintf("%d %d %d", r, g, b);
    return result;
}

int zTPlane::ClassifyPointBSPBuild(const zVEC3& point) const
{
    const float side = point.n[0] * normal.n[0] + point.n[1] * normal.n[1] +
        point.n[2] * normal.n[2] - distance;
    if (side >= 0.5f)
        return 0;
    if (side < -0.5f)
        return 1;
    return 2;
}

zSTRING zTPlane::Print() const
{
    zSTRING result;
    result.Sprintf("(%f ; %f ; %f ; %f)", normal.n[0], normal.n[1], normal.n[2], distance);
    return result;
}

static void NormalizePlaneVector(zVEC3& value)
{
    const float length2 = value.n[0] * value.n[0] + value.n[1] * value.n[1] +
        value.n[2] * value.n[2];
    const float invLength = 1.0f / std::sqrt(length2);
    value.n[0] *= invLength;
    value.n[1] *= invLength;
    value.n[2] *= invLength;
}

void zTPlane::Calc(const zVEC3& first, const zVEC3& second, const zVEC3& third)
{
    const zVEC3 firstEdge = { third.n[0] - second.n[0], third.n[1] - second.n[1],
        third.n[2] - second.n[2] };
    const zVEC3 secondEdge = { first.n[0] - second.n[0], first.n[1] - second.n[1],
        first.n[2] - second.n[2] };
    normal.n[0] = firstEdge.n[1] * secondEdge.n[2] - firstEdge.n[2] * secondEdge.n[1];
    normal.n[1] = firstEdge.n[2] * secondEdge.n[0] - firstEdge.n[0] * secondEdge.n[2];
    normal.n[2] = firstEdge.n[0] * secondEdge.n[1] - firstEdge.n[1] * secondEdge.n[0];
    NormalizePlaneVector(normal);
    distance = normal.n[0] * second.n[0] + normal.n[1] * second.n[1] + normal.n[2] * second.n[2];
}

void zTPlane::GetOriginUpRight(zVEC3& origin, zVEC3& up, zVEC3& right) const
{
    int axis = 0;
    float minimum = std::fabs(normal.n[0]);
    for (int i = 1; i < 3; ++i) {
        const float value = std::fabs(normal.n[i]);
        if (value < minimum) {
            minimum = value;
            axis = i;
        }
    }
    up.n[0] = 0;
    up.n[1] = 0;
    up.n[2] = axis <= 1 ? 1.0f : 0;
    if (axis == 2)
        up.n[0] = 1.0f;
    const float projection = up.n[0] * normal.n[0] + up.n[1] * normal.n[1] +
        up.n[2] * normal.n[2];
    for (int i = 0; i < 3; ++i)
        up.n[i] -= projection * normal.n[i];
    NormalizePlaneVector(up);
    right.n[0] = up.n[1] * normal.n[2] - up.n[2] * normal.n[1];
    right.n[1] = up.n[2] * normal.n[0] - up.n[0] * normal.n[2];
    right.n[2] = up.n[0] * normal.n[1] - up.n[1] * normal.n[0];
    NormalizePlaneVector(right);
    origin.n[0] = distance * normal.n[0];
    origin.n[1] = distance * normal.n[1];
    origin.n[2] = distance * normal.n[2];
}


zVEC3::zVEC3(float x, float y, float z)
{
    n[0] = x;
    n[1] = y;
    n[2] = z;
}

zVEC3::zVEC3(const zVEC3& other)
{
    n[0] = other.n[0];
    n[1] = other.n[1];
    n[2] = other.n[2];
}

zVEC3 zMAT4::GetTranslation() const
{
    return zVEC3(m[0][3], m[1][3], m[2][3]);
}

float zVEC3::Length2() const
{
    return n[0] * n[0] + n[1] * n[1] + n[2] * n[2];
}

zVEC3 operator*(const zMAT4& matrix, const zVEC3& vector)
{
    zVEC3 result;
    for (int row = 0; row < 3; ++row)
    {
        result.n[row] = matrix.m[row][0] * vector.n[0] + matrix.m[row][1] * vector.n[1] +
            matrix.m[row][2] * vector.n[2] + matrix.m[row][3];
    }
    return result;
}


zVEC3 operator*(const zVEC3& vector, float value)
{
    zVEC3 result;
    result.n[0] = vector.n[0] * value;
    result.n[1] = vector.n[1] * value;
    result.n[2] = vector.n[2] * value;
    return result;
}


zMAT4 operator*(const zMAT4& first, const zMAT4& second)
{
    zMAT4 result;
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            result.m[row][col] = first.m[row][0] * second.m[0][col] +
                first.m[row][1] * second.m[1][col] + first.m[row][2] * second.m[2][col] +
                first.m[row][3] * second.m[3][col];
        }
    }
    return result;
}

zVEC3 operator+(const zVEC3& first, const zVEC3& second)
{
    zVEC3 result;
    result.n[0] = first.n[0] + second.n[0];
    result.n[1] = first.n[1] + second.n[1];
    result.n[2] = first.n[2] + second.n[2];
    return result;
}

zVEC3 zMAT4::GetAtVector() const
{
    return zVEC3(m[0][2], m[1][2], m[2][2]);
}
