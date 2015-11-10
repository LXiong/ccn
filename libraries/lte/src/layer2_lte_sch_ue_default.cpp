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

#include "layer2_lte_sch_ue_default.h"
#ifdef LTE_LIB_LOG
#include "log_lte.h"
#endif

#ifdef LTE_LIB_LOG

#define ENABLE_SCH_UE_DEFAULT_IF_CHECKING_LOG  0

#define HogeFormat InfoFormat

void UeSchCheckInterfaces(UInt64 ttiNumber, Node* node, int interfaceIndex)
{
#if ENABLE_SCH_UE_DEFAULT_IF_CHECKING_LOG
    // 1.
    const ListLteRnti* listLteRnti
        = Layer3LteGetConnectedList(node, interfaceIndex);
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);

    if (listLteRnti->size() == 0)
    {
        return;
    }

    {
        std::stringstream log;
        char buf[MAX_STRING_LENGTH];
        ListLteRnti::const_iterator it;
        for (it = listLteRnti->begin(); it != listLteRnti->end(); ++it)
        {
            char rntiStr[MAX_STRING_LENGTH];
            STR_RNTI(rntiStr, *it);

            log << rntiStr << ",";
        }

        lte::LteLog::HogeFormat(
            node,
            interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            "IfCheckConnInfo",
            log.str().c_str());
    }

    ERROR_Assert(listLteRnti->size() == 1,
                "Number of connected eNB is not 1.");

    char enbRntiStr[MAX_STRING_LENGTH];
    LteRnti enbRnti = listLteRnti->front();
    STR_RNTI(enbRntiStr, enbRnti);


    {
        std::stringstream log;
        char buf[MAX_STRING_LENGTH];

        log << "enbRnti=," << enbRntiStr << ",";

        LteDciFormat0 df0;
        BOOL exist;
        exist = PhyLteGetDciForUl(
                            node, phyIndex, ttiNumber, FALSE, &df0);

        log << "Exist=," << exist << ","
            << "FreqHopFlag=," << df0.freqHopFlag << ","
            << "McsId=," << (int)df0.mcsID << ","
            << "ResAllocHdr=," << (int)df0.resAllocHeader << ","
            << "UsedRbStart=," << (int)df0.usedRB_start << ","
            << "UsedRbLen=," << (int)df0.usedRB_length << ",";

        lte::LteLog::HogeFormat(
            node,
            interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            "IfCheckDciForUe",
            log.str().c_str());
    }

    {


        std::stringstream log;
        char buf[MAX_STRING_LENGTH];

        log << "enbRnti=," << enbRntiStr << ",";

        const int numPattern = 8;
        const int mcs[numPattern]   = {0,4,8,12,16,20,24,28};
        const int numRb[numPattern] = {1,15,30,45,60,75,90,100};

        for (int i = 0; i < numPattern; ++i)
        {
            int assumedSize = PhyLteGetUlTxBlockSize(
                                    node, phyIndex, mcs[i], numRb[i]);

            int resSize = LteMacCheckMacPduSizeWithoutPadding(
                                node,
                                interfaceIndex,
                                enbRnti,
                                LTE_DEFAULT_BEARER_ID,
                                assumedSize);

            log << "Pattern[" << i << "]=,"
                << assumedSize << "," << resSize << ",";
        }

        lte::LteLog::HogeFormat(
            node,
            interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            "IfCheckMacPduSizeWoPadding",
            log.str().c_str());
    }
#endif // ENABLE_SCH_UE_DEFAULT_IF_CHECKING_LOG
}

#endif

// /**
// FUNCTION   :: LteSchedulerUEDefault::LteSchedulerUEDefault
// LAYER      :: MAC
// PURPOSE    :: Initialize UE default scheduler
// PARAMETERS ::
// + node           : Node* : Node Pointer
// + interfaceIndex : int   : Interface index
// + nodeInput      : const NodeInput* : Pointer to node input
// RETURN::    void:         NULL
// **/
LteSchedulerUEDefault::LteSchedulerUEDefault(
    Node* node, int interfaceIndex, const NodeInput* nodeInput)
    : LteSchedulerUE(node, interfaceIndex, nodeInput)
{

}

// /**
// FUNCTION   :: LteSchedulerUEDefault::~LteSchedulerUEDefault
// LAYER      :: MAC
// PURPOSE    :: Finalize UE default scheduler
// PARAMETERS ::
// RETURN::    void:         NULL
// **/
LteSchedulerUEDefault::~LteSchedulerUEDefault()
{

}

