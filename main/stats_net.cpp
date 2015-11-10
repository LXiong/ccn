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
#include "stats_net.h"
#ifdef ADDON_DB
#include "db.h"
#endif
#include "network_ip.h"

STAT_NetStatistics::STAT_NetStatistics(Node* node)
{

    m_LastDelay = (clocktype*) MEM_malloc(sizeof(clocktype) * node->numberInterfaces);
    m_LastUnicastDelay = (clocktype*) MEM_malloc(sizeof(clocktype) * node->numberInterfaces);
    m_LastBroadcastDelay = (clocktype*) MEM_malloc(sizeof(clocktype) * node->numberInterfaces);
    m_LastMulticastDelay = (clocktype*) MEM_malloc(sizeof(clocktype) * node->numberInterfaces);
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        m_LastDelay[i] = -1;
        // For New Stat APIs
        m_LastUnicastDelay[i] = -1;
        m_LastBroadcastDelay[i] = -1;
        m_LastMulticastDelay[i] = -1;
    }

    SetStatNames();
    AddToGlobal(&node->partitionData->stats->global.netAggregate);
}

void STAT_NetStatistics::GetList(std::vector<STAT_Statistic*>& stats)
{
    stats.clear();

    stats.push_back(&m_DataPacketsSentUnicast);
    stats.push_back(&m_DataPacketsReceivedUnicast);
    stats.push_back(&m_DataPacketsForwardedUnicast);
    stats.push_back(&m_ControlPacketsSentUnicast);
    stats.push_back(&m_ControlPacketsReceivedUnicast);
    stats.push_back(&m_ControlPacketsForwardedUnicast);
    stats.push_back(&m_DataBytesSentUnicast);
    stats.push_back(&m_DataBytesReceivedUnicast);
    stats.push_back(&m_DataBytesForwardedUnicast);
    stats.push_back(&m_ControlBytesSentUnicast);
    stats.push_back(&m_ControlBytesReceivedUnicast);
    stats.push_back(&m_ControlBytesForwardedUnicast);

    stats.push_back(&m_DataPacketsSentBroadcast);
    stats.push_back(&m_DataPacketsReceivedBroadcast);
    stats.push_back(&m_DataPacketsForwardedBroadcast);
    stats.push_back(&m_ControlPacketsSentBroadcast);
    stats.push_back(&m_ControlPacketsReceivedBroadcast);
    stats.push_back(&m_ControlPacketsForwardedBroadcast);
    stats.push_back(&m_DataBytesSentBroadcast);
    stats.push_back(&m_DataBytesReceivedBroadcast);
    stats.push_back(&m_DataBytesForwardedBroadcast);
    stats.push_back(&m_ControlBytesSentBroadcast);
    stats.push_back(&m_ControlBytesReceivedBroadcast);
    stats.push_back(&m_ControlBytesForwardedBroadcast);

    stats.push_back(&m_DataPacketsSentMulticast);
    stats.push_back(&m_DataPacketsReceivedMulticast);
    stats.push_back(&m_DataPacketsForwardedMulticast);
    stats.push_back(&m_ControlPacketsSentMulticast);
    stats.push_back(&m_ControlPacketsReceivedMulticast);
    stats.push_back(&m_ControlPacketsForwardedMulticast);
    stats.push_back(&m_DataBytesSentMulticast);
    stats.push_back(&m_DataBytesReceivedMulticast);
    stats.push_back(&m_DataBytesForwardedMulticast);
    stats.push_back(&m_ControlBytesSentMulticast);
    stats.push_back(&m_ControlBytesReceivedMulticast);
    stats.push_back(&m_ControlBytesForwardedMulticast);

    stats.push_back(&m_CarriedLoad);
    stats.push_back(&m_PacketsDroppedNoRoute);
    stats.push_back(&m_PacketsDroppedTtlExpired);
    stats.push_back(&m_PacketsDroppedQueueOverflow);
    stats.push_back(&m_PacketsDroppedOther);
    stats.push_back(&m_AverageDelay);
    stats.push_back(&m_AverageJitter);
    stats.push_back(&m_AverageDataHopCount);
    stats.push_back(&m_AverageControlHopCount);

    // Added new Stats APIs required for Network Aggregate Table
    stats.push_back(&m_CarriedLoadUnicast);
    stats.push_back(&m_CarriedLoadMulticast);
    stats.push_back(&m_CarriedLoadBroadcast);
    stats.push_back(&m_PacketsDroppedNoRouteUnicast);
    stats.push_back(&m_PacketsDroppedNoRouteMulticast);
    stats.push_back(&m_AverageDelayUnicast);
    stats.push_back(&m_AverageDelayMulticast);
    stats.push_back(&m_AverageDelayBroadcast);
    stats.push_back(&m_AverageJitterUnicast);
    stats.push_back(&m_AverageJitterMulticast);
    stats.push_back(&m_AverageJitterBroadcast);
    stats.push_back(&m_OriginatedCarriedLoad);
    stats.push_back(&m_ForwardedCarriedLoad);
}

