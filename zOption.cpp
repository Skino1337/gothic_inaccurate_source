#include "include/zOption.h"
#include "include/oGame.h"
#include "include/zFile.h"
#include "include/oCollisionObjectMiscChar.h"
#include <windows.h>
#include <direct.h>
#include <io.h>

static int optionState = -1;
zSTRING zOPT_SEC_INTERNAL = "INTERNAL";
zSTRING zOPT_SEC_GAME = "GAME";
zSTRING zOPT_SEC_VIDEO = "VIDEO";
zSTRING zOPT_SEC_SOUND = "SOUND";
zSTRING zOPT_SEC_MUSIC = "MUSIC";
zSTRING zOPT_SEC_KEYS = "KEYS";
zSTRING zOPT_SEC_KEYS_DEFAULT0 = "KEYSDEFAULT0";
zSTRING zOPT_SEC_KEYS_DEFAULT1 = "KEYSDEFAULT1";
zSTRING zOPT_SEC_PERFORMANCE = "PERFORMANCE";
zSTRING zOPT_SEC_INFO = "INFO";
zSTRING zOPT_SEC_FILES = "FILES";
zSTRING zOPT_SEC_SETTINGS = "SETTINGS";
zSTRING zOPT_SEC_OPTIONS = "OPTIONS";
zSTRING zOPTION_PATHNAMES[26] = {
    "DIR_SYSTEM", "DIR_WEB", "DIR_SAVEGAMES", "DIR_DATA", "DIR_ANIMS", "DIR_CUTSCENES",
    "DIR_OUTPUTUNITS", "DIR_MESHES", "DIR_SCRIPTS", "DIR_SOUND", "DIR_VIDEO", "DIR_MUSIC",
    "DIR_TEX", "DIR_TEX_DESKTOP", "DIR_WORLD", "DIR_PRESETS", "DIR_TOOLS_DATA",
    "DIR_COMPILED_ANIMS", "DIR_COMPILED_MESHES", "DIR_COMPILED_SCRIPTS", "DIR_COMPILED_TEXTURES",
    "DIR_TOOLS_CONFIG", "SUBDIR_INTERN", "", "DIR_ROOT", "DIR_EXECUTABLE"
};
zSTRING ZOPT_LANGUAGE = "gameLanguage";
zSTRING ZOPT_DEF_LANGUAGE = "ENG";
zSTRING ZOPT_SND_SFX_VOL = "soundVolume";
zSTRING ZOPT_SND_SPEECH_VOL = "soundVolume";
zSTRING ZOPT_SND_3DPROV = "soundProvider";
zSTRING ZOPT_SND_SPEAKERTYPE = "soundSpeakerType";
zSTRING ZOPT_SND_SAMPLERATE = "soundSampleRate";
zSTRING ZOPT_SND_USEREVERB = "soundUseReverb";
zSTRING ZOPT_SND_MUSIC_VOL = "musicVolume";
zSTRING ZOPT_VID_GAMMA = "zVidGamma";
zSTRING ZOPT_VID_RES = "vidResIndex";
zSTRING ZOPT_JOY_ENABLE = "gameJoyEnabled";
zSTRING ZOPT_GAMEKEY_UP = "keyUp";
zSTRING ZOPT_GAMEKEY_DOWN = "keyDown";
zSTRING ZOPT_GAMEKEY_LEFT = "keyLeft";
zSTRING ZOPT_GAMEKEY_RIGHT = "keyRight";
zSTRING ZOPT_GAMEKEY_ACTION = "keyAction";
zSTRING ZOPT_GAMEKEY_SLOW = "keySlow";
zSTRING ZOPT_GAMEKEY_SMOVE = "keySMove";
zSTRING ZOPT_GAMEKEY_WEAPON = "keyWeapon";
zSTRING ZOPT_GAMEKEY_SNEAK = "keySneak";
zSTRING ZOPT_GAMEKEY_LOOK = "keyLook";
zSTRING ZOPT_GAMEKEY_LOOK_FP = "keyLookFP";
zSTRING ZOPT_GAMEKEY_END = "keyEnd";
zSTRING ZOPT_GAMEKEY_STRAFELEFT = "keyStrafeLeft";
zSTRING ZOPT_GAMEKEY_STRAFERIGHT = "keyStrafeRight";
zSTRING ZOPT_GAMEKEY_INVENTORY = "keyInventory";
zSTRING ZOPT_GAMEKEY_SCREEN_STATUS = "keyShowStatus";
zSTRING ZOPT_GAMEKEY_SCREEN_LOG = "keyShowLog";
zSTRING ZOPT_GAMEKEY_SCREEN_MAP = "keyShowMap";
zSTRING ZOPT_GAMEKEY_LOCK_TARGET = "keyLockTarget";
zSTRING ZOPT_GAMEKEY_PARADE = "keyParade";
zSTRING ZOPT_GAMEKEY_ACTIONLEFT = "keyActionLeft";
zSTRING ZOPT_GAMEKEY_ACTIONRIGHT = "keyActionRight";
zSTRING ZOPT_GAMEKEY_LAME_POTION = "keyPotion";
zSTRING ZOPT_GAMEKEY_LAME_HEAL = "keyHeal";
static zSTRING optionAssignmentDelimiter = "=";

