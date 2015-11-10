#!/bin/bash

sed -i -e \
"1a CACHE_SIZE 500       # pkts per node \n\
CCN_METHOD NO_VIDEO    # DEFAULT or PRIOR or PROPOSAL \n\
VIDEO_RATE 300        # kbps \n\
COMMON_RATE 300       # kbps \n\
ERROR_RATE 0.3        # %" $1

sed -i -e \
"/Application/a APP-CONFIG-FILE   ./node_role.app" $1

# ***** QualNet Configuration File *****
#CCN_METHOD NO_VIDEO    # DEFAULT or PRIOR or PROPOSAL
#VIDEO_RATE 300        # kbps
#COMMON_RATE 300       # kbps
#ERROR_RATE 0.3        # %

#*******************Application Layer***********************************
#APP-CONFIG-FILE   ./node_role.app
