#define _POSIX_SOURCE 1
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include "dchmod.h"

char *strtok_r(char *str, const char *delim, char **saveptr);

#define USER_SIZE 8
#define GROUP_SIZE 8
typedef struct {
  char userid[USER_SIZE+1];
  char default_group[GROUP_SIZE+1];
  char** groups; 
} SAFInfo;

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

static int runcmd(const char* cmd, char* argv[], char** out, size_t* outsize, char** err, size_t* errsize)
{
  char buffer[10000];
  pid_t pid;
  FILE* outfp;
  FILE* errfp;
  ssize_t outbytes = 0;
  ssize_t errbytes = 0;
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
  if (! outfp) {
    return -1;
  }
  errfp = fopen("stderr-memory", "w+,type=memory");
  if (! errfp) {
    return -1;
  }

  do {
    outbytes = read(outfd[0], buffer, sizeof(buffer));
    if (outbytes == -1) {
      if (errno != EINTR) { 
        perror("read-out");
        return -1;
      }
    } else {
      fprintf(outfp, "%*.*s", (int) outbytes, (int) outbytes, buffer);
    }

    errbytes = read(errfd[0], buffer, sizeof(buffer));
    if (errbytes == -1) {
      if (errno != EINTR) { 
        perror("read-err");
        return -1;
      }
    } else {
      fprintf(errfp, "%*.*s", (int) errbytes, (int) errbytes, buffer);
    }
  } while (outbytes != 0 || errbytes != 0);

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

  rc = snprintf(cmd, sizeof(cmd), "LISTDSD DATASET('%s') GENERIC ALL", dataset->name);
  if (rc >= sizeof(cmd)) {
    fprintf(stderr, "Internal error. Truncation occurred\n");
    return NULL;
  }
  argv[1] = cmd;

  rc = runcmd("/bin/tsocmd", argv, &out, &outsize, &err, &errsize);

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

  rc = snprintf(cmd, sizeof(cmd), "LISTUSER %s", info->userid);
  if (rc >= sizeof(cmd)) {
    fprintf(stderr, "Internal error. Truncation occurred\n");
    return NULL;
  }
  argv[1] = cmd;

  rc = runcmd("/bin/tsocmd", argv, &out, &outsize, &err, &errsize);

  printf("rc:%d out:%p outsize:%zu err:%p errsize:%zu\n", rc, out, outsize, err, errsize);

  buff = out;
  rc = printf("out\n%s\n", out);
  printf("printed %d characters\n", rc);
  printf("err\n%s\n", err);
  while ((loc = strstr(buff, "GROUP="))) {
    printf("%14.14s\n", loc);
    buff=&loc[1];
  }

  return NULL;
}

/*
 *
 * LISTDSD DATASET('FULTONM.MY.FILE') GENERIC ALL
INFORMATION FOR DATASET FULTONM.MY.FILE (G)

LEVEL  OWNER    UNIVERSAL ACCESS   WARNING   ERASE
-----  -------- ----------------   -------   -----
 00    FULTONM         NONE          NO      NO

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

static int compute_modes(Mode* mode, Dataset* reference)
{
  fprintf(stderr, "write code to compute modes\n");
  exit(4);
  return 0;
}

void* dchmod_init(const char* userid, Mode* mode, Dataset* reference)
{
  SAFInfo* info;
  void* work;
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

  rdinfo(info);

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

