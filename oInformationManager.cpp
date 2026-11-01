#include "include/oInformationManager.h"

static int informationState = -1;
static int dialogChoiceState = -1;
static zSTRING viewDialogPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING viewDialogPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING viewDialogPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING viewDialogPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING viewDialogPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING viewDialogPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING viewDialogPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING viewDialogPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING viewDialogPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING viewDialogPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float dialogFarDistance = 250000.0f * 0.0166666675f;
static float dialogNearDistance = dialogFarDistance * 0.0166666675f;
static float dialogFarPlaneDistance = 250000.0f * 24.0f;
static zSTRING informationPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING informationPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING informationPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING informationPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING informationPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING informationPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING informationPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING informationPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING informationPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING informationPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
oCNpc* oCNpc::player;

zCViewDialogChoice::zCViewDialogChoice()
    : DurationOpen(0.0f), DurationClose(0.0f), ModeOpen(0), ModeClose(0), opened(0)
{
}

zCViewDialogChoice::~zCViewDialogChoice()
{
}

void zCViewDialogChoice::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

void zCViewDialogChoice::RemoveAllChoices()
{
    choices.numInArray = 0;
    choiceData.numInArray = 0;
}

void zCViewDialogChoice::AddChoice(const zSTRING& text, int userData)
{
    choices.InsertEnd(text);
    choiceData.InsertEnd(userData);
}

oCViewDialogTrade::oCViewDialogTrade()
    : NpcLeft(0), NpcRight(0), DurationOpen(0.0f), DurationClose(0.0f), ModeOpen(0), ModeClose(0),
      settingsCounter(0)
{
}

void oCViewDialogTrade::SetNpcLeft(oCNpc* npc)
{
    NpcLeft = npc;
}

void oCViewDialogTrade::SetNpcRight(oCNpc* npc)
{
    NpcRight = npc;
}

oCViewDialogTrade::~oCViewDialogTrade()
{
}

void oCViewDialogTrade::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

void oCViewDialogTrade::UpdateViewSettings()
{
    ++settingsCounter;
}

oCInformationManager::oCInformationManager()
    : LastMethod("Constructor"), Npc(0), Player(0), Info(0), IsDone(1), IsWaitingForEnd(0),
      IsWaitingForScript(0), IsWaitingForOpen(0), IsWaitingForClose(0), IsWaitingForSelection(0),
      MustOpen(1), IndexBye(-1), ImportantCurrent(0), ImportantMax(0), Mode(INFO_MGR_MODE_IMPORTANT)
{
    DlgStatus = new zCViewDialogChoice;
    DlgChoice = new zCViewDialogChoice;
    DlgTrade = new oCViewDialogTrade;
    zCClassDef::ObjectCreated(DlgStatus, zCViewDialogChoice::classDef);
    zCClassDef::ObjectCreated(DlgChoice, zCViewDialogChoice::classDef);
    zCClassDef::ObjectCreated(DlgTrade, oCViewDialogTrade::classDef);
    DlgStatus->RemoveAllChoices();
    DlgChoice->RemoveAllChoices();
    UpdateViewSettings();
}

oCInformationManager::~oCInformationManager()
{
    LastMethod = "Destructor";
    delete DlgStatus;
    delete DlgChoice;
    delete DlgTrade;
    DlgStatus = 0;
    DlgChoice = 0;
    DlgTrade = 0;
}

oCInformationManager& oCInformationManager::GetInformationManager()
{
    static oCInformationManager manager;
    return manager;
}

void oCInformationManager::UpdateViewSettings()
{
    DlgStatus->DurationOpen = 1.0f;
    DlgStatus->DurationClose = 1.0f;
    DlgStatus->ModeOpen = 2;
    DlgStatus->ModeClose = 2;
    DlgChoice->DurationOpen = 500.0f;
    DlgChoice->DurationClose = 500.0f;
    DlgChoice->ModeOpen = 1;
    DlgChoice->ModeClose = 1;
    DlgTrade->DurationOpen = 1.0f;
    DlgTrade->DurationClose = 1.0f;
    DlgTrade->ModeOpen = 1;
    DlgTrade->ModeClose = 1;
    DlgTrade->UpdateViewSettings();
}

void oCInformationManager::ToggleStatus()
{
    DlgStatus->opened = !DlgStatus->opened;
    if (DlgStatus->opened)
        PrintStatus();
}

static zSTRING InfoBool(int value)
{
    return value ? zSTRING("TRUE") : zSTRING("FALSE");
}