void STAT_NetStatistics::SetStatNames()
{
    std::string name;
    std::string description;
    std::string units;

    name = "DataPacketsSentUnicast";
    description = "Data packets sent unicast";
    units = "packets";
    m_DataPacketsSentUnicast.SetInfo(name, description, units);

    name = "DataPacketsReceivedUnicast";
    description = "Data packets received unicast";
    units = "packets";
    m_DataPacketsReceivedUnicast.SetInfo(name, description, units);

    name = "DataPacketsForwardedUnicast";
    description = "Data packets forwarded unicast";
    units = "packets";
    m_DataPacketsForwardedUnicast.SetInfo(name, description, units);

    name = "ControlPacketsSentUnicast";
    description = "Control packets sent unicast";
    units = "packets";
    m_ControlPacketsSentUnicast.SetInfo(name, description, units);

    name = "ControlPacketsReceivedUnicast";
    description = "Control packets received unicast";
    units = "packets";
    m_ControlPacketsReceivedUnicast.SetInfo(name, description, units);

    name = "ControlPacketsForwardedUnicast";
    description = "Control packets forwarded unicast";
    units = "packets";
    m_ControlPacketsForwardedUnicast.SetInfo(name, description, units);

    name = "DataBytesSentUnicast";
    description = "Data bytes sent unicast";
    units = "bytes";
    m_DataBytesSentUnicast.SetInfo(name, description, units);

    name = "DataBytesReceivedUnicast";
    description = "Data bytes received unicast";
    units = "bytes";
    m_DataBytesReceivedUnicast.SetInfo(name, description, units);

    name = "DataBytesForwardedUnicast";
    description = "Data bytes forwarded unicast";
    units = "bytes";
    m_DataBytesForwardedUnicast.SetInfo(name, description, units);

    // Corrected the wrong Stat name
    name = "ControlBytesSentUnicast";
    description = "Control bytes sent unicast";
    units = "bytes";
    m_ControlBytesSentUnicast.SetInfo(name, description, units);

    name = "ControlBytesReceivedUnicast";
    description = "Control bytes receved unicast";
    units = "bytes";
    m_ControlBytesReceivedUnicast.SetInfo(name, description, units);

    name = "ControlBytesForwardedUnicast";
    description = "Control bytes forwarded unicast";
    units = "bytes";
    m_ControlBytesForwardedUnicast.SetInfo(name, description, units);



    name = "DataPacketsSentBroadcast";
    description = "Data packet sent broadcast";
    units = "packets";
    m_DataPacketsSentBroadcast.SetInfo(name, description, units);

    name = "DataPacketsReceivedBroadcast";
    description = "Data packets received broadcast";
    units = "packets";
    m_DataPacketsReceivedBroadcast.SetInfo(name, description, units);

    name = "DataPacketsForwardedBroadcast";
    description = "Data packets forwarded broadcast";
    units = "packets";
    m_DataPacketsForwardedBroadcast.SetInfo(name, description, units);

    name = "ControlPacketsSentBroadcast";
    description = "Control packets sent broadcast";
    units = "packets";
    m_ControlPacketsSentBroadcast.SetInfo(name, description, units);

    name = "ControlPacketsReceivedBroadcast";
    description = "Control packets received broadcast";
    units = "packets";
    m_ControlPacketsReceivedBroadcast.SetInfo(name, description, units);

    name = "ControlPacketsForwardedBroadcast";
    description = "Control packets forwarded broadcast";
    units = "packets";
    m_ControlPacketsForwardedBroadcast.SetInfo(name, description, units);

    name = "DataBytesSentBroadcast";
    description = "Data bytes sent broadcast";
    units = "bytes";
    m_DataBytesSentBroadcast.SetInfo(name, description, units);

    name = "DataBytesReceivedBroadcast";
    description = "Data bytes received broadcast";
    units = "bytes";
    m_DataBytesReceivedBroadcast.SetInfo(name, description, units);

    name = "DataBytesForwardedBroadcast";
    description = "Data bytes forwarded broadcast";
    units = "bytes";
    m_DataBytesForwardedBroadcast.SetInfo(name, description, units);

    name = "ControlBytesSentBroadcast";
    description = "Control bytes sent broadcast";
    units = "bytes";
    m_ControlBytesSentBroadcast.SetInfo(name, description, units);

    name = "ControlBytesReceivedBroadcast";
    description = "Control bytes received broadcast";
    units = "bytes";
    m_ControlBytesReceivedBroadcast.SetInfo(name, description, units);

    name = "ControlBytesForwardedBroadcast";
    description = "Control bytes forwarded broadcast";
    units = "bytes";
    m_ControlBytesForwardedBroadcast.SetInfo(name, description, units);



    name = "DataPacketsSentMulticast";
    description = "Data packet sent multicast";
    units = "packets";
    m_DataPacketsSentMulticast.SetInfo(name, description, units);

    name = "DataPacketsReceivedMulticast";
    description = "Data packets received multicast";
    units = "packets";
    m_DataPacketsReceivedMulticast.SetInfo(name, description, units);

    name = "DataPacketsForwardedMulticast";
    description = "Data packets forwarded multicast";
    units = "packets";
    m_DataPacketsForwardedMulticast.SetInfo(name, description, units);

    name = "ControlPacketsSentMulticast";
    description = "Control packets sent multicast";
    units = "packets";
    m_ControlPacketsSentMulticast.SetInfo(name, description, units);

    name = "ControlPacketsReceivedMulticast";
    description = "Control packets received multicast";
    units = "packets";
    m_ControlPacketsReceivedMulticast.SetInfo(name, description, units);

    name = "ControlPacketsForwardedMulticast";
    description = "Control packets forwarded multicast";
    units = "packets";
    m_ControlPacketsForwardedMulticast.SetInfo(name, description, units);

    name = "DataBytesSentMulticast";
    description = "Data bytes sent multicast";
    units = "bytes";
    m_DataBytesSentMulticast.SetInfo(name, description, units);

    name = "DataBytesReceivedMulticast";
    description = "Data bytes received multicast";
    units = "bytes";
    m_DataBytesReceivedMulticast.SetInfo(name, description, units);

    name = "DataBytesForwardedMulticast";
    description = "Data bytes forwarded multicast";
    units = "bytes";
    m_DataBytesForwardedMulticast.SetInfo(name, description, units);

    name = "ControlBytesSentMulticast";
    description = "Control bytes sent multicast";
    units = "bytes";
    m_ControlBytesSentMulticast.SetInfo(name, description, units);

    name = "ControlBytesReceivedMulticast";
    description = "Control bytes received multicast";
    units = "bytes";
    m_ControlBytesReceivedMulticast.SetInfo(name, description, units);

    name = "ControlBytesForwardedMulticast";
    description = "Control bytes forwarded multicast";
    units = "bytes";
    m_ControlBytesForwardedMulticast.SetInfo(name, description, units);



    name = "CarriedLoad";
    description = "Carried load";
    units = "bits/second";
    m_CarriedLoad.SetInfo(name, description, units);

    name = "PacketsDroppedNoRoute";
    description = "Packets dropped due to no route";
    units = "packets";
    m_PacketsDroppedNoRoute.SetInfo(name, description, units);

    name = "PacketsDroppedTtlExpired";
    description = "Packets dropped due to expired TTL";
    units = "packets";
    m_PacketsDroppedTtlExpired.SetInfo(name, description, units);

    name = "PacketsDroppedQueueOverflow";
    description = "Packets dropped due to queue overflow";
    units = "packets";
    m_PacketsDroppedQueueOverflow.SetInfo(name, description, units);

    name = "PacketsDroppedOther";
    description = "Packets dropped due to other reasons";
    units = "packets";
    m_PacketsDroppedOther.SetInfo(name, description, units);

    name = "AverageDelay";
    description = "Average delay";
    units = "seconds";
    m_AverageDelay.SetInfo(name, description, units);

    name = "AverageJitter";
    description = "Average jitter";
    units = "seconds";
    m_AverageJitter.SetInfo(name, description, units);

    name = "AverageDataHopCount";
    description = "Average hop count for data packets";
    units = "hops";
    m_AverageDataHopCount.SetInfo(name, description, units);

    name = "AverageControlHopCount";
    description = "Average hop count for control packets";
    units = "hops";
    m_AverageControlHopCount.SetInfo(name, description, units);

    //.Added new Stats APIs required for Network Aggregate Table
    name = "CarriedLoadUnicast";
    description = "Carried load unicast";
    units = "bits/second";
    m_CarriedLoadUnicast.SetInfo(name, description, units);

    name = "CarriedLoadMulticast";
    description = "Carried load multicast";
    units = "bits/second";
    m_CarriedLoadMulticast.SetInfo(name, description, units);

    name = "CarriedLoadBroadcast";
    description = "Carried load broadcast";
    units = "bits/second";
    m_CarriedLoadBroadcast.SetInfo(name, description, units);

    name = "PacketsDroppedNoRouteUnicast";
    description = "Unicast Packets dropped due to no route";
    units = "packets";
    m_PacketsDroppedNoRouteUnicast.SetInfo(name, description, units);

    name = "PacketsDroppedNoRouteMulticast";
    description = "Multicast Packets dropped due to no route";
    units = "packets";
    m_PacketsDroppedNoRouteMulticast.SetInfo(name, description, units);

    name = "AverageDelayUnicast";
    description = "Average delay unicast";
    units = "seconds";
    m_AverageDelayUnicast.SetInfo(name, description, units);

    name = "AverageDelayMulticast";
    description = "Average delay multicast";
    units = "seconds";
    m_AverageDelayMulticast.SetInfo(name, description, units);

    name = "AverageDelayBroadcast";
    description = "Average delay broadcast";
    units = "seconds";
    m_AverageDelayBroadcast.SetInfo(name, description, units);

    name = "AverageJitterUnicast";
    description = "Average jitter unicast";
    units = "seconds";
    m_AverageJitterUnicast.SetInfo(name, description, units);

    name = "AverageJitterMulticast";
    description = "Average jitter multicast";
    units = "seconds";
    m_AverageJitterMulticast.SetInfo(name, description, units);

    name = "AverageJitterBroadcast";
    description = "Average jitter broadcast";
    units = "seconds";
    m_AverageJitterBroadcast.SetInfo(name, description, units);

    name = "OriginatedCarriedLoad";
    description = "Originated Carried load";
    units = "bits/second";
    m_OriginatedCarriedLoad.SetInfo(name, description, units);

    name = "ForwardedCarriedLoad";
    description = "Forwarded Carried load";
    units = "bits/second";
    m_ForwardedCarriedLoad.SetInfo(name, description, units);
}

