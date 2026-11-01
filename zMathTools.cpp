#include "include/zMathTools.h"
#include <math.h>

static zMAT4 poseTmpMat;
zMAT4& zCPose::tmpMat = poseTmpMat;

zMAT4& GetPoseOfHeading(const zVEC3& position, const zVEC3& heading)
{
    zVEC3 at = heading;
    zVEC3 up(0.0f, 1.0f - at.n[1] * at.n[1], -at.n[1] * at.n[2]);
    up.n[0] = -at.n[1] * at.n[0];

    if (up.Length2() <= 0.000001f) {
        up = zVEC3(1.0f - at.n[0] * at.n[0], -at.n[0] * at.n[1], -at.n[0] * at.n[2]);

        if (up.Length2() <= 0.000001f)
            up = zVEC3(-at.n[2] * at.n[0], -at.n[2] * at.n[1], 1.0f - at.n[2] * at.n[2]);
    }

    up.Normalize();
    zVEC3 right = (up ^ at).Normalize();

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++)
            poseTmpMat.m[y][x] = 0.0f;
    }

    poseTmpMat.m[0][0] = right.n[0];
    poseTmpMat.m[0][1] = right.n[1];
    poseTmpMat.m[0][2] = right.n[2];
    poseTmpMat.m[1][0] = at.n[0];
    poseTmpMat.m[1][1] = at.n[1];
    poseTmpMat.m[1][2] = at.n[2];
    poseTmpMat.m[2][0] = up.n[0];
    poseTmpMat.m[2][1] = up.n[1];
    poseTmpMat.m[2][2] = up.n[2];
    poseTmpMat.SetTranslation(position);
    return poseTmpMat;
}

void zCPose::SetInertiaRot(float inertia)
{
    SetInertiaSamplesRot(int(inertia * 99.0f + 1.0f));
}

void zCPose::SetInertiaPos(float inertia)
{
    SetInertiaSamplesPos(int(inertia * 49.0f + 1.0f));
}

void zCPose::SetInertiaSamplesRot(int samples)
{
    numSamplesRot = samples;

    for (int i = 0; i < numSamplesRot; i++)
        qRing[i] = actRot;

    ringPosRot = 0;
}

void zCPose::SetInertiaSamplesPos(int samples)
{
    numSamplesPos = samples;

    for (int i = 0; i < numSamplesPos; i++)
        pRing[i] = actPos;

    ringPosTrans = 0;
}

void zCPose::CalcAvgRot()
{
    int index = ringPosRot != numSamplesRot - 1 ? ringPosRot + 1 : 0;
    zCQuat average;
    if (oneTurn && (numSamplesRot == 4 || numSamplesRot == 16 || numSamplesRot == 64))
    {
        zCArray<zCQuat> ordered;
        for (int i = 0; i < numSamplesRot; ++i)
        {
            ordered.InsertEnd(qRing[index]);
            index = (index + 1) % numSamplesRot;
        }
        int count = numSamplesRot;
        while (count >= 4)
        {
            for (int i = 0; i < count; i += 4)
                ordered[i / 4].Squad(0.5f, ordered[i], ordered[i + 1],
                    ordered[i + 2], ordered[i + 3]);
            count /= 4;
        }
        average = ordered[0];
    }
    else
    {
        average = qRing[index];
        for (int i = 1; i < numSamplesRot; i++) {
            index = (index + 1) % numSamplesRot;
            average.Slerp(0.5f, qRing[index]);
        }
    }
    average.Unit();
    actRot = average;
}

zCPose::zCPose()
{
    numSamplesPos = 1;
    numSamplesRot = 1;
    actPos = zVEC3(0.0f, 0.0f, 0.0f);
    ringPosRot = 0;
    ringPosTrans = 0;
    actRot.q[0] = 0.0f;
    actRot.q[1] = 0.0f;
    actRot.q[2] = 0.0f;
    actRot.q[3] = 1.0f;
    actPosAbs = zVEC3(0.0f, 0.0f, 0.0f);
    oneTurn = 0;
}

