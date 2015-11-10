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
#include "stats_phy.h"
#include "phy_abstract.h"

STAT_PhySessionStatistics::STAT_PhySessionStatistics()
{
    senderId = 0;
    receiverId = 0;
    channelIndex = 0;
    phyIndex = 0;

    utilization = 0.0;
    totalInterference = 0.0;
    totalPathLoss = 0.0;
    totalDelay = 0.0;
    totalSignalPower = 0.0;
    numSignals = 0;
    numErrorSignals = 0;
    lastSignalStartTime = CLOCKTYPE_MAX;
}

STAT_PhySessionStatistics::STAT_PhySessionStatistics(NodeId sender, NodeId receiver, int channel, int pIndex)
{
    senderId = sender;
    receiverId = receiver;
    channelIndex = channel;
    phyIndex = pIndex;
    utilization = 0.0;
    totalInterference = 0.0;
    totalPathLoss = 0.0;
    totalDelay = 0.0;
    totalSignalPower = 0.0;
    numSignals = 0;
    numErrorSignals = 0;
    lastSignalStartTime = CLOCKTYPE_MAX;
}

STAT_PhySessionStatistics& STAT_PhySessionStatistics::operator+= (const STAT_PhySessionStatistics& rhs)
{
    utilization += rhs.utilization;
    totalInterference += rhs.totalInterference;
    totalPathLoss += rhs.totalPathLoss;
    totalDelay += rhs.totalDelay;
    totalSignalPower += rhs.totalSignalPower;
    numSignals += rhs.numSignals;
    numErrorSignals += rhs.numErrorSignals;

    return *this;
}

STAT_PhySessionStatistics* STAT_PhyStatistics::GetSession(
    Node* node,
    NodeId senderId,
    NodeId receiverId,
    int channelIndex,
    int phyIndex)
{
    // Build the session key
    STAT_PhySessionKey key;
    key.m_senderId = senderId;
    key.m_receiverId = receiverId;
    key.m_channelIndex = channelIndex;
    key.m_phyIndex = phyIndex;

    // Look for the session key
    SessionIterator it = m_Sessions.find(key);
    if (it == m_Sessions.end())
    {
        // Not found, insert new statistics and return
        STAT_PhySessionStatistics stat(senderId, receiverId, channelIndex, phyIndex);
        m_Sessions[key] = stat;
        return &m_Sessions[key];
    }
    else
    {
        // Found, return statistics
        return &it->second;
    }
}

void STAT_PhyStatistics::SetStatNames()
{
    std::string name;
    std::string description;
    std::string units;

    // Set statistic names and descriptions
    name = "SignalsTransmitted";
    description = "Signals transmitted";
    units = "signals";
    m_SignalsTransmitted.SetInfo(name, description, units);

    name = "SignalsDetected";
    description = "Signals detected";
    units = "signals";
    m_SignalsDetected.SetInfo(
        name,
        description,
        units);

    name = "SignalsLocked";
    description = "Signals locked";
    units = "signals";
    m_SignalsLocked.SetInfo(
        name,
        description,
        units);

    name = "SignalsWithErrors";
    description = "Signals received with errors";
    units = "signals";
    m_SignalsWithErrors.SetInfo(
        name,
        description,
        units);

    name = "SignalsWithInterferenceErrors";
    description = "Signals received with interference";
    units = "signals";
    m_SignalsInterferenceErrors.SetInfo(
        name,
        description,
        units);

    name = "SignalsToMac";
    description = "Signals sent to mac";
    units = "signals";
    m_SignalsToMac.SetInfo(
        name,
        description,
        units);

    name = "TotalTransmitTime";
    description = "Time spent transmitting";
    units = "seconds";
    m_TotalTransmitTime.SetInfo(
        name,
        description,
        units);

    name = "TotalReceiveTime";
    description = "Time spent receiving";
    units = "seconds";
    m_TotalReceiveTime.SetInfo(
        name,
        description,
        units);

    name = "AverageDelay";
    description = "Average tranmission delay";
    units = "seconds";
    m_AverageDelay.SetInfo(
        name,
        description,
        units);

    name = "Utilization";
    description = "Utilization";
    units = "percent/100";
    m_Utilization.SetInfo(
        name,
        description,
        units);

    name = "AverageSignalPower";
    description = "Average signal power";
    units = "dBm";
    m_AverageSignalPower_dBm.SetInfo(
        name,
        description,
        units);

    name = "AverageInterference";
    description = "Average interference";
    units = "dBm";
    m_AverageInterference_dBm.SetInfo(
        name,
        description,
        units);

    name = "AveragePathloss";
    description = "Average pathloss";
    units = "dB";
    m_AveragePathloss_dB.SetInfo(
        name,
        description,
        units);
}

