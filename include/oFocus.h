#ifndef OFOCUS_H
#define OFOCUS_H

#include "oCollisionObjectMiscChar.h"

class oCNpcFocus {
public:

    oCNpcFocus();
    ~oCNpcFocus();
    void Init(zSTRING& name);
    void Init(int instance);
    int IsValid(int vobType);
    float GetMaxRange();
    float GetRange2();
    float GetItemThrowRange();
    float GetMobThrowRange();
    int HasLowerPriority(int firstType, int secondType);
    int GetPriority(int vobType);
    int IsInRange(int vobType, float distance);
    int IsInAngle(int vobType, float azimuth, float elevation);
    int IsNpcInRange(float distance);
    int IsNpcInAngle(float azimuth, float elevation);
    int IsItemInRange(float distance);
    int IsItemInAngle(float azimuth, float elevation);
    int IsMobInRange(float distance);
    int IsMobInAngle(float azimuth, float elevation);
    float GetAzi();

    static void InitFocusModes();
    static void ExitFocusModes();
    static void SetFocusMode(int mode);
    static zSTRING GetFocusName();
    static void StartHighlightningFX(oCNpc* npc);
    static void StopHighlightningFX();

    float n_range3;
    float n_range1;
    float n_range2;
    float n_azi;
    float n_elev1;
    float n_elev2;
    int n_prio;
    float i_range1;
    float i_range2;
    float i_azi;
    float i_elev1;
    float i_elev2;
    int i_prio;
    float m_range1;
    float m_range2;
    float m_azi;
    float m_elev1;
    float m_elev2;
    int m_prio;
    float max_range;

    static oCNpcFocus* focus;
    static oCNpcFocus* focuslist[6];
    static int s_iHightlightMeleeFocus;
    static int s_bHighlightInteractFocus;
    static oCVisualFX* highLightFX;
    static int isHighLighted;
};

#endif
