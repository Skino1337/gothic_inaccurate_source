#include "include/oInformationManager.h"

int oCInfoManager::size_checked;
static int infoState = -1;
static int infoRuntimeState = -1;
static zSTRING infoRuntimePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING infoRuntimePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING infoRuntimePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING infoRuntimePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING infoRuntimePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING infoRuntimePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING infoRuntimePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING infoRuntimePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING infoRuntimePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING infoRuntimePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float infoFarDistance = 250000.0f * 0.0166666675f;
static float infoNearDistance = infoFarDistance * 0.0166666675f;
static float infoFarPlaneDistance = 250000.0f * 24.0f;
static zCOLOR infoColorDarkGray(144, 144, 144, 255);
static zCOLOR infoColorLightGray(208, 208, 208, 255);
static zCOLOR infoColorWhite(255, 255, 255, 255);
static zCOLOR infoColorGreen(0, 255, 0, 255);
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
static zSTRING infoClass("C_INFO");

oCInfo::oCInfo()
    : told(0), instance(-1), next(0)
{
    listChoices.data = 0;
    listChoices.next = 0;
    pd.important = 0;
    pd.trade = 0;
    pd.permanent = 0;
    pd.npc = -1;
    pd.nr = -1;
    pd.conditions = -1;
    pd.information = -1;
}

oCInfo::~oCInfo()
{
    next = 0;
    zCList_oCInfoChoice* choice = listChoices.next;
    while (choice) {
        zCList_oCInfoChoice* following = choice->next;
        delete choice;
        choice = following;
    }
    listChoices.next = 0;
    listChoices.data = 0;
}

oCInfo::Tpd::~Tpd()
{
}

oCInfoManager::oCInfoManager(zCParser* scriptParser)
    : p(scriptParser)
{
    infoList.data = 0;
    infoList.next = 0;
    infoList.compare_func = CompareInfos;
    if (!p)
        return;
    int classIndex = p->GetIndex(infoClass);
    if (classIndex < 0)
        return;
    int instanceIndex = classIndex;
    int count = 0;
    while (true) {
        int instance = p->GetInstance(classIndex, ++instanceIndex);
        if (instance < 0)
            break;
        oCInfo* info = new oCInfo;
        p->CreateInstance(instance, &info->pd);
        info->instance = instance;
        int first = 0;
        int second = 0;
        info->name = p->GetSymbolInfo(instance, first, second);
        info->DoCheck();
        infoList.InsertSort(info);
        ++count;
    }
    zerr->Message(zSTRING("U: INF: ") + zSTRING((long)count) + " Infos found.");
}

oCInfoManager::~oCInfoManager()
{
    zCListSort_oCInfo* node = infoList.next;
    while (node) {
        zCListSort_oCInfo* next = node->next;
        delete node;
        node = next;
    }
    infoList.next = 0;
    p = 0;
}

int oCInfoManager::CompareInfos(oCInfo* first, oCInfo* second)
{
    if (!first || !second)
        return 0;
    if (first->pd.nr > second->pd.nr)
        return 1;
    return first->pd.nr == second->pd.nr ? 0 : -1;
}

static void SetInfoParserInstances(zCParser* parser, oCNpc* self, oCNpc* other)
{
    parser->SetInstance("SELF", self);
    parser->SetInstance("OTHER", other);
}

static int InfoMatches(oCInfo* info, int npcInstance, int important)
{
    if (!info || (!info->pd.permanent && info->told) || info->pd.npc != npcInstance)
        return 0;
    if (important >= 0 && info->pd.important != important)
        return 0;
    return parser.CallFunc(info->pd.conditions) == 1;
}

oCInfo* oCInfoManager::GetInformation(oCNpc* self, oCNpc* other, int important)
{
    SetInfoParserInstances(p, self, other);
    int npcInstance = self->GetInstance();
    for (zCListSort_oCInfo* node = &infoList; node; node = node->next) {
        if (InfoMatches(node->data, npcInstance, important))
            return node->data;
    }
    return 0;
}

oCInfo* oCInfoManager::GetInformation(int instance)
{
    for (zCListSort_oCInfo* node = &infoList; node; node = node->next) {
        if (node->data && node->data->instance == instance)
            return node->data;
    }
    return 0;
}

static int CountInfos(zCListSort_oCInfo* list, int npcInstance, int important)
{
    int count = 0;
    for (zCListSort_oCInfo* node = list; node; node = node->next) {
        if (InfoMatches(node->data, npcInstance, important))
            ++count;
    }
    return count;
}

int oCInfoManager::GetInfoCount(oCNpc* self, oCNpc* other)
{
    SetInfoParserInstances(p, self, other);
    return CountInfos(&infoList, self->GetInstance(), -1);
}

int oCInfoManager::GetInfoCountImportant(oCNpc* self, oCNpc* other)
{
    SetInfoParserInstances(p, self, other);
    return CountInfos(&infoList, self->GetInstance(), 1);
}

