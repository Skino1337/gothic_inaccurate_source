#include "include/zVob.h"
#include "include/zRenderer.h"

int zCEventManager::disableEventManagers = 0;
static int eventManagerState = -1;
static int eventManagerRuntimeState = -1;
static zCClassDef zCEventManagerClassDef("zCEventManager", "zCObject",
    zCEventManager::_CreateNewInstance, 0, 0x40, 0, "");
zCClassDef* zCEventManager::classDef = &zCEventManagerClassDef;
zCArray<zCEventManager*> zCEventManager::activeEM;
int zCEventManager::showMsgCommunication = 0;

zCObject* zCEventManager::_CreateNewInstance()
{
    zCEventManager* manager = new zCEventManager;
    zCClassDef::ObjectCreated(manager, classDef);
    return manager;
}

zCClassDef* zCEventManager::_GetClassDef() const
{
    return classDef;
}

zCNetVobControl* zCEventManager::GetNetVobControl(int create)
{
    return NULL;
}

void zCEventManager::DoFrameActivity()
{
    if (disableEventManagers)
        return;
    for (int i = 0; i < activeEM.numInArray; ++i)
    {
        if (activeEM[i])
            activeEM[i]->ProcessMessageList();
    }
}

zCEventManager::zCEventManager()
    : cleared(0), active(0), cutscene(NULL), hostVob(NULL)
{
    SetCutscene(NULL);
}

zCEventManager::zCEventManager(zCVob* vob)
    : cleared(0), active(0), cutscene(NULL), hostVob(vob)
{
    SetCutscene(NULL);
}

zCEventManager::~zCEventManager()
{
    SetActive(0);
    SetCutscene(NULL);
    KillMessages();
}

void zCEventManager::KillMessages()
{
    for (int i = messageList.numInArray - 1; i >= 0; --i)
    {
        if (messageList[i])
        {
            messageList[i]->Release();
            messageList[i] = NULL;
        }
    }
    delete[] messageList.parray;
    messageList.parray = NULL;
    messageList.numAlloc = 0;
    messageList.numInArray = 0;
}

void zCEventManager::OnMessage(zCEventMessage* message, zCVob* sender)
{
    if (zerr.filter_level >= 8)
    {
        zSTRING hostName = hostVob ? hostVob->GetObjectName() : "NULL";
        zSTRING senderName = sender ? sender->GetObjectName() : "NULL";
        zerr.Message("U: EM: " + hostName + " ---[" + senderName + "]---> "
            + message->_GetClassDef()->className + " " + message->MD_GetSubTypeString(message->subType));
    }
    if (!hostVob->homeWorld && !GetCutscene())
    {
        Delete(message);
        return;
    }
    message->SetCutsceneMode(GetCutsceneMode());
    if (message->IsJob() || !message->IsHighPriority() && messageList.numInArray > 0 &&
        !dynamic_cast<zCEventCore*>(message))
    {
        InsertInList(message);
        SetActive(1);
    }
    else
    {
        SendMessageToHost(message, hostVob, sender);
        Delete(message);
    }
}

void zCEventManager::SendMessageToHost(zCEventMessage* message, zCVob* host, zCVob* sender)
{
    ShowMessageCommunication(sender, host);
    zCEventCore* core = dynamic_cast<zCEventCore*>(message);
    if (!core)
    {
        host->OnMessage(message, sender);
        return;
    }
    switch (core->subType)
    {
    case 0:
        host->OnTrigger(sender, core->vobInstigator);
        break;
    case 1:
        host->OnUntrigger(sender, core->vobInstigator);
        break;
    case 2:
        host->OnTouch(sender);
        break;
    case 3:
        host->OnUntouch(sender);
        break;
    case 4:
        host->OnTouchLevel();
        break;
    case 5:
        host->OnDamage(core->otherVob, sender, core->damage, core->damageType, core->hitLocation);
        break;
    default:
        zerr.Warning("U:EVM: new CoreMessage not yet implemented !");
        break;
    }
}

void zCEventManager::SetShowMessageCommunication(int show)
{
    showMsgCommunication = show;
}

int zCEventManager::GetShowMessageCommunication()
{
    return showMsgCommunication;
}

void zCEventManager::ShowMessageCommunication(zCVob* sender, zCVob* host)
{
    if (!showMsgCommunication || !sender || !host || !zCCamera::activeCam)
        return;
    zVEC3 start = zCCamera::activeCam->camMatrix * sender->trafoObjToWorld.GetTranslation();
    zVEC3 end = zCCamera::activeCam->camMatrix * host->trafoObjToWorld.GetTranslation();
    Alg_ClipAtZ0(start, end);
    if (start.n[2] <= 0.0f || end.n[2] <= 0.0f)
        return;
    int x1 = int(zCCamera::activeCam->vpData.xcenter
        + start.n[0] * zCCamera::activeCam->viewDistanceX / start.n[2]);
    int y1 = int(zCCamera::activeCam->vpData.ycenter
        - start.n[1] * zCCamera::activeCam->viewDistanceY / start.n[2]);
    int x2 = int(zCCamera::activeCam->vpData.xcenter
        + end.n[0] * zCCamera::activeCam->viewDistanceX / end.n[2]);
    int y2 = int(zCCamera::activeCam->vpData.ycenter
        - end.n[1] * zCCamera::activeCam->viewDistanceY / end.n[2]);
    if (screen->ClipLine(x1, y1, x2, y2))
        zrenderer->DrawLineZ(x1, y1, 1.0f / start.n[2], x2, y2, 1.0f / end.n[2], GFX_GREEN);
}

