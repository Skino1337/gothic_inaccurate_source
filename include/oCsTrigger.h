#ifndef OCSTRIGGER_H
#define OCSTRIGGER_H

#include "zVob.h"

class oCCSTrigger : public zCTrigger {
public:

    virtual ~oCCSTrigger();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void TriggerTarget(zCVob* other);
    virtual void UntriggerTarget(zCVob* other);

    static zCClassDef* classDef;
};

#endif
