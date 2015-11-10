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


#ifndef MAC_SWITCH_H
#define MAC_SWITCH_H

#include "if_queue.h"
#include "if_scheduler.h"
#include "mac_stp.h"

typedef struct SwitchGvrp_ SwitchGvrp;

// -------------------------------------------------------
// VLANs

// The range of valid VLAN IDs is 1 to 4094.
// In the implementation, the max ID is 4090 to allow
// for STP and other (future) frame types.
#define SWITCH_VLAN_ID_MAX              0xffa
#define SWITCH_VLAN_ID_MIN              1
#define SWITCH_VLAN_ID_DEFAULT          1
#define SWITCH_VLAN_ID_NULL             0
#define SWITCH_VLAN_ID_RESERVED         0xfff
#define SWITCH_VLAN_ID_STP              0xffe
#define SWITCH_VLAN_ID_GVRP             0xffd
#define SWITCH_VLAN_ID_INVALID          0xffff


// The range of filter database IDs match VLAN IDs.
#define SWITCH_FID_UNKNOWN              0xffff
#define SWITCH_FID_DEFAULT              SWITCH_VLAN_ID_MIN
#define SWITCH_FID_MAX                  SWITCH_VLAN_ID_MAX

// Type of VLAN configuration at a switch
typedef enum
{
    SWITCH_VLAN_CONFIG_STATIC,
    SWITCH_VLAN_CONFIG_DYNAMIC
} SwitchVlanConfigurationType;

// Type of VLAN tagging based ingress filtering
typedef enum
{
    SWITCH_TAGGED_FRAMES_ONLY,
    SWITCH_ACCEPT_ALL_FRAMES
} SwitchAcceptFrameType;

// Type of database learning relation (database number : VLAN).
// *    Shared learning is 1 : n
// *    Independent learning is n : 1
// *    Combined is m : n qualified by FID to VLAN map with
//          VLANs within a FID having shared learning.
typedef enum
{
    SWITCH_LEARNING_SHARED,
    SWITCH_LEARNING_INDEPENDENT,
    SWITCH_LEARNING_COMBINED
} SwitchDbVlanLearningType;

// Map of FID to VLANs for combined learning
typedef struct switch_db_fid_map
{
    unsigned short fid;
    LinkedList* vlanList;
} SwitchDbFidMap;

// For each VLAN, the map to its FID and member sets
typedef struct switch_vlan_db_item
{
    VlanId vlanId;
    unsigned short currentFid;
    LinkedList* memberSet;
    LinkedList* untaggedSet;
    LinkedList* dynamicMemberSet;
} SwitchVlanDbItem;


// -------------------------------------------------------
//  Switch port scheduler/queues

#define SWITCH_FRAME_PRIORITY_MIN           0
#define SWITCH_FRAME_PRIORITY_MAX           7
#define SWITCH_FRAME_PRIORITY_DEFAULT       0 // best effort

#define SWITCH_TRANSIT_DELAY_MAX            (4 * SECOND)
#define SWITCH_TRANSIT_DELAY_DEFAULT        (1 * SECOND)

#define SWITCH_BPDU_TRANSMIT_DELAY_MAX      (4 * SECOND)
#define SWITCH_BPDU_TRANSMIT_DELAY_DEFAULT  (1 * SECOND)

#define SWITCH_QUEUE_NUM_PRIORITIES_MIN     1
#define SWITCH_QUEUE_NUM_PRIORITIES_MAX     8
#define SWITCH_QUEUE_NUM_PRIORITIES_DEFAULT 3

#define SWITCH_OUTPUT_QUEUE_SIZE_DEFAULT    150000
#define SWITCH_INPUT_QUEUE_SIZE_DEFAULT     150000
#define SWITCH_CPU_QUEUE_SIZE_DEFAULT       640000


// Backplane related
#define SWITCH_UNLIMITED_BACKPLANE_THROUGHPUT (0)

typedef enum
{
     SWITCH_BACKPLANE_INVALID,
     SWITCH_BACKPLANE_IDLE,
     SWITCH_BACKPLANE_BUSY
} SwitchBackplaneStatusType;

typedef struct
{
    int incomingInterfaceIndex;
} SwitchBackplaneInfo;

// Info carried by frames in queues and across ports
typedef struct
{

    Mac802Address sourceAddr;
    Mac802Address destAddr;
    TosType userPriority;
    int outgoingInterfaceIndex;
    VlanId vlanId;
} SwitchInQueueInfo;

