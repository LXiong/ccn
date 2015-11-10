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

#include "layer2_lte_sch_roundrobin.h"
#ifdef LTE_LIB_LOG
#include "log_lte.h"
#endif

#define EFFECTIVE_SINR_BY_AVERAGE 1
#define LTE_BER_BASED 1

// /**
// FUNCTION   :: LteSchedulerENBRoundRobin::LteSchedulerENBRoundRobin
// LAYER      :: MAC
// PURPOSE    :: Initialize Round Robin scheduler
// PARAMETERS ::
// + node           : Node* : Node Pointer
// + interfaceIndex : int   : Interface index
// + nodeInput      : const NodeInput* : Pointer to node input
// RETURN::    void:         NULL
// **/
LteSchedulerENBRoundRobin::LteSchedulerENBRoundRobin(
    Node* node, int interfaceIndex, const NodeInput* nodeInput)
    : LteSchedulerENB(node, interfaceIndex, nodeInput)
{
#if SCH_LTE_ENABLE_RR_RANDOM_SORT
    RANDOM_SetSeed(
            randomSeed,
            node->globalSeed,
            node->nodeId,
            MAC_PROTOCOL_LTE,
            interfaceIndex);
#endif

    _dlNextAllocatedUe = _dlConnectedUeList.end();
    _ulNextAllocatedUe = _ulConnectedUeList.end();
}

// /**
// FUNCTION     :: LteSchedulerENBRoundRobin::~LteSchedulerENBRoundRobin
// LAYER        :: MAC
// PURPOSE      :: Finalize Round Robin scheduler
// PARAMETERS   ::
// RETURN::    void:         NULL
// **/
LteSchedulerENBRoundRobin::~LteSchedulerENBRoundRobin()
{
}

// /**
// FUNCTION   :: LteSchedulerENBRoundRobin::notifyPowerOn
// LAYER      :: MAC
// PURPOSE    :: Notify Round Robin scheduler of power on.
// PARAMETERS ::
// RETURN::    void:         NULL
// **/
void LteSchedulerENBRoundRobin::notifyPowerOn()
{
    // Do nothing
}

// /**
// FUNCTION   :: LteSchedulerENBRoundRobin::notifyPowerOff
// LAYER      :: MAC
// PURPOSE    :: Notify Round Robin scheduler of power off.
// PARAMETERS ::
// RETURN::    void:         NULL
// **/
void LteSchedulerENBRoundRobin::notifyPowerOff()
{
    // Do nothing
}

// /**
// FUNCTION   :: LteSchedulerENBRoundRobin::notifyAttach
// LAYER      :: MAC
// PURPOSE    :: Notify Round Robin scheduler of UE attach
// PARAMETERS ::
// + rnti : const LteRnti& : RNTI of newly attached UE
// RETURN::    void:         NULL
// **/
void LteSchedulerENBRoundRobin::notifyAttach(const LteRnti& rnti)
{
    // Do nothing
}

// /**
// FUNCTION   :: LteSchedulerENBRoundRobin::notifyDetach
// LAYER      :: MAC
// PURPOSE    :: Notify Round Robin scheduler of UE detach
// PARAMETERS ::
// + rnti : const LteRnti& : RNTI of detached UE
// RETURN::    void:         NULL
// **/
void LteSchedulerENBRoundRobin::notifyDetach(const LteRnti& rnti)
{
    // Do nothing
}

