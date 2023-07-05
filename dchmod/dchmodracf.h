#ifndef __DCHMOD_RACF__
  #define __DCHMOD_RACF__ 1

  #define GROUP_SIZE 8

  typedef struct {
    char name[GROUP_SIZE+1];
  } RACFGroup;

  typedef struct {
    RACFGroup default_group;
    size_t numgroups;
    RACFGroup* group;
  } RACFInfo;

  int racf_init(SAFInfo* info);
  int racf_term(SAFInfo* info);
#endif