int oCInfoManager::GetInfoCountUnimportant(oCNpc* self, oCNpc* other)
{
    SetInfoParserInstances(p, self, other);
    return CountInfos(&infoList, self->GetInstance(), 0);
}

static oCInfo* GetIndexedInfo(zCListSort_oCInfo* list, int npcInstance, int important, int index)
{
    int currentIndex = 0;
    for (zCListSort_oCInfo* node = list; node; node = node->next) {
        if (!InfoMatches(node->data, npcInstance, important))
            continue;
        if (currentIndex == index)
            return node->data;
        ++currentIndex;
    }
    return 0;
}

oCInfo* oCInfoManager::GetInfo(oCNpc* self, oCNpc* other, int index)
{
    SetInfoParserInstances(p, self, other);
    return GetIndexedInfo(&infoList, self->GetInstance(), -1, index);
}

oCInfo* oCInfoManager::GetInfoImportant(oCNpc* self, oCNpc* other, int index)
{
    SetInfoParserInstances(p, self, other);
    return GetIndexedInfo(&infoList, self->GetInstance(), 1, index);
}

oCInfo* oCInfoManager::GetInfoUnimportant(oCNpc* self, oCNpc* other, int index)
{
    SetInfoParserInstances(p, self, other);
    return GetIndexedInfo(&infoList, self->GetInstance(), 0, index);
}

int oCInfoManager::InformationTold(int instance)
{
    oCInfo* info = GetInformation(instance);
    return info && !info->pd.permanent ? info->told : 0;
}

void oCInfoManager::Archive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    int count = 0;
    for (zCListSort_oCInfo* node = infoList.next; node; node = node->next)
        ++count;
    archiver.WriteInt(count);
    for (zCListSort_oCInfo* node = &infoList; node; node = node->next) {
        if (node->data)
            node->data->Archive(archiver);
    }
}

void oCInfoManager::Unarchive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    int count = archiver.ReadInt();
    zCListSort_oCInfo* node = infoList.next;
    while (node) {
        zCListSort_oCInfo* next = node->next;
        delete node;
        node = next;
    }
    infoList.next = 0;
    for (int i = 0; i < count; ++i) {
        oCInfo* info = new oCInfo;
        info->Unarchive(archiver);
        infoList.InsertSort(info);
    }
}

void oCInfoManager::RestoreParserInstances()
{
    for (zCListSort_oCInfo* node = &infoList; node; node = node->next) {
        if (node->data)
            node->data->RestoreParserInstance();
    }
}

void oCInfo::SetInstance(int newInstance)
{
    instance = newInstance;
    int first = 0;
    int second = 0;
    name = parser.GetSymbolInfo(instance, first, second);
}

int oCInfo::GetInstance() const
{
    return instance;
}

int oCInfo::DoCheck()
{
    if (pd.npc <= 0)
        zerr->Message(zSTRING("U: INF: Skriptinfo:npc is not/wrong initialised:") +
            pd.description + ":" + zSTRING((long)pd.information));
    return 1;
}

int oCInfo::GetNpcID() const
{
    return pd.npc;
}

int oCInfo::GetConditionFunc() const
{
    return pd.conditions;
}

int oCInfo::WasTold() const
{
    return pd.permanent ? 0 : told;
}

void oCInfo::SetTold(int value)
{
    told = value;
}

int oCInfo::GetDataSize() const
{
    return 0x30;
}

void* oCInfo::GetDataAdr()
{
    return &pd;
}

zSTRING& oCInfo::GetText()
{
    return pd.description;
}

int oCInfo::InfoConditions()
{
    return parser.CallFunc(pd.conditions);
}

void oCInfo::Info()
{
    parser.CallFunc(pd.information);
    told = 1;
}

void oCInfo::Archive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    archiver.WriteBool(told);
    archiver.WriteString(name);
}

void oCInfo::Unarchive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    told = archiver.ReadBool();
    name = archiver.ReadString();
    instance = parser.GetIndex(name);
    if (!parser.CreateInstance(instance, &pd))
        zerr->Message("Info instance not found.");
}

void oCInfo::RestoreParserInstance()
{
    parser.SetInstance(instance, &pd);
}

void oCInfo::AddChoice(zSTRING text, int function)
{
    oCInfoChoice* choice = new oCInfoChoice;
    choice->text = text;
    choice->function = function;
    zCList_oCInfoChoice* node = new zCList_oCInfoChoice;
    node->data = choice;
    node->next = listChoices.next;
    listChoices.next = node;
}

void oCInfo::RemoveChoice(zSTRING text)
{
    zCList_oCInfoChoice* previous = &listChoices;
    while (previous->next) {
        zCList_oCInfoChoice* node = previous->next;
        if (node->data && node->data->text == text) {
            previous->next = node->next;
            delete node->data;
            delete node;
            return;
        }
        previous = node;
    }
}

void oCInfo::RemoveAllChoices()
{
    zCList_oCInfoChoice* node = listChoices.next;
    while (node) {
        zCList_oCInfoChoice* next = node->next;
        delete node->data;
        delete node;
        node = next;
    }
    listChoices.next = 0;
}

oCInfoChoice::~oCInfoChoice()
{
}
