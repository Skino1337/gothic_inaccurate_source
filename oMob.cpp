#include "include/oMob.h"

static int mobState = -1;
static int mobRuntimeState = -1;
static zSTRING mobRuntimePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING mobRuntimePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING mobRuntimePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING mobRuntimePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING mobRuntimePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING mobRuntimePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING mobRuntimePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING mobRuntimePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING mobRuntimePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING mobRuntimePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float mobRuntimeFarDistance = 250000.0f * 0.0166666675f;
static float mobRuntimeNearDistance = mobRuntimeFarDistance * 0.0166666675f;
static float mobRuntimeFarPlaneDistance = 250000.0f * 24.0f;
static zSTRING mobBroadcast("broadcast");
static zSTRING mobLocalhost("127.0.0.1");
static zSTRING mobPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING mobPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING mobPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING mobPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING mobPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING mobPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING mobPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING mobPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING mobPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING mobPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float mobFarDistance = 250000.0f * 0.0166666675f;
static float mobNearDistance = mobFarDistance * 0.0166666675f;
static float mobFarPlaneDistance = 250000.0f * 24.0f;
#include "include/oCollisionObjectMiscChar.h"
#include "include/zParticle.h"
#include "include/zSoundMan.h"
#include "include/zTools.h"
#include "include/oAiShoot.h"

extern zCSoundManager* zsndMan;
extern int mobNameConv;
extern int mobSaveConv;
zCArray<zSTRING> mobList;

static zCClassDef oCMOBClassDef("oCMOB", "oCVob", oCMOB::_CreateNewInstance, 0, 0x188, 2, "");
zCClassDef* oCMOB::classDef = &oCMOBClassDef;
static zCClassDef oCMobInterClassDef("oCMobInter", "oCMOB", oCMobInter::_CreateNewInstance,
    0, 0x234, 3, "");
zCClassDef* oCMobInter::classDef = &oCMobInterClassDef;
static zCClassDef oCMobBedClassDef("oCMobBed", "oCMobInter", oCMobBed::_CreateNewInstance,
    0, 0x248, 0, "");
zCClassDef* oCMobBed::classDef = &oCMobBedClassDef;
static zCClassDef oCMobSwitchClassDef("oCMobSwitch", "oCMobInter", oCMobSwitch::_CreateNewInstance,
    0, 0x234, 0, "");
zCClassDef* oCMobSwitch::classDef = &oCMobSwitchClassDef;
static zCClassDef oCMobLockableClassDef("oCMobLockable", "oCMobInter", oCMobLockable::_CreateNewInstance,
    0, 0x260, 0, "");
zCClassDef* oCMobLockable::classDef = &oCMobLockableClassDef;
static zCClassDef oCMobContainerClassDef("oCMobContainer", "oCMobLockable",
    oCMobContainer::_CreateNewInstance, 0, 0x284, 1, "");
zCClassDef* oCMobContainer::classDef = &oCMobContainerClassDef;
static zCClassDef oCMobWheelClassDef("oCMobWheel", "oCMobInter", oCMobWheel::_CreateNewInstance,
    0, 0x234, 0, "");
zCClassDef* oCMobWheel::classDef = &oCMobWheelClassDef;
static zCClassDef oCMobLadderClassDef("oCMobLadder", "oCMobInter", oCMobLadder::_CreateNewInstance,
    0, 0x23C, 0, "");
zCClassDef* oCMobLadder::classDef = &oCMobLadderClassDef;
static zCClassDef oCMobDoorClassDef("oCMobDoor", "oCMobLockable", oCMobDoor::_CreateNewInstance,
    0, 0x274, 1, "");
zCClassDef* oCMobDoor::classDef = &oCMobDoorClassDef;
static zCClassDef oCMobFireClassDef("oCMobFire", "oCMobLockable", oCMobFire::_CreateNewInstance,
    0, 0x260, 2, "");
zCClassDef* oCMobFire::classDef = &oCMobFireClassDef;
static zCClassDef oCMobItemSlotClassDef("oCMobItemSlot", "oCMobInter",
    oCMobItemSlot::_CreateNewInstance, 0, 0x23C, 0, "");
zCClassDef* oCMobItemSlot::classDef = &oCMobItemSlotClassDef;
static zCClassDef oCDummyVobGeneratorClassDef("oCDummyVobGenerator", "oCVob",
    oCDummyVobGenerator::_CreateNewInstance, 0, 0x120, 0, "");
zCClassDef* oCDummyVobGenerator::classDef = &oCDummyVobGeneratorClassDef;

zCObject* oCMOB::_CreateNewInstance()
{
    oCMOB* mob = new oCMOB;
    zCClassDef::ObjectCreated(mob, classDef);
    return mob;
}

zCObject* oCMobInter::_CreateNewInstance()
{
    oCMobInter* mob = new oCMobInter;
    zCClassDef::ObjectCreated(mob, classDef);
    return mob;
}

zCObject* oCMobBed::_CreateNewInstance()
{
    oCMobBed* mob = new oCMobBed;
    zCClassDef::ObjectCreated(mob, classDef);
    mob->sceme = "BED";
    mob->addName = "FRONT";
    mob->state_num = 1;
    return mob;
}

zCObject* oCMobSwitch::_CreateNewInstance()
{
    oCMobSwitch* mob = new oCMobSwitch;
    zCClassDef::ObjectCreated(mob, classDef);
    mob->sceme = "LEVER";
    return mob;
}

zCObject* oCMobContainer::_CreateNewInstance()
{
    oCMobContainer* mob = new oCMobContainer;
    zCClassDef::ObjectCreated(mob, classDef);
    mob->sceme = "CHEST";
    return mob;
}

zCObject* oCMobWheel::_CreateNewInstance()
{
    oCMobWheel* mob = new oCMobWheel;
    zCClassDef::ObjectCreated(mob, classDef);
    mob->sceme = "WHEEL";
    return mob;
}

zCObject* oCMobLadder::_CreateNewInstance()
{
    oCMobLadder* mob = new oCMobLadder;
    zCClassDef::ObjectCreated(mob, classDef);
    mob->sceme = "LADDER";
    return mob;
}

void oCMOB::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

void oCMobInter::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

void oCMobBed::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

void oCMobSwitch::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

void oCMobContainer::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

void oCMobWheel::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

void oCMobLadder::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

int oCMobInter::GetState()
{
    return state;
}

int oCMobInter::GetStateNum()
{
    return state_num;
}

oCMobInter::TMobInterDirection oCMobInter::GetDirection()
{
    return Direction;
}

void oCMobInter::SetDirection(TMobInterDirection direction)
{
    Direction = direction;
}

int oCMobInter::IsOccupied()
{
    return npcsCurrent > 0;
}

int oCMobInter::IsIn(int state)
{
    return 0;
}

const zSTRING* oCMobInter::GetTriggerTarget(int index) const
{
    return index == 0 ? &triggerTarget : 0;
}

zCClassDef* oCMobBed::_GetClassDef() const
{
    return classDef;
}

zCClassDef* oCMobSwitch::_GetClassDef() const
{
    return classDef;
}

zCClassDef* oCMobLockable::_GetClassDef() const
{
    return classDef;
}

zCClassDef* oCMobContainer::_GetClassDef() const
{
    return classDef;
}

zCClassDef* oCMobWheel::_GetClassDef() const
{
    return classDef;
}

zCClassDef* oCMobLadder::_GetClassDef() const
{
    return classDef;
}

void oCMobLockable::SetLocked(int value)
{
    locked = value != 0;
}

void oCMobLockable::SetKeyInstance(const zSTRING& value)
{
    keyInstance = value;
}

void oCMobLockable::SetPickLockStr(const zSTRING& value)
{
    pickLockStr = value;
}

void oCMobLockable::OnMessage(zCEventMessage* message, zCVob* sender)
{
    oCMobMsg* mobMessage = dynamic_cast<oCMobMsg*>(message);
    if (!mobMessage)
        return;

    switch (mobMessage->subType)
    {
    case oCMobMsg::EV_STARTINTERACTION:
        StartInteraction(mobMessage->npc);
        break;
    case oCMobMsg::EV_STARTSTATECHANGE:
        StartStateChange(mobMessage->npc, mobMessage->from, mobMessage->to);
        break;
    case oCMobMsg::EV_ENDINTERACTION:
        StopInteraction(mobMessage->npc);
        break;
    case oCMobMsg::EV_CALLSCRIPT:
        CallOnStateFunc(mobMessage->npc, mobMessage->to);
        break;
    }
}

int oCMobLadder::DoFocusCheckBBox()
{
    return 1;
}

zCObject* oCMobDoor::_CreateNewInstance()
{
    oCMobDoor* mob = new oCMobDoor;
    zCClassDef::ObjectCreated(mob, classDef);
    mob->addName = "FRONT";
    mob->sceme = "DOOR";
    mob->state_num = 1;
    return mob;
}

