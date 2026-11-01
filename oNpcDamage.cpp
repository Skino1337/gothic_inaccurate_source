#include "include/oCollisionObjectMiscChar.h"
#include "include/oAniCtrl.h"
#include "include/zSound.h"
#include "include/zTools.h"

extern zTIMER ztimer;

int oCNpc::isEnabledDamageDebug;

int oCVisualFX::Release()
{
    return --refCtr;
}

oCNpc::oSDamageDescriptor::oSDamageDescriptor(const oSDamageDescriptor& other)
    : pFXHit(0), pVisualFX(0)
{
    *this = other;
}

oCNpc::oSDamageDescriptor::~oSDamageDescriptor()
{
    Release();
}

oCNpc::oSDamageDescriptor& oCNpc::oSDamageDescriptor::operator=(const oSDamageDescriptor& other)
{
    if (this == &other)
        return *this;
    Release();
    dwFieldsValid = other.dwFieldsValid;
    pVobAttacker = other.pVobAttacker;
    pNpcAttacker = other.pNpcAttacker;
    pVobHit = other.pVobHit;
    pFXHit = other.pFXHit;
    pItemWeapon = other.pItemWeapon;
    nSpellID = other.nSpellID;
    nSpellCat = other.nSpellCat;
    nSpellLevel = other.nSpellLevel;
    enuModeDamage = other.enuModeDamage;
    enuModeWeapon = other.enuModeWeapon;
    for (int i = 0; i < 8; ++i) {
        aryDamage[i] = other.aryDamage[i];
        aryDamageEffective[i] = other.aryDamageEffective[i];
    }
    fDamageTotal = other.fDamageTotal;
    fDamageMultiplier = other.fDamageMultiplier;
    vecLocationHit = other.vecLocationHit;
    vecDirectionFly = other.vecDirectionFly;
    strVisualFX = other.strVisualFX;
    fTimeDuration = other.fTimeDuration;
    fTimeInterval = other.fTimeInterval;
    fDamagePerInterval = other.fDamagePerInterval;
    bDamageDontKill = other.bDamageDontKill;
    bOnce = other.bOnce;
    bFinished = other.bFinished;
    bIsDead = other.bIsDead;
    bIsUnconscious = other.bIsUnconscious;
    lReserved = other.lReserved;
    fAzimuth = other.fAzimuth;
    fElevation = other.fElevation;
    fTimeCurrent = other.fTimeCurrent;
    fDamageReal = other.fDamageReal;
    fDamageEffective = other.fDamageEffective;
    pVobParticleFX = other.pVobParticleFX;
    pParticleFX = other.pParticleFX;
    pVisualFX = other.pVisualFX;
    if (pFXHit)
        ++pFXHit->refCtr;
    if (pVisualFX)
        ++pVisualFX->refCtr;
    return *this;
}

void oCNpc::oSDamageDescriptor::Release()
{
    if (pVisualFX) {
        if (--pVisualFX->refCtr <= 0)
            pVisualFX->Release();
        pVisualFX = 0;
    }
    if (pFXHit) {
        if (--pFXHit->refCtr <= 0)
            pFXHit->Release();
        pFXHit = 0;
    }
}

void oCNpc::oSDamageDescriptor::SetVisualFX(oCVisualFX* effect)
{
    if (pVisualFX == effect)
        return;
    if (pVisualFX) {
        if (--pVisualFX->refCtr <= 0)
            pVisualFX->Release();
        pVisualFX = 0;
    }
    pVisualFX = effect;
    if (effect)
        ++effect->refCtr;
}

void oCNpc::oSDamageDescriptor::SetFXHit(oCVisualFX* effect)
{
    if (pFXHit == effect)
        return;
    if (pFXHit) {
        if (--pFXHit->refCtr <= 0)
            pFXHit->Release();
        pFXHit = 0;
    }
    pFXHit = effect;
    if (effect)
        ++effect->refCtr;
}

void oCNpc::EnableDamageDebugOutput(int enabled)
{
    isEnabledDamageDebug = enabled;
}

void oCNpc::NotifyDamageToSpy(zSTRING& text)
{
    if (zerr->filter_level >= 6)
        zerr->Message(zSTRING("R: DAM: ") + text);
}

