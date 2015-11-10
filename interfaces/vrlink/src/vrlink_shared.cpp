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
#include "vrlink.h"

#include <cerrno>
#include <matrix/vlVector.h>
#include <matrix/geodCoord.h>

#define DEBUG_NOTIFICATION 0

VRLinkSimulatorInterface* simIface = NULL;

// /**
// FUNCTION :: ConvertGccToLatLonAlt
// PURPOSE :: Converts the GCC coordinates to latLonAlt coordinates.
// PARAMETERS  ::
// + gccCoord : DtVector : GCC coordinates to be converted.
// + latLonAltInDeg : DtVector& : Converted latLinAlt coordinates.
// RETURN :: void : NULL.
// **/
void ConvertGccToLatLonAlt(
    DtVector gccCoord,
    DtVector& latLonAltInDeg)
{
    DtGeodeticCoord radioLatLonAltInRad;

    radioLatLonAltInRad.setGeocentric(gccCoord);
    latLonAltInDeg.setX(DtRad2Deg(radioLatLonAltInRad.lat()));
    latLonAltInDeg.setY(DtRad2Deg(radioLatLonAltInRad.lon()));
    latLonAltInDeg.setZ(radioLatLonAltInRad.alt());
}

// /**
// FUNCTION :: ConvertLatLonAltToGcc
// PURPOSE :: Converts the latLonAlt coordinates to GCC coordinates.
// PARAMETERS  ::
// + latLonAlt : DtVector : LatLonAlt coordinates to be converted.
// + gccCoord : DtVector& : Converted GCC coordinates.
// RETURN :: void : NULL.
// **/
void ConvertLatLonAltToGcc(
    DtVector latLonAlt,
    DtVector& gccCoord)
{
    DtGeodeticCoord geod(DtDeg2Rad(latLonAlt.x()), DtDeg2Rad(latLonAlt.y()),
                                                              latLonAlt.z());

    gccCoord = geod.geocentric();
}

// /**
// FUNCTION :: VRLinkEntity
// PURPOSE :: Initializing function for VR-Link entity.
// PARAMETERS :: None.
// **/
VRLinkEntity::VRLinkEntity()
{
    entityId = DtEntityIdentifier(0, 0, 0);

    nationality = "";
    azimuth = 0;
    elevation = 0;
    azimuthScheduled = 0;
    elevationScheduled = 0;

    currentVelocity.zero();
    speed = 0;
    speedScheduled = 0.0;
    velocityScheduled.zero();

    damageState = DtDamageNone;
    hierarchyIdExists = false;
    hierarchyId = 0;
    xyz.zero();
    latLonAlt.zero();

    const clocktype neverHappenedTime = -1;
    lastScheduledMobilityEventTime = neverHappenedTime;
}

// /**
// FUNCTION :: GetMarkingData
// PURPOSE :: Selector for returning the entity marking data.
// PARAMETERS :: None
// RETURN :: string
// **/
string VRLinkEntity::GetMarkingData() const
{
    return markingData;
}

// /**
// FUNCTION :: GetXYZ
// PURPOSE :: Selector for returning the GCC coordinates of an entity.
// PARAMETERS :: None
// RETURN :: string
// **/
DtVector VRLinkEntity::GetXYZ()
{
    return xyz;
}

// /**
// FUNCTION :: GetLatLon
// PURPOSE :: Selector for returning the entity latitude and longitude
//            coordinates.
// PARAMETERS :: None
// RETURN :: DtVector
// **/
DtVector VRLinkEntity::GetLatLon()
{
    return latLonAlt;
}

// /**
// FUNCTION :: GetLastScheduledMobilityEventTime
// PURPOSE :: Selector for returning the entity's last scheduled mobility
//            event.
// PARAMETERS :: None
// RETURN :: clocktype
// **/
clocktype VRLinkEntity::GetLastScheduledMobilityEventTime()
{
    return lastScheduledMobilityEventTime;
}

// /**
// FUNCTION :: GetDamageState
// PURPOSE :: Selector for returning the entity's damage state.
// PARAMETERS :: None
// RETURN :: DtDamageState
// **/
DtDamageState VRLinkEntity::GetDamageState() const
{
    return damageState;
}

// /**
// FUNCTION :: GetEntityIdString
// PURPOSE :: Selector for returning the entity id string.
// PARAMETERS :: None
// RETURN :: const char*
// **/
const char* VRLinkEntity::GetEntityIdString() const
{
    return entityId.string();
}

// /**
// FUNCTION :: GetEntityId
// PURPOSE :: Selector for returning the entity id.
// PARAMETERS :: None
// RETURN :: const char*
// **/
DtEntityIdentifier VRLinkEntity::GetEntityId() const
{
    return entityId;
}

// /**
// FUNCTION :: SetEntityId
// PURPOSE :: Selector for setting an entity's id.
// PARAMETERS ::
// + entId : const DtEntityIdentifier : Entity id to be set.
// RETURN :: void : NULL
// **/
void VRLinkEntity::SetEntityId(const DtEntityIdentifier &entId)
{
    if (entityId == DtEntityIdentifier(0, 0, 0))
    {
        entityId = entId;
    }
}

// /**
// FUNCTION :: GetRadioPtrsHash
// PURPOSE :: Selector for returning an entity's radio list.
// PARAMETERS :: None
// RETURN :: hash_map<unsigned short, VRLinkRadio*>
// **/
hash_map<unsigned short, VRLinkRadio*> VRLinkEntity::GetRadioPtrsHash()
{
    return radioPtrs;
}

// /**
// FUNCTION :: SetHierarchyData
// PURPOSE :: Selector for setting an entity's hierarchy data.
// PARAMETERS ::
// + nodeToHiereachyId : const hash_map<NodeId, int>& : Node to hierarchy id
//                                                      hash.
// RETURN :: void : NULL
// **/
void VRLinkEntity::SetHierarchyData(
    const hash_map<NodeId, int>& nodeToHiereachyId)
{
    VRLinkVerify(
        !hierarchyIdExists,
        "Hierarchy id already exists",
        __FILE__, __LINE__);

    hash_map<NodeId, int>::const_iterator nodeToHiereachyIdIter;
    hash_map<unsigned short, VRLinkRadio*>::iterator radioPtrsIter;

    VRLinkVerify(
        (!radioPtrs.empty()),
        "No radios found for entity",
        __FILE__, __LINE__);

    for (radioPtrsIter = radioPtrs.begin(); radioPtrsIter != radioPtrs.end();
                                                             radioPtrsIter++)
    {
        const VRLinkRadio& radio = *radioPtrsIter->second;

        VRLinkVerify(&radio, "Radio not found", __FILE__, __LINE__);

        VRLinkVerify(
            radio.GetNode(),
            "Node pointer not found",
            __FILE__, __LINE__);

        nodeToHiereachyIdIter = nodeToHiereachyId.find(
                                   simIface->GetNodeId(radio.GetNode()));

        if (nodeToHiereachyIdIter != nodeToHiereachyId.end())
        {
            hierarchyIdExists = true;
            hierarchyId = nodeToHiereachyIdIter->second;

            break;
        }
    }
}

// /**
// FUNCTION :: CbEntityAdded
// PURPOSE :: Callback for adding a new entity.
// PARAMETERS ::
// + ent : DtReflectedEntity* : Entity to be added.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLink::CbEntityAdded(DtReflectedEntity* ent, void* usr)
{
    VRLinkVerify(
        ent,
        "NULL reflected entity data received",
        __FILE__, __LINE__);

    VRLink* vrLinkPtr = (VRLink*)usr;

    if (vrLinkPtr)
    {
        vrLinkPtr->AddReflectedEntity(ent);
    }
}

// /**
// FUNCTION :: AddReflectedEntity
// PURPOSE :: Adds a new entity (called by CbEntityAdded callback).
// PARAMETERS ::
// + ent : DtReflectedEntity* : Entity to be added.
// RETURN :: void : NULL
// **/
void VRLink::AddReflectedEntity(DtReflectedEntity* ent)
{
    hash_map <string, VRLinkEntity*>::iterator entitiesIter;
    hash_map <string, VRLinkEntity*>::iterator reflectedEntitiesIter;

    VRLinkEntity* entityPtr = NULL;
    DtEntityStateRepository* esr = ent->esr();
    string markingDataStr = esr->markingText();
    DtEntityIdentifier entId = esr->entityId();
    reflectedEntitiesIter = reflectedEntities.find(entId.string());

    if (reflectedEntitiesIter == reflectedEntities.end())
    {
        entitiesIter = entities.find(markingDataStr);

        if (entitiesIter != entities.end())
        {
            entityPtr = entitiesIter->second;
            reflectedEntities[entId.string()] = entityPtr;

            EntityIdentifierKey entityIdKey = 
                GetEntityIdKeyFromDtEntityId(entId);
            entityIdToEntityData[entityIdKey] = entityPtr;

            entityPtr->SetEntityId(entId);
            entityPtr->ReflectAttributes(ent, this);            
        }
        else
        {
            //this reflected entity was not in the .entities qualnet file
            return;
        }

        if (debug || debugPrintMapping)
        {
            cout << "VRLINK: Mapped to object ("<<entityPtr->GetEntityIdString()
                << ") " << entityPtr->GetMarkingData() << endl;
        }

        VRLinkEntityCbUsrData* usrData = new VRLinkEntityCbUsrData(
                                                 this,
                                                 entityPtr);

        try
        {
            ent->addPostUpdateCallback(
                     &(VRLinkEntity::CbReflectAttributeValues),
                     usrData);
        }
        DtCATCH_AND_WARN(cout);
                
        //try and "rediscover" any already reflected radio that were tossed out because the 
        //entity was not discovered yet
        if (refRadiosWithoutEntities.find(entId) != refRadiosWithoutEntities.end())
        {
            for (list<DtReflectedRadioTransmitter*>::iterator it = refRadiosWithoutEntities[entId].begin();
                it != refRadiosWithoutEntities[entId].end();it++)
            {
                AddReflectedRadio(*it);
            }
            refRadiosWithoutEntities.erase(entId);
        }
    }
    else
    {
        char errorString[MAX_STRING_LENGTH];

        sprintf(
            errorString,
            "Two or more PhysicalEntity objects with duplicate EntityId =" \
            " %s", entId.string());

        VRLinkReportError(errorString, __FILE__, __LINE__);
    }
}

// /**
// FUNCTION :: RemoveReflectedEntity
// PURPOSE :: Removes an entity (called by CbEntityRemoved callback).
// PARAMETERS ::
// + ent : DtReflectedEntity* : Reflected entity pointer.
// RETURN :: void : NULL
// **/
void VRLink::RemoveReflectedEntity(DtReflectedEntity* ent)
{
    hash_map <string, VRLinkEntity*>::iterator reflEntitiesIter;

    string entityId = ent->esr()->entityId().string();

    reflEntitiesIter = reflectedEntities.find(entityId);

    if (reflEntitiesIter != reflectedEntities.end())
    {
        VRLinkEntity* entity = reflEntitiesIter->second;

         VRLinkVerify(
                entity,
                "Reflected entity not found while removing",
                __FILE__, __LINE__);

        entity->Remove();

        if (debug || debugPrintMapping)
        {
            cout << "VRLINK: Removing entity (" << entity->GetEntityIdString()
                << ") " << entity->GetMarkingData() << endl;
        }

        reflectedEntities.erase(entityId);

        EntityIdentifierKey key = GetEntityIdKeyFromDtEntityId(entity->GetEntityId());
        entityIdToEntityData.erase(key);
        
        if (entityIdToNodeIdMap.find(key) != entityIdToNodeIdMap.end())
        {
            entityIdToNodeIdMap.erase(key);
        }

        ent->removePostUpdateCallback(
                 &(VRLinkEntity::CbReflectAttributeValues),
                 this);
    }
}

// /**
// FUNCTION :: AddReflectedRadio
// PURPOSE :: Adds a new radio (called by CbRadioAdded callback).
// PARAMETERS ::
// + radioTx : DtReflectedRadioTransmitter* : Radio to be added.
// RETURN :: void : NULL
// **/
void VRLink::AddReflectedRadio(DtReflectedRadioTransmitter* radioTx)
{
    map <RadioIdKey, VRLinkRadio*>::iterator reflRadiosIter;
    hash_map <string, VRLinkEntity*>::iterator entitiesIter;
    hash_map <string, VRLinkEntity*>::iterator reflEntitiesIter;

    VRLinkRadio* radioPtr = NULL;
    DtRadioTransmitterRepository*  tsr = radioTx->tsr();

    RadioIdKey radioKey = GetRadioIdKey(tsr);

    reflRadiosIter = reflRadioIdToRadioMap.find(radioKey);

    if (reflRadiosIter == reflRadioIdToRadioMap.end())
    {
        const char* entIdStr = tsr->entityId().string();
        reflEntitiesIter = reflectedEntities.find(entIdStr);

        if (reflEntitiesIter != reflectedEntities.end())
        {
            radioPtr = reflEntitiesIter->second->GetRadioPtr(tsr->radioId());

            if (radioPtr)
            {
                reflRadioIdToRadioMap[radioKey] = radioPtr;
                
                const char* gId = tsr->globalId().string();
                reflGlobalIdToRadioHash[gId] = radioPtr;
                radioPtr->ReflectAttributes(radioTx, this);
                radioPtr->SetEntityId(tsr->entityId());

                NodeId id = simIface->GetNodeId(radioPtr->GetNode());
                nodeIdToEntityIdHash[id] = tsr->entityId();

                EntityIdentifierKey* entityIdKey =
                              GetEntityIdKeyFromDtEntityId(&tsr->entityId());

                entityIdToNodeIdMap[*entityIdKey].push_back(id);

                EntityIdKeyRadioIndex entityRadioIndex(*entityIdKey, radioPtr->GetRadioIndex());
                radioIdToRadioData[entityRadioIndex] = radioPtr;

                if (debug || debugPrintMapping)
                {
                    cout<<"VRLINK: Assigned node "<< id
                        << " (" << tsr->entityId().string()
                        << ", " << radioPtr->GetRadioIndex()<< ") "
                        <<radioPtr->GetMarkingData() << endl;
                }

                VRLinkRadioCbUsrData* usrData = new VRLinkRadioCbUsrData(
                                                        this,
                                                        radioPtr);

                try
                {
                    radioTx->addPostUpdateCallback(
                                 &(VRLinkRadio::CbReflectAttributeValues),
                                 usrData);

                    if (simIface->IsMilitaryLibraryEnabled())
                    {
                        //On start-up send out a RTSS to external interfaces for all Link11/16 nodes
                        //VRF will not send any CER till it receives a RTSS (if use-ready-to-send is set to TRUE)
                        // Schedule the RTSS notification to be sent 1 nanosecond from
                        // the current simulation time. Why 1 nanosecond and not 0?
                        // For a delay of 0, the RTSS will be sent immediately, i.e.,
                        // while still executing a addReflectedRadio() callback, which is illegal                                                                       
                        if (radioPtr->IsLink11Node() || radioPtr->IsLink16Node())
                        {
                            ScheduleRtssNotification(radioPtr, 1);
                        }
                    }
                }                
                DtCATCH_AND_WARN(cout);
            }
            else
            {
                VRLinkReportWarning("\nRadio not found for the " \
                                    "reflected entity\n",
                                    __FILE__, __LINE__);
            }
        }
        else
        {
            DtEntityIdentifier entId = radioTx->tsr()->entityId();
            if (refRadiosWithoutEntities.find(entId) != refRadiosWithoutEntities.end())
            {
                refRadiosWithoutEntities[entId].push_back(radioTx);                
            }
            else
            {
                refRadiosWithoutEntities[entId] = list<DtReflectedRadioTransmitter*>();
                refRadiosWithoutEntities[entId].push_back(radioTx);
            }
        }
    }
    else
    {
        VRLinkRadio* radio = reflRadiosIter->second;
        char errorString[MAX_STRING_LENGTH + 300];

        sprintf(
            errorString,
            " Duplicate RadioTransmitter object received:"
            " EntityID = %s, MarkingData = %s, RadioIndex = %u."
            " (Mapped RadioTransmitter object's host-entity EntityID to"
            " PhysicalEntity object, retrieved MarkingData for that"
            " PhysicalEntity object, but have already discovered a"
            " RadioTransmitter with that MarkingData and RadioIndex)",
            radio->GetEntityPtr()->GetEntityIdString(),
            radio->GetEntityPtr()->GetMarkingData().c_str(),
            radio->GetRadioIndex());

        VRLinkReportError(errorString);
    }
}

// /**
// FUNCTION :: RemoveReflectedRadio
// PURPOSE :: Removes a radio (called by CbRadioRemoved callback).
// PARAMETERS ::
// + tsr : DtRadioTransmitterRepository*& : Tx repository of the radio to be
//                                          removed.
// RETURN :: void : NULL
// **/
void VRLink::RemoveReflectedRadio(DtRadioTransmitterRepository*& tsr)
{
    map <RadioIdKey, VRLinkRadio*>::iterator reflRadiosIter;
    unsigned int radioId = tsr->radioId();

    RadioIdKey radioKey = GetRadioIdKey(tsr);

    reflRadiosIter = reflRadioIdToRadioMap.find(radioKey);

    if (reflRadiosIter != reflRadioIdToRadioMap.end())
    {
        VRLinkRadio* radio = reflRadiosIter->second;

        const VRLinkEntity* entity = radio->GetEntityPtr();

        if (debug || debugPrintMapping)
        {
            cout << "VRLINK: Removing node " << simIface->GetNodeId(radio->GetNode())
                << " (" << radio->GetEntityPtr()->GetEntityIdString()<< ", "
                << radio->GetRadioIndex() << ") "<< radio->GetMarkingData()
                << endl;
        }

        if (radio->GetTxOperationalStatus() != DtOff)
        {
            radio->Remove();
        }

        reflRadioIdToRadioMap.erase(radioKey);
        string gId = tsr->globalId().string();
        reflGlobalIdToRadioHash.erase(gId);

        NodeId nid = simIface->GetNodeId(radio->GetNode());
        nodeIdToEntityIdHash.erase(nid);

        EntityIdentifierKey entityIdKey = 
            GetEntityIdKeyFromDtEntityId(entity->GetEntityId());

        EntityIdKeyRadioIndex entityRadioIndex(entityIdKey, radio->GetRadioIndex());

        radioIdToRadioData.erase(entityRadioIndex);
        
        if (entityIdToNodeIdMap.find(entityIdKey) != entityIdToNodeIdMap.end())
        {
            list<NodeId>::iterator findIter = std::find(
                entityIdToNodeIdMap[entityIdKey].begin(), entityIdToNodeIdMap[entityIdKey].end(), nid);

            if (findIter != entityIdToNodeIdMap[entityIdKey].end())
            {
                entityIdToNodeIdMap[entityIdKey].erase(findIter);
            }
        }        
    }
}

// /**
// FUNCTION :: CbRadioAdded
// PURPOSE :: Callback for adding a new radio.
// PARAMETERS ::
// + radioTx : DtReflectedRadioTransmitter* : Radio to be added.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLink::CbRadioAdded(DtReflectedRadioTransmitter* radioTx, void* usr)
{
    VRLinkVerify(
        radioTx,
        "NULL reflected radio tx data received",
        __FILE__, __LINE__);

    VRLink* vrLinkPtr = (VRLink*)usr;

    vrLinkPtr->AddReflectedRadio(radioTx);
}

// /**
// FUNCTION :: CbRadioRemoved
// PURPOSE :: Callback for removing a radio.
// PARAMETERS ::
// + radioTx : DtReflectedRadioTransmitter* : Radio to be added.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLink::CbRadioRemoved(DtReflectedRadioTransmitter* radioTx, void* usr)
{
    VRLinkVerify(
        radioTx,
        "NULL reflected radio tx data received",
        __FILE__, __LINE__);

    VRLink* vrLinkPtr = (VRLink*)usr;
    DtRadioTransmitterRepository* tsr = radioTx->tsr();

    vrLinkPtr->RemoveReflectedRadio(tsr);
}

// /**
// FUNCTION :: CbEntityRemoved
// PURPOSE :: Callback for removing an entity.
// PARAMETERS ::
// + ent : DtReflectedEntity* : Entity to be removed.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLink::CbEntityRemoved(DtReflectedEntity* ent, void *usr)
{
    VRLinkVerify(
        ent,
        "NULL reflected entity data received",
        __FILE__, __LINE__);

    VRLink* vrLinkPtr = (VRLink*)usr;

    vrLinkPtr->RemoveReflectedEntity(ent);
}

// /**
// FUNCTION :: CbSignalIxnReceived
// PURPOSE :: Callback for receiving a signal interaction.
// PARAMETERS ::
// + inter : DtSignalInteraction* : Received signal interaction.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLink::CbSignalIxnReceived(DtSignalInteraction* inter, void* usr)
{
    VRLink* vrLinkPtr = (VRLink*)usr;
    vrLinkPtr->ProcessSignalIxn(inter);
}

// /**
// FUNCTION :: AppProcessTimeoutEvent
// PURPOSE :: Processes the MSG_EXTERNAL_VRLINK_AckTimeout event.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg : Message* : Pointer to message.
// RETURN :: void : NULL
// **/
void VRLink::AppProcessTimeoutEvent(Message* msg)
{    
    VRLinkSimulatedMsgInfo& smInfo
        = *((VRLinkSimulatedMsgInfo*) simIface->ReturnInfoFromMsg(msg));

    Node* srcNode = smInfo.srcNode;

    hash_map<NodeId, VRLinkData*>::iterator nodeDataIter;

    outstandingSimulatedMsgInfoMap::iterator simMsgInfoIter;

    nodeDataIter = nodeIdToPerNodeDataHash.find(simIface->GetNodeId(srcNode));

    VRLinkVerify(nodeDataIter != nodeIdToPerNodeDataHash.end(),
                " Entry not found in nodeIdToPerNodeDataHash",
                __FILE__, __LINE__);

    VRLinkData* srcVRLinkData = nodeDataIter->second;

    VRLinkVerify(
        srcVRLinkData,
        "VRLinkData pointer not found",
        __FILE__, __LINE__);

    // This is parallel ok -
    // The smInfo's memory locations have changed, but that is ok even though
    // we're passing in the address of the msgId field, the hash functions
    // used for keyvalue always deref (and thus don't use the actual address)
    if (srcVRLinkData->GetOutstandingSimulatedMsgInfoHash()->find(smInfo.msgId) ==
        srcVRLinkData->GetOutstandingSimulatedMsgInfoHash()->end())
    {
        // Timeout notification already sent.

        if (debug || debugPrintComms)
        {
            PrintCommEffectsResult(srcNode, smInfo, "ORIG TIMEOUT CANCELLED");
        }

        return;
    }
    else
    {
        if (debug || debugPrintComms)
        {
            PrintCommEffectsResult(srcNode, smInfo, "TIMEOUT OCCURRED");
        }
    }

    const VRLinkOutstandingSimulatedMsgInfo& osmInfo
        = (*(srcVRLinkData->GetOutstandingSimulatedMsgInfoHash()))[smInfo.msgId];

    SendTimeoutNotification(smInfo, osmInfo);
    srcVRLinkData->GetOutstandingSimulatedMsgInfoHash()->erase(smInfo.msgId);
}

// /**
// FUNCTION :: ProcessSignalIxn
// PURPOSE :: Processes the signal interaction received from an outside
//            federate.
// PARAMETERS ::
// + DtSignalInteraction* : Received interaction.
// RETURN :: void : NULL
// **/
void VRLink::ProcessSignalIxn(DtSignalInteraction*)
{
}

// /**
// FUNCTION :: ProcessTerminateCesRequest
// PURPOSE :: Processes the data interaction received to end the simulation.
// PARAMETERS :: None.
// RETURN :: void : NULL
// **/
void VRLink::ProcessTerminateCesRequest()
{
    cout<< "\nVRLINK: Received Terminate CES request.\n";
    EndSimulation();
}

// /**
// FUNCTION :: ProcessCommEffectsRequest
// PURPOSE :: Called to processes the signal interaction after validating the
//            the received signal.
// PARAMETERS ::
// + inter : DtSignalInteraction* : Received interaction.
// RETURN :: void : NULL
// **/
void VRLink::ProcessCommEffectsRequest(DtSignalInteraction* inter)
{
    VRLinkSimulatedMsgInfo smInfo;
    stringstream radioIdStr;
    double requestedDataRate = 0;
    unsigned dataMsgSize = 0;
    clocktype voiceMsgDuration = 0;
    bool isVoice = false;
    clocktype timeoutDelay = 0;
    unsigned int timestamp = 0;
    bool unicast = true;
    const VRLinkEntity* dstEntityPtr = NULL;
    const VRLinkRadio* dstRadioPtr = NULL;
    VRLinkRadio* srcRadio = NULL;
    Node* srcNodePtr = NULL;
    NodeId srcNodeId = 0;
    EntityIdentifierKey* entityKey = NULL;
    EntityIdToNodeIdIter entityIdToNodeIdIter;
    DtEntityIdentifier* destinationEntityId = NULL;
    
    radioIdStr<<inter->radioId();
    string radioId(radioIdStr.str());
    
    // message data
    const DtSignalInteraction *sig;
    sig = dynamic_cast<const DtSignalInteraction*>(inter);
    if (!sig)
        return;

    const char *msg = ((char *)sig->data())+4;
    if (!msg)
    {
        return;
    }
    // find the header size
    const char *pld = strstr(msg, "EOH") + 4;
    size_t hdr_len = pld - msg;

    // The msg may not be a C string so make sure we don't go past
    // the msg end when looking at the header.
    // message size (in bytes)
    size_t msgSize = sig->numDataBits()/8 - 4;
    if (hdr_len > msgSize)
        hdr_len = msgSize;

    std::string hdrStr(msg, hdr_len);
    vector<char> payload;    
    
    payload.reserve(msgSize-hdr_len);
    for (size_t i=0; i<msgSize-hdr_len; i++)
        payload.push_back(pld[i]);

#if DtHLA
    DtEntityIdentifier srcEntity(radioId.c_str());
    entityKey = GetEntityIdKeyFromDtEntityId(&srcEntity);
    entityIdToNodeIdIter = entityIdToNodeIdMap.find(*entityKey);

    if (entityIdToNodeIdIter == entityIdToNodeIdMap.end())
    {
        hash_map <string, VRLinkRadio*>::iterator reflGlobalIdToRadioIter;

        reflGlobalIdToRadioIter = reflGlobalIdToRadioHash.find(radioId);

        if (reflGlobalIdToRadioIter == reflGlobalIdToRadioHash.end())
        {
            char warningString[MAX_STRING_LENGTH];

            sprintf(
                warningString,
                    "Can't map to RadioTransmitter object using " \
                    "HostRadioIndex = %s",radioId.c_str());

            VRLinkReportWarning(warningString);

            return;
        }

        srcRadio = reflGlobalIdToRadioIter->second;
        srcNodePtr = srcRadio->GetNode();
    }
    DtEntityIdentifier entId = srcEntity;

#elif DtDIS
    
    hash_map <string, VRLinkEntity*>::iterator reflEntitiesIter;
    DtEntityIdentifier entId = inter->entityId();
    reflEntitiesIter = reflectedEntities.find(entId.string());

    if (reflEntitiesIter != reflectedEntities.end())
    {
        RadioIdKey radioKey = GetRadioIdKey(inter);
        map <RadioIdKey, VRLinkRadio*>::const_iterator 
            reflRadioIdToRadioIter = reflRadioIdToRadioMap.find(radioKey);

        if (reflRadioIdToRadioIter == reflRadioIdToRadioMap.end())
        {
            char warningString[MAX_STRING_LENGTH];

            sprintf(
                warningString,
                "Can't map to QualNet radio using Entity ID %s",
                reflEntitiesIter->second->GetEntityIdString());

            VRLinkReportWarning(warningString, __FILE__, __LINE__);
            return;
        }

        srcRadio = reflRadioIdToRadioIter->second;
        srcNodePtr = srcRadio->GetNode();
    }   
#endif
    
    if ((!srcRadio))
    {
        if (debug)
        {
            char warningString[MAX_STRING_LENGTH];

            sprintf(
                warningString,
                "RadioTransmitter object not mapped to a Qualnet node for" \
                " radioId %s", radioId.c_str());

            VRLinkReportWarning(warningString, __FILE__, __LINE__);
        }

        return;
    }

    smInfo.srcRadioPtr = srcRadio;    
    smInfo.srcNode = srcNodePtr;
    double sampleRate = inter->sampleRate();

    if (simIface->IsMilitaryLibraryEnabled())
    {
        if (sampleRate == 0 && (!srcRadio->IsLink11Node())
            && (!srcRadio->IsLink16Node()))
        {
            VRLinkReportWarning(
                              "Sample Rate field is 0 for non-Link-11/16 radio");
            return;
        }
    }

    requestedDataRate = sampleRate;

    char* msgStr = (char*)inter->data();

#if DtDIS
    msgStr += 4;
#endif
    
    if (!ParseMsgString(
             msgStr,
             &dstEntityPtr,
             dataMsgSize,
             voiceMsgDuration,
             isVoice,
             timeoutDelay,
             timestamp,            
             &destinationEntityId))
    {
        return;
    }

    // use of dstEntityPtr
    // use of destinationEntityId

    if (destinationEntityId)
    {
        
        if (entId == *destinationEntityId)        
        {
            VRLinkReportWarning(
                "Source and destination entities cannot be same",
                __FILE__, __LINE__);
            return;
        }
        unicast = true ;
    }
    else
    {
        
        bool txMsg = srcRadio->IsCommEffectsRequestValid(
                                   &dstEntityPtr,
                                   &dstRadioPtr,
                                   unicast,
                                   this);

        if (!txMsg)
        {
            return;
        }
    }

    hash_map<NodeId, VRLinkData*>::iterator nodeDataIter;
    Node* srcNode = srcNodePtr;

    nodeDataIter = nodeIdToPerNodeDataHash.find(simIface->GetNodeId(srcNode));

    VRLinkVerify(
        nodeDataIter != nodeIdToPerNodeDataHash.end(),
        " Entry not found in nodeIdToPerNodeDataHash",
        __FILE__, __LINE__);

    VRLinkData* srcVRLinkData = nodeDataIter->second;

    VRLinkVerify(
        srcVRLinkData,
        "VRLinkData pointer not found",
        __FILE__, __LINE__);

    smInfo.msgId = srcVRLinkData->GetNextMsgId();
    srcVRLinkData->SetNextMsgId(srcVRLinkData->GetNextMsgId() + 1);

    if (srcVRLinkData->GetNextMsgId() > maxMsgId)
    {
        VRLinkReportError("Maximum msgId reached", __FILE__, __LINE__);
    }

    clocktype simTime = simIface->GetCurrentSimTime(srcNode);
    clocktype sendTime
        = MAX(simIface->QueryExternalTime(iface), simTime);

    const clocktype sendDelay = sendTime -
                                        simIface->GetCurrentSimTime(srcNode);
    
    SendSimulatedMsgUsingMessenger(
        smInfo,
        (VRLinkEntity*)dstEntityPtr,
        requestedDataRate,
        dataMsgSize,
        voiceMsgDuration,
        isVoice,
        timeoutDelay,
        unicast,
        (VRLinkRadio*)dstRadioPtr,
        (clocktype)sendDelay);
        
    ScheduleTimeout(smInfo, timeoutDelay, sendDelay);

    StoreOutstandingSimulatedMsgInfo(
        smInfo,
        (VRLinkEntity*)dstEntityPtr,
        timestamp,
        sendTime);

    if ((debug || debugPrintComms))
    {
        PrintCommEffectsRequestProcessed(
            srcRadio, dstEntityPtr, unicast, dstRadioPtr, sendTime, smInfo.msgId);
    }
}

