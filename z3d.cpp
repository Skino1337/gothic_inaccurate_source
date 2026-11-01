#include "z3d.h"
#include "zViewBase.h"
#include "zRenderer.h"
#include "zSession.h"

#include <cstring>
#include <cmath>

static int lineCacheState = -1;
static int polygonState = -1;
static int polygonClipState = -1;
zCMemPool<zCVertex> zCVertex::s_MemPool;
zCMemPool<zCVertFeature> zCVertFeature::s_MemPool;
zCMemPool<zCPolygon> zCPolygon::s_MemPool;
zCMemPool<zCPortal> zCPortal::s_MemPool;
static zCMemPoolBase polygonSavedValuesPool(0x0C, 0x4000, 0x44000);
static zCMemPoolBase polygonClipPool(0x54, 0x1000, 0x1000);
static zCVertex polygonClipVertices[8192];
static zCVertFeature polygonClipFeatures[8192];
static zCVertFeature* polygonFeatureList[8192];

zCVertex* zCPolygon::s_clipVertScene = polygonClipVertices;
int zCPolygon::s_numClipVertScene;
zCVertFeature* zCPolygon::s_clipFeatScene = polygonClipFeatures;
int zCPolygon::s_numClipFeatScene;
int zCPolygon::s_numVertListScene;
int zCPolygon::s_numFeatListScene;
zCVertex** zCPolygon::s_actClipVert;
int zCPolygon::s_actNumClipVert;
zCVertFeature** zCPolygon::s_featListScene = polygonFeatureList;
zCArraySort<zSVertexSavedValues> zCPolygon::morphedVerticesList_Height;
zCArraySort<zSVertexSavedValues> zCPolygon::morphedVerticesList_Wall;
zCArraySort<zSFeatureSavedValues> zCPolygon::morphedFeatureList;
zCArray<zSPolySavedValues> zCPolygon::morphedPolysSavedValuesList;

void zCLightMap::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

void* zCPolygon::S_AllocPointers(int count)
{
    return new void*[count];
}

void zCPolygon::S_DeletePointers(int, void* pointers)
{
    delete[] static_cast<void**>(pointers);
}

zCVertFeature* zCPolygon::S_AllocVertFeatures(int count)
{
    zCVertFeature* features = new zCVertFeature[count];
    std::memset(features, 0, sizeof(zCVertFeature) * count);
    for (int i = 0; i < count; ++i) {
        features[i].color[0] = 255;
        features[i].color[3] = 255;
    }
    return features;
}

void zCPolygon::S_DeleteVertFeatures(int, zCVertFeature* features)
{
    delete[] features;
}

zCPolygon::zCPolygon()
    : vertex(0), lastTimeDrawn(-1), material(0), lightmap(0), clipVert(0), clipFeat(0),
      numClipVert(0), feature(0), polyNumVert(0)
{
    flags[0] = 0x10;
    flags[1] = 0xFF;
    flags[2] = 0xFF;
}

zCPolygon::~zCPolygon()
{
    if (material)
        material->Release();
    if (lightmap)
        lightmap->Release();
    delete[] vertex;
    if (feature) {
        delete[] *feature;
        delete[] feature;
    }
    vertex = 0;
    feature = 0;
    polyNumVert = 0;
}

int Compare_Verts(const void* ele1, const void* ele2)
{
    const zSVertexSavedValues* left = static_cast<const zSVertexSavedValues*>(ele1);
    const zSVertexSavedValues* right = static_cast<const zSVertexSavedValues*>(ele2);
    return right->vert > left->vert ? -1 : 0;
}

int Compare_Features(const void* ele1, const void* ele2)
{
    const zSFeatureSavedValues* left = static_cast<const zSFeatureSavedValues*>(ele1);
    const zSFeatureSavedValues* right = static_cast<const zSFeatureSavedValues*>(ele2);
    return right->feat > left->feat ? -1 : 0;
}

void zCPolygon::S_InitMorph()
{
    morphedVerticesList_Height.Compare = Compare_Verts;
    morphedVerticesList_Wall.Compare = Compare_Verts;
    morphedFeatureList.Compare = Compare_Features;
}

void zCPolygon::S_EmptyMorphedVerts()
{
    morphedVerticesList_Height.numInArray = 0;
    morphedVerticesList_Wall.numInArray = 0;
    morphedFeatureList.numInArray = 0;
    morphedPolysSavedValuesList.numInArray = 0;
}

void zCPolygon::S_ResetMorphedVerts()
{
    for (int i = 0; i < morphedVerticesList_Height.numInArray; ++i) {
        morphedVerticesList_Height.array[i].vert->position.n[1] =
            morphedVerticesList_Height.array[i].offset;
    }
    for (int i = 0; i < morphedVerticesList_Wall.numInArray; ++i) {
        morphedVerticesList_Wall.array[i].vert->position.n[0] -=
            morphedVerticesList_Wall.array[i].offset;
        morphedVerticesList_Wall.array[i].vert->position.n[2] -=
            morphedVerticesList_Wall.array[i].offset;
    }
    for (int i = 0; i < morphedFeatureList.numInArray; ++i)
        morphedFeatureList.array[i].feat->mapping = morphedFeatureList.array[i].mapping;
    for (int i = 0; i < morphedPolysSavedValuesList.numInArray; ++i)
        morphedPolysSavedValuesList.parray[i].poly->polyPlane =
            morphedPolysSavedValuesList.parray[i].plane;
    S_EmptyMorphedVerts();
}

void zCPolygon::S_DeleteMorphedVerts()
{
    S_ResetMorphedVerts();
    morphedVerticesList_Height.DeleteList();
    morphedVerticesList_Wall.DeleteList();
    morphedFeatureList.DeleteList();
    morphedPolysSavedValuesList.DeleteList();
}

static float PolygonPlaneDistance(const zCVertex* vertex, const zTPlane& plane)
{
    return vertex->position.n[0] * plane.normal.n[0] +
        vertex->position.n[1] * plane.normal.n[1] +
        vertex->position.n[2] * plane.normal.n[2] - plane.distance;
}

int zCPolygon::ClipToPlane(zCVertex** vertices, zCVertFeature** features, int numVerts,
    zCVertex** outVertices, zCVertFeature** outFeatures, int& outNumVerts,
    const zTPlane& plane)
{
    if (s_numClipVertScene + numVerts * 4 > 0x2000)
        return 0;

    outNumVerts = 0;
    zCVertex* previous = vertices[numVerts - 1];
    zCVertFeature* previousFeature = features[numVerts - 1];
    float previousDistance = PolygonPlaneDistance(previous, plane);
    for (int i = 0; i < numVerts; ++i) {
        zCVertex* current = vertices[i];
        zCVertFeature* currentFeature = features[i];
        const float currentDistance = PolygonPlaneDistance(current, plane);
        if (previousDistance >= 0) {
            outVertices[outNumVerts] = previous;
            outFeatures[outNumVerts] = previousFeature;
            ++outNumVerts;
        }

        if ((previousDistance >= 0) != (currentDistance >= 0)) {
            const float fraction = previousDistance / (previousDistance - currentDistance);
            const int vertexIndex = s_numClipVertScene++;
            zCVertex& clipped = s_clipVertScene[vertexIndex];
            for (int axis = 0; axis < 3; ++axis) {
                clipped.position.n[axis] = previous->position.n[axis] +
                    (current->position.n[axis] - previous->position.n[axis]) * fraction;
            }
            outVertices[outNumVerts] = &clipped;

            const int featureIndex = s_numClipFeatScene++;
            zCVertFeature& clippedFeature = s_clipFeatScene[featureIndex];
            clippedFeature = *previousFeature;
            for (int axis = 0; axis < 3; ++axis) {
                clippedFeature.mapping.n[axis] = previousFeature->mapping.n[axis] +
                    (currentFeature->mapping.n[axis] - previousFeature->mapping.n[axis]) * fraction;
                clippedFeature.vertNormal.n[axis] = previousFeature->vertNormal.n[axis] +
                    (currentFeature->vertNormal.n[axis] - previousFeature->vertNormal.n[axis]) * fraction;
            }
            outFeatures[outNumVerts] = &clippedFeature;
            ++outNumVerts;
        }
        previous = current;
        previousFeature = currentFeature;
        previousDistance = currentDistance;
    }
    return outNumVerts >= 3;
}