// /**
// FUNCTION   :: LteSchedulerENBRoundRobin::prepareForScheduleTti
// LAYER      :: MAC
// PURPOSE    :: Prepare for scheduling.
// PARAMETERS ::
// + ttiNumber : UInt64 : TTI number
// RETURN::    void:         NULL
// **/
void LteSchedulerENBRoundRobin::prepareForScheduleTti(UInt64 ttiNumber)
{
    LteScheduler::prepareForScheduleTti(ttiNumber);

#ifdef LTE_LIB_LOG
    debugOutputInterfaceCheckingLog();
#endif

    ListLteRnti tempList;
    Layer3LteGetSchedulableListSortedByConnectedTime(_node, _interfaceIndex,
        &tempList);
    const ListLteRnti* listLteRnti = &tempList;

    ListLteRnti::const_iterator start_dl =
                                    getNextAllocatedUe(true,  listLteRnti);
    ListLteRnti::const_iterator start_ul =
                                    getNextAllocatedUe(false, listLteRnti);

    _dlConnectedUeList.clear();
    _ulConnectedUeList.clear();

    _dlConnectedUeList.insert(
                _dlConnectedUeList.begin(), listLteRnti->begin(), start_dl);
    _dlConnectedUeList.insert(
                _dlConnectedUeList.begin(), start_dl, listLteRnti->end());


    _ulConnectedUeList.insert(
                _ulConnectedUeList.begin(), listLteRnti->begin(), start_ul);
    _ulConnectedUeList.insert(
                _ulConnectedUeList.begin(), start_ul, listLteRnti->end());

    _dlNextAllocatedUe = _dlConnectedUeList.begin();
    _ulNextAllocatedUe = _ulConnectedUeList.begin();


#ifdef LTE_LIB_LOG
    std::stringstream log;
    ListLteRnti::const_iterator c_it;
    char buf[MAX_STRING_LENGTH];

    // Downlink target UEs
    for (c_it = listLteRnti->begin(); c_it != listLteRnti->end(); ++c_it)
    {
        sprintf(buf,
                LTE_STRING_FORMAT_RNTI,
                c_it->nodeId,
                c_it->interfaceIndex);
        log << buf << ",";
    }
    lte::LteLog::DebugFormat(
        _node,
        _interfaceIndex,
        LTE_STRING_LAYER_TYPE_SCHEDULER,
        "%s,%s",
        LTE_STRING_CATEGORY_TYPE_CONNECTED_UE,
        log.str().c_str());

    // Connected UEs list for downlink
    log.str("");
    for (c_it = _dlConnectedUeList.begin();
        c_it != _dlConnectedUeList.end();
        ++c_it)
    {
        sprintf(buf,
                LTE_STRING_FORMAT_RNTI,
                c_it->nodeId,
                c_it->interfaceIndex);
        log << buf << ",";
    }
    lte::LteLog::DebugFormat(
        _node,
        _interfaceIndex,
        LTE_STRING_LAYER_TYPE_SCHEDULER,
        "%s,%s",
        LTE_STRING_CATEGORY_TYPE_CONNECTED_UE_DL,
        log.str().c_str());

    // Connected UEs list for uplink
    log.str("");
    for (c_it = _ulConnectedUeList.begin();
        c_it != _ulConnectedUeList.end();
        ++c_it)
    {
        sprintf(buf,
                LTE_STRING_FORMAT_RNTI,
                c_it->nodeId,
                c_it->interfaceIndex);
        log << buf << ",";
    }
    lte::LteLog::DebugFormat(
        _node,
        _interfaceIndex,
        LTE_STRING_LAYER_TYPE_SCHEDULER,
        "%s,%s",
        LTE_STRING_CATEGORY_TYPE_CONNECTED_UE_UL,
        log.str().c_str());
#endif // LTE_LIB_LOG
}


// /**
// FUNCTION   :: LteSchedulerENBRoundRobin::determineTargetUes
// LAYER      :: MAC
// PURPOSE    :: Determine target UEs for scheduling
// PARAMETERS ::
// + downlink       : bool                  : Downlink scheduling or uplink
// + targetUes     : std::vector<LteRnti>* : Buffer for the target UEs stored
// RETURN     :: void : NULL
// **/
void LteSchedulerENBRoundRobin::determineTargetUes(
    bool downlink,
    std::vector < LteRnti >* targetUes)
{
    // Get list of RNTI of UEs in RRC_CONNECTED status



    if (downlink)
    {
        ListLteRnti::const_iterator it;
        for (it = _dlConnectedUeList.begin();
            it != _dlConnectedUeList.end();
            ++it)
        {
            if (dlIsTargetUe(*it))
            {
                targetUes->push_back(*it);
            }
        }
    }else
    {
        ListLteRnti::const_iterator it;
        for (it = _ulConnectedUeList.begin();
            it != _ulConnectedUeList.end();
            ++it)
        {
            if (ulIsTargetUe(*it))
            {
                targetUes->push_back(*it);
            }
        }
    }
}

