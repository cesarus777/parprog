#include "colors.h"
#include "geometry.h"

#include "flt_type.h"

#include <assert.h>



#define EPSILON 0.001



SDL_Color convert_color_16to8(SDL_Color16 color16) {
  SDL_Color color;
  color.r = (color16.r > MAX_COLOR_MASK) ? MAX_COLOR_MASK : color16.r;
  color.g = (color16.g > MAX_COLOR_MASK) ? MAX_COLOR_MASK : color16.g;
  color.b = (color16.b > MAX_COLOR_MASK) ? MAX_COLOR_MASK : color16.b;
  color.a = (color16.a > MAX_COLOR_MASK) ? MAX_COLOR_MASK : color16.a;
  return color;
}

SDL_Color color_set_brightness(SDL_Color color, flt_type light_intensity) {
  vec3f clr    = {(flt_type) color.r, (flt_type) color.g, (flt_type) color.b};
  clr          = vec3f_mul(clr, light_intensity);
  flt_type max = flt_max(clr.x, flt_max(clr.y, clr.z));

  if (max > MAX_COLOR_MASK) {
    clr = vec3f_mul(clr, MAX_COLOR_MASK / max);
  }

  color.r = (uint8_t) flt_max(0.0, flt_min(MAX_COLOR_MASK, clr.x));
  color.g = (uint8_t) flt_max(0.0, flt_min(MAX_COLOR_MASK, clr.y));
  color.b = (uint8_t) flt_max(0.0, flt_min(MAX_COLOR_MASK, clr.z));

  return color;
}

SDL_Color16 color16_set_brightness(SDL_Color16 color,
                                   flt_type    light_intensity) {
  if (light_intensity < EPSILON) {
    SDL_Color16 clr = {0, 0, 0, MAX_COLOR16_MASK};
    return clr;
  }

  vec3f clr_f  = {(flt_type) color.r, (flt_type) color.g, (flt_type) color.b};
  clr_f        = vec3f_mul(clr_f, light_intensity);
  flt_type max = flt_max(clr_f.x, flt_max(clr_f.y, clr_f.z));

  if (max > MAX_COLOR16_MASK) {
    clr_f = vec3f_mul(clr_f, MAX_COLOR16_MASK / max);
  }

  color.r = (uint16_t) flt_max(0.0, flt_min(MAX_COLOR16_MASK, clr_f.x));
  color.g = (uint16_t) flt_max(0.0, flt_min(MAX_COLOR16_MASK, clr_f.y));
  color.b = (uint16_t) flt_max(0.0, flt_min(MAX_COLOR16_MASK, clr_f.z));

  return color;
}

SDL_Color mix_colors(SDL_Color *colors, int n) {
  if (n <= 0) {
    return BLK;
  }

  assert(colors);

  uint32_t components[4] = {0, 0, 0, 0};
  for (int i = 0; i < n; ++i) {
    components[0] += colors[i].r;
    components[1] += colors[i].g;
    components[2] += colors[i].b;
    components[3] += colors[i].a;
  }

  SDL_Color mix = {
      components[0] / n,
      components[1] / n,
      components[2] / n,
      components[3] / n,
  };

  return mix;
}

SDL_Color mix_colors_16to8(SDL_Color16 *colors, int n) {
  if (n <= 0) {
    return BLK;
  }

  assert(colors);

  uint32_t components[4] = {0, 0, 0, 0};
  for (int i = 0; i < n; ++i) {
    components[0] += colors[i].r;
    components[1] += colors[i].g;
    components[2] += colors[i].b;
    components[3] += colors[i].a;
  }

  SDL_Color mix = {
      components[0] / n,
      components[1] / n,
      components[2] / n,
      components[3] / n,
  };

  return mix;
}

SDL_Color16 mix_colors16(SDL_Color16 *colors, int n) {
  if (n <= 0) {
    return BLK16;
  }

  assert(colors);

  uint32_t components[4] = {0, 0, 0, 0};
  for (int i = 0; i < n; ++i) {
    components[0] += colors[i].r;
    components[1] += colors[i].g;
    components[2] += colors[i].b;
    components[3] += colors[i].a;
  }

  SDL_Color16 mix = {
      components[0],
      components[1],
      components[2],
      components[3],
  };

  return mix;
}

