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

/*
*  Name: pim.h
*  Purpose: To simulate Protocol Independent Multicast Routing Protocol (PIM)
*
 */


#ifndef _PIM_H_
#define _PIM_H_

#include "network_ip.h"
#include "main.h"
#include "list.h"
#include "buffer.h"
#ifdef ADDON_BOEINGFCS
#include "multicast_ces_rpim_sm.h"
#include "multicast_ces_rpim_dm.h"
#endif

#ifdef ADDON_DB
#include "db_multimedia.h"
#endif

#include <set>

#define PIM_VERSION    2

#define ALL_PIM_ROUTER    0xE000000D

#define NEXT_HOP_ME    0

#define UPSTREAM    0xFFFFFFFF

/* Jitter transmission to avoid synchronization */
#define ROUTING_PIM_BROADCAST_JITTER    (20*  MILLI_SECOND)

/* How often HELLO message are sent */
#define ROUTING_PIM_HELLO_PERIOD    (30*SECOND)

/* Triggered Hello dealy */
#define ROUTING_PIM_TRIGGERED_HELLO_DELAY    (5*  SECOND)

/* Neighbor liveness timer */
#define ROUTING_PIM_HELLO_HOLDTIME    (3.5 * ROUTING_PIM_HELLO_PERIOD)

#define PIM_DR_PRIORITY    1

#define PIM_INITIAL_TABLE_SIZE    10


/* Hello message option type */
#define    ROUTING_PIM_HOLD_TIME              1
#define    ROUTING_PIM_LAN_PRUNE_DELAY        2
#define    ROUTING_PIM_DR_PRIORITY           19
#define    ROUTING_PIM_GEN_ID                20


#define ROUTING_PIM_HOLDTIME_LENGTH    2
#define ROUTING_PIM_GEN_ID_LENGTH       4
#define ROUTING_PIM_DR_PRIORITY_LENGTH  4


#define ROUTING_PIM_CANDIDATE_RP_PRIORITY       192
#define ROUTING_PIM_CANDIDATE_BSR_PRIORITY       64   //RFC:5059
#define ROUTING_PIM_MAX_RP_PRIORITY         255
#define ROUTING_PIM_MAX_BSR_PRIORITY         255
#define ROUTING_PIM_MASK_LENGTH              32

#define ROUTING_PIM_REGISTER_SUPP_CONSTANT     3
/* PIM Interface type */
typedef enum routing_pim_interface_type_enum
{
    ROUTING_PIM_BROADCAST_INTERFACE,
    ROUTING_PIM_POINT_TO_POINT_INTERFACE,
#ifdef ADDON_BOEINGFCS
    MULTICAST_CES_RPIM_INTERFACE
#endif
} RoutingPimInterfaceType;


typedef enum
{
    ROUTING_PIM_JOIN_TREE,
    ROUTING_PIM_PRUNE_TREE
} RoutingPimJoinPruneActionType;

typedef enum
{
    ROUTING_PIM_MODE_DENSE,
    ROUTING_PIM_MODE_SPARSE,
    ROUTING_PIM_MODE_SPARSE_DENSE
} RoutingPimModeType;


/* PIM Packet type */
typedef enum
{
    ROUTING_PIM_HELLO,
    ROUTING_PIM_REGISTER,
    ROUTING_PIM_REGISTER_STOP,
    ROUTING_PIM_JOIN_PRUNE,
    ROUTING_PIM_BOOTSTRAP,
    ROUTING_PIM_ASSERT,
    ROUTING_PIM_GRAFT,
    ROUTING_PIM_GRAFT_ACK,
    ROUTING_PIM_CANDIDATE_RP,
    ROUTING_PIM_STATE_REFRESH
} RoutingPimPacketCode;

typedef enum
{
    ROUTING_PIM_JOIN,
    ROUTING_PIM_PRUNE,
    ROUTING_PIM_END_OF_MESSAGE
} RoutingPimJoinPruneMessageType;

/**************************************************************************/
/*                    PIM-DM specific Enums and Timers                    */
/**************************************************************************/

#define ROUTING_PIM_DM_DATA_TIMEOUT    (210*  SECOND)

#define ROUTING_PIM_DM_ASSERT_TIMEOUT    (180*  SECOND)

#define ROUTING_PIM_DM_PRUNE_TIMEOUT    (210*  SECOND)

#define ROUTING_PIM_DM_GRAFT_RETRANS_PERIOD    (3*  SECOND)

#define ROUTING_PIM_DM_RANDOM_DELAY_JOIN_TIMEOUT    (2.5*  SECOND)

#define ROUTING_PIM_DM_LAN_PROPAGATION_DELAY (SECOND/2)
#define ROUTING_PIM_INFINITE_METRIC_PREFERENCE  0xFFFFFFFF
#define ROUTING_PIM_INFINITE_METRIC             0xFFFFFFFF
#define ROUTING_PIM_INFINITE_HOLDTIME  -1
#define PIM_ASSERT_PACKET_SIZE 26
#define ROUTING_PIMSM_HALF_REGISTER_SUPPRESSION_TIME(registerSupp) \
        ((registerSupp) / 2)
/* Encoded unicast address */
typedef struct routing_pim_encoded_unicast_addr_str
{
    unsigned char        addressFamily;
    unsigned char        encodingType;
    char                 unicastAddr[4];
}RoutingPimEncodedUnicastAddr;


/* Encoded multicast address */
typedef struct
{
    unsigned char       addressFamily;
    unsigned char       encodingType;
    unsigned char       reserved;
    unsigned char       maskLength;
    NodeAddress         groupAddr;
} RoutingPimEncodedGroupAddr;

/* Encoded source address */
typedef struct routing_pim_encoded_src_addr_str
{
    unsigned char       addressFamily;
    unsigned char       encodingType;
    unsigned char       rpSimEsa;//reserved:5,
                        //sparseBit:1,
                        //wildCard:1,
                        //RPT:1;
    unsigned char       maskLength;
    NodeAddress         sourceAddr;
} RoutingPimEncodedSourceAddr;

/* Forwarding cache entry state */
typedef enum
{
    ROUTING_PIM_DM_FORWARD,
    ROUTING_PIM_DM_PRUNE,
    ROUTING_PIM_DM_ACKPENDING
} RoutingPimDmFwdTableState;


typedef struct routing_pim_dm_nbr_timer_str
{
    NodeAddress   nbrAddr;
    int           interfaceIndex;
    unsigned int  seqNo;
} RoutingPimDmNbrTimerInfo;
typedef RoutingPimDmNbrTimerInfo  RoutingPimSmNbrTimerInfo;


typedef struct routing_pim_dm_cache_expiration_timer_str
{
    NodeAddress   srcAddr;
    NodeAddress   grpAddr;
} RoutingPimDmDataTimeoutInfo;

typedef RoutingPimDmDataTimeoutInfo  RoutingPimDmGraftTimerInfo;
typedef RoutingPimDmDataTimeoutInfo  RoutingPimDmAssertTimerInfo;

typedef struct routing_pim_dm_delayed_join_timer_str
{
    NodeAddress   srcAddr;
    NodeAddress   grpAddr;
    int           interfaceIndex;
    unsigned int  seqNo;
} RoutingPimDmDelayedJoinTimerInfo;


typedef struct routing_pim_dm_delayed_prune_timer_str
{
    NodeAddress   srcAddr;
    NodeAddress   grpAddr;
    NodeAddress   downstreamIntf;
} RoutingPimDmDelayedPruneTimerInfo;

typedef RoutingPimDmDelayedPruneTimerInfo  RoutingPimDmPruneTimerInfo;

/**************************************************************************/
/*                    PIM-SM specific Enums and Timers                    */
/**************************************************************************/

#define PIMSM_HASH_MASK_LENGTH    30 /* for ip v4*/
#define PIMSM_HASH_MASK                  0xFFFFFFFC /* for ip v4*/
//#define PIMSM_HASH_MASK_LENGTH 126;/* for ipv6 */

/* Triggered BOOTSTRAP dealy */
#define ROUTING_PIMSM_TRIGGERED_DELAY_DEFAULT    (30*  SECOND)

