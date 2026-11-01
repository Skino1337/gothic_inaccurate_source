#include "include/zAiCamera.h"
#include "include/zAiPlayer.h"
#include "include/oGame.h"
#include "include/zConsole.h"
#include "include/zCsCamera.h"
#include "include/zRenderer.h"
#include "include/zSession.h"
#include "include/zSky.h"
#include <cmath>

extern zCOption* zgameoptions;
static int showDebug;
static zCClassDef zCAICameraClassDef("zCAICamera", "zCAIBase",
    zCAICamera::_CreateNewInstance, 0, 0x29C, 0, "");
zCClassDef* zCAICamera::classDef = &zCAICameraClassDef;
// EXE: 0x008CEAB8
zCAICamera* zCAICamera::current;
// EXE: 0x008CE9BC
zCArray<zCCSCamera*> zCAICamera::dialogCamList;
// EXE: 0x008CEAC0
float zCAICamera::lookingCamLastAzi;
// EXE: 0x008CEAC4
int zCAICamera::lookingCamAziGoalReached;
int zCAICamera::bCreated;
int zCAICamera::bCamChanges;
short zCAICamera::s_iLookAroundSgn = 1;
extern zCParser* parserCamera;
static int cameraState = -1;
zSTRING CamModNormal = "CAMMODNORMAL";
zSTRING CamModRun = "CAMMODRUN";
zSTRING curCamMode;
zSTRING CamModDialog = "CAMMODDIALOG";
zSTRING CamModInventory = "CAMMODINVENTORY";
zSTRING CamModMelee = "CAMMODMELEE";
zSTRING CamModMagic = "CAMMODMAGIC";
zSTRING CamModMeleeMult = "CAMMODMELEEMULT";
zSTRING CamModRanged = "CAMMODRANGED";
zSTRING CamModSwim = "CAMMODSWIM";
zSTRING CamModDive = "CAMMODDIVE";
zSTRING CamModJump = "CAMMODJUMP";
zSTRING CamModJumpUp = "CAMMODJUMPUP";
zSTRING CamModClimb = "CAMMODCLIMB";
zSTRING CamModDeath = "CAMMODDEATH";
zSTRING CamModLook = "CAMMODLOOK";
zSTRING CamModLookBack = "CAMMODLOOKBACK";
zSTRING CamModFocus = "CAMMODFOCUS";
zSTRING CamModRangedShrt = "CAMMODRANGEDSHORT";
zSTRING CamModShoulder = "CAMMODSHOULDER";
zSTRING CamModFirstPerson = "CAMMODFIRSTPERSON";
zSTRING CamModThrow = "CAMMODTHROW";
zSTRING CamModMobLadder = "CAMMODMOBLADDER";
zSTRING CamModFall = "CAMMODFALL";

zCAICamera* zCAICamera::GetCurrent()
{
    return current;
}

zCAICamera::zCAICamera()
{
    bestRange = 2.0f;
    oldCamSys = CamModNormal;
    target = NULL;
    targetAlpha = 1.0f;
    camVob = NULL;
    numTargets = 0;
    showPath = 0;
    d_showDots = 0;
    pathDetectCollision = 1;
    focusVob = NULL;
    underWater = 0;
    inCutsceneMode = 0;
    sysChanged = 1;
    raysCasted = 0;
    rotate = 1;
    translate = 1;
    collision = 1;
    playerIsMovable = 1;
    followIdealPos = 1;
    npcVolumeRangeOffset = 0.0f;
    camDistOffset = 0.0f;
    bCamChanges = 1;
    InitDialogCam();
    moveTracker = zCMovementTracker::GetTracker();
    pathSearch = zCPathSearch::GetSearch();
    evasionSearchReport = zCPathSearch::GetReport();
    pathSearch->SetAICamera(this);
    moveTracker->camai = this;
    moveTracker->TargetChanged();
    pathSearch->TargetChanged();
    moveTracker->CamVobChanged();
    pathSearch->CamVobChanged();
}

zCClassDef* zCAICamera::_GetClassDef() const
{
    return classDef;
}

zCAICamera::~zCAICamera()
{
    if (focusVob)
        focusVob->Release();
    for (int i = 0; i < targetVobList.numInArray; ++i)
        if (targetVobList[i])
            targetVobList[i]->Release();
    targetVobList.Clear();
    numTargets = 0;
    if (camVob)
        camVob->Release();
    if (target)
        target->Release();
    pathSearch = NULL;
    moveTracker = NULL;
    evasionSearchReport = NULL;
    bCreated = 0;
    current = NULL;
}

zCAICamera* zCAICamera::Create()
{
    if (bCreated)
        return current;
    current = new zCAICamera;
    bCreated = 1;
    return current;
}

void zCAICamera::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void zCAICamera::StartUp()
{
    if (parserCamera)
        return;
    zerr->Message("C: *** camera system v0.5");
    parserCamera = new zCParser(100);
    zoptions->ChangeDir(8);
    zSTRING fileName;
    if (zgameoptions)
        fileName = zgameoptions->ReadString("FILES", "Camera", "");
    if (fileName.IsEmpty())
        fileName = "System\\Camera";
    zerr->Report(zERR_TYPE_INFO, 0, "N: CAM: Loading file " + fileName + ".src or .dat",
        4, 1, 0x151, 0, 0);
    parserCamera->Parse(fileName + ".src");
    parserCamera->CreatePCode();
    zSTRING className = "CCAMSYS";
    parserCamera->CheckClassSize(className, 0x5c);
    StartupDialogCam();
    zcon.Register("EDIT CAMERA", "Camerainstanz editieren");
    zcon.Register("TOGGLE NOCLIP", "Toggles collision of camera with world");
    zcon.Register("TOGGLE CAMDEBUG", "Toggles camera debug");
    zcon.Register("TOGGLE INERTIATARGET", "Toggles averaging of target position (for camera)");
    zcon.AddEvalFunc(Console_EvalFunc);
    focusVob = new zCVob;
    focusVob->collButNoMove = 1;
}

void zCAICamera::CleanUp()
{
    if (focusVob)
    {
        focusVob->Release();
        focusVob = NULL;
    }
}

void zCAICamera::GlobalStartUp()
{
}

void zCAICamera::GlobalCleanUp()
{
    if (current)
    {
        current->AddRef();
        current->Release();
        current = NULL;
    }
    delete parserCamera;
    parserCamera = NULL;
    GlobalDialogCamCleanUp();
}

float zCAICamera::GetBestRange()
{
    static float range;
    float offset = targetInPortalRoom && camDistOffset > 1.5f ? 1.5f : camDistOffset;
    range = bestRange + npcVolumeRangeOffset + offset;
    float min = GetMinRange();
    float max = GetMaxRange();
    if (range < min)
        range = min;
    if (range > max)
        range = max;
    return range;
}

float zCAICamera::GetMinRange()
{
    return minRange + npcVolumeRangeOffset;
}

float zCAICamera::GetMaxRange()
{
    return maxRange + npcVolumeRangeOffset;
}

void zCAICamera::SetMode(zSTRING& mode, zCArray<zCVob*> targets)
{
    if (mode == "DIALOG" && !targets.numInArray)
        return;
    SetTargetList(targets);
    if (GetMode() != mode && bCamChanges)
        SetByScript(mode);
}

zSTRING zCAICamera::GetMode()
{
    return curcammode;
}

int zCAICamera::IsModeActive(zSTRING& mode)
{
    return curcammode == mode;
}

int zCAICamera::IsModeAvailable(const zSTRING& mode)
{
    return parserCamera && parserCamera->GetIndex(mode) >= 0;
}

void zCAICamera::ReceiveMsg(int message)
{
    if ((message == 1 || message == 2) && moveTracker)
        moveTracker->ReceiveMsg(message);
}

static void CalcInitialCameraPosition(zCAICamera* camera)
{
    zCMovementTracker* tracker = camera->moveTracker;
    zVEC3 direction = tracker->trafoTStoWS.GetAtVector();
    camera->camSysFirstPos = tracker->playerPos - direction;
    camera->camSysFirstPos = tracker->ApplyRangeToPoint(camera->GetBestRange() * 100.0f,
        camera->camSysFirstPos);
    camera->camSysFirstPos = tracker->ApplyAziToPoint(camera->bestRotY, camera->camSysFirstPos);
    camera->camSysFirstPos = tracker->ApplyElevToPoint(camera->bestRotX, camera->camSysFirstPos);
}

void zCAICamera::SetVob(zCVob* vob)
{
    if (camVob)
        camVob->Release();
    camVob = vob;
    camVob->AddRef();
    if (pathSearch)
        pathSearch->CamVobChanged();
    if (moveTracker)
    {
        moveTracker->CamVobChanged();
        CalcInitialCameraPosition(this);
    }
}

void zCAICamera::SetTarget(zCVob* vob)
{
    if (target)
        target->Release();
    target = vob;
    target->AddRef();
    targetAlpha = 1.0f;
    npcVolumeRangeOffset = 0.0f;

    zCModel* model = dynamic_cast<zCModel*>(target->visual);
    if (model)
    {
        zTBBox3D box = model->GetBBox3D();
        zVEC3 dimensions = box.maxs - box.mins;
        float largest = 0.0f;
        for (int i = 0; i < 3; ++i)
        {
            if (fabs(dimensions.n[i]) > largest)
                largest = fabs(dimensions.n[i]);
        }
        if (fabs(largest - 100.0f) > 10.0f)
            npcVolumeRangeOffset = (largest - 100.0f) * 0.008f;
    }

    if (moveTracker)
        moveTracker->TargetChanged();
    if (pathSearch)
        pathSearch->TargetChanged();
    if (moveTracker)
        CalcInitialCameraPosition(this);
}

