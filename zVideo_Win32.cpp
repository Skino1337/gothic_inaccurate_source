#include "include/zVideo_Win32.h"
#include "include/zMoving.h"
#include <windows.h>
#include <cstdlib>
#include <cstring>

unsigned char* Video;
long VideoW;
long VideoH;
long VideoPitch;
unsigned long VideoBPP = 16;
unsigned long VideoRGBSize = 0x565;
unsigned long VideoRPos = 11;
unsigned long VideoRSize = 5;
unsigned long VideoRMask = 0xf800;
unsigned long VideoGPos = 5;
unsigned long VideoGSize = 6;
unsigned long VideoGMask = 0x07e0;
unsigned long VideoBPos;
unsigned long VideoBSize = 5;
unsigned long VideoBMask = 0x001f;
VIDMODE VideoHwModeCur;
VIDMODE VideoModeCur;
VIDMODE* vidpModes;
int vidnModes;
int vidOnlyFullScreen;
int vidHideMenu;
int vidDisableMenuKeys;
int vidDisableAltEnter;
int vidAllowMinimizeWindow = 1;
unsigned long vidShowBitDepths = 0x38;
unsigned long vidEnableBitDepths = 0x38;
unsigned long vidWindowStretch;
int (*vidModeFilter)(void*);
int (*winSetVideoMode)(void*);

static unsigned long defaultVidSizes[] = {
    640, 480, 800, 600, 1024, 768, 1280, 1024, 0, 0
};
unsigned long* vidDefSize = defaultVidSizes;
static int vidModeCapacity;
static unsigned long vidCapabilities;
static unsigned char vidPalette[256][4];
static unsigned long vidFrameTimes[256];
static int vidFrameTimeIndex;

static int DefaultVidModeFilter(void*)
{
    return 1;
}

static int CompareVidModes(const void* first, const void* second)
{
    const VIDMODE* left = static_cast<const VIDMODE*>(first);
    const VIDMODE* right = static_cast<const VIDMODE*>(second);
    if (left->si.BPP != right->si.BPP)
        return left->si.BPP - right->si.BPP;
    if (left->si.Width != right->si.Width)
        return left->si.Width < right->si.Width ? -1 : 1;
    if (left->si.Height != right->si.Height)
        return left->si.Height < right->si.Height ? -1 : 1;
    return 0;
}

void vidRefreshModeList()
{
    int (*filter)(void*) = vidModeFilter ? vidModeFilter : DefaultVidModeFilter;
    std::free(vidpModes);
    vidpModes = NULL;
    vidnModes = 0;
    vidModeCapacity = 0;

    if (!vidOnlyFullScreen) {
        VIDMODE mode;
        std::memset(&mode, 0, sizeof(mode));
        mode.hwModeType = 4;
        mode.SysMemBuffers = 1;

        for (unsigned char depth = 8; depth <= 32; depth <<= 1) {
            if (!(vidShowBitDepths & depth) || !(vidEnableBitDepths & depth))
                continue;

            for (int i = 0; vidDefSize && vidDefSize[i * 2]; i++) {
                mode.si.Width = vidDefSize[i * 2];
                mode.si.Height = vidDefSize[i * 2 + 1];
                mode.si.BPP = depth;
                if (!filter(&mode))
                    continue;

                if (vidnModes >= vidModeCapacity) {
                    vidModeCapacity = vidModeCapacity > 0 ? vidModeCapacity * 2 : 16;
                    vidpModes = static_cast<VIDMODE*>(std::realloc(vidpModes,
                        vidModeCapacity * sizeof(VIDMODE)));
                }
                vidpModes[vidnModes++] = mode;
            }
        }
    }

    std::qsort(vidpModes, vidnModes, sizeof(VIDMODE), CompareVidModes);
}

unsigned long vidGetCaps(unsigned long capability)
{
    if (capability == 5)
        return vidCapabilities;
    return 0;
}

