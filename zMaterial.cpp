#include "include/zModel.h"
#include "include/zTools.h"
#include "include/zTexture.h"
#include "include/zArchiver.h"
#include <cmath>

static int materialState = -1;
static zCClassDef zCMaterialClassDef("zCMaterial", "zCObject", zCMaterial::_CreateNewInstance,
    1, 0xAC, 9, "");
zCClassDef* zCMaterial::classDef = &zCMaterialClassDef;
static zSTRING materialGroupNames[7] = {
    "UNDEF", "METAL", "STONE", "WOOD", "EARTH", "WATER", "SNOW"
};
int zCMaterial::s_matUsageDefault = 1;

zCObject* zCMaterial::_CreateNewInstance()
{
    zCMaterial* material = new zCMaterial;
    zCClassDef::ObjectCreated(material, classDef);
    return material;
}

zCClassDef* zCMaterial::_GetClassDef() const
{
    return classDef;
}

zCMaterial::zCMaterial()
{
    texAniCtrl.aniChannel = 0;
    texAniCtrl.actFrame = 0.0f;
    texAniCtrl.aniFPS = 0.0f;
    texAniCtrl.frameCtr = 0xFFFFFFFF;
    texAniCtrl.bOneShotAni = 0;
    InitValues();
}

zCMaterial::zCMaterial(const zSTRING& name)
    : zCMaterial()
{
    SetObjectName(name);
}

zCMaterial::zCMaterial(const zCMaterial& other)
    : zCMaterial()
{
    InitThisByMaterial(other);
}

zCMaterial& zCMaterial::operator=(const zCMaterial& other)
{
    InitThisByMaterial(other);
    return *this;
}

zCMaterial::~zCMaterial()
{
    if (texture) {
        texture->Release();
        texture = NULL;
    }
    if (detailTexture) {
        detailTexture->Release();
        detailTexture = NULL;
    }
    delete detailObjectVisualName;
    detailObjectVisualName = NULL;
    bspSectorFront = NULL;
    bspSectorBack = NULL;
}

void zCMaterial::InitValues()
{
    default_mapping[0] = 2.34375f;
    default_mapping[1] = 2.34375f;
    texScale[0] = 1.0f;
    texScale[1] = 1.0f;
    texAniMapDelta[0] = 0.0f;
    texAniMapDelta[1] = 0.0f;
    smooth = 1;
    matUsage = 0;
    libFlag = 0;
    ignoreSun = 0;
    kambient = 0.0f;
    texture = NULL;
    bspSectorFront = NULL;
    bspSectorBack = NULL;
    detailObjectVisualName = NULL;
    detailTexture = NULL;
    m_enuWaveMode = 0;
    kdiffuse = 0.899999976f;
    smoothAngle = 60.0f;
    rndAlphaBlendFunc = 0;
    polyListTimeStamp = -1;
    detailTextureScale = 1.0f;
    m_bEnvironmentalMappingStrength = 1.0f;
    m_enuWaveSpeed = 2;
    m_fWaveMaxAmplitude = 30.0f;
    m_fWaveGridSize = 100.0f;
    color[0] = 150;
    color[1] = 150;
    color[2] = 150;
    color[3] = 255;
    matGroup = 0;
}

void zCMaterial::InitThisByMaterial(const zCMaterial& other)
{
    InitValues();
    if (texture != other.texture) {
        if (texture)
            texture->Release();
        texture = other.texture;
        if (texture) {
            texture->AddRef();
            AutoAssignDetailTexture();
        }
    }
    color[0] = other.color[0];
    color[1] = other.color[1];
    color[2] = other.color[2];
    color[3] = other.color[3];
    smooth = other.smooth;
    smoothAngle = other.smoothAngle;
    matGroup = other.matGroup;
    rndAlphaBlendFunc = other.rndAlphaBlendFunc;
    texAniMapDelta[0] = other.texAniMapDelta[0];
    texAniMapDelta[1] = other.texAniMapDelta[1];
    matUsage = other.matUsage;
    libFlag = other.libFlag;
    flags = other.flags;
    m_bEnvironmentalMappingStrength = other.m_bEnvironmentalMappingStrength;
    m_enuWaveMode = other.m_enuWaveMode;
    m_enuWaveSpeed = other.m_enuWaveSpeed;
    ignoreSun = other.ignoreSun;
    m_fWaveGridSize = other.m_fWaveGridSize;
    m_fWaveMaxAmplitude = other.m_fWaveMaxAmplitude;
}

