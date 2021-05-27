#include "svc99.h"

#define FAILLIB "SYS1.LINKLIB"
#define PASSLIB "SYS1.MACLIB"

static int alloc(SVC99CommonTextUnit_T* dsn, SVC99CommonTextUnit_T* dd, SVC99CommonTextUnit_T* disp) {
	SVC99_T* __ptr32 parms;
	SVC99Verb_T verb = S99VRBAL;
	SVC99Flag1_T s99flag1 = {0};
	SVC99Flag2_T s99flag2 = {0};
	SVC99RBX_T s99rbx = {"S99RBX",S99RBXVR,{0,1,0,0,0,0,0},0,0,0};
	size_t numtextunits = 3;
	int rc;

	parms = SVC99init(verb, s99flag1, s99flag2, &s99rbx, numtextunits, dsn, dd, disp );
	if (!parms) {
		fprintf(stderr, "Unable to initialize SVC99 control blocks\n");
		return 16;
	}
	rc = SVC99X(parms);
	if (rc) {
		SVC99fmtdmp(stderr, parms);
                SVC99prtmsg(stderr, parms, rc);
		return rc;
	}
	SVC99free(parms);
	return 0;
}

static int pass(void) {
	SVC99CommonTextUnit_T dsn = { DALDSNAM, 1, sizeof(PASSLIB)-1, PASSLIB };
	SVC99CommonTextUnit_T dd = { DALDDNAM, 1, 6, "DDPASS" };
	SVC99CommonTextUnit_T stats = { DALSTATS, 1, 1, {0x8} };
	printf("Allocate DD to %s - this should pass\n", PASSLIB);
	return alloc(&dsn, &dd, &stats);
}

static int fail(void) {
	SVC99CommonTextUnit_T dsn = { DALDSNAM, 1, sizeof(FAILLIB)-1, FAILLIB };
	SVC99CommonTextUnit_T dd = { DALDDNAM, 1, 3, "DDFAIL" };
	SVC99CommonTextUnit_T disp = { DALNDISP, 1, 1, {0x8} };
	printf("Allocate DD to %s - this should fail\n", FAILLIB);
	return alloc(&dsn, &dd, &disp);
}

int main(int argc, char* argv[]) {
	int rc;

	rc = pass();
	if (rc != 0) {
		fprintf(stderr, "Unexpected failure allocating %s\n", PASSLIB);
		return rc;
	}
	rc = fail();
	if (rc == 0) {
		fprintf(stderr, "Unexpected success allocating %s (should fail)\n", FAILLIB);
		return rc;
	}
	return 0;
}
