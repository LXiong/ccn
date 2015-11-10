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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "api.h"
#include "app_util.h"
#include "network_ip.h"

#include "transport_udp.h"
#include "ipv6.h"

#include "routing_olsr-inria.h"

#define DEBUG          0
#define DEBUG_OUTPUT   0

// /**
// FUNCTION   ::  OlsrLookup2HopNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Searches an address in the two hop neighbor table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + dest : Address: address to be searched
// RETURN :: neighbor_2_entry* : Pointer to entry if found, else NULL
// **/
neighbor_2_entry*
OlsrLookup2HopNeighborTable(
    Node* node,
    Address dest);

// /**
// FUNCTION   :: OlsrDelete2HopNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Delete a two hop neighbor entry results the deletion of
//               its 1 hop neighbors list. We don't try to remove the one
//               hop neighbor even if it has no more 2 hop neighbors.
// PARAMETERS ::
// + two_hop_neighbor : neighbor_2_entry* : Pointer to two hop neighbor entry
// RETURN :: void : NULL
// **/
void OlsrDelete2HopNeighborTable(
    neighbor_2_entry* two_hop_neighbor);

// /**
// FUNCTION   :: OlsrLookupNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Searches neighbor table for this address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + dst : Address: Address to look for
// RETURN :: neighbor_entry * : if found, pointer to entry
//                              else NULL
// **/
neighbor_entry* OlsrLookupNeighborTable(
    Node* node,
    Address dst);

// /**
// FUNCTION   :: OlsrInsertNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Inserts neighbor entry in the neighbor table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + neighbor: neighbor_entry * : entry to be inserted
// RETURN :: void : NULL
// **/
void OlsrInsertNeighborTable(
    Node* node,
    neighbor_entry* neighbor);

// /**
// FUNCTION   :: OlsrDeleteNeighbor2Pointer
// LAYER      :: APPLICATION
// PURPOSE    :: Deletes current address from the 2 hop neighbor list of this
//               neighbor entry
// PARAMETERS ::
// + neighbor : neighbor_entry* : Pointer to neighbor entry
// + address : Address : Address to be deleted from two hop neighbor list
// RETURN :: void : NULL
// **/
void OlsrDeleteNeighbor2Pointer(
    neighbor_entry* neighbor,
    Address address);

// /**
// FUNCTION   :: OlsrDeleteNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Deletes current neighbor entry from the neighbor table
// PARAMETERS ::
// + neighbor : neighbor_entry* : Pointer to entry to be deleted
// RETURN :: void : NULL
// **/
void OlsrDeleteNeighborTable(
    neighbor_entry* neighbor);

// /**
// FUNCTION   :: OlsrForwardMessage
// LAYER      :: APPLICATION
// PURPOSE    :: Check if a message is to be forwarded and forward
//               it if necessary.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + m : olsrmsg * : Pointer to message
// + originator: Address: originator of message
// + seqno : UInt16 : seq no of message
// + interfaceIndex : Int32: interface that packet arrived on
// + from_addr : Address: Neighbor Interface address that transmitted msg
// RETURN :: Int32 : 1 if msg is sent to be fwded, 0 if not
// **/
Int32 OlsrForwardMessage(
    Node* node,
    olsrmsg* m,
    Address originator,
    UInt16 seqno,
    Int32 interfaceIndex,
    Address from_addr);

// /**
// FUNCTION   :: RoutingOlsrCheckMyIfaceAddress
// LAYER      :: APPLICATION
// PURPOSE    :: Function checks if address is not one of the
//               current node's interface address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + addr : Address: Address to be checked
// RETURN :: Int32 : Returns interface index if match, else -1
// **/
Int32 RoutingOlsrCheckMyIfaceAddress(
    Node* node,
    Address addr);

// /**
// FUNCTION   :: OlsrLookupMprSelectorTable
// LAYER      :: APPLICATION
// PURPOSE    :: Search an address in the MPR  selector table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + dst  : Address : Address to be searched
// RETURN :: mpr_selector_entry* : Pointer to entry if found, else NULL
// **/
mpr_selector_entry* OlsrLookupMprSelectorTable(
    Node* node,
    Address addr);

// /**
// FUNCTION   :: OlsrDeleteMprSelectorTable
// LAYER      :: APPLICATION
// PURPOSE    :: Deletes entry from MPR  selector table
// PARAMETERS ::
// + mprs : mpr_selector_entry *: Pointer to mpr entry
// RETURN :: void : NULL
// **/
void OlsrDeleteMprSelectorTable(
    mpr_selector_entry* mprs);

/***************************************************************************
 *                    Definition of List Management                        *
 ***************************************************************************/

// /**
// FUNCTION   :: OlsrInsertList
// LAYER      :: APPLICATION
// PURPOSE    :: Inserts in the list
// PARAMETERS ::
// + elem : ols_qelem* : Pointer to element
// + prev : ols_qelem* : Pointer to previous element
// RETURN :: void : NULL
// **/

static
void OlsrInsertList(
    olsr_qelem* elem,
    olsr_qelem* prev)
{
    elem->q_back = prev;
    elem->q_forw = (olsr_qelem *)prev->q_forw;
    prev->q_forw = elem;
    ((olsr_qelem *) elem->q_forw)->q_back = elem;
}


// /**
// FUNCTION   :: OlsrRemoveList
// LAYER      :: APPLICATION
// PURPOSE    :: Removes from  the list
// PARAMETERS ::
// + elem : ols_qelem* : Pointer to element
// RETURN :: void : NULL
// **/

static
void OlsrRemoveList(
    olsr_qelem* elem)
{
    ((olsr_qelem *) elem->q_back)->q_forw = (olsr_qelem *)elem->q_forw;
    ((olsr_qelem *) elem->q_forw)->q_back = (olsr_qelem *)elem->q_back;
}


// /**
// FUNCTION   :: OlsrHashing
// LAYER      :: APPLICATION
// PURPOSE    :: Returns hash value
// PARAMETERS ::
// + address      : Address : Address of node
// + h : UInt32*  : returned hash value
// RETURN :: void : NULL
// **/

static
void OlsrHashing(
    Address address,
    UInt32* h)
{
    UInt32 hash = 0;

    if (address.networkType == NETWORK_IPV6)
    {
        unsigned int byte = 0;

        byte = address.interfaceAddr.ipv6.s6_addr[12];
        hash |= (byte << 24);

        byte = address.interfaceAddr.ipv6.s6_addr[13];
        hash |= (byte << 16);

        byte = address.interfaceAddr.ipv6.s6_addr[14];
        hash |= (byte << 8);

        byte = address.interfaceAddr.ipv6.s6_addr[15];
        hash |= byte;
    }
    else

    {
        hash = GetIPv4Address(address);
    }
    *h = hash;
}


// /**
// FUNCTION   :: OlsrDouble2ME
// LAYER      :: APPLICATION
// PURPOSE    :: Converts a Float32 to mantissa/exponent
//               format as per RFC 3626
// PARAMETERS ::
// + interval : Float32 : value to be converted
// RETURN :: unsigned char : 8 bit mantissa/exponent product
// **/

static
unsigned char OlsrDouble2ME(
    Float32 interval)
{
    // Section 18.3 RFC 3626
    Int32 a;
    Int32 b;

    b = 0;
    while (interval / VTIME_SCALE_FACTOR >= pow((Float32)2, (Float32)b))
    {
        b++;
    }
    b--;
    if (b < 0)
    {
        a = 1;
        b = 0;
    }
    else if (b > 15)
    {
        a = 15;
        b = 15;
    }
    else
    {
        a = (Int32)(16 * ((Float32)interval /
                       (VTIME_SCALE_FACTOR * (Float32)pow((double)2, b)) - 1));
        while (a >= 16)
        {
            a -= 16;
            b++;
        }
    }
    unsigned char value;
    value = (unsigned char) (a * 16 + b);
    return value;
}


// /**
// FUNCTION   :: OlsrME2Double
// LAYER      :: APPLICATION
// PURPOSE    :: Converts a mantissa/exponent
//               format  to Float32 as per RFC 3626
// PARAMETERS ::
// + me : unsigned char : 8 bit m/e value to be converted
// RETURN :: Float32 : returns 32-bit value
// **/

static
Float32 OlsrME2Double(
    unsigned char me)
{
    // Section 18.3 RFC 3626
    Int32 a;
    Int32 b;

    a = me >> 4;
    b = me - a * 16;

    return (Float32)(VTIME_SCALE_FACTOR * (1 + (Float32)a / 16)
                   * (Float32)pow((double)2, b));
}


// /**
// FUNCTION   :: OlsrGetMsgSeqNo
// LAYER      :: APPLICATION
// PURPOSE    :: Returns current message sequence counter
//               and increments the counter
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: short : Message sequence number
// **/

static
Int16 OlsrGetMsgSeqNo(
    Node* node)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    return olsr->message_seqno++;
}


//*************************************************************************//
//                          MID Table Handling                             //
//*************************************************************************//

// /**
// FUNCTION   :: OlsrMidLookupMainAddr
// LAYER      :: APPLICATION
// PURPOSE    :: Returns main_address given an alias
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + alias : Address : alias address
// RETURN :: Address : main address of the alias
// **/

static
Address OlsrMidLookupMainAddr(
    Node* node,
    Address alias)
{
    UInt32 alias_addr_hash;
    mid_alias_hash_type* hash_mid_alias;
    mid_address* curr_mid_addr;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrHashing(alias, &alias_addr_hash);

    hash_mid_alias = &olsr->midtable.mid_alias_hash[alias_addr_hash
                                                    % HASHMASK];
    for (curr_mid_addr = hash_mid_alias->mid_forw;
         curr_mid_addr != (mid_address* )hash_mid_alias;
         curr_mid_addr = curr_mid_addr->next)
    {
        if (Address_IsSameAddress(&(curr_mid_addr->alias), &alias))
        {
            return curr_mid_addr->main_entry->mid_infos.main_addr;
        }
    }

    Address any_addr;
    Address_SetToAnyAddress(&any_addr, &alias);
    return any_addr;
}


// /**
// FUNCTION   :: OlsrLookupMidAliases
// LAYER      :: APPLICATION
// PURPOSE    :: Returns a link list of aliases for a given main_address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + main_addr :    Address : main_address
// RETURN :: mid_address* : link list of aliases of the given main_address
// **/

static
mid_address* OlsrLookupMidAliases(
    Node* node,
    Address main_addr)
{
    UInt32 main_addr_hash;
    mid_main_hash_type* hash_mid_main;
    mid_entry* curr_mid_entry;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrHashing(main_addr, &main_addr_hash);

    hash_mid_main = &olsr->midtable.mid_main_hash[main_addr_hash % HASHMASK];

    for (curr_mid_entry = hash_mid_main->mid_forw;
         curr_mid_entry != (mid_entry *)hash_mid_main;
         curr_mid_entry = curr_mid_entry->mid_forw)
    {
        if (Address_IsSameAddress(&curr_mid_entry->mid_infos.main_addr,
                &main_addr))
        {
            return curr_mid_entry->mid_infos.aliases;
        }
    }
    return NULL;
}

// /**
// FUNCTION   :: OlsrInsertMidTable
// LAYER      :: APPLICATION
// PURPOSE    :: Insert an alias entry into the MID table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + main_addr : Address : Main address of an olsr-node
// + alias : mid_address* : alias entry
// + vtime : Float32 : Validity time
// RETURN :: void : NULL
// **/
static
void OlsrInsertMidTable(
    Node* node,
    Address main_addr,
    mid_address* alias,
    Float32 vtime)
{
    UInt32 alias_addr_hash;
    UInt32 main_addr_hash;
    mid_main_hash_type* hash_mid_main;
    mid_alias_hash_type* hash_mid_alias;
    mid_entry* curr_mid_entry;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrHashing(main_addr, &main_addr_hash);
    OlsrHashing(alias->alias, &alias_addr_hash);

    hash_mid_main = &olsr->midtable.mid_main_hash[main_addr_hash % HASHMASK];
    hash_mid_alias = &olsr->midtable.mid_alias_hash[alias_addr_hash
                                                    % HASHMASK];

    for (curr_mid_entry = hash_mid_main->mid_forw;
         curr_mid_entry != (mid_entry *)hash_mid_main;
         curr_mid_entry = curr_mid_entry->mid_forw)
    {
        if (Address_IsSameAddress(&curr_mid_entry->mid_infos.main_addr,
                &main_addr))
        {
            break;
        }
    }

    if (curr_mid_entry != (mid_entry* )hash_mid_main)
    {
        alias->next_alias = curr_mid_entry->mid_infos.aliases;
    }
    else
    {
        curr_mid_entry = (mid_entry* )MEM_malloc(sizeof(mid_entry));
        memset(curr_mid_entry, 0, sizeof(mid_entry));
        OlsrInsertList((olsr_qelem* )curr_mid_entry,
            (olsr_qelem* )hash_mid_main);

        curr_mid_entry->mid_infos.main_addr = main_addr;
        alias->next_alias = NULL;
    }

    curr_mid_entry->mid_infos.aliases = alias;
    alias->main_entry = curr_mid_entry;
    curr_mid_entry->mid_infos.mid_timer = node->getNodeTime()
                                          + (clocktype)(vtime * SECOND);
    OlsrInsertList((olsr_qelem* )alias, (olsr_qelem* )hash_mid_alias);

    return;
}

// /**
// FUNCTION   :: OlsrInsertMidAlias
// LAYER      :: APPLICATION
// PURPOSE    :: Insert alias address into MID table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + main_address : Address : main address
// + alias : Address : alias address
// + vtime : Float32 : Validity time
// RETURN :: void : NULL
// **/
static
void OlsrInsertMidAlias(
    Node* node,
    Address main_address,
    Address alias,
    Float32 vtime)
{
    mid_address* mid_alias = (mid_address* )MEM_malloc(sizeof(mid_address));
    memset(mid_alias, 0, sizeof(mid_address));

    mid_alias->alias = alias;
    mid_alias->next_alias = NULL;

    OlsrInsertMidTable(node, main_address, mid_alias, vtime);

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    olsr->changes_topology = TRUE;
    olsr->changes_neighborhood = TRUE;

    return;
}


// /**
// FUNCTION   :: OlsrDeleteMidTable
// LAYER      :: APPLICATION
// PURPOSE    :: Delete a MID table entry
// PARAMETERS ::
// + entry : mid_entry* : Pointer to MID table entry
// RETURN :: void : NULL
// **/
static
void OlsrDeleteMidTable(
    mid_entry* entry)
{
    mid_address *aliases;

    // Free aliases
    aliases = entry->mid_infos.aliases;
    while (aliases)
    {
        mid_address *tmp_aliases = aliases;
        aliases = aliases->next_alias;
        OlsrRemoveList((olsr_qelem *) tmp_aliases);
        free(tmp_aliases);
    }
    OlsrRemoveList((olsr_qelem *)entry);
    free(entry);

    return;
}

// /**
// FUNCTION   :: OlsrTimeoutMidTable
// LAYER      :: APPLICATION
// PURPOSE    :: Check MID table entry for time out
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/
static
void OlsrTimeoutMidTable(
    Node* node)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    unsigned char index;
    for (index = 0; index < HASHSIZE; index++)
    {
        mid_entry* tmp_list = olsr->midtable.mid_main_hash[index].mid_forw;

        // Traverse MID list
        while (tmp_list != (mid_entry* )&olsr->midtable.mid_main_hash[index])
        {
            if (node->getNodeTime() >= tmp_list->mid_infos.mid_timer)
            {
                mid_entry* entry_to_delete = tmp_list;
                tmp_list = tmp_list->mid_forw;

                if (DEBUG)
                {
                    char addrString[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(
                        &entry_to_delete->mid_infos.main_addr,
                        addrString);
                    printf("Node %u: Deleting MID table entry "
                           "for main address %s\n",
                        node->nodeId,
                        addrString);
                }
                OlsrDeleteMidTable(entry_to_delete);
            }
            else
            {
                tmp_list = tmp_list->mid_forw;
            }
        }
    }

    return;
}

// /**
// FUNCTION   :: OlsrUpdateMidTable
// LAYER      :: APPLICATION
// PURPOSE    :: Update timer of a MID table entry
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + main_addr : Address : Main address of an olsr-node
// + vtime : Float32 : Validity time
// RETURN :: void : NULL
// **/
static
void OlsrUpdateMidTable(
    Node* node,
    Address main_addr,
    Float32 vtime)
{
    UInt32 main_addr_hash;
    mid_main_hash_type* hash_mid_main;
    mid_entry* curr_mid_entry;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    OlsrHashing(main_addr, &main_addr_hash);

    hash_mid_main = &olsr->midtable.mid_main_hash[main_addr_hash % HASHMASK];

    for (curr_mid_entry = hash_mid_main->mid_forw;
         curr_mid_entry != (mid_entry *)hash_mid_main;
         curr_mid_entry = curr_mid_entry->mid_forw)
    {
        if (Address_IsSameAddress(&curr_mid_entry->mid_infos.main_addr,
                &main_addr))
        {
            curr_mid_entry->mid_infos.mid_timer =
                node->getNodeTime() + (clocktype)(vtime * SECOND);
            return;
        }
    }
    return;
}

// /**
// FUNCTION   :: OlsrPrintMidTable
// LAYER      :: APPLICATION
// PURPOSE    :: Print MID table of an olsr node
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/
static
void OlsrPrintMidTable(
    Node* node)
{
    unsigned char index;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    char addrString[MAX_STRING_LENGTH];


    printf("\nMID table for Node:%u\n", node->nodeId);

    for (index = 0; index < HASHSIZE; index++)
    {
        mid_entry *tmp_list = olsr->midtable.mid_main_hash[index].mid_forw;

        // Traverse MID list
        for (tmp_list = olsr->midtable.mid_main_hash[index].mid_forw;
            tmp_list != (mid_entry* )&olsr->midtable.mid_main_hash[index];
            tmp_list = tmp_list->mid_forw)
        {
            mid_address* tmp_addr = tmp_list->mid_infos.aliases;

            IO_ConvertIpAddressToString(&tmp_list->mid_infos.main_addr,
                addrString);
            printf("%s(Main Address) : ", addrString);
            printf("[Alias Addresses : ");

            while (tmp_addr)
            {
                IO_ConvertIpAddressToString(&tmp_addr->alias, addrString);
                printf("%s ", addrString);

                tmp_addr = tmp_addr->next_alias;
            }
            printf("]\n");
        }
    }
    return;
}

//*************************************************************************//
//                         HNA Table Handling
//*************************************************************************//

// /**
// FUNCTION   :: OlsrLookupHnaNet
// LAYER      :: APPLICATION
// PURPOSE    :: Looks up HNA table for a given net address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + nets : hna_net* : pointer to the network list to look in
// + net  : Address : The network to look for
// + mask : hna_netmask : The netmask to look for
// RETURN :: hna_net* : pointer to the entry or NULL if not found
// **/
hna_net* OlsrLookupHnaNet(
    Node* node,
    hna_net* nets,
    Address net,
    hna_netmask mask)
{
  hna_net* tmp_net;

  // Loop trough entrys
  for (tmp_net = nets->next;
      tmp_net != nets;
      tmp_net = tmp_net->next)
  {
      if (Address_IsSameAddress(&tmp_net->A_network_addr, &net)
          && ((net.networkType == NETWORK_IPV6
               && tmp_net->A_netmask.v6 == mask.v6)
             || (net.networkType == NETWORK_IPV4
                 && tmp_net->A_netmask.v4 == mask.v4)))
      {

          return tmp_net;
      }
  }

  // Not found
  return NULL;
}

// /**
// FUNCTION   :: OlsrLookupHnaGateway
// LAYER      :: APPLICATION
// PURPOSE    :: Looks up HNA table for a given a gateway entry
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + gw   : Address : The gateway address
// RETURN :: hna_entry* : pointer to the entry or NULL if not found
// **/
hna_entry* OlsrLookupHnaGateway(
    Node* node,
    Address gw)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    hna_entry* tmp_hna;
    hna_entry* hash_hna_entry;
    UInt32 hash;

    OlsrHashing(gw, &hash);
    hash_hna_entry = &olsr->hna_table[hash % HASHMASK];

    // Check for registered entry
    for (tmp_hna = hash_hna_entry->next;
        tmp_hna != hash_hna_entry;
        tmp_hna = tmp_hna->next)
    {
        if (Address_IsSameAddress(&tmp_hna->A_gateway_addr, &gw))
        {
            return tmp_hna;
        }
    }

    return NULL;
}

// /**
// FUNCTION   :: OlsrAddHnaEntry
// LAYER      :: APPLICATION
// PURPOSE    :: adds gateway to HNA table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + gw   : Address : The gateway address
// RETURN :: hna_entry* : pointer to the new entry
// **/
hna_entry* OlsrAddHnaEntry(
    Node* node,
    Address addr)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    hna_entry* new_entry;
    UInt32 hash;

    new_entry = (hna_entry*)MEM_malloc(sizeof(hna_entry));
    memset(new_entry, 0, sizeof(hna_entry));

    new_entry->A_gateway_addr = addr;

    // Link nets
    new_entry->networks.next = &new_entry->networks;
    new_entry->networks.prev = &new_entry->networks;

    // queue

    OlsrHashing(addr, &hash);
    hna_entry* hash_hna_entry = &olsr->hna_table[hash % HASHMASK];
    OlsrInsertList((olsr_qelem* )new_entry, (olsr_qelem* )hash_hna_entry);

    return new_entry;

}

// /**
// FUNCTION   :: OlsrAddHnaNet
// LAYER      :: APPLICATION
// PURPOSE    :: Adds a network entry to a HNA gateway
// PARAMETERS ::
// + hna_gw : hna_entry* : Pointer to the gateway to the non-olsr network
// + net  : Address : The network to add
// + mask : hna_netmask : The netmask to add
// RETURN :: hna_net* : pointer to the new entry
// **/
hna_net* OlsrAddHnaNet(
    hna_entry* hna_gw,
    Address net,
    hna_netmask mask)
{
    hna_net* new_net;

    // Add the net
    new_net = (hna_net*) MEM_malloc(sizeof(hna_net));
    memset(new_net, 0, sizeof(hna_net));

    new_net->A_network_addr = net;
    new_net->A_netmask = mask;

    // Queue
    hna_gw->networks.next->prev = new_net;
    new_net->next = hna_gw->networks.next;
    hna_gw->networks.next = new_net;
    new_net->prev = &hna_gw->networks;

    return new_net;
}

// /**
// FUNCTION   :: OlsrUpdateHnaEntry
// LAYER      :: APPLICATION
// PURPOSE    :: updates network entry in HNA table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + gw   : Address : The gateway address
// + net  : Address : The network address
// + mask : hna_netmask : The netmask
// + vtime : Folat32 : validity time
// RETURN :: void : NULL
// **/
static
void OlsrUpdateHnaEntry(
    Node* node,
    Address gw,
    Address net,
    hna_netmask mask,
    Float32 vtime)
{
    hna_entry* gw_entry;
    hna_net* net_entry;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    // FIX: Skip update operation if the network is
    // directly connected to the receiving node

    for (Int32 i = 0; i < node->numberInterfaces; i++)
    {
        Address interfaceAddr;
        Address netAddr;

        if (NetworkIpGetInterfaceType(node, i) != olsr->ip_version
            && NetworkIpGetInterfaceType(node, i)!= NETWORK_DUAL)
        {
            continue;
        }

        if (olsr->ip_version == NETWORK_IPV6)
        {
            interfaceAddr.networkType = NETWORK_IPV6;
            Ipv6GetGlobalAggrAddress(
                    node,
                    i,
                    &interfaceAddr.interfaceAddr.ipv6);
        }
        else if (olsr->ip_version == NETWORK_IPV4)
        {
            interfaceAddr.networkType = NETWORK_IPV4;
            interfaceAddr.interfaceAddr.ipv4 =
                            NetworkIpGetInterfaceAddress(node,i);
        }
//#endif

        if (olsr->ip_version == NETWORK_IPV6)
        {
            Ipv6GetPrefix(&interfaceAddr.interfaceAddr.ipv6,
                &netAddr.interfaceAddr.ipv6,
                MAX_PREFIX_LEN);
            netAddr.networkType = NETWORK_IPV6;
        }
        else
        {
            UInt32 netmask = NetworkIpGetInterfaceSubnetMask(node, i);

            if (netmask)
            {
                netAddr.interfaceAddr.ipv4 =
                                interfaceAddr.interfaceAddr.ipv4 % netmask;
            }
            else
            {
                netAddr.interfaceAddr.ipv4 =
                                            interfaceAddr.interfaceAddr.ipv4;
            }
            netAddr.networkType = NETWORK_IPV4;
        }

        if (Address_IsSameAddress(&netAddr, &net))
        {
            return;
        }
    }

    if ((gw_entry = OlsrLookupHnaGateway(node, gw)) == NULL)
    {
        // Need to add the entry
        gw_entry = OlsrAddHnaEntry(node, gw);

        if (DEBUG)
        {
            printf("Node %u : Adding HNA GW entry\n", node->nodeId);
        }
    }

    if ((net_entry = OlsrLookupHnaNet(node,
                        &gw_entry->networks, net, mask)) == NULL)
    {
        // Need to add the net
        net_entry = OlsrAddHnaNet(gw_entry, net, mask);
        olsr->changes_topology = TRUE;

        if (DEBUG)
        {
            printf("Node %u : Adding HNA entry\n", node->nodeId);
        }
    }

    // Update holdingtime
    net_entry->A_time = node->getNodeTime() + (clocktype)(vtime * SECOND);

}

// /**
// FUNCTION   :: OlsrTimeoutHnaTable
// LAYER      :: APPLICATION
// PURPOSE    :: times out all the old entries in the hna table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/
static
void OlsrTimeoutHnaTable(
    Node* node)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    unsigned char index;

    for (index=0; index < HASHSIZE; index++)
    {
        hna_entry* tmp_hna = olsr->hna_table[index].next;
        // Check all entrys
        while (tmp_hna != &olsr->hna_table[index])
        {
            // Check all networks
            hna_net* tmp_net = tmp_hna->networks.next;

            while (tmp_net != &tmp_hna->networks)
            {
                if (node->getNodeTime() >= tmp_net->A_time)
                {
                    hna_net* net_to_delete = tmp_net;
                    tmp_net = tmp_net->next;
                    OlsrRemoveList((olsr_qelem *) net_to_delete);
                    MEM_free(net_to_delete);
                }
                else
                {
                    tmp_net = tmp_net->next;
                }
            }

            // Delete gw entry if empty
            if (tmp_hna->networks.next == &tmp_hna->networks)
            {
                hna_entry* hna_to_delete = tmp_hna;
                tmp_hna = tmp_hna->next;

                // Dequeue
                OlsrRemoveList((olsr_qelem *)hna_to_delete);
                // Delete
                MEM_free(hna_to_delete);
            }
            else
            {
                tmp_hna = tmp_hna->next;
            }
        }
    }
}

// /**
// FUNCTION   :: OlsrPrintHnaTable
// LAYER      :: APPLICATION
// PURPOSE    :: print all the entries in the hna table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/
static
void OlsrPrintHnaTable(Node* node)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    unsigned char index;
    char addrString[MAX_STRING_LENGTH];

    printf("HNA Table for Node %u\n",node->nodeId);
    if (olsr->ip_version == NETWORK_IPV6)
    {
        printf("IP net /prefixlen               GW-IP\n");
    }
    else
    {
        printf("IP net          netmask         GW-IP\n");
    }

    for (index = 0; index < HASHSIZE; index++)
    {
        hna_entry* tmp_hna = olsr->hna_table[index].next;
        // Check all entrys
        while (tmp_hna != &olsr->hna_table[index])
        {
            // Check all networks
            hna_net* tmp_net = tmp_hna->networks.next;

            while (tmp_net != &tmp_hna->networks)
            {
                IO_ConvertIpAddressToString(&tmp_net->A_network_addr,
                    addrString);

                if (olsr->ip_version == NETWORK_IPV6)
                {
                    printf("%-27s", addrString);
                    printf("/%d", tmp_net->A_netmask.v6);

                    IO_ConvertIpAddressToString(&tmp_hna->A_gateway_addr,
                        addrString);
                    printf(" %s\n", addrString);
                }
                else
                {
                    printf("%-15s ", addrString);
                    IO_ConvertIpAddressToString(tmp_net->A_netmask.v4,
                        addrString);
                    printf("%-15s ", addrString);
                    IO_ConvertIpAddressToString(&(tmp_hna->A_gateway_addr),
                        addrString);
                    printf("%-15s\n", addrString);
                }

                tmp_net = tmp_net->next;
            }
            tmp_hna = tmp_hna->next;
        }
    }
}

//*************************************************************************//
//                    Link Sensing Table Handling
//*************************************************************************//

// /**
// FUNCTION   :: OlsrLookupLinkStatus
// LAYER      :: APPLICATION
// PURPOSE    :: Looks up status of entry based on the timer entries
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + entry : link_entry* : Pointer to link set entry
// RETURN :: Int32 : status of link
// **/

static
Int32 OlsrLookupLinkStatus(
    Node* node,
    link_entry* entry)
{

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    if (entry == NULL || olsr->link_set == NULL)
    {
        return UNSPEC_LINK;
    }

    clocktype curr_time = node->getNodeTime();

    // RFC 3626 Section 6.2 Condition 1.1
    if (curr_time < entry->sym_timer)
    {
        return SYM_LINK;
    }

    // RFC 3626 Section 6.2 Condition 1.2
    if (curr_time < entry->asym_timer)
    {
        return ASYM_LINK;
    }

    // RFC 3626 Section 6.2 Condition 1.3
    return LOST_LINK;
}


// /**
// FUNCTION   :: OlsrLookupLinkEntry
// LAYER      :: APPLICATION
// PURPOSE    :: Looks up link set entry based on local and
//               neighbor interface address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + remote : Address : neighbor address
// + local : Address : local interface address
// RETURN :: link_entry* : pointer to the link entry
//                         if found, else NULL
// **/

static
link_entry* OlsrLookupLinkEntry(
    Node* node,
    Address remote,
    Address local)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    link_entry* tmp_link_set;

    tmp_link_set = olsr->link_set;

    while (tmp_link_set)
    {
        if ((Address_IsSameAddress(&remote,
                &tmp_link_set->neighbor_iface_addr)) &&
            (Address_IsSameAddress(&local,
                &tmp_link_set->local_iface_addr)))
        {
            return tmp_link_set;
        }

        tmp_link_set = tmp_link_set->next;
    }

    return NULL;
}

// /**
// FUNCTION   :: OlsrPrintLinkSet
// LAYER      :: APPLICATION
// PURPOSE    :: Prints link set neighbors
//               and the local iface address they
//               are attached to
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrPrintLinkSet(
    Node* node)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    link_entry* tmp_link_set;

    tmp_link_set = olsr->link_set;

    printf(" Link Set at Node %d\n", node->nodeId);

    char addrString1[MAX_STRING_LENGTH];
    char addrString2[MAX_STRING_LENGTH];

    while (tmp_link_set)
    {
        IO_ConvertIpAddressToString(&tmp_link_set->neighbor_iface_addr,
                                        addrString1);
        IO_ConvertIpAddressToString(&tmp_link_set->local_iface_addr,
                                        addrString2);
        printf(" Neighbor iface addr : %s  Local Iface Addr: %s \n",
            addrString1, addrString2);
        tmp_link_set = tmp_link_set->next;
    }

}


// /**
// FUNCTION   :: OlsrGetNeighborStatus
// LAYER      :: APPLICATION
// PURPOSE    :: Looks up neighbor address in link set and
//               returns the link status
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + address : Address : IP Address of neighbor
// RETURN :: Int32 : status of link
// **/
static
Int32 OlsrGetNeighborStatus(
    Node* node,
    Address address)
{
    Address main_addr;

    unsigned char index;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    main_addr = OlsrMidLookupMainAddr(node, address);
    if (Address_IsAnyAddress(&main_addr))
    {
        main_addr = address;
    }

    for (index = 0; index < node->numberInterfaces; index++)
    {
        mid_address* aliases;
        link_entry* link;
        Address ip_addr;
        ip_addr.networkType = NETWORK_IPV4;

        if (NetworkIpGetInterfaceType(node, index) != olsr->ip_version
            && NetworkIpGetInterfaceType(node, index)!= NETWORK_DUAL)
        {
            continue;
        }

        if (olsr->ip_version == NETWORK_IPV6)
        {
            ip_addr.networkType = NETWORK_IPV6;
            Ipv6GetGlobalAggrAddress(
                    node,
                    index,
                    &ip_addr.interfaceAddr.ipv6);
        }
        else if (olsr->ip_version == NETWORK_IPV4)
        {
            ip_addr.networkType = NETWORK_IPV4;
            ip_addr.interfaceAddr.ipv4 =
                            NetworkIpGetInterfaceAddress(node,index);
        }
//#endif

        if ((link = OlsrLookupLinkEntry(node, main_addr, ip_addr))
             != NULL)
        {
            if (OlsrLookupLinkStatus(node, link) == SYM_LINK)
            {
                return SYM_LINK;
            }
        }

        for (aliases = OlsrLookupMidAliases(node, main_addr);
                  aliases != NULL; aliases = aliases->next_alias)
        {
            if ((link = OlsrLookupLinkEntry(node, aliases->alias, ip_addr))
                 != NULL)
            {
                if (OlsrLookupLinkStatus(node,link) == SYM_LINK)
                {
                    return SYM_LINK;
                }
            }
        }
    }

    return 0;
}


// /**
// FUNCTION   :: OlsrUpdateNeighborStatus
// LAYER      :: APPLICATION
// PURPOSE    :: Updates neighbor status in the neighbor table entry depending
//               on the link status passed
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + entry : neighbor_entry * : Pointer to neighbor entry
// + link  : Int32 :  link status
// RETURN :: Int32 : status of neighbor set
// **/
static
Int32 OlsrUpdateNeighborStatus(
    Node* node,
    neighbor_entry* entry,
    Int32 link)
{

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    // Update neighbor entry
    if (link == SYM_LINK)
    {
        // N_status is set to SYM
        if (entry->neighbor_status == NOT_SYM)
        {
            neighbor_2_entry *two_hop_neighbor;

            // Delete posible 2 hop entry on this neighbor

            if ((two_hop_neighbor = OlsrLookup2HopNeighborTable(node,
                    entry->neighbor_main_addr)) != NULL)
            {
                OlsrDelete2HopNeighborTable(two_hop_neighbor);
            }

            olsr->changes_neighborhood = true;
            olsr->changes_topology = true;
        }
        entry->neighbor_status = SYM;
    }
    else
    {
        if (entry->neighbor_status == SYM)
        {
            olsr->changes_neighborhood = true;
            olsr->changes_topology = true;
        }
        // else N_status is set to NOT_SYM

        if (DEBUG)
        {
            printf(" Node %d : Setting status of neighbor as NOT SYM\n",
                node->nodeId);
        }

        entry->neighbor_status = NOT_SYM;
        // remove neighbor from routing list
    }

    return entry->neighbor_status;
}



// /**
// FUNCTION   :: OlsrAddNewLinkEntry
// LAYER      :: APPLICATION
// PURPOSE    :: Searches for an entry in link set
//               If not found, adds a new link entry
//               to the link set
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + local : Address : local interface IP Address
// + remote : Address : Neighbor interface IP Address
// + remote_main : Address : Neighbor Main IP Address
// + vtime : Float32 : validity time of entry
// + htime : Float32 : hello interval of the neighbor node
// RETURN :: link_entry* : Pointer to the found/new added entry
// **/

static
link_entry* OlsrAddNewLinkEntry(
    Node* node,
    Address local,
    Address remote,
    Address remote_main,
    Float32 vtime)
{
    link_entry *tmp_link_set, *new_link;
    neighbor_entry *neighbor;

   if (DEBUG)
   {
       printf(" In adding new entry function\n");
   }

   RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

   if (DEBUG)
   {
       if (olsr->link_set == NULL)
       {
           printf(" Node %d: Link set is NULL\n", node->nodeId);
       }
   }

   tmp_link_set = olsr->link_set;

   while (tmp_link_set)
   {
       if ((Address_IsSameAddress(&remote,
                &tmp_link_set->neighbor_iface_addr)) &&
           (Address_IsSameAddress(&local,
                &tmp_link_set->local_iface_addr)))
       {
           return tmp_link_set;
       }

       tmp_link_set = tmp_link_set->next;
    }

   if (DEBUG)
   {
       printf(" Node %d: Adding New entry in the link set\n",node->nodeId);
   }

   // RFC 3626 Section 7.1.1 Condition 1

   // if there exists no link tuple with
   // L_neighbor_iface_addr == Source Address

   // a new tuple is created with..

   new_link = (link_entry *)MEM_malloc(sizeof(link_entry));

   memset(new_link, 0 , sizeof(link_entry));

   // L_local_iface_addr = Address of the interface
   // which received the HELLO message

   new_link->local_iface_addr = local;

   // L_neighbor_iface_addr = Source Address
   new_link->neighbor_iface_addr = remote;

   // L_SYM_time = current time - 1 (expired)

   new_link->sym_timer = node->getNodeTime() - 1;

   // L_time = current time + validity time
   new_link->timer = node->getNodeTime() + (clocktype) (vtime * SECOND);

   new_link->prev_status = ASYM_LINK;

   // Add to queue
   new_link->next = olsr->link_set;
   olsr->link_set = new_link;

   // Create the neighbor entry

   // Neighbor MUST exist!
   neighbor = OlsrLookupNeighborTable(node, remote_main);
   if (!(neighbor))
   {
       neighbor = (neighbor_entry *) MEM_malloc(sizeof(neighbor_entry));
       memset(neighbor, 0, sizeof(neighbor_entry));
       neighbor->neighbor_main_addr = remote_main;
       neighbor->neighbor_willingness = WILL_NEVER;
       neighbor->neighbor_status = NOT_SYM;
       neighbor->neighbor_linkcount = 0;
       neighbor->neighbor_is_mpr = FALSE;
       neighbor->neighbor_was_mpr = FALSE;
       neighbor->neighbor_2_list = NULL;
       neighbor->neighbor_2_nocov = 0;
       OlsrInsertNeighborTable(node, neighbor);
   }

   // Copy the main address - make sure this is done every time
   // as neighbors might change main address

   neighbor->neighbor_main_addr = remote_main;

   neighbor->neighbor_linkcount++;

   new_link->neighbor = neighbor;

   if (!Address_IsSameAddress(&remote, &remote_main))
   {
       // Add MID alias if not already registered
       // This is kind of sketchy... and not specified
       // in the RFC. We can only guess a vtime.
       // We'll go for one that is hopefully long
       // enough in most cases. 20 seconds

       // Need to check if alias is already present in table..this is
       // not done in olsrd

       Address main_addr = OlsrMidLookupMainAddr(node, remote);

       if (Address_IsAnyAddress(&main_addr))
       {
           OlsrInsertMidAlias(node, remote_main, remote, 20.0);
       }
    }

  return new_link;
}


// /**
// FUNCTION   :: OlsrCheckLinktoNeighbor
// LAYER      :: APPLICATION
// PURPOSE    :: Lookup status of the link corresponding to the neighbor
//               address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + int_addr: Address: interface address to look up
// RETURN :: Int32 :  link status
// **/

