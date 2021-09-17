#pragma once

#include "colors.h"

#include "flt_type.h"

#include <SDL2/SDL_pixels.h>

#include <float.h>
#include <stddef.h>



typedef struct {
  flt_type x;
  flt_type y;
  flt_type z;
} vec3f;



typedef struct {
  vec3f    center;
  flt_type radius;
} sphere_t;

typedef struct {
  vec3f a;
  vec3f b;
  vec3f c;
} triangle_t;

typedef struct {
  vec3f r0;
  vec3f n;
} plane_t;



typedef struct {
  vec3f point;
  vec3f normal;
} intersection_t;



flt_type flt_min(flt_type f1, flt_type f2);
flt_type flt_max(flt_type f1, flt_type f2);

vec3f    get_vec3f(flt_type x, flt_type y, flt_type z);
vec3f    vec3f_add(vec3f v1, vec3f v2);
vec3f    vec3f_sub(vec3f v1, vec3f v2);
vec3f    vec3f_mul(vec3f v1, flt_type f);
vec3f    vec3f_vec_mul(vec3f v1, vec3f v2);
flt_type vec3f_scalar_mul(vec3f v1, vec3f v2);
flt_type vec3f_norm(vec3f v);
vec3f    vec3f_normalize(vec3f v);