void zCEventManager::ProcessMessageList()
{
    if (!hostVob->homeWorld)
        return;
    if (messageList.numInArray <= 0)
    {
        SetActive(0);
        return;
    }
    for (int i = 0; i < messageList.numInArray;)
    {
        zCEventMessage* message = messageList[i];
        if (!message->IsDeleted())
        {
            ++i;
            continue;
        }
        messageList.RemoveOrderIndex(i);
        message->Release();
    }
    cleared = 0;
    int count = messageList.numInArray;
    for (int i = 0; i < count; ++i)
    {
        zCEventMessage* message = messageList[i];
        if (GetCutsceneMode() == message->GetCutsceneMode() || message->GetIgnoreCutsceneMode())
        {
            SendMessageToHost(message, hostVob, NULL);
            if (cleared)
                break;
            if (!message->IsJob())
                message->Delete();
            if (!message->IsOverlay())
                break;
        }
    }
}

void zCEventManager::SetActive(int enabled)
{
    if (active == enabled)
        return;
    active = enabled;
    if (enabled)
        activeEM.InsertEnd(this);
    else
        activeEM.Remove(this);
}

void zCEventManager::Delete(zCEventMessage* message)
{
    if (!message->IsDeleteable())
        return;
    RemoveFromList(message);
    message->Release();
}

void zCEventManager::RemoveFromList(zCEventMessage* message)
{
    messageList.RemoveOrder(message);
}

void zCEventManager::InsertInList(zCEventMessage* message)
{
    if (!message->IsHighPriority() && !message->IsOverlay())
    {
        messageList.InsertEnd(message);
        return;
    }
    messageList.AllocAbs(messageList.numInArray + 1);
    for (int i = messageList.numInArray; i > 0; --i)
        messageList[i] = messageList[i - 1];
    messageList[0] = message;
    ++messageList.numInArray;
}

void zCEventManager::SetCutscene(zCCutscene* newCutscene)
{
    if (cutscene)
        cutscene->Release();
    cutscene = newCutscene;
    if (cutscene)
        cutscene->AddRef();
    else
    {
        for (int i = 0; i < messageList.numInArray; ++i)
        {
            if (messageList[i]->GetCutsceneMode())
                messageList[i]->SetCutsceneMode(0);
        }
    }
}

zCCutscene* zCEventManager::GetCutscene()
{
    return cutscene;
}

int zCEventManager::GetCutsceneMode()
{
    return cutscene != NULL;
}

void zCEventManager::Clear()
{
    for (int i = messageList.numInArray - 1; i >= 0; --i)
    {
        if (GetCutsceneMode() == messageList[i]->GetCutsceneMode())
            Delete(messageList[i]);
    }
    cleared = 1;
}

int zCEventManager::IsEmpty(int onlyNonOverlay)
{
    if (!onlyNonOverlay)
        return messageList.numInArray <= 0;
    for (int i = 0; i < messageList.numInArray; ++i)
    {
        if (!messageList[i]->IsDeleted() && !messageList[i]->IsOverlay())
            return 0;
    }
    return 1;
}

int zCEventManager::IsRunning(zCEventMessage* message)
{
    for (int i = 0; i < messageList.numInArray; ++i)
    {
        if (messageList[i] == message)
            return 1;
    }
    return 0;
}

int zCEventManager::IsRunning()
{
    return GetActiveMessage() != NULL;
}

zCEventMessage* zCEventManager::GetActiveMessage()
{
    for (int i = 0; i < messageList.numInArray; ++i)
    {
        if (!messageList[i]->IsOverlay())
            return messageList[i];
    }
    return NULL;
}

void zCEventManager::Print_db(const zSTRING& text, zCVob* vob)
{
}

void zCEventManager::OnTouch(zCVob* other)
{
    ShowMessageCommunication(other, hostVob);
    hostVob->OnTouch(other);
}

void zCEventManager::OnUntouch(zCVob* other)
{
    ShowMessageCommunication(other, hostVob);
    hostVob->OnUntouch(other);
}

void zCEventManager::OnTouchLevel()
{
    hostVob->OnTouchLevel();
}

void zCEventManager::OnTrigger(zCVob* other, zCVob* instigator)
{
    ShowMessageCommunication(other, hostVob);
    hostVob->OnTrigger(other, instigator);
}

void zCEventManager::OnUntrigger(zCVob* other, zCVob* instigator)
{
    ShowMessageCommunication(other, hostVob);
    hostVob->OnUntrigger(other, instigator);
}

void zCEventManager::OnDamage(zCVob* other, zCVob* instigator, float damage, int damageType,
    const zVEC3& hitLocation)
{
    ShowMessageCommunication(other, hostVob);
    hostVob->OnDamage(other, instigator, damage, damageType, hitLocation);
}

zCEventMessage* zCEventManager::GetEventMessage(int index)
{
    return messageList[index];
}

void zCEventManager::ShowList(int x, int y)
{
    for (int i = 0; i < messageList.numInArray && y < 8192; ++i)
    {
        zCEventMessage* message = messageList[i];
        zSTRING text = message->MD_GetSubTypeString(message->subType);
        if (message->IsOverlay())
            text.Append(" (Overlay)");
        if (message->GetCutsceneMode())
            text.Append(" (CS)");
        if (message->IsDeleted())
            text.Append(" (Deleted)");
        screen->Print(x, y, text);
        y += screen->FontY();
    }
}

void zCEventManager::Archive(zCArchiver& archiver)
{
    archiver.WriteBool(cleared);
    archiver.WriteBool(active);
    archiver.WriteObject(cutscene);
}

void zCEventManager::Unarchive(zCArchiver& archiver)
{
    cleared = archiver.ReadBool();
    SetActive(archiver.ReadBool());
    SetCutscene((zCCutscene*)archiver.ReadObject(NULL));
}
