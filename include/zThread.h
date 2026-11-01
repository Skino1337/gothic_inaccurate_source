#ifndef ZTHREAD_H
#define ZTHREAD_H

class zCSyncObject {
public:

    virtual ~zCSyncObject();
    virtual int  Lock(unsigned long timeout) = 0;
    virtual int  Unlock() = 0;
};

class zCCriticalSection : public zCSyncObject {
public:

    zCCriticalSection();
    virtual ~zCCriticalSection();

    int Lock(unsigned long timeout);
    int Unlock();

    unsigned long criticalSection[6];
};

class zCMutex : public zCSyncObject {
public:

    zCMutex();
    virtual ~zCMutex();

    int Lock(unsigned long timeout);
    int Unlock();

    void* mutex;
};

#endif
