#include "include/oObjFactory.h"
#include "include/zCsCamera.h"
#include "include/oCsPlayer.h"
#include "include/oCsProps.h"
#include "include/zCsLib.h"
#include "include/zSession.h"

extern zCOption* zgameoptions;

static int objectFactoryState = -1;
static int objectFactoryRuntimeState = -1;
static int gameplayState = -1;
static zSTRING gameplayPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING gameplayPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING gameplayPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING gameplayPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING gameplayPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING gameplayPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING gameplayPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING gameplayPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING gameplayPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING gameplayPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float objectFactoryFarDistance = 250000.0f * 0.0166666675f;
static float objectFactoryNearDistance = objectFactoryFarDistance * 0.0166666675f;
static float objectFactoryFarPlaneDistance = 250000.0f * 24.0f;
static zSTRING objectFactoryPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING objectFactoryPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING objectFactoryPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING objectFactoryPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING objectFactoryPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING objectFactoryPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING objectFactoryPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING objectFactoryPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING objectFactoryPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING objectFactoryPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static zCClassDef oCObjectFactoryClassDef("oCObjectFactory", "zCObjectFactory",
    oCObjectFactory::_CreateNewInstance, 0, 0x24, 0, "");
zCClassDef* oCObjectFactory::classDef = &oCObjectFactoryClassDef;
static int classDefState = -1;
static zCClassDef oCCSManagerClassDef("oCCSManager", "zCCSManager",
    oCCSManager::_CreateNewInstance, 0, 0x70, 0, "");
zCClassDef* oCCSManager::classDef = &oCCSManagerClassDef;
zCClassDef* oCWaypoint::classDef;

void oCObjectFactory::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* oCObjectFactory::_GetClassDef() const
{
    return classDef;
}

zCCSManager::zCCSManager()
    : library(NULL)
{
    library = new zCCSLib;
    zCClassDef::ObjectCreated(library, zCCSLib::classDef);
    zCCSCutsceneContext::playDisabled = zoptions->ReadBool("INTERNAL", "cutscenesDisabled", 0);
}

zCObject* zCCSManager::_CreateNewInstance()
{
    zCCSManager* manager = new zCCSManager;
    zCClassDef::ObjectCreated(manager, classDef);
    return manager;
}

void zCCSManager::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

static zCArray<zSTRING> libSvmModuleList;
static int zCCSPlayerClassDefState = -1;
static zCClassDef zCCSPlayerClassDef("zCCSPlayer", "zCObject",
    zCCSPlayer::_CreateNewInstance, 1, 0x68, 0, "");
zCClassDef* zCCSPlayer::classDef = &zCCSPlayerClassDef;
static int zCCSPoolItemClassDefState = -1;
static zCClassDef zCCSPoolItemClassDef("zCCSPoolItem", "zCObject",
    zCCSPoolItem::_CreateNewInstance, 0, 0x50, 0, "");
zCClassDef* zCCSPoolItem::classDef = &zCCSPoolItemClassDef;

zCCSPoolItem::zCCSPoolItem()
{
    Init();
}

zCCSPoolItem::zCCSPoolItem(zCCSCutsceneContext* context)
{
    Init();
    poolName = context->GetCSName();
}

zCCSPoolItem::~zCCSPoolItem()
{
}

zCObject* zCCSPoolItem::_CreateNewInstance()
{
    zCCSPoolItem* item = new zCCSPoolItem;
    zCClassDef::ObjectCreated(item, classDef);
    return item;
}

zCClassDef* zCCSPoolItem::_GetClassDef() const
{
    return classDef;
}

void zCCSPoolItem::Init()
{
    deactivated = 0;
    runBehaviour = 0;
    runBehaviourValue = 0;
    flags = 0;
    numPlayed = 0;
    playCounter = 0;
}

void zCCSPoolItem::Archive(zCArchiver& archiver)
{
    archiver.WriteString(poolName);
    archiver.WriteEnum(runBehaviour);
    archiver.WriteInt(runBehaviourValue);
    archiver.WriteInt(numPlayed);
    archiver.WriteInt(playCounter);
    archiver.WriteInt(deactivated);
}

void zCCSPoolItem::Unarchive(zCArchiver& archiver)
{
    poolName = archiver.ReadString();
    poolName.Upper();
    runBehaviour = archiver.ReadEnum();
    runBehaviourValue = archiver.ReadInt();
    numPlayed = archiver.ReadInt();
    playCounter = archiver.ReadInt();
    deactivated = archiver.ReadInt();
}

void zCCSPoolItem::PrintDebugInfo()
{
    zerr->Message(zSTRING("B: CS: Cutscene \"") + poolName + "\" in history-pool");
    if (deactivated)
        zerr->Message("B: CS: *** deactivated ***");
}

void zCCSPoolItem::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

extern zTIMER ztimer;

int zCCSPlayer::IsAssigned(zSTRING& name, zSTRING&)
{
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next) {
        if (entry->data->AreU(name))
            return 1;
    }
    return 0;
}

void zCCSPlayer::PrintListOfCutscenes(const zSTRING& prefix)
{
    zerr->Message(zSTRING("B: CS: LIST OF CUTSCENES (") + prefix + ")");
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next)
        zerr->Message(entry->data->GetCSName());
}

