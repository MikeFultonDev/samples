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

	KeyField=5,
	ValField=6,
	ProdIDField=7,

	FirstFilterField=0,
	LastFilterField=4,
	NumFilterFields=5,
	NumFields=8 
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
	fprintf(stderr, "%s [-?hlXSPVRMD] <key>[=<val>]\n", prog);
	fprintf(stderr, "Where:\n");
	fprintf(stderr, " -?|-h: show this help\n");
	fprintf(stderr, " -X*: variable has GLOBAL scope [default for set]\n");
	fprintf(stderr, " -X<sysplex>: variable is for SYSPLEX <sysplex> only\n");
	fprintf(stderr, " -S*: variable has SYSPLEX (all systems in this SYSPLEX) scope [default for set]\n");
	fprintf(stderr, " -S<system>: variable is for system <system> only\n");
	fprintf(stderr, " -P*: variable scope is for all products [default for get and set]\n");
	fprintf(stderr, " -P<prod>: variable is for product prefix <prod> only\n");
	fprintf(stderr, " -V*: variable scope is not version specific [default for get and set]\n");
	fprintf(stderr, " -V<ver>: variable scope is for version <ver> of product prefix <prod> only. Requires -P to be specified\n");
	fprintf(stderr, " -R*: variable scope is not release specific [default for get and set]\n");
	fprintf(stderr, " -R<rel>: variable scope is for release <rel> of product prefix <prod> only. Requires -P to be specified\n");
	fprintf(stderr, " -M*: variable scope is not modification specific [default for get and set]\n");
	fprintf(stderr, " -M<mod>: variable scope is for modification <mod> of product prefix <prod> only. Requires -P to be specified\n");
	fprintf(stderr, " -D<database-hlq>: use database (VSAM dataset) with prefix <database-hlq>. Default is SYS1.XSYSVAR\n");
	fprintf(stderr, " -l: Display (in order, separated by spaces, all matches, one per line) <sysplex> <system> <prod> <ver> <rel> <mod> <val>\n");
	fprintf(stderr, "     [default is to just display the value]\n");
	fprintf(stderr, "Note:\n");
	fprintf(stderr, " The key, value, sysplex, system, version, release, modification values can be up to 255 characters in length\n");
	fprintf(stderr, " The product prefix must be 3 characters in length or *\n");
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
	fprintf(stderr, " Assuming the previous commands have been issued\n");
	fprintf(stderr, " Get key/value pair for JAVA_HOME from SYSPLEX plex, SYSTEM S0W2\n");
	fprintf(stderr, "  %s JAVA_HOME <-- returns /usr/lpp/java/current_64\n", prog);
	fprintf(stderr, " Get key/value pair for JAVA_HOME from SYSPLEX plex, SYSTEM S0W1\n");
	fprintf(stderr, "  %s JAVA_HOME <-- returns /usr/local/devline_64\n", prog);
	fprintf(stderr, " Get key/value pairs for all CSIs\n");
	fprintf(stderr, "  %s -l CSI <-- returns\n", prog);
	fprintf(stderr, "   * * IGY 6 3 0 CSI SMPE.IGY630.CSI\n");
	fprintf(stderr, "   * * IGY 6 2 0 CSI SMPE.IGY620.CSI\n");
	fprintf(stderr, "   * * ZOS 2 4 0 CSI SMPE.ZOS240.CSI\n");

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

