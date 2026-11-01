#include "include/oGame.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/zConsole.h"
#include "include/zRenderer.h"
#include "include/zMoving.h"
#include "include/zException.h"
#include "include/zObject.h"
#include <windows.h>
#include <cstdlib>
#include <cstring>

int freeLook = 1;
class zCCSCamera {
public:

    static int playing;
};

int zCCSCamera::playing;

int oCarsten_EvalFunc(const zSTRING& commandLine, zSTRING& result);

void Freelook()
{
    float x = 0.0f;
    float y = 0.0f;
    float rotationScale = 0.5f;

    if (zinput->GetDeviceEnabled(2))
        zinput->GetMousePos(x, y);
    else
    {
        static POINT previousPosition;
        POINT position;
        GetCursorPos(&position);
        x = (float)(position.x - previousPosition.x);
        y = (float)(position.y - previousPosition.y);

        if (position.x < 10 || position.y < 10)
        {
            SetCursorPos(630, 470);
            position.x = 630;
            position.y = 470;
        }
        else if (position.x > 635 || position.y > 475)
        {
            SetCursorPos(15, 15);
            position.x = 15;
            position.y = 15;
        }

        previousPosition = position;
        rotationScale = 0.1f;
    }

    if (x != 0.0f)
        zCCamera::activeCam->connectedVob->RotateWorld(zVEC3(0.0f, 1.0f, 0.0f), x * rotationScale);
    if (y != 0.0f)
        zCCamera::activeCam->connectedVob->RotateLocalX(y * rotationScale);
}

void TestKeys_Carsten(int)
{
}

static void AddExceptionInfo(EXCEPTION_DESCR* descriptor, const zSTRING& text)
{
    char* copy = (char*)malloc(text.Length());
    strcpy(copy, text.ToChar());
    descriptor->additionalInfo[descriptor->additionalInfoCount++] = copy;
}

void ExceptionInfoCallback(EXCEPTION_DESCR* descriptor)
{
    AddExceptionInfo(descriptor,
        zSTRING("Gothic II, Parser Version: ") + zSTRING((int)zCParser::GetVersion()));

    char userName[200];
    DWORD userNameLength = sizeof(userName);
    if (!GetUserNameA(userName, &userNameLength))
        lstrcpyA(userName, "Unknown");

    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    MEMORYSTATUS memory;
    memory.dwLength = sizeof(memory);
    GlobalMemoryStatus(&memory);

    unsigned long totalMemory = (memory.dwTotalPhys + 0xFFFFF) >> 20;
    unsigned long freeMemory = (memory.dwAvailPhys + 0xFFFFF) >> 20;
    AddExceptionInfo(descriptor,
        zSTRING("User:  ") + userName + ",  CPUType: " + zSTRING(systemInfo.dwOemId) +
            ",  Mem: " + zSTRING(totalMemory) + " MB total, " + zSTRING(freeMemory) + " MB free");

    if (zCCamera::activeCam && zCCamera::activeCam->connectedVob)
    {
        zMAT4 transform = zCCamera::activeCam->connectedVob->trafoObjToWorld;
        zVEC3 position = transform.GetTranslation();
        zVEC3 direction = transform.GetAtVector();
        AddExceptionInfo(descriptor,
            zSTRING("Camera: Pos(") + zSTRING(position.x, 20) + "/" + zSTRING(position.y, 20) +
                "/" + zSTRING(position.z, 20) + "), At(" + zSTRING(direction.x, 20) + "/" +
                zSTRING(direction.y, 20) + "/" + zSTRING(direction.z, 20) + ")");
    }
}

void oCarsten_PerFrame()
{
    if (freeLook && !zrenderer->Vid_GetScreenMode() && !zCCSCamera::playing)
        Freelook();
}

void oCarsten_StartUp()
{
    oCVisualFX::InitParser();
    zcon.Register("EDIT FX", "edits a special effect");
    zcon.AddEvalFunc(oCVisualFX::FxConsole_EvalFunc);
    zinput->BindKeys(1);
    zinput->BindKeys(0);
    zCExceptionHandler::AddUnhandledExceptionInfoCallback(ExceptionInfoCallback);
    zcon.Register("FIRSTPERSON", "EXPERIMENTAL FIRST PERSON CAMERA");
    zcon.Register("GOTO CAMERA", "SETS THE PLAYER POSITION TO THE CAMERA POSITION");
    zcon.Register("GOTO POS", "SETS THE PLAYER POSITION TO THE GIVEN POSITION (X,Y,Z)");
    zcon.Register("SPAWNMASS", "spawns max [num] random monsters around the player");
    zcon.Register("SPAWNMASS GIGA", "spawns max [num] random giga monsters around the player");
    zcon.Register("DEBUG DAMAGE", "enables/disabled (0/1) damage debug output");
    zcon.Register("REPARSE",
        "reparses game scripts, warning: you have to know what you are doing (you are probably not)");
    zcon.Register("PLAY FX", "plays a special fx");
    zcon.Register("STOP FX", "stops the previously played fx");
    zcon.AddEvalFunc(oCarsten_EvalFunc);

    s_bCheckRoutines = zoptions->Parm("CHECKROUTINES");
    oCNpcFocus::s_iHightlightMeleeFocus = zoptions->ReadInt("GAME", "highlightMeleeFocus", 0);
    oCNpcFocus::s_bHighlightInteractFocus = zoptions->ReadBool("GAME", "highlightInteractFocus", 0);
    oCGame::s_fUseSpeechReverb = zoptions->ReadReal("GAME", "useSpeechReverbLevel", 1.0f);
    oCGame::s_bUsePotionKeys = zoptions->ReadBool("GAME", "usePotionKeys", 0);
    oCGame::s_bUseQuickSave = zoptions->ReadBool("GAME", "useQuickSaveKeys", 0);
    oCItem::s_fGlobalItemZBiasScale =
        zoptions->ReadReal("ENGINE", "zInventoryItemsDistanceScale", 1.3f);
    oCItem::s_bItemEffectEnabled = zoptions->ReadBool("GAME", "itemEffects", 1);

    if (zrenderer)
        zrenderer->SetRadialFog(zoptions->ReadBool("RENDERER_D3D", "zFogRadial", 1));
}

void oCarsten_ShutDown()
{
    oCVisualFX::ExitParser();
}
