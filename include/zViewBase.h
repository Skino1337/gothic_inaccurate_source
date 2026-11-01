#ifndef ZVIEWBASE_H
#define ZVIEWBASE_H

#include "zSky.h"

class zCViewBase {
public:

    zCViewBase();
    virtual ~zCViewBase();
    virtual int anx(int x);
    virtual int any(int y);
    virtual int nax(int x);
    virtual int nay(int y);
    virtual int ClipLine(int& x1, int& y1, int& x2, int& y2);
    virtual void Line(int x1, int y1, int x2, int y2, const zCOLOR& color);
    virtual void GetViewport(int& x1, int& y1, int& x2, int& y2);
    virtual void FillZBuffer();
    virtual int GetCode(int x, int y);

    static const unsigned short ViewVirtualWidth;
    static const unsigned short ViewVirtualHeight;
};

#endif
