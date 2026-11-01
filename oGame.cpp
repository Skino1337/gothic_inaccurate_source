#include "include/oGame.h"
#include "include/oFocus.h"
#include "include/oAiHuman.h"
#include "include/oTrigger.h"
#include "include/zConsole.h"
#include "include/zParticle.h"
#include <cstdio>

static zCClassDef oCTriggerChangeLevelClassDef("oCTriggerChangeLevel", "zCTrigger",
    oCTriggerChangeLevel::_CreateNewInstance, 0, 0x190, 0, "");
zCClassDef* oCTriggerChangeLevel::classDef = &oCTriggerChangeLevelClassDef;

static zCConsole* game_aiConsole;
static int game_con;
static int game_spawnTime;
static int game_insertRange;
static int game_removeRange;
static int game_scriptSkip;
static int game_messagesSkip;
static int game_baseEnabled;

// EXE: 0x008B21C8
int oCGame::s_bUseOnlineHelp = 1;
// EXE: 0x00AB0884
oCGame* ogame = 0;
static int routineState = -1;
static int routineRuntimeState = -1;
static int gameState = -1;
static int gameRuntimeState = -1;
static int gameInteractionState = -1;
static zSTRING gamePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING gamePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING gamePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING gamePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING gamePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING gamePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING gamePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING gamePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING gamePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING gamePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float gameFarDistance = 250000.0f * 0.0166666675f;
static float gameNearDistance = gameFarDistance * 0.0166666675f;
static float gameFarPlaneDistance = 250000.0f * 24.0f;
static zSTRING routineRuntimePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING routineRuntimePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING routineRuntimePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING routineRuntimePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING routineRuntimePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING routineRuntimePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING routineRuntimePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING routineRuntimePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING routineRuntimePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING routineRuntimePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float routineRuntimeFarDistance = 250000.0f * 0.0166666675f;
static float routineRuntimeNearDistance = routineRuntimeFarDistance * 0.0166666675f;
static float routineRuntimeFarPlaneDistance = 250000.0f * 24.0f;
static zSTRING routinePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING routinePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING routinePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING routinePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING routinePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING routinePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING routinePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING routinePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING routinePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING routinePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static float routineFarDistance = 250000.0f * 0.0166666675f;
static float routineNearDistance = routineFarDistance * 0.0166666675f;
static float routineFarPlaneDistance = 250000.0f * 24.0f;
oCRtnManager rtnMan;

void oUlfi_StartUp()
{
}

void oUlfi_ShutDown()
{
}

oCVob* SearchVobByInstance(zCTree<zCVob>* node, int& instance)
{
    if (!node)
        return 0;
    oCVob* vob = dynamic_cast<oCVob*>(node->data);
    if (vob) {
        zSTRING* name = 0;
        int vobInstance = -1;
        if (vob->GetScriptInstance(name, vobInstance) && vobInstance == instance)
            return vob;
    }
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next) {
        oCVob* result = SearchVobByInstance(child, instance);
        if (result)
            return result;
    }
    return 0;
}

oCGame::oCGame()
    : cliprange(1600.0f), fogrange(400.0f), inScriptStartup(0), inLoadSaveGame(0), inLevelChange(0),
      savegameManager(0), game_text(0), load_screen(0), save_screen(0), pause_screen(0), hpBar(0),
      swimBar(0), manaBar(0), focusBar(0), showPlayerStatus(1), game_drawall(1), game_frameinfo(0),
      game_showaniinfo(0), game_showwaynet(0), game_testmode(0), game_editwaynet(0),
      game_showtime(0), game_showranges(0), drawWayBoxes(0), scriptStartup(1), showFreePoints(0),
      showRoutineNpc(0), loadNextLevel(0), gameInfo(0), pl_light(0), pl_lightval(5000.0f),
      wldTimer(0), timeStep(-1.0f), singleStep(0), guilds(0), infoman(0), newsman(0), svmman(0),
      trademan(0), portalman(0), spawnman(0), music_delay(0.0f), watchnpc(0), m_bWorldEntered(0),
      m_fEnterWorldTimer(0.0f), initial_hour(0), initial_minute(0), debugChannels(0),
      debugAllInstances(0), oldRoutineDay(-1), currentObjectRoutine(0), progressBar(0)
{
    for (int i = 0; i < GAME_VIEW_MAX; ++i) {
        array_view[i] = 0;
        array_view_visible[i] = 0;
        array_view_enabled[i] = 1;
    }
    startpos.n[0] = 0.0f;
    startpos.n[1] = 0.0f;
    startpos.n[2] = 0.0f;
    objRoutineList.data = 0;
    objRoutineList.next = 0;
    game_testmode = zoptions->Parm("DEVMODE");
    SetEnableHandleEvent(1);
    debugChannels = zoptions->ReadBool(zOPT_SEC_INTERNAL, "debugAllChannels", 0) ? -1 : 0;
    debugAllInstances = zoptions->ReadBool(zOPT_SEC_INTERNAL, "debugAllInstances", 0);
    if (!debugChannels) {
        zSTRING channels = zoptions->ReadString(zOPT_SEC_INTERNAL, "debugChannels", "");
        for (int i = 1; ; ++i) {
            zSTRING word = channels.PickWord(i, ",; ", "\"");
            if (word.IsEmpty())
                break;
            int channel = word.ToInt();
            if (channel >= 1 && channel <= 32)
                debugChannels |= 1 << (channel - 1);
        }
    }
}

void oCGame::SetDrawWaynet(int enabled)
{
    game_showwaynet = enabled;
}

int oCGame::GetDrawWaynet()
{
    return game_showwaynet;
}

zVEC3 oCGame::GetStartPos()
{
    return startpos;
}

oCGame::~oCGame()
{
    objRoutineList.DeleteListDatas();
    currentObjectRoutine = 0;
    delete guilds;
    delete infoman;
    delete newsman;
    delete svmman;
    delete trademan;
    delete portalman;
    delete wldTimer;
    delete gameInfo;
    delete spawnman;
    guilds = 0;
    infoman = 0;
    newsman = 0;
    svmman = 0;
    trademan = 0;
    portalman = 0;
    wldTimer = 0;
    gameInfo = 0;
    spawnman = 0;
    if (game_text)
        screen->RemoveItem(game_text);
    if (hpBar)
        screen->RemoveItem(hpBar);
    if (swimBar)
        screen->RemoveItem(swimBar);
    if (manaBar)
        screen->RemoveItem(manaBar);
    if (focusBar)
        screen->RemoveItem(focusBar);
    delete game_text;
    delete hpBar;
    delete swimBar;
    delete manaBar;
    delete focusBar;
    game_text = 0;
    hpBar = 0;
    swimBar = 0;
    manaBar = 0;
    focusBar = 0;
    for (int i = 0; i < GAME_VIEW_MAX; ++i) {
        if (array_view[i] && array_view_visible[i] && array_view_enabled[i]) {
            array_view_visible[i] = 0;
            array_view[i]->Close();
        }
        delete array_view[i];
        array_view[i] = 0;
    }
    if (pl_light) {
        pl_light->Release();
        pl_light = 0;
    }
    oCarsten_ShutDown();
    oCNpcFocus::ExitFocusModes();
    Done();
    if (ogame == this)
        ogame = 0;
}

void oCGame::TObjectRoutine::Release()
{
    delete this;
}

oCGame::TObjectRoutine::~TObjectRoutine()
{
}

void oCGame::WorldInit()
{
    GetWorld()->bspTree.drawVobBBox3D = 0;
    wldTimer = new oCWorldTimer;
    initial_hour = 8;
    initial_minute = 0;
    if (zoptions->Parm("TIME")) {
        zSTRING time = zoptions->ParmValue("TIME");
        initial_hour = time.PickWord(1, " :", zSTR_SKIP).ToInt();
        initial_minute = time.PickWord(3, " :", zSTR_SKIP).ToInt();
    }
    rtnMan.SetWorld(GetWorld());
}

void oCGame::CamInit()
{
    zCSession::CamInit();
}

void oCGame::CamInit(zCVob* vob, zCCamera* camera)
{
    zCSession::CamInit(vob, camera);
}

void oCGame::SetCameraPosition()
{
    zCVob* cameraVob = GetCameraVob();
    zCWorld* world = GetWorld();
    if (!cameraVob || !world)
        return;
    cameraVob->SetCollDetStat(0);
    cameraVob->SetCollDetDyn(0);
    world->RemoveVob(cameraVob);
    world->AddVob(cameraVob);
    cameraVob->SetAI((zCAIBase*)GetCameraAI());
    cameraVob->SetSleeping(0);
}

void oCGame::DesktopInit()
{
    screen->SetFont(0);
    zCSession::DesktopInit();
    game_text = new zCView(0, 0, 100, 100, 0);
    screen->InsertItem(game_text);
    game_text->SetAutoScroll((float)zoptions->ReadInt(zOPT_SEC_GAME, "gametextAutoScroll", 1000));
    game_text->SetFont(0);
    array_view[GAME_VIEW_SCREEN] = new zCView(0, 0, 100, 100, 2);
    array_view[GAME_VIEW_CONVERSATION] = new zCView(0, 0, 100, 100, 2);
    array_view[GAME_VIEW_AMBIENT] = new zCView(0, 0, 100, 100, 2);
    array_view[GAME_VIEW_CINEMA] = new zCView(0, 0, 100, 100, 2);
    array_view[GAME_VIEW_NOISE] = new zCView(0, 0, 100, 100, 2);
    array_view[GAME_VIEW_CHOICE] = new zCView(0, 0, 100, 100, 2);
    array_view[GAME_VIEW_CONVERSATION]->InsertBack("dlg_conversation.tga");
    array_view[GAME_VIEW_AMBIENT]->InsertBack("dlg_ambient.tga");
    array_view[GAME_VIEW_NOISE]->InsertBack("dlg_noise.tga");
    array_view[GAME_VIEW_CHOICE]->InsertBack("dlg_choice.tga");
    UpdateViewSettings();
    if (!array_view_visible[GAME_VIEW_SCREEN] && array_view_enabled[GAME_VIEW_SCREEN]) {
        array_view_visible[GAME_VIEW_SCREEN] = 1;
        array_view[GAME_VIEW_SCREEN]->Open();
    }
    for (int i = 0; i < GAME_VIEW_MAX; ++i) {
        if (array_view[i])
            array_view[i]->SetFont(0);
    }
    if (array_view_visible[GAME_VIEW_AMBIENT] && array_view_enabled[GAME_VIEW_AMBIENT]) {
        array_view_visible[GAME_VIEW_AMBIENT] = 0;
        array_view[GAME_VIEW_AMBIENT]->Close();
    }
    array_view_enabled[GAME_VIEW_AMBIENT] = 0;
    zCView::PrintDebug("<Textfenster>\n");
    zCView::stdoutput = game_text;
}

void oCGame::UpdateViewSettings()
{
    for (int i = 0; i < GAME_VIEW_MAX; ++i) {
        if (!array_view[i])
            continue;
        int animated = i == GAME_VIEW_CONVERSATION || i == GAME_VIEW_CHOICE || i == GAME_VIEW_NOISE;
        array_view[i]->animatedWindows = animated;
        array_view[i]->animatedAlpha = animated;
    }
}

void oCGame::NpcInit()
{
    if (GetWorld())
        NpcInit(&GetWorld()->globalVobTree);
    for (zCListSort<oCNpc>* node = GetGameWorld()->voblist_npcs->next; node; node = node->next) {
        oCNpc* npc = node->data;
        if (!npc->IsDead() || !npc->IsAPlayer())
            rtnMan.CreateWayBoxes(npc);
    }
}

void oCGame::NpcInit(zCTree<zCVob>* node)
{
    if (!node)
        return;
    if (node->data && node->data->type == 2)
        node->data->SetSleepingMode(2);
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        NpcInit(child);
}

