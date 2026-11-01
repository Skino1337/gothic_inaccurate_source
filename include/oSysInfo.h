#ifndef OSYSINFO_H
#define OSYSINFO_H

#include "zString.h"

class zCSystemInfo {
public:

    zCSystemInfo() {}
    virtual ~zCSystemInfo() {}
    virtual void AnalyseNow();

    unsigned long cpuSpeed;
    unsigned long memSize;
    unsigned long graMemSize;
};

class oCSystemInfo : public zCSystemInfo {
public:

    oCSystemInfo();
    virtual ~oCSystemInfo();
    virtual void AnalyseNow();
    virtual void ScoreNow();
    virtual void ResetSettings(float score);
    float GetGraphicsBonus();
    float GetSoundBonus();
    zSTRING GetScoreDescr(float score);

    int scored;
    float cpuLow;
    float cpuHigh;
    float memLow;
    float memHigh;
    float graMemLow;
    float graMemHigh;
    float scoreCpu;
    float scoreMem;
    float scoreGraMem;
    float scoreGra;
    float scoreSnd;
    float score;
};

#endif
