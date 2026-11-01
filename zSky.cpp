#include "include/zSky.h"
#include "include/zSound.h"
#include "include/zTools.h"

#include <cmath>
#include <cstdlib>

static int skyState = -1;
static zCClassDef zCSkyControlerClassDef("zCSkyControler", "zCObject", NULL, 0, 0x4C, 0, "");
zCClassDef* zCSkyControler::classDef = &zCSkyControlerClassDef;
zCSkyControler* zCSkyControler::s_activeSkyControler;
int zCSkyControler::s_skyEffectsEnabled = 1;
static zCClassDef zCSkyControlerMidClassDef("zCSkyControler_Mid", "zCSkyControler", NULL, 0, 0x7C, 0, "");
zCClassDef* zCSkyControler_Mid::classDef = &zCSkyControlerMidClassDef;
static zCClassDef zCSkyControlerIndoorClassDef("zCSkyControler_Indoor", "zCSkyControler_Mid",
    NULL, 0, 0x88, 0, "");
zCClassDef* zCSkyControler_Indoor::classDef = &zCSkyControlerIndoorClassDef;

// EXE: 0x005DF340
zCObject* zCSkyControler_Indoor::_CreateNewInstance()
{
    zCSkyControler_Indoor* sky = new zCSkyControler_Indoor;
    zCClassDef::ObjectCreated(sky, classDef);
    return sky;
}

float zCSkyControler::GetCloudShadowScale() const
{
    return 0;
}

void zCSkyControler::SetCloudShadowScale(float)
{
}

void zCSkyControler::SetFillBackground(int fill)
{
    fillBackground = fill;
}

int zCSkyControler::GetFillBackground() const
{
    return fillBackground;
}

void zCSkyControler::SetLightDirty()
{
    ++relightCtr;
}

void zCSkyControler::SetRelightTime(float time)
{
    m_fRelightTime = time;
}

int zCSkyControler::GetRelightCtr()
{
    return relightCtr;
}

zCOLOR zCSkyControler::GetDaylightColorFromIntensity(int intensity)
{
    zCOLOR result;
    result.b = intensity;
    result.g = intensity;
    result.r = intensity;
    result.a = 255;
    return result;
}

int zCSkyControler::GetGlobalWindVec(zVEC3&, int)
{
    return 0;
}

void zCSkyControler::SetGlobalSkyScale(float)
{
}

float zCSkyControler::GetGlobalSkyScale() const
{
    return 0;
}

int zCSkyControler::GetGlobalSkyScaleChanged() const
{
    return 0;
}

void zCSkyControler::SetCameraLocationHint(int)
{
}

void zCSkyControler::SetWeatherType(zTWeather weather)
{
    m_enuWeather = weather;
}

zTWeather zCSkyControler::GetWeatherType() const
{
    return m_enuWeather;
}

zCClassDef* zCSkyControler_Mid::_GetClassDef() const
{
    return classDef;
}

void zCSkyControler_Indoor::operator delete(void* memory)
{
    ::operator delete(memory);
}

zCClassDef* zCSkyControler_Indoor::_GetClassDef() const
{
    return classDef;
}

float zCSkyControler_Indoor::GetTime() const
{
    return time;
}

void zCSkyControler_Indoor::ResetTime()
{
    SetTime(0);
}

void zCSkyControler_Indoor::SetOverrideColor(zVEC3)
{
}

void zCSkyControler_Indoor::SetOverrideColorFlag(int)
{
}

void zCSkyControler_Indoor::UpdateWorldDependencies()
{
}

zCSkyControler_Indoor::zCSkyControler_Indoor()
    : userFarZ(5000.0f), userFarZScalability(1.0f), time(0)
{
    underwaterColor.r = 50;
    underwaterColor.g = 0;
    underwaterColor.b = 255;
    underwaterColor.a = 255;
    underwaterFarZ = 2000.0f;
    underwaterFX = 0;
    vobUnderwaterPFX = 0;
}

zCSkyControler_Indoor::~zCSkyControler_Indoor()
{
    if (vobUnderwaterPFX)
        vobUnderwaterPFX->Release();
    delete scrPolyMesh;
    if (backgroundTexture)
        backgroundTexture->Release();
    vobUnderwaterPFX = 0;
    scrPolyMesh = 0;
    backgroundTexture = 0;
}

