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
#include "stats_mac.h"
#include "stats_global.h"

STAT_MacStatistics::STAT_MacStatistics(Node* node)
{  
    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        m_AddrStats[i].SetLastDelay(-1);
    }
    SetStatNames();
    AddToGlobal(&node->partitionData->stats->global.macAggregate);
}

void STAT_MacStatistics::AddToGlobal(STAT_GlobalMacStatistics* stat)
{
    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        stat->GetDataFramesSent(i).AddStatistic(&GetDataFramesSent(i));
        stat->GetDataFramesReceived(i).AddStatistic(&GetDataFramesReceived(i));
        stat->GetControlFramesSent(i).AddStatistic(&GetControlFramesSent(i));
        stat->GetControlFramesReceived(i).AddStatistic(&GetControlFramesReceived(i));
        stat->GetDataBytesSent(i).AddStatistic(&GetDataBytesSent(i));
        stat->GetDataBytesReceived(i).AddStatistic(&GetDataBytesReceived(i));
        stat->GetControlBytesSent(i).AddStatistic(&GetControlBytesSent(i));
        stat->GetControlBytesReceived(i).AddStatistic(&GetControlBytesReceived(i));
        stat->GetAverageQueuingDelay(i).AddStatistic(&GetAverageQueuingDelay(i));
        stat->GetAverageMediumAccessDelay(i).AddStatistic(&GetAverageMediumAccessDelay(i));
        stat->GetAverageMediumDelay(i).AddStatistic(&GetAverageMediumDelay(i));
        stat->GetAverageJitter(i).AddStatistic(&GetAverageJitter(i));
    }

    stat->GetCarriedLoad().AddStatistic(&m_CarriedLoad);
}

void STAT_MacStatistics::SetStatNames()
{
    std::string name;
    std::string description;
    std::string units;

    // Add application and global application statistics to data points
    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        std::string addrType = STAT_AddrToString(i);

        name = "Mac" + std::string("DataFramesSent") + addrType;
        description = addrType + std::string(" data frames sent to the phy layer");
        units = "frames";
        GetDataFramesSent(i).SetInfo(name, description, units);

        name = "Mac" + std::string("DataFramesReceived") + addrType;
        description = addrType + std::string(" data frames received from phy layer");
        units = "frames";
        GetDataFramesReceived(i).SetInfo(name, description, units);

        name = "Mac" + std::string("ControlFramesSent") + addrType;
        description = addrType + std::string(" control frames sent to the phy layer");
        units = "frames";
        GetControlFramesSent(i).SetInfo(name, description, units);

        name = "Mac" + std::string("ControlFramesReceived") + addrType;
        description = addrType + std::string(" control frames received from phy layer");
        units = "frames";
        GetControlFramesReceived(i).SetInfo(name, description, units);

        name = "Mac" + std::string("DataBytesSent") + addrType;
        description = addrType + std::string(" data bytes sent to the phy layer");
        units = "bytes";
        GetDataBytesSent(i).SetInfo(name, description, units);

        name = "Mac" + std::string("DataBytesReceived") + addrType;
        description = addrType + std::string(" data bytes received from phy layer");
        units = "bytes";
        GetDataBytesReceived(i).SetInfo(name, description, units);

        name = "Mac" + std::string("ControlBytesSent") + addrType;
        description = addrType + std::string(" control bytes sent to the phy layer");
        units = "bytes";
        GetControlBytesSent(i).SetInfo(name, description, units);

        name = "Mac" + std::string("ControlBytesReceived") + addrType;
        description = addrType + std::string(" control bytes received from phy layer");
        units = "bytes";
        GetControlBytesReceived(i).SetInfo(name, description, units);
        
        name = "Mac" + std::string("AverageQueuingDelay") + addrType;
        description = "Average delay for " + addrType + " packets in output queue at the mac layer";
        units = "seconds";
        GetAverageQueuingDelay(i).SetInfo(name, description, units);

        name = "Mac" + std::string("AverageMediumAccessDelay") + addrType;
        description = "Average delay to gain access to medium at the mac layer for " + addrType + " packets";
        units = "seconds";
        GetAverageMediumAccessDelay(i).SetInfo(name, description, units);

        name = "Mac" + std::string("AverageMediumDelay") + addrType;        
        description = "Average medium delay (transmission + propagation) at the mac layer for " + addrType + " packets";
        units = "seconds";
        GetAverageMediumDelay(i).SetInfo(name, description, units);

        name = "Mac" + std::string("AverageJitter") + addrType;
        description = "Average jitter at the mac layer for " + addrType + " packets";
        units = "seconds";
        GetAverageJitter(i).SetInfo(name, description, units);
    }

    name = "MacCarriedLoad";
    description = "Carried load at the mac layer";
    units = "bits/second";
    GetCarriedLoad().SetInfo(name, description, units);
}

       
void STAT_MacStatistics::GetList(std::vector<STAT_Statistic*>& stats)
{
    stats.clear();
    
    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        stats.push_back(&GetDataFramesSent(i));
        stats.push_back(&GetDataFramesReceived(i));
        stats.push_back(&GetControlFramesSent(i));
        stats.push_back(&GetControlFramesReceived(i));
        stats.push_back(&GetDataBytesSent(i));
        stats.push_back(&GetDataBytesReceived(i));
        stats.push_back(&GetControlBytesSent(i));
        stats.push_back(&GetControlBytesReceived(i));

        stats.push_back(&GetAverageQueuingDelay(i));
        stats.push_back(&GetAverageMediumAccessDelay(i));
        stats.push_back(&GetAverageMediumDelay(i));
        stats.push_back(&GetAverageJitter(i));
    }
    stats.push_back(&GetCarriedLoad());
}

