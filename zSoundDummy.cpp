#include "include/zSoundMan.h"

void zCSoundSystemDummy::DoSoundUpdate()
{
}

zCSoundFX* zCSoundSystemDummy::LoadSoundFX(const zSTRING&)
{
    return new zCSoundFXDummy;
}

zCSoundFXDummy::~zCSoundFXDummy()
{
}

int zCSoundSystemDummy::PlaySound(zCSoundFX*, int)
{
    return 0;
}

int zCSoundSystemDummy::PlaySound(zCSoundFX*, int, int, float, float)
{
    return 0;
}

void zCSoundSystemDummy::SetGlobalOcclusion(float)
{
}

int zCSoundSystemDummy::PlaySound3D(zCSoundFX*, zCVob*, int, zTSound3DParams*)
{
    return 0;
}

int zCSoundSystemDummy::PlaySound3D(const zSTRING&, zCVob*, int, zTSound3DParams*)
{
    return 0;
}

int zCSoundSystemDummy::GetSound3DProps(const int&, zTSound3DParams&)
{
    return 0;
}

int zCSoundSystemDummy::UpdateSound3D(const int&, zTSound3DParams&)
{
    return 0;
}

zCSoundManager::zCTableEntry::~zCTableEntry()
{
}
