#include "include/zSoundMan.h"

static int soundManagerState = -1;
static int soundScriptState = -1;
static int soundPlayerState = -1;
static int soundEventState = -1;
static int soundScriptRuntimeState = -1;
static int soundGameplayState = -1;
static int soundManagerExtraState = -1;
static int soundEventRuntimeState = -1;
static zSTRING soundEventPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING soundEventPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING soundEventPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING soundEventPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING soundEventPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING soundEventPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING soundEventPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING soundEventPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING soundEventPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING soundEventPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static zSTRING soundGameplayPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING soundGameplayPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING soundGameplayPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING soundGameplayPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING soundGameplayPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING soundGameplayPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING soundGameplayPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING soundGameplayPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING soundGameplayPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING soundGameplayPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static zSTRING soundPlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING soundPlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING soundPlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING soundPlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING soundPlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING soundPlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING soundPlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING soundPlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING soundPlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING soundPlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";
static unsigned int soundGlobalState[3];
static const unsigned int soundLookupTable[32] = {
    0x171F5020, 0x370F163E, 0x14340B14, 0x0C14360B, 0x320E1736, 0x183C0D14, 0x11193D10,
    0x3E131B3F, 0x1D40131B, 0x141E4114, 0x501B2249, 0x2F631D25, 0x2C367125, 0x782E3978,
    0x3B79303B, 0x323C7B31, 0x7E343E7C, 0x427F3541, 0x3A448037, 0x833D4782, 0x49843F48,
    0x414D843F, 0x88444E86, 0x53894752, 0x4D568B49, 0x904E588D, 0x5D8F535D, 0x57619153,
    0x9A5A6394, 0x6B9A5F69, 0x636D9C62, 0xA268709E
};
static const unsigned int soundScaleTable[14] = {
    0x07030507, 0x050A0305, 0x04060C03, 0x07040608, 0x06080406, 0x04060804, 0x08040608,
    0x06080406, 0x04060804, 0x08040608, 0x06080406, 0x05070904, 0x09050709, 0x07090507
};
static const unsigned int soundParameterTable[14] = {
    0x05020405, 0x04050204, 0x02040502, 0x05020405, 0x04050204, 0x35343002, 0xDEEEE4CF,
    0x746AFFF4, 0x03040678, 0x06030406, 0x04050304, 0x03040602, 0x07030507, 0x04060305
};
static int soundManagerRuntimeState = -1;
static zSTRING soundRuntimePlayerMobMissingKey = "PLAYER_MOB_MISSING_KEY";
static zSTRING soundRuntimePlayerMobMissingLockpick = "PLAYER_MOB_MISSING_LOCKPICK";
static zSTRING soundRuntimePlayerMobMissingKeyOrLockpick = "PLAYER_MOB_MISSING_KEY_OR_LOCKPICK";
static zSTRING soundRuntimePlayerMobNeverOpen = "PLAYER_MOB_NEVER_OPEN";
static zSTRING soundRuntimePlayerMobTooFarAway = "PLAYER_MOB_TOO_FAR_AWAY";
static zSTRING soundRuntimePlayerMobWrongSide = "PLAYER_MOB_WRONG_SIDE";
static zSTRING soundRuntimePlayerMobMissingItem = "PLAYER_MOB_MISSING_ITEM";
static zSTRING soundRuntimePlayerMobAnotherIsUsing = "PLAYER_MOB_ANOTHER_IS_USING";
static zSTRING soundRuntimePlayerPlunderIsEmpty = "PLAYER_PLUNDER_IS_EMPTY";
static zSTRING soundRuntimePlayerRangedNoAmmo = "PLAYER_RANGED_NO_AMMO";

zCSoundManager::zTScriptSoundData zCSoundManager::scriptSoundData;
zSTRING zCSoundManager::mediumIDList("MIOL");
zSTRING zCSoundManager::prefixProto("CS_XXX_");

