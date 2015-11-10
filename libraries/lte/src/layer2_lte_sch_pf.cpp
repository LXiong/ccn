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

#include "layer2_lte_sch_pf.h"
#ifdef LTE_LIB_LOG
#include "log_lte.h"
#endif

#define EFFECTIVE_SINR_BY_AVERAGE 1
#define LTE_BER_BASED 1

#define PfHogeFormat Debug2Format

// /**
// FUNCTION   :: LteSchedulerENBPf::LteSchedulerENBPf
// LAYER      :: MAC
// PURPOSE    :: Initialize PF scheduler
// PARAMETERS ::
// + node           : Node* : Node Pointer
// + interfaceIndex : int   : Interface index
// + nodeInput      : const NodeInput* : Pointer to node input
// RETURN::    void:         NULL
// **/
LteSchedulerENBPf::LteSchedulerENBPf(
    Node* node, int interfaceIndex, const NodeInput* nodeInput)
    : LteSchedulerENB(node, interfaceIndex, nodeInput)
{
    _filteringModule = NULL;

#if SCH_LTE_ENABLE_PF_RANDOM_SORT
    RANDOM_SetSeed(
            randomSeed,
            node->globalSeed,
            node->nodeId,
            MAC_PROTOCOL_LTE,
            interfaceIndex);
#endif

    initConfigurableParameters();
}

// /**
// FUNCTION     :: LteSchedulerENBPf::~LteSchedulerENBPf
// LAYER        :: MAC
// PURPOSE      :: Finalize PF scheduler
// PARAMETERS   ::
// RETURN::    void:         NULL
// **/
LteSchedulerENBPf::~LteSchedulerENBPf()
{
}

// /**
// FUNCTION   :: LteSchedulerENBPf::initConfigurableParameters
// LAYER      :: MAC
// PURPOSE    :: Initialize configurable parameter
// PARAMETERS::
// RETURN ::    void:         NULL
// **/
void LteSchedulerENBPf::initConfigurableParameters()
{
    // Call parent class's same function
    // LteSchedulerENB::initConfigurableParameters();

    // Load parameters for PF scheduler

    BOOL wasFound;
    int retInt;
    double retDouble;
    char errBuf[MAX_STRING_LENGTH] = {0};

    NodeAddress interfaceAddress =
        MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
            _node, _node->nodeId, _interfaceIndex);

    ////////////////////////////////////////////////////////////////
    // Read filter coefficient for average throughput
    ////////////////////////////////////////////////////////////////

    IO_ReadDouble(_node->nodeId,
        interfaceAddress,
        _nodeInput,
        "MAC-LTE-PF-FILTER-COEFFICIENT",
        &wasFound,
        &retDouble);

    if (wasFound == TRUE)
    {

        if (retDouble >= 0.0)
        {
            _pfFilterCoefficient = retDouble;
        }
        else
        {
            sprintf(errBuf,
                "Invalid PF filter coefficient %e. "
                "Should be greater than or equal to 0.0.", retDouble);

            ERROR_Assert(FALSE,errBuf);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "Filter coefficient for PF scheduler should be set."
            "Default value %f is used.",
            (SCH_LTE_DEFAULT_PF_FILTER_COEFFICIENT));
        ERROR_ReportWarning(warnBuf);

        // default value
        _pfFilterCoefficient = SCH_LTE_DEFAULT_PF_FILTER_COEFFICIENT;
    }

    ////////////////////////////////////////////////////////////////
    // Read uplink RB allocation unit
    ////////////////////////////////////////////////////////////////

    IO_ReadInt(_node->nodeId,
        interfaceAddress,
        _nodeInput,
        "MAC-LTE-PF-UL-RB-ALLOCATION-UNIT",
        &wasFound,
        &retInt);

    if (wasFound == TRUE)
    {
        if (retInt >= 1 && retInt < PHY_LTE_MAX_NUM_RB)
        {
            _ulRbAllocationUnit = retInt;
        }
        else
        {
            sprintf(errBuf,
                "Invalid PF UL RB allocation unit %d [RB]. "
                "Should be in the range of [1, %d]",
                retInt, PHY_LTE_MAX_NUM_RB - 1);

            ERROR_Assert(FALSE,errBuf);
        }
    }
    else
    {
        char warnBuf[MAX_STRING_LENGTH];
        sprintf(warnBuf,
            "RB allocation unit for PF uplink scheduling should be set."
            "Default value %d is used.",
            (SCH_LTE_DEFAULT_PF_UL_RB_ALLOCATION_UNIT));
        ERROR_ReportWarning(warnBuf);

        _ulRbAllocationUnit = SCH_LTE_DEFAULT_PF_UL_RB_ALLOCATION_UNIT;
    }
}

// /**
// FUNCTION   :: LteSchedulerENBPf::notifyPowerOn
// LAYER      :: MAC
// PURPOSE    :: Notify PF scheduler of power on.
// PARAMETERS ::
// RETURN::    void:         NULL
// **/
void LteSchedulerENBPf::notifyPowerOn()
{
    // Do nothing
}

// /**
// FUNCTION   :: LteSchedulerENBPf::notifyPowerOff
// LAYER      :: MAC
// PURPOSE    :: Notify PF scheduler of power off.
// PARAMETERS ::
// RETURN::    void:         NULL
// **/
void LteSchedulerENBPf::notifyPowerOff()
{
    // Do nothing
}

// /**
// FUNCTION   :: LteSchedulerENBPf::notifyAttach
// LAYER      :: MAC
// PURPOSE    :: Notify PF scheduler of UE attach
// PARAMETERS ::
// + rnti : const LteRnti& : RNTI of newly attached UE
// RETURN::    void:         NULL
// **/
void LteSchedulerENBPf::notifyAttach(const LteRnti& rnti)
{
    // Do nothing
    _filteringModule =
        LteLayer2GetLayer3Filtering(_node, _interfaceIndex);

    double avgThroughput;
    BOOL found;

    // Set DL initial average throughput
    found = _filteringModule->get(
        rnti, LTE_LIB_PF_AVERAGE_THROUGHPUT_DL,&avgThroughput);

    ERROR_Assert(found == FALSE, "Filtering entry exists.");

    _filteringModule->update(
        rnti, LTE_LIB_PF_AVERAGE_THROUGHPUT_DL,
        SCH_LTE_PF_INITIAL_AVERAGE_THROUGHPUT,
        _pfFilterCoefficient);

    // Set UL initial average throughput

    found = _filteringModule->get(
        rnti, LTE_LIB_PF_AVERAGE_THROUGHPUT_UL,&avgThroughput);

    ERROR_Assert(found == FALSE, "Filtering entry exists.");

    _filteringModule->update(
        rnti, LTE_LIB_PF_AVERAGE_THROUGHPUT_UL,
        SCH_LTE_PF_INITIAL_AVERAGE_THROUGHPUT,
        _pfFilterCoefficient);
}