void oCNpc::InitDamage()
{
    bloodEnabled = 0;
    bloodDistance = 1000;
    bloodAmount = 500;
    bloodFlow = 0;
    bloodEmitter = "PFX_BLOOD";
    bloodTexture = "ZBLOODSPLAT2.TGA";
    hasLockedEnemy = 0;
    didShoot = 0;
    didHit = 0;
    wasAiming = 0;
    waypointFlee = 0;
    speedTurn = 150.0f * 0.001f;
    foundFleePoint = 0;
    reachedFleePoint = 0;
    isDefending = 0;
    didParade = 0;
    damagetype = 2;
    for (int i = 0; i < 8; ++i)
        damage[i] = 0;
}

int oCNpc::EV_DamageOnce(oCMsgDamage* message)
{
    DoShootArrow(message->descDamage.dwFieldsValid);
    return 1;
}

int oCNpc::EV_DamagePerFrame(oCMsgDamage* message)
{
    oSDamageDescriptor& descriptor = message->descDamage;
    if (isEnabledDamageDebug)
        NotifyDamageToSpy(zSTRING("EV_DamagePerFrame()"));
    if (!descriptor.bOnce) {
        descriptor.bOnce = 1;
        descriptor.fTimeCurrent = descriptor.fTimeDuration;
        descriptor.fDamageReal = descriptor.fDamageTotal;
        descriptor.dwFieldsValid |= 0x1000;
    }
    descriptor.fTimeCurrent -= ztimer.frameTimeFloat;
    descriptor.fTimeInterval -= ztimer.frameTimeFloat;
    if (descriptor.fTimeCurrent > 0.0f)
        return 0;
    OnDamage(descriptor);
    descriptor.fTimeCurrent = descriptor.fTimeDuration;
    return descriptor.bFinished;
}

void oCNpc::OnDamage(oSDamageDescriptor& descriptor)
{
    if (descriptor.bFinished)
        return;
    OnDamage_Hit(descriptor);
    OnDamage_Condition(descriptor);
    if (descriptor.bOnce)
        OnDamage_Script(descriptor);
}

void oCNpc::OnDamage_Hit(oSDamageDescriptor& descriptor)
{
    float total = 0.0f;
    for (int i = 0; i < 8; ++i) {
        descriptor.aryDamageEffective[i] = static_cast<unsigned long>(
            descriptor.aryDamage[i] * descriptor.fDamageMultiplier);
        total += descriptor.aryDamageEffective[i];
    }
    descriptor.fDamageEffective = total;
    if (total > 0.0f) {
        descriptor.bIsDead = !descriptor.bDamageDontKill;
        descriptor.bIsUnconscious = descriptor.bDamageDontKill != 0;
    }
}

void oCNpc::OnDamage_Condition(oSDamageDescriptor& descriptor)
{
    if (descriptor.bDamageDontKill) {
        descriptor.bIsDead = 0;
        if (descriptor.fDamageEffective > 0.0f)
            descriptor.bIsUnconscious = 1;
    } else if (descriptor.fDamageEffective > 0.0f) {
        descriptor.bIsDead = 1;
        descriptor.bIsUnconscious = 0;
    }
}

void oCNpc::OnDamage_Script(oSDamageDescriptor& descriptor)
{
    Interrupt(0, 1);
    if (!descriptor.bIsDead)
        AssessDamage_S(descriptor.pNpcAttacker, static_cast<int>(descriptor.fDamageEffective));
}

void oCNpc::OnDamage_Effects(oSDamageDescriptor& descriptor)
{
    if (descriptor.bFinished)
        OnDamage_Effects_End(descriptor);
    if (descriptor.bOnce)
        OnDamage_Effects_Start(descriptor);
}

void oCNpc::OnDamage_Effects_Start(oSDamageDescriptor& descriptor)
{
    if (descriptor.fDamageEffective > 0.0f) {
        bloodEnabled = 1;
        bloodFlow = 1;
        didHit = 1;
    }
}

void oCNpc::OnDamage_Effects_End(oSDamageDescriptor& descriptor)
{
    bloodFlow = 0;
    descriptor.pParticleFX = 0;
    descriptor.pVobParticleFX = 0;
    descriptor.SetVisualFX(0);
}

void oCNpc::OnDamage_Anim(oSDamageDescriptor& descriptor)
{
    didHit = 1;
    isDefending = 0;
}

void oCNpc::OnDamage_Sound(oSDamageDescriptor& descriptor)
{
    if (!zsound)
        return;
    static zSTRING damageSound;
    damageSound = zSTRING("SVM_") + zSTRING(0);
    damageSound += descriptor.bIsDead ? "_DEAD" : "_AARGH";
    zsound->PlaySound3D(damageSound, this, 0, 0);
}

