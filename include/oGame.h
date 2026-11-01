#ifndef OGAME_H
#define OGAME_H

#include "zVob.h"
#include "zSession.h"
#include "zRenderer.h"
#include "oGameInfo.h"

class oCAIVobMove;

class oCVob : public zCVob {
public:

    oCVob();
    virtual ~oCVob();
    virtual zCClassDef* _GetClassDef() const;
    virtual void GetSoundMaterial_AM(zCSoundManager::zTSndManMedium& medium,
        oTSndMaterial& material, int slot);
    virtual void Init();
    int GetScriptInstance(zSTRING*& name, int& instance);
    virtual int GetInstance();
    virtual zSTRING GetInstanceName();
    void StartParticleEffects(zCTree<zCVob>* node);
    int HasNpcEnoughSpace(zVEC3& position);
    int HasEnoughSpace(zVEC3& position);
    int SearchNpcPosition(zVEC3& position);
    int SearchFreePosition(zVEC3& position, zCVob* originVob);
    int GetFloorPosition(zVEC3& position);
    int GetFloorPositionWithVob(zVEC3& position, zCVob*& floorVob);
    void SetOnFloor(zVEC3& position);
    int GetShowDebug();
    void ToggleShowDebug();
    void SetShowDebug(int enabled);
    static void ShowVobDebugInfo(zCCamera* camera);
    virtual void ShowDebugInfo(zCCamera* camera);
    virtual int DoFocusCheckBBox();
    virtual void SetSoundMaterial(oTSndMaterial material);
    virtual oTSndMaterial GetSoundMaterial();
    virtual int IsOwnedByGuild(int guild);
    virtual int IsOwnedByNpc(int npcInstance);
    virtual int GetCSStateFlags();
    virtual oCAIVobMove* GetAIVobMove();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual int Render(zTRenderContext& context);
    static void ClearDebugList();

    static zCClassDef* classDef;
    static zCList<oCVob> debuglist;
};

class oCTouchDamage : public zCTouchDamage {
public:

    virtual ~oCTouchDamage();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual const char* GetDamageTypeArcEnum(unsigned long index);

    static zCClassDef* classDef;
};

class zCVobSpot : public zCVob {
public:

    zCVobSpot();
    virtual ~zCVobSpot();
    virtual zCClassDef* _GetClassDef() const;
    int IsAvailable(zCVob* vob);
    void MarkAsUsed(float time, zCVob* vob);
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);

    float timerEnd;
    zCVob* inUseVob;

    static zCClassDef* classDef;
};

void oUlfi_StartUp();
void oUlfi_ShutDown();
oCVob* SearchVobByInstance(zCTree<zCVob>* node, int& instance);

template <class T>
class zCListSort;
class oCNpc;
class oCItem;

class zCPlayerInfo;

class zCPlayerGroup {
public:

    zCPlayerGroup() {}
    zCPlayerGroup(const zSTRING& name);
    virtual ~zCPlayerGroup();
    void AddGroupToList();
    void RemoveGroupFromList();
    void ResetPlayerList();
    int AddPlayer(zCPlayerInfo* player);
    int RemPlayer(zCPlayerInfo* player);
    int IsMember(zCPlayerInfo* player);
    int IsMember(zCPlayerGroup* group);
    zCPlayerInfo* GetFirstPlayer() { return playerList.next ? playerList.next->data : NULL; }
    zCPlayerInfo* GetFirstPlayer(int& index);
    zCPlayerInfo* GetNextPlayer(int& index);
    static zCPlayerGroup* GetFirstGroup(int& index);
    zCPlayerGroup* GetNextGroup(int& index);
    zCPlayerInfo* GetPlayerByID(int id);
    void ShowDebugInfo();
    static void ResetGroupList();
    zCPlayerInfo* GetNearestPlayerToVob(zCVob* vob, zCPlayerInfo* exclude);

    zSTRING name;
    zCList<zCPlayerInfo> playerList;

    static zCPlayerGroup allPlayerGroup;
    static zCList<zCPlayerGroup> group_list;
};

class oCWorld : public zCWorld {
public:

