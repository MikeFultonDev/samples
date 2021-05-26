SVC99M    TITLE 'SVC99M - SVC99 Message Service wrapper'
         SPACE 1
SVC99M   MSFSECT
*                                                                     *
*01* Description = C callable routine to drive IEFDB476 SVC99 msg svc *
*                                                                     *
*01* RECOVERY: None                                                   *
*                                                                     *
*      ENVIRONMENT:  AMODE = 31                                       *
*                    RMODE = ANY                                      *
*                    STATE = ANY                                      *
*                    KEY   = ANY                                      *
*                    RENT  = YES                                      *
*                                                                     *
*                                                                     *
*01*   INPUT/OUTPUT:                                                  *
*   R1 -> ---------------                                             *
*           @parms        ->  address of parms for IEFDB476           *
*         ---------------                                             *
*                                                                     *
*      REGISTER USAGE:                                                *
*               R12 BASE REGISTER FOR LOAD MODULE                     *
*               R13 POINTS TO DYNAMIC AREA                            *
*           ALL OTHERS STANDARD USAGE                                 *
**** END OF SPECIFICATIONS *******************************************/
         ENTRY SVC99MSG
SVC99MSG MSFPRO BASE_REG=12,USR_DSAL=MSG_DSAL
         USING MSG_DSA,R13
         ST    R1,PARMS
         LINK EP=IEFDB476
         MSFEPI
*
         DS    0D
MSG_DSA  DSECT
         DS    CL(120+8)
MSG_TOP  DS    0D
PARMS    DS    A
MSG_DSAL EQU   *-MSG_TOP
         LTORG
         DROP
*----------------------------------------------------------------------
R0       EQU   0
R1       EQU   1
R2       EQU   2
R3       EQU   3
R4       EQU   4
R5       EQU   5
R6       EQU   6
R7       EQU   7
R8       EQU   8
R9       EQU   9
R10      EQU   10
R11      EQU   11
R12      EQU   12
R13      EQU   13
R14      EQU   14
R15      EQU   15
         END
