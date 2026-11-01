#include "include/oCollisionObjectMiscChar.h"

static int parserOther = -1;
static int parserVictim = -1;
static int parserItem = -1;
static int parserStateNames[7] = { -1, -1, -1, -1, -1, -1, -1 };
static zSTRING npcStateNames[7] = {
    "ZS_NONE", "ZS_NONE", "ZS_ANSWER", "ZS_DEAD",
    "ZS_UNCONSCIOUS", "ZS_FADEAWAY", "ZS_FOLLOW"
};
static zSTRING lastStateError;

TNpcAIState::TNpcAIState()
    : index(0), loop(0), end(0), timeBehaviour(0), restTime(0.0f), phase(-1), valid(0),
      stateTime(0.0f), prgIndex(-1), isRtnState(0)
{
}

TNpcAIState::~TNpcAIState()
{
}

oCNpc_States::oCNpc_States()
    : npc(0), lastAIState(0), hasRoutine(0), rtnChanged(0), rtnBefore(0), rtnNow(0), rtnRoute(0),
      rtnOverlay(0), rtnOverlayCount(0), walkmode_routine(0), weaponmode_routine(0),
      startNewRoutine(0), aiStateDriven(0), parOther(0), parVictim(0), parItem(0),
      rntChangeCount(0), curStateValid(0), nextStateValid(0)
{
    if (parserOther < 0)
        parserOther = parser.GetIndex("OTHER");
    if (parserVictim < 0)
        parserVictim = parser.GetIndex("VICTIM");
    if (parserItem < 0)
        parserItem = parser.GetIndex("ITEM");
    if (parserStateNames[0] == -1)
    {
        static const char* stateNames[7] = {
            "ZS_NONE", "ZS_NONE", "ZS_ANSWER", "ZS_DEAD",
            "ZS_UNCONSCIOUS", "ZS_FADEAWAY", "ZS_FOLLOW"
        };
        for (int i = 0; i < 7; ++i)
            parserStateNames[i] = parser.GetIndex(stateNames[i]);
    }
}

oCNpc_States::~oCNpc_States()
{
    CloseCutscenes();
    rtnMan.RemoveRoutine(npc);
    rtnNow = NULL;
    rtnBefore = NULL;
    delete rtnRoute;
    rtnRoute = NULL;
    ClearParserReferences();
    npc = NULL;
    parOther = NULL;
    parVictim = NULL;
}

void oCNpc_States::ClearParserReferences()
{
    SetParserInstanceNpc(parserOther, NULL);
    SetParserInstanceNpc(parserVictim, NULL);
    if (parItem)
    {
        parser.SetInstance(parserItem, NULL);
        parItem->Release();
        parItem = NULL;
    }
}

void oCNpc_States::SetOwner(oCNpc* owner)
{
    npc = owner;
}

int oCNpc_States::GetState()
{
    if (!curState.valid)
        return 0;
    if (curState.prgIndex != -1)
        return curState.prgIndex;
    return curState.index;
}

int oCNpc_States::IsInState(int state)
{
    if (curState.valid)
        return curState.index == state || curState.prgIndex == state;
    if (nextState.valid)
        return nextState.index == state || nextState.prgIndex == state;
    return 0;
}

int oCNpc_States::IsScriptStateActive()
{
    return curState.valid && !curState.name.IsEmpty();
}

int oCNpc_States::GetStateTime()
{
    if (!curState.valid)
        return 0;
    return (int)(curState.stateTime / 1000.0f);
}

void oCNpc_States::SetStateTime(int time)
{
    if (curState.valid)
        curState.stateTime = time * 1000.0f;
}

void oCNpc_States::SetRoutine(oCRtnEntry* current, oCRtnEntry* before)
{
    if (rtnNow == current && (!before || before == rtnBefore))
    {
        if (!before)
            rtnBefore = rtnNow;
        return;
    }
    oCRtnEntry* oldRoutine = rtnNow;
    rtnBefore = before ? before : oldRoutine;
    if (oldRoutine && oldRoutine->cutscene)
        oldRoutine->cutscene->ResetPlayState();
    delete rtnRoute;
    rtnRoute = NULL;
    rtnNow = current;
    if (rtnBefore && rtnNow)
        rtnRoute = ogame->GetWorld()->wayNet->FindRoute(rtnBefore->GetWaypoint(),
            rtnNow->GetWaypoint(), npc);
    rntChangeCount = 0;
    if (rtnNow)
        npc->wpname = rtnNow->GetWaypoint();
}

