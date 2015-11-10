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

#include <string>
#include <vector>
#include <iostream>
#include <math.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#endif

#ifdef ADDON_DB
#include "api.h"
#include "dbapi.h"
#include "partition.h"
#include "WallClock.h"
#include "external_util.h"
#include "db_multimedia.h"
#include "db.h"
#include "network_ip.h"

#ifdef PARALLEL //Parallel
#include "parallel.h"
#endif

#ifdef USE_MPI
#include <mpi.h>
#endif


#include "db-dynamic.h"

StatsDBOspfTable::StatsDBOspfTable()
{
    createOspfNeighborStateTable = FALSE;
    createOspfRouterLsaTable = FALSE;
    createOspfNetworkLsaTable = FALSE;
    createOspfSummaryLsaTable = FALSE;
    createOspfExternalLsaTable = FALSE;
    createOspfInterfaceStateTable = FALSE;
    createOspfAggregateTable = FALSE;
    createOspfSummaryTable = FALSE;
    createMospfSummaryTable = FALSE;

    ospfNeighborStateTableInterval
        = STATSDB_DEFAULT_OSPF_NEIGHBOR_STATE_INTERVAL;
    ospfRouterLsaTableInterval = STATSDB_DEFAULT_OSPF_ROUTER_LSA_INTERVAL;
    ospfNetworkLsaTableInterval = STATSDB_DEFAULT_OSPF_NETWORK_LSA_INTERVAL;
    ospfSummaryLsaTableInterval = STATSDB_DEFAULT_OSPF_SUMMARY_LSA_INTERVAL;
    ospfExternalLsaTableInterval = STATSDB_DEFAULT_OSPF_EXTERNAL_LSA_INTERVAL;
    ospfInterfaceStateTableInterval
        = STATSDB_DEFAULT_OSPF_INTERFACE_STATE_INTERVAL;
    ospfAggregateTableInterval = STATSDB_DEFAULT_OSPF_AGGREGATE_INTERVAL;
    ospfSummaryTableInterval = STATSDB_DEFAULT_OSPF_SUMMARY_INTERVAL;
    mospfSummaryInterval = STATSDB_DEFAULT_SUMMARY_INTERVAL;
}

StatsDbOspfLsa::StatsDbOspfLsa()
{
    areaId = 0;
    linkStateAge = 0;
    cost = 0;
}

StatsDbOspfState::StatsDbOspfState()
{
    areaId = 0;
}

StatsDbOspfStats::StatsDbOspfStats()
{
    numLSUpdateSent = 0;
    numLSUpdateBytesSent = 0;
    numLSUpdateRecv = 0;
    numLSUpdateBytesRecv = 0;

    numAckSent = 0;
    numAckBytesSent = 0;
    numAckRecv = 0;
    numAckBytesRecv = 0;

    numDDPktSent = 0;
    numDDPktBytesSent = 0;
    numDDPktRecv = 0;
    numDDPktRxmt = 0;
    numDDPktBytesRxmt = 0;
    numDDPktBytesRecv = 0;

    numLSReqSent = 0;
    numLSReqBytesSent = 0;
    numLSReqRecv = 0;
    numLSReqRxmt = 0;
    numLSReqBytesRecv = 0;

    offeredLoad = 0.0;
}


StatsDBMospfSummary::StatsDBMospfSummary()
{
    m_NumGroupLSAGenerated = 0;
    m_NumGroupLSAFlushed = 0;
    m_NumGroupLSAReceived = 0;
    m_NumRouterLSA_WCMRSent = 0;
    m_NumRouterLSA_WCMRReceived = 0;
    m_NumRouterLSA_VLEPSent = 0;
    m_NumRouterLSA_VLEPReceived = 0;
    m_NumRouterLSA_ASBRSent = 0;
    m_NumRouterLSA_ASBRReceived = 0;
    m_NumRouterLSA_ABRSent = 0;
    m_NumRouterLSA_ABRReceived = 0;
}



