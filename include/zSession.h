#ifndef ZSESSION_H
#define ZSESSION_H

#include "z3d.h"
#include "zViewBase.h"
#include "zFonts.h"
#include "zTools.h"

extern zSTRING CamModDialog;
extern zCView* screen;
extern zCFontMan* zfontman;

int rnd2(float value);

class zCAICamera {
public:

    static zCAICamera* Create();
    static zCAICamera* GetCurrent();
    int                Release();
    void               SetTarget(zCVob* target);
    void               SetDialogCamDuration(float duration);
    void               SetMode(const zSTRING& mode);
    int                IsModeActive(const zSTRING& mode);
    void               ReceiveMsg(int message);

    zCVob* camVob;
};

class zCInputCallback {
public:

    zCInputCallback();
    virtual ~zCInputCallback();
    virtual int HandleEvent(int key);
    int GetEnableHandleEvent() const;
    void SetEnableHandleEvent(int enabled);
    void SetHandleEventTop();
    static int DoEvents(int key);
    static void GetInput();

    static zCList<zCInputCallback> inputList;
};

class zCViewText {
public:

    zCViewText() {}
    zCViewText(int x, int y, const zSTRING& text);
    zCViewText(int x, int y, const zSTRING& text, float& timer, zCOLOR& color,
        int timed, int colored);
    virtual ~zCViewText();
    void Create(int x, int y, const zSTRING& text, float& timer, zCOLOR& color,
        int timed, int colored);

    int posx;
    int posy;
    zSTRING text;
    zCFont* font;
    float timer;
    int inPrintWin;
    zCOLOR color;
    int timed;
    int colored;
};

class zCView : public zCViewBase, public zCInputCallback {
public:

