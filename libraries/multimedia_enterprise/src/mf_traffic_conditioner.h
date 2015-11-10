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

//
// Objectives: Support Diffserv Traffic Classification and Conditioning by
//             remarking/dropping based on bandwidth/delay policy
//
// References: RFC 2475

#ifndef MF_TRAFFIC_CONDITIONER_H
#define MF_TRAFFIC_CONDITIONER_H

//----- Start Internal Macro Definations -----//

#define DIFFSERV_NUM_INITIAL_MFTC_INFO_ENTRIES 4

//Difine different types of standard Service Classes
#define   DIFFSERV_DS_CLASS_EF                   46
#define   DIFFSERV_DS_CLASS_AF11                 10
#define   DIFFSERV_DS_CLASS_AF12                 12
#define   DIFFSERV_DS_CLASS_AF13                 14
#define   DIFFSERV_DS_CLASS_AF21                 18
#define   DIFFSERV_DS_CLASS_AF22                 20
#define   DIFFSERV_DS_CLASS_AF23                 22
#define   DIFFSERV_DS_CLASS_AF31                 26
#define   DIFFSERV_DS_CLASS_AF32                 28
#define   DIFFSERV_DS_CLASS_AF33                 30
#define   DIFFSERV_DS_CLASS_AF41                 34
#define   DIFFSERV_DS_CLASS_AF42                 36
#define   DIFFSERV_DS_CLASS_AF43                 38
#define   DIFFSERV_DS_CLASS_BE                   0

#define   DIFFSERV_NUM_INITIAL_MFTC_PARA_ENTRIES 3
#define   DIFFSERV_TIME_SLIDING_WINDOW_LENGTH    1.0
#define   DIFFSERV_CONFORMING                    1
#define   DIFFSERV_PART_CONFORMING               2
#define   DIFFSERV_NON_CONFORMING                3

#define   DIFFSERV_SET_TOS_FIELD(i, tos)    ((i << 2) | ((tos) & 3))
#define   DIFFSERV_UNUSED_FIELD_ADDRESS     INVALID_ADDRESS
#define   DIFFSERV_UNUSED_FIELD             0xff

//----- End Internal Macro Definations -----//


//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_MFTrafficConditionerInit
// PURPOSE:     Initialize the Traffic Classifier/Conditioner
// PARAMETERS:  node - Pointer to the node,
//              nodeInput - configuration Information.
// RETURN:      None.
//--------------------------------------------------------------------------

void DIFFSERV_MFTrafficConditionerInit(
    Node* node,
    const NodeInput* nodeInput);

//--------------------------------------------------------------------------
// FUNCTION    DIFFSERV_MFTrafficConditionerFinalize()
// PURPOSE     Print out Diffserv statistics
// PARAMETERS: node    Diffserv statistics of this node
//--------------------------------------------------------------------------

void DIFFSERV_MFTrafficConditionerFinalize(Node* node);

//--------------------------------------------------------------------------
// FUNCTION:    DIFFSERV_TrafficConditionerProfilePacketAndMarkOrDrop
// PURPOSE :    Analyze outgoing packet to determine whether or not it
//              is in/out-profile, and either mark or drop the packet
//              as indicated by the class of traffic and policy defined
//              in the configuration file.
// PARAMETERS:  node - pointer to the node,
//              ip - pointer to the IP state variables
//              msg - the outgoing packet
//              interfaceIndex - the outgoing interface
//              packetWasDropped - pointer to a BOOL which is set to
//                   TRUE if the Traffic Conditioner drops the packet,
//                   FALSE otherwise (even if the packet was marked)
//--------------------------------------------------------------------------

void DIFFSERV_TrafficConditionerProfilePacketAndMarkOrDrop(
    Node* node,
    NetworkDataIp* ip,
    Message* msg,
    int interfaceIndex,
    BOOL* packetWasDropped);

unsigned char DIFFSERV_ReturnServiceClass(Node* node, int ds);

unsigned char DIFFSERV_ReturnServiceClass_NoNMS(int ds);

void DIFFSERV_TrafficConditionerProfilePacketAndMarkOrDrop(
    Node* node,
    NetworkDataIp* ip,
    Message* msg,
    int interfaceIndex,
    BOOL* packetWasDropped);

static
int DIFFSERV_ReturnConditionClassForPacket(Node* node,
    NetworkDataIp* ip,
    Message* msg,
    int incomingInterface,
    BOOL* foundMatchingConditionClass);

void DIFFSERV_Marker(Node* node,
    int preference,
    unsigned char serviceClass,
    Message* msg);
#endif // MF_TRAFFIC_CONDITIONER_H
