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

#include "layer2_lte_sch.h"
#include "layer3_lte_filtering.h"

#define ENABLE_SCH_IF_CHECKING_LOG 0
#define LTE_UMTS_RLC_DEF_PDUSIZE 3

#if LTE_LIB_LOG
///////////////////////////////////////////////////////////////////////////
/// Log related
///////////////////////////////////////////////////////////////////////////
const char* STR_RNTI(char* buf, const LteRnti& rnti)
{
#if 1
    sprintf(buf, LTE_STRING_FORMAT_RNTI, rnti.nodeId, rnti.interfaceIndex);
#else
    sprintf(buf, "%d", rnti.nodeId);
#endif
    return buf;
}
#endif

// /**
// FUNCTION   :: LteScheduler::LteScheduler
// LAYER      :: MAC
// PURPOSE    :: Initialize LteScheduler
// PARAMETERS ::
// RETURN     :: void : NULL
// **/
LteScheduler::LteScheduler()
{
    _node = NULL;
    _interfaceIndex = -1;
    _phyIndex = -1;
    _nodeInput = NULL;
    _ttiNumber = 0;

    // Error if default constructor called.
    char errorStr[MAX_STRING_LENGTH];
    sprintf(errorStr,
        "SCH-LTE: Invalid LteScheduler initializing");
    ERROR_Assert(FALSE, errorStr);
}

// /**
// FUNCTION   :: LteScheduler::LteScheduler
// LAYER      :: MAC
// PURPOSE    :: Initialize LteScheduler
// PARAMETERS::
// + node           : Node* : Node Pointer
// + interfaceIndex : int   : Interface index
// + nodeInput      : const NodeInput* : Pointer to node input
// RETURN ::    void:         NULL
// **/
LteScheduler::LteScheduler(
    Node* node, int interfaceIndex, const NodeInput* nodeInput)
{
    _node = node;
    _interfaceIndex = interfaceIndex;
    _phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);
    _nodeInput = nodeInput;
    _ttiNumber = 0;

    initConfigurableParameters();
}

// /**
// FUNCTION   :: LteScheduler::init
// LAYER      :: MAC
// PURPOSE    :: Initialize
// PARAMETERS::
// RETURN ::    void:         NULL
// **/
void LteScheduler::init()
{
    _phyIndex = LteGetPhyIndexFromMacInterfaceIndex(_node, _interfaceIndex);
}

// /**
// FUNCTION   :: LteScheduler::initConfigurableParameters
// LAYER      :: MAC
// PURPOSE    :: Initialize configurable parameter
// PARAMETERS::
// RETURN ::    void:         NULL
// **/
void LteScheduler::initConfigurableParameters()
{
}

// /**
// FUNCTION   :: LteScheduler::prepareForScheduleTti
// LAYER      :: MAC
// PURPOSE    :: Prepare for scheduling.
// PARAMETERS ::
// + ttiNumber : UInt64 : TTI number
// RETURN ::    void:         NULL
// **/
void LteScheduler::prepareForScheduleTti(UInt64 ttiNumber)
{
    _ttiNumber = ttiNumber;
}

// /**
// FUNCTION    :: LteScheduler::notifyPowerOn
// LAYER       :: MAC
// PURPOSE     :: Notify scheduler of power on.
// PARAMETERS  ::
// RETURN ::    void:         NULL
// **/
void LteScheduler::notifyPowerOn()
{
    // Do nothing
}

// /**
// FUNCTION::           LteScheduler::notifyPowerOff
// LAYER::              MAC
// PURPOSE::            Notify scheduler of power off.
// PARAMETERS::
// RETURN ::    void:         NULL
// **/
void LteScheduler::notifyPowerOff()
{
    // Do nothing
}

// /**
// FUNCTION   :: LteScheduler::notifyAttach
// LAYER      :: MAC
// PURPOSE    :: Notify scheduler of UE attach
// PARAMETERS ::
// + rnti : const LteRnti& : RNTI of newly attached UE
// RETURN ::    void:         NULL
// **/
void LteScheduler::notifyAttach(const LteRnti& rnti)
{
    // Do nothing
}

// /**
// FUNCTION   :: LteScheduler::notifyDetach
// LAYER      :: MAC
// PURPOSE    :: Notify scheduler of UE detach
// PARAMETERS ::
// + rnti : const LteRnti& : RNTI of detached UE
// RETURN ::    void:         NULL
// **/
void LteScheduler::notifyDetach(const LteRnti& rnti)
{
    // Do nothing
}

// /**
// FUNCTION   :: LteScheduler::determineDequeueSize
// LAYER      :: MAC
// PURPOSE    :: Determine dequeue size.
// PARAMETERS ::
// + transportBlockSize : int : Transport block size in bit
// RETURN ::    int : Dequeue size in byte.
// **/
int LteScheduler::determineDequeueSize(int transportBlockSize)
{
    return transportBlockSize / 8;
}

// /**
// FUNCTION   :: LteScheduler::~LteScheduler
// LAYER      :: MAC
// PURPOSE    :: Finalize LteScheduler
// PARAMETERS ::
// RETURN ::    void:         NULL
// **/
LteScheduler::~LteScheduler()
{
}

// /**
// FUNCTION   :: LteSchedulerENB::LteSchedulerENB
// LAYER      :: MAC
// PURPOSE    :: Initialize LteSchedulerENB
// PARAMETERS::
// + node           : Node* : Node Pointer
// + interfaceIndex : int   : Interface index
// + nodeInput      : const NodeInput* : Pointer to node input
// RETURN ::    void:         NULL
// **/
LteSchedulerENB::LteSchedulerENB(
    Node* node, int interfaceIndex, const NodeInput* nodeInput)
        : LteScheduler(node, interfaceIndex, nodeInput)
{
    initConfigurableParameters();
}

