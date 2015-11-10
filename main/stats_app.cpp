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
#include "stats_app.h"
#include "stats_global.h"
#include "if_loopback.h"
#include "ipv6.h"

#ifdef ADDON_DB
#include "db.h"
#endif

void STAT_AppStatistics::SetStatNames()
{
    std::string name;
    std::string description;
    std::string units;

    // Add application and global application statistics to data points
    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        name = STAT_AddrToString(i) + std::string("SessionStart");
        description = STAT_AddrToString(i) + std::string(" Session Start");
        units = "seconds";
        m_AddrStats[i].m_SessionStart.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("SessionFinish");
        description = STAT_AddrToString(i) + std::string(" Session Finish");
        units = "seconds";
        m_AddrStats[i].m_SessionFinish.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("MessagesSent");
        description = std::string("Total ") + STAT_AddrToString(i)
            + " Messages Sent";
        units = "messages";
        m_AddrStats[i].m_MessagesSent.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("FirstMessageSent");
        description = std::string("First ") + STAT_AddrToString(i)
            + " Message Sent";
        units = "seconds";
        m_AddrStats[i].m_FirstMessageSent.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("LastMessageSent");
        description = std::string("Last ") + STAT_AddrToString(i)
            + " Message Sent";
        units = "seconds";
        m_AddrStats[i].m_LastMessageSent.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("MessagesReceived");
        description = std::string("Total ") + STAT_AddrToString(i)
            + " Messages Received";
        units = "messages";
        m_AddrStats[i].m_MessagesReceived.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("MessagesDropped");
        description = std::string("Total ") + STAT_AddrToString(i)
            + " Messages Dropped Due To Missing Fragment";
        units = "messages";
        m_AddrStats[i].m_MessagesDropped.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("FirstMessageReceived");
        description = std::string("First ") + STAT_AddrToString(i)
            + " Message Received";
        units = "seconds";
        m_AddrStats[i].m_FirstMessageReceived.SetInfo(
            name, description, units);

        name = STAT_AddrToString(i) + std::string("LastMessageReceived");
        description = std::string("Last ") + STAT_AddrToString(i)
            + " Message Received";
        units = "seconds";
        m_AddrStats[i].m_LastMessageReceived.SetInfo(
            name, description, units);

        name = STAT_AddrToString(i) + std::string("FirstFragmentSent");
        description = std::string("First ") + STAT_AddrToString(i)
            + " Fragment Sent";
        units = "seconds";
        m_AddrStats[i].m_FirstFragmentSent.SetInfo(
            name, description, units);

        name = STAT_AddrToString(i) + std::string("LastFragmentSent");
        description = std::string("Last ") + STAT_AddrToString(i)
            + " Fragment Sent";
        units = "seconds";
        m_AddrStats[i].m_LastFragmentSent.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("FirstFragmentReceived");
        description = std::string("First ") + STAT_AddrToString(i)
            + " Fragment Received";
        units = "seconds";
        m_AddrStats[i].m_FirstFragmentReceived.SetInfo(
            name, description, units);

        name = STAT_AddrToString(i) + std::string("LastFragmentReceived");
        description = std::string("Last ") + STAT_AddrToString(i)
            + " Fragment Received";
        units = "seconds";
        m_AddrStats[i].m_LastFragmentReceived.SetInfo(
            name, description, units);

        name = STAT_AddrToString(i) + std::string("FragmentsSent");
        description = std::string("Total ") + STAT_AddrToString(i)
            + " Fragments Sent";
        units = "fragments";
        m_AddrStats[i].m_FragmentsSent.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("FragmentsReceived"),
        description = std::string("Total ") + STAT_AddrToString(i)
        + " Fragments Received";
        units = "fragments";
        m_AddrStats[i].m_FragmentsReceived.SetInfo(
            name, description, units);

        name = STAT_AddrToString(i)
            + std::string("FragmentsReceivedOutOfOrder"),
        description = std::string("Total ") + STAT_AddrToString(i)
        + " Fragments Received Out of Order";
        units = "fragments";
        m_AddrStats[i].m_FragmentsReceivedOutOfOrder.SetInfo(
            name, description, units);

        name = STAT_AddrToString(i)
            + std::string("DuplicateFragmentsReceived"),
        description = std::string("Total ") + STAT_AddrToString(i)
        + " Duplicate Fragments Received";
        units = "fragments";
        m_AddrStats[i].m_DuplicateFragmentsReceived.SetInfo(
            name, description, units);

        name = STAT_AddrToString(i) + std::string("DataSent");
        description = std::string("Total ") + STAT_AddrToString(i)
            + " Data Sent";
        units = "bytes";
        m_AddrStats[i].m_DataSent.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("DataReceived");
        description = std::string("Total ") + STAT_AddrToString(i)
            + " Data Received";
        units = "bytes";
        m_AddrStats[i].m_DataReceived.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("OverheadSent");
        description = std::string("Total ") + STAT_AddrToString(i)
            + " Overhead Sent";
        units = "bytes";
        m_AddrStats[i].m_OverheadSent.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("OverheadReceived");
        description = std::string("Total ") + STAT_AddrToString(i)
            + " Overhead Received";
        units = "bytes";
        m_AddrStats[i].m_OverheadReceived.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("AverageDelay");
        description = std::string("Average ") + STAT_AddrToString(i)
            + " End-to-End Delay";
        units = "seconds";
        m_AddrStats[i].m_AverageDelay.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("OfferedLoad");
        description = STAT_AddrToString(i) + std::string(" Offered Load");
        units = "bits/second";
        m_AddrStats[i].m_OfferedLoad.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("Throughput");
        description = STAT_AddrToString(i) + std::string(" Received Throughput");
        units = "bits/second";
        m_AddrStats[i].m_Throughput.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("Jitter");
        description = std::string("Smoothed ") + STAT_AddrToString(i)
            + " Jitter";
        units = "seconds";
        m_AddrStats[i].m_Jitter.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("AverageJitter");
        description = std::string("Average ") + STAT_AddrToString(i)
            + " Jitter";
        units = "seconds";
        m_AddrStats[i].m_AverageJitter.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("TotalJitter");
        description = std::string("Total ") + STAT_AddrToString(i)
            + " Jitter";
        units = "seconds";
        m_AddrStats[i].m_TotalJitter.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + std::string("AverageHopCount");
        description = std::string("Average Hop Count Of Received ")
            + STAT_AddrToString(i) + " Fragments";
        units = "hops";
        m_AddrStats[i].m_AverageHopCount.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + "EffectiveFragmentsSent";
        if (i == STAT_Multicast)
        {
            description = "Effective fragments sent, fragments sent times multicast group size";
        }
        else
        {
            description = "Effective fragments sent, equivalent to fragments sent";
        }
        units = "fragments";
        m_AddrStats[i].m_EffectiveFragmentsSent.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + "EffectiveMessagesSent";
        if (i == STAT_Multicast)
        {
            description = "Effective messages sent is messages sent times multicast group size";
        }
        else
        {
            description = "Effective messages sent, equivalent to messages sent";
        }
        units = "messages";
        m_AddrStats[i].m_EffectiveMessagesSent.SetInfo(name, description, units);

        name = STAT_AddrToString(i) + "EffectiveDataSent";
        if (i == STAT_Multicast)
        {
            description = "Effective data sent is data sent times multicast group size";
        }
        else
        {
            description = "Effective data sent, equivalent to data sent";
        }
        units = "bytes";
        m_AddrStats[i].m_EffectiveDataSent.SetInfo(name, description, units);
    }
}

