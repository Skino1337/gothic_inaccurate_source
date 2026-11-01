#ifndef ZMENUITEM_H
#define ZMENUITEM_H

#include "zSession.h"

class zCViewWindow;

const int MAX_ITEMS = 150;
const int MAX_EVENTS = 10;
const int MAX_SEL_ACTIONS = 5;
const int MAX_USERVARS = 4;
const int MAX_USERSTRINGS = 10;

enum zTMenuItem {
    MENU_ITEM_UNDEF,
    MENU_ITEM_TEXT,
    MENU_ITEM_SLIDER,
    MENU_ITEM_INPUT,
    MENU_ITEM_CURSOR,
    MENU_ITEM_CHOICEBOX,
    MENU_ITEM_BUTTON,
    MENU_ITEM_LISTBOX
};

enum zTMenuItemEvent {
    MENU_EVENT_UNDEF,
    MENU_EVENT_EXECUTE,
    MENU_EVENT_CHANGED,
    MENU_EVENT_LEAVE,
    MENU_EVENT_TIMER,
    MENU_EVENT_CLOSE,
    MENU_EVENT_INIT,
    MENU_EVENT_SEL_PREV,
    MENU_EVENT_SEL_NEXT,
    MENU_EVENT_MAX
};

enum zTMenuItemSelAction {
    SEL_ACTION_UNDEF,
    SEL_ACTION_BACK,
    SEL_ACTION_STARTMENU,
    SEL_ACTION_STARTITEM,
    SEL_ACTION_CLOSE,
    SEL_ACTION_CONCOMMANDS,
    SEL_ACTION_PLAY_SOUND,
    SEL_ACTION_EXECCOMMANDS
};

class zCMenuItem : public zCView {
public:

    zCMenuItem();
    zCMenuItem(const zSTRING& name);
    virtual ~zCMenuItem();

    static int CompareNames(const void* first, const void* second);
    static void Startup();
    static void Shutdown();
    static zCMenuItem* Create(const zSTRING& name);
    static zCMenuItem* GetByName(const zSTRING& name);

    zCMenuItem* GetSelItem();
    void  Release();
    void  Init();
    void  SetByScript(zSTRING& script);
    int   GetDimX(int value);
    int   GetDimY(int value);
    void  InitView();
    void  Close();
    int   GetType();
    void  SetName(const zSTRING& name);
    void  SetName(zSTRING& name);
    zSTRING& GetName();
    void  GetCoords(int& x1, int& y1, int& x2, int& y2);
    int   GetSelActionType(int index);
    zSTRING& GetSelActionString(int index);
    void  Register();
    void  Unregister();
    int   IsRegistered();
    void  UpdateSize();
    void  SetIsActive(int active);
    void  SetIsSelectable(int selectable);
    int   GetIsSelectable();
    void  SetIsDisabled(int disabled);
    int   GetIsDisabled();
    void  SetIsVisible(int visible);
    int   GetIsVisible();

    virtual void DrawItems();
    virtual void InitMenuItem();
    virtual int ToggleValue(int direction, int alternate);
    virtual int Run();
    virtual zSTRING GetText(int index);
    virtual void SetText(const zSTRING& text, int index, int animated);
    virtual void SetText(zCArray<zSTRING> text, int animated);
    virtual void SetMinValue(float value);
    virtual void SetMaxValue(float value);
    virtual void Enter();
    virtual void Leave();
    virtual void Draw();
    virtual void DrawFront();
    virtual int Show();
    virtual int Hide();
    virtual int HasBeenCanceled();
    virtual void Open();
    virtual int Input(int key, zSTRING& text);
    virtual void InsertInWin(zCView* window);
    virtual void RemoveFromWin();
    virtual void UpdateContent();
    virtual void UpdateFX();
    virtual int HandleEvent(int key);

    zSTRING m_parFontName;
    zSTRING m_parText[MAX_USERSTRINGS];
    zSTRING m_parBackPic;
    zSTRING m_parAlphaMode;
    int m_parAlpha;
    int m_parType;
    int m_parOnSelAction[MAX_SEL_ACTIONS];
    zSTRING m_parOnSelAction_S[MAX_SEL_ACTIONS];
    zSTRING m_parOnChgSetOption;
    zSTRING m_parOnChgSetOptionSection;
    int m_parOnEventAction[MAX_EVENTS];
    int m_parPosX;
    int m_parPosY;
    int m_parDimX;
    int m_parDimY;
    float m_parSizeStartScale;
    int m_parItemFlags;
    float m_parOpenDelayTime;
    float m_parOpenDuration;
    float m_parUserFloat[MAX_USERVARS];
    zSTRING m_parUserString[MAX_USERVARS];
    int m_parFrameSizeX;
    int m_parFrameSizeY;
    zSTRING m_parHideIfOptionSectionSet;
    zSTRING m_parHideIfOptionSet;
    int m_parHideOnValue;
    int m_iRefCtr;
    zCView* m_pInnerWindow;
    zCFont* m_pFont;
    zCFont* m_pFontHi;
    zCFont* m_pFontSel;
    zCFont* m_pFontDis;
    int m_bViewInitialized;
    int m_bLeaveItem;
    int m_bVisible;
    int m_bDontRender;
    zCArray<zSTRING> m_listLines;
    zSTRING id;
    int inserted;
    int changed;
    int active;
    int open;
    int close;
    int opened;
    int closed;
    int disabled;
    zCView* orgWin;
    float fxTimer;
    float openDelayTimer;
    float activeTimer;
    int registeredCPP;
    int firstTimeInserted;

