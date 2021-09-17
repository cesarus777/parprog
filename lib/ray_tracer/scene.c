#include "scene.h"
#include "geometry.h"

#include "flt_type.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>



// {{{ Utility
// clang-format off
#define ever ;;
// clang-format on

enum {
  LINE_LEN = 128,
  LEX_LEN  = 128,
};

enum {
  N_LGHT_FLTS  = 4,
  N_MTRL_FLTS  = 6,
  N_SPHR_FLTS  = 4,
  N_PLANE_FLTS = 6,
  N_TRIAN_FLTS = 9,
  N_MODEL_FLTS = 4,
  N_SCENE_FLTS = 7,
};

enum {
  STRTOL_BASE = 10,
};



ssize_t skip_line(FILE *file) {
  char *  tmp    = NULL;
  size_t  tmp_sz = 0;
  ssize_t ret    = getline(&tmp, &tmp_sz, file);
  free(tmp);
  return ret;
}

// in .rtr commented lines starts from "//"
int is_comment(const char *lexem) {
  return (lexem[0] == '/') && (lexem[1] == '/');
}

int skip_comments(FILE *file, char *lexem) {
  if (fscanf(file, "%s", lexem) != 1) {
    return -1;
  }

  while (is_comment(lexem)) {
    skip_line(file);

    if (fscanf(file, "%s", lexem) != 1) {
      return -1;
    }
  }
  return 0;
}

// in .rtr "--" is delimiter between sections
int is_delimiter(const char *lexem) {
  return (lexem[0] == '-') && (lexem[1] == '-');
}

// scan "#[0-9]*" constructions
int scan_index(const char *lexem) {
  if ((lexem == NULL) || (strlen(lexem) < 2)) {
    return -1;
  }

  if (lexem[0] != '#') {
    return -1;
  }

  int index;
  if ((sscanf(lexem + 1, "%d", &index) != 1) || (index < 0)) {
    return -1;
  }

  return index;
}

int scan_arr_of_doubles(FILE *file, double *arr, int n) {
  assert(file);
  assert(arr);
  assert(n > 0);

  int n_scanned = 0;
  for (int i = 0; i < n; i++) {
    n_scanned += fscanf(file, "%lf", arr + i);
  }

  return n_scanned;
}

int scan_pure_nums(FILE *file, int **arr, char *lexem) {
  assert(file);
  assert(arr);
  assert(lexem);

  int  n_scanned = 0;
  int *local_arr = NULL;
  if (fscanf(file, "%s", lexem) != 1) {
    return 0;
  }

  char *endptr = "\0";
  errno        = 0;
  long int num = strtol(lexem, &endptr, STRTOL_BASE);
  if ((errno == ERANGE && (num == LONG_MAX || num == LONG_MIN)) ||
      (errno != 0 && num == 0)) {
    free(local_arr);
    return -1;
  }

  while ((lexem != endptr) && fscanf(file, "%s", lexem)) {
    n_scanned++;
    local_arr                = realloc(local_arr, n_scanned * sizeof(int));
    local_arr[n_scanned - 1] = num;

    errno = 0;
    num   = strtol(lexem, &endptr, STRTOL_BASE);
    if ((errno == ERANGE && (num == LONG_MAX || num == LONG_MIN)) ||
        (errno != 0 && num == 0)) {
      free(local_arr);
      return -1;
    }
  }


  *arr = local_arr;

  return n_scanned;
}

int scan_n_pure_nums(FILE *file, int *arr, int n) {
  assert(file);
  assert(arr);

  int   n_scanned = 0;
  char  lexem[LEX_LEN];
  char *endptr;
  for (int i = 0; i < n; i++) {
    if (fscanf(file, "%s", lexem) != 1) {
      return n_scanned;
    }

    errno        = 0;
    long int num = strtol(lexem, &endptr, STRTOL_BASE);
    if ((errno == ERANGE && (num == LONG_MAX || num == LONG_MIN)) ||
        (errno != 0 && num == 0)) {
      return 0;
    }

    arr[i] = num;
    n_scanned++;
  }

  return n_scanned;
}