void STAT_AppStatistics::Finalize(Node* node)
{
    // Call parent Finalize function
    STAT_ModelStatistics::Finalize(node);
}

STAT_AppStatistics::STAT_AppStatistics(
    Node* node,
    const std::string& name,
    STAT_DestAddressType addrType,
    STAT_AppStatsType statsType,
    const char* customName) :
        m_StatsType(statsType),
        m_AutoRefragment(FALSE),
        m_RefragmentBytesLeft(0),
        m_RefragmentBytes(0),
        m_Initialized(FALSE),
        m_Unicast(FALSE),
        m_Broadcast(FALSE),
        m_Multicast(FALSE),
        m_Tos(APP_DEFAULT_TOS)
{
    m_Type = STAT_StatisticsList::CacheString(name);

    if (addrType == STAT_Unicast)
    {
        m_Unicast = TRUE;
    }
    else if (addrType == STAT_Multicast)
    {
        m_Multicast = TRUE;
    }
    else if (addrType == STAT_Broadcast)
    {
        m_Broadcast = TRUE;
    }

    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        m_AddrStats[i].m_LastDelay = -1;
    }

    if (!customName)
    {
        m_CustomName = "";
    }
    else
    {
        m_CustomName = customName;
    }
    SetStatNames();
}

void STAT_AppStatistics::EnableAutoRefragment()
{
    m_AutoRefragment = TRUE;
}

static void AddAppStatToHierarchy(
    Node* node,
    STAT_Statistic* stat,
    std::string app,
    NodeId source,
    int sessionId,
    BOOL isSource)
{
    D_Hierarchy* h = &node->partitionData->dynamicHierarchy;
    std::string path;

    if (!h->IsAppEnabled())
    {
        return;
    }

    if (isSource)
    {
        h->BuildApplicationPathString(
            node->nodeId,
            app,
            sessionId,
            stat->GetName(),
            path);
    }
    else
    {
        h->BuildApplicationServerPathString(
            node->nodeId,
            app,
            source,
            sessionId,
            stat->GetName(),
            path);
    }

    stat->AddToHierarchy(h, path);
}

void STAT_AppStatistics::Initialize(
    Node* node,
    Address sourceAddr,
    Address destAddr,
    STAT_SessionIdType sessionId,
    Int32 uniqueId)
{
    ERROR_Assert(
        !m_Initialized,
        "STAT_AppStatistics::Initialize was called twice");
    m_Initialized = true;

    m_SourceAddr = sourceAddr;
    m_DestAddr = destAddr;

    // Lookup sourceNodeId based on address
    NodeId sourceNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
        node,
        sourceAddr);
    if (sourceNodeId == INVALID_MAPPING) 
    {
        return;
    }
    ERROR_Assert(sourceNodeId != INVALID_MAPPING, "Invalid node id");

    // Set unique session information
    m_SourceNodeId = sourceNodeId;
    m_SessionId = sessionId;

    m_UniqueId = uniqueId;

    m_IsSource = sourceNodeId == node->nodeId;

    // Lookup sourceNodeId based on address if unicast
    // Otherwise set to ANY_ADDRESS
    if (m_IsSource)
    {
        if (Address_IsMulticastAddress(&destAddr)
     || Address_IsSubnetBroadcastAddress(node, &destAddr)
     || Address_IsAnyAddress(&destAddr))
        {
            m_DestNodeId = ANY_ADDRESS;
        }
        else if (destAddr.networkType == NETWORK_IPV4 &&
            NetworkIpIsLoopbackInterfaceAddress(destAddr.interfaceAddr.ipv4))
        {
            m_DestNodeId = sourceNodeId ;
        }
        else if (destAddr.networkType == NETWORK_IPV6 &&
            Ipv6IsLoopbackAddress(node, destAddr.interfaceAddr.ipv6) )      
        {
            m_DestNodeId = sourceNodeId ;
        }
        else
        {
            m_DestNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                node,
                destAddr);            

            
            ERROR_Assert(m_DestNodeId != INVALID_MAPPING, "Invalid node id");
        }
    }
    else
    {
        m_DestNodeId = node->nodeId;
    }
    if (OculaInterfaceEnabled())
    {
        char key[MAX_STRING_LENGTH];
        char value[MAX_STRING_LENGTH];

        sprintf(key, "/node/%d/appLayer/appCount",
            node->nodeId);
        sprintf(value, "%d", node->appData.uniqueId);
        SetOculaProperty(
            node->partitionData,
            key,
            value);
        sprintf(key, "/node/%d/appLayer/app/%d/appId",
            node->nodeId, m_UniqueId);
        sprintf(value, "%d", m_UniqueId);
        SetOculaProperty(
            node->partitionData,
            key,
            value);
        sprintf(key, "/node/%d/appLayer/app/%d/sessionId",
            node->nodeId, m_UniqueId);
        sprintf(value, "%u", m_SessionId);
        SetOculaProperty(
            node->partitionData,
            key,
            value);
        sprintf(key, "/node/%d/appLayer/app/%d/type",
            node->nodeId, m_UniqueId);
        sprintf(value, "%s", m_Type->c_str());
        SetOculaProperty(
            node->partitionData,
            key,
            value);
        sprintf(key, "/node/%d/appLayer/app/%d/name",
            node->nodeId, m_UniqueId);
        if (GetCustomName().empty())
        {
            sprintf(value, "%s", m_Type->c_str());
        }
        else
        {
            sprintf(value, "%s", m_CustomName.c_str());
        }
        SetOculaProperty(
            node->partitionData,
            key,
            value);
        sprintf(key, "/node/%d/appLayer/app/%d/sourceNodeId",
            node->nodeId, m_UniqueId);
        sprintf(value, "%u", m_SourceNodeId);
        SetOculaProperty(
            node->partitionData,
            key,
            value);
        sprintf(key, "/node/%d/appLayer/app/%d/destNodeId",
            node->nodeId, m_UniqueId);
        if (m_DestNodeId == ANY_ADDRESS)
        {
            strcpy(value, "-1");
        }
        else
        {
            sprintf(value, "%u", m_DestNodeId);
        }
        SetOculaProperty(
            node->partitionData,
            key,
            value);
    }

    // Add application and global application statistics to data points
    // Unicast summary stats use the Peer* variables
    // Multicast stats are found in the node's appData.multicastMap
    // structure
    std::vector<STAT_Statistic*> statsToAdd;
    GetList(statsToAdd);
    for (unsigned int i = 0; i < statsToAdd.size(); i++)
    {
        AddAppStatToHierarchy(
            node,
            statsToAdd[i],
            *m_Type,
            m_SourceNodeId,
            m_SessionId,
            m_IsSource);
    }

    AddToGlobal(&node->partitionData->stats->global.appAggregate);

    STAT_ModelStatistics::Initialize(node);
}

