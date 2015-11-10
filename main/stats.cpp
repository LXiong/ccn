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

#include "api.h"
#include "partition.h"
#include "stats.h"
#include "stats_global.h"
#include "network_ip.h"
#include "WallClock.h"

// Macro to turn an address type into a string
std::string STAT_AddrToString(STAT_DestAddressType type)
{
    if (type == STAT_Unicast)
    {
        return std::string("Unicast");
    }
    else if (type == STAT_Broadcast)
    {
        return std::string("Broadcast");
    }
    else if (type == STAT_Multicast)
    {
        return std::string("Multicast");
    }
    else
    {
        ERROR_ReportError("Not a valid address type");
        return std::string(""); // To remove warning
    }
}

STAT_DestAddressType STAT_NodeAddressToDestAddressType(Node* node, NodeAddress& addr)
{
    // Check for broadcast address or subnet broadcast
    if (addr == ANY_DEST)
    {
        return STAT_Broadcast;
    }
    for (int i = 0; i < node->numberInterfaces; i++)
    {
        if (addr == NetworkIpGetInterfaceBroadcastAddress(node, i))
        {
            return STAT_Broadcast;
        }
    }
    
    if (NetworkIpIsMulticastAddress(node, addr))
    {
        return STAT_Multicast;
    }
    else
    {
        return STAT_Unicast;
    }
}

STAT_DestAddressType STAT_AddressToDestAddressType(Node* node, const Address& addr)
{
    NodeAddress a = GetIPv4Address(addr);
    return STAT_NodeAddressToDestAddressType(node, a);
}

void STAT_Timing::Initialize()
{
    sourceNodeId = ANY_NODEID;
    uniqueSessionId = STAT_UNDEFINED;
    messageSize = STAT_UNDEFINED;
    sentApplicationDown = STAT_UNDEFINED;
    receivedTransportUp = STAT_UNDEFINED;
    sentTransportDown = STAT_UNDEFINED;
    sourceSentNetworkDown = STAT_UNDEFINED;
    leftMacOutputQueue = STAT_UNDEFINED;
    sentMacDown = STAT_UNDEFINED;
    sentPhyDown = STAT_UNDEFINED;
    hopCount = 0;
    lastHopNodeId = ANY_NODEID;
    lastHopIntfIndex = ANY_INTERFACE;
}

STAT_Timing* STAT_GetTiming(Node* node, Message* message)
{
    STAT_Timing* timing = (STAT_Timing*) MESSAGE_ReturnInfo(message, INFO_TYPE_StatsTiming);
    if (timing == NULL)
    {
        MESSAGE_AddInfo(
            node,
            message,
            sizeof(STAT_Timing),
            INFO_TYPE_StatsTiming);
        timing = (STAT_Timing*) MESSAGE_ReturnInfo(
            message,
            INFO_TYPE_StatsTiming);
        timing->Initialize();
    }

    return timing;
}

void STAT_Statistic::SetInfo(
    std::string& name,
    std::string& description,
    std::string& units,
    STAT_Format format)
{
    m_Name = STAT_StatisticsList::CacheString(name);
    m_Description = STAT_StatisticsList::CacheString(description);
    m_Units = STAT_StatisticsList::CacheString(units);

    if (format == STAT_GuessFormat)
    {
        // Attempt to guess the format
        if (units == std::string("signals")
            || units == std::string("frames")
            || units == std::string("bytes")
            || units == std::string("packets")
            || units == std::string("segments")
            || units == std::string("fragments")
            || units == std::string("messages"))
        {
            // X/second, hops
            m_Format = STAT_Integer;
        }
        else if (units == std::string("seconds"))
        {
            // seconds
            m_Format = STAT_Time;
        }
        else
        {
            // Anything else is float
            m_Format = STAT_Float;
        }
    }
    else
    {
        m_Format = format;
    }
}

