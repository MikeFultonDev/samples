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

static int sgparse(struct SMS* sms) {
	return parsearg(sms, "cdhlrv");
}

static int sgrunsvc(struct SMS* sms) {
	int rc;
	if (!sms->opts.list) {
                return errmsg(sms, SMSISMFErr);
 	}
        rc = rundgt(sms, "ACBQBAIG SAVE SGNAMES STORGRP(*) SPACEGB(N)");
        if (!rc) {
	        rc = genrpt(sms, "ACBQBARJ SGNAMES", "STORGRP SGTYPE TOTALSPC FREESPC");
	}
	return rc;
}

int main(int argc, char* argv[]) {
	SMS sms  = { sgparse,  sgrunsvc,  SMSStorageGroup, argc, argv };

	return runsms(&sms);
}
