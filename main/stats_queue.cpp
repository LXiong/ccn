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

#include "api.h"
#include "partition.h"
#include "stats_queue.h"
#include "stats_global.h"

#ifdef ADDON_DB
#include "db.h"
#endif

void STAT_QueueStatistics::GetList(std::vector<STAT_Statistic*>& stats)
{
    stats.push_back(&GetPacketsEnqueued());
    stats.push_back(&GetPacketsDequeued());
    stats.push_back(&GetPacketsDropped());
    stats.push_back(&GetPacketsDroppedForcefully());
    stats.push_back(&GetPacketsDroppedForcefullyFromAging());
    stats.push_back(&GetAverageQueueLength());
    stats.push_back(&GetLongestTimeInQueue());
    stats.push_back(&GetPeakQueueLength());
}

void STAT_QueueStatistics::SetStatNames()
{
    std::string name;
    std::string description;
    std::string units;

    name = "QueuePacketsEnqueued";
    description = "Total Packets Enqueued";
    units = "packets";
    GetPacketsEnqueued().SetInfo(name, description, units);

    name = "QueuePacketsDequeued";
    description = "Total Packets Dequeued";
    units = "packets";
    GetPacketsDequeued().SetInfo(name, description, units);

    name = "QueuePacketsDropped";
    description = "Total Packets Dropped";
    units = "packets";
    GetPacketsDropped().SetInfo(name, description, units);

    name = "QueuePacketsDroppedForcefully";
    description = "Total Packets Dropped Forcefully";
    units = "packets";
    GetPacketsDroppedForcefully().SetInfo(name, description, units);

    name = "QueuePacketsDroppedForcefullyFromAging";
    description = "Total Packets Dropped Forcefully from Aging";
    units = "packets";
    GetPacketsDroppedForcefullyFromAging().SetInfo(name, description, units);

    name = "QueueBytesEnqueued";
    description = "Total Bytes Enqueued";
    units = "bytes";
    GetBytesEnqueued().SetInfo(name, description, units);

    name = "QueueBytesDequeued";
    description = "Total Bytes Dequeued";
    units = "bytes";
    GetBytesDequeued().SetInfo(name, description, units);

    name = "QueueBytesDropped";
    description = "Total Bytes Dropped";
    units = "bytes";
    GetBytesDropped().SetInfo(name, description, units);

    name = "QueueBytesDroppedForcefully";
    description = "Total Bytes Dropped Forcefully";
    units = "bytes";
    GetBytesDroppedForcefully().SetInfo(name, description, units);

    name = "AverageQueueLength";
    description = "Average Queue Length";
    units = "bytes";
    GetAverageQueueLength().SetInfo(name, description, units, STAT_Float);

    name = "PeakQueueLength";
    description = "Peak Queue Length";
    units = "bytes";
    GetPeakQueueLength().SetInfo(name, description, units);

    name = "LongestTimeInQueue";
    description = "Longest Time in Queue";
    units = "seconds";
    GetLongestTimeInQueue().SetInfo(name, description, units);
}

void STAT_QueueStatistics::AddToGlobal(STAT_GlobalQueueStatistics* stat)
{
    stat->GetPacketsEnqueued().AddStatistic(&GetPacketsEnqueued());
    stat->GetPacketsDequeued().AddStatistic(&GetPacketsDequeued());
    stat->GetPacketsDropped().AddStatistic(&GetPacketsDropped());
    stat->GetPacketsDroppedForcefully().AddStatistic(&GetPacketsDroppedForcefully());
    stat->GetBytesEnqueued().AddStatistic(&GetBytesEnqueued());
    stat->GetBytesDequeued().AddStatistic(&GetBytesDequeued());
    stat->GetBytesDropped().AddStatistic(&GetBytesDropped());
    stat->GetBytesDroppedForcefully().AddStatistic(&GetBytesDroppedForcefully());
}

STAT_QueueStatistics::STAT_QueueStatistics(Node* node,
    std::string queuePosition,
    int interfaceIndex,
    std::string queueType,
    int queueSize,
    int queueIndex,
    clocktype timeOfCreation)
{  
    m_Node = node;
    m_QueuePosition = queuePosition;
    m_InterfaceIndex = interfaceIndex;
    m_QueueSize = queueSize;
    m_QueueType = queueType;
    m_QueueIndex = queueIndex;
    m_lastChange = 0;
    m_creationTime = timeOfCreation;

    SetStatNames();
    AddToGlobal(&node->partitionData->stats->global.queueAggregate);
    m_AverageQueueLength.SetLastChange(timeOfCreation);
}

