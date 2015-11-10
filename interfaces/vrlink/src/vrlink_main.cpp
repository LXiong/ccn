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


#include "vrlink_shared.h"
#include "WallClock.h"
#include "external_util.h"

#if DtDIS
#include "vrlink_dis.h"
#elif DtHLA
#include "vrlink_hla.h"
#endif


// Factory / Loader function
extern "C"
{
VRLinkExternalInterface *makeVRLinkExternalInterface(VRLinkSimulatorInterface *simIface);
}

VRLinkExternalInterface *makeVRLinkExternalInterface(VRLinkSimulatorInterface *simIface)
{
    return new VRLinkExternalInterface_Impl(simIface);
}

// /**
// FUNCTION :: VRLinkExternalInterface
// PURPOSE :: Initialization function for class VRLinkExternalInterface.
// PARAMETERS ::
// + ifacePtr : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
// + ifaceType : VRLinkInterfaceType : VRLink interface to set.
// **/
VRLinkExternalInterface_Impl::VRLinkExternalInterface_Impl(VRLinkSimulatorInterface* simIface)
{
    ::simIface = simIface;
}

void VRLinkExternalInterface_Impl::Init(
    EXTERNAL_Interface* ifacePtr,
    NodeInput* nodeInput)
{
    iface = ifacePtr;

#if DtDIS
    vrlink = new VRLinkDIS(iface, nodeInput, ::simIface);
    if (!vrlink)
    {
        VRLinkReportError("Out of memory", __FILE__, __LINE__);
    }
    vrlink->SetType(VRLINK_TYPE_DIS);
#elif DtHLA13 
    vrlink = new VRLinkHLA13(iface, nodeInput, ::simIface);
    if (!vrlink)
    {
        VRLinkReportError("Out of memory", __FILE__, __LINE__);
    }
    vrlink->SetType(VRLINK_TYPE_HLA13);
#elif DtHLA_1516
    vrlink = new VRLinkHLA1516(iface, nodeInput, ::simIface);
    if (!vrlink)
    {
        VRLinkReportError("Out of memory", __FILE__, __LINE__);
    }
    vrlink->SetType(VRLINK_TYPE_HLA1516);
#endif

    ifaceType = vrlink->GetType();
    
    if (simIface->GetPartitionIdForIface(iface) == 0)
    {
        vrlink->InitConfigurableParameters(nodeInput);
    }
}

// /**
// FUNCTION :: GetType
// PURPOSE :: Returns the VRLinkInterfaceType configured on the interface.
// PARAMETERS :: None
// RETURN :: VRLinkInterfaceType : Type of VR-Link interface.
// **/
VRLinkInterfaceType VRLinkExternalInterface_Impl::GetType()
{
    return ifaceType;
}

// /**
// FUNCTION :: GetVRLinkInterfaceData
// PURPOSE :: Returns the interface data pointer.
// PARAMETERS :: None
// RETURN :: VRLink* : Interface data pointer.
// **/
VRLink* VRLinkExternalInterface_Impl::GetVRLinkInterfaceData()
{
    return vrlink;
}

// /**
// FUNCTION :: GetVRLinkInterfaceData
// PURPOSE :: Returns the simulator interface class pointer.
// PARAMETERS :: None
// RETURN :: VRLinkSimulatorInterface* : Simulator interface class pointer.
// **/
VRLinkSimulatorInterface*
VRLinkExternalInterface_Impl::GetVRLinkSimulatorInterface()
{
    return simIface;
}

