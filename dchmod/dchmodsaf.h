#ifndef __DCHMOD_SAF__
  #define __DCHMOD_SAF__ 1

  #define USER_SIZE 8

  struct SAFInfo;
  typedef struct SAFInfo {
    char userid[USER_SIZE+1];
    Dataset* reference;
    unsigned int verbose:1;
    void* provider;
    int (*drdmod)(Mode* mode, Dataset* dataset, struct SAFInfo* info);
    int (*dupdtmod)(Mode* mode, Dataset* dataset, struct SAFInfo* info);
  } SAFInfo;
#endif
