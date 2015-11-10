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

#ifndef MAC_MACA_H
#define MAC_MACA_H

#define MACA_EXTRA_DELAY 100

/* This vacation only applies to broadcasted packets. */
#define MACA_VACATION   100

#define MACA_BO_MIN     (20 * MICRO_SECOND)
#define MACA_BO_MAX     (16 * MACA_BO_MIN)

typedef enum
{
    MACA_RTS,
    MACA_CTS,
    MACA_UNICAST,
    MACA_BROADCAST
} MacaFrameType;


typedef enum
{
    MACA_S_PASSIVE,
    MACA_S_RTS,
    MACA_S_BACKOFF,
    MACA_S_REMOTE,
    MACA_S_XMIT,
    MACA_S_YIELD,
    MACA_S_IN_XMITING_RTS,
    MACA_S_IN_XMITING_CTS,
    MACA_S_IN_XMITING_UNICAST,
    MACA_S_IN_XMITING_BROADCAST
} MacaStateType;


#define MACA_TIMER_SWITCH       0x1     /* bit 0000 0001 is used for ON/OFF*/
#define MACA_TIMER_ON           0x1
#define MACA_TIMER_OFF          0x0

#define MACA_TIMER_TYPE 0xE     /* bit 0000 1110 is used for Timer type */
#define MACA_T_RTS      0x0     /* bit 0000 0000 */
#define MACA_T_BACKOFF  0x2     /* bit 0000 0010 */
#define MACA_T_XMIT     0x4     /* bit 0000 0100 */
#define MACA_T_REMOTE   0x6     /* bit 0000 0110 */
#define MACA_T_YIELD    0x8     /* bit 0000 1000 */
#define MACA_T_UNDEFINED 0xE    /* bit 0000 1110 */

typedef struct _maca_timer
{
    int            seq;
    unsigned char   flag;
} MacaTimer;

typedef struct maca_header_str {
    Mac802Address sourceAddr;
    Mac802Address destAddr;
    int payloadSize;
    int frameType;  /* RTS, CTS, MAC_DATA */
    int priority;
} MacaHeader;

typedef struct struct_mac_maca_str
{
    MacData* myMacData;
    int state;

    int BOmin;          /* minimum backoff */
    int BOmax;          /* maximum backoff */
    int BOtimes;        /* how many times has it backed off? */

    MacaTimer timer;

    int payloadSizeExpected;

    int pktsToSend;
    int pktsLostOverflow;

    int pktsSentUnicast;
    int pktsSentBroadcast;

    int pktsGotUnicast;
    int pktsGotBroadcast;

    int RtsPacketSent;
    int CtsPacketSent;

    int RtsPacketGot;
    int CtsPacketGot;

    int NoisyPacketGot;

    TosType currentPriority;
    Message *currentMessage;
    Mac802Address currentNextHopAddress;

    // For setting random backoff and yield.
    RandomSeed backoffSeed;
    RandomSeed yieldSeed;
} MacDataMaca;


/*
 * FUNCTION    MacMacaLayer.
 * PURPOSE     Models the behaviour of the MAC layer with the MACA protocol
 *             on receiving the message enclosed in msg.
 *
 * Parameters:
 *     node:     node which received the message.
 *     msg:      message received by the layer.
 */

void MacMacaLayer(
   Node *node, int InterfaceIndex, Message *msg);


/*
 * FUNCTION    MacMacaInit.
 * PURPOSE     Initialization function for Maca protocol of MAC layer.
 *
 * Parameters:
 *     node:      node being initialized.
 *     nodeInput: structure containing contents of input file.
 */

void MacMacaInit(
   Node *node, int interfaceIndex, const NodeInput *nodeInput);


/*
 * FUNCTION    MacMacaFinalize.
 * PURPOSE     Called at the end of simulation to collect the results of
 *             the simulation of MACA protocol of the MAC Layer.
 *
 * Parameter:
 *     node:     node for which results are to be collected.
 */

void MacMacaFinalize(Node *node, int interfaceIndex);

/*
 * FUNCTION    MacMacaNetworkLayerHasPacketToSend.
 * PURPOSE     For notifying MACA that a new packet at the network layer
 *             is ready to be sent.
 */

void MacMacaNetworkLayerHasPacketToSend(Node* node, MacDataMaca* maca);


void MacMacaReceivePacketFromPhy(
    Node* node, MacDataMaca* maca, Message* msg);


void MacMacaReceivePhyStatusChangeNotification(
   Node* node,
   MacDataMaca* maca,
   PhyStatusType oldPhyStatus,
   PhyStatusType newPhyStatus);


#endif //_MACA_H_

