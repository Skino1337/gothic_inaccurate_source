#include "include/oNpcMessages.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/oInformationManager.h"
#include <cstring>

static zCClassDef oCMsgMovementClassDef("oCMsgMovement", "oCNpcMessage",
    oCMsgMovement::_CreateNewInstance, 0, 0x7C, 0, "");
zCClassDef* oCMsgMovement::classDef = &oCMsgMovementClassDef;
static zCClassDef oCMsgAttackClassDef("oCMsgAttack", "oCMsgWeapon", oCMsgAttack::_CreateNewInstance,
    0, 0x58, 0, "");
zCClassDef* oCMsgAttack::classDef = &oCMsgAttackClassDef;
static zCClassDef oCMsgWeaponClassDef("oCMsgWeapon", "oCNpcMessage", oCMsgWeapon::_CreateNewInstance,
    0, 0x50, 0, "");
zCClassDef* oCMsgWeapon::classDef = &oCMsgWeaponClassDef;
static zCClassDef oCMsgMagicClassDef("oCMsgMagic", "oCMsgAttack", oCMsgMagic::_CreateNewInstance,
    0, 0x6C, 1, "");
zCClassDef* oCMsgMagic::classDef = &oCMsgMagicClassDef;
static zCClassDef oCMsgStateClassDef("oCMsgState", "oCNpcMessage", oCMsgState::_CreateNewInstance,
    0, 0x74, 0, "");
zCClassDef* oCMsgState::classDef = &oCMsgStateClassDef;
static zCClassDef oCMsgUseItemClassDef("oCMsgUseItem", "oCNpcMessage", oCMsgUseItem::_CreateNewInstance,
    0, 0x50, 0, "");
zCClassDef* oCMsgUseItem::classDef = &oCMsgUseItemClassDef;
static zCClassDef oCMsgManipulateClassDef("oCMsgManipulate", "oCNpcMessage",
    oCMsgManipulate::_CreateNewInstance, 0, 0x7C, 1, "");
zCClassDef* oCMsgManipulate::classDef = &oCMsgManipulateClassDef;
static zCClassDef oCMsgConversationClassDef("oCMsgConversation", "oCNpcMessage",
    oCMsgConversation::_CreateNewInstance, 0, 0x98, 0, "");
zCClassDef* oCMsgConversation::classDef = &oCMsgConversationClassDef;

void oCMsgState::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