int vidGetModeList(VIDMODE* filter, VIDMODE* modes, int maxModes)
{
    vidRefreshModeList();
    if (!filter) {
        const int count = maxModes < vidnModes ? maxModes : vidnModes;
        std::memcpy(modes, vidpModes, count * sizeof(VIDMODE));
        return count;
    }

    int count = 0;
    for (int i = 0; i < vidnModes && count < maxModes; i++) {
        VIDMODE* mode = &vidpModes[i];
        if (filter->hwModeType) {
            if (filter->hwModeType == 1 && mode->hwModeType == 4)
                continue;
            if (filter->hwModeType == 2 && mode->hwModeType == 5)
                continue;
            if (filter->hwModeType > 2 && mode->hwModeType != filter->hwModeType)
                continue;
        }
        if (filter->AccessMode == 1 && !mode->AccessMode)
            continue;
        if (filter->si.Width && filter->si.Width != mode->si.Width)
            continue;
        if (filter->si.Height && filter->si.Height != mode->si.Height)
            continue;
        if (filter->si.BPP && filter->si.BPP != mode->si.BPP)
            continue;
        if (filter->si.rgbMasksValid
                && (filter->si.rgbRMask != mode->si.rgbRMask
                    || filter->si.rgbGMask != mode->si.rgbGMask
                    || filter->si.rgbBMask != mode->si.rgbBMask))
            continue;
        modes[count++] = *mode;
    }

    return count;
}

static void (*vidAltEnter)(int);

int vidSetModeEx(VIDEO_INFO* info)
{
    vidOnlyFullScreen = info->bOnlyFullScreen;
    vidHideMenu = info->bHideMenu;
    vidDisableMenuKeys = info->bDisableMenuKeys;
    vidDisableAltEnter = info->bDisableAltEnter;
    vidAllowMinimizeWindow = info->bAllowMinimizeWindow;
    vidShowBitDepths = info->dwShowBitDepths;
    vidEnableBitDepths = info->dwEnableBitDepths;
    vidWindowStretch = info->dwStretch;
    vidAltEnter = reinterpret_cast<void(*)(int)>(info->fnAltEnter);
    vidModeFilter = reinterpret_cast<int(*)(void*)>(info->fnCheckVMode);
    winSetVideoMode = reinterpret_cast<int(*)(void*)>(info->fnSetVideoMode);
    vidRefreshModeList();
    return 1;
}

char* vidGetScreenModeName(int mode)
{
    switch (mode) {
    case 0:
        return const_cast<char*>("Any compatible mode");
    case 1:
        return const_cast<char*>("Any windowed mode");
    case 2:
        return const_cast<char*>("Any fullscreen mode");
    case 4:
        return const_cast<char*>("Window (DIB)");
    case 5:
        return const_cast<char*>("DirectDraw");
    default:
        return NULL;
    }
}

void vidSetPalette(unsigned char* palette, int flags)
{
    if (Video)
        return;

    for (int i = 0; i < 256; i++) {
        vidPalette[i][0] = palette[i * 4];
        vidPalette[i][1] = palette[i * 4 + 1];
        vidPalette[i][2] = palette[i * 4 + 2];
        vidPalette[i][3] = 0;
    }
}

int vidCreatePalette(int mode, void* destination, void* source, int count)
{
    if (!count)
        return 1;

    const unsigned char* sourcePixels = static_cast<const unsigned char*>(source);
    if (mode == 1 && VideoBPP > 16) {
        unsigned long* destinationPixels = static_cast<unsigned long*>(destination);
        for (int i = 0; i < count; i++) {
            destinationPixels[i] = (sourcePixels[i * 4] << VideoRPos)
                | (sourcePixels[i * 4 + 1] << VideoGPos)
                | (sourcePixels[i * 4 + 2] << VideoBPos);
        }
        return 1;
    }

    if ((mode != 0 && mode != 1) || VideoBPP != 16)
        return 0;

    unsigned short* destinationPixels = static_cast<unsigned short*>(destination);
    for (int i = 0; i < count; i++) {
        const unsigned long red = sourcePixels[i * 4];
        const unsigned long green = sourcePixels[i * 4 + 1];
        const unsigned long blue = sourcePixels[i * 4 + 2];
        destinationPixels[i] = unsigned short(
            (((red << VideoRPos) >> (8 - VideoRSize)) & VideoRMask)
            | (((green << VideoGPos) >> (8 - VideoGSize)) & VideoGMask)
            | (((blue << VideoBPos) >> (8 - VideoBSize)) & VideoBMask));
    }

    return 1;
}

