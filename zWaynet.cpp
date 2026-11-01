#include "include/oWaynet.h"

zCVobWaypoint::zCVobWaypoint()
{
    type = 7;
}

zCVobWaypoint::~zCVobWaypoint()
{
}

zCClassDef* zCVobWaypoint::_GetClassDef() const
{
    return classDef;
}

zCWayNet::zCWayNet()
    : world(NULL), routeCtr(0)
{
    openList.compare_func = (zCListSort<zCWaypoint>::TCompare)SortOpenList;
}

zCWayNet::zCWayNet(zCWorld* worldValue)
    : world(worldValue), routeCtr(0)
{
    wplist.compare_func = (zCListSort<zCWaypoint>::TCompare)SortWpList;
    openList.compare_func = (zCListSort<zCWaypoint>::TCompare)SortOpenList;
}

zCWayNet::~zCWayNet()
{
    wplist.DeleteListDatas();
    waylist.DeleteListDatas();
    world = NULL;
}

void zCWayNet::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCWayNet::_GetClassDef() const
{
    return classDef;
}

zCWaypoint* zCWayNet::HasWaypoint(float x, float y, float z)
{
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
    {
        zCWaypoint* waypoint = entry->data;
        if (waypoint->pos.n[0] == x && waypoint->pos.n[1] == y && waypoint->pos.n[2] == z)
            return waypoint;
    }
    return NULL;
}

int zCWayNet::HasWaypoint(zCWaypoint* waypoint)
{
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
    {
        if (entry->data == waypoint)
            return 1;
    }
    return 0;
}

zCWaypoint* zCWayNet::HasWaypoint(zVEC3& position)
{
    return HasWaypoint(position.n[0], position.n[1], position.n[2]);
}

zCWaypoint* zCWayNet::InsertWaypoint(float x, float y, float z)
{
    zCWaypoint* waypoint = HasWaypoint(x, y, z);
    if (waypoint)
        return waypoint;
    waypoint = zfactory->CreateWaypoint();
    waypoint->pos.n[0] = x;
    waypoint->pos.n[1] = y;
    waypoint->pos.n[2] = z;
    waypoint->dir.n[0] = 1.0f;
    waypoint->dir.n[1] = 0.0f;
    waypoint->dir.n[2] = 0.0f;
    waypoint->wpvob = NULL;
    InsertWaypoint(waypoint);
    return waypoint;
}

void zCWayNet::InsertWaypoint(zCWaypoint* waypoint)
{
    if (!HasWaypoint(waypoint))
        wplist.InsertSort(waypoint);
}

void zCWayNet::InsertWaypoint(zCWaypoint* first, zCWaypoint* middle, zCWaypoint* last)
{
    if (first == middle || middle == last || first == last)
        return;
    InsertWaypoint(first);
    InsertWaypoint(middle);
    InsertWaypoint(last);
    DeleteWay(first, last);
    CreateWay(first, middle);
    CreateWay(middle, last);
}

void zCWayNet::DeleteWaypoint(zCWaypoint* waypoint)
{
    if (!HasWaypoint(waypoint))
        return;
    zCList<zCWay>* entry = waylist.next;
    while (entry)
    {
        zCList<zCWay>* next = entry->next;
        zCWay* way = entry->data;
        if (way->left == waypoint || way->right == waypoint)
        {
            waylist.Remove(way);
            if (way->left)
                way->left->wayList.Remove(way);
            if (way->right)
                way->right->wayList.Remove(way);
            delete way;
        }
        entry = next;
    }
    wplist.Remove(waypoint);
    waypoint->Free();
    waypoint->Release();
}

void zCWayNet::CreateWay(zCWaypoint* first, zCWaypoint* second)
{
    if (first == second)
        return;
    InsertWaypoint(first);
    InsertWaypoint(second);
    if (first->HasWay(second))
        return;
    zCWay* way = zfactory->CreateWay();
    way->Init(first, second);
    waylist.Insert(way);
}

void zCWayNet::DeleteWay(zCWaypoint* first, zCWaypoint* second)
{
    zCWay* way = first->HasWay(second);
    if (!way)
        return;
    first->wayList.Remove(way);
    second->wayList.Remove(way);
    waylist.Remove(way);
    if (way->left)
        way->left->wayList.Remove(way);
    if (way->right)
        way->right->wayList.Remove(way);
    delete way;
}


zCWaypoint* zCWayNet::GetNearestWaypoint(const zVEC3& position)
{
    zCWaypoint* result = NULL;
    float nearest = 3.40282347e+38f;
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
    {
        zCWaypoint* waypoint = entry->data;
        float dx = waypoint->pos.n[0] - position.n[0];
        float dy = waypoint->pos.n[1] - position.n[1];
        float dz = waypoint->pos.n[2] - position.n[2];
        float distance = (dx < 0.0f ? -dx : dx) + (dy < 0.0f ? -dy : dy) +
            (dz < 0.0f ? -dz : dz);
        if (distance < nearest)
        {
            nearest = distance;
            result = waypoint;
        }
    }
    return result;
}

zCWaypoint* zCWayNet::GetSecNearestWaypoint(zVEC3& position)
{
    zCWaypoint* nearestWaypoint = NULL;
    zCWaypoint* secondWaypoint = NULL;
    int nearest = 0x7fffffff;
    int second = 0x7fffffff;
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
    {
        zCWaypoint* waypoint = entry->data;
        int dx = (int)(waypoint->pos.n[0] - position.n[0]);
        int dy = (int)(waypoint->pos.n[1] - position.n[1]);
        int dz = (int)(waypoint->pos.n[2] - position.n[2]);
        int distance = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy) + (dz < 0 ? -dz : dz);
        if (distance < nearest)
        {
            second = nearest;
            secondWaypoint = nearestWaypoint;
            nearest = distance;
            nearestWaypoint = waypoint;
        }
        else if (distance < second)
        {
            second = distance;
            secondWaypoint = waypoint;
        }
    }
    return secondWaypoint;
}

