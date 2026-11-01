#include "zPolyStrip.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

static int polyStripState = -1;
static int lightningState = -1;
static zCClassDef zCPolyStripClassDef("zCPolyStrip", "zCVisual", NULL, 0, 0xB4, 0, "");
zCClassDef* zCPolyStrip::classDef = &zCPolyStripClassDef;
static int quadMarkState = -1;
static int quadMarkSystemState = -1;
static zCClassDef zCQuadMarkClassDef("zCQuadMark", "zCVisual", zCQuadMark::_CreateNewInstance,
    1, 0x88, 2, "");
zCClassDef* zCQuadMark::classDef = &zCQuadMarkClassDef;

struct zTIMER {
    float frameTimeFloat;
};

extern zTIMER ztimer;

// EXE: 0x005BD2A0
zCObject* zCPolyStrip::_CreateNewInstance()
{
    zCPolyStrip* strip = new zCPolyStrip;
    zCClassDef::ObjectCreated(strip, classDef);
    return strip;
}

zCPolyStrip::zCPolyStrip()
    : material(new zCMaterial), vertList(0), polyList(0), numPoly(0), numVert(0),
      centerPointList(0), alphaList(0), width(10.0f), connectedVob(0),
      camAlign(zCAM_ALIGN_NONE), heightCheck(0), everyFrameUpdate(0),
      heightBound(3.40282347e+38f), firstSeg(0), lastSeg(0), numSeg(0),
      visLastFrac(0), visFirstFrac(1.0f), alphaFadeSpeed(0), newAlphaFadeSpeed(0),
      newAlpha(255.0f), lastDirSeg(-1)
{
    localFOR = 0;
    lastDirNormal.n[0] = 0;
    lastDirNormal.n[1] = 0;
    lastDirNormal.n[2] = 0;
    bbox3D.mins.n[0] = 0;
    bbox3D.mins.n[1] = 0;
    bbox3D.mins.n[2] = 0;
    bbox3D.maxs = bbox3D.mins;
}

zCPolyStrip::~zCPolyStrip()
{
    FreeResources();
    if (material)
        material->Release();
    connectedVob = 0;
}

int zCPolyStrip::IsBBox3DLocal()
{
    return localFOR;
}

zTBBox3D zCPolyStrip::GetBBox3D()
{
    if (numSeg <= 0) {
        if (!connectedVob) {
            bbox3D.mins.n[0] = 0;
            bbox3D.mins.n[1] = 0;
            bbox3D.mins.n[2] = 0;
            bbox3D.maxs.n[0] = 1.0f;
            bbox3D.maxs.n[1] = 1.0f;
            bbox3D.maxs.n[2] = 1.0f;
        }
        else {
            for (int axis = 0; axis < 3; ++axis) {
                bbox3D.mins.n[axis] = connectedVob->trafoObjToWorld.m[axis][3];
                bbox3D.maxs.n[axis] = bbox3D.mins.n[axis] + 1.0f;
            }
        }
    }
    return bbox3D;
}

void zCPolyStrip::CalcBBox3D()
{
    if (numSeg <= 0) {
        bbox3D.mins.n[0] = 0;
        bbox3D.mins.n[1] = 0;
        bbox3D.mins.n[2] = 0;
        bbox3D.maxs = bbox3D.mins;
    } else {
        for (int axis = 0; axis < 3; ++axis) {
            bbox3D.mins.n[axis] = 3.40282347e+38f;
            bbox3D.maxs.n[axis] = -3.40282347e+38f;
        }
        for (int i = 0; i < numSeg - 1; ++i) {
            const zVEC3& point = centerPointList[(firstSeg + i) % numPoly];
            for (int axis = 0; axis < 3; ++axis) {
                if (point.n[axis] < bbox3D.mins.n[axis])
                    bbox3D.mins.n[axis] = point.n[axis];
                if (point.n[axis] > bbox3D.maxs.n[axis])
                    bbox3D.maxs.n[axis] = point.n[axis];
            }
        }
    }

    if (!connectedVob)
        return;
    connectedVob->BeginMovement();
    zVEC3 center;
    for (int axis = 0; axis < 3; ++axis)
        center.n[axis] = (bbox3D.mins.n[axis] + bbox3D.maxs.n[axis]) * 0.5f;
    connectedVob->SetPositionWorld(center);
    connectedVob->UpdateVisualDependencies(1);
    connectedVob->EndMovement(1);
}

void zCPolyStrip::SetVisualUsedBy(zCVob* vob)
{
    vob->showVisual &= ~0x20;
    connectedVob = vob;
}

void zCPolyStrip::EmptyEffect()
{
    visFirstFrac = 1.0f;
    visLastFrac = 0;
    numSeg = 0;
    firstSeg = 0;
    lastSeg = 0;
    newAlpha = 255.0f;
}