void zCAICamera::SetTargetList(zCArray<zCVob*> targets)
{
    lastNumDialogCamTakes = numDialogCamTakes;
    int changed = targets.numInArray != targetVobList.numInArray;
    if (!changed)
    {
        for (int i = 0; i < targets.numInArray; ++i)
            if (targets[i] != targetVobList[i])
            {
                changed = 1;
                break;
            }
    }
    if (changed)
        numOUsSpoken = 0;
    else
        ++numOUsSpoken;

    for (int i = 0; i < targetVobList.numInArray; ++i)
        if (targetVobList[i])
            targetVobList[i]->Release();
    targetVobList = targets;
    for (int i = 0; i < targetVobList.numInArray; ++i)
        targetVobList[i]->AddRef();
    numTargets = targetVobList.numInArray;
    if (GetMode() == "DIALOG" && targets.numInArray >= 2 &&
        (changed || numOUsSpoken > 1))
        StartDialogCam(changed ? 0 : 1);
}

void zCAICamera::ClearTargetList()
{
    for (int i = 0; i < targetVobList.numInArray; ++i)
        if (targetVobList[i])
            targetVobList[i]->Release();
    targetVobList.Clear();
    numTargets = 0;
}

void zCAICamera::AddTarget(zCVob* vob)
{
    if (vob == target || targetVobList.Contains(vob))
        return;
    targetVobList.InsertEnd(vob);
    vob->AddRef();
    ++numTargets;
}

void zCAICamera::SubTarget(zCVob* vob)
{
    if (!targetVobList.Contains(vob))
        return;
    targetVobList.Remove(vob);
    vob->Release();
    --numTargets;
}

int zCAICamera::Console_EvalFunc(const zSTRING& input, zSTRING& message)
{
    static zCConsole* cameraConsole;
    zSTRING command = input.PickWord(1, " ", "");
    command.Upper();
    if (command == "EDIT" && input.Search("CAMERA", 1) >= 0)
    {
        zcon.Hide();
        delete cameraConsole;
        cameraConsole = new zCConsole;
        cameraConsole->SetPos(0, 0);
        cameraConsole->SetParser(parserCamera);
        cameraConsole->SetChangedFunc(ParameterChanged);
        cameraConsole->EditInstance(curcammode, &bestRange);
        cameraConsole->SetFocus();
        return 1;
    }
    if (command != "TOGGLE")
        return 0;
    if (input.Search("NOCLIP", 1) >= 0)
    {
        if (pathSearch)
            pathSearch->collisionEnabled = !pathSearch->collisionEnabled;
        else
            collision = !collision;
        int enabled = pathSearch ? pathSearch->collisionEnabled : collision;
        message = enabled ? "Camera collision enabled" : "Camera collision disabled";
        return 1;
    }
    if (input.Search("CAMDEBUG", 1) >= 0)
    {
        d_showDots = !d_showDots;
        showDebug = !showDebug;
        if (d_showDots)
        {
            InitHelperVobs();
            if (target)
            {
                target->showVisual |= 4;
                target->visualAlpha = 0.1f;
            }
            message = "visualizing camera control dots enabled";
        }
        else
        {
            D_ClrWin();
            DeleteHelperVobs();
            if (target)
            {
                target->showVisual &= ~4;
                target->visualAlpha = 1.0f;
            }
            message = "visualizing camera control dots disabled";
        }
        return 1;
    }
    if (input.Search("INERTIATARGET", 1) >= 0)
    {
        moveTracker->numPreSamplesPlayerPos = moveTracker->numPreSamplesPlayerPos == 1 ? 10 : 1;
        moveTracker->SetPlayerPos(moveTracker->playerPos);
        message = "target tr";
        return 1;
    }
    return 0;
}

void zCAICamera::ParameterChanged()
{
    moveTracker->RecalcRingPositions();
    CalcInitialCameraPosition(this);
    firstPerson = 0;
}

void zCAICamera::CalcFirstPosition()
{
    CalcInitialCameraPosition(this);
}

int zCAICamera::SetByScript(zSTRING& mode)
{
    mode.Upper();
    if (!IsModeAvailable(mode))
        return 0;
    parserCamera->CreateInstance(mode, &bestRange);
    oldCamSys = curcammode;
    curcammode = mode;
    ParameterChanged();
    return 1;
}

zSTRING zCAICamera::SetByScriptLine(zSTRING& line)
{
    line.Upper();
    zSTRING value = line.PickWord(3, " ", " ");
    float number = value.ToFloat();
    if (line.Search("MINRANGE", 1) >= 0) minRange = number;
    else if (line.Search("MAXRANGE", 1) >= 0) maxRange = number;
    else if (line.Search("BESTRANGE", 1) >= 0) bestRange = number;
    else if (line.Search("MINAZIMUTH", 1) >= 0) minRotY = number;
    else if (line.Search("MAXAZIMUTH", 1) >= 0) maxRotY = number;
    else if (line.Search("BESTAZIMUTH", 1) >= 0) bestRotY = number;
    else if (line.Search("MINELEVATION", 1) >= 0) minRotX = number;
    else if (line.Search("MAXELEVATION", 1) >= 0) maxRotX = number;
    else if (line.Search("BESTELEVATION", 1) >= 0) bestRotX = number;
    else if (line.Search("MINROTZ", 1) >= 0) minRotZ = number;
    else if (line.Search("MAXROTZ", 1) >= 0) maxRotZ = number;
    else if (line.Search("BESTROTZ", 1) >= 0) bestRotZ = number;
    else if (line.Search("ROTOFFSETX", 1) >= 0) rotOffsetX = number;
    else if (line.Search("ROTOFFSETY", 1) >= 0) rotOffsetY = number;
    else if (line.Search("ROTOFFSETZ", 1) >= 0) rotOffsetZ = number;
    else if (line.Search("VELOTRANS", 1) >= 0) veloTrans = number;
    else if (line.Search("VELOROT", 1) >= 0) veloRot = number;
    else if (line.Search("TRANSLATE", 1) >= 0) translate = value.ToInt();
    else if (line.Search("ROTATE", 1) >= 0) rotate = value.ToInt();
    else if (line.Search("COLLISION", 1) >= 0) collision = value.ToInt();
    else if (line.Search("INERTIAHEAD", 1) >= 0) moveTracker->inertiaHead = number;
    else if (line.Search("INERTIATRANS", 1) >= 0) moveTracker->inertiaTrans = number;
    else if (line.Search("INERTIATARGETROT", 1) >= 0) moveTracker->inertiaTargetRot = number;
    else if (line.Search("NOCLIP", 1) >= 0 && pathSearch)
    {
        pathSearch->collisionEnabled = !pathSearch->collisionEnabled;
        moveTracker->RecalcRingPositions();
        CalcInitialCameraPosition(this);
        firstPerson = 0;
        return "Collision: " + zSTRING(pathSearch->collisionEnabled);
    }
    else
        return "Unknown command: " + line;
    moveTracker->RecalcRingPositions();
    CalcInitialCameraPosition(this);
    firstPerson = 0;
    return "\nCommand executed!";
}

void zCAICamera::CreateInstance(zSTRING& mode)
{
    parserCamera->CreateInstance(mode, &bestRange);
}

void zCAICamera::AI_LookingCam()
{
    float frameTime = moveTracker->frameTime;
    bestRotX += (zinput->GetState(4) - zinput->GetState(3)) * 90.0f * frameTime;
    bestRotY += (zinput->GetState(2) + zinput->GetState(19) -
        zinput->GetState(1) - zinput->GetState(18)) * 90.0f * frameTime;
    if (bestRotX > 89.0f) bestRotX = 89.0f;
    if (bestRotX < -80.0f) bestRotX = -80.0f;
    if (bestRotY > 90.0f) bestRotY = 90.0f;
    if (bestRotY < -90.0f) bestRotY = -90.0f;
    if (zinput->GetState(23))
        firstPerson = 1;

    int reason = 0;
    zVEC3 candidate = CalcAziElevRange(bestRotX, bestRotY, GetBestRange(), moveTracker->trafoTStoWS);
    if (pathSearch->IsPointValid(candidate, 0x16, reason))
        camSysFirstPos = candidate;
    pathSearch->AdjustCenterSphere(moveTracker->playerPos, camSysFirstPos, 30.0f);

    if (firstPerson)
        AI_FirstPerson();
    else
        AI_Normal();
}

void zCAICamera::AI_FirstPerson()
{
    float mouseX;
    float mouseY;
    zinput->GetMousePos(mouseX, mouseY);
    rotOffsetX += mouseY * 0.6f;
    if (rotOffsetX > 89.0f) rotOffsetX = 89.0f;
    if (rotOffsetX < -89.0f) rotOffsetX = -89.0f;

    zSPathSearchOptions options = {};
    options.searchFlags = 0x16;
    options.minLerpMode = 0x10;
    options.bestScore = 1.0f;
    options.startAzi = -45.0f;
    options.endAzi = 45.0f;
    options.startElev = -45.0f;
    options.endElev = 45.0f;
    pathSearch->FindBestEvasion(options);

    evasionSearchReport->p2 = moveTracker->playerHead;
    evasionSearchReport->p2.n[1] += 10.0f;
    if (GetMode() == CamModInventory && target)
    {
        oCAniCtrl_Human* animation = dynamic_cast<oCAniCtrl_Human*>(target->callback_ai);
        if (!animation || !animation->IsStanding())
            evasionSearchReport->p2 += target->trafoObjToWorld.GetAtVector() * 28.0f;
    }
    pathSearch->AdjustCenterSphere(moveTracker->playerPos, evasionSearchReport->p2, 30.0f);
    moveTracker->InterpolateTo(evasionSearchReport);
}