void STAT_Statistic::AddToHierarchy(D_Hierarchy* h, std::string& path)
{
    // No path if statistic is not dynamically enabled
    if (h->IsEnabled())
    {
        m_Value.SetStatistic(this);
        try
        {
            h->CreatePath(path);
            h->AddObject(
                path,
                new D_StatisticObj(&m_Value),
                GetDescription());
        }
        catch (D_Exception& e)
        {
            std::string err;
            e.GetFullErrorString(err);
            ERROR_ReportError((char*) err.c_str());
        }
    }
}

void STAT_Statistic::AddDataPoint(clocktype now, double val)
{
    ERROR_ReportError("Not implemented");
}

Int64 STAT_Statistic::GetValueAsInt(clocktype now)
{
    Float64 doubleVal = GetValue(now);
    Float64 doubleValFloor = floor(doubleVal);

    if (doubleVal - doubleValFloor >= 0.5)
    {
        return (Int64) (doubleValFloor) + 1;
    }
    else
    {
        return (Int64) doubleValFloor;
    }
}

std::string STAT_Statistic::GetValueAsString(clocktype now)
{
    char buf[MAX_STRING_LENGTH];

    if (m_Format == STAT_Float)
    {
        sprintf(buf, "%f", GetValue(now));
    }
    else if (m_Format == STAT_Time)
    {
        sprintf(buf, "%.9f", GetValue(now));
    }
    else
    {
        sprintf(buf, "%.0f", GetValue(now));
    }

    return std::string(buf);
}

clocktype STAT_Statistic::GetValueAsClocktype(clocktype now)
{
    return GetValueAsInt(now);
}

void STAT_Statistic::Reset()
{
    m_Value = 0.0;
    m_NumDataPoints = 0;
}

void STAT_Statistic::Serialize(clocktype now, char** data, int* size)
{
    *data = (char*) MEM_malloc(sizeof(STAT_SimpleAggregationData));
    *size = sizeof(STAT_SimpleAggregationData);
    STAT_SimpleAggregationData* agg = (STAT_SimpleAggregationData*) *data;
    agg->m_Value = m_Value;
    agg->m_NumDataPoints = m_NumDataPoints;
}

void STAT_Statistic::DeserializeAndUpdate(
    clocktype now,
    char* data,
    int size)
{
    ERROR_Assert(
        size == sizeof(STAT_SimpleAggregationData),
        "Wrong size for statistic");

    STAT_SimpleAggregationData* agg = (STAT_SimpleAggregationData*) data;
    m_Value = agg->m_Value;
    m_NumDataPoints = agg->m_NumDataPoints;
}

void STAT_ModelStatistics::Initialize(Node* node)
{
    node->partitionData->stats->AddModelStatistics(this);
}

void STAT_ModelStatistics::Print(
    Node* node,
    const char* layer,
    const char* model,
    NodeAddress addr,
    int instanceId)
{
    char instanceStr[MAX_STRING_LENGTH];

    if (instanceId >= 0)
    {
        sprintf(instanceStr, "[%d]", instanceId);
    }
    else
    {
        strcpy(instanceStr, "");
    }
    Print(node, layer, model, addr, instanceStr);
}