static void OptionTrim(zSTRING& text, char value)
{
    int first = 0;
    while (first < text.Length() && text[first] == value)
        ++first;

    int last = text.Length();
    while (last > first && text[last - 1] == value)
        --last;

    text = text.Copied(first, last - first);
}

zCOptionEntry::zCOptionEntry(const zSTRING& name, const zSTRING& value)
    : changed(0), varName(name), varValue(value), varValueTemp(value), varFlag(0)
{
    OptionTrim(varName, ' ');
    OptionTrim(varValue, ' ');
}

zCOptionEntry::~zCOptionEntry()
{
    ccbList.DeleteList();
}

zCOptionSection::zCOptionSection(const zSTRING& name)
    : secName(name)
{
    OptionTrim(secName, ' ');
    OptionTrim(secName, '[');
    OptionTrim(secName, ']');
    secName.Upper();
}

zCOptionSection::~zCOptionSection()
{
    for (int i = entryList.numInArray - 1; i >= 0; --i)
        delete entryList[i];
    entryList.DeleteList();
}

zCOption::zCOption()
    : m_bReadTemp(1)
{
    for (int i = 0; i < 26; ++i)
        directory[i] = new zFILE_FILE;
}

zCOption::~zCOption()
{
    for (int i = sectionList.numInArray - 1; i >= 0; --i)
        delete sectionList[i];
    sectionList.DeleteList();

    for (int i = 0; i < 26; ++i)
    {
        delete directory[i];
        directory[i] = NULL;
    }
}

int zCOption::Load(zSTRING fileName)
{
    fileName.Lower();

    for (int i = sectionList.numInArray - 1; i >= 0; --i)
        delete sectionList[i];
    sectionList.DeleteList();

    ChangeDir(0);
    zFILE_FILE file(fileName);
    if (!file.Exists())
        return 0;

    file.Open(false);
    zCOptionSection* section = new zCOptionSection("");
    sectionList.Insert(section);
    zSTRING line;
    zSTRING trash;

    while (!file.Eof())
    {
        file.ReadCodeline(line);
        OptionTrim(line, ' ');
        if (line.IsEmpty() || line[0] == ';')
        {
            trash += line;
            if (!file.Eof())
                trash += "\n";
            continue;
        }

        if (!trash.IsEmpty())
        {
            section->entryList.Insert(new zCOptionEntry("", trash));
            trash.Clear();
        }

        if (line[0] == '[')
        {
            section = GetSectionByName(line, 1);
            continue;
        }

        int separator = line.Search("=");
        if (separator <= 0)
        {
            zerr->Report(zERR_TYPE_WARN, 0, "B: INI: Unknown line: " + line, 0, 0, 310,
                "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zOption.cpp", NULL);
            continue;
        }

        zSTRING name = line.Copied(0, separator);
        zSTRING value = line.Copied(separator + 1, line.Length() - separator - 1);
        section->entryList.Insert(new zCOptionEntry(name, value));
    }

    if (!trash.IsEmpty())
        section->entryList.Insert(new zCOptionEntry("", trash));
    file.Close();

    char computerName[16];
    DWORD computerNameSize = sizeof(computerName);
    GetComputerNameA(computerName, &computerNameSize);
    WriteString(zOPT_SEC_INTERNAL, "idComputerName", zSTRING(computerName), 0);
    char userName[16];
    DWORD userNameSize = sizeof(userName);
    GetUserNameA(userName, &userNameSize);
    WriteString(zOPT_SEC_INTERNAL, "idUserName", zSTRING(userName), 0);
    return 1;
}

