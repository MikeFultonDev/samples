#ifndef __WRAPPERS_H__		
	#define __WRAPPERS_H__		
                                
	#ifdef _LP64
		/*
		 * Unfortunately, right now we need to 'lie' and say that a 31-bit OS linkage assembler routine
		 * is data to keep the compiler/linker happy, which means we can't prototype the function
		 */
		#pragma variable(SVC99MSG, NORENT)
		extern int SVC99MSG;
		#define SVC99MSG(parms)	call31asm("SVC99MSG", &SVC99MSG, 1, parms)
	#else
		#pragma linkage(SVC99MSG, OS)
		int SVC99MSG(char* __ptr32 parms);
	#endif
#endif  
