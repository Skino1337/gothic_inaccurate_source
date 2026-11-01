#include "include/zVob.h"
#include "include/zSky.h"
#include <cmath>

void zCPolygon::LightStatic(zCVobLight& light, const zVEC3& lightPosition, zMAT4&, zCWorld*)
{
    if (!feature)
        return;
    const float diffuse = material ? material->kdiffuse : 0.8f;
    const int smooth = material && (material->smooth & 1);
    for (int i = 0; i < polyNumVert; ++i) {
        zVEC3 direction;
        float distance2 = 0.0f;
        for (int axis = 0; axis < 3; ++axis) {
            direction.n[axis] = lightPosition.n[axis] - vertex[i]->position.n[axis];
            distance2 += direction.n[axis] * direction.n[axis];
        }
        const float distance = std::sqrt(distance2);
        if (distance >= light.lightData.range || distance <= 0.0f)
            continue;
        for (int axis = 0; axis < 3; ++axis)
            direction.n[axis] /= distance;
        const zVEC3& normal = smooth ? feature[i]->vertNormal : polyPlane.normal;
        float intensity = 0.0f;
        for (int axis = 0; axis < 3; ++axis)
            intensity -= direction.n[axis] * normal.n[axis];
        intensity *= (1.0f - distance * light.lightData.rangeInv) * diffuse;
        if (intensity <= 0.0f)
            continue;
        if (intensity > 1.0f)
            intensity = 1.0f;
        const unsigned char* lightColor = &light.lightData.lightColor.b;
        for (int channel = 0; channel < 3; ++channel) {
            int color = feature[i]->lightStat[channel] +
                (lightColor[channel] * static_cast<int>(intensity * 255.0f) >> 8);
            if (color > 255)
                color = 255;
            feature[i]->lightStat[channel] = static_cast<unsigned char>(color);
        }
    }
}

void zCPolygon::SmoothSectorBorder(zCWorld* world)
{
    if (!(flags[0] & 8) || !feature || !material->bspSectorFront || !world->bspTree.mesh)
        return;
    zCMesh* mesh = world->bspTree.mesh;
    for (int i = 0; i < polyNumVert; ++i) {
        int found = 0;
        for (int j = 0; j < mesh->numPoly && !found; ++j) {
            zCPolygon* other = mesh->polyList[j];
            if ((other->flags[0] & 0x0c) || !other->feature)
                continue;
            for (int k = 0; k < other->polyNumVert; ++k) {
                float distance2 = 0.0f;
                for (int axis = 0; axis < 3; ++axis) {
                    const float delta = vertex[i]->position.n[axis] - other->vertex[k]->position.n[axis];
                    distance2 += delta * delta;
                }
                if (distance2 < 0.0001f) {
                    for (int channel = 0; channel < 3; ++channel)
                        feature[i]->lightStat[channel] = other->feature[k]->lightStat[channel];
                    feature[i]->lightStat[3] = 99;
                    found = 1;
                    break;
                }
            }
        }
    }
}

void zCMesh::LightMesh(zCVobLight& light, zMAT4& trafo, zCWorld* world)
{
    const zMAT4 worldToLocal = trafo.InverseLinTrafo();
    zVEC3 worldPosition;
    for (int axis = 0; axis < 3; ++axis)
        worldPosition.n[axis] = light.trafoObjToWorld.m[axis][3];
    const zVEC3 lightPosition = worldToLocal * worldPosition;
    for (int i = 0; i < numVert; ++i)
        vertList[i]->transformedIndex = 0;
    for (int i = 0; i < numPoly; ++i)
        polyList[i]->LightStatic(light, lightPosition, trafo, world);
}

