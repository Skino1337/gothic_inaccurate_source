#ifndef ZVERTEXBUFFER_H
#define ZVERTEXBUFFER_H

#include "z3d.h"

enum zTVBufferPrimitiveType {
    zVBUFFER_PT_TRIANGLELIST,
    zVBUFFER_PT_TRIANGLESTRIP,
    zVBUFFER_PT_TRIANGLEFAN,
    zVBUFFER_PT_POINTLIST,
    zVBUFFER_PT_LINELIST,
    zVBUFFER_PT_LINESTRIP
};

enum zTVBufferVertexType {
    zVBUFFER_VERTTYPE_UT_UL,
    zVBUFFER_VERTTYPE_UT_L,
    zVBUFFER_VERTTYPE_T_L
};

class zCVertexBuffer : public zCObject {
public:

    struct zTCallbackData {
        int(*recreateLostVBCallback)(zCVertexBuffer*, void*);
        void* recreateLostVBCallbackData;
    };

    struct zTVertexArray {
        unsigned char* basePtr;
        zVEC3*         XYZPtr;
        zVEC4*         XYZRHWPtr;
        zVEC3*         normalPtr;
        zCOLOR*        colorPtr;
        zVEC2*         texUVPtr[4];
    };

    zCVertexBuffer();
    virtual ~zCVertexBuffer();
    static void operator delete(void* memory);

    void AddRecreateLostVBCallback(int(*callback)(zCVertexBuffer*, void*), void* data);
    void RemoveRecreateLostVBCallback(int(*callback)(zCVertexBuffer*, void*), void* data);
    int  CallRecreateLostVBCallbacks();

    zCClassDef* _GetClassDef() const;
    virtual int  Create(unsigned long numVertices, unsigned long vertexFormat,
        unsigned long createFlags);
    virtual int  Destroy();
    virtual int  Lock(unsigned long flags);
    virtual int  Unlock();
    virtual int  IsLocked();
    virtual int  Optimize();
    virtual int  SetPrimitiveType(zTVBufferPrimitiveType type);
    virtual zTVBufferPrimitiveType GetPrimitiveType();
    virtual unsigned long GetVertexFormat();
    virtual zTVBufferVertexType GetVertexType();
    virtual int  SetIndexList(const zCArray<unsigned short>& indices);
    virtual int  SetIndexListSize(unsigned long size);
    virtual unsigned long GetIndexListSize();
    virtual unsigned short* GetIndexListPtr();

    unsigned long         numVertex;
    unsigned long         arrayStride;
    zTVertexArray         array;
    int                   numVertsFilled;
    unsigned long         createFlags;
    zCArray<zTCallbackData> callbackList;
    unsigned long         vertexBufferID;

    static zCClassDef*   classDef;
    static unsigned long s_classCtorCtr;
};

class zCVertexBuffer_D3D : public zCVertexBuffer {
public:

    zCVertexBuffer_D3D();
    virtual ~zCVertexBuffer_D3D();
    virtual int Create(unsigned long numVertices, unsigned long vertexFormat,
        unsigned long createFlags);
    virtual int Destroy();
    virtual int Lock(unsigned long flags);
    virtual int Unlock();
    virtual int IsLocked();
    virtual int Optimize();
    virtual int SetPrimitiveType(zTVBufferPrimitiveType type);
    virtual zTVBufferPrimitiveType GetPrimitiveType();
    virtual unsigned long GetVertexFormat();
    virtual zTVBufferVertexType GetVertexType();
    virtual int SetIndexList(const zCArray<unsigned short>& indices);
    virtual int SetIndexListSize(unsigned long size);
    virtual unsigned long GetIndexListSize();
    virtual unsigned short* GetIndexListPtr();

    int xvb_optimized;
    int xvb_locked;
    zTVBufferVertexType xvb_vertexType;
    zTVBufferPrimitiveType xvb_primitiveType;
    unsigned long xvb_vertexFormat;
    unsigned char* xvb_vertexBuffer;
    zCArray<unsigned short> xvb_indexList;

    static zCArray<zCVertexBuffer_D3D*> xvb_vertexbufferList;
};

class zCVertexBufferDyn {
public:

    zCVertexBufferDyn(unsigned long vertexFormat, unsigned long numVertices);
    ~zCVertexBufferDyn();

    void* Lock(unsigned long numVertices, unsigned long& startVertex);
    void  Unlock();

    zCVertexBuffer* vertexBuffer;
    unsigned long   vertexFormat;
    unsigned long   numVert;
    unsigned long   vertexSizeBytes;
    unsigned long   currentPos;
    int             locked;
    int             flush;
};

class zCVertexBufferDyn2 {
public:

    zCVertexBufferDyn2(unsigned long vertexFormat, unsigned long numVertices);
    ~zCVertexBufferDyn2();

    void  ReserveVerts(unsigned long numVertices);
    void* Lock(unsigned long& startVertex);
    void  Unlock(unsigned long numVertices);

    zCVertexBuffer* vertexBuffer;
    unsigned long   vertexFormat;
    unsigned long   numVert;
    unsigned long   vertexSizeBytes;
    unsigned long   currentPos;
    int             locked;
    int             flush;
};

class zCVertexBufferManager {
public:

    zCVertexBufferManager();
    ~zCVertexBufferManager();

    zCVertexBuffer* AcquireVertexBuffer(unsigned long vertexFormat, unsigned long numVertices,
        unsigned long createFlags, unsigned long lockFlags, int& startVertex);
    int  ReleaseVertexBuffer(zCVertexBuffer* vertexBuffer);
    void UnlockOpenVertexBuffers();
    void StartChangeWorld();
    void EndChangeWorld();

    zCArray<zCVertexBuffer*> openVBList;
    zCArray<zCVertexBuffer*> closedVBList;
    int                      changeWorldHintSet;
};

#endif
