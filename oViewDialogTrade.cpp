#include "include/oViewDialogTrade.h"

static int tradeState = -1;
static zCClassDef oCViewDialogTradeClassDef("oCViewDialogTrade", "zCObject",
    oCViewDialogTrade::_CreateNewInstance, 0, 0x110, 0, "");
zCClassDef* oCViewDialogTrade::classDef = &oCViewDialogTradeClassDef;
static zSTRING tradePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING tradePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING tradePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING tradePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING tradePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING tradePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING tradePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING tradePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING tradePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING tradePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float tradeFarDistance = 250000.0f * 0.0166666675f;
static float tradeNearDistance = tradeFarDistance * 0.0166666675f;
static float tradeFarPlaneDistance = 250000.0f * 24.0f;
oCStealContainer* oCNpc::stealcontainer = 0;

zCObject* oCViewDialogTrade::_CreateNewInstance()
{
    oCViewDialogTrade* dialog = new oCViewDialogTrade;
    zCClassDef::ObjectCreated(dialog, classDef);
    return dialog;
}

void oCViewDialogTrade::UpdateViewSettings()
{
    int animated = zoptions->ReadBool(zOPT_SEC_GAME, "animatedWindows", 1);
    unsigned long mode = animated ? 2 : 0;
    float duration = animated ? 500.0f : 0.0f;
    DlgInventoryNpc->ModeOpen = mode;
    DlgInventoryNpc->ModeClose = mode;
    DlgInventoryNpc->DurationOpen = duration;
    DlgInventoryNpc->DurationClose = duration;
    DlgInventoryPlayer->ModeOpen = mode;
    DlgInventoryPlayer->ModeClose = mode;
    DlgInventoryPlayer->DurationOpen = duration;
    DlgInventoryPlayer->DurationClose = duration;
}

oCViewDialogTrade::oCViewDialogTrade()
    : DlgInventoryNpc(new oCViewDialogStealContainer),
      DlgInventoryPlayer(new oCViewDialogInventory),
      SectionTrade(TRADE_SECTION_LEFT_INVENTORY), NpcLeft(0), NpcRight(0), TransferCount(0)
{
    zCClassDef::ObjectCreated(DlgInventoryNpc, oCViewDialogStealContainer::classDef);
    zCClassDef::ObjectCreated(DlgInventoryPlayer, oCViewDialogInventory::classDef);
    zSTRING texture = "DEFAULT.TGA";
    DlgInventoryNpc->Init(this, 0, 0, 0, 0.0f, 0.0f, texture);
    zCPosition position = { 0, 0 };
    DlgInventoryNpc->SetVirtualPosition(position);
    zCPosition size = { 0x400, 0x2000 };
    DlgInventoryNpc->SetVirtualSize(size);
    DlgInventoryNpc->SetAlignment(oCViewDialogStealContainer::oEStealContainerAlignment_Left);
    DlgInventoryPlayer->Init(this, 0, 0, 0, 0.0f, 0.0f, texture);
    position.X = 0x1c00;
    position.Y = 0;
    DlgInventoryPlayer->SetVirtualPosition(position);
    DlgInventoryPlayer->SetVirtualSize(size);
    DlgInventoryPlayer->SetAlignment(oCViewDialogInventory::oEInventoryAlignment_Right);
    DlgInventoryNpc->ValueMultiplier = oCItemContainer::GetValueMultiplier();
    UpdateViewSettings();
}

zCClassDef* oCViewDialogTrade::_GetClassDef() const
{
    return classDef;
}

short oCViewDialogTrade::GetTransferCount()
{
    return TransferCount;
}

void oCViewDialogTrade::SetTransferCount(short count)
{
    TransferCount = count;
}

void oCViewDialogTrade::IncTransferCount(short count)
{
    TransferCount += count;
}

oCViewDialogTrade::~oCViewDialogTrade()
{
    delete DlgInventoryNpc;
    DlgInventoryNpc = 0;
    delete DlgInventoryPlayer;
    DlgInventoryPlayer = 0;
    SetNpcLeft(0);
    NpcRight = 0;
}

void oCViewDialogTrade::AllDialogsStop()
{
    DlgInventoryNpc->StopSelection();
    DlgInventoryPlayer->StopSelection();
}

void oCViewDialogTrade::AllDialogsDisable()
{
    DlgInventoryNpc->Activate(0);
    DlgInventoryPlayer->Activate(0);
}

