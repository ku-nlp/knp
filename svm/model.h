#ifndef _MODEL_H
#define _MODEL_H
#include "base_example.h"
#include "param.h"
#include "kernel.h"
#include "classifier.h"

// $Id$;

namespace TinySVM {

class Model: public BaseExample
{
 private:
  double margin;
  double vc;
  Param  param;

  Classifier  *kernel;
  // It is very ad-hoc solution, this pointer is handled as *Classifer 
  // we must cast it into *Classifer using static_cast<> operation. 

 public:
  double b;
  double sphere;
  double loss;
  int    bsv;
  int    *svindex;
  int    training_data_size;

  int read         (const char *,  const char *mode = "r", const int offset = 0);
  int write        (const char *,  const char *mode = "w", const int offset = 0);
  int writeSVindex (const char *,  const char *mode = "r", const int offset = 0);
  int clear();

  // classify
  double classify(const char *);
  double classify(const feature_node *);

  // model information
  double estimateMargin();
  double estimateSphere();
  double estimateVC();
  double estimateXA(const double rho = 2.0);

  int    getSVnum()            { return this->l; };
  int    getBSVnum()           { return this->bsv; };
  int    getTrainingDataSize() { return this->training_data_size; };
  double getLoss()             { return this->loss; };

  const int *getSVindex()  { return (const int *)svindex; };

  Model();
  Model(const Param &);
  ~Model();

  Model& operator=(Model &m);
};

};
#endif