zCCSCutsceneContext* zCCSPlayer::GetStartedCutscene(zCVob* vob)
{
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next) {
        if (entry->data->mainRoleVob == vob)
            return entry->data;
    }
    return NULL;
}

zCCSCutsceneContext* zCCSPlayer::GetStartedCutscene(const zSTRING& name)
{
    zSTRING upperName = name;
    upperName.Upper();
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next) {
        if (entry->data->AreU(upperName))
            return entry->data;
    }
    return NULL;
}

zCCSCutsceneContext* zCCSPlayer::GetPlayingGlobalCutscene()
{
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next) {
        if (entry->data->properties && entry->data->properties->globalCutscene &&
            entry->data->IsPlaying())
            return entry->data;
    }
    return NULL;
}

int zCCSPlayer::UpdateCutscenesNow()
{
    if (!playList.next)
        return 0;
    if (timerCtr < 250.0f) {
        timerCtr += ztimer.frameTimeFloat;
        return 0;
    }
    timerCtr = 0.0f;
    return 1;
}

void zCCSPlayer::DebugResetInfoscreen()
{
}

void zCCSPlayer::DebugAddCutscene(int, zVEC3, int)
{
}

void zCCSPlayer::Process()
{
    if (!playList.next) {
        DebugResetInfoscreen();
        return;
    }
    if (timerCtr < 250.0f) {
        timerCtr += ztimer.frameTimeFloat;
        return;
    }
    timerCtr = 0.0f;
    zCSession* session = ownerWorld->GetOwnerSession();
    int hour;
    int minute;
    int second;
    session->GetTime(hour, minute, second);
    zCVob* camera = session->GetCameraVob();
    ProcessList(camera->trafoObjToWorld.GetTranslation(), hour, minute, second);
}

void zCCSPlayer::ProcessList(const zVEC3& position, int day, int hour, int second)
{
    int resetAllowed = 0;
    if (lastProcessDay != day) {
        ownerManager->PoolResetByDay();
        lastProcessDay = day;
        resetAllowed = 1;
    }
    if (lastProcessHour != hour) {
        ownerManager->PoolResetByHour();
        lastProcessHour = hour;
        resetAllowed = 1;
    }
    if (resetAllowed) {
        for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next) {
            if (!entry->data->allowedToPlay
                && ownerManager->PoolAllowedToStart(entry->data->GetCSName()) == 1)
                entry->data->allowedToPlay = 1;
        }
    }
    processingListItem = playList.next;
    processingCtr = 0;
    while (processingListItem) {
        zCCSCutsceneContext* context = processingListItem->data;
        processingListItem = processingListItem->next;
        ++processingCtr;
        context->AddRef();
        if (context->allowedToPlay && !context->IsFinished())
            ProcessCutscene(context, position);
        if (debugInfoOnScreen) {
            DebugResetInfoscreen();
            int index = 0;
            for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next) {
                DebugAddCutscene(index, position, entry->data == context);
                ++index;
            }
        }
        context->Release();
    }
}

void zCCSPlayer::ProcessCutscene(zCCSCutsceneContext* context, const zVEC3& position)
{
    context->Play(position);
    if (context->lastPlayResult == 0)
        ownerManager->PoolCountStart(context->GetCSName());
    else if (context->lastPlayResult == 1)
        ownerManager->PoolCountStop(context->GetCSName());
}

void zCCSPlayer::ToggleDebugInfo()
{
    debugInfoOnScreen = !debugInfoOnScreen;
    if (!debugInfoScreen)
        debugInfoScreen = new zCView(0, 500, 8192, 8000, 2);
    if (debugInfoOnScreen) {
        screen->InsertItem(debugInfoScreen);
        DebugResetInfoscreen();
    } else {
        screen->RemoveItem(debugInfoScreen);
    }
}

void zCCSPlayer::CSDB_BeginSyncBlock(zCCSSyncBlock* block)
{
    if (deb)
        deb->CSDB_BeginSyncBlock(block);
    else
        zerr->Message("A: CSBD: new syncblock");
}

void zCCSPlayer::CSDB_BeginAtomBlock(zCCSBlockBase* block)
{
    callstack.InsertEnd(block);
    if (deb)
        deb->CSDB_BeginAtomBlock(block);
}

void zCCSPlayer::CSDB_EndAtomBlock(zCCSBlockBase* block)
{
    callstack.Remove(block);
    if (deb)
        deb->CSDB_EndAtomBlock(block);
}

void zCCSPlayer::CSDB_Error(zCCSBlockBase* block, zSTRING& text)
{
    if (deb)
        deb->CSDB_Error(block, text);
    else
        zerr->Message(zSTRING("A: CSBD: ERROR  : ") + text);
}

void zCCSPlayer::CSDB_Warning(zCCSBlockBase* block, zSTRING& text)
{
    if (deb)
        deb->CSDB_Warning(block, text);
    else
        zerr->Message(zSTRING("A: CSBD: WARNING: ") + text);
}

void zCCSPlayer::CSDB_StartPlaying(zCCutscene* cutscene)
{
    if (deb)
        deb->CSDB_StartPlaying(cutscene);
}

void zCCSPlayer::CSDB_Stop(zCCutscene* cutscene)
{
    if (deb)
        deb->CSDB_Stop(cutscene);
}

void zCCSPlayer::ResetCutscenePlayer()
{
    StopAllCutscenes(NULL);
    ownerManager->RemoveCutscenePlayer(this);
}

