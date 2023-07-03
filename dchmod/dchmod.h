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
    char name[45];
  } Dataset;

  void* dchmod_init(const char* id, Mode* mode, Dataset* reference);
  int dchmod(Mode* mode, Dataset* dataset, void* work);
  void dchmod_term(void* work);
#endif

