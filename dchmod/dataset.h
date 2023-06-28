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
  } DatasetError;

  void pdataseterror(DatasetError err);
  DatasetError check_dataset(const char* dataset);
#endif