int zCCSPlayer::StopAllCutscenes(zCVob* vob)
{
    int stopped = 0;
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry;) {
        zCCSCutsceneContext* context = entry->data;
        entry = entry->next;
        if (!vob || context->mainRoleVob == vob) {
            context->Stop();
            RemoveCutscene(context);
            ++stopped;
        }
    }
    return stopped;
}

int zCCSPlayer::StopAllOutputUnits(zCVob* vob)
{
    int stopped = 0;
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry;) {
        zCCSCutsceneContext* context = entry->data;
        entry = entry->next;
        if (context->IsOutputUnit() && (!vob || context->mainRoleVob == vob)) {
            context->Stop();
            RemoveCutscene(context);
            ++stopped;
        }
    }
    return stopped;
}

void zCCSPlayer::ForgetCutscene(zCCutscene* cutscene)
{
    processingListItem = NULL;
    RemoveCutscene(dynamic_cast<zCCSCutsceneContext*>(cutscene));
}

void zCCSPlayer::Interrupt()
{
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next)
        entry->data->ForcedInterrupt();
}

void zCCSPlayer::Resume()
{
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next) {
        if (!entry->data->IsFinished())
            entry->data->ResumeActBlock();
    }
}

void zCCSPlayer::InsertCutscene(zCCSCutsceneContext* context)
{
    if (context) {
        playList.Insert(context);
        context->AddRef();
    }
}

void zCCSPlayer::RemoveCutscene(zCCSCutsceneContext* context)
{
    if (context) {
        playList.Remove(context);
        context->Release();
    }
}

void zCCSPlayer::OnCSTrigger(const zSTRING& target, zCVob*)
{
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next) {
        if (entry->data->GetCSName() == target)
            entry->data->HandleTrigger();
    }
}

void zCCSPlayer::OnCSUntrigger(const zSTRING& target, zCVob*)
{
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next) {
        if (entry->data->GetCSName() == target)
            entry->data->HandleUntrigger();
    }
}

void zCCSPlayer::Archive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    archiver.WriteInt(lastProcessDay);
    archiver.WriteInt(lastProcessHour);
    archiver.WriteInt(playList.GetNumInList());
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next)
        archiver.WriteObject(entry->data);
}

void zCCSPlayer::Unarchive(zCArchiver& archiver)
{
    while (playList.next) {
        if (playList.next->data)
            playList.next->data->Release();
        playList.Remove(playList.next->data);
    }
    if (!archiver.InSaveGame())
        return;
    lastProcessDay = archiver.ReadInt();
    lastProcessHour = archiver.ReadInt();
    int count = archiver.ReadInt();
    for (int i = 0; i < count; ++i)
        InsertCutscene(dynamic_cast<zCCSCutsceneContext*>(archiver.ReadObject(NULL)));
}

int zCCSPoolItem::IsAllowedToPlay()
{
    return runBehaviour == 0 || runBehaviourValue > numPlayed;
}

void zCCSPoolItem::SetRunBehaviour(int behaviour, int value)
{
    runBehaviour = behaviour;
    runBehaviourValue = value;
}

int zCCSPoolItem::GetRunBehaviour()
{
    return runBehaviour;
}

int zCCSPoolItem::GetRunBehaviour(int& value)
{
    value = runBehaviourValue;
    return runBehaviour;
}

void zCCSPoolItem::TestedDeactivation()
{
    if (runBehaviour == 1 && numPlayed >= runBehaviourValue)
        deactivated = 1;
}

int zCCSPoolItem::HasFlags(int flags) const
{
    return (this->flags & flags) != 0;
}

void zCCSPoolItem::SetFlags(int flags)
{
    this->flags |= flags;
}

void zCCSPoolItem::ClrFlags(int flags)
{
    this->flags &= ~flags;
}

int zCCSManager::IsDeactivated(zSTRING& name)
{
    zCCSPoolItem* item = PoolFindItem(name);
    return item ? item->deactivated : 0;
}

void zCCSManager::PoolCountStart(zSTRING& name)
{
    zCCSPoolItem* item = PoolFindItem(name);
    if (item) {
        ++item->numPlayed;
        ++item->playCounter;
    }
}

void zCCSManager::PoolCountStop(zSTRING& name)
{
    zCCSPoolItem* item = PoolFindItem(name);
    if (item)
        item->TestedDeactivation();
}

int zCCSManager::PoolHasFlags(zSTRING& name, int flags)
{
    zCCSPoolItem* item = PoolFindItem(name);
    return item && item->HasFlags(flags);
}

void zCCSManager::PoolClrFlags(const zSTRING& name, int flags)
{
    zCCSPoolItem* item = PoolFindItem(name);
    if (item)
        item->ClrFlags(flags);
}

void zCCSManager::PoolSetFlags(zSTRING& name, int flags)
{
    zCCSPoolItem* item = PoolFindItem(name);
    if (item)
        item->SetFlags(flags);
}

int zCCSManager::PoolNumPlayed(zSTRING& name)
{
    zCCSPoolItem* item = PoolFindItem(name);
    return item ? item->playCounter : 0;
}

int zCCSManager::PoolAllowedToStart(zSTRING& name)
{
    zCCSPoolItem* item = PoolFindItem(name);
    return item ? item->IsAllowedToPlay() : -1;
}

