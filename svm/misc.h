#ifndef _MISC_H
#define _MISC_H

namespace TinySVM {
   
enum { LINEAR, POLY };
enum { DOUBLE_FEATURE, BINARY_FEATURE };
enum { SVM, SVR };

struct feature_node
{
  int index;
  double value;
};
};
#endif