void vidSetFrameReady()
{
    vidFrameTimes[vidFrameTimeIndex++] = sysGetTime();
    if (vidFrameTimeIndex == 256)
        vidFrameTimeIndex = 0;
}

int vidGetFPSRate()
{
    const unsigned long oldestTime = sysGetTime() - 1000;
    int result = 0;
    for (int i = 0; i < 256; i++) {
        if (vidFrameTimes[i] >= oldestTime)
            result++;
    }
    return result;
}

static SURFACE* vidVideoSurfaces[3];
static SURFACE* vidSystemSurfaces[3];
static SURFACE* vidSurfaceList;
static unsigned long vidBusyMode;
static int vidSetModeCalled;

static void UpdateVideoMasks()
{
    if (VideoBPP == 16) {
        VideoRPos = 11;
        VideoRSize = 5;
        VideoGPos = 5;
        VideoGSize = 6;
        VideoBPos = 0;
        VideoBSize = 5;
    }
    else {
        VideoRPos = 16;
        VideoRSize = 8;
        VideoGPos = 8;
        VideoGSize = 8;
        VideoBPos = 0;
        VideoBSize = 8;
    }

    VideoRMask = ((1UL << VideoRSize) - 1) << VideoRPos;
    VideoGMask = ((1UL << VideoGSize) - 1) << VideoGPos;
    VideoBMask = ((1UL << VideoBSize) - 1) << VideoBPos;
    VideoRGBSize = (VideoRSize << 8) | (VideoGSize << 4) | VideoBSize;
    VideoHwModeCur.si.rgbMasksValid = 1;
    VideoHwModeCur.si.rgbRMask = VideoRMask;
    VideoHwModeCur.si.rgbGMask = VideoGMask;
    VideoHwModeCur.si.rgbBMask = VideoBMask;
}

int vidSetMode(VIDMODE* requested)
{
    vidBusyMode |= 4;
    VIDMODE actual;
    if (!vidGetModeList(requested, &actual, 1)) {
        vidBusyMode &= ~4;
        return 0;
    }

    vidUnLock(NULL);
    if (!requested->AccessMode) {
        actual.SysMemBuffers = 1;
        actual.VidMemBuffers = actual.hwModeType == 5 ? 2 : 0;
    }
    else if (requested->AccessMode == 1) {
        actual.VidMemBuffers = requested->VidMemBuffers;
        actual.SysMemBuffers = requested->SysMemBuffers;
    }

    VideoW = requested->si.Width;
    VideoH = requested->si.Height;
    VideoBPP = requested->si.BPP;
    VideoModeCur = *requested;
    VideoHwModeCur = actual;

    for (int i = 0; i < 3; i++) {
        if (vidVideoSurfaces[i])
            vidFreeSurface(vidVideoSurfaces[i]);
        if (vidSystemSurfaces[i])
            vidFreeSurface(vidSystemSurfaces[i]);
        vidVideoSurfaces[i] = NULL;
        vidSystemSurfaces[i] = NULL;
    }

    for (int i = 0; i < actual.VidMemBuffers && i < 3; i++)
        vidVideoSurfaces[i] = vidCreateSurface(&actual.si);
    for (int i = 0; i < actual.SysMemBuffers && i < 3; i++)
        vidSystemSurfaces[i] = vidCreateSurface(&actual.si);

    UpdateVideoMasks();
    vidBusyMode &= ~4;
    vidSetModeCalled = 1;
    return 1;
}