    oCWorld();
    virtual ~oCWorld();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    oCVob* CreateVob(zTVobType type, const zSTRING& name);
    oCVob* CreateVob(zTVobType type, int instance);
    zSTRING GetWorldFilename();
    zSTRING GetWorldName();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual int SaveWorld(const zSTRING& fileName, zTWorldSaveMode saveMode, int saveGame,
        int export3DS);
    virtual int LoadWorld(const zSTRING& fileName, zTWorldLoadMode loadMode);
    virtual zCTree<zCVob>* AddVobAsChild(zCVob* vob, zCTree<zCVob>* parent);
    virtual void RemoveVob(zCVob* vob);
    virtual void InsertVobInWorld(zCVob* vob);
    virtual void EnableVob(zCVob* vob, zCVob* parent);
    virtual void DisableVob(zCVob* vob);
    virtual zCVob* SearchVob(zCVob* vob, zCTree<zCVob>* node);
    virtual zCVob* SearchVobByID(unsigned long id, zCTree<zCVob>* node);
    virtual zCVob* SearchVobByName(const zSTRING& name);
    virtual void SearchVobListByName(const zSTRING& name, zCArray<zCVob*>& vobs);
    virtual void TraverseVobList(zCVobCallback& callback, void* data);
    void ClearNpcPerceptionVobLists();
    virtual void DisposeVobs();
    virtual void DisposeWorld();
    void InsertInLists(zCVob* vob);
    void RemoveFromLists(zCVob* vob);

    zSTRING worldFilename;
    zSTRING worldName;
    zCListSort<zCVob>* voblist;
    zCListSort<oCNpc>* voblist_npcs;
    zCListSort<oCItem>* voblist_items;

    static zCClassDef* classDef;
};

class oCNpc;
class zCParser;
class oCSavegameInfo;
class oCSavegameManager {
public:

    oCSavegameManager();
    virtual ~oCSavegameManager();
    void Init();
    void Reinit();
    void SetAndWriteSavegame(int slot, oCSavegameInfo* info);
    void CopyToCurrent(int slot);
    void CopyFromCurrent(int slot);
    void ClearCurrent();
    oCSavegameInfo* GetSavegame(int slot);
    void CleanResources();
    zSTRING GetSlotDirName(int slot);

    zCArray<oCSavegameInfo*> infoList;
    oCSavegameInfo* localInfo;
    int initialized;
};

class oCMenuSavegame;

#include "oParticleControl.h"

extern oCParticleControl* pfxc;
void Game_Particle_Changed(const zSTRING& name);
void Game_OpenParticleConsole(zSTRING& name);
void Game_CloseParticleConsole();
void Game_DeleteAllPfx(zCTree<zCVob>* node);

class oCViewStatusBar;
class oCBinkPlayer;
class CGameManager {
public:

    CGameManager();
    ~CGameManager();
    void Tool_ConvertData();
    void Init(void* window);
    void Run();
    void Done();
    void InitScreen_Open();
    void InitScreen_Menu();
    void InitScreen_Close();
    void InitSettings();
    void ShowSplashScreen();
    void RemoveSplashScreen();
    void GameInit();
    void GameDone();
    void GameSessionReset();
    void GameSessionInit();
    void GameSessionDone();
    void ShowIntro();
    void ShowExtro();
    void PreMenu();
    void PostMenu(zSTRING command);
    void ApplySomeSettings();
    void Menu(int noMenu);
    void Read_Savegame(int slot);
    void Write_Savegame(int slot);
    int HandleCancelKey(int key);
    int MenuEnabled(int& allowMenu);
    virtual int HandleEvent(int key);
    oCGame* GetGame();
    void ShowRealPlayTime();
    int PlayVideo(zSTRING fileName);
    int PlayVideoEx(zSTRING fileName, short first, int second);

    void* sysContextHandle;
    oCGame* gameSession;
    oCGame* backLoop;
    int exitGame;
    int exitSession;
    oCSavegameManager* savegameManager;
    zCArray<zSTRING> lastDatFileList;
    zCArray<zSTRING> lastWorldList;
    zSTRING backWorldRunning;
    zSTRING backDatFileRunning;
    zCView* vidScreen;
    zCView* initScreen;
    int introActive;
    oCBinkPlayer* videoPlayer;
    int videoPlayInGame;
    oCMenuSavegame* menu_save_savegame;
    oCMenuSavegame* menu_load_savegame;
    int playTime;
};
extern CGameManager* gameMan;
class zCOptionSection;
class zCOption {
public:

