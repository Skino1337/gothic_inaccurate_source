#include "zMath.h"

#include <cstdlib>
#include <cstring>
#include <cmath>

static int mathState = -1;

float zMin(float a, float b)
{
    return a < b ? a : b;
}

float zRandF2()
{
    return (rand() - 16383.5f) * 6.10370189e-05f;
}

float Alg_SqrtInvApprox(float value)
{
    unsigned long bits;
    std::memcpy(&bits, &value, sizeof(bits));
    bits = 0x5F3759DF - (bits >> 1);

    float approximation;
    std::memcpy(&approximation, &bits, sizeof(approximation));
    return approximation * (1.5f - value * 0.5f * approximation * approximation);
}

int zGetHighestBit(int value)
{
    unsigned int bit = 0x80000000;
    int bitIndex = 31;

    if (value >= 0) {
        while (bitIndex > 0) {
            bit >>= 1;
            --bitIndex;

            if (value & bit)
                break;
        }
    }

    return bitIndex;
}

void zClamp(float& value, float minValue, float maxValue)
{
    if (value < minValue) {
        value = minValue;
        return;
    }

    if (value > maxValue)
        value = maxValue;
}

void zClamp(int& value, int minValue, int maxValue)
{
    if (value < minValue) {
        value = minValue;
        return;
    }

    if (value > maxValue)
        value = maxValue;
}

void zClamp(unsigned long& value, unsigned long minValue, unsigned long maxValue)
{
    if (value < minValue) {
        value = minValue;
        return;
    }

    if (value > maxValue)
        value = maxValue;
}

void zClamp01(float& value)
{
    unsigned long bits;
    static_assert(sizeof(bits) == sizeof(value), "Unexpected float size");
    std::memcpy(&bits, &value, sizeof(bits));

    if (bits >= 0x80000000) {
        value = 0.0f;
        return;
    }

    if (bits > 0x3F800000)
        value = 1.0f;
}

float zSinApprox(float angle)
{
    return std::sin(angle);
}

float zCosApprox(float angle)
{
    return std::cos(angle);
}

void zSinCosApprox(float angle, float& sine, float& cosine)
{
    sine = std::sin(angle);
    cosine = std::cos(angle);
}

void Trg_Init()
{
}