void zCWayNet::CorrectHeight()
{
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
        entry->data->CorrectHeight(world);
}

void zCWayNet::Draw(zCCamera* camera)
{
    camera->Activate();
    for (zCList<zCWay>* entry = waylist.next; entry; entry = entry->next)
        entry->data->Draw(camera);
}

void zCWayNet::UpdateVobDependencies()
{
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
    {
        zCWaypoint* waypoint = entry->data;
        if (waypoint->wpvob)
        {
            waypoint->pos = waypoint->wpvob->trafoObjToWorld.v[3];
            waypoint->dir = waypoint->wpvob->trafoObjToWorld.v[2];
        }
    }
}

void zCWayNet::ClearVobDependencies()
{
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
    {
        zCWaypoint* waypoint = entry->data;
        zCVob* vob = waypoint->wpvob;
        if (vob)
        {
            if (vob->homeWorld)
                vob->homeWorld->RemoveVob(vob);
            vob->Release();
            waypoint->wpvob = NULL;
        }
    }
}

void zCWayNet::CreateVobDependencies(zCWorld* targetWorld)
{
    if (!targetWorld)
        return;
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
    {
        zCWaypoint* waypoint = entry->data;
        if (!waypoint->wpvob)
        {
            zCVobWaypoint* vob = new zCVobWaypoint;
            zCClassDef::ObjectCreated(vob, zCVobWaypoint::classDef);
            waypoint->wpvob = vob;
            vob->SetVobName(waypoint->name);
            vob->SetPositionWorld(waypoint->pos);
            targetWorld->AddVob(vob);
            vob->SetHeadingAtWorld(waypoint->dir);
        }
    }
}


zSTRING zCWayNet::MergeWaypoints()
{
    zSTRING report;
    zCListSort<zCWaypoint>* firstEntry = wplist.next;
    while (firstEntry)
    {
        zCWaypoint* first = firstEntry->data;
        zCListSort<zCWaypoint>* secondEntry = firstEntry->next;
        while (secondEntry)
        {
            zCListSort<zCWaypoint>* next = secondEntry->next;
            zCWaypoint* second = secondEntry->data;
            if (second->wpvob)
                second->name = second->wpvob->GetObjectName();
            if (first->wpvob)
                first->name = first->wpvob->GetObjectName();
            if (first->name == second->name)
            {
                report.Append(first->name);
                report += "\n";
                zCList<zCWay>* wayEntry = second->wayList.next;
                while (wayEntry)
                {
                    zCList<zCWay>* nextWay = wayEntry->next;
                    zCWay* way = wayEntry->data;
                    zCWaypoint* other = way->right == second ? way->left : way->right;
                    CreateWay(first, other);
                    wayEntry = nextWay;
                }
                DeleteWaypoint(second);
            }
            secondEntry = next;
        }
        firstEntry = firstEntry->next;
    }
    return report;
}

zSTRING zCWayNet::CheckConsistency(int)
{
    zSTRING duplicates;
    int failed = 0;
    for (zCListSort<zCWaypoint>* firstEntry = wplist.next; firstEntry;
        firstEntry = firstEntry->next)
    {
        zCWaypoint* first = firstEntry->data;
        for (zCListSort<zCWaypoint>* secondEntry = firstEntry->next; secondEntry;
            secondEntry = secondEntry->next)
        {
            zCWaypoint* second = secondEntry->data;
            if (second->wpvob)
                second->name = second->wpvob->GetObjectName();
            if (first->wpvob)
                first->name = first->wpvob->GetObjectName();
            if (first->name == second->name)
            {
                duplicates.Append(first->name);
                duplicates += "\n";
                failed = 1;
            }
        }
    }
    if (failed)
        return zSTRING("U: WAYNET: Waynet consistency check failed. Check zSpy-Messages.");
    return zSTRING();
}

void zCWayNet::AddWays(zCWaypoint* target, zCWaypoint* source)
{
    for (zCList<zCWay>* entry = source->wayList.next; entry; entry = entry->next)
    {
        zCWay* way = entry->data;
        zCWaypoint* other = way->right == source ? way->left : way->right;
        if (target != other)
            CreateWay(target, other);
    }
}


zCWay::zCWay()
    : cost(0), usedCtr(0), chasmDepth(0.0f), chasm(0), jump(0), left(NULL), right(NULL)
{
}

int zCWay::IsObjectOnWay(const zCVob*)
{
    return 0;
}

zCWay::zCWay(zCWaypoint* leftWaypoint, zCWaypoint* rightWaypoint)
    : cost(0), usedCtr(0), chasmDepth(0.0f), chasm(0), jump(0), left(NULL), right(NULL)
{
    Init(leftWaypoint, rightWaypoint);
}

void zCWay::Init(zCWaypoint* leftWaypoint, zCWaypoint* rightWaypoint)
{
    left = leftWaypoint;
    right = rightWaypoint;
    chasm = 0;
    chasmDepth = 0.0f;
    jump = 0;
    usedCtr = 0;
    left->wayList.Insert(this);
    right->wayList.Insert(this);
    EstimateCost();
}