void zCWorld::LightWorldStaticUncompiled(zCTree<zCVob>* node)
{
    if (!node)
        return;
    zCVob* vob = node->data;
    if (vob && (vob->showVisual & 1) && vob->type == 3) {
        zCMesh* mesh = dynamic_cast<zCMesh*>(vob->visual);
        if (mesh) {
            mesh->ResetStaticLight();
            mesh->CalcVertexNormals();
            zCArray<zCVob*> lights;
            SearchVobListByBaseClass(&zCVobLight::classDef, lights, &globalVobTree);
            for (int i = 0; i < lights.numInArray; ++i) {
                zCVobLight* light = static_cast<zCVobLight*>(lights.parray[i]);
                if (light->lightData.isStatic)
                    mesh->LightMesh(*light, vob->trafoObjToWorld, this);
            }
        }
    }
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        LightWorldStaticUncompiled(child);
}

void zCWorld::LightWorldStaticCompiled()
{
    zCVobLight* sunlight = 0;
    if (bspTree.bspTreeMode) {
        sunlight = new zCVobLight;
        sunlight->lightData.isStatic = 1;
        sunlight->lightData.lightType = 2;
        sunlight->SetRange(100.0f, 1);
        sunlight->lightData.lightColor.b = 255;
        sunlight->lightData.lightColor.g = 255;
        sunlight->lightData.lightColor.r = 255;
        zVEC3 direction;
        direction.n[0] = 0.4495f;
        direction.n[1] = -0.8660f;
        direction.n[2] = -0.2181f;
        sunlight->SetHeadingAtWorld(direction);
        AddVob(sunlight);
    }
    zCMesh* mesh = bspTree.mesh;
    if (mesh) {
        mesh->ResetStaticLight();
        mesh->CalcVertexNormals();
        zCArray<zCVob*> lights;
        SearchVobListByBaseClass(&zCVobLight::classDef, lights, &globalVobTree);
        zMAT4 identity = {};
        for (int axis = 0; axis < 4; ++axis)
            identity.m[axis][axis] = 1.0f;
        for (int i = 0; i < lights.numInArray; ++i) {
            zCVobLight* light = static_cast<zCVobLight*>(lights.parray[i]);
            if (light->lightData.isStatic)
                mesh->LightMesh(*light, identity, this);
        }
        for (int i = 0; i < mesh->numPoly; ++i)
            mesh->polyList[i]->SmoothSectorBorder(this);
    }
    if (sunlight) {
        RemoveVob(sunlight);
        sunlight->Release();
    }
}

void zCOLOR::SetRGB(unsigned char red, unsigned char green, unsigned char blue)
{
    r = red;
    g = green;
    b = blue;
}

void zCWorld::GenerateStaticVertexLighting()
{
    zerr->Message(zSTRING("D: WORLD: Calculating static Vertex Light for world ..."));
    bspTree.mesh->CreateListsFromArrays();
    if (compiled)
        LightWorldStaticCompiled();
    else
        LightWorldStaticUncompiled(&globalVobTree);
    zerr->Message(zSTRING("D: WORLD: ... Finished."));
}

static zCPolygon* lightingLastPoly;

int zCWorld::LightingTestRay(const zVEC3& start, const zVEC3& end, zVEC3& intersection,
    zCPolygon*& foundPoly)
{
    zVEC3 direction;
    for (int axis = 0; axis < 3; ++axis)
        direction.n[axis] = end.n[axis] - start.n[axis];
    if (lightingLastPoly) {
        float distance;
        if (lightingLastPoly->CheckRayPolyIntersection(start, direction, intersection, distance)) {
            foundPoly = lightingLastPoly;
            return 1;
        }
        lightingLastPoly = 0;
    }
    if (!TraceRayFirstHit(start, direction, static_cast<const zCArray<zCVob*>*>(0), 0x120))
        return 0;
    intersection = traceRayReport.foundIntersection;
    foundPoly = traceRayReport.foundPoly;
    lightingLastPoly = foundPoly;
    return 1;
}

