#ifndef __DCHMOD_TS__
  #define __DCHMOD_TS__ 1

  typedef struct {
    int tbd;
  } TSInfo;

  int topsecret_init(SAFInfo* info);
  int topsecret_term(SAFInfo* info);
#endif
