#ifndef OHELP_H
#define OHELP_H

#include "zVob.h"

class zCGfx {
public:

    zCGfx(const zSTRING& fileName);
};

extern zCView* screen;

class oCHelp {
public:

    oCHelp();
    ~oCHelp();
    void CalcPos();
    void Toggle();
    void Update();
    void Redraw(zCVob* vob);

    zCView* win;
    int dx;
    int dy;
    int sx;
    int sy;
    int x[4];
    int y[4];
    int index[4];
    int led[2];
    int ledx[2];
    int ledy[2];
};

#endif
