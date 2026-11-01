#include "include/oGame.h"
#include "include/oBinkPlayer.h"
#include "include/zTexture.h"
#include "include/zMorphMesh.h"
#include "include/zProgMesh.h"
#include "include/zSound.h"
#include "include/zMusic.h"
#include "include/oSysInfo.h"
#include "include/zEngine.h"
#include "include/oObjFactory.h"
#include "include/zMoving.h"
#include "include/zMenu.h"
#include "include/oInformationManager.h"
#include "include/oMenuSavegame.h"
#include <windows.h>
#include <process.h>

extern HINSTANCE hInstApp;
extern zCOption* zgameoptions;

static int menuItemState = -1;
static zSTRING menuListBoxLoadGame = "MENU_ITEM_LISTBOX_LOADGAME";
static zSTRING menuListBoxSaveGame = "MENU_ITEM_LISTBOX_SAVEGAME";
static zSTRING menuOptControls = "MENU_OPT_CONTROLS";
static zSTRING menuOptControlsExtKeys = "MENU_OPT_CONTROLS_EXTKEYS";
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
static zSTRING gameEmptyString1;
static zSTRING gameEmptyString2;
static zSTRING gameEmptyString3;
static zSTRING gameEmptyString4;
static zSTRING menuSavegameSave = "MENU_SAVEGAME_SAVE";
static zSTRING menuSavegameLoad = "MENU_SAVEGAME_LOAD";

int   oCGame::s_bUseQuickSave = 1;
int   oCGame::s_bUseOldControls = 1;
float oCGame::s_fUseSpeechReverb = 1.0f;
int   oCGame::s_bUsePotionKeys = 0;

static HINSTANCE hSplashInstance;
static HANDLE hSplashThread;
static HWND hSplashWnd;
static HBITMAP hSplashBitmap;
static DWORD splashThreadId;

int __stdcall SplashDialogProc(HWND window, unsigned int message, unsigned int wParam, long lParam);
unsigned long __stdcall SplashThreadProc(void* parameter);

void zDieter_ShutDown();
void zDieter_ShutDown_Fast();
void zCarsten_ShutDown();
void sysExit();
void sysEvent();
void oBert_StartUp();

CGameManager::~CGameManager()
{
    if (gameSession)
        zerr->Warning("B: GMAN: gameSession is existing. Call CGameManager::Done() before!");
    if (videoPlayInGame) {
        videoPlayer->PlayDeinit();
        videoPlayer->CloseVideo();
    }
    delete videoPlayer;
    videoPlayer = NULL;
    delete gameSession;
    gameSession = NULL;
    delete backLoop;
    backLoop = NULL;
    delete initScreen;
    initScreen = NULL;
    lastDatFileList.numInArray = 0;
    lastWorldList.numInArray = 0;
    gameMan = NULL;
}

void CGameManager::Tool_ConvertData()
{
    zerr->Message("B: Converting all data ...");
    zCScanDir scanner;
    zCModelConvertFileHandler modelHandler;
    zCMorphMeshConvertFileHandler morphHandler;
    zCProgMeshConvertFileHandler progMeshHandler;
    zCTextureFileHandler textureHandler;
    textureHandler.RegisterFileType(".tga");
    scanner.RegisterFileHandler(&modelHandler, "zCModel");
    scanner.RegisterFileHandler(&morphHandler, "zCMorphMesh");
    scanner.RegisterFileHandler(&progMeshHandler, "zCProgMeshProto");
    scanner.Scan(zoptions->GetDirString(3));
    zerr->Message("B: finished");
}

void emergency_exit()
{
    zsound = NULL;
    OSVERSIONINFOA versionInformation;
    versionInformation.dwOSVersionInfoSize = sizeof(versionInformation);
    if (!GetVersionExA(&versionInformation) || versionInformation.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS)
        zDieter_ShutDown_Fast();
    else {
        zDieter_ShutDown();
        zCarsten_ShutDown();
    }
    _exit(0);
}

void ExitGameFunc()
{
    gameMan->Done();
    delete gameMan;
    gameMan = NULL;
    sysExit();
}

