#ifndef OMENU_MAIN_H
#define OMENU_MAIN_H

#include "zMenu.h"

class oCMenu_Main : public zCMenu {
public:

    oCMenu_Main(const zSTRING& name) : zCMenu(name) {}
    virtual ~oCMenu_Main() {}
};

#endif