    zCView();
    zCView(int x1, int y1, int x2, int y2, int viewID = 2);
    virtual ~zCView();
    void Init();
    void Init(int x1, int y1, int x2, int y2, int viewID);
    void Init(int x, int y, const zSTRING& text, int viewID);
    void SetPos(int x, int y);
    void SetSize(int x, int y);
    void SetAutoScroll(float time);
    void CheckAutoScroll();
    void SetTransparency(int alpha);
    void SetAlphaBlendFunc(int function);
    void SetColor(const zCOLOR& color);
    zCOLOR GetColor();
    static void StdPrintwin(const zSTRING& text);
    void Setflags(int flags);
    void ClrFlags(int flags);
    int HasFlag(int flag);
    void InsertBack(zCTexture* texture);
    void InsertBack(const zSTRING& name);
    void InitPrintwin();
    void BlitText();
    void PrintChars(int x, int y, const zSTRING& text);
    virtual void Blit();
    virtual void DrawItems();
    void Top();
    void Create(const zSTRING& backName, int openEffect, int closeEffect, float scrollTime,
        int resize);
    void ForceOpen();
    void ApplyOpenClose(zVEC2& topLeft, zVEC2& bottomRight);
    void UpdateOpen();
    void UpdateClose();
    void UpdateOpenZoom();
    void UpdateCloseZoom();
    void Open();
    void Close();
    void InsertItem(zCView* item, int priority = 0);
    void RemoveItem(zCView* item);
    void Render();
    zSTRING PatchMenuItemPrintwin();
    void CalcPixelPos(int& x, int& y);
    void GetPos(int& x, int& y);
    void GetPixelPos(int& x, int& y);
    void Move(int x, int y);
    void GetSize(int& x, int& y);
    void GetPixelSize(int& x, int& y);
    virtual void GetViewport(int& x, int& y, int& width, int& height);
    void GetExtends(int& x1, int& y1, int& x2, int& y2);
    void GetScreenExtends(int& x1, int& y1, int& x2, int& y2);
    void GetPixelExtends(float& x1, float& y1, float& x2, float& y2);
    int IsIn(int x, int y);
    zCView* GetItem(int x, int y);
    void RecalcChildsPos();
    void RecalcChildsSize();
    zCViewText* CreateText(int x, int y, const zSTRING& text, float timer = 0.0f,
        zCOLOR& color = GFX_WHITE, int timed = 0, int colored = 0);
    void CheckTimedText();
    static float s_fViewTimePerChar;
    void SetFont(const zSTRING& name);
    void SetFont(zCFont* font);
    void SetFontColor(const zCOLOR& color);
    zCFont* GetFont();
    zSTRING GetFontName();
    virtual int anx(int x);
    virtual int any(int y);
    virtual int nax(int x);
    virtual int nay(int y);
    int  FontSize(const zSTRING& text);
    int  FontY() const;
    void Print(int x, int y, const zSTRING& text);
    void PrintCX(int x, const zSTRING& text);
    void PrintCY(int y, const zSTRING& text);
    void PrintCXY(const zSTRING& text);
    void Printwin(const zSTRING& text);
    void ClrPrintwin();
    void PrintTimed(int x, int y, const zSTRING& text, float time, zCOLOR* color = NULL);
    void PrintTimedCX(int x, const zSTRING& text, float time, zCOLOR* color = NULL);
    void PrintTimedCY(int y, const zSTRING& text, float time, zCOLOR* color = NULL);
    void PrintTimedCXY(const zSTRING& text, float time, zCOLOR* color = NULL);
    void PrintMessage(const zSTRING& text, const zSTRING& title, float time, zCOLOR& color);
    void PrintMessageCXY(const zSTRING& text, const zSTRING& title, float time, zCOLOR& color);
    void PrintSelection(const zSTRING& first, const zSTRING& second, zCOLOR& firstColor,
        zCOLOR& secondColor);
    void Nextline(const zSTRING& text, float time, zCOLOR* color, int* count);
    zSTRING GetFirstText();
    void Dialog(int x, int y, const zSTRING& text, float time, zCOLOR* color = NULL);
    void DialogCX(int x, const zSTRING& text, float time, zCOLOR* color = NULL);
    void DialogCY(int y, const zSTRING& text, float time, zCOLOR* color = NULL);
    void DialogCXY(const zSTRING& text, float time, zCOLOR* color = NULL);
    void DialogMessage(const zSTRING& text, const zSTRING& title, float time, zCOLOR& color);
    void DialogMessageCXY(const zSTRING& text, const zSTRING& title, float time, zCOLOR& color);
    void DialogSelection(const zSTRING& first, const zSTRING& second, zCOLOR& firstColor,
        zCOLOR& secondColor, float time);
    void DefPrintwin(int x1, int y1, int x2, int y2);
    zSTRING Input(int maxChars);
    zSTRING Input(int maxChars, zSTRING& text);
    zSTRING Input(int maxChars, int x, int y, zSTRING& text);
    int InputNumber(int maxChars, int defaultValue);
    static void PrintDebug(const zSTRING& text);
    static void SetShowDebug(int enabled);
    static int GetShowDebug();
    static void ChangeMode();
    static void SetNextMode();
    static void SetMode(int width, int height, int bpp, HWND__** window);
    virtual void Line(int x1, int y1, int x2, int y2, const zCOLOR& color);
    virtual int GetCode(int x, int y);
    virtual int ClipLine(int& x1, int& y1, int& x2, int& y2);