void STAT_NetStatistics::AddToGlobal(STAT_GlobalNetStatistics* stat)
{
    stat->GetDataPacketsSentUnicast().AddStatistic(&m_DataPacketsSentUnicast);
    stat->GetDataPacketsReceivedUnicast().AddStatistic(&m_DataPacketsReceivedUnicast);
    stat->GetDataPacketsForwardedUnicast().AddStatistic(&m_DataPacketsForwardedUnicast);
    stat->GetControlPacketsSentUnicast().AddStatistic(&m_ControlPacketsSentUnicast);
    stat->GetControlPacketsReceivedUnicast().AddStatistic(&m_ControlPacketsReceivedUnicast);
    stat->GetControlPacketsForwardedUnicast().AddStatistic(&m_ControlPacketsForwardedUnicast);
    stat->GetDataBytesSentUnicast().AddStatistic(&m_DataBytesSentUnicast);
    stat->GetDataBytesReceivedUnicast().AddStatistic(&m_DataBytesReceivedUnicast);
    stat->GetDataBytesForwardedUnicast().AddStatistic(&m_DataBytesForwardedUnicast);
    stat->GetControlBytesSentUnicast().AddStatistic(&m_ControlBytesSentUnicast);
    stat->GetControlBytesReceivedUnicast().AddStatistic(&m_ControlBytesReceivedUnicast);
    stat->GetControlBytesForwardedUnicast().AddStatistic(&m_ControlBytesForwardedUnicast);

    stat->GetDataPacketsSentBroadcast().AddStatistic(&m_DataPacketsSentBroadcast);
    stat->GetDataPacketsReceivedBroadcast().AddStatistic(&m_DataPacketsReceivedBroadcast);
    stat->GetDataPacketsForwardedBroadcast().AddStatistic(&m_DataPacketsForwardedBroadcast);
    stat->GetControlPacketsSentBroadcast().AddStatistic(&m_ControlPacketsSentBroadcast);
    stat->GetControlPacketsReceivedBroadcast().AddStatistic(&m_ControlPacketsReceivedBroadcast);
    stat->GetControlPacketsForwardedBroadcast().AddStatistic(&m_ControlPacketsForwardedBroadcast);
    stat->GetDataBytesSentBroadcast().AddStatistic(&m_DataBytesSentBroadcast);
    stat->GetDataBytesReceivedBroadcast().AddStatistic(&m_DataBytesReceivedBroadcast);
    stat->GetDataBytesForwardedBroadcast().AddStatistic(&m_DataBytesForwardedBroadcast);
    stat->GetControlBytesSentBroadcast().AddStatistic(&m_ControlBytesSentBroadcast);
    stat->GetControlBytesReceivedBroadcast().AddStatistic(&m_ControlBytesReceivedBroadcast);
    stat->GetControlBytesForwardedBroadcast().AddStatistic(&m_ControlBytesForwardedBroadcast);

    stat->GetDataPacketsSentMulticast().AddStatistic(&m_DataPacketsSentMulticast);
    stat->GetDataPacketsReceivedMulticast().AddStatistic(&m_DataPacketsReceivedMulticast);
    stat->GetDataPacketsForwardedMulticast().AddStatistic(&m_DataPacketsForwardedMulticast);
    stat->GetControlPacketsSentMulticast().AddStatistic(&m_ControlPacketsSentMulticast);
    stat->GetControlPacketsReceivedMulticast().AddStatistic(&m_ControlPacketsReceivedMulticast);
    stat->GetControlPacketsForwardedMulticast().AddStatistic(&m_ControlPacketsForwardedMulticast);
    stat->GetDataBytesSentMulticast().AddStatistic(&m_DataBytesSentMulticast);
    stat->GetDataBytesReceivedMulticast().AddStatistic(&m_DataBytesReceivedMulticast);
    stat->GetDataBytesForwardedMulticast().AddStatistic(&m_DataBytesForwardedMulticast);
    stat->GetControlBytesSentMulticast().AddStatistic(&m_ControlBytesSentMulticast);
    stat->GetControlBytesReceivedMulticast().AddStatistic(&m_ControlBytesReceivedMulticast);
    stat->GetControlBytesForwardedMulticast().AddStatistic(&m_ControlBytesForwardedMulticast);

    stat->GetCarriedLoad().AddStatistic(&m_CarriedLoad);
    stat->GetPacketsDroppedNoRoute().AddStatistic(&m_PacketsDroppedNoRoute);
    stat->GetPacketsDroppedTtlExpired().AddStatistic(&m_PacketsDroppedTtlExpired);
    stat->GetPacketsDroppedQueueOverflow().AddStatistic(&m_PacketsDroppedQueueOverflow);
    stat->GetPacketsDroppedOther().AddStatistic(&m_PacketsDroppedOther);
    stat->GetAverageDelay().AddStatistic(&m_AverageDelay);
    stat->GetAverageJitter().AddStatistic(&m_AverageJitter);
    stat->GetAverageDataHopCount().AddStatistic(&m_AverageDataHopCount);
    stat->GetAverageControlHopCount().AddStatistic(&m_AverageControlHopCount);

    // Added new Stats APIs required for Network Aggregate Table
    stat->GetCarriedLoadUnicast().AddStatistic(&m_CarriedLoadUnicast);
    stat->GetCarriedLoadMulticast().AddStatistic(&m_CarriedLoadMulticast);
    stat->GetCarriedLoadBroadcast().AddStatistic(&m_CarriedLoadBroadcast);
    stat->GetPacketsDroppedNoRouteUnicast().AddStatistic(&m_PacketsDroppedNoRouteUnicast);
    stat->GetPacketsDroppedNoRouteMulticast().AddStatistic(&m_PacketsDroppedNoRouteMulticast);
    stat->GetAverageDelayUnicast().AddStatistic(&m_AverageDelayUnicast);
    stat->GetAverageDelayMulticast().AddStatistic(&m_AverageDelayMulticast);
    stat->GetAverageDelayBroadcast().AddStatistic(&m_AverageDelayBroadcast);
    stat->GetAverageJitterUnicast().AddStatistic(&m_AverageJitterUnicast);
    stat->GetAverageJitterMulticast().AddStatistic(&m_AverageJitterMulticast);
    stat->GetAverageJitterBroadcast().AddStatistic(&m_AverageJitterBroadcast);
}

