#ifndef OVIEWDIALOGITEMCONTAINER_H
#define OVIEWDIALOGITEMCONTAINER_H

#include "oViewDialogItem.h"
#include "oDocumentManager.h"

class oCItemContainer {
public:

    void Activate();
    void Deactivate();
    void DisableManipulateItems(int disabled);
    void DisableTransferMoreThanOneItem(int disabled);
    oCItem* GetSelectedItem();
    int GetSelectedItemCount();
    oCItem* RemoveByPtr(oCItem* item, int amount);
    void Remove2(oCItem* item, int amount);
    oCItem* Insert(oCItem* item);
    zCListSort<oCItem>* GetContents();
    void Open(int x, int y, int mode);
    void Close();
    void SetName(zSTRING& name);
    static float GetValueMultiplier();
    static int GetInvSplitScreen();
    static short TransferCountToAmount(short count);
    static zSTRING GetCurrencyInstanceName();
    static oCItem* CreateCurrencyItem(short amount);
    static void RemoveCurrencyItem(oCItem* item);
};

class oCViewDialogItemContainer : public zCViewDialog {
public:

    enum oEItemContainerAlignment {
        oEItemContainerAlignment_Left,
        oEItemContainerAlignment_Right
    };

    oCViewDialogItemContainer();
    virtual ~oCViewDialogItemContainer();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Activate(int active);
    void SetItemContainer(oCItemContainer* container);
    void SetAlignment(oEItemContainerAlignment alignment);
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

    oEItemContainerAlignment oTItemContainerAlignment;
    oEItemContainerAlignment oTAlignmentItemContainer;
    oCItemContainer* ItemContainer;
    oEItemContainerAlignment Alignment;
    unsigned long Value;
    float ValueMultiplier;

    static zCClassDef* classDef;
};

#endif
