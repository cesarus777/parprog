#pragma once

#include "geometry.h"



typedef struct {
  triangle_t *triangles;
  int n_triangles;

  flt_type min_x;
  flt_type min_y;
  flt_type min_z;
  flt_type max_x;
  flt_type max_y;
  flt_type max_z;

#ifdef WITH_TEXTURES
  vec2f *texture_verts;
  int n_texture_verts;
#endif
} model_t;



void free_model(model_t *model);

model_t *extract_obj_model_from_file(const char *filename, const vec3f shift, const flt_type scale);

