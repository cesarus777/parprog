#pragma once

#include <SDL2/SDL_error.h>

#include <stdio.h>



#ifdef EXIT_ON_FAIL
#define SDL_ERROR_EXIT_ON_FAIL 1
#else
#define SDL_ERROR_EXIT_ON_FAIL 0
#endif



#define SDL_TRY(sdl_ret_val) \
    if(sdl_ret_val != 0) { \
      fprintf(stderr, \
              "%s:%d '" #sdl_ret_val "' fail: %s\n", \
              __FILE__, \
              __LINE__, \
              SDL_GetError()); \
      if(SDL_ERROR_EXIT_ON_FAIL) { \
        SDL_Quit(); \
        exit(EXIT_FAILURE); \
      } \
    }



#define SDL_NOT_NULL(sdl_ret_val) \
    if(sdl_ret_val == NULL) { \
      fprintf(stderr, \
              "%s:%d '" #sdl_ret_val "' is NULL, possible error: %s\n", \
              __FILE__, \
              __LINE__, \
              SDL_GetError()); \
      if(SDL_ERROR_EXIT_ON_FAIL) { \
        SDL_Quit(); \
        exit(EXIT_FAILURE); \
      } \
    }