void STAT_ModelStatistics::Print(
    Node* node,
    const char* layer,
    const char* model,
    NodeAddress addr,
    const std::string& instanceId)
{
    char ipAddrString[MAX_STRING_LENGTH];
    char instanceStr[MAX_STRING_LENGTH];
    clocktype now = node->getNodeTime();
    STAT_Statistic* stat;

    std::vector<STAT_Statistic*> stats;
    GetList(stats);

    if (addr != ANY_DEST)
    {
        IO_ConvertIpAddressToString(addr, ipAddrString);
    }
    else
    {
        strcpy(ipAddrString, "");
    }

    if (instanceId.size() > 0)
    {
        sprintf(instanceStr, "%s", instanceId.c_str());
    }
    else
    {
        strcpy(instanceStr, "");
    }

    for (unsigned int i = 0; i < stats.size(); i++)
    {
        // find the address string length
        fprintf(
            node->partitionData->statFd,
            "%4u,%15s,%4s,%12s,%12s,",
            node->nodeId,
            ipAddrString,
            instanceStr,
            layer,
            model);

        // Format according to type
        stat = stats[i];
        fprintf(node->partitionData->statFd, "%s (%s) = %s",
            stat->GetDescription().c_str(),
            stat->GetUnits().c_str(),
            stat->GetValueAsString(now).c_str());

        fprintf(node->partitionData->statFd, "\n");
    }
}
void STAT_ModelStatistics::Print(
    Node* node,
    const char* layer,
    const char* model,
    const char* ipAddrString,
    int instanceId)
{
    char instanceStr[MAX_STRING_LENGTH];
    clocktype now = node->getNodeTime();
    STAT_Statistic* stat;

    std::vector<STAT_Statistic*> stats;
    GetList(stats);

    if (instanceId >= 0)
    {
        sprintf(instanceStr, "[%d]", instanceId);
    }
    else
    {
        strcpy(instanceStr, "");
    }
    
    for (unsigned int i = 0; i < stats.size(); i++)
    {
        // find the address string length
        fprintf(
            node->partitionData->statFd,
            "%4u,%41s,%4s,%12s,%12s,",
            node->nodeId,
            ipAddrString,
            instanceStr,
            layer,
            model);

        // Format according to type
        stat = stats[i];
        fprintf(node->partitionData->statFd, "%s (%s) = %s",
            stat->GetDescription().c_str(),
            stat->GetUnits().c_str(),
            stat->GetValueAsString(now).c_str());

        fprintf(node->partitionData->statFd, "\n");
    }
}


void STAT_ModelStatistics::Finalize(Node* node)
{
    ERROR_Assert(
        !m_IsFinal,
        "Model statistics have already been finalized");

    // Immediately delete the model's statistics if
    // STATS-DELETE-WHEN-FINALIZED is YES.  Otherwise add to list of
    // finalized models.
    if (node->partitionData->stats->DeleteFinalizedModelStatistics())
    {
        node->partitionData->stats->RemoveModelStatistics(this);
        delete this;
    }
    else
    {
        m_IsFinal = TRUE;
        //node->partitionData->stats->AddFinalizedModel(this);
    }
}

STAT_ModelStatistics::~STAT_ModelStatistics()
{
    // Finalized model stats should not be deleted unless it is by the kernel
    ERROR_Assert(!m_IsFinal, "Model statistics have already been finalized");
}

void STAT_Maximum::AddDataPoint(clocktype now, double val)
{
    if (val > m_Value)
    {
        m_Value = val;
    }
    m_NumDataPoints++;
}

void STAT_Minimum::AddDataPoint(clocktype now, double val)
{
    if (val < m_Value || m_NumDataPoints == 0)
    {
        m_Value = val;
    }
    m_NumDataPoints++;
}

void STAT_Sum::AddDataPoint(clocktype now, double val)
{
    m_Value += val;
    m_NumDataPoints++;
}

void STAT_Average::AddDataPoint(clocktype now, double val)
{
    m_Value += val;
    m_NumDataPoints++;
}
Float64 STAT_Average::GetValue(clocktype now)
{
    if (m_NumDataPoints == 0)
    {
        return 0;
    }
    else
    {
        return m_Value / m_NumDataPoints;
    }
}
void STAT_Average_dBm::AddDataPoint(clocktype now, double val)
{
    // val is expected to be in mW
    m_Value += val;
    m_NumDataPoints++;
}
Float64 STAT_Average_dBm::GetValue(clocktype now)
{
    if (m_Value == 0)
    {
        return -120;
    }
    else
    {
        return IN_DB(m_Value / m_NumDataPoints);
    }
}
void STAT_SessionFinish::AddDataPoint(clocktype now, double val)
{
    if (m_Value == 0.0 || val < m_Value)
    {
        m_Value = val;
    }
    m_NumDataPoints = 1;
}

