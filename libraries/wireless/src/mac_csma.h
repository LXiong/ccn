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

#ifndef MAC_CSMA_H
#define MAC_CSMA_H

enum
{
    CSMA_STATUS_PASSIVE,
    CSMA_STATUS_CARRIER_SENSE,
    CSMA_STATUS_BACKOFF,
    CSMA_STATUS_XMIT,
    CSMA_STATUS_IN_XMITING,
    CSMA_STATUS_YIELD
};


#define CSMA_TX_DATA_YIELD_TIME         (20 * MICRO_SECOND) // SNT default
#define CSMA_BO_MIN                     (20 * MICRO_SECOND) // SNT default

// To achieve high bandwidth
// #define CSMA_TX_DATA_YIELD_TIME         (1 * NANO_SECOND)
// #define CSMA_BO_MIN                     (200 * NANO_SECOND)

/* Used to experiment with CSMA timers only. */
#define CSMA_LOCAL_DATA_YIELD_TIME      (0)
#define CSMA_REMOTE_DATA_YIELD_TIME     (0)

#define CSMA_BO_MAX          (16 * CSMA_BO_MIN)

#define CSMA_TIMER_SWITCH    0x1  /* bit 0000 0001 is used for ON/OFF*/
#define CSMA_TIMER_ON        0x1
#define CSMA_TIMER_OFF       0x0

#define CSMA_TIMER_TYPE      0xE  /* bit 0000 1110 is used for Timer type */
#define CSMA_TIMER_BACKOFF   0x0
#define CSMA_TIMER_YIELD     0x2
#define CSMA_TIMER_UNDEFINED 0xE


typedef struct csma_timer
{
    Int32 seq;
    unsigned char flag;
} CsmaTimer;

typedef struct csma_header_str {
    Mac802Address sourceAddr;
    Mac802Address destAddr;
    int priority;
} CsmaHeader;

typedef struct struct_mac_csma_str
{
    MacData* myMacData;

    Int32 status;           /* status of layer CSMA_STATUS_* */
    Int32 BOmin;            /* minimum backoff */
    Int32 BOmax;            /* maximum backoff */
    Int32 BOtimes;          /* how many times has it backoff ? */

    Int32 pktsToSend;
    Int32 pktsLostOverflow;

    Int32 pktsSentUnicast;
    Int32 pktsSentBroadcast;

    Int32 pktsGotUnicast;
    Int32 pktsGotBroadcast;

    CsmaTimer timer;

    RandomSeed seed;        /* for setting backoff timer */
} MacDataCsma;



/*
 * FUNCTION    MacCsmaInit
 * PURPOSE     Initialization function for CSMA protocol of MAC layer.
 *
 * Parameters:
 *     node:      node being initialized.
 *     nodeInput: structure containing contents of input file
 */
void MacCsmaInit(
    Node *node, int interfaceIndex, const NodeInput *nodeInput);


/*
 * FUNCTION    MacCsmaLayer
 * PURPOSE     Models the behaviour of the MAC layer with the CSMA protocol
 *             on receiving the message enclosed in msgHdr.
 *
 * Parameters:
 *     node:     node which received the message
 *     msgHdr:   message received by the layer
 */
void MacCsmaLayer(
    Node *node, int interfaceIndex, Message *msg);


/*
 * FUNCTION    MacCsmaFinalize
 * PURPOSE     Called at the end of simulation to collect the results of
 *             the simulation of CSMA protocol of the MAC Layer.
 *
 * Parameter:
 *     node:     node for which results are to be collected.
 */
void MacCsmaFinalize(Node *node, int interfaceIndex);

/*
 * FUNCTION    MacCsmaNetworkLayerHasPacketToSend
 * PURPOSE     To tell CSMA that the network layer has a packet to send.
 */

void MacCsmaNetworkLayerHasPacketToSend(Node *node, MacDataCsma *csma);


void MacCsmaReceivePacketFromPhy(
    Node* node, MacDataCsma* csma, Message* msg);


void MacCsmaReceivePhyStatusChangeNotification(
   Node* node,
   MacDataCsma* csma,
   PhyStatusType oldPhyStatus,
   PhyStatusType newPhyStatus);

#endif

