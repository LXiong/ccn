// Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
//                          600 Corporate Pointe
//                          Suite 1200
//                          Culver City, CA 90230
//                          info@scalable-networks.com
//
// This source code is licensed, not sold, and is subject to a written
// license agreement.  Among other things, no portion of this source
// code may be copied, transmitted, disclosed, displayed, distributed,
// translated, used as the basis for a derivative work, or used, in
// whole or in part, for any program or purpose other than its intended
// use in compliance with the license agreement as part of the QualNet
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

#ifndef _SWITCHED_ETHERNET_H_
#define _SWITCHED_ETHERNET_H_


//
// MacSwitchedEthernetFrameHeader
// It is similar to the 802.3 header, but is 28 bytes
// (2 bytes longer than the 802.3 header and no padding for short packets)
//
typedef struct mac_switchedEthernet_frame_header_str {
    char      framePreamble[8];
    Mac802Address sourceAddr;
    Mac802Address destAddr;
    char      length[2];
    char      checksum[4];
} MacSwitchedEthernetFrameHeader;

typedef struct {
    Int64 packetsSent;
    Int64 packetsReceived;
} SwitchedEthernetStats;

typedef struct {
    MacData *myMacData;

    int status;
    clocktype busyUntil;
    SubnetMemberData *nodeList;
    std::map<Mac802Address, int> *hwAddrMap;
    int numNodesInSubnet;

    SwitchedEthernetStats stats;
} MacSwitchedEthernetData;


void
ReturnSwitchedEthernetSubnetParameters(
    Node* node,
    int interfaceIndex,
    const Address* address,
    const NodeInput* nodeInput,
    Int64 *subnetBandwidth,
    clocktype *subnetPropDelay);

void
SwitchedEthernetInit(
    Node *node,
    const NodeInput* nodeInput,
    int interfaceIndex,
    SubnetMemberData* nodeList,
    int numNodesInSubnet);



void
SwitchedEthernetNetworkLayerHasPacketToSend(
    Node *node,
    MacSwitchedEthernetData *eth);


void
SwitchedEthernetLayer(
    Node *node,
    int interfaceIndex,
    Message *msg);


void
SwitchedEthernetFinalize(
    Node *node,
    int interfaceIndex);



#endif /* _SWITCHED_ETHERNET_H_ */

