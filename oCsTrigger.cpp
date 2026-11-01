#include "include/oCsTrigger.h"

static int classDefState = -1;
static zCClassDef oCCSTriggerClassDef("oCCSTrigger", "zCTrigger",
    oCCSTrigger::_CreateNewInstance, 0, 0x168, 0, "");
zCClassDef* oCCSTrigger::classDef = &oCCSTriggerClassDef;

oCCSTrigger::~oCCSTrigger()
{
}

void oCCSTrigger::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* oCCSTrigger::_GetClassDef() const
{
    return classDef;
}

void oCCSTrigger::Archive(zCArchiver& archiver)
{
    zCTrigger::Archive(archiver);
}

void oCCSTrigger::Unarchive(zCArchiver& archiver)
{
    zCTrigger::Unarchive(archiver);
}

void oCCSTrigger::TriggerTarget(zCVob* other)
{
    if (zerr->filter_level >= 8) {
        const zSTRING otherName = other ? other->GetObjectName() : zSTRING("");
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: CSTrigger/Trigger " + GetObjectName() + " by " + otherName,
            8, 0, 0x12, 0, 0);
    }
    zSTRING target = GetTriggerTarget();
    if (homeWorld && homeWorld->csPlayer)
        homeWorld->csPlayer->OnCSTrigger(target, other);
}

void oCCSTrigger::UntriggerTarget(zCVob* other)
{
    if (zerr->filter_level >= 8) {
        const zSTRING otherName = other ? other->GetObjectName() : zSTRING("");
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: CSTrigger/Untrigger " + GetObjectName() + " by " + otherName,
            8, 0, 0x1A, 0, 0);
    }
    zSTRING target = GetTriggerTarget();
    if (homeWorld && homeWorld->csPlayer)
        homeWorld->csPlayer->OnCSUntrigger(target, other);
}

zCObject* oCCSTrigger::_CreateNewInstance()
{
    oCCSTrigger* trigger = new oCCSTrigger;
    zCClassDef::ObjectCreated(trigger, classDef);
    return trigger;
}