void oCViewDialogTrade::SetNpcLeft(oCNpc* npc)
{
    NpcLeft = npc;
    if (!NpcLeft)
        return;
    delete NpcLeft->trader;
    NpcLeft->trader = new oCItemContainer;
    delete oCNpc::stealcontainer;
    oCNpc::stealcontainer = new oCStealContainer;
    oCNpc::stealcontainer->SetOwner(NpcLeft);
    DlgInventoryNpc->SetStealContainer(oCNpc::stealcontainer);
}

void oCViewDialogTrade::SetNpcRight(oCNpc* npc)
{
    NpcRight = npc;
    if (!NpcRight)
        return;
    delete NpcRight->trader;
    NpcRight->trader = new oCItemContainer;
    DlgInventoryPlayer->SetInventory(&NpcRight->inventory2);
}

void oCViewDialogTrade::StartSelection()
{
    Activate(1);
    IsDone = 0;
    DlgInventoryNpc->StartSelection();
    DlgInventoryPlayer->StartSelection();
    SetHandleEventTop();
    SectionTrade = TRADE_SECTION_LEFT_INVENTORY;
    DlgInventoryNpc->Activate(0);
    DlgInventoryPlayer->Activate(0);
    OnSection(SectionTrade);
    SetHandleEventTop();
}

int oCViewDialogTrade::HandleEvent(int key)
{
    if (!IsActive())
        return 0;
    if (DlgInventoryNpc->HasFinished() && SectionTrade != TRADE_SECTION_RIGHT_INVENTORY) {
        OnSection(TRADE_SECTION_RIGHT_INVENTORY);
        SetHandleEventTop();
    }
    if (DlgInventoryPlayer->HasFinished() && SectionTrade != TRADE_SECTION_LEFT_INVENTORY) {
        OnSection(TRADE_SECTION_LEFT_INVENTORY);
        SetHandleEventTop();
    }
    if (zinput->IsBindedToggled(5, key) || (key == 0x20c && zinput->KeyToggled(key)))
        SetTransferCount(0);
    short amount = oCItemContainer::TransferCountToAmount(GetTransferCount());
    if (zinput->IsBinded(0xb, key)) {
        SetTransferCount(0);
        int selectedCount = SectionTrade == TRADE_SECTION_LEFT_INVENTORY
            ? DlgInventoryNpc->GetSelectedItemCount() : DlgInventoryPlayer->GetSelectedItemCount();
        if (amount > selectedCount)
            selectedCount = amount;
        amount = selectedCount;
    }
    int result = 0;
    if (zinput->IsBindedToggled(0xe, key) || key == 0x20d) {
        OnExit();
        result = 1;
    } else if (zinput->IsBinded(5, key) || zinput->IsBinded(0xb, key) || key == 0x20c) {
        if (SectionTrade == TRADE_SECTION_LEFT_INVENTORY)
            result = OnTransferRight(amount);
        else if (SectionTrade == TRADE_SECTION_RIGHT_INVENTORY)
            result = OnTransferLeft(amount);
    } else if (zinput->IsBindedToggled(0xf, key)) {
        if (oCItemContainer::GetInvSplitScreen())
            OnExit();
        else
            OnSection(SectionTrade == TRADE_SECTION_LEFT_INVENTORY
                ? TRADE_SECTION_RIGHT_INVENTORY : TRADE_SECTION_LEFT_INVENTORY);
        result = 1;
    }
    SetHandleEventTop();
    NpcRight->SetMovLock(1);
    return result;
}

void oCViewDialogTrade::Update()
{
    SetHandleEventTop();
    NpcRight->SetMovLock(1);
}

int oCViewDialogTrade::OnKeyEnter()
{
    return 1;
}

int oCViewDialogTrade::OnKeyEsc()
{
    OnExit();
    return 1;
}

int oCViewDialogTrade::OnMoveLeft()
{
    if (SectionTrade == TRADE_SECTION_LEFT_INVENTORY)
        DlgInventoryNpc->CanHandleLeft();
    else
        DlgInventoryPlayer->CanHandleLeft();
    OnSection(SectionTrade == TRADE_SECTION_LEFT_INVENTORY
        ? TRADE_SECTION_RIGHT_INVENTORY : TRADE_SECTION_LEFT_INVENTORY);
    return 1;
}