STAT_Throughput::STAT_Throughput()
{
    Reset();
}

void STAT_Throughput::AddDataPoint(clocktype now, double val)
{
    // Start session if not started yet
    if (m_SessionStart == CLOCKTYPE_MAX)
    {
        SessionStart(now);
    }
    
    m_BytesProcessed += val;
    m_NumDataPoints++;
}

Float64 STAT_Throughput::GetValue(clocktype now)
{
    if (m_SessionStart == CLOCKTYPE_MAX || m_NumDataPoints <= 1)
    {
        return 0.0;
    }
    if (m_SessionFinish <= m_SessionStart)
    {
        return 0.0;
    }

    // If session has not ended use current time
    Float64 end = m_SessionFinish;
    if (m_SessionFinish == CLOCKTYPE_MAX)
    {
        end = (Float64) now / SECOND;
    }

    return (m_BytesProcessed * 8.0) / (end - m_SessionStart);
}

Float64 STAT_Throughput::GetValue(clocktype now, Float64 bytesProcessed)
{
    if (m_SessionStart == CLOCKTYPE_MAX || m_NumDataPoints <= 1)
    {
        return 0.0;
    }
    if (m_SessionFinish <= m_SessionStart)
    {
        return 0.0;
    }

    // If session has not ended use current time
    Float64 end = m_SessionFinish;
    if (m_SessionFinish == CLOCKTYPE_MAX)
    {
        end = (Float64) now / SECOND;
    }

    return (bytesProcessed * 8.0) / (end - m_SessionStart);
}

void STAT_Throughput::Reset()
{
    m_BytesProcessed = 0;
    m_SessionStart = (Float64)CLOCKTYPE_MAX;
    m_SessionFinish = (Float64)CLOCKTYPE_MAX;
}

void STAT_Throughput::SessionStart(clocktype now)
{
    if (m_SessionStart != CLOCKTYPE_MAX)
    {
        ERROR_ReportWarning("session started multiple times");
        return;
    }

    m_SessionStart = (Float64) now / SECOND;
}

void STAT_Throughput::SessionFinish(clocktype now)
{
    if (m_SessionStart == CLOCKTYPE_MAX)
    {
        // Never sent a packet, leave end as clocktype_max
        return;
    }
    if (m_SessionFinish != CLOCKTYPE_MAX)
    {
        // Commented out until STAT API is completely integrated
        //ERROR_ReportWarning("session end multiple times");
        return;
    }

    m_SessionFinish = (Float64) now / SECOND;
}

STAT_Utilization::STAT_Utilization()
{
    Reset();
}

void STAT_Utilization::AddDataPoint(clocktype now, double val)
{
    // Start session if not started yet
    if (m_SessionStart == CLOCKTYPE_MAX)
    {
        SessionStart(now);
    }
    
    m_ActivityTime += val;
    m_NumDataPoints++;
}

Float64 STAT_Utilization::GetValue(clocktype now)
{
    if (m_SessionStart == CLOCKTYPE_MAX)
    {
        return 0.0;
    }

    // If session has not ended use current time
    clocktype end = m_SessionFinish;
    if (m_SessionFinish == CLOCKTYPE_MAX)
    {
        end = now;
    }

    if (end <= m_SessionStart)
    {
        return 0.0;
    }

    return m_ActivityTime * SECOND / (end - m_SessionStart);
}

void STAT_Utilization::Reset()
{
    m_ActivityTime = 0;
    m_SessionStart = CLOCKTYPE_MAX;
    m_SessionFinish = CLOCKTYPE_MAX;
}