void InitializeStatsDBOspfTable(PartitionData* partition,
                                NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL wasFound = FALSE;
    BOOL ospfTableCreated = FALSE;

    StatsDb* db = NULL;
    db = partition->statsDb;
    db->statsOspfTable = NULL;

    // Check which tables are to be defined in the Protocol table
    // category. Check for the user configurations.

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "STATS-DB-OSPF-NEIGHBOR-STATE-TABLE",
        &wasFound,
        buf);
    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            if (!ospfTableCreated)
            {
                db->statsOspfTable = new StatsDBOspfTable();
                ospfTableCreated = TRUE;
            }

            // Initialize the Neighbor State table.
            db->statsOspfTable->createOspfNeighborStateTable = TRUE;
            InitializeStatsDBOspfNeighborStateTable(partition, nodeInput);
        }
        else if (strcmp(buf, "NO") != 0)
        {
            // We have invalid values.
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-NEIGHBOR-STATE-TABLE"
                "parameter, using Default\n");
        }
    }

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "STATS-DB-OSPF-ROUTER-LSA-TABLE",
        &wasFound,
        buf);
    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            if (!ospfTableCreated)
            {
                db->statsOspfTable = new StatsDBOspfTable();
                ospfTableCreated = TRUE;
            }

            // Initialize the Neighbor State table.
            db->statsOspfTable->createOspfRouterLsaTable = TRUE;
            InitializeStatsDBOspfRouterLsaTable(partition, nodeInput);
        }
        else if (strcmp(buf, "NO") != 0)
        {
            // We have invalid values.
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-ROUTER-LSA-TABLE"
                "parameter, using Default\n");
        }
    }

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "STATS-DB-OSPF-NETWORK-LSA-TABLE",
        &wasFound,
        buf);
    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            if (!ospfTableCreated)
            {
                db->statsOspfTable = new StatsDBOspfTable();
                ospfTableCreated = TRUE;
            }

            // Initialize the Neighbor State table.
            db->statsOspfTable->createOspfNetworkLsaTable = TRUE;
            InitializeStatsDBOspfNetworkLsaTable(partition, nodeInput);
        }
        else if (strcmp(buf, "NO") != 0)
        {
            // We have invalid values.
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-NETWORK-LSA-TABLE"
                "parameter, using Default\n");
        }
    }

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "STATS-DB-OSPF-SUMMARY-LSA-TABLE",
        &wasFound,
        buf);
    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            if (!ospfTableCreated)
            {
                db->statsOspfTable = new StatsDBOspfTable();
                ospfTableCreated = TRUE;
            }

            // Initialize the Neighbor State table.
            db->statsOspfTable->createOspfSummaryLsaTable = TRUE;
            InitializeStatsDBOspfSummaryLsaTable(partition, nodeInput);
        }
        else if (strcmp(buf, "NO") != 0)
        {
            // We have invalid values.
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-SUMMARY-LSA-TABLE"
                "parameter, using Default\n");
        }
    }

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "STATS-DB-OSPF-EXTERNAL-LSA-TABLE",
        &wasFound,
        buf);
    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            if (!ospfTableCreated)
            {
                db->statsOspfTable = new StatsDBOspfTable();
                ospfTableCreated = TRUE;
            }

            // Initialize the Neighbor State table.
            db->statsOspfTable->createOspfExternalLsaTable = TRUE;
            InitializeStatsDBOspfExternalLsaTable(partition, nodeInput);
        }
        else if (strcmp(buf, "NO") != 0)
        {
            // We have invalid values.
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-EXTERNAL-LSA-TABLE"
                "parameter, using Default\n");
        }
    }

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "STATS-DB-OSPF-INTERFACE-STATE-TABLE",
        &wasFound,
        buf);
    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            if (!ospfTableCreated)
            {
                db->statsOspfTable = new StatsDBOspfTable();
                ospfTableCreated = TRUE;
            }

            // Initialize the Neighbor State table.
            db->statsOspfTable->createOspfInterfaceStateTable = TRUE;
            InitializeStatsDBOspfInterfaceStateTable(partition, nodeInput);
        }
        else if (strcmp(buf, "NO") != 0)
        {
            // We have invalid values.
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-INTERFACE-STATE-TABLE"
                "parameter, using Default\n");
        }
    }

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "STATS-DB-OSPF-SUMMARY-TABLE",
        &wasFound,
        buf);
    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            if (!ospfTableCreated)
            {
                db->statsOspfTable = new StatsDBOspfTable();
                ospfTableCreated = TRUE;
            }

            // Initialize the Neighbor State table.
            db->statsOspfTable->createOspfSummaryTable = TRUE;
            InitializeStatsDBOspfSummaryTable(partition, nodeInput);
        }
        else if (strcmp(buf, "NO") != 0)
        {
            // We have invalid values.
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-SUMMARY-TABLE"
                "parameter, using Default\n");
        }
    }

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "STATS-DB-OSPF-AGGREGATE-TABLE",
        &wasFound,
        buf);
    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            if (!ospfTableCreated)
            {
                db->statsOspfTable = new StatsDBOspfTable();
                ospfTableCreated = TRUE;
            }

            // Initialize the Neighbor State table.
            db->statsOspfTable->createOspfAggregateTable = TRUE;
            InitializeStatsDBOspfAggregateTable(partition, nodeInput);
        }
        else if (strcmp(buf, "NO") != 0)
        {
            // We have invalid values.
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-AGGREGATE-TABLE"
                "parameter, using Default\n");
        }
    }

    //init related to MOSPF_Summary table
    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "STATS-DB-MULTICAST-MOSPF-SUMMARY-TABLE",
        &wasFound,
        buf);
    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            if (!ospfTableCreated)
            {
                db->statsOspfTable = new StatsDBOspfTable();
                ospfTableCreated = TRUE;
            }

            // Initialize the MOSPf_Summary table.
            db->statsOspfTable->createMospfSummaryTable = TRUE;
            if (partition->partitionId == 0)
            {
                InitializeStatsDBMospfSummaryTable(partition, nodeInput);
            }
        }
        else if (strcmp(buf, "NO") != 0)
        {
            // We have invalid values.
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-MULTICAST-MOSPF-SUMMARY-TABLE"
                "parameter, using Default\n");
        }
    }

    if (db->statsOspfTable)
    {
        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "STATS-DB-OSPF-NEIGHBOR-STATE-INTERVAL",
            &wasFound,
            buf);

        if (wasFound)
        {
            db->statsOspfTable->ospfNeighborStateTableInterval
                = TIME_ConvertToClock(buf);
        }

        if (db->statsOspfTable->ospfNeighborStateTableInterval <= 0)
        {
            // Set default value and print warning
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-NEIGHBOR-STATE-INTERVAL"
                "parameter, using Default\n");
            db->statsOspfTable->ospfNeighborStateTableInterval
                = STATSDB_DEFAULT_OSPF_NEIGHBOR_STATE_INTERVAL;

        }

        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "STATS-DB-OSPF-ROUTER-LSA-INTERVAL",
            &wasFound,
            buf);

        if (wasFound)
        {
            db->statsOspfTable->ospfRouterLsaTableInterval
                = TIME_ConvertToClock(buf);
        }

        if (db->statsOspfTable->ospfRouterLsaTableInterval <= 0)
        {
            // Set default value and print warning
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-ROUTER-LSA-INTERVAL"
                "parameter, using Default\n");
            db->statsOspfTable->ospfRouterLsaTableInterval
                = STATSDB_DEFAULT_OSPF_ROUTER_LSA_INTERVAL;
        }

        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "STATS-DB-OSPF-NETWORK-LSA-INTERVAL",
            &wasFound,
            buf);

        if (wasFound)
        {
            db->statsOspfTable->ospfNetworkLsaTableInterval
                = TIME_ConvertToClock(buf);
        }

        if (db->statsOspfTable->ospfNetworkLsaTableInterval <= 0)
        {
            // Set default value and print warning
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-NETWORK-LSA-INTERVAL"
                "parameter, using Default\n");
            db->statsOspfTable->ospfNetworkLsaTableInterval
                = STATSDB_DEFAULT_OSPF_NETWORK_LSA_INTERVAL;
        }

        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "STATS-DB-OSPF-SUMMARY-LSA-INTERVAL",
            &wasFound,
            buf);

        if (wasFound)
        {
            db->statsOspfTable->ospfSummaryLsaTableInterval
                = TIME_ConvertToClock(buf);
        }

        if (db->statsOspfTable->ospfSummaryLsaTableInterval <= 0)
        {
            // Set default value and print warning
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-SUMMARY-LSA-INTERVAL"
                "parameter, using Default\n");
            db->statsOspfTable->ospfSummaryLsaTableInterval
                = STATSDB_DEFAULT_OSPF_SUMMARY_LSA_INTERVAL;
        }

        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "STATS-DB-OSPF-EXTERNAL-LSA-INTERVAL",
            &wasFound,
            buf);

        if (wasFound)
        {
            db->statsOspfTable->ospfExternalLsaTableInterval
                = TIME_ConvertToClock(buf);
        }

        if (db->statsOspfTable->ospfExternalLsaTableInterval <= 0)
        {
            // Set default value and print warning
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-EXTERNAL-LSA-INTERVAL"
                "parameter, using Default\n");
            db->statsOspfTable->ospfExternalLsaTableInterval
                = STATSDB_DEFAULT_OSPF_EXTERNAL_LSA_INTERVAL;
        }

        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "STATS-DB-OSPF-INTERFACE-STATE-INTERVAL",
            &wasFound,
            buf);

        if (wasFound)
        {
            db->statsOspfTable->ospfInterfaceStateTableInterval
                = TIME_ConvertToClock(buf);
        }

        if (db->statsOspfTable->ospfInterfaceStateTableInterval <= 0)
        {
            // Set default value and print warning
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-INTERFACE-STATE-INTERVAL"
                "parameter, using Default\n");
            db->statsOspfTable->ospfInterfaceStateTableInterval
                = STATSDB_DEFAULT_OSPF_INTERFACE_STATE_INTERVAL;
        }

        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "STATS-DB-OSPF-SUMMARY-INTERVAL",
            &wasFound,
            buf);

        if (wasFound)
        {
            db->statsOspfTable->ospfSummaryTableInterval
                = TIME_ConvertToClock(buf);
        }

        if (db->statsOspfTable->ospfSummaryTableInterval <= 0)
        {
            // Set default value and print warning
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-SUMMARY-INTERVAL"
                "parameter, using Default\n");
            db->statsOspfTable->ospfSummaryTableInterval
                = STATSDB_DEFAULT_OSPF_SUMMARY_INTERVAL;
        }

        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "STATS-DB-OSPF-AGGREGATE-INTERVAL",
            &wasFound,
            buf);

        if (wasFound)
        {
            db->statsOspfTable->ospfAggregateTableInterval
                = TIME_ConvertToClock(buf);
        }

        if (db->statsOspfTable->ospfAggregateTableInterval <= 0)
        {
            // Set default value and print warning
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-OSPF-AGGREGATE-INTERVAL"
                "parameter, using Default\n");
            db->statsOspfTable->ospfAggregateTableInterval
                = STATSDB_DEFAULT_OSPF_AGGREGATE_INTERVAL;
        }

        //init timer related to MOSPF
        // Set Summary Interval timer.
        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "STATS-DB-SUMMARY-INTERVAL",
            &wasFound,
            buf);

        if (wasFound)
        {
            db->statsOspfTable->mospfSummaryInterval =
                                                    TIME_ConvertToClock(buf);
        }
        else
        {
            db->statsOspfTable->mospfSummaryInterval =
                                            STATSDB_DEFAULT_SUMMARY_INTERVAL;
        }

        if (db->statsOspfTable->mospfSummaryInterval <= 0)
        {
            // Set default value and print warning
            ERROR_ReportWarning(
            "Value for STATS-DB-SUMMARY-INTERVAL should be greater than 0."
            "Continue with default value.\n");
            db->statsOspfTable->mospfSummaryInterval =
                                            STATSDB_DEFAULT_SUMMARY_INTERVAL;
        }
    }

}

void InitializeStatsDBOspfNeighborStateTable(PartitionData* partition,
                                             NodeInput* nodeInput)
{
    clocktype start = 0;
    clocktype end = 0;
    int i;

    StatsDb* db = partition->statsDb;
    db->statsOspfTable->ospfNeighborStateTableDef.push_back
        (string_pair("Timestamp", "real"));
    db->statsOspfTable->ospfNeighborStateTableDef.push_back
        (string_pair("InterfaceAddress", "text"));
    db->statsOspfTable->ospfNeighborStateTableDef.push_back
        (string_pair("NeighborInterfaceAddress", "text"));
    db->statsOspfTable->ospfNeighborStateTableDef.push_back
        (string_pair("NeighborState", "text"));

    DBColumns columns;
    columns.reserve(4);

    for (i = 0; i < db->statsOspfTable->ospfNeighborStateTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfNeighborStateTableDef[i]);
    }



    if (partition->partitionId == 0)
    {
        if (STATS_DEBUG)
        {
            start = partition->wallClock->getRealTime();
        }

        CreateTable(db, "OSPF_NeighborState", columns);

        if (STATS_DEBUG)
        {
            end = partition->wallClock->getRealTime();
            clocktype diff = end - start;
            char temp[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(diff, temp);
            printf ("Time Taken to create OSPF Neighbor table partition %d, %s\n",
                    partition->partitionId,
                    temp);
        }
    }
}

void InitializeStatsDBOspfRouterLsaTable(PartitionData* partition,
                                         NodeInput* nodeInput)
{
    clocktype start = 0;
    clocktype end = 0;
    int i;

    StatsDb* db = partition->statsDb;
    db->statsOspfTable->ospfRouterLsaTableDef.push_back
        (string_pair("Timestamp", "real"));
    db->statsOspfTable->ospfRouterLsaTableDef.push_back
        (string_pair("NodeId", "int"));
    db->statsOspfTable->ospfRouterLsaTableDef.push_back
        (string_pair("AreaId", "int"));
    db->statsOspfTable->ospfRouterLsaTableDef.push_back
        (string_pair("AdvertisingRouterAddr", " text"));
    db->statsOspfTable->ospfRouterLsaTableDef.push_back
        (string_pair("LinkId", "text"));
    db->statsOspfTable->ospfRouterLsaTableDef.push_back
        (string_pair("LinkData", "text"));
    db->statsOspfTable->ospfRouterLsaTableDef.push_back
        (string_pair("LinkType", "text"));
    db->statsOspfTable->ospfRouterLsaTableDef.push_back
        (string_pair("LinkStateAge", "int"));
    db->statsOspfTable->ospfRouterLsaTableDef.push_back
        (string_pair("Cost", "int"));

    DBColumns columns;
    columns.reserve(9);

    for (i = 0; i < db->statsOspfTable->ospfRouterLsaTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfRouterLsaTableDef[i]);
    }



    if (partition->partitionId == 0)
    {
        if (STATS_DEBUG)
        {
            start = partition->wallClock->getRealTime();
        }

        CreateTable(db, "OSPF_RouterLsa", columns);

        if (STATS_DEBUG)
        {
            end = partition->wallClock->getRealTime();
            clocktype diff = end - start;
            char temp[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(diff, temp);
            printf ("Time Taken to create OSPF Router LSA table partition %d, %s\n",
                    partition->partitionId,
                    temp);
        }
    }
}

