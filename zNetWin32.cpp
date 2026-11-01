#include "include/zNetTypes.h"
#include "include/zObject.h"
#include <winsock2.h>
#include <windows.h>

struct zTNetSocket {
    unsigned long magic;
    zTNetSocket* prev;
    zTNetSocket* next;
    SOCKET socket;
    int state;
    bool closed;
    void* lastPacket;
    void* firstPacket;
    void* lastQueuedPacket;
    int (*callback)(void*);
    zTNetAddress address;
    unsigned long queuedCount;
    unsigned long queuedBytes;
    zTNetAddress remoteAddress;
    int maxInQueuePackets;
};

struct zTNetQueuedPacket {
    zTNetQueuedPacket* next;
    unsigned long readOffset;
    zTNetPacket packet;
};

static zTNetSocket* socketList;
static HANDLE socketMutex;
static int networkInitialized = 1;
static char netErrorString[256];

static int NetSetError()
{
    int error = WSAGetLastError();
    wsprintfA(netErrorString, "Winsock Error %d", error);
    return error;
}

static sockaddr_in NetToSockAddr(const zTNetAddress& address)
{
    sockaddr_in result = {};
    result.sin_family = AF_INET;
    result.sin_port = htons(address.INET.Port);
    result.sin_addr.s_addr = htonl(address.INET.IP);
    return result;
}

static zTNetAddress NetFromSockAddr(const sockaddr_in& address)
{
    zTNetAddress result = {};
    result.Protocol = 1;
    result.INET.Port = ntohs(address.sin_port);
    result.INET.IP = ntohl(address.sin_addr.s_addr);
    return result;
}

static zTNetSocket* NetCreateSocket(SOCKET socket, int state)
{
    zTNetSocket* result = new zTNetSocket;
    *result = zTNetSocket();
    result->magic = 0x4b434f53;
    result->socket = socket;
    result->state = state;
    result->next = socketList;
    if (socketList)
        socketList->prev = result;
    socketList = result;
    return result;
}

void netDumpSocketLL()
{
    if (!socketMutex)
        socketMutex = CreateMutexA(NULL, FALSE, NULL);
    WaitForSingleObject(socketMutex, INFINITE);
    OutputDebugStringA("NET: -- Sockets dump ------------------\n");

    for (zTNetSocket* socket = socketList; socket; socket = socket->next)
    {
        char text[128];
        wsprintfA(text, "NET: Socket %u This %u Next %u\n", (unsigned int)socket->socket,
            (unsigned int)socket, (unsigned int)socket->next);
        OutputDebugStringA(text);
    }

    ReleaseMutex(socketMutex);
}

void netInit()
{
}

void netExit()
{
    if (!networkInitialized)
        return;

    while (socketList)
    {
        zTNetSocket* socket = socketList;
        socketList = socket->next;
        if (socket->socket != INVALID_SOCKET)
            closesocket(socket->socket);
        delete socket;
    }

    WSACleanup();
    if (socketMutex)
        CloseHandle(socketMutex);
    socketMutex = NULL;
    networkInitialized = 0;
}

