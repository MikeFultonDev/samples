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
         YREGS
         END