void InitializeStatsDBOspfNetworkLsaTable(PartitionData* partition,
                                          NodeInput* nodeInput)
{
    clocktype start = 0;
    clocktype end = 0;
    int i;

    StatsDb* db = partition->statsDb;
    db->statsOspfTable->ospfNetworkLsaTableDef.push_back
        (string_pair("Timestamp", "real"));
    db->statsOspfTable->ospfNetworkLsaTableDef.push_back
        (string_pair("NodeId", "int"));
    db->statsOspfTable->ospfNetworkLsaTableDef.push_back
        (string_pair("AreaId", "int"));
    db->statsOspfTable->ospfNetworkLsaTableDef.push_back
        (string_pair("AdvertisingRouterAddr", "text"));
    db->statsOspfTable->ospfNetworkLsaTableDef.push_back
        (string_pair("AttachedRouterAddr", "text"));
    db->statsOspfTable->ospfNetworkLsaTableDef.push_back
        (string_pair("LinkStateAge", "int"));

    DBColumns columns;
    columns.reserve(6);

    for (i = 0; i < db->statsOspfTable->ospfNetworkLsaTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfNetworkLsaTableDef[i]);
    }


    if (partition->partitionId == 0)
    {
        if (STATS_DEBUG)
        {
            start = partition->wallClock->getRealTime();
        }

        CreateTable(db, "OSPF_NetworkLsa", columns);

        if (STATS_DEBUG)
        {
            end = partition->wallClock->getRealTime();
            clocktype diff = end - start;
            char temp[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(diff, temp);
            printf ("Time Taken to create OSPF_NetworkLsa table partition %d, %s\n",
                    partition->partitionId,
                    temp);
        }
    }

}

void InitializeStatsDBOspfSummaryLsaTable(PartitionData* partition,
                                          NodeInput* nodeInput)
{
    clocktype start = 0;
    clocktype end = 0;
    int i;

    StatsDb* db = partition->statsDb;
    db->statsOspfTable->ospfSummaryLsaTableDef.push_back
        (string_pair("Timestamp", "real"));
    db->statsOspfTable->ospfSummaryLsaTableDef.push_back
        (string_pair("NodeId", "int"));
    db->statsOspfTable->ospfSummaryLsaTableDef.push_back
        (string_pair("AreaId", "int"));
    db->statsOspfTable->ospfSummaryLsaTableDef.push_back
        (string_pair("AdvertisingRouterAddr", "text"));
    db->statsOspfTable->ospfSummaryLsaTableDef.push_back
        (string_pair("DestinationAddr", "text"));
    db->statsOspfTable->ospfSummaryLsaTableDef.push_back
        (string_pair("LinkStateAge", "int"));
    db->statsOspfTable->ospfSummaryLsaTableDef.push_back
        (string_pair("LinkStateType", "text"));

    DBColumns columns;
    columns.reserve(7);

    for (i = 0; i < db->statsOspfTable->ospfSummaryLsaTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfSummaryLsaTableDef[i]);
    }



    if (partition->partitionId == 0)
    {
        if (STATS_DEBUG)
        {
            start = partition->wallClock->getRealTime();
        }

        CreateTable(db, "OSPF_SummaryLsa", columns);

        if (STATS_DEBUG)
        {
            end = partition->wallClock->getRealTime();
            clocktype diff = end - start;
            char temp[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(diff, temp);
            printf ("Time Taken to create OSPF_SummaryLsa table partition %d, %s\n",
                    partition->partitionId,
                    temp);
        }
    }
}

void InitializeStatsDBOspfExternalLsaTable(PartitionData* partition,
                                           NodeInput* nodeInput)
{
    clocktype start = 0;
    clocktype end = 0;
    int i;

    StatsDb* db = partition->statsDb;
    db->statsOspfTable->ospfExternalLsaTableDef.push_back
        (string_pair("Timestamp", "real"));
    db->statsOspfTable->ospfExternalLsaTableDef.push_back
        (string_pair("NodeId", "int"));
    db->statsOspfTable->ospfExternalLsaTableDef.push_back
        (string_pair("AdvertisingRouterAddr", "text"));
    db->statsOspfTable->ospfExternalLsaTableDef.push_back
        (string_pair("DestinationAddr", "text"));
    db->statsOspfTable->ospfExternalLsaTableDef.push_back
        (string_pair("LinkStateAge", "int"));
    db->statsOspfTable->ospfExternalLsaTableDef.push_back
        (string_pair("Cost", "int"));
    db->statsOspfTable->ospfExternalLsaTableDef.push_back
        (string_pair("LinkStateType", "text"));

    DBColumns columns;
    columns.reserve(7);

    for (i = 0; i < db->statsOspfTable->ospfExternalLsaTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfExternalLsaTableDef[i]);
    }

    if (partition->partitionId == 0)
    {
        if (STATS_DEBUG)
        {
            start = partition->wallClock->getRealTime();
        }

        CreateTable(db, "OSPF_ExternalLsa", columns);

        if (STATS_DEBUG)
        {
            end = partition->wallClock->getRealTime();
            clocktype diff = end - start;
            char temp[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(diff, temp);
            printf ("Time Taken to create OSPF_ExternalLsa table partition %d, %s\n",
                    partition->partitionId,
                    temp);
        }
    }
}

void InitializeStatsDBOspfInterfaceStateTable(PartitionData* partition,
                                              NodeInput* nodeInput)
{
    clocktype start = 0;
    clocktype end = 0;
    int i;

    StatsDb* db = partition->statsDb;

    db->statsOspfTable->ospfInterfaceStateTableDef.push_back
        (string_pair("Timestamp", "real"));
    db->statsOspfTable->ospfInterfaceStateTableDef.push_back
        (string_pair("InterfaceAddress", "text"));
    db->statsOspfTable->ospfInterfaceStateTableDef.push_back
        (string_pair("CurrentState", "text"));
#ifdef ADDON_BOEINGFCS
    db->statsOspfTable->ospfInterfaceStateTableDef.push_back
        (string_pair("AreaId", "int"));
    db->statsOspfTable->ospfInterfaceStateTableDef.push_back
        (string_pair("MobileLeaf", "text"));
#else
    db->statsOspfTable->ospfInterfaceStateTableDef.push_back
        (string_pair("AreaId", "int"));
#endif

    DBColumns columns;
    columns.reserve(4);

    for (i = 0; i < db->statsOspfTable->ospfInterfaceStateTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfInterfaceStateTableDef[i]);
    }



    if (partition->partitionId == 0)
    {
        if (STATS_DEBUG)
        {
            start = partition->wallClock->getRealTime();
        }

        CreateTable(db, "OSPF_InterfaceState", columns);

        if (STATS_DEBUG)
        {
            end = partition->wallClock->getRealTime();
            clocktype diff = end - start;
            char temp[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(diff, temp);
            printf ("Time to create OSPF_InterfaceState table partition %d, %s\n",
                    partition->partitionId,
                    temp);
        }
    }
}