int zCWorld::GetSurfaceLightmapBBox2D(zCArray<zCPolygon*>& surface, zTBBox2D& bbox,
    int* const size)
{
    zVEC3 origin;
    zVEC3 up;
    zVEC3 right;
    surface.parray[0]->polyPlane.GetOriginUpRight(origin, up, right);
    for (int axis = 0; axis < 2; ++axis) {
        bbox.mins.n[axis] = 3.40282347e+38f;
        bbox.maxs.n[axis] = -3.40282347e+38f;
    }
    for (int i = 0; i < surface.numInArray; ++i) {
        zCPolygon* polygon = surface.parray[i];
        for (int j = 0; j < polygon->polyNumVert; ++j) {
            zVEC3 delta;
            float planeDistance = 0.0f;
            for (int axis = 0; axis < 3; ++axis) {
                delta.n[axis] = polygon->vertex[j]->position.n[axis] - origin.n[axis];
                planeDistance += delta.n[axis] * surface.parray[0]->polyPlane.normal.n[axis];
            }
            for (int axis = 0; axis < 3; ++axis)
                delta.n[axis] -= planeDistance * surface.parray[0]->polyPlane.normal.n[axis];
            float coordinates[2] = {};
            for (int axis = 0; axis < 3; ++axis) {
                coordinates[0] += delta.n[axis] * right.n[axis];
                coordinates[1] += delta.n[axis] * up.n[axis];
            }
            for (int axis = 0; axis < 2; ++axis) {
                if (coordinates[axis] < bbox.mins.n[axis])
                    bbox.mins.n[axis] = coordinates[axis];
                if (coordinates[axis] > bbox.maxs.n[axis])
                    bbox.maxs.n[axis] = coordinates[axis];
            }
        }
    }
    const float pixelSize = 1.0f;
    for (int axis = 0; axis < 2; ++axis) {
        bbox.mins.n[axis] = std::floor(bbox.mins.n[axis] / pixelSize) * pixelSize;
        bbox.maxs.n[axis] = std::ceil(bbox.maxs.n[axis] / pixelSize) * pixelSize;
        int dimension = static_cast<int>((bbox.maxs.n[axis] - bbox.mins.n[axis]) / pixelSize);
        if (dimension < 1)
            dimension = 1;
        int powerOfTwo = 1;
        while (powerOfTwo < dimension)
            powerOfTwo <<= 1;
        size[axis] = powerOfTwo;
        bbox.maxs.n[axis] = bbox.mins.n[axis] + powerOfTwo * pixelSize;
    }
    int valid = size[0] <= 256 && size[1] <= 256;
    for (int axis = 0; axis < 2; ++axis) {
        if (size[axis] < 0)
            size[axis] = 0;
        else if (size[axis] > 256)
            size[axis] = 256;
    }
    if (size[0] > size[1] * 8)
        size[0] = size[1] * 8;
    else if (size[1] > size[0] * 8)
        size[1] = size[0] * 8;
    for (int axis = 0; axis < 2; ++axis)
        bbox.maxs.n[axis] = bbox.mins.n[axis] + size[axis] * pixelSize;
    return valid;
}

zVEC3 zCWorld::GetPhongNormal(zCPolygon* polygon, const zVEC3& position)
{
    if (!polygon->feature || polygon->polyNumVert < 3)
        return polygon->polyPlane.normal;
    for (int i = 1; i < polygon->polyNumVert - 1; ++i) {
        zVEC3 first;
        zVEC3 second;
        zVEC3 point;
        for (int axis = 0; axis < 3; ++axis) {
            first.n[axis] = polygon->vertex[i]->position.n[axis] - polygon->vertex[0]->position.n[axis];
            second.n[axis] = polygon->vertex[i + 1]->position.n[axis] - polygon->vertex[0]->position.n[axis];
            point.n[axis] = position.n[axis] - polygon->vertex[0]->position.n[axis];
        }
        float d00 = 0.0f;
        float d01 = 0.0f;
        float d11 = 0.0f;
        float d20 = 0.0f;
        float d21 = 0.0f;
        for (int axis = 0; axis < 3; ++axis) {
            d00 += first.n[axis] * first.n[axis];
            d01 += first.n[axis] * second.n[axis];
            d11 += second.n[axis] * second.n[axis];
            d20 += point.n[axis] * first.n[axis];
            d21 += point.n[axis] * second.n[axis];
        }
        const float denominator = d00 * d11 - d01 * d01;
        if (denominator == 0.0f)
            continue;
        const float secondWeight = (d11 * d20 - d01 * d21) / denominator;
        const float thirdWeight = (d00 * d21 - d01 * d20) / denominator;
        const float firstWeight = 1.0f - secondWeight - thirdWeight;
        if (firstWeight < -0.001f || secondWeight < -0.001f || thirdWeight < -0.001f)
            continue;
        zVEC3 normal;
        float length2 = 0.0f;
        for (int axis = 0; axis < 3; ++axis) {
            normal.n[axis] = polygon->feature[0]->vertNormal.n[axis] * firstWeight +
                polygon->feature[i]->vertNormal.n[axis] * secondWeight +
                polygon->feature[i + 1]->vertNormal.n[axis] * thirdWeight;
            length2 += normal.n[axis] * normal.n[axis];
        }
        if (length2 > 0.0f) {
            const float invLength = 1.0f / std::sqrt(length2);
            for (int axis = 0; axis < 3; ++axis)
                normal.n[axis] *= invLength;
        }
        return normal;
    }
    return polygon->polyPlane.normal;
}

