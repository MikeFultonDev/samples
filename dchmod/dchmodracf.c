
/*
 * The code currently learns about RACF information by making 'tsocmd' calls and
 * scraping the output.
 * In the future, this should be replaced by code that accesses information through RACF macros and/or SVC 132
 */

#define _POSIX_SOURCE 1
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include "dchmod.h"
#include "dchmodsaf.h"
#include "dchmodracf.h"

static char* dupword(const char* in, char* out, size_t maxsize) 
{
  size_t i=0;

  for (i=0; in[i] != ' ' && i <= maxsize; ++i) {
    out[i] = in[i];
  }
  out[i] = '\0';
  return out;
}
    
/*
 *
 * LISTDSD DATASET('FULTONM.MY.FILE') GENERIC ALL
INFORMATION FOR DATASET FULTONM.MY.FILE (G)

LEVEL  OWNER    UNIVERSAL ACCESS   WARNING   ERASE
-----  -------- ----------------   -------   -----
 00    FULTONM         NONE          NO      NO <-- owner and level of universal access

AUDITING
--------
FAILURES(READ)

NOTIFY
--------
NO USER TO BE NOTIFIED

YOUR ACCESS  CREATION GROUP  DATASET TYPE
-----------  --------------  ------------
   ALTER        CDEV           NON-VSAM          <--- my access (alter)

NO INSTALLATION DATA

              SECURITY LEVEL
------------------------------------------
NO SECURITY LEVEL

CATEGORIES
----------
NO CATEGORIES

SECLABEL
--------
NO SECLABEL

CREATION DATE  LAST REFERENCE DATE  LAST CHANGE DATE
(DAY) (YEAR)        (DAY) (YEAR)      (DAY) (YEAR)
-------------  -------------------  ----------------
 ***    **      NOT APPLICABLE FOR GENERIC PROFILE

ALTER COUNT  CONTROL COUNT  UPDATE COUNT  READ COUNT
-----------  -------------  ------------  ----------
NOT APPLICABLE FOR GENERIC PROFILE

   ID     ACCESS
--------  -------
*          READ                                <--- all IDs with RACF have READ access
CDEV       READ                                <--- CDEV (group) has READ ACCESS

   ID    ACCESS   CLASS                ENTITY NAME
-------- ------- -------- ----------------------------------------------------
NO ENTRIES IN CONDITIONAL ACCESS LIST
 */

static char* findhdr(char* buffer, char* coltitle[])
{
  return NULL;
}

static char* skipdashes(char* buffer)
{
  return NULL;
}

static char* getcol(char* buffer, size_t num)
{
  return NULL;
}
static int dupdtmod(Mode* mode, Dataset* dataset, struct SAFInfo* info)
{
  return -1;
}

static int drdmod(Mode* mode, Dataset* dataset, struct SAFInfo* info)
{
  char cmd[256];
  char* out;
  char* err;
  size_t outsize;
  size_t errsize;
  int argc=2;
  char* argv[] = { "tsocmd", NULL, NULL }; 
  int rc;
  char* gencols[] = { "LEVEL", "OWNER", "UNIVERSAL", "ACCESS", "WARNING", "ERASE" };
  char* loc;
  char* owner;
  char* uacc;

  rc = snprintf(cmd, sizeof(cmd), "LISTDSD DATASET('%s') GENERIC AUTHUSER", dataset->name);
  if (rc >= sizeof(cmd)) {
    fprintf(stderr, "Internal error. Truncation occurred\n");
    return -1;
  }
  argv[1] = cmd;

  rc = runcmd("/bin/tsocmd", argv, &out, &outsize, &err, &errsize);
  if (rc != 0) {
    fprintf(stderr, "Internal error: %s failed with rc: %d\n", cmd, rc);
    return -1;
  }

  /*
   * This is ugly - to be replaced with code that talks to RACF programmatic interface
   * (in 31-bit assembler :( )
   */

  loc = findhdr(out, gencols);
  if (!loc) {
    fprintf(stderr, "Unable to find general column header from command %s\n", cmd);
    return -1;
  }

  loc = skipdashes(loc);
  if (!loc) {
    fprintf(stderr, "Unable to skip dashed lines after general column header from command %s\n", cmd);
    return -1;
  }

  owner = getcol(loc, 2);
  if (!owner) {
    fprintf(stderr, "Unable to determine dataset owner from command %s\n", cmd);
    return -1;
  }
  uacc = getcol(loc, 3);
  if (!loc) {
    fprintf(stderr, "Unable to determine universal access from command %s\n", cmd);
    return -1;
  }

  return 0;
}
  
