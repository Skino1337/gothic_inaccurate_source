#include "include/oCollisionObjectMiscChar.h"
#include "include/oGil.h"
#include "include/oNpcMessages.h"

static int animationState = -1;
static int animationRuntimeState = -1;
static float animationFarDistance = 250000.0f * 0.0166666675f;
static float animationNearDistance = animationFarDistance * 0.0166666675f;
static float animationFarPlaneDistance = 250000.0f * 24.0f;
static float animationRuntimeFarDistance = 250000.0f * 0.0166666675f;
static float animationRuntimeNearDistance = animationRuntimeFarDistance * 0.0166666675f;
static float animationRuntimeFarPlaneDistance = 250000.0f * 24.0f;
static zSTRING animationPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING animationPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING animationPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING animationPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING animationPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING animationPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING animationPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING animationPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING animationPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING animationPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
zCOLOR COLOR_YES(0, 255, 255, 255);
zCOLOR COLOR_NO(192, 192, 192, 255);
const zSTRING ANI_EYEBLINK("R_EYESBLINK");
static zCClassDef oCAniCtrlHumanClassDef("oCAniCtrl_Human", "oCAIBase",
    oCAniCtrl_Human::_CreateNewInstance, 0, 0x11F8, 0, "");
zCClassDef* oCAniCtrl_Human::classDef = &oCAniCtrlHumanClassDef;

oCAniCtrl_Human::oCAniCtrl_Human()
    : actionMode(0), wmode_last(0), walkmode(0), walkStopChasm(0), npc(0), changeweapon(0),
      wmode_selected(0), do_jump(0), _s_walk(0), s_swim(-1), s_dive(-1), targetVob(0),
      angle_slide1(0.0f), angle_slide2(0.0f), angle_heading(0.0f), angle_horiz(0.0f),
      angle_ground(0.0f)
{
    for (int i = 0; i < 8; ++i) {
        s_run[i] = -1;
        s_walk[i] = -1;
        s_sneak[i] = -1;
    }
}

oCAniCtrl_Human::~oCAniCtrl_Human()
{
    npc = 0;
    if (targetVob) {
        targetVob->Release();
        targetVob = 0;
    }
}

zCClassDef* oCAniCtrl_Human::_GetClassDef() const
{
    return classDef;
}

void oCAniCtrl_Human::Init(oCNpc* newNpc)
{
    npc = newNpc;
    if (npc)
        InitAnimations();
}

int oCAniCtrl_Human::StartAni(int aniID, int nextAniID)
{
    if (aniID == -1 || !npc)
        return 0;
    npc->GetModel()->StartAni(aniID, nextAniID);
    return 1;
}

void oCAniCtrl_Human::SetNextAni(int aniID, int nextAniID)
{
    if (aniID != -1 && npc)
        npc->GetModel()->StartAni(aniID, nextAniID);
}

void oCAniCtrl_Human::InitAnimations()
{
    InitAllAnis();
    SearchStandAni(1);
}

void oCAniCtrl_Human::SearchStandAni(int force)
{
    if (!npc)
        return;
    if (actionMode == 5) {
        if (s_swim != -1)
            npc->GetModel()->StartAni(s_swim, 0);
    } else if (actionMode == 6) {
        if (s_dive != -1)
            npc->GetModel()->StartAni(s_dive, 0);
        npc->SetPhysicsEnabled(0);
    } else {
        StartStandAni();
    }
}

void oCAniCtrl_Human::StartStandAni()
{
    if (npc)
        npc->GetModel()->StartAni(_s_walk, 0);
}

void oCAniCtrl_Human::Reset()
{
    if (npc) {
        InitFightAnis(npc->GetWeaponMode());
        SetWalkMode(0);
        StartStandAni();
    }
}

void oCAniCtrl_Human::InitAngleValuesForConsole()
{
    angle_slide1 = 57.2957764f;
    angle_slide2 = 57.2957764f;
    angle_heading = 57.2957764f;
    angle_horiz = 57.2957764f;
    angle_ground = 57.2957764f;
}

void oCAniCtrl_Human::SetScriptValues()
{
    static int gilValuesIndex = -1;
    if (gilValuesIndex < 0) {
        gilValuesIndex = parser.GetIndex("GIL_VALUES");
        parser.CreateInstance("GIL_VALUES", &TGilValues);
    }

    int guild = npc->GetTrueGuild();
    float waterDepth = TGilValues.waterDepthKnee[guild];
    if (!waterDepth && guild < 16)
        waterDepth = TGilValues.waterDepthKnee[0];
    if (waterDepth)
        config.zMV_WATER_DEPTH_KNEE = waterDepth;
    waterDepth = TGilValues.waterDepthChest[guild];
    if (!waterDepth && guild < 16)
        waterDepth = TGilValues.waterDepthChest[0];
    if (waterDepth)
        config.zMV_WATER_DEPTH_CHEST = waterDepth;

    float jumpHeight = TGilValues.jumpUpHeight[guild];
    if (!jumpHeight && guild < 16)
        jumpHeight = TGilValues.jumpUpHeight[0];
    SetJumpUpForceByHeight(jumpHeight ? jumpHeight : 200.0f);

    float swimTime = TGilValues.swimTime[guild];
    float diveTime = TGilValues.diveTime[guild];
    if (!swimTime && guild < 16)
        swimTime = TGilValues.swimTime[0];
    if (!diveTime && guild < 16)
        diveTime = TGilValues.diveTime[0];
    npc->SetSwimDiveTime(swimTime, diveTime);

    float value = TGilValues.stepHeight[guild];
    if (!value && guild < 16)
        value = TGilValues.stepHeight[0];
    if (value)
        config.zMV_STEP_HEIGHT = value;
    value = TGilValues.jumpLowHeight[guild];
    if (!value && guild < 16)
        value = TGilValues.jumpLowHeight[0];
    if (value)
        config.zMV_YMAX_JUMPLOW = value;
    value = TGilValues.jumpMidHeight[guild];
    if (!value && guild < 16)
        value = TGilValues.jumpMidHeight[0];
    if (value)
        config.zMV_YMAX_JUMPMID = value;
    forceModelHalt = TGilValues.disableAutoRoll[guild];
    value = TGilValues.maxGroundAngleWalk[guild];
    if (!value && guild < 16)
        value = TGilValues.maxGroundAngleWalk[0];
    if (value)
        config.zMV_MAX_GROUND_ANGLE_WALK = value * 0.0174532924f;
    value = TGilValues.maxGroundAngleSlide2[guild];
    if (!value && guild < 16)
        value = TGilValues.maxGroundAngleSlide2[0];
    if (value)
        config.zMV_MAX_GROUND_ANGLE_SLIDE2 = value * 0.0174532924f;
    zMV_DO_SURFACE_ALIGN = TGilValues.surfaceAlign[guild];
    defaultSurfaceAlign = TGilValues.surfaceAlign[guild];
    value = TGilValues.wallHeadingAngle[guild];
    if (!value && guild < 16)
        value = TGilValues.wallHeadingAngle[0];
    if (value)
        config.zMV_DCUL_WALL_HEADING_ANGLE = value * 0.0174532924f;
    value = TGilValues.wallHorizAngle[guild];
    if (!value && guild < 16)
        value = TGilValues.wallHorizAngle[0];
    if (value)
        config.zMV_DCUL_WALL_HORIZ_ANGLE = value * 0.0174532924f;
    value = TGilValues.groundAngle[guild];
    if (!value && guild < 16)
        value = TGilValues.groundAngle[0];
    if (value)
        config.zMV_DCUL_GROUND_ANGLE = value * 0.0174532924f;

    if (TGilValues.fallDownHeight[guild])
        npc->SetFallDownHeight(TGilValues.fallDownHeight[guild]);
    if (TGilValues.fallDownDamage[guild])
        npc->SetFallDownDamage(TGilValues.fallDownDamage[guild]);
    if (TGilValues.fightRangeBase[guild])
        npc->SetFightRangeBase(TGilValues.fightRangeBase[guild]);
    if (TGilValues.fightRangeG[guild])
        npc->SetFightRangeG(TGilValues.fightRangeG[guild]);

    npc->bloodEnabled = !TGilValues.bloodDisable[guild];
    npc->bloodDistance = TGilValues.bloodMaxDistance[guild];
    npc->bloodAmount = TGilValues.bloodAmount[guild];
    npc->bloodFlow = TGilValues.bloodFlow[guild];
    if (!npc->bloodDistance)
        npc->bloodDistance = 1000;
    if (!npc->bloodAmount)
        npc->bloodAmount = 10;
    npc->bloodEmitter = TGilValues.bloodEmitter[guild];
    if (npc->bloodEmitter.IsEmpty())
        npc->bloodEmitter = "PFX_BLOOD";
    npc->bloodTexture = TGilValues.bloodTexture[guild];
    if (npc->bloodTexture.IsEmpty())
        npc->bloodTexture = "ZBLOODSPLAT2.TGA";
    SetBloodDefaultTexture(npc->bloodTexture);
    npc->speedTurn = TGilValues.turnSpeed[guild] ? TGilValues.turnSpeed[guild] : 150.0f;
    npc->speedTurn *= 0.00100000005f;
}

void oCAniCtrl_Human::SetInterruptFollowAni(int followAni)
{
    if (!model)
        return;
    const int transitionAnis[] = {
        t_jumpstand_2_stand, t_jump_2_stand, t_hang_2_stand, t_jumpupmid_2_stand,
        t_jumpuplow_2_stand, t_stumble, t_stumbleb, t_fallen_2_stand, t_fallenb_2_stand,
        _t_walkbr_2_walk, _t_walkbl_2_walk
    };
    for (int i = 0; i < 11; ++i) {
        zCModelAniActive* activeAni = model->GetActiveAni(transitionAnis[i]);
        if (activeAni)
            activeAni->nextAni = model->GetAniFromAniID(followAni);
    }
}

static const char* GetWeaponName(int weaponMode)
{
    static const char* names[] = { "", "FIST", "1H", "2H", "BOW", "CBOW", "MAG", "MAG" };
    return names[weaponMode];
}

static zSTRING MakeAniName(const char* prefix, const char* weaponName, const char* suffix = "")
{
    zSTRING name(prefix);
    name += weaponName;
    name += suffix;
    return name;
}

static int FindAniID(zCModel* model, const zSTRING& name)
{
    return model->GetAniIDFromAniName(name);
}

