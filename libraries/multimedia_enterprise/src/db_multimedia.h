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

#ifndef DB_MULTIMEDIA_H
#define DB_MULTIMEDIA_H


#include <string>
#include <vector>
#include <iostream>
#include <list>

#include "fileio.h"
#include "node.h"
#ifdef ADDON_DB
#include "db.h"
#include "gestalt.h"
#include "db-core.h"
#endif
using namespace std;
#define STATSDB_DEFAULT_OSPF_NEIGHBOR_STATE_INTERVAL (600 * SECOND)
#define STATSDB_DEFAULT_OSPF_ROUTER_LSA_INTERVAL (600 * SECOND)
#define STATSDB_DEFAULT_OSPF_NETWORK_LSA_INTERVAL (600 * SECOND)
#define STATSDB_DEFAULT_OSPF_SUMMARY_LSA_INTERVAL (600 * SECOND)
#define STATSDB_DEFAULT_OSPF_EXTERNAL_LSA_INTERVAL (600 * SECOND)
#define STATSDB_DEFAULT_OSPF_INTERFACE_STATE_INTERVAL (600 * SECOND)
#define STATSDB_DEFAULT_OSPF_AGGREGATE_INTERVAL (600 * SECOND)
#define STATSDB_DEFAULT_OSPF_SUMMARY_INTERVAL (600 * SECOND)

// Protocol Specific Code
class StatsDBMospfSummary
{
public:
    int m_NumGroupLSAGenerated;
    int m_NumGroupLSAFlushed;
    int m_NumGroupLSAReceived;
    int m_NumRouterLSA_WCMRSent;
    int m_NumRouterLSA_WCMRReceived;
    int m_NumRouterLSA_VLEPSent;
    int m_NumRouterLSA_VLEPReceived;
    int m_NumRouterLSA_ASBRSent;
    int m_NumRouterLSA_ASBRReceived;
    int m_NumRouterLSA_ABRSent;
    int m_NumRouterLSA_ABRReceived;
    //constructor
    StatsDBMospfSummary();
};


class StatsDBOspfTable
{
public:

    StatsDBOspfTable();
    std::vector<std::pair<std::string, std::string> > ospfNeighborStateTableDef;
    std::vector<std::pair<std::string, std::string> > ospfRouterLsaTableDef;
    std::vector<std::pair<std::string, std::string> > ospfNetworkLsaTableDef;
    std::vector<std::pair<std::string, std::string> > ospfSummaryLsaTableDef;
    std::vector<std::pair<std::string, std::string> > ospfExternalLsaTableDef;
    std::vector<std::pair<std::string, std::string> > ospfInterfaceStateTableDef;
    std::vector<std::pair<std::string, std::string> > ospfSummaryTableDef;
    std::vector<std::pair<std::string, std::string> > ospfAggregateTableDef;
    BOOL createOspfNeighborStateTable;
    BOOL createOspfRouterLsaTable;
    BOOL createOspfNetworkLsaTable;
    BOOL createOspfSummaryLsaTable;
    BOOL createOspfExternalLsaTable;
    BOOL createOspfInterfaceStateTable;
    BOOL createOspfAggregateTable;
    BOOL createOspfSummaryTable;
    BOOL createMospfSummaryTable;

    clocktype ospfNeighborStateTableInterval;
    clocktype ospfRouterLsaTableInterval;
    clocktype ospfNetworkLsaTableInterval;
    clocktype ospfSummaryLsaTableInterval;
    clocktype ospfExternalLsaTableInterval;
    clocktype ospfInterfaceStateTableInterval;
    clocktype ospfAggregateTableInterval;
    clocktype ospfSummaryTableInterval;
    //this interval is currently same as STATS-DB-SUMMARY-INTERVAL
    //and is used for MOSPF_Summary table
    clocktype mospfSummaryInterval;
};

class StatsDbOspfLsa
{
public:
    StatsDbOspfLsa();
    int areaId;
    int linkStateAge;
    std::string advRouterAddr;
    std::string linkId;
    std::string linkData;
    std::string linkType;
    std::string routerAddr;
    int cost;
};

class StatsDbOspfState
{
public:
    StatsDbOspfState();
    std::string ipAddr;
    std::string state;
    int areaId;
    std::string mobileLeaf;
    std::string neighborAddr;
};

