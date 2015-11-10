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

#ifndef MAC_DOT11S_H
#define MAC_DOT11S_H

// ------------------------------------------------------------------
/*

The IEEE 802.11s mesh networking implementation follows the Draft 0,
Revision 2 (P802.11s-D0.02.pdf) of May 2006 along with subsequent
relevant revisions upto Revision 6 (or Version 1 of December 2006).

The current implementations provides for a subset of functionalities:
- Mesh Points and Mesh Access Points.
- MPs have a single interface, with single radio.
- MPs use the default protocol: HWMP (both on-demand and proactive)
- MPs use the default metric: ATLM
- Portals provide for IP/Layer 3 internetworking.

The implementation is spread over 3 sets of files:
- mac_dot11s.h/cpp : MP and MPP behavior and control code for HWMP
- mac_dot11s-frames.h/cpp : Additional IEs and frames related to 802.11s
- mac_dot11s-hwmp.h/cpp : HWMP comprising on-demand and tree routing.

Airtime Link Metric (ATLM), the default path metric, is too small to
have an individual implementation file and is included in mac_dot11s.h/cpp

Limitations:
- No QoS Support, deterministic access, congestion control, power save,
  or channel negotiation.
*/

// ------------------------------------------------------------------

/**
DEFINE      :: DOT11s_JITTER_TIME
DESCRIPTION :: Jitter range used for various timers.
**/
#define DOT11s_JITTER_TIME                      (50 *  MILLI_SECOND)

/**
DEFINE      :: DOT11s_INIT_START_BEACON_COUNT
DESCRIPTION :: Startup beacon count. On startup, a Mesh Point waits
                a random number of beacon intervals upto this value,
                prior to transmit of its first beacon.
**/
#define DOT11s_INIT_START_BEACON_COUNT          10

/**
DEFINE      :: DOT11s_NEIGHBOR_DISCOVERY_BEACON_COUNT
DESCRIPTION :: Number of beacon interval within Neighbor Discovery period.
                In this period, MPs send beacons but do not set up links.
**/
#define DOT11s_NEIGHBOR_DISCOVERY_BEACON_COUNT  10

/**
DEFINE      :: DOT11s_LINK_SETUP_BEACON_COUNT
DESCRIPTION :: Number of beacon interval within initial Link Setup period.
                An MP would associate with neighbors during this stage.
**/
#define DOT11s_LINK_SETUP_BEACON_COUNT          10

/**
DEFINE      :: DOT11s_LINK_STATE_BEACON_COUNT
DESCRIPTION :: Number of beacon interval within initial Link State period.
**/
#define DOT11s_LINK_STATE_BEACON_COUNT          15

/**
DEFINE      :: DOT11s_PATH_SELECTION_BEACON_COUNT
DESCRIPTION :: Number of beacon interval within initial Path Selection
                period.
**/
#define DOT11s_PATH_SELECTION_BEACON_COUNT      15

/**
DEFINE      :: DOT11s_LINK_SETUP_TIMER
DESCRIPTION :: MP Link Setup maintenance period.
**/

#define DOT11s_LINK_SETUP_TIMER                 (2 * SECOND)

/**
DEFINE      :: DOT11s_LINK_STATE_TIMER
DESCRIPTION :: MP Link State maintenance period.
**/
#define DOT11s_LINK_STATE_TIMER                 (2 * SECOND)

/**
DEFINE      :: DOT11s_PATH_SELECTION_TIMER
DESCRIPTION :: MP Path Selection maintenance period.
                Not used.
**/
#define DOT11s_PATH_SELECTION_TIMER             (4 * SECOND)

/**
DEFINE      :: DOT11s_PANN_TIMER
DESCRIPTION :: MP Portal Announcement period.
**/
#define DOT11s_PANN_TIMER                       (4 * SECOND)

/**
DEFINE      :: DOT11s_PANN_PROPAGATION_DELAY
DESCRIPTION :: MP propagation delay for retransmit of PANN.
                This is a minimum delay; a random value is added.
**/
#define DOT11s_PANN_PROPAGATION_DELAY           (20 * MILLI_SECOND)

/**
DEFINE      :: DOT11s_PORTAL_TIMEOUT_DEFAULT
DESCRIPTION :: Time for which a portal is considered active since
                the last received announcement.
               Default matches HWMP route timeout.
**/
#define DOT11s_PORTAL_TIMEOUT_DEFAULT           (10 * SECOND)

/**
DEFINE      :: DOT11s_ASSOC_ACTIVE_TIMEOUT
DESCRIPTION :: Time for which a neighbor is considered active since
                the last received link state.
               Default matches HWMP active route timeout.
**/
#define DOT11s_ASSOC_ACTIVE_TIMEOUT             (5 * SECOND)

/**
DEFINE      :: DOT11s_QUEUE_AGING_TIME
DESCRIPTION :: Aging time for queued mesh frames.
                Value is set to dot11MaxTransmitMSDULifetime.
**/
#define DOT11s_QUEUE_AGING_TIME                 (512 * DOT11_TIME_UNIT)

/**
DEFINE      :: DOT11s_DATA_SEEN_AGING_TIME
DESCRIPTION :: Aging time for seen mesh frames.
                Value is set to four times net traversal time.
**/
#define DOT11s_DATA_SEEN_AGING_TIME             \
    (4 * mp->netDiameter * mp->nodeTraversalTime)

/**
DEFINE      :: DOT11s_LINK_SETUP_RATE_LIMIT_DEFAULT
DESCRIPTION :: Default maximum peer links to setup per timer
**/
#define DOT11s_LINK_SETUP_RATE_LIMIT_DEFAULT    1

/**
DEFINE      :: DOT11s_MESH_ID_DEFAULT
DESCRIPTION :: Imaginative default mesh ID
**/
#define DOT11s_MESH_ID_DEFAULT                  "Mesh1"

/**
DEFINE      :: DOT11s_MESH_ID_INVALID
DESCRIPTION :: Contrived invalid mesh ID. Used internally.
**/
#define DOT11s_MESH_ID_INVALID                  "InvalidMeshID"