// /**
// FUNCTION :: SendSimulatedMsgUsingMessenger
// PURPOSE :: Sends the simulated message via the messenger.
// PARAMETERS ::
// + smInfo : VRLinkSimulatedMsgInfo& : Simulated message.
// + dstEntityPtr : VRLinkEntity* : Destination entity.
// + requestedDataRate : double : Data rate.
// + dataMsgSize : unsigned : Data message size.
// + voiceMsgDuration : clocktype : Voice message duration.
// + isVoice : bool : Specifies whether the message is voice message or not.
// + timeoutDelay : clocktype : Timeout delay of the message.
// + unicast : bool : Specifies whether the message is unicast or not.
// + dstRadioPtr : VRLinkRadio* : Destination radio.
// + sendDelay : clocktype : Propagation delay of the simulated message.
// RETURN :: void : NULL
// **/
void VRLink::SendSimulatedMsgUsingMessenger(
    VRLinkSimulatedMsgInfo& smInfo,
    VRLinkEntity* dstEntityPtr,
    double requestedDataRate,
    unsigned dataMsgSize,
    clocktype voiceMsgDuration,
    bool isVoice,
    clocktype timeoutDelay,
    bool unicast,
    VRLinkRadio* dstRadioPtr,
    clocktype sendDelay)
{
    // TODO: Check what destNodeId of -1 means.

    NodeAddress destNodeId = (NodeAddress) -1;    

    if (unicast)
    {
        assert(dstRadioPtr != NULL);
        destNodeId = simIface->GetNodeId(dstRadioPtr->GetNode());
    }
    else
    {       
        VRLinkVerify(
            (smInfo.srcRadioPtr->GetNetworkPtr()->GetIpAddress() != 0),
             "Associated network ip address cannot be 0.0.0.0",
             __FILE__, __LINE__);

        destNodeId = (NodeAddress) -1;
    }

    if (simIface->GetRealPartitionIdForNode(smInfo.srcRadioPtr->GetNode()) == 0)
    {
        // Call the core function
        StartSendSimulatedMsgUsingMessenger(
            smInfo.srcRadioPtr->GetNode(),
            smInfo.srcRadioPtr->GetNetworkPtr()->GetIpAddress(),
            destNodeId,
            smInfo,
            requestedDataRate,
            dataMsgSize,
            voiceMsgDuration,
            isVoice,
            timeoutDelay,
            unicast,
            sendDelay,
            smInfo.srcRadioPtr->IsLink11Node(),
            smInfo.srcRadioPtr->IsLink16Node());
    }
    else
    {
        Node* srcNode = smInfo.srcNode;
        NodeId srcNodeId = simIface->GetNodeId(srcNode);
        VRLinkRadio* srcRadio = radios[srcNodeId];

        // Build a VRLink-interface message so we can request the
        // correct partition start up a messenger app
        Message* startMessengerMsg = simIface->AllocateMessage(
            srcNode,
            simIface->GetExternalLayerId(),    // special layer
            simIface->GetVRLinkExternalInterfaceType(),   // protocol
            simIface->GetMessageEventType(EXTERNAL_VRLINK_StartMessengerForwarded));

        EXTERNAL_VRLinkStartMessenegerForwardedInfo* startInfo =
            (EXTERNAL_VRLinkStartMessenegerForwardedInfo *)
            simIface->AllocateInfoInMsg (srcNode, startMessengerMsg,
            sizeof (EXTERNAL_VRLinkStartMessenegerForwardedInfo));

        startInfo->isVoice = isVoice;
        startInfo->unicast = unicast;
        startInfo->destNodeId = destNodeId;
        startInfo->voiceMsgDuration = voiceMsgDuration;
        startInfo->timeoutDelay = timeoutDelay;
        startInfo->sendDelay = sendDelay;
        startInfo->srcNetworkAddress = 
            smInfo.srcRadioPtr->GetNetworkPtr()->GetIpAddress();
        startInfo->requestedDataRate = requestedDataRate;
        startInfo->srcNodeId = srcNodeId;
        startInfo->dataMsgSize = dataMsgSize;
        startInfo->isLink11 = srcRadio->IsLink11Node();
        startInfo->isLink16 = srcRadio->IsLink16Node();
        memcpy (&(startInfo->smInfo),
                &smInfo,
                sizeof (VRLinkSimulatedMsgInfo));
        simIface->RemoteExternalSendMessage (iface, 
                simIface->GetRealPartitionIdForNode(srcNode),
                startMessengerMsg, 0, simIface->GetExternalScheduleSafeType());
    }
    
    if (!newEventScheduled)
    {
        newEventScheduled = true;
    }
}

// for cyber chat
void VRLink::SendSimulatedMsgUsingMessenger(
    VRLinkSimulatedMsgInfo& smInfo,
    VRLinkEntity* dstEntityPtr,
    double requestedDataRate,
    unsigned dataMsgSize,
    clocktype voiceMsgDuration,
    bool isVoice,
    clocktype timeoutDelay,
    bool unicast,
    VRLinkRadio* dstRadioPtr,
    clocktype sendDelay,
    const set<NodeId>& destNodeIdSet)
{
    // TODO: Check what destNodeId of -1 means.

    NodeAddress destNodeId = (NodeAddress) -1;
    NodeAddress networkIpAddress = 0xffffffff;

    //printf("sending thru messenger delay %f msgid = %d\n", (double)sendDelay/SECOND,
    //   smInfo.msgId) ;    
    
    if (!unicast)    
    {
        VRLinkVerify(
            (unicast != true),
             "unicast mode cannot be false",
             __FILE__, __LINE__);        
    }

    set<NodeId>::const_iterator it = destNodeIdSet.begin() ;
    for (; it!= destNodeIdSet.end() ;++it)
    {
        if (simIface->GetRealPartitionIdForNode(smInfo.srcNode) == 0)
        {
            // Call the core function
            StartSendSimulatedMsgUsingMessenger(
                smInfo.srcNode,
                networkIpAddress,
                *it,
                smInfo,
                requestedDataRate,
                dataMsgSize,
                voiceMsgDuration,
                isVoice,
                timeoutDelay,
                unicast,
                sendDelay,
                smInfo.srcRadioPtr->IsLink11Node(),
                smInfo.srcRadioPtr->IsLink16Node());
        }
        else
        {
            VRLinkReportWarning("Chat: destination node can not be on remote partitions.", 
                __FILE__, __LINE__);            
        }
    }
    if (!newEventScheduled)
    {
        newEventScheduled = true;
    }
}


// /**
// FUNCTION :: SendSimulatedMsgUsingMessenger
// PURPOSE :: Sends the simulated message via the messenger.
// PARAMETERS ::
// + srcNode : Node* : Source node pointer.
// + srcNetworkAddress : NodeAddress : Source network address.
// + destNodeId : NodeAddress : Destination node id.
// + smInfo : VRLinkSimulatedMsgInfo& : Simulated message.
// + requestedDataRate : double : Data rate.
// + dataMsgSize : unsigned : Data message size.
// + voiceMsgDuration : clocktype : Voice message duration.
// + isVoice : bool : Specifies whether the message is voice message or not.
// + timeoutDelay : clocktype : Timeout delay of the message.
// + unicast : bool : Specifies whether the message is unicast or not.
// + sendDelay : clocktype : Propagation delay of the simulated message.
// RETURN :: void : NULL
// **/
void VRLink::StartSendSimulatedMsgUsingMessenger(
    Node* srcNode,
    NodeAddress srcNetworkAddress,
    NodeAddress destNodeId,
    const VRLinkSimulatedMsgInfo& smInfo,
    double requestedDataRate,
    unsigned dataMsgSize,
    clocktype voiceMsgDuration,
    bool isVoice,
    clocktype timeoutDelay,
    bool unicast,
    clocktype sendDelay,
    bool isLink11,
    bool isLink16)
{
    // The variable below will be retrieved as an info field in the Messenger
    // result function.

    MessengerPktHeader info;

    //temp variables to pass into SetMessengerPktHeaderInfo
    unsigned short numFrags;
    NodeAddress destAddr;
    unsigned int fragSize;
    TransportTypesUsedInVRLink transportType;
    MessengerTrafficTypesUsed appType;
    clocktype freq;
    unsigned short destNPGId = 0;

    VRLinkVerify(
        (sendDelay >= 0),
        "Initial processing delay for simulated message cannot be negative",
         __FILE__, __LINE__);

    if (unicast)
    {
        destAddr = simIface->GetDefaultInterfaceAddressFromNodeId(
               srcNode, destNodeId);
    }
    else
    {
        VRLinkVerify(
            (srcNetworkAddress != 0),
            "Source network address cannot be 0.0.0.0",
            __FILE__, __LINE__);

        destAddr = srcNetworkAddress;
    }

    // Add the size of one double variable in case the compiler adds padding
    // between the Messenger data and the custom data.

    unsigned minBytes
        = sizeof(MessengerPktHeader) + sizeof(smInfo) + sizeof(double);

    // This has to be done at real source (messenger use only)
    if (simIface->IsMilitaryLibraryEnabled())
    {
        if (isLink11 || isLink16)
        {
            transportType = MAC;
        }
        else
        {
            transportType = Unreliable;

        }
        
        destNPGId = simIface->GetDestNPGId(srcNode, 0);
    }
    else
    {
        transportType = Unreliable;
    }

    // Perform different modeling depending on if the message is a data
    // message or a voice message.
    if (!isVoice)
    {
        // Data message.
        appType = GeneralTraffic;

        if (simIface->IsMilitaryLibraryEnabled() && (isLink11 || isLink16))
        {
            // For Link-11 or Link-16, just send one huge packet for this
            // data message.
            // The Link-11 model won't fragment the packet, but send the
            // whole thing in one shot, following real-world operation.
            // The Link-16 model will send the packet using a TDMA approach.

            freq = 0 * SECOND;

            if (dataMsgSize >= minBytes) 
            { 
                fragSize = dataMsgSize;
            }
            else 
            {
                fragSize = minBytes;
            }

            numFrags = 1;
        }
        else
        {
            // For non-Link-11 network devices, chunk the data message into
            // packets, each packet containing 128-bytes payload.  This
            // size was arbitrarily chosen.  Since the data message is sent
            // via UDP, the payload size per packet is limited by QualNet's
            // UDP model.
            //
            // Dividing the payload-size of each packet by the ASRS
            // interaction's DataRate parameter, one obtains the packet
            // interval.
            //
            // Dividing dataMsgSize by the payload-size of each packet,
            // one obtains the number of packets.

            unsigned payloadSize = 128;
            double packetInterval = (double) payloadSize * 8.0
                                    / requestedDataRate;

            freq = simIface->ConvertDoubleToClocktype(packetInterval);

            fragSize = payloadSize;

            numFrags = (unsigned short)
                            ceil(((double) dataMsgSize)
                                  / (double) payloadSize);

            unsigned minFragments
                = (unsigned) ceil((double) minBytes
                                  / (double) fragSize);
            if (info.numFrags < minFragments)
            {
                numFrags = minFragments;
            }
        }
    }
    else
    {
        // Voice message.

        appType = VoiceTraffic;

        if (simIface->IsMilitaryLibraryEnabled() && isLink11)
        {
            // For Link-11, the voice message is comprised of a single large
            // packet, just as with Link-11 data messages.
            //
            // Multiplying voiceMsgDuration with the PHY data rate, one
            // obtains the size of the message in bytes such that it should
            // take approximately the correct amount of time.
            //
            // Note:  The actual number of bytes should be less than the
            // value calculated as described, since there are probably
            // overhead bytes added by the Link-11 model.  This is assumed
            // to be negligible though.

            freq = 0 * SECOND;

            unsigned packetSize
                = (unsigned)
                  ((simIface->ConvertClocktypeToDouble(voiceMsgDuration))
                   * ((double) simIface->GetRxDataRate(srcNode, 0) / 8.0));

            if (packetSize >= minBytes) 
            {
                fragSize = packetSize;
            }
            else
            {
                fragSize = minBytes;
            }

            numFrags = 1;
        }
        else
        if (simIface->IsMilitaryLibraryEnabled() && isLink16)
        {
            // For Link-16, the voice message is comprised of a single large
            // packet, just as with Link-16 data messages.
            //
            // An 8-kbps data rate for the voice message is assumed.
            // Multiplying the data rate by voiceMsgDuration, one obtains the
            // size of the voice message in bytes.
            //
            // Note:  This computation implies that the entire voice message
            // is available to be transmitted on the Link-16 network at the
            // time the ASRS interaction was received.  This means that the
            // entire voice message can be transmitted in a time much less
            // than the duration of the message, given a high Link-16 data
            // rate and a large proportion of slots assigned to the source
            // node.
            //
            // This is in contrast to the Link-11 and default approach for
            // voice messages, which assumes that the voice message is being
            // read off starting at the time the ASRS was received (in the
            // Link-11 and default approach for voice, if voiceMsgDuration is
            // 10 seconds, it should take ~ 10 seconds of simulation time
            // before QualNet sends a Process Message interaction).
            //
            // The differing approach for Link-16 is assumed to be okay
            // because Link-16 operates in a TDMA fashion anyway.

            freq = 0 * SECOND;

            unsigned packetSize
                = (unsigned)
                  (simIface->ConvertClocktypeToDouble(voiceMsgDuration)
                   * (8000.0 / 8.0));

            if (packetSize >= minBytes)
            {
                fragSize = packetSize;
            }
            else
            {
                fragSize = minBytes;
            }

            numFrags = 1;
        }
        else
        {
            // For non-Link-11 network devices, an 8-kbps data rate for the
            // voice message is assumed, with a 250-byte payload per packet
            // at 4 packets per second.  (Real VoIP applications would send
            // smaller payloads at a more frequent rate to reach 8-kbps.
            // The values uses in the model are chosen to increase modeling
            // speed.)
            //
            // Dividing voiceMsgDuration by the packet interval, one obtains
            // the number of packets for the voice message.

            const double   packetInterval = 0.25;
            const unsigned payloadSize    = 250;

            freq   = simIface->ConvertDoubleToClocktype(packetInterval);
            fragSize = payloadSize;

            numFrags
                = (unsigned short)
                  ceil(simIface->ConvertClocktypeToDouble(voiceMsgDuration)
                       / packetInterval);

            unsigned minFragments
                = (unsigned) ceil((double) minBytes
                                  / (double) fragSize);
            if (info.numFrags < minFragments)
            {
                numFrags = minFragments;
            }
        }
    }//if//

    if (debug2 || debugPrintComms2)
    {
        unsigned oldPrecision = cout.precision();
        cout.precision(3);

        cout << "VRLINK CREQ: " << numFrags << " packet(s), "
             << fragSize << " bytes per packet, "
             << simIface->ConvertClocktypeToDouble(freq) << " sec " \
             << "interval"<<endl;

        cout.precision(oldPrecision);
    }
    simIface->SetMessengerPktHeader(info, 
        0,
        sendDelay,
        simIface->GetDefaultInterfaceAddressFromNodeId(
                                 srcNode,
                                 simIface->GetNodeId(srcNode)),
        destAddr,
        (int) smInfo.msgId,
        timeoutDelay,
        transportType,
        destNPGId,
        appType,
        freq,
        fragSize,
        numFrags);

    simIface->SendMessageThroughMessenger(
        srcNode,
        info,
        (char*) &smInfo,
        sizeof(smInfo),
        &VRLink::AppMessengerResultFcn);
}

// /**
// FUNCTION :: AppMessengerResultFcn
// PURPOSE :: Called when the messenger has returned a result for a given
//            message. It is called for nodes on remote partitions.
// PARAMETERS ::
// + node : Node* : Node pointer.
// + msg : Message* : message pointer.
// + success : BOOL : Indicates whether message processing was successful or
//                    not.
// RETURN :: void : NULL
// **/
void VRLink::AppMessengerResultFcn(Node* node, Message* msg, BOOL success)
{
    VRLinkExternalInterface* vrlinkExtIface = VRLinkGetIfaceDataFromNode(node);

    VRLink* vrLink = vrlinkExtIface->GetVRLinkInterfaceData();

    vrLink->AppMessengerResultFcnBody(node, msg, success);
}

// /**
// FUNCTION :: AppMessengerResultCompleted
// PURPOSE :: Core function invoked on partition 0 so that the VRLink
//            federation can indicate that the messenger has completed
//            execution.
// PARAMETERS ::
// + node : Node* : Node pointer.
// + smInfo : VRLinkSimulatedMsgInfo& : Simulated message.
// + success : BOOL : Indicates whether message processing was successful or
//                    not.
// RETURN :: void : NULL
// **/
void VRLink::AppMessengerResultCompleted(
    Node* node,
    VRLinkSimulatedMsgInfo& smInfo,
    BOOL success)
{
    VRLinkVerify(
        (simIface->GetLocalPartitionIdForNode(node) == 0),
        "Result completed function of messenger must be invoked from" \
        " partition zero",
        __FILE__, __LINE__);

    if (!success)
    {
        // Do nothing if result function indicates failure.
        //
        // Note that, for failure, the Messenger result function (this
        // function) is called with the source-node pointer.
        //
        // TODO:Code can be written to check if the destination address maps
        // to a single node (and thus qualifies as unicast). If unicast, then
        // code can be written to handle failure.
        //
        // TODO: It should be determined whether Messenger-indicated failures
        // occur for non-unicast destinations, and if so, what such a failure
        // indicates.

        return;
    }

    VRLinkRadio& srcRadio = *smInfo.srcRadioPtr;

    hash_map<NodeId, VRLinkData*>::iterator nodeDataIter;
    hash_map<unsigned int, VRLinkOutstandingSimulatedMsgInfo*>::iterator
                                                              simMsgInfoIter;

    nodeDataIter = nodeIdToPerNodeDataHash.find(simIface->GetNodeId(node));

    VRLinkVerify(nodeDataIter != nodeIdToPerNodeDataHash.end(),
                " Entry not found in nodeIdToPerNodeDataHash",
                __FILE__, __LINE__);

    VRLinkData* dstVRLinkData = nodeDataIter->second;

    VRLinkVerify(
        dstVRLinkData,
        "VRLinkData pointer not found",
        __FILE__, __LINE__);

    VRLinkRadio& dstRadio = *dstVRLinkData->GetRadioPtr();

    nodeDataIter = nodeIdToPerNodeDataHash.find(simIface->GetNodeId(smInfo.srcNode));

    VRLinkVerify(
        nodeDataIter != nodeIdToPerNodeDataHash.end(),
        " Entry not found in nodeIdToPerNodeDataHash",
        __FILE__, __LINE__);

    VRLinkData* srcVRLinkData = nodeDataIter->second;

    // This is parallel ok -
    // The smInfo's memory locations have changed, but that is ok even though
    // we're passing in the address of the msgId field, the hash functions
    // used for keyvalue always deref (and thus don't use the actual address)
    if (srcVRLinkData->GetOutstandingSimulatedMsgInfoHash()->find(smInfo.msgId) ==
        
        srcVRLinkData->GetOutstandingSimulatedMsgInfoHash()->end())
    {
        // msgId doesn't exist in hash table.  The assumption is that this
        // msgId has been added and already removed from the hash table,
        // which also means that a Timeout notification has already been sent
        // for this msgId.
        //
        // This can occur in two situations:
        //
        // While this node has successfully received the simulated message,
        //
        // (1) All destinations indicated in the initial request have already
        //     been processed, and this node is not one of the destinations.
        // (2) A timeout occurred prior to this node receiving the simulated
        //     message.
        //
        // Return early (do not send another Timeout notification).

        if (debug || debugPrintComms)
        {
            PrintCommEffectsResult(node, smInfo, "IGNORE");
        }

        return;
    }

    VRLinkOutstandingSimulatedMsgInfo& osmInfo = 
    (*(srcVRLinkData->GetOutstandingSimulatedMsgInfoHash()))[smInfo.msgId] ;

    VRLinkVerify(
        (osmInfo.smDstEntitiesInfos.size() <= g_maxMembersInNetwork),
        "Number of destination entities in outstanding simulated message " \
        "cannot exceed the maximum members in the network",
        __FILE__, __LINE__);

    VRLinkVerify(
        (osmInfo.numDstEntitiesProcessed < osmInfo.smDstEntitiesInfos.size()),
        "Number of destination entities processed in outstanding simulated" \
        " message cannot exceed the total number of destination entities",
        __FILE__, __LINE__);

    unsigned i;

    for (i = 0; i < osmInfo.smDstEntitiesInfos.size(); i++)
    {
        const VRLinkSimulatedMsgDstEntityInfo &smDstEntityInfo
            = osmInfo.smDstEntitiesInfos[i];

        if (&dstRadio)
        {
            if (smDstEntityInfo.dstEntityPtr == dstRadio.GetEntityPtr())
            {
                break;
            }
        }
    }

    if (i == osmInfo.smDstEntitiesInfos.size())
    {
        // The result function has been called with successful delivery to
        // this node, but this node isn't in the source node's list of
        // destination nodes.

        return;
    }

    VRLinkSimulatedMsgDstEntityInfo &smDstEntityInfo =
                                                osmInfo.smDstEntitiesInfos[i];

    if (smDstEntityInfo.processed)
    {
        // This destination has already been processed.
        return;
    }

    smDstEntityInfo.processed = true;
    smDstEntityInfo.success = (success == TRUE);

    osmInfo.numDstEntitiesProcessed++;

    SendProcessMsgNotification(node, smInfo, osmInfo);

    if (osmInfo.numDstEntitiesProcessed == osmInfo.smDstEntitiesInfos.size())
    {
        SendTimeoutNotification(smInfo, osmInfo);

        srcVRLinkData->GetOutstandingSimulatedMsgInfoHash()->erase(
                                                               smInfo.msgId);
    }
}

// /**
// FUNCTION :: PrintCommEffectsResult
// PURPOSE :: Core function invoked on partition 0 so that the VRLink
//            federation can indicate that the messenger has completed
//            execution.
// PARAMETERS ::
// + node : Node* : Node pointer.
// + smInfo : VRLinkSimulatedMsgInfo& : Simulated message.
// + resultString : const char* : Prints the result of communication.
//                    not.
// RETURN :: void : NULL
// **/
void VRLink::PrintCommEffectsResult(
    Node* node,
    const VRLinkSimulatedMsgInfo& smInfo,
    const char* resultString)
{
    const VRLinkRadio* srcRadio = smInfo.srcRadioPtr;

    hash_map<NodeId, VRLinkData*>::iterator nodeDataIter;

    nodeDataIter = nodeIdToPerNodeDataHash.find(simIface->GetNodeId(node));

    VRLinkVerify(nodeDataIter != nodeIdToPerNodeDataHash.end(),
                "Entry not found in nodeIdToPerNodeDataHash");

    VRLinkData* dstVRLinkData = nodeDataIter->second;

    VRLinkVerify(
        dstVRLinkData,
        "VRLinkData pointer not found",
        __FILE__, __LINE__);

    const VRLinkRadio* dstRadio = dstVRLinkData->GetRadioPtr();

    char clocktypeString[g_ClocktypeStringBufSize];
    simIface->PrintClockInSecond(simIface->GetCurrentSimTime(node), clocktypeString);

    if (strcmp(resultString, "SUCCESS MSG NOTIFY") != 0 && 
        strcmp(resultString, "IGNORE") != 0)
    {
        if (srcRadio)
        {
            cout << "VRLINK: " << resultString << ", "
             << srcRadio->GetMarkingData()<< " ("
             << simIface->GetNodeId(srcRadio->GetNode()) << ","
             << srcRadio->GetRadioIndex()
             << "); time: " << clocktypeString 
             << ", msgId: " << smInfo.msgId << endl;
        }
        else
        {
            cout << "VRLINK: " << resultString << ", "
             << simIface->GetNodeId(smInfo.srcNode) 
             << "; time: " << clocktypeString 
             << ", msgId: " << smInfo.msgId << endl;
        }
    }
    else
    {
        if (srcRadio && dstRadio)
        {
            cout << "VRLINK: " << resultString << ", "
                     << srcRadio->GetMarkingData()<< " ("
                     << simIface->GetNodeId(srcRadio->GetNode()) << ","
                     << srcRadio->GetRadioIndex()
                    << ") to "
                     << dstRadio->GetMarkingData() << " ("
                     << simIface->GetNodeId(dstRadio->GetNode()) << ","
                     << dstRadio->GetRadioIndex()
                     << "), time: " << clocktypeString 
                    << ", msgId: " << smInfo.msgId << endl;
        }
        else
        {
            /*if (smInfo.destEntityId)
            {
            cout << "FED: " << resultString << ", "
                 << simIface->GetNodeId(smInfo.srcNode) << " to "
                 << "entity "<<smInfo.destEntityId->string()
                 << "; time " << clocktypeString << endl;
            }else {
                cout << "FED: " << resultString << ", "
                 << simIface->GetNodeId(smInfo.srcNode) << " to "
                 << "entity "
                 << "; time " << clocktypeString << endl;
            }*/

            cout << "VRLINK: " << resultString << ", "
                 << simIface->GetNodeId(smInfo.srcNode) << " to "
                 << simIface->GetNodeId(node)
                 << "; time: " << clocktypeString 
                 << ", msgId: " << smInfo.msgId << endl;
        }
    }
}

// /**
// FUNCTION :: AppMessengerResultFcnBody
// PURPOSE :: Forwards the message to partition 0 in case it is on a
//            partition other than 0 (for parallel processing), else calls
//            another function to end the message execution.
// PARAMETERS ::
// + node : Node* : Node pointer.
// + msg : Message* : Message pointer.
// + success : BOOL : Indicates whether message processing was successful or
//                    not.
// RETURN :: void : NULL
// **/
void VRLink::AppMessengerResultFcnBody(
    Node* node,
    Message* msg,
    BOOL success)
{
    
    if (!success)
    {
        // Do nothing if result function indicates failure.
        //
        // Note that, for failure, the Messenger result function (this
        // function) is called with the source-node pointer.
        //
        // TODO:  Code can be written to check if the destination address maps
        // to a single node (and thus qualifies as unicast).  If unicast, then
        // code can be written to handle failure.
        //
        // TODO:  It should be determined whether Messenger-indicated failures
        // occur for non-unicast destinations, and if so, what such a failure
        // indicates.

        return;
    }

    VRLinkSimulatedMsgInfo& smInfo
        = *((VRLinkSimulatedMsgInfo*) simIface->ReturnPacketFromMsg(msg));

    if (simIface->GetLocalPartitionIdForNode(node) != 0)
    {
        // Forward this message back to partition 0 to
        // indicate the comm request was successful.
        Message * completedMessengerMsg = simIface->AllocateMessage(
            node,
            simIface->GetExternalLayerId(),    // special layer
            simIface->GetVRLinkExternalInterfaceType(),   // protocol
            simIface->GetMessageEventType(EXTERNAL_VRLINK_CompletedMessengerForwarded));

        EXTERNAL_VRLinkCompletedMessenegerForwardedInfo * completedInfo =
            (EXTERNAL_VRLinkCompletedMessenegerForwardedInfo *)
            simIface->AllocateInfoInMsg (node, completedMessengerMsg,
            sizeof (EXTERNAL_VRLinkCompletedMessenegerForwardedInfo));

        completedInfo->destNodeId = simIface->GetNodeId(node);    // will be a proxy on p0
        completedInfo->smInfo = smInfo;
        completedInfo->success = TRUE;

        EXTERNAL_Interface* iface =
                               simIface->GetExternalInterfaceForVRLink(node);

        simIface->RemoteExternalSendMessage (
            iface,
            0 /* p0 */,
            completedMessengerMsg,
            0,
            simIface->GetExternalScheduleSafeType());

        return;
    }
    else
    {
        AppMessengerResultCompleted (node, smInfo, success);
        return;
    }
}