void oCGame::Done()
{
    oCNpc::ReleaseStatics();
    objRoutineList.DeleteListDatas();
    currentObjectRoutine = 0;
    delete guilds;
    delete infoman;
    delete newsman;
    delete svmman;
    delete trademan;
    delete portalman;
    delete wldTimer;
    delete gameInfo;
    delete spawnman;
    guilds = 0;
    infoman = 0;
    newsman = 0;
    svmman = 0;
    trademan = 0;
    portalman = 0;
    wldTimer = 0;
    gameInfo = 0;
    spawnman = 0;
    if (game_text)
        screen->RemoveItem(game_text);
    if (hpBar)
        screen->RemoveItem(hpBar);
    if (swimBar)
        screen->RemoveItem(swimBar);
    if (manaBar)
        screen->RemoveItem(manaBar);
    if (focusBar)
        screen->RemoveItem(focusBar);
    delete game_text;
    delete hpBar;
    delete swimBar;
    delete manaBar;
    delete focusBar;
    game_text = 0;
    hpBar = 0;
    swimBar = 0;
    manaBar = 0;
    focusBar = 0;
    for (int i = 0; i < GAME_VIEW_MAX; ++i) {
        if (array_view[i] && array_view_visible[i] && array_view_enabled[i]) {
            array_view_visible[i] = 0;
            array_view[i]->Close();
        }
        delete array_view[i];
        array_view[i] = 0;
    }
    if (pl_light) {
        pl_light->Release();
        pl_light = 0;
    }
    oCarsten_ShutDown();
    oCNpcFocus::ExitFocusModes();
    savegameManager->ClearCurrent();
    zCSession::Done();
}

void oCGame::Init()
{
    oCarsten_StartUp();
    zCSession::Init();
    sysEvent();
    SetWorld(zfactory->CreateWorld());
    zcon.SetWorld(GetWorld());
    zcon.SetParser(&parser);
    WorldInit();
    savegameManager->ClearCurrent();
    zSTRING script = zgameoptions
        ? zgameoptions->ReadString(zOPT_SEC_FILES, "Game", "Content\\Gothic")
        : zoptions->ReadString(zOPT_SEC_INTERNAL, "gameScript", "Content\\Gothic");
    LoadParserFile(script + ".src");
    sysEvent();
    oCNpc::InitStatics();
    guilds = new oCGuilds;
    infoman = new oCInfoManager(&parser);
    newsman = new oCNewsManager;
    svmman = new oCSVMManager;
    trademan = new oCTradeManager(&parser);
    portalman = new oCPortalRoomManager;
    spawnman = new oCSpawnManager;
    hpBar = new oCViewStatusBar;
    screen->InsertItem(hpBar, 0);
    hpBar->Init(100, 7800, 1.0f);
    hpBar->SetTextures("BAR_back.tga", "BAR_tempmax.tga", "BAR_health.tga", zSTR_EMPTY);
    hpBar->SetMaxRange(0.0f, 30.0f);
    screen->RemoveItem(hpBar);
    swimBar = new oCViewStatusBar;
    screen->InsertItem(swimBar, 0);
    swimBar->Init(2900, 7800, 1.0f);
    swimBar->SetTextures("BAR_back.tga", "BAR_tempmax.tga", "BAR_misc.tga", zSTR_EMPTY);
    screen->RemoveItem(swimBar);
    manaBar = new oCViewStatusBar;
    screen->InsertItem(manaBar, 0);
    manaBar->Init(5700, 7800, 1.0f);
    manaBar->SetTextures("BAR_back.tga", "BAR_tempmax.tga", "BAR_mana.tga", zSTR_EMPTY);
    manaBar->SetMaxRange(0.0f, 30.0f);
    screen->RemoveItem(manaBar);
    focusBar = new oCViewStatusBar(0, 0, 1000, 1000, 2);
    screen->InsertItem(focusBar, 0);
    focusBar->Init(500, 500, 0.75f);
    focusBar->SetTextures("BAR_back.tga", "BAR_tempmax.tga", "BAR_health.tga", zSTR_EMPTY);
    screen->RemoveItem(focusBar);
    oCNpcFocus::InitFocusModes();
    sysEvent();
    if (progressBar)
        progressBar->SetPercent(0, zSTR_EMPTY);
    if (zinput->GetDeviceConnected((zTInputDevice)2))
        zinput->SetDeviceEnabled((zTInputDevice)2, 1);
    if (zinput->GetDeviceConnected((zTInputDevice)3))
        zinput->SetDeviceEnabled((zTInputDevice)3, 1);
    zCPar_Symbol* symbol = parser.GetSymbol("VIEW_TIME_PER_CHAR");
    if (symbol)
        symbol->GetValue(zCView::s_fViewTimePerChar, 0);
}

void oCGame::SetGameInfo(oCGameInfo* info)
{
    if (gameInfo != info)
        delete gameInfo;
    gameInfo = info;
    if (!gameInfo) {
        gameInfo = new oCGameInfo;
        gameInfo->Init();
    }
}

void oCGame::CallScriptStartup()
{
    if (!scriptStartup)
        return;
    int globalInit = parser.GetIndex("STARTUP_GLOBAL");
    if (globalInit <= 0)
        return;
    inScriptStartup = 1;
    parser.CallFunc(globalInit);
    if (GetWorld()) {
        int worldInit = parser.GetIndex("STARTUP_" + GetWorld()->GetWorldName());
        if (worldInit > 0) {
            parser.progressBar = progressBar;
            parser.CallFunc(worldInit);
            parser.progressBar = 0;
        }
    }
    inScriptStartup = 0;
}

void oCGame::CallScriptInit()
{
    int globalInit = parser.GetIndex("INIT_GLOBAL");
    if (globalInit <= 0)
        return;
    inScriptStartup = 1;
    parser.CallFunc(globalInit);
    if (GetWorld()) {
        int worldInit = parser.GetIndex("INIT_" + GetWorld()->GetWorldName());
        if (worldInit > 0) {
            parser.progressBar = progressBar;
            parser.CallFunc(worldInit);
            parser.progressBar = 0;
        }
    }
    inScriptStartup = 0;
}

oCViewGothicProgressBar::~oCViewGothicProgressBar()
{
}

void oCViewGothicProgressBar::Init()
{
    zCViewProgressBar::Init();
    InsertBack("MENU_PROGRESSBAR.TGA");
    SetAlphaBlendFunc(zRND_ALPHA_FUNC_BLEND);
    SetTransparency(255);
}

void oCViewGothicProgressBar::HandleChange()
{
    int width = (int)(percent * 0.01f * 5792.0f);
    if (!bar)
    {
        bar = new zCView(0, 0, 1000, 1000, 1);
        InsertItem(bar);
        bar->InsertBack("MENU_PROGRESSBAR_FILL.TGA");
        bar->SetPos(1200, 1200);
    }
    bar->SetSize(width, 5792);
    bar->ClrPrintwin();
    Draw();
}

void oCGame::OpenSavescreen(bool showProgress)
{
    if (save_screen)
        CloseSavescreen();
    if (!showProgress) {
        save_screen = 0;
        return;
    }
    save_screen = new zCView(4096 - screen->anx() / 2, 4096 - screen->any() / 2,
        4096 + screen->anx() / 2, 4096 + screen->any() / 2, 2);
    screen->InsertItem(save_screen);
    save_screen->InsertBack("saving.tga");
    progressBar = new oCViewGothicProgressBar;
    progressBar->minStep = 3;
    save_screen->InsertItem(progressBar, 0);
    progressBar->Init();
    screen->DrawItems();
    zrenderer->Vid_Blit();
    if (GetWorld())
        GetWorld()->progressBar = progressBar;
}

void oCGame::CloseSavescreen()
{
    delete progressBar;
    delete save_screen;
    save_screen = 0;
    progressBar = 0;
    if (GetWorld())
        GetWorld()->progressBar = 0;
}

void oCGame::OpenLoadscreen(bool showProgress, zSTRING worldName)
{
    if (load_screen)
        CloseLoadscreen();
    load_screen = new zCView(0, 0, 8192, 8192, 2);
    screen->InsertItem(load_screen);
    worldName.Lower();
    load_screen->InsertBack(worldName.IsEmpty() ? zSTRING("loading.tga")
        : "loading_" + worldName + ".tga");
    progressBar = new oCViewGothicProgressBar;
    progressBar->minStep = 3;
    load_screen->InsertItem(progressBar, 0);
    progressBar->Init();
    screen->DrawItems();
    zrenderer->Vid_Blit();
    if (GetWorld())
        GetWorld()->progressBar = progressBar;
}

void oCGame::CloseLoadscreen()
{
    delete progressBar;
    delete load_screen;
    load_screen = 0;
    progressBar = 0;
    if (GetWorld())
        GetWorld()->progressBar = 0;
}


oCWorld* oCGame::GetGameWorld()
{
    return dynamic_cast<oCWorld*>(GetWorld());
}

oCNpc* oCGame::GetSelfPlayerVob()
{
    return oCNpc::player;
}

zCView* oCGame::GetTextView()
{
    return game_text;
}

oCWorldTimer* oCGame::GetWorldTimer()
{
    return wldTimer;
}

oCInfoManager* oCGame::GetInfoManager()
{
    return infoman;
}

oCNewsManager* oCGame::GetNewsManager()
{
    return newsman;
}

oCSVMManager* oCGame::GetSVMManager()
{
    return svmman;
}

oCTradeManager* oCGame::GetTradeManager()
{
    return trademan;
}

oCGuilds* oCGame::GetGuilds()
{
    return guilds;
}

oCGameInfo* oCGame::GetGameInfo()
{
    return gameInfo;
}

oCPortalRoomManager* oCGame::GetPortalRoomManager()
{
    return portalman;
}

oCSpawnManager* oCGame::GetSpawnManager()
{
    return spawnman;
}

oHEROSTATUS oCGame::GetHeroStatus()
{
    if (oCNpc::player && oCNpc::player->callback_ai)
        return (oHEROSTATUS)((oCAIHuman*)oCNpc::player->callback_ai)->GetEnemyThreat();
    return oHEROSTATUS_NORMAL;
}

void oCGame::SetShowNews(oCNpc* npc)
{
    for (zCListSort<oCNpc>* node = GetGameWorld()->voblist_npcs->next; node; node = node->next)
        node->data->SetShowNews(node->data == npc);
}

void oCGame::SetShowPlayerStatus(int enabled)
{
    if (!enabled) {
        screen->RemoveItem(hpBar);
        screen->RemoveItem(swimBar);
        screen->RemoveItem(manaBar);
        screen->RemoveItem(focusBar);
    }
    showPlayerStatus = enabled;
}

int oCGame::GetShowPlayerStatus()
{
    return showPlayerStatus;
}

void oCGame::UpdateScreenResolution()
{
    screen->SetFont(0);
    int fontY = screen->FontY();
    int width = screen->anx();
    int height = screen->any();
    int centerX = (8192 - width) / 2;
    int centerY = (8192 - height) / 2;
    game_text->SetPos(centerX, fontY * 2);
    game_text->SetSize(width, fontY * 6 + fontY / 4);
    array_view[GAME_VIEW_SCREEN]->SetPos(0, 0);
    array_view[GAME_VIEW_SCREEN]->SetSize(8192, 8192);
    array_view[GAME_VIEW_CONVERSATION]->SetPos(centerX, height);
    array_view[GAME_VIEW_CONVERSATION]->SetSize(width, centerX);
    array_view[GAME_VIEW_AMBIENT]->SetPos(centerX, centerY);
    array_view[GAME_VIEW_AMBIENT]->SetSize(width, fontY * 5);
    array_view[GAME_VIEW_CINEMA]->SetPos(centerX, 8192 - centerX - height);
    array_view[GAME_VIEW_CINEMA]->SetSize(width, centerY);
    array_view[GAME_VIEW_NOISE]->SetPos(centerX, centerY - fontY * 5);
    array_view[GAME_VIEW_NOISE]->SetSize(width, 8192 - centerX - height);
    array_view[GAME_VIEW_CHOICE]->SetPos(centerX, height * 2 + fontY);
    array_view[GAME_VIEW_CHOICE]->SetSize(width, centerY - fontY * 5);
    screen->InsertItem(hpBar, 0);
    screen->InsertItem(swimBar, 0);
    screen->InsertItem(manaBar, 0);
    screen->InsertItem(focusBar, 0);
    hpBar->Init(screen->anx(), 8192 - height, 1.0f);
    manaBar->Init(8192 - screen->anx(), 8192 - height, 1.0f);
    swimBar->Init(0, 0, 1.0f);
    int swimWidth;
    int swimHeight;
    swimBar->GetSize(swimWidth, swimHeight);
    swimBar->SetPos((8192 - swimWidth) / 2, 8192 - height);
    focusBar->Init(0, 0, 0.75f);
    int focusWidth;
    int focusHeight;
    focusBar->GetSize(focusWidth, focusHeight);
    focusBar->SetPos((8192 - focusWidth) / 2, screen->any());
    screen->RemoveItem(hpBar);
    screen->RemoveItem(swimBar);
    screen->RemoveItem(manaBar);
    screen->RemoveItem(focusBar);
}

