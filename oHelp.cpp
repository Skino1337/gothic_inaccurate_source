#include "include/oHelp.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/zSound.h"

oCHelp::oCHelp()
{
    win = new zCView(0, 0, 8192, 8192, 0);
    screen->InsertItem(win, 0);
    static const char* textureNames[] = {
        "k-alt.pcx", "k-alth.pcx", "k-altp.pcx",
        "k-ctrl.pcx", "k-ctrlh.pcx", "k-ctrlp.pcx",
        "k-cup.pcx", "k-cdown.pcx", "k-cleft.pcx", "k-cright.pcx",
        "k-cnone.pcx", "k-both.pcx"
    };
    static zCGfx* textures[12];
    for (int i = 0; i < 12; ++i)
        textures[i] = new zCGfx(textureNames[i]);
    for (int i = 0; i < 4; ++i)
        index[i] = -1;
    led[0] = 0;
    led[1] = 0;
    CalcPos();
    Redraw(0);
}

oCHelp::~oCHelp()
{
    if (win->ondesk)
        screen->RemoveItem(win);
    delete win;
}

void oCHelp::CalcPos()
{
    sx = 2048;
    sy = 2048;
    dx = 512;
    dy = 512;
    x[0] = win->anx(67);
    y[0] = win->any(40);
    x[1] = win->anx(145);
    y[1] = win->any(40);
    x[2] = win->anx(101);
    y[2] = win->any(1);
    x[3] = win->anx(101);
    y[3] = win->any(22);
    ledx[0] = dx;
    ledy[0] = dy;
    ledx[1] = dx * 3 + sx / 2 + sx * 2;
    ledy[1] = dy;
}

void oCHelp::Toggle()
{
    zCSoundFX* sound;
    if (win->ondesk <= 0) {
        screen->InsertItem(win, 0);
        Redraw(0);
        sound = zsound->LoadSoundFXScript("HELP_ON");
    } else {
        screen->RemoveItem(win);
        sound = zsound->LoadSoundFXScript("HELP_OFF");
    }
    zsound->PlaySound(sound, 0);
    if (sound)
        sound->Release();
}

void oCHelp::Update()
{
}

void oCHelp::Redraw(zCVob* vob)
{
    static zCVob* item;
    static zCVob* npc;
    item = 0;
    npc = 0;
    if (!win->ondesk)
        return;
    if (vob) {
        if (vob->type == zVOB_TYPE_ITEM)
            item = vob;
        else if (vob->type == 0x82)
            npc = vob;
    }
    for (int i = 0; i < 4; ++i) {
        if (index[i] < 0)
            continue;
        zSTRING text = parser.GetText("TXT_HELP", index[i]);
        int textWidth = win->FontSize(text);
        int textX = x[i] - textWidth / 2;
        if (i == 1 && textX + textWidth > 8192)
            textX = 8192 - textWidth;
        win->Print(textX, y[i], text);
    }
}