static int setField(Options_T* opt, char** argv, int i, size_t offset, VSAMField_T field) {
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

static int processArgs(int argc, char** argv, Options_T* opt) {
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
			opt->argindex[KeyField] = i;
			opt->offset[KeyField] = 0;
			eqp = strchr(arg, '=');
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

static FilterHeader_T* getFilterHeader(FixedHeader_T* hdr) {
	char* buff = ((char*) hdr) + sizeof(FixedHeader_T);
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
	char* buff = (char*) hdr;
	size_t bufflen;

	buff += sizeof(FixedHeader_T);
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
	char* buff = (char*) hdr;
	if (userlen == 0 && opt->list) {
		return FullMatch;
	}

	if (xlen != userlen) {
		return PartialMatch;
	}

	buff += sizeof(FixedHeader_T);
	if (!memcmp(&buff[xoffsetp], userkey, userlen)) {
		return FullMatch;
	} else {
		return PartialMatch;
	}
}

static int cmpfield(FixedHeader_T* hdr, Options_T* opt, const char* vsamfield, const char* userfield, size_t userlen) {
	if (userlen == 0 && opt->list) {
		return FullMatch;
	}
	if (!memcmp(vsamfield, userfield, userlen)) {
		if (hdr->inactive) {
			return PartialMatch;
		} else {
			return FullMatch;
		}
	} else {
		return NoMatch;
	}
}

static KeyMatch_T vsamxmatch(FixedHeader_T* hdr, Options_T* opt, char** argv, const char* field, size_t len, VSAMField_T fieldName) {
	KeyMatch_T fieldCheck;
	switch (fieldName) {
		case KeyField: {
			const char* prodID;
			size_t prodIDLen;
			size_t cmplen = (len < FIXED_KEY_SIZE) ? len : FIXED_KEY_SIZE-1;

			fieldCheck = cmpfield(hdr, opt, hdr->key, field, cmplen);
			if (fieldCheck != FullMatch) {
				return fieldCheck;
			}
			if (len >= FIXED_KEY_SIZE) {
				fieldCheck = cmpxfield(hdr, opt, hdr->keyXOffset, hdr->keyXLen, len-FIXED_KEY_SIZE+1, &field[FIXED_KEY_SIZE-1]);
				if (fieldCheck != FullMatch) {
					return fieldCheck;
				}
			}
			/*
			 * MSF TBD: this needs to check all filters, not just the prod id
			 * to determine if there is an exact match
			 */
			prodID = &argv[opt->argindex[ProdIDField]][opt->offset[ProdIDField]];
			prodIDLen = opt->len[ProdIDField];
			fieldCheck = vsamxmatch(hdr, opt, argv, prodID, prodIDLen, ProdIDField);
			return fieldCheck;
		}
		case ProdIDField:
			/*
			 * ProdID is a filter and is therefore either a full match or else searching should 
			 * continue (i.e. it is a partial match
			 */
			if (len >= FIXED_PRODID_SIZE) {
				fieldCheck = cmpfield(hdr, opt, hdr->prodID, field, FIXED_PRODID_SIZE-1);
				if (fieldCheck == FullMatch) {
					fieldCheck = cmpxfield(hdr, opt, hdr->prodIDXOffset, hdr->prodIDXLen, len-FIXED_PRODID_SIZE+1, &field[FIXED_PRODID_SIZE-1]);
				}
			} else {
				fieldCheck = cmpfield(hdr, opt, hdr->prodID, field, len);
			}
			if (fieldCheck == NoMatch) {
				fieldCheck = PartialMatch; 
			}
			return fieldCheck;
		default:
			fprintf(stderr, "Internal Error: vsamxmatch Unexpected key other than KEY or PRODID passed in:%d\n", fieldName);
			exit(16);
	}
}
	
static FixedHeader_T* vsamxlocate(FILE* fp, char* buff, char** argv, Options_T* opt, VSAMField_T fieldName, size_t* reclen) {
	FixedHeader_T* hdr = (FixedHeader_T*) buff;	
	char* vsamfield;
	char* userfield;
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
			userfield = &argv[opt->argindex[KeyField]][opt->offset[KeyField]];
			userlen = opt->len[KeyField];
			fixedlen = FIXED_KEY_SIZE;
			break;
		case ProdIDField:
			vsamfield = hdr->prodID;
			userfield = &argv[opt->argindex[ProdIDField]][opt->offset[ProdIDField]];
			userlen = opt->len[ProdIDField];
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
		result = vsamxmatch(hdr, opt, argv, userfield, userlen, fieldName);
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
	const char* buff = (((const char*) hdr) + sizeof(FixedHeader_T));
	int rc;

	if (xlen > 0) {	
		rc = printf("%s%.*s%s", fixed, xlen, &buff[offset], sep);
	} else {
		rc = printf("%s%s", fixed, sep);
	}
	return rc;
}

static int printffield(FixedHeader_T* hdr, VSAMField_T field, const char* sep) {
	const char* buff = (((const char*) hdr) + sizeof(FixedHeader_T));
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

	rc = printfield(hdr, ValField, PRINT_NL);
	if (rc <= 0) { return rc; }
	totrc += rc;

	return totrc;
}

static int setRecord(char* buff, size_t *bufflen, char** argv, Options_T* opt) {
	FixedHeader_T* fh = (FixedHeader_T*) buff;
	size_t extlen=0;
	VSAMField_T f;
  
	memset(fh, 0, sizeof(FixedHeader_T));
	for (f=FirstFilterField; f<=LastFilterField; ++f) {
		if (hasFilter(opt, f)) {
			extlen += setfield(fh, &argv[opt->argindex[f]][opt->offset[f]], opt->len[f], f);
		}
	}
	extlen += setfield(fh, &argv[opt->argindex[KeyField]][opt->offset[KeyField]], opt->len[KeyField], KeyField);
	extlen += setfield(fh, &argv[opt->argindex[ValField]][opt->offset[ValField]], opt->len[ValField], ValField);
	extlen += setfield(fh, &argv[opt->argindex[ProdIDField]][opt->offset[ProdIDField]], opt->len[ProdIDField], ProdIDField);

	*bufflen = (sizeof(FixedHeader_T) + extlen);

	return 0;
}

static int getKey(char** argv, Options_T* opt) {
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

static int listKeyEntries(char** argv, Options_T* opt) {
	fprintf(stderr, "Not implemented yet - list key entries\n");
	return 8;
}

static int listProdIDEntries(char** argv, Options_T* opt) {
	FILE* fp;
	int rc;
	FixedHeader_T* hdr;
	size_t reclen;
	char buff[MAX_RECLEN];
	KeyMatch_T result = FullMatch;

	fp = vsamopen(opt->vsamCluster, PRODID_QUAL, "rb,type=record");
	if (!fp) {
		return 16;
	}
	hdr = vsamxlocate(fp, buff, argv, opt, ProdIDField, &reclen);
	while (hdr) {
		const char* prodID = &argv[opt->argindex[ProdIDField]][opt->offset[ProdIDField]];
		unsigned short prodIDLen = opt->len[ProdIDField];

		rc = printfields(hdr);
		if (rc <= 0) {
			return 8;
		}
		do {
			rc = vsamread(hdr, MAX_RECLEN, fp);
			if (rc == 0) {
				if (!feof(fp)) {
					fprintf(stderr, "Internal Error: Unable to read record after flocate/fread of %s successful\n", prodID);
				}
				return 10;
			}
			result = vsamxmatch(hdr, opt, argv, prodID, prodIDLen, ProdIDField);
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

static int listEntries(char** argv, Options_T* opt) {
	if (hasFilter(opt, ProdIDField)) {
		if (hasFilter(opt, KeyField)) {
			return listKeyEntries(argv, opt);
		} else {
			return listProdIDEntries(argv, opt);
		}
	} else {
		return listKeyEntries(argv, opt);
	}
}

static int setKey(char** argv, Options_T* opt) {
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

int main(int argc, char* argv[]) {
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