static
Int32 OlsrCheckLinktoNeighbor(
    Node* node,
    Address int_addr)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    link_entry*  link_set=olsr->link_set;
    link_entry*     tmp_link_set;

    tmp_link_set = link_set;

    while (tmp_link_set)
    {
        if (Address_IsSameAddress(&int_addr,
                &tmp_link_set->neighbor_iface_addr))
        {
            return OlsrLookupLinkStatus(node, tmp_link_set);
        }
        tmp_link_set = tmp_link_set->next;
    }
    return UNSPEC_LINK;
}

// /**
// FUNCTION   :: OlsrGetLinktoNeighbor
// LAYER      :: APPLICATION
// PURPOSE    ::  Function returns a link where neighbor interface
//                address matches remote address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + remote: Address: Neighbor interface IP Address
// RETURN :: link_entry* : Pointer to the found entry
// **/

static
link_entry* OlsrGetLinktoNeighbor(
    Node* node,
    Address remote)
{
    link_entry* walker;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    link_entry* link_set = olsr->link_set;

    for (walker = link_set; walker != NULL; walker = walker->next)
    {
        // Perfect match..
        if (Address_IsSameAddress(&walker->neighbor_iface_addr, &remote))
        {
            return walker;
        }
    }

    return NULL;
}


// /**
// FUNCTION   :: OlsrGetBestLinktoNeighbor
// LAYER      :: APPLICATION
// PURPOSE    ::  Function returns a link where neighbor interface
//                address matches alias address/remote address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + remote: Address: Neighbor interface IP Address
// RETURN :: link_entry* : Pointer to the found entry
// **/

static
link_entry* OlsrGetBestLinktoNeighbor(
    Node* node,
    Address remote)
{
    Address main_addr;
    link_entry* walker;
    link_entry* good_link;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    link_entry* link_set = olsr->link_set;

    main_addr = OlsrMidLookupMainAddr(node, remote);

    if (Address_IsAnyAddress(&main_addr))
    {
        main_addr = remote;
    }

    good_link = NULL;

    for (walker = link_set; walker != NULL; walker = walker->next)
    {
        if (!Address_IsSameAddress(&walker->neighbor->neighbor_main_addr,
                &main_addr))
        {
            continue;
        }

        // Perfect match..if the neighbor iface address for this link
        // matches the address we are looking up

        if (Address_IsSameAddress(&walker->neighbor_iface_addr, &remote))
        {
            return walker;
        }
        else
        {
            // Didn't find a match, we need to set a backup to send in
            // case  a perfect match doesn't occur

            good_link = walker;
        }

    }

    if (Address_IsSameAddress(&remote, &main_addr))
    {
        // Return any associated link tuple if main addr
        // and no perfecr match found

        return good_link;
    }
    else
    {
        // if remote is not the main address, we still need to send any
        // link if present that can get to the main address node

        if (good_link)
        {
            return good_link;
        }
        else
        {
            return NULL;
        }
    }
}


// /**
// FUNCTION   :: OlsrCheckLinkStatus
// LAYER      :: APPLICATION
// PURPOSE    :: Checks the link status to a neighbor by
//               looking in a received HELLO message.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message:  hl_message* : Pointer to the hello message
// + interfaceIndex: Int32: interface on which hello was received
// RETURN :: unsigned char : Link status in the hello packet
// **/
static
unsigned char OlsrCheckLinkStatus(
    Node* node,
    hl_message* message,
    Int32 interfaceIndex)
{
    hello_neighbor* neighbors;
    neighbors = message->neighbors;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    while (neighbors!=NULL)
    {
        Address ip_addr;


        if (olsr->ip_version == NETWORK_IPV6)
        {
            ip_addr.networkType = NETWORK_IPV6;
            Ipv6GetGlobalAggrAddress(
                    node,
                    interfaceIndex,
                    &ip_addr.interfaceAddr.ipv6);
        }
        else if (olsr->ip_version == NETWORK_IPV4)
        {
            ip_addr.networkType = NETWORK_IPV4;
            ip_addr.interfaceAddr.ipv4 =
                            NetworkIpGetInterfaceAddress(node,interfaceIndex);
        }
//#endif

        if (Address_IsSameAddress(&neighbors->address, &ip_addr))
        {
            return neighbors->link;
        }

        neighbors = neighbors->next;
    }

    if (DEBUG)
    {
        printf(" Node %d: Returning unspec link as link status \n",
            node->nodeId);
    }

    return UNSPEC_LINK;
}


// /**
// FUNCTION   :: OlsrUpdateLinkEntry
// LAYER      :: APPLICATION
// PURPOSE    :: Updates a link entry, is main entrypoint in link-sensing
//               Function is called from Hello processing fn
//               Updates an existing entry or creates a new one
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + local: Address: local interface IP Address
// + remote: Address: Neighbor interface IP Address
// + message : hl_message* : Pointer to the hello message
// + interfaceIndex: Int32: interface on which message is received
// RETURN :: link_entry* : Pointer to the updated/new added entry
// **/

static
link_entry* OlsrUpdateLinkEntry(
    Node* node,
    Address local,
    Address remote,
    hl_message* message,
    Int32 interfaceIndex)
{
    if (DEBUG)
    {
        printf (" Node %d : updating link entry for neighbor\n",
            node->nodeId);
    }

    link_entry* entry;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    // Add if not registered
    entry = OlsrAddNewLinkEntry(node,
                local, remote, message->source_addr,
                message->vtime);

    // RFC 3626 Section 7.1.1 Condition 2

   // Update ASYM_time

   // L_ASYM_time = current time + validity time
   entry->asym_timer = node->getNodeTime()
                       + (clocktype)(message->vtime * SECOND);

   entry->prev_status = OlsrCheckLinkStatus(node, message,interfaceIndex);

   switch (entry->prev_status)
   {
       case (LOST_LINK):
       {
            if (DEBUG)
            {
                printf(" Node %d : Prev status was lost link\n",
                    node->nodeId);
            }

            // L_SYM_time = current time - 1 (i.e., expired)

            entry->sym_timer = node->getNodeTime() - 1;

            break;
        }
      case(SYM_LINK):
      case(ASYM_LINK):
      {
            if (DEBUG)
            {
                printf(" Node %d : Prev status was sym/asym link\n",
                    node->nodeId);
            }
            // L_SYM_time = current time + validity time

            entry->sym_timer = node->getNodeTime()
                               + (clocktype) (message->vtime * SECOND);

            // L_time = L_SYM_time + olsr->neighb_hold_time

            entry->timer = node->getNodeTime() + olsr->neighb_hold_time;

            break;
      }

      default:
      {
            if (DEBUG)
            {
                printf(" Node %d : Prev status is undefined\n",node->nodeId);
            }
            break;
      }

  }

  // L_time = max(L_time, L_ASYM_time)
  if (entry->timer < entry->asym_timer)
  {
    entry->timer = entry->asym_timer;
  }

  // Update neighbor
  OlsrUpdateNeighborStatus(node,
    entry->neighbor, OlsrGetNeighborStatus(node, remote));

  return entry;
}


// /**
// FUNCTION   :: OlsrReplaceNeighborLinkEntry
// LAYER      :: APPLICATION
// PURPOSE    :: Searches for a neighbor entry in link set
//               If  found, replaces that entry
//               with a new entry
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + old_neighbor: neighbor_entry* : old neighbor entry
// + new_neighbor: neighbor_entry* : new neighbor entry
// RETURN :: Int32 : Number of entries replaced
// **/
static
Int32 OlsrReplaceNeighborLinkEntry(
    Node* node,
    neighbor_entry* old_neighbor,
    neighbor_entry* new_neighbor)
{
    link_entry* tmp_link_set;
    Int32 retval = 0;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    if (olsr->link_set == NULL)
    {
        return retval;
    }

    tmp_link_set = olsr->link_set;

    while (tmp_link_set)
    {

        if (tmp_link_set->neighbor == old_neighbor)
        {
            tmp_link_set->neighbor = new_neighbor;
            retval++;
        }
        tmp_link_set = tmp_link_set->next;
    }

    return retval;
}


// /**
// FUNCTION   :: OlsrTimeoutLinkEntry
// LAYER      :: APPLICATION
// PURPOSE    :: Removes expired link entries in link set
//               Also removes unreferenced neighbors from neighbor table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrTimeoutLinkEntry(
    Node* node)
{
    if (DEBUG)
    {
        printf("Node %d: Timing out Link set\n",node->nodeId);
    }

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    link_entry* tmp_link_set;
    link_entry* last_link_entry;

    if (olsr->link_set == NULL)
    {
        return;
    }

    tmp_link_set = olsr->link_set;
    last_link_entry = NULL;

    while (tmp_link_set)
    {
        if (node->getNodeTime() > tmp_link_set->timer)
        {
            if (last_link_entry != NULL)
            {
                last_link_entry->next = tmp_link_set->next;

                // RFC 3626 Section 8.1 Removal Condition

                // Delete neighbor entry
                if (tmp_link_set->neighbor->neighbor_linkcount == 1)
                {

                    // Delete MPR selector entry corresponding to this
                    // neighbor?
                    // This is missing from olsrd but present in Section 8.5

                        mpr_selector_entry* selector_entry =
                                OlsrLookupMprSelectorTable(node,
                                tmp_link_set->neighbor->neighbor_main_addr);


                    if (selector_entry != NULL)
                    {
                        mpr_selector_entry* mprs_tmp = selector_entry;

                        selector_entry = selector_entry->mpr_selector_forw;
                        OlsrDeleteMprSelectorTable(mprs_tmp);
                        olsr->mprstable.ansn++;
                    }
                    OlsrDeleteNeighborTable(tmp_link_set->neighbor);
                }
                else
                {
                    tmp_link_set->neighbor->neighbor_linkcount --;
                }

                olsr->changes_neighborhood = TRUE;
                MEM_free(tmp_link_set);
                tmp_link_set = last_link_entry;
            }
            else
            {
                olsr->link_set = tmp_link_set->next; // CHANGED

                // Delete neighbor entry
                if (tmp_link_set->neighbor->neighbor_linkcount == 1)
                {

                    // Delete MPR selector entry corresponding to this
                    // neighbor?

                    // This is missing from olsrd but present in Section 8.5
                    mpr_selector_entry* selector_entry =
                        OlsrLookupMprSelectorTable(node,
                            tmp_link_set->neighbor->neighbor_main_addr);

                    if (selector_entry != NULL)
                    {
                        mpr_selector_entry* mprs_tmp = selector_entry;

                        selector_entry = selector_entry->mpr_selector_forw;
                        OlsrDeleteMprSelectorTable(mprs_tmp);
                        olsr->mprstable.ansn++;
                    }
                    OlsrDeleteNeighborTable(tmp_link_set->neighbor);
                }
                else
                {
                    tmp_link_set->neighbor->neighbor_linkcount--;
                }

                olsr->changes_neighborhood = TRUE;
                MEM_free(tmp_link_set);
                tmp_link_set = olsr->link_set;
                continue;
            }

        }
        else if ((tmp_link_set->prev_status == SYM_LINK) &&
                 (node->getNodeTime()>tmp_link_set->sym_timer))
        {
            tmp_link_set->prev_status = (unsigned char)OlsrLookupLinkStatus(
                                                          node,tmp_link_set);
            OlsrUpdateNeighborStatus(node, tmp_link_set->neighbor,
                    OlsrGetNeighborStatus(node,
                        tmp_link_set->neighbor_iface_addr));
            olsr->changes_neighborhood = TRUE;
        }
        last_link_entry = tmp_link_set;
        tmp_link_set = tmp_link_set->next;
    }

    return;
}


/***************************************************************************
 *                    Definition of Duplicate Table
 ***************************************************************************/

// /**
// FUNCTION   :: OlsrDeleteDuplicateTable
// LAYER      :: APPLICATION
// PURPOSE    :: Deletes an entry from duplicate table
// PARAMETERS ::
// + dup_entry : duplicate_entry* : Pointer to duplicate entry
// RETURN :: void : NULL
// **/

static
void OlsrDeleteDuplicateTable(
    duplicate_entry* dup_entry)
{
    duplicate_ifaces *tmp_iface, *del_iface;
    tmp_iface = dup_entry->dup_ifaces;

    //Free Interfaces
    while (tmp_iface)
    {
        del_iface = tmp_iface;
        tmp_iface = tmp_iface->duplicate_iface_next;
        MEM_free(del_iface);
    }

    OlsrRemoveList((olsr_qelem *)dup_entry);
    MEM_free((void *)dup_entry);
}

// /**
// FUNCTION   :: OlsrInsertDuplicateTable
// LAYER      :: APPLICATION
// PURPOSE    :: Inserts an entry in duplicate table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + originator : Address:  address of node to be inserted
// + packet_seq_number: UInt16: sequence number to be inserted
// RETURN :: duplicate_entry * : Pointer to entry inserted
// **/

static
duplicate_entry* OlsrInsertDuplicateTable(
    Node* node,
    Address originator,
    UInt16 packet_seq_number)
{
    UInt32 hash;
    duplicatehash* dup_hash;
    duplicate_entry* dup_message;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    dup_message = (duplicate_entry *) MEM_malloc(sizeof(duplicate_entry));
    memset(dup_message, 0, sizeof(duplicate_entry));

    // create a duplicate tuple
    dup_message->duplicate_seq = packet_seq_number;
    dup_message->duplicate_addr = originator;
    dup_message->duplicate_timer = node->getNodeTime() +
            olsr->dup_hold_time;

    // New RFC version variables
    dup_message->duplicate_retransmitted = 0;
    dup_message->dup_ifaces = NULL;

    OlsrHashing(dup_message->duplicate_addr, &hash);
    dup_message->duplicate_hash = hash;
    dup_hash = &olsr->duplicatetable[hash % HASHMASK];

    // insert in the list
    OlsrInsertList((olsr_qelem *)dup_message, (olsr_qelem *)dup_hash);
    return dup_message;
}

// /**
// FUNCTION   :: OlsrLookupDuplicateTableProc
// LAYER      :: APPLICATION
// PURPOSE    :: Search in duplicate table  if  originator and seq no are
//               processed
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + originator : Address : originator of the message
// + packet_seq_number : UInt16 : sequence number of message
// RETURN :: duplicate_entry * : Pointer to found entry else NULL
// **/

static
duplicate_entry* OlsrLookupDuplicateTableProc(
    Node *node,
    Address originator,
    UInt16 packet_seq_number)
{
    duplicate_entry* dup_message;
    duplicatehash* dup_hash;
    UInt32 hash;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrHashing(originator, &hash);

    dup_hash = &olsr->duplicatetable[hash % HASHMASK];

    // search in the duplicate table
    for (dup_message = dup_hash->duplicate_forw;
        dup_message != (duplicate_entry *) dup_hash;
        dup_message = dup_message->duplicate_forw)
    {
        if (dup_message->duplicate_hash != hash)
        {
            continue;
        }

        // RFC 3626 Section 3.4 Condition 3

        // if originator address in table and sequence number matches
        if (Address_IsSameAddress(&dup_message->duplicate_addr, &originator)
            && (dup_message->duplicate_seq == packet_seq_number))
        {
            if (DEBUG)
            {
                printf("message is in the duplicate table seqNo. is %d\n",
                        dup_message->duplicate_seq);
            }

            // entry found
            return (dup_message);
        }
    }
    // entry not found
    return (NULL);
}


// /**
// FUNCTION   :: OlsrLookupDuplicateTableFwd
// LAYER      :: APPLICATION
// PURPOSE    :: Search in duplicate table  if originator and seq no exist
//               in fwd list
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + originator: Address: originator node
// + packet_seq_number : UInt16 : sequence no
// RETURN :: Int32 : 0 if already fwded, 1 if not
// **/

static
Int32 OlsrLookupDuplicateTableFwd(
    Node *node,
    Address originator,
    UInt16 packet_seq_number,
    Address interfaceAddress)
{
    duplicate_entry* dup_message;
    duplicatehash* dup_hash;
    UInt32 hash;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrHashing(originator, &hash);

    dup_hash = &olsr->duplicatetable[hash % HASHMASK];
    // search in the duplicate table
    for (dup_message = dup_hash->duplicate_forw;
        dup_message != (duplicate_entry *) dup_hash;
        dup_message = dup_message->duplicate_forw)
    {
        if (dup_message->duplicate_hash != hash)
        {
            continue;
        }

        // RFC 3626 Section 3.4 Condition  4, Section 3.4.1

        // if originator address in table and sequence number matches
        if (Address_IsSameAddress(&dup_message->duplicate_addr, &originator)
            && (dup_message->duplicate_seq == packet_seq_number))
        {
            // Check retransmitted
            if (dup_message->duplicate_retransmitted)
            {
                return 0;
            }

            duplicate_ifaces* dup_iface;
            dup_iface = dup_message->dup_ifaces;
            while (dup_iface)
            {
                if (Address_IsSameAddress(&dup_iface->interfaceAddress,
                        &interfaceAddress))
                {
                    return 0;
                }
                dup_iface = dup_iface->duplicate_iface_next;
            }
        }
    }
    return 1;
}


// /**
// FUNCTION   :: OlsrReleaseDuplicateTable
// LAYER      :: APPLICATION
// PURPOSE    :: Release all the entries from duplicate table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrReleaseDuplicateTable(
    Node* node)
{
    Int32 index;
    duplicatehash* dup_hash;
    duplicate_entry* dup_message;
    duplicate_entry* dup_message_tmp;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    // get each entry from the table and delete it
    for (index = 0; index < HASHSIZE; index++)
    {
        dup_hash = &olsr->duplicatetable[index];
        dup_message = dup_hash->duplicate_forw;
        while (dup_message != (duplicate_entry *) dup_hash)
        {
            dup_message_tmp = dup_message;
            dup_message = dup_message->duplicate_forw;
            OlsrDeleteDuplicateTable(dup_message_tmp);
        }
    }
}


// /**
// FUNCTION   :: OlsrTimeoutDuplicateTable
// LAYER      :: APPLICATION
// PURPOSE    :: This function will be called when olsr->dup_hold_time
//               R times out. Check the entries with the current time
//               and delete if hold timer is less than current time.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrTimeoutDuplicateTable(
     Node* node)
{
    Int32 index;
    duplicatehash* dup_hash;
    duplicate_entry* dup_message;
    duplicate_entry* dup_message_tmp;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    for (index = 0; index < HASHSIZE; index++)
    {
        dup_hash = &olsr->duplicatetable[index];
        dup_message = dup_hash->duplicate_forw;
        while (dup_message != (duplicate_entry *) dup_hash)
        {
            // if current time exceeds duplicate hold time then delete it
            if (node->getNodeTime() >= dup_message->duplicate_timer)
            {
                dup_message_tmp = dup_message;
                dup_message = dup_message->duplicate_forw;
                OlsrDeleteDuplicateTable(dup_message_tmp);
            }
            else
            {
                dup_message = dup_message->duplicate_forw;
            }
        }
    }
}

// /**
// FUNCTION   :: OlsrUpdateDuplicateEntry
// LAYER      :: APPLICATION
// PURPOSE    :: Searches for an entry in duplicate table
//               if not found, adds a new entry
//               Then updates the status of the found/newly added entry
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + originator : Address : originator of the msg
// + packet_seq_number : UInt16 : sequence no of mesg
// + interfaceAddress : Address: Address of the interface
// RETURN :: Int32 : 1 to indicate successful updation
// **/

static
Int32 OlsrUpdateDuplicateEntry(
    Node *node,
    Address originator,
    UInt16 packet_seq_number,
    Address interfaceAddress)
{
    duplicate_entry* dup_message;
    duplicatehash* dup_hash;
    UInt32 hash;
    duplicate_ifaces* new_iface;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrHashing(originator, &hash);

    dup_hash = &olsr->duplicatetable[hash % HASHMASK];
    // search in the duplicate table
    for (dup_message = dup_hash->duplicate_forw;
        dup_message != (duplicate_entry *) dup_hash;
        dup_message = dup_message->duplicate_forw)
    {

        if (dup_message->duplicate_hash != hash)
        {
            continue;
        }

        // if originator address in table and sequence number matches
        if (Address_IsSameAddress(&dup_message->duplicate_addr, &originator)
            && (dup_message->duplicate_seq == packet_seq_number))
        {
            break;
        }
    }

    if (dup_message == (duplicate_entry *)dup_hash)
    {
        // Did not find entry - create it
        dup_message = OlsrInsertDuplicateTable(node,
                          originator, packet_seq_number);
    }

    // RFC 3626 Section 3.4.1  Condition 5

    // 0 for now
    dup_message->duplicate_retransmitted = 0;
    new_iface = (duplicate_ifaces *)MEM_malloc(sizeof(duplicate_ifaces));
    memset(new_iface, 0, sizeof(duplicate_ifaces));
    new_iface->interfaceAddress = interfaceAddress;
    new_iface->duplicate_iface_next = dup_message->dup_ifaces;
    dup_message->dup_ifaces = new_iface;

    // Set timer
    dup_message->duplicate_timer = node->getNodeTime() +
                                       olsr->dup_hold_time;

    return 1;
}


// /**
// FUNCTION   :: OlsrSetDuplicateForward
// LAYER      :: APPLICATION
// PURPOSE    :: Searches for an entry in table and sets the retransmitted
//               flag for the message entry
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + originator : Address : originator of message
// + packet_seq_number: UInt16: sequence number of the message
// RETURN :: Int32 : 0 if not found, 1 if found and flag set
//
// **/

static
Int32 OlsrSetDuplicateForward(
    Node *node,
    Address originator,
    UInt16 packet_seq_number)
{
    duplicate_entry* dup_message;
    duplicatehash* dup_hash;
    UInt32 hash;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrHashing(originator, &hash);

    dup_hash = &olsr->duplicatetable[hash % HASHMASK];
    // search in the duplicate table
    for (dup_message = dup_hash->duplicate_forw;
        dup_message != (duplicate_entry *) dup_hash;
        dup_message = dup_message->duplicate_forw)
    {
        if (dup_message->duplicate_hash != hash)
        {
            continue;
        }

        // if originator address in table and sequence number matches
        if (Address_IsSameAddress(&dup_message->duplicate_addr, &originator)
            && (dup_message->duplicate_seq == packet_seq_number))
        {
            break;
        }
    }

    if (dup_message == (duplicate_entry *)dup_hash)
    {
        return 0;
    }

    // RFC 3626 Section 3.4.1 Condition 5

    // Set forwarded
    dup_message->duplicate_retransmitted = 1;

    // Set timer
    dup_message->duplicate_timer = node->getNodeTime()
                                       + olsr->dup_hold_time;

    return 1;
}

