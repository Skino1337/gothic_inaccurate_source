#include "include/oNpcSlots.h"
#include "include/zCache.h"
#include "include/oAiHuman.h"
#include "include/oNpcTalent.h"
#include "include/oNpcMessages.h"
#include "include/oFocus.h"
#include "include/zParticle.h"
#include "include/oMob.h"
#include "include/zMorphMesh.h"
#include "include/oAiShoot.h"
#include "include/oScreens.h"
#include "include/oInformationManager.h"
#include "include/zSound.h"
#include "include/zTools.h"
#include "include/z3d.h"
#include <cmath>
#include <cstdlib>

static zCOLOR npcColorYellow(255, 255, 0, 255);
static zCOLOR npcColorWhite(255, 255, 255, 255);
static zCOLOR npcColorBrightWhite(255, 255, 255, 255);
static zCOLOR npcColorCyan(0, 63, 255, 255);
static zCOLOR npcColorGold(255, 255, 0, 255);
static zCOLOR npcColorGray(127, 127, 127, 255);
static zSTRING torchBurningInstance("ITLSTORCHBURNING");
static zSTRING npcPerceptionNames[32] = {
    "NONE", "ASSESSPLAYER", "ASSESSENEMY", "ASSESSFIGHTER", "ASSESSBODY", "ASSESSITEM\t",
    "ASSESSMURDER", "ASSESSDEFEAT", "ASSESSDAMAGE", "ASSESSOTHERSDAMAGE", "ASSESSTHREAT",
    "ASSESSREMOVEWEAPON", "OBSERVEINTRUDER", "ASSESSFIGHTSOUND", "ASSESSQUIETSOUND",
    "ASSESSWARN", "CATCHTHIEF", "ASSESSTHEFT", "ASSESSCALL", "ASSESSTALK", "ASSESSGIVENITEM",
    "ASSESSFAKEGUILD", "MOVEMOB", "MOVENPC", "DRAWWEAPON", "OBSERVESUSPECT", "NPCCOMMAND",
    "ASSESSMAGIC", "ASSESSSTOPMAGIC", "ASSESSCASTER", "ASSESSSURPRISE", "ASSESSENTERROOM",
    "ASSESSUSEMOB"
};
// EXE: 0x00AB1ED8
static zCArray<int> npcPerceptionList(1200);
// EXE: 0x00AB2688
int oCNpc::s_bEnabledDialogCamStop;
// EXE: 0x00AB2680
int oCNpc::s_bTargetLocked;
// EXE: 0x00AB26D8
zMAT4 oCNpc::s_playerPositionMatrix(1.0f);
// EXE: 0x008B0404
oCNpc::oEBloodMode oCNpc::modeBlood = oCNpc::oEBloodMode_Trails;
// EXE: 0x00AB268C
oCNpc* oCNpc::dontArchiveThisNpc;
// EXE: 0x008B8114
int oCNpc::isEnabledTalkBox = 1;
// EXE: 0x008B8118
int oCNpc::isEnabledTalkBoxPlayer = 1;
// EXE: 0x00AB2678
int oCNpc::isEnabledTalkBoxAmbient;
// EXE: 0x00AB267C
int oCNpc::isEnabledTalkBoxNoise;
static float npcTalentRange = 1000.0f;
zSTRING PLAYER_MOB_MISSING_KEY             = "PLAYER_MOB_MISSING_KEY";
zSTRING PLAYER_MOB_MISSING_LOCKPICK        = "PLAYER_MOB_MISSING_LOCKPICK";
zSTRING PLAYER_MOB_MISSING_KEY_OR_LOCKPICK = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
zSTRING PLAYER_MOB_NEVER_OPEN              = "PLAYER_MOB_NEVER_OPEN";
zSTRING PLAYER_MOB_TOO_FAR_AWAY            = "PLAYER_MOB_TOO_FAR_AWAY";
zSTRING PLAYER_MOB_WRONG_SIDE              = "PLAYER_MOB_WRONG_SIDE";
zSTRING PLAYER_MOB_MISSING_ITEM            = "PLAYER_MOB_MISSING_ITEM";
zSTRING PLAYER_MOB_ANOTHER_IS_USING        = "PLAYER_MOB_ANOTHER_IS_USING";
zSTRING PLAYER_PLUNDER_IS_EMPTY            = "PLAYER_PLUNDER_IS_EMPTY";
zSTRING PLAYER_RANGED_NO_AMMO              = "PLAYER_RANGED_NO_AMMO";

static int npcFightState = -1;
static int npcState = -1;
static int npcRuntimeState = -1;
static int npcRuntimeState2 = -1;
static zSTRING npcStatePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING npcStatePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING npcStatePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING npcStatePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING npcStatePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING npcStatePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING npcStatePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING npcStatePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING npcStatePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING npcStatePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static int npcDialogState = -1;
static int npcInteractionState = -1;
static int npcGeneralState = -1;
static int npcActionState = -1;
static int npcDialogRuntimeState = -1;
static int npcMobState = -1;
static zSTRING npcMobPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING npcMobPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING npcMobPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING npcMobPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING npcMobPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING npcMobPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING npcMobPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING npcMobPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING npcMobPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING npcMobPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static zSTRING npcDialogRuntimePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING npcDialogRuntimePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING npcDialogRuntimePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING npcDialogRuntimePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING npcDialogRuntimePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING npcDialogRuntimePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING npcDialogRuntimePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING npcDialogRuntimePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING npcDialogRuntimePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING npcDialogRuntimePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float npcActionFarDistance = 250000.0f * 0.0166666675f;
static float npcActionNearDistance = npcActionFarDistance * 0.0166666675f;
static float npcActionFarPlaneDistance = 250000.0f * 24.0f;
static zSTRING npcActionPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING npcActionPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING npcActionPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING npcActionPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING npcActionPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING npcActionPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING npcActionPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING npcActionPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING npcActionPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING npcActionPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float npcGeneralFarDistance = 250000.0f * 0.0166666675f;
static float npcGeneralNearDistance = npcGeneralFarDistance * 0.0166666675f;
static float npcGeneralFarPlaneDistance = 250000.0f * 24.0f;
static zSTRING npcGeneralPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING npcGeneralPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING npcGeneralPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING npcGeneralPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING npcGeneralPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING npcGeneralPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING npcGeneralPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING npcGeneralPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING npcGeneralPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING npcGeneralPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static zSTRING npcInteractionPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING npcInteractionPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING npcInteractionPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING npcInteractionPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING npcInteractionPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING npcInteractionPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING npcInteractionPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING npcInteractionPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING npcInteractionPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING npcInteractionPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float npcDialogFarDistance = 250000.0f * 0.0166666675f;
static float npcDialogNearDistance = npcDialogFarDistance * 0.0166666675f;
static float npcDialogFarPlaneDistance = 250000.0f * 24.0f;
static zSTRING npcDialogPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING npcDialogPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING npcDialogPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING npcDialogPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING npcDialogPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING npcDialogPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING npcDialogPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING npcDialogPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING npcDialogPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING npcDialogPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static zSTRING npcRuntimePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING npcRuntimePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING npcRuntimePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING npcRuntimePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING npcRuntimePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING npcRuntimePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING npcRuntimePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING npcRuntimePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING npcRuntimePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING npcRuntimePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static zSTRING npcPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING npcPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING npcPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING npcPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING npcPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING npcPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING npcPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING npcPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING npcPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING npcPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
zSTRING oCNpc::FightAIInstances[19];
static zSTRING npcFightActionNames[19] = {
    "ENEMY_PREHIT", "ENEMY_STORMPREHIT", "ENEMY_TURNTOHIT_FOCUS", "MY_W_COMBO",
    "MY_W_RUNTO", "MY_W_STRAFE", "MY_W_FOCUS", "MY_W_NOFOCUS", "MY_G_COMBO",
    "MY_G_RUNTO", "MY_G_STRAFE", "MY_G_FOCUS", "MY_G_FK_NOFOCUS", "MY_FK_FOCUS",
    "MY_G_FK_NOFOCUS", "MY_FK_FOCUS_FAR", "MY_FK_NOFOCUS_FAR", "MY_FK_FOCUS_MAG",
    "MY_FK_NOFOCUS_MAG"
};

static zCCacheAsk<oCNpc*, oCNpc::TActiveInfo> npcActiveInfoCache(13, 100);
int oCNpc::ai_disabled;

int BitToNumber(int value)
{
    int result = 0;
    if (value)
    {
        while (!(value & 1))
        {
            value >>= 1;
            ++result;
            if (result >= 64)
                return 0;
        }
    }
    return result;
}

void TNpcSlot::SetVob(oCVob* value)
{
    if (vob == value)
        return;
    ClearVob();

    vob = value;
    wasVobTreeWhenInserted = 1;
    value->AddRefVobSubtree();
    ogame->GetWorld()->RemoveVobFromLists(value);
}

void TNpcSlot::ClearVob()
{
    if (vob)
    {
        if (wasVobTreeWhenInserted)
            vob->ReleaseVobSubtree();
        else
            vob->Release();
        vob = 0;
    }
}

oCNpc::TActiveInfo::TActiveInfo(const oCNpc* npc)
    : modified(0), putTorchBackInHand(0), changeTorchAni(0), changeTorchAniTo(0)
{
}

oCNpc::TActiveInfo::~TActiveInfo()
{
}

const oCNpc::TActiveInfo* oCNpc::GetActiveInfo() const
{
    oCNpc* npc = const_cast<oCNpc*>(this);
    return npcActiveInfoCache[npc];
}

oCNpc::TActiveInfo* oCNpc::GetActiveInfoWritable() const
{
    oCNpc* npc = const_cast<oCNpc*>(this);
    TActiveInfo* info = npcActiveInfoCache[npc];
    info->modified = 1;
    return info;
}

void oCNpc::Shrink()
{
    if (!ogame)
        return;

    SetVisual(static_cast<zCVisual*>(0));
    activeOverlays.AllocAbs(activeOverlays.numInArray);
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (humanAI)
    {
        humanAI->SetVob(this);
        humanAI->Release();
        human_ai = 0;
    }
    SetAI(0);
    anictrl = 0;
    SetCollisionObject(0);

    if (!effect.IsEmpty() && effectList.numInArray > 0)
    {
        oCVisualFX* effectObject = effectList[effectList.numInArray - 1];
        effectList.RemoveOrder(effectObject);
        effectObject->Release();
    }
}

void oCNpc::UnShrink()
{
    if (visual && human_ai)
        return;

    static int recursion;
    if (recursion)
        return;
    recursion = 1;

    SetVisual(mds_name);
    if (visual)
    {
        InitModel();
        SetHead();
    }
    else
        SetVisual("Meatbug.mds");

    zCModel* model = dynamic_cast<zCModel*>(visual);
    if (model)
    {
        for (int i = activeOverlays.numInArray - 1; i >= 0; --i)
        {
            if (!model->ApplyModelProtoOverlay(activeOverlays[i]))
                activeOverlays.RemoveOrderIndex(i);
        }
        model->SetModelScale(model_scale);
        model->fatness = model_fatness;
    }

    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (humanAI)
    {
        humanAI->SetVob(this);
        humanAI->world = ogame->GetWorld();
        humanAI->InitAnimations();
    }
    else
        Enable();
    if (humanAI)
        humanAI->SearchStandAni();

    SetCollisionObject(0);
    CreateCollisionObject();
    if (homeWorld && !effect.IsEmpty())
        AddEffect(effect);
    recursion = 0;
}

void oCNpc::AvoidShrink(int flags)
{
    struct oCShrinkHelper {
        int unused;
    };
    static zCCacheAsk<oCNpc*, oCShrinkHelper> shrinkCache(0x65, 0x7D0);
    static int recursion;
    if (!recursion && !mds_name.IsEmpty())
    {
        recursion = 1;
        shrinkCache[this];
        recursion = 0;
    }
}

int oCNpc::ApplyOverlay(const zSTRING& overlay)
{
    zSTRING overlayName = overlay;
    overlayName.Upper();
    for (int i = 0; i < activeOverlays.numInArray; ++i)
    {
        if (activeOverlays[i] == overlayName)
            return 1;
    }

    if (!visual)
    {
        activeOverlays.InsertEnd(overlayName);
        return 1;
    }

    static zCSparseArray<zTOverlayIndex, zCModelPrototype*> overlayCache(13);
    zTOverlayIndex overlayIndex;
    overlayIndex.overlay = overlayName;
    zCModelPrototype** prototype = overlayCache[overlayIndex];
    if (!*prototype)
        *prototype = zCModelPrototype::Load(overlayName);

    zCModel* model = dynamic_cast<zCModel*>(visual);
    if (!model)
    {
        AvoidShrink(1000);
        model = dynamic_cast<zCModel*>(visual);
    }
    if (model && !model->ApplyModelProtoOverlay(*prototype))
        return 0;

    activeOverlays.InsertEnd(overlayName);
    return 1;
}

void oCNpc::RemoveOverlay(const zSTRING& overlay)
{
    zSTRING overlayName = overlay;
    overlayName.Upper();
    for (int i = activeOverlays.numInArray - 1; i >= 0; --i)
    {
        if (activeOverlays[i] != overlayName)
            continue;

        activeOverlays.RemoveOrderIndex(i);
        zCModel* model = dynamic_cast<zCModel*>(visual);
        if (!model)
        {
            AvoidShrink(0);
            model = dynamic_cast<zCModel*>(visual);
        }
        if (model)
            model->RemoveModelProtoOverlay(overlayName);
        return;
    }
}

void oCNpc::SetModelScale(const zVEC3& scale)
{
    model_scale = scale;
    zCModel* model = dynamic_cast<zCModel*>(visual);
    if (!model)
    {
        AvoidShrink(0);
        model = dynamic_cast<zCModel*>(visual);
    }
    if (model)
        model->SetModelScale(scale);
}

void oCNpc::SetFatness(float fatness)
{
    model_fatness = fatness;
    zCModel* model = dynamic_cast<zCModel*>(visual);
    if (!model)
    {
        AvoidShrink(0);
        model = dynamic_cast<zCModel*>(visual);
    }
    if (model)
        model->fatness = fatness;
}

static zCClassDef oCNpcClassDef("oCNpc", "oCVob", oCNpc::_CreateNewInstance, 0, 0xA04, 7, "");
zCClassDef* oCNpc::classDef = &oCNpcClassDef;
static zSTRING npcHeroClassNames[6] = {
    "", "PC_HERO", "PC_FIGHTER", "PC_THIEF", "PC_MAGE", "PC_PSIONIC"
};

oCNpc::oCNpc()
    : instanz(-1), attitude(2), tmp_attitude(2), bodyState(0), interactMob(0),
      interactItemCurrentState(-1), interactItemTargetState(0)
{
    mag_book = 0;
    spellMana = 0;
    anictrl = 0;
    route = 0;
    interactItem = 0;
    trader = 0;
    talkOther = 0;
    tradeNpc = 0;
    human_ai = 0;
    canTalk = 1.0f;
    fallDownHeight = 500.0f;
    fallDownDamage = 0;
    overrideFallDownHeight = 0;
    model_scale = zVEC3{ 1.0f, 1.0f, 1.0f };
    model_fatness = 0.0f;

    static const char* slotNames[] = {
        "ZS_RIGHTHAND", "ZS_LEFTHAND", "ZS_SWORD", "ZS_LONGSWORD",
        "ZS_BOW", "ZS_CROSSBOW", "ZS_HELMET", "ZS_TORSO"
    };
    for (int i = 0; i < 8; ++i)
    {
        TNpcSlot* slot = new TNpcSlot;
        slot->name = slotNames[i];
        invSlot.InsertEnd(slot);
    }

    InitDamage();
    RbtReset();
    inventory2.SetOwner(this);
    state.SetOwner(this);
    oCNpcTalent::CreateTalentList(&talents);
    type = zVOB_TYPE_NSC;
    SetCollisionClass(&oCCollObjectCharacter::s_oCollObjClass);
    SetCollDetStat(1);
    SetCollDetDyn(1);
}

int oCVob::IsOwnedByGuild(int guild)
{
    return 0;
}

int oCVob::IsOwnedByNpc(int npcInstance)
{
    return 0;
}

oTSndMaterial oCVob::GetSoundMaterial()
{
    return static_cast<oTSndMaterial>(0);
}

TNpcSlot::TNpcSlot()
    : inInventory(0), tmpLevel(0), vob(0), wasVobTreeWhenInserted(0)
{
}

TNpcSlot::~TNpcSlot()
{
    ClearVob();
}

zCClassDef* oCNpc::_GetClassDef() const
{
    return classDef;
}

int oCNpc::IsAniMessageRunning()
{
    return (bodyState >> 19) & 1;
}

oTRobustTrace::~oTRobustTrace()
{
}

void oCNpc::SetVisual(zCVisual* visual)
{
    zCVob::SetVisual(visual);
    SetCollisionClass(&oCCollObjectCharacter::s_oCollObjClass);
}

void oCNpc::CleanUp()
{
    state.SetRoutine(0, 0);
    for (int i = 0; i < listOfVoiceHandles.numInArray; ++i)
    {
        int handle = listOfVoiceHandles.GetSafe(i);
        zsound->StopSound(handle);
    }
    voiceIndex = 0;
    while (!UpdateNextVoice())
    {
    }
    if (focus_vob)
    {
        focus_vob->Release();
        focus_vob = 0;
    }
    ClearVobList();
    SetInteractItem(0);
    if (interactMob)
        GetInteractMob()->StopInteraction(this);
    SetInteractMob(0);
    knownNews.ClearNews();
    rtnMan.RemoveWayBoxes(this);
    rtnMan.RemoveRoutine(this);
    rbt.lastDirections.DeleteList();
    GetEM(0)->KillMessages();

    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (humanAI)
    {
        humanAI->Release();
        human_ai = 0;
    }
    SetAI(0);
    anictrl = 0;
    SetEnemy(0);
    SetFocusVob(0);
    if (fireVob)
    {
        fireVob->Release();
        fireVob = 0;
    }
    SetAI(0);
    SetRoute(0);
    magFrontier.DisposeWarningFX();
    magFrontier.DisposeShootFX();
    while (activeSpells.next)
    {
        oCSpell* spell = activeSpells.next->data;
        activeSpells.Remove(spell);
        if (spell)
            spell->Release();
    }
    talkOther = 0;
    bodyState &= ~(1 << 19);
    state.ClearParserReferences();
    while (effectList.numInArray > 0)
    {
        oCVisualFX* effect = effectList[0];
        effectList.Remove(effect);
        effect->Kill();
        effect->Release();
    }
}

// EXE: 0x0072E6A0
oCNpc::~oCNpc()
{
    CleanUp();
    for (int i = 0; i < invSlot.numInArray; ++i)
        delete invSlot[i];
    invSlot.DeleteList();
    for (int i = 0; i < talents.numInArray; ++i)
    {
        if (talents[i])
            talents[i]->Release();
    }
    talents.DeleteList();
    delete mag_book;
    mag_book = 0;
    delete trader;
    trader = 0;
    if (player == this)
        player = 0;
    parser.ResetWithVarReferenceList(npcPerceptionList, this);
}

void oCNpc::InitByScript(int instance, int flags)
{
    static int npcClassIndex = -1;
    if (npcClassIndex < 0)
    {
        npcClassIndex = parser.GetIndex("C_NPC");
        parser.CheckClassSize(npcClassIndex, 0x320);
    }
    if (!parser.MatchClass(instance, "C_NPC"))
        zerr.Report(zERR_TYPE_FAULT, 0, "U: NPC: Instance is not a npc. Dat/Tre-Files are not compatible !", 0, 0, 1631, "P:\\dev\\g2addon\\release\\Gothic\\_ulf\\oNpc.cpp", 0);

    instanz = instance;
    idx = instance;
    parser.SetInstance("SELF", this);
    parser.CreateInstance(instance, this);

    if (attribute[3] > 0)
    {
        if (!mag_book)
            mag_book = new oCMag_Book;
        mag_book->SetOwner(this);
        spells = 1;
    }

    if (!flags)
    {
        inventory2.CheckForEquippedItems(1);
        guildTrue = guild;
        if (ogame && ogame->GetGuilds() && player)
            attitude = ogame->GetGuilds()->GetAttitude(guildTrue, player->guildTrue);
        tmp_attitude = attitude;
        showaidebug = (showaidebug & ~4) | ((attitude & 1) << 2);
        if (attitude > 4)
            attitude = 4;
        if (attitude < 0)
            attitude = 0;
        if (fmode < 0 || fmode >= 8)
            fmode = 0;
        if (fmode == 1)
            SetToFistMode();
        else
        {
            oCItem* weapon = GetSlotItem(NPC_NODE_RIGHTHAND);
            if (weapon)
                SetToFightMode(weapon, 1);
        }
    }

    if (!damagetype)
        damagetype = 2;
    state.InitRoutine();
    int first;
    int second;
    zSTRING instanceName = parser.GetSymbolInfo(instance, first, second);
    SetVobName(instanceName);
    vobcheck_time = rand() % 3000;
    ogame->GetWorld()->RemoveVobFromLists(this);
    static float ghostAlpha = 0.3f;
    if (variousFlags & 4)
    {
        if (ghostAlpha < 0.01f)
        {
            ghostAlpha = zoptions->ReadReal("INTERNAL", "GhostAlpha", 0.3f);
            if (ghostAlpha < 0.01f || ghostAlpha > 1.01f)
            {
                ghostAlpha = 0.3f;
                zoptions->WriteReal("INTERNAL", "GhostAlpha", ghostAlpha, 0);
            }
        }
        visualAlpha = ghostAlpha;
    }
    else
        visualAlpha = 1.0f;
    if (!effect.IsEmpty())
        effect.Upper();
}

void oCNpc::InitHumanAI()
{
    if (human_ai)
        return;

    oCAIHuman* humanAI = new oCAIHuman;
    zCClassDef::ObjectCreated(humanAI, oCAIHuman::classDef);
    human_ai = humanAI;
    humanAI->Init(this);
    anictrl = humanAI;
    SetAI(humanAI);
}

void oCNpc::DeleteHumanAI()
{
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (humanAI)
        humanAI->Release();
    human_ai = 0;
    SetAI(0);
    anictrl = 0;
}

zSTRING oCNpc::GetGuildName()
{
    zCPar_Symbol* symbol = parser.GetSymbol("TXT_GUILDS");
    if (!symbol)
        return zSTRING("Error : Missing guild-names !");
    zSTRING result;
    symbol->GetValue(result, guild);
    return result;
}

zSTRING oCNpc::GetName(int index) const
{
    return name[index];
}

int oCNpc::IsHostile(oCNpc* other)
{
    return other && GetAttitude(other) == 0;
}

int oCNpc::IsFriendly(oCNpc* other)
{
    return other && GetAttitude(other) == 3;
}

int oCNpc::IsNeutral(oCNpc* other)
{
    return !other || GetAttitude(other) == 2;
}

int oCNpc::IsAngry(oCNpc* other)
{
    return other && GetAttitude(other) == 1;
}

int oCNpc::GetAttitude(oCNpc* other)
{
    if (!other)
        return 2;
    if (other->IsAPlayer())
        return attitude;
    return ogame->GetGuilds()->GetAttitude(guildTrue, other->guild);
}

int oCNpc::GetPermAttitude(oCNpc* other)
{
    if (other->IsAPlayer())
        return attitude;
    return ogame->GetGuilds()->GetAttitude(guildTrue, other->guild);
}

void oCNpc::SetAttitude(int value)
{
    attitude = value;
    if (value > 4)
        attitude = 4;
    if (value < 0)
        attitude = 0;
}

void oCNpc::SetTmpAttitude(int value)
{
    tmp_attitude = value;
    if (value > 4)
        tmp_attitude = 4;
    if (value < 0)
        tmp_attitude = 0;
}

int oCNpc::GetProtectionByIndex(oEIndexDamage index) const
{
    return protection[index];
}

int oCNpc::GetProtectionByType(oETypeDamage type) const
{
    for (int i = 0; i < 8; ++i)
    {
        if (type & (1 << i))
            return protection[i];
    }
    return 0;
}

int oCNpc::GetProtectionByMode(unsigned long mode) const
{
    int result = 0;
    for (int i = 0; i < 8; ++i)
    {
        if (mode & (1 << i))
            result += protection[i];
    }
    return result;
}

int oCNpc::GetFullProtection() const
{
    int result = 0;
    for (int i = 0; i < 8; ++i)
        result += protection[i];
    return result;
}

void oCNpc::SetProtectionByIndex(oEIndexDamage index, int value)
{
    protection[index] = value;
}

int oCNpc::GetDamageByIndex(oEIndexDamage index) const
{
    return damage[index];
}

int oCNpc::GetDamageByType(oETypeDamage type) const
{
    for (int i = 0; i < 8; ++i)
    {
        if (type & (1 << i))
            return damage[i];
    }
    return 0;
}

int oCNpc::GetDamageByMode(unsigned long mode) const
{
    int result = 0;
    for (int i = 0; i < 8; ++i)
    {
        if (mode & (1 << i))
            result += damage[i];
    }
    return result;
}

int oCNpc::GetFullDamage() const
{
    int result = 0;
    for (int i = 0; i < 8; ++i)
        result += damage[i];
    return result;
}

void oCNpc::SetDamages(int* values)
{
    if (values)
    {
        for (int i = 0; i < 8; ++i)
            damage[i] = values[i];
    }
}

int oCNpc::godmode;

int oCNpc::GetHitChance(int index) const
{
    return hitChance[index];
}

void oCNpc::SetHitChance(int index, int value)
{
    hitChance[index] = value;
}

void oCNpc::ChangeAttribute(int index, int delta)
{
    if (!delta || (delta < 0 && this == player && godmode)
        || (index == 0 && (variousFlags & 2) && delta != -999))
        return;

    attribute[index] += delta;
    if (attribute[index] < 0)
        attribute[index] = 0;
    if (index == 2 && attribute[2] > attribute[3])
        attribute[2] = attribute[3];
    else if (index == 0 && attribute[0] > attribute[1])
        attribute[0] = attribute[1];
    CheckModelOverlays();
}

void oCNpc::SetAttribute(int index, int value)
{
    attribute[index] = value;
    CheckModelOverlays();
}

int oCNpc::GetOverlay(zSTRING& overlay)
{
    zSTRING fileName = mds_name.PickWord(1, ".", "") + overlay + ".mds";
    zoptions->ChangeDir(zTOptionPaths_DIR_ANIMS);
    zFILE* file = zfactory->CreateZFile(fileName);
    int exists = file->Exists();
    delete file;
    return exists;
}

void oCNpc::CheckModelOverlays()
{
    if (attribute[0] <= 0)
        return;

    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    int wounded = (showaidebug & 0x100) != 0;
    zSTRING overlay("_wounded");
    if (attribute[0] <= 2 && (!IsAPlayer() || !humanAI || humanAI->GetWaterLevel() > 1))
    {
        if (!wounded && GetOverlay(overlay))
        {
            ApplyOverlay(overlay);
            if (humanAI)
                humanAI->InitAnimations();
            showaidebug |= 0x100;
        }
    }
    else if (wounded && GetOverlay(overlay))
    {
        RemoveOverlay(overlay);
        if (humanAI)
            humanAI->InitAnimations();
        showaidebug &= ~0x100;
    }
}

void oCNpc::SetToMad(float time)
{
    mad_heal = static_cast<int>(time);
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (humanAI && humanAI->IsStanding() && !(showaidebug & 0x80))
    {
        zSTRING overlay("_mad");
        if (GetOverlay(overlay))
        {
            ApplyOverlay(overlay);
            humanAI->InitAnimations();
            showaidebug |= 0x80;
        }
    }
}

void oCNpc::HealFromMad()
{
    if (!(showaidebug & 0x80))
        return;

    zSTRING overlay("_mad");
    if (GetOverlay(overlay))
    {
        RemoveOverlay(overlay);
        oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
        if (humanAI)
            humanAI->InitAnimations();
        showaidebug &= ~0x80;
        mad_heal = 0;
    }
}

void oCNpc::SetToDrunk(float time)
{
}

void oCNpc::HealFromDrunk()
{
}

void oCNpc::CheckAngryTime()
{
    if (tmp_attitude == attitude)
        return;

    if (attTimer <= 0.0f)
        attTimer = 60000.0f;
    attTimer -= ztimer.frameTimeFloat;
    if (attTimer > 0.0f)
        return;

    SetTmpAttitude(attitude);
    attTimer = 0.0f;
}

int oCNpc::CanRecruitSC()
{
    return 0;
}

int oCNpc::IsDisguised()
{
    return guild != guildTrue;
}

int oCNpc::GetGuild() const
{
    return guild;
}

void oCNpc::SetGuild(int value)
{
    guild = value;
}

void oCNpc::SetTrueGuild(int value)
{
    guildTrue = value;
}

int oCNpc::GetCamp()
{
    return 0;
}

int oCNpc::IsGuildFriendly(int otherGuild)
{
    return ogame->GetGuilds()->GetAttitude(guildTrue, otherGuild) == 3;
}

int oCNpc::GetGuildAttitude(int otherGuild)
{
    return ogame->GetGuilds()->GetAttitude(guildTrue, otherGuild);
}

void oCNpc::CreateItems(int instance, int amount)
{
    oCItem* item = dynamic_cast<oCItem*>(ogame->GetWorld()->CreateVob(zVOB_TYPE_ITEM, instance));
    if (!item)
        return;

    if (item->MultiSlot())
    {
        item->amount = amount;
        PutInInv(item);
        parser.SetInstance("ITEM", item);
        item->Release();
        return;
    }

    PutInInv(item);
    parser.SetInstance("ITEM", item);
    item->Release();
    for (int i = 1; i < amount; ++i)
    {
        item = dynamic_cast<oCItem*>(ogame->GetWorld()->CreateVob(zVOB_TYPE_ITEM, instance));
        PutInInv(item);
        item->Release();
    }
}

void oCNpc::SetFlag(int flag)
{
    variousFlags &= flag;
}

int oCNpc::HasFlag(int flag) const
{
    return (variousFlags & flag) != 0;
}

oCNpcTalent* oCNpc::GetTalent(int index, int create)
{
    if (index < 0 || index > oCNpcTalent::NPC_TAL_MAX || index >= talents.numInArray)
        return 0;
    if (!talents[index] && create)
    {
        talents[index] = new oCNpcTalent;
        zCClassDef::ObjectCreated(talents[index], oCNpcTalent::classDef);
        talents[index]->m_talent = static_cast<oCNpcTalent::oTEnumNpcTalent>(index);
        talents[index]->m_skill = 0;
        talents[index]->m_value = 0;
    }
    return talents[index];
}

void oCNpc::SetTalentValue(int index, int value)
{
    oCNpcTalent* talent = GetTalent(index, 1);
    if (talent)
        talent->m_value = value;
}

int oCNpc::GetTalentValue(int index)
{
    if (index < 0 || index > oCNpcTalent::NPC_TAL_MAX || index >= talents.numInArray
        || !talents[index])
        return -1;
    return talents[index]->m_value;
}

void oCNpc::SetTalentSkill(int index, int value)
{
    if (index < 0 || index > 0x16 || index >= talents.numInArray)
        return;
    oCNpcTalent* talent = GetTalent(index, 1);
    if (!talent)
        return;

    int oldSkill = talent->m_skill;
    talent->m_skill = value;
    if (value > oldSkill)
    {
        int fightMode = 0;
        switch (index)
        {
        case 0: fightMode = 3; break;
        case 1: fightMode = 4; break;
        case 2: fightMode = 5; break;
        case 3: fightMode = 6; break;
        }
        if (fightMode)
        {
            zSTRING weaponName = oCAniCtrl_Human::GetWeaponHitString(fightMode);
            if (zoptions->ReadBool("ENGINE", "zCacheInAllNSCAtNewGame", 0) || !ogame->inScriptStartup)
            {
                zCModel* model = GetModel();
                zSTRING modelName = "HUMANS";
                if (model && !model->modelProtoName.IsEmpty())
                {
                    modelName = model->modelProtoName;
                    int extension = modelName.Search(".MDS");
                    if (extension >= 0)
                        modelName = modelName.Left(extension);
                }
                zSTRING number;
                if (value >= 2)
                {
                    number = zSTRING(value - 1);
                    RemoveOverlay(modelName + "_" + weaponName + "T" + number + ".MDS");
                }
                number = zSTRING(value);
                ApplyOverlay(modelName + "_" + weaponName + "T" + number + ".MDS");
                if (human_ai)
                {
                    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
                    humanAI->InitFightAnis(fightMode);
                    if (fmode < 0 || fmode >= 8)
                        fmode = 0;
                    if (fightMode == fmode)
                        humanAI->SetFightAnis(fightMode);
                }
            }
        }
        else if (index == oCNpcTalent::NPC_TAL_ACROBAT)
        {
            ApplyOverlay("HUMANS_ACROBATIC.MDS");
            if (human_ai)
                static_cast<oCAIHuman*>(human_ai)->InitAnimations();
            SetFallDownHeight(GetFallDownHeight() * 2.0f);
        }
        if (IsAPlayer())
            zmusic->PlayJingleByScript("JINGLELEARNTALENT");
    }
    else if (value < oldSkill && index == oCNpcTalent::NPC_TAL_ACROBAT)
    {
        RemoveOverlay("HUMANS_ACROBATIC.MDS");
        if (human_ai)
            static_cast<oCAIHuman*>(human_ai)->InitAnimations();
        SetFallDownHeight(GetFallDownHeight() * 0.5f);
    }
}

int oCNpc::GetTalentSkill(int index)
{
    if (index < 0 || index > oCNpcTalent::NPC_TAL_MAX || index >= talents.numInArray
        || !talents[index])
        return -1;
    return talents[index]->m_skill;
}

int oCNpc::HasTalent(int index, int skill)
{
    return GetTalentSkill(index) >= index;
}

int oCNpc::CanUse(oCItem* item)
{
    if (!item)
        return 0;

    if (GetTalentSkill(oCNpcTalent::NPC_TAL_MAGE) < item->mag_circle)
    {
        parser.SetInstance("SELF", this);
        parser.SetInstance("ITEM", item);
        parser.CallFunc("G_CANNOTCAST");
        return 0;
    }

    for (int i = 0; i < 3; ++i)
    {
        if (item->cond_atr[i] > 0 && attribute[item->cond_atr[i]] < item->cond_value[i])
        {
            parser.SetInstance("SELF", this);
            parser.SetInstance("ITEM", item);
            parser.CallFunc("G_CANNOTUSE");
            return 0;
        }
    }
    return 1;
}