void zCPolyStrip::AllocateResources(int count)
{
    numPoly = count;
    numVert = count * 2;
    polyList = new zCPolygon[count];
    vertList = new zCVertex[numVert];
    centerPointList = new zVEC3[count];
    alphaList = new float[count];
    for (int i = 0; i < count; ++i) {
        polyList[i].SetMaterial(material);
        alphaList[i] = newAlpha;
        polyList[i].SetVertices(&vertList[(i * 2) % numVert],
            &vertList[(i * 2 + 1) % numVert], &vertList[(i * 2 + 2) % numVert],
            &vertList[(i * 2 + 3) % numVert], 0);
        for (int j = 0; j < 4; ++j) {
            zCVertFeature* feature = polyList[i].feature[j];
            feature->mapping.n[0] = (j == 0 || j == 3) ? 1.0f : 0.0f;
            feature->mapping.n[1] = (j >= 2) ? 1.0f : 0.0f;
            for (int channel = 0; channel < 4; ++channel) {
                feature->color[channel] = 255;
                feature->lightStat[channel] = 255;
            }
        }
    }
}

void zCPolyStrip::FreeResources()
{
    delete[] vertList;
    vertList = 0;
    delete[] polyList;
    polyList = 0;
    delete[] centerPointList;
    centerPointList = 0;
    delete[] alphaList;
    alphaList = 0;
    numPoly = 0;
    numVert = 0;
    visLastFrac = 0;
    numSeg = 0;
    firstSeg = 0;
    lastSeg = 0;
    visFirstFrac = 1.0f;
    newAlpha = 255.0f;
}

void zCPolyStrip::SetVisibleSegments(float first, float last)
{
    const float firstValue = first * numPoly;
    const float lastValue = last * numPoly;
    const int firstIndex = static_cast<int>(firstValue);
    const int lastIndex = static_cast<int>(lastValue + 0.999999f);
    firstSeg = firstIndex % numPoly;
    lastSeg = lastIndex % numPoly;
    visFirstFrac = firstValue - firstIndex;
    visLastFrac = 1.0f - (lastValue - static_cast<int>(lastValue));
    numSeg = lastSeg > firstSeg ? lastSeg - firstSeg + 1 : lastSeg - firstSeg + numPoly + 1;
    alphaList[lastSeg] = newAlpha;
}

int zCPolyStrip::Render(zTRenderContext&)
{
    if (numSeg <= 0)
        return 0;
    newAlpha -= ztimer.frameTimeFloat * newAlphaFadeSpeed;
    if (newAlpha < 0)
        newAlpha = 0;
    for (int i = 0; i < numSeg; ++i) {
        const int index = (firstSeg + i) % numPoly;
        alphaList[index] -= ztimer.frameTimeFloat * alphaFadeSpeed;
        if (alphaList[index] < 0)
            alphaList[index] = 0;
    }
    return 1;
}

void zCPolyStrip::AddSegment(const zVEC3& first, const zVEC3& second)
{
    if (numSeg >= numPoly)
        return;
    const int index = (firstSeg + numSeg) % numPoly;
    vertList[index * 2].position = first;
    vertList[index * 2 + 1].position = second;
    for (int axis = 0; axis < 3; ++axis)
        centerPointList[index].n[axis] = (first.n[axis] + second.n[axis]) * 0.5f;
    alphaList[index] = newAlpha;
    if (!everyFrameUpdate) {
        for (int axis = 0; axis < 3; ++axis) {
            if (centerPointList[index].n[axis] < bbox3D.mins.n[axis])
                bbox3D.mins.n[axis] = centerPointList[index].n[axis];
            if (centerPointList[index].n[axis] > bbox3D.maxs.n[axis])
                bbox3D.maxs.n[axis] = centerPointList[index].n[axis];
        }
    }
    lastSeg = index;
    ++numSeg;
}

void zCPolyStrip::AddSegment(const zVEC3& center)
{
    if (numSeg >= numPoly)
        return;
    const int index = (firstSeg + numSeg) % numPoly;
    centerPointList[index] = center;
    for (int axis = 0; axis < 3; ++axis) {
        if (center.n[axis] < bbox3D.mins.n[axis])
            bbox3D.mins.n[axis] = center.n[axis];
        if (center.n[axis] > bbox3D.maxs.n[axis])
            bbox3D.maxs.n[axis] = center.n[axis];
    }
    lastSeg = index;
    ++numSeg;
}

void zCPolyStrip::AddSegmentSmart(const zVEC3& first, const zVEC3& second)
{
    if (numSeg >= 2) {
        zVEC3 center;
        for (int axis = 0; axis < 3; ++axis)
            center.n[axis] = (first.n[axis] + second.n[axis]) * 0.5f;
        zVEC3 direction;
        for (int axis = 0; axis < 3; ++axis)
            direction.n[axis] = centerPointList[lastSeg].n[axis] - center.n[axis];
        const float length = std::sqrt(direction.n[0] * direction.n[0] +
            direction.n[1] * direction.n[1] + direction.n[2] * direction.n[2]);
        if (length > 0) {
            for (int axis = 0; axis < 3; ++axis)
                direction.n[axis] /= length;
        }
        const float dot = direction.n[0] * lastDirNormal.n[0] +
            direction.n[1] * lastDirNormal.n[1] + direction.n[2] * lastDirNormal.n[2];
        if (std::fabs(dot) > 0.995f) {
            vertList[lastSeg * 2].position = first;
            vertList[lastSeg * 2 + 1].position = second;
            centerPointList[lastSeg] = center;
            alphaList[lastSeg] = newAlpha;
            return;
        }
        lastDirNormal = direction;
    }
    AddSegment(first, second);
}