// /**
// FUNCTION :: InitNodes
// PURPOSE :: Initializes VR-Link by making in join a federation (for HLA)
//            or initializing it on a port (for DIS).
// PARAMETERS ::
// + ifacePtr : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
// **/
void VRLinkExternalInterface_Impl::InitNodes(
    EXTERNAL_Interface* iface,
    NodeInput* nodeInput)
{
    // This function is called after nodes and protocols are initialized.
    // Protocol state-data is available.
    // (in parallel) This part is only invoked on partition 0.

    simIface->InitMessengerApp(iface);
    if (simIface->GetPartitionIdForIface(iface) != 0)
    {
        // For parallel we create messenger applications.
        // All the other hla init work is only done on partition 0.
        return;
    }

    vrlink->ReadEntitiesFile();
    vrlink->ReadRadiosFile();
    vrlink->ReadNetworksFile();

    vrlink->SetMinAndMaxLatLon();
    vrlink->MapDefaultDstRadioPtrsToRadios();
    vrlink->MapHierarchyIds(iface);

    vrlink->CreateFederation();
    vrlink->RegisterCallbacks();
    vrlink->AssignEntityIdToNodesNotInRadiosFile();
    vrlink->TryForFirstObjectDiscovery(); 

    cout.precision(6);

    vrlink->ScheduleTasksIfAny();
    simIface->QueryExternalTime(iface);
}

// /**
// FUNCTION :: Receive
// PURPOSE :: Receives interactions into QualNet from an outside simulator.
// PARAMETERS ::
// + iface : EXTERNAL_Interface* : External Interface pointer.
// **/
void VRLinkExternalInterface_Impl::Receive(EXTERNAL_Interface* iface)
{
    VRLinkExternalInterface* vrLinkExtIface =
        (VRLinkExternalInterface*)simIface->GetDataForExternalIface(iface);

    if (!vrLinkExtIface)
    {
        VRLinkReportError("VRLinkExternalInterface* does not " \
                "exist for Receive function");
    }

    vrlink->SetSimulationTime();

    vrlink->GetExConn()->drainInput();
}

// /**
// FUNCTION :: ProcessEvent
// PURPOSE :: Handles all the protocol events.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg : Message* : Pointer to message.
// RETURN :: void : NULL
// **/
void VRLinkExternalInterface_Impl::ProcessEvent(Node* node, Message* msg)
{
    VRLinkVerify(
        (vrlink != NULL),
        "VRLink data not found on interface",
        __FILE__, __LINE__);

    vrlink->ProcessEvent(node, msg);
}

// /**
// FUNCTION :: Finalize
// PURPOSE :: VR-Link's finalize function.
// PARAMETERS ::
// + iface : EXTERNAL_Interface* : External Interface pointer.
// RETURN :: void : NULL
// **/
void VRLinkExternalInterface_Impl::Finalize(EXTERNAL_Interface* iface)
{
    VRLinkExternalInterface* vrLinkExtIface =
        (VRLinkExternalInterface*)simIface->GetDataForExternalIface(iface);

    if (!vrLinkExtIface)
    {
        VRLinkReportError("VRLinkExternalInterface* does not " \
                          "exist for Finalize function");
    }

    vrlink->DeregisterCallbacks();
    delete vrlink;
}
// /**
// FUNCTION :: UpdateMetric
// PURPOSE :: GUI is supplying metric changes that need to be 
// sent out as notification to listening federates
// PARAMETERS ::
// + nodeId : unsigned : Id of the node.
// + metric : const MetricData& : Metric data.
// + metricValue : void* : Metric value.
// + updateTime : clocktype : Update time.
// RETURN :: void : NULL.
// **/
void VRLinkExternalInterface_Impl::UpdateMetric(unsigned nodeId,
        const MetricData& metric,
        void* metricValue,
        clocktype updateTime)
{
    vrlink->UpdateMetric(nodeId, metric, metricValue, updateTime);
}

VRLinkExternalInterface_Impl::~VRLinkExternalInterface_Impl()
{
}

void VRLinkExternalInterface_Impl::SendRtssNotificationIfNodeIsVRLinkEnabled(Node* node)
{
    if (GetVRLinkSimulatorInterface()->IsMilitaryLibraryEnabled())
    {
        vrlink->AppProcessSendRtssEvent(node);
    }
}

