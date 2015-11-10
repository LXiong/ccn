// Portions of this file
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
 * This Copyright notice is in French. An English summary is given
 * but the referee text is the French one.
 *
 * Copyright (c) 2000, 2001 Adokoe.Plakoo@inria.fr, INRIA Rocquencourt,
 *                          Anis.Laouiti@inria.fr, INRIA Rocquencourt.
 *
 * Ce logiciel informatique est disponible aux conditions
 * usuelles dans la recherche, c'est-à-dire qu'il peut
 * être utilisé, copié, modifié, distribué à l'unique
 * condition que ce texte soit conservé afin que
 * l'origine de ce logiciel soit reconnue.
 * Le nom de l'Institut National de Recherche en Informatique
 * et en Automatique (INRIA), ou d'une personne morale
 * ou physique ayant participé à l'élaboration de ce logiciel ne peut
 * être utilisé sans son accord préalable explicite.
 *
 * Ce logiciel est fourni tel quel sans aucune garantie,
 * support ou responsabilité d'aucune sorte.
 * Certaines parties de ce logiciel sont dérivées de sources developpees par
 * University of California, Berkeley et ses contributeurs couvertes
 * par des copyrights.
 * This software is available with usual "research" terms
 * with the aim of retain credits of the software.
 * Permission to use, copy, modify and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies,
 * and the name of INRIA, or any contributor not be used in advertising
 * or publicity pertaining to this material without the prior explicit
 * permission. The software is provided "as is" without any
 * warranties, support or liabilities of any kind.
 * This product includes software developed by the University of
 * California, Berkeley and its contributors protected by copyrights.
 */
// /**
// PROTOCOL :: OLSR-INRIA

// SUMMARY ::
// The Optimized Link State Routing Protocol (OLSR) is a kind of
// optimization of the pure link state protocol tuned to the
// requirements for mobile ad-hoc network. It operates as a table
// driven and proactive manner and thus exchanges topology information
// with other nodes of the network on periodic basis. Its main objective
// is to minimize the control traffic by selecting a small number of nodes,
// called multi-point relay (MPR) for flooding topological information. In
// route calculation, these MPR nodes are used to form an optimal route from
// a given node to any destination in the network. This routing protocol is
// particularly suited for a large and dense network.

// LAYER ::  Application Layer

// STATISTICS ::
// + numHelloReceived : number of hello received
// + numHelloSent     : number of hello sent
// + numTcReceived    : number of TC received
// + numTcGenerated   : number of TC generated
// + numTcRelayed     : number of TC relayed
// + numMidReceived   : number of MID received
// + numMidGenerated  : number of MID generated
// + numMidRelayed    : number of MID relayed
// + numHnaReceived   : number of HNA received
// + numHnaGenerated  : number of HNA generated
// + numHnaRelayed    : number of HNA relayed

// APP_PARAM ::

// CONFIG_PARAM ::
// + ROUTING-PROTOCOL         : OLSR-INRIA : Specification of OLSR-INRIA
//                                           as routing protocol
// + OLSR-IP-VERSION          : <IPv6|IPv4>
// + OLSR-HELLO-INTERVAL      : <time-interval>
// + OLSR-TC-INTERVAL         : <time-interval>
// + OLSR-MID-INTERVAL        : <time-interval>
// + OLSR-HNA-INTERVAL        : <time-interval>
// + OLSR-NEIGHBOR-HOLD-TIME  : <time-interval>
// + OLSR-TOPOLOGY-HOLD-TIME  : <time-interval>
// + OLSR-DUPLICATE-HOLD-TIME : <time-interval>
// + OLSR-MID-HOLD-TIME       : <time-interval>
// + OLSR-HNA-HOLD-TIME       : <time-interval>

// VALIDATION ::$QUALNET_HOME/verification/olsr-inria

// IMPLEMENTED_FEATURES ::
// + OLSR control messages and information repositories, that are required for route table computation.
// + Multiple OLSR-interfaces.
// + Support for IPv6 interoperability.
// + Injection of non OLSR-MANET route into OLSR-MANET domain.
// + OLSR control messages and Table entry timeout intervals are user configurable.
// + Forwarding OLSR unsupported control messages.
// + Operability with IPv4-IPv6 networks (DualIP support).


// OMITTED_FEATURES :: (Title) : (Description) (list)
// + A node does not support both IPv4 and IPv6 configurations in an OLSR domain
//   (i.e. a node in dual IP mode), without IP tunneling. In other words,
//   a node can either run OLSR-INRIA in IPv4 mode or IPv6 mode. But not both.
// + Link Layer notification
// + Advanced Link sensing
// + Redundant topology
// + Redundant MPR flooding

