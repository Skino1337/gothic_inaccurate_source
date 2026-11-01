#include "include/zVob.h"
#include "include/zMusic.h"
#include "include/oGame.h"
#include <cstdlib>

static int musicZoneState = -1;
static float musicZoneFarDistance = 250000.0f * 0.0166666675f;
static float musicZoneNearDistance = musicZoneFarDistance * 0.0166666675f;
static float musicZoneFarPlaneDistance = 250000.0f * 24.0f;
static zCClassDef oCZoneMusicClassDef("oCZoneMusic", "zCZoneMusic", oCZoneMusic::_CreateNewInstance,
    0, 0x148, 0, "");
zCClassDef* oCZoneMusic::classDef = &oCZoneMusicClassDef;
static zCClassDef oCZoneMusicDefaultClassDef("oCZoneMusicDefault", "oCZoneMusic",
    oCZoneMusicDefault::_CreateNewInstance, 0, 0x148, 0, "");
zCClassDef* oCZoneMusicDefault::classDef = &oCZoneMusicDefaultClassDef;
long oCZoneMusic::s_lastzonechange;
int oCZoneMusic::s_zonetime;
int oCZoneMusic::s_daytime;
int oCZoneMusic::s_herostatus;
oCZoneMusic* oCZoneMusic::s_musiczone;
oCZoneMusic* oCZoneMusic::s_oldmusiczone;

oCZoneMusic::oCZoneMusic()
    : enabled(1), local_enabled(1), priority(1), ellipsoid(0), reverbLevel(-3.2190001f),
      volumeLevel(1.0f), loop(1), dayEntranceDone(0), nightEntranceDone(0)
{
}

oCZoneMusic::~oCZoneMusic()
{
}

// EXE: 0x006400F0
zCObject* oCZoneMusic::_CreateNewInstance()
{
    oCZoneMusic* zone = new oCZoneMusic;
    zCClassDef::ObjectCreated(zone, classDef);
    return zone;
}

zCClassDef* oCZoneMusic::_GetClassDef() const
{
    return classDef;
}

// EXE: 0x006403D0
zCObject* oCZoneMusicDefault::_CreateNewInstance()
{
    oCZoneMusicDefault* zone = new oCZoneMusicDefault;
    zCClassDef::ObjectCreated(zone, classDef);
    return zone;
}

zCClassDef* oCZoneMusicDefault::_GetClassDef() const
{
    return classDef;
}

void oCZoneMusic::ProcessZoneList(const zCArraySort<zCZone*>& active,
    const zCArraySort<zCZone*>& last, zCWorld* targetWorld)
{
    int timeChanged = 0;
    int statusChanged = 0;
    const int daytime = ogame->GetWorldTimer()->IsDay();
    if (s_daytime != daytime) {
        s_daytime = daytime;
        timeChanged = 1;
    }
    const oHEROSTATUS heroStatus = ogame->GetHeroStatus();
    if (s_herostatus != heroStatus) {
        s_herostatus = heroStatus;
        statusChanged = 1;
    }
    SwitchInAndOutZones(active, last);
    oCZoneMusicDefault* defaultZone = static_cast<oCZoneMusicDefault*>(
        targetWorld->SearchZoneDefaultByClass(&oCZoneMusicDefault::classDef));
    oCZoneMusicList* zones = BuildTempZoneList(defaultZone, active);
    int zoneChanged = 0;
    if (zones && s_musiczone != zones->entry) {
        s_oldmusiczone = s_musiczone;
        s_musiczone = zones->entry;
        zoneChanged = 1;
    }
    if (!zones || !zones->entry) {
        zCZoneMusic::s_canruleautochange = 0;
        if (zCZoneMusic::s_autochange)
            zmusic->Stop();
        else {
            s_oldmusiczone = 0;
            s_musiczone = 0;
        }
        ReleaseTempZoneList(zones);
        return;
    }
    zCZoneMusic::s_canruleautochange = 1;
    if (!zCZoneMusic::s_autochange) {
        s_oldmusiczone = 0;
        s_musiczone = 0;
        ReleaseTempZoneList(zones);
        return;
    }
    if (timeChanged || statusChanged || zoneChanged) {
        zSTRING theme;
        GetNewTheme(&theme);
        PlayThemeVariation(theme, s_herostatus);
    }
    ReleaseTempZoneList(zones);
}

zCClassDef* oCZoneMusic::GetDefaultZoneClass() const
{
    return &oCZoneMusicDefault::classDef;
}

zSTRING oCZoneMusic::GetDebugDescString()
{
    return zCZone::GetDebugDescString();
}

void oCZoneMusic::OnTrigger(zCVob*)
{
    local_enabled = 1;
}