    int m_bFillZ;
    zCView* next;
    int viewID;
    int flags;
    int intflags;
    int ondesk;
    int alphafunc;
    zCOLOR color;
    int alpha;
    zCList<zCView> childs;
    zCView* owner;
    zCTexture* backTex;
    int vposx;
    int vposy;
    int vsizex;
    int vsizey;
    int pposx;
    int pposy;
    int psizex;
    int psizey;
    zCFont* font;
    zCOLOR fontColor;
    int px1;
    int py1;
    int px2;
    int py2;
    int winx;
    int winy;
    zCList<zCViewText> textLines;
    float scrollMaxTime;
    float scrollTimer;
    int fxOpen;
    int fxClose;
    float timeDialog;
    float timeOpen;
    float timeClose;
    float speedOpen;
    float speedClose;
    int isOpen;
    int isClosed;
    int continueOpen;
    int continueClose;
    int removeOnClose;
    int resizeOnOpen;
    int maxTextLength;
    zSTRING textMaxLength;
    zVEC2 posCurrent[2];
    zVEC2 posOpenClose[2];
    int animatedWindows;
    int animatedAlpha;

    static zCView* modal;
    static zCView* stdoutput;
    static int showDebug;
};

struct zTProgressRange {
    int iteration;
    float min;
    float max;
};

class zCViewProgressBar : public zCView {
public:

    zCViewProgressBar();
    zCViewProgressBar(int x1, int y1, int x2, int y2, int viewID);
    virtual ~zCViewProgressBar();
    virtual void Init();
    float CalcPercentOf(float percent, float min, float max);
    void SetPercent(int percent, zSTRING text);
    virtual void HandleChange();
    virtual void Draw();
    void SetRange(int minimum, int maximum);
    void ResetRange();

    zCView* bar;
    int minStep;
    int percent;
    zSTRING text;
    long long start_time;
    zCList<zTProgressRange> rangeList;
};

class zCViewStatusBar : public zCView {
public:

    zCViewStatusBar();
    zCViewStatusBar(int x1, int y1, int x2, int y2, int viewID);
    virtual ~zCViewStatusBar();
    void Init();
    virtual void SetMaxRange(float min, float max);
    virtual void SetRange(float min, float max);
    virtual void SetPreview(float value);
    virtual void SetValue(float value);

    float minLow;
    float maxHigh;
    float low;
    float high;
    float previewValue;
    float currentValue;
};

class oCViewStatusBar : public zCViewStatusBar {
public:

    oCViewStatusBar();
    oCViewStatusBar(int x1, int y1, int x2, int y2, int viewID);
    virtual ~oCViewStatusBar();
    void Init(int x, int y, float scale);
    void SetTextures(const zSTRING& view, const zSTRING& range, const zSTRING& value,
        const zSTRING& unused);
    virtual void SetMaxRange(float min, float max);
    virtual void SetRange(float min, float max);
    virtual void SetPreview(float value);
    virtual void SetValue(float value);

    float scale;
    zCView* range_bar;
    zCView* value_bar;
    zSTRING texView;
    zSTRING texRange;
    zSTRING texValue;
};

class zCSession : public zCInputCallback {
public:

    zCSession();
    virtual ~zCSession();

    virtual void         Init();
    virtual void         Done();
    virtual void         DesktopInit();
    virtual void         CutsceneSystemInit();
    virtual void         Render();
    virtual void         RenderBlit();
    virtual void         CamInit();
    virtual void         CamInit(zCVob* vob, zCCamera* camera);
    virtual void         SetCamera(zCCamera* camera);
    virtual zCCamera*    GetCamera() const;
    virtual zCAICamera*  GetCameraAI() const;
    virtual zCVob*       GetCameraVob() const;
    virtual zCView*      GetViewport() const;
    virtual zCCSManager* GetCutsceneManager();
    virtual zCWorld*     GetWorld();
    virtual void         SaveWorld(const zSTRING& fileName, int saveMode, int flags, int saveGame);
    virtual int          LoadWorld(const zSTRING& fileName, int loadMode);
    virtual void         SetTime(int hour, int minute, int second);
    virtual void         GetTime(int& hour, int& minute, int& second);
    virtual void         SetWorld(zCWorld* world);

    zCCSManager* csMan;
    zCWorld*     world;
    zCCamera*    camera;
    zCAICamera*  aiCam;
    zCVob*       camVob;
    zCView*      viewport;
};

#endif
