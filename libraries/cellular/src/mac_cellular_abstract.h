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

// /**
// PACKAGE     :: CELLULAR_ABSTRACT_MAC
// DESCRIPTION :: Defines the data structures used in the Abstract Cellular
//                MAC Implementation. Most of the time The data structure
//                and function start with MacCellularAbstract**
// **/

#ifndef _CELLULAR_ABSTRACT_MAC_H_
#define _CELLULAR_ABSTRACT_MAC_H_

#include "cellular.h"
#include "cellular_abstract.h"
#include "simplesplay.h"

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_SIGNAL_RECORD : 6
// DESCRIPTION :: Maximum number of signal strength history data
//                record for each sector
// **/
#define CELLULAR_ABSTRACT_MAX_SIGNAL_RECORD     6

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_SIGNAL_RECORD_EFFECTIVE_TIME : 5S
// DESCRIPTION :: Effective time of a signal strength history data record
// **/
#define CELLULAR_ABSTRACT_MAX_SIGNAL_RECORD_EFFECTIVE_TIME (30 * SECOND)

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_MAX_SIGNAL_RECORD_EFFECTIVE_TIME_COEFFICIENT : 10
// DESCRIPTION :: coefficient for the Effective time of a
//                signal strength history data record
// **/
#define CELLULAR_ABSTRACT_MAX_SIGNAL_RECORD_EFFECTIVE_TIME_COEFFICIENT    10

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_SIGNAL_MEASUREMENT_TIME : 5S
// DESCRIPTION :: Measurement time of the signal strength
// **/
#define CELLULAR_ABSTRACT_SIGNAL_MEASUREMENT_TIME (5 * SECOND)

// /**
// CONSTANT    :: MAC_CELLULAR_ABSTRACT_UPLINK_CONTROL_DURATION : 577US
// DESCRIPTION :: The duration of an uplink control slot
// **/
#define MAC_CELLULAR_ABSTRACT_UPLINK_CONTROL_DURATION (577 * MICRO_SECOND)

// /**
// CONSTANT    :: MAC_CELLULAR_ABSTRACT_UPLINK_CONTROL_INTERVAL : 8 * 577US
// DESCRIPTION :: The interval for the recurrence of uplink control slot
// **/
#define MAC_CELLULAR_ABSTRACT_UPLINK_CONTROL_INTERVAL  \
         (8 * MAC_CELLULAR_ABSTRACT_UPLINK_CONTROL_DURATION)

// /**
// CONSTANT    :: MAC_CELLULAR_ABSTRACT_DOWNLINK_CONTROL_DURATION : 577US
// DESCRIPTION :: The duration of a downlink control slot
// **/
#define MAC_CELLULAR_ABSTRACT_DOWNLINK_CONTROL_DURATION (577 * MICRO_SECOND)

// /**
// CONSTANT    :: MAC_CELLULAR_ABSTRACT_MAC_PROP_DELAY_COMPENSATION : 33US
// DESCRIPTION :: The propagation delay compensation(10Km distance)
// **/
#define MAC_CELLULAR_ABSTRACT_MAC_PROP_DELAY_COMPENSATION (33 * MICRO_SECOND)

// /**
// CONSTANT    :: MAC_CELLULAR_ABSTRACT_DOWNLINK_CONTROL_INTERVAL : 8 * 577US
// DESCRIPTION :: The interval for the recurrence of downlink control slot
// **/
#define MAC_CELLULAR_ABSTRACT_DOWNLINK_CONTROL_INTERVAL  \
            (8 * MAC_CELLULAR_ABSTRACT_DOWNLINK_CONTROL_DURATION)

// /**
// CONSTANT    :: MAC_CELLULAR_ABSTRACT_UPLINK_DOWNLINK_DIFF : 3 * 577US
// DESCRIPTION :: The time difference between uplink slot and downlink slot
//                We assume uplink control slot begins this time period
//                later than the downlink slot.
// **/
#define MAC_CELLULAR_ABSTRACT_UPLINK_DOWNLINK_DIFF  \
            (3 * MAC_CELLULAR_ABSTRACT_DOWNLINK_CONTROL_DURATION)

// /**
// CONSTANT    :: MAC_CELLULAR_ABSTRACT_TCH_DELAY : 8 * 577US
// DESCRIPTION :: The time difference between uplink slot and downlink slot
//                We assume uplink control slot begins this time period
//                later than the downlink slot.
// **/
#define MAC_CELLULAR_ABSTRACT_TCH_DELAY (8 * 577 * MICRO_SECOND)

// /**
// CONSTANT    :: MAC_CELLULAR_ABSTRACT_SIGNAL_MEASUREMENT_SHORT_INTERVAL:1S
// DESCRIPTION :: The short interval for measuring signal levels of BSs.
//                Short interval is used when there is active communication
// **/
#define MAC_CELLULAR_ABSTRACT_SIGNAL_MEASUREMENT_SHORT_INTERVAL (1 * SECOND)

