#ifndef ZVERTEX_H
#define ZVERTEX_H

#include "z3d.h"

class Vertex;

class Triangle {
public:

    Triangle(Vertex* first, Vertex* second, Vertex* third, const zVEC2& firstUv,
        const zVEC2& secondUv, const zVEC2& thirdUv, const zVEC3& firstNormal,
        const zVEC3& secondNormal, const zVEC3& thirdNormal, int materialIndex, int flags);
    ~Triangle();

    int   HasVertex(Vertex* vertex) const;
    int   HasVertexIndex(Vertex* vertex) const;
    int   ComputeNormal();
    float GetArea();
    void  ReplaceVertex(Vertex* oldVertex, Vertex* newVertex);

    Vertex* vertices[3];
    zVEC2   texCoord[3];
    zVEC3   normals[3];
    zVEC3   normal;
    int     materialIndex;
    int     flags;

    static zCArray<Triangle*> triangleList;
};

class Vertex {
public:

    Vertex(const zVEC3& position, int index);
    ~Vertex();

    void CalcQuadric();
    int  IsBorder();
    void RemoveIfNotAdjacent(Vertex* other);

    int index;
    zVEC3 position;
    Triangle** triangles;
    int numTriangles;
    Vertex** adjacentVertices;
    int numAdjacentVertices;

    static zCArray<Vertex*> vertexList;
};

#endif
