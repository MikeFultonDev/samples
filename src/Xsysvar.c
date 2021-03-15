/*********************************************************************
 * Copyright (c) 2021 IBM
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

#pragma langlvl(extended)
#define _OPEN_SYS_UNLOCKED_EXT 1
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>

#define MAX_DSNAME_LEN (44)
#define MAX_RECLEN (32761)
#define FIXED_PRODID_SIZE (4)
#define FIXED_KEY_SIZE (16)
#define FIXED_VAL_SIZE (16)
#define CLUSTER_QUAL ""
#define KEY_QUAL ".KEY.PATH"
#define PRODID_QUAL ".PRODID.PATH"

#define PRINT_FIELD_SEP "\t"
#define PRINT_NL  "\n"

const static char DEFAULT_VSAM_CLUSTER[] = "SYS1.XSYSVAR";

/*
 * NOTE: This enum ordering can not be changed without changing the
 * corresponding FilterHeader_T and FixedHeader_T structures
 */
typedef enum {
	SysplexField=0,
	SystemField=1,
	VerField=2,
	RelField=3,
	ModField=4,
	CommentField=5,

	KeyField=6,
	ValField=7,
	ProdIDField=8,

	FirstFilterField=0,
	LastFilterField=5,
	NumFilterFields=6,
	NumFields=9 
} VSAMField_T;

typedef enum {
	NoMatch=1,
	PartialMatch=2,
	FullMatch=3
} KeyMatch_T;

typedef struct {
	char vsamCluster[MAX_DSNAME_LEN+1];
	size_t argindex[NumFields];
	size_t offset[NumFields];
	size_t len[NumFields];
	int list:1;
	int get:1;
	int set:1;
} Options_T;

typedef _Packed struct {
	unsigned short offset[NumFilterFields];
	unsigned short len[NumFilterFields];
} FilterHeader_T;

/*
 * NOTE: The following structure layout CAN NOT be changed without also 
 *       re-generating the VSAM Sphere as well as updating the code for
 *       calculating how to read/write to instances of this structure.
 *       The last byte of the fixed character fields is always 0x00.
 */
typedef _Packed struct {
	char inactive;
	char reserved;  
	char prodID[FIXED_PRODID_SIZE];
	char key[FIXED_KEY_SIZE];
	char val[FIXED_VAL_SIZE];
	unsigned short prodIDXOffset;
	unsigned short prodIDXLen;
	unsigned short keyXOffset;
	unsigned short keyXLen;
	unsigned short valXOffset;
	unsigned short valXLen;
	unsigned short filterXOffset;
	unsigned short filterXLen;
} FixedHeader_T;

static int syntax(const char* prog) {
	fprintf(stderr, "%s [-?hlCXSPVRMD] <key>[=<val>]\n", prog);
	fprintf(stderr, "Where:\n");
	fprintf(stderr, " -?|-h: show this help\n");
	fprintf(stderr, " Filters can be specified for set/get/list\n");
	fprintf(stderr, "  -X<sysplex>: Sysplex <sysplex> specific\n");
	fprintf(stderr, "  -S<system>: System <system> specific\n");
	fprintf(stderr, "  -P<prod>: Product <prod> specific\n");
	fprintf(stderr, "  -V<ver>: Version <ver> of product <prod> specific. Requires -P to be specified\n");
	fprintf(stderr, "  -R<rel>: Release <rel> of product <prod> specific. Requires -P to be specified\n");
	fprintf(stderr, "  -M<mod>: Modification <mod> of product <prod> specific. Requires -P to be specified\n");
	fprintf(stderr, " Listing multiple keys by filter:\n");
	fprintf(stderr, "  -l: list the filters, key, and value that match the filter request\n");
	fprintf(stderr, "      the list is by either key or <prod>, e.g. -l -P<prod> or -l <key>\n");
	fprintf(stderr, "      additional filters can be specified to restrict the listing\n");
 	fprintf(stderr, "      Each line is of the format:<sysplex>\t<system>\t<prod>\t<ver>\t<rel>\t<mod>\t<val>\t<comment>\n");
	fprintf(stderr, " Other options:\n");
	fprintf(stderr, "  -C<comment>: Comment <comment> specified\n");
	fprintf(stderr, "  -D<database-hlq>: use database (VSAM dataset) with prefix <database-hlq>. Default is SYS1.XSYSVAR\n");
	fprintf(stderr, "Note:\n");
	fprintf(stderr, " The combined length of the key, value, and filters must be less than 32K bytes\n");
	fprintf(stderr, "Examples:\n");
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
	fprintf(stderr, " Get key/value pairs for product IGY\n");
	fprintf(stderr, "  %s -l -PIGY <-- returns\n", prog);
	fprintf(stderr, "   \t\tIGY\t6\t3\t0\tCSI\tSMPE.IGY630.CSI\t\n");
	fprintf(stderr, "   \t\tIGY\t6\t2\t0\tCSI\tSMPE.IGY620.CSI\t\n");
	fprintf(stderr, "   \t\tIGY\t\t\t\tHLQ\tIGY630\tActive COBOL compiler HLQ\n");

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

const char* optstr(const char **argv, Options_T* opt, VSAMField_T field) {
	return &argv[opt->argindex[field]][opt->offset[field]];
}

size_t optlen(Options_T* opt, VSAMField_T field) {
	return opt->len[field];
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
			fprintf(stderr, "-V, -R, and -M can only be specified if -P is specified\n");
			return 8;
		}
	}

	if (opt->set && opt->list) {
		fprintf(stderr, "-l can not be specified when setting a variable\n");
		return 8;
	}
	return 0;
}


