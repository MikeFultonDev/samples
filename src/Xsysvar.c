#pragma langlvl(extended)
#include <stdio.h>
#include <string.h>

const static char DEFAULT_VSAM_CLUSTER[] = "SYS1.XSYSVAR";

typedef struct {
	int keyValIndex;
	int valOffset; 
	int keyLen; 
	int valLen; 
	int get:1;
	int set:1;
} Options_T;

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
	fprintf(stderr, " The key can be up to 255 characters in length\n");
	fprintf(stderr, " The value can be up to 255 characters in length\n");
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
	for (i=1; i<argc; ++i) {
		const char* arg = argv[i];
		if (arg[0] == '-') {
			switch (arg[1]) { 
				case 'h':
				case '?':
					if (arg[2] != '\0') {
						fprintf(stderr, "Unknown option:%s\n", arg);
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
						fprintf(stderr, "Unknown option:%s\n", arg);
						return(syntax(argv[0]));
					} else {
						fprintf(stderr, "Option %s not implemented yet\n", arg);
						return 4;
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

#define MAX_DSNAME_LEN (44)

static FILE* mvsfopen(const char* dataset, const char* fmt) {
	char mvsname[MAX_DSNAME_LEN+5];

	if (strlen(dataset) > MAX_DSNAME_LEN) {
		fprintf(stderr, "VSAM Cluster key/value dataset name invalid: %s\n", dataset);
		return NULL;
	}
	sprintf(mvsname, "//'%s'", dataset);
	return fopen(mvsname, fmt);
}

int main(int argc, char* argv[]) {
	Options_T opt = { 0 };
	const char* vsamcluster=DEFAULT_VSAM_CLUSTER;
	int rc;
	FILE* vsamfp;
	

	if (rc=processArgs(argc, argv, &opt)) {
		return rc;	
	}
	if (opt.get) {
		vsamfp = mvsfopen(vsamcluster, "rb,type=record");
		if (!vsamfp) {
			perror(vsamcluster);
			fprintf(stderr, "Unable to open VSAM Cluster %s for read\n", vsamcluster);
			return 16;
		}
		rc = flocate(vsamfp, argv[opt.keyValIndex], opt.keyLen, __KEY_EQ);
		if (rc) {
			fprintf(stderr, "<temporary msg>: %.*s not found", opt.keyLen, argv[opt.keyValIndex]);
			return 4;
		}
	} else if (opt.set) {
		vsamfp = mvsfopen(vsamcluster, "rb+,type=record");
		if (!vsamfp) {
			perror(vsamcluster);
			fprintf(stderr, "Unable to open VSAM Cluster %s for update\n", vsamcluster);
			return 16;
		}
		rc = flocate(vsamfp, argv[opt.keyValIndex], opt.keyLen, __KEY_EQ);
		if (rc) {
			fprintf(stderr, "<temporary msg>: %.*s not found for 'set'", opt.keyLen, argv[opt.keyValIndex]);
			return 4;
		}
	} else {
		fprintf(stderr, "Key not specified");
		return 16;
	}
	rc = fclose(vsamfp);
	if (rc) {
		fprintf(stderr, "Unable to close VSAM Cluster %s\n", vsamcluster);
		return 16;
	}
	return 0;
}
