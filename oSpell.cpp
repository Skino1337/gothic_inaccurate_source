#include "include/oCollisionObjectMiscChar.h"
#include "include/oGame.h"
#include "include/oNpcMessages.h"

static zSTRING spellEmptyString;
static int spellState = -1;
static zSTRING spellPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING spellPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING spellPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING spellPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING spellPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING spellPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING spellPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING spellPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING spellPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING spellPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static int spellRuntimeState = -1;
static zSTRING spellRuntimePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING spellRuntimePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING spellRuntimePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING spellRuntimePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING spellRuntimePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING spellRuntimePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING spellRuntimePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING spellRuntimePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING spellRuntimePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING spellRuntimePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";

oCSpell::oCSpell()
{
    InitValues(0);
}

oCSpell::oCSpell(int id)
{
    InitValues(id);
}

oCSpell::~oCSpell()
{
    if (effect)
    {
        effect->Stop(1);
        effect->Release();
        effect = NULL;
    }
    if (controlWarnFX)
    {
        controlWarnFX->Stop(1);
        controlWarnFX->Release();
        controlWarnFX = NULL;
    }
    if (saveNpc)
    {
        saveNpc->Release();
        saveNpc = NULL;
    }
}

zCClassDef* oCSpell::_GetClassDef() const
{
    return classDef;
}

void oCSpell::InitValues(int id)
{
    spellLevel = 1;
    spellStatus = 1;
    canBeDeleted = 1;
    canChangeTargetDuringInvest = 1;
    canTurnDuringInvest = 1;
    spellID = id;
    effect = NULL;
    controlWarnFX = NULL;
    saveNpc = NULL;
    manaInvestTime = 2000.0f;
    damageType = 4;
    damagePerLevel = 0;
    spellEnabled = 0;
    spellInitDone = 0;
    spellEnergy = 2;
    timerEffect = 0;
    up = 0.0f;
    hoverY = 0.0f;
    hoverOld = 0.0f;
    hoverDir = 1.0f;
    isMultiEffect = 0;
    targetCollectAlgo = 4;
    targetCollectType = 4;
    targetCollectRange = 10000;
    targetCollectAzi = 180;
    targetCollectElev = 90;
    keyNo = 32565;
    InitByScript(GetSpellInstanceName(spellID));
}

zSTRING oCSpell::GetSpellInstanceName(int id) const
{
    zSTRING result = "default";
    zCPar_Symbol* symbol = parser.GetSymbol("spellFXInstanceNames");
    if (symbol)
        symbol->GetValue(result, id);
    return result;
}

oCVisualFX* oCSpell::CreateEffect()
{
    if (isMultiEffect)
        effect = new oCVisFX_MultiTarget;
    else
        effect = new oCVisualFX;

    effect->SetSpellTargetTypes(targetCollectType);
    effect->SetDamage(damagePerLevel);
    effect->SetDamageType(damageType);
    effect->SetSpellType(spellID);
    effect->SetSpellCat(spellType);
    effect->SetByScript("SPELLFX_" + GetSpellInstanceName(spellID));
    return effect;
}

void oCSpell::InitByScript(const zSTRING& instanceName)
{
    zerr->Report(zERR_TYPE_INFO, 0, "C: SPL: InitByScript", 8, 0, 0x191, 0, 0);

    int index = parser.GetIndex("SPELL_" + instanceName);
    if (index >= 0)
        parser.CreateInstance(index, this);
    else
        zerr->Report(zERR_TYPE_WARN, 0, "U: SPL: Spell-Instance " + instanceName + " not found.",
            0, 0, 0x195, 0, 0);

    effect = CreateEffect();
    zerr->Report(zERR_TYPE_INFO, 0, "C: SPL: SetEnabled", 8, 0, 0x1A4, 0, 0);
    SetEnabled(1);
    SetObjectName(instanceName);
    manaTimer = 0.0f;
    manaInvested = 0;
    spellLevel = 1;
}

void oCSpell::SetEnabled(int value)
{
    spellEnabled = value;
    if (effect)
        effect->showVisual = value;
}