int zCPolygon::ClipToPlane_Occluder(zCVertex** vertices, int numVerts,
    zCVertex** outVertices, int& outNumVerts, const zTPlane& plane)
{
    if (s_numClipVertScene + numVerts * 4 > 0x2000)
        return 0;

    outNumVerts = 0;
    zCVertex* previous = vertices[numVerts - 1];
    float previousDistance = PolygonPlaneDistance(previous, plane);
    for (int i = 0; i < numVerts; ++i) {
        zCVertex* current = vertices[i];
        const float currentDistance = PolygonPlaneDistance(current, plane);
        if (previousDistance >= 0)
            outVertices[outNumVerts++] = previous;
        if ((previousDistance >= 0) != (currentDistance >= 0)) {
            const float fraction = previousDistance / (previousDistance - currentDistance);
            zCVertex& clipped = s_clipVertScene[s_numClipVertScene++];
            for (int axis = 0; axis < 3; ++axis) {
                clipped.position.n[axis] = previous->position.n[axis] +
                    (current->position.n[axis] - previous->position.n[axis]) * fraction;
            }
            outVertices[outNumVerts++] = &clipped;
        }
        previous = current;
        previousDistance = currentDistance;
    }
    return outNumVerts >= 3;
}

void zCPolygon::PrepareRendering()
{
    s_numClipVertScene = 0;
    s_numVertListScene = 0;
    s_numClipFeatScene = 0;
    s_numFeatListScene = 0;
}

void zCPolygon::Unclip()
{
    clipVert = vertex;
    clipFeat = feature;
    numClipVert = polyNumVert;
    if (!(material->smooth & 4))
        return;
    CopyClipFeaturesForTexAniMapping();
}

void zCPolygon::Unclip_Occluder()
{
    clipVert = vertex;
    clipFeat = feature;
    numClipVert = polyNumVert;
}

void zCPolygon::CopyClipFeaturesForTexAniMapping()
{
    if (!(material->smooth & 4))
        return;
    clipFeat = &s_featListScene[s_numFeatListScene];
    s_numFeatListScene += numClipVert;
    for (int i = 0; i < numClipVert; ++i) {
        zCVertFeature& clipped = s_clipFeatScene[s_numClipFeatScene++];
        clipped = *feature[i];
        clipFeat[i] = &clipped;
    }
}

zCCamera* zCCamera::activeCam;

int zCPolygon::ClipToFrustum(int planeMask)
{
    Unclip();
    if (!(planeMask & 0x0F) || !zCCamera::activeCam)
        return 1;

    zCVertex* verticesA[256];
    zCVertex* verticesB[256];
    zCVertFeature* featuresA[256];
    zCVertFeature* featuresB[256];
    zCVertex** inputVertices = clipVert;
    zCVertFeature** inputFeatures = clipFeat;
    int inputCount = numClipVert;
    for (int i = 0; i < 4; ++i) {
        if (!(planeMask & (1 << i)))
            continue;
        int outputCount = 0;
        if (!ClipToPlane(inputVertices, inputFeatures, inputCount, verticesA, featuresA,
            outputCount, zCCamera::activeCam->frustumplanes[i])) {
            numClipVert = 0;
            return 0;
        }
        inputVertices = verticesA;
        inputFeatures = featuresA;
        inputCount = outputCount;
        if (inputCount > 256)
            return 0;
    }

    numClipVert = inputCount;
    clipVert = inputVertices;
    clipFeat = inputFeatures;
    s_numVertListScene += numClipVert;
    s_numFeatListScene += numClipVert;
    return 1;
}

int zCPolygon::ClipToFrustum_Occluder(int planeMask)
{
    Unclip_Occluder();
    if (!(planeMask & 0x0F) || !zCCamera::activeCam)
        return 1;

    zCVertex* verticesA[256];
    zCVertex** inputVertices = clipVert;
    int inputCount = numClipVert;
    for (int i = 0; i < 4; ++i) {
        if (!(planeMask & (1 << i)))
            continue;
        int outputCount = 0;
        if (!ClipToPlane_Occluder(inputVertices, inputCount, verticesA, outputCount,
            zCCamera::activeCam->frustumplanes[i])) {
            numClipVert = 0;
            return 0;
        }
        inputVertices = verticesA;
        inputCount = outputCount;
    }
    numClipVert = inputCount;
    clipVert = inputVertices;
    s_numVertListScene += numClipVert;
    return 1;
}

int zCPolygon::ClassifyPlane(const zTPlane& plane)
{
    int front = 0;
    int back = 0;
    int coplanar = 0;
    for (int i = 0; i < polyNumVert; ++i) {
        const float distance = PolygonPlaneDistance(vertex[i], plane);
        if (distance >= 0.001f)
            ++front;
        else if (distance <= -0.001f)
            ++back;
        else {
            ++front;
            ++back;
            ++coplanar;
        }
    }
    if (coplanar == polyNumVert)
        return 2;
    if (front == polyNumVert)
        return 0;
    if (back == polyNumVert)
        return 1;
    return 3;
}

int zCPolygon::ClassifyPlaneBSPBuild(const zTPlane& plane)
{
    return ClassifyPlane(plane);
}

static void PolygonCalcNormal(zCPolygon* polygon, int& result)
{
    result = 0;
    polygon->polyPlane.normal.n[0] = 1.0f;
    polygon->polyPlane.normal.n[1] = 0;
    polygon->polyPlane.normal.n[2] = 0;
    polygon->polyPlane.distance = 0;
    for (int i = 2; i < polygon->polyNumVert; ++i) {
        zVEC3 first;
        zVEC3 second;
        for (int axis = 0; axis < 3; ++axis) {
            first.n[axis] = polygon->vertex[1]->position.n[axis] -
                polygon->vertex[0]->position.n[axis];
            second.n[axis] = polygon->vertex[i]->position.n[axis] -
                polygon->vertex[1]->position.n[axis];
        }
        zVEC3 normal;
        normal.n[0] = first.n[1] * second.n[2] - first.n[2] * second.n[1];
        normal.n[1] = first.n[2] * second.n[0] - first.n[0] * second.n[2];
        normal.n[2] = first.n[0] * second.n[1] - first.n[1] * second.n[0];
        const float length = std::sqrt(normal.n[0] * normal.n[0] +
            normal.n[1] * normal.n[1] + normal.n[2] * normal.n[2]);
        if (length == 0)
            continue;
        for (int axis = 0; axis < 3; ++axis)
            normal.n[axis] /= length;
        polygon->polyPlane.normal = normal;
        polygon->polyPlane.distance = PolygonPlaneDistance(polygon->vertex[1],
            polygon->polyPlane) + polygon->polyPlane.distance;
        result = 1;
        return;
    }
}

int zCPolygon::CalcNormal()
{
    int result;
    PolygonCalcNormal(this, result);
    return result;
}

zVEC4::zVEC4(const zVEC3& value, float w)
{
    n[0] = value.n[0];
    n[1] = value.n[1];
    n[2] = value.n[2];
    n[3] = w;
}

int zCPolygon::CalcNormalApprox()
{
    int result;
    PolygonCalcNormal(this, result);
    return result;
}

