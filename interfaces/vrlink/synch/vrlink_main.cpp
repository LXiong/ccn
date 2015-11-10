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
    char* scenarioName,
    char* connectionVar1,
    char* connectionVar2,
    char* connectionVar3,
    char* connectionVar4)
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

    vrlink->CreateFederation();
    vrlink->RegisterCallbacks();

    DtClock* clock = vrlink->GetExConn()->clock();
    clock->setSimTime(0);
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


// /**
// FUNCTION :: GetNextEntityStruct
// PURPOSE :: VR-Link's get next EntityStruct function from refEntity dll list
// PARAMETERS ::
// RETURN :: void : EntityStruct*
// **/
SNT_SYNCH::EntityStruct* VRLinkExternalInterface_Impl::GetNextEntityStruct()
{
    SNT_SYNCH::EntityStruct* entity = NULL;
    VRLinkEntity* refEntity = vrlink->GetNextReflectedEntity();

    if (refEntity)
    {
        entity = new SNT_SYNCH::EntityStruct;

        DtEntityIdentifier id = refEntity->GetId();

        entity->entityIdentifier.siteId = id.site();
        entity->entityIdentifier.applicationId = id.host();
        entity->entityIdentifier.entityNumber = id.entityNum();

        std::string myName;
#if DtDIS
        myName = id.string();
#else
        myName = refEntity->GetMyName();
#endif

        int len = myName.size();
        if (len)
        {
            entity->myName = (char*)malloc((len + 1) * sizeof(char));
            memcpy(entity->myName,
                   myName.c_str(),
                   len);
            entity->myName[len] = '\0';
        }

        std::string markingData = refEntity->GetMarkingData();

        len = markingData.size();
        if (len > 11)
        {
            memcpy(&entity->marking.markingData,
                   markingData.c_str(),
                   11);
            entity->marking.markingData[11] = 0;
        }
        else
        {
            memcpy(&entity->marking.markingData,
                   markingData.c_str(),
                   len);
            entity->marking.markingData[len] = 0;
        }


        switch (refEntity->GetForceType())
        {
            case DtForceFriendly:
            {
                entity->forceIdentifier =
                                    SNT_SYNCH::ForceIdentifierEnum::Friendly;
                break;
            }
            case DtForceOpposing:
            {
                entity->forceIdentifier =
                                    SNT_SYNCH::ForceIdentifierEnum::Opposing;
                break;
            }
            case DtForceNeutral :
            default: //DtForceOther
            {
                // Treat other ForceIDs as neutral forces.
                entity->forceIdentifier =
                                     SNT_SYNCH::ForceIdentifierEnum::Neutral;
                break;
            }
        }


        switch (refEntity->GetDamageState())
        {
            case DtDamageNone:
            {
                entity->damageState = SNT_SYNCH::DamageStatusEnum::NoDamage;
                break;
            }
            case DtDamageSlight:
            {
                entity->damageState =
                                   SNT_SYNCH::DamageStatusEnum::SlightDamage;
                break;
            }
            case DtDamageModerate :
            {
                entity->damageState =
                                 SNT_SYNCH::DamageStatusEnum::ModerateDamage;
                break;
            }
            default: //DtDamageDestroyed
            {
                entity->damageState = SNT_SYNCH::DamageStatusEnum::Destroyed;
                break;
            }
        }

        DtVector xyz = refEntity->GetXYZ();
        DtVector latLonAlt = refEntity->GetLatLon();

        entity->worldLocation.x = xyz.x();
        entity->worldLocation.y = xyz.y();
        entity->worldLocation.z = xyz.z();
        entity->worldLocation.lat = latLonAlt.x();
        entity->worldLocation.lon = latLonAlt.y();
        entity->worldLocation.alt = latLonAlt.z();

        DtTaitBryan orientation = refEntity->GetOrientation();

        entity->orientation.psi = orientation.psi();
        entity->orientation.theta = orientation.theta();
        entity->orientation.phi = orientation.phi();
        entity->orientation.azimuth = refEntity->GetAzimuth();
        entity->orientation.elevation = refEntity->GetElevation();


        DtEntityType type = refEntity->GetEntityType();

        entity->entityType.entityKind = type.kind();
        entity->entityType.domain = type.domain();
        entity->entityType.countryCode = type.country();
        entity->entityType.category = type.category();
        entity->entityType.subcategory = type.subCategory();
        entity->entityType.specific = type.specific();
        entity->entityType.extra = type.extra();
    }

    return entity;
}




