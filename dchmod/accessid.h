#ifndef __ACCESSID__
  #define __ACCESSID__ 1

  typedef enum {
    AccessIDOK=0,
    AccessIDInvChar,
    AccessIDTooShort,
    AccessIDTooLong
  } AccessIDError;

  void paccessiderror(AccessIDError err);
  AccessIDError accessid_check(const char* dataset, int altterminattor, const char** end);
  char* normalize_accessid(const char* in, char* out);
#endif