void zCPolyStrip::UpdateSegment(int index, const zVEC3& first, const zVEC3& second,
    const zVEC3& center)
{
    if (index == -1)
        index = lastSeg;
    vertList[index * 2].position = first;
    vertList[index * 2 + 1].position = second;
    centerPointList[index] = center;
    alphaList[index] = newAlpha;
}

void zCPolyStrip::UpdateSegmentAlpha(int index, float alpha)
{
    if (index == -1)
        index = lastSeg;
    alphaList[index] = alpha;
}

void zCPolyStrip::AlignToCamera()
{
    if (!camAlign || !zCCamera::activeCam || numSeg <= 0)
        return;
    if (camAlign == zCAM_ALIGN_STRIP) {
        zVEC3 segment;
        zVEC3 side;
        for (int axis = 0; axis < 3; ++axis)
            segment.n[axis] = centerPointList[lastSeg].n[axis] - centerPointList[firstSeg].n[axis];
        side.n[0] = segment.n[1] * zCCamera::activeCam->camMatrixInv.m[2][2] -
            segment.n[2] * zCCamera::activeCam->camMatrixInv.m[2][1];
        side.n[1] = segment.n[2] * zCCamera::activeCam->camMatrixInv.m[2][0] -
            segment.n[0] * zCCamera::activeCam->camMatrixInv.m[2][2];
        side.n[2] = segment.n[0] * zCCamera::activeCam->camMatrixInv.m[2][1] -
            segment.n[1] * zCCamera::activeCam->camMatrixInv.m[2][0];
        const float length = std::sqrt(side * side);
        if (length > 0) {
            for (int axis = 0; axis < 3; ++axis)
                side.n[axis] = side.n[axis] / length * width;
        }
        for (int i = 0; i < numSeg; ++i) {
            const int index = (firstSeg + i) % numPoly;
            for (int axis = 0; axis < 3; ++axis) {
                vertList[index * 2].position.n[axis] = centerPointList[index].n[axis] - side.n[axis];
                vertList[index * 2 + 1].position.n[axis] =
                    centerPointList[index].n[axis] + side.n[axis];
            }
        }
        return;
    }
    for (int i = 0; i < numSeg; ++i) {
        const int index = (firstSeg + i) % numPoly;
        const int next = (index + 1) % numPoly;
        zVEC3 segment;
        for (int axis = 0; axis < 3; ++axis)
            segment.n[axis] = centerPointList[index].n[axis] - centerPointList[next].n[axis];
        zVEC3 side;
        side.n[0] = segment.n[1] * zCCamera::activeCam->camMatrixInv.m[2][2] -
            segment.n[2] * zCCamera::activeCam->camMatrixInv.m[2][1];
        side.n[1] = segment.n[2] * zCCamera::activeCam->camMatrixInv.m[2][0] -
            segment.n[0] * zCCamera::activeCam->camMatrixInv.m[2][2];
        side.n[2] = segment.n[0] * zCCamera::activeCam->camMatrixInv.m[2][1] -
            segment.n[1] * zCCamera::activeCam->camMatrixInv.m[2][0];
        const float length = std::sqrt(side.n[0] * side.n[0] + side.n[1] * side.n[1] +
            side.n[2] * side.n[2]);
        if (length > 0) {
            for (int axis = 0; axis < 3; ++axis)
                side.n[axis] = side.n[axis] / length * width;
        }
        for (int axis = 0; axis < 3; ++axis) {
            vertList[index * 2].position.n[axis] = centerPointList[index].n[axis] - side.n[axis];
            vertList[index * 2 + 1].position.n[axis] = centerPointList[index].n[axis] + side.n[axis];
        }
    }
}

void zCPolyStrip::ProjectToSphere(const zTBSphere3D& sphere)
{
    for (int i = 0; i < numSeg; ++i) {
        const int index = (firstSeg + i) % numPoly;
        zVEC3 direction;
        for (int axis = 0; axis < 3; ++axis)
            direction.n[axis] = centerPointList[index].n[axis] - sphere.center.n[axis];
        const float length = std::sqrt(direction.n[0] * direction.n[0] +
            direction.n[1] * direction.n[1] + direction.n[2] * direction.n[2]);
        if (length > 0) {
            for (int axis = 0; axis < 3; ++axis)
                centerPointList[index].n[axis] = sphere.center.n[axis] +
                    direction.n[axis] / length * sphere.radius;
        }
    }
}

static zVEC3* lightningWritePos;

void zCPolyStrip::Lightning0Subdivide(const zVEC3& first, const zVEC3& second,
    float amount, int depth)
{
    if (!depth) {
        *++lightningWritePos = second;
        return;
    }
    zVEC3 middle;
    for (int axis = 0; axis < 3; ++axis) {
        int random = 0;
        for (int i = 0; i < 6; ++i)
            random += std::rand() - std::rand();
        middle.n[axis] = (first.n[axis] + second.n[axis]) * 0.5f + random * amount * 5.08641824e-06f;
    }
    const float nextAmount = amount * 0.5f;
    Lightning0Subdivide(first, middle, nextAmount, depth - 1);
    Lightning0Subdivide(middle, second, nextAmount, depth - 1);
}

