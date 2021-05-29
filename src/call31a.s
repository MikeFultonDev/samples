CALL31A CSECT      ,
CALL31A AMODE      64
CALL31A RMODE      ANY
*
* Specialized linkage for CALL31A
* R1 ->
*           @fn           ->  31-bit assembler routine to call        *
*         ---------------                                             *
*           @dsa          ->  31-bit storage to use as stack frame    *
*         ---------------                                             *
*           @parms        ->  31-bit parameter list for target routine*
*         ---------------
         LG         R15,0(,R1)       # target fn pointer
         LGR        R0,R13           # save R13 into R0
         LG         R13,8(,R1)       # load 31-bit DSA into R13
         LG         R1,16(,R1)       # load 31-bit R1 into R1
         STG        R0,0(,R13)       # store old R13 (dsa)
         STG        R14,8(,R13)      # store old R14 (return address)
         LA         R13,16(,R13)     # move 31-bit DSA to user DSA
         ST         R13,76(,R13)     # store NAB of DSA
*
         OILH       R15,X'8000'      # set bit 32 on in target fn
         BASSM      R14,R15          # branch-and-set to 31-bit mode
*
* Reload 64-bit R13 from start of low memory storage area
*


         AHI        R13,-16
         LG         R14,8(,R13)      # restore R14 return address
         LG         R13,0(,R13)
*
* Abbreviated epilog
*
         BR         R14
         LTORG
         DROP
*
         YREGS
*
         END
