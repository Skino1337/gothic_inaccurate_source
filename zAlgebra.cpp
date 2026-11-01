#include "include/zAlgebra.h"
#include <cmath>
#include <istream>
#include <ostream>

zVEC2& zVEC2::Apply(float (*function)(float))
{
    n[0] = function(n[0]);
    n[1] = function(n[1]);
    return *this;
}

zSTRING zVEC2::GetDescription() const
{
    return "| " + zSTRING(n[0], 8) + " " + zSTRING(n[1], 8) + " |";
}

void zVEC2::SetByDescription(const zSTRING& description)
{
    n[0] = description.PickWord_Old(1, 1).ToFloat();
    n[1] = description.PickWord_Old(2, 1).ToFloat();
}

std::ostream& operator<<(std::ostream& output, zVEC2& value)
{
    return output << "| " << value.n[0] << " " << value.n[1] << " |";
}

std::istream& operator>>(std::istream& input, zVEC2& value)
{
    return input >> value.n[0] >> value.n[1];
}

void Alg_Swap(zVEC2& first, zVEC2& second)
{
    const zVEC2 temporary = first;
    first = second;
    second = temporary;
}

zVEC2 Alg_Min(const zVEC2& first, const zVEC2& second)
{
    return zVEC2(first.n[0] < second.n[0] ? first.n[0] : second.n[0],
        first.n[1] < second.n[1] ? first.n[1] : second.n[1]);
}

zVEC2 Alg_Max(const zVEC2& first, const zVEC2& second)
{
    return zVEC2(first.n[0] > second.n[0] ? first.n[0] : second.n[0],
        first.n[1] > second.n[1] ? first.n[1] : second.n[1]);
}

zVEC2 Alg_Prod(const zVEC2& first, const zVEC2& second)
{
    return zVEC2(first.n[0] * second.n[0], first.n[1] * second.n[1]);
}

zVEC3& zVEC3::Apply(float (*function)(float))
{
    n[0] = function(n[0]);
    n[1] = function(n[1]);
    n[2] = function(n[2]);
    return *this;
}

std::ostream& operator<<(std::ostream& output, zVEC3& value)
{
    return output << "| " << value.n[0] << " " << value.n[1] << " " << value.n[2] << " |";
}

std::istream& operator>>(std::istream& input, zVEC3& value)
{
    return input >> value.n[0] >> value.n[1] >> value.n[2];
}

void Alg_Swap(zVEC3& first, zVEC3& second)
{
    const zVEC3 temporary = first;
    first = second;
    second = temporary;
}

zVEC3 Alg_Min(const zVEC3& first, const zVEC3& second)
{
    return zVEC3(first.n[0] < second.n[0] ? first.n[0] : second.n[0],
        first.n[1] < second.n[1] ? first.n[1] : second.n[1],
        first.n[2] < second.n[2] ? first.n[2] : second.n[2]);
}

zVEC3 Alg_Max(const zVEC3& first, const zVEC3& second)
{
    return zVEC3(first.n[0] > second.n[0] ? first.n[0] : second.n[0],
        first.n[1] > second.n[1] ? first.n[1] : second.n[1],
        first.n[2] > second.n[2] ? first.n[2] : second.n[2]);
}

zVEC3 Alg_Prod(const zVEC3& first, const zVEC3& second)
{
    return zVEC3(first.n[0] * second.n[0], first.n[1] * second.n[1],
        first.n[2] * second.n[2]);
}

float zVEC4::LengthApprox()
{
    float first = std::fabs(n[0]);
    float second = std::fabs(n[1]);
    float third = std::fabs(n[2]);
    float fourth = std::fabs(n[3]);

    if (first > second) {
        const float temporary = first;
        first = second;
        second = temporary;
    }
    if (third > fourth) {
        const float temporary = third;
        third = fourth;
        fourth = temporary;
    }
    if (first > third) {
        const float temporary = first;
        first = third;
        third = temporary;
    }
    if (second > fourth) {
        const float temporary = second;
        second = fourth;
        fourth = temporary;
    }
    if (second > third) {
        const float temporary = second;
        second = third;
        third = temporary;
    }

    return first + (fourth * 16.0f + third * 19.0f + second * 25.0f) * 0.0166666675f;
}

zVEC4& zVEC4::Normalize()
{
    const float inverseLength = 1.0f / std::sqrt(n[0] * n[0] + n[1] * n[1]
        + n[2] * n[2] + n[3] * n[3]);
    n[0] *= inverseLength;
    n[1] *= inverseLength;
    n[2] *= inverseLength;
    n[3] *= inverseLength;
    return *this;
}

zVEC4& zVEC4::Apply(float (*function)(float))
{
    n[0] = function(n[0]);
    n[1] = function(n[1]);
    n[2] = function(n[2]);
    n[3] = function(n[3]);
    return *this;
}

std::ostream& operator<<(std::ostream& output, zVEC4& value)
{
    return output << "| " << value.n[0] << " " << value.n[1] << " "
        << value.n[2] << " " << value.n[3] << " |";
}