void zCPolyStrip::GenerateCPLightning0(const zVEC3& first, const zVEC3& second, float amount)
{
    visLastFrac = 0;
    firstSeg = 0;
    numSeg = numPoly;
    visFirstFrac = 1.0f;
    newAlpha = 255.0f;
    lastSeg = numPoly - 1;
    centerPointList[0] = first;
    lightningWritePos = centerPointList;
    int depth = 0;
    int value = numPoly - 1;
    while (!(value & 1)) {
        value >>= 1;
        ++depth;
    }
    float x = std::fabs(second.n[0] - first.n[0]);
    float y = std::fabs(second.n[1] - first.n[1]);
    float z = std::fabs(second.n[2] - first.n[2]);
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
    const float distance = x - x * 0.0625f + (y + z) * 0.375f;
    Lightning0Subdivide(first, second, amount * distance, depth);
    CalcBBox3D();
    for (int i = 0; i < numPoly; ++i)
        alphaList[i] = 255.0f;
}

void zCPolyStrip::GenerateCircle(const zTBSphere3D& sphere, const zVEC3& start,
    const zVEC3& normal)
{
    visFirstFrac = 1.0f;
    visLastFrac = 0;
    numSeg = 0;
    firstSeg = 0;
    lastSeg = 0;
    zVEC3 current = start;
    const float angleStep = 360.0f / numPoly * 0.0174532924f;
    for (int i = 0; i < numPoly; ++i) {
        const float angle = angleStep * i;
        const float sine = std::sin(angle);
        const float cosine = std::cos(angle);
        zVEC3 relative;
        zVEC3 cross;
        for (int axis = 0; axis < 3; ++axis)
            relative.n[axis] = start.n[axis] - sphere.center.n[axis];
        cross.n[0] = normal.n[1] * relative.n[2] - normal.n[2] * relative.n[1];
        cross.n[1] = normal.n[2] * relative.n[0] - normal.n[0] * relative.n[2];
        cross.n[2] = normal.n[0] * relative.n[1] - normal.n[1] * relative.n[0];
        for (int axis = 0; axis < 3; ++axis)
            current.n[axis] = sphere.center.n[axis] + relative.n[axis] * cosine + cross.n[axis] * sine;
        zVEC3 first;
        zVEC3 second;
        for (int axis = 0; axis < 3; ++axis) {
            first.n[axis] = current.n[axis] - normal.n[axis] * width;
            second.n[axis] = current.n[axis] + normal.n[axis] * width;
        }
        UpdateSegment(i, first, second, current);
        ++numSeg;
    }
    newAlpha = 255.0f;
    CalcBBox3D();
}

int zCLightning::Render(zTRenderContext& context)
{
    return rootBolt.Render(context);
}

void zCLightning::SetVisualUsedBy(zCVob* vob)
{
    vob->showVisual &= ~0x20;
    vob->SetCollDetStat(0);
    vob->SetCollDetDyn(0);
}

static zTBSphere3D lightningProjectionSphere;

void zCLightning::SetProjectionSphere(const zTBSphere3D& sphere)
{
    lightningProjectionSphere = sphere;
}

void zCLightning::Generate(const zVEC3& first, const zVEC3& second)
{
    rootBolt.FreeResources();
    rootBolt.width = 30.0f;
    rootBolt.material->color[3] = 200;
    rootBolt.Generate_R(first, second, 1.0f);
}

zCLightning::zCBolt::~zCBolt()
{
    for (int i = 0; i < childList.numInArray; ++i)
        delete childList.parray[i];
}

void zCLightning::zCBolt::Generate_R(const zVEC3& first, const zVEC3& second, float amount)
{
    if (amount < 0.05f)
        return;
    AllocateResources(8);
    GenerateCPLightning0(first, second, amount * 0.6f);
    const int children = std::rand() % 3;
    for (int i = 0; i < children; ++i) {
        zCBolt* child = new zCBolt;
        const int segment = std::rand() % numSeg;
        zVEC3 direction;
        for (int axis = 0; axis < 3; ++axis)
            direction.n[axis] = second.n[axis] - centerPointList[segment].n[axis];
        child->width = width * 0.6f;
        child->material->color[3] = material->color[3] * 0.4f;
        child->Generate_R(centerPointList[segment], second, amount * 0.6f);
        childList.InsertEnd(child);
    }
}

void zCLightning::zCBolt::FreeResources()
{
    zCPolyStrip::FreeResources();
    EmptyEffect();
    for (int i = 0; i < childList.numInArray; ++i)
        childList.parray[i]->FreeResources();
    childList.DeleteList();
}

int zCLightning::zCBolt::Render(zTRenderContext& context)
{
    zCPolyStrip::Render(context);
    for (int i = 0; i < childList.numInArray; ++i)
        childList.parray[i]->Render(context);
    return 1;
}

