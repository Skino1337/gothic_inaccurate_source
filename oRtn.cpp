#include "include/oRtn.h"
#include "include/oGame.h"

oCRtnManager rtnMan;
int s_bCheckRoutines;

void oCRtnCutscene::ResetPlayState()
{
    if (ogame)
        ogame->GetCutsceneManager()->PoolClrFlags(csName, 2);
}

oCRtnCutscene::oCRtnCutscene()
    : npc(0), ignorePlayState(0), active(0)
{
}

oCRtnCutscene::~oCRtnCutscene()
{
    ResetPlayState();
}

oCRtnEntry::oCRtnEntry()
    : hour1(0), min1(0), hour2(0), min2(0), f_script(-1), instance(-1), poi(0), npc(0),
      cutscene(0), overlay(0)
{
}

oCRtnEntry::oCRtnEntry(int hour1Value, int min1Value, int hour2Value, int min2Value, int script,
    const zSTRING& waypoint, int instanceValue)
    : hour1(hour1Value), min1(min1Value), hour2(hour2Value), min2(min2Value), f_script(script),
      wpname(waypoint), instance(instanceValue), poi(0), npc(0), cutscene(0), overlay(0)
{
}

oCRtnEntry::~oCRtnEntry()
{
    delete cutscene;
    cutscene = NULL;
    poi = NULL;
    npc = NULL;
}

void oCRtnEntry::GetTime(int& hour1Value, int& min1Value, int& hour2Value, int& min2Value)
{
    hour1Value = hour1;
    min1Value = min1;
    hour2Value = hour2;
    min2Value = min2;
}

void oCRtnEntry::SetTime(int hour1Value, int min1Value, int hour2Value, int min2Value)
{
    hour1 = hour1Value;
    min1 = min1Value;
    hour2 = hour2Value;
    min2 = min2Value;
}

int oCRtnEntry::GetState()
{
    return f_script;
}

zSTRING oCRtnEntry::GetWaypoint()
{
    return wpname;
}

int oCRtnManager::Sort_Routine(oCRtnEntry* first, oCRtnEntry* second)
{
    if (first->hour1 != second->hour1)
        return first->hour1 < second->hour1 ? -1 : 1;
    if (first->min1 != second->min1)
        return first->min1 < second->min1 ? -1 : 1;
    return first->overlay ? 1 : -1;
}

int oCRtnManager::Sort_WayBoxLimit(TRtn_WayBoxLimit* first, TRtn_WayBoxLimit* second)
{
    return first->value <= second->value ? -1 : 1;
}

int oCRtnManager::Sort_WayBoxX(TRtn_WayBox* first, TRtn_WayBox* second)
{
    return first->bbox.mins.n[0] <= second->bbox.mins.n[0] ? -1 : 1;
}

int oCRtnManager::Sort_WayBoxY(TRtn_WayBox* first, TRtn_WayBox* second)
{
    return first->bbox.mins.n[1] <= second->bbox.mins.n[1] ? -1 : 1;
}

int oCRtnManager::Sort_WayBoxZ(TRtn_WayBox* first, TRtn_WayBox* second)
{
    return first->bbox.mins.n[2] <= second->bbox.mins.n[2] ? -1 : 1;
}

oCRtnManager::oCRtnManager()
    : world(0), nextEntryNode(0)
{
    for (int i = 0; i < 3; ++i)
    {
        indexStart[i] = 0;
        indexEnd[i] = 0;
    }
    rtnList.compare_func = (zCListSort<oCRtnEntry>::TCompare)Sort_Routine;
    wayboxListX.compare_func = (zCListSort<TRtn_WayBox>::TCompare)Sort_WayBoxX;
    wayboxListY.compare_func = (zCListSort<TRtn_WayBox>::TCompare)Sort_WayBoxY;
    wayboxListZ.compare_func = (zCListSort<TRtn_WayBox>::TCompare)Sort_WayBoxZ;
}