void oCAniCtrl_Human::InitAllAnis()
{
    _s_defend = -1;
    for (int weaponMode = 0; weaponMode < 8; ++weaponMode) {
        const char* weapon = GetWeaponName(weaponMode);
        s_run[weaponMode] = FindAniID(model, MakeAniName("S_RUN", weapon));
        t_run_2_runl[weaponMode] = FindAniID(model, MakeAniName("T_RUN_2_RUNL_", weapon));
        t_runl_2_run[weaponMode] = FindAniID(model, MakeAniName("T_RUNL_2_RUN_", weapon));
        s_runl[weaponMode] = FindAniID(model, MakeAniName("S_RUNL_", weapon));
        t_runl_2_runr[weaponMode] = FindAniID(model, MakeAniName("T_RUNL_2_RUNR_", weapon));
        t_runr_2_runl[weaponMode] = FindAniID(model, MakeAniName("T_RUNR_2_RUNL_", weapon));
        s_runr[weaponMode] = FindAniID(model, MakeAniName("S_RUNR_", weapon));
        t_runr_2_run[weaponMode] = FindAniID(model, MakeAniName("T_RUNR_2_RUN_", weapon));
        t_run_2_runbl[weaponMode] = FindAniID(model, MakeAniName("T_RUN_2_RUNBL_", weapon));
        t_runbl_2_run[weaponMode] = FindAniID(model, MakeAniName("T_RUNBL_2_RUN_", weapon));
        s_runbl[weaponMode] = FindAniID(model, MakeAniName("S_RUNBL_", weapon));
        t_runbl_2_runbr[weaponMode] = FindAniID(model, MakeAniName("T_RUNBL_2_RUNBR_", weapon));
        t_runbr_2_runbl[weaponMode] = FindAniID(model, MakeAniName("T_RUNBR_2_RUNBL_", weapon));
        s_runbr[weaponMode] = FindAniID(model, MakeAniName("S_RUNBR_", weapon));
        t_runbr_2_run[weaponMode] = FindAniID(model, MakeAniName("T_RUNBR_2_RUN_", weapon));
        t_jumpb[weaponMode] = FindAniID(model, MakeAniName("T_JUMPB_", weapon));
        t_runturnl[weaponMode] = FindAniID(model, MakeAniName("T_RUNTURNL_", weapon));
        t_runturnr[weaponMode] = FindAniID(model, MakeAniName("T_RUNTURNR_", weapon));
        t_runstrafel[weaponMode] = FindAniID(model, MakeAniName("T_RUNSTRAFEL_", weapon));
        t_runstrafer[weaponMode] = FindAniID(model, MakeAniName("T_RUNSTRAFER_", weapon));
        s_walk[weaponMode] = FindAniID(model, MakeAniName("S_WALK", weapon));
        t_walk_2_walkl[weaponMode] = FindAniID(model, MakeAniName("T_WALK_2_WALKL_", weapon));
        t_walkl_2_walk[weaponMode] = FindAniID(model, MakeAniName("T_WALKL_2_WALK_", weapon));
        s_walkl[weaponMode] = FindAniID(model, MakeAniName("S_WALKL_", weapon));
        t_walkl_2_walkr[weaponMode] = FindAniID(model, MakeAniName("T_WALKL_2_WALKR_", weapon));
        t_walkr_2_walkl[weaponMode] = FindAniID(model, MakeAniName("T_WALKR_2_WALKL_", weapon));
        s_walkr[weaponMode] = FindAniID(model, MakeAniName("S_WALKR_", weapon));
        t_walkr_2_walk[weaponMode] = FindAniID(model, MakeAniName("T_WALKR_2_WALK_", weapon));
        t_walkturnl[weaponMode] = FindAniID(model, MakeAniName("T_WALKTURNL_", weapon));
        t_walkturnr[weaponMode] = FindAniID(model, MakeAniName("T_WALKTURNR_", weapon));
        t_walkstrafel[weaponMode] = FindAniID(model, MakeAniName("T_WALKSTRAFEL_", weapon));
        t_walkstrafer[weaponMode] = FindAniID(model, MakeAniName("T_WALKSTRAFER_", weapon));
        t_walk_2_walkbl[weaponMode] = FindAniID(model, MakeAniName("T_WALK_2_WALKBL_", weapon));
        t_walkbl_2_walk[weaponMode] = FindAniID(model, MakeAniName("T_WALKBL_2_WALK_", weapon));
        s_walkbl[weaponMode] = FindAniID(model, MakeAniName("S_WALKBL_", weapon));
        t_walkbl_2_walkbr[weaponMode] = FindAniID(model, MakeAniName("T_WALKBL_2_WALKBR_", weapon));
        t_walkbr_2_walkbl[weaponMode] = FindAniID(model, MakeAniName("T_WALKBR_2_WALKBL_", weapon));
        s_walkbr[weaponMode] = FindAniID(model, MakeAniName("S_WALKBR_", weapon));
        t_walkbr_2_walk[weaponMode] = FindAniID(model, MakeAniName("T_WALKBR_2_WALK_", weapon));
        s_sneak[weaponMode] = FindAniID(model, MakeAniName("S_SNEAK", weapon));
        t_sneak_2_sneakl[weaponMode] = FindAniID(model, MakeAniName("T_SNEAK_2_SNEAKL_", weapon));
        t_sneakl_2_sneak[weaponMode] = FindAniID(model, MakeAniName("T_SNEAKL_2_SNEAK_", weapon));
        s_sneakl[weaponMode] = FindAniID(model, MakeAniName("S_SNEAKL_", weapon));
        t_sneakl_2_sneakr[weaponMode] = FindAniID(model, MakeAniName("T_SNEAKL_2_SNEAKR_", weapon));
        t_sneakr_2_sneakl[weaponMode] = FindAniID(model, MakeAniName("T_SNEAKR_2_SNEAKL_", weapon));
        s_sneakr[weaponMode] = FindAniID(model, MakeAniName("S_SNEAKR_", weapon));
        t_sneakr_2_sneak[weaponMode] = FindAniID(model, MakeAniName("T_SNEAKR_2_SNEAK_", weapon));
        t_sneakturnl[weaponMode] = FindAniID(model, MakeAniName("T_SNEAKTURNL_", weapon));
        t_sneakturnr[weaponMode] = FindAniID(model, MakeAniName("T_SNEAKTURNR_", weapon));
        t_sneakstrafel[weaponMode] = FindAniID(model, MakeAniName("T_SNEAKSTRAFEL_", weapon));
        t_sneakstrafer[weaponMode] = FindAniID(model, MakeAniName("T_SNEAKSTRAFER_", weapon));
        t_sneak_2_sneakbl[weaponMode] = FindAniID(model, MakeAniName("T_SNEAK_2_SNEAKBL_", weapon));
        t_sneakbl_2_sneak[weaponMode] = FindAniID(model, MakeAniName("T_SNEAKBL_2_SNEAK_", weapon));
        s_sneakbl[weaponMode] = FindAniID(model, MakeAniName("S_SNEAKBL_", weapon));
        t_sneakbl_2_sneakbr[weaponMode] = FindAniID(model, MakeAniName("T_SNEAKBL_2_SNEAKBR_", weapon));
        t_sneakbr_2_sneakbl[weaponMode] = FindAniID(model, MakeAniName("T_SNEAKBR_2_SNEAKBL_", weapon));
        s_sneakbr[weaponMode] = FindAniID(model, MakeAniName("S_SNEAKBR_", weapon));
        t_sneakbr_2_sneak[weaponMode] = FindAniID(model, MakeAniName("T_SNEAKBR_2_SNEAK_", weapon));
        t_runl_2_jump[weaponMode] = FindAniID(model, MakeAniName("T_RUNL_2_JUMP_", weapon));
        t_runr_2_jump[weaponMode] = FindAniID(model, MakeAniName("T_RUNR_2_JUMP_", weapon));
        t_jump_2_runl[weaponMode] = FindAniID(model, MakeAniName("T_JUMP_2_RUNL_", weapon));
        InitFightAnis(weaponMode);
    }

    s_dead1 = FindAniID(model, "S_DEAD");
    s_dead2 = FindAniID(model, "S_DEADB");
    s_hang = FindAniID(model, "S_HANG");
    t_hang_2_stand = FindAniID(model, "T_HANG_2_STAND");
    t_jumpstand_2_stand = FindAniID(model, "T_JUMPSTAND_2_STAND");
    t_jump_2_stand = FindAniID(model, "T_JUMP_2_STAND");
    t_jumpuplow_2_stand = FindAniID(model, "T_JUMPUPLOW_2_STAND");
    t_jumpupmid_2_stand = FindAniID(model, "T_JUMPUPMID_2_STAND");
    t_stumble = FindAniID(model, "T_STUMBLE");
    t_stumbleb = FindAniID(model, "T_STUMBLEB");
    t_fallen_2_stand = FindAniID(model, "T_FALLEN_2_STAND");
    t_fallenb_2_stand = FindAniID(model, "T_FALLENB_2_STAND");
    s_swim = FindAniID(model, "S_SWIM");
    s_dive = FindAniID(model, "S_DIVE");
    s_fall = FindAniID(model, "S_FALL");
    s_fallb = FindAniID(model, "S_FALLB");
    s_fallen = FindAniID(model, "S_FALLEN");
    s_fallenb = FindAniID(model, "S_FALLENB");
    s_falldn = FindAniID(model, "S_FALLDN");
}

zCModel* zCAIPlayer::GetModel() const
{
    return model;
}

void oCAniCtrl_Human::SetWalkMode(int mode)
{
    int weaponMode = npc->GetWeaponMode();
    if (mode == 0) {
        _s_walk = s_run[weaponMode];
        _t_walk_2_walkl = t_run_2_runl[weaponMode];
        _t_walkl_2_walk = t_runl_2_run[weaponMode];
        _s_walkl = s_runl[weaponMode];
        _t_walkl_2_walkr = t_runl_2_runr[weaponMode];
        _t_walkr_2_walkl = t_runr_2_runl[weaponMode];
        _s_walkr = s_runr[weaponMode];
        _t_walkr_2_walk = t_runr_2_run[weaponMode];
        _t_walk_2_walkbl = t_run_2_runbl[weaponMode];
        _t_walkbl_2_walk = t_runbl_2_run[weaponMode];
        _s_walkbl = s_runbl[weaponMode];
        _t_walkbl_2_walkbr = t_runbl_2_runbr[weaponMode];
        _t_walkbr_2_walkbl = t_runbr_2_runbl[weaponMode];
        _s_walkbr = s_runbr[weaponMode];
        _t_walkbr_2_walk = t_runbr_2_run[weaponMode];
        _t_jumpb = t_jumpb[weaponMode];
        _t_turnl = t_runturnl[weaponMode];
        _t_turnr = t_runturnr[weaponMode];
        _t_strafel = t_runstrafel[weaponMode];
        _t_strafer = t_runstrafer[weaponMode];
        _t_runl_2_jump = t_runl_2_jump[weaponMode];
        _t_runr_2_jump = t_runr_2_jump[weaponMode];
        _t_jump_2_runl = t_jump_2_runl[weaponMode];
    } else if (mode == 1 || mode == 2) {
        int* stand = mode == 1 ? s_walk : s_sneak;
        int* toLeft = mode == 1 ? t_walk_2_walkl : t_sneak_2_sneakl;
        int* fromLeft = mode == 1 ? t_walkl_2_walk : t_sneakl_2_sneak;
        int* left = mode == 1 ? s_walkl : s_sneakl;
        int* leftToRight = mode == 1 ? t_walkl_2_walkr : t_sneakl_2_sneakr;
        int* rightToLeft = mode == 1 ? t_walkr_2_walkl : t_sneakr_2_sneakl;
        int* right = mode == 1 ? s_walkr : s_sneakr;
        int* fromRight = mode == 1 ? t_walkr_2_walk : t_sneakr_2_sneak;
        int* toBackLeft = mode == 1 ? t_walk_2_walkbl : t_sneak_2_sneakbl;
        int* fromBackLeft = mode == 1 ? t_walkbl_2_walk : t_sneakbl_2_sneak;
        int* backLeft = mode == 1 ? s_walkbl : s_sneakbl;
        int* backLeftToRight = mode == 1 ? t_walkbl_2_walkbr : t_sneakbl_2_sneakbr;
        int* backRightToLeft = mode == 1 ? t_walkbr_2_walkbl : t_sneakbr_2_sneakbl;
        int* backRight = mode == 1 ? s_walkbr : s_sneakbr;
        int* fromBackRight = mode == 1 ? t_walkbr_2_walk : t_sneakbr_2_sneak;
        _s_walk = stand[weaponMode];
        _t_walk_2_walkl = toLeft[weaponMode];
        _t_walkl_2_walk = fromLeft[weaponMode];
        _s_walkl = left[weaponMode];
        _t_walkl_2_walkr = leftToRight[weaponMode];
        _t_walkr_2_walkl = rightToLeft[weaponMode];
        _s_walkr = right[weaponMode];
        _t_walkr_2_walk = fromRight[weaponMode];
        _t_walk_2_walkbl = toBackLeft[weaponMode];
        _t_walkbl_2_walk = fromBackLeft[weaponMode];
        _s_walkbl = backLeft[weaponMode];
        _t_walkbl_2_walkbr = backLeftToRight[weaponMode];
        _t_walkbr_2_walkbl = backRightToLeft[weaponMode];
        _s_walkbr = backRight[weaponMode];
        _t_walkbr_2_walk = fromBackRight[weaponMode];
        _t_jumpb = -1;
        _t_turnl = (mode == 1 ? t_walkturnl : t_sneakturnl)[weaponMode];
        _t_turnr = (mode == 1 ? t_walkturnr : t_sneakturnr)[weaponMode];
        _t_strafel = (mode == 1 ? t_walkstrafel : t_sneakstrafel)[weaponMode];
        _t_strafer = (mode == 1 ? t_walkstrafer : t_sneakstrafer)[weaponMode];
    } else if (mode == 3) {
        _s_walk = s_walk[weaponMode];
        _t_walk_2_walkl = t_walk_2_walkwl;
        _t_walkl_2_walk = t_walkwl_2_walk;
        _s_walkl = s_walkwl;
        _t_walkl_2_walkr = t_walkwl_2_walkwr;
        _t_walkr_2_walkl = t_walkwr_2_walkwl;
        _s_walkr = s_walkwr;
        _t_walkr_2_walk = t_walkwr_2_walk;
        _t_walk_2_walkbl = t_walk_2_walkwbl;
        _t_walkbl_2_walk = t_walkwbl_2_walk;
        _s_walkbl = s_walkwbl;
        _t_walkbl_2_walkbr = t_walkwbl_2_walkwbr;
        _t_walkbr_2_walkbl = t_walkwbr_2_walkwbl;
        _s_walkbr = s_walkwbr;
        _t_walkbr_2_walk = t_walkwbr_2_walk;
        _t_jumpb = -1;
        _t_turnl = FindAniID(model, "T_WALKWTURNL");
        _t_turnr = FindAniID(model, "T_WALKWTURNR");
        _t_strafel = FindAniID(model, "T_WALKWSTRAFEL");
        _t_strafer = FindAniID(model, "T_WALKWSTRAFER");
    }

    if (mode <= 3) {
        SetInterruptFollowAni(_s_walk);
        walkmode = mode;
        if (mode <= 2) {
            lastRunWalkSneak = mode;
            npc->SetBodyState(mode == 0 ? 3 : mode);
        }
    } else if (mode == 4 || mode == 5) {
        walkmode = mode;
        t_stand_2_iaim = -1;
        npc->SetBodyState(mode == 4 ? 5 : 7);
    }
}

