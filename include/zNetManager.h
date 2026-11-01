#ifndef ZNETMANAGER_H
#define ZNETMANAGER_H

#include "zTools.h"
#include "zNetMessage.h"

class zCWorld;

class zCNetManager : public zCObject {
public:

    zCNetManager();
    virtual ~zCNetManager();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    int Temp_HandleNetMessage(zCNetMessage*& message, zCWorld* world);
    void SendPing();
    void SendPingReply(zCPlayerInfo* player);
    void SendWorldTime();
    virtual zCClassDef* _GetClassDef() const;
    virtual void Process();
    virtual zCNetMessage* CreateNetMessage(const unsigned short& messageType, zCObject* object);
    virtual int HandleNetMessage(zCNetMessage* message, const unsigned short& messageType,
        zCClassDef* classDef, zCWorld* world);
    virtual void ProcessMessages();

    zCTimer timer;
    unsigned long timer_count_ping;
    unsigned long timer_count_worldtime;

    static zCClassDef* classDef;
};

extern zCNetManager* znetman;

#endif
