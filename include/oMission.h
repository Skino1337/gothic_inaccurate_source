#ifndef OMISSION_H
#define OMISSION_H

#include "oCollisionObjectMiscChar.h"

class oCMission {
public:

    struct TMisStatus {
        unsigned long vobID;
        int status;
        float startTime;
    };

    oCMission(int instance);
    ~oCMission();
    void AddEntry(const zSTRING& entry);
    void SetCurrentUser(oCNpc* npc, oCNpc* player);
    void GetCurrentUser(oCNpc*& npc, oCNpc*& player);
    TMisStatus* GetStatusInfo();
    float GetStartTime() const;
    void SetStartTime(float time);
    int CheckMission();
    int IsAvailable() const;
    int OfferConditions();
    void Offer();
    int SuccessConditions();
    void Success();
    int FailureConditions();
    void Failure();
    int ObsoleteConditions();
    void Obsolete();
    void Running();
    int GetStatus() const;
    void SetStatus(int status);
    int OnTime() const;
    virtual void Archive(zCArchiver& archiver);
    virtual oCMission* Unarchive(zCArchiver& archiver);

    zSTRING name;
    zSTRING description;
    int duration;
    int important;
    int offerConditions;
    int offerFunc;
    int successConditions;
    int successFunc;
    int failureConditions;
    int failureFunc;
    int obsoleteConditions;
    int obsoleteFunc;
    int runningFunc;
    int instance;
    oCNpc* npc;
    oCNpc* player;
    unsigned long curID;
    int available;
    zCArray<TMisStatus*> statusList;
    TMisStatus* status;
    int deleted;
};

class oCMissionManager {
public:

    oCMissionManager();
    ~oCMissionManager();
    void ClearMissions();
    static int SortMissions(const void* first, const void* second);
    oCMission* GetMission(int instance);
    void CreateAllMissions();
    oCMission* OfferThisMission(oCNpc* npc, oCNpc* player, int instance);
    oCMission* GetAvailableMission(oCNpc* npc, oCNpc* player, int instance);
    int GetStatus(oCNpc* player, int instance);
    void SetStatus(oCNpc* player, int instance, int status);
    int CancelMission(oCNpc* player, int instance);
    void CreateMissionInfoList(oCNpc* player, zCArray<oCMission*>& missions);
    int TestNextMission();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    zCArraySort<oCMission*> missionList;
};

#endif