void InitializeStatsDBOspfSummaryTable(PartitionData* partition,
                                       NodeInput* nodeInput)
{
    clocktype start = 0;
    clocktype end = 0;
    int i;

    StatsDb* db = partition->statsDb;
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("Timestamp", "real"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("InterfaceAddress", "text"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("OfferedLoad", "real"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("LSUpdateSent", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("LSUpdateRecv", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("LSUpdateBytesSent", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("LSUpdateBytesRecv", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("AckSent", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("AckRecv", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("AckBytesSent", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("AckBytesRecv", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("DDPktSent", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("DDPktRecv", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("DDPktBytesSent", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("DDPktBytesRecv", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("DDPktRxmt", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("DDPktBytesRxmt", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("LSReqSent", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("LSReqRecv", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("LSReqBytesSent", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("LSReqBytesRecv", "bigint"));
    db->statsOspfTable->ospfSummaryTableDef.push_back
        (string_pair("LSReqRxmt", "bigint"));

    DBColumns columns;
    columns.reserve(22);

    for (i = 0; i < db->statsOspfTable->ospfSummaryTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfSummaryTableDef[i]);
    }



    if (partition->partitionId == 0)
    {
        if (STATS_DEBUG)
        {
            start = partition->wallClock->getRealTime();
        }

        CreateTable(db, "OSPF_Summary", columns);

        if (STATS_DEBUG)
        {
            end = partition->wallClock->getRealTime();
            clocktype diff = end - start;
            char temp[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(diff, temp);
            printf ("Time to create OSPF_Summary table partition %d, %s\n",
                    partition->partitionId,
                    temp);
        }
    }
}

void InitializeStatsDBOspfAggregateTable(PartitionData* partition,
                                         NodeInput* nodeInput)
{
    clocktype start = 0;
    clocktype end = 0;
    int i;

    StatsDb* db = partition->statsDb;
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("Timestamp", "real"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("OfferedLoad", "real"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("LSUpdateSent", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("LSUpdateRecv", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("LSUpdateBytesSent", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("LSUpdateBytesRecv", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("AckSent", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("AckRecv", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("AckBytesSent", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("AckBytesRecv", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("DDPktSent", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("DDPktRecv", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("DDPktBytesSent", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("DDPktBytesRecv", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("DDPktRxmt", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("DDPktBytesRxmt", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("LSReqSent", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("LSReqRecv", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("LSReqBytesSent", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("LSReqBytesRecv", "int"));
    db->statsOspfTable->ospfAggregateTableDef.push_back
        (string_pair("LSReqRxmt", "int"));

    DBColumns columns;
    columns.reserve(21);

    for (i = 0; i < db->statsOspfTable->ospfAggregateTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfAggregateTableDef[i]);
    }

    if (partition->partitionId == 0)
    {
        if (STATS_DEBUG)
        {
            start = partition->wallClock->getRealTime();
        }

        CreateTable(db, "OSPF_Aggregate", columns);

        if (STATS_DEBUG)
        {
            end = partition->wallClock->getRealTime();
            clocktype diff = end - start;
            char temp[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(diff, temp);
            printf ("Time to create OSPF_Aggregate table partition %d, %s\n",
                    partition->partitionId,
                    temp);
        }
    }
}


void InitializeStatsDBMospfSummaryTable(PartitionData* partition,
                                             NodeInput* nodeInput)
{
    clocktype start = 0;
    clocktype end = 0;
    int i;

    StatsDb* db = partition->statsDb;

    DBColumns columns;
    columns.reserve(13);

    columns.push_back(string_pair("Timestamp", "real"));
    columns.push_back(string_pair("NodeId", "integer"));
    columns.push_back(string_pair("GroupLSAGenerated", "integer"));
    columns.push_back(string_pair("GroupLSAFlushed", "integer"));
    columns.push_back(string_pair("GroupLSAReceived", "integer"));
    columns.push_back(string_pair("RouterLSA_WCMRSent", "integer"));
    columns.push_back(string_pair("RouterLSA_WCMRReceived", "integer"));
    columns.push_back(string_pair("RouterLSA_VLEPSent", "integer"));
    columns.push_back(string_pair("RouterLSA_VLEPReceived", "integer"));
    columns.push_back(string_pair("RouterLSA_ASBRSent", "integer"));
    columns.push_back(string_pair("RouterLSA_ASBRReceived", "integer"));
    columns.push_back(string_pair("RouterLSA_ABRSent", "integer"));
    columns.push_back(string_pair("RouterLSA_ABRReceived", "integer"));

    if (STATS_DEBUG)
    {
        start = partition->wallClock->getRealTime();
    }

    CreateTable(db, "MOSPF_Summary", columns);

    if (STATS_DEBUG)
    {
        end = partition->wallClock->getRealTime();
        clocktype diff = end - start;
        char temp[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(diff, temp);
        printf("Time Taken to create MOSPF_Summary table partition %d, %s\n",
                partition->partitionId,
                temp);
    }
}

void StatsDBSendOspfTableTimerMessage(PartitionData* partition)
{
    StatsDb* db = partition->statsDb;

    if (!db)
    {
        return;
    }

    Node* firstNode = NULL;

    firstNode = StatsDBOspfGetInitialNodePointer(partition);

    if (firstNode == NULL)
    {
        // none of the nodes on this partition are running OSPF
        // so dont waste the timers...
        return;
    }

    if (db->statsOspfTable)
    {
        if (db->statsOspfTable->createOspfNeighborStateTable)
        {
            Message* ospfNeighborStateTimer;
            ospfNeighborStateTimer = MESSAGE_Alloc(
                firstNode,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv2,
                MSG_STATS_OSPF_InsertNeighborState);
            MESSAGE_Send(firstNode,
                ospfNeighborStateTimer,
                db->statsOspfTable->ospfNeighborStateTableInterval);
        }
        if (db->statsOspfTable->createOspfRouterLsaTable)
        {
            Message* ospfRouterLsaTimer;
            ospfRouterLsaTimer = MESSAGE_Alloc(
                firstNode,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv2,
                MSG_STATS_OSPF_InsertRouterLsa);
            MESSAGE_Send(firstNode,
                ospfRouterLsaTimer,
                db->statsOspfTable->ospfRouterLsaTableInterval);
        }
        if (db->statsOspfTable->createOspfNetworkLsaTable)
        {
            Message* ospfNetworkLsaTimer;
            ospfNetworkLsaTimer = MESSAGE_Alloc(
                firstNode,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv2,
                MSG_STATS_OSPF_InsertNetworkLsa);
            MESSAGE_Send(firstNode,
                ospfNetworkLsaTimer,
                db->statsOspfTable->ospfNetworkLsaTableInterval);
        }
        if (db->statsOspfTable->createOspfSummaryLsaTable)
        {
            Message* ospfSummaryLsaTimer;
            ospfSummaryLsaTimer = MESSAGE_Alloc(
                firstNode,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv2,
                MSG_STATS_OSPF_InsertSummaryLsa);
            MESSAGE_Send(firstNode,
                ospfSummaryLsaTimer,
                db->statsOspfTable->ospfSummaryLsaTableInterval);
        }
        if (db->statsOspfTable->createOspfExternalLsaTable)
        {
            Message* ospfExternalLsaTimer;
            ospfExternalLsaTimer = MESSAGE_Alloc(
                firstNode,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv2,
                MSG_STATS_OSPF_InsertExternalLsa);
            MESSAGE_Send(firstNode,
                ospfExternalLsaTimer,
                db->statsOspfTable->ospfExternalLsaTableInterval);
        }
        if (db->statsOspfTable->createOspfInterfaceStateTable)
        {
            Message* ospfInterfaceStateTimer;
            ospfInterfaceStateTimer = MESSAGE_Alloc(
                firstNode,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv2,
                MSG_STATS_OSPF_InsertInterfaceState);
            MESSAGE_Send(firstNode,
                ospfInterfaceStateTimer,
                db->statsOspfTable->ospfInterfaceStateTableInterval);
        }
        if (db->statsOspfTable->createOspfSummaryTable)
        {
            Message* ospfSummaryTimer;
            ospfSummaryTimer = MESSAGE_Alloc(
                firstNode,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv2,
                MSG_STATS_OSPF_InsertSummary);
            MESSAGE_Send(firstNode,
                ospfSummaryTimer,
                db->statsOspfTable->ospfSummaryTableInterval);
        }
        if (db->statsOspfTable->createOspfAggregateTable)
        {
            Message* ospfAggregateTimer;
            ospfAggregateTimer = MESSAGE_Alloc(
                firstNode,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv2,
                MSG_STATS_OSPF_InsertAggregate);
            MESSAGE_Send(firstNode,
                ospfAggregateTimer,
                db->statsOspfTable->ospfAggregateTableInterval);
        }
        if (db->statsOspfTable->createMospfSummaryTable)
        {
            Message* mospfSummaryTimer;
            mospfSummaryTimer = MESSAGE_Alloc(firstNode,
                                              NETWORK_LAYER,
                                              ROUTING_PROTOCOL_OSPFv2,
                                              MSG_STATS_MOSPF_InsertSummary);
            MESSAGE_Send(firstNode,
                mospfSummaryTimer,
                db->statsOspfTable->mospfSummaryInterval);
        }
    }

    if (db->statsSummaryTable
        && db->statsSummaryTable->createMulticastNetSummaryTable)
    {
        Message* mospfMulticastNetSummaryTimer;
        mospfMulticastNetSummaryTimer = MESSAGE_Alloc(firstNode,
                                         NETWORK_LAYER,
                                         ROUTING_PROTOCOL_OSPFv2,
                                         MSG_STATS_MULTICAST_InsertSummary);
        MESSAGE_Send(firstNode,
                     mospfMulticastNetSummaryTimer,
                     db->statsSummaryTable->summaryInterval);
    }
}

void STATSDB_HandleOspfNeighborStateTableInsert(Node* node,
                                                const StatsDbOspfState & stats)
{
    StatsDb* db = NULL;
    db = node->partitionData->statsDb;

    // Check if the Table exists.
    if (!db ||!db->statsOspfTable->createOspfNeighborStateTable)
    {
        // Table does not exist
        return;
    }

    double timeVal = 0.0;
    int i;

    std::vector<std::string> columns;
    columns.reserve(4);
    std::vector<std::string> newValues;
    newValues.reserve(4);

    for (i = 0; i < db->statsOspfTable->ospfNeighborStateTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfNeighborStateTableDef[i].first);
    }

    // Now to add the values for these parameters.
    // TIME_PrintClockInSecond(node->getNodeTime(), timeBuf);
    timeVal = (double) node->getNodeTime() / SECOND;
    newValues.push_back(STATSDB_DoubleToString(timeVal));
    newValues.push_back(stats.ipAddr);
    newValues.push_back(stats.neighborAddr);
    newValues.push_back(stats.state);

    InsertValues(db, "OSPF_NeighborState", columns, newValues);
}



void STATSDB_HandleOspfRouterLsaTableInsert(Node* node,
                                            const StatsDbOspfLsa& stats)
{
    StatsDb* db = NULL;
    db = node->partitionData->statsDb;

    // Check if the Table exists.
    if (!db ||!db->statsOspfTable->createOspfRouterLsaTable)
    {
        // Table does not exist
        return;
    }

    double timeVal = 0.0;
    int i;
    
    std::vector<std::string> columns;
    columns.reserve(9);
    std::vector<std::string> newValues;
    newValues.reserve(9);

    for (i = 0; i < db->statsOspfTable->ospfRouterLsaTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfRouterLsaTableDef[i].first);
    }

    // Now to add the values for these parameters.
    // TIME_PrintClockInSecond(node->getNodeTime(), timeBuf);
    timeVal = (double) node->getNodeTime() / SECOND;
    newValues.push_back(STATSDB_DoubleToString(timeVal));
    newValues.push_back(STATSDB_IntToString(node->nodeId));
    newValues.push_back(STATSDB_IntToString(stats.areaId));
    newValues.push_back(stats.advRouterAddr);
    newValues.push_back(stats.linkId);
    newValues.push_back(stats.linkData);
    newValues.push_back(stats.linkType);
    newValues.push_back(STATSDB_IntToString(stats.linkStateAge));
    newValues.push_back(STATSDB_IntToString(stats.cost));

    InsertValues(db, "OSPF_RouterLsa", columns, newValues);
}

void STATSDB_HandleOspfNetworkLsaTableInsert(Node* node,
                                             const StatsDbOspfLsa& stats)
{
    StatsDb* db = NULL;
    db = node->partitionData->statsDb;

    // Check if the Table exists.
    if (!db ||!db->statsOspfTable->createOspfNetworkLsaTable)
    {
        // Table does not exist
        return;
    }

    double timeVal = 0.0;
    int i;

    std::vector<std::string> columns;
    columns.reserve(6);
    std::vector<std::string> newValues;
    newValues.reserve(6);

    for (i = 0; i < db->statsOspfTable->ospfNetworkLsaTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfNetworkLsaTableDef[i].first);
    }

    // Now to add the values for these parameters.
    // TIME_PrintClockInSecond(node->getNodeTime(), timeBuf);
    timeVal = (double) node->getNodeTime() / SECOND;
    newValues.push_back(STATSDB_DoubleToString(timeVal));
    newValues.push_back(STATSDB_IntToString(node->nodeId));
    newValues.push_back(STATSDB_IntToString(stats.areaId));
    newValues.push_back(stats.advRouterAddr);
    newValues.push_back(stats.routerAddr);
    newValues.push_back(STATSDB_IntToString(stats.linkStateAge));

    InsertValues(db, "OSPF_NetworkLsa", columns, newValues);
}

void STATSDB_HandleOspfSummaryLsaTableInsert(Node* node,
                                             const StatsDbOspfLsa& stats)
{
    StatsDb* db = NULL;
    db = node->partitionData->statsDb;

    // Check if the Table exists.
    if (!db ||!db->statsOspfTable->createOspfNetworkLsaTable)
    {
        // Table does not exist
        return;
    }

    double timeVal = 0.0;
    int i;

    std::vector<std::string> columns;
    columns.reserve(7);
    std::vector<std::string> newValues;
    newValues.reserve(7);

    for (i = 0; i < db->statsOspfTable->ospfSummaryLsaTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfSummaryLsaTableDef[i].first);
    }

    // Now to add the values for these parameters.
    // TIME_PrintClockInSecond(node->getNodeTime(), timeBuf);
    timeVal = (double) node->getNodeTime() / SECOND;
    newValues.push_back(STATSDB_DoubleToString(timeVal));
    newValues.push_back(STATSDB_IntToString(node->nodeId));
    newValues.push_back(STATSDB_IntToString(stats.areaId));
    newValues.push_back(stats.advRouterAddr);
    newValues.push_back(stats.routerAddr);
    newValues.push_back(STATSDB_IntToString(stats.linkStateAge));
    newValues.push_back(stats.linkType);

    InsertValues(db, "OSPF_SummaryLsa", columns, newValues);
}

void STATSDB_HandleOspfExternalLsaTableInsert(Node* node,
                                              const StatsDbOspfLsa& stats)
{
    StatsDb* db = NULL;
    db = node->partitionData->statsDb;

    // Check if the Table exists.
    if (!db ||!db->statsOspfTable->createOspfExternalLsaTable)
    {
        // Table does not exist
        return;
    }

    double timeVal = 0.0;
    int i;

    std::vector<std::string> columns;
    columns.reserve(7);
    std::vector<std::string> newValues;
    newValues.reserve(7);

    for (i = 0; i < db->statsOspfTable->ospfExternalLsaTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfExternalLsaTableDef[i].first);
    }

    // Now to add the values for these parameters.
    // TIME_PrintClockInSecond(node->getNodeTime(), timeBuf);
    timeVal = (double) node->getNodeTime() / SECOND;
    newValues.push_back(STATSDB_DoubleToString(timeVal));
    newValues.push_back(STATSDB_IntToString(node->nodeId));
    newValues.push_back(stats.advRouterAddr);
    newValues.push_back(stats.routerAddr);
    newValues.push_back(STATSDB_IntToString(stats.linkStateAge));
    newValues.push_back(STATSDB_IntToString(stats.cost));
    newValues.push_back(stats.linkType);

    InsertValues(db, "OSPF_ExternalLsa", columns, newValues);
}

void STATSDB_HandleOspfInterfaceStateTableInsert(Node* node,
                                                 const StatsDbOspfState& stats)
{
    StatsDb* db = NULL;
    db = node->partitionData->statsDb;

    // Check if the Table exists.
    if (!db ||!db->statsOspfTable->createOspfInterfaceStateTable)
    {
        // Table does not exist
        return;
    }

    double timeVal = 0.0;
    int i;

    std::vector<std::string> columns;
    columns.reserve(4);
    std::vector<std::string> newValues;
    newValues.reserve(4);

    for (i = 0; i < db->statsOspfTable->ospfInterfaceStateTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfInterfaceStateTableDef[i].first);
    }

    timeVal = (double) node->getNodeTime() / SECOND;
    newValues.push_back(STATSDB_DoubleToString(timeVal));
    newValues.push_back(stats.ipAddr);
    newValues.push_back(stats.state);
    newValues.push_back(STATSDB_IntToString(stats.areaId));
#ifdef ADDON_BOEINGFCS
    newValues.push_back(stats.mobileLeaf);
#endif

    InsertValues(db, "OSPF_InterfaceState", columns, newValues);
}

void STATSDB_HandleOspfSummaryTableInsert(Node* node,
                                          const StatsDbOspfStats& stats)
{
    StatsDb* db = NULL;
    db = node->partitionData->statsDb;

    // Check if the Table exists.
    if (!db ||!db->statsOspfTable->createOspfSummaryTable)
    {
        // Table does not exist
        return;
    }

    double timeVal = 0.0;
    int i;

    std::vector<std::string> columns;
    columns.reserve(22);
    std::vector<std::string> newValues;
    newValues.reserve(22);

    for (i = 0; i < db->statsOspfTable->ospfSummaryTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfSummaryTableDef[i].first);
    }

    // Now to add the values for these parameters.
    // TIME_PrintClockInSecond(node->getNodeTime(), timeBuf);
    timeVal = (double) node->getNodeTime() / SECOND;
    newValues.push_back(STATSDB_DoubleToString(timeVal));
    newValues.push_back(stats.ipAddr);
    newValues.push_back(STATSDB_DoubleToString(stats.offeredLoad));

    newValues.push_back(STATSDB_Int64ToString(stats.numLSUpdateSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSUpdateRecv));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSUpdateBytesSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSUpdateBytesRecv));

    newValues.push_back(STATSDB_Int64ToString(stats.numAckSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numAckRecv));
    newValues.push_back(STATSDB_Int64ToString(stats.numAckBytesSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numAckBytesRecv));

    newValues.push_back(STATSDB_Int64ToString(stats.numDDPktSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numDDPktRecv));
    newValues.push_back(STATSDB_Int64ToString(stats.numDDPktBytesSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numDDPktBytesRecv));
    newValues.push_back(STATSDB_Int64ToString(stats.numDDPktRxmt));
    newValues.push_back(STATSDB_Int64ToString(stats.numDDPktBytesRxmt));

    newValues.push_back(STATSDB_Int64ToString(stats.numLSReqSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSReqRecv));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSReqBytesSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSReqBytesRecv));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSReqRxmt));

    InsertValues(db, "OSPF_Summary", columns, newValues);
}

void STATSDB_HandleOspfAggregateTableInsert(Node* node,
                                            const StatsDbOspfStats& stats)
{
    StatsDb* db = NULL;
    db = node->partitionData->statsDb;

    // Check if the Table exists.
    if (!db ||!db->statsOspfTable->createOspfAggregateTable)
    {
        // Table does not exist
        return;
    }

    double timeVal = 0.0;
    int i;

    std::vector<std::string> columns;
    columns.reserve(21);
    std::vector<std::string> newValues;
    newValues.reserve(21);
    
    for (i = 0; i < db->statsOspfTable->ospfAggregateTableDef.size(); i++)
    {
        columns.push_back(db->statsOspfTable->ospfAggregateTableDef[i].first);
    }

    // Now to add the values for these parameters.
    // TIME_PrintClockInSecond(node->getNodeTime(), timeBuf);
    timeVal = (double) node->getNodeTime() / SECOND;

    newValues.push_back(STATSDB_DoubleToString(timeVal));
    newValues.push_back(STATSDB_DoubleToString(stats.offeredLoad));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSUpdateSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSUpdateRecv));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSUpdateBytesSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSUpdateBytesRecv));

    newValues.push_back(STATSDB_Int64ToString(stats.numAckSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numAckRecv));
    newValues.push_back(STATSDB_Int64ToString(stats.numAckBytesSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numAckBytesRecv));

    newValues.push_back(STATSDB_Int64ToString(stats.numDDPktSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numDDPktRecv));
    newValues.push_back(STATSDB_Int64ToString(stats.numDDPktBytesSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numDDPktBytesRecv));
    newValues.push_back(STATSDB_Int64ToString(stats.numDDPktRxmt));
    newValues.push_back(STATSDB_Int64ToString(stats.numDDPktBytesRxmt));

    newValues.push_back(STATSDB_Int64ToString(stats.numLSReqSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSReqRecv));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSReqBytesSent));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSReqBytesRecv));
    newValues.push_back(STATSDB_Int64ToString(stats.numLSReqRxmt));

    InsertValues(db, "OSPF_Aggregate", columns, newValues);
}

void STATSDB_HandleOspfAggregateParallelRetreiveExisting(
    Node* node,
    StatsDbOspfStats* stats)
{
    StatsDb* db = node->partitionData->statsDb;
    if (db == NULL)
    {
        return;
    }

    clocktype timeVal = node->getNodeTime();
    std::vector<std::string> columns;
    columns.reserve(db->statsOspfTable->ospfAggregateTableDef.size() - 1);

    std::vector<std::string> qualifierColumns;
    std::vector<std::string> qualifierValues;
    qualifierColumns.push_back("Timestamp");
    qualifierValues.push_back(
        STATSDB_DoubleToString((double) timeVal / SECOND));

   
    char** resultp;
    int nrow;
    int ncol;
    int expectedCol = db->statsOspfTable->ospfAggregateTableDef.size() - 1;
    int index = db->statsOspfTable->ospfAggregateTableDef.size();

    for (int i = 1; i < db->statsOspfTable->ospfAggregateTableDef.size(); i++)
    {
       columns.push_back(db->statsOspfTable->ospfAggregateTableDef[i].first);
    }

    std::string queryStr = GetSelectSQL(
        "OSPF_Aggregate",
        columns,
        qualifierColumns,
        qualifierValues);

    if (STATS_DEBUG)
    {
        printf("%s\n", queryStr.c_str());
    }

    double bytesSent = 0;

    std::string out = Select(db, queryStr);

    db->driver->unmarshall(out, resultp, nrow, ncol);

#if defined(DEBUG_MARSHALL)
    printf("Returned %d rows, %d columns, expected %d\n",
           nrow, ncol, expectedCol);
#endif /* DEBUG_MARSHALL */

    if (nrow < 1 || ncol < expectedCol)
    {
        // TODO: try again
        ERROR_ReportWarning("Error during OSPF "
            "aggregate collection, existing stats not found");
    }
    else
    {
        // Now update stats for previous partitions
        stats->numLSUpdateSent += atoi(resultp[index++]);
        stats->numLSUpdateRecv += atoi(resultp[index++]);
        stats->numLSUpdateBytesSent += atoi(resultp[index++]);
        stats->numLSUpdateBytesRecv += atoi(resultp[index++]);

        stats->numAckSent += atoi(resultp[index++]);
        stats->numAckRecv += atoi(resultp[index++]);
        stats->numAckBytesSent += atoi(resultp[index++]);
        stats->numAckBytesRecv += atoi(resultp[index++]);

        stats->numDDPktSent += atoi(resultp[index++]);
        stats->numDDPktRecv += atoi(resultp[index++]);
        stats->numDDPktBytesSent += atoi(resultp[index++]);
        stats->numDDPktBytesRecv += atoi(resultp[index++]);
        stats->numDDPktRxmt += atoi(resultp[index++]);
        stats->numDDPktBytesRxmt += atoi(resultp[index++]);

        stats->numLSReqSent += atoi(resultp[index++]);
        stats->numLSReqRecv += atoi(resultp[index++]);
        stats->numLSReqBytesSent += atoi(resultp[index++]);
        stats->numLSReqBytesRecv += atoi(resultp[index++]);
        stats->numLSReqRxmt += atoi(resultp[index++]);

        bytesSent += stats->numLSUpdateBytesSent +
                    stats->numAckBytesSent + stats->numLSReqBytesSent +
                    stats->numDDPktBytesSent;
        stats->offeredLoad = ((double) bytesSent / timeVal) * 8;
    }
    db->driver->free_table(resultp, nrow, ncol);
}

void STATSDB_HandleOspfAggregateTableUpdate(Node* node,
                                            const StatsDbOspfStats & stats)
{
    StatsDb* db = node->partitionData->statsDb;

    std::vector<std::string> columns;
    columns.reserve(20);
    std::vector<std::string> values;
    values.reserve(20);
    std::vector<std::string> qualifiersColumns;
    std::vector<std::string> qualifiersValues;

    qualifiersColumns.push_back("OfferedLoad");
    qualifiersValues.push_back(STATSDB_DoubleToString(stats.offeredLoad));
    qualifiersColumns.push_back("LSUpdateSent");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numLSUpdateSent));
    qualifiersColumns.push_back("LSUpdateRecv");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numLSUpdateRecv));
    qualifiersColumns.push_back("LSUpdateBytesSent");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numLSUpdateBytesSent));
    qualifiersColumns.push_back("LSUpdateBytesRecv");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numLSUpdateBytesRecv));

    qualifiersColumns.push_back("AckSent");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numAckSent));
    qualifiersColumns.push_back("AckRecv");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numAckRecv));
    qualifiersColumns.push_back("AckBytesSent");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numAckBytesSent));
    qualifiersColumns.push_back("AckBytesRecv");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numAckBytesRecv));

    qualifiersColumns.push_back("DDPktSent");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numDDPktSent));
    qualifiersColumns.push_back("DDPktRecv");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numDDPktRecv));
    qualifiersColumns.push_back("DDPktBytesSent");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numDDPktBytesSent));
    qualifiersColumns.push_back("DDPktBytesRecv");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numDDPktBytesRecv));
    qualifiersColumns.push_back("DDPktRxmt");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numDDPktRxmt));
    qualifiersColumns.push_back("DDPktBytesRxmt");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numDDPktBytesRxmt));

    qualifiersColumns.push_back("LSReqSent");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numLSReqSent));
    qualifiersColumns.push_back("LSReqRecv");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numLSReqRecv));
    qualifiersColumns.push_back("LSReqBytesSent");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numLSReqBytesSent));
    qualifiersColumns.push_back("LSReqBytesRecv");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numLSReqBytesRecv));
    qualifiersColumns.push_back("LSReqRxmt");
    qualifiersValues.push_back(STATSDB_Int64ToString(stats.numLSReqRxmt));

    qualifiersColumns.push_back("Timestamp");
    qualifiersValues.push_back(STATSDB_DoubleToString(
        (double) node->getNodeTime() / SECOND));

    UpdateValues(db, "OSPF_Aggregate", columns, values,
        qualifiersColumns, qualifiersValues);
}


