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

#ifndef VRLINK_H
#define VRLINK_H

#include "archspec.h"
#include "data_types.h"

// Contains the public VRLinkInterface API

// /**
// ENUM        :: VRLinkInterfaceType
// DESCRIPTION :: Types of protocols supported by VR-Link.
// **/
enum VRLinkInterfaceType
{
    VRLINK_TYPE_NONE = 0, //this value should be zero only
    VRLINK_TYPE_DIS,
    VRLINK_TYPE_HLA13,
    VRLINK_TYPE_HLA1516
};

class VRLink;

class VRLinkExternalInterface
{
  public:
    virtual ~VRLinkExternalInterface() { };

    virtual void Init(bool debug,
                        VRLinkInterfaceType type,
                        char* scenarioName,
                        char* connectionVar1,
                        char* connectionVar2,
                        char* connectionVar3,
                        char* connectionVar4) = 0;

    virtual void Tick(clocktype simTime) = 0;

    virtual void Receive() = 0;

    virtual void Finalize() = 0;

    virtual SNT_SYNCH::EntityStruct* GetNextEntityStruct() = 0;
    virtual SNT_SYNCH::RadioStruct* GetNextRadioStruct() = 0;
    virtual SNT_SYNCH::AggregateEntityStruct* GetNextAggregateStruct() = 0;
};

VRLinkExternalInterface* VRLinkInit(bool debug,
    VRLinkInterfaceType type,
    char* scenarioName,
    char* connectionVar1,
    char* connectionVar2,
    char* connectionVar3,
    char* connectionVar4);

VRLinkInterfaceType GetVRLinkInterfaceType(
    std::string externalInterfaceType);


#endif /* VRLINK_H */