int zCOption::Save(zSTRING fileName)
{
    fileName.Lower();
    zFILE_FILE file(fileName);
    file.Create();

    for (int i = 0; i < sectionList.numInArray; ++i)
    {
        zCOptionSection* section = sectionList[i];
        if (!section->secName.IsEmpty())
            file.Write("[" + section->secName + "]\r\n");

        for (int j = 0; j < section->entryList.numInArray; ++j)
        {
            zCOptionEntry* entry = section->entryList[j];
            if (entry->varFlag & 1)
                continue;
            if (!entry->varName.IsEmpty())
                file.Write(entry->varName + "=" + entry->varValue + "\r\n");
            else
                file.Write(entry->varValue + "\r\n");
        }
        file.Write("\r\n");
    }

    file.Close();
    return 1;
}

zCOptionSection* zCOption::GetSectionByName(const zSTRING& name, int create)
{
    zSTRING upperName = name;
    upperName.Upper();

    for (int i = 0; i < sectionList.numInArray; ++i)
    {
        if (sectionList[i]->secName == upperName)
            return sectionList[i];
    }

    if (!create)
        return NULL;

    zCOptionSection* section = new zCOptionSection(name);
    sectionList.Insert(section);
    return section;
}

zCOptionEntry* zCOption::GetEntryByName(zCOptionSection* section, const zSTRING& name, int create)
{
    zSTRING upperName = name;
    upperName.Upper();

    for (int i = 0; i < section->entryList.numInArray; ++i)
    {
        zSTRING entryName = section->entryList[i]->varName;
        entryName.Upper();
        if (entryName == upperName)
            return section->entryList[i];
    }

    if (!create)
        return NULL;

    zCOptionEntry* entry = new zCOptionEntry(name, "");
    section->entryList.Insert(entry);
    return entry;
}

int zCOption::WriteString(const zSTRING& sectionName, const char* name, zSTRING value, int temp)
{
    zCOptionSection* section = GetSectionByName(sectionName, 1);
    zCOptionEntry* entry = GetEntryByName(section, name, 1);
    zSTRING& oldValue = m_bReadTemp ? entry->varValueTemp : entry->varValue;
    bool changed = !(oldValue == value);
    entry->varValueTemp = value;
    if (!m_bReadTemp)
        entry->varValue = value;
    entry->changed = 1;

    if (changed)
    {
        for (int i = 0; i < entry->ccbList.numInArray; ++i)
        {
            if (entry->ccbList[i](*entry) == 1)
                break;
        }
    }

    return 1;
}

int zCOption::WriteBool(const zSTRING& section, const char* name, int value, int temp)
{
    return WriteString(section, name, value ? "1" : "0", temp);
}

int zCOption::WriteInt(const zSTRING& section, const char* name, int value, int temp)
{
    return WriteString(section, name, zSTRING(value), temp);
}

int zCOption::WriteDWord(const zSTRING& section, const char* name, unsigned long value, int temp)
{
    return WriteString(section, name, zSTRING(value), temp);
}

int zCOption::WriteReal(const zSTRING& section, const char* name, float value, int temp)
{
    return WriteString(section, name, zSTRING(value, 20), temp);
}

#include <cstdlib>

zSTRING zCOption::ReadString(const zSTRING& sectionName, const char* name, const char* defaultValue)
{
    zCOptionSection* section = GetSectionByName(sectionName, 1);
    zCOptionEntry* entry = GetEntryByName(section, name, 1);

    if (entry->varValueTemp.IsEmpty() && defaultValue)
    {
        entry->varValueTemp = defaultValue;
        if (!m_bReadTemp)
            entry->varValue = defaultValue;
    }

    return m_bReadTemp ? entry->varValueTemp : entry->varValue;
}

int zCOption::ReadBool(const zSTRING& section, const char* name, int defaultValue)
{
    zSTRING value = ReadString(section, name, defaultValue ? "1" : "0");
    value.Upper();
    return value == "1" || value == "TRUE";
}

