#include "include/oSpawn.h"
#include "include/zSession.h"

float SPAWN_INSERTTIME_MAX = 1000.0f;
float SPAWN_INSERTRANGE = 4500.0f;
float SPAWN_REMOVERANGE = 5000.0f;

oCSpawnManager::oCSpawnManager()
    : spawningEnabled(1), insertTime(SPAWN_INSERTTIME_MAX), spawnFlags(0)
{
}

oCSpawnManager::~oCSpawnManager()
{
    ClearList();
}

void oCSpawnManager::SetInsertRange(float range)
{
    SPAWN_INSERTRANGE = range;
}

float oCSpawnManager::GetInsertRange()
{
    return SPAWN_INSERTRANGE;
}

void oCSpawnManager::SetRemoveRange(float range)
{
    SPAWN_REMOVERANGE = range;
}

float oCSpawnManager::GetRemoveRange()
{
    return SPAWN_REMOVERANGE;
}

void oCSpawnManager::SetSpawnTime(float time)
{
    SPAWN_INSERTTIME_MAX = time;
}

float oCSpawnManager::GetSpawnTime()
{
    return SPAWN_INSERTTIME_MAX;
}

void oCSpawnManager::ClearList()
{
    for (int i = 0; i < spawnList.numInArray; ++i)
    {
        spawnList[i]->npc->Release();
        delete spawnList[i];
    }
    spawnList.numInArray = 0;
}

void oCSpawnManager::InsertInList(oSSpawnNode* node)
{
    if (node)
        node->npc->AddRef();
    spawnList.InsertEnd(node);
}

void oCSpawnManager::RemoveFromList(oSSpawnNode* node)
{
    if (node)
        node->npc->Release();
    spawnList.Remove(node);
    delete node;
}

int oCSpawnManager::IsSpawningEnabled()
{
    return spawningEnabled;
}

void oCSpawnManager::SetSpawningEnabled(int enabled)
{
    spawningEnabled = enabled;
}

void oCSpawnManager::InitCameraPos()
{
    zCVob* vob = oCNpc::player;
    zCAICamera* camera = zCAICamera::GetCurrent();
    if (camera)
        vob = camera->camVob;
    if (vob)
        camPos = vob->trafoObjToWorld.GetTranslation();
}

void oCSpawnManager::SpawnImmediately(int immediately)
{
    insertTime = SPAWN_INSERTTIME_MAX;
    if (immediately)
    {
        for (int i = 0; i < spawnList.numInArray; ++i)
            spawnList[i]->timer = 0.0f;
    }
    int noLazyLoad = zoptions->Parm("NOLAZYLOAD");
    for (zCList<oCNpc>* node = (zCList<oCNpc>*)ogame->GetWorld()->voblist_npcs; node;
        node = node->next)
    {
        if (noLazyLoad)
            node->data->AvoidShrink(1000);
        CheckRemoveNpc(node->data);
    }
    if (!spawningEnabled)
        return;
    insertTime += ztimer.frameTimeFloat;
    if (insertTime > SPAWN_INSERTTIME_MAX)
    {
        insertTime -= SPAWN_INSERTTIME_MAX;
        InitCameraPos();
        CheckInsertNpc();
    }
}

void oCSpawnManager::CheckInsertNpcs()
{
    if (!spawningEnabled)
        return;
    insertTime += ztimer.frameTimeFloat;
    if (insertTime <= SPAWN_INSERTTIME_MAX)
        return;
    insertTime -= SPAWN_INSERTTIME_MAX;
    InitCameraPos();
    CheckInsertNpc();
}

void oCSpawnManager::ShowDebugInfo()
{
    screen->Print(0, 500, "Instance:");
    screen->Print(3000, 500, "Distance:");
    screen->Print(5000, 500, "Allowed:");
    screen->Print(7000, 500, "Delay:");
    int y = 500 + screen->FontY();
    for (int i = 0; i < spawnList.numInArray; ++i)
    {
        oSSpawnNode* node = spawnList[i];
        screen->Print(0, y, node->npc->GetInstanceName());
        screen->Print(3000, y, zSTRING((long)(node->spawnPos - camPos).Length()));
        screen->Print(5000, y, zSTRING(node->npc->showaidebug & 1));
        screen->Print(7000, y, zSTRING((long)node->timer));
        y += screen->FontY();
    }
}

