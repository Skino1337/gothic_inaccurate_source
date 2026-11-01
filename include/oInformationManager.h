#ifndef OINFORMATIONMANAGER_H
#define OINFORMATIONMANAGER_H

#include "oCollisionObjectMiscChar.h"

struct oCInfoChoice {
    ~oCInfoChoice();

    zSTRING text;
    int function;
};

struct zCList_oCInfoChoice {
    oCInfoChoice* data;
    zCList_oCInfoChoice* next;
};

class oCInfo {
public:

    oCInfo();
    ~oCInfo();
    zSTRING& GetText();
    int InfoConditions();
    void Info();
    void AddChoice(zSTRING text, int function);
    void RemoveChoice(zSTRING text);
    void RemoveChoice(oCInfoChoice* choice);
    void RemoveAllChoices();
    int DoCheck();
    void SetInstance(int instance);
    int GetInstance() const;
    int GetNpcID() const;
    int GetConditionFunc() const;
    int WasTold() const;
    void SetTold(int value);
    int GetDataSize() const;
    void* GetDataAdr();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void RestoreParserInstance();

    zCList_oCInfoChoice listChoices;
    struct Tpd {
        ~Tpd();

        int nr;
        int npc;
        int important;
        int trade;
        int permanent;
        int conditions;
        int information;
        zSTRING description;
    } pd;
    int told;
    int instance;
    oCInfo* next;
    zSTRING name;
};

struct zCListSort_oCInfo {
    void InsertSort(oCInfo* value)
    {
        zCListSort_oCInfo* entry = new zCListSort_oCInfo;
        entry->data = value;
        entry->next = 0;
        entry->compare_func = compare_func;
        zCListSort_oCInfo* current = this;
        while (current->next && compare_func &&
            compare_func(value, current->next->data) >= 0)
            current = current->next;
        entry->next = current->next;
        current->next = entry;
    }

    int(*compare_func)(oCInfo* first, oCInfo* second);
    oCInfo* data;
    zCListSort_oCInfo* next;
};

class oCInfoManager {
public:

    oCInfoManager(zCParser* scriptParser);
    ~oCInfoManager();
    static int CompareInfos(oCInfo* first, oCInfo* second);
    oCInfo* GetInformation(oCNpc* self, oCNpc* other, int important);
    oCInfo* GetInformation(int instance);
    int GetInfoCount(oCNpc* self, oCNpc* other);
    int GetInfoCountImportant(oCNpc* self, oCNpc* other);
    int GetInfoCountUnimportant(oCNpc* self, oCNpc* other);
    oCInfo* GetInfo(oCNpc* self, oCNpc* other, int index);
    oCInfo* GetInfoImportant(oCNpc* self, oCNpc* other, int index);
    oCInfo* GetInfoUnimportant(oCNpc* self, oCNpc* other, int index);
    int InformationTold(int instance);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void RestoreParserInstances();

    zCListSort_oCInfo infoList;
    zCParser* p;

    static int size_checked;
};

class zCViewDialogChoice : public zCObject {
public:

    zCViewDialogChoice();
    virtual ~zCViewDialogChoice();
    static void operator delete(void* memory);
    void RemoveAllChoices();
    void AddChoice(const zSTRING& text, int userData);

    zCArray<zSTRING> choices;
    zCArray<int> choiceData;
    float DurationOpen;
    float DurationClose;
    int ModeOpen;
    int ModeClose;
    int opened;
    int ChoiceSelected;

    static zCClassDef* classDef;
};

class oCViewDialogTrade : public zCObject {
public:

    oCViewDialogTrade();
    virtual ~oCViewDialogTrade();
    static void operator delete(void* memory);
    void UpdateViewSettings();
    void SetNpcLeft(oCNpc* npc);
    void SetNpcRight(oCNpc* npc);

    oCNpc* NpcLeft;
    oCNpc* NpcRight;
    float DurationOpen;
    float DurationClose;
    int ModeOpen;
    int ModeClose;
    int settingsCounter;

    static zCClassDef* classDef;
};

class oCInformationManager {
public:

    enum zTInfoMgrMode {
        INFO_MGR_MODE_IMPORTANT,
        INFO_MGR_MODE_INFO,
        INFO_MGR_MODE_CHOICE,
        INFO_MGR_MODE_TRADE
    };

    oCInformationManager();
    ~oCInformationManager();
    void UpdateViewSettings();
    void ToggleStatus();
    void PrintStatus();
    int HasFinished();
    int WaitingForEnd();
    void SetNpc(oCNpc* npc);
    void Update();
    void Exit();
    void CameraStart();
    void CameraStop();
    void CameraRefresh();
    void ProcessImportant();
    void ProcessNextImportant();
    void InfoWaitForEnd();
    void CollectInfos();
    int CollectChoices(oCInfo* info);
    void OnImportantBegin();
    void OnImportantEnd();
    void OnInfoBegin();
    void OnInfo(int index);
    void OnInfo(oCInfo* info);
    void OnChoiceBegin();
    void OnChoice(int index);
    void OnChoice(oCInfoChoice* choice);
    void OnChoiceEnd();
    void OnInfoEnd();
    void OnTradeBegin();
    void OnTradeEnd();
    void OnExit();
    void OnTermination();
    static oCInformationManager& GetInformationManager();

    zSTRING LastMethod;
    zCViewDialogChoice* DlgStatus;
    oCViewDialogTrade* DlgTrade;
    zCViewDialogChoice* DlgChoice;
    oCNpc* Npc;
    oCNpc* Player;
    oCInfo* Info;
    int IsDone;
    int IsWaitingForEnd;
    int IsWaitingForScript;
    int IsWaitingForOpen;
    int IsWaitingForClose;
    int IsWaitingForSelection;
    int MustOpen;
    int IndexBye;
    int ImportantCurrent;
    int ImportantMax;
    zTInfoMgrMode Mode;
};

#endif
