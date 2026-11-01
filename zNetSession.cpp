#include "include/zNetSession.h"

static int netProtocolState = -1;
static int netSessionState = -1;
static zSTRING netSessionBroadcast = "broadcast";
static zSTRING netSessionLocalhost = "127.0.0.1";

int netGetProtocolSupported(int protocol);

zCNetSession::zCNetSession(unsigned short newProtocol)
    : protocol(newProtocol), valid(0)
{
    listenPacketAddress = zTNetAddress();
    listenStreamAddress = zTNetAddress();

    if (protocol == 1)
    {
        listenPacketAddress.INET.Port = 7207;
        listenStreamAddress.INET.Port = 7208;
    }

    listenPacketAddress.Protocol = protocol;
    listenStreamAddress.Protocol = protocol;
    valid = netGetProtocolSupported(protocol);
}

void zCNetSession::Reset()
{
    valid = netGetProtocolSupported(protocol);
}

zCNetSession::~zCNetSession()
{
}