void zCCSManager::PoolResetAll()
{
    for (int i = 0; i < csPool.numInArray; ++i)
        csPool.parray[i]->numPlayed = 0;
}

void zCCSManager::PoolResetByHour()
{
    for (int i = 0; i < csPool.numInArray; ++i) {
        if (csPool.parray[i]->GetRunBehaviour() == 2)
            csPool.parray[i]->numPlayed = 0;
    }
}

void zCCSManager::PoolResetByDay()
{
    for (int i = 0; i < csPool.numInArray; ++i) {
        if (csPool.parray[i]->GetRunBehaviour() == 3)
            csPool.parray[i]->numPlayed = 0;
    }
}

int zCCSManager::GetNumOfShortCom()
{
    return commandStrings.numInArray;
}

zSTRING zCCSManager::GetShortCom(int index)
{
    if (index < 0 || index >= commandStrings.numInArray)
        return zSTRING("");
    return commandStrings.parray[index];
}

zCCSPoolItem* zCCSManager::PoolFindItem(const zSTRING& name)
{
    for (int i = 0; i < csPool.numInArray; ++i) {
        if (csPool.parray[i] && csPool.parray[i]->poolName == name)
            return csPool.parray[i];
    }
    return NULL;
}

void zCCSManager::PoolInsertItem(zCCSPoolItem* item)
{
    csPool.InsertEnd(item);
}

zCCSCutsceneContext* zCCSManager::InitiateCSwithContext(zCCSCutsceneContext* context)
{
    zCCSPoolItem* item = PoolFindItem(context->GetCSName());
    if (item) {
        if (item->IsAllowedToPlay() && !item->deactivated) {
            context->allowedToPlay = item->IsAllowedToPlay();
            context->SetPoolItem(item);
        }
        return context;
    }
    item = new zCCSPoolItem(context);
    zCClassDef::ObjectCreated(item, zCCSPoolItem::classDef);
    item->SetRunBehaviour(context->properties->runBehaviour,
        context->properties->runBehaviourValue);
    PoolInsertItem(item);
    context->SetPoolItem(item);
    return context;
}

zCCSCutsceneContext* zCCSManager::InitiateOUwithContext(zCCSCutsceneContext* context)
{
    if (context->reference >= 0)
        context->UseThisSubBlock(LibGet(context->reference));
    return context;
}

zCCSCutsceneContext* zCCSManager::CreateNewCutscene(zSTRING& name)
{
    zCCSPoolItem* item = PoolFindItem(name);
    if (item && (!item->IsAllowedToPlay() || item->deactivated))
        return NULL;
    zCCSCutsceneContext* context = new zCCSCutsceneContext(name);
    zCClassDef::ObjectCreated(context, zCCSCutsceneContext::classDef);
    if (item)
        context->allowedToPlay = item->IsAllowedToPlay();
    else {
        item = new zCCSPoolItem(context);
        zCClassDef::ObjectCreated(item, zCCSPoolItem::classDef);
        item->SetRunBehaviour(context->properties->runBehaviour,
            context->properties->runBehaviourValue);
        PoolInsertItem(item);
    }
    context->SetPoolItem(item);
    return context;
}

zCCSCutsceneContext* zCCSManager::CreateNewOutputUnit(int unit)
{
    if (!LibValidateOU(unit))
        return NULL;
    zCCSBlock* block = LibGet(unit);
    zCCSCutsceneContext* context = new zCCSCutsceneContext(block, unit, 1);
    zCClassDef::ObjectCreated(context, zCCSCutsceneContext::classDef);
    return context;
}

int zCCSManager::LibValidateOU(int index)
{
    return index >= 0 && index < library->ouList.numInArray;
}

zCCSPlayer::zCCSPlayer()
    : lastProcessDay(0), lastProcessHour(0), processingListItem(NULL), processingCtr(0), debugInfoOnScreen(0),
      debugInfoScreen(NULL), timerCtr(0.0f), ownerManager(NULL), ownerWorld(NULL)
{
    playList.data = NULL;
    playList.next = NULL;
}

zCCSPlayer::~zCCSPlayer()
{
    StopAllCutscenes(NULL);
    while (playList.next) {
        if (playList.next->data)
            playList.next->data->Release();
        playList.Remove(playList.next->data);
    }
    if (debugInfoScreen) {
        delete debugInfoScreen;
        debugInfoScreen = NULL;
    }
    ownerWorld = NULL;
    ownerManager = NULL;
}

zCObject* zCCSPlayer::_CreateNewInstance()
{
    zCCSPlayer* player = new zCCSPlayer;
    zCClassDef::ObjectCreated(player, classDef);
    return player;
}

zCClassDef* zCCSPlayer::_GetClassDef() const
{
    return classDef;
}

void zCCSPlayer::SetOwnerWorld(zCWorld* world)
{
    ownerWorld = world;
}

void zCCSPlayer::SetOwnerManager(zCCSManager* manager)
{
    ownerManager = manager;
}