/* BOOTSTRAP_TIMEOUT */
#define ROUTING_PIMSM_BOOTSTRAP_PERIOD_DEFAULT    (60 * SECOND)
#define ROUTING_PIMSM_BOOTSTRAP_MIN_INTERVAL_DEFAULT    (10 * SECOND)
#define ROUTING_PIMSM_CANDIDATE_RP_ADV_BACKOFF    (3 * SECOND)

/* CANDIDATE_RP_TIMEOUT */
#define ROUTING_PIMSM_CANDIDATE_RP_TIMEOUT_DEFAULT    (60*  SECOND)

/* KEEP_ALIVE TIMER PERIOD */
#define ROUTING_PIMSM_KEEPALIVE_TIMEOUT_DEFAULT    (210*  SECOND)

/* JOIN_PRUNE TIMER PERIOD */
#define ROUTING_PIMSM_JOINPRUNE_HOLD_TIMEOUT_DEFAULT  (210*  SECOND)

/* UPSTREAM JOIN TIMER */
#define ROUTING_PIMSM_T_PERIODIC_INTERVAL_DEFAULT  (60* SECOND)

/* OVERRIDE TIMER INTERVAL */
#define ROUTING_PIMSM_OVERRIDE_INTERVAL  (2.5*  SECOND)

/* ASSERT OVERRIDE TIMER INTERVAL */
#define ROUTING_PIMSM_ASSERT_OVERRIDE_INTERVAL  (3*  SECOND)

/* ASSERT TIMER INTERVAL */
#define ROUTING_PIMSM_ASSERT_TIMEOUT_DEFAULT  (180*  SECOND)

/* DEFAULT PROPAGATION DELAY */
#define ROUTING_PIMSM_PROPAGATION_DELAY  (0.5*  SECOND)

/* REGISTER SUPRESSION TIME */
#define ROUTING_PIMSM_REGISTER_SUPRESSION_TIME_DEFAULT (60* SECOND)

/* REGISTER PROBE TIME */
#define ROUTING_PIMSM_REGISTER_PROBE_TIME_DEFAULT (5* SECOND)

/* SPT SWITCHOVER THRESHOLD */
#define ROUTING_PIMSM_SWITCH_SPT_THRESHOLD_INFINITE -1

typedef enum
{
    ROUTING_PIMSM_GENERAL,
    ROUTING_PIMSM_RP,
    ROUTING_PIMSM_G,
    ROUTING_PIMSM_SG,
    ROUTING_PIMSM_SGrpt
} RoutingPimSmMulticastTreeState;

/* PIMSM Assert type enum */
typedef enum
{
    ROUTING_PIMSM_NOINFO_ASSERT,
    ROUTING_PIMSM_G_ASSERT,
    ROUTING_PIMSM_SG_ASSERT,
    ROUTING_PIMSM_G_ASSERT_CANCEL,
    ROUTING_PIMSM_SG_ASSERT_CANCEL
} RoutingPimSmAssertType;

typedef enum
{
    PIMSM_ASSERT_ORDINARY,
    PIMSM_ASSERT_INFERIOR,
    PIMSM_ASSERT_PREFFERED
} RoutingPimSmAssertStrength;


//To-Do: Remove RoutingPimSmAssertState enum and use RoutingPimAssertState
typedef enum
{
    PimSm_Assert_NoInfo,
    PimSm_Assert_ILostAssert,
    PimSm_Assert_IWonAssert
} RoutingPimSmAssertState;

typedef enum
{
    Pim_Assert_NoInfo,
    Pim_Assert_ILostAssert,
    Pim_Assert_IWonAssert
} RoutingPimAssertState;

typedef enum
{
    ROUTING_PIMSM_NOINFO_JOIN_PRUNE,
    ROUTING_PIMSM_RP_JOIN_PRUNE,
    ROUTING_PIMSM_G_JOIN_PRUNE,
    ROUTING_PIMSM_SG_JOIN_PRUNE,
    ROUTING_PIMSM_SGrpt_JOIN_PRUNE
} RoutingPimSmJoinPruneType;

typedef enum
{
    PimSm_JoinPrune_NoInfo,
    PimSm_JoinPrune_Join,
    PimSm_JoinPrune_PrunePending,
    PimSm_JoinPrune_Pruned,
    PimSm_JoinPrune_NotJoin,
    PimSm_JoinPrune_NotPruned,
    PimSm_JoinPrune_Temp_Pruned,
    PimSm_JoinPrune_Temp_PrunePending,
    PimSm_SGrpt_Pruned,
    PimSm_SGrpt_NotJoined,
    PimSm_SGrpt_NotPruned
} RoutingPimSmJoinPruneState;

typedef enum
{
    PimSm_Register_NoInfo,
    PimSm_Register_Join,
    PimSm_Register_JoinPending,
    PimSm_Register_Prune
} RoutingPimSmRegisterState;

/* PIMSM Assert Timer Info */
typedef struct
{
    NodeAddress                         srcAddr;
    NodeAddress                         grpAddr;
    unsigned int                        intfIndex;
    unsigned int                        seqNo;
    RoutingPimSmMulticastTreeState      treeState;
} RoutingPimSmAssertTimerInfo;

/* PIMSM Bootstrap Timer Info */
typedef struct
{
    NodeAddress                     srcAddr;
    unsigned int                    intfIndex;
    unsigned int                    seqNo;
} RoutingPimSmBootstrapTimerInfo;


/* PIMSM Candidate-RP Timer Info */
typedef struct
{
    NodeAddress                     srcAddr;
    unsigned int                    intfIndex;
    unsigned int                    seqNo;
} RoutingPimSmCandidateRPTimerInfo;


/* PIMSM Join Prune Timer Info */
typedef struct
{
    NodeAddress                       srcAddr;
    NodeAddress                       grpAddr;
    unsigned int                      intfIndex;
    unsigned int                      seqNo;
    RoutingPimSmMulticastTreeState    treeState;
} RoutingPimSmJoinPruneTimerInfo;

/* PIMSM Register Stop Timer Info */
typedef struct
{
    NodeAddress                         srcAddr;
    NodeAddress                         grpAddr;
    unsigned int                        intfIndex;
    unsigned int                        seqNo;
    RoutingPimSmMulticastTreeState      treeState;
} RoutingPimSmRegisterStopTimerInfo;

/* PIMSM Keep Alive Timer Info */
typedef struct
{
    NodeAddress                      srcAddr;
    NodeAddress                      grpAddr;
    unsigned int                     intfIndex;
    RoutingPimSmMulticastTreeState   treeState;
} RoutingPimSmKeepAliveTimerInfo;


/**************************************************************************/
/*            PIM GENERAL CONTROL MESSAGE STRUCTURE                       */
/**************************************************************************/

/* PIM packet header */
typedef struct routing_pim_common_header_str
{
    unsigned char   rpChType;//var:4, type:4;
    unsigned char   reserved;

    //1 bit : NO_FORWARD , rest are for reserved

    short           checksum;
} RoutingPimCommonHeaderType;

void RoutingPimCommonHeaderSetVar(unsigned char *rpChType,
                                         unsigned char var);

void RoutingPimCommonHeaderSetType(unsigned char *rpChType,
                                          unsigned char type);

/*
* FUNCTION     : RoutingPimCommonHeaderGetType()
* PURPOSE      : Returns the value of version for RoutingPimCommonHeaderType
* ASSUMPTION   : None
* RETURN VALUE : unsigned char
*/
unsigned char RoutingPimCommonHeaderGetType(unsigned char rpChType);

/* PIM hello packet option */
typedef struct
{
    unsigned short  optionType;
    unsigned short  optionLength;

    /* rest of the part will allocate dynamically */
} RoutingPimHelloPacketOption;


/* PIM hello packet structure */
typedef struct routing_pim_hello_packet_str
{
    RoutingPimCommonHeaderType  commonHeader;

    /* This part wil allocate dynamically */
} RoutingPimHelloPacket;

/**************************************************************************/
/*            PIM-DM SPECIFIC CONTROL MESSAGE STRUCTURE                   */
/**************************************************************************/