void oCZoneMusic::OnUntrigger(zCVob*)
{
    local_enabled = 0;
}

void oCZoneMusic::SetEnabled(int value)
{
    enabled = value;
}

int oCZoneMusic::IsEnabled() const
{
    return enabled;
}

void oCZoneMusic::SetPriority(int value)
{
    priority = value;
}

int oCZoneMusic::GetPriority() const
{
    return priority;
}

void oCZoneMusic::SetEllipsoid(int value)
{
    ellipsoid = value;
}

int oCZoneMusic::IsEllipsoid() const
{
    return ellipsoid;
}

void oCZoneMusic::SetReverb(float value)
{
    reverbLevel = value;
}

float oCZoneMusic::GetReverb() const
{
    return reverbLevel;
}

void oCZoneMusic::SetVolume(float value)
{
    volumeLevel = value;
}

float oCZoneMusic::GetVolume() const
{
    return volumeLevel;
}

void oCZoneMusic::SetLoop(int value)
{
    loop = value;
}

int oCZoneMusic::IsLoop() const
{
    return loop;
}

void oCZoneMusic::SetDayEntranceDone(int value)
{
    dayEntranceDone = value;
}

int oCZoneMusic::IsDayEntranceDone() const
{
    return dayEntranceDone;
}

void oCZoneMusic::SetNightEntranceDone(int value)
{
    nightEntranceDone = value;
}

int oCZoneMusic::IsNightEntranceDone() const
{
    return nightEntranceDone;
}

void oCZoneMusic::SetZonetime(int time)
{
    s_zonetime = time;
}

void oCZoneMusic::GetZonetime(int* time)
{
    *time = s_zonetime;
}

void oCZoneMusic::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteGroupBegin("MusicZone");
    archiver.WriteBool(enabled);
    archiver.WriteInt(priority);
    archiver.WriteBool(ellipsoid);
    archiver.WriteFloat(reverbLevel);
    archiver.WriteFloat(volumeLevel);
    archiver.WriteBool(loop);
    archiver.WriteGroupEnd();
    if (archiver.InSaveGame()) {
        archiver.WriteBool(local_enabled);
        archiver.WriteBool(dayEntranceDone);
        archiver.WriteBool(nightEntranceDone);
    }
}

void oCZoneMusic::Unarchive(zCArchiver& archiver)
{
    zCZone::Unarchive(archiver);
    enabled = archiver.ReadBool();
    priority = archiver.ReadInt();
    ellipsoid = archiver.ReadBool();
    reverbLevel = archiver.ReadFloat();
    volumeLevel = archiver.ReadFloat();
    loop = archiver.ReadBool();
    if (archiver.InSaveGame()) {
        local_enabled = archiver.ReadBool();
        dayEntranceDone = archiver.ReadBool();
        nightEntranceDone = archiver.ReadBool();
    } else
        local_enabled = enabled;
}

float oCZoneMusic::GetCamPosWeightElps()
{
    return GetCamPosWeight();
}

oCZoneMusicList* oCZoneMusic::BuildTempZoneList(oCZoneMusicDefault* defaultZone,
    const zCArraySort<zCZone*>& zones)
{
    oCZoneMusicList* first = 0;
    for (int i = 0; i < zones.numInArray; ++i) {
        oCZoneMusic* zone = static_cast<oCZoneMusic*>(zones.parray[i]);
        if (!zone || !zone->local_enabled || (zone->ellipsoid && zone->GetCamPosWeightElps() >= 1.0f))
            continue;
        oCZoneMusicList* current = first;
        oCZoneMusicList* previous = 0;
        while (current && (current->entry->priority > zone->priority ||
            (current->entry->priority == zone->priority &&
            current->entry->GetCamPosWeightElps() >= zone->GetCamPosWeightElps()))) {
            previous = current;
            current = current->next;
        }
        oCZoneMusicList* item = new oCZoneMusicList;
        item->entry = zone;
        item->next = current;
        if (previous)
            previous->next = item;
        else
            first = item;
    }
    if (defaultZone) {
        oCZoneMusicList* item = new oCZoneMusicList;
        item->entry = defaultZone;
        item->next = 0;
        if (!first)
            first = item;
        else {
            oCZoneMusicList* tail = first;
            while (tail->next)
                tail = tail->next;
            tail->next = item;
        }
    }
    return first;
}

void oCZoneMusic::ReleaseTempZoneList(oCZoneMusicList* list)
{
    while (list) {
        oCZoneMusicList* next = list->next;
        delete list;
        list = next;
    }
}

