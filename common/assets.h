#pragma once

#include "gfx.h"
#include "species.h"

/* Defined in assets_data.c, which tools/make_assets.py generates. */

extern const image_t asset_fish[SPECIES_COUNT]; /* 16x16, keyed, facing right */
extern const image_t asset_back;                /* 320x192, opaque: far ruins and sand */
extern const image_t asset_front;               /* 320x192, keyed: rocks and kelp */
extern const uint16_t asset_water;              /* the plain colour above the art */

/* A coarse map of the whole screen: 1 where a cell contains any foreground art.
 * The tank uses it to tell open water from rock and kelp. */
#define ASSET_MASK_CELL 8
#define ASSET_MASK_COLS 40
#define ASSET_MASK_ROWS 30
extern const uint8_t asset_front_mask[ASSET_MASK_ROWS][ASSET_MASK_COLS];
