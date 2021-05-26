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
         LG         15,0(,1)       # target fn pointer
         LGR        0,13           # save R13 into R0
         LG         13,8(,1)       # load 31-bit DSA into R13
         LG         1,16(,1)       # load 31-bit R1 into R1
         STG        0,0(,13)       # store old R13 (dsa)
         STG        14,8(,13)      # store old R14 (return address)
         LA         13,16(,13)     # move 31-bit DSA to user DSA
         ST         13,76(,13)     # store NAB of DSA
*
         OILH       15,X'8000'     # set bit 32 on in target fn
         BASSM      14,15          # branch-and-set to 31-bit mode
*
* Reload 64-bit R13 from start of low memory storage area
*


         AHI        13,-16
         LG         14,8(,13)      # restore R14 return address
         LG         13,0(,13)
*
* Abbreviated epilog
*
         BR         14
*
         END