zCObject* oCMobFire::_CreateNewInstance()
{
    oCMobFire* mob = new oCMobFire;
    zCClassDef::ObjectCreated(mob, classDef);
    mob->mobStateAni = -1;
    mob->npcStateAni = -1;
    mob->fireVobtree = 0;
    mob->fireSlot = "BIP01 FIRE";
    mob->fireVobtreeName = "FIRETREE_LARGE.ZEN";
    return mob;
}

zCObject* oCMobItemSlot::_CreateNewInstance()
{
    oCMobItemSlot* mob = new oCMobItemSlot;
    zCClassDef::ObjectCreated(mob, classDef);
    mob->insertedItem = 0;
    mob->removeable = 0;
    return mob;
}

zCObject* oCDummyVobGenerator::_CreateNewInstance()
{
    oCDummyVobGenerator* generator = new oCDummyVobGenerator;
    zCClassDef::ObjectCreated(generator, classDef);
    generator->SetCollDetStat(0);
    generator->SetCollDetDyn(0);
    generator->SetSleeping(0);
    return generator;
}

void oCMobDoor::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

void oCMobFire::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

void oCMobItemSlot::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

void oCDummyVobGenerator::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* oCMobDoor::_GetClassDef() const
{
    return classDef;
}

zCClassDef* oCMobFire::_GetClassDef() const
{
    return classDef;
}

zCClassDef* oCMobItemSlot::_GetClassDef() const
{
    return classDef;
}

zCClassDef* oCDummyVobGenerator::_GetClassDef() const
{
    return classDef;
}

void oCMobDoor::Open(oCNpc* npc)
{
}

void oCMobDoor::Close(oCNpc* npc)
{
}

oCDummyVobGenerator::~oCDummyVobGenerator()
{
}

oCMOB::oCMOB()
    : hitp(10), damage(0), isDestroyed(0), moveable(0), takeable(1), focusOverride(0),
      sndMat(oTSNDMAT_DEFAULT), owner(-1), ownerGuild(-1), focusNameIndex(-1)
{
    showVisual = 1;
    SetCollisionClass(&zCCollObjectComplex::s_oCollObjClass);
}

oCMOB::~oCMOB()
{
    ignoreVobList.DeleteList();
}

zCClassDef* oCMOB::_GetClassDef() const
{
    return classDef;
}

void oCMOB::SetVisual(zCVisual* visual)
{
    zCVob::SetVisual(visual);
    SetCollisionClass(&zCCollObjectComplex::s_oCollObjClass);
}

void oCMOB::SetName(const zSTRING& value)
{
    focusNameIndex = parser.GetIndex(value);
}

zSTRING oCMOB::GetName()
{
    zSTRING result;
    zCPar_Symbol* symbol = parser.GetSymbol(focusNameIndex);
    if (symbol)
        symbol->GetValue(result, 0);
    return result;
}

zSTRING oCMOB::GetScemeName()
{
    zSTRING result;
    if (visual)
    {
        zSTRING visualName = visual->GetVisualName();
        int separator = visualName.Search("_");
        result = visualName.Copied(0, separator);
    }
    return result;
}

void oCMOB::OnDamage(zCVob* other, zCVob* instigator, float damage, int damageType,
    const zVEC3& hitLocation)
{
}

void oCMOB::OnMessage(zCEventMessage* message, zCVob* sender)
{
}

zCModel* oCMOB::GetModel()
{
    return dynamic_cast<zCModel*>(visual);
}

void oCMOB::SetMoveable(int value)
{
    moveable = value != 0;
}

int oCMOB::IsMoveable()
{
    return moveable;
}

void oCMOB::SetSoundMaterial(oTSndMaterial material)
{
    sndMat = material;
}

oTSndMaterial oCMOB::GetSoundMaterial()
{
    return sndMat;
}

void oCMOB::GetSoundMaterial_AM(zCSoundManager::zTSndManMedium& medium,
    oTSndMaterial& material, int damageType)
{
    medium = zCSoundManager::zSND_MAN_MEDIUM_OBJECT;
    material = GetSoundMaterial();
}

void oCMOB::SetOwner(const zSTRING& ownerName, const zSTRING& guildName)
{
    ownerStr = ownerName;
    ownerGuildStr = guildName;
    owner = parser.GetIndex(ownerName);
    ownerGuild = -1;
    zCPar_Symbol* guildSymbol = parser.GetSymbol(guildName);
    if (guildSymbol)
        guildSymbol->GetValue(ownerGuild, 0);
}

void oCMOB::SetOwner(int ownerInstance, int guild)
{
    owner = ownerInstance;
    ownerGuild = guild;
}

int oCMOB::IsOwnedByGuild(int guild)
{
    return ownerGuild >= 0 && ownerGuild == guild;
}

int oCMOB::IsOwnedByNpc(int npcInstance)
{
    return owner >= 0 && owner == npcInstance;
}

int oCMOB::IsOwner(oCNpc* npc)
{
    if (IsOwnedByNpc(npc->GetInstance()))
        return 1;
    return IsOwnedByGuild(npc->GetTrueGuild());
}

void oCMOB::Hit(int damage)
{
}

void oCMOB::Destroy()
{
    if (isDestroyed)
        return;

    zsndMan->StartDestructionSound(this, GetSoundMaterial());
    zCParticleFX* particle = new zCParticleFX;
    zCClassDef::ObjectCreated(particle, zCParticleFX::classDef);
    zCVob* particleVob = new zCVob;
    zCClassDef::ObjectCreated(particleVob, zCVob::classDef);
    particleVob->SetCollDetStat(0);
    particleVob->SetCollDetDyn(0);
    zVEC3 position;
    GetPositionWorld(position.x, position.y, position.z);
    particleVob->SetPositionWorld(position);
    particleVob->SetVisual(particle);
    particle->SetEmitter("PFX_MOBDESTROY", 0);
    particle->CreateParticles();
    homeWorld->AddVob(particleVob);

    isDestroyed = 1;
    if (!visualDestroyed.IsEmpty())
    {
        SetVisual(visualDestroyed);
        UpdateVisualDependencies(1);
    }
    else if (homeWorld)
        homeWorld->RemoveVob(this);
}

void oCMOB::InsertInIgnoreList(zCVob* vob)
{
    for (zCList<zCVob>* entry = ignoreVobList.next; entry; entry = entry->next)
    {
        if (entry->data == vob)
            return;
    }
    ignoreVobList.Insert(vob);
}

void oCMOB::RemoveFromIgnoreList(zCVob* vob)
{
    zCList<zCVob>* previous = &ignoreVobList;
    for (zCList<zCVob>* entry = ignoreVobList.next; entry; entry = entry->next)
    {
        if (entry->data == vob)
        {
            previous->next = entry->next;
            entry->next = 0;
            delete entry;
            return;
        }
        previous = entry;
    }
}

int oCMOB::CanThisCollideWith(zCVob* vob)
{
    for (zCList<zCVob>* entry = ignoreVobList.next; entry; entry = entry->next)
    {
        if (entry->data == vob)
            return 0;
    }
    return 1;
}

void oCMOB::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteGroupBegin("MOB");
    archiver.WriteString(name);
    archiver.WriteInt(hitp);
    archiver.WriteInt(damage);
    archiver.WriteBool(moveable);
    archiver.WriteBool(takeable);
    archiver.WriteBool(focusOverride);
    archiver.WriteEnum(GetSoundMaterial());
    archiver.WriteString(visualDestroyed);
    archiver.WriteString(ownerStr);
    archiver.WriteString(ownerGuildStr);
    archiver.WriteGroupEnd();
    if (!archiver.InProperties())
        archiver.WriteBool(isDestroyed);
}

void oCMOB::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    name = archiver.ReadString();
    hitp = archiver.ReadInt();
    damage = archiver.ReadInt();
    moveable = archiver.ReadBool();
    takeable = archiver.ReadBool();
    focusOverride = archiver.ReadBool();
    sndMat = static_cast<oTSndMaterial>(archiver.ReadEnum());
    visualDestroyed = archiver.ReadString();
    ownerStr = archiver.ReadString();
    ownerGuildStr = archiver.ReadString();

    if (!archiver.InProperties())
    {
        isDestroyed = archiver.ReadBool();
        SetOwner(ownerStr, ownerGuildStr);
    }

    if (mobNameConv && !name.IsEmpty())
    {
        zSTRING convertedName = name;
        if (convertedName.Search("MOBNAME_") < 0)
            convertedName = zSTRING("MOBNAME_") + convertedName;
        convertedName.Upper();
        for (int i = 0; i < convertedName.Length(); ++i)
        {
            if (convertedName[i] <= ' ' || convertedName[i] == '-')
                convertedName[i] = '_';
        }
        SetName(convertedName);
    }
    else
        SetName(name);
}

int oCMOB::AllowDiscardingOfSubtree()
{
    return 1;
}

int oCMobInter::SetAllMobsToState(oCWorld* world, const zSTRING& scemeName, int targetState)
{
    int count = 0;
    TraverseMobs(&world->globalVobTree, scemeName, targetState, &count);
    return count;
}