int scan_color(FILE *file, SDL_Color16 *clr) {
  assert(file);
  assert(clr);

  uint32_t hex_color;
  if (fscanf(file, "%x", &hex_color) != 1) {
    return -1;
  }

  clr->r = (hex_color >> (BITS_PER_COLOR * 3)) & MAX_COLOR_MASK;
  clr->g = (hex_color >> (BITS_PER_COLOR * 2)) & MAX_COLOR_MASK;
  clr->b = (hex_color >> (BITS_PER_COLOR * 1)) & MAX_COLOR_MASK;
  clr->a = (hex_color >> (BITS_PER_COLOR * 0)) & MAX_COLOR_MASK;

  return 0;
}
// }}}

// {{{ Extract lights
int extract_lights(FILE *pack_file, light_t **lights) {
  *lights               = NULL;
  light_t *local_lights = NULL;
  int      n_lights     = 0;

  char lexem[LEX_LEN];
  while (fscanf(pack_file, "%s", lexem) != 0) {
    if (is_comment(lexem)) {
      skip_line(pack_file);
      continue;
    }

    if (is_delimiter(lexem)) {
      break;
    }

    const int index = scan_index(lexem);
    if (index < 0) {
      free(local_lights);
      return 0;
    }

    n_lights++;

    local_lights = realloc(local_lights, n_lights * sizeof(light_t));
    assert(local_lights);

    double flt_tmp[N_LGHT_FLTS];
    int    n_scanned = scan_arr_of_doubles(pack_file, flt_tmp, N_LGHT_FLTS);
    if (n_scanned != N_LGHT_FLTS) {
      free(local_lights);
      return 0;
    }

    local_lights[index].pos.x     = (flt_type) flt_tmp[0];
    local_lights[index].pos.y     = (flt_type) flt_tmp[1];
    local_lights[index].pos.z     = (flt_type) flt_tmp[2];
    local_lights[index].intensity = (flt_type) flt_tmp[3];
  }

  *lights = local_lights;

  return n_lights;
}
// }}}

// {{{ Extract materials
int extract_materials(FILE *pack_file, material_t **materials) {
  *materials                  = NULL;
  material_t *local_materials = NULL;
  int         n_materials     = 0;

  char lexem[LEX_LEN];
  while (fscanf(pack_file, "%s", lexem) != 0) {
    if (is_comment(lexem)) {
      skip_line(pack_file);
      continue;
    }

    if (is_delimiter(lexem)) {
      break;
    }

    const int index = scan_index(lexem);
    if (index < 0) {
      free(local_materials);
      return 0;
    }

    n_materials++;

    local_materials =
        realloc(local_materials, n_materials * sizeof(material_t));
    assert(local_materials);

    if (scan_color(pack_file, &local_materials[index].clr)) {
      free(local_materials);
      return 0;
    }

    double flt_tmp[N_MTRL_FLTS];
    int    n_scanned = scan_arr_of_doubles(pack_file, flt_tmp, N_MTRL_FLTS);
    if (n_scanned != N_MTRL_FLTS) {
      free(local_materials);
      return 0;
    }

    local_materials[index].albedo[0] = (flt_type) flt_tmp[0];
    local_materials[index].albedo[1] = (flt_type) flt_tmp[1];
    local_materials[index].albedo[2] = (flt_type) flt_tmp[2];
    local_materials[index].albedo[3] = (flt_type) flt_tmp[3];
    local_materials[index].spec_exp  = (flt_type) flt_tmp[4];
    local_materials[index].refractive_index =
        (flt_type) flt_tmp[5]; // NOLINT: not a magic number
  }

  *materials = local_materials;

  return n_materials;
}
// }}}

