#include "zVertex.h"

#include <cmath>

// EXE: 0x0095C96C
zCArray<Vertex*> Vertex::vertexList;

// EXE: 0x005C0150
Vertex::Vertex(const zVEC3& value, int index)
    : index(index), position(value), triangles(0), numTriangles(0),
      adjacentVertices(0), numAdjacentVertices(0)
{
    vertexList.InsertEnd(this);
}

// EXE: 0x005C02C0
Vertex::~Vertex()
{
    for (int i = 0; i < numAdjacentVertices; ++i)
        adjacentVertices[i]->RemoveIfNotAdjacent(this);
    numAdjacentVertices = 0;
    for (int i = 0; i < vertexList.numInArray; ++i) {
        if (vertexList.parray[i] == this) {
            for (int j = i; j < vertexList.numInArray - 1; ++j)
                vertexList.parray[j] = vertexList.parray[j + 1];
            --vertexList.numInArray;
            break;
        }
    }
    delete[] adjacentVertices;
    delete[] triangles;
}

// EXE: 0x005C03D0
void Vertex::CalcQuadric()
{
}

// EXE: 0x005C03E0
int Vertex::IsBorder()
{
    for (int i = 0; i < numAdjacentVertices; ++i) {
        Vertex* adjacent = adjacentVertices[i];
        int sameDirection = 0;
        Triangle* previous = 0;
        for (int j = 0; j < numTriangles; ++j) {
            Triangle* triangle = triangles[j];
            if (!triangle->HasVertex(adjacent))
                continue;
            if (previous) {
                const float dot = previous->normal.n[0] * triangle->normal.n[0] +
                    previous->normal.n[1] * triangle->normal.n[1] +
                    previous->normal.n[2] * triangle->normal.n[2];
                if (dot <= -0.95f)
                    return 1;
            }
            ++sameDirection;
            previous = triangle;
        }
        if (sameDirection == 1)
            return 1;
    }
    return 0;
}

// EXE: 0x005C0490
void Vertex::RemoveIfNotAdjacent(Vertex* other)
{
    int index = -1;
    for (int i = 0; i < numAdjacentVertices; ++i) {
        if (adjacentVertices[i] == other) {
            index = i;
            break;
        }
    }
    if (index < 0)
        return;
    for (int i = 0; i < numTriangles; ++i) {
        if (triangles[i]->HasVertex(other))
            return;
    }
    for (int i = index; i < numAdjacentVertices - 1; ++i)
        adjacentVertices[i] = adjacentVertices[i + 1];
    --numAdjacentVertices;
}

// EXE: 0x0095C9BC
zCArray<Triangle*> Triangle::triangleList;

// EXE: inlined in 0x005C0530
static void AddUniqueVertex(Vertex* vertex, Vertex* adjacent)
{
    for (int i = 0; i < vertex->numAdjacentVertices; ++i) {
        if (vertex->adjacentVertices[i] == adjacent)
            return;
    }
    vertex->adjacentVertices.InsertEnd(adjacent);
}

// EXE: inlined in 0x005C0530
static void AddUniqueTriangle(Vertex* vertex, Triangle* triangle)
{
    for (int i = 0; i < vertex->numTriangles; ++i) {
        if (vertex->triangles[i] == triangle)
            return;
    }
    vertex->triangles.InsertEnd(triangle);
}

// EXE: inlined in 0x005C0830
static void RemoveTriangle(Vertex* vertex, Triangle* triangle)
{
    for (int i = 0; i < vertex->numTriangles; ++i) {
        if (vertex->triangles[i] != triangle)
            continue;
        for (int j = i; j < vertex->numTriangles - 1; ++j)
            vertex->triangles[j] = vertex->triangles[j + 1];
        --vertex->numTriangles;
        return;
    }
}

