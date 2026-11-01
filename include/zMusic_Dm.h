#ifndef ZMUSIC_DM_H
#define ZMUSIC_DM_H

#include "zMusic.h"
#include "zTools.h"

const int NUM_MOTIFS = 16;

class zCMusicTheme_DM : public zCMusicTheme {
public:

    virtual ~zCMusicTheme_DM();
    virtual int IsActive() const;

    void* seg;
};

class zCMusicJingle_DM : public zCMusicJingle {
};

class CMusDrvProperties {
public:

    float volume;
    int bitResolution;
    int globalReverbEnabled;
    int sampleRate;
    int numChannels;
    int reverbBufferSize;
    unsigned char dScriptEnd;
};

class zCMusicSys_DirectMusic : public zCMusicSystem {
public:

    zCMusicSys_DirectMusic();
    virtual ~zCMusicSys_DirectMusic();
    virtual void PlayThemeByScript(const zSTRING& theme, int variation, int* defaultVariation);
    virtual zCMusicTheme* LoadThemeByScript(const zSTRING& name);
    virtual zCMusicTheme* LoadTheme(const zSTRING& name);
    virtual void PlayTheme(zCMusicTheme* theme, const float& volume, const zTMus_TransType& transition,
        const zTMus_TransSubType& subType);
    virtual zCMusicTheme* GetActiveTheme();
    virtual int PlayJingleByScript(const zSTRING& name);
    virtual void StopJingleByScript(const zSTRING& name);
    virtual zCMusicJingle* LoadJingleByScript(const zSTRING& name);
    virtual zCMusicJingle* LoadJingle(const zSTRING& name);
    virtual int PlayJingle(const zCMusicJingle* jingle, const float& volume,
        const zTMus_TransSubType& subType);
    virtual void StopJingle(const int& handle);
    virtual int IsJingleActive(const int& handle);
    virtual void DoMusicUpdate();
    virtual void Mute();
    virtual void Stop();
    virtual void SetVolume(float volume);
    virtual float GetVolume() const;
    virtual int IsAvailable(const zSTRING& theme);

    CMusDrvProperties prefs;
    zCMusicTheme* actTheme;
    zCMusicTheme* defTheme;
};

#endif