void oCMobInter::TraverseMobs(zCTree<zCVob>* node, const zSTRING& scemeName,
    int targetState, int* count)
{
    if (!node)
        return;

    oCMobInter* mob = dynamic_cast<oCMobInter*>(node->data);
    if (mob && mob->GetScemeName() == scemeName)
    {
        mob->tmpStateChanged = mob->tmpState != targetState || mob->state != targetState;
        mob->tmpState = targetState;
        if (mob->tmpStateChanged)
        {
            if (targetState == 1 && mob->state == 0)
                mob->GetEM(0)->OnTrigger(mob);
            else if (targetState != 1 && mob->state > 0)
                mob->GetEM(0)->OnUntrigger(mob);
            mob->tmpStateChanged = 0;
            ++*count;
        }
    }

    TraverseMobs(node->firstChild, scemeName, targetState, count);
    TraverseMobs(node->next, scemeName, targetState, count);
}

void oCMobInter::TriggerAllMobsToTmpState(zCWorld* world)
{
    TraverseTriggerMobs(&world->globalVobTree);
}

void oCMobInter::TraverseTriggerMobs(zCTree<zCVob>* node)
{
    if (!node)
        return;

    oCMobInter* mob = dynamic_cast<oCMobInter*>(node->data);
    if (mob && mob->tmpStateChanged)
    {
        if (mob->tmpState == 1 && mob->state == 0)
            mob->GetEM(0)->OnTrigger(mob);
        else if (mob->tmpState == 0 && mob->state > 0)
            mob->GetEM(0)->OnUntrigger(mob);
        mob->tmpStateChanged = 0;
    }

    TraverseTriggerMobs(node->firstChild);
    TraverseTriggerMobs(node->next);
}

oCMobInter::oCMobInter()
    : state(0), state_num(1), state_target(0), rewind(0), mobStateAni(-1), npcStateAni(-1),
      npcsMax(0), npcsNeeded(0), npcsCurrent(0), tmpState(0), tmpStateChanged(0),
      Direction(MOBINTER_DIRECTION_NONE), onInterruptReturnToSlotPos(1), aniCombHeight(0.0f),
      inUseVob(0), timerEnd(0.0f)
{
    startPos = zVEC3(0.0f, 0.0f, 0.0f);
}

oCMobInter::~oCMobInter()
{
    inUseVob = 0;
    optimalPosList.DeleteListDatas();
}

zCClassDef* oCMobInter::_GetClassDef() const
{
    return classDef;
}

void oCMobInter::SetVisual(zCVisual* visual)
{
    zCVob::SetVisual(visual);
    SetCollisionClass(&zCCollObjectComplex::s_oCollObjClass);
    if (visual)
    {
        zSTRING visualName = visual->GetVisualName();
        sceme = visualName.Copied(0, visualName.Search("_"));
        SetCollisionClass(&zCCollObjectComplex::s_oCollObjClass);
        showVisual &= ~0x20;
    }
}

void oCMobInter::SetTempState(int targetState)
{
    tmpStateChanged = targetState != tmpState || targetState != state;
    tmpState = targetState;
}

int oCMobInter::IsTempStateChanged()
{
    return tmpStateChanged;
}

void oCMobInter::SetStateToTempState()
{
    if (!tmpStateChanged)
        return;

    if (tmpState == 1 && GetState() == 0)
        GetEM(0)->OnTrigger(this);
    else if (tmpState == 0 && GetState() > 0)
        GetEM(0)->OnUntrigger(this);
    tmpStateChanged = 0;
}

void oCMobInter::SetMobBodyState(oCNpc* npc)
{
    static zCPar_Symbol* symbols[4];
    static const int bodyStates[4] = { 15, 11, 12, 9 };
    if (!symbols[0])
        symbols[0] = parser.GetSymbol("MOB_NOTINTERRUPTABLE");
    if (!symbols[1])
        symbols[1] = parser.GetSymbol("MOB_SIT");
    if (!symbols[2])
        symbols[2] = parser.GetSymbol("MOB_LIE");
    if (!symbols[3])
        symbols[3] = parser.GetSymbol("MOB_CLIMB");

    for (int i = 0; i < 4; ++i)
    {
        if (!symbols[i])
            continue;

        zSTRING values;
        symbols[i]->GetValue(values, 0);
        for (int wordIndex = 1;; ++wordIndex)
        {
            zSTRING word = values.PickWord(wordIndex, " ", "");
            if (word.IsEmpty())
                break;
            if (GetScemeName().Search(word) >= 0)
            {
                npc->SetBodyState(bodyStates[i]);
                return;
            }
        }
    }
    npc->SetBodyState(16);
}

void oCMobInter::OnDamage(zCVob* other, zCVob* instigator, float damage, int damageType,
    const zVEC3& hitLocation)
{
}

void oCMobInter::OnMessage(zCEventMessage* message, zCVob* sender)
{
    oCMobMsg* mobMessage = dynamic_cast<oCMobMsg*>(message);
    if (!mobMessage)
        return;

    switch (mobMessage->subType)
    {
    case oCMobMsg::EV_STARTINTERACTION:
        StartInteraction(mobMessage->npc);
        break;
    case oCMobMsg::EV_STARTSTATECHANGE:
        StartStateChange(mobMessage->npc, mobMessage->from, mobMessage->to);
        break;
    case oCMobMsg::EV_ENDINTERACTION:
        StopInteraction(mobMessage->npc);
        break;
    case oCMobMsg::EV_CALLSCRIPT:
        CallOnStateFunc(mobMessage->npc, mobMessage->to);
        break;
    }
}

void oCMobInter::SetUseWithItem(const zSTRING& itemName)
{
    useWithItem = itemName;
}

int oCMobInter::GetUseWithItem()
{
    if (!useWithItem.IsEmpty())
        return parser.GetIndex(useWithItem);
    return -1;
}

int oCMobInter::HasUseWithItem(oCNpc* npc)
{
    int itemInstance = GetUseWithItem();
    if (itemInstance > 0 && !npc->HasInHand(itemInstance))
    {
        oCItem* interactItem = npc->interactItem;
        if (!interactItem || interactItem->GetInstance() != itemInstance)
        {
            if (!npc->IsAPlayer() || !npc->IsInInv(itemInstance, 1))
                return 0;
            npc->SetInteractItem(npc->RemoveFromInv(itemInstance, 1));
        }
    }
    return 1;
}

zSTRING oCMobInter::GetScemeName()
{
    return sceme;
}

void oCMobInter::ScanIdealPositions()
{
    if (!homeWorld)
        return;

    zVEC3 currentPosition;
    GetPositionWorld(currentPosition.n[0], currentPosition.n[1], currentPosition.n[2]);
    int positionUnchanged = startPos.n[0] == currentPosition.n[0]
        && startPos.n[1] == currentPosition.n[1] && startPos.n[2] == currentPosition.n[2];
    if (positionUnchanged && optimalPosList.GetNumInList() > 0)
        return;
    if (npcsCurrent > 0)
        return;

    startPos = currentPosition;
    optimalPosList.DeleteListDatas();
    npcsMax = 0;
    npcsNeeded = 0;

    zCModel* model = GetModel();
    if (model)
    {
        for (int i = 0; i < model->nodeList.numInArray; ++i)
        {
            zCModelNodeInst* node = model->nodeList[i];
            const zSTRING& nodeName = node->protoNode->nodeName;
            if (nodeName.Search("ZS_POS") < 0)
                continue;

            TMobOptPos* position = new TMobOptPos;
            position->trafo = GetTrafoModelNodeToWorld(node);
            position->distance = nodeName.Search("DIST") >= 0;
            position->npc = 0;
            position->nodeName = nodeName;
            if (nodeName.Search("NPC") >= 0)
            {
                ++npcsMax;
                ++npcsNeeded;
            }
            optimalPosList.Insert(position);
        }
    }

    if (!npcsMax)
    {
        npcsMax = 1;
        npcsNeeded = 1;
    }
}

int oCMobInter::GetFreePosition(oCNpc* npc, zVEC3& position)
{
    ScanIdealPositions();
    TMobOptPos* optimalPosition = SearchFreePosition(npc, 150.0f);
    if (!optimalPosition)
        return 0;

    position.n[0] = optimalPosition->trafo.m[0][3];
    position.n[1] = optimalPosition->trafo.m[1][3];
    position.n[2] = optimalPosition->trafo.m[2][3];
    return 1;
}

