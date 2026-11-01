#include "include/zVob.h"
#include "include/zSound.h"

int zCZoneMusic::s_autochange;
int zCZoneMusic::s_canruleautochange;

zCZoneReverb::zCZoneReverb()
    : reverbPresetNr(0), reverbPresetWeight(1.0f), innerRangePerc(0.0f)
{
}

zCZoneReverb::~zCZoneReverb()
{
    if (world)
        world->RemoveVobFromLists(this);
    world = 0;
}

zSTRING zCZoneReverb::GetDebugDescString()
{
    zSTRING result = zCZone::GetDebugDescString();
    result.Append(zSTRING(", prs: "));
    result.Append(*zsound->GetReverbPresetName(reverbPresetNr));
    return result;
}

float zCZoneReverb::GetActiveWeight()
{
    float weight = GetCamPosWeight();
    if (weight > innerRangePerc)
        return (weight - innerRangePerc) / (1.0f - innerRangePerc) * reverbPresetWeight;
    return 0.0f;
}

zCClassDef* zCZoneReverb::GetDefaultZoneClass() const
{
    return &zCZoneReverbDefault::classDef;
}

void zCZoneReverb::ProcessZoneList(const zCArraySort<zCZone*>& active,
    const zCArraySort<zCZone*>& last, zCWorld*)
{
    if (!active.numInArray && !last.numInArray)
        return;
    int preset = 0;
    float presetWeight = 1.0f;
    zCZone* defaultZone = world->SearchZoneDefaultByClass(&zCZoneReverbDefault::classDef);
    if (defaultZone) {
        zCZoneReverb* reverb = static_cast<zCZoneReverb*>(defaultZone);
        preset = reverb->reverbPresetNr;
        presetWeight = reverb->reverbPresetWeight;
    }
    if (active.numInArray) {
        float bestScore = 10000.0f;
        for (int i = 0; i < active.numInArray; ++i) {
            zCZoneReverb* reverb = static_cast<zCZoneReverb*>(active.parray[i]);
            const float activeWeight = reverb->GetActiveWeight();
            const float score = (1.0f - activeWeight * reverb->reverbPresetWeight) *
                reverb->reverbPresetWeight;
            if (score < bestScore) {
                bestScore = score;
                preset = reverb->reverbPresetNr;
            }
        }
        presetWeight = bestScore < 1.0f ? bestScore : 1.0f;
    }
    zsound->SetGlobalReverbPreset(preset, presetWeight);
}

void zCZoneReverb::Archive(zCArchiver& archiver)
{
    zCVob::Archive(archiver);
    archiver.WriteGroupBegin("ZoneReverb");
    archiver.WriteEnum(reverbPresetNr);
    archiver.WriteFloat(reverbPresetWeight);
    archiver.WriteFloat(innerRangePerc);
    archiver.WriteGroupEnd();
}

void zCZoneReverb::Unarchive(zCArchiver& archiver)
{
    zCVob::Unarchive(archiver);
    SetZoneVobFlags();
    reverbPresetNr = archiver.ReadEnum();
    reverbPresetWeight = archiver.ReadFloat();
    innerRangePerc = archiver.ReadFloat();
    if (innerRangePerc < 0.0f)
        innerRangePerc = 0.0f;
    else if (innerRangePerc > 1.0f)
        innerRangePerc = 1.0f;
}

void zCZoneReverbDefault::ThisVobAddedToWorld(zCWorld* targetWorld)
{
    zCZone::ThisVobAddedToWorld(targetWorld);
    zsound->SetGlobalReverbPreset(reverbPresetNr, reverbPresetWeight);
}

void zCZoneReverbDefault::Unarchive(zCArchiver& archiver)
{
    zCZoneReverb::Unarchive(archiver);
    zsound->SetGlobalReverbPreset(reverbPresetNr, reverbPresetWeight);
}

int zCZoneMusic::SetAutochange(int autochange)
{
    s_autochange = autochange;
    return autochange == 1 ? s_canruleautochange : 1;
}

void zCZoneMusic::GetAutochange(int* autochange)
{
    *autochange = s_autochange;
}
