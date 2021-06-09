/*********************************************************************
 * Copyright (c) 2021 IBM
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/
#ifndef __SMS__
	#define __SMS__

	#define SMSMAXDSLEN 44

	typedef enum {
		SMSNoErr=0,
		SMSOptErr=1,
		SMSSyntax=2,
		SMSSetupErr=3,
		SMSAllocErr=4,
		SMSPropertyErr=5,
		SMSEnvVarNotSet=6,
		SMSCrtTmpSeq=7,
		SMSWriteTmpSeq=8,
		SMSISMFErr=9
	} SMSError;

	typedef enum {
		SMSTMPHLQ=0,
		SMSISPFHLQ=1,
		SMSISMFHLQ=2,	
		SMSSCDS=3,	
		SMSNumProps=4
	} SMSProps;  

	typedef enum {
		SMSInvalid=0,
		SMSManagementClass=1,
		SMSDataClass=2,
		SMSStorageClass=3,
		SMSStorageGroup=4,
		SMSAggregateGroup=5,
		SMSACS=6
	} SMSType;

	typedef struct {
		char* val[SMSNumProps];             
	} SMSProperties;

	typedef struct {
		SMSProperties prop;
		char* tmpProfile;
		char* tmpInputTable;
		char* tmpOutputTable;
		int extraarg;
		int verbose:1;
		int syntax:1;
		int list:1;
		int delete:1;
		int create:1;
		int rename:1;
		int translate:1;
		int test:1;
		int validate:1;
		int scds:1;
	} SMSOpts;

	typedef struct { char stggrp[8]; } SMSSGOpt;
	typedef struct { char stgcls[8]; } SMSSCOpt;
	typedef struct SMS {
		int (*parsearg)(struct SMS*);	
		int (*runsvc)(struct SMS*);	
		SMSType prog;
		int argc;
		char** argv;
		char* output;
		int err;
		SMSOpts opts;
		char scds[SMSMAXDSLEN];
	} SMS;

	SMSError runsms(SMS* sms);
	int rundgt(SMS* sms, const char* cmdopts);
	int genrpt(SMS* sms, const char* rptcmd, const char* rptfields);
	int syntax(SMS* sms);
	int errmsg(SMS* sms, SMSError err, ...);
	int parsearg(SMS* sms, const char* validopts);
#endif