void oCGame::UpdatePlayerStatus()
{
    if (!oCNpc::player || !showPlayerStatus)
        return;
    screen->RemoveItem(hpBar);
    screen->RemoveItem(swimBar);
    screen->RemoveItem(manaBar);
    screen->RemoveItem(focusBar);
    if (GetWorld() && GetWorld()->csPlayer->GetPlayingGlobalCutscene())
        return;
    int nutrition = 0;
    int manaPreview = 0;
    if (oCNpc::player->inventory2.IsOpen()) {
        oCItem* item = oCNpc::player->inventory2.GetSelectedItem();
        if (item) {
            nutrition = item->nutrition;
            for (int i = 0; i < 3; ++i) {
                if (!item->change_atr[i])
                    nutrition += item->change_value[i];
                if (item->change_atr[i] == 2)
                    manaPreview += item->change_value[i];
            }
        }
    }
    screen->InsertItem(hpBar, 0);
    float hpMaximum = (float)oCNpc::player->GetAttribute(1);
    float hp = (float)oCNpc::player->GetAttribute(0);
    hpBar->SetMaxRange(0.0f, hpMaximum);
    hpBar->SetRange(0.0f, hpMaximum);
    hpBar->SetPreview(hp + nutrition);
    hpBar->SetValue(hp);
    if (oCNpc::player->anictrl && oCNpc::player->anictrl->walkmode == 5) {
        float swimTime;
        float diveTime;
        float diveCounter;
        oCNpc::player->GetSwimDiveTime(swimTime, diveTime, diveCounter);
        screen->InsertItem(swimBar, 0);
        swimBar->SetMaxRange(0.0f, diveTime);
        swimBar->SetRange(0.0f, diveTime);
        swimBar->SetValue(diveCounter);
    }
    float manaMaximum = (float)oCNpc::player->GetAttribute(3);
    if ((oCNpc::player->inventory2.IsOpen() || oCNpc::player->GetWeaponMode() == 7) &&
        manaMaximum > 0.0f) {
        float mana = (float)oCNpc::player->GetAttribute(2);
        screen->InsertItem(manaBar, 0);
        manaBar->SetMaxRange(0.0f, manaMaximum);
        manaBar->SetRange(0.0f, manaMaximum);
        manaBar->SetPreview(mana + manaPreview);
        manaBar->SetValue(mana);
    }
}

void oCGame::RemoveHelperVobs(zCTree<zCVob>* node)
{
    for (zCTree<zCVob>* current = node; current; current = current->next) {
        zCVob* vob = current->data;
        if (vob) {
            if (vob->type == 1) {
                vob->SetVisual(0);
                vob->showVisual = 1;
            } else if (vob->type == 2 || (vob->type >= 4 && vob->type <= 7)) {
                vob->showVisual = 0;
                vob->SetCollDetDyn(0);
                vob->SetCollDetStat(0);
            }
        }
        RemoveHelperVobs(current->firstChild);
    }
}

void oCGame::CreateVobList(zCList<zCVob>& list, zCTree<zCVob>* node, zTVobType type)
{
    for (zCTree<zCVob>* current = node; current; current = current->next) {
        if (current->data && current->data->type == type)
            list.Insert(current->data);
        CreateVobList(list, current->firstChild, type);
    }
}

void oCGame::SetAsPlayer(const zSTRING& instanceName)
{
    int instance = parser.GetIndex(instanceName);
    if (!GetWorld())
        return;
    oCVob* vob = SearchVobByInstance(&GetWorld()->globalVobTree, instance);
    if (!vob) {
        for (zCListSort<oCNpc>* node = GetGameWorld()->voblist_npcs->next; node; node = node->next) {
            if (node->data->GetName() == instanceName) {
                vob = node->data;
                if (!vob->GetHomeWorld())
                    GetWorld()->AddVob(vob);
                break;
            }
        }
    }
    if (vob) {
        oCNpc::player = (oCNpc*)vob;
        oCNpc::player->SetAsPlayer();
    }
}

void oCGame::SetupPlayers(oCNpc*& hero, const zSTRING& instanceName)
{
    if (progressBar)
        progressBar->SetPercent(0, zSTR_EMPTY);
    startpos.n[0] = 0.0f;
    startpos.n[1] = 150.0f;
    startpos.n[2] = 0.0f;
    zSTRING playerInstance = instanceName;
    if (playerInstance.IsEmpty()) {
        playerInstance = zgameoptions
            ? zgameoptions->ReadString(zOPT_SEC_SETTINGS, "Player", "PC_Hero")
            : zoptions->ReadString(zOPT_SEC_INTERNAL, "playerInstanceName", "PC_Hero");
    }
    if (!hero) {
        hero = (oCNpc*)GetGameWorld()->CreateVob((zTVobType)0x82, playerInstance);
        if (hero)
            hero->AddRef();
    }
    if (progressBar)
        progressBar->SetPercent(90, zSTR_EMPTY);
    hero->SetCollDetStat(0);
    hero->SetCollDetDyn(0);
    hero->GetModel();
    if (!hero->GetHomeWorld())
        GetWorld()->AddVob(hero);
    hero->SetOnFloor(startpos);
    hero->ResetXZRotationsWorld();
    hero->SetCollDetStat(1);
    hero->SetCollDetDyn(1);
    hero->Release();
    oCPlayerInfo* playerInfo = dynamic_cast<oCPlayerInfo*>(zCPlayerInfo::GetActivePlayer());
    if (!playerInfo) {
        playerInfo = zfactory->CreatePlayerInfo();
        gameInfo->AddPlayer(playerInfo);
        playerInfo->SetPlayerVob(hero);
        playerInfo->SetActive();
    }
    oCNpc::player = hero;
    oCNpc::player->SetAsPlayer();
    if (progressBar)
        progressBar->SetPercent(100, zSTR_EMPTY);
}

static int playerLightInt;

void oCGame::SetRanges()
{
    if (GetCamera())
        GetCamera()->SetFarClipZ(cliprange);
    zrenderer->SetFogRange(fogrange, GetCamera() ? GetCamera()->farClipZ : cliprange, 0);
    zrenderer->SetFog(1);
}

void oCGame::SetRangesByCommandLine()
{
    if (zoptions->Parm("CLIPRANGE"))
        cliprange = (float)zoptions->ParmValueInt("CLIPRANGE");
    if (zoptions->Parm("FOGRANGE"))
        fogrange = (float)zoptions->ParmValueInt("FOGRANGE");
    playerLightInt = zoptions->ReadInt(zOPT_SEC_GAME, "cameraLightRange", 0);
    if (zoptions->Parm("CAMLIGHTRANGE")) {
        playerLightInt = zoptions->ParmValueInt("CAMLIGHTRANGE");
        zoptions->WriteInt(zOPT_SEC_GAME, "cameraLightRange", playerLightInt, 1);
    }
    if (pl_light) {
        if (zoptions->Parm("LIGHTRANGE"))
            pl_lightval = (float)zoptions->ParmValueInt("LIGHTRANGE");
        pl_light->range = pl_lightval;
        pl_light->SetRange(pl_lightval, 1);
    }
    SetRanges();
}

void oCGame::EnvironmentInit()
{
    if (GetWorld()->bspTree.bspTreeMode) {
        cliprange = 60000.0f;
        fogrange = 15000.0f;
        playerLightInt = 0;
        GetCamera()->SetFarClipZ(60000.0f);
        zrenderer->SetFogRange(60000.0f, GetCamera()->farClipZ, 0);
        zrenderer->SetFog(1);
        zrenderer->SetFogColor(zCOLOR(100, 100, 100, 255));
    } else {
        if (pl_light)
            pl_light->Release();
        pl_light = new zCVobLight;
        zCClassDef::ObjectCreated(pl_light, zCVobLight::classDef);
        pl_light->Move(100.0f, 200.0f, 200.0f);
        pl_light->SetRange(100.0f, 1);
        pl_light->lightData.lightColor = zCOLOR(155, 200, 255, 255);
        pl_light->lightData.isStatic = 0;
        playerLightInt = 5000;
    }
    SetRangesByCommandLine();
}

int oCGame::LoadParserFile(const zSTRING& fileName)
{
    zSTRING upperName = fileName;
    upperName.Upper();
    parser.Reset();
    DefineExternals_Ulfi(&parser);
    parser.EnableTreeLoad(0);
    parser.EnableTreeSave(0);
    parser.Parse(fileName);
    parser.AddClassOffset("C_NPC", 0x120);
    parser.AddClassOffset("C_ITEM", 0x120);
    parser.CreatePCode();
    return !parser.Error();
}

void oCGame::SetTime(int day, int hour, int minute)
{
    int currentDay;
    int currentHour;
    int currentMinute;
    wldTimer->GetTime(currentHour, currentMinute);
    currentDay = wldTimer->GetDay();
    int newDay = currentDay + day / 24;
    wldTimer->SetDay(newDay);
    wldTimer->SetTime(hour, minute);
    rtnMan.SetDailyRoutinePos(1);
    SetObjectRoutineTimeChange(currentDay, newDay, hour, minute);
    if (spawnman)
        spawnman->SpawnImmediately(1);
}

void oCGame::GetTime(int& day, int& hour, int& minute)
{
    day = wldTimer->GetDay();
    wldTimer->GetTime(hour, minute);
}

int oCGame::LoadWorld(const zSTRING& fileName, zCWorld::zTWorldLoadMode loadMode)
{
    if (!GetWorld())
        return 0;
    int result = GetWorld()->LoadWorld(fileName, loadMode);
    RemoveHelperVobs(GetWorld()->globalVobTree.firstChild);
    return result;
}

void oCGame::SaveWorld(const zSTRING& fileName, zCWorld::zTWorldSaveMode saveMode,
    int flags, int saveGame)
{
    oCNpc* savedPlayer = oCNpc::player;
    zMAT4 playerTrafo;
    if (savedPlayer) {
        playerTrafo = savedPlayer->trafoObjToWorld;
        savedPlayer->AddRef();
        savedPlayer->ClearPerceptionLists();
        if (GetWorld())
            GetWorld()->RemoveVob(savedPlayer);
        oCNpc::player = 0;
    }
    if (GetWorld())
        GetWorld()->SaveWorld(fileName, saveMode, flags, saveGame);
    if (savedPlayer) {
        if (GetWorld() && !savedPlayer->GetHomeWorld())
            GetWorld()->AddVob(savedPlayer);
        savedPlayer->SetTrafoObjToWorld(playerTrafo);
        savedPlayer->SetOnFloor(startpos);
        savedPlayer->SetCollDetStat(1);
        savedPlayer->SetCollDetDyn(1);
        oCNpc::player = savedPlayer;
        savedPlayer->Release();
    }
}

oCNpc* oCGame::RemovePlayerFromWorld()
{
    oCNpc* removedPlayer = oCNpc::player;
    if (removedPlayer) {
        removedPlayer->AddRef();
        removedPlayer->ClearPerceptionLists();
        if (GetWorld())
            GetWorld()->RemoveVob(removedPlayer);
        oCNpc::player = 0;
    }
    return removedPlayer;
}

void oCGame::InsertPlayerIntoWorld(oCNpc* insertedPlayer, zMAT4& trafo)
{
    if (!insertedPlayer || insertedPlayer->GetHomeWorld() || !GetWorld())
        return;
    GetWorld()->AddVob(insertedPlayer);
    insertedPlayer->SetCollDetStat(0);
    insertedPlayer->SetCollDetDyn(0);
    insertedPlayer->SetTrafoObjToWorld(trafo);
    zVEC3 position = {
        trafo.m[0][3], trafo.m[1][3], trafo.m[2][3]
    };
    insertedPlayer->SetOnFloor(position);
    insertedPlayer->SetCollDetStat(1);
    insertedPlayer->SetCollDetDyn(1);
    oCNpc::player = insertedPlayer;
}

void oCGame::PreSaveGameProcessing(int levelChange)
{
    for (zCListSort<oCNpc>* node = GetGameWorld()->voblist_npcs->next; node; node = node->next)
        node->data->PreSaveGameProcessing();
    for (zCListSort<zCVob>* node = GetGameWorld()->voblist->next; node; node = node->next) {
        oCMobFire* fire = dynamic_cast<oCMobFire*>(node->data);
        if (fire)
            fire->PreSave();
    }
    oCVisualFX::PreSaveGameProcessing(!levelChange);
}

