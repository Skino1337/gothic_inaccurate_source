#include "include/zSoundMan.h"
#include "include/zRenderer.h"
#include "include/zRenderManager.h"
#include "include/zVob.h"
#include "include/zParticle.h"
#include "include/zTools.h"
#include "include/zConsole.h"
#include "include/zMemPool.h"
#include "include/zSession.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/zInput_Win32.h"
#include "include/zMusic.h"
#include "include/zMenu.h"
#include "include/zAiCamera.h"
#include "include/zOption.h"
#include "include/zDiskVdfs.h"
#include <cstdlib>

class zCOption {
public:

    int Parm(const zSTRING& name);
    zSTRING ParmValue(const zSTRING& name);
    int ReadBool(const zSTRING& section, const zSTRING& entry, int defaultValue);
    int ReadInt(const zSTRING& section, const zSTRING& entry, int defaultValue);
    float ReadReal(const zSTRING& section, const zSTRING& entry, float defaultValue);
    void InsertChangeHandler(const zSTRING& section, const char* entry,
        int (*handler)(zCOptionEntry&));
    int WriteBool(const zSTRING& section, const zSTRING& entry, int value, int temp);
    int WriteInt(const zSTRING& section, const zSTRING& entry, int value, int temp);
    int WriteReal(const zSTRING& section, const zSTRING& entry, float value, int temp);
    int WriteString(const zSTRING& section, const zSTRING& entry, zSTRING value, int temp);
    int RemoveEntry(const zSTRING& section, const char* entry);
    int Init(zSTRING commandLine, bool readOnly = false);
    void ChangeDir(int path);
    int Load(zSTRING fileName);
    int Save(zSTRING fileName);
};

extern zSTRING zOPT_SEC_GAME;
extern zSTRING zOPT_SEC_INTERNAL;
extern zSTRING zOPT_SEC_PERFORMANCE;
zCOption* zgameoptions = NULL;
zSTRING sysCommandLine;

class zCRnd_D3D : public zCRenderer {
};

extern zCOption* zoptions;
extern zCRenderManager zrenderMan;
extern zCSoundManager* zsndMan;

void zDieter_StartUp(HWND__** window)
{
    if (zoptions->Parm(zSTRING("ZMAXFRAMERATE")))
        ztimer.LimitFPS(std::atoi(zoptions->ParmValue(zSTRING("ZMAXFRAMERATE")).ToChar()));
    zCMemPoolBase::DisablePools();
    if (!zrenderer)
        zrenderer = new zCRnd_D3D;
    const int windowed = zoptions->ReadBool(zSTRING("VIDEO"), zSTRING("zStartupWindowed"), 0);
    zrenderer->Vid_SetMode(windowed, window);
    if (!zsound)
        zsound = new zCSoundSystemDummy;
    if (!zsndMan) {
        zsndMan = new zCSoundManager;
        zsndMan->Init();
    }
    zrenderMan.InitRenderManager();
    zCVisual::InitVisualSystem();
    zCVob::InitVobSystem();
    zCVobLight::LoadLightPresets();
    zCParticleFX::InitParticleFX();
}

void zDieter_ShutDown()
{
    zCRenderLightContainer::S_ReleaseVobLightRefs();
    zCProgMeshProto::CleanupProgMeshProto();
    zCParticleFX::CleanupParticleFX();
    zCDecal::CleanupDecals();
    zCVobLight::CleanupVobLight();
    zCLensFlareFX::ReleaseLensFlareScript();
    zCVob::CleanupVobSystem();
    zCVisual::CleanupVisualSystem();
    zCMapDetailTexture::S_Cleanup();
    zrenderMan.CleanupRenderManager();
    zresMan->PurgeCaches(0);
    delete zsndMan;
    zsndMan = 0;
    delete zrenderer;
    zrenderer = 0;
    delete zsound;
    zsound = 0;
}


void zUlfi_StartUp()
{
    zfontman = new zCFontMan;
    zcon.SetPos(0, 0);
    zcon.SetDim(0x1fff, 1500);
    zcon.SetAutoCompletion(1);
    zCParser::enableParsing = zoptions->Parm(zSTRING("ZREPARSE"));
}

void zUlfi_ShutDown()
{
    delete zfontman;
}

void zLocal_Startup()
{
}

void zLocal_Shutdown()
{
}

void zLocal_PerFrame()
{
}


