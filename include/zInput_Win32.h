#ifndef ZINPUT_WIN32_H
#define ZINPUT_WIN32_H

#include "zInput.h"
#include <windows.h>

class zCInput_Win32 : public zCInput {
public:
    zCInput_Win32(HWND** window);
    virtual ~zCInput_Win32();
    void SetFocusWindow(HWND window);
    virtual int GetMouseIdle() const;
    virtual float GetState(unsigned short logicalID);
    virtual int SetState(unsigned short logicalID, int value);
    virtual int GetToggled(unsigned short logicalID);
    virtual void SetDeviceEnabled(zTInputDevice device, int enabled);
    virtual int GetDeviceEnabled(zTInputDevice device);
    virtual int GetDeviceConnected(zTInputDevice device);
    virtual int KeyPressed(int key);
    virtual int KeyToggled(int key);
    virtual int AnyKeyPressed();
    virtual void ResetRepeatKey(int key);
    void SetAutoRepeatDelayRate(float time, float delay);
    float GetAutoRepeatDelayRate();
    virtual unsigned short GetKey(int key, int modifiers);
    virtual void SetKey(int key, int modifiers);
    virtual unsigned char GetChar();
    virtual void ClearKeyBuffer();
    virtual void ProcessInputEvents();
    virtual void GetMousePos(float& x, float& y, float& wheel);
    virtual int GetMouseButtonPressedLeft();
    virtual int GetMouseButtonPressedMid();
    virtual int GetMouseButtonPressedRight();
    virtual void SetMouseSensitivity(float x, float y);
    virtual void GetMouseSensitivity(float& x, float& y);
    virtual void SetMouseFlipXY(int x, int y);
    virtual void GetMouseFlipXY(int& x, int& y);

    float m_fKeyRepeatDelay;
    float m_fKeyRepeatTime;
    float m_fKeyRepeatTimeMod;
    float m_fKeyRepeatCounter;
    float m_fKeyRepeatCounterDelay;
    int m_bMouseIdle;
    unsigned short m_iLastKey;
};

#endif
