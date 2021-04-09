#include "//'SYS1.SAMPLIB(CSRSIC)'"
#include <cvt.h>
#include <ihaecvt.h>
#include <stdio.h>

struct ihaipa {
	char ipaid[4];
	char _1[28];
	char ipalpnam[8];
	char _2[312];
	char ipasxnam[8];
};

struct psa { 
	char _1[16];
	struct cvtmap* cvt;
};
	
static char* reverse(char* time) {
	int i;
	for (i=0; i<8; ++i) {
		char tmp = time[i];
		time[i] = time[15-i];
		time[15-i] = tmp;    
	}
	return time;
}

static char nibble2x(unsigned char in) {
	return (in < 10 ? '0' + in : 'A' + (in-10));
}

static char* fmttime(const char* time, char* buffer) {
	int i;
	int o=0;
	for (i=0; i<16; ++i) {
		unsigned char nibble;
		nibble = ((unsigned char) time[i]) >> 4;
		buffer[o++] = nibble2x(nibble);
		nibble = ((unsigned char) time[i]) & 0xF;
		buffer[o++] = nibble2x(nibble);
	}
	return buffer;
}

#define KEY_SIZE (88)
int key(char* stg) {
	CSRSIRequest     req;
	CSRSIInfoAreaLen len;
	CSRSIReturnCode  rc;
	siv1             v1cpc_machine;
	char             hextime[32+1];
	char             time[16];
	char*            ptime;
	int              src;

	unsigned char*   manufacturer;
	unsigned char*   type;
	unsigned char*   model;
	unsigned char*   plant;
	unsigned char*   sequence;
 
	req = CSRSI_REQUEST_V1CPC_MACHINE;
	len = sizeof(siv1);
	csrsi_byaddr(req, len, &v1cpc_machine, &rc);
	manufacturer = v1cpc_machine.siv1si11v1.si11v1cpcmanufacturer;
	type = v1cpc_machine.siv1si11v1.si11v1cpctype;
	model = v1cpc_machine.siv1si11v1.si11v1cpcmodel;
	plant = v1cpc_machine.siv1si11v1.si11v1cpcplantofmanufacture;
	sequence = v1cpc_machine.siv1si11v1.si11v1cpcsequencecode;

	ptime = time;

	__asm(" STCKE %0" : "=m"(*ptime) : : );

	reverse(time);

	src = sprintf(stg, "%.16s%.4s%.16s%.4s%.16s%.32s", manufacturer, type, model, plant, sequence, fmttime(time, hextime));

	return src;
}

struct ihaipa* ipa() {
	struct psa* psa = 0;
	struct cvtmap* cvt = psa->cvt;
	struct ecvt* ecvt = (cvt->cvtecvt);
	struct ihaipa* ipa = (ecvt->ecvtipa);

printf("CVT Eye Catcher: %4.4s\n", cvt->cvtcvt);
printf("ECVT Eye Catcher: %4.4s\n", ecvt->ecvtecvt);
printf("IPA Eye Catcher: %4.4s\n", ipa->ipaid);
	return ipa;
}

const char* sysplex() {
	return ipa()->ipasxnam;
}	
const char* lpar() {
	return ipa()->ipalpnam;
}	
	
	
int main() {
	char stg[KEY_SIZE+1];
	int rc = key(stg); 
	printf("Key for SYSPLEX:%.8s LPAR:%.8s <%.*s>\n", sysplex(), lpar(), rc, stg);
	return 0;
}