int oCSpell::GetEnabled()
{
    return spellEnabled;
}

void oCSpell::Setup(zCVob* vob, zCVob* target, int flags)
{
    zerr->Report(zERR_TYPE_INFO, 0, "C: SPL: Setup", 8, 0, 0x1B7, 0, 0);

    if (!effect || (spellStatus == 2 && !flags))
        return;

    oCNpc* targetNpc = dynamic_cast<oCNpc*>(target);
    if (spellInitDone && spellTarget == targetNpc && targetNpc)
        return;
    if (spellTarget && !canChangeTargetDuringInvest && manaTimer > 0.0f)
        return;

    spellTarget = targetNpc;
    spellTargetNpc = targetNpc;
    spellCaster = vob;
    spellCasterNpc = dynamic_cast<oCNpc*>(vob);

    if ((spellID == 18 || spellID == 0) && spellCasterNpc &&
        (spellCasterNpc->IsSpellActive(18) || spellCasterNpc->IsSpellActive(0)))
    {
        spellStatus = 0;
        spellTarget = NULL;
        spellTargetNpc = NULL;
        return;
    }

    int initialized = 1;
    switch (targetCollectAlgo)
    {
    case 0:
        effect->Init(vob, NULL, NULL);
        break;
    case 1:
        effect->Init(vob, vob, 0);
        break;
    case 2:
        if (IsValidTarget(target))
            effect->Init(vob, target, 0);
        else
            initialized = 0;
        break;
    case 4:
    case 5:
        if (IsValidTarget(target))
            effect->Init(vob, target, 0);
        else if (targetCollectAlgo == 5)
            effect->Init(vob, vob, 0);
        else
            effect->Init(vob, NULL, NULL);
        break;
    case 3:
    case 6:
    case 7:
    {
        if (spellInitDone && !(ogame->GetWorld()->bspTree.worldRenderMode & 3))
            break;

        zCArray<zCVob*> targets;
        targets.InsertEnd(spellCaster);
        if (IsValidTarget(spellTarget))
            targets.InsertEnd(spellTarget);

        zCArray<zCVob*> foundVobs;
        zVEC3 center = spellCaster->GetPositionWorld();
        zVEC3 extent(targetCollectRange, targetCollectRange, targetCollectRange);
        zTBBox3D box;
        box.mins = center - extent;
        box.maxs = center + extent;
        if (spellCaster->homeWorld)
            spellCaster->homeWorld->bspTree.bspRoot->CollectVobsInBBox3D(foundVobs, box);

        for (int i = 0; i < foundVobs.numInArray; ++i)
        {
            oCNpc* npc = dynamic_cast<oCNpc*>(foundVobs[i]);
            if (npc && npc != spellTarget && npc != spellCaster && IsValidTarget(npc))
                targets.InsertEnd(npc);
        }
        if (targets.numInArray < 2 && targetCollectAlgo == 7)
            targets.InsertEnd(spellCaster);
        if (targets.numInArray < 2 && targetCollectAlgo != 6 && targetCollectAlgo != 7)
            initialized = 0;
        else if (isMultiEffect)
            effect->Init(targets);
        else
        {
            effect->Init(targets.numInArray > 1 ? targets[1] : NULL, NULL, 0);
            effect->SetCollisionCandidates(targets);
        }
        break;
    }
    default:
        break;
    }

    if (initialized)
    {
        spellStatus = 1;
        spellInitDone = 1;
        return;
    }

    spellStatus = 0;
    spellTarget = NULL;
    spellTargetNpc = NULL;
    if (spellInitDone)
        spellInitDone = 0;
    else if (isMultiEffect)
    {
        zCArray<zCVob*> emptyTargets;
        effect->Init(emptyTargets);
        spellInitDone = 0;
    }
    else
    {
        effect->Init(spellCasterNpc, NULL, NULL);
        spellInitDone = 0;
    }
}