/* PIM DM Join/Prune packet structure */

typedef struct routing_pim_dm_join_prune_group_str
{
    RoutingPimEncodedGroupAddr    groupAddr;
    unsigned short  numJoinedSrc;
    unsigned short  numPrunedSrc;

    /* Rest of the part will allocate dynamically */
} RoutingPimDmJoinPruneGroupInfo;

typedef struct routing_pim_dm_join_prune_packet_str
{
    RoutingPimCommonHeaderType  commonHeader;
    RoutingPimEncodedUnicastAddr  upstreamNbr;
    unsigned char               reserved;
    unsigned char               numGroups;
    unsigned short              holdTime;

    /* Rest of the part will be allocate dynamically */
} RoutingPimDmJoinPrunePacket;


/* PIM DM Assert packet structure */
typedef struct routing_pim_dm_assert_packet_str
{
    RoutingPimCommonHeaderType  commonHeader;
    RoutingPimEncodedGroupAddr    groupAddr;
    RoutingPimEncodedUnicastAddr  sourceAddr;
    UInt32               metricBitPref;// RPTBit:1,
                                //metricPreference:31;
    unsigned int                metric;
} RoutingPimDmAssertPacket;

/*
*  NOTE: Structure of Graft and GraftAck packets are same as Join/Prune
         packet except the type field in common header
*/
typedef RoutingPimDmJoinPrunePacket RoutingPimDmGraftPacket;

typedef RoutingPimDmJoinPrunePacket RoutingPimDmGraftAckPacket;

/**************************************************************************/
/*             GENERAL PIM INTERNAL DATA STRUCTURE                        */
/**************************************************************************/

typedef struct
{
    NodeAddress   groupAddr;
    NodeAddress   groupMask;
}RoutingPimSmRPAccessList;

typedef struct
{
    NodeAddress   srcAddr;
    NodeAddress   groupAddr;
}RoutingPimSourceGroupList;

typedef struct
{
    unsigned char  rpPriority;
    LinkedList*    rpAccessList;
}RoutingPimSmCandidateRP;

typedef struct
{
    unsigned char  bsrPriority;
}RoutingPimSmCandidateBSR;

typedef enum
{
    CANDIDATE_BSR,
    PENDING_BSR,
    ELECTED_BSR
}RoutingPimSmCandidateBSRState;

typedef enum
{
    NO_INFO,
    ACCEPT_ANY,
    ACCEPT_PREFERRED
}RoutingPimSmNonCandidateBSRState;

typedef enum
{
    RCVD_PREF_BSM,
    RCVD_NON_PREF_BSM,
    BOOTSTRAP_TIMER_EXP,
    RCVD_BSM
}RoutingPimSmBSREvent;

/* Neighbor list item */
typedef struct routing_pim_dm_neighbor_list_item_str
{
    NodeAddress                   ipAddr;
    unsigned int                  lastGenIdReceive;
    clocktype                     lastHelloReceive;
    unsigned short                holdTime;
    unsigned int                  lastDRPriorityReceive;
    BOOL                          isDRPrioritypresent;
    clocktype                     NLTTimer;
} RoutingPimNeighborListItem;

typedef struct
{

    clocktype                  helloInterval;
    unsigned int               helloGenerationId;
    unsigned int               DRPriority;
    RoutingPimNeighborListItem drInfo;
    int                        neighborCount;
    LinkedList*                neighborList;
}RoutingPimSmInterface;

typedef struct
{
    clocktype                  helloInterval;
    unsigned int               helloGenerationId;
    BOOL                       isLeaf;
    NodeAddress                designatedRouter;
    int                        neighborCount;
    LinkedList*                neighborList;

    //added for assert optimization
    LinkedList*                assertSourceList;
}RoutingPimDmInterface;

/* Interface specific data structure */
typedef struct routing_pim_interface_str
{
    RoutingPimInterfaceType             interfaceType;
    NodeAddress                         ipAddress;
    NodeAddress                         subnetMask;
    BOOL                                BSRFlg;
    BOOL                                RPFlag;
    RoutingPimSmCandidateRP             rpData;
    RoutingPimSmCandidateBSR            bsrData;
    RoutingPimSmCandidateBSRState       cBSRState; //Candidate BSR state
    RoutingPimSmNonCandidateBSRState    ncBSRState;//Non-Candidate BSR state
    unsigned int                        backOffCounter;
    NodeAddress                         currentBSR;
    unsigned char                       currentBSRPriority;

    unsigned int                        bootstrapTimerSeq;
    unsigned int                        candidateRPTimerSeq;
    RoutingPimSmInterface*              smInterface;
    RoutingPimDmInterface*              dmInterface;
#ifdef ADDON_BOEINGFCS
    BOOL                                useMpr;
#endif
    BOOL                                helloSuppression;
    BOOL                                broadcastMode;
    BOOL                                joinPruneSuppression;
    BOOL                                assertOptimization;
    BOOL                                switchDirectToSPT;
    LinkedList*                         srcGrpList;
} RoutingPimInterface;

/* PIM main structure */
typedef struct struct_routing_pim_str
{
    RoutingPimInterface* interface;
    BOOL                 showStat;
    BOOL                 statPrinted;
    RoutingPimModeType   modeType;
    void*                pimModePtr;
    // Really there should be several seeds, one for each purpose.
    RandomSeed           seed;
#ifdef ADDON_DB
    StatsDBMulticastNetworkSummaryContent  pimMulticastNetSummaryStats;
#endif
} PimData;



/**************************************************************************/
/*             PIM-DM INTERNAL DATA STRUCTURE                             */
/**************************************************************************/

typedef struct assert_metric_dm
{
    unsigned  int        metricPreference;
    unsigned  int        metric;
    NodeAddress          ipAddress;
} RoutingPimDmAssertMetric;

typedef std::set<NodeAddress> GroupSet;
typedef GroupSet::iterator GrpSetIterator;

struct RoutingPimDmAssertSrcListItem
{
    RoutingPimDmAssertMetric          winnerAssertMetric;
    RoutingPimAssertState             assertState;
    NodeAddress                       srcAddr;
    clocktype                         assertTime;
    NetworkRoutingAdminDistanceType   preference;
    int                               metric;
    int                               interfaceId;
    GroupSet                          *grpSet;
    BOOL                              isUpstream;
    NodeAddress                       upstreamAddr;
    BOOL                              assertTimerRunning;
};

/* Item associated with each downstream */
typedef struct routing_pim_dm_downstream_list_item_str
{
    NodeAddress                intfAddr;
    unsigned char              ttl;
    int                        interfaceIndex;

    /* Followings are used only in case of broadcast interface */
    BOOL                       isPruned;
    clocktype                  pruneTimer;
    clocktype                  lastPruneReceived;
    BOOL                       delayedPruneActive;
    clocktype                  delayedPruneTimer;

    RoutingPimAssertState      assertState;
    RoutingPimDmAssertMetric   winnerAssertMetric;
    clocktype                  assertTime;
    BOOL                       assertTimerRunning;
} RoutingPimDmDownstreamListItem;


/* Forwaridng table row */
typedef struct routing_pim_dm_forwarding_table_row_str
{
    NodeAddress                       srcAddr;
    NodeAddress                       grpAddr;
    NodeAddress                       inIntf;
    NodeAddress                       upstream;
    NetworkRoutingAdminDistanceType   preference;
    int                               metric;
    unsigned int                      delayedJoinTimerSeqNo;
    RoutingPimAssertState             assertState;
    LinkedList                        *downstream;
    RoutingPimDmFwdTableState         state;
    clocktype                         expirationTimer;
    clocktype                         assertTime;
    clocktype                         lastJoinTimerEnd;
    BOOL                              graftRxmtTimer;
    BOOL                              delayedJoinTimer;
    BOOL                              joinSeen;
    BOOL                              assertTimerRunning;
} RoutingPimDmForwardingTableRow;


/* Forwarding table */
typedef struct routing_pim_dm_forwarding_table_str
{
    unsigned int                    numEntries;
    DataBuffer                      buffer;
} RoutingPimDmForwardingTable;





