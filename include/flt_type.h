#pragma once

#include <float.h>
#include <math.h>



#if defined FLT_TYPE_FLOAT || defined FLT_TYPE_DOUBLE || defined FLT_TYPE_LONG_DOUBLE
  #define FLT_TYPE_DEFINED 1
#else
  #define FLT_TYPE_DEFINED 0
#endif



#ifdef FLT_TYPE_FLOAT

typedef float flt_type;
#define FLT_PRINT_FORMAT "f"
#define FLT_ZERO 0.0f
#define FLT_ONE  1.0f
#define FLT_TYPE_EPSILON FLT_EPSILON
#define FLT_TYPE_DIG     FLT_DIG
#define FLT_TYPE_MIN     FLT_MIN      
#define FLT_TYPE_MAX     FLT_MAX 
#define flt_abs   fabsf
#define flt_exp   expf
#define flt_log   logf
#define flt_log2  log2f
#define flt_log10 log10f
#define flt_pow   powf
#define flt_sin   sinf
#define flt_cos   cosf
#define flt_sqrt  sqrtf

#elif FLT_TYPE_DOUBLE

typedef double flt_type;
#define FLT_PRINT_FORMAT "lf"
#define FLT_ZERO 0.0
#define FLT_ONE  1.0
#define FLT_TYPE_EPSILON DBL_EPSILON
#define FLT_TYPE_DIG     DBL_DIG
#define FLT_TYPE_MIN     DBL_MIN      
#define FLT_TYPE_MAX     DBL_MAX 
#define flt_abs   fabs
#define flt_exp   exp
#define flt_log   log
#define flt_log2  log2
#define flt_log10 log10
#define flt_pow   pow
#define flt_sin   sin
#define flt_cos   cos
#define flt_sqrt  sqrt

#elif FLT_TYPE_LONG_DOUBLE

typedef long double flt_type;
#define FLT_PRINT_FORMAT "Lf"
#define FLT_ZERO 0.0l
#define FLT_ONE  1.0l
#define FLT_TYPE_EPSILON LDBL_EPSILON
#define FLT_TYPE_DIG     LDBL_DIG
#define FLT_TYPE_MIN     LDBL_MIN      
#define FLT_TYPE_MAX     LDBL_MAX 
#define flt_abs   fabsl
#define flt_exp   expl
#define flt_log   logl
#define flt_log2  log2l
#define flt_log10 log10l
#define flt_pow   powl
#define flt_sin   sinl
#define flt_cos   cosl
#define flt_sqrt  sqrtl

#endif