void oCAniCtrl_Human::InitFightAnis(int weaponMode)
{
    if (weaponMode < 1 || weaponMode > 7)
        return;
    const char* weapon = GetWeaponName(weaponMode);
    t_walk_2_aim[weaponMode] = FindAniID(model, MakeAniName("T_WALK_2_AIM_", weapon));
    t_walkl_2_aim[weaponMode] = FindAniID(model, MakeAniName("T_WALKL_2_AIM_", weapon));
    t_walkr_2_aim[weaponMode] = FindAniID(model, MakeAniName("T_WALKR_2_AIM_", weapon));
    s_aim[weaponMode] = FindAniID(model, MakeAniName("S_AIM_", weapon));
    t_aim_2_walk[weaponMode] = FindAniID(model, MakeAniName("T_AIM_2_WALK_", weapon));
    if (weaponMode <= 4) {
        t_hitl[weaponMode] = FindAniID(model, MakeAniName("T_", weapon, "ATTACKL"));
        t_hitr[weaponMode] = FindAniID(model, MakeAniName("T_", weapon, "ATTACKR"));
        t_hitf[weaponMode] = FindAniID(model, MakeAniName("S_", weapon, "ATTACK"));
        t_hitfrun[weaponMode] = FindAniID(model, MakeAniName("T_", weapon, "ATTACKMOVE"));
        t_paradeL[weaponMode] = FindAniID(model, MakeAniName("T_", weapon, "PARADES"));
    } else if (weaponMode == 5 || weaponMode == 6) {
        t_hitback[weaponMode] = FindAniID(model, MakeAniName("T_", weapon, "RELOAD"));
        t_hitf[weaponMode] = FindAniID(model, MakeAniName("T_", weapon, "AIM_2_SHOOT"));
        s_hitf[weaponMode] = FindAniID(model, MakeAniName("S_", weapon, "SHOOT"));
    }
    t_aim_2_defend[weaponMode] = FindAniID(model, MakeAniName("T_AIM_2_DEFEND_", weapon));
    s_defend[weaponMode] = FindAniID(model, MakeAniName("S_DEFEND_", weapon));
    t_defend_2_aim[weaponMode] = FindAniID(model, MakeAniName("T_DEFEND_2_AIM_", weapon));
    t_paradeM[weaponMode] = FindAniID(model, MakeAniName("T_PARADEM_", weapon));
    t_paradeS[weaponMode] = FindAniID(model, MakeAniName("T_PARADES_", weapon));
}

void oCAniCtrl_Human::SetFightAnis(int weaponMode)
{
    _t_walk_2_aim = t_walk_2_aim[weaponMode];
    _t_walkl_2_aim = t_walkl_2_aim[weaponMode];
    _t_walkr_2_aim = t_walkr_2_aim[weaponMode];
    _s_aim = s_aim[weaponMode];
    _t_aim_2_walk = t_aim_2_walk[weaponMode];
    if (weaponMode <= 4) {
        _t_hitl = t_hitl[weaponMode];
        _t_hitr = t_hitr[weaponMode];
        _t_hitf = t_hitf[weaponMode];
        _t_hitfrun = t_hitfrun[weaponMode];
        _t_paradeL = t_paradeL[weaponMode];
    } else if (weaponMode == 5 || weaponMode == 6) {
        _t_hitback = t_hitback[weaponMode];
        _t_hitf = t_hitf[weaponMode];
        _s_hitf = s_hitf[weaponMode];
    }
    int guild = npc->GetTrueGuild();
    if (weaponMode == 1)
        npc->SetFightRangeFist(TGilValues.fightRangeFist[guild]);
}

void oCAniCtrl_Human::StartAniWithOffset(int aniID, float offset)
{
}

void oCAniCtrl_Human::DoAniWithOffset()
{
}

void oCAniCtrl_Human::CorrectFightPosition()
{
    static int correctionInitialized = 0;
    static float correctionScaler = 1.0f;
    if (!correctionInitialized) {
        correctionInitialized = 1;
        zCPar_Symbol* symbol = parser.GetSymbol("NPC_COLLISION_CORRECTION_SCALER");
        if (symbol)
            symbol->GetValue(correctionScaler, 0);
    }
    if (npc->GetGuild() == 0x2f || npc->GetGuild() == 0x2d)
        correctionScaler = 0.5f;
    zCVob* enemy = npc->IsAPlayer() ? npc->GetFocusVob() : npc->enemy;
    if (!enemy || !vob)
        return;

    float distance = vob->GetDistanceToVobApprox(*enemy);
    float fightRange = npc->GetFightRange() * correctionScaler;
    if (distance <= fightRange || distance <= 0.0001f)
        return;

    zVEC3 npcPosition = vob->trafoObjToWorld.GetTranslation();
    zVEC3 enemyPosition = enemy->trafoObjToWorld.GetTranslation();
    float dirX = (enemyPosition.n[0] - npcPosition.n[0]) / distance;
    float dirZ = (enemyPosition.n[2] - npcPosition.n[2]) / distance;
    vob->Move(dirX * 0.025f * ztimer.frameTimeFloat, 0.0f,
        dirZ * 0.025f * ztimer.frameTimeFloat);
    oCCollObjectCharacter* enemyCollision = dynamic_cast<oCCollObjectCharacter*>(
        enemy->GetCollisionObject());
    if (enemyCollision) {
        enemyCollision->mustNotComeNearer = npc;
        enemyCollision->comeNearerCounter = 10;
    }
    if (distance > fightRange * 0.8f) {
        oCCollObjectCharacter* npcCollision = dynamic_cast<oCCollObjectCharacter*>(
            npc->GetCollisionObject());
        if (npcCollision) {
            npcCollision->mustNotComeNearer = enemy;
            npcCollision->comeNearerCounter = 10;
        }
    }
}

zSTRING oCAniCtrl_Human::GetWalkModeString()
{
    if (walkmode == 1)
        return "WALK";
    if (walkmode == 2)
        return "SNEAK";
    if (walkmode == 3)
        return "WALKW";
    if (walkmode == 4)
        return "SWIM";
    if (walkmode == 5)
        return "DIVE";
    return "RUN";
}

void oCAniCtrl_Human::CorrectAniStates()
{
    if (!model)
        return;
    int weaponMode = npc->GetWeaponMode();
    for (int i = 0; i < 8; ++i) {
        if (i == weaponMode || i == 2)
            continue;
        model->FadeOutAni(s_walk[i]);
        model->FadeOutAni(s_run[i]);
        model->FadeOutAni(s_sneak[i]);
    }
    for (int i = 0; i < model->numActiveAnis; ++i) {
        if (model->aniChannels[i]->protoAni->layer == 1)
            return;
    }
    SearchStandAni(0);
}

int oCAniCtrl_Human::CheckWaterLevel()
{
    if (npc->IsDead())
        return 0;
    if (waterLevel == 1) {
        if (actionMode == 5) {
            SetWalkMode(3);
            SetActionMode(4);
            return 1;
        }
        if (actionMode == 4 && walkmode != 3) {
            SetWalkMode(3);
            StartAni(_s_walk, -1);
            return 1;
        }
    } else if (waterLevel == 2 && actionMode != 5 && actionMode != 6) {
        SetActionMode(5);
        SetWalkMode(4);
        StartAni(s_swim, -1);
        return 1;
    }
    return 0;
}

void oCAniCtrl_Human::SetAlwaysWalk(int enabled)
{
    if (enabled)
        always_walk = !always_walk;
}

int oCAniCtrl_Human::IsAlwaysWalk()
{
    return always_walk;
}

int oCAniCtrl_Human::CanToggleWalkModeTo(int mode)
{
    togglewalk = -1;
    if (mode < 0 || mode > 2)
        return 0;
    if (mode == 2 && !npc->HasTalent(8, 1))
        return 0;
    nextwalkmode = mode;

    zSTRING oldMode = GetWalkModeString();
    zSTRING newMode = mode == 0 ? zSTRING("RUN") : mode == 1 ? zSTRING("WALK") : zSTRING("SNEAK");
    zSTRING transition = "T_";
    if (IsStateAniActive(_s_walkl)) {
        transition += oldMode.ToChar();
        transition += "L_2_";
        transition += newMode.ToChar();
        transition += "L";
    } else {
        transition += oldMode.ToChar();
        transition += "_2_";
        transition += newMode.ToChar();
    }
    transition += GetWeaponName(npc->GetWeaponMode());
    togglewalk = FindAniID(model, transition);
    zCModelAni* ani = model->GetAniFromAniID(togglewalk);
    return ani && ani->aniType != zMDL_ANI_TYPE_DISABLED;
}

int oCAniCtrl_Human::CanToggleWalkMode()
{
    if (walkmode == 0)
        return CanToggleWalkModeTo(always_walk ? 2 : 1);
    if (walkmode == 1)
        return CanToggleWalkModeTo(always_walk ? 2 : 0);
    if (walkmode == 2)
        return CanToggleWalkModeTo(1);
    return 0;
}

void oCAniCtrl_Human::ToggleWalkMode(int mode)
{
    model->FadeOutAnisLayerRange(20, 20);
    StartAni(togglewalk, -1);
    SetWalkMode(nextwalkmode);
}

int oCAniCtrl_Human::IsDead()
{
    if (!npc->IsDead())
        return 0;
    model = npc->GetModel();
    if (IsStateAniActive(s_fallen) || IsStateAniActive(s_fallenb)) {
        vob->SetCollDetDyn(0);
        vob->SetCollDetStat(1);
        npc->SetBodyState(23);
        zMV_DO_SURFACE_ALIGN = 1;
        model->StartAni(IsStateAniActive(s_fallen) ? s_dead1 : s_dead2, 0);
        return 1;
    }
    if (IsStateAniActive(s_dead1) || IsStateAniActive(s_dead2)
        || model->IsAniActive(model->GetAniFromAniID(s_dead1))
        || model->IsAniActive(model->GetAniFromAniID(s_dead2)))
        return 1;
    zSTRING diveTransition = "T_DIVE_2_DROWNED";
    zSTRING drowned = "S_DROWNED";
    if (model->IsAniActive(model->GetAniFromAniID(model->GetAniIDFromAniName(diveTransition)))
        || model->IsAniActive(model->GetAniFromAniID(model->GetAniIDFromAniName(drowned)))) {
        npc->SetWeaponMode2(0);
        return 1;
    }
    if (!IsStanding()) {
        npc->SetWeaponMode2(0);
        zSTRING deadAni = "T_DEADB";
        model->StartAni(deadAni, 0);
        zMV_DO_SURFACE_ALIGN = 1;
        return 1;
    }
    return 0;
}

int oCAniCtrl_Human::IsFallen()
{
    return IsStateAniActive(s_fallen) || IsStateAniActive(s_fallenb);
}

int oCAniCtrl_Human::IsAiming()
{
    if (!model)
        return 0;
    return model->IsAniActive(model->GetAniFromAniID(_s_aim)) ||
        model->IsAniActive(model->GetAniFromAniID(_t_walk_2_aim));
}

int oCAniCtrl_Human::IsStateAniActive(int aniID)
{
    return IsStateAniActive(model->GetAniFromAniID(aniID));
}

int oCAniCtrl_Human::IsStateAniActive(zCModelAni* ani)
{
    for (int i = 0; i < model->numActiveAnis; ++i) {
        zCModelAni* protoAni = model->aniChannels[i]->protoAni;
        if (protoAni->layer == 1) {
            if (protoAni != ani)
                return 0;
            return 1;
        }
    }
    return 0;
}

int oCAniCtrl_Human::IsStanding()
{
    if (!model)
        return 1;
    int weaponMode = npc->GetWeaponMode();
    if (walkmode == 0 && model->IsStateActive(model->GetAniFromAniID(s_walk[weaponMode])))
        model->StartAni(_s_walk, 0);
    return IsStateAniActive(_s_walk) || model->IsAniActive(model->GetAniFromAniID(s_swim)) ||
        model->IsAniActive(model->GetAniFromAniID(s_dive));
}

int oCAniCtrl_Human::IsJumpStanding()
{
    return model && model->IsStateActive(model->GetAniFromAniID(s_jumpstand));
}

