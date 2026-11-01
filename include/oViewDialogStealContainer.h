#ifndef OVIEWDIALOGSTEALCONTAINER_H
#define OVIEWDIALOGSTEALCONTAINER_H

#include "oViewDialogItemContainer.h"

class oCStealContainer : public oCItemContainer {
public:

    oCNpc* GetOwner();
    void   SetOwner(oCNpc* owner);
};

class oCViewDialogStealContainer : public zCViewDialog {
public:

    enum oEStealContainerAlignment {
        oEStealContainerAlignment_Left,
        oEStealContainerAlignment_Right
    };

    oCViewDialogStealContainer();
    virtual ~oCViewDialogStealContainer();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Activate(int active);
    void SetStealContainer(oCStealContainer* container);
    void SetAlignment(oEStealContainerAlignment alignment);
    oCItem* GetSelectedItem();
    int GetSelectedItemCount();
    oCItem* RemoveSelectedItem();
    void InsertItem(oCItem* item);
    void RemoveItem(oCItem* item);
    void TransferAllItemsTo(oCNpcInventory* inventory);
    void UpdateValue();
    int CanHandleLeft();
    int CanHandleRight();
    virtual int HandleEvent(int key);
    virtual void StartSelection();
    virtual void StopSelection();

    oEStealContainerAlignment oTStealContainerAlignment;
    oEStealContainerAlignment oTAlignmentStealContainer;
    oCStealContainer* StealContainer;
    oEStealContainerAlignment Alignment;
    unsigned long Value;
    float ValueMultiplier;
    zSTRING ownerName;

    static zCClassDef* classDef;
};

#endif