    zCOption();
    virtual ~zCOption();
    int Load(zSTRING fileName);
    int ChangeDir(int path);
    zSTRING GetDirString(int path);
    zSTRING ReadString(const zSTRING& section, const zSTRING& name, const zSTRING& defaultValue);
    zSTRING ReadString(const zSTRING& section, const char* name, const char* defaultValue);
    int WriteString(const zSTRING& section, const zSTRING& name, const zSTRING& value);
    int WriteString(const zSTRING& section, const zSTRING& name, zSTRING value, int temp);
    int WriteString(const zSTRING& section, const char* name, zSTRING value, int temp);
    int ReadBool(const zSTRING& section, const zSTRING& name, int defaultValue);
    int ReadBool(const zSTRING& section, const char* name, int defaultValue);
    int WriteBool(const zSTRING& section, const zSTRING& name, int value);
    int WriteBool(const zSTRING& section, const zSTRING& name, int value, int temp);
    int WriteBool(const zSTRING& section, const char* name, int value, int temp);
    int ReadInt(const zSTRING& section, const zSTRING& name, int defaultValue);
    int ReadInt(const zSTRING& section, const char* name, int defaultValue);
    int WriteInt(const zSTRING& section, const zSTRING& name, int value);
    int WriteInt(const zSTRING& section, const zSTRING& name, int value, int temp);
    int WriteInt(const zSTRING& section, const char* name, int value, int temp);
    unsigned long ReadDWord(const zSTRING& section, const zSTRING& name, unsigned long defaultValue);
    unsigned long ReadDWord(const zSTRING& section, const char* name, unsigned long defaultValue);
    int WriteDWord(const zSTRING& section, const zSTRING& name, unsigned long value, int temp);
    int WriteDWord(const zSTRING& section, const char* name, unsigned long value, int temp);
    float ReadReal(const zSTRING& section, const zSTRING& name, float defaultValue);
    float ReadReal(const zSTRING& section, const char* name, float defaultValue);
    int WriteReal(const zSTRING& section, const zSTRING& name, float value);
    int WriteReal(const zSTRING& section, const zSTRING& name, float value, int temp);
    int WriteReal(const zSTRING& section, const char* name, float value, int temp);
    zCOptionSection* GetSectionByName(const zSTRING& name, int create);
    zCOptionEntry* GetEntryByName(zCOptionSection* section, const zSTRING& name, int create);
    int EntryExists(const zSTRING& section, const char* name);
    unsigned short GetNumEntries(zCOptionSection* section);
    zCOptionEntry* GetEntry(zCOptionSection* section, int index);
    int RemoveEntry(const zSTRING& section, const char* name);
    void SetFlag(const zSTRING& section, const zSTRING& name, int enabled);
    void SetFlag(const zSTRING& section, const char* name, int enabled);
    int SectionExists(const zSTRING& name);
    unsigned short GetNumSections();
    zCOptionSection* GetSection(int index);
    int RemoveSection(const zSTRING& name);
    void InsertChangeHandler(const zSTRING& section, const char* name,
        int (*handler)(zCOptionEntry&));
    int WriteRaw(const zSTRING& section, const zSTRING& name, void* data, int size, int temp);
    int WriteRaw(const zSTRING& section, const char* name, void* data, int size, int temp);
    int ReadRaw(const zSTRING& section, const zSTRING& name, void* data, int size);
    int ReadRaw(const zSTRING& section, const char* name, void*& data, void* defaultData, int size);
    int AddParameters(zSTRING parameters);
    int Init(zSTRING parameters, bool initialize);
    zFILE* GetDir(int path);
    int Parm(const zSTRING& name);
    zSTRING ParmValue(const zSTRING& name);
    zSTRING ParmValueRaw(const zSTRING& name);
    int ParmValueInt(const zSTRING& name);
    float ParmValueFloat(const zSTRING& name);
    int Save(const zSTRING& fileName);