zCPatch::zCPatch()
    : area(0.0f), xpos(0), ypos(0)
{
    for (int axis = 0; axis < 3; ++axis) {
        center.n[axis] = 0.0f;
        centerLight.n[axis] = 0.0f;
        normal.n[axis] = 0.0f;
        radiosity.n[axis] = 0.0f;
        radToShoot.n[axis] = 0.0f;
        reflectivity.n[axis] = 0.0f;
    }
}

int zCPatchMap::CheckRaySurfaceIntersection(zVEC3& start, zVEC3& direction,
    zVEC3& intersection, zCPolygon*& foundPoly)
{
    float distance;
    if (lastRayHitPoly &&
        lastRayHitPoly->CheckRayPolyIntersection(start, direction, intersection, distance)) {
        foundPoly = lastRayHitPoly;
        return 1;
    }
    for (int i = 0; i < surface.numInArray; ++i) {
        if (surface.parray[i]->CheckRayPolyIntersection(start, direction, intersection, distance)) {
            foundPoly = surface.parray[i];
            lastRayHitPoly = foundPoly;
            return 1;
        }
    }
    foundPoly = 0;
    return 0;
}

zCPatchMap* zCWorld::GeneratePatchMapFromSurface(zCArray<zCPolygon*>& surface)
{
    zTBBox2D bbox;
    int size[2];
    GetSurfaceLightmapBBox2D(surface, bbox, size);
    zCPatchMap* patchMap = new zCPatchMap;
    patchMap->hit = 0;
    patchMap->xdim = static_cast<short>(size[0]);
    patchMap->ydim = static_cast<short>(size[1]);
    patchMap->lastRayHitPoly = 0;
    for (int i = 0; i < surface.numInArray; ++i)
        patchMap->surface.InsertEnd(surface.parray[i]);
    zVEC3 origin;
    zVEC3 up;
    zVEC3 right;
    surface.parray[0]->polyPlane.GetOriginUpRight(origin, up, right);
    patchMap->lightmapUp = up;
    patchMap->lightmapRight = right;
    const float pixelSize = 1.0f;
    for (int axis = 0; axis < 3; ++axis) {
        patchMap->lightmapOrigin.n[axis] = origin.n[axis] + right.n[axis] * bbox.mins.n[0] +
            up.n[axis] * bbox.mins.n[1];
        patchMap->bbox3D.mins.n[axis] = 3.40282347e+38f;
        patchMap->bbox3D.maxs.n[axis] = -3.40282347e+38f;
    }
    for (int y = 0; y < size[1]; ++y) {
        for (int x = 0; x < size[0]; ++x) {
            zCPatch* patch = new zCPatch;
            patch->xpos = static_cast<short>(x);
            patch->ypos = static_cast<short>(y);
            patch->area = pixelSize * pixelSize;
            for (int axis = 0; axis < 3; ++axis) {
                patch->center.n[axis] = patchMap->lightmapOrigin.n[axis] +
                    right.n[axis] * (x + 0.5f) * pixelSize + up.n[axis] * (y + 0.5f) * pixelSize;
                patch->centerLight.n[axis] = patch->center.n[axis];
                if (patch->center.n[axis] < patchMap->bbox3D.mins.n[axis])
                    patchMap->bbox3D.mins.n[axis] = patch->center.n[axis];
                if (patch->center.n[axis] > patchMap->bbox3D.maxs.n[axis])
                    patchMap->bbox3D.maxs.n[axis] = patch->center.n[axis];
            }
            patch->normal = GetPhongNormal(surface.parray[0], patch->center);
            if (surface.parray[0]->material) {
                patch->reflectivity.n[0] = surface.parray[0]->material->color[2] / 255.0f;
                patch->reflectivity.n[1] = surface.parray[0]->material->color[1] / 255.0f;
                patch->reflectivity.n[2] = surface.parray[0]->material->color[0] / 255.0f;
            }
            patchMap->patchList.InsertEnd(patch);
        }
    }
    return patchMap;
}

