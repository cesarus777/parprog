#include "geometry.h"
#include "scene.h"

#include "flt_type.h"

#include <SDL2/SDL_pixels.h>

#include <assert.h>
#include <math.h>



flt_type flt_min(flt_type f1, flt_type f2) { return (f1 < f2) ? f1 : f2; }

flt_type flt_max(flt_type f1, flt_type f2) { return (f1 > f2) ? f1 : f2; }



vec3f get_vec3f(const flt_type x, const flt_type y, const flt_type z) {
  vec3f res = {x, y, z};
  return res;
}

vec3f vec3f_add(const vec3f v1, const vec3f v2) {
  vec3f res = {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
  return res;
}

vec3f vec3f_sub(const vec3f v1, const vec3f v2) {
  vec3f res = {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
  return res;
}

vec3f vec3f_mul(const vec3f v, const flt_type f) {
  vec3f res = {v.x * f, v.y * f, v.z * f};
  return res;
}

flt_type vec3f_scalar_mul(const vec3f v1, const vec3f v2) {
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

vec3f vec3f_vec_mul(const vec3f v1, const vec3f v2) {
  vec3f res = {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
               v1.x * v2.y - v1.y * v2.x};
  return res;
}

flt_type vec3f_norm(const vec3f v) { return flt_sqrt(vec3f_scalar_mul(v, v)); }

vec3f vec3f_normalize(vec3f v) {
  flt_type norm = vec3f_norm(v);
  vec3f    res  = {
      v.x / norm,
      v.y / norm,
      v.z / norm,
  };
  return res;
}

