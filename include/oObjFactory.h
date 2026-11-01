#ifndef OOBJFACTORY_H
#define OOBJFACTORY_H

#include "oCollisionObjectMiscChar.h"

class oCCSManager : public zCCSManager {
public:

    oCCSManager();
    virtual ~oCCSManager();
    virtual zCClassDef* _GetClassDef() const;
    virtual zCEventMessage* CreateOuMessage();
    virtual zCEventMessage* CreateMessage(int command);
    virtual zCCSPlayer* CreateCutscenePlayer(zCWorld* world);
    virtual zCCSProps* CreateProperties();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);

    static zCClassDef* classDef;
};

class zCWorldInfo;
class zCNetHost;

class zCPlayerInfo {
public:

    zCPlayerInfo();
    virtual ~zCPlayerInfo();
    zCPlayerInfo* CreateDistinctPlayer();
    static zCPlayerInfo* GetActivePlayer();
    static zCPlayerInfo* CreateFromBuffer(zCBuffer& buffer);
    void PackToBuffer(zCBuffer& buffer) { Pack(buffer); }
    virtual void Init(const zSTRING& name, const zSTRING& world);
    virtual void Init(const zSTRING& name, void* handle);
    virtual void Reset();
    virtual void SetName(const zSTRING& value);
    virtual void SetPlayerVob(zCVob* vob);
    virtual void Deactivate(int value);
    virtual int IsDeactivated() const;
    virtual void SetReady(int value);
    virtual int IsReady() const;
    virtual void Pack(zCBuffer& buffer);
    virtual void Unpack(zCBuffer& buffer);
    void SetActive();
    void StorePlayerVobAsScript();
    zCVob* CreatePlayerVobFromScript();
    zCBuffer* GetPlayerVobScript();
    void SetPlayerVobScript(zCBuffer* buffer);
    zSTRING GetName() const { return name; }
    unsigned char GetID() const { return id; }
    int IsActive();
    unsigned long SetPingStart();
    unsigned long SetPingEnd();

    static zCArray<int> free_ids;
    static unsigned char next_id;
    static zCPlayerInfo* activePlayer;

    zCVob* hostVob;
    zCNetHost* netHost;
    zCBuffer* scriptBuffer;
    zCWorldInfo* worldInfo;
    zSTRING name;
    unsigned long pingTime;
    unsigned long pingTimeStart;
    unsigned long pingLost;
    unsigned char id;
    int ready : 1;
    int deactivated : 1;
    unsigned long reserved : 22;
};

enum TPlayerInstance {
    PC_UNKNOWN,
    PC_HERO,
    PC_FIGHTER,
    PC_THIEF,
    PC_MAGE,
    PC_PSIONIC
};

class oCPlayerInfo : public zCPlayerInfo {
public:

    oCPlayerInfo();
    virtual ~oCPlayerInfo() {}
    zSTRING GetInstanceName() const;
    TPlayerInstance GetInstance() const;
    virtual void Init(const zSTRING& name, const zSTRING& world);
    virtual void Reset();
    virtual void Pack(zCBuffer& buffer);
    virtual void Unpack(zCBuffer& buffer);
    virtual void SetInstance(TPlayerInstance value);
    virtual void SetInstanceByName(const zSTRING& name);

    TPlayerInstance instance;
};

class oCWaypoint : public zCWaypoint {
public:

    oCWaypoint() {}

    static zCClassDef* classDef;
};

class oCWay : public zCWay {
public:

    oCWay() {}
};

class oCObjectFactory : public zCObjectFactory {
public:

    oCObjectFactory() {}
    virtual ~oCObjectFactory() {}
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual zCEventManager* CreateEventManager(zCVob* vob);
    virtual zCCSManager* CreateCSManager();
    virtual oCGameInfo* CreateGameInfo();
    virtual oCPlayerInfo* CreatePlayerInfo();
    virtual zCWorld* CreateWorld();
    virtual zCWaypoint* CreateWaypoint();
    virtual zCWay* CreateWay();
    virtual oCNpc* CreateNpc(int instance);
    virtual oCItem* CreateItem(int instance);

    static zCClassDef* classDef;
};

#endif
