#ifndef ZFLASH_H
#define ZFLASH_H

#include "zPolyStrip.h"

class zCFlash : public zCVisual {
public:
    class zCBolt : public zCPolyStrip {
    public:
        zCBolt();
        virtual ~zCBolt();
        virtual int Render(zTRenderContext& context);
        virtual void FreeResources();
        void SetProjectionSphere(const zTBSphere3D& sphere);
        void Generate_R(const zVEC3& start, const zVEC3& end, float roughness, int depth);
        void SetTexture(const zSTRING& textureName);
        void InterpolateTo(zCBolt* other, float amount);
        void Copy(zCBolt* other);

        zCArray<zCBolt*> childList;
        int isRootBolt;
        int numberOfChilds;
        float roughness;
        int boltIndex;
        zTBSphere3D projectionSphere;
        float aniPhase;
    };

    zCFlash();
    virtual ~zCFlash();
    virtual int Render(zTRenderContext& context);
    virtual int IsBBox3DLocal();
    virtual zTBBox3D GetBBox3D();
    virtual zSTRING GetVisualName();
    virtual int GetVisualDied();
    virtual void SetVisualUsedBy(zCVob* vob);
    virtual void DynLightVisual(const zCArray<zCVobLight*>& lights, zMAT4* trafo);
    void SetProjectionSphere(const zTBSphere3D& sphere);
    void Generate(zCBolt* bolt, const zVEC3& start, const zVEC3& end, float roughness);
    void SetTexture(const zSTRING& textureName);
    void Update(float regenerateTime);
    void Create(zVEC3 start, zVEC3 end, float roughness);

    float timeAlive;
    zVEC3 step;
    int recalcStep;
    zCBolt* rootBolt;
    zCBolt* splitBolt;
    zVEC3 boltStartPos;
    zVEC3 boltEndPos;
    float boltRoughness;
    zSTRING textureName;
    float LastUpdateTime;
    zTBBox3D bbox3D;
};

#endif
