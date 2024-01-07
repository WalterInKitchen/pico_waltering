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

TaskHandle_t xRunnerTask;
u8g2_t u8g2;
i2c_inst_t *i2c_port = i2c0;

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

static void drawTest(void)
{
    u8g2_ClearBuffer(&u8g2);

    u8g2_SetFontMode(&u8g2, 1); // Transparent
    u8g2_SetFontDirection(&u8g2, 0);
    u8g2_SetFont(&u8g2, u8g2_font_inb24_mf);
    u8g2_DrawStr(&u8g2, 0, 30, "U");

    u8g2_SetFontDirection(&u8g2, 1);
    u8g2_SetFont(&u8g2, u8g2_font_inb30_mn);
    u8g2_DrawStr(&u8g2, 21, 8, "8");

    u8g2_SetFontDirection(&u8g2, 0);
    u8g2_SetFont(&u8g2, u8g2_font_inb24_mf);
    u8g2_DrawStr(&u8g2, 51, 30, "g");
    u8g2_DrawStr(&u8g2, 67, 30, "\xb2");

    u8g2_DrawHLine(&u8g2, 2, 35, 47);
    u8g2_DrawHLine(&u8g2, 3, 36, 47);
    u8g2_DrawVLine(&u8g2, 45, 32, 12);
    u8g2_DrawVLine(&u8g2, 46, 33, 12);

    u8g2_SetFont(&u8g2, u8g2_font_4x6_tr);

    u8g2_DrawStr(&u8g2, 1, 54, "github.com/olikraus/u8g2");

    u8g2_SendBuffer(&u8g2);
}

static void handlerTask(void *pvParameters)
{
    i2cInit();
    displayInit();
    u8g2_FirstPage(&u8g2);
    u8g2_SendBuffer(&u8g2);
    while (1)
    {
        vTaskDelay(10000);
        printf("\r\n u8g2 ready draw");
        drawTest();
    }
}

void display_oled_init(void)
{
    xTaskCreate(handlerTask, "display_u8g2", configMINIMAL_STACK_SIZE, NULL, TASK_PRIORITY, &xRunnerTask);
}