void CGameManager::InitScreen_Open()
{
    zerr->Message("B: GMAN: Open InitScreen");
    if (initScreen) {
        zerr->Message("B: GMAN: Close InitScreen");
        delete initScreen;
        initScreen = NULL;
    }
    initScreen = new zCView(0, 0, 0x2000, 0x2000, 2);
    zoptions->ChangeDir(13);
    screen->InsertItem(initScreen, 0);
    initScreen->InsertBack("startscreen.tga");
    zrenderer->BeginFrame();
    screen->Render();
    zrenderer->EndFrame();
    zrenderer->Vid_Blit(1, NULL, NULL);
}

void CGameManager::InitScreen_Menu()
{
    zerr->Message("B: GMAN: MenuScreen");
    zrenderer->BeginFrame();
    screen->Render();
    zrenderer->EndFrame();
    zrenderer->Vid_Blit(1, NULL, NULL);
}

void CGameManager::InitScreen_Close()
{
    zerr->Message("B: GMAN: Close InitScreen");
    delete initScreen;
    initScreen = NULL;
}

void CGameManager::InitSettings()
{
    zerr->Separator("");
    oCSystemInfo systemInfo;
    systemInfo.AnalyseNow();
    zerr->Separator("");
    systemInfo.cpuLow = 696.0f;
    systemInfo.cpuHigh = 1200.0f;
    systemInfo.memLow = 256.0f;
    systemInfo.memHigh = 512.0f;
    systemInfo.graMemLow = 16.0f;
    systemInfo.graMemHigh = 64.0f;
    systemInfo.ScoreNow();
    zerr->Separator("");
    systemInfo.ResetSettings(-1.0f);
    zerr->Separator("");
}

int __stdcall SplashDialogProc(HWND window, unsigned int message, unsigned int, long)
{
    if (message == WM_DESTROY)
        PostQuitMessage(0);
    return 0;
}

unsigned long __stdcall SplashThreadProc(void*)
{
    hSplashWnd = CreateDialogParamA(hSplashInstance, MAKEINTRESOURCEA(165), NULL,
        SplashDialogProc, 0);
    if (hSplashWnd) {
        HWND image = GetDlgItem(hSplashWnd, 1078);
        if (image) {
            const int width = 502;
            const int height = 202;
            SetWindowPos(hSplashWnd, HWND_TOPMOST,
                (GetSystemMetrics(SM_CXSCREEN) - width) / 2,
                (GetSystemMetrics(SM_CYSCREEN) - height) / 2,
                width, height, SWP_SHOWWINDOW);
            SetWindowPos(hSplashWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE);
            SendMessageA(image, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hSplashBitmap);
            UpdateWindow(hSplashWnd);
            MSG message;
            while (GetMessageA(&message, NULL, 0, 0))
                DispatchMessageA(&message);
        }
    }
    return 0;
}

void CGameManager::ShowSplashScreen()
{
    hSplashInstance = hInstApp;
    hSplashBitmap = LoadBitmapA(hInstApp, MAKEINTRESOURCEA(169));
    hSplashThread = CreateThread(NULL, 0, SplashThreadProc, NULL, 0, &splashThreadId);
}

void CGameManager::RemoveSplashScreen()
{
    if (splashThreadId)
        PostThreadMessageA(splashThreadId, WM_QUIT, 0, 0);
}

void MenuFrameCallback()
{
    if (gameMan->backLoop) {
        gameMan->backLoop->Render();
        gameMan->backLoop->RenderBlit();
    } else if (gameMan->gameSession && gameMan->gameSession->GetCamera()) {
        gameMan->gameSession->Render();
        gameMan->gameSession->RenderBlit();
    }
}

void MenuEnterCallback()
{
    oCGame* session = gameMan->gameSession;
    if (session && session->GetCamera()) {
        zinput->ClearKeyBuffer();
        zinput->ResetRepeatKey(1);
        session->SetEnableHandleEvent(0);
        if (session->GetSelfPlayerVob())
            session->GetSelfPlayerVob()->SetMovLock(1);
        zCMenu::world = session->GetWorld();
        zCMenu::camera = session->GetCamera();
        zCMenu::FrameCallback = MenuFrameCallback;
        session->CloseView((oEGameDialogView)2);
    } else {
        zCMenu::GetActive()->GetBackDatFile();
        zCMenu::GetActive()->GetBackWorld();
    }
}