int oCSpell::Invest()
{
    if (!effect || !spellCasterNpc)
        return 0;
    if (spellCasterNpc->GetAttribute(spellEnergy) <= 0)
    {
        if (manaInvested > 0)
        {
            SetReleaseStatus();
            return 1;
        }
        return 0;
    }

    manaTimer += manaInvested ? ztimer.frameTimeFloat : manaInvestTime;
    if (!manaInvested)
        spellCasterNpc->CreatePassivePerception(29, NULL, NULL);
    if (manaTimer < manaInvestTime)
        return 0;

    manaTimer -= manaInvestTime;
    if ((spellStatus != 1 && spellStatus != 8) || !spellCasterNpc)
        return 0;

    CallScriptInvestedMana();
    if (spellStatus & 0x10000)
    {
        manaInvested = spellStatus - 0x10000;
        spellCasterNpc->ChangeAttribute(spellEnergy, -manaInvested);
        spellStatus = 1;
    }
    else if (spellStatus == 1 || spellStatus == 2 || spellStatus == 4 || spellStatus == 8)
    {
        if (manaInvested && spellStatus == 1)
            spellCasterNpc->ChangeAttribute(spellEnergy, -1);
        ++manaInvested;
    }

    if (spellStatus == 4)
    {
        spellStatus = 1;
        ++spellLevel;
        if (effect)
            effect->InvestNext();
    }
    return 1;
}

int oCSpell::Cast()
{
    if (manaInvested <= 0)
    {
        spellStatus = 1;
        manaTimer = 0.0f;
        return 0;
    }
    if (effect)
    {
        effect->Cast(1);
        if (spellID != 18 && spellID != 0)
        {
            effect->Release();
            effect = NULL;
        }
    }
    if (spellStatus == 2)
        CastSpecificSpell();
    return 1;
}

int oCSpell::Stop()
{
    manaTimer = 0.0f;
    manaInvested = -1;
    spellLevel = 1;
    spellStatus = 1;
    if (effect)
    {
        effect->Stop(1);
        effect->Release();
        effect = NULL;
    }
    return keyNo;
}

int oCSpell::Kill()
{
    int result = keyNo;
    if (effect)
    {
        effect->Kill();
        effect->Release();
        effect = NULL;
    }
    Release();
    return result;
}

void oCSpell::Open()
{
    if (effect)
        effect->Open();
}

void oCSpell::Close()
{
    if (effect)
    {
        effect->Stop(1);
        effect->Release();
        effect = NULL;
    }
}

void oCSpell::Reset()
{
    if (effect && effect->GetLevel() < 2)
        effect->Reset();
}

void oCSpell::StopTargetEffects(zCVob*)
{
}

void oCSpell::CallScriptInvestedMana()
{
    parser.SetInstance("SELF", spellCasterNpc);
    parser.SetInstance("OTHER", spellTargetNpc);
    int function = parser.GetIndex("SPELL_PROCESSMANA");
    if (function > 0)
        spellStatus = (int)parser.CallFunc(function, manaInvested);
}

int oCSpell::IsTargetTypeValid(zCVob* target, int targetType)
{
    if (targetType == 1)
        return 1;
    for (int bit = 0; bit < 7; ++bit)
    {
        int flag = 1 << bit;
        if (!(targetType & flag))
            continue;
        if (flag == 2)
        {
            if (dynamic_cast<oCItem*>(target))
                return 1;
            continue;
        }
        oCNpc* npc = dynamic_cast<oCNpc*>(target);
        if (!npc || npc->IsDead())
            continue;
        int guild = npc->GetTrueGuild();
        if (flag == 4 || (flag == 8 && guild > 58) || (flag == 16 && guild < 16))
            return 1;
        if (flag == 32 && (guild == 31 || guild == 34 || guild == 20 || guild == 21 ||
            guild == 32 || guild == 37))
            return 1;
    }
    return 0;
}

int oCSpell::IsValidTarget(zCVob* target)
{
    if (targetCollectAlgo == 1)
        return target == spellCasterNpc;
    if (!target)
        return targetCollectAlgo == 0;
    if (!spellCasterNpc)
        return 0;

    float azimuth;
    float elevation;
    spellCasterNpc->GetAngles(target, azimuth, elevation);
    if (fabs(azimuth) > targetCollectAzi || fabs(elevation) > targetCollectElev)
        return 0;
    zVEC3 delta = spellCasterNpc->GetPositionWorld() - target->GetPositionWorld();
    if (delta.GetLength() > targetCollectRange)
        return 0;
    if (!spellCasterNpc->FreeLineOfSight(target))
        return 0;
    return IsTargetTypeValid(target, targetCollectType);
}