// /**
// FUNCTION   :: LteSchedulerENBPf::notifyDetach
// LAYER      :: MAC
// PURPOSE    :: Notify PF scheduler of UE detach
// PARAMETERS ::
// + rnti : const LteRnti& : RNTI of detached UE
// RETURN::    void:         NULL
// **/
void LteSchedulerENBPf::notifyDetach(const LteRnti& rnti)
{
    _filteringModule =
        LteLayer2GetLayer3Filtering(_node, _interfaceIndex);

    // Set DL initial average throughput
    _filteringModule->remove(
        rnti, LTE_LIB_PF_AVERAGE_THROUGHPUT_DL);

    // Set UL initial average throughput
    _filteringModule->remove(
        rnti, LTE_LIB_PF_AVERAGE_THROUGHPUT_UL);
}

// /**
// FUNCTION   :: LteSchedulerENBPf::prepareForScheduleTti
// LAYER      :: MAC
// PURPOSE    :: Prepare for scheduling.
// PARAMETERS ::
// + ttiNumber : UInt64 : TTI number
// RETURN::    void:         NULL
// **/
void LteSchedulerENBPf::prepareForScheduleTti(UInt64 ttiNumber)
{
    LteScheduler::prepareForScheduleTti(ttiNumber);

#ifdef LTE_LIB_LOG
    debugOutputInterfaceCheckingLog();
#endif

#ifdef LTE_LIB_LOG
    std::stringstream log;
    ListLteRnti::const_iterator c_it;
    char buf[MAX_STRING_LENGTH];

    ListLteRnti tempList;
    Layer3LteGetSchedulableListSortedByConnectedTime(_node, _interfaceIndex,
        &tempList);
    const ListLteRnti* listLteRnti = &tempList;

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

#endif // LTE_LIB_LOG

#ifdef LTE_LIB_LOG
    debugOutputRbgAverageThroughputDl();
#endif

#ifdef LTE_LIB_LOG
    debugOutputRbgAverageThroughputUl();
#endif

}

// /**
// FUNCTION   :: LteSchedulerENBPf::determineTargetUes
// LAYER      :: MAC
// PURPOSE    :: Determine target UEs for scheduling
// PARAMETERS ::
// + downlink       : bool                  : Downlink scheduling or uplink
// + targetUes     : std::vector<LteRnti>* : Buffer for the target UEs stored
// RETURN     :: void : NULL
// **/
void LteSchedulerENBPf::determineTargetUes(
    bool downlink,
    std::vector < LteRnti >* targetUes)
{
    // Get list of RNTI of UEs in RRC_CONNECTED status

    ListLteRnti tempList;
    Layer3LteGetSchedulableListSortedByConnectedTime(_node, _interfaceIndex,
        &tempList);
    const ListLteRnti* listLteRnti = &tempList;

    if (downlink)
    {
        ListLteRnti::const_iterator it;
        for (it = listLteRnti->begin(); it != listLteRnti->end(); ++it)
        {
            if (dlIsTargetUe(*it))
            {
                targetUes->push_back(*it);
            }
        }
    }else
    {
        ListLteRnti::const_iterator it;
        for (it = listLteRnti->begin(); it != listLteRnti->end(); ++it)
        {
            if (ulIsTargetUe(*it))
            {
                targetUes->push_back(*it);
            }
        }
    }
}

// /**
// FUNCTION   :: LteSchedulerENBPf::getAverageThroughput
// LAYER      :: MAC
// PURPOSE    :: Calculate average throughput
// PARAMETERS ::
// + rnti          : const LteRnti& : RNTI of UE
// + allocatedBits : int : std::vector<LteRnti>* :
//                           Allocated bits for indicated UE in this TTI.
// + isDl          : bool : Downlink or not.
// RETURN     :: double : Average throughput [bps]
// **/
double LteSchedulerENBPf::getAverageThroughput(
    const LteRnti& rnti, int allocatedBits, bool isDl)
{
    LteMeasurementType measurementType
        = isDl ?
            LTE_LIB_PF_AVERAGE_THROUGHPUT_DL :
            LTE_LIB_PF_AVERAGE_THROUGHPUT_UL ;

    double averageThroughput;
    BOOL found =
        _filteringModule->get(
            rnti,
            measurementType,
            &averageThroughput);

    ERROR_Assert(found == TRUE, "Filtered throughput not found");

    double filteringAlpha = 1.0 / pow(2.0, _pfFilterCoefficient / 4.0);
    double instantThroughputAllocated =
            allocatedBits /
            (MacLteGetTtiLength(_node, _interfaceIndex) / (double)SECOND);

    averageThroughput =
            (1.0 - filteringAlpha) * averageThroughput
            + filteringAlpha * instantThroughputAllocated;

    ERROR_Assert(averageThroughput > 0.0, "Invalid average throughput");

    return averageThroughput;
}

