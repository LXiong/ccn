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

#define __NETINET_IN_H__
#include "vrlink_main.h"

// 64-bit unsigned integer type.

#ifdef _WIN32
typedef unsigned __int64 uint64;
#define UINT64_MAX 0xffffffffffffffffui64

#define uint64toa(value, string) sprintf(string, "%I64u", value)
#define atouint64(string, value) sscanf(string, "%I64u", value)
#define UINT64_C(value)          value##ui64
#else /* _WIN32 */
typedef unsigned long long uint64;
#define UINT64_MAX 0xffffffffffffffffULL

#define uint64toa(value, string) sprintf(string, "%llu", value)
#define atouint64(string, value) sscanf(string, "%llu", value)
#define UINT64_C(value)          value##ULL
#endif /* _WIN32 */


// Maximum members that can be in a netwrok
const unsigned g_maxMembersInNetwork    = 254;

const unsigned g_lineBufSize = 16384;
const unsigned g_PathBufSize            = 256;
// SignalDataLength, as a two-byte unsigned integer, has a maximum value of
// 2^16 - 1 = 65535, in units of bits.  ceil(65535.0 / 8.0) is equal to the
// value below.
const unsigned g_SignalDataBufSize = 8192;

// (2^31 - 1) / 3600, is used to compute or decode a DIS timestamp.
// The figure is rounded to a value just before further digits would be
// ignored.
const double g_TimestampRatio = 596523.235277778;

const unsigned g_ClocktypeStringBufSize = 21;

// Must be large enough to satisfy ICD requirements for Data interactions
// sent by QualNet.
// The ICD does not specify a size limit on Comm Effects Requests received by
// QualNet, so the value can be increased if desired.
// This value must be a multiple of 8, to satisfy RPR FOM 1.0 padding
// requirements.
const unsigned g_DatumValueBufSize = 1024;

//Various Protocol IDs used for VRLink
const DtUserProtocol cerUserProtocolId = DtUserProtocol(10000);

//Various Datum IDs used for VRLink
const DtDatumParam processMsgNotificationDatumId = DtDatumParam(60001);
const DtDatumParam timeoutNotificationDatumId = DtDatumParam(60002);
const DtDatumParam readyToSendSignalNotificationDatumId =DtDatumParam(60010);

const int DEFAULT_APPLICATION_ID = 4000;

// QualNet only sends one VariableDatum per Data interaction, so the value
// below is calculated correctly.

// ICD draft 6 value.
const unsigned g_maxOutgoingDataIxnVariableDatumsSize = 1000;

const unsigned g_maxOutgoingDataIxnDatumValueSize
    = g_maxOutgoingDataIxnVariableDatumsSize
      - sizeof(unsigned)   // DatumId
      - sizeof(unsigned);  // DatumLength

const unsigned g_maxDstEntityIdsInTimeoutNotification
    = (g_maxOutgoingDataIxnDatumValueSize
       - sizeof(unsigned short)*3     // Source EntityID
       - sizeof(unsigned short)       // Radio index
       - sizeof(unsigned)             // Timestamp
       - sizeof(unsigned)             // Number of packets
       - sizeof(unsigned))            // Number of entities
      / (sizeof(unsigned short)*3     // Destination EntityID
         + sizeof(bool));             // Entity status

// /**
// STRUCT :: RtssForwardedInfo
// DESCRIPTION :: Structure of the Rtss forward info.
// **/
struct RtssForwardedInfo
{
    NodeAddress nodeId;
};

//forward declaration
class VRLinkEntity;
class VRLinkRadio;
class VRLinkNetwork;
class VRLink;

// /**
// STRUCT :: VRLinkHierarchyMobilityInfo
// DESCRIPTION :: Structure of the hierarchy mobility info.
// **/
struct VRLinkHierarchyMobilityInfo
{
    unsigned            hierarchyId;
    Coordinates         coordinates;
    Orientation         orientation;
};

