#include "include/zFft.h"
#include "include/zMath.h"
#include "include/zTools.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

static int fftUtilityState = -1;
static int fftState = -1;
static int randomGaussState = -1;
zCFFT zCFFT::s_fft;
zCFFT zCFFT::s_fftSlow;
zCFFT zCFFT::s_fftFast;

float Alg_ArcTan2Approx(float real, float img)
{
    const float absImg = fabs(img) + 1e-10f;
    float angle;
    if (real >= 0.0f)
        angle = 0.785398185f - (real - absImg) / (absImg + real) * 0.785398185f;
    else
        angle = 2.3561945f - (real + absImg) / (absImg - real) * 0.785398185f;
    return img < 0.0f ? -angle : angle;
}

float zComplex::Mag()
{
    if (mag == 0.0f) {
        const float square = real * real + img * img;
        mag = square == 0.0f ? 0.0f : Alg_SqrtInvApprox(square) * square;
    }
    return mag;
}

float zComplex::Phase()
{
    if (phase == 0.0f)
        phase = Alg_ArcTan2Approx(real, img);
    return phase;
}

float zCFFT::S_CalcWave2D(float x, float y, float scale, zTFFT mode)
{
    zCFFT* fft = &s_fft;
    if (mode == zTFFT_SLOW)
        fft = &s_fftSlow;
    else if (mode == zTFFT_FAST)
        fft = &s_fftFast;
    return fft->CalcWave2D(x, y, scale);
}

void zCFFT::S_Init()
{
    s_fft.fftSpeed = 0.5f;
    s_fftSlow.fftSpeed = 0.2f;
    s_fftFast.fftSpeed = 2.0f;
    zVEC3 direction(1.0f, 0.0f, 0.0f);
    s_fft.m_h0Map = s_fft.CreateArray();
    s_fft.m_IWaveMap = s_fft.CreateArray();
    s_fft.m_WaveMap = s_fft.CreateArray();
    s_fft.FillH0Array(1.0f, direction);
    s_fftSlow.m_h0Map = s_fftSlow.CreateArray();
    s_fftSlow.m_IWaveMap = s_fftSlow.CreateArray();
    s_fftSlow.m_WaveMap = s_fftSlow.CreateArray();
    s_fftSlow.FillH0Array(1.0f, direction);
    s_fftFast.m_h0Map = s_fftFast.CreateArray();
    s_fftFast.m_IWaveMap = s_fftFast.CreateArray();
    s_fftFast.m_WaveMap = s_fftFast.CreateArray();
    s_fftFast.FillH0Array(1.0f, direction);
}

void zCFFT::S_SetFrameCtr(int frame)
{
    s_fft.frameCtr = frame;
    s_fftSlow.frameCtr = frame;
    s_fftFast.frameCtr = frame;
}

void zCFFT::S_AnimateWaveMap(float time)
{
    s_fftSlow.AnimateWaveMap(time * 0.2f);
    s_fft.AnimateWaveMap(time * 0.5f);
    s_fftFast.AnimateWaveMap(time * 2.0f);
}

zCFFT::zCFFT()
    : m_h0Map(NULL), m_WaveMap(NULL), m_IWaveMap(NULL), fftSpeed(1.0f), frameCtr(0),
      frameCtrLastUsed(-1)
{
}

zCFFT::~zCFFT()
{
    FreeArray(m_h0Map);
    FreeArray(m_WaveMap);
    FreeArray(m_IWaveMap);
}

float zCFFT::CalcWave2D(float x, float y, float scale)
{
    if (frameCtr != frameCtrLastUsed) {
        AnimateWaveMap(ztimer.totalTimeFloat * 0.001f);
        frameCtrLastUsed = frameCtr;
    }
    static float lastScale = 0.0f;
    static float inverseScale = 0.0f;
    if (scale != lastScale) {
        lastScale = scale;
        inverseScale = 1.0f / scale;
    }
    const int mapX = static_cast<int>(x * inverseScale + 0.5f) & 31;
    const int mapY = static_cast<int>(y * inverseScale + 0.5f) & 31;
    zComplex value = m_IWaveMap[mapX][mapY];
    return zSinApprox(value.Mag() * 10000000.0f + value.Phase());
}

int zCFFT::IsPowerOfTwo(unsigned int value)
{
    return value >= 2 && (value & (value - 1)) == 0;
}