// /**
// FUNCTION   :: LteSchedulerENBPf::scheduleDlTti
// LAYER      :: MAC
// PURPOSE    :: Execute scheduling for downlink transmission
// PARAMETERS ::
// + schedulingResult : std::vector<LteDlSchedulingResultInfo>& :
//                        List of scheduling result.
// RETURN     :: void : NULL
// **/
void LteSchedulerENBPf::scheduleDlTti(
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
        updateAverageThroughputDl(NULL);
        return;
    }


    int rbGroupSize = PhyLteGetRbGroupsSize(_node, _phyIndex, numRb);

    int numberOfRbGroup = (int)ceil((double)numRb / rbGroupSize);

    int lastRbGroupSize = rbGroupSize;

    if (numRb % rbGroupSize != 0)
    {
        lastRbGroupSize = numRb % rbGroupSize;
    }


    // Determine transmission scheme
    //--------------------------------
    // Transmission scheme and tarnamission mode is difference concept.
    //

    // Scheduling results for temporary use
    std::vector < LteDlSchedulingResultInfo > tmpResults(numTargetUes);

   for (int ueIndex = 0; ueIndex < numTargetUes; ++ueIndex)
    {
        tmpResults[ueIndex].rnti = targetUes[ueIndex];

        for (int rbIndex = 0; rbIndex < LTE_MAX_NUM_RB; ++rbIndex)
        {
            tmpResults[ueIndex].allocatedRb[rbIndex] = 0;
        }

        tmpResults[ueIndex].numResourceBlocks = 0;

        tmpResults[ueIndex].mcsIndex[0] = PHY_LTE_INVALID_MCS;
        tmpResults[ueIndex].mcsIndex[1] = PHY_LTE_INVALID_MCS;
    }

    for (int ueIndex = 0; ueIndex < numTargetUes; ++ueIndex){

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

        tmpResults[ueIndex].txScheme = PhyLteDetermineTransmissiomScheme(
            _node, _phyIndex,
            phyLteCqiReportInfo.riInfo,
            &numLayer, &numTransportBlocksToSend);

        tmpResults[ueIndex].numTransportBlock = numTransportBlocksToSend;
    }

    // Calculate estimated SINR
    //------------------------------
    // In case of downlink, estimated SINR is
    // retrieved as CQI fedback from UE

    std::vector < std::vector < double > > estimatedSinr_dB(numTargetUes);
    for (int ueIndex = 0; ueIndex < numTargetUes; ++ueIndex)
    {
        estimatedSinr_dB[ueIndex].resize(
                                    tmpResults[ueIndex].numTransportBlock);
    }

    for (int ueIndex = 0; ueIndex < numTargetUes; ++ueIndex)
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
            tbIndex < tmpResults[ueIndex].numTransportBlock;
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


    // Rbg allocation
    // ------------------------

    std::vector < std::vector < double > > pfMetric(numTargetUes);
    std::vector < std::vector < int > > allocatedBitsIf(numTargetUes);

    for (int ueIndex = 0; ueIndex < numTargetUes; ++ueIndex)
    {
        pfMetric[ueIndex].resize(numberOfRbGroup);
        allocatedBitsIf[ueIndex].resize(numberOfRbGroup);
    }

    // Container for allocated for each UEs
    std::vector < int > allocatedBits(numTargetUes, 0);

    // Sorted Pf metric values
    PfMetricContainer sortedPfMetric(numTargetUes * numberOfRbGroup);

    for (int ueIndex = 0; ueIndex < numTargetUes; ++ueIndex)
    {
        for (int rbgIndex = 0; rbgIndex < numberOfRbGroup; ++rbgIndex)
        {
            // Create PfMetric entries
            pfMetricEntry obj;

            obj._metricValue = &pfMetric[ueIndex][rbgIndex];
            obj._ueIndex = ueIndex;
            obj._rbgIndex = rbgIndex;
#if SCH_LTE_ENABLE_PF_RANDOM_SORT
            obj._rand = RANDOM_erand(randomSeed);
#endif
            sortedPfMetric[ueIndex * numberOfRbGroup + rbgIndex] = obj;
        }
    }

    calculatePfMetricsDl(
            -1, // Calculate for all of the UEs
            sortedPfMetric, allocatedBitsIf, tmpResults,
            estimatedSinr_dB, allocatedBits, targetUes,
            numberOfRbGroup, rbGroupSize, lastRbGroupSize);

    // sort Pf Metric values
    std::sort(
        sortedPfMetric.begin(), sortedPfMetric.end(), pfMetricGreaterPtr());

    // For first release, only wideband CQI is supported.
    // RBG allocation becomes easier.
    std::vector < bool > allocatedRbgs(numberOfRbGroup, false);

#ifdef LTE_LIB_LOG
    int logAllocationCounter = 0;
#endif

    while (!sortedPfMetric.empty())
    {
        int allocatedUeIndex  = sortedPfMetric.front()._ueIndex;
        int allocatedRbgIndex = sortedPfMetric.front()._rbgIndex;

        int thisRbGroupSize = (allocatedRbgIndex == numberOfRbGroup - 1) ?
            lastRbGroupSize : rbGroupSize;


        if (pfMetric[allocatedUeIndex][allocatedRbgIndex] < 0.0)
        {
#ifdef LTE_LIB_LOG
            ERROR_ReportWarning("All the RBGs are not allocated.");
#endif
            break; // Stop allocating RBGs
        }

        int startRbIndex = allocatedRbgIndex * rbGroupSize;

        for (int lRbIndex = 0; lRbIndex < thisRbGroupSize; ++lRbIndex)
        {
            int rbIndex = startRbIndex + lRbIndex;
            tmpResults[allocatedUeIndex].allocatedRb[rbIndex] = 1;
        }

        tmpResults[allocatedUeIndex].numResourceBlocks += thisRbGroupSize;

        allocatedBits[allocatedUeIndex] =
                allocatedBitsIf[allocatedUeIndex][allocatedRbgIndex];

#ifdef LTE_LIB_LOG
        std::stringstream ss_rnti;
        std::stringstream ss_metric;
        std::stringstream ss_rbg;
        char buf[MAX_STRING_LENGTH];

        PfMetricContainer::const_iterator log_s_it;
        log_s_it = sortedPfMetric.begin();
        for (; log_s_it != sortedPfMetric.end(); ++log_s_it)
        {
            ss_rnti << STR_RNTI(buf, targetUes[log_s_it->_ueIndex]) << ",";
            ss_rbg << log_s_it->_rbgIndex << ",";
            ss_metric << *(log_s_it->_metricValue) << ",";
        }

        lte::LteLog::PfHogeFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,cnt=,%d,rnti=,%s",
            "PfRbgAllocDl",
            logAllocationCounter,
            ss_rnti.str().c_str());

        lte::LteLog::PfHogeFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,cnt=,%d,rbgIndex=,%s",
            "PfRbgAllocDl",
            logAllocationCounter,
            ss_rbg.str().c_str());

        lte::LteLog::PfHogeFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,cnt=,%d,"
            "allocdPfMetric=,%e,allocdRbgIndex=,%d,allocdRnti=,%s,"
            "pfMetric=,%s",
            "PfRbgAllocDl",
            logAllocationCounter,
            *(sortedPfMetric.front()._metricValue),
            sortedPfMetric.front()._rbgIndex,
            STR_RNTI(buf, targetUes[allocatedUeIndex]),
            ss_metric.str().c_str());

        ++logAllocationCounter;
