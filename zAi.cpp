#include "include/oDialog.h"
#include "include/zAi.h"
#include "include/zSoundMan.h"

static int aiBaseClassDefState = -1;
static zCClassDef zCAIBaseClassDef("zCAIBase", "zCObject", NULL, 0, 0x24, 0, "");
zCClassDef* zCAIBase::classDef = &zCAIBaseClassDef;
static zCClassDef zCAIBaseSoundClassDef("zCAIBaseSound", "zCAIBase",
    zCAIBaseSound::_CreateNewInstance, 0, 0x2C, 0, "");
zCClassDef* zCAIBaseSound::classDef = &zCAIBaseSoundClassDef;

void zCAIBase::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* zCAIBase::_GetClassDef() const
{
    return classDef;
}

int zCAIBase::CanThisCollideWith(zCVob*)
{
    return 1;
}

int zCAIBase::HasAIDetectedCollision()
{
    return 0;
}

void zCAIBase::AICollisionResponseSelfDetected(const zVEC3&, int&)
{
}

void zCAIBase::HostVobRemovedFromWorld(zCVob*, zCWorld*)
{
}

void zCAIBase::ReportCollisionToAI(const zCCollisionReport&)
{
}

void zCAIBase::HostVobAddedToWorld(zCVob*, zCWorld*)
{
}

int zCAIBase::GetIsProjectile()
{
    return 0;
}

zCAIBase::~zCAIBase()
{
}

void zCAIBaseSound::RemoveSlideSound()
{
    if (slideSoundOn) {
        slideSoundOn = 0;
        zsound->StopSound(slideSoundHandle);
    }
}

void zCAIBaseSound::CheckSlideSound(zCVob* vob)
{
    if ((vob->showVisual & 8) && vob->GetRigidBody()->mode == zPHY_RBMODE_SLIDE) {
        if (slideSoundOn) {
            zsound->UpdateSound3D(slideSoundHandle, NULL);
            return;
        }

        int material = 0;
        if (vob->groundPoly)
            material = vob->groundPoly->material->matGroup;
        slideSoundOn = 1;
        slideSoundHandle = zsndMan->StartSlideSound(vob, material);
        return;
    }

    RemoveSlideSound();
}

void zCAIBaseSound::StartDefaultCollisionSound(zCVob* vob, const zCCollisionReport& report)
{
    zCCollisionObject* collisionObject = report.m_poCollObject1;
    if (collisionObject->GetCollObjClass() == &zCCollObjectLevelPolys::s_oCollObjClass) {
        int material = 0;
        zCCollObjectLevelPolys* levelPolys = static_cast<zCCollObjectLevelPolys*>(collisionObject);
        if (levelPolys->m_aoPolyList.numInArray > 0)
            material = levelPolys->m_aoPolyList.parray[0]->material->matGroup;
        zsndMan->StartHitSound(vob, zCSoundManager::zSND_MAN_MEDIUM_OBJECT,
            zCSoundManager::zSND_MAN_MEDIUM_LEVEL, 0x63, material,
            const_cast<zCCollisionReport*>(&report));
        return;
    }

    zsndMan->StartHitSound(vob, zCSoundManager::zSND_MAN_MEDIUM_ITEM,
        zCSoundManager::zSND_MAN_MEDIUM_ITEM, 0x63, 0x63,
        const_cast<zCCollisionReport*>(&report));
}
