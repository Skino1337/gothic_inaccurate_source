#include "include/oSavegame.h"
#include "include/oGame.h"
#include "include/zTexture.h"
#include "include/zFile.h"
#include <windows.h>

static int oCSavegameInfoClassDefState = -1;
const zSTRING SAVEGAME_DIR = "savegame";
const zSTRING SAVEGAME_CURRENT_DIR = "current";
const zSTRING SAVEGAME_QUICKSAVE_DIR = "quicksave";
const zSTRING SAVEGAME_EXT = "sav";
const zSTRING SAVEGAME_HEADER_FILE = "saveinfo.sav";
const zSTRING SAVEGAME_GLOBAL_FILE = "savedat.sav";
const zSTRING SAVEGAME_PIC_FILE = "savepic.sav";
const zSTRING SAVEGAME_THUMB_FILE = "thumb.sav";
static zCClassDef oCSavegameInfoClassDef("oCSavegameInfo", "zCObject",
    oCSavegameInfo::_CreateNewInstance, 0, 0xB4, 0, "");
zCClassDef* oCSavegameInfo::classDef = &oCSavegameInfoClassDef;

zSTRING oCSavegameInfo::GetName()
{
    return m_Name;
}

zSTRING oCSavegameInfo::GetWorldName()
{
    return m_WorldName;
}

zSTRING oCSavegameInfo::GetSaveTime()
{
    return m_SaveDate;
}

oCSavegameInfo::oCSavegameInfo()
{
    Init();
}

oCSavegameInfo::oCSavegameInfo(int slot)
{
    Init();
    SetSlot(slot);
}

oCSavegameInfo::~oCSavegameInfo()
{
    CleanResources();
}

zCObject* oCSavegameInfo::_CreateNewInstance()
{
    oCSavegameInfo* info = new oCSavegameInfo;
    zCClassDef::ObjectCreated(info, classDef);
    return info;
}

void oCSavegameInfo::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(memory, classDef);
    ::operator delete(memory);
}

zCClassDef* oCSavegameInfo::_GetClassDef() const
{
    return classDef;
}

void oCSavegameInfo::Init()
{
    m_TimeDay = -1;
    m_TimeHour = 0;
    m_TimeMin = 0;
    m_SlotNr = -1;
    m_ThumbTex = NULL;
    m_notCompatible = 0;
    m_verMajor = 2;
    m_verMinor = 6;
    m_verPoint = 0;
    m_verInt = 0;
    m_verName = "Gothic II";
    m_PlayTimeSeconds = 0;
}

int oCSavegameInfo::DoesSavegameExist()
{
    return m_TimeDay >= 0;
}

zSTRING oCSavegameInfo::GetDir()
{
    return m_Dir;
}

void oCSavegameInfo::CleanResources()
{
    if (m_ThumbTex) {
        m_ThumbTex->Release();
        m_ThumbTex = NULL;
    }
}

void oCSavegameInfo::SetSlot(int slot)
{
    m_SlotNr = slot;
    m_Dir.Clear();
    m_notCompatible = 0;
    m_PlayTimeSeconds = 0;
    m_verMajor = 2;
    m_verMinor = 6;
    m_verPoint = 0;
    m_verInt = 0;
    m_verName = "Gothic II";
    if (slot == 0)
        m_Dir = "savegame_current";
    else if (slot >= 1 && slot <= 20)
        m_Dir = zSTRING("savegame_slot") + zSTRING(slot);
}

void oCSavegameInfo::UpdateThumbPic(zCTextureConvert* converter)
{
    if (!converter)
        return;
    zCTextureInfo info = converter->GetTextureInfo();
    info.texFormat = zCTexture::CalcNextBestTexFormat(zRND_TEX_FORMAT_RGB_565);
    info.sizeX = 256;
    info.sizeY = 256;
    info.numMipMap = 1;
    converter->SetTextureInfo(info);
    zCTexture* texture = zrenderer->CreateTexture();
    zCTextureExchange::CopyContents(converter, texture);
    zFILE_FILE file(zoptions->GetDirString(3) + m_Dir + "\\THUMB.TEX");
    if (!file.Create()) {
        zCTextureFileFormatInternal format;
        format.SaveTexture(file, texture);
        file.Close();
    }
    texture->Release();
}

void oCSavegameInfo::ReloadResources()
{
    CleanResources();
    zFILE_FILE file(zoptions->GetDirString(3) + m_Dir + "\\THUMB.TEX");
    if (file.Exists() && !file.Open(false)) {
        m_ThumbTex = zrenderer->CreateTexture();
        zCTextureFileFormatInternal format;
        format.LoadTexture(file, m_ThumbTex);
        file.Close();
    }
}

void oCSavegameInfo::Archive(zCArchiver& archiver)
{
    SYSTEMTIME time;
    GetLocalTime(&time);
    m_SaveDate.Sprintf("%d.%d.%d %d:%02d", time.wDay, time.wMonth, time.wYear,
        time.wHour, time.wMinute);
    archiver.WriteString(m_Name);
    archiver.WriteString(m_WorldName);
    archiver.WriteInt(m_TimeDay);
    archiver.WriteInt(m_TimeHour);
    archiver.WriteInt(m_TimeMin);
    archiver.WriteString(m_SaveDate);
    archiver.WriteInt(m_verMajor);
    archiver.WriteInt(m_verMinor);
    archiver.WriteInt(m_PlayTimeSeconds);
    archiver.WriteInt(m_verPoint);
    archiver.WriteInt(m_verInt);
    archiver.WriteString(m_verName);
    m_notCompatible = 0;
}