void oCSpawnManager::CheckInsertNpc()
{
    for (int i = 0; i < spawnList.numInArray; ++i)
    {
        oSSpawnNode* node = spawnList[i];
        if (!node)
            break;
        oCNpc* npc = node->npc;
        if (!npc)
        {
            zerr.Warning("N: SPAWN: Invalid NPC in List!");
            RemoveFromList(node);
            --i;
            continue;
        }
        if (node->timer > 0.0f)
        {
            node->timer -= SPAWN_INSERTTIME_MAX;
            continue;
        }
        if (npc->IsDead() && CanDeadNpcBeRemoved(npc))
        {
            zVEC3 npcPos = npc->trafoObjToWorld.GetTranslation();
            if ((npcPos - camPos).LengthApprox() > SPAWN_REMOVERANGE)
            {
                zVEC3 checkPos;
                if (!npc->IsDead() || UseDeadNpcRoutinePos(npc))
                {
                    if (npc->state.hasRoutine)
                        checkPos = rtnMan.GetRoutinePos(npc);
                    else
                        checkPos = npc->state.GetAIStatePosition();
                }
                else
                    checkPos = npcPos;
                if ((checkPos - camPos).LengthApprox() > SPAWN_REMOVERANGE)
                {
                    DeleteNpc(npc);
                    break;
                }
            }
            continue;
        }
        InsertNpc(npc, node->spawnPos);
    }

    zTBBox3D bbox;
    bbox.mins = camPos - zVEC3(1.0f, 1.0f, 1.0f) * SPAWN_INSERTRANGE;
    bbox.maxs = camPos + zVEC3(1.0f, 1.0f, 1.0f) * SPAWN_INSERTRANGE;
    zCList<oCNpc> npcList = {};
    rtnMan.CollidesWithWayBox(bbox, npcList);
    for (zCList<oCNpc>* node = npcList.next; node; node = node->next)
    {
        oCNpc* npc = node->data;
        if (npc->IsAPlayer())
            continue;
        zVEC3 position;
        if (!npc->IsDead() || UseDeadNpcRoutinePos(npc))
        {
            if (npc->state.hasRoutine)
                position = rtnMan.GetRoutinePos(npc);
            else
                position = npc->state.GetAIStatePosition();
        }
        else
            position = npc->trafoObjToWorld.GetTranslation();
        InsertNpc(npc, position);
    }
}

int oCSpawnManager::InsertNpc(oCNpc* npc, const zVEC3& position)
{
    if ((position - camPos).LengthApprox() > SPAWN_INSERTRANGE)
    {
        npc->showaidebug |= 0x2000;
        return 0;
    }
    if (npc->showaidebug & 0x2000)
    {
        zVEC3 enablePos = position;
        npc->Enable(enablePos);
        return 1;
    }
    return 0;
}

oCNpc* oCSpawnManager::SummonNpc(int instance, const zVEC3& position, float angle)
{
    oCNpc* npc = zDYNAMIC_CAST<oCNpc>(ogame->GetGameWorld()->CreateVob(zVOB_TYPE_NSC, instance));
    if (!npc)
        return NULL;
    npc->SetPositionWorld(position);
    ogame->GetGameWorld()->AddVob(npc);
    zVEC3 npcPos = position;
    if (!npc->SearchNpcPosition(npcPos))
    {
        ogame->GetWorld()->RemoveVob(npc);
        return NULL;
    }
    npc->showaidebug |= 0x2008;
    npc->Enable(npcPos);
    SpawnNpc(npc, position, angle);
    npc->Release();
    return npc;
}

oCNpc* oCSpawnManager::SpawnNpc(int instance, const zSTRING& waypoint, float angle)
{
    oCNpc* npc = zDYNAMIC_CAST<oCNpc>(ogame->GetGameWorld()->CreateVob(zVOB_TYPE_NSC, instance));
    if (!npc)
        return NULL;
    SpawnNpc(npc, waypoint, angle);
    npc->Release();
    return npc;
}