std::istream& operator>>(std::istream& input, zVEC4& value)
{
    return input >> value.n[0] >> value.n[1] >> value.n[2] >> value.n[3];
}

void Alg_Swap(zVEC4& first, zVEC4& second)
{
    const zVEC4 temporary = first;
    first = second;
    second = temporary;
}

zVEC4 Alg_Min(const zVEC4& first, const zVEC4& second)
{
    return zVEC4(first.n[0] < second.n[0] ? first.n[0] : second.n[0],
        first.n[1] < second.n[1] ? first.n[1] : second.n[1],
        first.n[2] < second.n[2] ? first.n[2] : second.n[2],
        first.n[3] < second.n[3] ? first.n[3] : second.n[3]);
}

zVEC4 Alg_Max(const zVEC4& first, const zVEC4& second)
{
    return zVEC4(first.n[0] > second.n[0] ? first.n[0] : second.n[0],
        first.n[1] > second.n[1] ? first.n[1] : second.n[1],
        first.n[2] > second.n[2] ? first.n[2] : second.n[2],
        first.n[3] > second.n[3] ? first.n[3] : second.n[3]);
}

zVEC4 Alg_Prod(const zVEC4& first, const zVEC4& second)
{
    return zVEC4(first.n[0] * second.n[0], first.n[1] * second.n[1],
        first.n[2] * second.n[2], first.n[3] * second.n[3]);
}

zMAT3::zMAT3(const zVEC3& first, const zVEC3& second, const zVEC3& third)
{
    v[0] = first;
    v[1] = second;
    v[2] = third;
}

zMAT3::zMAT3(float value)
{
    v[0] = zVEC3(value, value, value);
    v[1] = zVEC3(value, value, value);
    v[2] = zVEC3(value, value, value);
}

zMAT3::zMAT3(const zMAT3& other)
{
    *this = other;
}

zMAT3& zMAT3::operator=(const zMAT3& other)
{
    v[0] = other.v[0];
    v[1] = other.v[1];
    v[2] = other.v[2];
    return *this;
}

zMAT3& zMAT3::operator+=(const zMAT3& other)
{
    v[0] += other.v[0];
    v[1] += other.v[1];
    v[2] += other.v[2];
    return *this;
}

zMAT3& zMAT3::operator-=(const zMAT3& other)
{
    v[0] -= other.v[0];
    v[1] -= other.v[1];
    v[2] -= other.v[2];
    return *this;
}

zMAT3& zMAT3::operator*=(float value)
{
    v[0] *= value;
    v[1] *= value;
    v[2] *= value;
    return *this;
}

zMAT3& zMAT3::operator/=(float value)
{
    v[0] /= value;
    v[1] /= value;
    v[2] /= value;
    return *this;
}

zMAT3 zMAT3::Transpose() const
{
    return zMAT3(zVEC3(v[0].n[0], v[1].n[0], v[2].n[0]),
        zVEC3(v[0].n[1], v[1].n[1], v[2].n[1]),
        zVEC3(v[0].n[2], v[1].n[2], v[2].n[2]));
}

zMAT3 zMAT3::Inverse(float* determinant) const
{
    const float det = (v[1].n[1] * v[2].n[2] - v[2].n[1] * v[1].n[2]) * v[0].n[0]
        + (v[1].n[2] * v[2].n[0] - v[2].n[2] * v[1].n[0]) * v[0].n[1]
        + (v[2].n[1] * v[1].n[0] - v[1].n[1] * v[2].n[0]) * v[0].n[2];
    if (determinant)
        *determinant = det;
    const float inverseDet = 1.0f / det;
    return zMAT3(
        zVEC3(v[1].n[1] * v[2].n[2] - v[2].n[1] * v[1].n[2],
            v[2].n[1] * v[0].n[2] - v[0].n[1] * v[2].n[2],
            v[0].n[1] * v[1].n[2] - v[1].n[1] * v[0].n[2]),
        zVEC3(v[1].n[2] * v[2].n[0] - v[2].n[2] * v[1].n[0],
            v[0].n[0] * v[2].n[2] - v[0].n[2] * v[2].n[0],
            v[1].n[0] * v[0].n[2] - v[0].n[0] * v[1].n[2]),
        zVEC3(v[2].n[1] * v[1].n[0] - v[1].n[1] * v[2].n[0],
            v[0].n[1] * v[2].n[0] - v[2].n[1] * v[0].n[0],
            v[0].n[0] * v[1].n[1] - v[0].n[1] * v[1].n[0])) * inverseDet;
}

zMAT3& zMAT3::Apply(float (*function)(float))
{
    v[0].Apply(function);
    v[1].Apply(function);
    v[2].Apply(function);
    return *this;
}

zMAT3 operator-(const zMAT3& value)
{
    return zMAT3(-value.v[0], -value.v[1], -value.v[2]);
}

zMAT3 operator+(const zMAT3& first, const zMAT3& second)
{
    zMAT3 result = first;
    return result += second;
}

