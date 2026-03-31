#pragma once

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

#include "AppController.h"

class RoundRobinScheduler {
public:
    explicit RoundRobinScheduler(AppController& app);

    void start();

private:
    using TaskMethod = void (AppController::*)();

    struct TaskSlot {
        TaskMethod method;
        TickType_t periodTicks;
        TickType_t lastRun;
    };

    static void schedulerTaskThunk(void* context);
    void schedulerTask();

    AppController& app_;
};
