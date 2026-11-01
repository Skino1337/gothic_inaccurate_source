#include "include/oAiShoot.h"
#include "include/zAlgebra.h"
#include <cmath>

static zSTRING arrowTrailVisual("ArrowTrail.3ds");
static int aiActionState = -1;
static zSTRING aiActionPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING aiActionPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING aiActionPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING aiActionPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING aiActionPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING aiActionPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING aiActionPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING aiActionPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING aiActionPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING aiActionPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float aiActionFarDistance = 250000.0f * 0.0166666675f;
static float aiActionNearDistance = aiActionFarDistance * 0.0166666675f;
static float aiActionFarPlaneDistance = 250000.0f * 24.0f;
static zCClassDef oCAISoundClassDef("oCAISound", "oCAIBase", oCAISound::_CreateNewInstance,
    0, 0x2C, 0, "");
zCClassDef* oCAISound::classDef = &oCAISoundClassDef;
static zCClassDef oCAIVobMoveClassDef("oCAIVobMove", "oCAIBase", oCAIVobMove::_CreateNewInstance,
    0, 0x3C, 0, "");
zCClassDef* oCAIVobMove::classDef = &oCAIVobMoveClassDef;

oCAISound::oCAISound()
    : slideSoundHandle(0), slideSoundOn(0)
{
}

oCAISound::~oCAISound()
{
    RemoveSlideSound();
}

zCClassDef* oCAISound::_GetClassDef() const
{
    return classDef;
}

void oCAISound::RemoveSlideSound()
{
    if (slideSoundOn) {
        slideSoundOn = 0;
        zsound->StopSound(slideSoundHandle);
    }
}

void oCAISound::CheckSlideSound(zCVob* vob)
{
    if ((vob->showVisual & 8) && vob->GetRigidBody()->mode) {
        if (slideSoundOn) {
            zsound->UpdateSound3D(slideSoundHandle, 0);
            return;
        }
        slideSoundOn = 1;
        int material = vob->groundPoly ? vob->groundPoly->material->matGroup : 0;
        slideSoundHandle = zsndMan->StartSlideSound(vob, material);
        return;
    }
    RemoveSlideSound();
}

int oCAISound::GetSoundMaterial(zCVob* vob, zCSoundManager::zTSndManMedium& medium)
{
    medium = zCSoundManager::zSND_MAN_MEDIUM_ITEM;
    if (!vob)
        return 0x63;
    if (vob->type == zVOB_TYPE_ITEM)
        return static_cast<oCItem*>(vob)->GetSoundMaterial();
    oCMOB* mob = dynamic_cast<oCMOB*>(vob);
    if (mob)
        return mob->GetSoundMaterial();
    if (vob->type == zVOB_TYPE_NSC) {
        oCNpc* npc = dynamic_cast<oCNpc*>(vob);
        if (npc && npc->GetEquippedArmor())
            return npc->GetEquippedArmor()->GetSoundMaterial();
        return 0;
    }
    return 0x63;
}

void oCAISound::StartCollisionSound(zCVob* vob, const zCCollisionReport& report)
{
    zCSoundManager::zTSndManMedium medium;
    int material = GetSoundMaterial(vob, medium);
    if (report.m_poCollObject1->GetCollObjClass() != &zCCollObjectLevelPolys::s_oCollObjClass) {
        zCSoundManager::zTSndManMedium otherMedium;
        int otherMaterial = GetSoundMaterial(
            static_cast<zCVob*>(report.m_poCollObject1->m_pvClientData), otherMedium);
        zsndMan->StartHitSound(vob, medium, otherMedium, material, otherMaterial,
            const_cast<zCCollisionReport*>(&report));
        return;
    }
    int levelMaterial = 1;
    zCCollObjectLevelPolys* level = static_cast<zCCollObjectLevelPolys*>(report.m_poCollObject1);
    if (level->m_aoPolyList.numInArray > 0) {
        switch (level->m_aoPolyList.parray[0]->material->matGroup) {
        case 1:
            levelMaterial = 2;
            break;
        case 3:
            levelMaterial = 0;
            break;
        case 4:
            levelMaterial = 4;
            break;
        case 5:
            levelMaterial = 5;
            break;
        case 6:
            levelMaterial = 3;
            break;
        }
    }
    zsndMan->StartHitSound(vob, medium, zCSoundManager::zSND_MAN_MEDIUM_LEVEL,
        material, levelMaterial, const_cast<zCCollisionReport*>(&report));
}