zCCSCutsceneContext* zCCSPlayer::StartCutscene(zCCutscene* cutscene, zCVob* vob, zSTRING name)
{
    name.Upper();
    zSTRING upperName = name;
    upperName.Upper();
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next) {
        if (entry->data->AreU(upperName))
            return entry->data;
    }
    if (zerr->filter_level >= 7)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + upperName
            + "\": Start new cutscene: Creating new Cutscene-Instance ...", 7, 1, 0x6A, 0, 0);
    zCCSCutsceneContext* context = new zCCSCutsceneContext(cutscene, -1, 0);
    zCClassDef::ObjectCreated(context, zCCSCutsceneContext::classDef);
    context->SetRoleListForDebugging(cutscene->roleList);
    context->properties = dynamic_cast<zCCSProps*>(cutscene->properties->CreateCopy());
    if (context->properties) {
        context->properties->name = upperName;
        context->properties->name.Upper();
    }
    zCCSCutsceneContext* result = NULL;
    if (context->AddMainRole(vob)) {
        if (ownerManager) {
            ownerManager->InitiateCSwithContext(context);
            context->properties->ignore = 0;
        } else {
            context->properties->ignore = 1;
        }
        context->SetOwnerPlayer(this);
        InsertCutscene(context);
        result = context;
    } else {
        context->Stop();
    }
    context->Release();
    if (zerr->filter_level >= 7)
        zerr->Report(zERR_TYPE_INFO, 0, "", 7, 2, 0x8C, 0, 0);
    return result;
}

zCCSProps* zCCutscene::GetProperties()
{
    return properties;
}

zCCSCutsceneContext* zCCSPlayer::StartCutscene(const zSTRING& name, zCVob* vob)
{
    if (zCCSCutsceneContext::playDisabled)
        return NULL;
    zSTRING& cutsceneName = const_cast<zSTRING&>(name);
    cutsceneName.Upper();
    zSTRING upperName = cutsceneName;
    upperName.Upper();
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next) {
        if (entry->data->AreU(upperName))
            return NULL;
    }
    zCCSCutsceneContext* context = ownerManager->CreateNewCutscene(cutsceneName);
    zCCSCutsceneContext* result = NULL;
    if (context) {
        if (context->loaded) {
            if (context->AddMainRole(vob)) {
                if (zerr->filter_level >= 4)
                    zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + cutsceneName
                        + "\": Start new cutscene", 4, 0, 0xAF, 0, 0);
                context->SetOwnerPlayer(this);
                InsertCutscene(context);
                result = context;
            } else {
                context->Stop();
            }
        }
        context->Release();
    }
    if (zerr->filter_level >= 8)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: New Cutscene created and started", 8, 0, 0xBC, 0, 0);
    return result;
}

zCCSCutsceneContext* zCCSPlayer::CreateNewOutputUnit(int unit, zCVob*)
{
    if (!ownerManager->LibValidateOU(unit))
        return NULL;
    zCCSCutsceneContext* context = ownerManager->CreateNewOutputUnit(unit);
    if (context) {
        context->SetOwnerPlayer(this);
        playList.Insert(context);
    }
    return context;
}

zCCSCutsceneContext* zCCSPlayer::StartOutputUnit(int unit, zCVob* vob)
{
    const zSTRING reference = zSTRING(unit);
    if (!ownerManager->LibValidateOU(unit)) {
        zerr->Report(zERR_TYPE_WARN, 0, "B: CS \"" + reference
            + "\": Start new output-unit: reference not found", 0, 0, 0xEE, 0, 0);
        return NULL;
    }
    if (ownerManager->LibIsSvmModuleRunning(unit)) {
        if (zerr->filter_level >= 4)
            zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + reference
                + "\": *** SVM-Module ignored ***", 4, 0, 0xF4, 0, 0);
        return NULL;
    }
    for (zCList<zCCSCutsceneContext>* entry = playList.next; entry; entry = entry->next) {
        if (entry->data->AreU(unit)) {
            if (zerr->filter_level >= 7)
                zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + reference
                    + "\": Start new output-unit: is already running.", 7, 0, 0xFF, 0, 0);
            return NULL;
        }
    }
    if (zerr->filter_level >= 7)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + reference
            + "\": Starting new output-unit now", 7, 0, 0x106, 0, 0);
    zCCSCutsceneContext* context = ownerManager->CreateNewOutputUnit(unit);
    if (context) {
        context->properties->name = reference;
        context->properties->name.Upper();
        InsertCutscene(context);
        context->AddRoleOU(vob);
        context->StartPlaying();
        context->Release();
    }
    if (zerr->filter_level >= 8)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: New Outputunit created and started", 8, 0, 0x118, 0, 0);
    return context;
}

zCCSManager::~zCCSManager()
{
    for (int i = 0; i < csPool.numInArray; ++i) {
        if (csPool.parray[i]) {
            csPool.parray[i]->Release();
            csPool.parray[i] = NULL;
        }
    }
    csPool.numInArray = 0;
    for (int i = 0; i < csPlayerList.numInArray; ++i) {
        if (csPlayerList.parray[i]) {
            csPlayerList.parray[i]->Release();
            csPlayerList.parray[i] = NULL;
        }
    }
    csPlayerList.numInArray = 0;
    if (library) {
        library->Release();
        library = NULL;
    }
}

zCClassDef* zCCSManager::_GetClassDef() const
{
    return classDef;
}

zCEventMessage* zCCSManager::CreateMessage(int)
{
    return NULL;
}

zCEventMessage* zCCSManager::CreateOuMessage()
{
    return NULL;
}

zCCSPlayer* zCCSManager::CreateCutscenePlayer(zCWorld* world)
{
    zCCSPlayer* player = new zCCSPlayer;
    zCClassDef::ObjectCreated(player, zCCSPlayer::classDef);
    player->ownerWorld = world;
    InsertPlayerInList(player);
    return player;
}

