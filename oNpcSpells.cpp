#include "include/oCollisionObjectMiscChar.h"
#include "include/oNpcMessages.h"

extern zTIMER ztimer;

int oCNpc::EV_CastSpell(oCMsgMagic* message)
{
    if (!anictrl || !mag_book)
        return 1;
    oCSpell* spell = mag_book->GetSelectedSpell();
    if (!spell)
        return 1;
    zCVob* target = message ? enemy : enemy;
    SetFocusVob(target);
    if (spell->manaInvested > spellMana)
        spellMana = spell->manaInvested;
    int status = spell->GetSpellStatus();
    if (status == 2 || status == 3 || status == 0)
        return 1;
    if (GetTrueGuild() != 0x21 && (anictrl->IsRunning() || anictrl->IsWalking()))
        return 1;
    if (message && anictrl->IsStanding())
        return 1;
    return 0;
}

void oCNpc::CheckRunningFightAnis()
{
    if (fight_waitForAniEnd == -1)
        return;
    if (enemy) {
        zVEC3 position = {
            enemy->trafoObjToWorld.m[0][3],
            enemy->trafoObjToWorld.m[1][3],
            enemy->trafoObjToWorld.m[2][3]
        };
        Turn(position);
    }
    fight_waitForAniEnd = -1;
}

void oCNpc::GotoFightPosition()
{
    rbt.timer = 500.0f;
    rbt.targetVob = enemy;
    if (enemy) {
        rbt.targetPos.n[0] = enemy->trafoObjToWorld.m[0][3];
        rbt.targetPos.n[1] = enemy->trafoObjToWorld.m[1][3];
        rbt.targetPos.n[2] = enemy->trafoObjToWorld.m[2][3];
    }
    if (GetWeaponMode() >= 5) {
        rbt.flags = (rbt.flags & ~4) | 0x10;
        rbt.maxTargetDist = 1000000.0f;
        RobustTrace();
        return;
    }
    int range = GetFightRange();
    rbt.maxTargetDist = range * range;
    RbtGotoFollowPosition();
}

void oCNpc::Fighting()
{
    if (ztimer.frameTimeFloat <= 0.0f)
        return;
    anictrl = GetAnictrl();
    if (anictrl && anictrl->GetWaterLevel() < 2) {
        int mode = GetWeaponMode();
        if (!mode) {
            weaponMode = anictrl->wmode_last;
            return;
        }
        if ((mode == 5 || mode == 6) && !IsMunitionAvailable(GetWeapon())) {
            weaponMode = 0;
            return;
        }
    }
    rbt.flags |= 1;
    ThinkNextFightAction();
}

oCItem* oCNpc::GetSpellItem(int spellID)
{
    int count = inventory2.GetNumItemsInCategory(3);
    for (int i = 0; i < count; ++i) {
        oCItem* item = inventory2.GetItem(i, 3);
        if (item && item->spellID == spellID) {
            ++item->refCtr;
            return item;
        }
    }
    return 0;
}

int oCNpc::ReadySpell(int spellID, int mana)
{
    if (!mag_book) {
        spellMana = 0;
        return 0;
    }
    oCItem* item = GetSpellItem(spellID);
    if (!item)
        return 0;
    if (!item->HasFlag(0x40000000)) {
        EquipItem(item);
        mag_book->Register(item, 1);
    }
    int selectedSpell = spellID;
    oCSpell* spell = mag_book->GetSpell(item, selectedSpell);
    if (!spell)
        return 0;
    if (item->refCtr > 0)
        --item->refCtr;
    mag_book->SetFrontSpell(selectedSpell);
    spellMana = mana;
    weaponMode = 7;
    return 1;
}

int oCNpc::UnreadySpell()
{
    StandUp(0, 1);
    weaponMode = 0;
    return 1;
}

int oCNpc::HasRangedWeaponAndAmmo()
{
    int count = inventory2.GetNumItemsInCategory(4);
    inventory2.UnpackItemsInCategory(4);
    for (int i = 0; i < count; ++i) {
        oCItem* item = inventory2.GetItem(i, 4);
        if (item && CanUse(item) && item->HasFlag(4) && IsMunitionAvailable(item))
            return 1;
    }
    oCItem* item = GetWeapon();
    return item && CanUse(item) && item->HasFlag(4) && IsMunitionAvailable(item);
}

int oCNpc::CanDrawWeapon()
{
    oCAniCtrl_Human* control = GetAnictrl();
    if (!control)
        return 0;
    if (control->IsStanding() || control->IsWalking() || GetInteractMob())
        return 1;
    return GetWeaponMode() == 5 || GetWeaponMode() == 6;
}