void oCNpc::DisplayCannotUse()
{
    oCMsgConversation* message = new oCMsgConversation;
    message->subType = 5;
    message->text = "$SC_CANTUSEITEM";
    GetEM(0)->OnMessage(message, this);
}

void oCNpc::CheckAllCanUse()
{
    static int recursion;
    if (recursion)
        return;

    recursion = 1;
    for (int i = invSlot.numInArray - 1; i >= 0; --i)
    {
        oCItem* item = dynamic_cast<oCItem*>(invSlot[i]->vob);
        if (item && item->HasFlag(0x40000000) && !CanUse(item))
        {
            UnequipItem(item);
            DisplayCannotUse();
        }
    }

    for (zCList<oCItem>* entry = inventory2.GetContents()->next; entry; entry = entry->next)
    {
        oCItem* item = entry->data;
        if (item && item->HasFlag(0x40000000) && !CanUse(item))
        {
            UnequipItem(item);
            DisplayCannotUse();
        }
    }
    recursion = 0;
}

oCVob* oCNpc::GetSlotVob(const zSTRING& nodeName)
{
    for (int i = 0; i < invSlot.numInArray; ++i)
    {
        if (invSlot[i]->name == nodeName)
            return invSlot[i]->vob;
    }
    return 0;
}

oCItem* oCNpc::GetSlotItem(const zSTRING& nodeName)
{
    for (int i = 0; i < invSlot.numInArray; ++i)
    {
        if (invSlot[i]->name == nodeName)
            return dynamic_cast<oCItem*>(invSlot[i]->vob);
    }
    return 0;
}

oCItem* oCNpc::GetItem(int instance, int category)
{
    return inventory2.GetItem(category);
}

oCNpc* oCNpc::GetSlotNpc(const zSTRING& nodeName)
{
    for (int i = 0; i < invSlot.numInArray; ++i)
    {
        if (invSlot[i]->name == nodeName)
            return dynamic_cast<oCNpc*>(invSlot[i]->vob);
    }
    return 0;
}

void oCNpc::AddItemEffects(oCItem* item)
{
    if (!item)
        return;

    for (int i = 0; i < 3; ++i)
    {
        if (item->change_atr[i] > 0)
            ChangeAttribute(item->change_atr[i], item->change_value[i]);
    }
    for (int i = 0; i < 8; ++i)
        protection[i] += item->protection[i];

    if (item->disguiseGuild && item->disguiseGuild != guildTrue)
        guild = item->disguiseGuild;
    if (item->on_equip > 0)
    {
        parser.SetInstance("SELF", this);
        parser.CallFunc(item->on_equip);
    }
    CheckAllCanUse();
}

void oCNpc::RemoveItemEffects(oCItem* item)
{
    if (!item)
        return;

    for (int i = 0; i < 3; ++i)
    {
        if (item->change_atr[i] > 0)
            ChangeAttribute(item->change_atr[i], -item->change_value[i]);
    }
    for (int i = 0; i < 8; ++i)
        protection[i] -= item->protection[i];

    if (item->disguiseGuild)
        guild = guildTrue;
    if (item->on_unequip > 0)
    {
        parser.SetInstance("SELF", this);
        parser.CallFunc(item->on_unequip);
    }
    CheckAllCanUse();
}

static zSTRING GetNpcEquipmentSlot(oCItem* item)
{
    if (item->HasFlag(0x4000) || item->HasFlag(0x2000) || item->HasFlag(0x8000))
        return zSTRING("ZS_SWORD");
    if (item->HasFlag(0x10000) || item->HasFlag(0x20000))
        return zSTRING("ZS_LONGSWORD");
    if (item->HasFlag(0x40000))
        return zSTRING("ZS_SHIELD");
    if (item->HasFlag(0x80000))
        return zSTRING("ZS_BOW");
    if (item->HasFlag(0x100000))
        return zSTRING("ZS_CROSSBOW");
    if (item->wear & 2)
        return zSTRING("ZS_HELMET");
    if (item->wear & 1)
        return zSTRING("ZS_TORSO");
    return zSTRING();
}

void oCNpc::EquipItem(oCItem* item)
{
    if (!item || item->HasFlag(0x40000000))
        return;

    if (item->SplitSlot() && item->amount > 1)
    {
        oCItem* remaining = item->SplitItem(item->amount - 1);
        if (remaining)
        {
            item->SetFlag(0x40000000);
            item->amount = 1;
            inventory2.Insert(remaining);
            remaining->Release();
            item->ClearFlag(0x40000000);
        }
    }

    AddItemEffects(item);
    item->SetFlag(0x40000000);
    zSTRING slotName = GetNpcEquipmentSlot(item);
    if (!slotName.IsEmpty())
    {
        for (int i = 0; i < invSlot.numInArray; ++i)
        {
            if (invSlot[i]->name == slotName)
            {
                PutInSlot(invSlot[i], item, 1);
                break;
            }
        }
    }
}

void oCNpc::UnequipItem(oCItem* item)
{
    if (!item || !item->HasFlag(0x40000000))
        return;

    RemoveItemEffects(item);
    item->ClearFlag(0x40000000);
    zSTRING slotName = GetNpcEquipmentSlot(item);
    if (slotName.IsEmpty())
        return;
    TNpcSlot* slot = GetInvSlot(slotName);
    if (!RemoveFromSlot(slot, 1, 1) || !item->SplitSlot())
        return;
    for (zCList<oCItem>* entry = inventory2.GetContents()->next; entry; entry = entry->next)
    {
        oCItem* other = entry->data;
        if (!other || !other->SplitSlot() || other->HasFlag(0x40000000) || other == item
            || other->GetInstance() != item->GetInstance())
            continue;
        other->amount += item->amount;
        inventory2.Remove(item);
        if (item->refCtr >= 2)
            ogame->GetWorld()->RemoveVob(item);
        break;
    }
}

int oCNpc::GetCutsceneDistance()
{
    zCAICamera* camera = zCAICamera::GetCurrent();
    if (camera && camera->camVob)
        return static_cast<int>(GetDistanceToVobApprox(*camera->camVob));
    zCVob* playerVob = ogame->GetSelfPlayerVob();
    return playerVob ? static_cast<int>(GetDistanceToVobApprox(*playerVob)) : 0;
}

void oCNpc::SetCSEnabled(int enabled)
{
    if (enabled)
        showaidebug |= 4;
    else
        showaidebug &= ~4;
}

int oCNpc::GetCSStateFlags()
{
    int result = attribute[0] <= 0;
    static const int blockingSpells[] = { 33, 71, 82, 39, 37, 86 };
    for (zCList<oCSpell>* entry = activeSpells.next; entry; entry = entry->next)
    {
        for (int i = 0; i < 6; ++i)
        {
            if (entry->data->spellID == blockingSpells[i])
                result = 1;
        }
    }
    if (!(showaidebug & 4))
        result |= 1;
    return result;
}

void oCNpc::SetFocusVob(zCVob* vob)
{
    if (focus_vob == vob)
        return;
    ClearFocusVob();
    focus_vob = vob;
    if (vob)
        vob->AddRef();
}

void oCNpc::ClearFocusVob()
{
    if (focus_vob)
    {
        focus_vob->Release();
        focus_vob = 0;
    }
}

zCVob* oCNpc::GetFocusVob()
{
    return focus_vob;
}

oCNpc* oCNpc::GetFocusNpc()
{
    return dynamic_cast<oCNpc*>(focus_vob);
}

oCNpc* oCNpc::GetNearestFightNpcLeft()
{
    oCNpc* result = 0;
    float bestAngle = -360.0f;
    zCArray<zCVob*> vobs;
    CreateVobList(vobs, GetFightRange() * 2);
    for (int i = 0; i < vobs.numInArray; ++i)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(vobs[i]);
        if (!npc || npc == this || npc == GetFocusNpc())
            continue;
        float azimuth;
        float elevation;
        GetAngles(npc, azimuth, elevation);
        if (azimuth >= 0.0f)
            azimuth = -180.0f - azimuth;
        if (azimuth > -360.0f && azimuth <= 0.0f && azimuth > bestAngle)
        {
            bestAngle = azimuth;
            result = npc;
        }
    }
    return result;
}

oCNpc* oCNpc::GetNearestFightNpcRight()
{
    oCNpc* result = 0;
    float bestAngle = 360.0f;
    zCArray<zCVob*> vobs;
    CreateVobList(vobs, GetFightRange() * 2);
    for (int i = 0; i < vobs.numInArray; ++i)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(vobs[i]);
        if (!npc || npc == this || npc == GetFocusNpc())
            continue;
        float azimuth;
        float elevation;
        GetAngles(npc, azimuth, elevation);
        if (azimuth <= 0.0f)
            azimuth = 180.0f - azimuth;
        if (azimuth >= 0.0f && azimuth < 360.0f && azimuth < bestAngle)
        {
            bestAngle = azimuth;
            result = npc;
        }
    }
    return result;
}

int oCNpc::FocusCheckBBox(const zCVob* vob, int vobType, int flags, float maxAngle,
    float& bestAngle)
{
    zTBBox3D box = vob->GetBBox3DLocal();
    float radiusX = (box.maxs.n[0] - box.mins.n[0]) * 0.5f;
    float radiusZ = (box.maxs.n[2] - box.mins.n[2]) * 0.5f;
    float radius = radiusZ > radiusX ? radiusZ : radiusX;
    float distance = GetDistanceToVob2(*const_cast<zCVob*>(vob)) - radius * radius;
    if (distance < 0.0f)
        distance = 0.0f;
    if (!oCNpcFocus::focus->IsInRange(vobType, distance))
        return 0;

    for (int i = 0; i < 2; ++i)
    {
        zVEC3 position = vob->GetPositionWorld();
        position.n[1] += (box.maxs.n[1] + box.mins.n[1]) * 0.5f;
        if (i)
            position.n[1] += 180.0f;
        float azimuth;
        float elevation;
        GetAngles(position, azimuth, elevation);
        if (!oCNpcFocus::focus->IsInAngle(vobType, azimuth, elevation))
            continue;
        int priority = oCNpcFocus::focus->GetPriority(vobType);
        if (priority == oCNpcFocus::focus->GetPriority(flags)
            && fabs(azimuth) >= fabs(bestAngle))
            continue;
        if (FreeLineOfSight(position, const_cast<zCVob*>(vob)))
        {
            bestAngle = azimuth;
            return 1;
        }
    }
    return 0;
}

int oCNpc::FocusCheck(const zCVob* vob, int vobType, int flags, float maxAngle,
    float& bestAngle)
{
    zTBBox3D box = vob->GetBBox3DLocal();
    float radiusX = (box.maxs.n[0] - box.mins.n[0]) * 0.5f;
    float radiusZ = (box.maxs.n[2] - box.mins.n[2]) * 0.5f;
    float radius = radiusZ > radiusX ? radiusZ : radiusX;
    float distance = GetDistanceToVob2(*const_cast<zCVob*>(vob)) - radius * radius;
    if (distance < 0.0f)
        distance = 0.0f;
    if (!oCNpcFocus::focus->IsInRange(vobType, distance))
        return 0;

    zVEC3 position = vob->GetPositionWorld();
    position.n[1] += (box.maxs.n[1] + box.mins.n[1]) * 0.5f;
    if (vobType == zVOB_TYPE_NSC)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(const_cast<zCVob*>(vob));
        if (!npc || (npc->guild != 0x2D && npc->guild != 0x2F))
            position.n[1] += (box.maxs.n[1] - box.mins.n[1]) * 0.25f;
    }
    float azimuth;
    float elevation;
    GetAngles(position, azimuth, elevation);
    if (!oCNpcFocus::focus->IsInAngle(vobType, azimuth, elevation))
        return 0;
    int priority = oCNpcFocus::focus->GetPriority(vobType);
    if (priority == oCNpcFocus::focus->GetPriority(flags)
        && fabs(azimuth) >= fabs(bestAngle))
        return 0;
    if (!FreeLineOfSight(position, const_cast<zCVob*>(vob)))
        return 0;
    bestAngle = azimuth;
    return 1;
}

void oCNpc::ToggleFocusVob(int mode)
{
    CollectFocusVob(mode);
}

void oCNpc::CollectFocusVob(int mode)
{
    if (fmode < 0 || fmode >= 8)
        fmode = 0;
    if (s_bTargetLocked)
    {
        oCNpc* focusNpc = GetFocusNpc();
        if (focus_vob && (!focusNpc || focusNpc->attribute[0] > 0)
            && GetDistanceToVob2(*focus_vob) <= 4000000.0f)
            return;
        s_bTargetLocked = 0;
    }
    if (!mode && pickvobdelay > 0.0f)
    {
        pickvobdelay -= ztimer.frameTimeFloat;
        return;
    }
    pickvobdelay = (fmode >= 2 && fmode <= 4) ? 100.0f : 250.0f;
    SetFocusVob(0);
    SetEnemy(0);
    ClearVobList();
    CreateVobList(vobList, oCNpcFocus::focus->GetMaxRange());

    zCArray<oCNpc*> fallenNpcs;
    float bestAngle = 181.0f;
    int bestType = 0;
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        zCVob* vob = vobList[i];
        if (!vob || vob == this || !oCNpcFocus::focus->IsValid(vob->type))
            continue;
        oCNpc* npc = dynamic_cast<oCNpc*>(vob);
        if (npc)
        {
            if (fmode >= 2 && fmode <= 4)
            {
                if (npc->state.IsInState(-4))
                {
                    fallenNpcs.InsertEnd(npc);
                    continue;
                }
                if (npc->attribute[0] <= 0)
                    continue;
            }
            else if (npc->attribute[0] <= 0)
                continue;
        }
        if (fmode == 7 && mag_book)
        {
            oCSpell* spell = mag_book->GetSelectedSpell();
            if (spell && !spell->IsValidTarget(vob))
                continue;
        }
        int accepted = vob->CastTo<oCVob>()
            ? FocusCheckBBox(vob, vob->type, bestType, bestAngle, bestAngle)
            : FocusCheck(vob, vob->type, bestType, bestAngle, bestAngle);
        if (accepted)
        {
            bestType = vob->type;
            SetFocusVob(vob);
            SetEnemy(npc);
        }
    }
    if (!focus_vob && fallenNpcs.numInArray > 0)
    {
        oCNpc* bestNpc = 0;
        float bestScore = 3.402823466e+38f;
        float range = GetFightRange();
        if (range <= 0.0f)
            range = 1.0f;
        float focusAzi = oCNpcFocus::focus->GetAzi();
        for (int i = 0; i < fallenNpcs.numInArray; ++i)
        {
            oCNpc* npc = fallenNpcs[i];
            float azimuth;
            float elevation;
            GetAngles(npc, azimuth, elevation);
            float angleScore = azimuth == 0.0f ? 1.0f : azimuth / focusAzi;
            float score = GetDistanceToVob2(*npc) / range * 8.0f + angleScore * 2.0f;
            if (score < bestScore)
            {
                bestScore = score;
                bestNpc = npc;
            }
        }
        if (bestNpc)
        {
            SetFocusVob(bestNpc);
            SetEnemy(bestNpc);
        }
    }
}

void oCNpc::GetNearestValidVob(float maxDistance)
{
    zCVob* bestVob = 0;
    float bestDistance = maxDistance * maxDistance;
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        zCVob* vob = vobList[i];
        if (!vob || vob == this || !oCNpcFocus::focus->IsValid(vob->type))
            continue;
        float distance = GetDistanceToVob2(*vob);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestVob = vob;
        }
    }

    SetEnemy(0);
    if (bestVob)
    {
        SetFocusVob(bestVob);
        if (bestVob->type == zVOB_TYPE_NSC)
            SetEnemy(dynamic_cast<oCNpc*>(bestVob));
    }
}

void oCNpc::GetNearestVob(float maxDistance)
{
    zCVob* bestVob = 0;
    float bestDistance = maxDistance * maxDistance;
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        zCVob* vob = vobList[i];
        if (!vob || vob == this)
            continue;
        float distance = GetDistanceToVob2(*vob);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestVob = vob;
        }
    }

    SetEnemy(0);
    if (bestVob)
    {
        SetFocusVob(bestVob);
        if (bestVob->type == zVOB_TYPE_NSC)
            SetEnemy(dynamic_cast<oCNpc*>(bestVob));
    }
}

oCAniCtrl_Human* oCNpc::GetAnictrl()
{
    return static_cast<oCAIHuman*>(human_ai);
}

void oCNpc::ResetToHumanAI()
{
    if (callback_ai == human_ai)
        return;
    oCAIHuman* oldAI = static_cast<oCAIHuman*>(human_ai);
    if (oldAI)
        oldAI->Release();
    human_ai = dynamic_cast<oCAIHuman*>(callback_ai);
}

static int NpcHasBlockingSpell(oCNpc* npc)
{
    static const int blockingSpells[] = { 33, 71, 82, 39, 37, 86 };
    for (zCList<oCSpell>* entry = npc->activeSpells.next; entry; entry = entry->next)
    {
        for (int i = 0; i < 6; ++i)
        {
            if (entry->data->spellID == blockingSpells[i])
                return 1;
        }
    }
    return 0;
}

void oCNpc::SetEnemy(oCNpc* other)
{
    if (enemy == other && other && other->attribute[0] > 0 && !NpcHasBlockingSpell(other))
        return;
    if (enemy)
        enemy->Release();
    enemy = 0;
    if (other && other != this && other->attribute[0] > 0 && !NpcHasBlockingSpell(other))
    {
        enemy = other;
        enemy->AddRef();
    }
}

oCNpc* oCNpc::GetNextEnemy()
{
    if (enemy && enemy->attribute[0] > 0 && !NpcHasBlockingSpell(enemy))
        return enemy;

    oCNpc* result = 0;
    float bestDistance = 3.402823466e+38f;
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(vobList[i]);
        if (!npc || npc == this || npc->attribute[0] <= 0 || NpcHasBlockingSpell(npc)
            || npc->state.IsInState(-4) || npc->state.IsInState(-5))
            continue;
        zCCutscene* cutscene = npc->GetEM(0)->GetCutscene();
        if (cutscene && cutscene->IsPlaying())
            continue;
        int targetAttitude = GetAttitude(npc);
        if (IsAPlayer() && npc->IsAPlayer() && tmp_attitude != attitude)
            targetAttitude = tmp_attitude;
        if (targetAttitude != 0)
            continue;
        float distance = GetDistanceToVob2(*npc);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            result = npc;
        }
    }
    SetEnemy(result);
    return result;
}

int oCNpc::IsConditionValid()
{
    return attribute[0] > 0 && !NpcHasBlockingSpell(this);
}

int oCNpc::IsInFightFocus(zCVob* vob)
{
    if (!vob)
        return 0;
    float azimuth;
    float elevation;
    GetAngles(vob, azimuth, elevation);
    if (azimuth < 0.0f)
        azimuth = -azimuth;
    if (elevation < 0.0f)
        elevation = -elevation;
    return azimuth < 30.0f && elevation < 50.0f && GetDistanceToVob2(*vob) < 9000000.0f;
}

oCItem* oCNpc::HasEquippedStolenItem(oCNpc* other)
{
    for (int i = 0; i < invSlot.numInArray; ++i)
    {
        oCItem* item = dynamic_cast<oCItem*>(invSlot[i]->vob);
        if (item && item->IsOwnedByNpc(other->GetInstance()))
            return item;
    }
    return 0;
}

void oCNpc::Burn(int damage, float time)
{
    if (damage > 0 && !fireVob)
    {
        zCParticleFX* particle = new zCParticleFX;
        fireVob = new zCVob;
        fireVob->SetVobName("Particle_Test_Vob");
        fireVob->SetVisual(particle);
        particle->SetAndStartEmitter("FIRE", 0);
        fireVob->SetCollDetStat(0);
        fireVob->SetCollDetDyn(0);
        zVEC3 position;
        GetPositionWorld(position.n[0], position.n[1], position.n[2]);
        fireVob->SetPositionWorld(position);
        homeWorld->AddVobAsChild(fireVob, this);
        particle->Release();
    }

    if (damage > fireDamage)
    {
        fireDamageTimer = 0.0f;
        fireDamage = damage;
    }
    bodyState |= 0x400;
}

void oCNpc::StopBurn()
{
    if (fireVob && homeWorld)
    {
        zCParticleFX* smoke = new zCParticleFX;
        zCVob* smokeVob = new zCVob;
        smokeVob->SetVisual(smoke);
        smoke->SetAndStartEmitter("FIRE_SMOKE_ONCE", 0);
        smokeVob->SetCollDetStat(0);
        smokeVob->SetCollDetDyn(0);
        zVEC3 position;
        GetPositionWorld(position.n[0], position.n[1], position.n[2]);
        smokeVob->SetPositionWorld(position);
        homeWorld->AddVobAsChild(smokeVob, this);
        smoke->Release();
        smokeVob->Release();

        fireDamageTimer = 0.0f;
        fireDamage = 0;
        fireVob->Release();
        fireVob = 0;
    }
    bodyState &= ~0x400;
}

zVEC3 RotateVec(zVEC3& vector, zVEC3& axis, float angle, float scale)
{
    float length = std::sqrt(axis.n[0] * axis.n[0] + axis.n[1] * axis.n[1]
        + axis.n[2] * axis.n[2]);
    float x = axis.n[0] / length;
    float y = axis.n[1] / length;
    float z = axis.n[2] / length;
    float cosine = std::cos(angle);
    float sine = std::sin(angle);
    float dot = x * vector.n[0] + y * vector.n[1] + z * vector.n[2];
    zVEC3 result;
    result.n[0] = (vector.n[0] * cosine + (y * vector.n[2] - z * vector.n[1]) * sine
        + x * dot * (1.0f - cosine)) * scale;
    result.n[1] = (vector.n[1] * cosine + (z * vector.n[0] - x * vector.n[2]) * sine
        + y * dot * (1.0f - cosine)) * scale;
    result.n[2] = (vector.n[2] * cosine + (x * vector.n[1] - y * vector.n[0]) * sine
        + z * dot * (1.0f - cosine)) * scale;
    return result;
}

void oCNpc::Interrupt(int flags, int stopAI)
{
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (!humanAI)
        return;

    if (humanAI->actionMode != 5 && humanAI->actionMode != 6)
        humanAI->SetActionMode(0);
    humanAI->HitInterrupt();
    if (mag_book && (stopAI || !IsAPlayer()))
    {
        oCSpell* spell = mag_book->GetSelectedSpell();
        if (spell && spell->effect && spell->effect->fxState == zVFXSTATE_INVESTNEXT)
            mag_book->StopSelectedSpell();
        if (fmode < 0 || fmode >= 8)
            fmode = 0;
        if (fmode != 7)
            mag_book->Close(1);
    }
    if (interactMob && (!flags || (bodyState & 0x8000)))
    {
        GetInteractMob()->InterruptInteraction(this);
        SetInteractMob(0);
        humanAI->SearchStandAni(0);
    }
    if (interactItem)
    {
        SetInteractItem(0);
        humanAI->SearchStandAni(0);
        interactItemCurrentState = -1;
    }
    zCModel* model = GetModel();
    if (model)
    {
        model->StopAnisLayerRange(2, 1000);
        model->SetModelScale(zVEC3(1.0f, 1.0f, 1.0f));
        zCModelNode* head = model->SearchNode(zMDL_NODE_NAME_HEAD);
        if (head)
        {
            zCMorphMesh* morph = dynamic_cast<zCMorphMesh*>(head->nodeVisual);
            if (morph)
                morph->StopAni(NPC_TALK_ANI);
        }
    }
    StopTalkingWith();
    StopAllVoices();
    if (state.nextState.name == "ZS_ASSESSGIVENITEM")
        CloseTradeContainer();
    RbtReset();
    humanAI->ResetAutoRoll();
    if (humanAI->wmode != 1)
    {
        humanAI->wmode = 1;
        StandUp(1, 1);
    }
}

int oCNpc::GetWeaponDamage(oCItem* item)
{
    if (!item)
        return 0;

    int result = 0;
    for (int i = 0; i < 8; ++i)
    {
        int damage = item->damage[i] - protection[i];
        if (damage > 0)
            result += damage;
    }
    return result;
}

int oCNpc::GetBluntDamage(oCItem* item)
{
    return item ? item->damage[1] - protection[1] : 0;
}

void oCNpc::DropUnconscious(float time, oCNpc* attacker)
{
    if (state.IsInState(-4))
        return;
    if (IsAPlayer())
    {
        if (game_mode == 2 || game_mode == 1)
            StopTheft(this, 0);
        oCMobContainer* container = dynamic_cast<oCMobContainer*>(interactMob);
        if (container)
            container->Close(this);
        CloseInventory();
        showaidebug &= ~0x20;
    }
    if (visual)
    {
        zCModelNode* head = GetModel()->SearchNode(zMDL_NODE_NAME_HEAD);
        if (head)
        {
            zCMorphMesh* morph = dynamic_cast<zCMorphMesh*>(head->nodeVisual);
            if (morph)
                morph->StopAni(NPC_TALK_ANI);
        }
    }
    DropAllInHand();
    SetBodyState(0);
    attribute[0] = 1;
    CheckModelOverlays();
    bodyState = (bodyState & ~0x7FFE9) | 0x16;
    parser.SetInstance("OTHER", attacker);
    state.StartAIState(-4, 0, 0, 0.0f, 0);
    oCMsgConversation* message = new oCMsgConversation;
    message->subType = 14;
    message->text = time > 90.0f ? "T_STAND_2_WOUNDED" : "T_STAND_2_WOUNDEDB";
    GetEM(0)->OnMessage(message, this);
    if (attacker)
        CreatePassivePerception(7, attacker, this);
    if (IsAPlayer())
    {
        if (human_ai)
            static_cast<oCAIHuman*>(human_ai)->SetCamMode(CamModNormal, 0);
        oCNpcFocus::SetFocusMode(0);
        player->CloseInventory();
    }
}

void oCNpc::CheckUnconscious()
{
    if (!unconscious)
        return;

    zCModel* model = dynamic_cast<zCModel*>(visual);
    if (!model)
        return;

    static const char* recoveryAnis[] = {
        "T_STAND_2_WOUNDEDB", "T_STAND_2_WOUNDED", "S_WOUNDEDB", "S_WOUNDED",
        "T_WOUNDEDB_2_DEADB", "T_WOUNDED_2_DEAD", "T_WOUNDED_TRY", "T_WOUNDEDB_TRY"
    };
    for (int i = 0; i < 8; ++i)
    {
        if (model->IsAniActive(model->GetAniIDFromAniName(recoveryAnis[i])))
            return;
    }

    oCMsgConversation* message = new oCMsgConversation;
    message->subType = 14;
    message->text = "T_STAND_2_WOUNDEDB";
    GetEM(0)->OnMessage(message, this);
}

zSTRING oCNpc::GetInterruptPrefix()
{
    if (interactMob)
        return static_cast<oCMobInter*>(interactMob)->GetScemeName();
    return zSTRING();
}

void oCNpc::CompleteHeal()
{
    attribute[0] = attribute[1];
    attribute[2] = attribute[3];
}

void oCNpc::DoDie(oCNpc* attacker)
{
    if (IsAPlayer())
    {
        if (game_mode == 2 || game_mode == 1)
            StopTheft(this, 0);
        oCMobContainer* container = dynamic_cast<oCMobContainer*>(interactMob);
        if (container)
            container->Close(this);
        CloseInventory();
        showaidebug &= ~0x20;
    }
    Interrupt(0, 1);
    if (visual)
    {
        zCModelNode* head = GetModel()->SearchNode(zMDL_NODE_NAME_HEAD);
        if (head)
        {
            zCMorphMesh* morph = dynamic_cast<zCMorphMesh*>(head->nodeVisual);
            if (morph)
                morph->StopAni(NPC_TALK_ANI);
        }
    }
    if (IsAPlayer())
    {
        if (human_ai)
            static_cast<oCAIHuman*>(human_ai)->SetCamMode(CamModDeath, 0);
        SetFocusVob(0);
    }
    DropAllInHand();
    SetBodyState(0);
    StopBurn();
    if (human_ai)
        static_cast<oCAIHuman*>(human_ai)->StopTurnAnis();
    bodyState = (bodyState & ~0x7FFE8) | 0x17;
    memset(percList, 0, sizeof(percList));
    percActive = 0;
    if (!state.IsInState(-3))
    {
        parser.SetInstance("OTHER", attacker);
        state.StartAIState(-3, 0, 0, 0.0f, 0);
        parser.SetInstance("OTHER", attacker);
    }
    SetPhysicsEnabled(1);
    if (attacker)
        CreatePassivePerception(6, attacker, this);
    if (showaidebug & 0x10)
    {
        oCNpc* spawned = ogame->GetSpawnManager()->SpawnNpc(GetInstance(), spawnPoint,
            spawnDelay * 250000.0f);
        if (spawned)
        {
            spawned->showaidebug |= 0x10;
            spawned->spawnDelay = spawnDelay;
            spawned->showaidebug &= ~0x2000;
        }
        showaidebug &= ~0x10;
    }
    attribute[0] = 0;
    CheckModelOverlays();
}

void oCNpc::StartFadeAway()
{
    SetCollDetDyn(0);
    SetSleeping(0);
    ClearFocusVob();
    vobList.DeleteList();
    fadingAway = 1;
    fadeAwayTime = 5000.0f;
    respawnTime = spawnDelay * 1000.0f;
}

int oCNpc::IsFadingAway()
{
    return fadingAway;
}

int oCNpc::FadeAway()
{
    fadeAwayTime -= ztimer.frameTimeFloat;
    showVisual |= 4;
    if (fadeAwayTime <= 0.0f)
    {
        visualAlpha = 0.0f;
        if (homeWorld)
            homeWorld->RemoveVob(this);
        return 1;
    }
    visualAlpha = fadeAwayTime / 5000.0f;
    return 0;
}

int oCNpc::BeamTo(const zSTRING& destination)
{
    zCWaypoint* waypoint = ogame->GetWorld()->wayNet->GetWaypoint(destination);
    zCVob* target = waypoint ? 0 : ogame->GetWorld()->SearchVobByName(destination);
    if (!waypoint && !target)
        return 0;
    zVEC3 position = waypoint ? waypoint->GetPositionWorld() : target->GetPositionWorld();
    zVEC3 direction = waypoint ? waypoint->dir : zVEC3(target->trafoObjToWorld.m[0][2],
        target->trafoObjToWorld.m[1][2], target->trafoObjToWorld.m[2][2]);
    zCPar_Symbol* selfSymbol = parser.GetSymbol("SELF");
    oCNpc* oldSelf = selfSymbol ? dynamic_cast<oCNpc*>(selfSymbol->GetInstanceAdr()) : 0;
    SetCollDetStat(0);
    SetCollDetDyn(0);
    SetPositionWorld(position);
    SetHeadingAtWorld(direction);
    SetCollDetStat(1);
    SetCollDetDyn(1);
    if (IsAPlayer())
    {
        zCAICamera* camera = zCAICamera::GetCurrent();
        if (camera)
        {
            camera->ClearTargetList();
            camera->SetTarget(player);
            if (player->variousFlags & 4)
                camera->targetAlpha = zoptions->ReadReal("INTERNAL", "GhostAlpha", 0.3f);
            camera->ReceiveMsg(0x8000);
            zCArray<zCVob*> targets;
            camera->SetMode(CamModNormal, targets);
        }
    }
    ogame->GetSpawnManager()->SpawnImmediately(0);
    parser.SetInstance("SELF", oldSelf);
    return 1;
}

void oCNpc::DropInventory()
{
}

int oCNpc::HasInHand(int instance)
{
    oCItem* right = dynamic_cast<oCItem*>(GetSlotVob("ZS_RIGHTHAND"));
    oCItem* left = dynamic_cast<oCItem*>(GetSlotVob("ZS_LEFTHAND"));
    return (right && right->GetInstance() == instance) || (left && left->GetInstance() == instance);
}

int oCNpc::HasInHand(zCVob* vob)
{
    return GetSlotVob("ZS_RIGHTHAND") == vob || GetSlotVob("ZS_LEFTHAND") == vob;
}

void oCNpc::DropAllInHand()
{
    static const char* slotNames[] = { "ZS_RIGHTHAND", "ZS_LEFTHAND" };
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < invSlot.numInArray; ++j)
        {
            TNpcSlot* slot = invSlot[j];
            if (slot->name == slotNames[i] && slot->vob)
            {
                if (homeWorld)
                    homeWorld->AddVob(slot->vob);
                slot->ClearVob();
            }
        }
    }

    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (attribute[0] > 0 && humanAI)
    {
        humanAI->SetWalkMode(0);
        zCModel* model = dynamic_cast<zCModel*>(visual);
        if (model)
            model->StartAni(humanAI->_s_walk, 0);
    }
}

oCItem* oCNpc::GetWeapon()
{
    if (fmode <= 1)
        return 0;
    if (fmode > 4 && fmode != 6)
        return GetSlotItem("ZS_LEFTHAND");
    return GetSlotItem("ZS_RIGHTHAND");
}

oCItem* oCNpc::GetEquippedMeleeWeapon()
{
    oCItem* item = GetSlotItem("ZS_SWORD");
    return item ? item : GetSlotItem("ZS_LONGSWORD");
}

oCItem* oCNpc::GetEquippedRangedWeapon()
{
    oCItem* item = GetSlotItem("ZS_BOW");
    return item ? item : GetSlotItem("ZS_CROSSBOW");
}

oCItem* oCNpc::GetEquippedArmor()
{
    return GetSlotItem("ZS_TORSO");
}

int oCNpc::IsSameHeight(zCVob* vob)
{
    if (!vob)
        return 0;
    zTBBox3D ownBox = GetBBox3DLocal();
    zTBBox3D otherBox = vob->GetBBox3DLocal();
    float ownHeight = (ownBox.maxs.n[1] - ownBox.mins.n[1]) * 0.25f;
    float otherHeight = (otherBox.maxs.n[1] - otherBox.mins.n[1]) * 0.25f;
    float ownY = trafoObjToWorld.m[1][3];
    float otherY = vob->trafoObjToWorld.m[1][3];
    return otherY + otherHeight >= ownY - ownHeight && otherY - otherHeight <= ownY + ownHeight;
}

