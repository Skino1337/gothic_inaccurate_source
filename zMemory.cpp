#include "include/zMemory.h"
#include <cstdlib>
#include <cstring>

static int memoryState = -1;
zCMallocGeneric zmalloc;

int zCMalloc::GetStats(zTMallocStats& stats)
{
    return 0;
}

int zCMalloc::GetProcessStats(zTMallocProcessStats& stats)
{
    return 0;
}

int zCMalloc::DumpHeap(zTMallocDumpHeapMode mode)
{
    return 0;
}

int zCMalloc::CheckHeap()
{
    return 1;
}

zTMallocCheckPtrError zCMalloc::CheckPtr(void* memory)
{
    return zMALLOC_CHECK_PTR_NOT_ACTIVATED;
}

zCMallocGeneric::zCMallocGeneric()
{
}

zCMallocGeneric::~zCMallocGeneric()
{
}

void* operator new(unsigned int size)
{
    return malloc(size);
}

void operator delete(void* memory)
{
    free(memory);
}

void* operator new[](unsigned int size)
{
    return malloc(size);
}

void operator delete[](void* memory)
{
    free(memory);
}

void* operator new(unsigned int size, const char* className, const char* file, int line)
{
    return malloc(size);
}

void operator delete(void* memory, const char* className, const char* file, int line)
{
    free(memory);
}

void* operator new[](unsigned int size, const char* className, const char* file, int line)
{
    return malloc(size);
}

void operator delete[](void* memory, const char* className, const char* file, int line)
{
    free(memory);
}

void* zCMallocGeneric::Malloc(unsigned int size)
{
    return malloc(size);
}

void* zCMallocGeneric::Malloc(unsigned int size, const char* className, const char* file, int line)
{
    return malloc(size);
}

void* zCMallocGeneric::Realloc(void* memory, unsigned int size)
{
    return realloc(memory, size);
}

void zCMallocGeneric::Free(void* memory)
{
    free(memory);
}

int zCMallocGeneric::GetStats(zTMallocStats& stats)
{
    memset(&stats, 0, sizeof(stats));
    return 0;
}

int zCMallocGeneric::Init(int enabled)
{
    return 1;
}

int zCMallocGeneric::Shutdown()
{
    return 1;
}