/*
 *  tsocmd "listuser fultonm"
 * 
listuser fultonm
USER=FULTONM  NAME=************          OWNER=*******   CREATED=******
 DEFAULT-GROUP=CDEV     PASSDATE=****** PASS-INTERVAL= ** PHRASEDATE=***   <--- default group is CDEV
 ATTRIBUTES=NONE
 REVOKE DATE=NONE   RESUME DATE=NONE
 LAST-ACCESS=***************
 CLASS AUTHORIZATIONS=****
 INSTALLATION-DATA=**************************************
 NO-MODEL-NAME
 LOGON ALLOWED   (DAYS)          (TIME)
 ---------------------------------------------
 ANYDAY                          ANYTIME
  GROUP=BASIC     AUTH=USE      CONNECT-OWNER=*******   CONNECT-DATE=****** <--- group I can use
    CONNECTS=    **  UACC=NONE     LAST-CONNECT=*****************
    CONNECT ATTRIBUTES=****
    REVOKE DATE=****   RESUME DATE=****
  GROUP=CDEV      AUTH=USE      CONNECT-OWNER=*******   CONNECT-DATE=****** <--- group I can use
    CONNECTS= *****  UACC=NONE     LAST-CONNECT=***************
    CONNECT ATTRIBUTES=NONE
    REVOKE DATE=NONE   RESUME DATE=NONE
SECURITY-LEVEL=NONE SPECIFIED
CATEGORY-AUTHORIZATION
 NONE SPECIFIED
 */

#define DEFAULT_GROUP_PREFIX     "DEFAULT-GROUP="
#define DEFAULT_GROUP_PREFIX_LEN (sizeof(DEFAULT_GROUP_PREFIX)-1)
#define GROUP_PREFIX             " GROUP="
#define GROUP_PREFIX_LEN         (sizeof(GROUP_PREFIX)-1)

static SAFInfo* rdinfo(SAFInfo* info) 
{
  char cmd[256];
  char* out;
  char* err;
  char* saveptr;
  char* loc;
  char* buff;
  RACFInfo* racfinfo;
  size_t outsize;
  size_t errsize;
  int argc=2;
  char* argv[] = { "tsocmd", NULL, NULL }; 
  int rc;
  size_t numgroups=0;
  size_t i;

  rc = snprintf(cmd, sizeof(cmd), "LISTUSER %s", info->userid);
  if (rc >= sizeof(cmd)) {
    fprintf(stderr, "Internal error. Truncation occurred\n");
    return NULL;
  }
  argv[1] = cmd;

  rc = runcmd("/bin/tsocmd", argv, &out, &outsize, &err, &errsize);
  if (rc != 0) {
    fprintf(stderr, "Unexpected return code %d from %s\n", rc, cmd);
    return NULL;
  }

  loc = strstr(out, DEFAULT_GROUP_PREFIX);
  if (!loc) {
    fprintf(stderr, "Internal Error. No default group prefix returned from %s\n", cmd);
    return NULL;
  }

  racfinfo = (RACFInfo*) (info->provider);
  dupword(&loc[DEFAULT_GROUP_PREFIX_LEN], racfinfo->default_group.name, GROUP_SIZE);

  buff = out;
  while ((loc = strstr(buff, GROUP_PREFIX))) {
    ++numgroups;
    buff=&loc[GROUP_PREFIX_LEN];
  }

  racfinfo->numgroups = numgroups;
  racfinfo->group = calloc(numgroups, sizeof(RACFGroup));
  buff = out;
  i=0;
  while ((loc = strstr(buff, GROUP_PREFIX))) {
    dupword(&loc[GROUP_PREFIX_LEN], racfinfo->group[i].name, GROUP_SIZE);
    buff=&loc[GROUP_PREFIX_LEN];
    ++i;
  }
  return info;
}

int racf_init(SAFInfo* info) {
  info->drdmod = drdmod;
  info->dupdtmod = dupdtmod;
  info->provider = calloc(sizeof(RACFInfo), 1);
  if (!info->provider) {
    return -1;
  }
  if (!rdinfo(info)) {
    return -1;
  }
  if (info->verbose) {
    RACFInfo* racfinfo = (RACFInfo*) (info->provider);
    size_t i;

    fprintf(stderr, "RACF Default Group: %s\nNumber of Groups:%u\n", 
      racfinfo->default_group.name, racfinfo->numgroups);
    for (i=0; i<racfinfo->numgroups; ++i) {
      fprintf(stderr, " %s\n", racfinfo->group[i].name);
    }
  }
  return 0;
}

int racf_term(SAFInfo* info) {
  if (info->provider) {
    RACFInfo* racfinfo = (RACFInfo*) (info->provider);
    if (racfinfo->group) {
      free(racfinfo->group);
    }
    free(info->provider);
  }
  return 0;
}
