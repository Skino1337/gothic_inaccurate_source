#include "include/zException.h"
#include "include/zWin32.h"
#include <windows.h>
#include <cstdio>
#include <cstring>

static int exceptionState = -1;
static int exceptionCallbackState = -1;
int zCExceptionHandler::isActive;
void* zCExceptionHandler::m_previousFilter;
char zCExceptionHandler::m_szLogFileName[260];
zCArray<void(*)()> zCExceptionHandler::releaseCallbacks;
zCArray<void(*)(EXCEPTION_DESCR*)> zCExceptionHandler::infoCallbacks;

zCExceptionHandler::zCExceptionHandler()
{
}

zCExceptionHandler::~zCExceptionHandler()
{
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)m_previousFilter);
    isActive = 0;
}

void zCExceptionHandler::SetIsActive(int active)
{
    if (active)
        m_previousFilter = SetUnhandledExceptionFilter(zCExceptionHandler::UnhandledExceptionFilter);
    else
        SetUnhandledExceptionFilter(NULL);
    isActive = active;
}

int zCExceptionHandler::GetIsActive()
{
    return isActive;
}

void zCExceptionHandler::SetLogFileName(char* fileName)
{
    char* destination = m_szLogFileName;
    do
        *destination++ = *fileName;
    while (*fileName++);
}

static void WriteReportText(void* reportFile, const char* text)
{
    DWORD written;
    WriteFile(reportFile, text, strlen(text), &written, NULL);
}

static void CreateExceptionReport(const char* title, ASSERT_FAIL_INFO* failInfo)
{
    sysKillWindowsKeys(false);
    void* reportFile = CreateFileA(zCExceptionHandler::m_szLogFileName, GENERIC_WRITE,
        FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!reportFile)
        return;
    SetFilePointer(reportFile, 0, NULL, FILE_END);
    WriteReportText(reportFile, title);
    if (failInfo)
    {
        char buffer[1024];
        int length = sprintf(buffer, "%s in file %s, Line: %i\n", failInfo->expression,
            failInfo->file, failInfo->line);
        if (length > 0)
        {
            DWORD written;
            WriteFile(reportFile, buffer, length, &written, NULL);
        }
        if (failInfo->reason)
        {
            WriteReportText(reportFile, "Reason: ");
            WriteReportText(reportFile, failInfo->reason);
            WriteReportText(reportFile, "\n");
        }
    }
    EXCEPTION_DESCR description = {};
    for (int i = 0; i < zCExceptionHandler::infoCallbacks.numInArray; ++i)
        zCExceptionHandler::infoCallbacks[i](&description);
    WriteReportText(reportFile, "//=====================  INFOS =========================\n");
    for (int i = 0; i < description.additionalInfoCount; ++i)
    {
        WriteReportText(reportFile, description.additionalInfo[i]);
        WriteReportText(reportFile, "\n");
    }
    WriteReportText(reportFile, "//===================== MODULES =======================\n");
    zCExceptionHandler::RecordModuleList(reportFile);
    CloseHandle(reportFile);
}

long __stdcall zCExceptionHandler::UnhandledExceptionFilter(_EXCEPTION_POINTERS* exception)
{
    static int handled;
    if (handled)
        return 0;
    handled = 1;
    WalkReleaseCallbacks();
    CreateExceptionReport("//======================UNHANDLED EXCEPTION======================\n", NULL);
    if (m_previousFilter)
        return ((long(__stdcall*)(_EXCEPTION_POINTERS*))m_previousFilter)(exception);
    return 1;
}

void zCExceptionHandler::AddUnhandledExceptionReleaseCallback(void (*callback)())
{
    for (int i = 0; i < releaseCallbacks.numInArray; ++i)
    {
        if (releaseCallbacks[i] == callback)
            return;
    }
    releaseCallbacks.InsertEnd(callback);
}

void zCExceptionHandler::RemoveUnhandledExceptionReleaseCallback(void (*callback)())
{
    releaseCallbacks.RemoveOrder(callback);
}

void zCExceptionHandler::AddUnhandledExceptionInfoCallback(void (*callback)(EXCEPTION_DESCR*))
{
    for (int i = 0; i < infoCallbacks.numInArray; ++i)
    {
        if (infoCallbacks[i] == callback)
            return;
    }
    infoCallbacks.InsertEnd(callback);
}

void zCExceptionHandler::RemoveUnhandledExceptionInfoCallback(void (*callback)(EXCEPTION_DESCR*))
{
    infoCallbacks.RemoveOrder(callback);
}

void zCExceptionHandler::WalkReleaseCallbacks()
{
    for (int i = releaseCallbacks.numInArray - 1; i >= 0; --i)
        releaseCallbacks[i]();
    releaseCallbacks.DeleteList();
}

long __stdcall zCExceptionHandler::AssertExceptionFilter(_EXCEPTION_POINTERS*,
    ASSERT_FAIL_INFO* assertInfo, EXCEPTION_INFO*)
{
    static int handled;
    if (handled)
        return 0;
    handled = 1;
    WalkReleaseCallbacks();
    CreateExceptionReport("//======================ASSERTION FAILED======================\n",
        assertInfo);
    return 1;
}

long __stdcall zCExceptionHandler::BoundsExceptionFilter(_EXCEPTION_POINTERS*,
    ASSERT_FAIL_INFO* assertInfo, EXCEPTION_INFO*)
{
    static int handled;
    if (handled)
        return 0;
    handled = 1;
    WalkReleaseCallbacks();
    CreateExceptionReport("//======================BOUNDS CHECK FAILED======================\n",
        assertInfo);
    return 1;
}

void zCExceptionHandler::RecordModuleList(void* reportFile)
{
    const char header[] = "\r\nModule list: names, addresses, sizes, time stamps and file times:\r\n";
    DWORD written;
    WriteFile(reportFile, header, sizeof(header) - 1, &written, NULL);
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    const unsigned int pageSize = systemInfo.dwPageSize;
    const unsigned int maxPages = 0x40000000 / pageSize;
    unsigned char* address = NULL;
    void* lastAllocationBase = NULL;
    for (unsigned int page = 0; page < maxPages; )
    {
        MEMORY_BASIC_INFORMATION info;
        if (!VirtualQuery(address, &info, sizeof(info)) || !info.RegionSize)
        {
            address += 0x10000;
            continue;
        }
        address += info.RegionSize;
        if (info.State == MEM_COMMIT && info.AllocationBase > lastAllocationBase)
        {
            lastAllocationBase = info.AllocationBase;
            char text[128];
            int length = sprintf(text, "Base: %p Size: %u\r\n", info.AllocationBase,
                (unsigned int)info.RegionSize);
            if (length > 0)
                WriteFile(reportFile, text, length, &written, NULL);
        }
    }
}
