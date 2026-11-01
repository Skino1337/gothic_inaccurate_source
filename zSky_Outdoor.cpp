#include "include/zSky_Outdoor.h"
#include "include/zSound.h"

#include <cmath>

static int skyOutdoorState = -1;
static zCClassDef zCSkyControlerOutdoorClassDef("zCSkyControler_Outdoor", "zCSkyControler_Mid",
    NULL, 0, 0x6BC, 0, "");
zCClassDef* zCSkyControler_Outdoor::classDef = &zCSkyControlerOutdoorClassDef;
static zSTRING skyMoonTexture("moon.tga");
static zSTRING skySunTexture("unSun5.tga");

zCSkyPlanet::zCSkyPlanet()
    : mesh(0), color0(zVEC3(), 0), color1(zVEC3(), 0), size(0)
{
}

zCSkyPlanet::~zCSkyPlanet()
{
    delete mesh;
    mesh = 0;
}

zCSkyControler_Outdoor::zCSkyControler_Outdoor(int mainControler)
    : initDone(0), masterTime(0), masterTimeLast(0), state0(0), state1(0),
      dayCounter(0), m_fSkyScale(1.0f), m_bSkyScaleChanged(0), m_bOverrideColorFlag(0),
      resultFogScale(0), resultFogNear(0), resultFogFar(0), resultFogSkyNear(0),
      resultFogSkyFar(0), userFogFar(60000.0f), userFarZScalability(1.0f),
      skyCloudLayerTex(0), m_bSunVisible(0), m_fFadeScale(0), vobSkyPFX(0),
      skyPFXTimer(0), m_bIsMainControler(mainControler)
{
    polyLightCLUTPtr = polyLightCLUT;
    skyLayerState[0] = 0;
    skyLayerState[1] = 0;
    m_overrideColor.n[0] = 0;
    m_overrideColor.n[1] = 0;
    m_overrideColor.n[2] = 0;
    resultFogColor.a = 255;
    resultFogColorOverride.a = 255;
    rainFX.outdoorRainFX = new zCOutdoorRainFX;
    rainFX.camLocationHint = 0;
    rainFX.outdoorRainFXWeight = 0;
    rainFX.soundVolume = 0;
    rainFX.timerInsideSectorCantSeeOutside = 0;
    rainFX.timeStartRain = 0.187f;
    rainFX.timeStopRain = 0.25f;
    rainFX.renderLightning = 0;
    rainFX.m_bRaining = 0;
    rainFX.m_iRainCtr = 0;
}

zCSkyControler_Outdoor::~zCSkyControler_Outdoor()
{
    for (int i = 0; i < stateList.numInArray; ++i)
        delete stateList.parray[i];
    stateList.DeleteList();
    delete rainFX.outdoorRainFX;
    if (skyCloudLayerTex)
        skyCloudLayerTex->Release();
    if (vobSkyPFX)
        vobSkyPFX->Release();
    rainFX.outdoorRainFX = 0;
    skyCloudLayerTex = 0;
    vobSkyPFX = 0;
    polyLightCLUTPtr = 0;
}

zCClassDef* zCSkyControler_Outdoor::_GetClassDef() const
{
    return classDef;
}

float zCSkyControler_Outdoor::GetTime() const
{
    return masterTime;
}

void zCSkyControler_Outdoor::SetBackgroundColor(zCOLOR)
{
}

zCOLOR zCSkyControler_Outdoor::GetBackgroundColor() const
{
    return m_bOverrideColorFlag ? resultFogColorOverride : resultFogColor;
}

zCOLOR zCSkyControler_Outdoor::GetBackgroundColorDef() const
{
    return resultFogColor;
}

void zCSkyControler_Outdoor::SetOverrideColor(zVEC3 color)
{
    m_overrideColor = color;
}

void zCSkyControler_Outdoor::SetOverrideColorFlag(int enabled)
{
    m_bOverrideColorFlag = enabled;
}

int zCSkyControler_Outdoor::GetRenderLightning() const
{
    return rainFX.renderLightning;
}

void zCSkyControler_Outdoor::SetCameraLocationHint(int hint)
{
    rainFX.camLocationHint = hint;
}

float zCSkyControler_Outdoor::GetGlobalSkyScale() const
{
    return m_fSkyScale;
}

int zCSkyControler_Outdoor::GetGlobalSkyScaleChanged() const
{
    return m_bSkyScaleChanged;
}

// EXE: 0x005E0FB0
zCObject* zCSkyControler_Outdoor::_CreateNewInstance()
{
    zCSkyControler_Outdoor* sky = new zCSkyControler_Outdoor;
    zCClassDef::ObjectCreated(sky, classDef);
    return sky;
}

void zCSkyControler_Outdoor::operator delete(void* memory)
{
    ::operator delete(memory);
}

zCOutdoorRainFX::zCOutdoorRainFX()
    : m_effectWeight(0), m_numDestFlyParticle(0), m_numFlyParticle(0),
      m_numImpactParticle(0), m_timeLen(1000.0f), m_velocityLen(2.0f),
      m_soundEffect(0), m_soundActive(0), m_raindropTexture(0), m_rainTexture(0),
      m_rainMaterial(0), m_currentCacheIndex(0), m_soundHandle(0),
      m_enuWeather(zTWEATHER_RAIN)
{
    m_camPosLastFrame.n[0] = 0;
    m_camPosLastFrame.n[1] = 0;
    m_camPosLastFrame.n[2] = 0;
    m_positionUpdateVector.n[0] = 0;
    m_positionUpdateVector.n[1] = -1.0f;
    m_positionUpdateVector.n[2] = 0;
    for (int i = 0; i < 0x400; ++i)
        m_flyParticleList[i].lifeSpan = 100.0f;
    m_shader.InsertStage(0, &m_shaderStage);
    m_shaderStage.alphaGen = 1;
    m_shaderStage.rgbGen = 1;
    m_shaderStage.alphaFunc = 2;
}