zMAT3 operator-(const zMAT3& first, const zMAT3& second)
{
    zMAT3 result = first;
    return result -= second;
}

zMAT3 operator*(const zMAT3& first, const zMAT3& second)
{
    zMAT3 result;
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++) {
            result.v[row].n[column] = first.v[row].n[0] * second.v[0].n[column]
                + first.v[row].n[1] * second.v[1].n[column]
                + first.v[row].n[2] * second.v[2].n[column];
        }
    }
    return result;
}

zMAT3 operator*(const zMAT3& value, float scalar)
{
    zMAT3 result = value;
    return result *= scalar;
}

zMAT3 operator*(float scalar, const zMAT3& value)
{
    return value * scalar;
}

zMAT3 operator/(const zMAT3& value, float scalar)
{
    zMAT3 result = value;
    return result /= scalar;
}

int operator==(const zMAT3& first, const zMAT3& second)
{
    return first.v[0] == second.v[0] && first.v[1] == second.v[1]
        && first.v[2] == second.v[2];
}

int operator!=(const zMAT3& first, const zMAT3& second)
{
    return !(first == second);
}

std::ostream& operator<<(std::ostream& output, zMAT3& value)
{
    for (int row = 0; row < 3; row++)
        output << value.v[row];
    return output;
}

std::istream& operator>>(std::istream& input, zMAT3& value)
{
    for (int row = 0; row < 3; row++)
        input >> value.v[row];
    return input;
}

void Alg_Swap(zMAT3& first, zMAT3& second)
{
    const zMAT3 temporary = first;
    first = second;
    second = temporary;
}

zVEC3 operator*(const zMAT3& matrix, const zVEC3& vector)
{
    zVEC3 result;
    for (int row = 0; row < 3; row++)
        result.n[row] = matrix.v[row] * vector;
    return result;
}

zMAT3 operator*(const zMAT4& first, const zMAT3& second)
{
    zMAT3 result;
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++) {
            result.v[row].n[column] = first.m[row][0] * second.v[0].n[column]
                + first.m[row][1] * second.v[1].n[column]
                + first.m[row][2] * second.v[2].n[column];
        }
    }
    return result;
}

zMAT3 operator*(const zMAT3& first, const zMAT4& second)
{
    zMAT3 result;
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++) {
            result.v[row].n[column] = first.v[row].n[0] * second.m[0][column]
                + first.v[row].n[1] * second.m[1][column]
                + first.v[row].n[2] * second.m[2][column];
        }
    }
    return result;
}

zMAT4::zMAT4(const zVEC4& first, const zVEC4& second, const zVEC4& third, const zVEC4& fourth)
{
    for (int i = 0; i < 4; i++) {
        m[0][i] = first.n[i];
        m[1][i] = second.n[i];
        m[2][i] = third.n[i];
        m[3][i] = fourth.n[i];
    }
}

zMAT4::zMAT4(float value)
{
    for (int row = 0; row < 4; row++)
        for (int column = 0; column < 4; column++)
            m[row][column] = value;
}

zMAT4& zMAT4::operator=(const zMAT3& other)
{
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++)
            m[row][column] = other.v[row].n[column];
        m[row][3] = 0.0f;
    }
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
    return *this;
}

zMAT4& zMAT4::operator=(const zMAT4& other)
{
    for (int row = 0; row < 4; row++)
        for (int column = 0; column < 4; column++)
            m[row][column] = other.m[row][column];
    return *this;
}

zMAT4& zMAT4::operator+=(const zMAT4& other)
{
    for (int row = 0; row < 4; row++)
        for (int column = 0; column < 4; column++)
            m[row][column] += other.m[row][column];
    return *this;
}

zMAT4& zMAT4::operator-=(const zMAT4& other)
{
    for (int row = 0; row < 4; row++)
        for (int column = 0; column < 4; column++)
            m[row][column] -= other.m[row][column];
    return *this;
}

zMAT4& zMAT4::operator*=(float value)
{
    for (int row = 0; row < 4; row++)
        for (int column = 0; column < 4; column++)
            m[row][column] *= value;
    return *this;
}

zMAT4& zMAT4::operator/=(float value)
{
    for (int row = 0; row < 4; row++)
        for (int column = 0; column < 4; column++)
            m[row][column] /= value;
    return *this;
}

zMAT4 zMAT4::Transpose() const
{
    zMAT4 result;
    for (int row = 0; row < 4; row++)
        for (int column = 0; column < 4; column++)
            result.m[row][column] = m[column][row];
    return result;
}

zMAT4 zMAT4::TransposeLinTrafo() const
{
    zMAT4 result;
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++)
            result.m[row][column] = m[column][row];
        result.m[row][3] = -(m[0][3] * m[row][0] + m[1][3] * m[row][1] + m[2][3] * m[row][2]);
    }
    result.m[3][0] = 0.0f;
    result.m[3][1] = 0.0f;
    result.m[3][2] = 0.0f;
    result.m[3][3] = 1.0f;
    return result;
}

