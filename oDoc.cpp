#include "include/oDoc.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/oGame.h"

zCView* oCDoc::posView = 0;
oCDoc* document = 0;

oCDoc::oCDoc(const zSTRING& texture)
    : scalex(1.0f), scaley(1.0f), map_gameCoPerPixelx(0.0f), map_gameStartx(0.0f),
      map_gameCoPerPixely(0.0f), map_gameStarty(0.0f), pixelStartx(0.0f), pixelStarty(0.0f)
{
    SetAlphaBlendFunc(2);
    InsertBack(texture);
    int width = 0;
    int height = 0;
    if (backTex) {
        zCTextureInfo info = backTex->GetTextureInfo();
        width = info.sizeX;
        height = info.sizeY;
        if (width > zrenderer->vid_xdim) {
            scalex = (float)zrenderer->vid_xdim / (float)width;
            width = zrenderer->vid_xdim;
        }
        if (height > zrenderer->vid_ydim) {
            scaley = (float)zrenderer->vid_ydim / (float)height;
            height = zrenderer->vid_ydim;
        }
    }
    SetPos((8192 - screen->anx()) / 2, (8192 - screen->any()) / 2);
    SetSize(screen->anx(), screen->any());
    DefPrintwin(100, 100, 8092, 8092);
    SetEnableHandleEvent(1);
    SetHandleEventTop(1);
    if (oCNpc::player) {
        if (oCNpc::player->inventory2.IsOpen())
            oCNpc::player->CloseInventory();
        oCNpc::player->SetMovLock(1);
    }
    memset(&map_gameCoPerPixelx, 0, sizeof(float) * 6);
    screen->InsertItem(this, 0);
}

oCDoc::~oCDoc()
{
    if (document == this)
        document = 0;
    if (oCNpc::player)
        oCNpc::player->SetMovLock(0);
    delete posView;
    posView = 0;
}

void oCDoc::Show()
{
    if (screen)
        screen->InsertItem(this, 0);
}

int oCDoc::HandleEvent(int key)
{
    if (zinput->KeyToggled(key)) {
        document = 0;
        delete this;
    }
    return 1;
}

void oCDoc::MapInitCoordinates(float gameStartX, float gameStartY, float pixelStartX,
    float pixelStartY, float gameEndX, float gameEndY, float pixelEndX, float pixelEndY)
{
    map_gameStartx = gameStartX;
    pixelStartx = pixelStartX;
    map_gameStarty = gameStartY;
    pixelStarty = pixelStartY;
    map_gameCoPerPixelx = (gameEndX - gameStartX) / (pixelEndX - pixelStartX);
    map_gameCoPerPixely = (gameEndY - gameStartY) / (pixelEndY - pixelStartY);
}

void oCDoc::MapDrawCoordinates(float gameX, float gameY)
{
    if (levelName != ogame->GetGameWorld()->GetWorldName() || !ondesk)
        return;
    if (!posView) {
        posView = new zCView(0, 0, 8192, 8192, 0);
        posView->InsertBack("MAPPOS.TGA");
    }
    int x = (int)(((gameX - map_gameStartx) / map_gameCoPerPixelx + pixelStartx) * scalex);
    int y = (int)(((gameY - map_gameStarty) / map_gameCoPerPixely + pixelStarty) * scaley);
    int baseX = anx();
    int baseY = any();
    if (posView->backTex) {
        zCTextureInfo info = posView->backTex->GetTextureInfo();
        posView->Init(baseX + x, baseY + y, info.sizeX, info.sizeY, 2);
        InsertItem(posView, 0);
        posView->SetPos(baseX + x - info.sizeX / 2, baseY + y - info.sizeY / 2);
    }
}

void oCDoc::SetLevelName(const zSTRING& name)
{
    levelName = name;
    levelName.Upper();
    int extension = levelName.Search(".ZEN");
    if (extension >= 0)
        levelName.Delete(extension, 4);
}
