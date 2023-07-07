#include "dataset.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
  /*
   * Verify invalid dataset names are caught
   * https://www.ibm.com/docs/en/zos/2.5.0?topic=sets-data-set-names
   */
  DatasetError err;
  int rc=0;

  if ((err = check_dataset("9fred",1)) != DatasetInvCharStart) {
    fprintf(stderr, "Test 1 failed\n");
    rc=1;
  }
  if ((err = check_dataset(".fred",1)) != DatasetInvDotStart) {
    fprintf(stderr, "Test 2 failed\n");
    rc=2;
  }
  if ((err = check_dataset("joe..fred",1)) != DatasetInvSuccessive) {
    fprintf(stderr, "Test 3 failed\n");
    rc=3;
  }
  if ((err = check_dataset("joe",1)) != DatasetInvMinQual) {
    fprintf(stderr, "Test 4 failed\n");
    rc=4;
  }
  if ((err = check_dataset("A2345678.B2345678.C2345678.D2345678.E234567.F",1)) != DatasetNameTooLong) {
    fprintf(stderr, "Test 5 failed. Err:%d\n", err);
    rc=5;
  }
  if ((err = check_dataset("A2345678.B2345678.C23456789",1)) != DatasetQualifierTooLong) {
    fprintf(stderr, "Test 6 failed %d\n", err);
    rc=6;
  }
  if ((err = check_dataset("A2345678.B2345678.C2345678",0)) != DatasetOK) {
    fprintf(stderr, "Test 7 failed %d\n", err);
    rc=7;
  }
  if ((err = check_dataset("A2345678.B2345678.C2345678.*",0)) != DatasetInvWildcard) {
    fprintf(stderr, "Test 8 failed %d\n", err);
    rc=8;
  }
  if ((err = check_dataset("A2345678.B2345678.C2345678.*.MORE",1)) != DatasetInvWildcard) {
    fprintf(stderr, "Test 9 failed %d\n", err);
    rc=9;
  }
  if ((err = check_dataset("*",1)) != DatasetInvWildcard) {
    fprintf(stderr, "Test 10 failed %d\n", err);
    rc=10;
  }
  if ((err = check_dataset("FULTON.*",1)) != DatasetOK) {
    fprintf(stderr, "Test 11 failed %d\n", err);
    rc=11;
  }

  return rc;
}
   
