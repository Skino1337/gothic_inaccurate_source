#ifndef ZAI_H
#define ZAI_H

#include "oDialog.h"
#include "zPhysics.h"

class zCAIBaseSound : public zCAIBase {
public:

    zCAIBaseSound() : slideSoundHandle(0), slideSoundOn(0) {}

    void RemoveSlideSound();
    void CheckSlideSound(zCVob* vob);
    void StartDefaultCollisionSound(zCVob* vob, const zCCollisionReport& report);

    int slideSoundHandle;
    int slideSoundOn;
};

#endif
