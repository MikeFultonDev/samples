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

int main(int argc, char* argv[]) {
	SMS* sms;
	int rc, shutrc;

	sms = crtSMS(SMSStorageGroup, argc, argv);
	if (sms->inerr(sms)) {
		sms->prterr(sms);
		return sms->rc(sms);
	}
	sms->runsvc(sms);
	if (sms->inerr(sms)) {
		sms->prterr(sms);
	}
	return sms->rc(sms);
}