void STAT_Utilization::SessionStart(clocktype now)
{
    if (m_SessionStart != CLOCKTYPE_MAX)
    {
        ERROR_ReportWarning("session started multiple times");
        return;
    }

    m_SessionStart = now;
}

void STAT_Utilization::SessionFinish(clocktype now)
{
    if (m_SessionStart == CLOCKTYPE_MAX)
    {
        // Never sent a packet, leave end as clocktype_max
        return;
    }
    if (m_SessionFinish != CLOCKTYPE_MAX)
    {
        ERROR_ReportWarning("session end multiple times");
        return;
    }

    m_SessionFinish = now;
}

void STAT_SmoothedAverage::AddDataPoint(clocktype now, double val)
{
    m_Value += (val - m_Value) / m_SmoothingFactor;
    m_NumDataPoints++;
}

void STAT_TimeWeightedAverage::SetLastChange(clocktype creationTime)
{
    m_timeOfLastChange = creationTime;
}

void STAT_TimeWeightedAverage::AddDataPoint(clocktype time, double val)
{
    m_Value += ((double) (time - m_timeOfLastChange) / SECOND) * val;
    m_timeOfLastChange = time;
    m_NumDataPoints++;
}

Float64 STAT_TimeWeightedAverage::GetValue(clocktype now)
{
    if (m_NumDataPoints == 0)
    {
        return 0;
    }
    return m_Value / ((double) now / SECOND);
}

STAT_VectorIterator STAT_AggregatedStatistic::FindStatistic(
    STAT_Statistic* stat)
{
    return find(m_Statistics.begin(), m_Statistics.end(), stat);
}

void STAT_AggregatedStatistic::SetIteratorPath(std::string& path)
{
    if (m_Statistics.size() > 0)
    {
        ERROR_ReportError(
            "Aggregated statistics path may not be used with AddStatistic");
    }

    m_IteratorPath = path;
}

void STAT_AggregatedStatistic::RebuildIterator(D_Hierarchy* h)
{
    m_Statistics.clear();

    // Set up a hierarchy iterator over all statistics
    D_ObjectIterator it(h, m_IteratorPath);
    it.SetType(D_STATISTIC);

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

        // Add the statistic
        AddStatistic(stat);
    }
}

void STAT_AggregatedStatistic::AddStatistic(STAT_Statistic* stat)
{
    if (FindStatistic(stat) != m_Statistics.end())
    {
        ERROR_ReportWarning(
            "Statistic already added to "
            "aggregated statistic");
        return;
    }

    // If adding an aggregated statistic make sure the order is less
    // NOTE: Order might not be satisfied is statistics are added to a
    //      subordinate statistic at a later time.
    STAT_AggregatedStatistic* agg =
        dynamic_cast<STAT_AggregatedStatistic*>(stat);
    if (agg)
    {
        if (agg->GetOrder() >= m_Order)
        {
            m_Order = agg->GetOrder() + 1;
        }
    }

    m_Statistics.push_back(stat);
}

void STAT_AggregatedStatistic::RemoveStatistic(STAT_Statistic* stat)
{
    STAT_VectorIterator it = FindStatistic(stat);

    if (it == m_Statistics.end())
    {
        ERROR_ReportWarning("Statistic not found in aggregated statistic");
        return;
    }

    m_Statistics.erase(it);
}

void STAT_AggregatedSum::Aggregate(clocktype now)
{
    STAT_VectorIterator it;
    m_Value = 0.0;
    m_NumDataPoints = 0;
    Float64 sum = 0.0;

    for (it = m_Statistics.begin(); it != m_Statistics.end(); it++)
    {
        sum += (**it).GetValue(now);
        m_NumDataPoints += (**it).GetNumDataPoints();
    }

    // Set the value only once incase value is listened to
    m_Value = sum;
}

