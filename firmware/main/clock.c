#include "clock.h"

#include "esp_timer.h"

/* The board has no battery-backed clock yet: every boot starts at 12:00. */
#define BOOT_MINUTES (12 * 60)

uint32_t clock_millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

int clock_minutes(void)
{
    int64_t minutes = esp_timer_get_time() / 60000000;
    return (int)((BOOT_MINUTES + minutes) % (24 * 60));
}
