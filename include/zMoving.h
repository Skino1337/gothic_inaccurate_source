#ifndef ZMOVING_H
#define ZMOVING_H

#include "zModel.h"

unsigned long sysGetTime();
int GetCharFromDIK(int key, int shift, int caps);

class zCInput {
public:

    static zSTRING GetNameByControlValue(unsigned short value);
    unsigned short GetFirstBindedLogicalKey(int key);
    int KeyPressed(int key);
    int KeyToggled(int key);
    int GetKey(int key, int modifiers);
    int GetChar(int key, int modifiers);
    void ProcessInputEvents();
    void ClearKeyBuffer();
    void ResetRepeatKey(int repeat);
    void SetDeviceEnabled(int device, int enabled);
    int GetDeviceEnabled(int device);
    int GetMousePos(float& x, float& y);
    void SetMouseSensitivity(float sensitivity);
};

extern zCInput* zinput;

class CMov_Movement {
public:

    CMov_Movement(zCVob* vob);

    void IncreaseSpeed(double amount);
    void Movement();

    unsigned long oldtime;
    unsigned long now;
    double        time_corr;
    zCVob*        vob;
    double        force_x;
    double        force_y;
    double        force_z;
    double        force_rot;
    double        xspeed;
    double        yspeed;
    double        zspeed;
    double        rotspeed;
};

#endif
