#pragma once

#include "flt_type.h"

#include <math.h>



enum {
  X_STEPS = 100,
  T_STEPS = 100,
};



flt_type phi(flt_type x) {
  const int koeff = -200;
  return flt_exp(koeff * x * x);
}

flt_type psi(flt_type t) {
  const int koeff = -200;
  return flt_exp(koeff * t * t);
}

flt_type f(flt_type t, flt_type x) { return 1 + 0 * t * x; }



static const flt_type x_begin = 0.0;
static const flt_type x_end   = 1.0;
static const flt_type x_step  = (x_end - x_begin) / X_STEPS;

static const flt_type t_begin = 0.0;
static const flt_type t_end   = 1.0;
static const flt_type t_step  = (t_end - t_begin) / T_STEPS;

static const flt_type a = 1.0;



typedef struct {
  const flt_type x_begin;
  const flt_type x_end;
  const flt_type x_step;

  const flt_type t_begin;
  const flt_type t_end;
  const flt_type t_step;

  const flt_type a;

  flt_type (*phi)(flt_type);
  flt_type (*psi)(flt_type);
  flt_type (*f)(flt_type, flt_type);
} conditions_t;

