#ifndef ZMUSIC_H
#define ZMUSIC_H

#include "zString.h"

const float zMUS_JINGLE_VOL_DEFAULT = -1.0f;
const float zMUS_THEME_VOL_DEFAULT = -1.0f;

enum zTMus_TransType {
    zMUS_TR_DEFAULT,
    zMUS_TR_NONE,
    zMUS_TR_GROOVE,
    zMUS_TR_FILL,
    zMUS_TR_BREAK,
    zMUS_TR_INTRO,
    zMUS_TR_END,
    zMUS_TR_ENDANDINTRO
};

enum zTMus_TransSubType {
    zMUS_TRSUB_DEFAULT,
    zMUS_TRSUB_IMMEDIATE,
    zMUS_TRSUB_BEAT,
    zMUS_TRSUB_MEASURE
};

enum zTMus_EventType {
    zMUS_EVENT_THEME_CHG = 1,
    zMUS_EVENT_BEAT = 2,
    zMUS_EVENT_MEASURE = 4,
    zMUS_EVENT_TRANSITION = 8
};

class zCMusicTheme {
public:

    zCMusicTheme();
    virtual ~zCMusicTheme();
    virtual int IsActive() const;

    zSTRING fileName;
    float vol;
    int loop;
    float reverbMix;
    float reverbTime;
    zTMus_TransType trType;
    zTMus_TransSubType trSubType;
    unsigned char dScriptEnd;
    zSTRING name;
};

class zCMusicJingle {
public:

    zCMusicJingle();
    virtual ~zCMusicJingle();
    virtual zCMusicJingle* AddRef();
    virtual int Release();
    virtual int IsPlaying() const;
    virtual void StopPlaying();
    virtual void CacheIn();
    virtual void CacheOut();

    zSTRING fileName;
    int loop;
    float vol;
    zTMus_TransSubType trSubType;
    unsigned char dScriptEnd;
    int refCtr;
    zSTRING name;
};

class zCMusicSystem {
public:

    virtual ~zCMusicSystem();
    static void DisableMusicSystem(int disabled);
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
    virtual void AddEventCallBackFunc(const zTMus_EventType& event, void(*callback)(const zTMus_EventType&));
    virtual void RemoveEventCallBackFunc(void(*callback)(const zTMus_EventType&));
    virtual void Mute();
    virtual void Stop();
    virtual void SetVolume(float volume);
    virtual float GetVolume() const;
    virtual int IsAvailable(const zSTRING& theme);

    float volume;
    float reverb;

    static int s_musicSystemDisabled;
};

class zCMusicSys_Dummy : public zCMusicSystem {
public:

    zCMusicTheme* LoadThemeByScript(const zSTRING& name);
    zCMusicTheme* LoadTheme(const zSTRING& name);
    zCMusicJingle* LoadJingleByScript(const zSTRING& name);
    zCMusicJingle* LoadJingle(const zSTRING& name);
    void DoMusicUpdate();
    virtual ~zCMusicSys_Dummy();
};

extern zCMusicSystem* zmusic;

#endif