// /**
// FUNCTION :: SendProcessMsgNotification
// PURPOSE :: Virtual function to send notification to process message to
//            another federate.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + smInfo : VRLinkSimulatedMsgInfo& : Simulated message infomation.
// + osmInfo : VRLinkOutstandingSimulatedMsgInfo& : Outstanding simulated
//                                                  message infomation.
// RETURN :: void : NULL
// **/
void VRLink::SendProcessMsgNotification(
    Node* node,
    VRLinkSimulatedMsgInfo& smInfo,
    const VRLinkOutstandingSimulatedMsgInfo& osmInfo)
{
    DtDataInteraction pdu;
    DtDatumRetCode retcode;

     //All fields except Variable Datum Value are copied in host-byte-order
     //and swapped in a function call.

    const VRLinkRadio& srcRadio = *smInfo.srcRadioPtr;

    hash_map<NodeId, VRLinkData*>::iterator nodeDataIter;

    nodeDataIter = nodeIdToPerNodeDataHash.find(simIface->GetNodeId(node));

    VRLinkVerify(nodeDataIter != nodeIdToPerNodeDataHash.end(),
                 "Entry not found in nodeIdToPerNodeDataHash",
                 __FILE__, __LINE__);

    VRLinkData* dstDisData = nodeDataIter->second;
    VRLinkRadio& dstRadio = *dstDisData->GetRadioPtr();

    VRLinkVerify(
        dstDisData,
        "VRLinkData pointer not found",
        __FILE__, __LINE__);

    DtEntityIdentifier senderId;
    if (&srcRadio)
    {
        senderId = srcRadio.GetEntityPtr()->GetEntityId();
    }
    else if (simIface->GetNodeId(smInfo.srcNode))
    {
        senderId = *GetEntityPtrFromNodeIdToEntityIdHash(
                                                     simIface->GetNodeId(smInfo.srcNode));
        if (senderId == NULL)
        {
            VRLinkReportWarning(
                "EntityPtr not found for srcNode, skipping process msg notification",
                __FILE__, __LINE__);
            
            return;
        }
    }
    else
    {
        simIface->ERROR_ReportError("can't get the sender id");
    }

    pdu.setSenderId(senderId);
    pdu.setNumVarFields(1);
    pdu.setDatumParam(DtVAR, 1, processMsgNotificationDatumId, &retcode);

    if (retcode == DtDATUM_BAD_INDEX)
    {
       VRLinkReportError(
           "Invalid datum index or data",
           __FILE__, __LINE__);
    }

    unsigned char varDatumValue[g_DatumValueBufSize];
    unsigned char* vdv = varDatumValue;
    unsigned vdvOffset = 0;
    
    UInt16 site = pdu.senderId().site();
    UInt16 host = pdu.senderId().host();
    UInt16 num = pdu.senderId().entityNum();

    if (DEBUG_NOTIFICATION)
    {
        cout << "Sending Processed Msg Notification: { \nSender(site: " << site << 
            " host: " << host << " num: " << num << ")" << endl;
    }
    simIface->EXTERNAL_hton(&site, sizeof(site));
    simIface->EXTERNAL_hton(&host, sizeof(host));
    simIface->EXTERNAL_hton(&num, sizeof(num));

    CopyToOffset(vdv, vdvOffset, &site, sizeof(site));
    CopyToOffset(vdv, vdvOffset, &host, sizeof(host));
    CopyToOffset(vdv, vdvOffset, &num, sizeof(num));

    UInt16 srcRadioIndex = 0;

    if (&srcRadio)
    {
        srcRadioIndex = srcRadio.GetRadioIndex();
    }
   
    UInt32 timestamp = osmInfo.timestamp;  
    UInt32 numEntityIds = 1;
    
    if (DEBUG_NOTIFICATION)
    {
        cout << "radioIndex: " << srcRadioIndex << " timestamp: " << timestamp << 
            " numEntities: " << numEntityIds <<endl;
    }

    simIface->EXTERNAL_hton(&srcRadioIndex, sizeof(srcRadioIndex));
    CopyToOffset(vdv, vdvOffset, &srcRadioIndex, sizeof(srcRadioIndex));

    simIface->EXTERNAL_hton(&timestamp, sizeof(timestamp));
    CopyToOffset(
        vdv,
        vdvOffset,
        &timestamp,
        sizeof(timestamp));

    simIface->EXTERNAL_hton(&numEntityIds, sizeof(numEntityIds));
    CopyToOffset(vdv, vdvOffset, &numEntityIds, sizeof(numEntityIds));

    if (&dstRadio)
    {
        pdu.setReceiverId(dstRadio.GetEntityPtr()->GetEntityId());
        site = dstRadio.GetEntityPtr()->GetEntityId().site();
        host = dstRadio.GetEntityPtr()->GetEntityId().host();
        num = dstRadio.GetEntityPtr()->GetEntityId().entityNum();
    }
    else
    {
        NodeId id = simIface->GetNodeId(node);
        pdu.setReceiverId(nodeIdToEntityIdHash.find(id)->second);
        site = nodeIdToEntityIdHash.find(id)->second.site();
        host = nodeIdToEntityIdHash.find(id)->second.host();
        num = nodeIdToEntityIdHash.find(id)->second.entityNum();
    }
    
    if (DEBUG_NOTIFICATION)
    {
        cout << "Receiver(site: " << site << 
            " host: " << host << " num: " << num << ")";
    }
    simIface->EXTERNAL_hton(&site, sizeof(site));
    simIface->EXTERNAL_hton(&host, sizeof(host));
    simIface->EXTERNAL_hton(&num, sizeof(num));

    CopyToOffset(vdv, vdvOffset, &site, sizeof(site));
    CopyToOffset(vdv, vdvOffset, &host, sizeof(host));
    CopyToOffset(vdv, vdvOffset, &num, sizeof(num));

    clocktype delay = simIface->GetCurrentSimTime(node) - osmInfo.sendTime;
    Float64 double_delay = simIface->ConvertClocktypeToDouble(delay);

    if (DEBUG_NOTIFICATION)
    {
        cout << " delay: " << double_delay << endl ;
    }

    simIface->EXTERNAL_hton(&double_delay, sizeof(double_delay));
    CopyToOffset(vdv, vdvOffset, &double_delay, sizeof(double_delay));

    pdu.setVarDataBytes(1, vdvOffset, &retcode);

    if (retcode == DtDATUM_BAD_INDEX)
    {
         VRLinkReportError(
             "Invalid datum index or data",
             __FILE__, __LINE__);
    }

    pdu.setDatumValByteArray(
            DtVar,
            1,
            (const char*)vdv,
            &retcode);

    if (retcode == DtDATUM_BAD_INDEX)
    {
        VRLinkReportError(
            "Invalid datum index or data",
            __FILE__, __LINE__);
    }

    exConn->sendStamped(pdu);

    if (debug || debugPrintComms)
    {
        if (DEBUG_NOTIFICATION)
        {
            cout << "Size of Processed Msg Notification: " << vdvOffset << "\n}\n";
        }
        PrintCommEffectsResult(node, smInfo, "SUCCESS MSG NOTIFY");
    }
}

// /**
// FUNCTION :: SendTimeoutNotification
// PURPOSE :: Sends timeout notification.
// PARAMETERS ::
// + smInfo : const VRLinkSimulatedMsgInfo& : Simulated message infomation.
// + osmInfo : const VRLinkOutstandingSimulatedMsgInfo& : Outstanding
//                                              simulated message infomation.
// RETURN :: void : NULL
// **/
void VRLink::SendTimeoutNotification(
    const VRLinkSimulatedMsgInfo& smInfo,
    const VRLinkOutstandingSimulatedMsgInfo& osmInfo)
{
    // All fields except Variable Datum Value are copied in host-byte-order
    // and swapped in a function call.

    DtDataInteraction pdu;
    DtDatumRetCode retcode;
    const VRLinkRadio& srcRadio = *smInfo.srcRadioPtr;

    DtEntityIdentifier senderId;
    if (&srcRadio)
    {
        senderId = srcRadio.GetEntityPtr()->GetEntityId();
    }
    else if (simIface->GetNodeId(smInfo.srcNode))
    {
        senderId = *GetEntityPtrFromNodeIdToEntityIdHash(
                                                     simIface->GetNodeId(smInfo.srcNode));

        if (senderId == NULL)
        {
            VRLinkReportWarning(
                "EntityPtr not found for srcNode, skipping timeout notification",
                __FILE__, __LINE__);
            
            return;
        }
    }
    else
    {
        simIface->ERROR_ReportError("can't get the sender id");
    }

    pdu.setSenderId(senderId);
    pdu.setNumVarFields(1);
    pdu.setDatumParam(DtVAR, 1, timeoutNotificationDatumId, &retcode);

    if (retcode == DtDATUM_BAD_INDEX)
    {
         VRLinkReportError(
             "Invalid datum index or data",
             __FILE__, __LINE__);
    }

    unsigned char varDatumValue[g_DatumValueBufSize];
    unsigned char* vdv = varDatumValue;
    unsigned vdvOffset = 0;
    
    UInt16 site = pdu.senderId().site();    
    UInt16 host = pdu.senderId().host();    
    UInt16 num = pdu.senderId().entityNum();    

    if (DEBUG_NOTIFICATION)
    {
        cout << "Sending Timeout Notification: {\nSender(site: " << site << 
            " host: " << host << " num: " << num << ")" << endl;
    }

    simIface->EXTERNAL_hton(&site, sizeof(site));
    simIface->EXTERNAL_hton(&host, sizeof(host));
    simIface->EXTERNAL_hton(&num, sizeof(num));

    CopyToOffset(vdv, vdvOffset, &site, sizeof(site));
    CopyToOffset(vdv, vdvOffset, &host, sizeof(host));
    CopyToOffset(vdv, vdvOffset, &num, sizeof(num));

    unsigned short srcRadioIndex = 0;

    if (&srcRadio)
    {
        srcRadioIndex = srcRadio.GetRadioIndex();
    }

    UInt32 timestamp = osmInfo.timestamp;
    UInt32 numPackets = 1;
       
    if (DEBUG_NOTIFICATION)
    {
        cout << "radioIndex: " << srcRadioIndex << " timestamp: " 
            << timestamp << " numPackets: " << numPackets << endl;
    }

    simIface->EXTERNAL_hton(&srcRadioIndex, sizeof(srcRadioIndex));
    CopyToOffset(vdv, vdvOffset, &srcRadioIndex, sizeof(srcRadioIndex));

    simIface->EXTERNAL_hton(&timestamp, sizeof(timestamp));
    CopyToOffset(
        vdv,
        vdvOffset,
        &timestamp,
        sizeof(timestamp));    

    simIface->EXTERNAL_hton(&numPackets, sizeof(numPackets));
    CopyToOffset(vdv, vdvOffset, &numPackets, sizeof(numPackets));

    UInt32 numEntityIds = 0;
    unsigned numEntityIdsOffset = vdvOffset;
    vdvOffset += sizeof(numEntityIds);

    VRLinkVerify(
        (osmInfo.smDstEntitiesInfos.size() <= g_maxMembersInNetwork),
        "Number of destination entities in outstanding simulated message " \
        "cannot exceed the maximum members in the network",
        __FILE__, __LINE__);

    VRLinkVerify(
        (osmInfo.smDstEntitiesInfos.size() <= g_maxDstEntityIdsInTimeoutNotification),
        "Number of destination entities in outstanding simulated message " \
        "cannot exceed the maximum destination entity ids in timeout" \
        " notification",
        __FILE__, __LINE__);

    unsigned i;

    for (i = 0; i < osmInfo.smDstEntitiesInfos.size(); i++)
    {
        const VRLinkSimulatedMsgDstEntityInfo& smDstEntityInfo
            = osmInfo.smDstEntitiesInfos[i];

        if (&srcRadio)
        {
            VRLinkVerify(
                (smDstEntityInfo.dstEntityPtr != srcRadio.GetEntityPtr()),
                "Source and destination entities cannot be same",
                __FILE__, __LINE__);

            site = smDstEntityInfo.dstEntityPtr->GetEntityId().site();
            host = smDstEntityInfo.dstEntityPtr->GetEntityId().host();
            num = smDstEntityInfo.dstEntityPtr->GetEntityId().entityNum();

            if (DEBUG_NOTIFICATION)
            {
                cout << "Receiver(site: " << site << 
                    " host: " << host << " num: " << num;
            }
            simIface->EXTERNAL_hton(&site, sizeof(site));
            simIface->EXTERNAL_hton(&host, sizeof(host));
            simIface->EXTERNAL_hton(&num, sizeof(num));

            CopyToOffset(vdv, vdvOffset, &site, sizeof(site));
            CopyToOffset(vdv, vdvOffset, &host, sizeof(host));
            CopyToOffset(vdv, vdvOffset, &num, sizeof(num));
        }

        bool success = smDstEntityInfo.success;

        if (DEBUG_NOTIFICATION)
        {
            cout << " success: " << success << ")\n";
        }
        simIface->EXTERNAL_hton(&success, sizeof(success));
        CopyToOffset(vdv, vdvOffset, &success, sizeof(success));

        numEntityIds++;
    }

    if (DEBUG_NOTIFICATION)
    {
        cout << "numReceivingEntities: " << numEntityIds << endl;
    }

    if (osmInfo.smDstEntitiesInfos.size() == 1)
    {
        pdu.setReceiverId(osmInfo.smDstEntitiesInfos[0].dstEntityPtr->GetEntityId());
    }
    else
    {
        pdu.setReceiverId(DtEntityIdentifier::defaultId());
    }

    simIface->EXTERNAL_hton(&numEntityIds, sizeof(numEntityIds));
    CopyToOffset(
        vdv,
        numEntityIdsOffset,
        &numEntityIds,
        sizeof(numEntityIds));

    pdu.setVarDataBytes(1, vdvOffset, &retcode);

    if (retcode == DtDATUM_BAD_INDEX)
    {
         VRLinkReportError(
             "Invalid datum index or data",
             __FILE__, __LINE__);
    }

    pdu.setDatumValByteArray(
            DtVar,
            1,
            (const char*)vdv,
            &retcode);

    if (retcode == DtDATUM_BAD_INDEX)
    {
        VRLinkReportError(
            "Invalid datum index or data",
            __FILE__, __LINE__);
    }

    if (debug || debugPrintComms)
    {
        if (DEBUG_NOTIFICATION)
        {
            cout << "Size of Timeout Notification: " << vdvOffset << "\n}\n";
        }
        PrintCommEffectsResult(smInfo.srcNode, smInfo, "TIMEOUT NOTIFY");
    }

    exConn->sendStamped(pdu);
}

// /**
// FUNCTION :: PrintCommEffectsRequestProcessed
// PURPOSE :: Prints the result of the communication after processing it.
// PARAMETERS ::
// + srcRadio : const VRLinkRadio* : Radio sending the interaction.
// + dstEntityPtr : const VRLinkEntity* : Entity destined to receive the
//                                        interaction.
// + unicast : bool : Specifies whether the message is unicast or not.
// + dstRadioPtr : const VRLinkRadio* : Radio destined to reecive the
//                                      interaction.
// + sendTime : clocktype : Time at which interaction was sent.
// + msgId : unsigned int : msgId of simulated msg/CREQ
// RETURN :: void : NULL
// **/
void VRLink::PrintCommEffectsRequestProcessed(
    const VRLinkRadio* srcRadio,
    const VRLinkEntity* dstEntityPtr,
    bool unicast,
    const VRLinkRadio* dstRadioPtr,
    clocktype sendTime,
    unsigned int msgId)
{
    if (vrLinkType == VRLINK_TYPE_HLA13 || vrLinkType == VRLINK_TYPE_HLA1516)
    {
        cout << "VRLINK CREQ: "
                << simIface->GetNodeId(srcRadio->GetNode()) << " ("
                << srcRadio->GetEntityPtr()->GetEntityIdString()<< ", "
                << srcRadio->GetRadioIndex() << ") "
                << srcRadio->GetMarkingData();
    }
    else if (vrLinkType == VRLINK_TYPE_DIS)
    {
        cout << "VRLINK CREQ: "
            << srcRadio->GetMarkingData() << " ("
            << simIface->GetNodeId(srcRadio->GetNode()) << ","
            << srcRadio->GetRadioIndex() << ")";
    }

    if (unicast)
    {
        VRLinkVerify(
            (dstEntityPtr != NULL),
            "Destination entity not specified",
            __FILE__, __LINE__);

        VRLinkVerify(
            (dstRadioPtr != NULL),
            "Destination radio not specified",
            __FILE__, __LINE__);

        if ((vrLinkType == VRLINK_TYPE_HLA13) ||
            (vrLinkType == VRLINK_TYPE_HLA1516))
        {
            cout << " unicast to " << simIface->GetNodeId(dstRadioPtr->GetNode())
                    << " (" << dstEntityPtr->GetEntityIdString() << ") "
                    << dstEntityPtr->GetMarkingData();
        }
        else if (vrLinkType == VRLINK_TYPE_DIS)
        {
            cout << " unicast to " << dstEntityPtr->GetMarkingData() << " ("
                << simIface->GetNodeId(dstRadioPtr->GetNode()) << ","
                << dstRadioPtr->GetRadioIndex() << ")";
        }
    }
    else
    {
        cout << " broadcast";

        if (dstEntityPtr != NULL)
        {
            if ((vrLinkType == VRLINK_TYPE_HLA13) ||
                (vrLinkType == VRLINK_TYPE_HLA1516))
            {
                cout << " to (" << dstEntityPtr->GetEntityIdString() << ") "
                     << dstEntityPtr->GetMarkingData();
            }
            else if (vrLinkType == VRLINK_TYPE_DIS)
            {
                cout << " to " << dstEntityPtr->GetMarkingData();
            }
        }
    }

    char sendTimeString[MAX_STRING_LENGTH];
    simIface->PrintClockInSecond(sendTime, sendTimeString);

    cout << ", send time " << sendTimeString << ", msgId: " << msgId << endl;
}

// /**
// FUNCTION :: StoreOutstandingSimulatedMsgInfo
// PURPOSE :: Stores the outstanding simulated message in a hash for
//            retrieving it later.
// PARAMETERS ::
// + smInfo : VRLinkSimulatedMsgInfo : Simulated message.
// + dstEntityPtr : const VRLinkEntity* : Entity destined to receive the
//                                        interaction.
// + timestamp : unsigned : Timestamp of the received interaction.
// + sendTime : clocktype : Time at which interaction was sent.
// + dstEntityPtrs : set<VRLinkEntity*> : set of destination entity pointers for msg
// RETURN :: void : NULL
// **/
void VRLink::StoreOutstandingSimulatedMsgInfo(
    const VRLinkSimulatedMsgInfo& smInfo,
    VRLinkEntity* dstEntityPtr,
    unsigned timestamp,
    clocktype sendTime,
    const set<VRLinkEntity*>& dstEntityPtrs )
{
    VRLinkRadio& srcRadio = *smInfo.srcRadioPtr;
    Node* srcNode = smInfo.srcNode;

    hash_map<NodeId, VRLinkData*>::iterator nodeDataIter;

    nodeDataIter = nodeIdToPerNodeDataHash.find(simIface->GetNodeId(srcNode));

    VRLinkVerify(nodeDataIter != nodeIdToPerNodeDataHash.end(),
                " Entry not found in nodeIdToPerNodeDataHash",
                __FILE__, __LINE__);

    VRLinkData* vrlinkData = nodeDataIter->second;

    VRLinkVerify(
        vrlinkData,
        "VRLinkData pointer not found",
        __FILE__, __LINE__);

    VRLinkOutstandingSimulatedMsgInfo& osmInfo
        = (*(vrlinkData->GetOutstandingSimulatedMsgInfoHash()))[smInfo.msgId];

    osmInfo.timestamp = timestamp;
    osmInfo.sendTime = sendTime;

    osmInfo.smDstEntitiesInfos.clear();
    osmInfo.numDstEntitiesProcessed = 0;

    {
        set<VRLinkEntity*>::const_iterator it = dstEntityPtrs.begin();
        while (it!= dstEntityPtrs.end())
        {
            VRLinkSimulatedMsgDstEntityInfo smDstEntityInfo ;
            smDstEntityInfo.dstEntityPtr = *it;
            smDstEntityInfo.processed    = false;
            smDstEntityInfo.success      = false;

            osmInfo.smDstEntitiesInfos.push_back(smDstEntityInfo);
            it++;
        }

        VRLinkVerify(
            (osmInfo.smDstEntitiesInfos.size()> 0),
            "Number of destination entities in outstanding simulated " \
            "message cannot be negative",
            __FILE__, __LINE__);
    }
}

// /**
// FUNCTION :: StoreOutstandingSimulatedMsgInfo
// PURPOSE :: Stores the outstanding simulated message in a hash for
//            retrieving it later.
// PARAMETERS ::
// + smInfo : const VRLinkSimulatedMsgInfo& : Simulated message.
// + dstEntityPtr : const VRLinkEntity* : Entity destined to receive the
//                                        interaction.
// + timestamp : unsigned : Timestamp of the received interaction.
// + sendTime : clocktype : Time at which interaction was sent.
// RETURN :: void : NULL
// **/
void VRLink::StoreOutstandingSimulatedMsgInfo(
    const VRLinkSimulatedMsgInfo& smInfo,
    VRLinkEntity* dstEntityPtr,
    unsigned timestamp,
    clocktype sendTime)
{
    VRLinkRadio& srcRadio = *smInfo.srcRadioPtr;
    Node* srcNode = smInfo.srcNode;

    hash_map<NodeId, VRLinkData*>::iterator nodeDataIter;

    nodeDataIter = nodeIdToPerNodeDataHash.find(simIface->GetNodeId(srcNode));

    VRLinkVerify(nodeDataIter != nodeIdToPerNodeDataHash.end(),
                " Entry not found in nodeIdToPerNodeDataHash",
                __FILE__, __LINE__);

    VRLinkData* vrlinkData = nodeDataIter->second;

    VRLinkVerify(
        vrlinkData,
        "VRLinkData pointer not found",
        __FILE__, __LINE__);

    VRLinkOutstandingSimulatedMsgInfo& osmInfo
        = (*(vrlinkData->GetOutstandingSimulatedMsgInfoHash()))[smInfo.msgId];

    osmInfo.timestamp = timestamp;
    osmInfo.sendTime = sendTime;

    osmInfo.smDstEntitiesInfos.clear();
    osmInfo.numDstEntitiesProcessed = 0;

    if (dstEntityPtr /*|| destEntityId*/)
    {        

        VRLinkSimulatedMsgDstEntityInfo smDstEntityInfo ;
        smDstEntityInfo.dstEntityPtr = dstEntityPtr;
        smDstEntityInfo.processed    = false;
        smDstEntityInfo.success      = false;
        
        osmInfo.smDstEntitiesInfos.push_back(smDstEntityInfo);
    }
    
    else
    {
        const VRLinkNetwork& network = *srcRadio.GetNetworkPtr();

        const hash_map<NodeId, VRLinkRadio*>& networkRadioPtrHash =
                                                  network.GetRadioPtrsHash();

        hash_map<NodeId, VRLinkRadio*>::const_iterator networkRadioPtrsIter;

        for (networkRadioPtrsIter = networkRadioPtrHash.begin();
                     networkRadioPtrsIter != networkRadioPtrHash.end();
                             networkRadioPtrsIter++)
        {
            const VRLinkRadio* dstRadio = networkRadioPtrsIter->second;

            if (simIface->GetNodeId(dstRadio->GetNode()) ==
                simIface->GetNodeId(srcRadio.GetNode()))
            {
                continue;
            }

            VRLinkSimulatedMsgDstEntityInfo smDstEntityInfo ;
            smDstEntityInfo.dstEntityPtr = dstRadio->GetEntityPtr();
            smDstEntityInfo.processed    = false;
            smDstEntityInfo.success      = false;

            osmInfo.smDstEntitiesInfos.push_back(smDstEntityInfo);
        }

        VRLinkVerify(
            //(osmInfo->numDstEntities > 0),
            (osmInfo.smDstEntitiesInfos.size()> 0),
            "Number of destination entities in outstanding simulated " \
            "message cannot be negative",
            __FILE__, __LINE__);
    }
}

// /**
// FUNCTION :: ParseMsgString
// PURPOSE :: Parses the data stream received via signal interaction.
// PARAMETERS ::
// + msgString : char* : Data stream to be parsed.
// + dstEntityPtr : const VRLinkEntity** : Entity destined to receive the
//                                         interaction.
// + dataMsgSize : unsigned : Data message size.
// + voiceMsgDuration : clocktype& : Voice message duration.
// + isVoice : bool& : Specifies whether the message is voice message or not.
// + timeoutDelay : clocktype& : Timeout delay of the message.
// + timestamp : unsigned : Timestamp of the received interaction.
// RETURN :: bool : Returns TRUE if a valid message was received and was
//                  successfully parsed; FALSE otherwise.
// **/
bool VRLink::ParseMsgString(
    char* msgString,
    const VRLinkEntity** dstEntityPtr,
    unsigned& dataMsgSize,
    clocktype& voiceMsgDuration,
    bool& isVoice,
    clocktype& timeoutDelay,
    unsigned& timestamp,   
    DtEntityIdentifier** destinationEntityId
    )
{
    char token[g_SignalDataBufSize] = "";
    char* next = msgString;

    simIface->GetDelimitedToken(token, next, "\n", &next);
    if (strcmp(token, "HEADER") != 0)
    {
        VRLinkReportWarning("Message string did not start with HEADER");
        return false;
    }

    char name[g_SignalDataBufSize];
    char value[g_SignalDataBufSize];
    char* nextNameValue;

    // Required fields.
    bool foundSizeField = false;
    bool foundTimeoutField = false;
    bool foundTimestampField = false;

    if (debug || debugPrintComms2)
    {
        cout << "VRLINK CREQ: ";
    }

    while (simIface->GetDelimitedToken(token, next, "\n", &next))
    {
        if (strcmp(token, "EOH") == 0)
        {
            if (debug || debugPrintComms2)
            {
                cout << endl;
            }

            break;
        }

        if (debug || debugPrintComms2)
        {
            cout << token << " ";
        }

        if (!simIface->GetDelimitedToken(name, token, "=", &nextNameValue))
        {
            VRLinkReportWarning(
                "Can't find attribute=value in message string");
            
            return false;
        }

        if (!simIface->GetDelimitedToken(value,
                                nextNameValue, "=", &nextNameValue))
        {
            VRLinkReportWarning(
                "Can't find attribute=value in message string");
            return false;
        }

        if (strcmp(name, "receiver") == 0)
        {
            unsigned short dstSite;
            unsigned short dstApplId;
            unsigned short dstEntityNumber;

            if (sscanf(value, "%hu.%hu.%hu",
                       &dstSite,
                       &dstApplId,
                       &dstEntityNumber) != 3)
            {
                VRLinkReportWarning(
                    "\nCan't read receiver field in message string",
                    __FILE__, __LINE__);
                return false;
            }

            DtEntityIdentifier dstEntityId(value);

            hash_map <string, VRLinkEntity*>::iterator 
                reflEntitiesIter = reflectedEntities.find(
                                                      dstEntityId.string());

            if (reflEntitiesIter == reflectedEntities.end())
            {

                EntityIdToNodeIdIter entityIdToNodeIdIter;
                EntityIdentifierKey* entityKey = GetEntityIdKeyFromDtEntityId(
                                                               &dstEntityId);

                entityIdToNodeIdIter = entityIdToNodeIdMap.find(*entityKey);

                if (entityIdToNodeIdIter == entityIdToNodeIdMap.end())
                {                
                    VRLinkReportWarning(
                       "Can't map receiver EntityID in message string to entity",
                       __FILE__, __LINE__);

                    return false;
                }
                else {
                    *destinationEntityId = &dstEntityId;
                }
            }
            else {
                *dstEntityPtr = reflEntitiesIter->second;
            }
        }
        else
        if (strcmp(name, "size") == 0)
        {
            double sizeValue;
            char unitString[g_SignalDataBufSize];

            if (sscanf(value, "%lf %s", &sizeValue, unitString) != 2
                || sizeValue < 0.0
                || sizeValue > (double) UINT_MAX)
            {
                VRLinkReportWarning("Bad size field in message string",
                                 __FILE__, __LINE__);
                return false;
            }

            // TODO:  Fractional sizes (either bytes or seconds) are rounded
            // down here.  Perhaps fractional values should be retained,
            // especially for seconds.

            if (strcmp(unitString, "bytes") == 0)
            {
                dataMsgSize = (unsigned) sizeValue;

                isVoice = false;
            }
            else if (strcmp(unitString, "seconds") == 0)
            {
                voiceMsgDuration =
                               simIface->ConvertDoubleToClocktype(sizeValue);

                isVoice = true;
            }
            else
            {
                VRLinkReportWarning("Unrecognized units in size field",
                                 __FILE__, __LINE__);
                return false;
            }

            foundSizeField = true;
        }
        else if (!strcmp(name, "timeout"))
        {
            char* endPtr = NULL;
            errno = 0;
            double double_timeoutDelay = strtod(value, &endPtr);

            if (endPtr == value || errno != 0)
            {
                VRLinkReportWarning("Bad timeout field in message string",
                                 __FILE__, __LINE__);
                return false;
            }

            timeoutDelay =
                     simIface->ConvertDoubleToClocktype(double_timeoutDelay);
            foundTimeoutField = true;
        }
        else if (!strcmp(name, "timestamp"))
        {
            // 0x34567890, 10 bytes.

            const unsigned timestampStringLength = 10;

            if (strlen(value) != timestampStringLength
                || !(value[0] == '0' && (value[1] == 'x' || value[1] == 'X'))
                || sscanf(&value[2], "%x", &timestamp) != 1)
            {
                VRLinkReportWarning("Bad timestamp field in message string",
                                 __FILE__, __LINE__);
                return false;
            }

            foundTimestampField = true;
        }
        
    }//while//
    
    if (!(foundSizeField && foundTimeoutField && foundTimestampField))
    {
        VRLinkReportWarning(
            "All essential fields not present in message string",
            __FILE__, __LINE__);
        return false;
    }
    
   return true;
}

// /**
// FUNCTION :: ScheduleTimeout
// PURPOSE :: Schedules timeout for a received interaction.
// PARAMETERS ::
// + smInfo : const VRLinkSimulatedMsgInfo& : Simulated message.
// + timeoutDelay : clocktype& : Timeout delay of the message.
// + sendDelay : clocktype : Propagation delay of the simulated message.
// RETURN :: void : NULL.
// **/
void VRLink::ScheduleTimeout(
    const VRLinkSimulatedMsgInfo& smInfo,
    clocktype timeoutDelay,
    clocktype sendDelay)
{
    Node* firstNode = simIface->GetFirstNodeOnPartition(iface);
    Message* timeoutMsg;

    timeoutMsg = simIface->AllocateMessage(
                               firstNode,
                               simIface->GetExternalLayerId(),
                               simIface->GetVRLinkExternalInterfaceType(),
                               simIface->GetMessageEventType(EXTERNAL_VRLINK_AckTimeout));

    simIface->AllocateInfoInMsg(firstNode, timeoutMsg, sizeof(smInfo));

    VRLinkSimulatedMsgInfo& timeoutHeader =
        *((VRLinkSimulatedMsgInfo*) simIface->ReturnInfoFromMsg(timeoutMsg));

    timeoutHeader = smInfo;

    VRLinkVerify(
        (timeoutDelay >= 0),
        "Timeout delay cannot be negative",
        __FILE__, __LINE__);

    VRLinkVerify(
        (sendDelay >= 0),
        "Send delay cannot be negative",
        __FILE__, __LINE__);

    clocktype delay = sendDelay + timeoutDelay;

    simIface->SendMessageWithDelay (firstNode, timeoutMsg, delay);

    if (!newEventScheduled) { newEventScheduled = true; }
}

