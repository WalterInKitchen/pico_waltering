#include <stdio.h>
#include "pump_runner.h"
#include "hal_pump.h"
#include "log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

/* Priorities at which the tasks are created. */
#define TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define SECONDS_TO_MS(SEC) (SEC * 1000)
#define SECONDS_TO_TICK(SEC) (pdMS_TO_TICKS(SECONDS_TO_MS(SEC)))
#define MINUTE_TO_TICK(MIN) (SECONDS_TO_TICK(MIN * 60))

// 任务事件类型
#define TASK_EVT_TYPE_EXECUTE_PUMP (0x01 << 0)
#define TASK_EVT_TYPE_PUMP_RELEASE (0x01 << 1)
#define TASK_EVT_TYPE_PUMP_ENABLE (0x01 << 2)
#define TASK_EVT_TYPE_PUMP_DISABLE (0x01 << 3)

static struct
{
    TimerHandle_t periodTimer;
    TimerHandle_t holdTimer;
    StaticTimer_t holdTimerBuffers;
    StaticTimer_t periodTimerBuffers;
    TaskHandle_t xRunnerTask;
    ePumpState state;
} pumpState;

// 运行配置
static tPumpRunnerCfg runnerCfg = {5, 60 * 12};

static void periodTimerCallback()
{
    // 启动水泵
    xTaskNotify(pumpState.xRunnerTask, TASK_EVT_TYPE_EXECUTE_PUMP, eSetBits);
}

static void holdTimerCallback()
{
    // 释放水泵
    xTaskNotify(pumpState.xRunnerTask, TASK_EVT_TYPE_PUMP_RELEASE, eSetBits);
}

static void handlerTask(void *pvParameters)
{
    uint32_t taskEvent = 0;
    while (1)
    {
        taskEvent = ulTaskNotifyTake(pdTRUE, SECONDS_TO_TICK(10));
        if (taskEvent == 0)
        {
            continue;
        }
        // 启动水泵
        if ((taskEvent & TASK_EVT_TYPE_EXECUTE_PUMP) > 0)
        {
            printf("\r\nstart pump");
            hal_pump_control(eTrue);
            xTimerReset(pumpState.holdTimer, INFINITY_TIME);
            pumpState.state = PUMP_RUNNING;
        }
        // 停止泵水
        if ((taskEvent & TASK_EVT_TYPE_PUMP_RELEASE) > 0)
        {
            printf("\r\npump release");
            hal_pump_control(eFalse);
            xTimerReset(pumpState.periodTimer, INFINITY_TIME);
            pumpState.state = PUMP_WAITING;
        }
        // 启动水泵定时器，即打开水泵开关
        if ((taskEvent & TASK_EVT_TYPE_PUMP_ENABLE) > 0)
        {
            BaseType_t res = xTimerStart(pumpState.periodTimer, INFINITY_TIME);
            pumpState.state = PUMP_WAITING;
            printf("\r\npump runner enabled...%d", res);
        }
        // 停止水泵定时器，即关闭水泵开关
        if ((taskEvent & TASK_EVT_TYPE_PUMP_DISABLE) > 0)
        {
            BaseType_t res = xTimerStop(pumpState.periodTimer, INFINITY_TIME);
            res = xTimerStop(pumpState.holdTimer, INFINITY_TIME);
            pumpState.state = PUMP_INIT;
            printf("\r\npump runner disabled...%d", res);
            hal_pump_control(eFalse);
        }
    }
}

static uint32 timerNextRunGetSeconds(TimerHandle_t timer)
{
    if (timer == NULL)
    {
        return INFINITY_TIME;
    }
    if (pdFALSE == xTimerIsTimerActive(timer))
    {
        return INFINITY_TIME;
    }
    TickType_t remain = xTimerGetExpiryTime(timer) - xTaskGetTickCount();
    return (uint32)((int)remain < 0 ? 0 : remain) / 1000;
}

void pumpStartExecute(void)
{
    xTaskNotify(pumpState.xRunnerTask, TASK_EVT_TYPE_PUMP_ENABLE, eSetBits);
}

void pumpStopExecute(void)
{
    xTaskNotify(pumpState.xRunnerTask, TASK_EVT_TYPE_PUMP_DISABLE, eSetBits);
}

uint32 pumpNextRunTimeSecondsGet(void)
{
    return timerNextRunGetSeconds(pumpState.periodTimer);
}

uint32 pumpRunningRemainTimeSecondsGet(void)
{
    return timerNextRunGetSeconds(pumpState.holdTimer);
}

void pump_runner_init(void)
{
    pumpState.state = PUMP_INIT;
    pumpState.periodTimer = xTimerCreateStatic("periodTimer", MINUTE_TO_TICK(runnerCfg.pumpRunDurationMinutes), pdFALSE, (void *)0, periodTimerCallback, &pumpState.periodTimerBuffers);
    pumpState.holdTimer = xTimerCreateStatic("holdTimer", SECONDS_TO_TICK(runnerCfg.pumpKeepSeconds), pdFALSE, (void *)0, holdTimerCallback, &pumpState.holdTimerBuffers);
    xTaskCreate(handlerTask, "pumpRunner", configMINIMAL_STACK_SIZE, NULL, TASK_PRIORITY, &pumpState.xRunnerTask);
}

void pumpRunnerSetCfg(tPumpRunnerCfg *cfg)
{
    runnerCfg.pumpKeepSeconds = cfg->pumpKeepSeconds;
    runnerCfg.pumpRunDurationMinutes = cfg->pumpRunDurationMinutes;
    xTimerChangePeriod(pumpState.periodTimer, MINUTE_TO_TICK(runnerCfg.pumpRunDurationMinutes), 0);
    xTimerChangePeriod(pumpState.holdTimer, SECONDS_TO_TICK(runnerCfg.pumpKeepSeconds), 0);
}

tPumpRunnerCfg pumpRunnerGetCfg(void)
{
    return runnerCfg;
}