// ASSUMPTIONS ::
// + Every OLSR control packet issued by QualNet’s OLSR implementation
//   contains only one type of control message.
// + The interface address
//   with the lowest index participating inside the OLSR-MANET is
//   considered to be the main address for the node.



// STANDARD :: Coding standard follows the following Coding Guidelines
//             1. SNT C Coding Guidelines for QualNet 3.2

// **/


#ifndef _OLSR_H_
#define _OLSR_H_

#define HASHSIZE     32// must be a power of 2
#define HASHMASK     HASHSIZE

// "State" of two hops neighbor.
#define NB2S_COVERED    0x1     // node has been covered by a MPR

// Link Types
#define  UNSPEC_LINK 0
#define  ASYM_LINK   1
#define  SYM_LINK    2
#define  LOST_LINK   3
#define  HIDE_LINK   4
#define  MAX_LINK    4

// Neighbor Types
#define NOT_NEIGH 0
#define SYM_NEIGH 1
#define MPR_NEIGH 2
#define MAX_NEIGH 2

// Neighbor Status
#define NOT_SYM 0
#define SYM 1

// Willingness
#define WILL_NEVER   0
#define WILL_LOW     1
#define WILL_DEFAULT 3
#define WILL_HIGH    6
#define WILL_ALWAYS  7

#define HELLOSTATUS_MAX 4

// Pseudo status, used to test coming into neighbor set
#define NBS_PENDING 0

// Message types
#define HELLO_PACKET        1   // neighborhood  info
#define TC_PACKET           2   // topology information
#define MID_PACKET          3   // MID information
#define HNA_PACKET          4

#define MSG_NOBROADCAST     0   // don't broadcast the msg
#define MSG_BROADCAST       1   // msg to broadcast

#define HOPCNT_MAX          16  // maximum hops number
#define MAX_TTL             255 // Max TTL
#define MAXPACKETSIZE       2000 // max broadcast size

#define DEFAULT_HELLO_INTERVAL (2 * SECOND)  // time to supply neighbor
                                             // database
#define REFRESH_INTERVAL       (2 * SECOND)  // time to supply neighbor database
#define DEFAULT_TC_INTERVAL    (5 * SECOND)  // time to supply topology
                                             // informations

#define DEFAULT_MID_INTERVAL   DEFAULT_TC_INTERVAL
#define DEFAULT_MID_HOLD_TIME  (3 * DEFAULT_MID_INTERVAL)

#define DEFAULT_HNA_INTERVAL   DEFAULT_TC_INTERVAL
#define DEFAULT_HNA_HOLD_TIME  (3 * DEFAULT_HNA_INTERVAL)
#define MAX_HNA_JITTER         (500 * MILLI_SECOND)

// time to delete entry in neighbor table
#define DEFAULT_NEIGHB_HOLD_TIME    (3 * REFRESH_INTERVAL)

// time to delete entry in topology table
#define DEFAULT_TOP_HOLD_TIME       (3 * DEFAULT_TC_INTERVAL)
#define MAX_MID_JITTER              (500 * MILLI_SECOND)
#define DUPLICATE_TIMEOUT_MULT       3
#define DEFAULT_DUPLICATE_HOLD_TIME  (30 * SECOND)
#define MAX_HELLO_JITTER            (100 * MILLI_SECOND)
#define MAX_TC_JITTER               (500 * MILLI_SECOND)

#define OLSR_STARTUP_DELAY          (100 * MILLI_SECOND)  // The delay at startup
                                                   // that ranges (0, 100]
                                                   // milliseconds.
#define OLSR_STARTUP_JITTER   1.0
#define VTIME_SCALE_FACTOR 0.0625


#define MAXVALUE 0xFFFF // Seqnos are 16 bit values


#define CREATE_LINK_CODE(status, link) (link | (status<<2))
#define EXTRACT_STATUS(link_code) ((link_code & 0xC)>>2)
#define EXTRACT_LINK(link_code) (link_code & 0x3)


// Macro for checking seqnos "wraparound"
#define SEQNO_GREATER_THAN(s1, s2)                \
        (((s1 > s2) && (s1 - s2 <= (MAXVALUE/2))) \
        || ((s2 > s1) && (s2 - s1 > (MAXVALUE/2))))


// /**
// STRUCT      :: duplicate_ifaces
// DESCRIPTION :: structure to hold duplicate table information
// **/

typedef struct _duplicate_ifaces
{
    Address                   interfaceAddress;
    struct _duplicate_ifaces* duplicate_iface_next;
} duplicate_ifaces;

// /**
// STRUCT      :: duplicateinfo
// DESCRIPTION :: structure to hold duplicate table information
// **/

