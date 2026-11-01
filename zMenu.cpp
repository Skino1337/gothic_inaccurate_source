#include "include/zMenu.h"
#include "include/oGame.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/zSound.h"
#include "include/zMusic.h"
#include "include/zMoving.h"
#include "include/zConsole.h"
#include <string.h>
#include <windows.h>

void sysEvent();

extern zCOption* zgameoptions;

const zSTRING zMENU_SEPARATOR = "SEPARATOR";
zSTRING DEF_DAT_FILE = "intro.dat";
static int menuState = -1;
static int menuListState = -1;

zCParser* zCMenu::menuParser;
extern zCParser* parserMenu;
zCTimer* zCMenu::timer;
zCArraySort<zCMenu*> zCMenu::menuList;
zCArray<zCMenu*> zCMenu::activeList;
zCSoundFX* zCMenu::browseSnd;
zCSoundFX* zCMenu::selectSnd;
zCSoundFX* zCMenu::escSnd;
zCVob* zCMenu::soundSourceVob;
zCWorld* zCMenu::world;
zCCamera* zCMenu::camera;
int zCMenu::inGameMenu;
zCMenu* zCMenu::active;

int zCMenu::CompareNames(const void* first, const void* second)
{
    const zCMenu* firstMenu = *static_cast<zCMenu* const*>(first);
    const zCMenu* secondMenu = *static_cast<zCMenu* const*>(second);
    int result = strcmp(firstMenu->name.ToChar(), secondMenu->name.ToChar());
    return result < 0 ? -1 : result > 0 ? 1 : 0;
}

void zCMenu::CreateParser()
{
    zoptions->ChangeDir(8);

    if (!menuParser)
        menuParser = new zCParser(100);
    parserMenu = menuParser;
}

void zCMenu::Startup()
{
    zSTRING menuFile;

    if (zgameoptions)
        menuFile = zgameoptions->ReadString("FILES", "Menu", "");

    if (menuFile.IsEmpty())
        menuFile = "System\\Menu";

    menuParser->Parse(menuFile + ".src");
    menuParser->CreatePCode();
    menuList.Compare = CompareNames;

    if (zsound) {
        browseSnd = zsound->LoadSoundFXScript("MENU_BROWSE");
        selectSnd = zsound->LoadSoundFXScript("MENU_SELECT");
        escSnd = zsound->LoadSoundFXScript("MENU_ESC");
    }

    timer = new zCTimer();
    zCMenuItem::Startup();
}

void zCMenu::Shutdown()
{
    while (menuList.numInArray) {
        zCMenu* menu = menuList.array[menuList.numInArray - 1];
        menu->m_listItems.DeleteList();
        delete menu;
    }

    zCMenuItem::Shutdown();

    if (browseSnd) {
        browseSnd->Release();
        browseSnd = NULL;
    }
    if (selectSnd) {
        selectSnd->Release();
        selectSnd = NULL;
    }
    if (escSnd) {
        escSnd->Release();
        escSnd = NULL;
    }

    delete timer;
    timer = NULL;
}

zCMenu::zCMenu()
{
    InitValues();
    name = "MENU_UNDEFINED";
    name.Upper();
}

zCMenu::zCMenu(const zSTRING& menuName)
{
    InitValues();
    name = menuName;
    name.Upper();
    registeredCPP = 1;
    SetByScript(name);
}

zCMenu::~zCMenu()
{
    for (int i = 0; i < m_listItems.numInArray; i++) {
        if (m_listItems.parray[i]) {
            m_listItems.parray[i]->Release();
            m_listItems.parray[i] = NULL;
        }
    }

    m_listItems.DeleteList();

    for (int i = 0; i < menuList.numInArray; i++) {
        if (menuList.array[i] == this) {
            for (int j = i + 1; j < menuList.numInArray; j++)
                menuList.array[j - 1] = menuList.array[j];
            menuList.numInArray--;
            break;
        }
    }

    if (m_musicTheme)
    {
        m_musicTheme->Release();
        m_musicTheme = NULL;
    }
}

