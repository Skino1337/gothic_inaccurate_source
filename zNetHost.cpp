#include "include/zNetHost.h"
#include "include/zNet.h"
#include "include/zObject.h"

zCNet* znet;
unsigned char zCNetHost::iter;
zCNetHost* zCNetHost::localhost;
static const zSTRING netBroadcastHostName = "broadcast";
static const zSTRING netLocalhostAddress = "127.0.0.1";
static int netHostState = -1;
static zSTRING netHostBroadcast = "broadcast";
static zSTRING netHostLocalhost = "127.0.0.1";

zTNetAddress* netGetRemoteAddr(void* handle);
zTNetAddress* netGetHostAddr(char* hostName);
char* netGetLocalHost();

int zCNetHost::InitHost(const zSTRING& host)
{
    valid = 0;
    hostName = host;

    if (host.IsEmpty())
        return InitHostForBroadcast();

    if (!znet || !znet->session)
    {
        zerr->Report(zERR_TYPE_FAULT, 0, "B: (zCNetHost) No session-data available!", 0, 0,
            176, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNetHost.cpp", NULL);
        return 0;
    }

    zSTRING addressName = host;
    if (host == "localhost")
    {
        addressName = netGetLocalHost();
        localhost = this;
    }

    zTNetAddress* address = netGetHostAddr(addressName.ToChar());
    if (!address)
    {
        zerr->Report(zERR_TYPE_WARN, 0, "B: (zCNetHost) Could not retrieve ip for " + host,
            0, 0, 176, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNetHost.cpp", NULL);
        return 0;
    }

    InitHostAddr(address);
    valid = 1;
    return 1;
}

zCNetHost::zCNetHost(void* handle)
    : id(0), valid((int)handle), stream_handle(handle)
{
    InitHostAddr(netGetRemoteAddr(handle));
}

zCNetHost::zCNetHost(const zSTRING& host)
    : id(0), hostName(host), valid(0), stream_handle(NULL)
{
    valid = InitHost(host);
}

zCNetHost::zCNetHost(const zTNetAddress& address)
    : id(0), valid(0), stream_handle(NULL)
{
    InitHostAddr((zTNetAddress*)&address);
    valid = 1;
}

zCNetHost::~zCNetHost()
{
}

void zCNetHost::Init()
{
    id = 0;
    valid = 0;
    stream_handle = NULL;
    hostName.Clear();
    hostStreamAddr = zTNetAddress();
    hostPacketAddr = zTNetAddress();
}

int zCNetHost::InitHostForBroadcast()
{
    valid = 0;
    if (!znet || !znet->session)
    {
        zerr->Report(zERR_TYPE_FAULT, 0, "B: (zCNetHost) No session-data available!", 0, 0,
            176, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNetHost.cpp", NULL);
        return 0;
    }

    zTNetAddress address = {};
    address.Protocol = znet->session->protocol;
    address.INET.IP = 0xffffffff;
    InitHostAddr(&address);
    valid = 1;
    return 1;
}

void zCNetHost::InitHostAddr(zTNetAddress* address)
{
    if (!address)
        return;

    hostStreamAddr = *address;
    hostPacketAddr = *address;

    if (!znet || !znet->session)
    {
        zerr->Report(zERR_TYPE_FAULT, 0, "B: (zCNetHost) No session-data available!", 0, 0,
            200, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNetHost.cpp", NULL);
        return;
    }

    unsigned short protocol = znet->session->protocol;
    if (protocol == 0)
    {
        hostStreamAddr = znet->session->listenStreamAddress;
        hostPacketAddr.IPX.Socket = znet->session->listenPacketAddress.IPX.Socket;
    }
    else if (protocol == 1)
    {
        hostStreamAddr.INET.Port = znet->session->listenStreamAddress.INET.Port;
        hostPacketAddr.INET.Port = znet->session->listenPacketAddress.INET.Port;
        hostStreamAddr.Protocol = 1;
        hostPacketAddr.Protocol = 1;
    }
    else
        zerr->Report(zERR_TYPE_FATAL, 0, "B: (zCNetHost) unknown port-specifier", -1, 0,
            218, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNetHost.cpp", NULL);
}

#include <cstring>

void zCNetHost::Pack(zCBuffer& buffer)
{
    buffer.Write(&id, 1);
    buffer.WriteString(hostName);
}

void zCNetHost::Unpack(zCBuffer& buffer)
{
    buffer.Read(&id, 1);
    hostName = buffer.ReadString();
}

zCNetHost* zCNetHost::CreateFromBuffer(zCBuffer& buffer)
{
    zTNetAddress packetAddress = {};
    zTNetAddress streamAddress = {};
    buffer.Read(&packetAddress, 14);
    buffer.Read(&streamAddress, 14);
    zCNetHost* host = new zCNetHost(streamAddress);
    host->hostPacketAddr = packetAddress;
    host->Unpack(buffer);
    return host;
}

void zCNetHost::PackToBuffer(zCBuffer& buffer)
{
    buffer.Write(&hostPacketAddr, 14);
    buffer.Write(&hostStreamAddr, 14);
    Pack(buffer);
}

int zCNetHost::HasAddress(zTNetAddress& address)
{
    if (hostStreamAddr.Protocol != address.Protocol)
        return 0;

    zTNetAddress hostAddress = hostPacketAddr;
    if (hostAddress.Protocol == 1)
    {
        hostAddress.INET.Port = 0;
        address.INET.Port = 0;
    }
    else if (hostAddress.Protocol == 0)
    {
        hostAddress.IPX.Socket = 0;
        address.IPX.Socket = 0;
    }

    return std::memcmp(&hostAddress, &address, 14) == 0;
}