int oCAniCtrl_Human::IsWalking()
{
    if (!model)
        return 0;
    return model->IsAniActive(model->GetAniFromAniID(_s_walkl)) ||
        model->IsAniActive(model->GetAniFromAniID(_s_walkr)) ||
        model->IsAniActive(model->GetAniFromAniID(_t_walk_2_walkl));
}

int oCAniCtrl_Human::IsRunning()
{
    if (!model)
        return 0;
    int weaponMode = npc->GetWeaponMode();
    return model->IsAniActive(model->GetAniFromAniID(s_runl[weaponMode])) ||
        model->IsAniActive(model->GetAniFromAniID(s_runr[weaponMode])) ||
        model->IsAniActive(model->GetAniFromAniID(t_run_2_runl[weaponMode])) ||
        model->IsAniActive(model->GetAniFromAniID(t_runl_2_run[weaponMode])) ||
        model->IsAniActive(model->GetAniFromAniID(t_runl_2_runr[weaponMode])) ||
        model->IsAniActive(model->GetAniFromAniID(t_runr_2_runl[weaponMode])) ||
        model->IsAniActive(model->GetAniFromAniID(t_runr_2_run[weaponMode]));
}

int oCAniCtrl_Human::IsWalkingBackward()
{
    if (!model)
        return 0;
    return model->IsAniActive(model->GetAniFromAniID(_s_walkbl)) ||
        model->IsAniActive(model->GetAniFromAniID(_s_walkbr)) ||
        model->IsAniActive(model->GetAniFromAniID(_t_walk_2_walkbl));
}

void oCAniCtrl_Human::StopTurnAnis()
{
    model->FadeOutAnisLayerRange(20, 20);
}

float oCAniCtrl_Human::Turn(float angle, int animate)
{
    autoRollDirection = -angle;
    if (autoRollDirection < -1.0f)
        autoRollDirection = -1.0f;
    else if (autoRollDirection > 1.0f)
        autoRollDirection = 1.0f;
    if (IsFallen() || IsDead())
        return angle;
    if (animate) {
        int turnAni = angle < 0.0f ? _t_turnl : _t_turnr;
        if (model->IsAniActive(model->GetAniFromAniID(_s_walk))) {
            model->StartAni(turnAni, 0);
        } else if (model->IsAniActive(model->GetAniFromAniID(s_dive))) {
            zSTRING aniName = angle < 0.0f ? "T_DIVETURNL" : "T_DIVETURNR";
            model->StartAni(model->GetAniIDFromAniName(aniName), 0);
        } else if (model->IsAniActive(model->GetAniFromAniID(s_swim))) {
            zSTRING aniName = angle < 0.0f ? "T_SWIMTURNL" : "T_SWIMTURNR";
            model->StartAni(model->GetAniIDFromAniName(aniName), 0);
        } else {
            StopTurnAnis();
        }
        float speed = npc->GetTurnSpeed() * ztimer.frameTimeFloat / ztimer.factorMotion;
        if (model->IsAniActive(model->GetAniFromAniID(_t_hitl))
            || model->IsAniActive(model->GetAniFromAniID(_t_hitr)))
            speed *= 0.5f;
        zVEC3 axis = { 0.0f, 1.0f, 0.0f };
        vob->RotateWorld(axis, autoRollDirection * speed);
    }
    return angle;
}

void oCAniCtrl_Human::DoAutoRoll()
{
    if (actionMode == 5) {
        if (autoRollPosDest != 0.0f)
            ResetAutoRoll();
    } else if (actionMode == 6) {
        autoRollPosDest = autoRollDirection;
    } else if (!walkmode && model->IsAniActive(model->GetAniFromAniID(_s_walkl))) {
        autoRollPosDest = autoRollDirection;
    } else {
        autoRollPosDest = 0.0f;
    }
}

void oCAniCtrl_Human::TurnDegrees(float degrees, int animate)
{
    if (!animate)
        StopTurnAnis();
    else if (model->IsAniActive(model->GetAniFromAniID(_s_walk)))
        model->StartAni(degrees < 0.0f ? _t_turnl : _t_turnr, 0);
    zVEC3 axis = { 0.0f, 1.0f, 0.0f };
    vob->RotateWorld(axis, degrees);
}

int oCAniCtrl_Human::WallInFront(float distance)
{
    zVEC3 direction = {
        vob->trafoObjToWorld.m[0][2] * distance,
        vob->trafoObjToWorld.m[1][2] * distance + config.zMV_MAX_GROUND_ANGLE_WALK * distance,
        vob->trafoObjToWorld.m[2][2] * distance
    };
    return world->TraceRayNearestHit(centerPos, direction, vob, 2);
}

zSTRING oCAniCtrl_Human::GetWeaponString(int weaponMode)
{
    if (weaponMode < 0 || weaponMode > 7)
        return "";
    return GetWeaponName(weaponMode);
}

zSTRING oCAniCtrl_Human::GetWeaponHitString(int weaponMode)
{
    static const char* names[] = { "", "FIST", "1HS", "2HS", "DAG", "BOW", "CBOW", "MAG" };
    if (weaponMode < 1 || weaponMode > 7)
        return "";
    return names[weaponMode];
}

int oCAniCtrl_Human::IsDefending()
{
    return model->IsStateActive(model->GetAniFromAniID(_s_defend));
}

void oCAniCtrl_Human::GetFightLimbs()
{
    hitlimb[0] = model->SearchNode("ZS_RIGHTHAND");
    hitlimb[1] = model->SearchNode("ZS_LEFTHAND");
    hitlimb[2] = model->SearchNode("ZS_RIGHTFOOT");
    hitlimb[3] = model->SearchNode("ZS_LEFTFOOT");
    comboMax = 0;
    for (int i = 0; i < 10; ++i) {
        comboInfo[i].hitAni = -1;
        comboInfo[i].hitOptFrame = 0;
        comboInfo[i].hitEndFrame = 0;
        comboInfo[i].comboStartFrame = 0;
        comboInfo[i].comboEndFrame = 0;
        comboInfo[i].comboDir = 0;
    }
    for (int i = 0; i < model->occuredAniEvents.numInArray; ++i) {
        zCModelAniEvent* event = model->occuredAniEvents.parray[i];
        if (!event)
            continue;
        if (event->tagString == "DEF_HIT_LIMB") {
            for (int j = 0; j < 4 && j < zMDL_ANIEVENT_MAXSTRING; ++j) {
                if (event->string[j].Length())
                    hitlimb[j] = model->SearchNode(event->string[j]);
            }
            continue;
        }
        if (event->tagString == "DEF_DAM_MULTIPLY") {
            npc->SetDamageMultiplier(event->value1);
            continue;
        }
        int combo = std::atoi(event->string[0].PickWord(1, zSTR_SKIP, zSTR_SKIP).ToChar());
        if (combo < 0 || combo >= 10)
            continue;
        if (combo + 1 > comboMax)
            comboMax = combo + 1;
        int value = std::atoi(event->string[0].PickWord(2, zSTR_SKIP, zSTR_SKIP).ToChar());
        if (event->tagString == "DEF_OPT_FRAME") {
            comboInfo[combo].hitOptFrame = value;
        } else if (event->tagString == "DEF_HIT_END") {
            comboInfo[combo].hitEndFrame = value;
        } else if (event->tagString == "DEF_WINDOW") {
            comboInfo[combo].comboStartFrame = value;
            comboInfo[combo].comboEndFrame =
                std::atoi(event->string[0].PickWord(3, zSTR_SKIP, zSTR_SKIP).ToChar());
        } else if (event->tagString == "DEF_PAR_FRAME") {
            paradeBeginFrame = value;
            paradeEndFrame = std::atoi(event->string[0].PickWord(3, zSTR_SKIP, zSTR_SKIP).ToChar());
        } else if (event->tagString == "DEF_DIR") {
            comboInfo[combo].comboDir = event->string[0].Length() ? event->string[0][0] : 0;
        }
    }
}

void oCAniCtrl_Human::DoSparks(zVEC3& position, const zSTRING& emitter)
{
    zCParticleFX* effect = new zCParticleFX;
    zCVob* effectVob = new zCVob;
    effectVob->SetCollDetStat(0);
    effectVob->SetCollDetDyn(0);
    effectVob->SetPositionWorld(position);
    effectVob->SetVisual(effect);
    vob->GetHomeWorld()->AddVob(effectVob);
    effect->SetAndStartEmitter(emitter, 0);
    effect->Release();
    effectVob->Release();
}

zCModelAniActive* oCAniCtrl_Human::GetLayerAni(int layer)
{
    for (int i = 0; i < model->numActiveAnis; ++i) {
        zCModelAniActive* activeAni = model->aniChannels[i];
        if (activeAni->protoAni->layer == layer)
            return activeAni;
    }
    return 0;
}

int oCAniCtrl_Human::CheckLayerAni(int layer, const zSTRING& name)
{
    return GetLayerAni(layer) ? -1 : 0;
}

int oCAniCtrl_Human::CheckLimbHit(const zSTRING& limbName, zCList<zCModelNodeInst>& nodes)
{
    for (zCList<zCModelNodeInst>* entry = nodes.next; entry; entry = entry->next) {
        if (entry->data->protoNode->nodeName == limbName)
            return 1;
    }
    return 0;
}

void oCAniCtrl_Human::ShowWeaponTrail()
{
    int weaponMode = npc->GetWeaponMode();
    if (weaponMode < 2 || weaponMode > 4)
        return;
    zCModelNodeInst* hand = model->SearchNode("ZS_RIGHTHAND");
    if (!hand || !hand->nodeVisual)
        return;
    zCOBBox3D* box = hand->nodeVisual->GetOBBox3D();
    if (!box)
        return;
    zMAT4 trafo = vob->trafoObjToWorld * model->GetTrafoNodeToModel(hand);
    zVEC3 axis = box->axis[2];
    zVEC3 start = box->center - axis * (box->extent.n[2] * 1.1f);
    zVEC3 end = box->center + axis * (box->extent.n[2] * 0.8f);
    start = trafo.Rotate(start) + trafo.GetTranslation();
    end = trafo.Rotate(end) + trafo.GetTranslation();
    AddWeaponTrailSegment(start, end);
}

void oCAniCtrl_Human::StartHitCombo(int aniID, zCVob* target)
{
    hitTarget = target;
    comboNr = 0;
    lastHitAniFrame = 0.0f;
    paradeBeginFrame = 0.0f;
    paradeEndFrame = 0.0f;
    hitAniID = aniID;
    canEnableNextCombo = 1;
    endCombo = 1;
    comboCanHit = 1;
    canDoCollisionFX = 1;
    model->StartAni(aniID, 0);
    model->AdvanceAnis();
    GetFightLimbs();
}

void oCAniCtrl_Human::SetComboHitTarget(const zCVob* target)
{
    hitTarget = const_cast<zCVob*>(target);
}

void oCAniCtrl_Human::StartHitGraphical(int aniID, zCVob* target)
{
    hitGraphical = 1;
    comboNr = 0;
    hitAniID = aniID;
    hitTarget = 0;
    lastHitAniFrame = 0.0f;
    paradeBeginFrame = 0.0f;
    paradeEndFrame = 0.0f;
    model->StartAni(aniID, 0);
    model->AdvanceAnis();
    GetFightLimbs();
    stopTurnVob = target;
    if (target) {
        float azimuth;
        float elevation;
        npc->GetAngles(target, azimuth, elevation);
        stopTurnVobSign = azimuth < 0.0f ? -1 : azimuth > 0.0f ? 1 : 0;
    } else {
        stopTurnVobSign = 0;
    }
}

int oCAniCtrl_Human::GetStopTurnVobSign()
{
    if (!stopTurnVob)
        return 0;
    float azimuth;
    float elevation;
    npc->GetAngles(stopTurnVob, azimuth, elevation);
    return azimuth < 0.0f ? -1 : azimuth > 0.0f ? 1 : 0;
}

int oCAniCtrl_Human::HitCombo(int cancelCombo)
{
    ShowWeaponTrail();
    CorrectFightPosition();
    zCModelAniActive* activeAni = model->GetActiveAni(hitAniID);
    if (!activeAni)
        return 0;
    int frame = (int)activeAni->actFrame;
    oTComboInfo& combo = comboInfo[comboNr];
    if (cancelCombo && canEnableNextCombo && frame >= combo.comboStartFrame &&
        frame <= combo.comboEndFrame) {
        canEnableNextCombo = 0;
        endCombo = 0;
        activeAni->SetActFrame((float)combo.comboEndFrame - 1.0f);
    }
    if (comboCanHit && frame >= combo.hitOptFrame) {
        comboCanHit = 0;
        if (hitTarget) {
            float distance;
            if (npc->IsInFightRange(hitTarget, distance) && npc->IsInFightFocus(hitTarget)
                && npc->IsSameHeight(hitTarget))
                CreateHit(hitTarget);
        }
    }
    if (frame >= combo.hitEndFrame || frame < lastHitAniFrame) {
        if (!endCombo) {
            comboNr++;
            if (comboNr >= comboMax)
                comboNr = 0;
            canEnableNextCombo = 1;
            endCombo = 1;
            comboCanHit = 1;
            canDoCollisionFX = 1;
        } else {
            model->StartAni(_s_walk, 0);
        }
    }
    lastHitAniFrame = (float)frame;
    return !endCombo;
}

