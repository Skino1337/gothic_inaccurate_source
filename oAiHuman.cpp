#include "include/oAiHuman.h"
#include "include/oGame.h"
#include "include/zMoving.h"

extern zTIMER ztimer;

static int aiState = -1;
static zSTRING aiPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING aiPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING aiPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING aiPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING aiPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING aiPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING aiPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING aiPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING aiPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING aiPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float aiFarDistance = 250000.0f * 0.0166666675f;
static float aiNearDistance = aiFarDistance * 0.0166666675f;
static float aiFarPlaneDistance = 250000.0f * 24.0f;
// EXE: 0x00AAD648
static zCArray<zCVob*> aiCommandList;
static zCClassDef oCAIHumanClassDef("oCAIHuman", "oCAIBase", oCAIHuman::_CreateNewInstance,
    0, 0x120C, 0, "");
zCClassDef* oCAIHuman::classDef = &oCAIHumanClassDef;
static int aiRuntimeState = -1;
static int aiThreatCount = 0;
static zCModelPrototype* aiSwimModel = 0;
static zCModelPrototype* aiSprintModel = 0;
static int aiModelUsers = 0;
static zSTRING aiRuntimePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING aiRuntimePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING aiRuntimePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING aiRuntimePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING aiRuntimePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING aiRuntimePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING aiRuntimePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING aiRuntimePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING aiRuntimePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING aiRuntimePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float aiRuntimeFarDistance = 250000.0f * 0.0166666675f;
static float aiRuntimeNearDistance = aiRuntimeFarDistance * 0.0166666675f;
static float aiRuntimeFarPlaneDistance = 250000.0f * 24.0f;
zSTRING oCAIHuman::Cam_Normal("CAMMODNORMAL");
zSTRING oCAIHuman::Cam_Run("CAMMODRUN");
zSTRING oCAIHuman::Cam_Fight("CAMMODNORMAL");

oCAIHuman::oCAIHuman()
    : aiCam(0), forcejump(0), lookedAround(0), sprintActive(0), crawlActive(0), showai(1),
      startObserveIntruder(0), dontKnowAniPlayed(0), spellReleased(0), spellCastedLastFrame(0),
      eyeBlinkActivated(0), thirdPersonFallback(0), createFlyDamage(0.0f)
{
    ++aiModelUsers;
    Cam_Normal = zSTRING("CAMMODNORMAL");
    Cam_Run = zSTRING("CAMMODRUN");
    Cam_Fight = zSTRING("CAMMODNORMAL");
}

oCAIHuman::~oCAIHuman()
{
    SetSprintMode(0);
    SetCrawlMode(0);
    aiCam = 0;
    --aiModelUsers;
    if (aiModelUsers <= 0) {
        if (aiSprintModel) {
            aiSprintModel->Release();
            aiSprintModel = 0;
        }
        if (aiSwimModel) {
            aiSwimModel->Release();
            aiSwimModel = 0;
        }
        aiModelUsers = 0;
    }
}

zCClassDef* oCAIHuman::_GetClassDef() const
{
    return classDef;
}

void oCAIHuman::Init(oCNpc* npc)
{
    if (npc)
        oCAniCtrl_Human::Init(npc);
}

int oCAIHuman::DoSimpleAI()
{
    CheckJump();
    if (model->IsStateActive(model->GetAniFromAniID(s_fallenb))) {
        StartAni(t_fallenb_2_stand, _s_walk);
        SetCDStat(1);
        return 1;
    }
    if (model->IsStateActive(model->GetAniFromAniID(s_fallen))) {
        StartAni(t_fallen_2_stand, _s_walk);
        SetCDStat(1);
        return 1;
    }
    if (model->IsStateActive(model->GetAniFromAniID(s_hang))) {
        StartAni(t_hang_2_stand, _s_walk);
        return 1;
    }
    if (IsStanding()) {
        if (actionMode == 5)
            npc->SetBodyState(5);
        else if (actionMode == 6)
            npc->SetBodyState(7);
        else
            npc->SetBodyState(0);
    }
    return 0;
}

int oCAIHuman::Pressed(int key)
{
    return static_cast<int>(zinput->GetState(key));
}

int oCAIHuman::Toggled(int key)
{
    return zinput->GetToggled(key);
}

void oCAIHuman::SetCamModeMob(oCMobInter* mob)
{
    zCAICamera* camera = zCAICamera::GetCurrent();
    if (camera->IsModeActive(CamModDialog))
        return;
    zSTRING mode = "CAMMODMOB";
    if (mob)
        mode += mob->GetScemeName();
    if (!camera->IsModeAvailable(mode))
        mode = "CAMMODMOBDEFAULT";
    SetCamMode(mode, 0);
}

void oCAIHuman::ChangeCamMode(const zSTRING& mode)
{
    SetCamMode(mode, 1);
}

void oCAIHuman::InitCamModes(int weaponMode)
{
    npc->AvoidShrink(1000);
    if (weaponMode == -1)
        weaponMode = npc->GetWeaponMode();
    Cam_Normal = zSTRING(weaponMode >= 7 ? "CAMMODMAGIC" : "CAMMODNORMAL");
    Cam_Run = zSTRING("CAMMODRUN");
    if (weaponMode)
        Cam_Fight = zSTRING("CAMMODMELEE");
}

