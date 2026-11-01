#include "include/oMission.h"

static int missionState = -1;
static float missionFarDistance = 250000.0f * 0.0166666675f;
static float missionNearDistance = missionFarDistance * 0.0166666675f;
static float missionFarPlaneDistance = 250000.0f * 24.0f;
static zSTRING missionPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING missionPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING missionPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING missionPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING missionPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING missionPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING missionPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING missionPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING missionPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING missionPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
oCMissionManager misMan;
#include "include/oGame.h"

oCMission::oCMission(int missionInstance)
    : duration(0), important(0), offerConditions(-1), offerFunc(-1), successConditions(-1),
      successFunc(-1), failureConditions(-1), failureFunc(-1), obsoleteConditions(-1),
      obsoleteFunc(-1), runningFunc(-1), instance(missionInstance), npc(0), player(0), curID(0),
      available(1), status(0), deleted(0)
{
    parser.CreateInstance(instance, this);
    statusList.AllocAbs(1);
}

oCMission::~oCMission()
{
    for (int i = 0; i < statusList.numInArray; ++i)
        delete statusList.parray[i];
    statusList.numInArray = 0;
    npc = 0;
    player = 0;
    status = 0;
}

oCMissionManager::oCMissionManager()
{
    missionList.Compare = SortMissions;
}

oCMissionManager::~oCMissionManager()
{
    ClearMissions();
}

void oCMissionManager::ClearMissions()
{
    for (int i = 0; i < missionList.numInArray; ++i)
        delete missionList.array[i];
    missionList.numInArray = 0;
}

int oCMissionManager::SortMissions(const void* first, const void* second)
{
    const oCMission* firstMission = *(const oCMission* const*)first;
    const oCMission* secondMission = *(const oCMission* const*)second;
    return firstMission->instance >= secondMission->instance ? 1 : -1;
}

oCMission* oCMissionManager::GetMission(int instance)
{
    int first = 0;
    int last = missionList.numInArray - 1;
    while (first <= last) {
        int middle = (first + last) / 2;
        int current = missionList.array[middle]->instance;
        if (current == instance)
            return missionList.array[middle];
        if (current < instance)
            first = middle + 1;
        else
            last = middle - 1;
    }
    return 0;
}

void oCMissionManager::CreateAllMissions()
{
    missionList.DeleteList();
    int classIndex = parser.GetIndex("C_MISSION");
    if (classIndex < 0)
        return;
    int instanceIndex = classIndex;
    while (true) {
        int instance = parser.GetInstance(classIndex, ++instanceIndex);
        if (instance < 0)
            break;
        oCMission* mission = new oCMission(instance);
        mission->npc = 0;
        mission->player = oCNpc::player;
        if (mission->player)
            mission->curID = 1;
        mission->status = mission->GetStatusInfo();
        if (mission->status && mission->status->status == 3)
            mission->status->startTime = ogame->GetWorldTimer()->GetFullTime();
        missionList.InsertSort(mission);
    }
}

oCMission* oCMissionManager::OfferThisMission(oCNpc* npc, oCNpc* player, int instance)
{
    oCMission* mission = GetMission(instance);
    if (!mission) {
        mission = new oCMission(instance);
        missionList.InsertSort(mission);
    }
    mission->npc = npc;
    mission->player = player;
    if (player)
        mission->curID = 1;
    mission->status = mission->GetStatusInfo();
    if (mission->status) {
        if (mission->status->status == 3 || mission->status->status == 1)
            mission->CheckMission();
        else if (mission->available) {
            parser.SetInstance("SELF", npc);
            parser.SetInstance("OTHER", player);
            if (parser.CallFunc(mission->offerConditions)) {
                parser.SetInstance("SELF", npc);
                parser.SetInstance("OTHER", player);
                parser.CallFunc(mission->offerFunc);
            }
        }
    }
    return 0;
}