oCRtnManager::~oCRtnManager()
{
    while (rtnList.next)
    {
        zCListSort<oCRtnEntry>* entry = rtnList.next;
        rtnList.next = entry->next;
        delete entry->data;
        delete entry;
    }
    zCListSort<TRtn_WayBox>* lists[3] = { &wayboxListX, &wayboxListY, &wayboxListZ };
    for (int i = 0; i < 3; ++i)
    {
        while (lists[i]->next)
        {
            zCListSort<TRtn_WayBox>* entry = lists[i]->next;
            lists[i]->next = entry->next;
            delete entry->data;
            delete entry;
        }
    }
    delete activeList.next;
    activeList.next = NULL;
    activeList.data = NULL;
}

void oCRtnManager::SetWorld(zCWorld* value)
{
    world = value;
}

void oCRtnManager::ShowRoutine(int x, int y, oCNpc* npc)
{
    screen->Print(x, y, npc->GetName(0));
    y += screen->FontY();
    for (zCListSort<oCRtnEntry>* entry = rtnList.next; entry; entry = entry->next)
    {
        if (entry->data->npc != npc || y > 8192)
            continue;
        zSTRING text = GetRoutineString(entry->data);
        if (npc->state.IsInRoutine() && entry->data == npc->state.rtnNow)
            text += "   <<<";
        screen->Print(x, y, text);
        y += screen->FontY();
    }
}

zSTRING oCRtnManager::GetRoutineString(oCRtnEntry* entry)
{
    zSTRING result = zSTRING(entry->hour1) + ":" + zSTRING(entry->min1) + " - " +
        zSTRING(entry->hour2) + ":" + zSTRING(entry->min2);
    zCPar_Symbol* symbol = parser.GetSymbol(entry->f_script);
    if (symbol)
        result += " " + symbol->name;
    else
        result += " UNKNOWN";
    result += "  WP:" + entry->wpname;
    if (entry->cutscene)
        result += "  CS:" + entry->cutscene->csName;
    if (entry->overlay)
        result += "  (O)";
    return result;
}

void oCRtnManager::Insert(oCNpc* npc, oCRtnEntry* entry)
{
    if (!npc || !entry)
        return;
    entry->npc = npc;
    rtnList.InsertSort(entry);
}

void oCRtnManager::UpdateGlobalRoutineEntry()
{
    if (!ogame || !ogame->GetWorldTimer())
        return;
    for (nextEntryNode = rtnList.next; nextEntryNode; nextEntryNode = nextEntryNode->next)
    {
        if (!ogame->GetWorldTimer()->IsLater(nextEntryNode->data->hour1, nextEntryNode->data->min1))
            break;
    }
}

void oCRtnManager::UpdateSingleRoutine(oCNpc* npc)
{
    if (!npc->state.hasRoutine)
    {
        if (npc->state.IsInRoutine())
        {
            npc->ClearEM();
            npc->Interrupt(0, 1);
            npc->state.rtnNow = NULL;
            npc->state.StartRtnState(0);
        }
        return;
    }
    oCRtnEntry* before = NULL;
    oCRtnEntry* current = NULL;
    if (FindRoutine(npc, before, current))
    {
        if (!npc->state.IsInRoutine())
            npc->state.SetRoutine(current, before);
        else
        {
            if (npc->GetEM(0)->IsEmpty())
            {
                npc->ClearEM();
                npc->Interrupt(0, 1);
            }
            npc->state.rtnNow = NULL;
            npc->state.SetRoutine(current, before);
            if (npc->GetEM(0)->IsEmpty())
                npc->state.StartRtnState(1);
        }
    }
    UpdateGlobalRoutineEntry();
}

void oCRtnManager::RestartRoutines()
{
    for (zCList<oCNpc>* node = (zCList<oCNpc>*)ogame->GetWorld()->voblist_npcs; node;
        node = node->next)
    {
        oCNpc* npc = node->data;
        if (npc->IsAPlayer())
            continue;
        npc->state.curState.valid = 0;
        if (!npc->state.hasRoutine)
        {
            if (npc->state.IsAIStateDriven())
            {
                npc->ClearEM();
                npc->Interrupt(0, 1);
                npc->state.rtnNow = NULL;
                npc->state.StartRtnState(0);
            }
            continue;
        }
        oCRtnEntry* before = NULL;
        oCRtnEntry* current = NULL;
        if (FindRoutine(npc, before, current))
        {
            npc->ClearEM();
            npc->Interrupt(0, 1);
            npc->state.rtnNow = NULL;
            npc->state.nextState.valid = 0;
            npc->state.SetRoutine(current, before);
            npc->state.StartRtnState(1);
        }
    }
    UpdateGlobalRoutineEntry();
}