zVEC4 Alg_FindPlaneCoeffApprox(const zVEC3& first, const zVEC3& second, const zVEC3& third)
{
    zVEC3 firstEdge;
    zVEC3 secondEdge;
    for (int axis = 0; axis < 3; ++axis) {
        firstEdge.n[axis] = first.n[axis] - second.n[axis];
        secondEdge.n[axis] = third.n[axis] - second.n[axis];
    }
    zVEC3 normal;
    normal.n[0] = firstEdge.n[1] * secondEdge.n[2] - firstEdge.n[2] * secondEdge.n[1];
    normal.n[1] = firstEdge.n[2] * secondEdge.n[0] - firstEdge.n[0] * secondEdge.n[2];
    normal.n[2] = firstEdge.n[0] * secondEdge.n[1] - firstEdge.n[1] * secondEdge.n[0];
    const float length = std::sqrt(normal.n[0] * normal.n[0] +
        normal.n[1] * normal.n[1] + normal.n[2] * normal.n[2]);
    if (length > 0) {
        for (int axis = 0; axis < 3; ++axis)
            normal.n[axis] /= length;
    }
    const float distance = normal.n[0] * second.n[0] + normal.n[1] * second.n[1] +
        normal.n[2] * second.n[2];
    return zVEC4(normal, distance);
}

void zCPolygon::GetScreenBBox2D(zTBBox2D& bbox) const
{
    bbox.mins.n[0] = 3.40282347e+38f;
    bbox.mins.n[1] = 3.40282347e+38f;
    bbox.maxs.n[0] = -3.40282347e+38f;
    bbox.maxs.n[1] = -3.40282347e+38f;
    for (int i = 0; i < numClipVert; ++i) {
        for (int axis = 0; axis < 2; ++axis) {
            if (clipVert[i]->position.n[axis] < bbox.mins.n[axis])
                bbox.mins.n[axis] = clipVert[i]->position.n[axis];
            if (clipVert[i]->position.n[axis] > bbox.maxs.n[axis])
                bbox.maxs.n[axis] = clipVert[i]->position.n[axis];
        }
    }
}

void zCPolygon::DrawWire(unsigned long color) const
{
    for (int i = 0; i < numClipVert; ++i) {
        const zCVertexTransform& first = zCVertex::transformedVertexList[clipVert[i]->transformedIndex];
        const zCVertexTransform& second =
            zCVertex::transformedVertexList[clipVert[(i + 1) % numClipVert]->transformedIndex];
        zlineCache.Line(first.vertScrX, first.vertScrY, second.vertScrX, second.vertScrY, color);
    }
}

static zVEC3 PolygonLightStatColor(const zCVertFeature* feature)
{
    zVEC3 result;
    result.n[0] = feature->lightStat[2];
    result.n[1] = feature->lightStat[1];
    result.n[2] = feature->lightStat[0];
    return result;
}

void zCPolygon::LightPolyList(const zCArray<zCVobLight*>& lights, zCPolygon** polygons,
    int numPolygons, zMAT4* trafo)
{
    for (int i = 0; i < lights.numInArray; ++i) {
        const zCVobLight* light = lights.parray[i];
        zVEC3 lightPosition;
        if (trafo)
            lightPosition = trafo->Rotate(light->positionWorld) + trafo->GetTranslation();
        else
            lightPosition = light->positionWorld;

        for (int j = 0; j < numPolygons; ++j) {
            zCPolygon* polygon = polygons[j];
            for (int k = polygon->polyNumVert - 1; k >= 0; --k) {
                zCVertFeature* feature = polygon->feature[k];
                if (i == 0)
                    for (int channel = 0; channel < 4; ++channel)
                        feature->lightStat[channel] = feature->color[channel];

                zVEC3 delta;
                for (int axis = 0; axis < 3; ++axis)
                    delta.n[axis] = polygon->vertex[k]->position.n[axis] - lightPosition.n[axis];

                float distance;
                if (light->lightType == 2) {
                    distance = std::fabs(delta.n[0]) + std::fabs(delta.n[1]) +
                        std::fabs(delta.n[2]);
                } else {
                    float x = std::fabs(delta.n[0]);
                    float y = std::fabs(delta.n[1]);
                    float z = std::fabs(delta.n[2]);
                    if (x < y) {
                        const float temporary = x;
                        x = y;
                        y = temporary;
                    }
                    if (x < z) {
                        const float temporary = x;
                        x = z;
                        z = temporary;
                    }
                    distance = x - x * 0.0625f + (y + z) * 0.375f;
                }
                if (distance >= light->range)
                    continue;

                float attenuation = 1.0f - distance * light->rangeInv;
                if (light->lightType == 0) {
                    if (distance > 0) {
                        for (int axis = 0; axis < 3; ++axis)
                            delta.n[axis] /= distance;
                    }
                    const zVEC3& normal = (polygon->material->smooth & 1)
                        ? feature->vertNormal : polygon->polyPlane.normal;
                    const float dot = normal.n[0] * delta.n[0] + normal.n[1] * delta.n[1] +
                        normal.n[2] * delta.n[2];
                    attenuation = -dot * polygon->material->kdiffuse * attenuation +
                        polygon->material->kambient;
                    if (attenuation <= 0)
                        continue;
                }

                const int value = static_cast<int>(attenuation * 255.0f);
                for (int channel = 0; channel < 3; ++channel) {
                    int color = feature->lightStat[channel] +
                        ((value * light->color[channel]) >> 8);
                    if (color > 255)
                        color = 255;
                    feature->lightStat[channel] = color;
                }
            }
        }
    }
}

void zCPolygon::LightDynCamSpace(const zVEC3& lightPosition, int lightRange)
{
    if (lightRange < 50 || !material)
        return;
    for (int i = 0; i < numClipVert; ++i) {
        const float distance = 1.0f /
            zCVertex::transformedVertexList[clipVert[i]->transformedIndex].vertCamSpaceZInv;
        if (distance >= lightRange)
            continue;
        const zVEC3& normal = (material->smooth & 1) ? clipFeat[i]->vertNormal : polyPlane.normal;
        const float dot = normal.n[0] * (clipVert[i]->position.n[0] - lightPosition.n[0]) +
            normal.n[1] * (clipVert[i]->position.n[1] - lightPosition.n[1]) +
            normal.n[2] * (clipVert[i]->position.n[2] - lightPosition.n[2]);
        float light = (1.0f - distance / lightRange) * -dot / distance *
            material->kdiffuse + material->kambient;
        if (light < 0)
            light = 0;
        const int value = static_cast<int>(light * 255.0f);
        for (int channel = 0; channel < 3; ++channel) {
            int color = clipFeat[i]->color[channel] + value;
            if (color > 255)
                color = 255;
            clipFeat[i]->color[channel] = color;
        }
    }
}