// /**
// FUNCTION :: EndSimulation
// PURPOSE :: To end the simulation.
// PARAMETERS :: None.
// RETURN :: void : NULL.
// **/
void VRLink::EndSimulation()
{
    cout << endl << "Ending program ... " << endl;
    endProgram = true;

    simIface->SetExternalSimulationEndTime(iface,
            simIface->GetCurrentTimeForPartition(iface));
}

// /**
// FUNCTION :: InsertInRadioPtrList
// PURPOSE :: Associates a radio with an entity by inserting it in the
//            entity's radio hash.
// PARAMETERS ::
// + newRadio : VRLinkRadio* : Radio to insert.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::InsertInRadioPtrList(VRLinkRadio* newRadio)
{
    hash_map<unsigned short, VRLinkRadio*>::iterator radioPtrIter;

    radioPtrIter = radioPtrs.find(newRadio->GetRadioIndex());

    if (radioPtrIter == radioPtrs.end())
    {
        radioPtrs.insert(pair<unsigned short, VRLinkRadio*>
                                      (newRadio->GetRadioIndex(), newRadio));
    }
    else
    {
        VRLinkReportWarning("Radio already present in radioPtrs hash",
                            __FILE__, __LINE__);
    }
}

// /**
// FUNCTION :: ParseRecordFromFile
// PURPOSE :: To parse a record from the entities file.
// PARAMETERS ::
// + nodeInputStr : char* : Pointer to the record to be parsed.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ParseRecordFromFile(char* nodeInputStr)
{
    char* tempStr = nodeInputStr;
    char* p = NULL;
    char fieldStr[MAX_STRING_LENGTH] = "";

    p = simIface->GetDelimitedToken(fieldStr , tempStr, ",", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read markingData", nodeInputStr);
    markingData = fieldStr;

    p = simIface->GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read ForceID", nodeInputStr);
    forceId = fieldStr[0];

    p = simIface->GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read Country", nodeInputStr);
    nationality = fieldStr;

    p = simIface->GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read latitude", nodeInputStr);
    latLonAlt.setX(atof(fieldStr));

    p = simIface->GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read longitude", nodeInputStr);
    latLonAlt.setY(atof(fieldStr));

    p = simIface->GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read altitude", nodeInputStr);
    latLonAlt.setZ(atof(fieldStr));

    latLonAltScheduled = latLonAlt;

    VRLinkVerify(
         latLonAlt.x() >= -90.0 && latLonAlt.x() <= 90.0
         && latLonAlt.y() >= -180.0 && latLonAlt.y() <= 180.0,
         "Invalid geodetic coordinates", nodeInputStr);

    ConvertLatLonAltToGcc(latLonAlt, xyz);
    xyzScheduled = xyz;
}

// /**
// FUNCTION :: ScheduleChangeInterfaceState
// PURPOSE :: To start/end fault on an interface depending on the received
//            damage state.
// PARAMETERS ::
// + enableInterfaces : bool : TRUE when fault on an interface is to be
//                             ended; FALSE otherwise.
// + vrLink : VRLink* : VRLink pointer.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ScheduleChangeInterfaceState(
    bool enableInterfaces,
    VRLink*& vrLink)
{
    EXTERNAL_Interface* iface = vrLink->GetExtIfacePtr();

    VRLinkVerify(
        (!radioPtrs.empty()),
        "No radios found for entity",
        __FILE__, __LINE__);

    hash_map<unsigned short, VRLinkRadio*>::iterator radioPtrsIter;
    VRLinkRadio* radio = NULL;

    for (radioPtrsIter = radioPtrs.begin(); radioPtrsIter != radioPtrs.end();
                                                             radioPtrsIter++)
    {
        radio = radioPtrsIter->second;

        VRLinkVerify(
            (radio != NULL),
             "Radio pointer cannot be NULL",
             __FILE__, __LINE__);

        if (enableInterfaces && (radio->GetTxOperationalStatus() == DtOff))
        {
            continue;
        }

        Node* node = radio->GetNode();

        clocktype simTime = simIface->GetCurrentSimTime(node);
        clocktype delay;

        delay = MAX(simIface->QueryExternalTime(iface) - simTime, 0);

        if (delay == 0)
        {
            delay = simTime;
        }
        else
        {
            delay = simIface->QueryExternalTime(iface);
        }

        if (enableInterfaces)
        {
            simIface->EXTERNAL_ActivateNode(
                iface,
                node,
                simIface->GetExternalScheduleSafeType(),
                delay);
        }
        else
        {
            simIface->EXTERNAL_DeactivateNode(
                iface,
                node,
                simIface->GetExternalScheduleSafeType(),
                delay);
        }
    }
}

// /**
// FUNCTION :: ScheduleChangeMaxTxPower
// PURPOSE :: To schedule an event for changing the tx power of an entity
//            depending on the received damage state.
// PARAMETERS ::
// + node : Node* : Node pointer.
// + damageState : unsigned : reflected damage state.
// + delay : clocktype : Delay after which the is scheduled.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ScheduleChangeMaxTxPower(
    Node* node,
    unsigned damageState,
    clocktype delay)
{
    //Node here is first node on p0
    Message* msg = simIface->AllocateMessage(node,
                                 simIface->GetExternalLayerId(),
                                 simIface->GetVRLinkExternalInterfaceType(),
                                 simIface->GetMessageEventType(EXTERNAL_VRLINK_ChangeMaxTxPower));

    simIface->AllocateInfoInMsg(node,msg,sizeof(VRLinkChangeMaxTxPowerInfo));

    VRLinkChangeMaxTxPowerInfo& info
        = *((VRLinkChangeMaxTxPowerInfo*) simIface->ReturnInfoFromMsg(msg));

    info.entityPtr = this;
    info.damageState = damageState;
    simIface->SendMessageWithDelay(node, msg, delay);
}

// /**
// FUNCTION :: CbReflectAttributeValues
// PURPOSE :: Callback for reflecting the attribute updates.
// PARAMETERS ::
// + ent : DtReflectedEntity* : Entity of which the attributes are to be
//                              updated.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLinkEntity::CbReflectAttributeValues(
    DtReflectedEntity* ent,
    void* usr)
{
     VRLinkVerify(
         ent,
         "NULL reflected entity data received",
         __FILE__, __LINE__);

    VRLink* vrLink = NULL;
    VRLinkEntity* entity = NULL;

    VRLinkEntityCbUsrData* usrData = (VRLinkEntityCbUsrData*)usr;

    if (usrData)
    {
        usrData->GetData(&vrLink, &entity);
    }

    if ((entity) && (vrLink))
    {
        entity->ReflectAttributes(ent, vrLink);
    }
}

// /**
// FUNCTION :: ReflectAttributes
// PURPOSE :: Reflects new entity attributes (called by
//            CbReflectAttributeValues callback).
// PARAMETERS ::
// + ent : DtReflectedEntity* : Entity of which the attributes are to be
//                              updated.
// + vrLink : VRLink* : VRLink pointer.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ReflectAttributes(DtReflectedEntity* ent, VRLink* vrLink)
{
    DtEntityStateRepository* esr = ent->esr();
    bool schedulePossibleMobilityEvent = false;

    entityId = esr->entityId();
    ReflectDamageState(esr->damageState(), vrLink);

    ReflectVelocity(esr->velocity(), schedulePossibleMobilityEvent);

    ReflectWorldLocation(
        esr->location(),
        vrLink,
        schedulePossibleMobilityEvent);

    ReflectOrientation(esr->orientation(), schedulePossibleMobilityEvent);

    ReflectForceId(esr->forceId());

    ReflectDomainField(esr->entityType().domain());

    if (schedulePossibleMobilityEvent)
    {
        ScheduleMobilityEventIfNecessary(vrLink);
    }
}

// /**
// FUNCTION :: ScheduleMobilityEventIfNecessary
// PURPOSE :: Called to update the attributes of QualNet nodes based on the
//            attribute values received from an outside federate.
// PARAMETERS ::
// + vrLink : VRLink* : VRLink pointer.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ScheduleMobilityEventIfNecessary(VRLink* vrLink)
{
    clocktype mobilityEventTime = 0;
    clocktype simTime = 0;

    bool eventSch = vrLink->IsMobilityEventSchedulable(
                                this,
                                mobilityEventTime,
                                simTime);

    if (!eventSch)
    {        
        return;
    }

    bool scheduleWorldLocation = false;
    bool scheduleOrientation   = false;
    bool scheduleVelocity      = false;

    double xyzEpsilon = vrLink->GetXyzEpsilon();

    if (fabs(xyz.x() - xyzScheduled.x()) >= xyzEpsilon
        || fabs(xyz.y() - xyzScheduled.y()) >= xyzEpsilon
        || fabs(xyz.z() - xyzScheduled.z()) >= xyzEpsilon)
    {
        scheduleWorldLocation = true;
    }

    if (azimuth != azimuthScheduled
        || elevation != elevationScheduled)
    {
        scheduleOrientation = true;
    }
    if (currentVelocity != velocityScheduled)
    {
        scheduleVelocity = true;
    }

    if (!scheduleWorldLocation && !scheduleOrientation && !scheduleVelocity)
    {
        return;
    }

    DtVector entityXYZ;
    DtVector entityLatLonAlt;

    if (scheduleWorldLocation)
    {
        entityXYZ.setX(xyz.x());
        entityXYZ.setY(xyz.y());
        entityXYZ.setZ(xyz.z());

        entityLatLonAlt.setX(latLonAlt.x());
        entityLatLonAlt.setY(latLonAlt.y());
        entityLatLonAlt.setZ(latLonAlt.z());
    }
    else
    {
        entityXYZ.setX(xyzScheduled.x());
        entityXYZ.setY(xyzScheduled.y());
        entityXYZ.setZ(xyzScheduled.z());

        entityLatLonAlt.setX(latLonAltScheduled.x());
        entityLatLonAlt.setY(latLonAltScheduled.y());
        entityLatLonAlt.setZ(latLonAltScheduled.z());
    }

    Orientation orientation;

    if (scheduleOrientation)
    {
        orientation.azimuth   = azimuth;
        orientation.elevation = elevation;
    }
    else
    {
        orientation.azimuth   = azimuthScheduled;
        orientation.elevation = elevationScheduled;
    }

    double speed1;
    DtVector velocity1;

    if (scheduleVelocity)
    {
        velocity1 = currentVelocity;
    }
    else
    {
        velocity1 = velocityScheduled;
    }

    // Move entity (hierarchy).

    EXTERNAL_Interface* iface = vrLink->GetExtIfacePtr();

    if ((simIface->GetGuiOption(iface) && hierarchyIdExists)
         && (scheduleOrientation || scheduleWorldLocation))
    {
        Node* node = simIface->GetFirstNodeOnPartition(iface);

        Coordinates coordinates;
        coordinates.latlonalt.latitude  = entityLatLonAlt.x();
        coordinates.latlonalt.longitude = entityLatLonAlt.y();
        coordinates.latlonalt.altitude  = entityLatLonAlt.z();

        Message* msg = simIface->AllocateMessage(node,
                                     simIface->GetExternalLayerId(),
                                     simIface->GetVRLinkExternalInterfaceType(),
                                     simIface->GetMessageEventType(EXTERNAL_VRLINK_HierarchyMobility));

        simIface->AllocateInfoInMsg(node,
                                    msg,
                                    sizeof(VRLinkHierarchyMobilityInfo));
        VRLinkHierarchyMobilityInfo& info
            = *((VRLinkHierarchyMobilityInfo*)
                                           simIface->ReturnInfoFromMsg(msg));
        info.hierarchyId = hierarchyId;
        info.coordinates = coordinates;
        info.orientation = orientation;

        // Execute the hierarchy move one nanosecond before the node move
        // time.  This is a workaround for a GUI issue where the screen will
        // get all messed up if the GUI command to move the node is sent
        // before the GUI command to move the hierarchy -- even if both GUI
        // commands are sent with the same timestamp.

        clocktype delay = (mobilityEventTime - 1) - simTime ;

         VRLinkVerify(
             (delay >= 0),
             "Message send delay cannot be negative",
             __FILE__, __LINE__);

        simIface->SendMessageWithDelay(node, msg, delay);
    }

    // Move radios (nodes).

    hash_map<unsigned short, VRLinkRadio*>::iterator radioPtrsIter;

    for (radioPtrsIter = radioPtrs.begin(); radioPtrsIter != radioPtrs.end();
                                                             radioPtrsIter++)
    {
        VRLinkRadio* radio = radioPtrsIter->second;

        // Schedule QualNet node movement.

        Coordinates coordinates;      
        DtVector radioLatLonAltInDeg;

        if (scheduleWorldLocation)
        {
            ConvertGccToLatLonAlt(radio->GetRelativePosition() + entityXYZ, radioLatLonAltInDeg);

            VRLinkVerify(
                (radioLatLonAltInDeg.x() >= -90.0 &&
                radioLatLonAltInDeg.x() <= 90.0 &&
                radioLatLonAltInDeg.y() >= -180.0 &&
                radioLatLonAltInDeg.y() <= 180.0),
                "Invalid LatLonAlt coordinates",
                __FILE__, __LINE__);

            coordinates.latlonalt.latitude  = radioLatLonAltInDeg.x();
            coordinates.latlonalt.longitude = radioLatLonAltInDeg.y();
            coordinates.latlonalt.altitude  = radioLatLonAltInDeg.z();
        }
        else
        {
            radioLatLonAltInDeg = radio->GetLatLonAltSch();

            coordinates.latlonalt.latitude  = radioLatLonAltInDeg.x();
            coordinates.latlonalt.longitude = radioLatLonAltInDeg.y();
            coordinates.latlonalt.altitude  = radioLatLonAltInDeg.z();
        }
        // This ia parallel safe (though timing of position
        // update for nodes of remote partitions is best-effort)

        //the velocity for an entity is always meters per second, however
        //the qualnet mobility api expects you to put velocity into the
        //correct units currently in use.  At this point you are using LatLon, 
        //so you need to convert veloctity to degrees
        
        //First add the last known geocentric position with the geocentric velocity
        //Then convert that to geodetic (lat lon) which will give you one second later
        //Finally subtract previous lat lon from that value to get lat lon velocity                       
        DtVector currLatLon = DtVector(coordinates.latlonalt.latitude,
            coordinates.latlonalt.longitude,
            coordinates.latlonalt.altitude);
        DtVector currGeocentric;

        ConvertLatLonAltToGcc(currLatLon, currGeocentric);

        DtGeodeticCoord newlatlonalt;
        ConvertGccToLatLonAlt(currGeocentric + velocity1, newlatlonalt);

        DtVector latlonRate = newlatlonalt - currLatLon;

        simIface->ChangeExternalNodePositionOrientationAndVelocityAtTime(
            iface,
            radio->GetNode(),
            mobilityEventTime,
            coordinates.latlonalt.latitude,
            coordinates.latlonalt.longitude,
            coordinates.latlonalt.altitude,
            orientation.azimuth,
            orientation.elevation,
            latlonRate.x(),
            latlonRate.y(),
            latlonRate.z());

        if (scheduleWorldLocation)
        {
            radio->SetLatLonAltScheduled(radioLatLonAltInDeg);
        }
    }//for//

    if (scheduleWorldLocation)
    {
        xyzScheduled.setX(xyz.x());
        xyzScheduled.setY(xyz.y());
        xyzScheduled.setZ(xyz.z());

        latLonAltScheduled.setX(latLonAlt.x());
        latLonAltScheduled.setY(latLonAlt.y());
        latLonAltScheduled.setZ(latLonAlt.z());
    }

    if (scheduleOrientation)
    {
        azimuthScheduled   = azimuth;
        elevationScheduled = elevation;
    }

    if (scheduleVelocity)
    {
        velocityScheduled = currentVelocity;
    }

#if DtHLA
    if (!vrLink->GetNewEventScheduled())
    {
        vrLink->EnableNewEventScheduled();
    }
#endif

    lastScheduledMobilityEventTime = mobilityEventTime;

    if (vrLink->GetDebug2() && scheduleWorldLocation)
    {
        double dbl_mobilityEventTime
                     = simIface->ConvertClocktypeToDouble(mobilityEventTime);

        cout << "VRLINK: Moving (" << markingData << ")"
                << " to (" << entityLatLonAlt.x() << "," <<
                entityLatLonAlt.y() << ","<< entityLatLonAlt.z()
                << "), time " << dbl_mobilityEventTime << endl;
    }
    

    if (vrLink->GetDebugPrintMobility() && scheduleWorldLocation)
    {
        double entityAltDisplayed = entityLatLonAlt.z();

        if (entityLatLonAlt.z() < 0 && entityLatLonAlt.z() > -0.999999)
        {
            entityAltDisplayed = 0.0;
        }

        double dbl_mobilityEventTime
                     = simIface->ConvertClocktypeToDouble(mobilityEventTime);

        cout<<"VRLINK: Moving "<<markingData<<" to ("<<entityLatLonAlt.x()<<","
            <<entityLatLonAlt.y()<<","<<entityAltDisplayed<<") time "
            << dbl_mobilityEventTime<<"\n";
    }
    
    if ((vrLink->GetDebug2() || vrLink->GetDebugPrintMobility()) && scheduleOrientation)
    {
        double dbl_mobilityEventTime
                     = simIface->ConvertClocktypeToDouble(mobilityEventTime);

        cout << "VRLINK: Changing Orientation (" << markingData << ")"
                << " to (" << orientation.azimuth << "," << orientation.elevation
                << "), time " << dbl_mobilityEventTime << endl;
    }

    if ((vrLink->GetDebug2() || vrLink->GetDebugPrintMobility()) && scheduleVelocity)
    {
        double dbl_mobilityEventTime
                     = simIface->ConvertClocktypeToDouble(mobilityEventTime);

        cout << "VRLINK: Changing Velocity (" << markingData << ")"
            << " to (X:" << velocity1.x() << " Y:" << velocity1.y() << " Z:" <<
            velocity1.z() << "), time " << dbl_mobilityEventTime << endl;
    }
}


// /**
// FUNCTION :: Remove
// PURPOSE :: To set the entity damage state as destroyed and the tx
//            operational status of the associated radios as off.
// PARAMETERS :: None.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::Remove()
{
    hash_map<unsigned short, VRLinkRadio*>::iterator radioPtrsIter;

    damageState = DtDamageDestroyed;

    for (radioPtrsIter = radioPtrs.begin(); radioPtrsIter != radioPtrs.end();
                                                             radioPtrsIter++)
    {
        radioPtrsIter->second->Remove();
    }
}

// /**
// FUNCTION :: ~VRLinkEntity
// PURPOSE :: Destructor of Class VRLinkEntity.
// PARAMETERS :: None.
// **/
VRLinkEntity::~VRLinkEntity()
{
    radioPtrs.erase(radioPtrs.begin(), radioPtrs.end());
}

// /**
// FUNCTION :: ReflectDamageState
// PURPOSE :: To update the entity damage state based on the value received
//            from an outside federate.
// PARAMETERS ::
// + reflDamageState : DtDamageState : Received damage state.
// + vrLink : VRLink* : VRLink pointer.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ReflectDamageState(
    DtDamageState reflDamageState,
    VRLink* vrLink)
{
#if 0
#if DtHLA
    //COMMENTED OUT NAWC GATEWAY CODE, currently unsupported in vrlink
    VRLinkHLA* hla = (VRLinkHLA*)vrLink;

    if (hla->GetNawcGatewayCompatibility())
    {
        // NAWC Gateway copies the Entity State PDU Entity Appearance
        //Record field exactly. This field must be parsed to the RPR-FOM
        //DamageState parameter.

        unsigned int reflDamageSt = reflDamageState;

        reflDamageSt &= 24;
        reflDamageSt >>= 3;
        reflDamageState = DtDamageState(reflDamageSt);
    }
#endif
#endif

    VRLinkVerify(reflDamageState >= DtDamageNone
              && reflDamageState <= DtDamageDestroyed,
              "Unexpected attribute value",
              __FILE__, __LINE__);

    VRLinkVerify(
        (damageState >= DtDamageNone) && (damageState <= DtDamageDestroyed),
        "Invalid damage state received",
         __FILE__, __LINE__);

    if (damageState == reflDamageState)
    {
        return;
    }

    VRLinkInterfaceType vrlType = vrLink->GetType();

    if (vrlType == VRLINK_TYPE_HLA13 ||
        vrlType == VRLINK_TYPE_HLA1516)
    {
        cout<<"VRLINK: ("<<entityId.string()
                                      <<") "<<markingData<<" DamageState = ";
    }
    else if (vrLink->GetDebugPrintDamage())
    {
        cout<<"VRLINK: "<<markingData<<" ("<<entityId.string() << ") Damage = ";
    }

    if ((vrlType == VRLINK_TYPE_HLA13 || vrlType == VRLINK_TYPE_HLA1516)
        || vrLink->GetDebugPrintDamage())
    {
        switch (reflDamageState)
        {
            case DtDamageNone:
                cout << "NoDamage";
                break;
            case DtDamageSlight:
                cout << "SlightDamage";
                break;
            case DtDamageModerate:
                cout << "ModerateDamage";
                break;
            case DtDamageDestroyed:
                cout << "Destroyed";
                break;
            default:
                VRLinkReportError(
                    "Invalid damage state encountered",
                    __FILE__, __LINE__);
        }
    }

    cout << endl;

    if (!radioPtrs.empty())
    {
        hash_map<unsigned short, VRLinkRadio*>::iterator radioPtrsIter;

        radioPtrsIter = radioPtrs.begin();

        VRLinkVerify(
            radioPtrsIter->second,
            "Radio not found",
            __FILE__, __LINE__);

        // Get node pointer for first node on p0 (used for scheduling this event) and delay.

        EXTERNAL_Interface* iface = vrLink->GetExtIfacePtr();
        Node* node = simIface->GetFirstNodeOnPartition(iface);

        clocktype simTime = simIface->GetCurrentSimTime(node);
        clocktype delay = MAX(simIface->QueryExternalTime(iface)-simTime, 0);

        ////// Turn off/on interfaces.


        if (damageState != DtDamageDestroyed
            && reflDamageState == DtDamageDestroyed)
        {
            ScheduleChangeInterfaceState(false, vrLink);
        }
        else
        if (damageState == DtDamageDestroyed
            && reflDamageState != DtDamageDestroyed)
        {
            ScheduleChangeInterfaceState(true, vrLink);
        }

        // Schedule check for reducing TX power.

        if (damageState < DtDamageSlight
            && reflDamageState == DtDamageSlight)
        {
            // Less-damaged state -> SlightDamage.
            ScheduleChangeMaxTxPower(node, DtDamageSlight, delay);
        }
        else
        if (damageState < DtDamageModerate
            && reflDamageState == DtDamageModerate)
        {
            // Less-damaged state -> ModerateDamage.
            ScheduleChangeMaxTxPower(node, DtDamageModerate, delay);
        }
        else
        if (damageState < DtDamageDestroyed
            && reflDamageState == DtDamageDestroyed)
        {
            // Less-damaged state -> Destroyed.
            ScheduleChangeMaxTxPower(node, DtDamageDestroyed, delay);
        }
        else
        if ((damageState != DtDamageNone)
             && (reflDamageState == DtDamageNone))
        {
            // More-damaged state -> NoDamage.
            ScheduleChangeMaxTxPower(node, DtDamageNone, delay);
        }
        else
        if (damageState > reflDamageState)
        {
            // More-damaged state -> Less-damaged state (but not NoDamage).
            // Don't change max TX power.
        }
        else
        {
            VRLinkReportError(
                "Invalid damage state encountered",
                __FILE__, __LINE__);
        }
    }//if//

    damageState = reflDamageState;
}

// /**
// FUNCTION :: ReflectWorldLocation
// PURPOSE :: To update the entity location based on the value received from
//            an outside federate.
// PARAMETERS ::
// + reflWorldLocation : const DtVector& : Received entity location in (GCC).
// + vrLink : VRLink* : VRLink pointer.
// + schedulePossibleMobilityEvent : bool& : TRUE, if entity location has
//                                           changed; FALSE otherwise.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ReflectWorldLocation(
    const DtVector& reflWorldLocation,
    VRLink* vrLink,
    bool& schedulePossibleMobilityEvent)
{
    DtVector reflLatLonAltInDeg;
    bool isReflDataInValidRange;

    if (xyz == reflWorldLocation)
    {
        return;
    }

    ConvertGccToLatLonAlt(reflWorldLocation, reflLatLonAltInDeg);

    VRLinkVerify(
        ((reflLatLonAltInDeg.x() >= -90.0) &&
         (reflLatLonAltInDeg.x() <= 90.0)),
        "Reflected coordinates out of bound of specified terrain",
        __FILE__, __LINE__);

     VRLinkVerify(
         ((reflLatLonAltInDeg.y() >= -180.0) &&
          (reflLatLonAltInDeg.y() <= 180.0)),
         "Reflected coordinates out of bound of specified terrain",
         __FILE__, __LINE__);

    isReflDataInValidRange = vrLink->IsTerrainDataValid(
                                         reflLatLonAltInDeg.x(),
                                         reflLatLonAltInDeg.y());

    if (!isReflDataInValidRange)
    {
        VRLinkReportWarning("Received world location is outside scenario dimensions",
            __FILE__, __LINE__);
        return;
    }

    xyz = reflWorldLocation;
    latLonAlt = reflLatLonAltInDeg;
    schedulePossibleMobilityEvent = true;
}

// /**
// FUNCTION :: ReflectVelocity
// PURPOSE :: To update the entity velocity based on the value received from
//            an outside federate.
// PARAMETERS ::
// + reflSpeed : const DtVector& : Received entity velocity (in GCC).
// + schedulePossibleMobilityEvent : bool& : TRUE, if entity location has
//                                           changed; FALSE otherwise.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ReflectVelocity(
    const DtVector& reflVel,
    bool& schedulePossibleMobilityEvent)
{
    if (currentVelocity == reflVel)
    {
        return;
    }

    speed = pow(pow((double) reflVel.x(), 2)
                + pow((double) reflVel.y(), 2)
                + pow((double) reflVel.z(), 2),
                0.5);

    VRLinkVerify(
        (speed >= 0.0),
        "Negative reflected speed received",
        __FILE__, __LINE__);

    schedulePossibleMobilityEvent = true;
    currentVelocity = reflVel;
}

// /**
// FUNCTION :: ReflectOrientation
// PURPOSE :: To update the entity orientation based on the value received
//            from an outside federate.
// PARAMETERS ::
// + reflOrientation : const DtTaitBryan& : Received entity orientation.
// + schedulePossibleMobilityEvent : bool& : TRUE, if entity location has
//                                           changed; FALSE otherwise.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ReflectOrientation(
    const DtTaitBryan& reflOrientation,
    bool& schedulePossibleMobilityEvent)
{
    if (currentOrientation == reflOrientation)
    {
        return;
    }

    ConvertReflOrientationToQualNetOrientation(reflOrientation);

    schedulePossibleMobilityEvent = true;
    currentOrientation = reflOrientation;
}

// /**
// FUNCTION :: ReflectForceId
// PURPOSE :: To update the entity force id based on the value received
//            from an outside federate.
// PARAMETERS reflForceId
// + reflForceId : const DtForceType& : Received entity force id.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ReflectForceId(
   const DtForceType & reflForceId)
{   
        switch (reflForceId)
        {
            case DtForceFriendly:
            {
                forceId ='F';  
                forceIdByte = reflForceId;                 
                break;
            }
            case DtForceOpposing:
            {
                forceId = 'O';
                forceIdByte = reflForceId;
                break;
            }
            case DtForceNeutral :
            default: //DtForceOther
            {
                // Treat other ForceIDs as neutral forces.

                forceId = 'N';
                forceIdByte = DtForceNeutral;
                break;
            }
        }
}

// /**
// FUNCTION :: ReflectDomainField
// PURPOSE :: To update the entity force id based on the value received
//            from an outside federate.
// PARAMETERS reflForceId
// + reflDomain : const int : Received domain value.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ReflectDomainField(
    const int &reflDomain)
{
        switch (reflDomain)
        {
            case DOMAIN_AIR:
            case DOMAIN_SPACE:
            {
                domain = "air";
                domainInt = DOMAIN_AIR; // reflDomain;
                break;
            }
            default :
            {
                domain = "surface";
                domainInt = DOMAIN_SURFACE; //
                break;
            }
        }       
}

// /**
// FUNCTION :: Rint
// PURPOSE :: Round off to integral value in floating-point format.
// PARAMETERS ::
// + a : double : Number to be rounded off.
// RETURN :: double : Rounded off number.
// **/
double VRLinkEntity::Rint(double a)
{
#ifdef _WIN32
    // Always round up when the fractional value is exactly 0.5.
    // (Usually such functions round up only half the time.)

    return floor(a + 0.5);
#else /* _WIN32 */
    return rint(a);
#endif /* _WIN32 */
}

