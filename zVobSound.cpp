#include "include/zVob.h"
#include "include/zSound.h"
#include "include/z3d.h"
#include "include/zSky.h"
#include "include/zTools.h"
#include <cmath>
#include <cstdlib>

zCVobSound::zCVobSound()
    : soundRadius(1500.0f), soundMode(2), soundStartOn(1), soundRandDelay(5.0f),
      soundRandDelayVar(2.0f), soundVolume(100.0f), soundConeAngle(100.0f), sfx(0), sfxHandle(0),
      soundRandTimer(0.0f), obstruction0(-1.0f), obstruction1(-1.0f), obstructionFrameTime(0.0f)
{
    if (!(soundStartOn & 0x10)) {
        zTBBox3D box;
        for (int axis = 0; axis < 3; ++axis) {
            box.mins.n[axis] = -soundRadius;
            box.maxs.n[axis] = soundRadius;
        }
        SetBBox3DLocal(box);
    }
}

zCClassDef* zCVobSound::_GetClassDef() const
{
    return classDef;
}

zCVobSound::~zCVobSound()
{
    StopSound();
    if (sfx)
        sfx->Release();
    sfx = 0;
    if (world)
        world->RemoveVobFromLists(this);
    world = 0;
}

zSTRING zCVobSound::GetDebugDescString()
{
    zSTRING result;
    if (soundStartOn & 0x10) {
        result.Sprintf("vol: %.2f, ellRad: %.2f", CalcVolumeScale(), soundRadius);
        result.Append(zSTRING(" "));
    }
    if (soundStartOn & 8) {
        zSTRING obstruction;
        obstruction.Sprintf("obs: %.2f ", obstruction1);
        result.Append(obstruction);
    }
    result.Append(zCZone::GetDebugDescString());
    return result;
}

void zCVobSound::EndMovement(int flags)
{
    zCZone::EndMovement(flags);
    if (soundStartOn & 0x10) {
        const zTBBox3D box = GetBBox3DLocal();
        float largest = box.maxs.n[0] - box.mins.n[0];
        for (int axis = 1; axis < 3; ++axis) {
            const float size = box.maxs.n[axis] - box.mins.n[axis];
            if (size > largest)
                largest = size;
        }
        soundRadius = largest * 0.5f;
    }
}

void zCVobSound::SetSound(const zSTRING& name)
{
    soundName = name;
    soundStartOn &= ~2;
    zCSoundFX* newSound = LoadSound(name);
    if (sfx)
        sfx->Release();
    sfx = newSound;
}

zCSoundFX* zCVobSound::LoadSound(const zSTRING& name)
{
    if (!zsound)
        return 0;
    if (name.Search(zSTRING(".CSL")) != -1)
        return zsound->LoadSoundFXScript(name);
    return zsound->LoadSoundFX(name);
}

void zCVobSound::SetSoundVolume(float volume)
{
    soundVolume = volume;
}

void zCVobSound::SetSoundRadius(float radius)
{
    soundRadius = radius;
    if (soundStartOn & 0x10)
        return;
    zTBBox3D box;
    for (int axis = 0; axis < 3; ++axis) {
        box.mins.n[axis] = -radius;
        box.maxs.n[axis] = radius;
    }
    SetBBox3DLocal(box);
}

float zCVobSound::CalcObstruction()
{
    if (!(soundStartOn & 8) || !zCCamera::activeCam || !zCCamera::activeCam->connectedVob)
        return 0.0f;
    if (obstructionFrameTime > 0.0f) {
        const float result = obstruction1 + (obstruction0 - obstruction1) * obstructionFrameTime * 0.001f;
        obstructionFrameTime -= ztimer.frameTimeFloat;
        return result;
    }
    obstructionFrameTime = 1000.0f;
    zVEC3 start;
    zVEC3 direction;
    for (int axis = 0; axis < 3; ++axis) {
        start.n[axis] = trafoObjToWorld.m[axis][3];
        direction.n[axis] = zCCamera::activeCam->connectedVob->trafoObjToWorld.m[axis][3] - start.n[axis];
    }
    obstruction0 = obstruction1;
    obstruction1 = world->TraceRayNearestHit(start, direction, static_cast<zCVob*>(0), 2) ? 1.0f : 0.0f;
    if (obstruction0 == -1.0f)
        obstruction0 = obstruction1;
    return obstruction0;
}

