#define _POSIX_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "dchmod.h"
#include "dchmodsaf.h"

typedef enum {
  SAFUnk=0,
  RACF=1,
  TopSecret=2,
  ACF2=3
} SAFProvider;

static int compute_modes(Mode* mode, Dataset* reference)
{
  fprintf(stderr, "write code to compute modes\n");
  exit(4);
  return 0;
}

static SAFProvider saf_provider()
{
  return RACF;
}
  
void* dchmod_init(const char* userid, Mode* mode, Dataset* reference, int verbose)
{
  SAFInfo* info;
  SAFProvider provider;
  void* work;
  size_t i;
  int rc;

  work = calloc(sizeof(SAFInfo), 1);
  if (!work) {
    return NULL;
  }

  info = work;
  if (!userid) {
    userid = getlogin();
  }

  if (!userid) {
    return NULL;
  }

  strncpy(info->userid, userid, USER_SIZE+1);
  info->verbose = verbose;

  info->reference = reference;

  provider = saf_provider();

  switch (provider) {
    case RACF: 
      rc = racf_init(info);
      break;
    case TopSecret:
      rc = topsecret_init(info);
      break;
    case ACF2:
      rc = acf2_init(info);
      break;
    default:
      return NULL;
  }

  if (rc) {
    return NULL;
  }

  if (info->verbose) {
    fprintf(stderr, "SAF Info:\n User ID: %s\n Reference Dataset: %s\n", 
      info->userid, (reference == NULL) ? "<null>" : reference->name);
  }

  return work;
}

void dchmod_term(void* work)
{
  SAFInfo* info = (SAFInfo*) work;
  int rc;
  if (info) {
    SAFProvider provider = saf_provider();
    switch (provider) {
      case RACF: 
        rc = racf_term(info);
        break;
      case TopSecret:
        rc = topsecret_term(info);
        break;
      case ACF2:
        rc = acf2_term(info);
        break;
      default:
        fprintf(stderr, "Internal Error. Unknown provider on term\n");
        break;
    }
    free(info);
  }
}

int dchmod(Mode* mode, Dataset* dataset, void* work)
{
  SAFInfo* info = (SAFInfo*) work;
  Mode old = { 0 }; 
  Mode new = { 0 }; 
  int rc;

  if (!info) {
    return -1;
  }
  if (mode->group.read < 0 || mode->group.write || mode->group.exec) {
    fprintf(stderr, "Still need to implement group perms, except group+read\n");
    return -1;
  }
  if (mode->others.read || mode->others.write || mode->others.exec) {
    fprintf(stderr, "Still need to implement others perms\n");
    return -1;
  }
  if (mode->user.read || mode->user.write || mode->user.exec) {
    fprintf(stderr, "Still need to implement user perms\n");
    return -1;
  }

  if (info->reference) {
    if ((rc = compute_modes(mode, info->reference)) != 0) {
      return rc;
    }
  }

  if ((rc = info->drdmod(&old, dataset, info)) != 0) {
    return rc;
  }

  /*
   * Code needed here to merge changed mode and old mode
   * into new mode
   */
  if ((rc = info->dupdtmod(&new, dataset, info)) != 0) {
    return rc;
  }
  return 0;

}

