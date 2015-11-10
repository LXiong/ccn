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
// Name: odmrp.h
// Purpose: To simulate On-Demand Multicast Routing Protocol (ODMRP)
//
// /**
// PROTOCOL :: ODMRP
// SUMMARY ::  On-Demand Multicast Routing Protocol (ODMRP) is a multicast
//             routing protocol designed for ad hoc networks with mobile
//             hosts. ODMRP is a mesh-based, rather than a conventional
//             tree-based. The protocol uses a forwarding group concept
//             in which only a subset of nodes forwards the multicast
//             packets via scoped flooding.It applies on-demand procedures
//             to dynamically build routes and maintain multicast group
//             membership. ODMRP is well suited for ad hoc wireless
//             networks with mobile hosts where bandwidth is limited,
//             topology changes frequently and rapidly, and power is
//             constrained.
// LAYER ::    Network Layer
// STATISTICS ::
// + numQueryTxed        : Number of "Join Query" packets transmitted.
// + numQueryOriginated  : Number of "Join Query" packets originated.
// + numReplySent        : Number of "Join Reply" packets sent.
// + numReplyTransmitted : Number of "Join Reply" packets retransmitted.
// + numReplyForwarded   : Number of "Join Reply" packets forwarded.
// + numAckSent          : Number of Acknowledgement packets sent.
// + numDataSent         : Number of Data Packets sent.
// + numDataReceived     : Number of Data packets received by the
//                         Destination.
// +numDataTxed          : Number of Data packets transmitted by each node.
// APP_PARAM :: +++Tapo: can not understand
//              (Parameter Name) : (Type) : (Description) (list)  {optional}
// CONFIG_PARAM ::
//+ ODMRP-JR-REFRESH : CLOCKTYPE : Periodic Interval with which
//                                 "Join Query" message will be sent.
//+ ODMRP_FG_TIMEOUT : CLOCKTYPE : Forwarding Group Timeout Interval
//+ ODMRP-DEFAULT-TTL: INTEGER   : Time To Live value attached with
//                                 each Data Packet
//+ ODMRP-CLUSTER-TIME_OUT       : Cluster TimeOut Value.
//                                 Every list is maintained by Cluster
//                                 Timeout value. An entry is not updated
//                                 by Cluster Timeout value must be
//                                 removed from the list.
// VALIDATION ::
//   (Functionality) : (Number of Tests) : (Description (per test)) (list)
// IMPLEMENTED_FEATURES :: Present implementation conforms to
//                         draft-ietf-manet-odmrp-04.txt
// OMITTED_FEATURES ::
// 1.The mobility prediction using GPS is not implemented.
// 2.Join Reply Packet Retransmission is not done in the unicast address.
// 3.If there is a route failure, alternate path finding is postponed
//   until last refresh time (Propagation of next Join Query message).
// 4.Each Node has only one interface.
// 5.Due to the implemetation of Passive Clustering the present
//   implementation suffers from "Critical Path Loss".
//   The present implementation does provide any of the three
//   solutins as described on the page 28 in
//   IETF Draft, draft-yi-manet-pc-00.txt.
// 6. Odmrp Specific information present in "Join Query" message
//    is not printed in the trace file.
// ASSUMPTIONS ::
// 1.The implementation follows the latest specification
//   in the Internet Draft (draft-ietf-manet-odmrp-04.txt).
// 2.The present implementation does not say anything about
//   "Acknowledgement" packet. "Acknowledgement" Packet structure
//   is assumed.
// 3. Extra 4 bytes (unsigned int protocol) is sent  with
//    "Join Query" message.
// 4. Extra two fields like (1. int seqNumber
//                           2.unsigned char protocol) are sent
//    with Data Packet attached with IP Options field.
//
// STANDARD :: Coding standard follows the following Coding Guidelines
//             1. SNT C Coding Guidelines for QualNet 3.x-4.x
// RELATED ::  Passive Clustering wriiten by
//             Y. Yi, T.-J. Kwon, M. Gerla,
//             Passive Clustering (PC) in Ad Hoc Networks.
//             IETF Draft, draft-yi-manet-pc-00.txt
// **/


#ifndef _ODMRP_H_
#define _ODMRP_H_

#include "buffer.h"

// /**
//  CONSTANT    :  ODMRP_MAX_HASH_KEY_VALUE  : (97)
//  DESCRIPTION :: The maximum value of the array index of
//                 of the hash table. Message Cache is being
//                 in a hash table like structure.
// **/

#define ODMRP_MAX_HASH_KEY_VALUE          97

// /**
//  CONSTANT    :  ODMRP_IP_HASH_KEY  : (5)
//  DESCRIPTION :: This constant value is being used in order
//                 to retrive an element from the Hash Table
//
//
// **/

#define ODMRP_IP_HASH_KEY                 5

// /**
//  CONSTANT    :  ODMRP_SEQ_HASH_KEY  : (5)
//  DESCRIPTION :: This constant value is being used in order
//                 to retrive an element from the Hash Table
//
//
// **/

