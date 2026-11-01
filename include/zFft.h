#ifndef ZFFT_H
#define ZFFT_H

#include "z3d.h"

struct zComplex {
    zComplex() : real(0.0f), img(0.0f), mag(0.0f), phase(0.0f) {}
    zComplex(float realValue, float imgValue)
        : real(realValue), img(imgValue), mag(0.0f), phase(0.0f) {}

    void EmptyCache() { mag = 0.0f; phase = 0.0f; }
    float Mag();
    float Phase();

    float real;
    float img;
    float mag;
    float phase;
};

enum zTFFT {
    zTFFT_NONE,
    zTFFT_SLOW,
    zTFFT_NORMAL,
    zTFFT_FAST
};

class zRandom_Gauss {
public:

    zRandom_Gauss(int seed, int calls);
    ~zRandom_Gauss();
    float RandomUniform();
    float GetGaussian(float mean, float deviation);
    int GetInt(int minValue, int maxValue);
    float GetFloat(float minValue, float maxValue);
    void StepForward(long steps);
    float UnRandomUniform();
    void StepBack(long steps);
    unsigned int rand24();
    unsigned int unRand24();
    float GetGaussAt(long position);

    float* u;
    float c;
    float cd;
    float cm;
    int i97;
    int j97;
    int m_gaussCall;
};

class zCFFT {
public:

    zCFFT();
    virtual ~zCFFT();
    float CalcWave2D(float x, float y, float scale);
    int IsPowerOfTwo(unsigned int value);
    unsigned int NumberOfBitsNeeded(unsigned int value);
    unsigned int ReverseBits(unsigned int value, unsigned int bits);
    float Index_to_frequency(unsigned int samples, unsigned int index);
    void CreateWaveMap(float scale, zVEC3& direction);
    void FillH0Array(float scale, zVEC3& direction);
    void AnimateWaveMap(float time);
    void fft_float(unsigned int samples, int inverse, zComplex* input, zComplex* output);
    void SetFrameCtr(int frame);
    void SetSpeed(float speed);
    zComplex& GetWaveHeight(int x, int y);
    zComplex& GetWaveHeight(float x, float y, float scale);
    zComplex& GetWaveHeightSmooth(float x, float y, float scale);
    zComplex** CreateArray();
    void FreeArray(zComplex** array);

    static float S_CalcWave2D(float x, float y, float scale, zTFFT mode);
    static void S_Init();
    static void S_SetFrameCtr(int frame);
    static void S_AnimateWaveMap(float time);

    zComplex** m_h0Map;
    zComplex** m_WaveMap;
    zComplex** m_IWaveMap;
    float fftSpeed;
    int frameCtr;
    int frameCtrLastUsed;

    static zCFFT s_fft;
    static zCFFT s_fftSlow;
    static zCFFT s_fftFast;
};

float Alg_ArcTan2Approx(float real, float img);

#endif
