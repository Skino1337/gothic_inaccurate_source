#include "include/zWorldInfo.h"
#include "include/oGame.h"
#include "include/oObjFactory.h"
#include "include/zObject.h"

int zCPlayerGroup::IsMember(zCPlayerInfo* player)
{
    for (zCList<zCPlayerInfo>* entry = playerList.next; entry; entry = entry->next)
    {
        if (entry->data == player)
            return 1;
    }
    return 0;
}

void zCPlayerGroup::ShowDebugInfo()
{
    zerr->Message(name);
}

zCWorldInfo::zCWorldInfo(const zSTRING& worldName)
    : isRunning(0), name(worldName), playerGroup_Level(NULL)
{
    historyList.data = NULL;
    historyList.next = NULL;
    playerGroup_Level = new zCPlayerGroup("World_" + worldName);
}

zCWorldInfo::~zCWorldInfo()
{
    delete playerGroup_Level;
    playerGroup_Level = NULL;
    historyList.DeleteList();
}

zSTRING zCWorldInfo::GetName()
{
    return name;
}

zCPlayerGroup* zCWorldInfo::GetPlayerGroup()
{
    return playerGroup_Level;
}

int zCWorldInfo::IsMember(zCPlayerInfo* player)
{
    return playerGroup_Level->IsMember(player);
}

zCPlayerInfo* zCWorldInfo::GetLevelMaster()
{
    zCPlayerInfo* master = playerGroup_Level->GetFirstPlayer();
    if (master)
        return master;
    return historyList.next ? historyList.next->data : NULL;
}

void zCWorldInfo::EnterWorld(zCPlayerInfo* player)
{
    historyList.Remove(player);
    playerGroup_Level->AddPlayer(player);
    player->worldInfo = this;
    ShowDebugInfo();
}

void zCWorldInfo::LeaveWorld(zCPlayerInfo* player)
{
    int found = 0;
    for (zCList<zCPlayerInfo>* entry = historyList.next; entry; entry = entry->next)
    {
        if (entry->data == player)
        {
            found = 1;
            break;
        }
    }
    if (!found)
        historyList.Insert(player);
    playerGroup_Level->RemPlayer(player);
    player->worldInfo = NULL;
    ShowDebugInfo();
}

void zCWorldInfo::ShowDebugInfo()
{
    zerr->Separator("");
    zerr->Report(zERR_TYPE_INFO, 0, "B: LEVEL: " + GetName(), 0, 0, 139,
        "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zWorldInfo.cpp", NULL);
    playerGroup_Level->ShowDebugInfo();
    zerr->Report(zERR_TYPE_INFO, 0, "B:   History-List", 0, 0, 143,
        "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zWorldInfo.cpp", NULL);

    for (zCList<zCPlayerInfo>* entry = historyList.next; entry; entry = entry->next)
    {
        zCPlayerInfo* player = entry->data;
        zerr->Report(zERR_TYPE_INFO, 0,
            "B:   ... Player: \"" + player->GetName() + "\" id=" +
                zSTRING((int)player->GetID()),
            0, 0, 148, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zWorldInfo.cpp", NULL);
    }
    zerr->Separator("");
}
