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

static const char *TAG = "deskfish";

void app_main(void)
{
    uint32_t flash_bytes = 0;
    esp_flash_get_size(NULL, &flash_bytes);
    ESP_LOGI(TAG, "hello from deskfish");
    ESP_LOGI(TAG, "flash: %" PRIu32 " MB", flash_bytes / (1024 * 1024));
    ESP_LOGI(TAG, "psram: %u MB", (unsigned)(esp_psram_get_size() / (1024 * 1024)));

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
        app_frame(fb);

        frames++;
        uint32_t now = clock_millis();
        if (now - second_start >= 5000) {
            ESP_LOGI(TAG, "%d fps", frames / 5);
            frames = 0;
            second_start = now;
        }
    }
}
