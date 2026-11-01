#ifndef ZTOOLS_H
#define ZTOOLS_H

#include "zFile.h"

class zCTimer {
public:

    zCTimer();

    void ResetTimer();
    void SetFrameTime(float frameTime);
    void SetMaxFPS(unsigned long maxFPS);
    void LimitFPS(int maxFPS);
    void SetMotionFactor(float factor);

    static void          S_SetMaxFPS(unsigned long maxFPS);
    static unsigned long S_GetMaxFPS();
    static void          FrameUpdate();

    float         factorMotion;
    float         frameTimeFloat;
    float         totalTimeFloat;
    float         frameTimeFloatSecs;
    float         totalTimeFloatSecs;
    unsigned long lastTimer;
    unsigned long frameTime;
    unsigned long totalTime;
    unsigned long minFrameTime;
    unsigned long forcedMaxFrameTime;

    static unsigned long s_forcedMaxFrameTime;
};

typedef zCTimer zTIMER;
extern zCTimer ztimer;

class zCChecksum {
public:

    void          BeginChecksum();
    unsigned long EndChecksum();
    void          CalcBufferChecksum(unsigned char* buffer, unsigned long size);
    void          CalcFileChecksum(zFILE* file);
    void          CalcFileChecksum(const zSTRING& fileName);

    static unsigned long GetBufferCRC32(unsigned char* buffer, unsigned long size,
        unsigned long seed);

    unsigned long checksum;
};

class zCChecksum16 {
public:

    void           BeginChecksum();
    unsigned short EndChecksum();
    void           CalcBufferChecksum(unsigned char* buffer, unsigned long size);

    static unsigned short GetBufferCRC16(unsigned char* buffer, unsigned long size,
        unsigned short seed);

    unsigned short checksum;
};

class zCFPUControler {
public:

    zCFPUControler();

    void          SaveCurrentControlWord();
    void          RestoreSavedControlWord();
    unsigned long GetCurrentControlWord();
    void          RestoreDefaultControlWord();
    void          ComputeDefaultControlWord();
    void          SetControlWord(unsigned long value);
    void          SetPrecision_24();
    void          SetPrecision_53();
    void          SetPrecision_64();
    void          PrintStatus();

    unsigned long savedControlWord;
    unsigned long defaultControlWord;
};

#endif
