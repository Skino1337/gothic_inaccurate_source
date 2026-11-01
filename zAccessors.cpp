#include "include/zVob.h"
#include "include/zResource.h"
#include "include/zTools.h"

static int zoneState = -1;
static zCClassDef zCZoneClassDef("zCZone", "zCVob", zCZone::_CreateNewInstance, 0, 0x124, 0, "");
zCClassDef* zCZone::classDef = &zCZoneClassDef;
static zCClassDef zCZoneZFogDefaultClassDef("zCZoneZFogDefault", "zCZoneZFog",
    zCZoneZFogDefault::_CreateNewInstance, 0, 0x138, 0, "");
zCClassDef* zCZoneZFogDefault::classDef = &zCZoneZFogDefaultClassDef;
int zCBspTree::s_showPortals;

void zCTimer::SetMotionFactor(float factor)
{
    factorMotion = factor;
}

void zCResourceManager::SetShowDebugInfo(int show)
{
    showDebugInfo = show;
}

int zCResourceManager::GetShowDebugInfo() const
{
    return showDebugInfo;
}

void zCProgMeshProto::SetLODParamStrength(float strength)
{
    s_lodParamStrengthOverride = strength;
}

int zCBspTree::GetShowPortals()
{
    return s_showPortals;
}

void zCBspTree::SetShowPortals(int show)
{
    s_showPortals = show;
}

void zCVob::SetShowHelperVisuals(int show)
{
    s_showHelperVisuals = show;
}

int zCVob::GetShowHelperVisuals()
{
    return s_showHelperVisuals;
}

int zCVob::GetAnimationsEnabled()
{
    return s_enableAnimations;
}

void zCVob::SetAnimationsEnabled(int enabled)
{
    s_enableAnimations = enabled;
}

void zCZone::SetVisual(zCVisual*)
{
}

void zCZone::SetVisual(const zSTRING&)
{
}

zCClassDef* zCZone::GetZoneMotherClass() const
{
    return _GetClassDef();
}

zCClassDef* zCZoneZFogDefault::_GetClassDef() const
{
    return classDef;
}

int zCWorld::GetWaveAnisEnabled()
{
    return s_bWaveAnisEnabled;
}

void zCWorld::SetWaveAnisEnabled(int enabled)
{
    s_bWaveAnisEnabled = enabled;
}

int zCWorld::GetShowTextureStats()
{
    return showTextureStats;
}

void zCWorld::SetShowTextureStats(int enabled)
{
    showTextureStats = enabled;
}

void zCWorld::SetVobFarClipZScalability(float scalability)
{
    vobFarClipZScalability = scalability;
}

static zCClassDef zCZoneReverbClassDef("zCZoneReverb", "zCZone", zCZoneReverb::_CreateNewInstance,
    0, 0x130, 0, "");
zCClassDef* zCZoneReverb::classDef = &zCZoneReverbClassDef;
static zCClassDef zCZoneReverbDefaultClassDef("zCZoneReverbDefault", "zCZoneReverb",
    zCZoneReverbDefault::_CreateNewInstance, 0, 0x130, 0, "");
zCClassDef* zCZoneReverbDefault::classDef = &zCZoneReverbDefaultClassDef;

// EXE: 0x00639680
zCObject* zCZoneReverb::_CreateNewInstance()
{
    zCZoneReverb* zone = new zCZoneReverb;
    zCClassDef::ObjectCreated(zone, classDef);
    return zone;
}

zCClassDef* zCZone::_GetClassDef() const
{
    return classDef;
}

void zCZone::ProcessZoneList(const zCArraySort<zCZone*>&, const zCArraySort<zCZone*>&, zCWorld*)
{
}

zCClassDef* zCZone::GetDefaultZoneClass() const
{
    return 0;
}

zCClassDef* zCZoneReverb::_GetClassDef() const
{
    return classDef;
}

// EXE: 0x00639990
zCObject* zCZoneReverbDefault::_CreateNewInstance()
{
    zCZoneReverbDefault* zone = new zCZoneReverbDefault;
    zCClassDef::ObjectCreated(zone, classDef);
    return zone;
}

zCClassDef* zCZoneReverbDefault::_GetClassDef() const
{
    return classDef;
}

void zCZoneReverbDefault::ProcessZoneList(const zCArraySort<zCZone*>&,
    const zCArraySort<zCZone*>&, zCWorld*)
{
}

zCZoneReverbDefault::~zCZoneReverbDefault()
{
    if (world)
        world->RemoveVobFromLists(this);
    world = 0;
}

static zCClassDef zCZoneVobFarPlaneClassDef("zCZoneVobFarPlane", "zCZone",
    zCZoneVobFarPlane::_CreateNewInstance, 0, 0x12C, 0, "");
zCClassDef* zCZoneVobFarPlane::classDef = &zCZoneVobFarPlaneClassDef;
static zCClassDef zCZoneVobFarPlaneDefaultClassDef("zCZoneVobFarPlaneDefault", "zCZoneVobFarPlane",
    zCZoneVobFarPlaneDefault::_CreateNewInstance, 0, 0x12C, 0, "");
zCClassDef* zCZoneVobFarPlaneDefault::classDef = &zCZoneVobFarPlaneDefaultClassDef;
static zCClassDef zCZoneMusicClassDef("zCZoneMusic", "zCZone", zCZoneMusic::_CreateNewInstance,
    0, 0x124, 0, "");