void zCAICamera::AI_Normal()
{
    evasionSearchReport->p1 = camVob->GetPositionWorld();
    evasionSearchReport->p2 = camSysFirstPos;
    evasionSearchReport->bestLerpMode = 0;
    evasionSearchReport->veloTrans = veloTrans;
    evasionSearchReport->veloRot = veloRot;
    evasionSearchReport->r1.Matrix4ToQuat(camVob->trafoObjToWorld);
    evasionSearchReport->r2.Matrix4ToQuat(camVob->trafoObjToWorld);
    pathSearch->AdjustCenterSphere(evasionSearchReport->p2, moveTracker->playerPos,
        CAMSPHERE_DIAMETER * 0.5f);

    if ((evasionSearchReport->p2 - moveTracker->playerPos).LengthApprox() < 80.0f)
    {
        zMAT4& shoulder = moveTracker->GetShoulderCamMat();
        evasionSearchReport->p2 = shoulder.GetTranslation();
        evasionSearchReport->r2.Matrix4ToQuat(moveTracker->GetShoulderCamMat());
        evasionSearchReport->bestLerpMode = 3;
        int reason = 0;
        if (!pathSearch->IsPointValid(evasionSearchReport->p2, 4, reason))
            evasionSearchReport->p2 = moveTracker->playerHead;
    }
    moveTracker->InterpolateTo(evasionSearchReport);
}

zVEC3& zCAICamera::GetCamSysFirstPos()
{
    return camSysFirstPos;
}

void zCAICamera::SetHintTargetInPortalRoom(int enabled)
{
    if (targetInPortalRoom == enabled)
        return;
    if (!targetInPortalRoom && camDistOffset < 0.5f)
        camDistOffset = 0.5f;
    targetInPortalRoom = enabled;
}

void zCAICamera::CheckKeys()
{
    static int mouseConnected;
    if (zinput->KeyPressed(0x4a))
        camDistOffset -= moveTracker->frameTime * 3.0f;
    else if (zinput->KeyPressed(0x4e))
        camDistOffset += moveTracker->frameTime * 3.0f;
    float minimum = minRange - bestRange - npcVolumeRangeOffset;
    float maximum = maxRange - bestRange - npcVolumeRangeOffset;
    if (camDistOffset < minimum) camDistOffset = minimum;
    if (camDistOffset > maximum) camDistOffset = maximum;

    mouseConnected = zinput->GetDeviceConnected(zINPUT_MOUSE) &&
        zinput->GetDeviceEnabled(zINPUT_MOUSE);
    if (!mouseConnected)
        return;

    int looking = GetMode() == CamModLook;
    int inventory = GetMode() == CamModInventory;
    int climbing = GetMode() == CamModClimb;
    int death = GetMode() == CamModDeath;
    int ladder = GetMode() == CamModMobLadder;
    int diving = GetMode() == CamModDive;
    int lookBack = GetMode() == CamModLookBack;
    int mob = GetMode().Search("CAMMODMOB", 1) >= 0;
    float mouseX;
    float mouseY;
    float mouseWheel;
    zinput->GetMousePos(mouseX, mouseY, mouseWheel);
    if (mouseX < -100.0f) mouseX = -100.0f;
    if (mouseX > 100.0f) mouseX = 100.0f;
    if (mouseY < -100.0f) mouseY = -100.0f;
    if (mouseY > 100.0f) mouseY = 100.0f;
    if (mouseWheel < -100.0f) mouseWheel = -100.0f;
    if (mouseWheel > 100.0f) mouseWheel = 100.0f;

    if (fabs(mouseY) > 0.0f)
    {
        float speed = firstPerson ? 2.0f : 1.5f;
        if (lookBack)
            speed = 0.0f;
        bestRotX += moveTracker->frameTime * speed * mouseY * 5.0f * s_iLookAroundSgn;
    }
    if (fabs(mouseX) > 0.0f && (looking || inventory || climbing || mob || ladder || diving))
    {
        float speed = firstPerson ? 4.0f : 2.0f;
        if (firstPerson)
            mouseX = -mouseX;
        else if (looking || inventory || climbing || mob || ladder)
            speed = 2.5f;
        bestRotY -= moveTracker->frameTime * speed * mouseX * 5.0f;
    }
    if (!zCMenu::GetActive() && !inventory && fabs(mouseWheel) > 0.0f)
    {
        int direction = mouseWheel < 0.0f ? -1 : (mouseWheel > 0.0f ? 1 : 0);
        camDistOffset -= moveTracker->frameTime * direction * 5.0f;
        if (camDistOffset < minimum) camDistOffset = minimum;
        if (camDistOffset > maximum) camDistOffset = maximum;
    }
    if (bestRotX < -60.0f) bestRotX = -60.0f;
    if (bestRotX > 84.999f) bestRotX = 84.999f;
    float minRot = inventory || mob || diving ? -180.0f : -80.0f;
    float maxRot = inventory || mob || diving ? 180.0f : 89.999f;
    if (bestRotY < minRot) bestRotY = minRot;
    if (bestRotY > maxRot) bestRotY = maxRot;
    moveTracker->bMouseUsed = mouseConnected;
}

void zCAICamera::DoAI(zCVob*, int&)
{
    if (ztimer.frameTimeFloat == 0.0f)
        return;
    if (zCCSCamera::playing || GetMode() == CamModDialog)
    {
        if (target)
        {
            target->showVisual &= ~4;
            target->visualAlpha = 1.0f;
        }
        return;
    }

    int autochange;
    zCZoneMusic::GetAutochange(&autochange);
    if (!autochange)
        zCZoneMusic::SetAutochange(1);
    zfpuControler.SaveCurrentControlWord();
    zfpuControler.SetPrecision_64();
    CheckKeys();
    CheckUnderWaterFX();
    CalcInitialCameraPosition(this);
    camVob->BeginMovement();
    moveTracker->Update();
    raysCasted = 0;
    D_Reset();
    CheckKeys();
    if (GetMode() == CamModLook)
        AI_LookingCam();
    else if (GetMode() == CamModFirstPerson)
        AI_FirstPerson();
    else
        AI_Normal();

    if (target)
    {
        float alpha = targetAlpha;
        if (GetMode() != CamModFirstPerson)
        {
            float distance = (moveTracker->playerHead - moveTracker->camPos).LengthApprox();
            float minimum = minRange * 100.0f;
            if (distance < minimum && moveTracker->GetRange() <
                (100.0f + npcVolumeRangeOffset) * 0.8f && minimum > moveTracker->GetRange())
            {
                float fraction = distance / ((100.0f + npcVolumeRangeOffset) * 0.8f);
                if (fraction > 1.0f) fraction = 1.0f;
                alpha = (zSinApprox(fraction * 3.14159274f - 1.57079637f) + 1.0f) * 0.5f;
            }
            if (alpha >= 1.0f)
                target->showVisual &= ~4;
            else
                target->showVisual |= 4;
        }
        target->visualAlpha = alpha;
    }
    camVob->EndMovement(1);
    if (showDebug)
        ShowDebug();
    moveTracker->SaveFrameStates();
    zfpuControler.RestoreSavedControlWord();
}

void zCAICamera::SetRotateEnabled(int enabled)
{
    if (moveTracker)
        moveTracker->SetRotateEnabled(enabled);
}

// EXE: 0x004A56E0
zCObject* zCAICamera::_CreateNewInstance()
{
    zCAICamera* camera = new zCAICamera;
    zCClassDef::ObjectCreated(camera, classDef);
    return camera;
}

zCPathSearch::zCPathSearch()
{
    camai = NULL;
    camVob = NULL;
    target = NULL;
    world = NULL;
    collisionEnabled = 1;
    ignoreSearchFlags = 0;
    moveTracker = NULL;
    searchOptions.searchFlags = 0x16;
    searchOptions.poseFailReason = 1;
    searchOptions.minLerpMode = 0;
    searchOptions.minScore = 0.0f;
    searchOptions.bestScore = 1.0f;
    searchOptions.startAzi = -180.0f;
    searchOptions.startElev = -20.0f;
    searchOptions.startRange = 1.0f;
    searchOptions.endAzi = 180.0f;
    searchOptions.endElev = 20.0f;
    searchOptions.endRange = 4.0f;
    searchOptions.quality = 1.0f;
}

void zCPathSearch::CamVobChanged()
{
    if (camai)
    {
        camVob = camai->camVob;
        world = camVob ? camVob->homeWorld : NULL;
    }
}

void zCPathSearch::TargetChanged()
{
    if (camai)
        target = camai->target;
}

void zCPathSearch::SetAICamera(zCAICamera* camera)
{
    camai = camera;
}

zSPathSearchResult* zCPathSearch::GetReport()
{
    static zSPathSearchResult report;
    return &report;
}

zCPathSearch* zCPathSearch::GetSearch()
{
    static zCPathSearch search;
    return &search;
}

zCPathSearch::~zCPathSearch()
{
    searchCache.DeleteList();
}

