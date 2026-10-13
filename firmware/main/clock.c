#include "clock.h"

#include <stdbool.h>
#include <sys/time.h>
#include <time.h>

#include "esp_timer.h"

/* The moment the firmware was built, from CMake. Until the internet answers,
 * the board believes it is this, counting up. */
#ifndef BUILD_EPOCH
#define BUILD_EPOCH 0
#endif

uint32_t clock_millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

int64_t clock_unix(void)
{
    static bool started;
    if (!started) {
        /* The system clock starts at 1970. Give it the build time once, unless
         * something (network time) has already set it to later than that. */
        started = true;
        if (time(NULL) < (time_t)BUILD_EPOCH) {
            struct timeval tv = {.tv_sec = (time_t)BUILD_EPOCH};
            settimeofday(&tv, NULL);
        }
    }
    return (int64_t)time(NULL);
}