zCClassDef* zCSkyControler::_GetClassDef() const
{
    return classDef;
}

zCSkyControler::zCSkyControler()
    : polyLightCLUTPtr(0), cloudShadowScale(0), m_bColorChanged(0),
      m_enuWeather(zTWEATHER_RAIN), fillBackground(1), backgroundTexture(0),
      relightCtr(0), lastRelightTime(0), m_fRelightTime(0.000018f)
{
    backgroundColor.r = 0;
    backgroundColor.g = 0;
    backgroundColor.b = 0;
    backgroundColor.a = 255;
}

zCSkyControler::~zCSkyControler()
{
    if (backgroundTexture)
        backgroundTexture->Release();
    backgroundTexture = 0;
}

void zCSkyControler::ClearBackground(zCOLOR color)
{
    backgroundColor = color;
}

zCSkyControler_Mid::zCSkyControler_Mid()
    : underwaterFX(0), underwaterFarZ(3500.0f), underwaterStartTime(0),
      oldFovX(0), oldFovY(0), vobUnderwaterPFX(0), scrPoly(0), scrPolyMesh(0),
      scrPolyAlpha(0), scrPolyAlphaFunc(0)
{
    underwaterColor.r = 50;
    underwaterColor.g = 0;
    underwaterColor.b = 255;
    underwaterColor.a = 255;
    InitScreenBlend();
}

zCSkyControler_Mid::~zCSkyControler_Mid()
{
    if (vobUnderwaterPFX)
        vobUnderwaterPFX->Release();
    delete scrPolyMesh;
    vobUnderwaterPFX = 0;
    scrPolyMesh = 0;
}

void zCSkyControler_Mid::InitUnderwaterPFX()
{
    if (!vobUnderwaterPFX)
        vobUnderwaterPFX = new zCVob;
    vobUnderwaterPFX->SetCollDetStat(0);
    vobUnderwaterPFX->SetCollDetDyn(0);
}

zCUnderwaterPFX::~zCUnderwaterPFX()
{
}

void zCSkyControler_Mid::SetUnderwaterFX(int enabled)
{
    if (underwaterFX == enabled)
        return;
    underwaterFX = enabled;
    if (!zCCamera::activeCam)
        return;
    if (!enabled) {
        zCCamera::SetFOV(zCCamera::activeCam, oldFovX, oldFovY);
        zsound->SetGlobalOcclusion(0);
        return;
    }
    underwaterStartTime = ztimer.totalTimeFloat;
    zCCamera::GetFOV(zCCamera::activeCam, &oldFovX, &oldFovY);
    zsound->SetGlobalOcclusion(0.9f);
}

int zCSkyControler_Mid::GetUnderwaterFX() const
{
    return underwaterFX;
}

void zCSkyControler_Mid::RenderSkyPre()
{
    if (underwaterFX)
        m_bColorChanged = 1;
}

void zCSkyControler_Mid::RenderSkyPost(int)
{
    if (underwaterFX && !vobUnderwaterPFX)
        InitUnderwaterPFX();
    RenderScreenBlend();
}

void zCSkyControler_Mid::InitScreenBlend()
{
    scrPolyAlpha = 0;
    scrPolyColor.r = 200;
    scrPolyColor.g = 200;
    scrPolyColor.b = 200;
    scrPolyColor.a = 255;
    scrPolyAlphaFunc = 3;
    scrPolyMesh = new zCMesh;
    scrPoly = new zCPolygon;
    scrPoly->AllocVerts(4);
    scrPoly->material = new zCMaterial;
    scrPolyMesh->polyArray = scrPoly;
    scrPolyMesh->polyList = new zCPolygon*[1];
    scrPolyMesh->polyList[0] = scrPoly;
    scrPolyMesh->numPoly = 1;
    scrPolyMesh->numVerts = 4;
    scrPolyMesh->vertexList = new zVEC3[4];
    for (int i = 0; i < 4; ++i) {
        scrPoly->SetVertex(i, reinterpret_cast<zCVertex*>(&scrPolyMesh->vertexList[i]));
        scrPolyMesh->vertexList[i].n[0] = i == 1 || i == 2;
        scrPolyMesh->vertexList[i].n[1] = i >= 2;
        scrPolyMesh->vertexList[i].n[2] = 0;
        scrPoly->feature[i]->vertNormal.n[0] = 0;
        scrPoly->feature[i]->vertNormal.n[1] = 0;
        scrPoly->feature[i]->vertNormal.n[2] = 1.0f;
    }
}

