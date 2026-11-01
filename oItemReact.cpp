#include "include/oItemReact.h"

int oCTradeManager::size_checked;
static int itemReactState = -1;
static zSTRING itemReactPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING itemReactPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING itemReactPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING itemReactPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING itemReactPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING itemReactPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING itemReactPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING itemReactPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING itemReactPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING itemReactPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static zSTRING tradeClass("C_ITEMREACT");

oCItemReactModule::oCItemReactModule(int instance)
    : next(0)
{
    pd.trade_npc = -1;
    pd.trade_instance = -1;
    pd.trade_amount = 0;
    pd.other_category = 0;
    pd.other_instance = -1;
    pd.other_amount = 0;
    pd.reaction = -1;
    parser.CreateInstance(instance, &pd);
}

oCItemReactModule::~oCItemReactModule()
{
    next = 0;
}

void oCItemReactModule::Validate()
{
}

int oCItemReactModule::Conditions(int instance, int category) const
{
    if (pd.other_instance < 0)
        return (category & pd.other_category) > 0;
    return instance == pd.other_instance;
}

void oCItemReactModule::GetOfferedInstance(int& instance, int& amount) const
{
    instance = pd.trade_instance;
    amount = pd.trade_amount;
}

int oCItemReactModule::StartReaction(int amount)
{
    pd.other_amount = amount;
    return parser.CallFunc(pd.reaction);
}

int oCItemReactModule::GetDataSize() const
{
    return 0x1c;
}

int oCItemReactModule::GetNpcInstance() const
{
    return pd.trade_npc;
}

oCTradeManager::oCTradeManager(zCParser* scriptParser)
    : p(scriptParser)
{
    moduleList.last = 0;
    moduleList.root = 0;
    moduleList.count = 0;
    if (!p)
        return;
    int classIndex = p->GetIndex(tradeClass);
    if (classIndex < 0)
        return;
    int instanceIndex = classIndex;
    while (true) {
        int instance = p->GetInstance(classIndex, ++instanceIndex);
        if (instance < 0)
            break;
        oCItemReactModule* module = new oCItemReactModule(instance);
        if (!moduleList.last)
            moduleList.last = module;
        module->next = moduleList.root;
        moduleList.root = module;
        ++moduleList.count;
    }
    zerr->Message(zSTRING("U:TRADE: ") + zSTRING((long)moduleList.count) +
        " ItemReactModules found.");
}

oCTradeManager::~oCTradeManager()
{
    oCItemReactModule* module = moduleList.root;
    while (module) {
        oCItemReactModule* next = module->next;
        delete module;
        module = next;
    }
    moduleList.last = 0;
    moduleList.root = 0;
    moduleList.count = 0;
    p = 0;
}

oCItemReactModule* oCTradeManager::GetModule(oCNpc* self, oCNpc* other, oCItem* item)
{
    p->SetInstance("SELF", self);
    p->SetInstance("OTHER", other);
    p->SetInstance("ITEM", item);
    for (oCItemReactModule* module = moduleList.root; module; module = module->next) {
        if (module->pd.trade_npc == self->GetInstance() &&
            module->Conditions(item->GetInstance(), item->mainflag))
            return module;
    }
    return 0;
}