void oCNpc::TurnToEnemy()
{
    if (!enemy)
        return;

    float azimuth;
    float elevation;
    GetAngles(enemy, azimuth, elevation);
    if (azimuth > -1.0f && azimuth < 1.0f)
        return;
    float step = GetTurnSpeed() * ztimer.frameTimeFloat * 4.0f;
    if (azimuth < 0.0f)
        step = -step;
    if ((step < 0.0f && step < azimuth) || (step > 0.0f && step > azimuth))
        step = azimuth;
    RotateLocalY(step);
}

int oCNpc::ShieldEquipped()
{
    for (int i = 0; i < invSlot.numInArray; ++i)
    {
        if (invSlot[i]->name == "ZS_SHIELD" && !invSlot[i]->vob)
            return 0;
    }
    return 1;
}

int oCNpc::GetWeaponDamage()
{
    oCItem* item = GetWeapon();
    return item ? item->GetFullDamage() : 0;
}

void oCNpc::SetMovLock(int locked)
{
    if (locked)
        showaidebug |= 0x20;
    else
        showaidebug &= ~0x20;
}

void oCNpc::SetHead()
{
    zCModel* model = GetModel();
    if (!model || head_visualName.IsEmpty())
        return;

    zCModelNodeInst* node = model->SearchNode(zMDL_NODE_NAME_HEAD);
    if (!node)
        return;
    zSTRING visualName = GetVisualHead();
    if (visualName.IsEmpty())
        return;
    zCMorphMesh* headVisual = zCMorphMesh::Load(visualName);
    model->SetNodeVisual(node, headVisual, 0);
    zSTRING headTexture = "HEAD";
    headVisual->SetTextureVar(0, head_TexVarNr, &headTexture);
    headTexture = "HEAD";
    headVisual->SetTextureVar(1, body_TexColorNr, &headTexture);
    zSTRING teethTexture = "TEETH";
    headVisual->SetTextureVar(0, teeth_TexVarNr, &teethTexture);
    if (headVisual)
        headVisual->Release();
}

void oCNpc::SetAdditionalVisuals(zSTRING& body, int bodyColor, int armorInstance,
    zSTRING& head, int headVariation, int teethVariation, int bodyVariation)
{
    body_visualName = body;
    head_visualName = head;
    head_visualName.Upper();
    body_TexColorNr = bodyColor;
    head_TexVarNr = headVariation;
    teeth_TexVarNr = teethVariation;

    if (ogame && !ogame->inLoadSaveGame && armorInstance > 0)
    {
        oCItem* armor = dynamic_cast<oCItem*>(ogame->GetWorld()->CreateVob(zVOB_TYPE_ITEM,
            armorInstance));
        if (armor && !(armor->flags & 1))
        {
            armor->Release();
            return;
        }
        if (armor)
        {
            PutInInv(armor);
            EquipItem(armor);
            armor->Release();
            return;
        }
    }
    if (visual)
        InitModel();
}

void oCNpc::InitModel()
{
    zCModel* model = GetModel();
    if (!model)
        return;

    zSTRING bodyVisual = body_visualName;
    int visualSkin = 0;
    oCItem* armor = GetSlotItem(NPC_NODE_TORSO);
    if (armor)
    {
        bodyVisual = armor->GetVisualChange();
        visualSkin = armor->visual_skin;
    }
    if (!bodyVisual.IsEmpty())
    {
        model->RemoveMeshLibAll();
        model->ApplyMeshLib(bodyVisual);
        model->SetMeshLibTexture("BODY", 0, 0);
        model->SetMeshLibTexture("BODY", 1, body_TexColorNr);
        model->SetMeshLibTexture("ARMOR", 0, visualSkin);
    }
    SetHead();
    showVisual = 1;
    selfDist = 150.0f;
}

zCModel* oCNpc::GetModel()
{
    zCModel* model = dynamic_cast<zCModel*>(visual);
    if (!model)
    {
        AvoidShrink(0);
        model = dynamic_cast<zCModel*>(visual);
    }
    return model;
}

zSTRING oCNpc::GetVisualBody()
{
    return body_visualName;
}

zSTRING oCNpc::GetVisualHead()
{
    return head_visualName;
}

int oCNpc::StartFaceAni(const zSTRING& aniName, float blendIn, float blendOut)
{
    zCModel* model = GetModel();
    if (!model)
        return 0;
    zCModelNodeInst* node = model->SearchNode("BIP01 HEAD");
    zCMorphMesh* morph = node ? dynamic_cast<zCMorphMesh*>(node->nodeVisual) : 0;
    if (!morph)
        return 0;
    morph->StartAni(aniName, blendIn, blendOut);
    return 1;
}

int oCNpc::StartStdFaceAni(int attitude)
{
    static const char* aniNames[] = { "S_HOSTILE", "S_ANGRY", "S_NEUTRAL", "S_FRIENDLY" };
    if (attitude < 0 || attitude > 3)
        return 0;
    return StartFaceAni(aniNames[attitude], -2.0f, 1.0f);
}

int oCNpc::StopFaceAni(const zSTRING& aniName)
{
    if (!visual)
        return 1;
    zCModel* model = GetModel();
    if (!model)
        return 0;
    zCModelNodeInst* node = model->SearchNode("BIP01 HEAD");
    zCMorphMesh* morph = node ? dynamic_cast<zCMorphMesh*>(node->nodeVisual) : 0;
    if (!morph)
        return 0;
    morph->StopAni(aniName);
    return 1;
}

int oCNpc::GetWeaponMode()
{
    if (fmode < 0 || fmode >= 8)
        fmode = 0;
    return fmode;
}

void oCNpc::SetWeaponMode2(const zSTRING& mode)
{
    if (mode == "FIST")
        SetWeaponMode2(1);
    else if (mode == "1H")
        SetWeaponMode2(2);
    else if (mode == "2H")
        SetWeaponMode2(3);
    else if (mode == "BOW")
        SetWeaponMode2(4);
    else if (mode == "CBOW")
        SetWeaponMode2(5);
    else if (mode == "MAG")
        SetWeaponMode2(6);
    else
        SetWeaponMode2(0);
}

void oCNpc::SetWeaponMode2(int mode)
{
    if (mode == fmode)
        return;
    int oldMode = fmode;
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    zCModel* model = GetModel();
    int transitionMode = oldMode == 0 ? mode : oldMode;
    if (oldMode == 0 || mode == 0)
    {
        zSTRING sheathNode;
        zSTRING handNode;
        switch (transitionMode)
        {
        case 2:
        case 3:
            sheathNode = NPC_NODE_SWORD;
            handNode = NPC_NODE_RIGHTHAND;
            break;
        case 4:
            sheathNode = NPC_NODE_LONGSWORD;
            handNode = NPC_NODE_RIGHTHAND;
            break;
        case 5:
            sheathNode = NPC_NODE_BOW;
            handNode = NPC_NODE_LEFTHAND;
            break;
        case 6:
            sheathNode = NPC_NODE_CROSSBOW;
            handNode = NPC_NODE_RIGHTHAND;
            break;
        case 7:
            if (mag_book)
            {
                if (oldMode == 0)
                {
                    mag_book->Open(0);
                    mag_book->SetShowHandSymbol(1);
                }
                else
                    mag_book->Close(1);
            }
            break;
        }
        if (!sheathNode.IsEmpty() && !handNode.IsEmpty())
        {
            TNpcSlot* sheath = GetInvSlot(sheathNode);
            TNpcSlot* hand = GetInvSlot(handNode);
            int sheathInventory = sheath->inInventory;
            int handInventory = hand->inInventory;
            zCVob* sheathVob = RemoveFromSlot(sheath, 1, 0);
            zCVob* handVob = RemoveFromSlot(hand, 1, 0);
            PutInSlot(sheath, handVob, handInventory);
            PutInSlot(hand, sheathVob, sheathInventory);
        }
    }
    SetWeaponMode(mode);
    if (humanAI)
    {
        humanAI->SetFightAnis(mode);
        humanAI->SetWalkMode(humanAI->walkmode);
        if (model && humanAI->IsWalking())
            model->StartAni(humanAI->_s_walkl, 0);
    }
}

void oCNpc::SetWeaponMode(int mode)
{
    fmode = mode;
    if (fmode < 0 || fmode >= 8)
        fmode = 0;

    if (IsAPlayer())
    {
        oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
        if (humanAI)
        {
            if (fmode < 5)
                humanAI->SetCamMode(CamModMelee, 0);
            else
                humanAI->SetCamMode(CamModNormal, 0);
        }
        if (fmode < 5)
            oCNpcFocus::SetFocusMode(1);
        else if (fmode == 7)
            oCNpcFocus::SetFocusMode(5);
        else
            oCNpcFocus::SetFocusMode(2);
    }
}

int oCNpc::GetNextWeaponMode(int weaponMode, int direction, int remove)
{
    oCItem* melee = GetSlotItem("ZS_SWORD");
    if (!melee)
        melee = GetSlotItem("ZS_LONGSWORD");
    oCItem* ranged = GetSlotItem("ZS_BOW");
    if (!ranged)
        ranged = GetSlotItem("ZS_CROSSBOW");

    int result = weaponMode;
    while (true)
    {
        result += direction;
        if (result > 7)
            result = 1;
        else if (result <= 0)
            result = 7;

        switch (result)
        {
        case 1:
            if (remove || !melee)
                return result;
            break;
        case 2:
            if (melee && melee->HasFlag(0x2000))
                return result;
            break;
        case 3:
            if (melee && (melee->HasFlag(0x8000) || melee->HasFlag(0x4000)))
                return result;
            break;
        case 4:
            if (melee && (melee->HasFlag(0x10000) || melee->HasFlag(0x20000)))
                return result;
            break;
        case 5:
            if (ranged && ranged->HasFlag(0x80000) && IsMunitionAvailable(ranged))
                return result;
            break;
        case 6:
            if (ranged && ranged->HasFlag(0x100000) && IsMunitionAvailable(ranged))
                return result;
            break;
        case 7:
            if (mag_book && mag_book->GetNoOfSpells() > 0)
                return result;
            break;
        }
    }
}

void oCNpc::Equip(oCItem* item)
{
    if (!item)
        return;

    int category = inventory2.GetCategory(item);
    if (category == 1)
    {
        if (item->HasFlag(2))
            EquipWeapon(item);
        else if (item->HasFlag(4))
        {
            int equipped = item->HasFlag(0x40000000);
            UnequipItem(GetSlotItem("ZS_BOW"));
            UnequipItem(GetSlotItem("ZS_CROSSBOW"));
            if (!equipped)
            {
                if (!CanUse(item) && this == player)
                {
                    DisplayCannotUse();
                    return;
                }
                EquipItem(item);
            }
        }
        return;
    }
    if (category == 2)
    {
        EquipArmor(item);
        return;
    }
    if (category == 3)
    {
        if (item->HasFlag(0x40000000))
        {
            if (mag_book)
                mag_book->DeRegister(item);
            item->ClearFlag(0x40000000);
        }
        else if (mag_book && mag_book->GetNoOfSpells() <= 6)
        {
            mag_book->Register(item, 1);
            item->SetFlag(0x40000000);
        }
        return;
    }
    if (category == 4)
    {
        if (item->HasFlag(0x40000000))
        {
            RemoveItemEffects(item);
            item->ClearFlag(0x40000000);
            return;
        }
        if (item->HasFlag(0x100) || item->HasFlag(0x800) || item->HasFlag(0x1000000))
        {
            inventory2.UnpackCategory();
            int count = inventory2.GetNumItemsInCategory();
            int equipped = 0;
            for (int i = 0; i < count; ++i)
            {
                oCItem* other = inventory2.GetItem(i);
                if (other && other->HasFlag(0x40000000)
                    && (other->HasFlag(0x100) || other->HasFlag(0x800)
                        || other->HasFlag(0x1000000)))
                    ++equipped;
            }
            if ((item->HasFlag(0x100) && equipped)
                || (item->HasFlag(0x800) && equipped >= 2)
                || (item->HasFlag(0x1000000) && equipped))
                return;
        }
        AddItemEffects(item);
        item->SetFlag(0x40000000);
        return;
    }
    UseItem(item);
}

void oCNpc::EquipWeapon(oCItem* item)
{
    if (item->HasFlag(0x40000000))
    {
        UnequipItem(item);
        return;
    }
    if (!CanUse(item) && this == player)
    {
        DisplayCannotUse();
        return;
    }

    if (item->HasFlag(0x10000) || item->HasFlag(0x20000))
    {
        UnequipItem(GetSlotItem("ZS_SWORD"));
        UnequipItem(GetSlotItem("ZS_LONGSWORD"));
        UnequipItem(GetSlotItem("ZS_SHIELD"));
    }
    else if (item->HasFlag(2) && !item->HasFlag(0x40000))
    {
        UnequipItem(GetSlotItem("ZS_SWORD"));
        UnequipItem(GetSlotItem("ZS_LONGSWORD"));
    }
    else if (item->HasFlag(0x40000))
        UnequipItem(GetSlotItem("ZS_SHIELD"));
    EquipItem(item);
}

void oCNpc::EquipFarWeapon(oCItem* item)
{
    if (!item || !item->HasFlag(4))
        return;
    int equipped = item->HasFlag(0x40000000);
    UnequipItem(GetSlotItem("ZS_BOW"));
    UnequipItem(GetSlotItem("ZS_CROSSBOW"));
    if (!equipped)
    {
        if (!CanUse(item) && this == player)
        {
            DisplayCannotUse();
            return;
        }
        EquipItem(item);
    }
}

void oCNpc::EquipArmor(oCItem* item)
{
    if (!item->HasFlag(0x10))
        return;
    if (item->HasFlag(0x40000000))
    {
        UnequipItem(item);
        return;
    }
    if (!CanUse(item) && this == player)
    {
        DisplayCannotUse();
        return;
    }

    zSTRING nodeName;
    if (item->wear & 2)
        nodeName = "ZS_HELMET";
    else if (item->wear & 1)
        nodeName = "ZS_TORSO";
    if (!nodeName.IsEmpty())
    {
        UnequipItem(GetSlotItem(nodeName));
        EquipItem(item);
    }
}

void oCNpc::SetToFightMode(oCItem* item, int immediate)
{
    if (inventory2.GetCategory(item) != 1 || !item->HasFlag(2))
        return;

    int oldMode = GetWeaponMode();
    if (oldMode > 1)
    {
        oCItem* oldWeapon;
        if (oldMode > 4 && oldMode != 6)
            oldWeapon = GetSlotItem("ZS_LEFTHAND");
        else
            oldWeapon = GetSlotItem("ZS_RIGHTHAND");
        if (oldWeapon)
            UnequipItem(oldWeapon);
    }

    EquipWeapon(item);
    SetWeaponMode2(1);
    SetWeaponMode2(GetNextWeaponMode(GetWeaponMode(), 1, 0));
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (humanAI)
    {
        humanAI->InitCamModes(-1);
        humanAI->SetFightAnis(GetWeaponMode());
        humanAI->SetWalkMode(0);
        humanAI->wmode_selected = GetWeaponMode();
        zCModel* model = GetModel();
        if (model && immediate)
            model->StartAni(humanAI->_s_walk, 0);
    }
}

void oCNpc::SetToFistMode()
{
    SetWeaponMode2(1);
    zCModel* model = GetModel();
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (model)
    {
        zCModelNodeInst* node = model->SearchNode("ZS_RIGHTHAND");
        if (node)
            model->SetNodeVisual(node, 0, 0);
        if (humanAI)
            model->StartAni(humanAI->_s_walk, 0);
    }
    if (humanAI)
    {
        humanAI->SetFightAnis(GetWeaponMode());
        humanAI->SetWalkMode(0);
        humanAI->wmode_selected = 1;
        humanAI->InitCamModes(-1);
    }
}

void oCNpc::Activate(int, int)
{
}

oCVob* oCNpc::GetRightHand()
{
    return GetSlotVob("ZS_RIGHTHAND");
}

oCVob* oCNpc::GetLeftHand()
{
    return GetSlotVob("ZS_LEFTHAND");
}

void oCNpc::DropVob(oCVob* vob, zVEC3& position)
{
    zCWorld* world = ogame->GetWorld();
    if (!vob->homeWorld)
        world->EnableVob(vob, 0);
    if (!vob->SearchFreePosition(position, this))
        zerr.Message("U: NPC: DropItem:Not enough space.");
    vob->SetPositionWorld(position);
    if (!vob->homeWorld)
        world->AddVob(vob);
}

static TNpcSlot* FindNpcHandSlot(oCNpc* npc, const zSTRING& name)
{
    for (int i = 0; i < npc->invSlot.numInArray; ++i)
    {
        if (npc->invSlot[i]->name == name)
            return npc->invSlot[i];
    }
    return 0;
}

void oCNpc::SetCarryVob(zCVob* vob)
{
    zSTRING carryName;
    oCMOB* mob = dynamic_cast<oCMOB*>(vob);
    if (mob)
        carryName = mob->GetName();

    if (carry_vob)
    {
        oCMOB* oldMob = dynamic_cast<oCMOB*>(carry_vob);
        if (oldMob)
            RemoveOverlay("HUMANS_CARRY" + oldMob->GetName() + ".MDS");
        TNpcSlot* slot = FindNpcHandSlot(this, "ZS_RIGHTHAND");
        if (slot)
            slot->ClearVob();
    }

    if (vob)
    {
        oCAIVobMove* movement = new oCAIVobMove;
        zVEC3 position;
        position.n[0] = vob->trafoObjToWorld.m[3][0];
        position.n[1] = vob->trafoObjToWorld.m[3][1];
        position.n[2] = vob->trafoObjToWorld.m[3][2];
        movement->Init(vob, this, position, 0.0f, 0.0f, 0);
        TNpcSlot* slot = FindNpcHandSlot(this, "ZS_RIGHTHAND");
        if (slot)
            slot->SetVob(vob);
        ApplyOverlay("HUMANS_CARRY" + carryName + ".MDS");
        movement->Release();
    }
    carry_vob = vob;
}

zCVob* oCNpc::GetCarryVob()
{
    return carry_vob;
}

void oCNpc::SetLeftHand(oCVob* vob)
{
    if (!vob)
        return;
    oCItem* item = dynamic_cast<oCItem*>(vob);
    int inInventory = item && inventory2.IsIn(item, 1);
    TNpcSlot* slot = FindNpcHandSlot(this, "ZS_LEFTHAND");
    if (slot)
    {
        slot->inInventory = inInventory;
        slot->SetVob(vob);
    }
}

void oCNpc::SetRightHand(oCVob* vob)
{
    if (!vob)
        return;
    oCItem* item = dynamic_cast<oCItem*>(vob);
    int inInventory = item && inventory2.IsIn(item, 1);
    TNpcSlot* slot = FindNpcHandSlot(this, "ZS_RIGHTHAND");
    if (slot)
    {
        slot->inInventory = inInventory;
        slot->SetVob(vob);
    }
}

int oCNpc::HasTorch()
{
    oCItem* item = GetSlotItem("ZS_LEFTHAND");
    if (item)
        return item->HasFlag(0x10000000);
    item = GetSlotItem("ZS_RIGHTHAND");
    return item ? item->HasFlag(0x10000000) : 0;
}

void oCNpc::ExchangeTorch()
{
}

void oCNpc::SetTorchAni(int enabled, int immediate)
{
    TActiveInfo* info = GetActiveInfoWritable();
    info->changeTorchAniTo = enabled;
    if (immediate)
    {
        info->changeTorchAni = 1;
        CheckSetTorchAni();
        return;
    }

    info->changeTorchAni = 0;
    if (guildTrue >= 58)
    {
        if (enabled)
            ApplyOverlay("ORC_TORCH.MDS");
        else
            RemoveOverlay("ORC_TORCH.MDS");
    }
    if (guildTrue <= 16)
    {
        if (enabled)
            ApplyOverlay("HUMANS_TORCH.MDS");
        else
            RemoveOverlay("HUMANS_TORCH.MDS");
    }
}

void oCNpc::CheckSetTorchAni()
{
    TActiveInfo* info = GetActiveInfoWritable();
    if (!info->changeTorchAni || !visual || !human_ai)
        return;
    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (!model)
        return;

    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (model->IsStateActive(model->GetAniFromAniID(humanAI->_s_walk)))
        SetTorchAni(info->changeTorchAniTo, 0);
    else if (humanAI->s_run[GetWeaponMode()] != -1
        && model->IsStateActive(model->GetAniFromAniID(humanAI->s_run[GetWeaponMode()])))
        SetTorchAni(info->changeTorchAniTo, 0);
}

void oCNpc::CheckPutbackTorch()
{
    static int recursion;
    if (recursion)
        return;
    recursion = 1;

    if (!GetLeftHand())
    {
        TActiveInfo* info = GetActiveInfoWritable();
        if (info->putTorchBackInHand)
        {
            info->putTorchBackInHand = 0;
            oCItem* torch = GetSlotItem("ZS_RIGHTHAND");
            if (!torch || torch->GetObjectName() != "ITLSTORCHBURNING")
                torch = inventory2.IsIn("ITLSTORCH", 1);
            if (torch)
                UseItem(torch);
        }
    }

    oCItem* torch = GetSlotItem("ZS_LEFTHAND");
    SetTorchAni(torch && torch->GetObjectName() == "ITLSTORCHBURNING", 0);
    recursion = 0;
}

int oCNpc::UseItem(oCItem* item)
{
    if (!CanUse(item) && this == player)
    {
        DisplayCannotUse();
        return 0;
    }

    if (item->HasFlag(0x20) || item->HasFlag(0x40))
    {
        ChangeAttribute(1, item->nutrition);
        AddItemEffects(item);
        if (item->amount > 1)
            --item->amount;
        else if (!item->HasFlag(0x8000000))
        {
            if (GetSlotItem("ZS_RIGHTHAND") == item)
            {
                TNpcSlot* slot = FindNpcHandSlot(this, "ZS_RIGHTHAND");
                if (slot)
                    slot->ClearVob();
            }
            else if (GetSlotItem("ZS_LEFTHAND") == item)
            {
                TNpcSlot* slot = FindNpcHandSlot(this, "ZS_LEFTHAND");
                if (slot)
                    slot->ClearVob();
            }
            else if (inventory2.IsIn(item, 1))
            {
                oCItem* removed = inventory2.Remove(item, 1);
                if (removed && removed->HasFlag(0x40000000))
                    Equip(removed);
            }
            if (ogame && ogame->GetWorld())
                ogame->GetWorld()->RemoveVob(item);
        }
    }
    else if (item->HasFlag(0x10000000) && item->GetName() != "ITLSTORCHBURNING")
    {
        oCItem* left = GetSlotItem("ZS_LEFTHAND");
        oCItem* right = GetSlotItem("ZS_RIGHTHAND");
        oCItem* held = left ? left : right;
        if (held && held->HasFlag(0x10000000))
        {
            if (left && left->GetObjectName() == "ITLSTORCHBURNING")
            {
                TNpcSlot* slot = FindNpcHandSlot(this, "ZS_LEFTHAND");
                if (slot)
                {
                    slot->ClearVob();
                    inventory2.Insert(left);
                }
                SetTorchAni(0, 0);
            }
            else if (right)
            {
                TNpcSlot* rightSlot = FindNpcHandSlot(this, "ZS_RIGHTHAND");
                TNpcSlot* leftSlot = FindNpcHandSlot(this, "ZS_LEFTHAND");
                if (rightSlot)
                    rightSlot->ClearVob();
                if (leftSlot)
                    leftSlot->SetVob(right);
                SetTorchAni(1, 1);
            }
        }
        else
        {
            oCItem* equipped = GetSlotItem("ZS_RIGHTHAND");
            if (equipped && equipped->HasFlag(0x40000000))
                Equip(equipped);
            oCItem* burning = static_cast<oCItem*>(ogame->GetWorld()->CreateVob(
                static_cast<zTVobType>(0x81), "ITLSTORCHBURNING"));
            TNpcSlot* slot = FindNpcHandSlot(this, "ZS_LEFTHAND");
            if (slot)
            {
                slot->inInventory = 0;
                slot->SetVob(burning);
            }
            SetTorchAni(1, 1);
        }
    }
    return 1;
}

float oCNpc::GetThrowSpeed(float distance, float angle)
{
    float radians = angle * 0.0174532924f;
    return sqrt(distance * 981.0f / sin(radians + radians)) * 0.89999997615814209f;
}

int oCNpc::HasArrowInHand()
{
    oCItem* item = GetSlotItem("ZS_RIGHTHAND");
    return item && item->GetObjectName() == "ARROW";
}

int oCNpc::HasArrowInInv()
{
    return inventory2.IsIn("ARROW", 1) != 0;
}

int oCNpc::HasBoltInInv()
{
    return inventory2.IsIn("BOLT", 1) != 0;
}

int oCNpc::HasBoltInHand()
{
    oCItem* item = GetSlotItem("ZS_LEFTHAND");
    return item && item->GetObjectName() == "BOLT";
}

int oCNpc::IsMunitionAvailable(oCItem* item)
{
    if (!item)
        return 0;
    if (item->HasFlag(0x2000000))
        return 1;

    oCItem* right = GetSlotItem("ZS_RIGHTHAND");
    oCItem* left = GetSlotItem("ZS_LEFTHAND");
    if ((right && right->GetInstance() == item->munition)
        || (left && left->GetInstance() == item->munition))
        return 1;
    const int available = inventory2.IsIn(item->munition, 1) != 0;
    if (!available && this == player) {
        GetEM(0)->OnMessage(new oCMsgManipulate(oCMsgManipulate::EV_CALLSCRIPT,
            PLAYER_RANGED_NO_AMMO, -1), this);
    }
    return available;
}

void oCNpc::GetTalentInfo(int talent, zSTRING& name, zSTRING& description, zSTRING& skill)
{
    name = "";
    description = "";
    skill = "";
    zCPar_Symbol* symbol = parser.GetSymbol("TXT_TALENTS");
    if (symbol)
        symbol->GetValue(name, talent);
    symbol = parser.GetSymbol("TXT_TALENTS_DESC");
    if (symbol)
        symbol->GetValue(description, talent);
    symbol = parser.GetSymbol("TXT_TALENTS_SKILLS");
    if (symbol)
        symbol->GetValue(skill, talent);
}

void oCNpc::GetSpellInfo(int spell, zSTRING& name, zSTRING& description)
{
    name = "";
    description = "";
    zCPar_Symbol* symbol = parser.GetSymbol("TXT_SPELLS");
    if (symbol)
        symbol->GetValue(name, spell);
    symbol = parser.GetSymbol("TXT_SPELLS_DESC");
    if (symbol)
        symbol->GetValue(description, spell);
}

void oCNpc::GetAttribInfo(int attributeIndex, zSTRING& name, zSTRING& description)
{
    name = "";
    description = "";
    zCPar_Symbol* symbol = parser.GetSymbol("TXT_ATTRIBUTE_DESC");
    if (symbol)
        symbol->GetValue(description, attributeIndex);
}

void oCNpc::CreateSpell(int)
{
    if (!mag_book)
        mag_book = new oCMag_Book;
}

void oCNpc::LearnSpell(int spell)
{
    if (!(spells & (1 << (spell & 31))))
    {
        CreateSpell(spell);
        spells |= 1 << (spell & 31);
    }
}

int oCNpc::HasSpell(int spell)
{
    return (spells & (1 << (spell & 31))) > 0;
}

void oCNpc::DestroySpell(int spell)
{
    int mask = 1 << (spell & 31);
    if (spells & mask)
        spells -= mask;
}

void oCNpc::SetActiveSpellInfo(int info)
{
    if (GetWeaponMode() != 7 || !mag_book)
        return;
    oCSpell* spell = mag_book->GetSelectedSpell();
    if (spell)
        spell->SetSpellInfo(info);
}

int oCNpc::GetActiveSpellNr()
{
    if (GetWeaponMode() == 7 && mag_book)
    {
        oCSpell* spell = mag_book->GetSelectedSpell();
        if (spell)
            return spell->GetSpellID();
    }
    return -1;
}

int oCNpc::GetActiveSpellCategory()
{
    if (GetWeaponMode() == 7 && mag_book)
    {
        oCSpell* spell = mag_book->GetSelectedSpell();
        if (spell)
            return spell->GetCategory();
    }
    return -1;
}

int oCNpc::GetActiveSpellLevel()
{
    if (GetWeaponMode() == 7 && mag_book)
    {
        oCSpell* spell = mag_book->GetSelectedSpell();
        if (spell)
            return spell->GetLevel();
    }
    return -1;
}

int oCNpc::GetActiveSpellIsScroll()
{
    if (GetWeaponMode() == 7 && mag_book)
    {
        oCItem* item = mag_book->GetSpellItem(mag_book->GetSelectedSpellNr());
        if (item)
            return item->MultiSlot();
    }
    return 0;
}

void oCNpc::InsertActiveSpell(oCSpell* spell)
{
    if (!spell)
        return;
    spell->AddRef();
    activeSpells.Insert(spell);
}

void oCNpc::RemoveActiveSpell(oCSpell* spell)
{
    if (!spell)
        return;
    activeSpells.Remove(spell);
    spell->Release();
}

void oCNpc::RemoveActiveSpell(int spellID)
{
    zCList<oCSpell>* entry = activeSpells.next;
    while (entry)
    {
        oCSpell* spell = entry->data;
        entry = entry->next;
        if (spell->GetSpellID() == spellID)
            RemoveActiveSpell(spell);
    }
}

int oCNpc::DoActiveSpells()
{
    zCList<oCSpell>* entry = activeSpells.next;
    while (entry)
    {
        oCSpell* spell = entry->data;
        entry = entry->next;
        spell->DoTimedEffect();
        if (spell->CanBeDeleted())
        {
            int deleteCaster = spell->DeleteCaster();
            RemoveActiveSpell(spell);
            if (deleteCaster)
                return 1;
        }
    }
    return 0;
}

void oCNpc::KillActiveSpells()
{
    zCList<oCSpell>* entry = activeSpells.next;
    while (entry)
    {
        oCSpell* spell = entry->data;
        entry = entry->next;
        RemoveActiveSpell(spell);
    }
}

oCSpell* oCNpc::IsSpellActive(int spellID)
{
    for (zCList<oCSpell>* entry = activeSpells.next; entry; entry = entry->next)
    {
        if (entry->data->GetSpellID() == spellID)
            return entry->data;
    }
    return 0;
}

int oCNpc::HasMagic()
{
    return mag_book && mag_book->GetNoOfSpells() > 0;
}

int oCNpc::GetNumberOfSpells()
{
    return mag_book ? mag_book->GetNoOfSpells() : 0;
}

void oCNpc::CopyTransformSpellInvariantValuesTo(oCNpc* other)
{
    if (!other)
        return;
    other->level = level;
    other->learn_points = learn_points;
    other->experience_points = experience_points;
    other->experience_points_next_level = experience_points_next_level;
    other->CheckModelOverlays();
    other->attribute[0] = static_cast<int>(static_cast<float>(attribute[0]) / attribute[1]
        * other->attribute[1]);
    other->CheckModelOverlays();
    zCModel* sourceModel = GetModel();
    zCModel* targetModel = other->GetModel();
    if (sourceModel && targetModel)
        targetModel->fatness = sourceModel->fatness;
}

void oCNpc::OpenScreen_Help()
{
}

void oCNpc::OpenScreen_Log()
{
    oCLogScreen* screen = new oCLogScreen;
    screen->m_pLogMenu->SetLogTopics(oCLogManager::GetLogManager().m_lstTopics);
    int day;
    int hour;
    int minute;
    ogame->GetTime(day, hour, minute);
    const zSTRING dayText = zSTRING().Sprintf("%d", day + 1);
    const zSTRING timeText = zSTRING().Sprintf("%02d:%02d", hour, minute);
    screen->m_pLogMenu->SetDayTime(dayText, timeText);
    const int oldStatus = ogame->GetShowPlayerStatus();
    ogame->SetShowPlayerStatus(0);
    screen->Show();
    ogame->SetShowPlayerStatus(oldStatus);
    delete screen;
}

void oCNpc::OpenScreen_Map(int map)
{
    SetWeaponMode(0);
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (humanAI)
        humanAI->StartStandAni();
    oCMapScreen* screen = new oCMapScreen;
    int oldStatus = ogame->GetShowPlayerStatus();
    ogame->SetShowPlayerStatus(0);
    screen->Show(map);
    ogame->SetShowPlayerStatus(oldStatus);
    delete screen;
}

void oCNpc::OpenScreen_Status()
{
    oCStatusScreen* screen = new oCStatusScreen;
    const int attributeIndices[] = { 4, 5, 2, 0 };
    const oSMenuInfoAttribute::oTMenuAttrib attributeTypes[] = {
        oSMenuInfoAttribute::ATTRIB_HP, oSMenuInfoAttribute::ATTRIB_MANA,
        oSMenuInfoAttribute::ATTRIB_DEX, oSMenuInfoAttribute::ATTRIB_STR
    };
    for (int i = 0; i < 4; ++i) {
        const int index = attributeIndices[i];
        oSMenuInfoAttribute info = {};
        GetAttribInfo(index, info.name, info.descr);
        info.actValue = attribute[index];
        info.maxValue = attribute[index + (i >= 2)];
        info.type = attributeTypes[i];
        screen->m_pStatusMenu->AddAttribute(info);
    }
    screen->m_pStatusMenu->SetExperience(experience_points, level, experience_points_next_level);
    screen->m_pStatusMenu->SetLearnPoints(learn_points);

    const int armorTypes[] = { 2, 64, 8, 32 };
    for (int i = 0; i < 4; ++i) {
        oSMenuInfoArmor armor = {};
        armor.actValue = GetProtectionByType(static_cast<oETypeDamage>(armorTypes[i]));
        screen->m_pStatusMenu->AddArmor(armor);
    }
    for (int i = 0; i < 22; ++i) {
        oSMenuInfoTalent info = {};
        GetTalentInfo(i, info.name, info.desc, info.skillEnum);
        if (talents.parray[i]) {
            info.value = talents.parray[i]->m_value;
            info.skill = talents.parray[i]->m_skill;
        }
        screen->m_pStatusMenu->AddTalent(info);
    }
    screen->m_pStatusMenu->SetPlayerName(GetName());
    screen->m_pStatusMenu->SetGuild(GetGuildName());

    const int oldStatus = ogame->GetShowPlayerStatus();
    ogame->SetShowPlayerStatus(0);
    screen->Show();
    ogame->SetShowPlayerStatus(oldStatus);
    delete screen;
}