oCAIVobMove::oCAIVobMove()
    : vob(0), owner(0)
{
    slideSoundOn = 0;
}

oCAIVobMove::~oCAIVobMove()
{
    ClearIgnoreCDVob();
    if (vob)
        vob->Release();
    if (owner)
        owner->Release();
    vob = 0;
    owner = 0;
}

zCClassDef* oCAIVobMove::_GetClassDef() const
{
    return classDef;
}

int oCAIVobMove::CanThisCollideWith(zCVob* other)
{
    for (int i = 0; i < ignoreVobList.numInArray; ++i) {
        if (ignoreVobList.parray[i] == other)
            return 0;
    }
    return 1;
}

void oCAIVobMove::Init(zCVob* newVob, zCVob* newOwner, zVEC3& position, float speedXY,
    float speedZ, zMAT4* trajectory)
{
    ClearIgnoreCDVob();
    if (owner)
        owner->Release();
    if (vob)
        vob->Release();
    owner = newOwner;
    if (owner)
        owner->AddRef();
    vob = newVob;
    if (vob) {
        vob->AddRef();
        vob->SetAI(reinterpret_cast<zCAIBase*>(this));
        if (!vob->homeWorld && owner && owner->homeWorld)
            owner->homeWorld->AddVob(vob);
        vob->SetCollDetStat(0);
        vob->SetCollDetDyn(0);
        vob->ResetRotationsWorld();
        vob->SetPositionWorld(position);
        vob->SetCollDetStat(1);
        vob->SetCollDetDyn(1);
        vob->SetSleeping(0);
        vob->SetPhysicsEnabled(1);
        if (trajectory) {
            zVEC3 velocity = trajectory->GetAtVector();
            velocity *= speedXY;
            velocity.n[2] += speedZ;
            zCRigidBody* body = vob->GetRigidBody();
            body->gravityOn = 1;
            body->SetVelocity(velocity);
        }
        AddIgnoreCDVob(owner);
    }
}

void oCAIVobMove::DoAI(zCVob* other, int& state)
{
    state = 0;
    for (int i = ignoreVobList.numInArray - 1; i >= 0; --i) {
        zCVob* ignored = ignoreVobList.parray[i];
        if (ignored == owner)
            continue;
        int touching = 0;
        for (int j = 0; j < ignored->touchVobList.numInArray; ++j) {
            if (ignored->touchVobList.parray[j] == vob) {
                touching = 1;
                break;
            }
        }
        if (!touching) {
            ignoreVobList.Remove(ignored);
            ignored->Release();
        }
    }
    if (vob && !(vob->showVisual & 8))
        vob->SetSleeping(1);
}

void oCAIVobMove::ReportCollisionToAI(const zCCollisionReport& report)
{
    StartCollisionSound(vob, report);
    if (vob)
        oCNpc::AssessQuietSound_S(vob, vob->trafoObjToWorld.GetTranslation());
}

void oCAIVobMove::Archive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    archiver.WriteObject(vob);
    archiver.WriteObject(owner);
}

void oCAIVobMove::Unarchive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    ClearIgnoreCDVob();
    if (vob)
        vob->Release();
    if (owner)
        owner->Release();
    vob = static_cast<zCVob*>(archiver.ReadObject(0));
    owner = static_cast<zCVob*>(archiver.ReadObject(0));
    AddIgnoreCDVob(owner);
}

void oCAIVobMove::AddIgnoreCDVob(zCVob* ignored)
{
    if (ignored) {
        ignoreVobList.InsertEnd(ignored);
        ignored->AddRef();
    }
}

void oCAIVobMove::ClearIgnoreCDVob()
{
    for (int i = 0; i < ignoreVobList.numInArray; ++i)
        ignoreVobList.parray[i]->Release();
    ignoreVobList.numInArray = 0;
}

static zCClassDef oCAIVobMoveTorchClassDef("oCAIVobMoveTorch", "oCAIVobMove",
    oCAIVobMoveTorch::_CreateNewInstance, 0, 0x40, 0, "");
zCClassDef* oCAIVobMoveTorch::classDef = &oCAIVobMoveTorchClassDef;

