
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

#define USER_SIZE 8
#define GROUP_SIZE 8

typedef struct {
  char name[GROUP_SIZE+1];
} SAFGroup;

typedef struct {
  char userid[USER_SIZE+1];
  SAFGroup default_group;
  unsigned int verbose:1;
  size_t numgroups;
  SAFGroup* group; 
} SAFInfo;

static char* dupword(const char* in, char* out, size_t maxsize) 
{
  size_t i=0;

  for (i=0; in[i] != ' ' && i <= maxsize; ++i) {
    out[i] = in[i];
  }
  out[i] = '\0';
  return out;
}
    
static int convert_memfile_to_array(FILE* fp, char** arr, size_t* size)
{
  int rc;

  *size = ftell(fp); 
  if (*size == -1) {
    return -1;
  }
  rc = fseek(fp, 0L, SEEK_SET);
  if (rc) {
    return -1;
  }

  *arr = malloc((*size) + 1);
  if (!(*arr)) {
    return -1;
  }
  rc = fread(*arr, 1, *size, fp);
  if (rc != *size) {
    return -1;
  }
  (*arr)[*size] = '\0';
  fclose(fp);

  return 0;
}

static size_t read_stream_into_memfile(int strfd, FILE* memfile, char* buffer, size_t buffsz)
{
  ssize_t streambytes;
  size_t memfilebytes;

  streambytes = read(strfd, buffer, buffsz);
  if (streambytes == -1) {
    if (errno != EINTR) { 
      perror("read-stream");
      return -1;
    }
  } else {
    memfilebytes = fwrite(buffer, 1, (size_t) streambytes, memfile); 
    if (memfilebytes != streambytes) {
      perror("write-memfile");
      return -1;
    }
  }
  return memfilebytes;
}

static int runcmd(const char* cmd, char* argv[], char** out, size_t* outsize, char** err, size_t* errsize)
{
  char buffer[10000];
  pid_t pid;
  FILE* outfp;
  FILE* errfp;
  size_t outbytes;
  size_t errbytes;
  size_t byteswritten;
  int outfd[2]; 
  int errfd[2];
  int wstatus;
  int progrc;
  int rc;
  int i;

  if (pipe(outfd) == -1) {
    perror("stdout/stdin pipe");
    return -1;
  }
  if (pipe(errfd) == -1) {
    perror("stderr pipe");
    return -1;
  }

  pid = fork();
  if (pid == -1) {
    perror("fork");
    return -1;
  }

  if (pid == 0) { /* child */
    while ((dup2(outfd[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
    while ((dup2(errfd[1], STDERR_FILENO) == -1) && (errno == EINTR)) {}
    close(outfd[1]);
    close(outfd[0]);
    close(errfd[1]);
    close(errfd[0]);
    execv(cmd, argv);

    /* execl should not return except in error */
    perror("execl");
    exit(1); 
  }

  /* parent */
  close(outfd[1]);
  close(errfd[1]);

  outfp = fopen("stdout-memory", "w+,type=memory");
  if (!outfp) {
    return -1;
  }
  errfp = fopen("stderr-memory", "w+,type=memory");
  if (!errfp) {
    return -1;
  }

  do {
    outbytes = read_stream_into_memfile(outfd[0], outfp, buffer, sizeof(buffer));
    errbytes = read_stream_into_memfile(errfd[0], errfp, buffer, sizeof(buffer));
  } while (outbytes > 0 || errbytes > 0);

  close(outfd[0]);
  close(errfd[0]);

  waitpid(pid, &wstatus, 0);
  progrc = WEXITSTATUS(wstatus);

  rc = convert_memfile_to_array(outfp, out, outsize);
  if (rc) {
    return -1;
  }
  rc = convert_memfile_to_array(errfp, err, errsize);
  if (rc) {
    return -1;
  }

  return progrc;
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

static Mode* drdmod(Dataset* dataset, SAFInfo* info)
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
    return NULL;
  }
  argv[1] = cmd;

  rc = runcmd("/bin/tsocmd", argv, &out, &outsize, &err, &errsize);
  if (rc != 0) {
    fprintf(stderr, "Internal error: %s failed with rc: %d\n", cmd, rc);
    return NULL;
  }

  /*
   * This is ugly - to be replaced with code that talks to RACF programmatic interface
   * (in 31-bit assembler :( )
   */

  loc = findhdr(out, gencols);
  if (!loc) {
    fprintf(stderr, "Unable to find general column header from command %s\n", cmd);
    return NULL;
  }

  loc = skipdashes(loc);
  if (!loc) {
    fprintf(stderr, "Unable to skip dashed lines after general column header from command %s\n", cmd);
    return NULL;
  }

  owner = getcol(loc, 2);
  if (!owner) {
    fprintf(stderr, "Unable to determine dataset owner from command %s\n", cmd);
    return NULL;
  }
  uacc = getcol(loc, 3);
  if (!loc) {
    fprintf(stderr, "Unable to determine universal access from command %s\n", cmd);
    return NULL;
  }

  return NULL;
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

  dupword(&loc[DEFAULT_GROUP_PREFIX_LEN], info->default_group.name, GROUP_SIZE);

  buff = out;
  while ((loc = strstr(buff, GROUP_PREFIX))) {
    ++numgroups;
    buff=&loc[GROUP_PREFIX_LEN];
  }

  info->numgroups = numgroups;
  info->group = calloc(numgroups, sizeof(SAFGroup));
  buff = out;
  i=0;
  while ((loc = strstr(buff, GROUP_PREFIX))) {
    dupword(&loc[GROUP_PREFIX_LEN], info->group[i].name, GROUP_SIZE);
    buff=&loc[GROUP_PREFIX_LEN];
    ++i;
  }
  return info;
}


static int compute_modes(Mode* mode, Dataset* reference)
{
  fprintf(stderr, "write code to compute modes\n");
  exit(4);
  return 0;
}

void* dchmod_init(const char* userid, Mode* mode, Dataset* reference, int verbose)
{
  SAFInfo* info;
  void* work;
  size_t i;

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

  if (reference) {
    if (compute_modes(mode, reference)) {
      return NULL;
    }
  }

  strncpy(info->userid, userid, USER_SIZE+1);
  info->verbose = verbose;

  if (!rdinfo(info)) {
    return NULL;
  }

  if (info->verbose) {
    fprintf(stderr, "RACF Info:\n User ID: %s\n Reference Dataset: %s\n Default Group: %s\n Groups:\n", 
      info->userid, (reference == NULL) ? "<null>" : reference->name, info->default_group.name);
    for (i=0; i<info->numgroups; ++i) {
      fprintf(stderr, "  %s\n", info->group[i].name);
    }
  }

  return work;
}

void dchmod_term(void* work)
{
  if (work) {
    free(work);
  }
}

int dchmod(Mode* mode, Dataset* dataset, void* work)
{
  SAFInfo* info = (SAFInfo*) work;
  int rc;

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

  if (!drdmod(dataset, info)) {
    return 8;
  }
  /*
   * For now, just support for adding READ permission for JUST a group
   */
  return 0;
}

