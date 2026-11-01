#ifndef ZWIN32_H
#define ZWIN32_H

#include "zString.h"

struct zTWindowsVersionInformation {
    int     version;
    zSTRING versionString;
    int     buildNumber;
    zSTRING buildString;
};

zTWindowsVersionInformation GetWinVersion();
bool sysKillWindowsKeys(bool kill);
bool SetForegroundWindowEx(void* window);
void sysSetWindowMode(int fullScreen, int topmost);
void* sysAlloc(unsigned long size);
void* sysReAlloc(void* memory, unsigned long size);
void sysFree(void* memory);
int  sysHandleExceptions();
void libExit();
int  MasterExceptionHandler();
long __stdcall InputDlgProc(void* window, unsigned int message, unsigned long wParam, long lParam);
zSTRING CheckPassword();
int  initializeSystemMFC();
void winDoInstanceCheck();
void winCreateMenu();
void HandleFocusLoose();
void sysHardExit(char* format, ...);
void winResizeMainWindow();
void winCalcSizes();
void winSuspendThreads();
void sysExit();
unsigned long sysGetTime();
void sysProcessIdle();
void sysEvent();
void sysIdle(int milliseconds);
void winWaitMessage(unsigned short message, unsigned long wParam, unsigned long wMask,
    unsigned long lParam, unsigned long lMask, int allowNonIconic);
void TRACE(char* format, ...);
void cfgFetchTopic(char* topic);
void cfgSetBinary(char* topic, char* name, void* data, int size);
int  cfgGetBinary(char* topic, char* name, void* data, int* size);
void sysParseCommandLine(char* commandLine);
void sysSetFocus();
long winDialog(unsigned short resource,
    long __stdcall (*dialogProc)(void*, unsigned int, unsigned long, long));
long winDialogParam(unsigned short resource,
    long __stdcall (*dialogProc)(void*, unsigned int, unsigned long, long), unsigned long parameter);
long winDialogParamFinal(unsigned short resource,
    long __stdcall (*dialogProc)(void*, unsigned int, unsigned long, long), unsigned long parameter);

struct EXCEPTION_INFO;
void ExcSpyInfo(EXCEPTION_INFO* info);
long __stdcall ExceptionDlgProc(void* window, unsigned int message,
    unsigned long wParam, long lParam);
long __stdcall ASSERTDlgProc(void* window, unsigned int message,
    unsigned long wParam, long lParam);

struct ASSERT_FAIL_INFO;
void BoundsSpyInfo(ASSERT_FAIL_INFO* info, EXCEPTION_INFO* exceptionInfo);
void AssertSpyInfo(ASSERT_FAIL_INFO* info, EXCEPTION_INFO* exceptionInfo);
void BOUNDS_FAIL(char* name, int value, int minimum, int maximum,
    char* file, int line, const zSTRING& reason);
void ASSERT_FAIL(char* expression, char* file, int line, const zSTRING& reason);
int  sysGotDebugger();
void inCreateMouseMessage(unsigned long message, unsigned long coordinates);

class zCOptionEntry;
int zOpt_Sound_ChangeFXVol(zCOptionEntry& entry);
int zOpt_Sound_ChangeMusicVol(zCOptionEntry& entry);
int zOpt_Video_ChangeVidOptions(zCOptionEntry& entry);
int zOpt_ToggleJoy(zCOptionEntry& entry);

extern void* hWndApp;

#endif
