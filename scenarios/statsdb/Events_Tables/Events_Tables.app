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

CBR 1 2 200 512 1S 5S 0S PRECEDENCE 0
SUPER-APPLICATION 1 3 DELIVERY-TYPE RELIABLE CONNECTION-RETRY 0 DET 1S START-TIME DET 6S DURATION DET 200S REQUEST-NUM DET 50 REQUEST-SIZE DET 200  REQUEST-INTERVAL DET 1MS REQUEST-TOS PRECEDENCE 0 REPLY-PROCESS NO
SUPER-APPLICATION 1 3 DELIVERY-TYPE UNRELIABLE START-TIME DET 6S DURATION DET 200S REQUEST-NUM DET 50 REQUEST-SIZE DET 200  REQUEST-INTERVAL DET 1MS REQUEST-TOS PRECEDENCE 0 REPLY-PROCESS NO FRAGMENT-SIZE 140
MCBR 1 225.0.0.0 200 512 1S 5S 0S PRECEDENCE 0