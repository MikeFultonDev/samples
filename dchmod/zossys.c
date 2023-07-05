
/*
 * runcmd: runs command via fork/exec, captures stderr, stdout, 
 * and copies into arrays (via a memory file)
 */

#define _POSIX_SOURCE 1
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include "zossys.h"

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
