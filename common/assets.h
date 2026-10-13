#pragma once

#include "gfx.h"
#include "species.h"

/* Defined in assets_data.c, which tools/make_assets.py generates. */

extern const image_t asset_fish[SPECIES_COUNT]; /* 16x16, keyed, facing right */
extern const image_t asset_back;                /* 320x192, opaque: far ruins and sand */
extern const image_t asset_front;               /* 320x192, keyed: rocks and kelp */
extern const uint16_t asset_water;              /* the plain colour above the art */
