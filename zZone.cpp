#include "include/zVob.h"
#include "include/z3d.h"
#include <cmath>

zCZone::zCZone()
    : world(0), priority(0)
{
    SetZoneVobFlags();
}

zCZone::~zCZone()
{
    if (world)
        world->RemoveVobFromLists(this);
    world = 0;
}

void zCZone::ThisVobAddedToWorld(zCWorld* targetWorld)
{
    zCVob::ThisVobAddedToWorld(targetWorld);
    world = targetWorld;
}

void zCZone::ThisVobRemovedFromWorld(zCWorld* targetWorld)
{
    zCVob::ThisVobRemovedFromWorld(targetWorld);
    world = 0;
}

void zCZone::EndMovement(int flags)
{
    zCVob::EndMovement(flags);
    if (world)
        world->UpdateZone(this);
}

float zCZone::GetCamPosWeight()
{
    zCVob* cameraVob = zCCamera::activeCam->connectedVob;
    float weight[3];
    for (int axis = 0; axis < 3; ++axis) {
        const float halfSize = (bbox3D.maxs.n[axis] - bbox3D.mins.n[axis]) * 0.5f;
        if (halfSize == 0.0f)
            return 0.0f;
        const float center = (bbox3D.maxs.n[axis] + bbox3D.mins.n[axis]) * 0.5f;
        weight[axis] = std::fabs((cameraVob->trafoObjToWorld.m[axis][3] - center) / halfSize);
    }
    if (weight[0] < weight[1]) {
        const float swap = weight[0];
        weight[0] = weight[1];
        weight[1] = swap;
    }
    if (weight[0] < weight[2]) {
        const float swap = weight[0];
        weight[0] = weight[2];
        weight[2] = swap;
    }
    float result = weight[0] * 0.9375f + (weight[1] + weight[2]) * 0.375f;
    if (result > 1.0f)
        result = 1.0f;
    return result;
}

void zCZone::SetZoneVobFlags()
{
    SetCollDetStat(0);
    SetCollDetDyn(0);
    showVisual = (showVisual & ~0x20) ^ 0x20;
}

void zCZone::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    SetZoneVobFlags();
}

zSTRING zCZone::GetDebugDescString()
{
    zSTRING result;
    result.Sprintf("weight: %.2f", GetCamPosWeight());
    return result;
}

void zCZone::ProcessZone()
{
}

int zCZone::GetPriority() const
{
    return priority;
}
