#include "include/zInput.h"
#include "include/oGame.h"

extern zCOption* zoptions;
extern zSTRING zOPT_SEC_KEYS;
extern zSTRING zOPT_SEC_GAME;
extern zSTRING zOPT_SEC_KEYS_DEFAULT0;
extern zSTRING zOPT_SEC_KEYS_DEFAULT1;

static int inputState = -1;
static int inputValuesState = -1;
zCArray<zSCtrlValue> zCInput::values;

zCInput::zCInput()
{
    mapList.compare_func = CompareKeyMaps;
    InitializeControlValues();
}

zCInput::~zCInput()
{
    for (int i = 0; i < mapList.numInArray; ++i)
        delete mapList[i];
    mapList.DeleteList();
}

void zCInput::InitializeControlValues()
{
}

int zCInput::CompareKeyMaps(const void* first, const void* second)
{
    const zSKeyMapping* a = *(const zSKeyMapping* const*)first;
    const zSKeyMapping* b = *(const zSKeyMapping* const*)second;
    return a->logicalID - b->logicalID;
}

void zCInput::Bind(unsigned short logicalID, const zCArray<unsigned short>& controls)
{
    zSKeyMapping* mapping = new zSKeyMapping;
    mapping->logicalID = logicalID;
    mapping->controlValues = controls;
    mapping->set = 0;
    mapList.InsertSort(mapping);
}

void zCInput::BindOption(zSTRING name, unsigned short logicalID, zCArray<unsigned short> controls)
{
    void* data = NULL;
    int bytes = zoptions->ReadRaw(zOPT_SEC_KEYS, name, data, controls.parray,
        controls.numInArray * 2);
    controls.DeleteList();
    unsigned short* loaded = (unsigned short*)data;
    for (int i = 0; i < bytes / 2; ++i)
        if (loaded[i])
            controls.InsertEnd(loaded[i]);
    delete[] loaded;
    Unbind(logicalID);
    Bind(logicalID, controls);
}

void zCInput::Unbind(unsigned short logicalID)
{
    for (int i = 0; i < mapList.numInArray; ++i)
    {
        if (mapList[i]->logicalID == logicalID)
        {
            zSKeyMapping* mapping = mapList[i];
            mapList.RemoveOrderIndex(i);
            delete mapping;
            return;
        }
    }
}

int zCInput::IsBinded(unsigned short logicalID, unsigned short control)
{
    for (int i = 0; i < mapList.numInArray; ++i)
    {
        if (mapList[i]->logicalID == logicalID)
        {
            for (int j = 0; j < mapList[i]->controlValues.numInArray; ++j)
            {
                if (mapList[i]->controlValues[j] == control)
                    return 1;
            }
            return 0;
        }
    }
    return 0;
}

int zCInput::IsBindedToggled(unsigned short logicalID, unsigned short control)
{
    return IsBinded(logicalID, control) && GetToggled(logicalID);
}

int zCInput::GetBinding(unsigned short logicalID, zCArray<unsigned short>& controls)
{
    for (int i = 0; i < mapList.numInArray; ++i)
    {
        if (mapList[i]->logicalID == logicalID)
        {
            controls = mapList[i]->controlValues;
            return 1;
        }
    }
    return 0;
}

unsigned short zCInput::GetFirstBindedLogicalKey(unsigned short control)
{
    for (int i = 0; i < mapList.numInArray; ++i)
    {
        for (int j = 0; j < mapList[i]->controlValues.numInArray; ++j)
        {
            if (mapList[i]->controlValues[j] == control)
                return mapList[i]->logicalID;
        }
    }
    return 0;
}

static void AddControlValue(zCArray<zSCtrlValue>& values, const char* name, unsigned short code)
{
    zSCtrlValue value;
    value.name = name;
    value.value = code;
    values.InsertEnd(value);
}

