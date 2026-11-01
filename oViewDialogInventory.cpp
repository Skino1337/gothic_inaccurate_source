#include "include/oViewDialogInventory.h"

static zCClassDef oCViewDialogInventoryClassDef("oCViewDialogInventory", "zCObject", NULL, 0, 0x108, 0, "");
zCClassDef* oCViewDialogInventory::classDef = &oCViewDialogInventoryClassDef;
static int viewDialogState = -1;
static int viewDialogRuntimeState = -1;
static zCClassDef zCViewDialogClassDef("zCViewDialog", "zCObject", zCViewDialog::_CreateNewInstance,
    0, 0xF8, 0, "");
zCClassDef* zCViewDialog::classDef = &zCViewDialogClassDef;

zCViewDialog::zCViewDialog()
    : IsDone(0), IsActivated(1)
{
    ModeOpen = 0;
    ModeClose = 0;
    DurationOpen = 500.0f;
    DurationClose = 500.0f;
}

zCObject* zCViewDialog::_CreateNewInstance()
{
    zCViewDialog* dialog = new zCViewDialog;
    zCClassDef::ObjectCreated(dialog, classDef);
    return dialog;
}

zCViewDialog::~zCViewDialog()
{
}

zCClassDef* zCViewDialog::_GetClassDef() const
{
    return classDef;
}

int zCViewDialog::IsActive()
{
    return IsActivated;
}

void zCViewDialog::Activate(int active)
{
    IsActivated = active;
}

int zCViewDialog::HasFinished()
{
    return IsDone;
}

void zCViewDialog::StartSelection()
{
    IsDone = 0;
}

void zCViewDialog::StopSelection()
{
    IsDone = 1;
}

int zCViewDialog::HandleEvent(int key)
{
    return 0;
}

void zCViewDialog::CloseSafe()
{
    IsDone = 1;
}

zCObject* oCViewDialogInventory::_CreateNewInstance()
{
    oCViewDialogInventory* dialog = new oCViewDialogInventory;
    zCClassDef::ObjectCreated(dialog, classDef);
    return dialog;
}

void oCViewDialogInventory::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* oCViewDialogInventory::_GetClassDef() const
{
    return classDef;
}

oCViewDialogInventory::oCViewDialogInventory()
    : oTInventoryAlignment(oEInventoryAlignment_Left),
      oTAlignmentInventory(oEInventoryAlignment_Left), Inventory(0),
      Alignment(oEInventoryAlignment_Right)
{
}

oCViewDialogInventory::~oCViewDialogInventory()
{
}

void oCViewDialogInventory::Activate(int active)
{
    IsActivated = active;
    if (!Inventory)
        return;
    if (active)
        Inventory->Activate();
    else
        Inventory->Deactivate();
}

void oCViewDialogInventory::SetInventory(oCNpcInventory* inventory)
{
    Inventory = inventory;
    if (Inventory) {
        Inventory->DisableManipulateItems(1);
        Inventory->DisableTransferMoreThanOneItem(0);
    }
}

void oCViewDialogInventory::SetAlignment(oEInventoryAlignment alignment)
{
    Alignment = alignment;
}

oCItem* oCViewDialogInventory::GetSelectedItem()
{
    return Inventory ? Inventory->GetSelectedItem() : 0;
}

int oCViewDialogInventory::GetSelectedItemCount()
{
    return Inventory ? Inventory->GetSelectedItemCount() : 0;
}

oCItem* oCViewDialogInventory::RemoveSelectedItem()
{
    if (!Inventory || !Inventory->GetOwner())
        return 0;
    oCItem* selectedItem = Inventory->GetSelectedItem();
    int instance = selectedItem ? selectedItem->GetInstance() : -1;
    oCItem* result = Inventory->RemoveByPtr(selectedItem, 1);
    if (Inventory) {
        oCItem* nextItem = Inventory->GetSelectedItem();
        if (nextItem && nextItem->GetInstance() != instance)
            zinput->ResetRepeatKey(0);
    }
    return result;
}

void oCViewDialogInventory::InsertItem(oCItem* item)
{
    if (Inventory)
        Inventory->Insert(item);
}

int oCViewDialogInventory::CanHandleLeft()
{
    return 0;
}

int oCViewDialogInventory::CanHandleRight()
{
    return 0;
}

int oCViewDialogInventory::HandleEvent(int key)
{
    if (!IsActive())
        return 0;
    if (key != 0x1c && key != 0x20d)
        return 0;
    CloseSafe();
    return 1;
}

void oCViewDialogInventory::StartSelection()
{
    IsDone = 0;
    SetEnableHandleEvent(1);
    if (Inventory) {
        if (Alignment == oEInventoryAlignment_Right)
            Inventory->Open(0x1000, 0, 5);
        else
            Inventory->Open(0, 0, 4);
    }
    SetHandleEventTop();
}

void oCViewDialogInventory::StopSelection()
{
    IsDone = 0;
    SetEnableHandleEvent(0);
    if (Inventory)
        Inventory->Close();
}
