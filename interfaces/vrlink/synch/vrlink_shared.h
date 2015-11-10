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

#ifndef VRLINK_SHARED_H
#define VRLINK_SHARED_H

#include <stdlib.h>
#include "vrlink_main.h"
#include <vector>


const int DEFAULT_APPLICATION_ID = 5000;
const unsigned g_vrlinkScenarioNameBufSize   = 64;

// The Marking attribute consists of the MarkingEncodingType field (1 byte),
// followed by the MarkingData field (11 bytes).
// MarkingData should include a null terminator, which leaves a maximum of
// 10 useful characters.

const unsigned g_vrlinkMarkingBufSize     = 12;
const unsigned g_vrlinkMarkingDataBufSize = 11;

const double g_vrlinkTimestampRatio = 596523.235277778;


// forward declaration
class VRLinkEntity;
class VRLinkRadio;
class VRLinkNetwork;
class VRLink;


// /**
// STRUCT :: EntityIdentifierKey
// DESCRIPTION :: Key structure for identifying entities
// **/
struct EntityIdentifierKey
{
    UInt32 siteId;
    UInt32 applicationId;
    UInt16 entityNum;

    bool operator < (const EntityIdentifierKey& key) const
    {
        return (((siteId < key.siteId) || !(key.siteId < siteId)
                  && (applicationId < key.applicationId))
                ||
                !((key.siteId < siteId) || !(siteId < key.siteId)
                                      && (key.applicationId < applicationId))
                && (entityNum < key.entityNum));
    }
    EntityIdentifierKey():siteId(0),applicationId(0), entityNum(0){}
};

#ifdef DtDIS
typedef pair<DtEntityIdentifier, unsigned int> RadioIdKey;
#elif DtHLA
typedef DtGlobalObjectDesignator RadioIdKey;
#endif

typedef pair<EntityIdentifierKey, short> EntityIdKeyRadioIndex;

struct RadioSystemType
{
    unsigned char  entityKind;
    unsigned char  domain;
    unsigned short countryCode;
    unsigned char  category;
    unsigned char  nomenclatureVersion;
    unsigned short nomenclature;
};

struct RadioKey
{
    char                markingData[g_vrlinkMarkingDataBufSize];
    unsigned short      radioIndex;

    bool operator < (const RadioKey& key) const
    {
        return ((radioIndex < key.radioIndex)
                || (!(key.radioIndex < radioIndex)
                    && (markingData < key.markingData)));
    }
    RadioKey():radioIndex(0) {}

};

// /**
// CLASS        ::  VRLinkRadio
// DESCRIPTION  ::  Class used to store VRLink radio information.
// **/
class VRLinkRadio
{
    private:

    unsigned            nodeId;
    unsigned short      radioIndex;
    string              markingData;

    DtVector            relativePosition;  //GCC coordinate system
    DtVector            absolutePosInLatLonAlt;

    RadioSystemType     radioSystemType;
    DtRadioTransmitState txOperationalStatus;

    DtEntityIdentifier  entityId;
    VRLinkEntity*       entityPtr;
    VRLinkNetwork*      networkPtr;
    bool                isCopied;
    RadioIdKey          radioKey;

    public:
    VRLinkRadio();
    ~VRLinkRadio();

    unsigned GetNodeId();
    unsigned short GetRadioIndex();
    void SetRadioIndex(unsigned short index);
    string GetMarkingData();
    DtEntityIdentifier GetEntityId();
    RadioSystemType GetRadioSystemType();

    VRLinkNetwork* GetNetworkPtr();
    VRLinkEntity* GetEntityPtr();
    void SetNetworkPtr(VRLinkNetwork* ptr);
    void SetEntityPtr(VRLinkEntity* ptr);

    DtRadioTransmitState GetTransmitState();
    void SetTransmitState(DtRadioTransmitState val);
    DtVector GetRelativePosition();
    void SetRelativePosition(DtVector position);
    DtVector GetAbsoluteLatLonAltPosition();
    void RecalculateAbsolutePosition(DtVector entityPosition);

    void SetRadioSystemType(DtRadioEntityType radioEntType);
    void SetEntityId(DtEntityIdentifier entId);

    string GetRadioKey();
    void SetRadioIdKey(RadioIdKey key);
    RadioIdKey GetRadioIdKey();

    void ReflectOperationalStatus(
             const DtRadioTransmitState& txOperationalStatus,
             VRLink* vrLink);

    void ReflectAttributes(
             DtReflectedRadioTransmitter* radioTx,
             VRLink* vrLink);

    void SetIsCopied(bool value);
    bool GetIsCopied();

    /*callback function(s)*/
    static void CbReflectAttributeValues(
        DtReflectedRadioTransmitter* reflRadio,
        void* usr);
};


// /**
// CLASS        ::  VRLinkEntity
// DESCRIPTION  ::  Class used to store VRLink entity information.
// **/
class VRLinkEntity
{
    private:

    DtEntityIdentifier  id;
    string              markingData;
    DtForceType         forceType;
    DtDamageState       damageState;
    DtVector            latLonAlt;
    DtVector            xyz;   //GCC coordinate system

