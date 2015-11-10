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
    initializer.setDestinationAddress(DtString(m_destinationAddress.toDtString()));
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

    DtInetSocket *  skt = (DtInetSocket *) exConn->socket();
    DtDisSocket * dskt;
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
// + connectionVar1 : char [] : connection setting variable 1
// + connectionVar2 : char [] : connection setting variable 2
// + connectionVar3 : char [] : connection setting variable 3
// + connectionVar4 : char [] : connection setting variable 4
// **/
void VRLinkDIS::InitConnectionSetting(char connectionVar1[],
                            char connectionVar2[],
                            char connectionVar3[],
                            char connectionVar4[])
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
//FUNCTION :: PublishObjects
//PURPOSE :: Entity/Radio vrlink publisher for dis protocol
//PARAMETERS :: None
//RETURN :: void : NULL
// **/
void VRLinkDIS::PublishObjects()
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
              entity->GetId());

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

        // Use the entity's state repository to set data.
        DtEntityStateRepository* esr = entityPub->entityStateRep();
        esr->setEntityId(entity->GetId());
        esr->setMarkingText(entity->GetMarkingData().c_str()); 
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
            entity->GetId(), radio->GetRadioIndex());

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
//PURPOSE :: sends signal pdu for a cer for dis protocol
//PARAMETERS :: bool : isData : whether the CER is data only
//              unsigned : msgSize : size of the message, either bytes or secs
//              double : timeout : timeout delay for the CER
//              bool : dstNodeIdPresent : whether a destination node was specified
//              unsigned : dstNodeId : node ID of destination node
//RETURN :: void : NULL
// **/
void VRLinkDIS::SendCommEffectsRequest(unsigned   srcNodeId,
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

    VRLinkRadio* srcRadio = radioIter->second;
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
    sig.setRadioId(srcRadio->GetRadioIndex());

    //set the entity id that owns the srcRadio
    sig.setEntityId(srcRadio->GetEntityPtr()->GetId());

    //set the data rate
    sig.setSampleRate(1000000);  
        
    //set the encoding type to app specifc
    sig.setEncodingClass(DtEncodingClass(DtApplicationSpecific));
    
    //prepare the data string
    unsigned char signalData[g_testfedSignalDataBufSize];
    
    unsigned sdOffset = 0;
    char s[512];
    const char newLine = '\n';
    const char nullTerminator = 0;

    //set the user protocol Id, as the first four bytes of the data
    unsigned protocol = commEffectsProtocolId;
    
    CopyToOffsetAndHton(
        signalData,
        sdOffset,
        &protocol,
        sizeof(protocol));

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
