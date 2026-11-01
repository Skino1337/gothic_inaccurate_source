#include "include/zVob.h"

static int classDefState = -1;
static zCClassDef zCCSCutsceneContextClassDef("zCCSCutsceneContext", "zCCutscene",
    zCCSCutsceneContext::_CreateNewInstance, 1, 0xAC, 0, "");
zCClassDef* zCCSCutsceneContext::classDef = &zCCSCutsceneContextClassDef;
int zCCSCutsceneContext::playDisabled;
extern zTIMER ztimer;

void zCCutscene::PrepareBlock()
{
}

zCVob* zCCutscene::LoadStage()
{
    return NULL;
}

zCVob* zCCutscene::CatchPlayer(zCVob* vob)
{
    return vob;
}

int zCCutscene::AddMainRole(zCVob* vob)
{
    mainRoleVob = vob;
    if (mainRoleVob)
        mainRoleVob->AddRef();
    return 0;
}

int zCCutscene::CSCheck(int& reaction)
{
    reaction = 0;
    return 1;
}

void zCCutscene::PrintDebugInfo(const zSTRING&)
{
}

void zCCSCutsceneContext::PrintDebugInfo(const zSTRING& prefix)
{
    if (zerr->filter_level >= 1)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Cutscene \"" + GetCSName() + "\"",
            1, 1, 0x397, 0, 0);
    zCCutscene::PrintDebugInfo(prefix);
    if (prefix == "PROPS" || prefix == "STATES") {
        if (lastTimeStamp == -1.0f)
            zerr->Message("B: CS: not playing");
        else if (lastTimeStamp == -2.0f)
            zerr->Message("B: CS: interrupted");
        else if (lastTimeStamp == -3.0f)
            zerr->Message("B: CS: forced stop");
        else if (lastTimeStamp < 0.0f)
            zerr->Message("B: CS: unknown state " + zSTRING(lastTimeStamp, 9) + "...");
        else
            zerr->Message("B: CS: running ...");
        if (prefix == "PROPS")
            properties->PrintDebugInfo();
    }
    if (zerr->filter_level >= 1)
        zerr->Report(zERR_TYPE_INFO, 0, "", 1, 2, 0x3A9, 0, 0);
}

zCEventMessage* zCCSBlockBase::GetCommand()
{
    return NULL;
}

void zCCutscene::Archive(zCArchiver& archiver)
{
    zCObject::Archive(archiver);
}

void zCCutscene::Unarchive(zCArchiver& archiver)
{
    zCObject::Unarchive(archiver);
}

int zCCutscene::GetActualBlockNum() const
{
    return actualBlockNum;
}

int zCCSCutsceneContext::StartPlaying()
{
    if (properties && properties->globalCutscene && ownerPlayer) {
        zCCSCutsceneContext* playing = ownerPlayer->GetPlayingGlobalCutscene();
        if (playing && playing != this) {
            if (zerr->filter_level >= 7)
                zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Global Cutscene delayed: Another is running!",
                    7, 0, 0x199, 0, 0);
            return 0;
        }
    }
    if (!zCCutscene::StartPlaying()) {
        if (zerr->filter_level >= 7)
            zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Start failed (in zCCSCutscene)",
                7, 0, 0x1B0, 0, 0);
        return 0;
    }
    if (zerr->filter_level >= 5)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Start playing ...", 5, 0, 0x1A3, 0, 0);
    if (properties)
        properties->ignore = !properties->csLoop;
    return 1;
}