// {{{ Extract objects
object_t *extract_sphere_to_object(FILE *pack_file) {
  double flt_tmp[N_SPHR_FLTS];
  int    n_scanned = scan_arr_of_doubles(pack_file, flt_tmp, N_SPHR_FLTS);
  if (n_scanned != N_SPHR_FLTS) {
    return NULL;
  }

  int material_index;
  if (scan_n_pure_nums(pack_file, &material_index, 1) != 1) {
    return NULL;
  }

  sphere_t *sphere = malloc(sizeof(sphere_t));
  assert(sphere);

  sphere->center.x = flt_tmp[0];
  sphere->center.y = flt_tmp[1];
  sphere->center.z = flt_tmp[2];
  sphere->radius   = flt_tmp[3];

  object_t *object = malloc(sizeof(object_t));
  assert(object);
  object->type     = SPHERE;
  object->data     = (void *) sphere;
  object->mtrl_idx = material_index;

  return object;
}



object_t *extract_plane_to_object(FILE *pack_file) {
  double flt_tmp[N_PLANE_FLTS];
  int    n_scanned = scan_arr_of_doubles(pack_file, flt_tmp, N_PLANE_FLTS);
  if (n_scanned != N_PLANE_FLTS) {
    return NULL;
  }

  int material_index;
  if (scan_n_pure_nums(pack_file, &material_index, 1) != 1) {
    return NULL;
  }

  plane_t *plane = malloc(sizeof(plane_t));
  assert(plane);

  plane->r0.x = flt_tmp[0];
  plane->r0.y = flt_tmp[1];
  plane->r0.z = flt_tmp[2];
  plane->n.x  = flt_tmp[3];
  plane->n.y  = flt_tmp[4];
  plane->n.z  = flt_tmp[5]; // NOLINT: not a magic number
  plane->n    = vec3f_normalize(plane->n);

  object_t *object = malloc(sizeof(object_t));
  assert(object);
  object->type     = PLANE;
  object->data     = (void *) plane;
  object->mtrl_idx = material_index;

  return object;
}



object_t *extract_triangle_to_object(FILE *pack_file) {
  double flt_tmp[N_TRIAN_FLTS];
  int    n_scanned = scan_arr_of_doubles(pack_file, flt_tmp, N_TRIAN_FLTS);
  if (n_scanned != N_TRIAN_FLTS) {
    return NULL;
  }

  int material_index;
  if (scan_n_pure_nums(pack_file, &material_index, 1) != 1) {
    return NULL;
  }

  triangle_t *triangle = malloc(sizeof(triangle_t));
  assert(triangle);

  triangle->a.x = flt_tmp[0];
  triangle->a.y = flt_tmp[1];
  triangle->a.z = flt_tmp[2];
  triangle->b.x = flt_tmp[3];
  triangle->b.y = flt_tmp[4];
  triangle->b.z = flt_tmp[5]; // NOLINT: not a magic number
  triangle->c.x = flt_tmp[6]; // NOLINT: not a magic number
  triangle->c.y = flt_tmp[7]; // NOLINT: not a magic number
  triangle->c.z = flt_tmp[8]; // NOLINT: not a magic number

  object_t *object = malloc(sizeof(object_t));
  assert(object);
  object->type     = TRIANGLE;
  object->data     = (void *) triangle;
  object->mtrl_idx = material_index;

  return object;
}



#ifdef WITH_OBJ
  #include "obj_model.h"

object_t *extract_model_to_object(FILE *pack_file, material_t *materials,
                                  int n_materials) {
  char filename[LINE_LEN];
  if (fscanf(pack_file, "%s", filename) != 1)
    return NULL;

  double flt_tmp[N_MODEL_FLTS];
  int    n_scanned = scan_arr_of_doubles(pack_file, flt_tmp, N_MODEL_FLTS);
  if (n_scanned != N_MODEL_FLTS)
    return NULL;

  vec3f    shift = {flt_tmp[0], flt_tmp[1], flt_tmp[2]};
  flt_type scale = flt_tmp[3];

  int material_index;
  if (scan_n_pure_nums(pack_file, &material_index, 1) != 1)
    return NULL;

  if (material_index >= n_materials)
    return NULL;

  model_t *model = extract_obj_model_from_file(filename, shift, scale);
  if (model == NULL)
    return NULL;

  object_t *object = malloc(sizeof(object_t));
  assert(object);
  object->type = OBJ_MODEL;
  object->data = (void *) model;
  object->mtrl = materials + material_index;

  return object;
}
#endif