#define ODMRP_SEQ_HASH_KEY                3

// /**
//  CONSTANT    :  ODMRP_INITIAL_CHUNK  : (30)
//  DESCRIPTION :: Max number of sources of a given multicast
//                 group. This constant value is required
//                 initialize a buffer. If the number of sources
//                 exceed the given value, then it will reallocate
//                 a new buffer.
// **/

#define  ODMRP_INITIAL_CHUNK                 30

// /**
//  CONSTANT    :  ODMRP_INITIAL_ENTRY  : (10)
//  DESCRIPTION :: Max number of multicast groups. This constant value
//                 is required initialize a buffer. If the number
//                 of multicast groups  exceed the given value, then
//                 it will reallocate a new buffer.
// **/

#define  ODMRP_INITIAL_ENTRY                  10

// /**
//  CONSTANT    :  ODMRP_INITIAL_ROUTE_ENTRY  : (30)
//  DESCRIPTION :: Max number of Routing Table entries. This constant
//                 value is required initialize a buffer. If the number
//                 of Routing Table entries exceed the given value, then
//                 it will reallocate a new buffer.
// **/

#define ODMRP_INITIAL_ROUTE_ENTRY            30

// /**
//  CONSTANT    :  ODMRP_MAX_TTL_VALUE  : (255)
//  DESCRIPTION :: Max TTL values
//
// **/

#define ODMRP_MAX_TTL_VALUE              255

// /**
//  CONSTANT    :  ODMRP_DEFAULT_TTL  : (64)
//  DESCRIPTION :: Default TTL value. This is being used
//                 to send Data packet.
// **/

#define ODMRP_DEFAULT_TTL                64

// /**
//  CONSTANT    :  ODMRP_JR_REFRESH  : (20 * SECOND)
//  DESCRIPTION :: Route Refresh Interval Time
//  **/

#define ODMRP_JR_REFRESH                 (20 * SECOND )

// /**
//  CONSTANT    :  ODMRP_FG_TIMEOUT  : (60 * SECOND)
//  DESCRIPTION :: Forwarding Group TimeOut Value
//  **/

#define ODMRP_FG_TIMEOUT                    (60 * SECOND)

// /**
//  CONSTANT    :  ODMRP_CONGESTION_TIME  : (250 * MILLI_SECOND)
//  DESCRIPTION :: Network Congestion Time
//  **/

#define ODMRP_CONGESTION_TIME               (250 * MILLI_SECOND)

// /**
//  CONSTANT    :  ODMRP_CHECKACK_INTERVAL  : (75 * MILLI_SECOND)
//  DESCRIPTION :: Waiting time for a node to receive Ack message
//                 from its peer after sending "Join Reply" message.
//  **/

#define ODMRP_CHECKACK_INTERVAL             (75 * MILLI_SECOND)

// /**
//  CONSTANT    :  ODMRP_MAX_NUM_TX  : (3)
//  DESCRIPTION :: Maximum number of times "Join Reply"
//                 message will be transmitted by a node.
// **/

#define ODMRP_MAX_NUM_TX                      3

// /**
//  CONSTANT    :  ODMRP_JR_LIFETIME  : (120 * SECOND)
//  DESCRIPTION :: Expire time for a Particular Source
//                 for a given multicast group. This value is
//                 used to delete a particular source from the Temp
//                 Table Structure.
// **/

#define ODMRP_JR_LIFETIME                   (120 * SECOND)

// /**
//  CONSTANT    :  ODMRP_MEM_TIMEOUT  : (120 * SECOND)
//  DESCRIPTION :: Expire time for a Particular Source
//                 for a given multicast group. This value is being
//                 used to delete a particular source from the Member
//                 Table Structure.
// **/

#define ODMRP_MEM_TIMEOUT                   (120 * SECOND)

// /**
//  CONSTANT    :  ODMRP_TIMER_INTERVAL  : (10 * SECOND)
//  DESCRIPTION :: Checking Time for sending "Join Query" message
// **/

#define ODMRP_TIMER_INTERVAL                (10 * MILLI_SECOND)

// /**
//  CONSTANT    :  ODMRP_JR_PAUSE_TIME  : (25 * MILLI_SECOND)
//  DESCRIPTION :: Time for intermediate Forwarding Nodes to wait before
//                 forwarding "Join Reply" message.
// **/

#define ODMRP_JR_PAUSE_TIME             (25 * MILLI_SECOND)

// /**
//  CONSTANT    :  ODMRP_BROADCAST_JITTER  : (10 * MILLI_SECOND)
//  DESCRIPTION :: Jittering time when broadcast to avoid collisions.
// **/

#define ODMRP_BROADCAST_JITTER              (10 * MILLI_SECOND)

// /**
//  CONSTANT    :  ODMRP_JR_JITTER  : (15 * MILLI_SECOND)
//  DESCRIPTION :: Time to send "Join Reply" after receiving
//                 "Join Query" message from its peer.
// **/

