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

// Contains the public VRLinkInterface API

#include <stdio.h>

class VRLinkSimulatorInterface;

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
    virtual void Init(EXTERNAL_Interface* iface, NodeInput* nodeInput) = 0;
    virtual void InitNodes(EXTERNAL_Interface* iface, NodeInput* nodeInput) = 0;
    virtual void Receive(EXTERNAL_Interface* iface) = 0;
    virtual void ProcessEvent(Node* node, Message* msg) = 0;
    virtual void Finalize(EXTERNAL_Interface* iface) = 0;

    virtual void UpdateMetric(unsigned nodeId,
        const MetricData& metric,
        void* metricValue,
        clocktype updateTime) = 0;

    virtual void SendRtssNotificationIfNodeIsVRLinkEnabled(Node* node) = 0;

// get rid of these
    virtual VRLink* GetVRLinkInterfaceData() = 0;
};

// /**
//FUNCTION : VRLinkIsActive
//PURPOSE : To ascertain whether this scenario uses the VRLink Interface
//          This version is used at startup, before the interface is created
//PARAMETERS :
//     nodeInput: pointer to contents of the config file
//RETURN : bool
// **/
bool VRLinkIsActive(NodeInput* nodeInput);

//FUNCTION : VRLinkIsActive
//PURPOSE : To ascertain whether this scenario is using the VRLink Interface
//          This version is used after the interface is created
//PARAMETERS : None.
//RETURN : bool
// **/
bool VRLinkIsActive(Node *node);

//FUNCTION :: VRLinkIsActive
//PURPOSE :: To ascertain whether this scenario is using the VRLink Interface
//           This version is only valid on partition 0. It is used by the GUI Interface
//PARAMETERS :: None.
//RETURN :: bool
// **/
bool VRLinkIsActive();

// /**
//FUNCTION :: VRLinkInit
//PURPOSE :: To verify that OS is 32bit; initialize external interface.
//PARAMETERS ::
// + ifacePtr : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
//RETURN :: void : NULL
// **/
void VRLinkInit(EXTERNAL_Interface* iface, NodeInput* nodeInput);

// /**
//FUNCTION :: VRLinkInitNodes
//PURPOSE :: To initialize VRLink on an interface.
//PARAMETERS ::
// + iface : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
//RETURN :: void : NULL
// **/
void VRLinkInitNodes(EXTERNAL_Interface* iface, NodeInput* nodeInput);

// /**
//FUNCTION :: VRLinkReceive
//PURPOSE :: To receive interactions from an outside federate.
//PARAMETERS ::
// + iface : EXTERNAL_Interface* : External Interface pointer.
//RETURN :: void : NULL
// **/
void VRLinkReceive(EXTERNAL_Interface* iface);

// /**
// FUNCTION :: VRLinkProcessEvent
// PURPOSE :: Handles all the protocol events for VRLink.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg : Message* : Pointer to message.
// RETURN :: void : NULL
// **/
void VRLinkProcessEvent(Node* node, Message* msg);

// /**
// FUNCTION :: VRLinkFinalize
// PURPOSE :: VRLink's finalize function.
// PARAMETERS ::
// + iface : EXTERNAL_Interface* : External Interface pointer.
// RETURN :: void : NULL
// **/
void VRLinkFinalize(EXTERNAL_Interface* iface);

// /**
// FUNCTION :: VRLinkUpdateMetric
// PURPOSE :: To send state updates to GUI.
// PARAMETERS ::
// + nodeId : unsigned : Id of the node.
// + metric : const MetricData& : Metric data.
// + metricValue : void* : Metric value.
// + updateTime : clocktype : Update time.
// RETURN :: void : NULL.
// **/
void VRLinkUpdateMetric(
    unsigned nodeId,
    const MetricData& metric,
    void* metricValue,
    clocktype updateTime);

// /**
// FUNCTION :: VRLinkSendRtssNotificationIfNodeIsVRLinkEnabled
// PURPOSE :: To send Rtss notification if node is VRLink enabled.
// PARAMETERS ::
// + node : Node* : Node pointer.
// RETURN :: void : NULL
// **/
void VRLinkSendRtssNotificationIfNodeIsVRLinkEnabled(Node* node);

#endif /* VRLINK_H */
