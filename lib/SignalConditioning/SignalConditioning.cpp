#include "SignalConditioning.h"

int SignalConditioning::clampInt(int value, int minValue, int maxValue)
{
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

int SignalConditioning::median3(int a, int b, int c)
{
    if ((a >= b && a <= c) || (a <= b && a >= c)) {
        return a;
    }
    if ((b >= a && b <= c) || (b <= a && b >= c)) {
        return b;
    }
    return c;
}

int SignalConditioning::rampToward(int current, int target, int maxDelta)
{
    if (maxDelta <= 0) {
        return current;
    }

    if (target > current) {
        const int delta = target - current;
        return current + (delta > maxDelta ? maxDelta : delta);
    }

    if (target < current) {
        const int delta = current - target;
        return current - (delta > maxDelta ? maxDelta : delta);
    }

    return current;
}

int SignalConditioning::weightedAverageInt(int prev, int input, int alphaNum, int alphaDen)
{
    if (alphaDen <= 0) {
        return input;
    }

    alphaNum = clampInt(alphaNum, 0, alphaDen);
    const long mixed = ((long)alphaNum * input) + ((long)(alphaDen - alphaNum) * prev);
    return (int)(mixed / alphaDen);
}
