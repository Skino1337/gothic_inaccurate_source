#include "include/zKbSplines.h"
#include "include/zMathTools.h"
#include "include/zRenderer.h"
#include "include/zSession.h"
#include <math.h>

void zCKBSpline::Initialize(zCArray<zCPositionKey*>& keys, int deleteKeys)
{
    deleteKeysWhenDone = deleteKeys;
    int numKeys = keys.numInArray;
    keyList.AllocAbs(numKeys);
    keyList.numInArray = numKeys;

    for (int i = 0; i < numKeys; i++)
        keyList.parray[i] = keys.parray[i];

    if (numKeys == 2) {
        keyList.InsertAtPos(keys.parray[0], 0);
        keyList.InsertEnd(keys.parray[keys.numInArray - 1]);
        numKeys = 4;
    }
    else if (numKeys == 3) {
        keyList.InsertEnd(keys.parray[keys.numInArray - 1]);
        numKeys = 4;
    }

    numPolynomials = numKeys - 3;
    polynom = new zCCubicPolynomial[numPolynomials];

    for (int i = 0; i < numPolynomials; i++) {
        zCPositionKey* key0 = keys.parray[i];
        zCPositionKey* key1 = keys.parray[i + 1];
        zCPositionKey* key2 = keys.parray[i + 2];
        zCPositionKey* key3 = keys.parray[i + 3];
        zVEC3 delta = key2->p - key1->p;

        float scale0 = 2.0f * (key1->t - key0->t) / (key2->t - key0->t);
        float incoming0 = 0.5f * scale0 * (1.0f - key1->tension) *
            (1.0f - key1->continuity) * (1.0f - key1->bias);
        float outgoing0 = 0.5f * scale0 * (1.0f - key1->tension) *
            (1.0f + key1->continuity) * (1.0f + key1->bias);
        zVEC3 tangent0 = (key1->p - key0->p) * incoming0 + delta * outgoing0;

        float scale1 = 2.0f * (key2->t - key1->t) / (key3->t - key1->t);
        float incoming1 = 0.5f * scale1 * (1.0f - key2->tension) *
            (1.0f - key2->continuity) * (1.0f + key2->bias);
        float outgoing1 = 0.5f * scale1 * (1.0f - key2->tension) *
            (1.0f + key2->continuity) * (1.0f - key2->bias);
        zVEC3 tangent1 = delta * incoming1 + (key3->p - key2->p) * outgoing1;

        polynom[i].c0 = key1->p;
        polynom[i].c1 = tangent0;
        polynom[i].c2 = delta * 3.0f - tangent0 * 2.0f - tangent1;
        polynom[i].c3 = delta * -2.0f + tangent0 + tangent1;
        polynom[i].tmin = key1->t;
        polynom[i].tmax = key2->t;
        polynom[i].trange = key2->t - key1->t;
    }

    length = new float[numPolynomials + 1];
    length[0] = 0.0f;

    for (int i = 0; i < numPolynomials; i++) {
        float segmentLength = 0.0f;
        zVEC3 previous = polynom[i].c0;

        for (int j = 1; j <= 20; j++) {
            zVEC3 current;
            polynom[i].Interpolate0(float(j) / 20.0f, current);
            segmentLength += (current - previous).Length();
            previous = current;
        }

        length[i + 1] = length[i] + segmentLength;
    }

    totalLength = length[numPolynomials];
    delete[] keys.parray;
    keys.parray = NULL;
    keys.numAlloc = 0;
    keys.numInArray = 0;
}

zCCubicPolynomial::zCCubicPolynomial()
{
}

zCCubicPolynomial::~zCCubicPolynomial()
{
}

void zCKBSpline::Changed()
{
    delete[] polynom;
    delete[] length;
    Initialize(keyList, 1);
}

