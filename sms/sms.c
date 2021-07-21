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
#include <string.h>
#include "sms.h"
#include "smsmsg.h"
#include "zoausvc.h"

static int validopt(char c, const char* list) {
	int i;
	for (i=0; list[i] != '\0'; ++i) {
		if (list[i] == c) { return 1; }
	}
	return 0;
}
 
/*
 * https://tech.mikefulton.ca/DatasetNames
 */
int parseds(SMS* sms, const char* input, SMSDataset* output) {
	size_t segchar = 0;
	size_t dschar = 0;
	size_t memchar = 0;
	int inds = 1;
	int inmem = 0;
	while (*input) {
		char c = input[dschar];
		char o;
		if (!inds && !inmem) {
			return errmsg(sms, SMSTrailingData, input);
		}
		if ((c == '.' || c == '(' || c == ')') && (segchar == 0)) {
			return errmsg(sms, SMSSegmentTooShort, input);
		}
		if (c == ')' && memchar == 0) {
			return errmsg(sms, SMSMemberTooShort, input);
		}
		if (dschar == SMSMAXDSLEN) {
			return errmsg(sms, SMSDatasetTooLong, input);
		}
		if (memchar == SMSMEMLEN) {
			return errmsg(sms, SMSMemberTooLong, input);
		}
		if (segchar == SMSSEGLEN) {
			return errmsg(sms, SMSSegmentTooLong, input);
		}
		if (c == '(') {
			inds = 0;
			inmem = 1;
		} else if (c == ')') {
			inmem = 0;
		}
		switch (c) {
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
			case 'g':
			case 'h':
			case 'i':
			case 'j':
			case 'k':
			case 'l':
			case 'm':
			case 'n':
			case 'o':
			case 'p':
			case 'q':
			case 'r':
			case 's':
			case 't':
			case 'u':
			case 'v':
			case 'w':
			case 'x':
			case 'y':
			case 'z':
				o = c + 0x40; /* NOTE: EBCDIC specific */
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
			case 'G':
			case 'H':
			case 'I':
			case 'J':
			case 'K':
			case 'L':
			case 'M':
			case 'N':
			case 'O':
			case 'P':
			case 'Q':
			case 'R':
			case 'S':
			case 'T':
			case 'U':
			case 'V':
			case 'W':
			case 'X':
			case 'Y':
			case 'Z':
			case '#':
			case '@':
			case '$':
				o = c;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '-':
				o = c;
				if (segchar == 0) {
					return errmsg(sms, SMSInvalidFirstCharInSegment, o);
				}
				break;
			default:
				return errmsg(sms, SMSInvalidCharInSegment, c);
				break;
		}
		if (inds) {
			dschar++;
		} else if (inmem) {
			memchar++;
		}
		segchar++;	
	}
}
	
