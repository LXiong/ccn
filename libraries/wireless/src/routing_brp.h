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

// Note: This is BRP implementation following draft
//       draft-ietf-manet-zone-brp-02.txt. This is not a separate protocol
//       but a library that IERP can use to optimize route discovery.


#ifndef _BRP_H_

#define _BRP_H_

// IERP is going to send another request after 30 seconds. So
// previous query information should be deleted before that.

#define BRP_DEFAULT_REQUEST_TIMEOUT (30 * SECOND)

#define BRP_INITIAL_PERIPHERAL_ENTRY_SIZE 10

#define BRP_NULL_CACHE_ID 0xffffffff

#define BRP_MAX_NODE_ENTRY 1024

#define IARP_INITIAL_ROUTE_TABLE_ENTRY   8

// Brp packet format following draft-ietf-manet-zone-brp-02.txt
typedef struct brp_hdr_struct
{
    NodeAddress querySrcAddr;
    NodeAddress queryDestAddr;
    unsigned short queryId;
    unsigned char queryExtn;
    unsigned char reserved;
    NodeAddress prevBordercastAddr;
} BrpHeader;

typedef struct brp_periphral_entry_struct
{
    NodeAddress nextHop;
    NodeAddress peripheralNode;
    BOOL        isCovered;
} BrpPeripheralEntry;

typedef BrpPeripheralEntry* BrpNetGraph;

typedef struct brp_query_coverage_entry
{
    NodeAddress nodeId;           // source of the route query
    unsigned int queryId;
    unsigned int brpCacheId;
    clocktype insertionTime;
    NodeAddress prevBordercaster; // previous node from which received
                                  // query
    BOOL isCovered;
    BrpNetGraph graph;
    unsigned int numPeripheralNode;
    unsigned int numUncoveredPeripheral;
    struct brp_query_coverage_entry* next;
} BrpQueryCoverageEntry;

typedef struct brp_query_coverage_table
{
    unsigned int numEntry;
    BrpQueryCoverageEntry* coverageEntry;
} BrpQueryCoverageTable;


typedef struct brp_stat_struct
{
    unsigned int numQuerySent;
    unsigned int numQueryReceived;
} BrpStat;

typedef struct brp_data_str
{
    BrpQueryCoverageTable queryCoverageTable;
    unsigned int queryId;
    unsigned int brpCacheId;
    BrpStat stat;
    int printStats;
    BOOL statsPrinted;
} BrpData;

void
BrpDeliver(
    Node* node,
    Message* msg,
    NodeAddress src,
    NodeAddress dest,
    unsigned int interfaceIndex,
    unsigned int ttl);

void
BrpSend(
    Node* node,
    IerpData* ierpData,
    Message* msg,
    NodeAddress dest,
    unsigned int brpCacheId);

void
BrpInit(Node* node, BrpData* brpData);

void
BrpPrintStats(Node* node);

void
BrpProcessEvent(Node* node, Message* msg);

void
BrpFinalize(Node* node);

#endif
