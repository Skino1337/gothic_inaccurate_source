#ifndef ZMODEL_H
#define ZMODEL_H

#include "zFile3d.h"
#include "zObject.h"
#include <iosfwd>

class zCOBBox3D;
struct zCVertex;
struct zCVertFeature;
struct zVEC4;
struct zMAT4;
class zCVobLight;
class zCBspSector;

const int zMDL_ANIEVENT_MAXSTRING = 4;

extern const zSTRING zMDL_NODE_NAME_BIP_PREFIX;
extern const zSTRING zMDL_NODE_NAME_R_HAND;
extern const zSTRING zMDL_NODE_NAME_L_HAND;
extern const zSTRING zMDL_NODE_NAME_R_FOOT;
extern const zSTRING zMDL_NODE_NAME_L_FOOT;
extern const zSTRING zMDL_NODE_NAME_HEAD;
extern const zSTRING zMDL_NODE_NAME_FOOTSTEPS;

enum zTMdl_AniEventType {
    zMDL_EVENT_TAG,
    zMDL_EVENT_SOUND,
    zMDL_EVENT_SOUND_GRND,
    zMDL_EVENT_ANIBATCH,
    zMDL_EVENT_SWAPMESH,
    zMDL_EVENT_HEADING,
    zMDL_EVENT_PFX,
    zMDL_EVENT_PFX_GRND,
    zMDL_EVENT_PFX_STOP,
    zMDL_EVENT_SETMESH,
    zMDL_EVENT_MM_STARTANI,
    zMDL_EVENT_CAM_TREMOR
};

enum zTMdl_AniType {
    zMDL_ANI_TYPE_NORMAL,
    zMDL_ANI_TYPE_BLEND,
    zMDL_ANI_TYPE_SYNC,
    zMDL_ANI_TYPE_ALIAS,
    zMDL_ANI_TYPE_BATCH,
    zMDL_ANI_TYPE_COMB,
    zMDL_ANI_TYPE_DISABLED
};

enum zTMdl_AniDir {
    zMDL_ANIDIR_FORWARD,
    zMDL_ANIDIR_REVERSE,
    zMDL_ANIDIR_ENDFASTEST
};

class zCSoundFX {
public:

    int  Release();
    void TouchTimeStamp();
    void TouchTimeStampLocal();
    void CacheIn(float priority);

    unsigned long cacheState;
};

class zCParticleEmitter;
class zCModelNodeInst;

class zCTexture {
public:

    zSTRING GetObjectName() const;
    void    AddRef();
    int     Release();
    void    TouchTimeStamp();
    void    TouchTimeStampLocal();
    void    CacheIn(float priority);
    static zCTexture* Load(zSTRING& name, int flags = 0);

    int actAniFrame[2];
    unsigned long cacheState;
};

class zTRenderContext;

class zCRenderLightContainer;

enum zTPMLightingMode {
    zTPM_LIGHTING_DEFAULT
};

class zCVisual : public zCObject {
public:

    zCVisual();
    virtual ~zCVisual();

    zCClassDef*      _GetClassDef() const;
    void             SaveBIN(zCFileBIN& file);
    void             LoadBIN(zCFileBIN& file);
    virtual int      Render(zTRenderContext& context);
    void             AddNextLODVisual(zCVisual* visual);
    void             AddEndLODVisual(zCVisual* visual);
    virtual void     GetLODVisualAndAlpha(float distance, zCVisual*& visual, float& alpha);
    virtual int      TraceRay(const zVEC3& origin, const zVEC3& direction, int flags,
        zTTraceRayReport& report);
    virtual zSTRING  GetVisualName() const;
    virtual zCOBBox3D* GetOBBox3D();
    virtual int      GetVisualDied();
    virtual void     DynLightVisual(const zCArray<zCVobLight*>& lights, zMAT4* trafo);
    virtual unsigned long GetRenderSortKey() const;
    virtual int      CanTraceRay() const;
    virtual const zSTRING* GetFileExtension(int index);
    virtual int      GetAlphaTestingEnabled() const;
    virtual void     SetAlphaTestingEnabled(int enabled);
    virtual zCVisual* LoadVisualVirtual(const zSTRING& fileName) const;
    virtual int      IsBBox3DLocal();

    static zCVisual* LoadVisual(const zSTRING& fileName);
    static void      InitVisualSystem();
    void HostVobAddedToWorld(zCVob* vob, zCWorld* world);
    void HostVobRemovedFromWorld(zCVob* vob, zCWorld* world);
    virtual void SetVisualUsedBy(zCVob* vob);

    zCVisual* prevLODVisual;
    zCVisual* nextLODVisual;
    float     lodFarDistance;
    float     lodNearFadeOutDistance;

    static zCClassDef* classDef;
    static void operator delete(void* memory);
    static zCArray<zCVisual*> s_visualClassList;
};

struct zVEC3 {
    zVEC3() {}
    zVEC3(float value);
    zVEC3(float x, float y, float z);
    zVEC3(const zVEC3& other);
    zSTRING GetString() const;
    float& operator[](int index);
    float Length() const;
    float Length2() const;
    float LengthApprox() const;
    int IsEqualEps(const zVEC3& other) const;
    zVEC3& Normalize();
    zVEC3& NormalizeSafe();
    zVEC3& NormalizeApprox();
    zVEC3& operator+=(const zVEC3& value);
    zVEC3& operator-=(const zVEC3& value);
    zVEC3& operator*=(float value);
    zVEC3& operator/=(float value);

    float n[3];
};

struct zMAT3;

struct zMAT4 {
    zMAT4() {}
    zMAT4(const zMAT4& other);
    zMAT4(const zVEC4& first, const zVEC4& second, const zVEC4& third, const zVEC4& fourth);
    zMAT4(float value);
    zMAT4& operator=(const zMAT3& other);
    zMAT4& operator=(const zMAT4& other);
    zMAT4& operator+=(const zMAT4& other);
    zMAT4& operator-=(const zMAT4& other);
    zMAT4& operator*=(float value);
    zMAT4& operator/=(float value);
    zMAT4 Transpose() const;
    zMAT4 TransposeLinTrafo() const;
    zMAT4 InverseLinTrafo() const;
    zMAT4 Inverse() const;
    zVEC3 Rotate(const zVEC3& vector) const;
    zMAT4& Apply(float (*function)(float));
    int IsUpper3x3Orthonormal() const;
    zSTRING GetDescriptionRot() const;
    zSTRING GetDescriptionPos() const;
    void SetByDescriptionRot(zSTRING& description);
    void SetByDescriptionPos(zSTRING& description);
    zVEC3 GetEulerAngles() const;
    void SetByEulerAngles(zVEC3 angles);
    zVEC3 ExtractScaling() const;
    zMAT3 ExtractRotation() const;
    void MakeOrthonormal();
    void PostRotateX(float angle);
    void PostRotateY(float angle);
    void PostRotateZ(float angle);
    void PostScale(const zVEC3& scale);
    void PreScale(const zVEC3& scale);
    zMAT4& SetTranslation(const zVEC3& translation);
    zMAT4& Translate(const zVEC3& translation);
    void SetUpVector(const zVEC3& vector);
    void SetRightVector(const zVEC3& vector);
    zVEC3 GetTranslation() const;
    zVEC3 GetAtVector() const;
    zVEC3 GetUpVector() const;
    zVEC3 GetRightVector() const;
    zVEC4 operator[](int index) const;

    float m[4][4];

    static zMAT4 s_identity;
};

zVEC3 operator*(const zMAT3& matrix, const zVEC3& vector);
zMAT3 operator*(const zMAT4& first, const zMAT3& second);
zMAT3 operator*(const zMAT3& first, const zMAT4& second);
zVEC4 operator*(const zVEC4& vector, float scalar);
zMAT4 operator-(const zMAT4& value);
zMAT4 operator+(const zMAT4& first, const zMAT4& second);
zMAT4 operator-(const zMAT4& first, const zMAT4& second);
zMAT4 operator*(const zMAT4& value, float scalar);
zMAT4 operator*(float scalar, const zMAT4& value);
zMAT4 operator/(const zMAT4& value, float scalar);
int operator==(const zMAT4& first, const zMAT4& second);
int operator!=(const zMAT4& first, const zMAT4& second);
void Alg_Swap(zMAT4& first, zMAT4& second);

std::ostream& operator<<(std::ostream& output, zMAT3& value);
std::istream& operator>>(std::istream& input, zMAT3& value);
std::ostream& operator<<(std::ostream& output, zMAT4& value);
std::istream& operator>>(std::istream& input, zMAT4& value);

zVEC3 operator*(const zMAT4& matrix, const zVEC3& vector);
zVEC3 operator*(const zVEC3& vector, const zMAT4& matrix);
zMAT4 operator*(const zMAT4& first, const zMAT4& second);
zVEC3 operator*(const zVEC3& vector, float value);
zVEC3 operator+(const zVEC3& first, const zVEC3& second);
zVEC3 operator-(const zVEC3& first, const zVEC3& second);
zVEC3 operator-(const zVEC3& vector);
zVEC3 operator/(const zVEC3& vector, float value);
zMAT4 zInverseMatD(const zMAT4& matrix, const zSTRING& caller);
float operator*(const zVEC3& first, const zVEC3& second);
int operator==(const zVEC3& first, const zVEC3& second);
zVEC3 operator^(const zVEC3& first, const zVEC3& second);
void Alg_CalcAziElev(const zVEC3& vector, float& azimuth, float& elevation);

struct zCQuat {
    static zCQuat EulerToQuat(const zVEC3& euler);
    void Matrix4ToQuat(const zMAT4& matrix);
    void Unit();
    void QuatToMatrix4(zMAT4& matrix) const;
    void Slerp(float time, const zCQuat& other);
    void Squad(float weight, const zCQuat& first, const zCQuat& second,
        const zCQuat& third, const zCQuat& fourth);

    float q[4];
};

struct zTBSphere3D;

struct zTBBox3D {
    zTBBox3D() {}
    zTBBox3D(const zTBBox3D& other) : mins(other.mins), maxs(other.maxs) {}

    int IsIntersecting(const zTBSphere3D& sphere) const;
    int IsIntersecting(const zTBBox3D& other) const;
    int IsIntersecting(const zVEC3& start, const zVEC3& direction,
        float& nearTime, float& farTime) const;
    int IsIntersectingSweep(const zVEC3& direction, const zTBBox3D& other, float& time) const;
    int IsIntersectingSweep(const zTBBox3D& endBox, const zTBBox3D& otherStart,
        const zTBBox3D& otherEnd, float& time) const;
    int IsTrivIn(const zTBBox3D& other) const;
    int IsTrivInLine(const zVEC3& start, const zVEC3& end) const;
    int Classify(const zTBBox3D& other) const;
    void Init();
    void InitZero();
    void SetMaximumBox();
    void AddPoint(const zVEC3& point);
    void CalcGreaterBBox3D(const zTBBox3D& other);
    void ClipToBBox3D(const zTBBox3D& other);
    void Scale(float value);
    void Scale(const zVEC3& value);
    float GetVolume() const;
    float GetMinExtent() const;
    float GetScreenSize() const;
    int TraceRay(const zVEC3& start, const zVEC3& direction, zVEC3& intersection) const;
    zTBSphere3D GetSphere3D() const;
    void GetCornerPoints(zVEC3* points) const;
    int ClassifyToPlane(const zTPlane& plane) const;
    int ClassifyToPlane(const zTPlane& plane, int signBits) const;
    int ClassifyToPlaneSides(const zTPlane& plane) const;
    int ClassifyToPlaneSides(const zTPlane& plane, int signBits) const;
    zSTRING GetDescription() const;
    void SetByDescription(zSTRING& description);
    void Transform(const zMAT4& trafo, zTBBox3D& result) const;
    void UnTransform(const zMAT4& trafo, zTBBox3D& result) const;
    zVEC3 GetNearestPoint(const zTPlane* plane) const;
    zVEC3 GetCenter() const;
    zVEC3 GetCenterFloor() const;
    void SaveBIN(zCFileBIN& file);
    void LoadBIN(zCFileBIN& file);
    void Draw(const zCOLOR& color);

    zVEC3 mins;
    zVEC3 maxs;
};

struct zCRenderLight {
    zCRenderLight();

    int   lightType;
    zVEC3 colorDiffuse;
    zVEC3 position;
    zVEC3 direction;
    float range;
    float rangeInv;
    zVEC3 positionLS;
    zVEC3 directionLS;
    float dir_approxFalloff;
};

struct zTLightScore {
    int   index;
    float score;
};

class zCRenderLightContainer {
public:

    ~zCRenderLightContainer();

    void        Clear();
    void        CreatePrelightConfig(const zTBBox3D& bbox);
    int         CollectLights_FastPath(const zTRenderContext& context);
    void        CollectLights_DynLights(const zTRenderContext& context);
    void        CollectLights_StatLights(const zTRenderContext& context);
    void        CollectLights(const zTRenderContext& context);
    zVEC3       CalcStatLightDir(const zTRenderContext& context);
    zVEC3       LightVertex(const zVEC3& position, const zVEC3& normal) const;
    void        Activate(const zMAT4& trafo);
    void        SetupRendererLights();
    static int  CompareLightScore(const void* ele1, const void* ele2);
    static void S_ReleaseVobLightRefs();

