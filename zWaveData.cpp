#include "include/zWaveData.h"
#include "include/zPath.h"
#include "include/zObject.h"
#include "include/oGame.h"
#include "include/zError.h"

static unsigned int ReadWaveWord(const unsigned char* data)
{
    return data[0] | (data[1] << 8);
}

static unsigned int ReadWaveDWord(const unsigned char* data)
{
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

void zCWaveData::Init()
{
    size = 0;
    memAddr = NULL;
    msLength = 0.0f;
    info = NULL;
    fileName.Clear();
    refCtr = 1;
    cacheInCount = 0;
    adpcmCompressed = 0;
}

zCWaveData::zCWaveData()
{
    Init();
}

zCWaveData::zCWaveData(zSTRING& name)
{
    Init();
    fileName = name;
}

zCWaveData::~zCWaveData()
{
    CacheOut();
    if (info) {
        delete static_cast<zTWaveInfo*>(info);
        info = NULL;
    }
}

zSTRING zCWaveData::GetName()
{
    return fileName;
}

void zCWaveData::CacheIn()
{
    if (!memAddr) {
        zPATH path(fileName);
        if (path.SearchFile(fileName, zoptions->GetDirString(9), 1) == 0x138B) {
            zerr->Warning("C: zSndMSS.h(zCWaveData::zCWaveData): Wave not found: " + fileName);
        }
        else {
            zFILE* file = zfactory->CreateZFile(path.GetPath());
            if (file->Open(0)) {
                zerr->Warning("C: zCWaveData::CacheIn(): could not open file: " + path.GetPath());
            }
            else {
                size = file->Size();
                memAddr = new unsigned char[size];
                file->Read(memAddr, size);
                file->Close();

                if (!info)
                    info = new zTWaveInfo;
                zTWaveInfo* waveInfo = static_cast<zTWaveInfo*>(info);
                waveInfo->format = 0;
                waveInfo->sampleRate = 44100;
                waveInfo->channels = 1;

                const unsigned char* data = static_cast<const unsigned char*>(memAddr);
                if (size >= 12 && ReadWaveDWord(data) == 0x46464952
                        && ReadWaveDWord(data + 8) == 0x45564157) {
                    unsigned int byteRate = 0;
                    unsigned int bitsPerSample = 0;
                    int offset = 12;
                    while (offset + 8 <= size) {
                        const unsigned int chunk = ReadWaveDWord(data + offset);
                        const int chunkSize = ReadWaveDWord(data + offset + 4);
                        const unsigned char* chunkData = data + offset + 8;

                        if (chunk == 0x20746D66 && chunkSize >= 16 && offset + 24 <= size) {
                            waveInfo->format = ReadWaveWord(chunkData);
                            waveInfo->channels = ReadWaveWord(chunkData + 2);
                            waveInfo->sampleRate = ReadWaveDWord(chunkData + 4);
                            byteRate = ReadWaveDWord(chunkData + 8);
                            bitsPerSample = ReadWaveWord(chunkData + 14);
                        }
                        else if (chunk == 0x61746164 && byteRate) {
                            msLength = chunkSize * 1000.0f / byteRate;
                        }

                        offset += 8 + chunkSize + (chunkSize & 1);
                    }

                    adpcmCompressed = waveInfo->format == 2;
                    if (!msLength && byteRate)
                        msLength = size * 1000.0f / byteRate;
                    if (!msLength && waveInfo->sampleRate && bitsPerSample)
                        msLength = size * 8.0f * 1000.0f
                            / (waveInfo->sampleRate * waveInfo->channels * bitsPerSample);
                }
            }
            delete file;
        }
    }

    cacheInCount++;
}

void zCWaveData::CacheOut()
{
    cacheInCount--;
    if (cacheInCount || !memAddr)
        return;

    delete[] static_cast<unsigned char*>(memAddr);
    memAddr = NULL;
    if (adpcmCompressed && info) {
        delete static_cast<zTWaveInfo*>(info);
        info = NULL;
    }
}

int zCWaveData::CheckForCorrectFormat(int flags)
{
    if (!size) {
        CacheIn();
        if (!size)
            return 0;
    }

    if (flags && info) {
        zTWaveInfo* waveInfo = static_cast<zTWaveInfo*>(info);
        if (waveInfo->channels == 2) {
            zerr->Warning("C: SND: PlaySound3D: SoundFile " + fileName + " has to be a mono file");
            return 0;
        }

        switch (waveInfo->format) {
        case 0:
        case 1:
        case 5:
            break;
        case 2:
        case 3:
        case 7:
            zerr->Warning("C: PlaySound3D: SoundFile " + fileName + " has to be a mono file");
            return 0;
        default:
            zerr->Warning("C: PlaySound3D: SoundFile " + fileName + " is a unknown format");
            return 0;
        }
    }

    return 1;
}

int zCWaveData::Release()
{
    const int result = --refCtr;
    if (!result)
        delete this;
    return result;
}

int zCWaveData::GetSize()
{
    if (!size)
        CacheIn();
    return size;
}