    int m_bReadTemp;
    zCArray<zCOptionSection*> sectionList;
    zFILE* directory[26];
    zSTRING dir_string[26];
    zSTRING commandline;
};
extern zCOption* zoptions;
extern zSTRING zOPT_SEC_GAME;
extern zSTRING zOPT_SEC_PERFORMANCE;
extern zSTRING zOPT_SEC_VIDEO;
extern zSTRING zOPT_SEC_INTERNAL;
extern zSTRING zOPT_SEC_SOUND;
extern zSTRING zOPT_SEC_KEYS;
extern zSTRING ZOPT_VID_RES;
extern zSTRING ZOPT_SND_SFX_VOL;
extern zSTRING ZOPT_SND_MUSIC_VOL;
class oCWorldTimer {
public:

    oCWorldTimer();
    ~oCWorldTimer();
    void SetDay(int day);
    int GetDay() const;
    void SetTime(int hour, int minute);
    void GetTime(int& hour, int& minute) const;
    void SetFullTime(float time);
    void Timer();
    float GetSkyTime() const;
    zSTRING GetTimeString() const;
    int IsLater(int hour, int minute) const;
    int IsLaterEqual(int hour, int minute) const;
    int IsTimeBetween(int hour1, int min1, int hour2, int min2) const;
    int IsDay() const;
    int IsNight() const;
    int GetPassedTime(float time) const;
    float GetFullTime() const;
    static void AddTime(int& hour, int& minute, int addHour, int addMinute);

    float worldTime;
    int day;
};
class oCMobInter {
public:

    static void SetAllMobsToState(int stateNum, zCWorld* world);
};
#include "oRtn.h"
class oCGuilds {
public:

    oCGuilds();
    ~oCGuilds();
    int     GetAttitude(int firstGuild, int secondGuild);
    void    SetAttitude(int firstGuild, int secondGuild, unsigned char value);
    void    InitGuildTable(const zSTRING& name);
    zSTRING GetGuildName(int guild);
    void    SaveGuildTable(zCArchiver& archiver);
    void    LoadGuildTable(zCArchiver& archiver);

    int size;
    unsigned char* attitude;
};
class oCInfoManager;
class oCNewsManager;
#include "oSvm.h"
class oCTradeManager;
class oCPortalRoomManager {
public:

    void CleanUp();
};
class oCSpawnManager {
public:

    void         SpawnImmediately(int enabled);
    void         ClearList();
    void         CheckInsertNpcs();
    void         Archive(zCArchiver& archiver);
    void         Unarchive(zCArchiver& archiver);
    static void  SetSpawnTime(float time);
    static float GetSpawnTime();
    static void  SetInsertRange(float range);
    static float GetInsertRange();
    static void  SetRemoveRange(float range);
    static float GetRemoveRange();
};

class oCViewGothicProgressBar : public zCViewProgressBar {
public:

    virtual ~oCViewGothicProgressBar();
    virtual void Init();
    virtual void HandleChange();

    int progressbar_sizex;
};

enum oHEROSTATUS {
    oHEROSTATUS_NORMAL
};

class oCGame : public zCSession {
public:

    enum oEGameDialogView {
        GAME_VIEW_SCREEN,
        GAME_VIEW_CONVERSATION,
        GAME_VIEW_AMBIENT,
        GAME_VIEW_CINEMA,
        GAME_VIEW_CHOICE,
        GAME_VIEW_NOISE,
        GAME_VIEW_MAX
    };

    struct TObjectRoutine {
        void Release();
        ~TObjectRoutine();

        zSTRING objName;
        int stateNum;
        int hour1;
        int min1;
        int type;
    };