void zCPose::UpdatePose(zMAT4& matrix)
{
    qRing[ringPosRot].Matrix4ToQuat(matrix);
    actRotAbs = qRing[ringPosRot];
    ringPosRot++;

    if (ringPosRot == numSamplesRot)
        ringPosRot = 0;

    pRing[ringPosTrans] = matrix.GetTranslation();
    ringPosTrans++;

    if (ringPosTrans == numSamplesPos)
        ringPosTrans = 0;

    zVEC3 sum(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < numSamplesPos; i++)
        sum += pRing[i];

    CalcAvgRot();
    actPos = sum / float(numSamplesPos);
    actPosAbs = matrix.GetTranslation();
}

void zCPose::UpdatePose(zCQuat& rotation, zVEC3& position)
{
    qRing[ringPosRot] = rotation;
    ringPosRot++;

    if (ringPosRot == numSamplesRot)
        ringPosRot = 0;

    pRing[ringPosTrans] = position;
    ringPosTrans++;

    if (ringPosTrans == numSamplesPos)
        ringPosTrans = 0;

    zVEC3 sum(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < numSamplesPos; i++)
        sum += pRing[i];

    CalcAvgRot();
    actPos = sum.Normalize();
    actPosAbs = position;
    actRotAbs = rotation;
}

void zCPose::UpdateRot(zMAT4& matrix)
{
    qRing[ringPosRot].Matrix4ToQuat(matrix);
    actRotAbs = qRing[ringPosRot];
    ringPosRot++;

    if (ringPosRot == numSamplesRot) {
        oneTurn = 1;
        ringPosRot = 0;
    }

    CalcAvgRot();
}

void zCPose::UpdateRot(zCQuat& rotation)
{
    qRing[ringPosRot] = rotation;
    ringPosRot++;

    if (ringPosRot == numSamplesRot) {
        oneTurn = 1;
        ringPosRot = 0;
    }

    CalcAvgRot();
    actRotAbs = rotation;
}

void zCPose::UpdatePos(zVEC3& position)
{
    pRing[ringPosTrans] = position;
    ringPosTrans++;

    if (ringPosTrans == numSamplesPos)
        ringPosTrans = 0;

    zVEC3 sum(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < numSamplesPos; i++)
        sum += pRing[i];

    actPos = sum.Normalize();
    actPosAbs = position;
}

void zCPose::SetPose(zMAT4& matrix)
{
    SetRot(matrix);
    actPosAbs = matrix.GetTranslation();
    actPos = actPosAbs;
}

void zCPose::SetRot(zMAT4& matrix)
{
    zCQuat rotation;
    rotation.Matrix4ToQuat(matrix);
    SetRot(rotation);
}

void zCPose::SetRot(zCQuat& rotation)
{
    for (int i = 0; i < numSamplesRot; i++)
        qRing[i] = rotation;

    actRot = rotation;
}

void zCPose::SetPos(zMAT4& matrix)
{
    zVEC3 position = matrix.GetTranslation();
    SetPos(position);
}

void zCPose::SetPos(zVEC3& position)
{
    for (int i = 0; i < numSamplesPos; i++)
        pRing[i] = position;

    actPos = position;
}

void zCPose::GetPose(zMAT4& matrix)
{
    matrix = zMAT4::s_identity;
    actRot.QuatToMatrix4(matrix);
    matrix.SetTranslation(actPos);
}

zMAT4& zCPose::GetPose()
{
    GetPose(tmpMat);
    return tmpMat;
}

void zCPose::GetPoseAbs(zMAT4& matrix)
{
    matrix = zMAT4::s_identity;
    actRotAbs.QuatToMatrix4(matrix);
    matrix.SetTranslation(actPos);
}

zMAT4& zCPose::GetRotMatAbs()
{
    tmpMat = zMAT4::s_identity;
    actRotAbs.QuatToMatrix4(tmpMat);
    return tmpMat;
}

zMAT4& zCPose::GetRotMat()
{
    tmpMat = zMAT4::s_identity;
    actRot.QuatToMatrix4(tmpMat);
    return tmpMat;
}
