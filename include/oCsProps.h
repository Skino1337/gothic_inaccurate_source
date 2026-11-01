#ifndef OCSPROPS_H
#define OCSPROPS_H

#include "zVob.h"

class oCCSProps : public zCCSProps {
public:

    oCCSProps();
    virtual ~oCCSProps();

    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual int AcceptRole(zCCSRole* role, zCVob* vob);
    virtual int CheckRole(zCCSRole* role, zCVob* vob);
    virtual int CheckRoleResult();

    int resultReaction;

    static zCClassDef* classDef;
};

#endif
