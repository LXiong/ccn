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
// PACKAGE     :: STATS
// DESCRIPTION :: Implements the STATS API
// **/

#include <algorithm>
#include <map>

#include "api.h"
#include "partition.h"
#include "network_ip.h"
#include "transport_tcp.h"
#include "transport_udp.h"
#include "stats_global.h"
#include "WallClock.h"
#include "qualnet_mutex.h"
#ifdef ADDON_DB
#include "db.h"
#endif

// Workaround for windows
#ifdef _WIN32
#ifdef GetObject
#undef GetObject
#endif /* GetOjbect */
#endif /* _WIN32 */

// Enable to print out how long it took to perform aggregation/summarization
//#define DEBUG_TIMING

// Enable to print aggregated stats at each aggregation point
//#define PRINT_AGGREGATED_STATS

#ifndef USE_MPI
// Two dimensional array of partitions
// Used for aggregating data
volatile STAT_DataList*** gIncomingData;

// gRemoteSummarizers is shared by all partitions so each partition
// can access the Summarizer class of all other partitions
void* gRemoteSummarizers[MAX_THREADS] = {0};
#else
// One dimensional array of partitions
// Used for aggregating data
STAT_DataList** gIncomingData;
#endif

STAT_StatisticsList::STAT_StatisticsList(PartitionData* partition)
{
    // Read m_DeleteFinalizedModelStatistics parameter from config file
    BOOL wasFound;
    IO_ReadBool(
        ANY_DEST,
        ANY_ADDRESS,
        partition->nodeInput,
        "STATS-DELETE-WHEN-FINALIZED",
        &wasFound,
        &m_DeleteFinalizedModelStatistics);
    if (!wasFound)
    {
        m_DeleteFinalizedModelStatistics = TRUE;
    }

    m_LastAggregateTime = -1;
    m_NextAggregateId = 0;

    global.appAggregate.Initialize(partition, this);
    global.transportAggregate.Initialize(partition, this);
    global.netAggregate.Initialize(partition, this);
    global.phyAggregate.Initialize(partition, this);
    global.macAggregate.Initialize(partition, this);
    global.queueAggregate.Initialize(partition, this);

#ifndef USE_MPI
    if (partition->getNumPartitions() > 1)
    {
        if (partition->partitionId == 0)
        {
            gIncomingData = (volatile STAT_DataList***) MEM_malloc(sizeof(STAT_DataList**) * partition->getNumPartitions());
            for (int i = 0; i < partition->getNumPartitions(); i++)
            {
                gIncomingData[i] = (volatile STAT_DataList**) MEM_malloc(sizeof(STAT_DataList*) * partition->getNumPartitions());
                for (int j = 0; j < partition->getNumPartitions(); j++)
                {
                    gIncomingData[i][j] = NULL;
                }
            }
}

        PARALLEL_SynchronizePartitions(partition, BARRIER_TYPE_StatisticsList);
    }
    else
{
        gIncomingData = NULL;
    }
#else
    if (partition->getNumPartitions() > 1)
    {
        gIncomingData = (STAT_DataList**) MEM_malloc(sizeof(STAT_DataList*) * partition->getNumPartitions());
        for (int i = 0; i < partition->getNumPartitions(); i++)
        {
            gIncomingData[i] = NULL;
        }
    }
    else
    {
        gIncomingData = NULL;
    }


#endif
}

STAT_StatisticsList::aggIterator STAT_StatisticsList::FindAggregatedStatistic(STAT_AggregatedStatistic* stat)
{
    return find(m_AggregatedStats.begin(), m_AggregatedStats.end(), stat);
}

bool CompareAggregatePointer(STAT_AggregatedStatistic* a, STAT_AggregatedStatistic* b)
{
    return a->GetOrder() < b->GetOrder();
}

void STAT_StatisticsList::AddModelStatistics(STAT_ModelStatistics* stats)
{
    std::vector<STAT_ModelStatistics*>::iterator it;

    // Insert to list maintaining sorted order
    it = lower_bound(m_ModelStatistics.begin(), m_ModelStatistics.end(), stats);
    if (it == m_ModelStatistics.end())
    {
        m_ModelStatistics.push_back(stats);
    }
    else
    {
        m_ModelStatistics.insert(it, stats);
    }
}

void STAT_StatisticsList::RemoveModelStatistics(STAT_ModelStatistics* stats)
{
}

void STAT_StatisticsList::RegisterAggregatedStatistic(STAT_AggregatedStatistic* stat)
{
    // Make sure the statistic hasn't already been registered
    if (FindAggregatedStatistic(stat) != m_AggregatedStats.end())
    {
        ERROR_ReportWarning("aggregated statistic already added");
        return;
    }

    // Assign the statistic an id and put it in the array
    stat->SetId(m_NextAggregateId++);
    m_AggregatedStats.push_back(stat);

    if (stat->GetOrder() > 0)
    {
        m_OrderedAggregatedStats.push_back(stat);
        std::sort(
            m_OrderedAggregatedStats.begin(),
            m_OrderedAggregatedStats.end(),
            CompareAggregatePointer);
    }
}

