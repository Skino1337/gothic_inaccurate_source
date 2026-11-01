#ifndef ZSOUNDMAN_H
#define ZSOUNDMAN_H

#include "zSound.h"
#include "zParticle.h"

class zCCollisionReport;

class zCSoundManager {
public:

    enum zTSndManMedium {
        zSND_MAN_MEDIUM_MODEL,
        zSND_MAN_MEDIUM_ITEM,
        zSND_MAN_MEDIUM_OBJECT,
        zSND_MAN_MEDIUM_LEVEL
    };

    struct zTScriptSoundData {
        zTScriptSoundData();
        ~zTScriptSoundData();

        zSTRING file;
        int     pitchOff;
        int     pitchVar;
        int     vol;
        int     loop;
        float   reverbLevel;
        zSTRING pfxName;
    };

    struct zCSoundEventData {
        zCSoundEventData();
        ~zCSoundEventData();

        void GetData(const zSTRING& prefix, zCSoundFX*& sound, zCParticleEmitter*& pfxEmitter);
        void GetData(zCSoundFX*& sound, zCParticleEmitter*& pfxEmitter);

        zSTRING           name;
        zCSoundFX*        sound;
        zCParticleEmitter* pfxEmitter;
    };

    class zCMediumType {
    public:

        zCMediumType();
        ~zCMediumType();

        int     nameInt;
        zSTRING contents;
    };

    class zCTableEntry {
    public:

        zCTableEntry();
        ~zCTableEntry();

        zCSoundEventData              defaultSoundEventData;
        zCArraySort<zCSoundEventData> collSndSubTable;
    };

    zCSoundManager();
    ~zCSoundManager();

    void    DebugMessage(const zSTRING& message);
    zSTRING SearchMediumTypeIDList(int name, int type);
    int     GetMediumIndex(char medium);
    int     GetCollSndTableIndex(const zSTRING& name);
    void    StartHitSound(zCVob* vob, zTSndManMedium firstMedium, zTSndManMedium secondMedium,
        int firstMaterial, int secondMaterial, zCCollisionReport* collisionReport);
    void    StartAttackSound(zCVob* vob, zTSndManMedium firstMedium, zTSndManMedium secondMedium,
        int firstMaterial, int secondMaterial);
    int     StartSlideSound(zCVob* vob, int material);
    void    StartDestructionSound(zCVob* vob, int material);
    void    Init();

    zCArraySort<zCMediumType>      mediumTypeIDList;
    zCTableEntry                   collSndTable[17];
    zCArraySort<zCSoundEventData>  slideSndTable;
    zCArraySort<zCSoundEventData>  destSndTable;
    int                            debugMessageEnabled;

    static zTScriptSoundData scriptSoundData;
    static zSTRING           mediumIDList;
    static zSTRING           prefixProto;
};

#endif