zCOutdoorRainFX::~zCOutdoorRainFX()
{
    if (m_raindropTexture)
        m_raindropTexture->Release();
    if (m_rainMaterial)
        m_rainMaterial->Release();
    if (m_rainTexture)
        m_rainTexture->Release();
    if (m_soundEffect)
        m_soundEffect->Release();
    m_raindropTexture = 0;
    m_rainMaterial = 0;
    m_rainTexture = 0;
    m_soundEffect = 0;
}

void zCOutdoorRainFX::UpdateSound(float volume)
{
    if (volume <= 0.0f) {
        if (m_soundEffect && zsound->IsSoundActive(m_soundHandle)) {
            zsound->StopSound(m_soundHandle);
            m_soundActive = 0;
        }
        return;
    }

    if (!m_soundEffect) {
        const zSTRING soundName = m_enuWeather ? zSTRING("rain_01.wav") : zSTRING("snow_01.wav");
        m_soundEffect = zsound->LoadSoundFX(soundName);
    }

    if ((m_soundEffect->cacheState & 3) != 3) {
        m_soundEffect->TouchTimeStampLocal();
        zresMan->CacheIn(m_soundEffect, volume);
    } else {
        m_soundEffect->TouchTimeStamp();
    }

    if ((m_soundEffect->cacheState & 3) != 3)
        return;

    if (!zsound->IsSoundActive(m_soundHandle)) {
        m_soundHandle = zsound->PlaySound(m_soundEffect, 0, -1, volume, -2.0f);
        m_soundActive = 1;
    }

    int currentVolume;
    float pitch;
    float reverb;
    zsound->GetSoundProps(m_soundHandle, currentVolume, pitch, reverb);
    zsound->UpdateSoundProps(m_soundHandle, volume, pitch, reverb);
}

void zCOutdoorRainFX::SetWeatherType(zTWeather weather)
{
    if (m_enuWeather == weather)
        return;
    if (m_raindropTexture)
        m_raindropTexture->Release();
    if (m_rainTexture)
        m_rainTexture->Release();
    if (m_rainMaterial)
        m_rainMaterial->Release();
    if (m_soundEffect)
        m_soundEffect->Release();
    m_raindropTexture = 0;
    m_rainTexture = 0;
    m_rainMaterial = 0;
    m_soundEffect = 0;
    m_enuWeather = weather;
}

void zCOutdoorRainFX::SetEffectWeight(float weight, float velocityScale)
{
    m_effectWeight = weight;
    m_velocityLen = 2.0f * velocityScale;
    const int oldDestination = m_numDestFlyParticle;
    m_numDestFlyParticle = static_cast<int>(weight * 1024.0f);
    m_numFlyParticle = m_numDestFlyParticle;
    for (int i = oldDestination; i < m_numDestFlyParticle && i < 0x400; ++i) {
        if (m_flyParticleList[i].lifeSpan >= 100.0f) {
            m_flyParticleList[i].lifeSpan = 100.0f;
            m_freeFlyParticleList.InsertEnd(&m_flyParticleList[i]);
        }
    }
}

int zCOutdoorRainFX::CheckCameraBeam(const zVEC3& delta)
{
    const float distance2 = delta.n[0] * delta.n[0] + delta.n[1] * delta.n[1] +
        delta.n[2] * delta.n[2];
    if (distance2 < 3750.0f * 3750.0f)
        return 0;
    for (int i = 0; i < m_numDestFlyParticle && i < 0x400; ++i) {
        if (m_flyParticleList[i].lifeSpan < 1.0f) {
            m_flyParticleList[i].lifeSpan = 100.0f;
            m_freeFlyParticleList.InsertEnd(&m_flyParticleList[i]);
        }
    }
    return 1;
}

void zCOutdoorRainFX::CreateParticles(zTRenderContext&)
{
    for (int i = 0; i < m_numDestFlyParticle && i < 0x400; ++i) {
        zSParticle& particle = m_flyParticleList[i];
        for (int axis = 0; axis < 3; ++axis)
            particle.position.n[axis] += m_positionUpdateVector.n[axis] * m_velocityLen;
        particle.lifeSpan -= 1.0f;
        if (particle.lifeSpan <= 0) {
            particle.lifeSpan = 100.0f;
            m_freeFlyParticleList.InsertEnd(&particle);
        }
    }
}

void zCOutdoorRainFX::UpdateParticles()
{
    const float delta = 1.0f / m_timeLen;
    for (int i = 0; i < m_numFlyParticle && i < 0x400; ++i) {
        zSParticle& particle = m_flyParticleList[i];
        if (particle.lifeSpan < 1.0f) {
            particle.lifeSpan -= delta;
            if (particle.lifeSpan <= 0) {
                particle.lifeSpan = 100.0f;
                m_freeFlyParticleList.InsertEnd(&particle);
            }
        }
    }
    m_numImpactParticle = 0;
}

void zCOutdoorRainFX::RenderParticles(zTRenderContext&, zCOLOR color)
{
    for (int i = 0; i < m_numFlyParticle && i < 0x400; ++i) {
        m_flyParticleList[i].color.n[0] = color.r;
        m_flyParticleList[i].color.n[1] = color.g;
        m_flyParticleList[i].color.n[2] = color.b;
        m_flyParticleList[i].alpha = color.a;
    }
}

