/*********************************************************************
 * Copyright (c) 2021 IBM
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

#include "//'SYS1.SAMPLIB(CSRSIC)'"
#define _OPEN_SYS_UNLOCKED_EXT 1
#define _XOPEN_SOURCE 1

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>

#define _POSIX_SOURCE

#include<unistd.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>


#define MAX_DSNAME_LEN (44)
#define MAX_RECLEN (32761)
#define FIXED_KEY_SIZE (16)
#define FIXED_VAL_SIZE (16)
#define CLUSTER_QUAL ""
#define KEY_QUAL ".KEY.PATH"

#define PRINT_FIELD_SEP "\t"
#define PRINT_NL  "\n"

const static char DEFAULT_VSAM_CLUSTER[] = "SYS1.XSYSVAR";

struct ihaipa {
	char ipaid[4];
	char _1[28];
	char ipalpnam[8];
	char _2[312];
	char ipasxnam[8];
};

struct psa { 
	char _1[16];
	struct cvtmap* cvt;
};

/*
 * NOTE: This enum ordering can not be changed without changing the
 * corresponding FilterHeader_T and FixedHeader_T structures
 */
typedef enum {
	SysplexField=0,
	SystemField=1,
	ProdIDField=2,
	VerField=3,
	RelField=4,
	ModField=5,
	CommentField=6,

	KSDSKeyField=7,
	KeyField=8,
	ValField=9,

	FirstFilterField=0,
	LastFilterField=6,
	NumFilterFields=7,
	NumFields=10 
} VSAMField_T;

typedef enum {
	NoMatch=1,
	PartialMatch=2,
	FullMatch=3
} KeyMatch_T;

typedef enum {
	Entry=1,
	Exit=2,
	General=3
} MsgType_T;

typedef struct {
	char vsamCluster[MAX_DSNAME_LEN+1];
	size_t argindex[NumFields];
	size_t offset[NumFields];
	size_t len[NumFields];
	int list:1;
	int timing:1;
	int delete:1;
	int get:1;
	int set:1;
	int create:1;
	unsigned int indent:2;
	clock_t cpustart;
	unsigned int wallstart;
} Options_T;

typedef _Packed struct {
	unsigned short offset[NumFilterFields];
	unsigned short len[NumFilterFields];
} FilterHeader_T;


typedef _Packed struct {
	char manufacturer[16];
	char type[4];
	char model[16];
	char plant[4]; 
	char sequence[16];
	char time[32];
} KSDSKey_T;

/*
 * NOTE: The following structure layout CAN NOT be changed without also 
 *       re-generating the VSAM Sphere as well as updating the code for
 *       calculating how to read/write to instances of this structure.
 *       The last byte of the fixed character fields is always 0x00.
 */
typedef _Packed struct {
	KSDSKey_T ksdskey;
	char key[FIXED_KEY_SIZE];
	char val[FIXED_VAL_SIZE];
	unsigned short keyXOffset;
	unsigned short keyXLen;
	unsigned short valXOffset;
	unsigned short valXLen;
	unsigned short filterXOffset;
	unsigned short filterXLen;
} FixedHeader_T;

#define KSDSKEYNAME "KSDSKEY"
#define KEYNAME     "KEY"

#define RECLEN (sizeof(FixedHeader_T))
#define KSDSKEYLEN (sizeof(KSDSKey_T))
#define KSDSKEYOFF (offsetof(FixedHeader_T, ksdskey))
#define KEYLEN (FIXED_KEY_SIZE)
#define KEYOFF (offsetof(FixedHeader_T, key))

static int syntax(const char* prog) {
	fprintf(stderr, "%s [-?hldctCXSPVRMD] <key>[=<val>]\n", prog);
	fprintf(stderr, "Where:\n");
	fprintf(stderr, " -?|-h: show this help\n");
	fprintf(stderr, " Filters can be specified for set/get/list\n");
	fprintf(stderr, "  -X<sysplex>: Sysplex <sysplex> specific\n");
	fprintf(stderr, "  -S<system>: System <system> specific\n");
	fprintf(stderr, "  -P<prod>: Product <prod> specific\n");
	fprintf(stderr, "  -G<group>: -G is a synonym for -P\n");
	fprintf(stderr, "  -V<ver>: Version <ver> of product <prod> specific. Requires -P or -G to be specified\n");
	fprintf(stderr, "  -R<rel>: Release <rel> of product <prod> specific. Requires -P or -G to be specified\n");
	fprintf(stderr, "  -M<mod>: Modification <mod> of product <prod> specific. Requires -P or -G to be specified\n");
	fprintf(stderr, " Listing multiple keys by filter:\n");
	fprintf(stderr, "  -l: list the filters, key, and value that match the filter request\n");
	fprintf(stderr, "      the list is by key, e.g. -l <key>\n");
	fprintf(stderr, "      additional filters can be specified to restrict the listing\n");
 	fprintf(stderr, "      Each line is of the format:<sysplex>\t<system>\t<prod>\t<ver>\t<rel>\t<mod>\t<val>\t<comment>\n");
 	fprintf(stderr, "      If no filters or key is specified, all records will be listed\n");
	fprintf(stderr, " Other options:\n");
	fprintf(stderr, "  -C<comment>: Comment <comment> specified\n");
	fprintf(stderr, "  -D<database-hlq>: use database (VSAM dataset) with prefix <database-hlq>. Default is %s\n", DEFAULT_VSAM_CLUSTER);
	fprintf(stderr, "  -d: Delete key (with optional fields)\n");
	fprintf(stderr, "  -t: Print out timing information\n");
	fprintf(stderr, "  -c: Create database\n");
	fprintf(stderr, "Note:\n");
	fprintf(stderr, " The combined length of the key, value, and filters must be less than 32K bytes\n");
	fprintf(stderr, "Examples:\n");
	fprintf(stderr, " Create database using default high-level qualifier %s\n", DEFAULT_VSAM_CLUSTER);
	fprintf(stderr, "  %s -c\n");
	fprintf(stderr, " Set key/value pair for JAVA_HOME globally\n");
	fprintf(stderr, "  %s JAVA_HOME=/usr/lpp/java/current_64\n", prog);
	fprintf(stderr, " Set key/value pair for JAVA_HOME for SYSPLEX plex, SYSTEM S0W1\n");
	fprintf(stderr, "  %s -Xplex -SS0W1 JAVA_HOME=/usr/local/devline_64\n", prog);
	fprintf(stderr, " Set key/value pair for JAVA64_HOME associated with product-prefix AJV, Version 8, Release 0\n");
	fprintf(stderr, "  %s -PAVJ -V8 -R0 JAVA64_HOME=/usr/lpp/java/J8.0_64\n", prog);
	fprintf(stderr, " Set key/value pair for CSI associated with product prefix IGY, Version 6, Release 3, Modification 0\n");
	fprintf(stderr, "  %s -PIGY -V6 -R3 -M0 CSI=SMPE.IGY630.CSI\n", prog);
	fprintf(stderr, " Set key/value pair for CSI associated with product prefix IGY, Version 6, Release 2, Modification 0\n");
	fprintf(stderr, "  %s -PIGY -V6 -R2 -M0 CSI=SMPE.IGY620.CSI\n", prog);
	fprintf(stderr, " Set key/value pair for CSI associated with product prefix ZOS, Version 2, Release 4, Modification 0\n");
	fprintf(stderr, "  %s -PZOS -V2 -R2 -M0 CSI=SMPE.ZOS240.CSI\n", prog);
	fprintf(stderr, " Set key/value pair for HLQ associated with product prefix IGY\n");
	fprintf(stderr, "  %s -PIGY '-CActive COBOL compiler HLQ' HLQ=IGY630\n", prog);
	fprintf(stderr, " Assuming the previous commands have been issued\n");
	fprintf(stderr, " Get key/value pair for JAVA_HOME from SYSPLEX plex, SYSTEM S0W2\n");
	fprintf(stderr, "  %s JAVA_HOME <-- returns /usr/lpp/java/current_64\n", prog);
	fprintf(stderr, " Get key/value pair for JAVA_HOME from SYSPLEX plex, SYSTEM S0W1\n");
	fprintf(stderr, "  %s -Xplex -SS0W1 JAVA_HOME <-- returns /usr/local/devline_64\n", prog);
	fprintf(stderr, " Get key/value pairs for all CSIs\n");
	fprintf(stderr, "  %s -l CSI <-- returns\n", prog);
	fprintf(stderr, "   \t\tIGY\t6\t3\t0\tCSI\tSMPE.IGY630.CSI\t\n");
	fprintf(stderr, "   \t\tIGY\t6\t2\t0\tCSI\tSMPE.IGY620.CSI\t\n");
	fprintf(stderr, "   \t\tZOS\t2\t2\t0\tCSI\tSMPE.ZOS240.CSI\t\n");

	return 1;
}