void zCWay::Free()
{
    if (left)
        left->wayList.Remove(this);
    if (right)
        right->wayList.Remove(this);
}

zCWay::~zCWay()
{
    left = NULL;
    right = NULL;
}

zCWaypoint* zCWay::GetGoalWaypoint(zCWaypoint* waypoint)
{
    return left != waypoint ? left : right;
}

void zCWay::EstimateCost()
{
    int dx = (int)(left->pos.n[0] - right->pos.n[0]);
    int dy = (int)(left->pos.n[1] - right->pos.n[1]);
    int dz = (int)(left->pos.n[2] - right->pos.n[2]);
    cost = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy) + (dz < 0 ? -dz : dz);
}

float zCWay::GetLength()
{
    float values[3];
    values[0] = left->pos.n[0] - right->pos.n[0];
    values[1] = left->pos.n[1] - right->pos.n[1];
    values[2] = left->pos.n[2] - right->pos.n[2];
    for (int i = 0; i < 3; ++i)
    {
        if (values[i] < 0.0f)
            values[i] = -values[i];
    }
    float largest = values[0];
    float smaller = 0.0f;
    for (int i = 0; i < 3; ++i)
    {
        if (values[i] > largest)
            largest = values[i];
        smaller += values[i];
    }
    smaller -= largest;
    return largest - 0.0625f * largest + smaller * 0.375f;
}

float zCWay::GetJumpDistance()
{
    float dx = left->pos.n[0] - right->pos.n[0];
    float dz = left->pos.n[2] - right->pos.n[2];
    if (dx < 0.0f)
        dx = -dx;
    if (dz < 0.0f)
        dz = -dz;
    float largest = dx > dz ? dx : dz;
    float smaller = dx > dz ? dz : dx;
    return (largest - 0.0625f * largest + smaller * 0.375f) * 0.8f;
}

zVEC3 zCWay::GetDistancePos(float distance, zCWaypoint* start)
{
    zCWaypoint* goal = GetGoalWaypoint(start);
    zVEC3 direction = start->pos - goal->pos;
    float scale = distance / direction.LengthApprox();
    return zVEC3(start->pos.n[0] + direction.n[0] * scale,
        start->pos.n[1] + direction.n[1] * scale,
        start->pos.n[2] + direction.n[2] * scale);
}

int zCWay::CanBeUsed(const zCVob*)
{
    return 1;
}

float zCWay::GetChasm()
{
    return -chasmDepth;
}

int zCWay::CanJump()
{
    return jump;
}

int zCWayNet::SortWpList(zCWaypoint* first, zCWaypoint* second)
{
    if (first->wpvob)
        first->name = first->wpvob->GetObjectName();
    if (second->wpvob)
        second->name = second->wpvob->GetObjectName();
    return first->name < second->name ? -1 : 1;
}


void zCWay::Draw(zCCamera* camera)
{
    zVEC3 start = camera->camMatrix * left->pos;
    zVEC3 end = camera->camMatrix * right->pos;
    Alg_ClipAtZ0(start, end);
    if (start.n[2] <= 0.0f || end.n[2] <= 0.0f)
        return;
    int x1 = int(camera->vpData.xcenter + start.n[0] * camera->viewDistanceX / start.n[2]);
    int y1 = int(camera->vpData.ycenter - start.n[1] * camera->viewDistanceY / start.n[2]);
    int x2 = int(camera->vpData.xcenter + end.n[0] * camera->viewDistanceX / end.n[2]);
    int y2 = int(camera->vpData.ycenter - end.n[1] * camera->viewDistanceY / end.n[2]);
    if (screen->ClipLine(x1, y1, x2, y2))
        zrenderer->DrawLineZ(x1, y1, 1.0f / start.n[2], x2, y2, 1.0f / end.n[2], GFX_RED);
}

zVEC3 zCWay::GetFloor(zCWorld* targetWorld, const zVEC3& position)
{
    zVEC3 down;
    down.n[0] = 0.0f;
    down.n[1] = -1000.0f;
    down.n[2] = 0.0f;
    if (targetWorld->TraceRayNearestHit(position, down, (zCArray<zCVob*>*)NULL, 0x20) &&
        targetWorld->traceRayReport.foundPoly)
        return targetWorld->traceRayReport.foundIntersection;
    zVEC3 floor = position;
    floor.n[1] -= 1000.0f;
    return floor;
}

void zCWay::CalcProperties(zCWorld* targetWorld)
{
    zCWaypoint* start = left;
    zCWaypoint* end = right;
    if (end->pos.n[1] < start->pos.n[1] ||
        (end->pos.n[1] == start->pos.n[1] && end->pos.n[0] < start->pos.n[0]) ||
        (end->pos.n[1] == start->pos.n[1] && end->pos.n[0] == start->pos.n[0] &&
            end->pos.n[2] < start->pos.n[2]))
    {
        start = right;
        end = left;
    }
    zVEC3 delta = end->pos - start->pos;
    if (delta.n[0] == 0.0f && delta.n[1] == 0.0f && delta.n[2] == 0.0f)
        return;
    float length = delta.Length();
    int samples = int(length * 0.05f);
    zVEC3 step = delta * (20.0f / length);
    zVEC3 position = start->pos;
    zVEC3 previousFloor = GetFloor(targetWorld, position);
    float firstDepth = 0.0f;
    float secondDepth = 0.0f;
    for (int i = 0; i < samples; ++i)
    {
        position += step;
        zVEC3 sample = position;
        sample.n[1] += 20.0f;
        zVEC3 floor = GetFloor(targetWorld, sample);
        float depth = floor.n[1] - previousFloor.n[1];
        if (i < samples / 2)
            firstDepth += depth;
        else
            secondDepth += depth;
        previousFloor = floor;
    }
    if (firstDepth < -200.0f)
    {
        chasmDepth = firstDepth;
        if (firstDepth * -0.5f < secondDepth)
            jump = 1;
    }
}


