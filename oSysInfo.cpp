#include "include/oSysInfo.h"
#include "include/oGame.h"

static int systemInfoState = -1;

oCSystemInfo::oCSystemInfo()
    : scored(0), cpuLow(696.0f), cpuHigh(1200.0f), memLow(256.0f), memHigh(512.0f),
      graMemLow(16.0f), graMemHigh(64.0f), scoreCpu(1.0f), scoreMem(1.0f),
      scoreGraMem(1.0f), scoreGra(1.0f), scoreSnd(1.0f), score(1.0f)
{
}

oCSystemInfo::~oCSystemInfo()
{
}

void oCSystemInfo::AnalyseNow()
{
    zCSystemInfo::AnalyseNow();
}

float oCSystemInfo::GetGraphicsBonus()
{
    float bonus = 0.0f;
    const int tnl = zrenderer->HasCapability(zRND_CAP_TNL_HARDWARE);
    const int stages = zrenderer->HasCapability(zRND_CAP_MAX_BLEND_STAGES);
    if (tnl)
        bonus = 0.2f;
    if (stages == 2)
        bonus += 0.1f;
    else if (stages >= 3)
        bonus += 0.2f;
    if (!tnl) {
        zoptions->WriteBool("RENDERER_D3D", "zFogRadial", 0);
        if (zrenderer)
            zrenderer->SetRadialFog(0);
    }
    return bonus;
}

float oCSystemInfo::GetSoundBonus()
{
    return 0.25f;
}

void oCSystemInfo::ScoreNow()
{
    scoreCpu = (cpuSpeed - cpuLow) / (cpuHigh - cpuLow);
    const float memory = (float)(memSize >> 20);
    const float graphicsMemory = (float)(graMemSize >> 20);
    scoreMem = (memory - memLow) / (memHigh - memLow);
    scoreGraMem = (graphicsMemory - graMemLow) / (graMemHigh - graMemLow);
    scoreGra = GetGraphicsBonus() + 0.5f;
    scoreSnd = 0.75f;
    score = (2.0f * (scoreCpu + scoreMem) + scoreGra) * 0.2f;
    scored = 1;
}

zSTRING oCSystemInfo::GetScoreDescr(float value)
{
    if (value >= 1.0f)
        return "UNBELIEVABLE";
    if (value >= 0.8f)
        return "GREAT";
    if (value >= 0.6f)
        return "GOOD";
    if (value >= 0.4f)
        return "AVERAGE";
    if (value >= 0.2f)
        return "POOR";
    if (value >= 0.0f)
        return "BAD";
    return "NO WAY";
}

void oCSystemInfo::ResetSettings(float requestedScore)
{
    float textureDetail = 1.0f;
    float modelDetail = 0.5f;
    int sight = 4;
    int musicEnabled = 1;
    if (requestedScore >= 0.66f) {
        textureDetail = 1.0f;
        modelDetail = 0.8f;
    } else if (requestedScore >= 0.33f) {
        textureDetail = 0.7f;
        modelDetail = 0.5f;
        sight = 3;
    } else if (requestedScore >= 0.0f) {
        textureDetail = 0.4f;
        modelDetail = 0.2f;
        sight = 2;
        musicEnabled = 0;
    } else {
        if (scoreCpu < 0.2f) {
            sight = 2;
            modelDetail = 0.3f;
        } else if (scoreCpu < 0.4f) {
            sight = 3;
            modelDetail = 0.4f;
        } else if (scoreCpu > 0.8f) {
            sight = 5;
        }
        if (scoreMem < 0.2f) {
            sight -= 2;
            textureDetail = 0.4f;
            musicEnabled = 0;
        } else if (scoreMem < 0.4f) {
            --sight;
            textureDetail = 0.6f;
        } else if (scoreMem > 0.8f) {
            ++sight;
        }
        if (scoreGraMem < 0.35f) {
            sight -= 2;
            textureDetail -= 0.4f;
        } else if (scoreGraMem < 0.6f) {
            --sight;
            textureDetail -= 0.2f;
        }
        if (textureDetail < 0.0f)
            textureDetail = 0.0f;
        else if (textureDetail > 1.0f)
            textureDetail = 1.0f;
        if (modelDetail < 0.0f)
            modelDetail = 0.0f;
        else if (modelDetail > 1.0f)
            modelDetail = 1.0f;
    }
    zoptions->WriteInt(zOPT_SEC_PERFORMANCE, "sightValue", sight);
    zoptions->WriteReal(zOPT_SEC_INTERNAL, "texDetailIndex", textureDetail);
    zoptions->WriteReal(zOPT_SEC_PERFORMANCE, "modelDetail", modelDetail);
    zoptions->WriteBool(zOPT_SEC_SOUND, "musicEnabled", musicEnabled);
}