#endif

        // Purge elements in sortedPfMetric
        PfMetricContainer::iterator remaning_it =
            std::remove_if (
                sortedPfMetric.begin(), sortedPfMetric.end(),
                predRbgIndex(allocatedRbgIndex));

        sortedPfMetric.erase(remaning_it, sortedPfMetric.end());

        calculatePfMetricsDl(
            allocatedUeIndex, // Re-calculate PF metric for the selected UE
            sortedPfMetric,
            allocatedBitsIf,
            tmpResults,
            estimatedSinr_dB,
            allocatedBits,
            targetUes,
            numberOfRbGroup,
            rbGroupSize,
            lastRbGroupSize);

        // Resort Pf Metric values
        std::sort(sortedPfMetric.begin(),
                  sortedPfMetric.end(),
                  pfMetricGreaterPtr());

    }


    // Number of UEs for which at least one RB is allocated in this TTI
    int numAllocatedUes = 0;

    for (int ueIndex = 0; ueIndex < numTargetUes; ++ueIndex){
        if (tmpResults[ueIndex].numResourceBlocks > 0)
        {
            ++numAllocatedUes;
        }
    }

    if (numAllocatedUes == 0)
    {
        updateAverageThroughputDl(NULL);
        return;
    }

    // Allocate array in which to set scheduling results
    schedulingResult.resize(numAllocatedUes);
    LteDlSchedulingResultInfo* results = &schedulingResult[0];

    std::vector < std::vector < double >* >
                    allocatedUesEstimatedSinr_dB(numAllocatedUes);

    int allocatedUeIndex = 0;
    for (int ueIndex = 0; ueIndex < numTargetUes; ++ueIndex){
        if (tmpResults[ueIndex].numResourceBlocks > 0)
        {
            // Copy LteDlSchedulingResultInfo of allocated users
            results[allocatedUeIndex] = tmpResults[ueIndex];
            allocatedUesEstimatedSinr_dB[allocatedUeIndex] =
                                                &estimatedSinr_dB[ueIndex];
            ++allocatedUeIndex;
        }
    }

    // Determine MCS index
    //------------------------
    for (int allocatedUeIndex = 0;
        allocatedUeIndex < numAllocatedUes;
        ++allocatedUeIndex)
    {
        for (int tbIndex = 0;
            tbIndex < results[allocatedUeIndex].numTransportBlock;
            ++tbIndex)
        {
            int mcsIndex = dlSelectMcs(
                results[allocatedUeIndex].numResourceBlocks,
                allocatedUesEstimatedSinr_dB[allocatedUeIndex]->at(tbIndex),
                _targetBler);

            if (mcsIndex < 0)
            {
                mcsIndex = 0;
            }

            results[allocatedUeIndex].mcsIndex[tbIndex] = (UInt8)mcsIndex;
        }
    }

#ifdef LTE_LIB_LOG
    debugOutputEstimatedSinrLog(
                            schedulingResult, allocatedUesEstimatedSinr_dB);
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

    // Update average throughput for all UEs in the connected UE list
    //----------------------------------------------------------------
    updateAverageThroughputDl(&schedulingResult);

    // End of DL scheduling
}

// /**
// FUNCTION   :: LteSchedulerENBPf::scheduleUlTti
// LAYER      :: MAC
// PURPOSE    :: Execute scheduling for uplink transmission
// PARAMETERS ::
// + schedulingResult : std::vector<LteDlSchedulingResultInfo>& :
//                        List of scheduling result.
// RETURN     :: void : NULL
// **/
void LteSchedulerENBPf::scheduleUlTti(
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
       updateAverageThroughputUl(NULL);
       return;
   }

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
        updateAverageThroughputUl(NULL);
        return;
    }

    int rbGroupSize = _ulRbAllocationUnit;

    int numberOfRbGroup = (int)ceil((double)numRb / rbGroupSize);

    int lastRbGroupSize = rbGroupSize;

    if (numRb % rbGroupSize != 0)
    {
        lastRbGroupSize = numRb % rbGroupSize;
    }

    // Rbg allocation
    //---------------

    // Note that order of subscript of pfMetric is
    // contrary to that of DL pfMetric
    std::vector < std::vector < double > > pfMetric(numTargetUes);
    std::vector < std::vector < int > > allocatedBitsIf(numTargetUes);

    for (int ueIndex = 0; ueIndex < numTargetUes; ++ueIndex)
    {
        pfMetric[ueIndex].resize(numberOfRbGroup);
        allocatedBitsIf[ueIndex].resize(numberOfRbGroup);
    }

    // Sorted Pf metric values
    PfMetricContainer sortedPfMetric(numTargetUes * numberOfRbGroup);
    PfMetricContainer::iterator s_it;

    // RBG allocation
    std::vector < AllocatedRbgRange > allocatedRbg(numTargetUes);
    std::vector < int > allocatedBits(numTargetUes, 0);

    std::vector < bool > isRbgAllocated(numberOfRbGroup, false);

    for (int ueIndex = 0; ueIndex < numTargetUes; ++ueIndex)
    {
        for (int rbgIndex = 0; rbgIndex < numberOfRbGroup; ++rbgIndex)
        {
            // Create PfMetric entries
            pfMetricEntry obj;

            obj._metricValue = &pfMetric[ueIndex][rbgIndex];
            obj._ueIndex = ueIndex;
            obj._rbgIndex = rbgIndex;
#if SCH_LTE_ENABLE_PF_RANDOM_SORT
            obj._rand = RANDOM_erand(randomSeed);
#endif

            sortedPfMetric[ueIndex * numberOfRbGroup + rbgIndex] = obj;
        }
    }

    // Calculate 1st pf metrics
    calculatePfMetricsUl(
        -1, // Calculate PF metric for all of the UEs
        sortedPfMetric,
        allocatedBitsIf,
        allocatedRbg, allocatedBits,
        targetUes,
        numberOfRbGroup, rbGroupSize, lastRbGroupSize);

    // sort Pf Metric values
    std::sort(
        sortedPfMetric.begin(), sortedPfMetric.end(), pfMetricGreaterPtr());

    // Resource block group allocation
    std::vector < double > estimatedSinr_dB(numTargetUes);