zCWaypoint::zCWaypoint()
    : routeCtr(0), curCost(0), estCost(0), score(0), curList(0), parent(NULL),
      waterDepth(0), underWater(0), wpvob(NULL)
{
    pos.n[0] = 0.0f;
    pos.n[1] = 0.0f;
    pos.n[2] = 0.0f;
    dir.n[0] = 1.0f;
    dir.n[1] = 0.0f;
    dir.n[2] = 0.0f;
    wayList.data = NULL;
    wayList.next = NULL;
}

int zCWaypoint::CanBeUsed(const zCVob*)
{
    return 1;
}

void zCWaypoint::Init(zVEC3& position)
{
    wpvob = NULL;
    pos = position;
    dir.n[0] = 1.0f;
    dir.n[1] = 0.0f;
    dir.n[2] = 0.0f;
}

void zCWaypoint::Init(float x, float y, float z)
{
    wpvob = NULL;
    pos.n[0] = x;
    pos.n[1] = y;
    pos.n[2] = z;
    dir.n[0] = 1.0f;
    dir.n[1] = 0.0f;
    dir.n[2] = 0.0f;
}

void zCWaypoint::Init(zCVob* vob)
{
    name = vob->GetObjectName();
    name.Upper();
    wpvob = vob;
    pos = vob->trafoObjToWorld.v[3];
    dir = vob->trafoObjToWorld.v[2];
    vob->AddRef();
}

void zCWaypoint::Free()
{
    wayList.DeleteListDatas();
    parent = NULL;
    if (wpvob)
    {
        if (wpvob->homeWorld)
            wpvob->homeWorld->RemoveVob(wpvob);
        wpvob->Release();
        wpvob = NULL;
    }
}

zCWaypoint::~zCWaypoint()
{
    wpvob = NULL;
    parent = NULL;
}

zVEC3 zCWaypoint::GetPositionWorld()
{
    return pos;
}

void zCWaypoint::UpdatePositionWorld()
{
    if (wpvob)
    {
        pos = wpvob->trafoObjToWorld.v[3];
        dir = wpvob->trafoObjToWorld.v[2];
    }
}

void zCWaypoint::SetName(const zSTRING& value)
{
    name = value;
    name.Upper();
    if (wpvob)
        wpvob->SetVobName(name);
}

zSTRING zCWaypoint::GetName() const
{
    if (wpvob)
        const_cast<zCWaypoint*>(this)->name = wpvob->GetObjectName();
    return name;
}

zCVob* zCWaypoint::GetVob() const
{
    return wpvob;
}

void zCWaypoint::InsertWay(zCWay* way)
{
    wayList.Insert(way);
}

void zCWaypoint::RemoveWay(zCWay* way)
{
    wayList.Remove(way);
}

zCWay* zCWaypoint::HasWay(zCWaypoint* waypoint)
{
    for (zCList<zCWay>* entry = wayList.next; entry; entry = entry->next)
    {
        zCWay* way = entry->data;
        zCWaypoint* other = way->left == this ? way->right : way->left;
        if (other == waypoint)
            return way;
    }
    return NULL;
}

int zCWaypoint::GetNumberOfWays()
{
    return wayList.GetNumInList();
}

zCList<zCWay>& zCWaypoint::GetWayList()
{
    return wayList;
}

void zCWaypoint::SetWaypointVob(zCVob* vob)
{
    if (wpvob)
    {
        wpvob->Release();
        wpvob = NULL;
    }
    wpvob = vob;
    if (wpvob)
        wpvob->AddRef();
}


void zCWaypoint::CalcProperties(zCWorld* targetWorld)
{
    waterDepth = 0;
    zVEC3 down;
    down.n[0] = 0.0f;
    down.n[1] = -1000.0f;
    down.n[2] = 0.0f;
    if (targetWorld->TraceRayNearestHit(pos, down, wpvob, 0xa21))
    {
        zCPolygon* polygon = targetWorld->traceRayReport.foundPoly;
        if (polygon && polygon->material && polygon->material->matGroup == 5)
        {
            zVEC3 surface = targetWorld->traceRayReport.foundIntersection;
            if (targetWorld->TraceRayNearestHit(surface, down, wpvob, 0x821) &&
                targetWorld->traceRayReport.foundPoly)
                waterDepth = (int)(surface.n[1] - targetWorld->traceRayReport.foundIntersection.n[1] +
                    waterDepth);
        }
    }
    zVEC3 up;
    up.n[0] = 0.0f;
    up.n[1] = 1000.0f;
    up.n[2] = 0.0f;
    if (targetWorld->TraceRayNearestHit(pos, up, wpvob, 0xa21))
    {
        zCPolygon* polygon = targetWorld->traceRayReport.foundPoly;
        if (polygon && polygon->material && polygon->material->matGroup == 5)
        {
            waterDepth = (int)(targetWorld->traceRayReport.foundIntersection.n[1] - pos.n[1] +
                waterDepth);
            underWater = 1;
        }
    }
}