/**
DEFINE      :: DOT11s_MESH_BSS_ID
DESCRIPTION :: Proposed BSS ID for mesh management frames.
**/
const char DOT11s_MESH_BSS_ID[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/**
DEFINE      :: DOT11s_PEER_CAPACITY_MAX
DESCRIPTION :: MP peer capacity. Could be as large as 13 bits.
**/
#define DOT11s_PEER_CAPACITY_MAX                255

/**
DEFINE      :: DOT11s_ASSOC_REQUESTS_MAX
DESCRIPTION :: Maximum number of association attempts with a neighbor.
                If unsuccessful, the association is put on hold for
                some time prior to subsequent attempts.
                Draft recommends a value of 11.
**/
#define DOT11s_ASSOC_REQUESTS_MAX               3

/**
DEFINE      :: DOT11s_ASSOC_RETRY_TIMEOUT
DESCRIPTION :: Time to wait for an association response. Subsequent
                attempts use an exponential backoff.
                Draft recommends a value of 32 ms for the initial value.
*/
#define DOT11s_ASSOC_RETRY_TIMEOUT              (512 * MILLI_SECOND)

/**
DEFINE      :: DOT11s_ASSOC_OPEN_TIMEOUT
DESCRIPTION :: Time to wait for an association open after receiving
                an association confirm.
                Draft recommends a value of 128 ms.
*/
#define DOT11s_ASSOC_OPEN_TIMEOUT               (512 * MILLI_SECOND)

/**
DEFINE      :: DOT11s_ASSOC_HOLD_TIMEOUT
DESCRIPTION :: Time to put a neighbor association on hold after a
                close of unsuccessful attempt.
*/
#define DOT11s_ASSOC_HOLD_TIMEOUT               (2768 * MILLI_SECOND)

/**
DEFINE      :: DOT11s_NET_DIAMETER_DEFAULT
DESCRIPTION :: Default value of net diameter.
                The draft recommends a value of 20; but a smaller
                value is used here as it also the maximum value
                of TTL used in frame transmissions and will be
                accessed by path protocols (such as HWMP).
**/
#define DOT11s_NET_DIAMETER_DEFAULT             7

/**
DEFINE      :: DOT11s_TTL_MAX
DESCRIPTION :: Value of maximum TTL.
**/
#define DOT11s_TTL_MAX                          (dot11->mp->netDiameter)

/**
DEFINE      :: DOT11s_NODE_TRAVERSAL_TIME_DEFAULT
DESCRIPTION :: Default value of estimated node traversal time.
                This is an estimate of average one-hop traversal time.
                This should be a conservative estimate and should
                include queuing, transmission, propagation and other
                delays.
                The draft recommends a value of 40MS but a larger
                value is used to accommodate non-QoS behavior and
                as this value is also accessed by path protocols
                (such as HWMP for reverse route lifetime).
**/
#define DOT11s_NODE_TRAVERSAL_TIME_DEFAULT      (100 * MILLI_SECOND)

/**
DEFINE      :: DOT11s_FRAME_RETRANSMIT_PENALTY
DESCRIPTION :: Factor added to measure for retransmits.
                The ratio of retransmits to successfully sent frames
                gives one possible measure of PER.
**/
#define DOT11s_FRAME_RETRANSMIT_PENALTY         (0.25)

/**
DEFINE      :: DOT11s_FRAME_DROP_PENALTY
DESCRIPTION :: Penaly factor used to compute PER.
                Added to count of retransmit frames if a frame is dropped
                after multiple transmit attempts. This value is currently
                set to 0, to add no further penalty than given on
                retransmit attempt(s).
**/
#define DOT11s_FRAME_DROP_PENALTY               (0.0)

/**
DEFINE      :: DOT11s_MAP_HANDOFF_BC_TO_NETWORK_LAYER
DESCRIPTION :: Configure if an Mesh Access Point should hand a copy
                of received mesh broadcasts to the upper layers.
                Does not apply to Mesh Points or Portals.
                Default is TRUE.
**/
#define DOT11s_MAP_HANDOFF_BC_TO_NETWORK_LAYER  (TRUE)

/**
DEFINE      :: DOT11s_BEACON_DURATION_80211b_APPROX
DESCRIPTION :: Estimated beacon duration at 2 Mbps
                This duration is used to avoid overlapping beacons
                between neighboring mesh points.
**/
#define DOT11s_BEACON_DURATION_80211b_APPROX    (1000 * MICRO_SECOND)

/**
DEFINE      :: DOT11s_BEACON_DURATION_80211a_APPROX
DESCRIPTION :: Estimated beacon duration at 24 Mbps.
**/
#define DOT11s_BEACON_DURATION_80211a_APPROX    (100 * MICRO_SECOND)

/**
MACRO       :: Dot11s_Malloc
DESCRIPTION :: Utility macro for memory allocation.
**/
#define Dot11s_Malloc(type, var)                 \
        var = (type*) MEM_malloc(sizeof(type))

/**
MACRO       :: Dot11s_Memset0
DESCRIPTION :: Utility macro for memset to 0.
**/
#define Dot11s_Memset0(type, var)               \
        memset((var), 0, sizeof(type))

/**
DEFINE      :: Dot11s_MallocMemset0
DESCRIPTION :: Utility macro to combine malloc and memset to 0.
                Example usage:
                DOT11s_Data* mp;
                Dot11s_MallocMemset0(DOT11s_Data, mp);
                Note that the trailing semi-colon is needed.
**/
#define Dot11s_MallocMemset0(type, var)         \
        Dot11s_Malloc(type, var);               \
        Dot11s_Memset0(type, var)

/**
DEFINE      :: DOT11s_STATS_LABEL
DESCRIPTION :: Label used for mesh statistics.
**/
#define DOT11s_STATS_LABEL                      "802.11s"

/**
MACRO       :: DOT11s_STATS_PRINT
DESCRIPTION :: Utility macro to print a statistics line.
                e.g.
                sprintf(buf, "Mesh beacons sent = %d", stats->beaconsSent);
                DOT11s_STATS_PRINT;
                Note the semi-colon after DOT11s_STATS_PRINT
**/
#define DOT11s_STATS_PRINT                      \
        IO_PrintStat(                           \
        node, "MAC", DOT11s_STATS_LABEL, ANY_DEST, interfaceIndex, buf)


/**
DEFINE      :: ATLM_CHANNEL_OVERHEAD_80211a
DESCRIPTION :: 802.11a channel overhead in micro-seconds.
**/
#define ATLM_CHANNEL_OVERHEAD_80211a            (75)

/**
DEFINE      :: ATLM_CHANNEL_OVERHEAD_80211b
DESCRIPTION :: 802.11b channel overhead in micro-seconds.
**/
#define ATLM_CHANNEL_OVERHEAD_80211b            (335)

/**
DEFINE      :: ATLM_PROTOCOL_OVERHEAD_80211a
DESCRIPTION :: 802.11a protocol overhead
**/
#define ATLM_PROTOCOL_OVERHEAD_80211a           (110)

/**
DEFINE      :: ATLM_PROTOCOL_OVERHEAD_80211b
DESCRIPTION :: 802.11b protocol overhead
**/
#define ATLM_PROTOCOL_OVERHEAD_80211b           (364)

/**
DEFINE      :: ATLM_PROTOCOL_BITS_PER_TEST_FRAME
DESCRIPTION :: Bits per test frame (1024 bytes payload)
**/
#define ATLM_PROTOCOL_BITS_PER_TEST_FRAME       (8224)

/**
DEFINE      :: ATLM_OVERHEAD_80211a
DESCRIPTION :: Sum of 802.11a overhead
**/
#define ATLM_OVERHEAD_80211a                    \
          ( ATLM_CHANNEL_OVERHEAD_80211a        \
          + ATLM_PROTOCOL_OVERHEAD_80211a)

/**
DEFINE      :: ATLM_OVERHEAD_80211b
DESCRIPTION :: Sum of 802.11b overhead
**/
#define ATLM_OVERHEAD_80211b                    \
          (  ATLM_CHANNEL_OVERHEAD_80211b       \
          + ATLM_PROTOCOL_OVERHEAD_80211b)

// ------------------------------------------------------------------

/**
ENUM        :: DOT11s_MpState
DESCRIPTION :: Mesh Point initialization states.
                - Init Start : Listen to neighbors, adjust beacon offset.
                - Neighbor Discovery : Send beacons, listen to neighbors.
                - Link Setup : Send association requests.
                - Link State : Send link state to associated neighbors.
                - Path Selection : Initialize routing, determine paths.
                - Init Complete : Start AP services, if applicable.
**/
enum DOT11s_MpState
{
    DOT11s_S_INIT_START,
    DOT11s_S_NEIGHBOR_DISCOVERY,
    DOT11s_S_LINK_SETUP,
    DOT11s_S_LINK_STATE,
    DOT11s_S_PATH_SELECTION,
    DOT11s_S_INIT_COMPLETE
};

/**
ENUM        :: DOT11s_PathProtocol
DESCRIPTION :: Path protocols mentioned in the draft.
                Current implementation is for HWMP, the default protocol.
**/
enum DOT11s_PathProtocol
{
    DOT11s_PATH_PROTOCOL_HWMP,
    DOT11s_PATH_PROTOCOL_OLSR,
    DOT11s_PATH_PROTOCOL_NULL
};

/**
ENUM        :: DOT11s_PathMetric
DESCRIPTION :: Path metrics mentioned in the draft.
                Current implementation is for Airtime, the default metric.
**/
enum DOT11s_PathMetric
{
    DOT11s_PATH_METRIC_AIRTIME,
    DOT11s_PATH_METRIC_NULL
};

/**
ENUM        :: DOT11s_NeighborState
DESCRIPTION :: Mesh neighbor states.
                - No Capability :
                    Not compatible or no peer capacity.
                - Candidate peer :
                    Potential peer, to attempt to grid with.
                - Association pending:
                    Assoc. request sent/received.
                - Subordinate Link down :
                    Assoc. complete, link state not measured.
                - Subordinate Link up :
                    Assoc. complete, link state measured.
                - Superordinate Link down :
                    Assoc. complete, link state not measured.
                - Superordinate Link up :
                    Assoc. complete, link state measured.
**/
enum DOT11s_NeighborState
{
    DOT11s_NEIGHBOR_NO_CAPABILITY,
    DOT11s_NEIGHBOR_CANDIDATE_PEER,
    DOT11s_NEIGHBOR_ASSOC_PENDING,
    DOT11s_NEIGHBOR_SUBORDINATE_LINK_DOWN,
    DOT11s_NEIGHBOR_SUBORDINATE_LINK_UP,
    DOT11s_NEIGHBOR_SUPERORDINATE_LINK_DOWN,
    DOT11s_NEIGHBOR_SUPERORDINATE_LINK_UP
};

/**
ENUM        :: DOT11s_AssocState
DESCRIPTION :: States during neighbor association.
*/
enum DOT11s_AssocState
{
    DOT11s_ASSOC_S_IDLE,
    DOT11s_ASSOC_S_LISTEN,
    DOT11s_ASSOC_S_OPEN_SENT,
    DOT11s_ASSOC_S_CONFIRM_RECEIVED,
    DOT11s_ASSOC_S_CONFIRM_SENT,
    DOT11s_ASSOC_S_ESTABLISHED,
    DOT11s_ASSOC_S_HOLDING
};

/**
ENUM        :: DOT11s_AssocStateEvent
DESCRIPTION :: Events during association.
*/
enum DOT11s_AssocStateEvent
{
    DOT11s_ASSOC_S_EVENT_ENTER_STATE,
    DOT11s_ASSOC_S_EVENT_CANCEL_LINK,
    DOT11s_ASSOC_S_EVENT_ACTIVE_OPEN,
    DOT11s_ASSOC_S_EVENT_PASSIVE_OPEN,
    DOT11s_ASSOC_S_EVENT_CLOSE_RECEIVED,
    DOT11s_ASSOC_S_EVENT_OPEN_RECEIVED,
    DOT11s_ASSOC_S_EVENT_CONFIRM_RECEIVED,
    DOT11s_ASSOC_S_EVENT_RETRY_TIMEOUT,
    DOT11s_ASSOC_S_EVENT_OPEN_TIMEOUT,
    DOT11s_ASSOC_S_EVENT_CANCEL_TIMEOUT
};


/**
ENUM        :: DOT11s_PeerLinkStatus
DESCRIPTION :: Status codes for MP Peer Response IE.
NOTES       :: The last two (Success & Reserved) are
                not defined and for internal use.
**/
enum DOT11s_PeerLinkStatus
{
    DOT11s_PEER_LINK_CLOSE_CANCELLED,
    DOT11s_PEER_LINK_CLOSE_RECEIVED,
    DOT11s_PEER_LINK_CLOSE_INVALID_PARAMETERS,
    DOT11s_PEER_LINK_CLOSE_EXCEED_MAXIMUM_RETRIES,
    DOT11s_PEER_LINK_CLOSE_TIMEOUT,

    DOT11s_PEER_LINK_SUCCESS                = 254,
    DOT11s_PEER_LINK_STATUS_RESERVED        = 255
};

/**
ENUM        :: DOT11s_StationStatus
DESCRIPTION :: Status of associated stations.
                Currently, only the active state is used.
**/
enum DOT11s_StationStatus
{
    DOT11s_STATION_ASSOCIATING,
    DOT11s_STATION_ACTIVE,
    DOT11s_STATION_DISASSOCIATING,
    DOT11s_STATION_INACTIVE
};

// ------------------------------------------------------------------

/**
STRUCT      :: DOT11s_TimerInfo
DESCRIPTION :: Info field in mesh self timer messages.
               All fields may not be used in all timers.
               Some timers use none of the fields.
**/
struct DOT11s_TimerInfo
{
    Mac802Address addr;
    Mac802Address addr2;

    DOT11s_TimerInfo(Mac802Address address = INVALID_802ADDRESS,
                     Mac802Address address2 = INVALID_802ADDRESS)
        :   addr(address), addr2(address2)
    {}
};

/**
TYPEDEF     :: DOT11s_AssocStateFn
DESCRIPTION :: Function prototype for the assocation state behavior.
**/
typedef
void (*DOT11s_AssocStateFn)(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp);

/**
STRUCT      :: DOT11s_AssocStateData
DESCRIPTION :: State data used during neighbor association.
**/
struct DOT11s_AssocStateData {
    DOT11s_NeighborItem* neighborItem;
    DOT11s_AssocStateEvent event;
    char* peerMeshId;
    DOT11s_PathProtocol peerPathProtocol;
    DOT11s_PathMetric peerPathMetric;
    int peerCapacity;
    unsigned int linkId;
    unsigned int peerLinkId;
    DOT11s_PeerLinkStatus peerStatus;
    DOT11s_FrameInfo* frameInfo;
    int interfaceIndex;
};

/**
STRUCT   :: DOT11s_PathProfile
DESCRIPTION :: Path profile consisting of protocol and metric.
                This combines with Mesh ID to give a Mesh Profile.
**/
struct DOT11s_PathProfile
{
    DOT11s_PathProtocol pathProtocol;
    DOT11s_PathMetric pathMetric;

    DOT11s_PathProfile()
        :   pathProtocol(DOT11s_PATH_PROTOCOL_HWMP),
            pathMetric(DOT11s_PATH_METRIC_AIRTIME)
    {}
};

/**
STRUCT      :: DOT11s_InitValues
DESCRIPTION :: Temporary structure to hold values of interest during
                an MP's initialization period. Deleted once the
                DOT11s_S_INIT_COMPLETE state is reached.
**/
struct DOT11s_InitValues
{
    // Beacon duration for the PHY rate
    clocktype beaconDuration;

    // Count of beacons used to transition states
    int beaconsSent;

    // Configured SSID
    char configSSID[DOT11_SSID_MAX_LENGTH];
};

/**
STRUCT      :: DOT11s_NeighborItem
DESCRIPTION :: Mesh neighbor data
**/
struct DOT11s_NeighborItem
{
    Mac802Address neighborAddr;
    Mac802Address primaryAddr;

    // Authentication, association
    BOOL isAuthenticated;
    DOT11s_NeighborState state;
    DOT11s_NeighborState prevState;

    // Association state data
    unsigned int linkId;
    unsigned int peerLinkId;
    int retryCount;
    BOOL isConfirmSent;
    BOOL isConfirmReceived;
    BOOL isCancelled;
    clocktype retryTimeout;
    Message* retryTimerMsg;
    Message* openTimerMsg;
    Message* cancelTimerMsg;
    DOT11s_PeerLinkStatus linkStatus;
    DOT11s_PeerLinkStatus peerLinkStatus;
    DOT11s_AssocStateFn assocStateFn;
    DOT11s_AssocState assocState;
    DOT11s_AssocState prevAssocState;

    // PHY characteristics
    int channelNumber;                  // not used currently
    int channelPrecedence;              // not used currently
    PhyModel phyModel;

    // Link state info
    int dataRateInMbps;
    float PER;                          // packet error rate
    int signalQuality;                  // not used currently
    int framesSent;                     // used to compute PER
    float framesResent;                 // used to compute PER
    clocktype lastLinkStateTime;        // time of last link exchange

    // Beacon info
    clocktype beaconInterval;
    clocktype firstBeaconTime;
    clocktype lastBeaconTime;
    int beaconsReceived;


    DOT11s_NeighborItem()
        :   neighborAddr(INVALID_802ADDRESS), primaryAddr(INVALID_802ADDRESS),
            isAuthenticated(TRUE),
            state(DOT11s_NEIGHBOR_NO_CAPABILITY),
            prevState(DOT11s_NEIGHBOR_NO_CAPABILITY),
            linkId(0), peerLinkId(0),
            retryCount(0),
            isConfirmSent(FALSE), isConfirmReceived(FALSE),
            isCancelled(FALSE),
            retryTimeout(DOT11s_ASSOC_RETRY_TIMEOUT),
            retryTimerMsg(NULL), openTimerMsg(NULL), cancelTimerMsg(NULL),
            linkStatus(DOT11s_PEER_LINK_STATUS_RESERVED),
            peerLinkStatus(DOT11s_PEER_LINK_STATUS_RESERVED),
            assocStateFn(NULL),
            assocState(DOT11s_ASSOC_S_IDLE),
            prevAssocState(DOT11s_ASSOC_S_IDLE),
            channelNumber(0), channelPrecedence(0),
            phyModel(PHY802_11b), dataRateInMbps(2),
            PER(0), signalQuality(0),
            framesSent(0), framesResent(0.0f),
            lastLinkStateTime(0),
            beaconInterval(0), firstBeaconTime(0),
            lastBeaconTime(0), beaconsReceived(0)
    {}
};

/**
STRUCT      :: DOT11s_PortalItem
DESCRIPTION :: Mesh portal data
**/
struct DOT11s_PortalItem
{
    Mac802Address portalAddr;
    BOOL isActive;
    clocktype lastPannTime;

    Mac802Address nextHopAddr;
    DOT11s_PannData lastPannData;
};

/**
STRUCT      :: DOT11s_ProxyItem
DESCRIPTION :: Proxy table item.
**/
struct DOT11s_ProxyItem
{
    Mac802Address staAddr;
    BOOL inMesh;
    BOOL isProxied;
    Mac802Address proxyAddr;
};


/**
ENUM      :: DOT11s_FwdItemType
DESCRIPTION :: Forwarding table item type.
                Section 11A.10.2.1
**/
enum DOT11s_FwdItemType
{
    DOT11s_FWD_UNKNOWN,
    DOT11s_FWD_IN_MESH_NEXT_HOP,
    DOT11s_FWD_OUTSIDE_MESH_MPP
};

/**
STRUCT      :: DOT11s_FwdItem
DESCRIPTION :: Forwarding table item.
**/
struct DOT11s_FwdItem
{
    Mac802Address mpAddr;
    Mac802Address nextHopAddr;
    DOT11s_FwdItemType itemType;
};


/**
STRUCT      :: DOT11s_DataSeenItem
DESCRIPTION :: Item in list of seen data items.
                Used to eliminate handling duplicate data broadcasts
                or data unicasts.
**/
struct DOT11s_DataSeenItem
{
    Mac802Address RA;                   //Receiving address
    Mac802Address TA;                   //Transmitting address
    Mac802Address DA;                   //Final destination
    Mac802Address SA;                   //Initial source
    DOT11s_FwdControl fwdControl;
    clocktype insertTime;
};

/**
STRUCT      :: DOT11s_StationItem
DESCRIPTION :: Item in list of associated stations.
                Used to hold additional information at MAPs
                besides that held in the AP structure
**/
struct DOT11s_StationItem
{
    Mac802Address staAddr;
    Mac802Address prevApAddr;
    DOT11s_StationStatus status;
};

/**
STRUCT      :: DOT11s_E2eItem
DESCRIPTION :: Item in list for transmit of end to end data.
                Used to provide e2e sequence numbers.
NOTES       :  Maintaining e2e sequence numbers is used for reordering
                at destination and elimination of duplicates.
                Reordering at destination is not currently implemented.
**/
struct DOT11s_E2eItem
{
    Mac802Address DA;
    Mac802Address SA;
    int seqNo;                          //16 bits
};

/**
TYPEDEF     :: DOT11s_RouterFunction
DESCRIPTION :: Function prototype for a path protocol routing function.
**/
typedef
void (*DOT11s_RouterFunction)(
    Node *node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameItem,
    BOOL *packetWasRouted);


/**
TYPEDEF     :: DOT11s_LinkFailureFunction
DESCRIPTION :: Function prototype for a path protocol packet link
                failure indication e.g. packet drop event.
**/
typedef
void (*DOT11s_LinkFailureFunction)(
    Node *node,
    MacDataDot11* dot11,
    Mac802Address neighborAddr,
    Mac802Address destAddr,
    int interfaceIndex);


/**
TYPEDEF     :: DOT11s_LinkUpdateFunction
DESCRIPTION :: Function prototype for a path protocol
                update to a neighbor e.g. acknowledgement event.
**/
typedef
void (*DOT11s_LinkUpdateFunction)(
    Node *node,
    MacDataDot11* dot11,
    Mac802Address neighborAddr,
    unsigned int metric,
    clocktype lifetime,
    int interfaceIndex);

/**
TYPEDEF     :: DOT11s_LinkCloseFunction
DESCRIPTION :: Function prototype for a path protocol link close
                event. This may result from sending or receiving
                a link close frame.
**/
typedef
void (*DOT11s_LinkCloseFunction)(
    Node *node,
    MacDataDot11* dot11,
    Mac802Address neighborAddr,
    int interfaceIndex);

/**
TYPEDEF     :: DOT11s_PathUpdateFunction
DESCRIPTION :: Function prototype for a path update event.
**/
typedef
void (*DOT11s_PathUpdateFunction)(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr,
    unsigned int destSeqNo,
    int hopCount,
    unsigned int metric,
    clocktype lifetime,
    Mac802Address nextHopAddr,
    BOOL isActive,
    int interfaceIndex);

/**
TYPEDEF     :: DOT11s_StationAssociateFunction
DESCRIPTION :: Function prototype for a station association event.
**/
typedef
void (*DOT11s_StationAssociateFunction)(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr,
    Mac802Address prevApAddr,
    int interfaceIndex);

/**
TYPEDEF     :: DOT11s_StationDisassociateFunction
DESCRIPTION :: Function prototype for a station disassociation event.
**/
typedef
void (*DOT11s_StationDisassociateFunction)(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr,
    int interfaceIndex);

/**
STRUCT      :: DOT11s_ActiveProtocol
DESCRIPTION :: Selected active path protocol data.
**/
struct DOT11s_ActiveProtocol
{
    DOT11s_PathProtocol pathProtocol;

    // Protocol data
    void* data;

    // Protocols have a two stage initialization
    // - first, to read user configuration
    // - second, when an MP enters Path Selection state
    // This marks the second stage.
    BOOL isInitialized;

    // Inform the protocol that data forwarding services have begun.
        // Not required as stations & network dequeue is blocked till then.

    // Receive control packet function
        // Not required as control packets are enumed in action frames.

    // Routing function
    DOT11s_RouterFunction routerFunction;

    // Receive data packet function
        // Not required as called internally by the router function.

    // Finalize function
        // Not required as all path protocols are finalized.

    // Layer function
        // Not required as timers are enumed.

    // Link Failure event e.g. packet drop
    DOT11s_LinkFailureFunction linkFailureFunction;

    // Live Update event e.g. packet ack
    DOT11s_LinkUpdateFunction linkUpdateFunction;

    // Link close event handler
    DOT11s_LinkCloseFunction linkCloseFunction;

    // Path update event handler
    DOT11s_PathUpdateFunction pathUpdateFunction;

    // Station associate and disassocate handlers
    DOT11s_StationAssociateFunction stationAssociationFunction;
    DOT11s_StationDisassociateFunction stationDisassociationFunction;

    DOT11s_ActiveProtocol()
        :   pathProtocol(DOT11s_PATH_PROTOCOL_NULL),
            data(NULL), isInitialized(FALSE),
            routerFunction(NULL),
            linkFailureFunction(NULL),
            linkUpdateFunction(NULL),
            linkCloseFunction(NULL),
            pathUpdateFunction(NULL),
            stationAssociationFunction(NULL),
            stationDisassociationFunction(NULL)
    {}
};

/**
STRUCT      :: DOT11s_Stats
DESCRIPTION :: Mesh Point related statistics
NOTES       :: UC - Unicasts, BC - Broadcasts (including multicasts).
**/
struct DOT11s_Stats
{
    int beaconsSent;
    int beaconsReceived;

    //int authRequestsSent;
    //int authResponsesReceived;

    //int authRequestsReceived;
    //int authResponsesSent;

    int assocRequestsSent;
    int assocResponsesReceived;

    int assocRequestsReceived;
    int assocResponsesSent;

    int assocCloseSent;
    int assocCloseReceived;

    int linkStateSent;
    int linkStateReceived;

    int pannInitiated;
    int pannRelayed;
    int pannReceived;
    int pannDropped;

    int mgmtBcDropped;
    int mgmtUcDropped;

    int dataBcFromNetworkLayer;
    int dataUcFromNetworkLayer;
    int dataBcToNetworkLayer;
    int dataUcToNetworkLayer;

    int dataBcSentToBss;
    int dataBcSentToMesh;
    int dataBcReceivedFromBssAsUc;
    int dataBcReceivedFromMesh;

    int dataBcDropped;

    int dataUcSentToBss;
    int dataUcSentToMesh;               // sent for routing, actually

    int dataUcReceivedFromBss;
    int dataUcReceivedFromMesh;

    int dataUcRelayedToBssFromBss;
    int dataUcRelayedToBssFromMesh;
    int dataUcRelayedToMeshFromBss;
    int dataUcRelayedToMeshFromMesh;

    int dataUcSentToRoutingFn;

    int dataUcDropped;

    int mgmtQueueBcDropped;
    int mgmtQueueUcDropped;
    int dataQueueBcDropped;
    int dataQueueUcDropped;

};


/**
STRUCT      :: DOT11s_Data
DESCRIPTION :: 802.11s data.
**/
struct DOT11s_Data
{
    DOT11s_MpState state;
    DOT11s_MpState prevState;

    // TRUE if MP is a mesh portal.
    BOOL isMpp;

    // Mesh Profiles consist of a combination of mesh ID,
    // path protocol and path metric. The active profile would
    // define the selected one.
    char meshId[DOT11s_MESH_ID_LENGTH_MAX + 1];
    DOT11s_PathProtocol pathProtocol;
    DOT11s_PathMetric pathMetric;

    DOT11s_ActiveProtocol activeProtocol;

    // Peer capacity is 13 bits.
    int peerCapacity;

    clocktype linkSetupPeriod;
    int linkSetupRateLimit;
    clocktype linkStatePeriod;

    unsigned char netDiameter;
    clocktype nodeTraversalTime;

    clocktype pannPeriod;
    int portalSeqNo;
    clocktype portalTimeout;

    // Temporary holder for values useful during initialization.
    DOT11s_InitValues* initValues;

    // List for mesh neighbors.
    LinkedList* neighborList;

    // Data for neighbor association state
    DOT11s_AssocStateData assocStateData;

    // List of mesh portals.
    LinkedList* portalList;

    // List for proxied stations.
    LinkedList* proxyList;

    // List for MPs and their next hops.
    LinkedList* fwdList;

    // List of data frames recently seen; to eliminate duplicates.
    LinkedList* dataSeenList;

    // List of associated stations
    LinkedList* stationList;

    // List for transmit of end to end data
    LinkedList* e2eList;

    // Mesh related statistics.
    DOT11s_Stats stats;
};


// ------------------------------------------------------------------
// Function declarations

/**
FUNCTION   :: Dot11sIO_ReadTime
LAYER      :: MAC
PURPOSE    :: Read user configured time value.
              Reports an error if value is negative.
              Reports an error for unacceptable zero value.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ nodeId    : NodeAddress   : pointer to node
+ address   : Address*      : Pointer to link layer address structure
+ nodeInput : NodeInput*    : pointer to user configuration structure
+ parameter : char*         : parameter to be read
+ wasFound  : BOOL*         : TRUE if parameter was found and read
+ value     : clocktype*    : value if parameter was read
+ isZeroValid : BOOL        : permit a zero value of parameter
RETURN     :: void
**/
void Dot11sIO_ReadTime(
    Node* node,
    NodeAddress nodeId,
    Address* address,
    const NodeInput* nodeInput,
    const char* parameter,
    BOOL* wasFound,
    clocktype* value,
    BOOL isZeroValid);

/**
FUNCTION   :: Dot11sIO_ReadInt
LAYER      :: MAC
PURPOSE    :: Read user configured integer value.
              Reports an error for unacceptable negative value.
              Reports an error for unacceptable zero value.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ nodeId    : NodeAddress   : pointer to node
+ address   : Address*      : Pointer to link layer address structure
+ nodeInput : NodeInput*    : pointer to user configuration structure
+ parameter : char*         : parameter to be read
+ wasFound  : BOOL*         : TRUE if parameter was found and read
+ value     : int*          : value if parameter was read
+ isZeroValid : BOOL        : permit a zero value of parameter
+ isNegativeValid : BOOL    : permit a negative value of parameter
RETURN     :: void
**/
void Dot11sIO_ReadInt(
    Node* node,
    NodeAddress nodeId,
    Address* address,
    const NodeInput* nodeInput,
    const char* parameter,
    BOOL* wasFound,
    int* value,
    BOOL isZeroValid,
    BOOL isNegativeValid);


/**
FUNCTION   :: ListPrepend
LAYER      :: MAC
PURPOSE    :: Insert data at the beginning of list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ list      : LinkedList*   : Pointer to the LinkedList structure
+ timeStamp : clocktype     : Time stamp of insertion
+ data      : void*         : Data to be iserted
RETURN     :: void
NOTES      :: This function augments the LinkedList data structure
                that provides a ListInsert that actually appends
                to the list.
              Useful for Data Seen list, etc., where the items
                near the beginning of the list are most likely to
                be seen next.
**/
void ListPrepend(
    Node *node,
    LinkedList *list,
    clocktype timeStamp,
    void *data);

/**
FUNCTION   :: ListAppend
LAYER      :: MAC
PURPOSE    :: Wrapper for the ListInsert function.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ list      : LinkedList*   : Pointer to the LinkedList structure
+ timeStamp : clocktype     : Time stamp of insertion
+ data      : void*         : Data to be iserted
RETURN     :: void
NOTES      :: Added for symmetry with ListPrepend
**/
void ListAppend(
    Node *node,
    LinkedList *list,
    clocktype timeStamp,
    void *data);

/**
FUNCTION   :: ListDelete
LAYER      :: MAC
PURPOSE    :: Wrapper for ListGet function; removes listItem from list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ list      : LinkedList*   : Pointer to the LinkedList structure
+ listItem  : ListItem*     : Item to be deleted
+ isMsg     : BOOL          : TRUE is listItem contains a Message
RETURN     :: void
**/
void ListDelete(
    Node *node,
    LinkedList *list,
    ListItem *listItem,
    BOOL isMsg);

/**
FUNCTION   :: Dot11s_IsSelfOrBssStationAddr
LAYER      :: MAC
PURPOSE    :: Determine if a given address is own address
                or of one of the associated stations.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ addr      : Mac802Address : Address being queried
RETURN     :: BOOL          : TRUE if addr is in BSS
**/
BOOL Dot11s_IsSelfOrBssStationAddr(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address addr);

/**
FUNCTION   :: Dot11s_AddrAsDotIP
LAYER      :: MAC
PURPOSE    :: Convert addr to hex dot IP format e.g. [aa.bb.cc.dd]
              With change to 6 byte MAC address, the format is
              [aa-bb-cc-dd-ee-ff]
PARAMETERS ::
+ addrStr   : char*         : Converted address string
+ addr      : Mac802Address : Address of node
RETURN     :: void
NOTES      :: This is useful in debug trace for consistent
                columnar formatting.
**/
void Dot11s_AddrAsDotIP(
    char* addrStr,
    const Mac802Address* addr);

/**
FUNCTION   :: Dot11s_TraceFrameInfo
LAYER      :: MAC
PURPOSE    :: Utility fn to trace values in a FrameInfo structure.
                Useful for queued items in data & management queues.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo* : FrameInfo structure
+ prependStr: char*         : String to prepend to line of trace
RETURN     :: void
**/
void Dot11s_TraceFrameInfo(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameItem,
    const char* prependStr);

/**
FUNCTION   :: Dot11s_MemFreeFrameInfo
LAYER      :: MAC
PURPOSE    :: Utility fn to free memory used by DOT11s_FrameInfo.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ frameInfo : DOT11s_FrameInfo** : FrameInfo structure pointer
+ deleteMsg : BOOL              : TRUE to delete message, default FALSE
RETURN     :: void
**/
void Dot11s_MemFreeFrameInfo(
    Node* node,
    DOT11s_FrameInfo** frameInfo,
    BOOL deleteMsg = FALSE);

/**
FUNCTION   :: Dot11sMgmtQueue_DequeuePacket
LAYER      :: MAC
PURPOSE    :: Dequeue a frame from the management queue,
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo** : Parameter for dequeued frame info
RETURN     :: BOOL          : TRUE if dequeue is successful
**/
BOOL Dot11sMgmtQueue_DequeuePacket(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo** frameInfo);

/**
FUNCTION   :: Dot11sMgmtQueue_EnqueuePacket
LAYER      :: MAC
PURPOSE    :: Enqueue a frame in the management queue.
                Notifies if it is the only item in the queue.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo* : Enqueued frame info
RETURN     :: BOOL          : TRUE if enqueue is successful.
**/
BOOL Dot11sMgmtQueue_EnqueuePacket(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameInfo);

/**
FUNCTION   :: Dot11sMgmtQueue_Finalize
LAYER      :: MAC
PURPOSE    :: Deletes items in management queue & queue itself.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/
void Dot11sMgmtQueue_Finalize(
    Node* node,
    MacDataDot11* dot11);

/**
FUNCTION   :: Dot11sDataQueue_DequeuePacket
LAYER      :: MAC
PURPOSE    :: Dequeue a frame from the data queue.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo** : Parameter for dequeued frame info
RETURN     :: BOOL          : TRUE if dequeue is successful
**/
BOOL Dot11sDataQueue_DequeuePacket(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo** frameInfo);

/**
FUNCTION   :: Dot11sDataQueue_EnqueuePacket
LAYER      :: MAC
PURPOSE    :: Enqueue a frame in the single data queue.
                Notifies if it is the only item in the queue.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo* : Enqueued frame info
RETURN     :: BOOL          : TRUE if enqueue is successful.
**/
BOOL Dot11sDataQueue_EnqueuePacket(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameInfo);

/**
FUNCTION   :: Dot11sDataQueue_Finalize
LAYER      :: MAC
PURPOSE    :: Free the local data queue, including items in it.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/
void Dot11sDataQueue_Finalize(
    Node* node,
    MacDataDot11* dot11);


/**
FUNCTION   :: Dot11s_DeletePacketsToNode
LAYER      :: MAC
PURPOSE    :: Delete packets from queues going to a particular next hop
                and/or destination.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ nextHopAddr : Mac802Address : next hop address, ANY_MAC802 to ignore
+ destAddr  : Mac802Address : destination address, ANY_MAC802 to ignore
RETURN     :: void
**/
void Dot11s_DeletePacketsToNode(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address nextHopAddr,
    Mac802Address destAddr);


/**
FUNCTION   :: Dot11sProxyList_Lookup
LAYER      :: MAC
PURPOSE    :: Search proxy list for a station address.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ staAddr   : Mac802Address : address of station
RETURN     :: DOT11s_ProxyItem* : proxy item or NULL
**/
DOT11s_ProxyItem* Dot11sProxyList_Lookup(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr);

/**
FUNCTION   :: Dot11sProxyList_Insert
LAYER      :: MAC
PURPOSE    :: Insert a proxy item in the proxy list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ staAddr   : Mac802Address : address of station
+ inMesh    : BOOL          : TRUE if station is within mesh
+ isProxied : BOOL          : TRUE if station is proxied
+ proxyAddr : Mac802Address : proxy address if station is proxied
RETURN     :: void
NOTES      :: If the proxy item exists in the list, the values are
                checked for any change and a trace print occurs.
                This can happen if a station changes MP association
                without the previous MP's knowledge.
**/
void Dot11sProxyList_Insert(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr,
    BOOL inMesh,
    BOOL isProxied,
    Mac802Address proxyAddr);

/**
FUNCTION   :: Dot11sProxyList_DeleteItems
LAYER      :: MAC
PURPOSE    :: Delete proxied items for an MAP.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ proxyAddr : Mac802Address : proxy address if station is proxied
RETURN     :: void
**/
void Dot11sProxyList_DeleteItems(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address proxyAddr);

/**
FUNCTION   :: Dot11sFwdList_Lookup
LAYER      :: MAC
PURPOSE    :: Search forwarding list for an address.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ addr      : Mac802Address : address to lookup
RETURN     :: DOT11s_FwdItem* : item if found or NULL
**/
DOT11s_FwdItem* Dot11sFwdList_Lookup(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address addr);

/**
FUNCTION   :: Dot11sFwdList_Insert
LAYER      :: MAC
PURPOSE    :: Add an item to the forwarding list
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mpAddr    : Mac802Address : address of MP
+ nextHopAddr : Mac802Address : address of next hop towards the MP
+ itemType  : DOT11s_FwdItemType : item type
RETURN     :: void
NOTES      :: In case the item exists in the list, the values are
                validated and a trace print occurs.
**/
void Dot11sFwdList_Insert(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address mpAddr,
    Mac802Address nextHopAddr,
    DOT11s_FwdItemType itemType);

/**
FUNCTION   :: Dot11sDataSeenList_Insert
LAYER      :: MAC
PURPOSE    :: Add message details to the data seen list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : message to be inserted
RETURN     :: void
NOTES      :: Assumes a 4 address mesh data frame.
**/
void Dot11sDataSeenList_Insert(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);

/**
FUNCTION   :: Dot11sDataSeenList_Lookup
LAYER      :: MAC
PURPOSE    :: Search for message in data seen list using the
                tuple DA/SA/E2E sequence number.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : message to be looked up
RETURN     :: DOT11s_DataSeenItem* : value or NULL
**/
DOT11s_DataSeenItem* Dot11sDataSeenList_Lookup(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);

/**
FUNCTION   :: Dot11s_GetAssociatedNeighborCount
LAYER      :: MAC
PURPOSE    :: Get count of associated neighbors.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ neighborAddr : Mac802Address : neighbor address to check for association.
RETURN     :: int           : count of associated neighbors.
NOTES      :: Neighbor is both associated as well as in UP state.
**/
int Dot11s_GetAssociatedNeighborCount(
    Node* node,
    MacDataDot11* dot11);


/**
FUNCTION   :: Dot11s_GetAssociatedStationCount
LAYER      :: MAC
PURPOSE    :: Get count of associated stations.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: int           : count of associated stations.
**/
int Dot11s_GetAssociatedStationCount(
    Node* node,
    MacDataDot11* dot11);

/**
FUNCTION   :: Dot11s_StartTimer
LAYER      :: MAC
PURPOSE    :: Start a self timer message.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ timerInfo : DOT11s_TimerInfo* : info to be added to message
+ delay     : clocktype     : timer delay
+ timerType : int           : timer type
RETURN     :: Message*      : timer message
**/
Message* Dot11s_StartTimer(
    Node* node,
    MacDataDot11* dot11,
    const DOT11s_TimerInfo* const timerInfo,
    clocktype delay,
    int timerType);

/**
FUNCTION   :: Dot11s_SetFieldsInMgmtFrameHdr
LAYER      :: MAC
PURPOSE    :: Set fields in frame header from info details
                suitable for a mesh management frame.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ fHdr      : DOT11_FrameHdr*   : pointer to frame header
+ frameInfo : DOT11s_FrameInfo* : frame related info
RETURN     :: void
NOTES      :: The "transmit function" would fill in duration and
                sequence number. The proposed mesh BSS ID
                (all 0s) is used.
**/
void Dot11s_SetFieldsInMgmtFrameHdr(
    Node* node,
    const MacDataDot11* const dot11,
    DOT11_FrameHdr* const fHdr,
    DOT11s_FrameInfo* frameInfo);

/**
FUNCTION   :: Dot11s_SetFieldsInDataFrameHdr
LAYER      :: MAC
PURPOSE    :: Based on values in frameInfo, set fields in frame
                header suitable for a mesh data frame.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ fHdr      : DOT11s_FrameHdr*   : pointer to mesh frame header
+ frameInfo : DOT11s_FrameInfo*  : frame related info
RETURN     :: void
NOTES      :: Duration and sequence number are inserted by the
                "transmit function" depending on values
                prevalent at the time of dequeue.
**/
void Dot11s_SetFieldsInDataFrameHdr(
    Node* node,
    const MacDataDot11* const dot11,
    char* const fHdr,
    DOT11s_FrameInfo* frameInfo);

static
void Dot11s_ReturnMeshHeader(DOT11s_FrameHdr* meshHdr, Message* msg)
{
    memcpy(meshHdr, MESSAGE_ReturnPacket(msg), sizeof(DOT11s_FrameHdr));
}

/**
FUNCTION   :: Dot11s_ReceiveBeaconFrame
LAYER      :: MAC
PURPOSE    :: Receive and process a beacon frame if it has mesh elements.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received beacon message
+ isProcessd : BOOL*        : will be set to TRUE is message is processed
RETURN     :: void
NOTES      :: The mesh elements are matched for an acceptable profile.
                The sender, if new, is added to the neighbor list.
                Link Setup would, subsequently, start the association process.
                The code also checks for beacon overlap during the initial
                startup state and adjusts beacon offset if required.
**/
void Dot11s_ReceiveBeaconFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    BOOL* isProcessed);

