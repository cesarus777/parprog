#pragma once

#include "geometry.h"
#include "scene.h"



SDL_Color cast_ray(const scene_pack_t *pack, int scene_idx, vec3f src,
                   vec3f dir, int depth);

