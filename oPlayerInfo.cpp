#include "include/oObjFactory.h"

static zCArray<oCPlayerInfo*> playerInfoList;
static int playerInstanceState = -1;
static zSTRING playerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING playerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING playerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING playerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING playerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING playerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING playerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING playerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING playerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING playerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
const zSTRING PLAYER_INSTANCE = "-";
static zSTRING playerInstanceHero = "PC_HERO";
static zSTRING playerInstanceFighter = "PC_FIGHTER";
static zSTRING playerInstanceThief = "PC_THIEF";
static zSTRING playerInstanceMage = "PC_MAGE";
static zSTRING playerInstancePsionic = "PC_PSIONIC";
const zSTRING PLAYER_INSTANCE_NAME = "-";
static zSTRING playerDisplayHero = "Hero";
static zSTRING playerDisplayFighter = "Fighter";
static zSTRING playerDisplayThief = "Thief";
static zSTRING playerDisplayMage = "Mage";
static zSTRING playerDisplayPsionic = "Psionic";

static const char* playerInstanceNames[] = {
    "PC_UNKNOWN", "PC_HERO", "PC_FIGHTER", "PC_THIEF", "PC_MAGE", "PC_PSIONIC"
};

void zCPlayerInfo::SetName(const zSTRING& value)
{
    name = value;
}

oCPlayerInfo::oCPlayerInfo()
    : instance(PC_HERO)
{
}

void oCPlayerInfo::Init(const zSTRING& playerName, const zSTRING& world)
{
    zCPlayerInfo::Init(playerName, world);
}

void oCPlayerInfo::SetInstance(TPlayerInstance value)
{
    instance = value;
}

void oCPlayerInfo::SetInstanceByName(const zSTRING& value)
{
    for (int i = 0; i < 6; ++i) {
        if (value == playerInstanceNames[i]) {
            SetInstance((TPlayerInstance)i);
            return;
        }
    }
    zerr->Warning(zSTRING("B: (oCPlayerInfo::SetInstanceByName) Could not find instance with name ") + value);
    SetInstance(PC_UNKNOWN);
}

zSTRING oCPlayerInfo::GetInstanceName() const
{
    return playerInstanceNames[instance];
}

TPlayerInstance oCPlayerInfo::GetInstance() const
{
    return instance;
}

void oCPlayerInfo::Pack(zCBuffer& buffer)
{
    zCPlayerInfo::Pack(buffer);
    buffer.Write(&instance, sizeof(instance));
}

void oCPlayerInfo::Unpack(zCBuffer& buffer)
{
    zCPlayerInfo::Unpack(buffer);
    buffer.Read(&instance, sizeof(instance));
}

void oCPlayerInfo::Reset()
{
    zCPlayerInfo::Reset();
}