zCVob* oCAniCtrl_Human::CheckHitTarget()
{
    for (int i = 0; i < 4; ++i) {
        if (!hitlimb[i])
            continue;
        zCList<zCVob::zTModelLimbColl> collisions;
        npc->CheckModelLimbCollision(hitlimb[i], 1, collisions);
        for (zCList<zCVob::zTModelLimbColl>* entry = collisions.next; entry; entry = entry->next) {
            zCVob* target = entry->data->hitVob;
            if (!target)
                continue;
            hitpos = entry->data->approxCollisionPos;
            hitPosUsed = 1;
            hitTarget = target;
            limbname = hitlimb[i]->protoNode->nodeName;
            float distance;
            if (npc->IsInFightRange(target, distance)) {
                CreateHit(target);
                return target;
            }
        }
    }
    return hitTarget;
}

void oCAniCtrl_Human::CreateHit(zCVob* target)
{
    oCNpc* targetNpc = dynamic_cast<oCNpc*>(target);
    if (!targetNpc)
        return;
    oCItem* weapon = npc->GetWeapon();
    oCNpc::oSDamageDescriptor damage;
    damage.pVobAttacker = vob;
    damage.pNpcAttacker = npc;
    damage.pVobHit = target;
    damage.pItemWeapon = weapon;
    damage.vecLocationHit = hitPosUsed ? hitpos : target->trafoObjToWorld.GetTranslation();
    damage.fDamageMultiplier = npc->GetDamageMultiplier();
    damage.fDamageTotal = weapon ? weapon->GetFullDamage() : npc->GetFullDamage();
    damage.fDamageTotal *= damage.fDamageMultiplier;
    if (weapon) {
        unsigned long values[8];
        weapon->GetDamages(values);
        for (int i = 0; i < 8; ++i)
            damage.aryDamage[i] = values[i];
    } else {
        for (int i = 0; i < 8; ++i)
            damage.aryDamage[i] = npc->damage[i];
    }
    npc->SetBodyState(26);
    targetNpc->OnDamage(damage);
    oCNpc::AssessFightSound_S(npc, npc->trafoObjToWorld.GetTranslation(), target);
}

int oCAniCtrl_Human::CheckMeleeWeaponHitsLevel(oCItem* weapon)
{
    if (!canDoCollisionFX || (!IsInPreHit() && !IsInCombo()))
        return 0;
    zCModelAniActive* activeAni = model->GetActiveAni(hitAniID);
    if (activeAni) {
        int frame = static_cast<int>(activeAni->actFrame);
        if (frame >= comboInfo[comboNr].comboStartFrame
            && frame <= comboInfo[comboNr].comboEndFrame)
            return 0;
    }
    for (int i = 0; i < 4; ++i) {
        if (!hitlimb[i])
            continue;
        zCList<zCVob::zTModelLimbColl> collisions;
        npc->CheckModelLimbCollisionLevel(hitlimb[i], 1, collisions);
        if (!collisions.next)
            continue;
        hitpos = collisions.next->data->approxCollisionPos;
        canDoCollisionFX = 0;
        zSTRING emitter = "CPFX_IAI_FIST";
        DoSparks(hitpos, emitter);
        return 1;
    }
    return 0;
}

void oCAniCtrl_Human::HitInterrupt()
{
    endCombo = 1;
    canEnableNextCombo = 0;
    comboCanHit = 0;
    hitPosUsed = 0;
    hitGraphical = 0;
    canDoCollisionFX = 0;
    lastHitAniFrame = 0.0f;
    model->StopAni(hitAniID);
    hitAniID = -1;
}

int oCAniCtrl_Human::HitGraphical()
{
    ShowWeaponTrail();
    CorrectFightPosition();
    zCModelAniActive* activeAni = model->GetActiveAni(hitAniID);
    if (!activeAni)
        return 0;
    if (stopTurnVob && GetStopTurnVobSign() != stopTurnVobSign) {
        model->StopAni(activeAni);
        model->StartAni(_s_walk, 0);
        vob->SetHeadingYWorld(stopTurnVob);
        return 1;
    }
    if (model->GetProgressPercent(hitAniID) < 0.300000012f)
        return 0;
    return CheckHitTarget() != 0;
}

int oCAniCtrl_Human::IsInPreHit()
{
    zCModelAniActive* activeAni = model->GetActiveAni(hitAniID);
    if (!activeAni || hitGraphical)
        return 0;
    return (int)activeAni->actFrame < comboInfo[comboNr].comboStartFrame;
}

int oCAniCtrl_Human::IsInPostHit()
{
    zCModelAniActive* activeAni = model->GetActiveAni(hitAniID);
    if (!activeAni)
        return 1;
    int frame = (int)activeAni->actFrame;
    return hitGraphical || frame < comboInfo[comboNr].comboStartFrame ||
        frame > comboInfo[comboNr].comboEndFrame;
}

int oCAniCtrl_Human::IsInCombo()
{
    zCModelAniActive* activeAni = model->GetActiveAni(hitAniID);
    if (!activeAni)
        return 0;
    int frame = (int)activeAni->actFrame;
    return frame >= comboInfo[comboNr].comboStartFrame && frame <= comboInfo[comboNr].comboEndFrame;
}

void oCAniCtrl_Human::FirstPersonDrawWeapon()
{
}

zSTRING oCAniCtrl_Human::GetHitDirection()
{
    return "0";
}

zCModelAniActive* oCAniCtrl_Human::IsParadeRunning()
{
    for (int i = 0; i < model->numActiveAnis; ++i) {
        zCModelAniActive* activeAni = model->aniChannels[i];
        if (activeAni && activeAni->protoAni->aniName.Search("PARADE") > 0)
            return activeAni;
    }
    return 0;
}

int oCAniCtrl_Human::CanParade(oCNpc* attacker)
{
    if (!attacker || !IsParadeRunning())
        return 0;
    float azimuth;
    float elevation;
    npc->GetAngles(attacker, azimuth, elevation);
    if (azimuth < 0.0f)
        azimuth = -azimuth;
    return azimuth <= 90.0f;
}

void oCAniCtrl_Human::StartParadeEffects(oCNpc* attacker)
{
    zCModelNodeInst* hand = model->SearchNode("ZS_RIGHTHAND");
    if (!hand)
        return;
    zVEC3 hitPosition = model->GetNodePositionWorld(hand);
    zCSoundManager::zTSndManMedium medium;
    oTSndMaterial material;
    npc->GetSoundMaterial_MH_HM(medium, material);
    static const char* materialNames[] = { "WOOD", "STONE", "METAL", "LEATHER", "CLAY", "GLAS" };
    zSTRING emitter = zSTRING("CPFX_IAI_") + materialNames[material];
    DoSparks(hitPosition, emitter);
    zsndMan->StartAttackSound(npc, medium, medium, material, material);
}

void oCAniCtrl_Human::StrafeLeft()
{
    if (actionMode != 5) {
        model->FadeOutAnisLayerRange(20, 20);
        model->StartAni(_t_strafel, 0);
        npc->SetBodyState(1);
    }
}

void oCAniCtrl_Human::StrafeRight()
{
    if (actionMode != 5) {
        model->FadeOutAnisLayerRange(20, 20);
        model->StartAni(_t_strafer, 0);
        npc->SetBodyState(1);
    }
}

int oCAniCtrl_Human::GetFootPos()
{
    if (model->IsAniActive(model->GetAniFromAniID(_s_walkl))) {
        float progress = model->GetProgressPercent(_s_walkl);
        if (npc->GetWeaponMode())
            return progress >= 0.75f || progress <= 0.25f ? 2 : 1;
        if (progress >= 0.850000024f || progress <= 0.150000006f)
            return 1;
        if (progress >= 0.349999994f && progress <= 0.649999976f)
            return 2;
    } else if (model->IsAniActive(model->GetAniFromAniID(_s_walkbl))) {
        float progress = model->GetProgressPercent(_s_walkbl);
        if (progress >= 0.899999976f || progress <= 0.100000001f)
            return 1;
        if (progress >= 0.400000006f && progress <= 0.600000024f)
            return 2;
    }
    return 0;
}

void oCAniCtrl_Human::PC_GoForward()
{
    if ((!IsStanding() || CheckEnoughSpaceMoveForward(0)) && !CheckForceModelHalt(0)) {
        _Forward();
        return;
    }
    _Stand();
}

void oCAniCtrl_Human::PC_GoBackward()
{
    if (CheckEnoughSpaceMoveBackward(0.0f) && !CheckForceModelHalt(0)) {
        _Backward();
        return;
    }
    _Stand();
}

void oCAniCtrl_Human::PC_JumpForward()
{
    if (!model->IsAniActive(model->GetAniFromAniID(_s_walkl)) || walkmode) {
        if (model->IsAniActive(model->GetAniFromAniID(_s_walk))) {
            JumpForward();
            npc->SetBodyState(8);
            do_jump = 0;
        }
        return;
    }
    if (WallInFront(400.0f)) {
        do_jump = 0;
        return;
    }
    int footPos = GetFootPos();
    int jumpAni = footPos == 1 ? _t_runl_2_jump : footPos == 2 ? _t_runr_2_jump : -1;
    if (jumpAni != -1) {
        model->StartAni(jumpAni, 0);
        npc->SetBodyState(8);
        do_jump = 0;
    }
}

int oCAniCtrl_Human::CanJump(int jumpType)
{
    if (jumpType == 0)
        return _t_stand_2_jump != -1;
    if (jumpType == 1)
        return t_stand_2_jumpuplow != -1;
    if (jumpType == 2)
        return t_stand_2_jumpupmid != -1;
    if (jumpType == 3)
        return _t_stand_2_jumpup != -1;
    return 0;
}

int oCAniCtrl_Human::CanJumpLedge()
{
    if (model->IsStateActive(model->GetAniFromAniID(_s_jumpup)) ||
        model->IsAniActive(model->GetAniFromAniID(_t_stand_2_jumpup)) ||
        model->IsAniActive(model->GetAniFromAniID(s_jumpupmid)) ||
        model->IsAniActive(model->GetAniFromAniID(s_jumpuplow)))
        return 0;
    zVEC3 ledgePoint;
    DetectClimbUpLedge(ledgePoint, 1);
    if (!GetFoundLedge())
        return 0;
    float height = ledgePoint.n[1] - feetY;
    if (height > config.zMV_YMAX_JUMPMID)
        return 3;
    if (height > config.zMV_YMAX_JUMPLOW)
        return 2;
    if (height > config.zMV_STEP_HEIGHT)
        return 1;
    return 0;
}

int oCAniCtrl_Human::JumpForward()
{
    CanJumpLedge();
    vob->SetCollDetStat(1);
    if (!GetFoundLedge()) {
        if (WallInFront(150.0f))
            return 0;
        model->StartAni(_t_stand_2_jump, 0);
        npc->SetBodyState(8);
        return 4;
    }
    float height = GetLedgeInfo()->point.n[1] - feetY;
    int jumpAni = _t_stand_2_jumpup;
    int result = 3;
    if (height <= config.zMV_STEP_HEIGHT) {
        jumpAni = _t_stand_2_jump;
        result = 4;
        ClearFoundLedge();
    } else if (height <= config.zMV_YMAX_JUMPLOW) {
        jumpAni = t_stand_2_jumpuplow;
        result = 1;
    } else if (height <= config.zMV_YMAX_JUMPMID) {
        jumpAni = t_stand_2_jumpupmid;
        result = 2;
    }
    model->StartAni(jumpAni, 0);
    npc->SetBodyState(8);
    return result;
}

int oCAniCtrl_Human::Swim_CanClimbLedge()
{
    zVEC3 ledgePoint;
    if (!DetectClimbUpLedge(ledgePoint, 0))
        return 0;
    float height = ledgePoint.n[1] - headY;
    if (height < 0.0f)
        height = -height;
    return height <= config.zMV_YMAX_SWIM_CLIMB_OUT;
}

void oCAniCtrl_Human::Swim_ClimbLedge()
{
    zVEC3 ledgePoint;
    if (DetectClimbUpLedge(ledgePoint, 0)) {
        AlignModelToLedge("T_SWIM_2_HANG");
        walkmode = 0;
        SetWalkMode(0);
        SetActionMode(0);
    }
}

