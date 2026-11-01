#include "include/zNetVobControl.h"
#include "include/zNetManager.h"
#include "include/oGame.h"
#include "include/oObjFactory.h"

static int netVobControlState = -1;
static zSTRING netVobControlBroadcast = "broadcast";
static zSTRING netVobControlLocalhost = "127.0.0.1";
static int netVobControlClassDefState = -1;
static int netVobControlNetworkState = -1;
static zSTRING netVobControlDefaultBroadcast = "broadcast";
static zSTRING netVobControlDefaultLocalhost = "127.0.0.1";

zCObject* zCNetVobControl::_CreateNewInstance()
{
    zCNetVobControl* control = new zCNetVobControl;
    zCClassDef::ObjectCreated(control, classDef);
    return control;
}

void zCNetVobControl::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCNetVobControl::_GetClassDef() const
{
    return classDef;
}

int zCNetVobControl::IsLocalChained()
{
    return isLocalChained;
}

void zCNetVobControl::SetLocalChained(int value)
{
    isLocalChained = value;
}

zCNetVobControl::zCNetVobControl()
    : hostVob(NULL), ctrlPlayer(NULL), isLocalChained(0)
{
}

zCNetVobControl::~zCNetVobControl()
{
    hostVob = NULL;
    ctrlPlayer = NULL;
}

void zCNetVobControl::Init(zCVob* vob)
{
    hostVob = vob;
    ctrlPlayer = NULL;
    isLocalChained = 0;
}

int zCNetVobControl::Process()
{
    if (isLocalChained || !hostVob->homeWorld)
        return 0;

    zCPlayerInfo* nearest = ctrlPlayer;
    float nearestDistance = nearest->hostVob->GetDistanceToVob2(*hostVob);

    if (nearestDistance < 40000.0f)
        return 0;

    nearestDistance -= 40000.0f;
    zCPlayerGroup* group = (zCPlayerGroup*)hostVob->homeWorld->GetPlayerGroup();

    for (zCList<zCPlayerInfo>* entry = group->playerList.next; entry; entry = entry->next)
    {
        zCPlayerInfo* player = entry->data;
        if (player == ctrlPlayer)
            continue;

        float distance = player->hostVob->GetDistanceToVob2(*hostVob);
        if (distance <= nearestDistance)
        {
            nearestDistance = distance;
            nearest = player;
        }
    }

    if (nearest != ctrlPlayer)
        return HandOverToPlayer(nearest);
    return 0;
}

int zCNetVobControl::HandOverToPlayer(zCPlayerInfo* player)
{
    SetCtrlPlayer(player);
    unsigned char id = player ? player->GetID() : 0;
    unsigned short messageType = 0;
    zCNetMessage* message = CreateNetMessage(messageType);
    message->Add(&id, 1);
    message->Send();
    return 1;
}

int zCNetVobControl::HandleNetMessage(zCNetMessage* message, const unsigned short& messageType,
    zCWorld* world)
{
    unsigned long vobID = 0;
    message->Get(&vobID, 4);
    zCVob* vob = world->SearchVobByID(vobID, NULL);

    if (!vob)
    {
        zerr->Report(zERR_TYPE_WARN, 0,
            "B: VOBCTRL: Could not find vob for network-message " + zSTRING((int)messageType),
            0, 0, 210, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNetVobControl.cpp", NULL);
        return 0;
    }

    if (messageType == 0)
    {
        unsigned char playerID = 0;
        message->Get(&playerID, 1);
        zCPlayerInfo* player = zCPlayerGroup::allPlayerGroup.GetPlayerByID(playerID);
        vob->GetEM(0)->GetNetVobControl(0)->SetCtrlPlayer(player);
    }

    return 0;
}

zCNetMessage* zCNetVobControl::CreateNetMessage(const unsigned short& messageType)
{
    zCNetMessage* message = znetman->CreateNetMessage(messageType, this);
    message->SetTarget(&zCPlayerGroup::allPlayerGroup);
    unsigned long vobID = hostVob->GetVobID();
    message->Add(&vobID, 4);
    return message;
}

void zCNetVobControl::SetCtrlPlayer(zCPlayerInfo* player)
{
    ctrlPlayer = player;
}

int zCNetVobControl::IsLocalControlled()
{
    return ctrlPlayer ? ctrlPlayer->IsActive() : 0;
}

int zCNetVobControl::IsPlayerVob()
{
    return 0;
}

void zCNetVobControl::CopyCtrlState(zCVob* vob)
{
    if (vob)
        SetCtrlPlayer(vob->GetEM(0)->GetNetVobControl(0)->ctrlPlayer);
}

void zCNetVobControl::Archive(zCArchiver&)
{
    if (!hostVob)
        zerr->Report(zERR_TYPE_WARN, 0,
            "B: VOBCTRL: You must call zCNetVobControl::Init(zCVob* vob) first.", 0, 0, 286,
            "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNetVobControl.cpp", NULL);
}

void zCNetVobControl::Unarchive(zCArchiver&)
{
}