TMobOptPos* oCMobInter::SearchFreePosition(oCNpc* npc, float maxDistance)
{
    if (npcsCurrent >= npcsMax)
        return 0;

    TMobOptPos* bestPosition = 0;
    float bestDistance = 3.402823466e+38f;
    float maxDistanceSquared = maxDistance * maxDistance;
    int tooClose = 0;
    int blocked = 0;
    for (zCList<TMobOptPos>* entry = optimalPosList.next; entry; entry = entry->next)
    {
        TMobOptPos* position = entry->data;
        if (position->npc == npc)
            return position;
        if (position->npc)
            continue;

        float dx = npc->trafoObjToWorld.m[0][3] - position->trafo.m[0][3];
        float dy = npc->trafoObjToWorld.m[1][3] - position->trafo.m[1][3];
        float dz = npc->trafoObjToWorld.m[2][3] - position->trafo.m[2][3];
        float distanceSquared = dx * dx + dy * dy + dz * dz;
        if (distanceSquared >= maxDistanceSquared)
            continue;
        if (!position->distance && !npc->FreeLineOfSight(position->trafo)) {
            blocked = 1;
            continue;
        }
        if (distanceSquared < bestDistance) {
            bestDistance = distanceSquared;
            bestPosition = position;
        } else {
            tooClose = 1;
        }
    }
    if (!bestPosition && npc->IsAPlayer()) {
        zSTRING reason = tooClose ? PLAYER_MOB_TOO_FAR_AWAY : PLAYER_MOB_ANOTHER_IS_USING;
        oCMsgManipulate* message = new oCMsgManipulate(13, reason, -1);
        npc->GetEM(0)->OnMessage(message, npc);
    }
    return bestPosition;
}

void oCMobInter::SetHeading(oCNpc* npc)
{
    float azimuth;
    float elevation;
    npc->GetAngles(this, azimuth, elevation);
    zVEC3 axis = { 0.0f, 1.0f, 0.0f };
    npc->RotateWorld(axis, azimuth);
}

void oCMobInter::SetIdealPosition(oCNpc* npc)
{
    TMobOptPos* position = SearchFreePosition(npc, 150.0f);
    if (!position)
        return;

    int wasMoving = (npc->isInMovementMode & 3) != 0;
    if (wasMoving)
        npc->EndMovement(1);
    if (!position->distance)
    {
        npc->SetTrafoObjToWorld(position->trafo);
        zVEC3 worldPosition = { position->trafo.m[0][3], position->trafo.m[1][3],
            position->trafo.m[2][3] };
        npc->SetPositionWorld(worldPosition);
    }
    else
    {
        zVEC3 worldPosition = { position->trafo.m[0][3], position->trafo.m[1][3],
            position->trafo.m[2][3] };
        npc->SetCollDetStat(0);
        npc->SetCollDetDyn(0);
        npc->SetPositionWorld(worldPosition);
        npc->SetHeadingYWorld(this);
        npc->SetCollDetStat(1);
        npc->SetCollDetDyn(1);
    }
    position->npc = npc;
    if (wasMoving)
        npc->BeginMovement();
}

void oCMobInter::Reset()
{
    if (!rewind)
    {
        int resetState = 0;
        oCNpc* npc = dynamic_cast<oCNpc*>(inUseVob);
        if (npc && state > 0 && npc->GetModel()) {
            zSTRING aniName = "T_STAND_2_S" + zSTRING(state);
            if (npc->GetModel()->GetAniIDFromAniName(aniName) != -1)
                resetState = state;
        }
        state = resetState;
        state_target = resetState;
        if (GetModel())
            GetModel()->StartAni("S_S" + zSTRING(resetState), 0);
    }
    SetDirection(state <= 0 ? MOBINTER_DIRECTION_NONE : MOBINTER_DIRECTION_DOWN);
}

void oCMobInter::OnTick()
{
    if (rewind && npcsCurrent <= 0)
    {
        npcStateAni = -1;
        CheckStateChange(0);
        if (state > 0 && CanChangeState(0, state, state - 1))
            SendStateChange(state, state - 1);
        if (IsInState(0, 0))
            SetSleeping(1);
    }
}

void oCMobInter::OnTrigger(zCVob* other, zCVob* instigator)
{
    if (!other && !instigator && state == 0 && npcsCurrent <= 0)
    {
        state_target = 1;
        state = 1;
        if (GetModel())
            GetModel()->StartAni("S_S1", 0);
    }

    if (!triggerTarget.IsEmpty())
    {
        zCArray<zCVob*> targets;
        homeWorld->SearchVobListByName(triggerTarget, targets);
        for (int i = 0; i < targets.numInArray; ++i)
            targets[i]->GetEM(0)->OnTrigger(instigator ? instigator : this);
    }
}

void oCMobInter::OnUntrigger(zCVob* other, zCVob* instigator)
{
    if (!other && !instigator && state == 1 && npcsCurrent <= 0)
    {
        state_target = 0;
        state = 0;
        if (GetModel())
            GetModel()->StartAni("S_S0", 0);
    }

    if (!triggerTarget.IsEmpty())
    {
        zCArray<zCVob*> targets;
        homeWorld->SearchVobListByName(triggerTarget, targets);
        for (int i = 0; i < targets.numInArray; ++i)
            targets[i]->GetEM(0)->OnUntrigger(instigator);
    }
}

void oCMobInter::SendStateChange(int from, int to)
{
    int sent = 0;
    for (zCList<TMobOptPos>* entry = optimalPosList.next; entry; entry = entry->next)
    {
        oCNpc* npc = entry->data->npc;
        if (!npc)
            continue;
        oCMobMsg* message = new oCMobMsg(oCMobMsg::EV_STARTSTATECHANGE, npc);
        message->from = from;
        message->to = to;
        GetEM(0)->OnMessage(message, npc);
        sent = 1;
    }

    if (!sent)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(inUseVob);
        oCMobMsg* message = new oCMobMsg(oCMobMsg::EV_STARTSTATECHANGE, npc);
        message->from = from;
        message->to = to;
        GetEM(0)->OnMessage(message, npc);
    }
}

void oCMobInter::SendEndInteraction(oCNpc* npc, int targetState, int playAni)
{
    oCMobMsg* message = new oCMobMsg(oCMobMsg::EV_ENDINTERACTION, npc);
    message->from = targetState;
    message->playAni = playAni;
    npc->GetEM(0)->OnMessage(message, this);
}

void oCMobInter::Interact(oCNpc* npc, int apply, int up, int down, int keyState, int keyToggle)
{
    CheckStateChange(npc);
    if (!IsInteractingWith(npc) || state < 0 || npcsCurrent < npcsNeeded)
        return;
    if (npc->IsAPlayer() && !npc->IsMovLock())
        return;

    if (apply && !up && !down)
    {
        if (Direction == MOBINTER_DIRECTION_NONE && state > 0 && state < state_num
            && CanChangeState(npc, state, state + 1))
        {
            SendStateChange(state, state + 1);
            SetDirection(MOBINTER_DIRECTION_UP);
            return;
        }
        if (state == state_num && Direction == MOBINTER_DIRECTION_UP
            && CanChangeState(npc, state, state - 1))
        {
            SendStateChange(state, state - 1);
            SetDirection(MOBINTER_DIRECTION_DOWN);
            return;
        }
    }

    if (npc->inventory2.IsOpen())
        return;

    if (up)
    {
        if (state != state_num && CanChangeState(npc, state, state + 1))
        {
            SendStateChange(state, state + 1);
            SetDirection(MOBINTER_DIRECTION_UP);
            return;
        }
        if (state == state_num && CanChangeState(npc, state, -1))
        {
            EndInteraction(npc, 1);
            return;
        }
    }

    if (down || (!up && Direction == MOBINTER_DIRECTION_DOWN))
    {
        if (state == 0 && CanChangeState(npc, state, -1))
        {
            EndInteraction(npc, 1);
            return;
        }
        if (CanChangeState(npc, state, state - 1))
        {
            SendStateChange(state, state - 1);
            SetDirection(MOBINTER_DIRECTION_DOWN);
        }
    }
    else if (Direction == MOBINTER_DIRECTION_UP && CanChangeState(npc, state, state + 1))
        SendStateChange(state, state + 1);
}

void oCMobInter::GetTransitionNames(int from, int to, zSTRING& mobAni, zSTRING& npcAni)
{
    if (to == -1)
    {
        mobAni = zSTRING();
        npcAni = zSTRING("T_") + GetScemeName() + "_S" + zSTRING(from) + "_2_STAND";
        return;
    }

    mobAni = zSTRING("T_S") + zSTRING(from) + "_2_S" + zSTRING(to);
    npcAni = zSTRING("T_") + GetScemeName() + "_S" + zSTRING(from) + "_2_S" + zSTRING(to);
}

int oCMobInter::IsInState(oCNpc* npc, int targetState)
{
    if (!npc)
        npcStateAni = -1;

    if (state != targetState || state_target != targetState)
        return 0;

    zCModel* model = GetModel();
    if (mobStateAni != -1)
    {
        zCModelAni* ani = model ? model->GetAniFromAniID(mobStateAni) : 0;
        if (!ani || !model->IsAniActive(ani))
            return 0;
    }

    if (npcStateAni == -1)
        return 1;
    zCModel* npcModel = npc ? npc->GetModel() : 0;
    zCModelAni* ani = npcModel ? npcModel->GetAniFromAniID(npcStateAni) : 0;
    return ani && npcModel->IsAniActive(ani);
}

