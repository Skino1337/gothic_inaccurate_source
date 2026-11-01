#ifndef ZWAVEDATA_H
#define ZWAVEDATA_H

#include "zString.h"

struct zTWaveInfo {
    int           format;
    unsigned char reserved[8];
    int           sampleRate;
    unsigned char reserved2[4];
    int           channels;
    unsigned char reserved3[8];
};

class zCWaveData {
public:

    zCWaveData();
    zCWaveData(zSTRING& fileName);
    ~zCWaveData();
    zSTRING GetName();
    void  CacheIn();
    void  CacheOut();
    int   CheckForCorrectFormat(int flags);
    int   Release();
    int   GetSize();

    int size;
    void* memAddr;
    float msLength;
    void* info;
    zSTRING fileName;
    int refCtr;
    int cacheInCount;
    int adpcmCompressed;
};

#endif