STAT_SessionIdType STAT_AppStatistics::GetSessionId(Message* msg)
{
#ifdef EXATA
    if (msg->isEmulationPacket)
    {
        return -1;
    }
#endif
    STAT_Timing* timing = (STAT_Timing*)
        MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsTiming);
    if (timing == NULL)
    {
        ERROR_ReportWarning("Packet was not sent with AddMessageSentDataPoints");
        return -1;
    }

    return timing->uniqueSessionId;
}

BOOL STAT_AppStatistics::IsSessionStarted(STAT_DestAddressType type)
{
    return GetSessionStart(type).GetNumDataPoints() > 0;
}

BOOL STAT_AppStatistics::IsSessionFinished(STAT_DestAddressType type)
{
    return GetSessionFinish(type).GetNumDataPoints() > 0;
}

void STAT_AppStatistics::AddToGlobal(STAT_GlobalAppStatistics* stat)
{
    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        stat->GetMessagesSent(i).AddStatistic(&GetMessagesSent(i));
        stat->GetMessagesReceived(i).AddStatistic(&GetMessagesReceived(i));
        stat->GetFragmentsSent(i).AddStatistic(&GetFragmentsSent(i));
        stat->GetFragmentsReceived(i).AddStatistic(&GetFragmentsReceived(i));
        stat->GetDataSent(i).AddStatistic(&GetDataSent(i));
        stat->GetDataReceived(i).AddStatistic(&GetDataReceived(i));
        stat->GetOverheadSent(i).AddStatistic(&GetOverheadSent(i));
        stat->GetOverheadReceived(i).AddStatistic(&GetOverheadReceived(i));
        // Comment next line to build average delay from dynamic hierarchy
        stat->GetAverageDelay(i).AddStatistic(&GetAverageDelay(i));
        stat->GetOfferedLoad(i).AddStatistic(&GetOfferedLoad(i));
        stat->GetThroughput(i).AddStatistic(&GetThroughput(i));
        stat->GetAverageJitter(i).AddStatistic(&GetAverageJitter(i));
        stat->GetTotalJitter(i).AddStatistic(&GetTotalJitter(i));
        stat->GetAverageHopCount(i).AddStatistic(&GetAverageHopCount(i));
        stat->GetAggregatedEffectiveFragmentsSent(i).AddStatistic(&GetEffectiveFragmentsSent(i));
        stat->GetAggregatedEffectiveMessagesSent(i).AddStatistic(&GetEffectiveMessagesSent(i));
        stat->GetAggregatedEffectiveDataSent(i).AddStatistic(&GetEffectiveDataSent(i));
    }
}

void STAT_AppStatistics::AddTiming(
    Node* node,
    Message* msg,
    int dataSize,
    int overheadSize)
{
    ERROR_Assert(msg != NULL, "Unable to get the Message here!");

    // Get timing information and always initialize
    STAT_Timing* timing = STAT_GetTiming(node, msg);

    // Set timing information
    timing->messageSize = dataSize + overheadSize;
    timing->sourceNodeId = m_SourceNodeId;
    timing->uniqueSessionId = m_SessionId;
    timing->sentApplicationDown = node->getNodeTime();
}
    
void STAT_AppStatistics::AddMessageSentDataPoints(
    Node* node,
    Message* msg,
    int controlSize,
    int dataSize,
    int overheadSize,
    STAT_DestAddressType type)
{
    ERROR_Assert(msg != NULL, "Unable to get the Message here!");

    clocktype now = node->getNodeTime();
    
    AddTiming(node, msg, dataSize, overheadSize);

    // Add data points
    m_AddrStats[type].m_MessagesSent.AddDataPoint(now, 1);
    m_AddrStats[type].m_FirstMessageSent.AddDataPoint(
        now, (double) node->getNodeTime() / SECOND);
    m_AddrStats[type].m_LastMessageSent.AddDataPoint(
        now, (double) node->getNodeTime() / SECOND);

    if (controlSize > 0)
    {
        ERROR_ReportError("no control yet");
    }
    else
    {
        m_AddrStats[type].m_DataSent.AddDataPoint(
            now, dataSize);
        m_AddrStats[type].m_OfferedLoad.AddDataPoint(
            now, dataSize);

        if (overheadSize > 0)
        {
            m_AddrStats[type].m_OverheadSent.AddDataPoint(
                now, overheadSize);
            m_AddrStats[type].m_OfferedLoad.AddDataPoint(
                now, overheadSize);
        }
    }
    if (OculaInterfaceEnabled())
    {
        char key[MAX_STRING_LENGTH];
        char value[MAX_STRING_LENGTH];

        sprintf(key, "/node/%d/appLayer/app/%d/stats/dataBytesSent",
            node->nodeId, m_UniqueId);
        sprintf(value, "%.lf", GetDataSent(type).GetValue(now));
        SetOculaProperty(
            node->partitionData,
            key,
            value);                            
    }
#ifdef ADDON_DB
    // If multicast, update effective stats
    if (type == STAT_Multicast)
    {
        AppData::APPL_MULTICAST_NODE_MAP* memberNodeMap = NULL;
        Int32 count = STATSDB_MulticastMembershipCount(
                        node,
                        GetIPv4Address(m_DestAddr),
                        &memberNodeMap);
        if (count > 0)
        {
            GetEffectiveMessagesSent(STAT_Multicast).AddDataPoint(now,
                                                                  count);

            AppData::APPL_MULTICAST_NODE_ITER nodeIt;
            for (nodeIt = memberNodeMap->begin();
                 nodeIt != memberNodeMap->end();
                 nodeIt++)
            {
                STAT_MulticastAppSessionStatistics* stats = 
                    GetMulticastSession(now,
                                        node->nodeId,
                                        nodeIt->first,
                                        m_SessionId);
                if (!stats)
                {
                    ERROR_ReportWarning("Cannot create and or retrieve "
                        "session stats");
                    return;
                }
                stats->m_MessagesSent.AddDataPoint(now, 1);

                if (controlSize <= 0)
                {
                    stats->m_OfferedLoad.AddDataPoint(now, dataSize);
                    if (overheadSize > 0)
                    {
                        stats->m_OfferedLoad.AddDataPoint(now,
                                                          overheadSize);
                    }
                }
            }
        }
    }
#endif
}

