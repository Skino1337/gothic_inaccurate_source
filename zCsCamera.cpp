#include "include/zCsCamera.h"
#include "include/zCsMessages.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/zAiCamera.h"
#include <cmath>

static int cameraScriptState = -1;
static int cameraMessageState = -1;
static zSTRING cameraPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING cameraPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING cameraPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING cameraPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING cameraPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING cameraPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING cameraPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING cameraPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING cameraPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING cameraPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static zMAT4 cameraIdentityMatrix;

static zCClassDef zCCSCameraEventMsgClassDef("zCCSCamera_EventMsg", "zCEventMessage",
    zCCSCamera_EventMsg::_CreateNewInstance, 6, 0x3C, 0, "");
zCClassDef* zCCSCamera_EventMsg::classDef = &zCCSCameraEventMsgClassDef;
static zCClassDef zCCSCameraEventMsgActivateClassDef("zCCSCamera_EventMsgActivate", "zCEventMessage",
    zCCSCamera_EventMsgActivate::_CreateNewInstance, 6, 0x48, 0, "");
zCClassDef* zCCSCamera_EventMsgActivate::classDef = &zCCSCameraEventMsgActivateClassDef;

zCCSCamera_EventMsg::zCCSCamera_EventMsg()
    : key(0), time(0.0f), isDeleted(0), kfType(0)
{
}

zCCSCamera_EventMsg::zCCSCamera_EventMsg(zTCSCam_EvSubType eventSubType)
    : key(0), time(0.0f), isDeleted(0), kfType(0)
{
    subType = eventSubType;
}

zCCSCamera_EventMsg::~zCCSCamera_EventMsg()
{
}

zCObject* zCCSCamera_EventMsg::_CreateNewInstance()
{
    zCCSCamera_EventMsg* message = new zCCSCamera_EventMsg;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

void zCCSCamera_EventMsg::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCCSCamera_EventMsg::_GetClassDef() const
{
    return classDef;
}

int zCCSCamera_EventMsg::IsHighPriority()
{
    return 1;
}

int zCCSCamera_EventMsg::IsJob()
{
    return subType == EV_PLAY || subType == EV_GOTO_KEY;
}

void zCCSCamera_EventMsg::Delete()
{
    isDeleted = 1;
}

int zCCSCamera_EventMsg::IsDeleted()
{
    return isDeleted;
}

int zCCSCamera_EventMsg::MD_GetNumOfSubTypes()
{
    return 8;
}

zSTRING zCCSCamera_EventMsg::MD_GetSubTypeString(int eventSubType)
{
    static const char* names[] = {
        "EV_CAM_NOTHING", "EV_CAM_PLAY", "EV_CAM_PAUSE", "EV_CAM_RESUME", "EV_CAM_STOP",
        "EV_CAM_GOTO_KEY", "EV_CAM_SET_DURATION", "EV_CAM_SET_TO_TIME"
    };
    if (eventSubType < 0 || eventSubType >= 8)
        return zSTRING("");
    return zSTRING(names[eventSubType]);
}

zCEventMessage::zTTimeBehavior zCCSCamera_EventMsg::MD_GetTimeBehavior()
{
    return TBZero;
}

float zCCSCamera_EventMsg::MD_GetMinTime()
{
    return 0.0f;
}

zCCSCamera_EventMsgActivate::zCCSCamera_EventMsgActivate()
    : isDeleted(0), referenceVob(NULL)
{
}

zCCSCamera_EventMsgActivate::zCCSCamera_EventMsgActivate(zTCSCam_ActivateSubType eventSubType)
    : isDeleted(0), referenceVob(NULL)
{
    subType = eventSubType;
}

zCCSCamera_EventMsgActivate::~zCCSCamera_EventMsgActivate()
{
}

zCObject* zCCSCamera_EventMsgActivate::_CreateNewInstance()
{
    zCCSCamera_EventMsgActivate* message = new zCCSCamera_EventMsgActivate;
    zCClassDef::ObjectCreated(message, classDef);
    return message;
}

void zCCSCamera_EventMsgActivate::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCCSCamera_EventMsgActivate::_GetClassDef() const
{
    return classDef;
}

void zCCSCamera_EventMsgActivate::Delete()
{
    isDeleted = 1;
}

int zCCSCamera_EventMsgActivate::IsDeleted()
{
    return isDeleted;
}

int zCCSCamera_EventMsgActivate::MD_GetNumOfSubTypes()
{
    return 3;
}

zSTRING zCCSCamera_EventMsgActivate::MD_GetSubTypeString(int eventSubType)
{
    static const char* names[] = { "EV_DEACTIVATE", "EV_SETCAMREFERENCE", "EV_SETTARGETREFERENCE" };
    if (eventSubType < 0 || eventSubType >= 3)
        return zSTRING("");
    return zSTRING(names[eventSubType]);
}

zSTRING zCCSCamera_EventMsgActivate::MD_GetVobRefName()
{
    return referenceName;
}

void zCCSCamera_EventMsgActivate::MD_SetVobRefName(const zSTRING& name)
{
    referenceName = name;
}

void zCCSCamera_EventMsgActivate::MD_SetVobParam(zCVob* vob)
{
    referenceVob = vob;
}

zCCamTrj_KeyFrame* zCCSCamera::GetCamKey(int index)
{
    if (posKeys.numInArray <= index)
        return NULL;
    return posKeys[index];
}

zCCamTrj_KeyFrame* zCCSCamera::GetTargetKey(int index)
{
    if (targetKeys.numInArray <= index)
        return NULL;
    return targetKeys[index];
}

static zCClassDef zCCamTrjKeyFrameClassDef("zCCamTrj_KeyFrame", "zCVob",
    zCCamTrj_KeyFrame::_CreateNewInstance, 7, 0x1DC, 0, "");
zCClassDef* zCCamTrj_KeyFrame::classDef = &zCCamTrjKeyFrameClassDef;

zCCamTrj_KeyFrame::zCCamTrj_KeyFrame()
{
    time = -1.0f;
    angleRollDeg = 0.0f;
    camFOVScale = 1.0f;
    motionType = 1;
    motionTypeFOV = 1;
    motionTypeRoll = 1;
    motionTypeTimeScale = 1;
    timeIsFixed = 0;
    tension = 1.0f;
    bias = 1.0f;
    continuity = 1.0f;
    timeScale = 1.0f;
    originalPose = zMAT4::s_identity;
    actPose = zMAT4::s_identity;
    adjusted = 0;
    cscam = NULL;
    type = KF_UNDEF;
}

zCCamTrj_KeyFrame::~zCCamTrj_KeyFrame()
{
    if (cscam)
    {
        cscam->Release();
        cscam = NULL;
    }
}

zCClassDef* zCCamTrj_KeyFrame::_GetClassDef() const
{
    return classDef;
}

void zCCamTrj_KeyFrame::ThisVobAddedToWorld(zCWorld* world)
{
    zCVob::ThisVobAddedToWorld(world);
}

void zCCamTrj_KeyFrame::ThisVobRemovedFromWorld(zCWorld* world)
{
    zCVob::ThisVobRemovedFromWorld(world);
    if (!cscam)
        return;
    cscam->AddRef();
    if (type == KF_CAM)
    {
        cscam->posKeys.RemoveOrder(this);
        Release();
        cscam->keysPreProcessed = 0;
        cscam->posSpl.Terminate();
        cscam->targetSpl.Terminate();
        cscam->PreKFProcessing();
    }
    else if (type == KF_TARGET)
    {
        cscam->targetKeys.RemoveOrder(this);
        Release();
        cscam->PostKFProcessing();
        cscam->PreKFProcessing();
    }
    cscam->Refresh();
    cscam->Release();
}

void zCCamTrj_KeyFrame::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteFloat(time);
    archiver.WriteFloat(angleRollDeg);
    archiver.WriteFloat(camFOVScale);
    archiver.WriteEnum(motionType);
    archiver.WriteEnum(motionTypeFOV);
    archiver.WriteEnum(motionTypeRoll);
    archiver.WriteEnum(motionTypeTimeScale);
    archiver.WriteGroupBegin("KEYTIMES");
    archiver.WriteFloat(tension);
    archiver.WriteFloat(bias);
    archiver.WriteFloat(continuity);
    archiver.WriteFloat(timeScale);
    archiver.WriteBool(timeIsFixed);
    archiver.WriteGroupEnd();
    if (!archiver.InProperties())
        archiver.WriteRaw(&originalPose, sizeof(originalPose));
}

