#include <stdio.h>
#include "input_handler.h"
#include "hal_input.h"
#include "log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Priorities at which the tasks are created. */
#define TASK_PRIORITY (tskIDLE_PRIORITY + 1)

typedef struct
{
    hal_input_key key;
    hal_input_key_state state;
} KeyEvt;

static QueueHandle_t keySrcQueue;

static void inputKeyCallback(hal_input_key key, hal_input_key_state state)
{
    KeyEvt evt;
    evt.key = key;
    evt.state = state;
    xQueueSendToBack(keySrcQueue, &evt, 0);
}

static void inputEventHandlerTask(void *pvParameters)
{
    KeyEvt currentKeyEvt;

    // 准备队列
    keySrcQueue = xQueueCreate(32, sizeof(KeyEvt));
    // 注册按键回调
    hal_input_init(inputKeyCallback);

    /* Remove compiler warning about unused parameter. */
    (void)pvParameters;

    for (;;)
    {
        if (pdPASS != xQueueReceive(keySrcQueue, &currentKeyEvt, portMAX_DELAY))
        {
            continue;
        }
#if INPUT_HANDLER_LOG_ENABLE
        printf("input_event:%d %d\r\n", currentKeyEvt.key, currentKeyEvt.state);
#endif
    }
}

void input_handler_init(void)
{
    xTaskCreate(inputEventHandlerTask, "inputHandler", configMINIMAL_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
}