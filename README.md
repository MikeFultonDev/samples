# samples
Sample Code for various things I am working on

## crtzosmfCert
This script will generate a certificate that can be used instead of a userid/password when driving a z/OSMF API.
The certificate is issued for the user running the script, and it resides in RACF (no support for TopSecret or ACF2 at this point).
A browser certificate called zosmf-bin.p12 is generated. 
For syntax, specify:

 `crtzosmfCert -?`

To use the browser certificate:
- download it to your device, in binary format, then import it into your browser (For firefox, go to Preferences / Privacy and Security / View Certificates / Import)

To use the pem certificate and key pair:
 - download the two files to your device, in binary format, then use them (For Python, see [xsysvar.py](./bin/xsysvar.py) )
 
To use the RACF certificate directly on z/OS:
 - specify the user, certificate label, and keyring in your request (For z/OS Toolkit, see [xsysvar](./bin/xsysvar) )
 
These examples require the [zospm tools](https://github.com/zospm/zospm)
