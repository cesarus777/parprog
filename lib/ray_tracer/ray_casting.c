#include "ray_casting.h"
#include "colors.h"
#include "geometry.h"

#include "flt_type.h"

#include <assert.h>
#include <math.h>



int ray_intersect_sphere(const sphere_t *sphere, const vec3f src,
                         const vec3f dir, flt_type *dist) {
  assert(sphere);

  vec3f    dist_center = vec3f_sub(sphere->center, src);
  flt_type mul         = vec3f_scalar_mul(dist_center, dir);

  flt_type discriminant = sphere->radius * sphere->radius -
                          vec3f_scalar_mul(dist_center, dist_center) +
                          mul * mul;
  if (discriminant < 0) {
    return 0;
  }

  flt_type disc_sqrt = sqrt(discriminant);
  flt_type distance = (mul - disc_sqrt) < 0 ? mul + disc_sqrt : mul - disc_sqrt;

  if (dist != NULL) {
    *dist = distance;
  }

  return distance < 0 ? 0 : 1;
}

int ray_intersect_plane(const plane_t *plane, const vec3f src, const vec3f dir,
                        flt_type *dist) {
  assert(plane);

  flt_type distance = (vec3f_scalar_mul(plane->r0, plane->n) -
                       vec3f_scalar_mul(src, plane->n)) /
                      vec3f_scalar_mul(dir, plane->n);

  if (dist != NULL) {
    *dist = distance;
  }

  return distance < 0 ? 0 : 1;
}

// Moller-Trumbore intersection algorithm
int ray_intersect_triangle(const triangle_t *triangle, const vec3f src,
                           const vec3f dir, flt_type *dist) {
  assert(triangle);

  vec3f    ab    = vec3f_sub(triangle->b, triangle->a);
  vec3f    bc    = vec3f_sub(triangle->c, triangle->b);
  vec3f    p_vec = vec3f_vec_mul(dir, bc);
  flt_type mul   = vec3f_scalar_mul(ab, p_vec);

  const flt_type epsilon = 0.00001;
  if (flt_abs(mul) < epsilon) {
    return 0; // ray is parallel to the triangle
  }

  vec3f    t_vec = vec3f_sub(src, triangle->a);
  flt_type u     = vec3f_scalar_mul(t_vec, p_vec) / mul;
  if ((u < 0) || (u > 1)) {
    return 0;
  }

  vec3f    q_vec = vec3f_vec_mul(t_vec, ab);
  flt_type v     = vec3f_scalar_mul(dir, q_vec) / mul;
  if ((v < 0) || (u + v > 1)) {
    return 0;
  }

  flt_type distance = vec3f_scalar_mul(bc, q_vec) / mul;

  if (dist != NULL) {
    *dist = distance;
  }

  return distance < 0 ? 0 : 1;
}

#ifdef WITH_OBJ
int ray_intersect_model(const model_t *model, const vec3f src, const vec3f dir,
                        flt_type *dist) {
  assert(model);

  int      intersect_face    = -1;
  flt_type shortest_distance = FLT_TYPE_MAX;
  for (int i = 0; i < model->n_triangles; i++) {
    flt_type distance = FLT_TYPE_MAX;
    if (ray_intersect_triangle(model->triangles + i, src, dir, &distance) &&
        (distance < shortest_distance)) {
      shortest_distance = distance;
      intersect_face    = i + 1;
    }
  }

  if (dist != NULL)
    *dist = shortest_distance;

  return intersect_face > 0 ? intersect_face : 0;
}
#endif

int ray_intersect(const object_t *object, const vec3f src, const vec3f dir,
                  flt_type *dist) {
  assert(object);
#ifndef NDEBUG
  const flt_type epsilon = 0.00001;
  if (flt_abs(vec3f_norm(dir) - FLT_ONE) < epsilon) {
    fprintf(stderr,
            "fatal error: %s:%d in function '%s': 'dir' must be a normalized "
            "vector!\n",
            __FILE__, __LINE__, __func__);
    abort();
  }
#endif

  switch (object->type) {
  case SPHERE:
    return ray_intersect_sphere((sphere_t *) object->data, src, dir, dist);
    break;

  case PLANE:
    return ray_intersect_plane((plane_t *) object->data, src, dir, dist);
    break;

  case TRIANGLE:
    return ray_intersect_triangle((triangle_t *) object->data, src, dir, dist);
    break;

#ifdef WITH_OBJ
  case OBJ_MODEL:
    return ray_intersect_model((model_t *) object->data, src, dir, dist);
    break;
#endif

  default:
    return 0;
    break;
  }

  return 0;
}

