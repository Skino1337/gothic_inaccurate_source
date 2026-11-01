#include "include/zModel.h"
#include "include/z3d.h"
#include "include/zVob.h"
#include "include/zMemory.h"

void D_Reset()
{
}

void D_Print(const zVEC3&, const zSTRING&, int)
{
}

void D_Print(const float&, const zSTRING&, int)
{
}

void D_Print(zSTRING&, int)
{
}

void D_Print(char*, int)
{
}

void D_Print(char*, int, int)
{
}

void D_PrintA(char*, int, int)
{
}

void D_Print3D(const zVEC3&, zSTRING&, const zCOLOR&)
{
}

void D_DrawPolyWire(const zCPolygon&, const zCOLOR&)
{
}

void D_ClrWin()
{
}

class zCVobCallback_PrintBigBBox : public zCVobCallback {
public:

    virtual void HandleVob(zCVob* vob, void* data);
};

void zCVobCallback_PrintBigBBox::HandleVob(zCVob* vob, void*)
{
    if (dynamic_cast<zCZone*>(vob))
        return;
    zCVobLight* light = dynamic_cast<zCVobLight*>(vob);
    if (light && light->lightData.isStatic)
        return;
    int bigBBox = 0;
    for (int axis = 0; axis < 3; ++axis) {
        if (vob->bbox3D.maxs.n[axis] - vob->bbox3D.mins.n[axis] > 5000.0f)
            bigBBox = 1;
    }
    if (!bigBBox)
        return;
    zSTRING visualDescription("-none-");
    if (vob->visual) {
        visualDescription = vob->visual->_GetClassDef()->className + ":" +
            vob->visual->GetVisualName();
    }
    zerr->Message("D: Name: " + vob->GetObjectName() + ", class: " +
        vob->_GetClassDef()->className);
    zerr->Message("D: objname: " + vob->GetObjectName());
    zerr->Message("D:   visual: " + visualDescription);
    zerr->Message("D:   bbox  : " + vob->bbox3D.GetDescription());
    const zVEC3 position = vob->trafoObjToWorld.GetTranslation();
    zSTRING line;
    line.Sprintf("D:   pos: X: %f Y: %f Z: %f", position.n[0], position.n[1], position.n[2]);
    zerr->Message(line);
    if (light) {
        line.Sprintf("D:   lightRange : %f", light->lightData.range);
        zerr->Message(line);
        zerr->Message("D:   preset: " + light->lightPresetInUse);
        zerr->Message("D:   canMove:" + zSTRING(light->lightData.m_bCanMove));
    }
}

class zCShowMemCallback : public zCWorldPerFrameCallback {
public:

    zCShowMemCallback() : oldNumObjects(0), oldNumObjectBytes(0)
    {
        oldStats = zTMallocStats();
    }

    virtual void DoWorldPerFrameCallback(zCWorld* world, zCCamera* camera);

    zTMallocStats oldStats;
    int           oldNumObjects;
    int           oldNumObjectBytes;
};

void zCShowMemCallback::DoWorldPerFrameCallback(zCWorld*, zCCamera*)
{
    zTMallocStats stats;
    zmalloc.GetStats(stats);
    int numObjects;
    int numObjectBytes;
    zCObject::GetMemStats(numObjects, numObjectBytes, 0);
    static const char* labels[10] = {
        "numBlocks", "numBlocksPeak", "numBytesAllocated", "numBytesAllocatedPeak",
        "numAllocs", "numFrees", "numBytesAllocatedTotal", "numBytesFreedTotal",
        "numzCObject", "numzCObjectBytes"
    };
    const long long values[10] = {
        stats.numBlocks, stats.numBlocksPeak, stats.numBytesAllocated, stats.numBytesAllocatedPeak,
        stats.numAllocs, stats.numFrees, stats.numBytesAllocatedTotal, stats.numBytesFreedTotal,
        numObjects, numObjectBytes
    };
    const long long deltas[10] = {
        stats.numBlocks - oldStats.numBlocks, stats.numBlocksPeak - oldStats.numBlocksPeak,
        stats.numBytesAllocated - oldStats.numBytesAllocated,
        stats.numBytesAllocatedPeak - oldStats.numBytesAllocatedPeak,
        stats.numAllocs - oldStats.numAllocs, stats.numFrees - oldStats.numFrees,
        stats.numBytesAllocatedTotal - oldStats.numBytesAllocatedTotal,
        stats.numBytesFreedTotal - oldStats.numBytesFreedTotal,
        numObjects - oldNumObjects, numObjectBytes - oldNumObjectBytes
    };
    const int fontY = screen->FontY();
    for (int i = 0; i < 10; ++i) {
        screen->Print(2600, 5000 + i * fontY, zSTRING(labels[i]));
        zSTRING line;
        line.Sprintf("%lld", values[i]);
        screen->Print(5600, 5000 + i * fontY, line);
        line.Sprintf("%lld", deltas[i]);
        screen->Print(7200, 5000 + i * fontY, line);
    }
    screen->Print(2600, 5000 + 10 * fontY * 2, zSTRING("assigned mem"));
    zSTRING assigned;
    assigned.Sprintf("%d", stats.numBytesAllocated);
    screen->Print(5600, 5000 + 10 * fontY * 2, assigned);
    oldStats = stats;
    oldNumObjects = numObjects;
    oldNumObjectBytes = numObjectBytes;
}
