#include "include/zSndFxMss.h"
#include "include/zSndActive.h"
#include "include/zSndMss.h"
#include "include/zWaveData.h"
#include <cmath>
#include <cstdlib>

zCSndFrame::zCSndFrame()
{
    pitchOffset = 0.0f;
    pitchVariance = 0.0f;
    defVolume = 127;
    looping = 0;
    loopStartOffset = 0;
    loopEndOffset = -1;
    reverbLevel = 0.5f;
    wav = NULL;
}

zCSndFrame::~zCSndFrame()
{
    zCActiveSnd::RemoveSoundsByFrame(this);
    if (wav)
        zCWavePool::GetPool().ReleaseWave(wav);
    wav = NULL;
}

void zCSndFrame::CalcPitchOffset(float pitch)
{
    pitchOffset = pitch;
    actRate = static_cast<zTWaveInfo*>(wav->info)->sampleRate;

    if (pitch == -999999.0f || pitch == 0.0f)
        return;

    const float exponent = pitch < 0.0f ? 0.9438743f : 1.05946314f;
    actRate = int(std::pow(std::fabs(pitch), exponent) * actRate);
}

int zCSndFrame::CalcPitchVariance()
{
    if (pitchVariance == 0.0f)
        return actRate;

    const float minRate = std::pow(pitchVariance, 0.9438743f) * actRate;
    const float maxRate = std::pow(pitchVariance, 1.05946314f) * actRate;
    return int(std::rand() * 3.05185094e-05f * (maxRate - minRate) + minRate + 0.5f);
}

void zCSndFrame::SetDefaultProperties()
{
    actVolume = defVolume;
    actPan = 64;

    if (!wav->size)
        wav->CacheIn();
    if (!wav->size) {
        actRate = 0;
        return;
    }

    CalcPitchOffset(pitchOffset);
}

void zCSndFrame::CacheIn()
{
    if (!wav)
        wav = zCWavePool::GetPool().CacheIn(fileName);
    else
        zCWavePool::GetPool().CacheIn(wav);
}

void zCSndFrame::CacheOut()
{
    zCWavePool::GetPool().CacheOut(wav);
}

zCSndFX_MSS::zCSndFX_MSS()
{
    volume = 1.0f;
    pan = zSND_PAN_DEFAULT;
    rate = 44100;
    autoDelete = 0;
    startPlayTime = -1.0f;
    m_bIsFixed = 0;
    actChannel = 0;
    actFrame = 0;
    actChannelFrame = -1;
}

int zCSndFX_MSS::GetNumChannels() const
{
    return channelList.numInArray;
}

int zCSndFX_MSS::GetNumChannelFrames(int channel) const
{
    return channelList.parray[channel]->frameList.numInArray;
}

float zCSndFX_MSS::GetPan() const
{
    return pan;
}

float zCSndFX_MSS::GetVolume() const
{
    return volume;
}

int zCSndFX_MSS::GetFrequency() const
{
    return rate;
}

int zCSndFX_MSS::IsLooping() const
{
    return channelList.parray[0]->frameList.parray[0]->looping;
}

void zCSndFX_MSS::SetIsFixed(int fixed)
{
    m_bIsFixed = fixed;
}

int zCSndFX_MSS::GetIsFixed() const
{
    return m_bIsFixed;
}

zCSndFX_MSS::~zCSndFX_MSS()
{
    for (int i = 0; i < channelList.numInArray; i++) {
        zCSndChannel* channel = channelList.parray[i];
        for (int j = 0; j < channel->frameList.numInArray; j++)
            delete channel->frameList.parray[j];
        channel->frameList.DeleteList();
        delete channel;
    }

    channelList.DeleteList();
}

void zCSndFX_MSS::LoadChannel(int channel)
{
    zCSndChannel* current = channelList.parray[channel];
    if (current->loaded)
        return;

    current->loaded = 1;
    current->actFrame = 0;
}

void zCSndFX_MSS::SetFrequency(int frequency)
{
    for (int i = 0; i < channelList.numInArray; i++) {
        for (int j = 0; j < channelList.parray[i]->frameList.numInArray; j++)
            channelList.parray[i]->frameList.parray[j]->actRate = frequency == -1 ? 44100 : frequency;
    }

    rate = frequency;
}

void zCSndFX_MSS::SetPan(float value)
{
    int newPan = value == zSND_PAN_DEFAULT ? 64 : int((value + 1.0f) * 63.5f);

    for (int i = 0; i < channelList.numInArray; i++) {
        for (int j = 0; j < channelList.parray[i]->frameList.numInArray; j++)
            channelList.parray[i]->frameList.parray[j]->actPan = newPan;
    }

    pan = value;
}

void zCSndFX_MSS::SetVolume(float value)
{
    float weight = value == -1.0f ? 1.0f : value;

    for (int i = 0; i < channelList.numInArray; i++) {
        for (int j = 0; j < channelList.parray[i]->frameList.numInArray; j++)
            channelList.parray[i]->frameList.parray[j]->actVolume =
                int(channelList.parray[i]->frameList.parray[j]->defVolume * weight);
    }

    volume = value;
}