void STAT_MacStatistics::AddLeftOutputQueueDataPoints(Node* node,
    Message* msg,
    STAT_DestAddressType type,
    int senderId,
    int receiverId,
    int interfaceIndex)
{
    STAT_MacSessionStatistics* stat = GetSession(node,
        senderId, receiverId, interfaceIndex);

    STAT_Timing* timing = (STAT_Timing*)
        MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsTiming);
    if (timing == NULL)
    {
        MESSAGE_AddInfo(
            node,
            msg,
            sizeof(STAT_Timing),
            INFO_TYPE_StatsTiming);
        timing = (STAT_Timing*) MESSAGE_ReturnInfo(
            msg,
            INFO_TYPE_StatsTiming);
        timing->Initialize();          
    }
    if (timing->sourceNodeId == ANY_NODEID)
    {
        //if uninitialized set it
        timing->sourceNodeId = node->nodeId;
    }
    timing->lastHopNodeId = node->nodeId;
    timing->leftMacOutputQueue = node->getNodeTime();
    if (timing->sourceSentNetworkDown != -1)
    {
        clocktype queuingDelay = timing->leftMacOutputQueue - timing->sourceSentNetworkDown;
        GetAverageQueuingDelay(type).AddDataPoint(timing->leftMacOutputQueue, (Float64) queuingDelay / SECOND);
        stat->GetAverageQueuingDelay(type).AddDataPoint(timing->leftMacOutputQueue, (Float64) queuingDelay / SECOND);        
    }
}

