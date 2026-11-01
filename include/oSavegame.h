#ifndef OSAVEGAME_H
#define OSAVEGAME_H

#include "zString.h"

class zCTexture;

class zCTextureConvert;

void Dir_Copy(const zSTRING& source, const zSTRING& destination);
void Dir_Clear(const zSTRING& directory, const zSTRING& mask);

extern const zSTRING SAVEGAME_DIR;
extern const zSTRING SAVEGAME_CURRENT_DIR;
extern const zSTRING SAVEGAME_QUICKSAVE_DIR;
extern const zSTRING SAVEGAME_EXT;
extern const zSTRING SAVEGAME_HEADER_FILE;
extern const zSTRING SAVEGAME_GLOBAL_FILE;
extern const zSTRING SAVEGAME_PIC_FILE;
extern const zSTRING SAVEGAME_THUMB_FILE;

class oCSavegameInfo : public zCObject {
public:

    oCSavegameInfo();
    oCSavegameInfo(int slot);
    virtual ~oCSavegameInfo();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    virtual void Archive(zCArchiver& archiver);
    virtual void Unarchive(zCArchiver& archiver);
    void Init();
    int DoesSavegameExist();
    void UpdateThumbPic(zCTextureConvert* converter);
    zSTRING GetDir();
    void CleanResources();
    void SetSlot(int slot);
    zSTRING GetName();
    zSTRING GetWorldName();
    zSTRING GetSaveTime();
    zSTRING GetGameTime();
    void ReloadResources();

    static zCClassDef* classDef;

    zSTRING m_Dir;
    zCTexture* m_ThumbTex;
    int m_SlotNr;
    zSTRING m_Name;
    zSTRING m_WorldName;
    int m_TimeDay;
    int m_TimeHour;
    int m_TimeMin;
    zSTRING m_SaveDate;
    int m_verMajor;
    int m_verMinor;
    int m_PlayTimeSeconds;
    int m_verPoint;
    int m_verInt;
    zSTRING m_verName;
    int m_notCompatible;
};

#endif
