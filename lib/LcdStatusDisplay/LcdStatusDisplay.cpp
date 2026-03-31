#include "LcdStatusDisplay.h"

#include <LiquidCrystal.h>
#include <stdio.h>

namespace {
LiquidCrystal* g_lcd = nullptr;
}

void LcdStatusDisplay::init(uint8_t rsPin, uint8_t enPin, uint8_t d4Pin, uint8_t d5Pin, uint8_t d6Pin, uint8_t d7Pin)
{
    rsPin_ = rsPin;
    enPin_ = enPin;
    d4Pin_ = d4Pin;
    d5Pin_ = d5Pin;
    d6Pin_ = d6Pin;
    d7Pin_ = d7Pin;

    if (g_lcd == nullptr) {
        g_lcd = new LiquidCrystal(rsPin_, enPin_, d4Pin_, d5Pin_, d6Pin_, d7Pin_);
    }
    g_lcd->begin(16, 2);
    g_lcd->clear();
    initialized_ = true;
}

void LcdStatusDisplay::showBoot()
{
    if (!initialized_ || g_lcd == nullptr) {
        return;
    }

    g_lcd->setCursor(0, 0);
    g_lcd->print("Actuator Lab 5");
    clearLine(1);
    g_lcd->setCursor(0, 1);
    g_lcd->print("Init...");
}

void LcdStatusDisplay::showStatus(bool binaryOn, int motorRawPercent, int motorAppliedPercent, int motorSpeedPercent, bool limitAlert)
{
    if (!initialized_ || g_lcd == nullptr) {
        return;
    }

    char line1[17];
    char line2[17];

    snprintf(
        line1,
        sizeof(line1),
        "B:%s S:%3d",
        binaryOn ? "ON " : "OFF",
        motorSpeedPercent);

    snprintf(
        line2,
        sizeof(line2),
        "P:%3d A:%3d %c",
        motorRawPercent,
        motorAppliedPercent,
        limitAlert ? 'L' : ' ');

    clearLine(0);
    g_lcd->setCursor(0, 0);
    g_lcd->print(line1);

    clearLine(1);
    g_lcd->setCursor(0, 1);
    g_lcd->print(line2);
}

void LcdStatusDisplay::clearLine(uint8_t row)
{
    if (!initialized_ || g_lcd == nullptr) {
        return;
    }

    g_lcd->setCursor(0, row);
    g_lcd->print("                ");
}
