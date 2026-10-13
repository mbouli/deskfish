#pragma once

#include <stdbool.h>

/* Join Wi-Fi in the background and keep the system clock set from the
 * internet. Returns at once. Never stops the caller: every failure is logged
 * and the clock carries on from whatever it had. */
void wifi_time_start(void);

/* True once the internet has answered at least once. */
bool wifi_time_synced(void);