int zCOption::ReadInt(const zSTRING& section, const char* name, int defaultValue)
{
    return std::atoi(ReadString(section, name, zSTRING(defaultValue).ToChar()).ToChar());
}

unsigned long zCOption::ReadDWord(const zSTRING& section, const char* name,
    unsigned long defaultValue)
{
    return std::strtoul(ReadString(section, name, zSTRING(defaultValue).ToChar()).ToChar(), NULL,
        10);
}

float zCOption::ReadReal(const zSTRING& section, const char* name, float defaultValue)
{
    return (float)std::atof(ReadString(section, name, zSTRING(defaultValue, 9).ToChar()).ToChar());
}

int zCOption::EntryExists(const zSTRING& sectionName, const char* name)
{
    zCOptionSection* section = GetSectionByName(sectionName, 0);
    return section ? (int)GetEntryByName(section, name, 0) : 0;
}

unsigned short zCOption::GetNumEntries(zCOptionSection* section)
{
    return section ? section->entryList.numInArray : 0;
}

zCOptionEntry* zCOption::GetEntry(zCOptionSection* section, int index)
{
    if (section && index >= 0 && index < section->entryList.numInArray)
        return section->entryList.GetSafe(index);
    return NULL;
}

int zCOption::RemoveEntry(const zSTRING& sectionName, const char* name)
{
    zCOptionSection* section = GetSectionByName(sectionName, 1);
    zCOptionEntry* entry = GetEntryByName(section, name, 1);

    for (int i = 0; i < section->entryList.numInArray; ++i)
    {
        if (section->entryList[i] == entry)
        {
            section->entryList[i] = section->entryList[section->entryList.numInArray - 1];
            --section->entryList.numInArray;
            break;
        }
    }

    delete entry;
    return 1;
}

int zCOption::SectionExists(const zSTRING& name)
{
    return GetSectionByName(name, 1) != NULL;
}

unsigned short zCOption::GetNumSections()
{
    return sectionList.numInArray;
}

zCOptionSection* zCOption::GetSection(int index)
{
    if (index >= 0 && index < sectionList.numInArray)
        return sectionList.GetSafe(index);
    return NULL;
}

int zCOption::RemoveSection(const zSTRING& name)
{
    zCOptionSection* section = GetSectionByName(name, 1);
    if (!section)
        return 0;

    for (int i = 0; i < sectionList.numInArray; ++i)
    {
        if (sectionList[i] == section)
        {
            sectionList[i] = sectionList[sectionList.numInArray - 1];
            --sectionList.numInArray;
            break;
        }
    }
    return 1;
}

void zCOption::InsertChangeHandler(const zSTRING& sectionName, const char* name,
    int (*handler)(zCOptionEntry&))
{
    zCOptionSection* section = GetSectionByName(sectionName, 1);
    zCOptionEntry* entry = GetEntryByName(section, name, 1);
    entry->ccbList.Insert(handler);
}

#include <cstdio>

int zCOption::WriteRaw(const zSTRING& section, const char* name, void* data, int size, int temp)
{
    zSTRING text;
    unsigned char* bytes = (unsigned char*)data;

    for (int i = 0; i < size; ++i)
    {
        char hex[3];
        std::sprintf(hex, "%02x", bytes[i]);
        text += hex;
    }

    WriteString(section, name, text, temp);
    return 1;
}

int zCOption::ReadRaw(const zSTRING& section, const char* name, void*& data, void* defaultData,
    int size)
{
    if (!EntryExists(section, name))
        WriteRaw(section, name, defaultData, size, 0);

    zSTRING text = ReadString(section, name, "");
    int byteCount = text.Length() / 2;
    unsigned char* bytes = new unsigned char[byteCount];
    int result = 0;

    while (result < byteCount)
    {
        unsigned int value = 0;
        std::sscanf(text.ToChar() + result * 2, "%02x", &value);
        bytes[result] = (unsigned char)value;
        ++result;
    }

    data = bytes;
    return result;
}

void zCOption::SetFlag(const zSTRING& sectionName, const char* name, int enabled)
{
    zCOptionSection* section = GetSectionByName(sectionName, 1);
    zCOptionEntry* entry = GetEntryByName(section, name, 1);
    entry->varFlag |= enabled;
}