    DtTaitBryan         orientation;
    short               azimuth;
    short               elevation;
    DtEntityType        type;
    unsigned            numRadioPtrs;
    string              myName;
    bool                isCopied;

    public:

    VRLinkEntity();


    void SetEntityIdentifier(unsigned short siteId,
                             unsigned short appId,
                             unsigned short entityNum);

    string GetMarkingData();
    string GetEntityIdString();
    DtForceType GetForceType();
    DtEntityType GetEntityType();
    DtVector GetXYZ();

    void SetXYZ(DtVector val);
    DtVector GetLatLon();
    void SetLatLon(DtVector val);

    DtTaitBryan GetOrientation();
    void SetOrientation(DtTaitBryan val);
    short GetAzimuth();
    short GetElevation();
    DtDamageState GetDamageState();
    void SetDamageState(DtDamageState state);

    string GetMyName();
    void SetIsCopied(bool value);
    bool GetIsCopied();
    DtEntityIdentifier GetId();
    string GetIdAsString();

    void ReflectAttributes(DtReflectedEntity* ent, VRLink* vrLink);
    void ReflectDamageState(
             DtDamageState damageState,
             VRLink* vrLink);
    void ReflectWorldLocation(const DtVector& worldLocation,
                              VRLink* vrLink);

    void ReflectOrientation(const DtTaitBryan& orientation);
    void ReflectForceId(const DtForceType & reflForceId);

    void ConvertReflOrientationToQualNetOrientation(
             const DtTaitBryan& orientation);

    /*callback function(s)*/
    static void CbReflectAttributeValues(DtReflectedEntity* ent, void* usr);

    ~VRLinkEntity();
};


// /**
// CLASS        ::  VRLinkNetwork
// DESCRIPTION  ::  Class used to store VRLink network information.
// **/
class VRLinkNetwork
{
    private:

    string      name;
    unsigned    ipAddress;
    UInt64      frequency;
    bool        unicast;
    unsigned    numRadioPtrs;

    public:

    ~VRLinkNetwork();

    string GetName();
};


// /**
// CLASS        ::  VRLinkAggregate
// DESCRIPTION  ::  Class used to store VRLink aggregate information.
// **/
class VRLinkAggregate
{
    DtEntityIdentifier   id;
    DtVector             latLonAlt;
    DtVector             xyz;
    DtForceType          myForceID;
    DtAggregateState     myAggregateState;
    DtAggregateFormation myFormation;
    DtCharacterSet       myMarkingCharSet;
    DtString             myMarkingText;
    DtVector             myDimensions;
    std::vector<std::string> myEntities;
    std::vector<std::string> mySubAggs;
    bool                 isCopied;

public:
    VRLinkAggregate();
    void SetEntityId(DtEntityIdentifier entId);
    DtEntityIdentifier GetEntityId();
    void ReflectAttributes(DtReflectedAggregate* agge);

    void SetIsCopied(bool value);
    bool GetIsCopied();
    DtString GetMyMarkingText();
    DtVector GetXYZ();
    DtVector GetLatLonAlt();
    DtVector GetMyDimensions();

    std::vector<std::string> GetMyEntitiesList();
    std::vector<std::string> GetMySubAggsList();

    /*callback function(s)*/
    static void CbReflectAttributeValues(
        DtReflectedAggregate* agge,
        void* usr);
};



// /**
// CLASS        ::  VRLink
// DESCRIPTION  ::  Class used to store common VRLink data for HLA and DIS.
// **/
class VRLink
{
protected:
    DtExerciseConn*      exConn;
    VRLinkInterfaceType  vrLinkType;

    char m_scenarioName[g_vrlinkScenarioNameBufSize];
    int siteId;
    int applicationId;
    bool m_debug;

    DtReflectedEntityList* refEntityList;
    DtReflectedRadioTransmitterList* refRadioTxList;
    DtReflectedAggregateList* refAggregateList;

    hash_map <string, VRLinkEntity*> reflectedEntities;
    map <RadioIdKey, VRLinkRadio*> reflRadioIdToRadioMap;

    map <DtEntityIdentifier, list<DtReflectedRadioTransmitter*> > refRadiosWithoutEntities;
    vector <VRLinkAggregate*> refAggregateEntities;

    map<EntityIdentifierKey, VRLinkEntity*> entityIdToEntityData;
    map<EntityIdKeyRadioIndex, VRLinkRadio*> radioIdToRadioData;

public:
    VRLink();
    virtual ~VRLink();

    DtExerciseConn* GetExConn();

    void SetType(VRLinkInterfaceType vrlinkType);
    void SetScenarioName(char scenarioName[]);
    void SetDebug(bool val);
    VRLinkInterfaceType GetType();

    virtual void CreateFederation();
    virtual void RegisterCallbacks();
    virtual void DeregisterCallbacks();

    virtual void InitConnectionSetting(char connectionVar1[],
                                        char connectionVar2[],
                                        char connectionVar3[],
                                        char connectionVar4[]);


    void Tick(clocktype simTime);

