#ifndef __DCHMODCLI__
  #define __DCHMODCLI__ 1

  typedef struct {
    unsigned int changes:1;
    unsigned int quiet:1;
    unsigned int verbose:1;
    unsigned int debug:1;
    unsigned int reference:1;
    unsigned int help:1;
    unsigned int version:1;
    unsigned int no_preserve_hlq:1;
    unsigned int preserve_hlq:1;
  } Option;

  typedef struct {
    unsigned int user:1;
    unsigned int group:1;
    unsigned int others:1;
    unsigned int all:1;
    unsigned int minus:1;
    unsigned int plus:1;
    unsigned int read:1;
    unsigned int write:1;
    unsigned int exec:1;
  } ModeBits;

  typedef struct {
    Option o;
    Mode m;
    Dataset r;
    AccessID* id;
  } DatasetChangeMode;

  typedef struct {
    unsigned int opt_done:1;
    unsigned int mode_done:1;
    unsigned int ids_done:1;
    unsigned int err:1;
  } ParameterState;
#endif

