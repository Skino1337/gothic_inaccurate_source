#ifndef ZSKY_H
#define ZSKY_H

#include "z3d.h"
#include "zParticle.h"

struct zCOLOR {
    zCOLOR(int value);
    zCOLOR(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);

    union {
        struct {
            unsigned char b;
            unsigned char g;
            unsigned char r;
            unsigned char a;
        };
        unsigned long dword;
    };

    void    Reset();
    zSTRING GetDescription() const;
    void    SetRGB(unsigned char red, unsigned char green, unsigned char blue);
    void    SetByDescription(const zSTRING& description);
    zSTRING GetDescriptionRGB() const;
};

enum zTWeather {
    zTWEATHER_SNOW,
    zTWEATHER_RAIN
};

class zCSkyControler : public zCObject {
public:

    zCSkyControler();
    virtual ~zCSkyControler();

    virtual zCClassDef* _GetClassDef() const;
    void               ClearBackground(zCOLOR color);
    virtual float GetCloudShadowScale() const;
    virtual void  SetCloudShadowScale(float scale);
    virtual void  SetFillBackground(int fill);
    virtual int   GetFillBackground() const;
    virtual void  SetLightDirty();
    virtual void  SetRelightTime(float time);
    virtual int   GetRelightCtr();
    virtual zCOLOR GetDaylightColorFromIntensity(int intensity);
    virtual int   GetGlobalWindVec(zVEC3& direction, int animationMode);
    virtual void  SetGlobalSkyScale(float scale);
    virtual float GetGlobalSkyScale() const;
    virtual int   GetGlobalSkyScaleChanged() const;
    virtual void  SetCameraLocationHint(int hint);
    virtual void  SetWeatherType(zTWeather weather);
    virtual zTWeather GetWeatherType() const;
    virtual void  SetTime(float time) = 0;

    static zCClassDef*     classDef;
    static zCSkyControler* s_activeSkyControler;
    static int             s_skyEffectsEnabled;

    zCOLOR*       polyLightCLUTPtr;
    float         cloudShadowScale;
    int           m_bColorChanged;
    zTWeather     m_enuWeather;
    zCOLOR        backgroundColor;
    int           fillBackground;
    zCTexture*    backgroundTexture;
    int           relightCtr;
    float         lastRelightTime;
    float         m_fRelightTime;
};

class zCSkyControler_Mid : public zCSkyControler {
public:

    zCSkyControler_Mid();
    virtual ~zCSkyControler_Mid();

    zCClassDef* _GetClassDef() const;
    void        InitUnderwaterPFX();
    void        InitScreenBlend();
    void        RenderScreenBlend();
    void        SetScreenBlendAlpha(int alpha);
    void        SetScreenBlendColor(const zCOLOR& color);
    void        SetScreenBlendAlphaFunc(int alphaFunc);
    void        SetUnderwaterFX(int enabled);
    int         GetUnderwaterFX() const;
    void        RenderSkyPre();
    void        RenderSkyPost(int flags);

    static zCClassDef* classDef;

    int        underwaterFX;
    zCOLOR     underwaterColor;
    float      underwaterFarZ;
    float      underwaterStartTime;
    float      oldFovX;
    float      oldFovY;
    zCVob*     vobUnderwaterPFX;
    zCPolygon* scrPoly;
    zCMesh*    scrPolyMesh;
    int        scrPolyAlpha;
    zCOLOR     scrPolyColor;
    int        scrPolyAlphaFunc;
};

class zCSkyControler_Indoor : public zCSkyControler_Mid {
public:

    zCSkyControler_Indoor();
    virtual ~zCSkyControler_Indoor();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    float            GetTime() const;
    void             ResetTime();
    void             SetTime(float time);
    void             SetOverrideColor(zVEC3 color);
    void             SetOverrideColorFlag(int enabled);
    void             UpdateWorldDependencies();
    void             SetFarZ(float farZ);
    float            GetFarZ() const;
    void             SetFarZScalability(float scalability);
    float            GetFarZScalability() const;
    void             SetBackgroundColor(zCOLOR color);
    zCOLOR           GetBackgroundColor() const;
    zCOLOR           GetBackgroundColorDef() const;
    void             RenderSkyPre();
    void             RenderSkyPost(int flags);

    float userFarZ;
    float userFarZScalability;
    float time;

    static zCClassDef* classDef;
};

class zCUnderwaterPFX : public zCParticleFX {
public:

    virtual ~zCUnderwaterPFX();

    int  Render(zTRenderContext& context);
    void ProcessParticles();
    void CreateParticles();

    zVEC3 camPosLastFrame;
};

#endif
