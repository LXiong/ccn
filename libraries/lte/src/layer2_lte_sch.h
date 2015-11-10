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
// PACKAGE     :: LTE
// DESCRIPTION :: Defines constants, enums, structures used in the LTE
//                SCH sublayer.
// **/

#ifndef _MAC_LTE_SCH_H_
#define _MAC_LTE_SCH_H_

#include <node.h>
#include "lte_common.h"
#include "phy_lte.h"
#include "layer3_lte.h"

// For creating dequeue information
#include "layer2_lte_mac.h"

#if LTE_LIB_LOG
///////////////////////////////////////////////////////////////////////////
/// Log related
///////////////////////////////////////////////////////////////////////////
const char* STR_RNTI(char* buf, const LteRnti& rnti);
#endif

////////////////////////////////////////////////////////////////////////////
// Define
////////////////////////////////////////////////////////////////////////////
// default parameter
#define MAC_LTE_DEFAULT_TARGET_BLER (1.0E-2)

// Constants
#define MAC_LTE_SNR_OFFSET_FOR_UL_MCS_SELECTION  (7.0) // dB
#define MAC_LTE_SNR_OFFSET_FOR_DL_MCS_SELECTION  (0.0) // dB
// Note : See also PHY_LTE_SNR_OFFSET_FOR_CQI_SELECTION for DL
//        estimated SINR offset.

////////////////////////////////////////////////////////////////////////////
// Structure
////////////////////////////////////////////////////////////////////////////

// /**
// STRUCT:: LteSchedulingResultDequeueInfo
// DESCRIPTION:: Structure for dequeue information
// **/
typedef struct {
    UInt32 bearerId;
    int dequeueSizeByte;
} LteSchedulingResultDequeueInfo;

// /**
// STRUCT:: LteDlSchedulingResultInfo
// DESCRIPTION:: Structure for DL scheduling result
// **/
typedef struct {
    LteRnti rnti;
    LteTxScheme txScheme; // Transmission scheme
    UInt8 allocatedRb[LTE_MAX_NUM_RB]; // List of allocated RB groups
    int numTransportBlock; // Number of Transport Blocks 1 or 2
    UInt8 mcsIndex[2]; // MCS for Transport block 1 or 2
    LteSchedulingResultDequeueInfo dequeueInfo[2];

    //-----
    int numResourceBlocks; // Number of allocated resource blocks.
                           // Just for simulation purpose

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    double estimatedSinr_dB[2];
#endif
#endif
} LteDlSchedulingResultInfo;

// /**
// STRUCT:: LteUlSchedulingResultInfo
// DESCRIPTION:: Structure for UL scheduling result
// **/
typedef struct {
    LteRnti rnti;
    UInt8 startResourceBlock;
    int numResourceBlocks;
    UInt8 mcsIndex;
    LteSchedulingResultDequeueInfo dequeueInfo;

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    double estimatedSinr_dB;
#endif
#endif
} LteUlSchedulingResultInfo;


////////////////////////////////////////////////////////////////////////////
// Class
////////////////////////////////////////////////////////////////////////////

// /**
// STRUCT:: LteScheduler
// DESCRIPTION:: Base class for MAC scheduler
// **/
class LteScheduler {
public:
    LteScheduler();
    LteScheduler(Node* node, int interfaceIndex, const NodeInput* nodeInput);
    virtual ~LteScheduler();

    virtual void scheduleDlTti(
        std::vector < LteDlSchedulingResultInfo > &schedulingResult) = 0;

    virtual void scheduleUlTti(
        std::vector < LteUlSchedulingResultInfo > &schedulingResult) = 0;

    virtual void initConfigurableParameters();

    virtual void prepareForScheduleTti(UInt64 ttiNumber);

    virtual void purgeInvalidSchedulingResults(
        std::vector < LteDlSchedulingResultInfo > &schedulingResult) = 0;

    virtual void purgeInvalidSchedulingResults(
        std::vector < LteUlSchedulingResultInfo > &schedulingResult) = 0;