// /**
// FUNCTION   :: LteSchedulerENBRoundRobin::getNextAllocatedUe
// LAYER      :: MAC
// PURPOSE    :: Get next allocated UE
// PARAMETERS ::
// + downlink    : bool               : Downlink or not
// + listLteRnti : const ListLteRnti* : List of all of the current
//                                      connected UEs
// RETURN     :: int : Selected MCS index.
//                     In case no MCS index satisfies targetBler, return -1.
// **/
ListLteRnti::const_iterator LteSchedulerENBRoundRobin::getNextAllocatedUe(
    bool downlink,
    const ListLteRnti* listLteRnti)
{
    ListLteRnti::const_iterator nextAllocatedUe;
    ListLteRnti& _connectedUeList =
                        downlink ? _dlConnectedUeList : _ulConnectedUeList;

    if (downlink)
    {
        nextAllocatedUe = _dlNextAllocatedUe;

    }else
    {
        nextAllocatedUe = _ulNextAllocatedUe;
    }

    // If no UE is set as candidate of next allocated UE
    // Adopt head UE in connected UEs list
    if (nextAllocatedUe == _connectedUeList.end())
    {
        return listLteRnti->begin();
    }

    ListLteRnti::const_iterator start =
        find(listLteRnti->begin(), listLteRnti->end(), *nextAllocatedUe);

    // If next UE is disconnected, determine next candidate UE.

    ListLteRnti::const_iterator firstNextAllocatedUe = nextAllocatedUe;

    while (start == listLteRnti->end())
    {
        ++nextAllocatedUe;

        if (nextAllocatedUe == _connectedUeList.end())
        {
            nextAllocatedUe = _connectedUeList.begin();
        }

        if (nextAllocatedUe == firstNextAllocatedUe)
        {
            break;
        }

        start =
            find(listLteRnti->begin(), listLteRnti->end(), *nextAllocatedUe);

    }

    // start == listLteRnti->end() if no candidate UE is determined
    return start;

}

// /**
// FUNCTION   :: LteSchedulerENBRoundRobin::scheduleDlTti
// LAYER      :: MAC
// PURPOSE    :: Execute scheduling for downlink transmission
// PARAMETERS ::
// + schedulingResult : std::vector<LteDlSchedulingResultInfo>& :
//                        List of scheduling result.
// RETURN     :: void : NULL
// **/
void LteSchedulerENBRoundRobin::scheduleDlTti(
    std::vector < LteDlSchedulingResultInfo > &schedulingResult)
{
    // Initialize resutls
    schedulingResult.clear();

    // Get common informations (Could be class member variables)
    int numRb = PhyLteGetNumResourceBlocks(_node, _phyIndex);

    // Determine target UEs
    // -----------------------

    std::vector < LteRnti > targetUes;

    determineTargetUes(true, &targetUes);

#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        std::vector < LteRnti > ::const_iterator c_it;
        char buf[MAX_STRING_LENGTH];

        // Downlink target UEs
        for (c_it = targetUes.begin(); c_it != targetUes.end(); ++c_it)
        {
            log << STR_RNTI(buf, *c_it) << ",";
        }

        lte::LteLog::DebugFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_TARGET_UE_DL,
            log.str().c_str());
    }
