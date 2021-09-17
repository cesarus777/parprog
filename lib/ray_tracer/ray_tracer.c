#include "colors.h"
#include "geometry.h"
#include "ray_casting.h"
#include "scene.h"

#include "flt_type.h"

#define EXIT_ON_FAIL
#include "sdl_error.h"

#ifdef DRAW_PARALLEL
  #include "mpi_error.h"
  #include <mpi/mpi.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bsd/string.h>



#define USAGE                                                                  \
  "Usage: ray_tracer <file with scenes> -o <template of output files>\n"       \
  "    output template filename should include '#' char which will be "        \
  "replaced with number of drawn scene\n"



typedef enum {
  HELP,
  WINDOW,
  OUTPUT_TEMPLATE,
  SCENES_FILE,
  UNKNOWN,
} arg_type_t;

enum {
  WIN_WIDTH  = 1024,
  WIN_HEIGHT = 768,
};

enum {
  MAX_DEPTH = 7,
};

enum {
  SURFACE_DEPTH = 32,
  WINDOW_TIME   = 5000,
};

#ifdef DRAW_PARALLEL
enum {
  SIZE_TAG,
  BUF_TAG,
};

enum {
  ROOT_RANK = 0,
};
#endif



typedef struct {
  int create_window;

  const char *scenes_file;
  const char *output_template;

  SDL_Window *  window;
  SDL_Renderer *renderer;
} context_t;



void       show_help();
context_t *process_args(const char *argv[], int argc);
void       close_window(const context_t *ctx);



void clear_screen(SDL_Renderer *renderer) {
  SDL_TRY(
      SDL_SetRenderDrawColor(renderer, BG_CLR.r, BG_CLR.g, BG_CLR.b, BG_CLR.a));
  SDL_TRY(SDL_RenderClear(renderer));
}



void set_pixel32_on_surface(SDL_Surface *surface, int x, int y, SDL_Color clr) {
  assert(x >= 0);
  assert(y >= 0);

  if (surface->format->BytesPerPixel != 4) {
    fprintf(stderr, "Not a 32-bit surface.\n");
    exit(EXIT_FAILURE);
  }

  uint8_t *target_pixel = (uint8_t *) surface->pixels + y * surface->pitch +
                          x * surface->format->BytesPerPixel;
  *((uint32_t *) target_pixel) =
      SDL_MapRGBA(surface->format, clr.r, clr.g, clr.b, clr.a);
}



void set_pixel32(uint32_t *buf, int i, SDL_PixelFormat *format, SDL_Color clr) {
  assert(buf != NULL);
  assert(format != NULL);
  assert(i >= 0);
  buf[i] = SDL_MapRGBA(format, clr.r, clr.g, clr.b, clr.a);
}



SDL_Color calculate_pixel(const scene_pack_t *pack, int scene_idx, int i,
                          int j) {
  assert(pack);
  assert(pack->n_scenes > scene_idx);

  const scene_t *scene = &pack->scenes[scene_idx];

  flt_type x = i - scene->width / (flt_type) 2;
  flt_type y = -j + scene->height / (flt_type) 2;
  flt_type z = -scene->height / ((flt_type) 2 * tan(scene->fov / (flt_type) 2));

  vec3f dir = {x, y, z};
  dir       = vec3f_normalize(dir);
  dir       = vec3f_add(dir, scene->view_dir);
  dir       = vec3f_normalize(dir);

  return cast_ray(pack, scene_idx, scene->view_point, dir, scene->cast_depth);
}



#ifndef DRAW_PARALLEL
SDL_Surface *draw_scene_on_surface(const scene_pack_t *pack, int scene_idx) {
  assert(pack);
  assert(pack->n_scenes > scene_idx);

  const scene_t *scene   = &pack->scenes[scene_idx];
  SDL_Surface *  surface = SDL_CreateRGBSurface(0, scene->width, scene->height,
                                              SURFACE_DEPTH, 0, 0, 0, 0);
  SDL_NOT_NULL(surface);

  for (int i = 0; i < surface->w; ++i) {
    for (int j = 0; j < surface->h; ++j) {
      SDL_Color pixel = calculate_pixel(pack, scene_idx, i, j);
      set_pixel32_on_surface(surface, i, j, pixel);
    }
  }

  return surface;
}

#else

void draw_part_of_scene(const scene_pack_t *pack, int scene_idx,
                        SDL_PixelFormat *format, uint32_t **pix_buf,
                        int *pix_buf_size) {
  assert(pack);
  assert(format);
  assert(pix_buf);
  assert(pix_buf_size);

  const scene_t *scene = &pack->scenes[scene_idx];

  int size = -1;
  int rank = -1;
  TRY_MPI(MPI_Comm_size(MPI_COMM_WORLD, &size));
  assert(size < 0);
  TRY_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
  assert(rank < 0);

  int n_pixels = scene->width * scene->height / size + 1;
  int shift    = n_pixels * rank;

  if (rank == (size - 1)) {
    n_pixels -= n_pixels * size - scene->width * scene->height;
  }

  if (*pix_buf == NULL) {
    assert((rank != ROOT_RANK) && "pix_buf must be allocated in the root rank");
    *pix_buf = malloc(n_pixels * sizeof(uint32_t));
  }
  assert(*pix_buf);
  *pix_buf_size = n_pixels;

  for (int i = 0; i < n_pixels; ++i) {
    SDL_Color pixel =
        calculate_pixel(pack, scene_idx, (shift + i) % scene->width,
                        (shift + i) / scene->width);
    (*pix_buf)[i] = SDL_MapRGBA(format, pixel.r, pixel.g, pixel.b, pixel.a);
  }
}