zCLightMap::zCLightMap()
    : tex(0), colorBuffer(0), xdim(0), ydim(0)
{
}

void zCLightMap::SetTexture(zCTexture* texture)
{
    tex = texture;
}

void zCWorld::GenerateLightmapFromPatchMap(zCPatchMap* patchMap)
{
    if (!patchMap->hit)
        return;
    const int numPixels = patchMap->xdim * patchMap->ydim;
    zVEC3* pixels = new zVEC3[numPixels];
    zVEC3* smoothed = new zVEC3[numPixels];
    for (int i = 0; i < numPixels; ++i) {
        pixels[i].n[0] = -1.0f;
        pixels[i].n[1] = 0.0f;
        pixels[i].n[2] = 0.0f;
    }
    for (int i = 0; i < patchMap->patchList.numInArray; ++i) {
        zCPatch* patch = patchMap->patchList.parray[i];
        float maximum = patch->radiosity.n[0];
        for (int axis = 1; axis < 3; ++axis) {
            if (patch->radiosity.n[axis] > maximum)
                maximum = patch->radiosity.n[axis];
        }
        if (maximum > 255.0f) {
            for (int axis = 0; axis < 3; ++axis)
                patch->radiosity.n[axis] *= 255.0f / maximum;
        }
        pixels[patch->ypos * patchMap->xdim + patch->xpos] = patch->radiosity;
    }
    for (int y = 0; y < patchMap->ydim; ++y) {
        for (int x = 0; x < patchMap->xdim; ++x) {
            float sum[3] = {};
            int count = 0;
            for (int offsetY = -1; offsetY <= 1; ++offsetY) {
                for (int offsetX = -1; offsetX <= 1; ++offsetX) {
                    const int sampleX = x + offsetX;
                    const int sampleY = y + offsetY;
                    if (sampleX < 0 || sampleY < 0 || sampleX >= patchMap->xdim ||
                        sampleY >= patchMap->ydim)
                        continue;
                    const zVEC3& sample = pixels[sampleY * patchMap->xdim + sampleX];
                    if (sample.n[0] < 0.0f)
                        continue;
                    for (int axis = 0; axis < 3; ++axis)
                        sum[axis] += sample.n[axis];
                    ++count;
                }
            }
            for (int axis = 0; axis < 3; ++axis)
                smoothed[y * patchMap->xdim + x].n[axis] = count ? sum[axis] / count : 0.0f;
        }
    }
    delete[] pixels;
    zCLightMap* lightMap = new zCLightMap;
    lightMap->lightmapOrigin = patchMap->lightmapOrigin;
    lightMap->lightmapUVUp = patchMap->lightmapUp;
    lightMap->lightmapUVRight = patchMap->lightmapRight;
    lightMap->colorBuffer = smoothed;
    lightMap->xdim = patchMap->xdim;
    lightMap->ydim = patchMap->ydim;
    for (int i = 0; i < patchMap->surface.numInArray; ++i) {
        zCPolygon* polygon = patchMap->surface.parray[i];
        if (polygon->lightmap)
            polygon->lightmap->Release();
        lightMap->AddRef();
        polygon->lightmap = lightMap;
    }
}