int oCNpc_States::StartRtnState(int force)
{
    if (npc->IsAPlayer())
        return 1;
    startNewRoutine = 0;
    int result = ActivateRtnState(force);
    ++rntChangeCount;
    startNewRoutine = !result;
    return result;
}

int oCNpc_States::ActivateRtnState(int force)
{
    if (npc->IsAPlayer() || !oCNpc::IsScriptStateAIEnabled())
        return 0;
    if (!npc->IsConditionValid())
    {
        lastStateError = "Condition not valid.";
        return 0;
    }
    if (npc->GetEM(0)->GetCutsceneMode())
    {
        if (!force)
        {
            lastStateError = "In Cutscene Mode.";
            return 0;
        }
        zCCutscene* cutscene = npc->GetEM(0)->GetCutscene();
        if (cutscene)
            cutscene->Stop();
    }
    if (!hasRoutine)
    {
        if (npc->startAIState <= 0)
            return 0;
        if (force)
        {
            curState.valid = 0;
            nextState.valid = 0;
        }
        if (curState.valid || nextState.valid)
            return 0;
        return StartAIState(npc->startAIState, 0, 0, 0.0f, 0);
    }
    if (!rtnNow)
    {
        lastStateError = "No current Routine.";
        return 0;
    }
    if (!force)
    {
        if (!npc->GetEM(0)->IsEmpty())
        {
            lastStateError = "EM not empty.";
            return 0;
        }
        if (GetState() == rtnNow->GetState())
            return 1;
        if ((curState.valid && !curState.isRtnState) || nextState.valid)
        {
            lastStateError = "No routine state currently running.";
            return 0;
        }
    }
    npc->wpname = rtnNow->GetWaypoint();
    rtnChanged = 0;
    return StartAIState(rtnNow->GetState(), 1, 0, 0.0f, 1);
}

int oCNpc_States::StartAIState(const zSTRING& stateName, int endOldState, int timeBehaviour,
    float restTime, int isRtnState)
{
    int state = parser.GetIndex(stateName);
    if (state > 0)
        return StartAIState(state, endOldState, timeBehaviour, restTime, 0);
    return 0;
}

int oCNpc_States::StartAIState(int state, int endOldState, int timeBehaviour, float restTime,
    int isRtnState)
{
    if (!oCNpc::IsScriptStateAIEnabled())
        return 0;
    zCCutscene* activeCutscene = npc->GetEM(0)->GetCutscene();
    if (activeCutscene && activeCutscene->IsCutscene())
    {
        lastStateError = "Cutscene mode active.";
        return 0;
    }
    CloseCutscenes();
    if (state < -7 || state == -1)
        return 0;

    nextState.prgIndex = state < 0 ? state : 0;
    if (state < 0)
        nextState.name = npcStateNames[-state - 1];
    else
    {
        int loop;
        int end;
        nextState.name = parser.GetSymbolInfo(state, loop, end);
        if (nextState.name.Search("."))
        {
            parser.SetInstance("SELF", npc);
            parser.CallFunc(state);
            return 1;
        }
    }
    nextState.index = state;
    nextState.loop = parser.GetIndex(nextState.name + "_LOOP");
    nextState.end = parser.GetIndex(nextState.name + "_END");

    zCPar_Symbol* otherSymbol = parser.GetSymbol(parserOther);
    oCNpc* other = otherSymbol ? dynamic_cast<oCNpc*>(otherSymbol->GetInstanceAdr()) : 0;
    SetParserInstanceNpc(parserOther, other);
    zCPar_Symbol* victimSymbol = parser.GetSymbol(parserVictim);
    oCNpc* victim = victimSymbol ? dynamic_cast<oCNpc*>(victimSymbol->GetInstanceAdr()) : 0;
    SetParserInstanceNpc(parserVictim, victim);
    zCPar_Symbol* itemSymbol = parser.GetSymbol(parserItem);
    oCItem* item = itemSymbol ? dynamic_cast<oCItem*>(itemSymbol->GetInstanceAdr()) : 0;
    SetParserInstanceItem(item);

    nextState.timeBehaviour = timeBehaviour;
    nextState.restTime = restTime;
    nextState.phase = -1;
    nextState.valid = 1;
    nextState.isRtnState = isRtnState;
    if (endOldState)
    {
        curState.phase = 2;
        curState.valid = 0;
        if (!npc->GetInteractMob() && !npc->interactItem
            && (!npc->IsAPlayer() || nextState.prgIndex < 0))
        {
            zCModel* model = npc->GetModel();
            if (model)
                model->StopAnisLayerRange(2, 0x100);
            npc->Interrupt(0, 1);
            npc->ClearEM();
        }
    }
    else
        curState.phase = 1;
    if ((npc->sleepingMode & 3) && npc->homeWorld)
        return DoAIState();
    return 0;
}

