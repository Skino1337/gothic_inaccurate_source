#include "include/zSparseArray.h"

static int SparseIsPrime(unsigned long value)
{
    if (value < 2)
        return 0;
    for (unsigned long divisor = 2; divisor * divisor <= value; ++divisor) {
        if (value % divisor == 0)
            return 0;
    }
    return 1;
}

static unsigned long SparseNextPrime(unsigned long value)
{
    while (!SparseIsPrime(value))
        ++value;
    return value;
}

zCSparseArrayBase::zCSparseArrayBase(unsigned long minSize)
    : numberOfIterators(0), size(0), min_size(minSize), table(0), used(0),
      maxRefCtr(10), requestResize(0)
{
    if (!minSize)
        return;
    size = SparseNextPrime(minSize);
    table = new char[SizeAll() * size];
    for (unsigned long i = 0; i < size; ++i) {
        ManDatPtr(i)->refCtr = 0;
        ManDatPtr(i)->occupied = 0;
    }
}

unsigned long zCSparseArrayBase::SizeAll() const
{
    return SizeIndex() + SizeData() + sizeof(zCSparseArrayManDat);
}

void* zCSparseArrayBase::IndexPtr(unsigned long position) const
{
    return static_cast<char*>(table) + SizeAll() * position;
}

void* zCSparseArrayBase::DataPtr(unsigned long position) const
{
    return static_cast<char*>(IndexPtr(position)) + SizeIndex();
}

zCSparseArrayManDat* zCSparseArrayBase::ManDatPtr(unsigned long position) const
{
    return reinterpret_cast<zCSparseArrayManDat*>(static_cast<char*>(DataPtr(position)) + SizeData());
}

void zCSparseArrayBase::DestructIndex(void*)
{
}

void zCSparseArrayBase::ConstructData(void*)
{
}

void zCSparseArrayBase::DestructData(void*)
{
}

zCSparseArrayBase::~zCSparseArrayBase()
{
    Destroy();
}

void zCSparseArrayBase::Destroy()
{
    if (table) {
        for (unsigned long i = 0; i < size; ++i) {
            if (ManDatPtr(i)->occupied) {
                DestructIndex(IndexPtr(i));
                DestructData(DataPtr(i));
            }
        }
        delete[] static_cast<char*>(table);
        table = 0;
    }
    size = 0;
    used = 0;
}

void* zCSparseArrayBase::Element(const void* index) const
{
    if (requestResize && !numberOfIterators) {
        unsigned long newSize = used * 3;
        if (newSize < min_size)
            newSize = min_size;
        const_cast<zCSparseArrayBase*>(this)->Resize(newSize);
    }
    const unsigned long position = Find(index);
    return position < size ? DataPtr(position) : 0;
}

void* zCSparseArrayBase::Insert(const void* index)
{
    unsigned long position = Find(index);
    if (position < size)
        return DataPtr(position);
    if (requestResize && !numberOfIterators) {
        unsigned long newSize = used * 3;
        if (newSize < min_size)
            newSize = min_size;
        Resize(newSize);
    }
    unsigned long hash;
    unsigned long step;
    Hash(index, hash, step);
    position = hash % size;
    if (!step)
        step = 1;
    unsigned long references = 0;
    while (ManDatPtr(position)->occupied) {
        ++ManDatPtr(position)->refCtr;
        ++references;
        position = (position + step) % size;
    }
    ManDatPtr(position)->refCtr = references;
    ManDatPtr(position)->occupied = 1;
    ++used;
    if (used * 3 + 6 > size * 2 || references > maxRefCtr) {
        requestResize = 1;
        if (references > maxRefCtr)
            maxRefCtr += maxRefCtr / 2;
    }
    CopyIndex(IndexPtr(position), index);
    ConstructData(DataPtr(position));
    return DataPtr(position);
}

void zCSparseArrayBase::Delete(const void* index)
{
    const unsigned long position = Find(index);
    if (position >= size)
        return;
    DestructIndex(IndexPtr(position));
    DestructData(DataPtr(position));
    ManDatPtr(position)->occupied = 0;
    --used;
    if ((used * 3 + 6) * 2 < size && size > min_size)
        requestResize = 1;
}

void zCSparseArrayBase::Clear()
{
    for (unsigned long i = 0; i < size; ++i) {
        zCSparseArrayManDat* data = ManDatPtr(i);
        if (data->occupied) {
            DestructIndex(IndexPtr(i));
            DestructData(DataPtr(i));
        }
        data->occupied = 0;
        data->refCtr = 0;
    }
    used = 0;
}

int zCSparseArrayBase::IndexIsEqual(const void* first, const void* second) const
{
    const unsigned char* firstBytes = static_cast<const unsigned char*>(first);
    const unsigned char* secondBytes = static_cast<const unsigned char*>(second);
    for (unsigned long i = 0; i < SizeIndex(); ++i) {
        if (firstBytes[i] != secondBytes[i])
            return 0;
    }
    return 1;
}

void zCSparseArrayBase::CopyIndex(void* destination, const void* source) const
{
    unsigned char* destinationBytes = static_cast<unsigned char*>(destination);
    const unsigned char* sourceBytes = static_cast<const unsigned char*>(source);
    for (unsigned long i = 0; i < SizeIndex(); ++i)
        destinationBytes[i] = sourceBytes[i];
}