void calculate_intercection(const scene_pack_t *pack, const int object_idx,
                            intersection_t *intersection, const vec3f src,
                            const vec3f dir, const flt_type shortest_dist) {
  assert(pack);
  assert(object_idx < pack->n_objects);

  intersection->point = vec3f_add(src, vec3f_mul(dir, shortest_dist));
  object_t *object    = &pack->objects[object_idx];

  switch (object->type) {
  case SPHERE: {
    sphere_t *sphere = object->data;
    intersection->normal =
        vec3f_normalize(vec3f_sub(intersection->point, sphere->center));
    break;
  }
  case PLANE: {
    plane_t *plane = object->data;

    if (vec3f_scalar_mul(dir, plane->n) <= 0) {
      intersection->normal = plane->n;
    } else {
      intersection->normal = vec3f_mul(plane->n, -1.0);
    }
    break;
  }
  case TRIANGLE: {
    triangle_t *triangle = object->data;

    vec3f ab              = vec3f_sub(triangle->b, triangle->a);
    vec3f bc              = vec3f_sub(triangle->c, triangle->b);
    vec3f triangle_normal = vec3f_normalize(vec3f_vec_mul(ab, bc));
    triangle_normal       = vec3f_normalize(triangle_normal);
    if (vec3f_scalar_mul(dir, triangle_normal) <= 0) {
      intersection->normal = triangle_normal;
    } else {
      intersection->normal = vec3f_mul(triangle_normal, -1.0);
    }
    break;
  }
#ifdef WITH_OBJ
  case OBJ_MODEL: {
    model_t *  model    = object->data;
    triangle_t triangle = model->triangles[intersect];

    vec3f ab              = vec3f_sub(triangle.b, triangle.a);
    vec3f bc              = vec3f_sub(triangle.c, triangle.b);
    vec3f triangle_normal = vec3f_normalize(vec3f_vec_mul(ab, bc));
    triangle_normal       = vec3f_normalize(triangle_normal);
    if (vec3f_scalar_mul(dir, triangle_normal) <= 0)
      intersection->normal = triangle_normal;
    else
      intersection->normal = vec3f_mul(triangle_normal, -1.0);
    break;
  }
#endif
  default:
    assert(0 && "bad object type in scene_intersect!");
    break;
  }
}

int scene_intersect(const scene_pack_t *pack, int scene_idx, const vec3f src,
                    const vec3f dir, intersection_t *intersection,
                    int *material_index) {
  assert(pack);
  assert(scene_idx < pack->n_scenes);

  flt_type       shortest_dist = FLT_TYPE_MAX;
  const scene_t *scene         = &pack->scenes[scene_idx];

  for (int i = 0; i < scene->n_objects; ++i) {
    flt_type tmp_dist = FLT_TYPE_MAX;

    int       object_idx = scene->objects[i];
    object_t *object     = &pack->objects[object_idx];

    int intersect = ray_intersect(object, src, dir, &tmp_dist);
    if (intersect && (tmp_dist < shortest_dist)) {
      shortest_dist = tmp_dist;

      if (intersection != NULL) {
        calculate_intercection(pack, object_idx, intersection, src, dir,
                               shortest_dist);
      }

      if (material_index != NULL) {
        *material_index = object->mtrl_idx;
      }
    }
  }

  return shortest_dist < FLT_TYPE_MAX;
}



vec3f refract(const vec3f I, const vec3f normal, const flt_type eta_t,
              const flt_type eta_i) {
  flt_type cos_i = -flt_max(-1.0, flt_min(1.0, vec3f_scalar_mul(I, normal)));

  if (cos_i < 0.0) {
    return refract(I, vec3f_mul(normal, -1.0), eta_i, eta_t);
  }

  flt_type eta = eta_i / eta_t;
  flt_type k   = 1 - eta * eta * (1 - cos_i * cos_i);
  return (k < 0) ? get_vec3f(1.0, 0.0, 0.0)
                 : vec3f_add(vec3f_mul(I, eta),
                             vec3f_mul(normal, eta * cos_i - sqrt(k)));
}



vec3f reflect(const vec3f I, const vec3f normal) {
  return vec3f_sub(I, vec3f_mul(normal, 2 * vec3f_scalar_mul(I, normal)));
}



SDL_Color16 cast_ray16(const scene_pack_t *pack, int scene_idx, vec3f src,
                       vec3f dir, int depth);



SDL_Color16 calculate_reflect_color(const scene_pack_t *pack, int scene_idx,
                                    intersection_t intersection, vec3f dir,
                                    int depth) {
  vec3f reflect_dir = reflect(dir, intersection.normal);

  const flt_type epsilon = 0.001;
  vec3f reflect_src = vec3f_scalar_mul(reflect_dir, intersection.normal) < 0
                          ? vec3f_sub(intersection.point,
                                      vec3f_mul(intersection.normal, epsilon))
                          : vec3f_add(intersection.point,
                                      vec3f_mul(intersection.normal, epsilon));

  SDL_Color16 reflect_color =
      cast_ray16(pack, scene_idx, reflect_src, reflect_dir, depth - 1);

  return reflect_color;
}



