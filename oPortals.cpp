#include "include/oPortals.h"

oCPortalRoom::oCPortalRoom()
    : ownerGuild(-1)
{
}

oCPortalRoom::oCPortalRoom(const zSTRING& name)
    : portalName(name), ownerGuild(-1)
{
    portalName.Upper();
}

oCPortalRoom::~oCPortalRoom()
{
}

oCNpc* oCPortalRoom::GetOwnerNpc()
{
    if (ownerNpc.IsEmpty() || !ogame)
        return NULL;
    return zDYNAMIC_CAST<oCNpc>(ogame->GetWorld()->SearchVobByName(ownerNpc));
}

int oCPortalRoom::GetOwnerGuild()
{
    return ownerGuild;
}

zSTRING& oCPortalRoom::GetPortalName()
{
    return portalName;
}

int oCPortalRoom::IsPortalMyRoom(oCNpc* npc)
{
    if (!ownerNpc.IsEmpty() && ownerNpc == zSTRING(npc->GetInstance()))
        return 1;
    if (ownerGuild < 0)
        return 0;
    return ownerGuild == npc->GetTrueGuild();
}

void oCPortalRoom::SavePortalData(zCArchiver& archiver)
{
    archiver.WriteString(portalName);
    archiver.WriteString(ownerNpc);
    archiver.WriteInt(ownerGuild);
}

void oCPortalRoom::LoadPortalData(zCArchiver& archiver)
{
    portalName = archiver.ReadString();
    ownerNpc = archiver.ReadString();
    ownerGuild = archiver.ReadInt();
}

int oCPortalRoomManager::SortPortals(const void* first, const void* second)
{
    const oCPortalRoom* firstRoom = *(const oCPortalRoom**)first;
    const oCPortalRoom* secondRoom = *(const oCPortalRoom**)second;
    if (firstRoom->portalName == secondRoom->portalName)
        return 1;
    return firstRoom->portalName < secondRoom->portalName ? -1 : 1;
}

oCPortalRoomManager::oCPortalRoomManager()
    : oldPlayerPortal(0), curPlayerPortal(0), oldPlayerRoom(0), curPlayerRoom(0)
{
    portals.Compare = SortPortals;
}

oCPortalRoomManager::~oCPortalRoomManager()
{
    CleanUp();
}

void oCPortalRoomManager::CleanUp()
{
    oldPlayerPortal = NULL;
    curPlayerPortal = NULL;
    oldPlayerRoom = NULL;
    curPlayerRoom = NULL;
    for (int i = 0; i < portals.numInArray; ++i)
        delete portals[i];
    portals.numInArray = 0;
}

void oCPortalRoomManager::AssignPortalToNpc(const zSTRING& portal, const zSTRING& npc)
{
    oCPortalRoom* room = new oCPortalRoom(portal);
    room->ownerNpc = npc;
    portals.InsertSort(room);
}

void oCPortalRoomManager::AssignPortalToGuild(const zSTRING& portal, int guild)
{
    oCPortalRoom* room = new oCPortalRoom(portal);
    room->ownerGuild = guild;
    portals.InsertSort(room);
}

int oCPortalRoomManager::GetPortalRoomIndex(const zSTRING& portal)
{
    if (portal.IsEmpty())
        return -1;
    for (int i = 0; i < portals.numInArray; ++i)
    {
        if (portals[i]->portalName == portal)
            return i;
    }
    return -1;
}

int oCPortalRoomManager::IsPlayerInMyRoom(oCNpc* npc)
{
    if (!oCNpc::player)
        return 0;
    curPlayerPortal = oCNpc::player->GetSectorNameVobIsIn();
    return curPlayerPortal && IsPortalMyRoom(*curPlayerPortal, npc);
}

int oCPortalRoomManager::IsNSCInPlayerRoom(oCNpc* npc)
{
    if (!oCNpc::player || !npc)
        return 0;
    return npc->GetSectorNameVobIsIn() == oCNpc::player->GetSectorNameVobIsIn();
}

int oCPortalRoomManager::WasPlayerInMyRoom(oCNpc* npc)
{
    if (!oCNpc::player || !oldPlayerPortal)
        return 0;
    return IsPortalMyRoom(*oldPlayerPortal, npc);
}