int parsearg(SMS* sms, const char* validopts, size_t minarg, size_t maxarg) {
	size_t opt;
	opterr = 0; 
	size_t optarg;
	while ((opt = getopt(sms->argc, sms->argv, "cvdhlrtLTV")) != -1) {
		if (!validopt(opt, validopts)) {
			errmsg(sms, SMSOptErr, opt);
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
			case 't':
				sms->opts.test = 1;
				break;
			case 'T':
				sms->opts.translate = 1;
				break;
			case 'V':
				sms->opts.validate = 1;
				break;
			case 'L':
				sms->opts.volumes = 1;
				break;
			case 'r':
				sms->opts.rename = 1;
				break;
			case 'h':
				sms->opts.syntax = 1;
				return syntax(sms);
			default:
				errmsg(sms, SMSOptErr, opt);
				return SMSOptErr;
		}
	}
	sms->opts.extraarg = optind;
	optarg = (sms->argc - optind);
	if (optarg < minarg || optarg > maxarg) {
		printf("Expected %d to %d arguments but got %d were specified\n", minarg, maxarg, optarg);
		return SMSOptErr;
	}
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
	size_t textlen = strlen(text);
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
	rc = fwrite(text, 1, textlen, fp);
	if (rc != textlen) {
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

static int batchismf(SMS* sms, const char* input, const char* usropts[]) {
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
	if (sms->opts.verbose) {
		i=0;
		while (batchtsoopts[i] != NULL) {
			infomsg(sms, SMSBatchTSOParmInfo, batchtsoopts[i]);
			++i;
		}
		infomsg(sms, SMSBatchTSOParmInfo, "\n");
	}
	rc = batchtso(input, &sms->output, (const char**) batchtsoopts);
	free(batchtsoopts);
	return rc;
}

static int scdsset(SMS* sms) {
	return sms->opts.scds;
}
	
#if SCDSACTIVEDATASET
int getactivescds(SMS* sms, char** scds) {
	int rc;
	char* p;
	const char* scdspfx = "SCDS = ";
	size_t scdspfxlen = strlen(scdspfx);
	const char* opercmdopts[] = { "D SMS,ACTIVE", NULL };
	int i;
	size_t scdslen;

	if (!scdsset(sms)) {
		rc = opercmd(&sms->output, opercmdopts);
		if (rc) {
			return SMSISMFErr;
		}
		p = strstr(sms->output, scdspfx);
		if (!p) {
			return SMSISMFErr;
		}
		p = &p[scdspfxlen];
		for (i=0; p[i] != '\n'; ++i) {
			sms->scds[i] = p[i];
		}
		sms->scds[i] = '\0';
		strcpy(sms->scds, "ACTIVE");
	}
	scdslen = strlen(sms->scds);
	*scds = malloc(scdslen+1);
	if (*scds == NULL) {
		return SMSAllocErr;
	}
	memcpy(*scds, sms->scds, scdslen+1);
	return SMSNoErr;
}	
#else
int getactivescds(SMS* sms, char** scds) {
	size_t scdslen;

	if (!scdsset(sms)) {
		strcpy(sms->scds, "ACTIVE");
	}
	scdslen = strlen(sms->scds);
	*scds = malloc(scdslen+1);
	if (*scds == NULL) {
		return SMSAllocErr;
	}
	memcpy(*scds, sms->scds, scdslen+1);
	return SMSNoErr;
}
#endif

static const char* ftrtxt = "BATSCRW(132) BATSCRD(27) BREDIMAX(3) BDISPMAX(99999999)";
int rundgt(SMS* sms, const char* cmdopts, const char* SCDSOpt) {
	const char* scds;
	const char* genopts[] = { NULL };
	SMSError err;
	char input[4*80];
	const char* ismffmt = "ISPSTART CMD( +\n%s +\n%s('%s')) NEWAPPL(DGT) +\n%s";
	int rc;

	sms->output = NULL;
	scds = sms->opts.prop.val[SMSSCDS];
	sprintf(input, ismffmt, cmdopts, SCDSOpt, scds, ftrtxt);

	rc = batchismf(sms, input, genopts);	
	if (rc == 0) {
		if (sms->opts.verbose) {
			infomsg(sms, SMSISMFErr);
		}
	} else {
		errmsg(sms, SMSISMFErr);
		return SMSISMFErr;
	}
	return SMSNoErr;
}

int genrpt(SMS* sms, const char* rptcmd, const char* rptfields, const char* appl) {
	const char* rptopts[] = { NULL, NULL, NULL };
	SMSError err;
	char input[2*80];
	char rptdd[80];
	char* tmpseq=NULL;
	const char* ismfrptfmt = "ISPSTART CMD( +\n%s) +\n %s %s";
	int rc;

	sms->output = NULL;
	sprintf(input, ismfrptfmt, rptcmd, appl, ftrtxt);

	err = cidneTempSeq(sms, &tmpseq); 
	if (err) { 
		return errmsg(sms, SMSCrtTmpSeq);
	}
	err = writeText(sms, tmpseq, rptfields);
	if (err) { 
		return errmsg(sms, SMSWriteTmpSeq);
	}
	sprintf(rptdd, "--SYSIN=%s", tmpseq);
	rptopts[0] = "--ISPFILE=stdout";
	rptopts[1] = rptdd;
	rc = batchismf(sms, input, rptopts);	
	puts(sms->output);
	if (rc != 0) {
		return errmsg(sms, SMSISMFErr);
	}
	return SMSNoErr;
}				

static int invparse(struct SMS* sms) { 
	return 0; 
}
static int invrunsvc(struct SMS* sms) { 
	return 0; 
}

static SMS invSMS = { invparse, invrunsvc };

static SMSError getenvifset(SMS* sms, const char* env, char** val) {
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

	errmsg(sms, SMSAllocErr);
	return SMSAllocErr;
}

static int getproperty(SMS* sms, const char* key, char** val) {
	int rc;
	size_t vallen;
	const char* xsysvaropts[] = { key, NULL };
	SMSError enverr;

	enverr = getenvifset(sms, key, val);
	if (enverr != SMSEnvVarNotSet) {
		return enverr;
	}

	rc = xsysvar(val, xsysvaropts);
	if (rc) {
		return SMSPropertyErr;
	}

	return SMSNoErr;
}

static int tmphlq(SMS* sms, char** val) {
	const char* TMP_HLQ = "TMPHLQ";
	SMSError enverr;
	const char* hlqopts[] = { NULL };
	int rc;

	enverr = getenvifset(sms, TMP_HLQ, val);
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
	const char* SMS_SCDS = "SMSSCDS";

	rc = tmphlq(sms, &value);
	if (rc) {
		errmsg(sms, SMSSetupErr, "tmphlq", rc);
		return SMSSetupErr;
	}
	sms->opts.prop.val[SMSTMPHLQ] = value;

	rc = getproperty(sms, ISPF_HLQ, &value);
	if (rc) {
		errmsg(sms, SMSPropertyErr, ISPF_HLQ, rc);
		return SMSPropertyErr;
	}
	sms->opts.prop.val[SMSISPFHLQ] = value;

	rc = getproperty(sms, ISMF_HLQ, &value);
	if (rc) {
		errmsg(sms, SMSPropertyErr, ISMF_HLQ, rc);
		return SMSPropertyErr;
	}
	sms->opts.prop.val[SMSISMFHLQ] = value;

	rc = getactivescds(sms, &value);
	if (rc) {
		rc = getproperty(sms, SMS_SCDS, &value);
		if (rc) {
			errmsg(sms, SMSPropertyErr, SMSSCDS, rc);
			return SMSPropertyErr;
		}
	}
	sms->opts.prop.val[SMSSCDS] = value;

	return SMSNoErr;
}

static int inerr(SMS* sms) {
	return sms->err != SMSSyntax && sms->err != SMSNoErr;
}

SMSError runsms(SMS* sms) {
	sms->err = setupBaseEnv(sms);
	if (!inerr(sms)) {
		sms->err = sms->parsearg(sms);
	}
	if (!inerr(sms)) {
 		sms->err = sms->runsvc(sms);
	}
	return sms->err;
}