void vidWaitVSYNC()
{
}

int vidFlip()
{
    vidSetFrameReady();
    return 1;
}

SURFACE* vidGetStdSurface(int type, int index)
{
    if (index < 0)
        return NULL;
    if (type == 3 && index < VideoHwModeCur.SysMemBuffers)
        return vidSystemSurfaces[index];
    if (type == 1 && index < VideoHwModeCur.VidMemBuffers)
        return vidVideoSurfaces[index];
    return NULL;
}

int vidLock(SURFACE* surface)
{
    if (!surface) {
        if (Video)
            return 0;
        if (!vidLock(vidSystemSurfaces[0]))
            return 0;
        Video = vidSystemSurfaces[0]->Ptr;
        VideoPitch = vidSystemSurfaces[0]->Pitch;
        return 1;
    }

    if (surface->Ptr)
        return 0;
    surface->Ptr = reinterpret_cast<unsigned char*>(surface->APISpecific[0]);
    surface->Pitch = surface->APISpecific[1];
    return 1;
}

int vidUnLock(SURFACE* surface)
{
    if (!surface) {
        if (!Video)
            return 0;
        vidUnLock(vidSystemSurfaces[0]);
        Video = NULL;
        VideoPitch = 0;
        return 1;
    }

    if (!surface->Ptr)
        return 0;
    surface->Ptr = NULL;
    surface->Pitch = 0;
    return 1;
}

SURFACE* vidCreateSurface(SURFACE_INFO* info)
{
    SURFACE* surface = new SURFACE;
    std::memset(surface, 0, sizeof(SURFACE));
    surface->si = *info;
    const unsigned long pitch = info->Width * ((info->BPP + 7) / 8);
    surface->APISpecific[0] = reinterpret_cast<unsigned long>(new unsigned char[pitch * info->Height]);
    surface->APISpecific[1] = pitch;
    surface->APISpecific[2] = 4;
    surface->pNext = vidSurfaceList;
    if (vidSurfaceList)
        vidSurfaceList->pPrev = surface;
    vidSurfaceList = surface;
    return surface;
}

int vidFreeSurface(SURFACE* surface)
{
    if (!surface || !surface->APISpecific[2])
        return 0;

    if (surface->Ptr)
        vidUnLock(surface);
    delete[] reinterpret_cast<unsigned char*>(surface->APISpecific[0]);

    if (surface->pPrev)
        surface->pPrev->pNext = surface->pNext;
    else
        vidSurfaceList = surface->pNext;
    if (surface->pNext)
        surface->pNext->pPrev = surface->pPrev;

    for (int i = 0; i < 3; i++) {
        if (vidVideoSurfaces[i] == surface)
            vidVideoSurfaces[i] = NULL;
        if (vidSystemSurfaces[i] == surface)
            vidSystemSurfaces[i] = NULL;
    }

    std::memset(surface, 0, sizeof(SURFACE));
    delete surface;
    return 1;
}

int vidSetColorKey(SURFACE* surface, unsigned long color)
{
    if (!surface)
        return 0;
    surface->APISpecific[3] = color;
    return 1;
}

static int winMinimized;
static unsigned long sysTimeGap;

void vidPaintWindow(void* window)
{
}

void vidCloseDisplay(int resizing)
{
    if (vidBusyMode & 8)
        return;

    vidUnLock(NULL);
    for (int i = 0; i < 3; i++) {
        if (vidSystemSurfaces[i])
            vidFreeSurface(vidSystemSurfaces[i]);
        if (vidVideoSurfaces[i])
            vidFreeSurface(vidVideoSurfaces[i]);
        vidSystemSurfaces[i] = NULL;
        vidVideoSurfaces[i] = NULL;
    }

    vidBusyMode |= 8;
    vidBusyMode &= ~8;
}