// /**
// FUNCTION :: ConvertReflOrientationToQualNetOrientation
// PURPOSE :: Converts a DIS / RPR-FOM 1.0 orientation into QualNet azimuth
//            and elevation angles. When an entity is located exactly at the
//            north pole, this function will return an azimuth of 0 (facing
//            north) and the entity is pointing towards 180 degrees
//            longitude. When it is located exactly at the south pole, this
//            function will return an azimuth of 0 (facing north) and the
//            entity is pointing towards 0 degrees longitude. This function
//            has not been optimized at all, so the calculations are more
//            readily understood, e.g., the phi angle doesn't affect the
//            results; some vector components end up being multipled by 0,
//            etc.
// PARAMETERS ::
// + orientation : const DtTaitBryan& : Received entity orientation.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ConvertReflOrientationToQualNetOrientation(
    const DtTaitBryan& orientation)
{
    VRLinkVerify(
        ((latLonAlt.x() >= -90.0) && (latLonAlt.x() <= 90.0)),
        "LatLonAlt coordinates out of bound of specified terrain",
        __FILE__, __LINE__);

    VRLinkVerify(
        ((latLonAlt.y() >= -180.0) && (latLonAlt.y() <= 180.0)),
        "LatLonAlt coordinates out of bound of specified terrain",
        __FILE__, __LINE__);

    // Introduction: --------------------------------------------------------

    // There are two coordinate systems considered:
    //
    // (1) the GCC coordinate system, and
    // (2) the entity coordinate system.
    //
    // Both are right-handed coordinate systems.
    //
    // The GCC coordinate system has well-defined axes.
    // In the entity coordinate system, the x-axis points in the direction
    // the entity is facing (the entity orientation).
    //
    // psi, theta, and phi are the angles by which one rotates the GCC axes
    // so that they match in direction with the entity axes.
    //
    // Start with the GCC axes, and rotate them in the following order:
    //
    // (1) psi, a right-handed rotation about the z-axis
    // (2) theta, a right-handed rotation about the new y-axis
    // (3) phi, a right-handed rotation about the new x-axis

    double psiRadians = orientation.psi();
    double thetaRadians = orientation.theta();
    double phiRadians = orientation.phi();

    // Convert latitude and longitude into a unit vector.
    // If one imagines the earth as a unit sphere, the vector will point
    // to the location of the entity on the surface of the sphere.

    double latRadians = DtDeg2Rad(latLonAlt.x());
    double lonRadians = DtDeg2Rad(latLonAlt.y());

    double entityLocationX = cos(lonRadians) * cos(latRadians);
    double entityLocationY = sin(lonRadians) * cos(latRadians);
    double entityLocationZ = sin(latRadians);

    // Discussion of basic theory: ------------------------------------------

    // Start with a point b in the initial coordinate system. b is
    // represented as a vector.
    //
    // Rotate the axes of the initial coordinate system by angle theta to
    // obtain a new coordinate system.
    //
    // Representation of point b in the new coordinate system is given by:
    //
    // c = ab
    //
    // where a is a rotation matrix:
    //
    // a = [ cos(theta)        sin(theta)
    //       -sin(theta)       cos(theta) ]
    //
    // and c is the same point, but in the new coordinate system.
    //
    // Note that the coordinate system changes; the point itself does not
    // move.
    // Also, matrix a is for rotating the axes; the matrix is different when
    // rotating the vector while not changing the axes.
    //
    // Applying this discussion to three dimensions, and using psi, theta,
    // and phi as described previously, three rotation matrices can be
    // created:
    //
    // Rx =
    // [ 1           0           0
    //   0           cos(phi)    sin(phi)
    //   0           -sin(phi)   cos(phi) ]
    //
    // Ry =
    // [ cos(theta)  0           -sin(theta)
    //   0           1           0
    //   sin(theta)  0            cos(theta) ]
    //
    // Rz =
    // [ cos(psi)    sin(psi)    0
    //   -sin(psi)   cos(psi)    0
    //   0           0           1 ]
    //
    // where
    //
    // c = ab
    // a = (Rx)(Ry)(Rz)
    //
    // b is the point as represented in the GCC coordinate system;
    // c is the point as represented in the entity coordinate system.
    //
    // Note that matrix multiplication is not commutative, so the order of
    // the factors in a is important (rotate by Rz first, so it's on the
    // right side).

    // Determine elevation angle: -------------------------------------------

    // In the computation of the elevation angle below, the change is in the
    // opposite direction, from the entity coordinate system to the GCC
    // system.
    // So, for:
    //
    // c = ab
    //
    // Vector b represents the entity orientation as expressed in the entity
    // coordinate system.
    // Vector c represents the entity orientation as expressed in the GCC
    // coordinate system.
    //
    // It turns out that:
    //
    // a = (Rz)'(Ry)'(Rx)'
    //
    // where Rx, Ry, and Rz are given earlier, and the ' symbol indicates the
    // transpose of each matrix.
    //
    // The ordering of the matrices is reversed, since one is going from the
    // entity coordinate system to the GCC system.  The negative of psi,
    // theta, and phi angles are used, and the transposed matrices end up
    // being the correct ones.

    double a11 = cos(psiRadians) * cos(thetaRadians);
    double a12 = -sin(psiRadians) * cos(phiRadians)
                 + cos(psiRadians) * sin(thetaRadians) * sin(phiRadians);
    double a13 = -sin(psiRadians) * -sin(phiRadians)
                 + cos(psiRadians) * sin(thetaRadians) * cos(phiRadians);

    double a21 = sin(psiRadians) * cos(thetaRadians);
    double a22 = cos(psiRadians) * cos(phiRadians)
                 + sin(psiRadians) * sin(thetaRadians) * sin(phiRadians);
    double a23 = cos(psiRadians) * -sin(phiRadians)
                 + sin(psiRadians) * sin(thetaRadians) * cos(phiRadians);

    double a31 = -sin(thetaRadians);
    double a32 = cos(thetaRadians) * sin(phiRadians);
    double a33 = cos(thetaRadians) * cos(phiRadians);

    // Vector b is chosen such that it is the unit vector pointing along the
    // positive x-axis of the entity coordinate system. I.e., vector b points
    // in the same direction the entity is facing.

    double b1 = 1.0;
    double b2 = 0.0;
    double b3 = 0.0;

    // The values below are the components of vector c, which represent the
    // entity orientation in the GCC coordinate system.

    double entityOrientationX = a11 * b1 + a12 * b2 + a13 * b3;
    double entityOrientationY = a21 * b1 + a22 * b2 + a23 * b3;
    double entityOrientationZ = a31 * b1 + a32 * b2 + a33 * b3;

    // One now has two vectors:
    //
    // (1) an entity-position vector, and
    // (2) an entity-orientation vector.
    //
    // Note that the position vector is normal to the sphere at the point
    // where the entity is located on the sphere.
    //
    // One can determine the angle between the two vectors using dot product
    // formulas.  The computed angle is the deflection from the normal.

    double dotProduct
        = entityLocationX * entityOrientationX
          + entityLocationY * entityOrientationY
          + entityLocationZ * entityOrientationZ;

    double entityLocationMagnitude
        = sqrt(pow(entityLocationX, 2)
               + pow(entityLocationY, 2)
               + pow(entityLocationZ, 2));

    double entityOrientationMagnitude
        = sqrt(pow(entityOrientationX, 2)
               + pow(entityOrientationY, 2)
               + pow(entityOrientationZ, 2));

    double deflectionFromNormalToSphereRadians
        = acos(dotProduct / (entityLocationMagnitude
                             * entityOrientationMagnitude));

    // The elevation angle is 90 degrees minus the angle for deflection from
    // normal.  (Note that the elevation angle can range from -90 to +90
    // degrees.)

    double elevationRadians
        = (M_PI / 2.0) - deflectionFromNormalToSphereRadians;

    elevation = (short) Rint(DtRad2Deg(elevationRadians));

    VRLinkVerify(
        ((elevation >= -90) && (elevation <= 90)),
        "Elevation value not valid",
        __FILE__, __LINE__);

    // Determine azimuth angle: ---------------------------------------------

    // To determine the azimuth angle, for:
    //
    // c = ab
    //
    // b is the entity orientation as represented in the GCC coordinate
    // system.
    //
    // c is the entity orientation as expressed in a new coordinate system.
    // This is a coordinate system where the yz plane is tangent to the
    // sphere at the point on the sphere where the entity is located.
    // The z-axis points towards true north; the y-axis points towards east;
    // the x-axis points in the direction of the normal to the sphere.
    //
    // The rotation matrix turns is then:
    //
    // a = (Ry)(Rz)
    //
    // where longitude is used for Rz and the negative of latitude is used
    // for Ry (since right-handed rotations of the y-axis are positive).

    a11 = cos(-latRadians) * cos(lonRadians);
    a12 = cos(-latRadians) * sin(lonRadians);
    a13 = -sin(-latRadians);

    a21 = -sin(lonRadians);
    a22 = cos(lonRadians);
    a23 = 0;

    a31 = sin(-latRadians) * cos(lonRadians);
    a32 = sin(-latRadians) * sin(lonRadians);
    a33 = cos(-latRadians);

    b1 = entityOrientationX;
    b2 = entityOrientationY;
    b3 = entityOrientationZ;

    // Variable unused.
    //double c1 = a11 * b1 + a12 * b2 + a13 * b3;

    double c2 = a21 * b1 + a22 * b2 + a23 * b3;
    double c3 = a31 * b1 + a32 * b2 + a33 * b3;

    // To determine azimuth, project c against the yz plane(the plane tangent
    // to the sphere at the entity location), creating a new vector without
    // an x component.
    //
    // Determine the angle between this vector and the unit vector pointing
    // north, using dot-product formulas.

    double vectorInTangentPlaneX = 0;
    double vectorInTangentPlaneY = c2;
    double vectorInTangentPlaneZ = c3;

    double northVectorInTangentPlaneX = 0;
    double northVectorInTangentPlaneY = 0;
    double northVectorInTangentPlaneZ = 1;

    dotProduct
        = vectorInTangentPlaneX * northVectorInTangentPlaneX
          + vectorInTangentPlaneY * northVectorInTangentPlaneY
          + vectorInTangentPlaneZ * northVectorInTangentPlaneZ;

    double vectorInTangentPlaneMagnitude
        = sqrt(pow(vectorInTangentPlaneX, 2)
               + pow(vectorInTangentPlaneY, 2)
               + pow(vectorInTangentPlaneZ, 2));

    double northVectorInTangentPlaneMagnitude
        = sqrt(pow(northVectorInTangentPlaneX, 2)
               + pow(northVectorInTangentPlaneY, 2)
               + pow(northVectorInTangentPlaneZ, 2));

    double azimuthRadians
        = acos(dotProduct / (vectorInTangentPlaneMagnitude
                             * northVectorInTangentPlaneMagnitude));

    // Handle azimuth values between 180 and 360 degrees.

    if (vectorInTangentPlaneY < 0.0)
    {
        azimuthRadians = (2.0 * M_PI) - azimuthRadians;
    }

    azimuth = (short) Rint(DtRad2Deg(azimuthRadians));

    if (azimuth == 360)
    {
        azimuth = 0;
    }

    VRLinkVerify(
        ((azimuth >= 0) && (azimuth <= 359)),
        "Azimuth value not valid",
        __FILE__, __LINE__);
}

// /**
// FUNCTION :: GetRadioPtr
// PURPOSE :: Returns the radio pointer corresponding to the radio index
//            passed in the function.
// PARAMETERS ::
// + radioIndex : unsigned short : Index of the radio corresponding to which
//                                 the radio pointer is to be returned.
// RETURN :: VRLinkRadio* : Radio pointer.
// **/
VRLinkRadio* VRLinkEntity::GetRadioPtr(unsigned short radioIndex)
{
    hash_map<unsigned short, VRLinkRadio*>::iterator radioPtrsIter;

    radioPtrsIter = radioPtrs.find(radioIndex);

    if (radioPtrsIter != radioPtrs.end())
    {
        return radioPtrsIter->second;
    }

    return NULL;
}

// /**
// FUNCTION :: IsIdenticalEntityId
// PURPOSE :: Compares two entities for equality.
// PARAMETERS ::
// + otherEntity : const VRLinkEntity** : The entity to be compared with.
// RETURN :: bool : TRUE if entities are identical; FALSE otherwise.
// **/
bool VRLinkEntity::IsIdenticalEntityId(const VRLinkEntity** otherEntity)const
{
    return (entityId == (*otherEntity)->entityId);
}

// /**
// FUNCTION :: DisplayData
// PURPOSE :: To print the data for an entity.
// PARAMETERS :: None.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::DisplayData()
{
    hash_map<unsigned short, VRLinkRadio*>::iterator radioPtrsIter;

    cout<<"\nEntityId : "<<entityId;
    cout<<"\nMarking Data : "<<markingData;
    cout<<"\nForceId : "<<forceId;
    cout<<"\nDomain : "<<domain;
    cout<<"\nNationality : "<<nationality;
    cout<<"\nDamageState : "<<damageState;
    cout<<"\nVelocity : "<<speed;
    cout<<"\nHierarchyId Exists : "<<hierarchyIdExists;
    cout<<"\nHierarchyId : "<<hierarchyId;
    cout<<"\nRadio(s) associated (NodeId's / Radio index) : ";


    for (radioPtrsIter = radioPtrs.begin(); radioPtrsIter != radioPtrs.end();
                                                             radioPtrsIter++)
    {
        cout<<"\n"<<radioPtrsIter->first<<" / ";
    }
}

// /**
// FUNCTION :: AssignEntityIdToNodesNotInRadiosFile
// PURPOSE :: Assigns dummy entity ids to nodes not in the .vrlink-radios file
// PARAMETERS :: None.
// RETURN :: void : NULL.
// **/
void VRLink::AssignEntityIdToNodesNotInRadiosFile()
{
    int i = 0;
    Node* currNode = NULL;
    VRLinkData* vrlinkData = NULL;
    hash_map <NodeId, VRLinkRadio*>::iterator radiosIter;
    NodePointerCollection* allNodes = simIface->GetAllNodes(iface);

    for (i = 0; i < allNodes->size(); ++i)
    {
        currNode = allNodes->at(i);
        NodeId id = simIface->GetNodeId(currNode);
        radiosIter = radios.find(id);

        if (radiosIter == radios.end())
        {
            DtEntityIdentifier* entityId = new DtEntityIdentifier(0, 0, 0);
            entityId->setSite(siteId);
            entityId->setHost(0xffff & applicationId +
                                                   (id >> 16));

            entityId->setEntityNum(0xffff & id);
            nodeIdToEntityIdHash[id] = *entityId;

            EntityIdentifierKey* entityIdKey =
                                      GetEntityIdKeyFromDtEntityId(entityId);

            entityIdToNodeIdMap[*entityIdKey].push_back(id);

            //initialize nodeIdToPerNodeDataHash for this node
            vrlinkData = new VRLinkData;
            
            vrlinkData->SetNextMsgId(0);

            nodeIdToPerNodeDataHash.insert(pair<NodeId, VRLinkData*>
                                          (id,vrlinkData));

            VRLinkEntity* entity = new VRLinkEntity;
            entity->SetEntityId(*entityId);

            VRLinkRadio* newRadio = new VRLinkRadio();
            vrlinkData->SetRadioPtr(newRadio);
            newRadio->SetEntityId(*entityId);
            newRadio->SetEntityPtr(entity);
            newRadio->SetNode(currNode);
            entity->InsertInRadioPtrList(newRadio);
            entityIdToEntityData[*entityIdKey] = entity;
            
            EntityIdKeyRadioIndex entityRadioIndex(*entityIdKey, 0);
            radioIdToRadioData[entityRadioIndex] = newRadio;
        }
    }
}

// /**
// FUNCTION :: SetMinAndMaxLatLon
// PURPOSE :: Sets the upper and lower limit of the latitude and longitude.
// PARAMETERS ::
// RETURN :: void : NULL.
// **/
void VRLink::SetMinAndMaxLatLon()
{
    simIface->GetMinAndMaxLatLon(iface, minLat, maxLat, minLon, maxLon);
}

// /**
// FUNCTION :: IsTerrainDataValid
// PURPOSE :: Tests the passed latitude and longitude to check whether they
//            lie within a valid range.
// PARAMETERS ::
// + lat : double : Latitude to be validated.
// + lon : double : Longitude to be validated.
// RETURN :: bool : TRUE, if the passed parameters lie within a valid range,
//                  FALSE otherwise.
// **/
bool VRLink::IsTerrainDataValid(double lat, double lon)
{
    if ((lat < minLat) || (lat > maxLat) || (lon < minLon) || (lon > maxLon))
    {
        return false;
    }
    return true;
}

// /**
// FUNCTION :: ChangeMaxTxPowerOfRadios
// PURPOSE :: To change the tx power of all the radios associated with an
//            entity.
// PARAMETERS ::
// + restoreMaxTxPower : bool : TRUE when radio's transmit power is to be set
//                              to maximum; FALSE otherwise.
// + probability : double : Probability of changing the transmit power of the
//                          radio when restoreMaxTxPower is set to FALSE.
// + maxFraction : double : Fraction of change in the transmit power of the
//                          radio when restoreMaxTxPower is set to FALSE.
// + vrLink : VRLink* : VRLink pointer.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ChangeMaxTxPowerOfRadios(
    bool restoreMaxTxPower,
    double probability,
    double maxFraction,
    VRLink* vrLink)
{
    hash_map<unsigned short, VRLinkRadio*>::iterator radioPtrsIter;

     VRLinkVerify(
         (!radioPtrs.empty()),
         "No radios found for entity",
         __FILE__, __LINE__);

    for (radioPtrsIter = radioPtrs.begin();
                          radioPtrsIter != radioPtrs.end(); radioPtrsIter++)
    {
        VRLinkRadio* radio = radioPtrsIter->second;

        VRLinkVerify(
            (radio != NULL),
            "Radio pointer cannot be NULL",
            __FILE__, __LINE__);

        radio->ChangeMaxTxPower(
                   restoreMaxTxPower,
                   probability,
                   maxFraction,
                   vrLink);
    }//for//
}

// /**
// FUNCTION :: VRLinkRadio
// PURPOSE :: Initializing function for VRLink radio.
// PARAMETERS :: None.
// **/
VRLinkRadio::VRLinkRadio()
{
    entityPtr = NULL;
    networkPtr = NULL;
    defaultDstRadioPtr = NULL;    

    entityId = DtEntityIdentifier(0, 0, 0);
    txOperationalStatus = DtRadioOnNotTransmitting;
    radioIndex = 0;

    isLink11Node = false;
    isLink16Node = false;
}

// /**
// FUNCTION :: IsLink16Node
// PURPOSE :: Checks if node is Link16
// PARAMETERS :: None.
// RETURNS :: bool
// **/
bool VRLinkRadio::IsLink16Node()
{
    return isLink16Node;
}

// /**
// FUNCTION :: IsLink11Node
// PURPOSE :: Checks if node is Link11
// PARAMETERS :: None.
// RETURNS :: bool
// **/
bool VRLinkRadio::IsLink11Node()
{
    return isLink11Node;
}

// /**
// FUNCTION :: SetAsLink11Node
// PURPOSE :: Enables Link11 flag for node
// PARAMETERS :: None.
// RETURNS :: None
// **/
void VRLinkRadio::SetAsLink11Node()
{
    isLink11Node = true;
}

// /**
// FUNCTION :: SetAsLink16Node
// PURPOSE :: Enables Link16 flag for node
// PARAMETERS :: None.
// RETURNS :: None
// **/
void VRLinkRadio::SetAsLink16Node()
{
    isLink16Node = true;
}
// /**
// FUNCTION :: GetMarkingData
// PURPOSE :: Selector for returning the radio marking data.
// PARAMETERS :: None
// RETURN :: string
// **/
string VRLinkRadio::GetMarkingData() const
{
    return markingData;
}

// /**
// FUNCTION :: GetNode
// PURPOSE :: Selector for returning the QualNet node corresponding to the
//            radio.
// PARAMETERS :: None
// RETURN :: Node* : Node pointer.
// **/
Node* VRLinkRadio::GetNode() const
{
    return node;
}

// /**
// FUNCTION :: GetRadioIndex
// PURPOSE :: Selector for returning the radio's index.
// PARAMETERS :: None
// RETURN :: unsigned short : Radio index.
// **/
unsigned short VRLinkRadio::GetRadioIndex() const
{
    return radioIndex;
}

// /**
// FUNCTION :: GetNetworkPtr
// PURPOSE :: Selector for returning the associated network of the radio.
// PARAMETERS :: None
// RETURN :: const VRLinkNetwork* : Associated radio's network.
// **/
const VRLinkNetwork* VRLinkRadio::GetNetworkPtr()
{
    return networkPtr;
}

// /**
// FUNCTION :: GetEntityPtr
// PURPOSE :: Selector for returning the entity with which the radio is
//            associated.
// PARAMETERS :: None
// RETURN :: const VRLinkEntity* : Associated entity.
// **/
const VRLinkEntity* VRLinkRadio::GetEntityPtr() const
{
    return entityPtr;
}

// /**
// FUNCTION :: GetTxOperationalStatus
// PURPOSE :: Selector for returning the radio's transmission operational
//            status.
//            associated.
// PARAMETERS :: None
// RETURN :: DtRadioTransmitState : Radio's transmission operational status.
// **/
DtRadioTransmitState VRLinkRadio::GetTxOperationalStatus()
{
    return txOperationalStatus;
}

// /**
// FUNCTION :: GetDefaultDstRadioPtr
// PURPOSE :: Selector for returning the radio's default destination radio
//            pointer.
// PARAMETERS :: None
// RETURN :: const VRLinkRadio* : Default destination radio pointer.
// **/
const VRLinkRadio* VRLinkRadio::GetDefaultDstRadioPtr()
{
    return defaultDstRadioPtr;
}

// /**
// FUNCTION :: GetLatLonAltSch
// PURPOSE :: Selector for returning the radio's scheduled latitude,
//            longitude and altitude.
// PARAMETERS :: None
// RETURN :: DtVector : Scheduled latitude, longitude and altitude.
// **/
DtVector VRLinkRadio::GetLatLonAltSch()
{
    return latLonAltScheduled;
}

// /**
// FUNCTION :: GetRelativePosition
// PURPOSE :: Selector for returning the radio's relative position in GCC.
// PARAMETERS :: None
// RETURN :: DtVector : Relative position.
// **/
DtVector VRLinkRadio::GetRelativePosition()
{
    return relativePosition;
}

// /**
// FUNCTION :: ParseRecordFromFile
// PURPOSE :: To parse a record from the radios file.
// PARAMETERS ::
// + nodeInputStr : char* : Pointer to the record to be parsed.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::ParseRecordFromFile(
    PartitionData* partitionData,
    char* nodeInputStr)
{
    char* tempStr = nodeInputStr;
    char* p = NULL;
    char fieldStr[MAX_STRING_LENGTH] = "";

    p = simIface->GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read nodeId", nodeInputStr);

    if (!(simIface->ReturnNodePointerFromPartition(partitionData, &node,
                                                   atoi(fieldStr), TRUE)))
    {
        VRLinkVerify(FALSE, "Can't get node pointer associated with nodeId",
                                                               nodeInputStr);
    }

    p = simIface->GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    markingData = fieldStr;
    VRLinkVerify(p != NULL, "Can't read markingData", nodeInputStr);

    p = simIface->GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read RadioIndex", nodeInputStr);
    radioIndex = atoi(fieldStr);

    p = simIface->GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read RelativePosition (x)", nodeInputStr);
    relativePosition.setX(atof(fieldStr));

    p = simIface->GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read RelativePosition (y)", nodeInputStr);
    relativePosition.setY(atof(fieldStr));

    p = simIface->GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read RelativePosition (z)", nodeInputStr);
    relativePosition.setZ(atof(fieldStr));

    // If this is a proxy for a node that is on a different partiont,
    // we use dynamic hierarchy to obtain the node's transmitPower.
    // Internal Doc: 9-07: Dynamic hierarchy can be used
    // here because the other partitions won't invoke
    // InitNodes () - which is calling us - because
    // only partition 0 performs HlaInitNodes (). So all the
    // other partitions will have completed
    // EXTERNAL_InitializeExternalInterfaces () and thus will be in
    // their event loops and able to respond to dyn hierarchy reqs.

    simIface->RandomSetSeed(
              seed,
              simIface->GetGlobalSeed(node),
              simIface->GetNodeId(node),
              simIface->GetVRLinkExternalInterfaceType());
}

// /**
// FUNCTION :: SetInitialPhyTxPowerForRadio
// PURPOSE :: Initializes the txPower variables for this radio.
// PARAMETERS ::
// + txPower : double : txPower value
// **/
void VRLinkRadio::SetInitialPhyTxPowerForRadio(double txPower)
{
    if (txPower > 0.0)
    {
        usesTxPower = true;
        maxTxPower = txPower;
        currentMaxTxPower = txPower;
    }
    else
    {
        // Radio has no physicals
        usesTxPower = false;
    }
}


// /**
// FUNCTION :: IsCommEffectsRequestValid
// PURPOSE :: Validates the communication effects request.
// PARAMETERS ::
// + dstEntityPtr : const VRLinkEntity** : Destination entity.
// + dstRadioPtr : const VRLinkRadio** : Destination radio.
// + unicast : bool& : Specifies whether the message is unicast or not.
// + vrLink : VRLink* : VRLink pointer.
// RETURN :: bool : TRUE if a valid request is received; FALSE otherwise.
// **/
bool VRLinkRadio::IsCommEffectsRequestValid(
    const VRLinkEntity** dstEntityPtr,
    const VRLinkRadio** dstRadioPtr,
    bool& unicast,
    VRLink* vrLink)
{
    if (*dstEntityPtr != NULL)
    {
        if ((*dstEntityPtr)->IsIdenticalEntityId(&entityPtr))
        {
            VRLinkReportWarning("Can't send message to self");
            return false;
        }
    }

    if (networkPtr == NULL)
    {
        VRLinkReportWarning("Can't find associated network");
        return false;
    }

    if (networkPtr->IsUnicast())
    {
        unicast = true;
        if (*dstEntityPtr == NULL)
        {
            *dstRadioPtr = defaultDstRadioPtr;
            *dstEntityPtr = (*dstRadioPtr)->entityPtr;
        }
        else
        {
            // Find dst radio in same network as src radio.

            hash_map<NodeId, VRLinkRadio*> radioPtrsHash;
            hash_map<NodeId, VRLinkRadio*>::iterator radioPtrsIter;

            radioPtrsHash = networkPtr->GetRadioPtrsHash();

            for (radioPtrsIter = radioPtrsHash.begin();
                 radioPtrsIter != radioPtrsHash.end(); radioPtrsIter++)
            {
                if (radioPtrsIter->second->node == node)
                {
                    continue;
                }

                if ((*dstEntityPtr)->IsIdenticalEntityId(
                                        (&radioPtrsIter->second->entityPtr)))
                {
                    break;
                }
            }

            if (radioPtrsIter == radioPtrsHash.end())
            {
                VRLinkReportWarning("Dst EntityID is not in same network");
                return false;
            }

            *dstRadioPtr = radioPtrsIter->second;
        }
    }
    else
    {
        unicast = false;
    }

    if (!unicast)
    {
         //This is a broadcast, so check that there is at least one receiver.

        hash_map<NodeId, VRLinkRadio*> radioPtrsHash;
        hash_map<NodeId, VRLinkRadio*>::iterator radioPtrsIter;

        radioPtrsHash = networkPtr->GetRadioPtrsHash();

        for (radioPtrsIter = radioPtrsHash.begin();
                 radioPtrsIter != radioPtrsHash.end(); radioPtrsIter++)
        {
            if (radioPtrsIter->second->node == node)
            {
                continue;
            }

            break;
        }

        if (radioPtrsIter == radioPtrsHash.end())
        {
            if (vrLink)
            {
                if ((vrLink->GetDebug()) || (vrLink->GetDebugPrintComms()))
                {
                    VRLinkReportWarning(
                        "Ignoring broadcast to network with no potential"
                        " receivers");
                }
            }

            return false;
        }
    }
    return true;
}

// /**
// FUNCTION :: Remove
// PURPOSE :: To set the radio's tx operational status off.
// PARAMETERS :: None.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::Remove()
{
    txOperationalStatus = DtOff;
}

// /**
// FUNCTION :: ~VRLinkRadio
// PURPOSE :: Destructor of Class VRLinkRadio.
// PARAMETERS :: None.
// **/
VRLinkRadio::~VRLinkRadio()
{
    entityPtr = NULL;
    networkPtr = NULL;
    defaultDstRadioPtr = NULL;
}

// /**
// FUNCTION :: ScheduleChangeInterfaceState
// PURPOSE :: To start/end fault on an interface depending on the received
//            tx operational state.
// PARAMETERS ::
// + enableInterfaces : bool : TRUE when fault on an interface is to be
//                             ended; FALSE otherwise.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::ScheduleChangeInterfaceState(bool enableInterface)
{
    clocktype simTime = simIface->GetCurrentSimTime(node);
    EXTERNAL_Interface* iface =simIface->GetExternalInterfaceForVRLink(node);
    clocktype delay = MAX(simIface->QueryExternalTime(iface) - simTime, 0);

    if (delay == 0)
    {
        delay = simTime;
    }
    else
    {
        delay = simIface->QueryExternalTime(iface);
    }

    if (enableInterface)
    {
        simIface->EXTERNAL_ActivateNode(
            iface,
            node,
            simIface->GetExternalScheduleSafeType(),
            delay);
    }
    else
    {
        simIface->EXTERNAL_DeactivateNode(
            iface,
            node,
            simIface->GetExternalScheduleSafeType(),
            delay);
    }
}

// /**
// FUNCTION :: DisplayData
// PURPOSE :: To print the data for a radio.
// PARAMETERS :: None.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::DisplayData()
{
    cout<<"\nQualNet nodeId : "<<simIface->GetNodeId(node);
    cout<<"\nRadio index : "<<entityId;
    cout<<"\nMarking Data : "<<markingData;
    cout<<"\nEntityId : "<<entityId;
    cout<<"\nTxOperationalStatus : ";

    switch (txOperationalStatus)
    {
        case DtOff:
        {
            cout <<"Off";
            break;
        }
        case DtRadioOnNotTransmitting:
        {
            cout <<"OnButNotTransmitting";
            break;
        }
        case DtOn:
        {
            cout <<"OnAndTransmitting";
            break;
        }
        default:
        {
            VRLinkReportError(
                "Invalid tx operational status encountered",
                __FILE__, __LINE__);
        }
    }

    if (defaultDstRadioPtr)
    {

        cout<<"\n Default destination radio (nodeId / radio index) : (";
        cout<<simIface->GetNodeId(defaultDstRadioPtr->node)<<" / ";
        cout<<defaultDstRadioPtr->radioIndex<<")";
    }
}