// /**
// FUNCTION   :: LteSchedulerENB::initConfigurableParameters
// LAYER      :: MAC
// PURPOSE    :: Initialize configurable parameter
// PARAMETERS::
// RETURN ::    void:         NULL
// **/
void LteSchedulerENB::initConfigurableParameters()
{
    // Load parameters for LteSchedulerENB

    BOOL wasFound;
    double retDouble;
    char errBuf[MAX_STRING_LENGTH] = {0};

    NodeAddress interfaceAddress =
        MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
            _node, _node->nodeId, _interfaceIndex);

    ////////////////////////////////////////////////////////////////
    // Read Target BLER
    ////////////////////////////////////////////////////////////////

    IO_ReadDouble(_node->nodeId,
        interfaceAddress,
        _nodeInput,
        "MAC-LTE-TARGET-BLER",
        &wasFound,
        &retDouble);

    if (wasFound == TRUE)
    {
        if (retDouble < 0.0 || retDouble > 1.0)
        {
            sprintf(errBuf,
                "Invalid Target BLER %e. "
                "Should be in the range of [0.0, 1.0].", retDouble);

            ERROR_Assert(FALSE,errBuf);
        }

        _targetBler = retDouble;
    }
    else
    {
        _targetBler = MAC_LTE_DEFAULT_TARGET_BLER;
    }
}

// /**
// FUNCTION   :: LteSchedulerENB::~LteSchedulerENB
// LAYER      :: MAC
// PURPOSE    :: Finalize LteSchedulerENB
// PARAMETERS ::
// RETURN ::    void:         NULL
// **/
LteSchedulerENB::~LteSchedulerENB()
{
}

// /**
// FUNCTION   :: LteSchedulerENB::purgeInvalidSchedulingResults
// LAYER      :: MAC
// PURPOSE    :: Purge invalid DL scheduling results.
// PARAMETERS ::
// + schedulingResult : std::vector<LteDlSchedulingResultInfo>& :
//                        List of DL scheduling results.
// RETURN ::    void:         NULL
// **/
void LteSchedulerENB::purgeInvalidSchedulingResults(
    std::vector < LteDlSchedulingResultInfo > &schedulingResult)
{
    // Purge scheduling results which dequeueSize equals to 0

    std::vector < LteDlSchedulingResultInfo > ::iterator it;
    for (it = schedulingResult.begin(); it != schedulingResult.end();)
    {
        bool invalid = true;
        for (int tbIndex = 0; tbIndex < it->numTransportBlock; ++tbIndex)
        {
            if (it->dequeueInfo[tbIndex].dequeueSizeByte > 0)
            {
                invalid = false;
            }
        }

        if (invalid)
        {
            it = schedulingResult.erase(it);
        }else
        {
            ++it;
        }
    }
}

// /**
// FUNCTION   :: LteSchedulerENB::purgeInvalidSchedulingResults
// LAYER      :: MAC
// PURPOSE    :: Purge invalid UL scheduling results.
// PARAMETERS ::
// + schedulingResult : std::vector<LteDlSchedulingResultInfo>& :
//                        List of UL scheduling results.
// RETURN ::    void:         NULL
// **/
void LteSchedulerENB::purgeInvalidSchedulingResults(
    std::vector < LteUlSchedulingResultInfo > &schedulingResult)
{
    // Do nothing for UL scheduling at eNB scheduler
}

// /**
// FUNCTION   :: LteSchedulerENB::createDequeueInformation
// LAYER      :: MAC
// PURPOSE    :: Create dequeue information for each DL scheduling result.
// PARAMETERS ::
// + schedulingResult : std::vector<LteDlSchedulingResultInfo>& :
//                        List of scheduling results.
// RETURN ::    void:         NULL
// **/
void LteSchedulerENB::createDequeueInformation(
    std::vector < LteDlSchedulingResultInfo > &schedulingResult)
{
    for (size_t ueIndex = 0; ueIndex < schedulingResult.size(); ++ueIndex)
    {
        for (int tbIndex = 0;
            tbIndex < schedulingResult[ueIndex].numTransportBlock;
            ++tbIndex)
        {
            schedulingResult[ueIndex].dequeueInfo[tbIndex].bearerId =
                LTE_DEFAULT_BEARER_ID;

            int transportBlockSize = PhyLteGetDlTxBlockSize(
                _node,
                _phyIndex,
                schedulingResult[ueIndex].mcsIndex[tbIndex],
                schedulingResult[ueIndex].numResourceBlocks);

            schedulingResult[ueIndex].dequeueInfo[tbIndex].dequeueSizeByte =
                determineDequeueSize(transportBlockSize);
        }
    }
}

#ifdef LTE_LIB_LOG

#define HogeFormat InfoFormat