void zCCSCutsceneContext::Stop()
{
    if (!ownerPlayer) {
        zerr->Report(zERR_TYPE_WARN, 0, "B: CS: Cutscene has no player", 0, 0, 0x1BF, 0, 0);
        return;
    }
    if (zerr->filter_level >= 7)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Stopping cutscene-context ...", 7, 0, 0x1C3, 0, 0);
    if (properties && mainRoleVob && isCutscene && !properties->scriptFuncOnStop.IsEmpty()) {
        const int functionIndex = parser.GetIndex(properties->scriptFuncOnStop);
        if (functionIndex < 0) {
            zerr->Report(zERR_TYPE_WARN, 0, "B: CS: onStop-ScriptFunction not found: "
                + properties->scriptFuncOnStop, 0, 0, 0x1F0, 0, 0);
        } else {
            parser.SetInstance("SELF", mainRoleVob);
            parser.SetInstance("OTHER", NULL);
            parser.SetInstance("VICTIM", NULL);
            parser.CallFunc(functionIndex);
        }
    }
    zCCutscene::Stop();
    stage = NULL;
    wasTriggered = 0;
    lastPlayResult = 1;
    if (poolItem)
        poolItem->ClrFlags(1);
    if (zerr->filter_level >= 7)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Cutscene-Context stopped.", 7, 0, 0x1F7, 0, 0);
}

void zCCSCutsceneContext::Interrupt()
{
    lastTimeStamp = -2.0f;
    lastPlayResult = 2;
    zCCutscene::Interrupt();
    if (properties)
        properties->ignore = 0;
}

void zCCSCutsceneContext::ForcedInterrupt()
{
    lastTimeStamp = -3.0f;
    lastPlayResult = 2;
    zCCutscene::Interrupt();
}

int zCCSCutsceneContext::Play(zVEC3 position)
{
    lastPlayResult = CS_PLAY_CONTINUED;
    if (!properties) {
        if (zerr->filter_level >= 8)
            zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"??\": Cutscene has no properties-object",
                8, 0, 0x22F, 0, 0);
        return 0;
    }
    if (!loaded) {
        if (zerr->filter_level >= 8)
            zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + GetCSName()
                + "\": Cutscene was not correctly loaded", 8, 0, 0x230, 0, 0);
        return 1;
    }
    if (lastTimeStamp == -3.0f)
        return 0;

    int canPlay = allowedToPlay;
    int reaction = 1;
    if (canPlay) {
        if (IsCutscene() && !mainRoleVob) {
            canPlay = 0;
        } else {
            if (IsCutscene() && !properties->ignore) {
                const float distance = (position - mainRoleVob->trafoObjToWorld.GetTranslation()).LengthApprox();
                if (!properties->CheckDistance(distance)) {
                    if (lastPlayResult == CS_PLAY_CONTINUED)
                        lastPlayResult = CS_PLAY_TOOFAR;
                    canPlay = 0;
                } else if (!wasTriggered && properties->hasToBeTriggerd) {
                    canPlay = 0;
                    if (lastPlayResult == CS_PLAY_CONTINUED)
                        lastPlayResult = CS_PLAY_NOTTRIGGERED;
                }
            }
        }
        if (canPlay && IsCutscene() && !CSCheck(reaction)) {
            if (lastPlayResult == CS_PLAY_CONTINUED)
                lastPlayResult = CS_PLAY_CHECKFAILED;
            canPlay = 0;
        }
    }

    int advanceBlock = 0;
    if (!canPlay) {
        if (lastTimeStamp >= 0.0f) {
            if (reaction == 2) {
                Interrupt();
                lastPlayResult = CS_PLAY_INTERRUPTED;
            } else {
                Stop();
                lastPlayResult = CS_PLAY_STOPPED;
            }
        }
    } else if (lastTimeStamp == -1.0f) {
        if (StartPlaying()) {
            if (zerr->filter_level >= 7)
                zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + GetCSName() + "\": Start playing ...",
                    7, 0, 0x266, 0, 0);
            lastPlayResult = CS_PLAY_STARTEDPLAYING;
            if (poolItem)
                poolItem->SetFlags(2);
            advanceBlock = 1;
        } else {
            if (zerr->filter_level >= 7)
                zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + GetCSName()
                    + "\": Start playing ... failed", 7, 0, 0x26C, 0, 0);
            lastPlayResult = CS_PLAY_STARTFAILED;
        }
    } else if (lastTimeStamp == -2.0f) {
        if (ResumeActBlock()) {
            if (zerr->filter_level >= 7)
                zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + GetCSName() + "\": Resume playing ...",
                    7, 0, 0x277, 0, 0);
            lastPlayResult = CS_PLAY_RESUMED;
            advanceBlock = 1;
        } else {
            lastPlayResult = CS_PLAY_RESUMEFAILED;
        }
    } else {
        advanceBlock = 1;
    }

    if (advanceBlock) {
        if (poolItem)
            poolItem->SetFlags(1);
        const float currentTime = ztimer.totalTimeFloat * 0.001f - lastTimeStamp;
        if (lastTimePoint < currentTime && !PlaySub(lastTimePoint, currentTime))
            lastTimePoint = currentTime;
    }
    if (zerr->filter_level >= 9) {
        zSTRING resultText;
        switch (lastPlayResult) {
        case CS_PLAY_STOPPED:
            resultText = "stop";
            break;
        case CS_PLAY_INTERRUPTED:
            resultText = "interruptiion";
            break;
        case CS_PLAY_TOOFAR:
            resultText = "distance-check failed";
            break;
        case CS_PLAY_MAXREACHED:
            resultText = "maximum number of runs in specified period";
            break;
        case CS_PLAY_NOTTRIGGERED:
            resultText = "missing trigger-event";
            break;
        case CS_PLAY_CHECKFAILED:
            resultText = "failed cutscene-check";
            break;
        default:
            resultText = "playing cutscene: " + zSTRING(lastPlayResult);
            break;
        }
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS \"" + GetCSName() + "\": " + resultText,
            zerr->filter_level, 0, 0x2B4, 0, 0);
    }
    return 1;
}