#define ODMRP_JR_JITTER                     (10 * MILLI_SECOND)

// /**
//  CONSTANT    :  ODMRP_JR_RETX_JITTER  : (15 * MILLI_SECOND)
//  DESCRIPTION :: Jittering time when retransmitting Join Replies.
// **/

#define ODMRP_JR_RETX_JITTER                (15 * MILLI_SECOND)

// /**
//  CONSTANT    :  ODMRP_FLUSH_INTERVAL : (2 * MINUTE)
//  DESCRIPTION :: Time Interval to delete a particular
//                 entry from the Message Cache
// **/

#define ODMRP_FLUSH_INTERVAL                (2 * MINUTE)

// /**
//  CONSTANT    :  ODMRP_MAX_BUFFER_POOL : (2000)
//  DESCRIPTION :: Allocate 2000 chunk at a time
//                 This required for storing message cache
//                 information.
// **/

#define ODMRP_MAX_BUFFER_POOL              2000


// /**
//  CONSTANT    :  IPOPT_ODMRP : (219)
//  DESCRIPTION :: Odmrp Specific IP Option Name. This Option
//                 is present while sending "Join Query" message.
//
// **/

#define IPOPT_ODMRP  219

// /**
//  CONSTANT    :  ODMRP_UNKNOWN_ID : (9999)
//  DESCRIPTION :: This value is used to populate "GatewayInfo"
//                 message. This message is being sent by a Gateway
//                 Node.
// **/

#define ODMRP_UNKNOWN_ID  9999

// /**
//  CONSTANT    :  ODMRP_CLUSTER_TIME_OUT : (10 *  SECOND)
//  DESCRIPTION :: Cluster TimeOut Value.
//                 Every list is maintained by Cluster Timeout
//                 value. An entry is not updated by Cluster
//                 Timeout value must be removed from the list.
//
// **/

#define ODMRP_CLUSTER_TIMEOUT  (10 *  SECOND)

// /**
//  CONSTANT    :  IPOPT_ODMRP_CLUSTER : (119)
//  DESCRIPTION :: Passive Clustering Specific IP Option Name. This
//                 Option is present while sending Passive Clustering
//                 Related Information.
// **/

#define IPOPT_ODMRP_CLUSTER 119

// /**
// ENUM :: ODMRP_ClusterType
// DESCRIPTION:: Internal and External states of "Pasive Clustering"
//               Enabled nodes.
// **/

typedef enum
{
   ODMRP_INITIAL_NODE = 1,
   ODMRP_CLUSTER_HEAD = 2,
   ODMRP_FULL_GW = 3,
   ODMRP_DIST_GW = 4,
   ODMRP_ORDINARY_NODE = 5,
   ODMRP_CH_READY = 6,
   ODMRP_GW_READY =7,
   ODMRP_UNKNOWN_STATE
}ODMRP_ClusterType;

// /**
// STRUCT :: OdmrpPassiveClsPkt
// DESCRIPTION:: Passive Cluster Related Information.
//               This packet is sent by a non Ordinary node.
//
// **/

typedef struct {
   UInt32        opcp;//state:3,
                 //giveUpFlg:1,
                 //helloFlg:1,
                 //reserved:27;
    NodeAddress  nodeId;
    NodeAddress  clsId1;
    NodeAddress  clsId2;
}OdmrpPassiveClsPkt;

// /**
// API            :: OdmrpPassiveClsPktSetState()
// LAYER          :: Network
// PURPOSE        ::Returns the value of state for OdmrpPassiveClsPkt
// PARAMETERS     ::
// + opcp          : The variable containing the value of state,
//                   giveUpFlg,helloFlg and reserved
// + state         : Input value for set operation
// RETURN         :: void
// **/
static void OdmrpPassiveClsPktSetState (UInt32 *opcp, unsigned char state)
{
    //masks state within boundry range
    state = state & maskChar(6, 8);

    //clears first three bits
    *opcp = *opcp & maskInt(4, 32);

    //setting the value of state in first three bits position
    *opcp = *opcp | LshiftInt(state, 3);
}


// /**
// API            :: OdmrpPassiveClsPktSetGiveUpFlag()
// LAYER          :: Network
// PURPOSE        :: Returns the value of giveUpFlg for OdmrpPassiveClsPkt
// PARAMETERS     ::
// + opcp          : The variable containing the value of state,
//                   giveUpFlg,helloFlg and reserved
// + giveUpFlg     : Input value for set operation
// RETURN         :: void
// **/
static void OdmrpPassiveClsPktSetGiveUpFlag (UInt32 *opcp, BOOL giveUpFlg)
{
    //masks giveUpFlg within boundry range
    giveUpFlg = giveUpFlg & maskInt(32, 32);

    //clears the 4th bit
    *opcp = *opcp & (~(maskInt(4, 4)));

    //setting the value of giveUpFlg in the fourth place
    *opcp = *opcp | LshiftInt(giveUpFlg, 4);
}


