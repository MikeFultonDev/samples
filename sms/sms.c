/*********************************************************************
 * Copyright (c) 2021 IBM
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/
#include "sms.h"
#define _XOPEN_SOURCE
#include <stdio.h>

static const char* pgmname(SMS* sms) {
	switch (sms->prog) {
		case SMSStorageGroup: return "smssg";
		case SMSStorageClass: return "smssc";
	}
	return "inv";
}

static int validopt(char c, const char* list) {
	int i;
	for (i=0; list[i] != '\0'; ++i) {
		if (list[i] == c) { return 1; }
	}
	return 0;
}

static int syntax(struct SMS* sms) {
	fprintf(stderr, "Syntax for %s\n", pgmname(sms));
	fprintf(stderr, "...tbd\n");
	return SMSSyntax;  
}

static int errmsg(SMSError err, ...) {
	fprintf(stderr, "Error message %d issued\n", err);
	return err;
}

static int parsearg(struct SMS* sms, const char* validopts) {
	int opt;
	opterr = 0; 
	while ((opt = getopt(sms->argc, sms->argv, "cdhlr")) != -1) {
		if (!validopt(opt, validopts)) {
			errmsg(SMSOptErr, opt);
			return SMSOptErr;
		}  	
		switch (opt) {
			case 'v': 
				sms->opts.verbose = 1;
				break;
			case 'l': 
				sms->opts.list = 1;
				break;
			case 'c':
				sms->opts.create = 1;
				break;
			case 'd':
				sms->opts.delete = 1;
				break;
			case 'r':
				sms->opts.rename = 1;
				break;
			case 'h':
				sms->opts.syntax = 1;
				return syntax(sms);
			default:
				errmsg(SMSOptErr, opt);
				return SMSOptErr;
		}
	}
	sms->opts.extraarg = optind;
	return SMSNoErr;
}
				
static int genprterr(struct SMS* sms) { 
	fprintf(stderr, "Invalid parameters passed to: %s argc:%d\n", pgmname(sms), sms->argc); 
}
static int genrc(struct SMS* sms) { 
	return sms->err; 
}
static int geninerr(struct SMS* sms) { 
	SMSError err = genrc(sms);
	return err != SMSSyntax && err != SMSNoErr; 
}

static int invparse(struct SMS* sms) { 
	return 0; 
}
static int invrunsvc(struct SMS* sms) { 
	return 0; 
}
static int sgparse(struct SMS* sms) { 
	return parsearg(sms, "cdhlr");
}
static int sgrunsvc(struct SMS* sms) { 
	return 0; 
}
static int scparse(struct SMS* sms) { 
	return parsearg(sms, "cdhlr");
}
static int scrunsvc(struct SMS* sms) { 
	return 0; 
}

static SMS invSMS = { invparse, invrunsvc, geninerr, genprterr, genrc };
static SMS sgSMS  = { sgparse,  sgrunsvc,  geninerr, genprterr, genrc };
static SMS scSMS  = { scparse,  scrunsvc,  geninerr, genprterr, genrc };

static int setupBaseEnv(SMSOpts* opt) {

}

SMS* crtSMS(SMSProgram prog, int argc, char* argv[]) {
	SMS* sms;
	switch(prog) {
		case SMSStorageGroup:
			sms = &sgSMS;
			break;
		case SMSStorageClass:
			sms = &scSMS;
			break;
		default: 
			sms = &invSMS;
			break;
	}
	sms->prog = prog;
	sms->argc = argc;
	sms->argv = argv;
	sms->err  = sms->parsearg(sms);

	return sms;
}
