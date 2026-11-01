#include "include/oViewDocumentMap.h"
#include <cmath>

zSTRING oCWorld::GetWorldFilename()
{
    return worldFilename;
}

zSTRING oCWorld::GetWorldName()
{
    return worldName;
}

oCViewDocumentMap::oCViewDocumentMap()
    : ViewPageMap(0), Level("INVALID.ZEN")
{
    LevelCoords.n[0] = 0.0f;
    LevelCoords.n[1] = 0.0f;
    LevelCoords.n[2] = 0.0f;
    LevelCoords.n[3] = 0.0f;
}

oCViewDocumentMap::~oCViewDocumentMap()
{
}

oCViewDocument* oCViewDocumentMap::SetPage(int page, zSTRING& texture, int fullSize)
{
    oCViewDocument* result = oCViewDocument::SetPage(page, texture, fullSize);
    ViewPageMap = result;
    return result;
}

void oCViewDocumentMap::SetLevel(const zSTRING& level)
{
    Level = level;
    Level.Upper();
}

void oCViewDocumentMap::SetLevelCoords(int minX, int minZ, int maxX, int maxZ)
{
    LevelCoords.n[0] = minX;
    LevelCoords.n[1] = minZ;
    LevelCoords.n[2] = maxX;
    LevelCoords.n[3] = maxZ;
}

void oCViewDocumentMap::UpdatePosition()
{
    oCWorld* world = ogame->GetGameWorld();
    if (!ViewPageMap || !world)
        return;
    if (world->GetWorldFilename() != Level) {
        ViewArrow.Close();
        return;
    }
    ViewArrow.Open();
    oCNpc* player = ogame->GetSelfPlayerVob();
    if (!player || !player->homeWorld)
        return;
    zVEC3 position = {
        player->trafoObjToWorld.m[0][3],
        player->trafoObjToWorld.m[1][3],
        player->trafoObjToWorld.m[2][3]
    };
    float minX = LevelCoords.n[0];
    float minZ = LevelCoords.n[1];
    float maxX = LevelCoords.n[2];
    float maxZ = LevelCoords.n[3];
    if (minX == 0.0f && minZ == 0.0f && maxX == 0.0f && maxZ == 0.0f) {
        minX = player->homeWorld->bspTree.bbox3D.mins.n[0];
        maxX = player->homeWorld->bspTree.bbox3D.maxs.n[0];
        minZ = player->homeWorld->bspTree.bbox3D.mins.n[2];
        maxZ = player->homeWorld->bspTree.bbox3D.maxs.n[2];
    }
    float x = ViewPageMap->PixelPosition.X;
    float y = ViewPageMap->PixelPosition.Y + ViewPageMap->PixelSize.Y;
    if (maxX != minX)
        x += (position.n[0] - minX) / (maxX - minX) * ViewPageMap->PixelSize.X;
    if (maxZ != minZ)
        y -= (position.n[2] - minZ) / (maxZ - minZ) * ViewPageMap->PixelSize.Y;
    zCPosition arrowPosition = {
        static_cast<int>(x) - ViewArrow.PixelSize.X / 2,
        static_cast<int>(y) - ViewArrow.PixelSize.Y / 2
    };
    float azimuth;
    float elevation;
    player->GetAngles(position, azimuth, elevation);
    int heading = static_cast<int>(std::floor((azimuth + 22.5f) / 45.0f)) & 7;
    static const char* textures[] = {
        "O.TGA", "NO.TGA", "N.TGA", "NW.TGA",
        "W.TGA", "SW.TGA", "S.TGA", "SO.TGA"
    };
    zSTRING texture(textures[heading]);
    ViewArrow.SetTexture(texture, 0);
    ViewArrow.SetPixelPosition(arrowPosition);
}