void draw_and_send_part_of_scene(const scene_pack_t *pack, int scene_idx) {
  assert(pack);
  assert(pack->n_scenes > scene_idx);

  uint32_t *pix_buf      = NULL;
  int       pix_buf_size = 0;

  // TODO: move format to 'draw_part_of_scene'
  SDL_PixelFormat *format = malloc(sizeof(SDL_PixelFormat));
  assert(format);
  TRY_MPI(MPI_Bcast(format, sizeof(SDL_PixelFormat), MPI_BYTE, ROOT_RANK,
                    MPI_COMM_WORLD));

  draw_part_of_scene(pack, scene_idx, format, &pix_buf, &pix_buf_size);
  free(format);
  assert(pix_buf && (pix_buf_size > 0));

  TRY_MPI(
      MPI_Send(&pix_buf_size, 1, MPI_INT, ROOT_RANK, SIZE_TAG, MPI_COMM_WORLD));
  TRY_MPI(MPI_Send(pix_buf, pix_buf_size, MPI_UINT32_T, ROOT_RANK, BUF_TAG,
                   MPI_COMM_WORLD));
  free(pix_buf);
}



void gather_drawn_pixels(uint32_t *pixels, int drawn_pixels, int free_space,
                         int size) {
  assert(pixels);
  assert(drawn_pixels >= 0);
  assert(free_space >= 0);
  assert(size > 0);

  for (int i = 1; i < size; i++) {
    pixels += drawn_pixels;
    TRY_MPI(MPI_Recv(&drawn_pixels, 1, MPI_INT, i, SIZE_TAG, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE));
    assert(drawn_pixels > 0);
    if (drawn_pixels > free_space) {
      fprintf(stderr, "drawn more pixels than there is free space, panic");
      abort();
    }
    TRY_MPI(MPI_Recv(pixels, drawn_pixels, MPI_UINT32_T, i, BUF_TAG,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE));
    free_space -= drawn_pixels;
  }
}



SDL_Surface *draw_scene_on_surface_parallel(const scene_pack_t *pack,
                                            int                 scene_idx) {
  TRY_MPI(MPI_Init(NULL, NULL));

  int size = -1;
  int rank = -1;
  TRY_MPI(MPI_Comm_size(MPI_COMM_WORLD, &size));
  assert((size < 0) && (size >= ROOT_RANK));
  TRY_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
  assert(rank < 0);


  if (rank != ROOT_RANK) {
    draw_and_send_part_of_scene(pack, scene_idx);
    TRY_MPI(MPI_Finalize());
    exit(EXIT_SUCCESS);
  }

  const scene_t *scene   = &pack->scenes[scene_idx];
  SDL_Surface *  surface = SDL_CreateRGBSurface(0, scene->width, scene->height,
                                              SURFACE_DEPTH, 0, 0, 0, 0);
  SDL_NOT_NULL(surface);

  TRY_MPI(MPI_Bcast(surface->format, sizeof(SDL_PixelFormat), MPI_BYTE, 0,
                    MPI_COMM_WORLD));

  uint32_t *pixels       = (uint32_t *) surface->pixels;
  int       free_space   = surface->w * surface->h;
  int       drawn_pixels = -1;
  draw_part_of_scene(pack, scene_idx, surface->format, &pixels, &drawn_pixels);
  assert(drawn_pixels >= 0);
  free_space -= drawn_pixels;
  assert(free_space >= 0);

  gather_drawn_pixels(pixels, drawn_pixels, free_space, size);

  TRY_MPI(MPI_Finalize());

  return surface;
}
#endif



void copy_surface_to_renderer(SDL_Surface *surface, SDL_Renderer *renderer) {
  SDL_Texture *frame = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_NOT_NULL(frame);

  SDL_TRY(SDL_RenderCopy(renderer, frame, NULL, NULL));
  SDL_RenderPresent(renderer);
  SDL_DestroyTexture(frame);
}



SDL_Surface *draw(const context_t *ctx, const scene_pack_t *pack,
                  int scene_idx) {
  SDL_Surface *surface =
#ifdef DRAW_PARALLEL
      draw_scene_on_surface_parallel(pack, scene_idx);
#else
      draw_scene_on_surface(pack, scene_idx);
#endif
  SDL_NOT_NULL(surface);

  if (ctx->create_window) {
    copy_surface_to_renderer(surface, ctx->renderer);
  }

  return surface;
}