// /**
// API            :: OdmrpPassiveClsPktSetHelloFlag()
// LAYER          :: Network
// PURPOSE        :: Returns the value of helloFlg for OdmrpPassiveClsPkt
// PARAMETERS     ::
// + opcp          : The variable containing the value of state,
//                   giveUpFlg,helloFlg and reserved
// + helloFlg      : Input value for set operation
// RETURN         :: void
// **/
static void OdmrpPassiveClsPktSetHelloFlag (UInt32 *opcp, BOOL helloFlg)
{
    //masks helloFlg within boundry range
    helloFlg = helloFlg & maskInt(32, 32);

    //clears the 5th bit
    *opcp = *opcp & (~(maskInt(5, 5)));

    //setting the value of helloFlg in the fourth place
    *opcp = *opcp | LshiftInt(helloFlg, 5);
}


// /**
// API            :: OdmrpPassiveClsPktSetReserved()
// LAYER          :: Network
// PURPOSE        :: Returns the value of reserved for OdmrpPassiveClsPkt
// PARAMETERS     ::
// + opcp          : The variable containing the value of state,
//                   giveUpFlg,helloFlg and reserved
// + reserved      : Input value for set operation
// RETURN         :: void
// **/
static void OdmrpPassiveClsPktSetReserved (UInt32 *opcp, UInt32 reserved)
{
    //masks reserved within boundry range
    reserved = reserved & maskInt(6, 32);

    //clears all the bits except the first 5
    *opcp = *opcp & maskInt(1, 5);

    //setting the value of reserved
   *opcp = *opcp | reserved;
}


// /**
// API            :: OdmrpPassiveClsPktGetState()
// LAYER          :: Network
// PURPOSE        :: Returns the value of state for OdmrpPassiveClsPkt
// PARAMETERS     ::
// + opcp          : The variable containing the value of state,
//                   giveUpFlg,helloFlg and reserved
// RETURN         :: unsigned char
// **/
static unsigned char OdmrpPassiveClsPktGetState (UInt32 opcp)
{
    UInt32 state = opcp;

    //clears all the bits except first three
    state = state & maskInt(1, 3);

    //Right shifts so that last 3 bits represent state
    state = RshiftInt(state, 3);

    return (unsigned char)state;
}


// /**
// API            :: OdmrpPassiveClsPktGetGiveUpFlag()
// LAYER          :: Network
// PURPOSE        :: Returns the value of giveUpFlg for OdmrpPassiveClsPkt
// PARAMETERS     ::
// + opcp          : The variable containing the value of state,
//                   giveUpFlg,helloFlg and reserved
// RETURN         :: BOOL
// **/
static BOOL OdmrpPassiveClsPktGetGiveUpFlag (UInt32 opcp)
{
    UInt32 giveUpFlg = opcp;

    //clear all bits except 4th
    giveUpFlg = giveUpFlg & maskInt(4, 4);

    //Right shifts so that last bit represents giveUpFlg
    giveUpFlg = RshiftInt(giveUpFlg, 4);

    return (BOOL)giveUpFlg;
}


// /**
// API            :: OdmrpPassiveClsPktGetHelloFlag()
// LAYER          :: Network
// PURPOSE        :: Returns the value of helloFlg for OdmrpPassiveClsPkt
// PARAMETERS     ::
// + opcp          : The variable containing the value of state,
//                   giveUpFlg,helloFlg and reserved
// RETURN         :: BOOL
// **/
static BOOL OdmrpPassiveClsPktGetHelloFlag (UInt32 opcp)
{
    UInt32 helloFlg = opcp;

    //clear all bits except 5th
     helloFlg = helloFlg & maskInt(5, 5);

     //Right shifts so that last bit represents helloFlg
    helloFlg = RshiftInt(helloFlg, 5);

    return (BOOL)helloFlg;
}


// /**
// API            :: OdmrpPassiveClsPktGetReserved()
// LAYER          :: Network
// PURPOSE        :: Returns the value of reserved for OdmrpPassiveClsPkt
// PARAMETERS     ::
// + opcp          : The variable containing the value of state,
//                   giveUpFlg,helloFlg and reserved
// RETURN         :: UInt32
// **/
static UInt32 OdmrpPassiveClsPktGetReserved (UInt32 opcp)
{
    UInt32 reserved = opcp;

    //clears all first 5 bits
    reserved = reserved & maskInt(6, 32);

    return reserved;
}


// /**
// ENUM :: ODMRP_PacketType
// DESCRIPTION:: Types of Odmrp Packets
//
// **/

typedef enum
{
    ODMRP_JOIN_QUERY = 1,
    ODMRP_JOIN_REPLY = 2,
    ODMRP_ACK = 3
} ODMRP_PacketType;

// /**
// STRUCT :: OdmrpIpOptionType
// DESCRIPTION:: ODMRP options fields for IP Header
//               It is being with Data Packet. Here
//               Data Packet means only Data Packet,
//               but no "Join Query" message attached with it
// **/