int oCNpc_States::CanPlayerUseAIState(const TNpcAIState& state)
{
    if (!state.valid)
        return 0;
    if (state.prgIndex == -1)
        return 1;
    return state.prgIndex == -4 || state.prgIndex == -3;
}

int oCNpc_States::CanPlayerUseAIState()
{
    if (CanPlayerUseAIState(curState))
        return 1;
    if (curState.valid && curState.prgIndex != -1)
    {
        curState.valid = 0;
        curState.index = -1;
        curState.prgIndex = -1;
        curState.phase = -1;
        curState.name.Clear();
        nextState.valid = 0;
        nextState.index = -1;
        nextState.prgIndex = -1;
        nextState.phase = -1;
        nextState.name.Clear();
    }
    return 0;
}

int oCNpc_States::DoAIState()
{
    if (curState.valid && !curState.phase)
        curState.stateTime += ztimer.frameTimeFloat;
    if (!oCNpc::IsScriptStateAIEnabled() || npc->GetEM(0)->GetCutscene())
        return 0;

    if (npc->GetEM(0)->IsEmpty())
    {
        if (startNewRoutine || (!curState.valid && !nextState.valid))
            StartRtnState(0);
        if (!curState.valid && nextState.valid)
        {
            lastAIState = curState.index;
            curState = nextState;
            curState.valid = 1;
            curState.stateTime = 0.0f;
            nextState.valid = 0;
            npc->ClearPerception();
        }
        if (npc->IsAPlayer() && !CanPlayerUseAIState())
            return 0;

        parser.SetInstance("SELF", npc);
        parser.SetInstance(parserOther, parOther);
        parser.SetInstance(parserVictim, parVictim);
        parser.SetInstance(parserItem, parItem);
        if (curState.valid)
        {
            if (curState.phase == -1)
            {
                npc->SetPerceptionTime(5000.0f);
                if (curState.index > 0)
                    parser.CallFunc(curState.index);
                curState.phase = 0;
            }
            else if (!curState.phase)
            {
                int continueState = 1;
                if (curState.loop > 0)
                    continueState = parser.CallFunc(curState.loop);
                if (curState.prgIndex == -6)
                    npc->Follow();
                else if (curState.prgIndex == -4)
                    npc->CheckUnconscious();
                if (continueState)
                    curState.phase = 1;
            }
            else if (curState.phase == 1)
            {
                if (curState.end > 0)
                    parser.CallFunc(curState.end);
                curState.phase = 2;
                curState.valid = 0;
            }
        }
    }

    if (curState.valid && !curState.phase)
    {
        if (curState.isRtnState && rtnNow && rtnNow->cutscene && npc->homeWorld)
        {
            oCRtnCutscene* routineCutscene = rtnNow->cutscene;
            zCCSManager* manager = ogame->GetCutsceneManager();
            int pool2 = manager->PoolHasFlags(routineCutscene->csName, 2);
            int pool1 = manager->PoolHasFlags(routineCutscene->csName, 1);
            if (!pool1 && (routineCutscene->ignorePlayState || !pool2))
            {
                int moving = (npc->sleepingMode & 3) != 0;
                if (moving)
                    npc->EndMovement(1);
                zCCSCutsceneContext* started = npc->homeWorld->csPlayer->StartCutscene(
                    routineCutscene->csName, npc);
                if (started)
                    routineCutscene->ignorePlayState = started->ReplayCutscene();
                if (moving)
                    npc->BeginMovement();
            }
        }
        if (curState.timeBehaviour == 1)
            curState.phase = 1;
        else if (curState.timeBehaviour == 2)
        {
            curState.restTime -= ztimer.frameTimeFloat;
            if (curState.restTime <= 0.0f)
                curState.phase = 1;
        }
    }
    return 1;
}