// EXE: 0x005D0910
zCObject* zCQuadMark::_CreateNewInstance()
{
    zCQuadMark* mark = new zCQuadMark;
    zCClassDef::ObjectCreated(mark, classDef);
    return mark;
}

zCQuadMark::zCQuadMark()
    : connectedVob(0), alphaFadeSpeed(0), dontRepositionConnectedVob(0),
      worldPolyList(0), numWorldPoly(0), firstWorldPoly(0)
{
    quadMesh = new zCMesh;
    material = new zCMaterial;
    actMaterial = material;
    material->AddRef();
    averageNormal.n[0] = 0;
    averageNormal.n[1] = 0;
    averageNormal.n[2] = 0;
}

zCClassDef* zCQuadMark::_GetClassDef() const
{
    return classDef;
}

int zCQuadMark::IsBBox3DLocal()
{
    return 1;
}

zSTRING zCQuadMark::GetVisualName()
{
    return zSTRING("QUADMARK");
}

zTQuadMarkPoly::~zTQuadMarkPoly()
{
    vertex.DeleteList();
}

zCQuadMark::~zCQuadMark()
{
    delete quadMesh;
    quadMesh = 0;
    if (material)
        material->Release();
    if (actMaterial)
        actMaterial->Release();
    material = 0;
    actMaterial = 0;
    connectedVob = 0;
    worldPolyList = 0;
    firstWorldPoly = 0;
    numWorldPoly = 0;
}

void zCQuadMark::SetVisualUsedBy(zCVob* vob)
{
    connectedVob = vob;
    vob->showVisual ^= 0x20;
    vob->SetCollDetStat(0);
    vob->SetCollDetDyn(0);
}

zTBBox3D zCQuadMark::GetBBox3D()
{
    zTBBox3D bbox;
    if (!quadMesh->numVerts) {
        std::memset(&bbox, 0, sizeof(bbox));
        return bbox;
    }
    bbox.mins = quadMesh->vertexList[0];
    bbox.maxs = quadMesh->vertexList[0];
    for (int i = 1; i < quadMesh->numVerts; ++i) {
        for (int axis = 0; axis < 3; ++axis) {
            const float value = quadMesh->vertexList[i].n[axis];
            if (value < bbox.mins.n[axis])
                bbox.mins.n[axis] = value;
            if (value > bbox.maxs.n[axis])
                bbox.maxs.n[axis] = value;
        }
    }
    return bbox;
}

int zCQuadMark::Render(zTRenderContext& context)
{
    int result = 0;
    zCTexture* texture = quadMesh->numPoly > 0 ? (*quadMesh->polyList)->material->texture : 0;
    if (quadMesh->numPoly <= 0 || !texture || texture->CacheIn(0.5f) == 3) {
        if (quadMarkAniTracker.numInArray > 0)
            zrenderer->SetTextureWrapEnabled(0);
        result = quadMesh->Render(context);
        if (quadMarkAniTracker.numInArray > 0)
            zrenderer->SetTextureWrapEnabled(1);
    }
    ProcessAniTracker();
    return result;
}

static float QuadPlaneDistance(const zVEC3& position, const zTPlane& plane)
{
    return position.n[0] * plane.normal.n[0] + position.n[1] * plane.normal.n[1] +
        position.n[2] * plane.normal.n[2] - plane.distance;
}

int zCQuadMark::ClipQuadPolyToPlane(zTQuadMarkPoly& source,
    zTQuadMarkPoly& destination, const zTPlane& plane)
{
    destination.vertex.numInArray = 0;
    if (!source.vertex.numInArray)
        return 0;
    int clipped = 0;
    int previousIndex = source.vertex.numInArray - 1;
    zTQuadMarkVert previous = source.vertex.parray[previousIndex];
    float previousDistance = QuadPlaneDistance(previous.position, plane);
    int previousInside = previousDistance >= 0;
    for (int i = 0; i < source.vertex.numInArray; ++i) {
        const zTQuadMarkVert& current = source.vertex.parray[i];
        const float currentDistance = QuadPlaneDistance(current.position, plane);
        const int currentInside = currentDistance >= 0;
        if (currentInside != previousInside) {
            const float fraction = previousDistance / (previousDistance - currentDistance);
            zTQuadMarkVert intersection;
            for (int axis = 0; axis < 3; ++axis)
                intersection.position.n[axis] = previous.position.n[axis] +
                    (current.position.n[axis] - previous.position.n[axis]) * fraction;
            for (int axis = 0; axis < 2; ++axis)
                intersection.texCoord.n[axis] = previous.texCoord.n[axis] +
                    (current.texCoord.n[axis] - previous.texCoord.n[axis]) * fraction;
            destination.vertex.InsertEnd(intersection);
            clipped = 1;
        }
        if (currentInside)
            destination.vertex.InsertEnd(current);
        previous = current;
        previousDistance = currentDistance;
        previousInside = currentInside;
    }
    return clipped;
}

