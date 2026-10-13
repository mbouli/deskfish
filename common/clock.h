#pragma once

#include <stdint.h>

/* Minutes since local midnight, 0..1439. */
int clock_minutes(void);

/* Milliseconds since start-up. Wraps after about 49 days. */
uint32_t clock_millis(void);
