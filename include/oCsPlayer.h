#ifndef OCSPLAYER_H
#define OCSPLAYER_H

#include "oGame.h"

class oCCSPlayer : public zCCSPlayer {
public:

    oCCSPlayer();
    virtual ~oCCSPlayer();

    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void ProcessCutscene(zCCSCutsceneContext* context, const zVEC3& position);

    static zCClassDef* classDef;
};

#endif