typedef struct
{
    int seqNumber;
    unsigned int protocol;
} OdmrpIpOptionType;

// /**
// STRUCT :: OdmrpIpOptionType
// DESCRIPTION:: ODMRP options fields for "Join Query" message
//               It is being attached with IP Options Field.
// **/

typedef struct
{
    unsigned char  pktType;
    unsigned char  reserved;
    unsigned char  ttlVal;
    unsigned char  hopCount;
    NodeAddress  mcastAddress;
    int          seqNumber;
    NodeAddress  srcAddr;
    NodeAddress  lastAddr;
    unsigned int protocol;
} OdmrpJoinQuery;

// /**
// STRUCT :: OdmrpJoinTupleForm
// DESCRIPTION:: It is Source Address and Next Address
//               Tuple present in "Join Reply" packet.
// **/

typedef struct {
   NodeAddress  srcAddr;
   NodeAddress  nextAddr;
}OdmrpJoinTupleForm;

// /**
// STRUCT :: OdmrpJoinReply
// DESCRIPTION:: Constant part of ODMRP "Join Reply" message.
//
// **/

typedef struct
 {
    unsigned char pktType;
    unsigned char replyCount;
    UInt16 ojr;//ackReq:1,
                  //IAmFG:1,
                  //reserved1:14;
    NodeAddress  multicastGroupIPAddr;
    NodeAddress  previousHopIPAddr;
    int seqNum:32;
}OdmrpJoinReply;

// /**
// API            :: OdmrpJoinReplySetAckReq()
// LAYER          :: Network
// PURPOSE        :: Set the value of ackReq for OdmrpJoinReply
// PARAMETERS     ::
// + ojr           : The variable containing the value of ackReq,IAmFG and
//                  reserved
// + ackReq        : Input value for set operation
// RETURN         :: void
// **/
static void OdmrpJoinReplySetAckReq(UInt16 *ojr, BOOL ackReq)
{
    UInt16 x = (UInt16)ackReq;

    //masks ackReq within boundry range
    x = x & maskShort(16, 16);

    //clears the first bit of ojr
    *ojr = *ojr & (~(maskShort(1, 1)));

    //Setting the value of x at first position in ojr
    *ojr = *ojr | LshiftShort(x, 1);
}


// /**
// API            :: OdmrpJoinReplySetIAmFG()
// LAYER          :: Network
// PURPOSE        :: Set the value of IAmFG for OdmrpJoinReply
// PARAMETERS     ::
// + ojr           : The variable containing the value of ackReq,IAmFG and
//                  reserved
// + IAmFG         : Input value for set operation
// RETURN         :: void
// **/
static void OdmrpJoinReplySetIAmFG (UInt16 *ojr, BOOL IAmFG)
{
    UInt16 x = (UInt16)IAmFG;

    //masks IAmFG within boundry range
    x = x & maskShort(16, 16);

    //clears the second bit of ojr
    *ojr = *ojr & (~(maskShort(2, 2)));

    //Setting the value of x at second position in ojr
    *ojr = *ojr | LshiftShort(x, 2);
}


// /**
// API            :: OdmrpJoinReplySetReserved()
// LAYER          :: Network
// PURPOSE        :: Set the value of reserved for OdmrpJoinReply
// PARAMETERS     ::
// + ojr           : The variable containing the value of ackReq,IAmFG and
//                  reserved
// + reserved      : Input value for set operation
// RETURN         :: void
// **/
static void OdmrpJoinReplySetReserved (UInt16 *ojr, UInt16 reserved)
{
    //masks reserved within boundry range
    reserved = reserved & maskShort(3, 16);

    //clears the last 14 bit of ojr
    *ojr = *ojr & (maskShort(1, 2));

    //Setting the value of ojr
    *ojr = *ojr | reserved;
}


// /**
// API            :: OdmrpJoinReplyGetAckReq()
// LAYER          :: Network
// PURPOSE        :: Returns the value of ackReq for OdmrpJoinReply
// PARAMETERS     ::
// + ojr           : The variable containing the value of ackReq,IAmFG and
//                  reserved
// RETURN         :: BOOL
// **/
static BOOL OdmrpJoinReplyGetAckReq (UInt16 ojr)
{
    UInt16 ackReq = ojr;

    //clears all bits except first bit
    ackReq = ackReq & maskShort(1, 1);

    //right shifts 15 bits so that last bit contains the value of ackReq
    ackReq = RshiftShort(ackReq, 1);

    return (BOOL) ackReq;
}


// /**
// API            :: OdmrpJoinReplyGetIAmFG()
// LAYER          :: Network
// PURPOSE        :: Returns the value of IAmFG for OdmrpJoinReply
// PARAMETERS     ::
// + ojr           : The variable containing the value of ackReq,IAmFG and
//                  reserved
// RETURN         :: BOOL
// **/
static BOOL OdmrpJoinReplyGetIAmFG (UInt16 ojr)
{
    UInt16 IAmFG = ojr;

    //clears all bits except second bit
    IAmFG = IAmFG & maskShort(2, 2);

    //right shifts 14 bits so that last bit contains the value of IAmFG
    IAmFG = RshiftShort(IAmFG, 2);

    return (BOOL)IAmFG;
}


