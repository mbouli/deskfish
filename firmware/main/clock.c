#include "clock.h"

#include "esp_timer.h"

/* Until Task 4 gives the board Wi-Fi, it believes it is the moment the
 * firmware was built, and counts up from there. */
#ifndef BUILD_EPOCH
#define BUILD_EPOCH 0
#endif

uint32_t clock_millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

int64_t clock_unix(void)
{
    return (int64_t)BUILD_EPOCH + esp_timer_get_time() / 1000000;
}