float zCVobSound::CalcVolumeScale()
{
    const float volume = soundVolume * 0.01f;
    if (!(soundStartOn & 0x10) || !zCCamera::activeCam || !zCCamera::activeCam->connectedVob ||
        soundRadius <= 0.0f)
        return volume;
    zVEC3 inverseDistance;
    float distanceSum = 0.0f;
    for (int axis = 0; axis < 3; ++axis) {
        const float center = (bbox3D.maxs.n[axis] + bbox3D.mins.n[axis]) * 0.5f;
        const float delta = zCCamera::activeCam->connectedVob->trafoObjToWorld.m[axis][3] - center;
        if (delta == 0.0f)
            return 1.0f;
        const float halfSize = (bbox3D.maxs.n[axis] - bbox3D.mins.n[axis]) * 0.5f;
        inverseDistance.n[axis] = std::fabs(halfSize / delta);
        distanceSum += inverseDistance.n[axis];
    }
    if (distanceSum == 0.0f)
        return 1.0f;
    float radius = 0.0f;
    for (int axis = 0; axis < 3; ++axis) {
        const float weight = inverseDistance.n[axis] / distanceSum;
        radius += (bbox3D.maxs.n[axis] - bbox3D.mins.n[axis]) * weight * 0.5f;
    }
    return radius / soundRadius * volume;
}

void zCVobSound::StartSound(int forceCacheIn)
{
    if ((soundStartOn & 2) && zsound->IsSoundActive(sfxHandle))
        return;
    soundStartOn &= ~2;
    if (!sfx || !zsound)
        return;
    if ((sfx->cacheState & 3) != 3)
        zresMan->CacheIn(sfx, forceCacheIn ? -1.0f : 0.5f);
    else
        sfx->TouchTimeStamp();
    if ((sfx->cacheState & 3) != 3 && !forceCacheIn)
        return;
    zCSoundSystem::zTSound3DParams params;
    params.SetDefaults();
    params.obstruction = CalcObstruction();
    params.volume = CalcVolumeScale();
    params.radius = soundRadius;
    params.loopType = !soundMode;
    params.coneAngleDeg = soundConeAngle;
    params.isAmbient3D = (soundStartOn >> 2) & 1;
    sfxHandle = zsound->PlaySound3D(sfx, this, 0, &params);
    soundStartOn |= 2;
}

void zCVobSound::StopSound()
{
    if (!(soundStartOn & 2))
        return;
    if (zsound)
        zsound->StopSound(sfxHandle);
    sfxHandle = 0;
    soundStartOn &= ~2;
}

void zCVobSound::DoSoundUpdate(float volumeScale)
{
    if (soundMode == 0) {
        if (!(soundStartOn & 2))
            StartSound(0);
    } else if (soundMode == 2 && ztimer.totalTimeFloat >= soundRandTimer) {
        StartSound(1);
        soundRandTimer = ztimer.totalTimeFloat +
            ((std::rand() - 16383.5f) * 6.10370189e-05f * soundRandDelayVar + soundRandDelay) * 1000.0f;
    }
    if (!(soundStartOn & 2))
        return;
    zCSoundSystem::zTSound3DParams params;
    params.SetDefaults();
    if (zsound->GetSound3DProps(sfxHandle, params)) {
        params.obstruction = CalcObstruction();
        params.radius = soundRadius;
        params.volume = CalcVolumeScale() * volumeScale;
    }
    zsound->UpdateSound3D(sfxHandle, params);
}

void zCVobSound::OnTrigger(zCVob*, zCVob*)
{
    soundStartOn |= 0x20;
    StartSound(1);
}

void zCVobSound::OnUntrigger(zCVob*, zCVob*)
{
    soundStartOn &= ~0x20;
    StopSound();
}

void zCVobSound::OnMessage(zCEventMessage*, zCVob*)
{
}

void zCVobSound::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteGroupBegin("VobSound");
    archiver.WriteFloat(soundVolume);
    archiver.WriteEnum(soundMode);
    archiver.WriteFloat(soundRandDelay);
    archiver.WriteFloat(soundRandDelayVar);
    archiver.WriteBool(soundStartOn & 1);
    archiver.WriteBool((soundStartOn >> 2) & 1);
    archiver.WriteBool((soundStartOn >> 3) & 1);
    archiver.WriteFloat(soundConeAngle);
    archiver.WriteEnum((soundStartOn >> 4) & 1);
    archiver.WriteFloat(soundRadius);
    archiver.WriteString(soundName);
    archiver.WriteGroupEnd();
    if (archiver.InSaveGame()) {
        archiver.WriteBool((soundStartOn >> 1) & 1);
        archiver.WriteBool((soundStartOn >> 5) & 1);
    }
}

void zCVobSound::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    SetZoneVobFlags();
    StopSound();
    soundVolume = archiver.ReadFloat();
    soundMode = archiver.ReadEnum();
    soundRandDelay = archiver.ReadFloat();
    soundRandDelayVar = archiver.ReadFloat();
    soundStartOn = (soundStartOn & ~1) | (archiver.ReadBool() & 1);
    soundStartOn = (soundStartOn & ~4) | ((archiver.ReadBool() & 1) << 2);
    soundStartOn = (soundStartOn & ~8) | ((archiver.ReadBool() & 1) << 3);
    soundConeAngle = archiver.ReadFloat();
    soundStartOn = (soundStartOn & ~0x10) | ((archiver.ReadEnum() & 1) << 4);
    SetSoundRadius(archiver.ReadFloat());
    SetSound(archiver.ReadString());
    if (archiver.InSaveGame()) {
        soundStartOn = (soundStartOn & ~0x40) | ((archiver.ReadBool() & 1) << 6);
        soundStartOn = (soundStartOn & ~0x20) | ((archiver.ReadBool() & 1) << 5);
    } else {
        soundStartOn &= ~0x20;
        soundStartOn |= 0x40;
    }
}

