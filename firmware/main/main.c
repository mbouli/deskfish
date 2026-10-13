#include <inttypes.h>
#include <stdint.h>

#include "esp_flash.h"
#include "esp_log.h"
#include "esp_psram.h"

static const char *TAG = "deskfish";

void app_main(void)
{
    uint32_t flash_bytes = 0;
    esp_flash_get_size(NULL, &flash_bytes);

    ESP_LOGI(TAG, "hello from deskfish");
    ESP_LOGI(TAG, "flash: %" PRIu32 " MB", flash_bytes / (1024 * 1024));
    ESP_LOGI(TAG, "psram: %u MB", (unsigned)(esp_psram_get_size() / (1024 * 1024)));
}