int netSend(zTNetAddress* address, void* data, int size, void* handle)
{
    zTNetSocket* socket = (zTNetSocket*)handle;
    if (!socket && address && address->INET.IP < 3)
        socket = socketList;
    if (!data || !socket)
        return 1;

    sockaddr_in target = NetToSockAddr(*address);
    int result = sendto(socket->socket, (const char*)data, size, 0, (sockaddr*)&target,
        sizeof(target));

    if (result != size)
    {
        if (result == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
            return 1;
        NetSetError();
        if (result != SOCKET_ERROR)
            zerr->Report(zERR_TYPE_WARN, 0, "B: (netSend) Not all bytes of datagram were send",
                0, 0, 671, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zNet_Win32.cpp", NULL);
        return 0;
    }

    return 1;
}

zTNetPacket* netRecv(void* handle)
{
    zTNetSocket* socket = (zTNetSocket*)handle;
    if (!socket || socket->magic != 0x4b434f53 || socket->state >= 5)
        return NULL;

    WaitForSingleObject(socketMutex, INFINITE);
    if (socket->lastPacket) {
        delete[] (zTNetQueuedPacket*)socket->lastPacket;
        socket->lastPacket = NULL;
    }
    if (!socket->firstPacket) {
        static char data[65536];
        sockaddr_in source;
        int sourceSize = sizeof(source);
        const int size = recvfrom(socket->socket, data, sizeof(data) - sizeof(zTNetQueuedPacket), 0,
            (sockaddr*)&source, &sourceSize);
        if (size == SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
                NetSetError();
            ReleaseMutex(socketMutex);
            return NULL;
        }
        zTNetQueuedPacket* node = (zTNetQueuedPacket*)new char[sizeof(zTNetQueuedPacket) + size];
        node->next = NULL;
        node->readOffset = 0;
        node->packet.From = NetFromSockAddr(source);
        node->packet.timestamp = 0;
        node->packet.size = size;
        node->packet.Data = (unsigned char*)(node + 1);
        memcpy(node->packet.Data, data, size);
        if (socket->lastQueuedPacket)
            ((zTNetQueuedPacket*)socket->lastQueuedPacket)->next = node;
        else
            socket->firstPacket = node;
        socket->lastQueuedPacket = node;
        ++socket->queuedCount;
        socket->queuedBytes += size;
    }
    zTNetQueuedPacket* node = (zTNetQueuedPacket*)socket->firstPacket;
    socket->firstPacket = node->next;
    if (!socket->firstPacket)
        socket->lastQueuedPacket = NULL;
    --socket->queuedCount;
    socket->queuedBytes -= node->packet.size;
    socket->lastPacket = node;
    ReleaseMutex(socketMutex);
    return &node->packet;
}

int netWrite(void* handle, void* data, int size, int flush)
{
    zTNetSocket* socket = (zTNetSocket*)handle;
    if (!socket || socket->magic != 0x4b434f53 || socket->state >= 5 || socket->closed)
        return 0;

    int remaining = size;
    const char* cursor = (const char*)data;

    while (remaining > 0)
    {
        int written = send(socket->socket, cursor, remaining, 0);
        if (written == remaining)
            return 1;
        if (written == SOCKET_ERROR)
        {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
                return flush;
            NetSetError();
            return 0;
        }
        remaining -= written;
        cursor += written;
    }

    return 1;
}

int netRead(void* handle, void* data, int size)
{
    zTNetSocket* socket = (zTNetSocket*)handle;
    if (!socket || socket->magic != 0x4b434f53 || socket->state >= 5)
        return -1;

    WaitForSingleObject(socketMutex, INFINITE);
    int result = 0;
    unsigned char* destination = (unsigned char*)data;
    int remaining = size;
    while (remaining > 0 && socket->firstPacket) {
        zTNetQueuedPacket* node = (zTNetQueuedPacket*)socket->firstPacket;
        int count = node->packet.size - node->readOffset;
        if (count > remaining)
            count = remaining;
        memcpy(destination, node->packet.Data + node->readOffset, count);
        node->readOffset += count;
        destination += count;
        remaining -= count;
        result += count;
        if (node->readOffset == node->packet.size) {
            socket->firstPacket = node->next;
            if (!socket->firstPacket)
                socket->lastQueuedPacket = NULL;
            delete node;
        }
    }
    socket->queuedBytes -= result;
    ReleaseMutex(socketMutex);
    return result;
}

static void* packetHandles[2];

void* netListenPacket(zTNetAddress* address)
{
    SOCKET socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket == INVALID_SOCKET)
        return NULL;

    BOOL broadcast = TRUE;
    setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast));
    sockaddr_in local = NetToSockAddr(*address);

    if (bind(socket, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR)
    {
        NetSetError();
        closesocket(socket);
        return NULL;
    }

    u_long nonBlocking = 1;
    ioctlsocket(socket, FIONBIO, &nonBlocking);
    zTNetSocket* result = NetCreateSocket(socket, 2);
    result->address = *address;
    if (address->Protocol < 2)
        packetHandles[address->Protocol] = result;
    return result;
}

