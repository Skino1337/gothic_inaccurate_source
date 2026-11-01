#include "include/oGame.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/zFlash.h"
#include "include/zSound.h"
#include <cmath>

static int frontierWarningState;
static zCSoundFX* frontierWarningSound;
static int frontierWarningChannel;

oCMagFrontier::oCMagFrontier()
    : warningFX(NULL), shootFX(NULL), npc(NULL), isWarning(0), isShooting(0)
{
}

oCMagFrontier::~oCMagFrontier()
{
    DisposeShootFX();
    DisposeWarningFX();
    npc = NULL;
}

void oCMagFrontier::SetNPC(oCNpc* value)
{
    npc = value;
}

int oCMagFrontier::DoCheck()
{
    if (npc != oCNpc::player || !gameMan || !ogame)
        return 0;

    zCWorld* world = ogame->GetWorld();
    if (!world || world->csPlayer->GetPlayingGlobalCutscene() || npc->GetAttribute(0) <= 0)
        return 0;

    zSTRING worldName = world->GetWorldName();
    zVEC3 position = npc->trafoObjToWorld.v[3];
    float distance = 10000000.0f;
    zVEC3 unused;
    if (worldName == "NEWWORLD")
        distance = GetDistanceNewWorld(position, distance, unused);
    else if (worldName == "DRAGONISLAND")
        distance = GetDistanceDragonIsland(position, distance, unused);
    else if (worldName == "ADDONWORLD")
        distance = GetDistanceAddonWorld(position, distance, unused);
    else
        return 0;

    if (distance < 1500.0f && distance > 0.0f)
    {
        if (ogame->game_testmode)
            ogame->GetTextView()->Printwin("Piranhas crunching your boots.");
        else
        {
            npc->SetAttribute(0, 0);
            gameMan->PlayVideoEx("FishFood.bik", 0, 1);
        }
        frontierWarningState = 2;
        return 1;
    }

    if (distance < 3000.0f && distance > 0.0f)
    {
        if (frontierWarningState != 1)
        {
            if (!frontierWarningSound)
                frontierWarningSound = zsound->LoadSoundFXScript("OCEAN_WARN");
            if (frontierWarningSound)
                frontierWarningChannel = zsound->PlaySound(frontierWarningSound, 0);
            if (ogame->game_testmode)
                ogame->GetTextView()->Printwin("Piranhas are watching you.");
        }
        frontierWarningState = 1;
        return 1;
    }

    if (frontierWarningState)
    {
        if (frontierWarningSound && zsound->IsSoundActive(frontierWarningChannel))
        {
            zsound->StopSound(frontierWarningChannel);
            frontierWarningChannel = 0;
        }
        if (ogame->game_testmode)
            ogame->GetTextView()->Printwin("Well, swim home to mommy.");
    }
    frontierWarningState = 0;
    return 1;
}

float oCMagFrontier::GetDistanceNewWorld(const zVEC3& position, float& distance, zVEC3&)
{
    distance = 10000000.0f;
    if ((position.n[0] == 0.0f && position.n[2] == 0.0f) ||
        (!oCNpc::godmode && position.n[1] < 200.0f))
        return distance;
    distance = std::sqrt(position.n[0] * position.n[0] + position.n[2] * position.n[2]);
    return distance;
}

float oCMagFrontier::GetDistanceDragonIsland(const zVEC3& position, float& distance, zVEC3&)
{
    distance = 10000000.0f;
    if (position.n[0] == 0.0f && position.n[2] == 0.0f)
        return distance;
    distance = std::sqrt(position.n[0] * position.n[0] + position.n[2] * position.n[2]);
    return distance;
}

float oCMagFrontier::GetDistanceAddonWorld(const zVEC3& position, float& distance, zVEC3&)
{
    distance = 10000000.0f;
    if (position.n[0] == 0.0f && position.n[2] == 0.0f)
        return distance;
    distance = std::sqrt(position.n[0] * position.n[0] + position.n[2] * position.n[2]);
    return distance;
}

void oCMagFrontier::StartLightningAtPos(zVEC3& start, zVEC3& end)
{
    zCFlash* flash = new zCFlash;
    flash->Create(start, end, 10000.0f);
    flash->SetTexture("lightningNEU_big_a0.tga");
    zCVob* boltVob = new zCVob;
    boltVob->SetVobName("Lightning-Vob");
    boltVob->SetPositionWorld(start);
    boltVob->SetVisual(flash);
    boltVob->SetSleeping(0);
    boltVob->SetCollDetStat(0);
    boltVob->SetCollDetDyn(0);
    npc->homeWorld->AddVob(boltVob);
    boltVob->Release();
    flash->Release();

    zVEC3 direction = start - end;
    direction.Normalize();
    zCQuadMark* mark = new zCQuadMark;
    zCVob* impactVob = new zCVob;
    impactVob->SetVobName("Blitz-Einschlag-Vob");
    impactVob->SetPositionWorld(end);
    impactVob->SetVisual(mark);
    impactVob->SetSleeping(0);
    impactVob->SetCollDetStat(0);
    impactVob->SetCollDetDyn(0);
    zSTRING texture = "fireballquadmark.tga";
    mark->CreateQuadMark(end, direction * 10000.0f, zVEC2(100.0f, 100.0f), texture, NULL);
    npc->homeWorld->AddVob(impactVob);
    impactVob->Release();
    mark->Release();
}

void oCMagFrontier::DoWarningFX(int level)
{
    if (isWarning & 2)
        return;

    if (!(isWarning & 1))
    {
        isWarning |= 1;
        warningFX = new oCVisualFX;
        warningFX->SetByScript("BARRIERE_PC_WARNING");
        warningFX->SetPositionWorld(npc->trafoObjToWorld.v[3]);
        npc->homeWorld->AddVob(warningFX);
        warningFX->Init(npc, NULL, 0);
    }
    warningFX->SetLevel(level, 0);
}

void oCMagFrontier::DisposeWarningFX()
{
    if (warningFX)
    {
        warningFX->Stop(1);
        warningFX->Release();
        warningFX = NULL;
    }
    isWarning &= ~1;
}

void oCMagFrontier::DoShootFX(const zVEC3& position)
{
    if (isWarning & 2)
        return;

    isWarning |= 2;
    isShooting = 1;
    shootFX = new oCVisualFX;
    shootFX->SetByScript("BARRIERE_PC_SHOOT");
    shootFX->SetPositionWorld(npc->trafoObjToWorld.v[3]);
    npc->homeWorld->AddVob(shootFX);
    shootFX->Init(npc, NULL, 0);
    shootFX->Cast(1);

    zVEC3 direction = position - npc->trafoObjToWorld.v[3];
    direction.Normalize();
    npc->GetEM(0)->OnDamage(npc, shootFX, 5.0f, 33, direction);
    zVEC3 start = position + zVEC3(0.0f, 1000.0f, 0.0f);
    zVEC3 end = start + direction * 1000.0f;
    StartLightningAtPos(start, end);
}

void oCMagFrontier::DisposeShootFX()
{
    if (shootFX)
    {
        shootFX->Stop(1);
        shootFX->Release();
        shootFX = NULL;
    }
    isShooting = 0;
    isWarning &= ~2;
}