zCSoundManager::zTScriptSoundData::zTScriptSoundData()
{
}

zCSoundManager::zTScriptSoundData::~zTScriptSoundData()
{
}

void zCSoundManager::DebugMessage(const zSTRING& message)
{
    if (debugMessageEnabled)
        zerr->Message(zSTRING("D: SoundMan: ") + message);
}

zCSoundManager::zCTableEntry::zCTableEntry()
{
}

zSTRING zCSoundManager::SearchMediumTypeIDList(int name, int type)
{
    for (int i = 0; i < mediumTypeIDList.numInArray; ++i) {
        zCMediumType& medium = mediumTypeIDList.array[i];
        if (medium.nameInt != name)
            continue;
        const int offset = type * 3;
        if (medium.contents.Length() > offset + 1)
            return zSTRING(medium.contents[offset]) + zSTRING(medium.contents[offset + 1]);
        break;
    }
    return prefixProto;
}

zCSoundManager::zCMediumType::~zCMediumType()
{
}

int zCSoundManager::GetMediumIndex(char medium)
{
    for (int i = 0; i < mediumIDList.Length(); ++i) {
        if (mediumIDList[i] == medium)
            return i;
    }
    return 0;
}

int zCSoundManager::GetCollSndTableIndex(const zSTRING& name)
{
    const int firstMedium = GetMediumIndex(name[0]);
    const int secondMedium = GetMediumIndex(name[2]);
    if (name[1] == 'H') {
        if (firstMedium == 0)
            return secondMedium;
        if (firstMedium == 1)
            return 3 + secondMedium;
        if (firstMedium == 2)
            return 5 + secondMedium;
        return 9 + secondMedium;
    }
    return (firstMedium == 0 ? 9 : 13) + secondMedium;
}

zCSoundManager::zCSoundEventData::zCSoundEventData()
    : sound(0), pfxEmitter(0)
{
}

zCSoundManager::zCSoundEventData::~zCSoundEventData()
{
    if (sound) {
        sound->Release();
        sound = 0;
    }
}

void zCSoundManager::zCSoundEventData::GetData(const zSTRING& prefix, zCSoundFX*& soundValue,
    zCParticleEmitter*& pfxEmitterValue)
{
    if (!sound) {
        sound = zsound->LoadSoundFXScript(prefix + name);
        if (sound)
            pfxEmitter = zCParticleFX::SearchParticleEmitter(scriptSoundData.pfxName);
    }
    soundValue = sound;
    pfxEmitterValue = pfxEmitter;
}

void zCSoundManager::zCSoundEventData::GetData(zCSoundFX*& soundValue,
    zCParticleEmitter*& pfxEmitterValue)
{
    if (!sound) {
        sound = zsound->LoadSoundFXScript(name);
        if (sound)
            pfxEmitter = zCParticleFX::SearchParticleEmitter(scriptSoundData.pfxName);
    }
    soundValue = sound;
    pfxEmitterValue = pfxEmitter;
}