void LteSchedulerENB::debugOutputInterfaceCheckingLog()
{
#if ENABLE_SCH_IF_CHECKING_LOG

    // 1.
    const ListLteRnti* listLteRnti
        = Layer3LteGetConnectedList(_node, _interfaceIndex);

    ListLteRnti::const_iterator it;
    for (it = listLteRnti->begin(); it != listLteRnti->end(); ++it)
    {
        char ueRntiStr[MAX_STRING_LENGTH];
        STR_RNTI(ueRntiStr, *it);

        {
            // 2. Connected UE Info
            const LteConnectedInfo* lteConnectedInfo =
                Layer3LteGetConnectedInfo(_node, _interfaceIndex, *it);

            std::stringstream log;
            char buf[MAX_STRING_LENGTH];

            log << "Rnti=," << ueRntiStr << ",";

            // 2-0. Bearer
            log << "ConnTime=," << lteConnectedInfo->connectedTime << ",";

            // 2-1. Bearer

            MapLteRadioBearer::const_iterator itr;

            int cnt = 0;
            log << "Size=," << lteConnectedInfo->radioBearers.size() << ",";
            for (itr = lteConnectedInfo->radioBearers.begin();
                itr != lteConnectedInfo->radioBearers.end();
                ++itr)
            {
                log << itr->first << ",";
            }

            // 2-2. BSR

            log << "BufSize=,"
                << lteConnectedInfo->bsrInfo.bufferSizeByte << ","
                << "BufLevel=,"
                << lteConnectedInfo->bsrInfo.bufferSizeLevel << ","
                << "Rnti=,"
                << STR_RNTI(buf,lteConnectedInfo->bsrInfo.ueRnti) << ",";

            lte::LteLog::HogeFormat(
                _node,
                _interfaceIndex,
                LTE_STRING_LAYER_TYPE_SCHEDULER,
                "%s,%s",
                "IfCheckConnInfo",
                log.str().c_str());

        }

        {
            // 3. Sendable byte in queue

            std::stringstream log;
            char buf[MAX_STRING_LENGTH];

            log << "Rnti=," << ueRntiStr << ",";

            UInt32 byte = LteRlcSendableByteInQueue(
                _node, _interfaceIndex, *it, LTE_DEFAULT_BEARER_ID);

            log << "SendableByteInQueue=," << byte << ",";

            lte::LteLog::HogeFormat(
                _node,
                _interfaceIndex,
                LTE_STRING_LAYER_TYPE_SCHEDULER,
                "%s,%s",
                "IfCheckSendableByteInQueue",
                log.str().c_str());

        }


        // UE measurement information
        {
            std::stringstream log;
            char buf[MAX_STRING_LENGTH];

            log << "Rnti=," << ueRntiStr << ",";

            std::vector < double > ufp_dB;
            PhyLteGetUlFilteredPathloss(
                                    _node, _phyIndex, *it, &ufp_dB);

            log << "UlFilteredPloss=,";
            for (int i = 0; i < ufp_dB.size(); ++i)
            {
                log << ufp_dB[i] << ",";
            }

            std::vector < double > uip_dB;
            PhyLteGetUlInstantPathloss(_node, _phyIndex, *it, &uip_dB);


            log << "UlInstantPloss=,";
            for (int i = 0; i < uip_dB.size(); ++i)
            {
                log << uip_dB[i] << ",";
            }

            lte::LteLog::HogeFormat(
                _node,
                _interfaceIndex,
                LTE_STRING_LAYER_TYPE_SCHEDULER,
                "%s,%s",
                "IfCheckUlPloss",
                log.str().c_str());

        }

        // Dynamic context informations

        {
            std::stringstream log;
            char buf[MAX_STRING_LENGTH];

            log << "Rnti=," << ueRntiStr << ",";

            PhyLteCqiReportInfo cri;
            PhyLteGetCqiInfoFedbackFromUe(_node, _phyIndex, *it, &cri);

            log << "cqi0=," << cri.cqiInfo.cqi0 << ","
                << "cqi1=," << cri.cqiInfo.cqi1 << ","
                << "ri=," << cri.riInfo << ",";

            lte::LteLog::HogeFormat(
                _node,
                _interfaceIndex,
                LTE_STRING_LAYER_TYPE_SCHEDULER,
                "%s,%s",
                "IfCheckCqiRi",
                log.str().c_str());

        }

        // UE information
        {
            std::stringstream log;
            char buf[MAX_STRING_LENGTH];

            log << "Rnti=," << ueRntiStr << ",";

            double umtp_dBm;
            PhyLteGetUlMaxTxPower_dBm(
                                    _node, _phyIndex, *it, &umtp_dBm);

            log << "UlMaxTxPower_dBm=," << umtp_dBm << ",";

            lte::LteLog::HogeFormat(
                _node,
                _interfaceIndex,
                LTE_STRING_LAYER_TYPE_SCHEDULER,
                "%s,%s",
                "IfCheckUlMaxTxPower",
                log.str().c_str());

        }
    }

     // System fixed informations

    {
        std::stringstream log;
        char buf[MAX_STRING_LENGTH];

        Float64* cst;
        int cst_len;
        PhyLteGetCqiSnrTable(_node, _phyIndex, &cst, &cst_len);

        log << "CqiSnrTblLen=," << cst_len << ",";
        for (int i = 0; i < cst_len; ++i)
        {
            log << cst[i] << ",";
        }

        lte::LteLog::HogeFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            "IfCheckCqiSnrTbl",
            log.str().c_str());
    }

    {
        std::stringstream log;
        char buf[MAX_STRING_LENGTH];

        log << "Size=," << 110 << ",RbGrpSize=,";
        for (int numRb = 0; numRb < 110; ++numRb)
        {
            log << (int)(PhyLteGetRbGroupsSize(_node, _phyIndex,numRb))
                << ",";
        }
        lte::LteLog::HogeFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            "IfCheckRbGrpSize",
            log.str().c_str());
    }

    // Enb parmaeters
    {
        std::stringstream log;
        char buf[MAX_STRING_LENGTH];


        log << "NumRb=," <<
            (int)(PhyLteGetNumResourceBlocks(_node, _phyIndex)) << ",";

        log << "NumRxAntennas=," <<
            (int)(PhyLteGetNumRxAntennas(_node, _phyIndex)) << ",";

        log << "ThermalNoise=," <<
            PhyLteGetThermalNoise(_node, _phyIndex) << ",";

        log << "PucchOverhead=," <<
            PhyLteGetUlCtrlChannelOverhead(_node, _phyIndex) << ",";

        log << "TpcAlpha=," <<
            PhyLteGetTpcAlpha(_node, _phyIndex) << ",";

        log << "TpcPopusch=," <<
            PhyLteGetTpcPoPusch(_node, _phyIndex) << ",";

        lte::LteLog::HogeFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            "IfCheckEnbParam",
            log.str().c_str());

    }

    // Enb measurements
    {
        std::stringstream log;
        char buf[MAX_STRING_LENGTH];

        double* urip;
        int urip_len;
        PhyLteGetUlRxIfPower(_node, _phyIndex, &urip, &urip_len);

        log << "Size=," << urip_len << ","
            << "UlRxIfPower=,";
        for (int i = 0; i < urip_len; ++i)
        {
            log << urip[i] << ",";
        }
    }
