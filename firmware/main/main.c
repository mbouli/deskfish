#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "display.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_timer.h"

static const char *TAG = "deskfish";

/* The whole screen, one host-order RGB565 value per pixel. Allocated with one
 * extra guard pixel at each end so the self-check can detect stray writes. */
static uint16_t *fb;
#define GUARD 0xA5A5

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

/* Things the by-eye checks never try. Each bad display_push below is expected
 * to log one "rejected rectangle" error and draw nothing. */
static void selfcheck(void)
{
    bool ok = true;

    display_push(NULL, 0, 0, 10, 10);
    display_push(fb, -1, 0, 10, 10);
    display_push(fb, 0, 0, 0, 10);
    display_push(fb, 0, 0, 10, -5);
    display_push(fb, DISPLAY_W - 5, 0, 10, 10);
    display_push(fb, 0, DISPLAY_H - 5, 10, 10);

    /* Rectangles that hang off every edge must be clipped, not written out of bounds. */
    fill(0);
    fill_rect(-20, -20, 40, 40, 0xFFFF);
    fill_rect(DISPLAY_W - 20, DISPLAY_H - 20, 40, 40, 0xFFFF);
    fill_rect(-1000, 50, 5000, 10, 0xFFFF);
    fill_rect(50, -1000, 10, 5000, 0xFFFF);
    fill_rect(DISPLAY_W + 10, DISPLAY_H + 10, 40, 40, 0xFFFF);
    if (fb[-1] != GUARD || fb[DISPLAY_W * DISPLAY_H] != GUARD) {
        ok = false;
    }
    if (fb[0] != 0xFFFF || fb[DISPLAY_W * DISPLAY_H - 1] != 0xFFFF) {
        ok = false;
    }

    ESP_LOGI(TAG, "%s", ok ? "selfcheck ok" : "selfcheck FAILED");
}

void app_main(void)
{
    uint32_t flash_bytes = 0;
    esp_flash_get_size(NULL, &flash_bytes);
    ESP_LOGI(TAG, "hello from deskfish");
    ESP_LOGI(TAG, "flash: %" PRIu32 " MB", flash_bytes / (1024 * 1024));
    ESP_LOGI(TAG, "psram: %u MB", (unsigned)(esp_psram_get_size() / (1024 * 1024)));

    display_init();

    uint16_t *raw = heap_caps_malloc((DISPLAY_W * DISPLAY_H + 2) * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (raw == NULL) {
        ESP_LOGE(TAG, "cannot allocate the pixel buffer (is PSRAM enabled?)");
        abort();
    }
    raw[0] = GUARD;
    raw[DISPLAY_W * DISPLAY_H + 1] = GUARD;
    fb = raw + 1;

    selfcheck();

    const int size = 40;
    int x = 0, y = 0, dx = 3, dy = 2;
    int frames = 0;
    int64_t second_start = esp_timer_get_time();

    while (1) {
        fill(RGB565(0, 0, 64));
        fill_rect(x, y, size, size, RGB565(255, 160, 0));
        display_push(fb, 0, 0, DISPLAY_W, DISPLAY_H);

        x += dx;
        y += dy;
        if (x < 0 || x + size > DISPLAY_W) {
            dx = -dx;
            x += 2 * dx;
        }
        if (y < 0 || y + size > DISPLAY_H) {
            dy = -dy;
            y += 2 * dy;
        }

        frames++;
        int64_t now = esp_timer_get_time();
        if (now - second_start >= 1000000) {
            ESP_LOGI(TAG, "%d fps", frames);
            frames = 0;
            second_start = now;
        }
    }
}
