#include "include/oCollisionObjectMiscChar.h"
#include "include/zCollisionDetector.h"

zCCollisionObjectDef zCCollObjectCharacter::s_oCollObjClass(0, zCCollObjectCharacter::_CreateNewInstance);
zCCollisionObjectDef oCCollObjectCharacter::s_oCollObjClass(0, oCCollObjectCharacter::_CreateNewInstance);

zCCollObjectCharacter::zCCollObjectCharacter()
{
    m_oInterferenceReport.spacingRayFrontPoly = -1;
    m_oInterferenceReport.spacingRayFrontVob = 0;
}

zCCollObjectCharacter::~zCCollObjectCharacter()
{
}

zCConvexPrimitive* zCCollObjectCharacter::GetPrimitiveCC() const
{
    return 0;
}

oCCollObjectCharacter::oCCollObjectCharacter()
    : comeNearerCounter(-1), mustNotComeNearer(0)
{
}

oCCollObjectCharacter::~oCCollObjectCharacter()
{
}

zCCollisionObject* oCCollObjectCharacter::_CreateNewInstance()
{
    return new oCCollObjectCharacter;
}

zCCollisionObjectDef* oCCollObjectCharacter::GetCollObjClass() const
{
    return &s_oCollObjClass;
}

float oCCollObjectCharacter::TurnSpeed() const
{
    oCNpc* npc = static_cast<oCNpc*>(m_pvClientData);
    if (npc)
        return npc->GetTurnSpeed() * 0.0174532924f;
    return 1.0f;
}

int oCCollObjectCharacter::IsDead() const
{
    oCNpc* npc = static_cast<oCNpc*>(m_pvClientData);
    return npc ? npc->IsDead() : 0;
}

void oCCollObjectCharacter::PushAround(const zVEC3&)
{
}

int oCCollObjectCharacter::DontLetHimComeNearer(const zCVob* vob) const
{
    if (vob == mustNotComeNearer && comeNearerCounter > 0) {
        --comeNearerCounter;
        return 1;
    }
    return 0;
}

zCConvexPrimitive* oCCollObjectCharacter::GetPrimitiveCC() const
{
    oCNpc* npc = static_cast<oCNpc*>(m_pvClientData);
    if (npc)
        npc->GetModel();
    return zCCollObjectCharacter::GetPrimitiveCC();
}

void oCCollObjectCharacter::S_RegisterCollisionTestFuncs()
{
    static int registered;
    if (registered)
        return;
    registered = 1;
    zCVob::s_poCollisionDetector->RegisterStaticIntersectionTest(&s_oCollObjClass,
        &s_oCollObjClass, TestStatic_Char_Char);
    zCVob::s_poCollisionDetector->RegisterDynamicIntersectionTest(&s_oCollObjClass,
        &zCCollObjectBoxPassThrough::s_oCollObjClass, TestDynamic_XXX_BoxPassThrough);
    zCVob::s_poCollisionDetector->RegisterDynamicIntersectionTest(&zCCollObjectProjectile::s_oCollObjClass,
        &s_oCollObjClass, TestDynamic_Projectile_Char);
}
