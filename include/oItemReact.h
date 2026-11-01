#ifndef OITEMREACT_H
#define OITEMREACT_H

#include "oCollisionObjectMiscChar.h"

class oCItemReactModule {
public:

    oCItemReactModule(int instance);
    ~oCItemReactModule();
    void Validate();
    int Conditions(int instance, int category) const;
    void GetOfferedInstance(int& instance, int& amount) const;
    int StartReaction(int amount);
    int GetDataSize() const;
    int GetNpcInstance() const;

    oCItemReactModule* next;
    struct {
        int trade_npc;
        int trade_instance;
        int trade_amount;
        int other_category;
        int other_instance;
        int other_amount;
        int reaction;
    } pd;
};

class oCTradeManager {
public:

    oCTradeManager(zCParser* scriptParser);
    ~oCTradeManager();
    oCItemReactModule* GetModule(oCNpc* self, oCNpc* other, oCItem* item);

    zCParser* p;
    struct {
        oCItemReactModule* last;
        oCItemReactModule* root;
        int count;
    } moduleList;

    static int size_checked;
};

#endif
