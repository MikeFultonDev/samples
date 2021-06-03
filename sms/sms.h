#ifndef __SMS__
	#define __SMS__

	typedef enum {
		SMSInvalidProgram=0,
		SMSStorageGroup=1,
		SMSStorageClass=2
	} SMSProgram;

	typedef struct { char stggrp[8]; } SMSSGOpt;
	typedef struct { char stgcls[8]; } SMSSCOpt;
	typedef struct SMS {
		int (*parsearg)(struct SMS*);	
		int (*runsvc)(struct SMS*);	
		int (*inerr)(struct SMS*);	
		int (*prterr)(struct SMS*);	
		int (*rc)(struct SMS*);	
		void* opts;
		int argc;
		char** argv;
		SMSProgram prog;
	} SMS;

	SMS* crtSMS(SMSProgram prog, int argc, char* argv[]);
	int batchsms(const char* input, char** output, char* opts[]);
#endif
