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

#ifndef _MAC_LTE_SCH_PF_H_
#define _MAC_LTE_SCH_PF_H_

#include "layer2_lte_sch.h"
#include "layer3_lte_filtering.h"
#include <set>

///////////////////////////////////////////////////////////////
// swtiches
///////////////////////////////////////////////////////////////
#define SCH_LTE_ENABLE_SIMPLE_PF_METRIC 0
#define SCH_LTE_ENABLE_PF_RANDOM_SORT 1

////////////////////////////////////////////////////////////////////////////
// Define
////////////////////////////////////////////////////////////////////////////
// default parameter
#define SCH_LTE_DEFAULT_PF_FILTER_COEFFICIENT (36.0)
#define SCH_LTE_DEFAULT_PF_UL_RB_ALLOCATION_UNIT (1)

// Constants
#define SCH_LTE_PF_INITIAL_AVERAGE_THROUGHPUT (1.0)

// /**
// STRUCT:: pfMetricEntry
// DESCRIPTION::
//      Pair of UE index, RBG index, PF metric value and random number
//      used for PF scheduling
// **/
struct pfMetricEntry
{
    int _ueIndex;
    int _rbgIndex;
    double* _metricValue;
#if SCH_LTE_ENABLE_PF_RANDOM_SORT
    double _rand;
#endif
};

// /**
// STRUCT:: pfMetricGreaterPtr
// DESCRIPTION::
//      Functor comparing PF metric value of two pfMetricEntry objects.
// **/
struct pfMetricGreaterPtr{
    bool operator()(const pfMetricEntry& lhs, const pfMetricEntry& rhs){
#if SCH_LTE_ENABLE_PF_RANDOM_SORT
        if (*(lhs._metricValue) == *(rhs._metricValue))
        {
            return lhs._rand > rhs._rand;
        }
#endif
        return(*(lhs._metricValue) > *(rhs._metricValue));
    }
};

// /**
// STRUCT:: predRbgIndex
// DESCRIPTION::
//      Predicator : "rbgIndex of pfMetricEntry equals to indicated rbgIndex"
// **/
struct predRbgIndex
{
    predRbgIndex(int rbgIndex){ _rbgIndex = rbgIndex; }

    bool operator()(const pfMetricEntry& obj) const
    {
        return (obj._rbgIndex == _rbgIndex);
    }

    int _rbgIndex;
};

typedef std::vector < pfMetricEntry > PfMetricContainer;

// /**
// STRUCT:: AllocatedRbgRange
// DESCRIPTION::
//      A C++ class for manipulate continuous RBG allocation
// **/
struct AllocatedRbgRange{
    int lower;
    int upper;

    // /**
    // FUNCTION   :: AllocatedRbgRange::AllocatedRbgRange
    // LAYER      :: MAC
    // PURPOSE    :: Initialize AllocatedRbgRange
    // PARAMETERS ::
    // RETURN     :: void : NULL
    // **/
    AllocatedRbgRange()
    {
        lower = PHY_LTE_MAX_NUM_RB;
        upper = -1;
    }

    // /**
    // FUNCTION   :: AllocatedRbgRange::getNumAllocatedRbgs
    // LAYER      :: MAC
    // PURPOSE    :: Get number of allocated RBGs
    // PARAMETERS ::
    // RETURN     :: int : Number of allocated RBGs (>=0)
    // **/
    int getNumAllocatedRbgs () const
    {
        return MAX(0, upper - lower + 1);
    }

    // /**
    // FUNCTION   :: AllocatedRbgRange::isEmpty
    // LAYER      :: MAC
    // PURPOSE    :: Get whether allocation is empty or not.
    // PARAMETERS ::
    // RETURN     :: bool : true,  if more than one RBG is allocated.
    //                      false, otherwise.
    // **/
    bool isEmpty () const
    {
        return (getNumAllocatedRbgs() <= 0);
    }

    // /**
    // FUNCTION   :: AllocatedRbgRange::isAdjacent
    // LAYER      :: MAC
    // PURPOSE    :: Get whether indicated rbg is adjacent
    //               rbg of currently allocated rbgs.
    // PARAMETERS ::
    // + rbg      : int     : Rbg index to check
    // RETURN     :: int : -1 , if indicated rbg locates at lower side of
    //                          currently allocated rbgs
    //                      1 , if indicated rbg locates at upper side of
    //                          currently allocated rbgs
    //                      0 , if indicated rbg is not adjacent rbg of
    //                          currently allocated rbgs
    // NOTE       :: Return -1 if no rbg is allocated currently.
    // **/
    int isAdjacent (int rbg) const
    {
        if (rbg == (lower - 1) || isEmpty())
        {
            return -1;
        }
        else if (rbg == (upper + 1))
        {
            return 1;
        }else{
            return 0;
        }
    }

