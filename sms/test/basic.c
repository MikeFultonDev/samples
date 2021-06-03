#include "sms.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
	int rc;
	char* input = "LISTALC";
	char* output;
	char* smsopts[] = { NULL }; 
	char* xsysvaropts[] = { "Color=Black", NULL };
	rc = batchsms(input, &output, smsopts);

	puts(output);
	free(output);

	rc = xsysvar(&output, xsysvaropts);
	puts(output);
	free(output);
}
