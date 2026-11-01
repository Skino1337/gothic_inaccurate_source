#include "include/zMenu.h"

static int viewWindowState = -1;

zCViewWindow::zCViewWindow(int x1, int y1, int x2, int y2, int viewID)
    : zCView(x1, y1, x2, y2, viewID)
{
    ClrFlags(0x200);
}

zCViewWindow::~zCViewWindow()
{
}