void zCCamTrj_KeyFrame::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    time = archiver.ReadFloat();
    angleRollDeg = archiver.ReadFloat();
    camFOVScale = archiver.ReadFloat();
    motionType = archiver.ReadEnum();
    motionTypeFOV = archiver.ReadEnum();
    motionTypeRoll = archiver.ReadEnum();
    motionTypeTimeScale = archiver.ReadEnum();
    tension = archiver.ReadFloat();
    bias = archiver.ReadFloat();
    continuity = archiver.ReadFloat();
    timeScale = archiver.ReadFloat();
    timeIsFixed = archiver.ReadBool();
    if (!archiver.InProperties())
    {
        archiver.ReadRaw(&originalPose, sizeof(originalPose));
        actPose = originalPose;
    }
}

void zCCamTrj_KeyFrame::Changed()
{
    if (cscam)
    {
        if (cscam->camKeysFOR == 1)
            actPose = zInverseMatD(cscam->camKeysRefVob->trafoObjToWorld,
                zSTRING("zCCamTrj_KeyFrame::Changed")) * trafoObjToWorld;
        else
            actPose = trafoObjToWorld;
    }
    originalPose = actPose;
}

static zCClassDef zCCSCameraClassDef("zCCSCamera", "zCVob",
    zCCSCamera::_CreateNewInstance, 8, 0x14DC, 0, "");
zCClassDef* zCCSCamera::classDef = &zCCSCameraClassDef;

zCVob* zCCSCamera::GetPlayerVob()
{
    return oCNpc::player;
}

zCCSCamera::zCCSCamera()
{
    camKeysFOR = 0;
    targetKeysFOR = 0;
    loopMode = 0;
    splLerpMode = 0;
    ignoreFORVobRotCam = 0;
    ignoreFORVobRotTarget = 0;
    adaptToSurroundings = 1;
    easeToFirstKey = 0;
    easeFromLastKey = 0;
    totalTime = 10.0f;
    autoCamPlayerMovable = 1;
    autoCamUntriggerOnLastKey = 1;
    autoCamUntriggerOnLastKeyDelay = 0.0f;
    camKeysRefVob = NULL;
    targetKeysRefVob = NULL;
    paused = 0;
    hasBeenTriggered = 0;
    hasBeenUntriggered = 0;
    triggerQueued = 0;
    queuedVob = NULL;
    keysPreProcessed = 0;
    started = 0;
    gotoTimeMode = 0;
    initialized = 0;
    easeToFirstKeyNotPossible = 0;
    easeFromLastKeyNotPossible = 0;
    autoCamPlayerMovable_BAK = 1;
    autoAdjustTime = 1;
    avgFPS = 0.0f;
    avgFPSNumFrames = 0;
    actTimeCam = 0.0f;
    csTime = 0.0f;
    unTriggerDelayTime = 0.0f;
    trafoCStoWS = zMAT4::s_identity;
    lerpDirSign = 1.0f;
    timeToReach = 0.0f;
    timeToReachScale = 1.0f;
    camFOVSetBy = 0;
    angleRollDegSetBy = 0;
    globalTimeScaleSetBy = 0;
    d_testCS = 0;
    posSpl.deleteKeysWhenDone = 1;
    targetSpl.deleteKeysWhenDone = 1;
    initialPoseCamRefVob = zMAT4::s_identity;
    initialPoseTargetRefVob = zMAT4::s_identity;
    actTargetFocus = zVEC3(0.0f, 0.0f, 0.0f);
}

zCCSCamera::~zCCSCamera()
{
    keysPreProcessed = 0;
    posSpl.Terminate();
    targetSpl.Terminate();
    if (camKeysRefVob == this)
        camKeysRefVob = NULL;
    if (targetKeysRefVob == this)
        targetKeysRefVob = NULL;
    if (camKeysRefVob)
    {
        camKeysRefVob->Release();
        camKeysRefVob = NULL;
    }
    if (targetKeysRefVob)
    {
        targetKeysRefVob->Release();
        targetKeysRefVob = NULL;
    }
    for (int i = 0; i < evMsgProcessList.numInArray; ++i)
        evMsgProcessList[i]->Release();
    evMsgProcessList.DeleteList();
    posKeys.DeleteList();
    targetKeys.DeleteList();
}