void STAT_NetStatistics::AddPacketSentToTransportDataPoints(
    Node* node,
    Message* msg,
    int interfaceIndex,
    BOOL isData)
{
    STAT_Timing* timing = (STAT_Timing*)
        MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsTiming);
    // Temporarily remove assertion until network stats is finished
    if (timing == NULL)
    {
        ERROR_ReportWarning("No timing data for packet");
        return;
    }
    if (timing->sourceSentNetworkDown == -1)
    {
        ERROR_ReportWarning("Packet was not sent down from network");
        return;
    }

    clocktype delay = node->getNodeTime() - timing->sourceSentNetworkDown;
    
    m_AverageDelay.AddDataPoint(node->getNodeTime(), (double) delay / SECOND);
    
    if (isData)
    {
        m_AverageDataHopCount.AddDataPoint(node->getNodeTime(), timing->hopCount);
    }
    else
    {
        m_AverageControlHopCount.AddDataPoint(node->getNodeTime(), timing->hopCount);
    }

    // Calculate/update jitter based on interface index
    if (m_LastDelay[interfaceIndex] != -1)
    {
        clocktype d;

        if (delay > m_LastDelay[interfaceIndex])
        {
            d = delay - m_LastDelay[interfaceIndex];
        }
        else
        {
            d = m_LastDelay[interfaceIndex] - delay;
        }
        m_AverageJitter.AddDataPoint(node->getNodeTime(), (Float64) d / SECOND);
    }
    m_LastDelay[interfaceIndex] = delay;
}

void STAT_NetStatistics::AddPacketSentToMacDataPoints(
    Node* node,
    Message* msg,
    STAT_DestAddressType type,
    BOOL isData,
    BOOL isForward)
{
    clocktype now = node->getNodeTime();

    STAT_Timing* timing = NULL;

    // Get timing for all info fields
    int count = MESSAGE_ReturnNumFrags(msg);
    if (count == 0)
    {
        count = 1;
    }
    for (int i = 0; i < count; i++)
    {
        // Get timing. Must have at least one timing. There may be
        // multiple timings if multiple packets are combined.
        if (i == 0)
        {
            timing = STAT_GetTiming(node, msg);
        }
        else
        {
            timing = (STAT_Timing*)
                MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsTiming, i);
        }

        if (timing == NULL)
        {
            ERROR_ReportWarning("Timing info is not set for this frame.\n");
            continue;
        }

        // Called once when the packet is decided to be forwarded
        // Called once when it is sent to mac.  Make sure we only
        // update the hop count once.
        if (!isForward)
        {
            if (timing->sourceSentNetworkDown == -1)
            {
                timing->sourceSentNetworkDown = node->getNodeTime();
            }
        }

        // Hop count should be incremented when packet
        // is sent from originating node or is forwarded by other nodes
        // In case of fragmented packets, hop count should be incremented
        // for every fragment.
        timing->hopCount++;

        if (timing->sourceNodeId == ANY_NODEID)
        {
            timing->sourceNodeId = node->nodeId;
        }
    }
    if (type == STAT_Unicast)
    {
        if (isData)
        {
            if (isForward)
            {
                m_DataPacketsForwardedUnicast.AddDataPoint(now, 1);
                m_DataBytesForwardedUnicast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
                m_ForwardedCarriedLoad.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
            }
            else
            {
                m_DataPacketsSentUnicast.AddDataPoint(now, 1);
                m_DataBytesSentUnicast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
                m_OriginatedCarriedLoad.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
            }
        }
        else
        {
            if (isForward)
            {
                m_ControlPacketsForwardedUnicast.AddDataPoint(now, 1);
                m_ControlBytesForwardedUnicast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
                m_ForwardedCarriedLoad.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
            }
            else
            {
                m_ControlPacketsSentUnicast.AddDataPoint(now, 1);
                m_ControlBytesSentUnicast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
                m_OriginatedCarriedLoad.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
            }
        }
        m_CarriedLoadUnicast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
    }
    else if (type == STAT_Broadcast)
    {
        if (isData)
        {
            if (isForward)
            {
                m_DataPacketsForwardedBroadcast.AddDataPoint(now, 1);
                m_DataBytesForwardedBroadcast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
                m_ForwardedCarriedLoad.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
            }
            else
            {
                m_DataPacketsSentBroadcast.AddDataPoint(now, 1);
                m_DataBytesSentBroadcast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
                m_OriginatedCarriedLoad.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
            }
        }
        else
        {
            if (isForward)
            {
                m_ControlPacketsForwardedBroadcast.AddDataPoint(now, 1);
                m_ControlBytesForwardedBroadcast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
                m_ForwardedCarriedLoad.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
            }
            else
            {
                m_ControlPacketsSentBroadcast.AddDataPoint(now, 1);
                m_ControlBytesSentBroadcast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
                m_OriginatedCarriedLoad.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
            }
        }
        m_CarriedLoadBroadcast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
    }
    else if (type == STAT_Multicast)
    {
        if (isData)
        {
            if (isForward)
            {
                m_DataPacketsForwardedMulticast.AddDataPoint(now, 1);
                m_DataBytesForwardedMulticast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
                m_ForwardedCarriedLoad.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
            }
            else
            {
                m_DataPacketsSentMulticast.AddDataPoint(now, 1);
                m_DataBytesSentMulticast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
                m_OriginatedCarriedLoad.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
            }
        }
        else
        {
            if (isForward)
            {
                m_ControlPacketsForwardedMulticast.AddDataPoint(now, 1);
                m_ControlBytesForwardedMulticast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
                m_ForwardedCarriedLoad.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
            }
            else
            {
                m_ControlPacketsSentMulticast.AddDataPoint(now, 1);
                m_ControlBytesSentMulticast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
                m_OriginatedCarriedLoad.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
            }
        }
        m_CarriedLoadMulticast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
    }
    else
    {
        ERROR_ReportError("Unknown address type");
    }
    m_CarriedLoad.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
}

void STAT_NetStatistics::AddPacketSentToTransportDataPoints(
    Node* node,
    Message* msg,
    STAT_DestAddressType type,
    BOOL isData,
    int interfaceIndex)
{
    STAT_Timing* timing = (STAT_Timing*)
        MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsTiming);

    if (timing == NULL)
    {
        // Commented out until STAT API is completely integrated
        //ERROR_ReportWarning("No timing data for packet");
        return;
    }
    if (timing->sourceSentNetworkDown == -1)
    {
        // Commented out until STAT API is completely integrated
        //ERROR_ReportWarning("Packet was not sent down from network");
        return;
    }

    clocktype delay = node->getNodeTime() - timing->sourceSentNetworkDown;
    clocktype unicastDelay = node->getNodeTime() - timing->sourceSentNetworkDown;
    clocktype multicastDelay = node->getNodeTime() - timing->sourceSentNetworkDown;
    clocktype broadcastDelay = node->getNodeTime() - timing->sourceSentNetworkDown;

    if (type == STAT_Unicast)
    {
        m_AverageDelayUnicast.AddDataPoint(node->getNodeTime(),
                                             (double) unicastDelay / SECOND);
    }
    else if (type == STAT_Multicast)
    {
        m_AverageDelayMulticast.AddDataPoint(node->getNodeTime(),
                                           (double) multicastDelay / SECOND);
    }
    else if (type == STAT_Broadcast)
    {
        m_AverageDelayBroadcast.AddDataPoint(node->getNodeTime(),
                                           (double) broadcastDelay / SECOND);
    }
    m_AverageDelay.AddDataPoint(node->getNodeTime(), (double) delay / SECOND);
    
    if (isData)
    {
        m_AverageDataHopCount.AddDataPoint(node->getNodeTime(), timing->hopCount);
    }
    else
    {
        m_AverageControlHopCount.AddDataPoint(node->getNodeTime(), timing->hopCount);
    }

    // Calculate/update Unicast jitter based on interface index
    if (type == STAT_Unicast)
    {
        if (m_LastUnicastDelay[interfaceIndex] != -1)
        {
            clocktype d;

            if (unicastDelay > m_LastUnicastDelay[interfaceIndex])
            {
                d = unicastDelay - m_LastUnicastDelay[interfaceIndex];
            }
            else
            {
                d = m_LastUnicastDelay[interfaceIndex] - unicastDelay;
            }
            m_AverageJitterUnicast.AddDataPoint(node->getNodeTime(), (Float64) d / SECOND);
        }
        m_LastUnicastDelay[interfaceIndex] = unicastDelay;
    }

    // Calculate/update Multicast jitter based on interface index
    clocktype diffDelay;
    if (type == STAT_Multicast)
    {
        if (m_LastMulticastDelay[interfaceIndex] != -1)
        {
            if (multicastDelay > m_LastMulticastDelay[interfaceIndex])
            {
                diffDelay = multicastDelay - m_LastMulticastDelay[interfaceIndex];
            }
            else
            {
                diffDelay = m_LastMulticastDelay[interfaceIndex] - multicastDelay;
            }
            m_AverageJitterMulticast.AddDataPoint(node->getNodeTime(), (Float64) diffDelay / SECOND);
        }
        m_LastMulticastDelay[interfaceIndex] = multicastDelay;
    }

    // Calculate/update Broadcast jitter based on interface index
    if (type == STAT_Broadcast)
    {
        if (m_LastBroadcastDelay[interfaceIndex] != -1)
        {
            if (broadcastDelay > m_LastBroadcastDelay[interfaceIndex])
            {
                diffDelay = broadcastDelay - m_LastBroadcastDelay[interfaceIndex];
            }
            else
            {
                diffDelay = m_LastBroadcastDelay[interfaceIndex] - broadcastDelay;
            }
            m_AverageJitterBroadcast.AddDataPoint(node->getNodeTime(), (Float64) diffDelay / SECOND);
        }
        m_LastBroadcastDelay[interfaceIndex] = broadcastDelay;
    }

    // Calculate/update jitter based on interface index
    if (m_LastDelay[interfaceIndex] != -1)
    {
        if (delay > m_LastDelay[interfaceIndex])
        {
            diffDelay = delay - m_LastDelay[interfaceIndex];
        }
        else
        {
            diffDelay = m_LastDelay[interfaceIndex] - delay;
        }
        m_AverageJitter.AddDataPoint(node->getNodeTime(), (Float64) diffDelay / SECOND);
    }
    m_LastDelay[interfaceIndex] = delay;
}