// /**
// FUNCTION :: GetNextRadioStruct
// PURPOSE :: VR-Link's get next RadioStruct function from refRadio dll list
// PARAMETERS ::
// RETURN :: void : EntityStruct*
// **/
SNT_SYNCH::RadioStruct* VRLinkExternalInterface_Impl::GetNextRadioStruct()
{
    SNT_SYNCH::RadioStruct* radio = NULL;
    VRLinkRadio* refRadio = vrlink->GetNextReflectedRadio();

    if (refRadio)
    {
        radio = new SNT_SYNCH::RadioStruct;

        DtEntityIdentifier id = refRadio->GetEntityId();

        radio->entityId.siteId = id.site();
        radio->entityId.applicationId = id.host();
        radio->entityId.entityNumber = id.entityNum();

        radio->nodeId = refRadio->GetNodeId();
        radio->radioIndex = refRadio->GetRadioIndex();

        std::string markingData = refRadio->GetMarkingData();

        int len = markingData.size();
        if (len)
        {
            radio->markingData = (char*)malloc((len + 1) * sizeof(char));
            memcpy(radio->markingData,
                   markingData.c_str(),
                   len);
            radio->markingData[len] = '\0';
        }

        DtVector position = refRadio->GetRelativePosition();

        radio->relativePosition.bodyXDistance = position.x();
        radio->relativePosition.bodyYDistance = position.y();
        radio->relativePosition.bodyZDistance = position.z();

        DtVector absoluteNodePosInLatLonAlt = refRadio->GetAbsoluteLatLonAltPosition();

        radio->absoluteNodePosInLatLonAlt.lat = absoluteNodePosInLatLonAlt.x();
        radio->absoluteNodePosInLatLonAlt.lon = absoluteNodePosInLatLonAlt.y();
        radio->absoluteNodePosInLatLonAlt.alt = absoluteNodePosInLatLonAlt.z();

        RadioSystemType radioSystemType = refRadio->GetRadioSystemType();

        radio->radioSystemType.entityKind = radioSystemType.entityKind;
        radio->radioSystemType.domain = radioSystemType.domain;
        radio->radioSystemType.countryCode = radioSystemType.countryCode;
        radio->radioSystemType.category = radioSystemType.category;
        radio->radioSystemType.nomenclatureVersion =
            (SNT_SYNCH::NomenclatureVersionEnum::NomenclatureVersionEnum8)
                radioSystemType.nomenclatureVersion;
        radio->radioSystemType.nomenclature =
            (SNT_SYNCH::NomenclatureEnum::NomenclatureEnum16)
                radioSystemType.nomenclature;

    }
    return radio;
}



// /**
// FUNCTION :: GetNextAggregateStruct
// PURPOSE :: VR-Link's get next AggregateEntityStruct function from
//            refAggregateEntities dll list
// PARAMETERS ::
// RETURN :: void : AggregateEntityStruct*
// **/
SNT_SYNCH::AggregateEntityStruct*
    VRLinkExternalInterface_Impl::GetNextAggregateStruct()
{
    SNT_SYNCH::AggregateEntityStruct* aggre = NULL;
    VRLinkAggregate* refAggr = vrlink->GetNextReflectedAggregate();

    if (refAggr)
    {
        aggre = new SNT_SYNCH::AggregateEntityStruct;

        DtEntityIdentifier id = refAggr->GetEntityId();

        aggre->entityIdentifier.siteId = id.site();
        aggre->entityIdentifier.applicationId = id.host();
        aggre->entityIdentifier.entityNumber = id.entityNum();

        std::string myName = id.string();
        int len = myName.size();

        if (len)
        {
            aggre->myName = (char*)malloc((len + 1) * sizeof(char));
            memcpy(aggre->myName,
                   myName.c_str(),
                   len);
            aggre->myName[len] = '\0';
        }

        len = refAggr->GetMyMarkingText().size();

        if (len > 31)
        {
            memcpy(&aggre->aggregateMarking.markingData,
                   refAggr->GetMyMarkingText().c_str(),
                   31);
            aggre->aggregateMarking.markingData[31] = 0;
        }
        else
        {
            memcpy(&aggre->aggregateMarking.markingData,
                   refAggr->GetMyMarkingText().c_str(),
                   len);
            aggre->aggregateMarking.markingData[len] = 0;
        }

        DtVector xyz = refAggr->GetXYZ();
        DtVector latLonAlt = refAggr->GetLatLonAlt();

        aggre->worldLocation.x = xyz.x();
        aggre->worldLocation.y = xyz.y();
        aggre->worldLocation.z = xyz.z();
        aggre->worldLocation.lat = latLonAlt.x();
        aggre->worldLocation.lon = latLonAlt.y();
        aggre->worldLocation.alt = latLonAlt.z();

        DtVector dimension = refAggr->GetMyDimensions();

        aggre->dimensions.xAxisLength  = dimension.x();
        aggre->dimensions.yAxisLength  = dimension.y();
        aggre->dimensions.zAxisLength  = dimension.z();

        std::vector<std::string> entityIDs = refAggr->GetMyEntitiesList();

        std::vector<std::string>::iterator it;
        std::string entityIDsList;

        for (it = entityIDs.begin();
             it != entityIDs.end();
             it++)
        {
            if (it == entityIDs.begin())
            {
                entityIDsList.append(*it);
            }
            else
            {
                entityIDsList.append(",");
                entityIDsList.append(*it);
            }
        }

        len = entityIDsList.size();
        if (len)
        {
            aggre->containedEntityIDs = (char*)malloc((len + 1) * sizeof(char));
            memcpy(aggre->containedEntityIDs,
                   entityIDsList.c_str(),
                   len);
            aggre->containedEntityIDs[len] = '\0';
        }

        std::vector<std::string> subAggs = refAggr->GetMySubAggsList();
        std::string subAggsList;

        for (it = subAggs.begin();
             it != subAggs.end();
             it++)
        {
            if (it == subAggs.begin())
            {
                subAggsList.append(*it);
            }
            else
            {
                subAggsList.append(",");
                subAggsList.append(*it);
            }
        }

        len = subAggsList.size();
        if (len)
        {
            aggre->subAggregateIDs = (char*)malloc((len + 1) * sizeof(char));
            memcpy(aggre->subAggregateIDs,
                   subAggsList.c_str(),
                   len);
            aggre->subAggregateIDs[len] = '\0';
        }
    }

    return aggre;
}

void VRLinkExternalInterface_Impl::Tick(clocktype simTime)
{
    vrlink->Tick(simTime);
}