void STAT_AppStatistics::AddMessageReceivedDataPoints(
    Node* node,
    Message* msg,
    int controlSize,
    int dataSize,
    int overheadSize,
    STAT_DestAddressType type)
{
    clocktype now = node->getNodeTime();
    // Add data points
    m_AddrStats[type].m_MessagesReceived.AddDataPoint(now, 1);
    m_AddrStats[type].m_FirstMessageReceived.AddDataPoint(
        node->getNodeTime(), (double) now / SECOND);
    m_AddrStats[type].m_LastMessageReceived.AddDataPoint(
        node->getNodeTime(), (double) now / SECOND);

    if (controlSize > 0)
    {
        ERROR_ReportError("no control yet");
    }
    else
    {
        m_AddrStats[type].m_DataReceived.AddDataPoint(now, dataSize);
        m_AddrStats[type].m_Throughput.AddDataPoint(now, dataSize);

        if (overheadSize > 0)
        {
            m_AddrStats[type].m_OverheadReceived.AddDataPoint(
                now, overheadSize);
            m_AddrStats[type].m_Throughput.AddDataPoint(
                now, overheadSize);
        }
        if (OculaInterfaceEnabled())
        {
            char key[MAX_STRING_LENGTH];
            char value[MAX_STRING_LENGTH];

            sprintf(key, "/node/%d/appLayer/app/%d/stats/dataBytesReceived",
                node->nodeId, m_UniqueId);
            sprintf(value, "%.lf", GetDataReceived(type).GetValue(now));
            SetOculaProperty(
                node->partitionData,
                key,
                value);                            
        }
    }

    if (type == STAT_Multicast)
    {
        STAT_MulticastAppSessionStatistics* stats =
            GetMulticastSession(now,
                m_SourceNodeId,
                node->nodeId,
                m_SessionId);

        if (!stats)
        {
            ERROR_ReportWarning("Cannot create and or retrieve "
                "session stats");
            return;
        }
        stats->m_MessagesReceived.AddDataPoint(now, 1);
        if (controlSize <= 0)
        {
            stats->m_BytesReceived.AddDataPoint(now, dataSize);
            stats->m_Throughput.AddDataPoint(now, dataSize);
        }
        if (overheadSize > 0)
        {
            stats->m_Throughput.AddDataPoint(now, overheadSize);
        }
    }
}

void STAT_AppStatistics::AddFragmentSentDataPoints(
    Node* node,
    int size,
    STAT_DestAddressType type)
{
    clocktype now = node->getNodeTime();
    // Add data points
    m_AddrStats[type].m_FragmentsSent.AddDataPoint(now, 1);
    m_AddrStats[type].m_FirstFragmentSent.AddDataPoint(
        now, (double) now / SECOND);
    m_AddrStats[type].m_LastFragmentSent.AddDataPoint(
        now, (double) now / SECOND);

#ifdef ADDON_DB
    // If multicast, update effective stats
    if (type == STAT_Multicast)
    {
        AppData::APPL_MULTICAST_NODE_MAP* memberNodeMap = NULL;
        Int32 count = STATSDB_MulticastMembershipCount(
                        node,
                        GetIPv4Address(m_DestAddr),
                        &memberNodeMap);
        if (count > 0)
        {
            GetEffectiveFragmentsSent(STAT_Multicast).AddDataPoint(now,
                                                                   count);
            GetEffectiveDataSent(STAT_Multicast).AddDataPoint(now,
                                                              count*size);

            AppData::APPL_MULTICAST_NODE_ITER nodeIt;
            for (nodeIt = memberNodeMap->begin();
                 nodeIt != memberNodeMap->end();
                 nodeIt++)
            {
                STAT_MulticastAppSessionStatistics* stats =
                    GetMulticastSession(now,
                                        node->nodeId,
                                        nodeIt->first,
                                        m_SessionId);

                if (!stats)
                {
                    ERROR_ReportWarning("Cannot create and or retrieve "
                        "session stats");
                    return;
                }

                stats->m_FragmentsSent.AddDataPoint(now, 1);
                stats->m_BytesSent.AddDataPoint(now, size);
            }
        }
    }
#endif
}

void STAT_AppStatistics::AddFragmentReceivedDataPoints(
    Node* node,
    Message* msg,
    int size,
    STAT_DestAddressType type)
{
    ERROR_Assert(msg != NULL, "Unable to get the Message here!");
#ifdef EXATA
    if (msg->isEmulationPacket)
    {
        // Is emulated packet
        return;
    }
#endif

    STAT_Timing* timing;
    int packetSize;

    if (!IsSessionStarted(type))
    {
        SessionStart(node, type);
    }

    // Get timing for all info fields
    int count = MESSAGE_ReturnNumFrags(msg);
    if (count == 0)
    {
        count = 1;
    }
    for (int i = 0; i < count; i++)
    {
        // Get timing.  Must have at least one timing.  There may be
        // multiple timings if multiple packets are combined.
        if (i == 0)
        {
            timing = (STAT_Timing*)
                MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsTiming);
            if (timing == NULL)
            {
                ERROR_ReportWarning("Packet not sent using AddMessageSentDataPoints");
                return;
            }
        }
        else
        {
            timing = (STAT_Timing*)
                MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsTiming, i);
            if (timing == NULL)
            {
                continue;
            }
        }

        if (timing->sentApplicationDown == -1)
        {
            ERROR_ReportWarning("Message was not sent down from application");
            return;
        }

        if (MESSAGE_ReturnNumFrags(msg) > 0)
        {
            packetSize = MESSAGE_ReturnFragSize(msg, i);
        }
        else
        {
            packetSize = MESSAGE_ReturnPacketSize(msg);
        }

        clocktype delay = node->getNodeTime() - timing->sentApplicationDown;
        clocktype now = node->getNodeTime();
        // Add data points
        m_AddrStats[type].m_FragmentsReceived.AddDataPoint(now, 1);
        m_AddrStats[type].m_FirstFragmentReceived.AddDataPoint(
            now, (double) now / SECOND);
        m_AddrStats[type].m_LastFragmentReceived.AddDataPoint(
            now, (double) now / SECOND);
        m_AddrStats[type].m_AverageHopCount.AddDataPoint(
            now, timing->hopCount);

        m_AddrStats[type].m_AverageDelay.AddDataPoint(
            now, (Float64) delay /SECOND);
        m_AddrStats[type].m_TotalDelay.AddDataPoint(
            now, (Float64) delay /SECOND);

        if (m_AddrStats[type].m_LastDelay != -1)
        {
            clocktype d;

            if (delay > m_AddrStats[type].m_LastDelay)
            {
                d = delay - m_AddrStats[type].m_LastDelay;
            }
            else
            {
                d = m_AddrStats[type].m_LastDelay - delay;
            }
            m_AddrStats[type].m_Jitter.AddDataPoint(
                now, (Float64) d / SECOND);
            m_AddrStats[type].m_AverageJitter.AddDataPoint(
                now, (Float64) d / SECOND);
            m_AddrStats[type].m_TotalJitter.AddDataPoint(
                now, (Float64) d / SECOND);
        }
        m_AddrStats[type].m_LastDelay = delay;
        
        if (OculaInterfaceEnabled())
        {
            char key[MAX_STRING_LENGTH];
            char value[MAX_STRING_LENGTH];

            sprintf(key, "/node/%d/appLayer/app/%d/stats/averageDelay",
            node->nodeId, m_UniqueId);
            sprintf(value, "%lf", GetAverageDelay(type).GetValue(now));
            SetOculaProperty(
                node->partitionData,
                key,
                value);
        }
        if (type == STAT_Multicast)
        {            
            STAT_MulticastAppSessionStatistics* stats =
                GetMulticastSession(now,
                    m_SourceNodeId,
                    node->nodeId,
                    m_SessionId);

            if (!stats)
            {
                ERROR_ReportWarning("Cannot create and or retrieve "
                    "session stats");
                return;
            }
            stats->m_FragmentsReceived.AddDataPoint(now, 1);
            stats->m_AverageHopCount.AddDataPoint(now, timing->hopCount);

            stats->m_AverageDelay.AddDataPoint(
                now, (Float64) delay /SECOND);

            if (stats->m_LastDelay != -1)
            {
                clocktype d;

                if (delay > stats->m_LastDelay)
                {
                    d = delay - stats->m_LastDelay;
                }
                else
                {
                    d = stats->m_LastDelay - delay;
                }
                stats->m_AverageJitter.AddDataPoint(
                    now, (Float64) d / SECOND);
            }
            stats->m_LastDelay = delay;
        }

        // Auto refragment packet
        // NOTE: Does not currently support udp duplicates
        if (m_AutoRefragment)
        {
            // If this is a new message to refragment
            if (m_RefragmentBytesLeft == 0)
            {
                m_RefragmentBytesLeft = timing->messageSize;
                m_RefragmentBytes = timing->messageSize;
            }

            // If this is the end of a message to refragment
            m_RefragmentBytesLeft -= packetSize;
            if (m_RefragmentBytesLeft <= 0)
            {
                AddMessageReceivedDataPoints(
                    node, msg, 0, m_RefragmentBytes, 0, type);
                m_RefragmentBytes = 0;
            }
        }
    }
}

