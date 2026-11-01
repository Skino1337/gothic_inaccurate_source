#include "include/z3d.h"

#include <cmath>

static int renderLightState = -1;
static int dynVobLightState = -1;
zCMemPool<zCDynVobLightCacheElement> zCDynVobLightCacheElement::s_MemPool(0x2000, 0x2000);
static zCCache<zCVob*, zCDynVobLightCacheElement> dynVobLightCache(0x200, 0x258);
static zCArray<zCVobLight*> vobLightRefs;

int zCRenderLightContainer::CompareLightScore(const void* ele1, const void* ele2)
{
    const zTLightScore* first = static_cast<const zTLightScore*>(ele1);
    const zTLightScore* second = static_cast<const zTLightScore*>(ele2);
    return first->score < second->score ? -1 : 1;
}

void zCRenderLightContainer::CreatePrelightConfig(const zTBBox3D& bbox)
{
    Clear();
    zVEC3 center;
    for (int axis = 0; axis < 3; ++axis)
        center.n[axis] = (bbox.maxs.n[axis] + bbox.mins.n[axis]) * 0.5f;
    for (int i = 0; i < 2; ++i) {
        zCRenderLight& light = lightList[i];
        light.lightType = 3;
        light.colorDiffuse.n[0] = 255.0f;
        light.colorDiffuse.n[1] = 255.0f;
        light.colorDiffuse.n[2] = 255.0f;
        light.position = center;
        light.position.n[0] += 300.0f;
        light.position.n[2] += 300.0f;
        light.dir_approxFalloff = i == 0 ? 1.0f : 0.2f;
    }
    lightList[0].direction.n[0] = -2.0f / 3.0f;
    lightList[0].direction.n[1] = -1.0f / 3.0f;
    lightList[0].direction.n[2] = -2.0f / 3.0f;
    lightList[1].direction.n[0] = 0.1f;
    lightList[1].direction.n[1] = 0.1f;
    lightList[1].direction.n[2] = 1.0f;
    const float length = std::sqrt(1.02f);
    for (int axis = 0; axis < 3; ++axis)
        lightList[1].direction.n[axis] /= length;
    numLights = 2;
}

int zCRenderLightContainer::CollectLights_FastPath(const zTRenderContext& context)
{
    if (!doPrelight) {
        if (!(context.hintLightingFullbright & 1))
            return 0;
        numLights = 1;
        lightList[0].lightType = 3;
        lightList[0].colorDiffuse.n[0] = 255.0f;
        lightList[0].colorDiffuse.n[1] = 255.0f;
        lightList[0].colorDiffuse.n[2] = 255.0f;
        lightList[0].direction.n[0] = 0.45000002f;
        lightList[0].direction.n[1] = -0.865999997f;
        lightList[0].direction.n[2] = -0.218099996f;
        return 1;
    }

    numLights = 1;
    lightList[0].lightType = 4;
    zCOLOR color(255, 255, 255, 255);
    if (context.vob)
        color = context.vob->CalcLightSampleAtOrigin();
    for (int axis = 0; axis < 3; ++axis)
        lightList[0].colorDiffuse.n[axis] = color[axis] * 0.6f;
    return 1;
}

void zCRenderLightContainer::S_ReleaseVobLightRefs()
{
    vobLightRefs.DeleteList();
}

void zCRenderLightContainer::CollectLights_DynLights(const zTRenderContext& context)
{
    CollectLights_FastPath(context);
}

int zTBBox3D::IsIntersecting(const zTBSphere3D& sphere) const
{
    float distance2 = 0;
    for (int axis = 0; axis < 3; ++axis) {
        float delta = 0;
        if (sphere.center.n[axis] < mins.n[axis])
            delta = mins.n[axis] - sphere.center.n[axis];
        else if (sphere.center.n[axis] > maxs.n[axis])
            delta = sphere.center.n[axis] - maxs.n[axis];
        distance2 += delta * delta;
    }
    return distance2 <= sphere.radius * sphere.radius;
}

void zCDynVobLightCacheElement::ReleaseLights()
{
    for (int i = 0; i < 4; ++i) {
        if (lightList[i])
            lightList[i]->Release();
        lightList[i] = 0;
    }
}

void zCDynVobLightCacheElement::UpdateContents(const zCVob*)
{
    ReleaseLights();
}

void zCDynVobLightCacheElement::operator delete(void* memory)
{
    ::operator delete(memory);
}

