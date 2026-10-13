/* Hosyond / LCDWIKI ES3C28P: pins and panel facts. Nothing else lives here. */
#pragma once

/* Display: ILI9341V over SPI. Its reset line is tied to the chip's reset pin. */
#define PIN_LCD_CS        10
#define PIN_LCD_DC        46
#define PIN_LCD_SCK       12
#define PIN_LCD_MOSI      11
#define PIN_LCD_MISO      13
#define PIN_LCD_BACKLIGHT 45

/* Touch: FT6336G over I2C. */
#define PIN_TOUCH_SDA     16
#define PIN_TOUCH_SCL     15
#define PIN_TOUCH_RST     18
#define PIN_TOUCH_INT     17

/* The panel as built: portrait. */
#define LCD_NATIVE_W      240
#define LCD_NATIVE_H      320

/* Panel quirks, found by looking at the test pattern (see the bring-up plan).
 * LCD_MIRROR_X is 1 because the vendor start-up values in display.c reverse
 * the panel's column scan direction (register B6). */
#define LCD_BGR           1
#define LCD_INVERT        1
#define LCD_SWAP_XY       1
#define LCD_MIRROR_X      1
#define LCD_MIRROR_Y      0

/* Touch orientation, applied to the panel's native axes: mirror first, then swap. */
#define TOUCH_SWAP_XY     1
#define TOUCH_MIRROR_X    1
#define TOUCH_MIRROR_Y    0