static int setCluster(Options_T* opt, const char* hlq) {
	size_t hlqlen = strlen(hlq);
	int i;
	if (hlqlen > MAX_DSNAME_LEN) {
		fprintf(stderr, "Prefix %s is too long\n", hlq);
		return 16;
	}
	memcpy(opt->vsamCluster, hlq, hlqlen+1);
	for (i=0; i<hlqlen; ++i) {
		opt->vsamCluster[i] = toupper(opt->vsamCluster[i]);
	}
	return 0;
}

static int setField(Options_T* opt, const char** argv, int i, size_t offset, VSAMField_T field) {
	size_t len = strlen(&argv[i][offset]);
        if (len > MAX_RECLEN) {          
                fprintf(stderr, "Field is too long (limit is %d)\n", MAX_RECLEN);
                return 16;              
        }                       
	opt->argindex[field] = i;
	opt->offset[field] = offset;
	opt->len[field] = len;

        return 0;         		
}

static int hasFilter(Options_T* opt, VSAMField_T field) {
	return (opt->len[field] > 0);
}

static const char* optstr(const char **argv, Options_T* opt, VSAMField_T field) {
	return &argv[opt->argindex[field]][opt->offset[field]];
}

static size_t optlen(Options_T* opt, VSAMField_T field) {
	return opt->len[field];
}

/*
 * the following fractional portion of 'about' one second wraps every 1.048576 seconds  
 */
#define STCK_BIT31_TIME_SECS (1.048576)
static unsigned int fracsec() {
	char time[16];
	char* ptime = time;
	unsigned int fsec;

	__asm(" STCK %0" : "=m"(*ptime) : : );
	fsec = *((unsigned int*) &ptime[4]);
	return fsec;
}

static int processArgs(int argc, const char** argv, Options_T* opt) {
	int i, rc;

	setCluster(opt, DEFAULT_VSAM_CLUSTER);

	for (i=1; i<argc; ++i) {
		const char* arg = argv[i];
		if (arg[0] == '-') {
			switch (arg[1]) { 
				case 'h':
				case '?':
					rc = syntax(argv[0]);
					break;
				case 'C':
					rc = setField(opt, argv, i, 2, CommentField);
					break;
				case 'X':
					rc = setField(opt, argv, i, 2, SysplexField);
					break;
				case 'S':
					rc = setField(opt, argv, i, 2, SystemField);
					break;
				case 'V':
					rc = setField(opt, argv, i, 2, VerField);
					break;
				case 'R':
					rc = setField(opt, argv, i, 2, RelField);
					break;
				case 'M':
					rc = setField(opt, argv, i, 2, ModField);
					break;
				case 'P':
				case 'G':
					rc = setField(opt, argv, i, 2, ProdIDField);
					break;
				case 'D':
					rc = setCluster(opt, &arg[2]);
					if (rc) {		
						return rc;
					}
					break;
				case 'l':
					opt->list = 1;
					break;
				case 'c':
					opt->create = 1;
					break;
				case 'd':
					opt->delete = 1;
					break;
				case 't':
					opt->timing = 1;
					opt->cpustart = clock();
					opt->wallstart = fracsec();
					printf("Times printed are CPU time then wall clock time, in seconds\n");
					printf("Total wall clock time should not exceed 1s. Wall clock times will be wrong otherwise\n");
					break;
				default:
					fprintf(stderr, "Unknown option:%s\n", arg);
					return(syntax(argv[0]));
					break;
			}
		} else {	
			char* eqp;
			int len = strlen(arg);
			if (opt->set || opt->get) {
				fprintf(stderr, "Can only specify one key to get or set. %s should not be specified\n", arg);
				return 8;
			}
			eqp = strchr(arg, '=');
			opt->argindex[KeyField] = i;
			opt->offset[KeyField] = 0;
			if (eqp) {
				opt->set = 1;
				opt->argindex[ValField] = i;
				opt->offset[ValField] = (eqp-arg)+1;
				opt->len[KeyField] = (eqp-arg);
				opt->len[ValField] = len-opt->offset[ValField];
			} else {
				opt->get = 1;
				opt->len[KeyField] = len;
			}
		}
	}

	if (!hasFilter(opt, ProdIDField)) {
		if (hasFilter(opt, VerField) || hasFilter(opt, RelField) || hasFilter(opt, ModField)) {
			fprintf(stderr, "-V, -R, and -M can only be specified if -P or -G is specified\n");
			return 8;
		}
	}

	if (opt->create && (opt->list || opt->delete || opt->set || opt->get)) {
		fprintf(stderr, "-c can not be specified when listing, getting, setting, or deleting a variable\n");
		return 8;
	}
	if (opt->set && opt->list) {
		fprintf(stderr, "-l can not be specified when setting a variable\n");
		return 8;
	}
	if (opt->delete && opt->list) {
		fprintf(stderr, "-l can not be specified when deleting a variable\n");
		return 8;
	}
	if (opt->set && opt->delete) {
		fprintf(stderr, "You can not set and delete a variable at the same time\n");
		return 8;
	}
	return 0;
}