/* Statistics structure of pim */
typedef struct routing_pim_dm_stat_str
{
    int             helloReceived;
    int             helloSent;
    int             triggeredHelloSent;

    int             numDataPktOriginate;
    int             numDataPktReceived;
    int             numDataPktForward;
    int             numDataPktDiscard;


    int             joinPruneSent;
    int             joinPruneReceived;

    int             joinSent;
    int             pruneSent;

    int             joinReceived;
    int             pruneReceived;

    int             graftSent;
    int             graftReceived;

    int             graftAckSent;
    int             graftAckReceived;

    int             assertSent;
    int             assertReceived;

    int             numNeighbor;

} RoutingPimDmStats;


/* PIM-DM main structure */
typedef struct struct_routing_pim_dm_str
{

#ifdef ADDON_BOEINGFCS
    RPimMc                messageCache;
    RPimMemPool           freePool;
    clocktype             msgCacheTimeout;
#endif

    RoutingPimDmStats             stats;
    RoutingPimDmForwardingTable     fwdTable;
    // Really there should be several seeds, one for each purpose.
    RandomSeed           seed;
#ifdef ADDON_DB
    StatsDBPimDmSummary  dmSummaryStats;
#endif

} PimDmData;

/**************************************************************************/
/*       PIM-SM MULTICAST TREE INFORMATION BASE STRUCTURE                 */
/**************************************************************************/

typedef struct assert_metric
{
    BOOL                 RPTbit;
    unsigned  int        metricPreference;
    unsigned  int        metric;
    NodeAddress          ipAddress;
} RoutingPimSmAssertMetric;


/* Item associated with each downstream */
typedef struct
{
    int                             interfaceId;
    NodeAddress                     intfAddr;
    unsigned char                   ttl;
    BOOL                            prunePendingTimerRunning;
    RoutingPimSmJoinPruneState      joinPruneState;
    clocktype                       lastJoinReceived;
    clocktype                       lastPruneReceived;
    unsigned int                    expiryTimerSeq;
    unsigned int                    prunePendingTimerSeq;

    RoutingPimSmAssertState         assertState;
    NodeAddress                     assertWinner;
    RoutingPimSmAssertMetric        assertWinnerMetric;
    clocktype                       ETCurrentTime;
    clocktype                       assertTime;
} RoutingPimSmDownstream;

/* Routing table row */
struct TreeInfoRowKey
{
    NodeAddress                         srcAddr;
    NodeAddress                         grpAddr;
    RoutingPimSmMulticastTreeState      treeState;

    bool operator < (const TreeInfoRowKey& key) const
    {
        return (((srcAddr < key.srcAddr) || !(key.srcAddr < srcAddr) && (grpAddr < key.grpAddr))
                ||
                !((key.srcAddr < srcAddr) || !(srcAddr < key.srcAddr) && (key.grpAddr < grpAddr))
                &&
                (treeState < key.treeState));
    }
};

typedef struct routing_pim_sm_tree_info_base_str
{
    NodeAddress                         grpAddr;
    NodeAddress                         srcAddr;
    NodeAddress                         upstream;
    int                                 upInterface;
    RoutingPimSmJoinPruneState          upstreamState;
    RoutingPimSmAssertState             upAssertState;
    NodeAddress                         upAssertWinner;
    BOOL                                joinSeenInUpIntf;
    BOOL                                pruneSeenInUpIntf;
    unsigned int                        joinTimerSeq;
    BOOL                                SPTbit;
    clocktype                           keepAliveExpiryTimer;
    unsigned int                        OTTimerSeq;
    clocktype                           lastOTTimerStart;
    clocktype                           lastOTTimerEnd;
    clocktype                           lastJoinPruneSend;
    clocktype                           lastJoinTimerEnd;
     /* kept at DR for each (S.G) */
    RoutingPimSmRegisterState           registerState;
    unsigned int                        registerStopTimerSeq;
    BOOL                                isTunnelPresent;

    NetworkRoutingAdminDistanceType     preference;
    RoutingPimSmAssertMetric            metric;
    LinkedList                         * downstream;
    RoutingPimSmMulticastTreeState      treeState;
    //NodeAddress                         currentWinner;
    NodeAddress                         RPointAddr;
    NodeAddress                         nextHopForRP;
    int                                 nextIntfForRP;
    NodeAddress                         nextHopForSrc;
    int                                 nextIntfForSrc;
    LinkedList                         * immediateOutIntfRP;
    LinkedList                         * immediateOutIntfG;
    LinkedList                         * immediateOutIntfSG;
    LinkedList                         * inheritedOutIntfSGrpt;
    LinkedList                         * inheritedOutIntfSG;

    // For Join/Prune Suppression
    BOOL                                isSuppressed;
    clocktype                           suppressionEnds;


    NodeAddress                         oldRPatSrcDR;

} RoutingPimSmTreeInfoBaseRow;

typedef std::map<TreeInfoRowKey ,RoutingPimSmTreeInfoBaseRow*> TreeInfoRowMap;
typedef TreeInfoRowMap::iterator TreeInfoRowMapIterator;
typedef pair<TreeInfoRowMapIterator, bool> TreeInfoRowInsertSucceeded;


/* Routing table */
typedef struct
{
    unsigned  int                      numEntries;

    TreeInfoRowMap                     *rowPtrMap;

} RoutingPimSmTreeInformationBase;


/**************************************************************************/
/*             PIM-SM MULTICAST FORWARDING TABLE STRUCTURE                */
/**************************************************************************/

/* Forwarding table row */
typedef struct
{
    NodeAddress                           grpAddr;
    NodeAddress                           srcAddr;
    int                                   inInterface;
    BOOL                                  pktRouted;
    BOOL                                  SPTbit;
    unsigned int                          lastPktTTL;
} RoutingPimSmForwardingTableRow;

/* Forwarding table */
typedef struct
{
    unsigned  int                        numEntries;
    unsigned  int                        numRowsAllocated;
    RoutingPimSmForwardingTableRow     * rowPtr;
} RoutingPimSmForwardingTable;

typedef struct
{
    unsigned int                        threshold;
    LinkedList*                         grpAddrList;
    LinkedList*                         numDataPacketsReceivedPerGroup;
}RoutingPimSmSptThreshold;

typedef struct
{
    NodeAddress                           grpAddr;
    BOOL                                iAmReceiverDR;
    unsigned int                        numDataPacketsReceived;
}RoutingPimSmDataPacketsPerGroup;


/**************************************************************************/
/*  PIM-SM INTERNAL LOWER-LAYER MULTICAST GROUP MEMBERSHIP DATA STRUCTURE */
/**************************************************************************/

struct RoutingPimSmGroupMemStateCounterKey
{
    NodeAddress groupAddr;
    NodeAddress upstream;
    int upInterface;

    bool operator < (const RoutingPimSmGroupMemStateCounterKey& key) const
    {
        return (groupAddr < key.groupAddr
                || (groupAddr == key.groupAddr && upstream < key.upstream)
                || (groupAddr == key.groupAddr && upstream == key.upstream && 
                    upInterface < key.upInterface));
    }
} ;

typedef std::map<RoutingPimSmGroupMemStateCounterKey, 
                                      int> RoutingPimSmGroupMemStateCounters;

/**************************************************************************/
/*             PIM-SM INTERNAL DATA STRUCTURE                             */
/**************************************************************************/

