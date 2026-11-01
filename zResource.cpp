#include "include/zResource.h"
#include "include/zString.h"
#include <windows.h>

static int resourceState = -1;
static int resourceManagerState = -1;
static zCClassDef zCResourceClassDef("zCResource", "zCObject", NULL, 0, 0x54, 0, "");
zCResourceManager* zresMan;
zCClassDef* zCResource::classDef = &zCResourceClassDef;

zCResource::zCResource()
    : stateChangeGuard(0), nextRes(0), prevRes(0), timeStamp(0),
      cacheState(zRES_CACHED_OUT), cacheOutLock(0), cacheClassIndex(0xff),
      managedByResMan(1), cacheInPriority(0), canBeCachedOut(1)
{
}

zCResource::~zCResource()
{
    nextRes = 0;
    prevRes = 0;
}

zCClassDef* zCResource::_GetClassDef() const
{
    return classDef;
}

void zCResource::SetCacheInPriority(float priority)
{
    cacheInPriority = static_cast<unsigned short>(priority * 65535.0f);
}

int zCResource::LoadResourceData()
{
    return 0;
}

int zCResource::ReleaseResourceData()
{
    return 0;
}

unsigned long zCResource::GetResSizeBytes()
{
    return 0;
}

void zCResource::GetCacheConfig(unsigned long&, unsigned long&)
{
}

void zCResource::CacheOut()
{
    if (!cacheOutLock)
        zresMan->CacheOut(this);
}

void zCResource::TouchTimeStamp()
{
    if (timeStamp != zresMan->currentTimeStamp) {
        timeStamp = zresMan->currentTimeStamp;
        zresMan->TouchTimeStamp(this);
    }
}

void zCResource::TouchTimeStampLocal()
{
    timeStamp = zresMan->currentTimeStamp;
}

void zCResource::LockStateChange()
{
    stateChangeGuard = 1;
}

void zCResource::UnlockStateChange()
{
    stateChangeGuard = 0;
}

zTResourceCacheState zCResource::CacheIn(float priority)
{
    if (cacheState == zRES_CACHED_IN)
        TouchTimeStamp();
    else {
        TouchTimeStampLocal();
        zresMan->CacheIn(this, priority);
    }
    return static_cast<zTResourceCacheState>(cacheState);
}

zCThread::zCThread()
    : threadHandle(0), threadID(0), isThreadRunning(0), terminationRequested(0), suspendCount(0)
{
}

zCThread::~zCThread()
{
    if (!threadHandle)
        return;
    terminationRequested = 1;
    ResumeThread();
    WaitForSingleObject(threadHandle, INFINITE);
    CloseHandle(threadHandle);
    threadHandle = 0;
    threadID = 0;
    isThreadRunning = 0;
    terminationRequested = 0;
}

int zCThread::IsThreadRunning()
{
    return isThreadRunning;
}

int zCThread::GetTerminationRequested() const
{
    return terminationRequested;
}

void zCThread::SetTerminationRequested(int requested)
{
    terminationRequested = requested;
}

zCResourceManager::zCResourceManager()
    : cacheInQueueHead(0), cacheInQueueTail(0), currentTimeStamp(0), threadingEnabled(1),
      cacheInQueueGuard(0), numQueued(0), numResCachedInThisFrame(0), showDebugInfo(0),
      cacheInImmediatelyMsec(0), goToSuspend(0)
{
    InitClassCache();
}

zCResourceManager::~zCResourceManager()
{
    PurgeCaches(0);
}

void zCResourceManager::PurgeCaches(zCClassDef* classDef)
{
    goToSuspend = 1;
    for (int i = 0; i < classCacheList.numInArray; ++i) {
        zCClassCache& cache = classCacheList.parray[i];
        if (classDef && cache.resClassDef != classDef)
            continue;
        zCResource* resource = cache.resListHead;
        while (resource) {
            zCResource* next = resource->nextRes;
            if (!resource->cacheOutLock)
                CacheOut(resource);
            resource = next;
        }
    }
    numQueued = 0;
    cacheInQueueHead = 0;
    cacheInQueueTail = 0;
}

void zCResourceManager::SetThreadingEnabled(int enabled)
{
    threadingEnabled = enabled;
}

int zCResourceManager::GetThreadingEnabled() const
{
    return threadingEnabled;
}

