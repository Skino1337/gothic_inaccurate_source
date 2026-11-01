#ifndef ZCONTAINER_H
#define ZCONTAINER_H

int  zArraySortDefaultCompare(const void* ele1, const void* ele2);
void insertionsort(void* array, unsigned int numElements, unsigned int elementSize,
    int(__cdecl* compare)(const void*, const void*), bool useQSort);

#endif