zCSkyLayerData::zCSkyLayerData()
    : skyMode(1), tex(0), texAlpha(0), texScale(1.0f)
{
    texSpeed.n[0] = 1.0f;
    texSpeed.n[1] = 0;
}

zCSkyLayerData::~zCSkyLayerData()
{
}

zCSkyState::zCSkyState()
    : time(0), fogDist(0), sunOn(1), cloudShadowOn(0)
{
}

zCSkyState::~zCSkyState()
{
}

static void SetSkyPresetColors(zCSkyState* state, float polyR, float polyG, float polyB,
    float fogR, float fogG, float fogB)
{
    state->polyColor.n[0] = polyR;
    state->polyColor.n[1] = polyG;
    state->polyColor.n[2] = polyB;
    state->fogColor.n[0] = fogR;
    state->fogColor.n[1] = fogG;
    state->fogColor.n[2] = fogB;
    state->domeColor1 = state->polyColor;
    state->domeColor0 = state->fogColor;
}

void zCSkyState::PresetDay0()
{
    time = 0.75f;
    SetSkyPresetColors(this, 255, 250, 235, 120, 140, 180);
    fogDist = 0.2f;
    sunOn = 1;
    layer[0].texName = "SKYDAY_LAYER1_A0.TGA";
    layer[1].texName = "SKYDAY_LAYER0_A0.TGA";
    layer[0].texAlpha = 0;
}

void zCSkyState::PresetDay1()
{
    time = 0;
    SetSkyPresetColors(this, 255, 250, 235, 120, 140, 180);
    fogDist = 0.05f;
    sunOn = 1;
    layer[0].texName = "SKYDAY_LAYER1_A0.TGA";
}

void zCSkyState::PresetDay2()
{
    time = 0.25f;
    SetSkyPresetColors(this, 255, 250, 235, 120, 140, 180);
    layer[0].texAlpha = 0;
    layer[1].texAlpha = 255.0f;
}

void zCSkyState::PresetEvening()
{
    time = 0.3f;
    SetSkyPresetColors(this, 255, 185, 170, 170, 70, 50);
    layer[0].texAlpha = 128.0f;
    layer[1].texAlpha = 128.0f;
}

void zCSkyState::PresetNight0()
{
    time = 0.7f;
    SetSkyPresetColors(this, 210, 210, 195, 20, 20, 60);
    fogDist = 0.1f;
    sunOn = 0;
    layer[0].texName = "SKYNIGHT_LAYER0_A0.TGA";
    layer[1].texName = "SKYNIGHT_LAYER1_A0.TGA";
    layer[0].texAlpha = 255.0f;
    layer[0].texScale *= 4.0f;
}

void zCSkyState::PresetNight1()
{
    time = 0.5f;
    SetSkyPresetColors(this, 40, 60, 210, 5, 5, 20);
    fogDist = 0.05f;
    sunOn = 0;
    layer[1].texName = "SKYNIGHT_LAYER1_A0.TGA";
    layer[0].texAlpha = 255.0f;
}

void zCSkyState::PresetNight2()
{
    time = 0.65f;
    SetSkyPresetColors(this, 40, 60, 210, 5, 5, 20);
    fogDist = 0.2f;
    sunOn = 0;
    layer[1].texName = "SKYDAY_LAYER0_A0.TGA";
    layer[0].texAlpha = 255.0f;
}

void zCSkyState::PresetDawn()
{
    time = 0.7f;
    SetSkyPresetColors(this, 190, 160, 255, 80, 60, 105);
    layer[0].texAlpha = 128.0f;
    layer[1].texAlpha = 128.0f;
}

zCSkyLayerData::~zCSkyLayerData()
{
    if (tex)
        tex->Release();
    tex = 0;
}

zCMesh* zCSkyLayer::CreateSkyPoly()
{
    zCMesh* mesh = new zCMesh;
    mesh->numPoly = 1;
    mesh->numVerts = 4;
    mesh->polyArray = new zCPolygon;
    mesh->polyList = new zCPolygon*[1];
    mesh->polyList[0] = mesh->polyArray;
    mesh->vertexList = new zVEC3[4];
    mesh->polyArray->AllocVerts(4);
    mesh->polyArray->material = new zCMaterial;
    for (int i = 0; i < 4; ++i) {
        mesh->polyArray->SetVertex(i, reinterpret_cast<zCVertex*>(&mesh->vertexList[i]));
        mesh->vertexList[i].n[0] = i == 1 || i == 2 ? 50000.0f : -50000.0f;
        mesh->vertexList[i].n[1] = 1000.0f;
        mesh->vertexList[i].n[2] = i >= 2 ? 50000.0f : -50000.0f;
        mesh->polyArray->feature[i]->vertNormal.n[0] = 0;
        mesh->polyArray->feature[i]->vertNormal.n[1] = 1.0f;
        mesh->polyArray->feature[i]->vertNormal.n[2] = 0;
    }
    return mesh;
}

zCSkyLayer::zCSkyLayer()
    : skyPolyMesh(CreateSkyPoly()), skyPoly(skyPolyMesh->polyArray), skyDomeMesh(0), skyMode(0)
{
    skyTexOffs.n[0] = 0;
    skyTexOffs.n[1] = 0;
}

zCSkyLayer::~zCSkyLayer()
{
    delete skyPolyMesh;
    delete skyDomeMesh;
    skyPoly = 0;
    skyPolyMesh = 0;
    skyDomeMesh = 0;
}

zCMesh* zCSkyLayer::CreateSkyDomeMesh()
{
    return CreateSkyPoly();
}