int zCPathSearch::SphereSearch(const zSEvasionSearchDesc& desc)
{
    D_Print("AI Sphere", 2);
    bestSearchState.bestLerpMode = 0;
    for (int segment = 0; segment < desc.sphereSearchList.numInArray; ++segment)
    {
        const zSEvasionSearchDesc::zSEvasionSearchOptions& option =
            desc.sphereSearchList[segment];
        bestSearchState.startAzi = option.scStart.n[0];
        bestSearchState.startElev = option.scStart.n[1];
        bestSearchState.startRange = option.scStart.n[2];
        bestSearchState.endAzi = option.scEnd.n[0];
        bestSearchState.endElev = option.scEnd.n[1];
        bestSearchState.endRange = option.scEnd.n[2];
        bestSearchState.sphereSearchName = desc.searchName;
        bestSearchState.sphereSearchSegment = option.scName;

        float starts[3] = { option.scStart.n[0], option.scStart.n[1], option.scStart.n[2] };
        float ends[3] = { option.scEnd.n[0], option.scEnd.n[1], option.scEnd.n[2] };
        float steps[3];
        float increments[3];
        for (int axis = 0; axis < 3; ++axis)
        {
            float quality = option.scQuality.n[axis];
            float scale = axis == 2 ? 0.0025f : (axis == 1 ? 0.006666667f : 0.005555556f);
            int count = (int)(1.0f - quality * scale * 10.0f) + 1;
            if (count < 1) count = 1;
            steps[axis] = (float)count;
            increments[axis] = fabs(ends[axis] - starts[axis]) / count;
            if (ends[axis] < starts[axis])
                increments[axis] = -increments[axis];
        }

        int counts[3];
        for (int i = 0; i < 3; ++i)
        {
            char axis = option.scSequence[i];
            int index = axis == 'A' ? 0 : (axis == 'E' ? 1 : 2);
            counts[i] = index;
        }
        int candidates = 0;
        for (int first = 0; first <= (int)steps[counts[0]]; ++first)
            for (int second = 0; second <= (int)steps[counts[1]]; ++second)
                for (int third = 0; third <= (int)steps[counts[2]]; ++third)
                {
                    int indices[3];
                    indices[counts[0]] = first;
                    indices[counts[1]] = second;
                    indices[counts[2]] = third;
                    bestSearchState.endAzi = starts[0] + increments[0] * indices[0];
                    bestSearchState.endElev = starts[1] + increments[1] * indices[1];
                    bestSearchState.endRange = starts[2] + increments[2] * indices[2];
                    ++candidates;
                    CalcStateScores(bestSearchState);
                    if (bestSearchState.totalScore > lastSearchState.totalScore)
                    {
                        lastSearchState = bestSearchState;
                        return 1;
                    }
                }
    }
    return 0;
}

int zCPathSearch::SwitchSearch(const zSEvasionSearchDesc&)
{
    return 1;
}

int zCPathSearch::ShoulderSearch(const zSEvasionSearchDesc& desc)
{
    D_Print("AI Shoulder", 2);
    bestSearchState.bestLerpMode = 3;
    bestSearchState.trafoCamToWorld = moveTracker->GetShoulderCamMat();
    bestSearchState.foundPos = bestSearchState.trafoCamToWorld.GetTranslation();
    bestSearchState.bestAzi = 0.0f;
    bestSearchState.bestElev = 85.0f;
    bestSearchState.bestRange = zCAICamera::GetCurrent()->npcVolumeRangeOffset + 150.0f;
    bestSearchState.totalScore = 0.00001f;
    bestSearchState.sphereSearchName = desc.searchName;
    bestSearchState.sphereSearchSegment = "Shoulder";
    int reason = 0;
    if (!IsPointValid(bestSearchState.foundPos, searchOptions.searchFlags & ~0x410, reason))
    {
        if (reason == 1)
            return 0;
        if (reason == 2)
        {
            zVEC3 lastValid = moveTracker->GetLastValidWayPoint(zCMovementTracker::zWAYPOINT_CAM);
            bestSearchState.bestAzi = moveTracker->GetAzimuth(lastValid);
            bestSearchState.bestElev = moveTracker->GetElevation(lastValid);
            bestSearchState.bestRange = moveTracker->GetRange(lastValid);
            bestSearchState.totalScore = 0.00000999f;
            bestSearchState.sphereSearchSegment = "Shoulder/Ease To Last Valid Target Pos";
            bestSearchState.bestLerpMode = 1;
            if (collisionEnabled)
            {
                ++camai->raysCasted;
                if (world->TraceRayFirstHit(camVob->GetPositionWorld(),
                    lastValid - camVob->GetPositionWorld(), target, 0x16) ||
                    world->TraceRayFirstHit(moveTracker->playerPos,
                    lastValid - moveTracker->playerPos, target, 0x16))
                    bestSearchState.bestLerpMode = 4;
            }
        }
    }
    zSPathSearchState_Internal* state = NULL;
    searchCache.GetSphereCoordLazy(bestSearchState.bestAzi, bestSearchState.bestElev,
        bestSearchState.bestRange, state);
    if (state)
    {
        state->totalScore = bestSearchState.totalScore;
        state->trafoCamToWorld = bestSearchState.trafoCamToWorld;
    }
    return 1;
}

int zCPathSearch::PanicSearch(const zSEvasionSearchDesc& desc)
{
    D_Print("AI Panic", 2);
    int panic = 0;
    if (desc.searchName != "__FP__" && moveTracker->GetRange() >
        camai->npcVolumeRangeOffset + 1.5f)
    {
        zVEC3 oldPOI = moveTracker->POI_Player;
        moveTracker->POI_Player = moveTracker->playerPos;
        int reason = 0;
        if (IsPointValid(moveTracker->camPos, 8, reason))
        {
            bestSearchState.bestLerpMode = 3;
            bestSearchState.trafoCamToWorld = moveTracker->GetPanicCamMat();
            bestSearchState.foundPos = bestSearchState.trafoCamToWorld.GetTranslation();
            bestSearchState.bestAzi = moveTracker->GetAzimuth();
            bestSearchState.bestElev = moveTracker->GetElevation();
            bestSearchState.bestRange = moveTracker->GetRange();
            bestSearchState.totalScore = 0.0000010001f;
            bestSearchState.sphereSearchName = desc.searchName;
            bestSearchState.sphereSearchSegment = "Panic";
            panic = 1;
        }
        moveTracker->POI_Player = oldPOI;
    }
    if (!panic)
    {
        bestSearchState.bestLerpMode = 3;
        bestSearchState.trafoCamToWorld = moveTracker->GetFirstPersonCamMat();
        bestSearchState.foundPos = bestSearchState.trafoCamToWorld.GetTranslation();
        bestSearchState.bestAzi = 0.0f;
        bestSearchState.bestElev = 0.0f;
        bestSearchState.bestRange = 0.0f;
        bestSearchState.totalScore = 0.000001f;
        bestSearchState.sphereSearchName = desc.searchName;
        bestSearchState.sphereSearchSegment = "First Person";
        int reason = 0;
        if (!IsPointValid(bestSearchState.foundPos, 2, reason))
        {
            zVEC3 lastValid = moveTracker->GetLastValidWayPoint(zCMovementTracker::zWAYPOINT_HEAD);
            bestSearchState.bestAzi = moveTracker->GetAzimuth(lastValid);
            bestSearchState.bestElev = moveTracker->GetElevation(lastValid);
            bestSearchState.bestRange = moveTracker->GetRange(lastValid);
            bestSearchState.sphereSearchSegment = "First Person/Ease To Last Valid Target Pos";
            bestSearchState.bestLerpMode = 1;
            if (collisionEnabled)
            {
                ++camai->raysCasted;
                if (world->TraceRayFirstHit(camVob->GetPositionWorld(),
                    lastValid - camVob->GetPositionWorld(), target, 0x16) ||
                    world->TraceRayFirstHit(bestSearchState.foundPos,
                    lastValid - bestSearchState.foundPos, target, 0x16))
                    bestSearchState.bestLerpMode = 4;
            }
        }
    }
    zSPathSearchState_Internal* state = NULL;
    searchCache.GetSphereCoordLazy(bestSearchState.bestAzi, bestSearchState.bestElev,
        bestSearchState.bestRange, state);
    if (state)
    {
        state->bestLerpMode = bestSearchState.bestLerpMode;
        state->totalScore = bestSearchState.totalScore;
        state->trafoCamToWorld = bestSearchState.trafoCamToWorld;
    }
    return 1;
}

void zCPathSearch::CalcStateScores(zSPathSearchState& state)
{
    static float numSCsSearched;
    static float numSCsCached;
    numSCsSearched += 1.0f;
    zSPathSearchState_Internal* cached = NULL;
    if (searchCache.GetSphereCoordLazy(state.endAzi, state.endElev, state.endRange, cached))
    {
        numSCsCached += 1.0f;
        state.totalScore = cached->totalScore;
        state.bestLerpMode = cached->bestLerpMode;
        return;
    }
    if (cached->maxRange < state.endRange * state.endRange)
    {
        cached->totalScore = 0.0f;
        cached->bestLerpMode = 0;
        state.totalScore = 0.0f;
        state.bestLerpMode = 0;
        return;
    }

    numSCsCached += 1.0f;
    ignoreSearchFlags = 4;
    int reason = 0;
    int mode = IsPointValid(cached->foundPos, searchOptions.searchFlags & ~0x400, reason);
    if (mode)
        mode = LineOfSight(moveTracker->camPos, cached->foundPos) ? 16 : 2;
    cached->bestLerpMode = mode;
    if (mode < searchOptions.minLerpMode)
        cached->totalScore = 0.0f;
    else
    {
        zVEC3 delta = camai->GetCamSysFirstPos() - cached->foundPos;
        float distance = delta.LengthApprox();
        cached->scoreDistToIdealPos = 1.0f - distance /
            ((camai->GetMaxRange() + camai->GetBestRange()) * 100.0f);
        if (cached->scoreDistToIdealPos < 0.0f) cached->scoreDistToIdealPos = 0.0f;
        if (cached->scoreDistToIdealPos > 1.0f) cached->scoreDistToIdealPos = 1.0f;
        int bits = 0;
        for (int value = mode << 1; value; value <<= 1)
            ++bits;
        cached->scoreLerpMode = bits * 0.015625f;
        cached->totalScore = (cached->scoreLerpMode * 0.2f +
            cached->scoreDistToIdealPos) * 0.5f;
        if (cached->totalScore < 0.00011f) cached->totalScore = 0.00011f;
        if (cached->totalScore > 1.0f) cached->totalScore = 1.0f;
    }
    ignoreSearchFlags = 0;
    state.totalScore = cached->totalScore;
    state.bestLerpMode = cached->bestLerpMode;
}

int zCPathSearch::GetBestLerpMode(zSPathSearchState& state)
{
    int reason = 0;
    if (!IsPointValid(state.foundPos, searchOptions.searchFlags, reason))
        return reason;
    if (world && collisionEnabled)
    {
        zVEC3 direction = state.foundPos - camVob->GetPositionWorld();
        if (world->TraceRayFirstHit(camVob->GetPositionWorld(), direction, target, 0))
            return 2;
    }
    return 16;
}

