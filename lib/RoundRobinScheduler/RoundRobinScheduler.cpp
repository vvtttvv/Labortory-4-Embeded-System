#include "RoundRobinScheduler.h"

#include <Arduino_FreeRTOS.h>

#include <stdio.h>

namespace {
constexpr TickType_t kTick1Ms = pdMS_TO_TICKS(1);
constexpr TickType_t kInputPeriod = pdMS_TO_TICKS(10);
constexpr TickType_t kBinaryPeriod = pdMS_TO_TICKS(50);
constexpr TickType_t kAnalogPeriod = pdMS_TO_TICKS(50);
constexpr TickType_t kReportPeriod = pdMS_TO_TICKS(500);
constexpr uint16_t kSchedulerStackWords = 384;
constexpr UBaseType_t kSchedulerPriority = 2;
}

RoundRobinScheduler::RoundRobinScheduler(AppController& app)
    : app_(app)
{
}

void RoundRobinScheduler::start()
{
    const BaseType_t result = xTaskCreate(
        schedulerTaskThunk,
        "RR_SCHED",
        kSchedulerStackWords,
        this,
        kSchedulerPriority,
        nullptr);

    if (result != pdPASS) {
        printf("ERR scheduler create failed: RR_SCHED\n");
    } else {
        printf("OK scheduler task: RR_SCHED\n");
    }
}

void RoundRobinScheduler::schedulerTaskThunk(void* context)
{
    RoundRobinScheduler* self = static_cast<RoundRobinScheduler*>(context);
    if (self != nullptr) {
        self->schedulerTask();
    }

    vTaskDelete(nullptr);
}

void RoundRobinScheduler::schedulerTask()
{
    const TickType_t startTick = xTaskGetTickCount();
    TaskSlot tasks[] = {
        {&AppController::taskCommandInput, kInputPeriod, startTick},
        {&AppController::taskBinaryControl, kBinaryPeriod, startTick},
        {&AppController::taskAnalogControl, kAnalogPeriod, startTick},
        {&AppController::taskReporting, kReportPeriod, startTick},
    };

    while (true) {
        const TickType_t now = xTaskGetTickCount();

        for (TaskSlot& task : tasks) {
            if ((now - task.lastRun) >= task.periodTicks) {
                task.lastRun = now;
                (app_.*(task.method))();
            }
        }

        vTaskDelay(kTick1Ms);
    }
}
