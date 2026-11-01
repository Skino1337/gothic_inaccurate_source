#include "include/zMemPool.h"
#include "include/zObject.h"
#include "include/zString.h"
#include <cstdlib>
#include <cstring>
#include <new>

struct zCFreeBlock {
    zCFreeBlock*  next;
    zCFreeBlock** prev;
};

const char* zCMemPoolBase::s_className = NULL;
const char* zCMemPoolBase::s_fileName = NULL;
int zCMemPoolBase::s_line = 0;
int zCMemPoolBase::s_disablePools = 0;

static zCMemPoolBase s_TreePool(0x14, 0x1000, 0x4000);
static zCMemPoolBase s_ListPool(8, 0x1000, 0x5000);
static zCMemPoolBase s_ListSortPool(0x0C, 0x1000, 0x5000);

void* zCMemPoolBlock::operator new(size_t size)
{
    return std::malloc(size);
}

void zCMemPoolBlock::operator delete(void* memory)
{
    std::free(memory);
}

zCVolatileMemoryBase::zCVolatileMemoryBase(unsigned int elementSize, unsigned int elementCount,
    zTVolatileOverflowMode mode)
    : size(elementSize), num(elementCount), nextfree(0), maxAllocated(0), maxFill(80),
      overflowMode(mode), overflow(0), data(NULL)
{
    data = new char[size * num];
}

zCVolatileMemoryBase::zCVolatileMemoryBase(unsigned int elementSize, zTVolatileOverflowMode mode)
    : size(elementSize), num(0), nextfree(0), maxAllocated(0), maxFill(80),
      overflowMode(mode), overflow(0), data(NULL)
{
}

zCVolatileMemoryBase::~zCVolatileMemoryBase()
{
    Restore(num);
    delete[] static_cast<char*>(data);
    data = NULL;
}

void zCVolatileMemoryBase::Destroyed(void* memory)
{
}

void zCVolatileMemoryBase::Init(unsigned int elementCount)
{
    num = elementCount;
    data = new char[size * num];
}

void* zCVolatileMemoryBase::Alloc()
{
    if (nextfree >= num) {
        if (overflowMode == zVOLATILE_OVERFLOW_FORBID) {
            zerr->Report(zERR_TYPE_FATAL, 0, zSTRING("M: MemBlock allocation overflow."), -1, 0,
                179, const_cast<char*>("P:\\dev\\g2addon\\release\\ZenGin\\_dieter\\zMemPool.cpp"), NULL);
        } else if (overflowMode == zVOLATILE_OVERFLOW_WRAP) {
            nextfree = 0;
            overflow = 1;
        } else if (overflowMode == zVOLATILE_OVERFLOW_RESIZE) {
            const size_t oldNum = num;
            void* oldData = data;
            num = oldNum * 200 / maxFill;
            data = new char[size * num];
            std::memcpy(data, oldData, size * oldNum);
            delete[] static_cast<char*>(oldData);
        }
    }

    void* result = static_cast<char*>(data) + size * nextfree;
    ++nextfree;
    if (overflow)
        Destroyed(result);
    return result;
}

unsigned int zCVolatileMemoryBase::Mark()
{
    return nextfree ? nextfree : num;
}

void zCVolatileMemoryBase::Restore(unsigned int mark)
{
    size_t allocated = nextfree;
    if (overflow)
        allocated += num;
    if (allocated > maxAllocated)
        maxAllocated = allocated;

    bool resize = false;
    if (!mark && maxAllocated * 100 > maxFill * num)
        resize = true;
    if (mark == num)
        mark = 0;

    while (nextfree != mark) {
        if (nextfree == 0) {
            nextfree = num;
            overflow = 0;
        }
        --nextfree;
        Destroyed(static_cast<char*>(data) + size * nextfree);
    }

    if (resize) {
        delete[] static_cast<char*>(data);
        data = NULL;
        num = maxAllocated * 200 / maxFill;
        data = new char[size * num];
    }
}

void zCVolatileMemoryBase::MaxFillPercentage(int percentage)
{
    maxFill = percentage;
}

zCMemPoolBase::zCMemPoolBase(unsigned int elementSize)
    : size(elementSize), blocksize(0x10000 / elementSize), blocks(NULL), freeList(NULL),
      numFree(0), preallocate(0)
{
}