static int hasKey(FixedHeader_T* hdr) {
 	return hdr->key[0] != '\0';
}

static char* getBuffer(const FixedHeader_T* hdr) {
	return ((char*) hdr) + sizeof(FixedHeader_T);
}

static FilterHeader_T* getFilterHeader(FixedHeader_T* hdr) {
	char* buff = getBuffer(hdr);
	FilterHeader_T* filter = (FilterHeader_T*) (&buff[hdr->filterXOffset]);
	return filter;
}

static size_t computebufflen(FixedHeader_T* hdr) {
	size_t len = hdr->keyXLen + hdr->valXLen + hdr->filterXLen;
	VSAMField_T f;
	if (hdr->filterXLen > 0) {
		FilterHeader_T* filter = getFilterHeader(hdr);
		for (f=FirstFilterField; f<=LastFilterField; ++f) {
			len += filter->len[f];
		}
	}
	return len;
}
		
static int setxfield(FixedHeader_T* hdr, unsigned short* xoffsetp, unsigned short* xlenp, unsigned short xlen, const void* xfield) {
	char* buff = getBuffer(hdr);
	size_t bufflen;

	bufflen = computebufflen(hdr);
	if (bufflen + xlen > MAX_RECLEN) {
		fprintf(stderr, "Total length of new record would exceed maximum record length. copy failed\n");
		return 0;
	}
	*xoffsetp = bufflen;
	*xlenp = xlen;

	memcpy(&buff[*xoffsetp], xfield, xlen);
	return xlen;
}

static unsigned short* filterOffset(FixedHeader_T* hdr, VSAMField_T field) {
	FilterHeader_T* fh = getFilterHeader(hdr);
	return &fh->offset[field];
}

static unsigned short* filterLen(FixedHeader_T* hdr,VSAMField_T field) {
	FilterHeader_T* fh = getFilterHeader(hdr);
	return &fh->len[field];
}

static int setffield(FixedHeader_T* hdr, const void* xfield, unsigned short xlen, VSAMField_T field) {
	FilterHeader_T newFilter = { 0 };
	unsigned short* xoffsetp;
	unsigned short* xlenp;
	size_t bufflen;
	size_t inclen=0;

	bufflen = computebufflen(hdr);
	if (hdr->filterXLen == 0) {
		bufflen += sizeof(FilterHeader_T);
	}

	if (bufflen + xlen > MAX_RECLEN) {
		fprintf(stderr, "Total length of new record would exceed maximum record length. copy failed\n");
		return 0;
	}
	if (hdr->filterXLen == 0) {
		inclen += setxfield(hdr, &hdr->filterXOffset, &hdr->filterXLen, sizeof(newFilter), &newFilter);
	}
	xoffsetp = filterOffset(hdr, field);
	xlenp = filterLen(hdr, field);
	inclen += setxfield(hdr, xoffsetp, xlenp, xlen, xfield);
	return inclen;
}

static size_t setfield(FixedHeader_T* hdr, const char* field, size_t len, VSAMField_T fieldName) {
        switch (fieldName) {
		case KSDSKeyField:
			memcpy(&hdr->ksdskey, field, len);
			return 0;
                        break;
		case KeyField:
			if (len >= FIXED_KEY_SIZE) {
				memcpy(hdr->key, field, FIXED_KEY_SIZE-1);
				return setxfield(hdr, &hdr->keyXOffset, &hdr->keyXLen, len-FIXED_KEY_SIZE+1, &field[FIXED_KEY_SIZE-1]);
			} else {
				memcpy(hdr->key, field, len);
				return 0;
			}
                        break;
		case ValField:
			if (len >= FIXED_VAL_SIZE) {
				memcpy(hdr->val, field, FIXED_VAL_SIZE-1);
				return setxfield(hdr, &hdr->valXOffset, &hdr->valXLen, len-FIXED_VAL_SIZE+1, &field[FIXED_VAL_SIZE-1]);
			} else {
				memcpy(hdr->val, field, len);
				return 0;
			}
                        break;
		case CommentField:
		case SysplexField:
		case SystemField:
		case ProdIDField:
		case VerField:
		case RelField:
		case ModField:
			return setffield(hdr, field, len, fieldName);
			break;
                default:
                        fprintf(stderr, "Internal error. setfield called with unnexpected field %d\n", field);
	                exit(16);
        }
}

static int cmpxfield(FixedHeader_T* hdr, Options_T* opt, unsigned short xoffsetp, unsigned short xlen, unsigned short userlen, const char* userkey) {
	char* buff = getBuffer(hdr);
	if (userlen == 0 && opt->list) {
		return FullMatch;
	}

	if (xlen != userlen) {
		return PartialMatch;
	}

	if (!memcmp(&buff[xoffsetp], userkey, userlen)) {
		return FullMatch;
	} else {
		return PartialMatch;
	}
}

static int cmpfixedfield(FixedHeader_T* hdr, Options_T* opt, const char* vsamfield, const char* userfield, size_t userlen) {
	if (userlen == 0 && opt->list) {
		return FullMatch;
	}
	if (!memcmp(vsamfield, userfield, userlen) && (vsamfield[userlen] == '\0')) {
		return FullMatch;
	} else {
		return NoMatch;
	}
}

static KeyMatch_T cmpkeyfield(FixedHeader_T* hdr, Options_T* opt, const char* optStr, size_t optLen, VSAMField_T keyName) {
	KeyMatch_T fieldCheck;
	unsigned short xOffset; 
	unsigned short xLen;
	const char* fixedField;
	unsigned short fixedLen;
	unsigned short fixedCmpLen;

	switch(keyName) {
		case KeyField:
			fixedField = hdr->key;
			fixedLen = FIXED_KEY_SIZE;
			xOffset = hdr->keyXOffset;
			xLen = hdr->keyXLen;
			break;
		default:
			fprintf(stderr, "Internal Error: cmpkeyfield Unexpected key passed in:%d\n", keyName);
			exit(16);
	}
	fixedCmpLen = (optLen < fixedLen) ? optLen : fixedLen-1;

	fieldCheck = cmpfixedfield(hdr, opt, fixedField, optStr, fixedCmpLen);
	if (fieldCheck != FullMatch) {
		return fieldCheck;
	}
	if (optLen >= fixedLen) {
		fieldCheck = cmpxfield(hdr, opt, xOffset, xLen, optLen-fixedLen+1, &optStr[fixedLen-1]);
	}
	return fieldCheck;
}