oCAIVobMoveTorch::oCAIVobMoveTorch()
    : timer(0.0f)
{
    slideSoundOn = 0;
}

oCAIVobMoveTorch::~oCAIVobMoveTorch()
{
}

zCClassDef* oCAIVobMoveTorch::_GetClassDef() const
{
    return classDef;
}

void oCAIVobMoveTorch::Init(zCVob* newVob, zCVob* newOwner, zVEC3& position, float speedXY,
    float speedZ, zMAT4* trajectory)
{
    timer = 30000.0f;
    oCAIVobMove::Init(newVob, newOwner, position, speedXY, speedZ, trajectory);
}

void oCAIVobMoveTorch::DoAI(zCVob* other, int& state)
{
    int water = CheckWater();
    oCAIVobMove::DoAI(other, state);
    if (!state)
        state = water;
}

int oCAIVobMoveTorch::CheckWater()
{
    if (!vob || !vob->homeWorld)
        return 0;
    zVEC3 origin = {
        vob->trafoObjToWorld.m[0][3], vob->bbox3D.maxs.n[1], vob->trafoObjToWorld.m[2][3]
    };
    zVEC3 direction = { 0.0f, -1000000.0f, 0.0f };
    vob->homeWorld->TraceRayNearestHit(origin, direction, vob, 0x20);
    zTTraceRayReport& report = vob->homeWorld->traceRayReport;
    if (report.foundHit && report.foundPoly && report.foundPoly->material
        && report.foundPoly->material->matGroup == 5) {
        BurnedOut();
        return 1;
    }
    return 0;
}

void oCAIVobMoveTorch::BurnedOut()
{
    timer = 0.0f;
    if (!vob)
        return;
    zCWorld* world = vob->homeWorld;
    zMAT4 trafo = vob->trafoObjToWorld;
    zVEC3 velocity = vob->GetVelocity();
    oCNpc* npc = dynamic_cast<oCNpc*>(owner);
    if (npc) {
        if (npc->homeWorld)
            npc->homeWorld->RemoveVobSubtree(vob);
        npc->RemoveFromSlot(npc->GetInvSlot(vob), 0, 1);
    }
    zSTRING instance = "ITLSTORCHBURNED";
    zCVob* burned = ogame->GetGameWorld()->CreateVob((zTVobType)0x81, instance);
    burned->SetCollDetStat(0);
    burned->SetCollDetDyn(0);
    burned->SetTrafoObjToWorld(trafo);
    if (world)
        world->AddVob(burned);
    burned->SetPhysicsEnabled(1);
    burned->SetSleeping(0);
    burned->GetRigidBody()->SetVelocity(velocity);
    burned->SetCollDetStat(1);
    burned->SetCollDetDyn(1);
}

extern zTIMER ztimer;

static zCClassDef oCAIArrowBaseClassDef("oCAIArrowBase", "oCAIBase",
    oCAIArrowBase::_CreateNewInstance, 0, 0x58, 0, "");
zCClassDef* oCAIArrowBase::classDef = &oCAIArrowBaseClassDef;
static zCClassDef oCAIArrowClassDef("oCAIArrow", "oCAIArrowBase", oCAIArrow::_CreateNewInstance,
    0, 0x68, 0, "");
zCClassDef* oCAIArrow::classDef = &oCAIArrowClassDef;

oCAIArrowBase::oCAIArrowBase()
    : collisionOccured(0), timeLeft(-1.0f), vob(0), startDustFX(0), trailVob(0),
      trailStrip(0), trailActive(0), trailTime(0.0f), hasHit(1)
{
    slideSoundOn = 0;
}

oCAIArrowBase::~oCAIArrowBase()
{
    if (vob)
        vob->Release();
    if (trailVob) {
        trailVob->RemoveVobSubtreeFromWorld();
        trailVob->Release();
    }
    if (trailStrip)
        reinterpret_cast<zCVisual*>(trailStrip)->Release();
    ClearIgnoreCDVob();
}

zCClassDef* oCAIArrowBase::_GetClassDef() const
{
    return classDef;
}

int oCAIArrowBase::HasAIDetectedCollision()
{
    return 0;
}

void oCAIArrowBase::AICollisionResponseSelfDetected(const zVEC3& normal, int& state)
{
}

int oCAIArrowBase::GetIsProjectile()
{
    return 1;
}

