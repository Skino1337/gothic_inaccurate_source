#include "include/zNetEventMan.h"
#include "include/zNetMessage.h"
#include "include/zNetManager.h"
#include "include/zWorldInfo.h"
#include "include/oObjFactory.h"
#include "include/oGame.h"

static int netEventState = -1;
static zSTRING netEventBroadcast = "broadcast";
static zSTRING netEventLocalhost = "127.0.0.1";
static zCClassDef zCNetEventManagerClassDef("zCNetEventManager", "zCEventManager",
    zCNetEventManager::_CreateNewInstance, 0, 0x44, 0, "");
zCClassDef* zCNetEventManager::classDef = &zCNetEventManagerClassDef;

zCObject* zCNetEventManager::_CreateNewInstance()
{
    zCNetEventManager* manager = new zCNetEventManager;
    zCClassDef::ObjectCreated(manager, classDef);
    return manager;
}

int zCEventManager::GetNumMessages()
{
    return messageList.numInArray;
}

void zCNetEventManager::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCNetEventManager::_GetClassDef() const
{
    return classDef;
}

int zCNetEventManager::HandleNetMessage(zCNetMessage* message, unsigned short mode, zCWorld* world)
{
    unsigned long hostID = 0;
    unsigned long otherID = 0;
    message->Get(&hostID, 4);
    zCVob* hostVob = world->SearchVobByID(hostID, NULL);
    message->Get(&otherID, 4);
    zCVob* otherVob = otherID ? world->SearchVobByID(otherID, NULL) : NULL;
    zCNetEventManager* hostEM = (zCNetEventManager*)hostVob->GetEM(0);
    zCNetEventManager* otherEM = otherVob ? (zCNetEventManager*)otherVob->GetEM(1) : NULL;
    zCEventMessage* eventMessage = zCEventMessage::CreateFromBuffer(*message->GetBuffer(), hostEM);

    if (mode == 0)
    {
        hostEM->OnMessage(eventMessage, otherVob);
        return 1;
    }

    if (mode == 1 || (mode == 2 && !otherEM))
    {
        if (mode == 2)
        {
            zCNetVobControl* control = hostEM->GetNetVobControl(0);
            if (control)
            {
                zCPlayerInfo* player = zCPlayerGroup::allPlayerGroup.GetPlayerByID(
                    message->GetSenderID());
                control->HandOverToPlayer(player);
            }
        }

        zCNetMessage* netMessage = znetman->CreateNetMessage(mode, hostEM);
        netMessage->CopyFrom(message);
        netMessage->SetTarget((zCPlayerGroup*)world->GetPlayerGroup());
        netMessage->Send();
        hostEM->zCEventManager::OnMessage(eventMessage, otherVob);
        if (mode == 1)
            return 1;
    }

    return 0;
}

zCNetEventManager::zCNetEventManager()
    : netVobCtrl(NULL)
{
}

zCNetEventManager::zCNetEventManager(zCVob* vob)
    : zCEventManager(vob), netVobCtrl(NULL)
{
}

zCNetEventManager::~zCNetEventManager()
{
}

void zCNetEventManager::Init()
{
    netVobCtrl = NULL;
}

void zCNetEventManager::OnMessage(zCEventMessage* message, zCVob* sender)
{
    if (!message->IsNetRelevant())
    {
        zCEventManager::OnMessage(message, sender);
        return;
    }

    if (!sender)
    {
        zerr->Report(zERR_TYPE_WARN, 0,
            "B: EM: otherVob is NULL: " + message->MD_GetSubTypeString(message->subType),
            0, 0, 263, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNetEventMan.cpp", NULL);
        zCEventManager::OnMessage(message, sender);
        return;
    }

    zCPlayerInfo* activePlayer = zCPlayerInfo::GetActivePlayer();
    zCPlayerInfo* levelMaster = activePlayer->worldInfo->GetLevelMaster();
    zCPlayerGroup* playerGroup = activePlayer->worldInfo->GetPlayerGroup();
    zCNetEventManager* otherEM = (zCNetEventManager*)sender->GetEM(1);
    zCNetVobControl* otherControl = otherEM ? otherEM->GetNetVobControl(1) : NULL;
    zCNetVobControl* hostControl = GetNetVobControl(1);

    unsigned short mode;
    zCNetMessage* netMessage;
    if (!otherControl || !otherControl->ctrlPlayer)
    {
        mode = 2;
        netMessage = znetman->CreateNetMessage(mode, this);
        netMessage->SetTarget(levelMaster);
    }
    else if (otherControl->IsLocalChained())
    {
        if (hostControl && hostControl->IsLocalChained())
        {
            mode = 0;
            netMessage = znetman->CreateNetMessage(mode, this);
            netMessage->SetTarget(playerGroup);
        }
        else
        {
            mode = 1;
            netMessage = znetman->CreateNetMessage(mode, this);
            netMessage->SetTarget(hostControl && hostControl->ctrlPlayer
                ? hostControl->ctrlPlayer : levelMaster);
        }
    }
    else
        return;

    unsigned long hostID = hostVob->GetVobID();
    unsigned long senderID = sender->GetVobID();
    netMessage->Add(&hostID, 4);
    netMessage->Add(&senderID, 4);
    message->PackToBuffer(*netMessage->GetBuffer(), this);
    netMessage->Send();

    if (mode == 0)
        zCEventManager::OnMessage(message, sender);
}

void zCNetEventManager::Archive(zCArchiver& archiver)
{
    zCEventManager::Archive(archiver);
    if (archiver.InSaveGame())
    {
        zCObject* object = netVobCtrl;
        archiver.WriteObject(object);
    }
}

void zCNetEventManager::Unarchive(zCArchiver& archiver)
{
    zCEventManager::Unarchive(archiver);
    if (!archiver.InSaveGame())
        return;

    GetNetVobControl(0);
    zCObject* object = NULL;
    zCNetVobControl* control = dynamic_cast<zCNetVobControl*>(archiver.ReadObject(object));
    if (!control && netVobCtrl)
    {
        netVobCtrl->Release();
        netVobCtrl = NULL;
    }
    netVobCtrl = control;
}

zCNetVobControl* zCNetEventManager::GetNetVobControl(int create)
{
    if (!netVobCtrl && !create)
        netVobCtrl = zfactory->CreateNetVobControl(hostVob);
    return netVobCtrl;
}