// /**
// API            :: OdmrpJoinReplyGetReserved()
// LAYER          :: Network
// PURPOSE        ::Returns the value of reserved for OdmrpJoinReply
// PARAMETERS     ::
// + ojr           : The variable containing the value of ackReq,IAmFG and
//                  reserved
// RETURN         :: UInt16
// **/
static UInt16 OdmrpJoinReplyGetReserved (UInt16 ojr)
{
    UInt16 reserved = ojr;

    //clears first and second bits
    reserved = reserved & maskShort(3, 16);

    return reserved;
}


// /**
// STRUCT :: OdmrpAck
// DESCRIPTION:: ODMRP "Acknowledgement" packet
//
// **/

typedef struct
{
    unsigned char pktType;
    NodeAddress mcastAddr;
    NodeAddress srcAddr;
} OdmrpAck;

// /**
// STRUCT :: OdmrpAtNode
// DESCRIPTION:: Ack Table Structure is populated when
//               a node sends a "Join Reply" message.
//               When a node receives "Acknowledgement"
//               message,it deletes a particular entry
//               of Ack Table Structure
// **/

typedef struct
{
    NodeAddress srcAddr;
    NodeAddress nextAddr;
    int numTx;
} OdmrpAtSnode;

typedef struct
{
    NodeAddress mcastAddr;
    clocktype lastSent;
    DataBuffer head;
    int size;
} OdmrpAtNode;

//-----------------------------------------------------------
// /**
// STRUCT :: OdmrpMembership
// DESCRIPTION:: This structure is populated when a node
//               joins a particular multicast group.
//
//
// **/

typedef struct
{
   NodeAddress mcastAddr;
   clocktype timestamp;
} OdmrpMembership;

//-------------------------------------------------------------
// /**
// STRUCT :: OdmrpFgFlag
// DESCRIPTION:: This structure is populated when a node
//               receives a "Join Reply" message.  A particular
//               Entry of Forwarding Table Structure is deleted after
//               certain interval of time i.e ODMRP_FG_TIMEOUT
// **/

typedef struct
{
    NodeAddress mcastAddr;
    clocktype timestamp;
} OdmrpFgFlag;

//--------------------------------------------------------
// /**
// STRUCT :: OdmrpMtNode
// DESCRIPTION:: Member Table Structure is populated when
//               a node receives a "Join Query" message.
//               Member Table Structure is being used while
//               sending "Join Reply" message.
//
// **/

typedef struct
{
    NodeAddress srcAddr;
    clocktype timestamp;
} OdmrpMtSnode;

typedef struct
{
    NodeAddress mcastAddr;
    BOOL sent;                      // Flag to indicate join reply sent.
    clocktype lastSent;             // Last time that sent join reply.
    clocktype queryLastReceived;    // Join Query last received time
    DataBuffer head;
    int size;
} OdmrpMtNode;

//----------------------------------------------------
// /**
// STRUCT :: OdmrpRptNode
// DESCRIPTION:: Response Table Structure is populated when
//               a node receives a "Join Query" message.
//               Response Table Structure is being while
// **/           sending "Join Reply" message.

typedef struct
{
    NodeAddress srcAddr;
} OdmrpRptSnode;

typedef struct
{
    NodeAddress mcastAddr;
    DataBuffer head;
    int size;
} OdmrpRptNode;

//----------------------------------------------------------
// /**
// STRUCT :: OdmrpTtNode
// DESCRIPTION:: Temp Table Structure is populated when
//               a node receives a "Join Reply" message.
//               Temp Table Structure is being used to send
//               Odmrp "Join Reply" message
// **/

typedef struct
{
    NodeAddress srcAddr;
    clocktype timestamp;
    clocktype FGExpireTime;
} OdmrpTtSnode;

typedef struct
{
    NodeAddress mcastAddr;
    BOOL sent;
    DataBuffer head;
    int size;
} OdmrpTtNode;

//-----------------------------------------------------------
// /**
// STRUCT :: OdmrpRt
// DESCRIPTION:: Route Table Structure is populated when
//               a node receives a "Join Query" message.
//               Reply Table Structure is being while
// **/           sending "Join Reply" message.


typedef struct
{
   NodeAddress destAddr;
   NodeAddress nextAddr;
} OdmrpRt;

//---------------------------------------------------------
// /**
// STRUCT :: OdmrpMsgCacheEntry
// DESCRIPTION:: Message Cache Structure. It is populated
//               when a node receives Data Packet.
// **/

typedef struct struct_odmrp_msg_cache_entry
{
    NodeAddress srcAddress;
    int seqNumber;
    struct struct_odmrp_msg_cache_entry* next;
}OdmrpMsgCacheEntry;