class StatsDbOspfStats
{
public:
    StatsDbOspfStats();
    std::string ipAddr;
    Int64 numLSUpdateSent;
    Int64 numLSUpdateBytesSent;
    Int64 numLSUpdateRecv;
    Int64 numLSUpdateBytesRecv;

    Int64 numAckSent;
    Int64 numAckBytesSent;
    Int64 numAckRecv;
    Int64 numAckBytesRecv;

    Int64 numDDPktSent;
    Int64 numDDPktBytesSent;
    Int64 numDDPktRecv;
    Int64 numDDPktRxmt;
    Int64 numDDPktBytesRxmt;
    Int64 numDDPktBytesRecv;

    Int64 numLSReqSent;
    Int64 numLSReqBytesSent;
    Int64 numLSReqRecv;
    Int64 numLSReqRxmt;
    Int64 numLSReqBytesRecv;

    double offeredLoad;
};

void InitializeStatsDBOspfTable(PartitionData* paritionData,
                                NodeInput* nodeInput);

void InitializeStatsDBOspfNeighborStateTable(PartitionData* paritionData,
                                             NodeInput* nodeInput);
void InitializeStatsDBOspfRouterLsaTable(PartitionData* partitionData,
                                         NodeInput* nodeInput);
void InitializeStatsDBOspfNetworkLsaTable(PartitionData* partitionData,
                                          NodeInput* nodeInput);
void InitializeStatsDBOspfSummaryLsaTable(PartitionData* partitionData,
                                          NodeInput* nodeInput);
void InitializeStatsDBOspfExternalLsaTable(PartitionData* partitionData,
                                           NodeInput* nodeInput);
void InitializeStatsDBOspfInterfaceStateTable(PartitionData* paritionData,
                                              NodeInput* nodeInput);
void InitializeStatsDBOspfSummaryTable(PartitionData* paritionData,
                                       NodeInput* nodeInput);
void InitializeStatsDBOspfAggregateTable(PartitionData* paritionData,
                                         NodeInput* nodeInput);

//init for mospf summary table
void InitializeStatsDBMospfSummaryTable(PartitionData* paritionData,
                                        NodeInput* nodeInput);

void StatsDBSendOspfTableTimerMessage(PartitionData* partition);

/**
* To get the first node on each partition that is actually running OSPF.
*
* @param partition Data structure containing partition information
* @return First node on the partition that is running OSPF
*/
Node* StatsDBOspfGetInitialNodePointer(PartitionData* partition);

void HandleStatsDBOspfNeighborStateTableInsertion(Node* node);
void HandleStatsDBOspfRouterLsaTableInsertion(Node* node);
void HandleStatsDBOspfNetworkLsaTableInsertion(Node* node);
void HandleStatsDBOspfSummaryLsaTableInsertion(Node* node);
void HandleStatsDBOspfExternalLsaTableInsertion(Node* node);
void HandleStatsDBOspfInterfaceStateTableInsertion(Node* node);
void HandleStatsDBOspfSummaryTableInsertion(Node* node);
void HandleStatsDBOspfAggregateTableInsertion(Node* node);

//for mospf summary table
void HandleStatsDBMospfSummaryTableInsertion(Node* node);
void HandleStatsDBMospfMulticastNetSummaryTableInsertion(Node* node);

void STATSDB_GetDatabaseOspfStructure(Node* node);
void STATSDB_HandleOspfNeighborStateTableInsert(Node* node,
                                                const StatsDbOspfState & stats);
void STATSDB_HandleOspfRouterLsaTableInsert(Node* node,
                                            const StatsDbOspfLsa & stats);
void STATSDB_HandleOspfNetworkLsaTableInsert(Node* node,
                                             const StatsDbOspfLsa& stats);
void STATSDB_HandleOspfSummaryLsaTableInsert(Node* node,
                                             const StatsDbOspfLsa& stats);
void STATSDB_HandleOspfExternalLsaTableInsert(Node* node,
                                              const StatsDbOspfLsa& stats);
void STATSDB_HandleOspfInterfaceStateTableInsert(Node* node,
                                                const StatsDbOspfState & stats);
void STATSDB_HandleOspfSummaryTableInsert(Node* node,
                                          const StatsDbOspfStats & stats);