void STAT_AppStatistics::AddFragmentReceivedOutOfOrderDataPoints(
    Node* node,
    STAT_DestAddressType type)
{
    m_AddrStats[type].m_FragmentsReceivedOutOfOrder.AddDataPoint(
        node->getNodeTime(), 1);
}

void STAT_AppStatistics::AddFragmentReceivedDuplicateDataPoints(
    Node* node,
    STAT_DestAddressType type)
{
    m_AddrStats[type].m_DuplicateFragmentsReceived.AddDataPoint(
        node->getNodeTime(), 1);
}

void STAT_AppStatistics::AddMessageDroppedDataPoints(
    Node* node,
    STAT_DestAddressType type)
{
    m_AddrStats[type].m_MessagesDropped.AddDataPoint(node->getNodeTime(), 1);
}

void STAT_AppStatistics::SessionStart(Node* node, STAT_DestAddressType type)
{
    clocktype now = node->getNodeTime();
    GetThroughput(type).SessionStart(node->getNodeTime());
    GetOfferedLoad(type).SessionStart(node->getNodeTime());
    m_AddrStats[type].m_SessionStart.AddDataPoint(
        node->getNodeTime(), (double) node->getNodeTime() / SECOND);


    // If multicast, update effective stats
    if (type == STAT_Multicast)
    {
        if (m_StatsType != STAT_AppReceiver)
        {
#ifdef ADDON_DB
            AppData::APPL_MULTICAST_NODE_MAP* memberNodeMap = NULL;
            Int32 i;
            Int32 memberCount = STATSDB_MulticastMembershipCount(
                                    node,
                                    GetIPv4Address(m_DestAddr),
                                    &memberNodeMap);
            if (memberCount > 0)
            {
                AppData::APPL_MULTICAST_NODE_ITER nodeIt;
                for (nodeIt = memberNodeMap->begin(); 
                     nodeIt != memberNodeMap->end();
                     nodeIt++)
                {
                    //session start calls are handled in GetMulticastSession
                    STAT_MulticastAppSessionStatistics* stats =
                        GetMulticastSession(node->getNodeTime(),
                                            node->nodeId,
                                            nodeIt->first,
                                            m_SessionId);
                }
            }
#endif
        }
        else
        {
            STAT_MulticastAppSessionStatistics* stats =
                    GetMulticastSession(node->getNodeTime(),
                        m_SourceNodeId,
                        node->nodeId,
                        m_SessionId);
        }
    }
}

void STAT_AppStatistics::SessionFinish(
    Node* node,
    STAT_DestAddressType type)
{
    clocktype now = node->getNodeTime();
    GetThroughput(type).SessionFinish(now);
    GetOfferedLoad(type).SessionFinish(now);
    m_AddrStats[type].m_SessionFinish.AddDataPoint(
        node->getNodeTime(), (double) now / SECOND);
    
    MulticastAppSessionIterator it;
    for (it = m_multicastAppSessionStats.begin();
           it != m_multicastAppSessionStats.end();
           it++)
    {
        it->second->m_Throughput.SessionFinish(now);
        it->second->m_OfferedLoad.SessionFinish(now);
        it->second->m_SessionFinish.AddDataPoint(
            node->getNodeTime(), (double) now / SECOND);
    }
    
}