void STAT_StatisticsList::Aggregate(PartitionData* partition)
{
    if (partition->getGlobalTime() > m_LastAggregateTime)
    {
        m_LastAggregateTime = partition->getGlobalTime();
    }
    else
    {
        // Already aggregated at this time tick
        return;
    }

#ifdef DEBUG_TIMING
    double startTiming = partition->wallClock->getTrueRealTimeAsDouble();
#endif

    // The following block of code can be used to test the recursive
    // iterator
#ifdef DEBUG_ITERATORS
    std::string path = "node/*/*/*/averageDelay";
    //D_ObjectIterator it(&partition->dynamicHierarchy, std::string("node/*/ip/*"));
    //D_ObjectIterator it(&partition->dynamicHierarchy, std::string("node/*/ip/ipFragUnit"));
    //D_ObjectIterator it(&partition->dynamicHierarchy, std::string("node/*/*/*"));
    D_ObjectIterator it(&partition->dynamicHierarchy, path);
    //D_ObjectIterator it(&partition->dynamicHierarchy, std::string("*/1/*/*"));
    //D_ObjectIterator it(&partition->dynamicHierarchy, std::string("node/3/ip/ipFragUnit"));
    //D_ObjectIterator it(&partition->dynamicHierarchy, std::string("*/*"));
    //D_ObjectIterator it(&partition->dynamicHierarchy, std::string("node/*"));
    //D_ObjectIterator it(&partition->dynamicHierarchy, std::string("node/5"));
    //D_ObjectIterator it(&partition->dynamicHierarchy, std::string(""));
    it.SetType(D_STATISTIC);
    //it.SetRecursive(true);
    D_Object* obj;
    while (it.GetNext())
    {
        obj = it.GetObject();
        if (obj)
        {
            printf("level %s\n", obj->GetFullPath().c_str());
        }
    }
#endif

    char* data;
    int size;
    int highestOrder = -1;

    // Loop over all aggregated statistics on this partition and perform aggregation
    for (unsigned int i = 0; i < m_AggregatedStats.size(); i++)
    {
        if (m_AggregatedStats[i]->GetIteratorPath() != "")
        {
            m_AggregatedStats[i]->RebuildIterator(&partition->dynamicHierarchy);
        }

        m_AggregatedStats[i]->Aggregate(partition->getGlobalTime());
        if (m_AggregatedStats[i]->GetOrder() > highestOrder)
        {
            highestOrder = m_AggregatedStats[i]->GetOrder();
        }
    }

    // Check if we have more than one partition
    if (partition->getNumPartitions() > 1)
    {
        STAT_DataList* dataList = NULL;
        int totalSize = 0;
        int i;

        // Serialize all stats order 0 stats
        for (unsigned i = 0; i < m_AggregatedStats.size(); i++)
        {
            if (m_AggregatedStats[i]->GetOrder() != 0)
            {
                continue;
            }

            // Serialize.  If there is data add it to the list.
            m_AggregatedStats[i]->Serialize(partition->getGlobalTime(), &data, &size);
            if (data != NULL)
            {
                STAT_DataList* newData =
                    (STAT_DataList*) MEM_malloc(sizeof(STAT_DataList));
                newData->id = m_AggregatedStats[i]->GetId();
                newData->data = data;
                newData->size = size;
                newData->next = dataList;
                dataList = newData;
                totalSize += size;
            }
        }

        // Add this partition's stats to each other partition then have a
        // barrier to guarantee that each partition has each other
        // partition's stats
#ifndef USE_MPI
        for (i = 0; i < partition->getNumPartitions(); i++)
        {
            if (i != partition->partitionId)
            {
                gIncomingData[i][partition->partitionId] = dataList;
            }
        }
        PARALLEL_SynchronizePartitions(partition, BARRIER_TYPE_StatAggregate);
#else
        // Store our stats then send/receive other partition's stats
        gIncomingData[partition->partitionId] = dataList;

        int size;
        for (i = 0; i < partition->getNumPartitions(); i++)
        {
            if (i == partition->partitionId)
            {
                STAT_DataList* data = dataList;
                while (data != NULL)
                {
                    size = data->size;
                    assert(size != 0);

                    // Broadcast stat to other partitions
                    MPI_Bcast(&size, sizeof(int), MPI_BYTE, i, MPI_COMM_WORLD);
                    MPI_Bcast(&data->id, sizeof(int), MPI_BYTE, i, MPI_COMM_WORLD);
                    MPI_Bcast(data->data, data->size, MPI_BYTE, i, MPI_COMM_WORLD);
                    data = data->next;
                }

                // Send size 0 signifying no more stats
                size = 0;
                MPI_Bcast(&size, sizeof(int), MPI_BYTE, i, MPI_COMM_WORLD);
            }
            else
            {
                assert(gIncomingData[i] == NULL);

                while (1)
                {
                    // Receive next size from broadcasting partition
                    MPI_Bcast(&size, sizeof(int), MPI_BYTE, i, MPI_COMM_WORLD);

                    // Size 0 signifies no mroe stats
                    if (size == 0)
                    {
                        break;
                    }


                    STAT_DataList* newData =
                        (STAT_DataList*) MEM_malloc(sizeof(STAT_DataList));
                    newData->size = size;
                    MPI_Bcast(&newData->id, sizeof(int), MPI_BYTE, i, MPI_COMM_WORLD);
                    newData->data = (char*) MEM_malloc(size);
                    MPI_Bcast(newData->data, size, MPI_BYTE, i, MPI_COMM_WORLD);
                    newData->next = gIncomingData[i];
                    gIncomingData[i] = newData;
                }
            }
        }
#endif

        // Update our stats with information from other partitions
#ifndef USE_MPI
        for (i = 0; i < partition->getNumPartitions(); i++)
        {
            if (i != partition->partitionId)
            {
                STAT_DataList* agg = (STAT_DataList*)
                    gIncomingData[partition->partitionId][i];
                while (agg != NULL)
                {
                    ERROR_Assert(
                        m_AggregatedStats[agg->id]->GetId() == agg->id,
                        "IDs don't match");
                    m_AggregatedStats[agg->id]->DeserializeAndUpdate(
                        partition->getGlobalTime(),
                        agg->data,
                        agg->size);

                    agg = agg->next;
                }
            }
        }
        PARALLEL_SynchronizePartitions(partition, 
            BARRIER_TYPE_StatAggregateEnd);
#else
        for (i = 0; i < partition->getNumPartitions(); i++)
        {
            if (i != partition->partitionId)
            {
                STAT_DataList* agg = (STAT_DataList*)
                    gIncomingData[i];
                while (agg != NULL)
                {
                    ERROR_Assert(
                        m_AggregatedStats[agg->id]->GetId() == agg->id,
                        "IDs don't match");
                    m_AggregatedStats[agg->id]->DeserializeAndUpdate(
                        partition->getGlobalTime(),
                        agg->data,
                        agg->size);

                    agg = agg->next;
                }
            }
        }
#endif


#ifndef USE_MPI
        // Each partition frees its own memory
        // Free on one partition -- only 1 copy is created
        STAT_DataList* agg;
        STAT_DataList* temp;
        if (partition->partitionId == 0)
        {
            agg = (STAT_DataList*)
                gIncomingData[1][partition->partitionId];
        }
        else
        {
            agg = (STAT_DataList*)
                gIncomingData[0][partition->partitionId];
        }

        while (agg != NULL)
        {
            temp = agg;
            agg = agg->next;
            MEM_free(temp->data);
            MEM_free(temp);
        }

        // Now set to NULL
        for (i = 0; i < partition->getNumPartitions(); i++)  
        {
            gIncomingData[i][partition->partitionId] = NULL;
        }
#else
        // Free memory of all partitions
        for (i = 0; i < partition->getNumPartitions(); i++)  
        {
            STAT_DataList* agg;
                agg = (STAT_DataList*)
                gIncomingData[i];
            while (agg != NULL)
            {
                gIncomingData[i] = agg->next;
                MEM_free(agg->data);
                MEM_free(agg);
                agg = (STAT_DataList*)
                    gIncomingData[i];
            }
    }
#endif
    }

    // Now aggregate higher order stats.  This array is sorted by the 
    // statistic's order.
    for (unsigned int i = 0; i < m_OrderedAggregatedStats.size(); i++)
    {
        m_OrderedAggregatedStats[i]->Aggregate(partition->getGlobalTime());
    }

#ifdef DEBUG_TIMING
    double finish = partition->wallClock->getTrueRealTimeAsDouble();
    double elapsed = finish - startTiming;
    printf("Partition %d aggregation took %f seconds\n",
        partition->partitionId, elapsed);
#endif

    // Print out stats
#ifdef PRINT_AGGREGATED_STATS
    if (partition->partitionId == 0)
    {
        for (int i = 0; i < m_AggregatedStats.size(); i++)
        {
            printf("%s = %g (%s)\n",
                m_AggregatedStats[i]->GetName().c_str(),
                m_AggregatedStats[i]->GetValue(partition->getGlobalTime()),
                m_AggregatedStats[i]->GetUnits().c_str());
        }
    }
#endif
}