// /**
// FUNCTION   :: OlsrPrintDuplicateTable
// LAYER      :: APPLICATION
// PURPOSE    :: Print contents of Duplicate table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/
static
void OlsrPrintDuplicateTable(
    Node* node)
{
    Int32 index;
    duplicatehash* dup_hash;
    duplicate_entry* dup_message;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    printf("Duplicate Table\n");
    printf("Hash       Address         Duplicate Seq No.\n");

    for (index = 0; index < HASHSIZE; index++)
    {
        dup_hash = &olsr->duplicatetable[index];

        for (dup_message = dup_hash->duplicate_forw;
            dup_message != (duplicate_entry *) dup_hash;
            dup_message = dup_message->duplicate_forw)
        {
            char addrString[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(&dup_message->duplicate_addr,
                                        addrString);
            printf("%-10d %-15s %d\n",
                dup_message->duplicate_hash,
                addrString, dup_message->duplicate_seq);
        }
    }
}

/***************************************************************************
 *                 Neighbor Table related functions                        *
 ***************************************************************************/

// /**
// FUNCTION   :: OlsrDeleteNeighbor2Pointer
// LAYER      :: APPLICATION
// PURPOSE    :: Deletes current address from the 2 hop neighbor list of this
//               neighbor entry
// PARAMETERS ::
// + neighbor : neighbor_entry* : Pointer to neighbor entry
// + address : Address : Address to be deleted from two hop neighbor list
// RETURN :: void : NULL
// **/

void OlsrDeleteNeighbor2Pointer(
    neighbor_entry* neighbor,
    Address address)
{
    neighbor_2_list_entry* neighbor_two_list;
    neighbor_2_list_entry* neighbor_two_list_tmp;

    ERROR_Assert(neighbor, "Invalid neighbor entry");

    // get 2 hop list for current enighbor entry
    neighbor_two_list = neighbor->neighbor_2_list;

    if (Address_IsSameAddress(
                    &neighbor_two_list->neighbor_2->neighbor_2_addr,
                    &address))
    {
        // 2 hop matches going to delete it
        neighbor->neighbor_2_list = neighbor_two_list->neighbor_2_next;
        MEM_free(neighbor_two_list);
        return;
    }

    neighbor_two_list_tmp = neighbor_two_list;
    neighbor_two_list = neighbor_two_list->neighbor_2_next;

    while (neighbor_two_list != NULL)
    {
        if (Address_IsSameAddress(
                            &neighbor_two_list->neighbor_2->neighbor_2_addr,
                            &address))
        {
            // 2 hop matches going to delete it
            neighbor_two_list_tmp->neighbor_2_next =
                                neighbor_two_list->neighbor_2_next;
            MEM_free(neighbor_two_list);
            return;
        }
        else
        {
            // searches for the next in the 2 hop neighbor table
            neighbor_two_list_tmp = neighbor_two_list;
            neighbor_two_list = neighbor_two_list->neighbor_2_next;
        }
    }
}

// /**
// FUNCTION   :: OlsrLookupMyNeighbors
// LAYER      :: APPLICATION
// PURPOSE    :: Searches this neighbor address in the current neighbor entry
//               neighbor entry
// PARAMETERS ::
// + neighbor : neighbor_entry* : Pointer to neighbor entry
// + neighbor_main_address : Address : Address to be searched
// RETURN :: neighbor_2_list_entry * : corresponding two hop neighbor list
//                                     entry
// **/

static
neighbor_2_list_entry* OlsrLookupMyNeighbors(
    neighbor_entry* neighbor,
    Address neighbor_main_address)
{
    neighbor_2_list_entry* neighbor_two_list;

    ERROR_Assert(neighbor, "Invalid neighbor entry");

    neighbor_two_list = neighbor->neighbor_2_list;

    while (neighbor_two_list != NULL)
    {
        if (Address_IsSameAddress(
            &neighbor_two_list->neighbor_2->neighbor_2_addr,
            &neighbor_main_address))
        {
            // address matches return 2 hop neighbor list
            return(neighbor_two_list);
        }
        neighbor_two_list = neighbor_two_list->neighbor_2_next;
    }
    return NULL;
}


// /**
// FUNCTION   :: OlsrDeleteNeighborPointer
// LAYER      :: APPLICATION
// PURPOSE    :: This procedure deletes the pointer to this addr from
//               the list contained in the two hop neighbor entry
// PARAMETERS ::
// + two_hop_entry  : neighbor_2_entry * : Pointer to two hop neighbor list
// + address : Address: Address to be removed from 2 hop list
// RETURN :: void : NULL
// **/

static
void OlsrDeleteNeighborPointer(
    neighbor_2_entry* two_hop_entry,
    Address address)
{
    neighbor_list_entry* one_hop_list;
    neighbor_list_entry* one_hop_list_tmp;

    ERROR_Assert(two_hop_entry, "Invalid two hop neighbor entry");

    // get one hop list from the 2 hop entry
    one_hop_list = two_hop_entry->neighbor_2_nblist;

    if (Address_IsSameAddress(
            &one_hop_list->neighbor->neighbor_main_addr, &address))
    {
        two_hop_entry->neighbor_2_nblist = one_hop_list->neighbor_next;
        MEM_free(one_hop_list);
        return;
    }

    one_hop_list_tmp = one_hop_list;
    one_hop_list = one_hop_list->neighbor_next;

    while (one_hop_list)
    {
        if (Address_IsSameAddress(
                &one_hop_list->neighbor->neighbor_main_addr, &address))
        {
            one_hop_list_tmp->neighbor_next = one_hop_list->neighbor_next;
            MEM_free(one_hop_list);
            return;
        }
        else
        {
            one_hop_list_tmp = one_hop_list;
            one_hop_list = one_hop_list->neighbor_next;
        }
    }
}

// /**
// FUNCTION   :: OlsrDeleteNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Deletes current neighbor entry from the neighbor table
// PARAMETERS ::
// + neighbor : neighbor_entry* : Pointer to entry to be deleted
// RETURN :: void : NULL
// **/

void OlsrDeleteNeighborTable(
    neighbor_entry* neighbor)
{
    neighbor_2_list_entry* two_hop_list;
    neighbor_2_entry*   two_hop_entry;

    ERROR_Assert(neighbor, "Invalid neighbor entry");

    // delete 2 hop neighbor table for this neighbor
    two_hop_list = neighbor->neighbor_2_list;

    while (two_hop_list != NULL)
    {
        two_hop_entry = two_hop_list->neighbor_2;
        two_hop_entry->neighbor_2_pointer--;

        OlsrDeleteNeighborPointer(two_hop_entry,
            neighbor->neighbor_main_addr);

        if (two_hop_entry->neighbor_2_pointer < 1)
        {
            // means this two hop entry is no more pointed, not reachable
            OlsrRemoveList((olsr_qelem *) two_hop_entry);
            MEM_free((void *) two_hop_entry);
        }

        neighbor->neighbor_2_list = two_hop_list->neighbor_2_next;
        MEM_free(two_hop_list);
        two_hop_list = neighbor->neighbor_2_list;
    }

    // deletes neighbor
    OlsrRemoveList((olsr_qelem *) neighbor);
    MEM_free((void *)neighbor);
}


// /**
// FUNCTION   :: OlsrInsertNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Inserts neighbor entry in the neighbor table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + neighbor: neighbor_entry * : entry to be inserted
// RETURN :: void : NULL
// **/
void OlsrInsertNeighborTable(
    Node* node,
    neighbor_entry* neighbor)
{
    UInt32 hash;
    neighborhash_type* hash_neighbor;

    RoutingOlsr* olsr = (RoutingOlsr*) node->appData.olsr;

    ERROR_Assert(neighbor, "Invalid neighbor entry");

    OlsrHashing(neighbor->neighbor_main_addr, &hash);
    neighbor->neighbor_hash = hash;

    hash_neighbor = &olsr->neighbortable.neighborhash[hash % HASHMASK];

    OlsrInsertList((olsr_qelem *)neighbor, (olsr_qelem *)hash_neighbor);

}

// /**
// FUNCTION   :: OlsrLookupNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Searches neighbor table for this address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + dst : Address: Address to look for
// RETURN :: neighbor_entry * : if found, pointer to entry
//                              else NULL
// **/
neighbor_entry* OlsrLookupNeighborTable(
    Node* node,
    Address dst)
{
    neighbor_entry* neighbor;
    neighborhash_type* hash_neighbor;
    UInt32 hash;
    Address tmp_addr = OlsrMidLookupMainAddr(node, dst);

    if (!Address_IsAnyAddress(&tmp_addr))
    {
        dst = tmp_addr;
    }

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrHashing(dst, &hash);
    hash_neighbor = &olsr->neighbortable.neighborhash[hash % HASHMASK];

    for (neighbor = hash_neighbor->neighbor_forw;
        neighbor != (neighbor_entry *) hash_neighbor;
        neighbor = neighbor->neighbor_forw)
    {
        if (neighbor->neighbor_hash != hash)
        {
            continue;
        }

        // address matches with this neighbor entry return it
        if (Address_IsSameAddress(&neighbor->neighbor_main_addr, &dst))
        {
            return neighbor;
        }
    }
    return NULL;
}

// /**
// FUNCTION   :: OlsrReleaseNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Releases neighbor table for this node
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/
static
void OlsrReleaseNeighborTable(
    Node* node)
{
    unsigned char index;
    neighbor_entry* neighbor;
    neighborhash_type* hash_neighbor;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    for (index = 0; index < HASHSIZE; index++)
    {
        hash_neighbor = &olsr->neighbortable.neighborhash[index];

        for (neighbor = hash_neighbor->neighbor_forw;
            neighbor != (neighbor_entry *) hash_neighbor;
            neighbor = neighbor->neighbor_forw)
        {
            // deletes each neighbor entry
            OlsrDeleteNeighborTable(neighbor);
        }
    }
}

// /**
// FUNCTION   :: OlsrTimeout2HopNeighbors
// LAYER      :: APPLICATION
// PURPOSE    :: This function will be called when NEIGHBOR_HOLD_TIMER
//               expires. Removes two hop neighbors for the  entry for
//               which timer expires
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + neighbor  : neighbor_entry * : Pointer to expired entry
// RETURN :: void : NULL
// **/

static
void OlsrTimeout2HopNeighbors(
    Node* node,
    neighbor_entry* neighbor)
{
    neighbor_2_list_entry* two_hop_list;
    neighbor_2_list_entry* two_hop_list_tmp;
    neighbor_2_entry* two_hop_entry;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    ERROR_Assert(neighbor, "Invalid neighbor entry");

    two_hop_list = neighbor->neighbor_2_list;
    neighbor->neighbor_2_list = NULL;

    while (two_hop_list != NULL)
    {
        if (node->getNodeTime() >= two_hop_list->neighbor_2_timer)
        {
            // expires for current entry
            two_hop_entry = two_hop_list->neighbor_2;
            two_hop_entry->neighbor_2_pointer--;

            OlsrDeleteNeighborPointer(two_hop_entry,
                    neighbor->neighbor_main_addr);

            if (two_hop_entry->neighbor_2_pointer < 1)
            {
                // means this two hop entry is no more pointed, not reachable
                OlsrRemoveList((olsr_qelem *) two_hop_entry);
                MEM_free((void *)two_hop_entry);
            }
            two_hop_list_tmp = two_hop_list;
            two_hop_list = two_hop_list->neighbor_2_next;
            MEM_free(two_hop_list_tmp);

            // This flag is set to TRYE to recalculate the MPR set and the
            // routing table

            olsr->changes_neighborhood = TRUE;
            olsr->changes_topology = TRUE;
        }
        else
        {
            // not expired, still should be in the list
            two_hop_list_tmp = two_hop_list;
            two_hop_list = two_hop_list->neighbor_2_next;
            two_hop_list_tmp->neighbor_2_next = neighbor->neighbor_2_list;
            neighbor->neighbor_2_list = two_hop_list_tmp;
        }
    }
}

// /**
// FUNCTION   :: OlsrTimeoutNeighborhoodTables
// LAYER      :: APPLICATION
// PURPOSE    :: This function will be called when NEIGHBOR_HOLD_TIMER
//               expires. Removes that entry for which timer expires
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrTimeoutNeighborhoodTables(
    Node* node)
{
    unsigned char index;
    neighbor_entry* neighbor;
    neighborhash_type* hash_neighbor;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    for (index = 0; index < HASHSIZE; index++)
    {
        hash_neighbor = &olsr->neighbortable.neighborhash[index];
        neighbor = hash_neighbor->neighbor_forw;

        while (neighbor != (neighbor_entry *) hash_neighbor)
        {
            OlsrTimeout2HopNeighbors(node, neighbor);
            neighbor = neighbor->neighbor_forw;
        }

    }
}

// /**
// FUNCTION   :: OlsrPrintNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Prints neighbor table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrPrintNeighborTable(
    Node* node)
{
    int index;
    neighbor_entry* neighbor;
    neighborhash_type* hash_neighbor;
    neighbor_2_list_entry* list_2;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    printf("Neighbor List: \n");

    for (index = 0; index < HASHSIZE; index++)
    {
        hash_neighbor = &olsr->neighbortable.neighborhash[index];

        for (neighbor = hash_neighbor->neighbor_forw;
            neighbor != (neighbor_entry *) hash_neighbor;
            neighbor = neighbor->neighbor_forw)
        {
            char addrString[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(&neighbor->neighbor_main_addr,
                                        addrString);
            printf("%-15s %3d [2 hop neighbor: ",
                    addrString,
                    neighbor->neighbor_status);

            list_2 = neighbor->neighbor_2_list;
            while (list_2 != NULL)
            {
                IO_ConvertIpAddressToString(
                    &list_2->neighbor_2->neighbor_2_addr,
                    addrString);

                printf("%s ",addrString);
                list_2 = list_2->neighbor_2_next;
            }
            printf("]\n");
        }
    }
}

// /**
// FUNCTION   :: OlsrDelete2HopNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Delete a two hop neighbor entry results the deletion of its 1
//              hop neighbors list !! We don't try to remove the one hop
//              neighbor even if it has no more 2 hop neighbors !!
// PARAMETERS ::
// + two_hop_neighbor : neighbor_2_entry* : Pointer to two hop neighbor entry
// RETURN :: void : NULL
// **/

void OlsrDelete2HopNeighborTable(
    neighbor_2_entry* two_hop_neighbor)
{
    neighbor_list_entry* one_hop_list;
    neighbor_entry* one_hop_entry;

    ERROR_Assert(two_hop_neighbor, "Invalid two hop neighbor entry");

    // first delete one hop list of this 2 hop neighbor
    one_hop_list = two_hop_neighbor->neighbor_2_nblist;

    while (one_hop_list != NULL)
    {
        one_hop_entry = one_hop_list->neighbor;

        OlsrDeleteNeighbor2Pointer(one_hop_entry,
                two_hop_neighbor->neighbor_2_addr);

        two_hop_neighbor->neighbor_2_nblist = one_hop_list->neighbor_next;
        MEM_free(one_hop_list);
        one_hop_list = two_hop_neighbor->neighbor_2_nblist;
    }

    // deletes 2 hop neighbor
    OlsrRemoveList((olsr_qelem *) two_hop_neighbor);
    MEM_free((void *) two_hop_neighbor);
}


// /**
// FUNCTION   :: OlsrInsert2HopNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Inserts a two hop neighbor entry in the neighbor table
// PARAMETERS ::
// + two_hop_neighbor : neighbor_2_entry* : Pointer to two hop neighbor entry
// RETURN :: void : NULL
// **/

static
void OlsrInsert2HopNeighborTable(
    Node *node,
    neighbor_2_entry *two_hop_neighbor)
{
    UInt32 hash;
    neighbor2_hash* hash_two_neighbor;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    ERROR_Assert(two_hop_neighbor, "Invalid two hop neighbor entry");

    OlsrHashing(two_hop_neighbor->neighbor_2_addr, &hash);

    two_hop_neighbor->neighbor_2_hash = hash;
    hash_two_neighbor = &olsr->neighbor2table[hash % HASHMASK];

    OlsrInsertList((olsr_qelem *)two_hop_neighbor,
            (olsr_qelem *)hash_two_neighbor);
}

// /**
// FUNCTION   ::  OlsrLookup2HopNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Searches an address in the two hop neighbor table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + dest : Address: address to be searched
// RETURN :: neighbor_2_entry* : Pointer to entry if found, else NULL
// **/

neighbor_2_entry* OlsrLookup2HopNeighborTable(
    Node *node,
    Address dest)
{
    neighbor_2_entry* neighbor_2;
    neighbor2_hash* hash_2_neighbor;
    UInt32 hash;
    mid_address* addr;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrHashing(dest, &hash);
    hash_2_neighbor = &olsr->neighbor2table[hash % HASHMASK];

    for (neighbor_2 = hash_2_neighbor->neighbor2_forw;
        neighbor_2 != (neighbor_2_entry *) hash_2_neighbor;
        neighbor_2 = neighbor_2->neighbor_2_forw)
    {
        if (neighbor_2->neighbor_2_hash != hash)
        {
            continue;
        }

        // searches in the neighbor 2 table
        if (Address_IsSameAddress(&neighbor_2->neighbor_2_addr, &dest))
        {
            return(neighbor_2);
        }

        addr = OlsrLookupMidAliases(node, neighbor_2->neighbor_2_addr);
        while (addr)
        {
            if (Address_IsSameAddress(&addr->alias, &dest))
            {
                return neighbor_2;
            }
            addr = addr->next_alias;
        }
    }
    return NULL;
}

// /**
// FUNCTION   :: OlsrRelease2HopNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Releases the two hop neighbor table for this node
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrRelease2HopNeighborTable(
    Node* node)
{
    Int32 index;
    neighbor_2_entry* neighbor_2;
    neighbor_2_entry* neighbor_2_tmp;
    neighbor2_hash* hash_2_neighbor;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    for (index = 0; index < HASHSIZE; index++)
    {
        hash_2_neighbor = &olsr->neighbor2table[index];
        neighbor_2 = hash_2_neighbor->neighbor2_forw;

        while (neighbor_2 != (neighbor_2_entry *) hash_2_neighbor)
        {
            neighbor_2_tmp = neighbor_2;
            neighbor_2 = neighbor_2->neighbor_2_forw;
            OlsrDelete2HopNeighborTable(neighbor_2_tmp);
        }
    }
}


// /**
// FUNCTION   :: OlsrPrint2HopNeighborTable
// LAYER      :: APPLICATION
// PURPOSE    :: Prints the two hop neighbor table for this node
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrPrint2HopNeighborTable(Node *node)
{
    Int32 index;
    neighbor_2_entry* neighbor_2;
    neighbor2_hash* hash_2_neighbor;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    neighbor_list_entry* list_1 = NULL;

    printf("Two Hop Neighbors\n");

    for (index = 0; index < HASHSIZE; index++)
    {
        hash_2_neighbor = &olsr->neighbor2table[index];
        for (neighbor_2 = hash_2_neighbor->neighbor2_forw;
            neighbor_2 != (neighbor_2_entry *) hash_2_neighbor;
            neighbor_2 = neighbor_2->neighbor_2_forw)
        {
            char addrString[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(&neighbor_2->neighbor_2_addr,
                                        addrString);

            printf("%s ",addrString);
            list_1 = neighbor_2->neighbor_2_nblist;

            printf("pointed by ");
            while (list_1 != NULL)
            {
                IO_ConvertIpAddressToString(
                    &list_1->neighbor->neighbor_main_addr,
                    addrString);

                printf("%s ", addrString);
                list_1 = list_1->neighbor_next;
            }
            printf("\n");
        }
    }
}


// /**
// FUNCTION   :: OlsrClear2HopProcessed
// LAYER      :: APPLICATION
// PURPOSE    :: Function to clear processed status of 2 hop neighbors
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrClear2HopProcessed(
    Node* node)
{
   neighbor_2_entry* neighbor_2;
   neighbor2_hash* hash_2_neighbor;

   unsigned char index;
   RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

   for (index = 0; index < HASHSIZE; index++)
   {
       hash_2_neighbor = &olsr->neighbor2table[index];

       for (neighbor_2 = hash_2_neighbor->neighbor2_forw;
            neighbor_2 != (neighbor_2_entry  *) hash_2_neighbor;
            neighbor_2 = neighbor_2->neighbor_2_forw)
       {
           // Clear
           neighbor_2->processed = 0;
       }

   }
}


// /**
// FUNCTION   :: OlsrFind2HopNeighborsWith1Link
// LAYER      :: APPLICATION
// PURPOSE    :: Finds the two hop neighbor with 1 link for this node
//               with specified willingness
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + willingness: Int32: willingess value to be searched
// RETURN :: neighbor_2_list_entry : List of 2 hop neighbors satisfying
// criteria
// **/

static
neighbor_2_list_entry* OlsrFind2HopNeighborsWith1Link(
    Node* node,
    Int32 willingness)
{
    Int32 index;
    neighbor_2_list_entry* two_hop_list_tmp = NULL;
    neighbor_2_list_entry* two_hop_list = NULL;
    neighbor_2_entry* two_hop_neighbor = NULL;
    neighbor_entry* dup_neighbor;
    neighbor2_hash* hash_2_neighbor;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    for (index = 0; index < HASHSIZE; index++)
    {
        hash_2_neighbor = &olsr->neighbor2table[index];

        for (two_hop_neighbor = hash_2_neighbor->neighbor2_forw;
            two_hop_neighbor != (neighbor_2_entry  *) hash_2_neighbor;
            two_hop_neighbor = two_hop_neighbor->neighbor_2_forw)
        {
            // two_hop_neighbor->neighbor_2_state = 0;
            dup_neighbor = OlsrLookupNeighborTable(node,
                               two_hop_neighbor->neighbor_2_addr);

            if ((dup_neighbor!=NULL)
                && (dup_neighbor->neighbor_status != NOT_SYM))
            {
                continue;
            }

            if (two_hop_neighbor->neighbor_2_pointer == 1)
            {
                if ((two_hop_neighbor->neighbor_2_nblist->neighbor->
                    neighbor_status == SYM_NEIGH)
                    && (two_hop_neighbor->neighbor_2_nblist->neighbor->
                            neighbor_willingness == willingness))
                {
                    two_hop_list_tmp = (neighbor_2_list_entry *)
                            MEM_malloc(sizeof(neighbor_2_list_entry));

                    memset(
                        two_hop_list_tmp,
                        0,
                        sizeof(neighbor_2_list_entry));

                    two_hop_list_tmp->neighbor_2 = two_hop_neighbor;
                    two_hop_list_tmp->neighbor_2_next = two_hop_list;
                    two_hop_list = two_hop_list_tmp;
                }
            }
        }
    }
    // return 2 hop neighbor 2 list
    return(two_hop_list_tmp);
}

// /**
// FUNCTION   :: OlsrCalculateNeighbors
// LAYER      :: APPLICATION
// PURPOSE    :: This function calculates the number of second hop neighbors for
//               each neighbor
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + two_hop_count : UInt16* : Pointer to the counter to be filled
// RETURN :: void : NULL
// **/

static
void OlsrCalculateNeighbors(
    Node* node,
    UInt16* two_hop_count)
{
    int index;
    neighborhash_type* neighbor_hash_table;
    neighbor_entry* a_neighbor;
    UInt16 n_count = 0;
    UInt16 sum = 0;
    UInt16 count = 0;
    neighbor_entry* dup_neighbor;

    OlsrClear2HopProcessed(node);

    neighbor_2_list_entry* twohop_neighbors;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    neighbor_hash_table = olsr->neighbortable.neighborhash;

    for (index = 0; index < HASHSIZE; index++)
    {
        for (a_neighbor = neighbor_hash_table[index].neighbor_forw;
            a_neighbor != (neighbor_entry  *)&neighbor_hash_table[index];
            a_neighbor = a_neighbor->neighbor_forw)
        {
            count = 0;
            n_count = 0;

            if (a_neighbor->neighbor_status == NOT_SYM)
            {
                a_neighbor->neighbor_2_nocov = count;
                continue;
            }

            twohop_neighbors = a_neighbor->neighbor_2_list;
            while (twohop_neighbors != NULL)
            {
                dup_neighbor = OlsrLookupNeighborTable(node,
                                   twohop_neighbors->neighbor_2->
                                                 neighbor_2_addr);

                if ((dup_neighbor == NULL)
                     || (dup_neighbor->neighbor_status != SYM))
                {
                    n_count++;
                    if (!twohop_neighbors->neighbor_2->processed)
                    {
                        count++;
                        twohop_neighbors->neighbor_2->processed = 1;
                    }
                }

                twohop_neighbors = twohop_neighbors->neighbor_2_next;
            }

            a_neighbor->neighbor_2_nocov = n_count;
            sum = sum + count;
        }
    }
    *two_hop_count = (UInt16) (*two_hop_count + sum);
}

// /**
// FUNCTION   :: OlsrLinkingThis2Entries
// LAYER      :: APPLICATION
// PURPOSE    :: This function links 2 hop neighbor to neighbor entry
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + neighbor : neighbor_entry *:  Pointer to neighbor entry
// + two_hop_neighbor: neighbor_2_entry * : Pointer to 2 hop neighbor entry
// RETURN :: void : NULL
// **/

static
void OlsrLinkingThis2Entries(
    Node* node,
    neighbor_entry* neighbor,
    neighbor_2_entry* two_hop_neighbor,
    float vtime)
{
    neighbor_list_entry* list_of_1_neighbors;
    neighbor_2_list_entry* list_of_2_neighbors;

    ERROR_Assert(neighbor, "Invalid neighbor entry");
    ERROR_Assert(two_hop_neighbor, "Invalid two hop neighbor entry");

    list_of_1_neighbors = (neighbor_list_entry *)
            MEM_malloc(sizeof(neighbor_list_entry));

    memset(list_of_1_neighbors, 0, sizeof(neighbor_list_entry));

    list_of_2_neighbors = (neighbor_2_list_entry *)
            MEM_malloc(sizeof(neighbor_2_list_entry));

    memset(list_of_2_neighbors, 0, sizeof(neighbor_2_list_entry));

    list_of_1_neighbors->neighbor = neighbor;
    list_of_1_neighbors->neighbor_next = two_hop_neighbor->neighbor_2_nblist;

    two_hop_neighbor->neighbor_2_nblist = list_of_1_neighbors;

    list_of_2_neighbors->neighbor_2 = two_hop_neighbor;

    list_of_2_neighbors->neighbor_2_timer =
        node->getNodeTime() + (clocktype)(vtime * SECOND);

    list_of_2_neighbors->neighbor_2_next = neighbor->neighbor_2_list;

    neighbor->neighbor_2_list = list_of_2_neighbors;

    // increment the pointer counter
    two_hop_neighbor->neighbor_2_pointer++;
}


/***************************************************************************
 *                 Definition of MPR Selector Table                        *
 ***************************************************************************/

// /**
// FUNCTION   :: OlsrDeleteMprSelectorTable
// LAYER      :: APPLICATION
// PURPOSE    :: Deletes entry from MPR  selector table
// PARAMETERS ::
// + mprs : mpr_selector_entry *:  Pointer to mpr entry
// RETURN :: void : NULL
// **/

void OlsrDeleteMprSelectorTable(
    mpr_selector_entry* mprs)
{
    OlsrRemoveList((olsr_qelem*) mprs);
    MEM_free((void*) mprs);
}


// /**
// FUNCTION   :: OlsrClearMprs
// LAYER      :: APPLICATION
// PURPOSE    :: Clear all  neighbors' MPR status
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void  OlsrClearMprs(
    Node* node)
{
   unsigned char index;
   neighbor_entry* a_neighbor;
   neighbor_2_list_entry* two_hop_list;
   neighborhash_type* neighbor_hash_table;

   RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
   neighbor_hash_table = olsr->neighbortable.neighborhash;

   for (index = 0; index < HASHSIZE; index++)
   {
       for (a_neighbor = neighbor_hash_table[index].neighbor_forw;
               a_neighbor != (neighbor_entry  *)&neighbor_hash_table[index];
               a_neighbor = a_neighbor->neighbor_forw)
       {
           // Clear MPR selection
           if (a_neighbor->neighbor_is_mpr)
           {
               a_neighbor->neighbor_was_mpr = true;
               a_neighbor->neighbor_is_mpr = false;
           }

           // Clear two hop neighbors coverage count
           for (two_hop_list = a_neighbor->neighbor_2_list;
                   two_hop_list != NULL;
                   two_hop_list = two_hop_list->neighbor_2_next)
           {
               two_hop_list->neighbor_2->mpr_covered_count = 0;
           }
       }

   }
}

// /**
// FUNCTION   :: OlsrInsertMprSelectorTable
// LAYER      :: APPLICATION
// PURPOSE    :: Inserts entry in MPR  selector table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + mprs : mpr_selector_entry* :  Pointer to entry
// RETURN :: void : NULL
// **/

static
void OlsrInsertMprSelectorTable(
    Node* node,
    mpr_selector_entry* mprs)
{
    UInt32  hash;
    mpr_selector_hash_type* mprs_hash;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    ERROR_Assert(mprs, "Invalid MPR selector entry");

    OlsrHashing(mprs->mpr_selector_main_addr, &hash);
    mprs->mprselector_hash = hash;

    mprs_hash = &olsr->mprstable.mpr_selector_hash[hash % HASHMASK];

    // inserts new entry in the table
    OlsrInsertList((olsr_qelem *)mprs, (olsr_qelem *)mprs_hash);
}


// /**
// FUNCTION   :: OlsrLookupMprSelectorTable
// LAYER      :: APPLICATION
// PURPOSE    :: Search an address in the MPR  selector table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + dst  : Address : Address to be searched
// RETURN :: mpr_selector_entry* : Pointer to entry if found, else NULL
// **/
mpr_selector_entry* OlsrLookupMprSelectorTable(
    Node* node,
    Address dst)
{
    mpr_selector_entry* mprs;
    mpr_selector_hash_type* mprs_hash;
    UInt32 hash;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrHashing(dst, &hash);
    mprs_hash = &olsr->mprstable.mpr_selector_hash[hash % HASHMASK];

    for (mprs = mprs_hash->mpr_selector_forw;
        mprs != (mpr_selector_entry *) mprs_hash;
        mprs = mprs->mpr_selector_forw)
    {
        if (mprs->mprselector_hash != hash)
        {
            continue;
        }

        // try to match the address with the selector address from MPR entry
        if (Address_IsSameAddress(&mprs->mpr_selector_main_addr, &dst))
        {
            // match found
            return mprs;
        }
    }
    // no match return NULL
    return NULL;
}


// /**
// FUNCTION   :: OlsrUpdateMprSelectorTable
// LAYER      :: APPLICATION
// PURPOSE    :: Updates MPR selector table entry vtime
//               Adds a new entry if no entry exists
//               and sets the vtime
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + addr : Address : Address of node to be updated
// + vtime: float : validity time of entry in seconds
// RETURN :: void : NULL
// **/

static
void OlsrUpdateMprSelectorTable (
    Node* node,
    Address addr,
    float vtime)
{
    mpr_selector_entry* mprs;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    // search current source address from hello message in the MPR table
    if ((mprs = OlsrLookupMprSelectorTable(
                    node, addr)) == NULL)
    {
        // not found in the table, going to create a new one
        mprs = (mpr_selector_entry *)
            MEM_malloc(sizeof (mpr_selector_entry));

        memset(mprs, 0, sizeof(mpr_selector_entry));

        // mpr selector address set by the message source address
        mprs->mpr_selector_main_addr = addr;
        mprs->mpr_selector_timer =
            node->getNodeTime() + (clocktype)(vtime * SECOND);

        OlsrInsertMprSelectorTable(node, mprs);

        // increment mssn no. don't forget to increment this n. on timeout
        olsr->mprstable.ansn++;
    }
    else
    {
        mprs->mpr_selector_timer =
                node->getNodeTime() + (clocktype)(vtime * SECOND);
    }
}

// /**
// FUNCTION   :: OlsrReleaseMprSelectorTable
// LAYER      :: APPLICATION
// PURPOSE    :: Releases MPR selector table for this node
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrReleaseMprSelectorTable(
    Node* node)
{
    mpr_selector_entry* mprs;
    mpr_selector_entry* mprs_tmp;
    mpr_selector_hash_type* mprs_hash;
    int index;

    RoutingOlsr *olsr = (RoutingOlsr* ) node->appData.olsr;

    for (index = 0; index < HASHSIZE; index++)
    {
        mprs_hash = &olsr->mprstable.mpr_selector_hash[index];

        mprs = mprs_hash->mpr_selector_forw;

        while (mprs != (mpr_selector_entry *) mprs_hash)
        {
            // delete each MPR entry
            mprs_tmp = mprs;
            mprs = mprs->mpr_selector_forw;
            OlsrDeleteMprSelectorTable(mprs_tmp);
        }
    }
}

// /**
// FUNCTION   :: OlsrExistMprSelectorTable
// LAYER      :: APPLICATION
// PURPOSE    :: Check whether there is an entry in the MPR selctor table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: BOOL : TRUE if non-empty, else FALSE
// **/

static
BOOL OlsrExistMprSelectorTable(
    Node* node)
{
    mpr_selector_entry* mprs;
    mpr_selector_hash_type* mprs_hash;
    int index;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    for (index = 0; index < HASHSIZE; index++)
    {
        mprs_hash = &olsr->mprstable.mpr_selector_hash[index];

        mprs = mprs_hash->mpr_selector_forw;
        while (mprs != (mpr_selector_entry *)mprs_hash)
        {
            mprs = mprs->mpr_selector_forw;
        }
        if (mprs == (mpr_selector_entry *)mprs_hash)
        {
            return TRUE;
        }
    }
    return FALSE;
}

// /**
// FUNCTION   :: OlsrTimeoutMprSelectorTable
// LAYER      :: APPLICATION
// PURPOSE    :: This function will be called when NEIGHBOR_HOLD_TIMER
//               expires and removes that MPR selector entry that is
//               invalid w.r.t. current time
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrTimeoutMprSelectorTable(
    Node* node)
{
    mpr_selector_entry* mprs;
    mpr_selector_entry* mprs_tmp;
    mpr_selector_hash_type* mprs_hash;
    int index;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    for (index = 0; index < HASHSIZE; index++)
    {
        mprs_hash = &olsr->mprstable.mpr_selector_hash[index];
        mprs = mprs_hash->mpr_selector_forw;

        while (mprs != (mpr_selector_entry *) mprs_hash)
        {
            // current time is more than the valid hold time
            if (node->getNodeTime() >= mprs->mpr_selector_timer)
            {
                mprs_tmp = mprs;
                mprs = mprs->mpr_selector_forw;
                OlsrDeleteMprSelectorTable(mprs_tmp);

                // increment mssn no. don't forget to increment this number
                olsr->mprstable.ansn++;
            }
            else
            {
                mprs = mprs->mpr_selector_forw;
            }
        }
    }
}

// /**
// FUNCTION   :: OlsrPrintMprSelectorTable
// LAYER      :: APPLICATION
// PURPOSE    :: Prints MPR selector table for this node
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrPrintMprSelectorTable(
    Node* node)
{
    mpr_selector_entry* mprs;
    mpr_selector_hash_type* mprs_hash;
    int index;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    printf("Mpr Selector Table: [");
    for (index = 0; index < HASHSIZE; index++)
    {
        mprs_hash = &olsr->mprstable.mpr_selector_hash[index];

        for (mprs = mprs_hash->mpr_selector_forw;
            mprs != (mpr_selector_entry *) mprs_hash;
            mprs = mprs->mpr_selector_forw)
        {
            char addrString[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(&mprs->mpr_selector_main_addr,
                                        addrString);

            printf("%s ", addrString);
        }
    }
    printf("]\n");
}

// /**
// FUNCTION   :: OlsrFindMaximumCovered
// LAYER      :: APPLICATION
// PURPOSE    :: This function calculates maximum covered neighbor entry
//               for a specified willingness
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + willingness: Int32 : willingness of the node
// RETURN :: neighbor_entry* : pointer to neighbor entry if found, else
//           NULL
// **/

static
neighbor_entry* OlsrFindMaximumCovered(
    Node* node,
    Int32 willingness)
{
    UInt16 maximum = 0;
    int index;
    neighborhash_type* neighbor_hash_table;
    neighbor_entry* a_neighbor;
    neighbor_entry* mpr_candidate = NULL;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    neighbor_hash_table = olsr->neighbortable.neighborhash;

    for (index = 0; index < HASHSIZE; index++)
    {
        for (a_neighbor = neighbor_hash_table[index].neighbor_forw;
            a_neighbor != (neighbor_entry  *)&neighbor_hash_table[index];
            a_neighbor = a_neighbor->neighbor_forw)
        {
            if ((!a_neighbor->neighbor_is_mpr)
                    && (a_neighbor->neighbor_willingness == willingness)
                    && (maximum < a_neighbor->neighbor_2_nocov))
            {
                maximum = (UInt16) (a_neighbor->neighbor_2_nocov);
                mpr_candidate = a_neighbor;
            }
        }
    }
    return mpr_candidate;
}

// /**
// FUNCTION   :: OlsrChosenMpr
// LAYER      :: APPLICATION
// PURPOSE    :: This function  processes the chosen MPR and updates the
//               counters used in calculations
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + one_hop_neighbor : neighbor_entry* :  Pointer to neighbor entry
// + two_hop_covered_count : UInt16* : Pointer to counter
//                                             which counts number of 2 hop
//                                             neighbors covered
// RETURN :: void : NULL
// **/

static
void OlsrChosenMpr(
    Node* node,
    neighbor_entry* one_hop_neighbor,
    UInt16* two_hop_covered_count)
{
    neighbor_list_entry* the_one_hop_list;
    neighbor_2_list_entry* second_hop_entries;
    UInt16 count = 0;
    neighbor_entry* dup_neighbor;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    ERROR_Assert(one_hop_neighbor, "Invalid neighbor entry");

    count = *two_hop_covered_count;

    if (DEBUG)
    {
        char addrString[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&one_hop_neighbor->neighbor_main_addr,
                addrString);

        printf("Selecting  mpr as %s\n", addrString);
    }

    // set link status of one hop neighbor to MPR
    one_hop_neighbor->neighbor_is_mpr = true;

    second_hop_entries = one_hop_neighbor->neighbor_2_list;

    while (second_hop_entries != NULL)
    {

        dup_neighbor = OlsrLookupNeighborTable(node,
                           second_hop_entries->neighbor_2->neighbor_2_addr);

        if ((dup_neighbor != NULL) && (dup_neighbor->neighbor_status==SYM))
        {

            second_hop_entries = second_hop_entries->neighbor_2_next;
            continue;
        }

        // Now the neighbor is covered by this mpr

        second_hop_entries->neighbor_2->mpr_covered_count++;

        the_one_hop_list =
                second_hop_entries->neighbor_2->neighbor_2_nblist;

        if (second_hop_entries->neighbor_2->mpr_covered_count
            == olsr->mpr_coverage)
        {
            count ++;
        }

        while (the_one_hop_list != NULL)
        {
            if (the_one_hop_list->neighbor->neighbor_status == SYM)
            {
                if (second_hop_entries->neighbor_2->mpr_covered_count
                        >= olsr->mpr_coverage)
                {
                    the_one_hop_list->neighbor->neighbor_2_nocov--;
                }
            }
            the_one_hop_list = the_one_hop_list->neighbor_next;
        }

        second_hop_entries = second_hop_entries->neighbor_2_next;
    }

    *two_hop_covered_count = (UInt16)count;
}


// /**
// FUNCTION   :: OlsrAddWillAlwaysNodes
// LAYER      :: APPLICATION
// PURPOSE    :: Adds all willing nodes which are symmetric to MPR candidate set
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: UInt16 : number of nodes considered
// **/

static
UInt16 OlsrAddWillAlwaysNodes(
    Node* node)
{
    unsigned char index;
    neighbor_entry* a_neighbor;
    UInt16 count;
    count = 0;
    neighborhash_type* hash_neighbor;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    for (index = 0; index < HASHSIZE; index++)
    {
        hash_neighbor = &olsr->neighbortable.neighborhash[index];

        for (a_neighbor = hash_neighbor->neighbor_forw;
            a_neighbor != (neighbor_entry *) hash_neighbor;
            a_neighbor = a_neighbor->neighbor_forw)
        {

            if ((a_neighbor->neighbor_status == NOT_SYM)
                 || (a_neighbor->neighbor_willingness != WILL_ALWAYS))
            {
                continue;
            }

            OlsrChosenMpr(node, a_neighbor, &count);

        }

    }

    return count;
}

// /**
// FUNCTION   :: OlsrCalculateMpr
// LAYER      :: APPLICATION
// PURPOSE    :: Calculate MPR neighbors for current node
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrCalculateMpr(
    Node* node)
{
    UInt16 two_hop_covered_count = 0;
    UInt16 two_hop_count = 0;
    neighbor_2_list_entry *two_hop_list = NULL;
    neighbor_2_list_entry *tmp;
    neighbor_entry* mprs;
    Int32 i;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrClearMprs(node);

    OlsrCalculateNeighbors(node, &two_hop_count);

    two_hop_covered_count = OlsrAddWillAlwaysNodes(node);

    // RFC 3626  Section 8.3.1
    // Calculate MPRs based on willingness

    for (i = WILL_ALWAYS - 1; i > WILL_NEVER; i--)
    {
        two_hop_list = OlsrFind2HopNeighborsWith1Link(node, i);

        if (DEBUG)
        {
            printf(" Two hop count: %d\n",two_hop_count);
        }

        // if a node has two hop neighbor then it goes to select corresponding
        // one hop neighbor as MPR

        while (two_hop_list != NULL)
        {
            if (!two_hop_list->neighbor_2->neighbor_2_nblist->
                     neighbor->neighbor_is_mpr)
            {
                OlsrChosenMpr(node,
                    two_hop_list->neighbor_2->neighbor_2_nblist->neighbor,
                    &two_hop_covered_count);
            }
            tmp = two_hop_list;
            two_hop_list = two_hop_list->neighbor_2_next;
            MEM_free(tmp);
        }

        if (DEBUG)
        {
            printf(" Two hops covered: %d\n",two_hop_covered_count);
        }

        if (two_hop_covered_count >= two_hop_count)
        {
            i = WILL_NEVER;
            break;
        }

        while ((mprs = OlsrFindMaximumCovered(node,i)) != NULL)
        {
            OlsrChosenMpr(node, mprs, &two_hop_covered_count);

            if (two_hop_covered_count >= two_hop_count)
            {
                i = WILL_NEVER;
                break;
            }
        }
    }

    // increment the mpr sequence number
    olsr->neighbortable.neighbor_mpr_seq++;
}


/***************************************************************************
 *                 Hello Message Processing                                *
 ***************************************************************************/

// /**
// FUNCTION   :: OlsrDestroyHelloMessage
// LAYER      :: APPLICATION
// PURPOSE    :: Deletes Hello message working structure
// PARAMETERS ::
// + message : hl_message* : Pointer to Hello Message
// RETURN :: void : NULL
// **/

static
void OlsrDestroyHelloMessage(
    hl_message* message)
{
    hello_neighbor* neighbors;
    hello_neighbor* neighbors_tmp;

    ERROR_Assert(message, "Invalid Hello message");

    neighbors = message->neighbors;

    while (neighbors != NULL)
    {
        neighbors_tmp = neighbors;
        neighbors = neighbors->next;
        MEM_free(neighbors_tmp);
    }
}


// /**
// FUNCTION   :: OlsrPrintHelloMessage
// LAYER      :: APPLICATION
// PURPOSE    :: Prints Hello message working structure
// PARAMETERS ::
// + message : hl_message* : Pointer to Hello Message
// RETURN :: void : NULL
// **/

static
void OlsrPrintHelloMessage(
    hl_message* message)
{
    hello_neighbor* neighbors;

    char addrString[MAX_STRING_LENGTH];

    IO_ConvertIpAddressToString(&message->source_addr,
                                addrString);
    neighbors = message->neighbors;

    printf("HELLO Message Content:: \n");
    printf("%s [neighbors:", addrString);

    while (neighbors != NULL)
    {
        IO_ConvertIpAddressToString(&neighbors->address,
                                    addrString);

        printf("%s ", addrString);
        printf("Link: %d Status: %d ", neighbors->link, neighbors->status);

        neighbors = neighbors->next;
    }
    printf("]\n");
}

// /**
// FUNCTION   :: OlsrPrintTcMessage
// LAYER      :: APPLICATION
// PURPOSE    :: Prints Tc message working structure
// PARAMETERS ::
// + message: tc_message * : Pointer to TC Message
// RETURN :: void : NULL
// **/

static
void OlsrPrintTcMessage(
    tc_message* message)
{
    tc_mpr_addr  *mpr;

    char addrString1[MAX_STRING_LENGTH];
    char addrString2[MAX_STRING_LENGTH];

    IO_ConvertIpAddressToString(&message->source_addr,
                                addrString1);

    IO_ConvertIpAddressToString(&message->originator,
                                addrString2);

    mpr = message->multipoint_relay_selector_address;

    printf("TC Message Content::\n");

    printf("Source = %s Orig = %s Seq = %d Hop = %d ansn = %d [",
            addrString1,
            addrString2,
            message->packet_seq_number,
            message->hop_count,
            message->ansn);

    while (mpr != NULL)
    {
        IO_ConvertIpAddressToString(&mpr->address,
                                    addrString1);

        printf("%s ", addrString1);
        mpr = mpr->next;
    }
    printf("]\n");
}

// /**
// FUNCTION   :: OlsrDestroyTcMessage
// LAYER      :: APPLICATION
// PURPOSE    :: Deletes TC message working structure
// PARAMETERS ::
// + message : tc_message* : Pointer to TC Message
// RETURN :: void : NULL
// **/

static
void OlsrDestroyTcMessage(
    tc_message* message)
{
    tc_mpr_addr* mpr_set;
    tc_mpr_addr* mpr_set_tmp;

    mpr_set = message->multipoint_relay_selector_address;

    while (mpr_set != NULL)
    {
        mpr_set_tmp = mpr_set;
        mpr_set = mpr_set->next;
        MEM_free(mpr_set_tmp);
    }
}

// /**
// FUNCTION   :: OlsrSendonAllInterfaces
// LAYER      :: APPLICATION
// PURPOSE    :: Send olsr packet to the UDP layer on all interfaces
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + msg : olsrpacket* : OLSR packet to be sent
// + packetsize : Int32 : Size of packet
// + jitter : clocktype : jitter to be added to packet
// RETURN :: void : NULL
// **/

static
void OlsrSendonAllInterfaces(
    Node* node,
    olsrpacket* msg,
    Int32 packetsize,
    clocktype jitter)
{

    unsigned char index;

    clocktype delay;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    for (index = 0; index < node->numberInterfaces; index ++ )
    {
        Address interfaceAddr;
        interfaceAddr.networkType = NETWORK_IPV4;


        if (NetworkIpGetInterfaceType(node, index) != olsr->ip_version
            && NetworkIpGetInterfaceType(node, index)!= NETWORK_DUAL)
        {
            continue;
        }

        if (olsr->ip_version == NETWORK_IPV6)
        {
            interfaceAddr.networkType = NETWORK_IPV6;
            Ipv6GetGlobalAggrAddress(
                    node,
                    index,
                    &interfaceAddr.interfaceAddr.ipv6);
        }
        else if (olsr->ip_version == NETWORK_IPV4)
        {
            interfaceAddr.networkType = NETWORK_IPV4;
            interfaceAddr.interfaceAddr.ipv4 =
                            NetworkIpGetInterfaceAddress(node,index);
        }
//#endif

        // set msg->seq no with the  seq no for that interface
        msg->olsr_pack_seq_no = olsr->interfaceSequenceNumbers[index]++;
        delay = (clocktype) (jitter * RANDOM_erand(olsr->seed));

        if (interfaceAddr.networkType == NETWORK_IPV6)
        {
            Address multicastAddr;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[0] = 0xff;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[1] = 0x0e;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[2] = 0x00;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[3] = 0x00;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[4] = 0x00;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[5] = 0x00;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[6] = 0x00;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[7] = 0x00;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[8] = 0x00;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[9] = 0x00;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[10] = 0x00;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[11] = 0x00;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[12] = 0x00;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[13] = 0x00;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[14] = 0x00;
            multicastAddr.interfaceAddr.ipv6.s6_addr8[15] = 0x01;
            multicastAddr.networkType = NETWORK_IPV6;

            Message *udpmsg = APP_UdpCreateMessage(
                node,
                interfaceAddr,
                APP_ROUTING_OLSR_INRIA,
                multicastAddr,
                APP_ROUTING_OLSR_INRIA,
                TRACE_OLSR,
                IPTOS_PREC_INTERNETCONTROL);

            APP_UdpSetOutgoingInterface(udpmsg, index);

            APP_AddPayload(
                node,
                udpmsg,
                msg,
                packetsize);

            APP_UdpSend(node, udpmsg, delay);
        }
        else

        {
            Message *udpmsg = APP_UdpCreateMessage(
                node,
                GetIPv4Address(interfaceAddr),
                APP_ROUTING_OLSR_INRIA,
                ANY_DEST,
                APP_ROUTING_OLSR_INRIA,
                TRACE_OLSR,
                IPTOS_PREC_INTERNETCONTROL);

            APP_UdpSetOutgoingInterface(udpmsg, index);

            APP_AddPayload(
                node,
                udpmsg,
                msg,
                packetsize);

            APP_UdpSend(node, udpmsg, delay);
        }
    }
}


// /**
// FUNCTION   :: OlsrSend
// LAYER      :: APPLICATION
// PURPOSE    :: Send olsr packet to the UDP layer to specified interface
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + msg : olsrpacket *: OLSR packet to be sent
// + packetsize: Int32: Size of packet
// + jitter : clocktype: jitter to be added to packet
// + interfaceIndex: Int32 : interface to send on
// RETURN :: void : NULL
// **/

static
void OlsrSend(
    Node* node,
    olsrpacket* msg,
    Int32 packetsize,
    clocktype jitter,
    Int32 interfaceIndex)
{
    clocktype delay;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    Address interfaceAddr;
    interfaceAddr.networkType = NETWORK_IPV4;

        if (olsr->ip_version == NETWORK_IPV6)
        {
            interfaceAddr.networkType = NETWORK_IPV6;
            Ipv6GetGlobalAggrAddress(
                    node,
                    interfaceIndex,
                    &interfaceAddr.interfaceAddr.ipv6);
        }
        else if (olsr->ip_version == NETWORK_IPV4)
        {
            interfaceAddr.networkType = NETWORK_IPV4;
            interfaceAddr.interfaceAddr.ipv4 =
                            NetworkIpGetInterfaceAddress(node,interfaceIndex);
        }
//#endif

    // set msg->seq no with the packet seq no for that interface
    msg->olsr_pack_seq_no = olsr->interfaceSequenceNumbers[interfaceIndex]++;
    delay = (clocktype) (jitter * RANDOM_erand(olsr->seed));


    if (interfaceAddr.networkType == NETWORK_IPV6)
    {
        Address multicastAddr;

        multicastAddr.interfaceAddr.ipv6.s6_addr8[0] = 0xff;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[1] = 0x0e;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[2] = 0x00;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[3] = 0x00;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[4] = 0x00;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[5] = 0x00;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[6] = 0x00;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[7] = 0x00;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[8] = 0x00;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[9] = 0x00;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[10] = 0x00;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[11] = 0x00;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[12] = 0x00;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[13] = 0x00;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[14] = 0x00;
        multicastAddr.interfaceAddr.ipv6.s6_addr8[15] = 0x01;

        multicastAddr.networkType = NETWORK_IPV6;

            Message *udpmsg = APP_UdpCreateMessage(
                node,
                interfaceAddr,
                APP_ROUTING_OLSR_INRIA,
                multicastAddr,
                APP_ROUTING_OLSR_INRIA,
                TRACE_OLSR,
                IPTOS_PREC_INTERNETCONTROL);

            APP_UdpSetOutgoingInterface(udpmsg, interfaceIndex);

            APP_AddPayload(
                node,
                udpmsg,
                msg,
                packetsize);

            APP_UdpSend(node, udpmsg, delay);
    }
    else

    {
        Message* message = APP_UdpCreateMessage(
            node,
            GetIPv4Address(interfaceAddr),
            (short) APP_ROUTING_OLSR_INRIA,
            ANY_DEST,
            (short) APP_ROUTING_OLSR_INRIA,
            TRACE_OLSR,
            IPTOS_PREC_INTERNETCONTROL);

        APP_UdpSetOutgoingInterface(message, interfaceIndex);

        APP_AddPayload(
            node, 
            message,
            (char*) msg,
            packetsize);

        APP_UdpSend(
            node,
            message,
            delay);    
    }
}


//************************************************************************//
//                MID Message Related Functions                           //
//************************************************************************//

// /**
// FUNCTION   :: OlsrPrintMidMessage
// LAYER      :: APPLICATION
// PURPOSE    :: Print MID msg
// PARAMETERS ::
// + message : mid_message* : MID msg
// RETURN :: void : NULL
// **/
static
void OlsrPrintMidMessage(
    mid_message* message)
{

    char addrString[MAX_STRING_LENGTH];

    IO_ConvertIpAddressToString(&message->mid_origaddr,
        addrString);
    printf("MID Message Content::\n");
    printf("%s [ alias address : ", addrString);

    mid_alias* mid_addr = message->mid_addr;
    while (mid_addr)
    {
        IO_ConvertIpAddressToString(&mid_addr->alias_addr,
            addrString);

        printf("%s ", addrString);

        mid_addr = mid_addr->next;
    }
    printf("]\n");
}


// /**
// FUNCTION   :: OlsrSendMidPacket
// LAYER      :: APPLICATION
// PURPOSE    :: Build MID msg and Send it
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message : mid_message* : Pointer to a MID msg
// + interfaceIndex : Int32 : Outgoing Interface Index
// RETURN :: void : NULL
// **/
static
void OlsrSendMidPacket(
    Node* node,
    mid_message* message,
    Int32 interfaceIndex)
{
    char packet[MAXPACKETSIZE + 1];
    RoutingOlsr* olsr = (RoutingOlsr*) node->appData.olsr;
    olsrpacket* olsr_pkt = (olsrpacket* )packet;
    olsrmsg* olsr_msg = (olsrmsg* ) olsr_pkt->olsr_msg;
    midmsg* mid_msg = NULL;
    NodeAddress* alias_ipv4_addr = NULL;
    in6_addr* alias_ipv6_addr = NULL;

    if (olsr->ip_version == NETWORK_IPV6)
    {
        mid_msg = olsr_msg->olsr_ipv6_mid;
        alias_ipv6_addr = (in6_addr *)mid_msg->olsr_iface_addr;
    }
    else

    {
        mid_msg = olsr_msg->olsr_ipv4_mid;
        alias_ipv4_addr = (NodeAddress *)mid_msg->olsr_iface_addr;
    }

    mid_alias* mid_addr;
    Int32 outputsize;


    if (!message)
    {
        if (DEBUG)
        {
            printf("OlsrSendMidPacket: NULL msg return.\n");
        }
        return;
    }

    if (DEBUG)
    {
        char addrString[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&message->mid_origaddr,
                addrString);

        char strTime[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), strTime);
        printf("Node %d generates MID message with originator %s"
               " on interface %d at %s\n",
            node->nodeId, addrString, interfaceIndex, strTime);

        OlsrPrintMidMessage(message);
    }

    olsr->olsrStat.numMidGenerated++;

    mid_addr = message->mid_addr;
    while (mid_addr)
    {
        mid_alias* prev_mid_addr;

        if (olsr->ip_version == NETWORK_IPV6)
        {
            *alias_ipv6_addr = GetIPv6Address(mid_addr->alias_addr);
            alias_ipv6_addr++;
        }
        else

        {
            *alias_ipv4_addr = GetIPv4Address(mid_addr->alias_addr);
            alias_ipv4_addr++;
        }
        prev_mid_addr = mid_addr;

        mid_addr = mid_addr->next;

        MEM_free(prev_mid_addr);
    }

    olsr_msg->olsr_vtime = OlsrDouble2ME((Float32)(olsr->mid_hold_time
                                                       / SECOND));
    olsr_msg->olsr_msgtype = MID_PACKET;

    if (olsr->ip_version == NETWORK_IPV6)
    {
        olsr_msg->olsr_msg_tail.olsr_ipv6_msg.msg_seq_no =
            message->mid_seqno;
        olsr_msg->olsr_msg_tail.olsr_ipv6_msg.hop_count =
            message->mid_hopcnt;
        olsr_msg->olsr_msg_tail.olsr_ipv6_msg.ttl = message->mid_ttl;
        outputsize = (Int32)((char *)alias_ipv6_addr - packet);
        olsr_msg->olsr_msg_size = (UInt16) ((char *)alias_ipv6_addr
                                   - (char *)olsr_msg);
        olsr_msg->originator_ipv6_address = GetIPv6Address(
                                                message->mid_origaddr);
    }
    else

    {
        olsr_msg->olsr_msg_tail.olsr_ipv4_msg.msg_seq_no =
            message->mid_seqno;
        olsr_msg->olsr_msg_tail.olsr_ipv4_msg.hop_count =
            message->mid_hopcnt;
        olsr_msg->olsr_msg_tail.olsr_ipv4_msg.ttl = message->mid_ttl;
        outputsize = (Int32)((char *)alias_ipv4_addr - packet);
        olsr_msg->olsr_msg_size = (UInt16) ((char *)alias_ipv4_addr
                                      - (char *)olsr_msg);
        olsr_msg->originator_ipv4_address = GetIPv4Address(
                                                message->mid_origaddr);
    }

   if (DEBUG)
   {
       printf ("Node %d: MID Packet  size is %d\n",
           node->nodeId, outputsize);
   }

    olsr_pkt->olsr_packlen = (UInt16) (outputsize);

    OlsrSend(node,
        olsr_pkt,
        outputsize,
        MAX_MID_JITTER,
        interfaceIndex);

    return;
}


// /**
// FUNCTION   :: OlsrBuildMidPacket
// LAYER      :: APPLICATION
// PURPOSE    :: Build MID Packet
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message : mid_message* : Pointer to a MID msg
// RETURN :: void : NULL
// **/
static
void OlsrBuildMidPacket(
    Node* node,
    mid_message* message)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    Address main_addr = olsr->main_address;

    message->mid_origaddr = main_addr;  // originator's address
    message->mid_hopcnt = 0;    // number of hops to destination
    message->mid_ttl = MAX_TTL; // ttl

    message->mid_seqno = OlsrGetMsgSeqNo(node);     // sequence number

    message->main_addr = main_addr;     // main address
    message->mid_addr = NULL;  // alias address

    for (Int32 i = 0; i < node->numberInterfaces; i++)
    {
        mid_alias* mid_addr;
        Address interfaceAddr;

        if (NetworkIpGetInterfaceType(node, i) != olsr->ip_version
            && NetworkIpGetInterfaceType(node, i)!= NETWORK_DUAL)
        {
            continue;
        }

        if (olsr->ip_version == NETWORK_IPV6)
        {
            interfaceAddr.networkType = NETWORK_IPV6;
            Ipv6GetGlobalAggrAddress(
                    node,
                    i,
                    &interfaceAddr.interfaceAddr.ipv6);
        }
        else if (olsr->ip_version == NETWORK_IPV4)
        {
            interfaceAddr.networkType = NETWORK_IPV4;
            interfaceAddr.interfaceAddr.ipv4 =
                            NetworkIpGetInterfaceAddress(node,i);
        }
//#endif

        // Do not consider interface with main address and
        // also non-olsr interfaces

        if (Address_IsSameAddress(&interfaceAddr, &main_addr) ||
            (NetworkIpGetUnicastRoutingProtocolType(node, i, olsr->ip_version)
            != ROUTING_PROTOCOL_OLSR_INRIA)
//#endif
            )
        {
            continue;
        }
        mid_addr = (mid_alias* ) MEM_malloc(sizeof(mid_alias));
        memset(mid_addr, 0, sizeof(mid_alias));

        mid_addr->alias_addr = interfaceAddr;
        mid_addr->next = message->mid_addr;
        message->mid_addr = mid_addr;
    }

    return;
}


// /**
// FUNCTION   :: OlsrGenerateMid
// LAYER      :: APPLICATION
// PURPOSE    :: Generate a MID msg
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interfaceIndex : Int32 : Outgoing Interface Index
// RETURN :: void : NULL
// **/
static
void OlsrGenerateMid(
    Node* node,
    Int32 interfaceIndex)
{
    mid_message midpacket;

    // receive mid packet info from the node structure to mid_message
    OlsrBuildMidPacket(node, &midpacket);

    // prepare the mid message and send
    OlsrSendMidPacket(node, &midpacket, interfaceIndex);

    return;
}


// /**
// FUNCTION   :: OlsrMidForward
// LAYER      :: APPLICATION
// PURPOSE    :: Forward a MID msg
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + m_forward : olsrmsg* : Pointer to a generic olsr msg
// + interfaceIndex : Int32 : Outgoing Interface Index
// RETURN :: void : NULL
// **/
static
void OlsrMidForward(
    Node* node,
    olsrmsg* m_forward)
{

    char packet[MAXPACKETSIZE + 1];
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    olsrpacket* olsr_pkt = (olsrpacket* )packet;
    olsrmsg* olsr_msg = (olsrmsg* ) olsr_pkt->olsr_msg;

    olsr->olsrStat.numMidRelayed++;
    olsr_pkt->olsr_packlen = 2 * sizeof(UInt16) + m_forward->olsr_msg_size;

    memcpy(olsr_msg, m_forward, m_forward->olsr_msg_size);

    OlsrSendonAllInterfaces(node,
        olsr_pkt,
        olsr_pkt->olsr_packlen,
        MAX_MID_JITTER);

    return;
}


// /**
// FUNCTION   :: OlsrMidChgeStruct
// LAYER      :: APPLICATION
// PURPOSE    :: Parse an olsr message and collects all information in MID
//               msg
// PARAMETERS ::
// + node : Node* : Pointer to Node
// + mid_msg : mid_message* : Pointer to MID msg
// + m : olsrmsg* : Pointer to a generic olsr msg
// RETURN :: void : NULL
// **/
static
void OlsrMidChgeStruct(
    Node* node,
    mid_message* mid_msg,
    olsrmsg* m)
{
    midmsg* mid;
    NodeAddress* alias_ipv4_addr = NULL;
    in6_addr* alias_ipv6_addr = NULL;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    if (!m)
    {
        return;
    }
    if (m->olsr_msgtype != MID_PACKET)
    {
        return;
    }

    // parse olsr message and collects all information in mid_message

    if (olsr->ip_version == NETWORK_IPV6)
    {
        SetIPv6AddressInfo(&mid_msg->mid_origaddr,
            m->originator_ipv6_address);  // originator's address
        SetIPv6AddressInfo(&mid_msg->main_addr, m->originator_ipv6_address);     // main address
        mid = m->olsr_ipv6_mid;
        alias_ipv6_addr = (in6_addr *)mid->olsr_iface_addr;
    }
    else

    {
        SetIPv4AddressInfo(&mid_msg->mid_origaddr,
            m->originator_ipv4_address);  // originator's address
        SetIPv4AddressInfo(&mid_msg->main_addr, m->originator_ipv4_address);     // main address
        mid = m->olsr_ipv4_mid;
        alias_ipv4_addr = (NodeAddress *)mid->olsr_iface_addr;
    }
    mid_msg->vtime = OlsrME2Double(m->olsr_vtime); // validity time
    mid_msg->mid_addr = NULL;  // alias address

    mid_alias* mid_addr;

    if (olsr->ip_version == NETWORK_IPV6)
    {
        // number of hops to destination
        mid_msg->mid_hopcnt = m->olsr_msg_tail.olsr_ipv6_msg.hop_count;
        mid_msg->mid_ttl = MAX_TTL; // ttl
        // sequence number
        mid_msg->mid_seqno = m->olsr_msg_tail.olsr_ipv6_msg.msg_seq_no;
        while ((char *)alias_ipv6_addr < (char *)m + m->olsr_msg_size)
        {
            mid_addr = (mid_alias* )MEM_malloc(sizeof(mid_alias));
            memset(mid_addr, 0, sizeof(mid_alias));

            if (mid_addr == NULL)
            {
                printf("olsrd: out of memery \n");
                break;
            }
            SetIPv6AddressInfo(&mid_addr->alias_addr, *alias_ipv6_addr);
            mid_addr->next = mid_msg->mid_addr;
            mid_msg->mid_addr = mid_addr;
            alias_ipv6_addr++;
        }
    }
    else

    {

        mid_msg->mid_hopcnt = m->olsr_msg_tail.olsr_ipv4_msg.hop_count;
        mid_msg->mid_ttl = MAX_TTL;
        mid_msg->mid_seqno = m->olsr_msg_tail.olsr_ipv4_msg.msg_seq_no;

        while ((char *)alias_ipv4_addr < (char *)m + m->olsr_msg_size)
        {
            mid_addr = (mid_alias* )MEM_malloc(sizeof(mid_alias));
            memset(mid_addr, 0, sizeof(mid_alias));

            if (mid_addr == NULL)
            {
                printf("olsrd: out of memery \n");
                break;
            }
            SetIPv4AddressInfo(&mid_addr->alias_addr, *alias_ipv4_addr);
            mid_addr->next = mid_msg->mid_addr;
            mid_msg->mid_addr = mid_addr;
            alias_ipv4_addr++;
        }
    }
    return;
}

// /**
// FUNCTION   :: OlsrDestroyMidMessage
// LAYER      :: APPLICATION
// PURPOSE    :: Destroy a MID msg
// PARAMETERS ::
// + message : mid_message* : Pointer to MID msg
// RETURN :: void : NULL
// **/
static
void OlsrDestroyMidMessage(
    mid_message* message)
{
    mid_alias* mid_addr = message->mid_addr;

    while (mid_addr)
    {
        mid_alias* prev_mid_addr = mid_addr;
        mid_addr = mid_addr->next;
        MEM_free(prev_mid_addr);
    }

    return;
}

// HNA Message Related Utilities starts

// /**
// FUNCTION   :: OlsrPrintHnaMessage
// LAYER      :: APPLICATION
// PURPOSE    :: print HNA msg
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message : hna_message* : Pointer to a HNA msg
// RETURN :: void : NULL
// **/

static
void OlsrPrintHnaMessage(Node* node, hna_message* message)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    hna_net_addr* hna_net_pair = message->hna_net_pair;

    printf("HNA Message Content::\n");
    while (hna_net_pair)
    {
        char addrString[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&hna_net_pair->net,
            addrString);
        printf("NonOlsrNetAddress: %s    ", addrString);

        if (olsr->ip_version == NETWORK_IPV6)
        {

            printf("NonOlsrNetPrefixLen: %d\n", hna_net_pair->netmask.v6);

        }
        else
        {
            IO_ConvertIpAddressToString(hna_net_pair->netmask.v4,
                addrString);
            printf("NonOlsrNetMask: %s\n", addrString);

        }

        hna_net_pair = hna_net_pair->next;

    }

    return;
}

