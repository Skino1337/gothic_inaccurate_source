#ifndef ZRESOURCE_H
#define ZRESOURCE_H

#include "zObject.h"

enum zTResourceCacheState {
    zRES_FAILURE    = -1,
    zRES_CACHED_OUT = 0,
    zRES_QUEUED     = 1,
    zRES_LOADING    = 2,
    zRES_CACHED_IN  = 3
};

class zCResource;

class zCThread {
public:

    zCThread();
    virtual ~zCThread();

    virtual int  IsThreadRunning();
    virtual int  GetTerminationRequested() const;
    virtual void SetTerminationRequested(int requested);
    virtual void BeginThread();
    virtual void EndThread();
    virtual int  SuspendThread();
    virtual int  ResumeThread();
    virtual unsigned long ThreadProc();
    void         SleepThread(unsigned long milliseconds);

    void*        threadHandle;
    unsigned int threadID;
    int          isThreadRunning;
    int          terminationRequested;
    int          suspendCount;
};

class zCResourceManager : public zCThread {
public:

    class zCClassCache {
    public:

        zCClassCache();
        ~zCClassCache();

        void InsertRes(zCResource* resource);
        void RemoveRes(zCResource* resource);
        void TouchRes(zCResource* resource);

        zCClassDef*  resClassDef;
        zCResource*  resListHead;
        zCResource*  resListTail;
        unsigned long numRes;
        int          cacheSizeBytes;
        int          cacheSizeMaxBytes;
        unsigned long cacheOutTimeMSec;
    };

    zCResourceManager();
    virtual ~zCResourceManager();

    void                 PurgeCaches(zCClassDef* classDef);
    void                 SetThreadingEnabled(int enabled);
    int                  GetThreadingEnabled() const;
    void                 InitClassCache();
    zCClassCache*        GetClassCache(zCResource* resource);
    int                  GetClassCacheIndex(zCClassDef* classDef);
    void                 LoadResource(zCResource* resource);
    zTResourceCacheState CacheIn(zCResource* resource, float priority);
    void                 CacheOut(zCResource* resource);
    void                 TouchTimeStamp(zCResource* resource);
    void                 DoFrameActivity();
    void                 Evict();
    virtual unsigned long ThreadProc();
    zCResource*          RemoveCacheInQueue(zCResource* resource);
    zCResource*          GetNextCacheInQueue();
    void                 InsertCacheInQueue(zCResource* resource, float priority);
    void                 LockCacheInQueue();
    void                 UnlockCacheInQueue();
    int                  WaitForCacheIn(zCResource* resource);
    void                 QueueProcess_Start();
    void                 QueueProcess_Suspend();
    void                 QueueProcess_Resume();
    void                 PrintStatusDebug();
    void                 SetShowDebugInfo(int show);
    int                  GetShowDebugInfo() const;

    zCArray<zCClassCache> classCacheList;
    zCResource*          cacheInQueueHead;
    zCResource*          cacheInQueueTail;
    unsigned long        currentTimeStamp;
    int                  threadingEnabled;
    int                  cacheInQueueGuard;
    int                  numQueued;
    int                  numResCachedInThisFrame;
    int                  showDebugInfo;
    float                cacheInImmediatelyMsec;
    int                  goToSuspend;
};

class zCResource : public zCObject {
public:

    zCResource();
    virtual ~zCResource();

    zTResourceCacheState CacheIn(float priority);
    void                 SetCacheInPriority(float priority);
    void                 CacheOut();
    void                 TouchTimeStamp();
    void                 TouchTimeStampLocal();
    void                 LockStateChange();
    void                 UnlockStateChange();
    virtual zCClassDef*  _GetClassDef() const;
    virtual int          LoadResourceData();
    virtual int          ReleaseResourceData();
    virtual unsigned long GetResSizeBytes();
    virtual void         GetCacheConfig(unsigned long& cacheSize, unsigned long& cacheOutTime);

    static zCClassDef* classDef;

    int           stateChangeGuard;
    zCResource*    nextRes;
    zCResource*    prevRes;
    unsigned long  timeStamp;
    unsigned char  cacheState       : 2;
    unsigned char  cacheOutLock     : 1;
    unsigned char  cacheClassIndex  : 8;
    unsigned char  managedByResMan  : 1;
    unsigned short cacheInPriority  : 16;
    unsigned char  canBeCachedOut   : 1;
};

extern zCResourceManager* zresMan;

#endif
