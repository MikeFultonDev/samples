Building SMS samples:

./build.sh

Running SMS samples:

- make sure xsysvar is in your PATH, a database has been created and the following variables are set:
  - EDU1H01_HLQ (ISMF high level qualifier)
  - HIF7R02_HLQ (ISPF high level qualifier)
- On a zD&T system, you would set these high level qualifiers to SYS1 and ISP, respectively, e.g:
  - EDU1H01_HLQ=SYS1	
  - HIF7R02_HLQ=ISP

- add the 'bin' directory to your PATH to run smssg and smssc easier

smssg -l 
- list out the storage groups       

smssc -l
- list out the storage classes

To see a minimal report, pipe the output through smsrptmin, e.g.

smssc -l | smsrptmin
