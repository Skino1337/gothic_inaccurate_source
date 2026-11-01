#include "include/oViewDialogStealContainer.h"

static zCClassDef oCViewDialogStealContainerClassDef("oCViewDialogStealContainer", "zCObject",
    oCViewDialogStealContainer::_CreateNewInstance, 0, 0x110, 0, "");
zCClassDef* oCViewDialogStealContainer::classDef = &oCViewDialogStealContainerClassDef;

zCObject* oCViewDialogStealContainer::_CreateNewInstance()
{
    oCViewDialogStealContainer* dialog = new oCViewDialogStealContainer;
    zCClassDef::ObjectCreated(dialog, classDef);
    return dialog;
}

void oCViewDialogStealContainer::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* oCViewDialogStealContainer::_GetClassDef() const
{
    return classDef;
}

oCViewDialogStealContainer::oCViewDialogStealContainer()
    : oTStealContainerAlignment(oEStealContainerAlignment_Left),
      oTAlignmentStealContainer(oEStealContainerAlignment_Left), StealContainer(0),
      Alignment(oEStealContainerAlignment_Right), Value(0), ValueMultiplier(1.0f)
{
}

oCViewDialogStealContainer::~oCViewDialogStealContainer()
{
}

void oCViewDialogStealContainer::Activate(int active)
{
    IsActivated = active;
    if (!StealContainer)
        return;
    if (active)
        StealContainer->Activate();
    else
        StealContainer->Deactivate();
}

void oCViewDialogStealContainer::SetStealContainer(oCStealContainer* container)
{
    StealContainer = container;
    if (StealContainer) {
        StealContainer->DisableManipulateItems(1);
        StealContainer->DisableTransferMoreThanOneItem(0);
        UpdateValue();
    }
}

void oCViewDialogStealContainer::SetAlignment(oEStealContainerAlignment alignment)
{
    Alignment = alignment;
}

oCItem* oCViewDialogStealContainer::GetSelectedItem()
{
    return StealContainer ? StealContainer->GetSelectedItem() : 0;
}

int oCViewDialogStealContainer::GetSelectedItemCount()
{
    return StealContainer ? StealContainer->GetSelectedItemCount() : 0;
}

oCItem* oCViewDialogStealContainer::RemoveSelectedItem()
{
    if (!StealContainer)
        return 0;
    oCNpc* owner = StealContainer->GetOwner();
    if (!owner)
        return 0;
    oCItem* selectedItem = StealContainer->GetSelectedItem();
    int instance = selectedItem ? selectedItem->GetInstance() : -1;
    oCItem* result = owner->inventory2.RemoveByPtr(StealContainer->GetSelectedItem(), 1);
    StealContainer->SetOwner(owner);
    if (StealContainer) {
        oCItem* nextItem = StealContainer->GetSelectedItem();
        if (nextItem && nextItem->GetInstance() != instance)
            zinput->ResetRepeatKey(0);
    }
    if (result)
        UpdateValue();
    return result;
}

void oCViewDialogStealContainer::InsertItem(oCItem* item)
{
    if (!StealContainer || !item)
        return;
    oCNpc* owner = StealContainer->GetOwner();
    if (!owner)
        return;
    owner->inventory2.Insert(item);
    StealContainer->SetOwner(owner);
    UpdateValue();
}

void oCViewDialogStealContainer::RemoveItem(oCItem* item)
{
    if (!StealContainer || !item)
        return;
    oCNpc* owner = StealContainer->GetOwner();
    if (!owner)
        return;
    owner->inventory2.RemoveByPtr(item, 1);
    StealContainer->SetOwner(owner);
    UpdateValue();
}

void oCViewDialogStealContainer::TransferAllItemsTo(oCNpcInventory* inventory)
{
    if (!StealContainer || !inventory)
        return;
    zCListSort<oCItem>* contents = StealContainer->GetContents();
    if (!contents)
        return;
    zCArray<oCItem*> items;
    for (zCListSort<oCItem>* node = contents->next; node; node = node->next)
        items.InsertEnd(node->data);
    for (int i = items.numInArray - 1; i >= 0; --i) {
        if (items.parray[i])
            inventory->Insert(items.parray[i]);
    }
    oCNpc* owner = StealContainer->GetOwner();
    if (!owner)
        return;
    for (int i = items.numInArray - 1; i >= 0; --i) {
        if (items.parray[i])
            owner->inventory2.RemoveByPtr(items.parray[i], 1);
    }
    StealContainer->SetOwner(owner);
    UpdateValue();
}

void oCViewDialogStealContainer::UpdateValue()
{
    oCNpc* owner = StealContainer->GetOwner();
    if (owner) {
        zSTRING name = owner->GetName();
        StealContainer->SetName(name);
    }
}

int oCViewDialogStealContainer::CanHandleLeft()
{
    return 0;
}

int oCViewDialogStealContainer::CanHandleRight()
{
    return 0;
}

int oCViewDialogStealContainer::HandleEvent(int key)
{
    if (!IsActive())
        return 0;
    if (key != 0x1c && key != 0x20d)
        return 0;
    CloseSafe();
    return 1;
}

void oCViewDialogStealContainer::StartSelection()
{
    IsDone = 0;
    SetHandleEventTop();
    SetEnableHandleEvent(1);
    if (!StealContainer)
        return;
    if (Alignment == oEStealContainerAlignment_Right)
        StealContainer->Open(0x1000, 0, 5);
    else
        StealContainer->Open(0, 0, 4);
}

void oCViewDialogStealContainer::StopSelection()
{
    IsDone = 0;
    SetEnableHandleEvent(0);
    if (StealContainer)
        StealContainer->Close();
}