void zCCSPlayer::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void zCCSManager::RemoveCutscenePlayer(zCCSPlayer* player)
{
    RemovePlayerFromList(player);
}

void zCCSManager::RemovePlayerFromList(zCCSPlayer* player)
{
    player->SetOwnerManager(NULL);
    csPlayerList.Remove(player);
    player->Release();
}

void zCCSManager::Archive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    archiver.WriteInt(csPool.numInArray);
    for (int i = 0; i < csPool.numInArray; ++i)
        archiver.WriteObject(csPool.parray[i]);
}

void zCCSManager::Unarchive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    for (int i = 0; i < csPool.numInArray; ++i) {
        if (csPool.parray[i]) {
            csPool.parray[i]->Release();
            csPool.parray[i] = NULL;
        }
    }
    csPool.numInArray = 0;
    int count = archiver.ReadInt();
    for (int i = 0; i < count; ++i)
        csPool.InsertEnd(dynamic_cast<zCCSPoolItem*>(archiver.ReadObject(NULL)));
}

void zCCSManager::LibLoad(int format)
{
    if (!library)
        return;
    library->DeleteLib();
    zSTRING baseName;
    if (zgameoptions)
        baseName = zgameoptions->ReadString("FILES", "OutputUnits", "");
    if (baseName.IsEmpty())
        baseName = "OU";
    const zSTRING directory = zoptions->GetDirString(6);
    if (format & 2) {
        zFILE* file = zfactory->CreateZFile(directory + baseName + ".bin");
        if (file->Exists()) {
            if (zerr->filter_level >= 5)
                zerr->Report(zERR_TYPE_INFO, 0, "B: SPC: Loading binary output-library "
                    + file->GetFullPath() + " ...", 5, 0, 0x183, 0, 0);
            file->Open(0);
            zCArchiver* archiver = zarcFactory->CreateArchiverRead(file, 0);
            if (archiver) {
                library->Unarchive(*archiver);
                archiver->Close();
                archiver->Release();
            }
            file->Close();
            library->loaded = 1;
        }
        delete file;
    }
    if (!library->loaded && (format & 1)) {
        zFILE* file = zfactory->CreateZFile(directory + baseName + ".csl");
        if (file->Exists()) {
            if (zerr->filter_level >= 5)
                zerr->Report(zERR_TYPE_INFO, 0, "B: SPC: Loading ascii output-library "
                    + file->GetFullPath() + " ...", 5, 0, 0x19E, 0, 0);
            file->Open(0);
            zCArchiver* archiver = zarcFactory->CreateArchiverRead(file, 0);
            if (archiver) {
                library->Unarchive(*archiver);
                archiver->Close();
                archiver->Release();
            }
            file->Close();
            library->loaded = 1;
        }
        delete file;
    }
    if (!library->loaded)
        zerr->Fault("N: CS: File for output-units not found: " + baseName + ".bin or "
            + baseName + ".csl");
}

void zCCSManager::LibCheckLoaded(int format)
{
    if (!library->IsLoaded())
        LibLoad(format);
}

void zCCSManager::LibStore(int format)
{
    zSTRING baseName;
    if (zgameoptions)
        baseName = zgameoptions->ReadString("FILES", "OutputUnits", "");
    if (baseName.IsEmpty())
        baseName = "OU";
    if (!library->IsLoaded())
        return;
    const zSTRING directory = zoptions->GetDirString(6);
    if (format & 1) {
        if (zerr->filter_level >= 4)
            zerr->Report(zERR_TYPE_INFO, 0, "B: CSL: Writing ascii version of Cutscene-Library (OutputUnits) to "
                + directory + baseName + ".csl ...", 4, 0, 0x215, 0, 0);
        zCArchiver* archiver = zarcFactory->CreateArchiverWrite(directory + baseName + ".csl", 1, 0, 0);
        if (archiver) {
            library->Archive(*archiver);
            archiver->Close();
            archiver->Release();
        }
    }
    if (format & 2) {
        if (zerr->filter_level >= 4)
            zerr->Report(zERR_TYPE_INFO, 0, "B: CSL: Writing binary version of Cutscene-Library (OutputUnits) to "
                + directory + baseName + ".bin ...", 4, 0, 0x222, 0, 0);
        zCArchiver* archiver = zarcFactory->CreateArchiverWrite(directory + baseName + ".bin", 3, 0, 0);
        if (archiver) {
            library->Archive(*archiver);
            archiver->Close();
            archiver->Release();
        }
    }
}

int zCCSManager::LibValidateOU(int index)
{
    LibCheckLoaded(3);
    return index >= 0 && index < library->ouList.numInArray && library->GetOU(index);
}

int zCCSManager::LibValidateOU(zSTRING& name)
{
    LibCheckLoaded(3);
    return library->ValidateToken(name);
}

void zCCSManager::LibForceToLoad()
{
    LibCheckLoaded(3);
}

int zCCSManager::LibIsLoaded()
{
    return library->IsLoaded();
}

zCCSProps* zCCSManager::CreateProperties()
{
    zCCSProps* properties = new zCCSProps;
    zCClassDef::ObjectCreated(properties, zCCSProps::classDef);
    return properties;
}