void vidToggleFullScreenMode()
{
    if (vidAltEnter) {
        vidAltEnter(0);
        return;
    }

    if (VideoModeCur.AccessMode == 1 || vidOnlyFullScreen) {
        vidMinimize(1);
        return;
    }

    if (VideoModeCur.AccessMode)
        return;

    VIDMODE mode = VideoModeCur;
    mode.hwModeType = VideoHwModeCur.hwModeType == 4 ? 2 : 1;
    if (winSetVideoMode)
        winSetVideoMode(&mode);
    else
        vidSetMode(&mode);
}

void vidIdleEnter()
{
    vidBusyMode |= 1;
    winMinimized = 1;
    vidBusyMode &= ~1;
}

void vidWaitScreenUnblank()
{
    const unsigned long startTime = sysGetTime();
    winMinimized = 1;
    winMinimized = 0;
    sysTimeGap += sysGetTime() - startTime;
}

void vidMinimize(int waitForRestore)
{
    const unsigned long startTime = sysGetTime();
    vidBusyMode |= 1;
    winMinimized = 1;
    vidBusyMode &= ~1;

    if (waitForRestore)
        vidWaitRestore();
    sysTimeGap += sysGetTime() - startTime;
}

void vidWaitRestore()
{
    vidRestore();
}

void vidRestore()
{
    vidBusyMode |= 2;
    for (SURFACE* surface = vidSurfaceList; surface; surface = surface->pNext)
        surface->Lost = 0;
    vidBusyMode &= ~2;
    winMinimized = 0;
}

int vidIsMinimized()
{
    return winMinimized;
}

struct VideoRect {
    long left;
    long top;
    long right;
    long bottom;
};

int vidBlit(BLIT_CMD* command)
{
    if (!command)
        return vidFlip();
    if (!command->dstSurf || !command->srcSurf)
        return 0;

    SURFACE* destination = command->dstSurf;
    SURFACE* source = command->srcSurf;
    vidLock(destination);
    vidLock(source);
    if (!destination->Ptr || !source->Ptr)
        return 0;

    const int bytesPerPixel = (source->si.BPP + 7) / 8;
    VideoRect dst = { 0, 0, long(destination->si.Width), long(destination->si.Height) };
    VideoRect src = { 0, 0, long(source->si.Width), long(source->si.Height) };
    if (command->dstRect)
        dst = *reinterpret_cast<VideoRect*>(command->dstRect);
    if (command->srcRect)
        src = *reinterpret_cast<VideoRect*>(command->srcRect);

    int width = dst.right - dst.left;
    int height = dst.bottom - dst.top;
    if (src.right - src.left < width)
        width = src.right - src.left;
    if (src.bottom - src.top < height)
        height = src.bottom - src.top;

    for (int y = 0; y < height; y++) {
        unsigned char* destinationRow = destination->Ptr
            + (dst.top + y) * destination->Pitch + dst.left * bytesPerPixel;
        const unsigned char* sourceRow = source->Ptr
            + (src.top + y) * source->Pitch + src.left * bytesPerPixel;

        if (!command->Command) {
            std::memcpy(destinationRow, sourceRow, width * bytesPerPixel);
            continue;
        }

        for (int x = 0; x < width; x++) {
            unsigned long pixel = 0;
            std::memcpy(&pixel, sourceRow + x * bytesPerPixel, bytesPerPixel);
            if (pixel != command->Data)
                std::memcpy(destinationRow + x * bytesPerPixel,
                    sourceRow + x * bytesPerPixel, bytesPerPixel);
        }
    }

    vidUnLock(source);
    vidUnLock(destination);
    return 1;
}

int vidBlitBatch(BLIT_CMD* commands, int count)
{
    int result = 1;
    for (int i = 0; i < count; i++)
        result &= vidBlit(&commands[i]);
    return result;
}

int vidLostFocus()
{
    for (int i = 0; i < 3; i++) {
        if (vidVideoSurfaces[i] && vidVideoSurfaces[i]->Lost)
            return 1;
    }
    return 0;
}

