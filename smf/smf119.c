/* smf119_2.c convert SMF type 119 subtype 2 records to CSV */

/*
 * To compile:
 *
 * $ xlc -qasm -qasmlib=sys1.maclib -I "//'TCPIP.SEZANMAC'" -o smf119_2 smf119_2.c
 *
 */

#define _LARGE_TIME_API
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define __IPV6
#include "ezasmf.h"

struct SMF119SSH_LF {
    char SMF119SSH_LFRIP[16]; /* Remote IP address */
    char SMF119SSH_LFLIP[16]; /* Local IP address */
    unsigned short SMF119SSH_LFRPort; /* Remote port number (client) */
    unsigned short SMF119SSH_LFLPort; /* Local port number (server) */
    char SMF119SSH_LFUserID[8]; /* User name (login name) on server */
    unsigned short SMF119SSH_LFReason; /* Login failure reason (only for subtype 98) */
    unsigned short reserved; 
} ; 

#define NUL     '\0'
/* macro to load a character field into a nul-terminated string */
#define GRAB(A, B, C) { \
    memcpy((A), (B), (C)); \
    (A)[(C)] = NUL; \
}

#define MAXBUF  65536

#define SSHD_TASK "SSHD"

static void rtrim(char *s)
{
    char    *p;

    p = s + strlen(s);
    while (--p >= s && isspace(*p)) *p = NUL;

    return;
}

