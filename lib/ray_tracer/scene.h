#pragma once

#include "colors.h"
#include "geometry.h"

#include "flt_type.h"



struct vec3f;



typedef struct {
  SDL_Color16 clr;
  flt_type    albedo[4];
  flt_type    spec_exp;
  flt_type    refractive_index;
} material_t;



typedef struct {
  void *data;

  enum object_type_t {
    NO_TYPE,
    PLANE,
    SPHERE,
    TRIANGLE,
#ifdef WITH_OBJ
    OBJ_MODEL,
#endif
  } type;

  int mtrl_idx;
} object_t;



typedef struct {
  vec3f    pos;
  flt_type intensity;
} light_t;



typedef struct {
  int width;
  int height;

  vec3f    view_point;
  vec3f    view_dir;
  flt_type fov;

  int cast_depth;

  int *objects;
  int  n_objects;

  int *lights;
  int  n_lights;
} scene_t;



typedef struct {
  object_t *objects;
  int       n_objects;

  light_t *lights;
  int      n_lights;

  material_t *materials;
  int         n_materials;

  scene_t *scenes;
  int      n_scenes;
} scene_pack_t;



scene_pack_t *get_scenes(const char *scenes_file);
void free_scene_pack(scene_pack_t *pack);

