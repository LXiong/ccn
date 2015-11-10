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

#ifndef VRLINK_SIM_IFACE_H
#define VRLINK_SIM_IFACE_H

enum MessageEventTypesUsedInVRLink
{
    Invalid_MessageEventType = -1,
    EXTERNAL_VRLINK_HierarchyMobility = 0,
    EXTERNAL_VRLINK_ChangeMaxTxPower = 1,
    EXTERNAL_VRLINK_AckTimeout = 2,
    EXTERNAL_VRLINK_SendRtss = 3,
    EXTERNAL_VRLINK_SendRtssForwarded = 4,
    EXTERNAL_VRLINK_StartMessengerForwarded = 5,
    EXTERNAL_VRLINK_CompletedMessengerForwarded = 6,
    EXTERNAL_VRLINK_InitialPhyMaxTxPowerRequest = 7,
    EXTERNAL_VRLINK_InitialPhyMaxTxPowerResponse = 8,
    EXTERNAL_VRLINK_CheckMetricUpdate = 9
};

enum TransportTypesUsedInVRLink
{
    Unreliable = 0,
    MAC =1
};

enum MetricDataTypesUsedInVRLink
{
    IntegerType = 0,
    DoubleType = 1,
    UnsignedType = 2,
    InvalidType = 3
};

enum MessengerTrafficTypesUsed
{
    GeneralTraffic = 0,
    VoiceTraffic = 1
};

// /**
// CLASS :: VRLinkSimulatorInterface
// DESCRIPTION :: Abstract Base Class used to access QualNet APIs.
// **/
class VRLinkSimulatorInterface
{
    public:

    //Message related APIs
    virtual char* ReturnInfoFromMsg(Message* msg) = 0;
    virtual short ReturnEventTypeFromMsg(Message* msg) = 0;
    virtual char* ReturnPacketFromMsg(Message* msg) = 0;

    virtual void FreeMessage(Node* node, Message* msg) = 0;

    virtual Message* AllocateMessage(
        Node *node,
        int layerType,
        int protocol,
        int eventType) = 0;

    virtual char* AllocateInfoInMsg(Node *node, Message *msg, int infoSize) = 0;

    virtual void SendMessageWithDelay(Node *node, Message *msg, clocktype delay) = 0;
    virtual void SendRemoteMessageWithDelay(Node* node, NodeId destNodeId,
                                    Message*  msg, clocktype delay) = 0;
    virtual void SetInstanceIdInMsg(Message* msg, short instance) = 0;
    virtual Message* ReturnDuplicateMsg(Node* node, const Message* msg) = 0;

    virtual char* AddInfoInMsg(
        Node* node,
        Message* msg,
        int infoSize,
        unsigned short infoType) = 0;

    //Messenger APIs
    virtual void InitMessengerApp(EXTERNAL_Interface *iface) = 0;

    virtual void SendMessageThroughMessenger(
        Node *node,
        MessengerPktHeader pktHdr,
        char *additionalData,
        int additionalDataSize,
        MessageResultFunctionType functionPtr) = 0; 

    //FileIO related APIs
    virtual void ReadString(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *parameterName,
        BOOL *wasFound,
        char *parameterValue) = 0;

    virtual void ReadInt(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *parameterName,
        BOOL *wasFound,
        int *parameterValue) = 0;

    virtual void ReadDouble(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *parameterName,
        BOOL *wasFound,
        double *parameterValue) = 0;

    virtual void ReadTime(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *parameterName,
        BOOL *wasFound,
        clocktype *parameterValue) = 0;

    virtual void ReadCachedFile(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *parameterName,
        BOOL *wasFound,
        NodeInput *parameterValue) = 0;

    virtual char* GetDelimitedToken(
        char* dst,
        const char* src,
        const char* delim,
        char** next) = 0;

    virtual void TrimLeft(char* str) = 0;
    virtual void TrimRight(char* str) = 0;

    virtual void ParseNodeIdHostOrNetworkAddress(
        const char s[],
        NodeAddress *outputNodeAddress,
        int *numHostBits,
        BOOL *isNodeId) = 0;

