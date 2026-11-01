#include "include/zModel.h"
#include "include/zParticle.h"
#include "include/zVob.h"

static zCClassDef zCVisualClassDef("zCVisual", "zCObject", NULL, 0, 0x34, 0, "");
zCClassDef* zCVisual::classDef = &zCVisualClassDef;
zCArray<zCVisual*> zCVisual::s_visualClassList;

zCVisual::zCVisual()
    : prevLODVisual(0), nextLODVisual(0), lodFarDistance(50000.0f),
      lodNearFadeOutDistance(49000.0f)
{
}

zCVisual::~zCVisual()
{
    prevLODVisual = 0;
    nextLODVisual = 0;
}

zCClassDef* zCVisual::_GetClassDef() const
{
    return classDef;
}

void zCVisual::AddNextLODVisual(zCVisual* visual)
{
    if (!visual)
        return;
    if (nextLODVisual)
        nextLODVisual->prevLODVisual = visual;
    visual->nextLODVisual = nextLODVisual;
    visual->prevLODVisual = this;
    nextLODVisual = visual;
}

void zCVisual::AddEndLODVisual(zCVisual* visual)
{
    if (!visual)
        return;
    zCVisual* tail = this;
    while (tail->nextLODVisual)
        tail = tail->nextLODVisual;
    tail->nextLODVisual = visual;
    visual->nextLODVisual = 0;
    visual->prevLODVisual = tail;
}

void zCVisual::GetLODVisualAndAlpha(float distance, zCVisual*& visual, float& alpha)
{
    visual = this;
    while (visual) {
        if (distance <= visual->lodFarDistance) {
            alpha = 1.0f;
            if (distance > visual->lodNearFadeOutDistance) {
                alpha = (visual->lodFarDistance - distance) /
                    (visual->lodFarDistance - visual->lodNearFadeOutDistance);
            }
            return;
        }
        visual = visual->nextLODVisual;
    }
    alpha = 0;
}

int zCVisual::TraceRay(const zVEC3&, const zVEC3&, int, zTTraceRayReport&)
{
    return 1;
}

void zCVisual::InitVisualSystem()
{
    for (int i = 0; i < zCClassDef::classDefList->numInArray; ++i) {
        zCClassDef* objectClass = zCClassDef::classDefList->array[i];
        if (!objectClass)
            continue;
        for (zCClassDef* baseClass = objectClass; baseClass; baseClass = baseClass->baseClassDef) {
            if (baseClass != zCVisual::classDef)
                continue;
            if (objectClass->createNewInstance) {
                zCVisual* visual = static_cast<zCVisual*>(objectClass->CreateNewInstance());
                if (visual->GetFileExtension(0))
                    s_visualClassList.InsertEnd(visual);
                else
                    visual->Release();
            }
            break;
        }
    }
}

void zCVisual::CleanupVisualSystem()
{
    for (int i = 0; i < s_visualClassList.numInArray; ++i) {
        if (s_visualClassList.parray[i])
            s_visualClassList.parray[i]->Release();
        s_visualClassList.parray[i] = 0;
    }
    s_visualClassList.numInArray = 0;
}

zCVisual* zCVisual::LoadVisual(const zSTRING& fileName)
{
    for (int i = 0; i < s_visualClassList.numInArray; ++i) {
        zCVisual* visualClass = s_visualClassList.parray[i];
        for (int extensionIndex = 0;; ++extensionIndex) {
            const zSTRING* extension = visualClass->GetFileExtension(extensionIndex);
            if (!extension)
                break;
            if (fileName.Search(*extension) != -1)
                return visualClass->LoadVisualVirtual(fileName);
        }
    }
    zerr.Message("D: unknown visual class, unknown extension: " + fileName);
    return 0;
}

zSTRING zCVisual::GetVisualName() const
{
    return GetObjectName();
}

zCOBBox3D* zCVisual::GetOBBox3D()
{
    return NULL;
}

int zCVisual::GetVisualDied()
{
    return 0;
}

void zCVisual::DynLightVisual(const zCArray<zCVobLight*>&, zMAT4*)
{
}

unsigned long zCVisual::GetRenderSortKey() const
{
    return (unsigned long)this;
}

int zCVisual::CanTraceRay() const
{
    return 0;
}

void zCVisual::HostVobRemovedFromWorld(zCVob*, zCWorld*)
{
}

void zCVisual::HostVobAddedToWorld(zCVob*, zCWorld*)
{
}

const zSTRING* zCVisual::GetFileExtension(int)
{
    return NULL;
}

int zCVisual::GetAlphaTestingEnabled() const
{
    return 0;
}

void zCVisual::SetAlphaTestingEnabled(int)
{
}

zCVisual* zCVisual::LoadVisualVirtual(const zSTRING&) const
{
    return NULL;
}

static zCClassDef zCVisualAnimateClassDef("zCVisualAnimate", "zCVisual", NULL, 0, 0x34, 0, "");
zCClassDef* zCVisualAnimate::classDef = &zCVisualAnimateClassDef;

zCVisualAnimate::~zCVisualAnimate()
{
}

zCClassDef* zCVisualAnimate::_GetClassDef() const
{
    return classDef;
}