typedef struct _duplicateinfo
{
    UInt32            duplicate_hash;
    Address           duplicate_addr;
    UInt16            duplicate_seq;
    bool              duplicate_retransmitted;
    duplicate_ifaces* dup_ifaces;
    clocktype         duplicate_timer;
} duplicateinfo;

// /**
// STRUCT      :: duplicate_entry
// DESCRIPTION :: structure to hold duplicate table information
// **/

typedef struct _duplicate_entry
{
    struct _duplicate_entry* duplicate_forw;
    struct _duplicate_entry* duplicate_back;
    duplicateinfo            duplicate_infos;
} duplicate_entry;

#define duplicate_hash          duplicate_infos.duplicate_hash
#define duplicate_addr          duplicate_infos.duplicate_addr
#define duplicate_seq           duplicate_infos.duplicate_seq
#define duplicate_timer         duplicate_infos.duplicate_timer
#define duplicate_retransmitted duplicate_infos.duplicate_retransmitted
#define dup_ifaces              duplicate_infos.dup_ifaces

// /**
// STRUCT      :: duplicatehash
// DESCRIPTION :: structure to hold duplicate table information
// **/

typedef struct _duplicatehash
{
    duplicate_entry* duplicate_forw;
    duplicate_entry* duplicate_back;
} duplicatehash;

// /**
// STRUCT      :: mpr_selector_info
// DESCRIPTION :: structure to hold MPR selector information
// **/

typedef struct _mpr_selector_info
{
    UInt32       mprselector_hash;
    Address      mpr_selector_main_addr;
    clocktype    mpr_selector_timer;
} mpr_selector_info;

// /**
// STRUCT      :: mpr_selector_entry
// DESCRIPTION :: structure to hold MPR selector information
// **/

typedef struct _mpr_selector_entry
{
    struct _mpr_selector_entry* mpr_selector_forw;
    struct _mpr_selector_entry* mpr_selector_back;
    mpr_selector_info           mpr_selector_infos;
} mpr_selector_entry;

#define mprselector_hash mpr_selector_infos.mprselector_hash
#define mpr_selector_main_addr mpr_selector_infos.mpr_selector_main_addr
#define mpr_selector_timer mpr_selector_infos.mpr_selector_timer

// /**
// STRUCT      :: mpr_selector_hash_type
// DESCRIPTION :: structure to hold MPR selector information
// **/

typedef struct _mpr_selector_hash
{
    mpr_selector_entry* mpr_selector_forw;
    mpr_selector_entry* mpr_selector_back;
} mpr_selector_hash_type;

// /**
// STRUCT      :: mpr_selector_table
// DESCRIPTION :: structure to hold MPR selector information
// **/

typedef struct _mpr_selector_table
{
    UInt16                  ansn;
    mpr_selector_hash_type* mpr_selector_hash;
} mpr_selector_table;

// /**
// STRUCT      :: mid_address
// DESCRIPTION :: structure to hold Mid Table related information
// **/

typedef struct _mid_address
{
    struct _mid_address* next;
    struct _mid_address* prev;

    Address              alias;
    struct _mid_entry*   main_entry;
    struct _mid_address* next_alias;
} mid_address;

// /**
// STRUCT      :: mid_info
// DESCRIPTION :: structure to hold Mid Table related information
// **/

typedef struct _mid_info
{
    UInt32       mid_hash;
    mid_address* aliases;
    Address      main_addr;
    clocktype    mid_timer;
} mid_info;

// /**
// STRUCT      :: mid_entry
// DESCRIPTION :: structure to hold Mid Table related information
// **/

typedef struct _mid_entry
{
    struct _mid_entry* mid_forw;
    struct _mid_entry* mid_back;
    mid_info mid_infos;
} mid_entry;

// /**
// STRUCT      :: mid_main_hash_type
// DESCRIPTION :: structure to hold Mid Table related information
// **/

typedef struct _mid_main_hash
{
    mid_entry* mid_forw;
    mid_entry* mid_back;
} mid_main_hash_type;

// /**
// STRUCT      :: mid_alias_hash_type
// DESCRIPTION :: structure to hold Mid Table related information
// **/

typedef struct _mid_alias_hash
{
    mid_address* mid_forw;
    mid_address* mid_back;
} mid_alias_hash_type;

// /**
// STRUCT      :: mid_table
// DESCRIPTION :: structure to hold Mid Table related information
// **/

typedef struct _mid_table
{
    mid_main_hash_type* mid_main_hash;
    mid_alias_hash_type* mid_alias_hash;
} mid_table;


// /**
// ENUM         :: hna_netmask
// DESCRIPTION  :: Types of netmask
// **/

typedef union _hna_netmask
{
    UInt32         v4;
    UInt32         v6;
} hna_netmask;

// /**
// STRUCT      :: hna_net
// DESCRIPTION :: structure to hold HNA Table related information
// **/