static FILE* vsamopen(const char* dataset, const char* qual, const char* fmt) {
	char mvsname[MAX_DSNAME_LEN+5];
	FILE* fp;
	int saveerrno;

	if (strlen(dataset) + strlen(qual) > MAX_DSNAME_LEN) {
		fprintf(stderr, "VSAM Cluster key/value dataset name invalid: %s\n", dataset);
		return NULL;
	}
	sprintf(mvsname, "//'%s%s'", dataset, qual);
	fp=fopen(mvsname, fmt);
	if (!fp) {
		saveerrno=errno;
		fprintf(stderr, "Unable to open VSAM dataset %s for read\n", mvsname);
		errno=saveerrno;
		perror(mvsname);
	}
	return fp;
}

static size_t vsamread(void* buff, size_t numbytes, FILE* fp) {
	int saveerrno;
	size_t rc=fread(buff, 1, numbytes, fp);
	if (rc == 0 && !feof(fp)) {
		saveerrno=errno;
		fprintf(stderr, "Unable to read from VSAM dataset\n");
		errno=saveerrno;
		perror("fread");
	}
	return rc;
}

static int vsamwrite(const char* buff, size_t numbytes, FILE* fp) {
	int saveerrno;
	int rc=fwrite(buff, 1, numbytes, fp);
	if (rc != numbytes) {
		saveerrno=errno;
		fprintf(stderr, "Unable to write to VSAM dataset\n");
		errno=saveerrno;
		perror("fwrite");
	}
	return rc;
}

static char* getBuffer(hdr) {
	return ((char*) hdr) + sizeof(FixedHeader_T);
}

static FilterHeader_T* getFilterHeader(FixedHeader_T* hdr) {
	char* buff = getBuffer(hdr);
	FilterHeader_T* filter = (FilterHeader_T*) (&buff[hdr->filterXOffset]);
	return filter;
}

