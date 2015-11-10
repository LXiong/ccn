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

// iostream.h is needed to support DMSO RTI 1.3v6 (non-NG).

#ifndef DtDIS
#error This file should only be used in the DIS specific build
#endif

#include "vrlink_shared.h"
#include "vrlink_dis.h"

// /**
// FUNCTION :: VRLinkDIS
// PURPOSE :: constructor for DIS.
// **/
VRLinkDIS::VRLinkDIS() : VRLink()
{
    exerciseIdentifier = 1;
    isSubnetMaskSet = false;
}

// /**
// FUNCTION :: VRLinkDIS
// PURPOSE :: destructor for DIS
// **/
VRLinkDIS::~VRLinkDIS()
{

}

// /**
//FUNCTION :: CreateFederation
//PURPOSE :: Initializes QualNet on the specified port and exercise id for
//           DIS.
//PARAMETERS :: None
//RETURN :: void : NULL
// **/
void VRLinkDIS::CreateFederation()
{
    DtExerciseConnInitializer initializer;
    initializer.setPort(m_portNumber);
    initializer.setSiteId(siteId);
    initializer.setExerciseId((int)exerciseIdentifier);
    initializer.setApplicationNumber(applicationId);
    initializer.setDestinationAddress(
                                DtString(m_destinationAddress.toDtString()));
    initializer.setDeviceAddress(DtString(m_deviceAddress.toDtString()));
    if (isSubnetMaskSet)
    {
        initializer.setSubnetMask(DtString(m_subnetMask.toDtString()));
    }
    DtExerciseConn::InitializationStatus status;

    try
    {
        exConn = new DtExerciseConn(
                        initializer,
                        &status);

    }DtCATCH_AND_WARN(cout);

    if (status != 0)
    {
        VRLinkReportError(
            "VRLINK: could not create dis socket",
                __FILE__, __LINE__);
    }

    DtInetSocket*  skt = (DtInetSocket *) exConn->socket();
    DtDisSocket* dskt;
    if (skt && skt->isOk())
    {
        dskt = dynamic_cast<DtDisSocket*>(skt);
        if (!dskt )
        {
            char errorString[256] = {0};
            sprintf(errorString, "Error in exercise creation to: %s\n",
                m_destinationAddress.toDtString().c_str());
        }
    }
    else
    {
        char errorString[256] = {0};
        sprintf(errorString, "Error in exercise creation to: %s\n",
            m_destinationAddress.toDtString().c_str());
    }
}

// /**
// FUNCTION :: InitConnectionSetting
// PURPOSE :: Initializing function for exercise connection settings
// PARAMETERS ::
// + connectionVar1 : char * : connection setting variable 1
// + connectionVar2 : char * : connection setting variable 2
// + connectionVar3 : char * : connection setting variable 3
// + connectionVar4 : char * : connection setting variable 4
// **/
void VRLinkDIS::InitConnectionSetting(char* connectionVar1,
                            char* connectionVar2,
                            char* connectionVar3,
                            char* connectionVar4)
{
    m_portNumber = atoi(connectionVar1);
    m_destinationAddress = DtStringToInetAddr(connectionVar2);
    m_deviceAddress = DtStringToInetAddr(connectionVar3);
    if (strcmp(connectionVar4, "") != 0)
    {
        m_subnetMask = DtStringToInetAddr(connectionVar4);
        isSubnetMaskSet = true;
    }   
}


// /**
// FUNCTION :: RegisterProtocolSpecificCallbacks
// PURPOSE :: Registers DIS specific callbacks with the federation.
// PARAMETERS :: None.
// RETURN :: void : NULL
// **/
void VRLinkDIS::RegisterProtocolSpecificCallbacks()
{
    //there are currently no DIS specific callbacks; timeout processing is
    //currently not enabled for DIS.
    refEntityList->setTimeoutProcessing(false);
    refRadioTxList->setTimeoutProcessing(false);
}





// /**
// FUNCTION :: GetRadioIdKey
// PURPOSE :: Get the RadioIdKey for radio transmitter repository.
// PARAMETERS ::
// + dataIxnInfo : DtRadioTransmitterRepository* : Pointer to radio
//                  transmitter repository
// RETURN :: RadioIdKey : EntityIdentifierKey and int pair
// **/
RadioIdKey VRLinkDIS::GetRadioIdKey(DtRadioTransmitterRepository* radioTsr)
{
    EntityIdentifierKey* entityKey = GetEntityIdKeyFromDtEntityId(
                                                      &radioTsr->entityId());
    DtEntityIdentifier entId = DtEntityIdentifier (entityKey->siteId,
                                                   entityKey->applicationId,
                                                   entityKey->entityNum);

    return pair<DtEntityIdentifier, unsigned int>(
                                            entId, radioTsr->radioId());
}