// /**
// FUNCTION :: ChangeMaxTxPower
// PURPOSE :: To change the tx power of a radio.
// PARAMETERS ::
// + restoreMaxTxPower : bool : TRUE when radio's transmit power is to be set
//                              to maximum; FALSE otherwise.
// + probability : double : Probability of changing the transmit power of the
//                          radio when restoreMaxTxPower is set to FALSE.
// + maxFraction : double : Fraction of change in the transmit power of the
//                          radio when restoreMaxTxPower is set to FALSE.
// + vrLink : VRLink* : VRLink pointer.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::ChangeMaxTxPower(
    bool restoreMaxTxPower,
    double probability,
    double maxFraction,
    VRLink* vrLink)
{
    if (!usesTxPower) { return; }

    double newTxPower;

    if (restoreMaxTxPower)
    {
        newTxPower = maxTxPower;
    }
    else
    if (simIface->RandomErand(seed) > probability)
    {
        return;
    }
    else
    {
        newTxPower = currentMaxTxPower
                     * (maxFraction * simIface->RandomErand(seed));
    }

    if ((vrLink->GetType() == VRLINK_TYPE_HLA13) ||
        (vrLink->GetType() == VRLINK_TYPE_HLA13))
    {
        cout << "VRLINK: (" << entityPtr->GetEntityId().string()
             << ", " << radioIndex << ") " << entityPtr->GetMarkingData()
             << " Old TX power = " << currentMaxTxPower
             << " New TX power = " << newTxPower << " mW" << endl;
    }
    else if (vrLink->GetDebugPrintTxPower())
    {
        cout << "VRLINK: " << entityPtr->GetMarkingData() << " ("
                << simIface->GetNodeId(node) << ","
                << radioIndex << ")"
                << " Old TX power = " << currentMaxTxPower
                << " New TX power = " << newTxPower << " mW" << endl;
    }

    const int phyIndex = 0;

    // Ask EXTERNAL to change the phy tx power - this will become
    // a cross-partition message if needed (if node to change is remote)
    // Internal note: We could instead have ScheduleChangeMaxTxPower ()
    // perform the probability for each radio and then build an info
    // for each radio's node. That way the sceduled event would
    // get onto the correct partition a tiny bit sooner
    // (wall clock time) and then receiving partition would be
    // able to directly use PHY_SetTransmitPower (). However, it's
    // more work, not a big time diffs, and possibly less efficent.
    simIface->SetExternalPhyTxPower(node, phyIndex, newTxPower);
    currentMaxTxPower = newTxPower;
}

// /**
// FUNCTION :: GetName
// PURPOSE :: Selector to return the network name.
// PARAMETERS :: None
// RETURN :: string& : Network name.
// **/
string& VRLinkNetwork::GetName()
{
    return name;
}

// /**
// FUNCTION :: GetIpAddress
// PURPOSE :: Selector to return the network ipAddress.
// PARAMETERS :: None
// RETURN :: unsigned : IP Address.
// **/
unsigned VRLinkNetwork::GetIpAddress() const
{
    return ipAddress;
}

// /**
// FUNCTION :: IsUnicast
// PURPOSE :: Selector to return whether all transmissions on the network
//            should be modelled as unicast or not.
// PARAMETERS :: None
// RETURN :: bool : TRUE, when all transmissions on the network are to be
//                  modelled as unicast; FALSE otherwise.
// **/
 bool VRLinkNetwork::IsUnicast()
{
    return unicast;
}

// /**
// FUNCTION :: ~VRLinkNetwork
// PURPOSE :: Destructor of Class VRLinkNetwork.
// PARAMETERS :: None.
// **/
VRLinkNetwork::~VRLinkNetwork()
{
    radioPtrsHash.erase(radioPtrsHash.begin(), radioPtrsHash.end());
}

// /**
// FUNCTION :: GetRadioPtrsHash
// PURPOSE :: Selector to return the radios present in the network.
// PARAMETERS :: None.
// RETURN :: const hash_map<NodeId, VRLinkRadio*>& : Radio list.
// **/
const hash_map<NodeId, VRLinkRadio*>& VRLinkNetwork::GetRadioPtrsHash() const
{
    return radioPtrsHash;
}

// /**
// FUNCTION :: ParseRecordFromFile
// PURPOSE :: To parse a record from the networks file.
// PARAMETERS ::
// + nodeInputStr : char* : Pointer to the record to be parsed.
// RETURN :: void : NULL.
// **/
void VRLinkNetwork::ParseRecordFromFile(
    char* nodeInputStr,
    const hash_map <NodeId, VRLinkRadio*>& radioList)
{
    char* tempStr = nodeInputStr;
    char radioId[MAX_STRING_LENGTH] = "";
    char* p = NULL;
    char fieldStr[MAX_STRING_LENGTH] = "";
    char* next = NULL;

    hash_map <NodeId, VRLinkRadio*>::const_iterator radiosPtrIter;

    p = simIface->GetDelimitedToken(fieldStr , tempStr, ";", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read network name", nodeInputStr);
    name = fieldStr;

    p = simIface->GetDelimitedToken(fieldStr , tempStr, ";", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read network frequency", nodeInputStr);
    int retVal = atouint64(fieldStr, &frequency);
    VRLinkVerify(retVal == 1, "Can't parse network frequency", nodeInputStr);

    p = simIface->GetDelimitedToken(fieldStr , tempStr, ";", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);

    //parsing node list
    next = fieldStr;
    p = simIface->GetDelimitedToken(radioId, next, ",", &next);
    simIface->TrimLeft(radioId);
    simIface->TrimRight(radioId);

    while (p)
    {
        radiosPtrIter = radioList.find(atoi(radioId));

        if (radiosPtrIter != radioList.end())
        {
            VRLinkRadio* radio = radiosPtrIter->second;
            radioPtrsHash[atoi(radioId)] = radio;

            VRLinkVerify(
                radio->GetNetworkPtr() == NULL,
                "NodeId present in more than one network",
                nodeInputStr);

            radio->SetNetworkPtr(this);
        }
        else
        {
            VRLinkReportError(
                "Radio found in network not present in radioList",
                __FILE__, __LINE__);
        }

        p = simIface->GetDelimitedToken(radioId, next, ",", &next);
        simIface->TrimLeft(radioId);
        simIface->TrimRight(radioId);
    }

    VRLinkVerify(!radioPtrsHash.empty(),
                     "Network must have at least one node",
                     nodeInputStr);

    p = simIface->GetDelimitedToken(fieldStr , tempStr, ";", &tempStr);
    simIface->TrimLeft(fieldStr);
    simIface->TrimRight(fieldStr);
    VRLinkVerify(tempStr != NULL, "Can't parse IP Address", nodeInputStr);

    int numHostBits;
    BOOL isNodeId;

    simIface->ParseNodeIdHostOrNetworkAddress(
                fieldStr,
                &ipAddress,
                &numHostBits,
                &isNodeId);

    VRLinkVerify(!isNodeId, "Can't parse IP address", nodeInputStr);
    unicast = (ipAddress == 0);
}

// /**
// FUNCTION :: GetDefaultDstRadioPtrForRadio
// PURPOSE :: To get the default destination radio pointer for the radio
// PARAMETERS ::
// + srcRadio : VRLinkRadio* : Radio for which the default destination radio
//                             is to be retrieved.
// RETURN :: VRLinkRadio* : Default destination radio pointer.
// **/
VRLinkRadio* VRLinkNetwork::GetDefaultDstRadioPtrForRadio(
    VRLinkRadio* srcRadio)
{
    VRLinkVerify(
        (!radioPtrsHash.empty()),
        "No radios found for network",
        __FILE__, __LINE__);

    hash_map<NodeId, VRLinkRadio*>::iterator radioPtrsIter;

    for (radioPtrsIter = radioPtrsHash.begin();
                       radioPtrsIter != radioPtrsHash.end(); ++radioPtrsIter)
    {
        if (radioPtrsIter->second->GetNode() == srcRadio->GetNode())
        {
            continue;
        }
        return radioPtrsIter->second;
    }
    return NULL;
}

// /**
// FUNCTION :: VRLink
// PURPOSE :: Initializing function for VRLink.
// PARAMETERS ::
// + ifacePtr : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
// **/
VRLink::VRLink(EXTERNAL_Interface* ifacePtr, NodeInput* nodeInput, VRLinkSimulatorInterface *simIface)
{
    this->simIface = simIface;
    debug = false;
    debug2 = false;
    debugPrintComms = false;
    debugPrintComms2 = false;
    debugPrintMapping = false;
    debugPrintDamage = false;
    debugPrintTxState = false;
    debugPrintTxPower = false;
    debugPrintMobility = false;
    debugPrintTransmitterPdu = false;
    debugPrintPdus = false;

    endProgram = false;
    newEventScheduled = false;

    siteId = 1;
    applicationId = DEFAULT_APPLICATION_ID;

    //For the maximum msgId, INT_MAX - 1 is used instead of UINT_MAX - 1
    //because the msgId values are passed into the Messenger application,
    //and Messenger uses the (signed) int type for msgId.
    maxMsgId = INT_MAX - 1;

    slightDamageReduceTxPowerProbability = 0.25;
    moderateDamageReduceTxPowerProbability = 0.25;
    destroyedReduceTxPowerProbability = 0.50;
    slightDamageReduceTxPowerMaxFraction = 0.75;
    moderateDamageReduceTxPowerMaxFraction = 0.75;
    destroyedReduceTxPowerMaxFraction = 0.75;

    debugPrintRtss = false;

    disableReflectRadioTimeouts = false;
}

// /**
//FUNCTION :: CreateFederation
//PURPOSE :: To make QualNet join the federation. If the federation does not
//           exist, the exconn API creates a new federation first, and then
//           register QualNet with it.
//PARAMETERS :: None
//RETURN :: void : NULL
// **/
void VRLink::CreateFederation()
{
}

// /**
//FUNCTION :: InitConfigurableParameters
//PURPOSE :: To initialize the user configurable shared VRLink parameters or 
//          initialize the corresponding variables with the default values.
//PARAMETERS ::
// + nodeInput : NodeInput* : Pointer to cached file.
//RETURN :: void : NULL
// **/
void VRLink::InitConfigurableParameters(NodeInput* nodeInput)
{
    BOOL parameterFound;
    char buf[MAX_STRING_LENGTH];

    simIface->ReadTime(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "VRLINK-MOBILITY-INTERVAL",
        &parameterFound,
        &mobilityInterval);

    if (parameterFound)
    {
        VRLinkVerify(mobilityInterval >= 0.0, "Can't use negative time");
    }
    else
    {
        mobilityInterval = 500 * MILLI_SECOND;
    }

    char mobilityIntervalString[MAX_STRING_LENGTH];
    simIface->PrintClockInSecond(mobilityInterval, mobilityIntervalString);

    cout << "Mobility interval                         = " << mobilityIntervalString
            << " second(s)" << endl;

    simIface->ReadDouble(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-XYZ-EPSILON",
                &parameterFound,
                &xyzEpsilon);

    if (parameterFound == FALSE)
    {
        xyzEpsilon = 0.57735026918962576450914878050196;
    }

    VRLinkVerify(xyzEpsilon >= 0.0, "Can't use negative epsilon value");

    cout << "GCC (x,y,z) epsilons are ("
            << xyzEpsilon << ","
            << xyzEpsilon << ","
            << xyzEpsilon << " meter(s))" << endl
         << "  For movement to be reflected in QualNet, change in position"
            " must be" << endl
         << "  >= any one of these values." << endl;    

    simIface->IO_ReadInt(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "VRLINK-SITE-ID",
        &parameterFound,
        &siteId);

    if (!parameterFound)
    {
        cout<<"\nSiteID not found; assigning default value of 1 to it";
    }
    else
    {
        VRLinkVerify(siteId > 0, "Can't use zero or negative siteId value");
    }

    simIface->IO_ReadInt(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "VRLINK-APPLICATION-NUMBER",
        &parameterFound,
        &applicationId);

    if (!parameterFound)
    {
        cout<<"\nApplication Number not found; assigning default value of 4000\n";
    }
    else
    {
        VRLinkVerify(applicationId > 0, "Can't use zero or negative application number value");
    }


    if (simIface->IsMilitaryLibraryEnabled())
    {
        simIface->IO_ReadString(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "VRLINK-DEBUG-PRINT-RTSS",
                    &parameterFound,
                    buf);

        cout << "VRLINK debugging output (Ready To Send Signal interaction) is ";

        if (parameterFound && (strcmp(buf, "NO") == 0))
        {
            debugPrintRtss = false;
            cout << "off." << endl;
        }
        else
        {
            debugPrintRtss = true;
            cout << "on." << endl;
        }
    }

    char path[g_PathBufSize];
    BOOL retVal;

    simIface->IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "VRLINK-ENTITIES-FILE-PATH",
        &retVal,
        path);

    VRLinkVerify(
        retVal == TRUE,
        "VRLINK-ENTITIES-FILE-PATH must be specified",
        __FILE__, __LINE__);

    entitiesPath = path;

    simIface->IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "VRLINK-RADIOS-FILE-PATH",
        &retVal,
        path);

    VRLinkVerify(
        retVal == TRUE,
        "VRLINK-RADIOS-FILE-PATH must be specified",
        __FILE__, __LINE__);

    radiosPath = path;

    simIface->IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "VRLINK-NETWORKS-FILE-PATH",
        &retVal,
        path);

    VRLinkVerify(
        retVal == TRUE,
        "VRLINK-NETWORKS-FILE-PATH must be specified",
        __FILE__, __LINE__);

    networksPath = path;
}

// /**
// FUNCTION :: SetType
// PURPOSE :: Selector to set the VRLink interface type.
// PARAMETERS ::
// + type : VRLinkInterfaceType : VRLink interface to set.
// RETURN :: void : NULL.
// **/
void VRLink::SetType(VRLinkInterfaceType type)
{
    vrLinkType = type;
}

// /**
//FUNCTION :: GetType
//PURPOSE :: Selector to return the VRLink interface type.
//PARAMETERS :: None
//RETURN :: VRLinkInterfaceType : Configured VRLink interface.
// **/
VRLinkInterfaceType VRLink::GetType()
{
    return vrLinkType;
}

// /**
//FUNCTION :: GetSimIface
//PURPOSE :: Selector to return the VRLinkSimulatorInterface (singleton
//           class) pointer.
//PARAMETERS :: None
//RETURN :: VRLinkSimulatorInterface* : VRLinkSimulatorInterface pointer.
// **/
VRLinkSimulatorInterface* VRLink::GetSimIface()
{
    return simIface;
}

// /**
//FUNCTION :: GetDebug
//PURPOSE :: Selector to return the value of debug parameter.
//PARAMETERS :: None
//RETURN :: bool : Value of debug parameter.
// **/
bool VRLink::GetDebug()
{
    return debug;
}

// /**
//FUNCTION :: GetDebug2
//PURPOSE :: Selector to return the value of debug2 parameter.
//PARAMETERS :: None
//RETURN :: bool : Value of debug2 parameter.
// **/
bool VRLink::GetDebug2()
{
    return debug2;
}

// /**
//FUNCTION :: GetDebugPrintMobility
//PURPOSE :: Selector to return the value of debugPrintMobility parameter.
//PARAMETERS :: None
//RETURN :: bool : Value of debugPrintMobility parameter.
// **/
bool VRLink::GetDebugPrintMobility()
{
    return debugPrintMobility;
}

// /**
//FUNCTION :: GetDebugPrintDamage
//PURPOSE :: Selector to return the value of debugPrintDamage parameter.
//PARAMETERS :: None
//RETURN :: bool : Value of debugPrintDamage parameter.
// **/
bool VRLink::GetDebugPrintDamage()
{
    return debugPrintDamage;
}

// /**
//FUNCTION :: GetDebugPrintTransmitterPdu
//PURPOSE :: Selector to return the value of debugPrintTransmitterPdu
//           parameter.
//PARAMETERS :: None
//RETURN :: bool : Value of debugPrintTransmitterPdu parameter.
// **/
bool VRLink::GetDebugPrintTransmitterPdu()
{
    return debugPrintTransmitterPdu;
}

// /**
//FUNCTION :: GetDebugPrintTxState
//PURPOSE :: Selector to return the value of debugPrintTxState parameter.
//PARAMETERS :: None
//RETURN :: bool : Value of debugPrintTxState parameter.
// **/
bool VRLink::GetDebugPrintTxState()
{
    return debugPrintTxState;
}

// /**
//FUNCTION :: GetDebugPrintComms
//PURPOSE :: Selector to return the value of debugPrintComms parameter.
//PARAMETERS :: None
//RETURN :: bool : Value of debugPrintComms parameter.
// **/
bool VRLink::GetDebugPrintComms()
{
    return debugPrintComms;
}

// /**
//FUNCTION :: GetDebugPrintTxPower
//PURPOSE :: Selector to return the value of debugPrintTxPower parameter.
//PARAMETERS :: None
//RETURN :: bool : Value of debugPrintTxPower parameter.
// **/
bool VRLink::GetDebugPrintTxPower()
{
    return debugPrintTxPower;
}

// /**
//FUNCTION :: GetNumLinesInFile
//PURPOSE :: Determines number of lines in an input file.
//PARAMETERS :: 
// + path : const char* : path to input file 
//RETURN :: unsigned : number of lines.
// **/
unsigned
VRLink::GetNumLinesInFile(const char* path)
{
    // This function will correctly return 0 as the number of lines if the file
    // size is 0 bytes.

    // The line buffer should be large enough to accommodate one useful
    // character, a newline, and a null character (three bytes).

    assert(g_lineBufSize >= 3);

    FILE* fp = fopen(path, "r");
    VRLinkVerify(fp != NULL, "Can't open for reading", path);

    const char* p;
    char line[g_lineBufSize];
    unsigned numLines;
    unsigned lineNumber;
    for (numLines = 0, lineNumber = 1;
         (p = fgets(line, sizeof(line), fp)) != NULL;
         numLines++, lineNumber++)
    {
        if (strlen(p) + 1 == sizeof(line))
        {
            // Current line uses all of the line buffer.
            // Check if the line was too long.

            VRLinkVerify(
                p[sizeof(line) - 2] == '\n' || feof(fp),
                "Line too long",
                path,
                lineNumber);
        }//if//
    }//for//

    VRLinkVerify(!ferror(fp), "Error reading file", path);

    fclose(fp);

    return numLines;
}

// /**
//FUNCTION :: TrimLeft
//PURPOSE :: removes whitespace left of acutal characters in char array.
//PARAMETERS :: 
// + s : char* : character array
//RETURN :: none :
// **/
void
VRLink::TrimLeft(char *s)
{
    assert(s != NULL);

    char *p = s;
    unsigned stringLength = strlen(s);
    unsigned numBytesWhitespace = 0;

    while (numBytesWhitespace < stringLength
          && isspace(*p))
    {
        p++;
        numBytesWhitespace++;
    }

    if (numBytesWhitespace > 0)
    {
        memmove(s, s + numBytesWhitespace, stringLength - numBytesWhitespace + 1);
    }
}

// /**
//FUNCTION :: TrimRight
//PURPOSE :: removes whitespace right of acutal characters in char array.
//PARAMETERS :: 
// + s : char* : character array
//RETURN :: none :
// **/
void
VRLink::TrimRight(char *s)
{
    assert(s != NULL);

    unsigned stringLength = strlen(s);
    if (stringLength == 0) { return; }

    // p points one byte to left of terminating NULL.

    char *p;

    for (p = &s[stringLength - 1]; isspace(*p); p--)
    {
        if (p == s)
        {
            // Entire string was whitespace.

            *p = 0;
            return;
        }
    }//while//

    *(p + 1) = 0;
}

// /**
// FUNCTION :: ReadEntitiesFile
// PURPOSE :: To parse the .vrlink-entities file.
// PARAMETERS ::
// RETURN :: void : NULL
// **/
void VRLink::ReadEntitiesFile()
{
    VRLinkVerify(
        entities.empty(),
        "Data already present in .entities file",
        __FILE__, __LINE__);

    // Determine path of .hla-entities file.

    const char *path = entitiesPath.c_str();

    cout << ".vrlink-entities file = " << path << "." << endl;

    // Determine number of lines in file.

    int numEntities = GetNumLinesInFile(path);

    if (numEntities == 0) {
        return;
    }

    // Open file.

    FILE* fpEntities = fopen(path, "r");
    VRLinkVerify(fpEntities != NULL, "Can't open for reading", path);

    // Read file.

    char line[g_lineBufSize];    
    unsigned lineNumber;
    unsigned i;
    for (i = 0;
         i < numEntities;
         i++)
    {
        VRLinkVerify(fgets(line, g_lineBufSize, fpEntities) != NULL,
                 "Not enough lines",
                 path);

        VRLinkVerify(strlen(line) < g_lineBufSize - 1,
                 "Exceeds permitted line length",
                 path);
                
        VRLinkEntity* entity = new VRLinkEntity;

        // parse record from file, and retrieve the
        // corresponding senderId(s)
        entity->ParseRecordFromFile(line);

        hash_map <string, VRLinkEntity*>::iterator 
            entitiesIter = entities.find(entity->GetMarkingData());

        if (entitiesIter == entities.end())
        {
            entities[entity->GetMarkingData()] = entity;                     
        }
        else
        {
            VRLinkVerify(entitiesIter == entities.end(),
                   "Entity with duplicate MarkingData",
                    line, i + 1);
        }
    }
}

// /**
// FUNCTION :: ReadRadiosFile
// PURPOSE :: To parse the .vrlink-radios file.
// PARAMETERS ::
// RETURN :: void : NULL
// **/
void VRLink::ReadRadiosFile()
{
     VRLinkVerify(
        radios.empty(),
        "Data already present in .radios file",
        __FILE__, __LINE__);


     // Determine path of .hla-radios file.

    const char *path = radiosPath.c_str();

    cout << ".vrlink-radios file = " << path << "." << endl;

    // Determine number of lines in file.

    int numRadios = GetNumLinesInFile(path);

    if (numRadios == 0) {
        return;
    }

    // Open file.

    FILE* fpRadios = fopen(path, "r");
    VRLinkVerify(fpRadios != NULL, "Can't open for reading", path);

    // Read file.

    char line[g_lineBufSize];        
    unsigned i;
    for (i = 0;
         i < numRadios;
         i++)
    {
        VRLinkVerify(fgets(line, g_lineBufSize, fpRadios) != NULL,
                 "Not enough lines",
                 path);

        VRLinkVerify(strlen(line) < g_lineBufSize - 1,
                 "Exceeds permitted line length",
                 path);        

        hash_map <NodeId, VRLinkRadio*>::iterator radiosIter;
        hash_map<NodeId, VRLinkData*>::iterator nodeDataIter;


         VRLinkRadio* newRadio = new VRLinkRadio();

        //parse record from file, and retrieve the
        //corresponding senderId(s)
        newRadio->ParseRecordFromFile(
                      simIface->GetPartitionForIface(iface),
                      line);
        
        //this will handles setting the txPower of the radio, remote message sending etc.
        GetInitialPhyTxPowerForRadio(newRadio);

        NodeId nodeId = simIface->GetNodeId(newRadio->GetNode());

        radiosIter = radios.find(nodeId);

         if (radiosIter != radios.end())
         {
            VRLinkReportError("Dulplicate entry for radio",
                                __FILE__, __LINE__);
         }

         hash_map <string, VRLinkEntity*>::iterator entitiesIter;

         entitiesIter = entities.find(newRadio->GetMarkingData());

         if (entitiesIter == entities.end())
         {
            VRLinkReportError("Associated entity not present",
                                __FILE__, __LINE__);
         }

        radios.insert(pair<NodeId, VRLinkRadio*>(nodeId, newRadio));
        newRadio->SetLatLonAltScheduled(entitiesIter->second);

        //Initialize per-node VRLink data for this nodeId.

        nodeDataIter = nodeIdToPerNodeDataHash.find(nodeId);

        VRLinkVerify(nodeDataIter == nodeIdToPerNodeDataHash.end(),
            " Duplicate radio entry found in Radios File",
            __FILE__, __LINE__);

        VRLinkData* vrlinkData = new VRLinkData;
        vrlinkData->SetRadioPtr(newRadio);
        vrlinkData->SetNextMsgId(0);

        nodeIdToPerNodeDataHash.insert(pair<NodeId, VRLinkData*>
                                        (nodeId,vrlinkData));

        RegisterRadioWithEntity(newRadio);

        NodeId id = simIface->GetNodeId(newRadio->GetNode());

        if (simIface->IsMilitaryLibraryEnabled())
        {
            for (int i = 0; i < simIface->GetNumInterfacesForNode(newRadio->GetNode()); i++)
            {
                char paramVal[g_PathBufSize];
                BOOL retVal;

                simIface->IO_ReadString(
                    id,
                    simIface->GetInterfaceAddressForInterface(newRadio->GetNode(), i),
                    simIface->GetNodeInputFromIface(iface),
                    "MAC-PROTOCOL",
                    &retVal,
                    paramVal);

                if (retVal)
                {
                    if (strcmp(paramVal, "MAC-LINK-16") == 0)
                    {
                        newRadio->SetAsLink16Node();
                    }
                    else if (strcmp(paramVal, "MAC-LINK-11") == 0)
                    {
                        newRadio->SetAsLink11Node();
                    }
                }
            }
        }
    }
}

// /**
// FUNCTION :: ReadNetworksFile
// PURPOSE :: To parse the .vrlink-networks file.
// PARAMETERS ::
// + partitionData : PartitionData* : Pointer to partition data.
// + nodeInput : NodeInput* : Pointer to cached file.
// RETURN :: void : NULL
// **/
void VRLink::ReadNetworksFile()
{
    VRLinkVerify(
        networks.empty(),
        "Data already present in .networks file",
        __FILE__, __LINE__);

    cout << ".vrlink-networks file = " << networksPath << "." << endl;

    NodeInput &networksFile = *(simIface->IO_CreateNodeInput());

    simIface->IO_CacheFile(&networksFile, networksPath.c_str());

    VRLinkVerify(
        networksFile.numLines > 0,
        "Networks file is empty",
        __FILE__, __LINE__);

    int i = 0;
    VRLinkNetwork* newNetwork = NULL;

    hash_map <string, VRLinkNetwork*>::iterator networksIter;

    while (i < networksFile.numLines)
    {
        newNetwork = new VRLinkNetwork();

      //parse record from file, and retrieve the
      //corresponding senderId(s)
        newNetwork->ParseRecordFromFile(
                        networksFile.inputStrings[i],
                        radios);

        networksIter = networks.find(newNetwork->GetName());

        if (networksIter == networks.end())
        {
            networks.insert(pair<string, VRLinkNetwork*>
                                (newNetwork->GetName(), newNetwork));
        }
        else
        {
            VRLinkReportError("Duplicate entry for network",
                                __FILE__, __LINE__);
        }
        ++i;
    }
}

// /**
// FUNCTION :: ProcessEvent
// PURPOSE :: Virtual function to handle all the protocol events for HLA/DIS.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg : Message* : Pointer to message.
// RETURN :: void : NULL
// **/
void VRLink::ProcessEvent(Node* node, Message* msg)
{
}

// /**
// FUNCTION :: SetSimulationTime
// PURPOSE :: To set the VRLink clock.
// PARAMETERS :: None.
// RETURN :: void : NULL
// **/
void VRLink::SetSimulationTime()
{
    DtClock* clock = exConn->clock();

    DtTime simTime = simIface->ConvertClocktypeToDouble(
                               simIface->QueryExternalSimulationTime(iface));

    DtTime elapRealTime = clock->elapsedRealTime();

    if (simTime > elapRealTime)
    {
        GoingToSleep((unsigned)(simTime - elapRealTime));
    }

    // Tell VR-Link the current value of simulation time.
    clock->setSimTime(simTime);
}

// /**
//FUNCTION :: ScheduleTasksIfAny
//PURPOSE :: Virtual function to schedule tasks if dynamic stats are enabled
//           for HLA.
//PARAMETERS :: None.
//RETURN :: void : NULL.
// **/
void VRLink::ScheduleTasksIfAny()
{
}

// /**
// FUNCTION :: IsMobilityEventSchedulable
// PURPOSE :: Called to ascertain whether attributes of QualNet nodes can be
//            updated for a specific entity or not.
// PARAMETERS ::
// + entity : VRLinkEntity* : Entity for which node attributes updation is to
//                            be decided.
// + mobilityEventTime : clocktype& : Mobility event time.
// + simTime : clocktype& : Simulation time.
// RETURN :: void : NULL
// **/
bool VRLink::IsMobilityEventSchedulable(
    VRLinkEntity* entity,
    clocktype& mobilityEventTime,
    clocktype& simTime)
{
    VRLinkVerify(
        (simIface->GetPartitionForIface(iface) != NULL),
        "NULL Partitiondata pointer encountered",
        __FILE__, __LINE__);

    simTime = simIface->GetCurrentTimeForPartition(iface);
    mobilityEventTime = MAX(simIface->QueryExternalTime(iface), simTime + 1);

    if (mobilityEventTime < entity->GetLastScheduledMobilityEventTime() +
                                                            mobilityInterval)
    {
        return false;
    }

    return true;
}

// /**
//FUNCTION :: GetExtIfacePtr
//PURPOSE :: Selector to return the external interface pointer.
//PARAMETERS :: None
//RETURN :: EXTERNAL_Interface* : Value of the external interface pointer.
// **/
EXTERNAL_Interface* VRLink::GetExtIfacePtr()
{
    return iface;
}

// /**
//FUNCTION :: GetNodeIdToPerNodeDataHash
//PURPOSE :: Selector to return the hash nodeIdToPerNodeDataHash.
//PARAMETERS :: None
//RETURN :: const hash_map<NodeId,
//          VRLinkData*>& VRLink::GetNodeIdToPerNodeDataHash(): Hash
//          nodeIdToPerNodeDataHash.
// **/
const
hash_map<NodeId, VRLinkData*>& VRLink::GetNodeIdToPerNodeDataHash() const
{
    return nodeIdToPerNodeDataHash;
}

