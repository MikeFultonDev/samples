
 test: procedure ( x );
   dcl x fixed bin(31);
   %include "debug.h";

   %IF debug='YES' %THEN %DO;
     PUT SKIP LIST('The value of x in the test procedure is');
     PUT SKIP LIST(x);
   %END;
   x=x+1;
 
 end test;
