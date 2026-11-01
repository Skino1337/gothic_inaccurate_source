#include "zPrime.h"

static int primeState = -1;
static unsigned short primeList[6641];
static unsigned short primeCount;

int zCPrime::IsPrime(unsigned long value)
{
    if (value <= 1)
        return 0;

    int primeIndex = 0;

    while (true) {
        if (primeIndex >= primeCount) {
            if (!primeCount) {
                primeCount = 1;
                primeList[0] = 2;
            }

            unsigned long candidate = primeList[primeCount - 1] + 1;

            while (primeIndex >= primeCount) {
                if (candidate >= 0x10000)
                    break;

                if (IsPrime(candidate)) {
                    primeList[primeCount] = candidate;
                    ++primeCount;
                }

                ++candidate;
            }
        }

        const unsigned long prime = primeList[primeIndex];

        if (prime * prime > value)
            return 1;

        if (value % prime == 0)
            return 0;

        ++primeIndex;
    }
}

unsigned short zCPrime::PrimeNo(unsigned short value)
{
    if (value >= primeCount) {
        if (!primeCount) {
            primeCount = 1;
            primeList[0] = 2;
        }

        unsigned long candidate = primeList[primeCount - 1] + 1;

        while (value >= primeCount) {
            if (candidate >= 0x10000)
                return primeList[value];

            if (candidate > 1) {
                int primeIndex = 0;

                while (true) {
                    const unsigned long prime = PrimeNo(primeIndex);

                    if (prime * prime > candidate) {
                        primeList[primeCount] = candidate;
                        ++primeCount;
                        break;
                    }

                    if (candidate % prime == 0)
                        break;

                    ++primeIndex;
                }
            }

            ++candidate;
        }
    }

    return primeList[value];
}

unsigned long zCPrime::NextPrime(unsigned long value)
{
    unsigned long result = value;

    while (true) {
        if (result > 1 && IsPrime(result))
            return result;

        if (result == 0xFFFFFFFF)
            break;

        ++result;
    }

    return result;
}
