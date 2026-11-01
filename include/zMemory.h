#ifndef ZMEMORY_H
#define ZMEMORY_H

enum zTMallocDumpHeapMode {
    zMALLOC_SORT_BY_BYTES_PER_LINE,
    zMALLOC_SORT_BY_BLOCKS_PER_LINE,
    zMALLOC_SORT_BY_CLASS,
    zMALLOC_SORT_BY_BLOCK_SIZE,
    zMALLOC_SORT_BY_FILE,
    zMALLOC_SORT_BY_TIME
};

enum zTMallocCheckPtrError {
    zMALLOC_CHECK_PTR_OK,
    zMALLOC_CHECK_PTR_INVALID_PTR,
    zMALLOC_CHECK_PTR_CORRUPTED_MEM,
    zMALLOC_CHECK_PTR_NOT_ACTIVATED,
    zMALLOC_CHECK_PTR_NULL_PTR
};

struct zTMallocStats {
    int numBlocks;
    int numBlocksPeak;
    int numBytesAllocated;
    int numBytesAllocatedPeak;
    int numAllocs;
    int numFrees;
    long long numBytesAllocatedTotal;
    long long numBytesFreedTotal;
};

struct zTMallocProcessStats {
    unsigned long fixedMem;
    unsigned long freeMem;
    unsigned long moveable;
};

class zCMalloc {
public:

    virtual ~zCMalloc() {}
    virtual void* Malloc(unsigned int size) = 0;
    virtual void* Malloc(unsigned int size, const char* className, const char* file, int line) = 0;
    virtual void* Realloc(void* memory, unsigned int size) = 0;
    virtual void Free(void* memory) = 0;
    virtual int GetStats(zTMallocStats& stats);
    virtual int GetProcessStats(zTMallocProcessStats& stats);
    virtual int DumpHeap(zTMallocDumpHeapMode mode);
    virtual int CheckHeap();
    virtual zTMallocCheckPtrError CheckPtr(void* memory);
    virtual int Init(int enabled) = 0;
    virtual int Shutdown() = 0;
};

class zCMallocGeneric : public zCMalloc {
public:

    zCMallocGeneric();
    virtual ~zCMallocGeneric();
    virtual void* Malloc(unsigned int size);
    virtual void* Malloc(unsigned int size, const char* className, const char* file, int line);
    virtual void* Realloc(void* memory, unsigned int size);
    virtual void Free(void* memory);
    virtual int GetStats(zTMallocStats& stats);
    virtual int Init(int enabled);
    virtual int Shutdown();
};

extern zCMallocGeneric zmalloc;

#endif
