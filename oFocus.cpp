#include "include/oFocus.h"

oCNpcFocus* oCNpcFocus::focus = 0;
oCNpcFocus* oCNpcFocus::focuslist[6];
int oCNpcFocus::s_iHightlightMeleeFocus = 0;
int oCNpcFocus::s_bHighlightInteractFocus = 0;
oCVisualFX* oCNpcFocus::highLightFX = 0;
int oCNpcFocus::isHighLighted = 0;

static int focusState = -1;
static float focusFarDistance = 250000.0f * 0.0166666675f;
static float focusNearDistance = focusFarDistance * 0.0166666675f;
static float focusFarPlaneDistance = 250000.0f * 24.0f;
static zSTRING focusPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING focusPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING focusPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING focusPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING focusPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING focusPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING focusPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING focusPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING focusPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING focusPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static zSTRING focusBuffer;
static zSTRING focusnames[] = {
    "Focus_Normal", "Focus_Melee", "Focus_Ranged",
    "Focus_Throw_Item", "Focus_Throw_Mob", "Focus_Magic"
};

void oCNpcFocus::InitFocusModes()
{
    for (int i = 0; i < 6; ++i) {
        focuslist[i] = new oCNpcFocus;
        focuslist[i]->Init(focusnames[i]);
    }
    focus = focuslist[0];
}

void oCNpcFocus::ExitFocusModes()
{
    focus = 0;
    for (int i = 0; i < 6; ++i) {
        StopHighlightningFX();
        delete focuslist[i];
        focuslist[i] = 0;
    }
}

void oCNpcFocus::SetFocusMode(int mode)
{
    if (mode >= 0 && mode < 6)
        focus = focuslist[mode];
}

zSTRING oCNpcFocus::GetFocusName()
{
    for (int i = 0; i < 6; ++i) {
        if (focuslist[i] == focus)
            return focusnames[i];
    }
    return "no focus";
}

oCNpcFocus::oCNpcFocus()
    : n_range3(0.0f), n_range1(0.0f), n_range2(0.0f), n_azi(0.0f), n_elev1(0.0f), n_elev2(0.0f),
      n_prio(0), i_range1(0.0f), i_range2(0.0f), i_azi(0.0f), i_elev1(0.0f), i_elev2(0.0f),
      i_prio(0), m_range1(0.0f), m_range2(0.0f), m_azi(0.0f), m_elev1(0.0f), m_elev2(0.0f),
      m_prio(0), max_range(0.0f)
{
}

oCNpcFocus::~oCNpcFocus()
{
    StopHighlightningFX();
}

void oCNpcFocus::Init(zSTRING& name)
{
    name.Upper();
    Init(parser.GetIndex(name));
}

void oCNpcFocus::Init(int instance)
{
    parser.CreateInstance(instance, this);
    max_range = n_range2;
    if (max_range < i_range2)
        max_range = i_range2;
    if (max_range < m_range2)
        max_range = m_range2;
    if (max_range < n_range3)
        max_range = n_range3;
    n_range1 *= n_range1;
    n_range2 *= n_range2;
    n_range3 *= n_range3;
    i_range1 *= i_range1;
    i_range2 *= i_range2;
    m_range1 *= m_range1;
    m_range2 *= m_range2;
}

int oCNpcFocus::IsValid(int vobType)
{
    if (vobType == 0x82)
        return n_prio >= 0;
    if (vobType == 0x81)
        return i_prio >= 0;
    if (vobType == 0x80)
        return m_prio >= 0;
    return 0;
}

float oCNpcFocus::GetMaxRange()
{
    return max_range;
}

float oCNpcFocus::GetRange2()
{
    return n_range2;
}

float oCNpcFocus::GetItemThrowRange()
{
    return i_range2;
}

float oCNpcFocus::GetMobThrowRange()
{
    return m_range2;
}

int oCNpcFocus::HasLowerPriority(int firstType, int secondType)
{
    return GetPriority(firstType) < GetPriority(secondType);
}

int oCNpcFocus::GetPriority(int vobType)
{
    if (vobType == 0x82)
        return n_prio;
    if (vobType == 0x81)
        return i_prio;
    if (vobType == 0x80)
        return m_prio;
    return -1;
}

int oCNpcFocus::IsInRange(int vobType, float distance)
{
    if (vobType == 0x82)
        return IsNpcInRange(distance);
    if (vobType == 0x81)
        return IsItemInRange(distance);
    if (vobType == 0x80)
        return IsMobInRange(distance);
    return 0;
}

int oCNpcFocus::IsInAngle(int vobType, float azimuth, float elevation)
{
    if (vobType == 0x82)
        return IsNpcInAngle(azimuth, elevation);
    if (vobType == 0x81)
        return IsItemInAngle(azimuth, elevation);
    if (vobType == 0x80)
        return IsMobInAngle(azimuth, elevation);
    return 0;
}

int oCNpcFocus::IsNpcInRange(float distance)
{
    float maxRange = n_range2 > n_range3 ? n_range2 : n_range3;
    return n_range1 <= distance && distance <= maxRange;
}

int oCNpcFocus::IsNpcInAngle(float azimuth, float elevation)
{
    if (azimuth < 0.0f)
        azimuth = -azimuth;
    return azimuth <= n_azi && n_elev1 <= elevation && elevation <= n_elev2;
}

int oCNpcFocus::IsItemInRange(float distance)
{
    return i_range1 <= distance && distance <= i_range2;
}

int oCNpcFocus::IsItemInAngle(float azimuth, float elevation)
{
    if (azimuth < 0.0f)
        azimuth = -azimuth;
    return azimuth <= i_azi && i_elev1 <= elevation && elevation <= i_elev2;
}

int oCNpcFocus::IsMobInRange(float distance)
{
    return m_range1 <= distance && distance <= m_range2;
}

int oCNpcFocus::IsMobInAngle(float azimuth, float elevation)
{
    if (azimuth < 0.0f)
        azimuth = -azimuth;
    return azimuth <= m_azi && m_elev1 <= elevation && elevation <= m_elev2;
}

float oCNpcFocus::GetAzi()
{
    return n_azi;
}

void oCNpcFocus::StartHighlightningFX(oCNpc* npc)
{
    if (highLightFX && highLightFX->GetOrigin() != npc)
        StopHighlightningFX();
    if (!highLightFX) {
        if (s_iHightlightMeleeFocus == 1 || s_iHightlightMeleeFocus == 3) {
            zSTRING effectName = "FOCUS_HIGHLIGHT";
            highLightFX = oCVisualFX::CreateAndPlay(effectName, npc, npc, 1, 0.0f, 0, 0);
        }
        isHighLighted = 1;
    }
}

void oCNpcFocus::StopHighlightningFX()
{
    if (highLightFX) {
        highLightFX->Stop(1);
        highLightFX->Release();
        highLightFX = 0;
    }
    isHighLighted = 0;
}