void oCZoneMusic::SwitchInAndOutZones(const zCArraySort<zCZone*>& active,
    const zCArraySort<zCZone*>& last)
{
    for (int i = 0; i < active.numInArray; ++i) {
        oCZoneMusic* zone = static_cast<oCZoneMusic*>(active.parray[i]);
        if (!zone)
            continue;
        zSTRING name = zone->GetObjectName();
        if (zone->ellipsoid && zone->GetCamPosWeightElps() >= 1.0f) {
            const int position = name.SearchRev(zSTRING("_O_"));
            if (position > 0 && zone->local_enabled) {
                zone->local_enabled = 0;
                name.Overwrite(position, zSTRING("_I_"));
                oCZoneMusic* linked = GetMusicZoneByName(zone, name);
                if (linked)
                    linked->local_enabled = 1;
            }
        } else {
            const int position = name.SearchRev(zSTRING("_I_"));
            if (position > 0 && zone->local_enabled) {
                zone->local_enabled = 0;
                name.Overwrite(position, zSTRING("_O_"));
                oCZoneMusic* linked = GetMusicZoneByName(zone, name);
                if (linked)
                    linked->local_enabled = 1;
            }
        }
    }
    for (int i = 0; i < last.numInArray; ++i) {
        oCZoneMusic* zone = static_cast<oCZoneMusic*>(last.parray[i]);
        if (!zone)
            continue;
        zSTRING name = zone->GetObjectName();
        const int position = name.SearchRev(zSTRING("_O_"));
        if (position > 0 && zone->local_enabled) {
            zone->local_enabled = 0;
            name.Overwrite(position, zSTRING("_I_"));
            oCZoneMusic* linked = GetMusicZoneByName(zone, name);
            if (linked)
                linked->local_enabled = 1;
        }
    }
}

oCZoneMusic* oCZoneMusic::GetMusicZoneByName(oCZoneMusic* zone, zSTRING name)
{
    return static_cast<oCZoneMusic*>(zone->homeWorld->SearchVobByName(name));
}

void oCZoneMusic::GetNewTheme(zSTRING* theme)
{
    if (!s_musiczone) {
        *theme = zSTRING("NO_MUSIC_ZONE_ACTIVE");
        return;
    }
    *theme = s_musiczone->GetObjectName();
    theme->Delete(zSTRING("_"), zSTR_BEFORE);
    theme->Delete(zSTRING("_"), zSTR_ONLY);
    theme->Delete(zSTRING(""), zSTR_ONLY);
    theme->Delete(zSTRING("_"), zSTR_ONLY);
    theme->Append(zSTRING(s_daytime ? "_DAY" : "_NGT"));
    if (s_herostatus == 1)
        theme->Append(zSTRING("_THR"));
    else if (s_herostatus == 2)
        theme->Append(zSTRING("_FGT"));
    else {
        int& entranceDone = s_daytime ? s_musiczone->dayEntranceDone : s_musiczone->nightEntranceDone;
        if (!entranceDone) {
            entranceDone = 1;
            zSTRING entrance = *theme;
            entrance.Append(zSTRING("_ENT"));
            if (zmusic->IsAvailable(entrance)) {
                *theme = entrance;
                return;
            }
        }
        theme->Append(zSTRING("_STD"));
    }
}

int oCZoneMusic::PlayThemeVariation(zSTRING theme, int variation)
{
    if (zmusic->IsAvailable(theme)) {
        int started = 0;
        zmusic->PlayThemeByScript(theme, variation, &started);
        if (!started)
            s_musiczone = s_oldmusiczone;
        return 1;
    }
    zSTRING randomTheme = theme;
    randomTheme.Append(zSTRING("_A0"));
    if (zmusic->IsAvailable(randomTheme)) {
        int count = 1;
        while (true) {
            zSTRING candidate = theme;
            candidate.Append(zSTRING("_A"));
            candidate.Append(zSTRING(count));
            if (!zmusic->IsAvailable(candidate))
                break;
            ++count;
        }
        randomTheme = theme;
        randomTheme.Append(zSTRING("_A"));
        randomTheme.Append(zSTRING(std::rand() % count));
        int started = 0;
        zmusic->PlayThemeByScript(randomTheme, variation, &started);
        if (!started)
            s_musiczone = 0;
        return 1;
    }
    return 0;
}

int oCZoneMusic::IsDaytime()
{
    return ogame->GetWorldTimer()->IsDay();
}

int oCZoneMusic::GetHerostatus()
{
    return ogame->GetHeroStatus();
}

oCZoneMusicDefault::oCZoneMusicDefault()
{
    priority = 0;
}

oCZoneMusicDefault::~oCZoneMusicDefault()
{
}