void STAT_NetStatistics::AddPacketReceivedFromMacDataPoints(
    Node* node,
    Message* msg,
    STAT_DestAddressType type,
    int interfaceIndex,
    BOOL isData)
{
    clocktype now = node->getNodeTime();

    AddPacketSentToTransportDataPoints(node, msg, type, isData,
        interfaceIndex);

    if (type == STAT_Unicast)
    {
        if (isData)
        {
            m_DataPacketsReceivedUnicast.AddDataPoint(now, 1);
            m_DataBytesReceivedUnicast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
        }
        else
        {
            m_ControlPacketsReceivedUnicast.AddDataPoint(now, 1);
            m_ControlBytesReceivedUnicast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
        }
    }
    else if (type == STAT_Broadcast)
    {
        if (isData)
        {
            m_DataPacketsReceivedBroadcast.AddDataPoint(now, 1);
            m_DataBytesReceivedBroadcast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
        }
        else
        {
            m_ControlPacketsReceivedBroadcast.AddDataPoint(now, 1);
            m_ControlBytesReceivedBroadcast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
        }
    }
    else if (type == STAT_Multicast)
    {
        if (isData)
        {
            m_DataPacketsReceivedMulticast.AddDataPoint(now, 1);
            m_DataBytesReceivedMulticast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
        }
        else
        {
            m_ControlPacketsReceivedMulticast.AddDataPoint(now, 1);
            m_ControlBytesReceivedMulticast.AddDataPoint(now, MESSAGE_ReturnPacketSize(msg));
        }
    }
    else
    {
        ERROR_ReportError("Unknown address type");
    }
}

void STAT_NetStatistics::AddPacketDroppedNoRouteDataPoints(Node* node)
{
    m_PacketsDroppedNoRoute.AddDataPoint(node->getNodeTime(), 1);
}

void STAT_NetStatistics::AddPacketDroppedNoRouteDataPointsUnicast(Node* node)
{
    m_PacketsDroppedNoRouteUnicast.AddDataPoint(node->getNodeTime(), 1);
}

void STAT_NetStatistics::AddPacketDroppedNoRouteDataPointsMulticast(Node* node)
{
    m_PacketsDroppedNoRouteMulticast.AddDataPoint(node->getNodeTime(), 1);
}

void STAT_NetStatistics::AddPacketDroppedTtlExpiredDataPoints(Node* node)
{
    m_PacketsDroppedTtlExpired.AddDataPoint(node->getNodeTime(), 1);
}

void STAT_NetStatistics::AddPacketDroppedQueueOverflowDataPoints(Node* node)
{
    m_PacketsDroppedQueueOverflow.AddDataPoint(node->getNodeTime(), 1);
}

void STAT_NetStatistics::AddPacketDroppedOtherDataPoints(Node* node)
{
    m_PacketsDroppedOther.AddDataPoint(node->getNodeTime(), 1);
}

