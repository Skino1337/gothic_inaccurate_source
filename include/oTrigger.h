#ifndef OTRIGGER_H
#define OTRIGGER_H

#include "zVob.h"

class oCTriggerChangeLevel : public zCTrigger {
public:

    oCTriggerChangeLevel() {}
    virtual ~oCTriggerChangeLevel() {}
    static void* operator new(unsigned int size);
    static void operator delete(void* memory);
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void TriggerTarget(zCVob* vob);
    virtual void UntriggerTarget(zCVob* vob);
    void SetLevelName(const zSTRING& level, const zSTRING& start);

    zSTRING levelName;
    zSTRING startVob;

    static zCClassDef* classDef;
};

class oCObjectGenerator : public zCVob {
public:

    oCObjectGenerator() : speed(0.0f) {}
    virtual ~oCObjectGenerator() {}
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void OnTrigger(zCVob* first, zCVob* second);
    void SetObjectName(const zSTRING& name);
    void SetObjectSpeed(float value);

    float speed;
    zSTRING objectName;
};

class oCTriggerScript : public zCTrigger {
public:

    oCTriggerScript() {}
    virtual ~oCTriggerScript() {}
    static void operator delete(void* memory);
    static zCObject* _CreateNewInstance();
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    virtual void TriggerTarget(zCVob* vob);
    virtual void UntriggerTarget(zCVob* vob);
    void SetScriptFunc(const zSTRING& function, int count);

    zSTRING scriptFunc;

    static zCClassDef* classDef;
};

#endif