/**
FUNCTION   :: Dot11s_ReceiveMgmtBroadcast
LAYER      :: MAC
PURPOSE    :: Receive a broadcast frame of type management.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received message
RETURN     :: void
**/
BOOL Dot11s_ReceiveMgmtBroadcast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);

/**
FUNCTION   :: Dot11s_ReceiveMgmtUnicast
LAYER      :: MAC
PURPOSE    :: Process a unicast management type frame if it contains
                mesh elements i.e. is from a mesh neighbor.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received message
RETURN     :: void
**/
BOOL Dot11s_ReceiveMgmtUnicast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);


/**
FUNCTION   :: Dot11s_PacketRetransmitEvent
LAYER      :: MAC
PURPOSE    :: Handle notification of a retransmit from the "transmit fn".
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo* : info of frame retransmitted
RETURN     :: void
**/
void Dot11s_PacketRetransmitEvent(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameInfo);

/**
FUNCTION   :: Dot11s_PacketDropEvent
LAYER      :: MAC
PURPOSE    :: Handle notification of a drop event from the "transmit fn".
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo* : info of frame dropped
RETURN     :: void
NOTES      :: If the drop is to a mesh neighbor, updates values used
                to capture/measure PER.
**/
void Dot11s_PacketDropEvent(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameInfo);

