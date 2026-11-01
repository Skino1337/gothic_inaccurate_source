#include "include/oViewDialogItemContainer.h"

static int dialogState = -1;
static int dialogRuntimeState = -1;
static int dialogPlayerState = -1;
static zCClassDef oCViewDialogItemContainerClassDef("oCViewDialogItemContainer", "zCObject",
    oCViewDialogItemContainer::_CreateNewInstance, 0, 0x110, 0, "");
zCClassDef* oCViewDialogItemContainer::classDef = &oCViewDialogItemContainerClassDef;
static zSTRING dialogPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING dialogPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING dialogPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING dialogPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING dialogPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING dialogPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING dialogPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING dialogPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING dialogPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING dialogPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";

zCObject* oCViewDialogItemContainer::_CreateNewInstance()
{
    oCViewDialogItemContainer* dialog = new oCViewDialogItemContainer;
    zCClassDef::ObjectCreated(dialog, classDef);
    return dialog;
}

void oCViewDialogItemContainer::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* oCViewDialogItemContainer::_GetClassDef() const
{
    return classDef;
}

oCViewDialogItemContainer::oCViewDialogItemContainer()
    : oTItemContainerAlignment(oEItemContainerAlignment_Left),
      oTAlignmentItemContainer(oEItemContainerAlignment_Left), ItemContainer(0),
      Alignment(oEItemContainerAlignment_Right), Value(0), ValueMultiplier(1.0f)
{
}

oCViewDialogItemContainer::~oCViewDialogItemContainer()
{
}

void oCViewDialogItemContainer::Activate(int active)
{
    IsActivated = active;
    if (!ItemContainer)
        return;
    if (active)
        ItemContainer->Activate();
    else
        ItemContainer->Deactivate();
}

void oCViewDialogItemContainer::SetItemContainer(oCItemContainer* container)
{
    ItemContainer = container;
    if (ItemContainer) {
        ItemContainer->DisableManipulateItems(1);
        ItemContainer->DisableTransferMoreThanOneItem(0);
    }
}

void oCViewDialogItemContainer::SetAlignment(oEItemContainerAlignment alignment)
{
    Alignment = alignment;
}

oCItem* oCViewDialogItemContainer::GetSelectedItem()
{
    return ItemContainer ? ItemContainer->GetSelectedItem() : 0;
}

int oCViewDialogItemContainer::GetSelectedItemCount()
{
    return ItemContainer ? ItemContainer->GetSelectedItemCount() : 0;
}

oCItem* oCViewDialogItemContainer::RemoveSelectedItem()
{
    if (!ItemContainer)
        return 0;
    oCItem* selectedItem = ItemContainer->GetSelectedItem();
    int instance = selectedItem ? selectedItem->GetInstance() : -1;
    oCItem* result = ItemContainer->RemoveByPtr(selectedItem, 1);
    if (ItemContainer) {
        oCItem* nextItem = ItemContainer->GetSelectedItem();
        if (nextItem && nextItem->GetInstance() != instance)
            zinput->ResetRepeatKey(0);
    }
    return result;
}

void oCViewDialogItemContainer::InsertItem(oCItem* item)
{
    if (ItemContainer && item)
        ItemContainer->Insert(item);
}

void oCViewDialogItemContainer::RemoveItem(oCItem* item)
{
    if (ItemContainer && item)
        ItemContainer->Remove2(item, 1);
}

void oCViewDialogItemContainer::TransferAllItemsTo(oCNpcInventory* inventory)
{
    if (!ItemContainer || !inventory)
        return;
    zCListSort<oCItem>* contents = ItemContainer->GetContents();
    if (!contents)
        return;
    zCArray<oCItem*> items;
    for (zCListSort<oCItem>* node = contents->next; node; node = node->next)
        items.InsertEnd(node->data);
    for (int i = items.numInArray - 1; i >= 0; --i) {
        if (items.parray[i])
            inventory->Insert(items.parray[i]);
    }
    for (int i = items.numInArray - 1; i >= 0; --i) {
        if (items.parray[i])
            ItemContainer->Remove2(items.parray[i], 1);
    }
}

void oCViewDialogItemContainer::UpdateValue()
{
}

int oCViewDialogItemContainer::CanHandleLeft()
{
    return 0;
}

int oCViewDialogItemContainer::CanHandleRight()
{
    return 0;
}

int oCViewDialogItemContainer::HandleEvent(int key)
{
    if (!IsActive())
        return 0;
    if (key != 0x1c && key != 0x20d)
        return 0;
    CloseSafe();
    return 1;
}

void oCViewDialogItemContainer::StartSelection()
{
    IsDone = 0;
    SetHandleEventTop();
    SetEnableHandleEvent(1);
    if (!ItemContainer)
        return;
    if (Alignment == oEItemContainerAlignment_Left)
        ItemContainer->Open(0, 0, 4);
    else
        ItemContainer->Open(0x1000, 0, 5);
}

void oCViewDialogItemContainer::StopSelection()
{
    IsDone = 0;
    SetEnableHandleEvent(0);
    if (ItemContainer)
        ItemContainer->Close();
}