int oCMobInter::CanChangeState(oCNpc* npc, int from, int to)
{
    if (!IsInState(npc, from))
        return 0;
    if (npc && npc->IsAPlayer() && from == 0 && to > 0 && GetUseWithItem() > 0
        && !HasUseWithItem(npc))
        return 0;

    zSTRING mobAni;
    zSTRING npcAni;
    GetTransitionNames(from, to, mobAni, npcAni);
    zCModel* model = GetModel();
    if (model && model->GetAniIDFromAniName(mobAni) != -1)
        return 1;
    if (!npc)
        return 0;
    zCModel* npcModel = npc->GetModel();
    return npcModel && npcModel->GetAniIDFromAniName(npcAni) != -1;
}

void oCMobInter::StartStateChange(oCNpc* npc, int from, int to)
{
    zSTRING mobAni;
    zSTRING npcAni;
    GetTransitionNames(from, to, mobAni, npcAni);
    mobStateAni = -1;

    zCModel* model = GetModel();
    if (model)
    {
        mobStateAni = model->GetAniIDFromAniName(mobAni);
        if (mobStateAni != -1 && !model->IsAniActive(mobStateAni))
            model->StartAni(mobStateAni, 0);
    }

    npcStateAni = 0;
    if (npc)
        StartTransitionAniNpc(npc, npcAni);

    state_target = to;
    SetDirection(MOBINTER_DIRECTION_NONE);
    if (from >= 0 && from <= state_num && to >= 0 && to <= state_num && from != to)
        SetDirection(from < to ? MOBINTER_DIRECTION_UP : MOBINTER_DIRECTION_DOWN);
    OnBeginStateChange(npc, from, to);
}

void oCMobInter::StartTransitionAniNpc(oCNpc* npc, const zSTRING& aniName)
{
    zCModel* model = npc->GetModel();
    int aniID = model->GetAniIDFromAniName(aniName);
    npcStateAni = aniID;
    if (aniID == -1)
        return;

    zCModelAni* ani = model->GetAniFromAniID(aniID);
    if (!model->IsAniActive(ani))
        model->StartAni(aniID, 0);
    if (aniName.Search("2_STAND") > 0)
        npc->GetAnictrl()->SetNextAni(aniID, npc->GetAnictrl()->_s_walk);
}

void oCMobInter::CheckStateChange(oCNpc* npc)
{
    if (!npc)
        npcStateAni = -1;
    else if (npc->GetAnictrl()->IsStanding())
    {
        state = state_target;
        EndInteraction(npc, 0);
        return;
    }

    if (state != state_target && IsInState(npc, state))
    {
        if (npc)
            OnEndStateChange(npc, state, state_target);
        state = state_target;
    }
}

void oCMobInter::CallOnStateFunc(oCNpc* npc, int targetState)
{
    zSTRING functionName = onStateFuncName + "_S" + zSTRING(targetState);
    int function = parser.GetIndex(functionName);
    if (function < 0)
        return;

    parser.SetInstance("SELF", npc);
    parser.SetInstance("ITEM", npc ? npc->interactItem : 0);
    parser.CallFunc(function);
}

void oCMobInter::SendCallOnStateFunc(oCNpc* npc, int targetState)
{
    if (onStateFuncName.IsEmpty())
        return;

    oCMobMsg* message = new oCMobMsg(oCMobMsg::EV_CALLSCRIPT, npc);
    message->to = targetState;
    GetEM(0)->OnMessage(message, this);
}

void oCMobInter::OnBeginStateChange(oCNpc* npc, int from, int to)
{
}

void oCMobInter::OnEndStateChange(oCNpc* npc, int from, int to)
{
    if (to == 1)
        OnTrigger(npc, this);
    else if (to == 0)
        OnUntrigger(npc, this);

    if (from < to || (from == 1 && to == 0))
        SendCallOnStateFunc(npc, to);
}

void oCMobInter::InterruptInteraction(oCNpc* npc)
{
    for (zCList<TMobOptPos>* entry = optimalPosList.next; entry; entry = entry->next)
    {
        if (entry->data->npc == npc)
            entry->data->npc = 0;
    }

    zCModel* model = npc->GetModel();
    if (model && model->numActiveAnis > 1)
        model->StopAnisLayerRange(2, 256);
    if (npc->interactItem)
    {
        npc->PutInInv(npc->interactItem);
        npc->SetInteractItem(0);
    }
    npc->SetInteractMob(0);
    Reset();
    inUseVob = 0;
    npc->ResetXZRotationsWorld();
    if (model)
        model->RemoveAllVobFX();
    if (onInterruptReturnToSlotPos & 1) {
        if (npc->FreeLineOfSight(trafoObjToWorld)) {
            zVEC3 position;
            if (npc->SearchNpcPosition(position))
                npc->SetPositionWorld(position);
        }
    }
    npc->SetCollDetStat(1);
    npc->SetCollDetDyn(1);

    --npcsCurrent;
    if (npcsCurrent <= 0)
    {
        showVisual &= ~0x20;
        if (!rewind)
            SetSleeping(1);
    }
    RemoveFromIgnoreList(npc);
}

int oCMobInter::IsMultiMob()
{
    return npcsNeeded > 1;
}

int oCMobInter::IsAvailable(oCNpc* npc)
{
    if (!npc->IsAPlayer())
    {
        if (inUseVob && inUseVob != npc && ztimer.totalTimeFloat < timerEnd)
            return 0;
        inUseVob = 0;
    }
    return 1;
}

void oCMobInter::MarkAsUsed(float time, zCVob* vob)
{
    inUseVob = vob;
    timerEnd = ztimer.totalTimeFloat + time;
}

int oCMobInter::CanInteractWith(oCNpc* npc)
{
    if (isDestroyed || (showVisual & 8) || !IsAvailable(npc))
        return 0;
    if (npcsCurrent > 0) {
        if (npc->IsAPlayer()) {
            oCMsgManipulate* message = new oCMsgManipulate(13, PLAYER_MOB_ANOTHER_IS_USING, -1);
            npc->GetEM(0)->OnMessage(message, npc);
        }
        return 0;
    }

    ScanIdealPositions();
    if (!SearchFreePosition(npc, 150.0f) || !HasUseWithItem(npc))
        return 0;

    if (!conditionFunc.IsEmpty())
    {
        parser.SetInstance("SELF", npc);
        if (!parser.CallFunc(parser.GetIndex(conditionFunc)))
            return 0;
    }
    return 1;
}

int oCMobInter::IsInteractingWith(oCNpc* npc)
{
    for (zCList<TMobOptPos>* entry = optimalPosList.next; entry; entry = entry->next)
    {
        if (entry->data->npc == npc)
            return 1;
    }
    return 0;
}

void oCMobInter::StartInteraction(oCNpc* npc)
{
    if (IsInteractingWith(npc))
        return;

    mobStateAni = -1;
    npcStateAni = -1;
    npc->SetInteractMob(this);
    showVisual |= 0x20;
    npc->GetAnictrl()->StopTurnAnis();
    ScanIdealPositions();
    SetIdealPosition(npc);

    zCModel* model = npc->GetModel();
    if (model)
    {
        model->StartAni(zSTRING("T_") + GetScemeName() + "_STAND_2_S" + zSTRING(state), 0);
        npcStateAni = model->GetAniIDFromAniName(zSTRING("S_") + GetScemeName()
            + "_S" + zSTRING(state));
    }

    SetPhysicsEnabled(0);
    SetSleeping(0);
    SetMobBodyState(npc);
    ++npcsCurrent;
    if (state == state_num)
        SetDirection(MOBINTER_DIRECTION_DOWN);
    else if (state >= 0)
        SetDirection(MOBINTER_DIRECTION_UP);
}

void oCMobInter::EndInteraction(oCNpc* npc, int endMode)
{
    if (!endMode)
    {
        oCMobMsg* message = new oCMobMsg(oCMobMsg::EV_ENDINTERACTION, npc);
        message->from = state;
        message->to = -1;
        GetEM(0)->OnMessage(message, npc);
    }
    else
    {
        zCModel* model = npc->GetModel();
        if (model && !model->IsAniActive(npc->GetAnictrl()->_s_walk))
        {
            StartTransitionAniNpc(npc, zSTRING("T_") + GetScemeName() + "_S"
                + zSTRING(state) + "_2_STAND");
            inUseVob = 0;
        }
    }
    SetDirection(MOBINTER_DIRECTION_NONE);
}

void oCMobInter::StopInteraction(oCNpc* npc)
{
    if (!IsInteractingWith(npc))
        return;

    if (!npc->GetAnictrl()->IsStanding())
        StartTransitionAniNpc(npc, zSTRING("T_") + GetScemeName() + "_S"
            + zSTRING(state) + "_2_STAND");

    for (zCList<TMobOptPos>* entry = optimalPosList.next; entry; entry = entry->next)
    {
        if (entry->data->npc == npc)
            entry->data->npc = 0;
    }

    npc->ResetXZRotationsWorld();
    zCModel* model = npc->GetModel();
    if (model)
        model->RemoveAllVobFX();
    if (!rewind)
        SetSleeping(1);
    if (npc->interactItem)
    {
        npc->PutInInv(npc->interactItem);
        npc->SetInteractItem(0);
    }
    npc->SetInteractMob(0);
    npc->SetFocusVob(0);
    npc->CheckPutbackTorch();
    --npcsCurrent;
    if (npcsCurrent <= 0)
        showVisual &= ~0x20;
    RemoveFromIgnoreList(npc);
    inUseVob = 0;
}

