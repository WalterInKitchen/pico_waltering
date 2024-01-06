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
#define SSD1306_I2C_CLK 400
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

TaskHandle_t xRunnerTask;
u8g2_t u8g2;
i2c_inst_t *i2c_port = i2c0;

static void i2cInit(void)
{
    i2c_init(i2c_port, SSD1306_I2C_CLK * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
}

uint8_t u8x8_byte_arduino_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    static uint8_t buffer[32]; /* u8g2/u8x8 will never send more than 32 bytes between START_TRANSFER and END_TRANSFER */
    static uint8_t buf_idx;
    uint8_t *data;
    uint8_t cmdSize;

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
        cmdSize = arg_int;
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
        i2c_write_blocking(i2c_port, u8x8_GetI2CAddress(u8x8) >> 1, buffer, cmdSize, false);
        break;
    default:
        return 0;
    }
    return 1;
}

static void displayInit(void)
{
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_sw_i2c, u8x8_byte_arduino_hw_i2c);
    u8g2_InitDisplay(&u8g2);     // send init sequence to the display, display is in sleep mode after this,
    u8g2_SetPowerSave(&u8g2, 0); // wake up display
}

static void handlerTask(void *pvParameters)
{
    displayInit();

    while (1)
    {
        vTaskDelay(100);
        printf("\r\nhello world...");
    }
}

void display_oled_init(void)
{
    xTaskCreate(handlerTask, "display_u8g2", configMINIMAL_STACK_SIZE, NULL, TASK_PRIORITY, &xRunnerTask);
}
