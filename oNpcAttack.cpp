#include "include/oCollisionObjectMiscChar.h"
#include "include/oNpcMessages.h"

static void SendAttack(oCNpc* attacker, zCVob* target, int subType)
{
    oCMsgAttack* message = new oCMsgAttack;
    zCClassDef::ObjectCreated(message, oCMsgAttack::classDef);
    message->attackTarget = target;
    message->attackSubType = subType;
    target->GetEM(0)->OnMessage(message, attacker);
}

int oCNpc::FightAttackMelee(int action)
{
    if (action < 6 || action > 11)
        return 0;
    int count = action - 5;
    for (int i = 0; i < count; ++i)
        SendAttack(this, this, i % 4);
    return 1;
}

int oCNpc::FightAttackBow()
{
    if (!enemy || !IsMunitionAvailable(weapon))
        return 0;
    SendAttack(this, enemy, 6);
    return 1;
}

int oCNpc::EV_AttackBow(oCMsgAttack*)
{
    if (enemy)
        SendAttack(this, enemy, 7);
    return 1;
}

int oCNpc::FightAttackMagic()
{
    if (enemy && HasMagic() && GetAttribute(2) > 0) {
        oCMsgMagic* message = new oCMsgMagic;
        zCClassDef::ObjectCreated(message, oCMsgMagic::classDef);
        message->spellID = 8;
        enemy->GetEM(0)->OnMessage(message, this);
    }
    return 0;
}

int oCNpc::EV_AttackMagic(oCMsgAttack*)
{
    return 0;
}