void zCWaypoint::CorrectHeight(zCWorld* targetWorld)
{
    zVEC3 down;
    down.n[0] = 0.0f;
    down.n[1] = -1000.0f;
    down.n[2] = 0.0f;
    if (targetWorld->TraceRayNearestHit(pos, down, wpvob, 0x821) &&
        (targetWorld->traceRayReport.foundPoly || targetWorld->traceRayReport.foundVob))
    {
        pos = targetWorld->traceRayReport.foundIntersection;
        pos.n[1] += 50.0f;
        if (wpvob)
            wpvob->SetPositionWorld(pos);
    }
}

void zCWaypoint::Draw()
{
}

void zCWaypoint::Archive(zCArchiver& archiver)
{
    archiver.WriteString(name);
    archiver.WriteInt(waterDepth);
    archiver.WriteBool(underWater);
    archiver.WriteVec3(pos);
    archiver.WriteVec3(dir);
}

void zCWaypoint::Unarchive(zCArchiver& archiver)
{
    name = archiver.ReadString();
    waterDepth = archiver.ReadInt();
    underWater = archiver.ReadBool();
    pos = archiver.ReadVec3();
    dir = archiver.ReadVec3();
}


zCWaypoint* zCWayNet::GetWaypoint(const zSTRING& name)
{
    int count = 0;
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
        ++count;
    int first = 0;
    int last = count - 1;
    while (first <= last)
    {
        int middle = (first + last) / 2;
        zCListSort<zCWaypoint>* entry = wplist.next;
        for (int i = 0; i < middle && entry; ++i)
            entry = entry->next;
        if (!entry)
            return NULL;
        zCWaypoint* waypoint = entry->data;
        if (waypoint->wpvob)
            waypoint->name = waypoint->wpvob->GetObjectName();
        if (waypoint->name == name)
            return waypoint;
        if (waypoint->name < name)
            first = middle + 1;
        else
            last = middle - 1;
    }
    return NULL;
}

zCWaypoint* zCWayNet::SearchWaypoint(zCVob* vob)
{
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
    {
        if (entry->data->wpvob == vob)
            return entry->data;
    }
    return NULL;
}

zCRoute* zCWayNet::FindRoute(const zVEC3& start, const zVEC3& target, const zCVob* vob)
{
    zCWaypoint* startWaypoint = GetNearestWaypoint(start);
    zCWaypoint* targetWaypoint = GetNearestWaypoint(target);
    if (startWaypoint && targetWaypoint && AStar(startWaypoint, targetWaypoint, vob))
        return CreateRoute(targetWaypoint);
    return NULL;
}

zCRoute* zCWayNet::FindRoute(const zVEC3& start, zCWaypoint* target, const zCVob* vob)
{
    zCWaypoint* startWaypoint = GetNearestWaypoint(start);
    if (startWaypoint && target && AStar(startWaypoint, target, vob))
        return CreateRoute(target);
    return NULL;
}

zCRoute* zCWayNet::FindRoute(const zVEC3& start, const zSTRING& target, const zCVob* vob)
{
    zCWaypoint* startWaypoint = GetNearestWaypoint(start);
    zCWaypoint* targetWaypoint = GetWaypoint(target);
    if (startWaypoint && targetWaypoint && AStar(startWaypoint, targetWaypoint, vob))
        return CreateRoute(targetWaypoint);
    return NULL;
}

zCRoute* zCWayNet::FindRoute(zCWaypoint* start, zCWaypoint* target, const zCVob* vob)
{
    if (!AStar(start, target, vob))
        return NULL;
    return CreateRoute(target);
}

zCRoute* zCWayNet::FindRoute(const zSTRING& start, const zSTRING& target, const zCVob* vob)
{
    zCWaypoint* startWaypoint = GetWaypoint(start);
    zCWaypoint* targetWaypoint = GetWaypoint(target);
    if (startWaypoint && targetWaypoint)
    {
        startWaypoint->parent = NULL;
        targetWaypoint->parent = NULL;
        if (AStar(startWaypoint, targetWaypoint, vob))
            return CreateRoute(targetWaypoint);
    }
    return NULL;
}

int zCWayNet::SortOpenList(zCWaypoint* first, zCWaypoint* second)
{
    return first->score < second->score ? -1 : 1;
}

void zCWayNet::InsertInOpen(zCWaypoint* waypoint)
{
    openList.InsertSort(waypoint);
    waypoint->curList = WAY_LIST_OPEN;
    waypoint->routeCtr = routeCtr;
}

int zCWayNet::IsInOpen(zCWaypoint* waypoint)
{
    return waypoint->routeCtr == routeCtr && waypoint->curList == WAY_LIST_OPEN;
}

void zCWayNet::InsertInClosed(zCWaypoint* waypoint)
{
    waypoint->curList = WAY_LIST_CLOSED;
    waypoint->routeCtr = routeCtr;
}

void zCWayNet::RemoveFromClosed(zCWaypoint* waypoint)
{
    waypoint->curList = WAY_LIST_NONE;
}

int zCWayNet::IsInClosed(zCWaypoint* waypoint)
{
    return waypoint->routeCtr == routeCtr && waypoint->curList == WAY_LIST_CLOSED;
}

int zCWayNet::IsInAnyList(zCWaypoint* waypoint)
{
    return waypoint->routeCtr == routeCtr && waypoint->curList != WAY_LIST_NONE;
}

int zCWayNet::EstimateCost(zCWaypoint* start, zCWaypoint* target)
{
    int dx = (int)(start->pos.n[0] - target->pos.n[0]);
    int dy = (int)(start->pos.n[1] - target->pos.n[1]);
    int dz = (int)(start->pos.n[2] - target->pos.n[2]);
    return (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy) + (dz < 0 ? -dz : dz);
}