typedef struct _hna_net
{
    struct _hna_net*    prev;
    struct _hna_net*    next;

    Address             A_network_addr;
    union _hna_netmask  A_netmask;
    clocktype           A_time;
} hna_net;

// /**
// STRUCT      :: hna_entry
// DESCRIPTION :: structure to hold HNA Table related information
// **/

typedef struct _hna_entry
{
    struct _hna_entry* prev;
    struct _hna_entry* next;

    Address            A_gateway_addr;
    hna_net            networks;
} hna_entry;

// /**
// STRUCT      :: neighbor_list_entry
// DESCRIPTION :: structure to hold Two hops neighbor table related information
// **/

typedef struct _neighbor_list_entry
{
    struct _neighbor_entry*      neighbor;
    struct _neighbor_list_entry* neighbor_prev;
    struct _neighbor_list_entry* neighbor_next;
} neighbor_list_entry;

// /**
// STRUCT      :: neighbor_2_info
// DESCRIPTION :: structure to hold Two hops neighbor table related information
// **/

typedef struct _neighbor_2_info
{
    UInt32               neighbor_2_hash;
    Address              neighbor_2_addr;
    unsigned char        mpr_covered_count;
    unsigned char        processed;
    UInt16               neighbor_2_pointer;
    neighbor_list_entry* neighbor_2_nblist;
} neighbor_2_info;

// /**
// STRUCT      :: neighbor_2_entry
// DESCRIPTION :: structure to hold Two hops neighbor table related information
// **/

typedef struct _neighbor_2_entry
{
    struct _neighbor_2_entry* neighbor_2_forw;
    struct _neighbor_2_entry* neighbor_2_back;
    neighbor_2_info          neighbor_2_infos;
} neighbor_2_entry;

// /**
// STRUCT      :: neighbor_2_list_entry
// DESCRIPTION :: structure to hold Two hops neighbor table related information
// **/

typedef struct _neighbor_2_list_entry
{
    struct _neighbor_2_list_entry* neighbor_2_next;
    neighbor_2_entry*              neighbor_2;
    clocktype                      neighbor_2_timer;
} neighbor_2_list_entry;

#define neighbor_2_addr           neighbor_2_infos.neighbor_2_addr
#define mpr_covered_count         neighbor_2_infos.mpr_covered_count
#define processed                 neighbor_2_infos.processed
#define neighbor_2_pointer        neighbor_2_infos.neighbor_2_pointer
#define neighbor_2_hash           neighbor_2_infos.neighbor_2_hash
#define neighbor_2_nblist         neighbor_2_infos.neighbor_2_nblist

// /**
// STRUCT      :: neighbor2_hash
// DESCRIPTION :: structure to hold Two hops neighbor table related information
// **/

typedef struct _neighbor2_hash
{
    neighbor_2_entry *neighbor2_forw;
    neighbor_2_entry *neighbor2_back;
} neighbor2_hash;


// /**
// STRUCT      :: neighbor_info
// DESCRIPTION :: structure to hold one hop neighbor table related information
// **/

typedef struct _neighbor_info
{
    UInt32                 neighbor_hash;
    Address                neighbor_main_addr;
    Int32                  neighbor_willingness;
    unsigned char          neighbor_status;
    Int32                  neighbor_2_nocov;
    Int32                  neighbor_linkcount;
    bool                   neighbor_is_mpr;
    bool                   neighbor_was_mpr;
    bool                   neighbor_skip;
    neighbor_2_list_entry* neighbor_2_list;
} neighbor_info;

// /**
// STRUCT      :: neighbor_entry
// DESCRIPTION :: structure to hold one hop neighbor table related information
// **/

typedef struct _neighbor_entry
{
    struct _neighbor_entry* neighbor_forw;
    struct _neighbor_entry* neighbor_back;
    neighbor_info           neighbor_infos;
} neighbor_entry;

#define neighbor_hash            neighbor_infos.neighbor_hash

// one hop neighbor address
#define neighbor_main_addr       neighbor_infos.neighbor_main_addr
#define neighbor_willingness     neighbor_infos.neighbor_willingness
// status of the link between the neighbor and this node (see bellow)
#define neighbor_status          neighbor_infos.neighbor_status
// number of neighbors to the one hop neighbor and not yet covered
#define neighbor_2_nocov         neighbor_infos.neighbor_2_nocov
#define neighbor_linkcount       neighbor_infos.neighbor_linkcount
#define neighbor_is_mpr          neighbor_infos.neighbor_is_mpr
#define neighbor_was_mpr         neighbor_infos.neighbor_was_mpr
#define neighbor_skip            neighbor_infos.neighbor_skip
// list of two hops neighbors accessible by this one hop neighbor
#define neighbor_2_list          neighbor_infos.neighbor_2_list