zVEC3 zCPolygon::GetLightStatAtPos(const zVEC3& position)
{
    zVEC3 result;
    result.n[0] = 255.0f;
    result.n[1] = 0.0f;
    result.n[2] = 0.0f;

    if (!lightmap) {
        zVEC3 edges[3];
        zVEC3 weights[3];
        for (int axis = 0; axis < 3; ++axis)
            edges[2].n[axis] = position.n[axis] - vertex[0]->position.n[axis];

        for (int i = 2; i < polyNumVert; ++i) {
            for (int axis = 0; axis < 3; ++axis) {
                edges[0].n[axis] = vertex[i - 1]->position.n[axis] - vertex[0]->position.n[axis];
                edges[1].n[axis] = vertex[i]->position.n[axis] - vertex[0]->position.n[axis];
            }
            const float d00 = edges[0] * edges[0];
            const float d01 = edges[0] * edges[1];
            const float d11 = edges[1] * edges[1];
            const float d20 = edges[2] * edges[0];
            const float d21 = edges[2] * edges[1];
            const float denominator = d00 * d11 - d01 * d01;
            if (denominator == 0)
                continue;
            const float v = (d11 * d20 - d01 * d21) / denominator;
            const float w = (d00 * d21 - d01 * d20) / denominator;
            if (v >= -0.05f && v <= 1.05f && w >= -0.05f && w <= 1.05f && v + w <= 1.05f) {
                const float u = 1.0f - v - w;
                zVEC3 firstColor = PolygonLightStatColor(feature[0]);
                zVEC3 secondColor = PolygonLightStatColor(feature[i - 1]);
                zVEC3 thirdColor = PolygonLightStatColor(feature[i]);
                for (int axis = 0; axis < 3; ++axis)
                    result.n[axis] = firstColor.n[axis] * u + secondColor.n[axis] * v +
                        thirdColor.n[axis] * w;
                return result;
            }
        }

        zVEC3 firstColor = PolygonLightStatColor(feature[0]);
        zVEC3 secondColor = PolygonLightStatColor(feature[1]);
        zVEC3 thirdColor = PolygonLightStatColor(feature[2]);
        for (int axis = 0; axis < 3; ++axis)
            result.n[axis] = (firstColor.n[axis] + secondColor.n[axis] + thirdColor.n[axis]) / 3.0f;
        return result;
    }

    int bestIndex = 0;
    float bestDistance = 3.40282347e+38f;
    for (int i = 0; i < polyNumVert; ++i) {
        zVEC3 delta;
        for (int axis = 0; axis < 3; ++axis)
            delta.n[axis] = vertex[i]->position.n[axis] - position.n[axis];
        const float distance = delta * delta;
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = i;
        }
    }
    return PolygonLightStatColor(feature[bestIndex]);
}

static int PolygonRayTriangle(const zCPolygon* polygon, const zVEC3& origin,
    const zVEC3& direction, int firstIndex, int secondIndex, zVEC3& intersection,
    float& distance, int twoSided)
{
    zVEC3 first;
    zVEC3 second;
    for (int axis = 0; axis < 3; ++axis) {
        first.n[axis] = polygon->vertex[1]->position.n[axis] -
            polygon->vertex[0]->position.n[axis];
        second.n[axis] = polygon->vertex[secondIndex]->position.n[axis] -
            polygon->vertex[0]->position.n[axis];
    }
    zVEC3 pvec;
    pvec.n[0] = direction.n[1] * second.n[2] - direction.n[2] * second.n[1];
    pvec.n[1] = direction.n[2] * second.n[0] - direction.n[0] * second.n[2];
    pvec.n[2] = direction.n[0] * second.n[1] - direction.n[1] * second.n[0];
    const float determinant = first.n[0] * pvec.n[0] + first.n[1] * pvec.n[1] +
        first.n[2] * pvec.n[2];
    if ((!twoSided && determinant >= 0) || (twoSided && determinant > -0.000001f &&
        determinant < 0.000001f))
        return 0;
    const float inverse = 1.0f / determinant;
    zVEC3 tvec;
    for (int axis = 0; axis < 3; ++axis)
        tvec.n[axis] = origin.n[axis] - polygon->vertex[0]->position.n[axis];
    const float u = (tvec.n[0] * pvec.n[0] + tvec.n[1] * pvec.n[1] +
        tvec.n[2] * pvec.n[2]) * inverse;
    if (u < 0 || u > 1)
        return 0;
    zVEC3 qvec;
    qvec.n[0] = tvec.n[1] * first.n[2] - tvec.n[2] * first.n[1];
    qvec.n[1] = tvec.n[2] * first.n[0] - tvec.n[0] * first.n[2];
    qvec.n[2] = tvec.n[0] * first.n[1] - tvec.n[1] * first.n[0];
    const float v = (direction.n[0] * qvec.n[0] + direction.n[1] * qvec.n[1] +
        direction.n[2] * qvec.n[2]) * inverse;
    if (v < 0 || u + v > 1)
        return 0;
    distance = (second.n[0] * qvec.n[0] + second.n[1] * qvec.n[1] +
        second.n[2] * qvec.n[2]) * inverse;
    if (distance < 0 || distance > 1)
        return 0;
    for (int axis = 0; axis < 3; ++axis)
        intersection.n[axis] = origin.n[axis] + direction.n[axis] * distance;
    return 1;
}

int zCPolygon::CheckRayPolyIntersection(const zVEC3& origin, const zVEC3& direction,
    zVEC3& intersection, float& distance) const
{
    for (int i = 2; i < polyNumVert; ++i) {
        if (PolygonRayTriangle(this, origin, direction, 1, i, intersection, distance, 0))
            return 1;
    }
    return 0;
}

int zCPolygon::CheckRayPolyIntersection2Sided(const zVEC3& origin, const zVEC3& direction,
    zVEC3& intersection, float& distance) const
{
    for (int i = 2; i < polyNumVert; ++i) {
        if (PolygonRayTriangle(this, origin, direction, 1, i, intersection, distance, 1))
            return 1;
    }
    return 0;
}

zTBBox3D zCPolygon::GetBBox3D() const
{
    zTBBox3D result;
    if (polyNumVert <= 0) {
        result.mins.n[0] = 0;
        result.mins.n[1] = 0;
        result.mins.n[2] = 0;
        result.maxs = result.mins;
        return result;
    }
    result.mins = vertex[0]->position;
    result.maxs = vertex[0]->position;
    for (int i = 1; i < polyNumVert; ++i) {
        for (int axis = 0; axis < 3; ++axis) {
            if (vertex[i]->position.n[axis] < result.mins.n[axis])
                result.mins.n[axis] = vertex[i]->position.n[axis];
            if (vertex[i]->position.n[axis] > result.maxs.n[axis])
                result.maxs.n[axis] = vertex[i]->position.n[axis];
        }
    }
    return result;
}

zVEC3 zCPolygon::GetCenter()
{
    zVEC3 result;
    result.n[0] = 0;
    result.n[1] = 0;
    result.n[2] = 0;
    for (int i = 0; i < polyNumVert; ++i) {
        for (int axis = 0; axis < 3; ++axis)
            result.n[axis] += vertex[i]->position.n[axis];
    }
    if (polyNumVert > 0) {
        for (int axis = 0; axis < 3; ++axis)
            result.n[axis] /= polyNumVert;
    }
    return result;
}

void zCPolygon::SetMapping(const zVEC2& first, const zVEC2& second, const zVEC2& third)
{
    for (int i = 0; i < 3; ++i) {
        const zVEC2* value = i == 0 ? &first : i == 1 ? &second : &third;
        feature[i]->mapping.n[0] = value->n[0];
        feature[i]->mapping.n[1] = value->n[1];
        feature[i]->mapping.n[2] = 0;
    }
}

void zCPolygon::SetMapping(int index, const zVEC2& value)
{
    feature[index]->mapping.n[0] = value.n[0];
    feature[index]->mapping.n[1] = value.n[1];
    feature[index]->mapping.n[2] = 0;
}

void zCPolygon::Flip()
{
    for (int i = 0; i < polyNumVert / 2; ++i) {
        zCVertex* temporary = vertex[i];
        vertex[i] = vertex[polyNumVert - i - 1];
        vertex[polyNumVert - i - 1] = temporary;
    }
    CalcNormal();
}

int zCPolygon::VertPartOfPoly(zCVertex* value) const
{
    for (int i = 0; i < polyNumVert; ++i) {
        if (vertex[i] == value)
            return 1;
    }
    return 0;
}

