
;
ITRACE OFF
;
; ----------------------------------------------------------------------
;
; ----------------------------------------------------------------------
;
; Flush the arp tables every 5 minutes. default is 20 in 3.3
;
ARPAGE 20
;

DATASETPREFIX TCPIP

TCPCONFIG TTLS
SMFCONFIG TYPE119 TCPTERM

; ----------------------------------------------------------------------
;
; AUTOLOG the following servers.
;
AUTOLOG 5
    PAGENT                    ; Policy Agent
ENDAUTOLOG ;
; ----------------------------------------------------------------------
;
; Reserve PORTs for the following servers.
;
; NOTES:
;
;    A port that is not reserved in this list can be used by any user.
;    If you are have TCP/IP hosts in your network that reserve ports
;    in the range 1-1023 for privileged applications, you should
;    reserve them here to prevent users from using them.
;
;    The port values below are from RFC 1060, "Assigned Numbers"
;
; commented out so any proc name can bind
;  111 TCP PMAPOE              ; Portmap Server
;  111 UDP PMAPOE              ; Portmap Server
; 2049 UDP MVSNFS              ; NFS Server

PORT
    20 TCP OMVS      NOAUTOLOG ; FTP Server
    21 TCP OMVS                ; FTP Server
    22 TCP OMVS                ; Secure Shell (SSH)
    23 TCP TN3270              ; TELNET Server
    25 TCP SMTP                ; SMTP Server
    53 TCP NAMESRV             ; Domain Name Server
    53 UDP NAMESRV             ; Domain Name Server
   111 TCP RPCBIND1            ; Portmap Server
   111 UDP RPCBIND1            ; Portmap Server
   135 UDP NCSLLBD             ; NCS Location Broker
   161 UDP SNMPD               ; SNMP Agent
   162 UDP SNMPQE              ; SNMPQE Agent
   512 TCP OMVS                ; REXEC BOSKO
   514 TCP OMVS                ; RSH BOSKO
   515 TCP LPDSRV              ; LPD Server
   520 UDP ROUTED              ; RouteD Server
   750 TCP MVSKERB             ; Kerberos
   750 UDP MVSKERB             ; Kerberos
   751 TCP ADM@SRV             ; Kerberos Admin Server
   751 UDP ADM@SRV             ; Kerberos Admin Server
   992 TCP TN3270              ; Secure TELNET Server
  3000 TCP CICSTCP             ; CICS Socket

;
; ----------------------------------------------------------------------
;
; Hardware definitions:
;

INTERFACE OSA1500
 DEFINE IPAQENET
 IPADDR &HOMEIPADDRESS1
 PORTNAME &OSAPORTNAME
 INBPERF DYNAMIC
 PRIROUTER
;
; ----------------------------------------------------------------------
;
; HOME internet (IP) addresses of each link in the host.
;
; NOTES:
;
;    The IP addresses for the links of an offload box are specified in
;    the LINK statements themselves, and should not be in the HOME list.
;

;HOME
;  &HOMEIPADDRESS1    OSA1500
;
; ---------------------------------------------------------------------
;
; The new PRIMARYINTERFACE statement is used to specify which interface
; is the primary interface.  This is required for specifying a offload
; box as being the primary interface, since the offload box's links
; cannot appear in the HOME statement.
;
; A link of any type, not just an offload box, can be specified in the
; PRIMARYINTERFACE statement.  If PRIMARYINTERFACE not specified, then
; the first link in the HOME statement is the primary interface, as
; usual.
;

PRIMARYINTERFACE OSA1500

;
; ----------------------------------------------------------------------
;
; IP Routing information for the host.  All static IP routes should
; be added here.
;
BEGINROUTES
;     Destination    First Hop    Link       Packet size
ROUTE DEFAULT &DEFAULTROUTEADDR          OSA1500 MTU 1500
ENDROUTES

;
; ----------------------------------------------------------------------
;
; Use TRANSLATE to specify the hardware address of a specific internet
; address.  See the Planning and Customization manual for more
; information
;

;TRANSLATE

;
; ----------------------------------------------------------------------
;
; Start all the defined devices.
;

START OSA1500