void oCAIHuman::SetCamMode(const zSTRING& mode, int immediate)
{
}

void oCAIHuman::WeaponChoose()
{
    if (!Pressed(8)) {
        if (!changeweapon) {
            if (npc->GetTrueGuild() > 0x42 || IsStanding()) {
                SetActionMode(0);
            } else {
                oCMsgWeapon* message = new oCMsgWeapon(5, 0, 0);
                npc->GetEM(0)->OnMessage(message, npc);
            }
            InitCamModes(-1);
            SetCamMode(Cam_Normal, 0);
            oCNpcFocus::SetFocusMode(0);
            return;
        }
        if (npc->GetTrueGuild() <= 0x42) {
            oCMsgWeapon* message = new oCMsgWeapon(2, 0, 0);
            npc->GetEM(0)->OnMessage(message, npc);
        }
        InitCamModes(wmode_selected);
        if (!(Cam_Normal == zSTRING("CAMMODMAGIC")))
            SetCamMode(Cam_Normal, 0);
        oCNpcFocus::SetFocusMode(wmode_selected >= 5 ? 2 : 1);
        return;
    }
    if (Toggled(3) && npc->GetTrueGuild() <= 0x42) {
        oCMsgWeapon* message = new oCMsgWeapon(6, 1, 0);
        npc->GetEM(0)->OnMessage(message, npc);
    }
    if (Toggled(4) && npc->GetTrueGuild() <= 0x42) {
        oCMsgWeapon* message = new oCMsgWeapon(6, -1, 0);
        npc->GetEM(0)->OnMessage(message, npc);
    }
    if (wmode_selected < 7)
        return;
    npc->EndMovement(1);
    oCMag_Book* spellBook = npc->GetSpellBook();
    if (!spellBook || spellBook->IsInMovement())
        return;
    int remove = 0;
    if (Pressed(1) || Pressed(0x12))
        remove = -1;
    else if (Pressed(2) || Pressed(0x13))
        remove = 1;
    if (!remove)
        return;
    changeweapon = 1;
    oCMsgMagic* message = new oCMsgMagic(2, wmode_selected, remove);
    message->activeSpell = spellBook->GetSelectedSpellNr();
    npc->GetEM(0)->OnMessage(message, npc);
}

int oCAIHuman::BowMode(int oldControls)
{
    if (!npc || !IsStanding())
        return 0;
    if (!npc->IsMunitionAvailable(npc->GetWeapon())) {
        npc->weaponMode = 0;
        return 0;
    }
    return 1;
}

// EXE: 0x00696950
int oCAIHuman::GetEnemyThreat()
{
    aiCommandList.DeleteList();
    aiThreatCount = 0;
    if (!npc || !npc->homeWorld)
        return 0;
    static int attackState = -2;
    static int monsterAttackState = -2;
    if (attackState == -2) {
        zSTRING name = "ZS_ATTACK";
        attackState = parser.GetIndex(name);
    }
    if (monsterAttackState == -2) {
        zSTRING name = "ZS_MM_ATTACK";
        monsterAttackState = parser.GetIndex(name);
    }
    zVEC3 position = npc->trafoObjToWorld.GetTranslation();
    for (int i = 0; i < npc->homeWorld->activeVobList.numInArray; ++i) {
        oCNpc* other = dynamic_cast<oCNpc*>(npc->homeWorld->activeVobList.parray[i]);
        if (!other || other == npc || !other->IsConditionValid() || !other->GetWeaponMode())
            continue;
        zVEC3 otherPosition = other->trafoObjToWorld.GetTranslation();
        zVEC3 delta(otherPosition.n[0] - position.n[0], otherPosition.n[1] - position.n[1],
            otherPosition.n[2] - position.n[2]);
        if (delta.LengthApprox() >= 1000.0f)
            continue;
        aiCommandList.InsertEnd(other);
        aiThreatCount = aiCommandList.numInArray;
        if (other->enemy != npc)
            continue;
        if (other->state.IsInState(attackState) || other->state.IsInState(monsterAttackState))
            return npc->GetWeaponMode() ? 2 : 1;
    }
    if (npc->GetAttribute(0) <= 5)
        return 1;
    zSTRING* sector = npc->GetSectorNameVobIsIn();
    return sector && sector->Search("WALD") >= 0 ? 1 : 0;
}

void oCAIHuman::CheckFightCamera()
{
    if (aiThreatCount > 0 || !npc || npc->GetWeaponMode())
        return;
    if (zCAICamera::GetCurrent()->IsModeActive(Cam_Fight)) {
        if (walkmode)
            SetCamMode(Cam_Normal, 0);
        else
            SetCamMode(Cam_Run, 0);
    }
}

