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

    CHECK(asset_water != GFX_KEY);
    CHECK(asset_water == asset_back.px[0]); /* the top band continues the art's sky colour */
}
