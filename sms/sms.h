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

	typedef enum {
		SMSNoErr=0,
		SMSOptErr=1,
		SMSSyntax=2,
		SMSSetupErr=3,
		SMSAllocErr=4,
		SMSPropertyErr=5,
		SMSEnvVarNotSet=6
	} SMSError;

	typedef enum {
		SMSInvalidProgram=0,
		SMSStorageGroup=1,
		SMSStorageClass=2
	} SMSProgram;

	typedef enum {
		SMSTMPHLQ=0,
		SMSISPFHLQ=1,
		SMSISMFHLQ=2,	
		SMSNumProps=3
	} SMSProps;  

	typedef struct {
		char* val[SMSNumProps];             
	} SMSProperties;

	typedef struct {
		SMSProperties prop;
		int extraarg;
		int verbose:1;
		int syntax:1;
		int list:1;
		int delete:1;
		int create:1;
		int rename:1;
	} SMSOpts;

	typedef struct { char stggrp[8]; } SMSSGOpt;
	typedef struct { char stgcls[8]; } SMSSCOpt;
	typedef struct SMS {
		int (*parsearg)(struct SMS*);	
		int (*runsvc)(struct SMS*);	
		int (*inerr)(struct SMS*);	
		int (*prterr)(struct SMS*);	
		int (*rc)(struct SMS*);	
		int argc;
		char** argv;
		SMSProgram prog;
		int err;
		SMSOpts opts;
	} SMS;

	SMS* crtSMS(SMSProgram prog, int argc, char* argv[]);
#endif
