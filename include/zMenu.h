#ifndef ZMENU_H
#define ZMENU_H

#include "zSession.h"
#include "zTools.h"
#include "zMenuItem.h"

class zCParser;
class zCViewWindow : public zCView {
public:

    zCViewWindow(int x1, int y1, int x2, int y2, int viewID = 2);
    virtual ~zCViewWindow();
};
class zCMusicTheme;
class zCWorld;
class zCCamera;

extern const zSTRING zMENU_SEPARATOR;

class zCMenu : public zCInputCallback {
public:

    enum zCMenuEnum2 {
        NONE,
        BACK,
        GO,
        FINISHED
    };

    zCMenu();
    zCMenu(const zSTRING& name);
    virtual ~zCMenu();

    void  InitValues();
    void  Release();
    void  Register();
    void  Unregister();
    int   IsRegistered();
    void  ClearAllEvents();
    void  PreviousOption();
    void  NextOption();
    void  RestartMusicTheme();
    void  ForceSelAction(int action, const zSTRING& target, zCMenuItem* item);
    void  UpdateWindowGeometry();
    void  PrintInformation(zSTRING text);
    zSTRING& GetBackWorld();
    zSTRING& GetBackDatFile();
    zCMenuItem* GetActiveItem();
    int   GetNumberOfActiveItem();
    int   SetActiveItem(zCMenuItem* item);
    zSTRING& GetName();
    void  SetName(const zSTRING& name);
    static void DefineExternal(const zSTRING& name, int(*function)(), int returnType, int elements, ...);
    void  HandleFrame(int key);

    static int CompareNames(const void* first, const void* second);
    static void CreateParser();
    static void Startup();
    static void Shutdown();
    static zCMenu* Create(const zSTRING& name);
    static zCParser* GetParser();
    static zCMenu* GetByName(const zSTRING& name);
    static zCMenu* GetActive();

    virtual int HandleEvent(int key);
    virtual int HandleEnterItem(zCMenuItem* item);
    virtual int HandleLeaveItem(zCMenuItem* item);
    virtual int HandleActivateItem(zCMenuItem* item);
    virtual int Run();
    virtual void Enter();
    virtual void Leave();
    virtual void Render();
    virtual void RedrawItems();
    virtual void Activate();
    virtual void ScreenInit();
    virtual void ScreenDone();
    virtual int HandleAction();
    virtual int HandleSelAction(int action, const zSTRING& target, zCMenuItem* item);
    virtual void SetByScript(const zSTRING& script);
    virtual int ExecCommand(const zSTRING& command);

    zSTRING backPic;
    zSTRING backWorld;
    int posx;
    int posy;
    int dimx;
    int dimy;
    int alpha;
    zSTRING musicThemeName;
    int eventTimerUpdateMSec;
    zSTRING itemID[MAX_ITEMS];
    int flags;
    int defaultOutGame;
    int defaultInGame;
    zCViewWindow* m_pWindow;
    zCViewWindow* m_pInnerWindow;
    zCMenuEnum2 m_exitState;
    zSTRING name;
    zCMusicTheme* m_musicTheme;
    int m_mainSel;
    int m_screenInitDone;
    int m_use_dimx;
    int m_use_dimy;
    int m_use_posx;
    int m_use_posy;
    zCView* m_pViewInfo;
    zCVob* cursorVob;
    int eventOccured[MENU_EVENT_MAX];
    int cursorEn;
    int noneSelectable;
    int registeredCPP;
    int updateTimer;
    float fxTimer;
    int forceSelAction;
    zSTRING forceSelAction_S;
    zCMenuItem* forceSelActionItem;
    int forcedSelAction;
    zCArray<zCMenuItem*> m_listItems;

    static zCParser* menuParser;
    static zCTimer* timer;
    static zCArraySort<zCMenu*> menuList;
    static zCArray<zCMenu*> activeList;
    static zCSoundFX* browseSnd;
    static zCSoundFX* selectSnd;
    static zCSoundFX* escSnd;
    static zCVob* soundSourceVob;
    static zCWorld* world;
    static zCCamera* camera;
    static int inGameMenu;
    static zCMenu* active;
    static void(*EnterCallback)();
    static void(*LeaveCallback)();
    static void(*FrameCallback)();
};

#endif
