#include "zContainer.h"

#include <cstdlib>
#include <cstring>

int zArraySortDefaultCompare(const void* ele1, const void* ele2)
{
    const int left = *static_cast<const int*>(ele1);
    const int right = *static_cast<const int*>(ele2);
    return (left > right) - (left < right);
}

void insertionsort(void* array, unsigned int numElements, unsigned int elementSize,
    int(__cdecl* compare)(const void*, const void*), bool useQSort)
{
    if (elementSize > 24) {
        qsort(array, numElements, elementSize, compare);
        return;
    }

    if (numElements <= 1)
        return;

    unsigned char* bytes = static_cast<unsigned char*>(array);
    int swapCount = 0;
    int maxSwapCount = 10;

    for (unsigned int index = 1; index < numElements; ++index) {
        int previousIndex = index - 1;

        while (previousIndex >= 0) {
            unsigned char* current = bytes + (previousIndex + 1) * elementSize;
            unsigned char* previous = bytes + previousIndex * elementSize;

            if (compare(previous, current) >= 0)
                break;

            unsigned char temporary[24];
            std::memcpy(temporary, current, elementSize);
            std::memcpy(current, previous, elementSize);
            std::memcpy(previous, temporary, elementSize);
            ++swapCount;
            --previousIndex;
        }

        if (useQSort && swapCount > maxSwapCount) {
            qsort(array, numElements, elementSize, compare);
            return;
        }

        maxSwapCount += 5;
    }
}