static void formatIPV4(char *text, unsigned char *ip)
{
    sprintf(text, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

static void formatIPV6(char *text, unsigned char *ip)
{
    char unpadtext[32];

    if (*(unsigned short *)(ip + 0)  == 0      &&
        *(unsigned short *)(ip + 2)  == 0      &&
        *(unsigned short *)(ip + 4)  == 0      &&
        *(unsigned short *)(ip + 6)  == 0      &&
        *(unsigned short *)(ip + 8)  == 0      &&
        *(unsigned short *)(ip + 10) == 0xffff ) {
        formatIPV4(unpadtext, ip + 12);
    }
    else {
        sprintf(unpadtext, "%x:%x:%x:%x:%x:%x:%x:%x",
                *(unsigned short *)(void *)(ip +  0),
                *(unsigned short *)(void *)(ip +  2),
                *(unsigned short *)(void *)(ip +  4),
                *(unsigned short *)(void *)(ip +  6),
                *(unsigned short *)(void *)(ip +  8),
                *(unsigned short *)(void *)(ip + 10),
                *(unsigned short *)(void *)(ip + 12),
                *(unsigned short *)(void *)(ip + 14));
    }
    sprintf(text, "%31s", unpadtext); 
}

static void stck2tv64(uint8_t *stck, struct timeval64 *ptv)
{
    uint8_t     convval[16], plist[256];
    int         base, rc;
    struct tm   tm;
    time64_t    t;

    __asm(" BALR  %[base],0                   Load next addr into base  \n"
          " USING *,%[base]                   Set base register         \n"
          " STCKCONV STCKVAL=%[tod],CONVVAL=%[conv],TIMETYPE=DEC,DATETYPE=YYYYMMDD,MF=(E,(%[prm]))\n"
          " DROP  %[base]                     Drop base register          "
          :        "=NR:r15"(rc),
            [base]      "=r"(base),
            [conv]      "=m"(convval[0])
          : [tod]        "m"(stck[0]),
            [prm]        "r"(plist)
          : "r0", "r1", "r14", "r15");
    tm.tm_sec = (convval[2] / 16) * 10 + (convval[2] % 16);
    tm.tm_min = (convval[1] / 16) * 10 + (convval[1] % 16);
    tm.tm_hour = (convval[0] / 16) * 10 + (convval[0] % 16);
    tm.tm_mday = (convval[11] / 16) * 10 + (convval[11] % 16);
    tm.tm_mon = (convval[10] / 16) * 10 + (convval[10] % 16) - 1;
    tm.tm_year = (convval[8] / 16) * 1000 + (convval[8] % 16) * 100 + (convval[9] / 16) * 10 + (convval[9] % 16) - 1900;
    tm.tm_isdst = -1;
    t = mktime64(&tm);
    ptv->tv_sec = t;
    ptv->tv_usec = (convval[3] / 16) * 100000 + (convval[3] % 16) * 10000 + (convval[4] / 16) * 1000 + (convval[4] % 16) * 100 + (convval[5] / 16) * 10 + (convval[5] % 16);
}

static char *excelTime(struct timeval64 *ptv)
{
    struct tm   *ptm;
    static char temp[64];

    ptm = localtime64(&ptv->tv_sec);
    sprintf(temp, "%04u-%02u-%02u %02u:%02u:%02u.%06u",
            ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
            ptm->tm_hour, ptm->tm_min, ptm->tm_sec, ptv->tv_usec);

    return temp;
}

static double timeDifference(struct timeval64 *ptv1, struct timeval64 *ptv0)
{
    double  diff;

    diff = ptv1->tv_sec - ptv0->tv_sec +
            (ptv1->tv_usec - ptv0->tv_usec) / 1000000.0;

    return diff;
}

static void formatTime(char *text, int time)
{
    int     hh, mm;
    double  ss;

    hh = time / 360000;
    mm = (time - 360000 * hh) / 6000;
    ss = (double)(time - 360000 * hh - 6000 * mm);
    ss /= 100.0;
    sprintf(text, "%02u:%02u:%05.2f", hh, mm, ss);
}

static int dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static void formatDate(char *text, int date)
{
    int             c, y1, y2, ccyy, d1, d2, d3, ddd, dd, mm, mdays;
    unsigned char   *pdate;

    pdate = (unsigned char *)&date;
    c = pdate[0] & 0x0f;
    y1 = (pdate[1] & 0xf0) >> 4;
    y2 = pdate[1] & 0x0f;
    ccyy = (19 + c) * 100 + y1 * 10 + y2;
    d1 = (pdate[2] & 0xf0) >> 4;
    d2 = pdate[2] & 0x0f;
    d3 = (pdate[3] & 0xf0) >> 4;
    ddd = d1 * 100 + d2 * 10 + d3;
    mm = 0;
    while (ddd > 0) {
        mdays = dim[mm];
        if (mm == 1 && (ccyy % 4) == 0) {
            mdays++;
        }
        dd = ddd;
        ddd -= mdays;
        mm++;
    }
    sprintf(text, "%u-%02u-%02u", ccyy, mm, dd);
}

static void printHeader(struct Smf119Header *hdr, FILE* fp)  
{
    char hdsid[5], thetime[13], thedate[12];
    
    GRAB(hdsid, hdr->SMF119HDSID, 4); 
    rtrim(hdsid);
    formatTime(thetime, hdr->SMF119HDTime);
    formatDate(thedate, hdr->SMF119HDDate);

    fprintf(fp, "%11s %12s %4s %10d ", thedate, thetime, hdsid, hdr->SMF119HDSubType);
}

static void printIdent(struct SMF119Ident* ident, FILE* fp)
{
    char sysname[9], sysplexname[9];

    GRAB(sysname, ident->SMF119TI_SYSName, 8); 
    rtrim(sysname);
    GRAB(sysplexname, ident->SMF119TI_SysplexName, 8); 
    rtrim(sysplexname);

    fprintf(fp, "%8s %8s ", sysname, sysplexname);
}

static void printTerm(struct SMF119AP_TT *term, FILE* fp) {
    char remote[32], start[28], end[28];
    struct timeval64 tvStart, tvEnd;

    formatIPV6(remote, term->SMF119AP_TTRIP);
    stck2tv64(term->SMF119AP_TTSSTCK, &tvStart);
    strcpy(start, excelTime(&tvStart));
    stck2tv64(term->SMF119AP_TTESTCK, &tvEnd);
    strcpy(end, excelTime(&tvEnd));

    fprintf(fp, "%31s %10u %27s %27s\n", remote, term->SMF119AP_TTRPort, start, end);
}

static void printLogin(struct SMF119SSH_LF *login, FILE* fp) {
    char remote[32];
    char user[9];
	
    formatIPV6(remote, login->SMF119SSH_LFRIP);
    GRAB(user, login->SMF119SSH_LFUserID, 8); 
    rtrim(user);

    fprintf(fp, "%31s %10u %8s\n", remote, login->SMF119SSH_LFRPort, user);
}


static int process(struct Smf119Header *hdr, FILE* fp)
{
    struct SMF119SDefSect *sds;
    struct SMF119Ident *ident;
    struct SMF119AP_TT *term;
    struct SMF119SSH_LF *login;
    char resname[9];
    char* buf = (char*) hdr;
    int processed = 0;

    /* self-defining section */
    sds = (struct SMF119SDefSect *)(buf + sizeof(struct Smf119Header));
    ident = (struct SMF119Ident *)(buf + sds->SMF119IDOff);
 
    switch(hdr->SMF119HDSubType) {
        case SMF119HDST_TCPTerm:
            term = (struct SMF119AP_TT *) (buf + sds->SMF119S1Off);
            GRAB(resname, term->SMF119AP_TTRName, 8); 
            rtrim(resname);
            if (!strcmp(resname, SSHD_TASK)) {
                printHeader(hdr, fp);
                printIdent(ident, fp);
                printTerm(term, fp);
            }
            processed = 1;
            break;
        case SMF119HDST_SSH95:
            login = (struct SMF119SSH_LF *) (buf + sds->SMF119S2Off);
            printHeader(hdr, fp);
            printIdent(ident, fp);
            printLogin(login, fp);
            processed = 1;
            break;
        default:
            break;
    }
    return processed;
}

#define MAX_DATASET_LEN 54
int main(int argc, char **argv)
{
    char                infile[MAX_DATASET_LEN+5];
    FILE                *fin;
    unsigned char       buf[SMF_MAX_RECLEN];
    int                 n, n119, n119total;
    struct Smf119Header *phdr;

    /* process command-line arguments */
    if (argc != 2) {
        printf("Usage:  smf119 infile\n");
        return 12;
    }
    size_t len = strlen(argv[1]);
    if (argc != 2 || strlen(argv[1]) >= MAX_DATASET_LEN) {
       fprintf(stderr, "Usage: smf119 <fully qualified SMF dataset>\n");
       return 12;
    }
    sprintf(infile, "//'%s'", argv[1]);

    /* open input file */
    fin = fopen(infile, "rb, type=record");
    if (fin == NULL) {
        perror(infile);
        return 8;
    }

    /* process input file records */
    n = 0;
    n119 = 0;
    n119total = 0;
    while (!feof(fin)) {
        if (fread(buf+4, 1, sizeof(buf), fin) > 0) {  /* skip over LLZZ which fread() doesn't return */
            n++;
            phdr = (struct Smf119Header *)buf;
            if ((phdr->SMF119HDFlags & SMF119HDSub) &&
                (phdr->SMF119HDType == SMF119Type)) {
                n119total++;
                if (process(phdr, stdout)) {
                    n119++;
                } 
            }
        }
    }

    /* clean up */
    fclose(fin);
#if VERBOSE 
    fprintf(stderr, "%10u SMF119    records converted (sub-type 95 and sub-type 2 for SSHD)\n", n119);
    printf(stderr, "%10u SMF119     records read\n", n119total);
    printf(stderr, "%10u * TOTAL *  records read\n", n);
#endif
}