void STAT_AggregatedSum::Serialize(clocktype now, char** data, int* size)
{
    *data = (char*) MEM_malloc(sizeof(STAT_SimpleAggregationData));
    *size = sizeof(STAT_SimpleAggregationData);
    STAT_SimpleAggregationData* agg = (STAT_SimpleAggregationData*) *data;
    agg->m_Value = m_Value;
    agg->m_NumDataPoints = m_NumDataPoints;
}

void STAT_AggregatedSum::DeserializeAndUpdate(
    clocktype now,
    char* data,
    int size)
{
    ERROR_Assert(
        size == sizeof(STAT_SimpleAggregationData),
        "Wrong size for aggregated sum");

    STAT_SimpleAggregationData* agg = (STAT_SimpleAggregationData*) data;
    m_Value += agg->m_Value;
    m_NumDataPoints += agg->m_NumDataPoints;
}

void STAT_AggregatedAverage::Aggregate(clocktype now)
{
    STAT_VectorIterator it;
    m_Value = 0.0;
    m_NumDataPoints = 0;
    Float64 average = 0.0;

    // Count num statistics and data points
    m_NumStatistics = 0;
    for (it = m_Statistics.begin(); it != m_Statistics.end(); it++)
    {
        if ((**it).GetNumDataPoints() > 0)
        {
            m_NumStatistics++;
            m_NumDataPoints += (**it).GetNumDataPoints();
        }
    }

    // Now compute average.  Done this way to reduce chance of overflow.
    if (m_NumStatistics > 0)
    {
        for (it = m_Statistics.begin(); it != m_Statistics.end(); it++)
        {
            average += (**it).GetValue(now) / m_NumStatistics;
        }
    }

    // Set the value only once incase value is listened to
    m_Value = average;
}

void STAT_AggregatedAverage::Serialize(
    clocktype now,
    char** data,
    int* size)
{
    *data = (char*) MEM_malloc(sizeof(STAT_AggregationData));
    *size = sizeof(STAT_AggregationData);
    STAT_AggregationData* agg = (STAT_AggregationData*) *data;
    agg->m_Value = m_Value;
    agg->m_Value2 = m_NumStatistics;
    agg->m_NumDataPoints = m_NumDataPoints;
}

void STAT_AggregatedAverage::DeserializeAndUpdate(
    clocktype now,
    char* data,
    int size)
{
    ERROR_Assert(
        size == sizeof(STAT_AggregationData),
        "Wrong size for aggregated average");

    STAT_AggregationData* agg = (STAT_AggregationData*) data;

    Int64 totalStatistics = m_NumStatistics + agg->m_Value2;
    if (totalStatistics > 0)
    {
        m_Value = m_Value * ((double) m_NumStatistics / totalStatistics)
            + agg->m_Value * ((double) agg->m_Value2 / totalStatistics);
        m_NumDataPoints += agg->m_NumDataPoints;
    }
}

void STAT_AggregatedWeightedDataPointAverage::Aggregate(clocktype now)
{
    m_NumDataPoints = 0;
    Float64 average = 0.0;

    if (m_Statistics.size() > 0)
    {
        STAT_VectorIterator it;

        // First sum up total number of data points
        for (it = m_Statistics.begin(); it != m_Statistics.end(); it++)
        {
            m_NumDataPoints += (**it).GetNumDataPoints();
        }

        if (m_NumDataPoints == 0)
        {
            return;
        }

        // Next sum individual stats
        for (it = m_Statistics.begin(); it != m_Statistics.end(); it++)
        {
            average += (**it).GetValue(now)
                * ((double) (**it).GetNumDataPoints() / m_NumDataPoints);
        }
    }

    // Set the value only once incase value is listened to
    m_Value = average;
}

void STAT_AggregatedWeightedDataPointAverage::Serialize(
    clocktype now,
    char** data,
    int* size)
{
    *data = (char*) MEM_malloc(sizeof(STAT_SimpleAggregationData));
    *size = sizeof(STAT_SimpleAggregationData);
    STAT_SimpleAggregationData* agg = (STAT_SimpleAggregationData*) *data;
    agg->m_Value = m_Value;
    agg->m_NumDataPoints = m_NumDataPoints;
}