int zCPathSearch::GetBestLerpMode(const zVEC3& start, const zVEC3& end)
{
    int reason = 0;
    if (!IsPointValid(end, searchOptions.searchFlags, reason))
        return reason;
    if (world && collisionEnabled && world->TraceRayFirstHit(start, end - start, target, 0))
        return 2;
    return 16;
}

zVEC3 zCPathSearch::DoSlerpRotQuat(const zCQuat& first, const zCQuat& second,
    const float& time, const float& range)
{
    if (time < 0.0f || time > 1.0f)
        zerr->Report(zERR_TYPE_WARN, 0,
            "C: zCMovementTracker::DoSlerpRotQuat():rotTime not in range [0..1]", 0, 0,
            0x32e, "P:\\dev\\g2addon\\release\\ZenGin\\_carsten\\zAiCamera_Core.cpp", 0);
    zCQuat rotation = first;
    rotation.Slerp(time, second);
    zMAT4 matrix = {};
    for (int i = 0; i < 4; ++i)
        matrix.m[i][i] = 1.0f;
    rotation.QuatToMatrix4(matrix);
    matrix = moveTracker->trafoTStoWS * matrix;
    return moveTracker->playerPos - matrix.GetAtVector() * range;
}

void zCQuat::Slerp(float time, const zCQuat& other)
{
    for (int i = 0; i < 4; ++i)
        q[i] = q[i] * (1.0f - time) + other.q[i] * time;
    float length = sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (length > 0.0f)
        for (int i = 0; i < 4; ++i)
            q[i] /= length;
}

int zCPathSearch::FindBestEvasion(zSPathSearchOptions& options)
{
    searchOptions = options;
    searchOptions.startAzi = moveTracker->GetAzimuth();
    searchOptions.startElev = moveTracker->GetElevation();
    searchOptions.startRange = moveTracker->GetRange();
    searchOptions.bestScore = 0.000001f;
    searchOptions.searchFlags |= 2;
    evasionSearchReport = zSPathSearchResult();
    searchCache.DeleteList();
    zCArray<zSEvasionSearchDesc> sequence;
    if (!FindEvasionSequence(sequence))
        return 0;

    for (int i = 0; i < sequence.numInArray; ++i)
    {
        if (!sequence[i].searchFunc || !sequence[i].searchFunc(sequence[i]))
            continue;
        if (bestSearchState.totalScore > searchOptions.bestScore)
        {
            zSPathSearchState_Internal* best = NULL;
            searchCache.GetBestScoredSearch(bestSearchState.bestAzi, bestSearchState.bestElev,
                bestSearchState.bestRange, best);
            if (best)
            {
                bestSearchState.bestLerpMode = best->bestLerpMode;
                bestSearchState.trafoCamToWorld = best->trafoCamToWorld;
            }
        }

        zSPathSearchResult* report = zCAICamera::GetCurrent()->evasionSearchReport;
        report->foundEvasion = 1;
        report->p1 = moveTracker->camPos;
        report->p2 = bestSearchState.bestLerpMode == 3 ?
            bestSearchState.trafoCamToWorld.GetTranslation() :
            moveTracker->CalcAziElevRangeApprox(bestSearchState.bestAzi,
            bestSearchState.bestElev, bestSearchState.bestRange);
        report->bestLerpMode = bestSearchState.bestLerpMode;
        report->veloTrans = camai->veloTrans;
        report->veloRot = camai->veloRot;
        report->r1.Matrix4ToQuat(camVob->trafoObjToWorld);
        report->r2.Matrix4ToQuat(bestSearchState.trafoCamToWorld);

        int reuseLast = lastSearchState.totalScore > bestSearchState.totalScore;
        if (!reuseLast && lastSearchState.totalScore > 0.000001f &&
            fabs(lastSearchState.totalScore - bestSearchState.totalScore) < 0.08f)
        {
            zVEC3 lastPos = moveTracker->CalcAziElevRangeApprox(lastSearchState.bestAzi,
                lastSearchState.bestElev, lastSearchState.bestRange);
            int reason = 0;
            reuseLast = IsPointValid(lastPos, searchOptions.searchFlags, reason);
        }
        if (reuseLast)
        {
            report->p2 = moveTracker->CalcAziElevRangeApprox(lastSearchState.bestAzi,
                lastSearchState.bestElev, lastSearchState.bestRange);
            report->bestLerpMode = lastSearchState.bestLerpMode;
            report->r2.Matrix4ToQuat(lastSearchState.trafoCamToWorld);
        }
        else
            lastSearchState = bestSearchState;
        return 1;
    }
    return 0;
}

int zCPathSearch::CanEvadeTo(zSPathSearchState& state)
{
    return GetBestLerpMode(state) == 16;
}

static int ShoulderEvasionSearch(const zSEvasionSearchDesc& desc)
{
    return zCPathSearch::GetSearch()->ShoulderSearch(desc);
}

static int PanicEvasionSearch(const zSEvasionSearchDesc& desc);

int zCPathSearch::FindEvasionSequence(zCArray<zSEvasionSearchDesc>& sequence)
{
    if (searchOptions.poseFailReason == 0x200)
    {
        zSEvasionSearchDesc firstPerson;
        firstPerson.searchFunc = PanicEvasionSearch;
        firstPerson.searchName = "__FP__";
        sequence.InsertEnd(firstPerson);
        return 1;
    }

    int movedBackward = moveTracker->movement & 4;
    if (!movedBackward)
        FES_IdealCone(sequence);
    if (searchOptions.poseFailReason == 4)
        FES_PlayerTargetInvisible(sequence);
    else if (moveTracker->movement & 0x800)
        FES_PlayerInvisible_Moved(sequence);
    else if (moveTracker->movement & 0x2000)
        FES_PlayerInvisible_Rotated(sequence);
    else
        FES_PlayerInvisible_Standing(sequence);
    if (movedBackward)
        FES_IdealCone(sequence);

    zSEvasionSearchDesc shoulder;
    shoulder.searchFunc = ShoulderEvasionSearch;
    shoulder.searchName = "Shoulder Cam";
    sequence.InsertEnd(shoulder);
    zSEvasionSearchDesc panic;
    panic.searchFunc = PanicEvasionSearch;
    panic.searchName = "Panic Cam";
    sequence.InsertEnd(panic);
    return sequence.numInArray > 0;
}

static int SphereEvasionSearch(const zSEvasionSearchDesc& desc)
{
    return zCPathSearch::GetSearch()->SphereSearch(desc);
}

static float ClampSearchValue(float value, float minimum, float maximum)
{
    if (value < minimum)
        return minimum;
    if (value > maximum)
        return maximum;
    return value;
}

static void AppendEvasionSearch(zCArray<zSEvasionSearchDesc>& sequence, const zSTRING& searchName,
    const zSTRING& order, const zSTRING& segment, const zVEC3& center, const zVEC3& spread)
{
    static const char* suffixes[4] = { " A(+)E(+)R(-)", " A(+)E(-)R(-)",
        " A(-)E(+)R(-)", " A(-)E(-)R(-)" };
    const float aziSigns[4] = { 1.0f, 1.0f, -1.0f, -1.0f };
    const float elevSigns[4] = { 1.0f, -1.0f, 1.0f, -1.0f };
    zSEvasionSearchDesc desc;
    desc.searchFunc = SphereEvasionSearch;
    desc.searchName = searchName;
    for (int i = 0; i < 4; ++i)
    {
        zSEvasionSearchDesc::zSEvasionSearchOptions option;
        option.scSign = zVEC3(aziSigns[i], elevSigns[i], -1.0f);
        option.scQuality = zVEC3(0.5f, 0.2f, 0.5f);
        option.scStart = center;
        option.scEnd = center + zVEC3(spread.n[0] * aziSigns[i],
            spread.n[1] * elevSigns[i], -spread.n[2]);
        option.scEnd.n[1] = ClampSearchValue(option.scEnd.n[1], -89.9f, 89.9f);
        option.scEnd.n[2] = ClampSearchValue(option.scEnd.n[2], 0.0f, center.n[2]);
        option.scSequence = order;
        option.scName = segment;
        option.scName += suffixes[i];
        desc.sphereSearchList.InsertEnd(option);
    }
    sequence.InsertEnd(desc);
}

int zCPathSearch::FES_PlayerInvisible_Moved(zCArray<zSEvasionSearchDesc>& sequence)
{
    if (world && collisionEnabled)
    {
        if (world->TraceRayFirstHit(moveTracker->playerHipLeft,
            moveTracker->camPos - moveTracker->playerHipLeft, target, 0) ||
            world->TraceRayFirstHit(moveTracker->playerHipRight,
            moveTracker->camPos - moveTracker->playerHipRight, target, 0) ||
            world->TraceRayFirstHit(moveTracker->playerHead,
            moveTracker->camPos - moveTracker->playerHead, target, 0))
        {
            zVEC3 center(camai->bestRotY, camai->bestRotX, camai->GetBestRange() * 100.0f);
            AppendEvasionSearch(sequence, "Player Invisible, moved diagonally", "ARE", "PIMLRUD",
                center, zVEC3(100.0f, 89.9f, center.n[2] - camai->GetMinRange() * 100.0f));
            return 1;
        }
    }

    if (moveTracker->movement & 4)
        return FES_PlayerInvisible_Moved_BW(sequence);
    if (moveTracker->movement & 0x2000)
        return FES_PlayerInvisible_Rotated(sequence);
    return FES_PlayerInvisible_Moved_FW(sequence);
}

