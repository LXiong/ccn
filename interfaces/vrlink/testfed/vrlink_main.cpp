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

#if DtDIS
#include "vrlink_dis.h"
#elif DtHLA
#include "vrlink_hla.h"
#endif

// Factory / Loader function
extern "C"
{
VRLinkExternalInterface *makeVRLinkExternalInterface();
}

VRLinkExternalInterface *makeVRLinkExternalInterface()
{
 return new VRLinkExternalInterface_Impl();
}

// /**
// FUNCTION :: Init
// PURPOSE :: Initialization function for class VRLinkExternalInterface.
// PARAMETERS ::
// RETURN :: void : none
// **/
void VRLinkExternalInterface_Impl::Init(bool debug, 
    VRLinkInterfaceType type,
    char scenarioName[],
    char connectionVar1[],
    char connectionVar2[],
    char connectionVar3[],
    char connectionVar4[])
{
#if DtDIS
    vrlink = new VRLinkDIS();
    if (!vrlink)
    {
        VRLinkReportError("Out of memory", __FILE__, __LINE__);
    }
    vrlink->SetType(VRLINK_TYPE_DIS);

#elif DtHLA13 
    vrlink = new VRLinkHLA13();
    if (!vrlink)
    {
        VRLinkReportError("Out of memory", __FILE__, __LINE__);
    }
    vrlink->SetType(VRLINK_TYPE_HLA13);
#elif DtHLA_1516
    vrlink = new VRLinkHLA1516();
    if (!vrlink)
    {
        VRLinkReportError("Out of memory", __FILE__, __LINE__);
    }
    vrlink->SetType(VRLINK_TYPE_HLA1516);
#endif

    vrlink->SetScenarioName(scenarioName);
    vrlink->SetDebug(debug);
    vrlink->InitConnectionSetting(connectionVar1,
        connectionVar2,
        connectionVar3,
        connectionVar4);
    
    //INIT NODES Section
    vrlink->ReadEntitiesFile();
    vrlink->ReadRadiosFile();
    vrlink->ReadNetworksFile();

    vrlink->CreateFederation();
    vrlink->RegisterCallbacks();
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
// FUNCTION :: Receive
// PURPOSE :: Receives interactions into QualNet from an outside simulator.
// PARAMETERS ::
// **/
void VRLinkExternalInterface_Impl::Receive()
{
    /*vrlink->SetSimulationTime();*/

    vrlink->GetExConn()->drainInput();
}

VRLinkExternalInterface_Impl::~VRLinkExternalInterface_Impl()
{
}

// /**
// FUNCTION :: Finalize
// PURPOSE :: VR-Link's finalize function.
// PARAMETERS ::
// RETURN :: void : NULL
// **/
void VRLinkExternalInterface_Impl::Finalize()
{
    vrlink->DeregisterCallbacks();
    delete vrlink;
}

void VRLinkExternalInterface_Impl::PublishObjects()
{
    vrlink->PublishObjects();
}

void VRLinkExternalInterface_Impl::MoveEntity(unsigned nodeId,
        double lat,
        double lon,
        double alt)
{
    vrlink->MoveEntity(nodeId, lat, lon, alt);
}

void VRLinkExternalInterface_Impl::ChangeEntityOrientation(unsigned nodeId,        
        double orientationPsiDegrees,
        double orientationThetaDegrees,
        double orientationPhiDegrees)
{
    vrlink->ChangeEntityOrientation(nodeId, orientationPsiDegrees,
        orientationThetaDegrees, orientationPhiDegrees);
}

void VRLinkExternalInterface_Impl::SendCommEffectsRequest(unsigned   srcNodeId,
        bool      isData,
        unsigned   msgSize,
        double     timeoutDelay,
        bool       dstNodeIdPresent,
        unsigned   dstNodeId)
{
    vrlink->SendCommEffectsRequest(srcNodeId, isData, msgSize, timeoutDelay,
        dstNodeIdPresent, dstNodeId);
}

void VRLinkExternalInterface_Impl::ChangeDamageState(unsigned nodeId, 
        unsigned damageState)
{
    vrlink->ChangeDamageState(nodeId, damageState);
}

void VRLinkExternalInterface_Impl::ChangeTxOperationalStatus(unsigned nodeId,
        unsigned char txOperationalStatus)
{
    vrlink->ChangeTxOperationalStatus(nodeId, txOperationalStatus);
}

void VRLinkExternalInterface_Impl::SetSimTime(clocktype time)
{
    vrlink->SetSimTime(time);
}

void VRLinkExternalInterface_Impl::IterationSleepOperation(clocktype time)
{
    vrlink->IterationSleepOperation(time);
}

void VRLinkExternalInterface_Impl::Tick()
{
    vrlink->Tick();
}   

void VRLinkExternalInterface_Impl::ChangeEntityVelocity(unsigned nodeId,
    float x,
    float y,
    float z)
{
    vrlink->ChangeEntityVelocity(nodeId, x, y, z);
}

void VRLinkExternalInterface_Impl::IncrementEntitiesPositionIfVelocityIsSet(double timeSinceLastPostionUpdate)
{
    vrlink->IncrementEntitiesPositionIfVelocityIsSet(timeSinceLastPostionUpdate);
}
