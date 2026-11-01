#ifndef ZSND_FX_MSS_H
#define ZSND_FX_MSS_H

#include "zSound.h"
#include "zObject.h"

class zCWaveData;

class zCSndFrame {
public:

    zCSndFrame();
    ~zCSndFrame();
    void CalcPitchOffset(float pitch);
    int  CalcPitchVariance();
    void SetDefaultProperties();
    void CacheIn();
    void CacheOut();

    zSTRING fileName;
    float pitchOffset;
    float pitchVariance;
    int defVolume;
    int looping;
    int loopStartOffset;
    int loopEndOffset;
    float reverbLevel;
    zSTRING pfxName;
    unsigned char dScriptEnd;
    zSTRING instance;
    int actVolume;
    int actRate;
    int actPan;
    zCWaveData* wav;
};

class zCSndChannel {
public:

    zCSndChannel();
    ~zCSndChannel();

    int actFrame;
    int refCtr;
    int loaded;
    zCArray<zCSndFrame*> frameList;
};

class zCSndFX_MSS : public zCSoundFX {
public:

    zCSndFX_MSS();
    virtual ~zCSndFX_MSS();
    void  LoadChannel(int channel);
    void  SetDefaultProperties();
    int   IsActive();
    void  CheckCanBeCachedOut();
    void  CacheInActFrame();
    void  CacheOutActFrame();
    void  CacheInActChannel();
    void  CacheOutActChannel();
    void  ChooseVariance();
    virtual int LoadResourceData();
    virtual int ReleaseResourceData();
    virtual unsigned long GetResSizeBytes();
    virtual void SetLooping(int looping);
    virtual void SetChannelFrame(int channel, int frame);
    virtual float GetPlayingTimeMSEC() const;
    static void NewFrame();
    virtual void SetFrequency(int frequency);
    virtual void SetPan(float pan);
    virtual void SetVolume(float volume);
    virtual int GetNumChannels() const;
    virtual int GetNumChannelFrames(int channel) const;
    virtual float GetPan() const;
    virtual float GetVolume() const;
    virtual int GetFrequency() const;
    virtual int IsLooping() const;
    virtual void SetIsFixed(int fixed);
    virtual int GetIsFixed() const;

    float volume;
    float pan;
    int rate;
    int autoDelete;
    float startPlayTime;
    int m_bIsFixed;
    int actChannel;
    int actFrame;
    int actChannelFrame;
    zCArray<zCSndChannel*> channelList;
};

#endif
