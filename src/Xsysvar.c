#pragma langlvl(extended)
#define _OPEN_SYS_UNLOCKED_EXT 1
#include <stdio.h>
#include <string.h>

#define MAX_DSNAME_LEN (44)
#define MAX_RECLEN (32761)
#define FIXED_KEY_SIZE (16)
#define FIXED_VAL_SIZE (16)
#define CLUSTER_QUAL ""
#define KEY_QUAL ".KEY.PATH"

const static char DEFAULT_VSAM_CLUSTER[] = "SYS1.XSYSVAR";

typedef struct {
	const char* vsamCluster;
	int keyValIndex;
	size_t keyOffset; 
	size_t valOffset; 
	size_t keyLen; 
	size_t valLen; 
	int get:1;
	int set:1;
} Options_T;

typedef struct {
	char prodID[4];
	char ver[3];
	char rel[3];
	char mod[4];
	char key[FIXED_KEY_SIZE];
	char val[FIXED_VAL_SIZE];
	unsigned short prodIDXLen;
	unsigned short verXLen;
	unsigned short relXLen;
	unsigned short modXLen;
	unsigned short keyXLen;
	unsigned short valXLen;
} FixedHeader_T;

static int syntax(const char* prog) {
	fprintf(stderr, "%s [-?hlXSPVRM] <key>[=<val>]\n", prog);
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

static int processArgs(int argc, char** argv, Options_T* opt) {
	int i;

	opt->vsamCluster=DEFAULT_VSAM_CLUSTER;

	for (i=1; i<argc; ++i) {
		const char* arg = argv[i];
		if (arg[0] == '-') {
			switch (arg[1]) { 
				case 'h':
				case '?':
					if (arg[2] != '\0') {
						fprintf(stderr, "Unknown opt->on:%s\n", arg);
					}
					return(syntax(argv[0]));
					break;
				case 'l':
				case 'X':
				case 'S':
				case 'P':
				case 'V':
				case 'R':
				case 'M':
					if (arg[2] != '\0') {
						fprintf(stderr, "Unknown opt->on:%s\n", arg);
						return(syntax(argv[0]));
					} else {
						fprintf(stderr, "Option %s not implemented yet\n", arg);
						return 4;
					}
					break;
				default:
					fprintf(stderr, "Unknown opt->on:%s\n", arg);
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


static FILE* vsamfopen(const char* dataset, const char* qual, const char* fmt) {
	char mvsname[MAX_DSNAME_LEN+5];
	FILE* fp;

	if (strlen(dataset) > MAX_DSNAME_LEN) {
		fprintf(stderr, "VSAM Cluster key/value dataset name invalid: %s\n", dataset);
		return NULL;
	}
	sprintf(mvsname, "//'%s%s'", dataset, qual);
printf("vsamfopen %s\n", mvsname);
	fp=fopen(mvsname, fmt);
	if (!fp) {
		perror(mvsname);
		fprintf(stderr, "Unable to open VSAM dataset %s for read\n", mvsname);
	}
	return fp;
}

static int vsamfwrite(const char* buffer, size_t numbytes, FILE* fp) {
	int rc=fwrite(buffer, 1, numbytes, fp);
	if (rc != numbytes) {
		perror("fwrite");
		fprintf(stderr, "Unable to write to VSAM dataset\n");
	}
	return rc;
}

static int vsamfclose(FILE* fp) {
	int rc = fclose(fp);
	if (rc) {
		/* MSF - TBD - get dataset name here for better diagnostics */
		perror("fclose");
		fprintf(stderr, "Unable to close VSAM dataset\n");
	}
	return rc;
}

static int setRecord(char* buffer, int *bufferLen, char** argv, Options_T* opt) {
	/*
	 * MSF - TBD - deal with PVRM and key/value longer than minimum
	 */
	FixedHeader_T* fh = (FixedHeader_T*) buffer;
	size_t keyLen;
	size_t valLen;
	char* key;
	char* val;
  
	memset(fh, 0, sizeof(FixedHeader_T));
	keyLen = (opt->keyLen > FIXED_KEY_SIZE) ? FIXED_KEY_SIZE : opt->keyLen;
	valLen = (opt->valLen > FIXED_VAL_SIZE) ? FIXED_VAL_SIZE : opt->valLen;
	key = &argv[opt->keyValIndex][opt->keyOffset];
	val = &argv[opt->keyValIndex][opt->valOffset];

	memcpy(&fh->key, key, keyLen);
	memcpy(&fh->val, val, valLen);
	
	*bufferLen = sizeof(FixedHeader_T);

	return 0;
}

static int getKey(char** argv, Options_T* opt) {
	FILE* vsamfp;
	int rc;
	vsamfp = vsamfopen(opt->vsamCluster, KEY_QUAL, "rb,type=record");
	if (!vsamfp) {
		return 16;
	}
	rc = flocate(vsamfp, &argv[opt->keyValIndex][opt->keyOffset], opt->keyLen, __KEY_EQ);
	if (rc) {
		fprintf(stderr, "<temporary msg>: %.*s not found", opt->keyLen, argv[opt->keyValIndex]);
		return 4;
	}
	rc = vsamfclose(vsamfp);
	if (rc) {
		return 16;
	}
	return 0;
}

static int setKey(char** argv, Options_T* opt) {
	FixedHeader_T hdr = { 0 };
	char buffer[MAX_RECLEN];
	int bufferLen;
	FILE* vsamfp;
	int rc;
	vsamfp = vsamfopen(opt->vsamCluster, KEY_QUAL, "rb+,type=record");
	if (!vsamfp) {
		return 16;
	}
	memcpy(hdr.key, &argv[opt->keyValIndex][opt->keyOffset], opt->keyLen);
	rc = flocate(vsamfp, hdr.key, FIXED_KEY_SIZE, __KEY_EQ);
	if (rc) {
		fprintf(stderr, "<temporary msg>: %.*s not found for 'set'\n", opt->keyLen, argv[opt->keyValIndex]);
	} else {
		fprintf(stderr, "<temporary msg>: %.*s found for 'set'\n", opt->keyLen, argv[opt->keyValIndex]);
		/*
 		 * need to do an fupdate here
		 */
		return 0;
	}
	rc = vsamfclose(vsamfp);
	if (rc) {
		return 16;
	}

	rc = setRecord(buffer, &bufferLen, argv, opt);
	if (rc) {
		fprintf(stderr, "Key/Value information is too large for VSAM record. Maximum Length is %d\n", MAX_RECLEN);
		return 16;
	}
	vsamfp = vsamfopen(opt->vsamCluster, CLUSTER_QUAL, "rb+,type=record");
	if (!vsamfp) {
		return 16;
	}
	rc = vsamfwrite(buffer, bufferLen, vsamfp);
	if (rc != bufferLen) {
		return 16;
	}
	
	rc = vsamfclose(vsamfp);
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
	return 0;
}
