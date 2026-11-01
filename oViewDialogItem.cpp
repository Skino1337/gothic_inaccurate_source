#include "include/oViewDialogItem.h"

static zCClassDef oCViewDialogItemClassDef("oCViewDialogItem", "zCObject",
    oCViewDialogItem::_CreateNewInstance, 0, 0xFC, 0, "");
zCClassDef* oCViewDialogItem::classDef = &oCViewDialogItemClassDef;

zCObject* oCViewDialogItem::_CreateNewInstance()
{
    oCViewDialogItem* dialog = new oCViewDialogItem;
    zCClassDef::ObjectCreated(dialog, classDef);
    return dialog;
}

void oCViewDialogItem::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* oCViewDialogItem::_GetClassDef() const
{
    return classDef;
}

oCViewDialogItem::oCViewDialogItem()
    : Item(0)
{
}

oCViewDialogItem::~oCViewDialogItem()
{
    SetItem(0);
}

void oCViewDialogItem::SetItem(oCItem* item)
{
    if (Item) {
        --Item->refCtr;
        if (Item->refCtr <= 0)
            delete Item;
    }
    Item = item;
    if (Item)
        ++Item->refCtr;
}

void oCViewDialogItem::Blit()
{
}