    oCGame();
    virtual ~oCGame();
    virtual void WorldInit();
    virtual void CamInit();
    virtual void CamInit(zCVob* vob, zCCamera* camera);
    void SetCameraPosition();
    virtual void DesktopInit();
    void UpdateViewSettings();
    virtual void NpcInit();
    virtual void NpcInit(zCTree<zCVob>* node);
    virtual void Done();
    virtual void Init();
    virtual void SetGameInfo(oCGameInfo* info);
    void CallScriptStartup();
    void CallScriptInit();
    virtual void OpenSavescreen(bool showProgress);
    virtual void CloseSavescreen();
    virtual void OpenLoadscreen(bool showProgress, zSTRING worldName);
    virtual void CloseLoadscreen();
    virtual oCWorld* GetGameWorld();
    oCNpc* GetSelfPlayerVob();
    virtual zCView* GetTextView();
    oCWorldTimer* GetWorldTimer();
    oCInfoManager* GetInfoManager();
    oCNewsManager* GetNewsManager();
    oCSVMManager* GetSVMManager();
    oCTradeManager* GetTradeManager();
    oCGuilds* GetGuilds();
    virtual oCGameInfo* GetGameInfo();
    oCPortalRoomManager* GetPortalRoomManager();
    oCSpawnManager* GetSpawnManager();
    oHEROSTATUS GetHeroStatus();
    void SetShowNews(oCNpc* npc);
    void SetShowPlayerStatus(int enabled);
    int GetShowPlayerStatus();
    void UpdateScreenResolution();
    void UpdatePlayerStatus();
    void RemoveHelperVobs(zCTree<zCVob>* node);
    void CreateVobList(zCList<zCVob>& list, zCTree<zCVob>* node, zTVobType type);
    virtual void SetAsPlayer(const zSTRING& instanceName);
    void SetupPlayers(oCNpc*& hero, const zSTRING& instanceName);
    virtual void SetRanges();
    virtual void SetRangesByCommandLine();
    virtual void EnvironmentInit();
    virtual int LoadParserFile(const zSTRING& fileName);
    void DefineExternals_Ulfi(zCParser* scriptParser);
    virtual void SetTime(int day, int hour, int minute);
    virtual void GetTime(int& day, int& hour, int& minute);
    virtual int LoadWorld(const zSTRING& fileName, zCWorld::zTWorldLoadMode loadMode);
    void LoadWorld(int saveGame, const zSTRING& fileName);
    virtual void InitWorldSavegame(int& slot, zSTRING& worldName);
    virtual void CompileWorld();
    virtual void LoadWorldStartup(const zSTRING& fileName);
    virtual void LoadWorldStat(zSTRING fileName);
    virtual void LoadWorldDyn(const zSTRING& fileName);
    int IsInWorld(const zSTRING& fileName);
    virtual void SaveWorld(const zSTRING& fileName, zCWorld::zTWorldSaveMode saveMode,
        int flags, int saveGame);
    void SetObjectRoutineTimeChange(int oldDay, int newDay, int hour, int minute);
    oCNpc* RemovePlayerFromWorld();
    void InsertPlayerIntoWorld(oCNpc* insertedPlayer, zMAT4& trafo);
    void PreSaveGameProcessing(int levelChange);
    void PostSaveGameProcessing();
    void WriteSavegame(int slot, int showScreen);
    int CheckObjectConsistency(int expectedNpcs);
    static int ConsoleEvalFunc(const zSTRING& commandLine, zSTRING& result);
    static int Sort_Routine(TObjectRoutine* first, TObjectRoutine* second);
    void InsertObjectRoutine(int stateNum, const zSTRING& objName, int hour, int minute, int type);
    void ResetObjectRoutineList();
    void ClearObjectRoutineList();
    void CheckObjectRoutines();
    virtual void RenderBlit();
    void ToggleShowFreePoints();
    void ShowFreePoints();
    void RefreshNpcs();
    void DeleteTorches();
    void ClearGameState();
    void InitNpcAttitudes();
    void RecurseCacheVobs(zCTree<zCVob>* node);
    void CacheVisualsIn();
    void CacheVisualsOut();
    void LoadGame(int slot, const zSTRING& fileName);
    void LoadSavegame(int slot, int showScreen);
    virtual void ChangeLevel(const zSTRING& worldName, const zSTRING& startPoint);
    virtual void TriggerChangeLevel(const zSTRING& worldName, const zSTRING& startPoint);
    virtual int CheckIfSavegameExists(const zSTRING& fileName);
    virtual void EnterWorld(oCNpc* player, int saveGame, const zSTRING& startPoint);
    virtual void RenderWaynet();
    void ShowDebugInfos();
    virtual void Render();
    virtual void Pause(int showScreen);
    virtual void Unpause();
    void DeleteNpcs(oCNpc* first, oCNpc* second, float range);
    virtual void SetDrawWaynet(int enabled);
    virtual int GetDrawWaynet();
    virtual zVEC3 GetStartPos();
    int IsDebuggingChannel(int channel);
    int IsDebuggingInstance(zCVob* vob);
    int IsDebuggingAllInstances();
    int OpenView(oEGameDialogView view);
    int CloseView(oEGameDialogView view);
    void SwitchCamToNextNpc();
    virtual int HandleEvent(int key);
    int TestKeys(int key);
    int IA_TestWay(int key);