void zCInput::InitializeControlValues()
{
    if (values.numInArray)
        return;
    AddControlValue(values, "MOUSE-X", 0x204);
    AddControlValue(values, "MOUSE-Y", 0x205);
    AddControlValue(values, "WHEEL UP", 0x20a);
    AddControlValue(values, "WHEEL DOWN", 0x20b);
    AddControlValue(values, "MOUSE LEFT", 0x20c);
    AddControlValue(values, "MOUSE RIGHT", 0x20d);
    AddControlValue(values, "MOUSE MID", 0x20e);
    AddControlValue(values, "MOUSE X1", 0x20f);
    AddControlValue(values, "MOUSE X2", 0x210);
    AddControlValue(values, "MOUSE X3", 0x211);
    AddControlValue(values, "MOUSE X4", 0x212);
    AddControlValue(values, "MOUSE X5", 0x213);
    static const struct { const char* name; unsigned short code; } keyboard[] = {
        { "ESCAPE", 0x01 }, { "1", 0x02 }, { "2", 0x03 }, { "3", 0x04 },
        { "4", 0x05 }, { "5", 0x06 }, { "6", 0x07 }, { "7", 0x08 },
        { "8", 0x09 }, { "9", 0x0a }, { "0", 0x0b }, { "BACKSPACE", 0x0e },
        { "TAB", 0x0f }, { "Q", 0x10 }, { "W", 0x11 }, { "E", 0x12 },
        { "R", 0x13 }, { "T", 0x14 }, { "Y", 0x15 }, { "U", 0x16 },
        { "I", 0x17 }, { "O", 0x18 }, { "P", 0x19 }, { "RETURN", 0x1c },
        { "LCONTROL", 0x1d }, { "A", 0x1e }, { "S", 0x1f }, { "D", 0x20 },
        { "F", 0x21 }, { "G", 0x22 }, { "H", 0x23 }, { "J", 0x24 },
        { "K", 0x25 }, { "L", 0x26 }, { "LSHIFT", 0x2a }, { "Z", 0x2c },
        { "X", 0x2d }, { "C", 0x2e }, { "V", 0x2f }, { "B", 0x30 },
        { "N", 0x31 }, { "M", 0x32 }, { "SPACE", 0x39 },
        { "F1", 0x3b }, { "F2", 0x3c }, { "F3", 0x3d }, { "F4", 0x3e },
        { "F5", 0x3f }, { "F6", 0x40 }, { "F7", 0x41 }, { "F8", 0x42 },
        { "F9", 0x43 }, { "F10", 0x44 }, { "F11", 0x57 }, { "F12", 0x58 },
        { "UP", 0xc8 }, { "LEFT", 0xcb }, { "RIGHT", 0xcd }, { "DOWN", 0xd0 },
        { "INSERT", 0xd2 }, { "DELETE", 0xd3 }, { "HOME", 0xc7 }, { "END", 0xcf },
        { "PAGE UP", 0xc9 }, { "PAGE DOWN", 0xd1 }, { "LALT", 0x38 },
        { "RSHIFT", 0x36 }, { "RCONTROL", 0x9d }
    };
    for (int i = 0; i < sizeof(keyboard) / sizeof(keyboard[0]); ++i)
        AddControlValue(values, keyboard[i].name, keyboard[i].code);
}

static void SetDefaultControls(zCArray<unsigned short>& controls, unsigned short arrow,
    unsigned short wasd)
{
    controls.DeleteList();
    controls.InsertEnd(arrow);
    controls.InsertEnd(wasd);
}

void _GetDefaultBinding(short layout, zSTRING name, zCArray<unsigned short>& controls)
{
    void* data = NULL;
    zSTRING& section = layout ? zOPT_SEC_KEYS_DEFAULT1 : zOPT_SEC_KEYS_DEFAULT0;
    int bytes = zoptions->ReadRaw(section, name, data, controls.parray, controls.numInArray * 2);
    if (bytes > 0)
    {
        controls.DeleteList();
        unsigned short* loaded = (unsigned short*)data;
        for (int i = 0; i < bytes / 2; ++i)
            if (loaded[i])
                controls.InsertEnd(loaded[i]);
        delete[] loaded;
        return;
    }
    delete[] (unsigned short*)data;
    if (name == "ZOPT_GAMEKEY_UP")
        SetDefaultControls(controls, 0xc8, layout ? 0x11 : 0xc7);
    else if (name == "ZOPT_GAMEKEY_DOWN")
        SetDefaultControls(controls, 0xd0, 0x1f);
    else if (name == "ZOPT_GAMEKEY_LEFT")
        SetDefaultControls(controls, 0xcb, 0x1e);
    else if (name == "ZOPT_GAMEKEY_RIGHT")
        SetDefaultControls(controls, 0xcd, 0x20);
    else if (name == "ZOPT_GAMEKEY_ACTION")
        SetDefaultControls(controls, 0x1c, 0x39);
    else if (name == "ZOPT_GAMEKEY_INVENTORY")
        SetDefaultControls(controls, 0x0f, 0x17);
    else if (name == "ZOPT_GAMEKEY_MAP")
        SetDefaultControls(controls, 0x32, 0x19);
    else if (name == "ZOPT_GAMEKEY_MENU")
        SetDefaultControls(controls, 0x01, 0x01);
    else if (name == "ZOPT_GAMEKEY_JUMP")
        SetDefaultControls(controls, 0x39, 0x39);
    else if (name == "ZOPT_GAMEKEY_SNEAK")
        SetDefaultControls(controls, 0x2a, 0x2a);
    else if (name == "ZOPT_GAMEKEY_RUN")
        SetDefaultControls(controls, 0x1d, 0x1d);
}