int zCPathSearch::FES_PlayerInvisible_Moved_BW(zCArray<zSEvasionSearchDesc>& sequence)
{
    D_Print("FES Player Invisible (moved backward)", 2);
    const float bestRange = camai->GetBestRange() * 100.0f;
    const float minRange = camai->GetMinRange() * 100.0f;
    zVEC3 center(camai->bestRotY, camai->bestRotX, bestRange);
    AppendEvasionSearch(sequence, "Player Invisible, moved backward", "ARE", "PIMB", center,
        zVEC3(100.0f, 30.0f, bestRange - minRange));
    return 1;
}

int zCPathSearch::FES_PlayerInvisible_Moved_FW(zCArray<zSEvasionSearchDesc>& sequence)
{
    D_Print("FES Player Invisible (moved forward)", 2);
    const float bestRange = camai->GetBestRange() * 100.0f;
    const float minRange = camai->GetMinRange() * 100.0f;
    zVEC3 center(moveTracker->GetAzimuth(), moveTracker->GetElevation(), bestRange);
    AppendEvasionSearch(sequence, "Player Invisible, moved forward", "REA", "PIMF", center,
        zVEC3(20.0f, 40.0f, bestRange - minRange));
    return 1;
}

int zCPathSearch::FES_IdealCone(zCArray<zSEvasionSearchDesc>& sequence)
{
    D_Print("FES Ideal Cone", 2);
    const float bestRange = camai->GetBestRange() * 100.0f;
    const float minRange = camai->GetMinRange() * 100.0f;
    zSEvasionSearchDesc desc;
    desc.searchFunc = SphereEvasionSearch;
    desc.searchName = "Ideal Cone";
    for (int rangeIndex = 0; rangeIndex < 2; ++rangeIndex)
        for (int aziIndex = 0; aziIndex < 2; ++aziIndex)
            for (int elevIndex = 0; elevIndex < 2; ++elevIndex)
                for (int repeat = 0; repeat < 2; ++repeat)
                {
                    float aziSign = aziIndex ? -1.0f : 1.0f;
                    float elevSign = elevIndex ? -1.0f : 1.0f;
                    float rangeSign = rangeIndex ? 1.0f : -1.0f;
                    zSEvasionSearchDesc::zSEvasionSearchOptions option;
                    option.scSign = zVEC3(aziSign, elevSign, rangeSign);
                    option.scQuality = zVEC3(0.3f, 0.3f, 0.3f);
                    option.scStart = zVEC3(camai->bestRotY, camai->bestRotX,
                        rangeSign < 0.0f ? bestRange : 0.01f);
                    option.scEnd = option.scStart + zVEC3(aziSign * 10.0f,
                        elevSign > 0.0f ? 30.0f : -20.0f,
                        rangeSign < 0.0f ? minRange - bestRange : bestRange + 10.0f);
                    option.scEnd.n[0] = ClampSearchValue(option.scEnd.n[0], -180.0f, 180.0f);
                    option.scEnd.n[1] = ClampSearchValue(option.scEnd.n[1], -89.0f, 89.0f);
                    option.scSequence = "RAE";
                    option.scName = "IC A(";
                    option.scName += aziSign > 0.0f ? "+" : "-";
                    option.scName += ")E(";
                    option.scName += elevSign > 0.0f ? "+" : "-";
                    option.scName += ")R(";
                    option.scName += rangeSign > 0.0f ? "+" : "-";
                    option.scName += ")";
                    desc.sphereSearchList.InsertEnd(option);
                }
    sequence.InsertEnd(desc);
    return 1;
}

int zCPathSearch::FES_PlayerInvisible_Standing(zCArray<zSEvasionSearchDesc>& sequence)
{
    searchOptions.searchFlags |= 0x400;
    searchOptions.searchFlags |= 8;
    return FES_PlayerInvisible_Rotated(sequence);
}

int zCPathSearch::FES_PlayerInvisible_Rotated(zCArray<zSEvasionSearchDesc>& sequence)
{
    D_Print("FES Player Invisible (rotated)", 2);
    const float bestRange = camai->GetBestRange() * 100.0f;
    const float minRange = camai->GetMinRange() * 100.0f;
    zVEC3 center(camai->bestRotY, camai->bestRotX, bestRange);
    AppendEvasionSearch(sequence, "Player Invisible, rotated", "REA", "PIR", center,
        zVEC3(30.0f, 40.0f, bestRange - minRange));
    return 1;
}

int zCPathSearch::FES_PlayerTargetInvisible(zCArray<zSEvasionSearchDesc>& sequence)
{
    searchOptions.searchFlags |= 8;
    return FES_PlayerInvisible_Rotated(sequence);
}

int zCPathSearch::FES_CamHitWall(zCArray<zSEvasionSearchDesc>&)
{
    return 1;
}

static int PanicEvasionSearch(const zSEvasionSearchDesc& desc)
{
    return zCPathSearch::GetSearch()->PanicSearch(desc);
}

int zCPathSearch::FES_NoEvasionFound(zCArray<zSEvasionSearchDesc>& sequence)
{
    zSEvasionSearchDesc desc;
    desc.searchFunc = PanicEvasionSearch;
    desc.searchName = "__FP__";
    sequence.InsertEnd(desc);
    return 1;
}

int zCPathSearch::EvasionPossible()
{
    return 1;
}

int zCPathSearch::IsPointValid(const zVEC3& point, int flags, int& reason)
{
    if (!collisionEnabled)
        return 1;

    zVEC3 delta = point - moveTracker->playerPos;
    if (flags & 0x100)
    {
        const float range = sqrt(delta.Length2());
        if (range < camai->minRange * 100.0f || range > camai->maxRange * 100.0f)
        {
            reason = 0x10000;
            return 0;
        }
        const float azi = atan2(delta.n[0], delta.n[2]) * 57.2957795f;
        if (azi < camai->minRotY || azi > camai->maxRotY)
        {
            reason = 0x4000;
            return 0;
        }
        const float elev = atan2(delta.n[1], sqrt(delta.n[0] * delta.n[0] +
            delta.n[2] * delta.n[2])) * 57.2957795f;
        if (elev < camai->minRotX || elev > camai->maxRotX)
        {
            reason = 0x8000;
            return 0;
        }
    }

    if (!(ignoreSearchFlags & 4) && (flags & 4))
    {
        int visible = LineOfSight(moveTracker->playerPos, point);
        if (flags & 0x400)
        {
            visible = LineOfSight(moveTracker->playerHead, point) &&
                LineOfSight(moveTracker->playerHipLeft, point) &&
                LineOfSight(moveTracker->playerHipRight, point);
        }
        if (!visible)
        {
            reason = 1;
            return 0;
        }
    }

    if (!(ignoreSearchFlags & 8) && (flags & 8))
    {
        if (!LineOfSight(moveTracker->playerPos, point))
        {
            reason = 1;
            return 0;
        }
        if (point != moveTracker->playerPos && zCCamera::activeCam)
        {
            float screenX;
            float screenY;
            zCCamera::activeCam->Project(&moveTracker->POI_Player, screenX, screenY);
            if (screenX < zCCamera::activeCam->vpData.x0 ||
                screenX > zCCamera::activeCam->vpData.x1 ||
                screenY < zCCamera::activeCam->vpData.y0 ||
                screenY > zCCamera::activeCam->vpData.y1)
            {
                reason = 4;
                return 0;
            }
        }
    }

    if ((flags & 2) && !LineOfSight(moveTracker->camPos, point))
    {
        reason = 2;
        return 0;
    }

    if (!(ignoreSearchFlags & 0x10) && (flags & 0x10) && IsPointNearWall(point))
    {
        reason = 8;
        return 0;
    }

    reason = 0;
    return 1;
}

int zCPathSearch::IsPointNearWall(const zVEC3& point)
{
    if (!collisionEnabled || !world->compiled)
        return 0;
    float radius = CAMSPHERE_DIAMETER * 0.5f;
    zTBSphere3D sphere;
    sphere.center = point;
    sphere.radius = radius * 1.3f;
    zTBBox3D box;
    box.mins = point - zVEC3(radius, radius, radius);
    box.maxs = point + zVEC3(radius, radius, radius);
    zCPolygon** polygons = NULL;
    int count = 0;
    if (!world->bspTree.bspRoot->CollectPolysInBBox3D(box, polygons, count))
        return 0;
    for (int i = 0; i < count; ++i)
    {
        zCPolygon* polygon = polygons[i];
        zCMaterial* material = polygon->material;
        if (material->rndAlphaBlendFunc != 1)
            continue;
        if (material->texture && material->texture->hasAlpha)
            continue;
        float distance = polygon->polyPlane.normal * point - polygon->polyPlane.distance;
        if (distance > 0.001f)
            continue;
        if (polygon->CheckBSpherePolyIntersection(sphere))
            return 1;
    }
    return 0;
}

int zCPathSearch::LineOfSight(const zVEC3& start, const zVEC3& end)
{
    if (collisionEnabled)
    {
        ++camai->raysCasted;
        if (world->TraceRayFirstHit(start, end - start, target, 0))
            return 0;
    }
    return 1;
}