/* Statistics structure of pimsm */
typedef struct
{
    unsigned int     numNeighbor;

    unsigned int     numGroupJoin;
    unsigned int     numGroupLeave;

    unsigned int     numOfHelloPacketReceived;
    int              helloSent;
    int              triggeredHelloSent;

    int              numDataPktOriginate;
    int              numDataPktReceived;
    int              numDataPktForward;
    int              numDataPktDiscard;


    unsigned int     numOfCandidateRPPacketForwarded;
    unsigned int     numOfCandidateRPPacketReceived;

    unsigned int     numOfBootstrapPacketForwarded;
    unsigned int     numOfBootstrapPacketReceived;

    unsigned int     numOfRegisteredPacketForwarded;
    unsigned int     numOfRegisteredPacketReceived;
    unsigned int     numOfRegisterDataPacketForwarded;
    unsigned int     numOfRegisteredPacketDropped;

    unsigned int     numOfRegisterStopPacketForwarded;
    unsigned int     numOfRegisterStopPacketReceived;

    unsigned int     numOfJoinPrunePacketForwarded;
    unsigned int     numOfJoinPrunePacketReceived;

    unsigned int     numOfGAssertPacketForwarded;
    unsigned int     numOfGAssertPacketReceived;

    unsigned int     numOfSGAssertPacketForwarded;
    unsigned int     numOfSGAssertPacketReceived;

    unsigned int     numOfGAssertCancelPacketForwarded;
    unsigned int     numOfGAssertCancelPacketReceived;

    unsigned int     numOfSGAssertCancelPacketForwarded;
    unsigned int     numOfSGAssertCancelPacketReceived;

    unsigned int     numOfbootstrapPacketOriginated;

    unsigned int     numOfGJoinPacketForwarded;
    unsigned int     numOfGPrunePacketForwarded;
    unsigned int     numOfGJoinPacketReceived;
    unsigned int     numOfGPrunePacketReceived;

    unsigned int     numOfSGJoinPacketForwarded;
    unsigned int     numOfSGPrunePacketForwarded;
    unsigned int     numOfSGJoinPacketReceived;
    unsigned int     numOfSGPrunePacketReceived;

    unsigned int     numOfSGrptJoinPacketForwarded;
    unsigned int     numOfSGrptPrunePacketForwarded;
    unsigned int     numOfSGrptJoinPacketReceived;
    unsigned int     numOfSGrptPrunePacketReceived;
} RoutingPimSmStats;

typedef struct
{
    LinkedList*                         RPList;

    clocktype                           lastBootstrapSend;
    clocktype                           lastCandidateRPSend;
    RoutingPimSmTreeInformationBase     treeInfoBase;
    RoutingPimSmForwardingTable         forwardingTable;
    RoutingPimSmStats                   stats;

    clocktype                           routingPimSmTriggeredDelay;
    clocktype                           routingPimSmBootstrapTimeout;
    clocktype                           routingPimSmCandidateRpTimeout;
    clocktype                           routingPimSmKeepaliveTimeout;
    clocktype                           routingPimSmJoinPruneHoldTimeout;
    clocktype                           routingPimSmTPeriodicInterval;
    clocktype                           routingPimSmAssertTimeout;
    clocktype                           routingPimSmRegisterSupressionTime;
    clocktype                           routingPimSmRegisterProbeTime;

#ifdef ADDON_BOEINGFCS
    clocktype                           multicastCesRpimSmJoinPruneSuppressionTimeoutMin;
    clocktype                           multicastCesRpimSmJoinPruneSuppressionTimeoutMax;
    MulticastCesRpimMessageCache*       messageCache;
    clocktype                           msgCacheTimeout;
    RoutingPimSmGroupMemStateCounters*  groupMemStateCounters;
    clocktype                           miMulticastMeshTimeout;
#endif

    Message*                            lastBSMRcvdFromCurrentBSR;
    clocktype                           routingPimSmRouterGrpToRPTimeout;
    clocktype                           routingPimSmBSRGrpToRPTimeout;

    RoutingPimSmSptThreshold            sptSwitchThresholdInfo;
    // Really there should be several seeds, one for each purpose.
    RandomSeed           seed;
#ifdef ADDON_DB
    StatsDBPimSmSummary  smSummaryStats;
#endif

} PimDataSm;

typedef struct
{
    PimDataSm* pimSm;
    PimDmData* pimDm;
}PimDataSmDm;
/************************************************************************/
/* STRUCTURE USED TO HANDLE ENCAPSULATION IN PIM-SM AT DIFFERNT STAGES  */
/************************************************************************/

/* Encapsulation over IP header  */
typedef struct
{
    unsigned char rpsEncapVhl;
                 //encapsulated_ip_v:3,      /* version */
                 //encapsulated_ip_hl:5,     /* header length */
    unsigned char  encapsulated_ip_tos;    /* type of service */
    UInt16 encapsulated_ip_len; /* total length */

    UInt16 encapsulated_ip_id;
    UInt16 rpsEncapFlagOff;
                 //encapsulated_ip_flags:3,
                 //encapsulated_ip_fragment_offset:13;

    unsigned char   encapsulated_ip_ttl;   /* time to live */
    unsigned char   encapsulated_ip_p;     /* protocol */
    unsigned short  encapsulated_ip_sum;   /* checksum */
    NodeAddress     encapsulated_ip_src;   /* tunnel's entry-point */
    NodeAddress     encapsulated_ip_dst;   /* tunnel's exit-point */
} RoutingPimSmEncapsulationHeader;

void PimSmAssertPacketSetRPTBit(UInt32 *metricBitPrefSm, BOOL RPTBit);

void PimSmAssertPacketSetMetricPref(UInt32 *metricBitPrefSm,
                                           UInt32 metricPreference);
UInt32 PimSmAssertPacketGetMetricPref(UInt32 metricBitPrefSm);

void PimSmEncodedSourceAddrSetReserved(unsigned char *rpSimEsa,
                                              unsigned char reserved);

void PimSmEncodedSourceAddrSetSparseBit(unsigned char *rpSimEsa,
                                               BOOL sparseBit);


void PimSmEncodedSourceAddrSetWildCard(unsigned char *rpSimEsa,
                                              BOOL wildCard);

void PimSmEncodedSourceAddrSetRPT(unsigned char *rpSimEsa, BOOL RPT);


BOOL PimSmAssertPacketGetRPTBit(UInt32 metricBitPrefSm);

BOOL PimSmEncodedSourceAddrGetWildCard(unsigned char rpSimEsa);

BOOL PimSmEncodedSourceAddrGetRPT(unsigned char rpSimEsa);

/**************************************************************************/
/*            PIM-SM SPECIFIC CONTROL MESSAGE STRUCTURE                   */
/**************************************************************************/

/* PIMSM Assert packet structure */

typedef RoutingPimDmAssertPacket RoutingPimSmAssertPacket;

/*PIM-SM BOOTSTRAP Message*/
/* Encoded unicast addr used for BS Message */
typedef struct
{
    RoutingPimEncodedUnicastAddr   encodedUnicastRP;
    unsigned short                   RPHoldTime;
    unsigned char                    RPPriority;
    unsigned char                    reserved;

} RoutingPimSmEncodedUnicastRPInfo;

/* Encoded Group Addr used for BS Packet */
typedef struct
{
    RoutingPimEncodedGroupAddr    encodedGrpAddr;
    unsigned char                   rpCount;
    unsigned char                   fragmentRPCount;
    unsigned short                  reserved;

    /* dynamically allocated :
          RoutingPimSmEncodedUnicastBSPacket; */

} RoutingPimSmEncodedGroupBSRInfo;

/* PIMSM Bootstrap Message Packet structure */
typedef struct
{
    RoutingPimCommonHeaderType          commonHeader;
    unsigned short                      fragmentTag;
    unsigned char                       pimsmHashMaskLength;
    unsigned char                       BSRPriority;
    RoutingPimEncodedUnicastAddr        encodedUnicastBSR;

   /* dynamically allocated :
          RoutingPimSmEncodedGroupBSPacket; */
} RoutingPimSmBootstrapPacket;

/* PIM-SM Candidate RP Message Packet structure */
typedef struct
{
    RoutingPimCommonHeaderType          commonHeader;
    unsigned char                       prefixCount;
    unsigned char                       priority;
    unsigned short                      holdtime;
    RoutingPimEncodedUnicastAddr      encodedUnicastRP;

   /* dynamically allocated :
          RoutingPimSmEncodedGroupAddr; */
} RoutingPimSmCandidateRPPacket;


typedef struct
{
    unsigned char                         priority;
    unsigned short                        holdtime;
    NodeAddress                           RPAddress;
    NodeAddress                           grpPrefix;
    unsigned char                         maskLength;
    unsigned int                          routerGrpToRPTimerSeq;
    unsigned int                          bsrGrpToRPTimerSeq;
} RoutingPimSmRPList;