int oCRtnManager::FindRoutine(oCNpc* npc, oCRtnEntry*& before, oCRtnEntry*& now)
{
    before = NULL;
    now = NULL;
    oCWorldTimer* timer = ogame->GetWorldTimer();
    for (zCListSort<oCRtnEntry>* entry = rtnList.next; entry; entry = entry->next)
    {
        oCRtnEntry* data = entry->data;
        if (data->npc != npc)
            continue;
        if (timer->IsTimeBetween(data->hour1, data->min1, data->hour2, data->min2))
            now = data;
        else
            before = data;
    }
    if (now)
        return 1;
    if (before)
    {
        now = before;
        return 1;
    }
    return 0;
}

void oCRtnManager::CheckRoutines()
{
    if (!nextEntryNode)
        nextEntryNode = rtnList.next;
    oCWorldTimer* timer = ogame->GetWorldTimer();
    while (nextEntryNode)
    {
        oCRtnEntry* data = nextEntryNode->data;
        if (timer->IsTimeBetween(data->hour1, data->min1, data->hour2, data->min2))
        {
            if (data->f_script == -1)
            {
                data->npc->state.RemoveOverlay();
                return;
            }
            data->npc->state.SetRoutine(data, data->npc->state.rtnNow);
            if (data->npc->state.IsInRoutine())
                data->npc->state.StartRtnState(1);
            nextEntryNode = nextEntryNode->next;
            continue;
        }
        if (data->overlay && timer->IsLater(data->hour1, data->min1))
        {
            nextEntryNode = nextEntryNode->next;
            data->npc->state.RemoveOverlayEntry(data);
            continue;
        }
        break;
    }
}

zVEC3 oCRtnManager::GetRoutinePos(oCNpc* npc)
{
    zVEC3 position = npc->trafoObjToWorld.GetTranslation();
    if (!ogame || !ogame->GetWorld())
        return position;
    oCRtnEntry* current = npc->state.rtnNow;
    if (current)
    {
        if (!npc->state.rtnRoute)
        {
            zCWaypoint* waypoint = ogame->GetWorld()->wayNet->GetWaypoint(current->GetWaypoint());
            if (waypoint)
                return waypoint->GetPositionWorld();
        }
        else
        {
            int hour;
            int minute;
            ogame->GetWorldTimer()->GetTime(hour, minute);
            if (hour < current->hour1)
                hour += 24;
            int elapsed = (hour - current->hour1) * 60 - current->min1 + minute;
            npc->state.rtnRoute->GetInterpolatedPosition(0.5f, elapsed, position);
            return position;
        }
    }
    oCRtnEntry* before = npc->state.rtnBefore;
    if (before)
    {
        zCWaypoint* waypoint = ogame->GetWorld()->wayNet->GetWaypoint(before->GetWaypoint());
        if (waypoint)
            return waypoint->GetPositionWorld();
    }
    return position;
}

void oCRtnManager::WriteRoutine(zCArchiver& archiver, oCNpc* npc, int overlay)
{
    int count = 0;
    for (zCListSort<oCRtnEntry>* entry = rtnList.next; entry; entry = entry->next)
    {
        if (entry->data && entry->data->npc == npc && (!overlay || entry->data->overlay))
            ++count;
    }
    archiver.WriteInt(count);
    for (zCListSort<oCRtnEntry>* entry = rtnList.next; entry; entry = entry->next)
    {
        oCRtnEntry* data = entry->data;
        if (!data || data->npc != npc || (overlay && !data->overlay))
            continue;
        archiver.WriteInt(data->hour1);
        archiver.WriteInt(data->min1);
        archiver.WriteInt(data->hour2);
        archiver.WriteInt(data->min2);
        archiver.WriteInt(data->f_script);
        archiver.WriteInt(data->instance);
        archiver.WriteBool(data->overlay);
        archiver.WriteString(data->wpname);
    }
}