void zCMenu::InitValues()
{
    m_screenInitDone = 0;
    m_mainSel = -1;
    m_pViewInfo = NULL;
    cursorEn = 1;
    forcedSelAction = 0;
    noneSelectable = 0;
    registeredCPP = 0;
    updateTimer = 0;
    fxTimer = 0.0f;
    forceSelAction = 0;
    posx = 0;
    posy = 0;
    dimx = 8191;
    dimy = 8191;
    alpha = 255;
    eventTimerUpdateMSec = 1000;
    flags = 0;
    defaultOutGame = 0;
    defaultInGame = 0;
    m_pWindow = NULL;
    m_pInnerWindow = NULL;
    m_exitState = NONE;
    forceSelActionItem = NULL;
    backPic.Clear();
    backWorld.Clear();
    forceSelAction_S.Clear();
    name.Clear();
    musicThemeName.Clear();
    m_musicTheme = NULL;

    for (int i = 0; i < MAX_ITEMS; i++)
        itemID[i].Clear();

    ClearAllEvents();
}

void zCMenu::Release()
{
    SetEnableHandleEvent(0);

    if (!registeredCPP)
        delete this;
}

void zCMenu::DefineExternal(const zSTRING& name, int(*function)(), int returnType, int elements, ...)
{
    menuParser->DefineExternal(name, function, returnType, elements);
}

zCMenu* zCMenu::Create(const zSTRING& name)
{
    zCMenu* menu = GetByName(name);

    if (!menu)
        menu = new zCMenu(name);

    return menu;
}

void zCMenu::Register()
{
    if (!IsRegistered())
        menuList.InsertSort(this);
}

void zCMenu::Unregister()
{
    for (int i = 0; i < menuList.numInArray; i++) {
        if (menuList.array[i] == this) {
            for (int j = i + 1; j < menuList.numInArray; j++)
                menuList.array[j - 1] = menuList.array[j];
            menuList.numInArray--;
            return;
        }
    }
}

int zCMenu::IsRegistered()
{
    for (int i = 0; i < menuList.numInArray; i++) {
        if (menuList.array[i] == this)
            return 1;
    }

    return 0;
}

zCParser* zCMenu::GetParser()
{
    return menuParser;
}

zCMenu* zCMenu::GetByName(const zSTRING& menuName)
{
    zSTRING upperName = menuName;
    upperName.Upper();

    for (int i = 0; i < menuList.numInArray; i++) {
        if (menuList.array[i]->name == upperName)
            return menuList.array[i];
    }

    return NULL;
}

void zCMenu::SetByScript(const zSTRING& script)
{
    name = script;
    name.Upper();

    if (!menuParser->GetSymbol(name))
        return;

    for (int i = 0; i < MAX_ITEMS; i++)
        itemID[i].Clear();

    menuParser->CreateInstance(name, this);
    m_mainSel = -1;

    for (int i = 0; i < MAX_ITEMS; i++) {
        if (itemID[i].IsEmpty())
            continue;

        zCMenuItem* item = zCMenuItem::Create(itemID[i]);
        m_listItems.InsertEnd(item);

        if (m_mainSel == -1 && item->GetIsSelectable())
            m_mainSel = m_listItems.numInArray - 1;
    }

    Register();
}

void zCMenu::ClearAllEvents()
{
    for (int i = 0; i < MENU_EVENT_MAX; i++)
        eventOccured[i] = 0;
}

void zCMenu::PreviousOption()
{
    if (!m_listItems.numInArray || m_mainSel < 0)
        return;

    m_listItems.parray[m_mainSel]->SetIsActive(0);
    int selection = m_mainSel;

    for (int i = 0; i < m_listItems.numInArray; i++) {
        selection--;

        if (selection < 0)
            selection = m_listItems.numInArray - 1;

        if (m_listItems.parray[selection]->GetIsSelectable())
            break;
    }

    if (selection != m_mainSel) {
        HandleLeaveItem(m_listItems.parray[m_mainSel]);
        m_mainSel = selection;
        HandleEnterItem(m_listItems.parray[m_mainSel]);
    }

    if (cursorEn)
        m_listItems.parray[m_mainSel]->SetIsActive(1);
}

