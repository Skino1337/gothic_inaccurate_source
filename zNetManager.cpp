#include "include/zNetManager.h"
#include "include/zNetEventMan.h"
#include "include/zNet.h"
#include "include/oGame.h"
#include "include/oObjFactory.h"

static int netManagerState = -1;
static zSTRING netManagerBroadcast = "broadcast";
static zSTRING netManagerLocalhost = "127.0.0.1";
static zCClassDef zCNetManagerClassDef("zCNetManager", "zCObject",
    zCNetManager::_CreateNewInstance, 0, 0x54, 0, "");
zCClassDef* zCNetManager::classDef = &zCNetManagerClassDef;
zCNetManager* znetman;
extern zCNet* znet;

zCObject* zCNetManager::_CreateNewInstance()
{
    zCNetManager* manager = new zCNetManager;
    zCClassDef::ObjectCreated(manager, classDef);
    return manager;
}

void zCNetManager::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCNetManager::_GetClassDef() const
{
    return classDef;
}

int zCNetManager::Temp_HandleNetMessage(zCNetMessage*& message, zCWorld* world)
{
    unsigned long id = message->GetID();
    unsigned short messageType = id - 1;
    zCClassDef* messageClass = zCClassDef::classDefList->GetSafe((id >> 16) - 1);
    int result = HandleNetMessage(message, messageType, messageClass, world);
    delete message;
    message = NULL;
    return result;
}

zCNetManager::zCNetManager()
    : timer_count_ping(0), timer_count_worldtime(0)
{
    timer.ResetTimer();
}

zCNetManager::~zCNetManager()
{
}

void zCNetManager::Process()
{
    timer_count_ping += timer.frameTime;
    timer_count_worldtime += timer.frameTime;

    if (timer_count_ping > 5000)
    {
        SendPing();
        timer_count_ping = 0;
    }

    if (timer_count_worldtime > 6000)
        timer_count_worldtime = 0;

    ProcessMessages();
}

void zCNetManager::ProcessMessages()
{
    znet->Handle();
    zCNetMessage* message = NULL;
    znet->MsgGet(message, 0);

    if (message)
    {
        unsigned long id = message->GetID();
        unsigned short messageType = id - 1;
        zCClassDef* messageClass = zCClassDef::classDefList->GetSafe((id >> 16) - 1);
        HandleNetMessage(message, messageType, messageClass, NULL);
        delete message;
    }
}

int zCNetManager::HandleNetMessage(zCNetMessage* message, const unsigned short& messageType,
    zCClassDef* messageClass, zCWorld* world)
{
    if (messageClass == classDef)
    {
        zCPlayerInfo* player = zCPlayerGroup::allPlayerGroup.GetPlayerByID(message->GetSenderID());
        if (messageType == 0)
            SendPingReply(player);
        else if (messageType == 1)
            player->SetPingEnd();
    }
    else if (messageClass == zCNetEventManager::classDef)
        return zCNetEventManager::HandleNetMessage(message, messageType, world);
    else if (messageClass == zCNetVobControl::classDef)
        return zCNetVobControl::HandleNetMessage(message, messageType, world);

    return 0;
}

zCNetMessage* zCNetManager::CreateNetMessage(const unsigned short& messageType, zCObject* object)
{
    unsigned long id = ((object->_GetClassDef()->GetClassID() + 1) << 16) + messageType + 1;
    return new zCNetMessage(id, 512);
}

void zCNetManager::SendPing()
{
    if (!zCPlayerInfo::GetActivePlayer())
        return;

    for (zCList<zCPlayerInfo>* entry = zCPlayerGroup::allPlayerGroup.playerList.next;
        entry; entry = entry->next)
    {
        unsigned short messageType = 0;
        zCNetMessage* message = CreateNetMessage(messageType, this);
        message->SetTarget(entry->data);
        message->Send();
        entry->data->SetPingStart();
    }
}

void zCNetManager::SendPingReply(zCPlayerInfo* player)
{
    if (!player)
        return;

    unsigned short messageType = 1;
    zCNetMessage* message = CreateNetMessage(messageType, this);
    message->SetTarget(player);
    message->Send();
}

void zCNetManager::SendWorldTime()
{
}
