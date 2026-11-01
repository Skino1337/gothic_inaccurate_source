#ifndef ZNETEVENTMAN_H
#define ZNETEVENTMAN_H

#include "zVob.h"

class zCNetMessage;
class zCWorld;

class zCNetEventManager : public zCEventManager {
public:

    zCNetEventManager();
    zCNetEventManager(zCVob* vob);
    virtual ~zCNetEventManager();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    static int HandleNetMessage(zCNetMessage* message, unsigned short mode, zCWorld* world);
    void Init();
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual zCNetVobControl* GetNetVobControl(int create);
    virtual void OnMessage(zCEventMessage* message, zCVob* sender);

    zCNetVobControl* netVobCtrl;

    static zCClassDef* classDef;
};

#endif