static KeyMatch_T cmpfilterfield(FixedHeader_T* hdr, Options_T* opt, const char* userfield, size_t userlen, VSAMField_T field) {
	unsigned short len;
	unsigned short offset;
	char* buff;
	char* vsamfield;

        if (userlen == 0 && opt->list) {
	        return FullMatch;
	}          
	if (hdr->filterXLen == 0) {
		if (userlen != 0) {
			return PartialMatch; 
		} else {
			return FullMatch;
		}
	}

	len = *filterLen(hdr, field);
	offset = *filterOffset(hdr, field);
	buff = getBuffer(hdr);
	vsamfield = &buff[offset];

	if (userlen != len) {
		return PartialMatch;
	}
	if (!memcmp(vsamfield, userfield, userlen)) {
		return FullMatch;
	} else {
                return PartialMatch;
	}
}

/*
 * Check the passed in optStr/optLen against the VSAM key (KeyField)
 * If there is no match, or a partial match, then return the result (a partial match will cause another record to be read)
 * If there is a full match, check the filters. 
 *   - For a key, this is the extended filters
 * If a filter doesn't match, map it to a partial match so that additional records will be read
 */
static KeyMatch_T vsamkeymatch(FixedHeader_T* hdr, Options_T* opt, const char** argv, const char* optStr, size_t optLen, VSAMField_T fieldName) {
	KeyMatch_T fieldCheck;
	VSAMField_T f;
	switch (fieldName) {
		case KeyField: 
			fieldCheck = cmpkeyfield(hdr, opt, optStr, optLen, KeyField);
			if (fieldCheck != FullMatch) {
				return fieldCheck;
			}
			if (fieldCheck != FullMatch) {
				return fieldCheck;
			}
			break;

		default:
			fprintf(stderr, "Internal Error: vsamkeymatch Unexpected field passed in:%d\n", fieldName);
			exit(16);
	}

	for (f=FirstFilterField; f<=LastFilterField; ++f) {
		if (f == CommentField) continue;   
		fieldCheck = cmpfilterfield(hdr, opt, optstr(argv, opt, f), optlen(opt,f), f);
		if (fieldCheck != FullMatch) {
			return fieldCheck;
		}
	}
	return fieldCheck;
}
 

static void print_timing(Options_T* opt, MsgType_T type, const char* msg) {
	const char blanks[8] = "        ";
	clock_t cputime = clock(); /* time returned is in micro-seconds */
	unsigned long cpudiff = cputime - opt->cpustart;
	double cpuprt;
	unsigned int walltime = fracsec(); /* time returned is in fractional seconds (roughly) */
	double walldiff;
	double wallprt;
	const double maxwall = (double) 0xFFFFFFFF;
	
	if (walltime > opt->wallstart) {
		walldiff = (double)(walltime - opt->wallstart);
	} else {
		walldiff = maxwall - ((double)opt->wallstart) + ((double)walltime);
	}
	wallprt = walldiff / maxwall * STCK_BIT31_TIME_SECS;
	cpuprt = ((double) cpudiff) / ((double) CLOCKS_PER_SEC);
	switch (type) {
		case Entry: 
			printf("%.*s>>>%s %lf %lf\n", opt->indent, blanks, msg, cpuprt, wallprt);	
			opt->indent++;
			break;
		case Exit: 
			opt->indent--;
			printf("%.*s<<<%s %lf %lf\n", opt->indent, blanks, msg, cpuprt, wallprt);	
			break;
		case General:
			printf("%.*s%s %lf %lf\n", opt->indent, blanks, msg, cpuprt, wallprt);	
			break;
	}
}
#pragma noinline(print_timing)

static void timing(Options_T* opt, MsgType_T type, const char* msg) {
	if (!opt->timing) {         
		return;
	}
	print_timing(opt, type, msg);
}

static FILE* vsamopen_internal(Options_T* opt, const char* dataset, const char* qual, const char* fmt, int verbose) {
	char mvsname[MAX_DSNAME_LEN+5];
	FILE* fp;
	int saveerrno;

	timing(opt, Entry, "vsamopen");
	if (strlen(dataset) + strlen(qual) > MAX_DSNAME_LEN) {
		fprintf(stderr, "VSAM Cluster key/value dataset name invalid: %s\n", dataset);
		return NULL;
	}
	sprintf(mvsname, "//'%s%s'", dataset, qual);
	fp=fopen(mvsname, fmt);
	if (!fp && verbose) {
		saveerrno=errno;
		fprintf(stderr, "Unable to open VSAM dataset %s mode %s\n", mvsname, fmt);
		errno=saveerrno;
		perror(mvsname);
	}
	timing(opt, Exit, "vsamopen");
	return fp;
}

static FILE* vsamopen(Options_T* opt, const char* dataset, const char* qual, const char* fmt) {
	return vsamopen_internal(opt, dataset, qual, fmt, 1);
}

static size_t vsamread(Options_T* opt, void* buff, size_t numbytes, FILE* fp) {
	int saveerrno;
	size_t rc;

	timing(opt, Entry, "vsamread");
	rc=fread(buff, 1, numbytes, fp);
	if (rc == 0 && !feof(fp)) {
		saveerrno=errno;
		fprintf(stderr, "Unable to read from VSAM dataset\n");
		errno=saveerrno;
		perror("fread");
	}
	timing(opt, Exit, "vsamread");
	return rc;
}

static int vsamwrite(Options_T* opt, const char* buff, size_t numbytes, FILE* fp) {
	int saveerrno;
	size_t rc;

	timing(opt, Entry, "vsamwrite");
	rc=fwrite(buff, 1, numbytes, fp);
	if (rc != numbytes) {
		__amrc_type save_amrc = *__amrc;
		unsigned int* rplfdbwd = (unsigned int*) save_amrc.__rplfdbwd;
		saveerrno=errno;
		fprintf(stderr, "last fwrite errno=%d rplfdbwd=%X, lastop=%d syscode=%X rc=%d\n",
			   errno,
			   *rplfdbwd,
			   save_amrc.__last_op,
			   save_amrc.__code.__abend.__syscode,
			   save_amrc.__code.__abend.__rc);
		fprintf(stderr, "Unable to write to VSAM dataset. Expected to write %d bytes but wrote %d bytes\n", numbytes, rc);
		errno=saveerrno;
		perror("fwrite");
	}
	timing(opt, Exit, "vsamwrite");
	return rc;
}

static int vsamdelrec(Options_T* opt, FILE* fp) {
	int rc;
	timing(opt, Entry, "vsamdelrec");
	rc = fdelrec(fp);
	timing(opt, Exit, "vsamdelrec");
	return rc;
}
	
