#include "RoundRobinScheduler.h"

#include <Arduino_FreeRTOS.h>

#include <stdio.h>

namespace {
static TickType_t msToTicksCeil(uint16_t ms)
{
    const TickType_t ticks = pdMS_TO_TICKS(ms);
    if (ticks == 0) {
        return 1;
    }
    return ticks;
}

constexpr TickType_t kMinDelayTick = 1;
const TickType_t kInputPeriod = msToTicksCeil(10);
const TickType_t kBinaryPeriod = msToTicksCeil(50);
const TickType_t kAnalogPeriod = msToTicksCeil(50);
const TickType_t kReportPeriod = msToTicksCeil(500);
constexpr uint16_t kControlStackWords = 320;
constexpr uint16_t kReportStackWords = 224;
constexpr UBaseType_t kControlPriority = 3;
constexpr UBaseType_t kReportPriority = 1;
}

RoundRobinScheduler::RoundRobinScheduler(AppController& app)
    : app_(app)
{
}

void RoundRobinScheduler::start()
{
    BaseType_t result = xTaskCreate(
        controlTaskThunk,
        "CTRL_SCHED",
        kControlStackWords,
        this,
        kControlPriority,
        nullptr);

    if (result != pdPASS) {
        printf("ERR scheduler create failed: CTRL_SCHED\n");
    } else {
        printf("OK scheduler task: CTRL_SCHED\n");
    }

    result = xTaskCreate(
        reportTaskThunk,
        "REPORT",
        kReportStackWords,
        this,
        kReportPriority,
        nullptr);

    if (result != pdPASS) {
        reportDedicatedTaskActive_ = false;
        printf("WARN REPORT task create failed, fallback to control scheduler\n");
    } else {
        reportDedicatedTaskActive_ = true;
        printf("OK scheduler task: REPORT\n");
    }
}

void RoundRobinScheduler::controlTaskThunk(void* context)
{
    RoundRobinScheduler* self = static_cast<RoundRobinScheduler*>(context);
    if (self != nullptr) {
        self->controlTask();
    }

    vTaskDelete(nullptr);
}

void RoundRobinScheduler::reportTaskThunk(void* context)
{
    RoundRobinScheduler* self = static_cast<RoundRobinScheduler*>(context);
    if (self != nullptr) {
        self->reportTask();
    }

    vTaskDelete(nullptr);
}

void RoundRobinScheduler::controlTask()
{
    const TickType_t startTick = xTaskGetTickCount();
    TaskSlot tasks[] = {
        {&AppController::taskCommandInput, kInputPeriod, startTick},
        {&AppController::taskBinaryControl, kBinaryPeriod, startTick},
        {&AppController::taskAnalogControl, kAnalogPeriod, startTick},
    };

    TickType_t fallbackReportLastRun = startTick;

    while (true) {
        const TickType_t now = xTaskGetTickCount();

        for (TaskSlot& task : tasks) {
            if ((now - task.lastRun) >= task.periodTicks) {
                task.lastRun = now;
                (app_.*(task.method))();
            }
        }

        if (!reportDedicatedTaskActive_ && ((now - fallbackReportLastRun) >= kReportPeriod)) {
            fallbackReportLastRun = now;
            app_.taskReporting();
        }

        // Always block at least one RTOS tick to let lower-priority tasks run.
        vTaskDelay(kMinDelayTick);
    }
}

void RoundRobinScheduler::reportTask()
{
    TickType_t lastWake = xTaskGetTickCount();

    while (true) {
        app_.taskReporting();
        vTaskDelayUntil(&lastWake, kReportPeriod);
    }
}