void MenuLeaveCallback()
{
    oCGame* session = gameMan->gameSession;
    if (!session || !session->GetCamera())
        return;
    zinput->ClearKeyBuffer();
    zinput->ResetRepeatKey(1);
    session->SetEnableHandleEvent(1);
    if (session->GetSelfPlayerVob())
        session->GetSelfPlayerVob()->SetMovLock(0);
    session->OpenView((oEGameDialogView)2);
}

void CGameManager::GameInit()
{
    zerr->on_exit = ExitGameFunc;
    ShowSplashScreen();
    if (!zengine)
        zengine = new zCEngine;
    zengine->Init(sysContextHandle);
    RemoveSplashScreen();
    if (zoptions->ReadBool(zOPT_SEC_GAME, "playLogoVideos", 1)) {
        PlayVideo("logo1.bik");
        PlayVideo("logo2.bik");
    }
    sysEvent();
    zoptions->ReadBool(zOPT_SEC_PERFORMANCE, "recalc", 1);
    if (zoptions->WriteBool(zOPT_SEC_PERFORMANCE, "recalc", 0))
        InitSettings();
    InitScreen_Open();
    if (zfactory) {
        zfactory->Release();
        zfactory = NULL;
    }
    zfactory = new oCObjectFactory;
    zCClassDef::ObjectCreated(zfactory, oCObjectFactory::classDef);
    oBert_StartUp();
    vidScreen = screen;
    vidScreen->SetEnableHandleEvent(1);
    sysEvent();
    oCarsten_StartUp();
    zCMenu::EnterCallback = MenuEnterCallback;
    zCMenu::LeaveCallback = MenuLeaveCallback;
}

void CGameManager::GameDone()
{
    oCarsten_ShutDown();
    zengine->Shutdown();
    delete zengine;
    zengine = NULL;
}

void CGameManager::GameSessionReset()
{
    if (!gameSession)
        zerr->Report(zERR_TYPE_FATAL, 0, "B: GameMan: Call GameSessionInit before GameSessionReset() ",
            -1, 0, 0x40F, 0, 0);
    savegameManager->ClearCurrent();
    gameSession->SetGameInfo(NULL);
}

void CGameManager::GameSessionInit()
{
    if (gameSession)
        zerr->Report(zERR_TYPE_FATAL, 0, "B: GameMan: Call GameSessionInit once, not twice!",
            -1, 0, 0x417, 0, 0);
    gameSession = new oCGame;
    ogame = gameSession;
    gameSession->Init();
    gameSession->savegameManager = savegameManager;
    gameSession->SetGameInfo(NULL);
}

void CGameManager::GameSessionDone()
{
    if (!gameSession)
        zerr->Report(zERR_TYPE_FATAL, 0, "B: GameMan: Call GameSessionInit before GameSessionDone",
            -1, 0, 0x421, 0, 0);
    gameSession->Done();
    delete gameSession;
    gameSession = NULL;
    ogame = NULL;
}

void CGameManager::ShowIntro()
{
}

void CGameManager::ShowExtro()
{
}