STAT_PhyStatistics::STAT_PhyStatistics(Node* node,
                                       PhyMode phyMode)
{
    m_LockTime = CLOCKTYPE_MAX;
    SetStatNames();
    this->phyMode = phyMode;
    signalLockMsgList = NULL;
    AddToGlobal(&node->partitionData->stats->global.phyAggregate);
}

void STAT_PhyStatistics::AddToGlobal(STAT_GlobalPhyStatistics* stat)
{
    stat->GetSignalsTransmitted().AddStatistic(&m_SignalsTransmitted);
    stat->GetSignalsDetected().AddStatistic(&m_SignalsDetected);
    stat->GetSignalsLocked().AddStatistic(&m_SignalsLocked);
    stat->GetSignalsWithErrors().AddStatistic(&m_SignalsWithErrors);
    stat->GetSignalsInterferenceErrors().AddStatistic(&m_SignalsInterferenceErrors);
    stat->GetSignalsToMac().AddStatistic(&m_SignalsToMac);
    stat->GetTotalTransmitTime().AddStatistic(&m_TotalTransmitTime);
    stat->GetTotalReceiveTime().AddStatistic(&m_TotalReceiveTime);
    stat->GetAverageDelay().AddStatistic(&m_AverageDelay);
    stat->GetAverageUtilization().AddStatistic(&m_Utilization);
    stat->GetAverageSignalPower_dBm().AddStatistic(&m_AverageSignalPower_dBm);
    stat->GetAverageInterference_dBm().AddStatistic(&m_AverageInterference_dBm);
    stat->GetAveragePathloss_dB().AddStatistic(&m_AveragePathloss_dB);
}

void STAT_PhyStatistics::GetList(std::vector<STAT_Statistic*>& stats)
{
    stats.clear();

    stats.push_back(&m_SignalsTransmitted);
    stats.push_back(&m_SignalsDetected);
    stats.push_back(&m_SignalsLocked);
    stats.push_back(&m_SignalsWithErrors);
    stats.push_back(&m_SignalsInterferenceErrors);
    stats.push_back(&m_SignalsToMac);
    stats.push_back(&m_TotalTransmitTime);
    stats.push_back(&m_TotalReceiveTime);
    stats.push_back(&m_AverageDelay);
    stats.push_back(&m_Utilization);
    stats.push_back(&m_AverageSignalPower_dBm);
    stats.push_back(&m_AverageInterference_dBm);
    stats.push_back(&m_AveragePathloss_dB);
}

void STAT_PhyStatistics::AddSignalTransmittedDataPoints(Node* node,
                                                        clocktype duration)
{
    // Add data points
    m_SignalsTransmitted.AddDataPoint(node->getNodeTime(), 1);
    m_TotalTransmitTime.AddDataPoint(node->getNodeTime(), (double) duration / SECOND);
    m_Utilization.AddDataPoint(node->getNodeTime(), (double) duration / SECOND);
}

void STAT_PhyStatistics::AddSignalDetectedDataPoints(Node* node)
{
    // Add data points
    m_SignalsDetected.AddDataPoint(node->getNodeTime(), 1);
}