void STAT_QueueStatistics::EnqueuePacket(Message* msg,
    int totalBytesInQueueBeforeOperation,
    const clocktype currentTime)
{
    m_AverageQueueLength.AddDataPoint(currentTime,
        totalBytesInQueueBeforeOperation);
    m_PacketsEnqueued.AddDataPoint(m_Node->getNodeTime(), 1);
    m_BytesEnqueued.AddDataPoint(m_Node->getNodeTime(),
        MESSAGE_ReturnPacketSize(msg));
    m_PeakQueueLength.AddDataPoint(m_Node->getNodeTime(),
        totalBytesInQueueBeforeOperation + MESSAGE_ReturnPacketSize(msg));    
}

void STAT_QueueStatistics::FinalizeQueue(const clocktype currentTime,
    int currentBytesInQueue)
{
    m_AverageQueueLength.AddDataPoint(currentTime,
        currentBytesInQueue);
}

void STAT_QueueStatistics::DequeuePacket(Message* msg,
    int totalBytesInQueue,
    const clocktype currentTime)
{
    m_AverageQueueLength.AddDataPoint(currentTime,
        totalBytesInQueue);
    m_PacketsDequeued.AddDataPoint(m_Node->getNodeTime(), 1);
    m_BytesDequeued.AddDataPoint(m_Node->getNodeTime(),
        MESSAGE_ReturnPacketSize(msg));    
}

void STAT_QueueStatistics::DropPacketForcefully(Message* msg,
    int totalBytesInQueueBeforeOperation,
    const clocktype currentTime,
    bool fromAging)
{
    m_AverageQueueLength.AddDataPoint(currentTime,
        totalBytesInQueueBeforeOperation);
    if (!fromAging)
    {
        m_PacketsDroppedForcefully.AddDataPoint(m_Node->getNodeTime(), 1);
        m_BytesDroppedForcefully.AddDataPoint(m_Node->getNodeTime(),
            MESSAGE_ReturnPacketSize(msg));
    }
    else
    {
        m_PacketsDroppedForcefullyFromAging.AddDataPoint(m_Node->getNodeTime(), 1);
    }    
}

void STAT_QueueStatistics::DropPacket(Message* msg,
    int totalBytesInQueueBeforeOperation,
    const clocktype currentTime)
{
    m_AverageQueueLength.AddDataPoint(currentTime,
        totalBytesInQueueBeforeOperation);
    m_PacketsDropped.AddDataPoint(m_Node->getNodeTime(), 1);
    m_BytesDropped.AddDataPoint(m_Node->getNodeTime(),
        MESSAGE_ReturnPacketSize(msg));
}

void STAT_QueueStatistics::DelayDetected(clocktype delay)
{
    if (delay > 0)
    {
        m_totalDelays.AddDataPoint(m_Node->getNodeTime(), ((double) delay / SECOND));
        m_LongestTimeInQueue.AddDataPoint(m_Node->getNodeTime(), ((double) delay / SECOND));
    }
}

STAT_QueueSummaryStatistics& STAT_QueueSummaryStatistics::operator +=(
    const STAT_QueueSummaryStatistics& stats)
{
    totalPacketsEnqueued += stats.totalPacketsEnqueued;
    totalPacketsDequeued += stats.totalPacketsDequeued;
    totalPacketsDropped += stats.totalPacketsDropped;
    totalPacketsDroppedForcefully += stats.totalPacketsDroppedForcefully;
    totalPacketsDroppedForcefullyFromAging +=
        stats.totalPacketsDroppedForcefullyFromAging;
    totalBytesEnqueued += stats.totalBytesEnqueued;
    totalBytesDequeued += stats.totalBytesDequeued;
    totalBytesDropped += stats.totalBytesDropped;
    totalBytesDroppedForcefully += stats.totalBytesDroppedForcefully;

    totalDelays += stats.totalDelays;

    if (stats.peakQueueLength > peakQueueLength)
    {
        peakQueueLength = stats.peakQueueLength;
    }
    if (stats.longestTimeInQueue > longestTimeInQueue)
    {
        longestTimeInQueue = stats.longestTimeInQueue;
    }
    averageQueueLength += stats.averageQueueLength;
    curFreeSpace = stats.curFreeSpace;
    return *this;
}