void zCInput::BindKeys(int layout)
{
    static const char* names[] = {
        "ZOPT_GAMEKEY_UP", "ZOPT_GAMEKEY_DOWN", "ZOPT_GAMEKEY_LEFT", "ZOPT_GAMEKEY_RIGHT",
        "ZOPT_GAMEKEY_STRAFELEFT", "ZOPT_GAMEKEY_STRAFERIGHT", "ZOPT_GAMEKEY_ACTION",
        "ZOPT_GAMEKEY_SLOW", "ZOPT_GAMEKEY_SMOVE", "ZOPT_GAMEKEY_RUN", "ZOPT_GAMEKEY_JUMP",
        "ZOPT_GAMEKEY_SNEAK", "ZOPT_GAMEKEY_INVENTORY", "ZOPT_GAMEKEY_MAP",
        "ZOPT_GAMEKEY_MENU", "ZOPT_GAMEKEY_SCREENSHOT", "ZOPT_GAMEKEY_LOOK",
        "ZOPT_GAMEKEY_FIRSTPERSON", "ZOPT_GAMEKEY_TARGET", "ZOPT_GAMEKEY_TARGETLEFT",
        "ZOPT_GAMEKEY_TARGETRIGHT", "ZOPT_GAMEKEY_WEAPON", "ZOPT_GAMEKEY_WEAPONLEFT",
        "ZOPT_GAMEKEY_WEAPONRIGHT", "ZOPT_GAMEKEY_WEAPONSHEATH", "ZOPT_GAMEKEY_MAGIC",
        "ZOPT_GAMEKEY_RELOAD", "ZOPT_GAMEKEY_QUICKSAVE", "ZOPT_GAMEKEY_QUICKLOAD",
        "ZOPT_GAMEKEY_PAUSE", "ZOPT_GAMEKEY_HELP"
    };
    for (int i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
    {
        zCArray<unsigned short> controls;
        _GetDefaultBinding(layout, zSTRING(names[i]), controls);
        BindOption(zSTRING(names[i]), i + 1, controls);
    }
}

unsigned short zCInput::GetControlValueByName(const zSTRING& name)
{
    for (int i = 0; i < values.numInArray; ++i)
    {
        if (values[i].name == name)
            return values[i].value;
    }
    return 0;
}

zSTRING zCInput::GetNameByControlValue(unsigned short value)
{
    for (int i = 0; i < values.numInArray; ++i)
    {
        if (values[i].value == value)
            return values[i].name;
    }
    return zSTRING("");
}

float zCInput::GetState(unsigned short) { return 0.0f; }
int zCInput::SetState(unsigned short, int) { return 1; }
int zCInput::GetToggled(unsigned short) { return 1; }
void zCInput::SetDeviceEnabled(zTInputDevice, int) {}
int zCInput::GetDeviceEnabled(zTInputDevice) { return 1; }
int zCInput::GetDeviceConnected(zTInputDevice) { return 1; }
int zCInput::KeyPressed(int) { return 0; }
int zCInput::KeyToggled(int) { return 0; }
int zCInput::AnyKeyPressed() { return 0; }
void zCInput::ResetRepeatKey(int) {}
unsigned short zCInput::GetKey(int, int) { return 0; }
void zCInput::SetKey(int, int) {}
unsigned char zCInput::GetChar() { return 0; }
void zCInput::ClearKeyBuffer() {}
int zCInput::GetNumJoysConnected() { return 0; }
void zCInput::SetJoyDigitalEmu(int) {}
void zCInput::SetJoyEnabled(int) {}
float zCInput::JoyState(int) { return 0.0f; }
float zCInput::JoyState(int, int) { return 0.0f; }
void zCInput::GetMousePos(float&, float&, float&) {}
int zCInput::GetMouseButtonPressedLeft() { return 0; }
int zCInput::GetMouseButtonPressedMid() { return 0; }
int zCInput::GetMouseButtonPressedRight() { return 0; }
void zCInput::SetMouseSensitivity(float, float) {}
void zCInput::GetMouseSensitivity(float&, float&) {}
int zCInput::GetMouseIdle() const { return 0; }
void zCInput::SetMouseFlipXY(int, int) {}
void zCInput::GetMouseFlipXY(int&, int&) {}