int oCViewDialogTrade::OnTransferLeft(short count)
{
    if (SectionTrade != TRADE_SECTION_RIGHT_INVENTORY)
        return 1;
    SetTransferCount(count);
    oCItem* selectedItem = DlgInventoryPlayer->GetSelectedItem();
    int isCurrency = 0;
    if (selectedItem) {
        if (selectedItem->amount <= count) {
            count = selectedItem->amount;
            SetTransferCount(0);
        }
        isCurrency = selectedItem->GetInstanceName() == oCItemContainer::GetCurrencyInstanceName();
    }
    if (!isCurrency) {
        for (int i = 0; i < count; ++i) {
            oCItem* item = DlgInventoryPlayer->RemoveSelectedItem();
            if (!item)
                continue;
            int value = item->GetValue();
            if (value <= 0) {
                DlgInventoryNpc->InsertItem(item);
                continue;
            }
            int currencyAmount = static_cast<int>(DlgInventoryNpc->ValueMultiplier * value);
            if (currencyAmount <= 0)
                currencyAmount = 1;
            oCItem* currency = oCItemContainer::CreateCurrencyItem(currencyAmount);
            if (!currency) {
                DlgInventoryPlayer->InsertItem(item);
                continue;
            }
            DlgInventoryNpc->InsertItem(item);
            DlgInventoryPlayer->InsertItem(currency);
            currency->Release();
        }
    }
    return 1;
}

int oCViewDialogTrade::OnMoveRight()
{
    if (SectionTrade == TRADE_SECTION_LEFT_INVENTORY)
        DlgInventoryNpc->CanHandleRight();
    else
        DlgInventoryPlayer->CanHandleRight();
    OnSection(SectionTrade == TRADE_SECTION_LEFT_INVENTORY
        ? TRADE_SECTION_RIGHT_INVENTORY : TRADE_SECTION_LEFT_INVENTORY);
    return 1;
}

int oCViewDialogTrade::OnTransferRight(short count)
{
    if (SectionTrade != TRADE_SECTION_LEFT_INVENTORY)
        return 1;
    SetTransferCount(count);
    oCItem* selectedItem = DlgInventoryNpc->GetSelectedItem();
    if (selectedItem && selectedItem->amount <= count) {
        count = selectedItem->amount;
        SetTransferCount(0);
    }
    for (int i = 0; i < count; ++i) {
        oCItem* item = DlgInventoryNpc->RemoveSelectedItem();
        if (!item)
            continue;
        int value = item->GetValue();
        if (value <= 0) {
            DlgInventoryPlayer->InsertItem(item);
            continue;
        }
        oCNpcInventory* inventory = DlgInventoryPlayer->Inventory;
        oCItem* currency = 0;
        if (inventory && inventory->IsIn(oCItemContainer::GetCurrencyInstanceName(), value))
            currency = inventory->Remove(oCItemContainer::GetCurrencyInstanceName(), value);
        if (currency) {
            oCItemContainer::RemoveCurrencyItem(currency);
            DlgInventoryPlayer->InsertItem(item);
            continue;
        }
        DlgInventoryNpc->InsertItem(item);
        zSTRING messageName = "PLAYER_TRADE_NOT_ENOUGH_GOLD";
        int messageIndex = parser.GetIndex(messageName);
        if (messageIndex == -1) {
            zSTRING message = "Nicht genug Gold um den Gegenstand zu kaufen.";
            ogame->GetTextView()->Printwin(message);
        } else {
            oCMsgManipulate* message = new oCMsgManipulate(0xd, messageIndex);
            oCNpc::player->GetEM(0)->OnMessage(message, oCNpc::player);
        }
    }
    return 1;
}

int oCViewDialogTrade::OnSectionPrevious()
{
    OnSection(SectionTrade == TRADE_SECTION_LEFT_INVENTORY
        ? TRADE_SECTION_RIGHT_INVENTORY : TRADE_SECTION_LEFT_INVENTORY);
    return 1;
}

int oCViewDialogTrade::OnSectionNext()
{
    OnSection(SectionTrade == TRADE_SECTION_LEFT_INVENTORY
        ? TRADE_SECTION_RIGHT_INVENTORY : TRADE_SECTION_LEFT_INVENTORY);
    return 1;
}

void oCViewDialogTrade::OnSection(zETradeDialogSection section)
{
    SectionTrade = section;
    DlgInventoryNpc->Activate(section == TRADE_SECTION_LEFT_INVENTORY);
    DlgInventoryPlayer->Activate(section == TRADE_SECTION_RIGHT_INVENTORY);
    SetHandleEventTop();
}

void oCViewDialogTrade::OnExit()
{
    AllDialogsStop();
    AllDialogsDisable();
    if (NpcRight)
        NpcRight->SetMovLock(1);
    if (NpcLeft) {
        delete NpcLeft->trader;
        NpcLeft->trader = 0;
    }
    if (NpcRight) {
        delete NpcRight->trader;
        NpcRight->trader = 0;
    }
    IsDone = 1;
}
