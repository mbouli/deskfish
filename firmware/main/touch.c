#include "touch.h"

#include <stdint.h>

#include "board.h"
#include "display.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_log.h"

static const char *TAG = "touch";

static esp_lcd_touch_handle_t tp;

bool touch_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = -1, /* any free port */
        .sda_io_num = PIN_TOUCH_SDA,
        .scl_io_num = PIN_TOUCH_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    if (i2c_new_master_bus(&bus_cfg, &bus) != ESP_OK) {
        ESP_LOGW(TAG, "cannot start the I2C bus");
        return false;
    }

    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    esp_lcd_panel_io_handle_t io;
    if (esp_lcd_new_panel_io_i2c(bus, &io_cfg, &io) != ESP_OK) {
        ESP_LOGW(TAG, "cannot open the touch chip's I2C address");
        return false;
    }

    esp_lcd_touch_config_t cfg = {
        .x_max = LCD_NATIVE_W,
        .y_max = LCD_NATIVE_H,
        .rst_gpio_num = PIN_TOUCH_RST,
        .int_gpio_num = GPIO_NUM_NC, /* we poll once per frame */
        .levels = {.reset = 0, .interrupt = 0},
        .flags = {
            .swap_xy = TOUCH_SWAP_XY,
            .mirror_x = TOUCH_MIRROR_X,
            .mirror_y = TOUCH_MIRROR_Y,
        },
    };
    esp_lcd_touch_handle_t handle;
    if (esp_lcd_touch_new_i2c_ft5x06(io, &cfg, &handle) != ESP_OK) {
        ESP_LOGW(TAG, "touch driver failed to start");
        return false;
    }
    if (i2c_master_probe(bus, ESP_LCD_TOUCH_IO_I2C_FT5x06_ADDRESS, 100) != ESP_OK) {
        ESP_LOGW(TAG, "touch chip does not answer");
        return false;
    }

    tp = handle;
    return true;
}

bool touch_read(int *x, int *y)
{
    if (tp == NULL || x == NULL || y == NULL) {
        return false;
    }
    if (esp_lcd_touch_read_data(tp) != ESP_OK) {
        return false;
    }

    esp_lcd_touch_point_data_t point;
    uint8_t count = 0;
    if (esp_lcd_touch_get_data(tp, &point, &count, 1) != ESP_OK || count == 0) {
        return false;
    }

    /* The chip can report the far edge itself (e.g. 320); keep it on screen. */
    *x = point.x >= DISPLAY_W ? DISPLAY_W - 1 : point.x;
    *y = point.y >= DISPLAY_H ? DISPLAY_H - 1 : point.y;
    return true;
}