void oCGame::PostSaveGameProcessing()
{
    for (zCListSort<oCNpc>* node = GetGameWorld()->voblist_npcs->next; node; node = node->next)
        node->data->PostSaveGameProcessing();
    if (!inLevelChange) {
        for (zCListSort<zCVob>* node = GetGameWorld()->voblist->next; node; node = node->next) {
            oCMobFire* fire = dynamic_cast<oCMobFire*>(node->data);
            if (fire)
                fire->PostSave();
        }
    }
    oCVisualFX::PostSaveGameProcessing();
}

void oCGame::WriteSavegame(int slot, int showScreen)
{
    if (showScreen)
        OpenSavescreen(1);
    if (progressBar) {
        progressBar->SetPercent(0, zSTR_EMPTY);
        progressBar->SetRange(12, 92);
    }
    if (slot >= 0 && slot <= 20)
        savegameManager->CopyFromCurrent(slot);
    PreSaveGameProcessing(inLevelChange);
    if (GetWorld())
        SaveWorld("savegame.sav", zCWorld::zWORLD_SAVE_DEFAULT, 0, 1);
    PostSaveGameProcessing();
    if (progressBar)
        progressBar->ResetRange();
    SetCameraPosition();
    if (progressBar)
        progressBar->SetPercent(100, zSTR_EMPTY);
    if (showScreen)
        CloseSavescreen();
}

int oCGame::CheckObjectConsistency(int expectedNpcs)
{
    zCClassDef* npcClass = zCClassDef::GetClassDef("oCNpc");
    if (npcClass && npcClass->numLivingObjects > expectedNpcs)
        npcClass->ReportLeaks();
    if (oCNews::GetRemainingNews() > 0)
        oCNews::CheckRemainingNews();
    return 1;
}

void oCGame::ClearGameState()
{
    parser.ResetGlobalVars();
    parser.SetInstance("SELF", 0);
    parser.SetInstance("OTHER", 0);
    parser.SetInstance("VICTIM", 0);
    parser.SetInstance("ITEM", 0);
    if (spawnman)
        spawnman->ClearList();
    delete infoman;
    infoman = new oCInfoManager(&parser);
    misMan.ClearMissions();
    oCLogManager::GetLogManager()->Clear();
    delete newsman;
    newsman = new oCNewsManager;
    zsound->StopAllSounds();
    oCVisualFX::PreSaveGameProcessing(1);
    oCVisualFX::PostSaveGameProcessing();
    CamInit();
    oCVob::ClearDebugList();
    if (portalman)
        portalman->CleanUp();
    objRoutineList.DeleteListDatas();
    currentObjectRoutine = 0;
}

static void InitNpcAttitudesTree(oCGame* game, zCTree<zCVob>* node)
{
    if (!node)
        return;
    if (node->data && node->data->type == 2 && node->data != oCNpc::player) {
        oCNpc* npc = (oCNpc*)node->data;
        int attitude = game->guilds->GetAttitude(oCNpc::player->GetTrueGuild(), npc->GetTrueGuild());
        npc->SetTmpAttitude(attitude);
        npc->SetAttitude(attitude);
    }
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        InitNpcAttitudesTree(game, child);
}

void oCGame::InitNpcAttitudes()
{
    if (oCNpc::player && GetWorld())
        InitNpcAttitudesTree(this, &GetWorld()->globalVobTree);
}

void oCGame::RecurseCacheVobs(zCTree<zCVob>* node)
{
    if (!node)
        return;
    if (node->data && node->data->visual) {
        visualList.InsertEnd(node->data->visual);
        node->data->visual->AddRef();
    }
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        RecurseCacheVobs(child);
}

void oCGame::CacheVisualsIn()
{
    visualList.DeleteList();
    if (!GetWorld())
        return;
    if (GetWorld()->globalVobTree.data && GetWorld()->globalVobTree.data->visual) {
        visualList.InsertEnd(GetWorld()->globalVobTree.data->visual);
        GetWorld()->globalVobTree.data->visual->AddRef();
    }
    RecurseCacheVobs(GetWorld()->globalVobTree.firstChild);
}

void oCGame::CacheVisualsOut()
{
    for (int i = 0; i < visualList.numInArray; ++i) {
        if (visualList.parray[i])
            visualList.parray[i]->Release();
    }
    visualList.DeleteList();
}

void oCGame::LoadGame(int slot, const zSTRING& fileName)
{
    ClearGameState();
    OpenLoadscreen(1, fileName);
    if (progressBar) {
        progressBar->SetPercent(0, zSTR_EMPTY);
        progressBar->SetRange(0, 92);
    }
    LoadWorld(fileName, zCWorld::zWORLD_LOAD_DEFAULT);
    if (progressBar) {
        progressBar->ResetRange();
        progressBar->SetRange(92, 100);
    }
    EnterWorld(oCNpc::player, 1, "");
    if (progressBar)
        progressBar->ResetRange();
    if (slot == -2)
        InitNpcAttitudes();
    SetTime(0, initial_hour, initial_minute);
    if (progressBar)
        progressBar->SetPercent(100, zSTR_EMPTY);
    CloseLoadscreen();
}

void oCGame::LoadSavegame(int slot, int showScreen)
{
    ClearGameState();
    if (showScreen)
        OpenLoadscreen(1, "savegame.sav");
    if (progressBar) {
        progressBar->SetPercent(0, zSTR_EMPTY);
        progressBar->SetRange(0, 5);
    }
    zSTRING worldName = "savegame.sav";
    InitWorldSavegame(slot, worldName);
    if (progressBar) {
        progressBar->ResetRange();
        progressBar->SetRange(10, 92);
    }
    LoadWorld(worldName, zCWorld::zWORLD_LOAD_DEFAULT);
    CacheVisualsOut();
    CheckObjectConsistency(1);
    if (progressBar) {
        progressBar->ResetRange();
        progressBar->SetRange(92, 98);
    }
    EnterWorld(oCNpc::player, 0, "");
    if (progressBar)
        progressBar->ResetRange();
    CallScriptInit();
    SetCameraPosition();
    if (progressBar)
        progressBar->SetPercent(100, zSTR_EMPTY);
    if (showScreen)
        CloseLoadscreen();
}

void oCGame::ChangeLevel(const zSTRING& worldName, const zSTRING& startPoint)
{
    parser.ClearAllInstanceRefs();
    if (zresMan) {
        zresMan->PurgeCaches(0);
        zresMan->cacheInImmediatelyMsec = 100000000.0f;
    }
    OpenLoadscreen(1, worldName);
    int musicEnabled = zoptions->ReadBool(zOPT_SEC_SOUND, "musicEnabled", 0);
    zCZoneMusic::SetAutochange(0);
    zCMusicSystem::DisableMusicSystem(1);
    zsound->StopAllSounds();
    oCNpc* savedPlayer = RemovePlayerFromWorld();
    zMAT4 playerTrafo;
    if (savedPlayer)
        playerTrafo = savedPlayer->trafoObjToWorld;
    oCNpc::dontArchiveThisNpc = savedPlayer;
    DeleteTorches();
    inLevelChange = 1;
    WriteSavegame(-1, 0);
    inLevelChange = 0;
    oCNpc::dontArchiveThisNpc = 0;
    spawnman->ClearList();
    CheckObjectConsistency(0);
    LoadWorld(worldName, zCWorld::zWORLD_LOAD_DEFAULT);
    if (savedPlayer) {
        InsertPlayerIntoWorld(savedPlayer, playerTrafo);
        savedPlayer->Release();
    }
    EnterWorld(oCNpc::player, 1, startPoint);
    CallScriptInit();
    for (zCListSort<oCNpc>* node = GetGameWorld()->voblist_npcs->next; node; node = node->next) {
        if (!node->data->IsDead())
            node->data->RefreshNpc();
    }
    DeleteTorches();
    loadNextLevel = 0;
    infoman->RestoreParserInstances();
    zCZoneMusic::SetAutochange(musicEnabled);
    zCMusicSystem::DisableMusicSystem(!musicEnabled);
    CloseLoadscreen();
    if (zresMan)
        zresMan->cacheInImmediatelyMsec = 1500.0f;
}

void oCGame::TriggerChangeLevel(const zSTRING& worldName, const zSTRING& startPoint)
{
    loadNextLevel = 1;
    loadNextLevelName = worldName;
    loadNextLevelStart = startPoint;
    if (oCNpc::player && oCNpc::player->GetWeaponMode() == 7)
        oCNpc::player->EV_ForceRemoveWeapon();
}

int oCGame::CheckIfSavegameExists(const zSTRING& fileName)
{
    FILE* file = fopen(fileName.ToChar(), "rb");
    if (!file)
        return 0;
    fclose(file);
    return 1;
}

void oCGame::RenderWaynet()
{
    if (game_showwaynet && GetWorld() && GetWorld()->wayNet)
        GetWorld()->wayNet->Draw(GetCamera());
}

void oCGame::ShowDebugInfos()
{
    if (game_frameinfo) {
        int fontY = GetViewport()->FontY();
        GetViewport()->Print(0, 0, zSTRING((long)vidGetFPSRate()) + " fps");
        zTRnd_Stats stats;
        zrenderer->GetStatistics(stats);
        GetViewport()->Print(0, fontY, zSTRING((long)stats.numAlphaPolys) + " tris");
    }
    if (game_showranges) {
        int fontY = GetViewport()->FontY();
        GetViewport()->Print(0, 1200 + fontY * 2,
            "farclipz: " + zSTRING((long)zCCamera::activeCam->farClipZ));
        zCSkyControler* sky = zCCamera::activeCam->connectedVob->GetHomeWorld()->GetActiveSkyControler();
        GetViewport()->Print(0, 1200 + fontY * 3, "userFogFar: " + zSTRING((long)sky->GetFarZ()));
    }
    if (game_showtime) {
        zSTRING time = wldTimer->GetTimeString();
        screen->Print(8191 - screen->FontSize(time), 0, time);
    }
    if (game_showaniinfo && oCNpc::player) {
        zCModel* playerModel = oCNpc::player->GetModel();
        if (playerModel) {
            playerModel->PrintStatus(6000, 0);
            playerModel->ShowAniList(6200);
        }
    }
    if (game_species_con && game_species_con->IsVisible())
        game_species_con->Update();
    if (showRoutineNpc)
        rtnMan.ShowRoutine(showRoutineNpc);
}

void oCGame::Render()
{
    if (game_testmode || !m_bWorldEntered) {
        zrenderer->BeginFrame();
        oCItemContainer::Container_PrepareDraw();
        if (wldTimer)
            wldTimer->Timer();
        if (GetWorld()) {
            zCSkyControler* sky = GetWorld()->GetActiveSkyControler();
            if (sky)
                sky->SetTime(wldTimer->GetSkyTime());
            GetWorld()->Render(GetCamera());
        }
        oCVisualFX::SetupAIForAllFXInWorld();
        ztimer.FrameUpdate();
        if (spawnman)
            spawnman->CheckInsertNpcs();
        oCNpc::ProcessAITimer();
        if (loadNextLevel)
            ChangeLevel(loadNextLevelName, loadNextLevelStart);
        if (game_drawall) {
            RenderWaynet();
            ShowDebugInfos();
            UpdatePlayerStatus();
            screen->DrawItems();
            oCItemContainer::Container_Draw();
            if (drawWayBoxes)
                rtnMan.DrawWayBoxes();
            if (showFreePoints)
                ShowFreePoints();
            zCViewDraw::Render(zCViewDraw::GetScreen());
        }
        oCVob::ShowVobDebugInfo(GetCamera());
        zrenderer->Vid_Unlock();
        oCarsten_PerFrame();
        CheckObjectRoutines();
        if (GetWorld())
            GetWorld()->AdvanceClock(timeStep);
        zrenderer->EndFrame();
        if (singleStep)
            timeStep = 0.0f;
        return;
    }

    if (GetWorld()) {
        GetWorld()->AdvanceClock(timeStep);
        GetCamera()->Activate();
        GetWorld()->MoveVobs();
    }
    ztimer.FrameUpdate();
    if (zresMan)
        zresMan->DoFrameActivity();
    if (spawnman)
        spawnman->CheckInsertNpcs();
    m_fEnterWorldTimer += ztimer.frameTimeFloat;
    if (m_fEnterWorldTimer > 2500.0f) {
        m_bWorldEntered = 0;
        m_fEnterWorldTimer = 0.0f;
    }
}

void oCGame::Pause(int showScreen)
{
    if (singleStep)
        return;
    if (showScreen && !pause_screen)
        pause_screen = new zCView(100, 100, 2000, 500, 2);
    singleStep = 1;
    timeStep = 0.0f;
}