void zCSkyLayer::SetDomeMeshEnabled(int enabled)
{
    if (!enabled) {
        delete skyDomeMesh;
        skyDomeMesh = 0;
        skyMode = 0;
        skyPoly = skyPolyMesh->polyArray;
    } else {
        skyDomeMesh = CreateSkyDomeMesh();
        skyMode = 1;
        skyPoly = skyDomeMesh->polyArray;
    }
}

void zCSkyLayer::SetSkyDomeTexture(zCTexture* texture)
{
    if (!skyDomeMesh)
        return;
    for (int i = 0; i < skyDomeMesh->numPoly; ++i) {
        skyDomeMesh->polyList[i]->material->texture = texture;
        if (texture)
            texture->AddRef();
    }
}

void zCSkyLayer::SetSkyPolyTexture(zCTexture* texture)
{
    skyPoly->material->texture = texture;
    if (texture)
        texture->AddRef();
}

void zCSkyLayer::RenderSkyLayer(zCSkyState* state)
{
    if (!skyPoly->material->color[3])
        return;
    if (skyMode)
        RenderSkyDome(state);
    else
        RenderSkyPoly(state);
}

void zCSkyLayer::RenderSkyPoly(zCSkyState* state)
{
    static float time;
    time += 0.001f;
    skyTexOffs.n[0] = time * state->layer[0].texSpeed.n[0];
    skyTexOffs.n[1] = time * state->layer[0].texSpeed.n[1];
    skyPoly->feature[0]->mapping.n[0] = skyTexOffs.n[0];
    skyPoly->feature[0]->mapping.n[1] = skyTexOffs.n[1];
    skyPoly->material->rndAlphaBlendFunc = skyPoly->material->color[3] < 255 ? 2 : 1;
}

void zCSkyLayer::RenderSkyDome(zCSkyState* state)
{
    ColorizeSkyDome();
    skyPoly->material->rndAlphaBlendFunc = 2;
}

void zCSkyLayer::ColorizeSkyDome()
{
    if (!skyDomeMesh)
        return;
    for (int i = 0; i < skyDomeMesh->numPoly; ++i) {
        zCPolygon& polygon = *skyDomeMesh->polyList[i];
        for (int j = 0; j < polygon.polyNumVert; ++j) {
            polygon.feature[j]->lightDyn[0] = 255;
            polygon.feature[j]->lightDyn[1] = 255;
            polygon.feature[j]->lightDyn[2] = 255;
            polygon.feature[j]->lightDyn[3] = 255;
        }
    }
}

void zCSkyLayer::RenderRainCloudLayer(zCOLOR color)
{
    skyPoly->material->color[3] = color.a;
    static float time;
    time += 0.001f;
    skyPoly->feature[0]->mapping.n[0] = time * 1.3f;
    skyPoly->feature[0]->mapping.n[1] = time * 0.3f;
    skyPoly->feature[0]->lightDyn[0] = color.b;
    skyPoly->feature[0]->lightDyn[1] = color.g;
    skyPoly->feature[0]->lightDyn[2] = color.r;
    skyPoly->feature[0]->lightDyn[3] = color.a;
}

void zCSkyControler_Outdoor::Init()
{
    initDone = 1;
    for (int i = 0; i < 2; ++i) {
        planets[i].mesh = zCSkyLayer::CreateSkyPoly();
        planets[i].mesh->polyArray->material->rndAlphaBlendFunc = 3;
        planets[i].size = 1000.0f;
        planets[i].color0 = zVEC4(zVEC3(), 1.0f);
        planets[i].color1 = zVEC4(zVEC3(), 1.0f);
    }
}

void zCSkyControler_Outdoor::SetGlobalSkyScale(float scale)
{
    if (m_fSkyScale != scale) {
        m_fSkyScale = scale;
        m_bSkyScaleChanged = 1;
    }
}

void zCSkyControler_Outdoor::UpdateWorldDependencies()
{
    heightFogMinY = 0;
    heightFogMaxY = 10000.0f;
}

int zCSkyControler_Outdoor::GetStateTextureSearch(int start, int layerIndex, int forward)
{
    int result = start;
    do {
        if (stateList.parray[result]->layer[layerIndex].texName.Length())
            break;
        if (forward) {
            ++result;
            if (result > 7)
                result = 0;
        } else {
            --result;
            if (result < 0)
                result = 7;
        }
    } while (result != start);
    return result;
}

void zCSkyControler_Outdoor::ApplyStateTexToLayer(int stateIndex, int layerIndex)
{
    skyLayerState[layerIndex] = stateList.parray[stateIndex];
    zCSkyLayerData& data = skyLayerState[layerIndex]->layer[layerIndex];
    skyLayer[layerIndex].skyPoly->material->color[3] = data.texAlpha;
    if (data.tex)
        skyLayer[layerIndex].SetSkyPolyTexture(data.tex);
}

void zCSkyControler_Outdoor::CreateDefault()
{
    stateList.DeleteList();
    for (int i = 0; i < 8; ++i)
        stateList.InsertEnd(new zCSkyState);
    stateList.parray[0]->PresetDay0();
    stateList.parray[1]->PresetDay1();
    stateList.parray[2]->PresetDay2();
    stateList.parray[3]->PresetEvening();
    stateList.parray[4]->PresetNight0();
    stateList.parray[5]->PresetNight1();
    stateList.parray[6]->PresetNight2();
    stateList.parray[7]->PresetDawn();
}