    virtual char* GetTokenOrEmptyField(
        char* token,
        unsigned tokenBufSize,
        unsigned& numBytesCopied,
        bool& overflowed,
        const char*& src,
        const char* delimiters,
        bool& foundEmptyField) = 0;

    virtual char* GetToken(
        char* token,
        unsigned tokenBufSize,
        unsigned& numBytesCopied,
        bool& overflowed,
        const char*& src,
        const char* delimiters) = 0;

    virtual char* GetTokenHelper(
        bool skipEmptyFields,
        bool& foundEmptyField,
        bool& overflowed,
        char* token,
        unsigned tokenBufSize,
        unsigned& numBytesToCopyOrCopied,
        const char*& src,
        const char* delimiters) = 0;

    virtual char* IO_GetDelimitedToken(
        char* dst,
        const char* src,
        const char* delim,
        char** next) = 0;

    virtual void IO_ReadInt(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *index,
        BOOL *wasFound,
        int *readVal) = 0;

    virtual void IO_ReadString(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *index,
        BOOL *wasFound,
        char *readVal) = 0;

    virtual void IO_ReadStringInstance(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *parameterName,
        const int parameterInstanceNumber,
        const BOOL fallbackIfNoInstanceMatch,
        BOOL *wasFound,
        char *parameterValue) = 0;

    virtual NodeInput * IO_CreateNodeInput (bool allocateRouterModelInput = false) = 0;
    virtual void IO_CacheFile(NodeInput *nodeInput, const char *filename) = 0;
    virtual void IO_ReadCachedFile(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *parameterName,
        BOOL *wasFound,
        NodeInput *parameterValue) = 0;

    virtual BOOL ERROR_WriteError(int   type,
        const char* condition,
        const char* msg,
        const char* file,
        int   lineno) = 0;

    //Random APIs
    virtual void RandomSetSeed(
        RandomSeed seed,
        UInt32 globalSeed,
        UInt32 nodeId = 0,
        UInt32 protocolId = 0,
        UInt32 instanceId = 0) = 0;

    virtual double RandomErand(RandomSeed seed) = 0;

    //External APIs
    virtual EXTERNAL_Interface* GetExternalInterfaceForVRLink(Node* node) = 0;

    virtual void SetExternalReceiveDelay(
        EXTERNAL_Interface *iface,
        clocktype delay) = 0;

    virtual void SetExternalSimulationEndTime(
        EXTERNAL_Interface* iface,
        clocktype duration) = 0;

    virtual clocktype QueryExternalTime(EXTERNAL_Interface *iface) = 0;

    virtual clocktype QueryExternalSimulationTime(EXTERNAL_Interface *iface) = 0;

    virtual void RemoteExternalSendMessage(
        EXTERNAL_Interface* iface,
        int destinationPartitionId,
        Message* msg,
        clocktype delay,
        ExternalScheduleType scheduling) = 0;

    virtual void ChangeExternalNodePositionOrientationAndVelocityAtTime(
        EXTERNAL_Interface *iface,
        Node *node,
        clocktype mobilityEventTime,
        double c1,
        double c2,
        double c3,
        short azimuth,
        short elevation,
        double c1Speed,
        double c2Speed,
        double c3Speed) = 0;

    virtual void EXTERNAL_ActivateNode(EXTERNAL_Interface *iface, Node *node,
        ExternalScheduleType scheduling = EXTERNAL_SCHEDULE_STRICT,
        clocktype activationEventTime = -1) = 0;

    virtual void EXTERNAL_DeactivateNode(EXTERNAL_Interface *iface, Node *node,
        ExternalScheduleType scheduling = EXTERNAL_SCHEDULE_STRICT,
        clocktype deactivationEventTime = -1) = 0;
    
    virtual void EXTERNAL_hton(void* ptr, unsigned int size) = 0;
    virtual void EXTERNAL_ntoh(void* ptr, unsigned int size) = 0;

    //PHY APIs
    virtual void GetExternalPhyTxPower(
        Node* node,
        int phyIndex,
        double * txPowerPtr) = 0;

    virtual void SetExternalPhyTxPower(
        Node* node,
        int phyIndex,
        double newTxPower) = 0;

