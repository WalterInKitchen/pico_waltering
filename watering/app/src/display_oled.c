#include <stdio.h>
#include "display_oled.h"
#include "pump_runner.h"
#include "log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#include "hardware/i2c.h"
#include "u8g2.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"

/* Priorities at which the tasks are created. */
#define TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define SSD1306_I2C_CLK 400 * 1000
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

#define SEND_LOG_PRINT 0

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define TEXT_DISPLAY_START 5

// 任务事件类型
#define OLED_REFRESH (0x01 << 0)
#define OLED_EVT_SLEEP (0x01 << 1)

static char *drawRemainTextGet(void);
static char *drawPumpPeriodTextGet(void);
static char *drawPumpHoldTextGet(void);

typedef char *(*pFuncDrawTextGet)();
typedef struct
{
    pFuncDrawTextGet drawTextGet;
    bool canSetByUser;
    uint32 settingIdx;
} DisplayBlock;

TaskHandle_t xRunnerTask;
u8g2_t u8g2;
i2c_inst_t *i2c_port = i2c0;

DisplayBlock displayBlocks[] = {{drawRemainTextGet, 1, 1},
                                {drawPumpHoldTextGet, 1, 2},
                                {drawPumpPeriodTextGet, 1, 3}};

static uint32 highlightSettingdIdx;

static void i2cInit(void)
{
    i2c_init(i2c_port, SSD1306_I2C_CLK);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
}

static void i2cSend(uint8_t addr, const uint8_t *src, size_t len)
{
#if SEND_LOG_PRINT
    uint8_t i = 0;
    printf("\r\n>>send %d:", len);
    for (i = 0; i < len; i++)
    {
        printf("%02X ", *(src + i));
    }
#endif
    i2c_write_blocking(i2c_port, addr, src, len, false);
}

static void oledI2cInit(void)
{
}

uint8_t psoc_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg)
    {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
        oledI2cInit(); // 初始化
        break;

    case U8X8_MSG_DELAY_MILLI:
        vTaskDelay(arg_int); // 延时
        break;

    case U8X8_MSG_GPIO_I2C_CLOCK:
        break;

    case U8X8_MSG_GPIO_I2C_DATA:
        break;

    default:
        return 0;
    }
    return 1; // command processed successfully.
}

uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    static uint8_t buffer[32]; /* u8g2/u8x8 will never send more than 32 bytes between START_TRANSFER and END_TRANSFER */
    static uint8_t buf_idx;
    uint8_t *data;

    switch (msg)
    {
    case U8X8_MSG_BYTE_SEND:
        data = (uint8_t *)arg_ptr;
        while (arg_int > 0)
        {
            buffer[buf_idx++] = *data;
            data++;
            arg_int--;
        }
        break;
    case U8X8_MSG_BYTE_INIT:
        /* add your custom code to init i2c subsystem */
        break;
    case U8X8_MSG_BYTE_SET_DC:
        /* ignored for i2c */
        break;
    case U8X8_MSG_BYTE_START_TRANSFER:
        buf_idx = 0;
        break;
    case U8X8_MSG_BYTE_END_TRANSFER:
        i2cSend(u8x8_GetI2CAddress(u8x8) >> 1, buffer, buf_idx);
        break;
    default:
        return 0;
    }
    return 1;
}

static void displayInit(void)
{
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_hw_i2c, psoc_gpio_and_delay_cb);
    u8g2_SetI2CAddress(&u8g2, 0x3c * 2);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearDisplay(&u8g2);
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_SetFontDirection(&u8g2, 0);
}

static char *drawRemainTextGet(void)
{
    static char textBuf[128];
    uint32 remain = pumpNextRunTimeSecondsGet();
    char *unit = "Min";
    float remainFloat;

    if (remain == INFINITY_TIME)
    {
        sprintf(textBuf, "NEXT: --");
    }
    else
    {
        remainFloat = remain / 60.0;
        if (remainFloat > 60)
        {
            remainFloat = remainFloat / 60.0;
            unit = "Hr";
        }
        sprintf(textBuf, "NEXT: %.2f %s", remainFloat, unit);
    }
    return textBuf;
}

static char *drawPumpPeriodTextGet(void)
{
    static char textBuf[128];
    tPumpRunnerCfg cfg = pumpRunnerGetCfg();

    sprintf(textBuf, "PERIOD: %d M", cfg.pumpRunDurationMinutes);
    return textBuf;
}

static char *drawPumpHoldTextGet(void)
{
    static char textBuf[128];
    tPumpRunnerCfg cfg = pumpRunnerGetCfg();

    sprintf(textBuf, "HOLD: %d SEC", cfg.pumpKeepSeconds);
    return textBuf;
}

static void drawBlocks(void)
{
    uint32 i = 0;
    uint32 lineStart = 20;
    uint32 len = sizeof(displayBlocks) / sizeof(DisplayBlock);
    DisplayBlock *pCurBlock;
    char *text;
    uint32 displayFlag = 0;

    u8g2_SetFont(&u8g2, u8g2_font_helvB10_te);
    for (i = 0; i < len; i++)
    {
        pCurBlock = &displayBlocks[i];
        displayFlag = U8G2_BTN_BW0;
        if (highlightSettingdIdx == pCurBlock->settingIdx)
        {
            displayFlag |= U8G2_BTN_INV;
        }
        text = pCurBlock->drawTextGet();
        u8g2_DrawButtonUTF8(&u8g2, TEXT_DISPLAY_START, lineStart, displayFlag, DISPLAY_WIDTH - 2 * TEXT_DISPLAY_START, 1, 1, text);
        lineStart += 18;
    }
}

static void handlerTask(void *pvParameters)
{
    uint32_t taskEvent = 0;
    i2cInit();
    displayInit();

    while (1)
    {
        taskEvent = ulTaskNotifyTake(pdTRUE, SECONDS_TO_TICK(1));
        if (taskEvent | OLED_EVT_SLEEP)
        {
        }
        u8g2_FirstPage(&u8g2);
        do
        {
            u8g2_DrawRFrame(&u8g2, 0, 0, 128, 64, 8);
            drawBlocks();
            u8g2_SendBuffer(&u8g2);
        } while (u8g2_NextPage(&u8g2));
    }
    printf("\r\noled done...");
}

void display_highlight_settings(uint32 settingIdx)
{
    highlightSettingdIdx = settingIdx;
    display_refresh_content();
}

void display_refresh_content(void)
{
    xTaskNotify(xRunnerTask, OLED_REFRESH, eSetBits);
}

void display_oled_init(void)
{
    xTaskCreate(handlerTask, "display_u8g2", configMINIMAL_STACK_SIZE, NULL, TASK_PRIORITY, &xRunnerTask);
}