#ifdef LTE_LIB_LOG
    int logAllocationCounter = 0;
#endif

    s_it = sortedPfMetric.begin();
    for (; s_it != sortedPfMetric.end();)
    {
        if (*(s_it->_metricValue) < 0.0)
        {
#ifdef LTE_LIB_LOG
            ERROR_ReportWarning("All the RBGs are not allocated (UL).");
#endif
            break; // Stop allocating RBGs
        }

        if (allocatedRbg[s_it->_ueIndex].add(s_it->_rbgIndex) != 0)
        {
            // _rgbIndex can be allocated to _ueIndex
            int allocatedUeIndex = s_it->_ueIndex;
            int allocatedRbgIndex = s_it->_rbgIndex;

            // Estimated SINR assuming current allocation
            calculateEstimatedSinrUl(
                targetUes[allocatedUeIndex],
                allocatedRbg[allocatedUeIndex].getNumAllocatedRbgs(),
                allocatedRbg[allocatedUeIndex].lower,
                estimatedSinr_dB[allocatedUeIndex]);

            // Allocate RBG g for UE k
            isRbgAllocated[allocatedRbgIndex] = true;

#ifdef LTE_LIB_LOG
            std::stringstream ss_rnti;
            std::stringstream ss_rbg;
            std::stringstream ss_metric;

            PfMetricContainer::const_iterator log_s_it;
            log_s_it = sortedPfMetric.begin();
            char buf[MAX_STRING_LENGTH];
            for (; log_s_it != sortedPfMetric.end(); ++log_s_it)
            {
                ss_rnti << STR_RNTI(buf, targetUes[log_s_it->_ueIndex])
                        << ",";
                ss_rbg << log_s_it->_rbgIndex << ",";
                ss_metric << *(log_s_it->_metricValue) << ",";
            }

            lte::LteLog::PfHogeFormat(
                _node,
                _interfaceIndex,
                LTE_STRING_LAYER_TYPE_SCHEDULER,
                "%s,cnt=,%d,rnti=,%s",
                "PfRbgAllocUl",
                logAllocationCounter,
                ss_rnti.str().c_str());

            lte::LteLog::PfHogeFormat(
                _node,
                _interfaceIndex,
                LTE_STRING_LAYER_TYPE_SCHEDULER,
                "%s,cnt=,%d,rbgIndex=,%s",
                "PfRbgAllocUl",
                logAllocationCounter,
                ss_rbg.str().c_str());

            lte::LteLog::PfHogeFormat(
                _node,
                _interfaceIndex,
                LTE_STRING_LAYER_TYPE_SCHEDULER,
                "%s,cnt=,%d,"
                "allocdPfMetric=,%e,allocdRbgIndex=,%d,allocdRnti=,%s,"
                "pfMetric=,%s",
                "PfRbgAllocUl",
                logAllocationCounter,
                *(s_it->_metricValue),
                s_it->_rbgIndex,
                STR_RNTI(buf, targetUes[s_it->_ueIndex]),
                ss_metric.str().c_str());

            ++logAllocationCounter;
#endif

            // Update allocated bits
            allocatedBits[allocatedUeIndex] =
                allocatedBitsIf[allocatedUeIndex][allocatedRbgIndex];

            // Purge elements in sortedPfMetric
            PfMetricContainer::iterator remaning_it =
                std::remove_if (
                    sortedPfMetric.begin(), sortedPfMetric.end(),
                    predRbgIndex(allocatedRbgIndex));

            sortedPfMetric.erase(remaning_it, sortedPfMetric.end());

            // Recalculate pf metrics
            calculatePfMetricsUl(
                allocatedUeIndex,
                sortedPfMetric,
                allocatedBitsIf,
                allocatedRbg, allocatedBits,
                targetUes,
                numberOfRbGroup, rbGroupSize, lastRbGroupSize);

            // Sort pf Metric value
            std::sort(sortedPfMetric.begin(),
                      sortedPfMetric.end(),
                      pfMetricGreaterPtr());

            // Return to head
            s_it = sortedPfMetric.begin();
        }else
        {
            // _rbgIndex is not an adjacent rbg for UE _ueIndex
            ++s_it;
        }
    }

    // Count number of Ues allocated
    int numAllocatedUes = 0;
    for (int ueIndex = 0; ueIndex < numTargetUes; ++ueIndex)
    {
        if (allocatedRbg[ueIndex].getNumAllocatedRbgs() > 0)
        {
            ++numAllocatedUes;
        }
    }

    if (numAllocatedUes == 0)
    {
        updateAverageThroughputUl(NULL);
        return;
    }

    // Set resource block allocation info to scheduling results
    schedulingResult.resize(numAllocatedUes);
    std::vector < double > allocatedUesEstimatedSinr_dB(numAllocatedUes);

    int allocatedUeIndex = 0;
    for (int ueIndex = 0; ueIndex < numTargetUes; ++ueIndex)
    {
        if (allocatedRbg[ueIndex].getNumAllocatedRbgs() > 0)
        {
            schedulingResult[allocatedUeIndex].rnti = targetUes[ueIndex];
            schedulingResult[allocatedUeIndex].startResourceBlock =
                (UInt8)(allocatedRbg[ueIndex].lower * rbGroupSize);


            // Count number of resource blocks to allocate
            schedulingResult[allocatedUeIndex].numResourceBlocks =
                    getNumAllocatedRbs(
                            allocatedRbg[ueIndex],
                            numberOfRbGroup,
                            rbGroupSize,
                            lastRbGroupSize);

            // Store estimated SINR for determining MCS later on.
            allocatedUesEstimatedSinr_dB[allocatedUeIndex] =
                estimatedSinr_dB[ueIndex];

            ++allocatedUeIndex;
        }
    }

    // Determine MCS index
    //------------------------

    for (int allocatedUeIndex = 0;
        allocatedUeIndex < numAllocatedUes;
        ++allocatedUeIndex)
    {

        // Select the maximum mcs which estimated BLER is
        // greater than or equal to target BLER.
        int mcsIndex = ulSelectMcs(
            schedulingResult[allocatedUeIndex].numResourceBlocks,
            allocatedUesEstimatedSinr_dB[allocatedUeIndex],
            _targetBler);

        // No mcs selected
        if (mcsIndex < 0){
            mcsIndex = 0;
        }

        schedulingResult[allocatedUeIndex].mcsIndex = (UInt8)mcsIndex;
    }