void zCResourceManager::InitClassCache()
{
    classCacheList.DeleteList();
    if (!zCClassDef::classDefList)
        return;
    for (int i = 0; i < zCClassDef::classDefList->numInArray; ++i) {
        zCClassDef* classDef = zCClassDef::classDefList->array[i];
        if (!(classDef->classFlags & 4))
            continue;
        zCClassCache cache;
        cache.resClassDef = classDef;
        cache.resListHead = 0;
        cache.resListTail = 0;
        cache.numRes = 0;
        cache.cacheSizeBytes = 0;
        cache.cacheSizeMaxBytes = 0;
        cache.cacheOutTimeMSec = 60000;
        classCacheList.InsertEnd(cache);
    }
}

zCResourceManager::zCClassCache* zCResourceManager::GetClassCache(zCResource* resource)
{
    if (resource->cacheClassIndex == 0xff)
        resource->cacheClassIndex = GetClassCacheIndex(resource->_GetClassDef());
    return &classCacheList.parray[resource->cacheClassIndex];
}

int zCResourceManager::GetClassCacheIndex(zCClassDef* classDef)
{
    for (int i = 0; i < classCacheList.numInArray; ++i) {
        if (classCacheList.parray[i].resClassDef == classDef)
            return i;
    }
    return -1;
}

zCResourceManager::zCClassCache::zCClassCache()
    : resClassDef(0), resListHead(0), resListTail(0), numRes(0),
      cacheSizeBytes(0), cacheSizeMaxBytes(0), cacheOutTimeMSec(0)
{
}

zCResourceManager::zCClassCache::~zCClassCache()
{
}

void zCResourceManager::zCClassCache::InsertRes(zCResource* resource)
{
    if (cacheOutTimeMSec <= 0)
        resource->GetCacheConfig(*reinterpret_cast<unsigned long*>(&cacheOutTimeMSec),
            *reinterpret_cast<unsigned long*>(&cacheSizeMaxBytes));
    resource->prevRes = 0;
    resource->nextRes = resListHead;
    if (resListHead)
        resListHead->prevRes = resource;
    else
        resListTail = resource;
    resListHead = resource;
    ++numRes;
    cacheSizeBytes += resource->GetResSizeBytes();
}

void zCResourceManager::zCClassCache::RemoveRes(zCResource* resource)
{
    if (resource->prevRes)
        resource->prevRes->nextRes = resource->nextRes;
    else
        resListHead = resource->nextRes;
    if (resource->nextRes)
        resource->nextRes->prevRes = resource->prevRes;
    else
        resListTail = resource->prevRes;
    resource->nextRes = 0;
    resource->prevRes = 0;
    --numRes;
    cacheSizeBytes -= resource->GetResSizeBytes();
    if (cacheSizeBytes < 0)
        cacheSizeBytes = 0;
}

void zCResourceManager::zCClassCache::TouchRes(zCResource* resource)
{
    if (!resource->prevRes || resource->prevRes->timeStamp >= resource->timeStamp ||
        resource == resListHead)
        return;
    RemoveRes(resource);
    InsertRes(resource);
}

void zCResourceManager::LoadResource(zCResource* resource)
{
    if (resource->cacheState == zRES_CACHED_IN || resource->cacheState == zRES_LOADING)
        return;
    resource->cacheState = zRES_LOADING;
    resource->LoadResourceData();
    resource->cacheState = zRES_CACHED_IN;
    GetClassCache(resource)->InsertRes(resource);
    ++numResCachedInThisFrame;
}

zTResourceCacheState zCResourceManager::CacheIn(zCResource* resource, float priority)
{
    if (!resource->GetObjectName().Length())
        return zRES_FAILURE;
    if (resource->cacheState == zRES_CACHED_OUT) {
        resource->managedByResMan = 1;
        resource->AddRef();
        resource->SetCacheInPriority(priority);
        if (priority < 0)
            LoadResource(resource);
        else
            InsertCacheInQueue(resource, priority);
    } else if (resource->cacheState != zRES_CACHED_IN && priority < 0)
        LoadResource(resource);
    return static_cast<zTResourceCacheState>(resource->cacheState);
}

void zCResourceManager::CacheOut(zCResource* resource)
{
    if (resource->cacheState == zRES_CACHED_OUT)
        return;
    if (resource->cacheState == zRES_QUEUED)
        RemoveCacheInQueue(resource);
    else if (resource->cacheState == zRES_CACHED_IN) {
        GetClassCache(resource)->RemoveRes(resource);
        resource->ReleaseResourceData();
    }
    resource->cacheState = zRES_CACHED_OUT;
    if (resource->managedByResMan)
        resource->Release();
}

