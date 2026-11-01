#include "zPhysics.h"

#include <cmath>
#include <cstring>

struct zTIMER {
    float frameTimeFloat;
};

extern zTIMER ztimer;

static float PhysicsDot(const zVEC3& left, const zVEC3& right)
{
    return left.n[0] * right.n[0] + left.n[1] * right.n[1] + left.n[2] * right.n[2];
}

static zVEC3 PhysicsCross(const zVEC3& left, const zVEC3& right)
{
    zVEC3 result;
    result.n[0] = left.n[1] * right.n[2] - left.n[2] * right.n[1];
    result.n[1] = left.n[2] * right.n[0] - left.n[0] * right.n[2];
    result.n[2] = left.n[0] * right.n[1] - left.n[1] * right.n[0];
    return result;
}

static float PhysicsLength(const zVEC3& value)
{
    return std::sqrt(PhysicsDot(value, value));
}

static void PhysicsZero(zVEC3& value)
{
    value.n[0] = 0;
    value.n[1] = 0;
    value.n[2] = 0;
}

static void PhysicsScale(zVEC3& value, float scale)
{
    value.n[0] *= scale;
    value.n[1] *= scale;
    value.n[2] *= scale;
}

zCRigidBody::zCRigidBody()
{
    std::memset(this, 0, sizeof(zCRigidBody));
    mass = 10.0f;
    massInv = 0.1f;
    gravityScale = 1.0f;
    gravityOn = 1;
    for (int i = 0; i < 3; ++i) {
        RDir.v[i].n[i] = 1.0f;
        const float inertia = mass * 5000.0f / 12.0f;
        iBody.v[i].n[i] = inertia;
        iBodyInv.v[i].n[i] = 1.0f / inertia;
    }
}

void zCRigidBody::ComputeForceTorque(float)
{
}

zMAT3 zCRigidBody::Star(const zVEC3& value)
{
    zMAT3 result;
    PhysicsZero(result.v[0]);
    result.v[0].n[1] = value.n[2];
    result.v[0].n[2] = -value.n[1];
    result.v[1].n[0] = -value.n[2];
    PhysicsZero(result.v[1]);
    result.v[1].n[2] = value.n[0];
    PhysicsZero(result.v[2]);
    result.v[2].n[0] = value.n[1];
    result.v[2].n[1] = -value.n[0];
    return result;
}

int zCRigidBody::SetSliding(const zVEC3& normal)
{
    float dot = normal.n[1];
    if (dot > 1.0f)
        dot = 1.0f;
    else if (dot < -1.0f)
        dot = -1.0f;
    slideAngle = std::acos(dot) * 57.2957764f;
    slideDir = PhysicsCross(PhysicsCross(v, normal), normal);
    const float length = PhysicsLength(slideDir);
    if (length > 0)
        PhysicsScale(slideDir, 1.0f / length);
    if (slideDir.n[1] < 0)
        PhysicsScale(slideDir, -1.0f);
    mode = zPHY_RBMODE_SLIDE;
    justSetSliding = 1;
    return 1;
}

void zCRigidBody::Integrate(float time)
{
    if (justSetSliding > 0)
        --justSetSliding;
    if (collisionHad)
        force.n[1] += 1.0f;
    else if (gravityOn && mode == zPHY_RBMODE_FLY)
        force.n[1] -= gravityScale * mass * 981.0f;
    else if (gravityOn) {
        zVEC3 gravity;
        gravity.n[0] = 0;
        gravity.n[1] = -gravityScale * mass * 981.0f * slideDir.n[1];
        gravity.n[2] = 0;
        force.n[0] += gravity.n[0] - v.n[0] * mass;
        force.n[1] += gravity.n[1] - v.n[1] * mass;
        force.n[2] += gravity.n[2] - v.n[2] * mass;
    }

    for (int axis = 0; axis < 3; ++axis) {
        PLinMom.n[axis] += force.n[axis] * time;
        LAngMom.n[axis] += torque.n[axis] * time;
        v.n[axis] = PLinMom.n[axis] * massInv;
        xPos.n[axis] += v.n[axis] * time;
    }
    collisionHad = 0;
    PhysicsZero(force);
    PhysicsZero(torque);
}

void zCRigidBody::RunSimulation()
{
    const float time = ztimer.frameTimeFloat * 0.001f;
    if (time > 0)
        Integrate(time);
}

void zCRigidBody::SetInertiaTensorBlock(float x, float y, float z)
{
    const float diagonal[3] = { y * y + z * z, x * x + z * z, x * x + y * y };
    for (int i = 0; i < 3; ++i) {
        const float inertia = mass * diagonal[i] / 12.0f;
        iBody.v[i].n[i] = inertia;
        iBodyInv.v[i].n[i] = inertia != 0 ? 1.0f / inertia : 0;
    }
}

void zCRigidBody::ApplyForceCM(const zVEC3& forceValue)
{
    for (int axis = 0; axis < 3; ++axis)
        force.n[axis] += forceValue.n[axis];
}

void zCRigidBody::ApplyForce(const zVEC3& forceValue, const zVEC3&)
{
    ApplyForceCM(forceValue);
}