typedef struct
{

    Mac802Address sourceAddr;
    Mac802Address destAddr;
    TosType userPriority;
    VlanId vlanId;
} SwitchOutQueueInfo;

// -------------------------------------------------------
// Port stat variables

typedef struct
{
    Int64        numUnicastFlooded;
    Int64        numUnicastDeliveredDirectly;
    Int64        numUnicastDropped;
    Int64        numUnicastDeliveredToUpperLayer;
    Int64        numBroadcastFlooded;
    Int64        numBroadcastDropped;
    Int64        numDroppedInDiscardState;
    Int64        numDroppedInLearningState;
    Int64        numDroppedByIngressFiltering;
    Int64        numDroppedDueToUnknownVlan;
    Int64        numTotalFrameReceived;

    unsigned int configBpdusSent;
    unsigned int configBpdusReceived;
    unsigned int rstpBpdusSent;
    unsigned int rstpBpdusReceived;
    unsigned int tcnsSent;
    unsigned int tcnsReceived;

} SwitchPortStat;

// List item for port VLAN stats
typedef struct
{
    VlanId vlanId;
    SwitchPortStat stats;
} SwitchPerVlanPortStat;

// -------------------------------------------------------
// Switch port variables

struct SwitchPort_
{

    Mac802Address portAddr;
    unsigned short portNumber;
    //char name[32];

    MacData* macData;

    SwitchPort* nextPort;

    Scheduler* outPortScheduler;
    Scheduler* inPortScheduler;

    QueuePriorityType outPortPriorityToQueueMap[8];
    QueuePriorityType inPortPriorityToQueueMap[8];

    SwitchBackplaneStatusType backplaneStatus;

    unsigned char priority;
    unsigned int pathCost;

    BOOL macEnabled;
    BOOL macOperational;

    SwitchPortAdminP2PType adminPointToPointMac;
    BOOL adminEdgePort;

    // State machines
    SwitchStpSm* smPortTimers;
    SwitchStpSm* smPortInfo;
    SwitchStpSm* smPortRoleTransition;
    SwitchStpSm* smPortStateTransition;
    SwitchStpSm* smTopologyChange;
    SwitchStpSm* smPortProtocolMigration;
    SwitchStpSm* smPortTransmit;
    SwitchStpSm* smAgeingTimer;
    SwitchStpSm* smOperEdgeChangeDetection;
    SwitchStpSm* smBridgeDetection;

    // State machine timers
    clocktype fdWhile;
    clocktype helloWhen;
    clocktype mdelayWhile;
    clocktype rbWhile;
    clocktype rcvdInfoWhile;
    clocktype rrWhile;
    clocktype tcWhile;
    clocktype ageingTimer;

    // STP variables
    BOOL agreed;
    StpPriority designatedPriority;
    StpTimes designatedTimes;
    BOOL forward;
    BOOL forwarding;
    StpInfoIs infoIs;
    BOOL initPm;
    BOOL learn;
    BOOL learning;
    BOOL mcheck;
    StpPriority msgPriority;
    StpTimes msgTimes;
    BOOL newInfo;
    BOOL operEdge;
    BOOL portEnabled;
    StpPortId portId;
    StpPriority portPriority;
    StpTimes portTimes;
    BOOL proposed;
    BOOL proposing;
    BOOL rcvdBpdu;
    StpReceivedMsgType rcvdMsg;
    BOOL rcvdRstp;
    BOOL rcvdStp;
    BOOL rcvdTc;
    BOOL rcvdTcAck;
    BOOL rcvdTcn;
    BOOL reRoot;
    BOOL reselect;
    StpPortRole role;
    BOOL selected;
    StpPortRole selectedRole;
    BOOL sendRstp;
    BOOL sync;
    BOOL synced;
    BOOL tc;
    BOOL tcAck;
    BOOL tcProp;
    BOOL tick;
    unsigned int txCount;
    BOOL updtInfo;
    BOOL bpduReceived;

    BOOL operPointToPointMac;

    unsigned char msgProtocolVersion;
    StpBpduType msgType;
    unsigned char msgFlags;

    // stats
    SwitchPortStat stats;
    BOOL isPortStatEnabled;

    // List item is SwitchPerVlanPortStat
    LinkedList* portVlanStatList;
    BOOL vlanStatEnabled;

