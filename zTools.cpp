#include "include/zTools.h"
#include <float.h>

static int timerState = -1;
static int timerRuntimeState = -1;
unsigned long zCTimer::s_forcedMaxFrameTime = 100;
zCTimer ztimer;
zCFPUControler zfpuControler;

zCTimer::zCTimer()
    : factorMotion(1.0f), frameTimeFloat(1.0f), totalTimeFloat(0),
      frameTimeFloatSecs(0.001f), totalTimeFloatSecs(0), lastTimer(0),
      frameTime(1), totalTime(0), minFrameTime(0), forcedMaxFrameTime(s_forcedMaxFrameTime)
{
}

void zCTimer::ResetTimer()
{
    static unsigned long currentTime;
    currentTime += minFrameTime ? minFrameTime : 1;
    unsigned long elapsed = currentTime - lastTimer;
    if (elapsed > forcedMaxFrameTime)
        elapsed = forcedMaxFrameTime;
    frameTime = static_cast<unsigned long>(elapsed * factorMotion);
    if (frameTime > 100)
        frameTime = 100;
    lastTimer = currentTime;
    totalTime += frameTime;
    frameTimeFloat = static_cast<float>(frameTime);
    totalTimeFloat += frameTimeFloat;
    frameTimeFloatSecs = frameTimeFloat * 0.001f;
    totalTimeFloatSecs = totalTimeFloat * 0.001f;
}

void zCTimer::SetFrameTime(float value)
{
    lastTimer += static_cast<unsigned long>(value);
    frameTimeFloat = value;
    frameTimeFloatSecs = value * 0.001f;
    totalTimeFloat += value;
    totalTimeFloatSecs = totalTimeFloat * 0.001f;
    totalTime += static_cast<unsigned long>(frameTime * factorMotion);
}

void zCTimer::S_SetMaxFPS(unsigned long maxFPS)
{
    s_forcedMaxFrameTime = maxFPS;
}

unsigned long zCTimer::S_GetMaxFPS()
{
    return s_forcedMaxFrameTime;
}

void zCTimer::SetMaxFPS(unsigned long maxFPS)
{
    forcedMaxFrameTime = maxFPS;
}

void zCTimer::FrameUpdate()
{
    static int initialized;
    static unsigned long smoothedTime;
    static unsigned long updateCounter;
    if (!initialized) {
        initialized = 1;
        smoothedTime = sysGetTime();
        return;
    }

    const unsigned long currentTime = sysGetTime();
    smoothedTime = (smoothedTime * 3 + currentTime) >> 2;
    if (currentTime > smoothedTime)
        smoothedTime = currentTime;
    if (currentTime < smoothedTime - 400)
        smoothedTime = currentTime - 400;

    ++updateCounter;
    if (updateCounter > 10)
        updateCounter = 10;
    else if (updateCounter < 3)
        updateCounter = 3;
}

void zCTimer::LimitFPS(int maxFPS)
{
    minFrameTime = maxFPS > 0 ? 8000 / maxFPS : 0;
}

void zCTimer::SetMotionFactor(float factor)
{
    factorMotion = factor;
}

void zCChecksum::BeginChecksum()
{
    checksum = 0;
}

unsigned long zCChecksum::EndChecksum()
{
    return checksum;
}

unsigned long zCChecksum::GetBufferCRC32(unsigned char* buffer, unsigned long size,
    unsigned long seed)
{
    if (!buffer)
        return seed;
    unsigned long crc = ~seed;
    for (unsigned long i = 0; i < size; ++i) {
        crc ^= buffer[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
    }
    return ~crc;
}

void zCChecksum::CalcBufferChecksum(unsigned char* buffer, unsigned long size)
{
    checksum = GetBufferCRC32(buffer, size, checksum);
}

void zCChecksum::CalcFileChecksum(zFILE* file)
{
    const long size = file->Size();
    if (size <= 0)
        return;
    unsigned char* buffer = new unsigned char[size];
    file->Read(buffer, size);
    checksum = GetBufferCRC32(buffer, size, checksum);
    delete[] buffer;
}

void zCChecksum::CalcFileChecksum(const zSTRING& fileName)
{
    zFILE* file = zfactory->CreateZFile(fileName);
    file->Open(0);
    CalcFileChecksum(file);
    file->Close();
    delete file;
}

void zCChecksum16::BeginChecksum()
{
    checksum = 0;
}

unsigned short zCChecksum16::EndChecksum()
{
    return checksum;
}

unsigned short zCChecksum16::GetBufferCRC16(unsigned char* buffer, unsigned long size,
    unsigned short seed)
{
    if (!buffer)
        return seed;
    unsigned short crc = seed;
    for (unsigned long i = 0; i < size; ++i) {
        crc ^= buffer[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xA001 & -(crc & 1));
    }
    return crc;
}

void zCChecksum16::CalcBufferChecksum(unsigned char* buffer, unsigned long size)
{
    checksum = GetBufferCRC16(buffer, size, checksum);
}

static unsigned long fpuCurrentControlWord = 0x9001F;

zCFPUControler::zCFPUControler()
    : savedControlWord(_controlfp(0, 0))
{
    _controlfp(0x9001F, 0xFFFFFFFF);
    defaultControlWord = (_controlfp(0, 0) & 0xFFF4FFE0) ^ 0xA001F;
    _controlfp(defaultControlWord, 0xFFFFFFFF);
    fpuCurrentControlWord = defaultControlWord;
}

void zCFPUControler::SaveCurrentControlWord()
{
    savedControlWord = _controlfp(0, 0);
}

void zCFPUControler::RestoreSavedControlWord()
{
    fpuCurrentControlWord = savedControlWord;
}

unsigned long zCFPUControler::GetCurrentControlWord()
{
    return fpuCurrentControlWord;
}

void zCFPUControler::RestoreDefaultControlWord()
{
    fpuCurrentControlWord = defaultControlWord;
}

void zCFPUControler::ComputeDefaultControlWord()
{
    fpuCurrentControlWord = 0x9001F;
    defaultControlWord = (fpuCurrentControlWord & 0xFFF4FFE0) ^ 0xA001F;
    fpuCurrentControlWord = defaultControlWord;
}

void zCFPUControler::SetControlWord(unsigned long value)
{
    fpuCurrentControlWord = value;
}

void zCFPUControler::SetPrecision_24()
{
    fpuCurrentControlWord &= ~0x300;
}

void zCFPUControler::SetPrecision_53()
{
    fpuCurrentControlWord = (fpuCurrentControlWord & ~0x300) | 0x200;
}

void zCFPUControler::SetPrecision_64()
{
    fpuCurrentControlWord |= 0x300;
}

void zCFPUControler::PrintStatus()
{
}