int SetVMode(VIDMODE* mode)
{
    if (winSetVideoMode)
        return winSetVideoMode(mode);
    return vidSetMode(mode);
}

static int vidCheckMemory = 1;

int vidGetMemInfo(VIDMEM_INFO* info)
{
    if (!vidCheckMemory)
        return 0;

    info->vidMem = 0;
    info->vidMemFree = 0;
    info->agpMem = 0;
    info->agpMemFree = 0;
    for (SURFACE* surface = vidSurfaceList; surface; surface = surface->pNext)
        info->vidMem += surface->APISpecific[1] * surface->si.Height;
    return 1;
}

static char* vidLastError;

int ddCheck(long error)
{
    if (!error)
        return 1;
    vidLastError = const_cast<char*>("DirectDraw error");
    return 0;
}

char* vidGetLastError()
{
    return vidLastError ? vidLastError : const_cast<char*>("No error");
}

void ccDrawCaption(void* window, CUSTOMCAPTION* caption, int active, void* region)
{
    HWND windowHandle = static_cast<HWND>(window);
    if (!IsWindowVisible(windowHandle) || IsIconic(windowHandle))
        return;

    RECT windowRect;
    if (!GetWindowRect(windowHandle, &windowRect))
        return;

    const int border = caption->bSizeBorder ? GetSystemMetrics(SM_CXSIZEFRAME) + 3 : 3;
    const int captionHeight = GetSystemMetrics(SM_CYCAPTION);
    RECT captionRect = {
        border,
        border,
        windowRect.right - windowRect.left - border,
        border + captionHeight
    };
    HDC deviceContext = GetWindowDC(windowHandle);
    if (region) {
        SelectClipRgn(deviceContext, static_cast<HRGN>(region));
        OffsetClipRgn(deviceContext, -windowRect.left, -windowRect.top);
    }

    BitBlt(deviceContext, captionRect.left, captionRect.top,
        captionRect.right - captionRect.left, captionRect.bottom - captionRect.top,
        static_cast<HDC>(caption->hDC), 0, active ? captionHeight : 0, SRCCOPY);

    if (region) {
        ExcludeClipRect(deviceContext, captionRect.left, captionRect.top,
            captionRect.right, captionRect.bottom);
        OffsetClipRgn(deviceContext, windowRect.left, windowRect.top);
        GetClipRgn(deviceContext, static_cast<HRGN>(region));
    }
    ReleaseDC(windowHandle, deviceContext);
}

void ccRenderCaption(void* window, CUSTOMCAPTION* caption)
{
    HWND windowHandle = static_cast<HWND>(window);
    caption->bActive = 0;

    RECT windowRect;
    if (!GetWindowRect(windowHandle, &windowRect))
        return;

    const int border = caption->bSizeBorder ? GetSystemMetrics(SM_CXSIZEFRAME) + 3 : 3;
    const int captionHeight = GetSystemMetrics(SM_CYCAPTION);
    const int width = (windowRect.right - windowRect.left - border * 2 + 3) & ~3;
    const int height = captionHeight * 2;
    caption->iWidth = width;
    caption->bmpWidth = width;
    delete[] caption->bmpData;
    caption->bmpData = new unsigned char[width * height * 4];

    const COLORREF colors[2] = {
        GetSysColor(COLOR_INACTIVECAPTION),
        GetSysColor(COLOR_ACTIVECAPTION)
    };
    for (int state = 0; state < 2; state++) {
        for (int y = 0; y < captionHeight; y++) {
            for (int x = 0; x < width; x++) {
                unsigned char* pixel = caption->bmpData
                    + ((state * captionHeight + y) * width + x) * 4;
                pixel[0] = GetBValue(colors[state]);
                pixel[1] = GetGValue(colors[state]);
                pixel[2] = GetRValue(colors[state]);
                pixel[3] = 0;
            }
        }
    }

    caption->bActive = 1;
    PostMessage(windowHandle, WM_NCPAINT, 1, 0);
}
