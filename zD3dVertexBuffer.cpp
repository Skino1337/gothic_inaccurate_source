#include "include/zVertexBuffer.h"
#include <cstring>

static int d3dVertexBufferState = -1;
zCArray<zCVertexBuffer_D3D*> zCVertexBuffer_D3D::xvb_vertexbufferList(512);

zCVertexBuffer_D3D::zCVertexBuffer_D3D()
    : xvb_optimized(0), xvb_locked(0), xvb_vertexType(zVBUFFER_VERTTYPE_UT_UL),
      xvb_primitiveType(zVBUFFER_PT_TRIANGLELIST), xvb_vertexFormat(0), xvb_vertexBuffer(0)
{
    xvb_vertexbufferList.InsertEnd(this);
}

zCVertexBuffer_D3D::~zCVertexBuffer_D3D()
{
    Destroy();
    for (int i = 0; i < xvb_vertexbufferList.numInArray; ++i) {
        if (xvb_vertexbufferList.parray[i] == this) {
            xvb_vertexbufferList.parray[i] =
                xvb_vertexbufferList.parray[xvb_vertexbufferList.numInArray - 1];
            --xvb_vertexbufferList.numInArray;
            break;
        }
    }
}

static unsigned long D3DVertexStride(unsigned long format)
{
    unsigned long stride = 0;
    if (format & 1)
        stride += 12;
    if (format & 2)
        stride += 16;
    if (format & 4)
        stride += 12;
    if (format & 8)
        stride += 4;
    int textureCount = 0;
    if (format & 0x10)
        textureCount = 1;
    else if (format & 0x20)
        textureCount = 2;
    else if (format & 0x40)
        textureCount = 3;
    else if (format & 0x80)
        textureCount = 4;
    return stride + textureCount * 8;
}

int zCVertexBuffer_D3D::Create(unsigned long numVertices, unsigned long vertexFormat,
    unsigned long createFlags)
{
    if (xvb_vertexBuffer)
        return 0;
    xvb_vertexFormat = vertexFormat;
    this->numVertex = numVertices;
    this->createFlags = createFlags;
    numVertsFilled = 0;
    arrayStride = D3DVertexStride(vertexFormat);
    xvb_vertexBuffer = new unsigned char[numVertices * arrayStride];
    array.basePtr = xvb_vertexBuffer;
    if (vertexFormat & 2)
        xvb_vertexType = zVBUFFER_VERTTYPE_T_L;
    else if ((vertexFormat & 4) && !(vertexFormat & 8))
        xvb_vertexType = zVBUFFER_VERTTYPE_UT_UL;
    else
        xvb_vertexType = zVBUFFER_VERTTYPE_UT_L;
    return 1;
}

int zCVertexBuffer_D3D::Destroy()
{
    delete[] xvb_vertexBuffer;
    xvb_vertexBuffer = 0;
    array = zTVertexArray();
    xvb_locked = 0;
    xvb_optimized = 0;
    return 1;
}

int zCVertexBuffer_D3D::Lock(unsigned long)
{
    if (!xvb_vertexBuffer || xvb_optimized)
        return 0;
    if (xvb_locked)
        return 1;
    array = zTVertexArray();
    array.basePtr = xvb_vertexBuffer;
    unsigned char* current = xvb_vertexBuffer;
    if (xvb_vertexFormat & 1) {
        array.XYZPtr = reinterpret_cast<zVEC3*>(current);
        current += 12;
    }
    if (xvb_vertexFormat & 2) {
        array.XYZRHWPtr = reinterpret_cast<zVEC4*>(current);
        current += 16;
    }
    if (xvb_vertexFormat & 4) {
        array.normalPtr = reinterpret_cast<zVEC3*>(current);
        current += 12;
    }
    if (xvb_vertexFormat & 8) {
        array.colorPtr = reinterpret_cast<zCOLOR*>(current);
        current += 4;
    }
    int textureCount = 0;
    if (xvb_vertexFormat & 0x10)
        textureCount = 1;
    else if (xvb_vertexFormat & 0x20)
        textureCount = 2;
    else if (xvb_vertexFormat & 0x40)
        textureCount = 3;
    else if (xvb_vertexFormat & 0x80)
        textureCount = 4;
    for (int i = 0; i < textureCount; ++i) {
        array.texUVPtr[i] = reinterpret_cast<zVEC2*>(current);
        current += 8;
    }
    arrayStride = current - xvb_vertexBuffer;
    xvb_locked = 1;
    return 1;
}

int zCVertexBuffer_D3D::Unlock()
{
    if (!xvb_vertexBuffer)
        return 0;
    if (xvb_locked) {
        array = zTVertexArray();
        xvb_locked = 0;
    }
    return 1;
}

int zCVertexBuffer_D3D::IsLocked()
{
    return xvb_locked;
}

int zCVertexBuffer_D3D::Optimize()
{
    if (!xvb_vertexBuffer)
        return 0;
    if (!xvb_optimized) {
        xvb_optimized = 1;
        numVertsFilled = numVertex;
    }
    return 1;
}

int zCVertexBuffer_D3D::SetPrimitiveType(zTVBufferPrimitiveType type)
{
    xvb_primitiveType = type;
    return 1;
}

zTVBufferPrimitiveType zCVertexBuffer_D3D::GetPrimitiveType()
{
    return xvb_primitiveType;
}

unsigned long zCVertexBuffer_D3D::GetVertexFormat()
{
    return xvb_vertexFormat;
}

zTVBufferVertexType zCVertexBuffer_D3D::GetVertexType()
{
    return xvb_vertexType;
}

int zCVertexBuffer_D3D::SetIndexListSize(unsigned long size)
{
    xvb_indexList.AllocAbs(size);
    xvb_indexList.numInArray = xvb_indexList.numAlloc;
    return 1;
}

unsigned long zCVertexBuffer_D3D::GetIndexListSize()
{
    return xvb_indexList.numInArray;
}

unsigned short* zCVertexBuffer_D3D::GetIndexListPtr()
{
    return xvb_indexList.numInArray > 0 ? xvb_indexList.parray : 0;
}

int zCVertexBuffer_D3D::SetIndexList(const zCArray<unsigned short>& indices)
{
    xvb_indexList.numInArray = 0;
    xvb_indexList.AllocAbs(indices.numInArray);
    xvb_indexList.numInArray = indices.numInArray;
    for (int i = 0; i < indices.numInArray; ++i)
        xvb_indexList.parray[i] = indices.parray[i];
    return 1;
}