// /**
// FUNCTION   :: OlsrDestroyHnaMessage
// LAYER      :: APPLICATION
// PURPOSE    :: destroy HNA msg
// PARAMETERS ::
// + message : hna_message* : Pointer to a HNA msg
// RETURN :: void : NULL
// **/

static
void OlsrDestroyHnaMessage(hna_message* message)
{
    hna_net_addr* hna_net_pair = message->hna_net_pair;

    while (hna_net_pair)
    {
        hna_net_addr* prev_hna_net_pair = hna_net_pair;
        hna_net_pair = hna_net_pair->next;
        MEM_free(prev_hna_net_pair);
    }

    return;
}


// /**
// FUNCTION   :: OlsrSendHnaPacket
// LAYER      :: APPLICATION
// PURPOSE    :: Send HNA Packet
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message : hna_message* : Pointer to a HNA msg
// + interfaceIndex : Int32 : Outgoing Interface Index
// RETURN :: void : NULL
// **/
static
void OlsrSendHnaPacket(
    Node* node,
    hna_message* message,
    Int32 interfaceIndex)
{
    char packet[MAXPACKETSIZE + 1];
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    olsrpacket* olsr_pkt = (olsrpacket* )packet;
    olsrmsg* olsr_msg = (olsrmsg* ) olsr_pkt->olsr_msg;
    hnamsg* hna_msg = NULL;
    struct _hnapair_ipv6* hnapair_ipv6 = NULL;
    struct _hnapair_ipv4* hnapair_ipv4 = NULL;


    if (olsr->ip_version == NETWORK_IPV6)
    {
        hna_msg = olsr_msg->olsr_ipv6_hna;
        hnapair_ipv6 = (struct _hnapair_ipv6* )hna_msg->hna_net_pair;
    }
    else

    {
        hna_msg = olsr_msg->olsr_ipv4_hna;
        hnapair_ipv4 = (struct _hnapair_ipv4* )hna_msg->hna_net_pair;
    }


    if (!message)
    {
          if (DEBUG)
        {
            printf("OlsrSendHnaPacket: NULL msg return.\n");
        }
        return;
    }

    olsr->olsrStat.numHnaGenerated++;

    hna_net_addr* hna_net_pair = message->hna_net_pair;

    while (hna_net_pair)
    {

        if (olsr->ip_version == NETWORK_IPV6)
        {
            hnapair_ipv6->addr = GetIPv6Address(hna_net_pair->net);

            hnapair_ipv6->netmask.s6_addr32[3] = hna_net_pair->netmask.v6;

            hnapair_ipv6++;
        }
        else

        {
            hnapair_ipv4->addr = GetIPv4Address(hna_net_pair->net);

            hnapair_ipv4->netmask = hna_net_pair->netmask.v4;

            hnapair_ipv4++;
        }

        hna_net_pair = hna_net_pair->next;
    }

    olsr_msg->olsr_vtime = OlsrDouble2ME((Float32)(olsr->hna_hold_time
                                                       / SECOND));
    olsr_msg->olsr_msgtype = HNA_PACKET;

    if (olsr->ip_version == NETWORK_IPV6)
    {
        olsr_msg->olsr_msg_size = UInt16((char *)hnapair_ipv6 -
                                                           (char *)olsr_msg);
        olsr_pkt->olsr_packlen = UInt16((char *)hnapair_ipv6 -
                                                             (char *)packet);
        olsr_msg->olsr_msg_tail.olsr_ipv6_msg.msg_seq_no =
            message->hna_seqno;
        olsr_msg->olsr_msg_tail.olsr_ipv6_msg.ttl = message->hna_ttl;
        olsr_msg->olsr_msg_tail.olsr_ipv6_msg.hop_count =
            message->hna_hopcnt;
        olsr_msg->originator_ipv6_address = GetIPv6Address(
                                                message->hna_origaddr);
    }
    else
    {
        olsr_msg->olsr_msg_size = UInt16((char *)hnapair_ipv4 -
                                                           (char *)olsr_msg);
        olsr_pkt->olsr_packlen = UInt16((char *)hnapair_ipv4 -
                                                             (char *)packet);
        olsr_msg->olsr_msg_tail.olsr_ipv4_msg.msg_seq_no =
            message->hna_seqno;
        olsr_msg->olsr_msg_tail.olsr_ipv4_msg.ttl = message->hna_ttl;
        olsr_msg->olsr_msg_tail.olsr_ipv4_msg.hop_count =
            message->hna_hopcnt;
        olsr_msg->originator_ipv4_address = GetIPv4Address(
                                                message->hna_origaddr);
    }

    if (DEBUG)
    {
        OlsrPrintHnaMessage(node, message);
    }

    OlsrSend(node,
        olsr_pkt,
        olsr_pkt->olsr_packlen,
        MAX_HNA_JITTER,
        interfaceIndex);

    OlsrDestroyHnaMessage(message);

    return;
}

// /**
// FUNCTION   :: OlsrBuildHnaPacket
// LAYER      :: APPLICATION
// PURPOSE    :: Build HNA pkt
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message : hna_message* : Pointer to a HNA msg
// RETURN :: void : NULL
// **/

static
void OlsrBuildHnaPacket(
    Node* node,
    hna_message* message)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    message->hna_origaddr = olsr->main_address; // originator
    message->hna_hopcnt = 0;     // number of hops to destination
    message->hna_ttl = MAX_TTL;  // ttl

    message->hna_seqno = OlsrGetMsgSeqNo(node); // sequence number
    message->hna_net_pair = NULL;

    // Add non-olsr network address and netmask to the msg-tail
    // i.e. link list of message->hna_net_pair

    for (Int32 i = 0; i < node->numberInterfaces; i++)
    {
        if (NetworkIpGetUnicastRoutingProtocolType(node, i, olsr->ip_version)
            != ROUTING_PROTOCOL_OLSR_INRIA
            && NetworkIpInterfaceIsEnabled(node, i)
            )
        {

            Address netAddr;
            NetworkGetInterfaceInfo(node, i, &netAddr, olsr->ip_version);


        if (olsr->ip_version != netAddr.networkType)
        {
            continue;
        }
        hna_net_addr* hna_net_pair = (hna_net_addr*) MEM_malloc(
                                                       sizeof(hna_net_addr));
        memset(hna_net_pair, 0, sizeof(hna_net_addr));
//#endif

            if (olsr->ip_version == NETWORK_IPV6)
            {
                hna_net_pair->netmask.v6 = MAX_PREFIX_LEN;

                Ipv6GetPrefix(&netAddr.interfaceAddr.ipv6,
                    &hna_net_pair->net.interfaceAddr.ipv6,
                    hna_net_pair->netmask.v6);
                hna_net_pair->net.networkType = NETWORK_IPV6;

            }
            else
            {
                hna_net_pair->netmask.v4 =
                        NetworkIpGetInterfaceSubnetMask(node, i);

                if (hna_net_pair->netmask.v4)
                {
                    hna_net_pair->net.interfaceAddr.ipv4 =
                       netAddr.interfaceAddr.ipv4 & hna_net_pair->netmask.v4;
                }
                else
                {
                    hna_net_pair->net.interfaceAddr.ipv4 =
                                                  netAddr.interfaceAddr.ipv4;
                }
                hna_net_pair->net.networkType = NETWORK_IPV4;
            }
            hna_net_pair->next = message->hna_net_pair;
            message->hna_net_pair = hna_net_pair;
        }
    }
    return;
}


// /**
// FUNCTION   :: OlsrGenerateHna
// LAYER      :: APPLICATION
// PURPOSE    :: Builds and sends a HNA message
// PARAMETERS ::
//               :Pointer to Node structure
//               : Outgoing Interface Index
// RETURN :: void : NULL
// **/

static
void OlsrGenerateHna(
    Node* node,
    Int32 interfaceIndex)
{
    hna_message hnapacket;

    OlsrBuildHnaPacket(node, &hnapacket);
    OlsrSendHnaPacket(node, &hnapacket, interfaceIndex);

    return;

}
/***************************************************************************
 *                   Definition of Topology Table                          *
 ***************************************************************************/

// /**
// FUNCTION   :: OlsrDeleteListofLast
// LAYER      :: APPLICATION
// PURPOSE    :: Deletes address from destination list of current
//               topology entries
// PARAMETERS ::
// + last_entry : topology_last_entry* : Pointer to Topology entry list
// + dst : Address: Address to be deleted
// RETURN :: void : NULL
// **/

static
void OlsrDeleteListofLast(
    topology_last_entry* last_entry,
    Address dst)
{
    destination_list* dest_list;
    destination_list* dest_list_tmp;

    ERROR_Assert(last_entry, "Invalid topology entry");

    // get destination list from this topology
    dest_list = last_entry->topology_list_of_destinations;
    last_entry->topology_list_of_destinations = NULL;

    while (dest_list != NULL)
    {
        if (Address_IsSameAddress(&dest_list->destination_node->
                topology_destination_dst, &dst))
        {
            // address matches, remove from the destination list
            dest_list_tmp = dest_list;
            dest_list = dest_list->next;
            MEM_free(dest_list_tmp);
        }
        else
        {
            dest_list_tmp = dest_list;
            dest_list = dest_list->next;
            dest_list_tmp->next = last_entry->topology_list_of_destinations;
            last_entry->topology_list_of_destinations = dest_list_tmp;
        }
    }
}


// /**
// FUNCTION   :: OlsrDeleteDestTopolgyTable
// LAYER      :: APPLICATION
// PURPOSE    :: Deletes destination list from topology table
// PARAMETERS ::
// + dest_entry : topology_destination_entry * : Pointer to destination list
// RETURN :: void : NULL
// **/

static
void OlsrDeleteDestTopolgyTable(
    topology_destination_entry* dest_entry)
{
    last_list* list_of_last;
    topology_last_entry* last_entry;

    ERROR_Assert(dest_entry, "Invalid topology entry");

    list_of_last = dest_entry->topology_destination_list_of_last;

    while (list_of_last != NULL)
    {
        last_entry = list_of_last->last_neighbor;

        OlsrDeleteListofLast(last_entry,
                                 dest_entry->topology_destination_dst);

        dest_entry->topology_destination_list_of_last = list_of_last->next;
        MEM_free(list_of_last);
        list_of_last = dest_entry->topology_destination_list_of_last;
    }
    OlsrRemoveList((olsr_qelem* ) dest_entry);
    MEM_free((void* ) dest_entry);
}

// /**
// FUNCTION   :: OlsrDeleteListofDest
// LAYER      :: APPLICATION
// PURPOSE    :: Deletes address from destination list
// PARAMETERS ::
// + dest_entry : topology_destination_entry*  : Pointer to Topology
//                                               destination list
// + dst : Address: Address to be deleted
// RETURN :: void : NULL
// **/

static
void OlsrDeleteListofDest(
    topology_destination_entry* dest_entry,
    Address dst)
{
    last_list *list_of_last;
    last_list *list_of_last_tmp;

    ERROR_Assert(dest_entry, "Invalid topology entry");

    list_of_last = dest_entry->topology_destination_list_of_last;
    dest_entry->topology_destination_list_of_last = NULL;

    while (list_of_last != NULL)
    {
        if (Address_IsSameAddress(&list_of_last->last_neighbor->
                topology_last, &dst))
        {
            list_of_last_tmp = list_of_last;
            list_of_last = list_of_last->next;
            MEM_free(list_of_last_tmp);
        }
        else
        {
            list_of_last_tmp = list_of_last;
            list_of_last = list_of_last->next;
            list_of_last_tmp->next =
                    dest_entry->topology_destination_list_of_last;

            dest_entry->topology_destination_list_of_last = list_of_last_tmp;
        }
    }

    if (dest_entry->topology_destination_list_of_last == NULL)
    {
        OlsrRemoveList((olsr_qelem *)dest_entry);
        MEM_free((void *)dest_entry);
    }
}

// /**
// FUNCTION   :: OlsrDeleteLastTopolgyTable
// LAYER      :: APPLICATION
// PURPOSE    :: Deletes last entry from topology table
// PARAMETERS ::
// + last_entry : topology_last_entry* : Pointer to last entry
// RETURN :: void : NULL
// **/

static
void OlsrDeleteLastTopolgyTable(
    topology_last_entry* last_entry)
{
    destination_list* list_of_dest;
    topology_destination_entry* dest_entry;

    ERROR_Assert(last_entry, "Invalid topology entry");

    list_of_dest = last_entry->topology_list_of_destinations;

    while (list_of_dest != NULL)
    {
        dest_entry = list_of_dest->destination_node;

        OlsrDeleteListofDest(dest_entry, last_entry->topology_last);
        last_entry->topology_list_of_destinations = list_of_dest->next;
        MEM_free(list_of_dest);
        list_of_dest = last_entry->topology_list_of_destinations;
    }

    OlsrRemoveList((olsr_qelem *)last_entry);
    MEM_free((void *)last_entry);
}


// /**
// FUNCTION   :: OlsrInsertLastTopologyTable
// LAYER      :: APPLICATION
// PURPOSE    :: Inserts the originator message from TC message in last entry
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + last_entry : topology_last_entry* : Pointer to topology last entry
// + message : tc_message* : Pointer to tc message
// RETURN :: void : NULL
// **/

static
void OlsrInsertLastTopologyTable(
    Node* node,
    topology_last_entry* last_entry,
    tc_message* message)
{
    UInt32 hash;
    topology_last_hash* top_last_hash;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    ERROR_Assert(last_entry, "Invalid topology entry");
    ERROR_Assert(message, "Invalid TC message");

    OlsrHashing(message->originator, &hash);
    top_last_hash = &olsr->topologylasttable[hash % HASHMASK];

    /* prepare the last entry */
    last_entry->topology_last = message->originator;
    last_entry->topology_list_of_destinations = NULL;
    last_entry->topologylast_hash = hash;
    last_entry->topology_seq = message->ansn;

    last_entry->topology_timer = node->getNodeTime()
                                 + (clocktype)(message->vtime * SECOND);

    OlsrInsertList((olsr_qelem *)last_entry, (olsr_qelem *)top_last_hash);
}

// /**
// FUNCTION   :: OlsrInsertDestTopologyTable
// LAYER      :: APPLICATION
// PURPOSE    :: Inserts the MPR address from the TC message in dest entry
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + dest_entry : topology_destination_entry* : Pointer to Topology
//                                               destination  entry
// + mpr : tc_mpr_addr* : pointer to mpr address to be added
// RETURN :: void : NULL
// **/

static
void OlsrInsertDestTopologyTable(
    Node* node,
    topology_destination_entry* dest_entry,
    tc_mpr_addr* mpr)
{
    UInt32 hash;
    topology_destination_hash* top_dest_hash;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    ERROR_Assert(dest_entry, "Invalid topology entry");
    ERROR_Assert(mpr, "Invalid MPR entry");

    OlsrHashing(mpr->address, &hash);

    top_dest_hash = &olsr->topologytable[hash % HASHMASK];

    dest_entry->topology_destination_dst = mpr->address;
    dest_entry->topology_destination_list_of_last = NULL;
    dest_entry->topologydestination_hash = hash;

    OlsrInsertList((olsr_qelem* )dest_entry, (olsr_qelem *)top_dest_hash);
}

// /**
// FUNCTION   :: OlsrLookupLastTopologyTable
// LAYER      :: APPLICATION
// PURPOSE    :: Searches the address in the last entry
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + last : Address :  address to be searched
// RETURN :: topology_last_entry* : Pointer to entry if found.
//                                   else NULL
// **/

static
topology_last_entry* OlsrLookupLastTopologyTable(
    Node* node,
    Address last)
{
    topology_last_entry* top_last;
    topology_last_hash* top_last_hash;
    UInt32 hash;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrHashing(last, &hash);

    top_last_hash = &olsr->topologylasttable[hash % HASHMASK];

    for (top_last = top_last_hash->topology_last_forw;
        top_last != (topology_last_entry* ) top_last_hash;
        top_last = top_last->topology_last_forw)
    {
        if (top_last->topologylast_hash != hash)
        {
            continue;
        }

        // address matches with the topology last address?
        if (Address_IsSameAddress(&top_last->topology_last, &last))
        {
            return top_last;
        }
    }
    return NULL;
}

// /**
// FUNCTION   :: OlsrLookupDestTopologyTable
// LAYER      :: APPLICATION
// PURPOSE    :: Searches the address in the dest entry
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + dest : Address : Address to be searched
// RETURN :: topology_dest_entry* : Pointer to entry if found.
//                                   else NULL
// **/

static
topology_destination_entry* OlsrLookupDestTopologyTable(
    Node* node,
    Address dest)
{
    topology_destination_entry* top_dest;
    topology_destination_hash* top_dest_hash;
    UInt32 hash;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    if (DEBUG)
    {
        printf("OlsrLookupDestTopologyTable\n");
    }

    OlsrHashing(dest, &hash);

    top_dest_hash = &olsr->topologytable[hash % HASHMASK];

    for (top_dest = top_dest_hash->topology_destination_forw;
        top_dest != (topology_destination_entry* ) top_dest_hash;
        top_dest = top_dest->topology_destination_forw)
    {
        if (top_dest->topologydestination_hash != hash)
        {
            continue;
        }

        // matches the address with anyone from destination list
        if (Address_IsSameAddress(&top_dest->topology_destination_dst, &dest))
        {
            return top_dest;
        }
    }
    return NULL;
}


// /**
// FUNCTION   :: OlsrinListDestTopology
// LAYER      :: APPLICATION
// PURPOSE    :: Searches the address in the dest entry of this last entry
// PARAMETERS ::
// + last_entry : topology_last_entry* : Pointer to Topology
//                                               last  entry
// + address : Address : Address to be searched
// RETURN :: destination_list* : Pointer to destination list if found,
//                                else NULL
// **/


static
destination_list* OlsrinListDestTopology(
    topology_last_entry* last_entry,
    Address address)
{
    destination_list* list_of_dest;

    ERROR_Assert(last_entry, "Invalid topology entry");

    list_of_dest = last_entry->topology_list_of_destinations;

    if (DEBUG)
    {
        printf("OlsrinListDestTopology\n");
    }

    while (list_of_dest != NULL)
    {
        if (Address_IsSameAddress(
                &list_of_dest->destination_node->topology_destination_dst,
                &address))
        {
            // matches return list of destination
            return list_of_dest;
        }
        list_of_dest = list_of_dest->next;
    }
    return NULL;
}

// /**
// FUNCTION   :: OlsrUpdateLastTable
// LAYER      :: APPLICATION
// PURPOSE    :: This function updates time out for the
//               existing last entry and
//               creates new entries for the new destinations
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + last_entry : topology_last_entry * : entry to be updated
// + message : tc_message* : Pointer to tc msg
// + addr_ip : Address : address to be updated
// RETURN :: void : NULL
// **/