// /**
// CONSTANT    :: MAC_CELLULAR_ABSTRACT_SIGNAL_MEASUREMENT_LONG_INTERVAL : 5S
// DESCRIPTION :: The long interval for measuring signal levels of BSs.
//                Long interval is used when there is no communication
// **/
#define MAC_CELLULAR_ABSTRACT_SIGNAL_MEASUREMENT_LONG_INTERVAL (5 * SECOND)

// /**
// CONSTANT    :: MAC_CELLULAR_ABSTRACT_MAX_BUFFER_SIZE : 50
// DESCRIPTION :: Maximum size of the MAC buffer in terms of # of packets
// **/
#define MAC_CELLULAR_ABSTRACT_MAX_BUFFER_SIZE  50

// /**
// CONSTANT    :: MAC_CELLULAR_ABSTRACT_BS_LIST_INC : 5
// DESCRIPTION :: The initial and incremental size for the BS info list
// **/
#define MAC_CELLULAR_ABSTRACT_BS_LIST_INC  5

// /**
// CONSTANT    :: MAC_CELLULAR_ABSTRACT_BCCH_FRAC : 4
// DESCRIPTION :: How many DL control channel will have 1 BCCH channel
// **/
#define MAC_CELLULAR_ABSTRACT_BCCH_FRAC  4

// /**
// CONSTANT    :: CELLULAR_ABSTRACT_BCCH_INTERVAL : 20MS
// DESCRIPTION :: Interval of BCCH broadcast at a BS
// **/
#define CELLULAR_ABSTRACT_BCCH_INTERVAL  (20 * MILLI_SECOND)


// /**
// ENUM        :: MacCellularAbstractTimerType
// DESCRIPTION :: Type of timers
// **/
typedef enum
{
    UPLINK_CONTROL_SLOT_BEGIN,   // begin of the uplink control slot
    UPLINK_CONTROL_SLOT_END,     // end of the uplink control slot
    DOWNLINK_CONTROL_SLOT_BEGIN,   // begin of the downlink control slot
    DOWNLINK_CONTROL_SLOT_END,   // end of the downlink control slot
} MacCellularAbstractTimerType;

// /**
// ENUM        :: MacCellularAbstractStatus
// DESCRIPTION :: Status of MS
// **/
typedef enum
{
    MAC_CELLULAR_POWER_OFF,    // the MS is powered off
    MAC_CELLULAR_IDLE,
    MAC_CELLULAR_TRANSMITTING,
    MAC_CELLULAR_RECEIVING,
} MacCellularAbstractStatus;


// /**
// STRUCT      :: MacCellularAbstractTimer
// DESCRIPTION :: Structure of the info field of the timer message
// **/
typedef struct
{
    MacCellularAbstractTimerType timerType; // type of the timer
} MacCellularAbstractTimer;

// /**
// STRUCT      :: MacCellularAbstractBSInfo
// DESCRIPTION :: A strucut to keep track information of a BS
// **/
typedef struct
{
    // Node Id of the BS
    NodeId bsNodeId;

    // number of sectors this BS has
    short numSectors;

    // Position of the BS for calculating signal strength level
    Coordinates bsPosition;

    // signal strength level measurement counts
    int numMeasurementInLastInterval;
    clocktype lastMeasurementTime;

    // results from measurement
    double signalStrength; // signal strength
    short sectorId; // sector ID of the BS
} MacCellularAbstractBSInfo;

// /**
// STRUCT      :: MacCellularAbstractHeader
// DESCRIPTION :: MAC header of the cellular abstract MAC
// **/
typedef struct
{
    NodeAddress destId;                 // node ID of destination
    CellularAbstractChannelType chType; // type of the channel
} MacCellularAbstractHeader;

// /**
// STRUCT      :: CellularAbstractDownlinkMeasurement
// DESCRIPTION :: Record the location information of BSs and signal strength
//                measurements of sectors
// **/
typedef struct cellular_abstract_downlink_measurement_str
{
    NodeAddress monitoredBsId;      // node id of the BS
    short   monitoredSectorId;      // sector id
    double receivedSignalStrength;  // signal strength
    short   numMeasurement;         // number of measurements
    clocktype measurementTime;      // measurement time

    // next record
    struct cellular_abstract_downlink_measurement_str *nextMeasure;
} CellularAbstractDownlinkMeasurement;

// /**
// STRUCT      :: MacCellularAbstractStats
// DESCRIPTION :: Statistics of the abstract cellular MAC
// **/
typedef struct
{
    int numPktSentOnDL;  // # of pkts sent on downlink channel (BCCH, PAGCH)
    int numPktRecvOnDL;  // # of pkts recved on downlink channel
    int numPktSentOnUL;  // # of pkts sent on uplink channel (RACCH)
    int numPktRecvOnUL;  // # of pkts recved on uplink channel
    int numPktSentOnTCH; // # of pkts sent on TCH channels
    int numPktRecvOnTCH; // # of pkts recved on TCH channels
} MacCellularAbstractStats;