void oCNpc::ThisVobAddedToWorld(zCWorld* world)
{
    zCVob::ThisVobAddedToWorld(world);
    SetSleeping(0);
    SetCollDetDyn(1);
    SetCollDetStat(1);
    SetPhysicsEnabled(1);

    for (int i = 0; i < invSlot.numInArray; ++i)
    {
        TNpcSlot* slot = invSlot[i];
        if (!slot->inInventory && slot->vob)
        {
            slot->vob->SetSleeping(0);
            if (!slot->vob->homeWorld)
                world->AddVob(slot->vob);
            world->RemoveVobFromLists(slot->vob);
        }
    }
    AddEffect(effect, 0);
}

void oCNpc::ThisVobRemovedFromWorld(zCWorld* world)
{
    zCVob::ThisVobRemovedFromWorld(world);
    ClearFocusVob();
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        if (vobList[i])
            vobList[i]->Release();
    }
    vobList.DeleteList();
    SetEnemy(0);
    state.CloseCutscenes();
    SetInteractItem(0);

    for (int i = 0; i < invSlot.numInArray; ++i)
    {
        TNpcSlot* slot = invSlot[i];
        if (slot->vob && slot->vob->homeWorld && !slot->inInventory)
            world->RemoveVob(slot->vob);
    }
    for (int i = effectList.numInArray - 1; i >= 0; --i)
    {
        oCVisualFX* effectObject = effectList[i];
        if (effectObject->GetName() == effect)
        {
            effectList.RemoveOrder(effectObject);
            effectObject->Stop(1);
            effectObject->Release();
        }
    }
}

int oCNpc::IsVoiceActive()
{
    return listOfVoiceHandles.numInArray > 0;
}

void oCNpc::StopAllVoices()
{
    for (int i = 0; i < listOfVoiceHandles.numInArray; ++i)
        zsound->StopSound(listOfVoiceHandles[i]);
    voiceIndex = 0;
    while (!UpdateNextVoice())
    {
    }
}

int oCNpc::UpdateNextVoice()
{
    if (!listOfVoiceHandles.numInArray)
        return 1;
    if (voiceIndex >= listOfVoiceHandles.numInArray)
        voiceIndex = 0;

    int handle = listOfVoiceHandles[voiceIndex];
    if (zsound->IsSoundActive(handle))
    {
        zCSoundSystem::zTSound3DParams params;
        params.SetDefaults();
        zsound->UpdateSound3D(handle, params);
        ++voiceIndex;
    }
    else
        listOfVoiceHandles.RemoveOrderIndex(voiceIndex);

    if (voiceIndex >= listOfVoiceHandles.numInArray)
        voiceIndex = 0;
    return voiceIndex == 0;
}

void oCNpc::ProcessNpc()
{
    if (listOfVoiceHandles.numInArray > 0 && homeWorld && homeWorld->masterFrameCtr % 4)
        UpdateNextVoice();
    Regenerate();
    knownNews.CheckSpreadNews();
    if (!CanTalk())
        canTalk -= ztimer.frameTimeFloat;

    if (IsAPlayer()) {
        oCPortalRoomManager* portalRooms = ogame->GetPortalRoomManager();
        if (portalRooms && portalRooms->HasPlayerChangedPortalRoom())
            CreatePassivePerception(31, this, 0);
    }

    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (fabs(divetime + 1000000.0f) >= 0.0001f && humanAI)
    {
        if (humanAI->actionMode != 6)
        {
            if (divectr < divetime)
            {
                divectr += ztimer.frameTimeFloat * 2.0f;
                if (divectr > divetime)
                    divectr = divetime;
            }
        }
        else
        {
            int diveDamage = 0;
            zCPar_Symbol* symbol = parser.GetSymbol("NPC_DAM_DIVE_TIME");
            if (symbol)
                symbol->GetValue(diveDamage, 0);
            divectr -= ztimer.frameTimeFloat;
            if (divectr < -diveDamage)
            {
                divectr = 0.0f;
                ChangeAttribute(0, -1);
                if (attribute[0] <= 0)
                {
                    zCModel* model = GetModel();
                    if (!model)
                    {
                        AvoidShrink(1000);
                        model = GetModel();
                    }
                    if (model)
                        model->StartAni("T_DIVE_2_DROWNED", 0);
                    SetWeaponMode2(0);
                }
            }
        }
    }

    magFrontier.DoCheck();
    if (showaidebug & 2)
        knownNews.ShowDebugInfos();
    CheckSetTorchAni();

    zCList<oCNpcTimedOverlay>* entry = timedOverlays.next;
    while (entry)
    {
        oCNpcTimedOverlay* overlay = entry->data;
        entry = entry->next;
        if (!overlay)
            continue;
        overlay->timer -= ztimer.frameTimeFloat;
        if (overlay->timer >= 0.0f)
            continue;
        timedOverlays.Remove(overlay);
        zCModel* model = GetModel();
        if (!model)
        {
            AvoidShrink(1000);
            model = GetModel();
        }
        if (model)
            model->RemoveModelProtoOverlay(overlay->GetMdsName());
        if (humanAI)
            humanAI->InitAnimations();
        delete overlay;
    }
}

zSTRING oCNpc::oCNpcTimedOverlay::GetMdsName()
{
    return mdsOverlayName;
}

oCNpc::oCNpcTimedOverlay::~oCNpcTimedOverlay()
{
}

void oCNpc::DoSpellBook()
{
    if (mag_book)
        mag_book->DoPerFrame();
}

void oCNpc::OpenSpellBook(int spell)
{
    if (!mag_book)
        return;
    const int wasMoving = (isInMovementMode & 3) != 0;
    if (wasMoving)
        EndMovement(1);
    mag_book->SetOwner(this);
    mag_book->Open(spell);
    if (wasMoving)
        BeginMovement();
}

void oCNpc::CloseSpellBook(int spell)
{
    if (mag_book)
        mag_book->Close(spell);
}

oCMag_Book* oCNpc::GetSpellBook()
{
    return mag_book;
}

void oCNpc::GetSoundMaterial_MA(zCSoundManager::zTSndManMedium& medium,
    oTSndMaterial& material, zSTRING aniName)
{
    oCItem* weapon = 0;
    int mode = GetWeaponMode();
    if (mode > 1)
    {
        if (mode > 4 && mode != 6)
            weapon = GetSlotItem("ZS_LEFTHAND");
        else
            weapon = GetSlotItem("ZS_RIGHTHAND");
    }

    if (weapon)
    {
        medium = zCSoundManager::zSND_MAN_MEDIUM_ITEM;
        material = weapon->GetSoundMaterial();
        return;
    }

    medium = zCSoundManager::zSND_MAN_MEDIUM_MODEL;
    if (IsAPlayer())
        material = SND_MAT_STONE;
    else if (aniName.Search("FIST", 1) >= 0)
        material = SND_MAT_WOOD;
    else if (aniName.Search("TAIL", 1) >= 0)
        material = SND_MAT_STONE;
    else
        material = SND_MAT_METAL;
}

void oCNpc::GetSoundMaterial_AM(zCSoundManager::zTSndManMedium& medium,
    oTSndMaterial& material, int)
{
    medium = zCSoundManager::zSND_MAN_MEDIUM_MODEL;
    oCItem* armor = GetSlotItem("ZS_TORSO");
    material = armor ? armor->GetSoundMaterial() : SND_MAT_LEATHER;
}

void oCNpc::GetSoundMaterial_MH_HM(zCSoundManager::zTSndManMedium& medium,
    oTSndMaterial& material)
{
    medium = zCSoundManager::zSND_MAN_MEDIUM_MODEL;
    oCItem* armor = GetSlotItem("ZS_TORSO");
    material = armor ? armor->GetSoundMaterial() : SND_MAT_LEATHER;
}

int oCNpc::HasSenseHear()
{
    return (senses & 2) > 0;
}

int oCNpc::HasSenseSee()
{
    return (senses & 1) > 0;
}

int oCNpc::HasSenseSmell()
{
    return (senses & 4) > 0;
}

void oCNpc::SetSenses(int value)
{
    senses = value;
}

int oCNpc::GetOldScriptState()
{
    return old_script_state;
}

int oCNpc::GetAIState()
{
    return state.GetState();
}

int oCNpc::GetAIStateTime()
{
    return state.GetStateTime();
}

int oCNpc::IsAIState(int stateIndex)
{
    return state.IsInState(stateIndex);
}

void oCNpc::PrintStateCallDebug(const zSTRING& callName, int function, int)
{
    if (!(showaidebug & 1))
        return;
    zSTRING result = "success.";
    if (function != -99 && (function <= 0 || !parser.GetSymbol(function)))
        result = "failure : Script function not found.";
    zerr->Message("U: NPC: " + GetName() + ":Call " + callName + "..." + result);
}

void oCNpc::ShowState(int x, int y)
{
    static const char* walkNames[] = { "Run", "Walk", "Sneak", "Water", "Swim", "Dive" };
    zSTRING text = "Walk : ";
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (humanAI && humanAI->walkmode >= 0 && humanAI->walkmode <= 5)
        text += walkNames[humanAI->walkmode];
    text += "   State : " + zSTRING(GetAIState());
    screen->Print(x, y, text);
    y += screen->FontY();
    screen->Print(x, y, "NpcName : " + GetName());
    y += screen->FontY();
    screen->Print(x, y, "Guild : " + GetGuildName());
    y += screen->FontY();
    screen->Print(x, y, "TempAttitude : " + zSTRING(tmp_attitude));
    y += screen->FontY();
    screen->Print(x, y, "PermAttitude : " + zSTRING(attitude));
}

int oCNpc::IsNear(oCNpc* other)
{
    return GetDistanceToVob2(*other) < 250000.0f;
}

oCItem* oCNpc::DetectItem(int flags, int forbiddenFlags)
{
    if (IsAPlayer())
        return 0;
    float bestDistance = 3.402823466e38f;
    oCItem* result = 0;
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        oCItem* item = dynamic_cast<oCItem*>(vobList[i]);
        if (!item || item->HasFlag(forbiddenFlags) || !(flags & (item->mainflag | item->flags)))
            continue;
        float distance = GetDistanceToVob2(*item);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            result = item;
        }
    }
    return result;
}

oCNpc* oCNpc::DetectPlayer()
{
    if (IsAPlayer())
        return 0;
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(vobList[i]);
        if (npc && npc->IsAPlayer())
            return npc;
    }
    return 0;
}

oCMobInter* oCNpc::FindMobInter(const zSTRING& name)
{
    zCArray<zCVob*> vobs;
    ogame->GetWorld()->SearchVobListByClass(oCMobInter::classDef, vobs, 0);
    float bestDistance = 250000.0f;
    oCMobInter* result = 0;
    for (int i = 0; i < vobs.numInArray; ++i)
    {
        oCMobInter* mob = dynamic_cast<oCMobInter*>(vobs[i]);
        if (!mob || mob->GetName().Search(name, 1) < 0 || !mob->CanInteractWith(this) ||
            !FreeLineOfSight(mob))
            continue;
        float distance = GetDistanceToVob2(*mob);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            result = mob;
        }
    }
    return result;
}

zCVobSpot* oCNpc::FindSpot(const zSTRING& name, int sortByDistance, float range)
{
    zCArray<zCVob*> vobs;
    ogame->GetWorld()->SearchVobListByClass(zCVob::classDef, vobs, 0);
    float bestDistance = range * range;
    zCVobSpot* result = 0;
    for (int i = 0; i < vobs.numInArray; ++i)
    {
        zCVobSpot* spot = dynamic_cast<zCVobSpot*>(vobs[i]);
        if (!spot || spot->GetObjectName().Search(name, 1) < 0 || !spot->IsAvailable() ||
            !FreeLineOfSight(spot))
            continue;
        float distance = GetDistanceToVob2(*spot);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            result = spot;
            if (!sortByDistance)
                break;
        }
    }
    return result;
}

void oCNpc::ForceVobDetection(zCVob* vob)
{
    if (IsAPlayer() || !vob || HasVobDetected(vob))
        return;
    vobList.Insert(vob);
    vob->AddRef();
}

int oCNpc::HasVobDetected(zCVob* vob)
{
    if (!IsAPlayer())
    {
        for (int i = 0; i < vobList.numInArray; ++i)
        {
            if (vobList[i] == vob)
                return 1;
        }
    }
    return 0;
}

int oCNpc::CheckForOwner(zCVob* vob)
{
    oCItem* item = dynamic_cast<oCItem*>(vob);
    if (item)
    {
        parser.SetInstance("ITEM", item);
        CreatePassivePerception(17, vob, 0);
    }
    return 0;
}

void oCNpc::AI_ForceDetection()
{
    vobcheck_time = 100000.0f;
}

int oCNpc::CanSense(zCVob* vob)
{
    if (vob == this)
        return 0;
    oCItem* item = dynamic_cast<oCItem*>(vob);
    if (item && item->HasFlag(0x800000))
        return 0;
    if (HasSenseSmell() && GetDistanceToVobApprox(*vob) <= senses_range)
        return 1;
    return HasSenseSee() && CanSee(vob, 0);
}

void oCNpc::DetectWitnesses(oCNpc* other, int function)
{
    zCArray<zCVob*> vobs;
    ogame->GetWorld()->SearchVobListByClass(oCNpc::classDef, vobs, 0);
    for (int i = 0; i < vobs.numInArray; ++i)
    {
        oCNpc* witness = dynamic_cast<oCNpc*>(vobs[i]);
        if (!witness || witness == this || witness == other || witness->attribute[0] <= 0
            || GetDistanceToVob2(*witness) > 4000000.0f || !FreeLineOfSight(witness))
            continue;
        witness->PrintStateCallDebug("Witness", function, 0);
    }
}

oCNpc* oCNpc::FindNpc(int instance, int guild, int aiState, int nearest)
{
    if (IsAPlayer())
        return 0;
    oCNpc* result = 0;
    float bestDistance = 3.402823466e38f;
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(vobList[i]);
        if (!npc || (instance >= 0 && npc->GetInstance() != instance)
            || (guild >= 0 && npc->guildTrue != guild)
            || (aiState >= 0 && !npc->IsAIState(aiState)))
            continue;
        if (!nearest)
            return npc;
        float distance = GetDistanceToVob2(*npc);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            result = npc;
        }
    }
    return result;
}

static int NpcCanBeFound(oCNpc* npc, int includeDead, int includePlayer)
{
    if (includePlayer && npc->IsAPlayer())
        return 0;
    if (!includeDead)
        return 1;
    if (npc->attribute[0] <= 0 || npc->IsAIState(-4) || npc->IsAIState(-5))
        return 0;
    static const int blockingSpells[] = { 33, 71, 82, 39, 37, 86 };
    for (int i = 0; i < 6; ++i)
    {
        if (npc->IsSpellActive(blockingSpells[i]))
            return 0;
    }
    return 1;
}

oCNpc* oCNpc::FindNpcEx(int instance, int guild, int aiState, int nearest,
    int includeDead, int includePlayer)
{
    if (IsAPlayer())
        return 0;
    oCNpc* result = 0;
    float bestDistance = 3.402823466e38f;
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(vobList[i]);
        if (!npc || !NpcCanBeFound(npc, includeDead, includePlayer)
            || (instance >= 0 && npc->GetInstance() != instance)
            || (guild >= 0 && npc->guildTrue != guild)
            || (aiState >= 0 && !npc->IsAIState(aiState)))
            continue;
        if (!nearest)
            return npc;
        float distance = GetDistanceToVob2(*npc);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            result = npc;
        }
    }
    return result;
}

oCNpc* oCNpc::FindNpcExAtt(int instance, int guild, int aiState, int nearest,
    int includeDead, int includePlayer, int attitude)
{
    if (IsAPlayer())
        return 0;
    oCNpc* result = 0;
    float bestDistance = 3.402823466e38f;
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(vobList[i]);
        if (!npc || !NpcCanBeFound(npc, includeDead, includePlayer)
            || (instance >= 0 && npc->GetInstance() != instance)
            || (guild >= 0 && npc->guildTrue != guild)
            || (aiState >= 0 && !npc->IsAIState(aiState))
            || GetAttitude(npc) != attitude)
            continue;
        if (!nearest)
            return npc;
        float distance = GetDistanceToVob2(*npc);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            result = npc;
        }
    }
    return result;
}

int oCNpc::GetComrades()
{
    if (!homeWorld)
        return 0;
    zCArray<zCVob*> vobs;
    homeWorld->SearchVobListByClass(oCNpc::classDef, vobs, 0);
    int count = 0;
    for (int i = 0; i < vobs.numInArray; ++i)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(vobs[i]);
        if (npc && npc != this && npc->guildTrue == guild && NpcCanBeFound(npc, 1, 0)
            && GetDistanceToVob2(*npc) <= 1000000.0f)
            ++count;
    }
    return count;
}

int oCNpc::AreWeStronger(oCNpc* other)
{
    if (!other || !homeWorld)
        return 1;
    zCArray<zCVob*> vobs;
    homeWorld->SearchVobListByClass(oCNpc::classDef, vobs, 0);
    int friendlyStrength = 0;
    int enemyStrength = 0;
    for (int i = 0; i < vobs.numInArray; ++i)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(vobs[i]);
        if (!npc || GetDistanceToVob2(*npc) > 1000000.0f)
            continue;
        int toUs = npc->GetAttitude(this);
        int toEnemy = npc->GetAttitude(other);
        if (toUs == 0 && toEnemy == 3)
            friendlyStrength += npc->level;
        if (toUs == 3 && toEnemy == 0)
            enemyStrength += npc->level;
    }
    return friendlyStrength >= enemyStrength * 2;
}

oCNpc* oCNpc::IsEnemyBehindFriend(oCNpc* other)
{
    if (!homeWorld)
        return 0;
    zCArray<zCVob*> vobs;
    homeWorld->SearchVobListByClass(oCNpc::classDef, vobs, 0);
    for (int i = 0; i < vobs.numInArray; ++i)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(vobs[i]);
        if (!npc || npc->GetAttitude(other) != 3)
            continue;
        if (npc->IsAPlayer() && npc->HasVobDetected(this))
            continue;
        return npc;
    }
    return 0;
}

int oCNpc::InMobInteraction(const zSTRING& schemeName, int state)
{
    oCMobInter* mob = static_cast<oCMobInter*>(interactMob);
    if (!mob)
        return 0;
    return mob->GetScemeName().Search(schemeName, 1) >= 0 && mob->GetState() == state;
}

int oCNpc::FreeLineOfSight(zCVob* vob)
{
    zVEC3 center;
    for (int i = 0; i < 3; ++i)
        center.n[i] = (vob->bbox3D.maxs.n[i] + vob->bbox3D.mins.n[i]) * 0.5f;
    return FreeLineOfSight(center, vob);
}

int oCNpc::FreeLineOfSight(zVEC3& position, zCVob* vob)
{
    zCArray<zCVob*> ignoreVobs;
    ignoreVobs.Insert(this);
    if (vob)
        ignoreVobs.Insert(vob);
    zVEC3 origin;
    zVEC3 direction;
    for (int i = 0; i < 3; ++i)
    {
        origin.n[i] = trafoObjToWorld.m[i][3];
        direction.n[i] = (position.n[i] - origin.n[i]) * 0.949999988f;
    }
    return !ogame->GetWorld()->TraceRayFirstHit(origin, direction, &ignoreVobs, 0x900);
}

int oCNpc::FreeLineOfSight_WP(zVEC3& position, zCVob* vob)
{
    zCArray<zCVob*> ignoreVobs;
    ignoreVobs.Insert(this);
    if (vob)
        ignoreVobs.Insert(vob);
    zVEC3 origin;
    zVEC3 direction;
    for (int i = 0; i < 3; ++i)
        origin.n[i] = trafoObjToWorld.m[i][3];
    origin.n[1] += 100.0f;
    for (int i = 0; i < 3; ++i)
        direction.n[i] = (position.n[i] - origin.n[i]) * 0.949999988f;
    return !ogame->GetWorld()->TraceRayFirstHit(origin, direction, &ignoreVobs, 0x901);
}

int oCNpc::CanSee(zCVob* vob, int ignoreAngle)
{
    if (!vob)
        return 0;
    oCNpc* other = dynamic_cast<oCNpc*>(vob);
    if (other && other->interactMob) {
        oCMobInter* mob = static_cast<oCMobInter*>(other->interactMob);
        if (mob->GetScemeName().Search("BARREL", 1) >= 0 && mob->GetState() == 1)
            return 0;
    }
    if (GetDistanceToVobApprox(*vob) > senses_range)
        return 0;
    if (!FreeLineOfSight(vob))
        return 0;
    if (ignoreAngle)
        return 1;
    float azimuth;
    float elevation;
    GetAngles(vob, azimuth, elevation);
    return fabs(azimuth) <= 90.0f && fabs(elevation) <= 90.0f;
}

void oCNpc::SetSwimDiveTime(float swimTime, float diveTime)
{
    swimtime = swimTime * 1000.0f;
    divetime = diveTime * 1000.0f;
    divectr = divetime;
}

void oCNpc::GetSwimDiveTime(float& swimTime, float& diveTime, float& diveCounter)
{
    swimTime = swimtime;
    diveTime = divetime;
    diveCounter = divectr;
}

void oCNpc::Regenerate()
{
    float frameTime = ztimer.frameTimeFloat;
    if (attribute[0] > 0)
    {
        hpHeal -= frameTime;
        manaHeal -= frameTime;
        if (attribute[6] > 0 && hpHeal < 0.0f && attribute[0] < attribute[1])
        {
            ChangeAttribute(0, 1);
            hpHeal = attribute[6] * 1000.0f;
        }
        if (attribute[7] > 0 && manaHeal < 0.0f && attribute[2] < attribute[3])
        {
            ChangeAttribute(2, 1);
            manaHeal = attribute[7] * 1000.0f;
        }
    }

    if (fireDamage > 0)
    {
        oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
        if (humanAI && humanAI->GetWaterLevel() >= 1)
        {
            StopBurn();
            return;
        }
        fireDamageTimer -= frameTime;
        if (fireDamageTimer <= 0.0f)
        {
            --fireDamage;
            if (!fireDamage)
            {
                StopBurn();
                fireDamageTimer = 0.0f;
                return;
            }
            fireDamageTimer = 2000.0f;
        }
    }
}

void oCNpc::RefreshNpc()
{
    if (attribute[0] <= 0 || IsAPlayer())
        return;

    if (!GetSlotItem("ZS_TORSO"))
    {
        inventory2.UnpackCategory();
        int count = inventory2.GetNumItemsInCategory();
        for (int i = 0; i < count; ++i)
        {
            oCItem* item = inventory2.GetItem(i);
            if (item && CanUse(item) && item->HasFlag(0x10))
            {
                if (!item->HasFlag(0x40000000))
                    EquipArmor(item);
                break;
            }
        }
    }
    if (!GetSlotItem("ZS_TORSO"))
    {
        parser.SetInstance("SELF", this);
        parser.CallFunc("B_REFRESHARMOR");
    }

    inventory2.UnpackCategory();
    int count = inventory2.GetNumItemsInCategory();
    for (int i = 0; i < count; ++i)
    {
        oCItem* item = inventory2.GetItem(i);
        if (item && item->HasFlag(4) && CanUse(item))
        {
            if (!IsMunitionAvailable(item))
                CreateItems(item->munition, 50);
            break;
        }
    }
    attribute[0] = attribute[1];
    CheckModelOverlays();
    attribute[2] = attribute[3];
    CheckModelOverlays();
}

void oCNpc::RenderNpc(zCView* view)
{
    zCCamera camera;
    zCWorld* world = new zCWorld;
    zCVob* cameraVob = new zCVob;
    zVEC3 origin = { 0.0f, 0.0f, 0.0f };
    SetPositionWorld(origin);
    world->AddVob(this);

    float sizeX = bbox3D.maxs.n[0] - bbox3D.mins.n[0];
    float sizeY = bbox3D.maxs.n[1] - bbox3D.mins.n[1];
    float sizeZ = bbox3D.maxs.n[2] - bbox3D.mins.n[2];
    float distance = sizeY;
    if (sizeZ > distance)
        distance = sizeZ;
    if (sizeX > distance)
        distance = sizeX;
    zVEC3 cameraPosition;
    cameraPosition.n[0] = 0.0f;
    cameraPosition.n[1] = (bbox3D.maxs.n[1] + bbox3D.mins.n[1]) * 0.5f;
    cameraPosition.n[2] = -distance;
    cameraVob->SetPositionWorld(cameraPosition);
    camera.connectedVob = cameraVob;
    camera.SetRenderTarget(view);
    camera.SetFarClipZ(3.402823466e38f);
    world->Render(&camera);
    world->RemoveVob(this);
    cameraVob->Release();
    world->Release();
}

int oCNpc::IsAPlayer()
{
    return this == player;
}

int oCNpc::IsSelfPlayer()
{
    return this == player;
}

int oCNpc::GetPlayerNumber()
{
    return 0;
}

void oCNpc::SetKnowsPlayer(int)
{
    knowsPlayer = 1;
}

int oCNpc::KnowsPlayer(oCNpc*)
{
    return knowsPlayer > 0;
}

int oCNpc::KnowsPlayer(int)
{
    return knowsPlayer > 0;
}

int oCNpc::IsMonster()
{
    if (!IsAPlayer() && !IsGoblin() && guildTrue != 47 && guildTrue != 40 && guildTrue != 41)
        return 1;
    return 0;
}

int oCNpc::IsHuman()
{
    return guildTrue <= 16;
}

int oCNpc::IsGoblin()
{
    return guildTrue == 19 || guildTrue == 20 || guildTrue == 21;
}

int oCNpc::IsOrc()
{
    return guildTrue >= 58;
}

int oCNpc::IsSkeleton()
{
    return guildTrue == 31 || guildTrue == 32;
}

void oCNpc::SetAsPlayer()
{
    if (!oCNpcFocus::focus)
        oCNpcFocus::InitFocusModes();
    if (player)
    {
        for (int i = 0; i < player->vobList.numInArray; ++i)
        {
            if (player->vobList[i])
                player->vobList[i]->Release();
        }
        player->vobList.DeleteList();
        player->ClearFocusVob();
    }
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        if (vobList[i])
            vobList[i]->Release();
    }
    vobList.DeleteList();
    ClearFocusVob();

    player = this;
    parser.SetInstance("HERO", player);
    int mode = GetWeaponMode();
    oCNpcFocus::SetFocusMode(!mode ? 0 : mode >= 5 ? 2 : 1);
    if (!human_ai)
        Enable();
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (humanAI)
        humanAI->InitCamModes(-1);

    zCAICamera* cameraAI = ogame->GetCameraAI();
    zCVob* cameraVob = ogame->GetCameraVob();
    if (cameraAI && cameraVob && cameraVob->homeWorld)
    {
        cameraAI->SetTarget(player);
        cameraAI->ReceiveMsg(0x8000);
        cameraVob->SetHeadingAtWorld(this);
    }
    SetWalkStopChasm(0);
}

int oCNpc::DoDoAniEvents()
{
    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (!model)
        return 0;

    for (int i = 0; i < model->occuredAniEvents.numInArray; ++i)
    {
        zCModelAniEvent* event = model->occuredAniEvents[i];
        if (!event || event->aniEventType != 0)
            continue;
        const zSTRING& tag = event->tagString;
        if (tag == "DEF_FIGHTMODE")
            SetWeaponMode2(event->string[0]);
        else if (tag == "DEF_SWAPMESH")
            DoModelSwapMesh(event->string[0], event->string[1]);
        else if (tag == "DEF_PLACE_MUNITION")
            DoInsertMunition(event->string[0]);
        else if (tag == "DEF_REMOVE_MUNITION")
            DoRemoveMunition();
        else if (tag == "DEF_INSERT_ITEM" || tag == "DEF_CREATE_ITEM")
        {
            int nameIndex = tag == "DEF_INSERT_ITEM" ? 1 : 0;
            int nodeIndex = tag == "DEF_INSERT_ITEM" ? 0 : 1;
            oCItem* item = static_cast<oCItem*>(ogame->GetWorld()->CreateVob(
                static_cast<zTVobType>(0x81), event->string[nameIndex]));
            TNpcSlot* slot = FindNpcHandSlot(this, event->string[nodeIndex]);
            if (slot)
                slot->SetVob(item);
        }
        else if (tag == "DEF_REMOVE_ITEM" || tag == "DEF_DESTROY_ITEM")
        {
            TNpcSlot* slot = FindNpcHandSlot(this, event->string[0]);
            if (slot)
                slot->ClearVob();
        }
        else if (tag == "DEF_PLACE_ITEM")
        {
            oCItem* item = GetSlotItem(event->string[1]);
            if (item)
            {
                TNpcSlot* slot = FindNpcHandSlot(this, event->string[0]);
                if (slot)
                    slot->SetVob(item);
            }
        }
        else if (tag == "DEF_EXCHANGE_ITEM")
            DoModelSwapMesh(event->string[0], event->string[1]);
        else if (tag == "DEF_DROP_TORCH" || tag == "DEF_INV_TORCH")
        {
            oCItem* torch = GetSlotItem("ZS_LEFTHAND");
            if (torch)
                UseItem(torch);
        }
        else if (tag == "DEF_DRAWTORCH")
        {
            oCItem* torch = inventory2.IsIn("ITLSTORCH", 1);
            if (torch)
                UseItem(torch);
        }
    }
    return 0;
}

int oCNpc::DoModelSwapMesh(const zSTRING& firstNode, const zSTRING& secondNode)
{
    if (firstNode.IsEmpty() || secondNode.IsEmpty())
        return 0;
    if (firstNode == "ZS_LEFTHAND" || firstNode == "ZS_RIGHTHAND"
        || secondNode == "ZS_LEFTHAND" || secondNode == "ZS_RIGHTHAND")
    {
        oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
        if (humanAI && GetWeaponMode() != humanAI->wmode_selected)
        {
            SetWeaponMode2(humanAI->wmode_selected);
            humanAI->SetFightAnis(humanAI->wmode_selected);
            humanAI->SetWalkMode(humanAI->walkmode);
        }
    }

    TNpcSlot* first = FindNpcHandSlot(this, firstNode);
    TNpcSlot* second = FindNpcHandSlot(this, secondNode);
    if (!first || !second)
        return 0;
    oCVob* vob = first->vob;
    int inInventory = first->inInventory;
    first->vob = second->vob;
    first->inInventory = second->inInventory;
    second->vob = vob;
    second->inInventory = inInventory;
    return 1;
}

int oCNpc::HasMunitionInHand(oCItem* weapon)
{
    oCItem* item = GetSlotItem("ZS_RIGHTHAND");
    if (item && item->GetInstance() == weapon->munition)
        return 1;
    item = GetSlotItem("ZS_LEFTHAND");
    return item && item->GetInstance() == weapon->munition;
}

oCItem* oCNpc::DoInsertMunition(const zSTRING& nodeName)
{
    oCItem* weapon = 0;
    int mode = GetWeaponMode();
    if (mode > 1)
        weapon = GetSlotItem(mode > 4 && mode != 6 ? "ZS_LEFTHAND" : "ZS_RIGHTHAND");
    if (!weapon || HasMunitionInHand(weapon))
        return 0;
    if (nodeName != "ZS_LEFTHAND" && nodeName != "ZS_RIGHTHAND")
        return 0;
    TNpcSlot* slot = FindNpcHandSlot(this, nodeName);
    if (!slot || slot->vob)
        return 0;

    oCItem* ammunition;
    if (weapon->HasFlag(0x2000000))
        ammunition = static_cast<oCItem*>(ogame->GetWorld()->CreateVob(
            static_cast<zTVobType>(0x81), weapon->munition));
    else
    {
        ammunition = inventory2.Remove(weapon->munition, 1);
        if (ammunition && ammunition->HasFlag(0x40000000))
            Equip(ammunition);
    }
    const int wasMoving = (isInMovementMode & 3) != 0;
    if (wasMoving)
        EndMovement(1);
    if (ammunition)
    {
        ammunition->CopyDamage(weapon);
        slot->inInventory = 0;
        slot->SetVob(ammunition);
    }
    if (wasMoving)
        BeginMovement();
    return ammunition;
}

void oCNpc::DoRemoveMunition()
{
    oCItem* weapon = 0;
    int mode = GetWeaponMode();
    if (mode > 1)
        weapon = GetSlotItem(mode > 4 && mode != 6 ? "ZS_LEFTHAND" : "ZS_RIGHTHAND");
    if (!weapon)
        return;

    static const char* handNames[] = { "ZS_RIGHTHAND", "ZS_LEFTHAND" };
    for (int i = 0; i < 2; ++i)
    {
        TNpcSlot* slot = FindNpcHandSlot(this, handNames[i]);
        oCItem* ammunition = slot ? dynamic_cast<oCItem*>(slot->vob) : 0;
        if (!ammunition || ammunition->GetInstance() != weapon->munition)
            continue;
        slot->ClearVob();
        if (weapon->HasFlag(0x2000000))
            ammunition->Release();
        else
            PutInInv(ammunition);
    }
}

int oCNpc::DoShootArrow(int)
{
    int mode = GetWeaponMode();
    if (mode != 5 && mode != 6)
        return 0;
    oCItem* weapon = GetSlotItem("ZS_LEFTHAND");
    if (!weapon || !HasMunitionInHand(weapon))
        return 0;

    const zSTRING handName = mode == 5 ? zSTRING("ZS_RIGHTHAND") : zSTRING("ZS_LEFTHAND");
    TNpcSlot* slot = FindNpcHandSlot(this, handName);
    oCItem* arrow = slot ? dynamic_cast<oCItem*>(slot->vob) : 0;
    if (!arrow)
        return 0;
    slot->ClearVob();
    arrow->SetSleeping(1);
    if (!arrow->homeWorld)
        homeWorld->AddVob(arrow);

    oCAIArrow* arrowAI = new oCAIArrow;
    arrow->SetAI(reinterpret_cast<zCAIBase*>(arrowAI));
    arrow->SetVobName("Pfeil von NSC" + GetName());
    arrowAI->SetupAIVob(arrow, this, focus_vob);
    arrow->SetSleeping(0);
    arrowAI->Release();
    return 1;
}