// /**
// STRUCT      :: neighborhash_type
// DESCRIPTION :: structure to hold one hop neighbor table related information
// **/

typedef struct _neighborhash
{
    neighbor_entry* neighbor_forw;
    neighbor_entry* neighbor_back;
} neighborhash_type;

// /**
// STRUCT      :: neighbor_table
// DESCRIPTION :: structure to hold one hop neighbor table related information
// **/

typedef struct _neighbor_table
{
    UInt16             neighbor_mpr_seq;
    neighborhash_type* neighborhash;
} neighbor_table;

// /**
// STRUCT      :: link_entry
// DESCRIPTION :: structure to hold link table related information
// **/

typedef struct _link_entry
{
    Address             local_iface_addr;
    Address             neighbor_iface_addr;
    clocktype           sym_timer;
    clocktype           asym_timer;
    clocktype           timer;
    neighbor_entry*     neighbor;
    unsigned char       prev_status;
    struct _link_entry* next;
} link_entry;


// /**
// STRUCT      :: rt_entry_info
// DESCRIPTION :: structure to hold route table entry
// **/

typedef struct _rt_entry_info
{
    UInt32     rtu_hash;
    Address    rtu_dst;
    Address    rtu_router;
    UInt16     rtu_metric;
    Int32      rtu_interface;
} rt_entry_info;

// /**
// STRUCT      :: rt_entry
// DESCRIPTION :: structure to hold route table related information
// **/
typedef struct _rt_entry
{
    struct _rt_entry* rt_forw;
    struct _rt_entry* rt_back;
    rt_entry_info     rt_entry_infos;
} rt_entry;

// /**
// STRUCT      :: rthash
// DESCRIPTION :: structure to hold route table information
// **/

typedef struct _rthash
{
    rt_entry *rt_forw;
    rt_entry *rt_back;
} rthash;

#define rt_hash      rt_entry_infos.rtu_hash      // for host
#define rt_dst       rt_entry_infos.rtu_dst       // Destination
#define rt_router    rt_entry_infos.rtu_router    // who to forward to
#define rt_metric    rt_entry_infos.rtu_metric    // cost of route
#define rt_interface rt_entry_infos.rtu_interface // cost of route

// /**
// STRUCT      :: destination_n
// DESCRIPTION :: structure to hold destination wise route information
// **/

typedef struct _destination_n
{
    rt_entry*              destination;
    struct _destination_n* next;
} destination_n;

// /**
// STRUCT      :: topology_last_infos_type
// DESCRIPTION :: structure to hold topology table information
// **/
typedef struct _topology_last_infos
{
    UInt32                    topologylast_hash;
    Address                   topology_last;
    UInt16                    topology_seq;
    clocktype                 topology_timer;
    struct _destination_list* list_of_destinations;
} topology_last_infos_type;

// /**
// STRUCT      :: topology_last_entry
// DESCRIPTION :: structure to hold topology table information
// **/
typedef struct _topology_last_entry
{
    struct _topology_last_entry* topology_last_forw;
    struct _topology_last_entry* topology_last_back;
    topology_last_infos_type     topology_last_infos;
} topology_last_entry;

// /**
// STRUCT      :: last_list
// DESCRIPTION :: structure to hold topology table information
// **/
typedef struct _last_list
{
    struct _last_list*   next;
    topology_last_entry* last_neighbor;
} last_list;

// /**
// STRUCT      :: topology_destination_infos
// DESCRIPTION :: structure to hold topology table information
// **/
typedef struct _topology_destination_infos
{
    UInt32     topologydestination_hash;
    Address    topology_dst;
    last_list* list_of_last;
} topology_destination_infos;

// /**
// STRUCT      :: topology_destination_entry
// DESCRIPTION :: structure to hold topology table information
// **/
typedef struct _topology_destination_entry
{
    struct _topology_destination_entry* topology_destination_forw;
    struct _topology_destination_entry* topology_destination_back;
    topology_destination_infos          topology_destination_info;
} topology_destination_entry;

#define topologydestination_hash    \
            topology_destination_info.topologydestination_hash
#define topology_destination_dst    \
            topology_destination_info.topology_dst
#define topology_destination_list_of_last   \
            topology_destination_info.list_of_last

// /**
// STRUCT      :: destination_list
// DESCRIPTION :: structure to hold topology table information
// **/
typedef struct _destination_list
{
    struct _destination_list*   next;
    topology_destination_entry* destination_node;
} destination_list;

#define topologylast_hash             topology_last_infos.topologylast_hash
// node selected by topo_dst as a MultiPoint Relay
#define topology_last                 topology_last_infos.topology_last
// sequence number with which topo_last announced this information
#define topology_seq                  topology_last_infos.topology_seq
#define topology_timer                topology_last_infos.topology_timer
#define topology_list_of_destinations topology_last_infos.list_of_destinations