zCVob* oCSpell::GetVob()
{
    return effect;
}

int oCSpell::GetEnergyType()
{
    return spellEnergy;
}

int oCSpell::GetCategory() const
{
    return spellType;
}

zSTRING oCSpell::GetName()
{
    zSTRING result;
    zCPar_Symbol* symbol = parser.GetSymbol("TXT_SPELLS");
    if (symbol)
        symbol->GetValue(result, spellID);
    return result;
}

int oCSpell::GetLevel() const
{
    return spellLevel;
}

int oCSpell::IsInvestSpell()
{
    return 0;
}

void oCSpell::SetInvestedMana(int mana)
{
    manaInvested = mana;
    if (!mana)
        manaTimer = 0.0f;
}

int oCSpell::GetSpellStatus() const
{
    return spellStatus;
}

void oCSpell::SetReleaseStatus()
{
    parser.SetInstance("SELF", spellCasterNpc);
    parser.SetInstance("OTHER", spellTargetNpc);
    int function = parser.GetIndex("SPELL_PROCESSMANA_RELEASE");
    if (function > 0)
        spellStatus = (int)parser.CallFunc(function, manaInvested);
    if (spellStatus != 2 && spellStatus != 3)
        spellStatus = 3;
}

void oCSpell::SetSpellInfo(int info)
{
    spellInfo = info;
}

void oCSpell::DoLogicInvestEffect()
{
}

int oCSpell::CastSpecificSpell()
{
    if (!spellCasterNpc)
        return 1;

    zerr->Report(zERR_TYPE_INFO, 0, "C: SPL: CastSpecificSpell", 8, 0, 0x462, 0, 0);

    if (spellID == 18 || spellID == 0)
    {
        spellCasterNpc->InsertActiveSpell(this);
        canBeDeleted = 0;
        return 0;
    }
    if (spellID < 47 || spellID > 58 || spellInfo < 0)
        return 1;

    saveNpc = spellCasterNpc;
    zMAT4 savedTrafo = spellCasterNpc->trafoObjToWorld;
    zVEC3 savedPosition = spellCasterNpc->GetPositionWorld();
    spellCasterNpc = (oCNpc*)ogame->CreateVob((zTVobType)130, spellInfo);
    if (spellCasterNpc->GetInstanceName().IsEmpty())
        zerr->Report(zERR_TYPE_FAULT, 0,
            "oCSpell :: CastSpecificSpell(): (Transform) could not insert npc. given instance is invalid",
            0, 0, 0x46F, 0, 0);

    spellCasterNpc->IsSelfPlayer();
    saveNpc->AddRef();
    saveNpc->CloseSpellBook(1);
    saveNpc->SetWeaponMode(0);
    saveNpc->GetAnictrl()->SearchStandAni(1);

    spellCasterNpc->SetTrafoObjToWorld(savedTrafo);
    ogame->GetGameWorld()->AddVob(spellCasterNpc);
    spellCasterNpc->SetCollDetStat(0);
    spellCasterNpc->SetCollDetDyn(0);
    if (spellCasterNpc->SearchNpcPosition(savedPosition))
    {
        saveNpc->CopyTransformSpellInvariantValuesTo(spellCasterNpc);
        spellCasterNpc->SetCollDetStat(1);
        spellCasterNpc->SetCollDetDyn(1);
        spellCasterNpc->Disable();
        spellCasterNpc->GetAnictrl()->SearchStandAni(0);
        spellCasterNpc->Release();
        spellCasterNpc->InsertActiveSpell(this);
        spellCasterNpc->SetBodyStateModifier(0x1000);
        spellTargetNpc = spellCasterNpc;
        spellCasterNpc->AssessFakeGuild_S();
    }
    else
    {
        oCVisualFX* noPlace = oCVisualFX::CreateAndPlay("TRANSFORM_NOPLACEFX", savedPosition, NULL,
            1, 0.0f, 0, 0);
        if (noPlace)
            noPlace->Release();
        spellCasterNpc->SetCollDetStat(1);
        spellCasterNpc->SetCollDetDyn(1);
        spellCasterNpc->RemoveVobFromWorld();
        saveNpc->IsSelfPlayer();
        spellCasterNpc->Release();
        spellCasterNpc = NULL;
        saveNpc->Release();
        saveNpc->Disable();
        saveNpc->SetSleeping(0);
        timerEffect = 0;
    }
    canBeDeleted = 0;
    return 0;
}