void zCResourceManager::TouchTimeStamp(zCResource* resource)
{
    GetClassCache(resource)->TouchRes(resource);
}

void zCResourceManager::DoFrameActivity()
{
    if (cacheInImmediatelyMsec > 0) {
        cacheInImmediatelyMsec -= 1.0f;
        if (cacheInImmediatelyMsec < 0)
            cacheInImmediatelyMsec = 0;
    }
    ++currentTimeStamp;
    if (!threadingEnabled && cacheInQueueHead)
        ThreadProc();
    Evict();
    if (showDebugInfo)
        PrintStatusDebug();
    numResCachedInThisFrame = 0;
}

void zCResourceManager::Evict()
{
    for (int i = 0; i < classCacheList.numInArray; ++i) {
        zCClassCache& cache = classCacheList.parray[i];
        zCResource* resource = cache.resListTail;
        if (!resource || !resource->canBeCachedOut || resource->cacheOutLock)
            continue;
        if (currentTimeStamp > resource->timeStamp + cache.cacheOutTimeMSec)
            CacheOut(resource);
    }
}

unsigned long zCResourceManager::ThreadProc()
{
    while (!terminationRequested) {
        zCResource* resource = RemoveCacheInQueue(0);
        if (!resource)
            break;
        resource->LockStateChange();
        LoadResource(resource);
        resource->UnlockStateChange();
        if (!threadingEnabled && numResCachedInThisFrame >= 99999)
            break;
    }
    return 0;
}

zCResource* zCResourceManager::RemoveCacheInQueue(zCResource* resource)
{
    LockCacheInQueue();
    if (!resource)
        resource = cacheInQueueHead;
    if (resource) {
        if (resource->prevRes)
            resource->prevRes->nextRes = resource->nextRes;
        else
            cacheInQueueHead = resource->nextRes;
        if (resource->nextRes)
            resource->nextRes->prevRes = resource->prevRes;
        else
            cacheInQueueTail = resource->prevRes;
        resource->nextRes = 0;
        resource->prevRes = 0;
        --numQueued;
    }
    UnlockCacheInQueue();
    return resource;
}

zCResource* zCResourceManager::GetNextCacheInQueue()
{
    return RemoveCacheInQueue(0);
}

void zCResourceManager::InsertCacheInQueue(zCResource* resource, float priority)
{
    resource->SetCacheInPriority(priority);
    LockCacheInQueue();
    zCResource* next = cacheInQueueTail;
    zCResource* previous = 0;
    while (next && next->cacheInPriority > resource->cacheInPriority) {
        previous = next;
        next = next->prevRes;
    }
    resource->prevRes = next;
    resource->nextRes = previous;
    if (previous)
        previous->prevRes = resource;
    else
        cacheInQueueTail = resource;
    if (next)
        next->nextRes = resource;
    else
        cacheInQueueHead = resource;
    ++numQueued;
    resource->cacheState = zRES_QUEUED;
    UnlockCacheInQueue();
    if (!threadingEnabled)
        ThreadProc();
}

void zCResourceManager::LockCacheInQueue()
{
    if (threadingEnabled)
        cacheInQueueGuard = 1;
}

void zCResourceManager::UnlockCacheInQueue()
{
    if (threadingEnabled)
        cacheInQueueGuard = 0;
}

int zCResourceManager::WaitForCacheIn(zCResource* resource)
{
    while (resource->cacheState == zRES_QUEUED || resource->cacheState == zRES_LOADING)
        ThreadProc();
    return resource->cacheState == zRES_CACHED_IN;
}

void zCResourceManager::QueueProcess_Start()
{
    goToSuspend = 0;
    isThreadRunning = 1;
}

void zCResourceManager::QueueProcess_Suspend()
{
    goToSuspend = 1;
    ++suspendCount;
}

void zCResourceManager::QueueProcess_Resume()
{
    goToSuspend = 0;
    if (suspendCount > 0)
        --suspendCount;
}

void zCResourceManager::PrintStatusDebug()
{
    zSTRING status;
    status.Sprintf("RESMAN: queued %d, classes %d, cached this frame %d",
        numQueued, classCacheList.numInArray, numResCachedInThisFrame);
}