    static zCArraySort<zCMenuItem*> itemList;
    static zCMenuItem* tmp;
};

class zCMenuItemText : public zCMenuItem {
public:
    enum zCMenuItemTextEnum3 {
        MODE_SIMPLE,
        MODE_ENUM,
        MODE_MULTILINE
    };

    zCMenuItemText(const zSTRING& name);
    virtual ~zCMenuItemText();
    void  FormatMultiline();
    void  DrawMultiline();
    virtual void InitMenuItem();
    virtual void SetText(const zSTRING& text, int index, int animated);
    virtual void SetText(zCArray<zSTRING> text, int animated);
    virtual void DrawFront();
    virtual void SetEnum(const zSTRING& text);
    virtual int HandleEvent(int key);

    zCMenuItemTextEnum3 m_mode;
    zSTRING m_fullText;
    int m_numOptions;
    int m_topLine;
    int m_viewLines;
    int m_numLines;
    int m_unformated;
};

class zCMenuItemSlider : public zCMenuItem {
public:
    zCMenuItemSlider(const zSTRING& name);
    virtual ~zCMenuItemSlider();
    void  SetValue(float value);
    float GetValue();
    virtual void InitMenuItem();
    virtual int ToggleValue(int direction, int alternate);
    virtual void InsertInWin(zCView* window);
    virtual void RemoveFromWin();
    virtual void SetIsActive(int active);
    virtual int HandleEvent(int key);

    zCViewWindow* slider;
    int slideStep;
    float sliderValue;
    int sliderDimX;
    int sliderPos;
};

class zCMenuItemInput : public zCMenuItem {
public:
    zCMenuItemInput(const zSTRING& name);
    virtual ~zCMenuItemInput();
    void  SetValue(zSTRING value);
    zSTRING GetValue();
    virtual void InitMenuItem();
    virtual void InsertInWin(zCView* window);
    virtual int Run();
    virtual void Enter();
    virtual void Leave();
    virtual int HandleEvent(int key);
    virtual int HasBeenCanceled();

    zSTRING value;
    int curPos;
    int canceled;
};

class zCMenuItemChoice : public zCMenuItem {
public:
    zCMenuItemChoice(const zSTRING& name);
    virtual ~zCMenuItemChoice();
    int   SetOptionByString(zSTRING text);
    zSTRING GetStringByOption();
    void  Resize();
    int   GetNumOptions();
    virtual void InitMenuItem();
    virtual int ToggleValue(int direction, int alternate);
    virtual zSTRING GetText(int index);
    virtual void Draw();
    virtual void DrawFront();
    virtual void InsertInWin(zCView* window);
    virtual void RemoveFromWin();

    zCView* winCursor;
    int option;
    int optionStart;
    int curStep;
};

class zCMenuItemButton : public zCMenuItem {
public:
    zCMenuItemButton(const zSTRING& name);
    virtual ~zCMenuItemButton();
    void  Toggle();
    virtual void InitMenuItem();
    virtual int Run();
    virtual void InsertInWin(zCView* window);
    virtual void RemoveFromWin();

    zSTRING pic;
    int active;
};

struct zTMenuItemListElement {
    zTMenuItemListElement();

    zSTRING title;
    zSTRING content;
    zCMenuItem* contentViewer;
};

class zCMenuItemList : public zCMenuItem {
public:
    zCMenuItemList(const zSTRING& name);
    virtual ~zCMenuItemList();
    void  DeleteContent();
    void  InsertEnd(zTMenuItemListElement& element);
    void  InsertFront(zTMenuItemListElement& element);
    void  ShowContent();
    void  JumpToNextEntry();
    void  JumpToPrevEntry();
    void  JumpToTopEntry();
    void  JumpToLastEntry();
    virtual void InitMenuItem();
    virtual int Run();
    virtual void Enter();
    virtual void Leave();
    virtual void DrawFront();
    virtual int Show();
    virtual int Hide();
    virtual void InsertInWin(zCView* window);
    virtual void RemoveFromWin();
    virtual void UpdateContent();
    virtual int HandleEvent(int key);

    zCArray<zTMenuItemListElement> m_list;
    int m_cur_element;
    int m_top_element;
    zCView* m_pInnerWindow2;
    zCView* m_pViewArrowUp;
    zCView* m_pViewArrowDn;
};

#endif