int zCPolygon::EdgePartOfPoly(zCVertex* first, zCVertex* second) const
{
    for (int i = 0; i < polyNumVert; ++i) {
        const int previous = (i + polyNumVert - 1) % polyNumVert;
        if (vertex[previous] == first && vertex[i] == second)
            return 1;
    }
    return 0;
}

int zCPolygon::EdgePositionsPartOfPoly(zCVertex* first, zCVertex* second) const
{
    return VertPartOfPoly(first) && VertPartOfPoly(second);
}

void zCPolygon::TexTranslate(const zVEC2& offset)
{
    for (int i = 0; i < polyNumVert; ++i) {
        feature[i]->mapping.n[0] += offset.n[0];
        feature[i]->mapping.n[1] += offset.n[1];
    }
}

void zCPolygon::TexSetTranslation(const zVEC2& offset)
{
    for (int i = 0; i < polyNumVert; ++i) {
        feature[i]->mapping.n[0] = offset.n[0];
        feature[i]->mapping.n[1] = offset.n[1];
    }
}

void zCPolygon::TexScale(float u, float v)
{
    for (int i = 0; i < polyNumVert; ++i) {
        feature[i]->mapping.n[0] *= u;
        feature[i]->mapping.n[1] *= v;
    }
}

void zCPolygon::TexScale(zVEC2& center, float u, float v)
{
    for (int i = 0; i < polyNumVert; ++i) {
        feature[i]->mapping.n[0] = (feature[i]->mapping.n[0] - center.n[0]) * u + center.n[0];
        feature[i]->mapping.n[1] = (feature[i]->mapping.n[1] - center.n[1]) * v + center.n[1];
    }
}

void zCPolygon::TexRotate(zVEC2& center, float angle)
{
    const float radians = angle * 0.0174532924f;
    const float sine = std::sin(radians);
    const float cosine = std::cos(radians);
    for (int i = 0; i < polyNumVert; ++i) {
        const float u = feature[i]->mapping.n[0] - center.n[0];
        const float v = feature[i]->mapping.n[1] - center.n[1];
        feature[i]->mapping.n[0] = u * cosine - v * sine + center.n[0];
        feature[i]->mapping.n[1] = u * sine + v * cosine + center.n[1];
    }
}

void zCPolygon::TexMirrorU(float value)
{
    for (int i = 0; i < polyNumVert; ++i)
        feature[i]->mapping.n[0] = value - feature[i]->mapping.n[0];
}

void zCPolygon::TexMirrorV(float value)
{
    for (int i = 0; i < polyNumVert; ++i)
        feature[i]->mapping.n[1] = value - feature[i]->mapping.n[1];
}

void zCPolygon::TexShearU(float value)
{
    for (int i = 0; i < polyNumVert; ++i)
        feature[i]->mapping.n[0] += feature[i]->mapping.n[1] * value;
}

void zCPolygon::TexShearV(float value)
{
    for (int i = 0; i < polyNumVert; ++i)
        feature[i]->mapping.n[1] += feature[i]->mapping.n[0] * value;
}

void zCPolygon::TexApplyPlanarMapping(const zVEC3& origin, const zVEC3& right,
    const zVEC3& up, int useTexScale)
{
    zVEC3 scaledRight = right;
    zVEC3 scaledUp = up;
    if (useTexScale && material) {
        const zVEC2 scale = material->GetTexScale();
        for (int axis = 0; axis < 3; ++axis) {
            scaledRight.n[axis] *= scale.n[0];
            scaledUp.n[axis] *= scale.n[1];
        }
    }

    const float rightUp = scaledRight * scaledUp;
    const float rightLength = scaledRight * scaledRight;
    const float upLength = scaledUp * scaledUp;
    const float denominator = upLength * rightLength - rightUp * rightUp;
    const float rightLengthInv = 1.0f / rightLength;
    for (int i = 0; i < polyNumVert; ++i) {
        zVEC3 relative;
        for (int axis = 0; axis < 3; ++axis)
            relative.n[axis] = vertex[i]->position.n[axis] - origin.n[axis];
        const float projectedRight = relative * scaledRight;
        const float projectedUp = relative * scaledUp;
        const float u = (projectedUp * rightLength - projectedRight * rightUp) / denominator;
        feature[i]->mapping.n[0] = u;
        feature[i]->mapping.n[1] = (projectedRight - u * rightUp) * rightLengthInv;
        feature[i]->mapping.n[2] = 0;
    }
}

static void PolygonPlaneBasis(const zTPlane& plane, zVEC3& right, zVEC3& up)
{
    right.n[0] = 0;
    right.n[1] = 0;
    right.n[2] = 0;
    up.n[0] = 0;
    up.n[1] = 0;
    up.n[2] = 0;
    if (plane.normal.n[1] * plane.normal.n[1] < 0.9f) {
        right.n[1] = 1.0f;
        up.n[0] = plane.normal.n[2];
        up.n[2] = -plane.normal.n[0];
    }
    else {
        right.n[0] = 1.0f;
        up.n[1] = -plane.normal.n[2];
        up.n[2] = plane.normal.n[1];
    }
}

void zCPolygon::TexApplyPlanarMapping(float scaleU, float scaleV, int useTexScale)
{
    zVEC3 right;
    zVEC3 up;
    PolygonPlaneBasis(polyPlane, right, up);
    for (int axis = 0; axis < 3; ++axis) {
        right.n[axis] *= scaleU;
        up.n[axis] *= scaleV;
    }
    TexApplyPlanarMapping(vertex[0]->position, right, up, useTexScale);
}

void zCPolygon::TexApplyPlanarMapping(const zCList<zCPolygon>& polygons, float scaleU,
    float scaleV, int useTexScale)
{
    zVEC3 normal;
    zVEC3 origin;
    normal.n[0] = 0;
    normal.n[1] = 0;
    normal.n[2] = 0;
    origin = normal;
    int count = 0;
    for (const zCList<zCPolygon>* entry = polygons.next; entry; entry = entry->next) {
        for (int axis = 0; axis < 3; ++axis) {
            normal.n[axis] += entry->data->polyPlane.normal.n[axis];
            origin.n[axis] += entry->data->vertex[0]->position.n[axis];
        }
        ++count;
    }
    if (!count)
        return;
    for (int axis = 0; axis < 3; ++axis) {
        normal.n[axis] /= count;
        origin.n[axis] /= count;
    }
    const float length = std::sqrt(normal.n[0] * normal.n[0] +
        normal.n[1] * normal.n[1] + normal.n[2] * normal.n[2]);
    if (length > 0) {
        for (int axis = 0; axis < 3; ++axis)
            normal.n[axis] /= length;
    }
    zTPlane plane;
    plane.normal = normal;
    plane.distance = 0;
    zVEC3 right;
    zVEC3 up;
    PolygonPlaneBasis(plane, right, up);
    for (int axis = 0; axis < 3; ++axis) {
        right.n[axis] *= scaleU;
        up.n[axis] *= scaleV;
    }
    for (const zCList<zCPolygon>* entry = polygons.next; entry; entry = entry->next)
        entry->data->TexApplyPlanarMapping(origin, right, up, useTexScale);
}

