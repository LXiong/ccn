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

#ifndef _MAC_LTE_SCH_ROUNDROBIN_H_
#define _MAC_LTE_SCH_ROUNDROBIN_H_

#include "layer2_lte_sch.h"

///////////////////////////////////////////////////////////////
// swtiches
///////////////////////////////////////////////////////////////
#define SCH_LTE_ENABLE_RR_RANDOM_SORT 1

#if SCH_LTE_ENABLE_RR_RANDOM_SORT

// /**
// STRUCT:: rbgMetricEntry
// DESCRIPTION::
//      Pair of RBG index and random number used for PF scheduling
// **/
struct rbgMetricEntry
{
    int _rbgIndex;
    double _rand;
};

// /**
// STRUCT:: rbgMetricGreaterPtr
// DESCRIPTION::
//      Functor comparing two rbgMetricEntry objects
// **/
struct rbgMetricGreaterPtr{
    bool operator()(const rbgMetricEntry& lhs, const rbgMetricEntry& rhs){
        return lhs._rand > rhs._rand;
    }
};
#endif

// /**
// STRUCT:: LteSchedulerENBRoundRobin
// DESCRIPTION:: Round Robin scheduler
// **/
class LteSchedulerENBRoundRobin : public LteSchedulerENB
{
public:
    //LteSchedulerENBRoundRobin();
    LteSchedulerENBRoundRobin(
        Node* node, int interfaceIndex, const NodeInput* nodeInput);
    virtual ~LteSchedulerENBRoundRobin();

    virtual void scheduleDlTti(
        std::vector < LteDlSchedulingResultInfo > &schedulingResult);
    virtual void scheduleUlTti(
        std::vector < LteUlSchedulingResultInfo > &schedulingResult);

    ListLteRnti::const_iterator getNextAllocatedUe(
        bool downlink,
        const ListLteRnti* listLteRnti);

    void allocateResourceBlocks(bool isDl);
    void ulCalculateEstimatedSinr();

    void determineTargetUes(
        bool downlink,
        std::vector < LteRnti >* targetUes);

    virtual void prepareForScheduleTti(UInt64 ttiNumber);

    virtual void notifyPowerOn();
    virtual void notifyPowerOff();
    virtual void notifyAttach(const LteRnti& rnti);
    virtual void notifyDetach(const LteRnti& rnti);

protected:

    ListLteRnti _dlConnectedUeList;
    ListLteRnti _ulConnectedUeList;

    ListLteRnti::const_iterator _dlNextAllocatedUe;
    ListLteRnti::const_iterator _ulNextAllocatedUe;

#if SCH_LTE_ENABLE_RR_RANDOM_SORT
    RandomSeed randomSeed;
#endif
};

#endif // _MAC_LTE_SCH_ROUNDROBIN_H_


