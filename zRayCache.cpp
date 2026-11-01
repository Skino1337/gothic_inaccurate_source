#include "include/zRayCache.h"

static int rayCacheElementCount;
static int rayCacheRejected;
static int rayCacheUpdated;
static int rayCacheTotal;
static int rayCacheCasting;
static zCBspLeaf* rayCacheReleasedLeaves[10];
static int rayCacheReleasedLeafIndex;

zCRayCacheElement* zCRayCache::s_current;
zVEC3 zCRayCache::s_start;
zVEC3 zCRayCache::s_stop;
zCRayCache* zCRayCache::s_anchor;

zCRayCacheElement::zCRayCacheElement(const zCBspLeaf* leafValue, zCRayCacheElement** first)
    : leaf(const_cast<zCBspLeaf*>(leafValue)), min_updatePolys(-100.0f), next(*first)
{
    updatePolys = new float[leaf->numPolys];
    hit = new char[leaf->numPolys];
    for (int i = 0; i < leaf->numPolys; ++i) {
        updatePolys[i] = -1.0f;
        hit[i] = 0;
    }
    *first = this;
    ++rayCacheElementCount;
    leaf->PolyPlueckerAddRef();
}

zCRayCacheElement::~zCRayCacheElement()
{
    --rayCacheElementCount;
    delete next;
    delete[] updatePolys;
    delete[] hit;
    updatePolys = 0;
    hit = 0;
    next = 0;
    zCBspLeaf* previous = rayCacheReleasedLeaves[rayCacheReleasedLeafIndex];
    if (previous)
        previous->PolyPlueckerRelease();
    rayCacheReleasedLeaves[rayCacheReleasedLeafIndex] = leaf;
    rayCacheReleasedLeafIndex = (rayCacheReleasedLeafIndex + 1) % 10;
    leaf = 0;
}

void zCRayCacheElement::JumpBack(zCRayCacheElement** first)
{
    *first = next;
    next = next->next;
    (*first)->next = this;
}

char* zCRayCache::Hit() const
{
    return s_current->hit;
}

zCArray<unsigned short>& zCRayCache::HitList() const
{
    return s_current->hit_list;
}

float zCRayCache::LastTraceRayEffort()
{
    return static_cast<float>(rayCacheRejected + rayCacheUpdated) /
        (rayCacheTotal + rayCacheRejected + rayCacheUpdated + 1);
}

void zCRayCache::StartTraceRay(const zVEC3& start, const zVEC3& stop)
{
    rayCacheRejected = 0;
    rayCacheUpdated = 0;
    rayCacheTotal = 0;
    rayCacheCasting = 1;
    s_start = start;
    s_stop = stop;
    s_current = 0;
}

void zCRayCache::StopTraceRay()
{
    rayCacheCasting = 0;
    if (s_current) {
        zCRayCacheElement* next = s_current->next;
        s_current->next = 0;
        delete next;
        s_current = 0;
    }
}

zCRayCache::zCRayCache()
    : first(0), next(0)
{
    next = s_anchor;
    s_anchor = this;
}

zCRayCache::~zCRayCache()
{
    delete first;
    first = 0;
    s_current = 0;
    zCRayCache** cache = &s_anchor;
    while (*cache) {
        if (*cache == this)
            *cache = next;
        cache = &(*cache)->next;
    }
    if (!s_anchor) {
        for (int i = 0; i < 10; ++i) {
            if (rayCacheReleasedLeaves[i])
                rayCacheReleasedLeaves[i]->PolyPlueckerRelease();
            rayCacheReleasedLeaves[i] = 0;
        }
    }
}

void zCRayCache::NextNode(const zCBspBase* node)
{
    zCRayCacheElement** slot = s_current ? &s_current->next : &first;
    zCBspLeaf* leaf = static_cast<zCBspLeaf*>(const_cast<zCBspBase*>(node));
    if (*slot && (*slot)->leaf != leaf) {
        zCRayCacheElement* stale = *slot;
        *slot = stale->next;
        stale->next = 0;
        delete stale;
    }
    if (!*slot)
        new zCRayCacheElement(leaf, slot);
    s_current = *slot;
    s_current->hit_list.numInArray = 0;
    zVEC3 direction;
    for (int axis = 0; axis < 3; ++axis)
        direction.n[axis] = s_stop.n[axis] - s_start.n[axis];
    static_cast<zCPluecker&>(*s_current) = zCPluecker(s_start, direction);
    s_current->min_updatePolys = direction.n[0] * direction.n[0] +
        direction.n[1] * direction.n[1] + direction.n[2] * direction.n[2];
    for (int i = 0; i < node->numPolys; ++i) {
        zVEC3 intersection;
        float distance;
        const int hit = node->polyList[i]->CheckRayPolyIntersection2Sided(s_start, direction,
            intersection, distance);
        s_current->hit[i] = hit;
        if (hit) {
            s_current->hit_list.InsertEnd(i);
            ++rayCacheUpdated;
        } else
            ++rayCacheRejected;
        ++rayCacheTotal;
    }
}

float operator*(const zCPluecker& first, const zCPluecker& second)
{
    float result = 0;
    for (int i = 0; i < 3; ++i)
        result += first.dir[i] * second.pos[i] + first.pos[i] * second.dir[i];
    return result;
}

zCPluecker& zCPluecker::operator-=(const zCPluecker& other)
{
    for (int i = 0; i < 3; ++i) {
        dir[i] -= other.dir[i];
        pos[i] -= other.pos[i];
    }
    return *this;
}

void zCRayCache::NodeDeleted(zCBspLeaf* leaf)
{
    for (zCRayCache* cache = s_anchor; cache; cache = cache->next) {
        zCRayCacheElement** slot = &cache->first;
        while (*slot) {
            if ((*slot)->leaf == leaf) {
                zCRayCacheElement* element = *slot;
                *slot = element->next;
                element->next = 0;
                delete element;
            } else
                slot = &(*slot)->next;
        }
    }
    for (int i = 0; i < 10; ++i) {
        if (rayCacheReleasedLeaves[i] == leaf) {
            leaf->PolyPlueckerRelease();
            rayCacheReleasedLeaves[i] = 0;
        }
    }
}
