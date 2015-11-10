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


const unsigned g_lineBufSize = 16384;
const int DEFAULT_APPLICATION_ID = 5000;
const unsigned g_vrlinkScenarioNameBufSize   = 64;

const unsigned g_vrlinkMaxRadiosPerEntity = 64;
const unsigned g_vrlinkMaxMembersInNetwork = 254;
const unsigned g_vrlinkNetworkNameBufSize = 64;

// The Marking attribute consists of the MarkingEncodingType field (1 byte),
// followed by the MarkingData field (11 bytes).
// MarkingData should include a null terminator, which leaves a maximum of
// 10 useful characters.

const unsigned g_vrlinkMarkingBufSize     = 12;
const unsigned g_vrlinkMarkingDataBufSize = 11;

const double g_vrlinkTimestampRatio = 596523.235277778;

//Various Datum IDs used for VRLink
const DtDatumParam processMsgNotificationDatumId = DtDatumParam(60001);
const DtDatumParam timeoutNotificationDatumId = DtDatumParam(60002);
const DtDatumParam readyToSendSignalNotificationDatumId =DtDatumParam(60010);
const DtDatumParam nodeIdDescriptionNotificationDatumId =DtDatumParam(60101);
const DtDatumParam metricDefinitionNotificationDatumId = DtDatumParam(60102);
const DtDatumParam metricUpdateNotificationDatumId = DtDatumParam(60103);

const DtUserProtocol commEffectsProtocolId = DtUserProtocol(10000);

const unsigned g_testfedSignalDataBufSize = 8192;

//forward declaration
class VRLinkEntity;
class VRLinkRadio;
class VRLinkNetwork;
class VRLink;

#ifdef DtDIS
typedef pair<DtEntityIdentifier, unsigned int> RadioIdKey;
#elif DtHLA
typedef DtGlobalObjectDesignator RadioIdKey;
#endif

struct RadioKey
{
    char                markingData[g_vrlinkMarkingDataBufSize];
    unsigned short      radioIndex;

    bool operator < (const RadioKey& key) const
    {
        return ((radioIndex < key.radioIndex) || (!(key.radioIndex < radioIndex)
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
    unsigned short radioIndex;
    string          markingData;

    DtVector relativePosition;

    DtRadioEntityType   radioType;
    DtRadioTransmitState txOperationalStatus;

    VRLinkEntity*    entityPtr;
    VRLinkNetwork*         networkPtr;

    public:
    VRLinkRadio();
    ~VRLinkRadio();

    void ParseRecordFromFile(char* nodeInputStr);
    unsigned GetNodeId();    
    unsigned short GetRadioIndex();
    string GetMarkingData();

    VRLinkNetwork* GetNetworkPtr();
    VRLinkEntity* GetEntityPtr();
    void SetNetworkPtr(VRLinkNetwork* ptr);
    void SetEntityPtr(VRLinkEntity* ptr);

    DtRadioTransmitState GetTransmitState();
    void SetTransmitState(DtRadioTransmitState val);
    DtVector GetRelativePosition();

    string GetRadioKey();

    DtRadioEntityType GetRadioType();
};


// /**
// CLASS        ::  VRLinkEntity
// DESCRIPTION  ::  Class used to store VRLink entity information.
// **/
class VRLinkEntity
{
    private:

    DtEntityIdentifier id;
    string markingData;
    DtForceType forceType;
    
    DtDamageState damageState;

    DtVector latLonAlt;
    DtVector xyz;    //GCC coordinate system
    DtVector velocity;   //GCC coordinate system

    DtTaitBryan         orientation;

    DtEntityType        type;

    VRLinkRadio*        radioPtrs[g_vrlinkMaxRadiosPerEntity];
    unsigned            numRadioPtrs;

    public:

    VRLinkEntity();

    void ParseRecordFromFile(char* nodeInputStr);
    
    void SetEntityIdentifier(unsigned short siteId,
                             unsigned short appId,
                             unsigned short entityNum);
      
    string GetMarkingData();
    string GetEntityIdString();
    DtForceType GetForceType();
    DtEntityType GetEntityType();
    DtVector GetXYZ();
    DtVector GetVelocity();
    void SetXYZ(DtVector val);
    DtVector GetLatLon();
    void SetLatLon(DtVector val);
    void SetVelocity(DtVector val);
    DtTaitBryan GetOrientation();
    void SetOrientation(DtTaitBryan val);
    DtDamageState GetDamageState();
    void SetDamageState(DtDamageState state);

    void  AddRadio(VRLinkRadio* radio);

    DtEntityIdentifier GetId();
    string GetIdAsString();

    ~VRLinkEntity();
};


// /**
// CLASS        ::  VRLinkNetwork
// DESCRIPTION  ::  Class used to store VRLink network information.
// **/
class VRLinkNetwork
{
    private:

    string name;
    unsigned ipAddress;
    UInt64 frequency;
    bool unicast;

    unsigned numRadioPtrs;
    const VRLinkRadio*        radioPtrs[g_vrlinkMaxMembersInNetwork];

    public:

    ~VRLinkNetwork();

    void ParseRecordFromFile(
        char* nodeInputStr,
        hash_map <unsigned, VRLinkRadio*>& radioList);

    string GetName();
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

    hash_map <string, VRLinkEntity*> entities;
    hash_map <string, DtEntityPublisher*> entityPublishers;
    hash_map <unsigned, VRLinkRadio*> radios;
    hash_map <unsigned, DtRadioTransmitterPublisher*> radioPublishers;
    map <RadioKey, VRLinkRadio*> radioKeyToRadios;
    hash_map <string, VRLinkNetwork*> networks;

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

    void ReadEntitiesFile();
    void ReadRadiosFile();
    void ReadNetworksFile();

    virtual void PublishObjects();
    void SetSimTime(clocktype simTime);
    void Tick();
    void IterationSleepOperation(clocktype curSimTime);

    void IncrementEntitiesPositionIfVelocityIsSet(
        double timeSinceLastPostionUpdate);
    
    void MoveEntity(unsigned nodeId,
        double lat,
        double lon,
        double alt);
    
    void ChangeEntityOrientation(unsigned nodeId,
        double orientationPsiDegrees,
        double orientationThetaDegrees,
        double orientationPhiDegrees);

    void ChangeEntityVelocity(unsigned nodeId,
        float x,
        float y,
        float z);

    virtual void SendCommEffectsRequest(unsigned   srcNodeId,
        bool      isData,
        unsigned   msgSize,
        double     timeoutDelay,
        bool       dstNodeIdPresent,
        unsigned   dstNodeId);

    void ChangeDamageState(unsigned nodeId, 
        unsigned damageState);

    void ChangeTxOperationalStatus(unsigned nodeId,
        unsigned char txOperationalStatus);

    //callback function(s)
    static void CbDataIxnReceived(DtDataInteraction* inter, void* usr);
    static void CbCommentIxnReceived(DtCommentInteraction* inter, void* usr);
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

void Hton(void* ptr, unsigned size);

void Ntoh(void* ptr, unsigned size);

unsigned GetTimestamp();  //used to get real timestamp to place in outgoing msgs

double ConvertTimestampToDouble(unsigned timestamp);

unsigned ConvertDoubleToTimestamp(double double_timestamp, bool absolute);

double GetNumSecondsPastHour();

double Rint(double a);

void CopyToOffset(void* dst, unsigned& offset, const void* src, unsigned size);

void CopyToOffsetAndHton(void* dst, unsigned& offset, const void* src, unsigned size);

#endif //VRLINK_SHARED_H

