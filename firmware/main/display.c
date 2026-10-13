#include "display.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define LCD_HOST     SPI2_HOST
#define LCD_CLOCK_HZ (40 * 1000 * 1000)
/* The SPI driver sends a frame in chunks of at most this many bytes (40 rows). */
#define CHUNK_BYTES  (DISPLAY_W * 40 * 2)

static const char *TAG = "display";

/* Start-up register values for this board's panel, from the vendor's
 * ILI9341V_Init.txt (https://www.lcdwiki.com/2.8inch_ESP32-S3_Display).
 * The driver's built-in defaults target a different ILI9341 panel: with them
 * this one showed dark streaks in line with bright areas and held ghost
 * images, because the drive voltages (C0, C1, C5, C7) were wrong for it.
 * Sleep-out, pixel format, orientation, inversion and display-on are sent by
 * the driver itself, so they are left out here. */
static const ili9341_lcd_init_cmd_t panel_init[] = {
    {0xCF, (uint8_t[]){0x00, 0xC1, 0x30}, 3, 0},             /* power control B */
    {0xED, (uint8_t[]){0x64, 0x03, 0x12, 0x81}, 4, 0},       /* power-on sequence */
    {0xE8, (uint8_t[]){0x85, 0x00, 0x78}, 3, 0},             /* driver timing A */
    {0xCB, (uint8_t[]){0x39, 0x2C, 0x00, 0x34, 0x02}, 5, 0}, /* power control A */
    {0xF7, (uint8_t[]){0x20}, 1, 0},                         /* pump ratio */
    {0xEA, (uint8_t[]){0x00, 0x00}, 2, 0},                   /* driver timing B */
    {0xC0, (uint8_t[]){0x13}, 1, 0},                         /* power control 1: drive voltage */
    {0xC1, (uint8_t[]){0x13}, 1, 0},                         /* power control 2 */
    {0xC5, (uint8_t[]){0x22, 0x35}, 2, 0},                   /* VCOM control 1 */
    {0xC7, (uint8_t[]){0xBD}, 1, 0},                         /* VCOM control 2: offset */
    {0xB6, (uint8_t[]){0x0A, 0xA2}, 2, 0},                   /* display function control */
    {0xF6, (uint8_t[]){0x01, 0x30}, 2, 0},                   /* interface control */
    {0xB1, (uint8_t[]){0x00, 0x1B}, 2, 0},                   /* frame rate */
    {0xF2, (uint8_t[]){0x00}, 1, 0},                         /* 3-gamma off */
    {0x26, (uint8_t[]){0x01}, 1, 0},                         /* gamma curve 1 */
    {0xE0, (uint8_t[]){0x0F, 0x35, 0x31, 0x0B, 0x0E, 0x06, 0x49, 0xA7,
                       0x33, 0x07, 0x0F, 0x03, 0x0C, 0x0A, 0x00}, 15, 0}, /* positive gamma */
    {0xE1, (uint8_t[]){0x00, 0x0A, 0x0F, 0x04, 0x11, 0x08, 0x36, 0x58,
                       0x4D, 0x07, 0x10, 0x0C, 0x32, 0x34, 0x0F}, 15, 0}, /* negative gamma */
};

static esp_lcd_panel_handle_t panel;
static SemaphoreHandle_t sent;
/* Byte-swapped copy of whatever is being sent. Owned here so callers may
 * redraw their own buffer as soon as display_push returns. */
static uint16_t *txbuf;

/* Runs in interrupt context when the last chunk of a transfer has gone out. */
static bool on_sent(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *edata, void *ctx)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(sent, &woken);
    return woken == pdTRUE;
}

/* Send the first w*h pixels of txbuf to the rectangle at (x, y) and wait. */
static void send(int x, int y, int w, int h)
{
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, x, y, x + w, y + h, txbuf));
    xSemaphoreTake(sent, portMAX_DELAY);
}

void display_init(void)
{
    sent = xSemaphoreCreateBinary();
    txbuf = heap_caps_malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (sent == NULL || txbuf == NULL) {
        ESP_LOGE(TAG, "cannot allocate the transfer buffer (is PSRAM enabled?)");
        abort();
    }

    spi_bus_config_t bus = {
        .sclk_io_num = PIN_LCD_SCK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = PIN_LCD_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = CHUNK_BYTES,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = PIN_LCD_CS,
        .dc_gpio_num = PIN_LCD_DC,
        .spi_mode = 0,
        .pclk_hz = LCD_CLOCK_HZ,
        .trans_queue_depth = 10,
        .on_color_trans_done = on_sent,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    esp_lcd_panel_io_handle_t io;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_cfg, &io));

    ili9341_vendor_config_t vendor_cfg = {
        .init_cmds = panel_init,
        .init_cmds_size = sizeof(panel_init) / sizeof(panel_init[0]),
    };
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = -1, /* no reset pin: the driver sends a software reset */
        .rgb_ele_order = LCD_BGR ? LCD_RGB_ELEMENT_ORDER_BGR : LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_cfg,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io, &panel_cfg, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, LCD_INVERT));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, LCD_SWAP_XY));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, LCD_MIRROR_X, LCD_MIRROR_Y));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

    /* Clear to black before the backlight comes on, so no garbage is shown. */
    memset(txbuf, 0, DISPLAY_W * DISPLAY_H * sizeof(uint16_t));
    send(0, 0, DISPLAY_W, DISPLAY_H);

    gpio_config_t backlight = {
        .pin_bit_mask = 1ULL << PIN_LCD_BACKLIGHT,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&backlight));
    ESP_ERROR_CHECK(gpio_set_level(PIN_LCD_BACKLIGHT, 1));
}

void display_push(const uint16_t *pixels, int x, int y, int w, int h)
{
    if (pixels == NULL || w <= 0 || h <= 0 || x < 0 || y < 0 ||
        w > DISPLAY_W - x || h > DISPLAY_H - y) {
        ESP_LOGE(TAG, "display_push: rejected rectangle %d,%d %dx%d", x, y, w, h);
        return;
    }

    int count = w * h;
    for (int i = 0; i < count; i++) {
        txbuf[i] = __builtin_bswap16(pixels[i]);
    }
    send(x, y, w, h);
}