// /**
// STRUCT      :: topology_last_hash
// DESCRIPTION :: structure to hold topology table information
// **/
typedef struct _topology_last_hash
{
    topology_last_entry* topology_last_forw;
    topology_last_entry* topology_last_back;
} topology_last_hash;

// /**
// STRUCT      :: topology_destination_hash
// DESCRIPTION :: structure to hold topology table information
// **/
typedef struct _topology_destination_hash
{
    topology_destination_entry* topology_destination_forw;
    topology_destination_entry* topology_destination_back;
} topology_destination_hash;

// /**
// STRUCT      :: olsr_qelem
// DESCRIPTION :: structure of internal olsr-queue
// **/
typedef struct _olsr_qelem
{
    struct _olsr_qelem* q_forw;
    struct _olsr_qelem* q_back;
    char                q_data[1];
} olsr_qelem;

// /**
// STRUCT      :: hello_neighbor
// DESCRIPTION :: Internal Hello Msg structure related information
// **/
typedef struct _hello_neighbor
{
    unsigned char           status;
    unsigned char           link;
    Address                 main_address;
    Address                 address;
    struct _hello_neighbor* next;
} hello_neighbor;

// /**
// STRUCT      :: hl_message
// DESCRIPTION :: Internal Hello Msg structure related information
// **/
typedef struct _hl_message
{
    Float32         vtime;
    Float32         htime;
    Address         source_addr;
    UInt16          packet_seq_number;
    unsigned char   hop_count;
    unsigned char   ttl;
    unsigned char   willingness;
    hello_neighbor* neighbors;
} hl_message;

// /**
// STRUCT      :: tc_mpr_addr
// DESCRIPTION :: Internal TC Msg structure related information
// **/
typedef struct _tc_mpr_addr
{
    Address              address;
    struct _tc_mpr_addr* next;
} tc_mpr_addr;

// /**
// STRUCT      :: tc_message
// DESCRIPTION :: Internal TC Msg structure related information
// **/
typedef struct _tc_message
{
    Float32        vtime;
    Address        source_addr;
    Address        originator;
    UInt16         packet_seq_number;
    unsigned char  hop_count;
    unsigned char  ttl;
    UInt16         ansn;
    tc_mpr_addr*   multipoint_relay_selector_address;
} tc_message;

// /**
// STRUCT      :: mid_alias
// DESCRIPTION :: Internal MID Msg structure related information
// **/
typedef struct _mid_alias
{
    Address           alias_addr;
    struct _mid_alias *next;
} mid_alias;

// /**
// STRUCT      :: mid_message
// DESCRIPTION :: Internal MID Msg structure related information
// **/
typedef struct _mid_message
{
      Float32        vtime;
      Address        mid_origaddr;  // originator's address
      unsigned char  mid_hopcnt;    // number of hops to destination
      unsigned char  mid_ttl;       // ttl
      UInt16         mid_seqno;     // sequence number
      Address        main_addr;     // main address
      mid_alias      *mid_addr;     // variable length
} mid_message;

// /**
// STRUCT      :: hna_net_addr
// DESCRIPTION :: Internal HNA Msg structure related information
// **/
typedef struct _hna_net_addr
{
    struct _hna_net_addr* next;

    Address               net;
    hna_netmask           netmask;
} hna_net_addr;

// /**
// STRUCT      :: hna_message
// DESCRIPTION :: Internal HNA Msg structure related information
// **/
typedef struct _hna_message
{
    Float32              vtime;
    Address              hna_origaddr;
    UInt16               hna_seqno;
    unsigned char        hna_hopcnt;
    unsigned char        hna_ttl;
    hna_net_addr*        hna_net_pair;
} hna_message;

// /**
// STRUCT      :: hellinfo
// DESCRIPTION :: Hello packet related information
// **/
typedef struct _hellinfo
{
    unsigned char  link_code;       // link type
    unsigned char  res[1];          // padding
    UInt16         link_msg_size;   // Size
    union
    {
        NodeAddress ipv4_addr;
        in6_addr    ipv6_addr;
    } neigh_addr[1];   // neighbor address (variable length)
} hellinfo;

// /**
// STRUCT      :: hellomsg
// DESCRIPTION :: Hello packet related information
// **/
typedef struct _hellomsg
{
    UInt16         reserved;
    unsigned char  htime;
    unsigned char  willingness;
    hellinfo       hell_info[1];
} hellomsg;

