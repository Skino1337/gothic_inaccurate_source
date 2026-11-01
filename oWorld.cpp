#include "include/oGame.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/oBarrier.h"
#include "include/oObjFactory.h"
#include "include/zFile.h"

static const float WLD_TICKSPERHOUR = 250000.0f;
static const float WLD_TICKSPERMIN = WLD_TICKSPERHOUR / 60.0f;
static const float WLD_TICKSPERSEC = WLD_TICKSPERMIN / 60.0f;
static const float WLD_TICKSPERDAY = WLD_TICKSPERHOUR * 24.0f;
static int worldRuntimeState = -1;
static float worldRuntimeFarDistance = 250000.0f * 0.0166666675f;
static float worldRuntimeNearDistance = worldRuntimeFarDistance * 0.0166666675f;
static float worldRuntimeFarPlaneDistance = 250000.0f * 24.0f;
static zSTRING worldRuntimePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING worldRuntimePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING worldRuntimePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING worldRuntimePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING worldRuntimePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING worldRuntimePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING worldRuntimePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING worldRuntimePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING worldRuntimePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING worldRuntimePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static zCClassDef oCWorldClassDef("oCWorld", "zCWorld", oCWorld::_CreateNewInstance, 0, 0x628C, 0, "");
zCClassDef* oCWorld::classDef = &oCWorldClassDef;

static int CompareWorldVobs(zCVob* first, zCVob* second)
{
    return first - second;
}

static int CompareWorldNpcs(oCNpc* first, oCNpc* second)
{
    return first - second;
}

static int CompareWorldItems(oCItem* first, oCItem* second)
{
    return first - second;
}

zCObject* oCWorld::_CreateNewInstance()
{
    oCWorld* world = new oCWorld;
    zCClassDef::ObjectCreated(world, classDef);
    return world;
}

oCWorld::oCWorld()
{
    oCCollObjectCharacter::S_RegisterCollisionTestFuncs();
    voblist = new zCListSort<zCVob>();
    voblist_npcs = new zCListSort<oCNpc>();
    voblist_items = new zCListSort<oCItem>();
    voblist->compare_func = CompareWorldVobs;
    voblist_npcs->compare_func = CompareWorldNpcs;
    voblist_items->compare_func = CompareWorldItems;
    SetSkyControlerOutdoor(new oCSkyControler_Barrier);
}

oCWorld::~oCWorld()
{
    DisposeVobs();
    zCWorld::DisposeWorld();
    zCPlayerGroup::allPlayerGroup.ResetPlayerList();
    delete voblist;
    voblist = NULL;
    delete voblist_npcs;
    voblist_npcs = NULL;
    delete voblist_items;
    voblist_items = NULL;
}

zCClassDef* oCWorld::_GetClassDef() const
{
    return classDef;
}

oCVob* oCWorld::CreateVob(zTVobType type, const zSTRING& name)
{
    int instance = parser.GetIndex(name);
    if (instance < 0)
        zerr.Warning("U: OBJ: Script-object " + name +
            " not found ! Please correct instancename !");
    return CreateVob(type, instance);
}

oCVob* oCWorld::CreateVob(zTVobType type, int instance)
{
    oCObjectFactory* factory = dynamic_cast<oCObjectFactory*>(zfactory);
    if (!factory)
        return NULL;
    if (type == zVOB_TYPE_NSC)
        return factory->CreateNpc(instance);
    if (type == zVOB_TYPE_ITEM)
        return factory->CreateItem(instance);
    return NULL;
}

void oCWorld::Archive(zCArchiver& archiver)
{
    zCWorld::Archive(archiver);
    if (!archiver.InSaveGame())
        return;
    int count = 0;
    for (zCListSort<oCNpc>* node = voblist_npcs->next; node; node = node->next)
    {
        if (!node->data->IsAPlayer() && !node->data->homeWorld)
            ++count;
    }
    archiver.WriteInt(count);
    for (zCListSort<oCNpc>* node = voblist_npcs->next; node; node = node->next)
    {
        oCNpc* npc = node->data;
        if (!npc->IsAPlayer() && !npc->homeWorld)
            archiver.WriteObject(npc);
    }
    if (ogame)
        ogame->GetSpawnManager()->Archive(archiver);
}

void oCWorld::Unarchive(zCArchiver& archiver)
{
    zCWorld::Unarchive(archiver);
    if (!archiver.InSaveGame())
        return;
    int count = archiver.ReadInt();
    for (int i = 0; i < count; ++i)
    {
        zCObject* object = archiver.ReadObject(NULL);
        InsertInLists((zCVob*)object);
        if (object)
            object->Release();
    }
    if (ogame)
        ogame->GetSpawnManager()->Unarchive(archiver);
}

int oCWorld::SaveWorld(const zSTRING& fileName, zTWorldSaveMode saveMode, int saveGame,
    int export3DS)
{
    if (wayNet)
        wayNet->ClearVobDependencies();
    return zCWorld::SaveWorld(fileName, saveMode, saveGame, export3DS);
}