void STATSDB_HandleMospfSummaryTableInsert(Node* node,
                                           const StatsDBMospfSummary & stats)
{
    StatsDb* db = NULL;
    db = node->partitionData->statsDb;

    // Check if the Table exists.
    if (!db || !db->statsOspfTable ||
        !db->statsOspfTable->createMospfSummaryTable)
    {
        // Table does not exist
        return;
    }

    double timeVal = 0.0;
    int i;

    std::vector<std::string> columns;
    columns.reserve(13);
    std::vector<std::string> newValues;
    newValues.reserve(13);

    columns.push_back("Timestamp");
    columns.push_back("NodeId");
    columns.push_back("GroupLSAGenerated");
    columns.push_back("GroupLSAFlushed");
    columns.push_back("GroupLSAReceived");
    columns.push_back("RouterLSA_WCMRSent");
    columns.push_back("RouterLSA_WCMRReceived");
    columns.push_back("RouterLSA_VLEPSent");
    columns.push_back("RouterLSA_VLEPReceived");
    columns.push_back("RouterLSA_ASBRSent");
    columns.push_back("RouterLSA_ASBRReceived");
    columns.push_back("RouterLSA_ABRSent");
    columns.push_back("RouterLSA_ABRReceived");

    // Now to add the values for these parameters.
    timeVal = (double) node->getNodeTime() / SECOND;

    newValues.push_back(STATSDB_DoubleToString(timeVal));
    newValues.push_back(STATSDB_IntToString(node->nodeId));
    newValues.push_back(STATSDB_IntToString(stats.m_NumGroupLSAGenerated));
    newValues.push_back(STATSDB_IntToString(stats.m_NumGroupLSAFlushed));
    newValues.push_back(STATSDB_IntToString(stats.m_NumGroupLSAReceived));
    newValues.push_back(STATSDB_IntToString(stats.m_NumRouterLSA_WCMRSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumRouterLSA_WCMRReceived));
    newValues.push_back(STATSDB_IntToString(stats.m_NumRouterLSA_VLEPSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumRouterLSA_VLEPReceived));
    newValues.push_back(STATSDB_IntToString(stats.m_NumRouterLSA_ASBRSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumRouterLSA_ASBRReceived));
    newValues.push_back(STATSDB_IntToString(stats.m_NumRouterLSA_ABRSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumRouterLSA_ABRReceived));

    InsertValues(db, "MOSPF_Summary", columns, newValues);

}

