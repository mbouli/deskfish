#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "display.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "touch.h"

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

/* A grey 12x12 square centred on (cx, cy), for the finger to aim at. */
static void target(int cx, int cy)
{
    fill_rect(cx - 6, cy - 6, 12, 12, RGB565(96, 96, 96));
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

    /* Self-check: touch_read must refuse to work before touch has started,
     * and must refuse null pointers afterwards. */
    int tx = 0, ty = 0;
    bool check = !touch_read(&tx, &ty);

    bool have_touch = touch_init();
    if (!have_touch) {
        ESP_LOGW(TAG, "no touch: carrying on with the screen only");
    }
    check = check && !touch_read(NULL, &ty) && !touch_read(&tx, NULL);
    ESP_LOGI(TAG, "%s", check ? "selfcheck ok" : "selfcheck FAILED");

    fill(RGB565(0, 0, 32));
    target(15, 15);
    target(DISPLAY_W - 15, 15);
    target(15, DISPLAY_H - 15);
    target(DISPLAY_W - 15, DISPLAY_H - 15);
    target(DISPLAY_W / 2, DISPLAY_H / 2);

    bool was_down = false;
    while (1) {
        bool down = have_touch && touch_read(&tx, &ty);
        if (down) {
            fill_rect(tx - 4, ty - 4, 9, 9, RGB565(255, 255, 255));
            if (!was_down) {
                ESP_LOGI(TAG, "touch at %d,%d", tx, ty);
            }
        }
        was_down = down;
        display_push(fb, 0, 0, DISPLAY_W, DISPLAY_H);
    }
}