int oCWorld::LoadWorld(const zSTRING& fileName, zTWorldLoadMode loadMode)
{
    zSTRING upperName = fileName;
    upperName.Upper();
    worldFilename = upperName;
    zFILE_FILE file(fileName);
    worldName = file.GetFilename();
    if (upperName.Search(".3DS") <= 0)
    {
        zoptions->ChangeDir(14);
        return zCWorld::LoadWorld(fileName, loadMode);
    }
    zCVobLevelCompo* levelVob = new zCVobLevelCompo;
    zCClassDef::ObjectCreated(levelVob, zCVobLevelCompo::classDef);
    levelVob->SetVobName("Level-Vob");
    levelVob->SetVisual(fileName);
    AddVob(levelVob);
    levelVob->Release();
    return 1;
}

zCTree<zCVob>* oCWorld::AddVobAsChild(zCVob* vob, zCTree<zCVob>* parent)
{
    if (!vob)
        return NULL;
    InsertVobInWorld(vob);
    return zCWorld::AddVobAsChild(vob, parent);
}

void oCWorld::RemoveVob(zCVob* vob)
{
    if (!vob)
        return;
    zCWorld* oldWorld = vob->homeWorld;
    RemoveFromLists(vob);
    if (oldWorld)
        zCWorld::RemoveVob(vob);
}

void oCWorld::InsertVobInWorld(zCVob* vob)
{
    if (vob)
        InsertInLists(vob);
}

void oCWorld::EnableVob(zCVob* vob, zCVob* parent)
{
    if (vob->homeWorld)
        return;
    if (dynamic_cast<oCNpc*>(vob))
        vob->globalVobTreeNode = NULL;
    zCTree<zCVob>* parentNode = parent ? parent->globalVobTreeNode : &globalVobTree;
    zCWorld::AddVobAsChild(vob, parentNode);
    vob->SetSleeping(0);
}

void oCWorld::DisableVob(zCVob* vob)
{
    if (!vob->homeWorld)
        return;
    zCWorld::RemoveVobSubtree(vob);
    vob->SetSleeping(1);
}

zCVob* oCWorld::SearchVob(zCVob* vob, zCTree<zCVob>* node)
{
    zCVob* result = zCWorld::SearchVob(vob, node);
    if (result || node)
        return result;
    for (zCListSort<zCVob>* entry = voblist; entry; entry = entry->next)
    {
        if (entry->data == vob)
            return vob;
    }
    return NULL;
}

zCVob* oCWorld::SearchVobByID(unsigned long id, zCTree<zCVob>* node)
{
    zCVob* result = zCWorld::SearchVobByID(id, node);
    if (result || node)
        return result;
    for (zCListSort<zCVob>* entry = voblist; entry; entry = entry->next)
    {
        if (entry->data && id == 1)
            return entry->data;
    }
    return NULL;
}

zCVob* oCWorld::SearchVobByName(const zSTRING& name)
{
    zCVob* result = zCWorld::SearchVobByName(name);
    if (result)
        return result;
    for (zCListSort<zCVob>* entry = voblist; entry; entry = entry->next)
    {
        if (entry->data && entry->data->GetObjectName() == name)
            return entry->data;
    }
    return NULL;
}

void oCWorld::SearchVobListByName(const zSTRING& name, zCArray<zCVob*>& vobs)
{
    zCWorld::SearchVobListByName(name, vobs);
    for (zCListSort<zCVob>* entry = voblist; entry; entry = entry->next)
    {
        if (entry->data && entry->data->GetObjectName() == name)
            vobs.InsertEnd(entry->data);
    }
}

void oCWorld::TraverseVobList(zCVobCallback& callback, void* data)
{
    for (zCListSort<zCVob>* entry = voblist; entry; entry = entry->next)
    {
        if (entry->data)
            callback.HandleVob(entry->data, data);
    }
}

void oCWorld::ClearNpcPerceptionVobLists()
{
    for (zCListSort<oCNpc>* entry = voblist_npcs->next; entry; entry = entry->next)
    {
        oCNpc* npc = entry->data;
        if (!npc->IsAPlayer())
        {
            npc->ClearPerceptionLists();
            npc->SetEnemy(NULL);
        }
    }
}

template <class T>
static int InsertWorldList(zCListSort<T>* list, T* value)
{
    for (zCListSort<T>* entry = list->next; entry; entry = entry->next)
    {
        if (entry->data == value)
            return 0;
    }
    list->InsertSort(value);
    value->AddRef();
    return 1;
}

template <class T>
static int RemoveWorldList(zCListSort<T>* list, T* value)
{
    for (zCListSort<T>* entry = list->next; entry; entry = entry->next)
    {
        if (entry->data == value)
        {
            list->Remove(value);
            value->Release();
            return 1;
        }
    }
    return 0;
}

