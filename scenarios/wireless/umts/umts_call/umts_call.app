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
#
#  To specify the Umts Call application, include the following statement in the 
#  application configuration (.app) file:
#
#     UMTS-CALL <initiator> <receiver> <avg talk time> <start time> <duration>
#
# where
#
#     <initiator> is the Node ID or IP address of the initiator of the call.
#     <receiver> is the Node ID or IP address of the receiver of the call.
#     <avg talk time> is average length of a talk period (the time during
#                     which one party talks and the other party listens).
#                     The length of the talk period is taken from an
#                     exponential distribution with <avg-talk-time> as the mean.
#     <start time> is when to start UMTS-CALL during the simulation.
#     <duration> is the Total length of the call.

UMTS-CALL 11 8 50S 25S 180S
