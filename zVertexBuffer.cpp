#include "include/zVertexBuffer.h"

static int vertexBufferState = -1;
static zCClassDef zCVertexBufferClassDef("zCVertexBuffer", "zCObject", NULL, 0, 0x68, 0, "");
zCClassDef* zCVertexBuffer::classDef = &zCVertexBufferClassDef;
zCVertexBufferManager zvertexBufferMan;
unsigned long zCVertexBuffer::s_classCtorCtr;

zCVertexBuffer::zCVertexBuffer()
    : numVertex(0), arrayStride(0), array(), numVertsFilled(0), createFlags(0),
      vertexBufferID(s_classCtorCtr++)
{
}

zCVertexBuffer::~zCVertexBuffer()
{
    numVertex = 0;
    arrayStride = 0;
    array = zTVertexArray();
}

zCClassDef* zCVertexBuffer::_GetClassDef() const
{
    return classDef;
}

void zCVertexBuffer::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

int zCVertexBuffer::Create(unsigned long, unsigned long, unsigned long)
{
    return 0;
}

int zCVertexBuffer::Destroy()
{
    return 0;
}

int zCVertexBuffer::Lock(unsigned long)
{
    return 0;
}

int zCVertexBuffer::Unlock()
{
    return 0;
}

int zCVertexBuffer::IsLocked()
{
    return 0;
}

int zCVertexBuffer::Optimize()
{
    return 0;
}

int zCVertexBuffer::SetPrimitiveType(zTVBufferPrimitiveType)
{
    return 0;
}

zTVBufferPrimitiveType zCVertexBuffer::GetPrimitiveType()
{
    return zVBUFFER_PT_TRIANGLELIST;
}

unsigned long zCVertexBuffer::GetVertexFormat()
{
    return 0;
}

zTVBufferVertexType zCVertexBuffer::GetVertexType()
{
    return zVBUFFER_VERTTYPE_UT_UL;
}

int zCVertexBuffer::SetIndexList(const zCArray<unsigned short>&)
{
    return 0;
}

int zCVertexBuffer::SetIndexListSize(unsigned long)
{
    return 0;
}

unsigned long zCVertexBuffer::GetIndexListSize()
{
    return 0;
}

unsigned short* zCVertexBuffer::GetIndexListPtr()
{
    return 0;
}

void zCVertexBuffer::AddRecreateLostVBCallback(int(*callback)(zCVertexBuffer*, void*), void* data)
{
    for (int i = 0; i < callbackList.numInArray; ++i) {
        if (callbackList.parray[i].recreateLostVBCallback == callback &&
            callbackList.parray[i].recreateLostVBCallbackData == data)
            return;
    }
    zTCallbackData callbackData = { callback, data };
    callbackList.InsertEnd(callbackData);
}

void zCVertexBuffer::RemoveRecreateLostVBCallback(int(*callback)(zCVertexBuffer*, void*), void* data)
{
    for (int i = 0; i < callbackList.numInArray; ++i) {
        if (callbackList.parray[i].recreateLostVBCallback == callback &&
            callbackList.parray[i].recreateLostVBCallbackData == data) {
            callbackList.parray[i] = callbackList.parray[callbackList.numInArray - 1];
            --callbackList.numInArray;
            return;
        }
    }
}

int zCVertexBuffer::CallRecreateLostVBCallbacks()
{
    Unlock();
    Destroy();
    Create(GetVertexFormat(), numVertex, createFlags);
    int result = 1;
    for (int i = 0; i < callbackList.numInArray; ++i) {
        zTCallbackData& callback = callbackList.parray[i];
        if (callback.recreateLostVBCallback &&
            !callback.recreateLostVBCallback(this, callback.recreateLostVBCallbackData))
            result = 0;
    }
    return result;
}

zCVertexBufferDyn::zCVertexBufferDyn(unsigned long format, unsigned long numVertices)
    : vertexFormat(format), numVert(numVertices), vertexSizeBytes(0), currentPos(0),
      locked(0), flush(1)
{
    vertexBuffer = zrenderer->CreateVertexBuffer();
    vertexBuffer->Create(format, numVertices, 8);
}

zCVertexBufferDyn::~zCVertexBufferDyn()
{
    if (locked)
        vertexBuffer->Unlock();
    if (vertexBuffer)
        vertexBuffer->Release();
    vertexBuffer = 0;
}

void* zCVertexBufferDyn::Lock(unsigned long numVertices, unsigned long& startVertex)
{
    startVertex = 0;
    if (numVertices > vertexBuffer->numVertex)
        return 0;
    unsigned long lockFlags = 0x68;
    if (flush || currentPos + numVertices >= vertexBuffer->numVertex) {
        flush = 0;
        currentPos = 0;
        lockFlags = 0x58;
    }
    unsigned char* basePtr = vertexBuffer->array.basePtr;
    if (vertexBuffer->Lock(lockFlags)) {
        locked = 1;
        startVertex = currentPos;
        currentPos += numVertices;
        vertexSizeBytes = vertexBuffer->arrayStride;
    }
    return basePtr + vertexSizeBytes * startVertex;
}

