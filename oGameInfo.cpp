#include "include/oGameInfo.h"
#include "include/oGame.h"
#include "include/zNetHost.h"

static int gameInfoState = -1;
static int gameInfoNetworkState = -1;
static zSTRING gameInfoBroadcastHostName = "broadcast";
static zSTRING gameInfoLocalhostAddress = "127.0.0.1";
static zSTRING infoPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING infoPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING infoPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING infoPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING infoPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING infoPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING infoPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING infoPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING infoPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING infoPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";

void zCGameInfo::SetName(const zSTRING& value)
{
    name = value;
}

zSTRING zCGameInfo::GetName() const
{
    return name;
}

void zCGameInfo::Init()
{
    zCPlayerGroup::allPlayerGroup.ResetPlayerList();
}

void zCGameInfo::Reset()
{
    Init();
}

int zCGameInfo::AddPlayer(zCPlayerInfo* player)
{
    return zCPlayerGroup::allPlayerGroup.AddPlayer(player);
}

int zCGameInfo::RemPlayer(zCPlayerInfo* player)
{
    return zCPlayerGroup::allPlayerGroup.RemPlayer(player);
}

zCPlayerInfo* zCGameInfo::GetPlayerByID(int id)
{
    return zCPlayerGroup::allPlayerGroup.GetPlayerByID(id);
}

zCPlayerInfo* zCGameInfo::GetPlayerByVobID(unsigned long id)
{
    for (zCList<zCPlayerInfo>* entry = zCPlayerGroup::allPlayerGroup.playerList.next;
        entry; entry = entry->next)
    {
        if (id == 1)
            return entry->data;
    }

    return NULL;
}

void zCGameInfo::PackToBuffer(zCBuffer& buffer, unsigned char flags)
{
    Pack(buffer, flags);
}

zCGameInfo* zCGameInfo::CreateFromBuffer(zCBuffer& buffer)
{
    zCGameInfo* gameInfo = new oCGameInfo;
    gameInfo->Unpack(buffer);
    return gameInfo;
}

int zCGameInfo::GetNumPlayers()
{
    int count = 0;

    for (zCList<zCPlayerInfo>* entry = zCPlayerGroup::allPlayerGroup.playerList.next;
        entry; entry = entry->next)
        ++count;

    return count;
}

void zCGameInfo::Pack(zCBuffer& buffer, unsigned char flags)
{
    buffer.WriteByte(flags, 8);

    if (flags & 1)
        buffer.WriteString(GetName());

    if (flags & 2)
    {
        buffer.WriteWord(GetNumPlayers(), 16);

        for (zCList<zCPlayerInfo>* entry = zCPlayerGroup::allPlayerGroup.playerList.next;
            entry; entry = entry->next)
            entry->data->PackToBuffer(buffer);
    }
}

void zCGameInfo::Unpack(zCBuffer& buffer)
{
    zCPlayerGroup::allPlayerGroup.ResetPlayerList();
    unsigned char flags = buffer.ReadByte(8);

    if (flags & 1)
        name = buffer.ReadString();

    if (flags & 2)
    {
        int numPlayers = buffer.ReadWord(16);

        for (int i = 0; i < numPlayers; ++i)
            AddPlayer(zCPlayerInfo::CreateFromBuffer(buffer));
    }
}

zCPlayerInfo* zCGameInfo::GetPlayerByNetAddress(zTNetAddress& address)
{
    for (zCList<zCPlayerInfo>* entry = zCPlayerGroup::allPlayerGroup.playerList.next;
        entry; entry = entry->next)
    {
        if (entry->data->netHost->HasAddress(address))
            return entry->data;
    }

    return NULL;
}

oCGameInfo::oCGameInfo()
{
}

oCGameInfo::~oCGameInfo()
{
}

void oCGameInfo::Init()
{
    zCGameInfo::Init();
    SetName("Gothic II");
}

void oCGameInfo::Pack(zCBuffer& buffer, unsigned char flags)
{
    zCGameInfo::Pack(buffer, flags);
}

void oCGameInfo::Unpack(zCBuffer& buffer)
{
    zCGameInfo::Unpack(buffer);
}