void* netListenStream(zTNetAddress* address, int (*callback)(void*))
{
    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == INVALID_SOCKET)
        return NULL;

    sockaddr_in local = NetToSockAddr(*address);
    if (bind(socket, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR ||
        listen(socket, 3) == SOCKET_ERROR)
    {
        NetSetError();
        closesocket(socket);
        return NULL;
    }

    u_long nonBlocking = 1;
    ioctlsocket(socket, FIONBIO, &nonBlocking);
    int localSize = sizeof(local);
    getsockname(socket, (sockaddr*)&local, &localSize);
    zTNetSocket* result = NetCreateSocket(socket, 1);
    result->address = NetFromSockAddr(local);
    result->callback = callback;
    return result;
}

void* netConnect(zTNetAddress* address)
{
    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == INVALID_SOCKET)
        return NULL;

    sockaddr_in remote = NetToSockAddr(*address);
    if (connect(socket, (sockaddr*)&remote, sizeof(remote)) == SOCKET_ERROR)
    {
        NetSetError();
        closesocket(socket);
        return NULL;
    }

    zTNetSocket* result = NetCreateSocket(socket, 3);
    result->address = *address;
    return result;
}

int netClose(void* handle)
{
    zTNetSocket* socket = (zTNetSocket*)handle;
    if (!socket)
        return 0;
    if (socket->magic != 0x4b434f53 || socket->state >= 5)
        return 0;

    if (socket->prev)
        socket->prev->next = socket->next;
    else
        socketList = socket->next;
    if (socket->next)
        socket->next->prev = socket->prev;

    for (int i = 0; i < 2; ++i)
    {
        if (packetHandles[i] == socket)
            packetHandles[i] = NULL;
    }

    closesocket(socket->socket);
    delete socket->lastPacket;
    delete socket;
    return 1;
}

int netIsHandleClosed(void* handle)
{
    zTNetSocket* socket = (zTNetSocket*)handle;
    if (socket->closed)
        return 1;

    sockaddr_in peer;
    int size = sizeof(peer);
    if (getpeername(socket->socket, (sockaddr*)&peer, &size) == SOCKET_ERROR)
        NetSetError();
    return 0;
}

void* netGetPacketHandle(unsigned long protocol)
{
    return protocol < 2 ? packetHandles[protocol] : NULL;
}

char* netGetHostName(zTNetAddress* address)
{
    if (!address)
        return NULL;

    unsigned long hostAddress = htonl(address->INET.IP);
    hostent* host = gethostbyaddr((const char*)&hostAddress, sizeof(hostAddress), AF_INET);
    if (!host)
    {
        NetSetError();
        return NULL;
    }
    return host->h_name;
}

zTNetAddress* netGetHostAddr(char* hostName)
{
    static zTNetAddress address;
    if (!hostName)
        return NULL;

    unsigned long hostAddress = inet_addr(hostName);
    if (hostAddress == INADDR_NONE)
    {
        hostent* host = gethostbyname(hostName);
        if (!host)
        {
            NetSetError();
            return NULL;
        }
        hostAddress = *(unsigned long*)host->h_addr;
    }

    address.Protocol = 1;
    address.INET.Port = 0;
    address.INET.IP = ntohl(hostAddress);
    return &address;
}

unsigned long netGetServicePort(char* serviceName, int protocol)
{
    servent* service = getservbyname(serviceName, protocol ? "tcp" : "udp");
    if (!service)
    {
        NetSetError();
        return 0;
    }
    return ntohs(service->s_port);
}

