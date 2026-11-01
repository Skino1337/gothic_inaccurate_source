#include "include/zInput_Win32.h"
#include "include/oGame.h"

extern zCOption* zoptions;
extern zSTRING zOPT_SEC_GAME;

static HWND focusWindow;

int ReleaseDInput(void);

zCInput_Win32::zCInput_Win32(HWND** window)
{
    focusWindow = window ? *window : NULL;
    InitDInput(focusWindow, GetModuleHandle(NULL));
    float rate = zoptions->ReadReal(zOPT_SEC_GAME, "keyDelayRate", 150.0f);
    if (rate <= 0.0f)
    {
        rate = 150.0f;
        zoptions->WriteReal(zOPT_SEC_GAME, "keyDelayRate", rate, 0);
    }
    float first = zoptions->ReadReal(zOPT_SEC_GAME, "keyDelayFirst", -1.0f);
    if (first < 0.0f)
    {
        first = 0.0f;
        zoptions->WriteReal(zOPT_SEC_GAME, "keyDelayFirst", first, 0);
    }
    SetAutoRepeatDelayRate(rate, first);
    m_fKeyRepeatTimeMod = 0.0f;
    m_bMouseIdle = 1;
    m_iLastKey = 0;
}

zCInput_Win32::~zCInput_Win32()
{
    ReleaseDInput();
    focusWindow = NULL;
}

void zCInput_Win32::SetFocusWindow(HWND window)
{
    focusWindow = window;
}

int zCInput_Win32::GetMouseIdle() const
{
    return m_bMouseIdle;
}

extern int DInput_KeyPressed(int key);
extern int DInput_KeyToggled(int key);
extern int DInput_AnyKeyPressed();
extern void DInput_ClearKeyBuffer();

static int deviceEnabled[6] = { 0, 1, 1, 1, 0, 0 };
static int deviceConnected[6] = { 0, 1, 1, 1, 0, 0 };

float zCInput_Win32::GetState(unsigned short logicalID)
{
    for (int i = 0; i < mapList.numInArray; ++i)
    {
        zSKeyMapping* mapping = mapList[i];
        if (mapping->logicalID != logicalID)
            continue;
        if (mapping->set)
            return 1.0f;
        for (int j = 0; j < mapping->controlValues.numInArray; ++j)
        {
            if (KeyPressed(mapping->controlValues[j]))
                return 1.0f;
        }
        return 0.0f;
    }
    return 0.0f;
}

int zCInput_Win32::SetState(unsigned short logicalID, int value)
{
    if (GetFocus() != focusWindow)
        return 0;
    for (int i = 0; i < mapList.numInArray; ++i)
    {
        if (mapList[i]->logicalID == logicalID)
        {
            mapList[i]->set = value;
            return 1;
        }
    }
    zCArray<unsigned short> controls;
    controls.InsertEnd(logicalID);
    Bind(logicalID, controls);
    mapList[mapList.numInArray - 1]->set = value;
    return 1;
}

int zCInput_Win32::GetToggled(unsigned short logicalID)
{
    for (int i = 0; i < mapList.numInArray; ++i)
    {
        zSKeyMapping* mapping = mapList[i];
        if (mapping->logicalID != logicalID)
            continue;
        if (mapping->set)
            return 1;
        for (int j = 0; j < mapping->controlValues.numInArray; ++j)
        {
            int control = mapping->controlValues[j];
            if (KeyPressed(control) && KeyToggled(control))
                return 1;
        }
        return 0;
    }
    return 0;
}

void zCInput_Win32::SetDeviceEnabled(zTInputDevice device, int enabled)
{
    if (device >= 0 && device < 6)
        deviceEnabled[device] = enabled;
    for (int i = 0; i < mapList.numInArray; ++i)
        mapList[i]->set = 0;
}

int zCInput_Win32::GetDeviceEnabled(zTInputDevice device)
{
    return device >= 0 && device < 6 ? deviceEnabled[device] : 0;
}

int zCInput_Win32::GetDeviceConnected(zTInputDevice device)
{
    return device >= 0 && device < 6 ? deviceConnected[device] : 0;
}

int zCInput_Win32::KeyPressed(int key)
{
    return DInput_KeyPressed(key);
}

int zCInput_Win32::KeyToggled(int key)
{
    return DInput_KeyToggled(key);
}

int zCInput_Win32::AnyKeyPressed()
{
    return DInput_AnyKeyPressed();
}