int oCMobInter::AI_UseMobToState(oCNpc* npc, int targetState)
{
    if (!IsInteractingWith(npc))
    {
        if (!CanInteractWith(npc) || (targetState <= 0 && (targetState != 0 || state <= 0)))
            return 0;
        GetEM(0)->OnMessage(new oCMobMsg(oCMobMsg::EV_STARTINTERACTION, npc), npc);
        return 0;
    }

    if (targetState >= state_num)
        targetState = state_num;

    if (targetState == -1)
    {
        if (npc->GetAnictrl()->IsStanding() && (state == 0 || state == state_num))
        {
            oCMobMsg* message = new oCMobMsg(oCMobMsg::EV_ENDINTERACTION, npc);
            message->from = state;
            message->to = -1;
            GetEM(0)->OnMessage(message, npc);
            return 1;
        }
        if (state > 0 && IsInState(npc, state) && CanChangeState(npc, state, -1))
        {
            oCMobMsg* message = new oCMobMsg(oCMobMsg::EV_ENDINTERACTION, npc);
            message->from = state;
            message->to = -1;
            GetEM(0)->OnMessage(message, npc);
            return 0;
        }
    }

    CheckStateChange(npc);
    if (IsInState(npc, targetState))
        return 1;
    Interact(npc, state != 0 || targetState != -1, state < targetState,
        state > targetState, 0, 0);
    return 0;
}

void oCMobInter::Archive(zCArchiver& archiver)
{
    oCMOB::Archive(archiver);
    if (!archiver.InProperties())
        archiver.WriteInt(state_num);
    archiver.WriteString(triggerTarget);
    archiver.WriteString(useWithItem);
    archiver.WriteString(conditionFunc);
    archiver.WriteString(onStateFuncName);
    archiver.WriteBool(rewind);
}

void oCMobInter::Unarchive(zCArchiver& archiver)
{
    oCMOB::Unarchive(archiver);
    if (!archiver.InProperties())
        state_num = archiver.ReadInt();
    triggerTarget = archiver.ReadString();
    useWithItem = archiver.ReadString();
    conditionFunc = archiver.ReadString();
    onStateFuncName = archiver.ReadString();
    rewind = archiver.ReadBool();
    Reset();
}

oCMobFire::oCMobFire()
    : fireSlot("BIP01 FIRE"), fireVobtreeName("FIRETREE_LARGE.ZEN"), fireVobtree(0)
{
    mobStateAni = -1;
    npcStateAni = -1;
}

oCMobFire::~oCMobFire()
{
    DeleteEffects();
}

void oCMobFire::DeleteEffects()
{
    if (!fireVobtree)
        return;

    if (fireVobtree->homeWorld)
        fireVobtree->homeWorld->RemoveVobSubtree(fireVobtree);
    fireVobtree->Release();
    fireVobtree = 0;
}

void oCMobFire::OnDamage(zCVob* other, zCVob* instigator, float damage, int damageType,
    const zVEC3& hitLocation)
{
    if (damage > 0.0f && (damageType & 8))
        GetEM(0)->OnTrigger(other);
}

void oCMobFire::OnEndStateChange(oCNpc* npc, int from, int to)
{
    if (to == 1)
        OnTrigger(npc, this);
    else if (to == 0)
        OnUntrigger(npc, this);

    if (from < to || (from == 1 && to == 0))
        SendCallOnStateFunc(npc, to);
    if (to == 1)
        OnTrigger(npc, npc);
    else if (to == 0)
        OnUntrigger(npc, npc);
}

void oCMobFire::OnTrigger(zCVob* other, zCVob* instigator)
{
    if (state)
        return;

    state = 1;
    state_target = 1;
    if (!homeWorld)
        return;
    int wasMoving = other && (other->isInMovementMode & 3) != 0;
    if (wasMoving)
        other->EndMovement(1);

    if (!fireVobtree)
    {
        zoptions->ChangeDir(zTOptionPaths_DIR_WORLD);
        if (fireVobtreeName.IsEmpty())
            fireVobtreeName = "FIRETREE_SMALL.ZEN";
        fireVobtree = homeWorld->MergeVobSubtree(fireVobtreeName, 0, 0);
        if (fireVobtree) {
            fireVobtree->AddRefVobSubtree();
            if (fireVobtree->homeWorld)
                fireVobtree->homeWorld->RemoveVobSubtree(fireVobtree);
        }
    }

    if (fireVobtree)
    {
        zMAT4 trafo = GetTrafoModelNodeToWorld(fireSlot);
        zVEC3 position = { trafo.m[0][3], trafo.m[1][3], trafo.m[2][3] };
        fireVobtree->SetPositionWorld(position);
        if (!fireVobtree->homeWorld)
            homeWorld->AddVobAsChild(fireVobtree, this);
        fireVobtree->AddRef();
        fireVobtree->ReleaseVobSubtree();
    }
    if (wasMoving)
        other->BeginMovement();
}

void oCMobFire::OnUntrigger(zCVob* other, zCVob* instigator)
{
    if (state != 1)
        return;
    DeleteEffects();
    state = 0;
    state_target = 0;
}

void oCMobFire::PreSave()
{
    if (GetState() == 1)
        DeleteEffects();
}

void oCMobFire::Archive(zCArchiver& archiver)
{
    oCMobInter::Archive(archiver);
    archiver.WriteGroupBegin("FIRE");
    archiver.WriteString(fireSlot);
    archiver.WriteString(fireVobtreeName);
    archiver.WriteGroupEnd();
}

void oCMobFire::PostSave()
{
    if (GetState() != 1)
        return;
    state = 0;
    OnTrigger(this, this);
}

void oCMobFire::Unarchive(zCArchiver& archiver)
{
    oCMobInter::Unarchive(archiver);
    fireSlot = archiver.ReadString();
    fireVobtreeName = archiver.ReadString();
}

oCMobBed::oCMobBed()
    : addName("FRONT")
{
    sceme = "BED";
    state_num = 1;
}

oCMobBed::~oCMobBed()
{
}

void oCMobBed::StartInteraction(oCNpc* npc)
{
    oCMobInter::StartInteraction(npc);
}

void oCMobBed::OnBeginStateChange(oCNpc* npc, int from, int to)
{
}

void oCMobBed::OnEndStateChange(oCNpc* npc, int from, int to)
{
    oCMobInter::OnEndStateChange(npc, from, to);
}

zSTRING oCMobBed::GetScemeName()
{
    return sceme + "_" + addName;
}

TMobOptPos* oCMobBed::SearchFreePosition(oCNpc* npc, float maxDistance)
{
    TMobOptPos* position = oCMobInter::SearchFreePosition(npc, maxDistance);
    if (position)
        addName = position->nodeName.Search("FRONT") > 0 ? "FRONT" : "BACK";
    return position;
}

oCMobSwitch::oCMobSwitch()
{
    sceme = "LEVER";
    state_num = 1;
}

oCMobSwitch::~oCMobSwitch()
{
}

void oCMobSwitch::Archive(zCArchiver& archiver)
{
    oCMobInter::Archive(archiver);
}

void oCMobSwitch::Unarchive(zCArchiver& archiver)
{
    oCMobInter::Unarchive(archiver);
}

oCMobLockable::oCMobLockable()
    : locked(0), autoOpen(0), pickLockNr(0)
{
}

oCMobLockable::~oCMobLockable()
{
}

int oCMobLockable::CanInteractWith(oCNpc* npc)
{
    return oCMobInter::CanInteractWith(npc) && CanOpen(npc);
}

void oCMobLockable::Interact(oCNpc* npc, int apply, int up, int down, int keyState, int keyToggle)
{
    if (locked && (up || down))
    {
        Unlock(npc, down ? 'R' : 'L');
        return;
    }
    oCMobInter::Interact(npc, apply, up, down, keyState, keyToggle);
}

int oCMobLockable::CanChangeState(oCNpc* npc, int from, int to)
{
    if (!oCMobInter::CanChangeState(npc, from, to))
        return 0;
    return from != 0 || to != 1 || !locked;
}

void oCMobLockable::OnEndStateChange(oCNpc* npc, int from, int to)
{
    oCMobInter::OnEndStateChange(npc, from, to);
    if (to == 1)
        Open(npc);
    else if (to == 0)
        Close(npc);
}