static FixedHeader_T* vsamxlocate_internal(FILE* fp, char* buff, const char** argv, Options_T* opt, VSAMField_T fieldName, size_t* reclen) {
	FixedHeader_T* hdr = (FixedHeader_T*) buff;	
	char* vsamfield;
	const char* userfield;
	size_t vsamfieldlen;
	size_t userlen;
	size_t fixedlen;
	int rc;
	KeyMatch_T result = PartialMatch;

	memset(hdr, 0, sizeof(FixedHeader_T));
	*reclen = 0;
	switch (fieldName) {
		case KeyField:
			vsamfield = hdr->key;
			userfield = optstr(argv, opt, KeyField);
			userlen = optlen(opt,KeyField);
			fixedlen = FIXED_KEY_SIZE;
			break;
		default:
			fprintf(stderr, "Internal Error: vsamxlocate Unexpected key other than KEY passed in:%d\n", fieldName);
			exit(16);
	}
	vsamfieldlen = (userlen >= fixedlen) ? fixedlen-1 : userlen;
	memcpy(vsamfield, userfield, vsamfieldlen);
	rc = flocate(fp, vsamfield, fixedlen-1, __KEY_EQ);
	if (rc) {
		return NULL;
	}
	while (result == PartialMatch) {
		rc = vsamread(opt, hdr, MAX_RECLEN, fp);
		if (rc == 0) {
			if (!feof(fp)) {
				fprintf(stderr, "Internal Error: Unable to read record after flocate of %s successful\n", vsamfield);
			}
			return NULL;
		}
		result = vsamkeymatch(hdr, opt, argv, userfield, userlen, fieldName);
		if (result == NoMatch) {
			hdr = NULL;
		}
	}
	*reclen = rc;
	return hdr;
}

static FixedHeader_T* vsamxlocate(FILE* fp, char* buff, const char** argv, Options_T* opt, VSAMField_T fieldName, size_t* reclen) {
	FixedHeader_T* hdr;
	timing(opt, Entry, "vsamxlocate");
	hdr = vsamxlocate_internal(fp, buff, argv, opt, fieldName, reclen);
	timing(opt, Exit, "vsamxlocate");
	return hdr;
}

static int vsamclose(Options_T* opt, FILE* fp) {
	int saveerrno;
	int rc;
	timing(opt, Entry, "vsamclose");
	rc = fclose(fp);
	if (rc) {
		saveerrno=errno;
		fprintf(stderr, "Unable to close VSAM dataset\n");
		errno=saveerrno;
		perror("fclose");
	}
	timing(opt, Exit, "vsamclose");
	return rc;
}

static int printxfield(const FixedHeader_T* hdr, const char* fixed, size_t fixedlen, size_t offset, size_t xlen, const char* sep) {
	const char* buff = getBuffer(hdr);
	int rc;

	if (xlen > 0) {	
		rc = printf("%s%.*s%s", fixed, xlen, &buff[offset], sep);
	} else {
		rc = printf("%s%s", fixed, sep);
	}
	return rc;
}

static int printffield(FixedHeader_T* hdr, VSAMField_T field, const char* sep) {
	const char* buff = getBuffer(hdr);
	int rc;

	if (hdr->filterXLen == 0) {
		return printf("%s", sep);
	} else {
		unsigned short len = *filterLen(hdr, field);
		unsigned short offset = *filterOffset(hdr, field);
		return printf("%.*s%s", len, &buff[offset], sep);
	}
}
	
static int printfield(FixedHeader_T* hdr, VSAMField_T field, const char* sep) {
	int rc;	
	switch (field) {
                case KeyField:
			rc = printxfield(hdr, hdr->key, FIXED_KEY_SIZE-1, hdr->keyXOffset, hdr->keyXLen, sep);
	          	break;
                case ValField:
			rc = printxfield(hdr, hdr->val, FIXED_VAL_SIZE-1, hdr->valXOffset, hdr->valXLen, sep);
	          	break;
		case CommentField:
		case SysplexField:
		case SystemField:
		case ProdIDField:
		case VerField:
		case RelField:
		case ModField:
			rc = printffield(hdr, field, sep);
	          	break;
		default:
                        fprintf(stderr, "Internal error. Unknown field: %d for printfield\n", field);
	                exit(16);
        }
	return rc;
}

static int printfields(FixedHeader_T* hdr) {
	int rc;
	int totrc = 0;
	VSAMField_T f;

	for (f=FirstFilterField; f<=LastFilterField; ++f) { 
		if (f == CommentField) continue; 
		rc = printfield(hdr, f, PRINT_FIELD_SEP);
		if (rc <= 0) { return rc; }
		totrc += rc;
	}

	rc = printfield(hdr, KeyField, PRINT_FIELD_SEP);
	if (rc <= 0) { return rc; }
	totrc += rc;

	rc = printfield(hdr, ValField, PRINT_FIELD_SEP);
	if (rc <= 0) { return rc; }
	totrc += rc;

	rc = printfield(hdr, CommentField, PRINT_NL);
	if (rc <= 0) { return rc; }
	totrc += rc;

	return totrc;
}

static char* reverse(char* time) {
	int i;
	for (i=0; i<8; ++i) {
		char tmp = time[i];
		time[i] = time[15-i];
		time[15-i] = tmp;    
	}
	return time;
}

static char nibble2x(unsigned char in) {
	return (in < 10 ? '0' + in : 'A' + (in-10));
}

static char* fmttime(const char* time, char* buffer) {
	int i;
	int o=0;
	for (i=0; i<16; ++i) {
		unsigned char nibble;
		nibble = ((unsigned char) time[i]) >> 4;
		buffer[o++] = nibble2x(nibble);
		nibble = ((unsigned char) time[i]) & 0xF;
		buffer[o++] = nibble2x(nibble);
	}
	return buffer;
}

static int ksdskey(KSDSKey_T* key) {
	CSRSIRequest     req;
	CSRSIInfoAreaLen len;
	CSRSIReturnCode  rc;
	siv1             v1cpc_machine;
	char             hextime[32+1];
	char             time[16];
	char*            ptime;

	unsigned char*   manufacturer;
	unsigned char*   type;
	unsigned char*   model;
	unsigned char*   plant;
	unsigned char*   sequence;
 
	req = CSRSI_REQUEST_V1CPC_MACHINE;
	len = sizeof(siv1);
	csrsi_byaddr(req, len, &v1cpc_machine, &rc);
	manufacturer = v1cpc_machine.siv1si11v1.si11v1cpcmanufacturer;
	type = v1cpc_machine.siv1si11v1.si11v1cpctype;
	model = v1cpc_machine.siv1si11v1.si11v1cpcmodel;
	plant = v1cpc_machine.siv1si11v1.si11v1cpcplantofmanufacture;
	sequence = v1cpc_machine.siv1si11v1.si11v1cpcsequencecode;

	ptime = time;

	__asm(" STCKE %0" : "=m"(*ptime) : : );

	reverse(time);
	fmttime(time, hextime);

	memcpy(key->manufacturer, manufacturer, sizeof(key->manufacturer));
	memcpy(key->type, type, sizeof(key->type));
	memcpy(key->model, model, sizeof(key->model));
	memcpy(key->plant, plant, sizeof(key->plant));
	memcpy(key->sequence, sequence, sizeof(key->sequence));
	memcpy(key->time, hextime, sizeof(key->time));

	return 0;
}

