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

static int acsparse(struct SMS* sms) {
	return parsearg(sms, "cdhltrVT", 1, 1);
}
static int acsrunsvc(struct SMS* sms) {
	int rc;
	char opts[3*80];
	const char* acspds = "'SYS1.S0W1.DFSMS.CNTL'";
	const char* acsmem = "STORCLAS";
	const char* acslst = "'IBMUSER.ACS.LISTING'";
	if (!sms->opts.translate) {
		return errmsg(sms, SMSISMFErr);
	}

        sprintf(opts, "ACBQBAO1 +\nACSSRC(%s) MEMBER(%s) +\n LISTNAME(%s)", acspds, acsmem, acslst);
	rc = rundgt(sms, opts, "SCDS");
	return rc;
}


int main(int argc, char* argv[]) {
	SMS sms = { acsparse, acsrunsvc, SMSACS, argc, argv };

	return runsms(&sms);
}
