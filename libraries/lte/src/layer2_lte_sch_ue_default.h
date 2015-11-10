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

#ifndef _MAC_LTE_SCH_UE_DEFAULT_H_
#define _MAC_LTE_SCH_UE_DEFAULT_H_

#include "layer2_lte_sch.h"

// /**
// STRUCT:: LteSchedulerUEDefault
// DESCRIPTION:: Default MAC scheduler on UE
// **/
class LteSchedulerUEDefault : public LteSchedulerUE
{
public:
    LteSchedulerUEDefault(
        Node* node,
        int interfaceIndex,
        const NodeInput* nodeInput);
    virtual ~LteSchedulerUEDefault();

    virtual void scheduleUlTti(
        std::vector < LteUlSchedulingResultInfo > &schedulingResult);

    virtual void prepareForScheduleTti(UInt64 ttiNumber);
};

#endif // _MAC_LTE_SCH_UE_DEFAULT_H_