void zCSndFX_MSS::SetDefaultProperties()
{
    for (int i = 0; i < channelList.numInArray; i++) {
        for (int j = 0; j < channelList.parray[i]->frameList.numInArray; j++) {
            zCSndFrame* frame = channelList.parray[i]->frameList.parray[j];
            frame->actVolume = frame->defVolume;
            frame->actRate = 44100;
            frame->actPan = 64;
        }
    }
}

int zCSndFX_MSS::LoadResourceData()
{
    for (int i = 0; i < channelList.numInArray; i++) {
        for (int j = 0; j < channelList.parray[i]->frameList.numInArray; j++)
            channelList.parray[i]->frameList.parray[j]->CacheIn();
    }

    return 1;
}

int zCSndFX_MSS::ReleaseResourceData()
{
    for (int i = 0; i < channelList.numInArray; i++) {
        for (int j = 0; j < channelList.parray[i]->frameList.numInArray; j++) {
            zCWaveData* wave = channelList.parray[i]->frameList.parray[j]->wav;
            if (wave)
                wave->CacheOut();
        }
    }

    return 1;
}

int zCSndFX_MSS::IsActive()
{
    for (int i = 0; i < channelList.numInArray; i++) {
        for (int j = 0; j < channelList.parray[i]->frameList.numInArray; j++) {
            zCSndFrame* frame = channelList.parray[i]->frameList.parray[j];
            for (int k = 0; k < zCActiveSnd::activeSndList.numInArray; k++) {
                if (zCActiveSnd::activeSndList.array[k]->active &&
                        zCActiveSnd::activeSndList.array[k]->sourceFrm == frame)
                    return 1;
            }
        }
    }

    return 0;
}

float zCSndFX_MSS::GetPlayingTimeMSEC() const
{
    zCWaveData* wave = channelList.parray[actChannel]->frameList.parray[actFrame]->wav;
    return wave ? wave->msLength : 0.0f;
}

unsigned long zCSndFX_MSS::GetResSizeBytes()
{
    unsigned long result = 0;

    for (int i = 0; i < channelList.numInArray; i++) {
        for (int j = 0; j < channelList.parray[i]->frameList.numInArray; j++) {
            zCWaveData* wave = channelList.parray[i]->frameList.parray[j]->wav;
            if (wave) {
                wave->CacheIn();
                result += wave->size;
            }
        }
    }

    return result;
}

void zCSndFX_MSS::CheckCanBeCachedOut()
{
    if (!(cacheState & 4))
        return;

    for (int i = 0; i < zCActiveSnd::activeSndList.numInArray; i++) {
        if (zCActiveSnd::activeSndList.array[i]->sourceSnd == this)
            return;
    }

    cacheState &= ~4;
}

void zCSndFX_MSS::NewFrame()
{
    for (int i = 0; i < zCSoundFX::classDef->objectList.numInArray; i++) {
        zCSndFX_MSS* sound = dynamic_cast<zCSndFX_MSS*>(zCSoundFX::classDef->objectList.parray[i]);
        if (sound)
            sound->CheckCanBeCachedOut();
    }
}

void zCSndFX_MSS::SetLooping(int looping)
{
    for (int i = 0; i < channelList.numInArray; i++) {
        for (int j = 0; j < channelList.parray[i]->frameList.numInArray; j++)
            channelList.parray[i]->frameList.parray[j]->looping = looping;
    }
}

void zCSndFX_MSS::GetChannelFrame(int channel, int frame) const
{
}

void zCSndFX_MSS::SetChannelFrame(int channel, int frame)
{
    actChannelFrame = frame;
    actChannel = channel;
    int count = channelList.parray[channel]->frameList.numInArray;

    if (count <= 1)
        actFrame = 0;
    else if (frame == zSND_CHANNEL_FRAME_RANDOM)
        actFrame = rand() % count;
    else
        actFrame = frame;
    channelList.parray[channel]->actFrame = actFrame;
}

void zCSndFX_MSS::CacheInActFrame()
{
    zCSndFrame* frame = channelList.parray[actChannel]->frameList.parray[actFrame];
    if (!frame->wav)
        frame->wav = new zCWaveData();
    frame->wav->CacheIn();
}

void zCSndFX_MSS::CacheOutActFrame()
{
    zCWaveData* wave = channelList.parray[actChannel]->frameList.parray[actFrame]->wav;
    if (wave)
        wave->CacheOut();
}

void zCSndFX_MSS::CacheInActChannel()
{
    for (int i = 0; i < channelList.parray[actChannel]->frameList.numInArray; i++) {
        actFrame = i;
        CacheInActFrame();
    }
}

void zCSndFX_MSS::CacheOutActChannel()
{
    for (int i = 0; i < channelList.parray[actChannel]->frameList.numInArray; i++) {
        actFrame = i;
        CacheOutActFrame();
    }
}

void zCSndFX_MSS::ChooseVariance()
{
    zCSndChannel* channel = channelList.parray[actChannel];

    if (channel->frameList.numInArray <= 1)
        actFrame = 0;
    else if (actChannelFrame == zSND_CHANNEL_FRAME_RANDOM)
        actFrame = rand() % channel->frameList.numInArray;
    else
        actFrame = actChannelFrame;
    channel->actFrame = actFrame;
}