void oCInformationManager::PrintStatus()
{
    DlgStatus->RemoveAllChoices();
    DlgStatus->AddChoice(zSTRING("IsDone:                ") + InfoBool(IsDone), 0);
    DlgStatus->AddChoice(zSTRING("IsWaitingForEnd:       ") + InfoBool(IsWaitingForEnd), 0);
    DlgStatus->AddChoice(zSTRING("IsWaitingForScript:    ") + InfoBool(IsWaitingForScript), 0);
    DlgStatus->AddChoice(zSTRING("IsWaitingForOpen:      ") + InfoBool(IsWaitingForOpen), 0);
    DlgStatus->AddChoice(zSTRING("IsWaitingForClose:     ") + InfoBool(IsWaitingForClose), 0);
    DlgStatus->AddChoice(zSTRING("IsWaitingForSelection: ") + InfoBool(IsWaitingForSelection), 0);
    DlgStatus->AddChoice(zSTRING("MustOpen:              ") + InfoBool(MustOpen), 0);
    const char* modeName = "INVALID";
    if (Mode == INFO_MGR_MODE_IMPORTANT)
        modeName = "INFO_MGR_MODE_IMPORTANT";
    else if (Mode == INFO_MGR_MODE_INFO)
        modeName = "INFO_MGR_MODE_INFO";
    else if (Mode == INFO_MGR_MODE_CHOICE)
        modeName = "INFO_MGR_MODE_CHOICE";
    else if (Mode == INFO_MGR_MODE_TRADE)
        modeName = "INFO_MGR_MODE_TRADE";
    DlgStatus->AddChoice(zSTRING("Mode:                  ") + zSTRING(modeName), 0);
    DlgStatus->AddChoice(zSTRING("IndexBye:              ") + zSTRING(IndexBye), 0);
    DlgStatus->AddChoice(zSTRING("ImportantCurrent:      ") + zSTRING(ImportantCurrent), 0);
    DlgStatus->AddChoice(zSTRING("ImportantMax:          ") + zSTRING(ImportantMax), 0);
    if (Npc)
        DlgStatus->AddChoice(zSTRING("Npc:\t\t\t\t\t ") + Npc->GetName(), 0);
    DlgStatus->AddChoice(zSTRING(""), 0);
    if (Player)
        DlgStatus->AddChoice(zSTRING("Player:\t\t\t\t ") + Player->GetName(), 0);
    DlgStatus->AddChoice(zSTRING(""), 0);
    if (Info)
        DlgStatus->AddChoice(zSTRING("Info:\t\t\t\t\t ") + Info->GetText(), 0);
    DlgStatus->AddChoice(zSTRING("Last Method:\t\t\t ") + LastMethod, 0);
}

int oCInformationManager::HasFinished()
{
    return IsDone;
}

int oCInformationManager::WaitingForEnd()
{
    return IsWaitingForEnd;
}

void oCInformationManager::SetNpc(oCNpc* npc)
{
    LastMethod = "SetNpc";
    if (!IsDone)
        return;
    Npc = npc;
    Player = oCNpc::player;
    Info = 0;
    IsWaitingForEnd = 0;
    IsWaitingForOpen = 0;
    IsWaitingForClose = 0;
    IsWaitingForSelection = 0;
    IndexBye = -1;
    ImportantCurrent = 0;
    Mode = INFO_MGR_MODE_IMPORTANT;
    IsDone = 0;
    IsWaitingForScript = 1;
    LastMethod = "OnImportantBegin";
}

void oCInformationManager::Update()
{
    LastMethod = "Update";
    if (!Npc || !Player || IsDone)
        return;
    if (IsWaitingForScript) {
        InfoWaitForEnd();
        return;
    }
    if (IsWaitingForEnd) {
        OnTermination();
        return;
    }
    if (IsWaitingForClose) {
        IsWaitingForClose = 0;
        return;
    }
    if (MustOpen) {
        if (Mode == INFO_MGR_MODE_TRADE)
            DlgTrade->settingsCounter = 1;
        else
            DlgChoice->opened = 1;
        IsWaitingForOpen = 1;
        MustOpen = 0;
        return;
    }
    if (IsWaitingForOpen) {
        IsWaitingForOpen = 0;
        IsWaitingForSelection = 1;
        return;
    }
    if (IsWaitingForSelection) {
        IsWaitingForSelection = 0;
        IsWaitingForClose = 1;
        return;
    }
    LastMethod = "OnInfo";
    if (Mode == INFO_MGR_MODE_IMPORTANT) {
        ProcessNextImportant();
    } else if (Mode == INFO_MGR_MODE_INFO) {
        OnInfo(DlgChoice->ChoiceSelected);
    } else if (Mode == INFO_MGR_MODE_CHOICE) {
        if (Info && Info->listChoices.next) {
            zCList_oCInfoChoice* current = Info->listChoices.next;
            int index = 0;
            while (current && index != DlgChoice->ChoiceSelected) {
                current = current->next;
                ++index;
            }
            if (current)
                OnChoice(current->data);
        }
        OnChoiceBegin();
    } else if (Mode == INFO_MGR_MODE_TRADE) {
        OnTradeEnd();
    }
}

void oCInformationManager::Exit()
{
    LastMethod = "Exit";
    OnExit();
}

static int informationCameraMode;

void oCInformationManager::CameraStart()
{
    informationCameraMode = 1;
    CameraRefresh();
}

void oCInformationManager::CameraStop()
{
    informationCameraMode = 0;
}

