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

        cout << "VRLINK: Trying to create federation "<<m_federationName<< " ... \n";

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
// + connectionVar1 : char [] : connection setting variable 1
// + connectionVar2 : char [] : connection setting variable 2
// + connectionVar3 : char [] : connection setting variable 3
// + connectionVar4 : char [] : connection setting variable 4
// **/
void VRLinkHLA::InitConnectionSetting(char connectionVar1[],
                            char connectionVar2[],
                            char connectionVar3[],
                            char connectionVar4[])
{
    m_rprFomVersion = atof(connectionVar1);
    strcpy(m_federationName, connectionVar2);
    strcpy(m_federateName, connectionVar3);
    strcpy(m_fedFilePath, connectionVar4);
}

// /**
//FUNCTION :: PublishObjects
//PURPOSE :: Entity/Radio vrlink publisher for HLA13 and HLA1516 protocol
//PARAMETERS :: None
//RETURN :: void : NULL
// **/
void VRLinkHLA::PublishObjects()
{
    //publish the entities
    for (hash_map <string, VRLinkEntity*>::iterator entityIt = entities.begin();
        entityIt != entities.end();
        entityIt++)
    {
        VRLinkEntity* entity = entityIt->second;

        // Create an entity publisher for the entity we are simulating.
        DtEntityPublisher* entityPub = new DtEntityPublisher(entity->GetEntityType(), exConn,
              DtDrDrmRvw, entity->GetForceType(),
              DtEntityPublisher::guiseSameAsType(),
              entity->GetIdAsString().c_str());

        // store that entity publisher
        hash_map <string, DtEntityPublisher*>::iterator 
            entitiesPubIter = entityPublishers.find(entity->GetMarkingData());

        if (entitiesPubIter == entityPublishers.end())
        {
            entityPublishers[entity->GetMarkingData()] = entityPub;                     
        }
        else
        {
            VRLinkReportError("EntityPublisher with duplicate MarkingData");
        }

        // Hold on to the entity's state repository, where we can set data.
        DtEntityStateRepository* esr = entityPub->entityStateRep();
        esr->setEntityId(entity->GetId());
        esr->setMarkingText(DtString(entity->GetMarkingData().c_str())); 
        esr->setLocation(entity->GetXYZ());
        esr->setOrientation(entity->GetOrientation());
        esr->setDamageState(entity->GetDamageState());

        entityPub->tick();
    }
    //publish radios
    for (hash_map <unsigned, VRLinkRadio*>::iterator radioIt = radios.begin();
        radioIt != radios.end();
        radioIt++)
    {
        VRLinkRadio* radio = radioIt->second;
        VRLinkEntity* entity = radio->GetEntityPtr();

        assert(entity != NULL);
        DtRadioTransmitterPublisher* radioPub = new DtRadioTransmitterPublisher(exConn,
            radio->GetRadioKey().c_str());

        // store that entity publisher
        hash_map <unsigned, DtRadioTransmitterPublisher*>::iterator 
            radiosPubIter = radioPublishers.find(radio->GetNodeId());

        if (radiosPubIter == radioPublishers.end())
        {
            radioPublishers[radio->GetNodeId()] = radioPub;
        }
        else
        {
            VRLinkReportError("RadioPublisher with duplicate nodeId");
        }

        // Use the radio's state repository to set data.
        DtRadioTransmitterRepository* tsr = radioPub->tsr();
        tsr->setRadioEntityType(radio->GetRadioType());
        tsr->setEntityId(entity->GetId());
        tsr->setRadioId(radio->GetRadioIndex());
        tsr->setTransmitState(radio->GetTransmitState());
        tsr->setRelativePosition(radio->GetRelativePosition());
        
        radioPub->tick();
    }
}