#endif // ENABLE_SCH_IF_CHECKING_LOG
}

void LteSchedulerENB::debugOutputSchedulingResultLog(
    const std::vector < LteDlSchedulingResultInfo > &schedulingResult)
{
    int numRb = PhyLteGetNumResourceBlocks(_node, _phyIndex);

    {
        char buf[MAX_STRING_LENGTH];

        for (int ueIndex = 0; ueIndex < schedulingResult.size(); ++ueIndex)
        {
            std::stringstream log;

            log << "TtiNum=," << _ttiNumber << ","
                << "Rnti=," << STR_RNTI(buf, schedulingResult[ueIndex].rnti)
                << "," << "AllocRb=,";

            for (int rbIndex = 0; rbIndex < LTE_MAX_NUM_RB; ++rbIndex)
            {
                log << (int)(schedulingResult[ueIndex].allocatedRb[rbIndex])
                    << ",";
            }

            log << "TxScheme=,"
                << LteGetTxSchemeString(schedulingResult[ueIndex].txScheme)
                << "," << "NumTb=,"
                << schedulingResult[ueIndex].numTransportBlock
                << "," << "Mcs0=,"
                << (int)(schedulingResult[ueIndex].mcsIndex[0])
                << "," << "BearerId0=,"
                << schedulingResult[ueIndex].dequeueInfo[0].bearerId
                << "," << "DequeueSize0=,"
                << schedulingResult[ueIndex].dequeueInfo[0].dequeueSizeByte
                << "," << "Mcs1=,"
                << (int)(schedulingResult[ueIndex].mcsIndex[1])
                << "," << "BearerId1=,"
                << schedulingResult[ueIndex].dequeueInfo[1].bearerId
                << "," << "DequeueSize1=,"
                << schedulingResult[ueIndex].dequeueInfo[1].dequeueSizeByte;


            lte::LteLog::Debug2Format(
                _node,
                _interfaceIndex,
                LTE_STRING_LAYER_TYPE_SCHEDULER,
                "%s,%s",
                LTE_STRING_CATEGORY_TYPE_RAW_RESULT_DL,
                log.str().c_str());
        }
    }

    {
        std::stringstream log;
        std::vector < LteRnti > ::const_iterator c_it;
        char buf[MAX_STRING_LENGTH];

        // Make RB to UEResultMaps
        LteDlSchedulingResultInfo tmpDlSchedulingResultInfo;
        tmpDlSchedulingResultInfo.rnti = LTE_INVALID_RNTI;
        tmpDlSchedulingResultInfo.txScheme = TX_SCHEME_INVALID;
        tmpDlSchedulingResultInfo.mcsIndex[0] = PHY_LTE_INVALID_MCS;
        tmpDlSchedulingResultInfo.mcsIndex[1] = PHY_LTE_INVALID_MCS;
        tmpDlSchedulingResultInfo.numTransportBlock = 0;
        tmpDlSchedulingResultInfo.numResourceBlocks = 0;

        std::vector < const LteDlSchedulingResultInfo* > rbToSchResultMap(
                                        numRb, &tmpDlSchedulingResultInfo);
        for (int ueIndex = 0; ueIndex < schedulingResult.size(); ++ueIndex)
        {
            for (int rbIndex = 0; rbIndex < numRb; ++rbIndex)
            {
                if (schedulingResult[ueIndex].allocatedRb[rbIndex] == 1)
                {
                    ERROR_Assert(rbToSchResultMap[rbIndex] ==
                                    &tmpDlSchedulingResultInfo,
                                "Resource block allocation is overlapped");

                    rbToSchResultMap[rbIndex] = &schedulingResult[ueIndex];
                }
            }
        }

        // 1. RNTI
        log << "TtiNum=," << _ttiNumber << ","
            << "Rnti=,";
        for (int rbIndex = 0; rbIndex < numRb; ++rbIndex)
        {
            log << STR_RNTI(buf, rbToSchResultMap[rbIndex]->rnti) << ",";
        }

        lte::LteLog::InfoFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RESULT_DL,
            log.str().c_str());
        log.str("");

       // 2. TxScheme
        log << "TtiNum=," << _ttiNumber << ","
            << "TxScheme=,";
        for (int rbIndex = 0; rbIndex < numRb; ++rbIndex)
        {
            log << LteGetTxSchemeString(rbToSchResultMap[rbIndex]->txScheme)
                << ",";
        }

        lte::LteLog::InfoFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RESULT_DL,
            log.str().c_str());
        log.str("");

       // 3. Number of transport block
        log << "TtiNum=," << _ttiNumber << ","
            << "NumTb=,";
        for (int rbIndex = 0; rbIndex < numRb; ++rbIndex)
        {
            log <<rbToSchResultMap[rbIndex]->numTransportBlock << ",";
        }

        lte::LteLog::InfoFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RESULT_DL,
            log.str().c_str());
        log.str("");

       // 4. MCS for TB0
        log << "TtiNum=," << _ttiNumber << ","
            << "Mcs0=,";
        for (int rbIndex = 0; rbIndex < numRb; ++rbIndex)
        {
            log << (int)(rbToSchResultMap[rbIndex]->mcsIndex[0]) << ",";
        }

        lte::LteLog::InfoFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RESULT_DL,
            log.str().c_str());
        log.str("");

       // 5. MCS for TB1
        log << "TtiNum=," << _ttiNumber << ","
            << "Mcs1=,";
        for (int rbIndex = 0; rbIndex < numRb; ++rbIndex)
        {
            log << (int)(rbToSchResultMap[rbIndex]->mcsIndex[1]) << ",";
        }

        lte::LteLog::InfoFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RESULT_DL,
            log.str().c_str());
    }
}

