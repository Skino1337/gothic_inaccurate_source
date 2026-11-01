#include "include/zVob.h"
#include "include/zMusic.h"

// EXE: 0x009A4A34
static int musicControlerState = -1;
// EXE: 0x009A4B14
static int musicControlerRuntimeState = -1;
// EXE: 0x009A4A38
static zCClassDef zCEventMusicControlerClassDef("zCEventMusicControler", "zCEventMessage",
    zCEventMusicControler::_CreateNewInstance, 0, 0x40, 0, "");
// EXE: 0x009A4A38
zCClassDef* zCEventMusicControler::classDef = &zCEventMusicControlerClassDef;
// EXE: 0x009A4AA8
static zCClassDef zCMusicControlerClassDef("zCMusicControler", "zCVob",
    zCMusicControler::_CreateNewInstance, 0, 0x120, 0, "");
// EXE: 0x009A4AA8
zCClassDef* zCMusicControler::classDef = &zCMusicControlerClassDef;

// EXE: 0x006426F0
zCObject* zCMusicControler::_CreateNewInstance()
{
    zCMusicControler* controler = new zCMusicControler;
    zCClassDef::ObjectCreated(controler, classDef);
    return controler;
}

// EXE: 0x00642770
zCClassDef* zCMusicControler::_GetClassDef() const
{
    return classDef;
}

// EXE: 0x00642960
zCObject* zCEventMusicControler::_CreateNewInstance()
{
    zCEventMusicControler* controler = new zCEventMusicControler;
    zCClassDef::ObjectCreated(controler, classDef);
    return controler;
}

// EXE: 0x00642B20
zCClassDef* zCEventMusicControler::_GetClassDef() const
{
    return classDef;
}

// EXE: 0x00642B30
int zCEventMusicControler::MD_GetNumOfSubTypes()
{
    return 2;
}

// EXE: 0x00642B70
zCEventMusicControler::~zCEventMusicControler()
{
}

// EXE: 0x00642BC0
zSTRING zCEventMusicControler::MD_GetSubTypeString(int subType)
{
    if (subType == 0)
        return zSTRING("START_SPECIAL_SGT");
    if (subType == 1)
        return zSTRING("STOP_SPECIAL_SGT");
    return zSTRING();
}

// EXE: 0x00642CC0
int zCEventMusicControler::MD_GetTimeBehavior()
{
    return 0;
}

// EXE: 0x00642CD0
float zCEventMusicControler::MD_GetMinTime()
{
    return 0.0f;
}

// EXE: 0x00642CE0
void zCEventMusicControler::Archive(zCArchiver& archiver)
{
    zCEventMessage::Archive(archiver);
    archiver.WriteString(sgt);
}

// EXE: 0x00642D10
void zCEventMusicControler::Unarchive(zCArchiver& archiver)
{
    zCEventMessage::Unarchive(archiver);
    sgt = archiver.ReadString();
}

// EXE: 0x00642D40
zCMusicControler::zCMusicControler()
{
}

// EXE: 0x00642D90
zCMusicControler::~zCMusicControler()
{
}

// EXE: 0x00642DA0
void zCMusicControler::OnMessage(zCEventMessage* message, zCVob*)
{
    zCEventMusicControler* musicMessage = dynamic_cast<zCEventMusicControler*>(message);
    if (!musicMessage)
        return;
    if (musicMessage->subType == 0) {
        zCZoneMusic::SetAutochange(0);
        zmusic->PlayThemeByScript(musicMessage->sgt, 0, 0);
    } else if (musicMessage->subType == 1)
        zCZoneMusic::SetAutochange(1);
}

// EXE: 0x00642E00
void zCMusicControler::ThisVobRemovedFromWorld(zCWorld* targetWorld)
{
    zCVob::ThisVobRemovedFromWorld(targetWorld);
    if (!zCZoneMusic::SetAutochange(1))
        zmusic->Stop();
}