// /**
//FUNCTION :: SendCommEffectsRequest
//PURPOSE :: sends data interaction for a cer for hla protocol
//PARAMETERS :: bool : isData : whether the CER is data only
//              unsigned : msgSize : size of the message, either bytes or secs
//              double : timeout : timeout delay for the CER
//              bool : dstNodeIdPresent : whether a destination node was specified
//              unsigned : dstNodeId : node ID of destination node
//RETURN :: void : NULL
// **/
void VRLinkHLA::SendCommEffectsRequest(unsigned   srcNodeId,
        bool      isData,
        unsigned   msgSize,
        double     timeoutDelay,
        bool       dstNodeIdPresent,
        unsigned   dstNodeId)
{
    //check if this is a legal request
    hash_map <unsigned, VRLinkRadio*>::iterator 
        radioIter = radios.find(srcNodeId);
   
    VRLinkVerify(radioIter != radios.end(),
        "Cannot find radio for requested src nodeId", __FILE__, __LINE__);

    VRLinkEntity* destEntity = NULL;

    if (dstNodeIdPresent)
    {
        radioIter = radios.find(dstNodeId);
        
        VRLinkVerify(radioIter != radios.end(),
            "Cannot find radio for requested destation nodeId", __FILE__, __LINE__);
        destEntity = radioIter->second->GetEntityPtr();
    }

    //request is legal so start creating signal interactions
    DtSignalInteraction sig;

    hash_map <unsigned, DtRadioTransmitterPublisher*>::iterator 
        radioPubIter = radioPublishers.find(srcNodeId);

    //set the src radio
    sig.setRadioId(radioPubIter->second->globalId());

    //set the data rate
    sig.setSampleRate(1000000);  

    //set the user protocol Id
    sig.setProtocolId(commEffectsProtocolId);
    
    //set the encoding type to app specifc
    sig.setEncodingClass(DtEncodingClass(DtApplicationSpecific));
    
    //prepare the data string
    unsigned char signalData[g_testfedSignalDataBufSize];
    
    unsigned sdOffset = 0;
    char s[512];
    const char newLine = '\n';
    const char nullTerminator = 0;

    CopyToOffset(
        signalData,
        sdOffset,
        "HEADER\n",
        strlen("HEADER\n"));

    // Receiver.
    if (destEntity != NULL)
    {
        CopyToOffset(
            signalData,
            sdOffset,
            "receiver=",
            strlen("receiver="));

        CopyToOffset(
            signalData,
            sdOffset,
            destEntity->GetEntityIdString().c_str(),
            strlen(destEntity->GetEntityIdString().c_str()));

        CopyToOffset(
            signalData,
            sdOffset,
            &newLine,
            sizeof(newLine));
    }

    // Size.

    CopyToOffset(
        signalData,
        sdOffset,
        "size=",
        strlen("size="));

    sprintf(s, "%u ", msgSize);

    CopyToOffset(
        signalData,
        sdOffset,
        s,
        strlen(s));

    if (isData)
    {
        CopyToOffset(
            signalData,
            sdOffset,
            "bytes",
            strlen("bytes"));
    }
    else
    {
        CopyToOffset(
            signalData,
            sdOffset,
            "seconds",
            strlen("seconds"));
    }

    CopyToOffset(
        signalData,
        sdOffset,
        &newLine,
        sizeof(newLine));

    // Timeout.

    CopyToOffset(
        signalData,
        sdOffset,
        "timeout=",
        strlen("timeout="));

    sprintf(s, "%.3f", timeoutDelay);

    CopyToOffset(
        signalData,
        sdOffset,
        s,
        strlen(s));

    CopyToOffset(
        signalData,
        sdOffset,
        &newLine,
        sizeof(newLine));

    // Timestamp.

    CopyToOffset(
        signalData,
        sdOffset,
        "timestamp=",
        strlen("timestamp="));

    sprintf(s, "0x%08x", GetTimestamp());

    CopyToOffset(
        signalData,
        sdOffset,
        s,
        strlen(s));

    CopyToOffset(
        signalData,
        sdOffset,
        &newLine,
        sizeof(newLine));

    CopyToOffset(
        signalData,
        sdOffset,
        "EOH\nDATA\nEOD\n",
        strlen("EOH\nDATA\nEOD\n"));

    CopyToOffset(
        signalData,
        sdOffset,
        &nullTerminator,
        sizeof(nullTerminator));

    sig.setData(signalData, sdOffset);

    exConn->sendStamped(sig);

    cout << "Sending CommEffects Request- srcNodeId:" << srcNodeId << " msgSize:" << msgSize;
    if (!isData)
    {
        cout << "(voice)";
    }

    cout    << " timeoutDelay:" << timeoutDelay;

    if (dstNodeIdPresent)
    {
        cout << " destNodeId:" << dstNodeId;
    }
    cout << ".\n";
}