void CGameManager::PreMenu()
{
    zoptions->SetFlag(zOPT_SEC_VIDEO, ZOPT_VID_RES, 1);
    const int width = zoptions->ReadInt(zOPT_SEC_VIDEO, "zVidResFullscreenX", 800);
    const int height = zoptions->ReadInt(zOPT_SEC_VIDEO, "zVidResFullscreenY", 600);
    const int bpp = zoptions->ReadInt(zOPT_SEC_VIDEO, "zVidResFullscreenBPP", 16);
    int selectedMode = -1;
    int secureMode = -1;
    for (int i = 0; i < zrenderer->Vid_GetNumModes(); ++i) {
        zTRnd_VidModeInfo info;
        zrenderer->Vid_GetModeInfo(info, i);
        if (info.xdim <= 0 || info.ydim <= 0 || info.bpp <= 0)
            continue;
        if (info.xdim == width && info.ydim == height && info.bpp == bpp) {
            selectedMode = i;
            zerr->Message("B: VID: Videomode found");
        }
        if (info.xdim == 800 && info.ydim == 600 && info.bpp == 16) {
            secureMode = i;
            zerr->Message("B: VID: Secure videomode found: 800x600x16");
        }
    }
    if (selectedMode < 0) {
        selectedMode = secureMode;
        zoptions->WriteInt(zOPT_SEC_VIDEO, "zVidResFullscreenX", 800);
        zoptions->WriteInt(zOPT_SEC_VIDEO, "zVidResFullscreenY", 600);
        zoptions->WriteInt(zOPT_SEC_VIDEO, "zVidResFullscreenBPP", 16);
    }
    zoptions->WriteInt(zOPT_SEC_INTERNAL, ZOPT_VID_RES, selectedMode);
    const int maxTextureSize = zoptions->ReadInt(zOPT_SEC_VIDEO, "zTexMaxSize", 256);
    float detail = 0.8f;
    if (maxTextureSize <= 32)
        detail = 0.1f;
    else if (maxTextureSize <= 64)
        detail = 0.2f;
    else if (maxTextureSize <= 128)
        detail = 0.4f;
    else if (maxTextureSize <= 256)
        detail = 0.6f;
    else if (maxTextureSize > 512)
        detail = 1.0f;
    zoptions->WriteReal(zOPT_SEC_INTERNAL, "texDetailIndex", detail);
    const int mouseEnabled = zoptions->ReadBool(zOPT_SEC_GAME, "enableMouse", 1);
    float sensitivity = zoptions->ReadReal(zOPT_SEC_GAME, "mouseSensitivity", 0.5f);
    zinput->SetDeviceEnabled(2, mouseEnabled);
    sensitivity = sensitivity * 0.5f + 0.3f;
    zinput->SetMouseSensitivity(sensitivity);
    const int joystickEnabled = zoptions->ReadBool(zOPT_SEC_GAME, "enableJoystick", 0);
    zinput->SetDeviceEnabled(3, joystickEnabled);
}

void CGameManager::PostMenu(zSTRING)
{
    ApplySomeSettings();
    if (gameSession)
        gameSession->UpdateScreenResolution();
    zinput->ProcessInputEvents();
    zinput->ClearKeyBuffer();
    zinput->ResetRepeatKey(1);
}