void oCAIArrowBase::AddIgnoreCDVob(zCVob* ignored)
{
    if (ignored) {
        ignoreVobList.InsertEnd(ignored);
        ignored->AddRef();
    }
}

void oCAIArrowBase::ClearIgnoreCDVob()
{
    for (int i = 0; i < ignoreVobList.numInArray; ++i)
        ignoreVobList.parray[i]->Release();
    ignoreVobList.numInArray = 0;
}

void oCAIArrowBase::CreateTrail(zCVob* target)
{
    trailStrip = new zCPolyStrip;
    trailStrip->camAlign = zCPolyStrip::zCAM_ALIGN_STRIP;
    trailStrip->alphaFadeSpeed = 0.3f;
    trailStrip->material->rndAlphaBlendFunc = zRND_ALPHA_FUNC_BLEND;
    zSTRING texture = "ARROWSTRIP.TGA";
    trailStrip->material->SetTexture(texture);
    trailStrip->material->texAniCtrl.aniFPS = 0.01f;
    trailStrip->material->texAniCtrl.bOneShotAni = 1;
    trailStrip->material->color[3] = 255;
    trailStrip->AllocateResources(0x40);
    trailStrip->everyFrameUpdate = 1;
    trailTime = 100.0f;
    trailVob = new zCVob;
    trailVob->SetVisual(trailStrip);
    if (target && target->homeWorld) {
        trailVob->SetPositionWorld(target->trafoObjToWorld.m[0][3],
            target->trafoObjToWorld.m[1][3], target->trafoObjToWorld.m[2][3]);
        target->homeWorld->AddVobAsChild(trailVob, target);
    }
}

void oCAIArrowBase::DoAI(zCVob* other, int& state)
{
    state = 0;
    if (trailStrip) {
        trailTime -= ztimer.frameTimeFloat;
        if (trailTime <= 0.0f) {
            trailTime = 100.0f;
            if (other)
                trailStrip->AddSegment(other->trafoObjToWorld.GetTranslation());
        }
    }
    if (collisionOccured) {
        collisionOccured = 0;
        startDustFX = 1;
    }
    if (timeLeft != -1.0f) {
        timeLeft -= ztimer.frameTimeFloat * 0.00005f;
        if (timeLeft <= 0.0f)
            state = 1;
        if (vob) {
            vob->visualAlpha = (std::sin(timeLeft * 3.1415926535897931f - 1.5707963267948966f)
                + 1.0f) * 0.5f;
            vob->showVisual = (vob->showVisual & ~4) ^ 4;
        }
    }
    if (state && trailVob) {
        trailVob->RemoveVobSubtreeFromWorld();
        trailVob->Release();
        trailVob = 0;
        if (trailStrip) {
            trailStrip->Release();
            trailStrip = 0;
        }
        oCItem* item = dynamic_cast<oCItem*>(other);
        if (item)
            item->RemoveEffect();
    }
}

void oCAIArrowBase::ReportCollisionToAI(const zCCollisionReport& report)
{
    startDustFX = 1;
    if (vob) {
        vob->SetSleeping(0);
        if (timeLeft == -1.0f)
            timeLeft = 1.0f;
        zCRigidBody* body = vob->GetRigidBody();
        body->gravityOn = 1;
        if (report.m_poCollObject1->GetCollObjClass() == &zCCollObjectLevelPolys::s_oCollObjClass) {
            body->v *= 0.8f;
            body->SetVelocity(body->v);
        } else {
            oCNpc* npc = dynamic_cast<oCNpc*>(
                static_cast<zCVob*>(report.m_poCollObject1->m_pvClientData));
            oCItem* armor = npc ? npc->GetEquippedArmor() : 0;
            if (hasHit || !armor || armor->GetSoundMaterial() != 1) {
                vob->SetPhysicsEnabled(0);
                if (timeLeft > 0.1f)
                    timeLeft = 0.05f;
            }
        }
    }
    collisionOccured = 1;
}

void oCAIArrowBase::Archive(zCArchiver& archiver)
{
    if (archiver.InProperties() || !archiver.InSaveGame())
        return;
    archiver.WriteObject(vob);
    archiver.WriteBool(collisionOccured);
    archiver.WriteFloat(timeLeft);
    archiver.WriteBool(startDustFX);
}

