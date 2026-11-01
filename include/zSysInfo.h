#ifndef ZSYSINFO_H
#define ZSYSINFO_H

#include "zString.h"

class zCSystemInfo {
public:

    enum zTOperatingSystem {
        OS_UNKNOWN,
        OS_WINDOWS_3_1,
        OS_WINDOWS_95,
        OS_WINDOWS_98,
        OS_WINDOWS_98ME,
        OS_WINDOWS_NT,
        OS_WINDOWS_2000,
        OS_OTHER
    };

    enum zTCpuType {
        CPU_UNKNOWN,
        CPU_INTEL_PENTIUM_I,
        CPU_INTEL_PENTIUM_II,
        CPU_INTEL_PENTIUM_III,
        CPU_INTEL_PENTIUM_IV,
        CPU_AMD_K6,
        CPU_AMD_ATHLON,
        CPU_OTHER
    };

    zCSystemInfo();
    virtual ~zCSystemInfo();
    void AnalyseMemory();
    void AnalyseCpu();
    void AnalyseOs();
    void AnalyseGcard();
    zSTRING GetOsName();
    virtual void AnalyseNow();

    zTOperatingSystem osType;
    zSTRING osName;
    zSTRING osServicePack;
    zTCpuType cpuType;
    zSTRING cpuID;
    unsigned long cpuSpeed;
    unsigned long memSize;
    unsigned long memAvailable;
    zSTRING graName;
    unsigned long graMemSize;
};

#endif