#ifdef LTE_LIB_LOG
    debugOutputEstimatedSinrLog(
                            schedulingResult, allocatedUesEstimatedSinr_dB);
#endif


#ifdef LTE_LIB_LOG
    // Output scheduling result logs
    debugOutputSchedulingResultLog(schedulingResult);
#endif

    // Finally, Purge invalid scheduling results
    //-------------------------------------------
    purgeInvalidSchedulingResults(schedulingResult);

    // Update average throughput for all UEs in the connected UE list
    //----------------------------------------------------------------
    updateAverageThroughputUl(&schedulingResult);

    // End of UL scheduling
}

// /**
// FUNCTION   :: LteSchedulerENBPf::calculatePfMetricsUl
// LAYER      :: MAC
// PURPOSE    :: Calculate PF metric value for UL scheduling
// PARAMETERS ::
// + targetUeIndex   : int : Target UE index
// + sortedPfMetric  : PfMetricContainer& : List of sorted PF metric values
// + allocatdBitsIf  : std::vector< std::vector<int> >& :
//                       Allocated bits for each UEs assuming remaining
//                       RBGs are allocated to them.
// + allocatedRbg    : const std::vector<AllocatedRbgRange>& :
//                       List of RBG allocation information for each UE
// + allocatedBits   : const std::vector<int>& :
//                       Allocated bits for each UEs
// + targetUes       : const std::vector<LteRnti>& :
//                       List of RNTIs of target UEs
// + numberOfRbGroup : int : Number of RBG
// + rbGroupSize     : int : Size of RBG
// + lastRbGroupSize : int : Size of last RBG.
// RETURN     :: void : NULL
// **/
void LteSchedulerENBPf::calculatePfMetricsUl(
    int targetUeIndex,
    PfMetricContainer& sortedPfMetric,
    std::vector < std::vector < int > > &allocatedBitsIf,
    const std::vector < AllocatedRbgRange > &allocatedRbg,
    const std::vector < int > &allocatedBits,
    const std::vector < LteRnti > &targetUes,
    int numberOfRbGroup, int rbGroupSize, int lastRbGroupSize)
{
    PfMetricContainer::iterator it;
    it = sortedPfMetric.begin();
    for (; it != sortedPfMetric.end(); ++it)
    {
        int ueIndex = it->_ueIndex;

        // If targetUeIndex is specified, calculate PF metric only for the UE
        if (targetUeIndex >= 0 && targetUeIndex != ueIndex)
        {
            continue;
        }

        int rbgIndex = it->_rbgIndex;

        int thisRbGroupSize = (rbgIndex == numberOfRbGroup - 1) ?
            lastRbGroupSize : rbGroupSize;

#if SCH_LTE_ENABLE_SIMPLE_PF_METRIC
        int pfMetricNumRbs = thisRbGroupSize;
#else
        int numAllocatedRbs =
            getNumAllocatedRbs(
                    allocatedRbg[ueIndex],
                    numberOfRbGroup,
                    rbGroupSize,
                    lastRbGroupSize);

        int pfMetricNumRbs =
            numAllocatedRbs + thisRbGroupSize;
#endif

        int side = allocatedRbg[ueIndex].isAdjacent(rbgIndex);

        // If rgbIndex is not adjacent rbg of currently allocated rbgs,
        // Pf metric value is set to 0.0, expected number of allocated
        // bits is equal to the current one.
        if (side == 0)
        {
            *it->_metricValue = 0.0;
            allocatedBitsIf[ueIndex][rbgIndex] = allocatedBits[ueIndex];
            continue;
        }

        int startRbgIndex;

        if (side == -1)
        {
            startRbgIndex = rbgIndex;
        }else
        {
            ERROR_Assert(side == 1, "Invalid adjacent index");
            startRbgIndex = allocatedRbg[ueIndex].lower;
        }

        // Calculate estimated SINR
        double rbgEstimatedSinr_dB;
        calculateEstimatedSinrUl(
            targetUes[ueIndex],
            pfMetricNumRbs,
            startRbgIndex,
            rbgEstimatedSinr_dB);

        int mcsIndexIf = ulSelectMcs(
            pfMetricNumRbs,
            rbgEstimatedSinr_dB,
            _targetBler);

        // No mcs selected
        if (mcsIndexIf < 0)
        {
            mcsIndexIf = 0;
        }

        int transportBlockSizeIf = PhyLteGetUlTxBlockSize(
            _node,
            _phyIndex,
            mcsIndexIf,
            pfMetricNumRbs);

        allocatedBitsIf[ueIndex][rbgIndex] = transportBlockSizeIf;

#if SCH_LTE_ENABLE_SIMPLE_PF_METRIC
        double instantThroughput =
            allocatedBitsIf[ueIndex][rbgIndex] /
            (MacLteGetTtiLength(_node, _interfaceIndex) / (double)SECOND);
#else
        double instantThroughput =
            (allocatedBitsIf[ueIndex][rbgIndex] - allocatedBits[ueIndex]) /
            (MacLteGetTtiLength(_node, _interfaceIndex) / (double)SECOND);
#endif

        double averageThroughput =
                getAverageThroughput(
                        targetUes[ueIndex],allocatedBits[ueIndex],false);

        *it->_metricValue = instantThroughput / averageThroughput;
    }

}

