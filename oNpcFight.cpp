#include "include/oCollisionObjectMiscChar.h"
#include "include/zTools.h"
#include <cmath>
#include <cstdlib>

void oCNpc::oSActionBlock::InitBlock(zCParser* parser, int situation, int index)
{
    for (int i = 0; i < 6; ++i)
        actions[i] = 0;
    numActions = -1;
    parser->CreateInstance(zSTRING("FA_") + zSTRING(index) + zSTRING("_") + zSTRING(situation),
        this);
    for (int i = 0; i < 6; ++i) {
        if (!actions[i]) {
            numActions = i;
            break;
        }
    }
    if (numActions < 0)
        numActions = 6;
}

int oCNpc::oSActionBlock::GetOwnAction(int situation)
{
    if (numActions <= 0)
        return actions[0];
    return actions[std::rand() % numActions];
}

void oCNpc::oSFightAI::Init(zCParser* parser, int index)
{
    for (int i = 0; i < 19; ++i)
        myactions[i].InitBlock(parser, i, index);
}

int oCNpc::oSFightAI::GetOwnAction(int situation)
{
    return myactions[situation].GetOwnAction(situation);
}

static zCArray<oCNpc::oSFightAI*> fightAIList;

extern zCOption* zgameoptions;

void oCNpc::InitFightAI()
{
    zerr->Message("U:AI: Reading Script-fight-ai");
    zSTRING fileName;
    if (zgameoptions)
        fileName = zgameoptions->ReadString("FILES", "FightAI", "");
    if (fileName.IsEmpty())
        fileName = "Content\\Fight";
    zoptions->ChangeDir(8);
    zCParser* parser = new zCParser(100);
    parser->Parse(fileName + ".src");
    parser->CreatePCode();
    zCPar_Symbol* symbol = parser->GetSymbol("MAX_FIGHTAI");
    if (symbol) {
        int fightAICount;
        symbol->GetValue(fightAICount, 0);
        for (int i = 0; i < fightAICount; ++i) {
            oSFightAI* fightAI = new oSFightAI;
            fightAIList.InsertEnd(fightAI);
            fightAI->Init(parser, i);
        }
        delete parser;
        zerr->Message("U:AI: Reading Script-fight-ai finished");
    }
}

void oCNpc::ExitFightAI()
{
    for (int i = fightAIList.numInArray - 1; i >= 0; --i)
        delete fightAIList.parray[i];
    fightAIList.DeleteList();
}

oCItem* oCNpc::GetWeapon()
{
    return weapon;
}

int oCNpc::IsNpcBetweenMeAndTarget(zCVob* target)
{
    if (!target || !homeWorld)
        return 0;
    zVEC3 origin;
    origin.n[0] = trafoObjToWorld.m[0][3];
    origin.n[1] = trafoObjToWorld.m[1][3];
    origin.n[2] = trafoObjToWorld.m[2][3];
    zVEC3 direction;
    direction.n[0] = target->trafoObjToWorld.m[0][3] - origin.n[0];
    direction.n[1] = target->trafoObjToWorld.m[1][3] - origin.n[1];
    direction.n[2] = target->trafoObjToWorld.m[2][3] - origin.n[2];
    zCArray<zCVob*> ignoreVobs;
    ignoreVobs.InsertEnd(this);
    ignoreVobs.InsertEnd(target);
    if (!homeWorld->TraceRayNearestHit(origin, direction, &ignoreVobs, 5))
        return 0;
    zCVob* found = homeWorld->traceRayReport.foundVob;
    if (!found || !dynamic_cast<oCNpc*>(found))
        return 0;
    return bbox3D.GetSphere3D().IsIntersecting(found->bbox3D.GetSphere3D());
}

int oCNpc::IsInDoubleFightRange(zCVob* target, float& distance)
{
    if (!target)
        return 0;
    float x = trafoObjToWorld.m[0][3] - target->trafoObjToWorld.m[0][3];
    float y = trafoObjToWorld.m[1][3] - target->trafoObjToWorld.m[1][3];
    distance = std::sqrt(x * x + y * y);
    float range = fightRangeBase + GetFightRangeDynamic() * 2;
    return distance <= range;
}