void STAT_AppStatistics::GetList(std::vector<STAT_Statistic*>& stats)
{
    stats.clear();

    // Do not return any stats if not initialized
    if (!m_Initialized)
    {
        return;
    }

    std::list<STAT_DestAddressType> types;
    STAT_DestAddressType type;
    std::list<STAT_DestAddressType>::iterator it;

    // Build list of supported address types
    if (m_Unicast)
    {
        types.push_back(STAT_Unicast);
    }
    if (m_Broadcast)
    {
        types.push_back(STAT_Broadcast);
    }
    if (m_Multicast)
    {
        types.push_back(STAT_Multicast);
    }

    // Get stats based on address types and sender/receiver
    for (it = types.begin(); it != types.end(); ++it)
    {
        type = *it;

        if (m_StatsType == STAT_AppSender)
        {
            stats.push_back(&GetSessionStart(type));
            stats.push_back(&GetSessionFinish(type));
            stats.push_back(&GetFirstFragmentSent(type));
            stats.push_back(&GetLastFragmentSent(type));
            stats.push_back(&GetFragmentsSent(type));
            stats.push_back(&GetFirstMessageSent(type));
            stats.push_back(&GetLastMessageSent(type));
            stats.push_back(&GetMessagesSent(type));
            stats.push_back(&GetDataSent(type));
            stats.push_back(&GetOverheadSent(type));
            stats.push_back(&GetOfferedLoad(type));
        }
        else if (m_StatsType == STAT_AppReceiver)
        {
            stats.push_back(&GetSessionStart(type));
            stats.push_back(&GetSessionFinish(type));
            stats.push_back(&GetFirstFragmentReceived(type));
            stats.push_back(&GetLastFragmentReceived(type));
            stats.push_back(&GetFragmentsReceived(type));
            stats.push_back(&GetFirstMessageReceived(type));
            stats.push_back(&GetLastMessageReceived(type));
            stats.push_back(&GetMessagesReceived(type));
            stats.push_back(&GetDataReceived(type));
            stats.push_back(&GetOverheadReceived(type));
            stats.push_back(&GetAverageDelay(type));
            stats.push_back(&GetThroughput(type));
            stats.push_back(&GetAverageJitter(type));
        }
        else
        {
            stats.push_back(&GetSessionStart(type));
            stats.push_back(&GetSessionFinish(type));
            stats.push_back(&GetFirstFragmentSent(type));
            stats.push_back(&GetLastFragmentSent(type));
            stats.push_back(&GetFirstFragmentReceived(type));
            stats.push_back(&GetLastFragmentReceived(type));
            stats.push_back(&GetFragmentsSent(type));
            stats.push_back(&GetFragmentsReceived(type));
            stats.push_back(&GetFirstMessageSent(type));
            stats.push_back(&GetLastMessageSent(type));
            stats.push_back(&GetFirstMessageReceived(type));
            stats.push_back(&GetLastMessageReceived(type));
            stats.push_back(&GetMessagesSent(type));
            stats.push_back(&GetMessagesReceived(type));
            stats.push_back(&GetDataSent(type));
            stats.push_back(&GetDataReceived(type));
            stats.push_back(&GetOverheadSent(type));
            stats.push_back(&GetOverheadReceived(type));
            stats.push_back(&GetAverageDelay(type));
            stats.push_back(&GetOfferedLoad(type));
            stats.push_back(&GetThroughput(type));
            stats.push_back(&GetJitter(type));
            stats.push_back(&GetAverageJitter(type));
            stats.push_back(&GetTotalJitter(type));
        }
    }
}