// /**
// FUNCTION :: IsEntityMapped
// PURPOSE :: Ascertains whether the passed entity is present is present in
//            the reflected entity list.
// PARAMETERS ::
// + entity : VRLinkEntity* : VRlink entity.
// RETURN :: bool : Returns TRUE/FALSE depending on whether the passed entity
//                  is present in the reflected entity list or not.
// **/
bool VRLink::IsEntityMapped(const VRLinkEntity* entity)
{
    hash_map <string, VRLinkEntity*>::iterator reflEntitiesIter;

    reflEntitiesIter = reflectedEntities.find(entity->GetEntityIdString());

    if (reflEntitiesIter != reflectedEntities.end())
    {
        return true;
    }
    return false;
}

// /**
// FUNCTION :: AppProcessMoveHierarchy
// PURPOSE :: Handles the event type MSG_EXTERNAL_VRLINK_HierarchyMobility.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg : Message* : Pointer to message.
// RETURN :: void : NULL.
// **/
void VRLink::AppProcessMoveHierarchy(
    Node* node,
    Message* msg)
{
    VRLinkHierarchyMobilityInfo& info =
           *((VRLinkHierarchyMobilityInfo*)simIface->ReturnInfoFromMsg(msg));

    simIface->MoveHierarchyInGUI(info.hierarchyId,
                                 info.coordinates,
                                 info.orientation,
                                 simIface->GetCurrentSimTime(node));
}

// /**
// FUNCTION :: AppProcessChangeMaxTxPowerEvent
// PURPOSE :: Handles the event type MSG_EXTERNAL_VRLINK_ChangeMaxTxPower.
// PARAMETERS ::
// + msg : Message* : Pointer to message.
// RETURN :: void : NULL.
// **/
void VRLink::AppProcessChangeMaxTxPowerEvent(Message* msg)
{
    bool   restoreMaxTxPower = false;
    double probability = 0;
    double maxFraction = 0;

    VRLinkChangeMaxTxPowerInfo& info
        = *((VRLinkChangeMaxTxPowerInfo*) simIface->ReturnInfoFromMsg(msg));

    VRLinkEntity* entity = info.entityPtr;

    switch (info.damageState)
    {
        case DtDamageNone:
        {
            restoreMaxTxPower = true;
            break;
        }

        case DtDamageSlight:
        {
            probability = slightDamageReduceTxPowerProbability;
            maxFraction = slightDamageReduceTxPowerMaxFraction;
            break;
        }

        case DtDamageModerate:
        {
            probability  = moderateDamageReduceTxPowerProbability;
            maxFraction = moderateDamageReduceTxPowerMaxFraction;
            break;
        }

        case DtDamageDestroyed:
        {
            probability = destroyedReduceTxPowerProbability;
            maxFraction = destroyedReduceTxPowerMaxFraction;
            break;
        }

        default:
        {
            VRLinkReportError(
                "Invalid damage state encountered",
                __FILE__, __LINE__);
        }
    }

    entity->ChangeMaxTxPowerOfRadios(
                restoreMaxTxPower,
                probability,
                maxFraction,
                this);
}

// /**
// FUNCTION :: RegisterRadioWithEntity
// PURPOSE :: Associates an entity with a radio and vice-versa.
// PARAMETERS ::
// + radio : VRLinkRadio* : VRLink radio.
// RETURN :: void : NULL.
// **/
void VRLink::RegisterRadioWithEntity(VRLinkRadio* radio)
{
    hash_map <string, VRLinkEntity*>::iterator entitiesIter;

    entitiesIter = entities.find(string(radio->GetMarkingData()));

    if (entitiesIter != entities.end())
    {
        entitiesIter->second->InsertInRadioPtrList(radio);
        radio->SetEntityPtr(entitiesIter->second);
    }
    else
    {
        VRLinkReportError("Entity not found for radio",
                            __FILE__, __LINE__);
    }
}

// /**
// FUNCTION :: RegisterCallbacks
// PURPOSE :: Registers the VRLink callbacks required for communication with
//            other federates i.e. send/receive data.
// PARAMETERS :: None.
// RETURN :: void : NULL.
// **/
void VRLink::RegisterCallbacks()
{
    try
    {
        refEntityList = new DtReflectedEntityList(exConn);

        refEntityList->setDiscoveryCondition(
                           VRLink::EntityCallbackCriteria,
                           NULL);

        refEntityList->addEntityAdditionCallback(
                           VRLink::CbEntityAdded,
                           this);

        refEntityList->addEntityRemovalCallback(
                           VRLink::CbEntityRemoved,
                           this);

        refRadioTxList = new DtReflectedRadioTransmitterList(exConn);

        refRadioTxList->setDiscoveryCondition(
                            VRLink::RadioCallbackCriteria,
                            NULL);

        refRadioTxList->addRadioTransmitterAdditionCallback(
                            VRLink::CbRadioAdded,
                            this);

        refRadioTxList->addRadioTransmitterRemovalCallback(
                            VRLink::CbRadioRemoved,
                            this);

        DtSignalInteraction::addCallback(
                                 exConn,
                                 VRLink::CbSignalIxnReceived,
                                 this);

         RegisterProtocolSpecificCallbacks();
    }
    DtCATCH_AND_WARN(cout);
}

// /**
// FUNCTION :: RegisterProtocolSpecificCallbacks
// PURPOSE :: Registers HLA/DIS specific callbacks.
// PARAMETERS :: None.
// RETURN :: void : NULL
// **/
void VRLink::RegisterProtocolSpecificCallbacks()
{
}

// /**
// FUNCTION :: SetSenderIdForOutgoingDataPdu
// PURPOSE :: Using a sender node Id, set the send entityIdentifer for an
//              outgoing PDU
// PARAMETERS ::
// + nodeId : NodeId : node id of sender
// + pdu : DtDataInterfaction : outgong pdu
// RETURN :: void : NULL
// **/
void VRLink::SetSenderIdForOutgoingDataPdu(
    NodeId nodeId,
    DtDataInteraction& pdu)
{
    hash_map <NodeId, DtEntityIdentifier>::iterator nodeIdToEntityIdIter;

    nodeIdToEntityIdIter = nodeIdToEntityIdHash.find(nodeId);

    if (nodeIdToEntityIdIter != nodeIdToEntityIdHash.end())
    {
        pdu.setSenderId(nodeIdToEntityIdIter->second);
    }
    else
    {
        VRLinkReportWarning(
            "SenderId to PDU could not be assigned",
            __FILE__, __LINE__);
    }
}

// /**
// FUNCTION :: GetNodeIdFromIncomingDataPdu
// PURPOSE :: Gets node ids associated with the entityIdentifier from an
//              incoming data pdu
// PARAMETERS ::
// + pdu : DtDataInterfaction* : pointer to incoming pdu
// RETURN :: list<NodeId>* : pointer to a list of node ids
// **/
list<NodeId>* VRLink::GetNodeIdFromIncomingDataPdu(DtDataInteraction* pdu)
{
    map <EntityIdentifierKey, list<NodeId> >::iterator entityIdToNodeIdIter;

    EntityIdentifierKey* entityIdKey =
                              GetEntityIdKeyFromDtEntityId(&pdu->senderId());

    entityIdToNodeIdIter = entityIdToNodeIdMap.find(*entityIdKey);

    if (entityIdToNodeIdIter != entityIdToNodeIdMap.end())
    {
        return &entityIdToNodeIdIter->second;
    }
    else
    {
        VRLinkReportWarning(
            "NodeId for incoming PDU could not be retrieved",
            __FILE__, __LINE__);
    }

    return NULL;
}

// /**
// FUNCTION :: GetEntityIdKeyFromDtEntityId
// PURPOSE :: Builds entityIdentiferKey from vrlink DtEntityIdentifier
// PARAMETERS :: dtEntityId : DtEntityIdentifier* : pointer to DtEntity identifier
// RETURN :: EntityIdentifierKey* : pointer to a new Entity Identifer structure
// **/
EntityIdentifierKey* VRLink::GetEntityIdKeyFromDtEntityId(
    const DtEntityIdentifier* dtEntityId)
{
    EntityIdentifierKey* entityIdKey = new EntityIdentifierKey;

    entityIdKey->siteId = dtEntityId->site();
    entityIdKey->applicationId = dtEntityId->host();
    entityIdKey->entityNum = dtEntityId->entityNum();

    return entityIdKey;
}

// /**
// FUNCTION :: GetEntityIdKeyFromDtEntityId
// PURPOSE :: Builds entityIdentiferKey from vrlink DtEntityIdentifier
// PARAMETERS :: dtEntityId : DtEntityIdentifier* : pointer DtEntity identifier
// RETURN :: EntityIdentifierKey : Entity Identifer structure
// **/
EntityIdentifierKey VRLink::GetEntityIdKeyFromDtEntityId(
    const DtEntityIdentifier& dtEntityId)
{
    EntityIdentifierKey entityIdKey;

    entityIdKey.siteId = dtEntityId.site();
    entityIdKey.applicationId = dtEntityId.host();
    entityIdKey.entityNum = dtEntityId.entityNum();

    return entityIdKey;
}

// /**
// FUNCTION :: GetNodeIdFromEntityIdToNodeIdMap
// PURPOSE :: Get node ids from an entity identifer using the entityIdToNodeIdMap
// PARAMETERS :: entId : DtEntityIdentifier* : Entity identifier from outside federate
// RETURN :: list<NodeId>* : list of node ids associated with the entity
// **/
const list<NodeId>* VRLink::GetNodeIdFromEntityIdToNodeIdMap(
    DtEntityIdentifier& entId)
{
    map <EntityIdentifierKey, list<NodeId> >::iterator entityIdToNodeIdIter;
    EntityIdentifierKey* entityIdKey =
                              GetEntityIdKeyFromDtEntityId(&entId);

    entityIdToNodeIdIter = entityIdToNodeIdMap.find(*entityIdKey);

    if (entityIdToNodeIdIter == entityIdToNodeIdMap.end())
    {
        return 0;
    }

    return &entityIdToNodeIdIter->second;
}

// /**
// FUNCTION :: GetEntityPtrFromNodeIdToEntityIdHash
// PURPOSE :: Get entity identifier pointer from a node id using the 
//              nodeIdToEntityId hash
// PARAMETERS :: nodeId : NodeId : node id from qualnet
// RETURN :: DtEntityIdentifier* : pointer to vrlink entity identifier
// **/
DtEntityIdentifier* VRLink::GetEntityPtrFromNodeIdToEntityIdHash(
    const NodeId& nodeId)
{
    hash_map <NodeId, DtEntityIdentifier>::iterator nodeIdToEntityIdIter;

    nodeIdToEntityIdIter = nodeIdToEntityIdHash.find(nodeId);

    if (nodeIdToEntityIdIter == nodeIdToEntityIdHash.end())
    {
        return NULL;
    }

    return &nodeIdToEntityIdIter->second;
}

// /**
// FUNCTION :: GetRadioIdKey
// PURPOSE :: Get the RadioIdKey for signal interaction.
// PARAMETERS ::
// + dataIxnInfo : DtSignalInteraction* : Pointer to signal interaction
// RETURN :: RadioIdKey : EntityIdentifierKey and int pair
// **/
RadioIdKey VRLink::GetRadioIdKey(DtSignalInteraction* inter)
{
    return GetRadioIdKey(inter);
}

// /**
// FUNCTION :: GetRadioIdKey
// PURPOSE :: Get the RadioIdKey for radio transmitter repository.
// PARAMETERS ::
// + dataIxnInfo : DtRadioTransmitterRepository* : Pointer to radio 
//                  transmitter repository
// RETURN :: RadioIdKey : EntityIdentifierKey and int pair
// **/
RadioIdKey VRLink::GetRadioIdKey(DtRadioTransmitterRepository* radioTsr)
{
    return GetRadioIdKey(radioTsr);
}

// /**
// FUNCTION :: RadioCallbackCriteria
// PURPOSE :: Specifies the callback calling criteria for addition of
//            radios.
// PARAMETERS ::
// + obj : DtReflectedObject* : Reflected object.
// + usr : void* : User pointer.
// RETURN :: bool : Returns TRUE if criteria satisfied, FALSE otherwise.
// **/
bool VRLink::RadioCallbackCriteria(DtReflectedObject* obj, void* usr)
{
    DtReflectedRadioTransmitter* radTx = (DtReflectedRadioTransmitter*) obj;

    if ((radTx->tsr()->radioId() < 0) ||
                   (radTx->tsr()->entityId() == DtEntityIdentifier(0, 0, 0)))
    {
        return false;
    }
    return true;
}

// /**
// FUNCTION :: EntityCallbackCriteria
// PURPOSE :: Specifies the callback calling criteria for addition of
//            entities.
// PARAMETERS ::
// + obj : DtReflectedObject* : Reflected object.
// + usr : void* : User pointer.
// RETURN :: bool : Returns TRUE if criteria satisfied, FALSE otherwise.
// **/
bool VRLink::EntityCallbackCriteria(DtReflectedObject* obj, void* usr)
{

    DtReflectedEntity* entity = (DtReflectedEntity*) obj;

    if (!(entity->esr()->markingText()) ||
                   (entity->esr()->entityId() == DtEntityIdentifier(0,0,0)))
    {
        return false;
    }
    return true;
}

// /**
// FUNCTION :: TryForFirstObjectDiscovery
// PURPOSE :: Tries for first object discovery when QualNet joins the
//            exercise after other federates have joined.
// PARAMETERS :: None.
// RETURN :: void : NULL.
// **/
void VRLink::TryForFirstObjectDiscovery()
{
    try
    {
        SetSimulationTime();
        exConn->drainInput();
    }
    DtCATCH_AND_WARN(cout);
}

// /**
//FUNCTION :: GetNewEventScheduled
//PURPOSE :: Selector to return the value of newEventScheduled parameter.
//PARAMETERS :: None
//RETURN :: bool : Value of newEventScheduled parameter.
// **/
bool VRLink::GetNewEventScheduled()
{
    return newEventScheduled;
}

// /**
//FUNCTION :: EnableNewEventScheduled
//PURPOSE :: Enables the parameter newEventScheduled.
//PARAMETERS :: None
//RETURN :: void : NULL.
// **/
void VRLink::EnableNewEventScheduled()
{
    newEventScheduled = true;
}

// /**
//FUNCTION :: GetExConn
//PURPOSE :: Returns the pointer to the exercise connection.
//PARAMETERS :: None
//RETURN :: DtExerciseConn* : Pointer to the exercise connection.
// **/
DtExerciseConn* VRLink::GetExConn()
{
    return exConn;
}

// /**
//FUNCTION :: DisplayEntities
//PURPOSE :: Displays the entities parsed from the .hla-entities file.
//PARAMETERS :: None
//RETURN :: void : NULL.
// **/
void VRLink::DisplayEntities()
{
    hash_map <string, VRLinkEntity*>::iterator entIter;

    for (entIter = entities.begin(); entIter != reflectedEntities.end();
                                                                   entIter++)
    {
        entIter->second->DisplayData();
    }
}

// /**
//FUNCTION :: DisplayReflEntities
//PURPOSE :: Displays the reflected entities.
//PARAMETERS :: None
//RETURN :: void : NULL.
// **/
void VRLink::DisplayReflEntities()
{
    hash_map <string, VRLinkEntity*>::iterator reflEntIter;
    DtReflectedEntity* ent = refEntityList->first();

    while (ent)
    {
        cout<<"\n**************Entity State Repository Data**************\n";
        ent->esr()->printData();
        cout<<"\n*******************VRLinkEntity Data*******************\n";
        reflEntIter =reflectedEntities.find(ent->esr()->entityId().string());

        if (reflEntIter != reflectedEntities.end())
        {
            reflEntIter->second->DisplayData();
            cout<<"\n******************************************************";
            cout<<"**\n";
        }
        else
        {
            VRLinkReportWarning("Reflected entity not found",
                                __FILE__, __LINE__);
        }
        ent = ent->next();
    }
}

// /**
//FUNCTION :: DisplayRadios
//PURPOSE :: Displays the radios parsed from the .hla-radios file.
//PARAMETERS :: None
//RETURN :: void : NULL.
// **/
void VRLink::DisplayRadios()
{
    hash_map <NodeId, VRLinkRadio*>::iterator radiosIter;

    for (radiosIter = radios.begin(); radiosIter != radios.end();
                                                                radiosIter++)
    {
        radiosIter->second->DisplayData();
    }
}

// /**
//FUNCTION :: DisplayReflRadios
//PURPOSE :: Displays the reflected radios.
//PARAMETERS :: None
//RETURN :: void : NULL.
// **/
void VRLink::DisplayReflRadios()
{
    map <RadioIdKey, VRLinkRadio*>::iterator reflRadiosIter;

    DtReflectedRadioTransmitter* radioTx = refRadioTxList->first();

    while (radioTx)
    {
        cout<<"\n**************RadioTx State Repository Data**************";
        cout<<"\n";
        radioTx->tsr()->printData();
        cout<<"\n******************************************************";
        cout<<"**\n";
        radioTx = radioTx->next();
    }

    cout<<"\n*******************VRLinkRadio Data*******************\n";

    for (reflRadiosIter = reflRadioIdToRadioMap.begin();
         reflRadiosIter != reflRadioIdToRadioMap.end(); reflRadiosIter++)
    {
        reflRadiosIter->second->DisplayData();
        cout<<"\n******************************************************";
        cout<<"**\n";
    }
}

// /**
//FUNCTION :: CopyToOffsetEntityId
//PURPOSE :: Inserts the entity id in the datum value.
//PARAMETERS ::
// dst : void* : Datum value pointer.
// offset : unsigned& : Position at which entity id is to be inserted in the
//                      datum value.
// srcId : const DtEntityIdentifier& : EntityId to be copied.
//RETURN :: void : NULL.
// **/
void VRLink::CopyToOffsetEntityId(
    void* dst,
    unsigned& offset,
    const DtEntityIdentifier& srcId)
{
    const unsigned short siteId = (unsigned short)srcId.site();
    const unsigned short applicationId = (unsigned short)srcId.host();
    const unsigned short entityNumber = (unsigned short)srcId.entityNum();

    CopyToOffset(
        dst, offset, &siteId, sizeof(unsigned short));
    CopyToOffset(
        dst, offset, &applicationId, sizeof(unsigned short));
    CopyToOffset(
        dst, offset, &entityNumber, sizeof(unsigned short));
}

// /**
//FUNCTION :: CopyToOffset
//PURPOSE :: Copies the value pointed to by src to dst.
//PARAMETERS ::
// dst : void* : Datum value pointer.
// offset : unsigned& : Position at which src is to be inserted in the
//                      datum value.
// src : const void* : Pointer to data to be copied in datum value.
// size : unsigned : Size of the src in bytes.
//RETURN :: void : NULL.
// **/
void VRLink::CopyToOffset(
    void* dst,
    unsigned& offset,
    const void* src,
    unsigned size)
{
    unsigned char* uchar_dst = (unsigned char*) dst;

    memcpy(&uchar_dst[offset], src, size);
    offset += size;
}

// /**
//FUNCTION :: CopyUInt32ToOffset
//PURPOSE :: Copies the value pointed to by src to dst.
//PARAMETERS ::
// dst : void* : Datum value pointer.
// offset : unsigned& : Position at which src is to be inserted in the
//                      datum value.
// src : UInt32* : Pointer to data to be copied in datum value.
//RETURN :: void : NULL.
// **/
void VRLink::CopyUInt32ToOffset(
    void* dst,
    unsigned& offset,
    UInt32* src)
{
    unsigned char* uchar_dst = (unsigned char*) dst;
    char srcInChar[MAX_STRING_LENGTH] = "";

    sprintf(srcInChar, "%d", *src);
    memcpy(&uchar_dst[offset], srcInChar, strlen(srcInChar));
    offset += strlen(srcInChar);
}

// /**
//FUNCTION :: CopyBoolToOffset
//PURPOSE :: Copies the value pointed to by src to dst.
//PARAMETERS ::
// dst : void* : Datum value pointer.
// offset : unsigned& : Position at which src is to be inserted in the
//                      datum value.
// src : bool* : Pointer to data to be copied in datum value.
//RETURN :: void : NULL.
// **/
void VRLink::CopyBoolToOffset(
    void* dst,
    unsigned& offset,
    bool* src)
{
    if (*src)
    {
        CopyToOffset(dst, offset, "true", strlen("true"));
    }
    else
    {
        CopyToOffset(dst, offset, "false", strlen("false"));
    }
}

// /**
//FUNCTION :: CopyDoubleToOffset
//PURPOSE :: Copies the value pointed to by src to dst.
//PARAMETERS ::
// dst : void* : Datum value pointer.
// offset : unsigned& : Position at which src is to be inserted in the
//                      datum value.
// src : double* : Pointer to data to be copied in datum value.
//RETURN :: void : NULL.
// **/
void VRLink::CopyDoubleToOffset(
    void* dst,
    unsigned& offset,
    double* src)
{
    unsigned char* uchar_dst = (unsigned char*) dst;
    char srcInChar[MAX_STRING_LENGTH] = "";

    sprintf(srcInChar, "%lf", *src);
    memcpy(&uchar_dst[offset], srcInChar, strlen(srcInChar));
    offset += strlen(srcInChar);
}

// /**
//FUNCTION :: VRLinkEntityCbUsrData
//PURPOSE :: Initialization function for VRLinkEntityCbUsrData
//PARAMETERS ::
// + vrLinkPtr : VRLink* : VRLink pointer.
// + entityPtr : VRLinkEntity* : VRLink entity pointer.
// **/
VRLinkEntityCbUsrData::VRLinkEntityCbUsrData(
    VRLink* vrLinkPtr,
    VRLinkEntity* entityPtr)
{
    vrLink = vrLinkPtr;
    entity = entityPtr;
}

// /**
//FUNCTION :: GetData
//PURPOSE :: Selector to return the variables of class VRLinkEntityCbUsrData.
//PARAMETERS ::
// + vrLinkPtr : VRLink** : VRLink pointer
// + entityPtr : VRLinkEntity* : VRLink entity pointer.
//RETURN :: void : NULL.
// **/
void VRLinkEntityCbUsrData::GetData(
    VRLink** vrLinkPtr,
    VRLinkEntity** entityPtr)
{
    *vrLinkPtr = vrLink;
    *entityPtr = entity;
}

// /**
//FUNCTION :: VRLinkRadioCbUsrData
//PURPOSE :: Initialization function for VRLinkRadioCbUsrData
//PARAMETERS ::
// + vrLinkPtr : VRLink* : VRLink pointer.
// + radioPtr : VRLinkRadio* : VRLink radio pointer.
// **/
VRLinkRadioCbUsrData::VRLinkRadioCbUsrData(
    VRLink* vrLinkPtr,
    VRLinkRadio* radioPtr)
{
    vrLink = vrLinkPtr;
    radio = radioPtr;
}

// /**
//FUNCTION :: GetData
//PURPOSE :: Selector to return the variables of class VRLinkRadioCbUsrData.
//PARAMETERS ::
// + vrLinkPtr : VRLink** : VRLink pointer
// + radioPtr : VRLinkRadio* : VRLink radio pointer.
//RETURN :: void : NULL.
// **/
void VRLinkRadioCbUsrData::GetData(
    VRLink** vrLinkPtr,
    VRLinkRadio** radioPtr)
{
    *vrLinkPtr = vrLink;
    *radioPtr = radio;
}

// /**
// FUNCTION :: AppProcessSendRtssEvent
// PURPOSE :: Virtual function to handle the event type
//            MSG_EXTERNAL_VRLINK_SendRtss.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN :: void : NULL
// **/
void VRLink::AppProcessSendRtssEvent(Node* node)
{
}

// /**
// FUNCTION :: SendRtssNotification
// PURPOSE :: Sends rtss notification to another federate.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN :: void : NULL
// **/
void VRLink::SendRtssNotification(Node* node)
{
    if (simIface->IsMilitaryLibraryEnabled())
    {
        hash_map<NodeId, VRLinkData*>::iterator nodeDataIter;

        nodeDataIter = nodeIdToPerNodeDataHash.find(simIface->GetNodeId(node));

        if (nodeDataIter == nodeIdToPerNodeDataHash.end())
        {
            // Node is not DIS enabled.
            return;
        }

        VRLinkData* vrlinkData = nodeDataIter->second;

        VRLinkVerify(
            vrlinkData,
            "VRLinkData pointer not found",
            __FILE__, __LINE__);

        DtDataInteraction pdu;
        //DtPduHeader& pduHeader = *(pdu.netHeader());
        DtDatumRetCode retcode;

        const VRLinkRadio& radio = *vrlinkData->GetRadioPtr();

        VRLinkVerify(&radio, "Radio not found", __FILE__, __LINE__);

        pdu.setSenderId(radio.GetEntityPtr()->GetEntityId());
        pdu.setNumVarFields(1);

        pdu.setDatumParam(
                DtVAR,
                1,
                readyToSendSignalNotificationDatumId,
                &retcode);

        if (retcode == DtDATUM_BAD_INDEX)
        {
            VRLinkReportError(
                 "Invalid datum index or data",
                 __FILE__, __LINE__);
        }

        unsigned char varDatumValue[g_DatumValueBufSize];
        unsigned char* vdv = varDatumValue;
        unsigned vdvOffset = 0;

        UInt16 site = pdu.senderId().site();
        UInt16 host = pdu.senderId().host();
        UInt16 num = pdu.senderId().entityNum();

        if (DEBUG_NOTIFICATION)
        {
            cout << "Sending RTSS Notification: { \nSender(site: " << site << 
                " host: " << host << " num: " << num << ")\n";
        }

        simIface->EXTERNAL_hton(&site, sizeof(site));    
        simIface->EXTERNAL_hton(&host, sizeof(host));    
        simIface->EXTERNAL_hton(&num, sizeof(num));

        CopyToOffset(vdv, vdvOffset, &site, sizeof(site));
        CopyToOffset(vdv, vdvOffset, &host, sizeof(host));
        CopyToOffset(vdv, vdvOffset, &num, sizeof(num));

        UInt16 radioIndex = radio.GetRadioIndex();
        UInt32 timestamp = GetTimestamp();
        Float32 windowTime = 0.0;

        if (DEBUG_NOTIFICATION)
        {
            cout << "radioIndex: " << radioIndex << " timestamp: " << timestamp
                << " windowTime: " << windowTime << "\n";
        }

        simIface->EXTERNAL_hton(&radioIndex, sizeof(radioIndex));
        CopyToOffset(vdv, vdvOffset, &radioIndex, sizeof(radioIndex));

        
        simIface->EXTERNAL_hton(&timestamp, sizeof(timestamp));
        CopyToOffset(
            vdv,
            vdvOffset,
            &timestamp,
            sizeof(timestamp));

        
        simIface->EXTERNAL_hton(&windowTime, sizeof(windowTime));
        CopyToOffset(vdv, vdvOffset, &windowTime, sizeof(windowTime));

        pdu.setVarDataBytes(1, vdvOffset, &retcode);

        if (retcode == DtDATUM_BAD_INDEX)
        {
            VRLinkReportError("Invalid datum index or data", __FILE__, __LINE__);
        }

        pdu.setDatumValByteArray(
                DtVar,
                1,
                (const char*)vdv,
                &retcode);

        if (retcode == DtDATUM_BAD_INDEX)
        {
            VRLinkReportError(
                "Invalid datum index or data",
                __FILE__, __LINE__);
        }

        exConn->sendStamped(pdu);

        if (debug || debugPrintComms)
        {
            if (DEBUG_NOTIFICATION)
            {
                cout << "Size of RTSS Notification: " << vdvOffset << "\n}\n";
            }
            PrintSentRtssNotification(node);
        }
    }
}

// /**
// FUNCTION :: ScheduleRtssNotification
// PURPOSE :: Schedules the rtss notification to be sent to another federate.
// PARAMETERS ::
// + radio : VRLinkRadio* : VRLink radio pointer.
// + delay : clocktype : Delay with which rtss notification will be
//                       scheduled.
// RETURN :: void : NULL
// **/
void VRLink::ScheduleRtssNotification(
    VRLinkRadio* radio,
    clocktype delay)
{
    if (simIface->IsMilitaryLibraryEnabled())
    {
        Node* node = radio->GetNode();

        Message* msg = simIface->AllocateMessage(
                                     node,
                                     simIface->GetExternalLayerId(),
                                     simIface->GetVRLinkExternalInterfaceType(),
                                     simIface->GetMessageEventType(EXTERNAL_VRLINK_SendRtss));

        VRLinkVerify(
            (delay >= 0),
            "Message send delay cannot be negative",
            __FILE__, __LINE__);
        
        simIface->AllocateInfoInMsg (
                node,
                msg,
                sizeof(RtssForwardedInfo));

        RtssForwardedInfo * rtssForwardedInfo = (RtssForwardedInfo *)
            simIface->ReturnInfoFromMsg(msg);

        rtssForwardedInfo->nodeId = simIface->GetNodeId(node);

        //Use external remote send if not even if its already on p0 just for simplicity
        simIface->RemoteExternalSendMessage (iface, simIface->GetRealPartitionIdForNode(node),
                msg, delay, simIface->GetExternalScheduleSafeType());

    }
}

