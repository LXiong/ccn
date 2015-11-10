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
#include "stats_global.h"
#include "stats_transport.h"

STAT_TransportAddressStatistics::STAT_TransportAddressStatistics()
{
    m_LastDelay = -1;
    m_LastDeliveryDelay = -1;
}

STAT_TransportSessionStatistics::STAT_TransportSessionStatistics(
    Node* node,
    Address sourceAddr,
    Address destAddr,
    int sourcePort,
    int destPort)
{
    m_sourceAddr = sourceAddr;
    m_destAddr = destAddr;
    m_sourcePort = sourcePort;
    m_destPort = destPort;
    
    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        m_AddrStats[i].SetLastDelay(-1);
        m_AddrStats[i].SetLastDeliveryDelay(-1);
    }
}

STAT_TransportSessionStatistics* STAT_TransportStatistics::GetSession(
    Node* node,
    Address sourceAddr,
    Address destAddr,
    int sourcePort,
    int destPort)
{
    STAT_TransportSessionStatistics* stat;
    SessionIter i;

    // Build session key
    STAT_TransportSummarySessionKey session;
    session.m_sourceAddr = sourceAddr;
    session.m_destAddr = destAddr;
    session.m_sourcePort = sourcePort;
    session.m_destPort = destPort;
   
    // Search for session key
    i = m_Session.find(session);
    if (i == m_Session.end())
    {
        // Add it, brand new
        stat = new STAT_TransportSessionStatistics(
            node,
            sourceAddr,
            destAddr,
            sourcePort,
            destPort);
        m_Session[session] = stat;
    }
    else
    {
        // Already exists
        stat = i->second;
    }

    return stat;
}

STAT_TransportStatistics::STAT_TransportStatistics(Node* node, const std::string& protocol) : m_Protocol(protocol)
{
    SetStatNames();
    AddToGlobal(&node->partitionData->stats->global.transportAggregate);
}