void oCGame::Unpause()
{
    if (!singleStep)
        return;
    delete pause_screen;
    pause_screen = 0;
    singleStep = 0;
    timeStep = -1.0f;
}

static void DeleteNpcsTree(oCGame* game, zCTree<zCVob>* node, oCNpc* first, oCNpc* second,
    float range)
{
    if (!node)
        return;
    if (node->data && node->data->type == 2 && node->data != first && node->data != second && second) {
        oCNpc* npc = (oCNpc*)node->data;
        float dx = second->trafoObjToWorld.m[0][3] - npc->trafoObjToWorld.m[0][3];
        float dy = second->trafoObjToWorld.m[1][3] - npc->trafoObjToWorld.m[1][3];
        float dz = second->trafoObjToWorld.m[2][3] - npc->trafoObjToWorld.m[2][3];
        if (dx * dx + dy * dy + dz * dz <= range * range)
            game->GetWorld()->RemoveVob(npc);
    }
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        DeleteNpcsTree(game, child, first, second, range);
}

void oCGame::DeleteNpcs(oCNpc* first, oCNpc* second, float range)
{
    for (zCListSort<oCNpc>* node = GetGameWorld()->voblist_npcs->next; node; node = node->next) {
        oCNpc* npc = node->data;
        if (npc == first || !second || npc == second)
            continue;
        float x = fabs(second->trafoObjToWorld.m[0][3] - npc->trafoObjToWorld.m[0][3]);
        float y = fabs(second->trafoObjToWorld.m[1][3] - npc->trafoObjToWorld.m[1][3]);
        float z = fabs(second->trafoObjToWorld.m[2][3] - npc->trafoObjToWorld.m[2][3]);
        float maximum = x > y ? x : y;
        if (z > maximum)
            maximum = z;
        float distance = maximum * 0.9375f + (x + y + z - maximum) * 0.375f;
        if (distance >= range) {
            npc->state.hasRoutine = 0;
            GetGameWorld()->RemoveVob(npc);
        }
    }
}

oCParticleControl* pfxc = 0;

void Game_Particle_Changed(const zSTRING& name)
{
    if (pfxc)
        pfxc->UpdateInternals();
}

void Game_OpenParticleConsole(zSTRING& name)
{
    if (!pfxc)
        return;
    pfxc->SetPFXName(name);
    pfxc->BeginEditActive();
}

void Game_CloseParticleConsole()
{
    if (!pfxc)
        return;
    pfxc->EndEditActive();
    delete pfxc;
}

void Game_DeleteAllPfx(zCTree<zCVob>* node)
{
    if (!node)
        return;
    if (node->data && dynamic_cast<zCParticleFX*>(node->data->visual))
        node->data->Release();
    Game_DeleteAllPfx(node->firstChild);
    Game_DeleteAllPfx(node->next);
}

void oCGame::LoadWorld(int saveGame, const zSTRING& fileName)
{
    if (fileName.IsEmpty())
        return;
    if (progressBar)
        progressBar->SetPercent(12, zSTR_EMPTY);
    if (saveGame == -2) {
        spawnman->ClearList();
        GetWorld()->DisposeWorld();
        if (progressBar)
            progressBar->SetRange(15, 97);
        LoadWorldStartup(fileName);
        if (progressBar)
            progressBar->ResetRange();
        if (!GetWorld()->compiled)
            CompileWorld();
    } else {
        if (!IsInWorld(fileName)) {
            if (progressBar)
                progressBar->SetRange(15, 40);
            LoadWorldStat(fileName);
            if (progressBar)
                progressBar->ResetRange();
        }
        if (!GetWorld()->compiled)
            CompileWorld();
        inLoadSaveGame = 1;
        if (progressBar)
            progressBar->SetRange(45, 97);
        LoadWorldDyn(fileName);
        if (progressBar)
            progressBar->ResetRange();
        inLoadSaveGame = 0;
    }
    if (GetWorld()->wayNet)
        GetWorld()->wayNet->CorrectHeight();
    if (dynamic_cast<zCSkyControler_Indoor*>(GetWorld()->GetActiveSkyControler()))
        zsound->SetGlobalReverbPreset(8, 0.15f);
    else
        zsound->SetGlobalReverbPreset(0, 0.0f);
    if (progressBar)
        progressBar->SetPercent(100, zSTR_EMPTY);
}

void oCGame::InitWorldSavegame(int& slot, zSTRING& worldName)
{
    if (savegameManager)
        savegameManager->CopyToCurrent(slot);
}

void oCGame::EnterWorld(oCNpc* playerNpc, int saveGame, const zSTRING& startPoint)
{
    if (progressBar) {
        progressBar->SetPercent(0, zSTR_EMPTY);
        progressBar->SetRange(0, 55);
    }
    if (!playerNpc) {
        SetupPlayers(playerNpc, startPoint);
    } else {
        oCPlayerInfo* playerInfo = dynamic_cast<oCPlayerInfo*>(zCPlayerInfo::GetActivePlayer());
        if (!playerInfo) {
            playerInfo = zfactory->CreatePlayerInfo();
            gameInfo->AddPlayer(playerInfo);
        }
        playerInfo->SetPlayerVob(playerNpc);
        playerInfo->SetActive();
    }
    oCNpc::player = playerNpc;
    if (progressBar)
        progressBar->ResetRange();
    CamInit();
    if (playerNpc && playerNpc->anictrl)
        playerNpc->anictrl->SetFightAnis(playerNpc->GetWeaponMode());
    if (progressBar)
        progressBar->SetPercent(60, zSTR_EMPTY);
    EnvironmentInit();
    if (progressBar)
        progressBar->SetPercent(75, zSTR_EMPTY);
    NpcInit();
    if (progressBar) {
        progressBar->SetPercent(96, zSTR_EMPTY);
        progressBar->SetPercent(100, zSTR_EMPTY);
    }
    CloseLoadscreen();
    m_fEnterWorldTimer = 0.0f;
    m_bWorldEntered = 1;
}

void oCGame::CompileWorld()
{
    if (!GetWorld() || GetWorld()->compiled)
        return;
    GetWorld()->CompileWorld(1, 0.0f, 0, 0, 0);
    GetWorld()->GenerateStaticWorldLighting(0, 0);
}

void oCGame::LoadWorldStartup(const zSTRING& fileName)
{
    LoadWorld(fileName, zCWorld::zWORLD_LOAD_DEFAULT);
    RemoveHelperVobs(GetWorld()->globalVobTree.firstChild);
    CallScriptStartup();
    CallScriptInit();
}

void oCGame::LoadWorldStat(zSTRING fileName)
{
    LoadWorld(fileName, zCWorld::zWORLD_LOAD_DEFAULT);
}

void oCGame::LoadWorldDyn(const zSTRING& fileName)
{
    LoadWorld(fileName, zCWorld::zWORLD_LOAD_DEFAULT);
    RemoveHelperVobs(GetWorld()->globalVobTree.firstChild);
    if (!inLoadSaveGame)
        CallScriptInit();
}

int oCGame::IsInWorld(const zSTRING& fileName)
{
    if (fileName.IsEmpty())
        return 0;
    return GetWorld() && fileName == GetWorld()->GetWorldName();
}

void oCGame::RenderBlit()
{
    zrenderer->Vid_Blit(1, 0, 0);
}

int oCGame::Sort_Routine(TObjectRoutine* first, TObjectRoutine* second)
{
    if (first->hour1 < second->hour1)
        return -1;
    if (first->hour1 == second->hour1 && first->min1 < second->min1)
        return -1;
    return 1;
}

void oCGame::InsertObjectRoutine(int stateNum, const zSTRING& objName, int hour, int minute, int type)
{
    TObjectRoutine* routine = new TObjectRoutine;
    routine->stateNum = stateNum;
    routine->objName = objName;
    routine->hour1 = hour;
    routine->min1 = minute;
    routine->type = type;
    zCList<TObjectRoutine>* entry = new zCList<TObjectRoutine>;
    entry->data = routine;
    entry->next = 0;
    if (!objRoutineList.next || Sort_Routine(routine, objRoutineList.next->data) < 0) {
        entry->next = objRoutineList.next;
        objRoutineList.next = entry;
        return;
    }
    zCList<TObjectRoutine>* current = objRoutineList.next;
    while (current->next && Sort_Routine(routine, current->next->data) >= 0)
        current = current->next;
    entry->next = current->next;
    current->next = entry;
}

void oCGame::ResetObjectRoutineList()
{
    currentObjectRoutine = 0;
    oldRoutineDay = -1;
}

void oCGame::ClearObjectRoutineList()
{
    objRoutineList.DeleteListDatas();
    currentObjectRoutine = 0;
}

void oCGame::CheckObjectRoutines()
{
    if (!currentObjectRoutine && oldRoutineDay != wldTimer->GetDay()) {
        currentObjectRoutine = objRoutineList.next;
        oldRoutineDay = wldTimer->GetDay();
    }
    while (currentObjectRoutine) {
        TObjectRoutine* routine = currentObjectRoutine->data;
        if (!wldTimer->IsLater(routine->hour1, routine->min1))
            break;
        if (!routine->type) {
            oCMobInter::SetAllMobsToState(routine->stateNum, GetWorld());
        } else if (routine->type == 1) {
            zCVob* vob = GetGameWorld()->SearchVobByName(routine->objName);
            if (vob) {
                if (!routine->stateNum)
                    vob->GetEM(0)->OnUntrigger(0, 0);
                else if (routine->stateNum == 1)
                    vob->GetEM(0)->OnTrigger(0, 0);
            }
        }
        currentObjectRoutine = currentObjectRoutine->next;
    }
}

void oCGame::SetObjectRoutineTimeChange(int oldDay, int newDay, int hour, int minute)
{
    int oldMinutes = (oldDay * 60 + hour) % 1440;
    int newMinutes = (newDay * 60 + minute) % 1440;
    if (newMinutes < oldMinutes)
        newMinutes += 1440;
    if (!currentObjectRoutine)
        currentObjectRoutine = objRoutineList.next;
    while (currentObjectRoutine) {
        TObjectRoutine* routine = currentObjectRoutine->data;
        int routineMinutes = routine->hour1 * 60 + routine->min1;
        if (routineMinutes >= oldMinutes && routineMinutes <= newMinutes) {
            if (!routine->type) {
                oCMobInter::SetAllMobsToState(routine->stateNum, GetWorld());
            } else if (routine->type == 1) {
                zCVob* vob = GetGameWorld()->SearchVobByName(routine->objName);
                if (vob) {
                    if (!routine->stateNum)
                        vob->GetEM(0)->OnUntrigger(0, 0);
                    else if (routine->stateNum == 1)
                        vob->GetEM(0)->OnTrigger(0, 0);
                }
            }
        }
        currentObjectRoutine = currentObjectRoutine->next;
    }
    oCMobInter::TriggerAllMobsToTmpState(GetWorld());
}

void oCGame::ToggleShowFreePoints()
{
    showFreePoints = !showFreePoints;
}

static void ShowFreePointsTree(zCTree<zCVob>* node)
{
    if (!node)
        return;
    zCVobSpot* spot = dynamic_cast<zCVobSpot*>(node->data);
    if (spot) {
        zCOLOR color = spot->IsAvailable() ? zCOLOR{ 0, 255, 255, 255 } : zCOLOR{ 0, 0, 255, 255 };
        spot->bbox3D.Draw(color);
    }
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        ShowFreePointsTree(child);
}

void oCGame::ShowFreePoints()
{
    if (showFreePoints && GetCameraVob() && GetWorld())
        ShowFreePointsTree(&GetWorld()->globalVobTree);
}

static void RefreshNpcsTree(zCTree<zCVob>* node)
{
    if (!node)
        return;
    if (node->data && node->data->type == 2)
        ((oCNpc*)node->data)->RefreshNpc();
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        RefreshNpcsTree(child);
}

void oCGame::RefreshNpcs()
{
    if (GetWorld())
        RefreshNpcsTree(&GetWorld()->globalVobTree);
    DeleteTorches();
}

static void DeleteTorchesTree(zCTree<zCVob>* node)
{
    if (!node)
        return;
    if (node->data && dynamic_cast<oCItem*>(node->data) && node->data->GetHomeWorld()) {
        oCItem* item = (oCItem*)node->data;
        if (item->scemeName == "ITLSTORCHBURNING" && node->data->callback_ai) {
            node->data->SetAI(0);
            node->data->GetHomeWorld()->RemoveVobSubtree(node->data);
        }
    }
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        DeleteTorchesTree(child);
}