void free_object(object_t *object) {
  if (object != NULL) {
#ifdef WITH_OBJ
    if (object->type == OBJ_MODEL)
      free_model(object->data);
    else
#endif
      free(object->data);

    free(object);
  }
}



int extract_objects(FILE *pack_file, object_t **objects) {
  *objects                         = NULL;
  object_t *         local_objects = NULL;
  int                n_objects     = 0;
  enum object_type_t curr_type     = NO_TYPE;

  char lexem[LEX_LEN];

  for (ever) {
    skip_comments(pack_file, lexem);
    if (is_delimiter(lexem)) {
      break;
    }

    if (strcmp(lexem, "spheres") == 0) {
      curr_type = SPHERE;
      skip_comments(pack_file, lexem);
    } else if (strcmp(lexem, "planes") == 0) {
      curr_type = PLANE;
      skip_comments(pack_file, lexem);
    } else if (strcmp(lexem, "triangles") == 0) {
      curr_type = TRIANGLE;
      skip_comments(pack_file, lexem);
#ifdef WITH_OBJ
    } else if (strcmp(lexem, "models") == 0) {
      curr_type = OBJ_MODEL;
      skip_comments(pack_file, lexem);
#endif
    }

    const int index = scan_index(lexem);
    if (index < 0) {
      free(local_objects);
      return 0;
    }

    n_objects++;

    local_objects = realloc(local_objects, n_objects * sizeof(object_t));

    switch (curr_type) {
    case SPHERE: {
      object_t *object = extract_sphere_to_object(pack_file);
      if (object == NULL) {
        free(local_objects);
        return 0;
      }
      local_objects[index] = *object;
      free(object);
      break;
    }
    case PLANE: {
      object_t *object = extract_plane_to_object(pack_file);
      if (object == NULL) {
        free(local_objects);
        return 0;
      }
      local_objects[index] = *object;
      free(object);
      break;
    }
    case TRIANGLE: {
      object_t *object = extract_triangle_to_object(pack_file);
      if (object == NULL) {
        free(local_objects);
        return 0;
      }
      local_objects[index] = *object;
      free(object);
      break;
    }
#ifdef WITH_OBJ
    case OBJ_MODEL: {
      object_t *object =
          extract_model_to_object(pack_file, materials, n_materials);
      if (object == NULL) {
        free(local_objects);
        return 0;
      }
      local_objects[index] = *object;
      free(object);
      break;
    }
#endif
    default:
      free(local_objects);
      return 0;
      break;
    }
  }

  *objects = local_objects;

  return n_objects;
}
// }}}

// {{{ Extract scenes
int scan_scene_line(FILE *pack_file, scene_t *scene) {
  if (fscanf(pack_file, "%dx%d", &scene->width, &scene->height) != 2) {
    return 0;
  }

  double flt_tmp[N_SCENE_FLTS];
  int    n_scanned = scan_arr_of_doubles(pack_file, flt_tmp, N_SCENE_FLTS);
  if (n_scanned != N_SCENE_FLTS) {
    return 0;
  }

  scene->view_point.x = (flt_type) flt_tmp[0];
  scene->view_point.y = (flt_type) flt_tmp[1];
  scene->view_point.z = (flt_type) flt_tmp[2];
  scene->view_dir.x   = (flt_type) flt_tmp[3];
  scene->view_dir.y   = (flt_type) flt_tmp[4];
  scene->view_dir.z   = (flt_type) flt_tmp[5]; // NOLINT: not a magic number
  scene->fov          = (flt_type) flt_tmp[6]; // NOLINT: not a magic number

  if (scan_n_pure_nums(pack_file, &scene->cast_depth, 1) != 1) {
    return 0;
  }

  char lexem[LEX_LEN];

  if (fscanf(pack_file, "%s", lexem) == 0) {
    return 0;
  }

  if (strcmp(lexem, "{") != 0) {
    return 0;
  }

  scene->n_objects = scan_pure_nums(pack_file, &scene->objects, lexem);

  if (strcmp(lexem, "}") != 0) {
    return 0;
  }

  if (fscanf(pack_file, "%s", lexem) == 0) {
    return 0;
  }

  if (strcmp(lexem, "{") != 0) {
    return 0;
  }

  scene->n_lights = scan_pure_nums(pack_file, &scene->lights, lexem);

  if (strcmp(lexem, "}") != 0) {
    return 0;
  }

  return 1;
}