int oCNpc::IsInFightRange(zCVob* target, float& distance)
{
    if (!target || !IsSameHeight(target))
        return 0;
    float x = trafoObjToWorld.m[0][3] - target->trafoObjToWorld.m[0][3];
    float y = trafoObjToWorld.m[1][3] - target->trafoObjToWorld.m[1][3];
    distance = std::sqrt(x * x + y * y);
    float range = GetFightRange();
    return distance <= range;
}

int oCNpc::GetFightRange()
{
    return fightRangeBase + GetFightRangeDynamic();
}

void oCNpc::SetFightRangeBase(int range)
{
    fightRangeBase = range;
}

int oCNpc::GetFightRangeBase()
{
    return fightRangeBase;
}

int oCNpc::GetFightRangeDynamic()
{
    return weapon ? weapon->range : fightRangeFist;
}

int oCNpc::GetFightRangeFist()
{
    return fightRangeFist;
}

int oCNpc::GetFightRangeG()
{
    return fightRangeG;
}

void oCNpc::SetFightRangeFist(int range)
{
    fightRangeFist = range;
}

void oCNpc::SetFightRangeG(int range)
{
    fightRangeG = range;
}

int oCNpc::GetFightActionFromTable(int situation)
{
    return fightAIList.parray[fighttactic]->myactions[situation].GetOwnAction(situation);
}

int oCNpc::GetCurrentFightMove()
{
    switch (currentFightAction) {
    case 1:
        return 1;
    case 2:
        return 2;
    case 3:
        return 3;
    case 4:
        return 4;
    case 5:
        return 5;
    case 6:
        return 0x0c;
    case 7:
        return 0x0e;
    default:
        return 0x13;
    }
}

zSTRING GetFightMoveStringFromConstant(int move)
{
    static const char* names[] = {
        "NOT INITIALISED", "RUNTO", "RUNBACK", "JUMPBACK", "TURN", "STRAFE", "ATTACK",
        "SIDEATTACK", "FRONTATTACK", "TRIPLEATTACK", "WHIRLATTACK", "MASTERATTACK", "PREHIT",
        "COMBO", "POSTHIT", "TURNTOHIT", "STORMPREHIT", "PARADE", "STANDUP", "WAIT",
        "ONGROUND", "STUMBLE"
    };
    if (move < 0 || move > 0x15)
        return zSTRING("UNKNOWN ") + zSTRING(move);
    return names[move];
}

zSTRING GetSituationStringFromConstant(int situation)
{
    static const char* names[] = {
        "[ENEMY_PREHIT]", "[ENEMY_STORMPREHIT]", "[Enemy_TurnToHit_focus]", "[My_W_Combo]",
        "[My_W_RunTo] ", "[My_W_Strafe]", "[My_W_Focus]", "[My_W_noFocus]", "[My_G_Combo]",
        "[My_G_Runto]", "[My_G_Strafe]", "[My_G_Focus]", "[My_G_FK_noFocus]", "[My_FK_Focus]",
        "[My_G_FK_noFocus] (2)", "[MY_FK_FOCUS_FAR]", "[MY_FK_NOFOCUS_FAR]",
        "[MY_MAG_NOFOCUS_FAR]", "[MY_MAG_NOFOCUS_FAR]"
    };
    if (situation < 0 || situation > 0x12)
        return zSTRING("[UNKNOWN]") + zSTRING(situation);
    return names[situation];
}

int oCNpc::FindNextFightAction()
{
    if (!enemy)
        return 0x13;
    float distance;
    if (!IsInFightRange(enemy, distance))
        return fighttactic - 1;
    if (IsNpcBetweenMeAndTarget(enemy))
        return 5;
    return GetFightActionFromTable(6);
}

extern zTIMER ztimer;

int oCNpc::ThinkNextFightAction()
{
    fight_waitTime -= ztimer.frameTimeFloat;
    if (fight_waitTime < 0.0f)
        fight_waitTime = 0.0f;
    currentFightAction = FindNextFightAction();
    if (currentFightAction == 0x13)
        fight_waitTime = 200.0f;
    return 0;
}

int oCNpc::angle_far_up;
int oCNpc::angle_far_side;
int oCNpc::angle_near;

zSTRING oCNpc::GetBloodTexture()
{
    return bloodTexture;
}