/**
FUNCTION   :: Dot11s_PacketAckEvent
LAYER      :: MAC
PURPOSE    :: Handle an Ack Event from the transmit function.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo* : info of frame retransmitted
RETURN     :: void
**/
void Dot11s_PacketAckEvent(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* txQueueItem);

/**
FUNCTION   :: Dot11s_StationAssociateEvent
LAYER      :: MAC
PURPOSE    :: Handle an station association event.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ staAddr   : Mac802Address : address of station that has associated
+ previousAp : Mac802Address : address of previous AP or INVALID_802ADDRESS
RETURN     :: void
**/
void Dot11s_StationAssociateEvent(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address stationAddr,
    Mac802Address previousAp = INVALID_802ADDRESS);


/**
FUNCTION   :: Dot11s_StationDisssociateEvent
LAYER      :: MAC
PURPOSE    :: Handle an station disassociation event.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ staAddr   : Mac802Address : address of station that has associated
RETURN     :: void
**/
void Dot11s_StationDisassociateEvent(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr);


/**
FUNCTION   :: Dot11sStationList_Lookup
LAYER      :: MAC
PURPOSE    :: Search for station in associated list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ staAddr   : Mac802Address : address of station to lookup
RETURN     :: DOT11s_StationItem* : value or NULL
**/
DOT11s_StationItem* Dot11sStationList_Lookup(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr);

