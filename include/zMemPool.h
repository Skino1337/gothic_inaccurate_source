#ifndef ZMEMPOOL_H
#define ZMEMPOOL_H

#include <cstddef>
#include <new>

enum zTVolatileOverflowMode {
    zVOLATILE_OVERFLOW_FORBID,
    zVOLATILE_OVERFLOW_WRAP,
    zVOLATILE_OVERFLOW_RESIZE
};

class zCMemPoolBlock {
public:

    zCMemPoolBlock* next;
    void*           data;

    static void* operator new(size_t size);
    static void  operator delete(void* memory);
};

class zCVolatileMemoryBase {
public:

    zCVolatileMemoryBase(unsigned int elementSize, unsigned int elementCount,
        zTVolatileOverflowMode mode);
    zCVolatileMemoryBase(unsigned int elementSize, zTVolatileOverflowMode mode);
    virtual ~zCVolatileMemoryBase();

    void         Init(unsigned int elementCount);
    void*        Alloc();
    unsigned int Mark();
    void         Restore(unsigned int mark);
    void         MaxFillPercentage(int percentage);

protected:

    virtual void Destroyed(void* memory);

public:

    size_t                 size;
    size_t                 num;
    size_t                 nextfree;
    size_t                 maxAllocated;
    int                    maxFill;
    zTVolatileOverflowMode overflowMode;
    int                    overflow;
    void*                  data;
};

class zCMemPoolBase {
public:

    zCMemPoolBase(unsigned int elementSize);
    zCMemPoolBase(unsigned int elementSize, unsigned int elementsPerBlock,
        unsigned int preallocateCount);
    ~zCMemPoolBase();

    void*       Alloc();
    void        Free(void* memory);
    void        PreAlloc(unsigned int count, int force);
    static void SetMemDebug(const char* className, const char* fileName, int line);
    static void DisablePools(int disable = 1);

protected:

    void PoolAdd(void* memory, int count, int pool);

public:

    size_t          size;
    size_t          blocksize;
    zCMemPoolBlock* blocks;
    void*           freeList;
    size_t          numFree;
    size_t          preallocate;
    int             forcePrealloc;

    static const char* s_className;
    static const char* s_fileName;
    static int         s_line;
    static int         s_disablePools;
};

template <class T>
class zCMemPool : public zCMemPoolBase {
public:

    zCMemPool()
        : zCMemPoolBase(sizeof(T))
    {
    }

    zCMemPool(unsigned int elementsPerBlock, unsigned int preallocateCount)
        : zCMemPoolBase(sizeof(T), elementsPerBlock, preallocateCount)
    {
    }

    void PoolAdd(T* memory, int count, int pool)
    {
        zCMemPoolBase::PoolAdd(memory, count, pool);
    }
};

template <class T>
class zCVolatileMemory : public zCVolatileMemoryBase {
public:

    zCVolatileMemory(size_t count, zTVolatileOverflowMode mode = zVOLATILE_OVERFLOW_FORBID)
        : zCVolatileMemoryBase(sizeof(T), count, mode)
    {
    }

    zCVolatileMemory(zTVolatileOverflowMode mode = zVOLATILE_OVERFLOW_FORBID)
        : zCVolatileMemoryBase(sizeof(T), mode)
    {
    }

    T* CreateNew() { return new(Alloc()) T; }
    T& Element(size_t index) { return reinterpret_cast<T*>(data)[index]; }
    T& operator[](size_t index) { return Element(index); }
    int LastAllocatedIndex() { return nextfree - 1; }
    int Index(T* object) { return (reinterpret_cast<char*>(object) - static_cast<char*>(data)) / sizeof(T); }

protected:

    virtual void Destroyed(void* memory)
    {
        reinterpret_cast<T*>(memory)->~T();
    }
};

#endif