void zCKBSpline::Terminate()
{
    delete[] polynom;
    polynom = NULL;
    delete[] length;
    length = NULL;

    if (deleteKeysWhenDone) {
        for (int i = keyList.numInArray - 1; i >= 0; i--)
            delete keyList.parray[i];
    }

    delete[] keyList.parray;
    keyList.parray = NULL;
    keyList.numAlloc = 0;
    keyList.numInArray = 0;
}

void zCCubicPolynomial::Interpolate1(float time, zVEC3& result)
{
    if (time <= 0.0f)
        time = 0.0f;
    else if (time >= 1.0f)
        time = 1.0f;

    result = c1 + c2 * (2.0f * time) + c3 * (3.0f * time * time);
}

void zCKBSpline::ComputeIAndU(float time, int& index, float& fraction)
{
    if (time <= polynom[0].tmin) {
        index = 0;
        fraction = 0.0f;
        return;
    }

    if (time >= polynom[numPolynomials - 1].tmax) {
        index = numPolynomials - 1;
        fraction = 1.0f;
        return;
    }

    index = 0;
    while (index < numPolynomials && time > polynom[index].tmax)
        index++;

    fraction = (time - polynom[index].tmin) / polynom[index].trange;
}

void zCKBSpline::Interpolate0(float time, zVEC3& result)
{
    int index;
    float fraction;
    ComputeIAndU(time, index, fraction);
    lastKey = int(fraction * 3.0f) + index;

    if (fraction <= 0.0f)
        fraction = 0.0f;
    else if (fraction >= 1.0f)
        fraction = 1.0f;

    zCCubicPolynomial& polynomial = polynom[index];
    result = ((polynomial.c3 * fraction + polynomial.c2) * fraction + polynomial.c1) * fraction + polynomial.c0;
}

static zCQuat InterpolateKeyRotation(zCArray<zCPositionKey*>& keys, float time)
{
    for (int i = 0; i < keys.numInArray; i++) {
        if (time == keys.parray[i]->t)
            return keys.parray[i]->rot;

        if (time < keys.parray[i]->t) {
            zCPositionKey* first = keys.parray[i - 1];
            zCPositionKey* second = keys.parray[i];
            zCQuat rotation = first->rot;
            rotation.Slerp((time - first->t) / (second->t - first->t), second->rot);
            return rotation;
        }
    }

    return keys.parray[keys.numInArray - 1]->rot;
}

void zCKBSpline::Interpolate0(float time, zMAT4& matrix, int mode)
{
    if (time <= 0.0f)
        time = 0.0f;
    else if (time >= polynom[numPolynomials - 1].tmax)
        time = polynom[numPolynomials - 1].tmax;

    int index;
    float fraction;
    ComputeIAndU(time, index, fraction);
    lastKey = int(fraction * 3.0f) + index;
    zVEC3 position;
    polynom[index].Interpolate0(fraction, position);

    if (mode == zTSPL_LERP_PATH_ROTvoidS) {
        zCQuat rotation = InterpolateKeyRotation(keyList, time);
        rotation.QuatToMatrix4(matrix);
        matrix.SetTranslation(position);
        return;
    }

    zVEC3 heading;
    polynom[index].Interpolate1(fraction, heading);

    if (mode == zTSPL_LERP_PATH_IGNOREROLL)
        heading.n[1] = 0.0f;

    heading.Normalize();
    matrix = GetPoseOfHeading(position, heading);
}

void zCKBSpline::Interpolate1(float time, zMAT4& matrix, int mode)
{
    int index;
    float fraction;
    ComputeIAndU(time, index, fraction);
    zVEC3 position;
    polynom[index].Interpolate1(fraction, position);

    if (mode == zTSPL_LERP_PATH_ROTvoidS) {
        zCQuat rotation = InterpolateKeyRotation(keyList, time);
        rotation.QuatToMatrix4(matrix);
        matrix.SetTranslation(position);
        return;
    }

    zVEC3 heading = polynom[index].c2 * 2.0f + polynom[index].c3 * (6.0f * fraction);

    if (mode == zTSPL_LERP_PATH_IGNOREROLL)
        heading.n[1] = 0.0f;

    heading.Normalize();
    matrix = GetPoseOfHeading(position, heading);
}