zCRoute* zCWayNet::CreateRoute(zCWaypoint* targetWaypoint)
{
    zCRoute* route = new zCRoute;
    zCWaypoint* waypoint = targetWaypoint;
    while (waypoint)
    {
        zCWay* parent = waypoint->parent;
        if (!parent)
        {
            route->startwp = waypoint;
            route->target = waypoint;
            break;
        }
        route->wayList.Insert(parent);
        waypoint = parent->left != waypoint ? parent->left : parent->right;
    }
    return route;
}

int zCWayNet::AStar(zCWaypoint* start, zCWaypoint* target, const zCVob* vob)
{
    ++routeCtr;
    start->curCost = 0;
    start->estCost = EstimateCost(start, target);
    start->score = start->estCost;
    start->parent = NULL;
    while (openList.next)
    {
        zCListSort<zCWaypoint>* entry = openList.next;
        openList.next = entry->next;
        entry->next = NULL;
        delete entry;
    }
    InsertInOpen(start);
    while (openList.next)
    {
        zCWaypoint* current = openList.next->data;
        openList.Remove(current);
        if (current == target)
            return 1;
        for (zCList<zCWay>* entry = current->wayList.next; entry; entry = entry->next)
        {
            zCWay* way = entry->data;
            if (way->usedCtr == routeCtr)
                continue;
            zCWaypoint* next = way->left == current ? way->right : way->left;
            if (vob && (!next->CanBeUsed(vob) || !way->CanBeUsed(vob)))
                continue;
            way->usedCtr = routeCtr;
            int cost = current->curCost + way->cost;
            if (!IsInAnyList(next) || next->curCost > cost)
            {
                next->parent = way;
                next->curCost = cost;
                next->estCost = EstimateCost(next, target);
                next->score = next->estCost + cost;
                if (IsInClosed(next))
                    RemoveFromClosed(next);
                if (!IsInOpen(next))
                    InsertInOpen(next);
            }
        }
        InsertInClosed(current);
    }
    return 0;
}


void zCWayNet::GetWPList(zCList<zVEC3>& list, zVEC3& position, float minDistance,
    float maxDistance)
{
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
    {
        zCWaypoint* waypoint = entry->data;
        float dx = waypoint->pos.n[0] - position.n[0];
        float dy = waypoint->pos.n[1] - position.n[1];
        float dz = waypoint->pos.n[2] - position.n[2];
        if (dx < 0.0f)
            dx = -dx;
        if (dy < 0.0f)
            dy = -dy;
        if (dz < 0.0f)
            dz = -dz;
        float largest = dx;
        if (dy > largest)
            largest = dy;
        if (dz > largest)
            largest = dz;
        float distance = largest - 0.0625f * largest + (dx + dy + dz - largest) * 0.375f;
        if (distance >= minDistance && distance <= maxDistance)
        {
            zVEC3* point = new zVEC3;
            *point = waypoint->pos;
            list.Insert(point);
        }
    }
}

void zCWayNet::CreateWPVobList(zCList<zCVobWaypoint>& list, zCTree<zCVob>* tree)
{
    for (zCTree<zCVob>* entry = tree; entry; entry = entry->next)
    {
        if (entry->data->type == 7)
            list.Insert((zCVobWaypoint*)entry->data);
        CreateWPVobList(list, entry->firstChild);
    }
}

void zCWayNet::RemoveUnusedWPVobs()
{
    if (!world)
        return;
    zCList<zCVobWaypoint> vobs;
    vobs.data = NULL;
    vobs.next = NULL;
    CreateWPVobList(vobs, world->globalVobTree.firstChild);
    for (zCList<zCVobWaypoint>* entry = vobs.next; entry; entry = entry->next)
    {
        if (!SearchWaypoint(entry->data))
            entry->data->Release();
    }
    vobs.DeleteList();
}

void zCWayNet::ArchiveOldFormat(zCArchiver& archiver)
{
    CheckConsistency(1);
    int isolated = 0;
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
    {
        if (!entry->data->GetNumberOfWays())
            ++isolated;
    }
    archiver.WriteInt(isolated);
    int index = 0;
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
    {
        zCWaypoint* waypoint = entry->data;
        if (!waypoint->GetNumberOfWays())
        {
            if (waypoint->wpvob)
                waypoint->name = waypoint->wpvob->GetObjectName();
            zSTRING key = zSTRING("waypoint") + zSTRING(index++);
            archiver.WriteString(key.ToChar(), waypoint->name);
        }
    }
    archiver.WriteInt(waylist.GetNumInList());
    index = 0;
    for (zCList<zCWay>* entry = waylist.next; entry; entry = entry->next)
    {
        zCWay* way = entry->data;
        if (way->left->wpvob)
            way->left->name = way->left->wpvob->GetObjectName();
        if (way->right->wpvob)
            way->right->name = way->right->wpvob->GetObjectName();
        zSTRING key = zSTRING("way") + zSTRING(index++);
        archiver.WriteString(key.ToChar(), way->left->name + "/" + way->right->name);
    }
}