typedef struct
{
   OdmrpMsgCacheEntry*        front;
   OdmrpMsgCacheEntry*        rear;
}OdmrpCacheTable;

typedef struct
{
  OdmrpCacheTable*  cacheTable;
} OdmrpMc;

//---------------------------------------------------------
// /**
// STRUCT :: OdmrpCacheDeleteEntry
// DESCRIPTION:: Message Cache Delete Entry. A particular
//               entry of the message Cache is being deleted
//               after a time i.e ODMRP_FLUSH_INTERVAL
// **/

typedef struct
{
    NodeAddress srcAddr;
    int         seqNumber;
}OdmrpCacheDeleteEntry;

//--------------------------------------------------------
// /**
// STRUCT :: OdmrpSs
// DESCRIPTION:: Odmrp Sent Table Structure
//               Sent Table Structure is used while
//               sending a "Join Query" message.
// **/

typedef struct
{
    NodeAddress mcastAddr;
    clocktype minExpireTime;
    clocktype lastSent;
    BOOL nextQuerySend;  // Send Join Query for next data?
} OdmrpSs;

//---------------------------------------------------------
// /**
// STRUCT :: OdmrpStats
// DESCRIPTION:: Odmrp Specific Statistics
// **/

typedef struct
{
    // Total number of "Join Query" transmitted.
    int numQueryTxed;

    // Total number of "Join Query" Originated.
    int numQueryOriginated;

    // Total number of "Join Reply" packets sent.
    int numReplySent;

    // Total number of "Join Reply" packets Retransmitted
    int numReplyTransmitted;

    // Total number of "Join Reply" packets forwarded
    int numReplyForwarded ;

    // Total number of explicit acks sent.
    int numAckSent;

    // Total number of Data packets sent by the source.
    int numDataSent;

    // Total number of data packets received by the destination.
    int numDataReceived;

    // Total number of data packets transmitted by each node.
    int numDataTxed;

        // Toatal no of Give Up message transmitted
    int numGiveUpTx;

        // Total no of GiveUp message received
    int numGiveUpRx;
    // end
} OdmrpStats;

// /**
// STRUCT :: OdmrpClsHeadPair
// DESCRIPTION:: It contains Node-Ids of two cluster Heads.
//               This structure is maintained by each node
//               participating Passive Clustering
// **/

typedef struct struct_odmrp_cls_head_pair
{
    NodeAddress clsId1;
    NodeAddress clsId2;
    struct struct_odmrp_cls_head_pair* next;
}OdmrpClsHeadPair;

//----------------------------------------------------

// /**
// STRUCT :: OdmrpGwClsEntry
// DESCRIPTION:: It contains Node-Ids of two cluster Heads.
//               This structure is being used while sending
//               Passive Clustering related information by
//               Gateway node.
//
// **/

typedef struct
{
   NodeAddress  clsId1;
   NodeAddress  clsId2;
} OdmrpGwClsEntry;

//---------------------------------------------------------

// /**
// STRUCT :: OdmrpClsHeadList
// DESCRIPTION:: It contains Cluster Head Related Information.
// **/

typedef struct struct_odmrp_head_list
{
    NodeAddress nodeId; // Node Id of Cluster Head
    clocktype lastRecvTime;
    struct struct_odmrp_head_list* next;
}OdmrpHeadList;

typedef struct struct_odmrp_cls_head_list
{
   int size;
   OdmrpHeadList* head;
}OdmrpClsHeadList;

//------------------------------------------------------

// /**
// STRUCT :: OdmrpClsGwList
// DESCRIPTION:: It contains Gateway related Information.
// **/


typedef struct struct_odmrp_gw_list
{
    NodeAddress nodeId; // Node Id of CH
    clocktype lastRecvTime;
    unsigned char  state;
    NodeAddress  clsId1;
    NodeAddress  clsId2;
    struct struct_odmrp_gw_list* next;
}OdmrpGwList;

typedef struct
{
    int size;
    OdmrpGwList* head;
}OdmrpClsGwList;

//----------------------------------------------------

// /**
// STRUCT :: OdmrpClsDistGwList
// DESCRIPTION:: It contains Dist Gateway related Information.
// **/

typedef struct struct_odmrp_dist_gw_list
{
    NodeAddress nodeId; // Node Id of CH
    clocktype lastRecvTime;
    NodeAddress  clsId1;
    NodeAddress  clsId2;
    struct struct_odmrp_dist_gw_list* next;
}OdmrpDistGwList;

typedef struct
{
    int size;
    OdmrpDistGwList* head;
}OdmrpClsDistGwList;

//-----------------------------------------------------------

typedef struct struct_odmrp_free_buffer_pool
{
   OdmrpMsgCacheEntry cacheEntry;
   int* nextPointer;
}OdmrpFreeBufferPool;

typedef struct
{
   OdmrpFreeBufferPool* head;
   int* currentPointer;
}MemPool;

// /**
// STRUCT :: OdmrpData
// DESCRIPTION:: It contains total Odmrp related Information
// **/

