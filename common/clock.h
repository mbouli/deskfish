#pragma once

#include <stdint.h>

/* Seconds since 1970-01-01 00:00 UTC: the calendar date and time. */
int64_t clock_unix(void);

/* Milliseconds since start-up. Wraps after about 49 days. */
uint32_t clock_millis(void);