// /**
// STRUCT      :: tcmsg
// DESCRIPTION :: TC packet related information
// **/
typedef struct _tcmsg
{
    UInt16 ansn; // msg sequence number
    UInt16 reserved;
    union
    {
        // advertised neighbor main address (variable length)
        NodeAddress ipv4_addr;
        in6_addr ipv6_addr;
    } adv_neighbor_main_address[1];
} tcmsg;


// /**
// STRUCT      :: midmsg
// DESCRIPTION :: MID packet related information
// **/
typedef struct _midmsg
{
    union
    {
        NodeAddress ipv4_addr;
        in6_addr ipv6_addr;
    } olsr_iface_addr[1];
} midmsg;

// /**
// STRUCT      :: _hnapair_ipv4
// DESCRIPTION :: HNA packet related information
// **/
struct _hnapair_ipv4
{
  NodeAddress   addr;
  NodeAddress   netmask;
};

// /**
// STRUCT      :: _hnapair_ipv6
// DESCRIPTION :: HNA packet related information
// **/
struct _hnapair_ipv6
{
  in6_addr   addr;
  in6_addr   netmask;
};

// /**
// STRUCT      :: hnapair
// DESCRIPTION :: HNA packet related information
// **/
typedef union _hnapair
{
  struct _hnapair_ipv4 hnapair_ipv4;
  struct _hnapair_ipv6 hnapair_ipv6;
} hnapair;

// /**
// STRUCT      :: hnamsg
// DESCRIPTION :: HNA packet related information
// **/
typedef struct _hnamsg
{
    hnapair hna_net_pair[1];
} hnamsg;

// /**
// STRUCT      :: olsrmsg
// DESCRIPTION :: olsr msg related information
// **/
typedef struct _olsrmsg
{
    unsigned char  olsr_msgtype;    // msg type
    unsigned char  olsr_vtime;    // vtime
    UInt16 olsr_msg_size; // msg size

    union
    {
        struct
        {
            NodeAddress originator_address;
            unsigned char ttl;
            unsigned char hop_count;
            UInt16 msg_seq_no;

            union
            {
                hellomsg un_hello[1];
                tcmsg    un_tc[1];
                midmsg   un_mid[1];
                hnamsg   un_hna[1];
            } olsrun;
        } olsr_ipv4_msg;

        struct
        {
            in6_addr originator_address;
            unsigned char ttl;
            unsigned char hop_count;
            UInt16 msg_seq_no;

            union
            {
                hellomsg un_hello[1];
                tcmsg    un_tc[1];
                midmsg   un_mid[1];
                hnamsg   un_hna[1];
            } olsrun;
         } olsr_ipv6_msg;
     } olsr_msg_tail;
} olsrmsg;

#define originator_ipv4_address olsr_msg_tail.olsr_ipv4_msg.originator_address
#define originator_ipv6_address olsr_msg_tail.olsr_ipv6_msg.originator_address

#define olsr_ipv4_hello      olsr_msg_tail.olsr_ipv4_msg.olsrun.un_hello
#define olsr_ipv6_hello      olsr_msg_tail.olsr_ipv6_msg.olsrun.un_hello

#define olsr_ipv4_tc      olsr_msg_tail.olsr_ipv4_msg.olsrun.un_tc
#define olsr_ipv6_tc      olsr_msg_tail.olsr_ipv6_msg.olsrun.un_tc

#define olsr_ipv4_mid      olsr_msg_tail.olsr_ipv4_msg.olsrun.un_mid
#define olsr_ipv6_mid      olsr_msg_tail.olsr_ipv6_msg.olsrun.un_mid

#define olsr_ipv4_hna      olsr_msg_tail.olsr_ipv4_msg.olsrun.un_hna
#define olsr_ipv6_hna      olsr_msg_tail.olsr_ipv6_msg.olsrun.un_hna

// /**
// STRUCT      :: olsrpacket
// DESCRIPTION :: olsr pkt related information
// **/
typedef struct _olsrpacket
{
    UInt16   olsr_packlen;       // packet length
    UInt16   olsr_pack_seq_no;   // packet seq_no
    olsrmsg  olsr_msg[1];
} olsrpacket;



// /**
// STRUCT      :: RoutingOlsrStat
// DESCRIPTION :: olsr stat related information
// **/
typedef struct struct_routing_olsr_stat
{
    Int32 numHelloReceived;        // number of hello received
    Int32 numHelloSent;            // number of hello sent
    Int32 numTcReceived;           // number of TC received
    Int32 numTcGenerated;          // number of TC generated
    Int32 numTcRelayed;            // number of TC relayed
    Int32 numMidReceived;          // number of MID received
    Int32 numMidGenerated;         // number of MID generated
    Int32 numMidRelayed;           // number of MID relayed
    Int32 numHnaReceived;          // number of HNA received
    Int32 numHnaGenerated;         // number of HNA generated
    Int32 numHnaRelayed;           // number of HNA relayed
} RoutingOlsrStat;

