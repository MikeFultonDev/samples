#ifndef __DCHMOD_ACF2__
  #define __DCHMOD_ACF2__ 1

  typedef struct {
    int tbd;
  } ACF2Info;

  int acf2_init(SAFInfo* info);
  int acf2_term(SAFInfo* info);
#endif
