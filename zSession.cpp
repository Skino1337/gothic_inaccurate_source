#include "include/zSession.h"

static int graphicsState = -1;
static int inputState = -1;
static int inputRuntimeState = -1;
static zSTRING inputFontDefault("font_default.tga");
zCOLOR GFX_WHITE(255, 255, 255, 255);
zCOLOR GFX_YELLOW(255, 255, 0, 255);
zCOLOR GFX_LYELLOW(255, 255, 224, 255);
zCOLOR GFX_GOLD(255, 215, 0, 255);
zCOLOR GFX_ORANGE(255, 128, 0, 255);
zCOLOR GFX_DORANGE(255, 140, 0, 255);
zCOLOR GFX_CARROT(237, 145, 33, 255);
zCOLOR GFX_RED(255, 0, 0, 255);
zCOLOR GFX_DRED(255, 99, 71, 255);
zCOLOR GFX_PINK(255, 192, 203, 255);
zCOLOR GFX_LPINK(255, 182, 193, 255);
zCOLOR GFX_DPINK(255, 20, 147, 255);
zCOLOR GFX_MAGENTA(255, 0, 255, 255);
zCOLOR GFX_ORCHID(218, 112, 214, 255);
zCOLOR GFX_PURPLE(160, 32, 240, 255);
zCOLOR GFX_VIOLET(143, 94, 153, 255);
zCOLOR GFX_CYAN(0, 255, 255, 255);
zCOLOR GFX_AQUA(127, 255, 212, 255);
zCOLOR GFX_GREEN(0, 255, 0, 255);
zCOLOR GFX_DGREEN(0, 100, 0, 255);
zCOLOR GFX_PALEGREEN(152, 251, 152, 255);
zCOLOR GFX_OLIVE(59, 94, 43, 255);
zCOLOR GFX_BLUE(0, 0, 255, 255);
zCOLOR GFX_LBLUE(173, 216, 230, 255);
zCOLOR GFX_MBLUE(0, 0, 205, 255);
zCOLOR GFX_INDIGO(8, 46, 84, 255);
zCOLOR GFX_DBLUE(0, 0, 128, 255);
zCOLOR GFX_SKY(135, 206, 235, 255);
zCOLOR GFX_STEEL(70, 130, 180, 255);
zCOLOR GFX_BROWN(128, 42, 42, 255);
zCOLOR GFX_OCHRE(135, 66, 31, 255);
zCOLOR GFX_DOCHRE(115, 61, 26, 255);
zCOLOR GFX_BEIGE(163, 148, 128, 255);
zCOLOR GFX_FLESH(255, 125, 64, 255);
zCOLOR GFX_KHAKI(240, 230, 140, 255);
zCOLOR GFX_GREY(192, 192, 192, 255);
zCOLOR GFX_LGREY(211, 211, 211, 255);
zCOLOR GFX_COLDGREY(128, 138, 135, 255);
zCOLOR GFX_WARMGREY(128, 128, 105, 255);
zCOLOR GFX_BLACK(0, 0, 0, 255);
zCOLOR GFX_IVORY(41, 36, 33, 255);

void sysEvent();
void sysIdle(unsigned long milliseconds);

zCList<zCInputCallback> zCInputCallback::inputList;

zCInputCallback::zCInputCallback()
{
}

zCInputCallback::~zCInputCallback()
{
    inputList.Remove(this);
}

void zCInputCallback::SetHandleEventTop()
{
    inputList.Remove(this);
    inputList.Insert(this);
}

void zCInputCallback::SetEnableHandleEvent(int enabled)
{
    if (enabled)
    {
        if (!GetEnableHandleEvent())
            inputList.Insert(this);
    }
    else
        inputList.Remove(this);
}

int zCInputCallback::HandleEvent(int)
{
    return 0;
}

int zCInputCallback::GetEnableHandleEvent() const
{
    for (zCList<zCInputCallback>* entry = inputList.next; entry; entry = entry->next)
    {
        if (entry->data == this)
            return 1;
    }
    return 0;
}

int zCInputCallback::DoEvents(int key)
{
    for (zCList<zCInputCallback>* entry = inputList.next; entry; entry = entry->next)
    {
        if (entry->data->HandleEvent(key))
            return 1;
    }
    return 0;
}

void zCInputCallback::GetInput()
{
    int key = zinput->GetKey(1, 1);
    if (key)
    {
        for (zCList<zCInputCallback>* entry = inputList.next; entry; entry = entry->next)
        {
            if (entry->data->HandleEvent(key))
                break;
        }
    }
    zinput->ProcessInputEvents();
}

zCSession::zCSession()
    : csMan(0), world(0), camera(0), aiCam(0), camVob(0), viewport(0)
{
}

zCSession::~zCSession()
{
    Done();
}

void zCSession::SetCamera(zCCamera* cameraValue)
{
    camera = cameraValue;
}

zCCamera* zCSession::GetCamera() const
{
    return camera;
}

zCAICamera* zCSession::GetCameraAI() const
{
    return aiCam;
}

zCVob* zCSession::GetCameraVob() const
{
    return camVob;
}

zCView* zCSession::GetViewport() const
{
    return viewport;
}

zCCSManager* zCSession::GetCutsceneManager()
{
    return csMan;
}

zCView* zCView::modal = NULL;

zCView::zCView()
    : m_bFillZ(0), next(NULL), viewID(2), flags(0), intflags(0), ondesk(0), alphafunc(0),
      alpha(0), owner(NULL), backTex(NULL), vposx(0), vposy(0), vsizex(0), vsizey(0), pposx(0),
      pposy(0), psizex(0), psizey(0), font(NULL), px1(0), py1(0), px2(0), py2(0), winx(0),
      winy(0), scrollMaxTime(0), scrollTimer(0), fxOpen(0), fxClose(0), timeDialog(0),
      timeOpen(0), timeClose(0), speedOpen(0), speedClose(0), isOpen(0), isClosed(0),
      continueOpen(0), continueClose(0), removeOnClose(0), resizeOnOpen(0), maxTextLength(0),
      animatedWindows(0), animatedAlpha(0)
{
    Init();
}

zCView::zCView(int x1, int y1, int x2, int y2, int viewID)
    : zCView()
{
    Init(x1, y1, x2, y2, viewID);
}

zCView::~zCView()
{
    if (owner)
    {
        owner->RemoveItem(this);
        owner = NULL;
        ondesk = 0;
        SetEnableHandleEvent(0);
        if (flags & 0x40)
            modal = NULL;
    }
    font = NULL;
    owner = NULL;
    next = NULL;
    if (backTex)
    {
        backTex->Release();
        backTex = NULL;
    }
    delete textLines.data;
    textLines.data = NULL;
    textLines.DeleteListDatas();
    textLines.DeleteList();
}

zCView* screen = NULL;
zCFontMan* zfontman = NULL;