/********************** PIM Specific Code Starts****************************/

StatsDBPimSmStatus::StatsDBPimSmStatus()
{
    m_BsrType.clear();
    m_RpType.clear();
    m_GroupAddr.clear();
    m_GroupAddrRange.clear();
    m_IsDR = FALSE;
    m_DrNetworkAddr.clear();
}


StatsDBPimSmSummary::StatsDBPimSmSummary()
{
    m_NumHelloSent = 0;
    m_NumHelloRecvd = 0;
    m_NumJoinPruneSent = 0;
    m_NumJoinPruneRecvd = 0;
    m_NumCandidateRPSent = 0;
    m_NumCandidateRPRecvd = 0;
    m_NumBootstrapSent = 0;
    m_NumBootstrapRecvd = 0;
    m_NumRegisterSent = 0;
    m_NumRegisterRecvd = 0;
    m_NumRegisterStopSent = 0;
    m_NumRegisterStopRecvd = 0;
    m_NumAssertSent = 0;
    m_NumAssertRecvd = 0;
}


StatsDBPimDmSummary::StatsDBPimDmSummary()
{
    m_NumHelloSent = 0;
    m_NumHelloRecvd = 0;
    m_NumJoinPruneSent = 0;
    m_NumJoinPruneRecvd = 0;
    m_NumGraftSent = 0;
    m_NumGraftRecvd = 0;
    m_NumGraftAckSent = 0;
    m_NumGraftAckRecvd = 0;
    m_NumAssertSent = 0;
    m_NumAssertRecvd = 0;
}