zCCSBlock* zCCSManager::LibGetFirst()
{
    LibCheckLoaded(3);
    iterator = 0;
    while (iterator < library->ouList.numInArray && !library->GetOU(iterator))
        ++iterator;
    return iterator < library->ouList.numInArray ? library->GetOU(iterator) : NULL;
}

zCCSBlock* zCCSManager::LibGetNext()
{
    if (!library->IsLoaded())
        return NULL;
    ++iterator;
    while (iterator < library->ouList.numInArray && !library->GetOU(iterator))
        ++iterator;
    return iterator < library->ouList.numInArray ? library->GetOU(iterator) : NULL;
}

zCCSBlock* zCCSManager::LibGetFirst(int& index)
{
    zCCSBlock* block = LibGetFirst();
    index = iterator;
    return block;
}

zCCSBlock* zCCSManager::LibGetNext(int& index)
{
    zCCSBlock* block = LibGetNext();
    index = iterator;
    return block;
}

int zCCSManager::LibAddOU(zCCSBlock* block)
{
    LibCheckLoaded(3);
    return library->Add(block);
}

void zCCSManager::LibDelOU(int index)
{
    LibCheckLoaded(3);
    library->RemoveFromLib(index, 1);
}

void zCCSManager::LibNullOU(int index)
{
    LibCheckLoaded(3);
    library->RemoveFromLib(index, 0);
}

zCCSBlock* zCCSManager::LibGet(int index)
{
    LibCheckLoaded(3);
    iterator = index;
    return index < library->ouList.numInArray ? library->GetOU(index) : NULL;
}

zSTRING zCCSManager::LibGetSvmModuleName(int index)
{
    zCCSBlock* block = LibGet(index);
    if (!block)
        return "";
    const zSTRING roleName = block->GetRoleName();
    if (roleName.PickWord(1, "_", "_") != "SVM")
        return "";
    return roleName.PickWord(3, "_", "_");
}

int zCCSManager::LibIsSvmModuleRunning(int index)
{
    zSTRING name = LibGetSvmModuleName(index);
    if (name.IsEmpty())
        return 0;
    return libSvmModuleList.IsInList(name);
}

int zCCSManager::LibSvmModuleStart(int index)
{
    zSTRING name = LibGetSvmModuleName(index);
    if (name.IsEmpty() || libSvmModuleList.IsInList(name))
        return 0;
    libSvmModuleList.InsertEnd(name);
    return 1;
}

int zCCSManager::LibSvmModuleStop(int index)
{
    zSTRING name = LibGetSvmModuleName(index);
    if (name.IsEmpty())
        return 1;
    for (int i = 0; i < libSvmModuleList.numInArray; ++i) {
        if (libSvmModuleList.parray[i] == name) {
            libSvmModuleList.RemoveOrderIndex(i);
            return 1;
        }
    }
    return 0;
}

void zCCSManager::AddCommand(const zSTRING& command)
{
    commandStrings.InsertEnd(command);
}

void zCCSManager::InsertPlayerInList(zCCSPlayer* player)
{
    player->SetOwnerManager(this);
    csPlayerList.InsertEnd(player);
    player->AddRef();
}

zCCSPlayer* oCCSManager::CreateCutscenePlayer(zCWorld* world)
{
    oCCSPlayer* player = new oCCSPlayer;
    zCClassDef::ObjectCreated(player, oCCSPlayer::classDef);
    player->ownerWorld = world;
    InsertPlayerInList(player);
    return player;
}

zCCSProps* oCCSManager::CreateProperties()
{
    oCCSProps* properties = new oCCSProps;
    zCClassDef::ObjectCreated(properties, oCCSProps::classDef);
    return properties;
}

oCCSManager::oCCSManager()
{
    zCEventCore* coreMessage = new zCEventCore;
    AddCommand(coreMessage->MD_GetSubTypeString(0));
    AddCommand(zSTRING(""));
    coreMessage->Release();

    oCMsgMovement* movementMessage = new oCMsgMovement(0, 0);
    const int movementTypes[] = { 2, 3, 14, 6, 9 };
    for (int i = 0; i < 5; ++i)
        AddCommand(movementMessage->MD_GetSubTypeString(movementTypes[i]));
    AddCommand(zSTRING(""));
    movementMessage->Release();

    oCMsgConversation* conversationMessage = new oCMsgConversation(0);
    for (int i = 0; i < 4; ++i)
        AddCommand(conversationMessage->MD_GetSubTypeString(i));
    AddCommand(zSTRING(""));
    conversationMessage->Release();

    oCMsgManipulate* manipulateMessage = new oCMsgManipulate(0);
    AddCommand(manipulateMessage->MD_GetSubTypeString(4));
    AddCommand(manipulateMessage->MD_GetSubTypeString(5));
    AddCommand(zSTRING(""));
    manipulateMessage->Release();

    zCEventScreenFX* screenMessage = new zCEventScreenFX;
    AddCommand(screenMessage->MD_GetSubTypeString(0));
    AddCommand(screenMessage->MD_GetSubTypeString(1));
    AddCommand(zSTRING(""));
    screenMessage->Release();

    zCCSCamera_EventMsgActivate* activateMessage = new zCCSCamera_EventMsgActivate;
    AddCommand(activateMessage->MD_GetSubTypeString(1));
    AddCommand(activateMessage->MD_GetSubTypeString(2));
    AddCommand(zSTRING(""));
    activateMessage->Release();

    zCCSCamera_EventMsg* cameraMessage = new zCCSCamera_EventMsg;
    AddCommand(cameraMessage->MD_GetSubTypeString(1));
    AddCommand(cameraMessage->MD_GetSubTypeString(4));
    AddCommand(zSTRING(""));
    cameraMessage->Release();
}

