#include "include/oGame.h"
#include <windows.h>
#include <process.h>
#include <ctime>

void default_exit()
{
    _exit(1);
}

void ErrorBox(zSTRING text)
{
    text = zSTRING("\n") + text + "\n";
    if (zrenderer)
        zrenderer->Vid_SetScreenMode(1);
    MessageBoxA(GetFocus(), text.ToChar(), "Error-Message", MB_ICONERROR);
}

void MessageWindow(zSTRING)
{
}

void GetDiffTime(zSTRING& result)
{
    static time_t lastTime;
    const time_t now = time(NULL);
    const int seconds = (int)difftime(now, lastTime);
    lastTime = now;
    result.Sprintf("%d:%d", seconds % 60, seconds / 60);
}