zCClassDef* zCCSCamera::_GetClassDef() const
{
    return classDef;
}

static void InsertCSKey(zCCSCamera* camera, zCCamTrj_KeyFrame* key, int index, int target)
{
    zCVob* reference = target ? camera->targetKeysRefVob : camera->camKeysRefVob;
    int referenceMode = target ? camera->targetKeysFOR : camera->camKeysFOR;
    if (referenceMode == 1)
        key->actPose = zInverseMatD(reference->trafoObjToWorld, zSTRING("InsertCSKey")) *
            key->trafoObjToWorld;
    else
        key->actPose = key->trafoObjToWorld;
    key->originalPose = key->actPose;
    zCArray<zCCamTrj_KeyFrame*>& keys = target ? camera->targetKeys : camera->posKeys;
    keys.InsertEnd(key);
    if (index >= 0 && index < keys.numInArray - 1)
    {
        for (int i = keys.numInArray - 1; i > index; --i)
            keys[i] = keys[i - 1];
        keys[index] = key;
    }
    key->AddRef();
    key->cscam = camera;
    camera->AddRef();
    key->type = target ? zCCamTrj_KeyFrame::KF_TARGET : zCCamTrj_KeyFrame::KF_CAM;
    camera->keysPreProcessed = 0;
    camera->posSpl.Terminate();
    camera->targetSpl.Terminate();
    camera->PreKFProcessing();
}

void zCCSCamera::InsertCamKey(zCCamTrj_KeyFrame* key)
{
    InsertCSKey(this, key, -1, 0);
}

void zCCSCamera::InsertCamKeyAtPos(zCCamTrj_KeyFrame* key, int index)
{
    InsertCSKey(this, key, index, 0);
}

void zCCSCamera::InsertTargetKey(zCCamTrj_KeyFrame* key)
{
    InsertCSKey(this, key, -1, 1);
}

void zCCSCamera::InsertTargetKeyAtPos(zCCamTrj_KeyFrame* key, int index)
{
    InsertCSKey(this, key, index, 1);
}

static void RemoveCSKey(zCCSCamera* camera, zCCamTrj_KeyFrame* key, int target)
{
    zCArray<zCCamTrj_KeyFrame*>& keys = target ? camera->targetKeys : camera->posKeys;
    for (int i = 0; i < keys.numInArray; ++i)
    {
        if (keys[i] == key)
        {
            keys.RemoveOrderIndex(i);
            key->Release();
            camera->keysPreProcessed = 0;
            camera->posSpl.Terminate();
            camera->targetSpl.Terminate();
            camera->PreKFProcessing();
            return;
        }
    }
}

void zCCSCamera::RemoveCamKey(zCCamTrj_KeyFrame* key)
{
    RemoveCSKey(this, key, 0);
}

void zCCSCamera::RemoveTargetKey(zCCamTrj_KeyFrame* key)
{
    RemoveCSKey(this, key, 1);
}

void zCCSCamera::Refresh()
{
    for (int i = 0; i < posKeys.numInArray; ++i)
    {
        if (camKeysFOR == 1 && camKeysRefVob)
            posKeys[i]->actPose = zInverseMatD(camKeysRefVob->trafoObjToWorld,
                zSTRING("zCCSCamera::Refresh")) * posKeys[i]->trafoObjToWorld;
        else
            posKeys[i]->actPose = posKeys[i]->trafoObjToWorld;
    }
    for (int i = 0; i < targetKeys.numInArray; ++i)
    {
        if (targetKeysFOR == 1 && targetKeysRefVob)
            targetKeys[i]->actPose = zInverseMatD(targetKeysRefVob->trafoObjToWorld,
                zSTRING("zCCSCamera::Refresh")) * targetKeys[i]->trafoObjToWorld;
        else
            targetKeys[i]->actPose = targetKeys[i]->trafoObjToWorld;
    }
    Initialize();
    keysPreProcessed = 0;
    posSpl.Terminate();
    targetSpl.Terminate();
    PreKFProcessing();
}

static void DrawCSKeys(const zCArray<zCCamTrj_KeyFrame*>& source, zCOLOR color)
{
    if (source.numInArray < 2)
        return;
    zCArray<zCPositionKey*> keys;
    for (int i = -1; i <= source.numInArray; ++i)
    {
        int index = i < 0 ? 0 : (i >= source.numInArray ? source.numInArray - 1 : i);
        zCCamTrj_KeyFrame* sourceKey = source[index];
        zCPositionKey* key = new zCPositionKey;
        key->t = sourceKey->time;
        key->p = sourceKey->trafoObjToWorld.GetTranslation();
        key->rot.Matrix4ToQuat(sourceKey->trafoObjToWorld);
        key->tension = sourceKey->tension;
        key->continuity = sourceKey->continuity;
        key->bias = sourceKey->bias;
        keys.InsertEnd(key);
    }
    zCKBSpline spline;
    spline.Initialize(keys, 1);
    spline.Draw(color, 0.0f);
    spline.Terminate();
    keys.DeleteList();
}

void zCCSCamera::Draw()
{
    if (started)
        return;
    DrawCSKeys(posKeys, zCOLOR(0xFFFFFFFF));
    DrawCSKeys(targetKeys, zCOLOR(0xFF808080));
}

void zCCSCamera::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteEnum(camKeysFOR);
    archiver.WriteEnum(targetKeysFOR);
    archiver.WriteEnum(loopMode);
    archiver.WriteEnum(splLerpMode);
    archiver.WriteBool(ignoreFORVobRotCam);
    archiver.WriteBool(ignoreFORVobRotTarget);
    archiver.WriteBool(adaptToSurroundings);
    archiver.WriteBool(easeToFirstKey);
    archiver.WriteBool(easeFromLastKey);
    archiver.WriteFloat(totalTime);
    archiver.WriteString(autoCamFocusVobName);
    archiver.WriteBool(autoCamPlayerMovable);
    archiver.WriteBool(autoCamUntriggerOnLastKey);
    archiver.WriteFloat(autoCamUntriggerOnLastKeyDelay);
    if (!archiver.InProperties())
    {
        archiver.WriteInt(posKeys.numInArray);
        archiver.WriteInt(targetKeys.numInArray);
        for (int i = 0; i < posKeys.numInArray; ++i)
            archiver.WriteObject(posKeys[i]);
        for (int i = 0; i < targetKeys.numInArray; ++i)
            archiver.WriteObject(targetKeys[i]);
    }
    if (archiver.InSaveGame())
    {
        archiver.WriteBool(paused);
        archiver.WriteBool(started);
        archiver.WriteBool(gotoTimeMode);
        archiver.WriteFloat(csTime);
    }
}