void STAT_StatisticsList::SummarizeMulticastApp(PartitionData* partition)
{
    STAT_MulticastAppSessionSummarizer& multicastSessionSummary =
        partition->stats->global.appMulticastSessionSummary;

    STAT_MulticastAppSessionSummaryStatistics summary;
    STAT_MulticastAppSummaryTag tag;
    std::vector<STAT_ModelStatistics*>::iterator it;

    if (partition->getGlobalTime() > multicastSessionSummary.m_LastSummarizeTime)
    {
        multicastSessionSummary.clear();

        multicastSessionSummary.m_LastSummarizeTime = partition->getGlobalTime();

#ifdef DEBUG_TIMING
        double startTiming = partition->wallClock->getTrueRealTimeAsDouble();
#endif

        // Loop over all app model statistics and add summary stats
        for (it = m_ModelStatistics.begin();
            it != m_ModelStatistics.end();
            ++it)
        {
            STAT_AppStatistics* appStats = (STAT_AppStatistics*) *it;

            for (STAT_AppStatistics::MulticastAppSessionIterator sessionIt = appStats->GetSessionBegin();
                 sessionIt != appStats->GetSessionEnd();
                 sessionIt++)
            {
                 // Fill in summary stats
                 summary.InitializeFromModel(
                     partition->getGlobalTime(),
                     sessionIt->second,
                     appStats);

                 tag.m_senderId = summary.m_senderId;
                 tag.m_destId = summary.m_DestNodeId;
                 tag.m_SessionId = summary.m_SessionId;
                 tag.m_receiverAddr = summary.receiverAddress;
                 multicastSessionSummary.AddRemoteData(tag, summary);
            }
        }

        multicastSessionSummary.Summarize(partition);

#ifdef DEBUG_TIMING
        double finish = partition->wallClock->getTrueRealTimeAsDouble();
        double elapsed = finish - startTiming;
        printf("Partition %d summarization took %f seconds\n",
            partition->partitionId, elapsed);
#endif
    }

}
void STAT_StatisticsList::SummarizeApp(PartitionData* partition)
{
    STAT_AppSummarizer& unicastSummary = partition->stats->global.appUnicastSummary;
    STAT_AppMulticastSummarizer& multicastSummary = partition->stats->global.appMulticastSummary;

    STAT_AppSummaryStatistics summary;
    STAT_AppSummaryTag11 tag;
    STAT_AppSummaryTag1N multicastTag;
    std::vector<STAT_ModelStatistics*>::iterator it;


    if (partition->getGlobalTime() > unicastSummary.m_LastSummarizeTime
        || partition->getGlobalTime() > multicastSummary.m_LastSummarizeTime)
    {
        // Empty old stats
        unicastSummary.clear();
        multicastSummary.clear();

        unicastSummary.m_LastSummarizeTime = partition->getGlobalTime(); 
        multicastSummary.m_LastSummarizeTime = partition->getGlobalTime();

#ifdef DEBUG_TIMING
        double startTiming = partition->wallClock->getTrueRealTimeAsDouble();
#endif

        // Loop over all app model statistics and add summary stats
        for (it = m_ModelStatistics.begin();
            it != m_ModelStatistics.end();
            ++it)
        {
            STAT_AppStatistics* appStats = (STAT_AppStatistics*) *it;

            // Fill in summary stats
            summary.InitializeFromModel(partition->firstNode, appStats);

            if (!appStats->GetMulticast())
            {
                // Unicast app
                tag.sender = summary.senderId;
                tag.instanceId = summary.sessionId;
                if (summary.receiverAddress == ANY_DEST)
                {
                    summary.receiverId = 0;
                }
                unicastSummary.AddRemoteData(tag, summary);
            }
            else
            {
                // Multicast app
                multicastTag.sender = summary.senderId;
                multicastTag.receiver = appStats->GetDestAddr();
                multicastTag.instanceId = summary.sessionId;
                summary.receiverId = 0;
                multicastSummary.AddRemoteData(multicastTag, summary);
            }
        }

        // Now summarize.  Partition 0 will have the full summarized stats.
        unicastSummary.Summarize(partition);
        multicastSummary.Summarize(partition);

#ifdef DEBUG_TIMING
        double finish = partition->wallClock->getTrueRealTimeAsDouble();
        double elapsed = finish - startTiming;
        printf("Partition %d summarization took %f seconds\n",
            partition->partitionId, elapsed);
#endif
    }
}