void oCNpc_States::ClearAIState()
{
    curState.valid = 0;
    curState.index = -1;
    curState.prgIndex = -1;
    curState.phase = -1;
    curState.name.Clear();
    nextState.valid = 0;
    nextState.index = -1;
    nextState.prgIndex = -1;
    nextState.phase = -1;
    nextState.name.Clear();
}

void oCNpc_States::EndCurrentState()
{
    if (!curState.valid)
        return;
    curState.phase = 1;
    DoAIState();
}

int oCNpc_States::IsInRoutine()
{
    if (curState.valid)
        return curState.isRtnState;
    return !nextState.valid;
}

int oCNpc_States::IsInCutscene()
{
    return npc->GetEM(0)->GetCutsceneMode();
}

void oCNpc_States::StartOutputUnit(int unit)
{
    if (npc->homeWorld)
        npc->homeWorld->csPlayer->StartOutputUnit(unit, npc);
}

void oCNpc_States::StartCutscene(const zSTRING& name, const zSTRING& text)
{
    cutsceneName = name;
    cutsceneText = text;
    if (npc->homeWorld)
        npc->homeWorld->csPlayer->StartCutscene(name, npc);
}

void oCNpc_States::CloseCutscenes()
{
    if (npc->GetEM(0)->GetCutsceneMode())
        npc->StopFaceAni("VISEME");
    zCCutscene* cutscene = npc->GetEM(0)->GetCutscene();
    if (cutscene)
        cutscene->Stop();
    if (npc->homeWorld)
    {
        zCCSCutsceneContext* started = npc->homeWorld->csPlayer->GetStartedCutscene(npc);
        if (started)
            ((zCCutscene*)started)->Stop();
    }
}

void oCNpc_States::SetParserInstanceNpc(int index, oCNpc* value)
{
    oCNpc** target = index == parserOther ? &parOther : index == parserVictim ? &parVictim : NULL;
    if (!target || *target == value)
        return;
    if (*target)
        (*target)->Release();
    parser.SetInstance(index, value);
    *target = value;
    if (value)
        value->AddRef();
}

void oCNpc_States::SetParserInstanceItem(oCItem* value)
{
    if (parItem == value)
        return;
    parser.SetInstance(parserItem, value);
    if (parItem)
        parItem->Release();
    parItem = value;
    if (value)
        value->AddRef();
}

void oCNpc_States::InitRoutine()
{
    rtnBefore = NULL;
    rtnNow = NULL;
    if (npc->daily_routine)
    {
        parser.SetInstance("SELF", npc);
        parser.CallFunc(npc->daily_routine);
        if (ogame->GetWorld()->wayNet)
            rtnMan.CreateWayBoxes(npc);
    }
}

void oCNpc_States::ChangeRoutine(const zSTRING& routineName)
{
    zSTRING functionName = zSTRING("RTN_") + routineName + "_" + zSTRING(npc->idx);
    ChangeRoutine(parser.GetIndex(functionName));
}

void oCNpc_States::ChangeRoutine(int function)
{
    if (function <= 0)
        return;
    rtnMan.RemoveRoutine(npc);
    npc->daily_routine = function;
    InitRoutine();
    rtnMan.UpdateSingleRoutine(npc);
}

zSTRING oCNpc_States::GetRoutineName()
{
    if (npc->daily_routine > 0)
    {
        zCPar_Symbol* symbol = parser.GetSymbol(npc->daily_routine);
        if (symbol)
            return symbol->name;
    }
    return zSTRING();
}

