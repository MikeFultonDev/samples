#/bin/sh
subtypes='95 96 97 98'
for subtype in $subtypes; do
  tsocmd "RDEFINE FACILITY BPX.SMF.119.$subtype UACC(NONE)"
  tsocmd "PERMIT BPX.SMF.119.$subtype CLASS(FACILITY) ID(SSHDAEM) ACCESS(READ)"
  tsocmd "PERMIT BPX.SMF.119.$subtype CLASS(FACILITY) ID(SSHD) ACCESS(READ)"
done
tsocmd "SETROPTS RACLIST(FACILITY) REFRESH"
