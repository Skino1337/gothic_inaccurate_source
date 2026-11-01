#ifndef ZNETTYPES_H
#define ZNETTYPES_H

struct zTNetAddress {
    unsigned short Protocol;
    union {
        struct {
            unsigned short Port;
            unsigned long IP;
        } INET;
        struct {
            unsigned char Net[4];
            unsigned char Node[6];
            unsigned short Socket;
        } IPX;
    };
};

struct zTNetPacket {
    zTNetAddress From;
    unsigned long timestamp;
    unsigned long size;
    unsigned char* Data;
};

struct zTNetMessageHeader {
    unsigned long id;
    unsigned char senderID;
    unsigned long datasize;
    unsigned char timestamp;
    unsigned char flags;
};

struct zTNetBufferInfo {
    int inPackets;
    int inBytes;
};

struct zTNetTimeout {
    unsigned long getHostAddress;
    unsigned long getHostName;
    unsigned long getService;
    unsigned long Connect;
};

enum zTNetStreamReadMode {
    zNET_READ_BLOCK,
    zNET_READ_STREAM
};

#endif