void zCKBSpline::Interpolate1(float time, zVEC3& result)
{
    int index;
    float fraction;
    ComputeIAndU(time, index, fraction);
    polynom[index].Interpolate1(fraction, result);
}

void zCKBSpline::Interpolate2(float time, zVEC3& result)
{
    int index;
    float fraction;
    ComputeIAndU(time, index, fraction);
    zCCubicPolynomial& polynomial = polynom[index];
    result = polynomial.c2 * 2.0f + polynomial.c3 * (6.0f * fraction);
}

float zCCubicPolynomial::Speed(float time)
{
    zVEC3 derivative;
    Interpolate1(time, derivative);
    return derivative.Length();
}

float zCCubicPolynomial::Length(float time)
{
    float result = 0.0f;

    for (int i = 0; i < 20; i++)
        result += Speed(time * (float(i) + 0.5f) / 20.0f) * (time / 20.0f);

    return result;
}

void zCKBSpline::ComputeArcLength()
{
    length = new float[numPolynomials + 1];
    length[0] = 0.0f;

    for (int i = 0; i < numPolynomials; i++)
        length[i + 1] = length[i] + polynom[i].Length(1.0f);

    totalLength = length[numPolynomials];
}

void zCKBSpline::InvertIntegral(float distance, int& index, float& fraction)
{
    if (distance <= 0.0f) {
        index = 0;
        fraction = 0.0f;
        return;
    }

    if (distance >= totalLength) {
        index = numPolynomials - 1;
        fraction = 1.0f;
        return;
    }

    index = 0;
    while (index < numPolynomials && distance >= length[index + 1])
        index++;

    float targetLength = distance - length[index];
    fraction = targetLength / (length[index + 1] - length[index]);

    for (int iteration = 0; iteration < 32; iteration++) {
        float error = polynom[index].Length(fraction) - targetLength;

        if (fabs(error) < 0.000001f)
            break;

        fraction -= error / polynom[index].Speed(fraction);
    }
}

void zCKBSpline::Interpolate0_AL(float distance, zVEC3& result)
{
    int index;
    float fraction;
    InvertIntegral(distance, index, fraction);
    polynom[index].Interpolate0(fraction, result);
}

void zCKBSpline::Interpolate0_AL(float distance, zMAT4& matrix, int mode)
{
    int index;
    float fraction;
    InvertIntegral(distance, index, fraction);
    zVEC3 position;
    polynom[index].Interpolate0(fraction, position);

    if (mode == zTSPL_LERP_PATH_ROTvoidS) {
        float time = polynom[index].tmin + polynom[index].trange * fraction;
        zCQuat rotation = InterpolateKeyRotation(keyList, time);
        rotation.QuatToMatrix4(matrix);
        matrix.SetTranslation(position);
        return;
    }

    zVEC3 heading;
    polynom[index].Interpolate1(fraction, heading);

    if (mode == zTSPL_LERP_PATH_IGNOREROLL)
        heading.n[1] = 0.0f;

    heading.Normalize();
    matrix = GetPoseOfHeading(position, heading);
}

void zCKBSpline::Interpolate1_AL(float distance, zMAT4& matrix, int mode)
{
    int index;
    float fraction;
    InvertIntegral(distance, index, fraction);
    zVEC3 position;
    polynom[index].Interpolate1(fraction, position);

    if (mode == zTSPL_LERP_PATH_ROTvoidS) {
        float time = polynom[index].tmin + polynom[index].trange * fraction;
        zCQuat rotation = InterpolateKeyRotation(keyList, time);
        rotation.QuatToMatrix4(matrix);
        matrix.SetTranslation(position);
        return;
    }

    zVEC3 heading = polynom[index].c2 * 2.0f + polynom[index].c3 * (6.0f * fraction);

    if (mode == zTSPL_LERP_PATH_IGNOREROLL)
        heading.n[1] = 0.0f;

    heading.Normalize();
    matrix = GetPoseOfHeading(position, heading);
}

