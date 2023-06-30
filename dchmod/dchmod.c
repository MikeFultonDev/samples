#include "stdio.h"
#include "dchmod.h"

int dchmod(Mode* mode, Dataset* dataset)
{
  if (mode->group.read || mode->group.write || mode->group.exec) {
    fprintf(stderr, "Still need to implement group perms\n");
    return -1;
  }
  if (mode->others.read || mode->others.write || mode->others.exec) {
    fprintf(stderr, "Still need to implement others perms\n");
    return -1;
  }
  if (mode->user.read < 1 || mode->user.write || mode->user.exec) {
    fprintf(stderr, "Still need to implement user write/exec perms and removing user read perms\n");
    return -1;
  }

  /*
   * For now, just support for adding READ permission for JUST a user
   */
  return 0;
}