void zCSkyControler_Outdoor::RenderPlanets(float scale)
{
    SetScreenBlendAlpha(0);
    for (int i = 0; i < 2; ++i) {
        const float angle = (masterTime - 0.3f + i * 0.5f) * 6.28318548f;
        planets[i].pos.n[0] = std::cos(angle) * 10000.0f;
        planets[i].pos.n[1] = std::sin(angle) * 10000.0f;
        planets[i].pos.n[2] = -40000.0f;
        planets[i].rotAxis.n[0] = 0;
        planets[i].rotAxis.n[1] = 1.0f;
        planets[i].rotAxis.n[2] = 0;
        if (planets[i].mesh) {
            planets[i].mesh->polyArray->material->color[3] =
                static_cast<unsigned char>(255.0f * scale);
            for (int j = 0; j < 4; ++j) {
                planets[i].mesh->polyArray->feature[j]->lightDyn[0] = 255;
                planets[i].mesh->polyArray->feature[j]->lightDyn[1] = 255;
                planets[i].mesh->polyArray->feature[j]->lightDyn[2] = 255;
            }
        }
    }
}

void zCSkyControler_Outdoor::ReadFogColorsFromINI()
{
    fogColorDayVariations.DeleteList();
    fogColorDayVariations2.DeleteList();
    const zVEC3 day0 = { 82, 109, 198 };
    const zVEC3 day1 = { 255, 255, 0 };
    const zVEC3 day2 = { 18, 16, 60 };
    const zVEC3 day3 = { 120, 140, 180 };
    fogColorDayVariations.InsertEnd(day0);
    fogColorDayVariations.InsertEnd(day1);
    fogColorDayVariations.InsertEnd(day2);
    fogColorDayVariations.InsertEnd(day3);
    fogColorDayVariations2.InsertEnd(day2);
    fogColorDayVariations2.InsertEnd(day2);
    fogColorDayVariations2.InsertEnd(day0);
    fogColorDayVariations2.InsertEnd(day3);
}

void zCSkyControler_Outdoor::ApplyFogColorsFromINI(int frame)
{
    const zCArray<zVEC3>& colors = m_bDarkSky ? fogColorDayVariations2 : fogColorDayVariations;
    if (frame >= colors.numInArray)
        return;
    stateList.parray[0]->fogColor = colors.parray[frame];
    stateList.parray[1]->fogColor = colors.parray[frame];
    stateList.parray[7]->fogColor = colors.parray[frame];
}

void zCSkyControler_Outdoor::Interpolate()
{
    int index = 0;
    while (index < stateList.numInArray && stateList.parray[index]->time < masterTime)
        ++index;
    int firstIndex = index - 1;
    if (firstIndex >= stateList.numInArray)
        firstIndex = 0;
    else if (firstIndex < 0)
        firstIndex = stateList.numInArray - 1;
    const int secondIndex = (firstIndex + 1) % stateList.numInArray;
    state0 = stateList.parray[firstIndex];
    state1 = stateList.parray[secondIndex];
    float endTime = state1->time;
    if (endTime <= state0->time)
        endTime += 1.0f;
    const float fraction = (masterTime - state0->time) / (endTime - state0->time);
    for (int axis = 0; axis < 3; ++axis) {
        masterState.polyColor.n[axis] = state0->polyColor.n[axis] +
            (state1->polyColor.n[axis] - state0->polyColor.n[axis]) * fraction;
        masterState.fogColor.n[axis] = state0->fogColor.n[axis] +
            (state1->fogColor.n[axis] - state0->fogColor.n[axis]) * fraction;
        masterState.domeColor1.n[axis] = state0->domeColor1.n[axis] +
            (state1->domeColor1.n[axis] - state0->domeColor1.n[axis]) * fraction;
        masterState.domeColor0.n[axis] = state0->domeColor0.n[axis] +
            (state1->domeColor0.n[axis] - state0->domeColor0.n[axis]) * fraction;
    }
    masterState.fogDist = state0->fogDist + (state1->fogDist - state0->fogDist) * fraction;
}

void zCSkyControler_Outdoor::SetTime(float value)
{
    const float delta = lastRelightTime > value ? lastRelightTime - value : value - lastRelightTime;
    masterTime = value;
    if (delta >= m_fRelightTime) {
        ++relightCtr;
        lastRelightTime = value;
    }
}

void zCSkyControler_Outdoor::ResetTime()
{
    SetTime(0);
    dayCounter = 0;
}

zCOLOR zCSkyControler_Outdoor::GetDaylightColorFromIntensity(int intensity)
{
    return polyLightCLUT[intensity];
}

void zCSkyControler_Outdoor::CalcPolyLightCLUT(const zVEC3& first, const zVEC3& second)
{
    int red = static_cast<int>(first.n[0] * 65536.0f);
    int green = static_cast<int>(first.n[1] * 65536.0f);
    int blue = static_cast<int>(first.n[2] * 65536.0f);
    const int deltaRed = static_cast<int>((second.n[0] - first.n[0]) * 256.0f);
    const int deltaGreen = static_cast<int>((second.n[1] - first.n[1]) * 256.0f);
    const int deltaBlue = static_cast<int>((second.n[2] - first.n[2]) * 256.0f);
    for (int i = 0; i < 256; ++i) {
        red += deltaRed;
        green += deltaGreen;
        blue += deltaBlue;
        polyLightCLUT[i].r = red >> 16;
        polyLightCLUT[i].g = green >> 16;
        polyLightCLUT[i].b = blue >> 16;
        polyLightCLUT[i].a = 255;
    }
}

static float ClampUnit(float value)
{
    if (value < 0)
        return 0;
    if (value > 1.0f)
        return 1.0f;
    return value;
}

static float SkyRandom()
{
    return static_cast<float>(rand()) / 32768.0f;
}