void zCView::Init()
{
    viewID = 2;
    flags = 0;
    intflags = 0;
    ondesk = 0;
    alpha = 255;
    alphafunc = 1;
    owner = NULL;
    backTex = NULL;
    vposx = 0;
    vposy = 0;
    vsizex = 0;
    vsizey = 0;
    pposx = 0;
    pposy = 0;
    psizex = 0;
    psizey = 0;
    font = NULL;
    px1 = 0;
    py1 = 0;
    px2 = 0;
    py2 = 0;
    winx = 0;
    winy = 0;
    scrollMaxTime = 0;
    scrollTimer = 0;
    next = NULL;
    color = GFX_WHITE;
    fontColor = GFX_WHITE;
    fxOpen = 1;
    fxClose = 1;
    isOpen = 1;
    isClosed = 0;
    continueOpen = 0;
    continueClose = 0;
    removeOnClose = 0;
    resizeOnOpen = 0;
    textMaxLength = "";
    timeOpen = 0;
    timeClose = 0;
    timeDialog = 0;
    m_bFillZ = 0;
    speedOpen = 0.003f;
    speedClose = 0.003f;
}

void zCView::Init(int x1, int y1, int x2, int y2, int viewID)
{
    this->viewID = viewID;
    px1 = 0;
    py1 = 0;
    px2 = 0x1FFF;
    py2 = 0x1FFF;
    if (viewID)
    {
        int hadOwner = owner != NULL;
        if (!owner)
            owner = screen;
        vposx = x1;
        vposy = y1;
        pposx = 0;
        pposy = 0;
        if (owner)
        {
            pposx += owner->nax(x1);
            pposy += owner->nay(y1);
            owner->CalcPixelPos(pposx, pposy);
        }
        for (zCList<zCView>* entry = childs.next; entry; entry = entry->next)
        {
            entry->data->pposx = 0;
            entry->data->pposy = 0;
            entry->data->CalcPixelPos(entry->data->pposx, entry->data->pposy);
            entry->data->RecalcChildsPos();
        }
        vsizex = x2 - x1 + 1;
        if (vsizex <= 0)
            vsizex = 1;
        vsizey = y2 - y1 + 1;
        if (vsizey <= 0)
            vsizey = 1;
        psizex = 0;
        psizey = 0;
        if (owner)
        {
            psizex = owner->nax(vsizex);
            psizey = owner->nay(vsizey);
        }
        if (psizex <= 0)
            psizex = 1;
        if (psizey <= 0)
            psizey = 1;
        if (!hadOwner)
            owner = NULL;
    }
    else
    {
        vposx = 0;
        vposy = 0;
        vsizex = x2 - x1 + 1;
        vsizey = y2 - y1 + 1;
        pposx = 0;
        pposy = 0;
        psizex = zrenderer->vid_xdim;
        psizey = zrenderer->vid_ydim;
    }
    font = zfontman ? zfontman->GetFont(0) : NULL;
    flags |= 0x200;
}

void zCView::Init(int x, int y, const zSTRING& text, int viewID)
{
    Init();
    this->viewID = viewID;
    flags = 0x400;
    if (viewID)
    {
        int hadOwner = owner != NULL;
        if (!owner)
            owner = screen;
        vposx = x;
        vposy = y;
        pposx = 0;
        pposy = 0;
        if (owner)
        {
            pposx += owner->nax(x);
            pposy += owner->nay(y);
            owner->CalcPixelPos(pposx, pposy);
        }
        for (zCList<zCView>* entry = childs.next; entry; entry = entry->next)
        {
            zCView* child = entry->data;
            child->pposx = 0;
            child->pposy = 0;
            if (child->owner)
            {
                child->pposx += child->owner->nax(child->vposx);
                child->pposy += child->owner->nay(child->vposy);
                child->owner->CalcPixelPos(child->pposx, child->pposy);
            }
            child->RecalcChildsPos();
        }
        if (!hadOwner)
            owner = NULL;
    }
    else
    {
        vposx = 0;
        vposy = 0;
        pposx = 0;
        pposy = 0;
    }
    CreateText(x, y, text);
}

