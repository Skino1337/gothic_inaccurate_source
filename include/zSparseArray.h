#ifndef ZSPARSEARRAY_H
#define ZSPARSEARRAY_H

class zCSparseArrayManDat {
public:

    unsigned long refCtr   : 31;
    unsigned long occupied : 1;
};

class zCSparseArrayBase {
public:

    zCSparseArrayBase(unsigned long minSize = 0);
    virtual ~zCSparseArrayBase();

    void  Destroy();
    void* Element(const void* index) const;
    void* Insert(const void* index);
    void  Delete(const void* index);
    void  Clear();
    unsigned long Find(const void* index) const;
    void  Create(unsigned long minSize);
    void  Resize(unsigned long minSize);
    void* DataPtr(unsigned long position, void* data) const;
    void  AddRef(zCSparseArrayManDat* data) const;
    void  Release(zCSparseArrayManDat* data) const;
    void  CheckResize() const;

    virtual unsigned long SizeIndex() const = 0;
    virtual unsigned long SizeData() const = 0;
    virtual unsigned long SizeAll() const;
    virtual void* IndexPtr(unsigned long position) const;
    virtual void* DataPtr(unsigned long position) const;
    virtual zCSparseArrayManDat* ManDatPtr(unsigned long position) const;
    virtual int  IndexIsEqual(const void* first, const void* second) const;
    virtual void CopyIndex(void* destination, const void* source) const;
    virtual void DestructIndex(void* index) const;
    virtual void Hash(const void* index, unsigned long& hash, unsigned long& step) const;
    virtual void ConstructData(void* data) const;
    virtual void DestructData(void* data) const;

    unsigned long numberOfIterators;
    unsigned long size;
    unsigned long min_size;
    void*         table;
    unsigned long used;
    unsigned long maxRefCtr;
    int           requestResize;
};

unsigned long SparseArrayHash(const void* data, unsigned long dataSize,
    unsigned long& hash, unsigned long& step, unsigned long tableSize);

template <class K>
struct zTSparseIndexOps {
    static void Destroy(K* value)
    {
        value->~K();
    }
};

template <>
struct zTSparseIndexOps<const void*> {
    static void Destroy(const void**)
    {
    }
};

template <>
struct zTSparseIndexOps<void*> {
    static void Destroy(void**)
    {
    }
};

template <class K, class D>
class zCSparseArray : public zCSparseArrayBase {
public:

    zCSparseArray(unsigned long minSize = 0)
        : zCSparseArrayBase(minSize)
    {
        if (minSize)
            Create(minSize);
    }

    virtual ~zCSparseArray()
    {
        Destroy();
    }

    virtual unsigned long SizeIndex() const
    {
        return sizeof(K);
    }

    virtual unsigned long SizeData() const
    {
        return sizeof(D);
    }

    virtual unsigned long SizeAll() const
    {
        return SizeIndex() + SizeData() + sizeof(zCSparseArrayManDat);
    }

    virtual void* IndexPtr(unsigned long position) const
    {
        return static_cast<unsigned char*>(table) + position * SizeAll();
    }

    virtual void* DataPtr(unsigned long position) const
    {
        return static_cast<unsigned char*>(IndexPtr(position)) + SizeIndex();
    }

    virtual zCSparseArrayManDat* ManDatPtr(unsigned long position) const
    {
        return reinterpret_cast<zCSparseArrayManDat*>(
            static_cast<unsigned char*>(DataPtr(position)) + SizeData());
    }

    virtual int IndexIsEqual(const void* first, const void* second) const
    {
        return *static_cast<const K*>(first) == *static_cast<const K*>(second);
    }

    virtual void CopyIndex(void* destination, const void* source) const
    {
        if (destination)
            *static_cast<K*>(destination) = *static_cast<const K*>(source);
    }

    virtual void DestructIndex(void* index) const
    {
        zTSparseIndexOps<K>::Destroy(static_cast<K*>(index));
    }

    virtual void Hash(const void* index, unsigned long& hash, unsigned long& step) const
    {
        SparseArrayHash(index, sizeof(K), hash, step, size);
    }

    virtual void ConstructData(void* data) const
    {
        new(data) D;
    }

    virtual void DestructData(void* data) const
    {
        static_cast<D*>(data)->~D();
    }
};

unsigned long SparseArrayHash(const void* data, unsigned long dataSize,
    unsigned long& hash, unsigned long& step, unsigned long tableSize);

class zCSparseArrayIterator {
public:

    zCSparseArrayIterator(zCSparseArrayBase& array) : array(&array), registered(0) { Reset(); }
    ~zCSparseArrayIterator();
    void Reset();
    void Next();
    void Register();
    void DeRegister();
    operator int();

    int                count;
    zCSparseArrayBase* array;
    int                registered;
};

class zCCacheBase {
public:

    zCCacheBase();
    virtual ~zCCacheBase();

    virtual void PurgeCache() = 0;
    virtual void ClearCache() = 0;

    static void S_PurgeCaches();
    static void S_ClearCaches();

    int          timeout;
    zCCacheBase* next;
    zCCacheBase** anchor;

    static int          s_currentFrame;
    static zCCacheBase* s_anchor;
    static zCCacheBase* s_run;
};

#endif