// /**
// FUNCTION :: PrintSentRtssNotification
// PURPOSE :: Prints the rtss notification.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN :: void : NULL
// **/
void VRLink::PrintSentRtssNotification(Node* node)
{
    if (simIface->IsMilitaryLibraryEnabled())
    {
        hash_map<NodeId, VRLinkData*>::iterator nodeDataIter;

        nodeDataIter = nodeIdToPerNodeDataHash.find(simIface->GetNodeId(node));

        VRLinkVerify(
            nodeDataIter != nodeIdToPerNodeDataHash.end(),
            "Entry not found in nodeIdToPerNodeDataHash",
            __FILE__, __LINE__);

        VRLinkData* vrlinkData = nodeDataIter->second;

        VRLinkVerify(
            vrlinkData,
            "VRLinkData pointer not found",
            __FILE__, __LINE__);

        VRLinkRadio& radio = *vrlinkData->GetRadioPtr();

        VRLinkVerify(
            &radio,
            "Radio not found",
            __FILE__, __LINE__);

        char clocktypeString[g_ClocktypeStringBufSize];
        simIface->PrintClockInSecond(simIface->GetCurrentSimTime(node), clocktypeString);

        if ((vrLinkType == VRLINK_TYPE_HLA13) ||
            (vrLinkType == VRLINK_TYPE_HLA1516))
        {
            cout << "VRLINK: RTSS, "
                    << simIface->GetNodeId(radio.GetNode()) << " ("
                    << radio.GetEntityPtr()->GetEntityIdString()
                    << ") " << radio.GetEntityPtr()->GetMarkingData()
                    << ", time " << clocktypeString << endl;
        }
        else if (debugPrintComms)
        {
            cout << "VRLINK: RTSS, "
                    << radio.GetEntityPtr()->GetMarkingData() << " ("
                    << simIface->GetNodeId(radio.GetNode()) << ","
                    << radio.GetRadioIndex()
                    << "), time " << clocktypeString << endl;
        }
    }
}

// /**
// FUNCTION :: SetEntityPtr
// PURPOSE :: Selector to set the entity associated with the radio.
// PARAMETERS ::
// + entity : VRLinkEntity* : VRLink entity pointer.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::SetEntityPtr(VRLinkEntity* entity)
{
    entityPtr = entity;
}

// /**
// FUNCTION :: SetNetworkPtr
// PURPOSE :: Selector to set the network pointer of the radio.
// PARAMETERS ::
// + network : VRLinkNetwork* : VRLink network pointer.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::SetNetworkPtr(VRLinkNetwork* network)
{
    networkPtr = network;
}

// /**
// FUNCTION :: SetDefaultDstRadioPtr
// PURPOSE :: Sets the default destination radio pointer for a radio.
// PARAMETERS :: None.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::SetDefaultDstRadioPtr()
{

    VRLinkVerify(
        (defaultDstRadioPtr == NULL),
        "Default destination radio already set",
        __FILE__, __LINE__);

    if (!networkPtr->IsUnicast())
    {
        return;
    }

    defaultDstRadioPtr = networkPtr->GetDefaultDstRadioPtrForRadio(this);

    if (!defaultDstRadioPtr)
    {
        char errorString[MAX_STRING_LENGTH];

        sprintf(errorString,
                "Can't determine default destination for node %u",
                simIface->GetNodeId(node));

        VRLinkReportError(errorString, __FILE__, __LINE__);
    }
}

// /**
// FUNCTION :: SetEntityId
// PURPOSE :: Selector to set the entityId for an entity associated with a
//            radio.
// PARAMETERS ::
// + entId : const DtEntityIdentifier& : EntityId to be set.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::SetEntityId(const DtEntityIdentifier& entId)
{
    if (entityId == DtEntityIdentifier(0, 0, 0))
    {
        entityId = entId;
    }
}

// /**
// FUNCTION :: ReflectAttributes
// PURPOSE :: Updates the radio attributes received from an outside federate.
// PARAMETERS ::
// + radioTx : DtReflectedRadioTransmitter* : Reflected radio transmitter.
// + vrLink : VRLink* : VRLink pointer.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::ReflectAttributes(
    DtReflectedRadioTransmitter* radioTx,
    VRLink* vrLink)
{
    DtRadioTransmitterRepository* tsr = radioTx->tsr();

    ReflectOperationalStatus(tsr->transmitState(), vrLink);
}

// /**
// FUNCTION :: ReflectOperationalStatus
// PURPOSE :: Updates the radio's operational status received from an outside
//            federate.
// PARAMETERS ::
// + reflTxOperationalStatus : const DtRadioTransmitState& : Reflected
//                                                        operational status.
// + vrLink : VRLink* : VRLink pointer.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::ReflectOperationalStatus(
    const DtRadioTransmitState& reflTxOperationalStatus,
    VRLink* vrLink)
{
    if (reflTxOperationalStatus == txOperationalStatus)
    {
        return;
    }

    VRLinkVerify(
        reflTxOperationalStatus >= DtOff
        && reflTxOperationalStatus <= DtOn,
        "Unexpected attribute value",
        __FILE__, __LINE__);

    VRLinkVerify(
        (entityPtr != NULL),
        "Entity not specified for radio",
        __FILE__, __LINE__);

    VRLinkVerify(
        (txOperationalStatus <= DtOn),
        "invalid current radio tx state",
        __FILE__, __LINE__);

    VRLinkVerify(
        entityPtr,
        "Entity associated with radio not found",
        __FILE__, __LINE__);

    // Display full information if debugging is on; otherwise, just display
    // on/off changes to TransmitterOperationalStatus.

    if (vrLink->GetDebugPrintTransmitterPdu())
    {
        cout << "VRLINK: Transmitter PDU (" << entityPtr->GetEntityIdString() \
             <<  ", "<< radioIndex << ")" << endl;
    }

    if (vrLink->GetDebug() || vrLink->GetDebugPrintTxState())
    {
        if (vrLink->GetDebug())
        {
            cout << "VRLINK TX STATE: (" << entityPtr->GetEntityIdString()
                 << ", " << radioIndex << ") " << entityPtr->GetMarkingData()
                 << " TransmitterOperationalStatus = ";
        }
        else if (vrLink->GetDebugPrintTxState())
        {
            cout << "VRLINK TX STATE: " << entityPtr->GetMarkingData() << " ("
                 << simIface->GetNodeId(node) << ","
                 << radioIndex
                 << ") Transmit State = ";
        }

        switch (reflTxOperationalStatus)
        {
            case DtOff:
                cout << "Off";
                break;
            case DtRadioOnNotTransmitting:
                cout << "OnButNotTransmitting";
                break;
            case DtOn:
                cout << "OnAndTransmitting";
                break;
            default:
                VRLinkReportError(
                "Invalid tx operational status encountered",
                __FILE__, __LINE__);
        }
        cout << endl;
    }
    else if (((vrLink->GetType() == VRLINK_TYPE_HLA13) ||
         (vrLink->GetType() == VRLINK_TYPE_HLA1516)) &&
        (((reflTxOperationalStatus == DtOff) &&
          (txOperationalStatus != DtOff))
         ||
        ((reflTxOperationalStatus != DtOff) &&
         (txOperationalStatus == DtOff))))
    {
        cout << "VRLINK: (" << entityPtr->GetEntityIdString()
                << ", " << radioIndex << ") " << entityPtr->GetMarkingData()
                << " TransmitterOperationalStatus = ";

        switch (txOperationalStatus)
        {
            case DtOff:
                cout << "Off";
                break;
            case DtRadioOnNotTransmitting:
            case DtOn:
                cout << "(On)";
                break;
            default:
                VRLinkReportError(
                    "Invalid tx operational status encountered",
                    __FILE__, __LINE__);
        }
        cout << endl;
    }

    if ((reflTxOperationalStatus != DtOff) && (txOperationalStatus != DtOff))
    {
        // Radio was on, and is still on, but just a different mode.
    }
    else if (reflTxOperationalStatus == DtOff)
    {
        // Radio has been turned off.
        if (entityPtr->GetDamageState() != DtDamageDestroyed)
        {
            ScheduleChangeInterfaceState(false);
        }
    }
    else
    {
        // Radio has been turned on.
        if (entityPtr->GetDamageState() != DtDamageDestroyed)
        {
            ScheduleChangeInterfaceState(true);
        }
    }

    txOperationalStatus = reflTxOperationalStatus;
}

// /**
// FUNCTION :: CbReflectAttributeValues
// PURPOSE :: Callback used to reflect the attribute updates received from an
//            outside federate.
// PARAMETERS ::
// + reflRadio : DtReflectedRadioTransmitter* : Reflected radio transmitter.
// + usr : void* : User data.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::CbReflectAttributeValues(
    DtReflectedRadioTransmitter* reflRadio,
    void* usr)
{
    VRLinkVerify(
        reflRadio,
        "NULL reflected radio tx data received",
        __FILE__, __LINE__);

    VRLink* vrLink = NULL;
    VRLinkRadio* radio = NULL;

    VRLinkRadioCbUsrData* usrData = (VRLinkRadioCbUsrData*)usr;

    if (usrData)
    {
        usrData->GetData(&vrLink, &radio);
    }

    if ((radio) && (vrLink))
    {
        radio->ReflectAttributes(reflRadio, vrLink);
    }
}

// /**
// FUNCTION :: SetLatLonAltScheduled
// PURPOSE :: Sets the scheduled latLonAlt values.
// PARAMETERS ::
// + entity : VRLinkEntity* : VRLink entity pointer.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::SetLatLonAltScheduled(VRLinkEntity* entity)
{
    //combine entity gcc position and the relative gcc position of the 
    //radio, then convert the sum to lat lon alt
    ConvertGccToLatLonAlt(entity->GetXYZ() + relativePosition, latLonAltScheduled);
}

// /**
// FUNCTION :: SetLatLonAltScheduled
// PURPOSE :: Sets the scheduled latLonAlt values.
// PARAMETERS ::
// + latLonAltSch : DtVector : LatLonAlt values to be set.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::SetLatLonAltScheduled(DtVector latLonAltSch)
{
    latLonAltScheduled = latLonAltSch;
}

// /**
// FUNCTION :: MapDefaultDstRadioPtrsToRadios
// PURPOSE :: Assigns the first node in the same network as the default
//            destination radio node. This node is used when the
//            Commmunication Effects Request does not specify a receiver.
// PARAMETERS :: None.
// RETURN :: void : NULL.
// **/
void VRLink::MapDefaultDstRadioPtrsToRadios()
{
    // TODO - new feature:  The default destination can be specified as a nodeId 
    // in the .vrlink-radios file.  Right now it just uses the first node that 
    // is not the radio in question in the network node list

    hash_map <NodeId, VRLinkRadio*>::iterator radiosIter;

    for (radiosIter = radios.begin(); radiosIter != radios.end();
                                                                ++radiosIter)
    {
        VRLinkRadio* srcRadio = radiosIter->second;

        if (srcRadio->GetNetworkPtr() == NULL)
        {
            char warningString[MAX_STRING_LENGTH];
            sprintf(warningString,
                    "node %u does not have an associated network",
                    simIface->GetNodeId(srcRadio->GetNode()));
            VRLinkReportWarning(warningString, __FILE__, __LINE__);
            continue;
        }

        srcRadio->SetDefaultDstRadioPtr();
    }
}

// /**
// FUNCTION :: MapHierarchyIds
// PURPOSE :: Sets the hierarchy data for an entity.
// PARAMETERS ::
// + iface : EXTERNAL_Interface* : External interface pointer.
// RETURN :: void : NULL.
// **/
void VRLink::MapHierarchyIds(EXTERNAL_Interface* iface)
{
    VRLinkVerify(
        (simIface->GetPartitionForIface(iface) != NULL),
        "Partition data for interface not found",
        __FILE__, __LINE__);

    VRLinkVerify(
        (simIface->GetNodeInputFromIface(iface) != NULL),
        "NodeInput data for interface not found",
        __FILE__, __LINE__);

    // Fill hash table of nodeId -> hierarchy IDs.

    NodeInput* nodeInput = simIface->GetNodeInputFromIface(iface);
    hash_map<NodeId, int> nodeToHiereachyId;
    hash_map<NodeId, int>::iterator nodeToHiereachyIdIter;

    VRLinkVerify(
        (simIface->GetNumLinesOfNodeInput(nodeInput) >= 0),
        "Cannot read data from *.config file",
        __FILE__, __LINE__);

    unsigned i;

    for (i = 0; i < (unsigned) simIface->GetNumLinesOfNodeInput(nodeInput); i++)
    {
        char* value;
        char* variableName;

        simIface->GetNodeInputDetailForIndex(nodeInput, i, &variableName, &value);

        if (strcmp(variableName, "COMPONENT") != 0)
        { continue;  }

        errno = 0;
        int hierarchyId = (int) strtol(value, NULL, 10);
        VRLinkVerify(errno == 0, "Can't parse hierarchy ID .config file");

        if (hierarchyId == 0)
        {
            continue;
        }

        const char* p = strchr(value, '{');

        VRLinkVerify(p != NULL && p[1] != 0,
                     "Invalid COMPONENT value .config file");

        const char* next = p + 1;  // Points to character after '{'.
        char nodeIdList[g_lineBufSize];
        unsigned numBytesCopied;
        bool foundEmptyField;
        bool overflowed;

        p = simIface->GetTokenOrEmptyField(
                        nodeIdList,
                        sizeof(nodeIdList),
                        numBytesCopied,
                        overflowed,
                        next,
                        "}",
                        foundEmptyField);

        if (numBytesCopied == 0)
        {
            // Hierarchy has no associated nodeIds.
            continue;
        }

        VRLinkVerify(p != NULL && numBytesCopied + 1 < sizeof(nodeIdList),
                     "Invalid COMPONENT value .config file");

        char nodeIdString[g_lineBufSize];
        next = nodeIdList;

        while (1)
        {
            p = simIface->GetToken(
                            nodeIdString,
                            sizeof(nodeIdString),
                            numBytesCopied,
                            overflowed,
                            next,
                            " \t,");

            if (p == NULL) { break; }

            VRLinkVerify(
                numBytesCopied + 1 < sizeof(nodeIdString),
                "nodeId string too long .config file");

            if (!isdigit(nodeIdString[0])) { continue; }

            errno = 0;
            unsigned nodeId = (unsigned) strtoul(nodeIdString, NULL, 10);

            VRLinkVerify(errno == 0, "Can't parse nodeId .config file");

            nodeToHiereachyIdIter = nodeToHiereachyId.find(nodeId);

            VRLinkVerify(
                nodeToHiereachyIdIter == nodeToHiereachyId.end(),
                "nodeId belongs to more than one hierachy",
                ".config file");

            nodeToHiereachyId[nodeId] = hierarchyId;
        }
    }

    if (nodeToHiereachyId.empty())
    {
        return;
    }
   

    hash_map <string, VRLinkEntity*>::iterator entitiesIter;

    if (entities.empty())
    {
        // no external entities found
        return;        
    }

    for (entitiesIter = entities.begin(); entitiesIter != entities.end();
                                                              entitiesIter++)
    {
        VRLinkEntity& entity = *entitiesIter->second;
        entity.SetHierarchyData(nodeToHiereachyId);
    }
}

// /**
// FUNCTION :: GoingToSleep
// PURPOSE :: To suspend the execution of QualNet for a specified time.
// PARAMETERS ::
// + seconds : unsigned : Sleep duration.
// RETURN :: void : NULL.
// **/
void VRLink::GoingToSleep(unsigned seconds)
{
#ifdef _WIN32
    // Win32 Sleep() sleeps in milliseconds.
    Sleep((DWORD) seconds * 1000);
#else /* _WIN32 */
    sleep(seconds);
#endif /* _WIN32 */
}

// /**
// FUNCTION :: GetTimestamp
// PURPOSE :: To return the current timestamp.
// PARAMETERS :: None.
// RETURN :: unsigned : Timestamp.
// **/
unsigned VRLink::GetTimestamp()
{
    return ConvertDoubleToTimestamp(GetNumSecondsPastHour(), 1);
}

// /**
// FUNCTION :: ConvertDoubleToTimestamp
// PURPOSE :: Convert seconds to VRLink timestamp.
// PARAMETERS ::
// + double_timestamp : double : Value to be converted to timestamp.
// + absolute : bool : TRUE/FALSE depending on whether timestamp to be
//                     returned is absolute or not.
// RETURN :: unsigned : Timestamp.
// **/
unsigned VRLink::ConvertDoubleToTimestamp(
    double double_timestamp,
    bool absolute)
{
    // It is NOT guaranteed that converting from one type to the other,
    // then back to the original type, will provide the original value.

    // VRLink timestamps are flipped to 0 after one hour (3600 seconds).
    VRLinkVerify(
        (double_timestamp < 3600.0),
        "Invalid timestamp",
        __FILE__, __LINE__);

    unsigned timestamp = (unsigned) (double_timestamp * g_TimestampRatio);

    // Check high-order bit is still 0.

     VRLinkVerify(
         ((timestamp & 0x80000000) == 0),
         "Higher order bit not zero in timestamp",
         __FILE__, __LINE__);

    timestamp <<= 1;

    if (absolute) { return timestamp |= 1; }
    else { return timestamp; }
}

// /**
// FUNCTION :: GetNumSecondsPastHour
// PURPOSE :: Returns the current time (in sec).
// PARAMETERS :: None.
// RETURN :: double : Current time.
// **/
double VRLink::GetNumSecondsPastHour()
{
    time_t timeValue;
    unsigned numMicroseconds;

#ifdef _WIN32
    _timeb ftimeValue;
    _ftime(&ftimeValue);
    timeValue = ftimeValue.time;
    numMicroseconds = ftimeValue.millitm * 1000;
#else /* _WIN32 */
    timeval gettimeofdayValue;
    gettimeofday(&gettimeofdayValue, NULL);
    timeValue = (time_t) gettimeofdayValue.tv_sec;
    numMicroseconds = gettimeofdayValue.tv_usec;
#endif /* _WIN32 */

    tm* localtimeValue = localtime(&timeValue);

    double numSecondsPastHour
        = ((double) ((localtimeValue->tm_min * 60) + localtimeValue->tm_sec))
          + ((double) numMicroseconds) / 1e6;

    return numSecondsPastHour;
}

// /**
//FUNCTION :: GetXyzEpsilon
//PURPOSE :: Selector to return the value of xyzEpsilon parameter.
//PARAMETERS :: None
//RETURN :: double : Value of xyzEpsilon parameter.
// **/
double VRLink::GetXyzEpsilon()
{
    return xyzEpsilon;
}

void VRLink::DeregisterCallbacks()
{
    refEntityList->removeEntityAdditionCallback(
                       VRLink::CbEntityAdded,
                       this);

    refEntityList->removeEntityRemovalCallback(
                       VRLink::CbEntityRemoved,
                       this);

    refRadioTxList->removeRadioTransmitterAdditionCallback(
                        VRLink::CbRadioAdded,
                        this);

    refRadioTxList->removeRadioTransmitterRemovalCallback(
                        VRLink::CbRadioRemoved,
                        this);

    DtSignalInteraction::removeCallback(
                             exConn,
                             VRLink::CbSignalIxnReceived,
                             this); 
}

// /**
// FUNCTION :: ~VRLink
// PURPOSE :: Destructor of Class VRLink.
// PARAMETERS :: None.
// **/
VRLink::~VRLink()
{
    hash_map <string, VRLinkEntity*>::iterator entitiesIter;

    //free entity data
    for (entitiesIter= entities.begin(); entitiesIter != entities.end();
                                                              ++entitiesIter)
    {
        delete entitiesIter->second;
    }
    entities.erase(entities.begin(), entities.end());

    //free radio data
    hash_map <NodeId, VRLinkRadio*>::iterator radiosIter;

    for (radiosIter= radios.begin(); radiosIter != radios.end();
                                                                ++radiosIter)
    {
        delete radiosIter->second;
    }
    radios.erase(radios.begin(), radios.end());

    //free network data
    hash_map <string, VRLinkNetwork*>::iterator networksIter;

    for (networksIter = networks.begin(); networksIter != networks.end();
                                                              ++networksIter)
    {
        delete networksIter->second;
    }
    networks.erase(networks.begin(), networks.end());

    //free other hash data
    reflectedEntities.erase(reflectedEntities.begin(),
                            reflectedEntities.end());

    reflRadioIdToRadioMap.erase(reflRadioIdToRadioMap.begin(),
                                 reflRadioIdToRadioMap.end());

    reflGlobalIdToRadioHash.erase(reflGlobalIdToRadioHash.begin(),
                                  reflGlobalIdToRadioHash.end());

    hash_map<NodeId, VRLinkData*>::iterator nodeIdToPerNodeDataIter;

    for (nodeIdToPerNodeDataIter = nodeIdToPerNodeDataHash.begin();
         nodeIdToPerNodeDataIter != nodeIdToPerNodeDataHash.end();
         nodeIdToPerNodeDataIter++)
    {
        delete nodeIdToPerNodeDataIter->second;
    }

    nodeIdToPerNodeDataHash.erase(nodeIdToPerNodeDataHash.begin(),
                                  nodeIdToPerNodeDataHash.end());
}

// /**
//FUNCTION :: GetRadioPtr
//PURPOSE :: Selector to return the radio pointer present in the simulated
//           message information.
//PARAMETERS :: None
//RETURN :: const VRLinkRadio* : Radio pointer.
// **/
const VRLinkRadio* VRLinkData::GetRadioPtr() const
{
    return radioPtr;
}

// /**
//FUNCTION :: GetRadioPtr
//PURPOSE :: Selector to return the radio pointer present in the simulated
//           message information.
//PARAMETERS :: None
//RETURN :: VRLinkRadio* : Radio pointer.
// **/
VRLinkRadio* VRLinkData::GetRadioPtr()
{
    return radioPtr;
}

// /**
//FUNCTION :: GetNextMsgId
//PURPOSE :: Selector to return the next message id present in the simulated
//           message information.
//PARAMETERS :: None
//RETURN :: unsigned int : Next message id.
// **/
unsigned int VRLinkData::GetNextMsgId()
{
    return nextMsgId;
}

// /**
//FUNCTION :: SetRadioPtr
//PURPOSE :: Selector to set the radio pointer present in the simulated
//           message information.
//PARAMETERS ::
// + radio : VRLinkRadio* : VRLink radio pointer.
//RETURN :: void : NULL.
// **/
void VRLinkData::SetRadioPtr(VRLinkRadio* radio)
{
    radioPtr = radio;
}

// /**
// FUNCTION :: ~VRLinkData
// PURPOSE :: Destructor of Class VRLinkData.
// PARAMETERS :: None.
// **/
VRLinkData::~VRLinkData()
{
    radioPtr = NULL;
}

// /**
//FUNCTION :: SetNextMsgId
//PURPOSE :: Selector to set the next message id present in the simulated
//           message information.
//PARAMETERS ::
// + unsigned int : Next message id.
//RETURN :: void : NULL.
// **/
void VRLinkData::SetNextMsgId(unsigned int nextId)
{
    nextMsgId = nextId;
}

// /**
//FUNCTION :: VRLinkVerify
//PURPOSE :: Verifies the condition passed to be TRUE, and if found to be
//           FALSE, it asserts.
//PARAMETERS ::
// + condition : bool : Condition to be verified.
// + errorString : char* : Error message to be flashed in case the condition
//                         passed is found to be FALSE.
// + path : const char* : File from where the code asserts.
// + lineNumber : unsigned : Line number from where the code asserts.
//RETURN :: void : NULL.
// **/
void VRLinkVerify(
    bool condition,
    const char* errorString,
    const char* path,
    unsigned lineNumber)
{
    if (!condition)
    {
        VRLinkReportError(errorString, path, lineNumber);
    }
}

// /**
//FUNCTION :: VRLinkReportWarning
//PURPOSE :: Prints the warning message passed.
//PARAMETERS ::
// + warningString : const char* : Warning message to be printed.
// + path : const char* : File from where the code asserts.
// + lineNumber : unsigned : Line number from where the code asserts.
//RETURN :: void : NULL.
// **/
void VRLinkReportWarning(
    const char* warningString,
    const char* path,
    unsigned lineNumber)
{
    cout << "VRLink warning:";

    if (path != NULL)
    {
        cout << path << ":";

        if (lineNumber > 0)
        {
            cout << lineNumber << ":";
        }
    }

    cout << " " << warningString << endl;
}

// /**
//FUNCTION :: VRLinkReportError
//PURPOSE :: Prints the error message passed and exits code.
//PARAMETERS ::
// + errorString : const char* : Error message to be printed.
// + path : const char* : File from where the code asserts.
// + lineNumber : unsigned : Line number from where the code asserts.
//RETURN :: void : NULL.
// **/
void VRLinkReportError(
    const char* errorString,
    const char* path,
    unsigned lineNumber)
{
    cout << "VRLink error:";

    if (path != NULL)
    {
        cout << path << ":";

        if (lineNumber > 0)
        {
            cout << lineNumber << ":";
        }
    }

    cout << " " << errorString << endl;

    exit(EXIT_FAILURE);
}

// /**
//FUNCTION :: VRLinkGetIfaceDataFromNode
//PURPOSE :: Returns the external interface pointer extracted from the node
//           passed.
//PARAMETERS ::
// + node : Node* : Pointer to node.
//RETURN :: VRLinkExternalInterface* : External interface pointer.
// **/
VRLinkExternalInterface* VRLinkGetIfaceDataFromNode(Node* node)
{
    EXTERNAL_Interface* iface =simIface->GetExternalInterfaceForVRLink(node);
    return (VRLinkExternalInterface*) iface->data;
}

// /**
//FUNCTION :: VRLinkGetIfaceDataFromNode
//PURPOSE :: Get the Entity Identifier for a specific nodeId
//PARAMETERS ::
// + nodeId : NodeId : node Id used for searching
// + entId : DtEntityIdentifier : entity id associated with node id
//RETURN :: bool : FALSE if cannot find it, TRUE if it can.
// **/
bool VRLink::getEntityId(NodeId nodeId, DtEntityIdentifier &entId)
{
    hash_map <NodeId, DtEntityIdentifier>::const_iterator it = 
           nodeIdToEntityIdHash.find(nodeId);
    if (it == nodeIdToEntityIdHash.end())
    {
        
        return false;
    }
    entId = it->second;
    return true;
}

// /**
//FUNCTION :: GetInitialPhyTxPowerForRadio
//PURPOSE :: Find initial phy tx power for a specific radio and 
//             set the value onto the radio, sends message 
//                 if radio's node is on another partition
//PARAMETERS ::
// + radio : VRLinkRadio* : pointer to radio
//RETURN :: void : NULL
// **/
void VRLink::GetInitialPhyTxPowerForRadio(VRLinkRadio* radio)
{
    Node* node = radio->GetNode();
    
    if (simIface->GetRealPartitionIdForNode(node) == 0)
    {
        double txPower;
        simIface->GetExternalPhyTxPower(node, 0, &txPower);
        radio->SetInitialPhyTxPowerForRadio(txPower);
    }
    else
    {
        //schedule a MSG_EXTERNAL_VRLINK_InitialPhyMaxTxPowerRequest message
        // and send to external node
        Message* msg = simIface->AllocateMessage(node,
                             simIface->GetExternalLayerId(),
                             simIface->GetVRLinkExternalInterfaceType(),
                             simIface->GetMessageEventType(EXTERNAL_VRLINK_InitialPhyMaxTxPowerRequest));

        simIface->AllocateInfoInMsg(node, msg, sizeof(VRLinkGetInitialPhyTxPowerResposeInfo));
        VRLinkGetInitialPhyTxPowerResposeInfo& info
            = *((VRLinkGetInitialPhyTxPowerResposeInfo*) simIface->ReturnInfoFromMsg(msg));
        
        info.nodeId = simIface->GetNodeId(node);

        simIface->RemoteExternalSendMessage (iface, simIface->GetRealPartitionIdForNode(node),
                msg, 0, simIface->GetExternalScheduleSafeType());
    }
}

// /**
//FUNCTION :: AppProcessGetInitialPhyTxPowerRequest
//PURPOSE :: Message handler for initial phy tx power requests for remote nodes
//PARAMETERS ::
// + node : Node* : pointer to node
//RETURN :: void : NULL
// **/
void VRLink::AppProcessGetInitialPhyTxPowerRequest(Node* node, Message* msg)
{
    VRLinkGetInitialPhyTxPowerResposeInfo& info
        = *((VRLinkGetInitialPhyTxPowerResposeInfo*) simIface->ReturnInfoFromMsg(msg));

    Node* actualNode = simIface->GetNodePtrFromOtherNodesHash(
                        node, info.nodeId, false);
    double txPower;
    simIface->GetExternalPhyTxPower(actualNode, 0, &txPower);
    
    //schedule a MSG_EXTERNAL_VRLINK_InitialPhyMaxTxPowerResponse message
    // and send to p0
    Message* msg2 = simIface->AllocateMessage(actualNode,
                         simIface->GetExternalLayerId(),
                         simIface->GetVRLinkExternalInterfaceType(),
                         simIface->GetMessageEventType(EXTERNAL_VRLINK_InitialPhyMaxTxPowerResponse));

   
    simIface->AllocateInfoInMsg(actualNode, msg2, sizeof(VRLinkGetInitialPhyTxPowerResposeInfo));

    VRLinkGetInitialPhyTxPowerResposeInfo& info2
        = *((VRLinkGetInitialPhyTxPowerResposeInfo*) simIface->ReturnInfoFromMsg(msg2));
    
    info2.nodeId = info.nodeId;
    info2.txPower = txPower;

    simIface->RemoteExternalSendMessage (
        iface,
        0 /* p0 */,
        msg2,
        0,
        simIface->GetExternalScheduleSafeType());
}

// /**
//FUNCTION :: AppProcessGetInitialPhyTxPowerRequest
//PURPOSE :: Message handler for initial phy tx power response from remote nodes
//PARAMETERS ::
// + msg : Message* : pointer to return message from remote nodes
//RETURN :: void : NULL
// **/
void VRLink::AppProcessGetInitialPhyTxPowerResponse(Message* msg)
{
    VRLinkGetInitialPhyTxPowerResposeInfo& info
        = *((VRLinkGetInitialPhyTxPowerResposeInfo*) simIface->ReturnInfoFromMsg(msg));

    //Get the radio from the node Id and use power value from the message to set initialPhyTx
    VRLinkRadio* radio = radios[info.nodeId];
    radio->SetInitialPhyTxPowerForRadio(info.txPower);
}

// /**
//FUNCTION :: UpdateMetric
//PURPOSE :: Updates values of stored metrics sent by the GUI.
//PARAMETERS ::
// + unsigned : NodeId : node id of metric
// + metric : const MetricData : definition of metric being updated
// + metricValue : void* : pointer to new value of the metric
// + updateTime : clocktype : when this value update occurred
//RETURN :: void : NULL
// **/
void VRLink::UpdateMetric(unsigned nodeId,
        const MetricData& metric,
        void* metricValue,
        clocktype updateTime)
{
}
