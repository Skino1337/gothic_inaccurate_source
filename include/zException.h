#ifndef ZEXCEPTION_H
#define ZEXCEPTION_H

struct EXCEPTION_DESCR {
    char* additionalInfo[20];
    int   additionalInfoCount;
};
struct _EXCEPTION_POINTERS;
struct ASSERT_FAIL_INFO {
    const char* title;
    const char* expression;
    const char* file;
    int         line;
    const char* reason;
};
struct EXCEPTION_INFO;

#include "zObject.h"

class zCExceptionHandler {
public:
    zCExceptionHandler();
    ~zCExceptionHandler();
    static void SetIsActive(int active);
    static int GetIsActive();
    static void SetLogFileName(char* fileName);
    static long __stdcall UnhandledExceptionFilter(_EXCEPTION_POINTERS* exception);
    static void AddUnhandledExceptionReleaseCallback(void (*callback)());
    static void RemoveUnhandledExceptionReleaseCallback(void (*callback)());
    static void AddUnhandledExceptionInfoCallback(void (*callback)(EXCEPTION_DESCR*));
    static void RemoveUnhandledExceptionInfoCallback(void (*callback)(EXCEPTION_DESCR*));
    static void WalkReleaseCallbacks();
    static long __stdcall AssertExceptionFilter(_EXCEPTION_POINTERS* exception,
        ASSERT_FAIL_INFO* assertInfo, EXCEPTION_INFO* exceptionInfo);
    static long __stdcall BoundsExceptionFilter(_EXCEPTION_POINTERS* exception,
        ASSERT_FAIL_INFO* assertInfo, EXCEPTION_INFO* exceptionInfo);
    static void RecordModuleList(void* reportFile);

    static int isActive;
    static void* m_previousFilter;
    static char m_szLogFileName[260];
    static zCArray<void(*)()> releaseCallbacks;
    static zCArray<void(*)(EXCEPTION_DESCR*)> infoCallbacks;
};

#endif