zVEC3 zCRigidBody::GetPointVelocity(const zVEC3& point) const
{
    zVEC3 relative;
    for (int axis = 0; axis < 3; ++axis)
        relative.n[axis] = point.n[axis] - xPos.n[axis];
    zVEC3 result = PhysicsCross(omega, relative);
    for (int axis = 0; axis < 3; ++axis)
        result.n[axis] += v.n[axis];
    return result;
}

void zCRigidBody::ApplyImpulseCM(const zVEC3& impulse)
{
    for (int axis = 0; axis < 3; ++axis) {
        PLinMom.n[axis] += impulse.n[axis];
        v.n[axis] = PLinMom.n[axis] * massInv;
    }
}

void zCRigidBody::ApplyImpulse(const zVEC3& impulse, const zVEC3&)
{
    ApplyImpulseCM(impulse);
}

void zCRigidBody::CollisionImpulseStatic(const zVEC3& normal, const zVEC3&, float elasticity)
{
    const float speed = PhysicsDot(v, normal);
    if (speed >= -0.001f)
        return;
    float inverseMass = massInv;
    if (inverseMass == 0)
        inverseMass = 0.00001f;
    const float impulse = -speed * (elasticity + 1.0f) / inverseMass;
    for (int axis = 0; axis < 3; ++axis) {
        PLinMom.n[axis] += impulse * normal.n[axis];
        v.n[axis] = PLinMom.n[axis] * massInv;
    }
    collisionHad = 1;
}

void zCRigidBody::CollisionImpulseCenterStatic(const zVEC3& normal, float)
{
    const float speed = PhysicsDot(v, normal);
    for (int axis = 0; axis < 3; ++axis)
        v.n[axis] = (v.n[axis] - 2.0f * speed * normal.n[axis]) * 0.3f;
    for (int axis = 0; axis < 3; ++axis)
        PLinMom.n[axis] = v.n[axis] / massInv;
    PhysicsZero(force);
    collisionHad = 1;
}

void zCRigidBody::CollisionImpulse(const zVEC3& normal, const zVEC3&, float elasticity,
    zCRigidBody* other)
{
    zVEC3 relative;
    for (int axis = 0; axis < 3; ++axis)
        relative.n[axis] = v.n[axis] - other->v.n[axis];
    const float speed = PhysicsDot(relative, normal);
    if (speed >= -0.001f)
        return;
    const float impulse = -speed * (elasticity + 1.0f) / (massInv + other->massInv);
    for (int axis = 0; axis < 3; ++axis) {
        PLinMom.n[axis] += impulse * normal.n[axis];
        other->PLinMom.n[axis] -= impulse * normal.n[axis];
        v.n[axis] = PLinMom.n[axis] * massInv;
        other->v.n[axis] = other->PLinMom.n[axis] * other->massInv;
    }
}

void zCRigidBody::StopRotation()
{
    PhysicsZero(torque);
    PhysicsZero(LAngMom);
    PhysicsZero(omega);
}

void zCRigidBody::StopTranslation()
{
    PhysicsZero(force);
    PhysicsZero(PLinMom);
    PhysicsZero(v);
}

void zCRigidBody::StopTransRot()
{
    StopRotation();
    StopTranslation();
    collisionHad = 0;
}

void zCRigidBody::SetVelocity(const zVEC3& velocity)
{
    PhysicsZero(force);
    v = velocity;
    for (int axis = 0; axis < 3; ++axis)
        PLinMom.n[axis] = v.n[axis] / massInv;
}

void zCRigidBody::SetMode(const zTRigidBodyMode& newMode)
{
    mode = newMode;
}

int zCRigidBody::ShouldSlide(const zVEC3& normal) const
{
    float normalAngle = normal.n[1];
    if (normalAngle > 1.0f)
        normalAngle = 1.0f;
    else if (normalAngle < -1.0f)
        normalAngle = -1.0f;
    if (std::acos(normalAngle) * 57.2957764f <= 70.0f)
        return 0;

    const float speed = PhysicsLength(v);
    if (speed == 0)
        return 0;
    zVEC3 inverseVelocity;
    for (int axis = 0; axis < 3; ++axis)
        inverseVelocity.n[axis] = -v.n[axis] / speed;
    float direction = PhysicsDot(inverseVelocity, normal);
    if (direction > 1.0f)
        direction = 1.0f;
    else if (direction < -1.0f)
        direction = -1.0f;
    const float angle = std::acos(direction) * 57.2957764f;
    return angle >= 40.0f && angle <= 110.0f && speed * speed < 490000.0f;
}

void zCRigidBody::Archive(zCArchiver& archiver)
{
    archiver.WriteVec3(v);
    archiver.WriteByte(mode);
    archiver.WriteBool(gravityOn);
    archiver.WriteFloat(gravityScale);
    archiver.WriteVec3(slideDir);
}

void zCRigidBody::Unarchive(zCArchiver& archiver)
{
    SetVelocity(archiver.ReadVec3());
    mode = archiver.ReadByte() != 0;
    gravityOn = archiver.ReadBool() != 0;
    gravityScale = archiver.ReadFloat();
    slideDir = archiver.ReadVec3();
}
