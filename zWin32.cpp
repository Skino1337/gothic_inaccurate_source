#include "include/zWin32.h"
#include "include/oGame.h"
#include "include/zException.h"
#include "include/zRenderer.h"
#include "include/zVideo_Win32.h"
#include "include/zMusic.h"
#include "include/zOption.h"
#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

void* hWndApp;

zTWindowsVersionInformation GetWinVersion()
{
    zTWindowsVersionInformation result;
    result.version = 9;
    result.buildNumber = 0;

    OSVERSIONINFOEXA versionInfo;
    ZeroMemory(&versionInfo, sizeof(versionInfo));
    versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
    if (GetVersionExA(reinterpret_cast<OSVERSIONINFOA*>(&versionInfo))) {
        if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
            if (versionInfo.dwMinorVersion == 0)
                result.version = 0;
            else
                result.version = versionInfo.dwMinorVersion == 90 ? 2 : 1;
        }
        else if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            if (versionInfo.dwMajorVersion == 3 || versionInfo.dwMajorVersion == 4)
                result.version = 7;
            else if (versionInfo.dwMajorVersion == 5)
                result.version = 8;
        }

        result.buildNumber = versionInfo.dwBuildNumber;
        result.versionString = zSTRING(versionInfo.dwMajorVersion) + "."
            + zSTRING(versionInfo.dwMinorVersion);
        result.buildString = zSTRING(versionInfo.dwBuildNumber) + " " + versionInfo.szCSDVersion;
    }

    return result;
}

bool sysKillWindowsKeys(bool kill)
{
    if (zoptions && !zoptions->ReadBool("ENGINE", "zKillSysKeys", 0))
        return true;
    return SystemParametersInfo(SPI_SETFASTTASKSWITCH, kill, NULL, 0) != 0;
}

bool SetForegroundWindowEx(void* window)
{
    HWND windowHandle = static_cast<HWND>(window);
    if (!windowHandle)
        return false;

    HWND foreground = GetForegroundWindow();
    if (windowHandle == foreground)
        return true;

    DWORD foregroundProcess;
    const DWORD currentThread = GetCurrentThreadId();
    const DWORD foregroundThread = GetWindowThreadProcessId(foreground, &foregroundProcess);
    if (GetCurrentProcessId() == foregroundProcess)
        return SetForegroundWindow(windowHandle) != 0;

    bool result = false;
    if (foregroundThread && currentThread != foregroundThread
            && AttachThreadInput(currentThread, foregroundThread, TRUE)) {
        result = SetFocus(windowHandle) != NULL;
        AttachThreadInput(currentThread, foregroundThread, FALSE);
    }
    else if (currentThread == foregroundThread) {
        result = SetFocus(windowHandle) != NULL;
    }

    if (!result)
        result = SetForegroundWindow(windowHandle) != 0;
    return result;
}