void STAT_StatisticsList::SummarizeTransport(PartitionData* partition)
{
    STAT_TransportSummarizer& transportSummary = partition->stats->global.transportSummary;
    STAT_TransportSummaryStatistics summary;
    STAT_TransportSummaryTag tag;


    if (partition->getGlobalTime() > transportSummary.m_LastSummarizeTime)
    {
        // Empty old stats
        transportSummary.clear();

        transportSummary.m_LastSummarizeTime = partition->getGlobalTime();


#ifdef DEBUG_TIMING
        double startTiming = partition->wallClock->getTrueRealTimeAsDouble();
#endif

        // Network stats summarization requires ADDON_DB to be enabled
        for (Node* node = partition->firstNode;
            node != NULL;
            node = node->nextNodeData)
        {
            // Get TCP stats
            TransportDataTcp* tcpLayer = (TransportDataTcp*) node->transportData.tcp;
            if (!tcpLayer->newStats)
            {
                continue;
            }

            for (STAT_TransportStatistics::SessionIter it = tcpLayer->newStats->SessionBegin();
                it != tcpLayer->newStats->SessionEnd();
                ++it)
            {
                tag.sourceAddr = GetIPv4Address(it->second->GetSourceAddr());
                tag.destAddr = GetIPv4Address(it->second->GetDestAddr());
                tag.sourcePort = it->second->GetSourcePort();
                tag.destPort = it->second->GetDestPort();

                summary.InitializeFromModel(node, it->second);
                transportSummary.AddRemoteData(tag, summary);
            }

            // Get UDP stats
            TransportDataUdp* udpLayer = (TransportDataUdp*) node->transportData.udp;
            if (!udpLayer->newStats)
            {
                continue;
            }

            for (STAT_TransportStatistics::SessionIter it = udpLayer->newStats->SessionBegin();
                it != udpLayer->newStats->SessionEnd();
                ++it)
            {
                tag.sourceAddr = GetIPv4Address(it->second->GetSourceAddr());
                tag.destAddr = GetIPv4Address(it->second->GetDestAddr());
                tag.sourcePort = it->second->GetSourcePort();
                tag.destPort = it->second->GetDestPort();

                summary.InitializeFromModel(node, it->second);
                transportSummary.AddRemoteData(tag, summary);
            }
        }

        // Now summarize.  Partition 0 will have the full summarized stats.
        transportSummary.Summarize(partition);

#ifdef DEBUG_TIMING
        double finish = partition->wallClock->getTrueRealTimeAsDouble();
        double elapsed = finish - startTiming;
        printf("Partition %d summarization took %f seconds\n",
            partition->partitionId, elapsed);
#endif

#ifdef PRINT_AGGREGATED_STATS
        it = transportSummary.begin();
        while (it != transportSummary.end())
        std::map<STAT_TransportSummaryTag, STAT_TransportSummaryStatistics>::iterator it = transportSummary.begin();
        while (it != transportSummary.end())
        {
            const STAT_TransportSummaryTag& tag = it->first;
            STAT_TransportSummaryStatistics& s = it->second;
            printf("Transport %d (%d) -> %d (%d), sent %d received %d\n",
                tag.sourceAddr,
                tag.sourcePort,
                tag.destAddr,
                tag.destPort,
                s.senderSegmentsSentToNet,
                s.receiverSegmentsRcvdFromNet);
            ++it;
        }
#endif
    } 
}

