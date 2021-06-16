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

static int scparse(struct SMS* sms) {
	return parsearg(sms, "cdhlrv");
}

static int scrunsvc(struct SMS* sms) {
	int rc;
	if (!sms->opts.list) {
		return errmsg(sms, SMSISMFErr);
	}
	rc = rundgt(sms, "ACBQBAIF SAVE SCNAMES STORCLAS(*)", "SCDS");
	if (!rc) {
		rc = genrpt(sms, "ACBQBARH SCNAMES", "STORCLAS", "");
	}
	return rc;
}

int main(int argc, char* argv[]) {
	SMS sms = { scparse,  scrunsvc, SMSStorageClass, argc, argv };

	return runsms(&sms);
}
