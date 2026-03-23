#pragma once

class SignalConditioning {
public:
    static int clampInt(int value, int minValue, int maxValue);
    static int median3(int a, int b, int c);
    static int rampToward(int current, int target, int maxDelta);
    static int weightedAverageInt(int prev, int input, int alphaNum, int alphaDen);
};