void zCMenu::NextOption()
{
    if (!m_listItems.numInArray || m_mainSel < 0)
        return;

    m_listItems.parray[m_mainSel]->SetIsActive(0);
    int selection = m_mainSel;

    for (int i = 0; i < m_listItems.numInArray; i++) {
        selection++;

        if (selection >= m_listItems.numInArray)
            selection = 0;

        if (m_listItems.parray[selection]->GetIsSelectable())
            break;
    }

    if (selection != m_mainSel) {
        HandleLeaveItem(m_listItems.parray[m_mainSel]);
        m_mainSel = selection;
        HandleEnterItem(m_listItems.parray[m_mainSel]);
    }

    if (cursorEn)
        m_listItems.parray[m_mainSel]->SetIsActive(1);
}

void zCMenu::RestartMusicTheme()
{
    if (!inGameMenu)
        zmusic->PlayThemeByScript(musicThemeName, 0, NULL);
}

void(*zCMenu::EnterCallback)();
void(*zCMenu::LeaveCallback)();
int __lastSelection;
static zCSoundFX* gameStartSound;
static int playGameStartSound = 1;

void zCMenu::Enter()
{
    if (EnterCallback)
        EnterCallback();

    if (!inGameMenu) {
        zmusic->PlayThemeByScript(musicThemeName, 0, NULL);

        if (!gameStartSound)
        {
            gameStartSound = zsound->LoadSoundFXScript("GAMESTART");
            if (gameStartSound)
                gameStartSound->SetIsFixed(1);
        }

        if (playGameStartSound) {
            zsound->PlaySound(gameStartSound, 0);
            playGameStartSound = 0;
        }
    }

    ScreenInit();

    if (m_mainSel != -1)
        HandleEnterItem(m_listItems.parray[m_mainSel]);

    zinput->ClearKeyBuffer();
}

void zCMenu::Leave()
{
    if (m_mainSel != -1)
        HandleLeaveItem(m_listItems.parray[m_mainSel]);

    ScreenDone();

    if (LeaveCallback)
        LeaveCallback();

    zinput->ClearKeyBuffer();

    if (gameStartSound) {
        gameStartSound->Release();
        gameStartSound = NULL;
    }
}

int zCMenu::Run()
{
    m_exitState = NONE;
    activeList.InsertEnd(this);
    Activate();
    Enter();

    while (!m_exitState) {
        do {
            m_exitState = NONE;
            do {
                HandleFrame(1);
            } while (!m_exitState);
        } while (!HandleAction());
    }

    Leave();

    for (int i = 0; i < activeList.numInArray; i++) {
        if (activeList.parray[i] == this) {
            for (int j = i + 1; j < activeList.numInArray; j++)
                activeList.parray[j - 1] = activeList.parray[j];
            activeList.numInArray--;
            break;
        }
    }

    return m_exitState == GO;
}

int zCMenu::HandleEvent(int key)
{
    int alternate = 0;

    if (key == 524)
        key = 28;
    else if (key == 525)
        key = 1;
    else if (key == 523 || key == 15)
        key = 208;
    else if (key == 57) {
        key = 205;
        alternate = 1;
    }
    else if (key == 522)
        key = 200;

    switch (key) {
    case 1:
        if (escSnd)
            zsound->PlaySound(escSnd, 0);
        eventOccured[MENU_EVENT_LEAVE] = 1;
        m_exitState = BACK;
        break;
    case 28:
        if (m_mainSel >= 0) {
            if (selectSnd)
                zsound->PlaySound(selectSnd, 0);
            HandleActivateItem(m_listItems.parray[m_mainSel]);
            eventOccured[MENU_EVENT_EXECUTE] = 1;
            m_exitState = GO;
        }
        break;
    case 200:
        PreviousOption();
        if (browseSnd)
            zsound->PlaySound(browseSnd, 0);
        eventOccured[MENU_EVENT_SEL_PREV] = 1;
        __lastSelection = ztimer.totalTime;
        break;
    case 203:
        if (m_mainSel >= 0)
            m_listItems.parray[m_mainSel]->ToggleValue(-1, alternate);
        break;
    case 205:
        if (m_mainSel >= 0)
            m_listItems.parray[m_mainSel]->ToggleValue(1, alternate);
        break;
    case 208:
        NextOption();
        if (browseSnd)
            zsound->PlaySound(browseSnd, 0);
        eventOccured[MENU_EVENT_SEL_NEXT] = 1;
        __lastSelection = ztimer.totalTime;
        break;
    }

    updateTimer += timer->frameTime;

    if (updateTimer > eventTimerUpdateMSec) {
        updateTimer = 0;
        eventOccured[MENU_EVENT_TIMER] = 1;
    }

    return 1;
}