void zCSkyControler_Mid::RenderScreenBlend()
{
    if (!scrPoly || (!underwaterFX && scrPolyAlpha < 10))
        return;
    zCOLOR color = underwaterFX ? underwaterColor : scrPolyColor;
    scrPoly->material->color[0] = color.b;
    scrPoly->material->color[1] = color.g;
    scrPoly->material->color[2] = color.r;
    scrPoly->material->color[3] = underwaterFX ? 170 : scrPolyAlpha;
    scrPoly->material->rndAlphaBlendFunc = underwaterFX ? 2 : scrPolyAlphaFunc;
}

void zCSkyControler_Mid::SetScreenBlendAlpha(int alpha)
{
    scrPolyAlpha = alpha;
}

void zCSkyControler_Mid::SetScreenBlendColor(const zCOLOR& color)
{
    scrPolyColor = color;
}

void zCSkyControler_Mid::SetScreenBlendAlphaFunc(int alphaFunc)
{
    scrPolyAlphaFunc = alphaFunc;
}

void zCSkyControler_Indoor::SetTime(float value)
{
    time = value;
    while (time >= 1.0f)
        time -= 1.0f;
    while (time < 0)
        time += 1.0f;
    const float relightDelta = lastRelightTime > time
        ? lastRelightTime - time : time - lastRelightTime;
    if (relightDelta >= m_fRelightTime) {
        ++relightCtr;
        lastRelightTime = time;
    }
}

void zCSkyControler_Indoor::SetFarZ(float farZ)
{
    userFarZ = farZ;
}

float zCSkyControler_Indoor::GetFarZ() const
{
    return userFarZ;
}

void zCSkyControler_Indoor::SetFarZScalability(float scalability)
{
    userFarZScalability = scalability;
    if (userFarZScalability < 0)
        userFarZScalability = 0;
    else if (userFarZScalability > 4.0f)
        userFarZScalability = 4.0f;
}

float zCSkyControler_Indoor::GetFarZScalability() const
{
    return userFarZScalability;
}

void zCSkyControler_Indoor::SetBackgroundColor(zCOLOR color)
{
    backgroundColor = color;
}

zCOLOR zCSkyControler_Indoor::GetBackgroundColor() const
{
    return backgroundColor;
}

zCOLOR zCSkyControler_Indoor::GetBackgroundColorDef() const
{
    return backgroundColor;
}

void zCSkyControler_Indoor::RenderSkyPre()
{
    zCSkyControler_Mid::RenderSkyPre();
    ClearBackground(underwaterFX ? underwaterColor : backgroundColor);
}

void zCSkyControler_Indoor::RenderSkyPost(int flags)
{
    zCSkyControler_Mid::RenderSkyPost(flags);
}

int zCUnderwaterPFX::Render(zTRenderContext& context)
{
    ProcessParticles();
    return zCParticleFX::Render(context);
}

void zCUnderwaterPFX::ProcessParticles()
{
    static float time;
    time += 0.01f;
    for (zSParticle* particle = firstPart; particle; particle = particle->next) {
        for (int axis = 0; axis < 3; ++axis)
            particle->position.n[axis] += std::sin(time + axis) * 0.1f;
        particle->alpha = 255.0f;
    }
}

void zCUnderwaterPFX::CreateParticles()
{
    zCParticleFX::CreateParticles();
    camPosLastFrame.n[0] = 0;
    camPosLastFrame.n[1] = 0;
    camPosLastFrame.n[2] = 0;
    for (zSParticle* particle = firstPart; particle; particle = particle->next) {
        const float scale = (std::rand() / 32767.0f - 0.5f) * 0.5f;
        particle->size.n[0] += particle->size.n[0] * scale;
        particle->size.n[1] += particle->size.n[1] * scale;
    }
}