oCMission* oCMissionManager::GetAvailableMission(oCNpc* npc, oCNpc* player, int instance)
{
    for (int i = 0; i < 5; ++i) {
        int currentInstance = instance + i;
        oCMission* mission = GetMission(currentInstance);
        if (!mission) {
            mission = new oCMission(currentInstance);
            missionList.InsertSort(mission);
        }
        mission->npc = npc;
        mission->player = player;
        if (player)
            mission->curID = 1;
        mission->status = mission->GetStatusInfo();
        if (!mission->status)
            continue;
        if (mission->status->status == 3 || mission->status->status == 1)
            return mission;
        if (!mission->status->status && mission->available) {
            parser.SetInstance("SELF", npc);
            parser.SetInstance("OTHER", player);
            if (parser.CallFunc(mission->offerConditions))
                return mission;
        }
    }
    return 0;
}

int oCMissionManager::GetStatus(oCNpc* player, int instance)
{
    oCMission* mission = GetMission(instance);
    if (!mission)
        return -1;
    mission->npc = 0;
    mission->player = player;
    if (player)
        mission->curID = 1;
    mission->status = mission->GetStatusInfo();
    return mission->status ? mission->status->status : -1;
}

void oCMissionManager::SetStatus(oCNpc* player, int instance, int newStatus)
{
    oCMission* mission = GetMission(instance);
    if (!mission) {
        zerr->Message("U: MIS: Mission SetStatus failed ");
        return;
    }
    mission->npc = 0;
    mission->player = player;
    if (player)
        mission->curID = 1;
    mission->status = mission->GetStatusInfo();
    if (mission->status) {
        mission->status->status = newStatus;
        if (newStatus == 3)
            mission->status->startTime = ogame->GetWorldTimer()->GetFullTime();
    }
}

int oCMissionManager::CancelMission(oCNpc* player, int instance)
{
    oCMission* mission = GetMission(instance);
    if (!mission)
        return 0;
    mission->npc = 0;
    mission->player = player;
    if (player)
        mission->curID = 1;
    mission->status = mission->GetStatusInfo();
    if (!mission->status || (mission->status->status != 3 && mission->status->status != 1))
        return 0;
    mission->SetStatus(6);
    return 1;
}

void oCMissionManager::CreateMissionInfoList(oCNpc* player, zCArray<oCMission*>& missions)
{
    for (int i = 0; i < missionList.numInArray; ++i) {
        oCMission* mission = missionList.array[i];
        mission->npc = 0;
        mission->player = player;
        if (player)
            mission->curID = 1;
        mission->status = mission->GetStatusInfo();
        if (mission->status && mission->status->status)
            missions.InsertEnd(mission);
    }
}

int oCMissionManager::TestNextMission()
{
    return 1;
}

void oCMissionManager::Archive(zCArchiver& archiver)
{
    archiver.WriteInt(missionList.numInArray);
    for (int i = 0; i < missionList.numInArray; ++i)
        missionList.array[i]->Archive(archiver);
}

void oCMissionManager::Unarchive(zCArchiver& archiver)
{
    int count = archiver.ReadInt();
    missionList.numInArray = 0;
    for (int i = 0; i < count; ++i) {
        oCMission* mission = new oCMission(0);
        mission->Unarchive(archiver);
        missionList.InsertEnd(mission);
    }
}

void oCMission::AddEntry(const zSTRING& entry)
{
    description.Append("<hr>" + entry);
}

void oCMission::SetCurrentUser(oCNpc* newNpc, oCNpc* newPlayer)
{
    npc = newNpc;
    player = newPlayer;
    if (newPlayer)
        curID = 1;
    status = GetStatusInfo();
}

void oCMission::GetCurrentUser(oCNpc*& currentNpc, oCNpc*& currentPlayer)
{
    currentNpc = npc;
    currentPlayer = player;
}

oCMission::TMisStatus* oCMission::GetStatusInfo()
{
    for (int i = 0; i < statusList.numInArray; ++i) {
        if (statusList.parray[i]->vobID == curID)
            return statusList.parray[i];
    }
    TMisStatus* info = new TMisStatus;
    info->vobID = curID;
    info->status = 0;
    info->startTime = 0.0f;
    statusList.InsertEnd(info);
    return info;
}

float oCMission::GetStartTime() const
{
    return status ? status->startTime : -1.0f;
}