void zCCSCamera::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    camKeysFOR = archiver.ReadEnum();
    targetKeysFOR = archiver.ReadEnum();
    loopMode = archiver.ReadEnum();
    splLerpMode = archiver.ReadEnum();
    ignoreFORVobRotCam = archiver.ReadBool();
    ignoreFORVobRotTarget = archiver.ReadBool();
    adaptToSurroundings = archiver.ReadBool();
    easeToFirstKey = archiver.ReadBool();
    easeFromLastKey = archiver.ReadBool();
    totalTime = archiver.ReadFloat();
    autoCamFocusVobName = archiver.ReadString();
    autoCamPlayerMovable = archiver.ReadBool();
    autoCamUntriggerOnLastKey = archiver.ReadBool();
    autoCamUntriggerOnLastKeyDelay = archiver.ReadFloat();
    easeFromLastKey = 0;
    if (!archiver.InProperties())
    {
        int numPos = archiver.ReadInt();
        int numTarget = archiver.ReadInt();
        for (int i = 0; i < numPos; ++i)
            InsertCamKey((zCCamTrj_KeyFrame*)archiver.ReadObject(NULL));
        for (int i = 0; i < numTarget; ++i)
            InsertTargetKey((zCCamTrj_KeyFrame*)archiver.ReadObject(NULL));
    }
    if (archiver.InSaveGame())
    {
        paused = archiver.ReadBool();
        started = archiver.ReadBool();
        gotoTimeMode = archiver.ReadBool();
        csTime = archiver.ReadFloat();
    }
}

void zCCSCamera::ThisVobAddedToWorld(zCWorld* world)
{
    zCVob::ThisVobAddedToWorld(world);
}

static zCCSCamera* currentCutscene;

void zCCSCamera::ThisVobRemovedFromWorld(zCWorld* world)
{
    zCVob::ThisVobRemovedFromWorld(world);
    AddRef();
    while (posKeys.numInArray)
        RemoveCamKey(posKeys[0]);
    while (targetKeys.numInArray)
        RemoveTargetKey(targetKeys[0]);
    posKeys.DeleteList();
    targetKeys.DeleteList();
    if (currentCutscene == this)
    {
        playing = 0;
        currentCutscene->Release();
        currentCutscene = NULL;
    }
    Release();
}

void zCCSCamera::OnMessage(zCEventMessage* message, zCVob* source)
{
    zCCSCamera_EventMsg* event = dynamic_cast<zCCSCamera_EventMsg*>(message);
    if (event)
    {
        int handled = 1;
        switch (event->subType)
        {
        case zCCSCamera_EventMsg::EV_PLAY:
            handled = EV_Play(event);
            break;
        case zCCSCamera_EventMsg::EV_PAUSE:
            handled = EV_Pause(event);
            break;
        case zCCSCamera_EventMsg::EV_RESUME:
            if (paused)
                paused = 0;
            break;
        case zCCSCamera_EventMsg::EV_STOP:
            handled = EV_Stop(event);
            break;
        case zCCSCamera_EventMsg::EV_GOTO_KEY:
            handled = EV_GotoKey(event);
            break;
        case zCCSCamera_EventMsg::EV_SET_DURATION:
            totalTime = event->time;
            keysPreProcessed = 0;
            posSpl.Terminate();
            targetSpl.Terminate();
            PreKFProcessing();
            break;
        case zCCSCamera_EventMsg::EV_SET_TO_TIME:
            csTime = event->time;
            break;
        default:
            handled = 0;
            break;
        }
        if (handled)
        {
            message->Delete();
            return;
        }
    }

    zCCSCamera_EventMsgActivate* activate = dynamic_cast<zCCSCamera_EventMsgActivate*>(message);
    if (activate)
    {
        if (activate->subType == zCCSCamera_EventMsgActivate::EV_DEACTIVATE)
        {
            initialized = 0;
            csTime = 0.0f;
            actTimeCam = 0.0f;
            started = 0;
            paused = 0;
            keysPreProcessed = 0;
            camKeysRefVob = NULL;
            targetKeysRefVob = NULL;
            initialPoseCamRefVob = zMAT4::s_identity;
            initialPoseTargetRefVob = zMAT4::s_identity;
        }
        else
        {
            zCVob*& reference = activate->subType == zCCSCamera_EventMsgActivate::EV_SETCAMREFERENCE ?
                camKeysRefVob : targetKeysRefVob;
            if (reference)
                reference->Release();
            reference = activate->referenceVob ? activate->referenceVob : this;
            reference->AddRef();
        }
        message->Delete();
        return;
    }
    zCEvMsgCutscene* cutscene = dynamic_cast<zCEvMsgCutscene*>(message);
    if (cutscene && (cutscene->subType == 1 || cutscene->subType == 2))
    {
        EV_Stop(NULL);
        message->Delete();
        return;
    }
    zCVob::OnMessage(message, source);
}

int zCCSCamera::playing;
int zCCSCamera::draw;
int zCCSCamera::evaluateAvgFPS;

static zMAT4 oldCameraTrafo;
static float oldCameraFOV;
static float oldFactorMotion;
static int oldCameraSaved;