void zCSoundManager::StartHitSound(zCVob* vob, zTSndManMedium firstMedium,
    zTSndManMedium secondMedium, int firstMaterial, int secondMaterial, zCCollisionReport*)
{
    if (!vob || !vob->homeWorld)
        return;
    if (firstMedium > secondMedium) {
        const zTSndManMedium medium = firstMedium;
        firstMedium = secondMedium;
        secondMedium = medium;
        const int material = firstMaterial;
        firstMaterial = secondMaterial;
        secondMaterial = material;
    }
    int tableOffset = 0;
    if (firstMedium == zSND_MAN_MEDIUM_ITEM)
        tableOffset = 3;
    else if (firstMedium == zSND_MAN_MEDIUM_OBJECT)
        tableOffset = 5;
    else if (firstMedium == zSND_MAN_MEDIUM_LEVEL)
        tableOffset = 9;
    zCTableEntry& table = collSndTable[tableOffset + secondMedium];
    prefixProto[3] = mediumIDList[firstMedium];
    prefixProto[4] = 'H';
    prefixProto[5] = mediumIDList[secondMedium];

    zCSoundEventData* eventData = &table.defaultSoundEventData;
    for (int i = 0; i < table.collSndSubTable.numInArray; ++i) {
        if (table.collSndSubTable.array[i].name == prefixProto) {
            eventData = &table.collSndSubTable.array[i];
            break;
        }
    }
    zCSoundFX* sound;
    zCParticleEmitter* emitter;
    eventData->GetData(prefixProto, sound, emitter);
    if (sound)
        zsound->PlaySound3D(sound, vob, 8, 0);
    if (emitter) {
        zCParticleFX* particleFx = new zCParticleFX;
        zCVob* effectVob = new zCVob;
        effectVob->SetCollDetStat(0);
        effectVob->SetCollDetDyn(0);
        effectVob->SetPositionWorld(vob->trafoObjToWorld.GetTranslation());
        effectVob->SetVisual(particleFx);
        particleFx->SetEmitter(emitter, 0);
        particleFx->CreateParticles();
        vob->homeWorld->AddVob(effectVob);
    }
    DebugMessage(zSTRING("startHitSound: ") + prefixProto);
}

static zCSoundManager::zCSoundEventData* FindSoundEvent(
    zCArraySort<zCSoundManager::zCSoundEventData>& events, const zSTRING& name)
{
    for (int i = 0; i < events.numInArray; ++i) {
        if (events.array[i].name == name)
            return &events.array[i];
    }
    return 0;
}

void zCSoundManager::StartAttackSound(zCVob* vob, zTSndManMedium firstMedium,
    zTSndManMedium secondMedium, int firstMaterial, int secondMaterial)
{
    int tableIndex;
    if (firstMedium == zSND_MAN_MEDIUM_MODEL)
        tableIndex = 9 + secondMedium;
    else if (firstMedium == zSND_MAN_MEDIUM_ITEM)
        tableIndex = 13 + secondMedium;
    else
        return;
    prefixProto[3] = mediumIDList[firstMedium];
    prefixProto[4] = 'A';
    prefixProto[5] = mediumIDList[secondMedium];
    zCTableEntry& table = collSndTable[tableIndex];
    zCSoundEventData* eventData = FindSoundEvent(table.collSndSubTable, prefixProto);
    if (!eventData)
        eventData = &table.defaultSoundEventData;
    zCSoundFX* sound;
    zCParticleEmitter* emitter;
    eventData->GetData(prefixProto, sound, emitter);
    if (sound)
        zsound->PlaySound3D(sound, vob, 8, 0);
    DebugMessage(zSTRING("startAttackSound: ") + prefixProto);
}

int zCSoundManager::StartSlideSound(zCVob* vob, int material)
{
    const zSTRING soundName = zSTRING("SS_") + SearchMediumTypeIDList(material, 0x4C48);
    zCSoundEventData* eventData = FindSoundEvent(slideSndTable, soundName);
    if (!eventData)
        return -1;
    zCSoundFX* sound;
    zCParticleEmitter* emitter;
    eventData->GetData(sound, emitter);
    if (!sound)
        return -1;
    zCSoundSystem::zTSound3DParams params = {};
    params.volume = 1.0f;
    params.radius = zsound->defaultRadius;
    params.reverbLevel = 1.0f;
    params.pitchOffset = -10000.0f;
    return zsound->PlaySound3D(sound, vob, 7, &params);
}