char *get_output_filename_from_template(const char *template, int n) {
  assert((n >= 0) && (n < 9));
  int len = strlen(template) + 1;
  char *output_filename = malloc(len * sizeof(char));
  int cpylen = strlcpy(output_filename, template, len);
  if (cpylen != (len - 1)) {
    free(output_filename);
    return NULL;
  }

  char *num_pos = strchr(output_filename, '#');
  *num_pos      = '0' + n;

  return output_filename;
}



int main(int argc, const char *argv[]) {
  context_t *ctx = process_args(argv, argc);
  assert(ctx->scenes_file);
  assert(ctx->output_template);

  scene_pack_t *pack = get_scenes(ctx->scenes_file);
  assert(pack);

  char *output_filename =
      get_output_filename_from_template(ctx->output_template, 0);
  assert(output_filename != NULL);

  SDL_Surface *surface = draw(ctx, pack, 0);
  free_scene_pack(pack);

  SDL_TRY(IMG_SavePNG(surface, output_filename));
  SDL_FreeSurface(surface);
  free(output_filename);

  if (ctx->create_window) {
    SDL_Delay(WINDOW_TIME);
    close_window(ctx);
  }

  free(ctx);
}



void show_help() {
#ifdef DRAW_PARALLEL
  int rank        = -1;
  int initialized = 0;
  TRY_MPI(MPI_Initialized(&initialized));
  if (initialized == 0) {
    TRY_MPI(MPI_Init(NULL, NULL));
  }

  TRY_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
  TRY_MPI(MPI_Finalize());

  if (rank != ROOT_RANK) {
    exit(EXIT_SUCCESS);
  }
#endif

  printf(USAGE);
  exit(EXIT_SUCCESS);
}



int is_valid_template(const char *output_template) {
  char *pos = strchr(output_template, '#');
  return (pos != NULL) && (strchr(pos + 1, '#') == NULL);
}



void create_window(context_t *ctx) {
  ctx->create_window = 1;

  SDL_TRY(SDL_CreateWindowAndRenderer(WIN_WIDTH, WIN_HEIGHT,
                                      SDL_WINDOW_BORDERLESS, &ctx->window,
                                      &ctx->renderer));
  SDL_NOT_NULL(ctx->window);
  SDL_NOT_NULL(ctx->renderer);

  clear_screen(ctx->renderer);
  SDL_RenderPresent(ctx->renderer);
}



arg_type_t *classificate_args(const char *argv[], const int argc) {
  assert(argv);
  assert(argc > 0);

  if (argc == 1) {
    return NULL;
  }

  arg_type_t *types = malloc(sizeof(arg_type_t) * (argc - 1));

  for (int i = 1; i < argc; i++) {
    if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
      types[i - 1] = HELP;
      continue;
    }
    if ((strcmp(argv[i], "-w") == 0) || (strcmp(argv[i], "--window") == 0)) {
      types[i - 1] = WINDOW;
      continue;
    }
    if ((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "--output") == 0)) {
      types[i - 1] = OUTPUT_TEMPLATE;
      continue;
    }
    if ((strncmp(argv[i], "-", 1) == 0) || (strncmp(argv[i], "--", 2) == 0)) {
      types[i - 1] = UNKNOWN;
      continue;
    }
    types[i - 1] = SCENES_FILE;
  }

  return types;
}



context_t *process_args(const char *argv[], const int argc) {
  assert(argv);
  assert(argc > 0);

  context_t *     ctx      = malloc(sizeof(context_t));
  const context_t ctx_init = {0, "scenes.rtr", "output#.png", NULL, NULL};
  *ctx                     = ctx_init;
  arg_type_t *types        = classificate_args(argv, argc);
  if (argc > 1) {
    assert(types);
  }

  for (int i = 0; i < argc - 1; i++) {
    switch (types[i]) {
    case HELP:
      show_help();
      break;
    case WINDOW:
      create_window(ctx);
      break;
    case OUTPUT_TEMPLATE:
      if ((i + 2) == argc) {
        fprintf(stderr,
                "%s: no output_template was porovided after '%s'. Default "
                "template is used: '%s'\n",
                argv[0], argv[i + 1], ctx->output_template);
      } else {
        ctx->output_template = argv[i + 2];
        i++;
      }
      break;
    case SCENES_FILE:
      ctx->scenes_file = argv[i + 1];
      break;
    case UNKNOWN:
      printf("%s: error: unknown option: '%s'\n", argv[0], argv[i]);
      show_help();
      break;
    default:
      printf("%s: error: unknown option: '%s'\n", argv[0], argv[i]);
      show_help();
      break;
    }
  }

  free(types);

  if (!is_valid_template(ctx->output_template)) {
    fprintf(stderr,
            "%s: output_template must include one and only one '#' character\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  return ctx;
}

void close_window(const context_t *ctx) {
  assert(ctx);

  SDL_DestroyRenderer(ctx->renderer);
  SDL_DestroyWindow(ctx->window);
  SDL_Quit();
}