#endif

    int numTargetUes = targetUes.size();

    // Exit scheduling if there is no target user
    if (numTargetUes == 0)
    {
        return;
    }


    int rbGroupSize = PhyLteGetRbGroupsSize(_node, _phyIndex, numRb);

    int numberOfRbGroup = (int)ceil((double)numRb / rbGroupSize);

    int lastRbGroupSize = rbGroupSize;

    if (numRb % rbGroupSize != 0)
    {
        lastRbGroupSize = numRb % rbGroupSize;
    }

    // Number of allocated UEs
    int numAllocatedUes = MIN(numberOfRbGroup, numTargetUes);

    // Allocate array in which to set scheduling results
    // !! MAC layer must delete (free) !!
    // !! this dynamically allocated schedulingResult !!
    schedulingResult.resize(numAllocatedUes);

    // Initialize scheduling results
    for (int ueIndex = 0; ueIndex < numAllocatedUes; ++ueIndex)
    {
        schedulingResult[ueIndex].rnti = targetUes[ueIndex];

        for (int rbIndex = 0; rbIndex < LTE_MAX_NUM_RB; ++rbIndex)
        {
            schedulingResult[ueIndex].allocatedRb[rbIndex] = 0;
        }

        schedulingResult[ueIndex].numResourceBlocks = 0;

        schedulingResult[ueIndex].mcsIndex[0] = PHY_LTE_INVALID_MCS;
        schedulingResult[ueIndex].mcsIndex[1] = PHY_LTE_INVALID_MCS;
    }

    // RB allocation
    // -----------------

    int nextAllocatedUeIndex = 0;

#if SCH_LTE_ENABLE_RR_RANDOM_SORT
    std::vector < rbgMetricEntry > shuffledRbgs(numberOfRbGroup);
    for (int rbGroupIndex = 0;
        rbGroupIndex < numberOfRbGroup;
        ++rbGroupIndex)
    {
        shuffledRbgs[rbGroupIndex]._rbgIndex = rbGroupIndex;
        shuffledRbgs[rbGroupIndex]._rand = RANDOM_erand(randomSeed);
    }
    std::sort(
            shuffledRbgs.begin(), shuffledRbgs.end(), rbgMetricGreaterPtr());
#endif

