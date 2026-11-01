#ifndef ZVIEWFX_H
#define ZVIEWFX_H

#include "zViewDraw.h"

class zCViewFX : public zCViewDraw {
public:

    enum zEViewFX {
        VIEW_FX_NONE = 0,
        VIEW_FX_ZOOM = 1,
        VIEW_FX_FADE = 2,
        VIEW_FX_BOUNCE = 4,
        VIEW_FX_FORCE_DWORD = 0xffffffff
    };
    typedef zEViewFX zTViewFX;

    zCViewFX();
    virtual ~zCViewFX();
    static zCObject* _CreateNewInstance();
    static void operator delete(void* memory);
    virtual zCClassDef* _GetClassDef() const;
    void Init(zCViewObject* parent, int opened, unsigned long modeOpen, unsigned long modeClose,
        float durationOpen, float durationClose, zSTRING& texture);
    void Open();
    void OpenSafe();
    void OpenImmediately();
    void Close();
    void CloseSafe();
    void CloseImmediately();
    void ApplyOpenCloseFX();
    void ApplyOpenFX();
    void ApplyCloseFX();
    void ApplyOpenFade();
    void ApplyOpenZoom();
    void ApplyCloseFade();
    void ApplyCloseZoom();
    virtual void Blit();
    virtual void EvaluateTexturePosition();

    int HasOpened;
    int HasClosed;
    float TimeOpen;
    float TimeClose;
    float DurationOpen;
    float DurationClose;
    unsigned long ModeOpen;
    unsigned long ModeClose;
    zVEC2 TextureOffset[2];
    float baseAlpha;

    static zCClassDef* classDef;
};

#endif
