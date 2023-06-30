#ifndef __DCHMOD__
  #define __DCHMOD__ 1

  typedef struct {
    signed int read:2;
    signed int write:2;
    signed int exec:2;
  } ModeChange;

  typedef struct {
    ModeChange user;
    ModeChange group;
    ModeChange others;
  } Mode;

  typedef struct {
    const char* name;
  } Dataset;

  int dchmod(Mode* mode, Dataset* dataset);
  Mode* drdmod(Dataset* dataset);
#endif

