#ifndef ZMATHTOOLS_H
#define ZMATHTOOLS_H

#include "zModel.h"

zMAT4& GetPoseOfHeading(const zVEC3& position, const zVEC3& heading);

const int MAX_SAMPLES_POS = 50;
const int MAX_SAMPLES_ROT = 100;

class zCPose {
public:

    zCPose();

    void  SetPose(zMAT4& matrix);
    void  SetInertiaRot(float inertia);
    void  SetInertiaPos(float inertia);
    void  SetInertiaSamplesRot(int samples);
    void  SetInertiaSamplesPos(int samples);
    void  CalcAvgRot();
    void  UpdatePose(zMAT4& matrix);
    void  UpdatePose(zCQuat& rotation, zVEC3& position);
    void  UpdateRot(zMAT4& matrix);
    void  UpdateRot(zCQuat& rotation);
    void  UpdatePos(zVEC3& position);
    void  SetRot(zMAT4& matrix);
    void  SetRot(zCQuat& rotation);
    void  SetPos(zMAT4& matrix);
    void  SetPos(zVEC3& position);
    void  GetPose(zMAT4& matrix);
    zMAT4& GetPose();
    void  GetPoseAbs(zMAT4& matrix);
    zMAT4& GetRotMatAbs();
    zMAT4& GetRotMat();

    zCQuat qRing[MAX_SAMPLES_ROT];
    zVEC3 pRing[MAX_SAMPLES_POS];
    int ringPosTrans;
    int ringPosRot;
    zCQuat actRotAbs;
    zVEC3 actPosAbs;
    zCQuat actRot;
    zVEC3 actPos;
    int oneTurn;
    int numSamplesPos;
    int numSamplesRot;

    static zMAT4& tmpMat;
};

#endif
