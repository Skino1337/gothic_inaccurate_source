#ifndef OVIEWDIALOGTRADE_H
#define OVIEWDIALOGTRADE_H

#include "oViewDialogStealContainer.h"

class oCViewDialogTrade : public zCViewDialog {
public:

    enum zETradeDialogSection {
        TRADE_SECTION_LEFT_INVENTORY,
        TRADE_SECTION_RIGHT_INVENTORY
    };
    typedef zETradeDialogSection zTTradeSection;

    oCViewDialogTrade();
    virtual ~oCViewDialogTrade();
    static zCObject* _CreateNewInstance();
    void UpdateViewSettings();
    virtual zCClassDef* _GetClassDef() const;
    virtual short GetTransferCount();
    virtual void SetTransferCount(short count);
    virtual void IncTransferCount(short count);
    void AllDialogsStop();
    void AllDialogsDisable();
    void SetNpcLeft(oCNpc* npc);
    void SetNpcRight(oCNpc* npc);
    virtual void StartSelection();
    virtual int HandleEvent(int key);
    void Update();
    int OnKeyEnter();
    int OnKeyEsc();
    int OnMoveLeft();
    int OnTransferLeft(short count);
    int OnMoveRight();
    int OnTransferRight(short count);
    int OnSectionPrevious();
    int OnSectionNext();
    void OnSection(zETradeDialogSection section);
    void OnExit();

    oCViewDialogStealContainer* DlgInventoryNpc;
    oCViewDialogInventory* DlgInventoryPlayer;
    zTTradeSection SectionTrade;
    oCNpc* NpcLeft;
    oCNpc* NpcRight;
    short TransferCount;

    static zCClassDef* classDef;
};

#endif