void oCRtnManager::ReadRoutine(zCArchiver& archiver, oCNpc* npc)
{
    int count = archiver.ReadInt();
    RemoveRoutine(npc);
    for (int i = 0; i < count; ++i)
    {
        oCRtnEntry* entry = new oCRtnEntry;
        entry->hour1 = archiver.ReadInt();
        entry->min1 = archiver.ReadInt();
        entry->hour2 = archiver.ReadInt();
        entry->min2 = archiver.ReadInt();
        entry->f_script = archiver.ReadInt();
        entry->instance = archiver.ReadInt();
        entry->overlay = archiver.ReadBool();
        entry->wpname = archiver.ReadString();
        Insert(npc, entry);
    }
    CreateWayBoxes(npc);
}

void oCRtnManager::RemoveEntry(oCRtnEntry* entry)
{
    if (!entry)
        return;
    zCListSort<oCRtnEntry>* previous = &rtnList;
    while (previous->next && previous->next->data != entry)
        previous = previous->next;
    if (previous->next)
    {
        zCListSort<oCRtnEntry>* node = previous->next;
        previous->next = node->next;
        delete node;
    }
    delete entry;
}

void oCRtnManager::RemoveRoutine(oCNpc* npc)
{
    zCListSort<oCRtnEntry>* current = &rtnList;
    while (current->next)
    {
        if (current->next->data->npc != npc)
        {
            current = current->next;
            continue;
        }
        zCListSort<oCRtnEntry>* node = current->next;
        if (nextEntryNode == node)
            nextEntryNode = node->next;
        current->next = node->next;
        delete node->data;
        delete node;
    }
    UpdateGlobalRoutineEntry();
}

void oCRtnManager::CheckConsistency(oCNpc* npc)
{
    for (zCListSort<oCRtnEntry>* first = rtnList.next; first; first = first->next)
    {
        if (first->data->npc != npc || first->data->overlay)
            continue;
        for (zCListSort<oCRtnEntry>* second = rtnList.next; second; second = second->next)
        {
            if (first == second || second->data->npc != npc || second->data->overlay)
                continue;
            if (first->data->hour1 <= second->data->hour2 && second->data->hour1 <= first->data->hour2)
                zerr.Warning("C: TA Error for NPC " + npc->GetInstanceName() +
                    " please check the daily routines !");
        }
    }
}

void oCRtnManager::RemoveOverlay(oCNpc* npc)
{
    zCListSort<oCRtnEntry>* current = &rtnList;
    while (current->next)
    {
        if (current->next->data->npc != npc || !current->next->data->overlay)
        {
            current = current->next;
            continue;
        }
        zCListSort<oCRtnEntry>* node = current->next;
        if (nextEntryNode == node)
            nextEntryNode = node->next;
        current->next = node->next;
        delete node->data;
        delete node;
    }
    CreateWayBoxes(npc);
    UpdateGlobalRoutineEntry();
}

void oCRtnManager::SetDailyRoutinePos(int position)
{
    for (zCList<oCNpc>* node = (zCList<oCNpc>*)ogame->GetWorld()->voblist_npcs; node;
        node = node->next)
    {
        oCNpc* npc = node->data;
        if (!position || !npc->state.IsAIStateDriven())
            npc->state.ChangeRoutine(position);
    }
    RestartRoutines();
}

void oCRtnManager::InitWayBoxes()
{
    for (int i = 0; i < 3; ++i)
        wayboxList[i].Compare = (int(*)(const void*, const void*))Sort_WayBoxLimit;
}

void oCRtnManager::InsertWaybox(oCNpc* npc, zVEC3& minimum, zVEC3& maximum)
{
    for (int axis = 0; axis < 3; ++axis)
    {
        TRtn_WayBoxLimit* first = new TRtn_WayBoxLimit;
        first->begin = 1;
        first->bbox.mins = minimum;
        first->bbox.maxs = maximum;
        first->value = minimum.n[axis];
        first->npc = npc;
        wayboxList[axis].InsertSort(first);
        TRtn_WayBoxLimit* second = new TRtn_WayBoxLimit;
        second->begin = 0;
        second->bbox = first->bbox;
        second->value = maximum.n[axis];
        second->npc = npc;
        wayboxList[axis].InsertSort(second);
    }
}