int oCMobLockable::CanOpen(oCNpc* npc)
{
    if (!locked)
        return 1;
    if (!keyInstance.IsEmpty() && npc->IsInInv(keyInstance, 1))
        return 1;
    if (!pickLockStr.IsEmpty() && npc->IsInInv("ITKE_LOCKPICK", 1) && npc->HasTalent(5))
        return 1;
    zSTRING reason;
    if (keyInstance.IsEmpty() && pickLockStr.IsEmpty())
        reason = PLAYER_MOB_NEVER_OPEN;
    else if (keyInstance.IsEmpty())
        reason = PLAYER_MOB_MISSING_LOCKPICK;
    else if (pickLockStr.IsEmpty())
        reason = PLAYER_MOB_MISSING_KEY;
    else
        reason = PLAYER_MOB_MISSING_KEY_OR_LOCKPICK;
    oCMsgManipulate* message = new oCMsgManipulate(13, reason, -1);
    npc->GetEM(1)->OnMessage(message, this);
    return 0;
}

int oCMobLockable::PickLock(oCNpc* npc, char direction)
{
    if (pickLockStr.IsEmpty() || pickLockNr >= pickLockStr.Length())
        return 0;

    int result = pickLockStr[pickLockNr] == direction;
    if (result)
        ++pickLockNr;
    else
        pickLockNr = 0;

    zCModel* model = npc->GetModel();
    if (model)
        model->StartAni(zSTRING("T_") + GetScemeName()
            + (direction == 'L' ? "_S0_PICKLEFT" : "_S0_PICKRIGHT"), 0);
    return result;
}

void oCMobLockable::Unlock(oCNpc* npc, int playAni)
{
    if (!locked)
        return;

    int hasKey = !keyInstance.IsEmpty() && npc->IsInInv(keyInstance, 1);
    int canPick = !pickLockStr.IsEmpty() && npc->IsInInv("ITKE_LOCKPICK", 1)
        && npc->HasTalent(5);
    if (!hasKey && !canPick)
    {
        parser.CallFunc("G_NOKEY");
        return;
    }

    if (playAni)
    {
        if (GetModel())
            GetModel()->StartAni(hasKey ? "T_S0_UNLOCK" : "T_S0_TRY", 0);
        zCModel* model = npc->GetModel();
        if (model)
            model->StartAni(zSTRING("T_") + GetScemeName()
                + (hasKey ? "_S0_UNLOCK" : "_S0_TRY"), 0);
    }
    locked = 0;
}

void oCMobLockable::Lock(oCNpc* npc)
{
    if (locked || keyInstance.IsEmpty() || !npc->IsInInv(keyInstance, 1))
        return;

    if (GetModel())
        GetModel()->StartAni("T_S0_LOCK", 0);
    zCModel* model = npc->GetModel();
    if (model)
        model->StartAni(zSTRING("T_") + GetScemeName() + "_S0_LOCK", 0);
    locked = 1;
}

void oCMobLockable::Archive(zCArchiver& archiver)
{
    oCMobInter::Archive(archiver);
    archiver.WriteGroupBegin("LOCKABLE");
    archiver.WriteBool(locked);
    archiver.WriteString(keyInstance);
    archiver.WriteString(pickLockStr);
    archiver.WriteGroupEnd();
}

void oCMobLockable::Unarchive(zCArchiver& archiver)
{
    oCMobInter::Unarchive(archiver);
    locked = archiver.ReadBool();
    keyInstance = archiver.ReadString();
    pickLockStr = archiver.ReadString();
}

oCMobContainer::oCMobContainer()
    : items(0)
{
    sceme = "CHEST";
    state_num = 1;
    containList.compare_func = 0;
    containList.data = 0;
    containList.next = 0;
}

oCMobContainer::~oCMobContainer()
{
    zCListSort<oCItem>* entry = containList.next;
    while (entry)
    {
        zCListSort<oCItem>* next = entry->next;
        if (entry->data)
        {
            if (ogame && ogame->GetWorld())
                ogame->GetWorld()->RemoveVob(entry->data);
            entry->data->Release();
        }
        delete entry;
        entry = next;
    }
    containList.next = 0;
}

void oCMobContainer::OnMessage(zCEventMessage* message, zCVob* sender)
{
    oCMobMsg* mobMessage = dynamic_cast<oCMobMsg*>(message);
    if (mobMessage)
    {
        if (mobMessage->subType == oCMobMsg::EV_UNLOCK)
            Unlock(mobMessage->npc, mobMessage->playAni);
        else if (mobMessage->subType == oCMobMsg::EV_LOCK)
            Lock(mobMessage->npc);
    }
    oCMobLockable::OnMessage(message, sender);
}

void oCMobContainer::Destroy()
{
    oCMOB::Destroy();
    zCListSort<oCItem>* entry = containList.next;
    while (entry)
    {
        zCListSort<oCItem>* next = entry->next;
        oCItem* item = entry->data;
        if (item)
        {
            zVEC3 position = { trafoObjToWorld.m[0][3], trafoObjToWorld.m[1][3],
                trafoObjToWorld.m[2][3] };
            item->SetCollDetStat(0);
            item->SetCollDetDyn(0);
            item->SetPositionWorld(position);
            ogame->GetWorld()->AddVob(item);
            if (item->HasEnoughSpace(position)) {
                item->SetPositionWorld(position);
                item->SetCollDetStat(1);
                item->SetCollDetDyn(1);
                item->SetPhysicsEnabled(1);
                item->SetSleeping(0);
                item->GetRigidBody()->gravityOn = 1;
                item->GetRigidBody()->SetVelocity(zVEC3(
                    (float)(rand() % 300), (float)(rand() % 300), (float)(rand() % 300)));
            }
            item->Release();
        }
        delete entry;
        entry = next;
    }
    containList.next = 0;
}

void oCMobContainer::Reset()
{
    zCModel* model = GetModel();
    if (state == 1 && model)
    {
        if (model->numActiveAnis > 1)
            model->StopAnisLayerRange(2, 256);
        model->StartAni("T_S1_2_S0", 0);
    }
    state = 0;
    state_target = 0;
    SetDirection(MOBINTER_DIRECTION_NONE);
}

void oCMobContainer::Insert(oCItem* item)
{
    containList.Insert(item);
    item->AddRef();
    ogame->GetWorld()->RemoveVobFromLists(item);
}

void oCMobContainer::Remove(oCItem* item)
{
    zCListSort<oCItem>* previous = &containList;
    for (zCListSort<oCItem>* entry = containList.next; entry; entry = entry->next)
    {
        if (entry->data == item)
        {
            previous->next = entry->next;
            delete entry;
            break;
        }
        previous = entry;
    }
    ogame->GetWorld()->RemoveVobFromLists(item);
    item->Release();
}

oCItem* oCMobContainer::Remove(oCItem* item, int amount)
{
    for (zCListSort<oCItem>* entry = containList.next; entry; entry = entry->next)
    {
        if (entry->data != item)
            continue;
        if (item->amount <= amount)
        {
            Remove(item);
            return item;
        }

        item->amount -= amount;
        oCItem* result = (oCItem*)ogame->GetGameWorld()->CreateVob(zVOB_TYPE_ITEM,
            item->GetInstance());
        result->amount = amount;
        ogame->GetWorld()->RemoveVobFromLists(result);
        result->Release();
        return result;
    }
    return 0;
}

void oCMobContainer::CreateContents(const zSTRING& contents)
{
    for (int wordIndex = 1;; wordIndex += 2)
    {
        zSTRING itemName = contents.PickWord(wordIndex, ";", "");
        if (itemName.IsEmpty())
            break;
        zSTRING amountString = contents.PickWord(wordIndex + 1, ";", "");
        long amount = amountString.ToInt();
        if (amount <= 0)
            amount = 1;
        int itemInstance = parser.GetIndex(itemName);
        if (itemInstance >= 0 && parser.MatchClass(itemInstance, oCItem::classDef->scriptClassName))
            Insert(new oCItem(itemName, amount));
    }
}

int oCMobContainer::IsIn(int instance)
{
    int result = 0;
    for (zCListSort<oCItem>* entry = containList.next; entry; entry = entry->next)
    {
        if (entry->data->GetInstance() == instance)
            result += entry->data->amount;
    }
    return result;
}

void oCMobContainer::Open(oCNpc* npc)
{
    if (locked || !npc->IsAPlayer())
        return;

    npc->ModifyBodyState(0x8000, 0);
    if (!items)
    {
        items = new oCItemContainer;
        zSTRING title = GetName();
        items->SetName(title);
        items->SetContents(reinterpret_cast<zCList<oCItem>*>(&containList));
        items->Open(0, 0, 1);
    }
    npc->OpenInventory(0);
    items->Activate();
}

void oCMobContainer::Close(oCNpc* npc)
{
    npc->SetMovLock(0);
    npc->CloseInventory();
    delete items;
    items = 0;
}

void oCMobContainer::Archive(zCArchiver& archiver)
{
    oCMobLockable::Archive(archiver);
    archiver.WriteGroupBegin("CONTAINER");
    archiver.WriteString(contains);
    archiver.WriteGroupEnd();
    if (!archiver.InSaveGame())
        return;

    archiver.WriteInt(containList.GetNumInList());
    for (zCListSort<oCItem>* entry = containList.next; entry; entry = entry->next)
        archiver.WriteObject(entry->data);
}