unsigned int zCFFT::NumberOfBitsNeeded(unsigned int value)
{
    if (value < 2) {
        fprintf(stderr,
            ">>> Error in fftmisc.c: argument %d to NumberOfBitsNeeded is too small.\n", value);
        exit(1);
    }
    unsigned int result = 0;
    while ((value & (1U << result)) == 0)
        ++result;
    return result;
}

unsigned int zCFFT::ReverseBits(unsigned int value, unsigned int bits)
{
    unsigned int result = 0;
    for (unsigned int i = 0; i < bits; ++i) {
        result = (result << 1) | (value & 1);
        value >>= 1;
    }
    return result;
}

float zCFFT::Index_to_frequency(unsigned int samples, unsigned int index)
{
    if (index >= samples)
        return 0.0f;
    if (index > samples / 2)
        return -static_cast<float>(samples - index) / samples;
    return static_cast<float>(index) / samples;
}

void zCFFT::CreateWaveMap(float scale, zVEC3& direction)
{
    m_h0Map = CreateArray();
    m_IWaveMap = CreateArray();
    m_WaveMap = CreateArray();
    FillH0Array(scale, direction);
}

void zCFFT::FillH0Array(float scale, zVEC3& direction)
{
    zRandom_Gauss random(0, 100);
    const float directionLength2 = direction.Length2();
    const float windFactor = directionLength2 * 0.101936795f;
    const float inverseLength = 1.0f / direction.Length();
    direction *= inverseLength;
    for (int x = 0; x < 32; ++x) {
        const float waveX = (x - 16) * 0.19634954084936207f;
        for (int y = 0; y < 32; ++y) {
            const float waveY = (y - 16) * 0.19634954084936207f;
            const float waveLength2 = waveX * waveX + waveY * waveY + 0.0001f;
            const float waveLength = sqrt(waveLength2);
            const float dot = (direction.n[0] * waveX + direction.n[1] * waveY) / waveLength;
            const float damping = exp(-1.0f / (waveLength2 * windFactor * windFactor));
            const float spectrum = exp(-waveLength2 * directionLength2) * damping * dot * dot
                / (waveLength2 * waveLength2);
            const float amplitude = sqrt(spectrum * scale);
            m_h0Map[x][y].real = random.GetGaussian(0.0f, 1.0f) * amplitude / 1.41421354f;
            m_h0Map[x][y].img = 0.0f;
        }
    }
}

void zCFFT::AnimateWaveMap(float time)
{
    time *= fftSpeed;
    for (int x = 0; x < 32; ++x) {
        const float waveX = fabs((x - 16) * 0.19634954084936207f);
        for (int y = 0; y < 32; ++y) {
            const float waveY = fabs((y - 16) * 0.19634954084936207f);
            const float frequency = sqrt(9.81000042f * sqrt(waveX * waveX + waveY * waveY));
            const float angle = -frequency * time;
            const float sine = sin(angle);
            const float cosine = cos(angle);
            const zComplex& initial = m_h0Map[x][y];
            const float conjugateImg = -initial.img;
            m_WaveMap[x][y].real = initial.real * cosine - conjugateImg * sine;
            m_WaveMap[x][y].img = initial.real * sine + conjugateImg * cosine;
            m_WaveMap[x][y].EmptyCache();
        }
        fft_float(32, 1, m_WaveMap[x], m_IWaveMap[x]);
    }
}

void zCFFT::fft_float(unsigned int samples, int inverse, zComplex* input, zComplex* output)
{
    if (!IsPowerOfTwo(samples) || !input || !output)
        return;
    const unsigned int bits = NumberOfBitsNeeded(samples);
    for (unsigned int i = 0; i < samples; ++i) {
        const unsigned int reversed = ReverseBits(i, bits);
        output[reversed].real = input[i].real;
        output[reversed].img = input[i].img;
        output[reversed].EmptyCache();
    }
    const float direction = inverse ? -6.28318548f : 6.28318548f;
    for (unsigned int blockSize = 2; blockSize <= samples; blockSize <<= 1) {
        const float angle = direction / blockSize;
        const float stepReal = cos(angle);
        const float stepImg = sin(angle);
        for (unsigned int block = 0; block < samples; block += blockSize) {
            float twiddleReal = 1.0f;
            float twiddleImg = 0.0f;
            for (unsigned int j = 0; j < blockSize / 2; ++j) {
                const unsigned int first = block + j;
                const unsigned int second = first + blockSize / 2;
                const float productReal = twiddleReal * output[second].real
                    - twiddleImg * output[second].img;
                const float productImg = twiddleImg * output[second].real
                    + twiddleReal * output[second].img;
                output[second].real = output[first].real - productReal;
                output[second].img = output[first].img - productImg;
                output[first].real += productReal;
                output[first].img += productImg;
                const float nextReal = twiddleReal * stepReal - twiddleImg * stepImg;
                twiddleImg = twiddleReal * stepImg + twiddleImg * stepReal;
                twiddleReal = nextReal;
            }
        }
    }
    if (inverse) {
        const float scale = 1.0f / samples;
        for (unsigned int i = 0; i < samples; ++i) {
            output[i].real *= scale;
            output[i].img *= scale;
        }
    }
}