static size_t computebufflen(FixedHeader_T* hdr) {
	size_t len = hdr->prodIDXLen + hdr->keyXLen + hdr->valXLen + hdr->filterXLen;
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
		case ProdIDField:
			if (len >= FIXED_PRODID_SIZE) {
				memcpy(hdr->prodID, field, FIXED_PRODID_SIZE-1);
				return setxfield(hdr, &hdr->prodIDXOffset, &hdr->prodIDXLen, len-FIXED_PRODID_SIZE+1, &field[FIXED_PRODID_SIZE-1]);
			} else {
				memcpy(hdr->prodID, field, len);
				return 0;
			}
                        break;
		case CommentField:
		case SysplexField:
		case SystemField:
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
		if (hdr->inactive) {
			return PartialMatch;
		} else {
			return FullMatch;
		}
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
		case ProdIDField:
			fixedField = hdr->prodID;
			fixedLen = FIXED_PRODID_SIZE;
			xOffset = hdr->prodIDXOffset;
			xLen = hdr->prodIDXLen;
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
	unsigned short len = *filterLen(hdr, field);
	unsigned short offset = *filterOffset(hdr, field);
	char* buff = getBuffer(hdr);
	char* vsamfield = &buff[offset];

        if (userlen == 0 && opt->list) {
	        return FullMatch;
	}               
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
 * Check the passed in optStr/optLen against the VSAM key (either KeyField or ProdIDField)
 * If there is no match, or a partial match, then return the result (a partial match will cause another record to be read)
 * If there is a full match, check the filters. 
 *   - For a key, this is prodid and then the extended filters
 *   - For a prodid, this is key and then the extended filters
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
			fieldCheck = cmpkeyfield(hdr, opt, optstr(argv, opt, ProdIDField), optlen(opt,ProdIDField), ProdIDField);
			if (fieldCheck == NoMatch) {
				fieldCheck = PartialMatch; 
			}
			if (fieldCheck != FullMatch) {
				return fieldCheck;
			}
			break;

		case ProdIDField:
			fieldCheck = cmpkeyfield(hdr, opt, optStr, optLen, ProdIDField);
			if (fieldCheck != FullMatch) {
				return fieldCheck;
			}
			fieldCheck = cmpkeyfield(hdr, opt, optstr(argv, opt, KeyField), optlen(opt,KeyField), KeyField);
			if (fieldCheck == NoMatch) {
				fieldCheck = PartialMatch; 
			}
			if (fieldCheck != FullMatch) {
				return fieldCheck;
			}
			break;
	
		default:
			fprintf(stderr, "Internal Error: vsamkeymatch Unexpected field passed in:%d\n", fieldName);
			exit(16);
	}

	if (hdr->filterXLen > 0) {
		for (f=FirstFilterField; f<=LastFilterField; ++f) {
			if (f == CommentField) continue;   
			fieldCheck = cmpfilterfield(hdr, opt, optstr(argv, opt, f), optlen(opt,f), f);
			if (fieldCheck != FullMatch) {
				return fieldCheck;
			}
		}
	}
	return fieldCheck;
}