    VlanId pVid;
    SwitchAcceptFrameType acceptFrameType;
    BOOL enableIngressFiltering;
    BOOL enableEgressTagging;
};


// -------------------------------------------------------
// Filter database

#define SWITCH_DATABASE_ENTRIES_MIN         1
#define SWITCH_DATABASE_ENTRIES_MAX         2147483647
#define SWITCH_DATABASE_ENTRIES_DEFAULT     500


typedef enum
{
    SWITCH_DB_DEFAULT,
    SWITCH_DB_STATIC,
    SWITCH_DB_CONFIGURED,
    SWITCH_DB_DYNAMIC
} SwitchDbEntryType;

typedef struct switchdb_entry
{
    unsigned short fid;
    VlanId vlanId;

    Mac802Address srcAddr;
    unsigned short port;
    SwitchDbEntryType type;
    clocktype lastAccessTime;
    struct switchdb_entry* nextEntry;
} SwitchDbEntry;

typedef struct switchdb_stats
{
    unsigned int numEntryInserted;
    unsigned int numEntryDeleted;
    unsigned int numLRUEntryDeleted;
    unsigned int numEntryEdgedOut;
    unsigned int numEntryFound;
    unsigned int numTimesSearched;
} SwitchDbStats;

typedef struct switch_db
{
    SwitchDbEntry* flDbEntry;
    unsigned int numEntries;
    unsigned int maxEntries;
    clocktype lastAccessTime;
    SwitchDbStats stats;
    BOOL statsEnabled;
} SwitchDb;


// -------------------------------------------------------
// Switch


#define SWITCH_STP_ADDRESS_MASK             0xffff0000
#define SWITCH_NUMBER_MASK                  0xffffff 

// Based on available range in switch group address
#define SWITCH_ID_MIN                       1
#define SWITCH_ID_MAX                       65535

// Change structure StpPortId if it is required
// to change SWITCH_NUMBER_OF_PORTS_MAX to 4095
#define SWITCH_NUMBER_OF_PORTS_MIN          2
#define SWITCH_NUMBER_OF_PORTS_MAX          255

#define SWITCH_TRACE_DEFAULT                FALSE

#define SWITCH_DELIVER_PKTS_TO_UPPER_LAYERS FALSE

struct MacSwitch
{
    unsigned short swNumber;
    Mac802Address swAddr;
    SwitchPort* firstPort;

    Scheduler* cpuScheduler;
    QueuePriorityType cpuSchedulerPriorityToQueueMap[8];
    SwitchBackplaneStatusType backplaneStatus;
    unsigned int packetDroppedAtBackplane;
    clocktype swBackplaneThroughputCapacity;
    BOOL backplaneStatsEnabled;

    SwitchDb* db;

    // STP variables

    BOOL runStp;

    unsigned short priority;

    SwitchStpSm* smPortRoleSelection;

    unsigned int forceVersion;

    StpSwitchId swId;
    StpPriority swPriority;
    StpTimes swTimes;
    StpPortId rootPortId;
    StpPriority rootPriority;
    StpTimes rootTimes;

    unsigned int txHoldCount;
    clocktype migrateTime;
    clocktype ageingTime;

    BOOL stpTrace;

    // VLAN variables

    BOOL isVlanAware;
    SwitchDbVlanLearningType learningType;

    // List item is SwitchDbFidMap
    LinkedList* fidMapList;

    // List item is SwitchVlanDbItem
    LinkedList* vlanDbList;

    // Is database forwarding conditioned by member set
    BOOL isMemberSetAware;


    // GVRP variables.

    BOOL runGvrp;
    SwitchGvrp* gvrp;

    clocktype joinTime;
    clocktype leaveTime;
    clocktype leaveallTime;

    BOOL gvrpTrace;

    Mac802Address Switch_stp_address;
};


// -------------------------------------------------------
// Interface functions

// Gives switchData for a node.
MacSwitch* Switch_GetSwitchData(
    Node* node);

// Gives port pointer from port number.
SwitchPort* Switch_GetPortDataFromPortNumber(
    MacSwitch* sw,
    unsigned short portNumber);

// Gives port pointer for an interface
SwitchPort* Switch_GetPortDataFromInterface(
    MacSwitch* sw,
    int interfaceIndex);

// Flush all database entries for a particular port and type.
void SwitchDb_DeleteEntriesByPortAndType(
    SwitchDb* flDb,
    unsigned short port,
    SwitchDbEntryType type,
    clocktype currentTime);

