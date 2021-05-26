#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "wrappers.h"

#define DEBUG_CALL31ASM 0
#ifdef _LP64
	/*
	 * eventually move the DSA allocation and parms allocation to a per-thread allocation and re-use
	 */
	#define MAX_DSA_SIZE 4096

	#pragma linkage(CALL31A, OS)
	int CALL31A(int* fn, char* dsa, unsigned int* parms);

	int call31asm(const char* fn_name, int* fn, size_t num_parms, ...) {
		   va_list args;
		   size_t i;
		   int rc;
		   unsigned int* r1_31bit_parms = __malloc31(num_parms*sizeof(unsigned int));
		   char* r13_31bit_dsa = __malloc31(MAX_DSA_SIZE);

		   va_start(args, num_parms);
#if DEBUG_CALL31ASM
		   fprintf(stderr, "Function Name: %s Function Pointer:0x%p", fn_name, fn);
		   fprintf(stderr, "Number of parameters:%d Parm list:%p New DSA:%p", num_parms, r1_31bit_parms, r13_31bit_dsa);
#endif
		   for (i=0; i<num_parms; ++i) {
			   unsigned int parm = va_arg(args, unsigned int);
			   r1_31bit_parms[i] = parm;
#if DEBUG_CALL31ASM
			   fprintf(stderr, "Parameter %d: 0x%8.8x", i, parm);
#endif
		   }
		   va_end(args);

		   rc = CALL31A(fn, r13_31bit_dsa, r1_31bit_parms);

#if DEBUG_CALL31ASM
		   fprintf(stderr, "Return code:%d", rc);
#endif

		   if (r1_31bit_parms) free(r1_31bit_parms);
		   free(r13_31bit_dsa);

		   return rc;
	}
#endif