void STAT_QueueSummaryStatistics::InitializeFromModel(
    Node* node,
    STAT_QueueStatistics* stats,
    int currentFreeSpace)
{
    clocktype now = node->getNodeTime();
    
    nodeId = stats->GetNode()->nodeId;
    queuePosition = stats->GetQueuePosition();
    queueType = stats->GetQueueType();
    queueSize = stats->GetQueueSize();
    interfaceIndex = stats->GetInterfaceIndex();
    queueIndex = stats->GetQueueIndex();

    totalPacketsEnqueued = (UInt64) stats->GetPacketsEnqueued().GetValue(now);
    totalPacketsDequeued = (UInt64) stats->GetPacketsDequeued().GetValue(now);
    totalPacketsDropped = (UInt64) stats->GetPacketsDropped().GetValue(now);
    totalPacketsDroppedForcefully = (UInt64) stats->GetPacketsDroppedForcefully().GetValue(now);
    totalPacketsDroppedForcefullyFromAging = (UInt64) stats->GetPacketsDroppedForcefullyFromAging().GetValue(now);
    totalBytesEnqueued = (UInt64) stats->GetBytesEnqueued().GetValue(now);
    totalBytesDequeued = (UInt64) stats->GetBytesDequeued().GetValue(now);
    totalBytesDropped = (UInt64) stats->GetBytesDropped().GetValue(now);
    totalBytesDroppedForcefully = (UInt64) stats->GetBytesDroppedForcefully().GetValue(now);
    
    totalDelays = stats->GetTotalDelays().GetValue(now);

    peakQueueLength = (UInt64) stats->GetPeakQueueLength().GetValue(now);
    longestTimeInQueue = stats->GetLongestTimeInQueue().GetValue(now);
    
    averageQueueLength = stats->GetAverageQueueLength().GetValue(now);
    curFreeSpace = currentFreeSpace;
}
void STAT_GlobalQueueStatistics::Initialize(
    PartitionData* partition,
    STAT_StatisticsList* stats)
{
    std::string name;
    std::string path;
    std::string description;
    std::string units;

    if (partition->partitionId == 0)
    {
        name = "TotalPacketsEnqueued";
        path = "/stats/queue/PacketsEnqueues";
        description = "Total packets enqueued in all queues";
        units = "packets";
        GetPacketsEnqueued().SetInfo(
            name,
            description,
            units);
        GetPacketsEnqueued().AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&GetPacketsEnqueued());

    if (partition->partitionId == 0)
    {
        name = "TotalPacketsDequeued";
        path = "/stats/queue/PacketsDequeues";
        description = "Total packets dequeued in all queues";
        units = "packets";
        GetPacketsDequeued().SetInfo(
            name,
            description,
            units);
        GetPacketsDequeued().AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&GetPacketsDequeued());

    if (partition->partitionId == 0)
    {
        name = "TotalPacketsDropped";
        path = "/stats/queue/PacketsDropped";
        description = "Total packets dropped in all queues";
        units = "packets";
        GetPacketsDropped().SetInfo(
            name,
            description,
            units);
        GetPacketsDropped().AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&GetPacketsDropped());

    if (partition->partitionId == 0)
    {
        name = "TotalPacketsDroppedForcefully";
        path = "/stats/queue/PacketsDroppedForcefully";
        description = "Total packets forcefully dropped in all queues";
        units = "packets";
        GetPacketsDroppedForcefully().SetInfo(
            name,
            description,
            units);
        GetPacketsDroppedForcefully().AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&GetPacketsDroppedForcefully());

    if (partition->partitionId == 0)
    {
        name = "TotalBytesEnqueued";
        path = "/stats/queue/BytesEnqueues";
        description = "Total bytes enqueued in all queues";
        units = "bytes";
        GetBytesEnqueued().SetInfo(
            name,
            description,
            units);
        GetBytesEnqueued().AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&GetBytesEnqueued());

    if (partition->partitionId == 0)
    {
        name = "TotalBytesDequeued";
        path = "/stats/queue/BytesDequeues";
        description = "Total bytes dequeued in all queues";
        units = "bytes";
        GetBytesDequeued().SetInfo(
            name,
            description,
            units);
        GetBytesDequeued().AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&GetBytesDequeued());

    if (partition->partitionId == 0)
    {
        name = "TotalBytesDropped";
        path = "/stats/queue/BytesDropped";
        description = "Total bytes dropped in all queues";
        units = "bytes";
        GetBytesDropped().SetInfo(
            name,
            description,
            units);
        GetBytesDropped().AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&GetBytesDropped());

    if (partition->partitionId == 0)
    {
        name = "TotalBytesDroppedForcefully";
        path = "/stats/queue/BytesDroppedForcefully";
        description = "Total bytes forcefully dropped in all queues";
        units = "bytes";
        GetBytesDroppedForcefully().SetInfo(
            name,
            description,
            units);
        GetBytesDroppedForcefully().AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&GetBytesDroppedForcefully());
}