// Call once for every signal that is locked.  After calling
// AddSignalLockedDataPoints the phy model must call either
// AddSignalWithErrorsDataPoints (if the signal was not received) or
// AddSignalToMacDataPoints (if the signal was received) or
// in case of PHY_NOLOCKING AddSignalUnLockedDataPoints (if the signal
// was terminated while receiving)
void STAT_PhyStatistics::AddSignalLockedDataPoints(
    Node* node,
    PropRxInfo* propRxInfo,
    PhyData* phy,
    Int32 channelIndex,
    double power_mW)
{
    // Add data points.  Note expect input for average signal power is in mW.
    m_SignalsLocked.AddDataPoint(node->getNodeTime(), 1);
    m_AverageSignalPower_dBm.AddDataPoint(node->getNodeTime(), power_mW);

    if (phyMode == PHY_LOCKING)
    {
        if (m_LockTime != CLOCKTYPE_MAX)
        {
            ERROR_ReportWarning("Signal locked but already locked");
            return;
        }
        m_LockTime = node->getNodeTime();
    }
    else if (phyMode == PHY_NOLOCKING)
    {
        if (!signalLockMsgList)
        {
            // This is the first time, so allocate memory
            signalLockMsgList =
                            new STAT_PhyStatistics::PHY_SIGNAL_LOCK_MSG_LIST;
        }

        if (!signalLockMsgList->size())
        {
            if (m_LockTime != CLOCKTYPE_MAX)
            {
                char errStr[MAX_STRING_LENGTH];
                sprintf(errStr, "m_LockTime is found with incorrect value "
                                "on signal lock. Reset the value with "
                                "current time. \n");
                ERROR_ReportWarning(errStr);

                m_LockTime = node->getNodeTime();
            }
        }

        if (m_LockTime == CLOCKTYPE_MAX)
        {
            // This may be the first lock time for the bunch of messages
            // going to be received at this same instant of time.
            // This variable will also assist to keep track of last
            // unrecorded time period for TotalReceiveTime and Utilization
            // statistics
            m_LockTime = node->getNodeTime();
        }

        // Add this new message in vector
        signalLockMsgList->push_back(propRxInfo->txMsg);
    }

    UpdateSession(
        node,
        propRxInfo,
        phy,
        channelIndex,
        0,
        0,
        FALSE,
        FALSE,
        0);
}


// Call when a signal is detected but could not be recieved.
void STAT_PhyStatistics::AddSignalWithErrorsDataPoints(
    Node* node,
    PropRxInfo* propRxInfo,
    PhyData* phy,
    Int32 channelIndex,
    double interference_mW,
    double signalPower_mW,
    BOOL deleteMsg)
{
    clocktype duration = 0;
    STAT_PhyStatistics::PHY_SIGNAL_LOCK_LIST_ITER msgIter;

    // Add data points
    m_SignalsWithErrors.AddDataPoint(node->getNodeTime(), 1);

    if (phyMode == PHY_LOCKING)
    {
        if (m_LockTime == CLOCKTYPE_MAX)
        {
            ERROR_ReportWarning("Signal with errors not locked");
            return;
        }
        duration = node->getNodeTime() - m_LockTime;

        m_TotalReceiveTime.AddDataPoint(node->getNodeTime(),
                                       (double)duration / SECOND);
        m_Utilization.AddDataPoint(node->getNodeTime(),
                                   (double)duration / SECOND);
        m_LockTime = CLOCKTYPE_MAX;
    }
    else if (phyMode == PHY_NOLOCKING)
    {
        if (!signalLockMsgList)
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr, "PHY_SIGNAL_LOCK_MSG_LIST not defined. "
                            "Received information ignored at "
                            "'ErrorsDataPoints'.\n");
            ERROR_ReportWarning(errStr);
            return;
        }

        msgIter = std::find(signalLockMsgList->begin(),
                            signalLockMsgList->end(),
                            propRxInfo->txMsg);

        if (msgIter != signalLockMsgList->end())
        {
            if (m_LockTime < node->getNodeTime())
            {
                // Case 1: First msg which completed its signal end from
                //         channel, and others (if exist) may come later
                // Case 2: Calculate the extra duration added by the
                //         subsequent msg from the point of last recorded
                //         time. This is required because it will add
                //         additional values in TotalReceiveTime and
                //         Utilization stats
                // For both cases:
                duration = node->getNodeTime() - m_LockTime;

                // update m_LockTime with the current time
                m_LockTime = node->getNodeTime();
            }
            else
            {
                // do nothing in case of 'm_LockTime == node->getNodeTime()'
                // duration value would be 0 here
            }
        }
        else
        {
            if (!signalLockMsgList->size())
            {
                // nothing is there in msg list, so reseting m_LockTime
                m_LockTime = CLOCKTYPE_MAX;
            }

            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr, "Signal not locked. Received information ignored "
                            "in 'ErrorsDataPoints' count.\n");
            ERROR_ReportWarning(errStr);
            return;
        }

        if (duration > 0)
        {
            m_TotalReceiveTime.AddDataPoint(node->getNodeTime(),
                                            (double)duration / SECOND);
            m_Utilization.AddDataPoint(node->getNodeTime(),
                                       (double)duration / SECOND);
        }
    }

    if (interference_mW > STAT_PHY_THRESHOLD_MW)
    {
        m_AverageInterference_dBm.AddDataPoint(node->getNodeTime(),
                                               interference_mW);
    }

    UpdateSession(
        node,
        propRxInfo,
        phy,
        channelIndex,
        interference_mW,
        signalPower_mW,
        TRUE,
        TRUE,
        duration);

    if (phyMode == PHY_NOLOCKING)
    {
        if (deleteMsg)
        {
            // now delete this message entry from PHY_SIGNAL_LOCK_MSG_LIST
            signalLockMsgList->erase(msgIter);

            // check whether any more msg left
            if (!signalLockMsgList->size())
            {
                // Reset lock time
                m_LockTime = CLOCKTYPE_MAX;
            }
        }
    }
}