    virtual int determineDequeueSize(int transportBlockSize);

    // Notification I/Fs

    virtual void notifyPowerOn();
    virtual void notifyPowerOff();
    virtual void notifyAttach(const LteRnti& rnti);
    virtual void notifyDetach(const LteRnti& rnti);

    void init();

protected:
    Node* _node;
    int _interfaceIndex;
    int _phyIndex;
    const NodeInput* _nodeInput;

    UInt64 _ttiNumber;

private:
};

// /**
// STRUCT:: LteSchedulerENB
// DESCRIPTION:: Base class for MAC scheduler on eNB
// **/
class LteSchedulerENB : public LteScheduler {
public:
    LteSchedulerENB(
        Node* node, int interfaceIndex, const NodeInput* nodeInput);
    virtual ~LteSchedulerENB();

    virtual void scheduleDlTti(
        std::vector < LteDlSchedulingResultInfo > &schedulingResult) = 0;

    virtual void scheduleUlTti(
        std::vector < LteUlSchedulingResultInfo > &schedulingResult) = 0;

    virtual void initConfigurableParameters();

    virtual void purgeInvalidSchedulingResults(
        std::vector < LteDlSchedulingResultInfo > &schedulingResult);

    virtual void purgeInvalidSchedulingResults(
        std::vector < LteUlSchedulingResultInfo > &schedulingResult);

    virtual void createDequeueInformation(
        std::vector < LteDlSchedulingResultInfo > &schedulingResult);

    virtual void calculateEstimatedSinrUl(
        const LteRnti& rnti,
        int numResourceBlocks,
        int startResourceBlock,
        double& estimatedSinr_dB);

    virtual bool dlIsTargetUe(const LteRnti& rnti);
    virtual bool ulIsTargetUe(const LteRnti& rnti);

    int dlSelectMcs(
        int numResourceBlocks,
        double estimatedSinr_dB,
        double targetBler);

    int ulSelectMcs(
        int numResourceBlocks,
        double estimatedSinr_dB,
        double targetBler);


#ifdef LTE_LIB_LOG
    virtual void debugOutputInterfaceCheckingLog();

    virtual void debugOutputSchedulingResultLog(
        const std::vector < LteDlSchedulingResultInfo > &schedulingResult);

    virtual void debugOutputSchedulingResultLog(
        const std::vector < LteUlSchedulingResultInfo > &schedulingResult);

    void debugOutputEstimatedSinrLog(
        std::vector < LteDlSchedulingResultInfo > &schedulingResult,
        std::vector < std::vector < double >* > &estimatedSinr_dB);

    virtual void debugOutputEstimatedSinrLog(
        std::vector < LteDlSchedulingResultInfo > &schedulingResult,
        std::vector < std::vector < double > > &estimatedSinr_dB);

    virtual void debugOutputEstimatedSinrLog(
        std::vector < LteUlSchedulingResultInfo > &schedulingResult,
        std::vector < double > &estimatedSinr_dB);
#endif


protected:
    double _targetBler;

private:
};

// /**
// STRUCT:: LteSchedulerUE
// DESCRIPTION:: Base class for MAC scheduler on UE
// **/
class LteSchedulerUE : public LteScheduler {
public:
    LteSchedulerUE(
        Node* node, int interfaceIndex, const NodeInput* nodeInput);
    virtual ~LteSchedulerUE();

    virtual void scheduleDlTti(
        std::vector < LteDlSchedulingResultInfo > &schedulingResult);
    virtual void scheduleUlTti(
        std::vector < LteUlSchedulingResultInfo > &schedulingResult) = 0;

    virtual void purgeInvalidSchedulingResults(
        std::vector < LteDlSchedulingResultInfo > &schedulingResult);

    virtual void purgeInvalidSchedulingResults(
        std::vector < LteUlSchedulingResultInfo > &schedulingResult);

private:
};

#endif // _MAC_LTE_SCH_H_