void STAT_MacStatistics::AddFrameSentDataPoints(Node* node,
    Message* msg,
    STAT_DestAddressType type,
    UInt64 controlSize,
    UInt64 dataSize,
    int interfaceIndex,
    int destID)
{    
    STAT_MacSessionStatistics* stat = GetSession(node,
        node->nodeId, destID, interfaceIndex);

    // Add data points
    if (dataSize > 0)
    {
        // Data packet, may have some control
        GetDataFramesSent(type).AddDataPoint(node->getNodeTime(), 1);
        GetDataBytesSent(type).AddDataPoint(node->getNodeTime(), dataSize);
        stat->GetDataFramesSent(type).AddDataPoint(node->getNodeTime(), 1);
        stat->GetDataBytesSent(type).AddDataPoint(node->getNodeTime(), dataSize);

        // TODO: This should be considered overhead
        if (controlSize > 0)
        {
            GetControlBytesSent(type).AddDataPoint(node->getNodeTime(), controlSize);
            stat->GetControlBytesSent(type).AddDataPoint(node->getNodeTime(), controlSize);        
        }
    }
    else
    {
        // Pure control
        GetControlFramesSent(type).AddDataPoint(node->getNodeTime(), 1);
        GetControlBytesSent(type).AddDataPoint(node->getNodeTime(), controlSize);
        stat->GetControlFramesSent(type).AddDataPoint(node->getNodeTime(), 1);
        stat->GetControlBytesSent(type).AddDataPoint(node->getNodeTime(), controlSize);        
    }
        
    STAT_Timing* timing = (STAT_Timing*)
        MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsTiming);
    if (timing == NULL)
    {
        MESSAGE_AddInfo(
            node,
            msg,
            sizeof(STAT_Timing),
            INFO_TYPE_StatsTiming);
        timing = (STAT_Timing*) MESSAGE_ReturnInfo(
            msg,
            INFO_TYPE_StatsTiming);
        timing->Initialize();   
        
    }
    if (timing->sourceNodeId == ANY_NODEID)
    {
        //if uninitialized set it
        timing->sourceNodeId = node->nodeId;
    }
    if (timing->sentMacDown == -1)
    {
        //if uninitialized set it
        timing->sentMacDown = node->getNodeTime();
    }
    if (timing->leftMacOutputQueue != -1)
    {
        clocktype mediumAccessDelay = timing->sentMacDown - timing->leftMacOutputQueue;
        GetAverageMediumAccessDelay(type).AddDataPoint(timing->sentMacDown, (Float64) mediumAccessDelay / SECOND);
        stat->GetAverageMediumAccessDelay(type).AddDataPoint(timing->sentMacDown, (Float64) mediumAccessDelay / SECOND);        
    }
    timing->lastHopNodeId = node->nodeId;
    timing->lastHopIntfIndex = interfaceIndex;
}

void STAT_MacStatistics::AddFrameReceivedDataPoints(Node* node,
    Message* msg,
    STAT_DestAddressType type,
    UInt64 controlSize,
    UInt64 dataSize,
    int interfaceIndex)
{
    // Calculate the delay
    STAT_Timing *timing = (STAT_Timing*)
        MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsTiming);

    ERROR_Assert(
        timing != NULL,
        "No timing info found, AddFrameSentDataPoints was probably not called");

    clocktype delay = node->getNodeTime() - timing->sentMacDown;
    
    NodeId senderId = timing->lastHopNodeId;

    NodeId receiverId = node->nodeId;

    STAT_MacSessionStatistics* stat = GetSession(node,
                                                 senderId,
                                                 receiverId,
                                                 timing->lastHopIntfIndex);

    // Add data points
    if (dataSize > 0)
    {
        GetDataFramesReceived(type).AddDataPoint(node->getNodeTime(), 1);
        GetDataBytesReceived(type).AddDataPoint(node->getNodeTime(), dataSize);
        stat->GetDataFramesReceived(type).AddDataPoint(node->getNodeTime(), 1);
        stat->GetDataBytesReceived(type).AddDataPoint(node->getNodeTime(), dataSize);

        // TODO: This data should be considered overhead
        if (controlSize > 0)
        {
            GetControlBytesReceived(type).AddDataPoint(node->getNodeTime(), controlSize);
            stat->GetControlBytesReceived(type).AddDataPoint(node->getNodeTime(), controlSize);
        }
    }
    else
    {
        GetControlFramesReceived(type).AddDataPoint(node->getNodeTime(), 1);
        GetControlBytesReceived(type).AddDataPoint(node->getNodeTime(), controlSize);
        stat->GetControlFramesReceived(type).AddDataPoint(node->getNodeTime(), 1);
        stat->GetControlBytesReceived(type).AddDataPoint(node->getNodeTime(), controlSize);
    }

    GetAverageMediumDelay(type).AddDataPoint(node->getNodeTime(), (Float64) delay / SECOND);
    stat->GetAverageMediumDelay(type).AddDataPoint(node->getNodeTime(), (Float64) delay / SECOND);
    
    clocktype lastDelay = GetLastDelay(type);
    if (lastDelay != -1)
    {
        clocktype d;

        if (delay > lastDelay)
        {
            d = delay - lastDelay;
        }
        else
        {
            d = lastDelay - delay;
        }
        GetAverageJitter(type).AddDataPoint(node->getNodeTime(), (Float64) d / SECOND);
        stat->GetAverageJitter(type).AddDataPoint(node->getNodeTime(), (Float64) d / SECOND);        
    }
    SetLastDelay(delay, type);
    stat->SetLastDelay(delay, type);
    
    GetCarriedLoad().AddDataPoint(node->getNodeTime(), controlSize + dataSize);
}
void STAT_MacStatistics::AddFrameDroppedSenderDataPoints(
    Node* node,
    int receiverId,
    int interfaceIndex,
    UInt64 bytes)
{
    STAT_MacSessionStatistics* stat = GetSession(node,
        node->nodeId, receiverId, interfaceIndex);

    stat->GetFramesDroppedSender().AddDataPoint(node->getNodeTime(), 1);
    stat->GetBytesDroppedSender().AddDataPoint(node->getNodeTime(), bytes);
}