int zCCSCamera::EV_Play(zCCSCamera_EventMsg* event)
{
    if (started)
    {
        if (!evMsgProcessList.Contains(event))
        {
            evMsgProcessList.InsertEnd(event);
            event->AddRef();
        }
        csTime = 0.0f;
        return 0;
    }
    if (currentCutscene && currentCutscene != this)
        currentCutscene->EV_Stop(NULL);
    if (currentCutscene)
        currentCutscene->Release();
    currentCutscene = this;
    AddRef();
    csTime = 0.0f;
    float oldTotalTime = totalTime;
    totalTime = event->time;
    if (totalTime == -1.0f)
        totalTime = 10.0f;
    if (gotoTimeMode)
        DeleteEvMsgOfSubType(5);
    started = 1;
    paused = 0;
    gotoTimeMode = 0;
    Initialize();
    if (oldTotalTime != totalTime)
    {
        keysPreProcessed = 0;
        posSpl.Terminate();
        targetSpl.Terminate();
    }
    PreKFProcessing();
    if (!oldCameraSaved && zCCamera::activeCam)
    {
        oldCameraTrafo = zCCamera::activeCam->connectedVob->trafoObjToWorld;
        oldCameraFOV = zCCamera::activeCam->GetFOV();
        oldFactorMotion = ztimer.factorMotion;
        oldCameraSaved = 1;
    }
    playing = 1;
    if (camKeysRefVob)
        initialPoseCamRefVob = camKeysRefVob->trafoObjToWorld;
    if (targetKeysRefVob)
        initialPoseTargetRefVob = targetKeysRefVob->trafoObjToWorld;
    evMsgProcessList.InsertEnd(event);
    event->AddRef();
    if (evaluateAvgFPS)
    {
        avgFPS = 0.0f;
        avgFPSNumFrames = 0;
    }
    return 1;
}

int zCCSCamera::EV_Resume(zCCSCamera_EventMsg*)
{
    if (!paused)
        return 0;
    paused = 0;
    return 1;
}

int zCCSCamera::EV_Pause(zCCSCamera_EventMsg*)
{
    paused = 1;
    return 1;
}

int zCCSCamera::EV_Stop(zCCSCamera_EventMsg*)
{
    if (currentCutscene == this)
        playing = 0;
    if (!d_testCS && started)
    {
        if (easeFromLastKey && !easeFromLastKeyNotPossible && posKeys.numInArray)
        {
            RemoveCamKey(posKeys[posKeys.numInArray - 1]);
            totalTime -= 1.0f;
        }
        if (easeToFirstKey && !easeToFirstKeyNotPossible && posKeys.numInArray)
        {
            RemoveCamKey(posKeys[0]);
            totalTime -= 1.0f;
        }
        if (oldCameraSaved && zCCamera::activeCam)
        {
            zCCamera::activeCam->connectedVob->SetTrafoObjToWorld(oldCameraTrafo);
            zCCamera::activeCam->SetFOV(oldCameraFOV);
            ztimer.factorMotion = oldFactorMotion;
        }
        oldCameraSaved = 0;
    }
    started = 0;
    paused = 0;
    DeleteEvMsgOfSubType(1);
    DeleteEvMsgOfSubType(5);
    if (zCAICamera::GetCurrent())
        zCAICamera::GetCurrent()->playerIsMovable = autoCamPlayerMovable_BAK;
    if (currentCutscene == this)
    {
        currentCutscene->Release();
        currentCutscene = NULL;
    }
    return 1;
}

int zCCSCamera::EV_GotoKey(zCCSCamera_EventMsg* event)
{
    if (!started)
    {
        started = 1;
        Initialize();
        PreKFProcessing();
        playing = 1;
    }
    zCArray<zCCamTrj_KeyFrame*>& keys = event->kfType == 2 ? posKeys : targetKeys;
    if (!keys.numInArray)
        return 0;
    int index = event->key;
    if (index < 0 || index >= keys.numInArray)
        index = 0;
    csTime = keys[index]->time;
    return 1;
}

int zCCSCamera::EV_SetDuration(zCCSCamera_EventMsg* event)
{
    totalTime = event->time;
    keysPreProcessed = 0;
    posSpl.Terminate();
    targetSpl.Terminate();
    PreKFProcessing();
    return 1;
}

int zCCSCamera::EV_SetTime(zCCSCamera_EventMsg* event)
{
    csTime = event->time;
    return 1;
}

int zCCSCamera::EV_Deactivate(zCCSCamera_EventMsgActivate*)
{
    initialized = 0;
    csTime = 0.0f;
    actTimeCam = 0.0f;
    started = 0;
    paused = 0;
    keysPreProcessed = 0;
    camKeysRefVob = NULL;
    targetKeysRefVob = NULL;
    initialPoseCamRefVob = zMAT4::s_identity;
    initialPoseTargetRefVob = zMAT4::s_identity;
    return 1;
}

static void SetCSReference(zCCSCamera* camera, zCCSCamera_EventMsgActivate* event, int target)
{
    zCVob*& reference = target ? camera->targetKeysRefVob : camera->camKeysRefVob;
    if (reference)
        reference->Release();
    reference = event->referenceVob ? event->referenceVob : camera;
    reference->AddRef();
    if (target)
        camera->initialPoseTargetRefVob = reference->trafoObjToWorld;
    else
        camera->initialPoseCamRefVob = reference->trafoObjToWorld;
}

int zCCSCamera::EV_SetCamReference(zCCSCamera_EventMsgActivate* event)
{
    SetCSReference(this, event, 0);
    return 1;
}

int zCCSCamera::EV_SetTargetReference(zCCSCamera_EventMsgActivate* event)
{
    SetCSReference(this, event, 1);
    return 1;
}

void zCCSCamera::DeleteEvMsgOfSubType(int eventSubType)
{
    for (int i = evMsgProcessList.numInArray - 1; i >= 0; --i)
    {
        if (evMsgProcessList[i]->subType == eventSubType)
        {
            zCEventMessage* message = evMsgProcessList[i];
            evMsgProcessList.RemoveIndex(i);
            message->Release();
        }
    }
}

static zMAT4 EvaluateCSKeys(const zCArray<zCCamTrj_KeyFrame*>& keys, float time)
{
    if (!keys.numInArray)
        return zMAT4::s_identity;
    if (keys.numInArray == 1 || time <= keys[0]->time)
        return keys[0]->actPose;
    for (int i = 0; i < keys.numInArray - 1; ++i)
    {
        zCCamTrj_KeyFrame* first = keys[i];
        zCCamTrj_KeyFrame* second = keys[i + 1];
        if (time <= second->time)
        {
            float span = second->time - first->time;
            float weight = span != 0.0f ? (time - first->time) / span : 0.0f;
            if (weight < 0.0f)
                weight = 0.0f;
            if (weight > 1.0f)
                weight = 1.0f;
            zVEC3 position = first->actPose.GetTranslation() +
                (second->actPose.GetTranslation() - first->actPose.GetTranslation()) * weight;
            zCQuat firstRot;
            zCQuat secondRot;
            firstRot.Matrix4ToQuat(first->actPose);
            secondRot.Matrix4ToQuat(second->actPose);
            firstRot.Slerp(weight, secondRot);
            zMAT4 result;
            firstRot.QuatToMatrix4(result);
            result.SetTranslation(position);
            return result;
        }
    }
    return keys[keys.numInArray - 1]->actPose;
}