StatsDBPimTable::StatsDBPimTable()
{
    createPimSmStatusTable = FALSE;
    createPimSmSummaryTable = FALSE;
    createPimDmSummaryTable = FALSE;

    //this interval is currently same as STATS-DB-STATUS-INTERVAL
    clocktype pimStatusInterval = 0;
    //this interval is currently same as STATS-DB-SUMMARY-INTERVAL
    clocktype pimSummaryInterval = 0;

}



void InitializeStatsDBPimTable(PartitionData* partition,
                                NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL wasFound = FALSE;
    BOOL pimTableCreated = FALSE;

    StatsDb* db = NULL;
    db = partition->statsDb;
    db->statsPimTable = NULL;

    // Check which tables are to be defined in the Protocol table
    // category. Check for the user configurations.

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "STATS-DB-MULTICAST-PIM-SM-STATUS-TABLE",
        &wasFound,
        buf);
    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            if (!pimTableCreated)
            {
                db->statsPimTable = new StatsDBPimTable();
                pimTableCreated = TRUE;
            }

            // Initialize the PIM_SM_Status table.
            db->statsPimTable->createPimSmStatusTable = TRUE;
            if (partition->partitionId == 0)
            {
                InitializeStatsDBPimSmStatusTable(partition, nodeInput);
            }
        }
        else if (strcmp(buf, "NO") != 0)
        {
            // We have invalid values.
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-MULTICAST-PIM-SM-STATUS-TABLE"
                "parameter, using Default\n");
        }
    }

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "STATS-DB-MULTICAST-PIM-SM-SUMMARY-TABLE",
        &wasFound,
        buf);
    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            if (!pimTableCreated)
            {
                db->statsPimTable = new StatsDBPimTable();
                pimTableCreated = TRUE;
            }

            // Initialize the PIM_SM_Summary table.
            db->statsPimTable->createPimSmSummaryTable = TRUE;
            if (partition->partitionId == 0)
            {
                InitializeStatsDBPimSmSummaryTable(partition, nodeInput);
            }
        }
        else if (strcmp(buf, "NO") != 0)
        {
            // We have invalid values.
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-MULTICAST-PIM-SM-SUMMARY-TABLE"
                "parameter, using Default\n");
        }
    }

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "STATS-DB-MULTICAST-PIM-DM-SUMMARY-TABLE",
        &wasFound,
        buf);
    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            if (!pimTableCreated)
            {
                db->statsPimTable = new StatsDBPimTable();
                pimTableCreated = TRUE;
            }

            // Initialize the PIM_DM_Summary table.
            db->statsPimTable->createPimDmSummaryTable = TRUE;
            if (partition->partitionId == 0)
            {
                InitializeStatsDBPimDmSummaryTable(partition, nodeInput);
            }
        }
        else if (strcmp(buf, "NO") != 0)
        {
            // We have invalid values.
            ERROR_ReportWarning(
                "Invalid Value for STATS-DB-MULTICAST-PIM-DM-SUMMARY-TABLE"
                "parameter, using Default\n");
        }
    }

    //for interval the parameter values STATS-DB-STATUS-INTERVAL and
    //STATS-DB-SUMMARY-INTERVAL are used
    if (pimTableCreated)
    {
        // Set Status Interval timer.
        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "STATS-DB-STATUS-INTERVAL",
            &wasFound,
            buf);

        if (wasFound)
        {
            db->statsPimTable->pimStatusInterval = TIME_ConvertToClock(buf);
        }
        else
        {
            db->statsPimTable->pimStatusInterval=
                                             STATSDB_DEFAULT_STATUS_INTERVAL;
        }

        //value check
        if (db->statsPimTable->pimStatusInterval <= 0)
        {
            // We have invalid values.
            ERROR_ReportWarning(
                "Value for STATS-DB-STATUS-INTERVAL should be greater "
                "than 0. Continue with default value.\n");

            db->statsPimTable->pimStatusInterval =
                                             STATSDB_DEFAULT_STATUS_INTERVAL;
        }


        // Set Summary Interval timer.
        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "STATS-DB-SUMMARY-INTERVAL",
            &wasFound,
            buf);

        if (wasFound)
        {
            db->statsPimTable->pimSummaryInterval = TIME_ConvertToClock(buf);
        }
        else
        {
            db->statsPimTable->pimSummaryInterval =
                                            STATSDB_DEFAULT_SUMMARY_INTERVAL;
        }

        //value check
        if (db->statsPimTable->pimSummaryInterval <= 0)
        {
            // We have invalid values.
            ERROR_ReportWarning(
                "Value for STATS-DB-SUMMARY-INTERVAL should be greater "
                "than 0. Continue with default value.\n");

            db->statsPimTable->pimSummaryInterval =
                                             STATSDB_DEFAULT_SUMMARY_INTERVAL;
        }
    }
}