void zCWorld::LightPatchMap(zCPatchMap* patchMap)
{
    zCArray<zCVob*> lights;
    SearchVobListByBaseClass(&zCVobLight::classDef, lights, &globalVobTree);
    for (int i = 0; i < lights.numInArray; ++i) {
        zCVobLight* light = static_cast<zCVobLight*>(lights.parray[i]);
        if (!light->lightData.isStatic)
            continue;
        zVEC3 lightPosition;
        for (int axis = 0; axis < 3; ++axis)
            lightPosition.n[axis] = light->trafoObjToWorld.m[axis][3];
        for (int j = 0; j < patchMap->patchList.numInArray; ++j) {
            zCPatch* patch = patchMap->patchList.parray[j];
            zVEC3 direction;
            float distance2 = 0.0f;
            for (int axis = 0; axis < 3; ++axis) {
                direction.n[axis] = patch->center.n[axis] - lightPosition.n[axis];
                distance2 += direction.n[axis] * direction.n[axis];
            }
            if (distance2 >= light->lightData.range * light->lightData.range || distance2 <= 0.0f)
                continue;
            const float distance = std::sqrt(distance2);
            float incidence = 0.0f;
            for (int axis = 0; axis < 3; ++axis) {
                direction.n[axis] /= distance;
                incidence -= direction.n[axis] * patch->normal.n[axis];
            }
            if (incidence <= 0.0f)
                continue;
            float attenuation = 1.0f - distance * light->lightData.rangeInv;
            if (attenuation <= 0.0f)
                continue;
            for (int axis = 0; axis < 3; ++axis)
                direction.n[axis] *= distance;
            if (TraceRayNearestHit(lightPosition, direction, static_cast<const zCArray<zCVob*>*>(0),
                0x120)) {
                float hitDistance2 = 0.0f;
                for (int axis = 0; axis < 3; ++axis) {
                    const float delta = traceRayReport.foundIntersection.n[axis] - lightPosition.n[axis];
                    hitDistance2 += delta * delta;
                }
                if (hitDistance2 < distance2 * 0.81f)
                    continue;
            }
            const unsigned char* color = &light->lightData.lightColor.b;
            for (int axis = 0; axis < 3; ++axis) {
                const float contribution = color[axis] * incidence * attenuation;
                patch->radiosity.n[axis] += contribution;
                patch->radToShoot.n[axis] += contribution;
            }
            patchMap->hit = 1;
        }
    }
}

static zCArray<zCPatchMap*> radiosityPatchMaps;

void zCWorld::GenerateSurfaces(int generateLightmaps, zTBBox3D* bounds)
{
    zCMesh* mesh = bspTree.mesh;
    zCArray<zCPolygon*> remaining;
    for (int i = 0; i < mesh->numPoly; ++i) {
        zCPolygon* polygon = mesh->polyList[i];
        polygon->flags[0] |= 0x10;
        if (polygon->material && (polygon->material->smooth & 2))
            continue;
        if (bounds) {
            const zTBBox3D polygonBounds = polygon->GetBBox3D();
            int intersects = 1;
            for (int axis = 0; axis < 3; ++axis) {
                if (polygonBounds.maxs.n[axis] < bounds->mins.n[axis] ||
                    polygonBounds.mins.n[axis] > bounds->maxs.n[axis])
                    intersects = 0;
            }
            if (!intersects)
                continue;
        }
        polygon->flags[0] &= ~0x10;
        remaining.InsertEnd(polygon);
    }
    while (remaining.numInArray > 0) {
        zCArray<zCPolygon*> surface;
        zCPolygon* seed = remaining.parray[0];
        surface.InsertEnd(seed);
        --remaining.numInArray;
        if (remaining.numInArray > 0)
            remaining.parray[0] = remaining.parray[remaining.numInArray];
        seed->flags[0] |= 0x10;
        for (int surfaceIndex = 0; surfaceIndex < surface.numInArray; ++surfaceIndex) {
            zCPolygon* current = surface.parray[surfaceIndex];
            for (int i = 0; i < remaining.numInArray;) {
                zCPolygon* candidate = remaining.parray[i];
                float normalDot = 0.0f;
                for (int axis = 0; axis < 3; ++axis)
                    normalDot += current->polyPlane.normal.n[axis] * candidate->polyPlane.normal.n[axis];
                int sharesVertex = 0;
                for (int j = 0; j < current->polyNumVert && !sharesVertex; ++j)
                    sharesVertex = candidate->VertPartOfPoly(current->vertex[j]);
                if (normalDot < 0.7f || !sharesVertex) {
                    ++i;
                    continue;
                }
                surface.InsertEnd(candidate);
                candidate->flags[0] |= 0x10;
                --remaining.numInArray;
                remaining.parray[i] = remaining.parray[remaining.numInArray];
            }
        }
        zCPatchMap* patchMap = GeneratePatchMapFromSurface(surface);
        LightPatchMap(patchMap);
        if (generateLightmaps) {
            GenerateLightmapFromPatchMap(patchMap);
            delete patchMap;
        } else
            radiosityPatchMaps.InsertEnd(patchMap);
    }
    mesh->CombineLightmaps();
}