void oCRtnManager::RemoveWayBoxes(oCNpc* npc)
{
    zCListSort<TRtn_WayBox>* lists[3] = { &wayboxListX, &wayboxListY, &wayboxListZ };
    for (int axis = 0; axis < 3; ++axis)
    {
        zCListSort<TRtn_WayBox>* current = lists[axis];
        while (current->next)
        {
            if (current->next->data->npc != npc)
            {
                current = current->next;
                continue;
            }
            zCListSort<TRtn_WayBox>* node = current->next;
            current->next = node->next;
            delete node->data;
            delete node;
        }
    }
}

void oCRtnManager::CreateWayBoxes(oCNpc* npc)
{
    if (!npc->state.hasRoutine)
        return;
    RemoveWayBoxes(npc);
    zVEC3 minimum(0.0f, 0.0f, 0.0f);
    zVEC3 maximum(0.0f, 0.0f, 0.0f);
    int found = 0;
    for (zCListSort<oCRtnEntry>* entry = rtnList.next; entry; entry = entry->next)
    {
        if (entry->data->npc != npc)
            continue;
        zCWaypoint* waypoint = ogame->GetWorld()->wayNet->GetWaypoint(entry->data->GetWaypoint());
        if (!waypoint || !waypoint->GetVob())
            continue;
        zVEC3 position = waypoint->GetVob()->trafoObjToWorld.GetTranslation();
        if (!found)
        {
            minimum = position;
            maximum = position;
            found = 1;
        }
        else
        {
            for (int axis = 0; axis < 3; ++axis)
            {
                if (position.n[axis] < minimum.n[axis])
                    minimum.n[axis] = position.n[axis];
                if (position.n[axis] > maximum.n[axis])
                    maximum.n[axis] = position.n[axis];
            }
        }
    }
    if (found)
    {
        TRtn_WayBox* box = new TRtn_WayBox;
        box->bbox.mins = minimum;
        box->bbox.maxs = maximum;
        box->npc = npc;
        box->found = 0;
        wayboxListX.InsertSort(box);
        wayboxListY.InsertSort(box);
        wayboxListZ.InsertSort(box);
    }
}

void oCRtnManager::DrawWayBoxes()
{
    for (zCListSort<TRtn_WayBox>* entry = wayboxListX.next; entry; entry = entry->next)
        entry->data->bbox.Draw(zCOLOR(0, 0, 255, 255));
}

void oCRtnManager::CreateActiveList(oCNpc* npc, zTBBox3D& bbox)
{
    delete activeList.next;
    activeList.next = NULL;
    activeList.data = NULL;
    for (int axis = 0; axis < 3; ++axis)
    {
        for (int i = 0; i < wayboxList[axis].numInArray; ++i)
        {
            TRtn_WayBoxLimit* limit = wayboxList[axis][i];
            if (bbox.mins.n[axis] <= limit->value && limit->value <= bbox.maxs.n[axis])
                activeList.Insert(limit->npc);
        }
    }
}

int oCRtnManager::CollidesWithWayBox(zTBBox3D& bbox, zCList<oCNpc>& npcList)
{
    zCListSort<TRtn_WayBox>* lists[3] = { &wayboxListX, &wayboxListY, &wayboxListZ };
    for (int axis = 0; axis < 3; ++axis)
    {
        for (zCListSort<TRtn_WayBox>* entry = lists[axis]->next; entry; entry = entry->next)
        {
            TRtn_WayBox* box = entry->data;
            if (!axis)
                box->found = 0;
            if (box->bbox.mins.n[axis] <= bbox.maxs.n[axis] &&
                box->bbox.maxs.n[axis] >= bbox.mins.n[axis])
            {
                ++box->found;
                if (box->found == 2)
                {
                    int contains = 0;
                    for (zCList<oCNpc>* node = npcList.next; node; node = node->next)
                        contains |= node->data == box->npc;
                    if (!contains)
                        npcList.Insert(box->npc);
                }
            }
        }
    }
    return npcList.GetNumInList() > 0;
}

int oCRtnManager::GetNumOfWayBoxes()
{
    int count = 0;
    for (zCListSort<TRtn_WayBox>* entry = wayboxListX.next; entry; entry = entry->next)
        ++count;
    return count;
}