unsigned long SparseArrayHash(const void* data, unsigned long dataSize,
    unsigned long& hash, unsigned long& step, unsigned long tableSize)
{
    const unsigned short* words = static_cast<const unsigned short*>(data);
    unsigned long hashFactor = 1;
    unsigned long stepFactor = 0;
    hash = 0;
    step = 0;
    for (unsigned long i = 0; i < dataSize / 2; ++i) {
        hash += words[i] * hashFactor;
        step += words[i] * stepFactor;
        const unsigned long carry = hash / tableSize;
        hash %= tableSize;
        step = (step + carry) % (tableSize - 1);
        const unsigned long nextHashFactor = hashFactor << 16;
        const unsigned long nextStepFactor = (stepFactor << 16) + nextHashFactor / tableSize;
        hashFactor = nextHashFactor % tableSize;
        stepFactor = nextStepFactor % (tableSize - 1);
    }
    ++step;
    return hash;
}

void zCSparseArrayBase::Hash(const void* index, unsigned long& hash, unsigned long& step) const
{
    SparseArrayHash(index, SizeIndex(), hash, step, size);
}

unsigned long zCSparseArrayBase::Find(const void* index) const
{
    if (!size)
        return 0;
    unsigned long hash;
    unsigned long step;
    Hash(index, hash, step);
    unsigned long position = hash % size;
    if (!step)
        step = 1;
    for (unsigned long count = 0; count < size; ++count) {
        zCSparseArrayManDat* data = ManDatPtr(position);
        if (data->occupied && IndexIsEqual(IndexPtr(position), index))
            return position;
        if (!data->occupied && !data->refCtr)
            break;
        position = (position + step) % size;
    }
    return size;
}

void zCSparseArrayBase::Create(unsigned long minSize)
{
    size = SparseNextPrime(minSize);
    used = 0;
    table = new char[SizeAll() * size];
    for (unsigned long i = 0; i < size; ++i) {
        ManDatPtr(i)->occupied = 0;
        ManDatPtr(i)->refCtr = 0;
    }
    requestResize = 0;
}

void zCSparseArrayBase::Resize(unsigned long minSize)
{
    if (minSize <= used + 10)
        minSize = used + 10;
    unsigned long newSize = SparseNextPrime(minSize);
    if (newSize == size)
        newSize = SparseNextPrime(newSize + 1);
    char* oldTable = static_cast<char*>(table);
    const unsigned long oldSize = size;
    table = 0;
    size = 0;
    Create(newSize);
    for (unsigned long i = 0; i < oldSize; ++i) {
        char* oldIndex = oldTable + SizeAll() * i;
        zCSparseArrayManDat* oldData = reinterpret_cast<zCSparseArrayManDat*>(
            oldIndex + SizeIndex() + SizeData());
        if (!oldData->occupied)
            continue;
        void* newData = Insert(oldIndex);
        const unsigned char* source = reinterpret_cast<const unsigned char*>(oldIndex + SizeIndex());
        unsigned char* destination = static_cast<unsigned char*>(newData);
        for (unsigned long j = 0; j < SizeData(); ++j)
            destination[j] = source[j];
        DestructIndex(oldIndex);
    }
    delete[] oldTable;
}

void* zCSparseArrayBase::DataPtr(unsigned long position, void* data) const
{
    return static_cast<char*>(data) + SizeAll() * position + SizeIndex();
}

void zCSparseArrayBase::AddRef(zCSparseArrayManDat* data) const
{
    if (!data->refCtr) {
        ++used;
        if (used * 3 + 6 > size * 2)
            requestResize = 1;
    }
    if (data->refCtr > maxRefCtr) {
        requestResize = 1;
        maxRefCtr += maxRefCtr / 2;
    }
    ++data->refCtr;
}

void zCSparseArrayBase::Release(zCSparseArrayManDat* data) const
{
    if (data->refCtr)
        --data->refCtr;
    if (data->refCtr)
        return;
    --used;
    if ((used * 3 + 6) * 2 < size && size > min_size)
        requestResize = 1;
}

void zCSparseArrayBase::CheckResize() const
{
    if (!requestResize || numberOfIterators)
        return;
    unsigned long newSize = used * 3;
    if (newSize < min_size)
        newSize = min_size;
    const_cast<zCSparseArrayBase*>(this)->Resize(newSize);
}

int zCCacheBase::s_currentFrame;
zCCacheBase* zCCacheBase::s_anchor;
zCCacheBase* zCCacheBase::s_run;

zCCacheBase::zCCacheBase()
    : timeout(100), next(s_anchor)
{
    if (s_anchor)
        s_anchor->anchor = &next;
    s_anchor = this;
    anchor = &s_anchor;
}

zCCacheBase::~zCCacheBase()
{
    if (next)
        next->anchor = anchor;
    *anchor = next;
    next = 0;
    anchor = 0;
    if (s_run == this)
        s_run = 0;
}

void zCCacheBase::S_PurgeCaches()
{
    if (!s_run)
        s_run = s_anchor;
    s_run->PurgeCache();
    s_run = s_run->next;
    ++s_currentFrame;
}

void zCCacheBase::S_ClearCaches()
{
    s_run = s_anchor;
    while (s_run) {
        s_run->ClearCache();
        s_run = s_run->next;
    }
}