    virtual int GetRxDataRate(Node *node, int phyIndex) = 0;

    //MAC APIs
    virtual unsigned short GetDestNPGId(Node* node, int interfaceIndex) = 0;

    //Clock APIs
    virtual void PrintClockInSecond(clocktype clock, char stringInSecond[]) = 0;
    virtual clocktype GetCurrentSimTime(Node* node) = 0;
    virtual clocktype ConvertDoubleToClocktype (double timeInSeconds) = 0;
    virtual double ConvertClocktypeToDouble (clocktype timeInNS) = 0;
    
    //Other APIs
    virtual void MoveHierarchyInGUI(
        int hierarchyID,
        Coordinates coordinates,
        Orientation orientation,
        clocktype   time) = 0;
    
    virtual Node* GetNodePtrFromOtherNodesHash(Node* node, NodeAddress nodeId, bool remote) = 0;

    virtual BOOL ReturnNodePointerFromPartition(
        PartitionData* partitionData,
        Node** node,
        NodeId nodeId,
        BOOL remoteOK = FALSE) = 0;

    virtual NodeAddress GetDefaultInterfaceAddressFromNodeId(
        Node *node,
        NodeAddress nodeId) = 0;

    virtual bool IsMilitaryLibraryEnabled() = 0;

    virtual const MetricLayerData &getMetricLayerData(int idx) = 0;

    virtual int GetCoordinateSystemType(PartitionData* p) = 0;

    virtual int GetNumInterfacesForNode(Node* node) = 0;

    virtual NodeAddress GetInterfaceAddressForInterface(Node* node, int index) = 0;

    virtual int GetPartitionIdForIface(EXTERNAL_Interface *iface) = 0;

    virtual PartitionData* GetPartitionForIface(EXTERNAL_Interface *iface) = 0;

    virtual void GetMinAndMaxLatLon(EXTERNAL_Interface* iface,
        double& minLat, double& maxLat, double& minLon, double& maxLon) = 0;

    virtual NodeId GetNodeId(Node* node) = 0;

    virtual NodeInput* GetNodeInputFromIface(EXTERNAL_Interface* iface) = 0;

    virtual void GetNodeInputDetailForIndex(NodeInput* input, int index, char** variableName, char** value) = 0;

    virtual int GetNumLinesOfNodeInput(NodeInput* input) = 0;

    virtual int GetLocalPartitionIdForNode(Node* node) = 0;

    virtual int GetRealPartitionIdForNode(Node* node) = 0;

    virtual Node* GetFirstNodeOnPartition(EXTERNAL_Interface* iface) = 0;

    virtual NodePointerCollection* GetAllNodes(EXTERNAL_Interface* iface) = 0;

    virtual ExternalScheduleType GetExternalScheduleSafeType() = 0;

    virtual MessageEventTypesUsedInVRLink GetMessageEventTypesUsedInVRLink(int i) = 0;

    virtual int GetMessageEventType(MessageEventTypesUsedInVRLink val) = 0;

    virtual int GetExternalLayerId() = 0;

    virtual int GetVRLinkExternalInterfaceType() = 0;

    virtual clocktype GetCurrentTimeForPartition(EXTERNAL_Interface* iface) = 0;

    virtual bool GetGuiOption(EXTERNAL_Interface* iface) = 0;

    virtual void* GetDataForExternalIface(EXTERNAL_Interface* iface) = 0;

    virtual MetricDataTypesUsedInVRLink GetMetricDataTypesUsedInVRLink(int i) = 0;

    virtual Int32 GetGlobalSeed(Node* node) = 0;

    virtual void SetMessengerPktHeader(MessengerPktHeader& header,
        unsigned short pktNum,
        clocktype initialPrDelay,
        NodeAddress srcAddr,
        NodeAddress destAddr,
        int msgId,
        clocktype lifetime,
        TransportTypesUsedInVRLink transportType,
        unsigned short destNPGId,
        MessengerTrafficTypesUsed appType,
        clocktype freq,
        unsigned int fragSize,
        unsigned short numFrags) = 0;
};

extern VRLinkSimulatorInterface* simIface;

#endif /* VRLINK_SIM_IFACE_H */