// /**
// STRUCT :: VRLinkChangeMaxTxPowerInfo
// DESCRIPTION :: Structure containing information required to change the
//                max tx power of an entity.
// **/
struct VRLinkChangeMaxTxPowerInfo
{
    VRLinkEntity* entityPtr;
    unsigned int damageState;
};

// /**
// STRUCT :: VRLinkSimulatedMsgInfo
// DESCRIPTION :: Structure of the simulated message.
// **/
struct VRLinkSimulatedMsgInfo
{
    VRLinkRadio*     srcRadioPtr;
    unsigned int            msgId;
//    DtEntityIdentifier* destEntityId;
    Node* srcNode;
};

// /**
// STRUCT :: VRLinkSimulatedMsgDstEntityInfo
// DESCRIPTION :: Structure of individual destination info for
//              a simulated message.
// **/
struct VRLinkSimulatedMsgDstEntityInfo
{
    const VRLinkEntity*    dstEntityPtr;
    bool                processed;
    bool                success;
};

// /**
// STRUCT :: VRLinkOutstandingSimulatedMsgInfo
// DESCRIPTION :: Structure of the outstanding simulated 
//                  message (stores all destination info)
// **/
struct VRLinkOutstandingSimulatedMsgInfo
{
    unsigned int    timestamp;
    clocktype   sendTime;

    unsigned int    numDstEntitiesProcessed;
    std::vector<VRLinkSimulatedMsgDstEntityInfo>
        smDstEntitiesInfos; 
};

typedef std::map<unsigned, VRLinkOutstandingSimulatedMsgInfo> outstandingSimulatedMsgInfoMap;

// /**
// STRUCT :: VRLinkGetInitialPhyTxPowerResposeInfo
// DESCRIPTION :: Structure containing information required to get intial
//                max tx power of an entity.
// **/
struct VRLinkGetInitialPhyTxPowerResposeInfo
{
    NodeId     nodeId;
    double txPower;
};

// /**
// STRUCT :: EXTERNAL_VRLinkStartMessenegerForwardedInfo
// DESCRIPTION :: Structure of the start messenger forward info.
// **/
struct EXTERNAL_VRLinkStartMessenegerForwardedInfo
{
    VRLinkSimulatedMsgInfo smInfo;
    double requestedDataRate;
    NodeAddress srcNodeId;
    unsigned dataMsgSize;
    clocktype voiceMsgDuration;
    clocktype timeoutDelay;
    clocktype sendDelay;
    NodeAddress srcNetworkAddress;
    NodeAddress destNodeId;
    bool unicast;
    bool isVoice;
    bool isLink11;
    bool isLink16;
};

// /**
// STRUCT :: EXTERNAL_VRLinkCompletedMessenegerForwardedInfo
// DESCRIPTION :: Structure of the completed message forward info.
// **/
struct EXTERNAL_VRLinkCompletedMessenegerForwardedInfo
{
    VRLinkSimulatedMsgInfo smInfo;
    NodeAddress destNodeId;
    BOOL success;
};

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
typedef pair<EntityIdentifierKey, unsigned int> RadioIdKey;
#elif DtHLA
typedef DtGlobalObjectDesignator RadioIdKey;
#endif

typedef map<EntityIdentifierKey, list<NodeId> >::iterator
                                                        EntityIdToNodeIdIter;
typedef map<RadioIdKey, VRLinkRadio*>::iterator ReflRadioIdToVRLinkRadioIter;
typedef map<EntityIdentifierKey, VRLinkEntity*>::iterator EntityIdToDataIter;
typedef pair<EntityIdentifierKey, short> EntityIdKeyRadioIndex;

// /**
// CLASS        ::  VRLinkData
// DESCRIPTION  ::  Class used to store simulated message information.
// **/
class VRLinkData
{
    private:
    VRLinkRadio*     radioPtr;
    unsigned int     nextMsgId;
    
    outstandingSimulatedMsgInfoMap outstandingSimulatedMsgInfo ;