    zCRenderLight lightList[8];
    int           numLights;
    int           doPrelight;
    int           m_bSmoothPrelit;
    float         m_fPreLightDist;
    zMAT4         trafoObjToCam;
};

class zCVob;
class zCVobLight;

class zCDynVobLightCacheElement {
public:

    void        ReleaseLights();
    void        UpdateContents(const zCVob* vob);
    static void operator delete(void* memory);

    zCVobLight* lightList[4];

    static zCMemPool<zCDynVobLightCacheElement> s_MemPool;
};

class zCModelNode {
public:

    zCModelNode();
    zCModelNode(const zCModelNode& other);
    ~zCModelNode();

    void SetNodeVisualS(zCVisual* visual);

    zCModelNode*     parentNode;
    zSTRING          nodeName;
    zCVisual*        visual;
    zMAT4            trafo;
    zVEC3            nodeRotAxis;
    float            nodeRotAngle;
    zVEC3            translation;
    zMAT4            trafoObjToWorld;
    zMAT4*           nodeTrafoList;
    zCModelNodeInst* lastInstNode;
};

template <class T>
class zCList {
public:

    ~zCList()
    {
        delete next;
        next = 0;
        data = 0;
    }

    void Insert(T* value)
    {
        zCList* entry = new zCList;
        entry->data = value;
        entry->next = next;
        next = entry;
    }

    void DeleteList()
    {
        delete next;
        next = 0;
        data = 0;
    }

    void Remove(T* value)
    {
        zCList* previous = this;
        while (previous->next)
        {
            zCList* entry = previous->next;
            if (entry->data == value)
            {
                previous->next = entry->next;
                entry->next = 0;
                delete entry;
                return;
            }
            previous = entry;
        }
    }

    T* Get(int index) const
    {
        const zCList* entry = next;
        int current = 0;
        while (entry) {
            if (current == index)
                return entry->data;
            entry = entry->next;
            ++current;
        }
        return 0;
    }

    int GetNumInList() const
    {
        int count = 0;
        const zCList* entry = next;
        while (entry) {
            ++count;
            entry = entry->next;
        }
        return count;
    }

    void DeleteListDatas()
    {
        delete data;
        data = 0;
        if (next) {
            next->DeleteListDatas();
            delete next;
            next = 0;
        }
    }

    T*      data;
    zCList* next;
};

struct zTMdl_AniSample {
    static void CalcSamplePosConstants(float minimum, float range);
    static void SetSamplePosConstants(float minimum, float scaler);
    void Pack(const zCQuat& quat, const zVEC3& positionValue);
    void Unpack(zCQuat& quat, zVEC3& positionValue) const;
    void PackTrans(const zVEC3& positionValue);
    void UnpackTrans(zVEC3& positionValue) const;
    void PackQuat(const zCQuat& quat);
    void UnpackQuat(zCQuat& quat) const;

    unsigned short rotation[3];
    unsigned short position[3];

    static float samplePosScaler;
    static float samplePosRangeMin;
};

class zCModelAni {
public:

    zCModelAni();
    ~zCModelAni();

    static zCObject*  _CreateNewInstance();
    static void       operator delete(void* memory);
    zCClassDef*       _GetClassDef() const;
    int                 Release();
    void                PrecacheAniEventData();
    float               GetAniVelocity() const;
    zVEC3               GetAniTranslation() const;
    zCQuat              GetQuat(int frame, int node) const;
    void                AddTrafoMatrix(zMAT4** trafos);
    zVEC3               GetTrans(int frame, int node) const;
    void                SetTrans(int frame, int node, const zVEC3& value);
    void                SetQuat(int frame, int node, const zCQuat& value);
    void                AddNodeList(zCTree<zCModelNode>** nodes);
    void                CorrectRootNodeIdleMovement();
    void                CalcInPlaceFlag();
    int                 ResolveAlias(zCModelPrototype* model);
    int                 ResolveComb(zCModelPrototype* model);
    void                SaveMAN(zCModelPrototype* model, const zSTRING& fileName);
    int                 LoadMAN(const zSTRING& fileName, zCModelPrototype* model,
        const zSTRING& sourceFile);
    void                SetFlags(const zSTRING& flags);
    const zSTRING&      GetAniName() const;
    zTMdl_AniType       GetAniType() const;
    int                 GetAniID() const;
    void                SetBlendingSec(float blendIn, float blendOut);
    void                GetBlendingSec(float& blendIn, float& blendOut) const;

    zSTRING             aniName;
    zSTRING             ascName;
    int                 aniID;
    zSTRING             aliasName;
    zCList<zCModelAni>  combAniList;
    int                 layer;
    float               blendInSpeed;
    float               blendOutSpeed;
    zTBBox3D            aniBBox3DObjSpace;
    float               collisionVolumeScale;
    zCModelAni*         nextAni;
    zSTRING             nextAniName;
    zCModelAniEvent*    aniEvents;
    float               fpsRate;
    float               fpsRateSource;
    int                 rootNodeIndex;
    zCArray<int>        nodeIndexList;
    zCModelNode**       nodeList;
    zTMdl_AniSample*    aniSampleMatrix;
    float               samplePosRangeMin;
    float               samplePosScaler;
    int                 numFrames;
    int                 numNodes;
    zTMdl_AniType       aniType;
    zTMdl_AniDir        aniDir;
    int                 numAniEvents;
    unsigned char       aniFlags;

    static zCClassDef* classDef;
};

struct zTWeightEntry {
    zTWeightEntry();

    float         weight;
    zVEC3         vertPosOS;
    unsigned char nodeIndex;
    zVEC3         position;
    zVEC3         normal;
};

struct zTNodeWedgeNormal {
    zVEC3 normal;
    int   nodeIndex;
};

class zCMeshSoftSkin : public zCProgMeshProto {
public:

    zCMeshSoftSkin();
    virtual ~zCMeshSoftSkin();

    static void operator delete(void* memory);
    void       AllocVertWeightStream(int numVertices, int numWeights);
    void       DeleteVertWeightStream();
    void       AddNumWeightEntry(const int& count);
    void       AddWeightEntry(const zTWeightEntry& entry);
    void       ResetVertWeightIterator();
    int        GetVertWeightStreamSize();
    void       BuildNodeIndexList();
    void       DeleteNodeOBBList();
    void       BuildNodeOBBList();
    void       AddRef();
    int        Release();
    void       SaveBIN(zCFileBIN& file);
    void       LoadBIN(zCFileBIN& file);
    void       SetupLowestLOD(zTRenderContext& context, int* numPoly);
    void       SetupLowestLOD(zTRenderContext& context, const zCArray<zMAT4*>& trafos,
        float fatness, int& numPoly);
    void       RenderSoftSkin(zTRenderContext& context, float fatness);
    int        RenderSoftSkin(zTRenderContext& context, const zCArray<zMAT4*>& trafos,
        zCRenderLightContainer* lights, float fatness);
    int        GetLowestLODNumPolys();
    zVEC3*     GetLowestLODPoly(int index, zVEC3*& normal);
    virtual int Save(zCFileBIN& file);
    virtual int Load(zCFileBIN& file);
    static unsigned short GetFileVersion();
    static zCObject* _CreateNewInstance();
    zCClassDef* _GetClassDef() const;

    zCArray<zTWeightEntry>      weightList;
    zCArray<int>                nodeIndexList;
    zCArray<zCOBBox3D*>         nodeOBBList;
    zCArray<zTNodeWedgeNormal>  nodeWedgeNormalList;
    unsigned char*              vertWeightStream;
    unsigned char*              iterator;

    static zCClassDef* classDef;
    static int         s_lowestLODNumPolys;
};

template <class T>
class zCTree {
public:

    zCTree()
        : data(0), parent(0), firstChild(0), lastChild(0), prev(0), next(0)
    {
    }

    ~zCTree()
    {
        while (firstChild)
            delete firstChild;
        RemoveChild();
    }

    void DeleteDataSubtree()
    {
        delete data;
        data = 0;
        for (zCTree* child = firstChild; child; child = child->next)
            child->DeleteDataSubtree();
    }

    zCTree* AddChild(T* value)
    {
        zCTree* child = new zCTree;
        child->parent = this;
        child->data = value;
        child->next = firstChild;
        if (firstChild)
            firstChild->prev = child;
        firstChild = child;
        return child;
    }

    int CountNodes()
    {
        int count = 1;
        if (firstChild)
            count += firstChild->CountNodes();
        if (next)
            count += next->CountNodes();
        return count;
    }

    void RemoveChild()
    {
        if (prev)
            prev->next = next;
        if (next)
            next->prev = prev;
        if (parent && parent->firstChild == this)
            parent->firstChild = next;
    }

    static void operator delete(void* memory)
    {
        ::operator delete(memory);
    }

    T*       data;
    zCTree*  parent;
    zCTree*  firstChild;
    zCTree*  lastChild;
    zCTree*  prev;
    zCTree*  next;
};

class zCModelAniEvent {
public:

    zCModelAniEvent();
    ~zCModelAniEvent();

    void Save(zCFileBIN& file) const;
    void Load(zCFileBIN& file);

    zTMdl_AniEventType          aniEventType;
    int                         frameNr;
    zSTRING                     tagString;
    zSTRING                     string[zMDL_ANIEVENT_MAXSTRING];
    float                       value1;
    float                       value2;
    float                       value3;
    float                       value4;
    zCArray<zCSoundFX*>         soundList;
    zCArray<zCParticleEmitter*> pfxEmitterList;
};

struct zTAniAttachment;

class zCModelAniActive {
public:

    zCModelAniActive();
    ~zCModelAniActive();

    void  SetDirection(zTMdl_AniDir direction);
    float GetProgressPercent() const;
    void  SetProgressPercent(float progress);
    void  SetActFrame(float frame);
    void  DoCombineAni(zCModel* model, int frame, int nextFrame);

    zCModelAni*      protoAni;
    zCModelAni*      nextAni;
    int              advanceDir;
    float            actFrame;
    int              actAniEvent;
    float            combAniX;
    float            combAniY;
    int              isFadingOut;
    int              isFirstTime;
    zCModelAni*      nextAniOverride;
    float            blendInOverride;
    float            blendOutOverride;
    zVEC3            lastPos;
    zVEC3            thisPos;
    zCQuat           lastRotQuat;
    zCQuat           thisRotQuat;
    zCQuat           freezeRotQuat;
    int              rotFirstTime;
    float            transWeight;
    zTAniAttachment* aniAttachment;
    float            randAniTimer;
};

struct zTRandAni {
    int randAniProtoID;
    int prob;
};

struct zTAniAttachment {
    zTAniAttachment();
    ~zTAniAttachment();

    int                aniID;
    zCArray<zTRandAni> randAniList;
    float              randAniFreq;
    int                randAniProbSum;
};

struct zVEC2;

class zCMaterial : public zCObject {
public:

    zCMaterial();
    zCMaterial(const zSTRING& name);
    zCMaterial(const zCMaterial& other);
    virtual ~zCMaterial();
    zCMaterial& operator=(const zCMaterial& other);
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    void InitThisByMaterial(const zCMaterial& other);
    void InitValues();
    void SetDetailObjectVisual(const zSTRING& visualName);
    const zSTRING& GetDetailObjectVisual() const;
    void ApplyTexAniMapping(zCPolygon* polygon);
    zVEC3 GetTexAniVector(zCPolygon* polygon);
    int IsPortalMaterial() const;
    void SetTexAniMappingDir(const zVEC2& direction);
    void AutoAssignDetailTexture();
    void SetTexture(const zSTRING& fileName);
    void SetTexture(zCTexture* texture);
    void SetDetailTexture(const zSTRING& fileName);
    void SetDetailTexture(zCTexture* texture);
    zVEC2 GetTexScale();
    void SetTexScale(zVEC2 scale);
    void RemoveTexture();
    const zSTRING& GetMatGroupString() const;
    void SetMatGroupByString(const zSTRING& group);
    void RefreshAvgColorFromTexture();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    static const zSTRING& GetMatGroupString(int group);
    static zCMaterial* SearchName(zSTRING& name);
    void AddRef();
    int  Release();
    const zSTRING& GetName() const;
    zCTexture* GetAniTexture();

    zCArray<zCPolygon*> polyList;
    int         polyListTimeStamp;
    zCTexture*  texture;
    unsigned char color[4];
    float       smoothAngle;
    int         matGroup;
    zCBspSector* bspSectorFront;
    zCBspSector* bspSectorBack;
    zCTexAniCtrl texAniCtrl;
    zSTRING*    detailObjectVisualName;
    float       kambient;
    float       kdiffuse;
    float       m_bEnvironmentalMappingStrength;
    unsigned long flags;
    unsigned long smooth;
    int         matUsage;
    int         libFlag;
    int         rndAlphaBlendFunc;
    int         ignoreSun;
    int         m_enuWaveMode;
    int         m_enuWaveSpeed;
    float       m_fWaveMaxAmplitude;
    float       m_fWaveGridSize;
    zCTexture*  detailTexture;
    float       detailTextureScale;
    float       texAniMapDelta[2];
    float       default_mapping[2];
    float       texScale[2];

    static int s_matUsageDefault;
    static zCClassDef* classDef;
    static void operator delete(void* memory);
};

