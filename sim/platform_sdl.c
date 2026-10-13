/* The Mac side of display.h, touch.h and clock.h, on SDL2. */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "clock.h"
#include "display.h"
#include "sim.h"
#include "touch.h"

#define WINDOW_SCALE 3

static SDL_Renderer *renderer;
static SDL_Texture *texture;
static bool mouse_down;
static int mouse_x, mouse_y;
static int last_key;

void display_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_Window *window = SDL_CreateWindow("deskfish", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          DISPLAY_W * WINDOW_SCALE, DISPLAY_H * WINDOW_SCALE, 0);
    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        exit(1);
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        exit(1);
    }
    /* Mouse events are reported in 320x240 coordinates, and scaling stays blocky. */
    SDL_RenderSetLogicalSize(renderer, DISPLAY_W, DISPLAY_H);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                DISPLAY_W, DISPLAY_H);
    if (texture == NULL) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        exit(1);
    }
}

void display_push(const uint16_t *pixels, int x, int y, int w, int h)
{
    if (pixels == NULL || w <= 0 || h <= 0 || x < 0 || y < 0 ||
        w > DISPLAY_W - x || h > DISPLAY_H - y) {
        fprintf(stderr, "display_push: rejected rectangle %d,%d %dx%d\n", x, y, w, h);
        return;
    }
    SDL_Rect rect = {x, y, w, h};
    SDL_UpdateTexture(texture, &rect, pixels, w * (int)sizeof(uint16_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

bool touch_init(void)
{
    return true;
}

bool touch_read(int *x, int *y)
{
    if (!mouse_down || x == NULL || y == NULL) {
        return false;
    }
    *x = mouse_x < 0 ? 0 : mouse_x >= DISPLAY_W ? DISPLAY_W - 1 : mouse_x;
    *y = mouse_y < 0 ? 0 : mouse_y >= DISPLAY_H ? DISPLAY_H - 1 : mouse_y;
    return true;
}

int64_t clock_unix(void)
{
    return (int64_t)time(NULL);
}

uint32_t clock_millis(void)
{
    return SDL_GetTicks();
}

bool sim_pump(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            return false;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            mouse_down = event.button.state == SDL_PRESSED;
            mouse_x = event.button.x;
            mouse_y = event.button.y;
            break;
        case SDL_KEYDOWN:
            if (event.key.repeat == 0) {
                SDL_Keycode key = event.key.keysym.sym;
                if (key == SDLK_f || key == SDLK_r || key == SDLK_n || key == SDLK_d || key == SDLK_t) {
                    last_key = (int)key; /* SDL's letter codes are the ASCII letters */
                }
            }
            break;
        case SDL_MOUSEMOTION:
            mouse_x = event.motion.x;
            mouse_y = event.motion.y;
            break;
        default:
            break;
        }
    }
    return true;
}

void sim_sleep_ms(uint32_t ms)
{
    SDL_Delay(ms);
}

int sim_take_key(void)
{
    int key = last_key;
    last_key = 0;
    return key;
}