    public:

    outstandingSimulatedMsgInfoMap *
        GetOutstandingSimulatedMsgInfoHash() { return &outstandingSimulatedMsgInfo; }

    VRLinkData()
    {
        radioPtr = NULL;
        nextMsgId = 0;
    }

    const VRLinkRadio* GetRadioPtr() const;
    VRLinkRadio* GetRadioPtr();
    void SetRadioPtr(VRLinkRadio* radio);
    ~VRLinkData();

    unsigned int GetNextMsgId();
    void SetNextMsgId(unsigned int nextId);
};

// /**
// CLASS        ::  VRLinkRadio
// DESCRIPTION  ::  Class used to store VRLink radio information.
// **/
class VRLinkRadio
{
    private:
    Node* node;
    unsigned short radioIndex;
    string markingData;

    DtEntityIdentifier entityId;
    DtRadioTransmitState txOperationalStatus;

    RandomSeed seed;

    DtVector relativePosition;   //GCC coordinate system
    DtVector latLonAltScheduled;

    bool                usesTxPower;
    double              maxTxPower;
    double              currentMaxTxPower;

    const VRLinkEntity*    entityPtr;
    VRLinkNetwork*         networkPtr;
    const VRLinkRadio*     defaultDstRadioPtr;

    bool isLink16Node;
    bool isLink11Node;

    public:
    VRLinkRadio();
    string GetMarkingData() const;
    Node* GetNode() const;
    void SetNode(Node * n) {node = n;}
    unsigned short GetRadioIndex() const;

    DtRadioTransmitState GetTxOperationalStatus();
    DtVector GetLatLonAltSch();
    DtVector GetRelativePosition();
    const VRLinkNetwork* GetNetworkPtr();
    const VRLinkEntity* GetEntityPtr() const;
    const VRLinkRadio* GetDefaultDstRadioPtr();

    void SetEntityPtr(VRLinkEntity* entity);
    void SetNetworkPtr(VRLinkNetwork* network);
    void SetDefaultDstRadioPtr();
    void SetLatLonAltScheduled(VRLinkEntity* entity);
    void SetLatLonAltScheduled(DtVector latLonAltSch);

    void SetEntityId(const DtEntityIdentifier &entId);
    void ScheduleChangeInterfaceState(bool enableInterface);
    void DisplayData();
    void Remove();

    ~VRLinkRadio();

    void ReflectOperationalStatus(
             const DtRadioTransmitState& txOperationalStatus,
             VRLink* vrLink);

    void ReflectAttributes(
             DtReflectedRadioTransmitter* radioTx,
             VRLink* vrLink);

    void ParseRecordFromFile(
             PartitionData* partitionData,
             char* nodeInputStr);

    bool IsCommEffectsRequestValid(
             const VRLinkEntity** dstEntityPtr,
             const VRLinkRadio** dstRadioPtr,
             bool& unicast,
             VRLink* vrLink = NULL);

    void SetInitialPhyTxPowerForRadio(double txPower);

    void ChangeMaxTxPower(
        bool restoreMaxTxPower,
        double probability,
        double maxFraction,
        VRLink* vrLink);

    /*callback function(s)*/
    static void CbReflectAttributeValues(
        DtReflectedRadioTransmitter* reflRadio,
        void* usr);

    bool IsLink16Node();
    bool IsLink11Node();
    void SetAsLink11Node();
    void SetAsLink16Node();
};

// /**
// CLASS        ::  VRLinkEntity
// DESCRIPTION  ::  Class used to store VRLink entity information.
// **/
class VRLinkEntity
{
    private:

    DtEntityIdentifier entityId;
    string markingData;
    char forceId;
    UInt8 forceIdByte;
    string domain;
    UInt32 domainInt;
    string nationality;

    DtDamageState damageState;

    DtVector latLonAlt;
    DtVector latLonAltScheduled;

    DtVector xyz;   //GCC coordinate system
    DtVector xyzScheduled;