void oCGame::DeleteTorches()
{
    if (GetWorld())
        DeleteTorchesTree(&GetWorld()->globalVobTree);
}

int game_testmode;
static zVEC3 game_pos;

void Game_CreateInstance(zSTRING& instanceName, zSTRING& result)
{
    int instance = parser.GetIndex(instanceName);
    if (instance < 0) {
        result = instanceName + "' not found.";
        return;
    }
    int base = instance;
    while (true) {
        int parent = parser.GetBase(base);
        if (parent == base)
            break;
        base = parent;
    }
    zVEC3 position = {
        oCNpc::player->trafoObjToWorld.m[0][3] + oCNpc::player->trafoObjToWorld.m[0][2] * 200.0f,
        oCNpc::player->trafoObjToWorld.m[1][3] + oCNpc::player->trafoObjToWorld.m[1][2] * 200.0f,
        oCNpc::player->trafoObjToWorld.m[2][3] + oCNpc::player->trafoObjToWorld.m[2][2] * 200.0f
    };
    if (base == parser.GetIndex("C_Item")) {
        oCItem* item = (oCItem*)ogame->GetGameWorld()->CreateVob((zTVobType)0x81, instance);
        item->SetPositionWorld(position);
        ogame->GetGameWorld()->AddVob(item);
        item->Release();
    } else if (base == parser.GetIndex("C_NPC")) {
        oCNpc* npc = (oCNpc*)ogame->GetGameWorld()->CreateVob((zTVobType)0x82, instance);
        npc->SetPositionWorld(position);
        int attitude = ogame->guilds->GetAttitude(oCNpc::player->GetTrueGuild(), npc->GetTrueGuild());
        npc->SetTmpAttitude(attitude);
        npc->SetAttitude(attitude);
        npc->GetModel()->StartAni("S_FALLDN", 0);
        npc->Release();
    } else {
        result = instanceName + "' not found.";
        return;
    }
    result = instanceName + "' inserted";
}

void Game_GotoWP(const zSTRING& waypoint, zSTRING& result)
{
    if (oCNpc::player->BeamTo(waypoint))
        result = "Player beamed to Waypoint " + waypoint;
    else
        result = "Way- or Freepoint not found.";
}

void Game_SaveSCPos()
{
    game_pos.n[0] = oCNpc::player->trafoObjToWorld.m[0][3];
    game_pos.n[1] = oCNpc::player->trafoObjToWorld.m[1][3];
    game_pos.n[2] = oCNpc::player->trafoObjToWorld.m[2][3];
}

void Game_LoadSCPos()
{
    oCNpc::player->SetCollDetStat(0);
    oCNpc::player->SetCollDetDyn(0);
    oCNpc::player->SetPositionWorld(game_pos);
    oCNpc::player->SetCollDetStat(1);
    oCNpc::player->SetCollDetDyn(1);
}

void Game_ToggleTestMode(int enabled)
{
    game_testmode = enabled;
    if (ogame)
        ogame->game_testmode = enabled;
}

void Game_ToggleHoldTime()
{
    static int holdTime;
    holdTime = !holdTime;
}

void Game_ParserDebug(zSTRING& functionName)
{
    parser.ShowCode(parser.GetIndex(functionName));
}

void Game_ParserDebugOff()
{
    parser.CloseViews();
}

void Game_PlaySound(zSTRING& soundName)
{
    zCSoundFX* sound = zsound->LoadSoundFXScript(soundName);
    if (!sound)
        return;
    zsound->PlaySound3D(sound, 0, 0, 0);
    sound->Release();
}

void Game_ToggleScreenMode()
{
    zrenderer->Vid_SetScreenMode(!zrenderer->Vid_GetScreenMode());
}

void Edit_Changed(const zSTRING& name)
{
    if (name == "ATTRIBUTE")
        oCNpc::player->CheckModelOverlays();
}

int oCGame::ConsoleEvalFunc(const zSTRING& commandLine, zSTRING& result)
{
    zSTRING command = commandLine.PickWord(1, " ", "");
    command.Upper();
    zSTRING argument = commandLine.PickWord(2, " ", "");
    argument.Upper();
    zSTRING parameter = commandLine.PickWord(3, " ", "");
    if (command == "GOTO") {
        Game_GotoWP(argument, result);
        return 1;
    }
    if (command == "INSERT") {
        Game_CreateInstance(argument, result);
        return 1;
    }
    if (command == "TOGGLE" && argument == "SCREENMODE") {
        Game_ToggleScreenMode();
        result = "ScreenMode switched.";
        return 1;
    }
    if (command == "TOGGLE" && argument == "DESKTOP") {
        ogame->game_drawall = !ogame->game_drawall;
        result = "Desktop switched.";
        return 1;
    }
    if (command == "TOGGLE" && argument == "WAYNET") {
        ogame->game_showwaynet = !ogame->game_showwaynet;
        result = "Waynet switched.";
        return 1;
    }
    if (command == "TOGGLE" && argument == "FRAME") {
        ogame->game_frameinfo = !ogame->game_frameinfo;
        result = "Frame info switched.";
        return 1;
    }
    if (command == "CHEAT" && argument == "FULL") {
        oCNpc::player->CompleteHeal();
        result = "Player healed.";
        return 1;
    }
    if (command == "CHEAT" && argument == "GOD") {
        oCNpc::godmode = !oCNpc::godmode;
        result = oCNpc::godmode ? "Godmode on." : "Godmode off.";
        return 1;
    }
    if (command == "SAVE" && argument == "GAME") {
        ogame->WriteSavegame(0, 1);
        result = "Game saved.";
        return 1;
    }
    if (command == "LOAD" && argument == "GAME") {
        ogame->LoadSavegame(0, 1);
        result = "Game loaded.";
        return 1;
    }
    if (command == "SAVE") {
        Game_SaveSCPos();
        result = "Position saved.";
        return 1;
    }
    if (command == "LOAD") {
        Game_LoadSCPos();
        result = "Position loaded.";
        return 1;
    }
    if (command == "PLAY" && argument == "SOUND") {
        Game_PlaySound(parameter);
        result = "Sound played.";
        return 1;
    }
    if (command == "PLAY" && argument == "ANI") {
        oCNpc::player->GetModel()->StartAni(parameter, 0);
        result = "Animation played.";
        return 1;
    }
    if (command == "KILL") {
        oCNpc* focus = oCNpc::player->GetFocusNpc();
        if (focus)
            focus->Die();
        result = "Focus NPC killed.";
        return 1;
    }
    if (command == "PARSERD") {
        Game_ParserDebug(argument);
        result = "Parser debug.";
        return 1;
    }
    result = "Unknown command.";
    return 1;
}

void* oCTriggerChangeLevel::operator new(unsigned int size)
{
    oCTriggerChangeLevel* trigger = static_cast<oCTriggerChangeLevel*>(::operator new(size));
    zCClassDef::ObjectCreated(trigger, classDef);
    return trigger;
}

void Game_StartParticleEffects(zCTree<zCVob>* node)
{
    if (!node)
        return;
    zCParticleFX* particle = dynamic_cast<zCParticleFX*>(node->data->visual);
    if (particle)
        particle->CreateParticles();
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        Game_StartParticleEffects(child);
}

void Game_InitConsole()
{
    zcon.AddEvalFunc(oCGame::ConsoleEvalFunc);
    zcon.Register("insert", "insert a script-instance [name]");
    zcon.Register("goto waypoint", "beam player to waypoint [name]");
    zcon.Register("toggle desktop", "desktop on/off");
    zcon.Register("toggle debug", "debug-Messages on/off");
    zcon.Register("toggle spy", "spy-Messages on/off");
    zcon.Register("toggle text", "text window on/off");
    zcon.Register("toggle waynet", "waynet on/off");
    zcon.Register("toggle frame", "frame-info on/off");
    zcon.Register("toggle screen", "fullscreen / window");
    zcon.Register("toggle aniinfo", "animation-info on/off");
    zcon.Register("toggle camera", "change camera mode on/off");
    zcon.Register("toggle turns", "turn player on/off");
    zcon.Register("toggle time", "show worldtime on/off");
    zcon.Register("toggle wayboxes", "wayboxes on/off");
    zcon.Register("toggle freepoints", "freepoints on/off");
    zcon.Register("toggle musiccontrol", "game music logic on/off");
    zcon.Register("edit combat", "fight console on/off");
    zcon.Register("edit abilities", "edit player-instance");
    zcon.Register("edit species", "edit species values");
    zcon.Register("edit focus", "edit focus object");
    zcon.Register("edit ai", "edit ai parameters");
    zcon.Register("set throw", "set throw range");
    zcon.Register("camera mode", "set camera mode [name]");
    zcon.Register("camera autoswitch", "toggle camera autoswitch");
    zcon.Register("cheat full", "heal player");
    zcon.Register("cheat god", "godmode on/off");
    zcon.Register("save position", "save player position");
    zcon.Register("save zen", "save level [name.ZEN]");
    zcon.Register("load position", "restore player position");
    zcon.Register("set levelchanger 1", "Include LevelChanger to net/netlevel1.zen.");
    zcon.Register("set levelchanger 2", "Include LevelChanger to net/netlevel2.zen.");
    zcon.Register("play ani", "play animation [name]");
    zcon.Register("play sound", "player sound instance [name]");
    zcon.Register("kill", "kill focus npc");
    zcon.Register("set time", "set worldtime [hh mm]");
    zcon.Register("aigoto", "player goes to waypoint [name]");
    zcon.Register("autocomplement", "autocomplement on/off");
    zcon.Register("witness attack", "call reaction attack");
    zcon.Register("witness aim", "call reaction aim");
    zcon.Register("witness steal", "call reaction steal");
    zcon.Register("parserd on", "show parser debug info");
    zcon.Register("parserd off", "hide parser debug info");
    zcon.Register("parserd merge", "merge 'SRC' oder 'D'-File");
    zcon.Register("parserd pcode", "print pcode to spy.");
    zcon.Register("set fbbox", "show boundingbox of focus object");
    zcon.Register("apply overlaymds", "enable overlay-MDS [mds-name]");
    zcon.Register("remove overlaymds", "remove overlay-MDS [mds-name]");
    zcon.Register("apply randomani", "apply a random-ani [ani-name, random-ani-name]");
    zcon.Register("apply anifreq", "set random-ani frequency [ani-name, frequency]");
    zcon.Register("edit particlefx", "edit particle-effect-instance [name]");
    zcon.Register("start particlefx", "start particle-effect-instance [name]");
    zcon.Register("create particlefx", "create new particle-effect-instance [name]");
    zcon.Register("delete particlefx", "delete last used particle-effect-instance");
    zcon.Register("play faceani", "play a face-ani [name]");
    zcon.Register("save game", "save game in slot 0");
    zcon.Register("load game", "load game from Slot 0");
    zcon.Register("set modelfatness", "set model fatness [value]");
    zcon.Register("set debugfocus", "set debug focus object");
    zcon.Register("clear debugfocus", "clear debug focus object");
    zcon.Register("toggle debugfocus", "toggle debug focus object");
    zcon.Register("set alldebuginstances", "set all debug instances");
    zcon.Register("clear alldebuginstances", "clear all debug instances");
    zcon.Register("set debugchannel", "set debug channel [ch]");
    zcon.Register("clear debugchannel", "clear debug channel [ch]");
    zcon.Register("toggle debugchannel", "toggle debug channel [ch]");
    zcon.Register("set alldebugchannels", "set all debug channels");
    zcon.Register("clear alldebugchannels", "clear all debug channels");
    zcon.Register("toggle alldebugchannels", "toggle all debug channels");
    zcon.Register("set permattitude", "set permanent attitude [attitude]");
    zcon.Register("set tempattitude", "set temporary attitude [attitude]");
    zcon.Register("set trueguild", "set true guild [guild]");
    zcon.Register("set guild", "set guild [guild]");
    zcon.Register("goto vob", "beam player to vob [name] ([num])");
    zcon.Register("print vob", "print info for vob(s) [name]");
    zcon.Register("version", "print version info");
    game_con = 1;
}

void Game_OpenConsole()
{
    if (!game_con)
        Game_InitConsole();
    zcon.Toggle();
}

float oCNpc::ai_scriptStateSkip;
float oCNpc::ai_scriptStateSkipTimer;
float oCNpc::ai_messagesSkip;
float oCNpc::ai_messagesSkipTimer;
int oCNpc::ai_baseEnabled;

