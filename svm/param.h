#ifndef _PARAM_H
#define _PARAM_H

namespace TinySVM {

class Param
{
public:
  int    kernel_type;
  int    feature_type;
  int    solver_type;
  int    degree;
  double param_g;
  double param_r;
  double param_s;
  double cache_size; 
  double C;
  double eps;
  int    verbose;
  int    shrink_size;
  double shrink_eps;
  int    final_check;
  double insensitive_loss;

  int set (int,  char **);
  int set (const char *);

  Param();
};


};
#endif

