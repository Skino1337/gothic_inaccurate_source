#include "include/oAiHuman.h"

static zCClassDef oCAICameraClassDef("oCAICamera", "zCObject", oCAICamera::_CreateNewInstance,
    0, 0x4C, 0, "");
zCClassDef* oCAICamera::classDef = &oCAICameraClassDef;
static zCClassDef oCAIHumanStandClassDef("oCAIHuman_Stand", "oCAIBase",
    oCAIHuman_Stand::_CreateNewInstance, 0, 0x24, 0, "");
zCClassDef* oCAIHuman_Stand::classDef = &oCAIHumanStandClassDef;

oCAICamera::oCAICamera()
{
}

oCAICamera::~oCAICamera()
{
}

zCObject* oCAICamera::_CreateNewInstance()
{
    oCAICamera* ai = new oCAICamera;
    zCClassDef::ObjectCreated(ai, classDef);
    return ai;
}

void oCAICamera::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* oCAICamera::_GetClassDef() const
{
    return classDef;
}

int oCAICamera::HasAIDetectedCollision()
{
    return 0;
}

void oCAICamera::DoAI(zCVob* vob, int& state)
{
    state = 0;
    if (!vob)
        return;
    float time = ztimer.frameTimeFloat * 0.001f;
    if (time == 0.0f)
        time = 0.02f;
    float speed = 350.0f * time;
    float rotation = 150.0f * time;
    if (zinput->KeyPressed(0x2a) || zinput->KeyPressed(0x36)) {
        speed *= 4.0f;
        rotation *= 2.0f;
    }
    float forward = zinput->KeyPressed(0xc8) ? speed : zinput->KeyPressed(0xd0) ? -speed : 0.0f;
    float pitch = zinput->KeyPressed(0xcb) ? -rotation : zinput->KeyPressed(0xcd) ? rotation : 0.0f;
    float strafe = zinput->KeyPressed(0x1e) ? speed : zinput->KeyPressed(0x2c) ? -speed : 0.0f;
    float yaw = zinput->KeyPressed(0xc9) ? -rotation : zinput->KeyPressed(0xd1) ? rotation : 0.0f;
    zVEC3 up(0.0f, 1.0f, 0.0f);
    vob->RotateWorld(up, yaw);
    zVEC3 right(1.0f, 0.0f, 0.0f);
    vob->RotateLocal(right, pitch);
    vob->MoveLocal(strafe, 0.0f, forward);
}

oCAIHuman_Stand::oCAIHuman_Stand(zCVob* vob)
{
}

oCAIHuman_Stand::~oCAIHuman_Stand()
{
}

zCObject* oCAIHuman_Stand::_CreateNewInstance()
{
    oCAIHuman_Stand* ai = new oCAIHuman_Stand(0);
    zCClassDef::ObjectCreated(ai, classDef);
    return ai;
}

void oCAIHuman_Stand::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

zCClassDef* oCAIHuman_Stand::_GetClassDef() const
{
    return classDef;
}

void oCAIHuman_Stand::DoAI(zCVob* vob, int& state)
{
    state = 0;
    if (!vob || !vob->homeWorld)
        return;
    zVEC3 origin = {
        vob->trafoObjToWorld.m[0][3], vob->trafoObjToWorld.m[1][3],
        vob->trafoObjToWorld.m[2][3]
    };
    zVEC3 direction = { 0.0f, -100000.0f, 0.0f };
    if (vob->homeWorld->TraceRayNearestHit(origin, direction, vob, 0x20)
        && vob->homeWorld->traceRayReport.foundPoly) {
        zVEC3 position = vob->homeWorld->traceRayReport.foundIntersection;
        position.n[1] += 4.0f;
        vob->SetPositionWorld(position);
        vob->SetPhysicsEnabled(0);
    }
}

zCObject* oCAIHuman::_CreateNewInstance()
{
    oCAIHuman* ai = new oCAIHuman;
    zCClassDef::ObjectCreated(ai, classDef);
    return ai;
}

void oCAIHuman::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}