int oCAniCtrl_Human::CanWalkOnPoly(const zVEC3& normal)
{
    float y = normal.n[1];
    if (y < -1.0f)
        y = -1.0f;
    else if (y > 1.0f)
        y = 1.0f;
    return acos(y) <= config.zMV_MAX_GROUND_ANGLE_WALK;
}

int oCAniCtrl_Human::DrawWeapon1(int weaponMode, int direction, int remove)
{
    if (!model->IsAniActive(model->GetAniFromAniID(_s_walk)) && npc->GetTrueGuild() != 33)
        return IsStateAniActive(model->GetAniFromAniID(s_weapon[wmode_selected]));

    int nextWeapon = npc->GetNextWeaponMode(weaponMode > 0 ? weaponMode - 1 : 0, direction, remove);
    zSTRING weaponName = GetWeaponString(nextWeapon);
    zCModelAni* ani = model->modelProtoList.parray[0]->SearchAni("T_RUN_2_" + weaponName);
    int aniID = ani ? ani->aniID : -1;

    if (!npc->GetWeaponMode() && nextWeapon >= 7) {
        oCMsgMagic* message = new oCMsgMagic(0, nextWeapon, 0);
        message->activeSpell = nextWeapon;
        npc->GetEM(0)->OnMessage(message, npc);
    }
    if (aniID != -1) {
        model->StartAni(aniID, 0);
        SetActionMode(7);
        wmode = 2;
        wmode_last = 0;
        wmode_selected = nextWeapon;
        changeweapon = 1;
    }
    return IsStateAniActive(model->GetAniFromAniID(s_weapon[wmode_selected]));
}

int oCAniCtrl_Human::ChooseNextWeapon2(int remove)
{
    int oldWeapon = wmode_selected;
    if (!model->IsAniActive(model->GetAniFromAniID(s_weapon[oldWeapon])))
        return 0;

    int nextWeapon = npc->GetNextWeaponMode(oldWeapon, remove, 0);
    zSTRING aniName = "T_" + GetWeaponString(oldWeapon) + "_2_" + GetWeaponString(nextWeapon);
    zCModelAni* ani = model->modelProtoList.parray[0]->SearchAni(aniName);
    int aniID = ani ? ani->aniID : -1;
    if (aniID == -1)
        return 1;

    model->StartAni(aniID, 0);
    changeweapon = 1;
    wmode = 2;
    wmode_last = oldWeapon;
    wmode_selected = nextWeapon;
    if (nextWeapon >= 7) {
        oCMsgMagic* message = new oCMsgMagic(0, nextWeapon, 1);
        message->activeSpell = nextWeapon;
        npc->GetEM(0)->OnMessage(message, npc);
    }
    if (oldWeapon >= 7) {
        oCMsgMagic* message = new oCMsgMagic(1, oldWeapon, 1);
        npc->GetEM(0)->OnMessage(message, npc);
    }
    return 1;
}

int oCAniCtrl_Human::DrawWeapon2()
{
    int oldWeapon = wmode_selected;
    if (IsStateAniActive(model->GetAniFromAniID(s_weapon[oldWeapon]))) {
        zSTRING weaponName = GetWeaponString(oldWeapon);
        zSTRING aniName = "T_" + weaponName + "_2_" + weaponName + "_2_STAND";
        zCModelAni* ani = model->modelProtoList.parray[0]->SearchAni(aniName);
        int aniID = ani ? ani->aniID : -1;
        if (aniID == -1)
            return 1;

        if (aniID) {
            zSTRING walkSuffix = walkmode == 1 ? "WALK" : walkmode == 2 ? "SNEAK" : "RUN";
            zCModelAni* standAni = model->modelProtoList.parray[0]->SearchAni("S_" + aniName + walkSuffix);
            int standAniID = standAni ? standAni->aniID : -1;
            zCModelAniActive* activeAni = model->GetActiveAni(aniID);
            if (activeAni)
                activeAni->nextAni = model->GetAniFromAniID(standAniID);
            model->StartAni(aniID, 0);
            SetActionMode(0);
            wmode = 3;
            wmode_last = wmode_selected;
        }
    }
    return IsStateAniActive(model->GetAniFromAniID(_s_walk));
}

int oCAniCtrl_Human::RemoveWeapon1()
{
    if (model->IsAniActive(model->GetAniFromAniID(_s_walk))) {
        zSTRING weaponName = GetWeaponString(npc->GetWeaponMode());
        zCModelAni* ani = model->modelProtoList.parray[0]->SearchAni(
            "T_" + weaponName + "RUN_2_" + weaponName);
        int aniID = ani ? ani->aniID : -1;
        if (aniID == -1)
            return 1;
        if (aniID) {
            model->StartAni(aniID, 0);
            changeweapon = 0;
            wmode = 2;
            SetActionMode(7);
            wmode_last = npc->GetWeaponMode();
        }
    }
    return model->IsAniActive(model->GetAniFromAniID(s_weapon[wmode_last]));
}

int oCAniCtrl_Human::RemoveWeapon2()
{
    int oldWeapon = wmode_last;
    if (IsStateAniActive(model->GetAniFromAniID(s_weapon[oldWeapon]))) {
        zSTRING aniName = "T_" + GetWeaponString(oldWeapon) + "_2_RUN";
        zCModelAni* ani = model->modelProtoList.parray[0]->SearchAni(aniName);
        int aniID = ani ? ani->aniID : -1;
        changeweapon = 0;
        zSTRING walkSuffix = walkmode == 1 ? "WALK" : walkmode == 2 ? "SNEAK" : "RUN";
        zCModelAni* standAni = model->modelProtoList.parray[0]->SearchAni("S_" + aniName + walkSuffix);
        int standAniID = standAni ? standAni->aniID : -1;
        StartAni(aniID, standAniID);
        SetActionMode(0);
        wmode = 1;
        wmode_selected = 0;
    }
    return IsStateAniActive(model->GetAniFromAniID(_s_walk));
}

void oCAniCtrl_Human::StartUp(zCVob* target)
{
    zCAIPlayer::Begin(target);
}

void oCAniCtrl_Human::LerpFeetToTarget(int startAniID, int targetAniID, const zVEC3& target)
{
    if (vob->showVisual & 8)
        SetPhysicsEnabled(0);

    zCModelAni* startAni = model->GetAniFromAniID(startAniID);
    zCModelAni* targetAni = model->GetAniFromAniID(targetAniID);
    if (!startAni)
        startAni = targetAni;
    if (!targetAni)
        return;

    float progress = model->GetProgressPercent(targetAniID);
    zVEC3 translation = model->GetAniTransLerp(targetAni, progress, 0);
    zVEC3 movement = {
        target.n[0] - centerPos.n[0] - translation.n[0],
        target.n[1] - centerPos.n[1] - translation.n[1],
        target.n[2] - centerPos.n[2] - translation.n[2]
    };
    float duration = targetAni->numFrames / targetAni->fpsRate * 1000.0f * (1.0f - progress);
    if (startAni != targetAni)
        duration += startAni->numFrames / startAni->fpsRate * 1000.0f;

    if (duration <= ztimer.frameTimeFloat) {
        vob->Move(movement.n[0], movement.n[1], movement.n[2]);
        if (startAni == targetAni) {
            zMV_DO_SURFACE_ALIGN ^= 1;
            ClearFoundLedge();
        }
    } else {
        float scale = ztimer.frameTimeFloat / duration;
        vob->Move(movement.n[0] * scale, movement.n[1] * scale, movement.n[2] * scale);
    }
    if (GetFoundLedge())
        vob->SetHeadingAtWorld(GetLedgeInfo()->normal);
    vob->ResetXZRotationsWorld();
}

void oCAniCtrl_Human::RelaxHandToTarget(const zVEC3& target, const zVEC3& handTarget,
    int applyPhysics, float response)
{
    zMV_DO_SURFACE_ALIGN &= ~1;
    zVEC3 handPosition = GetHandPositionWorld();
    zVEC3 direction = {
        target.n[0] - handPosition.n[0],
        target.n[1] - handPosition.n[1],
        target.n[2] - handPosition.n[2]
    };
    float distance = sqrt(direction.n[0] * direction.n[0] + direction.n[1] * direction.n[1] +
        direction.n[2] * direction.n[2]);
    if (distance > GetJumpUpHeight() * 1.5f)
        model->StartAni(_t_jumpup_2_falldn, 0);

    if (distance > 0.0f) {
        zVEC3 at = {
            vob->trafoObjToWorld.m[0][2],
            vob->trafoObjToWorld.m[1][2],
            vob->trafoObjToWorld.m[2][2]
        };
        zVEC3 axis = {
            (direction.n[1] * at.n[2] - direction.n[2] * at.n[1]) * 0.3f / distance,
            (direction.n[2] * at.n[0] - direction.n[0] * at.n[2]) * 0.3f / distance,
            (direction.n[0] * at.n[1] - direction.n[1] * at.n[0]) * 0.3f / distance
        };
        float angle = distance * 57.2957764f * 0.5f * response;
        vob->RotateWorld(axis, angle);
    }

    if (applyPhysics) {
        vob->EndMovement(1);
        vob->BeginMovement();
        handPosition = GetHandPositionWorld();
        if (!(vob->showVisual & 8))
            SetPhysicsEnabled(1);

        zCRigidBody* body = vob->GetRigidBody();
        zVEC3 velocity = vob->GetVelocity();
        float blend = 1.0f / (response + 1.0f);
        zVEC3 force = {
            (handTarget.n[0] - handPosition.n[0] - velocity.n[0]) * blend,
            (handTarget.n[1] - handPosition.n[1] - velocity.n[1]) * blend,
            (handTarget.n[2] - handPosition.n[2] - velocity.n[2]) * blend
        };
        body->SetVelocity(force);
        vob->Move(force.n[0], force.n[1], force.n[2]);
        zVEC3 massForce = {
            force.n[0] * body->mass,
            force.n[1] * body->mass,
            force.n[2] * body->mass
        };
        body->ApplyForce(massForce, handPosition);
    }
}

void oCAniCtrl_Human::CheckSpecialStates()
{
    if (model->IsStateActive(model->GetAniFromAniID(_s_jumpup))) {
        if (!(vob->showVisual & 8)) {
            SetPhysicsEnabled(1);
            zMV_DO_SURFACE_ALIGN &= ~1;
            float force = config.zMV_FORCE_JUMP_UP;
            if (GetFoundLedge()) {
                float height = GetLedgeInfo()->point.n[1] - centerPos.n[1];
                float ledgeForce = vob->GetRigidBody()->mass * sqrt(fabs(height * 1962.0f));
                if (ledgeForce > force)
                    force = ledgeForce;
                vob->SetHeadingAtWorld(GetLedgeInfo()->normal);
            }
            zVEC3 impulse = { 0.0f, force, 0.0f };
            vob->GetRigidBody()->ApplyImpulseCM(impulse);
        } else {
            if (velocity.n[1] < 0.0f)
                model->StartAni(_t_jumpup_2_falldn, 0);
            if (GetFoundLedge()) {
                RelaxHandToTarget(GetLedgeInfo()->point, GetLedgeInfo()->normal, 0, 0.5f);
                if (headY >= GetLedgeInfo()->point.n[1]) {
                    model->StartAni("T_JUMPUP_2_HANG", 2);
                    zVEC3 stop = { 0.0f, 0.0f, 0.0f };
                    vob->GetRigidBody()->SetVelocity(stop);
                    SetPhysicsEnabled(1);
                }
            }
        }
    }

    if (GetFoundLedge() && (model->IsAniActive(model->GetAniFromAniID(s_hang)) ||
        model->IsAniActive(model->GetAniFromAniID(model->GetAniIDFromAniName("T_JUMPUP_2_HANG"))))) {
        RelaxHandToTarget(GetLedgeInfo()->point, GetLedgeInfo()->normal, 1, 4.0f);
    }
    if (GetFoundLedge() && model->IsAniActive(model->GetAniFromAniID(t_hang_2_stand))) {
        zTLedgeInfo* ledge = GetLedgeInfo();
        zVEC3 target = {
            ledge->point.n[0] + ledge->cont.n[0] * ledge->maxMoveForward,
            ledge->point.n[1] + ledge->cont.n[1] * ledge->maxMoveForward,
            ledge->point.n[2] + ledge->cont.n[2] * ledge->maxMoveForward
        };
        LerpFeetToTarget(t_hang_2_stand, t_hang_2_stand, target);
    }

    if (model->IsAniActive(model->GetAniFromAniID(t_stand_2_jumpupmid)))
        SetCDStat(0);
    if (model->IsAniActive(model->GetAniFromAniID(t_stand_2_jumpuplow)))
        SetCDStat(0);
    if (GetFoundLedge()) {
        if (model->IsAniActive(model->GetAniFromAniID(s_jumpupmid)))
            LerpFeetToTarget(s_jumpupmid, t_jumpupmid_2_stand, GetLedgeInfo()->point);
        if (model->IsAniActive(model->GetAniFromAniID(s_jumpuplow)))
            LerpFeetToTarget(s_jumpuplow, t_jumpuplow_2_stand, GetLedgeInfo()->point);
    }

    int finishAni = model->IsStateActive(model->GetAniFromAniID(t_jumpupmid_2_stand)) ?
        t_jumpupmid_2_stand : t_jumpuplow_2_stand;
    zCModelAniActive* activeAni = model->GetActiveAni(finishAni);
    if (activeAni)
        activeAni->nextAni = model->GetAniFromAniID(_s_walk);

    if (!model->IsAniActive(model->GetAniFromAniID(_s_jump)))
        CheckFallStates();
    else if (fallDownDistanceY > 350.0f)
        model->StartAni(s_falldn, 0);

    int bodyState = npc->GetBodyState();
    model->m_bSmoothRootNode = bodyState != 14 && bodyState != 15 && bodyState != 16;
}