/**
FUNCTION   :: Dot11s_ReceiveDataBroadcast
LAYER      :: MAC
PURPOSE    :: Receive a data broadcast of type DOT11_MESH_DATA.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received message
RETURN     :: void
NOTES      :: An MP does not receive broadcasts from the BSS.
                The broadcast can be
                - sent as a 3 address frame to the BSS
                - forwarded as a 4 address frame to other MPs
                - if required, handed to the upper layer for processing.
**/
void Dot11s_ReceiveDataBroadcast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);

/**
FUNCTION   :: Dot11s_ReceiveDataUnicast
LAYER      :: MAC
PURPOSE    :: A Mesh Point receives a data unicast.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received message
RETURN     :: void
NOTES      :: Hands message to MPP if required.
                This unicast can be a Mesh Data frame or a 3 address frame
                The received unicast is one of:
                1. From an associated station to
                    a. another associated station
                    b. to self
                    c. a non-bss destination within or outside the mesh
                    d. a broadcast
                2. From a mesh neighbor to
                    a. an associated station
                    b. to self
                    c. another destination within or outside the mesh
**/
void Dot11s_ReceiveDataUnicast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);

/**
FUNCTION   :: Dot11s_ReceiveNetworkLayerPacket
LAYER      :: MAC
PURPOSE    :: Process packet from Network layer.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ interfaceIndex : int      : interface index for next hop
+ msg       : Message*      : message from network layer
+ nextHopAddr:  Mac802Address : address of next hop
+ networkType : int         : network type
+ priority  : TosType       : type of service
RETURN     :: void
ASSUMPTION :: nextHopAddr is the MAC address of final destination.
NOTES      :: The next hop address could be:
                1. broadcast
                    Transmit to BSS and to mesh
                2. unicast
                    If BSS Station, queue it up
                    If in mesh, route
                    If outside mesh, send towards portal.
              IP Internetworking at portal is assumed.
**/
void Dot11s_ReceiveNetworkLayerPacket(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    Mac802Address nextHopAddr,
    int networkType,
    TosType priority);


