#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
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

  drdmod(dataset);
  /*
   * For now, just support for adding READ permission for JUST a user
   */
  return 0;
}

static int runcmd(const char* cmd, char* argv[], char** out, char** err) 
{
  char buffer[10000];
  ssize_t outbytes = 0;
  ssize_t errbytes = 0;
  ssize_t outtot = 0;
  ssize_t errtot = 0;
  int outfd[2]; 
  int errfd[2];
  pid_t pid;

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
 
  do {
    outbytes = read(outfd[0], buffer, sizeof(buffer));
    if (outbytes == -1) {
      if (errno != EINTR) { 
        perror("read-out");
        return -1;
      }
    } else {
      outtot += outbytes;
      fprintf(stdout, "%*.*s", outbytes, outbytes, buffer);
    }

    errbytes = read(errfd[0], buffer, sizeof(buffer));
    if (errbytes == -1) {
      if (errno != EINTR) { 
        perror("read-err");
        return -1;
      }
    } else {
      errtot += errbytes;
      fprintf(stderr, "%*.*s", errbytes, errbytes, buffer);
    }

  } while (outbytes != 0 && errbytes != 0);
  close(outfd[0]);
  close(errfd[0]);

  printf("read %d bytes from stdout and %d bytes from stderr\n", outtot, errtot);
  wait(0);

  *err = NULL;
  *out = NULL;
  return 0;

}

Mode* drdmod(Dataset* dataset)
{
  char cmd[256];
  char* out;
  char* err;
  int argc=2;
  char* argv[] = { "tsocmd", NULL, NULL }; 
  int rc;

  rc = snprintf(cmd, sizeof(cmd), "LISTDSD DATASET('%s') GENERIC", dataset->name);
  if (rc >= sizeof(cmd)) {
    fprintf(stderr, "Internal error. Truncation occurred\n");
    return NULL;
  }
  argv[1] = cmd;

  rc = runcmd("/bin/tsocmd", argv, &out, &err);

  return NULL;
}
  