void LteSchedulerENB::debugOutputSchedulingResultLog(
    const std::vector < LteUlSchedulingResultInfo > &schedulingResult)
{
    int numRb = PhyLteGetNumResourceBlocks(_node, _phyIndex);

    {
        char buf[MAX_STRING_LENGTH];

        for (int ueIndex = 0; ueIndex < schedulingResult.size(); ++ueIndex)
        {
            std::stringstream log;

            log << "Rnti=,"
                << STR_RNTI(buf, schedulingResult[ueIndex].rnti)
                << "," << "StartRb=,"
                << (int)(schedulingResult[ueIndex].startResourceBlock)
                << "," << "NumRb=,"
                << schedulingResult[ueIndex].numResourceBlocks
                << "," << "Mcs0=,"
                << (int)(schedulingResult[ueIndex].mcsIndex)
                << "," << "BearerId=,"
                << schedulingResult[ueIndex].dequeueInfo.bearerId
                << "," << "DequeueSize=,"
                << schedulingResult[ueIndex].dequeueInfo.dequeueSizeByte;


            lte::LteLog::Debug2Format(
                _node,
                _interfaceIndex,
                LTE_STRING_LAYER_TYPE_SCHEDULER,
                "%s,%s",
                LTE_STRING_CATEGORY_TYPE_RAW_RESULT_UL,
                log.str().c_str());
        }
    }

    {
        std::stringstream log;
        std::vector < LteRnti > ::const_iterator c_it;
        char buf[MAX_STRING_LENGTH];

        LteUlSchedulingResultInfo tmpUlSchedulingResultInfo;
        tmpUlSchedulingResultInfo.rnti = LTE_INVALID_RNTI;
        tmpUlSchedulingResultInfo.mcsIndex = PHY_LTE_INVALID_MCS;

        // Make RB to UEResultMaps
        std::vector < const LteUlSchedulingResultInfo* > rbToSchResultMap(
                                        numRb, &tmpUlSchedulingResultInfo);
        for (int ueIndex = 0; ueIndex < schedulingResult.size(); ++ueIndex)
        {

            for (int lRbIndex = 0;
                lRbIndex < schedulingResult[ueIndex].numResourceBlocks;
                ++lRbIndex)
            {
                int rbIndex =
                    schedulingResult[ueIndex].startResourceBlock + lRbIndex;

                ERROR_Assert(rbToSchResultMap[rbIndex] ==
                                &tmpUlSchedulingResultInfo,
                            "Resource block allocation is overlapping");

                rbToSchResultMap[rbIndex] = &schedulingResult[ueIndex];
            }
        }

        // 1. RNTI
        log << "TtiNum=," << _ttiNumber << ","
            << "Rnti=,";
        for (int rbIndex = 0; rbIndex < numRb; ++rbIndex)
        {
            if (rbToSchResultMap[rbIndex])
            {
                log << STR_RNTI(buf, rbToSchResultMap[rbIndex]->rnti) << ",";
            }else
            {
                static LteRnti invalidRnti(-1, -1);
                log << STR_RNTI(buf, invalidRnti) << ",";
            }
        }

        lte::LteLog::InfoFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RESULT_UL,
            log.str().c_str());

        log.str("");

       // 2. MCS0
        log << "TtiNum=," << _ttiNumber << ","
            << "Mcs0=,";
        for (int rbIndex = 0; rbIndex < numRb; ++rbIndex)
        {
            if (rbToSchResultMap[rbIndex])
            {
                log << (int)(rbToSchResultMap[rbIndex]->mcsIndex) << ",";
            }else
            {
                log << -1 << ",";
            }
        }

        lte::LteLog::InfoFormat(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RESULT_UL,
            log.str().c_str());
    }
}

void LteSchedulerENB::debugOutputEstimatedSinrLog(
    std::vector < LteDlSchedulingResultInfo > &schedulingResult,
    std::vector < std::vector < double >* > &estimatedSinr_dB)
{
    std::vector < std::vector < double > >
                            dammy_estimatedSinr_dB(estimatedSinr_dB.size());

    for (int i = 0; i < estimatedSinr_dB.size(); ++i)
    {
        dammy_estimatedSinr_dB[i] = *estimatedSinr_dB[i];
    }

    debugOutputEstimatedSinrLog(schedulingResult, dammy_estimatedSinr_dB);
}