void zCQuadMark::CalcQuadAxis(zCPolygon* polygon, zVEC3& firstAxis, zVEC3& secondAxis)
{
    if (std::fabs(polygon->polyPlane.normal.n[1]) >= 0.99f) {
        secondAxis.n[0] = 0;
        secondAxis.n[1] = 1.0f;
        secondAxis.n[2] = 0;
    } else if (std::fabs(polygon->polyPlane.normal.n[0]) >= 0.99f) {
        secondAxis.n[0] = 1.0f;
        secondAxis.n[1] = 0;
        secondAxis.n[2] = 0;
    } else {
        secondAxis.n[0] = 0;
        secondAxis.n[1] = 0;
        secondAxis.n[2] = 1.0f;
    }
    const zVEC3& normal = polygon->polyPlane.normal;
    const float projection = normal.n[0] * secondAxis.n[0] + normal.n[1] * secondAxis.n[1] +
        normal.n[2] * secondAxis.n[2];
    float length = 0;
    for (int axis = 0; axis < 3; ++axis) {
        secondAxis.n[axis] -= normal.n[axis] * projection;
        length += secondAxis.n[axis] * secondAxis.n[axis];
    }
    length = std::sqrt(length);
    for (int axis = 0; axis < 3; ++axis)
        secondAxis.n[axis] /= length;
    firstAxis.n[0] = normal.n[2] * secondAxis.n[1] - normal.n[1] * secondAxis.n[2];
    firstAxis.n[1] = normal.n[0] * secondAxis.n[2] - normal.n[2] * secondAxis.n[0];
    firstAxis.n[2] = normal.n[1] * secondAxis.n[0] - normal.n[0] * secondAxis.n[1];
}

void zCQuadMark::InitQuadPoly(zTQuadMarkPoly& destination, zCPolygon* polygon)
{
    destination.vertex.numInArray = 0;
    if (polygon == firstWorldPoly) {
        for (int i = 0; i < firstQuadPoly.vertex.numInArray; ++i)
            destination.vertex.InsertEnd(firstQuadPoly.vertex.parray[i]);
    } else
        ProjectQuadPolyToPlane(firstQuadPoly, destination, polygon->polyPlane);
}

void zCQuadMark::ProjectQuadPolyToPlane(const zTQuadMarkPoly& source,
    zTQuadMarkPoly& destination, const zTPlane& plane)
{
    destination.vertex.numInArray = 0;
    for (int i = 0; i < source.vertex.numInArray; ++i) {
        zTQuadMarkVert projected = source.vertex.parray[i];
        const float distance = QuadPlaneDistance(projected.position, plane);
        for (int axis = 0; axis < 3; ++axis)
            projected.position.n[axis] -= plane.normal.n[axis] * distance;
        destination.vertex.InsertEnd(projected);
    }
}

void zCQuadMark::ClipQuadPolyToWorldPoly(zTQuadMarkPoly& polygon,
    zCPolygon* worldPoly, zCArray<int>& clippedEdges)
{
    clippedEdges.numInArray = 0;
    zTQuadMarkPoly current = polygon;
    for (int i = 0; i < worldPoly->polyNumVert; ++i) {
        const int next = (i + 1) % worldPoly->polyNumVert;
        const zVEC3& first = worldPoly->vertex[i]->position;
        const zVEC3& second = worldPoly->vertex[next]->position;
        zVEC3 edge;
        for (int axis = 0; axis < 3; ++axis)
            edge.n[axis] = second.n[axis] - first.n[axis];
        const zVEC3& normal = worldPoly->polyPlane.normal;
        zTPlane clipPlane;
        clipPlane.normal.n[0] = edge.n[1] * normal.n[2] - edge.n[2] * normal.n[1];
        clipPlane.normal.n[1] = edge.n[2] * normal.n[0] - edge.n[0] * normal.n[2];
        clipPlane.normal.n[2] = edge.n[0] * normal.n[1] - edge.n[1] * normal.n[0];
        float length = 0;
        for (int axis = 0; axis < 3; ++axis)
            length += clipPlane.normal.n[axis] * clipPlane.normal.n[axis];
        length = std::sqrt(length);
        for (int axis = 0; axis < 3; ++axis)
            clipPlane.normal.n[axis] /= length;
        clipPlane.distance = first.n[0] * clipPlane.normal.n[0] +
            first.n[1] * clipPlane.normal.n[1] + first.n[2] * clipPlane.normal.n[2];
        zTQuadMarkPoly clipped;
        const int wasClipped = ClipQuadPolyToPlane(current, clipped, clipPlane);
        if (clipped.vertex.numInArray < 3) {
            polygon.vertex.numInArray = 0;
            return;
        }
        if (wasClipped)
            clippedEdges.InsertEnd(i);
        current = clipped;
    }
    polygon = current;
}