void oCSpell::DoTimedEffect()
{
    if (timerEffect > 0)
    {
        timerEffect -= (int)ztimer.frameTimeFloat;
        if (timerEffect <= 0)
        {
            canBeDeleted = 1;
            EndTimedEffect();
        }
    }
    if ((spellID == 18 || spellID == 0) && effect->CanBeDeleted())
    {
        effect->Release();
        effect = NULL;
        canBeDeleted = 1;
    }
}

int oCSpell::CanBeDeleted()
{
    return canBeDeleted;
}

int oCSpell::DeleteCaster()
{
    return spellID >= 47 && spellID <= 58;
}

void oCSpell::EndTimedEffect()
{
    zerr->Report(zERR_TYPE_INFO, 0, "C: SPL: EndTimedEffect", 8, 0, 0x4D5, 0, 0);
    canBeDeleted = 1;
    if (spellID < 47 || spellID > 58 || !saveNpc || !spellTargetNpc)
        return;

    zMAT4 targetTrafo = spellTargetNpc->trafoObjToWorld;
    zVEC3 targetPosition = spellTargetNpc->GetPositionWorld();
    saveNpc->SetTrafoObjToWorld(targetTrafo);
    ogame->GetGameWorld()->AddVob(saveNpc);
    saveNpc->SetCollDetStat(0);
    saveNpc->SetCollDetDyn(0);
    if (!saveNpc->SearchNpcPosition(targetPosition))
    {
        oCVisualFX* noPlace = oCVisualFX::CreateAndPlay("TRANSFORM_NOPLACEFX", targetPosition, NULL,
            1, 0.0f, 0, 0);
        if (noPlace)
            noPlace->Release();
        saveNpc->SetCollDetStat(1);
        saveNpc->SetCollDetDyn(1);
        canBeDeleted = 0;
        timerEffect = 0;
        return;
    }

    saveNpc->Disable();
    saveNpc->ResetRotationsWorld();
    zVEC3 heading(targetTrafo.m[0][2], targetTrafo.m[1][2], targetTrafo.m[2][2]);
    saveNpc->SetHeadingAtWorld(heading);
    saveNpc->ResetXZRotationsWorld();
    saveNpc->SetCollDetStat(1);
    saveNpc->SetCollDetDyn(1);
    if (!(spellTargetNpc->isInMovementMode & 3))
        ogame->GetGameWorld()->RemoveVob(spellTargetNpc);
    spellCasterNpc->CopyTransformSpellInvariantValuesTo(saveNpc);
    saveNpc->GetAnictrl()->SearchStandAni(0);
    saveNpc->IsSelfPlayer();

    effect = CreateEffect();
    effect->Init(saveNpc, NULL, NULL);
    effect->SetSpellTargetTypes(targetCollectType);
    effect->Cast(1);
    effect->Release();
    effect = NULL;

    oCMsgConversation* message = new oCMsgConversation(14, "T_TRFSHOOT_2_STAND");
    message->AddRef();
    saveNpc->GetEM(0)->OnMessage(message, saveNpc);
    saveNpc->CreatePassivePerception(30, saveNpc, NULL);
}

void oCSpell::Archive(zCArchiver&)
{
}

void oCSpell::Unarchive(zCArchiver&)
{
}

// EXE: 0x00487550
zCObject* oCSpell::_CreateNewInstance()
{
    oCSpell* spell = new oCSpell;
    zCClassDef::ObjectCreated(spell, classDef);
    return spell;
}