void STAT_GlobalNetStatistics::Initialize(PartitionData* partition, STAT_StatisticsList* stats)
{
    std::string name;
    std::string path;
    std::string description;
    std::string units;

    // Set names for all global statistics and add partition based
    // statistics to the global statistics.
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkUnicastDataPacketsSent";
        path = "/stats/net/unicastDataPacketsSent";
        description = "Total Unicast data packets sent to mac layer";
        units = "packets";
        m_DataPacketsSentUnicast.SetInfo(
            name,
            description,
            units);
        m_DataPacketsSentUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataPacketsSentUnicast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkUnicastDataPacketsReceived";
        path = "/stats/net/dataPacketsRecievedUnicast";
        description = "Total unicast data packets received from mac layer";
        units = "packets";
        m_DataPacketsReceivedUnicast.SetInfo(
            name,
            description,
            units);
        m_DataPacketsReceivedUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataPacketsReceivedUnicast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkUnicastDataPacketsForwarded";
        path = "/stats/net/dataPacketsForwardedUnicast";
        description = "Total unicast data packets received from mac layer and forwarded";
        units = "packets";
        m_DataPacketsForwardedUnicast.SetInfo(
            name,
            description,
            units);
        m_DataPacketsForwardedUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataPacketsForwardedUnicast);

    if (partition->partitionId == 0)
    {
        name = "ControlNetworkPacketsSentUnicast";
        path = "/stats/net/controlPacketsSentUnicast";
        description = "Total unicast control packets sent to mac layer";
        units = "packets";
        m_ControlPacketsSentUnicast.SetInfo(
            name,
            description,
            units);
        m_ControlPacketsSentUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlPacketsSentUnicast);
    
    if (partition->partitionId == 0)
    {
        name = "ControlNetworkPacketsReceivedUnicast";
        path = "/stats/net/controlPacketsReceivedUnicast";
        description = "Total unicast control packets received from mac layer";
        units = "packets";
        m_ControlPacketsReceivedUnicast.SetInfo(
            name,
            description,
            units);
        m_ControlPacketsReceivedUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlPacketsReceivedUnicast);
    
    if (partition->partitionId == 0)
    {
        name = "ControlNetworkPacketsForwardedUnicast";
        path = "/stats/net/controlPacketsForwardedUnicast";
        description = "Total unicast control packets received from mac layer and forwarded";
        units = "packets";
        m_ControlPacketsForwardedUnicast.SetInfo(
            name,
            description,
            units);
        m_ControlPacketsForwardedUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlPacketsForwardedUnicast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataBytesSentUnicast";
        path = "/stats/net/dataBytesSentUnicast";
        description = "Total unicast data bytes sent to mac layer";
        units = "bytes";
        m_DataBytesSentUnicast.SetInfo(
            name,
            description,
            units);
        m_DataBytesSentUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataBytesSentUnicast);
    
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataBytesReceivedUnicast";
        path = "/stats/net/dataBytesReceivedUnicast";
        description = "Total unicast data bytes received from mac layer";
        units = "bytes";
        m_DataBytesReceivedUnicast.SetInfo(
            name,
            description,
            units);
        m_DataBytesReceivedUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataBytesReceivedUnicast);
    
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataBytesForwardedUnicast";
        path = "/stats/net/dataBytesForwardedUnicast";
        description = "Total unicast data bytes received from mac layer and forward";
        units = "bytes";
        m_DataBytesForwardedUnicast.SetInfo(
            name,
            description,
            units);
        m_DataBytesForwardedUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataBytesForwardedUnicast);
    
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlBytesSentUnicast";
        path = "/stats/net/controlBytesSentUnicast";
        description = "Total unicast control bytes sent to mac layer";
        units = "bytes";
        m_ControlBytesSentUnicast.SetInfo(
            name,
            description,
            units);
        m_ControlBytesSentUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlBytesSentUnicast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlBytesReceivedUnicast";
        path = "/stats/net/controlBytesReceivedUnicast";
        description = "Total unicast control bytes received from mac layer";
        units = "bytes";
        m_ControlBytesReceivedUnicast.SetInfo(
            name,
            description,
            units);
        m_ControlBytesReceivedUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlBytesReceivedUnicast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlBytesForwardedUnicast";
        path = "/stats/net/controlBytesForwardedUnicast";
        description = "Total unicast control bytes received from mac layer and forwarded";
        units = "bytes";
        m_ControlBytesForwardedUnicast.SetInfo(
            name,
            description,
            units);
        m_ControlBytesForwardedUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlBytesForwardedUnicast);



    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataPacketsSentBroadcast";
        path = "/stats/net/dataPacketsSentBroadcast";
        description = "Total broadcast data packets sent to mac layer";
        units = "packets";
        m_DataPacketsSentBroadcast.SetInfo(
            name,
            description,
            units);
        m_DataPacketsSentBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataPacketsSentBroadcast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataPacketsReceivedBroadcast";
        path = "/stats/net/dataPacketsRecievedBroadcast";
        description = "Total broadcast data packets received from mac layer";
        units = "packets";
        m_DataPacketsReceivedBroadcast.SetInfo(
            name,
            description,
            units);
        m_DataPacketsReceivedBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataPacketsReceivedBroadcast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataPacketsForwardedBroadcast";
        path = "/stats/net/dataPacketsForwardedBroadcast";
        description = "Total broadcast data packets received from mac layer and forwarded";
        units = "packets";
        m_DataPacketsForwardedBroadcast.SetInfo(
            name,
            description,
            units);
        m_DataPacketsForwardedBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataPacketsForwardedBroadcast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlPacketsSentBroadcast";
        path = "/stats/net/controlPacketsSentBroadcast";
        description = "Total broadcast control packets sent to mac layer";
        units = "packets";
        m_ControlPacketsSentBroadcast.SetInfo(
            name,
            description,
            units);
        m_ControlPacketsSentBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlPacketsSentBroadcast);
    
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlPacketsReceivedBroadcast";
        path = "/stats/net/controlPacketsReceivedBroadcast";
        description = "Total broadcast control packets received from mac layer";
        units = "packets";
        m_ControlPacketsReceivedBroadcast.SetInfo(
            name,
            description,
            units);
        m_ControlPacketsReceivedBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlPacketsReceivedBroadcast);
    
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlPacketsForwardedBroadcast";
        path = "/stats/net/controlPacketsForwardedBroadcast";
        description = "Total broadcast control packets received from mac layer and forwarded";
        units = "packets";
        m_ControlPacketsForwardedBroadcast.SetInfo(
            name,
            description,
            units);
        m_ControlPacketsForwardedBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlPacketsForwardedBroadcast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataBytesSentBroadcast";
        path = "/stats/net/dataBytesSentBroadcast";
        description = "Total broadcast data bytes sent to mac layer";
        units = "bytes";
        m_DataBytesSentBroadcast.SetInfo(
            name,
            description,
            units);
        m_DataBytesSentBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataBytesSentBroadcast);
    
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataBytesReceivedBroadcast";
        path = "/stats/net/dataBytesReceivedBroadcast";
        description = "Total broadcast data bytes received from mac layer";
        units = "bytes";
        m_DataBytesReceivedBroadcast.SetInfo(
            name,
            description,
            units);
        m_DataBytesReceivedBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataBytesReceivedBroadcast);
    
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataBytesForwardedBroadcast";
        path = "/stats/net/dataBytesForwardedBroadcast";
        description = "Total broadcast data bytes received from mac layer and forward";
        units = "bytes";
        m_DataBytesForwardedBroadcast.SetInfo(
            name,
            description,
            units);
        m_DataBytesForwardedBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataBytesForwardedBroadcast);
    
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlBytesSentBroadcast";
        path = "/stats/net/controlBytesSentBroadcast";
        description = "Total broadcast control bytes sent to mac layer";
        units = "bytes";
        m_ControlBytesSentBroadcast.SetInfo(
            name,
            description,
            units);
        m_ControlBytesSentBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlBytesSentBroadcast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlBytesReceivedBroadcast";
        path = "/stats/net/controlBytesReceivedBroadcast";
        description = "Total broadcast control bytes received from mac layer";
        units = "bytes";
        m_ControlBytesReceivedBroadcast.SetInfo(
            name,
            description,
            units);
        m_ControlBytesReceivedBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlBytesReceivedBroadcast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlBytesForwardedBroadcast";
        path = "/stats/net/controlBytesForwardedBroadcast";
        description = "Total broadcast control bytes received from mac layer and forwarded";
        units = "bytes";
        m_ControlBytesForwardedBroadcast.SetInfo(
            name,
            description,
            units);
        m_ControlBytesForwardedBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlBytesForwardedBroadcast);



    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataPacketsSentMulticast";
        path = "/stats/net/dataPacketsSentMulticast";
        description = "Total multicast data packets sent to mac layer";
        units = "packets";
        m_DataPacketsSentMulticast.SetInfo(
            name,
            description,
            units);
        m_DataPacketsSentMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataPacketsSentMulticast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataPacketsRecievedMulticast";
        path = "/stats/net/dataPacketsRecievedMulticast";
        description = "Total multicast data packets received from mac layer";
        units = "packets";
        m_DataPacketsReceivedMulticast.SetInfo(
            name,
            description,
            units);
        m_DataPacketsReceivedMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataPacketsReceivedMulticast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataPacketsForwardedMulticast";
        path = "/stats/net/dataPacketsForwardedMulticast";
        description = "Total multicast data packets received from mac layer and forwarded";
        units = "packets";
        m_DataPacketsForwardedMulticast.SetInfo(
            name,
            description,
            units);
        m_DataPacketsForwardedMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataPacketsForwardedMulticast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlPacketsSentMulticast";
        path = "/stats/net/controlPacketsSentMulticast";
        description = "Total multicast control packets sent to mac layer";
        units = "packets";
        m_ControlPacketsSentMulticast.SetInfo(
            name,
            description,
            units);
        m_ControlPacketsSentMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlPacketsSentMulticast);
    
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlPacketsReceivedMulticast";
        path = "/stats/net/controlPacketsReceivedMulticast";
        description = "Total multicast control packets received from mac layer";
        units = "packets";
        m_ControlPacketsReceivedMulticast.SetInfo(
            name,
            description,
            units);
        m_ControlPacketsReceivedMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlPacketsReceivedMulticast);
    
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlPacketsForwardedMulticast";
        path = "/stats/net/controlPacketsForwardedMulticast";
        description = "Total multicast control packets received from mac layer and forwarded";
        units = "packets";
        m_ControlPacketsForwardedMulticast.SetInfo(
            name,
            description,
            units);
        m_ControlPacketsForwardedMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlPacketsForwardedMulticast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataBytesSentMulticast";
        path = "/stats/net/dataBytesSentMulticast";
        description = "Total multicast data bytes sent to mac layer";
        units = "btes";
        m_DataBytesSentMulticast.SetInfo(
            name,
            description,
            units);
        m_DataBytesSentMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataBytesSentMulticast);
    
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataBytesReceivedMulticast";
        path = "/stats/net/dataBytesReceivedMulticast";
        description = "Total multicast data bytes received from mac layer";
        units = "bytes";
        m_DataBytesReceivedMulticast.SetInfo(
            name,
            description,
            units);
        m_DataBytesReceivedMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataBytesReceivedMulticast);
    
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkDataBytesForwardedMulticast";
        path = "/stats/net/dataBytesForwardedMulticast";
        description = "Total multicast data bytes received from mac layer and forward";
        units = "bytes";
        m_DataBytesForwardedMulticast.SetInfo(
            name,
            description,
            units);
        m_DataBytesForwardedMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_DataBytesForwardedMulticast);
    
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlBytesSentMulticast";
        path = "/stats/net/controlBytesSentMulticast";
        description = "Total multicast control bytes sent to mac layer";
        units = "bytes";
        m_ControlBytesSentMulticast.SetInfo(
            name,
            description,
            units);
        m_ControlBytesSentMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlBytesSentMulticast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlBytesReceivedMulticast";
        path = "/stats/net/controlBytesReceivedMulticast";
        description = "Total multicast control bytes received from mac layer";
        units = "bytes";
        m_ControlBytesReceivedMulticast.SetInfo(
            name,
            description,
            units);
        m_ControlBytesReceivedMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlBytesReceivedMulticast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkControlBytesForwardedMulticast";
        path = "/stats/net/controlBytesForwardedMulticast";
        description = "Total multicast control bytes received from mac layer and forwarded";
        units = "bytes";
        m_ControlBytesForwardedMulticast.SetInfo(
            name,
            description,
            units);
        m_ControlBytesForwardedMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_ControlBytesForwardedMulticast);



    if (partition->partitionId == 0)
    {
        name = "TotalNetworkCarriedLoad";
        path = "/stats/net/carriedLoad";
        description = "Total carried load at the network layer";
        units = "bits/s";
        m_CarriedLoad.SetInfo(
            name,
            description,
            units);
        m_CarriedLoad.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_CarriedLoad);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkPacketsDroppedNoRoute";
        path = "/stats/net/packetsDroppedNoRoute";
        description = "Number of packets dropped due to no route";
        units = "packets";
        m_PacketsDroppedNoRoute.SetInfo(
            name,
            description,
            units);
        m_PacketsDroppedNoRoute.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_PacketsDroppedNoRoute);
   
    if (partition->partitionId == 0)
    {
        name = "TotalNetworkPacketsDroppedTTLExpired";
        path = "/stats/net/packetsDroppedTtlExpired";
        description = "Number of packets dropped due to TTL expiration";
        units = "packets";
        m_PacketsDroppedTtlExpired.SetInfo(
            name,
            description,
            units);
        m_PacketsDroppedTtlExpired.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_PacketsDroppedTtlExpired);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkPacketsDroppedQueueOverflow";
        path = "/stats/net/packetsDroppedQueueOverflow";
        description = "Number of packets dropped due to overflowed output queue";
        units = "packets";
        m_PacketsDroppedQueueOverflow.SetInfo(
            name,
            description,
            units);
        m_PacketsDroppedQueueOverflow.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_PacketsDroppedQueueOverflow);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkPacketsDroppedOther";
        path = "/stats/net/packetsDroppedOther";
        description = "Number of packets dropped due to other reasons";
        units = "packets";
        m_PacketsDroppedOther.SetInfo(
            name,
            description,
            units);
        m_PacketsDroppedOther.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_PacketsDroppedOther);

    if (partition->partitionId == 0)
    {
        name = "AverageNetworkDelay";
        path = "/stats/net/averageDelay";
        description = "Average delay of packets processed by network layer grouped by interface";
        units = "seconds";
        m_AverageDelay.SetInfo(
            name,
            description,
            units);
        m_AverageDelay.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_AverageDelay);

    if (partition->partitionId == 0)
    {
        name = "AverageNetworkJitter";
        path = "/stats/net/averageJitter";
        description = "Average jitter of packets processed by network layer grouped by interface";
        units = "seconds";
        m_AverageJitter.SetInfo(
            name,
            description,
            units);
        m_AverageJitter.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_AverageJitter);

    if (partition->partitionId == 0)
    {
        name = "AverageNetworkDataHopCount";
        path = "/stats/net/averageDataHopCount";
        description = "Average hop count of data packets";
        units = "hops";
        m_AverageDataHopCount.SetInfo(
            name,
            description,
            units);
        m_AverageDataHopCount.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_AverageDataHopCount);

    if (partition->partitionId == 0)
    {
        name = "AverageNetworkControlHopCount";
        path = "/stats/net/averageControlHopCount";
        description = "Average hop count of control packets";
        units = "hops";
        m_AverageControlHopCount.SetInfo(
            name,
            description,
            units);
        m_AverageControlHopCount.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_AverageControlHopCount);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkCarriedLoadUnicast";
        path = "/stats/net/carriedLoadUnicast";
        description = "Total Unicast carried load at the network layer";
        units = "bits/s";
        m_CarriedLoadUnicast.SetInfo(
            name,
            description,
            units);
        m_CarriedLoadUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_CarriedLoadUnicast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkCarriedLoadMulticast";
        path = "/stats/net/carriedLoadMulticast";
        description = "Total Multicast carried load at the network layer";
        units = "bits/s";
        m_CarriedLoadMulticast.SetInfo(
            name,
            description,
            units);
        m_CarriedLoadMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_CarriedLoadMulticast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkCarriedLoadBroadcast";
        path = "/stats/net/carriedLoadBroadcast";
        description = "Total Broadcast carried load at the network layer";
        units = "bits/s";
        m_CarriedLoadBroadcast.SetInfo(
            name,
            description,
            units);
        m_CarriedLoadBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_CarriedLoadBroadcast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkPacketsDroppedNoRouteUnicast";
        path = "/stats/net/packetsDroppedNoRouteUnicast";
        description = "Number of Unicast packets dropped due to no route";
        units = "packets";
        m_PacketsDroppedNoRouteUnicast.SetInfo(
            name,
            description,
            units);
        m_PacketsDroppedNoRouteUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_PacketsDroppedNoRouteUnicast);

    if (partition->partitionId == 0)
    {
        name = "TotalNetworkPacketsDroppedNoRouteMulticast";
        path = "/stats/net/packetsDroppedNoRouteMulticast";
        description = "Number of Multicast packets dropped due to no route";
        units = "packets";
        m_PacketsDroppedNoRouteMulticast.SetInfo(
            name,
            description,
            units);
        m_PacketsDroppedNoRouteMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_PacketsDroppedNoRouteMulticast);

    if (partition->partitionId == 0)
    {
        name = "AverageNetworkDelayUnicast";
        path = "/stats/net/averageDelayUnicast";
        description = "Average delay of Unicast packets processed by network layer grouped by interface";
        units = "seconds";
        m_AverageDelayUnicast.SetInfo(
            name,
            description,
            units);
        m_AverageDelayUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_AverageDelayUnicast);

    if (partition->partitionId == 0)
    {
        name = "AverageNetworkDelayMulticast";
        path = "/stats/net/averageDelayMulticast";
        description = "Average delay of Multicast packets processed by network layer grouped by interface";
        units = "seconds";
        m_AverageDelayMulticast.SetInfo(
            name,
            description,
            units);
        m_AverageDelayMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_AverageDelayMulticast);

    if (partition->partitionId == 0)
    {
        name = "AverageNetworkDelayBroadcast";
        path = "/stats/net/averageDelayBroadcast";
        description = "Average delay of Broadcast packets processed by network layer grouped by interface";
        units = "seconds";
        m_AverageDelayBroadcast.SetInfo(
            name,
            description,
            units);
        m_AverageDelayBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_AverageDelayBroadcast);

    if (partition->partitionId == 0)
    {
        name = "AverageNetworkJitterUnicast";
        path = "/stats/net/averageJitterUnicast";
        description = "Average jitter of Unicast packets processed by network layer grouped by interface";
        units = "seconds";
        m_AverageJitterUnicast.SetInfo(
            name,
            description,
            units);
        m_AverageJitterUnicast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_AverageJitterUnicast);

    if (partition->partitionId == 0)
    {
        name = "AverageNetworkJitterMulticast";
        path = "/stats/net/averageJitterMulticast";
        description = "Average jitter of Multicast packets processed by network layer grouped by interface";
        units = "seconds";
        m_AverageJitterMulticast.SetInfo(
            name,
            description,
            units);
        m_AverageJitterMulticast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_AverageJitterMulticast);

    if (partition->partitionId == 0)
    {
        name = "AverageNetworkJitterBroadcast";
        path = "/stats/net/averageJitterBroadcast";
        description = "Average jitter of Broadcast packets processed by network layer grouped by interface";
        units = "seconds";
        m_AverageJitterBroadcast.SetInfo(
            name,
            description,
            units);
        m_AverageJitterBroadcast.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_AverageJitterBroadcast);

}