/**
FUNCTION   :: Dot11s_BeaconTransmitted
LAYER      :: MAC
PURPOSE    :: A mesh beacon has been transmitted.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
NOTES      :: Changes initialization states, if applicable.
                MP initialization states transitions are based
                on beacon counts, for example,
                DOT11s_INIT_START_BEACON_COUNT
**/
void Dot11s_BeaconTransmitted(
    Node* node,
    MacDataDot11* dot11);

/**
FUNCTION   :: Dot11s_Init
LAYER      :: MAC
PURPOSE    :: Mesh Point initialization.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ nodeInput : NodeInput*    : pointer to user configuration data
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
NOTES      :: Check defaults, read relevant user configuration,
                and initialize mesh variables.
**/
void Dot11s_Init(
    Node* node,
    const NodeInput* nodeInput,
    MacDataDot11* dot11,
    NetworkType networkType);


/**
FUNCTION   :: Dot11s_HandleTimeout
LAYER      :: MAC
PURPOSE    :: Process self timer events & those of active protocol.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : timer message
RETURN     :: void
**/
void Dot11s_HandleTimeout(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);

/**
FUNCTION   :: Dot11s_Finalize
LAYER      :: MAC
PURPOSE    :: End of simulation finalization.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ interfaceIndex : int      : interface index
RETURN     :: void
NOTES      :: Print stats, release memory.
                In turn, requests finalization for path protocol.
**/
void Dot11s_Finalize(
    Node* node,
    MacDataDot11* dot11,
    int interfaceIndex);

/**
FUNCTION   :: Dot11s_ComputeLinkMetric
LAYER      :: MAC
PURPOSE    :: Compute link metric for given peer address.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ addr      : Mac802Address : neighbor address
RETURN     :: unsigned int  : computed link metric
NOTES      :: Currently, applies only to Airtime Link Metric
                and returns link cost in in microseconds.
**/
unsigned int Dot11s_ComputeLinkMetric(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address addr);

// ------------------------------------------------------------------


#endif // MAC_DOT11S_H