void zCQuadMark::InitFirstQuadPoly(zCPolygon* worldPoly, const zVEC3& center,
    const zVEC3& firstAxis, const zVEC3& secondAxis, zTQuadMarkEffectParams* params)
{
    firstWorldPoly = worldPoly;
    firstQuadPoly.vertex.numInArray = 0;
    for (int i = 0; i < 4; ++i) {
        zTQuadMarkVert vertex;
        const float firstSign = i == 1 || i == 2 ? -1.0f : 1.0f;
        const float secondSign = i >= 2 ? -1.0f : 1.0f;
        for (int axis = 0; axis < 3; ++axis)
            vertex.position.n[axis] = center.n[axis] + firstAxis.n[axis] * firstSign +
                secondAxis.n[axis] * secondSign;
        firstQuadPoly.vertex.InsertEnd(vertex);
    }
    firstQuadPoly.vertex.parray[0].texCoord.n[0] = 1.0f;
    firstQuadPoly.vertex.parray[0].texCoord.n[1] = 0;
    firstQuadPoly.vertex.parray[1].texCoord.n[0] = 0;
    firstQuadPoly.vertex.parray[1].texCoord.n[1] = 0;
    firstQuadPoly.vertex.parray[2].texCoord.n[0] = 0;
    firstQuadPoly.vertex.parray[2].texCoord.n[1] = 1.0f;
    firstQuadPoly.vertex.parray[3].texCoord.n[0] = 1.0f;
    firstQuadPoly.vertex.parray[3].texCoord.n[1] = 1.0f;
    if (params) {
        zTQuadMarkAniState state;
        std::memset(&state, 0, sizeof(state));
        state.firstPolyIndex = quadMesh->numPoly;
        state.lastPolyIndex = quadMesh->numPoly;
        state.scaleStart = params->texAniScaleStart;
        state.scaleCur = params->texAniScaleStart;
        state.scaleSpeed = params->texAniSpeed;
        state.scaleSpeedCur = params->texAniSpeed;
        state.texAniType = params->texAniType;
        state.alpha = 255.0f;
        quadMarkAniTracker.InsertEnd(state);
        if (params->texAniType == 1 || params->texAniType == 3) {
            for (int i = 0; i < 4; ++i) {
                for (int axis = 0; axis < 2; ++axis) {
                    float& uv = firstQuadPoly.vertex.parray[i].texCoord.n[axis];
                    uv = (uv - 0.5f) / params->texAniScaleStart + 0.5f;
                }
            }
        }
    }
}

void zCQuadMark::AddQuadPolyToQuadMesh(zTQuadMarkPoly& polygon, zCPolygon* worldPoly)
{
    if (polygon.vertex.numInArray < 3)
        return;
    zCPolygon* destination = new zCPolygon;
    destination->AllocVerts(polygon.vertex.numInArray);
    destination->material = actMaterial;
    if (actMaterial)
        actMaterial->AddRef();
    const int vertexBase = quadMesh->numVerts;
    zVEC3* vertices = new zVEC3[vertexBase + polygon.vertex.numInArray];
    for (int i = 0; i < vertexBase; ++i)
        vertices[i] = quadMesh->vertexList[i];
    for (int i = 0; i < polygon.vertex.numInArray; ++i) {
        vertices[vertexBase + i] = polygon.vertex.parray[i].position;
        destination->SetVertex(i, reinterpret_cast<zCVertex*>(&vertices[vertexBase + i]));
        destination->SetMapping(i, polygon.vertex.parray[i].texCoord);
        destination->feature[i]->vertNormal = worldPoly->polyPlane.normal;
    }
    delete[] quadMesh->vertexList;
    quadMesh->vertexList = vertices;
    quadMesh->numVerts += polygon.vertex.numInArray;
    zCPolygon** polygons = new zCPolygon*[quadMesh->numPoly + 1];
    for (int i = 0; i < quadMesh->numPoly; ++i)
        polygons[i] = quadMesh->polyList[i];
    polygons[quadMesh->numPoly] = destination;
    delete[] quadMesh->polyList;
    quadMesh->polyList = polygons;
    ++quadMesh->numPoly;
    destination->polyPlane = worldPoly->polyPlane;
    quadMesh->CalcBBox3D();
}

zCPolygon* zCQuadMark::GetNextWorldPoly(zCVertex* first, zCVertex* second)
{
    for (int i = 0; i < numWorldPoly; ++i) {
        zCPolygon* polygon = worldPolyList[i];
        if (!polygon->EdgePartOfPoly(first, second))
            continue;
        if (numWorldPoly > 1)
            worldPolyList[i] = worldPolyList[numWorldPoly - 1];
        --numWorldPoly;
        const zVEC3& firstNormal = firstWorldPoly->polyPlane.normal;
        const zVEC3& normal = polygon->polyPlane.normal;
        const float dot = firstNormal.n[0] * normal.n[0] + firstNormal.n[1] * normal.n[1] +
            firstNormal.n[2] * normal.n[2];
        if (dot >= 0)
            return polygon;
        --i;
    }
    return 0;
}

void zCQuadMark::ProcessWorldPoly(zCPolygon* worldPoly)
{
    zTQuadMarkPoly polygon;
    InitQuadPoly(polygon, worldPoly);
    zCArray<int> clippedEdges;
    ClipQuadPolyToWorldPoly(polygon, worldPoly, clippedEdges);
    if (polygon.vertex.numInArray < 3)
        return;
    AddQuadPolyToQuadMesh(polygon, worldPoly);
    for (int axis = 0; axis < 3; ++axis)
        averageNormal.n[axis] += worldPoly->polyPlane.normal.n[axis];
    for (int i = 0; i < clippedEdges.numInArray; ++i) {
        const int edge = clippedEdges.parray[i];
        zCPolygon* next = GetNextWorldPoly(worldPoly->vertex[edge],
            worldPoly->vertex[(edge + 1) % worldPoly->polyNumVert]);
        if (next && !visitedPolyList.IsInList(next)) {
            visitedPolyList.InsertEnd(next);
            ProcessWorldPoly(next);
        }
    }
}

