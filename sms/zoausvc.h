/*********************************************************************
 * Copyright (c) 2021 IBM
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/
#ifndef __ZOAUSVC__
	#define __ZOAUSVC__ 1

	int batchtso(const char* input, char** output, const char* usropts[]);
	int xsysvar(char **output, const char* usropts[]);
	int hlq(char **output, const char* usropts[]);
	int dtouch(char **output, const char* usropts[]);
	int mvstmp(char **output, const char* usropts[]);
#endif
