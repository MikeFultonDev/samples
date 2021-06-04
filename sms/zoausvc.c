/*********************************************************************
 * Copyright (c) 2021 IBM
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/
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

#include "zoausvc.h"
#include "sms.h"

#define PRTMSG(num, pgm) fprintf(stderr, zoausvcmsg[num], pgm)
#define CHKEXP0(exp, num, pgm) { if (exp) { fprintf(stderr, zoausvcmsg[num], pgm); return zoausvcerr[num]; } }
#define CHKEXP1(exp, num, pgm, a1) { if (exp) { fprintf(stderr, zoausvcmsg[num], pgm, a1); return zoausvcerr[num]; } }
#define CHKEXP2(exp, num, pgm, a1, a2) { if (exp) { fprintf(stderr, zoausvcmsg[num], pgm, a1, a2); return zoausvcerr[num]; } }
#define CHKEXP1NULL(exp, num, pgm, a1) { if (exp) { fprintf(stderr, zoausvcmsg[num], pgm, a1); return NULL; } }

typedef enum {
	noerr = 0,
	pipeerr = 1,
	duperr = 2,
	ccloseerr = 3,
	pcloseerr = 4,
	mallocerr = 5,
	openmemerr = 6,
	writeinerr = 7,
	writememerr = 8,
	ftellmemerr = 9,
	readmemerr = 10,
	closememerr = 11,
	waitpiderr = 12,
} ZOAUSVCMsg_T;

static const char* zoausvcmsg[] = {
	"No Error",
	"%s: Unable to open %s pipe. Return code from pipe:%d\n",
	"%s: Unable to dup2 %s pipe.\n", 
	"%s: Unable to close child %s pipe.\n", 
	"%s: Unable to close parent %s pipe.\n", 
	"%s: Unable to allocate %d bytes.\n", 
	"%s: Unable to open memory file.\n",
	"%s: Expected to write %d input bytes but wrote %d bytes.\n",
	"%s: Expected to write %d bytes to memory file but wrote %d bytes.\n",
	"%s: ftell on memory file failed.\n",
	"%s: Expected to read %d bytes from memory file but read %d bytes.\n",
	"%s: Unable to close memory file.\n",
	"%s: Failed waiting on child process.\n",
};

static const int zoausvcerr[] = {
	0x0, 
	0x10,
	0x20,
	0x30,
	0x40,
	0x50,
	0x60,
	0x70,
	0x80,
	0x90,
	0xA0,
	0xB0,
	0xC0,
};
	
int zoausvc(const char* input, char** output, char* opts[]) {
	char buf[137+1];
	int stdinfd[2];
	int stdoutfd[2];
	int stderrfd[2];
	pid_t pid = 0;
	FILE* memfp;
	char** argv;
	int status;
	int rc;
	int inlen;
	int outlen;
	int i=0;
	char* pgm = opts[0];
	int arglistsize;
	int numopts = 0;

	rc = pipe(stdinfd);
	CHKEXP1((rc != 0), pipeerr, pgm, "stdin");
	rc = pipe(stdoutfd);
	CHKEXP1((rc != 0), pipeerr, pgm, "stdout");
	rc = pipe(stderrfd);
	CHKEXP1((rc != 0), pipeerr, pgm, "stderr");
	pid = fork();

	if (pid == 0) {
		/* Child */
		rc = dup2(stdinfd[0], STDIN_FILENO);
		CHKEXP1((rc == -1), duperr, pgm, "stdin");
		rc = dup2(stdoutfd[1], STDOUT_FILENO);
		CHKEXP1((rc == -1), duperr, pgm, "stdout");
		rc = dup2(stderrfd[1], STDERR_FILENO);
		CHKEXP1((rc == -1), duperr, pgm, "stderr");

		rc = close(stdinfd[1]);
		CHKEXP1((rc == -1), ccloseerr, pgm, "stdin[1]");
		rc = close(stdoutfd[0]);
		CHKEXP1((rc == -1), ccloseerr, pgm, "stdout[0]");
		rc = close(stderrfd[0]);
		CHKEXP1((rc == -1), ccloseerr, pgm, "stderr[0]");

		while (opts[numopts] != NULL) { ++numopts; }
		numopts++;
		arglistsize = numopts * (sizeof(const char**));
		argv = malloc(arglistsize);
		CHKEXP1((argv == NULL), mallocerr, pgm, arglistsize);
		memcpy(argv, opts, arglistsize);
		execvp(argv[0], argv);
		exit(1);
	}

        /* Parent */
	rc = close(stdinfd[0]);
	CHKEXP1((rc == -1), pcloseerr, pgm, "stdin[0]");
	rc = close(stdoutfd[1]);
	CHKEXP1((rc == -1), pcloseerr, pgm, "stdout[1]");
	rc = close(stderrfd[1]);
	CHKEXP1((rc == -1), pcloseerr, pgm, "stderr[1]");

	inlen = (input == NULL) ? 0 : strlen(input);
	if (inlen > 0) {
		rc = write(stdinfd[1], input, inlen);
		CHKEXP2((rc != inlen), writeinerr, pgm, inlen, rc);
	}

	rc = close(stdinfd[1]);
	CHKEXP1((rc == -1), pcloseerr, pgm, "stdin[1]");

	memfp = fopen("%s.output", "wb+,type=memory");
	CHKEXP0((memfp == NULL), openmemerr, pgm);
	while ((rc = read(stdoutfd[0], buf, sizeof(buf))) > 0) {
		int written;
		written = fwrite(buf, 1, rc, memfp);
		CHKEXP2((rc != written), writememerr, pgm, written, rc);
	}
        rc = close(stdoutfd[0]);
	CHKEXP1((rc == -1), pcloseerr, pgm, "stdout[0]");
	outlen = ftell(memfp);
	CHKEXP0((outlen < 0), ftellmemerr, pgm);
	rewind(memfp);

	*output = malloc(outlen+1);
	CHKEXP1((*output == NULL), mallocerr, pgm, outlen+1);
	if (outlen > 0) {
		rc = fread(*output, 1, outlen, memfp);
		CHKEXP2((rc != outlen), readmemerr, pgm, outlen, rc);
	} 
	(*output)[outlen] = '\0';
	rc = fclose(memfp);
	CHKEXP0((rc != 0), closememerr, pgm);
	while ((rc = read(stderrfd[0], buf, 256)) > 0) {
		fprintf(stderr, "%.*s",rc, buf);
	}
        rc = close(stderrfd[0]);
	CHKEXP1((rc == -1), pcloseerr, pgm, "stderr[0]");
	do {
            	if ((pid = waitpid(pid, &status, 0)) == -1) {
                        perror("");
			PRTMSG(waitpiderr, pgm);
                        rc = -1;
		} else if (pid == 0) {
                        sleep(1);
		} else {
                        if (WIFEXITED(status)) {
                                rc = WEXITSTATUS(status);
                        } else {
                                if (WIFSIGNALED(status)) {
                                        fprintf(stderr, "%s abended\n");
                                }
                                rc = 255;
                        }
		}
	} while (pid == 0);
	return rc;
}