void zCKBSpline::Interpolate1_AL(float distance, zVEC3& result)
{
    int index;
    float fraction;
    InvertIntegral(distance, index, fraction);
    polynom[index].Interpolate1(fraction, result);
}

void zCKBSpline::Interpolate2_AL(float distance, zVEC3& result)
{
    int index;
    float fraction;
    InvertIntegral(distance, index, fraction);
    result = polynom[index].c2 * 2.0f + polynom[index].c3 * (6.0f * fraction);
}

float zCKBSpline::Length(float time)
{
    int index;
    float fraction;
    ComputeIAndU(time, index, fraction);
    return length[index] + polynom[index].Length(fraction);
}

static void DrawSplineLine(const zVEC3& start, const zVEC3& end, zCOLOR color)
{
    zVEC3 first = zCCamera::activeCam->camMatrix * start;
    zVEC3 second = zCCamera::activeCam->camMatrix * end;

    if (first.n[2] < 1.0f && second.n[2] < 1.0f)
        return;
    if (first.n[2] < 1.0f)
        first = first + (second - first) * ((1.0f - first.n[2]) / (second.n[2] - first.n[2]));
    if (second.n[2] < 1.0f)
        second = first + (second - first) * ((1.0f - first.n[2]) / (second.n[2] - first.n[2]));

    float firstZ = 1.0f / first.n[2];
    float secondZ = 1.0f / second.n[2];
    int x1 = int(zCCamera::activeCam->vpData.xcenter + first.n[0] *
        zCCamera::activeCam->viewDistanceX * firstZ);
    int y1 = int(zCCamera::activeCam->vpData.ycenter - first.n[1] *
        zCCamera::activeCam->viewDistanceY * firstZ);
    int x2 = int(zCCamera::activeCam->vpData.xcenter + second.n[0] *
        zCCamera::activeCam->viewDistanceX * secondZ);
    int y2 = int(zCCamera::activeCam->vpData.ycenter - second.n[1] *
        zCCamera::activeCam->viewDistanceY * secondZ);

    if (screen->ClipLine(x1, y1, x2, y2))
        zrenderer->DrawLineZ(float(x1), float(y1), firstZ, float(x2), float(y2), secondZ, color);
}

void zCKBSpline::DrawControlPolygon(zCOLOR color)
{
    if (!numPolynomials || keyList.numInArray < 4)
        return;

    for (int i = 1; i < keyList.numInArray; i++)
        DrawSplineLine(keyList.parray[i - 1]->p, keyList.parray[i]->p, color);
}

void zCKBSpline::Draw(zCOLOR color, float time)
{
    if (!numPolynomials || keyList.numInArray < 4)
        return;

    float start = polynom[0].tmin;
    float end = polynom[numPolynomials - 1].tmax;
    float step = (end - start) / ((time * 0.35f + 0.05f) * totalLength + 1.0f);
    zVEC3 previous;
    Interpolate0(start, previous);

    while (start < end) {
        float next = start + step;
        if (next > end)
            next = end;

        zVEC3 current;
        Interpolate0(next, current);
        DrawSplineLine(previous, current, color);
        previous = current;
        start = next;
    }
}

void zCCubicPolynomial::Interpolate0(float time, zVEC3& result)
{
    if (time <= 0.0f)
        time = 0.0f;
    else if (time >= 1.0f)
        time = 1.0f;

    result = ((c3 * time + c2) * time + c1) * time + c0;
}

float zCKBSpline::GetTime(float distance)
{
    int index;
    float fraction;
    InvertIntegral(distance, index, fraction);
    return polynom[index].tmin + polynom[index].trange * fraction;
}