void zCMenu::ForceSelAction(int action, const zSTRING& target, zCMenuItem* item)
{
    m_exitState = action == 1 ? BACK : GO;
    forcedSelAction = 1;
    forceSelAction = action;
    forceSelAction_S = target;
    forceSelActionItem = item;
}

void zCMenu::Activate()
{
    if (m_pInnerWindow) {
        SetEnableHandleEvent(1);
        SetHandleEventTop();
    }
    for (int i = 0; i < m_listItems.numInArray; ++i)
    {
        zCMenuItem* item = m_listItems.parray[i];
        if (!item->m_parHideIfOptionSectionSet.IsEmpty())
            item->m_bDontRender = zoptions->ReadInt(item->m_parHideIfOptionSectionSet,
                item->m_parHideIfOptionSet, item->m_parHideOnValue) != item->m_parHideOnValue;
    }
}

void zCMenu::UpdateWindowGeometry()
{
    m_use_dimx = dimx;
    m_use_dimy = dimy;
    m_use_posx = posx;
    m_use_posy = posy;

    if (flags & 8) {
        m_use_dimx = int(640.0f / screen->nax(8192) * dimx);
        m_use_dimy = int(480.0f / screen->nay(8192) * dimy);
    }

    if (flags & 32) {
        m_use_posx = 4096 - m_use_dimx / 2;
        m_use_posy = 4096 - m_use_dimy / 2;
    }
    else if (flags & 8) {
        m_use_posx = int(640.0f / screen->nax(8192) * posx);
        m_use_posy = int(480.0f / screen->nay(8192) * posy);
    }
}

void zCMenu::HandleFrame(int key)
{
    int render = key;

    if ((flags & 8) && m_pInnerWindow) {
        UpdateWindowGeometry();
        m_pInnerWindow->SetPos(m_use_posx, m_use_posy);
        m_pInnerWindow->SetSize(m_use_dimx, m_use_dimy);
        m_pInnerWindow->RecalcChildsSize();
        m_pInnerWindow->RecalcChildsPos();
        render = 1;
    }

    sysEvent();
    zCInputCallback::GetInput();

    if (render)
        Render();
}

int zCMenu::HandleAction()
{
    if (!m_exitState || m_exitState == BACK)
        return m_exitState;
    if (m_exitState == FINISHED)
        return 1;

    int handled = 0;
    zCMenuItem* item = m_mainSel >= 0 ? m_listItems.parray[m_mainSel] : NULL;

    if (forcedSelAction) {
        forcedSelAction = 0;
        handled = HandleSelAction(forceSelAction, forceSelAction_S, forceSelActionItem);
    }
    else if (item) {
        for (int i = 0; i < MAX_SEL_ACTIONS && item->GetSelActionType(i); i++) {
            handled |= HandleSelAction(item->GetSelActionType(i), item->GetSelActionString(i), item);

            if (item->HasBeenCanceled())
                break;

            HandleFrame(0);
        }

        if (!handled && !item->HasBeenCanceled()) {
            for (int event = MENU_EVENT_EXECUTE; event < MENU_EVENT_MAX; event += 2) {
                if (eventOccured[event] && item->GetEventFunc(event))
                    handled |= int(menuParser->CallFunc(item->GetEventFunc(event)));
                HandleFrame(0);
            }
        }
    }

    ClearAllEvents();
    SetEnableHandleEvent(1);
    return handled;
}