int oCNpc::DoTakeVob(zCVob* vob)
{
    if (!vob)
        return 0;
    oCNpc* npc = dynamic_cast<oCNpc*>(vob);
    if (npc)
    {
        if (npc->guildTrue == 17)
        {
            npc->SetSleeping(1);
            npc->SetPhysicsEnabled(0);
            npc->SetCollDetDyn(0);
            SetRightHand(static_cast<oCVob*>(npc));
            return 1;
        }
        if (npc->guildTrue == 19)
        {
            npc->SetSleeping(1);
            npc->SetPhysicsEnabled(0);
            npc->SetCollDetDyn(0);
            SetCarryVob(npc);
            npc->SetAI(0);
            npc->SetSleeping(0);
            zCModel* model = npc->GetModel();
            if (!model)
            {
                npc->AvoidShrink(1000);
                model = npc->GetModel();
            }
            if (model)
                model->StartAni("S_GOBGOT", 0);
        }
        return 1;
    }
    oCMOB* mob = dynamic_cast<oCMOB*>(vob);
    if (mob)
    {
        SetCarryVob(mob);
        oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
        if (humanAI)
            humanAI->StopLookAtTarget();
        return 1;
    }

    oCItem* item = dynamic_cast<oCItem*>(vob);
    if (!item || !item->homeWorld)
        return 0;
    if (!IsAPlayer())
        item->ClearFlag(0x400);
    if (focus_vob == item)
        ClearFocusVob();
    ogame->GetWorld()->RemoveVob(item);
    if (item->GetObjectName() == "ITLSTORCHBURNING" && !GetSlotItem("ZS_LEFTHAND"))
    {
        SetLeftHand(item);
        SetTorchAni(1, 1);
    }
    else
    {
        PutInInv(item);
        parser.SetInstance("ITEM", item);
        CreatePassivePerception(17, this, 0);
    }
    return 1;
}

int oCNpc::DoDropVob(zCVob* vob)
{
    if (!vob)
        return 0;
    zVEC3 position;
    if (vob->homeWorld)
    {
        for (int i = 0; i < 3; ++i)
            position.n[i] = vob->trafoObjToWorld.m[i][3];
    }
    else
    {
        zMAT4 handTrafo = GetTrafoModelNodeToWorld("ZS_RIGHTHAND");
        for (int i = 0; i < 3; ++i)
            position.n[i] = handTrafo.m[i][3];
    }

    oCItem* item = dynamic_cast<oCItem*>(vob);
    if (item)
    {
        oCItem* removed = inventory2.Remove(item, item->amount);
        if (removed && removed->HasFlag(0x40000000))
            Equip(removed);
        for (int i = 0; i < invSlot.numInArray; ++i)
        {
            if (invSlot[i]->vob == vob)
                invSlot[i]->ClearVob();
        }
        if (item->HasFlag(0x40000000))
        {
            RemoveItemEffects(item);
            item->ClearFlag(0x40000000);
        }
        if (IsAPlayer())
            item->SetFlag(0x400);
    }
    if (vob == carry_vob)
        SetCarryVob(0);
    DropVob(vob, position);
    oCAIVobMove* movement = new oCAIVobMove;
    movement->Init(vob, this, position, 0.0f, 100.0f, &trafoObjToWorld);
    movement->Release();
    return 1;
}

int oCNpc::DoThrowVob(zCVob* vob, float speed)
{
    if (!dynamic_cast<oCVob*>(vob))
        return 0;
    zVEC3 position;
    if (vob->homeWorld)
    {
        for (int i = 0; i < 3; ++i)
            position.n[i] = vob->trafoObjToWorld.m[i][3];
    }
    else
    {
        zMAT4 handTrafo = GetTrafoModelNodeToWorld("ZS_RIGHTHAND");
        for (int i = 0; i < 3; ++i)
            position.n[i] = handTrafo.m[i][3];
    }

    oCItem* item = dynamic_cast<oCItem*>(vob);
    if (item)
    {
        for (int i = 0; i < invSlot.numInArray; ++i)
        {
            if (invSlot[i]->vob == item)
                invSlot[i]->ClearVob();
        }
        oCItem* removed = inventory2.Remove(item, item->amount);
        if (removed && removed->HasFlag(0x40000000))
            Equip(removed);
    }
    if (vob == carry_vob)
        SetCarryVob(0);
    oCAIVobMove* movement = static_cast<oCVob*>(vob)->GetAIVobMove();
    if (movement)
    {
        float verticalSpeed = dynamic_cast<oCMOB*>(vob) ? 20.0f : 35.0f;
        movement->Init(vob, this, position, speed, verticalSpeed, &trafoObjToWorld);
    }
    return 1;
}

int oCNpc::DoPutInInventory(oCItem* item)
{
    if (!item)
        return 0;
    PutInInv(item);
    return 1;
}

int oCNpc::DoRemoveFromInventory(oCItem* item)
{
    if (!item)
        return 0;
    oCItem* removed = inventory2.Remove(item, 1);
    if (removed && removed->HasFlag(0x40000000))
        Equip(removed);
    return 1;
}

int oCNpc::DoSetToFightMode(oCItem* item)
{
    if (!item)
        return 0;
    SetToFightMode(item, 0);
    return 1;
}

int oCNpc::DoExchangeTorch()
{
    oCItem* right = GetSlotItem("ZS_RIGHTHAND");
    oCItem* left = GetSlotItem("ZS_LEFTHAND");
    TNpcSlot* rightSlot = FindNpcHandSlot(this, "ZS_RIGHTHAND");
    TNpcSlot* leftSlot = FindNpcHandSlot(this, "ZS_LEFTHAND");
    if (rightSlot)
        rightSlot->ClearVob();
    if (leftSlot)
        leftSlot->ClearVob();
    if (left)
        SetRightHand(left);
    if (right)
        SetLeftHand(right);
    return 1;
}

void oCNpc::Pack(zCBuffer& buffer)
{
    buffer.Write(&fmode, 4);
    buffer.Write(attribute, 0x20);
    buffer.Write(&script_aiprio, 4);
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    int walkMode = humanAI->walkmode;
    buffer.Write(&walkMode, 4);
    int hasEnemy = enemy != 0;
    buffer.Write(&hasEnemy, 4);
}

void oCNpc::Unpack(zCBuffer& buffer)
{
    buffer.Read(&fmode, 4);
    buffer.Read(attribute, 0x20);
    buffer.Read(&script_aiprio, 4);
    int walkMode;
    buffer.Read(&walkMode, 4);
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (humanAI && humanAI->walkmode != walkMode)
        humanAI->SetWalkMode(walkMode);
    int enemyID;
    buffer.Read(&enemyID, 4);
    if (enemyID > 0)
        enemy = static_cast<oCNpc*>(ogame->GetWorld()->SearchVobByID(enemyID, 0));
}

int oCNpc::IsInFightMode_S(int mode)
{
    int currentMode = GetWeaponMode();
    if (mode == 0)
        return currentMode == 0;
    if (mode == 1)
        return currentMode == 1;
    if (mode == 2)
        return currentMode >= 2 && currentMode < 5;
    if (mode == 5)
        return currentMode == 5 || currentMode == 6;
    if (mode == 7)
        return currentMode == 7;
    return 0;
}

int oCNpc::IsAiming_S(oCNpc* other)
{
    if (dynamic_cast<oCNpc*>(focus_vob) != other)
        return 0;
    if (GetWeaponMode() >= 7 && mag_book)
    {
        oCSpell* spell = mag_book->GetSelectedSpell();
        if (spell && spell->GetCategory() == 2)
            return 1;
    }
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    return humanAI && humanAI->IsAiming();
}

zSTRING oCNpc::GetInstanceName()
{
    int first = 0;
    int second = 0;
    return parser.GetSymbolInfo(instanz, first, second);
}

int oCNpc::GetScriptInstance(zSTRING*& instanceName, int& instance)
{
    instance = instanz;
    instanceName = &name[0];
    return 1;
}

int oCNpc::SetByScriptInstance(const zSTRING* instanceName, int instance)
{
    if (instanceName)
        instance = parser.GetIndex(*instanceName);
    InitByScript(instance, 0);
    return 1;
}

zSTRING oCNpc::GetInstanceByID(int id)
{
    static const char* instanceNames[] = { "SELF", "OTHER", "VICTIM", "HERO", "THIEF", "KILLER" };
    if (id < 0 || id >= 6)
        id = 0;
    return zSTRING(instanceNames[id]);
}

int oCNpc::GetIDByInstance(const zSTRING& instanceName)
{
    static const char* instanceNames[] = { "SELF", "OTHER", "VICTIM", "HERO", "THIEF", "KILLER" };
    for (int i = 0; i < 6; ++i)
    {
        if (instanceName == instanceNames[i])
            return i;
    }
    return 0;
}

void oCNpc::Disable()
{
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        if (vobList[i])
            vobList[i]->Release();
    }
    vobList.DeleteList();
    if (ai_disabled || IsAPlayer())
        return;

    while (effectList.numInArray > 0)
    {
        oCVisualFX* effectObject = effectList[0];
        effectList.RemoveOrder(effectObject);
        effectObject->Stop(1);
        effectObject->Release();
    }
    Interrupt(0, 1);
    if (homeWorld)
        ogame->GetWorld()->RemoveVob(this);
    ClearFocusVob();
    SetSleeping(1);
    state.CloseCutscenes();
}

void oCNpc::Enable(zVEC3& position)
{
    if (!GetModel())
        AvoidShrink(1000);
    if (!homeWorld)
        ogame->GetWorld()->AddVob(this);
    if (!ai_disabled)
        Interrupt(0, 1);
    SetCollDetStat(0);
    SetCollDetDyn(0);
    SetPositionWorld(position);
    SetCollDetStat(1);
    SetCollDetDyn(1);
    SetSleeping(0);

    if (!ai_disabled)
    {
        if (!human_ai)
            Enable();
        if (!IsAPlayer() && attribute[0] > 0 && !carry_vob)
        {
            if (!GetRightHand() && !GetLeftHand())
            {
                EquipBestWeapon(2);
                EquipBestWeapon(4);
            }
        }

        zCModel* model = GetModel();
        oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
        if (model)
        {
            model->StopAnisLayerRange(1, 0x100);
            if (attribute[0] > 0)
            {
                if (humanAI)
                {
                    if (GetWeaponMode())
                        humanAI->SetFightAnis(GetWeaponMode());
                    humanAI->SetWalkMode(0);
                    humanAI->SearchStandAni(1);
                }
            }
            else
                model->StartAni("S_DEAD", 0);
        }
        ClearEM();
        AddEffect(effect, 0);
        parser.SetInstance("SELF", this);
        parser.CallFunc("B_REFRESHATINSERT");
    }
}

void oCNpc::ClearEM()
{
    GetEM(0)->Clear();
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (humanAI)
    {
        if (humanAI->IsWalking() || humanAI->actionMode == 5 || humanAI->actionMode == 6)
            humanAI->_Stand();
        humanAI->StopTurnAnis();
    }
    lastLookMsg = 0;
    lastPointMsg = 0;
}

void oCNpc::Archive(zCArchiver& archiver)
{
    zCVisual* savedVisual = visual;
    if (savedVisual)
        savedVisual->AddRef();
    SetVisual(0);
    oCVob::Archive(archiver);
    SetVisual(savedVisual);
    if (savedVisual)
        savedVisual->Release();

    archiver.WriteString(GetInstanceName());
    archiver.WriteVec3(model_scale);
    archiver.WriteFloat(model_fatness);
    archiver.WriteInt(activeOverlays.numInArray);
    for (int i = activeOverlays.numInArray - 1; i >= 0; --i)
        archiver.WriteString(activeOverlays[i]);
    archiver.WriteInt(variousFlags);
    archiver.WriteInt(guild);
    archiver.WriteInt(guildTrue);
    archiver.WriteInt(level);
    archiver.WriteInt(experience_points);
    archiver.WriteInt(experience_points_next_level);
    archiver.WriteInt(learn_points);
    archiver.WriteInt(talents.numInArray);
    for (int i = 0; i < talents.numInArray; ++i)
        archiver.WriteObject(talents[i]);
    archiver.WriteInt(fighttactic);
    archiver.WriteInt(fmode);
    archiver.WriteInt(mad_heal);
    archiver.WriteBool(this == player);
    for (int i = 0; i < 8; ++i)
        archiver.WriteInt(attribute[i]);
    for (int i = 0; i < 4; ++i)
        archiver.WriteInt(hitChance[i]);
    archiver.WriteRaw(&spells, 4);
    knownNews.Archive(archiver);
    archiver.WriteObject(carry_vob);
    archiver.WriteObject(enemy);
    inventory2.Archive(archiver);
    archiver.WriteInt(invSlot.numInArray);
    for (int i = 0; i < invSlot.numInArray; ++i)
    {
        TNpcSlot* slot = invSlot[i];
        archiver.WriteBool(slot->vob != 0);
        archiver.WriteString(slot->name);
        if (slot->vob)
        {
            archiver.WriteObject(slot->vob);
            archiver.WriteBool(slot->inInventory);
        }
    }
    archiver.WriteRaw(protection, 0x20);
    archiver.WriteInt(spellMana);
}

void oCNpc::Unarchive(zCArchiver& archiver)
{
    oCVob::Unarchive(archiver);
    zSTRING instanceName = archiver.ReadString();
    int instance = parser.GetIndex(instanceName);
    if (instance < 0)
    {
        SetAI(0);
        instanz = -1;
    }
    else
        InitByScript(instance, 0);

    archiver.ReadVec3();
    model_fatness = archiver.ReadFloat();
    int overlayCount = archiver.ReadInt();
    for (int i = 0; i < overlayCount; ++i)
        activeOverlays.Insert(archiver.ReadString());
    variousFlags = archiver.ReadInt();
    guild = archiver.ReadInt();
    guildTrue = archiver.ReadInt();
    level = archiver.ReadInt();
    experience_points = archiver.ReadInt();
    experience_points_next_level = archiver.ReadInt();
    learn_points = archiver.ReadInt();
    int talentCount = archiver.ReadInt();
    for (int i = 0; i < talentCount; ++i)
    {
        zCObject* object = 0;
        oCNpcTalent* talent = static_cast<oCNpcTalent*>(archiver.ReadObject(object));
        if (talent)
        {
            SetTalentSkill(i, talent->m_skill);
            SetTalentValue(i, talent->m_value);
            talent->Release();
        }
    }
    fighttactic = archiver.ReadInt();
    fmode = archiver.ReadInt();
    mad_heal = archiver.ReadInt();
    archiver.ReadBool();
    for (int i = 0; i < 8; ++i)
        attribute[i] = archiver.ReadInt();
    for (int i = 0; i < 4; ++i)
        hitChance[i] = archiver.ReadInt();
    archiver.ReadRaw(&spells, 4);
    knownNews.Unarchive(archiver);
    zCObject* object = 0;
    carry_vob = static_cast<zCVob*>(archiver.ReadObject(object));
    object = 0;
    SetEnemy(static_cast<oCNpc*>(archiver.ReadObject(object)));
    inventory2.ClearInventory();
    inventory2.Unarchive(archiver);

    for (int i = 0; i < invSlot.numInArray; ++i)
        delete invSlot[i];
    invSlot.DeleteList();
    int slotCount = archiver.ReadInt();
    for (int i = 0; i < slotCount; ++i)
    {
        int hasVob = archiver.ReadBool();
        zSTRING slotName = archiver.ReadString();
        TNpcSlot* slot = new TNpcSlot;
        slot->name = slotName;
        slot->inInventory = 0;
        invSlot.Insert(slot);
        if (hasVob)
        {
            object = 0;
            oCVob* vob = static_cast<oCVob*>(archiver.ReadObject(object));
            slot->SetVob(vob);
            if (vob)
                vob->Release();
            slot->inInventory = archiver.ReadBool();
        }
    }
    archiver.ReadRaw(protection, 0x20);
    spellMana = archiver.ReadInt();
}

void oCNpc::PreSaveGameProcessing()
{
    for (int i = 0; i < invSlot.numInArray; ++i)
    {
        TNpcSlot* slot = invSlot[i];
        oCVob* vob = slot->vob;
        if (!vob || (interactItem != vob && slot->tmpLevel <= 0) || !vob->homeWorld)
            continue;
        vob->AddRef();
        vob->homeWorld->RemoveVob(vob);
        TNpcSlot* savedSlot = new TNpcSlot;
        savedSlot->name = slot->name;
        savedSlot->inInventory = slot->inInventory;
        savedSlot->tmpLevel = slot->tmpLevel;
        savedSlot->SetVob(vob);
        if (!slot->inInventory)
            PutInInv(static_cast<oCItem*>(vob));
        tmpSlotList.Insert(savedSlot);
        slot->ClearVob();
        vob->Release();
    }
}

void oCNpc::PostSaveGameProcessing()
{
    for (int i = 0; i < tmpSlotList.numInArray; ++i)
    {
        TNpcSlot* savedSlot = tmpSlotList[i];
        oCItem* savedItem = dynamic_cast<oCItem*>(savedSlot->vob);
        if (!savedItem)
            continue;
        oCItem* removed = RemoveFromInv(savedItem->GetInstance(), 1);
        if (removed != savedItem)
            ogame->GetWorld()->RemoveVob(removed);
        TNpcSlot* slot = FindNpcHandSlot(this, savedSlot->name);
        if (slot)
        {
            if (slot->vob != savedItem)
                slot->ClearVob();
            slot->inInventory = savedSlot->inInventory;
            slot->tmpLevel = savedSlot->tmpLevel;
            slot->SetVob(savedItem);
        }
        savedItem->Release();
    }
    tmpSlotList.DeleteList();
}

int oCNpc::AllowDiscardingOfSubtree()
{
    return 1;
}

oCItem* oCNpc::IdentifyMushroom(oCItem* item)
{
    if (item)
        item->Release();
    const zSTRING instanceName = rand() & 1 ? zSTRING("ITFOSTINKMUSHROOM")
        : zSTRING("ITFOHONEYMUSHROOM");
    int instance = parser.GetIndex(instanceName);
    if (instance < 0)
        return 0;
    return static_cast<oCItem*>(ogame->GetWorld()->CreateVob(static_cast<zTVobType>(0x81), instance));
}

void oCNpc::IdentifyAllMushrooms()
{
    int instance = parser.GetIndex("ITFOMUSHROOM");
    if (instance < 0)
        return;
    while (true)
    {
        oCItem* mushroom = inventory2.Remove(instance, 1);
        if (!mushroom)
            break;
        if (mushroom->HasFlag(0x40000000))
            Equip(mushroom);
        PutInInv(IdentifyMushroom(mushroom));
    }
}

int oCNpc::HasMissionItem()
{
    inventory2.UnpackCategory();
    int count = inventory2.GetNumItemsInCategory();
    for (int i = 0; i < count; ++i)
    {
        oCItem* item = inventory2.GetItem(i);
        if (item && item->HasFlag(0x1000))
            return 1;
    }
    return 0;
}

oCItem* oCNpc::IsInInv(oCItem* item, int amount)
{
    return inventory2.IsIn(item, amount);
}

oCItem* oCNpc::GetFromInv(int instance, int amount)
{
    oCItem* item = inventory2.IsIn(instance, amount);
    if (!item || item->amount == amount)
        return item;
    oCItem* copy = static_cast<oCItem*>(item->CreateCopy());
    copy->amount = amount;
    item->amount -= amount;
    return copy;
}

oCItem* oCNpc::IsInInv(int instance, int amount)
{
    return inventory2.IsIn(instance, amount);
}

oCItem* oCNpc::IsInInv(const zSTRING& name, int amount)
{
    return inventory2.IsIn(name, amount);
}

int oCNpc::CanCarry(oCItem* item)
{
    if (inventory2.CanCarry(item) || (item->MultiSlot() && inventory2.IsIn(item->GetInstance(), 1)))
        return 1;
    zCModel* model = GetModel();
    if (model)
        model->StartAni("T_CANNOTTAKE", 0);
    return 0;
}

oCItem* oCNpc::PutInInv(oCItem* item)
{
    if (!item)
        return 0;
    if (item->GetObjectName() == "ITLSTORCHBURNING")
        item = static_cast<oCItem*>(ogame->GetWorld()->CreateVob(static_cast<zTVobType>(0x81),
            "ITLSTORCH"));
    else
        item->AddRef();
    if (item->homeWorld)
        item->homeWorld->RemoveVob(item);
    if (item->effectVob)
        item->effectVob->RemoveVobFromWorld();
    oCItem* result = inventory2.Insert(item);
    item->Release();
    return result;
}

oCItem* oCNpc::PutInInv(int instance, int amount)
{
    if (instance < 0)
        return 0;
    oCItem* result = static_cast<oCItem*>(ogame->GetWorld()->CreateVob(
        static_cast<zTVobType>(0x81), instance));
    if (!result)
        return 0;
    if (result->MultiSlot())
    {
        result->amount = amount;
        return PutInInv(result);
    }
    for (int i = 1; i < amount; ++i)
    {
        oCItem* item = static_cast<oCItem*>(ogame->GetWorld()->CreateVob(
            static_cast<zTVobType>(0x81), instance));
        item->amount = 1;
        PutInInv(item);
    }
    return PutInInv(result);
}

oCItem* oCNpc::PutInInv(const zSTRING& name, int amount)
{
    return PutInInv(parser.GetIndex(name), amount);
}

oCItem* oCNpc::RemoveFromInv(oCItem* item, int amount)
{
    if (!item)
        return 0;
    oCItem* result = inventory2.Remove(item, amount);
    if (result && result->HasFlag(0x40000000))
        Equip(result);
    return result;
}

oCItem* oCNpc::RemoveFromInv(int instance, int amount)
{
    oCItem* result = inventory2.Remove(instance, amount);
    if (result && result->HasFlag(0x40000000))
        Equip(result);
    return result;
}

oCItem* oCNpc::RemoveFromInv(const zSTRING& name, int amount)
{
    return RemoveFromInv(parser.GetIndex(name), amount);
}

int oCNpc::IsSlotFree(const zSTRING& name)
{
    TNpcSlot* slot = GetInvSlot(name);
    return slot && !slot->vob;
}

int oCNpc::IsSlotFree(TNpcSlot* slot)
{
    return slot && !slot->vob;
}

void oCNpc::SetToSlotPosition(zCVob* vob, const zSTRING& nodeName)
{
    if (!vob)
        return;
    zMAT4 trafo = GetTrafoModelNodeToWorld(nodeName);
    vob->SetCollDetStat(0);
    vob->SetCollDetDyn(0);
    vob->SetTrafoObjToWorld(trafo);
    if (vob->effectVob)
        vob->effectVob->SetTrafoObjToWorld(trafo);
}

void oCNpc::CreateInvSlot(const zSTRING& name)
{
    TNpcSlot* slot = new TNpcSlot;
    slot->name = name;
    invSlot.Insert(slot);
}

void oCNpc::DeleteInvSlot(const zSTRING& name)
{
    TNpcSlot* slot = GetInvSlot(name);
    if (!slot)
        return;
    invSlot.RemoveOrder(slot);
    delete slot;
}

TNpcSlot* oCNpc::GetInvSlot(const zSTRING& name)
{
    for (int i = 0; i < invSlot.numInArray; ++i)
    {
        if (invSlot[i]->name == name)
            return invSlot[i];
    }
    return 0;
}

TNpcSlot* oCNpc::GetInvSlot(zCVob* vob)
{
    for (int i = 0; i < invSlot.numInArray; ++i)
    {
        if (invSlot[i]->vob == vob)
            return invSlot[i];
    }
    return 0;
}

int oCNpc::IsInvSlotAvailable(const zSTRING& name)
{
    return GetInvSlot(name) != 0;
}

void oCNpc::PutInSlot(const zSTRING& name, oCVob* vob, int inInventory)
{
    TNpcSlot* slot = GetInvSlot(name);
    PutInSlot(slot, vob, inInventory);
}

void oCNpc::PutInSlot(TNpcSlot* slot, oCVob* vob, int inInventory)
{
    if (!slot || !vob)
        return;
    if (slot->vob && slot->vob != vob)
        slot->ClearVob();
    slot->SetVob(vob);
    slot->inInventory = inInventory;
    oCItem* item = dynamic_cast<oCItem*>(vob);
    if (item)
        item->SetFlag(0x800000);
    oCItem* torch = GetSlotItem("ZS_LEFTHAND");
    SetTorchAni(torch && torch->GetObjectName() == "ITLSTORCHBURNING", 0);

    if (!inInventory)
    {
        vob->SetCollDetStat(0);
        vob->SetCollDetDyn(0);
        if (!vob->homeWorld)
        {
            zVEC3 origin = { 0.0f, 0.0f, 0.0f };
            vob->SetPositionWorld(origin);
            ogame->GetWorld()->AddVob(vob);
            SetToSlotPosition(vob, slot->name);
        }
        vob->showVisual = (vob->showVisual & ~0x20) | 0x20;
        vob->SetPhysicsEnabled(0);
        return;
    }

    if (item)
        item->CreateVisual();
    if (slot->name == "ZS_TORSO")
    {
        if (item && !item->GetVisualChange().IsEmpty())
        {
            item->SetFlag(0x40000000);
            if (homeWorld)
                InitModel();
        }
        return;
    }

    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (model)
    {
        zCModelNodeInst* node = model->SearchNode(slot->name);
        if (node)
            model->SetNodeVisual(node, vob->visual, 0);
    }
}

void oCNpc::RemoveFromAllSlots(int flags)
{
    for (int i = 0; i < invSlot.numInArray; ++i)
        RemoveFromSlot(invSlot[i], flags, 1);
}

oCVob* oCNpc::RemoveFromSlot(const zSTRING& name, int drop, int removeFromWorld)
{
    return RemoveFromSlot(GetInvSlot(name), drop, removeFromWorld);
}

oCVob* oCNpc::RemoveFromSlot(TNpcSlot* slot, int drop, int removeFromWorld)
{
    if (!slot)
        return 0;
    oCVob* vob = slot->vob;
    oCItem* item = dynamic_cast<oCItem*>(vob);
    if (item)
        item->ClearFlag(0x800000);

    if (slot->inInventory)
    {
        if (slot->name == "ZS_TORSO")
        {
            if (item)
                item->ClearFlag(0x40000000);
            if (visual)
                InitModel();
        }
        else
        {
            zCModel* model = GetModel();
            if (!model)
            {
                AvoidShrink(1000);
                model = GetModel();
            }
            if (model)
            {
                zCModelNodeInst* node = model->SearchNode(slot->name);
                if (node)
                    model->SetNodeVisual(node, 0, 0);
            }
        }
    }
    else if (drop)
        return DropFromSlot(slot);

    if (vob)
    {
        if (removeFromWorld && vob->homeWorld)
            vob->homeWorld->RemoveVob(vob);
        vob->showVisual &= ~0x20;
        slot->ClearVob();
    }
    CheckPutbackTorch();
    return vob;
}

oCVob* oCNpc::DropFromSlot(const zSTRING& name)
{
    return DropFromSlot(GetInvSlot(name));
}

oCVob* oCNpc::DropFromSlot(TNpcSlot* slot)
{
    if (!slot || !slot->vob)
        return 0;
    oCVob* vob = slot->vob;
    if (slot->inInventory)
    {
        zCModel* model = GetModel();
        if (model)
        {
            zCModelNodeInst* node = model->SearchNode(slot->name);
            if (node)
                model->SetNodeVisual(node, 0, 0);
        }
    }
    DoDropVob(vob);
    oCItem* item = dynamic_cast<oCItem*>(vob);
    if (item)
    {
        item->ClearFlag(0x40000000);
        item->ClearFlag(0x800000);
        if (IsAPlayer())
            item->SetFlag(0x400);
    }
    vob->showVisual &= ~0x20;
    slot->ClearVob();
    CheckPutbackTorch();
    return vob;
}

void oCNpc::UpdateSlots()
{
    for (int i = 0; i < invSlot.numInArray; ++i)
    {
        TNpcSlot* slot = invSlot[i];
        if (!slot || !slot->vob || slot->inInventory)
            continue;
        slot->vob->SetCollDetStat(0);
        slot->vob->SetCollDetDyn(0);
        SetToSlotPosition(slot->vob, slot->name);
    }
}

void oCNpc::SetInteractMob(oCMobInter* mob)
{
    interactMob = mob;
    zCModel* model = GetModel();
    if (!model && mob)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (model)
        model->n_bIsInMobInteraction = mob != 0;
}

oCMobInter* oCNpc::GetInteractMob()
{
    return static_cast<oCMobInter*>(interactMob);
}

int oCNpc::GetCharacterClass()
{
    return IsAPlayer() + 2;
}

void oCNpc::SetInteractItem(oCItem* item)
{
    if (interactItem && interactItem != item)
    {
        for (int i = invSlot.numInArray - 1; i >= 0; --i)
        {
            TNpcSlot* slot = invSlot[i];
            oCItem* slotItem = dynamic_cast<oCItem*>(slot->vob);
            if (!slotItem)
                continue;
            if (!slot->tmpLevel && slotItem->GetInstance() == interactItem->GetInstance())
            {
                oCItem* removed = static_cast<oCItem*>(RemoveFromSlot(slot, 0, 0));
                PutInInv(removed);
            }
            else if (slot->tmpLevel > 0)
            {
                oCVob* removed = RemoveFromSlot(slot, 0, 1);
                int temporary = slot->tmpLevel;
                if (temporary == 2)
                    DeleteInvSlot(slot->name);
                else
                    slot->tmpLevel = 0;
                if (removed)
                    removed->Release();
            }
        }
    }

    if (interactItem != item)
    {
        if (interactItem)
            interactItem->Release();
        interactItem = item;
        if (item)
            item->AddRef();
    }
}

void oCNpc::OnTouch(zCVob* vob)
{
    oCMOB* mob = dynamic_cast<oCMOB*>(vob);
    if (!mob)
        return;
    zCRigidBody* body = vob->GetRigidBody();
    float speedSquared = body->v.n[0] * body->v.n[0] + body->v.n[1] * body->v.n[1]
        + body->v.n[2] * body->v.n[2];
    if (speedSquared <= 10000.0f)
        return;
    oCAIVobMove* movement = dynamic_cast<oCAIVobMove*>(vob->callback_ai);
    if (!movement)
        return;
    zVEC3 hitLocation;
    for (int i = 0; i < 3; ++i)
        hitLocation.n[i] = trafoObjToWorld.m[i][3];
    OnDamage(vob, movement->owner, mob->damage, 4, hitLocation);
    movement->AddIgnoreCDVob(this);
}

void oCNpc::OnMessage(zCEventMessage* message, zCVob* sender)
{
    if (!message)
        return;
    if (ai_messagesSkip > 0.0f && ai_messagesSkipTimer < ai_messagesSkip)
    {
        message->Delete();
        return;
    }
    message->AddRef();
    if (!visual || !human_ai)
    {
        AvoidShrink(1000);
        if (!human_ai)
            Enable();
    }
    anictrl = static_cast<oCAniCtrl_Human*>(human_ai);
    int completed = 0;

    if (zCEvMsgCutscene* cutscene = dynamic_cast<zCEvMsgCutscene*>(message))
    {
        completed = 1;
    }
    else if (oCMsgDamage* damage = dynamic_cast<oCMsgDamage*>(message))
    {
        if (damage->subType == 0)
            completed = EV_DamageOnce(damage);
        else if (damage->subType == 1)
            completed = EV_DamagePerFrame(damage);
        else
            completed = 1;
    }
    else if (oCMsgMovement* movement = dynamic_cast<oCMsgMovement*>(message))
    {
        switch (movement->subType)
        {
        case 0: completed = EV_RobustTrace(movement); break;
        case 1: completed = EV_GotoPos(movement); break;
        case 2: completed = EV_GotoVob(movement); break;
        case 3: completed = EV_GoRoute(movement); break;
        case 4: completed = EV_Turn(movement); break;
        case 5: completed = EV_TurnToPos(movement); break;
        case 6: completed = EV_TurnToVob(movement); break;
        case 7: completed = EV_TurnAway(movement); break;
        case 8: completed = EV_Jump(movement); break;
        case 9: completed = EV_SetWalkMode(movement); break;
        case 10: completed = EV_WhirlAround(movement); break;
        case 11: completed = EV_StandUp(movement); break;
        case 12: completed = EV_CanSeeNpc(movement); break;
        case 13: completed = EV_Strafe(movement); break;
        case 14: completed = EV_GotoFP(movement); break;
        case 15: completed = EV_Dodge(movement); break;
        case 16: BeamTo(movement->targetPos); completed = 1; break;
        case 17: completed = EV_AlignToFP(movement); break;
        default: completed = 1; break;
        }
    }
    else if (oCMsgAttack* attack = dynamic_cast<oCMsgAttack*>(message))
    {
        switch (attack->subType)
        {
        case 0: completed = EV_AttackForward(attack); break;
        case 1: completed = EV_AttackLeft(attack); break;
        case 2: completed = EV_AttackRight(attack); break;
        case 3: completed = EV_AttackRun(attack); break;
        case 4: completed = EV_AttackFinish(attack); break;
        case 5: completed = EV_Parade(attack); break;
        case 6: completed = EV_AimAt(attack); break;
        case 7: completed = EV_ShootAt(attack); break;
        case 8: FinalizeAim(0, 1); completed = 1; break;
        case 9:
            isDefending = !didParade;
            if (didParade)
            {
                didParade = 0;
                completed = 1;
            }
            break;
        case 10: completed = EV_AttackBow(attack); break;
        case 11: EV_AttackMagic(attack); break;
        default: completed = 1; break;
        }
    }
    else if (oCMsgConversation* conversation = dynamic_cast<oCMsgConversation*>(message))
    {
        switch (conversation->subType)
        {
        case 0: completed = EV_PlayAniSound(conversation); break;
        case 1:
        case 14: completed = EV_PlayAni(conversation); break;
        case 2: completed = EV_PlaySound(conversation); break;
        case 3: completed = EV_LookAt(conversation); break;
        case 4: completed = EV_Output(conversation); break;
        case 5: completed = EV_OutputSVM(conversation); break;
        case 6: completed = EV_Cutscene(conversation); break;
        case 7: completed = EV_WaitTillEnd(conversation); break;
        case 8: completed = EV_Ask(conversation); break;
        case 9: completed = EV_WaitForQuestion(conversation); break;
        case 10: completed = EV_StopLookAt(conversation); break;
        case 11:
            if (lastPointMsg && GetEM(0)->IsRunning())
            {
                lastPointMsg->Delete();
                lastPointMsg = 0;
            }
            anictrl->StopCombineAni(anictrl->s_point);
            completed = 1;
            break;
        case 12: completed = EV_PointAt(conversation); break;
        case 13: completed = EV_QuickLook(conversation); break;
        case 15: StartFaceAni(conversation->name, 1.0f, -2.0f); completed = 1; break;
        case 16: completed = EV_ProcessInfos(conversation); break;
        case 17: completed = EV_StopProcessInfos(conversation); break;
        case 18: completed = EV_OutputSVM_Overlay(conversation); break;
        case 19:
        case 20: completed = EV_SndPlay(conversation); break;
        case 21: completed = EV_PrintScreen(conversation); break;
        case 22: completed = EV_StartFX(conversation); break;
        case 23: completed = EV_StopFX(conversation); break;
        default: completed = 1; break;
        }
    }
    else if (oCMsgMagic* magic = dynamic_cast<oCMsgMagic*>(message))
    {
        switch (magic->subType)
        {
        case 0: OpenSpellBook(magic->spellID); completed = 1; break;
        case 1: CloseSpellBook(magic->spellID); completed = 1; break;
        case 2:
            if (mag_book)
            {
                mag_book->SetFrontSpell(magic->activeSpell);
                if (magic->removeSymbol >= 0)
                    mag_book->Right();
                else
                    mag_book->Left();
            }
            completed = 1;
            break;
        case 7:
            if (mag_book)
                mag_book->SetFrontSpell(magic->activeSpell);
            completed = 1;
            break;
        case 8: completed = EV_CastSpell(magic); break;
        case 9: completed = ReadySpell(magic->manaInvested, magic->activeSpell); break;
        case 10: completed = UnreadySpell(); break;
        default: completed = 1; break;
        }
    }
    else if (oCMsgState* stateMessage = dynamic_cast<oCMsgState*>(message))
    {
        if (stateMessage->subType == 0)
            completed = EV_DoState(stateMessage);
        else
            completed = 1;
    }
    else if (oCMsgUseItem* useItem = dynamic_cast<oCMsgUseItem*>(message))
        completed = useItem->subType == 0 ? EV_Drink(useItem) : 1;
    else if (oCMsgManipulate* manipulate = dynamic_cast<oCMsgManipulate*>(message))
    {
        switch (manipulate->subType)
        {
        case 0: completed = EV_TakeVob(manipulate); break;
        case 1: completed = EV_DropVob(manipulate); break;
        case 2: completed = EV_ThrowVob(manipulate); break;
        case 3: completed = EV_Exchange(manipulate); break;
        case 4: completed = EV_UseMob(manipulate); break;
        case 5: completed = EV_UseItem(manipulate); break;
        case 6: completed = EV_InsertInteractItem(manipulate); break;
        case 7: SetInteractItem(0); completed = 1; break;
        case 8: completed = EV_CreateInteractItem(manipulate); break;
        case 9: completed = EV_DestroyInteractItem(manipulate); break;
        case 10: completed = EV_PlaceInteractItem(manipulate); break;
        case 11: completed = EV_ExchangeInteractItem(manipulate); break;
        case 12: SetInteractItem(dynamic_cast<oCItem*>(manipulate->targetVob)); completed = 1; break;
        case 13: completed = EV_CallScript(manipulate); break;
        case 14: completed = EV_EquipItem(manipulate); break;
        case 15: completed = EV_UseItemToState(manipulate); break;
        case 16: completed = EV_TakeMob(manipulate); break;
        case 17: completed = EV_DropMob(manipulate); break;
        default: completed = 1; break;
        }
    }
    else if (oCMsgWeapon* weaponMessage = dynamic_cast<oCMsgWeapon*>(message))
    {
        switch (weaponMessage->subType)
        {
        case 0: completed = EV_DrawWeapon(weaponMessage); break;
        case 1: completed = EV_DrawWeapon1(weaponMessage); break;
        case 2: completed = EV_DrawWeapon2(weaponMessage); break;
        case 3: completed = EV_RemoveWeapon(weaponMessage); break;
        case 4: completed = EV_RemoveWeapon1(weaponMessage); break;
        case 5: completed = EV_RemoveWeapon2(weaponMessage); break;
        case 6: completed = EV_ChooseWeapon(weaponMessage); break;
        case 7: completed = EV_ForceRemoveWeapon(weaponMessage); break;
        case 8: Fighting(); completed = 1; break;
        case 9: EquipBestWeapon(weaponMessage->targetMode); completed = 1; break;
        case 10: EquipBestArmor(); completed = 1; break;
        case 11: completed = EV_UnequipWeapons(weaponMessage); break;
        case 12: completed = EV_UnequipArmor(weaponMessage); break;
        case 13: completed = EV_EquipArmor(weaponMessage); break;
        default: completed = 1; break;
        }
    }
    else
        zCVob::OnMessage(message, sender);
    if (completed)
        message->Delete();
    message->Release();
}