void oCAIHuman::SetSprintMode(int enabled)
{
    if (!npc || npc->GetTrueGuild() > 0x10 || enabled == sprintActive)
        return;
    if (!aiSprintModel) {
        zSTRING modelName = "HUMANS_SPRINT.MDS";
        aiSprintModel = zCModelPrototype::Load(modelName, model->modelProtoList.parray[0]);
    }
    if (aiSprintModel) {
        if (enabled && !(forcejump & 4)) {
            model->ApplyModelProtoOverlay(aiSprintModel);
            forcejump |= 4;
            InitAnimations();
            if (IsWalking())
                model->StartAni(_s_walkl, 0);
            SetNextAni(_t_walk_2_walkl, _s_walkl);
        } else if (!enabled && (forcejump & 4)) {
            int walking = IsWalking();
            model->RemoveModelProtoOverlay(aiSprintModel);
            forcejump &= ~4;
            InitAnimations();
            if (GetWaterLevel() != 2) {
                model->StartAni(walking ? _s_walkl : _s_walk, 0);
                SetNextAni(_t_walkl_2_walk, _s_walk);
            } else {
                model->StartAni(s_swim, 0);
            }
        }
    }
    sprintActive = enabled;
}

void oCAIHuman::SetCrawlMode(int enabled)
{
    if (!npc || npc->GetTrueGuild() > 0x10 || enabled == crawlActive)
        return;
    if (!aiSwimModel) {
        zSTRING modelName = "HUMANS_SWIM.MDS";
        aiSwimModel = zCModelPrototype::Load(modelName, model->modelProtoList.parray[0]);
    }
    if (aiSwimModel) {
        if (enabled && !(forcejump & 8)) {
            model->ApplyModelProtoOverlay(aiSwimModel);
            forcejump |= 8;
            InitAnimations();
            model->StartAni(model->GetActiveAni(s_swimf) ? s_swimf : s_swim, 0);
            SetWalkMode(4);
            SetActionMode(5);
        } else if (!enabled && (forcejump & 8)) {
            model->RemoveModelProtoOverlay(aiSwimModel);
            forcejump &= ~8;
            InitAnimations();
            if (actionMode == 5) {
                SetWalkMode(4);
                SetActionMode(5);
                model->StartAni(model->GetActiveAni(s_swimf) ? s_swimf : s_swim, 0);
            }
        }
    }
    crawlActive = enabled;
}

zSTRING oCItem::GetSchemeName()
{
    return scemeName;
}

int oCAIHuman::FightMelee()
{
    CheckFightCamera();
    if (!npc)
        return 0;
    if (Pressed(5)) {
        npc->TurnToEnemy();
        oCMsgAttack* message = 0;
        if (Toggled(3)) {
            if (IsWalking() && _t_hitfrun != -1)
                message = new oCMsgAttack(3, _t_hitfrun, 1);
            else
                message = new oCMsgAttack(0, _t_hitf, 1);
        } else if (Toggled(4) || Toggled(0x19)) {
            StopTurnAnis();
            message = new oCMsgAttack(5, npc->GetFocusNpc(), 0.0f);
        } else if (Toggled(1) || Toggled(0x12) || Toggled(0x1a)) {
            message = new oCMsgAttack(1, _t_hitl, 1);
        } else if (Toggled(2) || Toggled(0x13) || Toggled(0x1b)) {
            message = new oCMsgAttack(2, _t_hitr, 1);
        }
        if (message) {
            npc->GetEM(0)->OnMessage(message, npc);
            return 1;
        }
    }
    if (!npc->enemy)
        return 0;
    return npc->FightAttackMelee(npc->currentFightAction);
}

int oCAIHuman::CheckMobInteraction()
{
    if (!npc)
        return 0;
    oCMobInter* mob = npc->GetInteractMob();
    if (!mob)
        return 0;
    if (Pressed(8)) {
        npc->Interrupt(0, 1);
        model->StartAni(_s_walk, 0);
        return 0;
    }
    SetCamModeMob(mob);
    if (Pressed(5) && Toggled(4)) {
        mob->StopInteraction(npc);
        oCMsgManipulate* message = new oCMsgManipulate(0, mob);
        npc->GetEM(0)->OnMessage(message, mob);
        npc->SetInteractMob(0);
        return 1;
    }
    int left = Pressed(1) || Pressed(0x12);
    int right = Pressed(2) || Pressed(0x13);
    mob->Interact(npc, 1, right, left, Pressed(3), Pressed(5));
    if (mob->IsInteractingWith(npc))
        return 1;
    npc->SetInteractMob(0);
    return 0;
}

int oCAIHuman::CheckItemInteraction()
{
    if (!npc || npc->interactItemCurrentState == -1)
        return 0;
    oCItem* item = npc->interactItem;
    int state = npc->interactItemCurrentState;
    if (item && Pressed(3)) {
        zSTRING scheme = item->GetSchemeName();
        if (npc->InteractItemGetAni(scheme, state, state + 1)) {
            oCMsgManipulate* message = new oCMsgManipulate(0xf, item, state + 1);
            npc->GetEM(0)->OnMessage(message, npc);
            return 1;
        }
    }
    if (item && Pressed(4)) {
        zSTRING scheme = item->GetSchemeName();
        if (npc->InteractItemGetAni(scheme, state, state - 1)) {
            oCMsgManipulate* message = new oCMsgManipulate(0xf, item, state - 1);
            npc->GetEM(0)->OnMessage(message, npc);
            return 1;
        }
    }
    if (item && Pressed(5) && state) {
        zSTRING scheme = item->GetSchemeName();
        if (npc->InteractItemGetAni(scheme, state, -1)) {
            oCMsgManipulate* message = new oCMsgManipulate(0xf, item, -1);
            npc->GetEM(0)->OnMessage(message, npc);
            return 1;
        }
    }
    return 1;
}

