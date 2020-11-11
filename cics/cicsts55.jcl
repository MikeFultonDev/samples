//*********************************************************************
//*                                                                   *
//* @BANNER_START                           01                        *
//* Licensed Materials - Property of IBM                              *
//*                                                                   *
//* 5655-S97              DFHSTART                                    *
//*                                                                   *
//* (C) Copyright IBM Corp. 1991, 2009                                *
//*                                                                   *
//* CICS                                                              *
//* (Element of CICS Transaction Server                               *
//* for z/OS, Version 5 Release 4)                                    *
//* @BANNER_END                                                       *
//*                                                                   *
//* STATUS = 7.2.0                                                    *
//*                                                                   *
//* CHANGE ACTIVITY :                                                 *
//*                                                                   *
//*   $MOD(DFHSTART),COMP(INSTALL),PROD(CICS    ):                    *
//*                                                                   *
//*  PN= REASON REL YYMMDD HDXXIII : REMARKS                          *
//* $01= A29022 650 070221 HD4PALS : Migrate PK29022 from SPA R640    *
//* $L0= Base   321 91     HD3SCWG : Base                             *
//* $L1= 839    630 030619 HDCQMDB : Convert JVM launcher and DTC to  *
//* $L2= 852    640 040813 HD3SCWG : Add MQ library SCSQLOAD          *
//* $L3= 852    640 040918 HD3SCWG : Add SCSQANLE  SCSQCICS  SCSQAUTH *
//* $L4= 807    650 051101 HD3SCWG : Add SEYUAUTH to STEPLIB          *
//* $P1= D06371 630 030321 HDFVGMB : Pull DFHSTART parms and DOCS int *
//* $P2= D07561 630 030516 HDCQMDB : Remove references to DFHJVM      *
//* $P3= D08500 630 030723 HDCQMDB : XPLink library changes           *
//* $P4= D09282 630 031006 HD3SCWG : Increase region size to 64M      *
//* $P5= D18467 650 070417 HD3SCWG : Remove SEYUAUTH                  *
//* $P6= D25567 660 090330 HD4IAEC : Update GCD AMP BUFSP             *
//*                                                                   *
//*********************************************************************
//CICSTS55 PROC START='INITIAL',
// INDEX1='DFH550',
// INDEX2='DFH550.CICS',
// INDEX3='DFH550.CPSM',
// REGNAM='',
// REG='0M',
// DUMPTR='YES',
// RUNCICS='YES',
// OUTC='*',
// SIP=1
//*
//*    INDEX1 - HIGH-LEVEL QUALIFIER(S) OF CICS RUN TIME DATASETS
//*    INDEX2 - HIGH-LEVEL QUALIFIER(S) OF CICS LOAD LIBRARIES
//*    REGNAM - REGION NAME FOR SINGLE OR MRO REGION
//*       REG - MVS REGION STORAGE REQUIRED
//*     START - TYPE OF CICS START-UP REQUIRED
//*    DUMPTR - DUMP/TRACE ANALYSIS REQUIRED, YES OR NO
//*   RUNCICS - CICS STARTUP REQUIRED, YES OR NO
//*      OUTC - PRINT OUTPUT CLASS
//*       SIP - SUFFIX OF DFH$SIP MEMBER IN THE SYSIN DATASET
//*
//* SET THE RETURN CODE TO CONTROL IF CICS SHOULD BE
//* STARTED OR NOT
//CICSCNTL EXEC PGM=IDCAMS,REGION=1M
//SYSPRINT DD SYSOUT=*
//SYSIN    DD DISP=SHR,
// DSN=&INDEX1..SYSIN(DFHRC&RUNCICS)
//*
//* SET THE RETURN CODE TO CONTROL THE DUMP AND TRACE
//* ANALYSIS STEPS
//DTCNTL   EXEC PGM=IDCAMS,REGION=1M
//SYSPRINT DD SYSOUT=*
//SYSIN    DD DISP=SHR,
// DSN=&INDEX1..SYSIN(DFHRC&DUMPTR)
//*
//***********************************************************
//*******************  EXECUTE CICS  ************************
//***********************************************************
//CICS    EXEC PGM=DFHSIP,REGION=&REG,TIME=1440,
// COND=(1,NE,CICSCNTL),
// PARM='START=&START,SYSIN'
//*
//*            THE CAVM DATASETS - XRF
//*
//* THE "FILEA" APPLICATIONS SAMPLE VSAM FILE
//* (THE FILEA DD STATEMENT BELOW WILL
//* OVERRIDE THE CSD DEFINITION IN GROUP DFHMROFD)
//FILEA    DD DISP=SHR,
// DSN=&INDEX1..CICS&REGNAM..FILEA
//*
//SYSIN    DD DISP=SHR,
// DSN=&INDEX1..SYSIN(DFH$SIP&SIP)
//DFHCMACD DD DSN=DFH550.DFHCMACD,DISP=SHR
//***********************************************************
//*        THE CICS STEPLIB CONCATENATION
//*        If Language Environment is required, the SCEERUN2
//*        and SCEERUN datasets is needed in STEPLIB or LNKLST
//***********************************************************
//STEPLIB  DD DSN=&INDEX2..SDFHAUTH,DISP=SHR
//         DD DSN=&INDEX3..SEYUAUTH,DISP=SHR
//         DD DSN=CEE.SCEERUN2,DISP=SHR
//         DD DSN=CEE.SCEERUN,DISP=SHR
//         DD DSN=DFH550.CICS.SDFHLINK,DISP=SHR
//         DD DSN=DFH550.SDFHLIC,DISP=SHR
//***********************************************************
//*        THE CICS LIBRARY (DFHRPL) CONCATENATION
//*        If Language Environment is required, the SCEECICS,
//*        SCEERUN2 and SCEERUN datasets are needed in DFHRPL.
//*        Refer to the Systems Definition Guide for
//*        information on how to run with the native
//*        runtime environments such as VS COBOL II.
//*
//*        If you are using MQ as the transport mechanism
//*        for SIBus uncomment the DD statements for the
//*        SCSQLOAD, SCSQANLE, SCSQCICS and SCSQAUTH datasets.
//***********************************************************
//DFHRPL   DD DSN=&INDEX2..SDFHLOAD,DISP=SHR
//         DD DSN=&INDEX3..SEYULOAD,DISP=SHR
//         DD DSN=CEE.SCEECICS,DISP=SHR
//         DD DSN=CEE.SCEERUN2,DISP=SHR
//         DD DSN=CEE.SCEERUN,DISP=SHR
//         DD DSN=TCPIP.SEZATCP,DISP=SHR
//*        DD DSN=CSQ901.SCSQLOAD,DISP=SHR
//*        DD DSN=CSQ901.SCSQANLE,DISP=SHR
//*        DD DSN=CSQ901.SCSQCICS,DISP=SHR
//*        DD DSN=CSQ901.SCSQAUTH,DISP=SHR
//*        THE AUXILIARY TEMPORARY STORAGE DATASET
//DFHTABLE DD DISP=SHR,DSN=DFH550.SYSIN(DFHPLT)
//DFHTEMP  DD DISP=SHR,
// DSN=&INDEX1..CNTL.CICS&REGNAM..DFHTEMP
//*        THE INTRAPARTITION DATASET
//DFHINTRA DD DISP=SHR,
// DSN=&INDEX1..CNTL.CICS&REGNAM..DFHINTRA
//*        THE AUXILIARY TRACE DATASETS
//DFHAUXT  DD DISP=SHR,DCB=BUFNO=5,
// DSN=&INDEX1..CICS&REGNAM..DFHAUXT
//DFHBUXT  DD DISP=SHR,DCB=BUFNO=5,
// DSN=&INDEX1..CICS&REGNAM..DFHBUXT
//*        THE CICS LOCAL CATALOG DATASET
//DFHLCD   DD DISP=SHR,
// DSN=&INDEX1..CICS&REGNAM..DFHLCD
//*        THE CICS GLOBAL CATALOG DATASET
//DFHGCD   DD DISP=SHR,
// DSN=&INDEX1..CICS&REGNAM..DFHGCD
//*            AMP=('BUFND=33,BUFNI=32,BUFSP=1114112')
//*        THE CICS LOCAL REQUEST QUEUE DATASET
//DFHLRQ   DD DISP=SHR,
// DSN=&INDEX1..CICS&REGNAM..DFHLRQ
//* EXTRAPARTITION DATASETS
//DFHCXRF  DD SYSOUT=&OUTC
//LOGUSR   DD SYSOUT=&OUTC,DCB=(DSORG=PS,RECFM=V,BLKSIZE=136)
//MSGUSR   DD SYSOUT=&OUTC,DCB=(DSORG=PS,RECFM=V,BLKSIZE=140)
//TCPDATA  DD SYSOUT=&OUTC,DCB=(DSORG=PS,RECFM=V,BLKSIZE=136)
//SYSTCPT  DD SYSOUT=&OUTC
//SYSTCPD DD DSN=ADCD.Z24B.TCPPARMS(GBLTDATA),DISP=SHR
//CEEMSG   DD SYSOUT=&OUTC
//CEEOUT   DD SYSOUT=&OUTC
//*        THE DUMP DATASETS
//DFHDMPA  DD DISP=SHR,
// DSN=&INDEX1..CICS&REGNAM..DFHDMPA
//DFHDMPB  DD DISP=SHR,
// DSN=&INDEX1..CICS&REGNAM..DFHDMPB
//SYSABEND DD SYSOUT=&OUTC
//SYSPRINT DD SYSOUT=&OUTC
//PRINTER  DD SYSOUT=&OUTC,DCB=BLKSIZE=121
//*        THE CICS SYSTEM DEFINITION DATASET
//DFHCSD   DD DISP=SHR,
// DSN=&INDEX2..DFHCSD
//* EXECUTE DUMP UTILITY PROGRAM TO PRINT THE
//* CONTENTS OF THE DUMP DATASET A
//PRTDMPA  EXEC  PGM=DFHDU720,PARM=SINGLE,
// REGION=0M,COND=(1,NE,DTCNTL)
//STEPLIB  DD DSN=&INDEX2..SDFHLOAD,DISP=SHR
//DFHTINDX DD SYSOUT=&OUTC
//SYSPRINT DD SYSOUT=&OUTC
//DFHPRINT DD SYSOUT=&OUTC
//DFHDMPDS DD DISP=SHR,
// DSN=&INDEX1..CICS&REGNAM..DFHDMPA
//SYSIN    DD DUMMY
//*        EXECUTE DUMP UTILITY PROGRAM TO PRINT THE
//*        CONTENTS OF THE DUMP DATASET B
//PRTDMPB  EXEC  PGM=DFHDU720,PARM=SINGLE,
// REGION=0M,COND=(1,NE,DTCNTL)
//STEPLIB  DD DSN=&INDEX2..SDFHLOAD,DISP=SHR
//DFHTINDX DD SYSOUT=&OUTC
//SYSPRINT DD SYSOUT=&OUTC
//DFHPRINT DD SYSOUT=&OUTC
//DFHDMPDS DD DISP=SHR,
// DSN=&INDEX1..CICS&REGNAM..DFHDMPB
//SYSIN    DD DUMMY
//*        EXECUTE TRACE UTILITY PROGRAM TO PRINT THE
//*        CONTENTS OF THE AUXILIARY TRACE DATASET 'A'.
//*        THIS DATASET WILL BE EMPTY UNLESS SIT
//*        PARAMETER AUXTR IS SET TO ON.
//PRTAUXT  EXEC PGM=DFHTU720,REGION=0M,COND=(1,NE,DTCNTL)
//STEPLIB  DD DSN=&INDEX2..SDFHLOAD,DISP=SHR
//DFHAUXT  DD DISP=SHR,
// DSN=&INDEX1..CICS&REGNAM..DFHAUXT
//DFHAXPRT DD SYSOUT=&OUTC
//DFHAXPRM DD DUMMY
//*        EXECUTE TRACE UTILITY PROGRAM TO PRINT THE
//*        CONTENTS OF THE AUXILIARY TRACE DATASET 'B'.
//*        THIS DATASET WILL BE EMPTY UNLESS SIT
//*        PARAMETER AUXTR IS SET TO ON.
//PRTBUXT  EXEC PGM=DFHTU720,REGION=0M,COND=(1,NE,DTCNTL)
//STEPLIB  DD DSN=&INDEX2..SDFHLOAD,DISP=SHR
//DFHAUXT  DD DISP=SHR,
// DSN=&INDEX1..CICS&REGNAM..DFHBUXT
//DFHAXPRT DD SYSOUT=&OUTC
//DFHAXPRM DD DUMMY
