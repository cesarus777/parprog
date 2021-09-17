#pragma once

#include "flt_type.h"

#include <SDL2/SDL_pixels.h>

#include <stddef.h>



enum {
  BITS_PER_COLOR   = 8,
  BITS_PER_COLOR16 = 16,

  MAX_COLOR_MASK   = 0xff,
  MAX_COLOR16_MASK = 0xffff,
};



typedef struct {
  uint16_t r;
  uint16_t g;
  uint16_t b;
  uint16_t a;
} SDL_Color16;



static const SDL_Color BG_CLR = {0x33, 0xb2, 0xcc, 0xff}; // blue
static const SDL_Color WHT    = {0xff, 0xff, 0xff, 0xff}; // white
static const SDL_Color BLK    = {0x00, 0x00, 0x00, 0xff}; // black

static const SDL_Color16 BG_CLR16 = {0x0033, 0x00b2, 0x00cc, 0x00ff}; // blue
static const SDL_Color16 WHT16    = {0x00ff, 0x00ff, 0x00ff, 0x00ff}; // white
static const SDL_Color16 BLK16    = {0x0000, 0x0000, 0x0000, 0x00ff}; // black



SDL_Color convert_color_16to8(SDL_Color16 color16);

SDL_Color   color_with_brightness(SDL_Color color, flt_type light_intensity);
SDL_Color16 color16_set_brightness(SDL_Color16 color, flt_type light_intensity);

SDL_Color   mix_colors(SDL_Color *colors, int n);
SDL_Color   mix_colors_16to8(SDL_Color16 *colors, int n);
SDL_Color16 mix_colors16(SDL_Color16 *colors, int n);