// /**
// STRUCT      :: RoutingOlsr
// DESCRIPTION :: node-level olsr related information
// **/
typedef struct struct_routing_olsr
{
    NetworkType               ip_version;
    Address                   main_address;

    UInt16*                   interfaceSequenceNumbers;
    short                     numOlsrInterfaces;
    unsigned char             mpr_coverage;
    unsigned char             willingness;

    clocktype                 hello_interval;
    clocktype                 tc_interval;
    clocktype                 mid_interval;
    clocktype                 hna_interval;

    clocktype                 neighb_hold_time;
    clocktype                 top_hold_time;
    clocktype                 dup_hold_time;
    clocktype                 mid_hold_time;
    clocktype                 hna_hold_time;

    mpr_selector_table        mprstable;                // MPR selector table
    duplicatehash             duplicatetable[HASHSIZE]; // duplicate table
    neighbor_table            neighbortable;            // neighbor table
    neighbor2_hash            neighbor2table[HASHSIZE]; // neighbor 2 table
    mid_table                 midtable;                 // mid table
    hna_entry                 hna_table[HASHSIZE];      // hna table

    rthash                    routingtable[HASHSIZE];   // routing table
    rthash                    mirror_table[HASHSIZE];   // routing mirror
                                                        // table
    topology_destination_hash topologytable[HASHSIZE];  // topology table
    topology_last_hash        topologylasttable[HASHSIZE]; // topo last
                                                           // table
    UInt16                    message_seqno; // message seq number
    BOOL                      changes_neighborhood; // neighborhood changed
    BOOL                      changes_topology;     // topology changed
    BOOL                      tcMessageStarted;     // TC message started?
    BOOL                      statsPrinted;  // to check whether stat
                                             // already printed or not
    RoutingOlsrStat           olsrStat;      // OLSR stat
    clocktype                 tc_valid_time; // To keep track of valid time
                                             // of the last tc sent
    link_entry*               link_set;                 // Link Set
    RandomSeed                seed;                     // for random jitter
} RoutingOlsr;



// /**
// FUNCTION   :: RoutingOlsrInriaInit
// LAYER      :: APPLICATION
// PURPOSE    :: Initialization function for OLSR.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + nodeInput : const NodeInput* : Pointer to input configuration
// + interfaceIndex: Int32 : Interface being initialized
// RETURN :: void : NULL
// **/
void RoutingOlsrInriaInit(
    Node *node,
    const NodeInput* nodeInput,
    Int32 interfaceIndex,
    NetworkType networkType);


// /**
// FUNCTION   :: RoutingOlsrInriaFinalize
// LAYER      :: APPLICATION
// PURPOSE    :: Called at the end of simulation to collect the results of
//               the simulation of the OLSR routing protocol.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interfaceId : Int32 : interface index
// RETURN :: void : NULL
// **/
void RoutingOlsrInriaFinalize(
    Node* node,
    Int32 interfaceId);

// /**
// FUNCTION   :: RoutingOlsrInriaLayer
// LAYER      :: APPLICATION
// PURPOSE    :: The main layer structure routine, called from application.cpp
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + msg : Message* : Message to be handled
// RETURN :: void : NULL
// **/
void RoutingOlsrInriaLayer(
    Node* node,
    Message* msg);

//--------------------------------------------------------------------------
// FUNCTION   :: RoutingOlsrvInriaHandleChangeAddressEvent
// PURPOSE    :: Handles any change in the interface address
// PARAMETERS ::
// + node                    : Node*       : Pointer to Node structure
// + interfaceIndex          : Int32       : interface index
// + oldAddress              : Address*    : old address
// + subnetMask              : NodeAddress : subnetMask
// + NetworkType networkType : type of network protocol
// RETURN     ::             : void        : NULL
//--------------------------------------------------------------------------
void RoutingOlsrvInriaHandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    Address* oldAddress,
    NodeAddress subnetMask,
    NetworkType networkType);

//--------------------------------------------------------------------------
// FUNCTION   :: RoutingOlsrHandleChangeAddressEvent
// LAYER      :: APPLICATION
// PURPOSE    :: Handles any change in the interface address
//               due to autoconfiguration feature
// PARAMETERS ::
// + node               : Node* : Pointer to Node structure
// + interfaceIndex     : Int32 : interface index
// + oldGlobalAddress   : in6_addr* old global address
// RETURN :: void : NULL
//---------------------------------------------------------------------------
void RoutingOlsrHandleChangeAddressEvent(
    Node* node,
    Int32 interfaceIndex,
    in6_addr* oldGlobalAddress);


#endif /* _OLSR_H_ */