void oCNpc::OnDamage_Events(oSDamageDescriptor& descriptor)
{
    if (descriptor.bIsUnconscious)
        DropUnconscious(descriptor.pNpcAttacker, descriptor.fAzimuth);
    if (descriptor.bIsDead)
        DoDie(descriptor.pNpcAttacker);
}

void oCNpc::OnDamage_State(oSDamageDescriptor& descriptor)
{
    if (!(descriptor.enuModeDamage & 8) || descriptor.aryDamageEffective[3] <= 1 ||
        HasBodyStateModifier(0x400))
        return;
    oCAniCtrl_Human* anictrl = GetAnictrl();
    if (!anictrl || !anictrl->GetWaterLevel())
        ModifyBodyState(0x400, 0);
}

void oCNpc::OnDamage(zCVob* attacker, zCVob* hit, float damageValue, int damageType,
    const zVEC3& hitLocation)
{
    oSDamageDescriptor descriptor;
    descriptor.dwFieldsValid = 0xffffffff;
    descriptor.pVobAttacker = attacker;
    descriptor.pNpcAttacker = dynamic_cast<oCNpc*>(attacker);
    descriptor.pVobHit = hit;
    descriptor.pItemWeapon = 0;
    descriptor.nSpellID = 0;
    descriptor.nSpellCat = 0;
    descriptor.nSpellLevel = 0;
    descriptor.enuModeDamage = damageType;
    descriptor.enuModeWeapon = 0;
    for (int i = 0; i < 8; ++i) {
        descriptor.aryDamage[i] = 0;
        descriptor.aryDamageEffective[i] = 0;
    }
    descriptor.fDamageTotal = damageValue;
    descriptor.fDamageMultiplier = 1.0f;
    descriptor.vecLocationHit = hitLocation;
    descriptor.vecDirectionFly = zVEC3();
    descriptor.fTimeDuration = 0.0f;
    descriptor.fTimeInterval = 0.0f;
    descriptor.fDamagePerInterval = 0.0f;
    descriptor.bDamageDontKill = 0;
    descriptor.bOnce = 1;
    descriptor.bFinished = 0;
    descriptor.bIsDead = 0;
    descriptor.bIsUnconscious = 0;
    descriptor.lReserved = 0;
    descriptor.fAzimuth = 0.0f;
    descriptor.fElevation = 0.0f;
    descriptor.fTimeCurrent = 0.0f;
    descriptor.fDamageReal = damageValue;
    descriptor.fDamageEffective = 0.0f;
    OnDamage(descriptor);
}

bool zSTRING::IsEmpty() const
{
    return length == 0;
}

int operator==(const zSTRING& first, const zSTRING& second)
{
    return first == second;
}

unsigned short zCEventMessage::GetSubType() const
{
    return subType;
}

zCWorld* zCVob::GetHomeWorld() const
{
    return homeWorld;
}

static zCClassDef oCMsgDamageClassDef("oCMsgDamage", "oCNpcMessage", oCMsgDamage::_CreateNewInstance,
    0, 0x118, 0, "");
zCClassDef* oCMsgDamage::classDef = &oCMsgDamageClassDef;

void* oCMsgDamage::operator new(unsigned int size)
{
    zCObject* object = static_cast<zCObject*>(::operator new(size));
    zCClassDef::ObjectCreated(object, classDef);
    return object;
}

static zCClassDef oCVisualFXClassDef("oCVisualFX", "zCEffect",
    oCVisualFX::_CreateNewInstance, 1, 0x610, 1, "");
zCClassDef* oCVisualFX::classDef = &oCVisualFXClassDef;

void* oCVisualFX::operator new(unsigned int size)
{
    zCObject* object = static_cast<zCObject*>(::operator new(size));
    zCClassDef::ObjectCreated(object, classDef);
    return object;
}

const zSTRING& oCVisualFX::GetName() const
{
    return fxName;
}

zSTRING& zSTRING::operator+=(const char* value)
{
    zSTRING result(*this);
    result.Append(value);
    *this = result;
    return *this;
}

float& zVEC3::operator[](int index)
{
    return n[index];
}

int oCAniCtrl_Human::GetActionMode() const
{
    return actionMode;
}

int oCNpc::HasFlag(unsigned long value, unsigned long flag)
{
    return (value & flag) == flag;
}