int oCAIHuman::StandActions()
{
    if (!npc)
        return 0;
    oCNpc* focusNpc = npc->GetFocusNpc();
    if (focusNpc) {
        if (focusNpc->IsDead() || focusNpc->IsUnconscious()) {
            npc->OpenDeadNpc();
            return 1;
        }
    } else {
        oCMobInter* mob = dynamic_cast<oCMobInter*>(npc->GetFocusVob());
        if (mob && IsStanding() && !Pressed(8) && !mob->IsInState(npc, 0)
            && mob->IsAvailable(npc)) {
            oCMobMsg* message = new oCMobMsg(oCMobMsg::EV_STARTINTERACTION, npc);
            mob->GetEM(0)->OnMessage(message, npc);
            return 1;
        }
        oCItem* item = dynamic_cast<oCItem*>(npc->GetFocusVob());
        if (item && npc->CanCarry(item)) {
            oCMsgManipulate* message = new oCMsgManipulate(0, item);
            npc->GetEM(0)->OnMessage(message, npc);
            zCAICamera::GetCurrent()->SubTarget(item);
            return 1;
        }
        if (!item && npc->GetRightHand()) {
            model->StartAni(t_stand_2_iaim, 0);
            return 1;
        }
    }
    if (npc->GetInteractMob())
        return CheckMobInteraction();
    if (npc->interactItem)
        return CheckItemInteraction();
    return 0;
}

int oCAIHuman::PC_CheckSpecialStates()
{
    if (model->IsAniActive(model->GetAniFromAniID(_s_jump))) {
        if (aboveFloor > 0.0f) {
            StartPhysicsWithVel();
            zSTRING ani = "T_JUMP_2_FALLDN";
            model->StartAni(ani, 0);
        } else {
            zCModelAni* ani;
            if (Pressed(3) && !Pressed(0x0b) && !walkmode)
                ani = model->GetAniFromAniID(_t_jump_2_runl);
            else
                ani = model->GetAniFromAniID(t_jump_2_stand);
            LandAndStartAni(ani);
        }
        return 0;
    }
    if (model->IsAniActive(model->GetAniFromAniID(s_hang))) {
        SetCamMode(zSTRING("CAMMODCLIMB"), 1);
        if (!Pressed(4))
            StartAni(t_hang_2_stand, _s_walk);
        return 1;
    }
    if (model->IsStateActive(model->GetAniFromAniID(s_fallen))) {
        npc->SetTorchAni(0, 1);
        if (Pressed(3)) {
            zSTRING ani = "T_FALLEN_2_STAND";
            model->StartAni(ani, 0);
            npc->SetCollDetStat(1);
            npc->SetCollDetDyn(1);
        }
        return 1;
    }
    if (model->IsStateActive(model->GetAniFromAniID(s_fallenb))) {
        npc->SetTorchAni(0, 1);
        if (Pressed(3)) {
            zSTRING ani = "T_FALLENB_2_STAND";
            model->StartAni(ani, 0);
            npc->SetCollDetStat(1);
            npc->SetCollDetDyn(1);
        }
        return 1;
    }
    return oCAniCtrl_Human::CheckSpecialStates();
}

void oCAIHuman::PC_Diving()
{
    SetCamMode(zSTRING("CAMMODDIVE"), 0);
    if (Pressed(0x0b) || Pressed(5))
        _Forward();
    else
        _Stand();
    float mouseX;
    float mouseY;
    float wheel;
    zinput->GetMousePos(mouseX, mouseY, wheel);
    if (!Pressed(3) && !Pressed(4) && mouseY != 0.0f)
        DiveRotateX(mouseY * ztimer.frameTimeFloat * 0.1f);
    PC_Turnings(0);
}

void oCAIHuman::PC_Turn(float angle, int animate)
{
    oCAniCtrl_Human::Turn(angle, animate);
}

int oCAIHuman::PC_SpecialMove(int enabled)
{
    static int specialMoveActive = 0;
    if (!enabled) {
        specialMoveActive = 0;
        do_jump = 0;
        return 0;
    }
    if (actionMode == 5) {
        if (!specialMoveActive) {
            model->StartAni(t_swim_2_dive, 0);
            SetWalkMode(5);
            SetActionMode(6);
        }
        specialMoveActive = 1;
        return 1;
    }
    specialMoveActive = 1;
    if (model->GetActiveAni(_s_walkl) || model->GetActiveAni(_s_walk)) {
        do_jump = 1;
        return 1;
    }
    do_jump = 0;
    return 0;
}

