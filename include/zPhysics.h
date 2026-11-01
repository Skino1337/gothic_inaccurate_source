#ifndef ZPHYSICS_H
#define ZPHYSICS_H

#include "zModel.h"

struct zMAT3 {
    zMAT3() {}
    zMAT3(const zVEC3& first, const zVEC3& second, const zVEC3& third);
    zMAT3(float value);
    zMAT3(const zMAT3& other);
    zMAT3& operator=(const zMAT3& other);
    zMAT3& operator+=(const zMAT3& other);
    zMAT3& operator-=(const zMAT3& other);
    zMAT3& operator*=(float value);
    zMAT3& operator/=(float value);
    zMAT3 Transpose() const;
    zMAT3 Inverse(float* determinant) const;
    zMAT3& Apply(float (*function)(float));
    void MakeOrthonormal();
    void SetUpVector(const zVEC3& vector);
    void SetRightVector(const zVEC3& vector);

    zVEC3 v[3];

    static zMAT3 s_identity;
};

zMAT3 operator-(const zMAT3& value);
zMAT3 operator+(const zMAT3& first, const zMAT3& second);
zMAT3 operator-(const zMAT3& first, const zMAT3& second);
zMAT3 operator*(const zMAT3& first, const zMAT3& second);
zMAT3 operator*(const zMAT3& value, float scalar);
zMAT3 operator*(float scalar, const zMAT3& value);
zMAT3 operator/(const zMAT3& value, float scalar);
int operator==(const zMAT3& first, const zMAT3& second);

enum zTRigidBodyMode {
    zPHY_RBMODE_FLY,
    zPHY_RBMODE_SLIDE
};

class zCRigidBody {
public:

    zCRigidBody();

    void  ComputeForceTorque(float time);
    zMAT3 Star(const zVEC3& value);
    int   SetSliding(const zVEC3& normal);
    void  Integrate(float time);
    void  RunSimulation();
    void  SetInertiaTensorBlock(float x, float y, float z);
    void  ApplyForceCM(const zVEC3& forceValue);
    void  ApplyForce(const zVEC3& forceValue, const zVEC3& point);
    zVEC3 GetPointVelocity(const zVEC3& point) const;
    void  ApplyImpulseCM(const zVEC3& impulse);
    void  ApplyImpulse(const zVEC3& impulse, const zVEC3& point);
    void  CollisionImpulseStatic(const zVEC3& normal, const zVEC3& point, float elasticity);
    void  CollisionImpulseCenterStatic(const zVEC3& normal, float friction);
    void  CollisionImpulse(const zVEC3& normal, const zVEC3& point, float elasticity,
        zCRigidBody* other);
    void  StopRotation();
    void  StopTranslation();
    void  StopTransRot();
    void  SetVelocity(const zVEC3& velocity);
    void  SetMode(const zTRigidBodyMode& newMode);
    int   ShouldSlide(const zVEC3& normal) const;
    void  Archive(zCArchiver& archiver);
    void  Unarchive(zCArchiver& archiver);

    float mass;
    float massInv;
    zMAT3 iBody;
    zMAT3 iBodyInv;
    zVEC3 xPos;
    zMAT3 RDir;
    zVEC3 PLinMom;
    zVEC3 LAngMom;
    zMAT3 iInv;
    zVEC3 v;
    zVEC3 omega;
    zVEC3 force;
    zVEC3 torque;
    float gravityScale;
    zVEC3 slideDir;
    float slideAngle;
    unsigned char gravityOn : 1;
    unsigned char collisionHad : 1;
    unsigned char mode : 1;
    unsigned char justSetSliding : 4;
};

class zCCollisionReport;

class zCCollisionObject {
public:

    zCCollisionObjectClass* GetCollObjClass() const;
    void CollisionResponse(zCArray<zCCollisionReport*>& reports, int& flags);

    zMAT4 m_oTrafoOld;
    zMAT4 m_oTrafoNew;
    void* m_pvClientData;
    unsigned char m_bHintTranslation : 1;
};

class zCCollisionReport {
public:

    zCCollisionObject* m_poCollObject0;
    zCCollisionObject* m_poCollObject1;
};

class zCCollisionDetector {
public:

    void DetectCollisions(zCVob::zTCollisionContext& context, zCArray<zCCollisionReport*>& reports);
};

class zCCollisionObjectClass {
public:

    zCCollisionObject* m_createNewInstance();

    unsigned char m_bIsVolatile : 1;
};

#endif
