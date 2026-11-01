#include "include/oMobMsg.h"
#include "include/oGame.h"

static zCClassDef oCMobMsgClassDef("oCMobMsg", "oCEventMessage", oCMobMsg::_CreateNewInstance,
    0, 0x38, 0, "");
zCClassDef* oCMobMsg::classDef = &oCMobMsgClassDef;

oCMobMsg::oCMobMsg()
    : npc(0), from(0), to(0), playAni(0)
{
    subType = EV_STARTINTERACTION;
}

oCMobMsg::oCMobMsg(TMobMsgSubType type, oCNpc* npc)
    : npc(npc), from(0), to(0), playAni(0)
{
    subType = type;
}

oCMobMsg::oCMobMsg(TMobMsgSubType type, oCNpc* npc, int playAni)
    : npc(npc), from(0), to(0), playAni(playAni)
{
    subType = type;
}

oCMobMsg::~oCMobMsg()
{
}

zCObject* oCMobMsg::_CreateNewInstance()
{
    oCMobMsg* message = new oCMobMsg;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

zCClassDef* oCMobMsg::_GetClassDef() const
{
    return classDef;
}

int oCMobMsg::IsNetRelevant()
{
    return 1;
}

void oCMobMsg::Pack(zCBuffer& buffer, zCEventManager* manager)
{
    int hasNpc = npc != 0;
    buffer.Write(&hasNpc, sizeof(hasNpc));
    buffer.Write(&from, sizeof(from));
    int state = to;
    buffer.Write(&state, sizeof(state));
    int animate = playAni;
    buffer.Write(&animate, sizeof(animate));
}

void oCMobMsg::Unpack(zCBuffer& buffer, zCEventManager* manager)
{
    unsigned long vobID = 0;
    buffer.Read(&vobID, sizeof(vobID));
    npc = static_cast<oCNpc*>(ogame->GetWorld()->SearchVobByID(vobID, 0));
    buffer.Read(&from, sizeof(from));
    int state;
    buffer.Read(&state, sizeof(state));
    to = state;
    int animate;
    buffer.Read(&animate, sizeof(animate));
    playAni = animate;
}

int oCMobMsg::MD_GetNumOfSubTypes()
{
    return 5;
}

zSTRING oCMobMsg::MD_GetSubTypeString(int type)
{
    switch (type)
    {
    case EV_STARTINTERACTION:
        return zSTRING("EV_STARTINTERACTION");
    case EV_STARTSTATECHANGE:
        return zSTRING("EV_STARTSTATECHANGE");
    case EV_ENDINTERACTION:
        return zSTRING("EV_ENDINTERACTION");
    case EV_UNLOCK:
        return zSTRING("EV_UNLOCK");
    case EV_LOCK:
        return zSTRING("EV_LOCK");
    case EV_CALLSCRIPT:
        return zSTRING("EV_CALLSCRIPT");
    }
    return zSTRING();
}

void oCMobMsg::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}
