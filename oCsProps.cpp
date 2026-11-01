#include "include/oCsProps.h"

static int classDefState = -1;
static zCClassDef oCCSPropsClassDef("oCCSProps", "zCCSProps",
    oCCSProps::_CreateNewInstance, 1, 0x88, 0, "");
zCClassDef* oCCSProps::classDef = &oCCSPropsClassDef;

zCCSProps::zCCSProps()
    : csLoop(0), globalCutscene(0), distance(500.0f), hasToBeTriggerd(0),
      range(2000.0f), ignore(0), numLockedBlocks(0), runBehaviour(0), runBehaviourValue(0)
{
}

zCCSProps::~zCCSProps()
{
}

zCObject* zCCSProps::_CreateNewInstance()
{
    zCCSProps* properties = new zCCSProps;
    zCClassDef::ObjectCreated(properties, classDef);
    return properties;
}

zCClassDef* zCCSProps::_GetClassDef() const
{
    return classDef;
}

zSTRING zCCSProps::GetName()
{
    return name;
}

zSTRING zCCSProps::GetScriptFuncOnStop()
{
    return scriptFuncOnStop;
}

void zCCSProps::Archive(zCArchiver& archiver)
{
    if (archiver.InSaveGame())
        archiver.WriteString(name);
    archiver.WriteBool(globalCutscene);
    archiver.WriteBool(csLoop);
    archiver.WriteBool(hasToBeTriggerd);
    archiver.WriteFloat(distance);
    archiver.WriteFloat(range);
    archiver.WriteInt(numLockedBlocks);
    archiver.WriteEnum(runBehaviour);
    archiver.WriteInt(runBehaviourValue);
    archiver.WriteString(stageName);
    archiver.WriteString(scriptFuncOnStop);
}

void zCCSProps::Unarchive(zCArchiver& archiver)
{
    if (archiver.InSaveGame())
        name = archiver.ReadString();
    globalCutscene = archiver.ReadBool();
    csLoop = archiver.ReadBool();
    hasToBeTriggerd = archiver.ReadBool();
    distance = archiver.ReadFloat();
    range = archiver.ReadFloat();
    numLockedBlocks = archiver.ReadInt();
    runBehaviour = archiver.ReadEnum();
    runBehaviourValue = archiver.ReadInt();
    stageName = archiver.ReadString();
    scriptFuncOnStop = archiver.ReadString();
}

int zCCSProps::AcceptRole(zCCSRole* role, zCVob* vob)
{
    return CheckRole(role, vob);
}

int zCCSProps::CheckRole(zCCSRole*, zCVob*)
{
    return 1;
}

int zCCSProps::CheckDistance(float value)
{
    return ignore || distance <= 0.0f || value <= distance;
}

int zCCSProps::CheckRange(float value)
{
    return range <= 0.0f || value <= range;
}

void zCCSProps::PrintDebugInfo()
{
    zerr->Message(globalCutscene ? "B: CS: - is a global cutscene." :
        "B: CS: - is an ambient cutscene.");
    zerr->Message(csLoop ? "B: CS: - loops." : "B: CS: - doesn't loop.");
    if (hasToBeTriggerd)
        zerr->Message("B: CS: - has to be triggered.");
    zerr->Message("B: CS: - distance: " + zSTRING(distance, 9));
    zerr->Message("B: CS: - range: " + zSTRING(range, 9));
    if (runBehaviour == 0)
        zerr->Message("B: CS: - runBehaviour: RUN_ALWAYS");
    else if (runBehaviour == 1)
        zerr->Message("B: CS: - runBehaviour: " + zSTRING(runBehaviourValue) + " times");
    else if (runBehaviour == 2)
        zerr->Message("B: CS: - runBehaviour: " + zSTRING(runBehaviourValue) + " times per hour");
    else if (runBehaviour == 3)
        zerr->Message("B: CS: - runBehaviour: " + zSTRING(runBehaviourValue) + " times per day");
    else
        zerr->Message("B: CS: - runBehaviour: unknown");
}

oCCSProps::oCCSProps()
    : resultReaction(0)
{
}

oCCSProps::~oCCSProps()
{
}

zCObject* oCCSProps::_CreateNewInstance()
{
    oCCSProps* properties = new oCCSProps;
    zCClassDef::ObjectCreated(properties, classDef);
    return properties;
}

void oCCSProps::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* oCCSProps::_GetClassDef() const
{
    return classDef;
}

void oCCSProps::Archive(zCArchiver& archiver)
{
    zCCSProps::Archive(archiver);
}

void oCCSProps::Unarchive(zCArchiver& archiver)
{
    zCCSProps::Unarchive(archiver);
}

int oCCSProps::AcceptRole(zCCSRole* role, zCVob* vob)
{
    return zCCSProps::AcceptRole(role, vob);
}

int oCCSProps::CheckRole(zCCSRole* role, zCVob* vob)
{
    resultReaction = 0;
    zCCSProps::CheckRole(role, vob);
    if (!vob)
        return 1;
    return (vob->GetCSStateFlags() & 1) != 0;
}

int oCCSProps::CheckRoleResult()
{
    return resultReaction;
}