void sysSetWindowMode(int fullScreen, int topmost)
{
    static int windowMode;
    static RECT windowedRect;
    HWND windowHandle = static_cast<HWND>(hWndApp);

    if (fullScreen) {
        if (windowMode == 1)
            GetWindowRect(windowHandle, &windowedRect);
        SetMenu(windowHandle, NULL);
        SetWindowLong(windowHandle, GWL_STYLE, WS_POPUP);
        SetWindowLong(windowHandle, GWL_EXSTYLE, WS_EX_TOPMOST);
        windowMode = 2;
        if (topmost) {
            SetWindowPos(windowHandle, HWND_TOPMOST, 0, 0,
                GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_DRAWFRAME);
            ShowWindow(windowHandle, SW_SHOW);
        }
        return;
    }

    if (windowMode == 2) {
        ShowWindow(windowHandle, SW_HIDE);
        MoveWindow(windowHandle, windowedRect.left, windowedRect.top,
            windowedRect.right - windowedRect.left, windowedRect.bottom - windowedRect.top, TRUE);
    }

    SetWindowLong(windowHandle, GWL_STYLE, WS_OVERLAPPEDWINDOW);
    SetWindowLong(windowHandle, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    SetWindowPos(windowHandle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    windowMode = 1;
    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindowEx(windowHandle);
    UpdateWindow(windowHandle);
    InvalidateRect(GetDesktopWindow(), NULL, TRUE);
}

void* sysAlloc(unsigned long size)
{
    return std::calloc(1, size);
}

void* sysReAlloc(void* memory, unsigned long size)
{
    if (!memory)
        return std::calloc(1, size);
    return std::realloc(memory, size);
}

void sysFree(void* memory)
{
    std::free(memory);
}

static int sysExceptionsEnabled = 1;
static int sysExitCalled;
static int sysStopProcessingMessages;

void netExit();

int sysHandleExceptions()
{
    return sysExceptionsEnabled;
}

void libExit()
{
    if (sysExitCalled)
        return;

    sysExitCalled = 1;
    sysStopProcessingMessages = 1;
    netExit();
    if (hWndApp)
        DestroyWindow(static_cast<HWND>(hWndApp));
    sysKillWindowsKeys(false);
    if (sysExceptionsEnabled)
        zCExceptionHandler::WalkReleaseCallbacks();
}

int MasterExceptionHandler()
{
    zCExceptionHandler::WalkReleaseCallbacks();
    return 0;
}

struct zTInputDialogData {
    char*       buffer;
    int         bufferSize;
    const char* text;
};

static zTInputDialogData* inputDialogData;

long __stdcall InputDlgProc(void* window, unsigned int message, unsigned long wParam, long lParam)
{
    HWND dialog = static_cast<HWND>(window);
    if (message == WM_INITDIALOG) {
        inputDialogData = reinterpret_cast<zTInputDialogData*>(wParam);
        SendDlgItemMessage(dialog, 0x437, WM_SETTEXT, 0,
            reinterpret_cast<LPARAM>(inputDialogData->text));
        SetFocus(GetDlgItem(dialog, 0x438));
    }
    else if (message == WM_COMMAND) {
        if (wParam == IDOK) {
            inputDialogData->bufferSize = SendDlgItemMessage(dialog, 0x438, WM_GETTEXT,
                inputDialogData->bufferSize, reinterpret_cast<LPARAM>(inputDialogData->buffer));
            EndDialog(dialog, inputDialogData->bufferSize);
            return 1;
        }
        if (wParam == IDCANCEL) {
            EndDialog(dialog, 0);
            return 1;
        }
    }

    return 0;
}

zSTRING CheckPassword()
{
    char buffer[80] = {};
    zTInputDialogData data = { buffer, sizeof(buffer), "Please enter password" };
    DialogBoxParam(GetModuleHandle(NULL), "InputDlgBox", GetFocus(), InputDlgProc,
        reinterpret_cast<LPARAM>(&data));
    return zSTRING(buffer);
}

HINSTANCE hInstApp;
int __stdcall HandledWinMain(HINSTANCE instance, HINSTANCE previous, char* commandLine, int show);

int __stdcall WinMain(HINSTANCE instance, HINSTANCE previous, char* commandLine, int show)
{
    InitCommonControls();
    hInstApp = instance;
    zSTRING parameters(commandLine);
    zerr->Init(parameters);
    parameters.Upper();
    sysExceptionsEnabled = parameters.Search(0, "ZNOEXHND", 1) == -1;
    zCExceptionHandler::SetIsActive(sysExceptionsEnabled);
    HandledWinMain(instance, previous, commandLine, show);
    return 0;
}

static HANDLE winInstanceLock;
static HANDLE winMainThread;
static unsigned long winMainThreadID;
static HICON hIconApp;
static HDC dcScreen;
static LARGE_INTEGER sysPerformanceFrequency;
static unsigned long sysStartTime;
static unsigned long sysStart70Time;
static int sysUsePerformanceCounter;

void netInit();
int MainProg();

void winDoInstanceCheck()
{
    winInstanceLock = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY,
        0, 0x20, "Lock_Gothic II");
    if (winInstanceLock && GetLastError() == ERROR_ALREADY_EXISTS)
        ExitProcess(1);
}

static void InitializeSystemTimers()
{
    QueryPerformanceFrequency(&sysPerformanceFrequency);
    if (sysPerformanceFrequency.QuadPart >= 1000
            && sysPerformanceFrequency.HighPart == 0) {
        sysUsePerformanceCounter = 1;
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        sysStartTime = unsigned long(counter.QuadPart
            / (sysPerformanceFrequency.QuadPart / 1000));
        sysStart70Time = unsigned long(counter.QuadPart
            / (sysPerformanceFrequency.QuadPart / 70));
    }
    else {
        sysUsePerformanceCounter = 0;
        sysStartTime = timeGetTime();
        sysStart70Time = sysStartTime / 14;
    }
}

static int RegisterSystemWindowClasses()
{
    WNDCLASS windowClass = {};
    hIconApp = LoadIcon(hInstApp, MAKEINTRESOURCE(0xa3));
    windowClass.style = CS_DBLCLKS;
    windowClass.lpfnWndProc = DefWindowProc;
    windowClass.hInstance = hInstApp;
    windowClass.hIcon = hIconApp;
    windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    windowClass.lpszClassName = "DDWndClass";
    if (!RegisterClass(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return 0;

    windowClass = {};
    windowClass.lpfnWndProc = DefWindowProc;
    windowClass.hInstance = hInstApp;
    windowClass.lpszClassName = "DDWinSock";
    return RegisterClass(&windowClass) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

int initializeSystemMFC()
{
    InitCommonControls();
    winDoInstanceCheck();
    winMainThreadID = GetCurrentThreadId();
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
        &winMainThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
    InitializeSystemTimers();
    CreateMutex(NULL, FALSE, NULL);

    char moduleFileName[MAX_PATH];
    GetModuleFileName(NULL, moduleFileName, sizeof(moduleFileName));
    dcScreen = CreateCompatibleDC(NULL);
    if (!RegisterSystemWindowClasses())
        return 0;

    netInit();
    return 0;
}

int __stdcall HandledWinMain(HINSTANCE instance, HINSTANCE previous, char* commandLine, int show)
{
    hInstApp = instance;
    if (initializeSystemMFC())
        return 0;

    const int extraWidth = GetSystemMetrics(SM_CXFRAME) * 2 + 5;
    int extraHeight = GetSystemMetrics(SM_CYFRAME) * 2
        + GetSystemMetrics(SM_CYCAPTION) + 5;
    if (!vidHideMenu)
        extraHeight += GetSystemMetrics(SM_CYMENU);

    RECT workArea;
    int x = 0;
    int y = 0;
    if (SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0)) {
        x = workArea.left;
        y = workArea.top;
    }

    hWndApp = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, "DDWndClass", "Gothic II",
        WS_OVERLAPPEDWINDOW, x, y, 800 + extraWidth, 600 + extraHeight,
        NULL, NULL, hInstApp, NULL);
    if (hWndApp) {
        MainProg();
        libExit();
    }
    return 0;
}

void winCreateMenu()
{
}

void HandleFocusLoose()
{
    if (!zrenderer || zrenderer->Vid_GetScreenMode() != zRND_SCRMODE_FULLSCREEN)
        return;

    zerr->Warning("C: lost focus in fullscreen, setting to windowed mode");
    zrenderer->SetSurfaceLost(1);
    if (ogame)
        ogame->Pause(0);
    zrenderer->Vid_SetScreenMode(zRND_SCRMODE_WINDOWED);
    SetWindowPos(static_cast<HWND>(hWndApp), NULL, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

static int sysExiting;
static int vidFreeStretch;
static int winExtraWidth;
static int winExtraHeight;

void sysHardExit(char* format, ...)
{
    if (sysExiting)
        return;

    sysExiting = 1;
    char message[1024];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    zerr->Fatal(message);
    vidCloseDisplay(0);
    ShowWindow(static_cast<HWND>(hWndApp), SW_HIDE);
    libExit();
    if (winMainThread)
        CloseHandle(winMainThread);
    if (winInstanceLock)
        CloseHandle(winInstanceLock);
    ExitProcess(0);
}

void winCalcSizes()
{
    winExtraWidth = GetSystemMetrics(SM_CXFRAME) * 2 + 5;
    winExtraHeight = GetSystemMetrics(SM_CYFRAME) * 2
        + GetSystemMetrics(SM_CYCAPTION) + 5;
    if (!vidHideMenu)
        winExtraHeight += GetSystemMetrics(SM_CYMENU);
}

void winResizeMainWindow()
{
    HWND windowHandle = static_cast<HWND>(hWndApp);
    if (vidFreeStretch == 1 || !windowHandle || IsZoomed(windowHandle) || IsIconic(windowHandle))
        return;

    RECT rect;
    GetWindowRect(windowHandle, &rect);
    MoveWindow(windowHandle, rect.left, rect.top,
        VideoW + winExtraWidth, VideoH + winExtraHeight, TRUE);
    RedrawWindow(windowHandle, NULL, NULL, RDW_INVALIDATE);
}

unsigned long sysGetTime()
{
    if (!sysUsePerformanceCounter)
        return timeGetTime() - sysStartTime;

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return unsigned long(counter.QuadPart
        / (sysPerformanceFrequency.QuadPart / 1000)) - sysStartTime;
}

void winSuspendThreads()
{
    if (GetCurrentThreadId() != winMainThreadID && winMainThread)
        SuspendThread(winMainThread);
}

void sysExit()
{
    libExit();
    if (winMainThread)
        CloseHandle(winMainThread);
    if (winInstanceLock)
        CloseHandle(winInstanceLock);
    ExitProcess(0);
}

static void (*sysIdleCallback)(unsigned long);
static unsigned long sysIdleInterval;
static unsigned long sysIdleLastTime;
static int sysIdleSkipFrames;
static int sysFocusWasLost;

void netAcceptPendingConnections();

void sysProcessIdle()
{
    netAcceptPendingConnections();
    if (!sysIdleCallback)
        return;

    const unsigned long currentTime = sysGetTime();
    while (sysIdleCallback && currentTime > sysIdleLastTime + sysIdleInterval) {
        sysIdleCallback(sysIdleLastTime);
        if (sysIdleSkipFrames) {
            sysIdleLastTime += (currentTime - sysIdleLastTime)
                / sysIdleInterval * sysIdleInterval;
            break;
        }
        sysIdleLastTime += sysIdleInterval;
    }
}

void sysEvent()
{
    static int eventActive;
    if (eventActive)
        return;

    eventActive = 1;
    sysProcessIdle();

    MSG message;
    while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    if (GetForegroundWindow() != hWndApp) {
        if (zrenderer && zrenderer->Vid_GetScreenMode() == zRND_SCRMODE_FULLSCREEN) {
            sysFocusWasLost = 1;
            HandleFocusLoose();
        }
    }
    else if (sysFocusWasLost) {
        SetForegroundWindowEx(hWndApp);
        SetFocus(static_cast<HWND>(hWndApp));
        ShowWindow(static_cast<HWND>(hWndApp), SW_RESTORE);
        if (ogame)
            ogame->Unpause();
        sysFocusWasLost = 0;
    }

    eventActive = 0;
}

void sysIdle(int milliseconds)
{
    Sleep(milliseconds);
}

void winWaitMessage(unsigned short message, unsigned long wParam, unsigned long wMask,
    unsigned long lParam, unsigned long lMask, int allowNonIconic)
{
    while (true) {
        sysProcessIdle();
        if (allowNonIconic && !IsIconic(static_cast<HWND>(hWndApp)))
            break;

        MSG currentMessage;
        if (!PeekMessage(&currentMessage, static_cast<HWND>(hWndApp), 0, 0, PM_REMOVE)) {
            Sleep(2);
            continue;
        }

        if (currentMessage.message == message
                && (currentMessage.wParam & wMask) == wParam
                && (currentMessage.lParam & lMask) == lParam)
            break;
    }
}

void TRACE(char* format, ...)
{
    char message[4096];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    zerr->Message(message);
}

void cfgFetchTopic(char* topic)
{
}

void cfgSetBinary(char* topic, char* name, void* data, int size)
{
}

int cfgGetBinary(char* topic, char* name, void* data, int* size)
{
    return 1;
}

extern zSTRING sysCommandLine;
static zSTRING sysLogFileName;
static int usrSafeMode;
static int netEnable = 1;
static int sysSafeLog;
static int sysNoRegistry = 1;
static int sysDirectDraw = 1;
static int sysDirectDrawWindow = 1;

void sysParseCommandLine(char* commandLine)
{
    sysCommandLine = commandLine;
    zSTRING parameters(commandLine);
    parameters.Lower();

    const int logPosition = parameters.Search("-log=", 0);
    if (logPosition >= 0) {
        zSTRING logValue = parameters.Copied(logPosition + 5, parameters.Length());
        const int space = logValue.Search(" ", 0);
        if (space >= 0)
            logValue = logValue.Copied(0, space);
        sysLogFileName = logValue;
        sysCommandLine.Delete(logPosition, logValue.Length() + 5);
    }

    if (parameters.Search("-safe", 0) >= 0)
        usrSafeMode = 1;
    if (parameters.Search("-windowed", 0) >= 0)
        VideoModeCur.hwModeType = 1;
    if (parameters.Search("-fullscreen", 0) >= 0)
        VideoModeCur.hwModeType = 2;
    if (parameters.Search("-safelog", 0) >= 0)
        sysSafeLog = 1;
    if (parameters.Search("-nonetwork", 0) >= 0)
        netEnable = 0;
    if (parameters.Search("-noddrawwin", 0) >= 0)
        sysDirectDrawWindow = 0;
    if (parameters.Search("-noddraw", 0) >= 0)
        sysDirectDraw = 0;
    if (parameters.Search("-noreg", 0) >= 0)
        sysNoRegistry = 0;

    if (usrSafeMode) {
        TRACE("WIN: Safe mode startup");
        VideoModeCur.hwModeType = 4;
        netEnable = 0;
        sysDirectDraw = 0;
        sysDirectDrawWindow = 0;
    }
}

void sysSetFocus()
{
    SetWindowPos(static_cast<HWND>(hWndApp), NULL, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    SetForegroundWindowEx(hWndApp);
    SetFocus(static_cast<HWND>(hWndApp));
    ShowWindow(static_cast<HWND>(hWndApp), SW_RESTORE);
    ShowWindow(static_cast<HWND>(hWndApp), SW_SHOWNORMAL);
}

long winDialog(unsigned short resource,
    long __stdcall (*dialogProc)(void*, unsigned int, unsigned long, long))
{
    return winDialogParam(resource, dialogProc, 0);
}

long winDialogParam(unsigned short resource,
    long __stdcall (*dialogProc)(void*, unsigned int, unsigned long, long), unsigned long parameter)
{
    const unsigned long startTime = sysGetTime();
    const int minimize = VideoHwModeCur.hwModeType == 5;
    if (minimize)
        vidMinimize(0);

    const long result = DialogBoxParam(hInstApp, MAKEINTRESOURCE(resource),
        static_cast<HWND>(hWndApp), reinterpret_cast<DLGPROC>(dialogProc), parameter);
    if (minimize)
        vidRestore();
    sysTimeGap += sysGetTime() - startTime;
    return result;
}

long winDialogParamFinal(unsigned short resource,
    long __stdcall (*dialogProc)(void*, unsigned int, unsigned long, long), unsigned long parameter)
{
    const unsigned long startTime = sysGetTime();
    vidCloseDisplay(0);
    ShowWindow(static_cast<HWND>(hWndApp), SW_HIDE);
    const long result = DialogBoxParam(hInstApp, MAKEINTRESOURCE(resource), NULL,
        reinterpret_cast<DLGPROC>(dialogProc), parameter);
    sysTimeGap += sysGetTime() - startTime;
    return result;
}

void ExcSpyInfo(EXCEPTION_INFO* info)
{
    zSTRING report("======================================= UNHANDLED EXCEPTION OCCURED "
        "=======================================\r\n");
    report += "======================================= CRASH INFOS: "
        "=======================================\r\n";
    report += "Startup Options: " + sysCommandLine + "\r\n";
    report += "======================================= CALLSTACK: "
        "=======================================\r\n";
    zerr->Warning(report);

    const int size = report.Length() + 1;
    HGLOBAL globalMemory = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!globalMemory)
        return;

    char* destination = static_cast<char*>(GlobalLock(globalMemory));
    if (destination) {
        lstrcpy(destination, report.ToChar());
        GlobalUnlock(globalMemory);
        if (OpenClipboard(static_cast<HWND>(hWndApp))) {
            EmptyClipboard();
            SetClipboardData(CF_TEXT, globalMemory);
            CloseClipboard();
        }
    }
}

static CUSTOMCAPTION exceptionCaption;
static CUSTOMCAPTION assertCaption;

long __stdcall ExceptionDlgProc(void* window, unsigned int message,
    unsigned long wParam, long lParam)
{
    HWND dialog = static_cast<HWND>(window);
    if (message == WM_INITDIALOG) {
        ccRenderCaption(dialog, &exceptionCaption);
        SetWindowText(dialog, "Application Error");
        ExcSpyInfo(reinterpret_cast<EXCEPTION_INFO*>(lParam));
        return 1;
    }
    if (message == WM_COMMAND) {
        if (wParam == IDOK || wParam == IDCANCEL || wParam == 0x41f)
            EndDialog(dialog, 0);
        return 1;
    }
    if (message == WM_NCPAINT) {
        ccDrawCaption(dialog, &exceptionCaption, GetActiveWindow() == dialog,
            reinterpret_cast<void*>(wParam));
        DefWindowProc(dialog, message, wParam, lParam);
        return 1;
    }
    return 0;
}

long __stdcall ASSERTDlgProc(void* window, unsigned int message,
    unsigned long wParam, long lParam)
{
    HWND dialog = static_cast<HWND>(window);
    if (message == WM_INITDIALOG) {
        ccRenderCaption(dialog, &assertCaption);
        SetForegroundWindowEx(dialog);
        return 1;
    }
    if (message == WM_COMMAND) {
        if (wParam == IDOK || wParam == IDCANCEL || wParam == 0x41f)
            EndDialog(dialog, wParam == IDOK);
        return 1;
    }
    if (message == WM_NCPAINT) {
        ccDrawCaption(dialog, &assertCaption, GetActiveWindow() == dialog,
            reinterpret_cast<void*>(wParam));
        DefWindowProc(dialog, message, wParam, lParam);
        return 1;
    }
    return 0;
}

void BoundsSpyInfo(ASSERT_FAIL_INFO* info, EXCEPTION_INFO* exceptionInfo)
{
    zSTRING message("Boundary check failed: ");
    message += info->expression;
    message += " in file ";
    message += info->file;
    message += ", Line: " + zSTRING(info->line);
    if (info->reason)
        message += ", Reason: " + zSTRING(info->reason);
    zerr->Warning(message);
    zerr->Warning("Call stack: ");
}

void AssertSpyInfo(ASSERT_FAIL_INFO* info, EXCEPTION_INFO* exceptionInfo)
{
    zSTRING message("ASSERTION failed: ");
    message += info->expression;
    message += " in file ";
    message += info->file;
    message += ", Line: " + zSTRING(info->line);
    if (info->reason)
        message += ", Reason: " + zSTRING(info->reason);
    zerr->Warning(message);
    zerr->Warning("Call stack: ");
}

int sysGotDebugger()
{
    return IsDebuggerPresent();
}

void BOUNDS_FAIL(char* name, int value, int minimum, int maximum,
    char* file, int line, const zSTRING& reason)
{
    if (sysExiting) {
        TRACE("WIN: Warning! Bounds failed; Variable %s (%d) not in bounds [%d, %d]",
            name, value, minimum, maximum);
        return;
    }

    sysExiting = 1;
    if (sysGotDebugger())
        DebugBreak();
    TRACE("WIN: Bounds Check Failed!");
    TRACE("WIN: Bounds Check : Variable %s (%d) not in bounds [%d, %d]",
        name, value, minimum, maximum);
    TRACE("WIN: File         : %s (line %u)", file, line);
    TRACE("WIN: Reason       : %s", reason.ToChar());

    ASSERT_FAIL_INFO info = { "Bounds Violation", name, file, line, reason.ToChar() };
    BoundsSpyInfo(&info, NULL);
    DialogBoxParam(hInstApp, MAKEINTRESOURCE(0x96), NULL, ASSERTDlgProc,
        reinterpret_cast<LPARAM>(&info));
    sysExit();
}

void ASSERT_FAIL(char* expression, char* file, int line, const zSTRING& reason)
{
    if (sysExiting) {
        TRACE("WIN: Warning! ASSERT(%s) Failed in %s (line %u)", expression, file, line);
        return;
    }

    sysExiting = 1;
    TRACE("WIN: Assertion Failed!");
    TRACE("WIN: Assertion : ASSERT(%s)", expression);
    TRACE("WIN: File      : %s (line %u)", file, line);
    TRACE("WIN: Reason    : %s", reason.ToChar());

    ASSERT_FAIL_INFO info = { "Assertion Failed", expression, file, line, reason.ToChar() };
    AssertSpyInfo(&info, NULL);
    DialogBoxParam(hInstApp, MAKEINTRESOURCE(0x94), NULL, ASSERTDlgProc,
        reinterpret_cast<LPARAM>(&info));
    sysExit();
}

struct zTMouseMessage {
    unsigned char  message;
    unsigned char  buttons;
    unsigned short x;
    unsigned short y;
};

static zTMouseMessage mouseMessages[64];
static int mouseReadPosition;
static int mouseWritePosition;
static int mouseLastButtons;
static int mouseLastX = -1;
static int mouseLastY = -1;

void inCreateMouseMessage(unsigned long message, unsigned long coordinates)
{
    if (vidIsMinimized())
        return;

    int x = coordinates & 0xffff;
    int y = coordinates >> 16;
    const int buttons = (message & 3) | ((message >> 2) & 4);
    if (x != 0xffff && VideoHwModeCur.hwModeType == 4) {
        RECT clientRect;
        GetClientRect(static_cast<HWND>(hWndApp), &clientRect);
        x = VideoW * x / (clientRect.right + 1);
        y = VideoH * y / (clientRect.bottom + 1);
    }

    int event = 0;
    if (x != mouseLastX || y != mouseLastY) {
        event = 1;
        mouseLastX = x;
        mouseLastY = y;
    }

    const int changed = mouseLastButtons ^ buttons;
    mouseLastButtons = buttons;
    for (int i = 0; i < 3; i++) {
        if (changed & (1 << i)) {
            event = 3 + i * 2 - ((buttons >> i) & 1);
            break;
        }
    }
    if (!event)
        return;

    if (mouseWritePosition > mouseReadPosition
            && mouseMessages[(mouseWritePosition - 1) & 63].message == 1)
        mouseWritePosition--;

    zTMouseMessage& mouseMessage = mouseMessages[mouseWritePosition & 63];
    mouseMessage.message = event;
    mouseMessage.buttons = buttons;
    mouseMessage.x = x;
    mouseMessage.y = y;
    mouseWritePosition++;
}

static float ClampOptionValue(float value)
{
    if (value < 0.0f)
        return 0.0f;
    if (value > 1.0f)
        return 1.0f;
    return value;
}

int zOpt_Sound_ChangeFXVol(zCOptionEntry& entry)
{
    zsound->SetMasterVolume(ClampOptionValue(entry.varValue.ToFloat()));
    return 1;
}

int zOpt_Sound_ChangeMusicVol(zCOptionEntry& entry)
{
    zmusic->SetVolume(ClampOptionValue(entry.varValue.ToFloat()));
    return 1;
}

int zOpt_Video_ChangeVidOptions(zCOptionEntry& entry)
{
    const float brightness = ClampOptionValue(zoptions->ReadReal("VIDEO", "zVidBrightness", 0.5f));
    const float contrast = ClampOptionValue(zoptions->ReadReal("VIDEO", "zVidContrast", 0.5f));
    const float gamma = ClampOptionValue(zoptions->ReadReal("VIDEO", "zVidGamma", 0.5f));
    zrenderer->Vid_SetGammaCorrection(brightness, contrast, gamma);
    return 1;
}

int zOpt_ToggleJoy(zCOptionEntry& entry)
{
    return 1;
}
