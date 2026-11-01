#include "include/zEngine.h"
#include "include/zObject.h"
#include "include/zResource.h"
#include "include/zFile.h"
#include "include/zTools.h"
#include "include/zWin32.h"
#include "include/zTexture.h"
#include "include/zMemory.h"

class zCOption {
public:

    int Parm(const zSTRING& name);
    zSTRING ParmValue(const zSTRING& name);
};

extern zCOption* zoptions;
extern zCFPUControler zfpuControler;

void zInitOptions();
void zDeinitOptions();
void zBert_StartUp();
void zUlfi_StartUp();
void zDieter_StartUp(HWND__** window);
void zCarsten_StartUp(HWND__** window);
void zCarsten_ShutDown();
void zDieter_ShutDown();
void zUlfi_ShutDown();
void zBert_ShutDown();

// EXE: 0x008C295C
zCEngine* zengine = NULL;
// EXE: 0x00AB08A0
zCConsole* edit_con = NULL;
// EXE: 0x00AB08A4
zCConsole* game_cam_con = NULL;
// EXE: 0x008D2A64
zCParser* parserSoundFX = NULL;
// EXE: 0x008D9234
zCParser* parserParticleFX = NULL;
// EXE: 0x008CE6EC
zCParser* parserVisualFX = NULL;
// EXE: 0x008CEAC8
zCParser* parserCamera = NULL;
// EXE: 0x008D1E68
zCParser* parserMenu = NULL;
// EXE: 0x008D2140
zCParser* parserMusic = NULL;

// EXE: 0x00558BA0
zCEngine::zCEngine()
{
}

// EXE: 0x00558BD0
zCEngine::~zCEngine()
{
}

// EXE: 0x00558BE0
int zCEngine::Init(HWND__** window)
{
    zerr.on_exit = sysExit;
    zCClassDef::EndStartup();
    if (!zfactory)
        zfactory = new zCObjectFactory;
    sysEvent();
    zInitOptions();
    zmalloc.Init(zoptions->Parm("ZMEMPROFILER"));
    zFILE_VDFS::InitFileSystem();
    sysEvent();
    zresMan = new zCResourceManager;
    zresMan->SetThreadingEnabled(!zoptions->Parm("ZNORESTHREAD"));

    zerr.Separator("");
    zerr.Report(zERR_TYPE_INFO, 0,
        "D: *** ZenGin v1.00 - developed 1997-2001 Mad Scientists", 0, 1, 137, NULL, NULL);
    zerr.Report(zERR_TYPE_INFO, 0,
        "D: *** Mad Scientists: Hildebrandt, Wohlers, Speckels", 0, 1, 138, NULL, NULL);
    zerr.Report(zERR_TYPE_INFO, 0, "D: *** compiled: Oct 22 2003, release-build",
        0, 1, 145, NULL, NULL);
    zerr.Report(zERR_TYPE_INFO, 0, "D: *** zEngine-Init ...", 0, 1, 146, NULL, NULL);

    zBert_StartUp();
    zUlfi_StartUp();
    sysEvent();
    zDieter_StartUp(window);
    zCarsten_StartUp(window);
    zerr.Report(zERR_TYPE_INFO, 0, "D: *** zEngine-Init ... Done !",
        0, 1, 159, NULL, NULL);
    zerr.Separator("");
    sysEvent();
    if (zoptions->Parm("ZTEXCONVERT"))
        zCTexture::ScanConvertTextures(zoptions->ParmValue("ZTEXCONVERT"));
    zfpuControler.RestoreDefaultControlWord();
    zfpuControler.SaveCurrentControlWord();
    return 1;
}

// EXE: 0x005591E0
void zCObjectFactory::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

// EXE: 0x00559200
int zCEngine::Shutdown()
{
    zerr.Separator("");
    zerr.Report(zERR_TYPE_INFO, 0, "D: *** zEngine-ShutDown ...",
        0, 1, 189, NULL, NULL);
    zCarsten_ShutDown();
    zDieter_ShutDown();
    zUlfi_ShutDown();
    zBert_ShutDown();
    zerr.Separator("");
    zresMan->EndThread();
    delete zresMan;
    zresMan = NULL;
    zDeinitOptions();
    if (zfactory) {
        zfactory->Release();
        zfactory = NULL;
    }
    zFILE_VDFS::DeinitFileSystem();
    zmalloc.Shutdown();
    zerr.Report(zERR_TYPE_INFO, 0, "D: *** zEngine-ShutDown ... Done!",
        0, 1, 218, NULL, NULL);
    return 1;
}