// Call for the case when there is an in-between termination
// of current receiving signal
// Currently this API is for phy mode PHY_LOCKING only
void STAT_PhyStatistics::AddSignalTerminatedDataPoints(
    Node* node,
    PhyData* phy,
    Message* rxMsg,
    Int32 rxChannelIndex,
    NodeAddress txNodeId,
    double pathloss_dB,
    clocktype rxStartTime,
    double interference_mW,
    double signalPower_mW)
{
    if (phyMode != PHY_LOCKING)
    {
        // This API is currently for phy mode PHY_LOCKING only
        // so simply return
        return;
    }

    clocktype duration = 0;

    // Add data points
    m_SignalsWithErrors.AddDataPoint(node->getNodeTime(), 1);

    ERROR_Assert(m_LockTime != CLOCKTYPE_MAX,
                 "Signal with errors not locked");
    duration = node->getNodeTime() - m_LockTime;

    m_TotalReceiveTime.AddDataPoint(node->getNodeTime(),
                                   (double)duration / SECOND);
    m_Utilization.AddDataPoint(node->getNodeTime(),
                               (double)duration / SECOND);
    m_LockTime = CLOCKTYPE_MAX;

    if (interference_mW > 0)
    {
        m_AverageInterference_dBm.AddDataPoint(node->getNodeTime(),
                                               interference_mW);
    }

    UpdateSessionForTerminatedDataPoints(
        node,
        phy,
        rxMsg,
        rxChannelIndex,
        txNodeId,
        pathloss_dB,
        rxStartTime,
        interference_mW,
        signalPower_mW);
}




void STAT_PhyStatistics::AddSignalWithInterferenceErrorsDataPoints(Node* node)
{
    // Add data points
    m_SignalsInterferenceErrors.AddDataPoint(node->getNodeTime(), 1);
}