void zCCSCutsceneContext::SetPoolItem(zCCSPoolItem* item)
{
    if (poolItem) {
        poolItem->Release();
        poolItem = NULL;
    }
    poolItem = item;
    if (poolItem) {
        poolItem->AddRef();
        if (!loaded)
            poolItem->deactivated = 1;
    }
}

void zCCSCutsceneContext::PrepareBlock()
{
    zCCutscene::PrepareBlock();
    lastTimePoint = 0.0f;
    lastTimeStamp = ztimer.totalTimeFloat * 0.001f;
    if (!properties->csLoop)
        properties->ignore = 1;
}

zCVob* zCCSCutsceneContext::GetStage()
{
    return stage ? stage : alternativeStageVob;
}

void zCCSCutsceneContext::SetStage(zCVob* vob)
{
    if (vob)
        vob->AddRef();
    if (alternativeStageVob) {
        alternativeStageVob->Release();
        alternativeStageVob = NULL;
    }
    alternativeStageVob = vob;
    stage = LoadStage();
}

zCVob* zCCSCutsceneContext::CatchPlayer(zCVob*)
{
    return zCCutscene::CatchPlayer(alternativeStageVob);
}

void zCCSCutsceneContext::ForcedResume()
{
    if (lastTimeStamp == -3.0f)
        lastTimeStamp = -2.0f;
}

int zCCSCutsceneContext::CheckRoleReactions()
{
    int reaction;
    int result = CSCheck(reaction);
    if (!result && lastTimeStamp >= 0.0f) {
        if (reaction == 2) {
            Interrupt();
            lastPlayResult = 2;
        } else {
            Stop();
            lastPlayResult = 1;
        }
    }
    return result;
}

int zCCSCutsceneContext::AddMainRole(zCVob* vob)
{
    int result = zCCutscene::AddMainRole(vob);
    if (!result && poolItem)
        poolItem->deactivated = 1;
    return result;
}

void zCCSCutsceneContext::HandleTrigger()
{
    wasTriggered = 1;
}

void zCCSCutsceneContext::HandleUntrigger()
{
}

float zCCSCutsceneContext::GetLastTimePoint()
{
    return lastTimePoint;
}

float zCCSCutsceneContext::GetLastTimeStamp()
{
    return lastTimeStamp;
}