void zCCSCamera::OnTick()
{
    if (triggerQueued && zCCamera::activeCam && zCCamera::activeCam->connectedVob)
    {
        triggerQueued = 0;
        OnTrigger(queuedVob, queuedVob);
    }
    if (!started)
    {
        if (draw)
            Draw();
        return;
    }
    if (adaptToSurroundings)
        RecalcTrajectories();
    if (paused)
    {
        if (draw)
            Draw();
        return;
    }
    if (!posKeys.numInArray)
        return;

    csTime += ztimer.frameTimeFloat / ztimer.factorMotion * 0.001f *
        timeToReachScale * lerpDirSign;
    if (gotoTimeMode && ((lerpDirSign > 0.0f && csTime >= timeToReach) ||
        (lerpDirSign < 0.0f && csTime <= timeToReach)))
    {
        csTime = timeToReach;
        DeleteEvMsgOfSubType(5);
    }
    if (csTime >= totalTime)
    {
        if (loopMode == 1)
            csTime = fmod(csTime, totalTime);
        else if (loopMode == 2)
        {
            csTime = totalTime;
            lerpDirSign = -1.0f;
        }
        else if (autoCamUntriggerOnLastKey)
        {
            OnUntrigger(this, this);
            return;
        }
        else
            csTime = totalTime;
    }
    else if (csTime < 0.0f)
    {
        csTime = 0.0f;
        lerpDirSign = 1.0f;
    }

    zMAT4 camPose;
    zMAT4 targetPose;
    if (posKeys.numInArray > 1)
        posSpl.Interpolate0(csTime, camPose, splLerpMode);
    else
        camPose = EvaluateCSKeys(posKeys, csTime);
    if (targetKeys.numInArray > 1)
        targetSpl.Interpolate0(csTime, targetPose, splLerpMode);
    else
        targetPose = EvaluateCSKeys(targetKeys, csTime);
    actTargetFocus = targetPose.GetTranslation();
    SetTrafoObjToWorld(camPose);
    if (playing && zCCamera::activeCam && zCCamera::activeCam->connectedVob)
        zCCamera::activeCam->connectedVob->SetTrafoObjToWorld(camPose);
    for (int i = 0; i < posKeys.numInArray - 1; ++i)
    {
        if (csTime >= posKeys[i]->time && csTime <= posKeys[i + 1]->time)
        {
            float span = posKeys[i + 1]->time - posKeys[i]->time;
            float weight = span != 0.0f ? (csTime - posKeys[i]->time) / span : 0.0f;
            if (camFOVSetBy == 2)
                zCCamera::activeCam->SetFOV(posKeys[i]->camFOVScale +
                    (posKeys[i + 1]->camFOVScale - posKeys[i]->camFOVScale) * weight);
            if (globalTimeScaleSetBy == 2)
                ztimer.factorMotion = posKeys[i]->timeScale +
                    (posKeys[i + 1]->timeScale - posKeys[i]->timeScale) * weight;
            break;
        }
    }
    actTimeCam = csTime;
    if (draw)
        Draw();
}

void zCCSCamera::OnTrigger(zCVob* other, zCVob* source)
{
    if (!zCCamera::activeCam || !zCCamera::activeCam->connectedVob)
    {
        triggerQueued = 1;
        queuedVob = other;
        return;
    }
    if (hasBeenTriggered)
        return;
    zCVob* focus = other;
    if (!autoCamFocusVobName.IsEmpty() && homeWorld)
    {
        focus = homeWorld->SearchVobByName(autoCamFocusVobName);
        if (autoCamFocusVobName == "_PLAYER")
            focus = oCNpc::player;
        if (!focus)
            focus = other;
    }
    if (zCAICamera::GetCurrent())
    {
        autoCamPlayerMovable_BAK = zCAICamera::GetCurrent()->playerIsMovable;
        zCAICamera::GetCurrent()->playerIsMovable = autoCamPlayerMovable;
    }
    if (camKeysFOR == 1)
    {
        zCCSCamera_EventMsgActivate activate(zCCSCamera_EventMsgActivate::EV_SETCAMREFERENCE);
        activate.referenceVob = focus;
        EV_SetCamReference(&activate);
    }
    if (targetKeysFOR == 1)
    {
        zCCSCamera_EventMsgActivate activate(zCCSCamera_EventMsgActivate::EV_SETTARGETREFERENCE);
        activate.referenceVob = focus;
        EV_SetTargetReference(&activate);
    }
    if (!posKeys.numInArray)
    {
        zCCamTrj_KeyFrame* key = new zCCamTrj_KeyFrame;
        key->SetTrafoObjToWorld(trafoObjToWorld);
        InsertCamKey(key);
        key->Release();
    }
    if (!targetKeys.numInArray)
    {
        zCCamTrj_KeyFrame* key = new zCCamTrj_KeyFrame;
        key->SetTrafoObjToWorld(focus ? focus->trafoObjToWorld : trafoObjToWorld);
        InsertTargetKey(key);
        key->Release();
    }
    zCCSCamera_EventMsg* play = new zCCSCamera_EventMsg(zCCSCamera_EventMsg::EV_PLAY);
    play->time = totalTime;
    EV_Play(play);
    play->Release();
    SetSleeping(0);
    hasBeenTriggered = 1;
    hasBeenUntriggered = 0;
}

void zCCSCamera::OnUntrigger(zCVob*, zCVob*)
{
    hasBeenUntriggered = 1;
    EV_Stop(NULL);
    hasBeenTriggered = 0;
}