struct zVEC2 {
    zVEC2() {}
    zVEC2(float x, float y) { n[0] = x; n[1] = y; }
    zVEC2& operator+=(const zVEC2& value);
    zVEC2& operator-=(const zVEC2& value);
    zVEC2& operator*=(float value);
    zVEC2& operator/=(float value)
    {
        float inv = 1.0f / value;
        n[0] *= inv;
        n[1] *= inv;
        return *this;
    }
    float Length2() { return n[0] * n[0] + n[1] * n[1]; }

    float n[2];
};

inline float operator*(const zVEC2& first, const zVEC2& second)
{
    return first.n[0] * second.n[0] + first.n[1] * second.n[1];
}

zVEC2 operator+(const zVEC2& first, const zVEC2& second);

struct zTMatIDList {
    int         numMaterials;
    zCMaterial** materials;
};

class zCPolygon {
public:

    void SetMapping(zVEC2* first, zVEC2* second, zVEC2* third);

    zCMaterial* material;
    int         vertexIndex[3];
    zVEC2*      texVertex[3];
};

class zCMesh : public zCVisual {
public:

    zCMesh();
    virtual ~zCMesh();

    static zCObject*  _CreateNewInstance();
    static zCMesh*    Load(const zSTRING& fileName, int saveGame);
    zCClassDef*       _GetClassDef() const;
    int               Release();
    virtual int       Render(zTRenderContext& context);
    int               Render(zTRenderContext& context, zCOLOR* color);
    int               RenderHorizon(zTRenderContext& context, float horizonZ);
    void              RenderDecal(zTRenderContext& context, int onTop);
    void              RenderDecal(float x, float y, float size, int onTop);
    void              RenderDecalHorizon(float x, float y, float size, float horizonZ);
    virtual zSTRING   GetVisualName();
    virtual void      DynLightVisual(const zCArray<zCVobLight*>& lights, zMAT4* trafo);
    virtual zTBBox3D  GetBBox3D();
    virtual zCOBBox3D* GetOBBox3D();
    int               CanTraceRay() const;
    virtual int       TraceRay(const zVEC3& origin, const zVEC3& direction, int flags,
        zTTraceRayReport& report);
    zCPolygon*        CheckRayPolyIntersection(const zVEC3& origin, const zVEC3& direction,
        zVEC3& intersection);
    zCPolygon*        CheckRayPolyIntersection(zCCamera& camera, int x, int y,
        zVEC3& intersection);
    zCPolygon*        PickPolygon(const zVEC3& origin, const zVEC3& direction,
        zVEC3& intersection);
    zCPolygon*        PickPolygon(zCCamera& camera, int x, int y, zVEC3& intersection);
    int               GetAlphaTestingEnabled() const;
    void              SetAlphaTestingEnabled(int enabled);
    const zSTRING*    GetFileExtension(int index);
    zCVisual*         LoadVisualVirtual(const zSTRING& fileName) const;
    void              DeleteAll();
    void              AllocVerts(int count);
    zCVertex*         AddVertex(zCVertex* vertex);
    zCVertex*         AddVertex(zVEC3& position);
    void              AddVertex(const zVEC3& value);
    zCVertex*         AddVertexSmart(zVEC3& position);
    zCVertex*         VertexInMesh(const zVEC3& position);
    void              AllocPolys(int count);
    zCPolygon*        AddPoly(zVEC3& first, zVEC3& second, zVEC3& third,
        zVEC2& firstMapping, zVEC2& secondMapping, zVEC2& thirdMapping);
    zCPolygon*        AddPoly(zCVertex* first, zCVertex* second, zCVertex* third);
    zCPolygon*        AddPoly(zCVertex* first, zCVertex* second, zCVertex* third,
        zCVertex* fourth);
    zCPolygon*        AddPoly(zCPolygon* polygon);
    zCPolygon*        AddPoly();
    void              AddPoly(const zCPolygon& value);
    void              DeletePoly(zCPolygon* polygon);
    void              DeletePolyAndVerts(zCPolygon* polygon);
    void              DeletePolyAndVerts(unsigned long index);
    void              RemoveDegeneratePolys();
    zCPolygon*        SharePoly(int index) const;
    void              ShareFeatures();
    void              UnshareFeatures();
    void              FlipPolys();
    void              SortPolysByMaterial();
    void              SortPolysByList(zCPolygon** polygons, int count);
    static void       ConvertTrisToNPolys(zCPolygon** polygons, int count, zCMesh* mesh,
        int preserveTriangles);
    static void       SaveMSH(zCFileBIN& file, zCPolygon** polygons, int count, zCMesh* mesh);
    void              MergeMesh(zCMesh* source, const zMAT4& trafo);
    void              SaveMSH(zCFileBIN& file);
    void              SaveMSH(const zSTRING& fileName);
    int               LoadMSH(zCFileBIN& file, int saveGame);
    int               LoadMSH(const zSTRING& fileName, int saveGame);
    void              SetMaterial(zCMaterial* material);
    void              ScaleXZ(float scaleX, float scaleZ);
    void              Scale(float scaleX, float scaleY, float scaleZ);
    void              UnSmoothMesh();
    void              SetDefaultPlanarMapping(float scaleU, float scaleV);
    void              Center();
    void              TransformAllVerts(const zMAT4& trafo);
    void              TransformAllVerts(const zMAT4& trafo, int calcNormals);
    void              CalcNormals();
    enum zTCalcVertNormalMode {
        zMSH_VERTNORMAL_MAT,
        zMSH_VERTNORMAL_SMOOTH,
        zMSH_VERTNORMAL_FACET
    };
    void              CalcVertexNormals();
    void              CalcVertexNormals(zTCalcVertNormalMode mode, zCBspTree* bspTree);
    void              CalcBBox3D();
    void              CalcBBox3D(int updateOBBox);
    void              CheckForMaterials() const;
    void              CreateListsFromArrays();
    void              ResetStaticLight();
    void              DynLightScalePrelit(zCOLOR color);
    void              PrelightMesh(zMAT4* trafo);
    void              LightMesh(zCVobLight& light, zMAT4& trafo, zCWorld* world);
    void              CombineLightmaps();

    int              numPoly;
    int              numVert;
    int              numFeat;
    zCVertex**       vertList;
    zCPolygon**      polyList;
    zCVertFeature**  featList;
    zCVertex*        vertArray;
    zCPolygon*       polyArray;
    zCVertFeature*   featArray;
    zTBBox3D         bbox3D;
    zCOBBox3D        obbox3D;
    int              masterFrameCtr;
    zCMesh*          next;
    zCMesh*          prev;
    zSTRING          meshName;
    int              hasLightmaps;
    int              m_bUsesAlphaTesting;
    int              numVertAlloc;
    int              numPolyAlloc;

    int              numVerts;
    zVEC3*           vertexList;

    static zCMesh* SearchName(const zSTRING& name);
    static zCMesh* CreateQuadMesh(int twoSided = 0);
    static zCMesh* CreateTriMesh();
    static int CheckTexUVOnPlane(const zCArray<zCVertex*>& vertices,
        const zCArray<zCVertFeature>& features);
    static unsigned short GetFileVersion();
    static void operator delete(void* memory);
    static void S_InitVertexMergeCache(zCMesh* mesh);
    static void S_DeleteVertexMergeCache();

    void SetStaticLight(zCOLOR color);
    void SetLightDyn(zCOLOR color);
    void SetLightStatAlpha(unsigned char alpha);
    void ResetLightDynToLightStat();
    void AddToLightDyn(zCOLOR color);

    static int                      s_numMeshes;
    static int                      s_usesVertexCache;
    static zCMesh*                  s_meshRoot;
    static zCClassDef*              classDef;
    static zCArraySort<zCVertex*>   s_vertexCache;
};

class zCLightMap;

class zCCombine {
public:

    struct zCCItem {
        zCLightMap* lightmap;
        int         x;
        int         y;
    };

    zCCombine();
    ~zCCombine();

    int CombineWith(zCLightMap* lightmap);

    unsigned char      occupied[258][258];
    zCArray<zCCItem*> items;

private:

    int CombineWith(zCLightMap* lightmap, int x, int y);
};

struct zTPMTriangle;
struct zTPMWedge;
struct zTPMTriangleEdges;
struct zTPMEdge;
struct zTPMVertexUpdate;

struct zCSubMesh {
    zCSubMesh();
    ~zCSubMesh();

    zCMaterial* material;
    zCArrayAdapt<zTPMTriangle> triList;
    zCArrayAdapt<zTPMWedge> wedgeList;
    zCArrayAdapt<float> colorList;
    zCArrayAdapt<unsigned short> triPlaneIndexList;
    zCArrayAdapt<zTPlane> triPlaneList;
    zCArrayAdapt<zTPMTriangleEdges> triEdgeList;
    zCArrayAdapt<zTPMEdge> edgeList;
    zCArrayAdapt<float> edgeScoreList;
    zCArrayAdapt<unsigned short> wedgeMap;
    zCArrayAdapt<zTPMVertexUpdate> vertexUpdates;
    int vbStartIndex;
};

class zCVertexBuffer {
public:

    void SetXYZ(unsigned long index, const zVEC3& position);

    unsigned long vertexBufferID;
    int           arrayStride;
    zVEC3*        positionList;
    zVEC3*        normalList;
    zVEC2*        texCoordList;
};

class zCVertexBufferDyn {
public:

    zCVertexBufferDyn(int format = 0, int numVertices = 0);
};

struct zTLODRenderArgs {
    float vertPerc;
    int   numPos;
    int   numMinMorphPos;
    float morphFrac;
};

struct zTLODParams {
    float strength;
    float zDisplace2;
    float morphPerc;
    int   minVerts;
};

struct zTLODRenderArgsSubMesh {
    int numTri;
    int numWedge;
    int numMinMorphWedge;
    int numSubdivEdges;
};

class zTRenderContext {
public:

    float distVobToCam;
    int   m_AniMode;
    float m_aniModeStrength;
    zCCamera* cam;
    zCWorld* world;
    zCVob* vob;
    int clipFlags;
    int hintLightingFullbright;
    zCOLOR hintLightingHighlightColor;
};

class zCProgMeshProto {
public:

    zCProgMeshProto();
    virtual ~zCProgMeshProto();

    void               CalcRenderSortKey();
    void               PackStaticVertexBuffers();
    int                CheckRayPolyIntersection(zCSubMesh* subMesh, int triIndex,
        const zVEC3& origin, const zVEC3& direction, zVEC3& intersection, float& distance) const;
    int                CheckRayPolyIntersection2Sided(zCSubMesh* subMesh, int triIndex,
        const zVEC3& origin, const zVEC3& direction, zVEC3& intersection, float& distance) const;
    virtual int        TraceRay(const zVEC3& origin, const zVEC3& direction, int flags,
        zTTraceRayReport& report);
    int                GetNumTri() const;
    zCPolygon*         GetPolygon(int index);
    zCMesh*            GetMesh(int flags);
    int                GetLowestLODNumPolys();
    zVEC3*             GetLowestLODPoly(int index, zVEC3*& normal);
    static void        InitProgMeshProto();
    static int         CreateDynamicVertexBuffer();
    static void        CleanupProgMeshProto();
    float              CalcLODRenderVertPerc(const zTRenderContext& context);
    int                CalcLODRenderArgs(const zTRenderContext& context, zTLODRenderArgs& args);
    int                CalcLODRenderArgsMin(const zTRenderContext& context, zTLODRenderArgs& args);
    int                CalcLODRenderArgsSubMesh(const zTRenderContext& context,
        const zTLODRenderArgs& args, const zCSubMesh* subMesh, zTLODRenderArgsSubMesh& subArgs);
    virtual int        Render(zTRenderContext& context);
    int                RenderProgMesh(zTRenderContext& context, int flags,
        zTLODRenderArgs* args, zCRenderLightContainer* lights, float aniStrength, float lodOverride);
    int                RenderStatic(zTRenderContext& context, int flags,
        zTLODRenderArgs* args, zCRenderLightContainer* lights);
    int                RenderStaticLOD(zTRenderContext& context, int flags,
        zTLODRenderArgs* args, zCRenderLightContainer* lights);
    int                RenderStaticLODShadow(zTRenderContext& context, int flags,
        zTLODRenderArgs* args, zCRenderLightContainer* lights);
    void               SoftwareTransformVerts(int flags, zTLODRenderArgs* args,
        float aniStrength, float lodOverride);
    void               CalcSubdiv(int flags, const zTLODRenderArgs* args);
    void               CalcNumSubdivEdges(zCSubMesh* subMesh, const zTLODRenderArgs* args,
        zTLODRenderArgsSubMesh& subArgs);
    void               CalcSubdivSubMesh(zCSubMesh* subMesh, zCVertexBuffer* vertexBuffer,
        const zTLODRenderArgs* args, zCRenderLightContainer* lights, zTLODRenderArgsSubMesh& subArgs);
    int                RenderDynamicLOD(zTRenderContext& context, int flags,
        zTLODRenderArgs* args, zCRenderLightContainer* lights, float aniStrength, float lodOverride);
    int                RenderDynamicSubdiv(zTRenderContext& context, int flags,
        zTLODRenderArgs* args, zCRenderLightContainer* lights, float aniStrength, float lodOverride);
    zSTRING            GetObjectName() const;
    static zCObject*   _CreateNewInstance();
    static void        SetLODParamStrengthModifier(float value);
    static float       GetLODParamStrengthModifier();
    static zCVisual*   Load(const zSTRING& fileName);
    static int         BuildProgMeshProto(zCMesh* mesh, zCProgMeshProto* proto,
        zCArray<int>* wedgeMap, int flags);
    static unsigned short GetFileVersion();
    virtual int        Save(zCFileBIN& file);
    virtual int        Load(zCFileBIN& file);
    static int         GetNumTri();
    static void        InitSetupLowestLOD();
    static void        operator delete(void* memory);