int zCMenu::HandleSelAction(int action, const zSTRING& target, zCMenuItem* item)
{
    switch (action) {
    case 1:
        m_exitState = BACK;
        return 1;
    case 2: {
        zCMenu* menu = GetByName(target);
        if (!menu) {
            menu = new zCMenu();
            menu->SetByScript(target);
        }

        int exclusive = menu->flags & 2;
        if (exclusive)
            Leave();
        int result = menu->Run();
        menu->SetEnableHandleEvent(0);
        if (!menu->registeredCPP)
            menu->Release();
        if (exclusive)
            Activate();
        return result;
    }
    case 3: {
        zCMenuItem* newItem = zCMenuItem::Create(target);
        if (newItem) {
            newItem->InsertInWin(m_pInnerWindow);
            int result = newItem->Run();
            eventOccured[MENU_EVENT_CHANGED] = newItem->HasBeenCanceled() ? 0 : 1;
            newItem->RemoveFromWin();
            newItem->Release();
            return result;
        }
        return 0;
    }
    case 4: {
        zoptions->WriteString("INTERNAL", "menuAction", target, 0);

        if (target == "NEW_GAME" && !inGameMenu) {
            static zCSoundFX* newGameSound;
            if (!newGameSound)
            {
                newGameSound = zsound->LoadSoundFXScript("NEWGAME");
                if (newGameSound)
                    newGameSound->SetIsFixed(1);
            }
            zsound->PlaySound(newGameSound, 0);
        }
        return 1;
    }
    case 5: {
        int index = 1;
        while (true) {
            zSTRING command = target.PickWord(index, ";", "");
            if (command.IsEmpty())
                break;
            command.Upper();
            zcon.Evaluate(command);
            index++;
        }
        return 1;
    }
    case 6: {
        zCSoundFX* sound = zsound->LoadSoundFXScript(target);
        if (sound) {
            zsound->PlaySound(sound, 0);
            sound->Release();
        }
        return 1;
    }
    case 7:
        return ExecCommand(target);
    }

    return 0;
}

int zCMenu::ExecCommand(const zSTRING& command)
{
    command.PickWord(1, " ", "");
    command.PickWord(2, " ", "");
    return 0;
}

void zCMenu::RedrawItems()
{
    for (int i = 0; i < m_listItems.numInArray; i++)
        m_listItems.parray[i]->DrawFront();
}

void zCMenu::PrintInformation(zSTRING text)
{
    if (!m_pViewInfo)
        return;

    m_pViewInfo->SetFontColor(zCOLOR(255, 255, 255, 255));
    m_pViewInfo->ClrPrintwin();
    m_pViewInfo->Printwin(text);
    m_pViewInfo->DrawItems();
}

void zCMenu::ScreenInit()
{
    if (m_screenInitDone)
        return;

    m_screenInitDone = 1;
    UpdateWindowGeometry();

    if (!m_pWindow) {
        m_pWindow = new zCViewWindow(0, 0, 8192, 8192);
        screen->InsertItem(m_pWindow);
    }

    if (!m_pInnerWindow) {
        m_pInnerWindow = new zCViewWindow(m_use_posx, m_use_posy,
            m_use_posx + m_use_dimx, m_use_posy + m_use_dimy);
        m_pWindow->InsertItem(m_pInnerWindow);
    }

    m_pInnerWindow->SetPos(m_use_posx, m_use_posy);
    m_pInnerWindow->SetSize(m_use_dimx, m_use_dimy);
    m_pInnerWindow->SetTransparency(alpha);

    if (!backPic.IsEmpty())
        m_pInnerWindow->InsertBack(backPic);

    if (flags & 64) {
        m_pViewInfo = new zCView(0, 0, 100, 100);
        m_pInnerWindow->InsertItem(m_pViewInfo);
    }

    m_mainSel = inGameMenu ? defaultInGame : defaultOutGame;

    if (m_mainSel >= 0 && m_mainSel < m_listItems.numInArray &&
            !m_listItems.parray[m_mainSel]->GetIsSelectable())
        m_mainSel = -1;

    for (int i = 0; i < m_listItems.numInArray; i++) {
        m_listItems.parray[i]->UpdateFX();
        m_listItems.parray[i]->InsertInWin(m_pInnerWindow);

        if (m_mainSel == -1 && m_listItems.parray[i]->GetIsSelectable())
            m_mainSel = i;
    }

    if (m_mainSel >= 0 && m_mainSel < m_listItems.numInArray && cursorEn)
        m_listItems.parray[m_mainSel]->SetIsActive(1);

    SetEnableHandleEvent(1);
    SetHandleEventTop();
}