static int createInitialRecord(FixedHeader_T* fh) {
        KSDSKey_T primarykey;
	char key[FIXED_KEY_SIZE+1] = "xsysvar";
	char val[FIXED_VAL_SIZE+1] = "1.0.0";
        
        memset(fh, 0, sizeof(FixedHeader_T));
        ksdskey(&primarykey);
        setfield(fh, (const char*) &primarykey, sizeof(KSDSKey_T), KSDSKeyField);
        setfield(fh, key, FIXED_KEY_SIZE, KeyField);
        setfield(fh, val, FIXED_VAL_SIZE, ValField);
        
	return sizeof(FixedHeader_T);
}

static int setRecord(char* buff, size_t *reclen, const char** argv, Options_T* opt) {
	FixedHeader_T* fh = (FixedHeader_T*) buff;
	size_t extlen=0;
	VSAMField_T f;
	KSDSKey_T primarykey;
  
	memset(fh, 0, sizeof(FixedHeader_T));
	for (f=FirstFilterField; f<=LastFilterField; ++f) {
		if (hasFilter(opt, f)) {
			extlen += setfield(fh, optstr(argv, opt, f), optlen(opt, f), f);
		}
	}
	ksdskey(&primarykey);
	extlen += setfield(fh, (const char*) &primarykey, sizeof(KSDSKey_T), KSDSKeyField);
	extlen += setfield(fh, optstr(argv, opt, KeyField), optlen(opt,KeyField), KeyField);
	extlen += setfield(fh, optstr(argv, opt, ValField), optlen(opt,ValField), ValField);

	*reclen = (sizeof(FixedHeader_T) + extlen);

	return 0;
}

static int getKey(const char** argv, Options_T* opt) {
	FILE* vsamfp;
	int rc;
	FixedHeader_T* hdr;
	size_t reclen;
	char buff[MAX_RECLEN];

	vsamfp = vsamopen(opt, opt->vsamCluster, KEY_QUAL, "rb,type=record");
	if (!vsamfp) {
		return 16;
	}
	hdr = vsamxlocate(vsamfp, buff, argv, opt, KeyField, &reclen);
	if (!hdr) {
		return 1;
	}
	rc = printfield(hdr, ValField, PRINT_NL);
	if (rc <= 0) {
		return 8;
	}
	rc = vsamclose(opt, vsamfp);
	if (rc) {
		return 12;
	}
	return 0;
}

static int deleteKey(const char** argv, Options_T* opt) {
	FILE* vsamkfp;
	int rc;
	FixedHeader_T* hdr;
	size_t reclen;
	char buff[MAX_RECLEN];

	vsamkfp = vsamopen(opt, opt->vsamCluster, KEY_QUAL, "rb+,type=record");
	if (!vsamkfp) {
		return 16;
	}
	hdr = vsamxlocate(vsamkfp, buff, argv, opt, KeyField, &reclen);
	if (!hdr) {
		return 1;
	}
	rc = vsamdelrec(opt, vsamkfp);
	if (rc) {
		return 8;
	}
	rc = vsamclose(opt, vsamkfp);
	if (rc) {
		return 10;
	}
	return 0;
}

static int listEntriesByKey(const char** argv, Options_T* opt, VSAMField_T keyfield) {
	FILE* fp;
	int rc;
	FixedHeader_T* hdr;
	size_t reclen;
	char buff[MAX_RECLEN];
	KeyMatch_T result = FullMatch;
	const char* qual;

	switch (keyfield) {
		case KeyField: 
			qual=KEY_QUAL;
			break;
		default:
                        fprintf(stderr, "Internal error. Unknown field: %d for printfield\n", keyfield);
	                exit(16);
	}

	fp = vsamopen(opt, opt->vsamCluster, qual, "rb,type=record");
	if (!fp) {
		return 16;
	}
	hdr = vsamxlocate(fp, buff, argv, opt, keyfield, &reclen);
	while (hdr) {
		const char* key = optstr(argv, opt, keyfield);
		unsigned short keyLen = optlen(opt, keyfield);

		/*
		 * Do not print the dummy NULL record
		 */
		if (hasKey(hdr)) {
			rc = printfields(hdr);
			if (rc <= 0) {
				return 8;
			}
		}
		do {
			rc = vsamread(opt, hdr, MAX_RECLEN, fp);
			if (rc == 0) {
				if (!feof(fp)) {
					fprintf(stderr, "Internal Error: Unable to read record after flocate/fread of %s successful\n", key);
				}
				result = NoMatch;
			} else {
				result = vsamkeymatch(hdr, opt, argv, key, keyLen, keyfield);
			}
		} while (result == PartialMatch);
		if (result == NoMatch) {
			hdr = NULL;
		}
	}
	rc = vsamclose(opt, fp);
	if (rc) {
		return 12;
	}
	return 0;
}

static int listAllEntries(const char** argv, Options_T* opt) {
	FILE* fp;
	int rc;
	FixedHeader_T* hdr;
	size_t reclen;
	char buff[MAX_RECLEN];
	const char* qual = KEY_QUAL;

        fp = vsamopen(opt, opt->vsamCluster, qual, "rb,type=record");
	if (!fp) {
                return 16;
	}
	hdr = (FixedHeader_T*) buff;

	do {
		rc = vsamread(opt, hdr, MAX_RECLEN, fp);
		if (rc == 0) {
			if (!feof(fp)) {
				fprintf(stderr, "Internal Error: Unable to read record after fopen/fread of %s successful\n", qual);
				rc = 10;
				break;
			}
		} else {
			/*
			 * Do not print the dummy NULL record
			 */
			if (hasKey(hdr)) {
				rc = printfields(hdr);
				if (rc <= 0) {
					break;
				}
			}
		}
	} while (!feof(fp)); 
        rc = vsamclose(opt, fp);
	if (rc) {
                return 12;
	}
        return 0;
}

static int listEntries(const char** argv, Options_T* opt) {
	int rc;
	timing(opt, Entry, "vsamread");
	if (hasFilter(opt, KeyField)) {
		rc = listEntriesByKey(argv, opt, KeyField);
	} else {
		rc = listAllEntries(argv, opt);
	}
	timing(opt, Exit, "vsamread");

	return rc;
}