int zCPathSearch::CorrectPosForNearClip(zVEC3& point)
{
    if (!zCCamera::activeCam || !zCCamera::activeCam->connectedVob)
        return 0;
    zCCamera defaultCamera;
    float defaultNearClip = defaultCamera.nearClipZ;
    if (defaultNearClip <= 1.0f)
        return 0;
    float radius = defaultNearClip + 1.0f;
    zTBSphere3D sphere;
    sphere.center = point;
    sphere.radius = radius * radius * 1.4f;
    zTBBox3D box;
    box.mins = point - zVEC3(radius, radius, radius);
    box.maxs = point + zVEC3(radius, radius, radius);
    zCPolygon** polygons = NULL;
    int count = 0;
    world->bspTree.bspRoot->CollectPolysInBBox3D(box, polygons, count);
    float bestDistance = 3.402823466e38f;
    float minimumDistance = radius * radius;
    zCPolygon* bestPolygon = NULL;
    for (int i = 0; i < count; ++i)
    {
        zCPolygon* polygon = polygons[i];
        zCMaterial* material = polygon->material;
        if (material->rndAlphaBlendFunc != 1 ||
            (material->texture && material->texture->hasAlpha) || (material->smooth & 0x10))
            continue;
        if (!polygon->CheckBSpherePolyIntersection(sphere))
            continue;
        zVEC3 nearest = GetPolyNearestPoint(polygon, point);
        float distance = (nearest - point).Length2();
        if (distance < bestDistance && distance < minimumDistance)
        {
            bestDistance = distance;
            bestPolygon = polygon;
        }
    }
    if (bestPolygon)
    {
        float nearClip = sqrt(bestDistance);
        if (nearClip > 1.0f) nearClip = 1.0f;
        zCCamera::activeCam->nearClipZ = -nearClip;
        zCCamera::activeCam->UpdateProjectionMatrix();
        return 1;
    }
    zCCamera::activeCam->nearClipZ = defaultNearClip < 0.0f ? defaultNearClip : 0.0f;
    zCCamera::activeCam->UpdateProjectionMatrix();
    return 0;
}

int zCPathSearch::AdjustCenterSphere(zVEC3& center, const zVEC3& player, float radius)
{
    if (!collisionEnabled)
        return 0;
    zVEC3 direction = center - player;
    if (direction.Length2() == 0.0f)
        return 0;
    direction.Normalize();
    zVEC3 end = center + direction * radius;
    ++camai->raysCasted;
    if (!world->TraceRayNearestHit(player, end - player, target, 0))
        return 0;

    zVEC3 intersection = world->traceRayReport.foundIntersection;
    zVEC3 offset = intersection - player;
    if (offset.Length2() < radius * radius)
        center = player;
    else
        center = intersection - direction * radius;
    return 1;
}

zVEC3& zCPathSearch::GetPolyNearestPoint(zCPolygon* polygon, const zVEC3& point) const
{
    static zVEC3 nearest;
    nearest = point;
    if (!polygon->polyNumVert)
        return nearest;

    zVEC3 intersection;
    float distance = 0.0f;
    zVEC3 direction = polygon->polyPlane.normal * -2000000.0f;
    if (polygon->CheckRayPolyIntersection(point, direction, intersection, distance))
    {
        nearest = intersection;
        return nearest;
    }

    float bestDistance = 3.402823466e38f;
    for (int i = 0; i < polygon->polyNumVert; ++i)
    {
        const zVEC3& start = polygon->vertex[i]->position;
        const zVEC3& end = polygon->vertex[(i + 1) % polygon->polyNumVert]->position;
        zVEC3 edge = end - start;
        float edgeLength = edge.Length2();
        if (edgeLength < 0.000000001f)
            edgeLength = 0.000000001f;
        float time = ((point - start) * edge) / edgeLength;
        time = ClampSearchValue(time, 0.0f, 1.0f);
        zVEC3 candidate = start + edge * time;
        float candidateDistance = (candidate - point).Length2();
        if (candidateDistance < bestDistance)
        {
            bestDistance = candidateDistance;
            nearest = candidate;
        }
    }
    return nearest;
}

static zCArray<zCCSCamera*> dialogCamList;

void zCAICamera::StartupDialogCam()
{
    zerr->Message("C: reading dialog presets ...");
    zoptions->ChangeDir(15);
    zCArchiver* archiver = zarcFactory->CreateArchiverRead(zSTRING("DIALOGCAMS.ZEN"), 0);
    if (!archiver)
    {
        zerr->Report(zERR_TYPE_FATAL, 0, "C: Dialogcam Preset File is not present!", -1, 0,
            0x1a, "P:\\dev\\g2addon\\release\\ZenGin\\_carsten\\zAiCamera_Dialog.cpp", 0);
        return;
    }

    while (!archiver->EndOfArchive())
    {
        zCObject* object = archiver->ReadObject(NULL);
        if (object->GetObjectName() != "CAMERASTART")
            zCAICamera::dialogCamList.InsertEnd((zCCSCamera*)object);
        else if (object)
            object->Release();
    }
    archiver->Close();
    archiver->Release();

    if (zoptions->Parm("CONVERTDIALOGCAMS"))
    {
        zoptions->ChangeDir(15);
        zCArchiver* output = zarcFactory->CreateArchiverWrite(zSTRING("DIALOGCAMS.ZEN"),
            zARC_MODE_BINARY, 0, 0);
        if (output)
        {
            for (int i = 0; i < zCAICamera::dialogCamList.numInArray; ++i)
                output->WriteObject(zCAICamera::dialogCamList[i]);
            output->Close();
            output->Release();
        }
    }

    for (int i = zCAICamera::dialogCamList.numInArray - 1; i >= 0; --i)
    {
        zCCSCamera* preset = zCAICamera::dialogCamList[i];
        int convert = preset->GetObjectName().Search("CONVERTDIALOGCAMS", 1) >= 0;
        for (int j = preset->posKeys.numInArray - 1; j >= 0; --j)
        {
            zCCamTrj_KeyFrame* key = preset->posKeys[j];
            zVEC3 position = key->actPose.GetTranslation();
            position.n[1] += convert ? 25.0f : 40.0f;
            if (convert)
                key->actPose.SetTranslation(position);
            else
                key->originalPose.SetTranslation(position);
        }
    }
}

void zCAICamera::GlobalDialogCamCleanUp()
{
    for (int i = dialogCamList.numInArray - 1; i >= 0; --i)
    {
        zCCSCamera* preset = dialogCamList[i];
        while (preset->posKeys.numInArray)
            preset->RemoveCamKey(preset->posKeys[0]);
        while (preset->targetKeys.numInArray)
            preset->RemoveTargetKey(preset->targetKeys[0]);
        preset->Release();
        dialogCamList[i] = NULL;
    }
    dialogCamList.DeleteList();
}

void zCAICamera::InitDialogCam()
{
    dialogCamDuration = 10.0f;
    lastDialogCamSide = 0;
    numOUsSpoken = 0;
    numDialogCamTakes = 0;
    lastNumDialogCamTakes = 0;
    dialogCam = NULL;
    firstSpeakerWasPC = 0;
}

void zCAICamera::SetDialogCamDuration(float duration)
{
    dialogCamDuration = duration;
}

void zCAICamera::ReleaseLastDialogCam()
{
    if (!dialogCam)
        return;
    if (camVob->homeWorld)
        camVob->homeWorld->RemoveVob(dialogCam);
    while (dialogCam->posKeys.numInArray)
        dialogCam->RemoveCamKey(dialogCam->posKeys[0]);
    while (dialogCam->targetKeys.numInArray)
        dialogCam->RemoveTargetKey(dialogCam->targetKeys[0]);
    dialogCam->Release();
    dialogCam = NULL;
}

zCCSCamera* zCAICamera::PickPreset(zSTRING presetName,
    const zCArray<zCCSCamera*>& presets)
{
    for (int i = 0; i < presets.numInArray; ++i)
    {
        if (presets[i]->GetObjectName() == presetName)
            return presets[i];
    }
    return NULL;
}

static zCVob* speaker;
static zCVob* listener;

void zCAICamera::StartDialogCam(int take)
{
    zCZoneMusic::SetAutochange(0);
    speaker = targetVobList[0];
    listener = targetVobList[1];
    zVEC3 speakerPos = speaker->trafoObjToWorld.GetTranslation();
    zVEC3 listenerPos = listener->trafoObjToWorld.GetTranslation();
    if ((speakerPos - listenerPos).Length2() == 0.0f)
        return;

    ++numDialogCamTakes;
    if (take && speaker->GetCharacterClass() == 1)
    {
        zCVob* first = speaker;
        speaker = listener;
        listener = first;
        firstSpeakerWasPC = 1;
    }
    else if (!take && firstSpeakerWasPC && numDialogCamTakes > 2)
        firstSpeakerWasPC = 0;

    zCCSCamera* preset = PickPreset(lastPresetName, zCAICamera::dialogCamList);
    if (!DialogPresetIsValid(preset))
        preset = NULL;
    static const char* fallbackNames[] = {
        "DIALOGCAM_CR", "DIALOGCAM_CL", "DIALOGCAM_2", "DIALOGCAM_1"
    };
    for (int i = 0; !preset && i < 4; ++i)
    {
        zCCSCamera* candidate = PickPreset(zSTRING(fallbackNames[i]), zCAICamera::dialogCamList);
        if (DialogPresetIsValid(candidate))
            preset = candidate;
    }
    for (int i = 0; !preset && i < zCAICamera::dialogCamList.numInArray; ++i)
    {
        if (DialogPresetIsValid(zCAICamera::dialogCamList[i]))
            preset = zCAICamera::dialogCamList[i];
    }
    if (!preset)
        return;

    ReleaseLastDialogCam();
    dialogCam = (zCCSCamera*)preset->CreateCopy();
    dialogCam->ignoreFORVobRotCam = 1;
    dialogCam->ignoreFORVobRotTarget = 1;
    dialogCam->adaptToSurroundings = 1;
    dialogCam->Refresh();
    camVob->homeWorld->AddVob(dialogCam);
    dialogCam->SetSleepingMode(2);
    dialogCam->SetSleeping(0);
    zCCSCamera_EventMsg setDuration(zCCSCamera_EventMsg::EV_SET_DURATION);
    setDuration.time = dialogCamDuration;
    dialogCam->OnMessage(&setDuration, camVob);
    lastPresetName = dialogCam->GetObjectName();
    lastDialogCamSide = dialogCam->GetObjectName().Search(0, "R", 1) != -1;
}