void zCMaterial::SetDetailObjectVisual(const zSTRING& visualName)
{
    if (visualName.IsEmpty()) {
        delete detailObjectVisualName;
        detailObjectVisualName = NULL;
        if (detailTexture) {
            detailTexture->Release();
            detailTexture = NULL;
        }
        return;
    }
    if (detailObjectVisualName)
        *detailObjectVisualName = visualName;
    else
        detailObjectVisualName = new zSTRING(visualName);
}

const zSTRING& zCMaterial::GetDetailObjectVisual() const
{
    return detailObjectVisualName ? *detailObjectVisualName : zSTR_EMPTY;
}

void zCMaterial::ApplyTexAniMapping(zCPolygon* polygon)
{
    if (!(smooth & 4))
        return;
    float offsetX = ztimer.totalTimeFloat * texAniMapDelta[0];
    float offsetY = ztimer.totalTimeFloat * texAniMapDelta[1];
    offsetX -= floor(offsetX);
    offsetY -= floor(offsetY);
    for (int i = 0; i < polygon->numClipVert; ++i) {
        polygon->clipFeat[i]->mapping.n[0] += offsetX;
        polygon->clipFeat[i]->mapping.n[1] += offsetY;
    }
}

zVEC3 zCMaterial::GetTexAniVector(zCPolygon* polygon)
{
    if (!(smooth & 4) || polygon->polyNumVert < 3)
        return zVEC3(0.0f);
    const zVEC3 first = polygon->vertex[1]->position - polygon->vertex[0]->position;
    const zVEC3 second = polygon->vertex[2]->position - polygon->vertex[0]->position;
    return first * (-texAniMapDelta[0]) + second * (-texAniMapDelta[1]);
}

int zVEC3::IsEqualEps(const zVEC3& other) const
{
    return fabs(n[0] - other.n[0]) < 0.001f && fabs(n[1] - other.n[1]) < 0.001f
        && fabs(n[2] - other.n[2]) < 0.001f;
}

int zCMaterial::IsPortalMaterial() const
{
    const zSTRING& name = GetObjectName();
    if (name.Length() < 2 || name[0] != 'P')
        return 0;
    if (name[1] == ':')
        return 1;
    return name.Length() >= 3 && (name[1] == 'N' || name[1] == 'I') && name[2] == ':';
}

void zCMaterial::SetTexAniMappingDir(const zVEC2& direction)
{
    texAniMapDelta[0] = direction.n[0];
    texAniMapDelta[1] = direction.n[1];
}

void zCMaterial::SetTexture(const zSTRING& fileName)
{
    const zSTRING upperName = fileName.Upper();
    if (texture && texture->GetObjectName() == upperName)
        return;
    zCTexture* newTexture = zCTexture::Load(const_cast<zSTRING&>(upperName), 0);
    SetTexture(newTexture);
    if (newTexture)
        newTexture->Release();
}

void zCMaterial::SetTexture(zCTexture* newTexture)
{
    if (texture == newTexture)
        return;
    if (texture) {
        texture->Release();
        texture = NULL;
    }
    texture = newTexture;
    if (texture) {
        texture->AddRef();
        AutoAssignDetailTexture();
    }
}

void zCMaterial::SetDetailTexture(const zSTRING& fileName)
{
    if (fileName.IsEmpty())
        return;
    zCTexture* newTexture = zCTexture::Load(const_cast<zSTRING&>(fileName), 0);
    SetDetailTexture(newTexture);
    if (newTexture)
        newTexture->Release();
}

void zCMaterial::SetDetailTexture(zCTexture* newTexture)
{
    if (detailTexture == newTexture)
        return;
    if (detailTexture) {
        detailTexture->Release();
        detailTexture = NULL;
    }
    detailTexture = newTexture;
    if (detailTexture)
        detailTexture->AddRef();
}