SDL_Color16 calculate_refract_color(const scene_pack_t *pack, int scene_idx,
                                    intersection_t intersection, vec3f dir,
                                    material_t *material, int depth) {
  vec3f refract_dir = vec3f_normalize(
      refract(dir, intersection.normal, material->refractive_index, 1.0));

  const flt_type epsilon = 0.001;
  vec3f refract_src = vec3f_scalar_mul(refract_dir, intersection.normal) < 0
                          ? vec3f_sub(intersection.point,
                                      vec3f_mul(intersection.normal, epsilon))
                          : vec3f_add(intersection.point,
                                      vec3f_mul(intersection.normal, epsilon));

  SDL_Color16 refract_color =
      cast_ray16(pack, scene_idx, refract_src, refract_dir, depth - 1);

  return refract_color;
}



int is_invisible_side(const scene_pack_t *pack, int scene_idx, vec3f shadow_src,
                      vec3f light_dir, flt_type light_dist) {
  intersection_t *shadow_intersection = malloc(sizeof(intersection_t));

  int intersect = scene_intersect(pack, scene_idx, shadow_src, light_dir,
                                  shadow_intersection, NULL);
  if (!intersect) {
    free(shadow_intersection);
    return 0;
  }

  vec3f    shadow_vec  = vec3f_sub(shadow_src, shadow_intersection->point);
  flt_type shadow_dist = vec3f_norm(shadow_vec);
  free(shadow_intersection);

  return shadow_dist < light_dist;
}



SDL_Color16 cast_ray16(const scene_pack_t *pack, int scene_idx, const vec3f src,
                       const vec3f dir, int depth) {
  assert(pack);
  assert(scene_idx < pack->n_scenes);

  scene_t *      scene    = &pack->scenes[scene_idx];
  int            mtrl_idx = -1;
  intersection_t intersection;
  const flt_type epsilon = 0.001;

  // TODO: rewrite recursion with depth control to cycle
  if ((depth < 0) || (scene_intersect(pack, scene_idx, src, dir, &intersection,
                                      &mtrl_idx) == 0)) {
    return BG_CLR16;
  }

  material_t *material = &pack->materials[mtrl_idx];

  SDL_Color16 reflect_color = material->clr;
  SDL_Color16 refract_color = material->clr;

  // calculate reflection
  if (material->albedo[2] > epsilon) {
    // TODO: rewrite recursion with depth control to cycle
    reflect_color =
        calculate_reflect_color(pack, scene_idx, intersection, dir, depth);
  }

  // calculate refraction
  if (material->albedo[3] > epsilon) {
    // TODO: rewrite recursion with depth control to cycle
    refract_color = calculate_refract_color(pack, scene_idx, intersection, dir,
                                            material, depth);
  }

  // calculate differential and specular light
  flt_type diff_light_intensity = 0.0;
  flt_type spec_light_intensity = 0.0;
  for (int i = 0; i < scene->n_lights; ++i) {
    light_t *light = &pack->lights[i];

    vec3f    light_dir  = vec3f_sub(light->pos, intersection.point);
    flt_type light_dist = vec3f_norm(light_dir);
    light_dir           = vec3f_normalize(light_dir);

    vec3f shadow_src = vec3f_scalar_mul(light_dir, intersection.normal) < 0
                           ? vec3f_sub(intersection.point,
                                       vec3f_mul(intersection.normal, epsilon))
                           : vec3f_add(intersection.point,
                                       vec3f_mul(intersection.normal, epsilon));

    if (is_invisible_side(pack, scene_idx, shadow_src, light_dir, light_dist)) {
      continue;
    }

    flt_type intensity = vec3f_scalar_mul(light_dir, intersection.normal);
    diff_light_intensity += light->intensity * flt_max(0.0, intensity);

    flt_type reflection =
        vec3f_scalar_mul(reflect(light_dir, intersection.normal), dir);
    spec_light_intensity +=
        pow(flt_max(0.0, reflection), material->spec_exp) * light->intensity;
  }

  // calculate result color
  SDL_Color16 diff_color = color16_set_brightness(
      material->clr, diff_light_intensity * material->albedo[0]);

  SDL_Color16 spec_color =
      color16_set_brightness(WHT16, spec_light_intensity * material->albedo[1]);

  reflect_color = color16_set_brightness(reflect_color, material->albedo[2]);
  refract_color = color16_set_brightness(refract_color, material->albedo[3]);
  SDL_Color16 colors[] = {diff_color, spec_color, reflect_color, refract_color};

  return mix_colors16(colors, sizeof(colors) / sizeof(SDL_Color16));
}

SDL_Color cast_ray(const scene_pack_t *pack, int scene_idx, const vec3f src,
                   const vec3f dir, int depth) {
  SDL_Color16 pixel16 = cast_ray16(pack, scene_idx, src, dir, depth);
  return convert_color_16to8(pixel16);
}

