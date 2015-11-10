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
#include "vrlink_hla.h"

// /**
// FUNCTION :: VRLinkHLA
// PURPOSE :: Constructor for HLA.
// PARAMETERS ::
// + ifacePtr : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
// **/
VRLinkHLA::VRLinkHLA() : VRLink()
{
}

// /**
// FUNCTION :: ~VRLinkHLA
// PURPOSE :: Destructor of Class VRLinkHLA.
// PARAMETERS :: None.
// **/
VRLinkHLA::~VRLinkHLA()
{
}

// /**
//FUNCTION :: CreateFederation
//PURPOSE :: To make QualNet join the federation. If the federation does not
//           exist, the exconn API creates a new federation first, and then
//           register QualNet with it.
//PARAMETERS :: None
//RETURN :: void : NULL
// **/
void VRLinkHLA::CreateFederation()
{
    try
    {
        DtExerciseConnInitializer initializer;
        initializer.setExecName((DtString)m_federationName);
        initializer.setFedFileName((DtString)m_fedFilePath);
        initializer.setRprFomVersion(m_rprFomVersion);
        initializer.setFederateType((DtString)m_federateName);
        DtExerciseConn::InitializationStatus status;

        cout << "VRLINK: Trying to create federation "
             << m_federationName
             << " ... \n";

        exConn = new DtExerciseConn(initializer, &status);

        if (status != 0)
        {
            VRLinkReportError("VRLINK: Unable to join federation");
        }
    }
    DtCATCH_AND_WARN(cout);
    cout << "VRLINK: Successfully joined federation." << endl;
}

// /**
// FUNCTION :: VRLinkHLA13
// PURPOSE :: Initializing function for HLA13.
// PARAMETERS ::
// + ifacePtr : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
// **/
VRLinkHLA13::VRLinkHLA13() : VRLinkHLA()
{
}

// /**
// FUNCTION :: VRLinkHLA1516
// PURPOSE :: Initializing function for HLA1516.
// PARAMETERS ::
// + ifacePtr : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
// **/
VRLinkHLA1516::VRLinkHLA1516() : VRLinkHLA()
{
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
void VRLinkHLA::InitConnectionSetting(char* connectionVar1,
                                      char* connectionVar2,
                                      char* connectionVar3,
                                      char* connectionVar4)
{
    m_rprFomVersion = atof(connectionVar1);
    strcpy(m_federationName, connectionVar2);
    strcpy(m_federateName, connectionVar3);
    strcpy(m_fedFilePath, connectionVar4);
}


// /**
// FUNCTION :: RegisterProtocolSpecificCallbacks
// PURPOSE :: Registers HLA specific callbacks.
// PARAMETERS :: None.
// RETURN :: void : NULL
// **/
void VRLinkHLA::RegisterProtocolSpecificCallbacks()
{

}




// /**
// FUNCTION :: GetRadioIdKey
// PURPOSE :: Get the RadioIdKey for radio transmitter repository.
// PARAMETERS ::
// + dataIxnInfo : DtRadioTransmitterRepository* : Pointer to radio
//                  transmitter repository
// RETURN :: RadioIdKey : EntityIdentifierKey and int pair
// **/
RadioIdKey VRLinkHLA::GetRadioIdKey(DtRadioTransmitterRepository* radioTsr)
{
    return radioTsr->globalId();
}