void zCMenu::ScreenDone()
{
    if (!m_screenInitDone)
        return;

    m_screenInitDone = 0;

    for (int i = 0; i < m_listItems.numInArray; i++)
        m_listItems.parray[i]->Close();

    SetEnableHandleEvent(0);

    for (int i = 0; i < m_listItems.numInArray; i++)
        m_listItems.parray[i]->RemoveFromWin();

    if (m_pViewInfo) {
        m_pWindow->RemoveItem(m_pViewInfo);
        delete m_pViewInfo;
        m_pViewInfo = NULL;
    }

    m_pWindow->RemoveItem(m_pInnerWindow);
    delete m_pInnerWindow;
    m_pInnerWindow = NULL;
    screen->RemoveItem(m_pWindow);
    delete m_pWindow;
    m_pWindow = NULL;
}

void(*zCMenu::FrameCallback)();

void zCMenu::Render()
{
    for (int i = 0; i < m_listItems.numInArray; i++)
        m_listItems.parray[i]->UpdateFX();

    if (!inGameMenu)
        Sleep(50);

    if (FrameCallback) {
        FrameCallback();
        timer->ResetTimer();
        return;
    }

    zCOLOR black(0);
    zrenderer->Vid_Clear(black, 3);
    zrenderer->BeginFrame();
    screen->Render();
    zrenderer->EndFrame();
    zrenderer->Vid_Blit(1, NULL, NULL);
    zsound->DoSoundUpdate();
    timer->ResetTimer();
}

zSTRING DEF_DAT_FILE;

zSTRING& zCMenu::GetBackWorld()
{
    return backWorld;
}

zSTRING& zCMenu::GetBackDatFile()
{
    return DEF_DAT_FILE;
}

zCMenu* zCMenu::GetActive()
{
    if (!activeList.numInArray)
        return NULL;

    return activeList.parray[activeList.numInArray - 1];
}

zCMenuItem* zCMenu::GetActiveItem()
{
    return m_listItems.parray[m_mainSel];
}

int zCMenu::GetNumberOfActiveItem()
{
    if (m_mainSel < 0 || !m_listItems.parray[m_mainSel]->GetIsSelectable())
        return -1;

    int result = 0;
    for (int i = 0; i <= m_mainSel; i++) {
        if (m_listItems.parray[i]->GetIsSelectable())
            result++;
    }

    return result;
}

int zCMenu::SetActiveItem(zCMenuItem* item)
{
    if (!item->GetIsSelectable())
        return 0;

    for (int i = 0; i < m_listItems.numInArray; i++)
        m_listItems.parray[i]->SetIsActive(0);

    item->SetIsActive(1);
    m_mainSel = -1;

    for (int i = 0; i < m_listItems.numInArray; i++) {
        if (m_listItems.parray[i] == item) {
            m_mainSel = i;
            break;
        }
    }

    return 1;
}

zSTRING& zCMenu::GetName()
{
    return name;
}

void zCMenu::SetName(const zSTRING& newName)
{
    name = newName;
    name.Upper();
}

int zCMenu::HandleEnterItem(zCMenuItem* item)
{
    return 1;
}

int zCMenu::HandleLeaveItem(zCMenuItem* item)
{
    return 1;
}

int zCMenu::HandleActivateItem(zCMenuItem* item)
{
    return 1;
}
