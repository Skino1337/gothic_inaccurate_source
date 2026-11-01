#include "include/zSysInfo.h"
#include "include/zRenderer.h"
#include <windows.h>

zCSystemInfo::zCSystemInfo()
    : osType(OS_UNKNOWN), cpuType(CPU_UNKNOWN), cpuSpeed(555), memSize(129 * 1024 * 1024),
      memAvailable(0), graMemSize(33 * 1024 * 1024)
{
}

zCSystemInfo::~zCSystemInfo()
{
}

void zCSystemInfo::AnalyseMemory()
{
    MEMORYSTATUS status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    memSize = status.dwTotalPhys;
    memAvailable = status.dwAvailPhys;
}

void zCSystemInfo::AnalyseCpu()
{
    HKEY key;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "Hardware\\Description\\System\\CentralProcessor\\0", 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return;
    DWORD type = 0;
    DWORD speed = 0;
    DWORD size = sizeof(speed);
    RegQueryValueExA(key, "~MHz", NULL, &type, (BYTE*)&speed, &size);
    cpuSpeed = speed;
    char vendor[1024];
    vendor[0] = 0;
    size = sizeof(vendor);
    RegQueryValueExA(key, "VendorIdentifier", NULL, &type, (BYTE*)vendor, &size);
    cpuID = vendor;
    RegCloseKey(key);
}

void zCSystemInfo::AnalyseOs()
{
    OSVERSIONINFOEXA version;
    version.dwOSVersionInfoSize = sizeof(version);
    if (!GetVersionExA((OSVERSIONINFOA*)&version))
        return;
    osServicePack = version.szCSDVersion;
    if (version.dwPlatformId == VER_PLATFORM_WIN32s)
        osType = OS_WINDOWS_3_1;
    else if (version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
        if (version.dwMinorVersion < 10)
            osType = OS_WINDOWS_95;
        else if (version.dwMinorVersion >= 90)
            osType = OS_WINDOWS_98ME;
        else
            osType = OS_WINDOWS_98;
    }
    else if (version.dwPlatformId == VER_PLATFORM_WIN32_NT)
        osType = version.dwMajorVersion < 5 ? OS_WINDOWS_NT : OS_WINDOWS_2000;
    else
        osType = OS_OTHER;
    switch (osType)
    {
    case OS_WINDOWS_3_1:
        osName = "Windows 3.1";
        break;
    case OS_WINDOWS_95:
        osName = "Windows 95";
        break;
    case OS_WINDOWS_98:
        osName = "Windows 98";
        break;
    case OS_WINDOWS_98ME:
        osName = "Windows 98 ME";
        break;
    case OS_WINDOWS_NT:
        osName = "Windows NT";
        break;
    case OS_WINDOWS_2000:
        osName = "Windows 2000";
        break;
    default:
        osName = "unknown";
        break;
    }
}

void zCSystemInfo::AnalyseGcard()
{
    graMemSize = zrenderer->GetTotalTextureMem();
}

zSTRING zCSystemInfo::GetOsName()
{
    return osName;
}

void zCSystemInfo::AnalyseNow()
{
    AnalyseCpu();
    AnalyseOs();
    AnalyseMemory();
    AnalyseGcard();
}