void STATSDB_HandleOspfAggregateTableInsert(Node* node,
                                            const StatsDbOspfStats & stats);
void STATSDB_HandleOspfAggregateParallelRetreiveExisting(
    Node* node,
    StatsDbOspfStats* stats);

void STATSDB_HandleOspfAggregateTableUpdate(Node* node,
                                            const StatsDbOspfStats & stats);

//for mospf summary table
void STATSDB_HandleMospfSummaryTableInsert(Node* node,
                                           const StatsDBMospfSummary& stats);

/********************** PIM Specific Code Starts****************************/
class StatsDBPimSmStatus
{
public:
    std::string m_BsrType;
    std::string m_RpType;
    std::string m_GroupAddr;
    std::vector<std::string> m_GroupAddrRange;
    BOOL m_IsDR;
    std::vector<std::string> m_DrNetworkAddr;

    //constructor
    StatsDBPimSmStatus();
};

class StatsDBPimSmSummary
{
public:
    int m_NumHelloSent;
    int m_NumHelloRecvd;
    int m_NumJoinPruneSent;
    int m_NumJoinPruneRecvd;
    int m_NumCandidateRPSent;
    int m_NumCandidateRPRecvd;
    int m_NumBootstrapSent;
    int m_NumBootstrapRecvd;
    int m_NumRegisterSent;
    int m_NumRegisterRecvd;
    int m_NumRegisterStopSent;
    int m_NumRegisterStopRecvd;
    int m_NumAssertSent;
    int m_NumAssertRecvd;

    //constructor
    StatsDBPimSmSummary();
};

class StatsDBPimDmSummary
{
public:
    int m_NumHelloSent;
    int m_NumHelloRecvd;
    int m_NumJoinPruneSent;
    int m_NumJoinPruneRecvd;
    int m_NumGraftSent;
    int m_NumGraftRecvd;
    int m_NumGraftAckSent;
    int m_NumGraftAckRecvd;
    int m_NumAssertSent;
    int m_NumAssertRecvd;

    //constructor
    StatsDBPimDmSummary();
};



class StatsDBPimTable
{
public:
    //data members
    BOOL createPimSmStatusTable;
    BOOL createPimSmSummaryTable;
    BOOL createPimDmSummaryTable;

    //this interval is currently same as STATS-DB-STATUS-INTERVAL
    clocktype pimStatusInterval;
    //this interval is currently same as STATS-DB-SUMMARY-INTERVAL
    clocktype pimSummaryInterval;

    //std::vector<StatsDBPimSmStatus> pimSmStatusTableDef;
    //std::vector<StatsDBPimSmSummary> pimSmSummaryTableDef;
    //std::vector<StatsDBPimDmSummary> pimDmSummaryTableDef;

    //constructor
    StatsDBPimTable();
};

void InitializeStatsDBPimTable(PartitionData* paritionData,
                                NodeInput* nodeInput);

void InitializeStatsDBPimSmStatusTable(PartitionData* paritionData,
                                             NodeInput* nodeInput);
void InitializeStatsDBPimSmSummaryTable(PartitionData* partitionData,
                                         NodeInput* nodeInput);
void InitializeStatsDBPimDmSummaryTable(PartitionData* partitionData,
                                          NodeInput* nodeInput);

void StatsDBSendPimTableTimerMessage(PartitionData* partition);

/**
* To get the first node on each partition that is actually running PIM.
*
* @param partition Data structure containing partition information
* @return First node on the partition that is running PIM
*/
Node* StatsDBPimGetInitialNodePointer(PartitionData* partition);

void HandleStatsDBPimSmStatusTableInsertion(Node* node);
void HandleStatsDBPimSmSummaryTableInsertion(Node* node);
void HandleStatsDBPimDmSummaryTableInsertion(Node* node);
void HandleStatsDBPimMulticastNetSummaryTableInsertion(Node* node);

void STATSDB_HandlePimSmStatusTableInsert(Node* node,
                                           const StatsDBPimSmStatus & stats);
void STATSDB_HandlePimSmSummaryTableInsert(Node* node,
                                           const StatsDBPimSmSummary& stats);
void STATSDB_HandlePimDmSummaryTableInsert(Node* node,
                                           const StatsDBPimDmSummary& stats);

/********************** PIM Specific Code Ends******************************/
#endif