void CGameManager::ApplySomeSettings()
{
    zerr->Message("B: OPT: Applying settings ...");
    const int oldReadTemp = zoptions->m_bReadTemp;
    zoptions->m_bReadTemp = 0;
    zoptions->ReadBool(zOPT_SEC_INTERNAL, "extendedMenu", 0);
    float textureDetail = zoptions->ReadReal(zOPT_SEC_INTERNAL, "texDetailIndex", 1.0f);
    const int sightValue = zoptions->ReadInt(zOPT_SEC_PERFORMANCE, "sightValue", 4);
    float modelDetail = zoptions->ReadReal(zOPT_SEC_PERFORMANCE, "modelDetail", 0.5f);
    const int skyEffects = zoptions->ReadInt(zOPT_SEC_GAME, "skyEffects", 1);
    int bloodDetail = zoptions->ReadInt(zOPT_SEC_GAME, "bloodDetail", 2);
    const float brightness = zoptions->ReadReal(zOPT_SEC_VIDEO, "zVidBrightness", 0.5f);
    const float contrast = zoptions->ReadReal(zOPT_SEC_VIDEO, "zVidContrast", 0.5f);
    const float gamma = zoptions->ReadReal(zOPT_SEC_VIDEO, "zVidGamma", 0.5f);
    const float soundVolume = zoptions->ReadReal(zOPT_SEC_SOUND, ZOPT_SND_SFX_VOL, 1.0f);
    const float musicVolume = zoptions->ReadReal(zOPT_SEC_SOUND, ZOPT_SND_MUSIC_VOL, 0.8f);
    const int lookaroundInverse = zoptions->ReadBool(zOPT_SEC_GAME, "camLookaroundInverse", 0);
    const int mouseEnabled = zoptions->ReadBool(zOPT_SEC_GAME, "enableMouse", 1);
    float mouseSensitivity = zoptions->ReadReal(zOPT_SEC_GAME, "mouseSensitivity", 0.5f);
    const float farClipScale = zoptions->ReadReal("ENGINE", "zVobFarClipZScale", 4.0f);
    const int provider = zoptions->ReadInt(zOPT_SEC_INTERNAL, "soundProviderIndex", 0);
    const int speaker = zoptions->ReadInt(zOPT_SEC_INTERNAL, "soundSpeakerIndex", 0);
    zCAICamera::s_iLookAroundSgn = (!lookaroundInverse) * 2 - 1;
    zsound->SetProvider(provider);
    zsound->SetSpeakerType(speaker);
    zsound->SetMasterVolume(soundVolume);
    zmusic->SetVolume(musicVolume);
    zinput->SetDeviceEnabled(2, mouseEnabled);
    mouseSensitivity = mouseSensitivity * 0.5f + 0.3f;
    zinput->SetMouseSensitivity(mouseSensitivity);
    if (gameSession && gameSession->GetWorld()) {
        int farPercent = ((int)(farClipScale * 5.0f + 5.0f)) << 2;
        if (farPercent < 20)
            farPercent = 20;
        else if (farPercent > 300)
            farPercent = 300;
        gameSession->GetWorld()->GetActiveSkyControler()->SetFarZScalability(farPercent * 0.01f);
        gameSession->GetWorld()->vobFarClipZScalability = sightValue * 0.221428573f + 0.4f;
    }
    zCParticleFX::s_bAmbientPFXEnabled = zoptions->ReadBool("ENGINE", "zAmbientPFXEnabled", 1);
    zCVob::s_enableAnimations = zoptions->ReadBool("ENGINE", "zWindEnabled", 1);
    zCWorld::s_bAmbientVobsEnabled = zoptions->ReadBool("ENGINE", "zAmbientVobsEnabled", 1);
    zCWorld::s_bWaveAnisEnabled = zoptions->ReadBool("ENGINE", "zWaterAniEnabled", 1);
    zCWorld::s_bEnvMappingEnabled = zoptions->ReadBool("ENGINE", "zEnvMappingEnabled", 1);
    zCWorld::s_bFadeOutFarVerts = zoptions->ReadBool("ENGINE", "zFarClipAlphaFade", 1);
    oCGame::s_bUseOldControls = zoptions->ReadBool("GAME", "useGothic1Controls", 1);
    if (zrenderer)
        zrenderer->SetRadialFog(zoptions->ReadBool("RENDERER_D3D", "zFogRadial", 1));
    if (zCSkyControler::s_activeSkyControler) {
        float cloudShadowScale = zoptions->ReadReal("ENGINE", "zCloudShadowScale", 0.0f);
        if (cloudShadowScale <= 0.0f)
            cloudShadowScale = 0.0f;
        else
            cloudShadowScale = 1.0f;
        zCSkyControler::s_activeSkyControler->SetCloudShadowScale(cloudShadowScale);
    }
    oCGame::s_fUseSpeechReverb = zoptions->ReadReal("GAME", "useSpeechReverbLevel", 1.0f);
    oCNpcFocus::s_iHightlightMeleeFocus = zoptions->ReadInt("GAME", "highlightMeleeFocus", 0);
    oCNpcFocus::s_bHighlightInteractFocus = zoptions->ReadBool("GAME", "highlightInteractFocus", 0);
    zCSkyControler::s_skyEffectsEnabled = skyEffects == 1;
    if (textureDetail < 0.0f)
        textureDetail = 0.0f;
    else if (textureDetail > 1.0f)
        textureDetail = 1.0f;
    int textureSize = 16384;
    if (textureDetail < 0.15f)
        textureSize = 32;
    else if (textureDetail < 0.35f)
        textureSize = 64;
    else if (textureDetail < 0.55f)
        textureSize = 128;
    else if (textureDetail < 0.75f)
        textureSize = 256;
    else if (textureDetail < 0.95f)
        textureSize = 512;
    if (zoptions->ReadInt(zOPT_SEC_VIDEO, "zTexMaxSize", -1) != textureSize) {
        zCTexture::RefreshTexMaxSize(textureSize);
        if (zresMan)
            zresMan->PurgeCaches(&zCTexture::classDef);
    }
    if (modelDetail < 0.0f)
        modelDetail = 0.0f;
    else if (modelDetail > 1.0f)
        modelDetail = 1.0f;
    zCProgMeshProto::SetLODParamStrengthModifier(modelDetail);
    if (bloodDetail < 0)
        bloodDetail = 0;
    else if (bloodDetail > 3)
        bloodDetail = 3;
    oCNpc::modeBlood = bloodDetail;
    int subTitles = zoptions->ReadBool(zOPT_SEC_GAME, "subTitles", 0);
    if (zgameoptions && zgameoptions->ReadBool("OPTIONS", "force_Subtitles", 0))
        subTitles = 1;
    oCNpc::isEnabledTalkBox = subTitles;
    oCNpc::isEnabledTalkBoxPlayer = zoptions->ReadBool(zOPT_SEC_GAME, "subTitlesPlayer", 1);
    oCNpc::isEnabledTalkBoxAmbient = zoptions->ReadBool(zOPT_SEC_GAME, "subTitlesAmbient", 1);
    oCNpc::isEnabledTalkBoxNoise = zoptions->ReadBool(zOPT_SEC_GAME, "subTitlesNoise", 0);
    oCInformationManager::GetInformationManager().UpdateViewSettings();
    if (ogame)
        ogame->UpdateViewSettings();
    if (zrenderer)
        zrenderer->Vid_SetGammaCorrection(contrast, gamma, brightness);
    zoptions->m_bReadTemp = oldReadTemp;
    zoptions->Save("Gothic.ini");
}