static
void OlsrUpdateLastTable(
    Node* node,
    topology_last_entry* last_entry,
    tc_message* message,
    Address addr_ip)
{
     // This function updates time out for the existant pointed by t_last and
     // creates new entries for the new destinations

    tc_mpr_addr* mpr;
    destination_list* topo_dest;
    topology_destination_entry* destination_entry;
    last_list* topo_last;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    ERROR_Assert(last_entry, "Invalid topology entry");
    ERROR_Assert(message, "Invalid TC message");

    mpr = message->multipoint_relay_selector_address;

    // refresh timer
    last_entry->topology_timer = node->getNodeTime()
                                 + (clocktype)(message->vtime * SECOND);

    if (DEBUG)
    {
        printf("Updating this top_last !\n");
        printf("The list of MPRs is\n:");
        while (mpr != NULL)
        {
            char addrString[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(&mpr->address,
                                        addrString);

            printf("This mpr is %s\n", addrString);
            mpr = mpr->next;
        }
        mpr = message->multipoint_relay_selector_address;
    }

    // search in the MPR list of TC message
    while (mpr != NULL)
    {
        // this node ip address in the MPR list not considered
        if (!Address_IsSameAddress(&mpr->address, &addr_ip))
        {
            if ((topo_dest = OlsrinListDestTopology(
                last_entry, mpr->address)) == NULL)
            {
                // if not in the topology table
                if (DEBUG)
                {
                    printf("update last table\n");
                }

                olsr->changes_topology = TRUE;

                // search in the destination list
                if ((destination_entry = OlsrLookupDestTopologyTable
                    (node, mpr->address)) == NULL)
                {
                    // not found, insert in the destination list
                    destination_entry = (topology_destination_entry *)
                        MEM_malloc(sizeof(topology_destination_entry));

                    memset(
                        destination_entry,
                        0,
                        sizeof(topology_destination_entry));

                    OlsrInsertDestTopologyTable(node,
                            destination_entry, mpr);
                }

                // creating the structure for the linkage
                topo_dest = (destination_list *)
                        MEM_malloc(sizeof(destination_list));
                memset(topo_dest, 0, sizeof(destination_list));

                topo_dest->next = last_entry->topology_list_of_destinations;
                last_entry->topology_list_of_destinations = topo_dest;

                topo_last = (last_list *) MEM_malloc(sizeof(last_list));
                memset(topo_last, 0, sizeof(last_list));

                topo_last->next =
                    destination_entry->topology_destination_list_of_last;

                destination_entry->topology_destination_list_of_last =
                    topo_last;

                // linking the last and the dest
                topo_last->last_neighbor = last_entry;
                topo_dest->destination_node = destination_entry;
            }

            if (DEBUG)
            {
                char addrString[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(&mpr->address,
                                            addrString);

                printf("This mpr is %s\n", addrString);
                if (mpr->next == NULL)
                {
                    printf("the next is NULL\n");
                }
                else
                {
                    printf("the next is not NULL\n");
                }
            }
        }
        mpr = mpr->next;
    }
}


// /**
// FUNCTION   :: OlsrReleaseTopologyTable
// LAYER      :: APPLICATION
// PURPOSE    :: releases topology table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrReleaseTopologyTable(
    Node* node)
{
    Int32 index;

    topology_destination_entry* top_dest;
    topology_destination_entry* top_dest_tmp;
    topology_destination_hash* top_dest_hash;

    topology_last_entry* top_last;
    topology_last_entry* top_last_tmp;
    topology_last_hash* top_last_hash;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    // delete topology destination table
    for (index = 0; index < HASHSIZE; index++)
    {
        top_dest_hash = &olsr->topologytable[index];
        top_dest = top_dest_hash->topology_destination_forw;
        while (top_dest != (topology_destination_entry *) top_dest_hash)
        {
            top_dest_tmp = top_dest;
            top_dest = top_dest->topology_destination_forw;
            OlsrDeleteDestTopolgyTable(top_dest_tmp);
        }
    }

    // delete topology last table
    for (index = 0; index < HASHSIZE; index++)
    {
        top_last_hash = &olsr->topologylasttable[index];
        top_last = top_last_hash->topology_last_forw;
        while (top_last != (topology_last_entry *) top_last_hash)
        {
            top_last_tmp = top_last;
            top_last = top_last->topology_last_forw;
            OlsrDeleteLastTopolgyTable(top_last_tmp);
        }
    }
}

// /**
// FUNCTION   :: OlsrTimeoutTopologyTable
// LAYER      :: APPLICATION
// PURPOSE    :: This function will be called
//               when olsr->top_hold_timer expires
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrTimeoutTopologyTable(
    Node* node)
{
    // We only check last table because we didn't include a time out in the
    // dest entries

    Int32 index;
    topology_last_entry* top_last;
    topology_last_entry* top_last_tmp;
    topology_last_hash* top_last_hash;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    for (index = 0; index < HASHSIZE; index++)
    {
        top_last_hash = &olsr->topologylasttable[index];
        top_last = top_last_hash->topology_last_forw;
        while (top_last != (topology_last_entry *) top_last_hash)
        {
            // check each tuple for expiration
            if (node->getNodeTime() >= top_last->topology_timer)
            {
                top_last_tmp = top_last;
                top_last = top_last->topology_last_forw;
                OlsrDeleteLastTopolgyTable(top_last_tmp);
                olsr->changes_topology = TRUE;
            }
            else
            {
                top_last = top_last->topology_last_forw;
            }
        }
    }
}


// /**
// FUNCTION   :: OlsrPrintTopologyTable
// LAYER      :: APPLICATION
// PURPOSE    :: Prints topology table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrPrintTopologyTable(
    Node* node)
{
    Int32 index;
    topology_last_entry* top_last;
    topology_last_hash* top_last_hash;
    destination_list* top_dest_list;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    printf("Topology Table:\n");
    for (index = 0; index < HASHSIZE; index++)
    {
        top_last_hash = &olsr->topologylasttable[index];
        for (top_last = top_last_hash->topology_last_forw;
            top_last != (topology_last_entry *) top_last_hash;
            top_last = top_last->topology_last_forw)
        {
            char addrString[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(&top_last->topology_last,
                                        addrString);

            printf("%-15s: [last_hop_of: ",addrString);
            top_dest_list = top_last->topology_list_of_destinations;

            while (top_dest_list != NULL)
            {
                IO_ConvertIpAddressToString(
                    &top_dest_list->destination_node->
                        topology_destination_dst, addrString);

                printf("%s ", addrString);
                top_dest_list = top_dest_list->next;
            }
            printf("]\n");
        }
    }
}



/***************************************************************************
 *                   Defination of Routing Table                           *
 ***************************************************************************/
// /**
// FUNCTION   :: OlsrLookupRoutingTable
// LAYER      :: APPLICATION
// PURPOSE    :: Searches in the routing table for this address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + dst : Address : Address to be looked up
// RETURN :: rt_entry* : Pointer to entry if found, else NULL
// **/

static
rt_entry* OlsrLookupRoutingTable(
    Node* node,
    Address dst)
{
    rt_entry* destination;
    rthash* routing_hash;
    UInt32 hash;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrHashing(dst, &hash);
    routing_hash = &olsr->routingtable[hash % HASHMASK];

    for (destination = routing_hash->rt_forw;
        destination != (rt_entry* ) routing_hash;
        destination = destination->rt_forw)
    {
        if (destination->rt_hash != hash)
        {
            continue;
        }

        // search address in the routing table entry
        if (Address_IsSameAddress(&destination->rt_dst, &dst))
        {
            return destination;
        }
    }
    return NULL;
}

// /**
// FUNCTION   :: OlsrDeleteRoutingTable
// LAYER      :: APPLICATION
// PURPOSE    :: Deletes the entry from routing table
// PARAMETERS ::
// + destination : rt_entry* : Pointer to entry
// RETURN :: void : NULL
// **/

static
void OlsrDeleteRoutingTable(
    rt_entry* destination)
{
    OlsrRemoveList((olsr_qelem *)destination);
    MEM_free((void *)destination);
}

// /**
// FUNCTION   :: OlsrReleaseRoutingTable
// LAYER      :: APPLICATION
// PURPOSE    :: Releases the routing table
// PARAMETERS ::
// + table : rthash* : Pointer to routing table
// RETURN :: void : NULL
// **/

static
void OlsrReleaseRoutingTable(
    rthash* table)
{
    rt_entry* destination;
    rt_entry* destination_tmp;
    rthash* routing_hash;

    unsigned char index;

    for (index = 0; index < HASHSIZE; index++)
    {
        routing_hash = &table[index];
        destination = routing_hash->rt_forw;
        while (destination != (rt_entry *) routing_hash)
        {
            destination_tmp = destination;
            destination = destination->rt_forw;

            // deletes each routing entry from the table
            OlsrDeleteRoutingTable(destination_tmp);
        }
    }
}

// /**
// FUNCTION   :: OlsrInsertRoutingTable
// LAYER      :: APPLICATION
// PURPOSE    :: Inserts the address in the routing table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + dst : Address : address to be inserted
// RETURN :: rt_entry* : Pointer to entry added
// **/

static
rt_entry* OlsrInsertRoutingTable(
    Node* node,
    Address dst)
{
    rt_entry* new_route_entry;
    rthash* routing_hash;
    UInt32 hash;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrHashing(dst, &hash);
    routing_hash = &olsr->routingtable[hash % HASHMASK];

    new_route_entry = (rt_entry*) MEM_malloc(sizeof(rt_entry));
    memset(new_route_entry, 0, sizeof(rt_entry));

    new_route_entry->rt_hash = hash;

    new_route_entry->rt_dst = dst;

    // No timeout is needed since the routing table is calculated for each
    // network change

    new_route_entry->rt_metric = 0;

    OlsrInsertList((olsr_qelem* )new_route_entry,(olsr_qelem *)routing_hash);

    // We need this pointer to use it for the next routing calculation
    return new_route_entry;
}

// /**
// FUNCTION   :: OlsrInsertRoutingTablefromTopology
// LAYER      :: APPLICATION
// PURPOSE    :: Inserts the address in the routing table from topology
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + r_last : rt_entry* : Pointer to routing entry
// + dst : Address : destination address
// RETURN :: rt_entry* : pointer to the entry if added, else NULL
// **/

static rt_entry*
OlsrInsertRoutingTablefromTopology(
    Node* node,
    rt_entry* r_last,
    Address dst)
{
    rt_entry* new_route_entry = NULL;

    ERROR_Assert(r_last, "Invalid topology entry");

    if (DEBUG)
    {
        printf("Insertion from the topology\n");
    }



    if (DEBUG)
    {
        char addrString[MAX_STRING_LENGTH];
        char addrString1[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&dst,
                addrString);
        IO_ConvertIpAddressToString(&r_last->rt_router,
                addrString1);

        printf("Looking up connection to %s for dest %s\n",
                addrString1, addrString);
    }

    if (DEBUG)
    {
        OlsrPrintLinkSet(node);
    }

    // Lookup best link to get to the router
    link_entry* link = OlsrGetLinktoNeighbor(node, r_last->rt_router);

    if (link == NULL)
    {
        printf(" No link exists to looked up neighbor\n");
        return NULL;
    }

    // Only if a link exists to the neighbor, add the node to the routing
    // table.

    new_route_entry = OlsrInsertRoutingTable(node, dst);
    new_route_entry->rt_router = r_last->rt_router;
    new_route_entry->rt_metric = (UInt16) (r_last->rt_metric + 1);
    new_route_entry->rt_interface =  RoutingOlsrCheckMyIfaceAddress(
                                        node,
                                        link->local_iface_addr);
    if (DEBUG)
    {
        char addrString1[MAX_STRING_LENGTH];
        char addrString2[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&dst,
            addrString1);
        IO_ConvertIpAddressToString(&new_route_entry->rt_router,
            addrString2);

        printf("Updating Routing Table at node %d\n", node->nodeId);
        printf("Dst : %s Next Hop : %s Interface: %d Metric: %d\n",
            addrString1,
            addrString2,
            new_route_entry->rt_interface,
            new_route_entry->rt_metric);
    }

    if (dst.networkType == NETWORK_IPV6)
    {
        Ipv6UpdateForwardingTable(
            node,
            GetIPv6Address(dst),
            128,
            GetIPv6Address(new_route_entry->rt_router),
            new_route_entry->rt_interface,
            new_route_entry->rt_metric,
            ROUTING_PROTOCOL_OLSR_INRIA);

    }
    else

    {
        NetworkUpdateForwardingTable(
            node,
            GetIPv4Address(dst),
            0xffffffff,
            GetIPv4Address(new_route_entry->rt_router),
            new_route_entry->rt_interface,
            new_route_entry->rt_metric,
            ROUTING_PROTOCOL_OLSR_INRIA);
    }


    return new_route_entry;
}


// /**
// FUNCTION   :: OlsrIs2HopNeighborReachable
// LAYER      :: APPLICATION
// PURPOSE    :: Check if  a two hop neighbor is reachable through a i
//               1 hop neighbor with willingness != WILL NEVER
// PARAMETERS ::
// + neigh_2_list : neighbor_2_list_entry* : Pointer to two hop neighbor
//                                           entry
// RETURN :: bool : true if reachable, else false
// **/


static
bool OlsrIs2HopNeighborReachable(
    neighbor_2_list_entry* neigh_2_list)
{
    neighbor_list_entry* neighbors;

    neighbors = neigh_2_list->neighbor_2->neighbor_2_nblist;

    while (neighbors != NULL)
    {
        if ((neighbors->neighbor->neighbor_status != NOT_NEIGH)
             && (neighbors->neighbor->neighbor_willingness != WILL_NEVER))
        {
            return true;
        }

        neighbors = neighbors->neighbor_next;
    }

    return false;
}


// /**
// FUNCTION   :: OlsrFillRoutingTableWith2HopNeighbors
// LAYER      :: APPLICATION
// PURPOSE    :: Adds two hop neighbors and returns the entries in the list
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: destination_n * :  Pointer to list
// **/

static
destination_n* OlsrFillRoutingTableWith2HopNeighbors(
    Node* node)
{
    destination_n* list_destination_n = NULL;
    unsigned char index;
    neighbor_entry* neighbor;
    neighborhash_type* hash_neighbor;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    if (DEBUG)
    {
        printf("Browse for 2 hop neighbors\n");
    }

    for (index = 0; index < HASHSIZE; index++)
    {
        hash_neighbor = &olsr->neighbortable.neighborhash[index];

        for (neighbor = hash_neighbor->neighbor_forw;
            neighbor != (neighbor_entry *) hash_neighbor;
            neighbor = neighbor->neighbor_forw)
        {
            neighbor_2_list_entry* neigh_2_list;

            if (neighbor->neighbor_status != SYM)
            {
                continue;
            }

            neigh_2_list = neighbor->neighbor_2_list;
            while (neigh_2_list != NULL)
            {
                Address n2_addr;
                mid_address addrs;
                memset( &addrs, 0, sizeof(mid_address));
                mid_address* addrsp;

                n2_addr = neigh_2_list->neighbor_2->neighbor_2_addr;

                if (DEBUG)
                {
                    char addrString[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(&n2_addr,
                            addrString);

                    printf("Checking Node  %s\n",
                            addrString);
                }

                if (OlsrLookupRoutingTable(node, n2_addr))
                {
                    neigh_2_list = neigh_2_list->neighbor_2_next;
                    continue;
                }

                if (!OlsrIs2HopNeighborReachable(neigh_2_list))
                {
                    neigh_2_list = neigh_2_list->neighbor_2_next;
                    continue;
                }

                addrs.alias = n2_addr;
                addrs.next_alias = OlsrLookupMidAliases(node,n2_addr);
                addrsp = &addrs;

                while (addrsp!=NULL)
                {
                    link_entry* link = OlsrGetBestLinktoNeighbor(
                                           node,
                                           neighbor->neighbor_main_addr);

                    if (link && RoutingOlsrCheckMyIfaceAddress(node,
                                link->local_iface_addr) >= 0)
                    {
                        rt_entry* new_route_entry =
                            OlsrInsertRoutingTable(
                                node,
                                addrsp->alias);

                        new_route_entry->rt_router =
                            link->neighbor_iface_addr;

                        // The next router is the neighbor itself
                        new_route_entry->rt_metric = 2;

                        new_route_entry->rt_interface =
                             RoutingOlsrCheckMyIfaceAddress(
                                node,
                                link->local_iface_addr);

                        if (DEBUG)
                        {
                            char addrString1[MAX_STRING_LENGTH];
                            char addrString2[MAX_STRING_LENGTH];
                            IO_ConvertIpAddressToString(&addrsp->alias,
                                addrString1);
                            IO_ConvertIpAddressToString(
                                &link->neighbor_iface_addr,
                                addrString2);
                            printf("Updating Routing Table at node %d\n",
                                node->nodeId);
                            printf("Dst : %s Next Hop : %s Interface: %d"
                                " Metric: %d\n",
                                addrString1,
                                addrString2,
                                new_route_entry->rt_interface,
                                new_route_entry->rt_metric);
                        }

                        if (addrsp->alias.networkType == NETWORK_IPV6)
                        {
                            Ipv6UpdateForwardingTable(
                                node,
                                GetIPv6Address(addrsp->alias),
                                128,
                                GetIPv6Address(link->neighbor_iface_addr),
                                new_route_entry->rt_interface,
                                new_route_entry->rt_metric,
                                ROUTING_PROTOCOL_OLSR_INRIA);
                        }
                        else

                        {
                            NetworkUpdateForwardingTable(
                                node,
                                GetIPv4Address(addrsp->alias),
                                0xffffffff,
                                GetIPv4Address(link->neighbor_iface_addr),
                                new_route_entry->rt_interface,
                                new_route_entry->rt_metric,
                                ROUTING_PROTOCOL_OLSR_INRIA);
                        }
                        if (new_route_entry != NULL)
                        {
                            destination_n* list_destination_tmp;
                            list_destination_tmp = (destination_n* )
                                MEM_malloc(sizeof(destination_n));

                            memset(
                                list_destination_tmp,
                                0,
                                sizeof(destination_n));

                            list_destination_tmp->destination =
                                new_route_entry;
                            list_destination_tmp->next = list_destination_n;
                            list_destination_n = list_destination_tmp;
                        }
                    }

                    addrsp = addrsp->next_alias;
                }

                neigh_2_list = neigh_2_list->neighbor_2_next;
            }
        }
    }
    return list_destination_n;
}

// /**
// FUNCTION   :: OlsrFillRoutingTableWithNeighbors
// LAYER      :: APPLICATION
// PURPOSE    :: Fills routing table with 1 hop neighbors
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: Int32 :  1
// **/

static
Int32 OlsrFillRoutingTableWithNeighbors(Node *node)
{
    int index;
    neighbor_entry* neighbor;
    neighborhash_type* hash_neighbor;
    rt_entry* new_route_entry = NULL;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    for (index = 0; index < HASHSIZE; index++)
    {
        hash_neighbor = &olsr->neighbortable.neighborhash[index];

        for (neighbor = hash_neighbor->neighbor_forw;
            neighbor != (neighbor_entry *) hash_neighbor;
            neighbor = neighbor->neighbor_forw)
        {
            if (neighbor->neighbor_status == SYM)
            {
                mid_address  addrs;
                memset( &addrs, 0, sizeof(mid_address));
                mid_address* addrs2;

                if (DEBUG)
                {
                    OlsrPrintMidTable(node);
                }
                addrs.alias = neighbor->neighbor_main_addr;
                addrs.next_alias = OlsrLookupMidAliases(
                                       node,
                                       neighbor->neighbor_main_addr);

                addrs2 = &addrs;

                if (DEBUG)
                {
                    printf(" List of aliases to be added\n");
                    mid_address* tmp_addr;
                    tmp_addr = &addrs;

                    while (tmp_addr != NULL)
                    {
                        char addrString[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(&tmp_addr->alias,
                                    addrString);
                        printf(" Alias: %s\n", addrString);

                        tmp_addr = tmp_addr->next_alias;
                    }
                }

                while (addrs2 != NULL)
                {
                    link_entry* link = OlsrGetBestLinktoNeighbor(
                                           node, addrs2->alias);

                    if (link && RoutingOlsrCheckMyIfaceAddress(node,
                                link->local_iface_addr) >= 0)
                    {
                        // inserts each neighbor in the routing table
                        new_route_entry = OlsrInsertRoutingTable(
                                              node,
                                              addrs2->alias);

                        new_route_entry->rt_router =
                            link->neighbor_iface_addr;

                        // The next router is the neighbor itself
                        new_route_entry->rt_metric = 1;

                        new_route_entry->rt_interface =
                            RoutingOlsrCheckMyIfaceAddress(
                                node,
                                link->local_iface_addr);

                        if (DEBUG)
                        {
                            char addrString1[MAX_STRING_LENGTH];
                            char addrString2[MAX_STRING_LENGTH];
                            IO_ConvertIpAddressToString(
                                &addrs2->alias,
                                addrString1);
                            IO_ConvertIpAddressToString(
                                &link->neighbor_iface_addr,
                                addrString2);
                            printf("Updating Routing Table at node %d\n",
                                node->nodeId);
                            printf("Dst : %s Next Hop : %s Interface: %d"
                                " Metric: %d\n",
                                 addrString1, addrString2,
                                 new_route_entry->rt_interface,
                                 new_route_entry->rt_metric);
                        }

                        if (addrs2->alias.networkType == NETWORK_IPV6)
                        {
                            Ipv6UpdateForwardingTable(
                                node,
                                GetIPv6Address(addrs2->alias),
                                128,
                                GetIPv6Address(link->neighbor_iface_addr),
                                new_route_entry->rt_interface,
                                new_route_entry->rt_metric,
                                ROUTING_PROTOCOL_OLSR_INRIA);
                        }
                        else

                        {
                            NetworkUpdateForwardingTable(
                                node,
                                GetIPv4Address(addrs2->alias),
                                0xffffffff,
                                GetIPv4Address(link->neighbor_iface_addr),
                                new_route_entry->rt_interface,
                                new_route_entry->rt_metric,
                                ROUTING_PROTOCOL_OLSR_INRIA);
                        }
                    }
                    addrs2 = addrs2->next_alias;
                }
            }
        }
    }
    return 1 ;
}

// /**
// FUNCTION   :: OlsrInsertRoutingTableFromHnaTable
// LAYER      :: APPLICATION
// PURPOSE    :: Fills routing table from HNA table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void :  NULL
// **/

static
void OlsrInsertRoutingTableFromHnaTable(Node* node)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    hna_entry* tmp_hna;
    hna_net* tmp_net;
    rt_entry *tmp_rt, *route_entry;

    unsigned char index;
    for (index = 0; index < HASHSIZE; index++)
    {
        // ALL hna entry
        for (tmp_hna = olsr->hna_table[index].next;
            tmp_hna != &olsr->hna_table[index];
            tmp_hna = tmp_hna->next)
        {
            // All networks
            for (tmp_net = tmp_hna->networks.next;
                 tmp_net != &tmp_hna->networks;
                 tmp_net = tmp_net->next)
            {

                // If no route to gateway - skip
                if ((tmp_rt = OlsrLookupRoutingTable(node,
                                tmp_hna->A_gateway_addr)) == NULL)
                {
                   continue;
                }
                if ((route_entry = OlsrLookupRoutingTable(node,
                                tmp_net->A_network_addr)) != NULL)
                {
                    // If there exists a better or equal entry - skip
                    if (route_entry->rt_metric > tmp_rt->rt_metric + 1)
                    {
                        // Replace old route with this better one
                        route_entry->rt_router = tmp_rt->rt_router;
                        route_entry->rt_metric = (UInt16) (tmp_rt->rt_metric)
                                                 + 1;
                        route_entry->rt_interface = tmp_rt->rt_interface;
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    // If not - create a new one and
                    // add into route table

                    route_entry = OlsrInsertRoutingTable(node,
                                      tmp_net->A_network_addr);
                    route_entry->rt_router = tmp_rt->rt_router;
                    route_entry->rt_metric = (UInt16) (tmp_rt->rt_metric)
                                             + 1;
                    route_entry->rt_interface = tmp_rt->rt_interface;
                }

                if (DEBUG)
                {
                    char addrString1[MAX_STRING_LENGTH];
                    char addrString2[MAX_STRING_LENGTH];

                    IO_ConvertIpAddressToString(&tmp_net->A_network_addr,
                        addrString1);
                    IO_ConvertIpAddressToString(&route_entry->rt_router,
                        addrString2);

                    printf("Updating Routing Table at node %d\n",node->nodeId);
                    printf("Dst: %s Next Hop: %s Interface: %d Metric: %d\n",
                        addrString1,
                        addrString2,
                        route_entry->rt_interface,
                        route_entry->rt_metric);
                }

              if (tmp_net->A_network_addr.networkType == NETWORK_IPV6)
              {
                    in6_addr ipv6_addr_prefix;
                    in6_addr ipv6_addr = GetIPv6Address(
                                             tmp_net->A_network_addr);
                    Ipv6GetPrefix(&ipv6_addr,
                        &ipv6_addr_prefix, tmp_net->A_netmask.v6);

                    Ipv6UpdateForwardingTable(
                        node,
                        ipv6_addr_prefix,
                        tmp_net->A_netmask.v6,
                        GetIPv6Address(route_entry->rt_router),
                        route_entry->rt_interface,
                        route_entry->rt_metric,
                        ROUTING_PROTOCOL_OLSR_INRIA);
              }
              else

              {
                    if (tmp_net->A_netmask.v4)
                    {
                        NetworkUpdateForwardingTable(
                                node,
                                GetIPv4Address(tmp_net->A_network_addr)
                                    % tmp_net->A_netmask.v4,
                                tmp_net->A_netmask.v4,
                                GetIPv4Address(route_entry->rt_router),
                                route_entry->rt_interface,
                                route_entry->rt_metric,
                                ROUTING_PROTOCOL_OLSR_INRIA);
                    }
                    else
                    {
                        NetworkUpdateForwardingTable(
                                node,
                                GetIPv4Address(tmp_net->A_network_addr),
                                tmp_net->A_netmask.v4,
                                GetIPv4Address(route_entry->rt_router),
                                route_entry->rt_interface,
                                route_entry->rt_metric,
                                ROUTING_PROTOCOL_OLSR_INRIA);
                    }

              }
          }
       }
   }
}

// /**
// FUNCTION   :: OlsrRoutingMirror
// LAYER      :: APPLICATION
// PURPOSE    :: Make mirror of current routing table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrRoutingMirror(
    Node* node)
{
    Int32 index;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    for (index = 0; index < HASHSIZE; index++)
    {
        if (olsr->routingtable[index].rt_forw ==
            (rt_entry *) &olsr->routingtable[index])
        {
            olsr->mirror_table[index].rt_forw = (rt_entry *)
                    &olsr->mirror_table[index];

            olsr->mirror_table[index].rt_back = (rt_entry *)
                    &olsr->mirror_table[index];
        }
        else
        {
            olsr->mirror_table[index].rt_forw =
                    olsr->routingtable[index].rt_forw;

            olsr->mirror_table[index].rt_forw->rt_back =
                    (rt_entry *) &olsr->mirror_table[index];

            olsr->mirror_table[index].rt_back =
                    olsr->routingtable[index].rt_back;

            olsr->mirror_table[index].rt_back->rt_forw =
                    (rt_entry *) &olsr->mirror_table[index];

            olsr->routingtable[index].rt_forw =
                    (rt_entry *) &olsr->routingtable[index];

            olsr->routingtable[index].rt_back =
                    (rt_entry *) &olsr->routingtable[index];
        }
    }
}

// /**
// FUNCTION   :: OlsrPrintRoutingTable
// LAYER      :: APPLICATION
// PURPOSE    :: Prints routing table
// PARAMETERS ::
// + table : rthash* : Pointer to Table to be printed
// RETURN :: void : NULL
// **/

static
void OlsrPrintRoutingTable(
    rthash* table)
{
    rt_entry* destination;
    rthash* routing_hash;
    int index;

    printf("Routing Table:\n");
    printf("Dest            NextHop           Intf Distance\n");

    for (index = 0; index < HASHSIZE; index++)
    {
        routing_hash = &table[index];
        for (destination = routing_hash->rt_forw;
            destination != (rt_entry *) routing_hash;
            destination = destination->rt_forw)
        {
            char addrString1[MAX_STRING_LENGTH];
            char addrString2[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(&destination->rt_dst,
                                        addrString1);

            IO_ConvertIpAddressToString(&destination->rt_router,
                                        addrString2);

            printf("%-15s %-15s   %d    %d\n",addrString1,
                    addrString2, destination->rt_interface,
                            destination->rt_metric);
        }
    }
}

// /**
// FUNCTION   :: OlsrCalculateRoutingTable
// LAYER      :: APPLICATION
// PURPOSE    :: Calculate routing table
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrCalculateRoutingTable(Node *node)
{
    destination_n* list_destination_n = NULL;
    destination_n* list_destination_n_1 = NULL;
    topology_last_entry* topo_last;
    destination_list* topo_dest;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    // RFC 3626 Section 10
    // Move routing table
    OlsrRoutingMirror(node);

    if (DEBUG)
    {
        printf(" Node %d : Printing Routing Table\n", node->nodeId);

        OlsrPrintRoutingTable(olsr->routingtable);

        printf(" Node %d : Printing Mirror Routing Table\n", node->nodeId);

        OlsrPrintRoutingTable(olsr->mirror_table);
    }

    // Step 1 Delete old entries from the table
    // empty previous routing table before creating a new one

    if (DEBUG)
    {
        printf("Empty IP forwarding table\n");
    }

    NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_OLSR_INRIA);

    // Step 2 Add One hop neighbors
    if (DEBUG)
    {
        printf("Fill table with neighbors\n");
    }

    OlsrFillRoutingTableWithNeighbors(node);

    if (DEBUG)
    {
        printf("Fill table with two hop neighbors\n");
    }

    // Step 3 Add two hop neighbors
    list_destination_n_1 = OlsrFillRoutingTableWith2HopNeighbors(node);

    // Step 3 Add the remaining destinations looking up the topology table
    list_destination_n = list_destination_n_1;

    while (list_destination_n_1 != NULL)
    {
        list_destination_n_1 = NULL;

        while (list_destination_n != NULL)
        {
            destination_n* destination_n_1 = NULL;

            if (DEBUG)
            {
                char addrString[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(
                    &list_destination_n->destination->rt_dst,
                    addrString);

                printf("Node %d, Last hop %s\n",
                    node->nodeId,
                    addrString);
            }

            if (NULL != (topo_last = OlsrLookupLastTopologyTable(
                node, list_destination_n->destination->rt_dst)))
            {
                topo_dest = topo_last->topology_list_of_destinations;
                while (topo_dest != NULL)
                {

                    mid_address tmp_addrs;
                    memset( &tmp_addrs, 0, sizeof(mid_address));
                    mid_address* tmp_addrsp;

                    if (RoutingOlsrCheckMyIfaceAddress(
                            node,
                            topo_dest->destination_node->topology_destination_dst) != -1)
                    {
                        topo_dest = topo_dest->next;
                        continue;
                    }

                    tmp_addrs.alias = topo_dest->destination_node->
                                          topology_destination_dst;
                    tmp_addrs.next_alias = OlsrLookupMidAliases(node,
                                               topo_dest->destination_node->
                                                   topology_destination_dst);
                    tmp_addrsp = &tmp_addrs;

                    while (tmp_addrsp != NULL)
                    {
                        // topo_dest is not in the routing table
                        if (NULL == OlsrLookupRoutingTable(node,
                             tmp_addrsp->alias))
                        {
                            // PRINT OUT: Last Hop to Final Destination. The
                            // function convert_address_to_string has to be
                            // seperately

                            if (DEBUG)
                            {
                                char addrString[MAX_STRING_LENGTH];
                                IO_ConvertIpAddressToString(
                                    &list_destination_n->destination->rt_dst,
                                    addrString);

                                printf("%s -> ", addrString);

                                IO_ConvertIpAddressToString(
                                    &topo_dest->destination_node->
                                        topology_destination_dst,
                                    addrString);

                                printf("%s\n", addrString);
                            }

                            destination_n_1 = (destination_n *)
                                MEM_malloc(sizeof(destination_n));

                            memset(destination_n_1, 0, sizeof(destination_n));

                            // changed by SRD for multiple iface problem
                            rt_entry* tmp =
                                OlsrInsertRoutingTablefromTopology(
                                    node,
                                    list_destination_n->destination,
                                    tmp_addrsp->alias);

                            // insert in routing table using topology
                            if (tmp != NULL)
                            {
                                destination_n_1->destination = tmp;
                            }
                            else
                            {
                                MEM_free(destination_n_1);
                                destination_n_1 = NULL;
                            }

                            if (destination_n_1!=NULL)
                            {
                                destination_n_1->next = list_destination_n_1;
                                list_destination_n_1 = destination_n_1;
                            }
                        }
                        tmp_addrsp = tmp_addrsp->next_alias;
                    }

                    topo_dest = topo_dest->next;
                }
            }

            destination_n_1 = list_destination_n;
            list_destination_n = list_destination_n->next;
            MEM_free(destination_n_1);
        }

        list_destination_n = list_destination_n_1;
    }

    if (DEBUG)
    {
        printf(" Node %d : Printing Routing Table\n", node->nodeId);
        OlsrPrintRoutingTable(olsr->routingtable);

        printf(" Node %d : Printing Mirror Routing Table\n", node->nodeId);
        OlsrPrintRoutingTable(olsr->mirror_table);
    }
    OlsrInsertRoutingTableFromHnaTable(node);
    OlsrReleaseRoutingTable(olsr->mirror_table);
}



/***************************************************************************
 *                    Definition of general functions                      *
 ***************************************************************************/

// /**
// FUNCTION   :: OlsrProcessChanges
// LAYER      :: APPLICATION
// PURPOSE    :: This function processes for changes
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
Int32 OlsrProcessChanges(
    Node* node)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    if (DEBUG)
    {
        if (olsr->changes_neighborhood)
        {
            printf("Changes in neighborhood\n");
        }
        if (olsr->changes_topology)
        {
            printf("Changes in topology\n");
        }
    }

    if (olsr->changes_neighborhood)
    {
        // Calculate new mprs and routing table
        OlsrCalculateMpr(node);
        OlsrCalculateRoutingTable(node);
        olsr->changes_neighborhood = FALSE;
        olsr->changes_topology = FALSE;
        return 0;
    }

    if (olsr->changes_topology)
    {
        // calculate the routing table
        OlsrCalculateRoutingTable(node);
        olsr->changes_topology = FALSE;
    }
    return 0;
}


// /**
// FUNCTION   :: OlsrDeleteTimeoutEntries
// LAYER      :: APPLICATION
// PURPOSE    :: This function removes all tuples for all tables when timer
//               is expired
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrDeleteTimeoutEntries(
    Node* node)
{
    if (DEBUG)
    {
        printf("hold timer expired for node %d\n", node->nodeId);
    }

    OlsrTimeoutDuplicateTable(node);
    OlsrTimeoutMprSelectorTable(node);
    OlsrTimeoutNeighborhoodTables(node);
    OlsrTimeoutTopologyTable(node);
    OlsrTimeoutLinkEntry(node);

    OlsrProcessChanges(node);
}

// /**
// FUNCTION   :: OlsrReleaseTables
// LAYER      :: APPLICATION
// PURPOSE    :: This function releases all entries for all tables
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrReleaseTables(
    Node* node)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    OlsrReleaseDuplicateTable(node);
    OlsrReleaseMprSelectorTable(node);
    OlsrReleaseNeighborTable(node);

    OlsrReleaseRoutingTable(olsr->mirror_table);
    OlsrReleaseRoutingTable(olsr->routingtable);

    OlsrReleaseTopologyTable(node);
    OlsrRelease2HopNeighborTable(node);
}

// /**
// FUNCTION   :: OlsrInitTables
// LAYER      :: APPLICATION
// PURPOSE    :: This function initialzes all the tables
// PARAMETERS ::
// + olsr : RoutingOlsr* : Pointer to olsr data structure
// RETURN :: void : NULL
// **/

static
void OlsrInitTables(RoutingOlsr *olsr)
{
    unsigned char index;

    // Setting Link set to NULL
    olsr->link_set = NULL;

    olsr->neighbortable.neighborhash= (neighborhash_type*)
            MEM_malloc(HASHSIZE * sizeof(neighborhash_type));

    memset(olsr->neighbortable.neighborhash,
            0,
            HASHSIZE * sizeof(neighborhash_type));

    olsr->mprstable.mpr_selector_hash= (mpr_selector_hash_type*)
            MEM_malloc(HASHSIZE * sizeof(mpr_selector_hash_type));

    memset(
        olsr->mprstable.mpr_selector_hash,
        0,
        HASHSIZE * sizeof(mpr_selector_hash_type));

    olsr->midtable.mid_main_hash = (mid_main_hash_type*)
        MEM_malloc(HASHSIZE * sizeof(mid_main_hash_type));

    memset(olsr->midtable.mid_main_hash,
            0,
            HASHSIZE * sizeof(mid_main_hash_type));

    olsr->midtable.mid_alias_hash = (mid_alias_hash_type*)
        MEM_malloc(HASHSIZE * sizeof(mid_alias_hash_type));

    memset(olsr->midtable.mid_alias_hash,
            0,
            HASHSIZE * sizeof(mid_alias_hash_type));

    for (index = 0; index < HASHSIZE; index++)
    {
        olsr->neighbortable.neighborhash[index].neighbor_forw =
            (neighbor_entry *) &olsr->neighbortable.neighborhash[index];

        olsr->neighbortable.neighborhash[index].neighbor_back =
            (neighbor_entry *) &olsr->neighbortable.neighborhash[index];

        olsr->neighbortable.neighbor_mpr_seq = 0;

        olsr->neighbor2table[index].neighbor2_forw =
            (neighbor_2_entry *) &olsr->neighbor2table[index];

        olsr->neighbor2table[index].neighbor2_back =
            (neighbor_2_entry *) &olsr->neighbor2table[index];

        olsr->duplicatetable[index].duplicate_forw =
            (duplicate_entry *)&olsr->duplicatetable[index];

        olsr->duplicatetable[index].duplicate_back =
            (duplicate_entry *)&olsr->duplicatetable[index];

        olsr->mprstable.mpr_selector_hash[index].mpr_selector_forw =
            (mpr_selector_entry *) &olsr->mprstable.mpr_selector_hash[index];

        olsr->mprstable.mpr_selector_hash[index].mpr_selector_back =
            (mpr_selector_entry *) &olsr->mprstable.mpr_selector_hash[index];

        olsr->mprstable.ansn = 0;

        olsr->routingtable[index].rt_forw =
            (rt_entry *) &olsr->routingtable[index];

        olsr->routingtable[index].rt_back =
            (rt_entry *) &olsr->routingtable[index];

        olsr->mirror_table[index].rt_forw =
            (rt_entry *) &olsr->mirror_table[index];

        olsr->mirror_table[index].rt_back =
            (rt_entry *) &olsr->mirror_table[index];

        olsr->topologytable[index].topology_destination_forw =
            (topology_destination_entry *) &olsr->topologytable[index];

        olsr->topologytable[index].topology_destination_back =
            (topology_destination_entry *) &olsr->topologytable[index];

        olsr->topologylasttable[index].topology_last_forw =
            (topology_last_entry *) &olsr->topologylasttable[index];

        olsr->topologylasttable[index].topology_last_back =
            (topology_last_entry *) &olsr->topologylasttable[index];

        olsr->midtable.mid_main_hash[index].mid_forw =
            (mid_entry *)&olsr->midtable.mid_main_hash[index];
        olsr->midtable.mid_alias_hash[index].mid_forw =
            (mid_address *)&olsr->midtable.mid_alias_hash[index];

        olsr->midtable.mid_main_hash[index].mid_back =
            (mid_entry *)&olsr->midtable.mid_main_hash[index];
        olsr->midtable.mid_alias_hash[index].mid_back =
            (mid_address *)&olsr->midtable.mid_alias_hash[index];

        olsr->hna_table[index].next =
            (hna_entry *) &olsr->hna_table[index];

        olsr->hna_table[index].prev =
            (hna_entry *) &olsr->hna_table[index];

    }
}


// /**
// FUNCTION   :: RoutingOlsrCheckMyIfaceAddress
// LAYER      :: APPLICATION
// PURPOSE    :: Function checks if address is not one of the
//               current node's interface address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + addr : Address: Address to be checked
// RETURN :: Int32 : Returns interface index if match, else -1
// **/

Int32 RoutingOlsrCheckMyIfaceAddress(
    Node* node,
    Address addr)
{
    unsigned char index;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

     // added during ipv6 auto-configuration support
    if (olsr->ip_version == NETWORK_IPV6)
    {
        return Ipv6GetInterfaceIndexFromAddress(
                                    node,
                                    &addr.interfaceAddr.ipv6);
    }

    Address interfaceAddress;

    for (index = 0; index < node->numberInterfaces; index++)
    {
        if (NetworkIpGetInterfaceType(node, index) != olsr->ip_version
            && NetworkIpGetInterfaceType(node, index)!= NETWORK_DUAL)
        {
            continue;
        }

        if (olsr->ip_version == NETWORK_IPV6)
        {
            interfaceAddress.networkType = NETWORK_IPV6;
            Ipv6GetGlobalAggrAddress(
                    node,
                    index,
                    &interfaceAddress.interfaceAddr.ipv6);
        }
        else if (olsr->ip_version == NETWORK_IPV4)
        {
            interfaceAddress.networkType = NETWORK_IPV4;
            interfaceAddress.interfaceAddr.ipv4 =
                            NetworkIpGetInterfaceAddress(node,index);
        }
//#endif

        if (Address_IsSameAddress(&interfaceAddress, &addr))
        {
            return index;
        }
    }

    return -1;
}


// /**
// FUNCTION   :: OlsrProcessNeighborsInHelloMsg
// LAYER      :: APPLICATION
// PURPOSE    :: This function process neighbors in hello msg
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + neighbor: neighbor_entry* : Pointer to neighbor entry
// + message : hl_message* : hello message pointer
// RETURN :: void : NULL
// **/

static
void OlsrProcessNeighborsInHelloMsg(
    Node* node,
    neighbor_entry* neighbor,
    hl_message* message)
{
    Address neigh_addr;
    hello_neighbor* message_neighbors;
    neighbor_2_list_entry* two_hop_neighbor_yet;
    neighbor_2_entry* two_hop_neighbor;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    ERROR_Assert(neighbor, "Invalid neighbor entry");
    ERROR_Assert(message, "Invalid Hello message");

    message_neighbors = message->neighbors;

    while (message_neighbors != NULL)
    {
        if (RoutingOlsrCheckMyIfaceAddress(node, message_neighbors->address)
                                                                       == -1)
        {
            neigh_addr = OlsrMidLookupMainAddr(node,
                message_neighbors->address);

            if (!Address_IsAnyAddress(&neigh_addr))
            {
                message_neighbors->address = neigh_addr;
            }

            if ((message_neighbors->status == SYM_NEIGH)
                || (message_neighbors->status == MPR_NEIGH))
            {
                if ((two_hop_neighbor_yet =
                    OlsrLookupMyNeighbors(neighbor,
                        message_neighbors->address)) != NULL)
                {
                    // Updating the holding time for this neighbor
                    two_hop_neighbor_yet->neighbor_2_timer =
                            node->getNodeTime()
                            + (clocktype)(message->vtime * SECOND);

                    two_hop_neighbor = two_hop_neighbor_yet->neighbor_2;

                    if (DEBUG)
                    {
                        char addrString[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(
                            &message_neighbors->address,
                            addrString);

                        printf("%s > Update second hop neighbor time\n",
                                addrString);
                    }
                }
                else
                {
                    if (DEBUG)
                    {
                        char addrString[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(
                            &message_neighbors->address,
                            addrString);

                        printf("%s > A new second hop neighbor\n",
                                addrString);
                    }

                    // Looking if the node is already in two hop neigh table
                    if ((two_hop_neighbor =
                        OlsrLookup2HopNeighborTable(
                            node, message_neighbors->address)) == NULL)
                    {
                        if (DEBUG)
                        {
                          printf("Adding entry in two hop neighbor table\n");
                        }

                        olsr->changes_neighborhood = TRUE;
                        olsr->changes_topology = TRUE;
                        two_hop_neighbor = (neighbor_2_entry *)
                            MEM_malloc(sizeof(neighbor_2_entry));

                        memset(
                            two_hop_neighbor,
                            0,
                            sizeof(neighbor_2_entry));

                        two_hop_neighbor->neighbor_2_nblist = NULL;
                        two_hop_neighbor->neighbor_2_pointer = 0;

                        two_hop_neighbor->neighbor_2_addr =
                            message_neighbors->address;

                        OlsrInsert2HopNeighborTable(
                            node,
                            two_hop_neighbor);

                        OlsrLinkingThis2Entries(
                            node,
                            neighbor,
                            two_hop_neighbor,
                            (float)message->vtime);
                    }
                    else
                    {
                        olsr->changes_neighborhood = TRUE;
                        olsr->changes_topology = TRUE;

                        // linking to this two_hop_neighbor entry
                        OlsrLinkingThis2Entries(
                            node,
                            neighbor,
                            two_hop_neighbor,
                            (float)message->vtime);
                    }
                }
            }
        }
        message_neighbors = message_neighbors->next;
    }
}

// /**
// FUNCTION   :: OlsrPrintTables
// LAYER      :: APPLICATION
// PURPOSE    :: Prints all the tables
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void OlsrPrintTables(
    Node* node)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    printf("********************************* Node %d ********************"
            "**************\n", node->nodeId);

    // OlsrPrintDuplicateTable(node);
    OlsrPrintNeighborTable(node);
    OlsrPrintMprSelectorTable(node);
    //OlsrPrint2HopNeighborTable(node);
    OlsrPrintTopologyTable(node);

    // print MID table
    OlsrPrintMidTable(node);
    OlsrPrintHnaTable(node);

    OlsrPrintRoutingTable(olsr->routingtable);
}


// /**
// FUNCTION   :: OlsrCheckPacketSizeGreaterThanFragUnit
// LAYER      :: APPLICATION
// PURPOSE    :: Check weather the olsr packet size be greater than
// the allowed size.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + msg : olsrpacket* : Pointer to Olsr Packet
// + h_ipv6_addr : in6_addr* : Pointer to ipv6 address
// RETURN :: void : NULL
// **/
static
void OlsrCheckPacketSizeGreaterThanFragUnit(Node* node,
                                            olsrpacket* msg,
                                            in6_addr* ipv6_addr,
                                            int interfaceIndex)
{
    Int32 outputsize;
    Int32 allowedSize;

    char errStr[MAX_STRING_LENGTH];

    outputsize = (Int32)(((char *)ipv6_addr - (char *)msg) +
                                                           sizeof(in6_addr));

    allowedSize = GetNetworkIPFragUnit(node, interfaceIndex) -
                    ( sizeof(ip6_hdr) + sizeof(TransportUdpHeader));

    if (outputsize > MAXPACKETSIZE)
    {
        sprintf(errStr, "Packet size increases from the defined"
                        " MAXPACKETSIZE %d. \n",MAXPACKETSIZE);
        ERROR_Assert(FALSE,errStr);
    }
    else if (outputsize > (allowedSize))
    {
        sprintf(errStr, "Packet size above %d"
                        " is currently not supported.\n",allowedSize);
        ERROR_Assert(FALSE,errStr);
    }
}


// /**
// FUNCTION   :: OlsrCheckPacketSizeGreaterThanFragUnit
// LAYER      :: APPLICATION
// PURPOSE    :: Check weather the olsr packet size be greater than
// the allowed size.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + msg : olsrpacket* : Pointer to Olsr Packet
// + h_ipv4_addr : NodeAddress* : Pointer to ipv4 address
// RETURN :: void : NULL
// **/
static
void OlsrCheckPacketSizeGreaterThanFragUnit(Node* node,
                                            olsrpacket* msg,
                                            NodeAddress* ipv4_addr,
                                            int interfaceIndex)
{
    Int32 outputsize;
    Int32 allowedSize;

    char errStr[MAX_STRING_LENGTH];

    outputsize = (Int32)(((char *)ipv4_addr - (char *)msg) +
                                                        sizeof(NodeAddress));

    allowedSize = GetNetworkIPFragUnit(node, interfaceIndex) -
                    ( sizeof(IpHeaderType) + sizeof(TransportUdpHeader));

    if (outputsize > MAXPACKETSIZE)
    {
        sprintf(errStr, "Packet size increases from the defined"
                        " MAXPACKETSIZE %d. \n",MAXPACKETSIZE);
        ERROR_Assert(FALSE,errStr);
    }
    else if (outputsize > (allowedSize))
    {
        sprintf(errStr, "Packet size above %d"
                        " is currently not supported.\n",allowedSize);
        ERROR_Assert(FALSE,errStr);
    }
}


// /**
// FUNCTION   :: OlsrSendHello
// LAYER      :: APPLICATION
// PURPOSE    :: Generate HELLO packet with the contents of the parameter
//              "message". Can generate an empty HELLO packet if the
//              neighbor table is empty.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message : hl_message* : hello message to be inserted in packet
// + interfaceIndex : Int32 : Interface on which packet is to be transmitted
// RETURN :: void : NULL
// **/

static
void OlsrSendHello(
    Node* node,
    hl_message *message,
    Int32 interfaceIndex)
{
    // TBD: In cases where message size > max packet size
    // divide the packet and send it in parts..
    // Skipping this for now

    char packet [MAXPACKETSIZE + 1];
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    Address main_address = olsr->main_address;

    olsrpacket* msg = (olsrpacket *)packet;
    olsrmsg* m = (olsrmsg *) msg->olsr_msg;

    hellomsg* h;
    hellinfo* hinfo;

    NodeAddress* h_ipv4_addr = NULL;
    in6_addr* h_ipv6_addr = NULL;

    hello_neighbor* nb = NULL;
    hello_neighbor* prev_nb = NULL;
    bool first_entry;
    Int32 outputsize;
    Int32 i;
    Int32 j;


    if (olsr->ip_version == NETWORK_IPV6)
    {
        h = m->olsr_ipv6_hello;
        hinfo = h->hell_info;
        h_ipv6_addr = (in6_addr *)hinfo->neigh_addr;
        outputsize = (Int32)((char *)h_ipv6_addr - (char *)packet);
        hinfo->link_msg_size = (UInt16) ((char *)h_ipv6_addr
                                   - (char *)hinfo);
        m->originator_ipv6_address = GetIPv6Address(main_address);
        m->olsr_msg_tail.olsr_ipv6_msg.ttl = message->ttl;
        m->olsr_msg_tail.olsr_ipv6_msg.hop_count = 0;
        m->olsr_msg_tail.olsr_ipv6_msg.msg_seq_no = OlsrGetMsgSeqNo(node);
    }
    else

    {
        h = m->olsr_ipv4_hello;
        hinfo = h->hell_info;
        h_ipv4_addr = (NodeAddress *)hinfo->neigh_addr;
        outputsize = (Int32)((char *)h_ipv4_addr - (char *)packet);
        hinfo->link_msg_size = (UInt16) ((char *)h_ipv4_addr
                                   - (char *)hinfo);
        m->originator_ipv4_address = GetIPv4Address(main_address);
        m->olsr_msg_tail.olsr_ipv4_msg.ttl = message->ttl;
        m->olsr_msg_tail.olsr_ipv4_msg.hop_count = 0;
        m->olsr_msg_tail.olsr_ipv4_msg.msg_seq_no = OlsrGetMsgSeqNo(node);
    }
    if (!message)
    {
        if (DEBUG)
        {
            printf("OlsrSendHello: NULL msg return.\n");
        }
        return;
    }

    if (DEBUG)
    {
        char addrString[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&main_address,
                addrString);

        char strTime[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), strTime);
        printf("Node %d generates HELLO message with originator %s"
            " on interface %d at %s\n",
            node->nodeId, addrString, interfaceIndex, strTime);

        OlsrPrintHelloMessage(message);
    }
    olsr->olsrStat.numHelloSent++;

    m->olsr_msgtype = HELLO_PACKET;

    // For hello packet vtime is set to olsr->neighb_hold_time
    m->olsr_vtime = OlsrDouble2ME((Float32)(olsr->neighb_hold_time
                                                / SECOND));
    h->willingness = message->willingness;

    // Set htime to HELLO INTERVAL
    h->htime = OlsrDouble2ME((Float32)(olsr->hello_interval / SECOND));
    memset(&h->reserved, 0, sizeof(UInt16));

    for (i = 0; i <= MAX_NEIGH; i++)
    {
        for (j=0; j <= MAX_LINK;j++)
        {
            first_entry = true;
            for (nb = message->neighbors; nb != NULL; nb = nb->next)
            {

                if ((nb->status != i)||(nb->link != j))
                {
                    continue;
                }
                if (first_entry)
                {
                    memset(&hinfo->res, 0, sizeof(unsigned char));
                    hinfo->link_code = (unsigned char)CREATE_LINK_CODE(i,j);
                }

                if (olsr->ip_version == NETWORK_IPV6)
                {
                    //checking packet size
                    OlsrCheckPacketSizeGreaterThanFragUnit(node, msg,
                                                           h_ipv6_addr,
                                                           interfaceIndex);

                    *h_ipv6_addr = GetIPv6Address(nb->address);
                    h_ipv6_addr++;
                }
                else

                {
                    //checking packet size
                    OlsrCheckPacketSizeGreaterThanFragUnit(node, msg,
                                                           h_ipv4_addr,
                                                           interfaceIndex);

                    *h_ipv4_addr = GetIPv4Address(nb->address);
                    h_ipv4_addr++;
                }
                first_entry = false;
            }

            if (!first_entry)
            {

                if (olsr->ip_version == NETWORK_IPV6)
                {
                    hinfo->link_msg_size = (UInt16) ((char *)h_ipv6_addr
                                               - (char *)hinfo);
                    if (DEBUG)
                    {
                            printf(" Link message size is %d\n",
                                hinfo->link_msg_size);
                    }
                    hinfo = (hellinfo *)((char *)h_ipv6_addr);
                    h_ipv6_addr = (in6_addr *)hinfo->neigh_addr;
                }
                else

                {
                    hinfo->link_msg_size = (UInt16) ((char *)h_ipv4_addr
                                               - (char *)hinfo);
                    if (DEBUG)
                    {
                            printf(" Link message size is %d\n",
                                hinfo->link_msg_size);
                    }

                    hinfo = (hellinfo *)((char *)h_ipv4_addr);
                    h_ipv4_addr = (NodeAddress *)hinfo->neigh_addr;
                }

            }
        } // for j
    } // for i


   m->olsr_msg_size = (UInt16) ((char *)hinfo - (char *)m);

   if (DEBUG)
   {
       printf (" Node %d: Hello Message size is %d\n",
           node->nodeId, m->olsr_msg_size);
   }

   outputsize = (Int32)((char *)hinfo - (char *)packet);

   if (DEBUG)
   {
       printf (" Node %d: Hello Packet  size is %d\n",
           node->nodeId, outputsize);
   }

    msg->olsr_packlen = (UInt16) (outputsize);

    // Delete the list of neighbor messages
    nb = message->neighbors;

    while (nb)
    {
        prev_nb = nb;
        nb = nb->next;
        MEM_free(prev_nb);
    }

    OlsrSend(node, msg, outputsize, MAX_HELLO_JITTER, interfaceIndex);
}

// /**
// FUNCTION   :: OlsrSendTc
// LAYER      :: APPLICATION
// PURPOSE    :: Generate TC packet with the contents of the parameter
//              "message".
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message : tc_message* : pointer to message which is to be inserted
//                           in packet
// + interfaceIndex : Int32 : interface to send the packet on
// RETURN :: void : NULL
// **/

static
void OlsrSendTc(Node *node, tc_message *message, Int32 interfaceIndex)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    char packet [MAXPACKETSIZE + 1];
    olsrpacket* msg = (olsrpacket *)packet;
    olsrmsg* m = msg->olsr_msg;
    tcmsg* tc = NULL;
    NodeAddress* mprs_ipv4_addr = NULL;
    in6_addr* mprs_ipv6_addr = NULL;

    tc_mpr_addr* mprs;
    tc_mpr_addr* prev_mprs;
    Int32 outputsize;

    if (olsr->ip_version == NETWORK_IPV6)
    {
        tc = m->olsr_ipv6_tc;
        mprs_ipv6_addr = (in6_addr *)tc->adv_neighbor_main_address;
    }
    else

    {
        tc = m->olsr_ipv4_tc;
        mprs_ipv4_addr = (NodeAddress *)tc->adv_neighbor_main_address;
    }

    if (!message)
    {
        if (DEBUG)
        {
            printf("OlsrSendTc: NULL msg return\n");
        }
        return;
    }

    olsr->olsrStat.numTcGenerated++;

    if (DEBUG)
    {
        char addrString[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&message->originator,
                addrString);

        char strTime[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), strTime);
        printf("Node %d generates TC message with originator %s"
            " on interface %d at %s\n",
            node->nodeId, addrString, interfaceIndex, strTime);

        OlsrPrintTcMessage(message);
    }

    for (mprs = message->multipoint_relay_selector_address;
        mprs != NULL; mprs = mprs->next)
    {

        if (olsr->ip_version == NETWORK_IPV6)
        {
            //checking packet size
            OlsrCheckPacketSizeGreaterThanFragUnit(node, msg,
                                                           mprs_ipv6_addr,
                                                           interfaceIndex);
            *mprs_ipv6_addr = GetIPv6Address(mprs->address);
            mprs_ipv6_addr++;
        }
        else

        {
            //checking packet size
            OlsrCheckPacketSizeGreaterThanFragUnit(node, msg,
                                                           mprs_ipv4_addr,
                                                           interfaceIndex);
            *mprs_ipv4_addr = GetIPv4Address(mprs->address);
            mprs_ipv4_addr++;
        }
    }

    if (olsr->ip_version == NETWORK_IPV6)
    {
        outputsize = (Int32)((char *)mprs_ipv6_addr - packet);
        m->olsr_msg_size = (UInt16) ((char *)mprs_ipv6_addr - (char *)m);
        m->originator_ipv6_address = GetIPv6Address(message->originator);

        m->olsr_msg_tail.olsr_ipv6_msg.msg_seq_no = OlsrGetMsgSeqNo(node);
        m->olsr_msg_tail.olsr_ipv6_msg.hop_count = message->hop_count;
        m->olsr_msg_tail.olsr_ipv6_msg.ttl = message->ttl;
    }
    else

    {
        outputsize = (Int32)((char *)mprs_ipv4_addr - packet);
        m->olsr_msg_size = (UInt16) ((char *)mprs_ipv4_addr - (char *)m);
        m->originator_ipv4_address = GetIPv4Address(message->originator);
        m->olsr_msg_tail.olsr_ipv4_msg.msg_seq_no = OlsrGetMsgSeqNo(node);
        m->olsr_msg_tail.olsr_ipv4_msg.hop_count = message->hop_count;
        m->olsr_msg_tail.olsr_ipv4_msg.ttl = message->ttl;
    }

   if (DEBUG)
   {
       printf ("\nNode %d: TC Packet  size is %d\n",
           node->nodeId, outputsize);
   }

    msg->olsr_packlen = (UInt16) (outputsize);

    // Set the TC vtime to olsr->top_hold_time
    m->olsr_vtime = OlsrDouble2ME((Float32)(olsr->top_hold_time
                                                / SECOND));
    m->olsr_msgtype = TC_PACKET;

    tc->ansn = message->ansn;

    memset(&tc->reserved, 0, sizeof(UInt16));

    // Delete the list of mprs messages
    mprs = message->multipoint_relay_selector_address;

    while (mprs)
    {
        prev_mprs = mprs;
        mprs = mprs->next;
        MEM_free(prev_mprs);
    }

    OlsrSend(node, msg, outputsize, MAX_TC_JITTER, interfaceIndex);
}