void STAT_StatisticsList::SummarizeNet(PartitionData* partition)
{
    STAT_NetSummarizer& netSummary = partition->stats->global.netSummary;
    STAT_NetSummaryStatistics summary;
    STAT_NetSummaryTag11 tag;

#ifdef DEBUG_TIMING
    double startTiming = partition->wallClock->getTrueRealTimeAsDouble();
#endif

    if (partition->getGlobalTime() > netSummary.m_LastSummarizeTime)
    {
        netSummary.m_LastSummarizeTime = partition->getGlobalTime();
    }
    else 
    {
        // already summarized. 
        return; 
    }

    // Empty old stats
    netSummary.clear();

    // Network stats summarization requires ADDON_DB to be enabled
#ifdef ADDON_DB
    for (Node* traverseNode = partition->firstNode;
        traverseNode != NULL;
        traverseNode = traverseNode->nextNodeData)
    {
        // Now we have to loop over the list of one hop neighbor, and insert
        // the data in the table.
        NetworkDataIp* ip = (NetworkDataIp *) traverseNode->networkData.networkVar;
        ERROR_Assert(ip != NULL, "Error in StatsDb, Network Data Unavailable\n");

        // Check that one hop stats are enabled
        if (ip->oneHopData == NULL)
        {
            continue;
        }

        for (NetworkSumAggrDataIter iter = ip->oneHopData->begin();
            iter != ip->oneHopData->end();
            ++iter)
        {
            // Fill in summary stats for sending and receiving side
            summary.InitializeFromOneHop(traverseNode, iter->second);
            tag.sender = summary.senderAddr;
            tag.receiver = summary.receiverAddr;
            netSummary.AddRemoteData(tag, summary);
        }
    }
#endif /* ADDON_DB */

    // Now summarize.  Partition 0 will have the full summarized stats.
    netSummary.Summarize(partition);

#ifdef DEBUG_TIMING
    double finish = partition->wallClock->getTrueRealTimeAsDouble();
    double elapsed = finish - startTiming;
    printf("Partition %d summarization took %f seconds\n",
        partition->partitionId, elapsed);
#endif

#ifdef PRINT_AGGREGATED_STATS
    std::map<STAT_NetSummaryTag11, STAT_NetSummaryStatistics>::iterator it = netSummary.begin();
    while (it != netSummary.end())
    {
        STAT_NetSummaryStatistics& s = it->second;
        printf("%d -> %d, type %s, sentDP %f, recDP %f, forDP %f, sentCP %f, recCP %f, forCP %f, %f, %f, %f, %f, %f, %f, delD %f, jitD %f, delC %f, jitC %f\n",
            s.senderAddr,
            s.receiverAddr,
            s.destinationType,
            s.dataPacketsSent,
            s.dataPacketsRecd,
            s.dataPacketsForward,
            s.controlPacketsSent,
            s.controlPacketsRecd,
            s.controlPacketsForward,
            s.dataBytesSent,
            s.dataBytesRecd,
            s.dataBytesForward,
            s.controlBytesSent,
            s.controlBytesRecd,
            s.controlBytesForward,
            s.dataDelay,
            s.dataJitter,
            s.controlDelay,
            s.controlJitter);
        ++it;
    }
#endif
}
void STAT_StatisticsList::SummarizeMac(PartitionData* partition)
{
    STAT_MacSummarizer& macSummary = partition->stats->global.macSummary;
    STAT_MacSummaryStatistics summary;
    STAT_MacSummaryTag tag;

    if (partition->getGlobalTime() > macSummary.m_LastSummarizeTime)
    {
        macSummary.m_LastSummarizeTime = partition->getGlobalTime();
    }
    else 
    {
        // already summarized. 
        return; 
    }
    
    // Empty old stats
    macSummary.clear();

#ifdef DEBUG_TIMING
        double startTiming = partition->wallClock->getTrueRealTimeAsDouble();
#endif

        for (Node* node = partition->firstNode;
            node != NULL;
            node = node->nextNodeData)
        {
            for (int i = 0; i < node->numberInterfaces; i++)
            {
                STAT_MacStatistics* macStatistics = NULL;

                if (node->macData[i])
                {
                    macStatistics = node->macData[i]->stats;
                }

                if (!macStatistics)
                {
                    continue;
                }

                for (STAT_MacStatistics::MacSessionIterator it = macStatistics->SessionBegin();
                    it != macStatistics->SessionEnd();
                    ++it)
                {
                    tag.sender = it->second->GetSender();
                    tag.receiver = it->second->GetReceiver();
                    tag.interfaceIndex = it->second->GetInterfaceIndex();

                    summary.InitializeFromModel(node, it->second);
                    macSummary.AddRemoteData(tag, summary);
                }
            }
        }

        // Now summarize.  Partition 0 will have the full summarized stats.
        macSummary.Summarize(partition);

#ifdef DEBUG_TIMING
        double finish = partition->wallClock->getTrueRealTimeAsDouble();
        double elapsed = finish - startTiming;
        printf("Partition %d summarization took %f seconds\n",
            partition->partitionId, elapsed);
#endif

    
#ifdef PRINT_AGGREGATED_STATS
    std::map<STAT_MacSummaryTag, STAT_MacSummaryStatistics>::iterator it = macSummary.begin();
    while (it != macSummary.end())
    {
        STAT_MacSummaryStatistics s = it->second;
        printf("%d -> %d interface %d,  totalDataFramesSentUnicast %d, totalDataFramesReceivedUnicast %d, totalControlFramesSentUnicast %d, totalControlFramesReceivedUnicast %d, "
            "totalDataBytesSentUnicast %d, totalDataBytesReceivedUnicast %d, totalControlBytesSentUnicast %d, totalControlBytesReceivedUnicast %d,"
            "averageQueuingDelayUnicast %d, averageMediumAccessDelayUnicast %d, averageMediumDelayUnicast %d, averageJitterUnicast %d,"
            "totalDataFramesSentBroadcast %d, totalDataFramesReceivedBroadcast %d, totalControlFramesSentBroadcast %d, totalControlFramesReceivedBroadcast %d,"
            "totalDataBytesSentBroadcast %d, totalDataBytesReceivedBroadcast %d, totalControlBytesSentBroadcast %d, totalControlBytesReceivedBroadcast %d,"
            "averageQueuingDelayBroadcast %d, averageMediumAccessDelayBroadcast %d, averageMediumDelayBroadcast %d, averageJitterBroadcast %d,"
            "totalDataFramesSentMulticast %d, totalDataFramesReceivedMulticast %d, totalControlFramesSentMulticast %d, totalControlFramesReceivedMulticast %d,"
            "totalDataBytesSentMulticast %d, totalDataBytesReceivedMulticast %d, totalControlBytesSentMulticast %d, totalControlBytesReceivedMulticast %d,"
            "averageQueuingDelayMulticast %d, averageMediumAccessDelayMulticast %d, averageMediumDelayMulticast %d, averageJitterMulticast %d,"
            "totalFramesDroppedSender %d, totalFramesDroppedReceiver %d, totalBytesDroppedSender %d, totalBytesDroppedReceiver %d\n",
            s.senderId,
            s.receiverId,
            s.interfaceIndex,           
            s.m_AddrStats[STAT_Unicast].totalDataFramesSent,
            s.m_AddrStats[STAT_Unicast].totalDataFramesReceived,
            s.m_AddrStats[STAT_Unicast].totalControlFramesSent,
            s.m_AddrStats[STAT_Unicast].totalControlFramesReceived,
            s.m_AddrStats[STAT_Unicast].totalDataBytesSent,
            s.m_AddrStats[STAT_Unicast].totalDataBytesReceived,
            s.m_AddrStats[STAT_Unicast].totalControlBytesSent,
            s.m_AddrStats[STAT_Unicast].totalControlBytesReceived,
            s.m_AddrStats[STAT_Unicast].averageQueuingDelay,
            s.m_AddrStats[STAT_Unicast].averageMediumAccessDelay,
            s.m_AddrStats[STAT_Unicast].averageMediumDelay,
            s.m_AddrStats[STAT_Unicast].averageJitter,            
            s.m_AddrStats[STAT_Broadcast].totalDataFramesSent,
            s.m_AddrStats[STAT_Broadcast].totalDataFramesReceived,
            s.m_AddrStats[STAT_Broadcast].totalControlFramesSent,
            s.m_AddrStats[STAT_Broadcast].totalControlFramesReceived,
            s.m_AddrStats[STAT_Broadcast].totalDataBytesSent,
            s.m_AddrStats[STAT_Broadcast].totalDataBytesReceived,
            s.m_AddrStats[STAT_Broadcast].totalControlBytesSent,
            s.m_AddrStats[STAT_Broadcast].totalControlBytesReceived,
            s.m_AddrStats[STAT_Broadcast].averageQueuingDelay,
            s.m_AddrStats[STAT_Broadcast].averageMediumAccessDelay,
            s.m_AddrStats[STAT_Broadcast].averageMediumDelay,
            s.m_AddrStats[STAT_Broadcast].averageJitter,            
            s.m_AddrStats[STAT_Multicast].totalDataFramesSent,
            s.m_AddrStats[STAT_Multicast].totalDataFramesReceived,
            s.m_AddrStats[STAT_Multicast].totalControlFramesSent,
            s.m_AddrStats[STAT_Multicast].totalControlFramesReceived,
            s.m_AddrStats[STAT_Multicast].totalDataBytesSent,
            s.m_AddrStats[STAT_Multicast].totalDataBytesReceived,
            s.m_AddrStats[STAT_Multicast].totalControlBytesSent,
            s.m_AddrStats[STAT_Multicast].totalControlBytesReceived,
            s.m_AddrStats[STAT_Multicast].averageQueuingDelay,
            s.m_AddrStats[STAT_Multicast].averageMediumAccessDelay,
            s.m_AddrStats[STAT_Multicast].averageMediumDelay,
            s.m_AddrStats[STAT_Multicast].averageJitter,            
            s.totalFramesDroppedSender,
            s.totalFramesDroppedReceiver,
            s.totalBytesDroppedSender,
            s.totalBytesDroppedReceiver,);
        ++it;
    }
#endif
}

