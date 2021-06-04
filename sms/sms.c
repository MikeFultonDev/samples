/*********************************************************************
 * Copyright (c) 2021 IBM
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/
#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "sms.h"
#include "zoausvc.h"

#define MAXDSLEN 44

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

static int parsearg(SMS* sms, const char* validopts) {
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

static batchismf(SMS* sms, char* input, char** output) {
	char* tempISPFProfile;
	char* mvstmpopts[] = { sms->opts.prop.val[SMSTMPHLQ], NULL };
	int rc;

	rc = mvstmp(NULL, &tempISPFProfile, mvstmpopts);
	rc = dtouch(tempISPFProfile);	
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
	if (sms->opts.list) {
		char* output;
		int rc;
		rc = batchismf(sms, "SCDS(ACTIVE) STORGRP(FRED)", &output);	
		if (rc == 0) {
			fprintf(stdout, "<%s>\n", output);
		} else {
			fprintf(stderr, "Error running batchismf:%d\n", rc);
		}
		if (output) {
			free(output);
		}
	} else {
		fprintf(stderr, "Only list implemented so far\n");
	}
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

static SMSError getenvifset(const char* env, char** val) {
	char* envval = getenv(env);  
	size_t vallen;
	if (!envval) {
		return SMSEnvVarNotSet;
	}

	vallen = strlen(envval);
	*val = malloc(vallen+1);
	if (*val) {
		memcpy(*val, envval, vallen+1);
		return SMSNoErr;
	}

	errmsg(SMSAllocErr);
	return SMSAllocErr;
}

static int getproperty(const char* key, char** val) {
	int rc;
	size_t vallen;
	const char* xsysvaropts[] = { key, NULL };
	SMSError enverr;

	enverr = getenvifset(key, val);
	if (enverr != SMSEnvVarNotSet) {
		return enverr;
	}

	rc = xsysvar(val, xsysvaropts);
	if (rc) {
		return SMSPropertyErr;
	}

	return SMSNoErr;
}

static int tmphlq(char** val) {
	const char* TMP_HLQ = "TMPHLQ";
	SMSError enverr;
	const char* hlqopts[] = { NULL };
	int rc;

	enverr = getenvifset(TMP_HLQ, val);
	if (enverr != SMSEnvVarNotSet) {
		return enverr;
	}
	rc = hlq(val, hlqopts);

	if (rc) {
		return SMSPropertyErr;
	}

	return SMSNoErr;
}

static int setupBaseEnv(SMS* sms) {
	int rc;
	char* value;
	const char* ISPF_HLQ = "HIF7R02_HLQ";
	const char* ISMF_HLQ = "EDU1H01_HLQ";

	rc = tmphlq(&value);
	if (rc) {
		errmsg(SMSSetupErr, "tmphlq", rc);
		return SMSSetupErr;
	}
	sms->opts.prop.val[SMSTMPHLQ] = value;

	rc = getproperty(ISPF_HLQ, &value);
	if (rc) {
		errmsg(SMSPropertyErr, ISPF_HLQ, rc);
		return SMSPropertyErr;
	}
	sms->opts.prop.val[SMSISPFHLQ] = value;

	rc = getproperty(ISMF_HLQ, &value);
	if (rc) {
		errmsg(SMSPropertyErr, ISMF_HLQ, rc);
		return SMSPropertyErr;
	}
	sms->opts.prop.val[SMSISMFHLQ] = value;

	return SMSNoErr;
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

	sms->err = setupBaseEnv(sms);
	if (!sms->inerr(sms)) {
		sms->err = sms->parsearg(sms);
	}

	return sms;
}
