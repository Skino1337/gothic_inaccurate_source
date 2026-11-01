#ifndef OVIEWDIALOGINVENTORY_H
#define OVIEWDIALOGINVENTORY_H

#include "oCollisionObjectMiscChar.h"
#include "zSession.h"
#include "zViewPrint.h"

class zCViewDialog : public zCViewPrint, public zCInputCallback {
public:

    zCViewDialog();
    virtual ~zCViewDialog();
    static zCObject* _CreateNewInstance();
    virtual int IsActive();
    virtual void Activate(int active);
    virtual int HasFinished();
    virtual void StartSelection();
    virtual void StopSelection();
    virtual int HandleEvent(int key);
    virtual zCClassDef* _GetClassDef() const;
    void CloseSafe();

    static zCClassDef* classDef;

    int IsDone;
    int IsActivated;
};

class oCViewDialogInventory : public zCViewDialog {
public:

    enum oEInventoryAlignment {
        oEInventoryAlignment_Left,
        oEInventoryAlignment_Right
    };

    oCViewDialogInventory();
    virtual ~oCViewDialogInventory();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Activate(int active);
    void SetInventory(oCNpcInventory* inventory);
    void SetAlignment(oEInventoryAlignment alignment);
    oCItem* GetSelectedItem();
    int GetSelectedItemCount();
    oCItem* RemoveSelectedItem();
    void InsertItem(oCItem* item);
    int CanHandleLeft();
    int CanHandleRight();
    virtual int HandleEvent(int key);
    virtual void StartSelection();
    virtual void StopSelection();

    oEInventoryAlignment oTInventoryAlignment;
    oEInventoryAlignment oTAlignmentInventory;
    oCNpcInventory* Inventory;
    oEInventoryAlignment Alignment;

    static zCClassDef* classDef;
};

#endif
