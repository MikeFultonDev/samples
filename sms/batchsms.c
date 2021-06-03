#include "sms.h"
#define _OPEN_SYS_UNLOCKED_EXT 1
#define _XOPEN_SOURCE 1

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>

#define _POSIX_SOURCE

#include<unistd.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>

int batchsms(const char* input, char** output, char* opts[]) {
	char buf[137+1];
	int stdinfd[2];
	int stdoutfd[2];
	int stderrfd[2];
	pid_t pid = 0;
	FILE* memfp;
	char** argv;
	size_t numopts=0;
	int status;
	int rc;
	int inlen;
	int outlen;
	int i=0;
	const int mvscmdcoreopts=5;

	rc = pipe(stdinfd);
	if (rc) { fprintf(stderr, "batchsms: Unable to open stdin pipe\n"); return rc; }
	rc = pipe(stdoutfd);
	if (rc) { fprintf(stderr, "batchsms: Unable to open stdout pipe\n"); return rc; }
	rc = pipe(stderrfd);
	if (rc) { fprintf(stderr, "batchsms: Unable to open stderr pipe\n"); return rc; }
	pid = fork();

	if (pid == 0) {
		/* Child */
		rc = dup2(stdinfd[0], STDIN_FILENO);
		if (rc == -1) { fprintf(stderr, "batchsms: Unable to dup2 stdin pipe\n"); return rc; }
		rc = dup2(stdoutfd[1], STDOUT_FILENO);
		if (rc == -1) { fprintf(stderr, "batchsms: Unable to dup2 stdout pipe\n"); return rc; }
		rc = dup2(stderrfd[1], STDERR_FILENO);
		if (rc == -1) { fprintf(stderr, "batchsms: Unable to dup2 stderr pipe\n"); return rc; }

		rc = close(stdinfd[1]);
		if (rc == -1) { fprintf(stderr, "batchsms: Unable to close child stdin pipe\n"); return rc; }
		rc = close(stdoutfd[0]);
		if (rc == -1) { fprintf(stderr, "batchsms: Unable to close child stdout pipe\n"); return rc; }
		rc = close(stderrfd[0]);
		if (rc == -1) { fprintf(stderr, "batchsms: Unable to close child stderr pipe\n"); return rc; }

		while (opts[numopts] != NULL) { ++numopts; }
		numopts++;
		argv = malloc((mvscmdcoreopts+numopts) * (sizeof(const char**)));
		argv[0] = "mvscmdauth";
		argv[1] = "--pgm=IKJEFT1B";
		argv[2] = "--systsin=stdin";
		argv[3] = "--sysprint=stdout";
		argv[4] = "--systsprt=stdout";
		memcpy(&argv[mvscmdcoreopts], opts, numopts*(sizeof(const char**)));
		execvp(argv[0], argv);
		exit(1);
	}

        /* Parent */
	rc = close(stdinfd[0]);
	if (rc == -1) { fprintf(stderr, "batchsms: Unable to close parent stdin pipe\n"); return rc; }
	rc = close(stdoutfd[1]);
	if (rc == -1) { fprintf(stderr, "batchsms: Unable to close parent stdout pipe\n"); return rc; }
	rc = close(stderrfd[1]);
	if (rc == -1) { fprintf(stderr, "batchsms: Unable to close parent stderr pipe\n"); return rc; }

	inlen = strlen(input);
	rc = write(stdinfd[1], input, inlen);
	if (rc != inlen) { fprintf(stderr, "batchsms: Expected to write %d input bytes but wrote %d bytes\n", inlen, rc); return -1; }

	rc = close(stdinfd[1]);
	if (rc == -1) { fprintf(stderr, "batchsms: Unable to close parent stderr pipe\n"); return rc; }

	memfp = fopen("batchsms.output", "wb+,type=memory");
	if (memfp == NULL) { fprintf(stderr, "batchsms: Unable to open memory file\n"); return -1; }
	while ((rc = read(stdoutfd[0], buf, sizeof(buf))) > 0) {
		int written;
		written = fwrite(buf, 1, rc, memfp);
		if (written != rc) { fprintf(stderr, "batchsms: Expected to write %d output bytes but wrote %d bytes\n", rc, written); return -1; }
	}
        rc = close(stdoutfd[0]);
	if (rc == -1) { fprintf(stderr, "batchsms: Unable to close parent stdout pipe\n"); return rc; }
	outlen = ftell(memfp);
	if (outlen < 0) { fprintf(stderr, "batchsms: ftell on memory file failed\n"); return -1; }
	rewind(memfp);

	*output = malloc(outlen);
	if (*output == NULL) { fprintf(stderr, "batchsms: Unable to malloc %d bytes for output buffer\n", outlen); return -1; }
	rc = fread(*output, 1, outlen, memfp);
	if (outlen != rc) { fprintf(stderr, "batchsms: Expected to read %d output bytes but wrote %d bytes\n", outlen, rc); return -1; }
	rc = fclose(memfp);
	if (rc) { fprintf(stderr, "batchsms: Unable to close memory file\n"); return -1; }
	while ((rc = read(stderrfd[0], buf, 256)) > 0) {
		fprintf(stderr, "%.*s",rc, buf);
	}
        rc = close(stderrfd[0]);
	if (rc) { fprintf(stderr, "batchsms: Unable to close parent stderr pipe\n"); return -1; }
	do {
            	if ((pid = waitpid(pid, &status, 0)) == -1) {
                        perror("");
                        fprintf(stderr, "batchsms: Error waiting for pid\n");
                        rc = -1;
		} else if (pid == 0) {
                        sleep(1);
		} else {
                        if (WIFEXITED(status)) {
                                rc = WEXITSTATUS(status);
                        } else {
                                if (WIFSIGNALED(status)) {
                                        fprintf(stderr, "batchsms abended\n");
                                }
                                rc = 255;
                        }
		}
	} while (pid == 0);
	return rc;
}