void oCMobContainer::Unarchive(zCArchiver& archiver)
{
    oCMobLockable::Unarchive(archiver);
    contains = archiver.ReadString();
    pickLockStr.Upper();
    if (!archiver.InSaveGame())
    {
        CreateContents(contains);
        return;
    }

    containList.DeleteListDatas();
    containList.next = 0;
    int count = archiver.ReadInt();
    for (int i = 0; i < count; ++i)
    {
        oCItem* item = dynamic_cast<oCItem*>(archiver.ReadObject(0));
        if (item)
        {
            containList.Insert(item);
            item->AddRef();
        }
        if (ogame && ogame->GetWorld())
            ogame->GetWorld()->RemoveVobFromLists(item);
        if (item)
            item->Release();
    }
}

oCMobDoor::oCMobDoor()
    : addName("FRONT")
{
    sceme = "DOOR";
    state_num = 1;
}

oCMobDoor::~oCMobDoor()
{
}

zSTRING oCMobDoor::GetScemeName()
{
    return sceme + "_" + addName;
}

TMobOptPos* oCMobDoor::SearchFreePosition(oCNpc* npc, float maxDistance)
{
    TMobOptPos* position = oCMobInter::SearchFreePosition(npc, maxDistance);
    if (position)
        addName = position->nodeName.Search("FRONT") > 0 ? "FRONT" : "BACK";
    return position;
}

oCMobWheel::oCMobWheel()
{
    sceme = "WHEEL";
    state_num = 16;
    onInterruptReturnToSlotPos = 0;
}

oCMobWheel::~oCMobWheel()
{
}

void oCMobWheel::OnTrigger(zCVob* other, zCVob* instigator)
{
    oCMobInter::OnTrigger(other, instigator);
    if (npcsCurrent <= 0)
    {
        state_target = 1;
        state = 1;
        if (GetModel())
            GetModel()->StartAni("S_S1", 0);
    }
}

void oCMobWheel::OnUntrigger(zCVob* other, zCVob* instigator)
{
    oCMobInter::OnUntrigger(other, instigator);
    if (npcsCurrent <= 0)
    {
        state_target = 0;
        state = 0;
        if (GetModel())
            GetModel()->StartAni("S_S0", 0);
    }
}

void oCMobWheel::Interact(oCNpc* npc, int apply, int up, int down, int keyState, int keyToggle)
{
    oCMobInter::Interact(npc, apply, up, down, keyState, keyToggle);
}

oCMobLadder::oCMobLadder()
    : Interacting(0), PrevAction(0)
{
    sceme = "LADDER";
    state_num = 0;
    onInterruptReturnToSlotPos = 0;
}

oCMobLadder::~oCMobLadder()
{
}

void oCMobLadder::StartInteraction(oCNpc* npc)
{
    if (visual)
    {
        zSTRING visualName = visual->GetVisualName();
        int separator = visualName.Search("_");
        state_num = visualName.Copied(separator + 1, visualName.Length()).ToInt() - 1;
    }
    if (state_num <= 0)
    {
        zTBBox3D box = GetBBox3DLocal();
        state_num = static_cast<int>((box.maxs.n[1] - box.mins.n[1]) / 100.0f);
    }

    oCMobInter::StartInteraction(npc);
    if (!Interacting)
    {
        Interacting = 1;
        PrevAction = 1;
        SetDirection(MOBINTER_DIRECTION_NONE);
    }
}

int oCMobLadder::CanChangeState(oCNpc* npc, int from, int to)
{
    if (!oCMobInter::CanChangeState(npc, from, to))
        return 0;
    return to != -1 || state == 0 || state == state_num;
}

int oCMobLadder::CanInteractWith(oCNpc* npc)
{
    return oCMobInter::CanInteractWith(npc);
}

TMobOptPos* oCMobLadder::SearchFreePosition(oCNpc* npc, float maxDistance)
{
    TMobOptPos* position = oCMobInter::SearchFreePosition(npc, maxDistance);
    if (position)
    {
        zTBBox3D box = GetBBox3DLocal();
        float middle = (box.maxs.n[1] + box.mins.n[1]) * 0.5f;
        state = position->trafo.m[1][3] < middle ? 0 : state_num;
        state_target = state;
    }
    return position;
}

void oCMobLadder::Interact(oCNpc* npc, int apply, int up, int down, int keyState, int keyToggle)
{
    PrevAction = apply;
    CheckStateChange(npc);
    if (!IsInteractingWith(npc))
        return;

    Interacting = 1;
    if (apply && state >= 0 && state <= state_num && !CanChangeState(npc, state, -1)) {
        npc->Interrupt(0, 1);
        npc->SetPhysicsEnabled(1);
        npc->GetAnictrl()->fallDownStartY = npc->GetAnictrl()->centerPos.n[1];
        npc->GetRigidBody()->gravityOn = 1;
        npc->GetRigidBody()->SetVelocity(zVEC3(
            -npc->trafoObjToWorld.m[0][2],
            -npc->trafoObjToWorld.m[1][2],
            -npc->trafoObjToWorld.m[2][2]));
        npc->GetModel()->StartAni("S_FALLDN", 0);
        EndInteraction(npc, 0);
        return;
    }
    if (up && state < state_num && CanChangeState(npc, state, state + 1))
    {
        SetDirection(MOBINTER_DIRECTION_UP);
        SendStateChange(state, state + 1);
    }
    else if (down && state > 0 && CanChangeState(npc, state, state - 1))
    {
        SetDirection(MOBINTER_DIRECTION_DOWN);
        SendStateChange(state, state - 1);
    }
    else if ((keyState || keyToggle || (up && state == state_num)
        || (down && state == 0)) && CanChangeState(npc, state, -1))
    {
        SendStateChange(state, -1);
        EndInteraction(npc, 1);
    }
}

void oCMobLadder::EndInteraction(oCNpc* npc, int endMode)
{
    Interacting = 0;
    PrevAction = 0;
    oCMobInter::EndInteraction(npc, endMode);
}

oCMobItemSlot::oCMobItemSlot()
    : insertedItem(0), removeable(0)
{
}

oCMobItemSlot::~oCMobItemSlot()
{
    insertedItem = 0;
}

int oCMobItemSlot::GetUseWithItem()
{
    if (insertedItem)
        return -1;
    return oCMobInter::GetUseWithItem();
}

int oCMobItemSlot::CanInteractWith(oCNpc* npc)
{
    return oCMobInter::CanInteractWith(npc) != 0;
}

oCItem* oCMobItemSlot::GetInsertedItem()
{
    return insertedItem;
}

int oCMobItemSlot::PlaceItem(oCItem* item)
{
    if (!item || insertedItem || GetUseWithItem() != item->GetInstance())
        return 0;

    zCModel* model = GetModel();
    zCModelPrototype* prototype = model->modelProtoList[0];
    for (int i = 0; i < prototype->nodeList.numInArray; ++i)
    {
        zCModelNode* node = prototype->nodeList[i];
        if (node->nodeName.Search("ZS_SLOT") < 0)
            continue;

        item->SetCollDetStat(0);
        item->SetCollDetDyn(0);
        if (item->homeWorld)
            item->homeWorld->RemoveVob(item);
        zVEC3 origin = { 0.0f, 0.0f, 0.0f };
        item->SetPositionWorld(origin);
        homeWorld->AddVob(item);
        item->SetTrafoObjToWorld(GetTrafoModelNodeToWorld(node->nodeName));
        item->SetPhysicsEnabled(0);
        item->showVisual |= 0x20;
        item->SetSleeping(1);
        insertedItem = item;
        return 1;
    }
    return 0;
}

oCItem* oCMobItemSlot::RemoveItem()
{
    oCItem* item = insertedItem;
    insertedItem = 0;
    if (item && item->homeWorld)
        item->homeWorld->RemoveVob(item);
    return item;
}

int oCMobItemSlot::IsIn(int instance)
{
    return insertedItem && insertedItem->GetInstance() == instance;
}

void oCMobItemSlot::Archive(zCArchiver& archiver)
{
    oCMobInter::Archive(archiver);
    archiver.WriteBool(removeable);
    if (!archiver.InProperties())
        archiver.WriteObject(insertedItem);
}

void oCMobItemSlot::Unarchive(zCArchiver& archiver)
{
    oCMobInter::Unarchive(archiver);
    removeable = archiver.ReadBool();
    if (!archiver.InProperties())
        insertedItem = dynamic_cast<oCItem*>(archiver.ReadObject(0));
}

oCDummyVobGenerator::oCDummyVobGenerator()
{
    SetCollDetStat(0);
    SetCollDetDyn(0);
    SetSleeping(0);
}

void oCDummyVobGenerator::OnTrigger(zCVob* other, zCVob* instigator)
{
    zSTRING arrowName("Arrow");
    oCItem* arrow = new oCItem(arrowName, 1);
    arrow->SetAI(new oCAIArrow);
    arrow->SetSleeping(0);
    arrow->SetVobName("Pfeil von Dummy-Generator");
    homeWorld->AddVob(arrow);
    zMAT4 trafo = trafoObjToWorld;
    trafo.PostRotateY(90.0f);
    arrow->SetTrafoObjToWorld(trafo);
}
