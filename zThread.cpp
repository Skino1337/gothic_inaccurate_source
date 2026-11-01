#include "include/zResource.h"
#include "include/zThread.h"
#include <process.h>
#include <windows.h>

unsigned long __stdcall zThreadProc(void* data)
{
    return static_cast<zCThread*>(data)->ThreadProc();
}

void zCThread::BeginThread()
{
    if (threadHandle)
        EndThread();
    threadHandle = reinterpret_cast<void*>(_beginthreadex(
        nullptr, 0, zThreadProc, this, THREAD_CREATE_RUN_IMMEDIATELY, &threadID));
    isThreadRunning = 1;
}

unsigned long zCThread::ThreadProc()
{
    return 0;
}

void zCThread::EndThread()
{
    if (!threadHandle)
        return;
    terminationRequested = 1;
    ResumeThread();
    threadHandle = 0;
    threadID = 0;
    isThreadRunning = 0;
    terminationRequested = 0;
}

int zCThread::SuspendThread()
{
    if (suspendCount > 0)
        return 0;
    ++suspendCount;
    return threadHandle != 0;
}

int zCThread::ResumeThread()
{
    if (suspendCount <= 0)
        return 0;
    --suspendCount;
    return threadHandle != 0;
}

void zCThread::SleepThread(unsigned long)
{
}

zCSyncObject::~zCSyncObject()
{
}

zCCriticalSection::zCCriticalSection()
    : criticalSection()
{
}

zCCriticalSection::~zCCriticalSection()
{
}

int zCCriticalSection::Lock(unsigned long)
{
    criticalSection[0] = 1;
    return 1;
}

int zCCriticalSection::Unlock()
{
    criticalSection[0] = 0;
    return 1;
}

zCMutex::zCMutex()
    : mutex(this)
{
}

zCMutex::~zCMutex()
{
    mutex = 0;
}

int zCMutex::Lock(unsigned long)
{
    return 1;
}

int zCMutex::Unlock()
{
    return 1;
}