void zCWayNet::Archive(zCArchiver& archiver)
{
    if (!archiver.InProperties())
        CheckConsistency(1);
    archiver.WriteInt(1);
    int isolated = 0;
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
    {
        if (!entry->data->GetNumberOfWays())
            ++isolated;
    }
    archiver.WriteInt(isolated);
    int index = 0;
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
    {
        zCWaypoint* waypoint = entry->data;
        if (!waypoint->GetNumberOfWays())
        {
            zSTRING key = zSTRING("waypoint") + zSTRING(index++);
            archiver.WriteObject(key.ToChar(), waypoint);
        }
    }
    archiver.WriteInt(waylist.GetNumInList());
    index = 0;
    for (zCList<zCWay>* entry = waylist.next; entry; entry = entry->next)
    {
        zSTRING leftKey = zSTRING("wayl") + zSTRING(index);
        zSTRING rightKey = zSTRING("wayr") + zSTRING(index++);
        archiver.WriteObject(leftKey.ToChar(), entry->data->left);
        archiver.WriteObject(rightKey.ToChar(), entry->data->right);
    }
}

void zCWayNet::UnarchiveOldFormat(zCArchiver& archiver)
{
    int waypointCount = archiver.ReadInt();
    for (int i = 0; i < waypointCount; ++i)
    {
        zSTRING key = zSTRING("waypoint") + zSTRING(i);
        zSTRING waypointName = archiver.ReadString(key.ToChar());
        waypointName.Delete("[", zSTR_FROM);
        zCWaypoint* waypoint = GetWaypoint(waypointName);
        if (!waypoint && world)
        {
            zCVob* vob = world->SearchVobByName(waypointName);
            if (vob)
            {
                waypoint = zfactory->CreateWaypoint();
                waypoint->Init(vob);
                InsertWaypoint(waypoint);
            }
        }
        if (!waypoint)
            zerr.Warning("U: WAY: Waypoint not found : " + waypointName);
    }
    int wayCount = archiver.ReadInt();
    for (int i = 0; i < wayCount; ++i)
    {
        zSTRING key = zSTRING("way") + zSTRING(i);
        zSTRING packedNames = archiver.ReadString(key.ToChar());
        zSTRING names[2];
        names[0] = packedNames.Copied("/", zSTR_BEFORE);
        names[1] = packedNames.Copied("/", zSTR_AFTER);
        zCWaypoint* endpoints[2];
        for (int side = 0; side < 2; ++side)
        {
            endpoints[side] = GetWaypoint(names[side]);
            if (!endpoints[side] && world)
            {
                zCVob* vob = world->SearchVobByName(names[side]);
                if (vob)
                {
                    endpoints[side] = zfactory->CreateWaypoint();
                    endpoints[side]->Init(vob);
                    InsertWaypoint(endpoints[side]);
                }
            }
            if (!endpoints[side])
                zerr.Warning("U:WAY: Waypoint not found : " + names[side]);
        }
        if (endpoints[0] && endpoints[1])
        {
            zCWay* way = zfactory->CreateWay();
            way->Init(endpoints[0], endpoints[1]);
            waylist.Insert(way);
        }
    }
    ClearVobDependencies();
}

void zCWayNet::Unarchive(zCArchiver& archiver)
{
    zSTRING chunkName;
    unsigned short version = 0;
    archiver.ReadChunkStart(chunkName, version);
    if (!version)
    {
        UnarchiveOldFormat(archiver);
        return;
    }
    int waypointCount = archiver.ReadInt();
    for (int i = 0; i < waypointCount; ++i)
    {
        zSTRING key = zSTRING("waypoint") + zSTRING(i);
        zCWaypoint* waypoint = (zCWaypoint*)archiver.ReadObject(key.ToChar(), NULL);
        if (!HasWaypoint(waypoint))
            InsertWaypoint(waypoint);
    }
    int wayCount = archiver.ReadInt();
    for (int i = 0; i < wayCount; ++i)
    {
        zSTRING leftKey = zSTRING("wayl") + zSTRING(i);
        zSTRING rightKey = zSTRING("wayr") + zSTRING(i);
        zCWaypoint* left = (zCWaypoint*)archiver.ReadObject(leftKey.ToChar(), NULL);
        zCWaypoint* right = (zCWaypoint*)archiver.ReadObject(rightKey.ToChar(), NULL);
        InsertWaypoint(left);
        InsertWaypoint(right);
        zCWay* way = zfactory->CreateWay();
        way->Init(left, right);
        waylist.Insert(way);
    }
}

void zCWayNet::CalcProperties(zCWorld* targetWorld)
{
    for (zCListSort<zCWaypoint>* entry = wplist.next; entry; entry = entry->next)
        entry->data->CalcProperties(targetWorld);
    for (zCList<zCWay>* entry = waylist.next; entry; entry = entry->next)
        entry->data->CalcProperties(targetWorld);
}


zCRoute::zCRoute()
    : startwp(NULL), target(NULL), way(NULL), waynode(NULL)
{
    wayList.data = NULL;
    wayList.next = NULL;
}

zCRoute::~zCRoute()
{
    target = NULL;
    way = NULL;
    waynode = NULL;
    startwp = NULL;
}

void zCRoute::SetStart(zCWaypoint* waypoint)
{
    target = waypoint;
    startwp = waypoint;
}

zCWaypoint* zCRoute::GetNextWP()
{
    if (waynode)
        waynode = waynode->next;
    else if (wayList.next)
        waynode = wayList.next;
    else
    {
        target = startwp;
        startwp = NULL;
        return target;
    }
    if (!waynode)
        return NULL;
    way = waynode->data;
    target = way->GetGoalWaypoint(target);
    return target;
}

zCWaypoint* zCRoute::GetTargetWP()
{
    return target;
}

zCWay* zCRoute::GetCurrentWay()
{
    return way;
}