void STAT_TransportStatistics::SetStatNames()
{
    std::string name;
    std::string description;
    std::string units;

    // Add application and global application statistics to data points
    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        name = "Transport" + STAT_AddrToString(i) + std::string("DataSegmentsSent");
        description = STAT_AddrToString(i) + std::string(" data segments sent from the transport layer");
        units = "segments";
        GetDataSegmentsSent(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("DataSegmentsReceived");
        description = STAT_AddrToString(i) + std::string(" data segments received at the transport layer");
        units = "segments";
        GetDataSegmentsReceived(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("DataBytesSent");
        description = STAT_AddrToString(i) + std::string(" data bytes sent from the transport layer");
        units = "bytes";
        GetDataBytesSent(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("DataBytesReceived");
        description = STAT_AddrToString(i) + std::string(" data bytes received at the transport layer");
        units = "bytes";
        GetDataBytesReceived(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("OverheadBytesSent");
        description = STAT_AddrToString(i) + std::string(" overhead bytes sent from the transport layer");
        units = "bytes";
        GetOverheadBytesSent(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("OverheadBytesReceived");
        description = STAT_AddrToString(i) + std::string(" overhead bytes received at the transport layer");
        units = "bytes";
        GetOverheadBytesReceived(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("ControlSegmentsSent");
        description = STAT_AddrToString(i) + std::string(" control segments sent from the transport layer");
        units = "segments";
        GetControlSegmentsSent(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("ControlSegmentsReceived");
        description = STAT_AddrToString(i) + std::string(" control segments received at the transport layer");
        units = "segments";
        GetControlSegmentsReceived(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("ControlBytesSent");
        description = STAT_AddrToString(i) + std::string(" control bytes sent from the transport layer");
        units = "bytes";
        GetControlBytesSent(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("ControlBytesReceived");
        description = STAT_AddrToString(i) + std::string(" control bytes received at the transport layer");
        units = "bytes";
        GetControlBytesReceived(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("OfferedLoad");
        description = STAT_AddrToString(i) + std::string(" offered load at the transport layer");
        units = "bits/second";
        GetOfferedLoad(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("Throughput");
        description = STAT_AddrToString(i) + std::string(" throughput at the transport layer");
        units = "bits/second";
        GetThroughput(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("Goodput");
        description = STAT_AddrToString(i) + std::string(" goodput at the transport layer");
        units = "bits/second";
        GetGoodput(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("AverageDelay");
        description = STAT_AddrToString(i) + std::string(" average delay at the transport layer");
        units = "seconds";
        GetAverageDelay(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("AverageDeliveryDelay");
        description = STAT_AddrToString(i) + std::string(" average delivery delay at the transport layer");
        units = "seconds";
        GetAverageDeliveryDelay(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("AverageJitter");
        description = STAT_AddrToString(i) + std::string(" average jitter at the transport layer");
        units = "seconds";
        GetAverageJitter(i).SetInfo(name, description, units);

        name = "Transport" + STAT_AddrToString(i) + std::string("AverageDeliveryJitter");
        description = STAT_AddrToString(i) + std::string(" average delivery jitter at the transport layer");
        units = "seconds";
        GetAverageDeliveryJitter(i).SetInfo(name, description, units);
    }
}
        
void STAT_TransportStatistics::GetList(std::vector<STAT_Statistic*>& stats)
{
    stats.clear();

    std::list<STAT_DestAddressType> types;
    STAT_DestAddressType type;
    std::list<STAT_DestAddressType>::iterator it;

    // Include unicast, broadcast and multicast for UDP
    // Include unicast for anything else
    if (m_Protocol == "udp")
    {
        types.push_back(STAT_Unicast);
        types.push_back(STAT_Broadcast);
        types.push_back(STAT_Multicast);
    }
    else
    {
        types.push_back(STAT_Unicast);
    }

    // Get stats based on address types and sender/receiver
    for (it = types.begin(); it != types.end(); ++it)
    {
        type = *it;

        stats.push_back(&GetDataSegmentsSent(type));
        stats.push_back(&GetDataSegmentsReceived(type));
        stats.push_back(&GetDataBytesSent(type));
        stats.push_back(&GetDataBytesReceived(type));
        stats.push_back(&GetOverheadBytesSent(type));
        stats.push_back(&GetOverheadBytesReceived(type));
        stats.push_back(&GetControlSegmentsSent(type));
        stats.push_back(&GetControlSegmentsReceived(type));
        stats.push_back(&GetControlBytesSent(type));
        stats.push_back(&GetControlBytesReceived(type));
        stats.push_back(&GetOfferedLoad(type));
        stats.push_back(&GetThroughput(type));
        stats.push_back(&GetGoodput(type));
        stats.push_back(&GetAverageDelay(type));
        stats.push_back(&GetAverageDeliveryDelay(type));
        stats.push_back(&GetAverageJitter(type));
        stats.push_back(&GetAverageDeliveryJitter(type));
    }
}

void STAT_TransportStatistics::AddToGlobal(STAT_GlobalTransportStatistics* stat)
{
    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        stat->GetDataSegmentsSent(i).AddStatistic(&GetDataSegmentsSent(i));
        stat->GetDataSegmentsReceived(i).AddStatistic(&GetDataSegmentsReceived(i));
        stat->GetDataBytesSent(i).AddStatistic(&GetDataBytesSent(i));
        stat->GetDataBytesReceived(i).AddStatistic(&GetDataBytesReceived(i));
        stat->GetOverheadBytesSent(i).AddStatistic(&GetOverheadBytesSent(i));
        stat->GetOverheadBytesReceived(i).AddStatistic(&GetOverheadBytesReceived(i));
        stat->GetControlSegmentsSent(i).AddStatistic(&GetControlSegmentsSent(i));
        stat->GetControlSegmentsReceived(i).AddStatistic(&GetControlSegmentsReceived(i));
        stat->GetControlBytesSent(i).AddStatistic(&GetControlBytesSent(i));
        stat->GetControlBytesReceived(i).AddStatistic(&GetControlBytesReceived(i));

        stat->GetOfferedLoad(i).AddStatistic(&GetOfferedLoad(i));
        stat->GetThroughput(i).AddStatistic(&GetThroughput(i));
        stat->GetGoodput(i).AddStatistic(&GetGoodput(i));
        stat->GetAverageDelay(i).AddStatistic(&GetAverageDelay(i));
        stat->GetAverageDeliveryDelay(i).AddStatistic(&GetAverageDeliveryDelay(i));
        stat->GetAverageJitter(i).AddStatistic(&GetAverageJitter(i));
        stat->GetAverageDeliveryJitter(i).AddStatistic(&GetAverageDeliveryJitter(i));
    }
}

void STAT_TransportStatistics::AddReceiveFromUpperLayerDataPoints(Node* node, Message* msg)
{
    STAT_Timing* timing = STAT_GetTiming(node, msg);
    timing->receivedTransportUp = node->getNodeTime();
}

void STAT_TransportStatistics::AddSegmentSentDataPoints(
    Node* node,
    Message* msg,
    UInt64 controlSize,
    UInt64 dataSize,
    UInt64 overheadSize,
    const Address& sourceAddr,
    const Address& destAddr,
    int sourcePort,
    int destPort)
{
    // Sanity check, no control and data
    ERROR_Assert(
        (controlSize == 0 && (dataSize + overheadSize > 0)
        || (controlSize > 0 && (dataSize + overheadSize == 0))),
        "Must be either data or control");

    // Mark timing info for when we sent from the transport layer
    // sourceNodeId is -1 if the packet originated at this layer (control)
    STAT_Timing* timing = STAT_GetTiming(node, msg);
    timing->sentTransportDown = node->getNodeTime();
    if (timing->sourceNodeId == ANY_NODEID)
    {
        timing->sourceNodeId = node->nodeId;
    }

    // Fill in session statistics
    STAT_TransportSessionStatistics* stat = GetSession(
        node,
        sourceAddr,
        destAddr,
        sourcePort,
        destPort);

    // Fill in statistics for this address type
    STAT_DestAddressType type = STAT_AddressToDestAddressType(node, destAddr);
    STAT_TransportAddressStatistics* addr = &m_AddrStats[type];
    if (dataSize > 0)
    {
        addr->m_DataSegmentsSent.AddDataPoint(timing->sentTransportDown, 1);
        addr->m_DataBytesSent.AddDataPoint(timing->sentTransportDown, dataSize);
        stat->GetDataSegmentsSent(type).AddDataPoint(timing->sentTransportDown, 1);
        stat->GetDataBytesSent(type).AddDataPoint(timing->sentTransportDown, dataSize);
    }
    if (overheadSize > 0)
    {
        addr->m_OverheadBytesSent.AddDataPoint(timing->sentTransportDown, overheadSize);
        stat->GetOverheadBytesSent(type).AddDataPoint(timing->sentTransportDown, overheadSize);
    }
    else
    {
        addr->m_ControlSegmentsSent.AddDataPoint(timing->sentTransportDown, 1);
        addr->m_ControlBytesSent.AddDataPoint(timing->sentTransportDown, controlSize);
        stat->GetControlSegmentsSent(type).AddDataPoint(timing->sentTransportDown, 1);
        stat->GetControlBytesSent(type).AddDataPoint(timing->sentTransportDown, controlSize);
    }
    addr->m_OfferedLoad.AddDataPoint(
        node->getNodeTime(),
        dataSize + overheadSize + controlSize);
    
    stat->GetOfferedLoad(type).AddDataPoint(
        node->getNodeTime(),
        dataSize + overheadSize + controlSize);
}

void STAT_TransportStatistics::AddSegmentReceivedDataPoints(
    Node* node,
    Message* msg,
    STAT_DestAddressType type,
    UInt64 controlSize,
    UInt64 dataSize,
    UInt64 overheadSize,
    const Address& sourceAddr,
    const Address& destAddr,
    int sourcePort,
    int destPort)
{
#ifdef EXATA
    if (msg->isEmulationPacket)
    {
        // Is emulated packet
        return;
    }
#endif
    // Sanity check, no control and data
    ERROR_Assert(
        (controlSize == 0 && (dataSize + overheadSize > 0)
        || (controlSize > 0 && (dataSize + overheadSize == 0))),
        "Must be either data or control");
    ERROR_Assert(
        type >= STAT_Unicast && type <= STAT_Multicast,
        "Bad address type");

    // Verify that timing info is present and that we have sentTransportDown
    STAT_Timing *timing = (STAT_Timing*)
        MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsTiming);
    if (timing == NULL)
    {
        ERROR_ReportWarning("No timing info found, AddFrameSentDataPoints was probably not called");
        return;
    }
    if (timing->sentTransportDown == -1)
    {
        ERROR_ReportWarning("Packet was never sent down from transport");
        return;
    }

    // Fill in statistics
    STAT_TransportAddressStatistics* addr = &m_AddrStats[type];
    STAT_TransportSessionStatistics* stat = GetSession(
        node,
        sourceAddr,
        destAddr,
        sourcePort,
        destPort);

    if (overheadSize > 0)
    {
        addr->m_OverheadBytesReceived.AddDataPoint(node->getNodeTime(), overheadSize);
        stat->GetOverheadBytesReceived(type).AddDataPoint(node->getNodeTime(), overheadSize);
    }
    if (dataSize > 0)
    {
        addr->m_DataSegmentsReceived.AddDataPoint(node->getNodeTime(), 1);
        addr->m_DataBytesReceived.AddDataPoint(node->getNodeTime(), dataSize);
        stat->GetDataSegmentsReceived(type).AddDataPoint(node->getNodeTime(), 1);
        stat->GetDataBytesReceived(type).AddDataPoint(node->getNodeTime(), dataSize);
    }
    else
    {
        addr->m_ControlSegmentsReceived.AddDataPoint(node->getNodeTime(), 1);
        addr->m_ControlBytesReceived.AddDataPoint(node->getNodeTime(), controlSize);
        stat->GetControlSegmentsReceived(type).AddDataPoint(node->getNodeTime(), 1);
        stat->GetControlBytesReceived(type).AddDataPoint(node->getNodeTime(), controlSize);
    }

    clocktype delay = node->getNodeTime() - timing->sentTransportDown;

    addr->m_AverageDelay.AddDataPoint(
        node->getNodeTime(), (double) delay / SECOND);
    stat->GetAverageDelay(type).AddDataPoint(
        node->getNodeTime(), (double) delay / SECOND);
    
    // Calculate/update jitter based on addr stats
    if (addr->m_LastDelay != -1)
    {
        clocktype diffDelay;

        if (delay > addr->m_LastDelay)
        {
            diffDelay = delay - addr->m_LastDelay;
        }
        else
        {
            diffDelay = addr->m_LastDelay - delay;
        }
        addr->m_AverageJitter.AddDataPoint(node->getNodeTime(), (Float64) diffDelay / SECOND);
    }
    addr->m_LastDelay = delay;

    addr->m_Throughput.AddDataPoint(
        node->getNodeTime(),
        dataSize + overheadSize + controlSize);
    stat->GetThroughput(type).AddDataPoint(
        node->getNodeTime(),
        dataSize + overheadSize + controlSize);

    // Calculate/update jitter based on session stats
    if (stat->GetLastDelay(type) != -1)
    {
        clocktype diffDelay;

        if (delay > stat->GetLastDelay(type))
        {
            diffDelay = delay - stat->GetLastDelay(type);
        }
        else
        {
            diffDelay = stat->GetLastDelay(type) - delay;
        }
        stat->GetAverageJitter(type).AddDataPoint(node->getNodeTime(), (Float64) diffDelay / SECOND);
    }
    stat->SetLastDelay(type, delay);
}

void STAT_TransportStatistics::AddSentToUpperLayerDataPoints(
    Node* node,
    Message* msg,
    const Address& sourceAddr,
    const Address& destAddr,
    int sourcePort,
    int destPort)
{
#ifdef EXATA
    if (msg->isEmulationPacket)
    {
        // Is emulated packet
        return;
    }
#endif
    // Verify that we have timing info
    STAT_Timing *timing = (STAT_Timing*)
        MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsTiming);
    if (timing == NULL)
    {
        ERROR_ReportWarning("No timing info found, AddFrameSentDataPoints was probably not called");
        return;
    }
    if (timing->receivedTransportUp == -1)
    {
        ERROR_ReportWarning("Transport packet was never received from upper layer");
        return;
    }

    // Update delays
    STAT_DestAddressType type = STAT_AddressToDestAddressType(node, destAddr);
    STAT_TransportAddressStatistics* addr = &m_AddrStats[type];
    
    STAT_TransportSessionStatistics* stat = 
        GetSession(node, 
        sourceAddr,
        destAddr,
        sourcePort,
        destPort);

    addr->m_Goodput.AddDataPoint(node->getNodeTime(), MESSAGE_ReturnPacketSize(msg));
    stat->GetGoodput(type).AddDataPoint(
        node->getNodeTime(),
        MESSAGE_ReturnPacketSize(msg));
    
    clocktype delay = node->getNodeTime() - timing->receivedTransportUp;
    addr->m_AverageDeliveryDelay.AddDataPoint(node->getNodeTime(), (Float64) delay / SECOND);        

    // Calculate/update jitter based on address stats
    if (addr->m_LastDeliveryDelay != -1)
    {
        clocktype diffDelay;

        if (delay > addr->m_LastDeliveryDelay)
        {
            diffDelay = delay - addr->m_LastDeliveryDelay;
        }
        else
        {
            diffDelay = addr->m_LastDeliveryDelay - delay;
        }

        addr->m_AverageDeliveryJitter.AddDataPoint(node->getNodeTime(), (Float64) diffDelay / SECOND);
    }
    addr->m_LastDeliveryDelay = delay;
}

void STAT_GlobalTransportStatistics::Initialize(PartitionData* partition, STAT_StatisticsList* stats)
{
    std::string name;
    std::string path;
    std::string description;
    std::string units;

    // Set names and meta-data for all global statistics

    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        if (partition->partitionId == 0)
        {
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "DataSegmentsSent";
            description = std::string("Total ") + STAT_AddrToString(i) +  " data segments sent from transport layer";
            units = "segments";
            GetDataSegmentsSent(i).SetInfo(
                path,
                description,
                units);
            GetDataSegmentsSent(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetDataSegmentsSent(i));

        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "DataSegmentsReceived";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "DataSegmentsReceived";
            description = std::string("Total ") + STAT_AddrToString(i) + " data segments received at the transport layer";
            units = "segments";
            GetDataSegmentsReceived(i).SetInfo(
                name,
                description,
                units);
            GetDataSegmentsReceived(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetDataSegmentsReceived(i));

        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "DataBytesSent";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "DataBytesSent";
            description = std::string("Total ") + STAT_AddrToString(i) + " data bytes sent from the transport layer";
            units = "bytes";
            GetDataBytesSent(i).SetInfo(
                name,
                description,
                units);
            GetDataBytesSent(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetDataBytesSent(i));
        
        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "DataBytesReceived";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "DataBytesReceived";
            description = std::string("Total ") + STAT_AddrToString(i) + " data bytes received at the transport layer";
            units = "bytes";
            GetDataBytesReceived(i).SetInfo(
                name,
                description,
                units);
            GetDataBytesReceived(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetDataBytesReceived(i));
        
        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "OverheadBytesSent";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "OverheadBytesSent";
            description = std::string("Total ") + STAT_AddrToString(i) + " overhead bytes sent from the transport layer";
            units = "bytes";
            GetOverheadBytesSent(i).SetInfo(
                name,
                description,
                units);
            GetOverheadBytesSent(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetOverheadBytesSent(i));
        
        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "OverheadBytesReceived";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "OverheadBytesReceived";
            description = std::string("Total ") + STAT_AddrToString(i) + " overhead bytes received at the transport layer";
            units = "bytes";
            GetOverheadBytesReceived(i).SetInfo(
                name,
                description,
                units);
            GetOverheadBytesReceived(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetOverheadBytesReceived(i));

        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "ControlSegmentsSent";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "ControlSegmentsSent";
            description = std::string("Total ") + STAT_AddrToString(i) + " control segments sent from transport layer";
            units = "segments";
            GetControlSegmentsSent(i).SetInfo(
                name,
                description,
                units);
            GetControlSegmentsSent(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetControlSegmentsSent(i));

        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "ControlSegmentsReceived";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "ControlSegmentsReceived";
            description = std::string("Total ") + STAT_AddrToString(i) + " control segments received at the transport layer";
            units = "segments";
            GetControlSegmentsReceived(i).SetInfo(
                name,
                description,
                units);
            GetControlSegmentsReceived(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetControlSegmentsReceived(i));

        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "ControlBytesSent";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "ControlBytesSent";
            description = std::string("Total ") + STAT_AddrToString(i) + " control bytes sent from the transport layer";
            units = "bytes";
            GetControlBytesSent(i).SetInfo(
                name,
                description,
                units);
            GetControlBytesSent(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetControlBytesSent(i));
        
        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "ControlBytesReceived";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "ControlBytesReceived";
            description = std::string("Total ") + STAT_AddrToString(i) + " control bytes received at the transport layer";
            units = "bytes";
            GetControlBytesReceived(i).SetInfo(
                name,
                description,
                units);
            GetControlBytesReceived(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetControlBytesReceived(i));



        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "OfferedLoad";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "Offeredload";
            description = std::string("Total ") + STAT_AddrToString(i) + " offered load at the transport layer (data sent down)";
            units = "bits/s";
            GetOfferedLoad(i).SetInfo(
                name,
                description,
                units);
            GetOfferedLoad(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetOfferedLoad(i));
        
        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "Throughput";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "Throughput";
            description = std::string("Total ") + STAT_AddrToString(i) + " throughput at the transport layer (data received down)";
            units = "bits/s";
            GetThroughput(i).SetInfo(
                name,
                description,
                units);
            GetThroughput(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetThroughput(i));

        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "Goodput";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "Goodput";
            description = std::string("Total ") + STAT_AddrToString(i) + " goodput at the transport layer (data sent up)";
            units = "bits/s";
            GetGoodput(i).SetInfo(
                name,
                description,
                units);
            GetGoodput(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetGoodput(i));
        
        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "AverageDelay";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "AverageDelay";
            description = std::string("Average ") + STAT_AddrToString(i) + " end-to-end delay at the transport layer";
            units = "s";
            GetAverageDelay(i).SetInfo(
                name,
                description,
                units);
            GetAverageDelay(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetAverageDelay(i));

        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "AverageJitter";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "AverageJitter";
            description = std::string("Average ") + STAT_AddrToString(i) + " jitter of all packets at the transport layer";
            units = "s";
            GetAverageJitter(i).SetInfo(
                name,
                description,
                units);
            GetAverageJitter(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetAverageJitter(i));

        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "AverageDeliveryDelay";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "AverageDeliveryDelay";
            description = std::string("Average ") + STAT_AddrToString(i) + " end-to-end delay for segments delivered below the transport layer";
            units = "s";
            GetAverageDeliveryDelay(i).SetInfo(
                name,
                description,
                units);
            GetAverageDeliveryDelay(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetAverageDeliveryDelay(i));

        if (partition->partitionId == 0)
        {
            name = "Transport" + STAT_AddrToString(i) + "AverageDeliveryDelay";
            path = std::string("/stats/transport/") + STAT_AddrToString(i) + "AverageDeliveryJitter";
            description = std::string("Average ") + STAT_AddrToString(i) + " jitter for segments delivered below the transport layer";
            units = "s";
            GetAverageDeliveryJitter(i).SetInfo(
                name,
                description,
                units);
            GetAverageDeliveryJitter(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetAverageDeliveryJitter(i));
    }
}

void STAT_TransportSummaryStatistics::InitializeFromModel(
    Node* node,
    STAT_TransportSessionStatistics* stats)
{
    clocktype now = node->getNodeTime();

    sourceAddr = stats->GetSourceAddr();
    destAddr = stats->GetDestAddr();
    sourcePort = stats->GetSourcePort();
    destPort = stats->GetDestPort();

    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        m_AddrStats[i].totalDataSegmentsSent = stats->GetDataSegmentsSent(i).GetValue(now);
        m_AddrStats[i].totalDataSegmentsReceived = stats->GetDataSegmentsReceived(i).GetValue(now);
        m_AddrStats[i].totalDataBytesSent = stats->GetDataBytesSent(i).GetValue(now);
        m_AddrStats[i].totalDataBytesReceived = stats->GetDataBytesReceived(i).GetValue(now);
        m_AddrStats[i].totalOverheadBytesSent = stats->GetOverheadBytesSent(i).GetValue(now);
        m_AddrStats[i].totalOverheadBytesReceived = stats->GetOverheadBytesReceived(i).GetValue(now);
        m_AddrStats[i].totalControlSegmentsSent = stats->GetControlSegmentsSent(i).GetValue(now);
        m_AddrStats[i].totalControlSegmentsReceived = stats->GetControlSegmentsReceived(i).GetValue(now);
        m_AddrStats[i].totalControlBytesSent = stats->GetControlBytesSent(i).GetValue(now);
        m_AddrStats[i].totalControlBytesReceived = stats->GetControlBytesReceived(i).GetValue(now);

        m_AddrStats[i].offeredLoad = stats->GetOfferedLoad(i).GetValue(now);
        m_AddrStats[i].throughput = stats->GetThroughput(i).GetValue(now);
        m_AddrStats[i].goodput = stats->GetGoodput(i).GetValue(now);

        m_AddrStats[i].averageDelay = stats->GetAverageDelay(i).GetValue(now);
        m_AddrStats[i].numDelayDataPoints = stats->GetAverageDelay(i).GetNumDataPoints();
        m_AddrStats[i].averageJitter = stats->GetAverageJitter(i).GetValue(now);
        m_AddrStats[i].numJitterDataPoints = stats->GetAverageJitter(i).GetNumDataPoints();
    }
}

STAT_TransportSummaryStatistics& STAT_TransportSummaryStatistics::operator +=(
    const STAT_TransportSummaryStatistics& stats)
{
    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        m_AddrStats[i].totalDataSegmentsSent += stats. m_AddrStats[i].totalDataSegmentsSent;
        m_AddrStats[i].totalDataSegmentsReceived += stats. m_AddrStats[i].totalDataSegmentsReceived;
        m_AddrStats[i].totalDataBytesSent += stats. m_AddrStats[i].totalDataBytesSent;
        m_AddrStats[i].totalDataBytesReceived += stats. m_AddrStats[i].totalDataBytesReceived;
        m_AddrStats[i].totalOverheadBytesSent += stats. m_AddrStats[i].totalOverheadBytesSent;
        m_AddrStats[i].totalOverheadBytesReceived += stats. m_AddrStats[i].totalOverheadBytesReceived;
        m_AddrStats[i].totalControlSegmentsSent += stats. m_AddrStats[i].totalControlSegmentsSent;
        m_AddrStats[i].totalControlSegmentsReceived += stats. m_AddrStats[i].totalControlSegmentsReceived;
        m_AddrStats[i].totalControlBytesSent += stats. m_AddrStats[i].totalControlBytesSent;
        m_AddrStats[i].totalControlBytesReceived += stats. m_AddrStats[i].totalControlBytesReceived;

        m_AddrStats[i].offeredLoad += stats. m_AddrStats[i].offeredLoad;
        m_AddrStats[i].throughput += stats. m_AddrStats[i].throughput;
        m_AddrStats[i].goodput += stats. m_AddrStats[i].goodput;

        if (stats.m_AddrStats[i].numDelayDataPoints > 0 ||
            m_AddrStats[i].numDelayDataPoints > 0)
        {
            m_AddrStats[i].averageDelay = (m_AddrStats[i].averageDelay * m_AddrStats[i].numDelayDataPoints
                + stats.m_AddrStats[i].averageDelay * stats.m_AddrStats[i].numDelayDataPoints)
                / (m_AddrStats[i].numDelayDataPoints + stats.m_AddrStats[i].numDelayDataPoints);

            m_AddrStats[i].numDelayDataPoints += stats.m_AddrStats[i].numDelayDataPoints;
        }
        
        if (stats.m_AddrStats[i].numJitterDataPoints > 0 ||
            m_AddrStats[i].numJitterDataPoints > 0)
        {
            m_AddrStats[i].averageJitter = (m_AddrStats[i].averageJitter * m_AddrStats[i].numJitterDataPoints
                + stats.m_AddrStats[i].averageJitter * stats.m_AddrStats[i].numJitterDataPoints)
                / (m_AddrStats[i].numJitterDataPoints + stats.m_AddrStats[i].numJitterDataPoints);

            m_AddrStats[i].numJitterDataPoints += stats.m_AddrStats[i].numJitterDataPoints;
        }
    }
    return *this;
}