// Moved inside ADDON_DB as compilation error was coming when ADDON-DB
// was not enabled
// Moved inside ADDON_DB as compilation error was coming when ADDON-DB
// was not enabled
#ifdef ADDON_DB
void STAT_NetSummaryStatistics::InitializeFromOneHop(Node* node, OneHopNetworkData* stats)
{
    BOOL ignore;

    IO_ParseNodeIdOrHostAddress(
        stats->srcAddr.c_str(),
        &senderAddr,
        &ignore);
    IO_ParseNodeIdOrHostAddress(
        stats->rcvAddr.c_str(),
        &receiverAddr,
        &ignore);
    strcpy(destinationType, stats->destinationType.c_str());
    dataPacketsSent = stats->oneHopStats.uDataPacketsSent;
    dataPacketsRecd = stats->oneHopStats.uDataPacketsRecd;
    dataPacketsForward = stats->oneHopStats.uDataPacketsForward;
    controlPacketsSent = stats->oneHopStats.uControlPacketsSent;
    controlPacketsRecd = stats->oneHopStats.uControlPacketsRecd;
    controlPacketsForward = stats->oneHopStats.uControlPacketsForward;
    dataBytesSent = stats->oneHopStats.uDataBytesSent;
    dataBytesRecd = stats->oneHopStats.uDataBytesRecd;
    dataBytesForward = stats->oneHopStats.uDataBytesForward;
    controlBytesSent = stats->oneHopStats.uControlBytesSent;
    controlBytesRecd = stats->oneHopStats.uControlBytesRecd;
    controlBytesForward = stats->oneHopStats.uControlBytesForward;

    // Set delays and jitters if appropriate number of packets are received
    // Need 1 packet for delay
    // Need 3 packets for jitter
    if (dataPacketsRecd > 0)
    {
        dataDelay = stats->oneHopStats.dataPacketDelay / ((Float64) SECOND * dataPacketsRecd);
    }
    else
    {
        dataDelay = 0;
    }

    dataJitterDataPoints = dataPacketsRecd - 1;
    if (dataJitterDataPoints > 0)
    {
        dataJitter = stats->oneHopStats.dataPacketJitter / ((Float64) SECOND * dataJitterDataPoints);
    }
    else
    {
        dataJitter = 0;
    }

    if (controlPacketsRecd > 0)
    {
        controlDelay = stats->oneHopStats.controlPacketDelay / ((Float64) SECOND * controlPacketsRecd);
    }
    else
    {
        controlDelay = 0;
    }

    controlJitterDataPoints = controlPacketsRecd - 1;
    if (controlJitterDataPoints > 0)
    {
        controlJitter = stats->oneHopStats.controlPacketJitter / ((Float64) SECOND * controlJitterDataPoints);
    }
    else
    {
        controlJitter = 0;
    }
}
#endif /* ADDON_DB */

