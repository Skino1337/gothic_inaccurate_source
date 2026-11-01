#ifndef ZRAYCACHE_H
#define ZRAYCACHE_H

#include "zPluecker.h"

class zCBspBase {
public:

    void CollectVobsInBBox3D(zCArray<zCVob*>& vobs, const zTBBox3D& bbox);

    int         numPolys;
    zTBBox3D    bbox3D;
    zCPolygon** polyList;
};

class zCBspLeaf : public zCBspBase {
public:

    void PolyPlueckerAddRef();
    void PolyPlueckerRelease();
};

class zCRayCacheElement : public zCPluecker {
public:

    zCRayCacheElement(const zCBspLeaf* leaf, zCRayCacheElement** first);
    ~zCRayCacheElement();

    void JumpBack(zCRayCacheElement** first);

    zCBspLeaf*          leaf;
    float*              updatePolys;
    float               min_updatePolys;
    char*               hit;
    zCArray<unsigned short> hit_list;
    zCRayCacheElement*  next;
};

class zCRayCache {
public:

    zCRayCache();
    ~zCRayCache();

    char*                    Hit() const;
    zCArray<unsigned short>& HitList() const;
    void                     StartTraceRay(const zVEC3& start, const zVEC3& stop);
    void                     StopTraceRay();
    void                     NextNode(const zCBspBase* node);

    static float LastTraceRayEffort();
    static void  NodeDeleted(zCBspLeaf* leaf);

    zCRayCacheElement* first;
    zCRayCache*        next;

    static zCRayCacheElement* s_current;
    static zVEC3              s_start;
    static zVEC3              s_stop;
    static zCRayCache*        s_anchor;
};

#endif
