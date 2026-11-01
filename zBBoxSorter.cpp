#include "include/zVob.h"

// EXE: 0x0061FF80
zCBBox3DSorterBase::zTBoxSortHandle::zTBoxSortHandle()
    : mySorter(NULL)
{
    bbox3D.mins = zVEC3(0.0f, 0.0f, 0.0f);
    bbox3D.maxs = zVEC3(0.0f, 0.0f, 0.0f);
    for (int dimension = 0; dimension < 3; ++dimension) {
        indexBegin[dimension] = 0;
        indexEnd[dimension] = 0;
    }
}

// EXE: 0x00620010
void zCBBox3DSorterBase::zTBoxSortHandle::ClearActive()
{
}

// EXE: 0x006288E0
const zTBBox3D& zCBBox3DSorterBase::GetBBox(const zTNode* node)
{
    return static_cast<zCZone*>(node->data)->bbox3D;
}

// EXE: 0x0063B2B0
void zCBBox3DSorterBase::Clear()
{
    for (int dimension = 0; dimension < 3; ++dimension) {
        for (int i = 0; i < nodeList[dimension].numInArray; ++i)
            delete nodeList[dimension].parray[i];
        nodeList[dimension].DeleteList();
    }
    sorted = 0;
    for (int i = 0; i < handles.numInArray; ++i)
        handles.parray[i]->ClearActive();
}

// EXE: 0x0063B330
void zCBBox3DSorterBase::AllocAbs(int count)
{
    for (int dimension = 0; dimension < 3; ++dimension) {
        if (nodeList[dimension].numAlloc < count)
            nodeList[dimension].AllocDelta(count - nodeList[dimension].numAlloc);
    }
}

// EXE: 0x0063B3B0
void zCBBox3DSorterBase::Insert(void* data)
{
    sorted = 0;
    for (int dimension = 0; dimension < 3; ++dimension) {
        zTNode* begin = new zTNode;
        begin->isBegin = 1;
        begin->data = data;
        nodeList[dimension].InsertEnd(begin);
        zTNode* end = new zTNode;
        end->isBegin = 0;
        end->data = data;
        nodeList[dimension].InsertEnd(end);
    }
    zTNode node;
    node.isBegin = 1;
    node.data = data;
    const zTBBox3D& box = GetBBox(&node);
    for (int i = 0; i < handles.numInArray; ++i) {
        zTBoxSortHandle* handle = handles.parray[i];
        int intersects = 1;
        for (int axis = 0; axis < 3; ++axis) {
            if (box.maxs.n[axis] < handle->bbox3D.mins.n[axis] ||
                box.mins.n[axis] > handle->bbox3D.maxs.n[axis])
                intersects = 0;
        }
        if (intersects)
            handle->AddActive(data);
    }
}

// EXE: 0x0063B510
void zCBBox3DSorterBase::Remove(void* data)
{
    sorted = 0;
    for (int dimension = 0; dimension < 3; ++dimension) {
        for (int i = 0; i < nodeList[dimension].numInArray;) {
            zTNode* node = nodeList[dimension].parray[i];
            if (node->data != data) {
                ++i;
                continue;
            }
            delete node;
            --nodeList[dimension].numInArray;
            for (int j = i; j < nodeList[dimension].numInArray; ++j)
                nodeList[dimension].parray[j] = nodeList[dimension].parray[j + 1];
        }
    }
    for (int i = 0; i < handles.numInArray; ++i)
        handles.parray[i]->DelActive(data);
}

// EXE: 0x0063BD30
void zCBBox3DSorterBase::Sort()
{
    if (sorted)
        return;
    s_currentSorter = this;
    sorted = 1;
    for (int dimension = 0; dimension < 3; ++dimension) {
        s_currentDimension = dimension;
        nodeList[dimension].QuickSort();
    }
}

// EXE: 0x0063B5D0
void zCBBox3DSorterBase::GetActiveList(const zTBBox3D& bbox, zTBoxSortHandle& handle)
{
    handle.bbox3D = bbox;
    handle.ClearActive();
    Sort();
    zCArraySort<void*> added;
    for (int dimension = 0; dimension < 3; ++dimension) {
        handle.indexBegin[dimension] = 0;
        handle.indexEnd[dimension] = nodeList[dimension].numInArray - 1;
    }
    for (int i = 0; i < nodeList[0].numInArray; ++i) {
        zTNode* node = nodeList[0].parray[i];
        if (added.IsInList(node->data))
            continue;
        const zTBBox3D& box = GetBBox(node);
        int intersects = 1;
        for (int axis = 0; axis < 3; ++axis) {
            if (box.maxs.n[axis] < bbox.mins.n[axis] || box.mins.n[axis] > bbox.maxs.n[axis])
                intersects = 0;
        }
        if (intersects) {
            handle.AddActive(node->data);
            added.InsertEnd(node->data);
        }
    }
}

// EXE: 0x0063B920
void zCBBox3DSorterBase::UpdateActiveList(const zTBBox3D& bbox, zTBoxSortHandle& handle)
{
    GetActiveList(bbox, handle);
}

// EXE: 0x0063BC00
void zCBBox3DSorterBase::InsertHandle(zTBoxSortHandle& handle)
{
    handle.mySorter = this;
    handles.InsertEnd(&handle);
    GetActiveList(handle.bbox3D, handle);
}

// EXE: 0x0063BCE0
void zCBBox3DSorterBase::RemoveHandle(zTBoxSortHandle& handle)
{
    for (int i = 0; i < handles.numInArray; ++i) {
        if (handles.parray[i] == &handle) {
            --handles.numInArray;
            if (i != handles.numInArray)
                handles.parray[i] = handles.parray[handles.numInArray];
            break;
        }
    }
    handle.mySorter = 0;
    handle.ClearActive();
}

// EXE: 0x0063BD70
void zCBBox3DSorterBase::AdjustSorting(zCArraySort<zTNode*>& nodes, int dimension)
{
    s_currentSorter = this;
    s_currentDimension = dimension;
    nodes.QuickSort();
    sorted = 1;
}

// EXE: 0x0063BEF0
void zCBBox3DSorterBase::Update(void* data)
{
    Remove(data);
    Insert(data);
}