zSTRING zCEvMsgCutscene::GetCutsceneName()
{
    return csName;
}

int oCNpc::EV_DrawWeapon(oCMsgWeapon* message)
{
    if (anictrl->GetWaterLevel() == 2)
        return 1;
    if (message->duringRun & 4)
    {
        if (GetWeaponMode())
            return 1;
        oCItem* torch = GetSlotItem("ZS_RIGHTHAND");
        if (torch && torch->HasFlag(0x10000000))
            SetTorchAni(2, 1);
        StandUp(1, 1);
        message->duringRun &= ~4;
    }
    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (message->duringRun & 2)
    {
        if (IsAPlayer())
        {
            static_cast<oCAIHuman*>(human_ai)->PC_Turnings(0);
            if (zinput->GetState(3) == 0.0f)
                anictrl->_Stand();
        }
        if (!(message->duringRun & 1))
            return EV_DrawWeapon2(message);
        zCModelAni* ani = message->ani == -1 ? 0 : model->GetAniFromAniID(message->ani);
        return !model->IsAniActive(ani);
    }

    if (model->IsAniActive(model->GetAniFromAniID(anictrl->_s_walkl)))
    {
        anictrl->StopTurnAnis();
        message->duringRun &= ~1;
        zSTRING weapon = oCAniCtrl_Human::GetWeaponString(GetNextWeaponMode(), message->targetMode - 1,
            message->duringRun & 1);
        message->ani = model->GetAniIDFromAniName("T_MOVE_2_" + weapon + "MOVE");
        oCVob* left = GetLeftHand();
        oCVob* right = GetRightHand();
        if (left)
            DoTakeVob(left);
        if (right)
            DoTakeVob(right);
        if (message->ani == -1)
            return 1;
        message->duringRun |= 1;
        anictrl->StartAni(message->ani, -1);
        message->duringRun |= 2;
        if (IsAPlayer())
        {
            int mode = GetWeaponMode();
            oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
            if (humanAI)
                humanAI->SetCamMode(mode >= 5 ? CamModNormal : CamModMelee, 0);
            oCNpcFocus::SetFocusMode(mode >= 5 ? 2 : 1);
        }
        return 0;
    }

    int nextStage = EV_DrawWeapon1(message);
    if (nextStage)
        message->duringRun |= 2;
    else
        message->duringRun &= ~2;
    return 0;
}

int oCNpc::EV_DrawWeapon1(oCMsgWeapon* message)
{
    if (!message->IsInUse())
    {
        oCVob* left = GetLeftHand();
        oCVob* right = GetRightHand();
        if (left)
            DoTakeVob(left);
        if (right)
            DoTakeVob(right);
        oCItem* torch = GetSlotItem("ZS_LEFTHAND");
        if (torch && torch->HasFlag(0x10000000))
            SetTorchAni(1, 0);
        message->SetInUse(1);
        anictrl->StopTurnAnis();
    }
    return anictrl->DrawWeapon1(message->targetMode, message->duringRun & 1,
        (message->duringRun >> 1) & 1);
}

int oCNpc::EV_DrawWeapon2(oCMsgWeapon* message)
{
    if (IsAPlayer())
    {
        int mode = GetWeaponMode();
        oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
        if (humanAI)
            humanAI->SetCamMode(mode >= 5 && mode != 7 ? CamModNormal : CamModMelee, 0);
        oCNpcFocus::SetFocusMode(mode == 7 ? 5 : mode >= 5 ? 2 : 1);
    }

    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (message->IsInUse())
    {
        if (model && (model->IsAniActive(model->GetAniFromAniID(anictrl->_s_walk))
            || model->IsAniActive(model->GetAniFromAniID(anictrl->_s_aim))))
        {
            anictrl->SetActionMode(0);
            anictrl->wmode_last = anictrl->wmode_selected;
            if (IsAPlayer())
                CreatePassivePerception(24, this, 0);
            return 1;
        }
        return 0;
    }

    zSTRING weapon = oCAniCtrl_Human::GetWeaponString(anictrl->wmode_selected);
    message->ani = model ? model->GetAniIDFromAniName("T_" + weapon + "MOVE_2_" + weapon + "MOVE") : -1;
    if (message->ani == -1)
        return 1;
    zSTRING walk = anictrl->walkmode == 3 ? zSTRING("WALK") : anictrl->GetWalkModeString();
    int transition = model->GetAniIDFromAniName(walk + "_2_" + weapon + "MOVE");
    anictrl->StartAni(message->ani, transition);
    message->SetInUse(1);
    return 0;
}

int oCNpc::EV_RemoveWeapon(oCMsgWeapon* message)
{
    if (message->duringRun & 4)
    {
        if (IsAPlayer() || !GetWeaponMode())
            return 1;
        oCItem* torch = GetSlotItem("ZS_RIGHTHAND");
        if (torch && torch->HasFlag(0x10000000))
            SetTorchAni(1, 1);
        StandUp(1, 1);
        message->duringRun &= ~4;
        anictrl->SetLookAtTarget(0);
        anictrl->StopTurnAnis();
    }
    if (IsAPlayer())
    {
        oCNpcFocus::SetFocusMode(0);
        oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
        if (humanAI)
            humanAI->SetCamMode(CamModNormal, 0);
    }
    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (message->duringRun & 2)
    {
        if (IsAPlayer())
        {
            static_cast<oCAIHuman*>(human_ai)->PC_Turnings(0);
            if (zinput->GetState(3) == 0.0f)
                anictrl->_Stand();
        }
        if (!(message->duringRun & 1))
            return EV_RemoveWeapon2(message);
        zCModelAni* ani = message->ani == -1 ? 0 : model->GetAniFromAniID(message->ani);
        return !model->IsAniActive(ani);
    }

    model->StopAnisLayerRange(20, 20);
    int mode = GetWeaponMode();
    int moving = model->IsAniActive(model->GetAniFromAniID(anictrl->s_run[mode]))
        || model->IsAniActive(model->GetAniFromAniID(anictrl->s_walk[mode]))
        || model->IsAniActive(model->GetAniFromAniID(anictrl->s_sneak[mode]));
    if (moving)
    {
        zSTRING weapon = oCAniCtrl_Human::GetWeaponString(mode);
        message->ani = model->GetAniIDFromAniName("T_" + weapon + "MOVE_2_MOVE");
        if (message->ani == -1)
            return 1;
        model->StartAni(message->ani, 0);
        message->duringRun |= 3;
        return 0;
    }

    anictrl->StopTurnAnis();
    message->duringRun &= ~1;
    int nextStage = EV_RemoveWeapon1(message);
    if (nextStage)
        message->duringRun |= 2;
    else
        message->duringRun &= ~2;
    return 0;
}

int oCNpc::EV_RemoveWeapon1(oCMsgWeapon*)
{
    if (IsAPlayer())
        return 1;
    anictrl->StopTurnAnis();
    if (!anictrl->RemoveWeapon1())
    {
        if (!GetWeaponMode() && anictrl->IsStanding())
            return 1;
        return 0;
    }
    return 1;
}

int oCNpc::EV_RemoveWeapon2(oCMsgWeapon* message)
{
    if (IsAPlayer())
        return 1;
    if (!message->IsInUse())
    {
        anictrl->wmode_selected = message->targetMode;
        message->SetInUse(1);
        if (mag_book)
            mag_book->Close(1);
        anictrl->StopTurnAnis();
    }
    if (!GetWeaponMode())
    {
        if (anictrl->RemoveWeapon2() || anictrl->IsStanding())
            return 1;
        return 0;
    }
    zCModel* model = GetModel();
    if (model)
        model->StartAni(anictrl->_s_walk, 0);
    return 1;
}

int oCNpc::EV_ChooseWeapon(oCMsgWeapon* message)
{
    if (!message->IsInUse())
    {
        anictrl->ChooseNextWeapon2(message->targetMode);
        message->SetInUse(1);
    }
    if (anictrl->IsStanding())
        return 1;
    if (anictrl->wmode_selected)
        return anictrl->IsStateAniActive(anictrl->s_weapon[anictrl->wmode_selected]);
    anictrl->SearchStandAni(0);
    return 1;
}

int oCNpc::EV_ForceRemoveWeapon(oCMsgWeapon*)
{
    if (IsAPlayer())
        return 1;
    oCItem* torch = GetSlotItem("ZS_LEFTHAND");
    if (!torch)
        torch = GetSlotItem("ZS_RIGHTHAND");
    if (torch && torch->HasFlag(0x10000000))
    {
        SetTorchAni(0, 1);
        TNpcSlot* slot = GetInvSlot(torch);
        RemoveFromSlot(slot, 1, 1);
        oCItem* unlit = static_cast<oCItem*>(ogame->GetWorld()->CreateVob(
            static_cast<zTVobType>(0x81), "ITLSTORCH"));
        PutInInv(unlit);
        if (unlit)
            unlit->Release();
    }

    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    int walkMode = humanAI ? humanAI->walkmode : 0;
    if (humanAI && (walkMode == 4 || walkMode == 5))
        humanAI->SetWalkMode(0);
    SetWeaponMode(0);
    if (humanAI && (walkMode == 4 || walkMode == 5))
        humanAI->SetWalkMode(walkMode);
    if (mag_book)
    {
        mag_book->Close(1);
        mag_book->SetShowHandSymbol(0);
    }
    static const char* handNames[] = { "ZS_RIGHTHAND", "ZS_LEFTHAND" };
    for (int i = 0; i < 2; ++i)
    {
        oCItem* item = GetSlotItem(handNames[i]);
        if (item)
        {
            oCItem* removed = static_cast<oCItem*>(RemoveFromSlot(handNames[i], 1, 1));
            PutInInv(removed);
        }
    }
    if (IsAPlayer())
    {
        oCNpcFocus::SetFocusMode(0);
        if (humanAI)
            humanAI->SetCamMode(CamModNormal, 0);
    }
    return 1;
}

int oCNpc::EV_EquipBestWeapon(oCMsgWeapon* message)
{
    EquipBestWeapon(message->targetMode);
    return 1;
}

void oCNpc::EquipBestWeapon(int flags)
{
    if (flags != 2 && flags != 4)
        return;
    inventory2.UnpackCategory();
    int count = inventory2.GetNumItemsInCategory();
    for (int i = 0; i < count; ++i)
    {
        oCItem* item = inventory2.GetItem(i);
        if (!item || !CanUse(item) || !item->HasFlag(flags))
            continue;
        if (flags == 4 && !IsMunitionAvailable(item))
            continue;
        if (!item->HasFlag(0x40000000))
            EquipWeapon(item);
        return;
    }
}

int oCNpc::EV_EquipBestArmor(oCMsgWeapon*)
{
    EquipBestArmor();
    return 1;
}

void oCNpc::EquipBestArmor()
{
    inventory2.UnpackCategory();
    int count = inventory2.GetNumItemsInCategory();
    for (int i = 0; i < count; ++i)
    {
        oCItem* item = inventory2.GetItem(i);
        if (item && CanUse(item) && item->HasFlag(0x10))
        {
            if (!item->HasFlag(0x40000000))
                EquipArmor(item);
            return;
        }
    }
}

int oCNpc::EV_UnequipWeapons(oCMsgWeapon*)
{
    UnequipItem(GetEquippedMeleeWeapon());
    UnequipItem(GetEquippedRangedWeapon());
    return 1;
}

int oCNpc::EV_UnequipArmor(oCMsgWeapon*)
{
    EquipArmor(GetEquippedArmor());
    return 1;
}

int oCNpc::EV_EquipArmor(oCMsgWeapon* message)
{
    oCItem* item = inventory2.IsIn(message->GetObjectName(), 1);
    if (item && CanUse(item) && item->HasFlag(0x10))
        EquipArmor(item);
    return 1;
}

static int NpcAttackCommon(oCNpc* npc, oCMsgAttack* message)
{
    if (!message->IsInUse() && message->attackAni != -1
        && (message->subType == 0 || message->attackAni != 0))
    {
        message->SetInUse(1);
        npc->anictrl->StopTurnAnis();
        zCVob* target = npc->IsAPlayer() ? npc->focus_vob : static_cast<zCVob*>(npc->enemy);
        npc->anictrl->StartHitCombo(message->attackAni, target);
        if (npc->enemy && npc->enemy->isDefending)
        {
            oCMsgAttack* parade = new oCMsgAttack(5, npc->anictrl->_t_hitr, 1);
            npc->enemy->GetEM(0)->OnMessage(parade, npc->enemy);
            npc->enemy->didParade = 1;
        }
    }
    if (npc->GetWeaponMode() <= 4)
        npc->anictrl->CheckMeleeWeaponHitsLevel(npc->GetSlotItem("ZS_RIGHTHAND"));
    npc->anictrl->HitCombo(0);
    npc->SetBodyState(27);
    zCModel* model = npc->GetModel();
    if (message->attackAni == -1)
        return 1;
    return model && !model->IsAniActive(model->GetAniFromAniID(message->attackAni));
}

static void NpcSendCombo(oCNpc* npc, int subType, int hitAni)
{
    oCMsgAttack* attack = new oCMsgAttack(subType, hitAni, 1);
    npc->GetEM(0)->OnMessage(attack, npc);
}

static void NpcSendStrafe(oCNpc* npc, int ani)
{
    oCMsgMovement* movement = new oCMsgMovement(13, npc->focus_vob);
    movement->ani = ani;
    npc->GetEM(0)->OnMessage(movement, npc);
}

int oCNpc::EV_AttackForward(oCMsgAttack* message)
{
    if (!message->IsInUse() && message->attackAni != -1)
        return NpcAttackCommon(this, message);

    if (!IsAPlayer())
    {
        if (message->combo != -1)
            anictrl->HitCombo(message->combo - 1 < anictrl->comboNr);
        else if (!anictrl->IsInCombo())
        {
            zCModel* model = GetModel();
            if (model)
                model->StartAni(anictrl->_s_walk, 0);
            return 1;
        }
        else
            anictrl->HitCombo(0);
        if (enemy)
            Turn(enemy->GetPositionWorld());
        return NpcAttackCommon(this, message);
    }

    int oldControls = oCGame::s_bUseOldControls;
    int left = oldControls ? zinput->GetToggled(2) || zinput->GetToggled(19)
        : zinput->GetToggled(27);
    int right = oldControls ? zinput->GetToggled(1) || zinput->GetToggled(18)
        : zinput->GetToggled(26);
    static_cast<oCAIHuman*>(human_ai)->PC_Turnings(0);

    if ((zinput->GetToggled(3) || zinput->GetToggled(4))
        && (anictrl->IsInCombo() || anictrl->IsInPostHit()))
    {
        zCModel* model = GetModel();
        if (model)
            model->StartAni(anictrl->_s_walk, 0);
        return 1;
    }

    if (zinput->GetState(19) != 0.0f && zinput->GetToggled(19)
        && (anictrl->IsInCombo() || anictrl->IsInPostHit()))
    {
        NpcSendStrafe(this, anictrl->_t_strafer);
        return 1;
    }
    if (zinput->GetState(18) != 0.0f && zinput->GetToggled(18)
        && (anictrl->IsInCombo() || anictrl->IsInPostHit()))
    {
        NpcSendStrafe(this, anictrl->_t_strafel);
        return 1;
    }
    if (zinput->GetToggled(5))
        anictrl->HitCombo(1);

    if (left && !anictrl->comboNr && anictrl->IsInCombo())
    {
        NpcSendCombo(this, 2, anictrl->_t_hitr);
        return 1;
    }
    if (right && (anictrl->comboNr == 1 || anictrl->comboNr == 2) && anictrl->IsInCombo())
    {
        NpcSendCombo(this, 1, anictrl->_t_hitl);
        return 1;
    }
    if (zinput->GetState(25) != 0.0f && (anictrl->IsInCombo() || anictrl->IsInPostHit()))
    {
        oCMsgAttack* parade = new oCMsgAttack(5, dynamic_cast<oCNpc*>(focus_vob), 0.0f);
        GetEM(0)->OnMessage(parade, this);
        return 1;
    }
    return NpcAttackCommon(this, message);
}

static int NpcAttackSide(oCNpc* npc, oCMsgAttack* message, int subType)
{
    if (!message->IsInUse() && message->attackAni)
        return NpcAttackCommon(npc, message);
    if (!npc->IsAPlayer())
    {
        if (message->combo == -1 && !npc->anictrl->IsInCombo())
            return 1;
        npc->anictrl->HitCombo(0);
        return NpcAttackCommon(npc, message);
    }

    int oldControls = oCGame::s_bUseOldControls;
    int left = oldControls ? zinput->GetToggled(2) || zinput->GetToggled(19)
        : zinput->GetToggled(27);
    int right = oldControls ? zinput->GetToggled(1) || zinput->GetToggled(18)
        : zinput->GetToggled(26);
    int same = subType == 1 ? left : right;
    int opposite = subType == 1 ? right : left;
    oCAniCtrl_Human* anictrl = npc->anictrl;

    if (zinput->GetState(5) != 0.0f)
    {
        if (same && anictrl->IsInCombo() && anictrl->canEnableNextCombo)
        {
            NpcSendCombo(npc, subType == 1 ? 2 : 1,
                subType == 1 ? anictrl->_t_hitr : anictrl->_t_hitl);
            return 1;
        }
        if (opposite)
        {
            zCEventMessage* active = npc->GetEM(0)->GetActiveMessage();
            if (!active || active->subType != subType)
                anictrl->HitCombo(1);
        }
        else if (zinput->GetState(25) == 0.0f)
            anictrl->HitCombo(0);
        if (!anictrl->IsInCombo() || !anictrl->canEnableNextCombo)
            anictrl->canEnableNextCombo = 0;
    }
    else if (anictrl->IsInCombo() || anictrl->IsInPostHit())
    {
        NpcSendCombo(npc, 0, anictrl->_t_hitf);
        return 1;
    }

    if (zinput->GetState(25) != 0.0f && (anictrl->IsInCombo() || anictrl->IsInPostHit()))
    {
        oCMsgAttack* parade = new oCMsgAttack(5, dynamic_cast<oCNpc*>(npc->focus_vob), 0.0f);
        npc->GetEM(0)->OnMessage(parade, npc);
        return 1;
    }
    if (zinput->GetState(11) != 0.0f && (left || right)
        && (anictrl->IsInCombo() || anictrl->IsInPostHit()))
    {
        NpcSendStrafe(npc, right ? anictrl->_t_strafer : anictrl->_t_strafel);
        return 1;
    }
    if (zinput->GetState(5) == 0.0f && (anictrl->IsInCombo() || anictrl->IsInPostHit()))
    {
        zCModel* model = npc->GetModel();
        if (model)
            model->StartAni(anictrl->_s_walk, 0);
        return 1;
    }
    return NpcAttackCommon(npc, message);
}

int oCNpc::EV_AttackLeft(oCMsgAttack* message)
{
    return NpcAttackSide(this, message, 1);
}

int oCNpc::EV_AttackRight(oCMsgAttack* message)
{
    return NpcAttackSide(this, message, 2);
}

int oCNpc::EV_AttackRun(oCMsgAttack* message)
{
    if (message->IsInUse() || !message->attackAni)
    {
        oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
        if (humanAI)
            humanAI->PC_Turnings(0);
        anictrl->HitCombo(0);
        if (IsAPlayer() && zinput->GetState(3) == 0.0f)
        {
            zCModel* model = GetModel();
            if (model)
                model->StartAni(anictrl->_s_walk, 0);
        }
    }
    return NpcAttackCommon(this, message);
}

int oCNpc::EV_AttackFinish(oCMsgAttack* message)
{
    oCNpc* target = dynamic_cast<oCNpc*>(message->attackTarget);
    int finishDistance = 180;
    zCPar_Symbol* symbol = parser.GetSymbol("NPC_ATTACK_FINISH_DISTANCE");
    if (symbol)
        symbol->GetValue(finishDistance, 0);

    zVEC3 position;
    if (target)
    {
        zVEC3 direction;
        float length = 0.0f;
        for (int i = 0; i < 3; ++i)
        {
            direction.n[i] = trafoObjToWorld.m[i][3] - target->trafoObjToWorld.m[i][3];
            length += direction.n[i] * direction.n[i];
        }
        length = sqrt(length);
        for (int i = 0; i < 3; ++i)
            position.n[i] = target->trafoObjToWorld.m[i][3];
        if (length > 0.0f)
        {
            for (int i = 0; i < 3; ++i)
                position.n[i] += direction.n[i] / length * finishDistance;
        }
    }

    if (!message->IsInUse())
    {
        if (!target)
            return 1;
        int unconsciousTime = 0;
        symbol = parser.GetSymbol("HAI_TIME_UNCONSCIOUS");
        if (symbol)
            symbol->GetValue(unconsciousTime, 0);
        if (target->state.GetStateTime() > unconsciousTime - 1)
            return 1;
        target->state.SetStateTime(0);
        if (IsAPlayer())
            SetEnemy(target);
        oCMsgMovement* movement = new oCMsgMovement(1, position);
        movement->SetHighPriority(1);
        GetEM(0)->OnMessage(movement, 0);
        message->SetInUse(1);
        message->duringRun &= ~2;
        return 0;
    }

    zCModel* model = GetModel();
    if (!(message->duringRun & 2))
    {
        SetPositionWorld(position);
        anictrl->StopTurnAnis();
        if (model && message->attackAni)
            model->StartAni(message->attackAni, 0);
        if (target && target->IsAIState(-4))
        {
            zCModel* targetModel = target->GetModel();
            if (targetModel)
            {
                if (targetModel->IsAniActive(targetModel->GetAniIDFromAniName("S_WOUNDED")))
                    targetModel->StartAni("T_WOUNDED_2_DEAD", 0);
                else if (targetModel->IsAniActive(targetModel->GetAniIDFromAniName("S_WOUNDEDB")))
                    targetModel->StartAni("T_WOUNDEDB_2_DEADB", 0);
            }
        }
        if (IsAPlayer())
            SetEnemy(target);
        message->duringRun |= 2;
        SetBodyState(27);
    }

    if (target && model && model->IsAniActive(model->GetAniFromAniID(message->attackAni)))
    {
        SetHeadingYWorld(target);
        if (IsAPlayer())
            SetEnemy(target);
        if (model->GetProgressPercent(message->attackAni) >= 0.5f && target->IsAIState(-4))
        {
            target->attribute[0] = 0;
            target->CheckModelOverlays();
        }
    }
    if (message->attackAni == -1)
        return 1;
    return model && !model->IsAniActive(model->GetAniFromAniID(message->attackAni));
}

int oCNpc::EV_Parade(oCMsgAttack* message)
{
    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (message->IsInUse())
    {
        if (IsAPlayer())
        {
            static_cast<oCAIHuman*>(human_ai)->PC_Turnings(0);
            if (oCGame::s_bUseOldControls && zinput->GetState(5) != 0.0f)
                TurnToEnemy();
        }
    }
    else
    {
        zSTRING direction = "0";
        if (enemy && IsInFightRange(enemy))
        {
            float azimuth;
            float elevation;
            GetAngles(enemy, azimuth, elevation);
            if (fabs(azimuth) < 30.0f && fabs(elevation) < 50.0f
                && GetDistanceToVob2(*enemy) < 9000000.0f)
            {
                oCAIHuman* enemyAI = static_cast<oCAIHuman*>(enemy->human_ai);
                if (enemyAI)
                    direction = enemyAI->GetHitDirection();
            }
        }

        zSTRING weapon = oCAniCtrl_Human::GetWeaponString(GetWeaponMode());
        zSTRING aniName;
        message->attackAni = -1;
        if (message->duringRun & 2)
            aniName = "T_" + weapon + "PARADEJUMPB";
        else
        {
            int variation = rand() % 3 + 1;
            aniName = "T_" + weapon + "PARADE_" + direction;
            if (variation > 1)
            {
                zSTRING numbered = aniName + "_A" + zSTRING(variation);
                message->attackAni = model ? model->GetAniIDFromAniName(numbered) : -1;
                if (message->attackAni != -1)
                    aniName = numbered;
            }
        }
        if (!(message->duringRun & 2) || message->attackAni == -1)
            message->attackAni = model ? model->GetAniIDFromAniName(aniName) : -1;
        if (message->attackAni == -1)
            return 1;
        message->SetInUse(1);
        anictrl->StartHitCombo(message->attackAni, 0);
        anictrl->StopTurnAnis();
        AssessDamage_S(enemy, 0);
    }

    SetBodyState(27);
    if (model && model->IsAniActive(model->GetAniFromAniID(message->attackAni)))
        return 0;
    return !anictrl->IsInCombo();
}

int oCNpc::InitAim(oCMsgAttack* message, oCNpc** targetNpc, int* ranged, int* munition,
    int removeOldMessages)
{
    if (targetNpc)
    {
        *targetNpc = dynamic_cast<oCNpc*>(message->target);
        if (!*targetNpc)
            *targetNpc = enemy;
        if (*targetNpc && (*targetNpc)->attribute[0] <= 0)
            *targetNpc = 0;
    }
    if (ranged)
        *ranged = GetWeaponMode() == 5 || GetWeaponMode() == 6;
    if (munition)
    {
        oCItem* weapon = 0;
        int mode = GetWeaponMode();
        if (mode > 1)
            weapon = GetSlotItem(mode > 4 && mode != 6 ? "ZS_LEFTHAND" : "ZS_RIGHTHAND");
        *munition = IsMunitionAvailable(weapon);
    }
    if (removeOldMessages)
    {
        zCEventManager* manager = GetEM(0);
        for (int i = 0; i < manager->GetNumMessages(); ++i)
        {
            oCMsgAttack* oldMessage = dynamic_cast<oCMsgAttack*>(manager->GetEventMessage(i));
            if (oldMessage && oldMessage != message && oldMessage->subType == message->subType)
                oldMessage->Delete();
        }
    }
    return 1;
}

int oCNpc::TransitionAim(int oldAni, int newAni)
{
    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (!model)
        return 0;
    if (newAni != -1 && model->IsAniActive(model->GetAniFromAniID(newAni)))
        return 1;

    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (!humanAI)
        return 0;
    if (oldAni == humanAI->_s_walk && newAni == humanAI->_s_aim)
        model->StartAni(humanAI->_t_walk_2_aim, 0);
    else if (oldAni == humanAI->_s_aim && newAni == humanAI->_s_hitf)
        model->StartAni(humanAI->_t_hitf, 0);
    else if (oldAni == humanAI->_s_hitf && newAni == humanAI->_s_aim)
        model->StartAni(humanAI->_t_hitback, 0);
    return 0;
}

int oCNpc::InterpolateAim(oCNpc* target)
{
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (!humanAI)
        return 0;
    AssessThreat_S(target);
    float azimuth;
    float elevation;
    GetAngles(target, azimuth, elevation);
    float targetX = azimuth / 90.0f + 0.5f;
    float targetY = 1.0f - (elevation / 90.0f + 0.5f);
    if (targetX >= 0.0f && targetX <= 1.0f && targetY >= 0.0f && targetY <= 1.0f)
    {
        hasLockedEnemy = 1;
        humanAI->InterpolateCombineAni(targetX, targetY, humanAI->_s_aim);
        return 1;
    }
    hasLockedEnemy = 0;
    Turn(target->trafoObjToWorld.m[3]);
    return 0;
}

int oCNpc::FinalizeAim(int removeWeapon, int standUp)
{
    int removedAim = 0;
    zCEventManager* manager = GetEM(0);
    for (int i = 0; i < manager->GetNumMessages(); ++i)
    {
        oCMsgAttack* message = dynamic_cast<oCMsgAttack*>(manager->GetEventMessage(i));
        if (message && message->subType == 6)
        {
            message->Delete();
            removedAim = 1;
        }
    }
    if (removeWeapon)
    {
        oCMsgWeapon* message = new oCMsgWeapon(3, 0, 0);
        message->SetHighPriority(1);
        manager->OnMessage(message, this);
    }
    if (standUp)
        StandUp(0, 1);
    return removedAim;
}

int oCNpc::EV_StopAim(oCMsgAttack*)
{
    FinalizeAim(0, 1);
    return 1;
}

int oCNpc::EV_AimAt(oCMsgAttack* message)
{
    oCNpc* target = 0;
    int ranged = 0;
    int munition = 0;
    InitAim(message, &target, &ranged, &munition, 1);
    if (!munition || !target)
    {
        FinalizeAim(1, 1);
        return 1;
    }
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (TransitionAim(humanAI->_s_walk, humanAI->_s_aim))
    {
        humanAI->StopTurnAnis();
        InterpolateAim(target);
    }
    return 0;
}

int oCNpc::EV_ShootAt(oCMsgAttack* message)
{
    if (!didShoot)
    {
        wasAiming = FinalizeAim(0, 0);
        if (!hasLockedEnemy)
            return EV_AimAt(message);
        DoShootArrow(1);
        didShoot = 1;
    }
    if (!didHit)
        didHit = TransitionAim(anictrl->_s_aim, anictrl->_s_hitf);
    if (didShoot && didHit && TransitionAim(anictrl->_s_hitf, anictrl->_s_aim))
    {
        didShoot = 0;
        didHit = 0;
        if (!wasAiming)
        {
            StandUp(0, 1);
            return 1;
        }
        oCMsgAttack* aimMessage = new oCMsgAttack(6, message->target, 0.0f);
        GetEM(0)->OnMessage(aimMessage, message->target);
        wasAiming = 0;
        return 1;
    }
    return 0;
}

int oCNpc::EV_Defend(oCMsgAttack*)
{
    if (!didParade)
    {
        isDefending = 1;
        return 0;
    }
    isDefending = 0;
    didParade = 0;
    return 1;
}

int oCNpc::EV_Drink(oCMsgUseItem* message)
{
    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (!model)
        return 1;

    if (!message->IsInUse())
    {
        message->ani = model->GetAniIDFromAniName("T_DRINKPOTION");
        anictrl->SetNextAni(anictrl->_s_walk, message->ani);
        model->StartAni(message->ani, 0);
        message->SetInUse(1);
        message->state = 0;
        return 0;
    }
    if (!model->IsAniActive(model->GetAniFromAniID(message->ani)))
        return 0;

    float progress = model->GetProgressPercent(message->ani);
    if (message->state == 0 && progress >= 0.15f)
    {
        if (!message->item)
            return 1;
        oCItem* item = inventory2.Remove(message->item, 1);
        if (!item)
            return 1;
        if (item->HasFlag(0x40000000))
            Equip(item);
        PutInSlot("ZS_LEFTHAND", item, 0);
        message->state = 1;
    }
    else if (message->state == 1 && progress >= 0.65f)
    {
        UseItem(message->item);
        message->state = 2;
    }
    else if (message->state == 2 && progress >= 0.85f)
    {
        oCVob* item = RemoveFromSlot("ZS_LEFTHAND", 0, 1);
        if (item)
            item->Release();
        message->state = 3;
    }
    return 0;
}

