#include "include/zVob.h"

zCZoneVobFarPlane::zCZoneVobFarPlane()
    : vobFarZ(3000.0f), innerRangePerc(0.7f)
{
}

zCZoneVobFarPlane::~zCZoneVobFarPlane()
{
    if (world)
        world->RemoveVobFromLists(this);
    world = 0;
}

float zCZoneVobFarPlane::GetActiveFarZ(float farZ)
{
    float weight = GetCamPosWeight();
    if (weight > innerRangePerc)
        weight = (weight - innerRangePerc) / (1.0f - innerRangePerc);
    else
        weight = 0.0f;
    return vobFarZ + weight * (farZ - vobFarZ);
}

zCClassDef* zCZoneVobFarPlane::GetDefaultZoneClass() const
{
    return &zCZoneVobFarPlaneDefault::classDef;
}

void zCZoneVobFarPlane::ProcessZoneList(const zCArraySort<zCZone*>& active,
    const zCArraySort<zCZone*>& last, zCWorld* targetWorld)
{
    if (!active.numInArray && !last.numInArray)
        return;
    zCZone* defaultZone = targetWorld->SearchZoneDefaultByClass(&zCZoneVobFarPlaneDefault::classDef);
    const float defaultFarZ = defaultZone ? static_cast<zCZoneVobFarPlane*>(defaultZone)->vobFarZ : vobFarZ;
    float farZ = defaultFarZ;
    if (active.numInArray) {
        farZ = 0.0f;
        for (int i = 0; i < active.numInArray; ++i) {
            zCZoneVobFarPlane* zone = static_cast<zCZoneVobFarPlane*>(active.parray[i]);
            farZ += zone->GetActiveFarZ(defaultFarZ);
        }
        farZ /= active.numInArray;
    }
    if (farZ < 0.0f)
        farZ = 0.0f;
    targetWorld->vobFarClipZ = farZ;
}

void zCZoneVobFarPlane::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteGroupBegin("VobFarPlane");
    archiver.WriteFloat(vobFarZ);
    archiver.WriteFloat(innerRangePerc);
    archiver.WriteGroupEnd();
}

void zCZoneVobFarPlane::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    SetZoneVobFlags();
    vobFarZ = archiver.ReadFloat();
    innerRangePerc = archiver.ReadFloat();
    if (innerRangePerc < 0.0f)
        innerRangePerc = 0.0f;
    else if (innerRangePerc > 1.0f)
        innerRangePerc = 1.0f;
    if (vobFarZ > 20000.0f)
        vobFarZ = 20000.0f;
}

void zCZoneVobFarPlaneDefault::ThisVobAddedToWorld(zCWorld* targetWorld)
{
    zCZone::ThisVobAddedToWorld(targetWorld);
    targetWorld->vobFarClipZ = vobFarZ > 0.0f ? vobFarZ : 0.0f;
}

void zCZoneVobFarPlaneDefault::Unarchive(zCArchiver& archiver)
{
    zCZoneVobFarPlane::Unarchive(archiver);
    if (world)
        world->vobFarClipZ = vobFarZ > 0.0f ? vobFarZ : 0.0f;
}