void zCVertexBufferDyn::Unlock()
{
    if (locked) {
        vertexBuffer->Unlock();
        locked = 0;
    }
}

zCVertexBufferDyn2::zCVertexBufferDyn2(unsigned long format, unsigned long numVertices)
    : vertexFormat(format), numVert(numVertices), vertexSizeBytes(0), currentPos(0),
      locked(0), flush(1)
{
    vertexBuffer = zrenderer->CreateVertexBuffer();
    vertexBuffer->Create(format, numVertices, 8);
}

zCVertexBufferDyn2::~zCVertexBufferDyn2()
{
    if (locked) {
        vertexBuffer->Unlock();
        if (currentPos >= numVert)
            flush = 1;
    }
    if (vertexBuffer)
        vertexBuffer->Release();
    vertexBuffer = 0;
}

void zCVertexBufferDyn2::ReserveVerts(unsigned long numVertices)
{
    if (currentPos + numVertices > numVert)
        flush = 1;
}

void* zCVertexBufferDyn2::Lock(unsigned long& startVertex)
{
    startVertex = 0;
    unsigned long lockFlags = 0x68;
    if (flush) {
        flush = 0;
        currentPos = 0;
        lockFlags = 0x58;
    }
    unsigned char* basePtr = vertexBuffer->array.basePtr;
    if (vertexBuffer->Lock(lockFlags)) {
        locked = 1;
        startVertex = currentPos;
        vertexSizeBytes = vertexBuffer->arrayStride;
    }
    return basePtr + vertexSizeBytes * startVertex;
}

void zCVertexBufferDyn2::Unlock(unsigned long numVertices)
{
    if (!locked)
        return;
    vertexBuffer->Unlock();
    locked = 0;
    currentPos += numVertices;
    if (currentPos >= numVert)
        flush = 1;
}

zCVertexBufferManager::zCVertexBufferManager()
    : changeWorldHintSet(0)
{
}

zCVertexBufferManager::~zCVertexBufferManager()
{
    openVBList.DeleteList();
    closedVBList.DeleteList();
}

zCVertexBuffer* zCVertexBufferManager::AcquireVertexBuffer(unsigned long format,
    unsigned long numVertices, unsigned long createFlags, unsigned long lockFlags, int& startVertex)
{
    for (int i = 0; i < openVBList.numInArray; ++i) {
        zCVertexBuffer* buffer = openVBList.parray[i];
        if (buffer->GetVertexFormat() == format && buffer->numVertex - buffer->numVertsFilled >= numVertices) {
            startVertex = buffer->numVertsFilled;
            buffer->numVertsFilled += numVertices;
            buffer->AddRef();
            return buffer;
        }
    }
    zCVertexBuffer* buffer = zrenderer->CreateVertexBuffer();
    unsigned long capacity = numVertices;
    if (changeWorldHintSet && capacity < 2048)
        capacity = 2048;
    buffer->Create(format, capacity, createFlags);
    buffer->Lock(lockFlags);
    buffer->numVertsFilled += numVertices;
    openVBList.InsertEnd(buffer);
    startVertex = 0;
    return buffer;
}

int zCVertexBufferManager::ReleaseVertexBuffer(zCVertexBuffer* vertexBuffer)
{
    for (int i = 0; i < closedVBList.numInArray; ++i) {
        if (closedVBList.parray[i] == vertexBuffer) {
            if (vertexBuffer->refCtr == 1) {
                closedVBList.parray[i] = closedVBList.parray[closedVBList.numInArray - 1];
                --closedVBList.numInArray;
            }
            vertexBuffer->Release();
            return 1;
        }
    }
    for (int i = 0; i < openVBList.numInArray; ++i) {
        if (openVBList.parray[i] == vertexBuffer) {
            if (vertexBuffer->refCtr == 1) {
                openVBList.parray[i] = openVBList.parray[openVBList.numInArray - 1];
                --openVBList.numInArray;
            }
            vertexBuffer->Release();
            return 1;
        }
    }
    return 1;
}

void zCVertexBufferManager::UnlockOpenVertexBuffers()
{
    for (int i = 0; i < closedVBList.numInArray; ++i) {
        if (closedVBList.parray[i]->IsLocked()) {
            closedVBList.parray[i]->Unlock();
            closedVBList.parray[i]->Optimize();
        }
    }
    while (openVBList.numInArray) {
        zCVertexBuffer* buffer = openVBList.parray[0];
        buffer->Unlock();
        buffer->Optimize();
        closedVBList.InsertEnd(buffer);
        openVBList.parray[0] = openVBList.parray[openVBList.numInArray - 1];
        --openVBList.numInArray;
    }
}

void zCVertexBufferManager::StartChangeWorld()
{
    changeWorldHintSet = 1;
}

void zCVertexBufferManager::EndChangeWorld()
{
    changeWorldHintSet = 0;
    UnlockOpenVertexBuffers();
}
