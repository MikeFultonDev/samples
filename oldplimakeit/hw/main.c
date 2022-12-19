 HELLO:   PROCEDURE OPTIONS (MAIN);
 DCL TEST EXTERNAL ENTRY;

 /* A PROGRAM TO PRINT OUT x */
 dcl x fixed bin(31);
 %include "debug.h";
 x = 3;
 call test(x);

 %IF debug='YES' %THEN %DO;
   PUT SKIP LIST('The value of x is');
   PUT SKIP LIST(x);
 %END;

 END HELLO;