void STAT_StatisticsList::SummarizePhy(PartitionData* partition)
{
    STAT_PhySummarizer& phySummary = partition->stats->global.phySummary;
    STAT_PhySummaryTag tag;

#ifdef DEBUG_TIMING
    double startTiming = partition->wallClock->getTrueRealTimeAsDouble();
#endif

    if (partition->getGlobalTime() > phySummary.m_LastSummarizeTime)
    {
        phySummary.m_LastSummarizeTime = partition->getGlobalTime();
    }
    else 
    {
        // already summarized. 
        return; 
    }

    // Empty old stats
    phySummary.clear();

    for (Node* traverseNode = partition->firstNode;
        traverseNode != NULL;
        traverseNode = traverseNode->nextNodeData)
    {
        for (int i = 0; i < traverseNode->numberPhys; i++)
        {
            // Ignore if this phy model does not implement Stats API
            STAT_PhyStatistics* phyStatistics = traverseNode->phyData[i]->stats;
            if (!phyStatistics)
            {
                continue;
            }

            for (STAT_PhyStatistics::SessionIterator it = phyStatistics->SessionBegin();
                it != phyStatistics->SessionEnd();
                it++)
            {
                // Fill in summary stats for sending and receiving side
                tag.sender = it->second.senderId;
                tag.receiver = it->second.receiverId;
                tag.channelIndex = it->second.channelIndex;
                phySummary.AddRemoteData(tag, it->second);
            }
        }
    }

    // Now summarize.  Partition 0 will have the full summarized stats.
    phySummary.Summarize(partition);

#ifdef DEBUG_TIMING
    double finish = partition->wallClock->getTrueRealTimeAsDouble();
    double elapsed = finish - startTiming;
    printf("Partition %d summarization took %f seconds\n",
        partition->partitionId, elapsed);
#endif

#ifdef PRINT_AGGREGATED_STATS
    std::map<STAT_PhySummaryTag, STAT_PhySessionStatistics>::iterator it = phySummary.begin();
    while (it != phySummary.end())
    {
        STAT_PhySessionStatistics& s = it->second;
        printf("%d -> %d channel %d, utilization %f, totalInterference %f, totalPathLoss %f, totalDelay %f, totalSignalPower %f, numSignals %d, numErrorSignals %d\n",
            s.senderId,
            s.receiverId,
            s.channelIndex,
            s.utilization,
            s.totalInterference,
            s.totalPathLoss,
            s.totalDelay,
            s.totalSignalPower,
            s.numSignals,
            s.numErrorSignals);
        ++it;
    }
#endif
}

