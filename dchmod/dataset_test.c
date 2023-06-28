#include "dataset.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
  /*
   * Verify invalid dataset names are caught
   * https://www.ibm.com/docs/en/zos/2.5.0?topic=sets-data-set-names
   */
  DatasetError err;
  int rc=0;

  if ((err = check_dataset("9fred")) != DatasetInvCharStart) {
    fprintf(stderr, "Test 1 failed\n");
    rc=1;
  }
  if ((err = check_dataset(".fred")) != DatasetInvDotStart) {
    fprintf(stderr, "Test 2 failed\n");
    rc=2;
  }
  if ((err = check_dataset("joe..fred")) != DatasetInvSuccessive) {
    fprintf(stderr, "Test 3 failed\n");
    rc=3;
  }
  if ((err = check_dataset("joe")) != DatasetInvMinQual) {
    fprintf(stderr, "Test 4 failed\n");
    rc=4;
  }
  if ((err = check_dataset("A2345678.B2345678.C2345678.D2345678.E234567.F")) != DatasetNameTooLong) {
    fprintf(stderr, "Test 5 failed. Err:%d\n", err);
    rc=5;
  }
  if ((err = check_dataset("A2345678.B2345678.C23456789")) != DatasetQualifierTooLong) {
    fprintf(stderr, "Test 6 failed %d\n", err);
    rc=6;
  }

  return rc;
}
   