void* oCMsgMovement::operator new(unsigned int size)
{
    oCMsgMovement* message = static_cast<oCMsgMovement*>(::operator new(size));
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

oCMsgAttack::oCMsgAttack()
    : attackTarget(0), target(0), attackSubType(0), attackAni(0), hitChance(0.0f),
      combo(0), hitAni(-1), startFrame(0.0f), enableNextHit(0), reachedTarget(0)
{
}

oCMsgAttack::oCMsgAttack(int type, int comboValue, int hitAniValue)
    : attackTarget(0), target(0), attackSubType(0), attackAni(0), hitChance(0.0f),
      combo(comboValue), hitAni(hitAniValue), startFrame(0.0f), enableNextHit(0), reachedTarget(0)
{
    subType = type;
}

oCMsgAttack::oCMsgAttack(int type, zCVob* targetVob, float frame)
    : attackTarget(0), target(targetVob), attackSubType(0), attackAni(0), hitChance(0.0f),
      combo(0), hitAni(-1), startFrame(frame), enableNextHit(0), reachedTarget(0)
{
    subType = type;
}

void oCMsgAttack::Archive(zCArchiver& archiver)
{
    oCNpcMessage::Archive(archiver);
    archiver.WriteInt(combo);
    if (!archiver.InProperties())
        archiver.WriteBool(enableNextHit);
}

void oCMsgAttack::Unarchive(zCArchiver& archiver)
{
    oCNpcMessage::Unarchive(archiver);
    combo = archiver.ReadInt();
    if (!archiver.InProperties())
        enableNextHit = archiver.ReadBool();
}

void oCMsgAttack::Pack(zCBuffer& buffer, zCEventManager* manager)
{
    buffer.Write(&combo, sizeof(combo));
    buffer.Write(&enableNextHit, sizeof(enableNextHit));
    buffer.Write(&startFrame, sizeof(startFrame));
    zSTRING aniName;
    oCNpc* npc = zDYNAMIC_CAST<oCNpc>(manager->hostVob);
    zCModel* model = npc ? npc->GetModel() : NULL;
    if (model && hitAni != -1)
    {
        for (int i = model->modelProtoList.numInArray - 1; i >= 0; --i)
        {
            zCModelPrototype* prototype = model->modelProtoList[i];
            if (hitAni < prototype->protoAnis.numInArray && prototype->protoAnis[hitAni])
            {
                aniName = prototype->protoAnis[hitAni]->GetAniName();
                break;
            }
        }
    }
    buffer.WriteString(aniName);
}

void oCMsgAttack::Unpack(zCBuffer& buffer, zCEventManager* manager)
{
    buffer.Read(&combo, sizeof(combo));
    buffer.Read(&enableNextHit, sizeof(enableNextHit));
    buffer.Read(&startFrame, sizeof(startFrame));
    zSTRING aniName = buffer.ReadString();
    oCNpc* npc = zDYNAMIC_CAST<oCNpc>(manager->hostVob);
    zCModel* model = npc ? npc->GetModel() : NULL;
    hitAni = -1;
    if (model && model->modelProtoList.numInArray > 0)
    {
        zCModelAni* ani = model->modelProtoList[0]->SearchAni(aniName);
        if (ani)
            hitAni = ani->aniID;
    }
}

oCMsgAttack::~oCMsgAttack()
{
}

void oCMsgAttack::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

oCMsgMagic::oCMsgMagic()
    : spellID(0), target(0), what(0), removeSymbol(0), level(0), manaInvested(0), energyLeft(0),
      activeSpell(0)
{
}

oCMsgMagic::oCMsgMagic(int type)
    : spellID(0), target(0), what(0), removeSymbol(0), level(0), manaInvested(0), energyLeft(0),
      activeSpell(0)
{
    subType = type;
}

oCMsgMagic::oCMsgMagic(int type, int spell, int remove)
    : spellID(spell), target(0), what(spell), removeSymbol(remove), level(0), manaInvested(0),
      energyLeft(0), activeSpell(0)
{
    subType = type;
}

oCMsgMagic::~oCMsgMagic()
{
}

void oCMsgMagic::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

static int npcMessageState = -1;
static int npcCombatState = -1;
static zSTRING npcCombatPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING npcCombatPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING npcCombatPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING npcCombatPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING npcCombatPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING npcCombatPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING npcCombatPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING npcCombatPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING npcCombatPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING npcCombatPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float npcCombatFarDistance = 250000.0f * 0.0166666675f;
static float npcCombatNearDistance = npcCombatFarDistance * 0.0166666675f;
static float npcCombatFarPlaneDistance = 250000.0f * 24.0f;
static zSTRING npcCombatBuffer;
static zSTRING npcMagicStateNames[9] = {
    "ZS_ASSESSMAGIC", "ZS_ASSESSSTOPMAGIC", "ZS_MAGICFREEZE", "ZS_WHIRLWIND",
    "ZS_SHORTZAPPED", "ZS_ZAPPED", "ZS_PYRO", "ZS_MAGICSLEEP", ""
};
static float npcMessageFarDistance = 250000.0f * 0.0166666675f;
static float npcMessageNearDistance = npcMessageFarDistance * 0.0166666675f;
static float npcMessageFarPlaneDistance = 250000.0f * 24.0f;
static zSTRING npcMessagePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING npcMessagePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING npcMessagePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING npcMessagePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING npcMessagePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING npcMessagePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING npcMessagePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING npcMessagePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING npcMessagePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING npcMessagePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static zCClassDef oCNpcMessageClassDef("oCNpcMessage", "zCEventMessage",
    oCNpcMessage::_CreateNewInstance, 0, 0x44, 0, "");
zCClassDef* oCNpcMessage::classDef = &oCNpcMessageClassDef;

zCObject* oCNpcMessage::_CreateNewInstance()
{
    oCNpcMessage* message = new oCNpcMessage;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

void oCNpcMessage::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* oCNpcMessage::_GetClassDef() const
{
    return classDef;
}

void oCNpcMessage::Delete()
{
    deleted = 1;
}

void oCNpcMessage::SetInUse(int value)
{
    inUse = value;
}

int oCNpcMessage::IsInUse()
{
    return inUse;
}

void oCNpcMessage::SetHighPriority(int value)
{
    highPriority = value;
}

int oCNpcMessage::GetIgnoreCutsceneMode()
{
    return 0;
}

int oCNpcMessage::IsOverlay()
{
    return 0;
}

int oCNpcMessage::IsJob()
{
    return 1;
}

int oCNpcMessage::IsDeleted()
{
    return deleted;
}

int oCNpcMessage::IsDeleteable()
{
    return 1;
}

int oCNpcMessage::IsHighPriority()
{
    return highPriority;
}

zCObject* oCMsgDamage::_CreateNewInstance()
{
    oCMsgDamage* message = new oCMsgDamage;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

zCClassDef* oCMsgDamage::_GetClassDef() const
{
    return classDef;
}

int oCMsgDamage::IsNetRelevant()
{
    return 1;
}

int oCMsgDamage::IsOverlay()
{
    return subType == 1;
}

int oCMsgDamage::IsDeleteable()
{
    return 0;
}

int oCMsgDamage::IsHighPriority()
{
    highPriority = 1;
    return -1;
}

oCMsgWeapon::oCMsgWeapon()
    : targetMode(0), duringRun(0), initDone(0), firstTime(1), useFist(0), showMagicCircle(1),
      ani(-1), inUse(0)
{
}

oCMsgWeapon::oCMsgWeapon(int type, int mode, int run)
    : targetMode(mode), duringRun(run), initDone(0), firstTime(1), useFist(0), showMagicCircle(1),
      ani(-1), inUse(0)
{
    subType = type;
}

void oCMsgWeapon::Archive(zCArchiver& archiver)
{
    oCNpcMessage::Archive(archiver);
    archiver.WriteInt(targetMode);
    if (archiver.InProperties())
        return;
    archiver.WriteBool(duringRun);
    archiver.WriteBool(initDone);
    archiver.WriteBool(firstTime);
}

void oCMsgWeapon::Unarchive(zCArchiver& archiver)
{
    oCNpcMessage::Unarchive(archiver);
    targetMode = archiver.ReadInt();
    if (archiver.InProperties())
        return;
    duringRun = archiver.ReadBool();
    initDone = archiver.ReadBool();
    firstTime = archiver.ReadBool();
}

int oCMsgWeapon::MD_GetNumOfSubTypes()
{
    return 14;
}

zSTRING oCMsgWeapon::MD_GetSubTypeString(int type)
{
    static const char* names[14] = {
        "EV_DRAWWEAPON", "EV_DRAWWEAPON1", "EV_DRAWWEAPON2", "EV_REMOVEWEAPON",
        "EV_REMOVEWEAPON1", "EV_REMOVEWEAPON2", "EV_CHOOSEWEAPON", "EV_FORCEREMOVEWEAPON",
        "EV_ATTACK", "EV_EQUIPBESTWEAPON", "EV_EQUIPBESTARMOR", "EV_UNEQUIPWEAPONS",
        "EV_UNEQUIPARMOR", "EV_EQUIPARMOR"
    };
    if (type >= 0 && type < 14)
        return zSTRING(names[type]);
    return zSTRING();
}

zCEventMessage::zTTimeBehavior oCMsgWeapon::MD_GetTimeBehavior()
{
    return (zCEventMessage::zTTimeBehavior)0;
}

float oCMsgWeapon::MD_GetMinTime()
{
    return 0.0f;
}

oCMsgWeapon::~oCMsgWeapon()
{
}

zCObject* oCMsgWeapon::_CreateNewInstance()
{
    oCMsgWeapon* message = new oCMsgWeapon;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

zCClassDef* oCMsgWeapon::_GetClassDef() const
{
    return classDef;
}

void oCMsgMovement::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void oCMsgManipulate::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void oCMsgConversation::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCObject* oCMsgMovement::_CreateNewInstance()
{
    oCMsgMovement* message = new oCMsgMovement;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

zCClassDef* oCMsgMovement::_GetClassDef() const
{
    return classDef;
}

zCObject* oCMsgAttack::_CreateNewInstance()
{
    oCMsgAttack* message = new oCMsgAttack;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

zCClassDef* oCMsgAttack::_GetClassDef() const
{
    return classDef;
}

int oCMsgAttack::IsNetRelevant()
{
    return 1;
}

int oCMsgAttack::IsOverlay()
{
    return subType == 6 || subType == 9;
}

zCObject* oCMsgUseItem::_CreateNewInstance()
{
    oCMsgUseItem* message = new oCMsgUseItem;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

void oCMsgUseItem::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* oCMsgUseItem::_GetClassDef() const
{
    return classDef;
}

int oCMsgUseItem::IsNetRelevant()
{
    return 1;
}

zCObject* oCMsgState::_CreateNewInstance()
{
    oCMsgState* message = new oCMsgState;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

zCClassDef* oCMsgState::_GetClassDef() const
{
    return classDef;
}

int oCMsgState::IsOverlay()
{
    return subType == 4;
}

zCObject* oCMsgManipulate::_CreateNewInstance()
{
    oCMsgManipulate* message = new oCMsgManipulate;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

zCClassDef* oCMsgManipulate::_GetClassDef() const
{
    return classDef;
}

int oCMsgManipulate::IsNetRelevant()
{
    return subType >= 6 && subType <= 14;
}

zCObject* oCMsgConversation::_CreateNewInstance()
{
    oCMsgConversation* message = new oCMsgConversation;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

zCClassDef* oCMsgConversation::_GetClassDef() const
{
    return classDef;
}

zCObject* oCMsgMagic::_CreateNewInstance()
{
    oCMsgMagic* message = new oCMsgMagic;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

zCClassDef* oCMsgMagic::_GetClassDef() const
{
    return classDef;
}

int oCMsgMagic::IsJob()
{
    return subType == 8;
}

int oCMsgMagic::IsHighPriority()
{
    return subType != 8;
}

int oCMsgMagic::IsNetRelevant()
{
    return 0;
}

void oCNpcMessage::Archive(zCArchiver& archiver)
{
    zCEventMessage::Archive(archiver);
    if (!archiver.InSaveGame())
        return;
    archiver.WriteBool(highPriority);
    archiver.WriteBool(inUse);
    archiver.WriteBool(deleted);
}

void oCNpcMessage::Unarchive(zCArchiver& archiver)
{
    zCEventMessage::Unarchive(archiver);
    if (!archiver.InSaveGame())
        return;
    highPriority = archiver.ReadBool();
    inUse = archiver.ReadBool();
    deleted = archiver.ReadBool();
}

oCMsgDamage::oCMsgDamage()
{
    std::memset(&descDamage, 0, sizeof(descDamage));
    descDamage.fDamageMultiplier = 1.0f;
}

oCMsgDamage::oCMsgDamage(int type)
{
    std::memset(&descDamage, 0, sizeof(descDamage));
    descDamage.fDamageMultiplier = 1.0f;
    subType = type;
}

oCMsgDamage::oCMsgDamage(int type, const oCNpc::oSDamageDescriptor& descriptor)
{
    subType = type;
    descDamage = descriptor;
}

int oCMsgDamage::MD_GetNumOfSubTypes()
{
    return 2;
}

zSTRING oCMsgDamage::MD_GetSubTypeString(int type)
{
    if (type == 0)
        return zSTRING("EV_DAMAGE_ONCE");
    if (type == 1)
        return zSTRING("EV_DAMAGE_PER_FRAME");
    return zSTRING();
}

void oCMsgDamage::Pack(zCBuffer& buffer, zCEventManager* manager)
{
}

void oCMsgDamage::Unpack(zCBuffer& buffer, zCEventManager* manager)
{
}

void oCMsgDamage::Archive(zCArchiver& archiver)
{
    oCNpcMessage::Archive(archiver);
    archiver.WriteRaw(&descDamage, sizeof(descDamage));
}

void oCMsgDamage::Unarchive(zCArchiver& archiver)
{
    oCNpcMessage::Unarchive(archiver);
    archiver.ReadRaw(&descDamage, sizeof(descDamage));
}

oCMsgMovement::oCMsgMovement()
    : targetVob(0), targetMode(0), ani(-1), inUse(0), timer(0.0f), angle(0.0f), route(0)
{
}

oCMsgMovement::oCMsgMovement(int type, const zSTRING& name)
    : targetVob(0), targetName(name), targetMode(0), ani(-1), inUse(0), timer(0.0f),
      angle(0.0f), route(0)
{
    subType = type;
}

oCMsgMovement::oCMsgMovement(int type, zCVob* vob)
    : targetVob(vob), targetMode(0), ani(-1), inUse(0), timer(0.0f), angle(0.0f), route(0)
{
    subType = type;
    if (vob)
        targetPos = vob->trafoObjToWorld.GetTranslation();
}

oCMsgMovement::oCMsgMovement(int type, const zVEC3& position)
    : targetVob(0), targetPos(position), targetMode(0), ani(-1), inUse(0), timer(0.0f),
      angle(0.0f), route(0)
{
    subType = type;
}

oCMsgMovement::oCMsgMovement(int type, float angleValue)
    : targetVob(0), targetMode(0), ani(-1), inUse(0), timer(0.0f), angle(angleValue), route(0)
{
    subType = type;
}

oCMsgMovement::oCMsgMovement(int type, int mode)
    : targetVob(0), targetMode(mode), ani(-1), inUse(0), timer(0.0f), angle(0.0f), route(0)
{
    subType = type;
}

void oCMsgMovement::Init()
{
    targetPos = zVEC3(0.0f, 0.0f, 0.0f);
    route = NULL;
    targetVob = NULL;
    angle = 0.0f;
    targetMode = 0;
    ani = -1;
}

void oCMsgMovement::Archive(zCArchiver& archiver)
{
    oCNpcMessage::Archive(archiver);
    if (subType == 14 || subType == 3 || subType == 16)
        archiver.WriteString(targetName);
    if (subType == 2 || subType == 6 || subType == 7 || subType == 10)
    {
        if (!archiver.InProperties())
            archiver.WriteObject(targetVob);
        archiver.WriteString(targetVobName);
    }
    if (subType == 1 || subType == 5 || subType == 8)
        archiver.WriteVec3(targetPos);
    if (subType == 4)
        archiver.WriteFloat(angle);
    else if (subType == 9)
        archiver.WriteInt(targetMode);
}

void oCMsgMovement::Unarchive(zCArchiver& archiver)
{
    route = NULL;
    targetVob = NULL;
    oCNpcMessage::Unarchive(archiver);
    if (subType == 14 || subType == 3 || subType == 16)
        targetName = archiver.ReadString();
    if (subType == 2 || subType == 6 || subType == 7 || subType == 10)
    {
        if (!archiver.InProperties())
            targetVob = (zCVob*)archiver.ReadObject(NULL);
        targetVobName = archiver.ReadString();
    }
    if (subType == 1 || subType == 5 || subType == 8)
        targetPos = archiver.ReadVec3();
    if (subType == 4)
        angle = archiver.ReadFloat();
    else if (subType == 9)
        targetMode = archiver.ReadInt();
}

int oCMsgMovement::MD_GetNumOfSubTypes()
{
    return 18;
}

zSTRING oCMsgMovement::MD_GetSubTypeString(int type)
{
    static const char* names[18] = {
        "EV_ROBUSTTRACE", "EV_GOTOPOS", "EV_GOTOVOB", "EV_GOROUTE", "EV_TURN",
        "EV_TURNTOPOS", "EV_TURNTOVOB", "EV_TURNAWAY", "EV_JUMP", "EV_SETWALKMODE",
        "EV_WHIRLAROUND", "EV_STANDUP", "EV_CANSEENPC", "EV_STRAFE", "EV_GOTOFP",
        "EV_DODGE", "EV_BEAMTO", "EV_ALIGNTOFP"
    };
    if (type >= 0 && type < 18)
        return zSTRING(names[type]);
    return zSTRING();
}

zSTRING oCMsgMovement::MD_GetVobRefName()
{
    if (subType == 3 || subType == 14 || subType == 16)
        return zSTRING();
    return targetVobName;
}

void oCMsgMovement::MD_SetVobRefName(const zSTRING& name)
{
    targetVobName = name;
}

void oCMsgMovement::MD_SetVobParam(zCVob* vob)
{
    targetVob = vob;
}

void oCMsgMovement::Delete()
{
    oCNpcMessage::Delete();
}

zCEventMessage::zTTimeBehavior oCMsgMovement::MD_GetTimeBehavior()
{
    switch (subType)
    {
    case 1:
    case 2:
    case 3:
    case 8:
    case 10:
    case 11:
    case 14:
        return (zCEventMessage::zTTimeBehavior)2;
    }
    return (zCEventMessage::zTTimeBehavior)0;
}

float oCMsgMovement::MD_GetMinTime()
{
    return 3.0f;
}

int oCMsgAttack::MD_GetNumOfSubTypes()
{
    return 12;
}

zSTRING oCMsgAttack::MD_GetSubTypeString(int type)
{
    static const char* names[10] = {
        "EV_ATTACKFORWARD", "EV_ATTACKLEFT", "EV_ATTACKRIGHT", "EV_ATTACKRUN",
        "EV_ATTACKFINISH", "EV_PARADE", "EV_AIMAT", "EV_SHOOTAT", "EV_STOPAIM", "EV_DEFEND"
    };
    if (type >= 0 && type < 10)
        return zSTRING(names[type]);
    return zSTRING();
}

oCMsgUseItem::oCMsgUseItem()
    : item(0), vob(0), ani(-1), state(0), inUse(0)
{
}

oCMsgUseItem::oCMsgUseItem(int type, zCVob* target)
    : item((oCItem*)target), vob(target), ani(0), state(0), inUse(0)
{
    subType = type;
}

oCMsgUseItem::~oCMsgUseItem()
{
    vob = NULL;
    item = NULL;
}

void oCMsgUseItem::Pack(zCBuffer& buffer, zCEventManager* manager)
{
    int hasVob = vob != NULL;
    buffer.Write(&hasVob, sizeof(hasVob));
    buffer.Write(&state, sizeof(state));
}

void oCMsgUseItem::Unpack(zCBuffer& buffer, zCEventManager* manager)
{
    unsigned long vobID = 0;
    buffer.Read(&vobID, sizeof(vobID));
    vob = ogame->GetWorld()->SearchVobByID(vobID, NULL);
    item = (oCItem*)vob;
    buffer.Read(&state, sizeof(state));
}

void oCMsgUseItem::Archive(zCArchiver& archiver)
{
    oCNpcMessage::Archive(archiver);
    archiver.WriteObject(vob);
}

void oCMsgUseItem::Unarchive(zCArchiver& archiver)
{
    oCNpcMessage::Unarchive(archiver);
    vob = (zCVob*)archiver.ReadObject(NULL);
    item = (oCItem*)vob;
}

oCMsgState::oCMsgState()
    : function(-1), instance(-1), minutes(0), other(0), victim(0), endOldState(0), timer(0.0f)
{
}

oCMsgState::oCMsgState(int type, float time)
    : function(-1), instance(-1), minutes(0), other(0), victim(0), endOldState(0), timer(time)
{
    subType = type;
}

oCMsgState::oCMsgState(int type, int func)
    : function(func), instance(-1), minutes(0), other(0), victim(0), endOldState(0), timer(0.0f)
{
    subType = type;
}

oCMsgState::oCMsgState(int type, int func, int endOld, const zSTRING& waypoint)
    : wp(waypoint), function(func), instance(-1), minutes(0), other(0), victim(0),
      endOldState(endOld), timer(0.0f)
{
    subType = type;
}

oCMsgState::~oCMsgState()
{
    other = NULL;
    victim = NULL;
}

void oCMsgState::Archive(zCArchiver& archiver)
{
    oCNpcMessage::Archive(archiver);
    if (subType == 1 || subType == 4)
        archiver.WriteFloat(timer);
    if (subType == 3)
    {
        archiver.WriteInt(function);
        archiver.WriteInt(minutes);
    }
}

void oCMsgState::Unarchive(zCArchiver& archiver)
{
    oCNpcMessage::Unarchive(archiver);
    if (subType == 1 || subType == 4)
        timer = archiver.ReadFloat();
    if (subType == 3)
    {
        function = archiver.ReadInt();
        minutes = archiver.ReadInt();
    }
}

int oCMsgState::MD_GetNumOfSubTypes()
{
    return 5;
}

zSTRING oCMsgState::MD_GetSubTypeString(int type)
{
    static const char* names[5] = {
        "EV_STARTSTATE", "EV_WAIT", "EV_SETNPCSTOSTATE", "EV_SETTIME", "EV_APPLYTIMEDOVERLAY"
    };
    if (type >= 0 && type < 5)
        return zSTRING(names[type]);
    return zSTRING();
}

zCEventMessage::zTTimeBehavior oCMsgState::MD_GetTimeBehavior()
{
    return (zCEventMessage::zTTimeBehavior)1;
}

float oCMsgState::MD_GetMinTime()
{
    if (subType == 1)
        return timer;
    if (subType > 3)
        return 3.0f;
    return 0.0f;
}

oCMsgManipulate::oCMsgManipulate()
    : targetVob(0), aniCombY(0.5f), targetState(-1), flag(0), state(0), inUse(0)
{
}

oCMsgManipulate::oCMsgManipulate(int type)
    : targetVob(0), aniCombY(0.5f), targetState(-1), flag(0), state(0), inUse(0)
{
    subType = type;
}

oCMsgManipulate::oCMsgManipulate(int type, zCVob* vob)
    : targetVob(vob), aniCombY(0.5f), targetState(-1), flag(0), state(0), inUse(0)
{
    subType = type;
}

oCMsgManipulate::oCMsgManipulate(int type, int stateValue)
    : targetVob(0), aniCombY(0.5f), targetState(stateValue), flag(0), state(0), inUse(0)
{
    subType = type;
}

oCMsgManipulate::oCMsgManipulate(int type, const zSTRING& itemName, int stateValue)
    : targetVob(0), name(itemName), aniCombY(0.5f), targetState(stateValue), flag(0), state(0),
      inUse(0)
{
    subType = type;
}

oCMsgManipulate::oCMsgManipulate(int type, zCVob* vob, int stateValue)
    : targetVob(vob), aniCombY(0.5f), targetState(stateValue), flag(0), state(0), inUse(0)
{
    subType = type;
}

oCMsgManipulate::oCMsgManipulate(int type, const zSTRING& itemName, const zSTRING& slotName)
    : targetVob(0), name(itemName), slot(slotName), aniCombY(0.5f), targetState(0), flag(0),
      state(0), inUse(0)
{
    subType = type;
}

oCMsgManipulate::~oCMsgManipulate()
{
    targetVob = NULL;
}

void oCMsgManipulate::Archive(zCArchiver& archiver)
{
    oCNpcMessage::Archive(archiver);
    if (!subType)
    {
        archiver.WriteString(targetVobName);
        return;
    }
    if (subType == 1 || subType == 4 || subType == 17 || subType == 16 || subType == 5 ||
        subType == 13 || subType == 14 || subType == 15)
        archiver.WriteString(name);
    if (subType == 4 || subType == 15)
        archiver.WriteInt(targetState);
}

void oCMsgManipulate::Unarchive(zCArchiver& archiver)
{
    oCNpcMessage::Unarchive(archiver);
    if (!subType)
    {
        targetVobName = archiver.ReadString();
        return;
    }
    if (subType == 1 || subType == 4 || subType == 17 || subType == 16 || subType == 5 ||
        subType == 13 || subType == 14 || subType == 15)
        name = archiver.ReadString();
    if (subType == 4 || subType == 15)
        targetState = archiver.ReadInt();
}

void oCMsgManipulate::Pack(zCBuffer& buffer, zCEventManager* manager)
{
    buffer.Write(&subType, sizeof(subType));
    buffer.WriteString(name);
    buffer.WriteString(slot);
    int hasVob = targetVob != NULL;
    buffer.Write(&hasVob, sizeof(hasVob));
    buffer.Write(&targetState, sizeof(targetState));
    buffer.WriteBool(flag, 32);
}

void oCMsgManipulate::Unpack(zCBuffer& buffer, zCEventManager* manager)
{
    buffer.Read(&subType, sizeof(subType));
    name = buffer.ReadString();
    slot = buffer.ReadString();
    unsigned long vobID = 0;
    buffer.Read(&vobID, sizeof(vobID));
    targetVob = NULL;
    if (vobID > 0)
        targetVob = ogame->GetWorld()->SearchVobByID(vobID, NULL);
    buffer.Read(&targetState, sizeof(targetState));
    flag = buffer.ReadBool(32);
}

int oCMsgManipulate::MD_GetNumOfSubTypes()
{
    return 18;
}

zSTRING oCMsgManipulate::MD_GetSubTypeString(int type)
{
    static const char* names[18] = {
        "EV_TAKEVOB", "EV_DROPVOB", "EV_THROWVOB", "EV_EXCHANGE", "EV_USEMOB",
        "EV_USEITEM", "EV_INSERTINTERACTITEM", "EV_REMOVEINTERACTITEM", "EV_CREATEINTERACTITEM",
        "EV_DESTROYINTERACTITEM", "EV_PLACEINTERACTITEM", "EV_EXCHANGEINTERACTITEM",
        "EV_USEMOBWITHITEM", "EV_CALLSCRIPT", "EV_EQUIPITEM", "EV_USEITEMTOSTATE",
        "EV_TAKEMOB", "EV_DROPMOB"
    };
    if (type >= 0 && type < 18)
        return zSTRING(names[type]);
    return zSTRING();
}

zCEventMessage::zTTimeBehavior oCMsgManipulate::MD_GetTimeBehavior()
{
    switch (subType)
    {
    case 0:
    case 1:
    case 2:
    case 4:
    case 5:
    case 15:
    case 16:
    case 17:
        return (zCEventMessage::zTTimeBehavior)2;
    }
    return (zCEventMessage::zTTimeBehavior)0;
}

float oCMsgManipulate::MD_GetMinTime()
{
    return 6.0f;
}

zSTRING oCMsgManipulate::MD_GetVobRefName()
{
    if (subType == 4 || subType == 16 || subType == 17 || subType == 1 || subType == 5 ||
        subType == 15 || subType == 14)
        return zSTRING();
    return targetVobName;
}

void oCMsgManipulate::MD_SetVobRefName(const zSTRING& name)
{
    targetVobName = name;
}

void oCMsgManipulate::MD_SetVobParam(zCVob* vob)
{
    targetVob = vob;
}

oCMsgConversation::oCMsgConversation()
    : target(0), ani(-1), handle(0), timer(0.0f), number(-1), f_no(0), f_yes(0), inUse(0)
{
}

oCMsgConversation::oCMsgConversation(int type)
    : target(0), ani(-1), handle(0), timer(0.0f), number(-1), f_no(0), f_yes(0), inUse(0)
{
    subType = type;
}

oCMsgConversation::oCMsgConversation(int type, const zSTRING& nameValue)
    : name(nameValue), target(0), ani(-1), handle(0), timer(0.0f), number(-1), f_no(0), f_yes(0),
      inUse(0)
{
    subType = type;
}

oCMsgConversation::oCMsgConversation(int type, const zSTRING& textValue, const zSTRING& nameValue)
    : text(textValue), name(nameValue), target(0), ani(-1), handle(0), timer(0.0f), number(-1),
      f_no(0), f_yes(0), inUse(0)
{
    subType = type;
}

oCMsgConversation::oCMsgConversation(int type, zCVob* targetVob)
    : target(targetVob), ani(-1), handle(0), timer(0.0f), number(-1), f_no(0), f_yes(0), inUse(0)
{
    subType = type;
}

oCMsgConversation::oCMsgConversation(int type, int aniValue)
    : target(0), ani(aniValue), handle(0), timer(0.0f), number(-1), f_no(0), f_yes(0), inUse(0)
{
    subType = type;
}

oCMsgConversation::oCMsgConversation(int type, int numberValue, int no, float time)
    : target(0), ani(-1), handle(0), timer(time), number(numberValue), f_no(no), f_yes(0), inUse(0)
{
    subType = type;
}

oCMsgConversation::~oCMsgConversation()
{
    target = NULL;
}

void oCMsgConversation::Archive(zCArchiver& archiver)
{
    oCNpcMessage::Archive(archiver);
    if (subType == 2 || subType == 6 || !subType)
        archiver.WriteString(text);
    if (!subType || subType == 1 || subType == 15 || subType == 2 || subType == 5 || subType == 6)
        archiver.WriteString(name);
    if (subType == 3 || subType == 12 || subType == 7 || subType == 13)
        archiver.WriteString(targetVobName);
    if (subType == 4)
        archiver.WriteInt(number);
}

void oCMsgConversation::Unarchive(zCArchiver& archiver)
{
    oCNpcMessage::Unarchive(archiver);
    if (!subType || subType == 2 || subType == 6)
        text = archiver.ReadString();
    if (!subType || subType == 1 || subType == 15 || subType == 2 || subType == 5 || subType == 6)
    {
        name = archiver.ReadString();
        name.Upper();
    }
    if (subType == 3 || subType == 12 || subType == 7 || subType == 13)
    {
        targetVobName = archiver.ReadString();
        name = targetVobName;
    }
    if (subType == 4)
        number = archiver.ReadInt();
}

int oCMsgConversation::MD_GetNumOfSubTypes()
{
    return 24;
}

void oCMsgConversation::Delete()
{
    if (subType != 16 || oCInformationManager::GetInformationManager().HasFinished())
        oCNpcMessage::Delete();
}

int oCMsgConversation::IsOverlay()
{
    switch (subType)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 12:
    case 13:
    case 15:
    case 16:
    case 18:
    case 20:
        return 1;
    }
    return 0;
}

zSTRING oCMsgConversation::MD_GetSubTypeString(int type)
{
    static const char* names[20] = {
        "EV_PLAYANISOUND", "EV_PLAYANI", "EV_PLAYSOUND", "EV_LOOKAT", "EV_OUTPUT",
        "EV_OUTPUTSVM", "EV_CUTSCENE", "EV_WAITTILLEND", "EV_ASK", "EV_WAITFORQUESTION",
        "EV_STOPLOOKAT", "EV_STOPPOINTAT", "EV_POINTAT", "EV_QUICKLOOK",
        "EV_PLAYANI_NOOVERLAY", "EV_PLAYANI_FACE", "EV_PROCESSINFOS", "EV_STOPPROCESSINFOS",
        "EV_OUTPUTSVM_OVERLAY", "EV_SNDPLAY"
    };
    if (type >= 0 && type < 20)
        return zSTRING(names[type]);
    return zSTRING();
}

zSTRING oCMsgConversation::MD_GetVobRefName()
{
    return targetVobName;
}

void oCMsgConversation::MD_SetVobRefName(const zSTRING& name)
{
    targetVobName = name;
}

void oCMsgConversation::MD_SetVobParam(zCVob* vob)
{
    target = vob;
}

zCEventMessage::zTTimeBehavior oCMsgConversation::MD_GetTimeBehavior()
{
    if (subType > 13)
        return (zCEventMessage::zTTimeBehavior)0;
    switch (subType)
    {
    case 0:
    case 1:
    case 2:
    case 13:
        return (zCEventMessage::zTTimeBehavior)1;
    case 3:
    case 10:
    case 11:
    case 12:
        return (zCEventMessage::zTTimeBehavior)3;
    default:
        return (zCEventMessage::zTTimeBehavior)2;
    }
}

float oCMsgConversation::MD_GetMinTime()
{
    switch (subType)
    {
    case 0:
    case 2:
    case 19:
    {
        float playingTime = zsound->GetPlayingTimeMSEC(name) * 0.001f;
        return playingTime > 0.0f ? playingTime : timer;
    }
    case 1:
        return timer;
    case 3:
    case 6:
    case 12:
        return 1.0f;
    case 4:
    case 5:
        return 2.0f;
    }
    return 0.0f;
}

void oCMsgMagic::Pack(zCBuffer& buffer, zCEventManager* manager)
{
    buffer.Write(&subType, sizeof(subType));
    buffer.Write(&what, sizeof(what));
    buffer.Write(&removeSymbol, sizeof(removeSymbol));
    buffer.Write(&level, sizeof(level));
    buffer.Write(&manaInvested, sizeof(manaInvested));
    buffer.Write(&energyLeft, sizeof(energyLeft));
    buffer.Write(&activeSpell, sizeof(activeSpell));
    int hasTarget = target != NULL;
    buffer.Write(&hasTarget, sizeof(hasTarget));
    buffer.Write(&targetPos, sizeof(targetPos));
}

void oCMsgMagic::Unpack(zCBuffer& buffer, zCEventManager* manager)
{
    buffer.Read(&subType, sizeof(subType));
    buffer.Read(&what, sizeof(what));
    buffer.Read(&removeSymbol, sizeof(removeSymbol));
    buffer.Read(&level, sizeof(level));
    buffer.Read(&manaInvested, sizeof(manaInvested));
    buffer.Read(&energyLeft, sizeof(energyLeft));
    buffer.Read(&activeSpell, sizeof(activeSpell));
    unsigned long targetID = 0;
    buffer.Read(&targetID, sizeof(targetID));
    target = NULL;
    if (targetID)
        target = ogame->GetWorld()->SearchVobByID(targetID, NULL);
    buffer.Read(&targetPos, sizeof(targetPos));
}

int oCMsgMagic::MD_GetNumOfSubTypes()
{
    return 11;
}

zSTRING oCMsgMagic::MD_GetSubTypeString(int type)
{
    static const char* names[11] = {
        "EV_OPEN", "EV_CLOSE", "EV_MOVE", "EV_INVEST", "EV_CAST", "EV_SETLEVEL",
        "EV_SHOWSYMBOL", "EV_SETFRONTSPELL", "EV_CASTSPELL", "EV_READY", "EV_UNREADY"
    };
    if (type >= 0 && type < 11)
        return zSTRING(names[type]);
    return zSTRING();
}

void oCMsgMagic::Archive(zCArchiver& archiver)
{
    oCNpcMessage::Archive(archiver);
    archiver.WriteInt(what);
    archiver.WriteInt(level);
    archiver.WriteBool(removeSymbol);
}

void oCMsgMagic::Unarchive(zCArchiver& archiver)
{
    oCNpcMessage::Unarchive(archiver);
    what = archiver.ReadInt();
    level = archiver.ReadInt();
    removeSymbol = archiver.ReadBool();
}