    //callback function(s)
    static bool RadioCallbackCriteria(DtReflectedObject* obj, void* usr);
    static bool EntityCallbackCriteria(DtReflectedObject* obj, void* usr);

    static void CbEntityAdded(DtReflectedEntity* ent, void* usr);
    static void CbEntityRemoved(DtReflectedEntity* ent, void *usr);

    static void CbRadioAdded(
        DtReflectedRadioTransmitter* radioTx,
        void* usr);

    static void CbRadioRemoved(
        DtReflectedRadioTransmitter* radioTx,
        void* usr);

    static void CbAggregateAdded(DtReflectedAggregate* agge, void* usr);
    static void CbAggregateRemoved(DtReflectedAggregate* agge, void *usr);


    virtual void RegisterProtocolSpecificCallbacks();
    void AddReflectedEntity(DtReflectedEntity* ent);
    void RemoveReflectedEntity(DtReflectedEntity* ent);
    void AddReflectedRadio(DtReflectedRadioTransmitter* radioTx);
    void RemoveReflectedRadio(DtReflectedRadioTransmitter* radioTx);
    void AddReflectedAggregate(DtReflectedAggregate* agge);
    void RemoveReflectedAggregate(DtReflectedAggregate* agge);

    EntityIdentifierKey* GetEntityIdKeyFromDtEntityId(
                                       const DtEntityIdentifier* dtEntityId);
    EntityIdentifierKey GetEntityIdKeyFromDtEntityId(
                                       const DtEntityIdentifier& dtEntityId);
    virtual RadioIdKey GetRadioIdKey(DtRadioTransmitterRepository* radioTsr);

    VRLinkEntity* GetNextReflectedEntity();
    VRLinkRadio* GetNextReflectedRadio();
    VRLinkAggregate* GetNextReflectedAggregate();

    map <RadioIdKey, VRLinkRadio*>* GetVRLinkRadios();
};



// /**
// CLASS        ::  VRLinkEntityCbUsrData
// DESCRIPTION  ::  Class used to specify usr data for entity callbacks.
// **/
class VRLinkEntityCbUsrData
{
    private:
    VRLink* vrLink;
    VRLinkEntity* entity;

    public:
    VRLinkEntityCbUsrData(VRLink* vrLinkPtr, VRLinkEntity* entityPtr);
    void GetData(VRLink** vrLinkPtr, VRLinkEntity** entity);
};

// /**
// CLASS        ::  VRLinkRadioCbUsrData
// DESCRIPTION  ::  Class used to specify usr data for radio callbacks.
// **/
class VRLinkRadioCbUsrData
{
    private:
    VRLink* vrLink;
    VRLinkRadio* radio;

    public:
    VRLinkRadioCbUsrData(VRLink* vrLinkPtr, VRLinkRadio* radioPtr);
    void GetData(VRLink** vrLinkPtr, VRLinkRadio** radio);
};

// /**
// CLASS        ::  VRLinkAggregateCbUsrData
// DESCRIPTION  ::  Class used to specify usr data for aggregate callbacks.
// **/
class VRLinkAggregateCbUsrData
{
    private:
    VRLink* vrLink;
    VRLinkAggregate* agge;

    public:
    VRLinkAggregateCbUsrData(VRLink* vrLinkPtr, VRLinkAggregate* aggePtr);
    void GetData(VRLink** vrLinkPtr, VRLinkAggregate** agge);
};



// /**
// FUNCTION :: VRLinkVerify
// PURPOSE :: Evaluates the condition passed, and if found false, asserts.
// PARAMETERS ::
// + condition : bool : Condition to be evaluated.
// + errorString : char* : Error string to be displayed in case the passed
//                         condition is found to be false.
// + path : const char* : Location of file in which this verification is
//                        being done.
// + lineNumber : unsigned : Line in file where this API has been called.
// RETURN :: void : NULL
// **/
void VRLinkVerify(
    bool condition,
    const char* errorString,
    const char* path = NULL,
    unsigned lineNumber = 0);

// /**
// FUNCTION :: VRLinkReportWarning
// PURPOSE :: Displays the warning string passed.
// PARAMETERS ::
// + warningString : const char* : Warning string to be displayed.
// + path : const char* : Location of file in which this warning is being
//                        flashed.
// + lineNumber : unsigned : Line in file where this API has been called.
// RETURN :: void : NULL
// **/
void VRLinkReportWarning(
    const char* warningString,
    const char* path = NULL,
    unsigned lineNumber = 0);

// /**
// FUNCTION :: VRLinkReportError
// PURPOSE :: Displays the error string passed and asserts.
// PARAMETERS ::
// + errorString : const char* : Error string to be displayed.
// + path : const char* : Location of file in which this error is being
//                        flashed.
// + lineNumber : unsigned : Line in file where this API has been called.
// RETURN :: void : NULL
// **/
void VRLinkReportError(
    const char* errorString,
    const char* path = NULL,
    unsigned lineNumber = 0);

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
    DtVector& gccCoord);



double Rint(double a);


#endif //VRLINK_SHARED_H

