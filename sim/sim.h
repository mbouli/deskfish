#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Handle window events. Returns false once the window has been closed. */
bool sim_pump(void);

void sim_sleep_ms(uint32_t ms);
