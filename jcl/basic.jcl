//FULTONMA JOB MSGLEVEL=(1,1)
//MYPROG PROC P=IEFBR14,
//        O=''
//PSTEP1 EXEC PGM=&P,PARM='&O'
// IF (RC LT 4) THEN
//PSTEP2 EXEC PGM=IEFBR14,PARM='Y'
// ELSE
//PSTEP2 EXEC PGM=IEFBR14,PARM='N'
// ENDIF
// PEND
//STEP1 EXEC PROC=MYPROG,P=IEFBR14
//STEP2 EXEC PROC=MYPROG,P=IEFBR14,O='LIST'
//*
//* This is a comment
//*
//SETVAR SET O='ANOTHER LIST'
//STEP3 EXEC PGM=IEFBR14,
//       PARM='&O'