void STAT_GlobalAppStatistics::Initialize(
    PartitionData* partition,
    STAT_StatisticsList* stats)
{
    std::string name;
    std::string path;
    std::string description;
    std::string units;

    // Set names for all global statistics and add partition based
    // statistics to the global statistics.
    for (int i = 0; i < STAT_NUM_ADDRESS_TYPES; i++)
    {
        if (partition->partitionId == 0)
        {
            name = std::string("Total") + STAT_AddrToString(i) + std::string("ApplicationMessagesSent");
            path = "/stats/application/" + STAT_AddrToString(i) + "MessagesSent";
            description = "Total " + STAT_AddrToString(i) + " messages sent from the application layer";
            units = "messages";
            GetMessagesSent(i).SetInfo(
                name,
                description,
                units);
            GetMessagesSent(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetMessagesSent(i));

        if (partition->partitionId == 0)
        {
            name = std::string("Total") + STAT_AddrToString(i) + std::string("ApplicationMessagesReceived");
            path = "/stats/application/" + STAT_AddrToString(i) + "MessagesReceived";
            description = "Total " + STAT_AddrToString(i) + " messages received at the application layer";
            units = "messages";
            GetMessagesReceived(i).SetInfo(
                name,
                description,
                units);
            GetMessagesReceived(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetMessagesReceived(i));

        if (partition->partitionId == 0)
        {
            name = std::string("Total") + STAT_AddrToString(i) + std::string("ApplicationFragmentsSent");
            path = "/stats/application/" + STAT_AddrToString(i) + "FragmentsSent";
            description = "Total " + STAT_AddrToString(i) + " fragments sent from the application layer";
            units = "fragments";
            GetFragmentsSent(i).SetInfo(
                name,
                description,
                units);
            GetFragmentsSent(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetFragmentsSent(i));

        if (partition->partitionId == 0)
        {
            name = std::string("Total") + STAT_AddrToString(i) + std::string("ApplicationFragmentsReceived");
            path = "/stats/application/" + STAT_AddrToString(i) + "FragmentsReceived";
            description = "Total " + STAT_AddrToString(i) + " fragments received at the application layer";
            units = "fragments";
            GetFragmentsReceived(i).SetInfo(
                name,
                description,
                units);
            GetFragmentsReceived(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetFragmentsReceived(i));

        if (partition->partitionId == 0)
        {
            name = std::string("Total") + STAT_AddrToString(i) + std::string("ApplicationDataSent");
            path = "/stats/application/" + STAT_AddrToString(i) + "DataSent";
            description = "Total " + STAT_AddrToString(i) + " data sent from the application layer";
            units = "bytes";
            GetDataSent(i).SetInfo(
                name,
                description,
                units);
            GetDataSent(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetDataSent(i));

        if (partition->partitionId == 0)
        {
            name = std::string("Total") + STAT_AddrToString(i) + std::string("ApplicationDataReceived");
            path = "/stats/application/" + STAT_AddrToString(i) + "DataReceived";
            description = "Total " + STAT_AddrToString(i) + " data received at the application layer";
            units = "bytes";
            GetDataReceived(i).SetInfo(
                name,
                description,
                units);
            GetDataReceived(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetDataReceived(i));

        if (partition->partitionId == 0)
        {
            name = std::string("Total") + STAT_AddrToString(i) + std::string("ApplicationOverheadSent");
            path = "/stats/application/" + STAT_AddrToString(i) + "OverheadSent";
            description = "Total " + STAT_AddrToString(i) + " overhead sent from the application layer";
            units = "bytes";
            GetOverheadSent(i).SetInfo(
                name,
                description,
                units);
            GetOverheadSent(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetOverheadSent(i));

        if (partition->partitionId == 0)
        {
            name = std::string("Total") + STAT_AddrToString(i) + std::string("ApplicationOverheadReceived");
            path = "/stats/application/" + STAT_AddrToString(i) + "OverheadReceived";
            description = "Total " + STAT_AddrToString(i) + " overhead received at the application layer";
            units = "bytes";
            GetOverheadReceived(i).SetInfo(
                name,
                description,
                units);
            GetOverheadReceived(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetOverheadReceived(i));

        if (partition->partitionId == 0)
        {
            name = std::string("Average") + STAT_AddrToString(i) + std::string("ApplicationDelay");
            path = "/stats/application/" + STAT_AddrToString(i) + "AverageDelay";
            description = "Average " + STAT_AddrToString(i) + " delay at the application layer";
            units = "s";
            GetAverageDelay(i).SetInfo(
                name,
                description,
                units);
            GetAverageDelay(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetAverageDelay(i));
        // Uncomment next 2 lines to build average delay from dynamic hieararchy
        //path = "node/*/*/*/averageDelay";
        //m_AverageDelay.SetIteratorPath(path);

        if (partition->partitionId == 0)
        {
            name = std::string("Total") + STAT_AddrToString(i) + std::string("ApplicationOfferedLoad");
            path = "/stats/application/" + STAT_AddrToString(i) + "OfferedLoad";
            description = "Total " + STAT_AddrToString(i) + " offered load at the application layer";
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
            name = std::string("Total") + STAT_AddrToString(i) + std::string("ApplicationThroughput");
            path = "/stats/application/" + STAT_AddrToString(i) + "Throughput";
            description = "Total " + STAT_AddrToString(i) + " throughput received at the application layer";
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
            name = std::string("Average") + STAT_AddrToString(i) + std::string("ApplicationJitter");
            path = "/stats/application/" + STAT_AddrToString(i) + "AverageJitter";
            description = "Average " + STAT_AddrToString(i) + " jitter at the application layer";
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
            name = std::string("Average") + STAT_AddrToString(i) + std::string("ApplicationHopCount");
            path = "/stats/application/" + STAT_AddrToString(i) + "AverageHopCount";
            description = "Average " + STAT_AddrToString(i) + " hop count at the application layer";
            units = "hops";
            GetAverageHopCount(i).SetInfo(
                name,
                description,
                units);
            GetAverageHopCount(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetAverageHopCount(i));

        if (partition->partitionId == 0)
        {
            name = std::string("Total") + STAT_AddrToString(i) + std::string("ApplicationJitter");
            path = "/stats/application/" + STAT_AddrToString(i) + "TotalJitter";
            description = "Total " + STAT_AddrToString(i) + " jitter at the application layer";
            units = "s";
            GetTotalJitter(i).SetInfo(
                name,
                description,
                units);
            GetTotalJitter(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetTotalJitter(i));

        // Build MCR
        if (partition->partitionId == 0)
        {
            name = STAT_AddrToString(i) + std::string("ApplicationMessagesCompletionRate");
            path = "/stats/application/" + STAT_AddrToString(i) + "MessageCompletionRate";
            description = STAT_AddrToString(i) + " message completion rate at the application layer";
            units = "%/100";
            GetMessageCompletionRate(i).SetInfo(
                name,
                description,
                units);
            GetMessageCompletionRate(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        GetMessageCompletionRate(i).AddStatistic(&GetMessagesReceived(i));
        if (i == STAT_Unicast)
        {
            GetMessageCompletionRate(i).AddStatistic(&GetMessagesSent(i));
        }
        else
        {
            GetMessageCompletionRate(i).AddStatistic(&GetAggregatedEffectiveMessagesSent(i));
        }
        stats->RegisterAggregatedStatistic(&GetMessageCompletionRate(i));

       // Effective  stats
       if (partition->partitionId == 0)
       {
            name = "TotalEffective" + STAT_AddrToString(i) + "FragmentsSent";
            path = "/stats/application/totalEffective" + STAT_AddrToString(i) + "FragmentsSent";
            description = STAT_AddrToString(i) + " total effective fragments sent at the application layer";
            units = "fragments";
            GetAggregatedEffectiveFragmentsSent(i).SetInfo(
                name,
                description,
                units);
            GetAggregatedEffectiveFragmentsSent(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetAggregatedEffectiveFragmentsSent(i));

        if (partition->partitionId == 0)
        {
            name = "TotalEffective" + STAT_AddrToString(i) + "MessagesSent";
            path = "/stats/application/totalEffective" + STAT_AddrToString(i) + "MessagesSent";
            description = STAT_AddrToString(i) + " total effective messages sent at the application layer";
            units = "messages";
            GetAggregatedEffectiveMessagesSent(i).SetInfo(
                name,
                description,
                units);
            GetAggregatedEffectiveMessagesSent(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetAggregatedEffectiveMessagesSent(i));

        if (partition->partitionId == 0)
        {
            name = "TotalEffective" + STAT_AddrToString(i) + "DataSent";
            path = "/stats/application/totalEffective" + STAT_AddrToString(i) + "DataSent";
            description = STAT_AddrToString(i) + " total effective data sent at the application layer";
            units = "bytes";
            GetAggregatedEffectiveDataSent(i).SetInfo(
                name,
                description,
                units);
            GetAggregatedEffectiveDataSent(i).AddToHierarchy(&partition->dynamicHierarchy, path);
        }
        stats->RegisterAggregatedStatistic(&GetAggregatedEffectiveDataSent(i));
    }
}

void STAT_AppSummaryStatistics::InitializeFromModel(
    Node* node,
    STAT_AppStatistics* stats)
{
    clocktype now = node->getNodeTime();
    STAT_DestAddressType addrType;

    if (stats->GetIsSource())
    {
        numSenders = 1;
        numReceivers = 0;
    }
    else
    {
        numSenders = 0;
        numReceivers = 1;
    }

    if (stats->GetMulticast())
    {
        addrType = STAT_Multicast;
    }
    else if (stats->GetBroadcast())
    {
        addrType = STAT_Broadcast;
    }
    else
    {
        addrType = STAT_Unicast;
    }

    memset(type, 0, MAX_STRING_LENGTH);
    memset(name, 0, MAX_STRING_LENGTH);

    strcpy(type, stats->GetType().c_str());
    strcpy(name, stats->GetCustomName().c_str());
    
    tos = (UInt8)stats->getTos();
    sessionId = stats->GetSessionId();
    senderId = stats->GetSourceNodeId();
    receiverId = stats->GetDestNodeId();
    receiverAddress = GetIPv4Address(stats->GetDestAddr());

    messagesSent = stats->GetMessagesSent(addrType).GetValue(now);
    messagesReceived = stats->GetMessagesReceived(addrType).GetValue(now);
    fragmentsSent = stats->GetFragmentsSent(addrType).GetValue(now);
    fragmentsReceived = stats->GetFragmentsReceived(addrType).GetValue(now);
    dataSent = stats->GetDataSent(addrType).GetValue(now);
    dataReceived = stats->GetDataReceived(addrType).GetValue(now);
    overheadSent = stats->GetOverheadSent(addrType).GetValue(now);
    overheadReceived = stats->GetOverheadReceived(addrType).GetValue(now);
    offeredLoad = stats->GetOfferedLoad(addrType).GetValue(now);
    throughput = stats->GetThroughput(addrType).GetValue(now);
    numJitterDataPoints = stats->GetAverageJitter(addrType).GetNumDataPoints();
    averageDelay = stats->GetAverageDelay(addrType).GetValue(now);
    averageJitter = stats->GetAverageJitter(addrType).GetValue(now);
    averageHopCount = stats->GetAverageHopCount(addrType).GetValue(now);
    effectiveMessagesSent = stats->GetEffectiveMessagesSent(addrType).GetValue(now);
    effectiveDataSent = stats->GetEffectiveDataSent(addrType).GetValue(now);
    effectiveFragmentsSent = stats->GetEffectiveFragmentsSent(addrType).GetValue(now);
}

STAT_AppSummaryStatistics& STAT_AppSummaryStatistics::operator +=(
    const STAT_AppSummaryStatistics& stats)
{
    // Update sender and receiver variables
    numSenders += stats.numSenders;
    numReceivers += stats.numReceivers;

    if (name[0] == '\0' && stats.name[0] != '\0')
    {
        memcpy(name, stats.name, strlen(stats.name));
    }

    if (stats.numSenders > 0)
    {
        tos = stats.tos;
    }
    else
    {
        receiverId = stats.receiverId;
    }

    // Update averages if some fragments were received.  Stats will be 0
    // if no fragments were received.
    if (stats.fragmentsReceived > 0 || fragmentsReceived > 0)
    {
        averageDelay = (averageDelay * fragmentsReceived
            + stats.averageDelay * stats.fragmentsReceived)
            / (fragmentsReceived + stats.fragmentsReceived);
        averageHopCount = (averageHopCount * fragmentsReceived
            + stats.averageHopCount * stats.fragmentsReceived)
            / (fragmentsReceived + stats.fragmentsReceived);
    }
    if (stats.numJitterDataPoints > 0 || numJitterDataPoints > 0)
    {
        averageJitter = (averageJitter * numJitterDataPoints
            + stats.averageJitter * stats.numJitterDataPoints)
            / (numJitterDataPoints + stats.numJitterDataPoints);
        numJitterDataPoints += stats.numJitterDataPoints;
    }

    messagesSent += stats.messagesSent;
    effectiveMessagesSent += stats.effectiveMessagesSent;
    messagesReceived += stats.messagesReceived;
    fragmentsSent += stats.fragmentsSent;
    effectiveFragmentsSent += stats.effectiveFragmentsSent;
    fragmentsReceived += stats.fragmentsReceived;
    dataSent += stats.dataSent;
    effectiveDataSent += stats.effectiveDataSent;
    dataReceived += stats.dataReceived;
    overheadSent += stats.overheadSent;
    overheadReceived += stats.overheadReceived;
    offeredLoad += stats.offeredLoad;
    throughput += stats.throughput;

    return *this;
}

STAT_MulticastAppSessionStatistics::STAT_MulticastAppSessionStatistics(
    NodeId senderId, NodeId destNodeId, STAT_SessionIdType sessionId)
{
    m_senderId = senderId;
    m_DestNodeId = destNodeId;
    m_SessionId = sessionId;

    m_LastDelay = -1;
}


STAT_MulticastAppSessionStatistics* STAT_AppStatistics::GetMulticastSession(clocktype time,
    NodeId& senderId, NodeId destNodeId, STAT_SessionIdType sessionId)
{
    STAT_MulticastAppSessionKey key;
    key.m_senderId = senderId;
    key.m_destId = destNodeId;
    key.m_SessionId = sessionId;

    MulticastAppSessionIterator it = m_multicastAppSessionStats.find(key);

    if (it == m_multicastAppSessionStats.end())
    {
        // Not found, insert new statistics and return
        STAT_MulticastAppSessionStatistics* stat =
            new STAT_MulticastAppSessionStatistics(senderId, destNodeId, sessionId);
        m_multicastAppSessionStats[key] = stat;

        if (IsSessionStarted(STAT_Multicast))
        {
            stat->m_Throughput.SessionStart(time);
            stat->m_OfferedLoad.SessionStart(time);
            stat->m_SessionStart.AddDataPoint(
                time, (double) time / SECOND);
        }
        return m_multicastAppSessionStats[key];
    }
    else
    {
        return it->second;
    }
}

void STAT_MulticastAppSessionSummaryStatistics::InitializeFromModel(
    clocktype now,
    STAT_MulticastAppSessionStatistics* stats,
    STAT_AppStatistics* parentStats)
{
    strcpy(type, parentStats->GetType().c_str());
    strcpy(name, parentStats->GetCustomName().c_str());

    tos = (UInt8)parentStats->getTos();
    m_SessionId = stats->GetSessionId();
    m_senderId = stats->GetSenderId();
    m_DestNodeId = stats->GetDestNodeId();
    receiverAddress = GetIPv4Address(parentStats->GetDestAddr());

    m_messagesSent = stats->GetMessagesSent().GetValue(now);
    m_messagesReceived = stats->GetMessagesReceived().GetValue(now);
    m_bytesSent = stats->GetBytesSent().GetValue(now);
    m_bytesReceived = stats->GetBytesReceived().GetValue(now);
    m_fragmentsReceived = stats->GetFragmentsReceived().GetValue(now);
    m_fragmentsSent = stats->GetFragmentsSent().GetValue(now);

    m_averageDelay = stats->GetAverageDelay().GetValue(now);
    m_offerload = stats->GetOfferload().GetValue(now);
    m_throughput = stats->GetThroughput().GetValue(now);
    m_avgJitter = stats->GetAverageJitter().GetValue(now);
    m_numJitterPoints = stats->GetAverageJitter().GetNumDataPoints();
    m_avgHopCount = stats->GetAverageHopCount().GetValue(now);
}

STAT_MulticastAppSessionSummaryStatistics& STAT_MulticastAppSessionSummaryStatistics::operator +=(
    const STAT_MulticastAppSessionSummaryStatistics& stats)
{
    m_messagesSent += stats.m_messagesSent;
    m_messagesReceived += stats.m_messagesReceived;
    m_bytesSent += stats.m_bytesSent;
    m_bytesReceived += stats.m_bytesReceived;
    m_fragmentsReceived += stats.m_fragmentsReceived;
    m_fragmentsSent += stats.m_fragmentsSent;

    m_offerload += stats.m_offerload;
    m_throughput += stats.m_throughput;

    if (stats.m_fragmentsReceived > 0 || m_fragmentsReceived > 0)
    {
        m_averageDelay = (m_averageDelay * m_fragmentsReceived
            + stats.m_averageDelay * stats.m_fragmentsReceived)
            / (m_fragmentsReceived + stats.m_fragmentsReceived);
        m_avgHopCount = (m_avgHopCount * m_fragmentsReceived
            + stats.m_avgHopCount * stats.m_fragmentsReceived)
            / (m_fragmentsReceived + stats.m_fragmentsReceived);
    }
    if (stats.m_numJitterPoints > 0 || m_numJitterPoints > 0)
    {
        m_avgJitter = (m_avgJitter * m_numJitterPoints
            + stats.m_avgJitter * stats.m_numJitterPoints)
            / (m_numJitterPoints + stats.m_numJitterPoints);
        m_numJitterPoints += stats.m_numJitterPoints;
    }

    return *this;
}