void zCSkyControler_Outdoor::InitSkyPFX()
{
    skyPFXTimer = 0;
    zCParticleFX* particleFx = new zCParticleFX;
    vobSkyPFX = new zCVob;
    vobSkyPFX->SetCollDetStat(0);
    vobSkyPFX->SetCollDetDyn(0);
    vobSkyPFX->RotateLocalY(45.0f);
    vobSkyPFX->SetPositionWorld(zVEC3(0, 500.0f, -1000.0f));
    vobSkyPFX->SetVisual(particleFx);
    particleFx->SetEmitter("ZSKYNIGHT1", 0);
    particleFx->CreateParticles();
    particleFx->Release();
}

void zCSkyControler_Outdoor::CalcFog()
{
    if (GetUnderwaterFX()) {
        resultFogColor = underwaterColor;
        resultFogColorOverride = underwaterColor;
        resultFogFar = underwaterFarZ;
        resultFogSkyFar = 1000.0f;
        resultFogSkyNear = 0;
        resultFogNear = 0;
        return;
    }

    const float cameraY = zCCamera::activeCam->connectedVob->trafoObjToWorld.v[1].n[3];
    const float heightRange = heightFogMaxY - heightFogMinY;
    resultFogScale = heightRange != 0 ? ClampUnit((cameraY - heightFogMinY) / heightRange) : 0;
    const float nearDistance = userFogFar * 0.4f;
    const float farDistance = userFogFar - nearDistance;
    resultFogFar = ((1.0f - resultFogScale) * farDistance + nearDistance) * userFarZScalability;
    resultFogNear = resultFogFar * 0.3f;
    if (rainFX.outdoorRainFXWeight > 0)
        resultFogNear += (userFogFar * 0.1f - resultFogNear) * rainFX.outdoorRainFXWeight;

    float skyFar = resultFogFar * 1.5f;
    if (skyFar > 40000.0f)
        skyFar = 40000.0f;
    float skyNear = resultFogNear * 1.5f;
    if (skyNear > 10000.0f)
        skyNear = 10000.0f;
    skyNear *= 0.5f;
    resultFogSkyFar = skyNear + (skyFar * 0.5f - skyNear) * resultFogScale;
    resultFogSkyNear = resultFogSkyFar * 0.25f;
    if (rainFX.outdoorRainFXWeight > 0) {
        resultFogSkyNear = resultFogSkyFar * 0.1f;
        resultFogSkyFar *= 0.8f;
    }

    float gray = masterState.fogColor.n[0];
    if (gray > 120.0f)
        gray = 120.0f;
    const float fogBlend = resultFogScale * 0.5f;
    zVEC3 fogColor = masterState.fogColor * (1.0f - fogBlend) + zVEC3(gray, gray, gray) * fogBlend;
    if (rainFX.outdoorRainFXWeight > 0)
        fogColor += (zVEC3(gray, gray, gray) - fogColor) * rainFX.outdoorRainFXWeight;
    resultFogColor.r = static_cast<unsigned char>(fogColor.n[0]);
    resultFogColor.g = static_cast<unsigned char>(fogColor.n[1]);
    resultFogColor.b = static_cast<unsigned char>(fogColor.n[2]);
    resultFogColor.a = 255;

    float overrideGray = m_overrideColor.n[0] * 0.299f + m_overrideColor.n[1] * 0.587f +
        m_overrideColor.n[2] * 0.114f;
    if (overrideGray > 120.0f)
        overrideGray = 120.0f;
    zVEC3 overrideColor = m_overrideColor * (1.0f - fogBlend) +
        zVEC3(overrideGray, overrideGray, overrideGray) * fogBlend;
    if (rainFX.outdoorRainFXWeight > 0)
        overrideColor += (zVEC3(overrideGray, overrideGray, overrideGray) - overrideColor) *
            rainFX.outdoorRainFXWeight;
    resultFogColorOverride.r = static_cast<unsigned char>(overrideColor.n[0]);
    resultFogColorOverride.g = static_cast<unsigned char>(overrideColor.n[1]);
    resultFogColorOverride.b = static_cast<unsigned char>(overrideColor.n[2]);
    resultFogColorOverride.a = 255;
}

void zCSkyControler_Outdoor::SetFarZ(float farZ)
{
    userFogFar = farZ;
}

float zCSkyControler_Outdoor::GetFarZ() const
{
    return userFogFar;
}

void zCSkyControler_Outdoor::SetFarZScalability(float scalability)
{
    if (scalability < 0)
        scalability = 0;
    else if (scalability > 4.0f)
        scalability = 4.0f;
    userFarZScalability = scalability;
}

float zCSkyControler_Outdoor::GetFarZScalability() const
{
    return userFarZScalability;
}

void zCSkyControler_Outdoor::RenderSkyPFX()
{
    if (masterTime < 0.3f || masterTime >= 0.65f || rainFX.outdoorRainFXWeight >= 0.4f)
        return;
    zCParticleFX* particleFx = static_cast<zCParticleFX*>(vobSkyPFX->visual);
    if (particleFx->firstPart)
        return;
    if (skyPFXTimer > 0) {
        skyPFXTimer -= ztimer.frameTimeFloat;
        return;
    }
    skyPFXTimer = 500.0f + SkyRandom() * 2000.0f;
    zVEC3 direction = zCCamera::activeCam->connectedVob->trafoObjToWorld.GetAtVector();
    zVEC3 right = zCCamera::activeCam->connectedVob->trafoObjToWorld.GetRightVector();
    direction.n[1] = 0;
    right.n[1] = 0;
    direction.NormalizeApprox();
    right.NormalizeApprox();
    zVEC3 position = direction + right;
    position += right * ((SkyRandom() * 2.0f - 1.0f) * 0.5f);
    position.n[1] += SkyRandom() + 0.1f;
    position.NormalizeApprox();
    vobSkyPFX->SetPositionWorld(position * 800.0f);
    particleFx->CreateParticles();
}

