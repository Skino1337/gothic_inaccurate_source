#include "include/oTrigger.h"
#include "include/oGame.h"

static int triggerClassDefState = -1;
static int objectGeneratorState = -1;
static zSTRING objectGeneratorProgressTexture = "PROGRESS.TGA";
static zSTRING objectGeneratorProgressBarTexture = "PROGRESS_BAR.TGA";
static int objectGeneratorVisualState = -1;
static int triggerRuntimeState = -1;
static zSTRING triggerPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING triggerPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING triggerPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING triggerPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING triggerPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING triggerPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING triggerPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING triggerPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING triggerPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING triggerPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static zCClassDef oCTriggerScriptClassDef("oCTriggerScript", "zCTrigger",
    oCTriggerScript::_CreateNewInstance, 0, 0x17C, 0, "");
zCClassDef* oCTriggerScript::classDef = &oCTriggerScriptClassDef;

zCObject* oCTriggerChangeLevel::_CreateNewInstance()
{
    oCTriggerChangeLevel* trigger = new oCTriggerChangeLevel;
    zCClassDef::ObjectCreated(trigger, classDef);
    return trigger;
}

void oCTriggerChangeLevel::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* oCTriggerChangeLevel::_GetClassDef() const
{
    return classDef;
}

void oCTriggerChangeLevel::TriggerTarget(zCVob* vob)
{
    zCTrigger::TriggerTarget(vob);
    if (!vob || vob != oCNpc::player)
        return;
    for (int spell = 47; spell <= 58; ++spell) {
        oCSpell* activeSpell = oCNpc::player->IsSpellActive(spell);
        if (activeSpell) {
            activeSpell->EndTimedEffect();
            break;
        }
    }
    if (zerr->filter_level >= 8)
        zerr->Report(zERR_TYPE_INFO, 0, "B: TRIGGER: " + GetObjectName() + " "
            + _GetClassDef()->className + " triggered -> " + levelName, 8, 0, 0x60, 0, 0);
    if (oCNpc::player->GetWeaponMode() == 7) {
        oCMag_Book* spellBook = oCNpc::player->GetSpellBook();
        if (spellBook)
            spellBook->KillSelectedSpell();
        oCNpc::player->EV_ForceRemoveWeapon(0);
    }
    oCNpc::player->KillActiveSpells();
    ogame->TriggerChangeLevel(levelName, startVob);
}

void oCTriggerChangeLevel::UntriggerTarget(zCVob* vob)
{
    zCTrigger::UntriggerTarget(vob);
}

void oCTriggerChangeLevel::SetLevelName(const zSTRING& level, const zSTRING& start)
{
    levelName = level;
    startVob = start;
}

void oCTriggerChangeLevel::Archive(zCArchiver& archiver)
{
    zCTrigger::Archive(archiver);
    archiver.WriteString(levelName);
    archiver.WriteString(startVob);
}

void oCTriggerChangeLevel::Unarchive(zCArchiver& archiver)
{
    zCTrigger::Unarchive(archiver);
    levelName = archiver.ReadString();
    startVob = archiver.ReadString();
}

zCObject* oCTriggerScript::_CreateNewInstance()
{
    oCTriggerScript* trigger = new oCTriggerScript;
    zCClassDef::ObjectCreated(trigger, classDef);
    return trigger;
}

void oCTriggerScript::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* oCTriggerScript::_GetClassDef() const
{
    return classDef;
}

void oCTriggerScript::TriggerTarget(zCVob* vob)
{
    zCTrigger::TriggerTarget(vob);
    parser.SetInstance("SELF", this);
    parser.SetInstance("OTHER", vob);
    parser.SetInstance("ITEM", NULL);
    parser.CallFunc(scriptFunc);
}

void oCTriggerScript::UntriggerTarget(zCVob* vob)
{
    zCTrigger::UntriggerTarget(vob);
}

void oCTriggerScript::SetScriptFunc(const zSTRING& function, int count)
{
    scriptFunc = function;
    numCanBeActivated = (short)count;
    countCanBeActivated = (short)count;
}

void oCTriggerScript::Archive(zCArchiver& archiver)
{
    zCTrigger::Archive(archiver);
    archiver.WriteString(scriptFunc);
}

void oCTriggerScript::Unarchive(zCArchiver& archiver)
{
    zCTrigger::Unarchive(archiver);
    scriptFunc = archiver.ReadString();
}

void oCObjectGenerator::SetObjectName(const zSTRING& name)
{
    objectName = name;
}

void oCObjectGenerator::SetObjectSpeed(float value)
{
    speed = value;
}

void oCObjectGenerator::OnTrigger(zCVob*, zCVob*)
{
    if (!homeWorld || objectName.IsEmpty() || objectName.Search(".ZEN") > 0)
        return;
    const int instance = parser.GetIndex(objectName);
    zCVob* vob;
    if (instance > 0)
        vob = ogame->GetGameWorld()->CreateVob(0, instance);
    else {
        vob = new zCVob;
        zCClassDef::ObjectCreated(vob, zCVob::classDef);
        vob->SetObjectName(objectName);
    }
    if (vob) {
        vob->SetPhysicsEnabled(1);
        vob->SetTrafoObjToWorld(trafoObjToWorld);
        homeWorld->AddVob(vob);
    }
}

void oCObjectGenerator::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteString(objectName);
    archiver.WriteFloat(speed);
}

void oCObjectGenerator::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    objectName = archiver.ReadString();
    speed = archiver.ReadFloat();
}