    DtTaitBryan         currentOrientation;
    short               azimuth;
    short               elevation;
    short               azimuthScheduled;
    short               elevationScheduled;

    DtVector            currentVelocity;  //GCC coordinate system
    DtVector            velocityScheduled;
    double              speed;
    double              speedScheduled;

    clocktype           lastScheduledMobilityEventTime;

    bool                hierarchyIdExists;
    int                 hierarchyId;

    hash_map<unsigned short, VRLinkRadio*> radioPtrs;

    public:

    VRLinkEntity();
    void ParseRecordFromFile(char* nodeInputStr);
    void InsertInRadioPtrList(VRLinkRadio* radio);
    string GetMarkingData() const;
    DtEntityIdentifier GetEntityId() const;
    const char * GetEntityIdString() const;
    DtVector GetXYZ();
    DtVector GetLatLon();
    clocktype GetLastScheduledMobilityEventTime();
    DtDamageState GetDamageState() const;

    void SetEntityId(const DtEntityIdentifier &entId);
    void SetHierarchyData(const hash_map<NodeId, int>& nodeToHiereachyId);
    void Remove();
    ~VRLinkEntity();
    void ReflectAttributes(DtReflectedEntity* ent, VRLink* vrLink);

    void ReflectDamageState(
             DtDamageState damageState,
             VRLink* vrLink);

    void ReflectWorldLocation(const DtVector& worldLocation,
                              VRLink* vrLink,
                              bool& schedulePossibleMobilityEvent);
    void ReflectVelocity(const DtVector& speed,
                         bool& schedulePossibleMobilityEvent);
    void ReflectOrientation(const DtTaitBryan& orientation,
                            bool& schedulePossibleMobilityEvent);
    void ReflectForceId(const DtForceType & reflForceId);
    void ReflectDomainField(const int &reflDomain);
    void ScheduleMobilityEventIfNecessary(VRLink* vrLink);

    void ConvertReflOrientationToQualNetOrientation(
             const DtTaitBryan& orientation);

    VRLinkRadio* GetRadioPtr(unsigned short radioIndex);
    void DisplayData();
    bool IsIdenticalEntityId(const VRLinkEntity** otherEntity) const;

    void ChangeMaxTxPowerOfRadios(
        bool restoreMaxTxPower,
        double probability,
        double maxFraction,
        VRLink* vrLink);

    void ScheduleChangeInterfaceState(bool enableIfaces, VRLink*& vrLink);

    void ScheduleChangeMaxTxPower(
        Node *node,
        unsigned damageState,
        clocktype delay);

    hash_map<unsigned short, VRLinkRadio*> GetRadioPtrsHash();

    double Rint(double a);

    /*callback function(s)*/
    static void CbReflectAttributeValues(DtReflectedEntity* ent, void* usr);

    enum Domain {
        DOMAIN_OTHER,
        DOMAIN_LAND,
        DOMAIN_AIR,
        DOMAIN_SURFACE,
        DOMAIN_SUBSURFACE,
        DOMAIN_SPACE
    };
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
    uint64 frequency;
    hash_map<NodeId, VRLinkRadio*> radioPtrsHash;
    bool unicast;

    public:

    string& GetName();
    unsigned GetIpAddress() const;
    bool IsUnicast();
    const hash_map<NodeId, VRLinkRadio*>& GetRadioPtrsHash() const;
    ~VRLinkNetwork();

    void ParseRecordFromFile(
        char* nodeInputStr,
        const hash_map <NodeId, VRLinkRadio*>& radiosPtrHash);

    VRLinkRadio* GetDefaultDstRadioPtrForRadio(VRLinkRadio* srcRadio);
};

// /**
// CLASS        ::  VRLink
// DESCRIPTION  ::  Class used to store common VRLink data for HLA and DIS.
// **/
class VRLink
{
    protected:
    EXTERNAL_Interface* iface;

    VRLinkSimulatorInterface* simIface;
    DtExerciseConn*      exConn;