void oCNpc_States::BeginInsertOverlayRoutine()
{
    if (rtnOverlayCount > 0)
    {
        rtnOverlayCount = 0;
        rtnMan.RemoveOverlay(npc);
        InitRoutine();
        rtnNow = NULL;
        rtnMan.UpdateSingleRoutine(npc);
    }
    rtnMan.RemoveRoutine(npc);
    rtnOverlayCount = 0;
    rtnNow = NULL;
    rtnBefore = NULL;
    rtnOverlay = 1;
}

void oCNpc_States::StopInsertOverlayRoutine()
{
    if (rtnBefore)
    {
        oCRtnEntry* entry = new oCRtnEntry(rtnBefore->hour2, rtnBefore->min2,
            rtnBefore->hour2, rtnBefore->min2, -1, rtnBefore->GetWaypoint(), -1);
        entry->npc = npc;
        entry->overlay = rtnOverlay;
        rtnMan.Insert(npc, entry);
    }
    rtnMan.CreateWayBoxes(npc);
    rtnMan.UpdateSingleRoutine(npc);
    rtnOverlay = 0;
}

void oCNpc_States::RemoveOverlayEntry(oCRtnEntry* entry)
{
    if (!entry || !entry->overlay)
        return;
    rtnMan.RemoveEntry(entry);
    --rtnOverlayCount;
    if (rtnOverlayCount <= 0)
    {
        InitRoutine();
        rtnNow = NULL;
        rtnMan.UpdateSingleRoutine(npc);
    }
    else if (rtnNow == entry)
        rtnNow = NULL;
}

void oCNpc_States::RemoveOverlay()
{
    rtnOverlayCount = 0;
    rtnMan.RemoveOverlay(npc);
    InitRoutine();
    rtnNow = NULL;
    rtnMan.UpdateSingleRoutine(npc);
}

void oCNpc_States::InsertRoutine(int hour1, int min1, int hour2, int min2, int script,
    const zSTRING& waypoint, int instance)
{
    hasRoutine = 1;
    if (rtnOverlay)
    {
        int currentHour;
        int currentMinute;
        ogame->GetWorldTimer()->GetTime(currentHour, currentMinute);
        int endHour = currentHour;
        int endMinute = currentMinute;
        oCWorldTimer::AddTime(endHour, endMinute, hour2, min2);
        int startHour = endHour;
        int startMinute = endMinute;
        oCWorldTimer::AddTime(startHour, startMinute, hour1, min1);
        hour1 = startHour;
        min1 = startMinute;
        hour2 = endHour;
        min2 = endMinute;
        ++rtnOverlayCount;
    }
    oCRtnEntry* entry = new oCRtnEntry(hour1, min1, hour2, min2, script, waypoint, instance);
    entry->overlay = rtnOverlay;
    rtnMan.Insert(npc, entry);
    rtnBefore = entry;
    if (s_bCheckRoutines)
        rtnMan.CheckConsistency(npc);
}

void oCNpc_States::InsertRoutineCS(const zSTRING& name, const zSTRING& role)
{
    if (!rtnBefore)
        return;
    oCRtnCutscene* cutscene = new oCRtnCutscene;
    cutscene->csName = name;
    cutscene->csName.Upper();
    if (cutscene->csName.Search(".") <= 0)
        cutscene->csName += ".CS";
    cutscene->roleName = role;
    cutscene->roleName.Upper();
    cutscene->npc = npc;
    cutscene->active = 0;
    rtnBefore->cutscene = cutscene;
}

int oCNpc_States::GetLastRoutineState()
{
    return rtnNow ? rtnNow->GetState() : 0;
}

int oCNpc_States::GetLastState()
{
    return lastAIState;
}

int oCNpc_States::IsAIStateDriven()
{
    return !hasRoutine && npc->startAIState > 0;
}

void oCNpc_States::InitAIStateDriven(const zVEC3& position)
{
    aiStatePosition = position;
    zCWaypoint* waypoint = ogame->GetWorld()->wayNet->GetNearestWaypoint(position);
    if (waypoint)
        npc->wpname = waypoint->GetName();
}