zMAT4 zMAT4::InverseLinTrafo() const
{
    return TransposeLinTrafo();
}

zMAT4 zMAT4::Inverse() const
{
    zMAT4 work = *this;
    zMAT4 result(0.0f);
    for (int i = 0; i < 4; i++)
        result.m[i][i] = 1.0f;

    for (int column = 0; column < 4; column++) {
        int pivot = column;
        for (int row = column + 1; row < 4; row++) {
            if (std::fabs(work.m[row][column]) > std::fabs(work.m[pivot][column]))
                pivot = row;
        }
        if (work.m[pivot][column] == 0.0f)
            return result;

        if (pivot != column) {
            for (int i = 0; i < 4; i++) {
                const float temporary = work.m[column][i];
                work.m[column][i] = work.m[pivot][i];
                work.m[pivot][i] = temporary;
            }
        }

        const float divisor = work.m[column][column];
        for (int i = 0; i < 4; i++)
            work.m[column][i] /= divisor;
        for (int row = 0; row < 4; row++) {
            if (row == column)
                continue;
            const float factor = work.m[row][column];
            for (int i = 0; i < 4; i++)
                work.m[row][i] -= work.m[column][i] * factor;
        }
    }

    return work;
}

zVEC4 operator*(const zVEC4& vector, float scalar)
{
    return zVEC4(vector.n[0] * scalar, vector.n[1] * scalar,
        vector.n[2] * scalar, vector.n[3] * scalar);
}

zMAT4& zMAT4::Apply(float (*function)(float))
{
    for (int row = 0; row < 4; row++)
        for (int column = 0; column < 4; column++)
            m[row][column] = function(m[row][column]);
    return *this;
}

int zMAT4::IsUpper3x3Orthonormal() const
{
    for (int first = 0; first < 3; first++) {
        const zVEC3 firstVector(m[first][0], m[first][1], m[first][2]);
        const float length = firstVector.Length();
        if (length < 0.99f || length > 1.01f)
            return 0;
        for (int second = first + 1; second < 3; second++) {
            const zVEC3 secondVector(m[second][0], m[second][1], m[second][2]);
            if (std::fabs(firstVector * secondVector) > 0.01f)
                return 0;
        }
    }
    return 1;
}

zSTRING zMAT4::GetDescriptionRot() const
{
    zSTRING result;
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++)
            result += zSTRING(m[row][column], 8) + " ";
    }
    return result;
}

zSTRING zMAT4::GetDescriptionPos() const
{
    return zSTRING(m[0][3], 8) + " " + zSTRING(m[1][3], 8) + " " + zSTRING(m[2][3], 8);
}

void zMAT4::SetByDescriptionRot(zSTRING& description)
{
    for (int row = 0; row < 3; row++)
        for (int column = 0; column < 3; column++)
            m[row][column] = description.PickWord_Old(row * 3 + column + 1, 1).ToFloat();
}

void zMAT4::SetByDescriptionPos(zSTRING& description)
{
    m[0][3] = description.PickWord_Old(1, 1).ToFloat();
    m[1][3] = description.PickWord_Old(2, 1).ToFloat();
    m[2][3] = description.PickWord_Old(3, 1).ToFloat();
}

zVEC3 zMAT4::GetEulerAngles() const
{
    zCQuat quat;
    quat.Matrix4ToQuat(*this);
    const float sinPitch = 2.0f * (quat.q[0] * quat.q[2] - quat.q[3] * quat.q[1]);
    const float clamped = sinPitch < -1.0f ? -1.0f : sinPitch > 1.0f ? 1.0f : sinPitch;
    return zVEC3(
        std::atan2(2.0f * (quat.q[0] * quat.q[1] + quat.q[2] * quat.q[3]),
            1.0f - 2.0f * (quat.q[1] * quat.q[1] + quat.q[2] * quat.q[2])),
        std::asin(clamped),
        std::atan2(2.0f * (quat.q[0] * quat.q[3] + quat.q[1] * quat.q[2]),
            1.0f - 2.0f * (quat.q[2] * quat.q[2] + quat.q[3] * quat.q[3])));
}

void zMAT4::SetByEulerAngles(zVEC3 angles)
{
    zCQuat quat = zCQuat::EulerToQuat(angles);
    quat.QuatToMatrix4(*this);
}

zMAT4 operator-(const zMAT4& value)
{
    zMAT4 result = value;
    for (int row = 0; row < 4; row++)
        for (int column = 0; column < 4; column++)
            result.m[row][column] = -result.m[row][column];
    return result;
}

zMAT4 operator+(const zMAT4& first, const zMAT4& second)
{
    zMAT4 result = first;
    return result += second;
}

zMAT4 operator-(const zMAT4& first, const zMAT4& second)
{
    zMAT4 result = first;
    return result -= second;
}

zMAT4 operator*(const zMAT4& value, float scalar)
{
    zMAT4 result = value;
    return result *= scalar;
}

