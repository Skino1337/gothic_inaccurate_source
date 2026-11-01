#include "include/zVob.h"
#include "include/zSky.h"
#include <cmath>

zCZoneZFog::zCZoneZFog()
    : fogRangeCenter(3000.0f), innerRangePerc(0.7f), bFadeOutSky(0), bOverrideColor(0)
{
    fogColor.b = 120;
    fogColor.g = 120;
    fogColor.r = 120;
    fogColor.a = 255;
}

zCZoneZFog::~zCZoneZFog()
{
    if (world)
        world->RemoveVobFromLists(this);
    world = 0;
}

float zCZoneZFog::GetActiveRange(float farZ)
{
    float weight = GetCamPosWeight();
    if (weight > innerRangePerc)
        weight = (weight - innerRangePerc) / (1.0f - innerRangePerc);
    else
        weight = 0.0f;
    return fogRangeCenter + weight * (farZ - fogRangeCenter);
}

float zCZoneZFog::GetSkyFadeWeight()
{
    float weight = GetCamPosWeight();
    if (weight > innerRangePerc)
        return (weight - innerRangePerc) / (1.0f - innerRangePerc);
    return 0.0f;
}

zCClassDef* zCZoneZFog::GetDefaultZoneClass() const
{
    return &zCZoneZFogDefault::classDef;
}

void zCZoneZFog::ProcessZoneList(const zCArraySort<zCZone*>& active,
    const zCArraySort<zCZone*>& last, zCWorld* targetWorld)
{
    if (!active.numInArray && !last.numInArray)
        return;
    zCSkyControler* sky = targetWorld->GetActiveSkyControler();
    zCZone* defaultZone = targetWorld->SearchZoneDefaultByClass(&zCZoneZFogDefault::classDef);
    float farZ = defaultZone ? static_cast<zCZoneZFog*>(defaultZone)->fogRangeCenter : sky->GetFarZ();
    float farZSum = 0.0f;
    float colorSum[3] = {};
    float colorWeight = 0.0f;
    float skyFade = 0.0f;
    for (int i = 0; i < active.numInArray; ++i) {
        zCZoneZFog* fog = static_cast<zCZoneZFog*>(active.parray[i]);
        float weight = fog->GetCamPosWeight();
        if (weight > fog->innerRangePerc)
            weight = (weight - fog->innerRangePerc) / (1.0f - fog->innerRangePerc);
        else
            weight = 0.0f;
        farZSum += fog->fogRangeCenter + weight * (farZ - fog->fogRangeCenter);
        if (fog->bOverrideColor) {
            colorSum[0] += fog->fogColor.b * weight;
            colorSum[1] += fog->fogColor.g * weight;
            colorSum[2] += fog->fogColor.r * weight;
            colorWeight += weight;
        }
        if (fog->bFadeOutSky)
            skyFade += weight;
    }
    sky->SetFarZ(active.numInArray ? farZSum / active.numInArray : farZ);
    if (colorWeight > 0.0f) {
        zCOLOR color;
        color.b = static_cast<unsigned char>(colorSum[0] / colorWeight);
        color.g = static_cast<unsigned char>(colorSum[1] / colorWeight);
        color.r = static_cast<unsigned char>(colorSum[2] / colorWeight);
        color.a = 255;
        sky->SetOverrideColor(color);
        sky->SetOverrideColorFlag(1);
    } else
        sky->SetOverrideColorFlag(0);
    if (skyFade > 1.0f)
        skyFade = 1.0f;
    sky->SetGlobalSkyScale(1.0f - skyFade * 0.5f);
}

float zSinusEase(float value)
{
    return (std::sin(value * 3.14159274f - 1.57079637f) + 1.0f) * 0.5f;
}

void zCZoneZFog::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteGroupBegin("ZoneFog");
    archiver.WriteFloat(fogRangeCenter);
    archiver.WriteFloat(innerRangePerc);
    archiver.WriteColor(reinterpret_cast<unsigned long&>(fogColor));
    archiver.WriteBool(bFadeOutSky);
    archiver.WriteBool(bOverrideColor);
    archiver.WriteGroupEnd();
}

void zCZoneZFog::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    SetZoneVobFlags();
    fogRangeCenter = archiver.ReadFloat();
    innerRangePerc = archiver.ReadFloat();
    if (innerRangePerc < 0.0f)
        innerRangePerc = 0.0f;
    else if (innerRangePerc > 1.0f)
        innerRangePerc = 1.0f;
    archiver.ReadColor(reinterpret_cast<unsigned long&>(fogColor));
    bFadeOutSky = archiver.ReadBool();
    bOverrideColor = archiver.ReadBool();
}

void zCZoneZFogDefault::ThisVobAddedToWorld(zCWorld* targetWorld)
{
    zCZone::ThisVobAddedToWorld(targetWorld);
    targetWorld->GetActiveSkyControler()->SetFarZ(fogRangeCenter);
}

void zCZoneZFogDefault::Unarchive(zCArchiver& archiver)
{
    zCZoneZFog::Unarchive(archiver);
    if (world)
        world->GetActiveSkyControler()->SetFarZ(fogRangeCenter);
}