#if SCH_LTE_ENABLE_RR_RANDOM_SORT
    for (int shuffledRbGroupIndex = 0;
        shuffledRbGroupIndex < numberOfRbGroup;
        ++shuffledRbGroupIndex)
    {
        int rbGroupIndex = shuffledRbgs[shuffledRbGroupIndex]._rbgIndex;
#else
    for (int rbGroupIndex = 0;
        rbGroupIndex < numberOfRbGroup;
        ++rbGroupIndex)
    {
#endif
        int startRbIndex = rbGroupIndex * rbGroupSize;

        int numRbsInThisRbGroup =
            (rbGroupIndex < numberOfRbGroup - 1) ?
                rbGroupSize : lastRbGroupSize;

        for (int lRbIndex = 0; lRbIndex < numRbsInThisRbGroup; ++lRbIndex)
        {
            int rbIndex = startRbIndex + lRbIndex;
            schedulingResult[nextAllocatedUeIndex].allocatedRb[rbIndex] = 1;
        }

        schedulingResult[nextAllocatedUeIndex].numResourceBlocks +=
                                                        numRbsInThisRbGroup;

        nextAllocatedUeIndex = (nextAllocatedUeIndex + 1) % numAllocatedUes;
    }

    // Update the candidate of next allocated Ue (for next TTI)
    if (numAllocatedUes < numTargetUes)
    {
        // If number of allocated UE is less than number of target UEs
        // (Which means, UEs which have packet to sent
        // were not be allocated any RBs)
        // then, such UEs has to be allocated at first in next scheduling.

        _dlNextAllocatedUe = find(
            _dlConnectedUeList.begin(),
            _dlConnectedUeList.end(),
            targetUes[numAllocatedUes]);
    }else
    {
        // If not, all UEs are checked equally whether
        // it has packet to sent or not,
        // So next allocated UE is same as the next allocated UE of
        // current allocation.
        _dlNextAllocatedUe = find(
            _dlConnectedUeList.begin(),
            _dlConnectedUeList.end(),
            targetUes[nextAllocatedUeIndex]);
    }


    // Determine transmission scheme
    //--------------------------------
    // Transmission scheme and tarnamission mode is difference concept.
    //

    for (int ueIndex = 0; ueIndex < numAllocatedUes; ++ueIndex) {

        // Retrieve CQI fedback from UE
        PhyLteCqiReportInfo phyLteCqiReportInfo;

        bool ret = PhyLteGetCqiInfoFedbackFromUe(
            _node,
            _phyIndex,
            targetUes[ueIndex],
            &phyLteCqiReportInfo);

        ERROR_Assert(ret == true, "CQIs not found.");

        int numLayer;
        int numTransportBlocksToSend;

        //results[ueIndex].txScheme = determineTransmissiomScheme(
        //    phyLteCqiReportInfo.riInfo,
        //    &numLayer, &numTransportBlocksToSend);

        schedulingResult[ueIndex].txScheme =
                                        PhyLteDetermineTransmissiomScheme(
                                                _node,
                                                _phyIndex,
                                                phyLteCqiReportInfo.riInfo,
                                                &numLayer,
                                                &numTransportBlocksToSend);

        schedulingResult[ueIndex].numTransportBlock =
                                        numTransportBlocksToSend;
    }


    // Calculate estimated SINR
    //------------------------------
    // In case of downlink,
    // estimated SINR is retrieved as CQI fedback from UE

    std::vector < std::vector < double > > estimatedSinr_dB(numAllocatedUes);
    for (int ueIndex = 0; ueIndex < numAllocatedUes; ++ueIndex)
    {
        estimatedSinr_dB[ueIndex].resize(
                                schedulingResult[ueIndex].numTransportBlock);
    }

    for (int ueIndex = 0; ueIndex < numAllocatedUes; ++ueIndex)
    {

        // Retrieve CQI fedback from UE
        PhyLteCqiReportInfo phyLteCqiReportInfo;

        bool ret = PhyLteGetCqiInfoFedbackFromUe(
            _node,
            _phyIndex,
            targetUes[ueIndex],
            &phyLteCqiReportInfo);

        ERROR_Assert(ret == true, "CQIs not found.");

        int len;
        Float64* cqiTable;
        PhyLteGetCqiSnrTable(_node, _phyIndex, &cqiTable, &len);

        for (int tbIndex = 0;
            tbIndex < schedulingResult[ueIndex].numTransportBlock;
            ++tbIndex)
        {
            int cqi = (tbIndex == 0) ?
                phyLteCqiReportInfo.cqiInfo.cqi0 :
                phyLteCqiReportInfo.cqiInfo.cqi1;

            estimatedSinr_dB[ueIndex][tbIndex] =
                                (cqi != PHY_LTE_INVALID_CQI) ?
                                cqiTable[cqi] :
                                LTE_NEGATIVE_INFINITY_SINR_dB;
        }
    }

    // Determine MCS index
    //------------------------

    for (int ueIndex = 0; ueIndex < numAllocatedUes; ++ueIndex)
    {
        for (int tbIndex = 0;
            tbIndex < schedulingResult[ueIndex].numTransportBlock;
            ++tbIndex)
        {
            int mcsIndex = dlSelectMcs(
                schedulingResult[ueIndex].numResourceBlocks,
                estimatedSinr_dB[ueIndex][tbIndex],
                _targetBler);

            if (mcsIndex < 0)
            {
                mcsIndex = 0;
            }

            schedulingResult[ueIndex].mcsIndex[tbIndex] = (UInt8)mcsIndex;

#ifdef LTE_LIB_LOG
            PhyLteGetAggregator(_node, _phyIndex)->regist(
                                            schedulingResult[ueIndex].rnti,
                                            lte::Aggregator::DL_MCS,
                                            mcsIndex);
#endif
        }
    }

#ifdef LTE_LIB_LOG
    debugOutputEstimatedSinrLog(schedulingResult, estimatedSinr_dB);
#endif

    // Create dequeue information
    //----------------------------

    createDequeueInformation(schedulingResult);

#ifdef LTE_LIB_LOG
    // Output scheduling result logs
    debugOutputSchedulingResultLog(schedulingResult);
#endif

    // Finally, Purge invalid scheduling results
    //-------------------------------------------
    purgeInvalidSchedulingResults(schedulingResult);

    // End of DL scheduling
}