typedef struct struct_network_odmrp_str
{
    DataBuffer memberFlag;
    DataBuffer fgFlag;
    DataBuffer memberTable;
    DataBuffer tempTable;
    DataBuffer routeTable;
    OdmrpMc messageCache;
    MemPool freePool;
    clocktype refreshTime;
    clocktype expireTime;
    int  ttlValue;
    int seqTable;
    unsigned char      internalState;
    unsigned char      externalState;
    OdmrpGwClsEntry    gwClsEntry;
    OdmrpClsHeadList   clsHeadList;
    OdmrpClsGwList     clsGwList;
    OdmrpClsDistGwList clsDistGwList;
    clocktype          clsExpireTime;
    DataBuffer sentTable;
    OdmrpStats stats;
    DataBuffer ackTable;
    DataBuffer responseTable;
    BOOL pcEnable;
    RandomSeed retxJitterSeed;
    RandomSeed jrJitterSeed;
    RandomSeed broadcastJitterSeed;
} OdmrpData;

// /**
// API        :: OdmrpHandleProtocolEvent
// LAYER      :: Network
// PURPOSE    :: Event handler function of user protocol
// PARAMETERS ::
// + node      : Node*    : The node that is handling the event.
// + msg       : Message* : the event that is being handled
// RETURN     :: void
// **/

void OdmrpHandleProtocolEvent(Node* node, Message* msg);

// /**
// API        :: OdmrpHandleProtocolPacket
// LAYER      :: Network
// PURPOSE    :: Process a user protocol generated control packet.
// PARAMETERS ::
// + node               : Node* :this node
// + msg                : Message* :  message that is being received.
// + srcAddr            : IP Address of Source
// + destAddr           : IP Address of Destination
// RETURN     :: void
// **/

void OdmrpHandleProtocolPacket(Node* node,
                               Message* msg,
                               NodeAddress srcAddr,
                               NodeAddress destAddr);
// /**
// API        :: OdmrpRouterFunction
// LAYER      :: Network
// PURPOSE    :: Route the packet according to the Routing information.
// PARAMETERS ::
// + node               : Node* :this node
// + msg                : Message* :  message that is being received.
// + destAddr           : IP Address of Destination
// + interfaceIndex     : Default Interface of the node
// + query              : If TRUE, then "Join Query" message or Data
//                        Packet is sent FALSE otherwise
// RETURN     :: void
// **/


void OdmrpRouterFunction(Node* node,
                         Message* msg,
                         NodeAddress destAddr,
                         int interfaceIndex,
                         BOOL* packetWasRouted,
                         NodeAddress prevhop);


// API                :: OdmrpInit
// LAYER              :: Network
// PURPOSE            :: Initialization of user protocol
// PARAMETERS         ::
// + node             :   Node* : this node
// + nodeInput        : const NodeInput* : Provides access to
//                                         configuration file.
//                                                        type
// + interfaceIndex   : Interface Index of the node

void OdmrpInit(Node* node,
               const NodeInput* nodeInput,
               int interfaceIndex);

// /**
// API                 :: OdmrpFinalize
// LAYER               :: Network
// PURPOSE             :: Finalization of user protocol
// PARAMETERS          ::
// + node               : Node* : this node
// RETURN              :: void
// **/

void OdmrpFinalize(Node* node);

// /**
// API                 :: OdmrpJoinGroup
// LAYER               :: Network
// PURPOSE             :: A particular node joins a particular
//                        multicast group.
// PARAMETERS          ::
// + node               : Node* : this node
// RETURN              :: void
// **/

void OdmrpJoinGroup(Node* node,
                    NodeAddress mcastAddr);
// /**
// API                 :: OdmrpLeaveGroup
// LAYER               :: Network
// PURPOSE             :: A particular node leaves a particular
//                        multicast group.
// PARAMETERS          ::
// + node               : Node* : this node
// + mcastAddr          : Multicast Address of the Group
//
// RETURN              :: void
// **/

void OdmrpLeaveGroup(Node* node,
                     NodeAddress mcastAddr);


// /**
// API                 :: OdmrpPrintTraceXML
// LAYER               :: Network
// PURPOSE             :: Print the trace info. of ODMRP packets.
// PARAMETERS          ::
// + node               : Node* :this node
// + msg                : Message* :  packet to be traced.
// RETURN              :: void
// **/

void OdmrpPrintTraceXML(Node* node,
                        Message* msg);


// /**
// API                 :: OdmrpCheckIfItIsDuplicatePacket
// LAYER               :: Network
// PURPOSE             :: Check if the packet is duplicated.
// PARAMETERS          ::
// + node               : Node* :this node
// + msg                : Message* :  message that is being received.
// RETURN              :: BOOL : TRUE duplicated. FALSE, not duplicated.
// **/

BOOL
OdmrpCheckIfItIsDuplicatePacket(Node* node,
                                Message* msg);

#endif // _ODMRP_H_