void zInitOptions()
{
    if (zoptions)
        return;

    zoptions = new zCOption;
    zoptions->Init(sysCommandLine);
    zoptions->ChangeDir(0);
    zSTRING fileName = zoptions->ParmValue("ini");
    if (fileName.IsEmpty())
        fileName = "Gothic.ini";
    zoptions->Load(fileName);

    int patchVersion = zoptions->ReadInt(zOPT_SEC_GAME, "PATCHVERSION", 0);
    if (patchVersion < 0)
        patchVersion = 0;
    if (patchVersion == 0)
    {
        zoptions->WriteBool(zOPT_SEC_GAME, "enableJoystick", 0, 0);
        zoptions->WriteReal(zOPT_SEC_GAME, "SHORTKEY1FARPLANEDIST", 0.8f, 0);
        zoptions->WriteReal(zOPT_SEC_GAME, "SHORTKEY2FARPLANEDIST", 1.2f, 0);
        zoptions->WriteReal(zOPT_SEC_GAME, "SHORTKEY3FARPLANEDIST", 2.0f, 0);
        zoptions->WriteReal(zOPT_SEC_GAME, "SHORTKEY4FARPLANEDIST", 3.0f, 0);
    }
    if (patchVersion <= 1)
    {
        zoptions->WriteBool("RENDERER_D3D", "zSyncAmbientCol", 0, 0);
        zoptions->WriteBool(zOPT_SEC_INTERNAL, "logStatistics", 0, 0);
        zoptions->WriteBool(zOPT_SEC_INTERNAL, "extendedMenu", 0, 0);
        zoptions->WriteBool(zOPT_SEC_INTERNAL, "gameAbnormalExit", 0, 0);
        zoptions->WriteReal("ENGINE", "zInventoryItemsDistanceScale", 1.05f, 0);
    }
    if (patchVersion <= 2)
        zoptions->WriteBool("RENDERER_D3D", "radeonHackAmbientColBug", 0, 0);
    if (patchVersion <= 3)
    {
        zoptions->WriteString(zOPT_SEC_INTERNAL, "gameScript", "", 0);
        zoptions->WriteString(zOPT_SEC_INTERNAL, "playerInstanceName", "", 0);
    }
    if (patchVersion <= 4)
    {
        zoptions->WriteBool("RENDERER_D3D", "geforce3HackWBufferBug", 1, 0);
        zoptions->WriteBool("ENGINE", "zSkyRenderFirst", 1, 0);
    }
    zoptions->WriteInt(zOPT_SEC_GAME, "PATCHVERSION", 6, 0);

    if (zoptions->ReadBool(zOPT_SEC_INTERNAL, "gameStartFailed", 0))
        zerr->Report(zERR_TYPE_WARN, 0, "B: GOTHIC: Start of gothic failed before.", 0, 0,
            146, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zzBert.cpp", NULL);
    if (zoptions->ReadBool(zOPT_SEC_INTERNAL, "gameAbnormalExit", 0))
        zerr->Report(zERR_TYPE_WARN, 0, "B: GOTHIC: Abnormal exit of gothic detected.", 0, 0,
            152, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zzBert.cpp", NULL);

    zoptions->WriteBool(zOPT_SEC_INTERNAL, "gameAbnormalExit", 1, 0);
    zoptions->WriteBool(zOPT_SEC_INTERNAL, "gameStartFailed", 1, 0);
    int starts = zoptions->ReadInt(zOPT_SEC_INTERNAL, "gameStarts", 0);
    zoptions->WriteInt(zOPT_SEC_INTERNAL, "gameStarts", starts + 1, 0);
    zoptions->RemoveEntry(zOPT_SEC_INTERNAL, "testmode");
    zoptions->RemoveEntry(zOPT_SEC_GAME, "weatherEffects");
    zoptions->RemoveEntry(zOPT_SEC_INTERNAL, "zFastUnsafeSaveGames");
    zoptions->Save(fileName);

    if (zoptions->Parm("VDFS") && zoptions->ParmValue("VDFS") == "PHYSICALFIRST")
        zFILE_VDFS::s_physical_first = true;
    zoptions->ReadBool(zOPT_SEC_PERFORMANCE, "recalc", 1);
}

void zDeinitOptions()
{
    if (!zoptions)
        return;
    zoptions->WriteBool(zOPT_SEC_INTERNAL, "gameAbnormalExit", 0, 0);
    zoptions->ChangeDir(0);
    zSTRING fileName = zoptions->ParmValue("ini");
    if (fileName.IsEmpty())
        fileName = "Gothic.ini";
    zoptions->Save(fileName);
    delete zoptions;
    zoptions = NULL;
    delete zgameoptions;
    zgameoptions = NULL;
}

static int EvalZErrConsoleCommand(const zSTRING& commandLine, zSTRING& result)
{
    zSTRING command = commandLine.PickWord(1, " ", "\"");
    zSTRING operation = commandLine.PickWord(2, " ", "\"");
    zSTRING value = commandLine.PickWord(3, " ", "\"");
    zSTRING remark = commandLine.PickWord(4, " ", "\"");
    if (command != "ZERR")
        return 0;

    if (operation == "AUTHORS")
    {
        zerr->SetFilterAuthors(value);
        result = "authors: " + zerr->GetFilterAuthors();
    }
    else if (operation == "LEVEL")
    {
        zerr->SearchForSpy();
        zerr->SetFilterLevel(std::atoi(value.ToChar()));
        result = "max.level is now: " + zSTRING(zerr->filter_level);
    }
    else if (operation == "SEARCHSPY")
        result = zerr->SearchForSpy() ? "spy found" : "spy not found";
    else if (operation == "STATUS")
    {
        result = "max.level: " + zSTRING(zerr->filter_level) + "  (authors: " +
            zerr->GetFilterAuthors() + ")\nflags: " + zerr->GetFilterFlagDescription() +
            "\ntargets: " + zerr->GetTargetDescription() + "\n";
    }
    else if (operation == "REM")
    {
        zerr->Separator("");
        zerr->Report(zERR_TYPE_INFO, 0, " Remark: " + remark, 5, 0, 339,
            "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zzBert.cpp", NULL);
        zerr->Separator("");
        result = "added remark \"" + remark + "\"";
    }
    else
        return 0;
    return 1;
}

void zBert_StartUp()
{
    zoptions->ChangeDir(0);
    zcon.Register("ZERR AUTHORS", "Set the author-filter for messages (as characters A-Z)");
    zcon.Register("ZERR SEARCHSPY", "Search for existing zSpy if started later than game.");
    zcon.Register("ZERR LEVEL", "Set the maximum priority-level for messages (from -1 to 10)");
    zcon.Register("ZERR STATUS", "Show error-status (current level, target ...)");
    zcon.Register("ZERR REM", "Include a remark into the error-log.");
    zcon.Register("ZERR ZVIEW", "just for internal tests");
    zcon.AddEvalFunc(EvalZErrConsoleCommand);
}

void zBert_ShutDown()
{
}

int zOpt_Sound_ChangeFXVol(zCOptionEntry& entry);
int zOpt_Sound_ChangeMusicVol(zCOptionEntry& entry);
int zOpt_Video_ChangeVidOptions(zCOptionEntry& entry);

void zCarsten_StartUp(HWND__** window)
{
    zoptions->InsertChangeHandler("SOUND", "zSndFXVol", zOpt_Sound_ChangeFXVol);
    zoptions->InsertChangeHandler("SOUND", "zSndMusicVol", zOpt_Sound_ChangeMusicVol);
    zoptions->InsertChangeHandler("VIDEO", "zVidBrightness", zOpt_Video_ChangeVidOptions);
    zoptions->InsertChangeHandler("VIDEO", "zVidContrast", zOpt_Video_ChangeVidOptions);
    zoptions->InsertChangeHandler("VIDEO", "zVidGamma", zOpt_Video_ChangeVidOptions);
    zoptions->WriteReal("SOUND", "zSndFXVol",
        zoptions->ReadReal("SOUND", "zSndFXVol", 1.0f), 0);
    zoptions->WriteReal("SOUND", "zSndMusicVol",
        zoptions->ReadReal("SOUND", "zSndMusicVol", 0.8f), 0);

    delete zinput;
    zinput = new zCInput_Win32(window);

    zcon.Register("ZTOGGLE SHOWSPLINES", "Toggles camera splines ");
    zcon.Register("ZTOGGLE TIMEDEMO", "Toggles a time demo with avg FPS Rate");
    zcon.Register("ZTOGGLE RENDERORDER", "Renders Sky last (possible better z buffer performance)");
    zcon.Register("ZTOGGLE ENVMAP", "Toggled rendering of environmental fx");
    zcon.Register("ZTOGGLE AMBIENTVOBS", "Toggles rendering of ambient vobs");
    zcon.Register("ZTOGGLE AMBIENTPFX", "Toggles rendering of ambient pfx");
    zcon.Register("ZTOGGLE RENDERPORTALS", "Toggles rendering of all portals (spacer only)");
    zcon.Register("ZSET NEARCLIPZ", "sets the value for the near clipping plane");
    zcon.Register("ZTOGGLE FLUSHONAMBIENTCOL", "toggles flushing the renderer on a ambientcol change");

    zCPolygon::S_InitMorph();
    zCTimer::S_SetMaxFPS(zoptions->ReadInt("ENGINE", "zMaxFPS", zCTimer::S_GetMaxFPS()));
    zCParticleFX::s_bAmbientPFXEnabled = zoptions->ReadBool("ENGINE", "zAmbientPFXEnabled", 1);
    ztimer.SetMaxFPS(zCTimer::S_GetMaxFPS());
}

void zCarsten_ShutDown()
{
    zmusic = NULL;
    zCMenu::Shutdown();
    delete zinput;
    zinput = NULL;
    zCAICamera::GlobalCleanUp();
}

void zCarsten_ShutDown_Fast()
{
    delete zinput;
    zinput = NULL;
}