static int setKey(const char** argv, Options_T* opt) {
	FILE* vsamkfp;
	size_t currreclen;
	size_t newreclen;
	int rc;
	FixedHeader_T* hdr;
	char buff[MAX_RECLEN];

	vsamkfp = vsamopen(opt, opt->vsamCluster, KEY_QUAL, "rb+,type=record");
	if (!vsamkfp) {
		return 16;
	}
	hdr = vsamxlocate(vsamkfp, buff, argv, opt, KeyField, &currreclen);
	rc = setRecord(buff, &newreclen, argv, opt);
	if (rc) {
		fprintf(stderr, "Key/Value information is too large for VSAM record. Maximum Length is %d\n", MAX_RECLEN);
		return 12;
	}
	if (hdr) {
		rc = vsamdelrec(opt, vsamkfp);
		if (rc) {
			return 8;
		}
	}
	rc = vsamwrite(opt, buff, newreclen, vsamkfp);
	if (rc != newreclen) {
		return 9;
	}
	rc = vsamclose(opt, vsamkfp);
	if (rc) {
		return 10;
	}
	return 0;
}

static int idcams(const char* input, char** output, char* opts[]) {
	char buf[137+1];
	int stdinfd[2];
	int stdoutfd[2];
	int stderrfd[2];
	pid_t pid = 0;
	FILE* memfp;
	char** argv;
	size_t numopts=0;
	int status;
	int rc;
	int inlen;
	int outlen;
	int i=0;
	const int mvscmdcoreopts=4;

	rc = pipe(stdinfd);
	if (rc) { fprintf(stderr, "idcams: Unable to open stdin pipe\n"); return rc; }	
	rc = pipe(stdoutfd);
	if (rc) { fprintf(stderr, "idcams: Unable to open stdout pipe\n"); return rc; }	
	rc = pipe(stderrfd);
	if (rc) { fprintf(stderr, "idcams: Unable to open stderr pipe\n"); return rc; }	
	pid = fork();

	if (pid == 0) {
		/* Child */
		rc = dup2(stdinfd[0], STDIN_FILENO);
		if (rc == -1) { fprintf(stderr, "idcams: Unable to dup2 stdin pipe\n"); return rc; }	
		rc = dup2(stdoutfd[1], STDOUT_FILENO);
		if (rc == -1) { fprintf(stderr, "idcams: Unable to dup2 stdout pipe\n"); return rc; }	
		rc = dup2(stderrfd[1], STDERR_FILENO);
		if (rc == -1) { fprintf(stderr, "idcams: Unable to dup2 stderr pipe\n"); return rc; }	

		rc = close(stdinfd[1]);
		if (rc == -1) { fprintf(stderr, "idcams: Unable to close child stdin pipe\n"); return rc; }	
		rc = close(stdoutfd[0]);
		if (rc == -1) { fprintf(stderr, "idcams: Unable to close child stdout pipe\n"); return rc; }	
		rc = close(stderrfd[0]);
		if (rc == -1) { fprintf(stderr, "idcams: Unable to close child stderr pipe\n"); return rc; }	

		while (opts[numopts] != NULL) { ++numopts; }
		numopts++;
		argv = malloc((mvscmdcoreopts+numopts) * (sizeof(const char**)));
		argv[0] = "mvscmdauthhelper";
		argv[1] = "--pgm=IDCAMS";
		argv[2] = "--sysin=stdin";
		argv[3] = "--sysprint=stdout";
		memcpy(&argv[mvscmdcoreopts], opts, numopts*(sizeof(const char**)));
		execvp(argv[0], argv);  
		exit(1);
	}

	/* Parent */
	rc = close(stdinfd[0]);
	if (rc == -1) { fprintf(stderr, "idcams: Unable to close parent stdin pipe\n"); return rc; }	
	rc = close(stdoutfd[1]);
	if (rc == -1) { fprintf(stderr, "idcams: Unable to close parent stdout pipe\n"); return rc; }	
	rc = close(stderrfd[1]);
	if (rc == -1) { fprintf(stderr, "idcams: Unable to close parent stderr pipe\n"); return rc; }	

	inlen = strlen(input);
	rc = write(stdinfd[1], input, inlen);
	if (rc != inlen) { fprintf(stderr, "idcams: Expected to write %d input bytes but wrote %d bytes\n", inlen, rc); return -1; }

	rc = close(stdinfd[1]);
	if (rc == -1) { fprintf(stderr, "idcams: Unable to close parent stderr pipe\n"); return rc; }	

	memfp = fopen("idcams.output", "wb+,type=memory");
	if (memfp == NULL) { fprintf(stderr, "idcams: Unable to open memory file\n"); return -1; }
	while ((rc = read(stdoutfd[0], buf, sizeof(buf))) > 0) {
		int written;
		written = fwrite(buf, 1, rc, memfp);
		if (written != rc) { fprintf(stderr, "idcams: Expected to write %d output bytes but wrote %d bytes\n", rc, written); return -1; }
	}
	rc = close(stdoutfd[0]);
	if (rc == -1) { fprintf(stderr, "idcams: Unable to close parent stdout pipe\n"); return rc; }	
	outlen = ftell(memfp);
	if (outlen < 0) { fprintf(stderr, "idcams: ftell on memory file failed\n"); return -1; }
	rewind(memfp);

	*output = malloc(outlen);
	if (*output == NULL) { fprintf(stderr, "idcams: Unable to malloc %d bytes for output buffer\n", outlen); return -1; }
	rc = fread(*output, 1, outlen, memfp);
	if (outlen != rc) { fprintf(stderr, "idcams: Expected to read %d output bytes but wrote %d bytes\n", outlen, rc); return -1; }
	rc = fclose(memfp);	
	if (rc) { fprintf(stderr, "idcams: Unable to close memory file\n"); return -1; }	
	while ((rc = read(stderrfd[0], buf, 256)) > 0) {
		fprintf(stderr, "%.*s",rc, buf);
	}
	rc = close(stderrfd[0]);
	if (rc) { fprintf(stderr, "idcams: Unable to close parent stderr pipe\n"); return -1; }	
	do {
		if ((pid = waitpid(pid, &status, 0)) == -1) {
			perror("");
			fprintf(stderr, "idcams: Error waiting for pid\n"); 
			rc = -1;
		} else if (pid == 0) {
			sleep(1);
		} else {
			if (WIFEXITED(status)) {
				rc = WEXITSTATUS(status);
			} else {
				if (WIFSIGNALED(status)) {
					fprintf(stderr, "idcams abended\n");
				}
				rc = 255;
			}
		}
	} while (pid == 0);
	return rc;
}