void oCAIArrowBase::Unarchive(zCArchiver& archiver)
{
    if (archiver.InProperties() || !archiver.InSaveGame())
        return;
    if (vob)
        vob->Release();
    vob = static_cast<zCVob*>(archiver.ReadObject(0));
    collisionOccured = archiver.ReadBool();
    timeLeft = archiver.ReadFloat();
    startDustFX = archiver.ReadBool();
    if (trailVob) {
        trailVob->Release();
        trailVob = 0;
    }
}

oCAIArrow::oCAIArrow()
    : arrow(0), owner(0), removeVob(0), targetNPC(0)
{
}

oCAIArrow::~oCAIArrow()
{
    ClearUsedVobs();
}

zCClassDef* oCAIArrow::_GetClassDef() const
{
    return classDef;
}

void oCAIArrow::SetTarget(zCVob* target)
{
    if (targetNPC)
        targetNPC->Release();
    targetNPC = target;
    if (targetNPC)
        targetNPC->AddRef();
}

void oCAIArrow::ClearUsedVobs()
{
    ClearIgnoreCDVob();
    if (vob) {
        vob->Release();
        vob = 0;
    }
    if (arrow) {
        arrow->Release();
        arrow = 0;
    }
    if (owner) {
        owner->Release();
        owner = 0;
    }
    if (targetNPC) {
        targetNPC->Release();
        targetNPC = 0;
    }
}

void oCAIArrow::SetupAIVob(zCVob* newVob, zCVob* newOwner, zCVob* target)
{
    ClearUsedVobs();
    SetTarget(target);
    collisionOccured = 0;
    vob = newVob;
    if (vob)
        vob->AddRef();
    owner = dynamic_cast<oCNpc*>(newOwner);
    if (owner)
        owner->AddRef();
    arrow = dynamic_cast<oCItem*>(newVob);
    if (arrow)
        arrow->AddRef();
    AddIgnoreCDVob(newOwner);
    if (vob) {
        vob->SetSleeping(0);
        vob->SetPhysicsEnabled(1);
        if (owner) {
            zSTRING node = "ZS_RIGHTHAND";
            vob->SetTrafoObjToWorld(owner->GetTrafoModelNodeToWorld(node));
        }
        vob->trafoObjToWorld.PostRotateY(-90.0f);
        zVEC3 velocity = vob->trafoObjToWorld.GetAtVector();
        if (targetNPC) {
            zVEC3 targetPosition = targetNPC->trafoObjToWorld.GetTranslation();
            zVEC3 position = vob->trafoObjToWorld.GetTranslation();
            velocity = zVEC3(targetPosition.n[0] - position.n[0],
                targetPosition.n[1] - position.n[1], targetPosition.n[2] - position.n[2]);
            velocity.Normalize();
            vob->SetHeadingAtWorld(velocity);
            vob->trafoObjToWorld.PostRotateY(90.0f);
        }
        velocity *= 2000.0f;
        zCRigidBody* body = vob->GetRigidBody();
        body->SetVelocity(velocity);
        body->gravityOn = 0;
        vob->SetCollDetStat(1);
        vob->SetCollDetDyn(1);
        vob->SetCollisionClass(&zCCollObjectProjectile::s_oCollObjClass);
        if (arrow)
            arrow->SetFlag(0x800000);
        CreateTrail(vob);
    }
}

void oCAIArrow::DoAI(zCVob* other, int& state)
{
    oCAIArrowBase::DoAI(other, state);
    if (state) {
        removeVob = 1;
        if (other)
            other->SetAI(0);
    }
}

int oCAIArrow::CanThisCollideWith(zCVob* other)
{
    if (targetNPC) {
        oCNpc* npc = dynamic_cast<oCNpc*>(other);
        if (npc && npc != targetNPC && !npc->IsDead())
            return 0;
    }
    for (int i = 0; i < ignoreVobList.numInArray; ++i) {
        if (ignoreVobList.parray[i] == other)
            return 0;
    }
    return 1;
}

