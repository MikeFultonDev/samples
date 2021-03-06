#pragma langlvl(extended)
#define _OPEN_SYS_UNLOCKED_EXT 1
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define MAX_DSNAME_LEN (44)
#define MAX_RECLEN (32761)
#define FIXED_KEY_SIZE (16)
#define FIXED_VAL_SIZE (16)
#define CLUSTER_QUAL ""
#define KEY_QUAL ".KEY.PATH"

const static char DEFAULT_VSAM_CLUSTER[] = "SYS1.XSYSVAR";

typedef enum {
	NoField=0,
	KeyField=1,
	ValField=2
} VSAMField_T;

typedef enum {
	NoMatch=1,
	PartialMatch=2,
	FullMatch=3
} KeyMatch_T;

typedef struct {
	char vsamCluster[MAX_DSNAME_LEN+1];
	size_t keyOffset; 
	size_t valOffset; 
	size_t keyLen; 
	size_t valLen; 
	int keyValIndex;
	int get:1;
	int set:1;
} Options_T;

/*
 * NOTE: The following structure layout can not be changed without also 
 *       re-generating the VSAM Sphere as well as updating the code for
 *       calculating how to read/write to instances of this structure.
 *       The last byte of the fixed character fields is always 0x00.
 */
typedef _Packed struct {
	char inactive;
	char prodID[4];
	char ver[3];
	char rel[3];
	char mod[3];
	char key[FIXED_KEY_SIZE];
	char val[FIXED_VAL_SIZE];
	unsigned short prodIDXOffset;
	unsigned short prodIDXLen;
	unsigned short verXOffset;
	unsigned short verXLen;
	unsigned short relXOffset;
	unsigned short relXLen;
	unsigned short modXOffset;
	unsigned short modXLen;
	unsigned short keyXOffset;
	unsigned short keyXLen;
	unsigned short valXOffset;
	unsigned short valXLen;
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

	return 8;
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

static int processArgs(int argc, char** argv, Options_T* opt) {
	int i, rc;

	setCluster(opt, DEFAULT_VSAM_CLUSTER);

	for (i=1; i<argc; ++i) {
		const char* arg = argv[i];
		if (arg[0] == '-') {
			switch (arg[1]) { 
				case 'h':
				case '?':
					return(syntax(argv[0]));
					break;
				case 'l':
				case 'X':
				case 'S':
				case 'P':
				case 'V':
				case 'R':
				case 'M':
					fprintf(stderr, "Option %s not implemented yet\n", arg);
					return 4;
				case 'D':
					rc = setCluster(opt, &arg[2]);
					if (rc) {		
						return rc;
					}
					break;
				default:
					fprintf(stderr, "Unknown option:%s\n", arg);
					return(syntax(argv[0]));
					break;
			}
		} else {	
			char* eqp;
			int len = strlen(arg);
			opt->keyValIndex = i;
			opt->keyOffset = 0;
			eqp = strchr(arg, '=');
			if (eqp) {
				opt->set = 1;
				opt->valOffset = (eqp-arg)+1;
				opt->keyLen = (eqp-arg);
				opt->valLen = len-opt->valOffset;
			} else {
				opt->get = 1;
				opt->keyLen = len;
			}
		}
	}
	return 0;
}


static FILE* vsamopen(const char* dataset, const char* qual, const char* fmt) {
	char mvsname[MAX_DSNAME_LEN+5];
	FILE* fp;

	if (strlen(dataset) + strlen(qual) > MAX_DSNAME_LEN) {
		fprintf(stderr, "VSAM Cluster key/value dataset name invalid: %s\n", dataset);
		return NULL;
	}
	sprintf(mvsname, "//'%s%s'", dataset, qual);
	fp=fopen(mvsname, fmt);
	if (!fp) {
		perror(mvsname);
		fprintf(stderr, "Unable to open VSAM dataset %s for read\n", mvsname);
	}
	return fp;
}

static int vsamread(void* buffer, size_t numbytes, FILE* fp) {
	int rc=fread(buffer, 1, numbytes, fp);
	if (rc <= 0) {
		perror("fread");
		fprintf(stderr, "Unable to read from VSAM dataset\n");
	}
	return rc;
}

static int vsamwrite(const char* buffer, size_t numbytes, FILE* fp) {
	int rc=fwrite(buffer, 1, numbytes, fp);
	if (rc != numbytes) {
		perror("fwrite");
		fprintf(stderr, "Unable to write to VSAM dataset\n");
	}
	return rc;
}

static int setxfield(FixedHeader_T* hdr, unsigned short* xoffsetp, unsigned short* xlenp, unsigned short xlen, const char* xfield) {
	char* buffer = (char*) hdr;
	size_t bufferLen = (hdr->verXLen + hdr->relXLen + hdr->modXLen + hdr->keyXLen + hdr->valXLen);
	buffer += sizeof(FixedHeader_T);
	if (bufferLen + xlen > MAX_RECLEN) {
		fprintf(stderr, "Total length of new record would exceed maximum record length. copy failed\n");
		return 0;
	}
	*xoffsetp = bufferLen;
	*xlenp = xlen;
	memcpy(&buffer[*xoffsetp], xfield, xlen);
	return xlen;
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
                default:
                        fprintf(stderr, "TBD: Implement setfield of other fields\n");
	                exit(16);
        }
}