void zCMaterial::AutoAssignDetailTexture()
{
    if (detailObjectVisualName) {
        SetDetailTexture(*detailObjectVisualName);
        detailTextureScale = detailTextureScale;
    }
}

zVEC2 zCMaterial::GetTexScale()
{
    if (texture && texScale[0] == 1.0f) {
        int width;
        int height;
        texture->GetPixelSize(width, height);
        texScale[0] = static_cast<float>(width);
        texScale[1] = static_cast<float>(height);
    }
    return zVEC2(texScale[0], texScale[1]);
}

void zCMaterial::SetTexScale(zVEC2 scale)
{
    texScale[0] = scale.n[0];
    texScale[1] = scale.n[1];
}

void zCMaterial::RemoveTexture()
{
    if (texture) {
        texture->Release();
        texture = NULL;
    }
}

const zSTRING& zCMaterial::GetMatGroupString(int group)
{
    return group >= 0 && group <= 6 ? materialGroupNames[group] : materialGroupNames[0];
}

const zSTRING& zCMaterial::GetMatGroupString() const
{
    return GetMatGroupString(matGroup);
}

void zCMaterial::SetMatGroupByString(const zSTRING& group)
{
    for (int i = 0; i <= 6; ++i) {
        if (group == GetMatGroupString(i)) {
            matGroup = i;
            return;
        }
    }
}

void zCMaterial::RefreshAvgColorFromTexture()
{
    if (!texture)
        return;
    const int cacheState = texture->cacheState & 3;
    const zCOLOR average = texture->GetTextureInfo().averageColor;
    color[0] = average.b;
    color[1] = average.g;
    color[2] = average.r;
    if (cacheState != 3)
        texture->CacheOut();
}

void zCMaterial::Archive(zCArchiver& archiver)
{
    RefreshAvgColorFromTexture();
    archiver.WriteString("name", GetObjectName());
    archiver.WriteEnum("matGroup", "UNDEF;METAL;STONE;WOOD;EARTH;WATER;SNOW", matGroup);
    archiver.WriteColor("color", *reinterpret_cast<zCOLOR*>(color));
    archiver.WriteFloat("smoothAngle", smoothAngle);
    archiver.WriteString("texture", texture ? texture->GetObjectName() : zSTR_EMPTY);
    GetTexScale();
    archiver.WriteRawFloat("texScale", texScale, sizeof(texScale));
    archiver.WriteFloat("aniFPS", texAniCtrl.aniFPS * 1000.0f);
    archiver.WriteEnum("texAniMap", "NONE;LINEAR", (smooth >> 2) & 1);
    archiver.WriteRawFloat("texAniMapDir", texAniMapDelta, sizeof(texAniMapDelta));
    archiver.WriteBool("lodDontCollapse", (smooth >> 4) & 1);
    archiver.WriteBool("dontUseLightmaps", (smooth >> 1) & 1);
    archiver.WriteBool("noCollDet", (smooth >> 3) & 1);
    archiver.WriteString("detailObject", GetDetailObjectVisual());
    archiver.WriteFloat("detailTextureScale", detailTextureScale);
    archiver.WriteBool("forceOccluder", (smooth >> 5) & 1);
    archiver.WriteBool("environmentalMapping", (smooth >> 6) & 1);
    archiver.WriteFloat("environmentalMappingStrength", m_bEnvironmentalMappingStrength);
    archiver.WriteEnum("waveMode",
        "NONE;AMBIENT_GROUND;GROUND;AMBIENT_WALL;WALL;ENV;AMBIENT_WIND;WIND", m_enuWaveMode);
    archiver.WriteEnum("waveSpeed", "NONE;SLOW;NORMAL;FAST", m_enuWaveSpeed);
    archiver.WriteFloat("waveMaxAmplitude", m_fWaveMaxAmplitude);
    archiver.WriteFloat("waveGridSize", m_fWaveGridSize);
    archiver.WriteBool("ignoreSun", ignoreSun);
    archiver.WriteEnum("alphaBlendFunc", "MAT_DEFAULT;NONE;BLEND;ADD;SUB;MUL;MUL2",
        rndAlphaBlendFunc);
    archiver.WriteRawFloat("defaultMapping", default_mapping, sizeof(default_mapping));
}