void zCSoundManager::StartDestructionSound(zCVob* vob, int material)
{
    if (!vob)
        return;
    const zSTRING soundName = zSTRING("DS_") + SearchMediumTypeIDList(material, 0x4F48);
    zCSoundEventData* eventData = FindSoundEvent(destSndTable, soundName);
    if (!eventData)
        return;
    zCSoundFX* sound;
    zCParticleEmitter* emitter;
    eventData->GetData(sound, emitter);
    if (sound) {
        zCSoundSystem::zTSound3DParams params = {};
        params.volume = 1.0f;
        params.radius = zsound->defaultRadius;
        params.reverbLevel = 1.0f;
        params.pitchOffset = -10000.0f;
        zsound->PlaySound3D(sound, vob, 6, &params);
    }
    if (emitter) {
        zCParticleFX* particleFx = new zCParticleFX;
        zCVob* effectVob = new zCVob;
        effectVob->SetCollDetStat(0);
        effectVob->SetCollDetDyn(0);
        effectVob->SetPositionWorld(vob->trafoObjToWorld.GetTranslation());
        effectVob->SetVisual(particleFx);
        particleFx->SetEmitter(emitter, 0);
        particleFx->CreateParticles();
        vob->homeWorld->AddVob(effectVob);
    }
}

zCSoundManager::zCMediumType::zCMediumType()
{
}

static int CompareSoundEvents(const void* first, const void* second)
{
    const zCSoundManager::zCSoundEventData* firstEvent =
        static_cast<const zCSoundManager::zCSoundEventData*>(first);
    const zCSoundManager::zCSoundEventData* secondEvent =
        static_cast<const zCSoundManager::zCSoundEventData*>(second);
    const char* firstName = firstEvent->name.ToChar();
    const char* secondName = secondEvent->name.ToChar();
    int index = 0;
    while (firstName[index] && firstName[index] == secondName[index])
        ++index;
    return static_cast<unsigned char>(firstName[index]) -
        static_cast<unsigned char>(secondName[index]);
}

static int CompareMediumTypes(const void* first, const void* second)
{
    const zCSoundManager::zCMediumType* firstMedium =
        static_cast<const zCSoundManager::zCMediumType*>(first);
    const zCSoundManager::zCMediumType* secondMedium =
        static_cast<const zCSoundManager::zCMediumType*>(second);
    return firstMedium->nameInt - secondMedium->nameInt;
}

void zCSoundManager::Init()
{
    debugMessageEnabled = 0;
    zCParser* parser = zsound->GetSFXParser();
    if (!parser)
        return;
    mediumTypeIDList.Compare = CompareMediumTypes;
    slideSndTable.Compare = CompareSoundEvents;
    destSndTable.Compare = CompareSoundEvents;
    for (int i = 0; i < 17; ++i)
        collSndTable[i].collSndSubTable.Compare = CompareSoundEvents;

    for (int i = 0; i < parser->GetNumSymbols(); ++i) {
        const int symbolType = parser->GetSymbolType(i);
        zSTRING fullName = parser->GetSymbolName(i);
        zSTRING name = fullName;
        name.Delete(0, 3);
        if (symbolType == 3) {
            zCMediumType medium;
            medium.nameInt = 0;
            for (int j = 0; j < name.Length() && j < 4; ++j)
                medium.nameInt += static_cast<unsigned char>(name[j]) << (j * 8);
            medium.contents = parser->GetSymbolStringValue(i, 0);
            mediumTypeIDList.InsertEnd(medium);
            continue;
        }
        if (symbolType != 7)
            continue;
        if (fullName.Search("SS_") != -1) {
            zCSoundEventData eventData;
            eventData.name = name;
            slideSndTable.InsertEnd(eventData);
            continue;
        }
        if (fullName.Search("DS_") != -1) {
            zCSoundEventData eventData;
            eventData.name = name;
            destSndTable.InsertEnd(eventData);
            continue;
        }
        const int tableIndex = GetCollSndTableIndex(name);
        if (name.Length() == 6) {
            collSndTable[tableIndex].defaultSoundEventData.name = name;
        } else if (name.Length() == 12) {
            zCSoundEventData eventData;
            eventData.name = name;
            collSndTable[tableIndex].collSndSubTable.InsertEnd(eventData);
        }
    }
    for (int i = 0; i < 17; ++i)
        collSndTable[i].collSndSubTable.QuickSort();
    mediumTypeIDList.QuickSort();
    slideSndTable.QuickSort();
    destSndTable.QuickSort();
}
