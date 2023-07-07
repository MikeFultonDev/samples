#include "accessid.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
  /*
   * Verify invalid userids are caught
   * https://www.ibm.com/docs/en/zos/2.5.0?topic=users-user-naming-conventions
   */
  AccessIDError err;
  int rc=0;
  int count;
  const char* end;

  if ((err = accessid_check("9fred",'\0', NULL)) != AccessIDOK) {
    fprintf(stderr, "Test 1 failed\n");
    rc=1;
  }
  if ((err = accessid_check(".fred",',', &end)) != AccessIDInvChar) {
    fprintf(stderr, "Test 2 failed\n");
    rc=2;
  }
  if ((err = accessid_check("joe*jill",',', &end)) != AccessIDInvChar) {
    fprintf(stderr, "Test 3 failed\n");
    rc=3;
  }
  if ((err = accessid_check("joejacobjingleheimerschmidt",',',&end)) != AccessIDTooLong) {
    fprintf(stderr, "Test 4 failed\n");
    rc=4;
  }
  if ((err = accessid_check("",',',&end)) != AccessIDTooShort) {
    fprintf(stderr, "Test 4.5 failed\n");
    rc=4;
  }
  count=0;
  if ((err = accessid_check("A2345678^B2345678^C2345678^D2345678^E234567",'^',&end)) != AccessIDOK) {
    fprintf(stderr, "Test 5.%d failed. Err:%d\n", count, err);
    rc=5;
  }
  while (*end) {
    const char* start=end+1;
    ++count;
    err = accessid_check(start, '^', &end);
    if (err != AccessIDOK) {
      fprintf(stderr, "Test 5.%d failed. Err:%d\n", count, err);
      rc=5;
      break;
    }
  }

  return rc;
}
   