    float cliprange;
    float fogrange;
    int inScriptStartup;
    int inLoadSaveGame;
    int inLevelChange;
    zCView* array_view[GAME_VIEW_MAX];
    int array_view_visible[GAME_VIEW_MAX];
    int array_view_enabled[GAME_VIEW_MAX];
    oCSavegameManager* savegameManager;
    zCView* game_text;
    zCView* load_screen;
    zCView* save_screen;
    zCView* pause_screen;
    oCViewStatusBar* hpBar;
    oCViewStatusBar* swimBar;
    oCViewStatusBar* manaBar;
    oCViewStatusBar* focusBar;
    int showPlayerStatus;
    int game_drawall;
    int game_frameinfo;
    int game_showaniinfo;
    int game_showwaynet;
    int game_testmode;
    int game_editwaynet;
    int game_showtime;
    int game_showranges;
    int drawWayBoxes;
    int scriptStartup;
    int showFreePoints;
    oCNpc* showRoutineNpc;
    int loadNextLevel;
    zSTRING loadNextLevelName;
    zSTRING loadNextLevelStart;
    zVEC3 startpos;
    oCGameInfo* gameInfo;
    zCVobLight* pl_light;
    float pl_lightval;
    oCWorldTimer* wldTimer;
    float timeStep;
    int singleStep;
    oCGuilds* guilds;
    oCInfoManager* infoman;
    oCNewsManager* newsman;
    oCSVMManager* svmman;
    oCTradeManager* trademan;
    oCPortalRoomManager* portalman;
    oCSpawnManager* spawnman;
    float music_delay;
    oCNpc* watchnpc;
    int m_bWorldEntered;
    float m_fEnterWorldTimer;
    int initial_hour;
    int initial_minute;
    zCArray<zCVob*> debugInstances;
    int debugChannels;
    int debugAllInstances;
    int oldRoutineDay;
    zCList<TObjectRoutine> objRoutineList;
    zCList<TObjectRoutine>* currentObjectRoutine;
    zCViewProgressBar* progressBar;
    zCArray<zCVisual*> visualList;

    static int   s_bUseQuickSave;
    static int   s_bUseOldControls;
    static float s_fUseSpeechReverb;
    static int   s_bUseOnlineHelp;
    static int   s_bUsePotionKeys;
};

extern oCGame* ogame;
extern oCRtnManager rtnMan;
extern int game_testmode;
void Game_CreateInstance(zSTRING& instanceName, zSTRING& result);
void Game_GotoWP(const zSTRING& waypoint, zSTRING& result);
void Game_SaveSCPos();
void Game_LoadSCPos();
void Game_ToggleTestMode(int enabled);
void Game_ToggleHoldTime();
void Game_ParserDebug(zSTRING& functionName);
void Game_ParserDebugOff();
void Game_PlaySound(zSTRING& soundName);
void Game_ToggleScreenMode();
void Edit_Changed(const zSTRING& name);
void Game_StartParticleEffects(zCTree<zCVob>* node);
void Game_InitConsole();
void Game_OpenConsole();
void Game_ChangeAIConsole(const zSTRING& name);
void Game_ToggleAIConsole();
void Game_SpeciesChanged(const zSTRING& name);
void Game_OpenSpeciesConsole();
void Game_OpenFightConsole();
void NotifyInfoToSpy(zSTRING& text);
void NotifyInfoToSpy(char* fileName, char* line, char* functionName);
int PlayVideo();
int PlayVideoEx();
void oCarsten_StartUp();
void oCarsten_ShutDown();

#endif