void oCWorld::InsertInLists(zCVob* vob)
{
    if (!vob || !dynamic_cast<oCVob*>(vob))
        return;
    InsertWorldList(voblist, vob);
    if (oCNpc* npc = dynamic_cast<oCNpc*>(vob))
        InsertWorldList(voblist_npcs, npc);
    if (oCItem* item = dynamic_cast<oCItem*>(vob))
        InsertWorldList(voblist_items, item);
}

void oCWorld::RemoveFromLists(zCVob* vob)
{
    if (vob->refCtr <= 0)
    {
        zerr.Warning("RemoveFromList(vob) called from vob destructor!\n");
        return;
    }
    vob->AddRef();
    if (dynamic_cast<oCVob*>(vob))
        RemoveWorldList(voblist, vob);
    if (oCNpc* npc = dynamic_cast<oCNpc*>(vob))
    {
        if (RemoveWorldList(voblist_npcs, npc))
            npc->CleanUp();
    }
    if (oCItem* item = dynamic_cast<oCItem*>(vob))
        RemoveWorldList(voblist_items, item);
    vob->Release();
}

void oCWorld::DisposeVobs()
{
    while (voblist_npcs->next)
    {
        oCNpc* npc = voblist_npcs->next->data;
        npc->CleanUp();
        npc->DestroySpellBook();
        npc->RemoveFromAllSlots(0);
        RemoveWorldList(voblist_npcs, npc);
    }
    while (voblist_items->next)
        RemoveWorldList(voblist_items, voblist_items->next->data);
    while (voblist->next)
        RemoveWorldList(voblist, voblist->next->data);
    zCWorld::DisposeVobs(NULL);
}

void oCWorld::DisposeWorld()
{
    DisposeVobs();
    zCWorld::DisposeWorld();
}

oCWorldTimer::oCWorldTimer()
    : worldTime(0.0f), day(0)
{
}

oCWorldTimer::~oCWorldTimer()
{
}

void oCWorldTimer::Timer()
{
    worldTime += ztimer.frameTimeFloat;
    if (worldTime >= WLD_TICKSPERDAY)
    {
        worldTime -= WLD_TICKSPERDAY;
        ++day;
        if (ogame)
            ogame->RefreshNpcs();
    }
    rtnMan.CheckRoutines();
}

int oCWorldTimer::GetDay() const
{
    return day;
}

void oCWorldTimer::SetDay(int newDay)
{
    day = newDay;
}

void oCWorldTimer::GetTime(int& hour, int& minute) const
{
    hour = int(worldTime / WLD_TICKSPERHOUR);
    minute = int((worldTime - hour * WLD_TICKSPERHOUR) / WLD_TICKSPERMIN);
}

void oCWorldTimer::SetTime(int hour, int minute)
{
    worldTime = hour % 24 * WLD_TICKSPERHOUR + minute % 60 * WLD_TICKSPERMIN;
}

float oCWorldTimer::GetFullTime() const
{
    return day * WLD_TICKSPERDAY + worldTime;
}

void oCWorldTimer::SetFullTime(float time)
{
    day = int(time / WLD_TICKSPERDAY);
    worldTime = time - day * WLD_TICKSPERDAY;
}

zSTRING oCWorldTimer::GetTimeString() const
{
    int hour;
    int minute;
    GetTime(hour, minute);
    zSTRING minuteString(minute);
    if (minuteString.Length() == 1)
        minuteString = "0" + minuteString;
    return zSTRING(hour) + ":" + minuteString;
}

int oCWorldTimer::IsLater(int hour, int minute) const
{
    float target = hour * WLD_TICKSPERHOUR + minute * WLD_TICKSPERMIN + WLD_TICKSPERMIN - 1.0f;
    return target > worldTime;
}

int oCWorldTimer::IsLaterEqual(int hour, int minute) const
{
    float target = hour * WLD_TICKSPERHOUR + minute * WLD_TICKSPERMIN + WLD_TICKSPERMIN - 1.0f;
    return target >= worldTime;
}

int oCWorldTimer::IsTimeBetween(int hour1, int min1, int hour2, int min2) const
{
    float begin = hour1 * WLD_TICKSPERHOUR + min1 * WLD_TICKSPERMIN;
    float end = hour2 * WLD_TICKSPERHOUR + min2 * WLD_TICKSPERMIN;
    if (begin <= end)
        return worldTime >= begin && worldTime <= end;
    return worldTime >= begin || worldTime <= end;
}

int oCWorldTimer::GetPassedTime(float time) const
{
    return int((GetFullTime() - time) / WLD_TICKSPERMIN);
}

float oCWorldTimer::GetSkyTime() const
{
    float time = worldTime / WLD_TICKSPERDAY + 0.5f;
    while (time >= 1.0f)
        time -= 1.0f;
    return time;
}

int oCWorldTimer::IsDay() const
{
    return IsTimeBetween(6, 30, 18, 30);
}

int oCWorldTimer::IsNight() const
{
    return !IsDay();
}

void oCWorldTimer::AddTime(int& hour, int& minute, int addHour, int addMinute)
{
    minute += addMinute;
    hour += minute / 60;
    minute %= 60;
    hour += addHour;
    if (hour > 23)
        hour %= 24;
}
