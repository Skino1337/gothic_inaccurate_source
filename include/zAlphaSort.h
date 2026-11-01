#ifndef ZALPHASORT_H
#define ZALPHASORT_H

#include "zRndD3d.h"
#include "zModel.h"

class zCRndAlphaSortObject_Vob : public zCRndAlphaSortObject {
public:

    zCRndAlphaSortObject_Vob();
    virtual ~zCRndAlphaSortObject_Vob();
    virtual void Draw(int index);

    static zCRndAlphaSortObject_Vob* GetFreeAlphaVob();
    static void ResetFreeList();
    static void ClearList();

    zCVob* vob;
    float  alpha;

    static zCRndAlphaSortObject_Vob s_alphaVobList[256];
    static int s_currentAlphaVob;
};

#endif