void STAT_StatisticsList::SummarizeQueue(PartitionData* partition)
{
    STAT_QueueSummarizer& queueSummary = partition->stats->global.queueSummary;
    STAT_QueueSummaryStatistics summary;
    STAT_QueueSummaryTag tag;

#ifdef DEBUG_TIMING
    double startTiming = partition->wallClock->getTrueRealTimeAsDouble();
#endif

    if (partition->getGlobalTime() > queueSummary.m_LastSummarizeTime)
    {
        queueSummary.m_LastSummarizeTime = partition->getGlobalTime();
    }
    else 
    {
        // already summarized. 
        return; 
    }
        
    // Empty old stats
    queueSummary.clear();

    // first loop over any inactive queues
    int i;
    for (i = 0; i < partition->stats->global.inactiveQueues.size(); i++)
    {
        STAT_QueueStatistics* stats = partition->stats->global.inactiveQueues[i];

        if (stats)
        {
            // Fill in summary stats for sending and receiving side
            tag.node = stats->GetNode()->nodeId;
            tag.queuePosition = stats->GetQueuePosition();
            tag.queueIndex = stats->GetQueueIndex();
            tag.interfaceIndex = stats->GetInterfaceIndex();

            summary.InitializeFromModel(stats->GetNode(),
                stats,
                0);   //these dead queues should show they have zero free space

            queueSummary.AddRemoteData(tag, summary);
        }
    }

    Scheduler *schedulerPtr = NULL;
    i = 0;
    int j = 0;
    size_t k = 0;
    int index = 0;

    for (Node* traverseNode = partition->firstNode;
        traverseNode != NULL;
        traverseNode = traverseNode->nextNodeData)
    {
        NetworkDataIp *ip
            = (NetworkDataIp *) traverseNode->networkData.networkVar;

        if (ip->cpuScheduler)
        {
            schedulerPtr = ip->cpuScheduler;

            for (j = 0; j < schedulerPtr->numQueue(); ++j)
            {
                STAT_QueueStatistics* stats = schedulerPtr->queueData[j].queue->stats;
                
                if (!stats)
                {
                    continue;
                }
                // Fill in summary stats for sending and receiving side
                tag.node = stats->GetNode()->nodeId;
                tag.queuePosition = stats->GetQueuePosition();
                tag.queueIndex = stats->GetQueueIndex();
                tag.interfaceIndex = stats->GetInterfaceIndex();

                summary.InitializeFromModel(traverseNode,
                    stats,
                    schedulerPtr->queueData[j].queue->freeSpaceInQueue());

                queueSummary.AddRemoteData(tag, summary);
            }
        }
        for (i = 0; i < traverseNode->numberInterfaces; ++i)
        {
            if (ip->interfaceInfo[i]->isVirtualInterface)
            {
                continue;
            }

            vector<Scheduler *> vPtr;
            vPtr.push_back(ip->interfaceInfo[i]->scheduler);
            vPtr.push_back(ip->interfaceInfo[i]->inputScheduler);

            for (k = 0; k < vPtr.size(); ++k)
            {
                for (j = 0; vPtr[k] && j < vPtr[k]->numQueue(); ++j)
                {
                    STAT_QueueStatistics* stats = vPtr[k]->queueData[j].queue->stats;
                
                    if (!stats)
                    {
                        continue;
                    }
                    // Fill in summary stats for sending and receiving side
                    tag.node = stats->GetNode()->nodeId;
                    tag.queuePosition = stats->GetQueuePosition();
                    tag.queueIndex = stats->GetQueueIndex();
                    tag.interfaceIndex = stats->GetInterfaceIndex();

                    summary.InitializeFromModel(traverseNode,
                        stats,
                        vPtr[k]->queueData[j].queue->freeSpaceInQueue());

                    queueSummary.AddRemoteData(tag, summary);
                }
            }
        }
    }
}
void STAT_StatisticsList::Reset(PartitionData* partition)
{
    // Set up a hierarchy iterator over all statistics
    std::string root = "";
    D_ObjectIterator it(&partition->dynamicHierarchy, root);
    it.SetType(D_STATISTIC);
    it.SetRecursive(true);

    // Loop over all statistics and call their reset
    while (it.GetNext())
    {
        // Get statistic object.  Check to make sure that it is actually a
        // statistic.
        D_StatisticObj* statObj =
            dynamic_cast<D_StatisticObj*>(it.GetObject());
        ERROR_Assert(
            statObj != NULL,
            "Got null object from hierarchy or got non-statistic from "
            "hierarchy");

        // Get the actual statistic from the dynamic object
        STAT_Statistic* stat = statObj->GetStatistic();
        ERROR_Assert(stat != NULL, "Statistics object has no statistic");

        // Reset the statistic
        stat->Reset();
    }
}