void zCCSCamera::RecalcTrajectories()
{
    int changed = !CheckAndResetAdjustedKeys();
    if (camKeysFOR == 1)
    {
        int checked = 0;
        for (int i = 0; i < posKeys.numInArray && checked < 3; ++i)
        {
            if (posKeys[i]->time >= csTime)
            {
                ++checked;
                if (!CheckAndCorrectKey(i))
                    changed = 1;
            }
        }
    }
    if (changed)
    {
        keysPreProcessed = 0;
        posSpl.Terminate();
        targetSpl.Terminate();
        PreKFProcessing();
    }
}

int zCCSCamera::CheckAndResetAdjustedKeys()
{
    int unchanged = 1;
    for (int i = 0; i < posKeys.numInArray; ++i)
    {
        if (posKeys[i]->adjusted && CheckAndCorrectKey(i))
        {
            posKeys[i]->adjusted = 0;
            unchanged = 0;
        }
    }
    return unchanged;
}

int zCCSCamera::CheckAndCorrectKey(int index)
{
    zCCamTrj_KeyFrame* key = posKeys[index];
    zMAT4 pose = camKeysRefVob->trafoObjToWorld * key->actPose;
    if (CheckAndCorrectPose(pose))
        return 1;
    key->adjusted = 1;
    key->actPose = zInverseMatD(camKeysRefVob->trafoObjToWorld,
        zSTRING("CheckAndCorrectKey")) * pose;
    return 0;
}

int zCCSCamera::CheckAndCorrectPose(zMAT4& pose)
{
    if (!camKeysRefVob)
        return 1;
    zCPathSearch* search = zCPathSearch::GetSearch();
    zVEC3 center = pose.GetTranslation();
    zVEC3 player = camKeysRefVob->trafoObjToWorld.GetTranslation();
    if (search->IsPointNearWall(center))
    {
        search->AdjustCenterSphere(center, player, 40.0f);
        pose.SetTranslation(center);
        return 0;
    }
    if (homeWorld->TraceRayFirstHit(player, center - player, camKeysRefVob, 0x921))
    {
        zVEC3 direction = center - player;
        direction.NormalizeSafe();
        pose.SetTranslation(homeWorld->traceRayReport.foundIntersection -
            direction * (CAMSPHERE_DIAMETER * 0.51f));
        return 0;
    }
    return 1;
}

void zCCamTrj_KeyFrame::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void zCCSCamera::AutoSetKeyTimes(zCArray<zCCamTrj_KeyFrame*>& keys)
{
    if (!autoAdjustTime || !keys.numInArray)
        return;
    if (keys.numInArray == 1)
    {
        keys[0]->time = 0.0f;
        return;
    }
    int invalid = 0;
    for (int i = 0; i < keys.numInArray; ++i)
    {
        if (keys[i]->time == -1.0f || (i && keys[i]->time < keys[i - 1]->time))
            invalid = 1;
    }
    float lastTime = keys[keys.numInArray - 1]->time;
    if (lastTime > totalTime || invalid || lastTime == 0.0f)
    {
        for (int i = 0; i < keys.numInArray; ++i)
            keys[i]->time = float(i) / float(keys.numInArray - 1) * totalTime;
    }
    else
    {
        float scale = totalTime / lastTime;
        for (int i = 0; i < keys.numInArray; ++i)
            keys[i]->time *= scale;
    }
    for (int i = 1; i < keys.numInArray; ++i)
        if (keys[i]->time <= keys[i - 1]->time)
            keys[i]->time = keys[i - 1]->time + 0.0001f;
}

static void BuildCSSpline(zCKBSpline& spline, const zCArray<zCCamTrj_KeyFrame*>& source)
{
    if (source.numInArray < 2)
        return;
    zCArray<zCPositionKey*> keys;
    for (int i = -1; i <= source.numInArray; ++i)
    {
        int index = i < 0 ? 0 : (i >= source.numInArray ? source.numInArray - 1 : i);
        zCCamTrj_KeyFrame* sourceKey = source[index];
        zCPositionKey* key = new zCPositionKey;
        key->t = sourceKey->time;
        key->p = sourceKey->actPose.GetTranslation();
        key->rot.Matrix4ToQuat(sourceKey->actPose);
        key->tension = sourceKey->tension;
        key->continuity = sourceKey->continuity;
        key->bias = sourceKey->bias;
        keys.InsertEnd(key);
    }
    spline.Initialize(keys, 1);
}

void zCCSCamera::PreKFProcessing()
{
    if (keysPreProcessed)
        return;
    camFOVSetBy = 0;
    angleRollDegSetBy = 0;
    globalTimeScaleSetBy = 0;
    for (int i = 0; i < posKeys.numInArray; ++i)
    {
        if (posKeys[i]->camFOVScale != 1.0f)
            camFOVSetBy = 2;
        if (posKeys[i]->angleRollDeg != 0.0f)
            angleRollDegSetBy = 2;
        if (posKeys[i]->timeScale != 1.0f)
            globalTimeScaleSetBy = 2;
    }
    for (int i = 0; i < targetKeys.numInArray; ++i)
    {
        if (targetKeys[i]->camFOVScale != 1.0f)
            camFOVSetBy = 1;
        if (targetKeys[i]->angleRollDeg != 0.0f)
            angleRollDegSetBy = 1;
        if (targetKeys[i]->timeScale != 1.0f)
            globalTimeScaleSetBy = 1;
    }
    AutoSetKeyTimes(posKeys);
    AutoSetKeyTimes(targetKeys);
    posSpl.Terminate();
    targetSpl.Terminate();
    BuildCSSpline(posSpl, posKeys);
    BuildCSSpline(targetSpl, targetKeys);
    keysPreProcessed = 1;
}

void zCCSCamera::PostKFProcessing()
{
    keysPreProcessed = 0;
    posSpl.Terminate();
    targetSpl.Terminate();
}

void zCCSCamera::Changed()
{
    keysPreProcessed = 0;
    posSpl.Terminate();
    targetSpl.Terminate();
    PreKFProcessing();
}