static FixedHeader_T* vsamxlocate(FILE* fp, char* buff, const char** argv, Options_T* opt, VSAMField_T fieldName, size_t* reclen) {
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
		case ProdIDField:
			vsamfield = hdr->prodID;
			userfield = optstr(argv, opt, ProdIDField);
			userlen = optlen(opt, ProdIDField);
			fixedlen = FIXED_PRODID_SIZE;
			break;
		default:
			fprintf(stderr, "Internal Error: vsamxlocate Unexpected key other than KEY or PRODID passed in:%d\n", fieldName);
			exit(16);
	}
	vsamfieldlen = (userlen >= fixedlen) ? fixedlen-1 : userlen;
	memcpy(vsamfield, userfield, vsamfieldlen);
	rc = flocate(fp, vsamfield, fixedlen-1, __KEY_EQ);
	if (rc) {
		return NULL;
	}
	while (result == PartialMatch) {
		rc = vsamread(hdr, MAX_RECLEN, fp);
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

static int vsamclose(FILE* fp) {
	int saveerrno;
	int rc = fclose(fp);
	if (rc) {
		saveerrno=errno;
		fprintf(stderr, "Unable to close VSAM dataset\n");
		errno=saveerrno;
		perror("fclose");
	}
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
                case ProdIDField:
			rc = printxfield(hdr, hdr->prodID, FIXED_PRODID_SIZE-1, hdr->prodIDXOffset, hdr->prodIDXLen, sep);
	          	break;
                case KeyField:
			rc = printxfield(hdr, hdr->key, FIXED_KEY_SIZE-1, hdr->keyXOffset, hdr->keyXLen, sep);
	          	break;
                case ValField:
			rc = printxfield(hdr, hdr->val, FIXED_VAL_SIZE-1, hdr->valXOffset, hdr->valXLen, sep);
	          	break;
		case CommentField:
		case SysplexField:
		case SystemField:
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

	rc = printfield(hdr, ProdIDField, PRINT_FIELD_SEP);
	if (rc <= 0) { return rc; }
	totrc += rc;

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

static int setRecord(char* buff, size_t *reclen, const char** argv, Options_T* opt) {
	FixedHeader_T* fh = (FixedHeader_T*) buff;
	size_t extlen=0;
	VSAMField_T f;
  
	memset(fh, 0, sizeof(FixedHeader_T));
	for (f=FirstFilterField; f<=LastFilterField; ++f) {
		if (hasFilter(opt, f)) {
			extlen += setfield(fh, optstr(argv, opt, f), optlen(opt, f), f);
		}
	}
	extlen += setfield(fh, optstr(argv, opt, KeyField), optlen(opt,KeyField), KeyField);
	extlen += setfield(fh, optstr(argv, opt, ValField), optlen(opt,ValField), ValField);
	extlen += setfield(fh, optstr(argv, opt, ProdIDField), optlen(opt,ProdIDField), ProdIDField);

	*reclen = (sizeof(FixedHeader_T) + extlen);

	return 0;
}

static int getKey(const char** argv, Options_T* opt) {
	FILE* vsamfp;
	int rc;
	FixedHeader_T* hdr;
	size_t reclen;
	char buff[MAX_RECLEN];

	vsamfp = vsamopen(opt->vsamCluster, KEY_QUAL, "rb,type=record");
	if (!vsamfp) {
		return 16;
	}
	hdr = vsamxlocate(vsamfp, buff, argv, opt, KeyField, &reclen);
	if (!hdr) {
		return 4;
	}
	rc = printfield(hdr, ValField, PRINT_NL);
	if (rc <= 0) {
		return 8;
	}
	rc = vsamclose(vsamfp);
	if (rc) {
		return 12;
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
		case ProdIDField: 
			qual=PRODID_QUAL;
			break;
		case KeyField: 
			qual=KEY_QUAL;
			break;
		default:
                        fprintf(stderr, "Internal error. Unknown field: %d for printfield\n", keyfield);
	                exit(16);
	}

	fp = vsamopen(opt->vsamCluster, qual, "rb,type=record");
	if (!fp) {
		return 16;
	}
	hdr = vsamxlocate(fp, buff, argv, opt, keyfield, &reclen);
	while (hdr) {
		const char* key = optstr(argv, opt, keyfield);
		unsigned short keyLen = optlen(opt, keyfield);

		rc = printfields(hdr);
		if (rc <= 0) {
			return 8;
		}
		do {
			rc = vsamread(hdr, MAX_RECLEN, fp);
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
	rc = vsamclose(fp);
	if (rc) {
		return 12;
	}
	return 0;
}

static int listEntries(const char** argv, Options_T* opt) {
	if (hasFilter(opt, ProdIDField)) {
		if (hasFilter(opt, KeyField)) {
			return listEntriesByKey(argv, opt, KeyField);
		} else {
			return listEntriesByKey(argv, opt, ProdIDField);
		}
	} else {
		return listEntriesByKey(argv, opt, KeyField);
	}
}

static int setKey(const char** argv, Options_T* opt) {
	FILE* vsamkfp;
	FILE* vsamcfp;
	size_t currreclen;
	size_t newreclen;
	int rc;
	FixedHeader_T* hdr;
	char buff[MAX_RECLEN];
	char invalidrecord[1] = { 1 };

	vsamkfp = vsamopen(opt->vsamCluster, KEY_QUAL, "rb+,type=record");
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
		if (newreclen <= currreclen) {
			if (newreclen < currreclen) {
				memset(&buff[newreclen], 0, currreclen-newreclen);
			}
			fupdate(buff, currreclen, vsamkfp);
			return 0;
		} else {
			fupdate(invalidrecord, sizeof(invalidrecord), vsamkfp);
		}
	}
	rc = vsamclose(vsamkfp);
	if (rc) {
		return 10;
	}

	vsamcfp = vsamopen(opt->vsamCluster, CLUSTER_QUAL, "rb+,type=record");
	if (!vsamcfp) {
		return 7;
	}
	rc = vsamwrite(buff, newreclen, vsamcfp);
	if (rc != newreclen) {
		return 9;
	}
	
	rc = vsamclose(vsamcfp);
	if (rc) {
		return 13;
	}
	return 0;
}

int main(int argc, const char** argv) {
	Options_T opt = { 0 };
	int rc;

	if (rc=processArgs(argc, argv, &opt)) {
		return rc;	
	}
	if (opt.list) {
		rc=listEntries(argv, &opt);
	} else if (opt.get) {
		rc=getKey(argv, &opt);
	} else if (opt.set) {
		rc=setKey(argv, &opt);
	} else {
		fprintf(stderr, "Key not specified\n");
		return syntax(argv[0]);
	}
	return rc;
}
