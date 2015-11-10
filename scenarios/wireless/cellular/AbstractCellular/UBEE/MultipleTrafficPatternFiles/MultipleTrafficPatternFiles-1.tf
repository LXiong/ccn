# Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
#                          600 Corporate Pointe
#                          Suite 1200
#                          Culver City, CA 90230
#                          info@scalable-networks.com
#
# This source code is licensed, not sold, and is subject to a written
# license agreement.  Among other things, no portion of this source
# code may be copied, transmitted, disclosed, displayed, distributed,
# translated, used as the basis for a derivative work, or used, in
# whole or in part, for any program or purpose other than its intended
# use in compliance with the license agreement as part of the QualNet
# software.  This source code and certain of the algorithms contained
# within it are confidential trade secrets of Scalable Network
# Technologies, Inc. and may not be used as the basis for any other
# software, hardware, product or service.


TRAFFIC-PATTERN     active
NUM-APP-TYPES       1
MAX-NUM-APPS        DET 5 
ARRIVAL-INTERVAL    DET 1M 
PROBABILITY         1.0
RETRY-PROBABILITY   0.5
RETRY-INTERVAL      EXP 20S 
MAX-NUM-RETRIES     5
CELLULAR-ABSTRACT-APP UNI 1 10  DET 1M  DET 0  DET 13

