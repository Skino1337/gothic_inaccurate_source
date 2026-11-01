#include "zMoving.h"
#include <windows.h>

static int movementState = -1;
static double movementSpeedX = 20;
static double movementSpeedY = 20;
static double movementSpeedZ = 20;

struct zTIMER {
    float frameTimeFloat;
};

extern zTIMER ztimer;

static double IncreaseMovementSpeed(double speed, double amount)
{
    const double result = speed + amount;
    return result >= 20 ? result : 20;
}

CMov_Movement::CMov_Movement(zCVob* vob)
    : vob(vob), rotspeed(0), xspeed(0), yspeed(0), zspeed(0)
{
    oldtime = sysGetTime();
}

void CMov_Movement::IncreaseSpeed(double amount)
{
    movementSpeedX = IncreaseMovementSpeed(movementSpeedX, amount);
    movementSpeedY = IncreaseMovementSpeed(movementSpeedY, amount);
    movementSpeedZ = IncreaseMovementSpeed(movementSpeedZ, amount);
}

void CMov_Movement::Movement()
{
    if (!vob)
        return;
    vob->BeginMovement();

    force_y = 0;
    if (zinput->KeyPressed(0x48) || zinput->KeyPressed(0xC8))
        force_y += movementSpeedZ;
    if (zinput->KeyPressed(0x50) || zinput->KeyPressed(0xD0))
        force_y -= movementSpeedZ;

    force_rot = 0;
    if (zinput->KeyPressed(0x51))
        force_rot += movementSpeedX;
    if (zinput->KeyPressed(0x4F))
        force_rot -= movementSpeedX;

    force_x = 0;
    if (zinput->KeyPressed(0x1E))
        force_x += movementSpeedY;
    if (zinput->KeyPressed(0x2C))
        force_x -= movementSpeedY;

    force_z = 0;
    if (zinput->KeyPressed(0x4B) || zinput->KeyPressed(0xCB))
        force_z -= 2.0;
    if (zinput->KeyPressed(0x4D) || zinput->KeyPressed(0xCD))
        force_z += 2.0;

    const double rotation = ztimer.frameTimeFloat * 0.1;
    zVEC3 axisX;
    axisX.n[0] = 1;
    axisX.n[1] = 0;
    axisX.n[2] = 0;
    zVEC3 axisY;
    axisY.n[0] = 0;
    axisY.n[1] = 1;
    axisY.n[2] = 0;
    zVEC3 axisZ;
    axisZ.n[0] = 0;
    axisZ.n[1] = 0;
    axisZ.n[2] = 1;
    const int local = zinput->KeyPressed(0x2A);

    if (zinput->KeyPressed(0xD2)) {
        if (local)
            vob->RotateLocal(axisY, -rotation);
        else
            vob->RotateWorldY(-rotation);
    }
    if (zinput->KeyPressed(0xD3)) {
        if (local)
            vob->RotateLocal(axisY, rotation);
        else
            vob->RotateWorldY(rotation);
    }
    if (zinput->KeyPressed(0xC7)) {
        if (local)
            vob->RotateLocal(axisX, -rotation);
        else
            vob->RotateWorldX(-rotation);
    }
    if (zinput->KeyPressed(0xCF)) {
        if (local)
            vob->RotateLocal(axisX, rotation);
        else
            vob->RotateWorldX(rotation);
    }
    if (zinput->KeyPressed(0xC9)) {
        if (local)
            vob->RotateLocal(axisZ, -rotation);
        else
            vob->RotateWorldZ(-rotation);
    }
    if (zinput->KeyPressed(0xD1)) {
        if (local)
            vob->RotateLocal(axisZ, rotation);
        else
            vob->RotateWorldZ(rotation);
    }

    if (!zinput->KeyPressed(0x36)) {
        if (zinput->KeyPressed(0x1B))
            IncreaseSpeed(50);
        if (zinput->KeyPressed(0x1A))
            IncreaseSpeed(-50);
    }

    now = sysGetTime();
    time_corr = static_cast<double>(now - oldtime) * 0.015625;
    oldtime = now;
    const double damping = 0.2 * time_corr;
    rotspeed += force_rot / 60.0 * time_corr - damping * rotspeed;
    xspeed += force_x / 60.0 * time_corr - damping * xspeed;
    yspeed += force_y / 60.0 * time_corr - damping * yspeed;
    zspeed += force_z * time_corr - 0.3 * time_corr * zspeed;

    if (!vob->GetCharacterClass()) {
        vob->MoveLocal(rotspeed, xspeed, yspeed);
        vob->RotateWorld(axisY, zspeed * time_corr);
    }
    vob->EndMovement(1);
}

unsigned char GetCharFromDIK(int key, int shift, int caps)
{
    if (key < 0 || key >= 256)
        return 0;
    unsigned char state[256] = {};
    if (shift)
        state[VK_SHIFT] = 0x80;
    if (caps)
        state[VK_CAPITAL] = 0x01;
    unsigned char result = 0;
    if (ToAscii((unsigned int)key, (unsigned int)key, state, (LPWORD)&result, 0) > 0)
        return result;
    return 0;
}
