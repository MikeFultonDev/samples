#include <stdio.h>
int main(int argc, char** argv) {
	int c;
	long tot=0;
	long binchar=0;
	long asciichar=0;
	long ebcdicchar=0;

	long const BIN_THRESHOLD=1L;
	long const EBCDIC_THRESHOLD=1L;

	const char* opts;
	int verbose=0;

	/*
	 * msf - tbd - enhance
	 */
	if (argc > 1) {
		opts=argv[1];
		if (!strcmp(opts, "-v")) {
			verbose=1;
		}
	}

	while ((c = getchar()) >= 0) {
		++tot;
		if (c & 0x80) {
			ebcdicchar++;
		} else if (c == 0) {
			binchar++;
		} else {
			asciichar++; 
		}
	}

	if (verbose) {
		fprintf(stderr, "Byte Count: %ld\n", tot);
		fprintf(stderr, "Binary Count: %ld\n", binchar);
		fprintf(stderr, "EBCDIC Count: %ld\n", ebcdicchar);
		fprintf(stderr, "ASCII  Count: %ld\n", asciichar);
	}

	if (tot == 0L) {
		puts("empty");
	} else if (binchar * 100L / tot > BIN_THRESHOLD) {
		puts("binary");
	} else if (ebcdicchar * 100L / tot > EBCDIC_THRESHOLD) {
		puts("ebcdic");
	} else {
		puts("ascii");
	}
	return 0;
}