void STAT_MacStatistics::AddFrameDroppedReceiverDataPoints(
    Node* node,
    Message* msg,
    int interfaceIndex,
    UInt64 bytes,
    BOOL isAnyFrame)
{
    STAT_Timing *timing = (STAT_Timing*)
        MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsTiming);
    ERROR_Assert(
        timing != NULL,
        "No timing info found, AddFrameSentDataPoints was probably not called");

    NodeId receiverId = node->nodeId;
    if (isAnyFrame)
    {
        receiverId = -1;
    }

    STAT_MacSessionStatistics* stat = GetSession(node,
        timing->sourceNodeId, receiverId, interfaceIndex);

    stat->GetFramesDroppedReceiver().AddDataPoint(node->getNodeTime(), 1);
    stat->GetBytesDroppedReceiver().AddDataPoint(node->getNodeTime(), bytes);
}


void STAT_GlobalMacStatistics::Initialize(PartitionData* partition,
    STAT_StatisticsList* stats)
{
    std::string name;
    std::string path;
    std::string description;
    std::string units;

    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        std::string addrType = STAT_AddrToString(i);
        // Set names for all global statistics and add partition based
        // statistics to the global statistics.
        
        if (partition->partitionId == 0)
        {
            name = "MacTotalDataFramesSent" + addrType;
            path = "/stats/mac/dataFramesSent" + addrType;
            description = "Total " + addrType + " data frames sent to phy layer";
            units = "frames";
            GetDataFramesSent(i).SetInfo(
                name,
                description,
                units);

            GetDataFramesSent(i).AddToHierarchy(
                &partition->dynamicHierarchy,
                path);
        }
        stats->RegisterAggregatedStatistic(&GetDataFramesSent(i));

        if (partition->partitionId == 0)
        {
            name = "MacTotalDataFramesReceived" + addrType;
            path = "/stats/mac/dataFramesRecieved" + addrType;
            description = "Total " + addrType + " data frames received from phy layer";
            units = "frames";
            
            GetDataFramesReceived(i).SetInfo(
                name,
                description,
                units);

            GetDataFramesReceived(i).AddToHierarchy(
                &partition->dynamicHierarchy,
                path);
        }
        stats->RegisterAggregatedStatistic(&GetDataFramesReceived(i));

        if (partition->partitionId == 0)
        {
            name = "MacTotalControlFramesSent" + addrType;
            path = "/stats/mac/controlFramesSent" + addrType;
            description = "Total " + addrType + " control frames sent to phy layer";
            units = "frames";
            GetControlFramesSent(i).SetInfo(
                name,
                description,
                units);

            GetControlFramesSent(i).AddToHierarchy(
                &partition->dynamicHierarchy,
                path);
        }
        stats->RegisterAggregatedStatistic(&GetControlFramesSent(i));
        
        if (partition->partitionId == 0)
        {
            name = "MacTotalControlFramesReceived" + addrType;
            path = "/stats/mac/controlFramesRecieved" + addrType;
            description = "Total " + addrType + " control frames received from phy layer";
            units = "frames";
            GetControlFramesReceived(i).SetInfo(
                name,
                description,
                units);

            GetControlFramesReceived(i).AddToHierarchy(
                &partition->dynamicHierarchy,
                path);
        }
        stats->RegisterAggregatedStatistic(&GetControlFramesReceived(i));
        
        if (partition->partitionId == 0)
        {
            name = "MacTotalDataBytesSent" + addrType;
            path = "/stats/mac/dataBytesSent" + addrType;
            description = "Total " + addrType + " data bytes sent to phy layer";
            units = "bytes";
            GetDataBytesSent(i).SetInfo(
                name,
                description,
                units);

            GetDataBytesSent(i).AddToHierarchy(
                &partition->dynamicHierarchy,
                path);
        }
        stats->RegisterAggregatedStatistic(&GetDataBytesSent(i));
        
        if (partition->partitionId == 0)
        {
            name = "MacTotalDataBytesReceived" + addrType;
            path = "/stats/mac/dataBytesReceived" + addrType;
            description = "Total " + addrType + " data bytes received from phy layer";
            units = "bytes";
            GetDataBytesReceived(i).SetInfo(
                name,
                description,
                units);

            GetDataBytesReceived(i).AddToHierarchy(
                &partition->dynamicHierarchy,
                path);
        }
        stats->RegisterAggregatedStatistic(&GetDataBytesReceived(i));
        
        if (partition->partitionId == 0)
        {
            name = "MacTotalControlBytesSent" + addrType;
            path = "/stats/mac/controlBytesSent" + addrType;
            description = "Total " + addrType + " control bytes sent to phy layer";
            units = "bytes";
            GetControlBytesSent(i).SetInfo(
                name,
                description,
                units);

            GetControlBytesSent(i).AddToHierarchy(
                &partition->dynamicHierarchy,
                path);
        }
        stats->RegisterAggregatedStatistic(&GetControlBytesSent(i));

        if (partition->partitionId == 0)
        {
            name = "MacTotalControlBytesReceived" + addrType;
            path = "/stats/mac/controlBytesReceived" + addrType;
            description = "Total " + addrType + " control bytes received from phy layer";
            units = "bytes";
            GetControlBytesReceived(i).SetInfo(
                name,
                description,
                units);

            GetControlBytesReceived(i).AddToHierarchy(
                &partition->dynamicHierarchy,
                path);
        }
        stats->RegisterAggregatedStatistic(&GetControlBytesReceived(i));

        if (partition->partitionId == 0)
        {
            name = "MacAverageQueuingDelay" + addrType;
            path = "/stats/mac/averageQueuingDelay" + addrType;
            description = "Average time from when " + addrType + " packets are sent from network layer to leave mac output queues";
            units = "s";
            GetAverageQueuingDelay(i).SetInfo(
                name,
                description,
                units);

            GetAverageQueuingDelay(i).AddToHierarchy(
                &partition->dynamicHierarchy,
                path);
        }
        stats->RegisterAggregatedStatistic(&GetAverageQueuingDelay(i));

        if (partition->partitionId == 0)
        {
            name = "MacAverageMediumAccessDelay" + addrType;
            path = "/stats/mac/averageMediumAccessDelay" + addrType;
            description = "Average time from when the mac layer sends" + addrType + " packets to phy layer to when they are actually sent by the phy";
            units = "s";
            GetAverageMediumAccessDelay(i).SetInfo(
                name,
                description,
                units);

            GetAverageMediumAccessDelay(i).AddToHierarchy(
                &partition->dynamicHierarchy,
                path);
        }
        stats->RegisterAggregatedStatistic(&GetAverageMediumAccessDelay(i));
        
        if (partition->partitionId == 0)
        {
            name = "MacAverageMediumDelay" + addrType;
            path = "/stats/mac/averageMediumDelay" + addrType;
            description = "Average time from when a "  + addrType + " packet is sent to the mac until it was received at the other side";
            units = "s";
            GetAverageMediumDelay(i).SetInfo(
                name,
                description,
                units);

            GetAverageMediumDelay(i).AddToHierarchy(
                &partition->dynamicHierarchy,
                path);
        }
        stats->RegisterAggregatedStatistic(&GetAverageMediumDelay(i));
        
        if (partition->partitionId == 0)
        {
            name = "MacAverageJitter" + addrType;
            path = "/stats/mac/averageJitter" + addrType;
            description = "Average jitter at the mac layer for " + addrType + " packets";
            units = "s";
            GetAverageJitter(i).SetInfo(
                name,
                description,
                units);

            GetAverageJitter(i).AddToHierarchy(
                &partition->dynamicHierarchy,
                path);
        }
        stats->RegisterAggregatedStatistic(&GetAverageJitter(i));
    }
    if (partition->partitionId == 0)
    {
        name = "MacTotalCarriedLoad";
        path = "/stats/mac/carriedLoad";
        description = "Total carried load at the mac layer";
        units = "bits/s";
        m_CarriedLoad.SetInfo(
            name,
            description,
            units);

        m_CarriedLoad.AddToHierarchy(
            &partition->dynamicHierarchy,
            path);
    }
    stats->RegisterAggregatedStatistic(&m_CarriedLoad);
}
STAT_MacSessionStatistics* STAT_MacStatistics::GetSession(
    Node* node,
    int senderId,
    int receiverId,
    int interfacendex)
{
    // Build the session key
    STAT_MacSessionKey key;
    key.m_senderId = senderId;
    key.m_receiverId = receiverId;
    key.m_interfaceIndex = interfacendex;

    // Look for the session key
    MacSessionIterator it = m_Sessions.find(key);
    if (it == m_Sessions.end())
    {
        // Not found, insert new statistics and return
        STAT_MacSessionStatistics* stat = new STAT_MacSessionStatistics(senderId, receiverId, interfacendex);
        m_Sessions[key] = stat;
        return m_Sessions[key];
    }
    else
    {
        // Found, return statistics
        return it->second;
    }
}