int zCOption::WriteBool(const zSTRING& section, const zSTRING& name, int value, int temp)
{
    return WriteBool(section, name.ToChar(), value, temp);
}

int zCOption::WriteInt(const zSTRING& section, const zSTRING& name, int value, int temp)
{
    return WriteInt(section, name.ToChar(), value, temp);
}

int zCOption::WriteDWord(const zSTRING& section, const zSTRING& name, unsigned long value, int temp)
{
    return WriteDWord(section, name.ToChar(), value, temp);
}

int zCOption::WriteReal(const zSTRING& section, const zSTRING& name, float value, int temp)
{
    return WriteReal(section, name.ToChar(), value, temp);
}

int zCOption::WriteString(const zSTRING& section, const zSTRING& name, zSTRING value, int temp)
{
    return WriteString(section, name.ToChar(), value, temp);
}

int zCOption::WriteRaw(const zSTRING& section, const zSTRING& name, void* data, int size, int temp)
{
    return WriteRaw(section, name.ToChar(), data, size, temp);
}

int zCOption::ReadBool(const zSTRING& section, const zSTRING& name, int defaultValue)
{
    return ReadBool(section, name.ToChar(), defaultValue);
}

int zCOption::ReadInt(const zSTRING& section, const zSTRING& name, int defaultValue)
{
    return ReadInt(section, name.ToChar(), defaultValue);
}

unsigned long zCOption::ReadDWord(const zSTRING& section, const zSTRING& name,
    unsigned long defaultValue)
{
    return ReadDWord(section, name.ToChar(), defaultValue);
}

float zCOption::ReadReal(const zSTRING& section, const zSTRING& name, float defaultValue)
{
    return ReadReal(section, name.ToChar(), defaultValue);
}

zSTRING zCOption::ReadString(const zSTRING& section, const zSTRING& name, const char* defaultValue)
{
    return ReadString(section, name.ToChar(), defaultValue);
}

int zCOption::ReadRaw(const zSTRING& section, const zSTRING& name, void* data, int size)
{
    zSTRING text = ReadString(section, name, "");
    int count = size < text.Length() / 2 ? size : text.Length() / 2;
    unsigned char* bytes = (unsigned char*)data;

    for (int i = 0; i < count; ++i)
    {
        unsigned int value = 0;
        std::sscanf(text.ToChar() + i * 2, "%02x", &value);
        bytes[i] = (unsigned char)value;
    }
    return count;
}

int zCOption::WriteString(const zSTRING& section, const zSTRING& name, const zSTRING& value)
{
    return WriteString(section, name, value, 0);
}

int zCOption::WriteBool(const zSTRING& section, const zSTRING& name, int value)
{
    return WriteBool(section, name, value, 0);
}

int zCOption::WriteInt(const zSTRING& section, const zSTRING& name, int value)
{
    return WriteInt(section, name, value, 0);
}

int zCOption::WriteReal(const zSTRING& section, const zSTRING& name, float value)
{
    return WriteReal(section, name, value, 0);
}

zSTRING zCOption::ReadString(const zSTRING& section, const zSTRING& name,
    const zSTRING& defaultValue)
{
    return ReadString(section, name, defaultValue.ToChar());
}

void zCOption::SetFlag(const zSTRING& section, const zSTRING& name, int enabled)
{
    SetFlag(section, name.ToChar(), enabled);
}

int zCOption::AddParameters(zSTRING parameters)
{
    commandline = parameters + " " + commandline;
    commandline.Upper();
    return 0;
}

