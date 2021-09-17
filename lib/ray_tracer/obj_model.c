#include "geometry.h"
#include "obj_model.h"

#include <stdio.h>

#define ever ;;

enum {
  VERTICES_ALLOC_SIZE = 1000,
  TRIANGLES_ALLOC_SIZE = 1000,
};

ssize_t skip_line(FILE *file) {
  char *tmp = NULL;
  size_t tmp_sz = 0;
  ssize_t ret = getline(&tmp, &tmp_sz, file);
  free(tmp);
  return ret;
}

void free_model(model_t *model) {
  if(model != NULL) {
    free(model->triangles);
    model->n_triangles = 0;
    free(model);
  }
}

model_t *extract_obj_model_from_file(const char *filename, const vec3f shift, const flt_type scale) {
  model_t *model = malloc(sizeof(model_t));
  if(model == NULL)
    return NULL;

  FILE *file = fopen(filename, "r");
  if(file == NULL) {
    fprintf(stderr, "Can't open file: %s\n", filename);
    free_model(model);
    return NULL;
  }

  int is_vt_warning_shown = 0;
  int is_vn_warning_shown = 0;
  int is_vp_warning_shown = 0;
  int is_l_warning_shown = 0;
  int is_g_warning_shown = 0;
  int is_s_warning_shown = 0;
  int is_o_warning_shown = 0;
  int is_mtllib_warning_shown = 0;
  int is_usemtl_warning_shown = 0;

  int n_vertices = 0;
  vec3f *vertices = malloc(VERTICES_ALLOC_SIZE * sizeof(vec3f));
  if(vertices == NULL) {
    free_model(model);
    return NULL;
  }

  model->n_triangles = 0;
  model->triangles = malloc(TRIANGLES_ALLOC_SIZE * sizeof(triangle_t));
  if(model->triangles == NULL) {
    free_model(model);
    return NULL;
  }

  model->min_x =  FLT_TYPE_MAX;
  model->min_y =  FLT_TYPE_MAX;
  model->min_z =  FLT_TYPE_MAX;
  model->max_x = -FLT_TYPE_MAX;
  model->max_y = -FLT_TYPE_MAX;
  model->max_z = -FLT_TYPE_MAX;

  flt_type shift_arr[3] = { shift.x, shift.y, shift.z };

  for(ever) {
    char type[3];
    if(fscanf(file, "%s", type) != 1)
      break;

    if(strcmp(type, "#") == 0) {
      if(skip_line(file) < 0) {
        free_model(model);
        return NULL;
      }
      continue;
    } else if(strcmp(type, "v") == 0) {
      double tmp[3];
      for(int i = 0; i < 3; ++i) {
        if(fscanf(file, "%lf", &tmp[i]) != 1) {
          free_model(model);
          return NULL;
        }
        tmp[i] *= scale;
        tmp[i] += shift_arr[i];
      }
      vec3f tmp_vec = { tmp[0], tmp[1], tmp[2], };
      vertices[n_vertices] = tmp_vec;
      n_vertices++;
      if((n_vertices % VERTICES_ALLOC_SIZE) == 0) {
        vertices = realloc(vertices, (n_vertices + VERTICES_ALLOC_SIZE) * sizeof(vec3f));
        if(vertices == NULL) {
          free_model(model);
          return NULL;
        }
      }

      if(tmp_vec.x < model->min_x)
        model->min_x = tmp_vec.x;
      if(tmp_vec.x > model->max_x)
        model->max_x = tmp_vec.x;

      if(tmp_vec.y < model->min_y)
        model->min_y = tmp_vec.y;
      if(tmp_vec.y > model->max_y)
        model->max_y = tmp_vec.y;

      if(tmp_vec.z < model->min_z)
        model->min_z = tmp_vec.z;
      if(tmp_vec.z > model->max_z)
        model->max_z = tmp_vec.z;

      continue;
    } else if(strcmp(type, "vt") == 0) {
      if(is_vt_warning_shown == 0) {
        printf("vt doesn't supported and will be ignored further\n");
        is_vt_warning_shown = 1;
      }
      if(skip_line(file) < 0) {
        free_model(model);
        return NULL;
      }
      continue;
    } else if(strcmp(type, "vn") == 0) {
      if(is_vn_warning_shown == 0) {
        printf("vn doesn't supported and will be ignored further\n");
        is_vn_warning_shown = 1;
      }
      if(skip_line(file) < 0) {
        free_model(model);
        return NULL;
      }
      continue;
    } else if(strcmp(type, "vp") == 0) {
      if(is_vp_warning_shown == 0) {
        printf("vp doesn't supported and will be ignored further\n");
        is_vp_warning_shown = 1;
      }
      if(skip_line(file) < 0) {
        free_model(model);
        return NULL;
      }
      continue;
    } else if(strcmp(type, "f") == 0) {
      int tmp[3];
      for(int i = 0; i < 3; ++i) {
        char tmp_str[32];
        if(fscanf(file, "%s", tmp_str) != 1) {
          free_model(model);
          return NULL;
        }

        char *endptr;
        tmp[i] = strtol(tmp_str, &endptr, 10);
        if((tmp[i] < 0) || !((*endptr == '\0') || (*endptr == '/'))) {
          free_model(model);
          return NULL;
        }
        tmp[i]--;
      }

      //vec3i tmp_vec = { tmp[0] - 1, tmp[1] - 1, tmp[2] - 1, };
      triangle_t tmp_triangle = { vertices[tmp[0]], vertices[tmp[1]], vertices[tmp[2]], };
      model->triangles[model->n_triangles] = tmp_triangle;
      model->n_triangles++;
      if((model->n_triangles % TRIANGLES_ALLOC_SIZE) == 0) {
        model->triangles = realloc(model->triangles, (model->n_triangles + TRIANGLES_ALLOC_SIZE) * sizeof(triangle_t));
        if(model->triangles == NULL) {
          free_model(model);
          return NULL;
        }
      }
      continue;
    } else if(strcmp(type, "l") == 0) {
      if(is_l_warning_shown == 0) {
        printf("l doesn't supported and will be ignored further\n");
        is_l_warning_shown = 1;
      }
      if(skip_line(file) < 0) {
        free_model(model);
        return NULL;
      }
      continue;
    } else if(strcmp(type, "g") == 0) {
      if(is_g_warning_shown == 0) {
        printf("g doesn't supported and will be ignored further\n");
        is_g_warning_shown = 1;
      }
      if(skip_line(file) < 0) {
        free_model(model);
        return NULL;
      }
      continue;
    } else if(strcmp(type, "s") == 0) {
      if(is_s_warning_shown == 0) {
        printf("s doesn't supported and will be ignored further\n");
        is_s_warning_shown = 1;
      }
      if(skip_line(file) < 0) {
        free_model(model);
        return NULL;
      }
      continue;
    } else if(strcmp(type, "o") == 0) {
      if(is_o_warning_shown == 0) {
        printf("o doesn't supported and will be ignored further\n");
        is_o_warning_shown = 1;
      }
      if(skip_line(file) < 0) {
        free_model(model);
        return NULL;
      }
      continue;
    } else if(strcmp(type, "mtllib") == 0) {
      if(is_mtllib_warning_shown == 0) {
        printf("mtllib doesn't supported and will be ignored further\n");
        is_mtllib_warning_shown = 1;
      }
      if(skip_line(file) < 0) {
        free_model(model);
        return NULL;
      }
      continue;
    } else if(strcmp(type, "usemtl") == 0) {
      if(is_usemtl_warning_shown == 0) {
        printf("usemtl doesn't supported and will be ignored further\n");
        is_usemtl_warning_shown = 1;
      }
      if(skip_line(file) < 0) {
        free_model(model);
        return NULL;
      }
      continue;
    } else {
      fprintf(stderr, "Unknown data type '%s' in %s\n", type, filename);
      free_model(model);
      return NULL;
    }
  }

  free(vertices);

  if(model->triangles == NULL) {
    free_model(model);
    return NULL;
  }

  return model;
}