int oCAIHuman::PC_ActionMove(int oldControls)
{
    if (!npc)
        return 0;
    int mode = npc->GetWeaponMode();
    if (mode) {
        if (mode <= 4)
            return FightMelee();
        if (mode <= 6)
            return BowMode(oldControls);
        return MagicMode();
    }
    if (IsStanding()) {
        oCNpc* focusNpc = npc->GetFocusNpc();
        if (focusNpc && focusNpc->IsWaitingForAnswer()) {
            if (!oCNpc::IsInPerceptionRange(0x13, npc, focusNpc)) {
                focusNpc->SetCurrentAnswer(0, 0, npc);
                return focusNpc->SetCurrentAnswer(0, 1, npc);
            }
            if (!focusNpc->CanBeTalkedTo())
                return 1;
            int answer = oldControls ? 1 : Pressed(0x0e) ? -1 : 0;
            return focusNpc->SetCurrentAnswer(answer, oldControls, npc);
        }
        if (oldControls && zinput->GetToggled(5))
            return StandActions();
    }
    return 0;
}

int oCAIHuman::PC_WeaponMove(int enabled)
{
    if (!enabled || !npc || npc->GetCarryVob() || actionMode == 5 || actionMode == 6)
        return 0;
    if (IsStanding()) {
        oCMsgWeapon* message = new oCMsgWeapon(npc->GetWeaponMode() ? 4 : 1, wmode_last, 0);
        npc->GetEM(0)->OnMessage(message, npc);
        return 1;
    }
    if (IsWalking()) {
        if (npc->GetWeaponMode()) {
            oCMsgWeapon* message = new oCMsgWeapon(3, 0, 0);
            npc->GetEM(0)->OnMessage(message, npc);
            return 1;
        }
        int nextMode = npc->GetNextWeaponMode(wmode_last - 1, 1, 0);
        if (nextMode > 0) {
            oCMsgWeapon* message = new oCMsgWeapon(0, nextMode, 0);
            npc->GetEM(0)->OnMessage(message, npc);
            return 1;
        }
    }
    return 0;
}

int oCAIHuman::PC_SlowMove(int enabled)
{
    if ((enabled != 0) != (CapsLock() != 0))
        SetCapsLock(enabled);
    if (actionMode == 5) {
        if (model->IsStateActive(model->GetAniFromAniID(s_swim))
            || model->IsAniActive(model->GetAniFromAniID(s_swimf)))
            SetCrawlMode(enabled ? 1 : 0);
        return 0;
    }
    if (IsWalking() && walkmode == 1 && CanToggleWalkModeTo(0)) {
        ToggleWalkMode(-1);
        return 0;
    }
    if (IsStanding() && walkmode == 1 && CanToggleWalkModeTo(0)) {
        ToggleWalkMode(-1);
        return 1;
    }
    if ((IsStanding() || IsWalking()) && walkmode == 2 && CanToggleWalkModeTo(0)) {
        ToggleWalkMode(-1);
        return 0;
    }
    if (enabled && IsStanding() && !walkmode && CanToggleWalkModeTo(1)) {
        ToggleWalkMode(-1);
        return 1;
    }
    return 0;
}

int oCAIHuman::PC_Sneak(int enabled)
{
    if (!enabled)
        return 0;
    if (walkmode == 2) {
        SetWalkMode(0);
        return 1;
    }
    SetWalkMode(2);
    ResetObserveSuspectCounter();
    return 1;
}

void oCAIHuman::PC_Turnings(int oldControls)
{
    if (ztimer.frameTimeFloat == 0.0f)
        return;
    static int rotationInitialized = 0;
    static float rotationScale = 1.0f;
    if (!rotationInitialized) {
        rotationInitialized = 1;
        rotationScale = zoptions->ReadReal("ENGINE", "zMouseRotationScale", 1.0f);
    }
    npc->AvoidShrink(1000);
    if (Pressed(5) || oldControls) {
        float mouseX;
        float mouseY;
        float wheel;
        zinput->GetMousePos(mouseX, mouseY, wheel);
        mouseX *= rotationScale;
        if (mouseX < -100.0f)
            mouseX = -100.0f;
        else if (mouseX > 100.0f)
            mouseX = 100.0f;
        if (Pressed(1)) {
            Turn(npc->GetWeaponMode() ? -2.0f : -1.0f, 1);
            return;
        }
        if (Pressed(2)) {
            Turn(npc->GetWeaponMode() ? 2.0f : 1.0f, 1);
            return;
        }
        if (mouseX < -10.0f || mouseX > 10.0f) {
            Turn(mouseX * 0.1f, 1);
            return;
        }
    }
    StopTurnAnis();
}

int oCAIHuman::PC_Strafe(int enabled)
{
    if (actionMode == 5)
        return 0;
    if (Pressed(0x12)) {
        oCMsgMovement* message = new oCMsgMovement(0xd, npc->GetFocusNpc());
        message->ani = _t_strafel;
        npc->GetEM(0)->OnMessage(message, npc);
        return 1;
    }
    if (Pressed(0x13)) {
        oCMsgMovement* message = new oCMsgMovement(0xd, npc->GetFocusNpc());
        message->ani = _t_strafer;
        npc->GetEM(0)->OnMessage(message, npc);
        return 1;
    }
    return 0;
}

static float observeSuspectTimer = 3000.0f;
static float footStepTimer = 0.0f;
static float assessUseMobTimer = 3000.0f;