STAT_MacSessionStatistics::STAT_MacSessionStatistics(
        NodeId senderId,
        NodeId receiverId,
        int interfaceIndex)
{
    m_senderId = senderId;
    m_receiverId = receiverId;
    m_interfaceIndex = interfaceIndex;

    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        m_AddrStats[i].SetLastDelay(-1);
    }
}
void STAT_MacSummaryStatistics::InitializeFromModel(
    Node* node,
    STAT_MacSessionStatistics* stats)
{
    clocktype now = node->getNodeTime();
    senderId = stats->GetSender();
    receiverId = stats->GetReceiver();
    interfaceIndex = stats->GetInterfaceIndex();

    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        m_AddrStats[i].totalDataFramesSent = stats->GetDataFramesSent(i).GetValue(now);
        m_AddrStats[i].totalDataFramesReceived = stats->GetDataFramesReceived(i).GetValue(now);
        m_AddrStats[i].totalControlFramesSent = stats->GetControlFramesSent(i).GetValue(now);
        m_AddrStats[i].totalControlFramesReceived = stats->GetControlFramesReceived(i).GetValue(now);
        m_AddrStats[i].totalDataBytesSent = stats->GetDataBytesSent(i).GetValue(now);
        m_AddrStats[i].totalDataBytesReceived = stats->GetDataBytesReceived(i).GetValue(now);
        m_AddrStats[i].totalControlBytesSent = stats->GetControlBytesSent(i).GetValue(now);
        m_AddrStats[i].totalControlBytesReceived = stats->GetControlBytesReceived(i).GetValue(now);

        m_AddrStats[i].averageQueuingDelay = stats->GetAverageQueuingDelay(i).GetValue(now);
        m_AddrStats[i].numQueuingDelayDataPoints = stats->GetAverageQueuingDelay(i).GetNumDataPoints();

        m_AddrStats[i].averageMediumAccessDelay = stats->GetAverageMediumAccessDelay(i).GetValue(now);
        m_AddrStats[i].numMediumAccessDelayDataPoints = stats->GetAverageMediumAccessDelay(i).GetNumDataPoints();

        m_AddrStats[i].averageMediumDelay = stats->GetAverageMediumDelay(i).GetValue(now);
        m_AddrStats[i].numMediumDelayDataPoints = stats->GetAverageMediumDelay(i).GetNumDataPoints();

        m_AddrStats[i].averageJitter = stats->GetAverageJitter(i).GetValue(now);
        m_AddrStats[i].numJitterDataPoints = stats->GetAverageJitter(i).GetNumDataPoints();    
    }

    totalFramesDroppedSender = stats->GetFramesDroppedSender().GetValue(now);
    totalFramesDroppedReceiver = stats->GetFramesDroppedReceiver().GetValue(now);
    totalBytesDroppedSender = stats->GetBytesDroppedSender().GetValue(now);
    totalBytesDroppedReceiver = stats->GetBytesDroppedReceiver().GetValue(now);
}

