/*********************************************************************
 * Copyright (c) 2021 IBM
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/
#include <stdarg.h>
#include <stdio.h>
#include "sms.h"
#include "smsmsg.h"

static const char* errfmt[] = {
	/*  0 */ "0",
	/*  1 */ "1",
	/*  2 */ "2",
	/*  3 */ "3",
	/*  4 */ "4",
	/*  5 */ "5",
	/*  6 */ "6",
	/*  7 */ "7",
	/*  8 */ "8",
	/*  9 */ "9",
	/* 10 */ "Details: %s",
	/* 11 */ "11",
};

static const char* infofmt[] = {
	/*  0 */ "I0",
	/*  1 */ "Temporary Report Dataset: %s\n",
	/*  2 */ "Information Details: %s\n",
	/*  3 */ " %s",
	/*  4 */ "I4",
	/*  5 */ "I5",
	/*  6 */ "I6",
	/*  7 */ "I7",
	/*  8 */ "I8",
	/*  9 */ "I9",
	/* 10 */ "I10",
	/* 11 */ "I11",
};

static const char* pgmname(SMS* sms) {
	switch (sms->prog) {
		case SMSStorageGroup: return "smssg";
		case SMSStorageClass: return "smssc";
	}
        return "inv";
}

static void prtmsg(const char* fmt, va_list args) {
	char cmdbuff[512];
	size_t cmdbuffsz = sizeof(cmdbuff)-1;
	char el[] = "...";
	size_t ellen = sizeof(el)-1;
	int rc;

	rc = vsnprintf(cmdbuff, cmdbuffsz, fmt, args);
	if (rc > cmdbuffsz-ellen) {
		strcpy(&cmdbuff[cmdbuffsz-ellen-1], el);
		rc = cmdbuffsz-1;
	}
	fwrite(cmdbuff, 1, rc, stderr);
}

int infomsg(SMS* sms, SMSInfo info, ...) {
	va_list args;
	va_start(args, info);
	prtmsg(infofmt[info], args);
	va_end(args);

	if (info == SMSDetailInfo) {
		return info;
	}
	if (sms->output) {
		infomsg(sms, SMSDetailInfo, sms->output);
	}
	return info;
}

int errmsg(SMS* sms, SMSError err, ...) {
	va_list args;
	va_start(args, err);
	prtmsg(errfmt[err], args);
	va_end(args);

	if (err == SMSDetailErr) {
		return err;
	}
	if (sms->output) {
		errmsg(sms, SMSDetailErr, sms->output);
	}
	return err;
}

int syntax(struct SMS* sms) {
	fprintf(stderr, "Syntax for %s\n", pgmname(sms));
	fprintf(stderr, "...tbd\n");
	return SMSSyntax;
}