// /**
// FUNCTION   :: LteSchedulerUEDefault::prepareForScheduleTti
// LAYER      :: MAC
// PURPOSE    :: Prepare for scheduling.
// PARAMETERS ::
// + ttiNumber : UInt64 : TTI number
// RETURN::    void:         NULL
// **/
void LteSchedulerUEDefault::prepareForScheduleTti(
    UInt64 ttiNumber)
{
    LteScheduler::prepareForScheduleTti(ttiNumber);


#ifdef LTE_LIB_LOG
    UeSchCheckInterfaces(_ttiNumber, _node, _interfaceIndex);
#endif
}

// /**
// FUNCTION   :: LteSchedulerUEDefault::scheduleUlTti
// LAYER      :: MAC
// PURPOSE    :: Execute scheduling for uplink transmission
// PARAMETERS ::
// + schedulingResult : std::vector<LteUlSchedulingResultInfo>& :
//                        List of scheduling result
// RETURN     :: void : NULL
// **/
void LteSchedulerUEDefault::scheduleUlTti(
    std::vector < LteUlSchedulingResultInfo > &schedulingResult)
{
    // Initialize resutls
    schedulingResult.clear();

    // Retrieve DCI format 0
    LteDciFormat0 dciFormat0;

    BOOL allocated = TRUE;

    allocated = PhyLteGetDciForUl(
        _node,
        _phyIndex,
        _ttiNumber,
        true,
        &dciFormat0);


    if (FALSE == allocated)
    {
        return;
    }

    // Allocate array in which to set scheduling results
    schedulingResult.resize(1);

    LteUlSchedulingResultInfo* result = &schedulingResult[0];

    // Retrieve RNTI of serving eNB
    ListLteRnti tempList;
    Layer3LteGetSchedulableListSortedByConnectedTime(_node, _interfaceIndex,
        &tempList);
    const ListLteRnti* listLteRnti = &tempList;

    ERROR_Assert(listLteRnti->size() < 2, "Invalid connected node list");

    if (listLteRnti->size() == 0)
    {
        return;
    }

    ERROR_Assert(listLteRnti->size() == 1, "Invalid connected node list");
    LteRnti servingEnbRnti = listLteRnti->front();

    // Create dequeue information
    result->mcsIndex = dciFormat0.mcsID;
    result->numResourceBlocks = dciFormat0.usedRB_length;
    result->startResourceBlock = dciFormat0.usedRB_start;
    result->rnti = servingEnbRnti;
    result->dequeueInfo.bearerId = LTE_DEFAULT_BEARER_ID;

    int transportBlockSize = PhyLteGetUlTxBlockSize(
        _node,
        _phyIndex,
        result->mcsIndex,
        result->numResourceBlocks);

    result->dequeueInfo.dequeueSizeByte =
        determineDequeueSize(transportBlockSize);

#if LTE_LIB_LOG

    {
        char buf[MAX_STRING_LENGTH];

        std::stringstream log;

        log << "Rnti=," << STR_RNTI(buf,result->rnti) << ","
            << "StartRb=," << (int)(result->startResourceBlock) << ","
            << "NumRb=," << result->numResourceBlocks << ","
            << "Mcs0=," << (int)(result->mcsIndex) << ","
            << "BearerId=," << result->dequeueInfo.bearerId << ","
            << "DequeueSize=," << result->dequeueInfo.dequeueSizeByte;


        lte::LteLog::Debug2Format(
            _node,
            _interfaceIndex,
            LTE_STRING_LAYER_TYPE_SCHEDULER,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RAW_RESULT_UL,
            log.str().c_str());

    }

    {
        int numRb =
            PhyLteGetNumResourceBlocks(_node, _phyIndex);

        // Make RB to UEResultMaps
        std::vector < LteUlSchedulingResultInfo* >
                                    rbToSchResultMap(numRb, NULL);
        for (int lRbIndex = 0;
            lRbIndex < result->numResourceBlocks;
            ++lRbIndex)
        {
            int rbIndex = result->startResourceBlock + lRbIndex;

            ERROR_Assert(rbToSchResultMap[rbIndex] == NULL,
                "Resource block allocation is duplicated");

            rbToSchResultMap[rbIndex] = result;
        }

        std::stringstream log;
        char buf[MAX_STRING_LENGTH];

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
                static LteRnti invalidRnti(-1,-1);
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

#endif

    // Finally, Purge invalid scheduling results
    //-------------------------------------------
    purgeInvalidSchedulingResults(schedulingResult);

    // End of UL scheduling
}