void oCAniCtrl_Human::CheckJump()
{
    if (!model->IsAniActive(model->GetAniFromAniID(_s_jump)))
        return;
    if (waterLevel == 2) {
        StartPhysicsWithVel();
        model->StartAni("T_JUMP_2_FALLDN", 0);
        return;
    }
    if (aboveFloor > 0.0f) {
        LandAndStartAni(model->GetAniFromAniID(t_jump_2_stand));
        zCModelAniActive* activeAni = model->GetActiveAni(t_jump_2_stand);
        if (activeAni)
            activeAni->nextAni = model->GetAniFromAniID(_s_walk);
    } else if (!(vob->showVisual & 8)) {
        vob->SetPhysicsEnabled(1);
        vob->GetRigidBody()->SetVelocity(velocity);
    }
}

void oCAniCtrl_Human::StartFallDownAni()
{
    if (npc->IsUnconscious())
        return;
    static const char* protectedAnis[] = {
        "T_FALLDN_2_STAND", "T_DEAD", "T_DEADB", "S_DEAD", "S_DEADB"
    };
    for (int i = 0; i < 5; ++i) {
        int aniID = model->GetAniIDFromAniName(protectedAnis[i]);
        if (model->IsAniActive(model->GetAniFromAniID(aniID)))
            return;
    }
    model->StartAni("S_FALLDN", 0);
}

void oCAniCtrl_Human::HackNPCToSwim()
{
    if (model->IsAniActive(model->GetAniFromAniID(model->GetAniIDFromAniName("S_RUN"))) ||
        model->IsAniActive(model->GetAniFromAniID(model->GetAniIDFromAniName("S_RUNL"))) ||
        model->IsAniActive(model->GetAniFromAniID(model->GetAniIDFromAniName("S_WALKL")))) {
        zVEC3 position = {
            npc->trafoObjToWorld.m[0][3],
            npc->trafoObjToWorld.m[1][3] + 50.0f,
            npc->trafoObjToWorld.m[2][3]
        };
        npc->SetPositionWorld(position);
    }

    zVEC3 start = {
        npc->trafoObjToWorld.m[0][3],
        npc->trafoObjToWorld.m[1][3],
        npc->trafoObjToWorld.m[2][3]
    };
    zVEC3 direction = { 0.0f, -100000.0f, 0.0f };
    zCWorld* homeWorld = npc->GetHomeWorld();
    if (homeWorld->TraceRayNearestHit(start, direction, npc, 0xA21)) {
        int oldShowVisual = npc->showVisual;
        npc->SetCollDetStat(0);
        npc->SetCollDetDyn(0);
        zVEC3 position = homeWorld->traceRayReport.foundIntersection;
        position.n[1] += 5.0f;
        npc->SetPositionWorld(position);
        model->StopAni(s_fall);
        model->StopAni(s_fallb);
        model->StopAni(s_fallen);
        model->StopAni(s_fallenb);
        model->StopAni(s_falldn);
        model->StartAni(t_divef_2_swim, 0);
        npc->SetCollDetDyn(oldShowVisual >> 7);
        npc->SetCollDetStat((oldShowVisual >> 6) & 1);
        SetWalkMode(4);
        SetActionMode(5);
    }
}

void oCAniCtrl_Human::CheckFallStates()
{
    if (waterLevel == 2 && actionMode != 5 && actionMode != 6) {
        npc->SetPhysicsEnabled(1);
        zCRigidBody* body = npc->GetRigidBody();
        if (body)
            body->gravityOn = 1;
        StartFallDownAni();
    }

    if (model->IsStateActive(model->GetAniFromAniID(s_falldn))) {
        npc->SetBodyState(10);
        if (waterLevel == 2) {
            SetPhysicsEnabled(0);
            model->StartAni("T_FALLDN_2_DIVE", 0);
            SetWalkMode(5);
            SetActionMode(6);
            return;
        }
        if (aboveFloor < 10.0f) {
            zCModelAni* landAni = model->modelProtoList.parray[0]->SearchAni("T_FALLDN_2_STAND");
            LandAndStartAni(landAni);
            zCModelAniActive* activeAni = model->GetActiveAni(landAni);
            if (activeAni)
                activeAni->nextAni = model->GetAniFromAniID(_s_walk);
            return;
        }
        if (fallDownDistanceY > 200.0f) {
            zVEC3 start = {
                npc->trafoObjToWorld.m[0][3],
                feetY,
                npc->trafoObjToWorld.m[2][3]
            };
            zVEC3 direction = { 0.0f, -(npc->GetFallDownHeight() - 200.0f), 0.0f };
            if (!world->TraceRayNearestHit(start, direction, npc, 2)) {
                float forwardVelocity =
                    vob->trafoObjToWorld.m[0][2] * velocity.n[0] +
                    vob->trafoObjToWorld.m[1][2] * velocity.n[1] +
                    vob->trafoObjToWorld.m[2][2] * velocity.n[2];
                model->StartAni(forwardVelocity < 0.0f ? "T_FALLDN_2_FALLB" : "T_FALLDN_2_FALL", 0);
            }
        }
        return;
    }

    int falling = model->IsStateActive(model->GetAniFromAniID(s_fall));
    int fallingBack = model->IsStateActive(model->GetAniFromAniID(s_fallb));
    if (!falling && !fallingBack)
        return;
    if (waterLevel == 2) {
        SetPhysicsEnabled(0);
        model->StartAni(falling ? "T_FALL_2_DIVE" : "T_FALLB_2_DIVE", 0);
        SetWalkMode(5);
        SetActionMode(6);
        return;
    }
    if (aboveFloor < 10.0f) {
        model->StartAni(falling ? "T_FALL_2_FALLEN" : "T_FALLB_2_FALLENB", 0);
        npc->SetBodyState(12);
        npc->CreateFallDamage(fallDownDistanceY);
        npc->SetCollDetStat(1);
        npc->SetCollDetDyn(1);
    }
}

float oCAniCtrl_Human::CombineAniLerp(float current, float target, float delta, float maxTime)
{
    float step = delta / maxTime;
    if (target < current) {
        current -= step;
        if (current < target)
            current = target;
    } else {
        current += step;
        if (current > target)
            current = target;
    }
    return current;
}

int oCAniCtrl_Human::InterpolateCombineAni(float targetX, float targetY, int aniID)
{
    float currentX = 0.5f;
    float currentY = 0.5f;
    if (model->IsAniActive(model->GetAniFromAniID(aniID)))
        model->GetCombineAniXY(aniID, currentX, currentY);
    if (currentX == targetX && currentY == targetY)
        return 1;

    float step = ztimer.frameTimeFloat / 200.0f;
    currentX = CombineAniLerp(currentX, targetX, step, 1.0f);
    currentY = CombineAniLerp(currentY, targetY, step, 1.0f);
    model->StartAni(aniID, 0);
    model->SetCombineAniXY(aniID, currentX, currentY);
    if (currentX == 0.5f && currentY == 0.5f)
        model->FadeOutAni(aniID);
    return 0;
}

void oCAniCtrl_Human::LookAtTarget()
{
    InterpolateCombineAni(lookTargetx, lookTargety, s_look);
}

static void GetCombineCoordinates(oCNpc* npc, const zVEC3& position, float maxAzimuth,
    float maxElevation, float& targetX, float& targetY)
{
    float azimuth;
    float elevation;
    npc->GetAngles(const_cast<zVEC3&>(position), azimuth, elevation);
    targetX = fabs(azimuth) > maxAzimuth ? 0.5f : azimuth / maxAzimuth + 0.5f;
    targetY = fabs(elevation) > maxElevation ? 0.5f : 1.0f - (elevation / maxElevation + 0.5f);
    if (targetX < 0.0f)
        targetX = 0.0f;
    else if (targetX > 1.0f)
        targetX = 1.0f;
    if (targetY < 0.0f)
        targetY = 0.0f;
    else if (targetY > 1.0f)
        targetY = 1.0f;
}

void oCAniCtrl_Human::SetLookAtTarget(float targetX, float targetY)
{
    lookTargetx = targetX;
    lookTargety = targetY;
    if (targetVob) {
        targetVob->Release();
        targetVob = 0;
    }
}

void oCAniCtrl_Human::SetLookAtTarget(zVEC3& position)
{
    GetCombineCoordinates(npc, position, 180.0f, 120.0f, lookTargetx, lookTargety);
    if (targetVob) {
        targetVob->Release();
        targetVob = 0;
    }
}

void oCAniCtrl_Human::SetLookAtTarget(zCVob* target)
{
    if (targetVob) {
        targetVob->Release();
        targetVob = 0;
    }
    if (target) {
        zVEC3 position = {
            (target->bbox3D.mins.n[0] + target->bbox3D.maxs.n[0]) * 0.5f,
            (target->bbox3D.mins.n[1] + target->bbox3D.maxs.n[1]) * 0.5f,
            (target->bbox3D.mins.n[2] + target->bbox3D.maxs.n[2]) * 0.5f
        };
        GetCombineCoordinates(npc, position, 180.0f, 120.0f, lookTargetx, lookTargety);
        targetVob = target;
        targetVob->AddRef();
    }
}

void oCAniCtrl_Human::StopLookAtTarget()
{
    lookTargetx = 0.5f;
    lookTargety = 0.5f;
    if (targetVob) {
        targetVob->Release();
        targetVob = 0;
    }
    zCModelAni* quickLook = model->modelProtoList.parray[0]->SearchAni("T_QLOOK");
    zCModelAni* turnLook = model->modelProtoList.parray[0]->SearchAni("S_TLOOK");
    model->FadeOutAni(quickLook ? quickLook->aniID : -1);
    model->FadeOutAni(turnLook ? turnLook->aniID : -1);
}

void oCAniCtrl_Human::StartCombineAni(zVEC3& position, int aniID, float maxAzimuth,
    float maxElevation)
{
    float targetX;
    float targetY;
    GetCombineCoordinates(npc, position, maxAzimuth, maxElevation, targetX, targetY);
    InterpolateCombineAni(targetX, targetY, aniID);
}

void oCAniCtrl_Human::StartCombineAni(zCVob* target, int aniID, float maxAzimuth,
    float maxElevation)
{
    if (target) {
        zVEC3 position = {
            target->trafoObjToWorld.m[0][3],
            target->trafoObjToWorld.m[1][3],
            target->trafoObjToWorld.m[2][3]
        };
        StartCombineAni(position, aniID, maxAzimuth, maxElevation);
    }
}

void oCAniCtrl_Human::StopCombineAni(int aniID)
{
    model->FadeOutAni(aniID);
}

int oCAniCtrl_Human::DetectChasm(const zVEC3& origin, const zVEC3& direction, float& chasmDepth,
    zVEC3& normal)
{
    zCWorld* homeWorld = npc->GetHomeWorld();
    if (homeWorld->TraceRayFirstHit(origin, direction, npc, 0x20))
        return 0;

    zVEC3 downOrigin = {
        origin.n[0] + direction.n[0],
        origin.n[1] + direction.n[1],
        origin.n[2] + direction.n[2]
    };
    zVEC3 down = { 0.0f, -(origin.n[1] - feetY + chasmDepth), 0.0f };
    if (homeWorld->TraceRayNearestHit(downOrigin, down, npc, 0xA0)) {
        chasmDepth = feetY - homeWorld->traceRayReport.foundIntersection.n[1];
        normal = homeWorld->traceRayReport.foundPolyNormal;
        float normalY = normal.n[1];
        if (normalY < -1.0f)
            normalY = -1.0f;
        else if (normalY > 1.0f)
            normalY = 1.0f;
        float angle = acos(normalY);
        if (angle < 3.1415926535897931f && angle > config.zMV_MAX_GROUND_ANGLE_WALK)
            return 1;
    }
    return chasmDepth >= 50.0f;
}

