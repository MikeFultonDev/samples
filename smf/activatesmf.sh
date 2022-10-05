#!/bin/sh
#
# After verifying with checksmf.sh that TYPE 119 is enabled,
# activate your changes
# Determine which entry you need based on which SMFPRMxx parmlib 
# member you updated
opercmd "t smf=$xx ."
