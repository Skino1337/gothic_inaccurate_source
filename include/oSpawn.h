#ifndef OSAPWN_H
#define OSAPWN_H

#include "oCollisionObjectMiscChar.h"

class oCSpawnManager {
public:

    struct oSSpawnNode {
        oCNpc* npc;
        zVEC3 spawnPos;
        float timer;
    };
    typedef oSSpawnNode oTSpawnNode;

    oCSpawnManager();
    ~oCSpawnManager();
    void ClearList();
    void InsertInList(oSSpawnNode* node);
    void RemoveFromList(oSSpawnNode* node);
    int IsSpawningEnabled();
    void SetSpawningEnabled(int enabled);
    void SpawnImmediately(int immediately);
    void CheckInsertNpcs();
    void ShowDebugInfo();
    void CheckInsertNpc();
    void InitCameraPos();
    int InsertNpc(oCNpc* npc, const zVEC3& position);
    oCNpc* SummonNpc(int instance, const zVEC3& position, float angle);
    oCNpc* SpawnNpc(int instance, const zSTRING& waypoint, float angle);
    void SpawnNpc(oCNpc* npc, const zSTRING& waypoint, float angle);
    void SpawnNpc(oCNpc* npc, const zVEC3& position, float angle);
    void CheckForInvalidDialogCamPos(float angle, oCNpc* npc);
    int UseDeadNpcRoutinePos(oCNpc* npc);
    int CanDeadNpcBeRemoved(oCNpc* npc);
    int CheckRemoveNpc(oCNpc* npc);
    void DeleteNpc(oCNpc* npc);
    void DeleteAllSummoned();
    void Archive(zCArchiver& archiver);
    void Unarchive(zCArchiver& archiver);

    static void SetInsertRange(float range);
    static float GetInsertRange();
    static void SetRemoveRange(float range);
    static float GetRemoveRange();
    static void SetSpawnTime(float time);
    static float GetSpawnTime();

    zCArray<oTSpawnNode*> spawnList;
    int spawningEnabled;
    zVEC3 camPos;
    float insertTime;
    int spawnFlags;
};

extern float SPAWN_INSERTTIME_MAX;
extern float SPAWN_INSERTRANGE;
extern float SPAWN_REMOVERANGE;

#endif