class CacheOperator
{
public:
    bool operator () (const std::string* a, const std::string* b)
    {
        return *a < *b;
    }
};

const std::string* STAT_StatisticsList::CacheString(const std::string& str)
{
    static QNPartitionMutex stringCacheLock;
    static std::vector<const std::string*> m_StringCache;
    CacheOperator op;

    std::vector<const std::string*>::iterator it;

    // Only one partition may access the cache at once
    // Once a string is cached it will be accessed by other models, but
    // only once per model.
    QNPartitionLock lock(&stringCacheLock);

    // Check if string is already in cache
    it = std::lower_bound(m_StringCache.begin(), m_StringCache.end(), &str, op);
    if (it != m_StringCache.end() && *(*it) == str)
    {
        return *it;
    }
    else
    {
        // Not in cache.  Cache string, then return pointer.
        std::string* newStr = new std::string(str);
        m_StringCache.push_back(newStr);
        std::sort(m_StringCache.begin(), m_StringCache.end(), op);

        return newStr;
    }
}

BOOL STAT_IsParallel(PartitionData* partition)
{
    return partition->isRunningInParallel();
}

int STAT_PartitionId(PartitionData* partition)
{
    return partition->partitionId;
}

int STAT_NumPartitions(PartitionData* partition)
{
    return partition->getNumPartitions();
}

/*  STAT_GlobalStatistics constructor  */
STAT_GlobalStatistics::STAT_GlobalStatistics()
{
#ifdef ADDON_DB
    // assigns NULL value to bridge object pointers
    appBridge = NULL;
    appSummaryBridge = NULL;
    netBridge = NULL;
    netSummaryBridge = NULL;
    transportBridge = NULL;
    transportSummaryBridge = NULL;
    macBridge = NULL;
    macSummaryBridge = NULL;
    phyBridge = NULL;
    phySummaryBridge = NULL;
    queueBridge = NULL;
    queueSummaryBridge = NULL;
    queueStatusBridge = NULL;
    multicastAppSummaryBridge = NULL;
#endif
}