void zCInput_Win32::ResetRepeatKey(int)
{
    m_iLastKey = 0;
    m_fKeyRepeatTimeMod = 0.0f;
    m_fKeyRepeatCounter = 0.0f;
    m_fKeyRepeatCounterDelay = m_fKeyRepeatDelay;
}

extern int DInput_GetBufferedKey();
extern void DInput_SetKey(int key, int value);
extern void DInput_ClearInputState();
extern unsigned char GetCharFromDIK(int key, int shift, int caps);

void zCInput_Win32::SetAutoRepeatDelayRate(float time, float delay)
{
    m_fKeyRepeatTime = time;
    m_fKeyRepeatCounter = time;
    m_fKeyRepeatDelay = time + delay;
    m_fKeyRepeatCounterDelay = m_fKeyRepeatDelay;
}

float zCInput_Win32::GetAutoRepeatDelayRate()
{
    return m_fKeyRepeatTime;
}

unsigned short zCInput_Win32::GetKey(int repeat, int)
{
    if (GetFocus() != focusWindow)
        return 0;
    int key = DInput_GetBufferedKey();
    if (key)
    {
        m_kbdTimer.ResetTimer();
        m_iLastKey = (unsigned short)key;
        m_fKeyRepeatCounter = 0.0f;
        m_fKeyRepeatTimeMod = 0.0f;
        if (repeat)
            m_fKeyRepeatCounterDelay = m_fKeyRepeatDelay;
        return (unsigned short)key;
    }
    if (m_fKeyRepeatTime > 0.0f && m_iLastKey && KeyPressed(m_iLastKey))
    {
        m_fKeyRepeatCounter += m_kbdTimer.frameTimeFloat;
        if (m_fKeyRepeatCounter >= m_fKeyRepeatTime + m_fKeyRepeatCounterDelay)
        {
            m_fKeyRepeatCounter = m_fKeyRepeatTimeMod;
            m_fKeyRepeatCounterDelay = 0.0f;
            m_kbdTimer.ResetTimer();
            return m_iLastKey;
        }
    }
    return 0;
}

void zCInput_Win32::SetKey(int key, int modifiers)
{
    if (GetFocus() == focusWindow)
        DInput_SetKey(key, modifiers != 0);
}

unsigned char zCInput_Win32::GetChar()
{
    int key = DInput_GetBufferedKey();
    if (!key)
        return 0;
    int shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    int caps = (GetKeyState(VK_CAPITAL) & 1) != 0;
    return GetCharFromDIK(key, shift, caps);
}

void zCInput_Win32::ClearKeyBuffer()
{
    DInput_ClearInputState();
    for (int i = 0; i < mapList.numInArray; ++i)
        mapList[i]->set = 0;
}

static float mouseSensX = 1.0f;
static float mouseSensY = 1.0f;
static float mouseDeltaX;
static float mouseDeltaY;
static float mouseWheel;
static int mouseFlipX;
static int mouseFlipY;

void zCInput_Win32::ProcessInputEvents()
{
    for (int i = 0; i < mapList.numInArray; ++i)
        mapList[i]->set = 0;
    UpdateDInput();
}

void zCInput_Win32::GetMousePos(float& x, float& y, float& wheel)
{
    x = mouseDeltaX * mouseSensX;
    y = mouseDeltaY * mouseSensY;
    wheel = mouseWheel;
    if (mouseFlipX)
        x = -x;
    if (mouseFlipY)
        y = -y;
}

int zCInput_Win32::GetMouseButtonPressedLeft()
{
    return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
}

int zCInput_Win32::GetMouseButtonPressedMid()
{
    return (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
}

int zCInput_Win32::GetMouseButtonPressedRight()
{
    return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
}

void zCInput_Win32::SetMouseSensitivity(float x, float y)
{
    mouseSensX = x;
    mouseSensY = y;
    if (mouseSensX < 0.0001f)
        mouseSensX = 0.0001f;
    if (mouseSensY < 0.0001f)
        mouseSensY = 0.0001f;
}

void zCInput_Win32::GetMouseSensitivity(float& x, float& y)
{
    x = mouseSensX;
    y = mouseSensY;
}

void zCInput_Win32::SetMouseFlipXY(int x, int y)
{
    mouseFlipX = x;
    mouseFlipY = y;
}

void zCInput_Win32::GetMouseFlipXY(int& x, int& y)
{
    x = mouseFlipX;
    y = mouseFlipY;
}
