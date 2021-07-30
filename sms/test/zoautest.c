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
	const char* smsopts[] = { NULL }; 
	const char* xsysvaropts[] = { "Color=Black", NULL };
	const char* hlqopts[] = { NULL };
	rc = batchtso(input, &output, smsopts);

	printf("<%s>\n", output);
	free(output);

	rc = xsysvar(&output, xsysvaropts);
	puts(output);
	free(output);

	rc = hlq(&output, hlqopts);
	puts(output);
	free(output);
}