// EXE: 0x005C0530
Triangle::Triangle(Vertex* first, Vertex* second, Vertex* third, const zVEC2& firstUv,
    const zVEC2& secondUv, const zVEC2& thirdUv, const zVEC3& firstNormal,
    const zVEC3& secondNormal, const zVEC3& thirdNormal, int materialIndex, int flags)
    : materialIndex(materialIndex), flags(flags)
{
    vertices[0] = first;
    vertices[1] = second;
    vertices[2] = third;
    texCoord[0] = firstUv;
    texCoord[1] = secondUv;
    texCoord[2] = thirdUv;
    normals[0] = firstNormal;
    normals[1] = secondNormal;
    normals[2] = thirdNormal;
    ComputeNormal();
    triangleList.InsertEnd(this);
    for (int i = 0; i < 3; ++i) {
        AddUniqueTriangle(vertices[i], this);
        for (int j = 0; j < 3; ++j) {
            if (i != j)
                AddUniqueVertex(vertices[i], vertices[j]);
        }
    }
}

// EXE: 0x005C0830
Triangle::~Triangle()
{
    for (int i = 0; i < triangleList.numInArray; ++i) {
        if (triangleList.parray[i] == this) {
            for (int j = i; j < triangleList.numInArray - 1; ++j)
                triangleList.parray[j] = triangleList.parray[j + 1];
            --triangleList.numInArray;
            break;
        }
    }
    for (int i = 0; i < 3; ++i) {
        RemoveTriangle(vertices[i], this);
        for (int j = 0; j < 3; ++j) {
            if (i != j)
                vertices[i]->RemoveIfNotAdjacent(vertices[j]);
        }
    }
}

// EXE: 0x005C0A30
int Triangle::HasVertex(Vertex* vertex) const
{
    return vertices[0] == vertex || vertices[1] == vertex || vertices[2] == vertex;
}

// EXE: 0x005C0A50
int Triangle::HasVertexIndex(Vertex* vertex) const
{
    if (vertices[0] == vertex)
        return 0;
    if (vertices[1] == vertex)
        return 1;
    if (vertices[2] == vertex)
        return 2;
    return -1;
}

// EXE: 0x005C0A80
int Triangle::ComputeNormal()
{
    zVEC3 first;
    zVEC3 second;
    for (int axis = 0; axis < 3; ++axis) {
        first.n[axis] = vertices[1]->position.n[axis] - vertices[0]->position.n[axis];
        second.n[axis] = vertices[2]->position.n[axis] - vertices[1]->position.n[axis];
    }
    normal.n[0] = first.n[1] * second.n[2] - first.n[2] * second.n[1];
    normal.n[1] = first.n[2] * second.n[0] - first.n[0] * second.n[2];
    normal.n[2] = first.n[0] * second.n[1] - first.n[1] * second.n[0];
    const float length = std::sqrt(normal.n[0] * normal.n[0] +
        normal.n[1] * normal.n[1] + normal.n[2] * normal.n[2]);
    if (length == 0)
        return 0;
    for (int axis = 0; axis < 3; ++axis)
        normal.n[axis] /= length;
    return 1;
}

// EXE: 0x005C0BC0
float Triangle::GetArea()
{
    zVEC3 sum;
    sum.n[0] = 0;
    sum.n[1] = 0;
    sum.n[2] = 0;
    for (int i = 0; i < 3; ++i) {
        const int next = (i + 1) % 3;
        sum.n[0] += vertices[i]->position.n[1] * vertices[next]->position.n[2] -
            vertices[i]->position.n[2] * vertices[next]->position.n[1];
        sum.n[1] += vertices[i]->position.n[2] * vertices[next]->position.n[0] -
            vertices[next]->position.n[2] * vertices[i]->position.n[0];
        sum.n[2] += vertices[next]->position.n[1] * vertices[i]->position.n[0] -
            vertices[i]->position.n[1] * vertices[next]->position.n[0];
    }
    return std::fabs(sum.n[0] * normal.n[0] + sum.n[1] * normal.n[1] +
        sum.n[2] * normal.n[2]) * 0.5f;
}

// EXE: 0x005C0C60
void Triangle::ReplaceVertex(Vertex* oldVertex, Vertex* newVertex)
{
    const int index = HasVertexIndex(oldVertex);
    if (index >= 0)
        vertices[index] = newVertex;
    RemoveTriangle(oldVertex, this);
    AddUniqueTriangle(newVertex, this);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (i != j)
                AddUniqueVertex(vertices[i], vertices[j]);
        }
    }
    ComputeNormal();
}