zMAT4 operator*(float scalar, const zMAT4& value)
{
    return value * scalar;
}

zMAT4 operator/(const zMAT4& value, float scalar)
{
    zMAT4 result = value;
    return result /= scalar;
}

int operator==(const zMAT4& first, const zMAT4& second)
{
    for (int row = 0; row < 4; row++)
        for (int column = 0; column < 4; column++)
            if (first.m[row][column] != second.m[row][column])
                return 0;
    return 1;
}

int operator!=(const zMAT4& first, const zMAT4& second)
{
    return !(first == second);
}

std::ostream& operator<<(std::ostream& output, zMAT4& value)
{
    for (int row = 0; row < 4; row++) {
        for (int column = 0; column < 4; column++)
            output << value.m[row][column] << " ";
    }
    return output;
}

std::istream& operator>>(std::istream& input, zMAT4& value)
{
    for (int row = 0; row < 4; row++)
        for (int column = 0; column < 4; column++)
            input >> value.m[row][column];
    return input;
}

void Alg_Swap(zMAT4& first, zMAT4& second)
{
    const zMAT4 temporary = first;
    first = second;
    second = temporary;
}

zVEC3 zMAT4::ExtractScaling() const
{
    return zVEC3(
        std::sqrt(m[0][0] * m[0][0] + m[1][0] * m[1][0] + m[2][0] * m[2][0]),
        std::sqrt(m[0][1] * m[0][1] + m[1][1] * m[1][1] + m[2][1] * m[2][1]),
        std::sqrt(m[0][2] * m[0][2] + m[1][2] * m[1][2] + m[2][2] * m[2][2]));
}

zMAT3 zMAT4::ExtractRotation() const
{
    const zVEC3 scaling = ExtractScaling();
    return zMAT3(
        zVEC3(m[0][0] / scaling.n[0], m[0][1] / scaling.n[1], m[0][2] / scaling.n[2]),
        zVEC3(m[1][0] / scaling.n[0], m[1][1] / scaling.n[1], m[1][2] / scaling.n[2]),
        zVEC3(m[2][0] / scaling.n[0], m[2][1] / scaling.n[1], m[2][2] / scaling.n[2]));
}

void zMAT4::MakeOrthonormal()
{
    zVEC3 zAxis(m[0][2], m[1][2], m[2][2]);
    zAxis.NormalizeSafe();
    zVEC3 yAxis(m[0][1], m[1][1], m[2][1]);
    yAxis -= zAxis * (yAxis * zAxis);
    yAxis.NormalizeSafe();
    const zVEC3 xAxis = yAxis ^ zAxis;
    for (int row = 0; row < 4; row++) {
        m[row][0] = xAxis.n[row];
        m[row][1] = yAxis.n[row];
        m[row][2] = zAxis.n[row];
    }
}

void zMAT3::MakeOrthonormal()
{
    zVEC3 zAxis = v[2];
    zAxis.NormalizeSafe();
    zVEC3 yAxis = v[1];
    yAxis -= zAxis * (yAxis * zAxis);
    yAxis.NormalizeSafe();
    v[0] = yAxis ^ zAxis;
    v[1] = yAxis;
    v[2] = zAxis;
}

void zMAT3::SetUpVector(const zVEC3& vector)
{
    v[1] = vector;
}

void zMAT3::SetRightVector(const zVEC3& vector)
{
    v[0] = vector;
}

void zMAT4::PostRotateX(float angle)
{
    const float radians = angle * -0.0174532924f;
    const float sine = std::sin(radians);
    const float cosine = std::cos(radians);
    for (int row = 0; row < 4; row++) {
        const float first = m[row][1];
        const float second = m[row][2];
        m[row][1] = first * cosine - second * sine;
        m[row][2] = second * cosine + first * sine;
    }
}

void zMAT4::PostRotateY(float angle)
{
    const float radians = angle * -0.0174532924f;
    const float sine = std::sin(radians);
    const float cosine = std::cos(radians);
    for (int row = 0; row < 4; row++) {
        const float first = m[row][0];
        const float second = m[row][2];
        m[row][0] = first * cosine + second * sine;
        m[row][2] = second * cosine - first * sine;
    }
}

void zMAT4::PostRotateZ(float angle)
{
    const float radians = angle * -0.0174532924f;
    const float sine = std::sin(radians);
    const float cosine = std::cos(radians);
    for (int row = 0; row < 4; row++) {
        const float first = m[row][0];
        const float second = m[row][1];
        m[row][0] = first * cosine - second * sine;
        m[row][1] = second * cosine + first * sine;
    }
}

void zMAT4::PostScale(const zVEC3& scale)
{
    for (int row = 0; row < 4; row++) {
        m[row][0] *= scale.n[0];
        m[row][1] *= scale.n[1];
        m[row][2] *= scale.n[2];
    }
}

void zMAT4::PreScale(const zVEC3& scale)
{
    for (int column = 0; column < 4; column++) {
        m[0][column] *= scale.n[0];
        m[1][column] *= scale.n[1];
        m[2][column] *= scale.n[2];
    }
}