char* netAddrToString(zTNetAddress* address, int includePort)
{
    static char text[64];

    if (address->Protocol == 1)
    {
        unsigned long ip = address->INET.IP;
        wsprintfA(text, "%u.%u.%u.%u", ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff,
            (ip >> 24) & 0xff);
        if (includePort)
            wsprintfA(text + lstrlenA(text), " port %u", address->INET.Port);
    }
    else if (address->Protocol == 0)
    {
        wsprintfA(text, "%02X%02X%02X%02X:%02X%02X%02X%02X%02X%02X",
            address->IPX.Net[0], address->IPX.Net[1], address->IPX.Net[2], address->IPX.Net[3],
            address->IPX.Node[0], address->IPX.Node[1], address->IPX.Node[2],
            address->IPX.Node[3], address->IPX.Node[4], address->IPX.Node[5]);
        if (includePort)
            wsprintfA(text + lstrlenA(text), " socket %04Xh", address->IPX.Socket);
    }
    else
        lstrcpyA(text, "UNKNOWN");

    return text;
}

char* netGetLocalHost()
{
    static char localHost[256];
    if (!localHost[0])
        gethostname(localHost, sizeof(localHost));
    return localHost;
}

zTNetAddress* netGetRemoteAddr(void* handle)
{
    return &((zTNetSocket*)handle)->remoteAddress;
}

zTNetAddress* netGetLocalAddr(void* handle)
{
    return &((zTNetSocket*)handle)->address;
}

int netGetProtocolSupported(int protocol)
{
    if (!networkInitialized || protocol < 0 || protocol > 1)
        return 0;
    return (int)packetHandles[protocol];
}

void netSetTimeOut(int protocol, zTNetTimeout* timeout)
{
    static zTNetTimeout timeouts[2];
    if (protocol >= 0 && protocol < 2)
        timeouts[protocol] = *timeout;
}

zTNetBufferInfo* netGetBufferInfo(void* handle)
{
    zTNetSocket* socket = (zTNetSocket*)handle;
    return (zTNetBufferInfo*)&socket->queuedCount;
}

int netSetMaxInQueuePackets(void* handle, int maximum)
{
    zTNetSocket* socket = (zTNetSocket*)handle;
    if (!socket || !maximum)
        return 0;

    WaitForSingleObject(socketMutex, INFINITE);
    while (socket->queuedCount > (unsigned long)maximum)
    {
        zTNetQueuedPacket* node = (zTNetQueuedPacket*)socket->firstPacket;
        socket->firstPacket = node->next;
        --socket->queuedCount;
        socket->queuedBytes -= node->packet.size;
        delete[] (zTNetQueuedPacket*)node;
    }
    if (!socket->firstPacket)
        socket->lastQueuedPacket = NULL;
    socket->maxInQueuePackets = maximum;
    ReleaseMutex(socketMutex);
    return 1;
}

int netGetLastError()
{
    int error = WSAGetLastError();
    WSASetLastError(0);
    return error;
}

char* netGetLastErrorString()
{
    static char result[256];
    lstrcpyA(result, netErrorString);
    netErrorString[0] = 0;
    return result;
}

long __stdcall netWindowProc(HWND window, unsigned short message, unsigned int socketHandle,
    long eventData)
{
    if (message < 0x401 || message > 0x405)
        return DefWindowProcA(window, message, socketHandle, eventData);

    zTNetSocket* socket = socketList;
    while (socket && socket->socket != socketHandle)
        socket = socket->next;

    int event = eventData & 0xffff;
    int error = (eventData >> 16) & 0xffff;
    if (error)
    {
        WSASetLastError(error);
        NetSetError();
        if (socket)
            socket->closed = true;
        return 0;
    }

    if (message == 0x401 && socket)
    {
        if (event == FD_CLOSE)
            socket->closed = true;
        else if ((event == FD_ACCEPT || event == FD_CONNECT) && socket->callback)
            socket->callback(socket);
    }

    return 0;
}

void netAcceptPendingConnections()
{
    if (!socketMutex)
        return;

    WaitForSingleObject(socketMutex, INFINITE);
    zTNetSocket* socket = socketList;

    while (socket)
    {
        if (socket->state == 4)
        {
            socket->state = 3;
            ReleaseMutex(socketMutex);
            if (socket->callback)
                socket->callback(socket);
            WaitForSingleObject(socketMutex, INFINITE);
            socket = socketList;
        }
        else
            socket = socket->next;
    }

    ReleaseMutex(socketMutex);
}
