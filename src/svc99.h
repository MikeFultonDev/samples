/*********************************************************************
 * Copyright (c) 2021 IBM
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/
#ifndef __SVC99X__
	#define __SVC99X__ 1
	#include <stdlib.h>
	#include <stdio.h>

	#pragma pack(1)

	typedef enum {
		S99VRBAL = 1, 
		S99VRBUN = 2, 
		S99VRBCC = 3, 
		S99VRBDC = 4, 
		S99VRBRI = 5, 
		S99VRBDN = 6, 
		S99VRBIN = 7 
	} SVC99Verb_T;
	typedef struct {
		int s99oncnv:1;
		int s99nocnv:1;
		int s99nomnt:1;
		int s99jbsys:1;
		int s99cnenq:1;
		int s99gdgnt:1;
		int s99msglo:1;
		int s99nomig:1;
		int s99nosym:1;
		int s99acucb:1;
		int s99dsaba:1;
		int s99dxacu:1;
		int s99rsrv:4;
	} SVC99Flag1_T;
	typedef struct {
		int s99wtvol:1;
		int s99wtdsn:1;
		int s99nores:1;
		int s99wtunt:1;
		int s99offln:1;
		int s99tionq:1;
		int s99catlg:1;
		int s99mount:1;
		int s99udevt:1;
		int s99rsrv1:1;
		int s99dyndi:1;
		int s99tioex:1;
		int s99dasup:1;
		int s99rsrv2:19;
	} SVC99Flag2_T;
		
	typedef struct {         
		unsigned short s99tulng;
		char s99tupar[0];
	} SVC99UnitEntry_T;

	typedef struct {
		unsigned short s99tukey;
		unsigned short s99tunum;
		SVC99UnitEntry_T entry[0];
	} SVC99BasicTextUnit_T;

	typedef struct {
		unsigned short s99tukey;
		unsigned short s99tunum;
		unsigned short s99tulng;
		char s99tupar[256];
	} SVC99CommonTextUnit_T;

	#define DALEROPT_SKIP 0x40

	#define BTOKLEN(member) (sizeof((SVC99BrowseTokenTextUnit_T*) 0)->member)
	#define BTOKIDLEN (BTOKLEN(btokid))
	#define BTOKIOTPLEN (BTOKLEN(btokiotp))
	#define BTOKJKEYLEN (BTOKLEN(btokjkey))
	#define BTOKASIDLEN (BTOKLEN(btokasid))
	#define BTOKRCIDLEN (BTOKLEN(btokrcid))

	#define BTOKACTBUF    0xFFFF
        #define BTOKBRWS 0
        #define BTOKSTKN 3
        #define BTOKVRNM 3

	typedef struct {
		unsigned short s99tukey;
		unsigned short s99tunum;
		unsigned short btokpl1; 
		char btokid[4];
		unsigned short btokpl2;
		unsigned char btoktype;
		unsigned char btokvers;
		unsigned short btokpl3;
		char * __ptr32 btokiotp;
		unsigned short btokpl4;
		unsigned int btokjkey;
		unsigned short btokpl5;
		unsigned short btokasid;
		unsigned short btokpl6;
		char btokrcid[8];
		unsigned short btokpl7;
		unsigned char btoklsdl;
		char btoklsda[254];
	} SVC99BrowseTokenTextUnit_T;

	typedef struct {
		union {
			SVC99CommonTextUnit_T ctu;
			SVC99BasicTextUnit_T btu;
			SVC99BrowseTokenTextUnit_T bttu;
		};
	} SVC99TextUnit_T;

	#define DALDDNAM 0x01
	#define DALDSNAM 0x02
	#define DALSTATS 0x04
	#define DALNDISP 0x05
	#define DALEROPT 0x3D
	#define DALRTDDN 0x55
	#define DALBRTKN 0x6E
	#define DALSSREQ 0x5C

	#define DUNDDNAM 0x01

	typedef struct {
		int s99eimsg:1;
		int s99ermsg:1;
		int s99elsto:1;
		int s99emkey:1;
		int s99emsub:1;
		int s99ewtp:1;
		int s99ersrv:2;
	} SVC99EOpts_T;

	typedef struct {
		int s99xrsrv1:4;
		int s99xseve:1;
		int s99xwarn:1;
		int s99xrsrv2:2;
	} SVC99EMGSV_T;

	#define S99RBXVR 1
	typedef struct {
		char s99eid[6];
		char s99ever;
		SVC99EOpts_T s99eopts;
		char s99esubp;
		char s99ekey;
		SVC99EMGSV_T s99emgsv;
		char s99enmsg;
		void* __ptr32 s99ecppl;
		char s99ercr;
		char s99ercm;
		char s99erco;
		char s99ercf;
		unsigned int s99ewrc;
		void* __ptr32 s99emsgp;
		unsigned short s99eerr;
		unsigned short s99einfo;
		unsigned int s99ersn;
	} SVC99RBX_T;


	typedef struct {
		unsigned char s99rbln;   /* length of request block-20*/
		SVC99Verb_T s99verb;  
        	SVC99Flag1_T s99flag1; 
        	unsigned short s99error;
        	unsigned short s99info;
		SVC99TextUnit_T* __ptr32 * __ptr32 s99txtpp;
		SVC99RBX_T* __ptr32 s99s99x;
        	SVC99Flag2_T s99flag2;  
	} SVC99_T;

	typedef struct {
		unsigned short embufl1;
		unsigned short embufo1;
		char embuft1[251];
		char emrsrv1;
		unsigned short embufl2;
		unsigned short embufo2;
		char embuft2[251];
		char emrsrv2;
	} EMBufs_T;

	typedef struct {
		unsigned short emwtdesc;
		char emwtrtcd[16];
		unsigned short emrsrv;
	} EMWTDERT_T;

	typedef struct {
		int emputlin:1;
		int emwtp:1; 
		int emreturn:1; 
		int emkeep:1; 
		int emwtpcde:1; 
		int emrsrv1:3; 

		char emidnum;
		char emnmsgbk;
		char emsrsrv2;
		SVC99_T * __ptr32 ems99rbp;
		unsigned int emretcod;
		void* __ptr32 emcpplp;
		void* __ptr32 embufp;
		unsigned int emsrsrv3;
		void* __ptr32 emwtpcdp;

		EMWTDERT_T emwtdert;

		EMBufs_T embuf;
	} EMParms_T;

	#pragma pack(reset)

	#define EMDAIR  1
	#define EMFREE  51
	#define EMSVC99 50

	SVC99_T* __ptr32 SVC99init(SVC99Verb_T verb, SVC99Flag1_T flag1, SVC99Flag2_T flag2, SVC99RBX_T* rbx, size_t numtextunits, ...);
	int SVC99X(SVC99_T* __ptr32 parms);
	int SVC99prtmsg(FILE *stream, SVC99_T* __ptr32 parms, int svc99rc);
	void SVC99free(SVC99_T* __ptr32 parms);
	void SVC99fmtdmp(FILE* stream, SVC99_T* __ptr32 parms); 		
#endif
