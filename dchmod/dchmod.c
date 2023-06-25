/*
 * dchmod: code to update dataset file 'mode' bits
 *         reduced interface from 'chmod':
 *          - supporting ug for symbolic pattern (u: user, g: group)
 *          - support + or - to add or remove access for specified symbolic pattern 
 * Notes:
 *   If 'g' is specified and more than one group is associated with the dataset, it will report an error
 *   If a non-RACF security subsystem is in use, it will report an error
 */

#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int tbd;
} Option;
typedef struct {
  int tbd;
} Mode;
typedef struct {
  int tbd;
} ReferenceDataset;
typedef struct {
  int tbd;
} Dataset;

typedef struct {
  Option* o;
  Mode* m;
  ReferenceDataset* r;
  Dataset* d;
} DatasetChangeMode;

typedef struct {
  unsigned int opt_done:1;
  unsigned int mode_done:1;
} ParameterState;

static void syntax()
{
  fprintf(stderr, "dchmod [OPTION]... MODE[,MODE]... DATASET...\n");
  fprintf(stderr, "dchmod [OPTION]... --reference=RDATASET DATASET...\n");
  exit(4);
}

static Option* options(ParameterState* ps, const char* argv[], int entry) 
{
  return NULL;
}
static int add_option(DatasetChangeMode* dcm, Option* option) 
{
  return 0;
}
static Mode* mode(ParameterState* ps, const char* argv[], int entry) 
{
  return NULL;
}
static int add_mode(DatasetChangeMode* dcm, Mode* mode) 
{
  return 0;
}
static ReferenceDataset* reference(ParameterState* ps, const char* argv[], int entry) 
{
  return NULL;
}
static int compute_modes(DatasetChangeMode* dcm, ReferenceDataset* ref) 
{
  return 0;
}
static Dataset* dataset(ParameterState* ps, const char* argv[], int entry) 
{
  return NULL;
}
static int add_dataset(DatasetChangeMode* dcm, Dataset* dataset) 
{
  return 0;
}

static int change_mode(DatasetChangeMode* dcm)
{
  return 0;
}

int main(int argc, const char* argv[])
{
  DatasetChangeMode dcm = { 0 };
  ParameterState ps = { 0 };
  Option* o;
  Mode* m;
  ReferenceDataset* r;
  Dataset* d;
  int i;

  if (argc < 3) {
    fprintf(stderr, "At least one symbolic mode and file must be specified.\n");
    syntax();
  }
  for (i=1; i<argc; ++i) {
    if (o=options(&ps, argv, i)) {
      add_option(&dcm, o);
    } else if (m=mode(&ps, argv, i)) {
      add_mode(&dcm, m);
    } else if (r=reference(&ps, argv, i)) {
      compute_modes(&dcm, r);
    } else if (d=dataset(&ps, argv, i)) {
      add_dataset(&dcm, d);
    }
  }
  
  return change_mode(&dcm);
}


  
