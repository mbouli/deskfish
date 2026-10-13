#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>

#include "app.h"
#include "clock.h"
#include "display.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_random.h"
#include "touch.h"
#include "wifi_time.h"

static const char *TAG = "deskfish";

void app_main(void)
{
    uint32_t flash_bytes = 0;
    esp_flash_get_size(NULL, &flash_bytes);
    ESP_LOGI(TAG, "hello from deskfish");
    ESP_LOGI(TAG, "flash: %" PRIu32 " MB", flash_bytes / (1024 * 1024));
    ESP_LOGI(TAG, "psram: %u MB", (unsigned)(esp_psram_get_size() / (1024 * 1024)));

    clock_unix(); /* starts the clock from the build time */
    wifi_time_start();

    display_init();
    if (!touch_init()) {
        ESP_LOGW(TAG, "no touch: carrying on with the screen only");
    }

    uint16_t *fb = heap_caps_malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (fb == NULL) {
        ESP_LOGE(TAG, "cannot allocate the pixel buffer (is PSRAM enabled?)");
        abort();
    }

    if (!app_init(esp_random())) {
        ESP_LOGE(TAG, "cannot allocate the lit pictures (is PSRAM enabled?)");
        abort();
    }

    int frames = 0;
    uint32_t second_start = clock_millis();
    while (1) {
        static unsigned seen_relights;
        static uint32_t relight_ms;
        uint32_t before = clock_millis();
        app_frame(fb);
        uint32_t frame_ms = clock_millis() - before;
        if (app_relights() != seen_relights) {
            seen_relights = app_relights();
            relight_ms = frame_ms; /* this frame included a re-light */
        }

        frames++;
        uint32_t now = clock_millis();
        if (now - second_start >= 5000) {
            ESP_LOGI(TAG, "%d fps; last re-light frame took %" PRIu32 " ms; clock from %s", frames / 5,
                     relight_ms, wifi_time_synced() ? "the internet" : "the build time");
            frames = 0;
            second_start = now;
        }
    }
}