// /**
// FUNCTION   :: OlsrTcForward
// LAYER      :: APPLICATION
// PURPOSE    :: Generate and send TC packet with the contents of the
//               original message "m_forward" from a received OLSR packet.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + m_forward : olsrmsg* : TC message to be fwded
// RETURN :: void : NULL
// **/

static
void OlsrTcForward(
    Node* node,
    olsrmsg* m_forward)
{
    char packet [MAXPACKETSIZE + 1];
    olsrpacket* msg = (olsrpacket *)packet;
    olsrmsg* m = msg->olsr_msg;
    tcmsg* tc = NULL;
    tcmsg* OlsrTcForward = NULL;
    NodeAddress* mprs_ipv4_addr = NULL;
    in6_addr* mprs_ipv6_addr = NULL;
    NodeAddress* old_msg_mprs_ipv4_addr = NULL;
    in6_addr* old_msg_mprs_ipv6_addr = NULL;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    Int32 outputsize;


    if (olsr->ip_version == NETWORK_IPV6)
    {
        tc = m->olsr_ipv6_tc;
        OlsrTcForward = m_forward->olsr_ipv6_tc;
        mprs_ipv6_addr = (in6_addr *)tc->adv_neighbor_main_address;
        old_msg_mprs_ipv6_addr = (in6_addr *)OlsrTcForward->
                                                 adv_neighbor_main_address;
    }
    else

    {
        tc = m->olsr_ipv4_tc;
        OlsrTcForward = m_forward->olsr_ipv4_tc;
        mprs_ipv4_addr = (NodeAddress *)tc->adv_neighbor_main_address;
        old_msg_mprs_ipv4_addr = (NodeAddress *)OlsrTcForward->
                                                   adv_neighbor_main_address;
    }
    if (DEBUG)
    {
        printf(" Forwarding TC message \n");
    }

    if (m_forward == NULL)
    {
        return;
    }

    // Copy addresses from message to be forwarded

    if (olsr->ip_version == NETWORK_IPV6)
    {
        in6_addr* m_ipv6_addr = old_msg_mprs_ipv6_addr;
        while ((char *)m_ipv6_addr < (char *)m_forward
                                         + m_forward->olsr_msg_size)
        {
            *mprs_ipv6_addr = *m_ipv6_addr;
            mprs_ipv6_addr++;
            m_ipv6_addr++;
        }
        outputsize = (Int32)((char *)mprs_ipv6_addr - packet);
        m->olsr_msg_size = (UInt16) ((char *)mprs_ipv6_addr - (char *)m);
        m->originator_ipv6_address = m_forward->originator_ipv6_address;
        m->olsr_msg_tail.olsr_ipv6_msg.msg_seq_no =
            m_forward->olsr_msg_tail.olsr_ipv6_msg.msg_seq_no;
        m->olsr_msg_tail.olsr_ipv6_msg.hop_count =
            m_forward->olsr_msg_tail.olsr_ipv6_msg.hop_count;
        m->olsr_msg_tail.olsr_ipv6_msg.ttl =
            m_forward->olsr_msg_tail.olsr_ipv6_msg.ttl;
    }
    else

    {
        NodeAddress* m_ipv4_addr = old_msg_mprs_ipv4_addr;
        while ((char *)m_ipv4_addr < (char *)m_forward
                                         + m_forward->olsr_msg_size)
        {
            *mprs_ipv4_addr = *m_ipv4_addr;
            mprs_ipv4_addr++;
            m_ipv4_addr++;
        }
        outputsize = (Int32)((char *)mprs_ipv4_addr - packet);
        m->olsr_msg_size = (UInt16) ((char *)mprs_ipv4_addr - (char *)m);
        m->originator_ipv4_address = m_forward->originator_ipv4_address;
        m->olsr_msg_tail.olsr_ipv4_msg.msg_seq_no =
            m_forward->olsr_msg_tail.olsr_ipv4_msg.msg_seq_no;
        m->olsr_msg_tail.olsr_ipv4_msg.hop_count =
            m_forward->olsr_msg_tail.olsr_ipv4_msg.hop_count;
        m->olsr_msg_tail.olsr_ipv4_msg.ttl =
            m_forward->olsr_msg_tail.olsr_ipv4_msg.ttl;
    }

    msg->olsr_packlen = (UInt16) (outputsize);
    m->olsr_msgtype = TC_PACKET;
    m->olsr_vtime = OlsrDouble2ME((Float32)(olsr->top_hold_time
                                                / SECOND));

    tc->ansn = OlsrTcForward->ansn;
    tc->reserved = (UInt16) 0;

    RoutingOlsr* olsrData = (RoutingOlsr* ) node->appData.olsr;
    olsrData->olsrStat.numTcRelayed++;
    OlsrSendonAllInterfaces(node, msg, outputsize, MAX_TC_JITTER);
}
// /**
// FUNCTION   :: OlsrHnaForward
// LAYER      :: APPLICATION
// PURPOSE    :: Generate and send HNA packet with the contents of the
//               original message "m_forward" from a received OLSR packet.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + m_forward : olsrmsg* : HNA message to be fwded
// RETURN :: void : NULL
// **/

static
void OlsrHnaForward(
    Node* node,
    olsrmsg* m_forward)
{
    char packet[MAXPACKETSIZE + 1];
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    olsrpacket* olsr_pkt = (olsrpacket* )packet;
    olsrmsg* olsr_msg = (olsrmsg* ) olsr_pkt->olsr_msg;


    olsr->olsrStat.numHnaRelayed++;
    olsr_pkt->olsr_packlen = 2 * sizeof(UInt16) + m_forward->olsr_msg_size;

    memcpy(olsr_msg, m_forward, m_forward->olsr_msg_size);

    OlsrSendonAllInterfaces(node,
        olsr_pkt,
        olsr_pkt->olsr_packlen,
        MAX_HNA_JITTER);

    return;
}

// /**
// FUNCTION   :: OlsrUnknownMsgForward
// LAYER      :: APPLICATION
// PURPOSE    :: Forwarding the unknown message type with the contents of the
//               original message "m_forward" from a received OLSR packet.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + m_forward : olsrmsg* : unknown message to be fwded
// RETURN :: void : NULL
// **/
static
void OlsrUnknownMsgForward(
    Node* node,
    olsrmsg* m_forward)
{
    char packet[MAXPACKETSIZE + 1];
    olsrpacket* olsr_pkt = (olsrpacket* )packet;
    olsrmsg* olsr_msg = (olsrmsg* ) olsr_pkt->olsr_msg;

    olsr_pkt->olsr_packlen = 2 * sizeof(UInt16) + m_forward->olsr_msg_size;

    memcpy(olsr_msg, m_forward, m_forward->olsr_msg_size);

    OlsrSendonAllInterfaces(node,
        olsr_pkt,
        olsr_pkt->olsr_packlen,
        MAX_HNA_JITTER);

    return;
}

// /**
// FUNCTION   :: OlsrForwardMessage
// LAYER      :: APPLICATION
// PURPOSE    :: Check if a message is to be forwarded and forward
//               it if necessary.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + m : olsrmsg * : Pointer to message
// + originator: Address: originator of message
// + seqno : UInt16 : seq no of message
// + interfaceIndex : Int32: interface that packet arrived on
// + from_addr : Address: Neighbor Interface address that transmitted msg
// RETURN :: Int32 : 1 if msg is sent to be fwded, 0 if not
// **/

Int32
OlsrForwardMessage(
    Node* node,
    olsrmsg* m,
    Address originator,
    UInt16 seqno,
    Int32 interfaceIndex,
    Address from_addr)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    Address src = OlsrMidLookupMainAddr(node, from_addr);

    neighbor_entry* neighbor;
    Address ip_addr;
    ip_addr.networkType = NETWORK_IPV4;


    if (olsr->ip_version == NETWORK_IPV6)
    {
        ip_addr.networkType = NETWORK_IPV6;
        Ipv6GetGlobalAggrAddress(
                node,
                interfaceIndex,
                &ip_addr.interfaceAddr.ipv6);
    }
    else if (olsr->ip_version == NETWORK_IPV4)
    {
        ip_addr.networkType = NETWORK_IPV4;
        ip_addr.interfaceAddr.ipv4 =
                        NetworkIpGetInterfaceAddress(node,interfaceIndex);
    }
//#endif

    // Default forwarding algorithm
    // RFC 3626 Section 3.4.1

    // Lookup sender address

    if (Address_IsAnyAddress(&src))
    {
        src = from_addr;
    }

    // Condition 1
    if (NULL == (neighbor = OlsrLookupNeighborTable(node, src)))
    {

        return 0;
    }

    // Condition 1
    if (neighbor->neighbor_status != SYM)
    {
        return 0;
    }

    // Condition 2
    if (!OlsrLookupDuplicateTableFwd(node, originator, seqno, ip_addr))
    {
        return 0;
    }

    // Check if previous node is in the MPR selector set of the current
    // node

    // Condition 4
    unsigned char ttl;

    if (olsr->ip_version == NETWORK_IPV6)
    {
        ttl = m->olsr_msg_tail.olsr_ipv6_msg.ttl;
    }
    else
//#endif
    {
        ttl = m->olsr_msg_tail.olsr_ipv4_msg.ttl;
    }

    // Condition 5
    // Update duplicate table interface

    OlsrUpdateDuplicateEntry(node, originator, seqno, ip_addr);
    if (OlsrLookupMprSelectorTable(node,src) == NULL
       || ttl <= 1)
    {

        if (DEBUG)
        {
            printf(" Node is not the MPR of prev node,  don't forward\n");
        }
        return 0;
    }

    if (DEBUG)
    {
        printf(" Node is in the MPR of prev node, forward\n");
    }


    // Update dup forwarded

    OlsrSetDuplicateForward(node, originator, seqno);

    // Treat TTL hopcnt

    // Condition 6 and 7

    if (olsr->ip_version == NETWORK_IPV6)
    {
        m->olsr_msg_tail.olsr_ipv6_msg.hop_count++;
        m->olsr_msg_tail.olsr_ipv6_msg.ttl--;
    }
    else

    {
        m->olsr_msg_tail.olsr_ipv4_msg.hop_count++;
        m->olsr_msg_tail.olsr_ipv4_msg.ttl--;
    }


    // Depending upon type of message call OlsrTcForward or OlsrMidForward....

    if (m->olsr_msgtype == TC_PACKET)
    {
        OlsrTcForward(node, m);
        return 1;
    }
    else if (m->olsr_msgtype == MID_PACKET)
    {
        OlsrMidForward(node, m);
        return 1;
    }
    else if (m->olsr_msgtype == HNA_PACKET)
    {
        OlsrHnaForward(node, m);
        return 1;
    }

    else
    {
        OlsrUnknownMsgForward(node, m);
        return 0;
    }
}

// /**
// FUNCTION   :: OlsrHelloChgeStruct
// LAYER      :: APPLICATION
// PURPOSE    :: Build HELLO Message according to hello_message struct from
//               olsrmsg struct.
// PARAMETERS ::
// + node* : Node* : Pointer to Node
// + hmsg : hl_message* : Pointer to hl_message structure
// + m : olsrmsg* : Pointer to the olsrmsg
// RETURN :: void : NULL
// **/

static
void OlsrHelloChgeStruct(
    Node* node,
    hl_message* hmsg,
    olsrmsg* m)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    hellomsg* h;
    hellinfo* hinfo;
    hellinfo* hinf;
    NodeAddress* h_ipv4_addr;
    NodeAddress* h_ipv4_adr;
    in6_addr* h_ipv6_addr;
    in6_addr* h_ipv6_adr;
    hello_neighbor* nb;

    hmsg->neighbors = (hello_neighbor *)NULL;

    if (!m)
    {
        return;
    }

    if (m->olsr_msgtype != HELLO_PACKET)
    {
        return;
    }

    // parse olsr message and collects all information in hl_message

    if (olsr->ip_version == NETWORK_IPV6)
    {
        h = m->olsr_ipv6_hello;
        hinfo = h->hell_info;
        SetIPv6AddressInfo(&hmsg->source_addr, m->originator_ipv6_address);
        hmsg->packet_seq_number = m->olsr_msg_tail.olsr_ipv6_msg.msg_seq_no;
    }
    else

    {
        h = m->olsr_ipv4_hello;
        hinfo = h->hell_info;
        SetIPv4AddressInfo(&hmsg->source_addr, m->originator_ipv4_address);
        hmsg->packet_seq_number = m->olsr_msg_tail.olsr_ipv4_msg.msg_seq_no;
    }


     // Get vtime
    hmsg->vtime = OlsrME2Double(m->olsr_vtime);

    // Get htime
    hmsg->htime = OlsrME2Double(h->htime);

    // Willingness
    hmsg->willingness = h->willingness;

    if (DEBUG)
    {
        printf(" Hello message size is %d\n", m->olsr_msg_size);
    }

    for (hinf = hinfo; (char *)hinf < (char *)m + m->olsr_msg_size;
         hinf = (hellinfo *)((char *)hinf + hinf->link_msg_size))
    {
        if (DEBUG)
        {
            printf(" Link message size is %d\n", hinf->link_msg_size);
        }

        if (olsr->ip_version == NETWORK_IPV6)
        {
            h_ipv6_addr = (in6_addr *)hinf->neigh_addr;
            h_ipv6_adr = h_ipv6_addr;

            while ((char *)h_ipv6_adr < (char *)hinf + hinf->link_msg_size)
            {
                nb = (hello_neighbor *) MEM_malloc(sizeof(hello_neighbor));
                memset(nb, 0, sizeof(hello_neighbor));

                if (nb == 0)
                {
                    printf("olsrd: out of memery \n");
                    break;
                }

                SetIPv6AddressInfo(&nb->address, *h_ipv6_adr);
                nb->link = EXTRACT_LINK(hinf->link_code);
                nb->status = EXTRACT_STATUS(hinf->link_code);

                if (DEBUG)
                {
                    char addrString[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(&nb->address,
                    addrString);

                    printf(" Neighbor adress in hello is %s Link: %d ,"
                       " Status: %d \n", addrString, nb->link, nb->status);
                }

                nb->next = hmsg->neighbors;
                hmsg->neighbors = nb;

                h_ipv6_adr++;
            }

        }
        else

        {
            h_ipv4_addr = (NodeAddress *)hinf->neigh_addr;
            h_ipv4_adr = h_ipv4_addr;

            while ((char *)h_ipv4_adr < (char *)hinf + hinf->link_msg_size)
            {
                nb = (hello_neighbor *) MEM_malloc(sizeof(hello_neighbor));
                memset(nb, 0, sizeof(hello_neighbor));

                if (nb == 0)
                {
                    printf("olsrd: out of memery \n");
                    break;
                }

                SetIPv4AddressInfo(&nb->address, *h_ipv4_adr);
                nb->link = EXTRACT_LINK(hinf->link_code);
                nb->status = EXTRACT_STATUS(hinf->link_code);

                if (DEBUG)
                {
                    char addrString[MAX_STRING_LENGTH];
                    IO_ConvertIpAddressToString(&nb->address,
                    addrString);

                    printf(" Neighbor adress in hello is %s Link: %d ,"
                        " Status: %d \n", addrString, nb->link, nb->status);
                }

                nb->next = hmsg->neighbors;
                hmsg->neighbors = nb;

                h_ipv4_adr++;
            }
        }
    }
}

// /**
// FUNCTION   :: OlsrTcChgeStruct
// LAYER      :: APPLICATION
// PURPOSE    :: Build TC Message according to tc_message struct from
//               olsrmsg struct.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + tmsg : tc_message * : Pointer to the TC message
// + m : olsrmsg* : OLSR message pointer
// + from_addr: Address: Neighbor interface address that transmitted TC
// RETURN :: void : NULL
// **/

static
void OlsrTcChgeStruct(
    Node* node,
    tc_message* tmsg,
    olsrmsg* m,
    Address from_addr)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    tcmsg* tc;
    NodeAddress* mprs_ipv4_addr = NULL;
    in6_addr* mprs_ipv6_addr = NULL;
    NodeAddress* m_ipv4_addr;
    in6_addr* m_ipv6_addr;
    tc_mpr_addr* mprs;
    Address tmp_addr = OlsrMidLookupMainAddr(node, from_addr);

    tmsg->multipoint_relay_selector_address = (tc_mpr_addr *) NULL;

    if (!m)
    {
        return;
    }

    if (m->olsr_msgtype != TC_PACKET)
    {
        return;
    }

    // parse olsr message and collects all information in tc_message

    if (olsr->ip_version == NETWORK_IPV6)
    {
        tc = m->olsr_ipv6_tc;
        mprs_ipv6_addr = (in6_addr *)tc->adv_neighbor_main_address;
    }
    else

    {
        tc = m->olsr_ipv4_tc;
        mprs_ipv4_addr = (NodeAddress *)tc->adv_neighbor_main_address;
    }

    if (Address_IsAnyAddress(&tmp_addr))
    {
        tmsg->source_addr = from_addr;
    }
    else
    {
       tmsg->source_addr = tmp_addr;
    }

    tmsg->vtime = OlsrME2Double(m->olsr_vtime);
    tmsg->ansn = tc->ansn;

    if (olsr->ip_version == NETWORK_IPV6)
    {
        SetIPv6AddressInfo(&tmsg->originator, m->originator_ipv6_address);

        tmsg->packet_seq_number = m->olsr_msg_tail.olsr_ipv6_msg.msg_seq_no;
        tmsg->hop_count =  m->olsr_msg_tail.olsr_ipv6_msg.hop_count;

        m_ipv6_addr = mprs_ipv6_addr;
        while ((char *)m_ipv6_addr < (char *)m + m->olsr_msg_size)
        {
            mprs = (tc_mpr_addr *)MEM_malloc(sizeof(tc_mpr_addr));

            memset(mprs, 0, sizeof(tc_mpr_addr));

            if (mprs == 0)
            {
                printf("olsrd: out of memery \n");
                break;
            }

            SetIPv6AddressInfo(&mprs->address, *m_ipv6_addr);
            mprs->next = tmsg->multipoint_relay_selector_address;
            tmsg->multipoint_relay_selector_address = mprs;

            m_ipv6_addr++;
        }
    }
    else

    {
        SetIPv4AddressInfo(&tmsg->originator, m->originator_ipv4_address);
        tmsg->packet_seq_number = m->olsr_msg_tail.olsr_ipv4_msg.msg_seq_no;
        tmsg->hop_count =  m->olsr_msg_tail.olsr_ipv4_msg.hop_count;
        m_ipv4_addr = mprs_ipv4_addr;
        while ((char *)m_ipv4_addr < (char *)m + m->olsr_msg_size)
        {
            mprs = (tc_mpr_addr *)MEM_malloc(sizeof(tc_mpr_addr));

            memset(mprs, 0, sizeof(tc_mpr_addr));

            if (mprs == 0)
            {
                printf("olsrd: out of memery \n");
                break;
            }

            SetIPv4AddressInfo(&mprs->address, *m_ipv4_addr);
            mprs->next = tmsg->multipoint_relay_selector_address;
            tmsg->multipoint_relay_selector_address = mprs;

            m_ipv4_addr++;
        }
    }
}


/***************************************************************************
 *             Definition of message handling functions                    *
 ***************************************************************************/

// /**
// FUNCTION   :: OlsrBuildTcPacket
// LAYER      :: APPLICATION
// PURPOSE    :: Build tc message
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message : tc_message * : Pointer to TC message data structure
// RETURN :: Int32 : 0 if message to be sent, -1 if not
// **/

static
Int32 OlsrBuildTcPacket(
    Node* node,
    tc_message* message)
{
    tc_mpr_addr* message_mpr;
    mpr_selector_entry* mprs;
    mpr_selector_hash_type* mprs_hash;

    unsigned char index;
    bool empty_packet=true;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    Address addr_ip = olsr->main_address;


    message->multipoint_relay_selector_address = NULL;
    message->packet_seq_number = 0;

    message->hop_count = 0;
    message->ttl = MAX_TTL;

    message->ansn = olsr->mprstable.ansn;

    message->originator = addr_ip;
    message->source_addr = addr_ip;

    for (index = 0; index < HASHSIZE; index++)
    {
        mprs_hash = &olsr->mprstable.mpr_selector_hash[index];

        for (mprs = mprs_hash->mpr_selector_forw;
            mprs != (mpr_selector_entry *)mprs_hash;
            mprs = mprs->mpr_selector_forw)
        {
            message_mpr = (tc_mpr_addr *) MEM_malloc(sizeof(tc_mpr_addr));

            memset(message_mpr, 0, sizeof(tc_mpr_addr));

            message_mpr->address = mprs->mpr_selector_main_addr;
            message_mpr->next = message->multipoint_relay_selector_address;
            message->multipoint_relay_selector_address = message_mpr;
            empty_packet=false;
        }
    }

    // To handle the empty packet case..
    // Keep sending empty tc till vtime of previous TC message is valid
    // After that time, don't send till non empty set occurs

    if (empty_packet == true)
    {
        if (olsr->tc_valid_time > node->getNodeTime())
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        olsr->tc_valid_time = node->getNodeTime() + olsr->top_hold_time;
        return 0;
    }
}

// /**
// FUNCTION   :: OlsrGenerateTc
// LAYER      :: APPLICATION
// PURPOSE    :: Generate tc message
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interfaceIndex : Int32 : outgoing interface
// RETURN :: void : NULL
// **/

static
void OlsrGenerateTc(
    Node* node,
    Int32 interfaceIndex)
{
    tc_message tcpacket;
    Int32 send_message;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    // If this node is selected MPR by others, then only TC message is
    // generated, otherwise stop the generation of TC message. It will
    // be again activated when the node will be MPR again.

    if (OlsrExistMprSelectorTable(node))
    {

        // receive tc packet info from the node structure to tc_message
        // structure. If empty mpr selector set and validity time of prev
        // tc expired, don't send tc message

        send_message = OlsrBuildTcPacket(node, &tcpacket);

        if (send_message!=-1)
        {
            // printf("Generate Tc send_message\n");
               // prepare the tc message and send
            OlsrSendTc(node, &tcpacket, interfaceIndex);
        }
        else
        {
            if (DEBUG)
            {
                printf ("Validation time expires. ");
                printf ("Returning without any TC generation.\n");
            }
        }
    }
    else
    {
        if (DEBUG)
        {
            printf ("No entry in MPR Selection Table. ");
            printf ("Returning without any TC generation.\n");
        }
        // printf("Generate Tc tcMessageStarted = FALSE\n");
        olsr->tcMessageStarted = FALSE;
    }
}


// /**
// FUNCTION   :: OlsrLookupMprStatus
// LAYER      :: APPLICATION
// PURPOSE    :: Function looks up hello message if the current node
//               status is MPR or not
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message : hl_message *: Pointer to hello message
// + incomingInterface: Int32 : interface on which hello message arrived
// RETURN :: Int32 : 1  if node is MPR, 0 if not
// **/


static
Int32 OlsrLookupMprStatus(
    Node* node,
    hl_message* message,
    Int32 incomingInterface)
{
    hello_neighbor* neighbors;
    neighbors = message->neighbors;
    Address interfaceAddress;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;


    if (olsr->ip_version == NETWORK_IPV6)
    {
        interfaceAddress.networkType = NETWORK_IPV6;
        Ipv6GetGlobalAggrAddress(
                node,
                incomingInterface,
                &interfaceAddress.interfaceAddr.ipv6);
    }
    else if (olsr->ip_version == NETWORK_IPV4)
    {
        interfaceAddress.networkType = NETWORK_IPV4;
        interfaceAddress.interfaceAddr.ipv4 =
                        NetworkIpGetInterfaceAddress(node,incomingInterface);
    }
//#endif


    while (neighbors != NULL)
    {
        if (Address_IsSameAddress(&neighbors->address, &interfaceAddress))
        {
            if ((neighbors->link == SYM_LINK)
                 && (neighbors->status == MPR_NEIGH))
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
        neighbors = neighbors->next;
    }
    return 0;
}

// /**
// FUNCTION   :: OlsrProcessReceivedHello
// LAYER      :: APPLICATION
// PURPOSE    :: Process hello message
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message : hl_message * : Pointer to hello message
// + incomingInterface: Int32: interface at which packet arrived
// + from_addr: Address: neighbor interface address that sent packet
// RETURN :: Int32 : 1 if error
// **/

static
Int32 OlsrProcessReceivedHello(
    Node* node,
    hl_message* message,
    Int32 incomingInterface,
    Address from_addr)
{
    link_entry* link;
    neighbor_entry* neighbor;

    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    Address addr_ip;
    addr_ip.networkType = NETWORK_IPV4;

    if (olsr->ip_version == NETWORK_IPV6)
    {
        addr_ip.networkType = NETWORK_IPV6;
        Ipv6GetGlobalAggrAddress(
                node,
                incomingInterface,
                &addr_ip.interfaceAddr.ipv6);
    }
    else if (olsr->ip_version == NETWORK_IPV4)
    {
        addr_ip.networkType = NETWORK_IPV4;
        addr_ip.interfaceAddr.ipv4 =
                        NetworkIpGetInterfaceAddress(node,incomingInterface);
    }
//#endif

    if (DEBUG)
    {
        char addrString[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&from_addr,
                addrString);

        char strTime[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), strTime);
        printf("Node %d receives HELLO message from %s"
            " on interface %d at %s\n",
            node->nodeId, addrString, incomingInterface, strTime);

        OlsrPrintHelloMessage(message);
    }

    olsr->olsrStat.numHelloReceived++;

    if (message == NULL)
    {
        return 1;
    }


    // RFC 3626 Section 7.1.1

    // Update Link Status
    // OlsrPrintLinkSet(node);

    link = OlsrUpdateLinkEntry(node,
               addr_ip, from_addr, message, incomingInterface);

    neighbor = link->neighbor;

    // RFC 3626 Section 8.1.1

    // Update neighbor willingness if different

    if (neighbor->neighbor_willingness != message->willingness)
    {
          //If willingness changed - recalculate
          neighbor->neighbor_willingness = message->willingness;

          // This flag is set to UP to recalculate MPR set/routing table
          olsr->changes_neighborhood = TRUE;
          olsr->changes_topology = TRUE;

    }

    // RFC 3626 Section 8.2.1
    // Don't register neighbors of neighbors that announces WILL_NEVER

    if (neighbor->neighbor_willingness != WILL_NEVER)
    {
        // Processing the list of neighbors in the received message
        OlsrProcessNeighborsInHelloMsg(node, neighbor, message);
    }

    // RFC 3626 Section 8.4.1
    // Check if we are chosen as MPR

    if (OlsrLookupMprStatus(node, message, incomingInterface))
    {
        // source_addr is always the main addr of a node!
        OlsrUpdateMprSelectorTable(node,
            message->source_addr, (float)message->vtime);
    }

    // Process changes immedeatly in case of MPR updates
    OlsrDestroyHelloMessage(message);
    OlsrProcessChanges(node);

    if (DEBUG)
    {
        OlsrPrintNeighborTable(node);
        OlsrPrint2HopNeighborTable(node);
        OlsrPrintMprSelectorTable(node);
    }

    return 0;
}

// /**
// FUNCTION   :: OlsrProcessReceivedHna
// LAYER      :: APPLICATION
// PURPOSE    :: Process received hna msg
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message : hna_message* : Pointer to HNA msg
// + source_addr : Address : source address of the msg
// + incomingInterface : Int32 : Incoming Interface Index
// + m : olsrmsg* : Pointer to a generic olsr msg
// RETURN :: Int32 : Returns zero only if the hna msg is processed
// **/
static
Int32 OlsrProcessReceivedHna(
    Node* node,
    hna_message* message,
    Address source_addr,
    Int32 incomingInterface,
    olsrmsg* m)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    Address addr_ip;


    if (olsr->ip_version == NETWORK_IPV6)
    {
        addr_ip.networkType = NETWORK_IPV6;
        Ipv6GetGlobalAggrAddress(
                node,
                incomingInterface,
                &addr_ip.interfaceAddr.ipv6);
    }
    else if (olsr->ip_version == NETWORK_IPV4)
    {
        addr_ip.networkType = NETWORK_IPV4;
        addr_ip.interfaceAddr.ipv4 =
                        NetworkIpGetInterfaceAddress(node,incomingInterface);
    }
//#endif
    //neighbor_entry* neighbor;

    olsr->olsrStat.numHnaReceived++;

    if (DEBUG)
    {
        OlsrPrintHnaMessage(node, message);
    }

    // we should not check the originator address
    if (Address_IsSameAddress(&message->hna_origaddr, &addr_ip))
    {
        OlsrDestroyHnaMessage(message);
        return 1;
    }

    // Check whether the hna_msg is received from a sym neighbor or not
    if (OlsrCheckLinktoNeighbor(node, source_addr) != SYM_LINK)
    {
        // The hna_msg's source address is not in the neighbor table
        OlsrDestroyHnaMessage(message);
        return 1;
    }

    if (NULL != OlsrLookupDuplicateTableProc(node,
                    message->hna_origaddr, message->hna_seqno))
    {
        OlsrForwardMessage(node,
            m,
            message->hna_origaddr,
            message->hna_seqno,
            incomingInterface,
            source_addr);

        OlsrDestroyHnaMessage(message);
        return 1;
    }


    hna_net_addr*  hna_net_pair = message->hna_net_pair;

    while (hna_net_pair)
    {
        OlsrUpdateHnaEntry(node,
            message->hna_origaddr,
            hna_net_pair->net,
            hna_net_pair->netmask,
            message->vtime);

        hna_net_pair = hna_net_pair->next;
    }

    OlsrForwardMessage(node,
        m,
        message->hna_origaddr,
        message->hna_seqno,
        incomingInterface,
        source_addr);

    OlsrDestroyHnaMessage(message);

    // To allow routing table to incorporate HNA insertions
    OlsrProcessChanges(node);

    return 0;
}

