#ifndef ZMATH_H
#define ZMATH_H

float zMin(float a, float b);
float zRandF2();
float Alg_SqrtInvApprox(float value);
int   zGetHighestBit(int value);
void  zClamp(float& value, float minValue, float maxValue);
void  zClamp(int& value, int minValue, int maxValue);
void  zClamp(unsigned long& value, unsigned long minValue, unsigned long maxValue);
void  zClamp01(float& value);
float zSinApprox(float angle);
float zCosApprox(float angle);
void  zSinCosApprox(float angle, float& sine, float& cosine);
void  Trg_Init();

#endif