void zCSkyControler_Outdoor::CalcGlobalWind()
{
    if (!m_bIsMainControler || ztimer.frameTimeFloat <= 0)
        return;
    static float cycleTime = 20.0f;
    static float cycleTimeVar = 10.0f;
    static float strength = 1.0f;
    static float strengthVar = 0.5f;
    static float angleVelocity = 0.1f;
    static float angleVelocityVar = 0.05f;
    static float currentCycleTime = cycleTime;
    static float currentStrength = strength;
    static float currentAngleVelocity = angleVelocity;
    static float cyclePosition = 0;
    static float cycleDirection = 1.0f;
    static int selectNewCycle = 1;
    if (selectNewCycle) {
        currentCycleTime = cycleTime + (SkyRandom() * 2.0f - 1.0f) * cycleTimeVar;
        currentStrength = strength + (SkyRandom() * 2.0f - 1.0f) * strengthVar;
        currentAngleVelocity = angleVelocity + (SkyRandom() * 2.0f - 1.0f) * angleVelocityVar;
        if (SkyRandom() > 0.7f) {
            currentStrength *= 0.35f;
            currentAngleVelocity *= 0.35f;
        }
        selectNewCycle = 0;
    }
    cyclePosition += ztimer.frameTimeFloat * 0.001f * cycleDirection;
    if (cyclePosition >= currentCycleTime) {
        cyclePosition = currentCycleTime;
        cycleDirection = -cycleDirection;
    } else if (cyclePosition <= 0) {
        cyclePosition = 0;
        cycleDirection = -cycleDirection;
        selectNewCycle = 1;
    }
    const float phase = cyclePosition / currentCycleTime * 3.14159274f - 1.57079637f;
    const float angle = (std::sin(phase) + 1.0f) * currentAngleVelocity * 0.5f;
    const float magnitude = (std::sin(phase) + 1.0f) * currentStrength * 0.5f;
    m_bWindVec.n[0] = std::cos(angle) * magnitude;
    m_bWindVec.n[1] = 0;
    m_bWindVec.n[2] = std::sin(angle) * magnitude;
}

int zCSkyControler_Outdoor::GetGlobalWindVec(zVEC3& windVec, int)
{
    if (!m_bIsMainControler)
        return 0;
    windVec = m_bWindVec;
    return 1;
}

static float cloudShadowOverrideScale;
static int cloudShadowOverrideEnabled;

float zCSkyControler_Outdoor::GetCloudShadowScale() const
{
    return cloudShadowOverrideEnabled ? cloudShadowOverrideScale : cloudShadowScale;
}

void zCSkyControler_Outdoor::SetCloudShadowScale(float scale)
{
    cloudShadowOverrideScale = scale;
    if (!cloudShadowOverrideEnabled) {
        cloudShadowScale = scale;
        cloudShadowOverrideEnabled = 1;
    }
    m_bColorChanged = 1;
}

void zCSkyControler_Outdoor::RenderSkyPre()
{
    if (!initDone)
        Init();
    zCSkyControler_Mid::RenderSkyPre();
    CalcGlobalWind();
    if (masterTimeLast - masterTime >= 0.95f && masterTime < 0.02f) {
        rainFX.timeStartRain = SkyRandom();
        if (rainFX.timeStartRain > 0.958f)
            rainFX.timeStartRain = 0.958f;
        rainFX.timeStopRain = rainFX.timeStartRain + 0.042f + SkyRandom() * 0.06f;
        if (rainFX.timeStopRain > 1.0f)
            rainFX.timeStopRain = 1.0f;
        rainFX.renderLightning = rainFX.m_iRainCtr > 3 && SkyRandom() > 0.6f;
        dayCounter += 1.0f;
    }
    if (masterTime != masterTimeLast)
        Interpolate();
    CalcFog();
    ClearBackground(GetBackgroundColor());
    RenderSky();
    RenderSkyPFX();
    RenderPlanets(1.0f);
    if (m_bSunVisible) {
        m_fFadeScale += ztimer.frameTimeFloat * 0.005f;
        if (m_fFadeScale > 1.0f)
            m_fFadeScale = 1.0f;
    } else if (m_fFadeScale > 0) {
        m_fFadeScale -= ztimer.frameTimeFloat * 0.005f;
        if (m_fFadeScale < 0)
            m_fFadeScale = 0;
    }
    masterTimeLast = masterTime;
}

