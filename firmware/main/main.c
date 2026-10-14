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
#include "model.h"
#include "touch.h"
#include "wifi_time.h"

static const char *TAG = "deskfish";

/* The model file, built into the firmware: its first byte and one past its last. */
extern const uint8_t model_start[] asm("_binary_deskfish_model_bin_start");
extern const uint8_t model_end[] asm("_binary_deskfish_model_bin_end");

/* The model's numbers are read over and over, so they go in the board's fast
 * internal memory if there is room, not the large slow PSRAM. */
static void *fast_alloc(size_t size)
{
    void *p = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    return p != NULL ? p : malloc(size);
}

static void fast_release(void *p)
{
    free(p);
}

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

    size_t model_bytes = (size_t)(model_end - model_start);
    size_t fast_before = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    model_use_memory(fast_alloc, fast_release);
    if (app_set_model(model_start, model_bytes)) {
        size_t fast_after = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        ESP_LOGI(TAG, "model: %u bytes built in; fish are listening to it", (unsigned)model_bytes);
        ESP_LOGI(TAG, "model: using %u KB of fast memory, %u KB of it left", (unsigned)((fast_before - fast_after) / 1024),
                 (unsigned)(fast_after / 1024));
    } else {
        ESP_LOGE(TAG, "model: the built-in file is not usable; fish are following the rules");
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
            relight_ms = frame_ms;
        }

        frames++;
        uint32_t now = clock_millis();
        if (now - second_start >= 5000) {
            static unsigned seen_decisions, seen_recalled;
            static uint32_t seen_decision_ms;
            unsigned decisions = 0;
            uint32_t decision_ms = 0, longest_ms = 0;
            app_model_stats(&decisions, &decision_ms, &longest_ms);
            unsigned made = decisions - seen_decisions;
            uint32_t spent = decision_ms - seen_decision_ms;
            seen_decisions = decisions;
            unsigned recalled = app_model_recalled() - seen_recalled;
            seen_recalled += recalled;
            seen_decision_ms = decision_ms;
            ESP_LOGI(TAG, "%d fps; last re-light frame took %" PRIu32 " ms; clock from %s", frames / 5,
                     relight_ms, wifi_time_synced() ? "the internet" : "the build time");
            unsigned worked = made - recalled;
            ESP_LOGI(TAG, "model: %u decisions in 5 s, %u from memory; %" PRIu32 " ms in all (%" PRIu32
                          " ms for each worked out), longest ever %" PRIu32 " ms",
                     made, recalled, spent, worked ? spent / worked : 0, longest_ms);
            frames = 0;
            second_start = now;
        }
    }
}