void oCMission::SetStartTime(float time)
{
    if (status)
        status->startTime = time;
}

int oCMission::CheckMission()
{
    if (SuccessConditions()) {
        Success();
        return 1;
    }
    if (FailureConditions()) {
        Failure();
        return 1;
    }
    if (ObsoleteConditions()) {
        Obsolete();
        return 1;
    }
    if (!status || status->status != 3)
        return 0;
    Running();
    return 1;
}

int oCMission::IsAvailable() const
{
    return available;
}

int oCMission::OfferConditions()
{
    parser.SetInstance("SELF", npc);
    parser.SetInstance("OTHER", player);
    return parser.CallFunc(offerConditions);
}

void oCMission::Offer()
{
    parser.SetInstance("SELF", npc);
    parser.SetInstance("OTHER", player);
    parser.CallFunc(offerFunc);
}

int oCMission::SuccessConditions()
{
    parser.SetInstance("SELF", npc);
    parser.SetInstance("OTHER", player);
    return parser.CallFunc(successConditions);
}

void oCMission::Success()
{
    parser.SetInstance("SELF", npc);
    parser.SetInstance("OTHER", player);
    parser.CallFunc(successFunc);
}

int oCMission::FailureConditions()
{
    parser.SetInstance("SELF", npc);
    parser.SetInstance("OTHER", player);
    return parser.CallFunc(failureConditions);
}

void oCMission::Failure()
{
    parser.SetInstance("SELF", npc);
    parser.SetInstance("OTHER", player);
    parser.CallFunc(failureFunc);
}

int oCMission::ObsoleteConditions()
{
    parser.SetInstance("SELF", npc);
    parser.SetInstance("OTHER", player);
    return parser.CallFunc(obsoleteConditions);
}

void oCMission::Obsolete()
{
    parser.SetInstance("SELF", npc);
    parser.SetInstance("OTHER", player);
    parser.CallFunc(obsoleteFunc);
}

void oCMission::Running()
{
    parser.SetInstance("SELF", npc);
    parser.SetInstance("OTHER", player);
    parser.CallFunc(runningFunc);
}

int oCMission::GetStatus() const
{
    return status ? status->status : -1;
}

void oCMission::SetStatus(int newStatus)
{
    if (!status) {
        zerr->Message("U:MISSION:Used Mis_SetState without active Mission.");
        return;
    }
    status->status = newStatus;
    if (newStatus == 3)
        status->startTime = ogame->GetWorldTimer()->GetFullTime();
}

int oCMission::OnTime() const
{
    if (duration > 0) {
        float start = status ? status->startTime : -1.0f;
        if (ogame->GetWorldTimer()->GetFullTime() < start + duration * 250000.0f)
            return 0;
    }
    return 1;
}

void oCMission::Archive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    int first = 0;
    int second = 0;
    archiver.WriteString(parser.GetSymbolInfo(instance, first, second));
    archiver.WriteInt(curID);
    archiver.WriteBool(available);
    archiver.WriteInt(statusList.numInArray);
    int current = -1;
    for (int i = 0; i < statusList.numInArray; ++i) {
        TMisStatus* info = statusList.parray[i];
        archiver.WriteInt(info->vobID);
        archiver.WriteInt(info->status);
        archiver.WriteFloat(info->startTime);
        if (info == status)
            current = i;
    }
    archiver.WriteInt(current);
}

oCMission* oCMission::Unarchive(zCArchiver& archiver)
{
    npc = 0;
    player = 0;
    if (!archiver.InSaveGame())
        return this;
    statusList.numInArray = 0;
    name = archiver.ReadString();
    instance = parser.GetIndex(name);
    parser.CreateInstance(instance, this);
    curID = archiver.ReadInt();
    available = archiver.ReadBool();
    int count = archiver.ReadInt();
    for (int i = 0; i < count; ++i) {
        TMisStatus* info = new TMisStatus;
        info->vobID = archiver.ReadInt();
        info->status = archiver.ReadInt();
        info->startTime = archiver.ReadFloat();
        statusList.InsertEnd(info);
    }
    int current = archiver.ReadInt();
    status = current < 0 ? 0 : statusList.parray[current];
    return this;
}