void InitializeStatsDBPimSmStatusTable(PartitionData* partition,
                                             NodeInput* nodeInput)
{
    clocktype start = 0;
    clocktype end = 0;
    int i;

    StatsDb* db = partition->statsDb;
    
    DBColumns columns;
    columns.reserve(8);

    columns.push_back(string_pair("Timestamp", "real"));
    columns.push_back(string_pair("NodeId", "integer"));
    columns.push_back(string_pair("BSR_Type", "VARCHAR(20)"));
    columns.push_back(string_pair("RP_Type", "VARCHAR(20)"));
    columns.push_back(string_pair("GroupAddress", "VARCHAR(64)"));
    columns.push_back(string_pair("GroupAddressRange", "text"));
    columns.push_back(string_pair("IsDR", "VARCHAR(10)"));
    columns.push_back(string_pair("NetworkAddress", "text"));

    if (STATS_DEBUG)
    {
        start = partition->wallClock->getRealTime();
    }

    CreateTable(db, "PIM_SM_Status", columns);

    if (STATS_DEBUG)
    {
        end = partition->wallClock->getRealTime();
        clocktype diff = end - start;
        char temp[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(diff, temp);
        printf ("Time Taken to create PIM_SM_Status table partition %d, %s\n",
                partition->partitionId,
                temp);
    }
}



void InitializeStatsDBPimSmSummaryTable(PartitionData* partition,
                                             NodeInput* nodeInput)
{
    clocktype start = 0;
    clocktype end = 0;
    int i;

    StatsDb* db = partition->statsDb;
    
    DBColumns columns;
    columns.reserve(16);

    columns.push_back(string_pair("Timestamp", "real"));
    columns.push_back(string_pair("NodeId", "integer"));
    columns.push_back(string_pair("HelloSent", "integer"));
    columns.push_back(string_pair("HelloReceived", "integer"));
    columns.push_back(string_pair("JoinPruneSent", "integer"));
    columns.push_back(string_pair("JoinPruneReceived", "integer"));
    columns.push_back(string_pair("CandidateRPSent", "integer"));
    columns.push_back(string_pair("CandidateRPReceived", "integer"));
    columns.push_back(string_pair("BootstrapSent", "integer"));
    columns.push_back(string_pair("BootstrapReceived", "integer"));
    columns.push_back(string_pair("RegisterSent", "integer"));
    columns.push_back(string_pair("RegisterReceived", "integer"));
    columns.push_back(string_pair("RegisterStopSent", "integer"));
    columns.push_back(string_pair("RegisterStopReceived", "integer"));
    columns.push_back(string_pair("AssertSent", "integer"));
    columns.push_back(string_pair("AssertReceived", "integer"));

    if (STATS_DEBUG)
    {
        start = partition->wallClock->getRealTime();
    }

    CreateTable(db, "PIM_SM_Summary", columns);

    if (STATS_DEBUG)
    {
        end = partition->wallClock->getRealTime();
        clocktype diff = end - start;
        char temp[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(diff, temp);
       printf("Time Taken to create PIM_SM_Summary table partition %d, %s\n",
                partition->partitionId,
                temp);
    }
}



void InitializeStatsDBPimDmSummaryTable(PartitionData* partition,
                                             NodeInput* nodeInput)
{
    clocktype start = 0;
    clocktype end = 0;
    int i;

    StatsDb* db = partition->statsDb;

    DBColumns columns;
    columns.reserve(12);

    columns.push_back(string_pair("Timestamp", "real"));
    columns.push_back(string_pair("NodeId", "integer"));
    columns.push_back(string_pair("HelloSent", "integer"));
    columns.push_back(string_pair("HelloReceived", "integer"));
    columns.push_back(string_pair("JoinPruneSent", "integer"));
    columns.push_back(string_pair("JoinPruneReceived", "integer"));
    columns.push_back(string_pair("GraftSent", "integer"));
    columns.push_back(string_pair("GraftReceived", "integer"));
    columns.push_back(string_pair("GraftAckSent", "integer"));
    columns.push_back(string_pair("GraftAckReceived", "integer"));
    columns.push_back(string_pair("AssertSent", "integer"));
    columns.push_back(string_pair("AssertReceived", "integer"));

    if (STATS_DEBUG)
    {
        start = partition->wallClock->getRealTime();
    }

    CreateTable(db, "PIM_DM_Summary", columns);

    if (STATS_DEBUG)
    {
        end = partition->wallClock->getRealTime();
        clocktype diff = end - start;
        char temp[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(diff, temp);
       printf("Time Taken to create PIM_DM_Summary table partition %d, %s\n",
                partition->partitionId,
                temp);
    }
}


void StatsDBSendPimTableTimerMessage(PartitionData* partition)
{
    StatsDb* db = partition->statsDb;

    if (!db)
    {
        return;
    }

    Node* firstNode = NULL;

    firstNode = StatsDBPimGetInitialNodePointer(partition);

    if (firstNode == NULL)
    {
        // none of the nodes on this partition are running PIM
        // so dont waste the timers...
        return;
    }

    if (db->statsPimTable)
    {
        if (db->statsPimTable->createPimSmStatusTable)
        {
            Message* pimSmStatusTimer;
            pimSmStatusTimer = MESSAGE_Alloc(firstNode,
                                             NETWORK_LAYER,
                                             MULTICAST_PROTOCOL_PIM,
                                             MSG_STATS_PIM_SM_InsertStatus);
            MESSAGE_Send(firstNode,
                         pimSmStatusTimer,
                         db->statsPimTable->pimStatusInterval);
        }

        if (db->statsPimTable->createPimSmSummaryTable)
        {
            Message* pimSmSummaryTimer;
            pimSmSummaryTimer = MESSAGE_Alloc(firstNode,
                                             NETWORK_LAYER,
                                             MULTICAST_PROTOCOL_PIM,
                                             MSG_STATS_PIM_SM_InsertSummary);
            MESSAGE_Send(firstNode,
                         pimSmSummaryTimer,
                         db->statsPimTable->pimSummaryInterval);
        }

        if (db->statsPimTable->createPimDmSummaryTable)
        {
            Message* pimDmSummaryTimer;
            pimDmSummaryTimer = MESSAGE_Alloc(firstNode,
                                             NETWORK_LAYER,
                                             MULTICAST_PROTOCOL_PIM,
                                             MSG_STATS_PIM_DM_InsertSummary);
            MESSAGE_Send(firstNode,
                         pimDmSummaryTimer,
                         db->statsPimTable->pimSummaryInterval);
        }
    }

    if (db->statsSummaryTable
        && db->statsSummaryTable->createMulticastNetSummaryTable)
    {
        Message* pimMulticastNetSummaryTimer;
        pimMulticastNetSummaryTimer = MESSAGE_Alloc(firstNode,
                                      NETWORK_LAYER,
                                      MULTICAST_PROTOCOL_PIM,
                                      MSG_STATS_MULTICAST_InsertSummary);
        MESSAGE_Send(firstNode,
                     pimMulticastNetSummaryTimer,
                     db->statsSummaryTable->summaryInterval);
    }
}


void STATSDB_HandlePimSmStatusTableInsert(Node* node,
                                          const StatsDBPimSmStatus & stats)
{
    StatsDb* db = NULL;
    db = node->partitionData->statsDb;

    // Check if the Table exists.
    if (!db ||!db->statsPimTable ||
        !db->statsPimTable->createPimSmStatusTable)
    {
        // Table does not exist
        return;
    }

    double timeVal = 0.0;
    int i;

    std::vector<std::string> columns;
    columns.reserve(2);
    std::vector<std::string> newValues;
    newValues.reserve(2);

    columns.push_back("Timestamp");
    columns.push_back("NodeId");

    // Now to add the values for these parameters.
    timeVal = (double) node->getNodeTime() / SECOND;

    newValues.push_back(STATSDB_DoubleToString(timeVal));
    newValues.push_back(STATSDB_IntToString(node->nodeId));

    if (!stats.m_BsrType.empty())
    {
        columns.push_back("BSR_Type");
        newValues.push_back(stats.m_BsrType);
    }

    if (!stats.m_RpType.empty())
    {
        columns.push_back("RP_Type");
        newValues.push_back(stats.m_RpType);
    }

    if (!stats.m_GroupAddr.empty())
    {
        columns.push_back("GroupAddress");
        newValues.push_back(stats.m_GroupAddr);
    }
    
    if (!stats.m_GroupAddrRange.empty())
    {
        std::string range = "";
        for (i = 0; i < stats.m_GroupAddrRange.size(); i++)
        {
            range += stats.m_GroupAddrRange[i];

            if ((i+1) < stats.m_GroupAddrRange.size())
            {
                range += ", ";
            }
        }
        columns.push_back("GroupAddressRange");
        newValues.push_back(range);
    }

    if (stats.m_IsDR)
    {
        columns.push_back("IsDR");
        newValues.push_back("YES");
    }
    else
    {
        if (stats.m_RpType.compare("RP"))
        {
            columns.push_back("IsDR");
            newValues.push_back("NO");
        }
    }

    if (!stats.m_DrNetworkAddr.empty())
    {
        std::string netAddr = "";
        for (i = 0; i < stats.m_DrNetworkAddr.size(); i++)
        {
            netAddr += stats.m_DrNetworkAddr[i];

            if ((i+1) < stats.m_DrNetworkAddr.size())
            {
                netAddr += ", ";
            }
        }
        columns.push_back("NetworkAddress");
        newValues.push_back(netAddr);
    }

    InsertValues(db, "PIM_SM_Status", columns, newValues);
}


void STATSDB_HandlePimSmSummaryTableInsert(Node* node,
                                          const StatsDBPimSmSummary & stats)
{
    StatsDb* db = NULL;
    db = node->partitionData->statsDb;

    // Check if the Table exists.
    if (!db ||!db->statsPimTable ||
        !db->statsPimTable->createPimSmSummaryTable)
    {
        // Table does not exist
        return;
    }

    std::vector<std::string> columns;
    columns.reserve(16);
    std::vector<std::string> newValues;
    newValues.reserve(16);

    columns.push_back("Timestamp");
    columns.push_back("NodeId");
    columns.push_back("HelloSent");
    columns.push_back("HelloReceived");
    columns.push_back("JoinPruneSent");
    columns.push_back("JoinPruneReceived");
    columns.push_back("CandidateRPSent");
    columns.push_back("CandidateRPReceived");
    columns.push_back("BootstrapSent");
    columns.push_back("BootstrapReceived");
    columns.push_back("RegisterSent");
    columns.push_back("RegisterReceived");
    columns.push_back("RegisterStopSent");
    columns.push_back("RegisterStopReceived");
    columns.push_back("AssertSent");
    columns.push_back("AssertReceived");

    // Now to add the values for these parameters.
    double timeVal  = (double) node->getNodeTime() / SECOND;

    newValues.push_back(STATSDB_DoubleToString(timeVal));
    newValues.push_back(STATSDB_IntToString(node->nodeId));
    newValues.push_back(STATSDB_IntToString(stats.m_NumHelloSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumHelloRecvd));
    newValues.push_back(STATSDB_IntToString(stats.m_NumJoinPruneSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumJoinPruneRecvd));
    newValues.push_back(STATSDB_IntToString(stats.m_NumCandidateRPSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumCandidateRPRecvd));
    newValues.push_back(STATSDB_IntToString(stats.m_NumBootstrapSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumBootstrapRecvd));
    newValues.push_back(STATSDB_IntToString(stats.m_NumRegisterSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumRegisterRecvd));
    newValues.push_back(STATSDB_IntToString(stats.m_NumRegisterStopSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumRegisterStopRecvd));
    newValues.push_back(STATSDB_IntToString(stats.m_NumAssertSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumAssertRecvd));

    InsertValues(db, "PIM_SM_Summary", columns, newValues);
}

void STATSDB_HandlePimDmSummaryTableInsert(Node* node,
                                          const StatsDBPimDmSummary & stats)
{
    StatsDb* db = NULL;
    db = node->partitionData->statsDb;

    // Check if the Table exists.
    if (!db ||!db->statsPimTable ||
        !db->statsPimTable->createPimDmSummaryTable)
    {
        // Table does not exist
        return;
    }

    double timeVal = 0.0;

    std::vector<std::string> columns;
    columns.reserve(12);
    std::vector<std::string> newValues;
    newValues.reserve(12);

    columns.push_back("Timestamp");
    columns.push_back("NodeId");
    columns.push_back("HelloSent");
    columns.push_back("HelloReceived");
    columns.push_back("JoinPruneSent");
    columns.push_back("JoinPruneReceived");
    columns.push_back("GraftSent");
    columns.push_back("GraftReceived");
    columns.push_back("GraftAckSent");
    columns.push_back("GraftAckReceived");
    columns.push_back("AssertSent");
    columns.push_back("AssertReceived");

    // Now to add the values for these parameters.
    timeVal = (double) node->getNodeTime() / SECOND;

    newValues.push_back(STATSDB_DoubleToString(timeVal));
    newValues.push_back(STATSDB_IntToString(node->nodeId));
    newValues.push_back(STATSDB_IntToString(stats.m_NumHelloSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumHelloRecvd) );
    newValues.push_back(STATSDB_IntToString(stats.m_NumJoinPruneSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumJoinPruneRecvd));
    newValues.push_back(STATSDB_IntToString(stats.m_NumGraftSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumGraftRecvd));
    newValues.push_back(STATSDB_IntToString(stats.m_NumGraftAckSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumGraftAckRecvd));
    newValues.push_back(STATSDB_IntToString(stats.m_NumAssertSent));
    newValues.push_back(STATSDB_IntToString(stats.m_NumAssertRecvd));

    InsertValues(db, "PIM_DM_Summary", columns, newValues);
}

/********************** PIM Specific Code Ends******************************/

#endif // ADDON_DB