zMAT3 Alg_Identity2D()
{
    return zMAT3(zVEC3(1.0f, 0.0f, 0.0f), zVEC3(0.0f, 1.0f, 0.0f),
        zVEC3(0.0f, 0.0f, 1.0f));
}

zMAT3 Alg_Translation2D(zVEC2& translation)
{
    return zMAT3(zVEC3(1.0f, 0.0f, translation.n[0]),
        zVEC3(0.0f, 1.0f, translation.n[1]), zVEC3(0.0f, 0.0f, 1.0f));
}

zMAT3 Alg_Rotation2D(zVEC2& center, float angle)
{
    const float radians = angle * 0.0174532924f;
    const float sine = std::sin(radians);
    const float cosine = std::cos(radians);
    const float remainder = 1.0f - cosine;
    return zMAT3(
        zVEC3(cosine, -sine, sine * center.n[1] + center.n[0] * remainder),
        zVEC3(sine, cosine, center.n[1] * remainder - sine * center.n[0]),
        zVEC3(0.0f, 0.0f, 1.0f));
}

zMAT3 Alg_Scaling2D(zVEC2& scaling)
{
    return zMAT3(zVEC3(scaling.n[0], 0.0f, 0.0f),
        zVEC3(0.0f, scaling.n[1], 0.0f), zVEC3(0.0f, 0.0f, 1.0f));
}

zMAT4 Alg_Identity3D()
{
    zMAT4 result(0.0f);
    for (int i = 0; i < 4; i++)
        result.m[i][i] = 1.0f;
    return result;
}

zMAT4 Alg_Translation3D(const zVEC3& translation)
{
    zMAT4 result = Alg_Identity3D();
    result.SetTranslation(translation);
    return result;
}

zMAT4 Alg_Rotation3D(const zVEC3& axis, float angle)
{
    zVEC3 normalized = axis;
    normalized.Normalize();
    return Alg_Rotation3DNRad(normalized, angle * 0.0174532924f);
}

zMAT4 Alg_Rotation3DN(const zVEC3& axis, float angle)
{
    return Alg_Rotation3DNRad(axis, angle * 0.0174532924f);
}

zMAT4 Alg_Rotation3DRad(const zVEC3& axis, float angle)
{
    zVEC3 normalized = axis;
    normalized.Normalize();
    return Alg_Rotation3DNRad(normalized, angle);
}

zMAT4 Alg_Rotation3DNRad(const zVEC3& axis, float angle)
{
    const float sine = std::sin(angle);
    const float cosine = std::cos(angle);
    const float remainder = 1.0f - cosine;
    const float xy = axis.n[0] * axis.n[1] * remainder;
    const float xz = axis.n[0] * axis.n[2] * remainder;
    const float yz = axis.n[1] * axis.n[2] * remainder;
    const float xs = axis.n[0] * sine;
    const float ys = axis.n[1] * sine;
    const float zs = axis.n[2] * sine;
    zMAT4 result = Alg_Identity3D();
    result.m[0][0] = axis.n[0] * axis.n[0] * remainder + cosine;
    result.m[0][1] = xy - zs;
    result.m[0][2] = xz + ys;
    result.m[1][0] = xy + zs;
    result.m[1][1] = axis.n[1] * axis.n[1] * remainder + cosine;
    result.m[1][2] = yz - xs;
    result.m[2][0] = xz - ys;
    result.m[2][1] = yz + xs;
    result.m[2][2] = axis.n[2] * axis.n[2] * remainder + cosine;
    return result;
}

zMAT4 Alg_Scaling3D(const zVEC3& scaling)
{
    zMAT4 result = Alg_Identity3D();
    result.m[0][0] = scaling.n[0];
    result.m[1][1] = scaling.n[1];
    result.m[2][2] = scaling.n[2];
    return result;
}

zMAT4 Alg_Perspective3D(float distance)
{
    zMAT4 result = Alg_Identity3D();
    result.m[3][2] = 1.0f / distance;
    result.m[3][3] = 0.0f;
    return result;
}

void Alg_ClipAtZ0(zVEC3& first, zVEC3& second)
{
    if ((first.n[2] < 0.0f && second.n[2] < 0.0f)
            || (first.n[2] > 0.0f && second.n[2] > 0.0f)
            || second.n[2] == first.n[2])
        return;

    const float weight = -first.n[2] / (second.n[2] - first.n[2]);
    const zVEC3 intersection = first + (second - first) * weight;
    if (first.n[2] < 0.0f) {
        second = intersection;
        second.n[2] = 10.0f;
    }
    else {
        first = intersection;
        first.n[2] = 10.0f;
    }
}

void zCQuat::Unit()
{
    const float inverseLength = 1.0f / std::sqrt(q[0] * q[0] + q[1] * q[1]
        + q[2] * q[2] + q[3] * q[3]);
    q[0] *= inverseLength;
    q[1] *= inverseLength;
    q[2] *= inverseLength;
    q[3] *= inverseLength;
}