// Call when a signal is detected and successfully received
void STAT_PhyStatistics::AddSignalToMacDataPoints(
    Node* node,
    PropRxInfo* propRxInfo,
    PhyData* phy,
    Int32 channelIndex,
    clocktype delay,
    double interference_mW,
    double pathloss_dB,
    double signalPower_mW,
    BOOL deleteMsg)
{
    clocktype duration = 0;
    STAT_PhyStatistics::PHY_SIGNAL_LOCK_LIST_ITER msgIter;

    m_SignalsToMac.AddDataPoint(node->getNodeTime(), 1);
    m_AveragePathloss_dB.AddDataPoint(node->getNodeTime(), pathloss_dB);
    m_AverageDelay.AddDataPoint(node->getNodeTime(), (double)delay / SECOND);

    if (interference_mW > STAT_PHY_THRESHOLD_MW)
    {
        m_AverageInterference_dBm.AddDataPoint(node->getNodeTime(),
                                               interference_mW);
    }

    if (phyMode == PHY_LOCKING)
    {
        if (m_LockTime == CLOCKTYPE_MAX)
        {
            ERROR_ReportWarning("Signal to mac but not locked");
            return;
        }
        duration = node->getNodeTime() - m_LockTime;

        m_TotalReceiveTime.AddDataPoint(node->getNodeTime(),
                                        (double)duration / SECOND);
        m_Utilization.AddDataPoint(node->getNodeTime(),
                                   (double)duration / SECOND);
        m_LockTime = CLOCKTYPE_MAX;
    }
    else if (phyMode == PHY_NOLOCKING)
    {
        if (!signalLockMsgList)
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr, "PHY_SIGNAL_LOCK_MSG_LIST not defined. "
                            "Received information ignored at "
                            "'MacDataPoints'.\n");
            ERROR_ReportWarning(errStr);
            return;
        }

        msgIter = std::find(signalLockMsgList->begin(),
                            signalLockMsgList->end(),
                            propRxInfo->txMsg);

        if (msgIter != signalLockMsgList->end())
        {
            if (m_LockTime < node->getNodeTime())
            {
                // Case 1: First msg which completed its signal end from
                //         channel, and others (if exist) may come later
                // Case 2: Calculate the extra duration added by the
                //         subsequent msg from the point of last recorded
                //         time. This is required because it will add
                //         additional values in TotalReceiveTime and
                //         Utilization stats
                // For both cases:
                duration = node->getNodeTime() - m_LockTime;

                // update m_LockTime with the current time
                m_LockTime = node->getNodeTime();
            }
            else
            {
                // do nothing in case of 'm_LockTime == node->getNodeTime()'
                // duration value would be 0 here
            }
        }
        else
        {
            if (!signalLockMsgList->size())
            {
                // nothing is there in msg list, so reseting m_LockTime
                m_LockTime = CLOCKTYPE_MAX;
            }

            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr, "Signal not locked. Received information ignored "
                            "in 'ErrorsDataPoints' count.\n");
            ERROR_ReportWarning(errStr);
            return;
        }

        if (duration > 0)
        {
            m_TotalReceiveTime.AddDataPoint(node->getNodeTime(),
                                           (double)duration / SECOND);
            m_Utilization.AddDataPoint(node->getNodeTime(),
                                       (double)duration / SECOND);
        }
    }

    UpdateSession(
        node,
        propRxInfo,
        phy,
        channelIndex,
        interference_mW,
        signalPower_mW,
        TRUE,
        FALSE,
        duration);

    if (phyMode == PHY_NOLOCKING)
    {
        if (deleteMsg)
        {
            // now delete this message entry from PHY_SIGNAL_LOCK_MSG_LIST
            signalLockMsgList->erase(msgIter);

            // check whether any more msg left
            if (!signalLockMsgList->size())
            {
                // Reset lock time
                m_LockTime = CLOCKTYPE_MAX;
            }
        }
    }
}


// Update session statistics.  Called by AddSignalLockedDataPoints,
// AddSignalWithErrorsDataPoints and AddSignalToMacDataPoints
void STAT_PhyStatistics::UpdateSession(
    Node* node,
    PropRxInfo* propRxInfo,
    PhyData* phy,
    Int32 channelIndex,
    double totalInterference_mW,
    double signalPower_mW,
    BOOL endSignal,
    BOOL isErrorSignal,
    clocktype duration)
{

    // Update session statistics
    STAT_PhySessionStatistics* sess = GetSession(node,
                                                 propRxInfo->txNodeId,
                                                 node->nodeId,
                                                 propRxInfo->channelIndex,
                                                 phy->phyIndex);

    if (endSignal)
    {
        PropTxInfo* txInfo = (PropTxInfo*)MESSAGE_ReturnInfo(
                                                          propRxInfo->txMsg);

        if (phyMode == PHY_LOCKING)
        {
            if (sess->lastSignalStartTime == CLOCKTYPE_MAX)
            {
                ERROR_ReportWarning("Signal end before signal start");
                return;
            }
            sess->utilization += node->getNodeTime() - sess->lastSignalStartTime;
        }
        else if (phyMode == PHY_NOLOCKING)
        {
            sess->utilization += duration;
        }

        // common code
        if (isErrorSignal)
        {
            sess->numErrorSignals++;
        }
        else
        {
            sess->numSignals++;
        }

        // Record interference and pathloss
        sess->totalInterference += totalInterference_mW;
        sess->totalPathLoss += propRxInfo->pathloss_dB;

        // Record delay
        clocktype txDelay = propRxInfo->rxStartTime - txInfo->txStartTime;
        sess->totalDelay += (double)txDelay / SECOND;

        // Record signal power
        sess->totalSignalPower += IN_DB(signalPower_mW);

        if (phyMode == PHY_LOCKING)
        {
            sess->lastSignalStartTime = CLOCKTYPE_MAX;
        }
    }
    else
    {
        if (phyMode == PHY_LOCKING)
        {
            // Record when signal started
            sess->lastSignalStartTime = node->getNodeTime();
        }
    }
}