int oCPortalRoomManager::IsPortalMyRoom(const zSTRING& portal, oCNpc* npc)
{
    int index = GetPortalRoomIndex(portal);
    return index >= 0 && portals[index]->IsPortalMyRoom(npc);
}

oCNpc* oCPortalRoomManager::GetCurPlayerPortalRoomOwner()
{
    return curPlayerRoom ? curPlayerRoom->GetOwnerNpc() : NULL;
}

int oCPortalRoomManager::GetCurPlayerPortalRoomGuild()
{
    return curPlayerRoom ? curPlayerRoom->ownerGuild : 0;
}

oCNpc* oCPortalRoomManager::GetFormerPlayerPortalRoomOwner()
{
    return oldPlayerRoom ? oldPlayerRoom->GetOwnerNpc() : NULL;
}

int oCPortalRoomManager::GetFormerPlayerPortalRoomGuild()
{
    return oldPlayerRoom ? oldPlayerRoom->ownerGuild : 0;
}

int oCPortalRoomManager::HasPlayerChangedPortalRoom()
{
    zSTRING* sector = oCNpc::player->GetSectorNameVobIsIn();
    if (sector == curPlayerPortal)
        return 0;
    oldPlayerPortal = curPlayerPortal;
    oldPlayerRoom = curPlayerRoom;
    curPlayerPortal = sector;
    curPlayerRoom = NULL;
    if (sector)
    {
        int index = GetPortalRoomIndex(*sector);
        if (index >= 0)
            curPlayerRoom = portals[index];
    }
    return 1;
}

oCNpc* oCPortalRoomManager::GetCurNpcPortalRoomOwner(oCNpc* npc)
{
    if (!npc)
        return NULL;
    zSTRING* sector = npc->GetSectorNameVobIsIn();
    if (!sector)
        return NULL;
    int index = GetPortalRoomIndex(*sector);
    return index >= 0 ? portals[index]->GetOwnerNpc() : NULL;
}

int oCPortalRoomManager::GetCurNpcPortalRoomGuild(oCNpc* npc)
{
    if (!npc)
        return -1;
    zSTRING* sector = npc->GetSectorNameVobIsIn();
    if (!sector)
        return -1;
    int index = GetPortalRoomIndex(*sector);
    return index >= 0 ? portals[index]->ownerGuild : -1;
}

void oCPortalRoomManager::ShowDebugInfo()
{
    zSTRING current = "CurPortal : ";
    if (curPlayerRoom)
    {
        current += curPlayerRoom->portalName;
        current += "/Guild:" + zSTRING(curPlayerRoom->ownerGuild);
        if (curPlayerRoom->ownerGuild > 0)
            current += "/" + ogame->GetGuilds()->GetGuildName(curPlayerRoom->ownerGuild);
    }
    screen->Print(0, 0, current);
    zSTRING former = "FormerPortal : ";
    if (oldPlayerRoom)
    {
        former += oldPlayerRoom->portalName;
        former += "/Guild:" + zSTRING(oldPlayerRoom->ownerGuild);
        if (oldPlayerRoom->ownerGuild > 0)
            former += "/" + ogame->GetGuilds()->GetGuildName(oldPlayerRoom->ownerGuild);
    }
    screen->Print(0, screen->FontY(), former);
}

void oCPortalRoomManager::ShowPortalInfo()
{
    for (int i = 0; i < portals.numInArray; ++i)
        zerr.Message("U: PRT: - " + portals[i]->portalName);
    if (oldPlayerPortal)
        zerr.Message("U: PRT: Player was in Portal: " + *oldPlayerPortal);
    if (curPlayerPortal)
        zerr.Message("U: PRT: Player in in Portal: " + *curPlayerPortal);
}

void oCPortalRoomManager::SavePortalData(zCArchiver& archiver)
{
    archiver.WriteInt(portals.numInArray);
    for (int i = 0; i < portals.numInArray; ++i)
        portals[i]->SavePortalData(archiver);
}

void oCPortalRoomManager::LoadPortalData(zCArchiver& archiver)
{
    CleanUp();
    int count = archiver.ReadInt();
    for (int i = 0; i < count; ++i)
    {
        oCPortalRoom* room = new oCPortalRoom;
        room->LoadPortalData(archiver);
        portals.InsertSort(room);
    }
}