zCCSCutsceneContext::zTCSPlayResult zCCSCutsceneContext::GetLastPlayResult(zSTRING& text)
{
    static const char* names[] = {
        "startplay", "stop", "interpt", "resumed", "cont.", "disabled", "startfail",
        "res.fail", "TooFar", "MaxReached", "NoTrigger", "CheckFail"
    };
    if (lastPlayResult >= 0 && lastPlayResult < 12)
        text = names[lastPlayResult];
    else
        text = "unknown";
    return (zTCSPlayResult)lastPlayResult;
}

int zCCSCutsceneContext::AreU(const zSTRING& name)
{
    return properties->GetName() == name;
}

int zCCSCutsceneContext::AreU(int referenceValue)
{
    return reference == referenceValue;
}

int zCCSCutsceneContext::IsForcedInterrupted()
{
    return lastTimeStamp == -3.0f;
}

int zCCSCutsceneContext::IsFinished()
{
    if (!loaded && (!properties || !properties->globalCutscene))
        return 0;
    return zCCutscene::IsFinished();
}

int zCCSCutsceneContext::IsPlaying()
{
    return lastTimeStamp >= 0.0f && !IsFinished();
}

int zCCSCutsceneContext::ReplayCutscene()
{
    return properties->csLoop;
}

zCCSCutsceneContext::zCCSCutsceneContext()
    : lastPlayResult(0), allowedToPlay(1), alternativeStageVob(NULL), wasTriggered(0),
      isCutscene(1), loaded(1), reference(-1), stage(NULL), poolItem(NULL),
      lastTimeStamp(-1.0f), lastTimePoint(0.0f)
{
}

zCCSCutsceneContext::zCCSCutsceneContext(zSTRING& name)
    : lastPlayResult(0), allowedToPlay(1), alternativeStageVob(NULL), wasTriggered(0),
      isCutscene(1), loaded(0), reference(-1), stage(NULL), poolItem(NULL),
      lastTimeStamp(-1.0f), lastTimePoint(0.0f)
{
    if (!name.Length()) {
        zerr->Report(zERR_TYPE_FAULT, 0, "B: CS: Cutscene without name not allowed", 0, 0, 0x77, 0, 0);
        return;
    }
    zFILE_FILE file(zoptions->GetDirString(5) + name);
    if (file.Exists()) {
        if (zerr->filter_level >= 6)
            zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Loading " + name, 6, 0, 0x81, 0, 0);
        if (!file.Open(false)) {
            zCArchiver* archiver = zarcFactory->CreateArchiverRead(&file, 0);
            Unarchive(*archiver);
            archiver->Release();
            file.Close();
            loaded = 1;
            if (zerr->filter_level >= 6)
                zerr->Message(zSTRING("B: CS: ") + zSTRING(roleList.numInArray) + " roles in cutscene");
        }
    }
    if (!loaded)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Cutscene not found (" + file.GetFullPath() + ")",
            0, 0, 0x97, 0, 0);
    if (!properties)
        properties = new zCCSProps;
    properties->name = name;
    properties->name.Upper();
    properties->PrintDebugInfo();
    SetObjectName(name);
    if (zerr->filter_level >= 8)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Cutscene-Context constructed: " + name,
            8, 0, 0x9E, 0, 0);
}

zCCSCutsceneContext::zCCSCutsceneContext(zCCSBlock* block, int referenceValue, int outputUnit)
    : lastPlayResult(0), allowedToPlay(1), alternativeStageVob(NULL), wasTriggered(0),
      isCutscene(!outputUnit), loaded(1), reference(referenceValue), stage(NULL), poolItem(NULL),
      lastTimeStamp(-1.0f), lastTimePoint(0.0f)
{
    if (!outputUnit) {
        reference = 0;
        blocks = block->blocks;
        properties = NULL;
    } else {
        blocks.InsertEnd(zCCSBlockPosition());
    }
    SetObjectName(zSTRING(reference));
}