int zCOption::Init(zSTRING parameters, bool initialize)
{
    if (!initialize)
        return 0;

    AddParameters(parameters);
    char modulePath[256];
    GetModuleFileNameA(NULL, modulePath, sizeof(modulePath) - 1);
    zSTRING executableDirectory = modulePath;
    int separator = executableDirectory.SearchRev("\\", 1);
    if (separator >= 0)
        executableDirectory = executableDirectory.Copied(0, separator);
    _chdir(executableDirectory.ToChar());
    dir_string[25] = executableDirectory;
    ((zFILE_FILE*)directory[25])->Init(executableDirectory);

    char currentDirectory[256];
    _getcwd(currentDirectory, sizeof(currentDirectory) - 1);
    zSTRING pathsFile = "paths.d";
    if (_access(pathsFile.ToChar(), 0) == -1)
    {
        zerr->Report(zERR_TYPE_FATAL, 0, "B: zOPT: Could not find file " + pathsFile,
            -1, 0, 781, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zOption.cpp", NULL);
        return 0;
    }

    parser.Reset();
    parser.ParseFile(pathsFile);
    int rootIndex = parser.GetIndex("DIR_ROOT");
    if (rootIndex < 0)
    {
        zerr->Report(zERR_TYPE_FATAL, 0,
            "B: zOPT: Entry missed in " + pathsFile + ": DIR_ROOT", -1, 0, 799,
            "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zOption.cpp", NULL);
    }

    zSTRING rootDirectory = parser.GetText(rootIndex, 0);
    _chdir(rootDirectory.ToChar());
    _getcwd(currentDirectory, sizeof(currentDirectory) - 1);
    rootDirectory = zFILE::SetRootDirectory(zSTRING(currentDirectory));

    for (int i = 0; i < 23; ++i)
    {
        if (zOPTION_PATHNAMES[i].IsEmpty())
            continue;

        int index = parser.GetIndex(zOPTION_PATHNAMES[i]);
        if (index < 0)
        {
            zerr->Report(zERR_TYPE_FATAL, 0,
                "B: zOPT: Entry missed in " + pathsFile + ": " + zOPTION_PATHNAMES[i],
                -1, 0, 851, "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zOption.cpp", NULL);
            continue;
        }

        zSTRING path = parser.GetText(index, 0);
        path.Upper();
        int dataTag = path.Search("$DATA$");
        if (dataTag >= 0)
        {
            path.Delete(dataTag, 6);
            path.Insert(dataTag, dir_string[3]);
            int slash = path.Search("\\\\");
            if (slash >= 0)
                path.Delete(slash, 1);
        }

        ((zFILE_FILE*)directory[i])->Init(path);
        dir_string[i] = ((zFILE_FILE*)directory[i])->GetFullPath();
    }

    dir_string[24] = rootDirectory + "\\";
    ((zFILE_FILE*)directory[24])->Init(dir_string[24]);
    parser.Reset();
    return 0;
}

int zCOption::ChangeDir(int path)
{
    if (directory[path])
        ((zFILE_FILE*)directory[path])->ChangeDir(false);
    else
        zerr->Report(zERR_TYPE_FAULT, 0,
            "B: (zCOption::ChangeDir) directory-entry not defined!", 0, 0, 900,
            "P:\\dev\\g2addon\\release\\ZenGin\\_bert\\zOption.cpp", NULL);
    return 0;
}

zFILE* zCOption::GetDir(int path)
{
    return directory[path];
}

zSTRING zCOption::GetDirString(int path)
{
    return dir_string[path];
}

int zCOption::Parm(const zSTRING& name)
{
    zSTRING key = "-" + name;
    key.Upper();
    commandline.Upper();
    return commandline.Search(key) >= 0;
}

zSTRING zCOption::ParmValueRaw(const zSTRING& name)
{
    zSTRING key = "-" + name + ":";
    key.Upper();
    int position = commandline.Search(key);
    if (position < 0)
        return "";

    int start = position + key.Length();
    int end = start;
    while (end < commandline.Length() && commandline[end] != ' ' && commandline[end] != 0)
        ++end;
    return commandline.Copied(start, end - start);
}

zSTRING zCOption::ParmValue(const zSTRING& name)
{
    return ParmValueRaw(name);
}

int zCOption::ParmValueInt(const zSTRING& name)
{
    zSTRING key = "-" + name + " ";
    key.Upper();
    int position = commandline.Search(key);
    if (position < 0)
        return -1;
    return std::atoi(commandline.ToChar() + position + key.Length());
}

float zCOption::ParmValueFloat(const zSTRING& name)
{
    zSTRING key = "-" + name + " ";
    key.Upper();
    int position = commandline.Search(key);
    if (position < 0)
        return -1.0f;
    return (float)std::atof(commandline.ToChar() + position + key.Length());
}