// Update session statistics.
// Called by API AddSignalTerminatedDataPoints
// Currently this API is for phy mode PHY_LOCKING only
void STAT_PhyStatistics::UpdateSessionForTerminatedDataPoints(
    Node* node,
    PhyData* phy,
    Message* rxMsg,
    Int32 rxChannelIndex,
    NodeAddress txNodeId,
    double pathloss_dB,
    clocktype rxStartTime,
    double totalInterference_mW,
    double signalPower_mW)
{
    if (phyMode != PHY_LOCKING)
    {
        // This API is currently for phy mode PHY_LOCKING only
        // so simply return
        return;
    }

    // Update session statistics
    STAT_PhySessionStatistics* sess = GetSession(node,
                                                 txNodeId,
                                                 node->nodeId,
                                                 rxChannelIndex,
                                                 phy->phyIndex);

    PropTxInfo* txInfo = (PropTxInfo*)MESSAGE_ReturnInfo(rxMsg);

    ERROR_Assert(sess->lastSignalStartTime != CLOCKTYPE_MAX,
                 "Signal end before signal start");
    sess->utilization += node->getNodeTime() - sess->lastSignalStartTime;

    sess->numErrorSignals++;

    // Record interference and pathloss
    sess->totalInterference += totalInterference_mW;
    sess->totalPathLoss += pathloss_dB;

    // Record delay
    clocktype txDelay = rxStartTime - txInfo->txStartTime;
    sess->totalDelay += (double)txDelay / SECOND;

    // Record signal power
    sess->totalSignalPower += IN_DB(signalPower_mW);
    sess->lastSignalStartTime = CLOCKTYPE_MAX;
}




// Update session statistics for Msg in the received PackedMsg on PHY.
// Called by APIs AddSignalWithErrorsDataPointsForMsgInRecvPackedMsg() and
// AddSignalToMacDataPointsForMsgInRecvPackedMsg() only.
void STAT_PhyStatistics::UpdateSessionForMsgInRecvPackedMsg(
    Node* node,
    PropRxInfo* propRxInfo,
    PhyData* phy,
    Int32 channelIndex,
    BOOL isErrorSignal)
{
    // Update session statistics
    STAT_PhySessionStatistics* sess = GetSession(node,
                                                 propRxInfo->txNodeId,
                                                 node->nodeId,
                                                 propRxInfo->channelIndex,
                                                 phy->phyIndex);
    // increment counters for additional values
    if (isErrorSignal)
    {
        sess->numErrorSignals++;
    }
    else
    {
        sess->numSignals++;
    }
}