zCPatchMap::~zCPatchMap()
{
    for (int i = 0; i < patchList.numInArray; ++i)
        delete patchList.parray[i];
    patchList.numInArray = 0;
    surface.numInArray = 0;
    lastRayHitPoly = 0;
}

void zCWorld::MakeTransfers()
{
    zerr->Message(zSTRING("D: WORLD: LM: Making transfers..."));
    for (int mapIndex = 0; mapIndex < radiosityPatchMaps.numInArray; ++mapIndex) {
        zCPatchMap* sourceMap = radiosityPatchMaps.parray[mapIndex];
        for (int patchIndex = 0; patchIndex < sourceMap->patchList.numInArray; ++patchIndex) {
            zCPatch* source = sourceMap->patchList.parray[patchIndex];
            zCArray<zCTransferConstr> candidates;
            float totalFactor = 0.0f;
            for (int targetMapIndex = 0; targetMapIndex < radiosityPatchMaps.numInArray;
                ++targetMapIndex) {
                zCPatchMap* targetMap = radiosityPatchMaps.parray[targetMapIndex];
                for (int targetIndex = 0; targetIndex < targetMap->patchList.numInArray;
                    ++targetIndex) {
                    zCPatch* target = targetMap->patchList.parray[targetIndex];
                    if (target == source)
                        continue;
                    zVEC3 direction;
                    float distance2 = 0.0f;
                    for (int axis = 0; axis < 3; ++axis) {
                        direction.n[axis] = target->center.n[axis] - source->center.n[axis];
                        distance2 += direction.n[axis] * direction.n[axis];
                    }
                    if (distance2 >= 640000.0f || distance2 <= 0.0f)
                        continue;
                    const float invDistance = 1.0f / std::sqrt(distance2);
                    float sourceDot = 0.0f;
                    float targetDot = 0.0f;
                    for (int axis = 0; axis < 3; ++axis) {
                        direction.n[axis] *= invDistance;
                        sourceDot += direction.n[axis] * source->normal.n[axis];
                        targetDot -= direction.n[axis] * target->normal.n[axis];
                    }
                    const float orientation = sourceDot * targetDot;
                    if (orientation <= 0.0f)
                        continue;
                    const float formFactor = orientation * target->area /
                        (distance2 * 3.14159265358979323846f);
                    if (formFactor < 0.001f)
                        continue;
                    zCTransferConstr transfer;
                    transfer.targetPatch = target;
                    transfer.formFactor = formFactor;
                    candidates.InsertEnd(transfer);
                    totalFactor += formFactor;
                }
            }
            if (totalFactor <= 0.0f)
                continue;
            source->transferList.numInArray = 0;
            for (int i = 0; i < candidates.numInArray; ++i) {
                zCTransfer transfer;
                transfer.targetPatch = candidates.parray[i].targetPatch;
                float normalized = candidates.parray[i].formFactor / totalFactor;
                if (normalized > 1.0f)
                    normalized = 1.0f;
                transfer.formFactor = static_cast<unsigned short>(normalized * 65536.0f);
                source->transferList.InsertEnd(transfer);
            }
        }
    }
}