static int cmpxfield(FixedHeader_T* hdr, unsigned short xoffsetp, unsigned short xlen, unsigned short userlen, const char* userkey) {
	char* buffer = (char*) hdr;

	if (xlen != userlen) {
		return PartialMatch;
	}

	buffer += sizeof(FixedHeader_T);
	if (!memcmp(&buffer[xoffsetp], userkey, userlen)) {
		return FullMatch;
	} else {
		return PartialMatch;
	}
}

static int cmpfield(FixedHeader_T* hdr, const char* vsamfield, const char* userfield, size_t userlen) {
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

static KeyMatch_T vsamxmatch(FixedHeader_T* hdr, const char* field, size_t len, VSAMField_T fieldName) {
	KeyMatch_T fieldCheck;
	switch (fieldName) {
		case KeyField:
			if (len >= FIXED_KEY_SIZE) {
				fieldCheck = cmpfield(hdr, hdr->key, field, FIXED_KEY_SIZE-1);
				if (fieldCheck == FullMatch) {
					return cmpxfield(hdr, hdr->keyXOffset, hdr->keyXLen, len-FIXED_KEY_SIZE+1, &field[FIXED_KEY_SIZE-1]);
				} else {
					return fieldCheck;
				}
			} else {
				return cmpfield(hdr, hdr->key, field, len);
			}
		default:
			fprintf(stderr, "TBD: Implement extended match of other fields\n");
			exit(16);
	}
}
	
static FixedHeader_T* vsamxlocate(FILE* fp, char* buffer, char** argv, Options_T* opt, VSAMField_T fieldName, size_t* reclen) {
	FixedHeader_T* hdr = (FixedHeader_T*) buffer;	
	char* vsamfield;
	char* userfield;
	size_t vsamfieldlen;
	size_t userlen;
	int rc;
	KeyMatch_T result = PartialMatch;

	memset(hdr, 0, sizeof(FixedHeader_T));
	*reclen = 0;
	switch (fieldName) {
		case KeyField:
			vsamfield = hdr->key;
			userfield = &argv[opt->keyValIndex][opt->keyOffset];
			userlen = opt->keyLen;
			vsamfieldlen = (userlen >= FIXED_KEY_SIZE) ? FIXED_KEY_SIZE-1 : opt->keyLen;
			memcpy(vsamfield, userfield, vsamfieldlen);
			break;
		default:
			fprintf(stderr, "TBD: Implement extended locate of other fields\n");
			exit(16);
	}
	rc = flocate(fp, vsamfield, FIXED_KEY_SIZE-1, __KEY_EQ);
	if (rc) {
		return NULL;
	}
	while (result == PartialMatch) {
		rc = vsamread(hdr, MAX_RECLEN, fp);
		if (rc <= 0) {
			fprintf(stderr, "Unable to read record after flocate of %s successful\n", vsamfield);
			return NULL;
		}
		result = vsamxmatch(hdr, userfield, userlen, fieldName);
		if (result == NoMatch) {
			hdr = NULL;
		}
	}
	*reclen = rc;
	return hdr;
}

static int vsamclose(FILE* fp) {
	int rc = fclose(fp);
	if (rc) {
		/* MSF - TBD - get dataset name here for better diagnostics */
		perror("fclose");
		fprintf(stderr, "Unable to close VSAM dataset\n");
	}
	return rc;
}

static int printxfield(const FixedHeader_T* hdr, const char* fixed, size_t fixedlen, size_t offset, size_t xlen) {
	const char* buffer = (((const char*) hdr) + sizeof(FixedHeader_T));
	int rc;

	if (xlen > 0) {	
		rc = printf("%s%.*s\n", fixed, xlen, &buffer[offset]);
	} else {
		rc = printf("%s\n", fixed);
	}
	return rc;
}
		
static int printfield(FixedHeader_T* hdr, VSAMField_T field) {
	int rc;	
	switch (field) {
                case ValField:
			rc = printxfield(hdr, hdr->val, FIXED_VAL_SIZE-1, hdr->valXOffset, hdr->valXLen);
	          	break;
                case KeyField:
			rc = printxfield(hdr, hdr->key, FIXED_KEY_SIZE-1, hdr->keyXOffset, hdr->keyXLen);
	          	break;
		default:
                        fprintf(stderr, "TBD: Implement print of other fields\n");
	                exit(16);
        }
	return rc;
}

static int setRecord(char* buffer, size_t *bufflen, char** argv, Options_T* opt) {
	/*
	 * MSF - TBD - deal with PVRM and value longer than minimum
	 */
	FixedHeader_T* fh = (FixedHeader_T*) buffer;
	size_t extlen=0;
  
	memset(fh, 0, sizeof(FixedHeader_T));
	extlen += setfield(fh, &argv[opt->keyValIndex][opt->keyOffset], opt->keyLen, KeyField);
	extlen += setfield(fh, &argv[opt->keyValIndex][opt->valOffset], opt->valLen, ValField);
	
	*bufflen = (sizeof(FixedHeader_T) + extlen);

	return 0;
}

static int getKey(char** argv, Options_T* opt) {
	FILE* vsamfp;
	int rc;
	FixedHeader_T* hdr;
	size_t reclen;
	char buffer[MAX_RECLEN];

	vsamfp = vsamopen(opt->vsamCluster, KEY_QUAL, "rb,type=record");
	if (!vsamfp) {
		return 16;
	}
	hdr = vsamxlocate(vsamfp, buffer, argv, opt, KeyField, &reclen);
	if (!hdr) {
		return 4;
	}
	printfield(hdr, ValField);
	if (rc <= 0) {
		return 16;
	}
	rc = vsamclose(vsamfp);
	if (rc) {
		return 16;
	}
	return 0;
}

static int setKey(char** argv, Options_T* opt) {
	FILE* vsamkfp;
	FILE* vsamcfp;
	size_t currreclen;
	size_t newreclen;
	int rc;
	FixedHeader_T* hdr;
	char buffer[MAX_RECLEN];
	char invalidrecord[1] = { 1 };

	vsamkfp = vsamopen(opt->vsamCluster, KEY_QUAL, "rb+,type=record");
	if (!vsamkfp) {
		return 16;
	}
	hdr = vsamxlocate(vsamkfp, buffer, argv, opt, KeyField, &currreclen);
	rc = setRecord(buffer, &newreclen, argv, opt);
	if (rc) {
		fprintf(stderr, "Key/Value information is too large for VSAM record. Maximum Length is %d\n", MAX_RECLEN);
		return 16;
	}
	if (hdr) {
		if (newreclen <= currreclen) {
			if (newreclen < currreclen) {
				memset(&buffer[newreclen], 0, currreclen-newreclen);
			}
			fupdate(buffer, currreclen, vsamkfp);
			return 0;
		} else {
			fupdate(invalidrecord, sizeof(invalidrecord), vsamkfp);
		}
	}
	rc = vsamclose(vsamkfp);
	if (rc) {
		return 16;
	}

	vsamcfp = vsamopen(opt->vsamCluster, CLUSTER_QUAL, "rb+,type=record");
	if (!vsamcfp) {
		return 16;
	}
	rc = vsamwrite(buffer, newreclen, vsamcfp);
	if (rc != newreclen) {
		return 16;
	}
	
	rc = vsamclose(vsamcfp);
	if (rc) {
		return 16;
	}
	return 0;
}

int main(int argc, char* argv[]) {
	Options_T opt = { 0 };
	int rc;

	if (rc=processArgs(argc, argv, &opt)) {
		return rc;	
	}
	if (opt.get) {
		rc=getKey(argv, &opt);
	} else if (opt.set) {
		rc=setKey(argv, &opt);
	} else {
		fprintf(stderr, "Key not specified\n");
		return syntax(argv[0]);
	}
	return rc;
}