void zCRenderLightContainer::CollectLights_StatLights(const zTRenderContext&)
{
    if (numLights >= 8)
        return;
    zCRenderLight& light = lightList[numLights++];
    light.lightType = 3;
    light.colorDiffuse.n[0] = 255.0f;
    light.colorDiffuse.n[1] = 255.0f;
    light.colorDiffuse.n[2] = 255.0f;
    light.direction.n[0] = -0.5f;
    light.direction.n[1] = -1.0f;
    light.direction.n[2] = -0.5f;
}

void zCRenderLightContainer::CollectLights(const zTRenderContext& context)
{
    numLights = 0;
    m_bSmoothPrelit = 0;
    const int fastPath = CollectLights_FastPath(context);
    if (fastPath && m_fPreLightDist > 0) {
        m_bSmoothPrelit = context.distVobToCam >= m_fPreLightDist &&
            context.distVobToCam < m_fPreLightDist + 300.0f;
    }
    if (!fastPath || m_bSmoothPrelit) {
        CollectLights_DynLights(context);
        CollectLights_StatLights(context);
    }
}

zVEC3 zCRenderLightContainer::CalcStatLightDir(const zTRenderContext&)
{
    zVEC3 direction;
    direction.n[0] = -0.5f;
    direction.n[1] = -1.0f;
    direction.n[2] = -0.5f;
    const float invLength = 1.0f / std::sqrt(1.5f);
    for (int axis = 0; axis < 3; ++axis)
        direction.n[axis] *= invLength;
    return direction;
}

zVEC3 zCRenderLightContainer::LightVertex(const zVEC3& position, const zVEC3& normal) const
{
    zVEC3 result = { 0, 0, 0 };
    for (int i = 0; i < numLights; ++i) {
        const zCRenderLight& light = lightList[i];
        float intensity = 0;
        if (light.lightType == 1) {
            zVEC3 delta;
            float distance2 = 0;
            for (int axis = 0; axis < 3; ++axis) {
                delta.n[axis] = position.n[axis] - light.positionLS.n[axis];
                distance2 += delta.n[axis] * delta.n[axis];
            }
            const float distance = std::sqrt(distance2);
            if (distance < light.range) {
                const float dot = delta.n[0] * normal.n[0] + delta.n[1] * normal.n[1] +
                    delta.n[2] * normal.n[2];
                intensity = -(dot / distance) * (1.0f - distance * light.rangeInv);
            }
        } else if (light.lightType == 3) {
            intensity = -(light.directionLS.n[0] * normal.n[0] +
                light.directionLS.n[1] * normal.n[1] + light.directionLS.n[2] * normal.n[2]);
            if (intensity < 0)
                intensity *= -0.2f;
            intensity = intensity * light.dir_approxFalloff + 0.23f;
        }
        for (int axis = 0; axis < 3; ++axis)
            result.n[axis] += light.colorDiffuse.n[axis] * intensity;
    }
    for (int axis = 0; axis < 3; ++axis) {
        if (result.n[axis] > 255.0f)
            result.n[axis] = 255.0f;
    }
    return result;
}

void zCRenderLightContainer::Activate(const zMAT4& trafo)
{
    for (int i = 0; i < numLights; ++i) {
        zCRenderLight& light = lightList[i];
        if (light.lightType == 1) {
            for (int row = 0; row < 3; ++row)
                light.positionLS.n[row] = trafo.m[row][0] * light.position.n[0] +
                    trafo.m[row][1] * light.position.n[1] +
                    trafo.m[row][2] * light.position.n[2] + trafo.m[row][3];
        } else if (light.lightType == 3) {
            for (int row = 0; row < 3; ++row)
                light.directionLS.n[row] = trafo.m[row][0] * light.direction.n[0] +
                    trafo.m[row][1] * light.direction.n[1] +
                    trafo.m[row][2] * light.direction.n[2];
        }
    }
}

void zCRenderLightContainer::SetupRendererLights()
{
    static int numRendererLights;
    if (numLights > numRendererLights)
        numRendererLights = numLights;
    numRendererLights = numLights;
}

zCRenderLightContainer::~zCRenderLightContainer()
{
    numLights = 0;
    doPrelight = 0;
    m_fPreLightDist = 0;
    m_bSmoothPrelit = 0;
}

zCRenderLight::zCRenderLight()
{
}