void oCAIHuman::_WalkCycle()
{
    if (!walkmode)
        npc->SetWalkStopChasm(0);
    if (!PC_ActionMove(Pressed(5))
        && !PC_SpecialMove(Pressed(0x0b) || (actionMode == 5 && Pressed(5)))
        && !PC_WeaponMove(Pressed(8)) && !PC_SlowMove(Pressed(6))
        && !PC_Sneak(Toggled(0x11)) && !PC_Strafe(Pressed(0x12) || Pressed(0x13))) {
        PC_Turnings(Pressed(5));
        if (Pressed(3)) {
            PC_GoForward();
            if (!Pressed(0x0b))
                SetCamMode(zSTRING("CAMMODRUN"), 1);
        } else if (Pressed(4)) {
            PC_GoBackward();
        } else {
            SetCrawlMode(0);
            SetSprintMode(0);
            _Stand();
            SetCamMode(zSTRING("CAMMODNORMAL"), 1);
        }
    }
    if (do_jump)
        PC_JumpForward();
    CreateObserveSuspect(0);
    CreateFootStepSound(0);
}

void oCAIHuman::ResetObserveSuspectCounter()
{
    observeSuspectTimer = 3000.0f;
}

void oCAIHuman::CreateObserveSuspect(int reset)
{
    if (walkmode != 2 || IsStanding())
        return;
    observeSuspectTimer += ztimer.frameTimeFloat;
    if (observeSuspectTimer < 3000.0f)
        return;
    observeSuspectTimer -= 3000.0f;
    npc->CreatePassivePerception(0x19, npc, 0);
}

void oCAIHuman::CreateFootStepSound(int reset)
{
    if (walkmode == 2 || !IsWalking())
        return;
    if (reset) {
        footStepTimer = 0.0f;
        return;
    }
    footStepTimer += ztimer.frameTimeFloat;
    if (footStepTimer < 1000.0f)
        return;
    footStepTimer -= 1000.0f;
    oCNpc::AssessQuietSound_S(npc, npc->trafoObjToWorld.GetTranslation());
}

void oCAIHuman::ResetAssessUseMobCounter()
{
    assessUseMobTimer = 3000.0f;
}

void oCAIHuman::CreateAssessUseMob()
{
    if (!npc)
        return;
    oCMobInter* mob = npc->GetInteractMob();
    if (!mob)
        return;
    assessUseMobTimer += ztimer.frameTimeFloat;
    if (assessUseMobTimer < 3000.0f)
        return;
    assessUseMobTimer -= 3000.0f;
    npc->AssessUseMob_S(mob);
}

int oCAIHuman::MoveCamera()
{
    return 0;
}

void oCAIHuman::CamMessages()
{
}

void oCAIHuman::CheckFocusVob(int oldControls)
{
    if (oCNpc::game_mode == 1 || oCNpc::game_mode == 2)
        return;
    if (!Pressed(0x10))
        StopLookAtTarget();
    int left = Pressed(1) || Pressed(0x12) || Pressed(0x1a);
    int right = Pressed(2) || Pressed(0x13) || Pressed(0x1b);
    oCNpc* focusNpc = npc->GetFocusNpc();
    int deadFocus = focusNpc && (focusNpc->IsDead() || focusNpc->IsUnconscious());
    if (Pressed(5)) {
        int weaponMode = npc->GetWeaponMode();
        if (!weaponMode || (weaponMode > 4 && weaponMode <= 7)) {
            if (left || right) {
                npc->ToggleFocusVob(left ? 1 : 0);
                SetLookAtTarget(npc->GetFocusNpc());
                return;
            }
        }
        if (npc->GetFocusVob() && !deadFocus)
            return;
    }
    npc->CollectFocusVob(oldControls);
    if (npc->enemy && npc->enemy->IsDead())
        npc->SetEnemy(0);
}

void oCAIHuman::Moving()
{
    CheckFocusVob(0);
    CheckItemInteraction();
    zCAICamera* camera = zCAICamera::GetCurrent();
    int playerIsMovable = !camera || camera->playerIsMovable;
    int cutscene = npc->homeWorld && npc->homeWorld->csPlayer->GetPlayingGlobalCutscene();
    if (!npc->IsMovLock() && !cutscene && !npc->state.CanPlayerUseAIState() && playerIsMovable) {
        if (!IsSliding()) {
            if (!aiCam)
                aiCam = ogame->GetCameraAI();
            if (!(forcejump & 2) && !CheckMobInteraction() && !PC_CheckSpecialStates()) {
                if (actionMode == 6)
                    PC_Diving();
                else if (actionMode == 7)
                    WeaponChoose();
                else
                    _WalkCycle();
            }
        }
        return;
    }
    _Stand();
}