zCMemPoolBase::zCMemPoolBase(unsigned int elementSize, unsigned int elementsPerBlock,
    unsigned int preallocateCount)
    : size(elementSize), blocksize(elementsPerBlock), blocks(NULL), freeList(NULL),
      numFree(0), preallocate(preallocateCount)
{
    if (!blocksize)
        blocksize = 0x10000 / elementSize;
}

zCMemPoolBase::~zCMemPoolBase()
{
    zCFreeBlock* entry = static_cast<zCFreeBlock*>(freeList);
    while (entry) {
        zCFreeBlock* next = entry->next;
        entry->next = NULL;
        entry->prev = NULL;
        entry = next;
    }
    freeList = NULL;

    while (blocks) {
        zCMemPoolBlock* next = blocks->next;
        delete[] static_cast<char*>(blocks->data);
        delete blocks;
        blocks = next;
    }
}

void zCMemPoolBase::PoolAdd(void* memory, int count, int pool)
{
    if (s_disablePools && pool) {
        delete[] static_cast<char*>(memory);
        return;
    }

    zCMemPoolBlock* block = new zCMemPoolBlock;
    if (!block)
        return;

    block->next = blocks;
    blocks = block;
    block->data = memory;
    if (count <= 0)
        return;

    const size_t elementSize = size < sizeof(zCFreeBlock) ? sizeof(zCFreeBlock) : size;
    numFree += count;
    for (int i = count - 1; i >= 0; --i) {
        zCFreeBlock* entry = reinterpret_cast<zCFreeBlock*>(
            static_cast<char*>(memory) + i * elementSize);
        entry->next = static_cast<zCFreeBlock*>(freeList);
        entry->prev = reinterpret_cast<zCFreeBlock**>(&freeList);
        freeList = entry;
        if (entry->next)
            entry->next->prev = &entry->next;
    }
}

void* zCMemPoolBase::Alloc()
{
    if (s_disablePools) {
        void* result = new char[size];
        if (s_className)
            s_className = NULL;
        return result;
    }

    if (!freeList) {
        const size_t elementSize = size < sizeof(zCFreeBlock) ? sizeof(zCFreeBlock) : size;
        zCMemPoolBlock* block = new zCMemPoolBlock;
        if (block) {
            block->next = blocks;
            blocks = block;
            block->data = new char[elementSize * blocksize];
            if (s_className)
                s_className = NULL;
            PoolAdd(block->data, blocksize, 0);
            delete block;
        }
    }

    zCFreeBlock* result = static_cast<zCFreeBlock*>(freeList);
    --numFree;
    *result->prev = result->next;
    if (result->next)
        result->next->prev = result->prev;
    result->next = NULL;
    result->prev = NULL;
    return result;
}

void zCMemPoolBase::Free(void* memory)
{
    if (!memory)
        return;
    if (s_disablePools) {
        delete[] static_cast<char*>(memory);
        return;
    }

    zCFreeBlock* entry = static_cast<zCFreeBlock*>(memory);
    entry->next = static_cast<zCFreeBlock*>(freeList);
    entry->prev = reinterpret_cast<zCFreeBlock**>(&freeList);
    freeList = entry;
    if (entry->next)
        entry->next->prev = &entry->next;
    ++numFree;
}

void zCMemPoolBase::PreAlloc(unsigned int count, int force)
{
    preallocate += count;
    forcePrealloc |= force;
    if (forcePrealloc)
        PoolAdd(new char[(size < sizeof(zCFreeBlock) ? sizeof(zCFreeBlock) : size) *
            (preallocate ? preallocate : blocksize)], preallocate ? preallocate : blocksize, 0);
    if (preallocate > numFree)
        PoolAdd(new char[(size < sizeof(zCFreeBlock) ? sizeof(zCFreeBlock) : size) *
            (preallocate - numFree)], preallocate - numFree, 0);
    preallocate = 0;
}

void zCMemPoolBase::SetMemDebug(const char* className, const char* fileName, int line)
{
    s_className = className;
    s_fileName = fileName;
    s_line = line;
}

void zCMemPoolBase::DisablePools(int disable)
{
    s_disablePools = disable;
}