void zCFFT::SetFrameCtr(int frame)
{
    frameCtr = frame;
}

void zCFFT::SetSpeed(float speed)
{
    fftSpeed = speed;
}

zComplex& zCFFT::GetWaveHeight(int x, int y)
{
    if (frameCtr != frameCtrLastUsed) {
        AnimateWaveMap(ztimer.totalTimeFloat * 0.001f);
        frameCtrLastUsed = frameCtr;
    }
    return m_IWaveMap[x & 31][y & 31];
}

zComplex& zCFFT::GetWaveHeight(float x, float y, float scale)
{
    const float inverseScale = 1.0f / scale;
    return GetWaveHeight(static_cast<int>(x * inverseScale + 0.5f),
        static_cast<int>(y * inverseScale + 0.5f));
}

zComplex& zCFFT::GetWaveHeightSmooth(float x, float y, float scale)
{
    if (frameCtr != frameCtrLastUsed) {
        AnimateWaveMap(ztimer.totalTimeFloat * 0.001f);
        frameCtrLastUsed = frameCtr;
    }
    static zComplex result;
    const float mapX = (x + 16.0f) / scale;
    const float mapY = (y + 16.0f) / scale;
    const int x0 = static_cast<int>(mapX);
    const int y0 = static_cast<int>(mapY);
    const float fractionX = mapX - x0;
    const float fractionY = mapY - y0;
    const float weightXY = fractionX * fractionY;
    const float weight00 = 1.0f - fractionX - fractionY + weightXY;
    const float weight10 = fractionX - weightXY;
    const float weight01 = fractionY - weightXY;
    const zComplex& value00 = m_IWaveMap[x0 & 31][y0 & 31];
    const zComplex& value10 = m_IWaveMap[(x0 + 1) & 31][y0 & 31];
    const zComplex& value01 = m_IWaveMap[x0 & 31][(y0 + 1) & 31];
    const zComplex& value11 = m_IWaveMap[(x0 + 1) & 31][(y0 + 1) & 31];
    result.real = value00.real * weight00 + value10.real * weight10
        + value01.real * weight01 + value11.real * weightXY;
    result.img = value00.img * weight00 + value10.img * weight10
        + value01.img * weight01 + value11.img * weightXY;
    result.EmptyCache();
    return result;
}

zComplex** zCFFT::CreateArray()
{
    zComplex** array = new zComplex*[32];
    if (!array)
        return NULL;
    for (int i = 0; i < 32; ++i) {
        array[i] = new zComplex[32];
        if (!array[i]) {
            for (int j = 0; j < 32; ++j)
                delete[] array[j];
            delete[] array;
            return NULL;
        }
    }
    return array;
}

void zCFFT::FreeArray(zComplex** array)
{
    if (!array)
        return;
    for (int i = 0; i < 32; ++i) {
        delete[] array[i];
        array[i] = NULL;
    }
    delete[] array;
}

zRandom_Gauss::zRandom_Gauss(int seed, int calls)
{
    u = new float[97];
    if (seed < 0 || seed > 31328 || calls < 0 || calls > 30081) {
        seed = 1802;
        calls = 9373;
    }
    int first = seed % 177 + 2;
    int second = seed / 177 % 177 + 2;
    int third = calls % 169;
    int fourth = calls / 169 % 178 + 1;
    for (int i = 0; i < 97; ++i) {
        float value = 0.0f;
        float bitValue = 0.5f;
        for (int bit = 0; bit < 24; ++bit) {
            const int next = static_cast<int>((static_cast<long long>(first) * second) % 179)
                * fourth % 179;
            second = first;
            first = fourth;
            fourth = next;
            third = (third * 53 + 1) % 169;
            int mask = (fourth * third) & 0x8000003F;
            if (mask < 0)
                mask = ((mask - 1) | ~63) + 1;
            if (mask >= 32)
                value += bitValue;
            bitValue *= 0.5f;
        }
        u[i] = value;
    }
    c = 0.021119835f;
    cd = 0.45643207f;
    cm = 0.99999994f;
    i97 = 97;
    j97 = 33;
    m_gaussCall = 0;
}

