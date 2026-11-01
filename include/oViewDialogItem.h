#ifndef OVIEWDIALOGITEM_H
#define OVIEWDIALOGITEM_H

#include "oViewDialogInventory.h"

class oCViewDialogItem : public zCViewDialog {
public:

    oCViewDialogItem();
    virtual ~oCViewDialogItem();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    void SetItem(oCItem* item);
    virtual void Blit();

    oCItem* Item;

    static zCClassDef* classDef;
};

#endif
