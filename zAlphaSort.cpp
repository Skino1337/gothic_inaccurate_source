#include "include/zAlphaSort.h"
#include "include/zRenderManager.h"
#include "include/zRenderer.h"
#include "include/zObject.h"
#include <cstring>

extern zCRenderManager zrenderMan;

zCRndAlphaSortObject_Vob zCRndAlphaSortObject_Vob::s_alphaVobList[256];
int zCRndAlphaSortObject_Vob::s_currentAlphaVob;

zCRndAlphaSortObject_Vob::zCRndAlphaSortObject_Vob()
    : nextSortObject(0), zvalue(0), vob(0), alpha(1.0f)
{
}

zCRndAlphaSortObject_Vob::~zCRndAlphaSortObject_Vob()
{
    if (vob) {
        vob->Release();
        vob = NULL;
    }
}

zCRndAlphaSortObject_Vob* zCRndAlphaSortObject_Vob::GetFreeAlphaVob()
{
    static int warned;

    if (s_currentAlphaVob >= 256) {
        s_currentAlphaVob = 0;
        if (!warned) {
            zerr->Fault("D: RNDMAN: too few alphaVOBs!");
            warned = 1;
        }
    }
    return &s_alphaVobList[s_currentAlphaVob++];
}

void zCRndAlphaSortObject_Vob::ResetFreeList()
{
    s_currentAlphaVob = 0;
}

void zCRndAlphaSortObject_Vob::Draw(int index)
{
    if (!vob)
        return;

    zTRenderContext context;
    std::memset(&context, 0, sizeof(context));
    context.clipFlags = 0x3F;
    context.vob = vob;
    context.cam = zCCamera::activeCam;
    context.world = vob->GetHomeWorld();

    const unsigned long oldReference = zrenderer->GetAlphaReference();
    zrenderer->SetZBias(vob->m_zBias);

    if (!vob->visual->GetAlphaTestingEnabled()) {
        zrenderMan.overrideAlphaValue = static_cast<unsigned long>(alpha * 255.0f);
        zrenderMan.overrideAlphaBlendFunc = zRND_ALPHA_FUNC_BLEND;
    }
    else {
        zrenderer->SetAlphaReference(static_cast<unsigned long>(oldReference * alpha));
        zrenderMan.overrideAlphaValue = static_cast<unsigned long>(alpha * 255.0f);
        zrenderMan.overrideAlphaBlendFunc = zRND_ALPHA_FUNC_BLEND_TEST;
    }

    vob->Render(context);
    zrenderMan.overrideAlphaValue = 255;
    zrenderMan.overrideAlphaBlendFunc = zRND_ALPHA_FUNC_NONE;
    zrenderer->SetAlphaReference(oldReference);

    if (vob) {
        vob->Release();
        vob = NULL;
    }
}

void zCRndAlphaSortObject_Vob::ClearList()
{
    for (int i = 255; i >= 0; --i) {
        if (s_alphaVobList[i].vob) {
            s_alphaVobList[i].vob->Release();
            s_alphaVobList[i].vob = NULL;
        }
    }
    s_currentAlphaVob = 0;
}