const zVEC3& oCNpc_States::GetAIStatePosition()
{
    return aiStatePosition;
}

zSTRING oCNpc_States::GetStateInfo()
{
    zSTRING result = curState.name;
    if (!curState.phase)
        result += "_LOOP";
    if (curState.phase == 1)
        result += "_END";
    if (curState.phase == 2)
        result += "_INTERRUPT";
    result += "/";
    if ((curState.valid && curState.isRtnState) || (!curState.valid && !nextState.valid))
        result += "R/";
    if (npc->GetEM(0)->GetCutsceneMode())
        result += "CS/";
    result += "(" + zSTRING(GetStateTime()) + ")";
    result += " Ref:" + zSTRING(npc->refCtr);
    return result;
}

zSTRING oCNpc_States::GetLastError()
{
    return lastStateError;
}

void oCNpc_States::Archive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    archiver.WriteBool(curState.valid);
    archiver.WriteString(curState.name);
    archiver.WriteInt(curState.prgIndex);
    archiver.WriteBool(curState.isRtnState);
    archiver.WriteBool(nextState.valid);
    archiver.WriteString(nextState.name);
    archiver.WriteInt(nextState.prgIndex);
    archiver.WriteBool(nextState.isRtnState);
    archiver.WriteInt(lastAIState);
    archiver.WriteBool(hasRoutine);
    archiver.WriteBool(rtnChanged);
    archiver.WriteBool(rtnOverlay);
    archiver.WriteInt(rtnOverlayCount);
    archiver.WriteInt(walkmode_routine);
    archiver.WriteBool(weaponmode_routine);
    archiver.WriteBool(startNewRoutine);
    archiver.WriteInt(aiStateDriven);
    archiver.WriteVec3(aiStatePosition);
    if (rtnOverlayCount > 0)
        rtnMan.WriteRoutine(archiver, npc, 1);
}

void oCNpc_States::Unarchive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    curState.phase = -1;
    curState.valid = archiver.ReadBool();
    curState.name = archiver.ReadString();
    curState.prgIndex = archiver.ReadInt();
    curState.isRtnState = archiver.ReadBool();
    if (curState.prgIndex < -1 && curState.prgIndex > -8)
        curState.name = npcStateNames[-curState.prgIndex - 1];
    curState.index = parser.GetIndex(curState.name);
    curState.loop = parser.GetIndex(curState.name + "_LOOP");
    curState.end = parser.GetIndex(curState.name + "_END");
    nextState.phase = -1;
    nextState.valid = archiver.ReadBool();
    nextState.name = archiver.ReadString();
    nextState.prgIndex = archiver.ReadInt();
    nextState.isRtnState = archiver.ReadBool();
    if (nextState.prgIndex < -1 && nextState.prgIndex > -8)
        nextState.name = npcStateNames[-nextState.prgIndex - 1];
    nextState.index = parser.GetIndex(nextState.name);
    nextState.loop = parser.GetIndex(nextState.name + "_LOOP");
    nextState.end = parser.GetIndex(nextState.name + "_END");
    lastAIState = archiver.ReadInt();
    hasRoutine = archiver.ReadBool();
    rtnChanged = archiver.ReadBool();
    rtnOverlay = archiver.ReadBool();
    rtnOverlayCount = archiver.ReadInt();
    walkmode_routine = archiver.ReadInt();
    weaponmode_routine = archiver.ReadBool();
    startNewRoutine = archiver.ReadBool();
    aiStateDriven = archiver.ReadInt();
    aiStatePosition = archiver.ReadVec3();
    if (rtnOverlayCount > 0)
        rtnMan.ReadRoutine(archiver, npc);
    rtnMan.FindRoutine(npc, rtnBefore, rtnNow);
}

void oCNpc_States::PackState(zCBuffer& buffer)
{
    buffer.Write(&curState, sizeof(curState));
    buffer.Write(&nextState, sizeof(nextState));
    buffer.WriteInt(lastAIState, 32);
}

void oCNpc_States::UnpackState(zCBuffer& buffer)
{
    buffer.Read(&curState, sizeof(curState));
    buffer.Read(&nextState, sizeof(nextState));
    lastAIState = buffer.ReadInt(32);
}
