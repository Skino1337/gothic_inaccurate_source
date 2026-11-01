#ifndef OPORTALS_H
#define OPORTALS_H

#include "oCollisionObjectMiscChar.h"

class oCPortalRoom {
public:

    oCPortalRoom();
    oCPortalRoom(const zSTRING& name);
    ~oCPortalRoom();
    oCNpc* GetOwnerNpc();
    int GetOwnerGuild();
    zSTRING& GetPortalName();
    int IsPortalMyRoom(oCNpc* npc);
    void SavePortalData(zCArchiver& archiver);
    void LoadPortalData(zCArchiver& archiver);

    zSTRING portalName;
    zSTRING ownerNpc;
    int ownerGuild;
};

class oCPortalRoomManager {
public:

    oCPortalRoomManager();
    ~oCPortalRoomManager();
    void CleanUp();
    void AssignPortalToNpc(const zSTRING& portal, const zSTRING& npc);
    void AssignPortalToGuild(const zSTRING& portal, int guild);
    int GetPortalRoomIndex(const zSTRING& portal);
    int IsPlayerInMyRoom(oCNpc* npc);
    int IsNSCInPlayerRoom(oCNpc* npc);
    int WasPlayerInMyRoom(oCNpc* npc);
    int IsPortalMyRoom(const zSTRING& portal, oCNpc* npc);
    oCNpc* GetCurPlayerPortalRoomOwner();
    int GetCurPlayerPortalRoomGuild();
    oCNpc* GetFormerPlayerPortalRoomOwner();
    int GetFormerPlayerPortalRoomGuild();
    int HasPlayerChangedPortalRoom();
    oCNpc* GetCurNpcPortalRoomOwner(oCNpc* npc);
    int GetCurNpcPortalRoomGuild(oCNpc* npc);
    void ShowDebugInfo();
    void ShowPortalInfo();
    void SavePortalData(zCArchiver& archiver);
    void LoadPortalData(zCArchiver& archiver);

    static int SortPortals(const void* first, const void* second);

    zSTRING* oldPlayerPortal;
    zSTRING* curPlayerPortal;
    oCPortalRoom* oldPlayerRoom;
    oCPortalRoom* curPlayerRoom;
    zCArraySort<oCPortalRoom*> portals;
};

#endif