int zCAICamera::DialogPresetIsValid(zCCSCamera* preset)
{
    if (!preset)
        return 0;
    zVEC3 speakerPos = speaker->trafoObjToWorld.GetTranslation();
    for (int i = 0; i < preset->posKeys.numInArray; ++i)
    {
        zCCamTrj_KeyFrame* key = preset->GetCamKey(i);
        if (!pathSearch->LineOfSight(speakerPos, key->originalPose.GetTranslation()))
            return 0;
    }
    return 1;
}

void zCAICamera::InitHelperVobs()
{
    if (!focusVob)
        focusVob = new zCVob;
    focusVob->SetSleeping(0);
    focusVob->SetVisual(zSTRING("itfo_booze_01.3ds"));
    focusVob->SetPhysicsEnabled(0);
    focusVob->SetCollDetStat(0);
    focusVob->SetCollDetDyn(0);
    focusVob->SetTrafoObjToWorld(moveTracker->trafoTStoWS);
    camVob->homeWorld->AddVob(focusVob);
    for (int i = 0; i < 80; ++i)
    {
        ctrlDot[i] = new zCVob;
        ctrlDot[i]->SetSleeping(0);
        ctrlDot[i]->SetVisual(zSTRING("itfo_apple_01.3ds"));
        ctrlDot[i]->SetPhysicsEnabled(0);
        ctrlDot[i]->SetCollDetStat(0);
        ctrlDot[i]->SetCollDetDyn(0);
        ctrlDot[i]->SetTrafoObjToWorld(moveTracker->trafoTStoWS);
        camVob->homeWorld->AddVob(ctrlDot[i]);
    }
}

void zCAICamera::DeleteHelperVobs()
{
    camVob->homeWorld->RemoveVob(focusVob);
    for (int i = 0; i < 80; ++i)
    {
        camVob->homeWorld->RemoveVob(ctrlDot[i]);
        ctrlDot[i]->Release();
        ctrlDot[i] = NULL;
    }
}

void zCAICamera::ShowDebug()
{
    focusVob->SetTrafoObjToWorld(moveTracker->trafoTStoWS);
    ctrlDot[0]->SetPositionWorld(moveTracker->playerHipLeft);
    ctrlDot[1]->SetPositionWorld(moveTracker->playerHipRight);
    ctrlDot[2]->SetPositionWorld(moveTracker->playerHead);
    ctrlDot[3]->SetPositionWorld(moveTracker->GetLastValidWayPoint(zCMovementTracker::zWAYPOINT_PLAYER));
    ctrlDot[4]->SetPositionWorld(moveTracker->GetLastValidWayPoint(zCMovementTracker::zWAYPOINT_CAM));
    ctrlDot[5]->SetPositionWorld(moveTracker->playerPos);
    ctrlDot[6]->SetPositionWorld(moveTracker->camPos);
    ctrlDot[7]->SetPositionWorld(moveTracker->POI_Player);
    ctrlDot[8]->SetPositionWorld(moveTracker->GetPlayerFuturePos(moveTracker->playerFuturePosDT));
    ctrlDot[9]->SetPositionWorld(moveTracker->GetIdealRangePos());

    int reason = 0;
    zSTRING valid = pathSearch->IsPointValid(moveTracker->camPos, 0x16, reason) ?
        zSTRING("CamPos valid\n") : zSTRING("CamPos not valid\n");
    debugS = zSTRING("PlayerPos: ") + zSTRING(moveTracker->playerPos.n[0], 2) + " " +
        zSTRING(moveTracker->playerPos.n[1], 2) + " " + zSTRING(moveTracker->playerPos.n[2], 2) +
        "\nTarget velo: " + zSTRING(moveTracker->playerVeloRange, 2) +
        "\nRange To Player: " + zSTRING(moveTracker->GetRange(), 2) +
        "\nAzi/Elev: " + zSTRING(moveTracker->actAzi, 2) + " / " +
        zSTRING(moveTracker->actElev, 2) + "\nBest score: " +
        zSTRING(pathSearch->bestSearchState.totalScore, 5) + "\nSearch: " +
        pathSearch->bestSearchState.sphereSearchName + " / " +
        pathSearch->bestSearchState.sphereSearchSegment + "\n" + valid +
        "ModeName: " + curcammode + "\nRays casted: " + zSTRING(raysCasted);
    screen->Print(0, 3000, debugS);
}

void zCAICamera::DrawClipLine(const zVEC3& start, const zVEC3& end, zCOLOR color)
{
    if (!showDebug)
        return;
    zVEC3 first = zCCamera::activeCam->camMatrix * start;
    zVEC3 second = zCCamera::activeCam->camMatrix * end;
    if (first.n[2] < 1.0f && second.n[2] < 1.0f)
        return;
    if (first.n[2] < 1.0f)
        first = first + (second - first) * ((1.0f - first.n[2]) / (second.n[2] - first.n[2]));
    if (second.n[2] < 1.0f)
        second = first + (second - first) * ((1.0f - first.n[2]) / (second.n[2] - first.n[2]));

    float firstZ = 1.0f / first.n[2];
    float secondZ = 1.0f / second.n[2];
    int x1 = int(zCCamera::activeCam->vpData.xcenter + first.n[0] *
        zCCamera::activeCam->viewDistanceX * firstZ);
    int y1 = int(zCCamera::activeCam->vpData.ycenter - first.n[1] *
        zCCamera::activeCam->viewDistanceY * firstZ);
    int x2 = int(zCCamera::activeCam->vpData.xcenter + second.n[0] *
        zCCamera::activeCam->viewDistanceX * secondZ);
    int y2 = int(zCCamera::activeCam->vpData.ycenter - second.n[1] *
        zCCamera::activeCam->viewDistanceY * secondZ);
    if (screen->ClipLine(x1, y1, x2, y2))
        zrenderer->DrawLineZ(float(x1), float(y1), firstZ, float(x2), float(y2), secondZ, color);
}

zTBSphere3D zCAICamera::CalcMinimumBSphere(zCArray<zVEC3>& points)
{
    zTBSphere3D sphere;
    sphere.center = zVEC3(0.0f, 0.0f, 0.0f);
    sphere.radius = 0.0f;
    if (!points.numInArray)
        return sphere;

    zVEC3 mins = points[0];
    zVEC3 maxs = points[0];
    for (int i = 1; i < points.numInArray; ++i)
    {
        for (int axis = 0; axis < 3; ++axis)
        {
            if (points[i].n[axis] < mins.n[axis])
                mins.n[axis] = points[i].n[axis];
            if (points[i].n[axis] > maxs.n[axis])
                maxs.n[axis] = points[i].n[axis];
        }
    }
    zVEC3 diagonal = maxs - mins;
    sphere.center = (maxs + mins) * 0.5f;
    sphere.radius = sqrt(diagonal.Length2()) * 0.5f;
    for (int i = 0; i < points.numInArray; ++i)
    {
        zVEC3 offset = points[i] - sphere.center;
        float distance = sqrt(offset.Length2());
        if (distance > sphere.radius)
        {
            float newRadius = (sphere.radius + distance) * 0.5f;
            sphere.center += offset * ((distance - newRadius) / distance);
            sphere.radius = newRadius;
        }
    }
    return sphere;
}

int zCAICamera::CheckUnderWaterFX()
{
    if (!camVob || !camVob->homeWorld)
        return 0;
    zVEC3 position = camVob->trafoObjToWorld.GetTranslation();
    zCAIPlayer* playerAI = target ? dynamic_cast<zCAIPlayer*>(target->callback_ai) : NULL;
    if (target && playerAI && target->GetCharacterClass() == 1)
    {
        if (playerAI->state == zCAIPlayer::zMV_STATE_DIVE &&
            position.n[1] < target->trafoObjToWorld.GetTranslation().n[1])
            underWater = 1;
        else if (underWater && playerAI->state != zCAIPlayer::zMV_STATE_DIVE &&
            position.n[1] > target->trafoObjToWorld.GetTranslation().n[1])
            underWater = 0;
        camVob->homeWorld->GetActiveSkyControler()->SetUnderwaterFX(underWater);
        return underWater;
    }

    zVEC3 lastValid = moveTracker->GetLastValidWayPoint(zCMovementTracker::zWAYPOINT_CAM);
    if (fabs(lastValid.n[1] - position.n[1]) > 10.0f)
        lastValid.n[1] += lastValid.n[1] < position.n[1] ? -5.0f : 5.0f;
    ++raysCasted;
    int newUnderWater = underWater;
    if (camVob->homeWorld->TraceRayNearestHit(position,
        zVEC3(0.0f, lastValid.n[1] - position.n[1], 0.0f), NULL, 0x221))
    {
        zCPolygon* polygon = camVob->homeWorld->traceRayReport.foundPoly;
        if (polygon && polygon->material->matGroup == 5)
        {
            moveTracker->ResetWayPoints(position, zCMovementTracker::zWAYPOINT_CAM);
            newUnderWater = position.n[1] <
                camVob->homeWorld->traceRayReport.foundIntersection.n[1];
        }
    }
    if (underWater != newUnderWater)
    {
        underWater = newUnderWater;
        camVob->homeWorld->GetActiveSkyControler()->SetUnderwaterFX(underWater);
    }
    return underWater;
}

zVEC3& zCAICamera::CalcAziElevRange(const float& azi, const float& elev, const float& range,
    const zMAT4& trafo)
{
    static zVEC3 result;
    float aziRad = (180.0f - azi) * 0.0174532924f;
    float elevRad = (elev + 360.0f) * 0.0174532924f;
    zVEC3 right = trafo.GetRightVector();
    zVEC3 up = trafo.GetUpVector();
    zVEC3 at = trafo.GetAtVector();
    right.NormalizeSafe();
    up.NormalizeSafe();
    at.NormalizeSafe();
    float horizontal = cos(elevRad) * range;
    result = trafo.GetTranslation() + right * (cos(aziRad) * horizontal) +
        at * (sin(aziRad) * horizontal) + up * (sin(elevRad) * range);
    return result;
}