void zCCSCamera::Initialize()
{
    if (!started || !posKeys.numInArray || !zCCamera::activeCam ||
        !zCCamera::activeCam->connectedVob)
        return;
    if (easeToFirstKey)
    {
        zVEC3 current = zCCamera::activeCam->connectedVob->GetPositionWorld();
        zVEC3 first = posKeys[0]->GetPositionWorld();
        if (homeWorld->TraceRayFirstHit(current, first - current, camKeysRefVob, 0x921))
            easeToFirstKeyNotPossible = 1;
        else
        {
            zCCamTrj_KeyFrame* key = new zCCamTrj_KeyFrame;
            key->SetTrafoObjToWorld(zCCamera::activeCam->connectedVob->trafoObjToWorld);
            key->motionType = 4;
            key->motionTypeFOV = 4;
            key->motionTypeRoll = 4;
            key->motionTypeTimeScale = 4;
            for (int i = 0; i < posKeys.numInArray; ++i)
                posKeys[i]->time += 1.0f;
            totalTime += 1.0f;
            InsertCamKeyAtPos(key, 0);
            key->Release();
            easeToFirstKeyNotPossible = 0;
        }
    }
    if (easeFromLastKey)
    {
        zVEC3 current = zCCamera::activeCam->connectedVob->GetPositionWorld();
        zVEC3 last = posKeys[posKeys.numInArray - 1]->GetPositionWorld();
        if (homeWorld->TraceRayFirstHit(current, last - current, camKeysRefVob, 0x921))
            easeFromLastKeyNotPossible = 1;
        else
        {
            zCCamTrj_KeyFrame* key = new zCCamTrj_KeyFrame;
            key->SetTrafoObjToWorld(zCCamera::activeCam->connectedVob->trafoObjToWorld);
            key->motionType = 4;
            key->motionTypeFOV = 4;
            key->motionTypeRoll = 4;
            key->motionTypeTimeScale = 4;
            key->time = totalTime + 1.0f;
            totalTime += 1.0f;
            InsertCamKey(key);
            key->Release();
            easeFromLastKeyNotPossible = 0;
        }
    }
    initialized = 1;
    if (csTime < 0.0f)
        csTime = 0.0f;
    if (csTime > totalTime)
        csTime = totalTime;
    if (camKeysRefVob)
        initialPoseCamRefVob = camKeysRefVob->trafoObjToWorld;
    if (targetKeysRefVob)
        initialPoseTargetRefVob = targetKeysRefVob->trafoObjToWorld;
    PreKFProcessing();
    trafoCStoWS = posKeys[0]->actPose;
    if (targetKeys.numInArray)
        actTargetFocus = targetKeys[0]->actPose.GetTranslation();
}

void zCCSCamera::Reset()
{
    for (int i = 0; i < posKeys.numInArray; ++i)
    {
        posKeys[i]->actPose = posKeys[i]->originalPose;
        posKeys[i]->motionType = 1;
    }
    csTime = 0.0f;
    keysPreProcessed = 0;
    posSpl.Terminate();
    targetSpl.Terminate();
    PreKFProcessing();
}

void zCCSCamera::CalcPrevNextActKeyOfTime(const float& time,
    zCArray<zCCamTrj_KeyFrame*>& keys, int& previous, int& next, int& current)
{
    current = 0;
    for (int i = 0; i < keys.numInArray; ++i)
    {
        if (keys[i]->time <= time)
            current = i;
    }
    if (current >= keys.numInArray - 1)
        current = keys.numInArray - 2;
    if (current < 0)
        current = 0;
    previous = current;
    while (previous > 0 && (keys[previous]->motionType == 0 || keys[previous]->motionType == 1))
        --previous;
    next = current + 1;
    if (next >= keys.numInArray)
        next = keys.numInArray - 1;
    while (next < keys.numInArray - 1 &&
        (keys[next]->motionType == 0 || keys[next]->motionType == 1))
        ++next;
}

int zCCSCamera::IsPointNearWall(const zVEC3& point)
{
    if (!homeWorld->compiled)
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
    if (!homeWorld->bspTree.bspRoot->CollectPolysInBBox3D(box, polygons, count))
        return 0;
    for (int i = 0; i < count; ++i)
        if (polygons[i]->material->matGroup != 5 &&
            polygons[i]->CheckBSpherePolyIntersection(sphere))
            return 1;
    return 0;
}

zVEC3 zCCSCamera::GetPolyNearestPoint(zCPolygon* polygon, const zVEC3& point) const
{
    return zCPathSearch::GetSearch()->GetPolyNearestPoint(polygon, point);
}

int zCCSCamera::AdjustCenterSphere(zVEC3& center, const zVEC3& player, float radius)
{
    zCPathSearch* search = zCPathSearch::GetSearch();
    search->world = homeWorld;
    search->target = camKeysRefVob;
    return search->AdjustCenterSphere(center, player, radius);
}

void zCCSCamera::SetDrawEnabled(int enabled)
{
    draw = enabled;
}

void zCCSCamera_EventMsg::Archive(zCArchiver& archiver)
{
    zCEventMessage::Archive(archiver);
    if (subType == EV_PLAY || subType == EV_SET_DURATION || subType == EV_SET_TO_TIME)
        archiver.WriteFloat(time);
    else if (subType == EV_GOTO_KEY)
    {
        archiver.WriteFloat(time);
        archiver.WriteInt(key);
        archiver.WriteEnum(kfType);
    }
}

void zCCSCamera_EventMsg::Unarchive(zCArchiver& archiver)
{
    zCEventMessage::Unarchive(archiver);
    if (subType == EV_PLAY || subType == EV_SET_DURATION || subType == EV_SET_TO_TIME)
        time = archiver.ReadFloat();
    else if (subType == EV_GOTO_KEY)
    {
        time = archiver.ReadFloat();
        key = archiver.ReadInt();
        kfType = archiver.ReadEnum();
    }
}

void zCCSCamera_EventMsgActivate::Archive(zCArchiver& archiver)
{
    zCEventMessage::Archive(archiver);
    archiver.WriteString(referenceName);
}

void zCCSCamera_EventMsgActivate::Unarchive(zCArchiver& archiver)
{
    zCEventMessage::Unarchive(archiver);
    referenceName = archiver.ReadString("refVobName");
}

zCObject* zCCSCamera::_CreateNewInstance()
{
    zCCSCamera* camera = new zCCSCamera;
    zCClassDef::ObjectCreated(camera, classDef);
    return camera;
}

void zCCSCamera::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCObject* zCCamTrj_KeyFrame::_CreateNewInstance()
{
    zCCamTrj_KeyFrame* key = new zCCamTrj_KeyFrame;
    zCClassDef::ObjectCreated(key, classDef);
    return key;
}