zRandom_Gauss::~zRandom_Gauss()
{
    delete[] u;
}

float zRandom_Gauss::RandomUniform()
{
    float value = u[i97 - 1] - u[j97 - 1];
    if (value < 0.0f)
        value += 1.0f;
    u[i97 - 1] = value;
    if (--i97 == 0)
        i97 = 97;
    if (--j97 == 0)
        j97 = 97;
    c -= cd;
    if (c < 0.0f)
        c += cm;
    value -= c;
    if (value < 0.0f)
        value += 1.0f;
    return value;
}

float zRandom_Gauss::GetGaussian(float mean, float deviation)
{
    float first;
    float scaled;
    while (true) {
        first = RandomUniform();
        float second = RandomUniform();
        if (first <= 0.0f || second <= 0.0f) {
            first = 1.0f;
            second = 1.0f;
        }
        scaled = (second - 0.5f) * 1.7156f;
        const float shifted = first - 0.449871f;
        const float magnitude = fabs(scaled) + 0.386595f;
        const float acceptance = magnitude * (0.196f * magnitude - shifted * 0.25472f)
            + shifted * shifted;
        if (acceptance > 0.27597f)
            break;
        if (acceptance < 0.27846f && -4.0f * log(first) >= scaled * scaled)
            break;
    }
    ++m_gaussCall;
    return mean + deviation * scaled / first;
}

int zRandom_Gauss::GetInt(int minValue, int maxValue)
{
    return static_cast<int>(RandomUniform() * (maxValue - minValue + 1)) + minValue;
}

float zRandom_Gauss::GetFloat(float minValue, float maxValue)
{
    return RandomUniform() * (maxValue - minValue) + minValue;
}

void zRandom_Gauss::StepForward(long steps)
{
    for (long step = 0; step < steps; ++step) {
        float value = u[i97 - 1] - u[j97 - 1];
        if (value < 0.0f)
            value += 1.0f;
        u[i97 - 1] = value;
        if (--i97 == 0)
            i97 = 97;
        if (--j97 == 0)
            j97 = 97;
        c -= cd;
        if (c < 0.0f)
            c += cm;
    }
}

float zRandom_Gauss::UnRandomUniform()
{
    if (c >= cm)
        c -= cm;
    c += cd;
    if (j97 == 97)
        j97 = 0;
    ++j97;
    if (i97 == 97)
        i97 = 0;
    ++i97;
    const float value = u[i97 - 1];
    float restored = value + u[j97 - 1];
    if (restored < 0.0f || restored >= 1.0f)
        restored = value - 1.0f + u[j97 - 1];
    u[i97 - 1] = restored;
    float carry = c - cd;
    if (carry < 0.0f)
        carry += cm;
    float result = value - carry;
    if (result < 0.0f)
        result += 1.0f;
    return result;
}

void zRandom_Gauss::StepBack(long steps)
{
    for (long step = 0; step < steps; ++step) {
        if (c >= cm)
            c -= cm;
        c += cd;
        if (j97 == 97)
            j97 = 0;
        ++j97;
        if (i97 == 97)
            i97 = 0;
        ++i97;
        const float value = u[i97 - 1];
        float restored = value + u[j97 - 1];
        if (restored < 0.0f || restored >= 1.0f)
            restored = value - 1.0f + u[j97 - 1];
        u[i97 - 1] = restored;
    }
}

unsigned int zRandom_Gauss::rand24()
{
    return static_cast<unsigned int>(RandomUniform() * 16777216.0f);
}

unsigned int zRandom_Gauss::unRand24()
{
    return static_cast<unsigned int>(UnRandomUniform() * 16777216.0f);
}

float zRandom_Gauss::GetGaussAt(long position)
{
    const long difference = m_gaussCall - position;
    if (difference > 0)
        StepForward(difference);
    else
        StepBack(difference < 0 ? -difference : difference);
    return GetGaussian(0.0f, 1.0f);
}
