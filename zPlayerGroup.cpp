#include "include/oGame.h"
#include "include/oObjFactory.h"
#include "include/zObject.h"
#include "include/zNetHost.h"

static int playerGroupState = -1;
static zSTRING playerGroupBroadcast = "broadcast";
static zSTRING playerGroupLocalhost = "127.0.0.1";
zCPlayerGroup zCPlayerGroup::allPlayerGroup("allPlayerGroup");
zCList<zCPlayerGroup> zCPlayerGroup::group_list;

void zCPlayerGroup::AddGroupToList()
{
    for (zCList<zCPlayerGroup>* entry = group_list.next; entry; entry = entry->next)
    {
        if (entry->data == this)
        {
            zerr->Report(zERR_TYPE_WARN, 0,
                "B: (zCPlayerGroup::AddGroupToList) Group is already in list", 0, 0, 96,
                "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zPlayerGroup.cpp", NULL);
            return;
        }
    }
    group_list.Insert(this);
}

void zCPlayerGroup::RemoveGroupFromList()
{
    if (!IsMember(this))
        zerr->Report(zERR_TYPE_FAULT, 0,
            "B: (zCPlayerGroup::RemoveGroupFromList) Group is not in list", 0, 0, 103,
            "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zPlayerGroup.cpp", NULL);
    else
        group_list.Remove(this);
}

zCPlayerGroup::zCPlayerGroup(const zSTRING& groupName)
    : name(groupName)
{
    AddGroupToList();
}

zCPlayerGroup::~zCPlayerGroup()
{
    ResetPlayerList();
    RemoveGroupFromList();
}

zCPlayerInfo* zCPlayerGroup::GetFirstPlayer(int& index)
{
    index = 0;
    return playerList.next ? playerList.next->data : NULL;
}

zCPlayerInfo* zCPlayerGroup::GetNextPlayer(int& index)
{
    ++index;
    int current = 0;
    for (zCList<zCPlayerInfo>* entry = playerList.next; entry; entry = entry->next)
    {
        if (current == index)
            return entry->data;
        ++current;
    }
    return NULL;
}

zCPlayerGroup* zCPlayerGroup::GetFirstGroup(int& index)
{
    index = 0;
    return group_list.next ? group_list.next->data : NULL;
}

zCPlayerGroup* zCPlayerGroup::GetNextGroup(int& index)
{
    ++index;
    int current = 0;
    for (zCList<zCPlayerGroup>* entry = group_list.next; entry; entry = entry->next)
    {
        if (current == index)
            return entry->data;
        ++current;
    }
    return NULL;
}

int zCPlayerGroup::AddPlayer(zCPlayerInfo* player)
{
    if (IsMember(player))
        return 0;
    playerList.Insert(player);
    return 0;
}

int zCPlayerGroup::RemPlayer(zCPlayerInfo* player)
{
    if (!IsMember(player))
        return 0;
    playerList.Remove(player);
    return 1;
}

int zCPlayerGroup::IsMember(zCPlayerInfo* player)
{
    for (zCList<zCPlayerInfo>* entry = playerList.next; entry; entry = entry->next)
    {
        if (entry->data == player)
            return 1;
    }
    return 0;
}

int zCPlayerGroup::IsMember(zCPlayerGroup* group)
{
    for (zCList<zCPlayerGroup>* entry = group_list.next; entry; entry = entry->next)
    {
        if (entry->data == group)
            return 1;
    }
    return 0;
}

zCPlayerInfo* zCPlayerGroup::GetPlayerByID(int id)
{
    for (zCList<zCPlayerInfo>* entry = playerList.next; entry; entry = entry->next)
    {
        if (entry->data->GetID() == id)
            return entry->data;
    }
    return NULL;
}

void zCPlayerGroup::ShowDebugInfo()
{
    zerr->Report(zERR_TYPE_INFO, 0, "B: PGROUP: PlayerGroup \"" + name + "\"", 1, 1, 195,
        "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zPlayerGroup.cpp", NULL);

    int count = 0;
    for (zCList<zCPlayerInfo>* entry = playerList.next; entry; entry = entry->next)
        ++count;
    zerr->Report(zERR_TYPE_INFO, 0, "B: PGROUP: " + zSTRING(count) + " players", 1, 1, 196,
        "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zPlayerGroup.cpp", NULL);

    for (zCList<zCPlayerInfo>* entry = playerList.next; entry; entry = entry->next)
    {
        zCPlayerInfo* player = entry->data;
        zSTRING host = "no host";
        if (player->netHost)
            host = player->netHost->GetHostname() + " (" + zSTRING((int)player->netHost->id) + ")";
        zerr->Report(zERR_TYPE_INFO, 0,
            "B: PGROUP: Player: \"" + player->GetName() + "\" (" + host + ") id=" +
                zSTRING((int)player->GetID()),
            1, 1, 203, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zPlayerGroup.cpp", NULL);
    }
    zerr->Report(zERR_TYPE_INFO, 0, "", 2, 0, 207,
        "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zPlayerGroup.cpp", NULL);
}

void zCPlayerGroup::ResetPlayerList()
{
    while (playerList.next)
    {
        zCPlayerInfo* player = playerList.next->data;
        playerList.Remove(player);
        delete player;
    }
}

void zCPlayerGroup::ResetGroupList()
{
    while (group_list.next)
        delete group_list.next->data;
    group_list.DeleteList();
}

zCPlayerInfo* zCPlayerGroup::GetNearestPlayerToVob(zCVob* vob, zCPlayerInfo* exclude)
{
    zCPlayerInfo* nearest = NULL;
    float nearestDistance = 3.402823466e+38f;

    for (zCList<zCPlayerInfo>* entry = playerList.next; entry; entry = entry->next)
    {
        zCPlayerInfo* player = entry->data;
        if (!player->hostVob || player == exclude)
            continue;

        float distance = player->hostVob->GetDistanceToVob2(*vob);
        if (distance <= nearestDistance)
        {
            nearestDistance = distance;
            nearest = player;
        }
    }
    return nearest;
}