STAT_MacSummaryStatistics& STAT_MacSummaryStatistics::operator +=(
    const STAT_MacSummaryStatistics& stats)
{    
    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        m_AddrStats[i].totalDataFramesSent += stats.m_AddrStats[i].totalDataFramesSent;
        m_AddrStats[i].totalDataFramesReceived += stats.m_AddrStats[i].totalDataFramesReceived;
        m_AddrStats[i].totalControlFramesSent += stats.m_AddrStats[i].totalControlFramesSent;
        m_AddrStats[i].totalControlFramesReceived += stats.m_AddrStats[i].totalControlFramesReceived;
        m_AddrStats[i].totalDataBytesSent += stats.m_AddrStats[i].totalDataBytesSent;
        m_AddrStats[i].totalDataBytesReceived += stats.m_AddrStats[i].totalDataBytesReceived;
        m_AddrStats[i].totalControlBytesSent += stats.m_AddrStats[i].totalControlBytesSent;
        m_AddrStats[i].totalControlBytesReceived += stats.m_AddrStats[i].totalControlBytesReceived;

        if (m_AddrStats[i].numQueuingDelayDataPoints > 0 || 
            stats.m_AddrStats[i].numQueuingDelayDataPoints > 0)
        {
            m_AddrStats[i].averageQueuingDelay = (m_AddrStats[i].averageQueuingDelay * m_AddrStats[i].numQueuingDelayDataPoints
                + stats.m_AddrStats[i].averageQueuingDelay * stats.m_AddrStats[i].numQueuingDelayDataPoints)
                / (m_AddrStats[i].numQueuingDelayDataPoints + stats.m_AddrStats[i].numQueuingDelayDataPoints);
        
            m_AddrStats[i].numQueuingDelayDataPoints += stats.m_AddrStats[i].numQueuingDelayDataPoints;
        }
        
        if (m_AddrStats[i].numMediumAccessDelayDataPoints > 0 || 
            stats.m_AddrStats[i].numMediumAccessDelayDataPoints > 0)
        {
            m_AddrStats[i].averageMediumAccessDelay = (m_AddrStats[i].averageMediumAccessDelay * m_AddrStats[i].numMediumAccessDelayDataPoints
                + stats.m_AddrStats[i].averageMediumAccessDelay * stats.m_AddrStats[i].numMediumAccessDelayDataPoints)
                / (m_AddrStats[i].numMediumAccessDelayDataPoints + stats.m_AddrStats[i].numMediumAccessDelayDataPoints);
        
            m_AddrStats[i].numMediumAccessDelayDataPoints += stats.m_AddrStats[i].numMediumAccessDelayDataPoints;
        }

        if (m_AddrStats[i].numMediumDelayDataPoints > 0 || 
            stats.m_AddrStats[i].numMediumDelayDataPoints > 0)
        {
            m_AddrStats[i].averageMediumDelay = (m_AddrStats[i].averageMediumDelay * m_AddrStats[i].numMediumDelayDataPoints
                + stats.m_AddrStats[i].averageMediumDelay * stats.m_AddrStats[i].numMediumDelayDataPoints)
                / (m_AddrStats[i].numMediumDelayDataPoints + stats.m_AddrStats[i].numMediumDelayDataPoints);
        
            m_AddrStats[i].numMediumDelayDataPoints += stats.m_AddrStats[i].numMediumDelayDataPoints;
        }

        if (m_AddrStats[i].numJitterDataPoints > 0 || 
            stats.m_AddrStats[i].numJitterDataPoints > 0)
        {
            m_AddrStats[i].averageJitter = (m_AddrStats[i].averageJitter * m_AddrStats[i].numJitterDataPoints
                + stats.m_AddrStats[i].averageJitter * stats.m_AddrStats[i].numJitterDataPoints)
                / (m_AddrStats[i].numJitterDataPoints + stats.m_AddrStats[i].numJitterDataPoints);
        
            m_AddrStats[i].numJitterDataPoints += stats.m_AddrStats[i].numJitterDataPoints;
        }
    }

    totalFramesDroppedSender += stats.totalFramesDroppedSender;
    totalFramesDroppedReceiver += stats.totalFramesDroppedReceiver;
    totalBytesDroppedSender += stats.totalBytesDroppedSender;
    totalBytesDroppedReceiver += stats.totalBytesDroppedReceiver;

    return *this;
}