void CGameManager::Menu(int noMenu)
{
    zerr->Message("B: GMAN: Entering Menu-Section");
    zinput->ResetRepeatKey(1);
    gameSession->SetShowPlayerStatus(0);
    zoptions->WriteString("internal", "menuAction", "RESUME_GAME");
    PreMenu();
    const int commandLineNoMenu = zoptions->Parm("NOMENU");
    const int inGame = gameSession && gameSession->GetCamera();
    int processAction = 1;
    zSTRING action;
    if (inGame && commandLineNoMenu) {
        exitGame = 1;
        processAction = 0;
    } else {
        if (!inGame && commandLineNoMenu) {
            action = "NEW_GAME";
        } else {
            if (inGame) {
                gameSession->Pause(exitSession);
                zCMenu::inGameMenu = noMenu;
            }
            zCMenu* menu = zCMenu::Create("MENU_MAIN");
            menu->Run();
            menu->Release();
            zCMenu::inGameMenu = 0;
            if (gameSession && gameSession->GetCamera())
                gameSession->Unpause(exitSession);
            action = zoptions->ReadString("internal", "menuAction", "");
        }
        if (zrenderer && screen)
            screen->DrawItems();
        zerr->Message(zSTRING("B: MENU: ") + action);
        if (action == "LEAVE_GAME") {
            exitGame = 1;
            zoptions->WriteBool(zOPT_SEC_INTERNAL, "gameAbnormalExit", 0);
        } else if (action == "NEW_GAME") {
            zSTRING world;
            if (zgameoptions)
                world = zgameoptions->ReadString(zOPT_SEC_SETTINGS, "World", "");
            else
                world = zoptions->ReadString(zOPT_SEC_INTERNAL, "gamePath", "");
            if (world.IsEmpty())
                world = "NewWorld\\NewWorld.zen";
            GameSessionReset();
            exitSession = 0;
            zerr->Message("B: GMAN: Loading the World ...");
            gameSession->LoadGame(-2, world);
            if (zmusic)
                zmusic->Stop();
            playTime = 0;
        } else if (action == "SAVEGAME_LOAD") {
            Read_Savegame(menu_load_savegame->GetSelectedSlot());
        } else if (action == "SAVEGAME_SAVE") {
            Write_Savegame(menu_save_savegame->GetSelectedSlot());
        } else if (action != "RESUME_GAME") {
            zerr->Warning(zSTRING("C: oGameManager.cpp(CGameManager::Menu()): Menu Selection not known :") + action);
        }
    }
    if (processAction) {
        zoptions->Save("Gothic.ini");
        introActive = 0;
        PostMenu(action);
    }
    if (!exitSession && gameSession)
        gameSession->SetShowPlayerStatus(1);
    zinput->ProcessInputEvents();
    zinput->ClearKeyBuffer();
    zinput->ResetRepeatKey(1);
    zerr->Message("B: GMAN: Leaving Menu-Section");
}

