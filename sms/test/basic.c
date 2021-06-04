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
#include "zoausvc.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
	int rc;
	char* input = "LISTALC";
	char* output;
	char* smsopts[] = { NULL }; 
	char* xsysvaropts[] = { "Color=Black", NULL };
	rc = batchsms(input, &output, smsopts);

	puts(output);
	free(output);

	rc = xsysvar(&output, xsysvaropts);
	puts(output);
	free(output);
}