zCObject* oCCSManager::_CreateNewInstance()
{
    oCCSManager* manager = new oCCSManager;
    zCClassDef::ObjectCreated(manager, classDef);
    return manager;
}

void oCCSManager::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* oCCSManager::_GetClassDef() const
{
    return classDef;
}

oCCSManager::~oCCSManager()
{
}

zCEventMessage* oCCSManager::CreateOuMessage()
{
    return CreateMessage(8);
}

template <class T>
static T* CreateCutsceneMessage(T* message, zCClassDef* classDef)
{
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

zCEventMessage* oCCSManager::CreateMessage(int command)
{
    switch (command) {
    case 0:
        return CreateCutsceneMessage(new zCEventCore, zCEventCore::classDef);
    case 2:
        return CreateCutsceneMessage(new oCMsgMovement(2, 0), oCMsgMovement::classDef);
    case 3:
        return CreateCutsceneMessage(new oCMsgMovement(3, 0), oCMsgMovement::classDef);
    case 4:
        return CreateCutsceneMessage(new oCMsgMovement(14, 0), oCMsgMovement::classDef);
    case 5:
        return CreateCutsceneMessage(new oCMsgMovement(6, 0), oCMsgMovement::classDef);
    case 6:
        return CreateCutsceneMessage(new oCMsgMovement(9, 0), oCMsgMovement::classDef);
    case 8:
    case 9:
    case 10:
    case 11:
        return CreateCutsceneMessage(new oCMsgConversation(command - 8), oCMsgConversation::classDef);
    case 13:
        return CreateCutsceneMessage(new oCMsgManipulate(4), oCMsgManipulate::classDef);
    case 14:
        return CreateCutsceneMessage(new oCMsgManipulate(5), oCMsgManipulate::classDef);
    case 16:
        return CreateCutsceneMessage(new zCEventScreenFX(zCEventScreenFX::zEVENT_BLEND_FADEIN,
            0.0f, zCOLOR(0), 0.0f), zCEventScreenFX::classDef);
    case 17:
        return CreateCutsceneMessage(new zCEventScreenFX(zCEventScreenFX::zEVENT_BLEND_FADEOUT,
            0.0f, zCOLOR(0), 0.0f), zCEventScreenFX::classDef);
    case 19:
        return CreateCutsceneMessage(new zCCSCamera_EventMsgActivate(
            zCCSCamera_EventMsgActivate::EV_SETCAMREFERENCE), zCCSCamera_EventMsgActivate::classDef);
    case 20:
        return CreateCutsceneMessage(new zCCSCamera_EventMsgActivate(
            zCCSCamera_EventMsgActivate::EV_SETTARGETREFERENCE), zCCSCamera_EventMsgActivate::classDef);
    case 22:
        return CreateCutsceneMessage(new zCCSCamera_EventMsg(zCCSCamera_EventMsg::EV_PLAY),
            zCCSCamera_EventMsg::classDef);
    case 23:
        return CreateCutsceneMessage(new zCCSCamera_EventMsg(zCCSCamera_EventMsg::EV_STOP),
            zCCSCamera_EventMsg::classDef);
    }
    return NULL;
}

zCObject* oCObjectFactory::_CreateNewInstance()
{
    oCObjectFactory* factory = new oCObjectFactory;
    zCClassDef::ObjectCreated(factory, classDef);
    return factory;
}

zCEventManager* oCObjectFactory::CreateEventManager(zCVob* vob)
{
    zCEventManager* manager = new zCEventManager(vob);
    zCClassDef::ObjectCreated(manager, zCEventManager::classDef);
    return manager;
}

zCCSManager* oCObjectFactory::CreateCSManager()
{
    oCCSManager* manager = new oCCSManager;
    zCClassDef::ObjectCreated(manager, oCCSManager::classDef);
    return manager;
}

oCGameInfo* oCObjectFactory::CreateGameInfo()
{
    return new oCGameInfo;
}

oCPlayerInfo* oCObjectFactory::CreatePlayerInfo()
{
    return new oCPlayerInfo;
}

zCWorld* oCObjectFactory::CreateWorld()
{
    oCWorld* world = new oCWorld;
    zCClassDef::ObjectCreated(world, oCWorld::classDef);
    return world;
}

void oCWorld::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

oCNpc* oCObjectFactory::CreateNpc(int instance)
{
    oCNpc* npc = new oCNpc;
    zCClassDef::ObjectCreated(npc, oCNpc::classDef);
    if (instance != -1)
        npc->InitByScript(instance, 0);
    return npc;
}

oCItem* oCObjectFactory::CreateItem(int instance)
{
    oCItem* item = new oCItem;
    zCClassDef::ObjectCreated(item, oCItem::classDef);
    if (instance != -1)
        item->InitByScript(instance, 1);
    return item;
}

zCWaypoint* oCObjectFactory::CreateWaypoint()
{
    oCWaypoint* waypoint = new oCWaypoint;
    zCClassDef::ObjectCreated(waypoint, oCWaypoint::classDef);
    return waypoint;
}

zCWay* oCObjectFactory::CreateWay()
{
    return new oCWay;
}