typedef struct
{
    RoutingPimSmEncodedGroupBSRInfo       grpInfo;
    LinkedList*                           encodedUnicastRPInfoList;
} RoutingPimSmGroupHashList;

/* PIMSM Join/Prune packet structure */

typedef struct
{
    RoutingPimEncodedGroupAddr       encodedGrpAddr;
    unsigned short                     numJoinedSrc;
    unsigned short                     numPrunedSrc;

    /* dynamically allocated join source (1-n):
            RoutingPimSmEncodedSourceAddr; */

    /* dynamically allocated prune source (1-m):
            RoutingPimSmEncodedSourceAddr; */
} RoutingPimSmJoinPruneGroupInfo;

typedef struct routing_pim_sm_joinprune_packet_str
{
    RoutingPimCommonHeaderType          commonHeader;
    RoutingPimEncodedUnicastAddr        upstreamNbr;
    unsigned char                       reserved;
    unsigned char                       numGroups;
    unsigned short                      holdTime;

    /* dynamically allocated :
            RoutingPimSmJoinPruneGroupInfo; */

} RoutingPimSmJoinPrunePacket;

/*PIM-SM REGISTER Packet          */

typedef struct
{
    RoutingPimCommonHeaderType      commonHeader;
    UInt32                          rpSmRegPkt;//border:1,
                                    //nullRegister:1,
                                    //reserved:30;

    /* dynamically allocated :
            MulticastDataPacket; */
} RoutingPimSmRegisterPacket;

/* PIMSM Group to RP Timer Info */
typedef struct
{
    NodeAddress                           RPAddress;
    NodeAddress                           grpPrefix;
    unsigned char                         maskLength;
    unsigned int                    seqNo;
} RoutingPimSmGrpToRPTimerInfo;


void PimSmRegisterPacketSetBorder(UInt32 *rpSmRegPkt, BOOL border);
void PimSmRegisterPacketSetNullReg(UInt32 *rpSmRegPkt,
                                          BOOL nullRegister);

void PimSmRegisterPacketSetReserved(UInt32 *rpSmRegPkt,
                                           UInt32 reserved);

BOOL PimSmRegisterPacketGetNullReg(UInt32 rpSmRegPkt);
UInt32 PimSmRegisterPacketGetReserved(UInt32 rpSmRegPkt);



typedef struct
{
    NodeAddress       groupAddr;
    NodeAddress       rpAddr;
} RoutingPimSmGroupSpecificRPInfo;


/*PIM-SM REGISTER STOP Packet*/

typedef struct
{
    RoutingPimCommonHeaderType          commonHeader;
    RoutingPimEncodedGroupAddr        encodedGroupAddr;
    RoutingPimEncodedUnicastAddr      encodedSrcAddr;
} RoutingPimSmRegisterStopPacket;

typedef struct
{
    NodeAddress srcAddr;
    NodeAddress upstreamNbr;
    NodeAddress grpAddr;
    int outIntfId;
    RoutingPimJoinPruneActionType actionType;
} RoutingPimSmPeriodicSGRptInfo;

/**************************************************************************/
/*                     GLOBAL FUNCTION PROTOTYPE                          */
/**************************************************************************/


BOOL
RoutingPimSmJoinDesiredG(Node* node, NodeAddress grpAddr,
        NodeAddress srcAddr, int interfaceId);

/*
*  FUNCTION     RoutingPimInit()
*  PURPOSE      Initialization function for PIM multicast protocol
*
*  Parameters:
*      node:            Node being initialized
*      nodeInput:       Reference to input file.
*      interfaceIndex:  Interface over which PIM is running
*/
void RoutingPimInit(Node* node,
                    const NodeInput* nodeInput,
                    int interfaceIndex);

/*
*  FUNCTION     :RoutingPimFinalize()
*  PURPOSE      :Finalize funtion for PIM
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimFinalize(Node* node);


/*
*  FUNCTION     :RoutingPimHandleProtocolPacket()
*  PURPOSE      :Handle incoming control packet of PIM
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimHandleProtocolPacket(Node* node, Message* msg,
                                    NodeAddress srcAddr, int interfaceId);

/*
*  FUNCTION     :RoutingPimHandleProtocolEvent()
*  PURPOSE      :Handle PIM protocol event
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimHandleProtocolEvent(Node* node, Message* msg);

/*
*  FUNCTION     :RoutingPimLocalMembersJoinOrLeave()
*  PURPOSE      :Handle local group membership events
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimLocalMembersJoinOrLeave(Node* node,
                                       NodeAddress groupAddr,
                                       int interfaceId,
                                       LocalGroupMembershipEventType event);


/*------------------------------------------------------------------------*/
/*     PROTOTYPE FOR CREATING AND SENDING DIFFERENT PACKET TO IP          */
/*------------------------------------------------------------------------*/

void RoutingPimSendHelloPacket(Node* node,
                               int interfaceIndex,
                               unsigned short holdTime = 0,
                               NodeAddress oldAddress = 0);

void RoutingPimCreateCommonHeader(RoutingPimCommonHeaderType* header,
                                         unsigned char pktCode);

BOOL RoutingPimProcessHelloPacket(RoutingPimHelloPacket* helloPkt,
        int pktSize, unsigned short* holdTime, unsigned int* genId,
        unsigned int*  DRpriority, BOOL *isDRPrioritypresent);

BOOL RoutingPimIsPimEnabledInterface(Node* node,
        int interfaceId);

void RoutingPimSearchNeighborList(LinkedList* list, NodeAddress addr,
        RoutingPimNeighborListItem* *item);

void RoutingPimSetTimer(Node* node,
                        int interfaceIndex,
                        int eventType,
                        void* data,
                        clocktype delay);



/**************************************************************************/
/*        VARIOUS PIM_SM FUNCTIONS USED FOR INITIALIZATION PROCESS        */
/**************************************************************************/

void
RoutingPimSmLocalMembersJoinOrLeave(Node* node,
                                       NodeAddress groupAddr,
                                       int interfaceId,
                                       LocalGroupMembershipEventType event);

void RoutingPimSmInitForwardingTable(Node* node);

void RoutingPimSmInitTreeInfoBase(Node* node);

#ifdef ADDON_BOEINGFCS
static void RoutingPimSmInitGroupMemStateCounters(Node* node)
{
    PimData* pim = (PimData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_PIM);
    PimDataSm* pimDataSm = NULL;
    if (pim->modeType == ROUTING_PIM_MODE_SPARSE)
    {
        pimDataSm = (PimDataSm*)pim->pimModePtr;
    }
    else if (pim->modeType == ROUTING_PIM_MODE_SPARSE_DENSE)
    {
        pimDataSm = ((PimDataSmDm*)pim->pimModePtr)->pimSm;
    }

    pimDataSm->groupMemStateCounters = new RoutingPimSmGroupMemStateCounters;
}
#endif // ADDON_BOEINGFCS


/**************************************************************************/
/*    VARIOUS FUNCTIONS USED FOR BOOTSTRAP & RP SELECTION PROCESS         */
/**************************************************************************/
NodeAddress
RoutingPimSmFindRPForThisGroup(Node* node,
                               NodeAddress  groupAddr);
void
RoutingPimSmCheckForCandidateRP(Node* node, int interfaceIndex,
                         const NodeInput* nodeInput);
BOOL
RoutingPimSmSendCandidateRPPacket(Node* node);

void
RoutingPimSmHandleCandidateRPPacket(Node* node, Message* msg,
                                    int interfaceId);

void
RoutingPimSmHandleBootstrapPacket(Node* node, Message* msg,
                                  NodeAddress srcAddr, int incomingIntf);
void
RoutingPimSmPrintRPList(Node* node);

/**************************************************************************/
/*        VARIOUS FUNCTIONS USED FOR FORWARDING PROCESS                   */
/**************************************************************************/
void
RoutingPimSmForwardingFunction(Node* node,
                              Message* msg,
                              NodeAddress grpAddr,
                              int interfaceIndex,
                              BOOL* packetWasRouted,
                              NodeAddress preHop);
void
RoutingPimSmPrintForwardingTable(Node* node);