zSTRING zCRoute::GetDesc()
{
    zSTRING description;
    zCWaypoint* current = startwp;
    if (!current)
        return description;
    description = current->GetName();
    for (zCList<zCWay>* entry = wayList.next; entry; entry = entry->next)
    {
        way = entry->data;
        current = way->GetGoalWaypoint(current);
        description += " -> ";
        description.Append(current->GetName());
    }
    return description;
}

zSTRING zCRoute::GetRemainingDesc()
{
    zSTRING description;
    zCWaypoint* current = target;
    if (!current)
        return description;
    description = current->GetName();
    zCList<zCWay>* entry = wayList.next;
    for (; entry; entry = entry->next)
    {
        way = entry->data;
        current = way->GetGoalWaypoint(current);
        description += " -> ";
        description.Append(current->GetName());
    }
    return description;
}

int zCRoute::GetNumberOfWaypoints()
{
    return wayList.GetNumInList() + 1;
}

int zCRoute::IsEmpty()
{
    return wayList.GetNumInList() <= 0;
}

int zCRoute::IsLastTarget()
{
    return !waynode || !waynode->next;
}

int zCRoute::GetInterpolatedPosition(float time, int speed, zVEC3& position)
{
    if (IsEmpty())
    {
        if (!startwp)
            return 0;
        position = startwp->pos;
        return 1;
    }
    float remaining = time * 100000.0f * (1.0f / 60.0f) * speed;
    zCWaypoint* current = startwp;
    for (zCList<zCWay>* entry = wayList.next; entry; entry = entry->next)
    {
        way = entry->data;
        float length = way->GetLength();
        remaining -= length;
        if (remaining <= 0.0f)
        {
            zCWaypoint* next = way->GetGoalWaypoint(current);
            float fraction = (length + remaining) / length;
            position.n[0] = current->pos.n[0] + (next->pos.n[0] - current->pos.n[0]) * fraction;
            position.n[1] = current->pos.n[1] + (next->pos.n[1] - current->pos.n[1]) * fraction;
            position.n[2] = current->pos.n[2] + (next->pos.n[2] - current->pos.n[2]) * fraction;
            return 1;
        }
        current = way->GetGoalWaypoint(current);
    }
    if (!current)
        return 0;
    position = current->pos;
    return 1;
}


extern zTIMER ztimer;

static int waypointState = -1;
static int waynetState = -1;
static int waynetRuntimeState = -1;
static zCClassDef zCVobSpotClassDef("zCVobSpot", "zCVobWaypoint", zCVobSpot::_CreateNewInstance,
    0, 0x128, 0, "");
zCClassDef* zCVobSpot::classDef = &zCVobSpotClassDef;
static zCClassDef zCVobStartpointClassDef("zCVobStartpoint", "zCVobWaypoint",
    zCVobStartpoint::_CreateNewInstance, 0, 0x120, 0, "");
zCClassDef* zCVobStartpoint::classDef = &zCVobStartpointClassDef;

zCVobSpot::zCVobSpot()
    : timerEnd(0.0f), inUseVob(NULL)
{
}

zCVobSpot::~zCVobSpot()
{
    inUseVob = NULL;
}

zCClassDef* zCVobSpot::_GetClassDef() const
{
    return classDef;
}

int zCVobSpot::IsAvailable(zCVob* vob)
{
    if (inUseVob && inUseVob != vob && ztimer.totalTimeFloat < timerEnd)
        return 0;
    if (!homeWorld)
        return 0;
    zTBBox3D box;
    zVEC3 position = trafoObjToWorld.v[3];
    box.mins.n[0] = position.n[0] - 50.0f;
    box.mins.n[1] = position.n[1] - 100.0f;
    box.mins.n[2] = position.n[2] - 50.0f;
    box.maxs.n[0] = position.n[0] + 50.0f;
    box.maxs.n[1] = position.n[1] + 100.0f;
    box.maxs.n[2] = position.n[2] + 50.0f;
    zCArray<zCVob*> vobs;
    homeWorld->bspTree.bspRoot->CollectVobsInBBox3D(vobs, box);
    for (int i = 0; i < vobs.numInArray; ++i)
    {
        if (vobs[i] == inUseVob)
            return inUseVob == vob;
    }
    inUseVob = NULL;
    return 1;
}

void zCVobSpot::MarkAsUsed(float time, zCVob* vob)
{
    inUseVob = vob;
    timerEnd = ztimer.totalTimeFloat + time;
}

zCObject* zCVobWaypoint::_CreateNewInstance()
{
    zCVobWaypoint* vob = new zCVobWaypoint;
    zCClassDef::ObjectCreated(vob, classDef);
    return vob;
}

zCObject* zCVobSpot::_CreateNewInstance()
{
    zCVobSpot* vob = new zCVobSpot;
    zCClassDef::ObjectCreated(vob, classDef);
    return vob;
}

void zCVobSpot::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCObject* zCVobStartpoint::_CreateNewInstance()
{
    zCVobStartpoint* vob = new zCVobStartpoint;
    zCClassDef::ObjectCreated(vob, classDef);
    return vob;
}

void zCVobStartpoint::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* zCVobStartpoint::_GetClassDef() const
{
    return classDef;
}

zCVobStartpoint::~zCVobStartpoint()
{
}

zCObject* zCWaypoint::_CreateNewInstance()
{
    zCWaypoint* waypoint = new zCWaypoint;
    zCClassDef::ObjectCreated(waypoint, classDef);
    return waypoint;
}

zCObject* zCWayNet::_CreateNewInstance()
{
    zCWayNet* wayNet = new zCWayNet;
    zCClassDef::ObjectCreated(wayNet, classDef);
    return wayNet;
}