int CGameManager::HandleCancelKey(int)
{
    if (!gameSession || !gameSession->GetCamera() || !gameSession->GetWorld() || !oCNpc::player)
        return 0;
    zCCSPlayer* player = gameSession->GetWorld()->csPlayer;
    if (!player)
        return 0;
    zCCSCutsceneContext* cutscene = player->GetPlayingGlobalCutscene();
    if (cutscene) {
        if (cutscene->TryToCancel())
            zerr->Message("B: CS: User canceled global cutscene");
        else
            zerr->Message("B: CS: User tried to cancel global cutscene: Not allowed at this moment!");
        return 1;
    }
    oCNpc* talkingWith = oCNpc::player->GetTalkingWith();
    if (!talkingWith)
        return 0;
    int stopped = player->StopAllOutputUnits(oCNpc::player);
    stopped += player->StopAllOutputUnits(talkingWith);
    oCNpc::player->StopAllVoices();
    talkingWith->StopAllVoices();
    oCNpc::player->StopFaceAni("NPC_TALK_ANI");
    talkingWith->StopFaceAni("NPC_TALK_ANI");
    return stopped > 0;
}

int CGameManager::MenuEnabled(int& allowMenu)
{
    allowMenu = 1;
    if (!oCNpc::player || !gameSession || !gameSession->GetCamera())
        return 0;
    if (gameSession->GetWorld()->csPlayer->GetPlayingGlobalCutscene())
        return 0;
    const int bodyState = oCNpc::player->GetBodyState();
    if (bodyState == 10 || bodyState == 13 || bodyState == 20)
        return 0;
    if (bodyState == 15 || bodyState == 16 || bodyState == 11 || bodyState == 12 ||
        bodyState == 14 || bodyState == 17 || bodyState == 18)
        allowMenu = 0;
    if (bodyState == 28 || oCNpc::player->HasBodyStateModifier(0x800)
        || oCNpc::player->HasBodyStateModifier(0x2000)
        || oCNpc::player->HasBodyStateModifier(0x1000) || oCNpc::player->anictrl->IsDead())
        allowMenu = 0;
    oCNpc* talkingWith = oCNpc::player->GetTalkingWith();
    const int talkState = parser.GetIndex("ZS_TALK");
    const int dialogFinished = (talkingWith && talkingWith->state.GetState() == talkState)
        || oCInformationManager::GetInformationManager().HasFinished();
    if (!dialogFinished)
        return 0;
    if (oCNpc::game_mode || oCNpc::player->inventory2.IsOpen())
        return 0;
    return 1;
}

int CGameManager::HandleEvent(int key)
{
    if (!gameSession)
        return 0;
    int handled = 0;
    if (key == 1) {
        int allowMenu = 1;
        if (zinput->KeyPressed(0x2a))
            Menu(0);
        else if (MenuEnabled(allowMenu))
            Menu(allowMenu);
        else
            HandleCancelKey(key);
        handled = 1;
    } else if (key == 0x20d) {
        handled = HandleCancelKey(key);
        if (handled) {
            zinput->ClearKeyBuffer();
            zinput->ResetRepeatKey(1);
        }
    } else if (key == 0x43 && s_bUseQuickSave && zinput->KeyToggled(key)
        && zinput->KeyPressed(0x1d) != !gameSession->game_testmode) {
        Read_Savegame(0);
        handled = 1;
    } else if (key == 0x3f && s_bUseQuickSave && zinput->KeyToggled(key)
        && gameSession->GetCamera() && zinput->KeyPressed(0x1d) != !gameSession->game_testmode) {
        int allowMenu = 0;
        if (MenuEnabled(allowMenu)) {
            Write_Savegame(0);
            handled = 1;
        }
    } else if (oCGame::s_bUsePotionKeys && zinput->KeyToggled(key)
        && gameSession->GetCamera() && !gameSession->game_testmode && oCNpc::player) {
        const int logicalKey = zinput->GetFirstBindedLogicalKey(key);
        if (logicalKey == 0x1c || logicalKey == 0x1d) {
            const int functionIndex = parser.GetIndex(logicalKey == 0x1c
                ? "PLAYER_HOTKEY_LAME_POTION" : "PLAYER_HOTKEY_LAME_HEAL");
            if (functionIndex > 0) {
                oCMsgManipulate* message = new oCMsgManipulate(oCMsgManipulate::EV_CALLSCRIPT,
                    functionIndex);
                zCClassDef::ObjectCreated(message, oCMsgManipulate::classDef);
                oCNpc::player->GetEM(1)->OnMessage(message, oCNpc::player);
                handled = 1;
            }
        }
    }
    return handled;
}

oCGame* CGameManager::GetGame()
{
    return gameSession;
}