void Game_ChangeAIConsole(const zSTRING& name)
{
    if (name == "SPAWN") {
        oCSpawnManager::SetSpawnTime((float)game_spawnTime);
        return;
    }
    if (name == "INSERT") {
        oCSpawnManager::SetInsertRange((float)game_insertRange);
        return;
    }
    if (name == "REMOVE") {
        oCSpawnManager::SetRemoveRange((float)game_removeRange);
        return;
    }
    if (name == "SCRIPT") {
        oCNpc::ai_scriptStateSkip = (float)game_scriptSkip;
        oCNpc::ai_scriptStateSkipTimer = 0.0f;
        return;
    }
    if (name == "MESSAGES") {
        oCNpc::ai_messagesSkip = (float)game_messagesSkip;
        oCNpc::ai_messagesSkipTimer = 0.0f;
        return;
    }
    if (name == "BASE")
        oCNpc::ai_baseEnabled = game_baseEnabled > 0;
}

void Game_ToggleAIConsole()
{
    delete game_aiConsole;
    game_aiConsole = 0;
    game_spawnTime = (int)oCSpawnManager::GetSpawnTime();
    game_insertRange = (int)oCSpawnManager::GetInsertRange();
    game_removeRange = (int)oCSpawnManager::GetRemoveRange();
    game_scriptSkip = (int)oCNpc::ai_scriptStateSkip;
    game_messagesSkip = (int)oCNpc::ai_messagesSkip;
    game_baseEnabled = oCNpc::ai_baseEnabled;
    game_aiConsole = new zCConsole(0, 2500, "AI - Konsole");
    game_aiConsole->InsertVar("SPAWN", "Spawntime (ms)", 0, &game_spawnTime, 1);
    game_aiConsole->InsertVar("INSERT", "Spawn-Insert-Range (cm)", 0, &game_insertRange, 1);
    game_aiConsole->InsertVar("REMOVE", "Spawn-Remove-Range (cm)", 0, &game_removeRange, 1);
    game_aiConsole->InsertVar("SCRIPT", "Scriptstates skiptimer (ms)", 0, &game_scriptSkip, 1);
    game_aiConsole->InsertVar("MESSAGES", "Messages skiptimer (ms)", 0, &game_messagesSkip, 1);
    game_aiConsole->InsertVar("BASE", "Base AI enabled", 0, &game_baseEnabled, 1);
    game_aiConsole->SetChangedFunc(Game_ChangeAIConsole);
    game_aiConsole->Toggle();
}

static zCConsole* game_species_con;
static zCConsole* game_fight_con;
static int game_jumpUpHeight;
static float game_turnSpeed;
static int game_bloodDistance;
static int game_bloodFlow;
static int game_bloodAmount;
static zSTRING game_bloodTexture;
static float game_swimTime;
static float game_diveTime;
static int game_disableAutoroll;
static int game_surfaceAlign;
static int game_fightRange;

void Game_SpeciesChanged(const zSTRING& name)
{
    oCAniCtrl_Human* aniCtrl = oCNpc::player->GetAnictrl();
    if (name == "SLIDE1") {
        aniCtrl->config.zMV_MAX_GROUND_ANGLE_WALK = aniCtrl->angle_slide1 * 0.0174532924f;
        return;
    }
    if (name == "SLIDE2") {
        aniCtrl->config.zMV_MAX_GROUND_ANGLE_SLIDE2 = aniCtrl->angle_slide2 * 0.0174532924f;
        return;
    }
    if (name == "HEADING") {
        aniCtrl->config.zMV_DCUL_WALL_HEADING_ANGLE = aniCtrl->angle_heading * 0.0174532924f;
        return;
    }
    if (name == "HORIZ") {
        aniCtrl->config.zMV_DCUL_WALL_HORIZ_ANGLE = aniCtrl->angle_horiz * 0.0174532924f;
        return;
    }
    if (name == "GROUND") {
        aniCtrl->config.zMV_DCUL_GROUND_ANGLE = aniCtrl->angle_ground * 0.0174532924f;
        return;
    }
    if (name == "SWIM_TIME" || name == "DIVE_TIME") {
        oCNpc::player->SetSwimDiveTime(game_swimTime, game_diveTime);
        return;
    }
    if (name == "DISABLE_AUTOROLL") {
        aniCtrl->forceModelHalt = !game_disableAutoroll;
        return;
    }
    if (name == "SURFACE_ALIGN") {
        aniCtrl->zMV_DO_SURFACE_ALIGN = game_surfaceAlign;
        return;
    }
    if (name == "TURN") {
        int turnSpeed = (int)game_turnSpeed;
        oCNpc::player->speedTurn = (turnSpeed ? turnSpeed : 150) * 0.00100000005f;
        return;
    }
    if (name == "JUMPUP") {
        aniCtrl->SetJumpUpForceByHeight((float)game_jumpUpHeight);
        return;
    }
    if (name == "BL_DIST") {
        oCNpc::player->bloodDistance = game_bloodDistance ? game_bloodDistance : 1000;
        return;
    }
    if (name == "BL_FLOW") {
        oCNpc::player->bloodFlow = game_bloodFlow;
        return;
    }
    if (name == "BL_AMOUNT") {
        oCNpc::player->bloodAmount = game_bloodAmount ? game_bloodAmount : 10;
        return;
    }
    if (name == "BL_TEX")
        oCNpc::player->bloodTexture = game_bloodTexture.IsEmpty()
            ? zSTRING("ZBLOODSPLAT2.TGA") : game_bloodTexture;
}

void Game_OpenSpeciesConsole()
{
    delete game_species_con;
    game_species_con = 0;
    oCAniCtrl_Human* aniCtrl = oCNpc::player->GetAnictrl();
    aniCtrl->InitAngleValuesForConsole();
    game_species_con = new zCConsole(0, 1500, "Spezies abh");
    game_species_con->SetDim(6000, 6000);
    game_species_con->InsertVar("SLIDE1", "Slide-Winkel 1", 1, &aniCtrl->angle_slide1, 1);
    game_species_con->InsertVar("SLIDE2", "Slide-Winkel 2", 1, &aniCtrl->angle_slide2, 1);
    game_species_con->InsertVar("JUMPMID", "JumpUpMid bis", 1, &aniCtrl->config.zMV_YMAX_JUMPMID, 1);
    game_species_con->InsertVar("JUMPLOW", "JumpUpLow bis", 1, &aniCtrl->config.zMV_YMAX_JUMPLOW, 1);
    game_species_con->InsertVar("KNEE", "Wasserhöhe", 1, &aniCtrl->config.zMV_WATER_DEPTH_KNEE, 1);
    game_species_con->InsertVar("CHEST", "Wasserhöhe", 1, &aniCtrl->config.zMV_WATER_DEPTH_CHEST, 1);
    game_jumpUpHeight = (int)aniCtrl->GetJumpUpHeight();
    game_species_con->InsertVar("JUMPUP", "JumpUpHeight", 0, &game_jumpUpHeight, 1);
    game_turnSpeed = oCNpc::player->GetTurnSpeed() * 1000.0f;
    game_species_con->InsertVar("TURN", "TurnSpeed", 1, &game_turnSpeed, 1);
    game_bloodDistance = oCNpc::player->bloodDistance;
    game_species_con->InsertVar("BL_DIST", "BloodDistance", 0, &game_bloodDistance, 1);
    game_bloodFlow = oCNpc::player->bloodFlow;
    game_species_con->InsertVar("BL_FLOW", "BloodFlow", 0, &game_bloodFlow, 1);
    game_bloodAmount = oCNpc::player->bloodAmount;
    game_species_con->InsertVar("BL_AMOUNT", "BloodAmount", 0, &game_bloodAmount, 1);
    game_bloodTexture = oCNpc::player->GetBloodTexture();
    game_species_con->InsertVar("BL_TEX", "BloodTexture", 2, &game_bloodTexture, 1);
    oCNpc::player->GetSwimDiveTime(game_swimTime, game_diveTime);
    game_swimTime *= 0.00100000005f;
    game_diveTime *= 0.00100000005f;
    game_species_con->InsertVar("SWIM_TIME", "SwimTime", 1, &game_swimTime, 1);
    game_species_con->InsertVar("DIVE_TIME", "DiveTime", 1, &game_diveTime, 1);
    game_disableAutoroll = !aniCtrl->forceModelHalt;
    game_species_con->InsertVar("DISABLE_AUTOROLL", "DisableAutoroll", 0, &game_disableAutoroll, 1);
    game_surfaceAlign = aniCtrl->zMV_DO_SURFACE_ALIGN;
    game_species_con->InsertVar("SURFACE_ALIGN", "SurfaceAlign", 0, &game_surfaceAlign, 1);
    game_species_con->InsertVar("STEP", "Stufenhöhe", 1, &aniCtrl->config.zMV_STEP_HEIGHT, 1);
    game_species_con->InsertVar("HEADING", "Climb-Wall Heading", 1, &aniCtrl->angle_heading, 1);
    game_species_con->InsertVar("HORIZ", "Climb-Wall Horiz.", 1, &aniCtrl->angle_horiz, 1);
    game_species_con->InsertVar("GROUND", "Climb-Ground", 1, &aniCtrl->angle_ground, 1);
    game_fightRange = oCNpc::player->GetFightRange();
    game_species_con->InsertVar("RANGE", "Fight-Range", 0, &game_fightRange, 1);
    game_species_con->SetChangedFunc(Game_SpeciesChanged);
    game_species_con->Toggle();
}

void Game_OpenFightConsole()
{
    delete game_fight_con;
    game_fight_con = new zCConsole(0, 4000, "Kampfsystem");
    game_fight_con->InsertVar("UPANGLE", "AutoAim Höhenwinkel", 0, &oCNpc::angle_far_up, 1);
    game_fight_con->InsertVar("SIANGLE", "AutoAim Seitenwinkel", 0, &oCNpc::angle_far_side, 1);
    game_fight_con->InsertVar("HEADING", "Nahkampf-Heading Winkel", 0, &oCNpc::angle_near, 1);
    game_fight_con->Toggle();
}

int oCGame::IsDebuggingChannel(int channel)
{
    return (debugChannels & (1 << ((channel - 1) & 0x1f))) != 0;
}

int oCGame::IsDebuggingInstance(zCVob* vob)
{
    for (int i = 0; i < debugInstances.numInArray; ++i) {
        if (debugInstances.parray[i] == vob)
            return 1;
    }
    return 0;
}

int oCGame::IsDebuggingAllInstances()
{
    return debugAllInstances;
}

int oCGame::OpenView(oEGameDialogView view)
{
    if (!array_view_visible[view] && array_view_enabled[view]) {
        array_view_visible[view] = 1;
        array_view[view]->Open();
    }
    return 1;
}

int oCGame::CloseView(oEGameDialogView view)
{
    if (array_view_visible[view] && array_view_enabled[view]) {
        array_view_visible[view] = 0;
        array_view[view]->Close();
    }
    return 1;
}

void NotifyInfoToSpy(zSTRING& text)
{
    if (zerr->filter_level >= 8)
        zerr->Message(zSTRING("R: INF: ") + text);
}

void NotifyInfoToSpy(char* fileName, char* line, char* functionName)
{
    zSTRING text;
    text.Sprintf("[%-30s]->[%-30s] [%-30s]", fileName, line, functionName);
    NotifyInfoToSpy(text);
}

int PlayVideo()
{
    zCParser* currentParser = zCParser::GetParser();
    zSTRING fileName;
    currentParser->GetParameter(&fileName);
    currentParser->SetReturn(gameMan->PlayVideo(fileName));
    return 1;
}

int PlayVideoEx()
{
    zCParser* currentParser = zCParser::GetParser();
    int first;
    int second;
    zSTRING fileName;
    currentParser->GetParameter(&first);
    currentParser->GetParameter(&second);
    currentParser->GetParameter(&fileName);
    currentParser->SetReturn(gameMan->PlayVideoEx(fileName, (short)first, second));
    return 1;
}

