#include "include/zInput.h"
#include <dinput.h>

static IDirectInput7A* lpDInput;
static IDirectInputDevice7A* lpDIKeyboard;
static IDirectInputDevice7A* lpDIJoystick;
static unsigned char keyevent[256];
static unsigned char keytoggle[256];
static zCArray<int> keybuffer;
static int inputKeyState = -1;
static int inputBufferState = -1;

int __stdcall _EnumJoysticksCallback(const DIDEVICEINSTANCEA* instance, void*)
{
    if (!lpDInput || !instance)
        return 0;
    HRESULT result = lpDInput->CreateDevice(instance->guidInstance, &lpDIJoystick, NULL);
    return FAILED(result);
}

int __stdcall _EnumJoystickObjectsCallback(const DIDEVICEOBJECTINSTANCEA* object, void*)
{
    if (!lpDIJoystick || !object)
        return 1;
    DIPROPRANGE range;
    range.diph.dwSize = sizeof(range);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwObj = object->dwType;
    range.diph.dwHow = DIPH_BYOFFSET;
    range.lMin = -1000;
    range.lMax = 1000;
    return FAILED(lpDIJoystick->SetProperty(DIPH_BYOFFSET, &range.diph)) ? 0 : 1;
}

int InitDInput(HWND window, HINSTANCE instance)
{
    if (FAILED(DirectInput8Create(instance, DIRECTINPUT_VERSION, IID_IDirectInput7A,
        (void**)&lpDInput, NULL)))
        return 0;
    if (FAILED(lpDInput->CreateDevice(GUID_SysKeyboard, &lpDIKeyboard, NULL)))
        return 0;
    lpDIKeyboard->SetDataFormat(&c_dfDIKeyboard);
    lpDIKeyboard->SetCooperativeLevel(window, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
    lpDIKeyboard->Acquire();
    lpDInput->EnumDevices(DIDEVTYPE_JOYSTICK, _EnumJoysticksCallback, NULL, DIEDFL_ATTACHEDONLY);
    if (lpDIJoystick)
    {
        lpDIJoystick->EnumObjects(_EnumJoystickObjectsCallback, NULL, DIDFT_ALL);
        lpDIJoystick->Acquire();
    }
    for (int i = 0; i < 256; ++i)
    {
        keyevent[i] = 0;
        keytoggle[i] = 0;
    }
    keybuffer.DeleteList();
    return 1;
}

int UpdateDInput()
{
    if (!lpDIKeyboard)
        return 0;
    lpDIKeyboard->Acquire();
    lpDIKeyboard->Poll();
    unsigned char state[256];
    if (FAILED(lpDIKeyboard->GetDeviceState(sizeof(state), state)))
    {
        for (int i = 0; i < 256; ++i)
        {
            keyevent[i] = 0;
            keytoggle[i] = 0;
        }
        return 0;
    }
    for (int i = 0; i < 256; ++i)
    {
        int pressed = (state[i] & 0x80) != 0;
        if (pressed)
        {
            keybuffer.InsertEnd(i);
            keytoggle[i] = !keyevent[i];
        }
        else
            keytoggle[i] = 0;
        keyevent[i] = pressed;
    }
    return 1;
}

int SetAcquireMouse()
{
    return lpDIKeyboard ? SUCCEEDED(lpDIKeyboard->Acquire()) : 0;
}

int SetAcquireJoystick()
{
    return lpDIJoystick ? SUCCEEDED(lpDIJoystick->Acquire()) : 0;
}

int ReleaseDInput()
{
    if (lpDIKeyboard)
    {
        lpDIKeyboard->Unacquire();
        lpDIKeyboard->Release();
        lpDIKeyboard = NULL;
    }
    if (lpDIJoystick)
    {
        lpDIJoystick->Unacquire();
        lpDIJoystick->Release();
        lpDIJoystick = NULL;
    }
    if (lpDInput)
    {
        lpDInput->Release();
        lpDInput = NULL;
    }
    keybuffer.DeleteList();
    return 0;
}

int DInput_KeyPressed(int key)
{
    return keyevent[key];
}

int DInput_KeyToggled(int key)
{
    int result = keytoggle[key];
    keytoggle[key] = 0;
    return result;
}

int DInput_AnyKeyPressed()
{
    for (int i = 0; i < 256; ++i)
    {
        if (keyevent[i])
            return 1;
    }
    return 0;
}

void DInput_ClearKeyBuffer()
{
    keybuffer.DeleteList();
}

int DInput_GetBufferedKey()
{
    if (!keybuffer.numInArray)
        return 0;
    int key = keybuffer[0];
    keybuffer.RemoveOrderIndex(0);
    return key;
}

void DInput_SetKey(int key, int value)
{
    int changed = keyevent[key] != value;
    keyevent[key] = value;
    keytoggle[key] = changed;
}

void DInput_ClearInputState()
{
    keybuffer.DeleteList();
    for (int i = 0; i < 256; ++i)
    {
        keyevent[i] = 0;
        keytoggle[i] = 0;
    }
}