void oCAIArrow::ReportCollisionToAI(const zCCollisionReport& report)
{
    collisionOccured = 0;
    hasHit = 1;
    StartCollisionSound(arrow, report);
    oCNpc* npc = dynamic_cast<oCNpc*>(static_cast<zCVob*>(report.m_poCollObject1->m_pvClientData));
    if (npc) {
        AddIgnoreCDVob(npc);
        if (owner && owner != npc && arrow) {
            static int constantsInitialized = 0;
            static float minDistance = 1000.0f;
            static float maxDistance = 5000.0f;
            if (!constantsInitialized) {
                constantsInitialized = 1;
                zCPar_Symbol* symbol = parser.GetSymbol("RANGED_CHANCE_MINDIST");
                if (symbol)
                    symbol->GetValue(minDistance, 0);
                symbol = parser.GetSymbol("RANGED_CHANCE_MAXDIST");
                if (symbol)
                    symbol->GetValue(maxDistance, 0);
            }
            zVEC3 ownerPosition = owner->trafoObjToWorld.GetTranslation();
            zVEC3 targetPosition = npc->trafoObjToWorld.GetTranslation();
            zVEC3 delta(targetPosition.n[0] - ownerPosition.n[0],
                targetPosition.n[1] - ownerPosition.n[1], targetPosition.n[2] - ownerPosition.n[2]);
            float distance = delta.Length();
            int chance = owner->GetHitChance(owner->GetWeaponMode() != 5 ? 4 : 3);
            if (distance > minDistance)
                chance = static_cast<int>(-chance * distance / maxDistance + chance);
            else
                chance = static_cast<int>((chance - 100.0f) * distance / minDistance + 100.0f);
            if (rand() % 100 >= chance) {
                hasHit = 0;
            } else {
                npc->OnDamage(owner, arrow, arrow->GetFullDamage(), arrow->GetDamageTypes(),
                    arrow->trafoObjToWorld.GetTranslation());
                oCNpc::AssessFightSound_S(owner, arrow->trafoObjToWorld.GetTranslation(), npc);
            }
        }
    }
    AddIgnoreCDVob(arrow);
    removeVob = 1;
    oCAIArrowBase::ReportCollisionToAI(report);
}

void oCAIArrow::Archive(zCArchiver& archiver)
{
    oCAIArrowBase::Archive(archiver);
    if (archiver.InProperties() || !archiver.InSaveGame())
        return;
    archiver.WriteObject(arrow);
    archiver.WriteObject(owner);
}

void oCAIArrow::Unarchive(zCArchiver& archiver)
{
    oCAIArrowBase::Unarchive(archiver);
    if (archiver.InProperties() || !archiver.InSaveGame())
        return;
    ClearIgnoreCDVob();
    if (arrow)
        arrow->Release();
    if (owner)
        owner->Release();
    if (targetNPC)
        targetNPC->Release();
    arrow = static_cast<oCItem*>(archiver.ReadObject(0));
    owner = static_cast<oCNpc*>(archiver.ReadObject(0));
    targetNPC = 0;
}

static zCClassDef oCAIDropClassDef("oCAIDrop", "oCAIBase", oCAIDrop::_CreateNewInstance,
    0, 0x48, 0, "");
zCClassDef* oCAIDrop::classDef = &oCAIDropClassDef;

oCAIDrop::oCAIDrop()
    : vob(0), owner(0), collisionOccured(0), timer(0.0f), count(0.0f)
{
    slideSoundOn = 0;
}

oCAIDrop::~oCAIDrop()
{
    ClearIgnoreCDVob();
    if (vob)
        vob->Release();
    if (owner)
        owner->Release();
}

zCObject* oCAIDrop::_CreateNewInstance()
{
    oCAIDrop* ai = new oCAIDrop;
    zCClassDef::ObjectCreated(ai, classDef);
    return ai;
}

void oCAIDrop::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* oCAIDrop::_GetClassDef() const
{
    return classDef;
}

int oCAIDrop::HasAIDetectedCollision()
{
    return 0;
}

int oCAIDrop::CanThisCollideWith(zCVob* other)
{
    for (int i = 0; i < ignoreVobList.numInArray; ++i) {
        if (ignoreVobList.parray[i] == other)
            return 0;
    }
    return 1;
}

void oCAIDrop::AddIgnoreCDVob(zCVob* ignored)
{
    if (ignored) {
        ignoreVobList.InsertEnd(ignored);
        ignored->AddRef();
    }
}

void oCAIDrop::ClearIgnoreCDVob()
{
    for (int i = 0; i < ignoreVobList.numInArray; ++i)
        ignoreVobList.parray[i]->Release();
    ignoreVobList.numInArray = 0;
}