void oCSpawnManager::SpawnNpc(oCNpc* npc, const zSTRING& waypoint, float angle)
{
    zCWaypoint* way = ogame->GetWorld()->wayNet->GetWaypoint(waypoint);
    zVEC3 position;
    if (way)
        position = way->GetPositionWorld();
    else
    {
        zCVob* vob = ogame->GetWorld()->SearchVobByName(waypoint);
        if (!vob)
        {
            zerr.Warning("U: SPAWN: Spawnpoint " + waypoint + " not found. Npc " +
                npc->GetInstanceName() + " cannot be spawned.");
            return;
        }
        position = vob->trafoObjToWorld.GetTranslation();
    }
    if (npc && (!npc->state.hasRoutine || npc->IsDead()))
    {
        npc->spawnPoint = waypoint;
    {
        if (npc->state.IsAIStateDriven())
            npc->state.InitAIStateDriven(position);
        oSSpawnNode* node = new oSSpawnNode;
        node->npc = npc;
        node->spawnPos = position;
        node->timer = angle;
        InsertInList(node);
    }
}

void oCSpawnManager::SpawnNpc(oCNpc* npc, const zVEC3& position, float angle)
{
    if (!npc || npc->state.hasRoutine && !npc->IsDead())
        return;
    if (npc->state.IsAIStateDriven())
        npc->state.InitAIStateDriven(position);
    oSSpawnNode* node = new oSSpawnNode;
    node->npc = npc;
    node->spawnPos = position;
    node->timer = angle;
    InsertInList(node);
}

void oCSpawnManager::CheckForInvalidDialogCamPos(float angle, oCNpc* npc)
{
    zCAICamera* camera = zCAICamera::GetCurrent();
    if (!camera || !camera->IsModeActive(CamModDialog))
        return;
    if (npc->state.IsInState(parser.GetIndex("ZS_TALK")) && angle > 3000.0f)
        zerr.Warning("U:SPAWN:Shitty DialogCam is not in valid range... >30m. Your dialog-npc may disappear...");
}

int oCSpawnManager::UseDeadNpcRoutinePos(oCNpc* npc)
{
    if (!npc)
        return 1;
    static int useRoutinePos = -2;
    if (useRoutinePos == -2)
        useRoutinePos = zoptions->ReadBool(zOPT_SEC_GAME, "spawnUseDeadNpcRoutinePos", 1);
    if (useRoutinePos)
        return 1;
    return npc->trafoObjToWorld.GetTranslation() == zVEC3(0.0f, 0.0f, 0.0f);
}

int oCSpawnManager::CanDeadNpcBeRemoved(oCNpc* npc)
{
    if (!npc || npc->GetGuild() == 47)
        return 0;
    static int onlyIfEmpty = -2;
    if (onlyIfEmpty == -2)
        onlyIfEmpty = zoptions->ReadBool(zOPT_SEC_GAME, "spawnRemoveNpcOnlyIfEmpty", 0);
    if (onlyIfEmpty)
        return npc->inventory2.IsEmpty(1, 1);
    return !npc->HasMissionItem();
}

int oCSpawnManager::CheckRemoveNpc(oCNpc* npc)
{
    if (!npc || !npc->homeWorld || npc->GetEM(0)->GetCutsceneMode() ||
        npc->IsInGlobalCutscene() || !npc->IsConditionValid() && !npc->IsDead())
        return 0;
    zCVob* cameraVob = oCNpc::player;
    zCAICamera* camera = zCAICamera::GetCurrent();
    if (camera)
        cameraVob = camera->camVob;
    if (cameraVob)
        camPos = cameraVob->trafoObjToWorld.GetTranslation();
    zVEC3 npcPos = npc->trafoObjToWorld.GetTranslation();
    float distance = (npcPos - camPos).LengthApprox();
    if (distance <= SPAWN_REMOVERANGE)
        return 0;
    zVEC3 checkPos;
    if (!npc->IsDead() || UseDeadNpcRoutinePos(npc))
    {
        if (npc->state.hasRoutine)
            checkPos = rtnMan.GetRoutinePos(npc);
        else
            checkPos = npc->state.GetAIStatePosition();
    }
    else
        checkPos = npcPos;
    if ((checkPos - camPos).LengthApprox() <= SPAWN_REMOVERANGE)
        return 0;
    CheckForInvalidDialogCamPos(distance, npc);
    if (npc->showaidebug & 8)
    {
        DeleteNpc(npc);
        return 1;
    }
    npc->Disable();
    npc->showaidebug |= 0x2000;
    if (npc->IsDead())
    {
        SpawnNpc(npc, npcPos, 0.0f);
        return 1;
    }
    if (npc->state.IsAIStateDriven())
        SpawnNpc(npc, npc->state.GetAIStatePosition(), 0.0f);
    return 1;
}

void oCSpawnManager::DeleteNpc(oCNpc* npc)
{
    for (int i = 0; i < spawnList.numInArray; ++i)
    {
        if (spawnList[i]->npc != npc)
            continue;
        RemoveFromList(spawnList[i]);
        --i;
    }
    ogame->GetWorld()->RemoveVob(npc);
}

void oCSpawnManager::DeleteAllSummoned()
{
    while (true)
    {
        oCNpc* summoned = NULL;
        for (int i = 0; i < spawnList.numInArray; ++i)
        {
            oSSpawnNode* node = spawnList[i];
            if (node && node->npc && node->npc->showaidebug & 8)
            {
                summoned = node->npc;
                break;
            }
        }
        if (!summoned)
            return;
        DeleteNpc(summoned);
    }
}

void oCSpawnManager::Archive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    archiver.WriteInt(spawnList.numInArray);
    for (int i = 0; i < spawnList.numInArray; ++i)
    {
        oSSpawnNode* node = spawnList[i];
        archiver.WriteObject(node->npc);
        archiver.WriteVec3(node->spawnPos);
        archiver.WriteFloat(node->timer);
    }
    archiver.WriteBool(spawningEnabled);
    archiver.WriteInt(spawnFlags);
}

void oCSpawnManager::Unarchive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    spawnList.numInArray = 0;
    int count = archiver.ReadInt();
    int failed = count < 0;
    for (int i = 0; i < count; ++i)
    {
        oSSpawnNode* node = new oSSpawnNode;
        node->npc = NULL;
        node->spawnPos = zVEC3(0.0f, 0.0f, 0.0f);
        node->timer = 0.0f;
        node->npc = (oCNpc*)archiver.ReadObject(node->npc);
        if (!node->npc)
        {
            delete node;
            failed = 1;
            break;
        }
        node->spawnPos = archiver.ReadVec3();
        node->timer = archiver.ReadFloat();
        spawnList.InsertEnd(node);
    }
    if (failed)
    {
        spawningEnabled = 1;
        spawnFlags = 0;
        return;
    }
    spawningEnabled = archiver.ReadBool();
    spawnFlags = archiver.ReadInt();
}