int IntToString();
int FloatToString();
int FloatToInt();
int IntToFloat();
int ConcatStrings();
int Print();
int PrintDebug();
int PrintScreen();
int AI_PrintScreen();
int PrintDialog();
int PrintDebugInst();
int PrintDebugInstCh();
int PrintDebugCh();
int PrintMulti();
int ExitGame();
int ExitSession();
int SetPercentDone();
int IntroduceChapter();
int Npc_MemoryEntry();
int Npc_MemoryEntryGuild();
int Npc_HasNews();
int Npc_GetNewsWitness();
int Npc_GetNewsOffender();
int Npc_GetNewsVictim();
int Npc_IsNewsGossip();
int Npc_DeleteNews();
int Npc_GetGuildAttitude();
int Npc_GetActiveSpell();
int Npc_GetLastHitSpellID();
int Npc_GetLastHitSpellCat();
int Npc_GetActiveSpellCat();
int Npc_GetActiveSpellLevel();
int Npc_GetActiveSpellIsScroll();
int Npc_SetActiveSpellInfo();
int Wld_DetectItem();
int Wld_DetectPlayer();
int Wld_DetectNpc();
int Wld_DetectNpcEx();
int Wld_DetectNpcExAtt();
int Wld_SetGuildAttitude();
int Wld_GetGuildAttitude();
int Wld_IsMobAvailable();

void oCGame::DefineExternals_Ulfi(zCParser* scriptParser)
{
    scriptParser->DefineExternal("IntToString", IntToString, 3, 2, 0);
    scriptParser->DefineExternal("FloatToString", FloatToString, 3, 1, 0);
    scriptParser->DefineExternal("FloatToInt", FloatToInt, 2, 1, 0);
    scriptParser->DefineExternal("IntToFloat", IntToFloat, 1, 2, 0);
    scriptParser->DefineExternal("ConcatStrings", ConcatStrings, 3, 3, 3, 0);
    scriptParser->DefineExternal("Print", Print, 0, 3, 0);
    scriptParser->DefineExternal("PrintDebug", PrintDebug, 0, 3, 0);
    scriptParser->DefineExternal("PrintScreen", PrintScreen, 2, 3, 2, 2, 3, 2, 0);
    scriptParser->DefineExternal("AI_PrintScreen", AI_PrintScreen, 2, 3, 2, 2, 3, 2, 0);
    scriptParser->DefineExternal("PrintDialog", PrintDialog, 2, 3, 2, 2, 3, 2, 2, 0);
    scriptParser->DefineExternal("PrintDebugInst", PrintDebugInst, 0, 3, 0);
    scriptParser->DefineExternal("PrintDebugInstCh", PrintDebugInstCh, 0, 3, 2, 0);
    scriptParser->DefineExternal("PrintDebugCh", PrintDebugCh, 0, 3, 2, 0);
    scriptParser->DefineExternal("PrintMulti", PrintMulti, 0, 3, 3, 3, 3, 3, 0);
    scriptParser->DefineExternal("ExitGame", ExitGame, 0, 0);
    scriptParser->DefineExternal("ExitSession", ExitSession, 0, 0);
    scriptParser->DefineExternal("PlayVideo", PlayVideo, 2, 3, 0);
    scriptParser->DefineExternal("PlayVideoEx", PlayVideoEx, 2, 2, 3, 2, 0);
    scriptParser->DefineExternal("SetPercentDone", SetPercentDone, 0, 2, 0);
    scriptParser->DefineExternal("IntroduceChapter", IntroduceChapter, 0, 3, 3, 3, 3, 2, 0);
    scriptParser->DefineExternal("Npc_MemoryEntry", Npc_MemoryEntry, 0, 7, 2, 7, 2, 7, 0);
    scriptParser->DefineExternal("Npc_MemoryEntryGuild", Npc_MemoryEntryGuild, 0, 7, 2, 7, 2, 7, 0);
    scriptParser->DefineExternal("Npc_HasNews", Npc_HasNews, 2, 7, 2, 7, 0);
    scriptParser->DefineExternal("Npc_GetNewsWitness", Npc_GetNewsWitness, 2, 7, 7, 0);
    scriptParser->DefineExternal("Npc_GetNewsOffender", Npc_GetNewsOffender, 2, 7, 7, 0);
    scriptParser->DefineExternal("Npc_GetNewsVictim", Npc_GetNewsVictim, 2, 7, 7, 0);
    scriptParser->DefineExternal("Npc_IsNewsGossip", Npc_IsNewsGossip, 2, 7, 2, 0);
    scriptParser->DefineExternal("Npc_DeleteNews", Npc_DeleteNews, 2, 7, 2, 0);
    scriptParser->DefineExternal("Npc_GetGuildAttitude", Npc_GetGuildAttitude, 2, 7, 7, 0);
    scriptParser->DefineExternal("Npc_GetActiveSpell", Npc_GetActiveSpell, 2, 7, 0);
    scriptParser->DefineExternal("Npc_GetLastHitSpellID", Npc_GetLastHitSpellID, 2, 7, 0);
    scriptParser->DefineExternal("Npc_GetLastHitSpellCat", Npc_GetLastHitSpellCat, 2, 7, 0);
    scriptParser->DefineExternal("Npc_GetActiveSpellCat", Npc_GetActiveSpellCat, 2, 7, 0);
    scriptParser->DefineExternal("Npc_GetActiveSpellLevel", Npc_GetActiveSpellLevel, 2, 7, 0);
    scriptParser->DefineExternal("Npc_GetActiveSpellIsScroll", Npc_GetActiveSpellIsScroll, 2, 7, 0);
    scriptParser->DefineExternal("Npc_SetActiveSpellInfo", Npc_SetActiveSpellInfo, 2, 7, 2, 0);
    scriptParser->DefineExternal("Wld_DetectItem", Wld_DetectItem, 2, 7, 2, 0);
    scriptParser->DefineExternal("Wld_DetectPlayer", Wld_DetectPlayer, 2, 7, 0);
    scriptParser->DefineExternal("Wld_DetectNpc", Wld_DetectNpc, 2, 5, 2, 7, 2, 0);
    scriptParser->DefineExternal("Wld_DetectNpcEx", Wld_DetectNpcEx, 2, 2, 5, 2, 7, 2, 0);
    scriptParser->DefineExternal("Wld_DetectNpcExAtt", Wld_DetectNpcExAtt, 2, 2, 2, 5, 2, 7, 2, 0);
    scriptParser->DefineExternal("Wld_SetGuildAttitude", Wld_SetGuildAttitude, 0, 2, 2, 2, 0);
    scriptParser->DefineExternal("Wld_GetGuildAttitude", Wld_GetGuildAttitude, 2, 2, 2, 0);
    scriptParser->DefineExternal("Wld_IsMobAvailable", Wld_IsMobAvailable, 2, 3, 7, 0);
}

void ToggleShowStateDebug(oCNpc* npc)
{
    if (!npc)
        return;
    npc->showaidebug ^= 1;
    if (npc->showaidebug & 1)
        zerr->Message(zSTRING("U:NPC:") + npc->GetName() + " Log actions on.");
    else
        zerr->Message(zSTRING("U:NPC:") + npc->GetName() + " Log actions off.");
}

void ToggleShowAI(oCNpc* npc)
{
    zCList<zCVob>* node = ogame->GetGameWorld()->voblist_npcs->next;
    while (node) {
        oCNpc* current = (oCNpc*)node->data;
        node = node->next;
        if (current != npc && current->human_ai)
            ((oCAIHuman*)current->human_ai)->SetShowAI(0);
    }
    if (npc && npc->human_ai) {
        oCAIHuman* humanAI = (oCAIHuman*)npc->human_ai;
        humanAI->SetShowAI(!humanAI->GetShowAI());
    }
}

void oCGame::SwitchCamToNextNpc()
{
    static oCNpc* cameraNpc;
    static int cameraNpcIndex;
    int showAI = 0;
    if (cameraNpc && cameraNpc->human_ai) {
        oCAIHuman* humanAI = (oCAIHuman*)cameraNpc->human_ai;
        showAI = humanAI->GetShowAI();
        humanAI->SetShowAI(0);
    }
    do {
        cameraNpc = (oCNpc*)GetGameWorld()->voblist_npcs->Get(cameraNpcIndex++);
        if (cameraNpc && cameraNpc->human_ai)
            ((oCAIHuman*)cameraNpc->human_ai)->SetShowAI(showAI);
        else
            cameraNpc = 0;
    } while (!cameraNpc);
    GetCameraAI()->SetTarget(cameraNpc);
    GetCameraAI()->ReceiveMsg(0x8000);
    oCNpc* player = oCNpc::player;
    if (!(player->sleepingMode & 3))
        oCNpc::player = 0;
    HandleEvent(0x3e);
    oCNpc::player = player;
}

int oCGame::HandleEvent(int key)
{
    if (GetWorld()->csPlayer->GetPlayingGlobalCutscene() || !oCNpc::player)
        return 0;
    if (game_testmode)
        return TestKeys(key);
    return 0;
}

int oCGame::TestKeys(int key)
{
    if (!oCNpc::player)
        return 1;
    if (game_editwaynet)
        return IA_TestWay(key);
    return 0;
}

static zCWaypoint* game_firstWaypoint;
static zCWaypoint* game_secondWaypoint;
static zSTRING game_waypointName("WP");
static int game_waypointIndex;

static zCWaypoint* Game_CreateTestWaypoint(oCGame* game)
{
    zCVobWaypoint* vob = new zCVobWaypoint;
    zCClassDef::ObjectCreated(vob, zCVobWaypoint::classDef);
    vob->SetVobName(game_waypointName);
    game->GetWorld()->AddVob(vob);
    vob->SetCollDetStat(0);
    vob->SetCollDetDyn(0);
    zVEC3 position = {
        oCNpc::player->trafoObjToWorld.m[0][3],
        oCNpc::player->trafoObjToWorld.m[1][3],
        oCNpc::player->trafoObjToWorld.m[2][3]
    };
    zVEC3 heading = {
        oCNpc::player->trafoObjToWorld.m[0][2],
        oCNpc::player->trafoObjToWorld.m[1][2],
        oCNpc::player->trafoObjToWorld.m[2][2]
    };
    vob->SetPositionWorld(position);
    vob->SetHeadingAtWorld(heading);
    zCWaypoint* waypoint = zfactory->CreateWaypoint();
    waypoint->Init(vob);
    waypoint->SetName(game_waypointName + zSTRING((long)game_waypointIndex++));
    game->GetWorld()->wayNet->InsertWaypoint(waypoint);
    return waypoint;
}

int oCGame::IA_TestWay(int key)
{
    zCWayNet* wayNet = GetWorld()->wayNet;
    switch (key) {
    case 2:
        game_firstWaypoint = wayNet->GetNearestWaypoint(GetStartPos());
        if (game_firstWaypoint)
            zerr->Message(zSTRING("< Select WP1 ") + game_firstWaypoint->GetName() + " >");
        return 1;
    case 3:
        game_secondWaypoint = wayNet->GetNearestWaypoint(GetStartPos());
        if (game_secondWaypoint)
            zerr->Message(zSTRING("< Select WP2 ") + game_secondWaypoint->GetName() + " >");
        return 1;
    case 16:
        if (zinput->KeyPressed(29)) {
            zCView::PrintDebug("< Waynet - Mode aus >\n");
            game_editwaynet = 0;
        } else if (game_firstWaypoint && game_secondWaypoint) {
            wayNet->CreateWay(game_firstWaypoint, game_secondWaypoint);
        }
        return 1;
    case 17:
        if (game_firstWaypoint && game_secondWaypoint)
            wayNet->CreateWay(game_firstWaypoint, game_secondWaypoint);
        return 1;
    case 18:
        if (game_firstWaypoint) {
            zCView* view = new zCView(0, 0, 2048, screen->FontY(), 2);
            screen->InsertItem(view);
            zSTRING name = view->Input(30, game_firstWaypoint->GetName());
            game_firstWaypoint->SetName(name);
            if (game_firstWaypoint->GetVob())
                game_firstWaypoint->GetVob()->SetVobName(name);
            screen->RemoveItem(view);
            delete view;
        }
        return 1;
    case 29:
        wayNet->CorrectHeight();
        return 1;
    case 30:
        if (zinput->KeyPressed(29))
            wayNet->DeleteWaypoint(game_firstWaypoint);
        else
            wayNet->DeleteWay(game_firstWaypoint, game_secondWaypoint);
        return 1;
    case 32:
        if (game_firstWaypoint)
            wayNet->DeleteWaypoint(game_firstWaypoint);
        return 1;
    case 46:
        if (game_firstWaypoint) {
            game_secondWaypoint = Game_CreateTestWaypoint(this);
            wayNet->CreateWay(game_firstWaypoint, game_secondWaypoint);
            game_firstWaypoint = game_secondWaypoint;
        }
        return 1;
    case 47:
        game_firstWaypoint = Game_CreateTestWaypoint(this);
        return 1;
    default:
        return 0;
    }
}