// /**
// FUNCTION   :: OlsrHnaChgeStruct
// LAYER      :: APPLICATION
// PURPOSE    :: Parse an olsr message and collects all information
//               in HNA msg
// PARAMETERS ::
// + node : Node* : Pointer to Node
// + hna_msg : hna_message* : Pointer to HNA msg
// + m : olsrmsg* : Pointer to a generic olsr msg
// RETURN :: void : NULL
// **/
static
void OlsrHnaChgeStruct(
    Node* node,
    hna_message* hna_msg,
    olsrmsg* m)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    hnamsg* hna;
    struct _hnapair_ipv6* hna_pair_ipv6 = NULL;
    struct _hnapair_ipv4* hna_pair_ipv4 = NULL;

    if (!m)
    {
        return;
    }
    if (m->olsr_msgtype != HNA_PACKET)
    {
        return;
    }

    // parse olsr message and collects all information in mid_message

    if (olsr->ip_version == NETWORK_IPV6)
    {
        SetIPv6AddressInfo(&hna_msg->hna_origaddr,
            m->originator_ipv6_address);  // originator's address
        hna = m->olsr_ipv6_hna;
        hna_pair_ipv6 = (struct _hnapair_ipv6* )hna->hna_net_pair;
    }
    else

    {
        SetIPv4AddressInfo(&hna_msg->hna_origaddr,
            m->originator_ipv4_address);  // originator's address
        hna = m->olsr_ipv4_hna;
        hna_pair_ipv4 = (struct _hnapair_ipv4* )hna->hna_net_pair;
    }
    hna_msg->vtime = OlsrME2Double(m->olsr_vtime); // validity time
    hna_msg->hna_net_pair = NULL;  // alias address

    hna_net_addr* hna_pair_entry;

    if (olsr->ip_version == NETWORK_IPV6)
    {
        // number of hops to destination
        hna_msg->hna_hopcnt = m->olsr_msg_tail.olsr_ipv6_msg.hop_count;
        hna_msg->hna_ttl = MAX_TTL;       // ttl
        // sequence number
        hna_msg->hna_seqno = m->olsr_msg_tail.olsr_ipv6_msg.msg_seq_no;
        while ((char *)hna_pair_ipv6 < (char *)m + m->olsr_msg_size)
        {
            hna_pair_entry = (hna_net_addr* )
                                 MEM_malloc(sizeof(hna_net_addr));

            memset(hna_pair_entry, 0, sizeof(hna_net_addr));

            if (hna_pair_entry == NULL)
            {
                printf("olsrd: out of memery \n");
                break;
            }
            SetIPv6AddressInfo(&hna_pair_entry->net, hna_pair_ipv6->addr);
            hna_pair_entry->netmask.v6 = hna_pair_ipv6->netmask.s6_addr32[3];
            hna_pair_entry->next = hna_msg->hna_net_pair;
            hna_msg->hna_net_pair = hna_pair_entry;
            hna_pair_ipv6++;
        }
    }
    else

    {
        hna_msg->hna_hopcnt = m->olsr_msg_tail.olsr_ipv4_msg.hop_count;
        hna_msg->hna_ttl = MAX_TTL;       // ttl
        hna_msg->hna_seqno = m->olsr_msg_tail.olsr_ipv4_msg.msg_seq_no;
        while ((char *)hna_pair_ipv4 < (char *)m + m->olsr_msg_size)
        {
            hna_pair_entry = (hna_net_addr* )
                                 MEM_malloc(sizeof(hna_net_addr));

            memset(hna_pair_entry, 0, sizeof(hna_net_addr));

            if (hna_pair_entry == NULL)
            {
                printf("olsrd: out of memery \n");
                break;
            }
            SetIPv4AddressInfo(&hna_pair_entry->net, hna_pair_ipv4->addr);
            hna_pair_entry->netmask.v4 = hna_pair_ipv4->netmask;
            hna_pair_entry->next = hna_msg->hna_net_pair;
            hna_msg->hna_net_pair = hna_pair_entry;
            hna_pair_ipv4++;
        }
    }
    return;
}



// /**
// FUNCTION   :: OlsrProcessReceivedMid
// LAYER      :: APPLICATION
// PURPOSE    :: Process received mid msg
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message : mid_message* : Pointer to MID msg
// + source_addr : Address : source address of the msg
// + incomingInterface : Int32 : Incoming Interface Index
// + m : olsrmsg* : Pointer to a generic olsr msg
// RETURN :: Int32 : Returns zero only if the mid msg is processed
// **/
static
Int32 OlsrProcessReceivedMid(
    Node* node,
    mid_message* message,
    Address source_addr,
    Int32 incomingInterface,
    olsrmsg* m)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    Address addr_ip;


    if (olsr->ip_version == NETWORK_IPV6)
    {
        addr_ip.networkType = NETWORK_IPV6;
        Ipv6GetGlobalAggrAddress(
                node,
                incomingInterface,
                &addr_ip.interfaceAddr.ipv6);
    }
    else if (olsr->ip_version == NETWORK_IPV4)
    {
        addr_ip.networkType = NETWORK_IPV4;
        addr_ip.interfaceAddr.ipv4 =
                        NetworkIpGetInterfaceAddress(node,incomingInterface);
    }
//#endif

    olsr->olsrStat.numMidReceived++;

    if (message == NULL)
    {
        return 1;
    }

    if (DEBUG)
    {
        char addrString[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&source_addr,
                addrString);

        char strTime[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), strTime);
        printf("Node %d receives MID message from %s"
            " on interface %d at %s\n",
            node->nodeId, addrString, incomingInterface, strTime);

        OlsrPrintMidMessage(message);
    }

    // we should not check the originator address
    if (Address_IsSameAddress(&message->mid_origaddr, &addr_ip))
    {
        OlsrDestroyMidMessage(message);
        return 1;
    }

    // Check whether the mid_msg is received from a sym neighbor or not
    if (OlsrCheckLinktoNeighbor(node, source_addr) != SYM_LINK)
    {
        // The mid_message's source address is not in the neighbor table
        OlsrDestroyMidMessage(message);
        return 1;
    }

    if (NULL != OlsrLookupDuplicateTableProc(node,
                    message->mid_origaddr, message->mid_seqno))
    {
        OlsrForwardMessage(node,
            m,
            message->mid_origaddr,
            message->mid_seqno,
            incomingInterface,
            source_addr);

        OlsrDestroyMidMessage(message);
        return 1;
    }

    mid_alias* mid_addr = message->mid_addr;
    while (mid_addr)
    {
        Address main_addr = OlsrMidLookupMainAddr(node,
                                mid_addr->alias_addr);
        if (!Address_IsAnyAddress(&main_addr))
        {
            // already exists in the mid-table, so just update the
            // timer associated with mid_table_entry

            OlsrUpdateMidTable(node, main_addr, message->vtime);
        }
        else
        {
            OlsrInsertMidAlias(node,
                message->mid_origaddr,
                mid_addr->alias_addr,
                message->vtime);

            // We add a Mid entry,
            // should recalculate RT

            olsr->changes_topology = TRUE;
        }
        mid_addr = mid_addr->next;
    }

    OlsrForwardMessage(node,
        m,
        message->mid_origaddr,
        message->mid_seqno,
        incomingInterface,
        source_addr);

    OlsrDestroyMidMessage(message);

    // To allow routing table to incorporate MID insertions
    OlsrProcessChanges(node);

    return 0;
}
// /**
// FUNCTION   :: OlsrProcessReceivedTc
// LAYER      :: APPLICATION
// PURPOSE    :: Process tc message
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message : tc_message*: Pointer to TC message
// + source_addr: Address: Address of sender
// + incomingInterface: Int32: incoming interface
// + m: olsrmsg*: olsr message pointer
// RETURN :: Int32 : 1 if error, else 0
// **/

static
Int32 OlsrProcessReceivedTc(
    Node* node,
    tc_message* message,
    Address source_addr,
    Int32 incomingInterface,
    olsrmsg* m)
{
    topology_last_entry *t_last;

    RoutingOlsr *olsr = (RoutingOlsr* ) node->appData.olsr;
    Address addr_ip;


    if (olsr->ip_version == NETWORK_IPV6)
    {
        addr_ip.networkType = NETWORK_IPV6;
        Ipv6GetGlobalAggrAddress(
                node,
                incomingInterface,
                &addr_ip.interfaceAddr.ipv6);
    }
    else if (olsr->ip_version == NETWORK_IPV4)
    {
        addr_ip.networkType = NETWORK_IPV4;
        addr_ip.interfaceAddr.ipv4 =
                        NetworkIpGetInterfaceAddress(node,incomingInterface);
    }
//#endif

    olsr->olsrStat.numTcReceived++;

    if (message == NULL)
    {
        return 1;
    }

    if (DEBUG)
    {
        char addrString[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&source_addr,
                addrString);

        char strTime[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), strTime);
        printf("Node %d receives TC message from %s"
            " on interface %d at %s\n",
            node->nodeId, addrString, incomingInterface, strTime);

        OlsrPrintTcMessage(message);
    }


    // we should not check the originator address
    if (Address_IsSameAddress(&message->originator, &addr_ip))
    {
        OlsrDestroyTcMessage(message);
        return 1;
    }

    // if originator address already in duplicate table proc
    // then ignore message processing and fwd

    if (NULL != OlsrLookupDuplicateTableProc(node,
                    message->originator, message->packet_seq_number))
    {
        OlsrForwardMessage(node, m,
        message->originator,message->packet_seq_number,
        incomingInterface,source_addr);

        OlsrDestroyTcMessage(message);
        return 1;
    }

    // Section 9.5 Condition 1
    // Check whether the tc is received from a sym neighbor or not

    if (OlsrCheckLinktoNeighbor(node, source_addr) != SYM_LINK)
    {
        OlsrDestroyTcMessage(message);
        return 1;
    }

    if (DEBUG)
    {
        tc_mpr_addr* mpr;
        char strTime[MAX_STRING_LENGTH];
        char addrString[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&source_addr,
            addrString);

        ctoa(node->getNodeTime(), strTime);

        printf("Node %d receives tc message from %s at %s\n",
            node->nodeId, addrString, strTime);

        mpr = message->multipoint_relay_selector_address;
        printf("mpr_selector_list:[");
        while (mpr != NULL)
        {
            IO_ConvertIpAddressToString(&mpr->address,
                addrString);

            printf("%s:", addrString);
            mpr = mpr->next;
        }
        printf("]\n");
    }

    // if originator address is the last address in an entry in topology
    // table then return the last topology entry

    // RFC Section 9.5 Condition 2
    t_last = OlsrLookupLastTopologyTable(node, message->originator);

    if (t_last != NULL)
    {
        if (DEBUG)
        {
            printf("T_Last exists in Topology table\n");
        }

        // current seq. is older than the last one, then message ignored
        if (SEQNO_GREATER_THAN(t_last->topology_seq, message->ansn))
        {
            OlsrDestroyTcMessage(message);

            if (DEBUG)
            {
                printf("TC_message too old\n");
            }

            return 1;
        }

        if (DEBUG)
        {
            printf("The new ANSN is %d and the old one is %d\n",
                message->ansn, t_last->topology_seq);
        }

        // Condition 3
        // For ANSN comparisons use section 19 of RFC
        // current seq. is newer than the last one

        if (SEQNO_GREATER_THAN(message->ansn, t_last->topology_seq))
        {
            // delete old entry from topology table
            OlsrDeleteLastTopolgyTable(t_last);

            olsr->changes_topology = TRUE;

            // We must insert new entries contained in received message
            t_last = (topology_last_entry *)
                    MEM_malloc(sizeof(topology_last_entry));

            memset(t_last, 0, sizeof(topology_last_entry));

            // Condition 4.1
            OlsrInsertLastTopologyTable(node, t_last, message);
            OlsrUpdateLastTable(node, t_last, message, addr_ip);
        }
        else
        {
            // Condition 4.2
            // This the case where t_last->topology_seq == message->ansn, so
            // we must update the time out and create the new dest entries

            OlsrUpdateLastTable(node, t_last, message, addr_ip);
        }
    }
    else
    {
        // Condition 4.1
        // changes_topology will be set to DOWN  after recalculating the
        // routing table
        olsr->changes_topology = TRUE;
        t_last = (topology_last_entry *)
                MEM_malloc(sizeof(topology_last_entry));

        memset(t_last, 0, sizeof(topology_last_entry));

        OlsrInsertLastTopologyTable(node, t_last, message);
        OlsrUpdateLastTable(node, t_last, message, addr_ip);
    }

    // Call forwarding function here...
    OlsrForwardMessage(node,
        m,
        message->originator,
        message->packet_seq_number,
        incomingInterface,source_addr);
    OlsrDestroyTcMessage(message);

    if (DEBUG)
    {
        OlsrPrintTopologyTable(node);
    }

    OlsrProcessChanges(node);
    return 0;
}


/***************************************************************************
 *                  Definition of build and send message                   *
 ***************************************************************************/

// /**
// FUNCTION   :: OlsrBuildHelloPacket
// LAYER      :: APPLICATION
// PURPOSE    :: Build hello message
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + message: hl_message* : Pointer to hello msg structure
// + interfaceIndex: Int32: outgoing interface for hello pkt
// RETURN :: Int32 : 0
// **/

static
Int32 OlsrBuildHelloPacket(
    Node* node,
    hl_message* message,
    Int32 interfaceIndex)
{
    hello_neighbor* message_neighbor;
    hello_neighbor* tmp_neigh;
    link_entry* links;
    neighbor_entry* neighbor;
    neighborhash_type* hash_neighbor;

    unsigned char index;
    Int32 link;
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    Address main_addr = olsr->main_address;

    Address outif_addr;


    if (olsr->ip_version == NETWORK_IPV6)
    {
        outif_addr.networkType = NETWORK_IPV6;
        Ipv6GetGlobalAggrAddress(
                node,
                interfaceIndex,
                &outif_addr.interfaceAddr.ipv6);
    }
    else if (olsr->ip_version == NETWORK_IPV4)
    {
        outif_addr.networkType = NETWORK_IPV4;
        outif_addr.interfaceAddr.ipv4 =
                        NetworkIpGetInterfaceAddress(node,interfaceIndex);
    }

    message->neighbors = NULL;
    message->packet_seq_number = 0;

    // message->mpr_seq_number = olsr->neighbortable.neighbor_mpr_seq;

    // Set Willingness
    message->willingness = olsr->willingness;
    message->ttl = 1;
    message->source_addr = main_addr;

    links = olsr->link_set;

    while (links != NULL)
    {
        link = OlsrLookupLinkStatus(node, links);

        if (!Address_IsSameAddress(&(links->local_iface_addr), &outif_addr))
        {
            links = links->next;
            continue;
        }

        if (DEBUG)
        {
            printf (" Node %d: Adding neighbors in hello \n", node->nodeId);
            char addrString1[MAX_STRING_LENGTH];
            char addrString2[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(&links->neighbor_iface_addr,
                                        addrString1);
            IO_ConvertIpAddressToString(&links->local_iface_addr,
                                        addrString2);
            printf(" Neighbor iface addr : %s  Local Iface Addr: %s \n",
                addrString1, addrString2);
        }

        message_neighbor = (hello_neighbor *)
                    MEM_malloc(sizeof(hello_neighbor));

        memset(message_neighbor, 0, sizeof(hello_neighbor));

        message_neighbor->link = (unsigned char)link;

        if (links->neighbor->neighbor_is_mpr)
        {
            message_neighbor->status = MPR_NEIGH;
        }
        else
        {
            if (links->neighbor->neighbor_status == SYM)
            {
                message_neighbor->status = SYM_NEIGH;
            }
            else if (links->neighbor->neighbor_status == NOT_SYM)
            {
                message_neighbor->status = NOT_NEIGH;
            }
        }

        message_neighbor->address = links->neighbor_iface_addr;
        message_neighbor->main_address = links->neighbor->neighbor_main_addr;
        message_neighbor->next = message->neighbors;
        message->neighbors = message_neighbor;

        links = links->next;
    }

    // If Multiple OLSR interfaces

    if (olsr->numOlsrInterfaces > 1)
    {
        for (index = 0; index < HASHSIZE; index++)
        {
            hash_neighbor = &olsr->neighbortable.neighborhash[index];

            for (neighbor = hash_neighbor->neighbor_forw;
                    neighbor!= (neighbor_entry *) hash_neighbor;
                    neighbor = neighbor->neighbor_forw)
            {
                tmp_neigh = message->neighbors;

                while (tmp_neigh)
                {
                    if (Address_IsSameAddress(&tmp_neigh->main_address,
                            &neighbor->neighbor_main_addr))
                    {
                        break;
                    }
                    tmp_neigh = tmp_neigh->next;
                }

                if (tmp_neigh)
                {
                    continue;
                }

                message_neighbor = (hello_neighbor *)
                    MEM_malloc(sizeof(hello_neighbor));

                memset(message_neighbor, 0, sizeof(hello_neighbor));

                message_neighbor->link = UNSPEC_LINK;

                if (neighbor->neighbor_is_mpr)
                {
                    message_neighbor->status = MPR_NEIGH;
                }
                else
                {
                    if (neighbor->neighbor_status == SYM)
                    {
                        message_neighbor->status = SYM_NEIGH;
                    }

                    else if (neighbor->neighbor_status == NOT_SYM)
                    {
                        message_neighbor->status= NOT_NEIGH;
                    }

                }

                message_neighbor->address = neighbor->neighbor_main_addr;
                message_neighbor->main_address = neighbor->neighbor_main_addr;

                message_neighbor->next = message->neighbors;
                message->neighbors = message_neighbor;
            }
        }
    }

    return 0;
}


// /**
// FUNCTION   :: OlsrGenerateHello
// LAYER      :: APPLICATION
// PURPOSE    :: Generate hello message
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interfaceIndex: Int32: interface for which msg is to be
//                        generated
// RETURN :: void : NULL
// **/

static
void OlsrGenerateHello(
    Node* node,
    Int32 interfaceIndex)
{
    hl_message hellopacket;

    // receive hello packet info from the node structure to hl_message
    OlsrBuildHelloPacket(node, &hellopacket, interfaceIndex);

    // prepare the hello message and send
    OlsrSendHello(node, &hellopacket, interfaceIndex);

}

// /**
// FUNCTION   :: RoutingOlsrHandlePacket
// LAYER      :: APPLICATION
// PURPOSE    :: Handles OLSR control message received from transport layer
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + msg : Message* : Pointer to Message
// RETURN :: void : NULL
// **/

static
void RoutingOlsrHandlePacket(
    Node* node,
    Message* msg)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    //NON-OLSR node received OLSR packet
    if (olsr == NULL)
    {
        return;
    }
    olsrpacket* olsr_pkt = (olsrpacket *) MESSAGE_ReturnPacket(msg);

    // set m pointer to first msg
    olsrmsg* m = olsr_pkt->olsr_msg;

    Int32 size;
    Int32 msgsize;
    Int32 count;
    Int32 minsize = (sizeof(unsigned char) * 4);

    hl_message hellopacket;
    tc_message tcpacket_in;
    mid_message midpacket_in;
    hna_message hnapacket_in;

     ActionData acnData;
     acnData.actionType = RECV;
     acnData.actionComment = NO_COMMENT;
     TRACE_PrintTrace(node,
                      msg,
                      TRACE_APPLICATION_LAYER,
                      PACKET_IN,
                      &acnData);

    UdpToAppRecv* udpToApp = (UdpToAppRecv *) MESSAGE_ReturnInfo(msg);

    // sender address received from info field
    Address source_addr = udpToApp->sourceAddr;

    // incoming interface received from info field
    Int32 incomingInterface = udpToApp->incomingInterfaceIndex;

    // check whether message is for OLSR only
    if (NetworkIpGetUnicastRoutingProtocolType(node,
        incomingInterface, olsr->ip_version) != ROUTING_PROTOCOL_OLSR_INRIA)
    {
        return;
    }

    // Get main address (address of default interface)
    Address main_addr = olsr->main_address;

    Address originator_address;


    // New RFC version support for multiple messages in OLSR packet

    // First get the size of the packet

    // Get the size of the message portion by subtracting the packet size and
    // the size of the olsr packet header, verify this with the olsr packet
    // size in the packet header

    // Process each message portion by shifting the message pointer to the
    // next message once the message is processed

    // For unknown type, discard message for now

    // Get packet size
    size = MESSAGE_ReturnPacketSize(msg);

    // Get message portion size of the packet as count
    count = olsr_pkt->olsr_packlen - minsize;

    if (DEBUG)
    {
        printf("\nNode %d  Receiving a packet: Size of messages in packet is %d\n",node->nodeId, count);
    }

    // RFC Section 3.4 Condition 1

    if (count < minsize)
    {
        return;
    }

    while (count > 0)
    {
        if (count < minsize)
        {
            break;
        }

        unsigned char ttl;

        if (olsr->ip_version == NETWORK_IPV6)
        {
            ttl = m->olsr_msg_tail.olsr_ipv6_msg.ttl;
        }
        else

        {
            ttl = m->olsr_msg_tail.olsr_ipv4_msg.ttl;
        }
        // RFC 3626 Section 3.4 Condition 2
        if (ttl <= 0)
        {
            if (DEBUG)
            {
                printf("Dropping message type %d from neigh with TTL 0\n",
                                                         m->olsr_msgtype);
            }

            count = count - m->olsr_msg_size;
            continue;
        }

        if (olsr->ip_version == NETWORK_IPV6)
        {

            SetIPv6AddressInfo(&originator_address,
                                m->originator_ipv6_address);
        }
        else

        {
            SetIPv4AddressInfo(&originator_address,
                               m->originator_ipv4_address);
        }
        if (Address_IsSameAddress(&main_addr, &originator_address))
        {
            if (DEBUG)
            {
                printf("Not processing message originating from us \n");
            }
            count = count - m->olsr_msg_size;
            continue;
        }

        // RFC 3626 Section 3.4 Condition 3 seems  relevant for only TC and Mid message types
        // olsrd implem/geentation includes it only for TC messages and mid
        // messages
        // RFC 3626 Section 3.4 Condition 4 seems only relevant for TC, MID
        // messages and unknown types, We discard unknown types currently and
        // implement condition 4 in TC and mid message processing similar to olsrd
        // implementation

        switch (m->olsr_msgtype)
        {

            case HELLO_PACKET:
            {
                // hello message received, convert it to hl_message structure
                OlsrHelloChgeStruct(node, &hellopacket, m);

                // process hello message
                OlsrProcessReceivedHello(node,
                    &hellopacket,
                    incomingInterface,
                    source_addr);
                break;
            }
            case TC_PACKET:
            {

                // tc message received, convert it to tc_message structure
                OlsrTcChgeStruct( node, &tcpacket_in, m, source_addr);

                // process tc message
                OlsrProcessReceivedTc(node,
                    &tcpacket_in,
                    source_addr,
                    incomingInterface,
                    m);

                break;
            }
            case MID_PACKET:
            {
                // mid message received, convert it to mid_message structure
                OlsrMidChgeStruct(node, &midpacket_in, m);

                // process mid message
                OlsrProcessReceivedMid(node,
                    &midpacket_in,
                    source_addr,
                    incomingInterface,
                    m);

                break;
            }
            case HNA_PACKET:
            {
                // mid message received, convert it to mid_message structure
                OlsrHnaChgeStruct(node, &hnapacket_in, m);

                // process mid message
                OlsrProcessReceivedHna(node,
                    &hnapacket_in,
                    source_addr,
                    incomingInterface,
                    m);

                break;
            }


            default:
            {
                if (DEBUG)
                {
                    printf("Unknown OLSR message type\n");
                }
                // Ideally forward this message if originator node is not me
                UInt16 seqno;

                if (olsr->ip_version == NETWORK_IPV6)
                {
                    seqno = m->olsr_msg_tail.olsr_ipv6_msg.msg_seq_no;
                }
                else

                {
                    seqno = m->olsr_msg_tail.olsr_ipv4_msg.msg_seq_no;
                }


                OlsrForwardMessage(
                    node,
                    m,
                    originator_address,
                    seqno,
                    incomingInterface,
                    source_addr);

                break;

            }

        } // end switch

        msgsize = m->olsr_msg_size;

        if (DEBUG)
        {
            printf (" Node %d: size of this message is %d\n",
                node->nodeId, msgsize);
        }

        count = count - msgsize;

        if (count < 0)
        {
            if (DEBUG)
            {
                printf(" Error in packet length\n");
            }
            break;
        }

        // Move the message pointer  to the next message
        m = (olsrmsg *) ((char *)m + msgsize);

    } // end while

} // end function


// /**
// FUNCTION   :: OlsrPrintTraceXML
// LAYER      :: NETWORK
// PURPOSE    :: Print LSA header trace information in XML format
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + msg  : Message* : Pointer to Message
// RETURN ::  void : NULL
// **/

void OlsrPrintTraceXML(
       Node* node,
       Message* msg)
{
    char buf[MAX_STRING_LENGTH];
    char origAddr[MAX_STRING_LENGTH];
    char neigAddr[MAX_STRING_LENGTH];

    Int32 size;
    Int32 msgsize;
    Int32 count;
    Int32 minsize = (sizeof(unsigned char) * 4);
    Float32 vtime;

    unsigned char ttl;
    unsigned char hopCount;
    UInt16 msgSeqNo;

    olsrpacket* olsr_pkt = (olsrpacket *) MESSAGE_ReturnPacket(msg);

    // set m pointer to first msg
    olsrmsg* m = (olsrmsg* )olsr_pkt->olsr_msg;

    // Get olsr node pointer
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;

    // Get main address (address of default interface)
    Address source_addr = MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(
                                       node,
                                       msg->originatingNodeId,
                                       olsr->ip_version);

    sprintf(buf, "<olsr>");
    TRACE_WriteToBufferXML(node, buf);

    sprintf(buf, "%hu %hu ",
        olsr_pkt->olsr_packlen,
        olsr_pkt->olsr_pack_seq_no);
     TRACE_WriteToBufferXML(node, buf);

     // Get packet size
    size = MESSAGE_ReturnPacketSize(msg);

    // Get size of the message in packet as count
    count = olsr_pkt->olsr_packlen - minsize;

    if (count < minsize)
    {
        return;
    }

    sprintf(buf, " <olsrMsgList>");
    TRACE_WriteToBufferXML(node, buf);

    while (count > 0)
    {

        if (count < minsize)
        {
            break;
        }

        if (olsr->ip_version == NETWORK_IPV6)
        {

            ttl = m->olsr_msg_tail.olsr_ipv6_msg.ttl;
            hopCount = m->olsr_msg_tail.olsr_ipv6_msg.hop_count;
            msgSeqNo = m->olsr_msg_tail.olsr_ipv6_msg.msg_seq_no;
            IO_ConvertIpAddressToString(&m->originator_ipv6_address,
                                        origAddr);

        }
        else
        {
              ttl = m->olsr_msg_tail.olsr_ipv4_msg.ttl;
              hopCount = m->olsr_msg_tail.olsr_ipv4_msg.hop_count;
              msgSeqNo = m->olsr_msg_tail.olsr_ipv4_msg.msg_seq_no;
              IO_ConvertIpAddressToString(m->originator_ipv4_address,
                                          origAddr);
        }

       vtime = OlsrME2Double(m->olsr_vtime);

       sprintf(buf, "%hu %f %hu %s %hu %hu %hu ",
                        m->olsr_msgtype,
                        vtime,
                        m->olsr_msg_size,
                        origAddr,
                        ttl,
                        hopCount,
                        msgSeqNo);
                        TRACE_WriteToBufferXML(node, buf);

        switch (m->olsr_msgtype)
        {
        case HELLO_PACKET:
            {

                char neighborAddr[MAX_STRING_LENGTH];
                in6_addr* h_ipv6_addr = NULL;
                NodeAddress* h_ipv4_addr = NULL;
                NodeAddress* h_ipv4_adr;
                in6_addr* h_ipv6_adr;
                hl_message hellopacket;
                hellomsg* h;
                hellinfo* hinfo;
                hellinfo* hinf;
                hello_neighbor* neighbors;

                // hello message received, convert it to hl_message structure
                OlsrHelloChgeStruct(node, &hellopacket, m);

                if (olsr->ip_version == NETWORK_IPV6)
                {
                    h = m->olsr_ipv6_hello;
                    hinfo = h->hell_info;
                    if (hellopacket.neighbors != NULL){
                        IO_ConvertIpAddressToString(
                            &hinfo->neigh_addr->ipv6_addr,
                            neigAddr);
                    }
                }
                else

                {
                    h = m->olsr_ipv4_hello;
                    hinfo = h->hell_info;
                    if (hellopacket.neighbors != NULL){
                        IO_ConvertIpAddressToString(
                            hinfo->neigh_addr->ipv4_addr,
                            neigAddr);
                    }
                }

                sprintf(buf, " <helloMsg>");
                TRACE_WriteToBufferXML(node, buf);

               sprintf(buf, "%hu %f %hu ",
                   0,//reserved
                   OlsrME2Double(h->htime),
                   h->willingness);
               TRACE_WriteToBufferXML(node, buf);



               sprintf(buf, "<linkInfolist>");
               TRACE_WriteToBufferXML(node, buf);

                for (hinf = hinfo;
                     (char *)hinf < (char *)m + m->olsr_msg_size;
                     hinf = (hellinfo *)((char *)hinf +
                                            hinf->link_msg_size))
                {

                     if (olsr->ip_version == NETWORK_IPV6)
                     {

                        h_ipv6_addr = (in6_addr *)hinf->neigh_addr;
                        h_ipv6_adr = h_ipv6_addr;

                         while ((char *)h_ipv6_adr < (char *)hinf +
                                                        hinf->link_msg_size)
                         {

                            char neighborAddr[MAX_STRING_LENGTH];
                            IO_ConvertIpAddressToString(
                                                &hinf->neigh_addr->ipv6_addr,
                                                neighborAddr);

                            sprintf(buf,
                                    "<helloInfo>%hu %hu %hu %s </helloInfo>",
                                        EXTRACT_LINK(hinf->link_code),
                                        0,//reserved
                                        hinf->link_msg_size,
                                        neigAddr);
                            TRACE_WriteToBufferXML(node, buf);
                             h_ipv6_adr++;
                        }

                    }
                    else
                    {

                            h_ipv4_addr = (NodeAddress *)hinf->neigh_addr;
                            h_ipv4_adr = h_ipv4_addr;

                            while ((char *)h_ipv4_adr < (char *)hinf +
                                                        hinf->link_msg_size)
                            {

                                char addrString[MAX_STRING_LENGTH];
                                IO_ConvertIpAddressToString(*h_ipv4_adr,
                                                              addrString);

                                sprintf(buf,
                                "<helloInfo>%hu %hu %hu %s </helloInfo>",
                                                   EXTRACT_LINK(hinf->link_code),
                                                   0,//reserved
                                                   hinf->link_msg_size,
                                                   addrString);
                                TRACE_WriteToBufferXML(node, buf);
                                h_ipv4_adr++;

                        }

                    }//end of else
                }//end of for loop

               sprintf(buf, "</linkInfolist>");
               TRACE_WriteToBufferXML(node, buf);

               sprintf(buf, "<neighbourList>");
               TRACE_WriteToBufferXML(node, buf);

               IO_ConvertIpAddressToString(&hellopacket.source_addr,
                       neighborAddr);
               neighbors = hellopacket.neighbors;

               while (neighbors != NULL)
               {
                    IO_ConvertIpAddressToString(&neighbors->address,
                                                neighborAddr);
                    sprintf(buf, "<neighbours>%s </neighbours>",
                            neighborAddr);

                    TRACE_WriteToBufferXML(node, buf);
                    neighbors = neighbors->next;
                }//end of while
               sprintf(buf, " </neighbourList>");
               TRACE_WriteToBufferXML(node, buf);

               sprintf(buf, " </helloMsg>");
               TRACE_WriteToBufferXML(node, buf);
               OlsrDestroyHelloMessage(&hellopacket);
               break;
            }//end hello packet case
        case TC_PACKET:
            {
                NodeAddress* mprs_ipv4_addr = NULL;
                in6_addr* mprs_ipv6_addr = NULL;
                tcmsg* tc = NULL;
                tc_message message;
                tc_mpr_addr* mprs;

                char addrString1[MAX_STRING_LENGTH];
                char addrString2[MAX_STRING_LENGTH];

                // tc message received, convert it to tc_message structure
                OlsrTcChgeStruct( node, &message, m, source_addr);

                if (olsr->ip_version == NETWORK_IPV6)
                {
                    tc = m->olsr_ipv6_tc;
                    mprs_ipv6_addr =
                        (in6_addr *)tc->adv_neighbor_main_address;
                }
                else
                {
                    tc = m->olsr_ipv4_tc;
                    mprs_ipv4_addr =
                        (NodeAddress *)tc->adv_neighbor_main_address;
                }

                IO_ConvertIpAddressToString(&message.source_addr,
                                            addrString1);

                 IO_ConvertIpAddressToString(&message.originator,
                                             addrString2);

                mprs = message.multipoint_relay_selector_address;

                sprintf(buf, "<tcMsg>");
                TRACE_WriteToBufferXML(node, buf);

                sprintf(buf, "%s %s %hu %hu %hu ",
                         addrString1,
                         addrString2,
                         message.packet_seq_number,
                         message.hop_count,
                         message.ansn);

                TRACE_WriteToBufferXML(node, buf);

                sprintf(buf, " <mprsAddrlist>");
                TRACE_WriteToBufferXML(node, buf);

                while (mprs != NULL)
                {
                    sprintf(buf, " <mprsAddr>");
                    TRACE_WriteToBufferXML(node, buf);

                    IO_ConvertIpAddressToString(&mprs->address,
                                                addrString1);

                    sprintf(buf, "%s ",
                        addrString1);
                    TRACE_WriteToBufferXML(node, buf);

                    mprs = mprs->next;

                   sprintf(buf, " </mprsAddr>");
                   TRACE_WriteToBufferXML(node, buf);
                }

                sprintf(buf, " </mprsAddrlist>");
                TRACE_WriteToBufferXML(node, buf);

                sprintf(buf, " </tcMsg>");
                TRACE_WriteToBufferXML(node, buf);
                OlsrDestroyTcMessage(&message);
                break;
            }//end of TC packet case
       case MID_PACKET :
            {
                char addrString[MAX_STRING_LENGTH];
                char addrString1[MAX_STRING_LENGTH];
                mid_message midpacket;
                midmsg* mid_msg = NULL;
                NodeAddress* alias_ipv4_addr = NULL;
                in6_addr* alias_ipv6_addr = NULL;
                mid_alias* mid_addr;
                Float32 vtime;

                sprintf(buf, "<midPacket>");
                TRACE_WriteToBufferXML(node, buf);

                // receive mid packet info from the node structure to
                // mid_message
                // mid message received, convert it to mid_message structure

                OlsrMidChgeStruct(node, &midpacket, m);

                if (olsr->ip_version == NETWORK_IPV6)
                {
                    mid_msg = m->olsr_ipv6_mid;
                    alias_ipv6_addr = (in6_addr *)mid_msg->olsr_iface_addr;
                }
                else

                {
                    mid_msg = m->olsr_ipv4_mid;
                    alias_ipv4_addr =
                        (NodeAddress *)mid_msg->olsr_iface_addr;
                }

                IO_ConvertIpAddressToString(&midpacket.mid_origaddr,
                                    addrString);

                IO_ConvertIpAddressToString(&midpacket.main_addr,
                                             addrString1);
                vtime = OlsrME2Double((unsigned char) midpacket.vtime);

                 sprintf(buf, "%f %s %hu %hu %hu %s ",
                     vtime,
                     addrString,
                     midpacket.mid_hopcnt,
                     midpacket.mid_ttl,
                     midpacket.mid_seqno,
                     addrString1);

                TRACE_WriteToBufferXML(node, buf);


               sprintf(buf, "<midaliaslist>");
               TRACE_WriteToBufferXML(node, buf);

                mid_addr = midpacket.mid_addr;

                while (mid_addr != NULL)
                {
                    sprintf(buf, "<midAddr>");
                    TRACE_WriteToBufferXML(node, buf);

                    IO_ConvertIpAddressToString(&mid_addr->alias_addr,
                        addrString);

                    sprintf(buf, "%s ",
                        addrString);
                    TRACE_WriteToBufferXML(node, buf);

                    mid_addr = mid_addr->next;

                    sprintf(buf, "</midAddr>");
                    TRACE_WriteToBufferXML(node, buf);
                }//end of while

                sprintf(buf, "</midaliaslist>");
                TRACE_WriteToBufferXML(node, buf);

                sprintf(buf, " </midPacket>");
                TRACE_WriteToBufferXML(node, buf);
                OlsrDestroyMidMessage(&midpacket);

               break;
            }//end of mid packet case
       case HNA_PACKET :
           {
                char originatorAddr[MAX_STRING_LENGTH];

                char addrString1[MAX_STRING_LENGTH];
                hna_message hnapacket;
                hnamsg* hna_msg = NULL;
                struct _hnapair_ipv6* hnapair_ipv6 = NULL;
                struct _hnapair_ipv4* hnapair_ipv4 = NULL;
                Float32 vtime;
                UInt16 netPrefixlength;

                sprintf(buf, " <hnaPacket>");
                TRACE_WriteToBufferXML(node, buf);

                //hna message received, convert it to hna_message structure
                OlsrHnaChgeStruct(node, &hnapacket, m);

                if (olsr->ip_version == NETWORK_IPV6)
                {
                    hna_msg = m->olsr_ipv6_hna;
                    hnapair_ipv6 =
                        (struct _hnapair_ipv6* )hna_msg->hna_net_pair;
                }
                else

                {
                    hna_msg = m->olsr_ipv4_hna;
                    hnapair_ipv4 =
                        (struct _hnapair_ipv4* )hna_msg->hna_net_pair;
                }
                vtime = OlsrME2Double((unsigned char) hnapacket.vtime); // validity time
                IO_ConvertIpAddressToString(&hnapacket.hna_origaddr,
                                            originatorAddr);

                sprintf(buf, "%f %s %hu %hu %hu ",
                     vtime,
                     originatorAddr,
                     hnapacket.hna_seqno,
                     hnapacket.hna_hopcnt,
                     hnapacket.hna_ttl
                     );

                TRACE_WriteToBufferXML(node, buf);

                hna_net_addr* hna_net_pair = hnapacket.hna_net_pair;

                sprintf(buf, " <hnanetPairList>");
                TRACE_WriteToBufferXML(node, buf);

                while (hna_net_pair != NULL)
                {
                    char addrString[MAX_STRING_LENGTH];

                    IO_ConvertIpAddressToString(&hna_net_pair->net,
                                                addrString);

                    if (olsr->ip_version == NETWORK_IPV6)
                    {
                        netPrefixlength = (UInt16)hna_net_pair->netmask.v6;

                        sprintf(buf, "<hnanetPairv6>%s %hu </hnanetPairv6>",
                                    addrString,
                                    netPrefixlength);
                        TRACE_WriteToBufferXML(node, buf);

                    }
                    else
                    {
                         IO_ConvertIpAddressToString(
                                                   hna_net_pair->netmask.v4,
                                                   addrString1);

                          sprintf(buf, "<hnanetPairv4>%s %s </hnanetPairv4>",
                                    addrString,
                                    addrString1);
                            TRACE_WriteToBufferXML(node, buf);

                    }

                    hna_net_pair = hna_net_pair->next;
                }

                sprintf(buf, " </hnanetPairList>");
                TRACE_WriteToBufferXML(node, buf);

                sprintf(buf, " </hnaPacket>");
                TRACE_WriteToBufferXML(node, buf);

                OlsrDestroyHnaMessage(&hnapacket);

                break;
           }//end of hna packet case

       }//end of switch
        msgsize = m->olsr_msg_size;
        count = count - msgsize;
        // Move the message pointer  to the next message
        m = (olsrmsg *) ((char *)m + msgsize);


    }//end of while loop count>0

    sprintf(buf, "</olsrMsgList>");
    TRACE_WriteToBufferXML(node, buf);

    sprintf(buf, "</olsr>");
    TRACE_WriteToBufferXML(node, buf);

}