    virtual zCClassDef* _GetClassDef() const;
    virtual int         IsBBox3DLocal();
    virtual zTBBox3D    GetBBox3D();
    virtual zCOBBox3D*  GetOBBox3D();
    virtual zSTRING     GetVisualName();
    virtual const zSTRING* GetFileExtension(int index);
    virtual zCVisual*   LoadVisualVirtual(const zSTRING& fileName) const;
    virtual void        DynLightVisual(const zCArray<zCVobLight*>& lights, zMAT4* trafo);
    virtual unsigned long GetRenderSortKey() const;
    virtual int         CanTraceRay() const;
    virtual int         GetAlphaTestingEnabled() const;
    virtual void        SetAlphaTestingEnabled(int enabled);

    static zCClassDef*       classDef;
    static unsigned long     s_classCtorCtr;
    static float             s_lodParamStrengthModifier;
    static float             s_lodParamStrengthOverride;
    static void              SetLODParamStrength(float strength);
    static int               s_frameCtr;
    static int               s_autoSubdivEnabled;
    static int               s_subMeshRenderDir;
    static unsigned short*   s_wedgeRemap;
    static zCArray<unsigned short> s_workIndexList;
    static zVEC3*            s_posCacheList;
    static zVEC3*            s_posNormalCSList;
    static zVEC3*            s_posCacheListLOD;
    static int*              s_posCacheListLODIndices;
    static int               s_posCacheCount;
    static int               s_posCacheCountLOD;
    static int               s_posCacheIndex;
    static zCVertexBufferDyn* s_vertBufferNormal;
    static zCVertexBufferDyn* s_vertBufferColor;
    static int               s_markMeshMaterials;

    int              numSubMeshes;
    zCSubMesh*       subMeshList;
    zCArray<zVEC3>   posList;
    zCArray<zVEC3>   posNormalList;
    zTBBox3D         bbox3D;
    zCOBBox3D        obbox3D;
    zCVertexBuffer*  vertexBufferStatic;
    float            avgTriArea;
    unsigned char*   dataPool;
    unsigned long    dataSize;
    int              m_bUsesAlphaTesting;
    zTLODParams      lodParams;
    unsigned long    pmeshID;
    unsigned long    renderSortKey;
    int              lightingMode;
    int              refCtr;
};

struct zTNodeMesh {
    int       nodeIndex;
    zCVisual* visual;
};

class zCModelMeshLib {
public:

    zCModelMeshLib();
    ~zCModelMeshLib();

    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    zCClassDef* _GetClassDef() const;
    zSTRING GetObjectName() const;
    void    ReleaseData();
    void    ApplyToModel(zCModel* model);
    void    ApplyToModel(zCModelPrototype* model);
    void    RemoveFromModel(zCModel* model);
    void    BuildFromModel(zCModel* model);
    void    BuildFromModel(zCModelPrototype* model);
    void    AllocNumNodeVisuals(int count);
    void    AddMeshSoftSkin(zCMeshSoftSkin* skin);
    void    AddNodeVisual(int nodeIndex, zCVisual* visual);
    void    SaveMDM(zCModelPrototype* model);
    void    SaveMDM(zCFileBIN& file, zCModelPrototype* model);
    int     Release();
    static unsigned long GetMDMFileVersion();
    static int LoadMDM(const zSTRING& fileName, zCModelPrototype* proto, zCModel* model,
        zCModelMeshLib** meshLib);
    static int LoadMDM_Try(const zSTRING& fileName, zCModelPrototype* proto, zCModel* model,
        zCModelMeshLib** meshLib);
    static int LoadMDM(zCFileBIN& file, zCModelPrototype* proto, zCModel* model,
        zCModelMeshLib** meshLib);
    static int ConvertMDM(const zSTRING& fileName, zCModelPrototype* model);

    int refCtr;
    zSTRING objectName;
    zCArray<zTNodeMesh> meshNodeList;
    zCArray<zCMeshSoftSkin*> meshSoftSkinList;

    static zCClassDef* classDef;
};

class zCModelTexAniState {
public:

    enum {
        zMDL_MAX_ANI_CHANNELS = 2,
        zMDL_MAX_TEX = 4
    };

    zCModelTexAniState();
    ~zCModelTexAniState();

    void DeleteTexList();
    void UpdateTexList();
    void SetChannelVariation(int channel, int frame, zSTRING* filter);
    void BuildTexListFromMeshLib(zCModelMeshLib* meshLib);
    void BuildTexListFromMesh(zCMesh* mesh);
    void BuildTexListFromProgMesh(zCProgMeshProto* mesh);
    void AddTexListFromMeshLib(zCModelMeshLib* meshLib, zCArray<zCTexture*>& textures);
    void AddTexListFromMesh(zCMesh* mesh, zCArray<zCTexture*>& textures);
    void AddTexListFromProgMesh(zCProgMeshProto* mesh, zCArray<zCTexture*>& textures);
    void FinishTexList(zCArray<zCTexture*>& textures);

    int        numNodeTex;
    zCTexture** nodeTexList;
    int        actAniFrames[zMDL_MAX_ANI_CHANNELS][zMDL_MAX_TEX];
};

class zCModelNodeInst {
public:

    enum {
        zMDL_MAX_ANIS_PARALLEL = 6,
        zMDL_BLEND_STATE_FADEIN,
        zMDL_BLEND_STATE_CONST,
        zMDL_BLEND_STATE_FADEOUT
    };

    struct zTNodeAni {
        zCModelAniActive* modelAni;
        float            weight;
        float            weightSpeed;
        int              blendState;
        zCQuat           quat;
    };

    zCModelNodeInst();
    zCModelNodeInst(zCModelNode* node);
    ~zCModelNodeInst();

    void Init();
    void InitByModelProtoNode(zCModelNode* node);
    void SetNodeVisualS(zCVisual* visual, int buildTexList);
    void AddNodeAni(zCModelAniActive* ani);
    void RemoveAllNodeAnis();
    void RemoveNodeAni(zCModelAniActive* ani);
    void FindMasterAni();
    void FadeOutNodeAni(zCModelAniActive* ani);
    int  GetNodeAniListIndex(const zCModelAniActive* ani) const;
    void CalcWeights(zCModel* model);
    void CalcBlending(zCModel* model);

    zCModelNodeInst*  parentNode;
    zCModelNode*      protoNode;
    zCVisual*         nodeVisual;
    zMAT4             trafo;
    zMAT4             trafoObjToCam;
    zTBBox3D          bbox3D;
    zCModelTexAniState texAniState;
    zTNodeAni         nodeAniList[zMDL_MAX_ANIS_PARALLEL];
    int               numNodeAnis;
    int               masterAni;
    float             masterAniSpeed;
};

class zCModelPrototype {
public:

    enum zTFileSourceType {
        zFROM_MDS,
        zFROM_ASC
    };

    zCModelPrototype();
    ~zCModelPrototype();

    void                    Init();
    void                    Clear();
    void                    ReleaseMeshSoftSkinList();
    void                    ReleaseMeshes();
    int                     Release();
    const zSTRING&          GetModelProtoFileName() const;
    void                    SetModelProtoName(const zSTRING& name);
    void                    SetFileSourceType(zTFileSourceType type);
    int                     PrepareAsModelProtoOverlay(zCModelPrototype* base);
    void                    CalcNodeListChecksum();
    void                    CalcAniBBox(zCModelAni* ani);
    void                    ResolveReferences();
    void                    SkipBlockMDS(int depth);
    int                     ReadMeshAndTreeMSB(int& nextStart, zCFileBIN& file);
    void                    ReadMeshAndTree(int& dontUseMesh, zCFileBIN& file);
    void                    RegisterMesh(zCFileBIN& file);
    int                     ReadAniEnumMSB(int chunkID, zCFileBIN& file);
    int                     LoadModelScriptMSB(const zSTRING& fileName);
    int                     ReadModelMSB(zCFileBIN& file);
    void                    ReadModel(zCFileBIN& file);
    void                    ReadSoftSkinVertList();
    void                    ReadGeomobject(zMAT4*& tracks, zCTree<zCModelNode>*& resultTree);
    void                    ReadMaterialList();
    void                    ReadAniEnum(int chunkID, zCFileBIN& file);
    zCMesh*                 ReadMesh(int nodeIndex, zCModelNode* node, int flags);
    void                    ReadMeshAnimation(zCModelNode* node, int frame);
    void                    ReadPosTrack(zMAT4* track);
    void                    ReadRotTrack(zMAT4* track);
    void                    ReadTMAnimation(zCModelNode* node, zMAT4*& tracks);
    void                    LocalizeTrafos(zCTree<zCModelNode>* tree, zCTree<zCModelNode>* parent);
    void                    TransformNodeVisual(zCModelNode* node, const zMAT4& trafo);
    void                    ProcessMeshNode(zCModelNode* node, zCTree<zCModelNode>* tree,
        int frame, int flags, int& processed);
    void                    ReadMapDiffuse(zCMaterial* material);
    zCMaterial*             ReadMaterial();
    zCMaterial*             ReadWireframeColor();
    void                    CollectNodeMeshes(zCArray<zCModelNode*>& nodes);
    zCModelNode*            SearchNode(const zSTRING& name);
    zCTree<zCModelNode>*    FindMeshTreeNode(const zSTRING& name, zCTree<zCModelNode>* tree = 0);
    int                     FindMeshTreeNodeIndex(const zSTRING& name, zCTree<zCModelNode>* tree = 0);
    int                     FindNodeListIndex(const zSTRING& name);
    void                    AddAni(zCModelAni* ani);
    int                     SearchAniIndex(const zSTRING& name) const;
    zCModelAni*             SearchAni(const zSTRING& name) const;
    void                    DescribeTree(zCTree<zCModelNode>* tree = 0, int level = 0);
    void                    ConvertVec3(zVEC3& value);
    void                    ConvertAngle(float& value);
    void                    SkipBlock();
    void                    SkipBlockCmt();
    void                    ReadComment();
    void                    ReadScene(float& frameSpeed);
    zVEC3                   ReadTriple();
    zMAT4                   ReadNodeTM(zCModelNode* node);
    void                    ReadVertexList(zCMesh* mesh, int count);
    zTMatIDList*            ReadFaceList(zCMesh* mesh, int count);
    zVEC2*                  ReadTVertexList(int count);
    int                     LoadMDH(const zSTRING& fileName);
    int                     LoadMDH(zCFileBIN& file);
    void                    SaveMDH();
    void                    SaveMDH(zCFileBIN& file);
    int                     LoadModelScript(const zSTRING& fileName);
    int                     LoadModelASC(const zSTRING& fileName);
    void                    Load3DSMAXAsc(zCModelAni*& ani, const zSTRING& fileName,
        const zSTRING& modelName, int flags, int convert, int trim, float scale);
    static int              NumInList();
    static zCModelPrototype* Load(const zSTRING& fileName, zCModelPrototype* base = 0);
    static zCModelPrototype* SearchName(zSTRING name);
    static unsigned long    GetMDHFileVersion();
    static unsigned long    GetMDSFileVersion();

    zCModelPrototype*        next;
    zCModelPrototype*        prev;
    int                      refCtr;
    zSTRING                  modelProtoName;
    zSTRING                  modelProtoFileName;
    zCTree<zCModelNode>      meshTree;
    zCArraySort<zCModelAni*> protoAnis;
    zCArray<zCModelAniEvent*> modelEvents;
    zCArray<zCModelNode*>    nodeList;
    unsigned long            nodeListChecksum;
    zCArray<zCMeshSoftSkin*> meshSoftSkinList;
    zSTRING                  hierarchySourceASC;
    zTBBox3D                 bbox3D;
    zTBBox3D                 bbox3DCollDet;
    zCModelPrototype*        baseModelProto;
    zVEC3                    rootNodeTrans;
    zTFileSourceType         fileSourceType;
    zCArray<zCMesh*>         sourceMeshSoftSkinList;

    static zCModelPrototype* s_modelRoot;
    static int               s_autoConvertAnis;
    static int               s_autoConvertMeshes;
    static int               s_ignoreAnis;
};

class zCWorld;
class zCAIBase;
class zCEventManager;
class zCEventMessage;
class zCRigidBody;
class zCCollisionObject;
class zCCollisionObjectClass;
class zCBspLeaf;

struct zCTree {
    zCTree* parent;
    zCTree* firstChild;
    zCTree* prev;
    zCTree* next;
    zCVob*  data;
};

enum zTVobType {
    zVOB_TYPE_NORMAL,
    zVOB_TYPE_LIGHT,
    zVOB_TYPE_SOUND,
    zVOB_TYPE_LEVEL_COMPONENT,
    zVOB_TYPE_SPOT,
    zVOB_TYPE_CAMERA,
    zVOB_TYPE_STARTPOINT,
    zVOB_TYPE_WAYPOINT,
    zVOB_TYPE_MARKER,
    zVOB_TYPE_SEPARATOR = 127,
    zVOB_TYPE_MOB,
    zVOB_TYPE_ITEM,
    zVOB_TYPE_NSC
};