int extract_scenes(FILE *pack_file, scene_t **scenes) {
  scene_t *local_scenes = NULL;
  int      n_scenes     = 0;

  char lexem[LEX_LEN];
  for (ever) {
    skip_comments(pack_file, lexem);

    if (is_delimiter(lexem)) {
      break;
    }

    const int index = scan_index(lexem);
    if (index != n_scenes) {
      free(local_scenes);
      return 0;
    }

    n_scenes++;
    local_scenes = realloc(local_scenes, n_scenes * sizeof(scene_t));

    if (scan_scene_line(pack_file, &local_scenes[index]) == 0) {
      return 0;
    }
  }

  *scenes = local_scenes;

  return n_scenes;
}

void free_scenes(scene_t *scenes, int n_scenes) {
  for (int i = 0; i < n_scenes; i++) {
    free(scenes[i].lights);
    free(scenes[i].objects);
  }

  free(scenes);
}



scene_pack_t *parse_scenes_file(const char *pack_filename) {
  if (pack_filename == NULL) {
    pack_filename = "scenes.rtr";
  }

  FILE *pack_file = fopen(pack_filename, "r");
  if (pack_file == NULL) {
    fprintf(stderr, "Can't open file: %s\n", pack_filename);
    return NULL;
  }

  light_t *   lights      = NULL;
  int         n_lights    = 0;
  material_t *materials   = NULL;
  int         n_materials = 0;
  object_t *  objects     = NULL;
  int         n_objects   = 0;
  scene_t *   scenes      = NULL;
  int         n_scenes    = 0;

  char lexem[LEX_LEN];

  for (ever) {
    if (skip_comments(pack_file, lexem) != 0) {
      break;
    }

    if (strcmp(lexem, "lights") == 0) {
      n_lights = extract_lights(pack_file, &lights);
    } else if (strcmp(lexem, "materials") == 0) {
      n_materials = extract_materials(pack_file, &materials);
    } else if (strcmp(lexem, "objects") == 0) {
      n_objects = extract_objects(pack_file, &objects);
    } else if (strcmp(lexem, "scenes") == 0) {
      n_scenes = extract_scenes(pack_file, &scenes);
    } else {
      free(lights);
      free(materials);
      free(objects);
      free(scenes);
      return NULL;
    }
  }

  fclose(pack_file);

  scene_pack_t *pack = malloc(sizeof(scene_pack_t));
  assert(pack);
  pack->scenes   = scenes;
  pack->n_scenes = n_scenes;

  pack->lights   = lights;
  pack->n_lights = n_lights;

  pack->materials   = materials;
  pack->n_materials = n_materials;

  pack->objects   = objects;
  pack->n_objects = n_objects;

  return pack;
}



scene_pack_t *get_scenes(const char *scenes_file) {
  if (scenes_file == NULL) {
    scenes_file = "scenes.rtr";
  }

  scene_pack_t *pack = parse_scenes_file(scenes_file);

  if (pack == NULL) {
    fprintf(stderr, "parse_scenes failed!\n");
    exit(EXIT_FAILURE);
  }

  if (pack->n_scenes < 1) {
    fprintf(stderr, "No scenes in %s!", scenes_file);
    exit(EXIT_FAILURE);
  }

  return pack;
}

void free_scene_pack(scene_pack_t *pack) {
  assert(pack);
  free_scenes(pack->scenes, pack->n_scenes);
  free(pack->objects);
  free(pack->lights);
  free(pack->materials);
  free(pack);
}
// }}}