void zCMaterial::Unarchive(zCArchiver& archiver)
{
    SetObjectName(archiver.ReadString("name"));
    matGroup = archiver.ReadEnum("matGroup");
    *reinterpret_cast<zCOLOR*>(color) = archiver.ReadColor("color");
    smoothAngle = archiver.ReadFloat("smoothAngle");
    smooth = (smooth & ~1UL) | (smoothAngle != 0.0f);
    SetTexture(archiver.ReadString("texture").Upper());
    archiver.ReadRawFloat("texScale", texScale, sizeof(texScale));
    texAniCtrl.aniFPS = archiver.ReadFloat("aniFPS") * 0.001f;
    smooth = (smooth & ~4UL) | (archiver.ReadEnum("texAniMap") << 2);
    archiver.ReadRawFloat("texAniMapDir", texAniMapDelta, sizeof(texAniMapDelta));
    smooth = (smooth & ~0x10UL) | (archiver.ReadBool("lodDontCollapse") << 4);
    smooth = (smooth & ~2UL) | (archiver.ReadBool("dontUseLightmaps") << 1);
    smooth = (smooth & ~8UL) | (archiver.ReadBool("noCollDet") << 3);
    SetDetailObjectVisual(archiver.ReadString("detailObject"));
    detailTextureScale = archiver.ReadFloat("detailTextureScale");
    smooth = (smooth & ~0x20UL) | (archiver.ReadBool("forceOccluder") << 5);
    smooth = (smooth & ~0x40UL) | (archiver.ReadBool("environmentalMapping") << 6);
    m_bEnvironmentalMappingStrength = archiver.ReadFloat("environmentalMappingStrength");
    m_enuWaveMode = archiver.ReadEnum("waveMode");
    if (m_enuWaveMode > 7)
        m_enuWaveMode = 0;
    m_enuWaveSpeed = archiver.ReadEnum("waveSpeed");
    m_fWaveMaxAmplitude = archiver.ReadFloat("waveMaxAmplitude");
    m_fWaveGridSize = archiver.ReadFloat("waveGridSize");
    ignoreSun = archiver.ReadBool("ignoreSun");
    rndAlphaBlendFunc = archiver.ReadEnum("alphaBlendFunc");
    if (rndAlphaBlendFunc == 4)
        rndAlphaBlendFunc = 2;
    if (detailTextureScale == 0.0f)
        detailTextureScale = 1.0f;
    AutoAssignDetailTexture();
    archiver.ReadRawFloat("defaultMapping", default_mapping, sizeof(default_mapping));
    if (color[3] < 255 && rndAlphaBlendFunc == 1)
        rndAlphaBlendFunc = 2;
}

int zCTexAniCtrl::masterFrameCtr = 0;

zCTexAniCtrl::zCTexAniCtrl()
    : aniChannel(0), actFrame(0.0f), aniFPS(0.0f), frameCtr(0xFFFFFFFF), bOneShotAni(0)
{
}

void zCTexAniCtrl::AdvanceAni(zCTexture* texture)
{
    if (aniFPS == 0.0f || frameCtr == static_cast<unsigned long>(masterFrameCtr))
        return;
    frameCtr = masterFrameCtr;
    if (!(texture->cacheState & 3)) {
        texture->PrecacheTexAniFrames(0.5f);
        actFrame = 0.0f;
    }
    else if (texture->numAniFrames[aniChannel] > 0) {
        const int numFrames = texture->numAniFrames[aniChannel];
        actFrame += ztimer.frameTimeFloat * aniFPS;
        if (bOneShotAni) {
            if (static_cast<int>(actFrame) > numFrames - 1)
                actFrame = static_cast<float>(numFrames - 1);
        }
        else {
            while (static_cast<int>(actFrame) > numFrames - 1)
                actFrame -= numFrames;
        }
    }
    texture->actAniFrame[aniChannel] = static_cast<int>(floor(actFrame));
}

zCMaterial* zCMaterial::SearchName(zSTRING& name)
{
    return static_cast<zCMaterial*>(classDef->SearchHashTable(name.Upper()));
}