// /**
// FUNCTION   :: OlsrInitTrace
// LAYER      :: APPLCATION
// PURPOSE    :: Olsr initialization  for tracing
// PARAMETERS ::
// + node : Node* : Pointer to node
// + nodeInput    : const NodeInput* : Pointer to NodeInput
// RETURN ::  void : NULL
// **/

static
void OlsrInitTrace(Node* node, const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    BOOL traceAll = TRACE_IsTraceAll(node);
    BOOL trace = FALSE;
    static BOOL writeMap = TRUE;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "TRACE-OLSR",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "YES") == 0)
        {
            trace = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            trace = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "TRACE-OLSR should be either \"YES\" or \"NO\".\n");
        }
    }
    else
    {
        if (traceAll || node->traceData->layer[TRACE_APPLICATION_LAYER])
        {
            trace = TRUE;
        }
    }

    if (trace)
    {
            TRACE_EnableTraceXML(node, TRACE_OLSR,
                "OSLR", OlsrPrintTraceXML, writeMap);
    }
    else
    {
           TRACE_DisableTraceXML(node, TRACE_OLSR,
               "OLSR", writeMap);
    }
    writeMap = FALSE;
}

// /**
// FUNCTION   :: RoutingOlsrInriaInit
// LAYER      :: APPLICATION
// PURPOSE    :: Initialization function for OLSR.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + nodeInput : const NodeInput* : Pointer to input configuration
// + interfaceIndex: Int32: Interface being initialized
// RETURN :: void : NULL
// **/

void RoutingOlsrInriaInit(
    Node *node,
    const NodeInput* nodeInput,
    Int32 interfaceIndex,
    NetworkType networkType)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    unsigned char index;
    char errStr[MAX_STRING_LENGTH];

    if (olsr == NULL)
    {

        // Allocate Olsr struct
        olsr = (RoutingOlsr* ) MEM_malloc(sizeof(RoutingOlsr));

        memset(olsr, 0, sizeof(RoutingOlsr));

        node->appData.olsr = (void *) olsr;

        // initialize sequence number
        olsr->numOlsrInterfaces = 1;
        olsr->mpr_coverage = 1;
        olsr->willingness = WILL_DEFAULT;

        /* initialize random seed */
        RANDOM_SetSeed(olsr->seed,
                       node->globalSeed,
                       node->nodeId,
                       APP_ROUTING_OLSR_INRIA);

        olsr->message_seqno = 1;
        olsr->tcMessageStarted = FALSE;

        // initialize stat variables
        olsr->olsrStat.numHelloReceived = 0;
        olsr->olsrStat.numHelloSent = 0;
        olsr->olsrStat.numTcReceived = 0;
        olsr->olsrStat.numTcGenerated = 0;
        olsr->olsrStat.numTcRelayed = 0;
        olsr->olsrStat.numMidReceived = 0;
        olsr->olsrStat.numMidGenerated = 0;
        olsr->olsrStat.numMidRelayed = 0;
        olsr->olsrStat.numHnaReceived = 0;
        olsr->olsrStat.numHnaGenerated = 0;
        olsr->olsrStat.numHnaRelayed = 0;
        olsr->statsPrinted = FALSE;

        // Initialisation of differents tables to be used.
        OlsrInitTables(olsr);

        olsr->interfaceSequenceNumbers = (UInt16 *)MEM_malloc(
                                                 sizeof(UInt16)
                                                 * (node->numberInterfaces));

        memset(
            olsr->interfaceSequenceNumbers,
            0,
            sizeof(UInt16) * (node->numberInterfaces));

        // Init interface seq no
        for (index = 0; index < node->numberInterfaces; index++)
        {
            olsr->interfaceSequenceNumbers[index] = 1;
        }
        BOOL retVal = FALSE;
        char buf[MAX_STRING_LENGTH];

        olsr->ip_version = networkType;

        if (networkType == NETWORK_IPV4)
        {
            IO_ReadString(
                node->nodeId,
                NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "OLSR-HELLO-INTERVAL",
                &retVal,
                buf);
        }

        else
        {
            Address address;

            NetworkGetInterfaceInfo(
                node,
                interfaceIndex,
                &address,
                olsr->ip_version);

            IO_ReadString(
                node->nodeId,
                &address.interfaceAddr.ipv6,
                nodeInput,
                "OLSR-HELLO-INTERVAL",
                &retVal,
                buf);
        }



        if (retVal)
        {
            olsr->hello_interval = TIME_ConvertToClock(buf);

            if (olsr->hello_interval <= 0 )
            {
                sprintf(errStr, "Invalid Hello Interval"
                            " Parameter specified, hence continuing"
                            " with the default value: %" TYPES_64BITFMT 
                            "dS. \n",
                            DEFAULT_HELLO_INTERVAL / SECOND);
                ERROR_ReportWarning(errStr);

                olsr->hello_interval = DEFAULT_HELLO_INTERVAL;
            }
        }
        else
        {
            olsr->hello_interval = DEFAULT_HELLO_INTERVAL;
        }
        retVal = FALSE;

        if (networkType == NETWORK_IPV4)
        {
            IO_ReadString(
                node->nodeId,
                NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "OLSR-TC-INTERVAL",
                &retVal,
                buf);
        }

        else
        {
            Address address;

            NetworkGetInterfaceInfo(
                node,
                interfaceIndex,
                &address,
                olsr->ip_version);

            IO_ReadString(
                node->nodeId,
                &address.interfaceAddr.ipv6,
                nodeInput,
                "OLSR-TC-INTERVAL",
                &retVal,
                buf);
        }

        if (retVal)
        {
            olsr->tc_interval = TIME_ConvertToClock(buf);

            if (olsr->tc_interval <= 0 )
            {
                sprintf(errStr, "Invalid TC Interval"
                            " Parameter specified, hence continuing"
                            " with the default value: %" TYPES_64BITFMT 
                            "dS. \n",
                            DEFAULT_TC_INTERVAL / SECOND);
                ERROR_ReportWarning(errStr);

                olsr->tc_interval = DEFAULT_TC_INTERVAL;
            }
        }
        else
        {
            olsr->tc_interval = DEFAULT_TC_INTERVAL;
        }
        retVal = FALSE;

        if (networkType == NETWORK_IPV4)
        {
            IO_ReadString(
                node->nodeId,
                NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "OLSR-MID-INTERVAL",
                &retVal,
                buf);
        }

        else
        {
            Address address;

            NetworkGetInterfaceInfo(
                node,
                interfaceIndex,
                &address,
                olsr->ip_version);

            IO_ReadString(
                node->nodeId,
                &address.interfaceAddr.ipv6,
                nodeInput,
                "OLSR-MID-INTERVAL",
                &retVal,
                buf);
        }

        if (retVal)
        {
            olsr->mid_interval = TIME_ConvertToClock(buf);

            if (olsr->mid_interval <= 0 )
            {
                sprintf(errStr, "Invalid MID Interval"
                            " Parameter specified, hence continuing"
                            " with the default value: %" TYPES_64BITFMT 
                            "dS. \n",
                            DEFAULT_MID_INTERVAL / SECOND);
                ERROR_ReportWarning(errStr);

                olsr->mid_interval = DEFAULT_MID_INTERVAL;
            }
        }
        else
        {
            olsr->mid_interval = DEFAULT_MID_INTERVAL;
        }
        retVal = FALSE;

        if (networkType == NETWORK_IPV4)
        {
            IO_ReadString(
                node->nodeId,
                NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "OLSR-HNA-INTERVAL",
                &retVal,
                buf);
        }

        else
        {
            Address address;

            NetworkGetInterfaceInfo(
                node,
                interfaceIndex,
                &address,
                olsr->ip_version);

            IO_ReadString(
                node->nodeId,
                &address.interfaceAddr.ipv6,
                nodeInput,
                "OLSR-HNA-INTERVAL",
                &retVal,
                buf);
        }

        if (retVal)
        {
            olsr->hna_interval = TIME_ConvertToClock(buf);

            if (olsr->hna_interval <= 0 )
            {
                sprintf(errStr, "Invalid HNA Interval"
                            " Parameter specified, hence continuing"
                            " with the default value: %" TYPES_64BITFMT 
                            "dS. \n",
                            DEFAULT_HNA_INTERVAL / SECOND);
                ERROR_ReportWarning(errStr);

                olsr->hna_interval = DEFAULT_HNA_INTERVAL;
            }
        }
        else
        {
            olsr->hna_interval = DEFAULT_HNA_INTERVAL;
        }
        retVal = FALSE;

        if (networkType == NETWORK_IPV4)
        {
            IO_ReadString(
                node->nodeId,
                NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "OLSR-NEIGHBOR-HOLD-TIME",
                &retVal,
                buf);
        }

        else
        {
            Address address;

            NetworkGetInterfaceInfo(
                node,
                interfaceIndex,
                &address,
                olsr->ip_version);

            IO_ReadString(
                node->nodeId,
                &address.interfaceAddr.ipv6,
                nodeInput,
                "OLSR-NEIGHBOR-HOLD-TIME",
                &retVal,
                buf);
        }

        if (retVal)
        {
            olsr->neighb_hold_time = TIME_ConvertToClock(buf);

            if (olsr->neighb_hold_time <= 0 )
            {
                sprintf(errStr, "Invalid NEIGHBOR-HOLD Interval"
                            " Parameter specified, hence continuing"
                            " with the default value: %" TYPES_64BITFMT 
                            "dS. \n",
                            DEFAULT_NEIGHB_HOLD_TIME / SECOND);
                ERROR_ReportWarning(errStr);

                olsr->neighb_hold_time = DEFAULT_NEIGHB_HOLD_TIME;
            }
        }
        else
        {

            olsr->neighb_hold_time = DEFAULT_NEIGHB_HOLD_TIME;
        }
        retVal = FALSE;

        if (networkType == NETWORK_IPV4)
        {
            IO_ReadString(
                node->nodeId,
                NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "OLSR-TOPOLOGY-HOLD-TIME",
                &retVal,
                buf);
        }

        else
        {
            Address address;

            NetworkGetInterfaceInfo(
                node,
                interfaceIndex,
                &address,
                olsr->ip_version);

            IO_ReadString(
                node->nodeId,
                &address.interfaceAddr.ipv6,
                nodeInput,
                "OLSR-TOPOLOGY-HOLD-TIME",
                &retVal,
                buf);
        }

        if (retVal)
        {
            olsr->top_hold_time = TIME_ConvertToClock(buf);

            if (olsr->top_hold_time <= 0 )
            {
                sprintf(errStr, "Invalid TOP-HOLD Interval"
                            " Parameter specified, hence continuing"
                            " with the default value: %" TYPES_64BITFMT 
                            "dS. \n",
                            DEFAULT_TOP_HOLD_TIME / SECOND);
                ERROR_ReportWarning(errStr);

                olsr->top_hold_time = DEFAULT_TOP_HOLD_TIME;
            }
        }
        else
        {
            olsr->top_hold_time = DEFAULT_TOP_HOLD_TIME;
        }
        retVal = FALSE;

        if (networkType == NETWORK_IPV4)
        {
            IO_ReadString(
                node->nodeId,
                NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "OLSR-DUPLICATE-HOLD-TIME",
                &retVal,
                buf);
        }

        else
        {
            Address address;

            NetworkGetInterfaceInfo(
                node,
                interfaceIndex,
                &address,
                olsr->ip_version);

            IO_ReadString(
                node->nodeId,
                &address.interfaceAddr.ipv6,
                nodeInput,
                "OLSR-DUPLICATE-HOLD-TIME",
                &retVal,
                buf);
        }

        if (retVal)
        {
            olsr->dup_hold_time = TIME_ConvertToClock(buf);

            if (olsr->dup_hold_time <= 0 )
            {
                sprintf(errStr, "Invalid DUPLICATE-HOLD Interval"
                            " Parameter specified, hence continuing"
                            " with the default value: %" TYPES_64BITFMT 
                            "dS. \n",
                            DEFAULT_DUPLICATE_HOLD_TIME / SECOND);
                ERROR_ReportWarning(errStr);

                olsr->dup_hold_time = DEFAULT_DUPLICATE_HOLD_TIME;
            }
        }
        else
        {
            olsr->dup_hold_time = DEFAULT_DUPLICATE_HOLD_TIME;
        }
        retVal = FALSE;

        if (networkType == NETWORK_IPV4)
        {
            IO_ReadString(
                node->nodeId,
                NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "OLSR-MID-HOLD-TIME",
                &retVal,
                buf);
        }

        else
        {
            Address address;

            NetworkGetInterfaceInfo(
                node,
                interfaceIndex,
                &address,
                olsr->ip_version);

            IO_ReadString(
                node->nodeId,
                &address.interfaceAddr.ipv6,
                nodeInput,
                "OLSR-MID-HOLD-TIME",
                &retVal,
                buf);
        }

        if (retVal)
        {
            olsr->mid_hold_time = TIME_ConvertToClock(buf);

            if (olsr->mid_hold_time <= 0 )
            {
                sprintf(errStr, "Invalid MID-HOLD Interval"
                            " Parameter specified, hence continuing"
                            " with the default value: %" TYPES_64BITFMT
                            "dS. \n",
                            DEFAULT_MID_HOLD_TIME / SECOND);
                ERROR_ReportWarning(errStr);

                olsr->mid_hold_time = DEFAULT_MID_HOLD_TIME;
            }
        }
        else
        {
            olsr->mid_hold_time = DEFAULT_MID_HOLD_TIME;
        }
        retVal = FALSE;

        if (networkType == NETWORK_IPV4)
        {
            IO_ReadString(
                node->nodeId,
                NetworkIpGetInterfaceAddress(node, interfaceIndex),
                nodeInput,
                "OLSR-HNA-HOLD-TIME",
                &retVal,
                buf);
        }

        else
        {
            Address address;

            NetworkGetInterfaceInfo(
                node,
                interfaceIndex,
                &address,
                olsr->ip_version);

            IO_ReadString(
                node->nodeId,
                &address.interfaceAddr.ipv6,
                nodeInput,
                "OLSR-HNA-HOLD-TIME",
                &retVal,
                buf);
        }

        if (retVal)
        {
            olsr->hna_hold_time = TIME_ConvertToClock(buf);

            if (olsr->hna_hold_time <= 0 )
            {
                sprintf(errStr, "Invalid HNA-HOLD Interval"
                            " Parameter specified, hence continuing"
                            " with the default value: %" TYPES_64BITFMT 
                            "dS. \n",
                            DEFAULT_HNA_HOLD_TIME / SECOND);
                ERROR_ReportWarning(errStr);

                olsr->hna_hold_time = DEFAULT_HNA_HOLD_TIME;
            }
        }
        else
        {
            olsr->hna_hold_time = DEFAULT_HNA_HOLD_TIME;
        }

        clocktype delay = OLSR_STARTUP_DELAY
                          - (RANDOM_nrand(olsr->seed)
                          % (clocktype) ((Float32) OLSR_STARTUP_DELAY
                                                   * OLSR_STARTUP_JITTER));

        // set refresh timer for hello message
        Message* helloPeriodicMsg = MESSAGE_Alloc(node,
                                      APP_LAYER,
                                      APP_ROUTING_OLSR_INRIA,
                                      MSG_APP_OlsrPeriodicHello);

        MESSAGE_Send(node, helloPeriodicMsg, delay);

       // set refresh timer for tc message
        Message* tcPeriodicMsg = MESSAGE_Alloc(node,
                                     APP_LAYER,
                                     APP_ROUTING_OLSR_INRIA,
                                     MSG_APP_OlsrPeriodicTc);

        MESSAGE_Send(node, tcPeriodicMsg, delay);

        // set refresh timer for mid message
        Message* midPeriodicMsg = MESSAGE_Alloc(node,
                                      APP_LAYER,
                                      APP_ROUTING_OLSR_INRIA,
                                      MSG_APP_OlsrPeriodicMid);

        MESSAGE_Send(node, midPeriodicMsg, delay);

        // set refresh timer for hna message
        Message* hnaPeriodicMsg = MESSAGE_Alloc(node,
                                      APP_LAYER,
                                      APP_ROUTING_OLSR_INRIA,
                                      MSG_APP_OlsrPeriodicHna);

        MESSAGE_Send(node, hnaPeriodicMsg, delay);


        // set neighbor hold timer
        Message* neighHoldMsg = MESSAGE_Alloc(node,
                                    APP_LAYER,
                                    APP_ROUTING_OLSR_INRIA,
                                    MSG_APP_OlsrNeighHoldTimer);

        MESSAGE_Send(node, neighHoldMsg, olsr->neighb_hold_time);


        // set topology hold timer
        Message* topoHoldMsg = MESSAGE_Alloc(node,
                                   APP_LAYER,
                                   APP_ROUTING_OLSR_INRIA,
                                   MSG_APP_OlsrTopologyHoldTimer);

        MESSAGE_Send(node, topoHoldMsg, olsr->top_hold_time);

        // set duplicate hold timer
        Message* duplicateHoldMsg = MESSAGE_Alloc(node,
                                        APP_LAYER,
                                        APP_ROUTING_OLSR_INRIA,
                                        MSG_APP_OlsrDuplicateHoldTimer);

        MESSAGE_Send(node, duplicateHoldMsg, olsr->dup_hold_time);

        // set MID hold timer
        Message* midHoldMsg = MESSAGE_Alloc(node,
                                  APP_LAYER,
                                  APP_ROUTING_OLSR_INRIA,
                                  MSG_APP_OlsrMidHoldTimer);

        MESSAGE_Send(node, midHoldMsg, olsr->mid_hold_time);

        // set HNA hold timer
        Message* hnaHoldMsg = MESSAGE_Alloc(node,
                                  APP_LAYER,
                                  APP_ROUTING_OLSR_INRIA,
                                  MSG_APP_OlsrHnaHoldTimer);

        MESSAGE_Send(node, hnaHoldMsg, olsr->hna_hold_time);


        NetworkGetInterfaceInfo(
            node, interfaceIndex, &olsr->main_address, networkType);

        // registering RoutingOlsr2HandleAddressChangeEvent function
        NetworkIpAddAddressChangedHandlerFunction(node,
                            &RoutingOlsrvInriaHandleChangeAddressEvent);

        // Intialize trace
        OlsrInitTrace(node, nodeInput);

        // registering RoutingOlsrHandleChangeAddressEvent function
        Ipv6AddPrefixChangedHandlerFunction(node,
                                &RoutingOlsrHandleChangeAddressEvent);
    }
}


// /**
// FUNCTION   :: RoutingOlsrPrintStat
// LAYER      :: APPLICATION
// PURPOSE    :: Print the statistics
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// RETURN :: void : NULL
// **/

static
void RoutingOlsrPrintStat(Node *node)
{
    RoutingOlsrStat olsrStat;
    RoutingOlsr *olsr;
    char buf[MAX_STRING_LENGTH];

    olsr = (RoutingOlsr* ) node->appData.olsr;
    olsrStat = olsr->olsrStat;

    if (node->appData.routingStats)
    {
        sprintf(buf, "Hello Messages Received = %u",
                olsrStat.numHelloReceived);

        IO_PrintStat(
            node,
            "Application",
            "OLSR",
            ANY_DEST,
            -1, // instance Id
            buf);
        sprintf(buf, "Hello Messages Sent = %u",
                olsrStat.numHelloSent);

        IO_PrintStat(
            node,
            "Application",
            "OLSR",
            ANY_DEST,
            -1, // instance Id
            buf);
        sprintf(buf, "TC Messages Received = %u",
                olsrStat.numTcReceived);

        IO_PrintStat(
            node,
            "Application",
            "OLSR",
            ANY_DEST,
            -1, // instance Id
            buf);
        sprintf(buf, "TC Messages Generated = %u",
                olsrStat.numTcGenerated);

        IO_PrintStat(
            node,
            "Application",
            "OLSR",
            ANY_DEST,
            -1, // instance Id
            buf);
        sprintf(buf, "TC Messages Relayed = %u",
                olsrStat.numTcRelayed);

        IO_PrintStat(
            node,
            "Application",
            "OLSR",
            ANY_DEST,
            -1, // instance Id
            buf);
        sprintf(buf, "MID Messages Received = %u",
                olsrStat.numMidReceived);

        IO_PrintStat(
            node,
            "Application",
            "OLSR",
            ANY_DEST,
            -1, // instance Id
            buf);
        sprintf(buf, "MID Messages Generated = %u",
                olsrStat.numMidGenerated);

        IO_PrintStat(
            node,
            "Application",
            "OLSR",
            ANY_DEST,
            -1, // instance Id
            buf);
        sprintf(buf, "MID Messages Relayed = %u",
                olsrStat.numMidRelayed);

        IO_PrintStat(
            node,
            "Application",
            "OLSR",
            ANY_DEST,
            -1, // instance Id
            buf);
        sprintf(buf, "HNA Messages Received = %u",
                olsrStat.numHnaReceived);

        IO_PrintStat(
            node,
            "Application",
            "OLSR",
            ANY_DEST,
            -1, // instance Id
            buf);
        sprintf(buf, "HNA Messages Generated = %u",
                olsrStat.numHnaGenerated);

        IO_PrintStat(
            node,
            "Application",
            "OLSR",
            ANY_DEST,
            -1, // instance Id
            buf);
        sprintf(buf, "HNA Messages Relayed = %u",
                olsrStat.numHnaRelayed);

        IO_PrintStat(
            node,
            "Application",
            "OLSR",
            ANY_DEST,
            -1, // instance Id
            buf);

    }
}

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
void RoutingOlsrInriaFinalize(Node * node, Int32 )
{
    RoutingOlsr*  olsr = (RoutingOlsr* ) node->appData.olsr;
    if (!olsr->statsPrinted)
    {
        RoutingOlsrPrintStat(node);
        olsr->statsPrinted = TRUE;

        if (DEBUG_OUTPUT)
        {
            OlsrPrintTables(node);
            // NetworkPrintForwardingTable(node);
        }
    }
}

// /**
// FUNCTION   :: RoutingOlsrInriaLayer
// LAYER      :: APPLICATION
// PURPOSE    :: The main layer structure routine, called from application.cpp
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + msg : Message* : Message to be handled
// RETURN :: void : NULL
// **/
void RoutingOlsrInriaLayer(Node *node, Message *msg)
{
    unsigned char index;
    RoutingOlsr*  olsr = (RoutingOlsr* ) node->appData.olsr;
    if (olsr == NULL)
    {
        MESSAGE_Free(node, msg);
        return;
    }



     switch (msg->eventType)
    {
        case MSG_APP_OlsrPeriodicHello:
        {
            // hello periodic message expired. Generate hello message
            // and set same timer for next refresh. For all OLSR interfaces
            // call OlsrGenerateHello

            if (DEBUG)
            {
                printf ("Generating Hellos for Node %d on all"
                    " olsr-interfaces\n", node->nodeId);
            }

            for (index = 0; index < node->numberInterfaces; index++)
            {
                // Do not consider  non-olsr interfaces
                if (NetworkIpGetUnicastRoutingProtocolType(node,
                    index, olsr->ip_version) == ROUTING_PROTOCOL_OLSR_INRIA)
                {
                   OlsrGenerateHello(node,index);
                }
            }

            MESSAGE_Free(node, msg);

            // set refresh timer for hello message
            Message* helloPeriodicMsg = MESSAGE_Alloc(node,
                                            APP_LAYER,
                                            APP_ROUTING_OLSR_INRIA,
                                            MSG_APP_OlsrPeriodicHello);

            MESSAGE_Send(node, helloPeriodicMsg, olsr->hello_interval);

            break;
        }
        case MSG_APP_OlsrPeriodicTc:
        {
            // tc periodic message expired. Generate tc message and set
            // same timer for next refresh

            // For all olsr interfaces
            // generate tc

            if (DEBUG)
            {
                printf ("Generating TC for Node %d on all olsr-interfaces\n",
                                                               node->nodeId);
            }

            for (index = 0; index < node->numberInterfaces; index++)
            {
                // Do not consider  non-olsr interfaces
                if (NetworkIpGetUnicastRoutingProtocolType(node,
                    index, olsr->ip_version) == ROUTING_PROTOCOL_OLSR_INRIA)
                {
                    //Calling for TC generation
                    OlsrGenerateTc(node, index);
                }
            }
            MESSAGE_Free(node, msg);

            // set refresh timer for tc message
            Message* tcPeriodicMsg = MESSAGE_Alloc(node,
                                        APP_LAYER,
                                        APP_ROUTING_OLSR_INRIA,
                                        MSG_APP_OlsrPeriodicTc);

            MESSAGE_Send(node, tcPeriodicMsg, olsr->tc_interval);

            break;
        }
        case MSG_APP_OlsrPeriodicMid:
        {
            RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
            // Generate and send MID message for multiple olsr-interfaces
            if (olsr->numOlsrInterfaces <= 1)
            {
                MESSAGE_Free(node, msg);
                break;
            }
            // Mid periodic message expired. Generate mid message and set
            // same timer for next refresh

            if (DEBUG)
            {
                printf ("Generating Mid  for Node %d on all"
                    " olsr-interfaces\n", node->nodeId);
            }

            for (index = 0; index < node->numberInterfaces; index++)
            {
                // Do not consider  non-olsr interfaces
                if (NetworkIpGetUnicastRoutingProtocolType(node,
                    index, olsr->ip_version) == ROUTING_PROTOCOL_OLSR_INRIA)
                {
                    OlsrGenerateMid(node, index);
                }
            }

            MESSAGE_Free(node, msg);

            // set refresh timer for mid message
            Message* midPeriodicMsg = MESSAGE_Alloc(node,
                                          APP_LAYER,
                                          APP_ROUTING_OLSR_INRIA,
                                          MSG_APP_OlsrPeriodicMid);

            MESSAGE_Send(node, midPeriodicMsg, olsr->mid_interval);

            break;
        }
        case MSG_APP_OlsrPeriodicHna:
        {
            RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
            // Generate and send HNA message having non olsr-interfaces
            int noOfQualifiedInterfaces = 0;
            for (index = 0; index < node->numberInterfaces; index++)
            {
                if (NetworkIpGetInterfaceType(node, index)== olsr->ip_version
                   || NetworkIpGetInterfaceType(node, index) == NETWORK_DUAL)
                {
                    noOfQualifiedInterfaces++;
                }
            }
            if (olsr->numOlsrInterfaces == noOfQualifiedInterfaces)
            {
                MESSAGE_Free(node, msg);
                break;
            }
            // HNA periodic message expired. Generate hna message and set
            // same timer for next refresh

            if (DEBUG)
            {
                printf ("Generating Hna for Node %d on all"
                    " olsr-interfaces\n", node->nodeId);
            }

            for (index = 0; index < node->numberInterfaces; index++)
            {
                // Do not consider  non-olsr interfaces
                if (NetworkIpGetUnicastRoutingProtocolType(node,
                    index, olsr->ip_version) == ROUTING_PROTOCOL_OLSR_INRIA)
                {
                    OlsrGenerateHna(node, index);
                }
            }

            MESSAGE_Free(node, msg);

            // set refresh timer for mid message
            Message* hnaPeriodicMsg = MESSAGE_Alloc(node,
                                          APP_LAYER,
                                          APP_ROUTING_OLSR_INRIA,
                                          MSG_APP_OlsrPeriodicHna);

            MESSAGE_Send(node, hnaPeriodicMsg, olsr->hna_interval);

            break;
        }
        case MSG_APP_FromTransport:
        {
            // OLSR message received from Transport layer
            RoutingOlsrHandlePacket(node, msg);
            MESSAGE_Free(node, msg);
            break;
        }

        case MSG_APP_OlsrNeighHoldTimer:
        {
            // neighbor hold timer message expired. Refresh MPR selector and
            // neighbor table and resend send the hold timer message again

            Message *neighHoldMsg;

            OlsrTimeoutMprSelectorTable(node);

            if (DEBUG)
            {
                OlsrPrintLinkSet(node);
            }
            OlsrTimeoutLinkEntry(node);

            if (DEBUG)
            {
                OlsrPrintLinkSet(node);
            }
            OlsrTimeoutNeighborhoodTables(node);
            OlsrProcessChanges(node);

            MESSAGE_Free(node, msg);
            neighHoldMsg = MESSAGE_Alloc(node,
                                    APP_LAYER,
                                    APP_ROUTING_OLSR_INRIA,
                                    MSG_APP_OlsrNeighHoldTimer);

            MESSAGE_Send(node, neighHoldMsg, olsr->neighb_hold_time);
            break;
        }

        case MSG_APP_OlsrTopologyHoldTimer:
        {
            // topology hold timer message expired. Refresh topology table and
            // resend send the hold timer message again

            Message* topoHoldMsg;

            OlsrTimeoutTopologyTable(node);
            OlsrProcessChanges(node);

            MESSAGE_Free(node, msg);
            topoHoldMsg = MESSAGE_Alloc(node,
                                    APP_LAYER,
                                    APP_ROUTING_OLSR_INRIA,
                                    MSG_APP_OlsrTopologyHoldTimer);

            MESSAGE_Send(node, topoHoldMsg, olsr->top_hold_time);
            break;
        }

        case MSG_APP_OlsrDuplicateHoldTimer:
        {
            // duplicate hold timer message expired. Refresh duplicate table
            // and resend send the hold timer message again

            Message *duplicateHoldMsg;

            OlsrTimeoutDuplicateTable(node);
            OlsrProcessChanges(node);

            MESSAGE_Free(node, msg);
            duplicateHoldMsg = MESSAGE_Alloc(node,
                                    APP_LAYER,
                                    APP_ROUTING_OLSR_INRIA,
                                    MSG_APP_OlsrDuplicateHoldTimer);

            MESSAGE_Send(node,
                         duplicateHoldMsg,
                         olsr->dup_hold_time);
            break;
        }

        case MSG_APP_OlsrMidHoldTimer:
        {
            // mid hold timer message expired. Refresh mid table
            // and resend send the hold timer message again

            Message* midHoldMsg;

            OlsrTimeoutMidTable(node);
            OlsrProcessChanges(node);

            MESSAGE_Free(node, msg);
            midHoldMsg = MESSAGE_Alloc(node,
                                    APP_LAYER,
                                    APP_ROUTING_OLSR_INRIA,
                                    MSG_APP_OlsrMidHoldTimer);

            MESSAGE_Send(node,
                         midHoldMsg,
                         olsr->mid_hold_time);
            break;
        }
        case MSG_APP_OlsrHnaHoldTimer:
        {
            // HNA hold timer message expired. Refresh mid table
            // and resend send the hold timer message again

            Message* hnaHoldMsg;

            OlsrTimeoutHnaTable(node);
            OlsrProcessChanges(node);

            MESSAGE_Free(node, msg);
            hnaHoldMsg = MESSAGE_Alloc(node,
                                    APP_LAYER,
                                    APP_ROUTING_OLSR_INRIA,
                                    MSG_APP_OlsrHnaHoldTimer);

            MESSAGE_Send(node,
                         hnaHoldMsg,
                         olsr->hna_hold_time);
            break;
        }


        default:
        {
            MESSAGE_Free(node, msg);
            // invalid message
            ERROR_Assert(FALSE, "Invalid switch value");
        }
    }
 }



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
    Address* old_addr,
    NodeAddress subnetMask,
    NetworkType networkType)
{
    RoutingOlsr* olsr = (RoutingOlsr*)node->appData.olsr;
    unsigned char index = 0;

    Address new_addr;
    NetworkDataIp* ip = node->networkData.networkVar;
    IpInterfaceInfoType* interfaceInfo =
                   ip->interfaceInfo[interfaceIndex];

    if (ip->interfaceInfo[interfaceIndex]->routingProtocolType
        != ROUTING_PROTOCOL_OLSR_INRIA )
    {
        return;
    }
    if (networkType == NETWORK_IPV6)
    {
        return;
    }
    if (interfaceInfo->addressState == INVALID)
    {
        return;
    }
    // initializing variables
    memset(&new_addr, 0, sizeof(Address));

    // extracting new address
    new_addr.interfaceAddr.ipv4 = interfaceInfo->ipAddress;
    new_addr.networkType = NETWORK_IPV4;

    if (Address_IsSameAddress(old_addr, &olsr->main_address))
    {
        // changing the olsr node's main address
        NetworkGetInterfaceInfo(node,
                                interfaceIndex,
                                &olsr->main_address,
                                NETWORK_IPV4);
    }

    if (NetworkIpGetUnicastRoutingProtocolType(
           node,
           interfaceIndex,
           NETWORK_IPV4) == ROUTING_PROTOCOL_OLSR_INRIA)
    {
        if (DEBUG)
        {
            printf("Generating explicit Hello message for Node %d on all"
                   " olsr-interfaces\n",
                   node->nodeId);
            if (olsr->numOlsrInterfaces > 1)
            {
                printf("Generating explicit Mid message also for Node %d on"
                       "all olsr-interfaces\n",
                       node->nodeId);
            }
        }

        link_entry* link_set = olsr->link_set;
        while (link_set)
        {
            link_set->local_iface_addr.interfaceAddr.ipv4 =
                NetworkIpGetInterfaceAddress(node, 0);
            link_set = link_set->next;
        }
        for (index = 0; index < node->numberInterfaces; index++)
        {
            // Do not consider  non-olsr interfaces
            if (NetworkIpGetUnicastRoutingProtocolType(
                node, index, NETWORK_IPV4) == ROUTING_PROTOCOL_OLSR_INRIA)
            {
                OlsrGenerateHello(node,index);

                if (olsr->numOlsrInterfaces > 1)
                {
                    OlsrGenerateMid(node, index);
                }
            }
        }// For loop
    }
    else
    {
        if (DEBUG || DEBUG_OUTPUT)
        {
            if (olsr->numOlsrInterfaces != node->numberInterfaces)
            {
                printf("Generating explicit Hna message for Node %d on all"
                       " olsr-interfaces\n",
                       node->nodeId);
            }
        }

        if (olsr->numOlsrInterfaces != node->numberInterfaces)
        {
            for (index = 0; index < node->numberInterfaces; index++)
            {
                // Do not consider  non-olsr interfaces
                if (NetworkIpGetUnicastRoutingProtocolType(
                       node,
                       index,
                       NETWORK_IPV4) == ROUTING_PROTOCOL_OLSR_INRIA)
                {
                    OlsrGenerateHna(node, index);
                }
            }// For loop
        }
    }
}

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
    in6_addr* oldGlobalAddress)
{
    RoutingOlsr* olsr = (RoutingOlsr* ) node->appData.olsr;
    unsigned char index;
    Address old_addr;
    Address new_addr;

    //initializing variables
    memset(&old_addr,0,sizeof(Address));
    memset(&new_addr,0,sizeof(Address));

    if (olsr == NULL)
    {
        // invalid call
        ERROR_Assert(FALSE, "Invalid call to this function.");
    }

    IPv6Data* ipv6 = (IPv6Data *)node->networkData.networkVar->ipv6;

    IPv6InterfaceInfo* ipv6InterfaceInfo =
                  ipv6->ip->interfaceInfo[interfaceIndex]->ipv6InterfaceInfo;

    COPY_ADDR6(*oldGlobalAddress, old_addr.interfaceAddr.ipv6);
    old_addr.networkType = NETWORK_IPV6;

    //extracting new address
    COPY_ADDR6(ipv6InterfaceInfo->globalAddr, new_addr.interfaceAddr.ipv6);
    new_addr.networkType = NETWORK_IPV6;

    if (DEBUG)
    {
        char addrString1[MAX_STRING_LENGTH];
        char addrString2[MAX_STRING_LENGTH];
        char strTime[MAX_STRING_LENGTH];

        ctoa(node->getNodeTime(), strTime);
        IO_ConvertIpAddressToString(&old_addr,addrString1);
        IO_ConvertIpAddressToString(&new_addr,addrString2);

        printf ("Receive notification of address change on node %d."
                " Old Address: %s with prefix length %d"
                " New Address: %s with prefix length %d"
                " at simulation time %s\n",
                node->nodeId, addrString1,
        ipv6InterfaceInfo->autoConfigParam.depGlobalAddrPrefixLen,
        addrString2,
        ipv6InterfaceInfo->prefixLen,
        strTime);
    }

    if (Address_IsSameAddress(&old_addr, &olsr->main_address))
    {
        //changing the olsr node's main address
        NetworkGetInterfaceInfo(node, interfaceIndex,
                                &olsr->main_address, NETWORK_IPV6);

        if (DEBUG)
        {
            char addrString1[MAX_STRING_LENGTH];
            char addrString2[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(&old_addr,addrString1);
            IO_ConvertIpAddressToString(&olsr->main_address,addrString2);
            printf ("Changing node's %d main address from %s to %s"
                        " with prefix\n",
                    node->nodeId,addrString1,addrString2);
        }
    }

    if (NetworkIpGetUnicastRoutingProtocolType(
          node, interfaceIndex, NETWORK_IPV6) == ROUTING_PROTOCOL_OLSR_INRIA)
    {
        if (DEBUG)
        {
            printf ("Generating explicit Hello message for Node %d on all"
                    " olsr-interfaces\n", node->nodeId);
            if (olsr->numOlsrInterfaces > 1)
            {
            printf ("Generating explicit Mid message also for Node %d on all"
                    " olsr-interfaces\n", node->nodeId);
            }
        }

        for (index = 0; index < node->numberInterfaces; index++)
        {
            // Do not consider  non-olsr interfaces
            if (NetworkIpGetUnicastRoutingProtocolType(
                node, index, NETWORK_IPV6) == ROUTING_PROTOCOL_OLSR_INRIA)
            {
               OlsrGenerateHello(node,index);

               if (olsr->numOlsrInterfaces > 1)
               {
                   OlsrGenerateMid(node, index);
               }
            }
        }//For loop
    }
    else
    {
        if (DEBUG || DEBUG_OUTPUT)
        {
            if (olsr->numOlsrInterfaces != node->numberInterfaces)
            {
            printf ("Generating explicit Hna message for Node %d on all"
                    " olsr-interfaces\n", node->nodeId);
            }
        }

        if (olsr->numOlsrInterfaces != node->numberInterfaces)
        {
            for (index = 0; index < node->numberInterfaces; index++)
            {
                // Do not consider  non-olsr interfaces
                if (NetworkIpGetUnicastRoutingProtocolType(
                   node, index, NETWORK_IPV6) == ROUTING_PROTOCOL_OLSR_INRIA)
                {
                    OlsrGenerateHna(node, index);
                }
            }//For loop
        }
    }
}