    int siteId;
    int applicationId;

    clocktype           mobilityInterval;

    DtReflectedEntityList* refEntityList;
    DtReflectedRadioTransmitterList* refRadioTxList;

    map <DtEntityIdentifier, list<DtReflectedRadioTransmitter*> > refRadiosWithoutEntities;

    VRLinkInterfaceType vrLinkType;

    double              minLat;
    double              maxLat;
    double              minLon;
    double              maxLon;

    hash_map <NodeId, DtEntityIdentifier> nodeIdToEntityIdHash;
    map <EntityIdentifierKey, list<NodeId> > entityIdToNodeIdMap;
    hash_map <string, VRLinkEntity*> reflectedEntities;
    map <RadioIdKey, VRLinkRadio*> reflRadioIdToRadioMap;
    hash_map <string, VRLinkRadio*> reflGlobalIdToRadioHash;
    hash_map<NodeId, VRLinkData*> nodeIdToPerNodeDataHash;
    
    map<EntityIdentifierKey, VRLinkEntity*> entityIdToEntityData;
    map<EntityIdKeyRadioIndex, VRLinkRadio*> radioIdToRadioData;

    string            entitiesPath;
    string            radiosPath;
    string            networksPath;

    double              xyzEpsilon;

    double              slightDamageReduceTxPowerProbability;
    double              moderateDamageReduceTxPowerProbability;
    double              destroyedReduceTxPowerProbability;
    double              slightDamageReduceTxPowerMaxFraction;
    double              moderateDamageReduceTxPowerMaxFraction;
    double              destroyedReduceTxPowerMaxFraction;

    bool endProgram;
    unsigned int maxMsgId;
    bool newEventScheduled;

    bool                debug;
    bool                debug2;
    bool                debugPrintComms;
    bool                debugPrintComms2;
    bool                debugPrintMapping;
    bool                debugPrintDamage;
    bool                debugPrintTxState;
    bool                debugPrintTxPower;
    bool                debugPrintMobility;
    bool                debugPrintTransmitterPdu;
    bool                debugPrintPdus;

    bool disableReflectRadioTimeouts;

    hash_map <string, VRLinkEntity*> entities;
    hash_map <NodeId, VRLinkRadio*> radios;
    hash_map <string, VRLinkNetwork*> networks;

    public:
    VRLink(EXTERNAL_Interface* ifacePtr, NodeInput* nodeInput, VRLinkSimulatorInterface *simIface);
    VRLinkSimulatorInterface* GetSimIface();

    void AssignEntityIdToNodesNotInRadiosFile();
    void SetMinAndMaxLatLon();
    bool IsTerrainDataValid(double lat, double lon);

    virtual void CreateFederation();

    virtual void InitConfigurableParameters(NodeInput* nodeInput);

    virtual void ReadLegacyParameters(NodeInput* nodeInput) = 0;

    virtual void ProcessEvent(Node* node, Message* msg);
    virtual void SetSimulationTime();
    void ProcessCommEffectsRequest(DtSignalInteraction* inter);
    virtual void RegisterProtocolSpecificCallbacks();

    void SetSenderIdForOutgoingDataPdu(
        NodeId nodeId,
        DtDataInteraction& pdu);

    list<NodeId>* GetNodeIdFromIncomingDataPdu(DtDataInteraction* pdu);

    EntityIdentifierKey* GetEntityIdKeyFromDtEntityId(
                                       const DtEntityIdentifier* dtEntityId);
    EntityIdentifierKey GetEntityIdKeyFromDtEntityId(
        const DtEntityIdentifier& dtEntityId);

    virtual RadioIdKey GetRadioIdKey(DtSignalInteraction* inter);    
    virtual RadioIdKey GetRadioIdKey(DtRadioTransmitterRepository* radioTsr);

    const list<NodeId>* GetNodeIdFromEntityIdToNodeIdMap(
        DtEntityIdentifier& entId);