void LteSchedulerENB::debugOutputEstimatedSinrLog(
    std::vector < LteDlSchedulingResultInfo > &schedulingResult,
    std::vector < std::vector < double > > &estimatedSinr_dB)
{
#if LTE_LIB_LOG

    for (int ueIndex = 0; ueIndex < schedulingResult.size(); ++ueIndex)
    {
        stringstream log;
        char buf[MAX_STRING_LENGTH];
        log << "Rnti=," << STR_RNTI(buf, schedulingResult[ueIndex].rnti);
        log << "," << "EstmSinr=,";
        for (int tbIndex = 0;
            tbIndex < schedulingResult[ueIndex].numTransportBlock;
            ++tbIndex)
        {
            log << estimatedSinr_dB[ueIndex][tbIndex] << ",";
        }
        log << "Mcs=,";
        for (int tbIndex = 0;
            tbIndex < schedulingResult[ueIndex].numTransportBlock;
            ++tbIndex)
        {
            log << (int)(schedulingResult[ueIndex].mcsIndex[tbIndex]) << ",";
        }
        lte::LteLog::Debug2Format(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            "McsDl",
            log.str().c_str());

        for (int tbIndex = 0;
            tbIndex < schedulingResult[ueIndex].numTransportBlock;
            ++tbIndex)
        {
            PhyLteGetAggregator(_node, _phyIndex)->regist(
                    schedulingResult[ueIndex].rnti,
                lte::Aggregator::DL_ESTIMATED_TB_SINR,
                estimatedSinr_dB[ueIndex][tbIndex]);
        }

#ifdef LTE_LIB_VALIDATION_LOG
        for (int tbIndex = 0;
            tbIndex < schedulingResult[ueIndex].numTransportBlock;
            ++tbIndex)
        {
            schedulingResult[ueIndex].estimatedSinr_dB[tbIndex] =
                                        estimatedSinr_dB[ueIndex][tbIndex];
        }
#endif
    }

#endif
}

void LteSchedulerENB::debugOutputEstimatedSinrLog(
    std::vector < LteUlSchedulingResultInfo > &schedulingResult,
    std::vector < double > &estimatedSinr_dB)
{
    {
        for (int allocatedUeIndex = 0;
            allocatedUeIndex < schedulingResult.size();
            ++allocatedUeIndex)
        {
            stringstream log;
            char buf[MAX_STRING_LENGTH];
            log << "Rnti=,"
                << STR_RNTI(buf, schedulingResult[allocatedUeIndex].rnti)
                << "," << "EstmSinr=,"
                << estimatedSinr_dB[allocatedUeIndex] << ","
                << "Mcs=,"
                << (int)(schedulingResult[allocatedUeIndex].mcsIndex) << ",";

            lte::LteLog::Debug2Format(
                _node,
                _interfaceIndex,
                LTE_STRING_LAYER_TYPE_SCHEDULER,
                "%s,%s",
                "McsUl",
                log.str().c_str());

#ifdef LTE_LIB_VALIDATION_LOG
            schedulingResult[allocatedUeIndex].estimatedSinr_dB =
                                        estimatedSinr_dB[allocatedUeIndex];
#endif
        }
    }
}

#endif

// /**
// FUNCTION   :: LteSchedulerENB::purgeInvalidSchedulingResults
// LAYER      :: MAC
// PURPOSE    :: Purge invalid UL scheduling results.
// PARAMETERS ::
// + schedulingResult : std::vector<LteUlSchedulingResultInfo>& :
//                        List of scheduling results.
// RETURN ::    void:         NULL
// **/
void LteSchedulerUE::purgeInvalidSchedulingResults(
    std::vector < LteUlSchedulingResultInfo > &schedulingResult)
{
    // Purge scheduling results which dequeueSize equals to 0

    std::vector < LteUlSchedulingResultInfo > ::iterator it;
    for (it = schedulingResult.begin(); it != schedulingResult.end();)
    {
        bool invalid = true;

        if (it->dequeueInfo.dequeueSizeByte > 0)
        {
            invalid = false;
        }

        if (invalid)
        {
            it = schedulingResult.erase(it);
        }else
        {
            ++it;
        }
    }
}