static int defineCluster(Options_T* opt) {
	const char dcfmt[] = "\
  DEFINE CLUSTER ( - \n\
    NAME(%s) - \n\
    MEGABYTES(10,50) - \n\
    SHAREOPTIONS(2 3) - \n\
    INDEXED KEYS(%d %d) - \n\
    RECSZ(%d 32761) - \n\
  ) - \n\
  DATA ( - \n\
    NAME(%s.DATA) - \n\
  ) - \n\
  INDEX (NAME(%s.%s.INDEX))";
	char input[11*80];
	char* output = NULL;
	char* defineOpts[] = {  NULL };
	int rc;

	sprintf(input, dcfmt, opt->vsamCluster, KSDSKEYLEN, KSDSKEYOFF, RECLEN, opt->vsamCluster, opt->vsamCluster, KSDSKEYNAME);
	rc = idcams(input, &output, defineOpts);
	if (rc) {
		fputs(output, stderr);
	}
	free(output);

	return rc;
}

static int reproCluster(Options_T* opt) {
	int rc;
	char reproIn[80];
	char reproOut[80];
	char* output = NULL;
	const char reproInFmt[] = "--in=%s,lrecl=%d";
	const char reproOutFmt[] = "--out=%s";
	char* reproopts[] = { reproIn, reproOut, NULL };
	const char reprostdin[] = "  REPRO INFILE(IN) OUTFILE(OUT)";
	FixedHeader_T initrec;          
	char* tmpreprofile = tmpnam(NULL);
	int tfd;
	int reprowritten;

	createInitialRecord(&initrec);

	tfd = open(tmpreprofile, O_WRONLY | O_CREAT | O_EXCL);
	if (tfd == -1) {
		fprintf(stderr, "Unable to open temporary repro file %s\n", tmpreprofile);
		return 12;
	}
	reprowritten = write(tfd, &initrec, sizeof(initrec));
	if (reprowritten != sizeof(initrec)) {
		fprintf(stderr, "Unable to write initial record to repro file %s\n", tmpreprofile);
		return 16;
	}
	rc = close(tfd);
	if (rc == -1) {
		fprintf(stderr, "Unable to close temporary repro file %s\n", tmpreprofile);
		return 12;
	}

	sprintf(reproIn, reproInFmt, tmpreprofile, RECLEN);
	sprintf(reproOut, reproOutFmt, opt->vsamCluster);

	rc = idcams(reprostdin, &output, reproopts);
	if (rc) {
		fputs(output, stderr);
	}
	free(output);
	remove(tmpreprofile);

	return rc;
}

static int defineAIX(Options_T* opt) {
	const char dafmt[] = "\
  DEFINE AIX ( - \n\
    NAME(%s.%s) - \n\
    RELATE(%s) - \n\
    MEGABYTES(2,10) - \n\
    SHAREOPTIONS(2 3) - \n\
    KEYS(%d %d) - \n\
    NONUNIQUEKEY - \n\
    UPGRADE - \n\
  ) - \n\
  DATA ( - \n\
    NAME(%s.%s.DATA) - \n\
  ) - \n\
  INDEX ( - \n\
    NAME(%s.%s.INDEX) - \n\
  )";
	char input[15*80];
	char* output = NULL;
	char* defineOpts[] = {  NULL };
	int rc;

	sprintf(input, dafmt, opt->vsamCluster, KEYNAME, opt->vsamCluster, KEYLEN, KEYOFF, opt->vsamCluster, KEYNAME, opt->vsamCluster, KEYNAME);
	rc = idcams(input, &output, defineOpts);
	if (rc) {
		fputs(output, stderr);
	}
	free(output);

	return rc;
}

static int definePath(Options_T* opt) {
	const char dpfmt[] = "\
  DEFINE PATH ( - \n\
    NAME(%s.%s.PATH) - \n\
    PATHENTRY(%s.%s) - \n\
    UPDATE - \n\
  )";
	char input[5*80];
	char* output = NULL;
	char* defineOpts[] = {  NULL };
	int rc;

	sprintf(input, dpfmt, opt->vsamCluster, KEYNAME, opt->vsamCluster, KEYNAME);
	rc = idcams(input, &output, defineOpts);
	if (rc) {
		fputs(output, stderr);
	}
	free(output);

	return rc;
}

static int buildIndex(Options_T* opt) {
	const char bifmt[] = "\
  BLDINDEX  - \n\
    INDATASET(%s) - \n\
    OUTDATASET(%s.%s)";
	char input[3*80];
	char* output = NULL;
	char* buildOpts[] = {  NULL };
	int rc;

	sprintf(input, bifmt, opt->vsamCluster, opt->vsamCluster, KEYNAME);
	rc = idcams(input, &output, buildOpts);
	if (rc) {
		fputs(output, stderr);
	}
	free(output);

	return rc;
}

static int createDB(const char** argv, Options_T* opt) {
	FILE* vsamkfp = vsamopen_internal(opt, opt->vsamCluster, KEY_QUAL, "rb,type=record", 0);
	FILE* vsamcfp = vsamopen_internal(opt, opt->vsamCluster, CLUSTER_QUAL, "rb,type=record", 0);
	int rc;

	if (vsamkfp != NULL) {
		fprintf(stderr, "Database %s%s already exists. Creation not performed\n", opt->vsamCluster, KEY_QUAL);
		vsamclose(opt, vsamkfp);
		return 8;
	}
	if (vsamcfp != NULL) {
		fprintf(stderr, "Database %s%s already exists. Creation not performed\n", opt->vsamCluster, CLUSTER_QUAL);
		vsamclose(opt, vsamkfp);
		return 8;
	}

	rc = defineCluster(opt);
	if (rc) {
		return rc;
	}
	rc = reproCluster(opt);
	if (rc) {
		return rc;
	}
	rc = defineAIX(opt);
	if (rc) {
		return rc;
	}
	rc = definePath(opt);
	if (rc) {
		return rc;
	}
	rc = buildIndex(opt);
	if (rc) {
		return rc;
	}
	
	return rc;
}

int xsysvar(int argc, const char** argv) {
	Options_T opt = { 0 };
	int rc;

	if (rc=processArgs(argc, argv, &opt)) {
		return rc;	
	}
	timing(&opt, Entry, "main");
	if (opt.create) {
		rc=createDB(argv, &opt);
	} else if (opt.list) {
		rc=listEntries(argv, &opt);
	} else if (opt.delete) {
		rc=deleteKey(argv, &opt);
	} else if (opt.get) {
		rc=getKey(argv, &opt);
	} else if (opt.set) {
		rc=setKey(argv, &opt);
	} else {
		fprintf(stderr, "Key not specified\n");
		return syntax(argv[0]);
	}
	timing(&opt, Exit, "main");
	return rc;
}

int main(int argc, const char** argv) {
	return xsysvar(argc, argv);
}
