#include "percept_text.h"

#include <stdio.h>

#include "species.h"

static bool is_digit(int v)
{
    return v >= 0 && v <= 9;
}

static bool is_dist(dist_t d)
{
    return (int)d >= DIST_NONE && (int)d <= DIST_FAR;
}

int percept_encode(const percept_t *p, char *buf, size_t size)
{
    if (size > 0) {
        buf[0] = '\0';
    }
    const species_info_t *info = species_info(p->species);
    if (info == NULL || !is_digit(p->hunger) || !is_digit(p->energy) || !is_digit(p->stress) ||
        !is_digit(p->bold) || !is_digit(p->social) || (int)p->current < 0 || (int)p->current >= GOAL_COUNT ||
        !is_dist(p->food) || !is_dist(p->friend_) || !is_dist(p->finger) || !is_dist(p->cover)) {
        return -1;
    }
    if (size == 0) {
        return -1;
    }

    int n = snprintf(buf, size,
                     "species %s hunger %d energy %d stress %d bold %d social %d time %s startled %s doing %s "
                     "food %s friend %s finger %s cover %s",
                     info->name, p->hunger, p->energy, p->stress, p->bold, p->social,
                     p->night ? "night" : "day", p->startled ? "yes" : "no", goal_name(p->current),
                     dist_name(p->food), dist_name(p->friend_), dist_name(p->finger), dist_name(p->cover));
    if (n < 0 || (size_t)n >= size) {
        buf[0] = '\0'; /* a cut-off line could parse as a different situation */
        return -1;
    }
    return n;
}