void zCPolygon::AlignPolyListToAvgPlane(zCArray<zCPolygon*> polygons)
{
    if (polygons.numInArray <= 1) {
        delete[] polygons.parray;
        return;
    }

    zVEC3 normal;
    zVEC3 origin;
    normal.n[0] = 0;
    normal.n[1] = 0;
    normal.n[2] = 0;
    origin = normal;
    int vertexCount = 0;
    for (int i = 0; i < polygons.numInArray; ++i) {
        zCPolygon* polygon = polygons.parray[i];
        for (int axis = 0; axis < 3; ++axis)
            normal.n[axis] += polygon->polyPlane.normal.n[axis];
        for (int j = 0; j < polygon->polyNumVert; ++j) {
            for (int axis = 0; axis < 3; ++axis)
                origin.n[axis] += polygon->vertex[j]->position.n[axis];
            ++vertexCount;
        }
    }
    if (!vertexCount)
        return;
    for (int axis = 0; axis < 3; ++axis)
        origin.n[axis] /= vertexCount;
    const float length = std::sqrt(normal.n[0] * normal.n[0] +
        normal.n[1] * normal.n[1] + normal.n[2] * normal.n[2]);
    if (length > 0) {
        for (int axis = 0; axis < 3; ++axis)
            normal.n[axis] /= length;
    }
    for (int axis = 0; axis < 3; ++axis) {
        const float value = normal.n[axis] >= 0 ? 1.0f : -1.0f;
        if (normal.n[axis] > 0.99f || normal.n[axis] < -0.99f) {
            normal.n[0] = 0;
            normal.n[1] = 0;
            normal.n[2] = 0;
            normal.n[axis] = value;
            break;
        }
    }
    for (int i = 0; i < polygons.numInArray; ++i) {
        zCPolygon* polygon = polygons.parray[i];
        for (int j = 0; j < polygon->polyNumVert; ++j) {
            zVEC3& position = polygon->vertex[j]->position;
            const float distance = normal.n[0] * (position.n[0] - origin.n[0]) +
                normal.n[1] * (position.n[1] - origin.n[1]) +
                normal.n[2] * (position.n[2] - origin.n[2]);
            for (int axis = 0; axis < 3; ++axis)
                position.n[axis] -= normal.n[axis] * distance;
        }
        polygon->CalcNormal();
    }
    delete[] polygons.parray;
}

void zCPolygon::TexCorrectUV()
{
    float minU = 99999.0f;
    float minV = 99999.0f;
    float maxU = -99999.0f;
    float maxV = -99999.0f;
    for (int i = 0; i < polyNumVert; ++i) {
        if (feature[i]->mapping.n[0] < minU)
            minU = feature[i]->mapping.n[0];
        if (feature[i]->mapping.n[0] > maxU)
            maxU = feature[i]->mapping.n[0];
        if (feature[i]->mapping.n[1] < minV)
            minV = feature[i]->mapping.n[1];
        if (feature[i]->mapping.n[1] > maxV)
            maxV = feature[i]->mapping.n[1];
    }
    const float offsetU = std::floor(-(minU + maxU) * 0.5f);
    const float offsetV = std::floor(-(minV + maxV) * 0.5f);
    for (int i = 0; i < polyNumVert; ++i) {
        feature[i]->mapping.n[0] += offsetU;
        feature[i]->mapping.n[1] += offsetV;
    }
}

void zCPolygon::AllocVerts(int count)
{
    const int oldCount = polyNumVert;
    const int newCount = oldCount + count;
    zCVertFeature* newFeatures = S_AllocVertFeatures(newCount);
    zCVertFeature** newFeaturePointers = new zCVertFeature*[newCount];
    zCVertex** newVertices = new zCVertex*[newCount];
    for (int i = 0; i < oldCount; ++i) {
        newFeatures[i] = *feature[i];
        newVertices[i] = vertex[i];
    }
    for (int i = 0; i < newCount; ++i)
        newFeaturePointers[i] = &newFeatures[i];
    if (oldCount) {
        delete[] *feature;
        delete[] feature;
        delete[] vertex;
    }
    feature = newFeaturePointers;
    vertex = newVertices;
    polyNumVert = newCount;
}

void zCPolygon::AddVertex(zCVertex* value)
{
    AllocVerts(1);
    vertex[polyNumVert - 1] = value;
}

void zCPolygon::AddVertexAndFeature(zCVertex* vertexValue, zCVertFeature* featureValue)
{
    AllocVerts(1);
    vertex[polyNumVert - 1] = vertexValue;
    *feature[polyNumVert - 1] = *featureValue;
}

void zCPolygon::SetVertices(zCVertex* first, zCVertex* second, zCVertex* third)
{
    if (polyNumVert < 3)
        AllocVerts(3 - polyNumVert);
    vertex[polyNumVert - 3] = first;
    vertex[polyNumVert - 2] = second;
    vertex[polyNumVert - 1] = third;
    CalcNormal();
}

void zCPolygon::SetVertices(zCVertex* first, zCVertex* second, zCVertex* third,
    zCVertex* fourth, int calcNormal)
{
    if (polyNumVert < 4)
        AllocVerts(4 - polyNumVert);
    vertex[polyNumVert - 4] = first;
    vertex[polyNumVert - 3] = second;
    vertex[polyNumVert - 2] = third;
    vertex[polyNumVert - 1] = fourth;
    if (calcNormal)
        CalcNormal();
}

void zCPolygon::SetVertex(int index, zCVertex* value)
{
    if (polyNumVert <= index)
        AllocVerts(index - polyNumVert + 1);
    vertex[index] = value;
}

void zCPolygon::SetFeature(int index, zCVertFeature* value)
{
    if (polyNumVert <= index)
        AllocVerts(index - polyNumVert + 1);
    *feature[index] = *value;
}

void zCPolygon::CopyValuesInto(zCPolygon* destination) const
{
    destination->polyPlane = polyPlane;
    for (int i = 0; i < 3; ++i)
        destination->flags[i] = flags[i];
    if (material) {
        material->AddRef();
        destination->material = material;
    }
    zCVertFeature* features = S_AllocVertFeatures(polyNumVert);
    destination->feature = new zCVertFeature*[polyNumVert];
    for (int i = 0; i < polyNumVert; ++i) {
        destination->feature[i] = &features[i];
        *destination->feature[i] = *feature[i];
    }
}

void zCPolygon::CopyPolyInto(zCPolygon* destination) const
{
    destination->polyPlane = polyPlane;
    destination->material = material;
    destination->lightmap = lightmap;
    destination->lastTimeDrawn = lastTimeDrawn;
    for (int i = 0; i < 3; ++i)
        destination->flags[i] = flags[i];
    destination->vertex = 0;
    destination->feature = 0;
    destination->polyNumVert = 0;
    destination->AllocVerts(polyNumVert);
    CopyValuesInto(destination);
    for (int i = 0; i < polyNumVert; ++i)
        destination->vertex[i] = vertex[i];
}

void zCPolygon::RemoveVerticesAndFeatures()
{
    delete[] vertex;
    if (feature) {
        delete[] *feature;
        delete[] feature;
    }
    vertex = 0;
    feature = 0;
    polyNumVert = 0;
}

float zCPolygon::GetArea() const
{
    zVEC3 sum;
    sum.n[0] = 0;
    sum.n[1] = 0;
    sum.n[2] = 0;
    for (int i = 0; i < polyNumVert; ++i) {
        const zCVertex& first = *vertex[i];
        const zCVertex& second = *vertex[(i + 1) % polyNumVert];
        sum.n[0] += first.position.n[1] * second.position.n[2] -
            second.position.n[1] * first.position.n[2];
        sum.n[1] += second.position.n[0] * first.position.n[2] -
            second.position.n[2] * first.position.n[0];
        sum.n[2] += second.position.n[1] * first.position.n[0] -
            second.position.n[0] * first.position.n[1];
    }
    return std::fabs(sum.n[0] * polyPlane.normal.n[0] + sum.n[1] * polyPlane.normal.n[1] +
        sum.n[2] * polyPlane.normal.n[2]) * 0.5f;
}

float zCPolygon::GetLongestEdgeLen() const
{
    float result = 0;
    for (int i = 0; i < polyNumVert; ++i) {
        const zCVertex& first = *vertex[i];
        const zCVertex& second = *vertex[(i + 1) % polyNumVert];
        const float dx = first.position.n[0] - second.position.n[0];
        const float dy = first.position.n[1] - second.position.n[1];
        const float dz = first.position.n[2] - second.position.n[2];
        const float length = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (length > result)
            result = length;
    }
    return result;
}

