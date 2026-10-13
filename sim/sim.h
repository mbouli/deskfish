#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Handle window events. Returns false once the window has been closed. */
bool sim_pump(void);

void sim_sleep_ms(uint32_t ms);

/* True while fast-forward (the F key) is switched on. */
bool sim_fast(void);

/* True once after each press of R. */
bool sim_take_restart(void);