void oCAIDrop::SetupAIVob(zCVob* newVob, zCVob* newOwner)
{
    ClearIgnoreCDVob();
    if (vob)
        vob->Release();
    if (owner)
        owner->Release();
    vob = newVob;
    if (vob)
        vob->AddRef();
    owner = newOwner;
    if (owner)
        owner->AddRef();
    collisionOccured = 0;
    count = 0.0f;
    timer = 0.0f;
    if (vob) {
        if (newOwner) {
            zSTRING node = "ZS_RIGHTHAND";
            vob->SetTrafoObjToWorld(newOwner->GetTrafoModelNodeToWorld(node));
        }
        vob->SetCollDetStat(0);
        vob->SetCollDetDyn(0);
        if (!vob->homeWorld && newOwner && newOwner->homeWorld)
            newOwner->homeWorld->AddVob(vob);
        vob->ResetRotationsWorld();
        zVEC3 position = vob->trafoObjToWorld.GetTranslation();
        SetStartPosition(position);
        if (newOwner) {
            zVEC3 velocity = newOwner->trafoObjToWorld.GetRightVector();
            velocity *= 50.0f;
            vob->GetRigidBody()->SetVelocity(velocity);
        }
        vob->GetRigidBody()->gravityOn = 1;
        vob->SetSleeping(0);
        vob->SetPhysicsEnabled(1);
        AddIgnoreCDVob(owner);
        if (vob->type == zVOB_TYPE_ITEM)
            static_cast<oCItem*>(vob)->ClearFlag(0x800000);
        vob->SetCollDetStat(0);
        vob->SetCollDetDyn(0);
    }
}

void oCAIDrop::DoAI(zCVob* other, int& state)
{
    ++count;
    if (other && !(other->showVisual & 8)) {
        if (count <= 10.0f) {
            other->SetPhysicsEnabled(1);
            return;
        }
        other->SetCollDetStat(1);
        other->SetCollDetDyn(1);
        other->SetSleeping(1);
        other->SetAI(0);
    }
    state = 0;
}

void oCAIDrop::ReportCollisionToAI(const zCCollisionReport& report)
{
    StartCollisionSound(vob, report);
    if (vob)
        oCNpc::AssessQuietSound_S(vob, vob->trafoObjToWorld.GetTranslation());
}

void oCAIDrop::SetVelocity(float speed, float angle)
{
    if (!vob || !owner)
        return;
    zMAT4 rotation = Alg_Rotation3DN(zVEC3(1.0f, 0.0f, 0.0f), -angle);
    zMAT4 trafo = owner->trafoObjToWorld * rotation;
    zVEC3 velocity = trafo.GetAtVector();
    velocity *= speed;
    vob->GetRigidBody()->SetVelocity(velocity);
}

void oCAIDrop::SetStartPosition(zVEC3& position)
{
    if (!vob)
        return;
    vob->SetCollDetStat(0);
    vob->SetCollDetDyn(0);
    vob->SetPositionWorld(position);
    vob->ResetRotationsWorld();
    vob->SetCollDetStat(1);
    vob->SetCollDetDyn(1);
}

void oCAIDrop::Archive(zCArchiver& archiver)
{
    archiver.InProperties();
}

void oCAIDrop::Unarchive(zCArchiver& archiver)
{
    archiver.InProperties();
}

zCObject* oCAIArrowBase::_CreateNewInstance()
{
    oCAIArrowBase* ai = new oCAIArrowBase;
    zCClassDef::ObjectCreated(ai, classDef);
    return ai;
}

void oCAIArrowBase::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCObject* oCAIArrow::_CreateNewInstance()
{
    oCAIArrow* ai = new oCAIArrow;
    zCClassDef::ObjectCreated(ai, classDef);
    return ai;
}

void oCAIArrow::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCObject* oCAIVobMove::_CreateNewInstance()
{
    oCAIVobMove* ai = new oCAIVobMove;
    zCClassDef::ObjectCreated(ai, classDef);
    return ai;
}

void oCAIVobMove::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCObject* oCAIVobMoveTorch::_CreateNewInstance()
{
    oCAIVobMoveTorch* ai = new oCAIVobMoveTorch;
    zCClassDef::ObjectCreated(ai, classDef);
    return ai;
}

void oCAIVobMoveTorch::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}