int oCNpc::EV_TakeVob(oCMsgManipulate* message)
{
    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (anictrl->GetWaterLevel() == 2)
    {
        if (message->targetVob && message->targetVob->homeWorld == homeWorld)
        {
            DoTakeVob(message->targetVob);
            message->targetVob = 0;
        }
        return 1;
    }

    if (!anictrl->IsStateAniActive(anictrl->_s_walk))
    {
        if (model->IsAniActive(model->GetAniFromAniID(anictrl->s_iget))
            || model->IsAniActive(model->GetAniFromAniID(anictrl->s_oget)))
        {
            if (message->targetVob && message->targetVob->homeWorld == homeWorld)
            {
                DoTakeVob(message->targetVob);
                message->targetVob = 0;
            }
            model->StartAni(anictrl->t_iget_2_stand, 0);
            anictrl->SetNextAni(anictrl->t_iget_2_stand, anictrl->_s_walk);
            message->SetInUse(1);
            message->state = anictrl->t_iget_2_stand;
        }
    }
    else
    {
        oCItem* item = dynamic_cast<oCItem*>(message->targetVob);
        message->state = model->GetAniIDFromAniName(item ? "T_STAND_2_IGET" : "T_STAND_2_OGET");
        SetBodyState(17);
        if (message->IsInUse())
            return 1;
        model->StartAni(message->state, 0);
        if (message->targetVob)
        {
            float height = trafoObjToWorld.m[1][3] - message->targetVob->trafoObjToWorld.m[1][3];
            message->aniCombY = (height + 100.0f) * 0.00499999989f;
            if (message->aniCombY < 0.0f)
                message->aniCombY = 0.0f;
            if (message->aniCombY > 1.0f)
                message->aniCombY = 1.0f;
        }
    }
    if (message->state != -1 && model->GetAniFromAniID(message->state))
        model->SetCombineAniXY(message->state, 0.5f, message->aniCombY);
    return 0;
}

int oCNpc::EV_DropVob(oCMsgManipulate* message)
{
    if (message->IsInUse())
    {
        if (message->targetVob)
            DoDropVob(message->targetVob);
        else if (!message->slot.IsEmpty())
            RemoveFromSlot(message->slot, 1, 1);
        else if (!message->name.IsEmpty())
        {
            oCItem* item = RemoveFromInv(parser.GetIndex(message->name), 1);
            if (item)
                DoDropVob(item);
        }
        zCModel* model = GetModel();
        if (model)
        {
            int ani = model->GetAniIDFromAniName("T_IDROP_2_STAND");
            anictrl->SetNextAni(anictrl->_s_walk, ani);
            model->StartAni(ani, 0);
        }
        return 0;
    }

    zCModel* model = GetModel();
    if (!model)
        return 1;
    model->StartAni(model->GetAniIDFromAniName("T_STAND_2_IDROP"), 0);
    message->SetInUse(1);
    return 0;
}

int oCNpc::EV_ThrowVob(oCMsgManipulate* message)
{
    zCModel* model = GetModel();
    if (!model)
        return 1;
    if (anictrl->IsStateAniActive(anictrl->_s_walk))
    {
        if (message->IsInUse())
            return 1;
        model->StartAni(anictrl->t_stand_2_iaim, 0);
        message->SetInUse(1);
        return 0;
    }
    if (anictrl->IsStateAniActive(anictrl->s_iaim))
    {
        model->StartAni(anictrl->t_iaim_2_ithrow, 0);
        return 0;
    }
    if (anictrl->IsStateAniActive(anictrl->s_ithrow))
    {
        oCVob* vob = GetRightHand();
        if (vob)
        {
            int mob = dynamic_cast<oCMOB*>(vob) != 0;
            oCNpcFocus::SetFocusMode(mob ? 4 : 3);
            float range = mob ? oCNpcFocus::focus.GetMobThrowRange()
                : oCNpcFocus::focus.GetItemThrowRange();
            float angle = mob ? 0.69813172022501635f : 1.2217305103937786f;
            float speed = sqrt(sqrt(range) * 981.0f / sin(angle)) * 0.9f;
            DoThrowVob(vob, speed);
            oCNpcFocus::SetFocusMode(0);
        }
        if (anictrl->t_ithrow_2_stand)
        {
            anictrl->SetNextAni(anictrl->t_ithrow_2_stand, anictrl->_s_walk);
            model->StartAni(anictrl->t_ithrow_2_stand, 0);
        }
        else
            model->StartAni(anictrl->_s_walk, 0);
        message->SetInUse(1);
    }
    return 0;
}

int oCNpc::EV_Exchange(oCMsgManipulate* message)
{
    zCModel* model = GetModel();
    if (!model)
        return 0;
    int ani = anictrl->_t_stand_2_torch;
    if (model->IsAniActive(model->GetAniFromAniID(ani)))
    {
        if (!message->IsInUse())
        {
            model->StartAni(ani, 0);
            return 0;
        }
        zCModelAniActive* active = model->GetActiveAni(ani);
        if (active && active->actFrame >= 4.0f)
        {
            oCVob* vob = GetRightHand();
            if (vob)
                DoThrowVob(vob, 35.0f);
            message->SetInUse(1);
        }
        return 0;
    }
    if (model->IsAniActive(model->GetAniFromAniID(anictrl->_s_walk)))
    {
        if (!message->IsInUse())
        {
            model->StartAni(ani, 0);
            return 0;
        }
        oCItem* torch = GetSlotItem("ZS_LEFTHAND");
        SetTorchAni(torch != 0, 1);
        return 1;
    }
    if (model->IsAniActive(model->GetAniFromAniID(anictrl->s_iaim)))
    {
        if (!message->IsInUse())
            model->StartAni(ani, 0);
        else
            model->StartAni(anictrl->_s_walk, 0);
    }
    return 0;
}

int oCNpc::EV_DropMob(oCMsgManipulate*)
{
    if (dynamic_cast<oCMobInter*>(carry_vob))
        DoDropVob(carry_vob);
    return 1;
}

int oCNpc::EV_TakeMob(oCMsgManipulate* message)
{
    oCMobInter* mob = FindMobInter(message->name);
    if (mob)
        DoTakeVob(mob);
    return 1;
}

int oCNpc::EV_UseMob(oCMsgManipulate* message)
{
    if (!message->targetVob)
        message->targetVob = static_cast<oCMobInter*>(interactMob);
    if (!message->targetVob && !message->name.IsEmpty())
    {
        message->name.Upper();
        oCMobInter* mob = FindMobInter(message->name);
        message->targetVob = mob;
        if (!mob)
            return 1;
        zVEC3 position;
        mob->GetFreePosition(this, position);
        oCMsgMovement* movement = new oCMsgMovement(1, position);
        movement->SetHighPriority(1);
        GetEM(0)->OnMessage(movement, 0);
        mob->MarkAsUsed(20000.0f, this);
        return 0;
    }

    oCMobInter* mob = dynamic_cast<oCMobInter*>(message->targetVob);
    if (!mob)
        return 0;
    if (mob->CanInteractWith(this) || mob->IsInteractingWith(this))
        return mob->AI_UseMobToState(this, message->targetState);
    return 1;
}

int oCNpc::EV_UseMobWithItem(oCMsgManipulate* message)
{
    SetInteractItem(dynamic_cast<oCItem*>(message->targetVob));
    return 1;
}

int oCNpc::EV_InsertInteractItem(oCMsgManipulate* message)
{
    if (!interactItem || message->slot.IsEmpty())
        return 1;
    if (GetSlotItem(message->slot) == interactItem)
        return 1;
    if (inventory2.IsIn(interactItem, 1))
    {
        oCItem* item = inventory2.Remove(interactItem, 1);
        if (item && item->HasFlag(0x40000000))
            Equip(item);
    }
    PutInSlot(message->slot, interactItem, 0);
    SetInteractItem(0);
    return 1;
}

int oCNpc::EV_ExchangeInteractItem(oCMsgManipulate* message)
{
    if (interactItem)
    {
        for (int i = invSlot.numInArray - 1; i >= 0; --i)
        {
            if (invSlot[i]->vob == interactItem)
                RemoveFromSlot(invSlot[i], 1, 1);
        }
        SetInteractItem(0);
    }

    oCItem* item = static_cast<oCItem*>(ogame->GetWorld()->CreateVob(
        static_cast<zTVobType>(0x81), message->name));
    if (item)
    {
        PutInInv(item);
        oCItem* newItem = RemoveFromInv(item->GetInstance(), 1);
        SetInteractItem(newItem);
        EV_InsertInteractItem(message);
        item->Release();
    }
    return 1;
}

int oCNpc::EV_RemoveInteractItem(oCMsgManipulate*)
{
    SetInteractItem(0);
    return 1;
}

int oCNpc::EV_CreateInteractItem(oCMsgManipulate* message)
{
    if (message->name.IsEmpty())
        return 1;
    oCItem* item = static_cast<oCItem*>(ogame->GetWorld()->CreateVob(
        static_cast<zTVobType>(0x81), message->name));
    if (!item)
        return 1;
    TNpcSlot* slot = GetInvSlot(message->slot);
    if (!slot)
    {
        CreateInvSlot(message->slot);
        slot = GetInvSlot(message->slot);
    }
    if (slot)
    {
        slot->tmpLevel = 2;
        PutInSlot(slot, item, 0);
    }
    SetInteractItem(item);
    item->Release();
    return 1;
}

int oCNpc::EV_DestroyInteractItem(oCMsgManipulate*)
{
    int instance = interactItem ? interactItem->GetInstance() : -1;
    SetInteractItem(0);
    if (instance >= 0)
    {
        oCItem* item = RemoveFromInv(instance, 1);
        if (item)
        {
            if (item->HasFlag(0x40000000))
                Equip(item);
            ogame->GetWorld()->RemoveVob(item);
        }
    }
    return 1;
}

zCModelAni* oCNpc::InteractItemGetAni(const zSTRING& slot, int state, int targetState)
{
    zSTRING stateName = state == -1 ? zSTRING("STAND") : "S" + zSTRING(state);
    zSTRING targetName = targetState == -1 ? zSTRING("STAND") : "S" + zSTRING(targetState);
    zSTRING aniName = "T_" + slot + "_" + stateName + "_" + targetName;
    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    return model ? model->GetAniFromAniID(model->GetAniIDFromAniName(aniName)) : 0;
}

int oCNpc::EV_PlaceInteractItem(oCMsgManipulate*)
{
    oCMobItemSlot* slot = dynamic_cast<oCMobItemSlot*>(interactMob);
    if (slot)
    {
        if (!interactItem)
        {
            PutInInv(slot->RemoveItem());
            return 1;
        }
        oCItem* item = interactItem;
        SetInteractItem(0);
        if (inventory2.IsIn(item, 1))
        {
            oCItem* removed = inventory2.Remove(item, 1);
            if (removed && removed->HasFlag(0x40000000))
                Equip(removed);
        }
        slot->PlaceItem(item);
        return 1;
    }

    if (interactItem)
    {
        oCItem* item = interactItem;
        SetInteractItem(0);
        item = RemoveFromInv(item->GetInstance(), 1);
        if (item && homeWorld)
            homeWorld->RemoveVob(item);
    }
    return 1;
}

int oCNpc::GetInteractItemMaxState(const zSTRING& slot)
{
    int state = 0;
    int result = -1;
    while (InteractItemGetAni(slot, state, 0))
    {
        result = state;
        ++state;
    }
    return result;
}

int oCNpc::InteractItemIsInState(const zSTRING& slot, int state)
{
    if (state == -1)
        return anictrl->IsStanding();
    zCModelAni* ani = InteractItemGetAni(slot, state, 0);
    zCModel* model = GetModel();
    return model && model->IsAniActive(ani);
}

int oCNpc::EV_EquipItem(oCMsgManipulate* message)
{
    if (!message->targetVob)
        message->targetVob = inventory2.IsIn(message->name, 1);
    oCItem* item = dynamic_cast<oCItem*>(message->targetVob);
    if (!item)
        return 1;
    if (!item->HasFlag(0x40000000) && !CanUse(item) && this == player)
    {
        DisplayCannotUse();
        return 1;
    }
    Equip(item);
    return 1;
}

int oCNpc::EV_UseItem(oCMsgManipulate* message)
{
    if (!message->targetVob)
        message->targetVob = inventory2.IsIn(message->name, 1);
    if (!message->targetVob)
        return 1;
    oCItem* item = dynamic_cast<oCItem*>(message->targetVob);
    if (item)
    {
        message->name = item->GetSchemeName();
        interactItemCurrentState = -1;
        message->targetState = GetInteractItemMaxState(message->name);
    }
    int result = EV_UseItemToState(message);
    if (message->targetState >= 0)
    {
        message->targetState = -1;
        return 0;
    }
    return result;
}

int oCNpc::EV_UseItemToState(oCMsgManipulate* message)
{
    if (!message->IsInUse())
    {
        if (!message->targetVob)
            message->targetVob = inventory2.IsIn(message->name, 1);
        if (!message->targetVob && interactItem
            && interactItem->GetInstanceName() == message->name)
            message->targetVob = interactItem;
        oCItem* item = dynamic_cast<oCItem*>(message->targetVob);
        if (!item)
            return 1;
        message->name = item->GetSchemeName();
        if (message->name.IsEmpty())
            return 1;
        message->flag = 0;
        message->SetInUse(1);
        SetInteractItem(item);
    }

    interactItemTargetState = message->targetState;
    SetBodyState(14);
    if (!interactItem && interactItemTargetState >= 0)
        SetInteractItem(dynamic_cast<oCItem*>(message->targetVob));
    if (!interactItem || interactItemTargetState < 0)
    {
        interactItemCurrentState = -1;
        oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
        if (humanAI)
            humanAI->SearchStandAni(1);
        return 1;
    }

    if (interactItemCurrentState != interactItemTargetState)
    {
        int ready = anictrl->IsStanding();
        zCModel* model = GetModel();
        if (interactItemCurrentState != -1)
        {
            zSTRING currentName = message->name + "_2_S" + zSTRING(interactItemCurrentState);
            ready = model && model->IsAniActive(model->GetAniFromAniID(
                model->GetAniIDFromAniName(currentName)));
        }
        if (ready)
        {
            if (interactItem && !message->flag)
            {
                parser.SetInstance("SELF", this);
                parser.SetInstance("ITEM", interactItem);
                int function = interactItem->GetStateEffectFunc(interactItemCurrentState);
                if (function >= 0)
                    parser.CallFunc(function);
                message->flag = 1;
            }

            zCModelAni* ani = InteractItemGetAni(message->name, interactItemCurrentState,
                interactItemTargetState);
            if (ani)
            {
                model->StartAni(ani, 0);
                interactItemCurrentState = interactItemTargetState;
                message->flag = 0;
            }
            else
            {
                int direction = interactItemCurrentState < interactItemTargetState ? 1 : -1;
                ani = InteractItemGetAni(message->name, interactItemCurrentState,
                    interactItemCurrentState + direction);
                if (ani)
                {
                    model->StartAni(ani, 0);
                    interactItemCurrentState += direction;
                    message->flag = 0;
                }
                else
                    interactItemTargetState = interactItemCurrentState;
            }
        }
    }

    if (interactItemCurrentState >= 0 && interactItemCurrentState == interactItemTargetState
        && anictrl->IsStateAniActive(anictrl->_s_walk))
        return 1;
    return InteractItemIsInState(message->name, interactItemTargetState);
}

void oCNpc::CallScript(const zSTRING& function)
{
    int index = parser.GetIndex(function);
    if (index >= 0)
        parser.CallFunc(index);
}

void oCNpc::CallScript(int function)
{
    if (function >= 0)
        parser.CallFunc(function);
}

int oCNpc::EV_CallScript(oCMsgManipulate* message)
{
    parser.SetInstance("SELF", this);
    parser.SetInstance("OTHER", player);
    if (message->targetState >= 0)
        parser.CallFunc(message->targetState);
    else if (message->flag)
        parser.CallFunc(message->name);
    return 1;
}

int oCNpc::EV_Unconscious(oCMsgState* message)
{
    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (!model)
        return 1;

    if (!model->IsAniActive(model->GetAniFromAniID(model->GetAniIDFromAniName("S_KNOCKEDOUT"))))
    {
        int knocked = model->GetAniIDFromAniName("T_KNOCKEDOUT");
        if (!model->IsAniActive(model->GetAniFromAniID(knocked)))
        {
            model->StartAni(knocked, 0);
            attribute[0] = 1;
            CheckModelOverlays();
        }
        return 0;
    }

    message->timer -= ztimer.frameTimeFloat;
    if (message->timer > 0.0f)
        return 0;
    model->StartAni(model->GetAniIDFromAniName("T_KNOCKEDOUT_2_STAND"), 0);
    return 1;
}

int oCNpc::EV_DoState(oCMsgState* message)
{
    if (!message->wp.IsEmpty())
        wpname = message->wp;
    parser.SetInstance("OTHER", message->other);
    parser.SetInstance("VICTIM", message->victim);
    ClearEM();
    if (message->endOldState)
        state.StartAIState(message->function, message->endOldState);
    else
    {
        state.curStateValid = 0;
        state.nextStateValid = 0;
        state.StartRtnState();
    }
    return 1;
}

int oCNpc::EV_Wait(oCMsgState* message)
{
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (humanAI)
    {
        if (humanAI->IsWalking() || humanAI->actionMode == 5 || humanAI->actionMode == 6)
            humanAI->_Stand();
        humanAI->StopTurnAnis();
    }
    message->timer -= ztimer.frameTimeFloat;
    return message->timer <= 0.0f;
}

int oCNpc::ApplyTimedOverlayMds(const zSTRING& overlay, float time)
{
    if (!visual || !human_ai)
        return 0;
    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (!model)
        return 0;
    model->ApplyModelProtoOverlay(overlay);
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    humanAI->InitAnimations();
    oCNpcTimedOverlay* timedOverlay = new oCNpcTimedOverlay;
    timedOverlay->mdsOverlayName = overlay;
    timedOverlay->timer = time;
    timedOverlays.Insert(timedOverlay);
    return 1;
}

int oCNpc::EV_OutputSVM_Overlay(oCMsgConversation* message)
{
    zCCSManager* manager = ogame->GetCutsceneManager();
    if (message->number != -1)
    {
        int result = message->number >= 0 ? EV_PlaySound(message) : 1;
        if (result && message->number >= 0)
            manager->LibSvmModuleStop(message->number);
        return result;
    }

    oCNpc* target = dynamic_cast<oCNpc*>(message->target);
    if (target && target->GetTalkingWith() && target->GetTalkingWith() != this)
        return 1;
    message->number = ogame->GetSVMManager()->GetOU(voice, message->name);
    if (message->number < 0 || !manager->LibValidateOU(message->number)
        || manager->LibIsSvmModuleRunning(message->number))
        return 1;
    zCCSCutsceneContext* context = ogame->GetWorld()->csPlayer->StartOutputUnit(message->number, this);
    if (!context)
    {
        message->number = -1;
        return 1;
    }
    StartTalkingWith(target);
    manager->LibSvmModuleStart(message->number);
    return EV_PlaySound(message);
}

int oCNpc::EV_OutputSVM(oCMsgConversation* message)
{
    oCNpc* target = dynamic_cast<oCNpc*>(message->target);
    if (target && target->GetTalkingWith() && target->GetTalkingWith() != this)
        return 1;
    message->number = ogame->GetSVMManager()->GetOU(voice, message->name);
    if (message->number >= 0 && ogame->GetCutsceneManager()->LibValidateOU(message->number))
    {
        StartTalkingWith(target);
        state.StartOutputUnit(message->number);
    }
    return 1;
}

int oCNpc::EV_Output(oCMsgConversation* message)
{
    oCNpc* target = dynamic_cast<oCNpc*>(message->target);
    if (target && target->GetTalkingWith() && target->GetTalkingWith() != this)
        return 1;
    message->number = ogame->GetCutsceneManager()->LibValidateOU((zSTRING&)message->name);
    if (message->number >= 0)
    {
        StartTalkingWith(target);
        state.StartOutputUnit(message->number);
    }
    return 1;
}

int oCNpc::EV_PlayAni(oCMsgConversation* message)
{
    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (!model)
        return 1;
    if (!message->IsInUse())
    {
        if (message->name.Search("_2_STAND", 0) > 0
            && model->IsAniActive(model->GetAniFromAniID(anictrl->_s_walk)))
            return 1;
        model->StartAni(message->ani, 0);
        if (interactMob)
            static_cast<oCMobInter*>(interactMob)->SetMobBodyState(message->subType);
        else
            SetBodyState(message->subType & 0x7f);
        message->SetInUse(1);
    }
    return message->ani == -1 || !model->IsAniActive(model->GetAniFromAniID(message->ani));
}

int oCNpc::EV_PlayAniFace(oCMsgConversation* message)
{
    StartFaceAni(message->name, -2.0f, 1.0f);
    return 1;
}

int oCNpc::StartDialogAni()
{
    if (GetWeaponMode())
        return -1;
    zCModel* model = GetModel();
    if (!model)
    {
        AvoidShrink(1000);
        model = GetModel();
    }
    if (!model)
        return -1;
    int gesture = rand() % 20 + 1;
    zSTRING number = gesture < 10 ? zSTRING("0") + zSTRING(gesture) : zSTRING(gesture);
    int ani = model->GetAniIDFromAniName("T_DIALOGGESTURE_" + number);
    model->StartAni(ani, 0);
    return ani;
}

int oCNpc::EV_PlayAniSound(oCMsgConversation* message)
{
    if (!message->IsInUse())
    {
        message->ani = StartDialogAni();
        message->SetInUse(1);
    }
    if (EV_PlaySound(message))
    {
        zCModel* model = GetModel();
        if (model)
            model->FadeOutAni(message->ani);
        return 1;
    }
    return 0;
}

int oCNpc::ActivateDialogCam(float duration)
{
    if (!GetTalkingWith())
        return 0;
    zCAICamera* camera = ogame->GetCameraAI();
    if (duration != 0.0f)
        camera->SetDialogCamDuration(duration);
    camera->SetMode("DIALOG");
    return 1;
}

int oCNpc::DeactivateDialogCam()
{
    if (!s_bEnabledDialogCamStop)
        return 0;
    oCNpc* target = GetTalkingWith();
    if (!target || target->state.IsInState(parser.GetIndex("ZS_TALK")))
    {
        dialogCamStopTimer += ztimer.frameTimeFloat;
        if (dialogCamStopTimer >= 1000.0f)
        {
            ogame->GetCameraAI()->SetMode(CamModNormal);
            dialogCamStopTimer = 0.0f;
        }
        return 1;
    }
    dialogCamStopTimer = 0.0f;
    return 0;
}

int oCNpc::EV_PlaySound(oCMsgConversation* message)
{
    message->f_yes = 2;
    if (!message->IsInUse())
    {
        message->handle = 0;
        message->f_no = 0;
        zCSoundFX* sound = zsound->LoadSoundFX(message->name);
        if (sound)
        {
            zCSoundSystem::zTSound3DParams params;
            params.SetDefaults();
            params.pitchOffset = voicePitch;
            params.reverbLevel = oCGame::s_fUseSpeechReverb == 0.0f ? 0.0f
                : oCGame::s_fUseSpeechReverb == 1.0f ? 0.5f : 1.0f;
            message->handle = zsound->PlaySound3D(sound, this, 2, &params);
            listOfVoiceHandles.Insert(message->handle);
            float playingTime = sound->GetPlayingTimeMSEC();
            if (playingTime >= 50.0f)
                message->f_no = static_cast<int>(playingTime);
            sound->Release();
        }
        if (message->f_no <= 0)
        {
            message->f_no = static_cast<int>(message->text.length * zCView::s_fViewTimePerChar);
            if (message->f_no < 1000)
                message->f_no = 1000;
            if (message->f_no > 8000)
                message->f_no = 8000;
        }
        StartFaceAni("TALK", 1.0f, -2.0f);

        if (!message->text.IsEmpty() && isEnabledTalkBox)
        {
            zCView* view = ogame->array_view[5];
            int dialog = 0;
            if (homeWorld && homeWorld->csPlayer->GetPlayingGlobalCutscene())
            {
                view = ogame->array_view[3];
                dialog = 1;
            }
            else if (GetTalkingWith())
            {
                view = ogame->array_view[1];
                dialog = 1;
            }
            if (view && (dialog || isEnabledTalkBoxNoise))
            {
                zCOLOR color(255, 255, 255, 255);
                view->DialogMessageCXY(message->text, GetName(), message->f_no, color);
            }
        }
        ActivateDialogCam(static_cast<float>(message->f_no));
        message->SetInUse(1);
        return 0;
    }

    message->f_no -= static_cast<int>(ztimer.frameTime);
    if (message->f_no <= 500)
    {
        zCModel* model = GetModel();
        if (model)
        {
            for (int i = 0; i < model->numActiveAnis; ++i)
            {
                zCModelAniActive* active = &model->activeAniList[i];
                if (active->protoAni && active->protoAni->aniName.Search("DIALOGGESTURE_", 0) >= 0)
                    model->FadeOutAni(active);
            }
        }
        StopFaceAni("TALK");
    }
    if (message->f_no <= 0)
    {
        StopFaceAni("TALK");
        StopTalkingWith();
        return 1;
    }
    return 0;
}

int oCNpc::EV_SndPlay(oCMsgConversation* message)
{
    if (message->IsInUse())
        return 1;
    message->SetInUse(1);
    zCSoundFX* sound = zsound->LoadSoundFX(message->name);
    if (sound)
    {
        message->timer = sound->GetPlayingTimeMSEC();
        message->f_no = 0;
        if (message->target)
        {
            message->handle = zsound->PlaySound3D(sound, message->target, 0, 0);
            message->f_no = static_cast<int>(sound->GetPlayingTimeMSEC());
            listOfVoiceHandles.Insert(message->handle);
        }
        else
            zsound->PlaySound(sound, 0);
        sound->Release();
    }
    return 1;
}

int oCNpc::EV_PrintScreen(oCMsgConversation* message)
{
    zCView* view = ogame->array_view[0];
    if (!message->name.IsEmpty())
    {
        view->SetFont(message->name);
        if (view->GetFont()->GetFontName() != message->name)
            return 1;
    }
    float time = message->timer * 1000.0f;
    int x = static_cast<int>(message->f_no * 81.9199982f);
    int y = static_cast<int>(message->f_yes * 81.9199982f);
    if (message->f_no == -1 && message->f_yes == -1)
        view->PrintTimedCXY(message->text, time);
    else if (message->f_no == -1)
        view->PrintTimedCX(y, message->text, time);
    else if (message->f_yes == -1)
        view->PrintTimedCY(x, message->text, time);
    else
        view->PrintTimed(x, y, message->text, time);
    return 1;
}

void oCNpc::AddEffect(const zSTRING& effectName, const zCVob* target)
{
    if (!oCItem::s_bItemEffectEnabled || effectName.IsEmpty())
        return;
    for (int i = 0; i < effectList.numInArray; ++i)
    {
        if (effectList[i]->GetName() == effectName)
            return;
    }
    oCVisualFX* effectObject = oCVisualFX::CreateAndPlay(effectName, this,
        const_cast<zCVob*>(target), 0, 0.0f, 0, 1);
    if (effectObject)
    {
        if (visual)
            effectObject->SetPFXShapeVisual(visual, 0);
        effectList.Insert(effectObject);
    }
}

void oCNpc::RemoveEffect(const zSTRING& effectName)
{
    if (effectName.IsEmpty())
        return;
    for (int i = effectList.numInArray - 1; i >= 0; --i)
    {
        oCVisualFX* effectObject = effectList[i];
        if (effectObject->GetName() == effectName)
        {
            effectList.RemoveOrder(effectObject);
            effectObject->Stop(1);
            effectObject->Release();
        }
    }
}

int oCNpc::EV_StartFX(oCMsgConversation* message)
{
    AddEffect(message->name, message->target);
    return 1;
}

int oCNpc::EV_StopFX(oCMsgConversation* message)
{
    RemoveEffect(message->name);
    return 1;
}

int oCNpc::EV_LookAt(oCMsgConversation* message)
{
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (!message->IsInUse())
    {
        zCEventManager* manager = GetEM(0);
        int found = 0;
        for (int i = 0; i < manager->GetNumMessages(); ++i)
        {
            oCMsgConversation* queued = dynamic_cast<oCMsgConversation*>(manager->GetEventMessage(i));
            if (!queued)
                continue;
            if (queued == message)
                found = 1;
            else if (found && queued->subType == 3)
                queued->Delete();
        }
        if (!message->target)
        {
            zCWaypoint* waypoint = homeWorld->wayNet->GetWaypoint(message->name);
            if (waypoint)
                message->targetPos = waypoint->GetPositionWorld();
            else
                message->target = homeWorld->SearchVobByName(message->name);
        }
        if (!message->target || humanAI->targetVob == message->target)
            message->SetInUse(1);
        else
        {
            humanAI->StopLookAtTarget();
            zCModel* model = GetModel();
            if (model)
            {
                humanAI->StopCombineAni(model->GetAniIDFromAniName("T_QLOOK"));
                humanAI->StopCombineAni(model->GetAniIDFromAniName("S_TLOOK"));
            }
            message->SetInUse(1);
        }
    }
    lastLookMsg = message;
    if (message->ani == -1)
    {
        zCModel* model = GetModel();
        message->ani = model ? model->GetAniIDFromAniName("S_TLOOK") : -1;
    }
    if (message->target)
        humanAI->StartCombineAni(message->target, message->ani, 45.0f, 60.0f);
    else
        humanAI->StartCombineAni(message->targetPos, message->ani, 45.0f, 60.0f);
    return 0;
}

int oCNpc::EV_StopLookAt(oCMsgConversation*)
{
    lastLookMsg = 0;
    anictrl->StopLookAtTarget();
    return 1;
}

int oCNpc::EV_PointAt(oCMsgConversation* message)
{
    if (!message->target && !message->IsInUse())
    {
        zCWaypoint* waypoint = homeWorld->wayNet->GetWaypoint(message->name);
        if (waypoint)
            message->targetPos = waypoint->GetPositionWorld();
        else
            message->target = homeWorld->SearchVobByName(message->name);
        message->SetInUse(1);
    }
    if (message->target)
    {
        for (int i = 0; i < 3; ++i)
            message->targetPos.n[i] = message->target->trafoObjToWorld.m[i][3];
    }
    lastPointMsg = message;
    float azimuth;
    float elevation;
    GetAngles(message->targetPos, azimuth, elevation);
    if (fabs(azimuth) <= 90.0f && fabs(elevation) <= 45.0f)
    {
        if (message->target)
            anictrl->StartCombineAni(message->target, anictrl->s_point, 45.0f, 90.0f);
        else
            anictrl->StartCombineAni(message->targetPos, anictrl->s_point, 45.0f, 90.0f);
    }
    else
        anictrl->StopCombineAni(anictrl->s_point);
    return 0;
}

int oCNpc::EV_StopPointAt(oCMsgConversation*)
{
    lastPointMsg = 0;
    anictrl->StopCombineAni(anictrl->s_point);
    return 1;
}

int oCNpc::EV_QuickLook(oCMsgConversation* message)
{
    if (!message->IsInUse())
    {
        message->number = 2000;
        zCModel* model = GetModel();
        message->ani = model ? model->GetAniIDFromAniName("T_QLOOK") : -1;
        message->SetInUse(1);
    }
    if (message->target)
    {
        float azimuth;
        float elevation;
        GetAngles(message->target, azimuth, elevation);
        float targetX = azimuth / 120.0f + 0.5f;
        float targetY = 1.0f - (elevation / 120.0f + 0.5f);
        if (targetX < 0.0f)
            targetX = 0.0f;
        if (targetX > 1.0f)
            targetX = 1.0f;
        if (targetY < 0.0f)
            targetY = 0.0f;
        if (targetY > 1.0f)
            targetY = 1.0f;
        anictrl->InterpolateCombineAni(targetX, targetY, message->ani);
    }
    message->number -= static_cast<int>(ztimer.frameTime);
    if (message->number > 0)
        return 0;
    anictrl->StopCombineAni(message->ani);
    return 1;
}

int oCNpc::EV_Cutscene(oCMsgConversation* message)
{
    if (message->name.IsEmpty())
        return 1;
    if (message->name.Search(".CS", 0) <= 0)
        message->name += ".CS";
    state.StartCutscene(message->name, message->text);
    return 1;
}

int oCNpc::EV_WaitTillEnd(oCMsgConversation* message)
{
    oCNpc* target = dynamic_cast<oCNpc*>(message->target);
    if (!target)
        return 1;
    oCAIHuman* humanAI = static_cast<oCAIHuman*>(human_ai);
    if (humanAI)
    {
        humanAI->StopTurnAnis();
        if (IsAPlayer())
            StandUp(0, 1);
    }
    return !target->GetEM(0)->IsRunning() && !target->GetEM(0)->GetCutsceneMode();
}

int oCNpc::EV_StopProcessInfos(oCMsgConversation*)
{
    oCInformationManager::GetInformationManager().Exit();
    StopTalkingWith();
    return 1;
}

int oCNpc::EV_ProcessInfos(oCMsgConversation* message)
{
    oCInformationManager& manager = oCInformationManager::GetInformationManager();
    if (message->IsInUse())
    {
        manager.Update();
        return manager.HasFinished();
    }
    if (GetEM(0)->GetCutsceneMode())
        return 1;
    oCNpc* target = dynamic_cast<oCNpc*>(message->target);
    if (target && target->GetEM(0)->GetCutsceneMode())
        return 1;
    manager.SetNpc(target);
    message->SetInUse(1);
    return 0;
}

static int NpcAssessPerc(oCNpc* npc, int percID, void* other)
{
    for (int i = 0; i < npc->percActive; ++i)
    {
        if (npc->percList[i].percID != percID)
            continue;
        int function = npc->percList[i].percFunc;
        if (function >= 0)
        {
            zCCutscene* cutscene = npc->GetEM(0)->GetCutscene();
            zCCSCutsceneContext* context = dynamic_cast<zCCSCutsceneContext*>(cutscene);
            if (context && !context->IsPlaying()
                && (!cutscene->properties || !cutscene->properties->globalCutscene))
                cutscene->Stop();
            parser.SetInstance("SELF", npc);
            parser.SetInstance("OTHER", static_cast<zCVob*>(other));
            npc->state.StartAIState(function, 0, 0, 0.0f, 0);
            return 1;
        }
        break;
    }
    return 0;
}

