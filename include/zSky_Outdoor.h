#ifndef ZSKY_OUTDOOR_H
#define ZSKY_OUTDOOR_H

#include "zSky.h"
#include "zRenderManager.h"

class zCSkyControler_Outdoor;

class zCOutdoorRainFX {
public:

    zCOutdoorRainFX();
    ~zCOutdoorRainFX();

    void UpdateSound(float volume);
    void SetWeatherType(zTWeather weather);
    void SetEffectWeight(float weight, float velocityScale);
    int  CheckCameraBeam(const zVEC3& delta);
    void CreateParticles(zTRenderContext& context);
    void UpdateParticles();
    void RenderParticles(zTRenderContext& context, zCOLOR color);

    zCArray<zSParticle*> m_freeFlyParticleList;
    zCShader            m_shader;
    zCShaderStage       m_shaderStage;
    zSParticle          m_flyParticleList[0x400];
    zVEC3               m_camPosLastFrame;
    zVEC3               m_positionUpdateVector;
    float               m_effectWeight;
    int                 m_numDestFlyParticle;
    int                 m_numFlyParticle;
    int                 m_numImpactParticle;
    float               m_timeLen;
    float               m_velocityLen;
    zCSoundFX*          m_soundEffect;
    int                 m_soundActive;
    zCTexture*          m_raindropTexture;
    zCTexture*          m_rainTexture;
    zCMaterial*         m_rainMaterial;
    int                 m_currentCacheIndex;
    int                 m_soundHandle;
    zTWeather           m_enuWeather;
};

class zCSkyLayerData {
public:

    zCSkyLayerData();
    ~zCSkyLayerData();

    int       skyMode;
    zCTexture* tex;
    zSTRING   texName;
    float     texAlpha;
    float     texScale;
    zVEC2     texSpeed;
};

class zCSkyState {
public:

    zCSkyState();
    ~zCSkyState();

    void PresetDay0();
    void PresetDay1();
    void PresetDay2();
    void PresetEvening();
    void PresetNight0();
    void PresetNight1();
    void PresetNight2();
    void PresetDawn();

    float time;
    zVEC3 polyColor;
    zVEC3 fogColor;
    zVEC3 domeColor1;
    zVEC3 domeColor0;
    float fogDist;
    int   sunOn;
    int   cloudShadowOn;
    zCSkyLayerData layer[2];
};

class zCSkyLayer {
public:

    zCSkyLayer();
    ~zCSkyLayer();

    static zCMesh* CreateSkyPoly();
    static zCMesh* CreateSkyDomeMesh();
    void SetDomeMeshEnabled(int enabled);
    void SetSkyDomeTexture(zCTexture* texture);
    void SetSkyPolyTexture(zCTexture* texture);
    void RenderSkyLayer(zCSkyState* state);
    void RenderSkyPoly(zCSkyState* state);
    void RenderSkyDome(zCSkyState* state);
    void ColorizeSkyDome();
    void RenderRainCloudLayer(zCOLOR color);

    zCMesh*    skyPolyMesh;
    zCPolygon* skyPoly;
    zVEC2      skyTexOffs;
    zCMesh*    skyDomeMesh;
    int        skyMode;
};

class zCSkyPlanet {
public:

    zCSkyPlanet();
    ~zCSkyPlanet();

    zCMesh* mesh;
    zVEC4   color0;
    zVEC4   color1;
    float   size;
    zVEC3   pos;
    zVEC3   rotAxis;
};

struct zTOutdoorRainFXState {
    zCOutdoorRainFX* outdoorRainFX;
    int              camLocationHint;
    float            outdoorRainFXWeight;
    float            soundVolume;
    float            timerInsideSectorCantSeeOutside;
    float            timeStartRain;
    float            timeStopRain;
    int              renderLightning;
    int              m_bRaining;
    int              m_iRainCtr;
};

class zCSkyControler_Outdoor : public zCSkyControler_Mid {
public:

    zCSkyControler_Outdoor(int mainControler = 0);
    virtual ~zCSkyControler_Outdoor();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    float            GetTime() const;
    void             SetBackgroundColor(zCOLOR color);
    zCOLOR           GetBackgroundColor() const;
    zCOLOR           GetBackgroundColorDef() const;
    void             SetOverrideColor(zVEC3 color);
    void             SetOverrideColorFlag(int enabled);
    int              GetRenderLightning() const;
    void             SetCameraLocationHint(int hint);
    float            GetGlobalSkyScale() const;
    int              GetGlobalSkyScaleChanged() const;
    void             Init();
    void             SetGlobalSkyScale(float scale);
    void             UpdateWorldDependencies();
    int              GetStateTextureSearch(int start, int layer, int forward);
    void             ApplyStateTexToLayer(int stateIndex, int layerIndex);
    void             CreateDefault();
    void             RenderPlanets(float scale);
    void             ReadFogColorsFromINI();
    void             ApplyFogColorsFromINI(int frame);
    void             Interpolate();
    void             SetTime(float time);
    void             ResetTime();
    zCOLOR           GetDaylightColorFromIntensity(int intensity);
    void             CalcPolyLightCLUT(const zVEC3& first, const zVEC3& second);
    void             InitSkyPFX();
    void             CalcFog();
    void             SetFarZ(float farZ);
    float            GetFarZ() const;
    void             SetFarZScalability(float scalability);
    float            GetFarZScalability() const;
    void             RenderSkyPFX();
    void             CalcGlobalWind();
    int              GetGlobalWindVec(zVEC3& windVec, int animationMode);
    float            GetCloudShadowScale() const;
    void             SetCloudShadowScale(float scale);
    void             RenderSkyPre();
    void             ProcessRainFX();
    void             SetRainFXWeight(float weight, float velocityScale);
    void             RenderSetup();
    void             RenderSky();
    void             RenderSkyPost(int renderSky);
    void             Archive(zCArchiver& archiver);
    void             Unarchive(zCArchiver& archiver);
    void             SetWeatherType(zTWeather weather);

    int            initDone;
    float          masterTime;
    float          masterTimeLast;
    zCSkyState     masterState;
    zCSkyState*    state0;
    zCSkyState*    state1;
    zCArray<zCSkyState*> stateList;
    zCOLOR         polyLightCLUT[256];
    float          dayCounter;
    zCArray<zVEC3> fogColorDayVariations;
    zCArray<zVEC3> fogColorDayVariations2;
    int            m_bDarkSky;
    float          m_fSkyScale;
    int            m_bSkyScaleChanged;
    zVEC3          m_overrideColor;
    int            m_bOverrideColorFlag;
    int            m_bLevelChanged;
    float          heightFogMinY;
    float          heightFogMaxY;
    float          resultFogScale;
    float          resultFogNear;
    float          resultFogFar;
    float          resultFogSkyNear;
    float          resultFogSkyFar;
    zCOLOR         resultFogColor;
    zCOLOR         resultFogColorOverride;
    float          userFogFar;
    float          userFarZScalability;
    zCSkyState*    skyLayerState[2];
    zCSkyLayer     skyLayer[2];
    zCSkyLayer     skyLayerRainClouds;
    zCTexture*     skyCloudLayerTex;
    zCSkyPlanet    planets[2];
    int            m_bSunVisible;
    float          m_fFadeScale;
    zCVob*         vobSkyPFX;
    float          skyPFXTimer;
    int            m_bIsMainControler;
    zVEC3          m_bWindVec;
    int            m_bDontRain;
    zTOutdoorRainFXState rainFX;

    static zCClassDef* classDef;
};

#endif