class zCVob : public zCObject {
public:

    struct zTCollisionContext {
        ~zTCollisionContext() {}

        zCArray<zCCollisionObject*> otherCollObjectList;
        zCArray<zCVob*> otherCollVobList;
    };

    zCVob();
    virtual ~zCVob();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    virtual const zSTRING* GetTriggerTarget(int index) const;
    void             SetTrafo(const zMAT4& trafo);
    zCVisual*        GetVisual() const;
    zCWorld*         GetHomeWorld() const;
    static zCVob*    GetAutoLinkParent(zCVob* vob);
    static int       CanAutoLinkWith(zCVob* vob, zCVob* other);
    static void      CheckAutoLink(zCVob* vob, zCVob* other);
    static void      CheckAutoUnlink(zCVob* vob);
    void             CalcGroundPoly();
    void             CalcWaterVob();
    void             SetCollTypeDefaultFromVisual();
    void             SetCollDetStat(int enabled);
    void             SetCollDetDyn(int enabled);
    void             BeginMovement();
    void             EndMovement(int flags);
    void             SetInMovement(int movement);
    void             SetInMovementMode(int movementMode);
    void             ResetToOldMovementState();
    void             UpdateTouchList(const zCArray<zCVob*>& newTouchList);
    void             CalcDestinationBBox3DWorld(zTBBox3D& destination);
    void             AdoptCollObjResults();
    void             SetCollisionObject(zCCollisionObject* object);
    void             SetCollisionClass(zCCollisionObjectClass* objectClass);
    zCCollisionObject* GetCollisionObject() const;
    void             CreateCollisionObject();
    void             ResetCollisionObjectState();
    void             DestroyCollisionObject(int force);
    void             CheckEmergencyPutToSleep();
    void             CollectCollisionContext_Vobs(const zTBBox3D& box, zTCollisionContext& context);
    void             CollectCollisionContext_Static(const zTBBox3D& box,
        zCArray<zCCollisionObject*>& objects);
    void             CollectCollisionContext(zTCollisionContext& context);
    void             CleanupCollisionContext(const zTCollisionContext& context);
    void             NotifyCollisionPair(zCCollisionReport* report,
        const zCArray<zCCollisionReport*>& reports, int flags);
    void             ExecuteCollisionResponse(const zCArray<zCCollisionReport*>& reports, int& flags);
    void             CheckAndResolveCollisions();
    void             TouchMovement();
    const zMAT4&     GetNewTrafoObjToWorld() const;
    zMAT4&           GetNewTrafoObjToWorld();
    void             SetNewTrafoObjToWorld(const zMAT4& trafo);
    void             SetBBox3DWorld(const zTBBox3D& box);
    void             SetBBox3DLocal(const zTBBox3D& box);
    zTBBox3D         GetBBox3DLocal() const;
    void             Move(float x, float y, float z);
    void             MoveWorld(float x, float y, float z);
    void             MoveLocal(float x, float y, float z);
    void             SetRotationWorld(const zCQuat& rotation);
    void             RotateWorld(const zVEC3& axis, float angle);
    void             RotateLocal(const zVEC3& axis, float angle);
    void             RotateLocalX(float angle);
    void             RotateLocalY(float angle);
    void             RotateLocalZ(float angle);
    void             RotateWorldX(float angle);
    void             RotateWorldY(float angle);
    void             RotateWorldZ(float angle);
    void             GetPositionWorld(float& x, float& y, float& z) const;
    void             GetPositionLocal(float& x, float& y, float& z) const;
    void             CreateTrafoLocal() const;
    float            GetDistanceToVob(zCVob& other) const;
    float            GetDistanceToVobApprox(zCVob& other) const;
    float            GetDistanceToVob2(zCVob& other) const;
    void             SetPositionLocal(const zVEC3& position);
    void             SetTrafoObjToWorld(const zMAT4& trafo);
    void             ResetRotationsLocal();
    void             ResetXZRotationsLocal();
    void             ResetRotationsWorld();
    void             ResetXZRotationsWorld();
    void             SetHeadingYLocal(const zVEC3& target);
    void             SetHeadingYWorld(const zVEC3& target);
    void             SetHeadingYWorld(zCVob* target);
    void             SetHeadingLocal(const zVEC3& target);
    void             SetHeadingWorld(const zVEC3& target);
    void             SetHeadingWorld(zCVob* target);
    void             SetHeadingAtLocal(const zVEC3& direction);
    void             SetHeadingAtWorld(const zVEC3& direction);
    int              HasParentVob() const;
    void             CorrectTrafo();
    void             DoneWithTrafoLocal();
    void             SetPhysicsEnabled(int enabled);
    void             UpdatePhysics();
    int              DetectCollision(zMAT4* trafo);
    int              IsColliding();
    virtual int      GetCharacterClass();
    virtual int      CanThisCollideWith(zCVob* other);
    virtual int      GetScriptInstance(zSTRING*& name, int& instance);
    virtual int      SetByScriptInstance(const zSTRING* name, int instance);
    virtual int      GetCSStateFlags();
    int              MoveLocal(double x, double y, double z);
    void             RotateWorld(const zVEC3& axis, double angle);
    void             RotateWorldX(double angle);
    void             RotateWorldY(double angle);
    void             RotateWorldZ(double angle);
    void             RotateLocal(const zVEC3& axis, double angle);
    void             RotateLocalY(double angle);
    void             SetPositionWorld(const zVEC3& position);
    int              SearchFreePosition(zVEC3& position, zCVob* ignoreVob);
    void             SetVisual(zCVisual* visual);
    zVEC3            GetVelocity();
    void             SetVobID(const unsigned long& id);
    unsigned long    GetVobID() const;
    void             SetAI(zCAIBase* ai);
    void             SetVobPresetName(const zSTRING& name);
    const zSTRING&   GetVobPresetName() const;
    zCRigidBody*     GetRigidBody();
    void             SetVobName(const zSTRING& name);
    zCEventManager*  GetEM(int dontCreate);
    virtual int      TraceRay(const zVEC3& origin, const zVEC3& direction, int flags,
        zTTraceRayReport& report);
    zCOLOR           CalcLightSampleAtOrigin();
    zCOLOR           GetLightColorStat();
    zCOLOR           GetLightColorStatGroundPoly();
    zCOLOR           GetLightColorDyn();
    int              CalcStaticLight(zVEC3& color) const;
    virtual void     OnTrigger(zCVob* other, zCVob* instigator);
    virtual void     OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void     OnTouch(zCVob* other);
    virtual void     OnUntouch(zCVob* other);
    virtual void     OnTouchLevel();
    virtual void     OnDamage(zCVob* other, zCVob* instigator, float damage, int damageType,
        const zVEC3& hitLocation);
    virtual void     OnMessage(zCEventMessage* message, zCVob* sender);
    void             SetStaticVob(int enabled);
    zTVobType        GetVobType() const;
    void             SetDrawBBox3D(int enabled);
    void             SetCollDet(int enabled);

    int                lastTimeDrawn;
    zCVisual*          visual;
    zMAT4              trafoObjToWorld;
    zMAT4*             trafo;
    zCWorld*           homeWorld;
    int                showVisual;
    unsigned long      castDynShadow;
    int                collButNoMove;
    int                m_zBias;
    int                type;
    float              visualAlpha;
    zCAIBase*          callback_ai;
    zCEventManager*    eventManager;
    zSTRING*           vobPresetName;
    zCRigidBody*       rigidBody;
    zCCollisionObject* m_poCollisionObject;
    zCCollisionObjectClass* m_poCollisionObjectClass;
    zCTree*            globalVobTreeNode;
    zCPolygon*         groundPoly;
    float              nextOnTimer;
    zTBBox3D           bbox3D;
    int                m_AniMode;
    float              m_aniModeStrength;
    float              m_fVobFarClipZScale;
    int                sleepingMode;
    int                isInMovementMode;
    int                physicsEnabled;
    zCOLOR             lightColorStat;
    zCOLOR             lightColorDyn;
    unsigned long      groundShadowSizePacked;

    const zSTRING*   GetSectorNameVobIsIn() const;
    static void      LoadGroundShadowMesh();
    static void      DeleteGroundShadowMesh();
    void             SetGroundShadowSize(const zVEC2& size);
    zVEC2            GetGroundShadowSize() const;
    void             RenderGroundShadow(zTRenderContext& context);

    void ArchivePacked(zCArchiver& archiver);
    void ArchiveVerbose(zCArchiver& archiver);
    virtual void Archive(zCArchiver& archiver);
    void UnarchivePacked(zCArchiver& archiver, int version);
    void UnarchiveVerbose(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    zCArray<zCBspLeaf*> vobLeafList;
    zCArray<zCVob*>    touchVobList;

    zCVisual*        GetClassHelperVisual() const;
    void             CalcRenderAlpha(float distance, zCVisual*& visual, float& alpha);
    virtual int      Render(zTRenderContext& context);
    zCOLOR           GetLightColorAtCenter() const;
    void             RemoveVobFromWorld();
    void             RemoveVobSubtreeFromWorld();
    virtual void     ThisVobAddedToWorld(zCWorld* world);
    virtual void     ThisVobRemovedFromWorld(zCWorld* world);
    void             AddRefVobSubtree(zCTree* node, int lightCleanup);
    void             ReleaseVobSubtree(zCTree* node);
    void             RemoveWorldDependencies(int removeChildren);
    void             RemoveVobFromGlobalVobTree();
    void             RemoveVobFromBspTree();
    zSTRING          GetVobInfo();
    virtual void     PostLoad();
    virtual void     SetVisual(const zSTRING& fileName);
    void             SetSleeping(int sleeping);
    virtual void     SetSleepingMode(int mode);
    void             UpdateVisualDependencies(int endFlags);
    void             AddVobToWorld_CorrectParentDependencies();
    void             ResetOnTimer();
    void             SetOnTimer(float seconds);
    int              IsOnTimer() const;
    void             ProcessOnTimer();
    void             DoFrameActivity();
    virtual void     OnTimer();
    virtual void     OnTick();
    virtual void     ReportCollisionToAI(const zCCollisionReport& report);

    struct zTModelLimbColl {
        zTModelLimbColl();

        zCVob*                 hitVob;
        zCMaterial*            hitMaterial;
        zCList<zCModelNodeInst> hitModelNodeList;
        zVEC3                  approxCollisionPos;
    };

    void CheckModelLimbCollision(zCModelNodeInst* node, int flags,
        zCList<zTModelLimbColl>& collisions);
    void CheckModelLimbCollisionLevel(zCModelNodeInst* node, int flags,
        zCList<zTModelLimbColl>& collisions);
    void CheckModelLimbCollisionPolys(zCModelNodeInst* node, int flags, zCPolygon** polygons,
        const int& numPolygons, zCList<zTModelLimbColl>& collisions);
    zMAT4            GetTrafoModelNodeToWorld(const zSTRING& nodeName);
    zMAT4            GetTrafoModelNodeToWorld(zCModelNodeInst* node);
    zTBBox2D         GetScreenBBox2D();
    static void      RepairIllegalFloats(zMAT4& trafo);
    void             PackStateBinary(zCBuffer& buffer) const;
    void             UnpackStateBinary(zCBuffer& buffer);

    static zCClassDef* classDef;
    static zCMesh*     s_shadowMesh;
    static zCCollisionDetector* s_poCollisionDetector;
    static int         s_vobAlreadyInMovement;
    static unsigned long s_nextFreeID;
    static int         s_renderVobs;
    static int         s_showHelperVisuals;
    static int         s_enableAnimations;
    static int         s_ignoreVisuals;

    static void InitVobSystem();
    static void CleanupVobSystem();
    static void ResetIDCtr();
    static unsigned long GetNextFreeVobID();
    static void SetShowHelperVisuals(int show);
    static int GetShowHelperVisuals();
    static int GetAnimationsEnabled();
    static void SetAnimationsEnabled(int enabled);
};

class zCParticleFX {
public:

    static void StopEmitterOutput(zCVisual* visual);
};

class zCPFXPerFrameCallback;

class zCPatch;

class zCTransferConstr {
public:

    zCPatch* targetPatch;
    float formFactor;
};

class zCTransfer {
public:

    zCPatch* targetPatch;
    unsigned short formFactor;
};

class zCPatch {
public:

    zCPatch();
    void DoTransfers();

    zVEC3 center;
    zVEC3 centerLight;
    zVEC3 normal;
    float area;
    zVEC3 radiosity;
    zVEC3 radToShoot;
    short xpos;
    short ypos;
    zCArray<zCTransfer> transferList;
    zVEC3 reflectivity;
};

class zCPatchMap {
public:

    int CheckRaySurfaceIntersection(zVEC3& start, zVEC3& direction, zVEC3& intersection,
        zCPolygon*& foundPoly);
    ~zCPatchMap();

    char hit;
    short xdim;
    short ydim;
    zCArray<zCPatch*> patchList;
    zCArray<zCPolygon*> surface;
    zTPlane lightRejectPlane0;
    zTPlane lightRejectPlane1;
    zTBBox3D bbox3D;
    zVEC3 lightmapOrigin;
    zVEC3 lightmapUp;
    zVEC3 lightmapRight;
    zCPolygon* lastRayHitPoly;
};

class zCSkyControler;

class zCZone : public zCVob {
public:

    zCZone();
    virtual ~zCZone();
    virtual void SetVisual(zCVisual* visual);
    virtual void SetVisual(const zSTRING& fileName);
    virtual void ProcessZoneList(const zCArraySort<zCZone*>& active, const zCArraySort<zCZone*>& last,
        zCWorld* world);
    virtual zCClassDef* GetZoneMotherClass() const;
    virtual zCClassDef* GetDefaultZoneClass() const;
    virtual zCClassDef* _GetClassDef() const;
    virtual void ProcessZone();
    virtual void ThisVobAddedToWorld(zCWorld* world);
    virtual void ThisVobRemovedFromWorld(zCWorld* world);
    virtual void EndMovement(int flags);
    virtual void Unarchive(zCArchiver& archiver);
    virtual zSTRING GetDebugDescString();
    float GetCamPosWeight();
    void SetZoneVobFlags();
    int GetPriority() const;

    zCWorld* world;
    int priority;

    static zCClassDef* classDef;
};

class zCZoneReverb : public zCZone {
public:

    zCZoneReverb();
    virtual ~zCZoneReverb();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual zSTRING GetDebugDescString();
    float GetActiveWeight();
    virtual zCClassDef* GetDefaultZoneClass() const;
    virtual void ProcessZoneList(const zCArraySort<zCZone*>& active,
        const zCArraySort<zCZone*>& last, zCWorld* world);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    int reverbPresetNr;
    float reverbPresetWeight;
    float innerRangePerc;

    static zCClassDef* classDef;
};

class zCZoneReverbDefault : public zCZoneReverb {
public:

    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual void ProcessZoneList(const zCArraySort<zCZone*>& active,
        const zCArraySort<zCZone*>& last, zCWorld* world);
    virtual void ThisVobAddedToWorld(zCWorld* world);
    virtual void Unarchive(zCArchiver& archiver);
    virtual ~zCZoneReverbDefault();

    static zCClassDef* classDef;
};

class zCZoneVobFarPlane : public zCZone {
public:

    zCZoneVobFarPlane();
    virtual ~zCZoneVobFarPlane();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    float GetActiveFarZ(float farZ);
    virtual zCClassDef* GetDefaultZoneClass() const;
    virtual void ProcessZoneList(const zCArraySort<zCZone*>& active,
        const zCArraySort<zCZone*>& last, zCWorld* world);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    float vobFarZ;
    float innerRangePerc;

    static zCClassDef* classDef;
};

class zCZoneVobFarPlaneDefault : public zCZoneVobFarPlane {
public:

    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual void ThisVobAddedToWorld(zCWorld* world);
    virtual void Unarchive(zCArchiver& archiver);
    virtual ~zCZoneVobFarPlaneDefault();

    static zCClassDef* classDef;
};

class zCZoneMusic : public zCZone {
public:

    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual ~zCZoneMusic();
    static int SetAutochange(int autochange);
    static void GetAutochange(int* autochange);

    static int s_autochange;
    static int s_canruleautochange;
    static zCClassDef* classDef;
};

class oCZoneMusic;
class oCZoneMusicDefault;

struct oCZoneMusicList {
    oCZoneMusic* entry;
    oCZoneMusicList* next;
};

class oCZoneMusic : public zCZoneMusic {
public:

    oCZoneMusic();
    virtual ~oCZoneMusic();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    void SetEnabled(int value);
    int IsEnabled() const;
    void SetPriority(int value);
    int GetPriority() const;
    void SetEllipsoid(int value);
    int IsEllipsoid() const;
    void SetReverb(float value);
    float GetReverb() const;
    void SetVolume(float value);
    float GetVolume() const;
    void SetLoop(int value);
    int IsLoop() const;
    void SetDayEntranceDone(int value);
    int IsDayEntranceDone() const;
    void SetNightEntranceDone(int value);
    int IsNightEntranceDone() const;
    float GetCamPosWeightElps();
    virtual void OnTrigger(zCVob* other);
    virtual void OnUntrigger(zCVob* other);
    virtual void ProcessZoneList(const zCArraySort<zCZone*>& active,
        const zCArraySort<zCZone*>& last, zCWorld* world);
    virtual zCClassDef* GetDefaultZoneClass() const;
    virtual zSTRING GetDebugDescString();
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    int enabled;
    int local_enabled;
    int priority;
    int ellipsoid;
    float reverbLevel;
    float volumeLevel;
    int loop;
    int dayEntranceDone;
    int nightEntranceDone;

    static zCClassDef* classDef;
    static long s_lastzonechange;
    static int s_zonetime;
    static int s_daytime;
    static int s_herostatus;
    static oCZoneMusic* s_musiczone;
    static oCZoneMusic* s_oldmusiczone;
    static void GetNewTheme(zSTRING* theme);
    static int PlayThemeVariation(zSTRING theme, int variation);
    static oCZoneMusicList* BuildTempZoneList(oCZoneMusicDefault* defaultZone,
        const zCArraySort<zCZone*>& zones);
    static void ReleaseTempZoneList(oCZoneMusicList* list);
    static void SwitchInAndOutZones(const zCArraySort<zCZone*>& active,
        const zCArraySort<zCZone*>& last);
    static oCZoneMusic* GetMusicZoneByName(oCZoneMusic* zone, zSTRING name);
    static int IsDaytime();
    static int GetHerostatus();
};

class oCZoneMusicDefault : public oCZoneMusic {
public:

    oCZoneMusicDefault();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual ~oCZoneMusicDefault();

    static zCClassDef* classDef;
};

class zCVobSound : public zCZone {
public:

    zCVobSound();
    virtual ~zCVobSound();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    void SetSound(const zSTRING& name);
    zCSoundFX* LoadSound(const zSTRING& name);
    void SetSoundVolume(float volume);
    void SetSoundRadius(float radius);
    float CalcObstruction();
    float CalcVolumeScale();
    void StartSound(int forceCacheIn);
    void StopSound();
    virtual void DoSoundUpdate(float frameTime);
    virtual void OnTrigger(zCVob* other, zCVob* instigator);
    virtual void OnUntrigger(zCVob* other, zCVob* instigator);
    virtual void OnMessage(zCEventMessage* message, zCVob* sender);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void ProcessZoneList(const zCArraySort<zCZone*>& active,
        const zCArraySort<zCZone*>& last, zCWorld* world);
    virtual void ThisVobAddedToWorld(zCWorld* world);
    virtual void ThisVobRemovedFromWorld(zCWorld* world);
    virtual void EndMovement(int flags);
    virtual zSTRING GetDebugDescString();

    zSTRING soundName;
    float soundRadius;
    int soundMode;
    unsigned char soundStartOn;
    float soundRandDelay;
    float soundRandDelayVar;
    float soundVolume;
    float soundConeAngle;
    zCSoundFX* sfx;
    int sfxHandle;
    float soundRandTimer;
    float obstruction0;
    float obstruction1;
    float obstructionFrameTime;

    static zCClassDef* classDef;
};

class zCVobSoundDaytime : public zCVobSound {
public:

    zCVobSoundDaytime();
    virtual ~zCVobSoundDaytime();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual zCClassDef* GetZoneMotherClass() const;
    void ActivateSection(int section);
    int CalcTimeFrac(float start, float end, float current, float& fraction);
    virtual void DoSoundUpdate(float frameTime);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual zSTRING GetDebugDescString();

    float soundStartTime;
    float soundEndTime;
    zSTRING soundName2;
    zCSoundFX* sfx2;
    int activeSection;

    static zCClassDef* classDef;
};

class zCZoneZFog : public zCZone {
public:

    zCZoneZFog();
    virtual ~zCZoneZFog();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    float GetActiveRange(float farZ);
    float GetSkyFadeWeight();
    virtual zCClassDef* GetDefaultZoneClass() const;
    virtual void ProcessZoneList(const zCArraySort<zCZone*>& active,
        const zCArraySort<zCZone*>& last, zCWorld* world);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);

    float fogRangeCenter;
    float innerRangePerc;
    zCOLOR fogColor;
    int bFadeOutSky;
    int bOverrideColor;

    static zCClassDef* classDef;
};

class zCZoneZFogDefault : public zCZoneZFog {
public:

    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual void ThisVobAddedToWorld(zCWorld* world);
    virtual void Unarchive(zCArchiver& archiver);
    virtual ~zCZoneZFogDefault();

    static zCClassDef* classDef;
};

class zCWayNet;
class zCCBspTree;
class zCBspBase;
class zCWorldPerFrameCallback {
public:

    virtual void DoWorldPerFrameCallback(zCWorld* world, zCCamera* camera);
};

class zCVobCallback {
public:

    virtual void HandleVob(zCVob* vob, void* data);
};

class zCRayCache;

class zCView;

class zCCSDebugger {
public:

    zCCSDebugger() : callframe(NULL), deb(NULL) {}
    virtual ~zCCSDebugger() {}
    virtual void CSDB_BeginSyncBlock(zCCSSyncBlock* block);
    virtual void CSDB_BeginAtomBlock(zCCSBlockBase* block);
    virtual void CSDB_EndAtomBlock(zCCSBlockBase* block);
    virtual void CSDB_Error(zCCSBlockBase* block, zSTRING& text);
    virtual void CSDB_Warning(zCCSBlockBase* block, zSTRING& text);
    virtual void CSDB_StartPlaying(zCCutscene* cutscene);
    virtual void CSDB_Stop(zCCutscene* cutscene);

    zCArray<zCCSBlockBase*> callstack;
    zCCSSyncBlock* callframe;
    zCCSDebugger* deb;
};

class zCCSPlayer : public zCObject, public zCCSDebugger {
public:

    zCCSPlayer();
    virtual ~zCCSPlayer();
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual void SetOwnerWorld(zCWorld* world);
    virtual void SetOwnerManager(zCCSManager* manager);
    virtual zCCSCutsceneContext* StartCutscene(zCCutscene* cutscene, zCVob* vob, zSTRING name);
    virtual zCCSCutsceneContext* CreateNewOutputUnit(int unit, zCVob* vob);
    virtual void ForgetCutscene(zCCutscene* cutscene);
    void Interrupt();
    void Resume();
    virtual void InsertCutscene(zCCSCutsceneContext* context);
    virtual void RemoveCutscene(zCCSCutsceneContext* context);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void ProcessCutscene(zCCSCutsceneContext* context, const zVEC3& position);
    virtual void OnCSTrigger(const zSTRING& target, zCVob* other);
    virtual void OnCSUntrigger(const zSTRING& target, zCVob* other);
    virtual void CSDB_BeginSyncBlock(zCCSSyncBlock* block);
    virtual void CSDB_BeginAtomBlock(zCCSBlockBase* block);
    virtual void CSDB_EndAtomBlock(zCCSBlockBase* block);
    virtual void CSDB_Error(zCCSBlockBase* block, zSTRING& text);
    virtual void CSDB_Warning(zCCSBlockBase* block, zSTRING& text);
    virtual void CSDB_StartPlaying(zCCutscene* cutscene);
    virtual void CSDB_Stop(zCCutscene* cutscene);
    void ResetCutscenePlayer();
    virtual int StopAllCutscenes(zCVob* vob);
    virtual int StopAllOutputUnits(zCVob* vob);
    int IsAssigned(zSTRING& name, zSTRING& reference);
    void PrintListOfCutscenes(const zSTRING& prefix);
    zCCSCutsceneContext* GetStartedCutscene(const zSTRING& name);
    int UpdateCutscenesNow();
    void DebugResetInfoscreen();
    void DebugAddCutscene(int index, zVEC3 position, int unused);
    void ProcessList(const zVEC3& position, int hour, int minute, int second);
    void Process();
    void ToggleDebugInfo();
    zCCSCutsceneContext* GetPlayingGlobalCutscene();
    zCCSCutsceneContext* StartCutscene(const zSTRING& name, zCVob* vob);
    zCCSCutsceneContext* GetStartedCutscene(zCVob* vob);
    void StartOutputUnit(int unit, zCVob* vob);

    int lastProcessDay;
    int lastProcessHour;
    zCList<zCCSCutsceneContext>* processingListItem;
    int processingCtr;
    int debugInfoOnScreen;
    zCView* debugInfoScreen;
    float timerCtr;
    zCList<zCCSCutsceneContext> playList;
    zCWorld* ownerWorld;
    zCCSManager* ownerManager;

    static void operator delete(void* memory);
    static zCClassDef* classDef;
};

class zCViewProgressBar;

struct zCBspTree {
    void DisposeTree();
    void AddVob(zCVob* vob);
    void SaveBIN(zCFileBIN& file);
    int  LoadBIN(zCFileBIN& file, int skipMesh);
    void CalcStaticLeafLightData();
    static int GetShowPortals();
    static void SetShowPortals(int show);

    static int s_showPortals;

    int bspTreeMode;
    int worldRenderMode;
    float vobFarClipZ;
    zCBspBase* bspRoot;
    zCMesh* mesh;
    int numPolys;
    int m_bRenderedFirstTime;
    zCArray<zCVob*> renderVobList;
};

template <class T>
class zCListSort;

class zCRoute {
public:

    zCRoute();
    virtual ~zCRoute();
    void SetStart(zCWaypoint* waypoint);
    zCWaypoint* GetNextWP();
    zCWaypoint* GetTargetWP();
    zCWay* GetCurrentWay();
    zSTRING GetDesc();
    zSTRING GetRemainingDesc();
    int GetNumberOfWaypoints();
    int IsEmpty();
    int IsLastTarget();
    int GetInterpolatedPosition(float time, int speed, zVEC3& position);

    zCList<zCWay> wayList;
    zCWaypoint* startwp;
    zCWaypoint* target;
    zCWay* way;
    zCList<zCWay>* waynode;
};

class zCWayNet : public zCObject {
public:

    enum {
        WAY_LIST_NONE,
        WAY_LIST_OPEN,
        WAY_LIST_CLOSED
    };

    zCWayNet();
    zCWayNet(zCWorld* world);
    virtual ~zCWayNet();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    zCWaypoint* HasWaypoint(float x, float y, float z);
    int HasWaypoint(zCWaypoint* waypoint);
    zCWaypoint* HasWaypoint(zVEC3& position);
    zCWaypoint* InsertWaypoint(float x, float y, float z);
    void InsertWaypoint(zCWaypoint* waypoint);
    void InsertWaypoint(zCWaypoint* first, zCWaypoint* middle, zCWaypoint* last);
    void Draw(zCCamera* camera);
    void CorrectHeight();
    void UpdateVobDependencies();
    void ClearVobDependencies();
    void CreateVobDependencies(zCWorld* world);
    zCWaypoint* GetSecNearestWaypoint(zVEC3& position);
    zCWaypoint* GetNearestWaypoint(const zVEC3& position);
    zCWaypoint* GetWaypoint(const zSTRING& name);
    void CreateWay(zCWaypoint* first, zCWaypoint* second);
    void DeleteWay(zCWaypoint* first, zCWaypoint* second);
    void DeleteWaypoint(zCWaypoint* waypoint);
    zSTRING MergeWaypoints();
    zSTRING CheckConsistency(int flags);
    void AddWays(zCWaypoint* target, zCWaypoint* source);
    zCWaypoint* SearchWaypoint(zCVob* vob);
    zCRoute* FindRoute(const zVEC3& start, const zVEC3& target, const zCVob* vob);
    zCRoute* FindRoute(const zVEC3& start, zCWaypoint* target, const zCVob* vob);
    zCRoute* FindRoute(const zVEC3& start, const zSTRING& target, const zCVob* vob);
    zCRoute* FindRoute(zCWaypoint* start, zCWaypoint* target, const zCVob* vob);
    zCRoute* FindRoute(const zSTRING& start, const zSTRING& target, const zCVob* vob);
    void InsertInOpen(zCWaypoint* waypoint);
    int IsInOpen(zCWaypoint* waypoint);
    void InsertInClosed(zCWaypoint* waypoint);
    void RemoveFromClosed(zCWaypoint* waypoint);
    int IsInClosed(zCWaypoint* waypoint);
    int IsInAnyList(zCWaypoint* waypoint);
    int EstimateCost(zCWaypoint* start, zCWaypoint* target);
    zCRoute* CreateRoute(zCWaypoint* target);
    int AStar(zCWaypoint* start, zCWaypoint* target, const zCVob* vob);
    void GetWPList(zCList<zVEC3>& list, zVEC3& position, float minDistance, float maxDistance);
    void CreateWPVobList(zCList<zCVobWaypoint>& list, zCTree<zCVob>* tree);
    void RemoveUnusedWPVobs();
    void ArchiveOldFormat(zCArchiver& archiver);
    void UnarchiveOldFormat(zCArchiver& archiver);
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    void CalcProperties(zCWorld* world);

    zCWorld* world;
    zCListSort<zCWaypoint> wplist;
    zCList<zCWay> waylist;
    zCListSort<zCWaypoint> openList;
    int routeCtr;

    static zCClassDef* classDef;
    static zCObject* _CreateNewInstance();
    static int SortWpList(zCWaypoint* first, zCWaypoint* second);
    static int SortOpenList(zCWaypoint* first, zCWaypoint* second);
};

class zCBBox3DSorterBase {
public:

    class zTNode {
    public:

        int isBegin;
        void* data;
    };

    class zTBoxSortHandle {
    public:

        zTBoxSortHandle();
        virtual ~zTBoxSortHandle();
        virtual void AddActive(void* data);
        virtual void DelActive(void* data);
        virtual void ClearActive();

        zCBBox3DSorterBase* mySorter;
        zTBBox3D bbox3D;
        int indexBegin[3];
        int indexEnd[3];
    };

    zCBBox3DSorterBase();
    virtual ~zCBBox3DSorterBase();
    void Clear();
    void AllocAbs(int count);
    void Insert(void* data);
    void Remove(void* data);
    void Update(void* data);
    void GetActiveList(const zTBBox3D& bbox, zTBoxSortHandle& handle);
    void UpdateActiveList(const zTBBox3D& bbox, zTBoxSortHandle& handle);
    void InsertHandle(zTBoxSortHandle& handle);
    void RemoveHandle(zTBoxSortHandle& handle);
    void Sort();
    void AdjustSorting(zCArraySort<zTNode*>& nodes, int dimension);
    virtual const zTBBox3D& GetBBox(const zTNode* node);

    static int ArrayCompare(const void* first, const void* second);
    static zCBBox3DSorterBase* s_currentSorter;
    static int s_currentDimension;

    zCArray<zTBoxSortHandle*> handles;
    zCArraySort<zTNode*> nodeList[3];
    int sorted;
};

class zCWorld : public zCObject {
public:

    zCWorld();
    virtual ~zCWorld();

    zSTRING GetWorldName() const;
    static zCObject* _CreateNewInstance();
    zCClassDef*      _GetClassDef() const;
    void             DisposeWorld();
    void*            GetPlayerGroup();
    zCSkyControler*  GetActiveSkyControler();
    void             SetSkyControlerIndoor(zCSkyControler* sky);
    void             SetSkyControlerOutdoor(zCSkyControler* sky);
    static int       ActiveZoneListCompare(const void* first, const void* second);
    void             RenderWaynet(zCCamera* camera);
    void             AddZone(zCZone* zone);
    void             RemoveZone(zCZone* zone);
    void             RemoveAllZones();
    void             UpdateZone(zCZone* zone);
    void             ProcessZones();
    void             ShowZonesDebugInfo();
    void             ShowTextureStats();
    int              TraceRayFirstHit(const zVEC3& origin, const zVEC3& direction,
        const zCArray<zCVob*>* ignoreVobs, int flags);
    int              TraceRayNearestHit(const zVEC3& origin, const zVEC3& direction,
        const zCArray<zCVob*>* ignoreVobs, int flags);
    int              TraceRayFirstHit(const zVEC3& origin, const zVEC3& direction,
        zCVob* ignoreVob, int flags);
    int              TraceRayNearestHit(const zVEC3& origin, const zVEC3& direction,
        zCVob* ignoreVob, int flags);
    int              TraceRayFirstHitCache(const zVEC3& origin, const zVEC3& direction,
        const zCArray<zCVob*>* ignoreVobs, int flags, zCRayCache* cache);
    int              TraceRayFirstHitCache(const zVEC3& origin, const zVEC3& direction,
        zCVob* ignoreVob, int flags, zCRayCache* cache);
    int              TraceRayNearestHitCache(const zVEC3& origin, const zVEC3& direction,
        const zCArray<zCVob*>* ignoreVobs, int flags, zCRayCache* cache);
    int              TraceRayNearestHitCache(const zVEC3& origin, const zVEC3& direction,
        zCVob* ignoreVob, int flags, zCRayCache* cache);
    int              PickScene(zCCamera& camera, int x, int y, float distance);
    int              TraverseBsp(zCCBspTree* tree, zCTree<zCVob>* node, int mode);
    int              TraverseBspAddVobsToMesh(zCCBspTree* tree, zCTree<zCVob>* node);
    void             SaveBspTreeMesh3DS(const zSTRING& fileName);
    void             CompileWorld(int bspMode, float priority, int flags, int lighting,
        zCArray<zCPolygon*>* polys);
    virtual int      DisposeVobs(zCTree<zCVob>* node);
    virtual int      DisposeVobsDbg(zCTree<zCVob>* node);
    virtual void     DisposeStaticWorld();
    virtual zCVob*   SearchVob(zCVob* vob, zCTree<zCVob>* node);
    virtual zCVob*   SearchVobByID(unsigned long vobID, zCTree<zCVob>* node);
    virtual zCVob*   SearchVobByName(const zSTRING& name);
    virtual void     SearchVobListByName(const zSTRING& name, zCArray<zCVob*>& vobs);
    virtual void     SearchVobListByClass(zCClassDef* classDef, zCArray<zCVob*>& vobs,
        zCTree<zCVob>* node);
    virtual void     SearchVobListByBaseClass(zCClassDef* classDef, zCArray<zCVob*>& vobs,
        zCTree<zCVob>* node);
    void             SearchZoneListByClass(zCClassDef* classDef, zCArray<zCZone*>& zones);
    zCZone*          SearchZoneDefaultByClass(zCClassDef* classDef);
    int              ShouldAddThisVobToBsp(zCVob* vob) const;
    virtual zCTree<zCVob>* AddVobAsChild(zCVob* vob, zCTree<zCVob>* parent);
    zCTree<zCVob>*   AddVobAsChild(zCVob* vob, zCVob* parent);
    zCTree<zCVob>*   AddVob(zCVob* vob);
    virtual void     VobAddedToWorld(zCVob* vob);
    virtual void     VobRemovedFromWorld(zCVob* vob);
    virtual void     RemoveVob(zCVob* vob);
    void             RemoveVob(zCTree<zCVob>* node);
    void             RemoveVobSubtree_r(zCTree<zCVob>* node, int removeChildren);
    virtual void     RemoveVobSubtree(zCVob* vob);
    void             RemoveVobSubtree(zCTree<zCVob>* node);
    virtual void     MoveVobSubtreeTo(zCVob* vob, zCTree<zCVob>* parent);
    void             MoveVobSubtreeTo(zCVob* vob, zCVob* parent);
    void             MoveVobSubtreeToWorldSpace(zCVob* vob);
    void             RemoveVobFromLists(zCVob* vob);
    void             InsertVobHashTable(zCVob* vob);
    void             RemoveVobHashTable(zCVob* vob);
    void             UpdateVobTreeBspDependencies(zCTree<zCVob>* node);
    void             MoveVobs();
    void             AdvanceClock(float seconds);
    void             PrintStatus();
    void             PrintGlobalVobTree(zCTree<zCVob>* node, int indentation);
    void             PrintActiveVobs();
    void             SaveBspTree(zFILE& file);
    int              LoadBspTree(zFILE& file, int skipMesh);
    void             DebugMarkOccluderPolys();
    void             UnarcTraverseVobs(zCArchiver& archiver, zCTree<zCVob>* parent);
    zCViewProgressBar* GetProgressBar() const;
    zCBspTree*       GetBspTree();
    virtual void     Unarchive(zCArchiver& archiver);
    enum zTWorldLoadMode {
        zWORLD_LOAD_DEFAULT
    };
    enum zTWorldSaveMode {
        zWORLD_SAVE_DEFAULT
    };
    virtual int      LoadWorld(const zSTRING& fileName, int loadMode);
    virtual zCVob*   MergeVobSubtree(const zSTRING& fileName, zCVob* parent, int mergeMode);
    void             ArcTraverseVobs(zCArchiver& archiver, zCTree<zCVob>* node, int writeRoot);
    virtual void     Archive(zCArchiver& archiver);
    virtual int      SaveWorld(const zSTRING& fileName, int saveMode, int saveGame, int export3DS);
    virtual int      SaveVobSubtree(const zSTRING& fileName, zCVob* vob, int saveGame,
        int export3DS);
    void             TraverseVobTree(zCVobCallback& callback, void* data, zCTree<zCVob>* node);
    unsigned long    GetVobHashIndex(const zSTRING& name);
    unsigned long    GetVobHashIndex(zCVob* vob);
    zCVob*           SearchVobHashTable(const zSTRING& name);
    void             SearchVobListHashTable(const zSTRING& name, zCArray<zCVob*>& vobs);
    void             RegisterPerFrameCallback(zCWorldPerFrameCallback* callback);
    void             UnregisterPerFrameCallback(zCWorldPerFrameCallback* callback);
    void             ResetCutscenePlayer();
    int              Release();
    void             Render(zCCamera* camera);
    void             SetOwnerSession(zCSession* session);
    zCSession*       GetOwnerSession();
    void             LightWorldStaticUncompiled(zCTree<zCVob>* node);
    void             LightWorldStaticCompiled();
    void             GenerateStaticVertexLighting();
    int              LightingTestRay(const zVEC3& start, const zVEC3& end, zVEC3& intersection,
        zCPolygon*& foundPoly);
    int              GetSurfaceLightmapBBox2D(zCArray<zCPolygon*>& surface, zTBBox2D& bbox,
        int* const size);
    zVEC3            GetPhongNormal(zCPolygon* polygon, const zVEC3& position);
    zCPatchMap*      GeneratePatchMapFromSurface(zCArray<zCPolygon*>& surface);
    void             GenerateLightmapFromPatchMap(zCPatchMap* patchMap);
    void             LightPatchMap(zCPatchMap* patchMap);
    void             GenerateSurfaces(int generateLightmaps, zTBBox3D* bounds);
    void             MakeTransfers();
    void             GenerateLightmapsRadiosity(zTBBox3D* bounds);
    void             GenerateLightmaps(const int& lightMode, zTBBox3D* bounds);
    void             GenerateStaticWorldLighting(const int& lightMode, zTBBox3D* bounds);
    void             MakeVobLightingDirty();