void zCQuat::QuatToMatrix4(zMAT4& matrix) const
{
    matrix.m[0][0] = q[3] * q[3] + q[0] * q[0] - q[1] * q[1] - q[2] * q[2];
    matrix.m[0][1] = 2.0f * (q[2] * q[3] + q[0] * q[1]);
    matrix.m[0][2] = 2.0f * (q[2] * q[0] - q[3] * q[1]);
    matrix.m[1][0] = 2.0f * (q[0] * q[1] - q[2] * q[3]);
    matrix.m[1][1] = q[3] * q[3] - q[0] * q[0] + q[1] * q[1] - q[2] * q[2];
    matrix.m[1][2] = 2.0f * (q[2] * q[1] + q[0] * q[3]);
    matrix.m[2][0] = 2.0f * (q[2] * q[0] + q[3] * q[1]);
    matrix.m[2][1] = 2.0f * (q[2] * q[1] - q[0] * q[3]);
    matrix.m[2][2] = q[3] * q[3] - q[0] * q[0] - q[1] * q[1] + q[2] * q[2];
}

void zCQuat::QuatToMatrix3(zMAT3& matrix) const
{
    matrix.v[0].n[0] = q[3] * q[3] + q[0] * q[0] - q[1] * q[1] - q[2] * q[2];
    matrix.v[0].n[1] = 2.0f * (q[2] * q[3] + q[0] * q[1]);
    matrix.v[0].n[2] = 2.0f * (q[2] * q[0] - q[3] * q[1]);
    matrix.v[1].n[0] = 2.0f * (q[0] * q[1] - q[2] * q[3]);
    matrix.v[1].n[1] = q[3] * q[3] - q[0] * q[0] + q[1] * q[1] - q[2] * q[2];
    matrix.v[1].n[2] = 2.0f * (q[2] * q[1] + q[0] * q[3]);
    matrix.v[2].n[0] = 2.0f * (q[2] * q[0] + q[3] * q[1]);
    matrix.v[2].n[1] = 2.0f * (q[2] * q[1] - q[0] * q[3]);
    matrix.v[2].n[2] = q[3] * q[3] - q[0] * q[0] - q[1] * q[1] + q[2] * q[2];
}

static void MatrixToQuat(const float m[3][3], zCQuat& quat)
{
    const float trace = m[0][0] + m[1][1] + m[2][2];
    if (trace > 0.0f) {
        const float root = std::sqrt(trace + 1.0f);
        quat.q[3] = root * 0.5f;
        const float scale = 0.5f / root;
        quat.q[0] = (m[1][2] - m[2][1]) * scale;
        quat.q[1] = (m[2][0] - m[0][2]) * scale;
        quat.q[2] = (m[0][1] - m[1][0]) * scale;
        return;
    }

    int largest = 0;
    if (m[1][1] > m[0][0])
        largest = 1;
    if (m[2][2] > m[largest][largest])
        largest = 2;
    const int next = (largest + 1) % 3;
    const int last = (next + 1) % 3;
    const float root = std::sqrt(m[largest][largest] - m[next][next] - m[last][last] + 1.0f);
    quat.q[largest] = root * 0.5f;
    const float scale = root != 0.0f ? 0.5f / root : 0.0f;
    quat.q[3] = (m[next][last] - m[last][next]) * scale;
    quat.q[next] = (m[largest][next] + m[next][largest]) * scale;
    quat.q[last] = (m[largest][last] + m[last][largest]) * scale;
}

void zCQuat::Matrix4ToQuat(const zMAT4& matrix)
{
    const float values[3][3] = {
        { matrix.m[0][0], matrix.m[0][1], matrix.m[0][2] },
        { matrix.m[1][0], matrix.m[1][1], matrix.m[1][2] },
        { matrix.m[2][0], matrix.m[2][1], matrix.m[2][2] }
    };
    MatrixToQuat(values, *this);
}

void zCQuat::Matrix3ToQuat(const zMAT3& matrix)
{
    const float values[3][3] = {
        { matrix.v[0].n[0], matrix.v[0].n[1], matrix.v[0].n[2] },
        { matrix.v[1].n[0], matrix.v[1].n[1], matrix.v[1].n[2] },
        { matrix.v[2].n[0], matrix.v[2].n[1], matrix.v[2].n[2] }
    };
    MatrixToQuat(values, *this);
}

void zCQuat::QuatToEuler(zVEC3& angles) const
{
    const float sinPitch = -2.0f * (q[0] * q[2] - q[3] * q[1]);
    if (std::fabs(sinPitch) < 0.99999f) {
        angles.n[0] = std::atan2(2.0f * (q[3] * q[2] + q[0] * q[1]),
            2.0f * (q[3] * q[3] + q[2] * q[2]) - 1.0f);
        angles.n[1] = std::asin(sinPitch);
        angles.n[2] = std::atan2(2.0f * (q[3] * q[0] + q[1] * q[2]),
            2.0f * (q[3] * q[3] + q[0] * q[0]) - 1.0f);
        return;
    }

    angles.n[2] = 0.0f;
    angles.n[1] = sinPitch < 0.0f ? -1.57079637f : 1.57079637f;
    angles.n[0] = std::atan2(-2.0f * (q[1] * q[2] - q[3] * q[0]),
        2.0f * (q[1] * q[1] + q[3] * q[3]) - 1.0f);
}

