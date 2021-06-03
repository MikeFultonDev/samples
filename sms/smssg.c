#include "sms.h"

int main(int argc, char* argv[]) {
	SMS* sms;
	int rc, shutrc;

	sms = crtSMS(SMSStorageGroup, argc, argv);
	sms->parsearg(sms);
	if (sms->inerr(sms)) {
		sms->prterr(sms);
		return sms->rc(sms);
	}
	sms->runsvc(sms);
	if (sms->inerr(sms)) {
		sms->prterr(sms);
	}
	return sms->rc(sms);
}
