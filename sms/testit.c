#include "sms.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
	int rc;
	char* input = "LISTALC";
	char* output;
	char* opts[] = { NULL }; 
	rc = batchsms(input, &output, opts);

	printf(output);
}
