#ifndef OMOBMSG_H
#define OMOBMSG_H

#include "zVob.h"

class oCNpc;

class oCMobMsg : public zCEventMessage {
public:

    enum TMobMsgSubType {
        EV_STARTINTERACTION,
        EV_STARTSTATECHANGE,
        EV_ENDINTERACTION,
        EV_UNLOCK,
        EV_LOCK,
        EV_CALLSCRIPT
    };

    oCMobMsg();
    oCMobMsg(TMobMsgSubType type, oCNpc* npc);
    oCMobMsg(TMobMsgSubType type, oCNpc* npc, int playAni);
    virtual ~oCMobMsg();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);

    virtual zCClassDef* _GetClassDef() const;
    virtual int         IsNetRelevant();
    virtual void        Pack(zCBuffer& buffer, zCEventManager* manager);
    virtual void        Unpack(zCBuffer& buffer, zCEventManager* manager);
    virtual int         MD_GetNumOfSubTypes();
    virtual zSTRING     MD_GetSubTypeString(int type);

    oCNpc* npc;
    int    from;
    int    to : 31;
    int    playAni : 1;

    static zCClassDef* classDef;
};

#endif