zCCSCutsceneContext::~zCCSCutsceneContext()
{
    if (reference >= 0) {
        for (int i = 0; i < blocks.numInArray; ++i)
            blocks.parray[i].block = NULL;
    }
    if (!IsFinished())
        Stop();
    if (alternativeStageVob && alternativeStageVob->homeWorld) {
        for (int i = 0; i < roleList.numInArray; ++i) {
            zCVob* roleVob = roleList.parray[i]->GetRoleVob();
            if (roleVob && roleVob->homeWorld)
                alternativeStageVob->homeWorld->RemoveVob(roleVob);
        }
        alternativeStageVob->Release();
        alternativeStageVob = NULL;
    }
    stage = NULL;
    if (poolItem) {
        poolItem->Release();
        poolItem = NULL;
    }
    if (zerr->filter_level >= 8)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Cutscene-Context destructed.", 8, 0, 0xFC, 0, 0);
}

void zCCSCutsceneContext::UseThisSubBlock(zCCSBlock* block)
{
    if (!isCutscene && block) {
        blocks.DeleteList();
        zCCSBlockPosition position;
        position.block = block;
        blocks.InsertEnd(position);
    }
}

zSTRING zCCSCutsceneContext::GetCSName() const
{
    return properties ? properties->GetName() : zSTRING("");
}

void zCCSCutsceneContext::Archive(zCArchiver& archiver)
{
    zCCutscene::Archive(archiver);
    if (!archiver.InSaveGame())
        return;
    archiver.WriteBool(isCutscene);
    if (isCutscene)
        archiver.WriteBool(allowedToPlay);
    else
        archiver.WriteInt(reference);
    archiver.WriteInt(GetActualBlockNum());
    archiver.WriteObject(alternativeStageVob);
    archiver.WriteObject(stage);
    archiver.WriteBool(wasTriggered);
}

void zCCSCutsceneContext::Unarchive(zCArchiver& archiver)
{
    if (zerr->filter_level >= 6)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: Unarchiving cutscene ... ", 6, 1, 0x136, 0, 0);
    zCCutscene::Unarchive(archiver);
    if (zerr->filter_level >= 6)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: -- Cutscenename: " + GetCSName(), 6, 0, 0x13A, 0, 0);
    if (archiver.InSaveGame()) {
        isCutscene = archiver.ReadBool();
        if (isCutscene) {
            reference = -1;
            allowedToPlay = archiver.ReadBool();
            if (zerr->filter_level >= 6)
                zerr->Report(zERR_TYPE_INFO, 0, "B: CS: -- Is full featured cutscene", 6, 1, 0x150, 0, 0);
        } else {
            reference = archiver.ReadInt();
            if (zerr->filter_level >= 6)
                zerr->Report(zERR_TYPE_INFO, 0, "B: CS: -- Is outputunit cutscene", 6, 1, 0x148, 0, 0);
        }
        ResumeAtBlock(archiver.ReadInt());
        if (alternativeStageVob) {
            alternativeStageVob->Release();
            alternativeStageVob = NULL;
        }
        if (stage) {
            stage->Release();
            stage = NULL;
        }
        alternativeStageVob = dynamic_cast<zCVob*>(archiver.ReadObject(NULL));
        stage = dynamic_cast<zCVob*>(archiver.ReadObject(NULL));
        wasTriggered = archiver.ReadBool();
        lastTimeStamp = -2.0f;
    }
    if (zerr->filter_level >= 6)
        zerr->Report(zERR_TYPE_INFO, 0, "B: CS: -> Cutscene unarchived.", 6, 2, 0x179, 0, 0);
}

zCObject* zCCSCutsceneContext::_CreateNewInstance()
{
    zCCSCutsceneContext* context = new zCCSCutsceneContext;
    zCClassDef::ObjectCreated(context, classDef);
    return context;
}

void zCCSCutsceneContext::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCCSCutsceneContext::_GetClassDef() const
{
    return classDef;
}

int zCCSCutsceneContext::IsCutscene()
{
    return isCutscene;
}

int zCCSCutsceneContext::IsOutputUnit()
{
    return !isCutscene;
}

int zCCSCutsceneContext::DisablePlayCutscenes(int disabled)
{
    int oldDisabled = playDisabled;
    playDisabled = disabled;
    return oldDisabled;
}
