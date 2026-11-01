#include "include/oGame.h"
#include "include/zMoving.h"

extern void* hWndApp;

void sysEvent();

void TestFont()
{
    zCOLOR red = { 0, 0, 255, 255 };
    do
    {
        zrenderer->Vid_Clear(red, 3);
        zrenderer->Vid_Blit(1, NULL, NULL);
        sysEvent();
    } while (!zinput->KeyPressed(0x1C));
    do
    {
        zrenderer->Vid_Clear(red, 3);
        screen->SetFont(zSTRING());
        screen->Print(0, 0, "Guten Tach !");
        screen->SetFont(zSTRING(1));
        screen->Print(0, 1000, "Guten Tach !");
        zrenderer->Vid_Blit(1, NULL, NULL);
        sysEvent();
    } while (!zinput->KeyPressed(0x1C));
}

int MainProg()
{
    sysEvent();
    CGameManager manager;
    manager.Init(hWndApp);
    manager.Run();
    manager.Done();
    return 0;
}