    DtEntityIdentifier* GetEntityPtrFromNodeIdToEntityIdHash(
        const NodeId& nodeId);

    void SetType(VRLinkInterfaceType vrlinkType);
    VRLinkInterfaceType GetType();
    virtual void ScheduleTasksIfAny();
    EXTERNAL_Interface* GetExtIfacePtr();

    const hash_map<NodeId, VRLinkData*>& GetNodeIdToPerNodeDataHash() const;
    bool IsEntityMapped(const VRLinkEntity* entity);

    void TrimLeft(char *s);
    void TrimRight(char *s);
    unsigned
        GetNumLinesInFile(const char* path);
    void ReadEntitiesFile();
    void ReadRadiosFile();
    void ReadNetworksFile();

    void MapDefaultDstRadioPtrsToRadios();
    void MapHierarchyIds(EXTERNAL_Interface* iface);
    void RegisterRadioWithEntity(VRLinkRadio* radio);

    void CopyToOffsetEntityId(
        void* dst,
        unsigned& offset,
        const DtEntityIdentifier& srcId);

    void CopyToOffset(
        void* dst,
        unsigned& offset,
        const void* src,
        unsigned size);

    void CopyUInt32ToOffset(void* dst, unsigned& offset,UInt32* src);
    void CopyBoolToOffset(void* dst, unsigned& offset, bool* src);
    void CopyDoubleToOffset(void* dst, unsigned& offset, double* src);

    void ProcessTerminateCesRequest();
    void EndSimulation();

    void AppProcessChangeMaxTxPowerEvent(Message* msg);
    void AppProcessMoveHierarchy(Node* node, Message* msg);
    void RegisterCallbacks();
    void TryForFirstObjectDiscovery();
    DtExerciseConn* GetExConn();
    bool GetNewEventScheduled();
    void EnableNewEventScheduled();

    static bool RadioCallbackCriteria(DtReflectedObject* obj, void* usr);
    static bool EntityCallbackCriteria(DtReflectedObject* obj, void* usr);
    void AddReflectedEntity(DtReflectedEntity* ent);
    void RemoveReflectedEntity(DtReflectedEntity* ent);
    void AddReflectedRadio(DtReflectedRadioTransmitter* radioTx);
    void RemoveReflectedRadio(DtRadioTransmitterRepository*& tsr);
    void AppProcessTimeoutEvent(Message* msg);
    virtual void ProcessSignalIxn(DtSignalInteraction* inter);
    static void AppMessengerResultFcn(Node* node, Message* msg,BOOL success);

    void AppProcessGetInitialPhyTxPowerRequest(Node* node, Message* msg);
    void AppProcessGetInitialPhyTxPowerResponse(Message* msg);

    void PrintCommEffectsResult(
        Node* node,
        const VRLinkSimulatedMsgInfo& smInfo,
        const char* resultString);

    void AppMessengerResultFcnBody(
        Node* node,
        Message* msg,
        BOOL success);

    virtual void SendProcessMsgNotification(
        Node* node,
        VRLinkSimulatedMsgInfo& smInfo,
        const VRLinkOutstandingSimulatedMsgInfo& osmInfo);

    virtual void SendTimeoutNotification(
        const VRLinkSimulatedMsgInfo& smInfo,
        const VRLinkOutstandingSimulatedMsgInfo& osmInfo);

    bool IsMobilityEventSchedulable(
        VRLinkEntity* entity,
        clocktype& mobilityEventTime,
        clocktype& simTime);

    bool ParseMsgString(
        char* msgString,
        const VRLinkEntity** dstEntityPtr,
        unsigned& dataMsgSize,
        clocktype& voiceMsgDuration,
        bool& isVoice,
        clocktype& timeoutDelay,
        unsigned& timestamp,        
        DtEntityIdentifier** destinationEntityId = NULL);

    void ScheduleTimeout(
        const VRLinkSimulatedMsgInfo& smInfo,
        clocktype timeoutDelay,
        clocktype sendDelay);