// /**
// FUNCTION   :: LteSchedulerENBPf::calculatePfMetricsDl
// LAYER      :: MAC
// PURPOSE    :: Calculate PF metric value for DL scheduling
// PARAMETERS ::
// + targetUeIndex    : int : Target UE index
// + sortedPfMetric   : PfMetricContainer& : List of sorted PF metric values
// + allocatdBitsIf   : std::vector< std::vector<int> >& :
//                        Allocated bits for each UEs assuming remaining
//                        RBGs are allocated to them.
// + tmpResults       : std::vector<LteDlSchedulingResultInfo>& :
//                        List of temporary resource allocation information
//                        for each UE.
// + estimatedSinr_dB : const std::vector< std::vector<double> >& :
//                        List of estimated SINR for each TB and UE.
// + allocatedBits    : const std::vector<int>& :
//                        Allocated bits for each UEs
// + targetUes        : const std::vector<LteRnti>& :
//                        List of RNTIs of target UEs
// + numberOfRbGroup  : int : Number of RBG
// + rbGroupSize      : int : Size of RBG
// + lastRbGroupSize  : int : Size of last RBG.
// RETURN     :: void : NULL
// **/
void LteSchedulerENBPf::calculatePfMetricsDl(
    int targetUeIndex,
    PfMetricContainer& sortedPfMetric,
    std::vector < std::vector < int > > &allocatedBitsIf,
    std::vector < LteDlSchedulingResultInfo > &tmpResults,
    const std::vector < std::vector < double > > &estimatedSinr_dB,
    const std::vector < int > &allocatedBits,
    const std::vector < LteRnti > &targetUes,
    int numberOfRbGroup, int rbGroupSize, int lastRbGroupSize)
{
    PfMetricContainer::iterator it;
    it = sortedPfMetric.begin();
    for (; it != sortedPfMetric.end(); ++it)
    {
        int ueIndex = it->_ueIndex;

        // If targetUeIndex is specified, calculate PF metric only for the UE
        if (targetUeIndex >= 0 && targetUeIndex != ueIndex)
        {
            continue;
        }

        int rbgIndex = it->_rbgIndex;

        int thisRbGroupSize = (rbgIndex == numberOfRbGroup - 1) ?
            lastRbGroupSize : rbGroupSize;

        allocatedBitsIf[ueIndex][rbgIndex] = 0;

        for (int tbIndex = 0;
            tbIndex < tmpResults[ueIndex].numTransportBlock;
            ++tbIndex)
        {
            // Add SINR estimation code when
            // subband CQI feedback is supported.
            // estimatedSinr_dB[ueInex][tbIndex] = AverageSinrOfAllocatedRbs

#if SCH_LTE_ENABLE_SIMPLE_PF_METRIC
            int pfMetricNumRbs = thisRbGroupSize;
#else
            int pfMetricNumRbs =
                    tmpResults[ueIndex].numResourceBlocks + thisRbGroupSize;
#endif

            int mcsIndexIf = dlSelectMcs(
                pfMetricNumRbs,
                estimatedSinr_dB[ueIndex][tbIndex],
                _targetBler);

            // No mcs selected
            if (mcsIndexIf < 0)
            {
                mcsIndexIf = 0;
            }

            int transportBlockSizeIf = PhyLteGetDlTxBlockSize(
                _node,
                _phyIndex,
                mcsIndexIf,
                pfMetricNumRbs);

            allocatedBitsIf[ueIndex][rbgIndex] += transportBlockSizeIf;
        }

#if SCH_LTE_ENABLE_SIMPLE_PF_METRIC
        double instantThroughput =
            allocatedBitsIf[ueIndex][rbgIndex] /
                (MacLteGetTtiLength(_node, _interfaceIndex) /
                (double)SECOND);
#else
        double instantThroughput =
            (allocatedBitsIf[ueIndex][rbgIndex] - allocatedBits[ueIndex]) /
                (MacLteGetTtiLength(_node, _interfaceIndex) /
                (double)SECOND);
#endif

        double averageThroughput = getAverageThroughput(
                targetUes[ueIndex],
                allocatedBits[ueIndex],
                true);

        *it->_metricValue = instantThroughput / averageThroughput;
    }
}

// /**
// FUNCTION   :: LteSchedulerENBPf::getNumAllocatedRbs
// LAYER      :: MAC
// PURPOSE    :: Get number of allocated RBs
// PARAMETERS ::
// + arr              : int : Instance of RBG range class.
// + rbGroupSize      : int : Size of RBG
// + lastRbGroupSize  : int : Size of last RBG.
// RETURN     :: void : NULL
// **/
int LteSchedulerENBPf::getNumAllocatedRbs(
    const AllocatedRbgRange& arr,
    int numberOfRbGroup,
    int rbGroupSize,
    int lastRbGroupSize)
{
    int numAllocatedRbs = 0;

    if (arr.getNumAllocatedRbgs() > 0)
    {
        if (arr.upper == (numberOfRbGroup - 1))
        {
            numAllocatedRbs =
                    (arr.getNumAllocatedRbgs() - 1) * rbGroupSize +
                    lastRbGroupSize;
        }else
        {
            numAllocatedRbs =
                    arr.getNumAllocatedRbgs() * rbGroupSize;
        }
    }

    return numAllocatedRbs;
}

