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

static int cidneTempSeq(SMS* sms, char** name) { /* cidne -> Create If Does Not Exist */
	const char* mvstmpopts[] = { sms->opts.prop.val[SMSTMPHLQ], NULL };
	const char* dtouchopts[] = { "-tseq", "-l80", "-rfb", NULL, NULL };
	char* out;
	int rc;

	if (*name) { return SMSNoErr; }

	rc = mvstmp(name, mvstmpopts);
	if (rc) {
		if (*name) {
			free(*name);
			*name = NULL;
		}
		return SMSISMFErr;
	}
	dtouchopts[3] = *name;
	rc = dtouch(&out, dtouchopts);	
	if (rc) {
		if (out) {
			free(out);
		}
		return SMSISMFErr;
	}
	return SMSNoErr;
}

static int cidneTempPDSE(SMS* sms, char** name) { /* cidne -> Create If Does Not Exist */
	const char* mvstmpopts[] = { sms->opts.prop.val[SMSTMPHLQ], NULL };
	const char* dtouchopts[] = { NULL, NULL };
	char* out;
	int rc;

	if (*name) { return SMSNoErr; }

	rc = mvstmp(name, mvstmpopts);
	if (rc) {
		if (*name) {
			free(*name);
			*name = NULL;
		}
		return SMSISMFErr;
	}
	dtouchopts[0] = *name;
	rc = dtouch(&out, dtouchopts);	
	if (rc) {
		if (out) {
			free(out);
		}
		return SMSISMFErr;
	}
	return SMSNoErr;
}

static int writeText(SMS* sms, const char* file, const char* text) {
	char qualfile[54+4+1];
	FILE* fp; 
	int rc;

	if (file[0] == '/') {
		strcpy(qualfile, file);
	} else {
		sprintf(qualfile, "//'%s'", file);
	}
	fp = fopen(qualfile, "a");
	if (!fp) {
		perror("fopen error on file");         
		return SMSISMFErr;          
	}
	rc = fwrite(text, 1, strlen(text), fp);
	if (rc != strlen(text)) {
		perror("fwrite error on file");         
		return SMSISMFErr;
	}	
	rc = fclose(fp);
	if (rc) {
		perror("fclose error on file");         
		return SMSISMFErr;
	}	
	return SMSNoErr;
}

enum { 
	ISPF_STEPLIBDD=0,
	ISPF_ISPPLIBDD=1,
	ISPF_ISPMLIBDD=2,
	ISPF_ISPSLIBDD=3,
	ISPF_SYSEXECDD=4,
	ISPF_ISPLOGDD=5,
	ISPF_ISPTLIBDD=6,
	ISPF_ISPTABLDD=7,
	ISPF_ISPPROFDD=8,
	ISPF_NUM_DD=9
};

static int batchismf(SMS* sms, const char* input, char** output, const char* usropts[]) {
	SMSError err;
	char dd[ISPF_NUM_DD][2+8+1+(4*(44+1))+1];  /* --<dd>=<dsn1>:<dsn2>:<dsn3>:<dsn4><null> */
	const char* ddp[ISPF_NUM_DD+1];
	const char* ispfhlq = sms->opts.prop.val[SMSISPFHLQ];
	const char* ismfhlq = sms->opts.prop.val[SMSISMFHLQ];
	char** batchtsoopts;
	int i;
	int rc;

	err = cidneTempPDSE(sms, &sms->opts.tmpProfile);
	if (err != SMSNoErr) { return err; }
	err = cidneTempPDSE(sms, &sms->opts.tmpInputTable);
	if (err != SMSNoErr) { return err; }
	err = cidneTempPDSE(sms, &sms->opts.tmpOutputTable);
	if (err != SMSNoErr) { return err; }

	sprintf(dd[ISPF_STEPLIBDD], "--STEPLIB=%s.DGTLLIB:%s.SISPLOAD", ismfhlq, ispfhlq);
	sprintf(dd[ISPF_ISPPLIBDD], "--ISPPLIB=%s.DGTPLIB:%s.SISPPENU", ismfhlq, ispfhlq);
	sprintf(dd[ISPF_ISPMLIBDD], "--ISPMLIB=%s.DGTMLIB:%s.SISPMENU", ismfhlq, ispfhlq);
	sprintf(dd[ISPF_ISPSLIBDD], "--ISPSLIB=%s.DGTSLIB:%s.SISPSENU", ismfhlq, ispfhlq);
	sprintf(dd[ISPF_SYSEXECDD], "--SYSEXEC=%s.DGTCLIB:%s.SISPCLIB", ismfhlq, ispfhlq);
	sprintf(dd[ISPF_ISPLOGDD],  "--ISPLOG=DUMMY");
	sprintf(dd[ISPF_ISPTLIBDD], "--ISPTLIB=%s:%s:%s.DGTTLIB:%s.SISPTENU", 
		sms->opts.tmpInputTable, sms->opts.tmpOutputTable, ismfhlq, ispfhlq);
	sprintf(dd[ISPF_ISPTABLDD], "--ISPTABL=%s", sms->opts.tmpOutputTable); 
	sprintf(dd[ISPF_ISPPROFDD], "--ISPPROF=%s", sms->opts.tmpProfile); 

	for (i=0; i<ISPF_NUM_DD; ++i) {
		ddp[i] = dd[i];
	}
	ddp[ISPF_NUM_DD] = NULL;
	batchtsoopts = concopts("batchismf", ddp, usropts);
	if (!batchtsoopts) {
		return SMSAllocErr;
	}
	rc = batchtso(input, output, (const char**) batchtsoopts);
	free(batchtsoopts);
	return rc;
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
		const char* genopts[] = { NULL };
		const char* rptopts[] = { NULL, NULL, NULL };
		SMSError err;
		char* output;
		char* input;
		char rptdd[80];
		char* tmpseq=NULL;
		int rc;
		input="\
ISPSTART CMD(ACBQBAIG SAVE SGNAMES + \n\
SCDS('SYS1.S0W1.SCDS') STORGRP(*) STGTYPE(POOL) SPACEGB(N))+ \n\
NEWAPPL(DGT) BATSCRW(132) BATSCRD(27) BREDIMAX(3) BDISPMAX(99999999)";

		rc = batchismf(sms, input, &output, genopts);	
		if (rc == 0) {
#if DEBUG
			fprintf(stdout, "Table Generation Results: <%s>\n", output);
#endif
		} else {
			fprintf(stderr, "Error running batchismf:%d\n", rc);
			fprintf(stderr, "%s", output);
		}
		if (output) {
			free(output);
		}
		input="\
ISPSTART CMD(ACBQBARJ SGNAMES) +\n\
BATSCRW(132) BATSCRD(27) BREDIMAX(3) BDISPMAX(99999999)";
		err = cidneTempSeq(sms, &tmpseq); 
		if (err) { fprintf(stderr, "failure creating temporary sequential dataset\n"); return err; }
		err = writeText(sms, tmpseq, "STORGRP SGTYPE TOTALSPC FREESPC");
		if (err) { fprintf(stderr, "failure writing to temporary sequential dataset\n"); }
		sprintf(rptdd, "--SYSIN=%s", tmpseq);
		rptopts[0] = "--ISPFILE=stdout";
		rptopts[1] = rptdd;
		rc = batchismf(sms, input, &output, rptopts);	
		if (rc == 0) {
			puts(output);
		} else {
			fprintf(stderr, "Error running batchismf:%d\n", rc);
			fprintf(stderr, "%s", output);
		}
		if (output) {
			free(output);
		}
	} else {
		fprintf(stderr, "Only list implemented so far\n");
	}
	return SMSNoErr;
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
