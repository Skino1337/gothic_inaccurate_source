#ifndef ZVIDEO_WIN32_H
#define ZVIDEO_WIN32_H

const unsigned int BLIT_COMMANDS = 6;

struct VIDEO_INFO {
    int           bOnlyFullScreen;
    int           bHideMenu;
    int           bDisableMenuKeys;
    int           bDisableAltEnter;
    int           bDisableCtrlAltDel;
    int           bAllowMinimizeWindow;
    unsigned long dwShowBitDepths;
    unsigned long dwEnableBitDepths;
    unsigned long dwStretch;
    void*         fnAltEnter;
    void*         fnCheckVMode;
    void*         fnSetVideoMode;
};

struct VIDMEM_INFO {
    unsigned long vidMem;
    unsigned long vidMemFree;
    unsigned long agpMem;
    unsigned long agpMemFree;
};

struct SURFACE_INFO {
    unsigned char MemType;
    unsigned long Width;
    unsigned long Height;
    unsigned char BPP;
    int           rgbMasksValid;
    unsigned long rgbRMask;
    unsigned long rgbGMask;
    unsigned long rgbBMask;
};

struct SURFACE {
    SURFACE_INFO  si;
    unsigned char* Ptr;
    unsigned long Pitch;
    unsigned long APISpecific[5];
    int           Lost;
    SURFACE*      pNext;
    SURFACE*      pPrev;
};

struct VIDMODE {
    SURFACE_INFO  si;
    unsigned char hwModeType;
    unsigned char AccessMode;
    unsigned char VidMemBuffers;
    unsigned char SysMemBuffers;
    unsigned char BlitInfo[BLIT_COMMANDS];
    unsigned long APISpecific[4];
};

struct tagRECT;

struct BLIT_CMD {
    SURFACE*      dstSurf;
    tagRECT*      dstRect;
    SURFACE*      srcSurf;
    tagRECT*      srcRect;
    unsigned long Command;
    unsigned long Data;
};

struct CUSTOMCAPTION {
    void*         hIcon;
    long          iButtons;
    void*         hDC;
    void*         hBmp;
    long          iWidth;
    long          winWidth;
    int           bActive;
    int           bSizeBorder;
    unsigned long bmpWidth;
    unsigned char* bmpData;
    unsigned char* bmpBackground;
};

void          vidRefreshModeList();
unsigned long vidGetCaps(unsigned long capability);
int           vidGetModeList(VIDMODE* filter, VIDMODE* modes, int maxModes);
int           vidSetModeEx(VIDEO_INFO* info);
char*         vidGetScreenModeName(int mode);
void          vidSetPalette(unsigned char* palette, int flags);
int           vidCreatePalette(int mode, void* destination, void* source, int count);
void          vidSetFrameReady();
int           vidGetFPSRate();
int           vidSetMode(VIDMODE* requested);
void          vidWaitVSYNC();
int           vidFlip();
SURFACE*      vidGetStdSurface(int type, int index);
int           vidLock(SURFACE* surface);
int           vidUnLock(SURFACE* surface);
SURFACE*      vidCreateSurface(SURFACE_INFO* info);
int           vidFreeSurface(SURFACE* surface);
int           vidSetColorKey(SURFACE* surface, unsigned long color);
void          vidPaintWindow(void* window);
void          vidCloseDisplay(int resizing);
void          vidToggleFullScreenMode();
void          vidIdleEnter();
void          vidWaitScreenUnblank();
void          vidMinimize(int waitForRestore);
void          vidWaitRestore();
void          vidRestore();
int           vidIsMinimized();
int           vidBlit(BLIT_CMD* command);
int           vidBlitBatch(BLIT_CMD* commands, int count);
int           vidLostFocus();
int           SetVMode(VIDMODE* mode);
int           vidGetMemInfo(VIDMEM_INFO* info);
int           ddCheck(long error);
char*         vidGetLastError();
void          ccDrawCaption(void* window, CUSTOMCAPTION* caption, int active, void* region);
void          ccRenderCaption(void* window, CUSTOMCAPTION* caption);

extern unsigned char* Video;
extern long           VideoW;
extern long           VideoH;
extern long           VideoPitch;
extern unsigned long  VideoBPP;
extern unsigned long  VideoRGBSize;
extern unsigned long  VideoRPos;
extern unsigned long  VideoRSize;
extern unsigned long  VideoRMask;
extern unsigned long  VideoGPos;
extern unsigned long  VideoGSize;
extern unsigned long  VideoGMask;
extern unsigned long  VideoBPos;
extern unsigned long  VideoBSize;
extern unsigned long  VideoBMask;
extern VIDMODE        VideoHwModeCur;
extern VIDMODE        VideoModeCur;
extern VIDMODE*       vidpModes;
extern int            vidnModes;
extern unsigned long* vidDefSize;
extern int            vidOnlyFullScreen;
extern int            vidHideMenu;
extern int            vidDisableMenuKeys;
extern int            vidDisableAltEnter;
extern int            vidAllowMinimizeWindow;
extern unsigned long  vidShowBitDepths;
extern unsigned long  vidEnableBitDepths;
extern unsigned long  vidWindowStretch;
extern int            (*vidModeFilter)(void*);
extern int            (*winSetVideoMode)(void*);

#endif