// /**
// FUNCTION   :: LteSchedulerENBPf::updateAverageThroughputDl
// LAYER      :: MAC
// PURPOSE    :: Update average throughput for DL scheduling
// PARAMETERS ::
// + pSchedulingResult : const std::vector<LteDlSchedulingResultInfo>* :
//                         List of DL scheduling results
// RETURN     :: void : NULL
// **/
void LteSchedulerENBPf::updateAverageThroughputDl(
        const std::vector < LteDlSchedulingResultInfo > *pSchedulingResult)
{
    // Update average throughput
    // Note that average throughput for all of the UEs in the connected
    // UE list shall be updated even if no resources allocated.

    ListLteRnti tempList;
    Layer3LteGetSchedulableListSortedByConnectedTime(_node, _interfaceIndex,
        &tempList);
    const ListLteRnti* listLteRnti = &tempList;

    ListLteRnti::const_iterator it;

    if (pSchedulingResult == NULL)
    {
        for (it = listLteRnti->begin(); it != listLteRnti->end(); ++it)
        {
            _filteringModule->update(
                *it, LTE_LIB_PF_AVERAGE_THROUGHPUT_DL,
                0.0,
                _pfFilterCoefficient);
        }

        return;
    }

    const std::vector < LteDlSchedulingResultInfo > &schedulingResult
        = *pSchedulingResult;

    std::map < LteRnti, const LteDlSchedulingResultInfo* > allocatedUes;

    for (size_t i = 0; i < schedulingResult.size(); ++i)
    {
        allocatedUes.insert(
            std::pair < LteRnti, const LteDlSchedulingResultInfo* > (
                schedulingResult[i].rnti,&schedulingResult[i]));
    }

    for (it = listLteRnti->begin(); it != listLteRnti->end(); ++it)
    {
        double instantThroughput;
        std::map < LteRnti, const LteDlSchedulingResultInfo* >
            ::const_iterator allocated_itr = allocatedUes.find(*it);
        if (allocated_itr == allocatedUes.end())
        {
            // Not allocated
            instantThroughput = 0.0;
        }else
        {
            // allocated
            const LteDlSchedulingResultInfo* result = allocated_itr->second;

            int allocatedBits = 0;

            for (int tbIndex = 0;
                tbIndex < result->numTransportBlock;
                ++tbIndex)
            {
                int transportBlockSize = PhyLteGetDlTxBlockSize(
                    _node,
                    _phyIndex,
                    result->mcsIndex[tbIndex],
                    result->numResourceBlocks);

                allocatedBits += transportBlockSize;
            }

            instantThroughput = allocatedBits /
                                (MacLteGetTtiLength(_node, _interfaceIndex) /
                                (double)SECOND);

        }

        _filteringModule->update(
            *it, LTE_LIB_PF_AVERAGE_THROUGHPUT_DL,
            instantThroughput,
            _pfFilterCoefficient);
    }
}

// /**
// FUNCTION   :: LteSchedulerENBPf::updateAverageThroughputUl
// LAYER      :: MAC
// PURPOSE    :: Update average throughput for UL scheduling
// PARAMETERS ::
// + pSchedulingResult : const std::vector<LteUlSchedulingResultInfo>* :
//                         List of UL scheduling results
// RETURN     :: void : NULL
// **/
void LteSchedulerENBPf::updateAverageThroughputUl(
        const std::vector < LteUlSchedulingResultInfo >* pSchedulingResult)
{
    // Update average throughput
    // Note that average throughput for all of the UEs in the connected
    // UE list shall be updated even if no resources allocated.

    ListLteRnti tempList;
    Layer3LteGetSchedulableListSortedByConnectedTime(_node, _interfaceIndex,
        &tempList);
    const ListLteRnti* listLteRnti = &tempList;

    ListLteRnti::const_iterator it;

    if (pSchedulingResult == NULL)
    {
        for (it = listLteRnti->begin(); it != listLteRnti->end(); ++it)
        {
            _filteringModule->update(
                *it, LTE_LIB_PF_AVERAGE_THROUGHPUT_UL,
                0.0,
                _pfFilterCoefficient);
        }

        return;
    }

    const std::vector < LteUlSchedulingResultInfo > &schedulingResult
        = *pSchedulingResult;

    std::map < LteRnti, const LteUlSchedulingResultInfo* > allocatedUes;

    for (size_t i = 0; i < schedulingResult.size(); ++i)
    {
        allocatedUes.insert(
            std::pair < LteRnti, const LteUlSchedulingResultInfo* > (
                schedulingResult[i].rnti,&schedulingResult[i]));
    }

    for (it = listLteRnti->begin(); it != listLteRnti->end(); ++it)
    {
        double instantThroughput;
        std::map < LteRnti, const LteUlSchedulingResultInfo* > ::const_iterator
            allocated_itr = allocatedUes.find(*it);
        if (allocated_itr == allocatedUes.end())
        {
            // Not allocated
            instantThroughput = 0.0;
        }else
        {
            // allocated
            const LteUlSchedulingResultInfo* result = allocated_itr->second;

            int transportBlockSize = PhyLteGetUlTxBlockSize(
                _node,
                _phyIndex,
                result->mcsIndex,
                result->numResourceBlocks);

            instantThroughput = transportBlockSize /
                                (MacLteGetTtiLength(_node, _interfaceIndex) /
                                (double)SECOND);

        }

        _filteringModule->update(
            *it, LTE_LIB_PF_AVERAGE_THROUGHPUT_UL,
            instantThroughput,
            _pfFilterCoefficient);
    }
}

#ifdef LTE_LIB_LOG

void LteSchedulerENBPf::debugOutputRbgAverageThroughputDl()
{
    ListLteRnti tempList;
    Layer3LteGetSchedulableListSortedByConnectedTime(_node, _interfaceIndex,
        &tempList);
    const ListLteRnti* listLteRnti = &tempList;

    ListLteRnti::const_iterator it;

    for (it = listLteRnti->begin(); it != listLteRnti->end(); ++it)
    {
        double averageThroughput;
        char buf[MAX_STRING_LENGTH];
        _filteringModule->get(
            *it, LTE_LIB_PF_AVERAGE_THROUGHPUT_DL, &averageThroughput);

        std::stringstream ss;
        ss << "rnti=," << STR_RNTI(buf, *it) << ","
           << "avgThroughput=," << averageThroughput << ",";

        lte::LteLog::PfHogeFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            "PfAvgThroughtputDl",
            ss.str().c_str());
    }
}

void LteSchedulerENBPf::debugOutputRbgAverageThroughputUl()
{
    ListLteRnti tempList;
    Layer3LteGetSchedulableListSortedByConnectedTime(_node, _interfaceIndex,
        &tempList);
    const ListLteRnti* listLteRnti = &tempList;

    ListLteRnti::const_iterator it;

    for (it = listLteRnti->begin(); it != listLteRnti->end(); ++it)
    {
        double averageThroughput;
        char buf[MAX_STRING_LENGTH];
        _filteringModule->get(
            *it, LTE_LIB_PF_AVERAGE_THROUGHPUT_UL, &averageThroughput);

        std::stringstream ss;
        ss << "rnti=," << STR_RNTI(buf, *it) << ","
           << "avgThroughput=," << averageThroughput << ",";

        lte::LteLog::PfHogeFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            "PfAvgThroughtputUl",
            ss.str().c_str());
    }
}

#endif // LTE_LIB_LOG

