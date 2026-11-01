#include "include/oWaynet.h"
#include "include/oGame.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/zRayCache.h"

oCWaypoint::oCWaypoint()
{
}

oCWaypoint::~oCWaypoint()
{
}

zCClassDef* zCWaypoint::_GetClassDef() const
{
    return classDef;
}

int oCWaypoint::CanBeUsed(const zCVob* vob)
{
    oCNpc* npc = dynamic_cast<oCNpc*>(const_cast<zCVob*>(vob));
    if (npc && waterDepth != 0.0f)
    {
        if (!underWater || !npc->CanDive())
            return npc->CanSwim() != 0;
    }
    return 1;
}

oCWay::oCWay()
    : ladder(NULL), door(NULL)
{
}

oCWay::~oCWay()
{
    if (ladder)
    {
        ladder->Release();
        ladder = NULL;
    }
    if (door)
    {
        door->Release();
        door = NULL;
    }
}

int oCWay::CanBeUsed(const zCVob* vob)
{
    oCNpc* npc = dynamic_cast<oCNpc*>(const_cast<zCVob*>(vob));
    if (!npc)
        return 1;
    if (ladder)
        return !npc->IsDead();
    if (CanJump())
    {
        if (GetJumpDistance() > npc->GetJumpRange())
            return 0;
        if (GetChasm() > npc->GetFallDownHeight() && GetChasm() > npc->GetClimbRange())
            return 0;
    }
    else if (GetChasm() != 0.0f && GetChasm() > npc->GetFallDownHeight())
        return 0;
    return 1;
}

void oCWay::Init(zCWaypoint* leftWaypoint, zCWaypoint* rightWaypoint)
{
    zCWay::Init(leftWaypoint, rightWaypoint);
    zVEC3 leftPos = left->GetPositionWorld();
    zVEC3 rightPos = right->GetPositionWorld();
    zTBBox3D bbox;
    for (int axis = 0; axis < 3; ++axis)
    {
        bbox.mins.n[axis] = leftPos.n[axis] < rightPos.n[axis] ? leftPos.n[axis] : rightPos.n[axis];
        bbox.maxs.n[axis] = leftPos.n[axis] > rightPos.n[axis] ? leftPos.n[axis] : rightPos.n[axis];
    }
    zCArray<zCVob*> vobs;
    ogame->GetWorld()->bspTree.bspRoot->CollectVobsInBBox3D(vobs, bbox);
    for (int i = 0; i < vobs.numInArray; ++i)
    {
        oCMobLadder* foundLadder = dynamic_cast<oCMobLadder*>(vobs[i]);
        if (foundLadder)
        {
            zVEC3 low = leftPos;
            zVEC3 high = rightPos;
            if (low.n[1] > high.n[1])
            {
                zVEC3 swap = low;
                low = high;
                high = swap;
            }
            zTTraceRayReport report;
            if (!foundLadder->TraceRay(low, high - low, 4, report))
                continue;
            float middle = (foundLadder->bbox3D.mins.n[1] + foundLadder->bbox3D.maxs.n[1]) * 0.5f;
            if (low.n[1] >= middle || high.n[1] < middle)
            {
                zerr.Warning("U:WAY: Ladder highpoint error (" + left->GetName() + " -> " +
                    right->GetName() + ")");
                continue;
            }
            if (ladder)
            {
                zerr.Warning("U:WAY:Found multiple ladders on path " + left->GetName() + " -> " +
                    right->GetName());
                ladder->Release();
            }
            ladder = foundLadder;
            ladder->AddRef();
            continue;
        }
        oCMobDoor* foundDoor = dynamic_cast<oCMobDoor*>(vobs[i]);
        if (foundDoor)
        {
            zVEC3 wayDirection = rightPos - leftPos;
            wayDirection.n[1] = 0.0f;
            wayDirection.Normalize();
            zVEC3 doorDirection = foundDoor->trafoObjToWorld.GetTranslation() - leftPos;
            doorDirection.n[1] = 0.0f;
            doorDirection.Normalize();
            float alignment = wayDirection.n[0] * doorDirection.n[0]
                + wayDirection.n[2] * doorDirection.n[2];
            if (alignment < 0.9848077f)
                continue;
            if (door)
                door->Release();
            door = foundDoor;
            door->AddRef();
        }
    }
}

int oCWay::IsObjectOnWay(const zCVob* vob)
{
    return ladder == vob || door == vob;
}

oCMobLadder* oCWay::GetLadder()
{
    return ladder;
}

oCMobDoor* oCWay::GetDoor()
{
    return door;
}
