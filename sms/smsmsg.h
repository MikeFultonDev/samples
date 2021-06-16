/*********************************************************************
 * Copyright (c) 2021 IBM
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/
#ifndef __SMSMSG__
	#define __SMSMSG__

	typedef struct SMS SMS;

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
		SMSISMFErr=9,
		SMSDetailErr=10
	} SMSError;

	typedef enum {
		SMSNoInfo=0,
		SMSTmpSeqInfo=1,
		SMSDetailInfo=2,
		SMSBatchTSOParmInfo=3
	} SMSInfo;

	int infomsg(SMS* sms, SMSInfo info, ...);
	int errmsg(SMS* sms, SMSError err, ...);
	int syntax(struct SMS* sms);
#endif
