#include "include/zModel.h"

void ApplyDamages(unsigned long damageTypes, int* damages, int& totalDamage)
{
    if (!damageTypes) {
        zerr->Message("U: Dam: Invalid damage-type.");
        return;
    }
    int numTypes = 0;
    for (int i = 0; i < 8; ++i) {
        if (damageTypes & (1 << i))
            ++numTypes;
    }
    int damage = static_cast<int>(totalDamage / static_cast<float>(numTypes) + 0.5f);
    for (int i = 0; i < 8; ++i) {
        if ((damageTypes & (1 << i)) && !damages[i])
            damages[i] = damage;
    }
}