void oCSavegameInfo::Unarchive(zCArchiver& archiver)
{
    m_Name = archiver.ReadString();
    m_WorldName = archiver.ReadString();
    m_TimeDay = archiver.ReadInt();
    m_TimeHour = archiver.ReadInt();
    m_TimeMin = archiver.ReadInt();
    m_SaveDate = archiver.ReadString();
    m_verMajor = archiver.ReadInt();
    m_verMinor = archiver.ReadInt();
    m_PlayTimeSeconds = archiver.ReadInt();
    m_verPoint = -1;
    m_verInt = -1;
    m_verName.Clear();
    if (m_verMajor > 1 || (m_verMajor == 1 && m_verMinor >= 30)) {
        m_verPoint = archiver.ReadInt();
        m_verInt = archiver.ReadInt();
        m_verName = archiver.ReadString();
    }
    m_notCompatible = m_verMajor != 1 || m_verMinor > 27;
    if (m_notCompatible)
        zerr->Warning(zSTRING("B: VER: Version of savegame \"") + m_verName + "\" is incompatible");
}

zSTRING oCSavegameInfo::GetGameTime()
{
    if (m_TimeDay < 0)
        return "";
    zSTRING result;
    result.Sprintf("%d - %d:", m_TimeDay, m_TimeHour);
    if (m_TimeMin < 10)
        result += "0";
    result += zSTRING(m_TimeMin);
    return result;
}

oCSavegameManager::oCSavegameManager()
    : localInfo(NULL), initialized(0)
{
    localInfo = new oCSavegameInfo(0);
    zCClassDef::ObjectCreated(localInfo, oCSavegameInfo::classDef);
    for (int i = 0; i <= 20; ++i) {
        oCSavegameInfo* info = new oCSavegameInfo(i);
        zCClassDef::ObjectCreated(info, oCSavegameInfo::classDef);
        infoList.InsertEnd(info);
    }
}

oCSavegameManager::~oCSavegameManager()
{
    if (localInfo) {
        localInfo->Release();
        localInfo = NULL;
    }
    for (int i = 0; i < infoList.numInArray; ++i) {
        if (infoList.parray[i]) {
            infoList.parray[i]->Release();
            infoList.parray[i] = NULL;
        }
    }
}

void oCSavegameManager::Init()
{
    if (initialized)
        return;
    zerr->Message("B: SG: Initializing savegame-manager ...");
    zFILE_FILE savegameDirectory(zoptions->GetDirString(2));
    savegameDirectory.DirCreate();
    zFILE_FILE currentDirectory(zoptions->GetDirString(2) + SAVEGAME_CURRENT_DIR + "\\");
    currentDirectory.DirCreate();
    zoptions->ChangeDir(2);
    for (int i = 0; i < infoList.numInArray; ++i) {
        oCSavegameInfo* info = infoList.parray[i];
        info->SetSlot(i);
        info->m_Name = "---";
        info->UpdateThumbPic(NULL);
        zFILE_FILE header(zoptions->GetDirString(2) + info->GetDir() + "\\SAVEGAME_HDR.SAV");
        if (!header.Exists() || header.Open(false))
            continue;
        zCArchiver* archiver = zarcFactory->CreateArchiverRead(&header, 0);
        if (archiver) {
            info->Unarchive(*archiver);
            archiver->Close();
            archiver->Release();
        }
        header.Close();
        if (zerr->filter_level >= 5)
            zerr->Message("B: BS: Savegame " + zSTRING(i) + " found: \"" + info->GetName() + "\"");
    }
    initialized = 1;
}

void oCSavegameManager::Reinit()
{
    initialized = 0;
    Init();
}

void oCSavegameManager::CleanResources()
{
    for (int i = 0; i < infoList.numInArray; ++i)
        infoList.parray[i]->CleanResources();
}

void oCSavegameManager::SetAndWriteSavegame(int slot, oCSavegameInfo* info)
{
    oCSavegameInfo* oldInfo = infoList.parray[slot];
    infoList.parray[slot] = info;
    info->AddRef();
    if (oldInfo)
        oldInfo->Release();
    zSTRING directory = zoptions->GetDirString(2) + info->GetDir();
    zFILE_FILE dir(directory);
    dir.DirCreate();
    zFILE_FILE header(directory + "\\SAVEGAME_HDR.SAV");
    zCArchiver* archiver = zarcFactory->CreateArchiverWrite(header.GetFullPath(), 0, 0, 0);
    archiver->WriteObject(info);
    archiver->Close();
    archiver->Release();
    header.Close();
}

oCSavegameInfo* oCSavegameManager::GetSavegame(int slot)
{
    if (slot >= 0)
        return infoList.parray[slot];
    localInfo->SetSlot(slot);
    return localInfo;
}

void oCSavegameManager::CopyToCurrent(int slot)
{
    Dir_Copy(infoList.parray[slot]->GetDir(), GetSlotDirName(-1));
}

void oCSavegameManager::CopyFromCurrent(int slot)
{
    Dir_Copy(GetSlotDirName(-1), infoList.parray[slot]->GetDir());
}

void oCSavegameManager::ClearCurrent()
{
    zoptions->ChangeDir(2);
    Dir_Clear(GetSlotDirName(-1), "\\*.*");
}

zSTRING oCSavegameManager::GetSlotDirName(int slot)
{
    if (slot == -1 || slot == 0)
        return "savegame_current\\";
    if (slot >= 1 && slot <= 20)
        return zSTRING("savegame_slot") + zSTRING(slot) + "\\";
    return "";
}