// /**
// FUNCTION   :: LteSchedulerENB::calculateEstimatedSinrUl
// LAYER      :: MAC
// PURPOSE    :: Calculate UL estimated SINR
// PARAMETERS ::
// + rnti               : const LteRnti& : RNTI of UE
// + numResourceBlocks  : int : Number of resource blocks
// + startResourceBlock : int : Start number of allocated resource block
// + estimatedSinr_dB   : double& : Estimated SINR
// RETURN ::    void:         NULL
// **/
void LteSchedulerENB::calculateEstimatedSinrUl(
    const LteRnti& rnti,
    int numResourceBlocks,
    int startResourceBlock,
    double& estimatedSinr_dB)
{
    double averageSinr = 0.0;
    std::vector < double > ulFilteredPathloss_dB;
    std::vector < double > ulInstantPathloss_dB;
    int len;

    BOOL ret;

    // Uplink instant pathlosses for each received antennas
    ret = PhyLteGetUlInstantPathloss(
        _node,
        _phyIndex,
        rnti,
        &ulInstantPathloss_dB);

    double rbThermalNoisePower_mW =
        PhyLteGetThermalNoise(_node, _phyIndex);

    ERROR_Assert(ret, "UE not found.");

    // Uplink filtered pathlosses for each received antennas
    ret = PhyLteGetUlFilteredPathloss(
        _node,
        _phyIndex,
        rnti,
        &ulFilteredPathloss_dB);

    ERROR_Assert(ret, "UE not found.");

    // Interference power
#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING
    LteExponentialMean* filteredInterferencePower_dBm;
    PhyLteGetUlFilteredRxIfPower(
        _node,
        _phyIndex,
        &filteredInterferencePower_dBm,
        &len);
#else
    double* interferencePower_mW;
    PhyLteGetUlRxIfPower(
        _node,
        _phyIndex,
        &interferencePower_mW,
        &len);
#endif

    double maxTxPower_dBm;


    PhyLteGetUlMaxTxPower_dBm(
        _node, _phyIndex, rnti, &maxTxPower_dBm);

    double popusch = PhyLteGetTpcPoPusch(_node, _phyIndex);
    double alpha = PhyLteGetTpcAlpha(_node, _phyIndex);

    // User filtered pathloss of rx antenna 0 for uplink transmission
    // power estimation.
    double txPower_dBm = MIN(maxTxPower_dBm,
        (IN_DB((double)numResourceBlocks)) +
        popusch + (alpha * ulFilteredPathloss_dB[0]));

    double txRbPower_mW = NON_DB(txPower_dBm) /
        numResourceBlocks;

    for (int lRbIndex = 0; lRbIndex < numResourceBlocks; ++lRbIndex)
    {
        int rbIndex = lRbIndex + startResourceBlock;
        double sinr = 0.0;

        double interferencePower;

#if PHY_LTE_ENABLE_INTERFERENCE_FILTERING
        filteredInterferencePower_dBm[rbIndex].get(&interferencePower);
#if PHY_LTE_INTERFERENCE_FILTERING_IN_DBM
        interferencePower = NON_DB(interferencePower);
#endif
#else
        interferencePower = interferencePower_mW[rbIndex];
#endif

        // Calculate eistimated sinr for uplink transmission
        for (int r = 0;
            r < PhyLteGetNumRxAntennas(_node, _phyIndex);
            ++r)
        {
            double sinr_rx =
                txRbPower_mW / NON_DB(ulInstantPathloss_dB[r]) /
                (rbThermalNoisePower_mW + interferencePower);

            sinr += sinr_rx;

#if LTE_LIB_LOG
            {
                stringstream log;
                char buf[MAX_STRING_LENGTH];
                log << "RbIndex=," << rbIndex << ","
                    << "Rx=," << r << ","
                    << "MaxTxPower=," << NON_DB(maxTxPower_dBm) << ","
                    << "TxPower=," << NON_DB(txPower_dBm) << ","
                    << "NumAllocRb=," << numResourceBlocks << ","
                    << "TxRbPower=," << txRbPower_mW << ","
                    << "UlInstantPloss_dB=," << ulInstantPathloss_dB[r]
                    << "," << "RbThermalNoisePower=,"
                    << rbThermalNoisePower_mW << ","
                    << "IfPower=," << interferencePower << ","
                    << "Sinr=," << sinr_rx << ",";

                lte::LteLog::Debug2Format(
                    _node,
                    _interfaceIndex,
                    LTE_STRING_LAYER_TYPE_SCHEDULER,
                    "%s,%s",
                    "EstmSinrUl",
                    log.str().c_str());
            }
#endif
        }

        averageSinr += sinr;
    }

    averageSinr /= numResourceBlocks;

    estimatedSinr_dB = IN_DB(averageSinr);
}

// /**
// FUNCTION   :: LteSchedulerENB::dlIsTargetUe
// LAYER      :: MAC
// PURPOSE    :: Check if indicated UE is target UE for DL scheduling.
// PARAMETERS ::
// + rnti               : const LteRnti& : RNTI of UE
// RETURN ::    bool: true, if indicated UE is target UE. false, otherwise.
// **/
bool LteSchedulerENB::dlIsTargetUe(const LteRnti& rnti)
{
    // Retrieve size of data in RLC queue
    UInt32 sendableByteInQueue =
        LteRlcSendableByteInQueue(
            _node,
            _interfaceIndex,
            rnti,
            LTE_DEFAULT_BEARER_ID);

    // Retrieve CQI fedback from UE
    PhyLteCqiReportInfo phyLteCqiReportInfo;
    bool hasValidCqi = PhyLteGetCqiInfoFedbackFromUe(
        _node,
        _phyIndex,
        rnti,
        &phyLteCqiReportInfo);

    if ((sendableByteInQueue > 0) &&
        (hasValidCqi == true))
    {
        return true;
    }

    return false;
}

// /**
// FUNCTION   :: LteSchedulerENB::ulIsTargetUe
// LAYER      :: MAC
// PURPOSE    :: Check if indicated UE is target UE for UL scheduling.
// PARAMETERS ::
// + rnti               : const LteRnti& : RNTI of UE
// RETURN ::    bool: true, if indicated UE is target UE. false, otherwise.
// **/
bool LteSchedulerENB::ulIsTargetUe(const LteRnti& rnti)
{
    // Get conncted UE information related to MAC & Upper layers
    const LteConnectionInfo* lteConnectionInfo =
        Layer3LteGetConnectionInfo(_node, _interfaceIndex, rnti);

    // Whether or not SRS is received is judged by the existtence of
    // filtered pathloss for the UE.
    std::vector < double > ulFilteredPathloss_dB;
    BOOL exist = PhyLteGetUlFilteredPathloss(
        _node, _phyIndex, rnti, &ulFilteredPathloss_dB);

    if (lteConnectionInfo->connectedInfo.bsrInfo.bufferSizeLevel > 0
        && exist)
    {
        return true;
    }

    return false;
}

