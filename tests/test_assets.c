#include "assets.h"
#include "test.h"

static int keyed(const image_t *img)
{
    int n = 0;
    for (int i = 0; i < img->w * img->h; i++) {
        n += img->px[i] == GFX_KEY;
    }
    return n;
}

/* The coarse map of where the foreground art is must agree with the art. */
static void test_front_mask(void)
{
    const int art_top = DISPLAY_H - asset_front.h;
    CHECK(ASSET_MASK_COLS * ASSET_MASK_CELL == DISPLAY_W);
    CHECK(ASSET_MASK_ROWS * ASSET_MASK_CELL == DISPLAY_H);
    int wrong = 0, covered = 0;
    for (int row = 0; row < ASSET_MASK_ROWS; row++) {
        for (int col = 0; col < ASSET_MASK_COLS; col++) {
            int any = 0;
            for (int y = row * ASSET_MASK_CELL; y < (row + 1) * ASSET_MASK_CELL; y++) {
                for (int x = col * ASSET_MASK_CELL; x < (col + 1) * ASSET_MASK_CELL; x++) {
                    if (y >= art_top && asset_front.px[(y - art_top) * asset_front.w + x] != GFX_KEY) {
                        any = 1;
                    }
                }
            }
            wrong += (asset_front_mask[row][col] != 0) != any;
            covered += any;
        }
    }
    CHECK(wrong == 0);
    CHECK(covered > 0 && covered < ASSET_MASK_COLS * ASSET_MASK_ROWS);
}

void test_assets(void)
{
    for (int s = 0; s < SPECIES_COUNT; s++) {
        const image_t *fish = &asset_fish[s];
        CHECK(fish->w == 16 && fish->h == 16);
        CHECK(fish->px != NULL);
        int k = keyed(fish);
        CHECK(k > 0 && k < 16 * 16); /* some transparent, some fish */
    }

    CHECK(asset_back.w == DISPLAY_W && asset_back.h == 192);
    CHECK(keyed(&asset_back) == 0); /* fully opaque */

    CHECK(asset_front.w == DISPLAY_W && asset_front.h == 192);
    int k = keyed(&asset_front);
    CHECK(k > 0 && k < DISPLAY_W * 192);

    test_front_mask();

    CHECK(asset_water != GFX_KEY);
    CHECK(asset_water == asset_back.px[0]); /* the top band continues the art's sky colour */
}