void zCPolygon::ResetStaticLight()
{
    if (!feature)
        return;
    for (int i = 0; i < polyNumVert; ++i) {
        feature[i]->lightStat[0] = 0;
        feature[i]->lightStat[1] = 0;
        feature[i]->lightStat[2] = 0;
        feature[i]->lightStat[3] = 255;
    }
}

void zCPolygon::SetStaticLight(unsigned long color)
{
    if (!feature)
        return;
    for (int i = 0; i < polyNumVert; ++i)
        for (int channel = 0; channel < 4; ++channel)
            feature[i]->lightStat[channel] = (color >> (channel * 8)) & 0xFF;
}

void zCPolygon::SetLightDyn(unsigned long color)
{
    if (!feature)
        return;
    for (int i = 0; i < polyNumVert; ++i)
        for (int channel = 0; channel < 4; ++channel)
            feature[i]->lightDyn[channel] = (color >> (channel * 8)) & 0xFF;
}

void zCPolygon::SetLightStatAlpha(unsigned char alpha)
{
    if (!feature)
        return;
    for (int i = 0; i < polyNumVert; ++i)
        feature[i]->lightStat[3] = alpha;
}

void zCPolygon::ResetLightDynToLightStat()
{
    if (!feature)
        return;
    for (int i = 0; i < polyNumVert; ++i)
        for (int channel = 0; channel < 4; ++channel)
            feature[i]->lightDyn[channel] = feature[i]->lightStat[channel];
}

void zCPolygon::AddToLightDyn(unsigned long color)
{
    if (!feature)
        return;
    for (int i = 0; i < polyNumVert; ++i) {
        for (int channel = 0; channel < 3; ++channel) {
            int value = feature[i]->lightDyn[channel] + ((color >> (channel * 8)) & 0xFF);
            if (value > 255)
                value = 255;
            feature[i]->lightDyn[channel] = value;
        }
    }
}

int zCPolygon::CheckBBoxPolyIntersection(const zTBBox3D& bbox) const
{
    int inside = 0;
    for (int i = 0; i < polyNumVert; ++i) {
        if (vertex[i]->position.n[0] >= bbox.mins.n[0] && vertex[i]->position.n[0] <= bbox.maxs.n[0] &&
            vertex[i]->position.n[1] >= bbox.mins.n[1] && vertex[i]->position.n[1] <= bbox.maxs.n[1] &&
            vertex[i]->position.n[2] >= bbox.mins.n[2] && vertex[i]->position.n[2] <= bbox.maxs.n[2]) {
            inside = 1;
            break;
        }
    }
    if (inside)
        return 1;
    const zTBBox3D polygonBox = GetBBox3D();
    return polygonBox.mins.n[0] <= bbox.maxs.n[0] && polygonBox.maxs.n[0] >= bbox.mins.n[0] &&
        polygonBox.mins.n[1] <= bbox.maxs.n[1] && polygonBox.maxs.n[1] >= bbox.mins.n[1] &&
        polygonBox.mins.n[2] <= bbox.maxs.n[2] && polygonBox.maxs.n[2] >= bbox.mins.n[2];
}

int zCPolygon::CheckBSpherePolyIntersection(const zTBSphere3D& sphere) const
{
    if (sphere.ClassifyPlane(polyPlane) != 2)
        return 0;

    zVEC3 direction;
    for (int axis = 0; axis < 3; ++axis)
        direction.n[axis] = -polyPlane.normal.n[axis] * sphere.radius * 2.0f;
    zVEC3 intersection;
    float rayDistance;
    if (CheckRayPolyIntersection(sphere.center, direction, intersection, rayDistance))
        return 1;

    for (int i = 0; i < polyNumVert; ++i) {
        if (sphere.IsIntersecting(vertex[i]->position, vertex[(i + 1) % polyNumVert]->position))
            return 1;
    }
    return 0;
}

void zCPolygon::SplitPoly(const zTPlane& plane, zCPolygon* front, zCPolygon* back, zCMesh* mesh)
{
    zCVertex* previous = vertex[polyNumVert - 1];
    zCVertFeature* previousFeature = feature[polyNumVert - 1];
    float previousDistance = PolygonPlaneDistance(previous, plane);
    for (int i = 0; i < polyNumVert; ++i) {
        zCVertex* current = vertex[i];
        zCVertFeature* currentFeature = feature[i];
        const float currentDistance = PolygonPlaneDistance(current, plane);
        zCPolygon* currentTarget = currentDistance >= 0 ? front : back;
        currentTarget->AddVertexAndFeature(current, currentFeature);

        if ((previousDistance >= 0) != (currentDistance >= 0)) {
            const float fraction = previousDistance / (previousDistance - currentDistance);
            zCVertex* clipped = new zCVertex;
            zCVertFeature clippedFeature = *previousFeature;
            for (int axis = 0; axis < 3; ++axis) {
                clipped->position.n[axis] = previous->position.n[axis] +
                    (current->position.n[axis] - previous->position.n[axis]) * fraction;
                clippedFeature.mapping.n[axis] = previousFeature->mapping.n[axis] +
                    (currentFeature->mapping.n[axis] - previousFeature->mapping.n[axis]) * fraction;
                clippedFeature.vertNormal.n[axis] = previousFeature->vertNormal.n[axis] +
                    (currentFeature->vertNormal.n[axis] - previousFeature->vertNormal.n[axis]) * fraction;
            }
            front->AddVertexAndFeature(clipped, &clippedFeature);
            back->AddVertexAndFeature(clipped, &clippedFeature);
            if (mesh)
                mesh->AddVertex(clipped->position);
        }
        previous = current;
        previousFeature = currentFeature;
        previousDistance = currentDistance;
    }

    front->polyPlane = polyPlane;
    back->polyPlane = polyPlane;
    if (material) {
        material->AddRef();
        front->material = material;
        material->AddRef();
        back->material = material;
    }
    for (int i = 0; i < 3; ++i) {
        front->flags[i] = flags[i];
        back->flags[i] = flags[i];
    }
}

int zCPolygon::IsIntersectingProjection(zCPolygon* other, const zVEC3&)
{
    return IsIntersecting(other);
}

int zCPolygon::IsIntersecting(zCPolygon* other)
{
    for (int polygonIndex = 0; polygonIndex < 2; ++polygonIndex) {
        zCPolygon* source = polygonIndex == 0 ? this : other;
        zCPolygon* target = polygonIndex == 0 ? other : this;
        for (int i = 0; i < source->polyNumVert; ++i) {
            zVEC3 direction;
            const zCVertex& first = *source->vertex[i];
            const zCVertex& second = *source->vertex[(i + 1) % source->polyNumVert];
            for (int axis = 0; axis < 3; ++axis)
                direction.n[axis] = second.position.n[axis] - first.position.n[axis];
            zVEC3 intersection;
            float distance;
            if (target->CheckRayPolyIntersection2Sided(first.position, direction,
                intersection, distance))
                return 1;
        }
    }
    return 0;
}

int zCPolygon::IsNeighbourOf(const zCPolygon* other)
{
    for (int i = 0; i < other->polyNumVert; ++i) {
        const int next = (i + 1) % other->polyNumVert;
        if (EdgePartOfPoly(other->vertex[i], other->vertex[next]))
            return 1;
    }
    return 0;
}

int zCPolygon::GetStaticLODPossible()
{
    if (!material || !material->texture)
        return 0;
    const zSTRING name = material->texture->GetObjectName();
    static const char* names[] = {
        "FLOOR", "BODEN", "GROUND", "PATH", "GRASS", "ACKER", "MOUNTAIN", "SURFACE"
    };
    for (int i = 0; i < 8; ++i) {
        if (name.Search(names[i]) >= 0)
            return 1;
    }
    return 0;
}

