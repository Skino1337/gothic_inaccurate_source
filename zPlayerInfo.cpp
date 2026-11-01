#include "include/oObjFactory.h"
#include "include/oGame.h"
#include "include/zNetHost.h"
#include "include/zVob.h"

static int playerInfoState = -1;
static zSTRING playerInfoBroadcast = "broadcast";
static zSTRING playerInfoLocalhost = "127.0.0.1";
zCArray<int> zCPlayerInfo::free_ids;
unsigned char zCPlayerInfo::next_id = 1;
zCPlayerInfo* zCPlayerInfo::activePlayer;

unsigned long sysGetTime();

zCPlayerInfo::zCPlayerInfo()
    : hostVob(NULL), netHost(NULL), scriptBuffer(NULL), worldInfo(NULL), pingTime(0),
      pingTimeStart(0), pingLost(0), id(0), ready(0), deactivated(0), reserved(0)
{
}

zCPlayerInfo::~zCPlayerInfo()
{
    if (this == activePlayer)
        activePlayer = NULL;

    int index;
    for (zCPlayerGroup* group = zCPlayerGroup::GetFirstGroup(index); group;
        group = zCPlayerGroup::GetNextGroup(index))
        group->RemPlayer(this);

    worldInfo = NULL;
    if (id > 0)
    {
        if (id != next_id - 1)
            free_ids.Insert(id);
        else
            next_id = id;
    }

    if (hostVob)
    {
        hostVob->Release();
        hostVob = NULL;
    }
}

zCPlayerInfo* zCPlayerInfo::CreateDistinctPlayer()
{
    if (free_ids.numInArray <= 0)
        id = next_id++;
    else
    {
        id = free_ids[0];
        free_ids[0] = free_ids[free_ids.numInArray - 1];
        --free_ids.numInArray;
    }

    if (netHost)
        netHost->id = id;
    return this;
}

void zCPlayerInfo::Init(const zSTRING& playerName, void* handle)
{
    name = playerName;
    if (handle)
    {
        delete netHost;
        netHost = new zCNetHost(handle);
    }
}

void zCPlayerInfo::Init(const zSTRING& playerName, const zSTRING& host)
{
    name = playerName;
    if (!host.IsEmpty())
    {
        delete netHost;
        netHost = new zCNetHost(host);
    }
}

void zCPlayerInfo::Pack(zCBuffer& buffer)
{
    buffer.WriteBool(netHost != NULL, 1);
    if (netHost)
        netHost->PackToBuffer(buffer);
    buffer.WriteString(name);
    buffer.WriteByte(id, 8);
    buffer.WriteBool(ready, 1);
}

void zCPlayerInfo::Unpack(zCBuffer& buffer)
{
    delete netHost;
    netHost = NULL;
    if (buffer.ReadBool(1))
        netHost = zCNetHost::CreateFromBuffer(buffer);
    name = buffer.ReadString();
    id = buffer.ReadByte(8);
    ready = buffer.ReadBool(1);
}

zCPlayerInfo* zCPlayerInfo::CreateFromBuffer(zCBuffer& buffer)
{
    zCPlayerInfo* player = zfactory->CreatePlayerInfo();
    player->Unpack(buffer);
    return player;
}

zCPlayerInfo* zCPlayerInfo::GetActivePlayer()
{
    return activePlayer;
}

void zCPlayerInfo::SetActive()
{
    activePlayer = this;
}

int zCPlayerInfo::IsActive()
{
    return this == activePlayer;
}

void zCPlayerInfo::Reset()
{
}

void zCPlayerInfo::StorePlayerVobAsScript()
{
    delete scriptBuffer;
    scriptBuffer = NULL;
    zCArchiver* archiver = zarcFactory->CreateArchiverWrite(NULL, 1, 1, 0);
    zCObject* object = hostVob;
    archiver->WriteObject(object);
    zCBuffer* source = archiver->GetBuffer();
    scriptBuffer = new zCBuffer(source->GetSize() + 1);
    scriptBuffer->Write(source->GetBuffer(), source->GetSize());
    archiver->Close();
    archiver->Release();
    scriptBuffer->SetPosBegin();
}

zCVob* zCPlayerInfo::CreatePlayerVobFromScript()
{
    if (hostVob)
    {
        hostVob->Release();
        hostVob = NULL;
    }

    scriptBuffer->SetPosBegin();
    zCArchiver* archiver = zarcFactory->CreateArchiverRead(scriptBuffer, 0);
    zCObject* object = NULL;
    hostVob = dynamic_cast<zCVob*>(archiver->ReadObject(object));
    archiver->Close();
    archiver->Release();
    return hostVob;
}

zCBuffer* zCPlayerInfo::GetPlayerVobScript()
{
    scriptBuffer->SetPosBegin();
    return scriptBuffer;
}

void zCPlayerInfo::SetPlayerVobScript(zCBuffer* buffer)
{
    delete scriptBuffer;
    scriptBuffer = new zCBuffer(buffer->GetSize() + 1);
    scriptBuffer->Write(buffer->GetBuffer(), buffer->GetSize());
}

void zCPlayerInfo::SetPlayerVob(zCVob* vob)
{
    if (hostVob)
        hostVob->Release();
    hostVob = vob;
    if (hostVob)
        hostVob->AddRef();
}

void zCPlayerInfo::Deactivate(int value)
{
    deactivated = value;
}

int zCPlayerInfo::IsDeactivated() const
{
    return deactivated;
}

void zCPlayerInfo::SetReady(int value)
{
    ready = value;
}

int zCPlayerInfo::IsReady() const
{
    return ready;
}

unsigned long zCPlayerInfo::SetPingStart()
{
    if (pingTimeStart)
        ++pingLost;
    pingTimeStart = sysGetTime();
    return 0;
}

unsigned long zCPlayerInfo::SetPingEnd()
{
    pingTime = 0;
    if (pingTimeStart)
        pingTime = sysGetTime() - pingTimeStart;
    pingTimeStart = 0;
    pingLost = 0;
    return pingTime;
}