void zCPatch::DoTransfers()
{
    for (int axis = 0; axis < 3; ++axis)
        radiosity.n[axis] += radToShoot.n[axis] * reflectivity.n[axis];
    for (int i = 0; i < transferList.numInArray; ++i) {
        zCTransfer& transfer = transferList.parray[i];
        const float formFactor = transfer.formFactor * 1.52587891e-05f;
        for (int axis = 0; axis < 3; ++axis) {
            const float energy = formFactor * radToShoot.n[axis] * 0.45f;
            transfer.targetPatch->radiosity.n[axis] += energy;
            transfer.targetPatch->radToShoot.n[axis] += energy;
        }
    }
    for (int axis = 0; axis < 3; ++axis)
        radToShoot.n[axis] = 0.0f;
}

void zCWorld::GenerateLightmapsRadiosity(zTBBox3D* bounds)
{
    GenerateSurfaces(0, bounds);
    MakeTransfers();
    zerr->Message(zSTRING("D: WORLD: LM: Iterating..."));
    for (int iteration = 0; iteration < 7; ++iteration) {
        for (int i = 0; i < radiosityPatchMaps.numInArray; ++i) {
            zCPatchMap* patchMap = radiosityPatchMaps.parray[i];
            for (int j = 0; j < patchMap->patchList.numInArray; ++j)
                patchMap->patchList.parray[j]->DoTransfers();
        }
    }
    for (int i = 0; i < radiosityPatchMaps.numInArray; ++i) {
        GenerateLightmapFromPatchMap(radiosityPatchMaps.parray[i]);
        delete radiosityPatchMaps.parray[i];
    }
    radiosityPatchMaps.numInArray = 0;
    zerr->Message(zSTRING("D: WORLD: LM: Finished."));
}

void zCWorld::GenerateLightmaps(const int& lightMode, zTBBox3D* bounds)
{
    if (!compiled)
        return;
    bspTree.mesh->CreateListsFromArrays();
    if (!bounds) {
        for (int i = 0; i < bspTree.mesh->numPoly; ++i) {
            if (bspTree.mesh->polyList[i]->lightmap) {
                bspTree.mesh->polyList[i]->lightmap->Release();
                bspTree.mesh->polyList[i]->lightmap = 0;
            }
        }
        bspTree.mesh->hasLightmaps = 0;
    }
    GenerateSurfaces(1, bounds);
    bspTree.mesh->hasLightmaps = 0;
    for (int i = 0; i < bspTree.mesh->numPoly; ++i) {
        if (bspTree.mesh->polyList[i]->lightmap) {
            bspTree.mesh->hasLightmaps = 1;
            break;
        }
    }
    worldRenderMode = 1;
    bspTree.worldRenderMode = 1;
}

int g_bIsInCompileLightMode;

static void MarkVobLightingDirtyRecursive(zCTree<zCVob>* node)
{
    if (!node)
        return;
    if (node->data)
        node->data->castDynShadow |= 0x0c;
    for (zCTree<zCVob>* child = node->firstChild; child; child = child->next)
        MarkVobLightingDirtyRecursive(child);
}

void zCWorld::MakeVobLightingDirty()
{
    MarkVobLightingDirtyRecursive(&globalVobTree);
}

void zCWorld::GenerateStaticWorldLighting(const int& lightMode, zTBBox3D* bounds)
{
    if (bspTree.mesh) {
        bspTree.mesh->ResetStaticLight();
        bspTree.mesh->CalcVertexNormals();
    }
    g_bIsInCompileLightMode = 1;
    if (lightMode)
        GenerateLightmaps(lightMode, bounds);
    GenerateStaticVertexLighting();
    bspTree.CalcStaticLeafLightData();
    MakeVobLightingDirty();
    g_bIsInCompileLightMode = 0;
}
