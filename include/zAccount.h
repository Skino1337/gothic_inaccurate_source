#ifndef ZACCOUNT_H
#define ZACCOUNT_H

#include "zObject.h"
#include "zString.h"

struct zTIndexA {
    ~zTIndexA();

    zSTRING name;
    int count;
};

struct zTIndexB {
    ~zTIndexB();

    zSTRING name;
    int count;
};

class zCAccountInfo {
public:

    ~zCAccountInfo();
    void Add(const char* file, int line, int count);
    void SumUp();

    zCArray<zTIndexA*> files;
    zCArray<zTIndexB*> lines;
    int total;
};

class zCAccount {
public:

    static void Add(const void* owner, const char* file, int line, int count);
    static void Sum(const void* owner);
    static void Clear(const void* owner);
    static void Begin();
    static void End();
    static int IsActive();

    static int s_actCounter;
};

void zAccountInit();
int Hash(const zSTRING& value);

#endif