void oCAIHuman::DoAI(zCVob* vob, int& state)
{
    state = 0;
    npc->AvoidShrink(1000);
    if (npc->IsDead()) {
        if (npc->IsAPlayer() && npc->inventory2.IsActive())
            npc->inventory2.Deactivate();
        ogame->GetSpawnManager()->CheckRemoveNpc(npc);
        return;
    }
    if (npc == oCNpc::player)
        ChangeCamModeBySituation();
    npc->CheckAngryTime();
    CorrectAniStates();
    if (oCNpc::ai_baseEnabled)
        StartUp(npc);
    if (actionMode != 7)
        CheckWaterLevel();
    CheckSpecialStates();
    npc->state.DoAIState();
    if (!oCNpc::ai_disabled) {
        DoHackSpecials();
        if (npc == oCNpc::player) {
            npc->CheckSpecialSituations();
            Moving();
        } else {
            DoSimpleAI();
            if (!npc->GetEM(0)->GetCutsceneMode())
                npc->PerceptionCheck();
        }
        DoAniWithOffset();
    }
    if (oCNpc::ai_baseEnabled)
        End();
    LookAtTarget();
    DoAutoRoll();
    npc->DoSpellBook();
    npc->UpdateSlots();
    if (npc->IsAPlayer())
        CheckActiveSpells();
    state = npc->DoActiveSpells();
    ogame->GetSpawnManager()->CheckRemoveNpc(npc);
}

void oCAIHuman::DoHackSpecials()
{
    if (model)
        model->CorrectAniFreezer();
    if (npc == oCNpc::player && model->IsStateActive(model->GetAniFromAniID(s_falldn))
        && !(npc->showVisual & 8))
        npc->SetPhysicsEnabled(1);
}

void oCAIHuman::ChangeCamModeBySituation()
{
    zCAICamera* camera = zCAICamera::GetCurrent();
    if (!camera || ztimer.frameTimeFloat == 0.0f || camera->IsModeActive(CamModDialog))
        return;
    if (IsDead() || npc->IsUnconscious()) {
        SetCamMode(zSTRING("CAMMODDEATH"), 0);
        return;
    }
    if (actionMode == 6 || npc->GetBodyState() == 7) {
        SetCamMode(zSTRING("CAMMODDIVE"), 0);
        return;
    }
    if (actionMode == 5 || GetWaterLevel() > 1) {
        SetCamMode(zSTRING("CAMMODSWIM"), 0);
        return;
    }
    if (model->IsAniActive(model->GetAniFromAniID(s_hang))) {
        SetCamMode(zSTRING("CAMMODCLIMB"), 0);
        return;
    }
    if (model->IsAniActive(model->GetAniFromAniID(s_fall))
        || model->IsAniActive(model->GetAniFromAniID(s_fallb))) {
        SetCamMode(zSTRING("CAMMODFALL"), 0);
        return;
    }
    oCMobInter* mob = npc->GetInteractMob();
    if (mob && !Pressed(8)) {
        SetCamModeMob(mob);
        return;
    }
    if (npc->inventory2.IsOpen()) {
        SetCamMode(zSTRING("CAMMODINVENTORY"), 0);
        return;
    }
    int weaponMode = npc->GetWeaponMode();
    if (weaponMode == 7)
        SetCamMode(zSTRING("CAMMODMAGIC"), 0);
    else if (weaponMode == 5 || weaponMode == 6)
        SetCamMode(zSTRING("CAMMODRANGED"), 0);
    else if (weaponMode >= 1 && weaponMode <= 4)
        SetCamMode(zSTRING("CAMMODMELEE"), 0);
    else
        SetCamMode(zSTRING("CAMMODNORMAL"), 0);
}

zSTRING oCAICamera::GetPreviousMode()
{
    return oldCamSys;
}

void oCAIHuman::DoMonsterSpecial()
{
}

void oCAIHuman::SetShowAI(int enabled)
{
    showai = enabled;
}

int oCAIHuman::GetShowAI()
{
    return showai;
}

int oCAIHuman::CanThisCollideWith(zCVob* vob)
{
    for (int i = 0; i < ignoreVobList.numInArray; ++i) {
        if (ignoreVobList.parray[i] == vob)
            return 0;
    }
    return 1;
}

void oCAIHuman::AddIgnoreCD(zCVob* vob)
{
    ignoreVobList.InsertEnd(vob);
}

void oCAIHuman::SubIgnoreCD(zCVob* vob)
{
    ignoreVobList.Remove(vob);
}

void oCAIHuman::StartFlyDamage(float damage, zVEC3& direction)
{
    float maxDamage = 100.0f;
    float minDamage = 0.0f;
    float perPoint = 1.0f;
    zCPar_Symbol* symbol = parser.GetSymbol("DAMAGE_FLY_CM_MAX");
    if (symbol)
        symbol->GetValue(maxDamage, 0);
    symbol = parser.GetSymbol("DAMAGE_FLY_CM_MIN");
    if (symbol)
        symbol->GetValue(minDamage, 0);
    symbol = parser.GetSymbol("DAMAGE_FLY_CM_PER_POINT");
    if (symbol)
        symbol->GetValue(perPoint, 0);
    if (perPoint <= 0.0f)
        perPoint = 1.0f;
    float impulse = damage * perPoint;
    if (impulse < minDamage)
        impulse = minDamage;
    if (impulse > maxDamage)
        impulse = maxDamage;
    direction *= impulse * 100.0f;
    direction.n[1] *= 0.8f;
    float azimuth;
    float elevation;
    npc->GetAngles(direction, azimuth, elevation);
    zSTRING ani = azimuth >= -90.0f && azimuth <= 90.0f ? "S_FALL" : "S_FALLB";
    model->StartAni(ani, 0);
    zVEC3 position = npc->trafoObjToWorld.GetTranslation();
    position.n[1] += 30.0f;
    npc->SetPositionWorld(position);
    npc->SetPhysicsEnabled(1);
    zCRigidBody* body = npc->GetRigidBody();
    body->gravityOn = 1;
    body->ApplyImpulseCM(direction);
    createFlyDamage = damage;
}