static char** concopts(const char* pgm, const char* coreopts[], const char* usropts[]) {
	char** opts;
	int numcoreopts = 0;
	int numusropts = 0;
	int argsize;
	while (coreopts[numcoreopts] != NULL) { ++numcoreopts; }
	while (usropts[numusropts] != NULL) { ++numusropts; }
	numusropts++;
	argsize = (numcoreopts+numusropts) * (sizeof(const char**));

	opts = malloc(argsize);
	CHKEXP1NULL((opts == NULL), mallocerr, pgm, argsize);
	memcpy(opts, coreopts, numcoreopts*(sizeof(const char**)));
	if (numusropts > 0) {
		memcpy(&opts[numcoreopts], usropts, numusropts*(sizeof(const char**)));
	}

	return opts;
}

int batchtso(const char* input, char** output, const char* usropts[]) {
	int rc;
	char** opts;
	const char* coreopts[] = {
		"mvscmdauth",
		"--pgm=IKJEFT1B",
		"--systsin=stdin",
		"--sysprint=stdout",
		"--systsprt=stdout",
		NULL
	};
	
	opts = concopts(coreopts[0], coreopts, usropts);
	rc = zoausvc(input, output, opts);
	free(opts);
	return rc;
}

int xsysvar(char **output, const char* usropts[]) {
	int rc;
	char** opts;
	const char* coreopts[] = { "Xsysvar", NULL };
	char* input = "";

	opts = concopts(coreopts[0], coreopts, usropts);
	rc = zoausvc(input, output, opts);
	free(opts);
	if (rc == 0) {
		size_t outlen = strlen(*output);
		(*output)[outlen-1] = '\0'; /* remove newline */
	}
	return rc;
}

int hlq(char **output, const char* usropts[]) {
	int rc;
	char** opts;
	const char* coreopts[] = { "hlq", NULL };
	char* input = "";

	opts = concopts(coreopts[0], coreopts, usropts);
	rc = zoausvc(input, output, opts);
	free(opts);
	return rc;
}