int rnd2(float value)
{
    return (int)(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

int zCView::nax(int x)
{
    return rnd2(psizex * x * 0.000122070312f);
}

int zCView::nay(int y)
{
    return rnd2(psizey * y * 0.000122070312f);
}

int zCView::anx(int x)
{
    return rnd2((float)(x << 13) / psizex);
}

int zCView::any(int y)
{
    return rnd2((float)(y << 13) / psizey);
}

zCView* zCView::stdoutput = NULL;

void zCView::StdPrintwin(const zSTRING& text)
{
    if (stdoutput)
        stdoutput->Printwin(text);
}

void zCView::SetAutoScroll(float time)
{
    if (time == 0.0f)
    {
        flags &= ~0x100;
        return;
    }
    flags |= 0x100;
    scrollMaxTime = time;
    scrollTimer = 0;
}

void zCView::CheckAutoScroll()
{
    if (!(flags & 0x100))
        return;
    scrollTimer += ztimer.frameTime / ztimer.factorMotion;
    if (scrollTimer >= scrollMaxTime)
    {
        Printwin("\n");
        scrollTimer -= scrollMaxTime;
    }
}

void zCView::SetTransparency(int alpha)
{
    this->alpha = alpha;
}

void zCView::SetAlphaBlendFunc(int function)
{
    alphafunc = function;
}

void zCView::SetColor(const zCOLOR& color)
{
    this->color = color;
}

zCOLOR zCView::GetColor()
{
    return color;
}

void zCView::Setflags(int flags)
{
    this->flags |= flags;
}

void zCView::ClrFlags(int flags)
{
    this->flags &= ~flags;
}

int zCView::HasFlag(int flag)
{
    return (flags & flag) != 0;
}

void zCView::InsertBack(zCTexture* texture)
{
    if (backTex)
        backTex->Release();
    backTex = texture;
    if (backTex)
        backTex->AddRef();
}

void zCView::InsertBack(const zSTRING& name)
{
    zSTRING fileName = name;
    zCTexture* texture = zCTexture::Load(fileName, 1);
    InsertBack(texture);
    if (texture)
        texture->Release();
}

void zCView::InitPrintwin()
{
    delete textLines.data;
    textLines.data = NULL;
    textLines.DeleteListDatas();
    textLines.DeleteList();
    intflags |= 1;
}

void zCView::BlitText()
{
    CheckAutoScroll();
    zCList<zCViewText>* entry = textLines.next;
    while (entry)
    {
        zCList<zCViewText>* next = entry->next;
        zCViewText* text = entry->data;
        if (text && text->timed)
        {
            text->timer -= ztimer.frameTime / ztimer.factorMotion;
            if (text->timer <= 0.0f)
            {
                textLines.Remove(text);
                delete text;
            }
        }
        entry = next;
    }
    if (!isOpen)
        return;
    for (entry = textLines.next; entry; entry = entry->next)
    {
        zCViewText* text = entry->data;
        font = text->font ? text->font : zfontman->GetFont(0);
        if (text->colored)
            fontColor = text->color;
        PrintChars(nax(text->posx) + pposx, nay(text->posy) + pposy, text->text);
    }
}

void zCView::Blit()
{
    if (this == screen || viewID == 1)
        return;
    int oldWrite = zrenderer->GetZBufferWriteEnabled();
    zTRnd_ZBufferCmp oldCompare = zrenderer->GetZBufferCompare();
    zTRnd_AlphaBlendFunc oldAlpha = zrenderer->GetAlphaBlendFunc();
    zrenderer->SetZBufferWriteEnabled(m_bFillZ != 0);
    zVEC2 topLeft(pposx, pposy);
    zVEC2 bottomRight(pposx + psizex - 1, pposy + psizey - 1);
    ApplyOpenClose(topLeft, bottomRight);
    if (topLeft.n[0] < 0.0f)
        topLeft.n[0] = 0.0f;
    if (topLeft.n[1] < 0.0f)
        topLeft.n[1] = 0.0f;
    if (bottomRight.n[0] > zrenderer->vid_xdim - 1.0f)
        bottomRight.n[0] = zrenderer->vid_xdim - 1.0f;
    if (bottomRight.n[1] > zrenderer->vid_ydim - 1.0f)
        bottomRight.n[1] = zrenderer->vid_ydim - 1.0f;
    if (bottomRight.n[0] - topLeft.n[0] <= 0.0f || bottomRight.n[1] - topLeft.n[1] <= 0.0f)
        return;
    zrenderer->SetViewport(int(topLeft.n[0]), int(topLeft.n[1]),
        int(bottomRight.n[0] - topLeft.n[0]), int(bottomRight.n[1] - topLeft.n[1]));
    zrenderer->SetAlphaBlendFunc((zTRnd_AlphaBlendFunc)alphafunc);
    color.a = (unsigned char)alpha;
    if (backTex)
    {
        int oldFilter = zrenderer->GetBilerpFilterEnabled();
        zrenderer->SetBilerpFilterEnabled(1);
        float depth = 1.0f;
        if (m_bFillZ)
            depth = zCCamera::activeCam ? zCCamera::activeCam->farClipZ - 1.0f : 65534.0f;
        else if (zCCamera::activeCam)
            depth = zCCamera::activeCam->nearClipZ + 1.0f;
        zVEC2 uvTopLeft(0.0f, 0.0f);
        zVEC2 uvBottomRight(1.0f, 1.0f);
        zrenderer->DrawTile(backTex, topLeft, bottomRight, depth, uvTopLeft, uvBottomRight,
            *(unsigned long*)&color);
        zrenderer->SetBilerpFilterEnabled(oldFilter);
    }
    BlitText();
    zrenderer->SetAlphaBlendFunc(oldAlpha);
    zrenderer->SetZBufferWriteEnabled(oldWrite);
    zrenderer->SetZBufferCompare(oldCompare);
}

void zCView::DrawItems()
{
    Blit();
    for (zCList<zCView>* entry = childs.next; entry; entry = entry->next)
        entry->data->DrawItems();
    zrenderer->SetViewport(0, 0, zrenderer->vid_xdim, zrenderer->vid_ydim);
}

void zCView::Top()
{
    zCView* current = this;
    while (current->owner && !(current->flags & 8) && current->viewID != 1)
    {
        zCView* parent = current->owner;
        parent->RemoveItem(current);
        parent->InsertItem(current, 0);
        current = parent;
    }
}

void zCView::Create(const zSTRING& backName, int openEffect, int closeEffect, float scrollTime,
    int resize)
{
    fxOpen = openEffect;
    fxClose = closeEffect;
    resizeOnOpen = resize;
    if (!backName.IsEmpty())
        InsertBack(backName);
    if (scrollTime != -2.0f)
        SetAutoScroll(scrollTime);
    textMaxLength = "";
    maxTextLength = 0;
    Close();
    alphafunc = 2;
}

void zCView::ForceOpen()
{
    if (continueClose)
        zerr.Warning("U: VIE: Window was still closing while trying to open ...");
    textMaxLength = "";
    px1 = 50;
    py1 = 50;
    px2 = 8142;
    py2 = 8142;
    winx = 50;
    winy = 50;
    maxTextLength = 0;
    timeOpen = 0;
    timeClose = 0;
    isOpen = 0;
    isClosed = 0;
    continueOpen = 1;
    continueClose = 0;
    if (!(intflags & 1))
    {
        textLines.DeleteListDatas();
        intflags |= 1;
    }
    InitPrintwin();
    winx = px1;
    winy = py1;
    screen->InsertItem(this, 0);
}

void zCView::Open()
{
    textMaxLength = "";
    maxTextLength = 0;
    if (timeOpen != 0.0f || timeClose != 0.0f)
        return;
    px1 = 50;
    py1 = 50;
    px2 = 8142;
    py2 = 8142;
    winx = 50;
    winy = 50;
    timeOpen = 0;
    timeClose = 0;
    isOpen = 0;
    isClosed = 0;
    continueOpen = 1;
    continueClose = 0;
    if (!(intflags & 1))
        intflags |= 1;
    InitPrintwin();
    winx = px1;
    winy = py1;
    screen->InsertItem(this, 0);
}

void zCView::Close()
{
    textMaxLength = "";
    maxTextLength = 0;
    if (timeClose != 0.0f)
        return;
    if (!continueOpen)
        timeClose = 0;
    else
        timeClose = 1.0f - timeOpen;
    timeOpen = 0;
    isOpen = 0;
    isClosed = 0;
    continueOpen = 0;
    continueClose = 1;
}

void zCView::ApplyOpenClose(zVEC2& topLeft, zVEC2& bottomRight)
{
    if (timeDialog > 0.0f)
    {
        timeDialog -= ztimer.frameTime / ztimer.factorMotion;
        if (timeDialog <= 0.0f)
        {
            timeDialog = 0;
            Close();
        }
    }
    if (!continueOpen && !continueClose)
        return;
    posCurrent[0] = topLeft;
    posCurrent[1] = bottomRight;
    if (continueOpen)
        UpdateOpen();
    else
        UpdateClose();
    topLeft = posOpenClose[0];
    bottomRight = posOpenClose[1];
}

void zCView::UpdateOpen()
{
    if (timeOpen >= 1.0f)
    {
        timeOpen = 0;
        continueOpen = 0;
        isOpen = 1;
        isClosed = 0;
        return;
    }
    if (fxOpen == 1)
        UpdateOpenZoom();
    else
        timeOpen = 2.0f;
}

void zCView::UpdateClose()
{
    if (timeClose < 1.0f)
    {
        if (fxClose == 1)
            UpdateCloseZoom();
        else
            timeClose = 2.0f;
        return;
    }
    timeClose = 0;
    continueClose = 0;
    isOpen = 0;
    isClosed = 1;
    if (owner == screen)
    {
        screen->RemoveItem(this);
        owner = NULL;
        ondesk = 0;
        SetEnableHandleEvent(0);
        if (flags & 0x40)
            modal = NULL;
    }
}

void zCView::UpdateOpenZoom()
{
    float centerX = (posCurrent[1].n[0] + posCurrent[0].n[0]) * 0.5f;
    float centerY = (posCurrent[1].n[1] + posCurrent[0].n[1]) * 0.5f;
    float halfX = centerX - posCurrent[0].n[0];
    float halfY = centerY - posCurrent[0].n[1];
    posOpenClose[0].n[0] = centerX - halfX * timeOpen;
    posOpenClose[0].n[1] = centerY - halfY * timeOpen;
    posOpenClose[1].n[0] = centerX + halfX * timeOpen;
    posOpenClose[1].n[1] = centerY + halfY * timeOpen;
    timeOpen += ztimer.frameTime / ztimer.factorMotion * speedOpen;
    if (timeOpen > 1.0f)
        timeOpen = 1.0f;
}

void zCView::UpdateCloseZoom()
{
    float centerX = (posCurrent[1].n[0] + posCurrent[0].n[0]) * 0.5f;
    float centerY = (posCurrent[1].n[1] + posCurrent[0].n[1]) * 0.5f;
    float halfX = centerX - posCurrent[0].n[0];
    float halfY = centerY - posCurrent[0].n[1];
    float remaining = 1.0f - timeClose;
    posOpenClose[0].n[0] = centerX - remaining * halfX;
    posOpenClose[0].n[1] = centerY - remaining * halfY;
    posOpenClose[1].n[0] = centerX + remaining * halfX;
    posOpenClose[1].n[1] = centerY + remaining * halfY;
    timeClose += ztimer.frameTime / ztimer.factorMotion * speedClose;
    if (timeClose > 1.0f)
        timeClose = 1.0f;
}

void zCView::RecalcChildsSize()
{
    for (zCList<zCView>* entry = childs.next; entry; entry = entry->next)
    {
        zCView* child = entry->data;
        if (child->vsizex <= 0)
            child->vsizex = 1;
        if (child->vsizey <= 0)
            child->vsizey = 1;
        child->psizex = 0;
        child->psizey = 0;
        if (child->owner)
        {
            child->psizex = child->owner->nax(child->vsizex);
            child->psizey = child->owner->nay(child->vsizey);
        }
        if (child->psizex <= 0)
            child->psizex = 1;
        if (child->psizey <= 0)
            child->psizey = 1;
        child->RecalcChildsSize();
    }
}

void zCView::RecalcChildsPos()
{
    for (zCList<zCView>* entry = childs.next; entry; entry = entry->next)
    {
        zCView* child = entry->data;
        child->pposx = 0;
        child->pposy = 0;
        if (child->owner)
        {
            child->pposx += child->owner->nax(child->vposx);
            child->pposy += child->owner->nay(child->vposy);
            child->owner->CalcPixelPos(child->pposx, child->pposy);
        }
        child->RecalcChildsPos();
    }
}

void zCView::SetPos(int x, int y)
{
    vposx = x;
    vposy = y;
    pposx = 0;
    pposy = 0;
    if (owner)
    {
        pposx += owner->nax(x);
        pposy += owner->nay(y);
        owner->CalcPixelPos(pposx, pposy);
    }
    RecalcChildsPos();
}

void zCView::GetPos(int& x, int& y)
{
    x = vposx;
    y = vposy;
}

void zCView::CalcPixelPos(int& x, int& y)
{
    zCView* current = this;
    while (current->owner)
    {
        current = current->owner;
        x += current->nax(current->vposx);
        y += current->nay(current->vposy);
    }
}

void zCView::GetPixelPos(int& x, int& y)
{
    x = pposx;
    y = pposy;
}

void zCView::Move(int x, int y)
{
    SetPos(vposx + x, vposy + y);
}

void zCView::SetSize(int x, int y)
{
    vsizex = x > 0 ? x : 1;
    vsizey = y > 0 ? y : 1;
    psizex = 0;
    psizey = 0;
    if (owner)
    {
        psizex = owner->nax(vsizex);
        psizey = owner->nay(vsizey);
    }
    if (psizex <= 0)
        psizex = 1;
    if (psizey <= 0)
        psizey = 1;
}

void zCView::GetSize(int& x, int& y)
{
    x = vsizex;
    y = vsizey;
}

void zCView::GetPixelSize(int& x, int& y)
{
    if (owner)
    {
        x = owner->nax(vsizex);
        y = owner->nay(vsizey);
    }
}

void zCView::GetViewport(int& x, int& y, int& width, int& height)
{
    x = pposx;
    y = pposy;
    width = psizex;
    height = psizey;
}

void zCView::GetExtends(int& x1, int& y1, int& x2, int& y2)
{
    x1 = vposx;
    y1 = vposy;
    x2 = vposx + vsizex - 1;
    y2 = vposy + vsizey - 1;
}

void zCView::GetScreenExtends(int& x1, int& y1, int& x2, int& y2)
{
    if (!screen)
        return;
    x1 = screen->anx(pposx);
    y1 = screen->any(pposy);
    x2 = x1 + screen->anx(psizex - 1);
    y2 = y1 + screen->any(psizey - 1);
}

void zCView::GetPixelExtends(float& x1, float& y1, float& x2, float& y2)
{
    x1 = (float)pposx;
    y1 = (float)pposy;
    x2 = (float)(pposx + psizex - 1);
    y2 = (float)(pposy + psizey - 1);
}

int zCView::IsIn(int x, int y)
{
    int x1 = pposx;
    int y1 = pposy;
    int x2 = pposx + psizex - 1;
    int y2 = pposy + psizey - 1;
    if (screen)
    {
        x1 = screen->anx(pposx);
        y1 = screen->any(pposy);
        x2 = x1 + screen->anx(psizex - 1);
        y2 = y1 + screen->any(psizey - 1);
    }
    return x >= x1 && y >= y1 && x <= x2 && y <= y2;
}

zCView* zCView::GetItem(int x, int y)
{
    for (zCList<zCView>* entry = childs.next; entry; entry = entry->next)
    {
        zCView* item = entry->data->GetItem(x, y);
        if (item)
            return item;
    }
    return IsIn(x, y) ? this : NULL;
}

float zCView::s_fViewTimePerChar = 50.0f;

zCViewText* zCView::CreateText(int x, int y, const zSTRING& text, float timer, zCOLOR& color,
    int timed, int colored)
{
    for (zCList<zCViewText>* entry = textLines.next; entry; entry = entry->next)
    {
        zCViewText* line = entry->data;
        if (line->posx == x && line->posy == y)
        {
            line->text = text;
            line->timer = timer;
            line->timed = timed;
            line->color = color;
            line->colored = colored;
            line->font = font;
            line->inPrintWin = 0;
            return line;
        }
    }
    if (text.Length() > maxTextLength)
    {
        maxTextLength = text.Length();
        textMaxLength = text;
    }
    zCViewText* line = new zCViewText;
    line->posx = x;
    line->posy = y;
    line->text = text;
    line->font = font;
    line->timer = timer;
    line->inPrintWin = 0;
    line->color = color;
    line->timed = timed;
    line->colored = timer == 0.0f && timed == 0 && colored == 0 ? 1 : colored;
    textLines.Insert(line);
    return line;
}

void zCView::CheckTimedText()
{
    zCList<zCViewText>* entry = textLines.next;
    while (entry)
    {
        zCList<zCViewText>* next = entry->next;
        zCViewText* line = entry->data;
        if (line && line->timed)
        {
            line->timer -= ztimer.frameTime / ztimer.factorMotion;
            if (line->timer <= 0.0f)
            {
                textLines.Remove(line);
                delete line;
            }
        }
        entry = next;
    }
}

void zCView::PrintTimed(int x, int y, const zSTRING& text, float time, zCOLOR* color)
{
    if (time == -1.0f)
        time = text.Length() * s_fViewTimePerChar;
    CreateText(x, y, text, time, color ? *color : fontColor, time != -2.0f, color != NULL);
}

void zCView::PrintTimedCX(int x, const zSTRING& text, float time, zCOLOR* color)
{
    int textWidth = font ? anx(font->GetFontX(text)) : 0;
    PrintTimed((0x2000 - textWidth) / 2, x, text, time, color);
}

void zCView::PrintTimedCY(int y, const zSTRING& text, float time, zCOLOR* color)
{
    int textHeight = font ? any(font->GetFontY() - 1) : 0;
    PrintTimed(y, (0x2000 - textHeight) / 2, text, time, color);
}

void zCView::PrintTimedCXY(const zSTRING& text, float time, zCOLOR* color)
{
    int textWidth = font ? anx(font->GetFontX(text)) : 0;
    int textHeight = font ? any(font->GetFontY() - 1) : 0;
    PrintTimed((0x2000 - textWidth) / 2, (0x2000 - textHeight) / 2, text, time, color);
}

static int ParMessageLineCount(zCView* view, const zSTRING& text)
{
    if (!view->font)
        return 0;
    int lines = 1;
    int width = 0;
    for (int i = 1;; ++i)
    {
        zSTRING word = text.PickWord(i, " #+*~`", " ");
        if (word.IsEmpty())
            break;
        if (word == "\n")
        {
            ++lines;
            width = 0;
            continue;
        }
        width += view->anx(view->font->GetFontX(word));
        if (view->winx + width >= view->px2)
        {
            ++lines;
            width = view->anx(view->font->GetFontX(word));
        }
    }
    return lines;
}

static void ParPrintMessage(zCView* view, const zSTRING& text, float time, zCOLOR& color)
{
    zSTRING line;
    for (int i = 1;; ++i)
    {
        zSTRING word = text.PickWord(i, " #+*~`", " ");
        if (word.IsEmpty())
            break;
        if (word == "\n")
        {
            view->Nextline(line, time, &color, NULL);
            line = "";
            continue;
        }
        int width = view->anx(view->font->GetFontX(word));
        if (view->winx + width >= view->px2)
        {
            view->Nextline(line, time, &color, NULL);
            line = word;
        }
        else
        {
            if (!line.IsEmpty())
                line = line + " ";
            line = line + word;
        }
    }
    view->Nextline(line, time, &color, NULL);
}

void zCView::PrintMessage(const zSTRING& text, const zSTRING& title, float time, zCOLOR& color)
{
    if (!font || any(font->GetFontY() - 1) <= 0)
        return;
    if (!title.IsEmpty())
        Nextline(title, time, &color, NULL);
    ParPrintMessage(this, text, time, color);
}

void zCView::PrintMessageCXY(const zSTRING& text, const zSTRING& title, float time, zCOLOR& color)
{
    if (!font || any(font->GetFontY() - 1) <= 0)
        return;
    int lines = ParMessageLineCount(this, text) + !title.IsEmpty();
    int height = any(font->GetFontY() - 1);
    winy = py1 + rnd2(((py2 - py1) - lines * height) * 0.5f);
    winx = px1;
    maxTextLength = 0;
    textMaxLength = "";
    if (!title.IsEmpty())
        Nextline(title, time, &color, NULL);
    ParPrintMessage(this, text, time, color);
}

void zCView::PrintSelection(const zSTRING& first, const zSTRING& second, zCOLOR& firstColor,
    zCOLOR& secondColor)
{
    InitPrintwin();
    winx = px1;
    winy = py1;
    maxTextLength = 0;
    textMaxLength = "";
    PrintTimedCX(first, -2.0f, &firstColor);
    PrintTimedCX(second, -2.0f, &secondColor);
}

void zCView::Dialog(int x, int y, const zSTRING& text, float time, zCOLOR* color)
{
    ForceOpen();
    timeDialog = time == -1.0f ? text.Length() * s_fViewTimePerChar : time;
    CreateText(x, y, text, time, color ? *color : fontColor, time != -2.0f, color != NULL);
}

void zCView::DialogCX(int x, const zSTRING& text, float time, zCOLOR* color)
{
    ForceOpen();
    timeDialog = time == -1.0f ? text.Length() * s_fViewTimePerChar : time;
    PrintTimedCX(x, text, time, color);
}

void zCView::DialogCY(int y, const zSTRING& text, float time, zCOLOR* color)
{
    ForceOpen();
    timeDialog = time == -1.0f ? text.Length() * s_fViewTimePerChar : time;
    PrintTimedCY(y, text, time, color);
}

void zCView::DialogCXY(const zSTRING& text, float time, zCOLOR* color)
{
    ForceOpen();
    timeDialog = time == -1.0f ? text.Length() * s_fViewTimePerChar : time;
    PrintTimedCXY(text, time, color);
}

void zCView::DialogMessage(const zSTRING& text, const zSTRING& title, float time, zCOLOR& color)
{
    ForceOpen();
    timeDialog = time == -1.0f ? text.Length() * s_fViewTimePerChar : time;
    if (resizeOnOpen)
    {
        if (text.Length() > textMaxLength.Length())
        {
            textMaxLength = text;
            maxTextLength = text.Length() > 40 ? 40 : text.Length();
        }
        int width = screen->font ? anx(screen->font->GetFontX(textMaxLength)) : 0;
        if (width > 0x1800)
            width = 0x1800;
        int padding = font ? anx(font->GetFontX("    ")) : 0;
        SetSize(width + padding, vsizey);
        SetPos((0x2000 - width) / 2, vposy);
    }
    PrintMessage(text, title, time, color);
}

void zCView::DialogMessageCXY(const zSTRING& text, const zSTRING& title, float time, zCOLOR& color)
{
    ForceOpen();
    timeDialog = time == -1.0f ? text.Length() * s_fViewTimePerChar : time;
    if (resizeOnOpen)
    {
        if (text.Length() > textMaxLength.Length())
        {
            textMaxLength = text;
            maxTextLength = text.Length() > 40 ? 40 : text.Length();
        }
        int width = screen->font ? anx(screen->font->GetFontX(textMaxLength)) : 0;
        int padding = font ? anx(font->GetFontX("    ")) : 0;
        width += padding;
        if (width > 0x1800)
            width = 0x1800;
        SetSize(width, vsizey);
        SetPos((0x2000 - width) / 2, vposy);
    }
    PrintMessageCXY(text, title, time, color);
}

void zCView::DialogSelection(const zSTRING& first, const zSTRING& second, zCOLOR& firstColor,
    zCOLOR& secondColor, float time)
{
    if (!resizeOnOpen)
    {
        if (!isOpen && timeOpen == 0.0f && timeClose == 0.0f)
        {
            timeOpen = 0;
            timeClose = 0;
            isOpen = 0;
            isClosed = 0;
            continueOpen = 1;
            continueClose = 0;
            DefPrintwin(50, 50, 8142, 8142);
            screen->InsertItem(this, 0);
        }
        timeDialog = time == -1.0f ?
            (first.Length() > second.Length() ? first.Length() : second.Length()) *
                s_fViewTimePerChar : time;
        ClrPrintwin();
        PrintTimedCX(first, -2.0f, &firstColor);
        PrintTimedCX(second, -2.0f, &secondColor);
        return;
    }
    timeDialog = time == -1.0f ?
        (first.Length() > second.Length() ? first.Length() : second.Length()) *
            s_fViewTimePerChar : time;
    if (first.Length() > textMaxLength.Length())
    {
        textMaxLength = first;
        maxTextLength = first.Length();
    }
    if (second.Length() > textMaxLength.Length())
    {
        textMaxLength = second;
        maxTextLength = second.Length();
    }
    int width = screen->font ? anx(screen->font->GetFontX(textMaxLength + "    ")) : 0;
    if (width > 0x1C00)
        width = 0x1C00;
    SetSize(width, vsizey);
    SetPos((0x2000 - width) / 2, vposy);
    if (!isOpen && timeOpen == 0.0f && timeClose == 0.0f)
    {
        timeOpen = 0;
        timeClose = 0;
        isOpen = 0;
        isClosed = 0;
        continueOpen = 1;
        continueClose = 0;
        DefPrintwin(50, 50, 8142, 8142);
        screen->InsertItem(this, 0);
    }
    ClrPrintwin();
    PrintTimedCX(first, -2.0f, &firstColor);
    PrintTimedCX(second, -2.0f, &secondColor);
}

void zCView::SetFont(zCFont* font)
{
    this->font = font ? font : zfontman->GetFont(0);
}

void zCView::SetFontColor(const zCOLOR& color)
{
    fontColor = color;
}

void zCView::SetFont(const zSTRING& name)
{
    if (zfontman)
        font = zfontman->GetFont(zfontman->Load(name));
}

zCFont* zCView::GetFont()
{
    return font;
}

zSTRING zCView::GetFontName()
{
    return font ? font->GetFontName() : zSTRING();
}

int zCView::FontY() const
{
    return font ? any(font->GetFontY() - 1) : 0;
}

int zCView::FontSize(const zSTRING& text)
{
    return font ? anx(font->GetFontX(text)) : 0;
}

void zCView::Print(int x, int y, const zSTRING& text)
{
    if (!font)
        return;
    scrollTimer = 0;
    if (viewID != 1 && this != screen)
    {
        CreateText(x, y, text);
        return;
    }
    zTRnd_AlphaBlendFunc oldAlpha = zrenderer->GetAlphaBlendFunc();
    if (alphafunc != 2 && alphafunc != 3)
        zrenderer->SetAlphaBlendFunc(zRND_ALPHA_FUNC_BLEND);
    PrintChars(nax(x), nay(y), text);
    zrenderer->SetAlphaBlendFunc(oldAlpha);
}

void zCView::PrintChars(int x, int y, const zSTRING& text)
{
    if (!font)
        return;
    int oldFilter = zrenderer->GetBilerpFilterEnabled();
    zrenderer->SetBilerpFilterEnabled(1);
    int oldWrite = zrenderer->GetZBufferWriteEnabled();
    zTRnd_ZBufferCmp oldCompare = zrenderer->GetZBufferCompare();
    zTRnd_AlphaBlendFunc oldAlpha = zrenderer->GetAlphaBlendFunc();
    zrenderer->SetZBufferWriteEnabled(0);
    zrenderer->SetZBufferCompare(zRND_ZBUFFER_CMP_ALWAYS);
    if (oldAlpha != zRND_ALPHA_FUNC_ADD && oldAlpha != zRND_ALPHA_FUNC_BLEND)
        zrenderer->SetAlphaBlendFunc(zRND_ALPHA_FUNC_BLEND);
    int position = x;
    for (int i = 0; i < text.Length(); ++i)
    {
        unsigned char character = (unsigned char)text[i];
        if (position >= pposx + psizex)
            break;
        if (character <= ' ')
        {
            if (character == ' ')
                position += font->GetWidth(' ');
            continue;
        }
        int width;
        zVEC2 uv1;
        zVEC2 uv2;
        font->GetFontData(character, width, uv1, uv2);
        zVEC2 topLeft((float)position, (float)y);
        zVEC2 bottomRight((float)(position + width - 1), (float)(y + font->GetFontY() - 1));
        float depth = zCCamera::activeCam ? zCCamera::activeCam->nearClipZ + 1.0f : 1.0f;
        zrenderer->DrawTile(font->GetFontTexture(), topLeft, bottomRight, depth, uv1, uv2,
            *(unsigned long*)&fontColor);
        position += width + font->GetLetterDistance();
    }
    zrenderer->SetBilerpFilterEnabled(oldFilter);
    zrenderer->SetAlphaBlendFunc(oldAlpha);
    zrenderer->SetZBufferWriteEnabled(oldWrite);
    zrenderer->SetZBufferCompare(oldCompare);
}

void zCView::PrintCX(int x, const zSTRING& text)
{
    int width = font ? anx(font->GetFontX(text)) : 0;
    Print(x, (0x2000 - width) / 2, text);
}

void zCView::PrintCY(int y, const zSTRING& text)
{
    int width = font ? anx(font->GetFontX(text)) : 0;
    Print(y, (0x2000 - width) / 2, text);
}

void zCView::PrintCXY(const zSTRING& text)
{
    int width = font ? anx(font->GetFontX(text)) : 0;
    int height = font ? any(font->GetFontY() - 1) : 0;
    Print((0x2000 - width) / 2, (0x2000 - height) / 2, text);
}

void zCView::ClrPrintwin()
{
    if (!(intflags & 1))
    {
        delete textLines.data;
        textLines.data = NULL;
        textLines.DeleteListDatas();
        textLines.DeleteList();
        intflags |= 1;
    }
    delete textLines.data;
    textLines.data = NULL;
    textLines.DeleteListDatas();
    textLines.DeleteList();
    winx = px1;
    winy = py1;
    maxTextLength = 0;
    textMaxLength = "";
}

zSTRING zCView::GetFirstText()
{
    if (!textLines.next)
        return zSTRING();
    zCList<zCViewText>* entry = textLines.next;
    while (entry->next)
        entry = entry->next;
    return entry->data->text;
}

void zCView::Nextline(const zSTRING& text, float time, zCOLOR* color, int* count)
{
    if (!(flags & 0x200))
        winx = px1;
    else
        winx = 0x1000 - (font ? anx(font->GetFontX(text)) / 2 : 0);
    int height = font ? any(font->GetFontY() - 1) : 0;
    if (winy + height > py2)
    {
        zCList<zCViewText>* entry = textLines.next;
        while (entry)
        {
            zCList<zCViewText>* next = entry->next;
            zCViewText* line = entry->data;
            if (!line->inPrintWin)
            {
                line->posy -= height;
                if (line->posy < py1)
                {
                    textLines.Remove(line);
                    delete line;
                }
            }
            entry = next;
        }
        winy -= height;
    }
    zCViewText* line = CreateText(winx, winy, text, time, color ? *color : fontColor,
        time != 0.0f, color != NULL);
    if (line)
        line->inPrintWin = 1;
    winy += height;
    scrollTimer = 0;
    if (count)
        ++*count;
}

void zCView::DefPrintwin(int x1, int y1, int x2, int y2)
{
    px1 = x1;
    winx = x1;
    px2 = x2;
    py1 = y1;
    py2 = y2;
    winy = y1;
    ClrPrintwin();
}

void zCView::Printwin(const zSTRING& text)
{
    if (!(intflags & 1))
    {
        textLines.DeleteListDatas();
        intflags |= 1;
    }
    ParPrintMessage(this, text, 0.0f, fontColor);
}

int zCView::showDebug = 0;

zSTRING zCView::Input(int maxChars)
{
    zSTRING text;
    return Input(maxChars, text);
}

zSTRING zCView::Input(int maxChars, zSTRING& text)
{
    zSTRING input = text;
    int count = input.Length();
    int key = 0;
    do
    {
        sysEvent();
        sysIdle(10);
        ClrPrintwin();
        zSTRING output = input;
        output += "|";
        PrintCXY(output);
        DrawItems();
        key = zinput->GetKey(1, 1);
        if (!key)
            continue;
        if (key == 8)
        {
            if (count > 0)
            {
                input.Delete(input.Length() - 1, 1);
                --count;
            }
        }
        else if (key != 13 && count < maxChars && key >= 32)
        {
            int shift = zinput->KeyPressed(42) || zinput->KeyPressed(54);
            if (key >= 30 && key <= 44 && !zinput->KeyPressed(42))
                key += 32;
            if (key == 11)
                key = shift ? '=' : 0;
            else if (key == 26)
                key = shift ? '_' : '-';
            else if (key == 52)
                key = '.';
            else if (key == 9)
                key = shift ? 0 : '(';
            else if (key == 10)
                key = shift ? ')' : 0;
            if (key)
            {
                input.Append(zSTRING((char)key));
                ++count;
            }
        }
    } while (key != 13);
    ClrPrintwin();
    PrintCXY(input);
    text = input;
    return input;
}

zSTRING zCView::Input(int maxChars, int x, int y, zSTRING& text)
{
    zSTRING input = text;
    int count = input.Length();
    int key = 0;
    do
    {
        sysEvent();
        sysIdle(10);
        ClrPrintwin();
        zSTRING output = input;
        output += "|";
        Print(x, y, output);
        screen->DrawItems();
        key = zinput->GetChar(1, 1);
        if (key == 8)
        {
            if (count > 0)
            {
                input.Delete(input.Length() - 1, 1);
                --count;
            }
        }
        else if (key != 13 && key >= 32 && key <= 255 && count < maxChars)
        {
            input.Append(zSTRING((char)key));
            ++count;
        }
    } while (key != 13);
    while (zinput->GetKey(1, 1))
        ;
    text = input;
    return input;
}

int zCView::InputNumber(int maxChars, int defaultValue)
{
    zSTRING input(defaultValue);
    int count = input.Length();
    int key = 0;
    do
    {
        sysEvent();
        ClrPrintwin();
        zSTRING output = input;
        output += "|";
        PrintCXY(output);
        DrawItems();
        key = zinput->GetKey(1, 1);
        if (key == 8)
        {
            if (count > 0)
            {
                input.Delete(input.Length() - 1, 1);
                --count;
            }
        }
        else if (key != 13 && count < maxChars && key >= '0' && key <= '9')
        {
            input.Append(zSTRING((char)key));
            ++count;
        }
    } while (key != 13);
    ClrPrintwin();
    PrintCXY(input);
    return input.ToInt();
}

void zCView::PrintDebug(const zSTRING& text)
{
    if (showDebug && stdoutput)
        stdoutput->Printwin(text);
}

void zCView::SetShowDebug(int enabled)
{
    showDebug = enabled;
}

int zCView::GetShowDebug()
{
    return showDebug;
}

void zCView::InsertItem(zCView* item, int priority)
{
    if (!item || item->ondesk)
        return;
    if (item->flags & 0x40)
        priority = 1;
    int virtualWidth = item->vsizex;
    int virtualHeight = item->vsizey;
    item->pposx = 0;
    item->pposy = 0;
    item->ondesk = 1;
    item->owner = this;
    item->CalcPixelPos(item->pposx, item->pposy);
    item->RecalcChildsPos();
    item->vsizex = virtualWidth > 0 ? virtualWidth : 1;
    item->vsizey = virtualHeight > 0 ? virtualHeight : 1;
    item->psizex = 0;
    item->psizey = 0;
    item->GetPixelSize(item->psizex, item->psizey);
    if (item->psizex <= 0)
        item->psizex = 1;
    if (item->psizey <= 0)
        item->psizey = 1;
    if (priority)
        item->flags |= 0x800;
    if (item->flags & 0x400)
    {
        if (!font)
            font = zfontman->GetFont(0);
        zSTRING firstText = item->GetFirstText();
        int textWidth = font ? anx(font->GetFontX(firstText)) : 0;
        int textHeight = font ? any(font->GetFontY() - 1) : 0;
        item->Init(item->vposx, item->vposy, item->vposx + textWidth,
            item->vposy + textHeight, item->viewID);
        item->flags |= flags;
        if (!item->font)
            item->font = zfontman->GetFont(0);
        if (!font)
            font = zfontman->GetFont(0);
    }
    if (item->flags & 0x40)
        modal = item;
    if (priority)
    {
        zCList<zCView>* entry = &childs;
        while (entry->next && (entry->next->data->flags & 0x800))
            entry = entry->next;
        zCList<zCView>* inserted = new zCList<zCView>;
        inserted->data = item;
        inserted->next = entry->next;
        entry->next = inserted;
    }
    else
    {
        zCList<zCView>* entry = &childs;
        while (entry->next)
            entry = entry->next;
        zCList<zCView>* inserted = new zCList<zCView>;
        inserted->data = item;
        inserted->next = NULL;
        entry->next = inserted;
    }
}

void zCView::RemoveItem(zCView* item)
{
    for (zCList<zCView>* entry = childs.next; entry; entry = entry->next)
    {
        if (entry->data == item)
        {
            childs.Remove(item);
            item->owner = NULL;
            item->ondesk = 0;
            inputList.Remove(item);
            if (item->flags & 0x40)
                modal = NULL;
            return;
        }
    }
    if (item->owner == this)
        item->owner = NULL;
}

void zCView::ChangeMode()
{
}

void zCView::SetNextMode()
{
}

void zCView::SetMode(int width, int height, int bpp, HWND__** window)
{
    zrenderer->Vid_SetMode(width, height, bpp, window);
    if (screen)
    {
        screen->psizex = width;
        screen->psizey = height;
    }
    else
    {
        screen = new zCView(0, 0, 0x1fff, 0x1fff, 0);
        if (zfontman)
            screen->font = zfontman->GetFont(zfontman->Load(zSTRING()));
        screen->ondesk = 1;
        screen->psizex = width;
        screen->psizey = height;
    }
    for (zCList<zCView>* entry = screen->childs.next; entry; entry = entry->next)
    {
        zCView* item = entry->data;
        if (item->vsizex <= 0)
            item->vsizex = 1;
        if (item->vsizey <= 0)
            item->vsizey = 1;
        item->psizex = 0;
        item->psizey = 0;
        if (item->owner)
        {
            item->psizex = item->owner->nax(item->vsizex);
            item->psizey = item->owner->nay(item->vsizey);
        }
        if (item->psizex <= 0)
            item->psizex = 1;
        if (item->psizey <= 0)
            item->psizey = 1;
        item->RecalcChildsSize();
    }
    for (zCList<zCView>* entry = screen->childs.next; entry; entry = entry->next)
    {
        zCView* item = entry->data;
        item->pposx = 0;
        item->pposy = 0;
        if (item->owner)
        {
            item->pposx += item->owner->nax(item->vposx);
            item->pposy += item->owner->nay(item->vposy);
            item->owner->CalcPixelPos(item->pposx, item->pposy);
        }
        item->RecalcChildsPos();
    }
}

void zCView::Line(int x1, int y1, int x2, int y2, const zCOLOR& lineColor)
{
    x1 = screen->nax(x1);
    y1 = screen->nay(y1);
    x2 = screen->nax(x2);
    y2 = screen->nay(y2);
    if (ClipLine(x1, y1, x2, y2))
        zrenderer->DrawLine(x1, y1, x2, y2, lineColor);
}

int zCView::GetCode(int x, int y)
{
    int code = 0;
    if (y > psizey - 1)
        code = 8;
    else if (y < 0)
        code = 4;
    if (x > psizex - 1)
        code |= 2;
    else if (x < 0)
        code |= 1;
    return code;
}

int zCView::ClipLine(int& x1, int& y1, int& x2, int& y2)
{
    int code1 = GetCode(x1, y1);
    int code2 = GetCode(x2, y2);
    while (true)
    {
        if (!code1 && !code2)
            return 1;
        if (code1 & code2)
            return 0;
        int code = code1 ? code1 : code2;
        int x;
        int y;
        if (code & 8)
        {
            x = x1 + (x2 - x1) * (psizey - 1 - y1) / (y2 - y1);
            y = psizey - 1;
        }
        else if (code & 4)
        {
            x = x1 + (x2 - x1) * -y1 / (y2 - y1);
            y = 0;
        }
        else if (code & 2)
        {
            y = y1 + (y2 - y1) * (psizex - 1 - x1) / (x2 - x1);
            x = psizex - 1;
        }
        else
        {
            y = y1 + (y2 - y1) * -x1 / (x2 - x1);
            x = 0;
        }
        if (code == code1)
        {
            x1 = x;
            y1 = y;
            code1 = GetCode(x1, y1);
        }
        else
        {
            x2 = x;
            y2 = y;
            code2 = GetCode(x2, y2);
        }
    }
}

void zCView::Render()
{
    DrawItems();
}

zCViewText::zCViewText(int x, int y, const zSTRING& value)
    : posx(x), posy(y), text(value), font(NULL), timer(0.0f), inPrintWin(0),
      color(GFX_WHITE), timed(0), colored(0)
{
}

zCViewText::zCViewText(int x, int y, const zSTRING& value, float& time, zCOLOR& textColor,
    int isTimed, int isColored)
    : posx(x), posy(y), text(value), font(NULL), timer(time), inPrintWin(0),
      color(textColor), timed(isTimed), colored(isColored)
{
}

void zCViewText::Create(int x, int y, const zSTRING& value, float& time, zCOLOR& textColor,
    int isTimed, int isColored)
{
    posx = x;
    posy = y;
    text = value;
    timer = time;
    timed = isTimed;
    color = textColor;
    colored = isColored;
    font = NULL;
    inPrintWin = 0;
}

zCViewText::~zCViewText()
{
    font = NULL;
}

zSTRING zCView::PatchMenuItemPrintwin()
{
    return GetFirstText();
}

zCAICamera* zCAICamera::Create()
{
    return new zCAICamera;
}

int zCAICamera::Release()
{
    delete this;
    return 0;
}

void zCSession::Done()
{
    if (world)
        world->Release();
    if (aiCam)
        aiCam->Release();
    if (camVob)
        camVob->Release();
    if (csMan)
        csMan->Release();
    delete camera;
    delete viewport;
    world = 0;
    aiCam = 0;
    camVob = 0;
    csMan = 0;
    camera = 0;
    viewport = 0;
}

void zCSession::Init()
{
    DesktopInit();
    CutsceneSystemInit();
}

void zCSession::DesktopInit()
{
    viewport = new zCView(0, 0, 0x1fff, 0x1fff, 0);
    screen->InsertItem(viewport, 0);
    viewport->SetFont(0);
}

void zCSession::CutsceneSystemInit()
{
    if (!csMan)
        csMan = new zCCSManager;
    csMan->LibForceToLoad();
}

void zCSession::Render()
{
    if (world)
        world->Render(camera);
    zrenderer->Vid_BeginLfbAccess();
    screen->DrawItems();
    zrenderer->Vid_EndLfbAccess();
    zrenderer->Vid_Unlock();
}

void zCSession::RenderBlit()
{
    zrenderer->Vid_Blit(1, 0, 0);
}

void zCSession::CamInit()
{
    delete camera;
    camera = new zCCamera;
    camera->SetRenderTarget(viewport);
    if (aiCam)
        aiCam->Release();
    if (camVob)
        camVob->Release();
    aiCam = zCAICamera::Create();
    camVob = new zCVob;
}

void zCSession::CamInit(zCVob* vob, zCCamera* cameraValue)
{
    camVob = vob;
    SetCamera(cameraValue);
}

zCWorld* zCSession::GetWorld()
{
    return world;
}

void zCSession::SaveWorld(const zSTRING&, int, int, int)
{
}

int zCSession::LoadWorld(const zSTRING&, int)
{
    return 0;
}

void zCSession::SetTime(int, int, int)
{
}

void zCSession::GetTime(int&, int&, int&)
{
}

void zCSession::SetWorld(zCWorld* worldValue)
{
    world = worldValue;
    world->SetOwnerSession(this);
}