// /**
// FUNCTION   :: LteSchedulerENBRoundRobin::scheduleUlTti
// LAYER      :: MAC
// PURPOSE    :: Execute scheduling for uplink transmission
// PARAMETERS ::
// + schedulingResult : std::vector<LteUlSchedulingResultInfo>& :
//                        List of scheduling result
// RETURN     :: void : NULL
// **/
void LteSchedulerENBRoundRobin::scheduleUlTti(
    std::vector < LteUlSchedulingResultInfo > &schedulingResult)
{
    // Initialize resutls
    schedulingResult.clear();

    // Get common informations (Could be class member variables)
    int numRb = PhyLteGetNumResourceBlocks(_node, _phyIndex);

    int pucchOverhead =
        PhyLteGetUlCtrlChannelOverhead(_node, _phyIndex);

    // Number of available RBs
   int numAvailableRb = numRb - pucchOverhead;

   if (numAvailableRb <= 0)
   {
       return;
   }

    // If pucchOverhead = 5, 3 RBs at the bottom of bandwidth and
    // 2 RBs at the top of all bandwidth are used for PUCCH.
    int puschRbOffset = (int)ceil((double)pucchOverhead / 2.0);

    // Determine target UEs
    // -----------------------

    std::vector < LteRnti > targetUes;
    determineTargetUes(false, &targetUes);

#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        std::vector < LteRnti > ::const_iterator c_it;
        char buf[MAX_STRING_LENGTH];

        // Downlink target UEs
        for (c_it = targetUes.begin(); c_it != targetUes.end(); ++c_it)
        {
            log << STR_RNTI(buf, *c_it) << ",";
        }
        lte::LteLog::DebugFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_TARGET_UE_UL,
            log.str().c_str());
    }
#endif

    int numTargetUes = targetUes.size();

    // Exit scheduling if there is no target user
    if (numTargetUes == 0)
    {
        return;
    }

    int rbGroupSizeL = (int)ceil((double)numAvailableRb / numTargetUes);
    int rbGroupSizeS = rbGroupSizeL - 1;

    int numberOfRbGroupS = numTargetUes * rbGroupSizeL - numAvailableRb;
    int numberOfRbGroupL = numTargetUes - numberOfRbGroupS;

    // Number of allocated UEs
    int numAllocatedUes = numberOfRbGroupL +
        (rbGroupSizeS > 0 ? numberOfRbGroupS : 0);

    int numberOfRbGroup = numAllocatedUes;

    // Update the candidate of next allocated Ue (for next TTI)
    if (numAllocatedUes < numTargetUes)
    {
        // If number of allocated UE is less than number of target UEs
        // (Which means, An UE which have packet to sent
        // was not be allocated any RBs)
        // then, such UE has to be allocated at first in next scheduling.

        _ulNextAllocatedUe = find(
            _ulConnectedUeList.begin(),
            _ulConnectedUeList.end(),
            targetUes[numAllocatedUes]);
    }else
    {
        // If not, all UEs are checked equally whether
        // it has packet to sent or not,
        // So next allocated UE is same as that of current TTI

        _ulNextAllocatedUe = _ulConnectedUeList.begin();
    }


    // Allocate array in which to set scheduling results
    schedulingResult.resize(numAllocatedUes);

    // RB allocation
    // -----------------

#if SCH_LTE_ENABLE_RR_RANDOM_SORT
    std::vector < rbgMetricEntry > shuffledRbgs(numberOfRbGroup);
    for (int rbGroupIndex = 0;
        rbGroupIndex < numberOfRbGroup;
        ++rbGroupIndex)
    {
        shuffledRbgs[rbGroupIndex]._rbgIndex = rbGroupIndex;
        shuffledRbgs[rbGroupIndex]._rand = RANDOM_erand(randomSeed);
    }
    std::sort(
            shuffledRbgs.begin(), shuffledRbgs.end(), rbgMetricGreaterPtr());
