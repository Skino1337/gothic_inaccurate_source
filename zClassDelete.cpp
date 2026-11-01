#include "include/oNpcMessages.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/zPolyStrip.h"
#include "include/zParticle.h"

void oCMsgWeapon::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void zCMaterial::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void zCVisual::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void zCQuadMark::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void oCVisualFX::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

static zCClassDef oCSpellClassDef("oCSpell", "zCObject",
    oCSpell::_CreateNewInstance, 0, 0xB0, 0, "");
zCClassDef* oCSpell::classDef = &oCSpellClassDef;

void oCSpell::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

static zCClassDef oCVisFXMultiTargetClassDef("oCVisFX_MultiTarget", "oCVisualFX",
    oCVisFX_MultiTarget::_CreateNewInstance, 0, 0x61C, 0, "");
zCClassDef* oCVisFX_MultiTarget::classDef = &oCVisFXMultiTargetClassDef;

void oCVisFX_MultiTarget::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void zCParticleFX::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void zCEarthquake::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void zCVobScreenFX::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void zCPolyStrip::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void zCVobLight::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void oCMsgDamage::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}