    void SendSimulatedMsgUsingMessenger(
        VRLinkSimulatedMsgInfo& smInfo,
        VRLinkEntity* dstEntityPtr,
        double requestedDataRate,
        unsigned dataMsgSize,
        clocktype voiceMsgDuration,
        bool isVoice,
        clocktype timeoutDelay,
        bool unicast,
        VRLinkRadio* dstRadioPtr,
        clocktype sendDelay);

    void SendSimulatedMsgUsingMessenger(
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
        const set<NodeId>& destNodeIdSet);

    void StartSendSimulatedMsgUsingMessenger(Node* srcNode,
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
        bool isLink16);

    void PrintCommEffectsRequestProcessed(
        const VRLinkRadio* srcRadio,
        const VRLinkEntity* dstEntityPtr,
        bool unicast,
        const VRLinkRadio* dstRadioPtr,
        clocktype sendTime,
        unsigned int msgId);

    void StoreOutstandingSimulatedMsgInfo(
        const VRLinkSimulatedMsgInfo& smInfo,
        VRLinkEntity* dstEntityPtr,
        unsigned timestamp,
        clocktype sendTime);

    void StoreOutstandingSimulatedMsgInfo(
        const VRLinkSimulatedMsgInfo& smInfo,
        VRLinkEntity* dstEntityPtr,
        unsigned timestamp,
        clocktype sendTime,
        const set<VRLinkEntity*> &);

    void AppMessengerResultCompleted(
        Node* node,
        VRLinkSimulatedMsgInfo& smInfo,
        BOOL success);

    static void CbEntityAdded(DtReflectedEntity* ent, void* usr);
    static void CbEntityRemoved(DtReflectedEntity* ent, void *usr);
    static void CbSignalIxnReceived(DtSignalInteraction* inter, void* usr);

    static void CbRadioAdded(
        DtReflectedRadioTransmitter* radioTx,
        void* usr);

    static void CbRadioRemoved(
        DtReflectedRadioTransmitter* radioTx,
        void* usr);

    bool getEntityId(NodeId nodeId, DtEntityIdentifier &entId);
    void DisplayEntities();
    void DisplayReflEntities();
    void DisplayRadios();
    void DisplayReflRadios();

    void GoingToSleep(unsigned seconds);
    unsigned GetTimestamp();
    unsigned ConvertDoubleToTimestamp(double double_timestamp,bool absolute);
    double GetNumSecondsPastHour();
    double GetXyzEpsilon();

    bool GetDebug();
    bool GetDebug2();
    bool GetDebugPrintMobility();
    bool GetDebugPrintDamage();
    bool GetDebugPrintTransmitterPdu();
    bool GetDebugPrintTxState();
    bool GetDebugPrintComms();
    bool GetDebugPrintTxPower();

    void GetInitialPhyTxPowerForRadio(VRLinkRadio* radio);

    virtual ~VRLink();
    virtual void DeregisterCallbacks();

    virtual void UpdateMetric(unsigned nodeId,
        const MetricData& metric,
        void* metricValue,
        clocktype updateTime);

    bool debugPrintRtss;
    virtual void SendRtssNotification(Node* node);
    virtual void AppProcessSendRtssEvent(Node* node);
    void ScheduleRtssNotification(VRLinkRadio* radio, clocktype delay);
    void PrintSentRtssNotification(Node* node);
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
//FUNCTION :: VRLinkGetIfaceDataFromNode
//PURPOSE :: Returns the external interface pointer extracted from the node
//           passed.
//PARAMETERS ::
// + node : Node* : Pointer to node.
//RETURN :: VRLinkExternalInterface* : External interface pointer.
// **/
VRLinkExternalInterface* VRLinkGetIfaceDataFromNode(Node* node);

typedef VRLinkExternalInterface *dsoLoadFunc(VRLinkSimulatorInterface *simIface);

#endif /* VRLINK_SHARED_H */
