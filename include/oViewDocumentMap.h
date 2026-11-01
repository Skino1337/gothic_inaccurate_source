#ifndef OVIEWDOCUMENTMAP_H
#define OVIEWDOCUMENTMAP_H

#include "oViewDocument.h"
#include "z3d.h"

class oCWorld {
public:

    zSTRING GetWorldFilename();
    zCVob* CreateVob(zTVobType type, const zSTRING& name);

    zSTRING worldFilename;
};

class oCViewDocumentMap : public oCViewDocument {
public:

    enum zEHeading {
        HEADING_NORTH,
        HEADING_NORTHEAST,
        HEADING_EAST,
        HEADING_SOUTHEAST,
        HEADING_SOUTH,
        HEADING_SOUTHWEST,
        HEADING_WEST,
        HEADING_NORTHWEST
    };
    typedef zEHeading zTHeading;

    oCViewDocumentMap();
    virtual ~oCViewDocumentMap();
    virtual oCViewDocument* SetPage(int page, zSTRING& texture, int fullSize);
    void SetLevel(const zSTRING& level);
    void SetLevelCoords(int minX, int minZ, int maxX, int maxZ);
    void UpdatePosition();

    oCViewDocument ViewArrow;
    oCViewDocument* ViewPageMap;
    zSTRING Level;
    zVEC4 LevelCoords;
};

#endif
