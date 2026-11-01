#ifndef ZMORPHMESH_H
#define ZMORPHMESH_H

#include "zModel.h"

class zCOBBox3D;
class zCMorphMesh;
class zCMorphMeshProto;

class zCMorphMeshAni {
public:

    zCMorphMeshAni();
    ~zCMorphMeshAni();

    void SetBlending(float blendIn, float blendOut);
    void Load(zCFileBIN& file);
    void Save(zCFileBIN& file);
    void SetFlagsByString(const zSTRING& flagString);

    zSTRING aniName;
    zSTRING ascName;
    float   blendInSpeed;
    float   blendOutSpeed;
    float   holdTime;
    int     layer;
    float   fpsRate;
    struct {
        unsigned char discontinuity : 1;
        unsigned char looping : 1;
        unsigned char shape : 1;
        unsigned char refShape : 1;
    } flags;
    int    numVert;
    int*   vertIndexList;
    int    numFrames;
    zVEC3* vertPosMatrix;
};

class zCMorphMeshProto {
public:

    zCMorphMeshProto();
    ~zCMorphMeshProto();

    int             Release();
    zCMorphMeshAni* SearchAni(const zSTRING& name);
    int             LoadMMB(const zSTRING& fileName);
    void            SaveMMB(const zSTRING& fileName);
    zCMorphMeshAni* GenerateMorphAni(zCMesh* mesh, const zCArray<int>& vertIndexList,
        int numFrames);
    zCMesh*         Load3DSMAXAsc(const zSTRING& fileName, float& fps, int firstFrame,
        int lastFrame);
    void            ReadMorphRef(zCArray<int>& vertIndexList);
    void            ReadMorphAni(const zCArray<int>& vertIndexList);
    void            LoadMorphScript(const zSTRING& fileName);

    static zCMorphMeshProto* SearchName(const zSTRING& name);
    static zCMorphMeshProto* Load(const zSTRING& fileName);

    zCMorphMeshProto*          next;
    zCMorphMeshProto*          prev;
    int                        refCtr;
    zSTRING                    morphProtoName;
    zCProgMeshProto*           morphRefMesh;
    zVEC3*                     morphRefMeshVertPos;
    zCArraySort<zCMorphMeshAni*> aniList;
    int                        m_bAlphaTestingEnabled;

    static int              autoConvertBinaryFile;
    static zCMorphMeshProto* morphRoot;
};

struct zTMorphAniEntry {
    zCMorphMeshAni* ani;
    float           weight;
    float           weightDest;
    float           actFrame;
    int             actFrameInt;
    int             nextFrameInt;
    float           frac;
    float           blendInSpeed;
    float           blendOutSpeed;
    float           holdTime;
    int             blendState;
};

struct zTMorphRandAni {
    float           randAniTimer;
    zCMorphMeshAni* randAni;
    float           randAniTimeMin;
    float           randAniTimeMinVar;
    float           randAniTimeMax;
    float           randAniTimeMaxVar;
    float           randAniProbMin;
};

class zCMorphMesh : public zCVisual {
public:

    zCMorphMesh();
    zCMorphMesh(zCMorphMeshProto* proto);
    virtual ~zCMorphMesh();

    void            Init();
    zCMorphMeshAni* SearchAni(const zSTRING& name);
    int             Render(zTRenderContext& context);
    int             Render(zTRenderContext& context, zCRenderLightContainer* lights,
        float fatness, float scale);
    void            CalcVertPositions();
    void            AdvanceAnis();
    void            ProcessRandAnis();
    void            StartAni(const zSTRING& name, float blendIn, float blendOut);
    void            StartAni(zCMorphMeshAni* ani, float blendIn, float blendOut);
    void            StopAni(const zSTRING& name);
    void            StopAni(zCMorphMeshAni* ani);
    void            FadeOutAni(const zSTRING& name);
    void            FadeOutAni(zCMorphMeshAni* ani);
    int             IsAniActive(const zSTRING& name);
    int             IsAniActive(zCMorphMeshAni* ani);
    const zSTRING*  GetAnyAnimation();
    void            AddRandAni(zCMorphMeshAni* ani, float timeMin, float timeMinVar,
        float timeMax, float timeMaxVar, float probMin);
    void            RemoveRandAni(zCMorphMeshAni* ani);
    void            SetTextureVar(int channel, int frame, zSTRING* texture);
    int             TraceRay(const zVEC3& origin, const zVEC3& direction, int flags,
        struct zTTraceRayReport& report);
    void            PrintStatus(int x, int y);
    int             IsBBox3DLocal();
    zCOBBox3D*      GetOBBox3D();
    unsigned long   GetRenderSortKey() const;
    int             GetAlphaTestingEnabled() const;
    void            SetAlphaTestingEnabled(int enabled);
    int             CanTraceRay() const;
    void            StartAnimation(const zSTRING& name);
    void            StopAnimation(const zSTRING& name);
    int             IsAnimationActive(const zSTRING& name);
    zTBBox3D        GetBBox3D();
    zSTRING         GetVisualName();
    const zSTRING*  GetFileExtension(int index);
    zCVisual*       LoadVisualVirtual(const zSTRING& fileName) const;

    static zCMorphMesh* Load(const zSTRING& fileName);
    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;

    zCMorphMeshProto*             morphProto;
    zCProgMeshProto*              morphMesh;
    zCMorphMeshAni*               refShapeAni;
    zCModelTexAniState            texAniState;
    int                           m_bUsesAlphaTesting;
    zCArraySort<zTMorphAniEntry*> aniChannels;
    zCArray<zTMorphRandAni>       randAniList;

    static zCClassDef* classDef;
};

class zCView {
public:

    void Print(int x, int y, const zSTRING& text);
};

extern zCView* screen;

class zCMorphMeshConvertFileHandler : public zCScanDirFileHandler {
public:

    zCMorphMeshConvertFileHandler();
    virtual ~zCMorphMeshConvertFileHandler() {}

    virtual int HandleFile(const zSTRING& path, const char* fileName,
        const zFINDDATA& findData);
};

zCProgMeshProto* zDYNAMIC_CAST(const zCObject* object);

#endif