RoutingPimSmTreeInfoBaseRow*
RoutingPimSmSetMulticastTreeInfoBase(Node* node,
     NodeAddress groupAddr, NodeAddress srcAddr,
     RoutingPimSmMulticastTreeState treeState);

void
RoutingPimSmPrintTreeInfoBase(Node* node);

/**************************************************************************/
/*             VARIOUS FUNCTIONS USED FOR HELLO PROCESS                   */
/**************************************************************************/
void
RoutingPimSmHandleHelloPacket(Node* node, NodeAddress srcAddr,
    RoutingPimHelloPacket* helloPkt, unsigned int pktSize, int interfaceId);

/**************************************************************************/
/*           VARIOUS FUNCTIONS USED FOR JOIN_PRUNE PROCESS                */
/**************************************************************************/
void
RoutingPimSmSendJoinPrunePacket(Node* node,
                                NodeAddress srcAddr,
                                NodeAddress upstreamNbr,
                                NodeAddress grpAddr,
                                int outIntfId,
                                RoutingPimSmJoinPruneType  joinPruneType,
                                RoutingPimJoinPruneActionType actionType,
                          RoutingPimSmTreeInfoBaseRow * treeInfoBaseRowPtr);


RoutingPimSmTreeInfoBaseRow*
RoutingPimSmSearchTreeInfoBaseForThisGroup(Node* node,
      NodeAddress grpAddr, NodeAddress srcAddr,
      RoutingPimSmMulticastTreeState treeState);

RoutingPimSmDownstream*
IsInterfaceInPimSmDownstreamList(Node* node,
        RoutingPimSmTreeInfoBaseRow* entry, int interfaceIndex);

void
 RoutingPimSmHandleUpstreamStateMachine(Node* node, NodeAddress srcAddr,
            int interfaceId, NodeAddress groupAddr,
            RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr,
            clocktype joinPruneHoldTime = 0xffff,
            BOOL suppressJoinPrune = FALSE);

void
 RoutingPimSmHandleDownstreamStateMachine(Node* node, NodeAddress srcAddr,
            int interfaceId, NodeAddress groupAddr,
            RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr,
            RoutingPimJoinPruneMessageType isJoinOrPrune,
            clocktype JoinPruneHoldTime = -1);

void
RoutingPimSmHandlePrunePendingTimerExpiresEvent(Node* node,
        NodeAddress srcAddr, NodeAddress groupAddr, int interfaceId,
        RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr);

void
RoutingPimSmHandleExpiryTimerExpiresEvent(Node* node,
        NodeAddress srcAddr, NodeAddress groupAddr, int interfaceId,
        RoutingPimSmTreeInfoBaseRow * treeInfoBaseRowPtr);

void
RoutingPimSmHandleJoinTimerExpiresEvent(
    Node* node,
                                    NodeAddress srcAddr,
                                    NodeAddress groupAddr,
                            RoutingPimSmTreeInfoBaseRow * treeInfoBaseRowPtr);

void
RoutingPimSmHandleJoinPrunePacket(Node *node,
                    int interfaceId,
                    RoutingPimSmJoinPruneGroupInfo* grpInfo,
                    RoutingPimEncodedSourceAddr * encodedSrcAddr,
                    RoutingPimEncodedUnicastAddr upstreamNbr,
                    RoutingPimJoinPruneMessageType isJoinOrPrune,
                    clocktype joinPruneHoldTime,
                    NodeAddress srcAddr);


/**************************************************************************/
/*           VARIOUS FUNCTIONS USED FOR REGISTER PROCESS                  */
/**************************************************************************/
void
RoutingPimSmHandleRegisteredPacket(Node* node, Message* msg,
                                    NodeAddress srcAddr, int incomingIntf);
void
RoutingPimSmHandleRegisterStopPacket(Node* node, NodeAddress srcAddr,
         RoutingPimSmRegisterStopPacket* registerStopPkt);

/**************************************************************************/
/*             VARIOUS FUNCTIONS USED FOR ASSERT PROCESS                  */
/**************************************************************************/
void
RoutingPimSmHandleGAssertStateMachine(Node* node, NodeAddress srcAddr,
      RoutingPimSmAssertPacket* assertPkt,
      RoutingPimSmMulticastTreeState treeState, unsigned int interfaceId);
void
RoutingPimSmHandleSGAssertStateMachine(Node* node, NodeAddress srcAddr,
      RoutingPimSmAssertPacket* assertPkt,
      RoutingPimSmMulticastTreeState treeState, unsigned int interfaceId);

void
RoutingPimSmHandleAssertTimeOutEvent(
    Node* node,
    NodeAddress srcAddr,
    NodeAddress groupAddr,
    int interfaceId,
    RoutingPimSmTreeInfoBaseRow* treeInfoBaseRowPtr);

/**************************************************************************/
/*      VARIOUS FUNCTIONS TO HANDLE UNICAST-ROUTE CHANGE PROCESS          */
/**************************************************************************/

void
RoutingPimSmProcessUnicastRouteChange(Node* node, NodeAddress destAddr,
        NodeAddress destAddrMask, NodeAddress nextHop, int interfaceId,
        int metric, NetworkRoutingAdminDistanceType adminDistance);

void RoutingPimGetInterfaceAndNextHopFromForwardingTable(
    Node *node,
    NodeAddress destinationAddress,
    int *interfaceIndex,
    NodeAddress *nextHopAddress);

/**************************************************************************/
/*                    VARIOUS PIM-DM FUNCTIONS                            */
/**************************************************************************/

/*
*  FUNCTION     :RoutingPimDmLocalMembersJoinOrLeave()
*  PURPOSE      :Handle local group membership events
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimDmLocalMembersJoinOrLeave(Node* node,
                                       NodeAddress groupAddr,
                                       int interfaceId,
                                       LocalGroupMembershipEventType event);

/*
*  FUNCTION     :RoutingPimDmRouterFunction()
*  PURPOSE      :Forward multicast data packet
*  ASSUMPTION   :None
*  RETURN VALUE :NULL
*/
void RoutingPimDmRouterFunction(Node* node,
                              Message* msg,
                              NodeAddress destAddr,
                              int interfaceIndex,
                              BOOL* packetWasRouted,
                              NodeAddress prevHop);


void
RoutingPimDmAdaptUnicastRouteChange(
    Node* node,
    NodeAddress destAddr,
    NodeAddress destAddrMask,
    NodeAddress nextHop,
    int interfaceId,
    int metric,
    NetworkRoutingAdminDistanceType adminDistance);

void
RoutingPimDmDeleteNeighbor(
    Node* node,
    NodeAddress nbrAddr,
        int interfaceId);
void RoutingPimSmDeleteNeighbor(Node* node, NodeAddress nbrAddr,
        int interfaceId);
void RoutingPimSmHandleNeighborTimeoutEvent(Node* node, NodeAddress nbrAddr,
        int interfaceId);
RoutingPimDmForwardingTableRow*
RoutingPimDmGetFwdTableEntryForThisPair(
    Node* node,
        NodeAddress srcAddr,
        NodeAddress grpAddr);

RoutingPimDmDownstreamListItem*
RoutingPimDmGetDownstreamInfo(
                    RoutingPimDmForwardingTableRow* rowPtr,
                    int interfaceIndex);

BOOL
RoutingPimDmHasLocalGroup(
    Node* node,
    NodeAddress grpAddr,
        int interfaceIndex);

BOOL
RoutingPimDmIsDownstreamInterface(
                    RoutingPimDmForwardingTableRow* rowPtr,
                    int interfaceIndex);

BOOL
RoutingPimDmAllDownstreamPruned(RoutingPimDmForwardingTableRow* rowPtr);

void RoutingPimDmPrintForwardingTable(Node* node);


/*------------------------------------------------------------------------*/
/*          PROTOTYPE FOR INITIALIZING DIFFERENT STRUCTURE                */
/*------------------------------------------------------------------------*/
void RoutingPimDmInitForwardingTable(Node* node);