int oCAniCtrl_Human::IsInCriticalAni()
{
    int criticalAnis[] = { s_hang, t_hang_2_stand, s_fallen, s_fallenb, s_dead1, s_dead2 };
    for (int i = 0; i < 6; ++i) {
        if (model->IsAniActive(model->GetAniFromAniID(criticalAnis[i])))
            return 1;
    }
    static const char* criticalNames[] = {
        "T_STAND_2_HANG",
        "T_STAND_2_SITB", "S_SITB", "T_SITB_2_STAND", "T_SITB_2_LIEB", "S_LIEB",
        "T_LIEB_2_SITB", "T_STAND_2_SITF", "S_SITF", "T_SITF_2_STAND", "T_SITF_2_LIEF",
        "S_LIEF", "T_LIEF_2_SITF"
    };
    for (int i = 0; i < 13; ++i) {
        int aniID = model->GetAniIDFromAniName(criticalNames[i]);
        if (model->IsAniActive(model->GetAniFromAniID(aniID)))
            return 1;
    }
    return 0;
}

void oCAniCtrl_Human::Archive(zCArchiver& archiver)
{
    zCAIPlayer::Archive(archiver);
    if (!archiver.InSaveGame())
        return;
    archiver.WriteObject(npc);
    archiver.WriteInt(walkmode);
    archiver.WriteInt(wmode);
    archiver.WriteInt(wmode_last);
    archiver.WriteInt(wmode_selected);
    archiver.WriteBool(changeweapon);
    archiver.WriteInt(actionMode);
}

void oCAniCtrl_Human::Unarchive(zCArchiver& archiver)
{
    zCAIPlayer::Unarchive(archiver);
    if (!archiver.InSaveGame())
        return;
    npc = (oCNpc*)archiver.ReadObject(0);
    Init(npc);
    if (npc)
        npc->Release();
    walkmode = 0;
    actionMode = 0;
    walkmode = archiver.ReadInt();
    wmode = archiver.ReadInt();
    wmode_last = archiver.ReadInt();
    wmode_selected = archiver.ReadInt();
    changeweapon = archiver.ReadBool();
    actionMode = archiver.ReadInt();
}

void oCAniCtrl_Human::SetActionMode(int mode)
{
    actionMode = mode;
    if (mode == 5) {
        npc->SetBodyState(5);
        zMV_DO_SURFACE_ALIGN = 0;
        npc->ResetXZRotationsWorld();
    } else if (mode == 6) {
        npc->SetBodyState(7);
        zMV_DO_SURFACE_ALIGN = 1;
        npc->ResetXZRotationsWorld();
    } else {
        zMV_DO_SURFACE_ALIGN = defaultSurfaceAlign;
    }
}

void oCAniCtrl_Human::_Stand()
{
    if (!model)
        return;
    if (actionMode == 6) {
        npc->SetBodyState(7);
        if (model->IsAniActive(model->GetAniFromAniID(s_divef)))
            model->StartAni(t_divef_2_dive, 0);
    } else if (actionMode == 5) {
        npc->SetBodyState(5);
        if (model->IsAniActive(model->GetAniFromAniID(s_swimf)))
            model->StartAni(t_swimf_2_swim, 0);
        else if (model->IsAniActive(model->GetAniFromAniID(s_swimb)))
            model->StartAni(t_swimb_2_swim, 0);
    } else if (model->IsAniActive(model->GetAniFromAniID(_s_walkl))) {
        int footPos = GetFootPos();
        if (npc->GetWeaponMode())
            model->StartAni(_s_walk, 0);
        else if (footPos == 1)
            StartAni(_t_walkl_2_walk, _s_walk);
        else if (footPos == 2)
            StartAni(_t_walkr_2_walk, _s_walk);
        npc->SetBodyState(0);
    } else if (model->IsAniActive(model->GetAniFromAniID(_t_walk_2_walkl))) {
        if (npc->GetWeaponMode())
            model->StartAni(_s_walk, 0);
        npc->SetBodyState(0);
    } else if (model->IsAniActive(model->GetAniFromAniID(_s_walkbl))) {
        int footPos = GetFootPos();
        if (footPos == 1)
            StartAni(_t_walkbl_2_walk, _s_walk);
        else if (footPos == 2)
            StartAni(_t_walkbr_2_walk, _s_walk);
        npc->SetBodyState(0);
    } else if (model->IsAniActive(model->GetAniFromAniID(_s_aim))) {
        StartAni(_t_aim_2_walk, _s_walk);
        npc->SetBodyState(0);
    } else if (IsStateAniActive(model->GetAniFromAniID(_s_walk))) {
        npc->SetCollDetStat(1);
        npc->SetCollDetDyn(1);
        npc->SetBodyState(0);
        npc->overrideFallDownHeight = 0;
    }
    if (!model->IsStateActive(model->GetAniFromAniID(_s_jumpup)) &&
        !model->IsStateActive(model->GetAniFromAniID(s_hang)))
        zMV_DO_SURFACE_ALIGN &= ~1;
}

void oCAniCtrl_Human::_Forward()
{
    if (actionMode == 6) {
        npc->SetBodyState(7);
        if (!model->IsAniActive(model->GetAniFromAniID(t_swim_2_dive)) && waterLevel != 2) {
            model->StartAni(s_swim, 0);
            npc->ResetXZRotationsWorld();
            SetWalkMode(4);
            SetActionMode(5);
            return;
        }
        if (model->IsAniActive(model->GetAniFromAniID(s_dive))) {
            model->StartAni(t_dive_2_divef, 0);
            model->FadeOutAnisLayerRange(20, 20);
            return;
        }
    } else if (actionMode == 5) {
        npc->SetBodyState(5);
        if (model->IsAniActive(model->GetAniFromAniID(s_swim))) {
            model->StartAni(t_swim_2_swimf, 0);
            model->FadeOutAnisLayerRange(20, 20);
            return;
        }
        if (model->IsAniActive(model->GetAniFromAniID(s_swimb))) {
            model->StartAni(t_swimb_2_swim, 0);
            model->FadeOutAnisLayerRange(20, 20);
            return;
        }
    } else {
        if (model->IsAniActive(model->GetAniFromAniID(_s_walk))) {
            model->StartAni(_t_walk_2_walkl, 0);
            npc->SetCollDetStat(1);
            npc->SetCollDetDyn(1);
        } else if (model->IsAniActive(model->GetAniFromAniID(_s_walkbl))) {
            model->StartAni(_t_walkbl_2_walk, 0);
        }
        npc->SetBodyState(walkmode == 0 ? 3 : walkmode == 1 ? 1 : 2);
    }
    model->FadeOutAnisLayerRange(20, 20);
}

void oCAniCtrl_Human::_Backward()
{
    if (actionMode == 6) {
        npc->SetBodyState(7);
        if (model->IsAniActive(model->GetAniFromAniID(s_divef))) {
            model->StartAni(t_divef_2_dive, 0);
            model->FadeOutAnisLayerRange(20, 20);
            return;
        }
    } else if (actionMode == 5) {
        npc->SetBodyState(5);
        if (model->IsAniActive(model->GetAniFromAniID(s_swim))) {
            model->StartAni(t_swim_2_swimb, 0);
            model->FadeOutAnisLayerRange(20, 20);
            return;
        }
        if (model->IsAniActive(model->GetAniFromAniID(s_swimf))) {
            model->StartAni(t_swimf_2_swim, 0);
            model->FadeOutAnisLayerRange(20, 20);
            return;
        }
    } else if (model->IsAniActive(model->GetAniFromAniID(_s_walk))) {
        npc->SetCollDetStat(1);
        npc->SetCollDetDyn(1);
        if (_t_walk_2_walkbl == -1)
            model->StartAni(_t_jumpb, 0);
        else
            StartAni(_t_walk_2_walkbl, -1);
        npc->SetBodyState(walkmode == 0 ? 3 : walkmode == 1 ? 1 : 2);
    } else if (model->IsAniActive(model->GetAniFromAniID(_s_walkl))) {
        _Stand();
    }
    model->FadeOutAnisLayerRange(20, 20);
}

void oCAniCtrl_Human::SetAnimations(int weaponMode, const zSTRING& weaponName)
{
    t_stand_2_cast = -1;
    s_cast = -1;
    t_cast_2_shoot = -1;
    t_cast_2_stand = -1;
    s_shoot = -1;
    t_shoot_2_stand = -1;
    t_stand_2_cast = model->GetAniIDFromAniName(
        "T_" + weaponName + "RUN_2_" + weaponName + "CAST");
    s_cast = model->GetAniIDFromAniName("S_" + weaponName + "CAST");
    t_cast_2_shoot = model->GetAniIDFromAniName(
        "T_" + weaponName + "CAST_2_" + weaponName + "SHOOT");
    t_cast_2_stand = model->GetAniIDFromAniName("T_" + weaponName + "CAST_2_STAND");
    s_shoot = model->GetAniIDFromAniName("S_" + weaponName + "SHOOT");
    t_shoot_2_stand = model->GetAniIDFromAniName("T_" + weaponName + "SHOOT_2_STAND");
    if (t_stand_2_cast == -1) {
        t_stand_2_cast = model->GetAniIDFromAniName(
            "T_" + weaponName + "RUN_2_" + weaponName + "SHOOT");
    }
}

int oCAniCtrl_Human::TransitionToInvest()
{
    if (model->IsAniActive(model->GetAniFromAniID(_s_walk))) {
        if (t_stand_2_cast != -1)
            model->StartAni(t_stand_2_cast, 0);
    } else if (model->IsAniActive(model->GetAniFromAniID(t_shoot_2_stand))) {
        model->FadeOutAni(t_shoot_2_stand);
    }
    return model->IsAniActive(model->GetAniFromAniID(s_cast));
}

int oCAniCtrl_Human::TransitionToCast()
{
    if (model->IsAniActive(model->GetAniFromAniID(s_cast))) {
        if (t_cast_2_shoot != -1)
            StartAni(t_cast_2_shoot, s_shoot);
    } else if (model->IsAniActive(model->GetAniFromAniID(s_shoot))) {
        zCModelAniActive* activeAni = model->GetActiveAni(s_shoot);
        if (activeAni)
            activeAni->nextAni = model->GetAniFromAniID(t_shoot_2_stand);
    }
    if (model->IsAniActive(model->GetAniFromAniID(t_shoot_2_stand))) {
        zCModelAniActive* activeAni = model->GetActiveAni(t_shoot_2_stand);
        if (activeAni)
            activeAni->nextAni = model->GetAniFromAniID(_s_walk);
    }
    return model->IsAniActive(model->GetAniFromAniID(s_shoot));
}

int oCAniCtrl_Human::TransitionToStand()
{
    if (model->IsAniActive(model->GetAniFromAniID(s_shoot))) {
        if (t_shoot_2_stand == -1)
            model->StartAni(_s_walk, 0);
        else
            StartAni(t_shoot_2_stand, _s_walk);
    } else if (model->IsAniActive(model->GetAniFromAniID(s_cast))) {
        if (t_cast_2_stand == -1)
            StartAni(_s_walk, -1);
        else
            StartAni(t_cast_2_stand, _s_walk);
    } else if (model->IsAniActive(model->GetAniFromAniID(t_stand_2_cast))) {
        StartAni(_s_walk, -1);
    }
    return model->IsAniActive(model->GetAniFromAniID(_s_walk));
}

int oCAniCtrl_Human::IsInCastAni()
{
    return model->IsAniActive(model->GetAniFromAniID(s_shoot)) ||
        model->IsAniActive(model->GetAniFromAniID(s_cast));
}

int oCAniCtrl_Human::IsInWeaponChoose()
{
    if (!model)
        return 0;
    for (int i = 0; i < 8; ++i) {
        if (model->IsAniActive(model->GetAniFromAniID(s_weapon[i])))
            return 1;
    }
    return 0;
}

int oCAniCtrl_Human::GetWaterLevel()
{
    zCCollObjectCharacter* collision = static_cast<zCCollObjectCharacter*>(vob->GetCollisionObject());
    if (waterLevel == 1 && collision->m_oSpatialState.m_fWaterY >= feetY)
        return 1;
    if (waterLevel == 2 && collision->m_oSpatialState.m_fWaterY >= feetY)
        return 2;
    return 0;
}

int oCAniCtrl_Human::IsInWater()
{
    return GetWaterLevel() != 0;
}

zCObject* oCAniCtrl_Human::_CreateNewInstance()
{
    oCAniCtrl_Human* controller = new oCAniCtrl_Human;
    zCClassDef::ObjectCreated(controller, classDef);
    return controller;
}

void oCAniCtrl_Human::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}
