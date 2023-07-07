#ifndef __DATASET__
  #define __DATASET__ 1

  typedef enum {
    DatasetOK=0,
    DatasetInvCharStart,
    DatasetInvDotStart,
    DatasetInvSuccessive,
    DatasetInvDotEnd,
    DatasetInvMinQual,
    DatasetNameTooLong,
    DatasetQualifierTooLong,
    DatasetInvWildcard
  } DatasetError;

  void pdataseterror(DatasetError err);
  DatasetError check_dataset(const char* dataset, int haswildcard);
  char* normalize_dataset(const char* in, char* out);
#endif