    // /**
    // FUNCTION   :: AllocatedRbgRange::add
    // LAYER      :: MAC
    // PURPOSE    :: Allocate indicated rbg if possible
    // PARAMETERS ::
    // + rbg      : int     : Rbg index to allocate
    // RETURN    :: int : -1 , if indicated rbg is allocated to lower side of
    //                         currently allocated rbgs
    //                     1 , if indicated rbg is allocated to upper side of
    //                         currently allocated rbgs
    //                     0 , if indicated rbg is not allocated
    // **/
    int add(int rbg)
    {
        int side = isAdjacent(rbg);

        if (side == 0)
        {
            // Can not allocate rbg
        }
        else if (side == -1)
        {
            lower = rbg;
            if (isEmpty())
            {
                upper = lower;
            }
        }else if (side == 1)
        {
            upper = rbg;
            if (isEmpty())
            {
                lower = upper;
            }
        }

        return side;
    }
};

// /**
// STRUCT:: LteSchedulerENBPf
// DESCRIPTION:: PF scheduler
// **/
class LteSchedulerENBPf : public LteSchedulerENB
{
public:
    LteSchedulerENBPf(
        Node* node, int interfaceIndex, const NodeInput* nodeInput);
    virtual ~LteSchedulerENBPf();

    virtual void initConfigurableParameters();

    virtual void scheduleDlTti(
        std::vector < LteDlSchedulingResultInfo > &schedulingResult);
    virtual void scheduleUlTti(
        std::vector < LteUlSchedulingResultInfo > &schedulingResult);

    ListLteRnti::const_iterator getNextAllocatedUe(
        bool downlink,
        const ListLteRnti* listLteRnti);

    void determineTargetUes(
        bool downlink,
        std::vector < LteRnti >* targetUes);

    void allocateResourceBlocks(bool isDl);
    void ulCalculateEstimatedSinr();

    virtual void prepareForScheduleTti(UInt64 ttiNumber);

    virtual void updateAverageThroughputDl(
        const std::vector < LteDlSchedulingResultInfo >* pSchedulingResult);

    virtual void updateAverageThroughputUl(
        const std::vector < LteUlSchedulingResultInfo >* pSchedulingResult);

    virtual void calculatePfMetricsDl(
        int targetUeIndex,
        PfMetricContainer& sortedPfMetric,
        std::vector < std::vector < int > > &allocatedBitsIf,
        std::vector < LteDlSchedulingResultInfo > &tmpResults,
        const std::vector < std::vector < double > > &estimatedSinr_dB,
        const std::vector < int > &allocatedBits,
        const std::vector < LteRnti > &targetUes,
        int numberOfRbGroup, int rbGroupSize, int lastRbGroupSize);

    virtual void calculatePfMetricsUl(
        int targetUeIndex,
        PfMetricContainer& sortedPfMetric,
        std::vector < std::vector < int > > &allocatedBitsIf,
        const std::vector < AllocatedRbgRange > &allocatedRbg,
        const std::vector < int > &allocatedBits,
        const std::vector < LteRnti > &targetUes,
        int numberOfRbGroup, int rbGroupSize, int lastRbGroupSize);

    virtual int getNumAllocatedRbs(
        const AllocatedRbgRange& arr,
        int numberOfRbGroup,
        int rbGroupSize,
        int lastRbGroupSize);

    virtual double getAverageThroughput(
        const LteRnti& rnti, int allocatedBits, bool isDl);

    virtual void notifyPowerOn();
    virtual void notifyPowerOff();
    virtual void notifyAttach(const LteRnti& rnti);
    virtual void notifyDetach(const LteRnti& rnti);

#ifdef LTE_LIB_LOG
    virtual void debugOutputRbgAverageThroughputDl();
    virtual void debugOutputRbgAverageThroughputUl();
#endif

protected:

    int _ulRbAllocationUnit;
    double _pfFilterCoefficient;
    LteLayer3Filtering* _filteringModule;

    std::map < LteRnti, double > averageThroughputDl;
    std::map < LteRnti, double > averageThroughputUl;

#if SCH_LTE_ENABLE_PF_RANDOM_SORT
    RandomSeed randomSeed;
#endif
};
#endif // _MAC_LTE_SCH_PF_H_