zCVertexTransform* zCVertex::transformedVertexList;

zCVertex::zCVertex()
    : transformedIndex(0), myIndex(0)
{
}

zCVertex::~zCVertex()
{
    ResetVertexTransform();
}

void zCVertex::ResetVertexTransform()
{
    if (transformedIndex)
        transformedVertexList[transformedIndex].untransformed = 0;
    transformedIndex = 0;
}

zCVertFeature::zCVertFeature()
{
    lightStat[0] = 0;
    lightStat[1] = 0;
    lightStat[2] = 0;
    lightStat[3] = 255;
    lightDyn[0] = 0;
    lightDyn[1] = 0;
    lightDyn[2] = 0;
    lightDyn[3] = 255;
}

void zTEngineStats::PrintScreen(int x, int y)
{
    if (x == -1)
        x = 6600;
    if (y == -1)
        y = 1000;

    screen->Print(x, y, "ai       : " + zSTRING(numVobAI));
    screen->Print(x, y + 200, "ai_only  : " + zSTRING(numVobAI_only));
    screen->Print(x, y + 400, "ai_full  : " + zSTRING(numVobAI_full));
    screen->Print(x, y + 600, "aiEndMove: " + zSTRING(numVob_EndMovement));
    screen->Print(x, y + 800, "mdBoxTree: " + zSTRING(numModelUpdateBBoxTree));
    screen->Print(x, y + 1000, "cdIntrp  : " + zSTRING(numVobFirstTestInterpen));
    screen->Print(x, y + 1200, "clPolys  : " + zSTRING(numPolysClipped));
    screen->Print(x, y + 1400, "cdPlanes : " + zSTRING(numPlanesClipped));
    screen->Print(x, y + 1600, "traceRay : " + zSTRING(numTraceRay));
    std::memset(this, 0, sizeof(zTEngineStats));
}

zCLineCache zlineCache;

zCLineCache::~zCLineCache()
{
    ::operator delete[](cache);
}

void zCLineCache::Line(int x1, int y1, int x2, int y2, zCOLOR color)
{
    if (nextFree >= numEntries) {
        if (numEntries) {
            nextFree = 0;
            return;
        }
        numEntries = 4096;
        cache = static_cast<zTCacheEntry*>(::operator new[](numEntries * sizeof(zTCacheEntry)));
    }

    cache[nextFree].x1 = x1;
    cache[nextFree].y1 = y1;
    cache[nextFree].x2 = x2;
    cache[nextFree].y2 = y2;
    cache[nextFree].color = color;
    nextFree++;
}

void zCLineCache::Flush(zCViewBase* view)
{
    for (int i = 0; i < nextFree; i++) {
        zTCacheEntry& entry = cache[i];
        view->Line(view->anx(entry.x1), view->any(entry.y1),
            view->anx(entry.x2), view->any(entry.y2), entry.color);
    }
    nextFree = 0;
}

static int ClipLineAtNear(zVEC3& start, zVEC3& end)
{
    const float nearZ = 0.00001f;
    const int startOutside = start.n[2] < nearZ;
    const int endOutside = end.n[2] < nearZ;
    if (startOutside && endOutside)
        return 0;
    if (!startOutside && !endOutside)
        return 1;

    zVEC3 intersection = start + (end - start) * ((nearZ - start.n[2]) / (end.n[2] - start.n[2]));
    intersection.n[2] = nearZ;
    if (startOutside)
        start = intersection;
    else
        end = intersection;
    return 1;
}

void zCLineCache::Line3D(const zVEC3& start, const zVEC3& end, zCOLOR color, int mode)
{
    if (!zCCamera::activeCam || !zCCamera::activeCam->connectedVob)
        return;

    zVEC3 first = zCCamera::activeCam->camMatrix * start;
    zVEC3 second = zCCamera::activeCam->camMatrix * end;
    if (!ClipLineAtNear(first, second))
        return;

    int x1 = int(zCCamera::activeCam->vpData.xcenter
        + first.n[0] * zCCamera::activeCam->viewDistanceX / first.n[2]);
    int y1 = int(zCCamera::activeCam->vpData.ycenter
        - first.n[1] * zCCamera::activeCam->viewDistanceY / first.n[2]);
    int x2 = int(zCCamera::activeCam->vpData.xcenter
        + second.n[0] * zCCamera::activeCam->viewDistanceX / second.n[2]);
    int y2 = int(zCCamera::activeCam->vpData.ycenter
        - second.n[1] * zCCamera::activeCam->viewDistanceY / second.n[2]);

    if (!mode) {
        Line(x1, y1, x2, y2, color);
        return;
    }
    if (screen->ClipLine(x1, y1, x2, y2))
        zrenderer->DrawLineZ(x1, y1, 1.0f / first.n[2],
            x2, y2, 1.0f / second.n[2], color);
}

void zCLineCache::LineCS3D(const zVEC3& start, const zVEC3& end, zCOLOR color)
{
    if (!zCCamera::activeCam)
        return;

    zCCamera::activeCam->Activate();
    zVEC3 first = start;
    zVEC3 second = end;
    if (!ClipLineAtNear(first, second))
        return;

    int x1 = int(zCCamera::activeCam->vpData.xcenter
        + first.n[0] * zCCamera::activeCam->viewDistanceX / first.n[2]);
    int y1 = int(zCCamera::activeCam->vpData.ycenter
        - first.n[1] * zCCamera::activeCam->viewDistanceY / first.n[2]);
    int x2 = int(zCCamera::activeCam->vpData.xcenter
        + second.n[0] * zCCamera::activeCam->viewDistanceX / second.n[2]);
    int y2 = int(zCCamera::activeCam->vpData.ycenter
        - second.n[1] * zCCamera::activeCam->viewDistanceY / second.n[2]);

    if (screen->ClipLine(x1, y1, x2, y2))
        zrenderer->DrawLineZ(x1, y1, 1.0f / first.n[2],
            x2, y2, 1.0f / second.n[2], color);
}

zCVolatileMemory<zCVertexTransform> zCVertexTransform::s_MemMan(0x4E20, zVOLATILE_OVERFLOW_RESIZE);

zCVertexTransform::~zCVertexTransform()
{
    if (untransformed)
        untransformed->transformedIndex = 0;
    untransformed = NULL;
}

void zCVertex::Draw(const zCOLOR& color, int mode)
{
    zVEC3 first = position;
    zVEC3 second = position;
    first.n[1] -= 5.0f;
    second.n[1] += 5.0f;
    zlineCache.Line3D(first, second, color, mode);

    first = position;
    second = position;
    first.n[0] -= 5.0f;
    second.n[0] += 5.0f;
    zlineCache.Line3D(first, second, color, mode);
}

void zCVertex::ResetVertexTransforms()
{
    zCVertexTransform::s_MemMan.Restore(0);
    transformedVertexList = static_cast<zCVertexTransform*>(zCVertexTransform::s_MemMan.Alloc());
    if (transformedVertexList)
        transformedVertexList->untransformed = NULL;
}

zCVertexTransform* zCVertex::CreateVertexTransform()
{
    if (!transformedIndex) {
        zCVertexTransform* transform = static_cast<zCVertexTransform*>(
            zCVertexTransform::s_MemMan.Alloc());
        if (!transform)
            return NULL;
        transform->untransformed = this;
        transformedIndex = zCVertexTransform::s_MemMan.Index(transform) + 1;
        transformedVertexList = static_cast<zCVertexTransform*>(zCVertexTransform::s_MemMan.data);
        return transform;
    }

    return &transformedVertexList[transformedIndex];
}