void zCQuadMark::CreateQuadMark(const zVEC3& origin, const zVEC3& direction,
    const zVEC2& size, zSTRING& textureName, zTQuadMarkEffectParams* params)
{
    if (!actMaterial->texture) {
        if (actMaterial)
            actMaterial->Release();
        actMaterial = new zCMaterial;
        actMaterial->SetTexture(textureName);
    }
    CreateQuadMark(origin, direction, size, params);
}

void zCQuadMark::CreateQuadMark(const zVEC3& origin, const zVEC3& direction,
    const zVEC2& size, zCMaterial* materialValue, zTQuadMarkEffectParams* params)
{
    if (actMaterial != materialValue) {
        if (actMaterial)
            actMaterial->Release();
        actMaterial = materialValue;
        actMaterial->AddRef();
    }
    CreateQuadMark(origin, direction, size, params);
}

void zCQuadMark::CreateQuadMark(const zVEC3& origin, const zVEC3& direction,
    const zVEC2& size, zTQuadMarkEffectParams* params)
{
    if (!connectedVob || !connectedVob->homeWorld)
        return;
    zCWorld* world = connectedVob->homeWorld;
    if (world->TraceRayNearestHit(origin, direction, 0, 0x22)) {
        const zVEC3 center = world->traceRayReport.foundHitPosition;
        CreateQuadMark(world->traceRayReport.foundPoly, center, size, params);
    }
}

void zCQuadMark::CreateQuadMark(zCPolygon* worldPoly, const zVEC3& center,
    const zVEC2& size, zTQuadMarkEffectParams* params)
{
    if (!connectedVob || !connectedVob->homeWorld || !worldPoly)
        return;
    alphaFadeSpeed = params ? params->alphaFadeSpeed : 0;
    zVEC3 firstAxis;
    zVEC3 secondAxis;
    CalcQuadAxis(worldPoly, firstAxis, secondAxis);
    for (int axis = 0; axis < 3; ++axis) {
        firstAxis.n[axis] *= size.n[0];
        secondAxis.n[axis] *= size.n[1];
    }
    InitFirstQuadPoly(worldPoly, center, firstAxis, secondAxis, params);
    worldPolyList = new zCPolygon*[1];
    worldPolyList[0] = worldPoly;
    numWorldPoly = 1;
    visitedPolyList.numInArray = 0;
    visitedPolyList.InsertEnd(worldPoly);
    ProcessWorldPoly(worldPoly);
    delete[] worldPolyList;
    worldPolyList = 0;
    numWorldPoly = 0;
}

int zCQuadMark::GetNumActive()
{
    if (alphaFadeSpeed != 0)
        return quadMarkAniTracker.numInArray;
    return quadMesh->numPoly;
}

void zCQuadMark::ProcessAniTracker()
{
    const float fade = ztimer.frameTimeFloat * alphaFadeSpeed;
    for (int i = 0; i < quadMarkAniTracker.numInArray; ++i) {
        zTQuadMarkAniState& state = quadMarkAniTracker.parray[i];
        state.scaleCur *= 1.0f - ztimer.frameTimeFloat * state.scaleSpeedCur * 0.001f;
        for (int j = state.firstPolyIndex; j <= state.lastPolyIndex && j < quadMesh->numPoly; ++j) {
            zCPolygon* polygon = quadMesh->polyList[j];
            for (int k = 0; k < polygon->polyNumVert; ++k) {
                zCVertFeature* feature = polygon->feature[k];
                if (state.texAniType) {
                    feature->mapping.n[0] = state.uvDelta.n[0] +
                        (feature->mapping.n[0] - state.uvDelta.n[0]) * state.scaleCur;
                    feature->mapping.n[1] = state.uvDelta.n[1] +
                        (feature->mapping.n[1] - state.uvDelta.n[1]) * state.scaleCur;
                }
                const int alpha = state.alpha > 0 ? static_cast<int>(state.alpha) : 0;
                feature->lightStat[3] = alpha;
                feature->lightDyn[3] = alpha;
            }
        }
        state.alpha -= fade;
        if (state.alpha <= 0 || state.scaleCur <= 0) {
            quadMarkAniTracker.RemoveOrderIndex(i);
            --i;
        }
    }
    if (alphaFadeSpeed != 0 && quadMarkAniTracker.numInArray <= 0) {
        for (int i = 0; i < quadMesh->numPoly; ++i)
            delete quadMesh->polyList[i];
        delete[] quadMesh->polyList;
        delete[] quadMesh->vertexList;
        quadMesh->polyList = 0;
        quadMesh->vertexList = 0;
        quadMesh->numPoly = 0;
        quadMesh->numVerts = 0;
    }
}

zCClassDef* zCPolyStrip::_GetClassDef() const
{
    return classDef;
}

zSTRING zCPolyStrip::GetVisualName()
{
    return zSTRING("");
}
