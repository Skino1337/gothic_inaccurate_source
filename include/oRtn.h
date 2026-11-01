#ifndef ORTN_H
#define ORTN_H

#include "zVob.h"

class oCRtnCutscene {
public:

    oCRtnCutscene();
    virtual ~oCRtnCutscene();
    void ResetPlayState();

    oCNpc* npc;
    zSTRING csName;
    zSTRING roleName;
    int ignorePlayState;
    int active;
};

class oCRtnEntry {
public:

    oCRtnEntry();
    oCRtnEntry(int hour1, int min1, int hour2, int min2, int script, const zSTRING& waypoint,
        int instance);
    ~oCRtnEntry();
    void GetTime(int& hour1, int& min1, int& hour2, int& min2);
    void SetTime(int hour1, int min1, int hour2, int min2);
    int GetState();
    zSTRING GetWaypoint();

    int hour1;
    int min1;
    int hour2;
    int min2;
    int f_script;
    zSTRING wpname;
    int instance;
    zCVob* poi;
    oCNpc* npc;
    oCRtnCutscene* cutscene;
    int overlay;
};

template <class T>
class zCListSort {
public:

    typedef int (*TCompare)(const void*, const void*);

    zCListSort()
        : data(0), next(0), compare_func(0) {}

    ~zCListSort()
    {
        delete next;
        next = 0;
        data = 0;
    }

    void InsertSort(T* value)
    {
        zCListSort* entry = new zCListSort;
        entry->data = value;
        if (!next || (compare_func && compare_func(&value, &next->data) < 0))
        {
            entry->next = next;
            next = entry;
            return;
        }
        zCListSort* current = next;
        while (current->next &&
            (!compare_func || compare_func(&value, &current->next->data) >= 0))
            current = current->next;
        entry->next = current->next;
        current->next = entry;
    }

    void DeleteListDatas()
    {
        delete data;
        data = 0;
        if (next)
        {
            next->DeleteListDatas();
            delete next;
            next = 0;
        }
    }

    void Remove(T* value)
    {
        zCListSort* previous = this;
        while (previous->next)
        {
            if (previous->next->data == value)
            {
                zCListSort* entry = previous->next;
                previous->next = entry->next;
                entry->next = 0;
                delete entry;
                return;
            }
            previous = previous->next;
        }
    }

    T* data;
    zCListSort* next;
    TCompare compare_func;
};

class oCRtnManager {
public:

    struct TRtn_WayBoxLimit {
        int begin;
        zTBBox3D bbox;
        float value;
        oCNpc* npc;
    };

    struct TRtn_WayBox {
        zTBBox3D bbox;
        oCNpc* npc;
        int found;
    };

    oCRtnManager();
    ~oCRtnManager();
    void SetWorld(zCWorld* world);
    void SetDailyRoutinePos(int position);
    void ShowRoutine(int x, int y, oCNpc* npc);
    zSTRING GetRoutineString(oCRtnEntry* entry);
    void Insert(oCNpc* npc, oCRtnEntry* entry);
    void UpdateGlobalRoutineEntry();
    void UpdateSingleRoutine(oCNpc* npc);
    void RestartRoutines();
    void CheckRoutines();
    zVEC3 GetRoutinePos(oCNpc* npc);
    void RemoveEntry(oCRtnEntry* entry);
    void RemoveRoutine(oCNpc* npc);
    void CheckConsistency(oCNpc* npc);
    void RemoveOverlay(oCNpc* npc);
    void CreateWayBoxes(oCNpc* npc);
    void InitWayBoxes();
    void InsertWaybox(oCNpc* npc, zVEC3& minimum, zVEC3& maximum);
    void RemoveWayBoxes(oCNpc* npc);
    void DrawWayBoxes();
    void CreateActiveList(oCNpc* npc, zTBBox3D& bbox);
    int CollidesWithWayBox(zTBBox3D& bbox, zCList<oCNpc>& npcList);
    int GetNumOfWayBoxes();
    void WriteRoutine(zCArchiver& archiver, oCNpc* npc, int overlay);
    void ReadRoutine(zCArchiver& archiver, oCNpc* npc);
    int FindRoutine(oCNpc* npc, oCRtnEntry*& before, oCRtnEntry*& now);

    static int Sort_Routine(oCRtnEntry* first, oCRtnEntry* second);
    static int Sort_WayBoxLimit(TRtn_WayBoxLimit* first, TRtn_WayBoxLimit* second);
    static int Sort_WayBoxX(TRtn_WayBox* first, TRtn_WayBox* second);
    static int Sort_WayBoxY(TRtn_WayBox* first, TRtn_WayBox* second);
    static int Sort_WayBoxZ(TRtn_WayBox* first, TRtn_WayBox* second);

    int indexStart[3];
    int indexEnd[3];
    zCWorld* world;
    zCListSort<oCRtnEntry>* nextEntryNode;
    zCListSort<oCRtnEntry> rtnList;
    zCListSort<TRtn_WayBox> wayboxListX;
    zCListSort<TRtn_WayBox> wayboxListY;
    zCListSort<TRtn_WayBox> wayboxListZ;
    zCList<oCNpc> activeList;
    zCArraySort<TRtn_WayBoxLimit*> wayboxList[3];
};

extern oCRtnManager rtnMan;
extern int s_bCheckRoutines;

#endif