void oCInformationManager::CameraRefresh()
{
}

void oCInformationManager::ProcessImportant()
{
    LastMethod = "ProcessImportant";
    IsWaitingForScript = 0;
    ImportantCurrent = 0;
    ImportantMax = 0;
    ProcessNextImportant();
}

void oCInformationManager::ProcessNextImportant()
{
    LastMethod = "ProcessNextImportant";
    if (ImportantCurrent >= ImportantMax) {
        OnImportantEnd();
        return;
    }
    Info = 0;
    ++ImportantCurrent;
    if (Info)
        OnInfo(Info);
}

void oCInformationManager::InfoWaitForEnd()
{
    LastMethod = "InfoWaitForEnd";
    IsWaitingForScript = 0;
}

void oCInformationManager::CollectInfos()
{
    LastMethod = "CollectInfos";
    if (!Npc)
        return;
    DlgChoice->RemoveAllChoices();
    if (ImportantMax > 0) {
        OnExit();
        return;
    }
    IndexBye = 0;
    DlgChoice->AddChoice("ENDE.", 0);
}

int oCInformationManager::CollectChoices(oCInfo* info)
{
    LastMethod = "CollectChoices";
    if (!info || !info->listChoices.next)
        return 0;
    DlgChoice->RemoveAllChoices();
    zCList_oCInfoChoice* current = info->listChoices.next;
    while (current) {
        if (current->data)
            DlgChoice->AddChoice(current->data->text, 0);
        current = current->next;
    }
    return 1;
}

void oCInformationManager::OnImportantBegin()
{
    LastMethod = "OnImportantBegin";
    Mode = INFO_MGR_MODE_IMPORTANT;
    IsWaitingForScript = 1;
}

void oCInformationManager::OnImportantEnd()
{
    LastMethod = "OnImportantEnd";
    OnInfoBegin();
}

void oCInformationManager::OnInfoBegin()
{
    LastMethod = "OnInfoBegin";
    Mode = INFO_MGR_MODE_INFO;
    MustOpen = 1;
    CollectInfos();
}

void oCInformationManager::OnInfo(int index)
{
    LastMethod = "OnInfo";
    if (index == IndexBye) {
        OnExit();
        return;
    }
    if (Info) {
        OnInfo(Info);
        if (Info->pd.trade)
            OnTradeBegin();
    }
}

void oCInformationManager::OnInfo(oCInfo* info)
{
    LastMethod = "OnInfo2";
    if (!info)
        return;
    Info = info;
    if (info->InfoConditions()) {
        info->Info();
        IsWaitingForScript = 1;
    }
    if (info->listChoices.next && CollectChoices(info)) {
        Mode = INFO_MGR_MODE_CHOICE;
        MustOpen = 1;
        return;
    }
    OnInfoBegin();
}

void oCInformationManager::OnChoiceBegin()
{
    LastMethod = "OnChoiceBegin";
    if (CollectChoices(Info)) {
        Mode = INFO_MGR_MODE_CHOICE;
        MustOpen = 1;
        return;
    }
    OnChoiceEnd();
}

void oCInformationManager::OnChoice(int index)
{
    LastMethod = "OnChoice";
    if (Info) {
        zCList_oCInfoChoice* current = Info->listChoices.next;
        int currentIndex = 0;
        while (current && currentIndex != index) {
            current = current->next;
            ++currentIndex;
        }
        if (current)
            OnChoice(current->data);
        OnChoiceBegin();
    }
}

void oCInformationManager::OnChoice(oCInfoChoice* choice)
{
    LastMethod = "OnChoice2";
    if (!choice)
        return;
    Info->RemoveChoice(choice);
    IsWaitingForScript = 1;
}

void oCInformationManager::OnChoiceEnd()
{
    LastMethod = "OnChoiceEnd";
    OnInfoBegin();
}

void oCInformationManager::OnInfoEnd()
{
    LastMethod = "OnInfoEnd";
    Mode = INFO_MGR_MODE_TRADE;
    OnExit();
}

void oCInformationManager::OnTradeBegin()
{
    LastMethod = "OnTradeBegin";
    Mode = INFO_MGR_MODE_TRADE;
    MustOpen = 1;
    DlgTrade->SetNpcLeft(Npc);
    DlgTrade->SetNpcRight(Player);
}

void oCInformationManager::OnTradeEnd()
{
    LastMethod = "OnTradeEnd";
    DlgTrade->SetNpcLeft(0);
    DlgTrade->SetNpcRight(0);
    OnInfoBegin();
}

void oCInformationManager::OnExit()
{
    LastMethod = "OnExit";
    DlgChoice->opened = 0;
    DlgTrade->settingsCounter = 0;
    IsWaitingForEnd = 1;
    MustOpen = 0;
}

void oCInformationManager::OnTermination()
{
    LastMethod = "OnTermination";
    CameraStop();
    Player = 0;
    Npc = 0;
    IsWaitingForEnd = 0;
    ImportantCurrent = 0;
    IsDone = 1;
}