void STAT_AggregatedWeightedDataPointAverage::DeserializeAndUpdate(
    clocktype now,
    char* data,
    int size)
{
    ERROR_Assert(
        size == sizeof(STAT_SimpleAggregationData),
        "Wrong size for aggregated average");

    STAT_SimpleAggregationData* agg = (STAT_SimpleAggregationData*) data;

    Int64 totalDP = m_NumDataPoints + agg->m_NumDataPoints;
    if (totalDP > 0)
    {
        m_Value = agg->m_Value * ((double) agg->m_NumDataPoints / totalDP)
                   + m_Value * ((double) m_NumDataPoints / totalDP);
        m_NumDataPoints += agg->m_NumDataPoints;
    }
}

void STAT_AggregatedThroughput::Aggregate(clocktype now)
{
    STAT_VectorIterator it;
    STAT_Throughput* stat;
    m_BytesProcessed = 0;

    m_NumDataPoints = 0;
    for (it = m_Statistics.begin(); it != m_Statistics.end(); it++)
    {
        stat = (STAT_Throughput*) *it;

        // Check if session has started
        if (stat->GetSessionStart() != CLOCKTYPE_MAX)
        {
            m_BytesProcessed += stat->GetBytesProcessed();
            m_NumDataPoints += stat->GetNumDataPoints();
        }
    }

    // Compute throughput if time has elapsed
    if (now > 0)
    {
        m_Value = (m_BytesProcessed * 8.0 * SECOND) / now;
    }
    else
    {
        m_Value = 0.0;
    }
}

void STAT_AggregatedThroughput::Serialize(
    clocktype now,
    char** data,
    int* size)
{
    *data = (char*) MEM_malloc(sizeof(STAT_AggregationData));
    *size = sizeof(STAT_AggregationData);
    STAT_AggregationData* agg =
        (STAT_AggregationData*) *data;
    agg->m_Value = m_BytesProcessed;
    agg->m_NumDataPoints = m_NumDataPoints;
}

void STAT_AggregatedThroughput::DeserializeAndUpdate(
    clocktype now,
    char* data,
    int size)
{
    ERROR_Assert(
        size == sizeof(STAT_AggregationData),
        "Wrong size for aggregated average");

    STAT_AggregationData* agg =
        (STAT_AggregationData*) data;
    if (agg->m_NumDataPoints > 0)
    {
        m_BytesProcessed += agg->m_Value;
        m_NumDataPoints += agg->m_NumDataPoints;
    }

    if (now > 0)
    {
        m_Value = (m_BytesProcessed * 8.0 * SECOND) / now;
    }
    else
    {
        m_Value = 0.0;
    }
}

void STAT_AggregatedDivide::Aggregate(clocktype now)
{
    STAT_VectorIterator it;
    Float64 val;
    Float64 newValue = 0.0;

    if (m_Statistics.size() < 2)
    {
        m_Value = 0.0;
        m_NumDataPoints = 0;
        return;
    }

    it = m_Statistics.begin();
    newValue = (**it).GetValue(now);
    m_NumDataPoints = (**it).GetNumDataPoints();
    it++;
    while (it != m_Statistics.end())
    {
        val = (**it).GetValue(now);
        if (val == 0.0)
        {
            newValue = 0.0;
        }
        else
        {
            newValue /= val;
        }
        m_NumDataPoints += (**it).GetNumDataPoints();
        it++;
    }

    // Set the value only once incase value is listened to
    m_Value = newValue;
}

void STAT_AggregatedDivide::Serialize(clocktype now, char** data, int* size)
{
    *data = NULL;
    size = 0;
}

void STAT_AggregatedDivide::DeserializeAndUpdate(
    clocktype now,
    char* data,
    int size)
{
}
