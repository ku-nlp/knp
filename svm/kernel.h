#ifndef _KERNEL_H
#define _KERNEL_H
#include "misc.h"
#include "param.h"
#include "base_example.h"
#include <math.h>

// $Id$;
namespace TinySVM {

class Kernel
{
protected:
  // constant param
  const int    l;
  const int    d;
  const int    pack_d;
  const int    kernel_type;
  const int    feature_type;
  const int    degree;
  const double param_g;
  const double param_r;
  const double param_s;

  double  (Kernel::*_getKernel)(const double) const;

  inline double Kernel::_getKernel_linear(const double _x) const
  {
    return _x;
  }

  inline double Kernel::_getKernel_poly(const double _x) const
  {
    return pow(param_s * _x + param_r, degree);
  }

  inline double dot_normal(const feature_node *x1, const feature_node *x2) const
  {
    register double sum = 0;
    while (x1->index != -1 && x2->index != -1) {
      if (x1->index == x2->index) {
	sum += (x1->value * x2->value);
	++x1; ++x2;
      } else {
	if (x1->index > x2->index) ++x2;
	else ++x1;
      }			
    }
    return sum;
  }

  inline int dot_binary(const feature_node *x1, const feature_node *x2) const
  {
    register int sum = 0;
    while (x1->index != -1 && x2->index != -1) {
      if (x1->index == x2->index) {
	sum++; 
	++x1; ++x2;
      } else {
	if (x1->index > x2->index) ++x2;
	else ++x1;
      }			
    }
    return sum;
  }

public:
  feature_node ** x;
  double         *y;

  Kernel(const BaseExample& example, const Param& param):
    l(example.l), 
    d(example.d), 
    pack_d(example.pack_d), 
    kernel_type(param.kernel_type),
    feature_type(example.feature_type),
    degree(param.degree),
    param_g(param.param_g),
    param_r(param.param_r),
    param_s(param.param_s)
  {
    // default
    switch (kernel_type) {
    case LINEAR:
      _getKernel  = &Kernel::_getKernel_linear;
      break;
    case POLY:
      _getKernel  = &Kernel::_getKernel_poly;
      break;
    default:
      fprintf(stderr,"Kernel::Kernel: Unknown kernel function\n");
      //std::exit(1);
    }
  }

  // wrapper for getKernel
  inline double getKernel(const feature_node *x1, const feature_node *x2)
  {
    return this->getKernel(dot_normal(x1, x2));
  }

  inline double getKernel(const double _x)
  {
    return (this->*_getKernel)(_x);
  }
};


};
#endif

