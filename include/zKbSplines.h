#ifndef ZKB_SPLINES_H
#define ZKB_SPLINES_H

#include "zModel.h"

class zCPositionKey {
public:

    void SetMat(const zMAT4& matrix);
    zMAT4 GetMat();

    float t;
    zVEC3 p;
    zCQuat rot;
    float tension;
    float continuity;
    float bias;
};

enum zTSplLerpMode {
    zTSPL_LERP_UNDEF,
    zTSPL_LERP_PATH,
    zTSPL_LERP_PATH_IGNOREROLL,
    zTSPL_LERP_PATH_ROTvoidS
};

class zCCubicPolynomial {
public:

    zCCubicPolynomial();
    ~zCCubicPolynomial();

    void  Interpolate1(float time, zVEC3& result);
    float Speed(float time);
    float Length(float time);
    void  Interpolate0(float time, zVEC3& result);

    zVEC3 c0;
    zVEC3 c1;
    zVEC3 c2;
    zVEC3 c3;
    float tmin;
    float tmax;
    float trange;
};

class zCKBSpline {
public:

    ~zCKBSpline();
    void  InitVars();
    void  Initialize(zCArray<zCPositionKey*>& keys, int deleteKeys);
    void  Changed();
    void  Terminate();
    void  ComputeIAndU(float time, int& index, float& fraction);
    void  Interpolate0(float time, zVEC3& result);
    void  Interpolate0(float time, zMAT4& matrix, int mode);
    void  Interpolate1(float time, zMAT4& matrix, int mode);
    void  Interpolate1(float time, zVEC3& result);
    void  Interpolate2(float time, zVEC3& result);
    void  ComputeArcLength();
    void  InvertIntegral(float time, int& index, float& fraction);
    void  Interpolate0_AL(float time, zVEC3& result);
    void  Interpolate0_AL(float time, zMAT4& matrix, int mode);
    void  Interpolate1_AL(float time, zMAT4& matrix, int mode);
    void  Interpolate1_AL(float time, zVEC3& result);
    void  Interpolate2_AL(float time, zVEC3& result);
    float Length(float time);
    void  DrawControlPolygon(zCOLOR color);
    void  InterpolateN(float time, zMAT4& matrix, int mode);
    void  Draw(zCOLOR color, float time);
    float GetTime(float time);

    zCCubicPolynomial* polynom;
    int numPolynomials;
    float* length;
    float totalLength;
    int lastKey;
    zCArray<zCPositionKey*> keyList;
    int deleteKeysWhenDone;
};

#endif