void zCSkyControler_Outdoor::ProcessRainFX()
{
    if (!s_skyEffectsEnabled || m_bDontRain) {
        rainFX.outdoorRainFXWeight = 0;
        return;
    }
    if (masterTime < rainFX.timeStartRain || masterTime >= rainFX.timeStopRain) {
        rainFX.outdoorRainFXWeight = 0;
    } else {
        const float rainPhase = (masterTime - rainFX.timeStartRain) /
            (rainFX.timeStopRain - rainFX.timeStartRain);
        if (rainPhase < 0.2f)
            rainFX.outdoorRainFXWeight = rainPhase * 5.0f;
        else if (rainPhase < 0.8f)
            rainFX.outdoorRainFXWeight = 1.0f;
        else
            rainFX.outdoorRainFXWeight = (1.0f - rainPhase) * 5.0f;
    }
    if (rainFX.outdoorRainFXWeight <= 0 &&
        (rainFX.outdoorRainFX->m_numImpactParticle <= 0 || rainFX.outdoorRainFX->m_numFlyParticle > 0)) {
        rainFX.m_bRaining = 0;
        return;
    }
    if (!rainFX.m_bRaining) {
        ++rainFX.m_iRainCtr;
        rainFX.m_bRaining = 1;
    }

    float soundTarget = rainFX.outdoorRainFXWeight;
    if (rainFX.camLocationHint || GetUnderwaterFX())
        soundTarget *= 0.25f;
    const float soundDelta = ztimer.frameTimeFloat * 0.0005f;
    if (rainFX.soundVolume < soundTarget)
        rainFX.soundVolume += soundDelta;
    else if (rainFX.soundVolume > soundTarget)
        rainFX.soundVolume -= soundDelta;
    rainFX.soundVolume = ClampUnit(rainFX.soundVolume);
    if (rainFX.camLocationHint == 2)
        rainFX.timerInsideSectorCantSeeOutside += ztimer.frameTimeFloat;
    else
        rainFX.timerInsideSectorCantSeeOutside = 0;

    rainFX.outdoorRainFX->SetEffectWeight(rainFX.outdoorRainFXWeight, rainFX.soundVolume);
    rainFX.outdoorRainFX->UpdateParticles();
    zTRenderContext context;
    rainFX.outdoorRainFX->CreateParticles(context);
    if (rainFX.camLocationHint != 2) {
        const zCOLOR daylight = GetDaylightColorFromIntensity(255);
        const float gray = (daylight.r * 0.11328125f + daylight.g * 0.5859375f +
            daylight.b * 0.30078125f);
        zCOLOR rainColor;
        rainColor.r = static_cast<unsigned char>(gray + (daylight.r - gray) * 0.4296875f);
        rainColor.g = static_cast<unsigned char>(gray + (daylight.g - gray) * 0.4296875f);
        rainColor.b = static_cast<unsigned char>(gray + (daylight.b - gray) * 0.4296875f);
        rainColor.a = 255;
        rainFX.outdoorRainFX->RenderParticles(context, rainColor);
    }
}

void zCSkyControler_Outdoor::SetRainFXWeight(float weight, float velocityScale)
{
    if (!s_skyEffectsEnabled || m_bDontRain)
        return;
    weight = ClampUnit(weight);
    velocityScale = ClampUnit(velocityScale);
    rainFX.timeStartRain = masterTime - weight * velocityScale;
    rainFX.timeStopRain = masterTime + (1.0f - weight) * velocityScale;
    rainFX.renderLightning = m_enuWeather != zTWEATHER_RAIN && SkyRandom() > 0.7f;
    rainFX.timeStartRain = ClampUnit(rainFX.timeStartRain);
    rainFX.timeStopRain = ClampUnit(rainFX.timeStopRain);
}

void zCSkyControler_Outdoor::RenderSetup()
{
    ClearBackground(GetBackgroundColor());
}

void zCSkyControler_Outdoor::RenderSky()
{
    for (int i = 0; i < 2; ++i) {
        if (skyLayer[i].skyPoly->material->color[3])
            skyLayer[i].RenderSkyLayer(skyLayerState[i]);
    }
    int rainAlpha = static_cast<int>(rainFX.outdoorRainFXWeight * 255.0f * 2.0f);
    if (rainAlpha > 255)
        rainAlpha = 255;
    if (rainAlpha > 0) {
        zCOLOR rainColor;
        rainColor.r = static_cast<unsigned char>(masterState.domeColor1.n[0]);
        rainColor.g = static_cast<unsigned char>(masterState.domeColor1.n[1]);
        rainColor.b = static_cast<unsigned char>(masterState.domeColor1.n[2]);
        rainColor.a = static_cast<unsigned char>(rainAlpha);
        skyLayerRainClouds.RenderRainCloudLayer(rainColor);
    }
}

void zCSkyControler_Outdoor::RenderSkyPost(int renderSky)
{
    ProcessRainFX();
    if (renderSky)
        RenderSky();
    zCSkyControler_Mid::RenderSkyPost(renderSky);
    m_bSkyScaleChanged = 0;
}

void zCSkyControler_Outdoor::Archive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    archiver.WriteFloat(masterTime);
    archiver.WriteFloat(rainFX.outdoorRainFXWeight);
    archiver.WriteFloat(rainFX.timeStartRain);
    archiver.WriteFloat(rainFX.timeStopRain);
    archiver.WriteFloat(rainFX.timerInsideSectorCantSeeOutside);
    archiver.WriteFloat(rainFX.soundVolume);
    archiver.WriteFloat(dayCounter);
    archiver.WriteFloat(m_fFadeScale);
    archiver.WriteBool(rainFX.renderLightning);
    archiver.WriteBool(rainFX.m_bRaining);
    archiver.WriteInt(rainFX.m_iRainCtr);
}

void zCSkyControler_Outdoor::Unarchive(zCArchiver& archiver)
{
    if (!archiver.InSaveGame())
        return;
    masterTime = archiver.ReadFloat();
    rainFX.outdoorRainFXWeight = archiver.ReadFloat();
    rainFX.timeStartRain = archiver.ReadFloat();
    rainFX.timeStopRain = archiver.ReadFloat();
    rainFX.timerInsideSectorCantSeeOutside = archiver.ReadFloat();
    rainFX.soundVolume = archiver.ReadFloat();
    dayCounter = archiver.ReadFloat();
    m_fFadeScale = archiver.ReadFloat();
    rainFX.renderLightning = archiver.ReadBool();
    rainFX.m_bRaining = archiver.ReadBool();
    rainFX.m_iRainCtr = archiver.ReadInt();
}

void zCSkyControler_Outdoor::SetWeatherType(zTWeather weather)
{
    if (weather == m_enuWeather || rainFX.outdoorRainFXWeight > 0)
        return;
    if (weather == zTWEATHER_SNOW)
        SetRainFXWeight(0, 0.1f);
    m_enuWeather = weather;
    rainFX.outdoorRainFX->SetWeatherType(weather);
}
