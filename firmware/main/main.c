#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>

#include "display.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "deskfish";

/* The whole screen, one host-order RGB565 value per pixel. */
static uint16_t *fb;

static void fill(uint16_t colour)
{
    for (int i = 0; i < DISPLAY_W * DISPLAY_H; i++) {
        fb[i] = colour;
    }
}

/* Fill a rectangle, clipped to the screen. */
static void fill_rect(int x, int y, int w, int h, uint16_t colour)
{
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > DISPLAY_W ? DISPLAY_W : x + w;
    int y1 = y + h > DISPLAY_H ? DISPLAY_H : y + h;
    for (int row = y0; row < y1; row++) {
        for (int col = x0; col < x1; col++) {
            fb[row * DISPLAY_W + col] = colour;
        }
    }
}

void app_main(void)
{
    uint32_t flash_bytes = 0;
    esp_flash_get_size(NULL, &flash_bytes);
    ESP_LOGI(TAG, "hello from deskfish");
    ESP_LOGI(TAG, "flash: %" PRIu32 " MB", flash_bytes / (1024 * 1024));
    ESP_LOGI(TAG, "psram: %u MB", (unsigned)(esp_psram_get_size() / (1024 * 1024)));

    display_init();

    fb = heap_caps_malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (fb == NULL) {
        ESP_LOGE(TAG, "cannot allocate the pixel buffer (is PSRAM enabled?)");
        abort();
    }

    static const struct {
        const char *name;
        uint16_t colour;
    } fills[] = {
        {"red", RGB565(255, 0, 0)},
        {"green", RGB565(0, 255, 0)},
        {"blue", RGB565(0, 0, 255)},
    };

    while (1) {
        for (int i = 0; i < 3; i++) {
            fill(fills[i].colour);
            /* Orientation marker: big white square in the top-left corner,
             * small black square to its right. */
            fill_rect(0, 0, 40, 40, RGB565(255, 255, 255));
            fill_rect(50, 0, 20, 20, RGB565(0, 0, 0));
            display_push(fb, 0, 0, DISPLAY_W, DISPLAY_H);
            ESP_LOGI(TAG, "showing %s", fills[i].name);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}