zCClassDef* zCZoneMusic::classDef = &zCZoneMusicClassDef;
static zCClassDef zCZoneZFogClassDef("zCZoneZFog", "zCZone", zCZoneZFog::_CreateNewInstance,
    0, 0x138, 2, "");
zCClassDef* zCZoneZFog::classDef = &zCZoneZFogClassDef;

// EXE: 0x00639D20
zCObject* zCZoneVobFarPlane::_CreateNewInstance()
{
    zCZoneVobFarPlane* zone = new zCZoneVobFarPlane;
    zCClassDef::ObjectCreated(zone, classDef);
    return zone;
}

zCClassDef* zCZoneVobFarPlane::_GetClassDef() const
{
    return classDef;
}

// EXE: 0x0063A000
zCObject* zCZoneVobFarPlaneDefault::_CreateNewInstance()
{
    zCZoneVobFarPlaneDefault* zone = new zCZoneVobFarPlaneDefault;
    zCClassDef::ObjectCreated(zone, classDef);
    return zone;
}

zCClassDef* zCZoneVobFarPlaneDefault::_GetClassDef() const
{
    return classDef;
}

zCZoneVobFarPlaneDefault::~zCZoneVobFarPlaneDefault()
{
    if (world)
        world->RemoveVobFromLists(this);
    world = 0;
}

// EXE: 0x0063A380
zCObject* zCZoneMusic::_CreateNewInstance()
{
    zCZoneMusic* zone = new zCZoneMusic;
    zCClassDef::ObjectCreated(zone, classDef);
    return zone;
}

zCClassDef* zCZoneMusic::_GetClassDef() const
{
    return classDef;
}

zCZoneMusic::~zCZoneMusic()
{
    if (world)
        world->RemoveVobFromLists(this);
    world = 0;
}

// EXE: 0x0063A6F0
zCObject* zCZoneZFog::_CreateNewInstance()
{
    zCZoneZFog* zone = new zCZoneZFog;
    zCClassDef::ObjectCreated(zone, classDef);
    return zone;
}

zCClassDef* zCZoneZFog::_GetClassDef() const
{
    return classDef;
}

// EXE: 0x0063A9E0
zCObject* zCZoneZFogDefault::_CreateNewInstance()
{
    zCZoneZFogDefault* zone = new zCZoneZFogDefault;
    zCClassDef::ObjectCreated(zone, classDef);
    return zone;
}

static zCClassDef zCVobSoundClassDef("zCVobSound", "zCVob", zCVobSound::_CreateNewInstance,
    0, 0x16C, 4, "");
zCClassDef* zCVobSound::classDef = &zCVobSoundClassDef;
static zCClassDef zCVobSoundDaytimeClassDef("zCVobSoundDaytime", "zCVobSound",
    zCVobSoundDaytime::_CreateNewInstance, 0, 0x190, 0, "");
zCClassDef* zCVobSoundDaytime::classDef = &zCVobSoundDaytimeClassDef;

// EXE: 0x0063ACC0
zCObject* zCVobSound::_CreateNewInstance()
{
    zCVobSound* zone = new zCVobSound;
    zCClassDef::ObjectCreated(zone, classDef);
    return zone;
}

// EXE: 0x0063AF30
zCObject* zCVobSoundDaytime::_CreateNewInstance()
{
    zCVobSoundDaytime* zone = new zCVobSoundDaytime;
    zCClassDef::ObjectCreated(zone, classDef);
    return zone;
}

zCClassDef* zCVobSoundDaytime::_GetClassDef() const
{
    return classDef;
}

zCClassDef* zCVobSoundDaytime::GetZoneMotherClass() const
{
    return &zCVobSound::classDef;
}

zCBBox3DSorterBase* zCBBox3DSorterBase::s_currentSorter;
int zCBBox3DSorterBase::s_currentDimension;

zCBBox3DSorterBase::zTBoxSortHandle::~zTBoxSortHandle()
{
    if (mySorter) {
        zCBBox3DSorterBase* sorter = mySorter;
        mySorter = 0;
        for (int i = 0; i < sorter->handles.numInArray; ++i) {
            if (sorter->handles.parray[i] == this) {
                --sorter->handles.numInArray;
                if (i != sorter->handles.numInArray)
                    sorter->handles.parray[i] = sorter->handles.parray[sorter->handles.numInArray];
                break;
            }
        }
    }
    ClearActive();
}

zCBBox3DSorterBase::zCBBox3DSorterBase()
    : sorted(0)
{
    for (int i = 0; i < 3; ++i)
        nodeList[i].Compare = ArrayCompare;
}

zCBBox3DSorterBase::~zCBBox3DSorterBase()
{
    if (s_currentSorter == this)
        s_currentSorter = 0;
    Clear();
}

int zCBBox3DSorterBase::ArrayCompare(const void* first, const void* second)
{
    const zTNode* firstNode = *static_cast<zTNode* const*>(first);
    const zTNode* secondNode = *static_cast<zTNode* const*>(second);
    const zTBBox3D& firstBox = s_currentSorter->GetBBox(firstNode);
    const zTBBox3D& secondBox = s_currentSorter->GetBBox(secondNode);
    const float firstValue = firstNode->isBegin ? firstBox.maxs.n[s_currentDimension] :
        firstBox.mins.n[s_currentDimension];
    const float secondValue = secondNode->isBegin ? secondBox.maxs.n[s_currentDimension] :
        secondBox.mins.n[s_currentDimension];
    if (firstValue < secondValue)
        return -1;
    return firstValue > secondValue;
}