    zCTree<zCVob> globalVobTree;
    zCList<zCVob>* voblist_npcs;
    zTTraceRayReport traceRayReport;
    zCViewProgressBar* progressBar;
    zCSession* ownerSession;
    zCCSPlayer* csPlayer;
    zSTRING m_strlevelName;
    int compiled;
    int compiledEditorMode;
    int traceRayIgnoreVobFlag;
    int m_bIsInventoryWorld;
    int worldRenderMode;
    zCWayNet* wayNet;
    int masterFrameCtr;
    float vobFarClipZ;
    float vobFarClipZScalability;
    zCArray<zCVob*> traceRayVobList;
    zCArray<zCVob*> traceRayTempIgnoreVobList;
    int renderingFirstTime;
    int showWaynet;
    int showTraceRayLines;
    zCViewProgressBar* progressBar;
    unsigned long unarchiveFileLen;
    unsigned long unarchiveStartPosVobtree;
    int numVobsInWorld;
    zCArray<zCWorldPerFrameCallback*> perFrameCallbackList;
    zCSkyControler* skyControlerIndoor;
    zCSkyControler* skyControlerOutdoor;
    zCSkyControler* activeSkyControler;
    zCArray<zCZone*> zoneGlobalList;
    zCArraySort<zCZone*> zoneActiveList;
    zCArraySort<zCZone*> zoneLastClassList;
    zCBBox3DSorterBase zoneBoxSorter;
    int addZonesToWorld;
    int showZonesDebugInfo;
    zCCBspTree* cbspTree;
    zCBspTree bspTree;
    zCArray<zCVob*> activeVobList;
    zCArray<zCVob*> walkList;
    zCArray<zCVob*> vobHashTable[2048];

    static zCClassDef* classDef;
    static int showTextureStats;
    static int s_bWaveAnisEnabled;
    static int s_worldLoadMode;
    static int s_worldSaveMode;
    static int s_worldMergeMode;
    static int s_bFadeOutFarVerts;
    static int s_bAlternateRenderOrder;
    static int s_bEnvMappingEnabled;
    static int s_bAmbientVobsEnabled;
    static int GetWaveAnisEnabled();
    static void SetWaveAnisEnabled(int enabled);
    static int GetShowTextureStats();
    static void SetShowTextureStats(int enabled);
    void SetVobFarClipZScalability(float scalability);
    static void operator delete(void* memory);
};

struct zTMdl_StartedVobFX {
    zCVob* vob;
    float  vobFXHandle;
};

struct zTMdl_NodeVobAttachment {
    zCVob*           vob;
    zCModelNodeInst* mnode;
};

struct zTMeshLibEntry {
    zCModelTexAniState texAniState;
    zCModelMeshLib*    meshLib;
};

struct zTTraceRayReport {
    int        foundHit;
    zCVob*     foundVob;
    zCPolygon* foundPoly;
    zVEC3      foundIntersection;
    zVEC3      foundPolyNormal;
    zCVertex*  foundVertex;
    zVEC3      foundHitPosition;
};

class zCOBBox3D {
public:

    static int TestIntersectionTree(const zTBBox3D& bbox);

    zVEC3 center;
    zVEC3 axis[3];
    zVEC3 extent;
};

class zCModel : public zCVisual {
public:

    zCModel();
    zCModel(zCModelPrototype* model);
    ~zCModel();

    static zCObject* _CreateNewInstance();
    static void      operator delete(void* memory);
    zCClassDef*      _GetClassDef() const;
    void             Init();
    void             CopyProtoNodeList();
    void             SetModelScale(const zVEC3& scale);
    void             SetupLowestLOD(zTRenderContext& context);
    void             RenderNodeList(zTRenderContext& context, zCArray<zMAT4*>& trafos,
        zCRenderLightContainer& lights, zTPMLightingMode mode);
    int              Render(zTRenderContext& context);
    void             CheckNodeCollisionList(const zCOBBox3D& bbox, zMAT4& trafo);
    void             CheckNodeCollision(zCModel* other, zCModelNodeInst* node, zMAT4& trafo,
        zCList<zCModelNodeInst>& results);
    int              TraceRay(const zVEC3& origin, const zVEC3& direction, int flags,
        zTTraceRayReport& report);
    void             SetVisualUsedBy(zCVob* vob);
    const zSTRING*   GetFileExtension(int index);
    zCVisual*        LoadVisualVirtual(const zSTRING& fileName) const;
    int              ApplyModelProtoOverlay(const zSTRING& fileName);
    int              ApplyModelProtoOverlay(zCModelPrototype* model);
    int              HasAppliedModelProtoOverlay(zCModelPrototype* model) const;
    int              HasAppliedModelProtoOverlay(const zSTRING& fileName) const;
    void             RemoveModelProtoOverlay(const zSTRING& fileName);
    void             RemoveModelProtoOverlay(zCModelPrototype* model);
    void             CalcNodeListBBoxWorld();
    zTBBox3D         GetBBox3DNodeWorld(zCModelNodeInst* node);
    zVEC3            GetNodePositionWorld(zCModelNodeInst* node);
    void             CalcModelBBox3DWorld();
    void             SetNodeVisual(zCModelNodeInst* node, zCVisual* visual, int buildTexList);
    void             SetDynLightMode(int mode);
    void             HostVobRemovedFromWorld(zCVob* vob, zCWorld* world);
    int              GetLowestLODNumPolys();
    zVEC3*           GetLowestLODPoly(int index, zVEC3*& vertices);
    zVEC3            GetAniTransLerp(zCModelAni* ani, float progress, int flags) const;
    const zSTRING*   GetAnyAnimation();
    int              IsBBox3DLocal();
    int              CanTraceRay() const;
    unsigned long    GetRenderSortKey() const;
    zCMaterial*      GetMaterial(int index);
    int              GetNumMaterials() const;
    zCModelAni*      GetAniFromAniID(int aniID) const;
    int              IsAniActive(zCModelAni* ani);
    zCModelAniActive* GetActiveAni(int aniID) const;
    zCModelAniActive* GetActiveAni(zCModelAni* ani) const;
    int              GetAniIDFromAniName(const zSTRING& name) const;
    void             SetRandAnisEnabled(int enabled);
    void             GetAniMinMaxWeight(zCModelAniActive* ani, float& minWeight,
        float& maxWeight);
    void             PrintStatus(int x, int y);
    int              CorrectAniFreezer();
    void             ShowAniListAdd(zCModelAni* ani);
    void             ShowAniList(int x);
    void             StopAni(int aniID);
    void             StopAni(zCModelAni* ani);
    void             StopAni(zCModelAniActive* ani);
    void             StopAnisLayerRange(int minLayer, int maxLayer);
    void             FadeOutAni(int aniID);
    void             FadeOutAni(zCModelAni* ani);
    void             FadeOutAni(zCModelAniActive* ani);
    void             FadeOutAnisLayerRange(int minLayer, int maxLayer);
    float            GetProgressPercent(const zSTRING& name) const;
    float            GetProgressPercent(int aniID) const;
    void             SetCombineAniXY(int aniID, float x, float y) const;
    int              GetCombineAniXY(int aniID, float& x, float& y) const;
    void             StartAnimation(const zSTRING& name);
    void             StopAnimation(const zSTRING& name);
    int              IsAnimationActive(const zSTRING& name);
    void             StartAni(const zSTRING& name, int startMode);
    void             StartAni(int aniID, int startMode);
    void             StartAni(zCModelAni* ani, int startMode);
    void             AssertActiveAniListAlloced();
    void             DoAniEvents(zCModelAniActive* ani);
    void             AdvanceAni(zCModelAniActive* ani, int& frame);
    void             AdvanceAnis();
    void             psb_WriteAniBlock(zCBuffer& buffer, int index, int reserved) const;
    void             psb_ReadAniBlock(zCBuffer& buffer, int index, zCModelAniActive* ani);
    void             PackStateBinary(zCBuffer& buffer);
    void             UnpackStateBinary(zCBuffer& buffer);
    void             CalcNodeListAniBlending();
    void             CalcTransBlending();
    int              IsStateActive(const zCModelAni* ani) const;
    zTBBox3D         GetBBox3D();
    zSTRING          GetVisualName();
    zCModelNodeInst* SearchNode(const zSTRING& name);
    zVEC3            GetVelocityRing() const;
    void             ResetVelocity();
    int              SetNodeMeshTexture(const zSTRING& nodeName, int channel, int frame,
        zSTRING* filter);
    int              SetMeshLibTexture(const zSTRING& fileName, int channel, int frame,
        zSTRING* filter);
    void             UpdateMeshLibTexAniState();
    void             RemoveMeshLibAll();
    int              RemoveMeshLib(const zSTRING& fileName);
    int              ApplyMeshLib(const zSTRING& fileName);
    zTAniAttachment* SearchAniAttachList(int aniID) const;
    void             RemoveAniAttachment(int aniID);
    void             RemoveAllAniAttachments();
    zTAniAttachment* GetCreateAniAttachment(int aniID);
    static int       AniAttachmentCompare(const void* ele1, const void* ele2);
    void             DeleteRandAniList(int aniID);
    void             InsertRandAni(int aniID, int randAniProtoID, int prob);
    float            GetRandAniFreq(int aniID) const;
    void             SetRandAniFreq(int aniID, float frequency);
    void             RecalcRootPosLocal(int highest);
    void             AttachChildVobToNode(zCVob* vob, zCModelNodeInst* node);
    void             RemoveChildVobFromNode(zCVob* vob);
    zCVob*           GetAttachedNodeVob(zCModelNodeInst* node);
    void             RemoveAllChildVobsFromNode();
    void             UpdateAttachedVobs();
    zMAT4            GetTrafoNodeToModel(zCModelNodeInst* node);
    void             RemoveStartedVobFX(zCVob* vob);
    void             RemoveAllVobFX();

    int                          numActiveAnis;
    zCModelAniActive*            aniChannels[6];
    zCModelAniActive*            activeAniList;
    zCVob*                       homeVob;
    zCArray<zCModelPrototype*>   modelProtoList;
    zCArray<zCModelNodeInst*>    nodeList;
    zCArray<zTMdl_NodeVobAttachment> attachedVobList;
    zCArray<zTMdl_StartedVobFX>  startedVobFX;
    zCArray<zCMeshSoftSkin*>     meshSoftSkinList;
    zCArray<zTMeshLibEntry*>     meshLibList;
    zCArraySort<zTAniAttachment*> aniAttachList;
    zCArray<zCModelAniEvent*>    occuredAniEvents;
    int                          lastTimeBBox3DTreeUpdate;
    zTBBox3D                     bbox3D;
    zTBBox3D                     bbox3DLocalFixed;
    zTBBox3D                     bbox3DCollDet;
    float                        modelDistanceToCam;
    int                          n_bIsInMobInteraction;
    float                        fatness;
    zVEC3                        modelScale;
    zVEC3                        aniTransScale;
    zVEC3                        rootPosLocal;
    zVEC3                        vobTrans;
    zVEC3                        vobTransRing;
    int                          newAniStarted;
    int                          m_bSmoothRootNode;
    float                        relaxWeight;
    int                          m_bDrawHandVisualsOnly;
    zCQuat                       vobRot;
    zVEC3                        modelVelocity;
    int                          actVelRingPos;
    zVEC3                        modelVelRing[8];
    float                        timeScale;
    zCModelAni**                 aniHistoryList;
    int                          isVisible;

    static zCClassDef* classDef;
    static int s_drawSkeleton;
    static int s_bSmoothRootNode;
    static int* s_iNumSoftSkinPoly;
};

struct zFINDDATA {
    unsigned int attributes;
    unsigned long size;
    char name[260];
};

class zCScanDirFileHandler {
public:

    zCScanDirFileHandler();
    virtual ~zCScanDirFileHandler();
    int CanHandleFile(zSTRING& fileName);
    int GetNumMessages();
    zSTRING GetMessageA(int index);
    void RegisterFileType(zSTRING extension);
    void AddMessage(zSTRING message);
    void ClearMessages();
    void SetName(const zSTRING& value) { name = value; }
    zSTRING& GetName() { return name; }
    virtual int HandleFile(const zSTRING& path, const char* fileName,
        const zFINDDATA& findData);

    zCArray<zSTRING> msgList;
    zCArray<zSTRING> extList;
    zSTRING name;
};

class zCScanDir {
public:

    zCScanDir();
    virtual ~zCScanDir();
    void ScanX();
    virtual void RegisterFileHandler(zCScanDirFileHandler* handler, const zSTRING& directory);
    virtual void Scan(const zSTRING& directory);

    zCList<zCScanDirFileHandler> cbList;
    int statFoundFiles;
    int statHandledFiles;
    int statTouchedFiles;
};

class zCModelConvertFileHandler : public zCScanDirFileHandler {
public:

    zCModelConvertFileHandler();
    virtual ~zCModelConvertFileHandler() {}

    virtual int HandleFile(const zSTRING& path, const char* fileName,
        const zFINDDATA& findData);
};

#endif