// This API is currently used to reset m_LockTime for both the phy modes
// PHY_LOCKING and PHY_NOLOCKING.
// This API should also be called after APIs
// AddSignalWithErrorsDataPointsForMsgInRecvPackedMsg() and
// AddSignalToMacDataPointsForMsgInRecvPackedMsg() to unlock and delete the
// original received packed message on PHY.
void STAT_PhyStatistics::AddSignalUnLockedDataPoints(Node* node,
                                                     Message* msg)
{
    if (phyMode == PHY_LOCKING)
    {
        if (m_LockTime == CLOCKTYPE_MAX)
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr, "Signal not locked. "
                            "Ignoring the Unlock request.\n");
            ERROR_ReportWarning(errStr);
        }
        // Reset lock and return
        m_LockTime = CLOCKTYPE_MAX;
        return;
    }

    if (!signalLockMsgList)
    {
        char errStr[MAX_STRING_LENGTH];
        sprintf(errStr, "PHY_SIGNAL_LOCK_MSG_LIST not defined. "
                        "Ignoring the message Unlock request\n");
        ERROR_ReportWarning(errStr);
        return;
    }

    STAT_PhyStatistics::PHY_SIGNAL_LOCK_LIST_ITER msgIter;

    msgIter = std::find(signalLockMsgList->begin(),
                        signalLockMsgList->end(),
                        msg);

    if (msgIter != signalLockMsgList->end())
    {
        // now delete this message entry from PHY_SIGNAL_LOCK_MSG_LIST
        signalLockMsgList->erase(msgIter);

        // check whether any more msg left
        if (!signalLockMsgList->size())
        {
            // Reset lock time
            m_LockTime = CLOCKTYPE_MAX;
        }
    }
    else
    {
        if (!signalLockMsgList->size())
        {
            // nothing is there in msg list, so reseting m_LockTime
            m_LockTime = CLOCKTYPE_MAX;
        }

        char errStr[MAX_STRING_LENGTH];
        sprintf(errStr, "Signal not found for Unlock. "
                        "Ignoring the message Unlock request.\n");
        ERROR_ReportWarning(errStr);
        return;
    }
}



// Call when a signal is detected but could not be recieved.
// This API is for handling individual msg that are extracted from the
// message list under received packed message on PHY.
// AddSignalUnLockedDataPoints() must be called after calling this API
// to make sure of unlocking the original packed message.
void STAT_PhyStatistics::AddSignalWithErrorsDataPointsForMsgInRecvPackedMsg(
    Node* node,
    PropRxInfo* propRxInfo,
    PhyData* phy,
    Int32 channelIndex,
    double interference_mW,
    double pathloss_dB,
    double signalPower_mW,
    BOOL isOriginalPackedMsg)
{
    if (isOriginalPackedMsg)
    {
        // call this API only once for the original packed message
        // received on PHY, without deleting the message from the lock list
        AddSignalWithErrorsDataPoints(node,
                                      propRxInfo,
                                      phy,
                                      channelIndex,
                                      interference_mW,
                                      signalPower_mW,
                                      FALSE);
    }
    else
    {
        UpdateSessionForMsgInRecvPackedMsg(
            node,
            propRxInfo,
            phy,
            channelIndex,
            TRUE);
    }
}


// Call when a signal is detected and successfully received.
// This API is for handling individual msg that are extracted from the
// message list under received packed message on PHY.
// AddSignalUnLockedDataPoints() must be called after calling this API
// to make sure of unlocking the original packed message.
void STAT_PhyStatistics::AddSignalToMacDataPointsForMsgInRecvPackedMsg(
    Node* node,
    PropRxInfo* propRxInfo,
    PhyData* phy,
    Int32 channelIndex,
    clocktype delay,
    double interference_mW,
    double pathloss_dB,
    double signalPower_mW,
    BOOL isOriginalPackedMsg)
{
    if (isOriginalPackedMsg)
    {
        // call this API only once for the original packed message
        // received on PHY, without deleting the message from the lock list
        AddSignalToMacDataPoints(node,
                                 propRxInfo,
                                 phy,
                                 channelIndex,
                                 delay,
                                 interference_mW,
                                 pathloss_dB,
                                 signalPower_mW,
                                 FALSE);
    }
    else
    {
        UpdateSessionForMsgInRecvPackedMsg(
            node,
            propRxInfo,
            phy,
            channelIndex,
            FALSE);
    }
}


void STAT_PhyStatistics::Finalize(Node* node)
{
    // Remove from hierarchy
    STAT_ModelStatistics::Finalize(node);
}

