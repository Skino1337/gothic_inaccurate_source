#ifndef ODOC_H
#define ODOC_H

#include "zViewBase.h"
#include "zModel.h"

class zCView : public zCViewBase {
public:

    void SetAlphaBlendFunc(int function);
    void InsertBack(const zSTRING& texture);
    void SetPos(int x, int y);
    void SetSize(int x, int y);
    void DefPrintwin(int x, int y, int width, int height);
    void SetEnableHandleEvent(int enabled);
    void SetHandleEventTop(int enabled);
    void InsertItem(zCView* item, int priority);
    void Init(int x, int y, int width, int height, int priority);

    int ondesk;
    zCTexture* backTex;
};

extern zCView* screen;

class oCDoc : public zCView {
public:

    oCDoc(const zSTRING& texture);
    virtual ~oCDoc();
    void Show();
    virtual int HandleEvent(int key);
    void MapInitCoordinates(float gameStartX, float gameStartY, float pixelStartX,
        float pixelStartY, float gameEndX, float gameEndY, float pixelEndX, float pixelEndY);
    void MapDrawCoordinates(float gameX, float gameY);
    void SetLevelName(const zSTRING& name);

    zSTRING levelName;
    float scalex;
    float scaley;
    float map_gameCoPerPixelx;
    float map_gameStartx;
    float map_gameCoPerPixely;
    float map_gameStarty;
    float pixelStartx;
    float pixelStarty;

    static zCView* posView;
};

extern oCDoc* document;

#endif