// /**
// FUNCTION   :: dlSelectMcs
// LAYER      :: MAC
// PURPOSE    :: Select MCS for uplink transmission
// PARAMETERS ::
// + numResourceBlocks   : int              : Number of resource blocks
// + spatialMultiplexing : bool             : Spatial multiplexing or not
//                                          (2 TB(or CW) transmission or not)
//                              In case of uplink, this parameter is ignored.
// + estimatedSinr_dB    : estimatedSinr_dB : Estimated SINR in dB
// + targetBler          : double           : Target bler
// RETURN     :: int : Selected MCS index.
//                     In case no MCS index satisfies targetBler, return -1
int LteSchedulerENB::dlSelectMcs(
    int numResourceBlocks,
    double estimatedSinr_dB,
    double targetBler)
{
     // Select the maximum mcs which estimated BLER is
    // greater than or equal to target BLER.
    int mcsIndex;

    for (mcsIndex = PHY_LTE_REGULAR_MCS_INDEX_LEN-1;
        mcsIndex >= 0;
        --mcsIndex){

        // Sinr for determining MCS
        double estimatedSinrForMcsSelection_dB =
                estimatedSinr_dB - MAC_LTE_SNR_OFFSET_FOR_DL_MCS_SELECTION;

        // Calculate bit error ratio
        double ber = PHY_BER(
            _node->phyData[_phyIndex],
            mcsIndex,
            NON_DB(estimatedSinrForMcsSelection_dB));

        int transportBlockSize = PhyLteGetDlTxBlockSize(
            _node,
            _phyIndex,
            mcsIndex,
            numResourceBlocks);


        // Calculate estimated block error rate
        double bler = 1.0 - pow(1.0 - ber, (double)transportBlockSize);

        if (bler <= targetBler)
        {
            break;
        }
    }

    return mcsIndex;
}

// /**
// FUNCTION   :: ulSelectMcs
// LAYER      :: MAC
// PURPOSE    :: Select MCS for uplink transmission
// PARAMETERS ::
// + numResourceBlocks   : int              : Number of resource blocks
// + estimatedSinr_dB    : estimatedSinr_dB : Estimated SINR in dB
// + targetBler          : double           : Target bler
// RETURN     :: int : Selected MCS index.
//                     In case no MCS index satisfies targetBler, return -1.
// **/
int LteSchedulerENB::ulSelectMcs(
    int numResourceBlocks,
    double estimatedSinr_dB,
    double targetBler)
{

    // Select the maximum mcs which estimated BLER is
    // greater than or equal to target BLER.
    int mcsIndex;

    for (mcsIndex = PHY_LTE_REGULAR_MCS_INDEX_LEN-1;
        mcsIndex >= 0;
        --mcsIndex){

        // Sinr for determining MCS
        double estimatedSinrForMcsSelection_dB =
                estimatedSinr_dB - MAC_LTE_SNR_OFFSET_FOR_UL_MCS_SELECTION;

        // Calculate bit error ratio
        double ber = PHY_BER(
            _node->phyData[_phyIndex],
            mcsIndex + PHY_LTE_MCS_INDEX_LEN,
            NON_DB(estimatedSinrForMcsSelection_dB));

        int transportBlockSize;

        transportBlockSize = PhyLteGetUlTxBlockSize(
            _node,
            _phyIndex,
            mcsIndex,
            numResourceBlocks);

        // Calculate estimated block error rate
        double bler = 1.0 - pow(1.0 - ber, (double)transportBlockSize);

        if (bler <= targetBler)
        {
            break;
        }
    }

    return mcsIndex;
}

// /**
// FUNCTION   :: LteSchedulerUE::purgeInvalidSchedulingResults
// LAYER      :: MAC
// PURPOSE    :: Purge invalid UL scheduling results.
// PARAMETERS ::
// + schedulingResult : std::vector<LteUlSchedulingResultInfo>& :
//                        List of scheduling results.
// RETURN ::    void:         NULL
// **/
void LteSchedulerUE::purgeInvalidSchedulingResults(
    std::vector < LteDlSchedulingResultInfo > &schedulingResult)
{
    // Error if default constructor called.
    char errorStr[MAX_STRING_LENGTH];
    sprintf(errorStr,
        "SCH-LTE: LteSchedulerUE::purgeInvalidSchedulingResults()"
        "is not supported.");
    ERROR_Assert(FALSE, errorStr);
}

// /**
// FUNCTION   :: LteSchedulerUE::LteSchedulerUE
// LAYER      :: MAC
// PURPOSE    :: Initialize LteScheduler
// PARAMETERS::
// + node           : Node* : Node Pointer
// + interfaceIndex : int   : Interface index
// + nodeInput      : const NodeInput* : Pointer to node input
// RETURN ::    void:         NULL
// **/
LteSchedulerUE::LteSchedulerUE(
    Node* node, int interfaceIndex, const NodeInput* nodeInput)
        : LteScheduler(node, interfaceIndex, nodeInput)
{
}

// /**
// FUNCTION   :: LteSchedulerUE::~LteSchedulerUE
// LAYER      :: MAC
// PURPOSE    :: Finalize LteSchedulerUE
// PARAMETERS ::
// RETURN ::    void:         NULL
// **/
LteSchedulerUE::~LteSchedulerUE()
{
}

// /**
// FUNCTION   :: LteSchedulerUE::scheduleDlTti
// LAYER      :: MAC
// PURPOSE    :: Execute DL scheduling
// PARAMETERS ::
// + schedulingResult : std::vector<LteDlSchedulingResultInfo>& :
//                        List of DL scheduling results
// RETURN ::    void:         NULL
// **/
void LteSchedulerUE::scheduleDlTti(
    std::vector < LteDlSchedulingResultInfo > &schedulingResult)
{
    // Error if default constructor called.
    char errorStr[MAX_STRING_LENGTH];
    sprintf(errorStr,
        "SCH-LTE: LteSchedulerUE::schedulerDlTti() is not supported.");
    ERROR_Assert(FALSE, errorStr);
}