// Counts number of entries for a particular port and type.
unsigned int SwitchDb_GetCountForPort(
    SwitchDb* flDb,
    unsigned short port,
    SwitchDbEntryType type);

// Check and delete which have aged out at a port
void SwitchDb_EntryAgeOutByPort(
    SwitchDb* flDb,
    unsigned short portNumber,
    SwitchDbEntryType type,
    clocktype currentTime,
    clocktype ageTime);

// Check if a node is configured as a switch.
// If it is, initialize it.
void Switch_CheckAndInit(
    Node* firstNode,
    const NodeInput* nodeInput);

// Check if queue is empty for given scheduler.
BOOL Switch_QueueIsEmpty(
    Scheduler* scheduler);

// Check if queue is empty for given interface.
BOOL Switch_QueueIsEmptyForInterface(
    Node* node,
    int interfaceIndex);

// Drop/remove a packet from an interface queue.
void Switch_QueueDropPacketForInterface(
    Node* node,
    int interfaceIndex,
    Message** msg);

// Clear all queued packets.
void Switch_ClearMessagesFromQueue(
    Node* node,
    Scheduler* scheduler);

// Send frame to MAC protocol via the output queue
void Switch_SendFrameToMac(
    Node* node,
    Message* msg,
    int incomingInterfaceIndex,
    int outgoingInterfaceIndex,
    Mac802Address destAddr,
    Mac802Address sourceAddr,
    VlanId vlanId,
    TosType priority,
    BOOL* isEnqueued);

// Handle events and timers
void Switch_Layer(
    Node* node,
    int interfaceIndex,
    Message* msg);

// Print stats and release switch data.
void Switch_Finalize(
    Node* node);

// Enable an interface.
void Switch_EnableInterface(
    Node* node,
    int interfaceIndex);

// Disable an interface.
void Switch_DisableInterface(
    Node* node,
    int interfaceIndex);

// Port enabled. Start normal actions.
void SwitchPort_EnableForwarding(
    Node* node,
    SwitchPort* thePort);

// Port disabled. Stop normal actions.
void SwitchPort_DisableForwarding(
    Node* node,
    SwitchPort* thePort);

// Get statically configured VLANs from database.
VlanId SwitchVlan_GetStaticMembership(
    MacSwitch* sw,
    unsigned short thePortNumber,
    BOOL isFirst,
    VlanId prevVlanId);

// Finds filter database id for given vlan
unsigned short SwitchDb_GetFidForGivenVlan(
    MacSwitch* sw, VlanId vlanId);

// Check if a port number is in the member list
BOOL SwitchVlan_IsPortInMemberList(
    LinkedList* list,
    unsigned short thePortNumber);

// Check if port present in member set of given VLAN.
BOOL SwitchVlan_IsPortInMemberSet(
    MacSwitch* sw,
    unsigned short portNumber,
    VlanId vlanId);

// Get database item of given vlan
SwitchVlanDbItem* SwitchVlan_GetDbItem(
    MacSwitch* sw,
    VlanId vlanId);

// Check if outgoing frame will be VLAN tagged.
BOOL Switch_IsOutgoingFrameTagged(
    Node* node,
    MacSwitch* sw,
    VlanId vlanClassification,
    unsigned short portNumber);

// Initialize VLAN related variables for a switch.
void Switch_VlanInit(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput);

// Initialize VLAN related variables for a switch port.
void SwitchPort_VlanInit(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    const NodeInput* nodeInput);

// Print stats and finalize for a switch.
void Switch_VlanFinalize(
    Node* node,
    MacSwitch* sw);

// Print stats and finalize for a switch port.
void SwitchPort_VlanFinalize(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port);

// Checks whether an address is a Switch STP group address.
BOOL Switch_IsStpGroupAddr(
    Mac802Address destAddr,Mac802Address stpAddr);

BOOL Switch_IsStpGroupAddr(
    NodeAddress destAddr);

// Initialize the switch data of a node.
void Switch_Init(
    Node* node,
    const NodeInput* nodeInput);

// Checks if switch is Garp aware.
BOOL Switch_IsGarpAware(MacSwitch* sw);

// Checks if an address is a Garp group address.
BOOL Switch_IsGarpGroupAddr(Mac802Address destAddr,
                            MacSwitch* sw);
// -------------------------------------------------------

#endif /* MAC_SWITCH_H */