// /**
// STRUCT      :: MacCellularAbstractData
// DESCRIPTION :: Mac data of the cellular abstract MAC
// **/
typedef struct mac_cellular_abstract_str
{
    MacCellularAbstractStatus status;  // status
    BOOL commActive; // Whether the MS is in communication active mode

    BOOL associated;
    int selectedBsId;
    int selectedSectorId;

    int strongestSignalBsId;
    int strongestSignalSectorId;
    double handoverMargin;
    CellularAbstractDownlinkMeasurement *downlinkMeasurement;

    // list of downlink channels in the PLMN
    // MS will scan these channels for searching the BS.
    int *dlChannelList;
    int numDLChannels;

    // for channel scan to attach
    int scanIndex;
    int numFramePerChannel;
    int frameCount;

    // current uplink and downlink channel
    int currentULChannel;
    int currentDLChannel;

    // variables for helping changing channels in the middle of tx
    BOOL cellReselected;
    int nextULChannel;
    int nextDLChannel;

    // type of the control channel slot
    int chTypeCount;
    CellularAbstractChannelType chType;

    // buffer for holding control packets
    // For BS, packets will be sent on downlink control channel
    // For MS, packets will be sent on uplink control channel
    Message *bufferPtr;
    Message *lastPkt;
    Message *bcchBufferPtr;
    Message *bcchLastPkt;

    // Timer messages for reuse
    Message *dlSlotBeginTimer;
    Message *dlSlotEndTimer;
    Message *ulSlotBeginTimer;
    Message *ulSlotEndTimer;

    RandomSeed seed;

    // splay tree for tch messages
    SimpleSplayTree tchSplayTree;

    // statistics
    MacCellularAbstractStats stats;

    //optimization
    short optLevel;
}MacCellularAbstractData;


//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: MacCellularAbstractInit
// LAYER      :: MAC
// PURPOSE    :: Init Abstract Cellular MAC protocol at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacCellularAbstractInit(Node *node,
                             int interfaceIndex,
                             const NodeInput* nodeInput);

// /**
// FUNCTION   :: MacCellularAbstractFinalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacCellularAbstractFinalize(Node *node, int interfaceIndex);

// /**
// FUNCTION   :: MacCellularAbstractLayer
// LAYER      :: MAC
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void MacCellularAbstractLayer(Node *node, int interfaceIndex, Message *msg);

// /**
// FUNCTION   :: MacCellularAbstractNetworkLayerHasPacketToSend
// LAYER      :: MAC
// PURPOSE    :: Called when network layer buffers transition from empty.
//               This function is not supported in the abstract cellular MAC
// PARAMETERS ::
// + node             : Node*                    : Pointer to node.
// + interfaceIndex   : int                      : interface running this MAC
// + cellularAbstractMac : MacCellularAbstractData* : Pointer to abstract
//                                                    cellular MAC structure
// RETURN     :: void : NULL
// **/
void MacCellularAbstractNetworkLayerHasPacketToSend(
         Node *node,
         int interfaceIndex,
         MacCellularAbstractData *cellularAbstractMac);

// /**
// FUNCTION   :: MacCellularAbstractReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : interface running this MAC
// + msg              : Message*          : Packet received from PHY
// RETURN     :: void : NULL
// **/
void MacCellularAbstractReceivePacketFromPhy(
         Node* node,
         int interfaceIndex,
         Message* msg);

#ifdef PARALLEL //Parallel
// /**
// FUNCTION   :: MacCellularAbstractReceiveTCH
// LAYER      :: MAC
// PURPOSE    :: Receive a TCH signal from a remote partition
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Packet received from PHY
// RETURN     :: void : NULL
// **/
void MacCellularAbstractReceiveTCH(
         Node* node,
         Message* msg);
#endif //endParallel

// /**
// FUNCTION   :: MacCellularScheduleTCH
// LAYER      :: MAC
// PURPOSE    :: Schedule a TCH message
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : TCH message
// + delay            : clocktype         : delay suffered by this message
// RETURN     :: void : NULL
// **/
void MacCellularScheduleTCH(
    Node* node,
    Message* msg,
    clocktype delay);


// /**
// FUNCTION   :: MacCellularScheduleTCH
// LAYER      :: MAC
// PURPOSE    :: Schedule a TCH message
// PARAMETERS ::
// + tree             : SimpleSplayTree*  : Splay tree of events
// + splayNodePtr     : SimpleSplayNode*  : Splay node with message
// + newTime          : BOOL*             : whether this is the first
//                                        : message at the schedule time
// RETURN     :: void : NULL
// **/
void MacCellularSplayInsert(
    SimpleSplayTree* tree,
    SimpleSplayNode *splayNodePtr,
    BOOL* newTime);


// /**
// FUNCTION   :: MacCellularProcessTchMessages
// LAYER      :: MAC
// PURPOSE    :: Process TCH messages for this node for the current time
// PARAMETERS ::
// + node             : Node*             : Pointer to node
// + interfaceIndex   : interfaceIndex    : cellular interface number
// RETURN     :: void : NULL
// **/
void MacCellularProcessTchMessages(
    Node* node,
    int interfaceIndex);
#endif