int oCNpc::AssessPlayer_S(oCNpc* other)
{
    return NpcAssessPerc(this, 1, other);
}

int oCNpc::AssessEnemy_S(oCNpc* other)
{
    return NpcAssessPerc(this, 2, other);
}

int oCNpc::AssessFighter_S(oCNpc* other)
{
    return NpcAssessPerc(this, 3, other);
}

int oCNpc::AssessBody_S(oCNpc* other)
{
    return NpcAssessPerc(this, 4, other);
}

int oCNpc::AssessItem_S(oCItem* other)
{
    for (int i = 0; i < percActive; ++i)
    {
        if (percList[i].percID != 5)
            continue;
        int function = percList[i].percFunc;
        if (function >= 0)
        {
            zCCutscene* cutscene = GetEM(0)->GetCutscene();
            zCCSCutsceneContext* context = dynamic_cast<zCCSCutsceneContext*>(cutscene);
            if (context && !context->IsPlaying()
                && (!cutscene->properties || !cutscene->properties->globalCutscene))
                cutscene->Stop();
            parser.SetInstance("SELF", this);
            parser.SetInstance("ITEM", other);
            state.StartAIState(function, 0, 0, 0.0f, 0);
            return 1;
        }
        break;
    }
    return 0;
}

static void NpcCreateVobList(oCNpc* npc, zCArray<zCVob*>& vobs, float range)
{
    if (!npc->homeWorld)
        return;
    npc->homeWorld->SearchVobListByClass(oCNpc::classDef, vobs, 0);
    float rangeSquared = range * range;
    for (int i = vobs.numInArray - 1; i >= 0; --i)
    {
        if (npc->GetDistanceToVob2(*vobs[i]) > rangeSquared)
            vobs.RemoveOrderIndex(i);
    }
}

void oCNpc::SetNpcsToState(int function, float range)
{
    zCArray<zCVob*> vobs;
    NpcCreateVobList(this, vobs, range);
    for (int i = 0; i < vobs.numInArray; ++i)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(vobs[i]);
        if (npc && npc != this && npc->attribute[0] > 0)
            npc->state.StartAIState(function, 0);
    }
}

void oCNpc::StopCutscenes()
{
    zCCutscene* cutscene = GetEM(0)->GetCutscene();
    if (cutscene)
        cutscene->Stop();
}

extern float percRange[33];

void oCNpc::CreatePassivePerception(int perception, zCVob* vob, zCVob* other)
{
    zCArray<zCVob*> vobs;
    NpcCreateVobList(this, vobs, percRange[perception]);
    int moving = perception != 9 && (isInMovementMode & 3);
    if (moving)
    {
        SetCollDetStat(0);
        SetCollDetDyn(0);
        EndMovement(1);
        SetCollDetStat(1);
        SetCollDetDyn(1);
    }
    for (int i = 0; i < vobs.numInArray; ++i)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(vobs[i]);
        if (!npc || npc == this || npc->attribute[0] <= 0 || npc->IsAPlayer())
            continue;
        for (int j = 0; j < npc->percActive; ++j)
        {
            int function = npc->percList[j].percFunc;
            if (npc->percList[j].percID != perception || function < 0)
                continue;
            zCCutscene* cutscene = npc->GetEM(0)->GetCutscene();
            zCCSCutsceneContext* context = dynamic_cast<zCCSCutsceneContext*>(cutscene);
            if (context && !context->IsPlaying()
                && (!cutscene->properties || !cutscene->properties->globalCutscene))
                cutscene->Stop();
            npc->InsertInVobList(this);
            parser.SetInstance("VICTIM", vob);
            parser.SetInstance("OTHER", other);
            npc->state.StartAIState(function, 0, 0, 0.0f, 0);
            break;
        }
    }
    if (moving)
        BeginMovement();
}

void oCNpc::CreateSoundPerception(int perception, zCVob* source, const zVEC3& position,
    zCVob* other, int)
{
    if (!ogame || !ogame->GetWorld())
        return;
    float range = percRange[perception];
    zVEC3 extent(range, range, range);
    zTBBox3D box;
    box.mins = position - extent;
    box.maxs = position + extent;
    zCArray<zCVob*> vobs;
    ogame->GetWorld()->bspTree.bspRoot->CollectVobsInBBox3D(vobs, box);

    oCNpc* sourceNpc = dynamic_cast<oCNpc*>(source);
    int moving = sourceNpc && (sourceNpc->isInMovementMode & 3);
    if (moving)
        sourceNpc->EndMovement(1);
    for (int i = 0; i < vobs.numInArray; ++i)
    {
        oCNpc* npc = dynamic_cast<oCNpc*>(vobs[i]);
        if (!npc || npc == source || npc->attribute[0] <= 0 || npc->IsAPlayer())
            continue;
        for (int j = 0; j < npc->percActive; ++j)
        {
            int function = npc->percList[j].percFunc;
            if (npc->percList[j].percID != perception || function < 0)
                continue;
            parser.SetInstance("VICTIM", dynamic_cast<oCNpc*>(other));
            parser.SetInstance("OTHER", sourceNpc);
            parser.SetInstance("ITEM", dynamic_cast<oCItem*>(source));
            npc->InsertInVobList(source);
            npc->soundType = 0;
            npc->soundVob = source;
            npc->soundPosition = source ? source->GetPositionWorld() : position;
            npc->state.StartAIState(function, 0, 0, 0.0f, 0);
            break;
        }
    }
    if (moving)
        sourceNpc->BeginMovement();
}

int oCNpc::AssessMurder_S(oCNpc* other)
{
    CreatePassivePerception(6, other, this);
    return 1;
}

int oCNpc::AssessThreat_S(oCNpc* other)
{
    static oCNpc* threatNpc;
    static float threatTimer;
    if (threatNpc == other)
        threatTimer += ztimer.frameTimeFloat;
    else
    {
        threatTimer = 1000.0f;
        threatNpc = other;
    }
    if (threatTimer >= 1000.0f)
    {
        threatTimer -= 1000.0f;
        NpcAssessPerc(this, 10, other);
    }
    return 1;
}

int oCNpc::AssessDefeat_S(oCNpc* other)
{
    CreatePassivePerception(7, other, this);
    return 1;
}

int oCNpc::AssessDamage_S(oCNpc* attacker, int damage)
{
    NpcAssessPerc(this, 8, attacker);
    CreatePassivePerception(9, attacker, this);
    return 1;
}

int oCNpc::AssessOthersDamage_S(oCNpc* victim, oCNpc* attacker, int damage)
{
    CreatePassivePerception(9, victim, attacker);
    return 1;
}

int oCNpc::AssessRemoveWeapon_S(oCNpc* other)
{
    CreatePassivePerception(11, this, 0);
    return 1;
}

int oCNpc::ObserveIntruder_S()
{
    CreatePassivePerception(12, this, 0);
    return 1;
}

int oCNpc::AssessFightSound_S(zCVob* source, const zVEC3& position, zCVob* other)
{
    CreateSoundPerception(13, source, position, other, 1);
    return 1;
}

static int g_bDontKillOUSound;

int oCNpc::AssessQuietSound_S(zCVob* source, const zVEC3& position)
{
    g_bDontKillOUSound = 1;
    CreateSoundPerception(14, source, position, 0, 0);
    g_bDontKillOUSound = 0;
    return 1;
}

int oCNpc::AssessWarn_S(oCNpc* other)
{
    CreatePassivePerception(15, other, 0);
    return 1;
}

int oCNpc::CatchThief_S(oCNpc* other)
{
    return NpcAssessPerc(this, 16, other);
}

int oCNpc::AssessTheft_S(oCNpc* other)
{
    CreatePassivePerception(17, other, this);
    return 1;
}

int oCNpc::AssessCall_S(oCNpc* other)
{
    return NpcAssessPerc(this, 18, other);
}

int oCNpc::AssessTalk_S(oCNpc* other)
{
    return NpcAssessPerc(this, 19, other);
}

int oCNpc::AssessGivenItem_S(oCNpc* givenBy, oCItem* item)
{
    return NpcAssessPerc(this, 20, item);
}

int oCNpc::playerPercAssessMagic = -1;
int oCNpc::playerPercAssessStopMagic = -1;

int oCNpc::AssessMagic_S(oCNpc* other, int spellID)
{
    if (IsAPlayer())
        return playerPercAssessMagic >= 0;
    return NpcAssessPerc(this, 27, other);
}

int oCNpc::AssessStopMagic_S(oCNpc* other, int spellID)
{
    if (IsAPlayer())
        return playerPercAssessStopMagic >= 0;
    return NpcAssessPerc(this, 28, other);
}

int oCNpc::AssessCaster_S(oCSpell*)
{
    CreatePassivePerception(29, this, 0);
    return 1;
}

int oCNpc::AssessFakeGuild_S()
{
    if (!IsAPlayer())
        return 0;
    CreatePassivePerception(21, this, 0);
    return 1;
}

int oCNpc::MoveMob_S(oCMOB* mob)
{
    return NpcAssessPerc(this, 22, mob);
}

int oCNpc::AssessUseMob_S(oCMobInter* mob)
{
    if (!mob || !IsAPlayer())
        return 0;
    rbt.obstVob = mob;
    CreatePassivePerception(32, this, 0);
    return 1;
}

int oCNpc::MoveNpc_S(oCNpc* other)
{
    return NpcAssessPerc(this, 23, other);
}

int oCNpc::AssessEnterRoom_S()
{
    CreatePassivePerception(31, this, 0);
    return 1;
}

float oCNpc::GetDistToSound()
{
    float dx = fabs(soundPosition.n[0] - trafoObjToWorld.m[0][3]);
    float dy = fabs(soundPosition.n[1] - trafoObjToWorld.m[1][3]);
    float dz = fabs(soundPosition.n[2] - trafoObjToWorld.m[2][3]);
    float largest = dx;
    if (dy > largest)
        largest = dy;
    if (dz > largest)
        largest = dz;
    return largest * 0.9375f + (dx + dy + dz - largest) * 0.375f;
}

zCVob* oCNpc::GetSoundSource()
{
    return soundVob;
}

void oCNpc::SetSoundSource(int type, zCVob* vob, zVEC3& position)
{
    soundType = type;
    soundVob = vob;
    if (vob)
    {
        for (int i = 0; i < 3; ++i)
            soundPosition.n[i] = vob->trafoObjToWorld.m[i][3];
    }
    else
        soundPosition = position;
}

void oCNpc::ClearPerceptionLists()
{
    ClearFocusVob();
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        if (vobList[i])
            vobList[i]->Release();
    }
    vobList.DeleteList();
}

int oCNpc::IsMoreImportant(zCVob* first, zCVob* second)
{
    if (!first)
        return 0;
    if (!second)
        return 1;
    return GetDistanceToVob2(*second) >= GetDistanceToVob2(*first);
}

void oCNpc::CreateVobList(zCArray<zCVob*>& vobs, float range)
{
    NpcCreateVobList(this, vobs, range);
}

void oCNpc::ClearVobList()
{
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        if (vobList[i])
            vobList[i]->Release();
    }
    vobList.DeleteList();
}

void oCNpc::InsertInVobList(zCVob* vob)
{
    if (!vob)
        return;
    for (int i = 0; i < vobList.numInArray; ++i)
    {
        if (vobList[i] == vob)
            return;
    }
    vobList.Insert(vob);
    vob->AddRef();
}

void oCNpc::RemoveFromVobList(zCVob* vob)
{
    vobList.RemoveOrder(vob);
    vob->Release();
}

void oCNpc::CreateVobList(float range)
{
    ClearVobList();
    CreateVobList(vobList, range);
    RemoveFromVobList(this);
}

void oCNpc::SetPerceptionTime(float delta)
{
    percActiveDelta = delta;
    while (percActiveTime > percActiveDelta)
        percActiveTime -= percActiveDelta;
}

void oCNpc::PerceiveAll()
{
    if (IsAPlayer())
        return;
    CreateVobList(senses_range);
    for (int i = vobList.numInArray - 1; i >= 0; --i)
    {
        zCVob* vob = vobList[i];
        oCItem* item = vob->CastTo<oCItem>();
        oCNpc* other = vob->CastTo<oCNpc>();
        int remove = item && !PercFilterItem(item);
        if (other && homeWorld && homeWorld->csPlayer &&
            homeWorld->csPlayer->GetPlayingGlobalCutscene())
            remove = 1;
        if (remove)
            RemoveFromVobList(vob);
    }
}

void oCNpc::PerceptionCheck()
{
    percActiveTime += ztimer.frameTimeFloat;
    if (percActiveTime < percActiveDelta)
        return;
    percActiveTime -= percActiveDelta;
    ClearVobList();
    if (GetEM(0)->GetCutsceneMode())
        return;

    int npcPerception = HasPerception(1) || HasPerception(2) ||
        HasPerception(3) || HasPerception(4);
    int itemPerception = HasPerception(5);
    if (!npcPerception && !itemPerception)
        return;

    CreateVobList(senses_range);
    oCNpc* playerNpc = NULL;
    oCNpc* enemyNpc = NULL;
    oCNpc* fighterNpc = NULL;
    oCNpc* bodyNpc = NULL;
    oCItem* foundItem = NULL;
    for (int i = vobList.numInArray - 1; i >= 0; --i)
    {
        zCVob* vob = vobList[i];
        int keep = 0;
        if (npcPerception)
        {
            oCNpc* other = vob->CastTo<oCNpc>();
            if (other)
            {
                if (other->attribute[0] > 0 && !other->state.IsInState(-4))
                {
                    if (PercFilterNpc(other) && CanSense(other))
                    {
                        bodyNpc = other;
                        keep = 1;
                    }
                }
                else
                {
                    if (other->IsAPlayer() && CanSense(other))
                    {
                        playerNpc = other;
                        keep = 1;
                    }
                    int otherAttitude = other->IsAPlayer() && tmp_attitude != attitude ?
                        tmp_attitude : GetPermAttitude(other);
                    if (otherAttitude)
                    {
                        if (other->guild < 0 || other->guild >= 8)
                            other->guild = 0;
                        if (other->guild && PercFilterNpc(other) && CanSense(other))
                        {
                            fighterNpc = other;
                            keep = 1;
                        }
                    }
                    else if (PercFilterNpc(other) && CanSense(other))
                    {
                        enemyNpc = other;
                        keep = 1;
                    }
                }
            }
        }
        if (!keep && itemPerception)
        {
            oCItem* item = vob->CastTo<oCItem>();
            if (item && PercFilterItem(item) && CanSense(item))
            {
                foundItem = item;
                keep = 1;
            }
        }
        if (!keep)
            RemoveFromVobList(vob);
    }

    for (int i = 0; i < percActive; ++i)
    {
        switch (percList[i].percID)
        {
        case 1:
            if (playerNpc)
                AssessPlayer_S(playerNpc);
            break;
        case 2:
            if (enemyNpc)
                AssessEnemy_S(enemyNpc);
            break;
        case 3:
            if (fighterNpc)
                AssessFighter_S(fighterNpc);
            break;
        case 4:
            if (bodyNpc)
                AssessBody_S(bodyNpc);
            break;
        case 5:
            if (foundItem)
                AssessItem_S(foundItem);
            break;
        }
    }
}

int oCNpc::PercFilterNpc(oCNpc* other)
{
    return 1;
}

int oCNpc::PercFilterItem(oCItem* item)
{
    return !item->HasFlag(0x800000);
}

void oCNpc::ClearPerception()
{
    for (int i = 0; i < 33; ++i)
    {
        percList[i].percID = 0;
        percList[i].percFunc = -1;
    }
    percActive = 0;
}

void oCNpc::EnablePerception(int perception, int function)
{
    for (int i = 0; i < percActive; ++i)
    {
        if (percList[i].percID == perception)
        {
            percList[i].percFunc = function;
            return;
        }
    }
    if (percActive < 33)
    {
        percList[percActive].percID = perception;
        percList[percActive].percFunc = function;
        ++percActive;
    }
}

void oCNpc::DisablePerception(int perception)
{
    for (int i = 0; i < percActive; ++i)
    {
        if (percList[i].percID == perception)
        {
            percList[i].percID = 0;
            percList[i].percFunc = -1;
            if (i == percActive - 1)
                --percActive;
            return;
        }
    }
}

int oCNpc::HasPerception(int perception)
{
    return GetPerceptionFunc(perception) != -1;
}

int oCNpc::GetPerceptionFunc(int perception)
{
    for (int i = 0; i < percActive; ++i)
    {
        if (percList[i].percID == perception)
            return percList[i].percFunc;
    }
    return -1;
}

float percRange[33];
float NPC_ANGRY_TIME;

void oCNpc::SetPerceptionRange(int perception, float range)
{
    if (perception < 33)
        percRange[perception] = range;
}

int oCNpc::IsInPerceptionRange(int perception, float distance)
{
    return perception < 33 && distance <= percRange[perception];
}

int oCNpc::IsInPerceptionRange(int perception, zCVob* first, zCVob* second)
{
    return first && second && IsInPerceptionRange(perception, first->GetDistanceToVob(*second));
}

void oCNpc::InitStatics()
{
    zCPar_Symbol* symbol = parser.GetSymbol("NPC_ANGRY_TIME");
    if (symbol)
    {
        int angryTime = 0;
        symbol->GetValue(angryTime, 0);
        NPC_ANGRY_TIME = angryTime * 1000.0f;
    }
    for (int i = 0; i < 33; ++i)
        percRange[i] = 500.0f;

    int initPerceptions = parser.GetIndex("INITPERCEPTIONS");
    if (initPerceptions >= 0)
        parser.CallFunc(initPerceptions);
    InitFightAI();
    InitBodyStates();

    symbol = parser.GetSymbol("PLAYER_PERC_ASSESSMAGIC");
    if (symbol)
        symbol->GetValue(playerPercAssessMagic, 0);
    symbol = parser.GetSymbol("PLAYER_PERC_ASSESSSTOPMAGIC");
    if (symbol)
        symbol->GetValue(playerPercAssessStopMagic, 0);
    // EXE: 0x00AB2690
    static int referencesCreated;
    if (!referencesCreated)
    {
        parser.CreateVarReferenceList("C_NPC", npcPerceptionList);
        referencesCreated = 1;
    }
}

void oCNpc::ReleaseStatics()
{
    ExitFightAI();
}

static int npcBodyStates[31];
int* oCNpc::bodyStateList = npcBodyStates;
static zSTRING BS_NAMES[31] = {
    "BS_STAND", "BS_WALK", "BS_SNEAK", "BS_RUN", "BS_SPRINT", "BS_SWIM",
    "BS_CRAWL", "BS_DIVE", "BS_JUMP", "BS_CLIMB", "BS_FALL", "BS_SIT",
    "BS_LIE", "BS_INVENTORY", "BS_ITEMINTERACT", "BS_MOBINTERACT",
    "BS_MOBINTERACT_INTERRUPT", "BS_TAKEITEM", "BS_DROPITEM", "BS_THROWITEM",
    "BS_PICKPOCKET", "BS_STUMBLE", "BS_UNCONSCIOUS", "BS_DEAD", "BS_AIMNEAR",
    "BS_AIMFAR", "BS_HIT", "BS_PARADE", "BS_CASTING", "BS_PETRIFIED", "BS_CONTROLLING"
};
static zSTRING BS_MOD_NAMES[6] = {
    "BS_MOD_HIDDEN", "BS_MOD_DRUNK", "BS_MOD_NUTS",
    "BS_MOD_BURNING", "BS_MOD_CONTROLLED", "BS_MOD_TRANSFORMED"
};
const zSTRING NPC_NODE_RIGHTHAND("ZS_RIGHTHAND");
const zSTRING NPC_NODE_LEFTHAND("ZS_LEFTHAND");
const zSTRING NPC_NODE_SWORD("ZS_SWORD");
const zSTRING NPC_NODE_LONGSWORD("ZS_LONGSWORD");
const zSTRING NPC_NODE_BOW("ZS_BOW");
const zSTRING NPC_NODE_CROSSBOW("ZS_CROSSBOW");
const zSTRING NPC_NODE_SHIELD("ZS_SHIELD");
const zSTRING NPC_NODE_HELMET("ZS_HELMET");
const zSTRING NPC_NODE_JAWS("ZS_JAWS");
const zSTRING NPC_NODE_TORSO("ZS_TORSO");
static zSTRING npcStatesPrg[7] = {
    "ZS_NONE", "ZS_NONE", "ZS_ANSWER", "ZS_DEAD",
    "ZS_UNCONSCIOUS", "ZS_FADEAWAY", "ZS_FOLLOW"
};

void oCNpc::InitBodyStates()
{
    for (int i = 0; i < 31; ++i)
    {
        bodyStateList[i] = 0;
        zCPar_Symbol* symbol = parser.GetSymbol(BS_NAMES[i]);
        if (symbol)
            symbol->GetValue(bodyStateList[i], 0);
    }
}

void oCNpc::SetBodyState(int state)
{
    if (state < 0 || state >= 31)
        return;
    bodyState = (bodyState & 0xFFF80000) | (bodyState & 0x3F80) |
        state | (bodyStateList[state] & 0x18000);
}

int oCNpc::GetBodyState()
{
    return bodyState & 0x7F;
}

int oCNpc::GetFullBodyState()
{
    return bodyState & 0xFFFFC07F;
}

zSTRING oCNpc::GetBodyStateName()
{
    int state = GetBodyState();
    if (state >= 0 && state < 31)
        return zSTRING(BS_NAMES[state]);
    return zSTRING("unknown");
}

int oCNpc::HasBodyStateModifier(int modifier)
{
    return (bodyState & modifier) > 0;
}

void oCNpc::SetBodyStateModifier(int modifier)
{
    bodyState = (bodyState & 0xFFF80000) | ((bodyState | modifier) & 0x7FFFF);
}

void oCNpc::ClrBodyStateModifier(int modifier)
{
    bodyState = (bodyState & 0xFFF80000) | ((bodyState & ~modifier) & 0x7FFFF);
}

zSTRING oCNpc::GetBodyStateModifierNames()
{
    zSTRING result;
    if (bodyState & 0x400)
        result += "BS_MOD_BURNING\n";
    if (bodyState & 0x800)
        result += "BS_MOD_CONTROLLED\n";
    if (bodyState & 0x100)
        result += "BS_MOD_DRUNK\n";
    if (bodyState & 0x80)
        result += "BS_MOD_HIDDEN\n";
    if (bodyState & 0x200)
        result += "BS_MOD_NUTS\n";
    if (bodyState & 0x1000)
        result += "BS_MOD_TRANSFORMED\n";
    return result;
}

int oCNpc::ModifyBodyState(int add, int remove)
{
    SetBodyStateModifier(add);
    ClrBodyStateModifier(remove);
    return GetBodyState();
}

int oCNpc::IsBodyStateInterruptable()
{
    return !(bodyState & 0x3F80) && (bodyState & 0x8000);
}

int oCNpc::HasBodyStateFreeHands()
{
    return !(bodyState & 0x3F80) && (bodyState & 0x10000);
}

int oCNpc::IsInGlobalCutscene()
{
    zCCutscene* cutscene = GetEM(0)->GetCutscene();
    return cutscene && cutscene->properties && cutscene->properties->globalCutscene;
}

void oCNpc::MakeSpellBook()
{
    if (!mag_book)
        mag_book = new oCMag_Book;
    mag_book->SetOwner(this);
    spells = 1;
}

void oCNpc::DestroySpellBook()
{
    delete mag_book;
    mag_book = NULL;
}

static const char* PERC_NAMES[33] = {
    "PERC_NONE", "PERC_ASSESSPLAYER", "PERC_ASSESSENEMY", "PERC_ASSESSFIGHTER",
    "PERC_ASSESSBODY", "PERC_ASSESSITEM", "PERC_ASSESSMURDER", "PERC_ASSESSDEFEAT",
    "PERC_ASSESSDAMAGE", "PERC_ASSESSOTHERSDAMAGE", "PERC_ASSESSTHREAT",
    "PERC_ASSESSREMOVEWEAPON", "PERC_OBSERVEINTRUDER", "PERC_ASSESSFIGHTSOUND",
    "PERC_ASSESSQUIETSOUND", "PERC_ASSESSWARN", "PERC_CATCHTHIEF", "PERC_ASSESSTHEFT",
    "PERC_ASSESSCALL", "PERC_ASSESSTALK", "PERC_ASSESSGIVENITEM", "PERC_ASSESSFAKEGUILD",
    "PERC_MOVEMOB", "PERC_MOVENPC", "PERC_DRAWWEAPON", "PERC_OBSERVESUSPECT",
    "PERC_NPCCOMMAND", "PERC_ASSESSMAGIC", "PERC_ASSESSSTOPMAGIC", "PERC_ASSESSCASTER",
    "PERC_ASSESSSURPRISE", "PERC_ASSESSENTERROOM", "PERC_ASSESSUSEMOB"
};

void oCNpc::ShowDebugInfo(zCCamera* camera)
{
    oCVob::ShowDebugInfo(camera);
    if (!zDYNAMIC_CAST<oCAIHuman>(callback_ai))
        return;
    screen->Print(0, 5000, zSTRING("Perc: ") + GetInstanceName());
    int x = 0;
    int y = 5000 + screen->FontY();
    for (int perception = 0; perception < 33; ++perception)
    {
        if (!HasPerception(perception))
            continue;
        if (x >= 5462)
        {
            y += screen->FontY();
            x = 0;
        }
        screen->Print(x, y, PERC_NAMES[perception]);
        x += 2730;
    }
}

float oCNpc::GetDamageMultiplier()
{
    return damageMul;
}

void oCNpc::SetDamageMultiplier(float multiplier)
{
    damageMul = multiplier;
}

void oCNpc::SetNpcAIDisabled(int disabled)
{
    ai_disabled = disabled;
    ai_baseEnabled = 1;
    ai_scriptStateSkipTimer = 0.0f;
    ai_messagesSkipTimer = 0.0f;
    ai_messagesSkip = 0.0f;
    ai_scriptStateSkip = 0.0f;
    if (disabled)
        ai_scriptStateSkip = -1.0f;
}

void oCNpc::ProcessAITimer()
{
    if (ai_messagesSkip > 0.0f)
    {
        if (ai_messagesSkipTimer >= ai_messagesSkip)
            ai_messagesSkipTimer -= ai_messagesSkip;
        ai_messagesSkipTimer += ztimer.frameTimeFloat;
    }
    if (ai_scriptStateSkip > 0.0f)
    {
        if (ai_scriptStateSkipTimer >= ai_scriptStateSkip)
            ai_scriptStateSkipTimer -= ai_scriptStateSkip;
        ai_scriptStateSkipTimer += ztimer.frameTimeFloat;
    }
}

int oCNpc::IsScriptStateAIEnabled()
{
    if (ai_scriptStateSkip <= 0.0f)
        return 1;
    return ai_scriptStateSkipTimer >= ai_scriptStateSkip;
}

int oCNpc::IsMessageAIEnabled()
{
    if (ai_messagesSkip <= 0.0f)
        return 1;
    return ai_messagesSkipTimer >= ai_messagesSkip;
}

void oCNpc::SetShowNews(int enabled)
{
    showaidebug = (showaidebug & ~2) | ((enabled << 1) & 2);
}

int oCNpc::oCNpcTimedOverlay::Process()
{
    timer -= ztimer.frameTimeFloat;
    return timer > 0.0f;
}

void oCNpc::UseStandAI()
{
    SetAI((zCAIBase*)new oCAIHuman_Stand(this));
    callback_ai->Release();
}

void oCNpc::SetItemEffectControledByModel(oCItem* item, zCModelNodeInst* node,
    int attached, int remove, int removeEffect)
{
    if (!item || item->effectName.IsEmpty())
        return;
    if (!remove)
        item->InsertEffect();
    if (!item->effectVob)
        return;

    zCWorld* world = homeWorld;
    if (!world)
        world = ogame->GetWorld();
    oCVisualFX* effectVob = item->effectVob;
    zCModel* model = zDYNAMIC_CAST<zCModel>(visual);
    if (!model)
    {
        AvoidShrink(1000);
        model = zDYNAMIC_CAST<zCModel>(visual);
    }

    if (remove)
    {
        model->RemoveChildVobFromNode(effectVob);
        effectVob->RemoveVobSubtreeFromWorld();
        if (removeEffect)
            item->RemoveEffect();
        return;
    }

    model->RemoveChildVobFromNode(effectVob);
    if (attached)
    {
        effectVob->m_bAllowMovement = 0;
        if (node)
        {
            effectVob->SetPFXShapeVisual(item->visual, 0);
            model->AttachChildVobToNode(effectVob, node);
        }
        else
        {
            effectVob->SetPFXShapeVisual(item->visual, 0);
            effectVob->SetTrafoObjToWorld(trafoObjToWorld);
        }
        if (!effectVob->homeWorld)
            world->AddVob(effectVob);
        if (!globalVobTreeNode)
            effectVob->RemoveVobSubtreeFromWorld();
        world->MoveVobSubtreeTo(effectVob, this);
    }
    else
    {
        effectVob->m_bAllowMovement = 1;
        world->MoveVobSubtreeToWorldSpace(effectVob);
        effectVob->SetPFXShapeVisual(item->visual, 0);
    }
    effectVob->SetSleeping(0);
}

zCObject* oCNpc::_CreateNewInstance()
{
    oCNpc* npc = new oCNpc;
    zCClassDef::ObjectCreated(npc, classDef);
    return npc;
}

void oCNpc::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

int oCNpc::game_mode;
oCNpc* stealnpc;
float stealcheck_timer;

int oCNpc::IsVictimAwareOfTheft()
{
    if (!stealnpc)
        return 0;
    int bodyState = stealnpc->GetBodyState();
    int detected = stealnpc->HasVobDetected(this);
    int helpless = stealnpc->IsDead() || stealnpc->IsUnconscious();
    int stealMode = game_mode == 2 && (!bodyState || bodyState == 14);
    if (!detected && (stealMode || helpless))
        return 0;
    return 1;
}

void oCNpc::CheckSpecialSituations()
{
    if (!stealnpc || (game_mode != 2 && game_mode != 1))
        return;
    int focusChanged = GetFocusNpc() != stealnpc;
    int aware = IsVictimAwareOfTheft();
    if (!focusChanged && !aware)
    {
        stealcheck_timer -= ztimer.frameTimeFloat;
        if (stealcheck_timer <= 0.0f)
        {
            stealnpc->AssessTheft_S(this);
            stealcheck_timer += 3000.0f;
        }
        return;
    }
    StopTheft(this, aware);
}

void oCNpc::StopTheft(oCNpc* other, int catchThief)
{
    if (!other)
        return;
    if (catchThief)
    {
        EndMovement(1);
        CatchThief_S(other);
        BeginMovement();
    }
    other->inventory2.Close();
    other->CloseTradeContainer();
    if (game_mode == 2 && stealcontainer)
    {
        stealcontainer->Close();
        game_mode = 0;
    }
    stealnpc = NULL;
}

void oCNpc::OpenInventory(int mode)
{
    if (ztimer.frameTimeFloat == 0.0f || !IsBodyStateInterruptable())
        return;
    inventory2.SetMode(mode);
    if (inventory2.IsOpen() || IsDead())
        return;
    if (anictrl && anictrl->IsInWeaponChoose())
        return;
    if (GetWeaponMode())
        GetEM(0)->OnMessage(new oCMsgWeapon(3, 0, 0), this);
    if (anictrl)
        anictrl->StopTurnAnis();
    inventory2.DisableManipulateItems(0);
    inventory2.DisableTransferMoreThanOneItem(0);
    inventory2.SetName(GetName(0));
    inventory2.Open(0, 0, IsAPlayer() ? 0x1000 : 0);
}

void oCNpc::CloseInventory()
{
    inventory2.Close();
    CloseTradeContainer();
}

int oCNpc::OpenSteal()
{
    stealnpc = GetFocusNpc();
    if (!stealnpc || stealnpc->IsDead() || stealnpc->IsAPlayer() ||
        stealnpc->GetTrueGuild() == 58)
        return 0;
    parser.SetInstance("SELF", this);
    parser.SetInstance("OTHER", stealnpc);
    int canSteal = parser.CallFunc("G_CANSTEAL");
    if (!canSteal || IsVictimAwareOfTheft() ||
        oCNpcFocus::focus->GetRange2() < GetDistanceToVob2(*stealnpc))
    {
        oCMsgConversation* message = new oCMsgConversation;
        message->name = "T_DONTKNOW";
        GetEM(0)->OnMessage(message, this);
        return 0;
    }

    delete stealcontainer;
    stealcontainer = new oCStealContainer;
    stealcontainer->SetOwner(stealnpc);
    if (stealcontainer->IsEmpty())
    {
        oCMsgConversation* message = new oCMsgConversation;
        message->name = "T_DONTKNOW";
        GetEM(0)->OnMessage(message, this);
        return 0;
    }
    stealcontainer->SetName(stealnpc->GetName(0));
    stealcontainer->Open(0, 0, 3);
    OpenInventory(0);
    stealcontainer->Activate();
    game_mode = 2;
    return 1;
}

void oCNpc::CloseSteal()
{
    if (stealcontainer)
    {
        stealcontainer->Close();
        game_mode = 0;
    }
}

static oCNpcContainer* deadNpcContainer;

void oCNpc::OpenDeadNpc()
{
    stealnpc = GetFocusNpc();
    if (stealnpc && (stealnpc->IsDead() || stealnpc->IsUnconscious()))
    {
        delete deadNpcContainer;
        deadNpcContainer = new oCNpcContainer;
        deadNpcContainer->SetOwner(stealnpc);
        if (!deadNpcContainer->IsEmpty())
        {
            deadNpcContainer->SetName(stealnpc->GetName(0));
            deadNpcContainer->Open(0, 0, 2);
            OpenInventory(0);
            deadNpcContainer->Activate();
            game_mode = 0;
            return;
        }
    }
    GetEM(0)->OnMessage(new oCMsgManipulate(oCMsgManipulate::EV_CALLSCRIPT,
        PLAYER_PLUNDER_IS_EMPTY, -1), this);
}

void oCNpc::CloseDeadNpc()
{
    if (!deadNpcContainer)
        return;
    inventory2.Close();
    CloseTradeContainer();
    deadNpcContainer->Close();
    delete deadNpcContainer;
    deadNpcContainer = NULL;
    game_mode = 0;
}
