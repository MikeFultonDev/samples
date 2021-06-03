#include "sms.h"
#include <stdio.h>

static const char* pgmname(SMS* sms) {
	switch (sms->prog) {
		case SMSStorageGroup: return "smssg";
		case SMSStorageClass: return "smssc";
	}
	return "unk";
}

static int invparse(struct SMS* sms) { 
	return 0; 
}
static int invrunsvc(struct SMS* sms) { 
	return 0; 
}
static int invprterr(struct SMS* sms) { 
	fprintf(stderr, "Invalid parameters passed to: %s argc:%d\n", pgmname(sms), sms->argc); 
}
static int invrc(struct SMS* sms) { 
	return 1; 
}
static int invinerr(struct SMS* sms) { 
	return invrc(sms); 
}

static SMS invSMS = { invparse, invrunsvc, invinerr, invprterr, invrc, SMSInvalidProgram, 0, 0 };

SMS* crtSMS(SMSProgram prog, int argc, char* argv[]) {
	SMS* sms = &invSMS;
	invSMS.prog = prog;
	invSMS.argc = argc;
	invSMS.argv = argv;
	return sms;
}