#endif

    int ueIndex = 0;

#if SCH_LTE_ENABLE_RR_RANDOM_SORT
    for (int shuffledRbGroupIndex = 0;
        shuffledRbGroupIndex < numberOfRbGroup;
        ++shuffledRbGroupIndex)
    {
        int rbGroupIndex = shuffledRbgs[shuffledRbGroupIndex]._rbgIndex;
#else
    for (int rbGroupIndex = 0;
        rbGroupIndex < numberOfRbGroup;
        ++rbGroupIndex)
    {
#endif

        int rbIndex;

        if (rbGroupIndex < numberOfRbGroupL)
        {
            rbIndex = rbGroupIndex * rbGroupSizeL;
        }else
        {
            rbIndex = numberOfRbGroupL * rbGroupSizeL +
                    (rbGroupIndex - numberOfRbGroupL) * rbGroupSizeS;
        }

        schedulingResult[ueIndex].rnti = targetUes[ ueIndex ];

        // Allocate resource blocks
        schedulingResult[ueIndex].startResourceBlock = (UInt8)rbIndex;

        if (rbGroupIndex < numberOfRbGroupL)
        {
           schedulingResult[ueIndex].numResourceBlocks = rbGroupSizeL;
        }else
        {
           schedulingResult[ueIndex].numResourceBlocks = rbGroupSizeS;
        }

        ++ueIndex;
    }

    // Add offset for PUSCH
    for (int ueIndex = 0; ueIndex < numAllocatedUes; ++ueIndex)
    {
        schedulingResult[ueIndex].startResourceBlock += (UInt8)puschRbOffset;
    }

    // Calculate estimated SINR
    //------------------------------

    std::vector < double > estimatedSinr_dB(numAllocatedUes);

    for (int ueIndex = 0; ueIndex < numAllocatedUes; ++ueIndex)
    {
        calculateEstimatedSinrUl(
            targetUes[ueIndex],
            schedulingResult[ueIndex].numResourceBlocks,
            schedulingResult[ueIndex].startResourceBlock,
            estimatedSinr_dB[ueIndex]);
    }

    // Determine MCS index
    //------------------------

    for (int ueIndex = 0; ueIndex < numAllocatedUes; ++ueIndex)
    {

#ifdef LTE_BER_BASED

        // Select the maximum mcs which estimated BLER is
        // greater than or equal to target BLER.
        int mcsIndex = ulSelectMcs(
            schedulingResult[ueIndex].numResourceBlocks,
            estimatedSinr_dB[ueIndex],
            _targetBler);

        // No mcs selected
        if (mcsIndex < 0){
            mcsIndex = 0;
        }

        schedulingResult[ueIndex].mcsIndex = (UInt8)mcsIndex;

#ifdef LTE_LIB_LOG
        PhyLteGetAggregator(_node, _phyIndex)->regist(\
                                            schedulingResult[ueIndex].rnti,
                                            lte::Aggregator::UL_MCS,
                                            mcsIndex);
#endif

#endif
    }

    for (int ueIndex = 0; ueIndex < numAllocatedUes; ++ueIndex)
    {

        // Select the maximum mcs which estimated BLER is
        // greater than or equal to target BLER.
        int mcsIndex = ulSelectMcs(
            schedulingResult[ueIndex].numResourceBlocks,
            estimatedSinr_dB[ueIndex],
            _targetBler);

        // No mcs selected
        if (mcsIndex < 0){
            mcsIndex = 0;
        }

        schedulingResult[ueIndex].mcsIndex = (UInt8)mcsIndex;
    }

#ifdef LTE_LIB_LOG
    debugOutputEstimatedSinrLog(schedulingResult, estimatedSinr_dB);
#endif



#ifdef LTE_LIB_LOG
    // Output scheduling result logs
    debugOutputSchedulingResultLog(schedulingResult);
#endif

    // Finally, Purge invalid scheduling results
    //-------------------------------------------
    purgeInvalidSchedulingResults(schedulingResult);

    //End UL scheduling
}
