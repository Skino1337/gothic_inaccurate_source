#include "include/zCsMessages.h"

static zCClassDef zCEvMsgCutsceneClassDef("zCEvMsgCutscene", "zCEventMessage",
    zCEvMsgCutscene::_CreateNewInstance, 0, 0x50, 0, "");
zCClassDef* zCEvMsgCutscene::classDef = &zCEvMsgCutsceneClassDef;

zCEvMsgCutscene::zCEvMsgCutscene()
    : isOutputUnit(0), isGlobalCutscene(0), isMainRole(0), deleted(0)
{
}

zCEvMsgCutscene::zCEvMsgCutscene(TCutsceneSubType eventSubType)
    : isOutputUnit(0), isGlobalCutscene(0), isMainRole(0), deleted(0)
{
    subType = eventSubType;
}

zCEvMsgCutscene::~zCEvMsgCutscene()
{
}

zCObject* zCEvMsgCutscene::_CreateNewInstance()
{
    zCEvMsgCutscene* message = new zCEvMsgCutscene;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

void zCEvMsgCutscene::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCEvMsgCutscene::_GetClassDef() const
{
    return classDef;
}

void zCEvMsgCutscene::SetCutsceneName(zSTRING name)
{
    csName = name;
    csName.Upper();
}

int zCEvMsgCutscene::IsHighPriority()
{
    return 1;
}

void zCEvMsgCutscene::Delete()
{
    deleted = 1;
}

int zCEvMsgCutscene::IsDeleted()
{
    return deleted;
}

int zCEvMsgCutscene::MD_GetNumOfSubTypes()
{
    return 4;
}

zSTRING zCEvMsgCutscene::MD_GetSubTypeString(int eventSubType)
{
    static const char* names[] = { "EV_CS_STARTPLAY", "EV_CS_STOP", "EV_CS_INTERRUPT", "EV_CS_RESUME" };
    if (eventSubType < 0 || eventSubType >= 4)
        return zSTRING("");
    return zSTRING(names[eventSubType]);
}
