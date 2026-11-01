#ifndef ZINPUT_H
#define ZINPUT_H

#include "zObject.h"

enum zTInputDevice {
    zINPUT_UNDEF,
    zINPUT_KEYBOARD,
    zINPUT_MOUSE,
    zINPUT_JOYSTICK1,
    zINPUT_JOYSTICK2,
    zINPUT_JOYSTICK3,
    zINPUT_JOYSTICK4
};

struct zSKeyMapping {
    unsigned short logicalID;
    zCArray<unsigned short> controlValues;
    int set;
};

struct zSCtrlValue {
    zSTRING name;
    unsigned short value;
};

class zCInput {
public:
    zCInput();
    virtual ~zCInput();
    void InitializeControlValues();
    void BindKeys(int layout);
    static int CompareKeyMaps(const void* first, const void* second);
    void Bind(unsigned short logicalID, const zCArray<unsigned short>& controls);
    void BindOption(zSTRING name, unsigned short logicalID, zCArray<unsigned short> controls);
    void Unbind(unsigned short logicalID);
    int IsBinded(unsigned short logicalID, unsigned short control);
    int IsBindedToggled(unsigned short logicalID, unsigned short control);
    int GetBinding(unsigned short logicalID, zCArray<unsigned short>& controls);
    unsigned short GetFirstBindedLogicalKey(unsigned short control);
    static unsigned short GetControlValueByName(const zSTRING& name);
    static zSTRING GetNameByControlValue(unsigned short value);
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
    virtual unsigned short GetKey(int key, int modifiers);
    virtual void SetKey(int key, int modifiers);
    virtual unsigned char GetChar();
    virtual void ClearKeyBuffer();
    virtual int GetNumJoysConnected();
    virtual void SetJoyDigitalEmu(int enabled);
    virtual void SetJoyEnabled(int enabled);
    virtual float JoyState(int axis);
    virtual float JoyState(int axis, int joy);
    virtual void GetMousePos(float& x, float& y, float& wheel);
    virtual int GetMouseButtonPressedLeft();
    virtual int GetMouseButtonPressedMid();
    virtual int GetMouseButtonPressedRight();
    virtual void SetMouseSensitivity(float x, float y);
    virtual void GetMouseSensitivity(float& x, float& y);
    virtual int GetMouseIdle() const;
    virtual void SetMouseFlipXY(int x, int y);
    virtual void GetMouseFlipXY(int& x, int& y);

    zCArraySort<zSKeyMapping*> mapList;
    static zCArray<zSCtrlValue> values;
};

#endif
