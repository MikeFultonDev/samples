#pragma runopts("HEAPCHK(ON),HEAPPOOLS(OFF),STORAGE(FF,FD,FB)")

#include "svc99.h"

#define FAILLIB "SYS1.LINKLIB"
#define PASSLIB "SYS1.MACLIB"

const SVC99RBX_T s99rbxtemplate = {"S99RBX",S99RBXVR,{0,1,0,0,0,0,0},0,0,0};

#pragma noinline(alloc)
static int alloc(SVC99CommonTextUnit_T* dsn, SVC99CommonTextUnit_T* dd, SVC99CommonTextUnit_T* disp) {
	SVC99_T* __ptr32 parms;
	SVC99Verb_T verb = S99VRBAL;
	SVC99Flag1_T s99flag1 = {0};
	SVC99Flag2_T s99flag2 = {0};
	size_t numtextunits = 3;
	int rc;
	SVC99RBX_T s99rbx = s99rbxtemplate;

	if (s99rbx.s99eid[0] != 'S') { printf("oops\n"); }
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
	SVC99CommonTextUnit_T dd = { DALDDNAM, 1, 6, "DDFAIL" };
	SVC99CommonTextUnit_T disp = { DALNDISP, 1, 1, {0x8} };
	printf("Allocate DD to %s - this should fail\n", FAILLIB);
	return alloc(&dsn, &dd, &disp);
}

#pragma noinline(console)
static int console(void) {
	SVC99CommonTextUnit_T ddname = { DALDDNAM, 1, 7, {"DDSPOOL"}};
	SVC99CommonTextUnit_T dsname = { DALDSNAM, 1, 18, {"S0W1.SYSLOG.SYSTEM"}};
	SVC99CommonTextUnit_T dsstat = { DALSTATS, 1, 1, 0x8};
	SVC99CommonTextUnit_T ssreq = { DALSSREQ, 1, 4, {"JES2"}};

	/*
	 * See: https://tech.mikefulton.ca/DALBRTKN
	 */
	SVC99BrowseTokenTextUnit_T brtkn = {
		DALBRTKN, 7, BTOKIDLEN, "BTOK", 2, BTOKSTKN, BTOKVRNM, BTOKIOTPLEN, 0, BTOKJKEYLEN, 0, BTOKASIDLEN, BTOKACTBUF, BTOKRCIDLEN, {0}, 255, {0}
	};

	SVC99CommonTextUnit_T eropt = { DALEROPT, 1, 1, { DALEROPT_SKIP }};
	SVC99_T* __ptr32 parms;
	SVC99Verb_T verb = S99VRBAL;
	SVC99Flag1_T s99flag1 = {0};
	SVC99Flag2_T s99flag2 = {0};
	SVC99RBX_T s99rbx = s99rbxtemplate;
	size_t numtextunits = 6;
	int rc;

	printf("Allocate DD to Master Console %s - this should pass if you have authority (change S0W1 to your system name)\n", dsname.s99tupar);

	parms = SVC99init(verb, s99flag1, s99flag2, &s99rbx, numtextunits, &dsstat, &ddname, &dsname, &ssreq, &brtkn, &eropt);
	if (!parms) {
		fprintf(stderr, "Internal Error: Unable to initialize SVC99 control blocks\n");
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

int main(int argc, char* argv[]) {
	int rc;

	rc = console();
	if (rc != 0) {
		fprintf(stderr, "Unexpected failure allocating DD to master console\n");
		return rc;
	}
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