void oCAIHuman::Archive(zCArchiver& archiver)
{
    oCAniCtrl_Human::Archive(archiver);
    archiver.InSaveGame();
}

void oCAIHuman::Unarchive(zCArchiver& archiver)
{
    oCAniCtrl_Human::Unarchive(archiver);
    archiver.InSaveGame();
}


int oCAIHuman::CheckActiveSpells()
{
    if (ztimer.frameTimeFloat == 0.0f || !zinput->KeyToggled(28))
        return 0;
    for (int spellID = 47; spellID <= 58; ++spellID)
    {
        oCSpell* spell = npc->IsSpellActive(spellID);
        if (spell)
        {
            spell->EndTimedEffect();
            return 1;
        }
    }
    return 0;
}

int oCAIHuman::MagicInvestSpell()
{
    oCMag_Book* book = npc->GetSpellBook();
    oCSpell* spell = book->GetSelectedSpell();
    if (!spell)
        return 0;

    book->Spell_Setup(npc, npc->GetFocusVob(), 0);
    int status = spell->GetSpellStatus();
    if (status == 0 && spell->manaInvested == 0 && !(forcejump & 0x40))
    {
        npc->GetModel()->StartAni("T_CASTFAIL", 0);
        forcejump |= 0x40;
    }

    if (IsWalking() || (status != 1 && status != 8))
        return 0;

    zSTRING spellAni;
    zCPar_Symbol* symbol = parser.GetSymbol("spellFXAniLetters");
    if (symbol)
        symbol->GetValue(spellAni, spell->GetSpellID());
    SetAnimations(npc->GetWeaponMode(), spellAni);
    book->Spell_Invest(spell);
    status = spell->GetSpellStatus();

    if (spell->manaInvested)
    {
        if (status)
        {
            TransitionToInvest();
            npc->AssessCaster_S(spell);
        }
    }
    else if (!(forcejump & 0x40))
    {
        npc->GetModel()->StartAni("T_CASTFAIL", 0);
        forcejump |= 0x40;
    }

    if (status == 2 || status == 3)
        forcejump |= 0x80;
    else
        forcejump &= ~0x80;
    return 1;
}

int oCAIHuman::MagicCheckSpellStates(int spellID)
{
    oCMag_Book* book = npc->GetSpellBook();
    oCSpell* spell = book->GetSelectedSpell();
    if (!spell)
        return 0;

    int status = spell->GetSpellStatus();
    if (status == 2)
    {
        if (!spell->manaInvested)
        {
            spell->spellStatus = 1;
            TransitionToStand();
            return 1;
        }

        if (s_shoot == -1)
        {
            book->Spell_Cast(spell);
            TransitionToCast();
        }
        else if (!npc->GetModel()->IsAniActive(npc->GetModel()->GetAniFromAniID(s_shoot)))
        {
            if (IsStanding())
                spell->spellStatus = 1;
            TransitionToCast();
        }
        else
        {
            book->Spell_Cast(spell);
            forcejump |= 0x100;
            TransitionToCast();
        }
    }
    else if (status == 3)
    {
        TransitionToStand();
        if (spell->manaInvested <= 0)
            spell->spellStatus = 1;
        else
        {
            book->StopSelectedSpell();
            book->GetSelectedSpell()->Open();
        }
    }
    else if (!spellID)
    {
        if (forcejump >= 0)
        {
            spell->SetReleaseStatus();
            forcejump |= 0x80;
        }
        if (s_shoot != -1 && npc->GetModel()->IsAniActive(
            npc->GetModel()->GetAniFromAniID(s_shoot)))
            TransitionToStand();
    }
    return 1;
}

int oCAIHuman::MagicMode()
{
    if (ztimer.frameTimeFloat == 0.0f)
        return 0;

    npc->EndMovement(1);
    oCMag_Book* book = npc->GetSpellBook();
    oCSpell* spell = book->GetSelectedSpell();
    if (!spell)
        return 0;

    if (spell->effect && spell->effect->fxState <= 1)
    {
        book->SetOwner(npc);
        book->SetShowHandSymbol(1);
    }

    int invested = 0;
    bool released = !Pressed(5) || (oCGame::s_bUseOldControls && Pressed(3));
    if (released)
    {
        forcejump &= ~0x40;
        if (spell->manaInvested > 0)
            spell->IsInvestSpell();
        ChangeCamMode(Cam_Normal);
    }
    else if (!(forcejump & 0x100))
    {
        if (spell->canTurnDuringInvest)
            npc->TurnToEnemy();
        invested = MagicInvestSpell();
        int status = spell->GetSpellStatus();
        if (status == 0 || status == 2 || status == 3)
        {
            if (spell->manaInvested > 0)
                spell->IsInvestSpell();
            ChangeCamMode(Cam_Normal);
        }
    }

    MagicCheckSpellStates(invested);
    npc->BeginMovement();
    return invested;
}