void zCVobSound::ProcessZoneList(const zCArraySort<zCZone*>& active,
    const zCArraySort<zCZone*>& last, zCWorld*)
{
    for (int i = 0; i < active.numInArray; ++i) {
        zCVobSound* sound = static_cast<zCVobSound*>(active.parray[i]);
        if (sound->soundStartOn & 0x20) {
            if (sound->soundStartOn & 0x40) {
                sound->soundStartOn &= ~0x40;
                sound->StartSound(1);
            }
            sound->DoSoundUpdate(1.0f);
        } else
            sound->StopSound();
    }
    for (int i = 0; i < last.numInArray; ++i)
        static_cast<zCVobSound*>(last.parray[i])->StopSound();
}

void zCVobSound::ThisVobAddedToWorld(zCWorld* targetWorld)
{
    zCZone::ThisVobAddedToWorld(targetWorld);
}

void zCVobSound::ThisVobRemovedFromWorld(zCWorld* targetWorld)
{
    zCZone::ThisVobRemovedFromWorld(targetWorld);
    StopSound();
}

zCVobSoundDaytime::zCVobSoundDaytime()
    : soundStartTime(0.0f), soundEndTime(0.0f), sfx2(0), activeSection(0)
{
}

zCVobSoundDaytime::~zCVobSoundDaytime()
{
    if (sfx2)
        sfx2->Release();
    sfx2 = 0;
}

zSTRING zCVobSoundDaytime::GetDebugDescString()
{
    zSTRING result = zCVobSound::GetDebugDescString();
    zSTRING section;
    section.Sprintf(", seg: %d", activeSection);
    result.Append(section);
    return result;
}

void zCVobSoundDaytime::ActivateSection(int section)
{
    StopSound();
    activeSection = section;
    zCSoundFX* swap = sfx;
    sfx = sfx2;
    sfx2 = swap;
}

int zCVobSoundDaytime::CalcTimeFrac(float start, float end, float current, float& fraction)
{
    if (end < start) {
        if (current < start)
            current += 24.0f;
        end += 24.0f;
    }
    const float duration = end - start;
    if (duration != 0.0f) {
        fraction = (current - start) / duration;
        if (fraction >= 0.0f && fraction <= 1.0f)
            return 1;
    }
    return 0;
}

void zCVobSoundDaytime::DoSoundUpdate(float volumeScale)
{
    if (!homeWorld || !homeWorld->GetActiveSkyControler())
        return;
    float current = homeWorld->GetActiveSkyControler()->GetTime() * 24.0f + 12.0f;
    if (current >= 24.0f)
        current -= 24.0f;
    float fraction = 0.0f;
    int section = 0;
    int playing = CalcTimeFrac(soundStartTime, soundEndTime, current, fraction);
    if (!playing && soundName2.Length()) {
        section = 1;
        playing = CalcTimeFrac(soundEndTime, soundStartTime, current, fraction);
    }
    if (activeSection != section && soundName2.Length()) {
        if (!sfx2)
            sfx2 = LoadSound(soundName2);
        ActivateSection(section);
    }
    if (playing) {
        float scale = 1.0f;
        if (fraction < 0.1f)
            scale = fraction * 10.0f;
        else if (fraction >= 0.9f)
            scale = 1.0f - (fraction - 0.9f) * 10.0f;
        if (!(soundStartOn & 2))
            StartSound(0);
        zCVobSound::DoSoundUpdate(scale * volumeScale);
    } else
        StopSound();
}

void zCVobSoundDaytime::Archive(zCArchiver& archiver)
{
    zCVobSound::Archive(archiver);
    archiver.WriteGroupBegin("SoundDaytime");
    archiver.WriteFloat(soundStartTime);
    archiver.WriteFloat(soundEndTime);
    archiver.WriteString(soundName2);
    archiver.WriteGroupEnd();
}

void zCVobSoundDaytime::Unarchive(zCArchiver& archiver)
{
    zCVobSound::Unarchive(archiver);
    if (sfx2)
        sfx2->Release();
    sfx2 = 0;
    activeSection = 0;
    soundStartTime = archiver.ReadFloat();
    if (soundStartTime < 0.0f)
        soundStartTime = 0.0f;
    else if (soundStartTime > 24.0f)
        soundStartTime = 24.0f;
    soundEndTime = archiver.ReadFloat();
    if (soundEndTime < 0.0f)
        soundEndTime = 0.0f;
    else if (soundEndTime > 24.0f)
        soundEndTime = 24.0f;
    soundName2 = archiver.ReadString();
}