// Call to update this struct based on statistics from another model's
// summary statistics.  Can be called for a sender or receiver's stats.
STAT_NetSummaryStatistics& STAT_NetSummaryStatistics::operator +=(const STAT_NetSummaryStatistics& stats)
{
    // Update data delay
    double totalRecd = dataPacketsRecd + stats.dataPacketsRecd;
    if (dataPacketsRecd > 0)
    {
        dataDelay = (dataDelay * dataPacketsRecd
            + stats.dataDelay * stats.dataPacketsRecd)
            / totalRecd;
    }

    // Update data jitter
    totalRecd = dataJitterDataPoints + stats.dataJitterDataPoints;
    if (totalRecd > 0)
    {
        dataJitter = (dataJitter * dataJitterDataPoints
            + stats.dataJitter * stats.dataJitterDataPoints)
            / totalRecd;
    }

    // Update control delay
    totalRecd = controlPacketsRecd + stats.controlPacketsRecd;
    if (totalRecd > 0)
    {
        controlDelay = (controlDelay * controlPacketsRecd
            + stats.controlDelay * stats.controlPacketsRecd)
            / totalRecd;
    }

    // Update control jitter
    totalRecd = controlJitterDataPoints + stats.controlJitterDataPoints;
    if (totalRecd > 0)
    {
        controlJitter = (controlJitter * controlJitterDataPoints
            + stats.controlJitter * stats.controlJitterDataPoints)
            / totalRecd;
    }

    dataPacketsSent += stats.dataPacketsSent;
    dataPacketsRecd += stats.dataPacketsRecd;
    dataPacketsForward += stats.dataPacketsForward;
    controlPacketsSent += stats.controlPacketsSent;
    controlPacketsRecd += stats.controlPacketsRecd;
    controlPacketsForward += stats.controlPacketsForward;
    dataBytesSent += stats.dataBytesSent;
    dataBytesRecd += stats.dataBytesRecd;
    dataBytesForward += stats.dataBytesForward;
    controlBytesSent += stats.controlBytesSent;
    controlBytesRecd += stats.controlBytesRecd;
    controlBytesForward += stats.controlBytesForward;
    dataJitterDataPoints += stats.dataJitterDataPoints;
    controlJitterDataPoints += stats.controlJitterDataPoints;

    return *this;
}
