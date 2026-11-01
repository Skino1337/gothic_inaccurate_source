#ifndef ZCACHE_H
#define ZCACHE_H

#include "zSparseArray.h"

template <class K, class D>
class zCCacheData {
public:

    zCCacheData()
        : data(0), lastFrame(0)
    {
    }

    void Clear()
    {
        delete data;
        data = 0;
    }

    D* GetData(const K&)
    {
        if (!data)
            data = new D(key);
        lastFrame = zCCacheBase::s_currentFrame;
        return data;
    }

    D*  data;
    int lastFrame;
};

template <class K, class D>
class zCCache : public zCCacheBase {
public:

    zCCache(unsigned long minSize = 0, unsigned long timeout = 0)
        : entries(minSize)
    {
        this->timeout = timeout;
    }

    virtual ~zCCache()
    {
        ClearCache();
    }

    virtual void ClearCache()
    {
        for (int i = entries.size - 1; i >= 0; --i)
        {
            if (!entries.ManDatPtr(i)->occupied)
                continue;
            zCCacheData<K, D>* entry = static_cast<zCCacheData<K, D>*>(entries.DataPtr(i));
            delete entry->data;
            entry->data = 0;
            entries.Delete(entries.IndexPtr(i));
        }
    }

    virtual void PurgeCache()
    {
        for (int i = entries.size - 1; i >= 0; --i)
        {
            if (!entries.ManDatPtr(i)->occupied)
                continue;
            zCCacheData<K, D>* entry = static_cast<zCCacheData<K, D>*>(entries.DataPtr(i));
            if (zCCacheBase::s_currentFrame - entry->lastFrame <= timeout)
                continue;
            delete entry->data;
            entry->data = 0;
            entries.Delete(entries.IndexPtr(i));
        }
    }

    D* operator[](const K& key)
    {
        zCCacheData<K, D>* entry = static_cast<zCCacheData<K, D>*>(entries.Insert(&key));
        return entry->GetData(key);
    }

    D* GetDataDangerous(const K& key)
    {
        zCCacheData<K, D>* entry = static_cast<zCCacheData<K, D>*>(entries.Element(&key));
        return entry ? entry->data : 0;
    }

    void Delete(const K& key)
    {
        zCCacheData<K, D>* entry = static_cast<zCCacheData<K, D>*>(entries.Element(&key));
        if (entry) {
            delete entry->data;
            entry->data = 0;
            entries.Delete(&key);
        }
    }

    zCSparseArray<K, zCCacheData<K, D> > entries;
};

template <class K, class D>
class zCCacheAsk : public zCCache<K, D> {
public:

    zCCacheAsk(unsigned long minSize = 0, unsigned long timeout = 0)
        : zCCache<K, D>(minSize, timeout)
    {
    }

    virtual void PurgeCache()
    {
        for (int i = this->entries.size - 1; i >= 0; --i)
        {
            if (!this->entries.ManDatPtr(i)->occupied)
                continue;
            zCCacheData<K, D>* entry = static_cast<zCCacheData<K, D>*>(this->entries.DataPtr(i));
            if (entry->data->modified)
                continue;
            if (zCCacheBase::s_currentFrame - entry->lastFrame <= this->timeout)
                continue;
            delete entry->data;
            entry->data = 0;
            this->entries.Delete(this->entries.IndexPtr(i));
        }
    }
};

template <class T>
class zCMemPool {
public:

    zCMemPool(unsigned int numElements = 0, unsigned int = 0)
        : memory(new unsigned char[numElements * sizeof(T)])
    {
    }

    ~zCMemPool()
    {
        delete[] memory;
    }

    unsigned char* memory;
};

#endif