zCQuat zCQuat::EulerToQuat(const zVEC3& angles)
{
    const float x = angles.n[0] * 0.5f;
    const float y = angles.n[1] * 0.5f;
    const float z = angles.n[2] * 0.5f;
    const float sx = std::sin(x);
    const float cx = std::cos(x);
    const float sy = std::sin(y);
    const float cy = std::cos(y);
    const float sz = std::sin(z);
    const float cz = std::cos(z);
    zCQuat result;
    result.q[0] = cz * cy * sx - sz * sy * cx;
    result.q[1] = sz * cy * sx + cz * sy * cx;
    result.q[2] = cz * sy * cx - sz * cy * sx;
    result.q[3] = sz * sy * sx + cz * cy * cx;
    return result;
}

void zCQuat::QuatToAxisAngle(zVEC3& axis, float& angle) const
{
    const float length2 = q[0] * q[0] + q[1] * q[1] + q[2] * q[2];
    if (length2 < 0.000001f) {
        axis = zVEC3(0.0f, 0.0f, 1.0f);
        angle = 0.0f;
        return;
    }

    const float inverseLength = 1.0f / std::sqrt(length2);
    axis = zVEC3(q[0] * inverseLength, q[1] * inverseLength, q[2] * inverseLength);
    angle = 2.0f * std::acos(q[3]);
}

void zCQuat::Slerp(float weight, const zCQuat& first, const zCQuat& second)
{
    float dot = first.q[0] * second.q[0] + first.q[1] * second.q[1]
        + first.q[2] * second.q[2] + first.q[3] * second.q[3];
    int positive = 1;
    if (dot < 0.0f) {
        dot = -dot;
        positive = 0;
    }
    if (dot > 1.0f)
        dot = 1.0f;

    const float angle = std::acos(dot);
    const float sine = std::sin(angle);
    if (sine < 0.001f) {
        *this = first;
        return;
    }

    const float firstWeight = std::sin((1.0f - weight) * angle) / sine;
    const float secondWeight = std::sin(weight * angle) / sine;
    for (int i = 0; i < 4; i++)
        q[i] = firstWeight * first.q[i] + (positive ? secondWeight : -secondWeight) * second.q[i];
}

void zCQuat::Lerp(float weight, const zCQuat& first, const zCQuat& second)
{
    const float dot = first.q[0] * second.q[0] + first.q[1] * second.q[1]
        + first.q[2] * second.q[2] + first.q[3] * second.q[3];
    const float sign = dot < 0.0f ? -1.0f : 1.0f;
    for (int i = 0; i < 4; i++)
        q[i] = (1.0f - weight) * first.q[i] + weight * sign * second.q[i];
}

void zCQuat::Squad(float weight, const zCQuat& first, const zCQuat& second,
    const zCQuat& third, const zCQuat& fourth)
{
    zCQuat firstHalf;
    firstHalf.Slerp(weight, first, second);
    zCQuat secondHalf;
    secondHalf.Slerp(weight, third, fourth);
    Slerp(2.0f * weight * (1.0f - weight), firstHalf, secondHalf);
}

zCValueSmooth::zCValueSmooth()
{
    fixedValue = 0;
    smoothedValue = 0.0f;
    inertia = 2.0f;
    bSmooth = 1;
}

void zCValueSmooth::Smooth(float& value)
{
    fixedValue = value;
    if (!bSmooth)
        return;

    const float delta = value - smoothedValue;
    const float weight = (delta * delta * 4.0f + 1.0f) / (delta * delta + 1.0f)
        * ztimer.frameTimeFloatSecs / (0.025f * inertia);
    smoothedValue = (weight * value + smoothedValue) / (weight + 1.0f);
    value = smoothedValue;
}

zCVEC3Smooth::zCVEC3Smooth()
{
    fixedValue = zVEC3(0.0f, 0.0f, 0.0f);
    smoothedValue = zVEC3(0.0f, 0.0f, 0.0f);
    inertia = 2.0f;
    bSmooth = 1;
}

void zCVEC3Smooth::Smooth(zVEC3& value)
{
    fixedValue = value;
    if (!bSmooth)
        return;

    const zVEC3 delta = value - smoothedValue;
    const float length2 = delta.Length2();
    const float weight = (length2 * 4.0f + 1.0f) / (length2 + 1.0f)
        * ztimer.frameTimeFloatSecs / (0.025f * inertia);
    smoothedValue = (value * weight + smoothedValue) / (weight + 1.0f);
    value = smoothedValue;
}