/*------------------------------------------------------------------------*/
/*       PROTOTYPE FOR CREATING AND SENDING DIFFERENT PACKET TO IP        */
/*------------------------------------------------------------------------*/
void
RoutingPimDmSendJoinPrunePacket(
    Node* node,
    NodeAddress srcAddr,
    NodeAddress grpAddr,
    NodeAddress nonRPFNbr,
        RoutingPimJoinPruneActionType actionType,
        clocktype holdtime);

void
RoutingPimDmSendGraftPacket(
    Node* node,
    NodeAddress srcAddr,
        NodeAddress grpAddr);

void
RoutingPimDmSendAssertPacket(
    Node* node,
    NodeAddress srcAddr,
    NodeAddress grpAddr,
    int interfaceId);

/*------------------------------------------------------------------------*/
/*           PROTOTYPE FOR HANDLING DIFFERENT PROTOCOL PACKET             */
/*------------------------------------------------------------------------*/
void
RoutingPimDmHandleHelloPacket(
    Node* node,
    NodeAddress srcAddr,
    RoutingPimHelloPacket* helloPkt,
    int pktSize,
    int interfaceIndex);

void
RoutingPimDmHandleJoinPrunePacket(
    Node* node,
    NodeAddress srcAddr,
    RoutingPimDmJoinPrunePacket* joinPrunePkt,
    int interfaceId);

void
RoutingPimDmHandleCompoundJoinPrunePacket(
    Node *node,
                    int interfaceId,
                    RoutingPimDmJoinPruneGroupInfo* grpInfo,
                    RoutingPimEncodedSourceAddr* encodedSrcAddr,
                    RoutingPimEncodedUnicastAddr upstreamNbr,
                    RoutingPimJoinPruneMessageType isJoinOrPrune,
                    clocktype joinPruneHoldTime);

void
RoutingPimDmHandleGraftPacket(
    Node* node,
    NodeAddress srcAddr,
    RoutingPimDmGraftPacket* graftPkt,
    int pktSize,
    int interfaceId);

void
RoutingPimDmHandleGraftAckPacket(
    Node* node,
    NodeAddress srcAddr,
    RoutingPimDmGraftAckPacket* graftAckPkt);

void
RoutingPimDmHandleAssertPacket(
    Node* node,
    NodeAddress srcAddr,
    RoutingPimDmAssertPacket* assertPkt,
    int inIntfIndex);

void
RoutingPimSmCheckForCandidateBSR(Node* node,
                                 int interfaceIndex,
                                 const NodeInput* nodeInput);

void
RoutingPimSmCBSRStateMachine(Node* node,
                             int interfaceIndex,
                             RoutingPimSmBSREvent eventType,
                             Message* bootstrapMsg);

void
RoutingPimSmNonCBSRStateMachine(Node* node,
                                int interfaceIndex,
                                RoutingPimSmBSREvent eventType,
                                Message* bootstrapMsg);

clocktype
RoutingPimSmCalculateBSRandOverride(Node* node,
                                    int interfaceIndex);

void
RoutingPimSmRemoveGrpToRPMappingFromRPSet(Node* node,
                                          ListItem *rpListItem);

BOOL
RoutingPimSmSendCandidateRPPacket(Node* node,
                                  int interfaceIndex);

BOOL
RoutingPimSmCheckForStaticRP(Node* node,
                             const NodeInput* nodeInput);

unsigned int
RoutingPimGetMaskLengthFromSubnetMask(NodeAddress grpMask);

BOOL
RoutingPimCheckInverseSubnetMask(NodeAddress grpMask);

BOOL
RoutingPimGetNoForwardCommonHeader(RoutingPimCommonHeaderType commonHeader);

unsigned int
RoutingPimGetMaskLengthFromSubnetMask(NodeAddress grpMask);

void
RoutingPimPrintEncodedUnicastFormat(
    RoutingPimEncodedUnicastAddr unicastAddr);

void
RoutingPimPrintEncodedGroupFormat(RoutingPimEncodedGroupAddr grpAddr);

/*------------------------End of function declaration---------------------*/

NodeAddress
getNodeAddressFromCharArray(char* address);

void
setNodeAddressInCharArray(
    char* destinationAddr,
                                      NodeAddress srcAddr);

void
RoutingPimGetAssertPacketFromBuffer(
    char* packet,
    RoutingPimDmAssertPacket* assertPkt);

void
RoutingPimSetBufferFromAssertPacket(
    char* packet,
    RoutingPimDmAssertPacket* assertPkt);

void RoutingPimSmGetSptThresholdInfo(
                Node *node,
                char *sptSwitchThresholdInfoStr,
                RoutingPimSmSptThreshold *sptSwitchThresholdInfo);
void
RoutingPimSmPerformActionA5(Node* node, NodeAddress grpAddr,
                            NodeAddress srcAddr, int interfaceId,
                            RoutingPimSmMulticastTreeState treeState);


void RoutingPimDmAddDownstream(Node* node,
        RoutingPimDmForwardingTableRow* rowPtr, int interfaceId);
void
RoutingPimDmRemoveDownstream(
    Node* node,
    RoutingPimDmForwardingTableRow* rowPtr,
    int interfaceId);
#ifdef ADDON_BOEINGFCS

void RPimDeleteMsgCache(PimData* pim,
                         RPimMc*  msgCache,
                         RPimCacheDeleteEntry* msgDelEntry);


void RPimInsertMessageCache(PimData* pim,
                                  Node* node,
                                  NodeAddress srcAddr,
                                  int seqNumber,
                                  unsigned short fragOffset,
                                  RPimMc* messageCache);

#endif
BOOL
RoutingPimSmJoinDesiredSG(Node* node, NodeAddress grpAddr,
                          NodeAddress srcAddr);

void
RoutingPimDmPrintNeibhborList(Node* node);

void
RoutingPimDmPrintForwardingTable(Node* node);

#ifdef ADDON_DB
// functions added for stats db
BOOL RoutingPimDmIsMyMulticastPacket(Node *node,
                                     NodeAddress srcAddr,
                                     NodeAddress dstAddr,
                                     NodeAddress prevAddr,
                                     int incomingInterface) ;

BOOL RoutingPimSmIsMyMulticastPacket(Node *node,
                                     NodeAddress srcAddr,
                                     NodeAddress dstAddr,
                                     NodeAddress prevAddr,
                                     int incomingInterface) ;
#endif //ADDON_DB

void
RoutingPimSmGetNextHopOutInterface(Node* node,
                                   NodeAddress addr,
                                   NodeAddress* nextHop,
                                   int* outInterface);

void
RoutingPimDmAddAssertSrcPair(Node* node,
                                NodeAddress sourceAddr,
                                NodeAddress grpAddr,
                                int interfaceId,
                                BOOL isUpstream,
                                NodeAddress upstreamAddr);

RoutingPimDmAssertSrcListItem*
RoutingPimDmSearchAssertSrcPair(Node* node,
                                   NodeAddress srcAddr,
                                   int interfaceId);

void
RoutingPimGetNeighborFromIPTable(Node* node,
                                 int interfaceId);

void
RoutingPimDmAssertStateMachine(Node* node,
                               NodeAddress sourceAddr,
                               NodeAddress grpAddress,
                               int interfaceId,
                               UInt32 metricBitPref,
                               unsigned int metric,
                               NodeAddress assertPktSrcAddr);

LinkedList*
RoutingPimParseSourceGroupStr(Node* node,
                              char* srcGroupStr,
                              LinkedList* srcGroupList,
                              Int32 intfId);

//---------------------------------------------------------------------------
// FUNCTION   :: RoutingPimHandleChangeAddressEvent
// PURPOSE    :: Handles any change in the interface address for Pim-SM
// and Pim-DM
// PARAMETERS ::
// + node               : Node* : Pointer to Node structure
// + interfaceIndex     : Int32 : interface index
// + oldAddress         : Address* : current address
// + subnetMask         : NodeAddress : subnetMask
// + networkType        : NetworkType : type of network protocol
// RETURN :: void : NULL
//---------------------------------------------------------------------------
void RoutingPimHandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    Address* oldAddress,
    NodeAddress subnetMask,
    NetworkType networkType);


#endif /* _PIM_H_ */