void STAT_GlobalPhyStatistics::Initialize(PartitionData* partition, STAT_StatisticsList* stats)
{
    std::string name;
    std::string path;
    std::string description;
    std::string units;

    // Set names for all global statistics and add partition based
    // statistics to the global statistics.
    if (partition->partitionId == 0)
    {
        name = "SignalsTransmitted";
        path = "/stats/phy/signalsTransmitted";
        description = "Total number of signals trasmitted";
        units = "signals";
        m_SignalsTransmitted.SetInfo(
            name,
            description,
            units);
        m_SignalsTransmitted.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_SignalsTransmitted);

    if (partition->partitionId == 0)
    {
        name = "SignalsDetected";
        path = "/stats/phy/signalsDetected";
        description = "Total number of signals detected at the phy layer";
        units = "signals";
        m_SignalsDetected.SetInfo(
            name,
            description,
            units);
        m_SignalsDetected.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_SignalsDetected);

    if (partition->partitionId == 0)
    {
        name = "SignalsLocked";
        path = "/stats/phy/signalsLocked";
        description = "Total number of signals locked at the phy layer";
        units = "signals";
        m_SignalsLocked.SetInfo(
            name,
            description,
            units);
        m_SignalsLocked.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_SignalsLocked);

    if (partition->partitionId == 0)
    {
        name = "SignalsWithErrors";
        path = "/stats/phy/signalsWithErrors";
        description = "Total number of signals received with errors at the phy layer";
        units = "signals";
        m_SignalsWithErrors.SetInfo(
            name,
            description,
            units);
        m_SignalsWithErrors.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_SignalsWithErrors);

    if (partition->partitionId == 0)
    {
        name = "SignalsWithInterferenceErrors";
        path = "/stats/phy/signalsWithInterferenceErrors";
        description = "Total number of signals received with interference at the phy layer";
        units = "signals";
        m_SignalsInterferenceErrors.SetInfo(
            name,
            description,
            units);
        m_SignalsInterferenceErrors.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_SignalsInterferenceErrors);

    if (partition->partitionId == 0)
    {
        name = "SignalsToMac";
        path = "/stats/phy/signalsToMac";
        description = "Total number of signals sent to mac from the phy layer";
        units = "signals";
        m_SignalsToMac.SetInfo(
            name,
            description,
            units);
        m_SignalsToMac.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_SignalsToMac);

    if (partition->partitionId == 0)
    {
        name = "TotalTransmitTime";
        path = "/stats/phy/totalTransmitTime";
        description = "Total amount of time spent transmitting";
        units = "seconds";
        m_TotalTransmitTime.SetInfo(
            name,
            description,
            units);
        m_TotalTransmitTime.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_TotalTransmitTime);

    if (partition->partitionId == 0)
    {
        name = "TotalReceiveTime";
        path = "/stats/phy/totalReceiveTime";
        description = "Total amount of time spent receiving";
        units = "seconds";
        m_TotalReceiveTime.SetInfo(
            name,
            description,
            units);
        m_TotalReceiveTime.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_TotalReceiveTime);

    if (partition->partitionId == 0)
    {
        name = "AverageDelay";
        path = "/stats/phy/averageDelay";
        description = "Average tranmission delay at the phy layer";
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
        name = "AverageUtilization";
        path = "/stats/phy/averageUtilization";
        description = "Average utilization of all radios at the phy layer";
        units = "percent/100";
        m_AverageUtilization.SetInfo(
            name,
            description,
            units);
        m_AverageUtilization.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_AverageUtilization);

    if (partition->partitionId == 0)
    {
        name = "AverageSignalPower";
        path = "/stats/phy/averageSignalPower";
        description = "Average signal power at the phy layer";
        units = "dBm";
        m_AverageSignalPower_dBm.SetInfo(
            name,
            description,
            units);
        m_AverageSignalPower_dBm.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_AverageSignalPower_dBm);

    if (partition->partitionId == 0)
    {
        name = "AverageInterference";
        path = "/stats/phy/averageInterference";
        description = "Average interference at the phy layer";
        units = "dBm";
        m_AverageInterference_dBm.SetInfo(
            name,
            description,
            units);
        m_AverageInterference_dBm.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_AverageInterference_dBm);

    if (partition->partitionId == 0)
    {
        name = "AveragePathloss";
        path = "/stats/phy/averagePathloss";
        description = "Average pathloss at the phy layer";
        units = "dB";
        m_AveragePathloss_dB.SetInfo(
            name,
            description,
            units);
        m_AveragePathloss_dB.AddToHierarchy(&partition->dynamicHierarchy, path);
    }
    stats->RegisterAggregatedStatistic(&m_AveragePathloss_dB);
}

