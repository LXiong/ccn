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

#ifndef VRLINK_SIM_IFACE_IMPL_H
#define VRLINK_SIM_IFACE_IMPL_H

#include <vrlink_sim_iface.h>

// /**
// CLASS :: VRLinkSimulatorInterface_Impl
// DESCRIPTION :: Singleton class used to access QualNet APIs.
// **/
class VRLinkSimulatorInterface_Impl : public VRLinkSimulatorInterface
{
    private:
    static VRLinkSimulatorInterface_Impl* simIface;

    public:
    static VRLinkSimulatorInterface_Impl* GetSimulatorInterface();

    //Message related APIs
    char* ReturnInfoFromMsg(Message* msg);
    short ReturnEventTypeFromMsg(Message* msg);
    char* ReturnPacketFromMsg(Message* msg);

    void FreeMessage(Node* node, Message* msg);

    Message* AllocateMessage(
        Node *node,
        int layerType,
        int protocol,
        int eventType);

    char* AllocateInfoInMsg(Node *node, Message *msg, int infoSize);

    void SendMessageWithDelay(Node *node, Message *msg, clocktype delay);
    void SendRemoteMessageWithDelay(Node* node, NodeId destNodeId,
                                    Message*  msg, clocktype delay);
    void SetInstanceIdInMsg(Message* msg, short instance);
    Message* ReturnDuplicateMsg(Node* node, const Message* msg);

    char* AddInfoInMsg(
        Node* node,
        Message* msg,
        int infoSize,
        unsigned short infoType);

    //Messenger APIs
    void InitMessengerApp(EXTERNAL_Interface *iface);

    void SendMessageThroughMessenger(
        Node *node,
        MessengerPktHeader pktHdr,
        char *additionalData,
        int additionalDataSize,
        MessageResultFunctionType functionPtr);    

    //FileIO related APIs
    void ReadString(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *parameterName,
        BOOL *wasFound,
        char *parameterValue);

    void ReadInt(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *parameterName,
        BOOL *wasFound,
        int *parameterValue);

    void ReadDouble(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *parameterName,
        BOOL *wasFound,
        double *parameterValue);

    void ReadTime(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *parameterName,
        BOOL *wasFound,
        clocktype *parameterValue);

    void ReadCachedFile(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *parameterName,
        BOOL *wasFound,
        NodeInput *parameterValue);

    char* GetDelimitedToken(
        char* dst,
        const char* src,
        const char* delim,
        char** next);

    void TrimLeft(char* str);
    void TrimRight(char* str);

    void ParseNodeIdHostOrNetworkAddress(
        const char s[],
        NodeAddress *outputNodeAddress,
        int *numHostBits,
        BOOL *isNodeId);

    char* GetTokenOrEmptyField(
        char* token,
        unsigned tokenBufSize,
        unsigned& numBytesCopied,
        bool& overflowed,
        const char*& src,
        const char* delimiters,
        bool& foundEmptyField);

    char* GetToken(
        char* token,
        unsigned tokenBufSize,
        unsigned& numBytesCopied,
        bool& overflowed,
        const char*& src,
        const char* delimiters);

    char* GetTokenHelper(
        bool skipEmptyFields,
        bool& foundEmptyField,
        bool& overflowed,
        char* token,
        unsigned tokenBufSize,
        unsigned& numBytesToCopyOrCopied,
        const char*& src,
        const char* delimiters);

    virtual char* IO_GetDelimitedToken(
        char* dst,
        const char* src,
        const char* delim,
        char** next);

    virtual void IO_ReadInt(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *index,
        BOOL *wasFound,
        int *readVal);

    virtual void IO_ReadString(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *index,
        BOOL *wasFound,
        char *readVal);

    virtual void IO_ReadStringInstance(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *parameterName,
        const int parameterInstanceNumber,
        const BOOL fallbackIfNoInstanceMatch,
        BOOL *wasFound,
        char *parameterValue);

    virtual NodeInput * IO_CreateNodeInput (bool allocateRouterModelInput = false);
    virtual void IO_CacheFile(NodeInput *nodeInput, const char *filename);
    virtual void IO_ReadCachedFile(
        const NodeAddress nodeId,
        const NodeAddress interfaceAddress,
        const NodeInput *nodeInput,
        const char *parameterName,
        BOOL *wasFound,
        NodeInput *parameterValue);

    virtual BOOL ERROR_WriteError(int   type,
        const char* condition,
        const char* msg,
        const char* file,
        int   lineno);

    //Random APIs
    void RandomSetSeed(
        RandomSeed seed,
        UInt32 globalSeed,
        UInt32 nodeId,
        UInt32 protocolId,
        UInt32 instanceId);

    double RandomErand(RandomSeed seed);

    //External APIs
    EXTERNAL_Interface* GetExternalInterfaceForVRLink(Node* node);

    void SetExternalReceiveDelay(
        EXTERNAL_Interface *iface,
        clocktype delay);

    void SetExternalSimulationEndTime(
        EXTERNAL_Interface* iface,
        clocktype duration);

    clocktype QueryExternalTime(EXTERNAL_Interface *iface);

    clocktype QueryExternalSimulationTime(EXTERNAL_Interface *iface);

    void RemoteExternalSendMessage(
        EXTERNAL_Interface* iface,
        int destinationPartitionId,
        Message* msg,
        clocktype delay,
        ExternalScheduleType scheduling);

    void ChangeExternalNodePositionOrientationAndVelocityAtTime(
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
        double c3Speed);

    virtual void EXTERNAL_ActivateNode(EXTERNAL_Interface *iface, Node *node,
        ExternalScheduleType scheduling = EXTERNAL_SCHEDULE_STRICT,
        clocktype activationEventTime = -1);

    virtual void EXTERNAL_DeactivateNode(EXTERNAL_Interface *iface, Node *node,
        ExternalScheduleType scheduling = EXTERNAL_SCHEDULE_STRICT,
        clocktype deactivationEventTime = -1);

    virtual void EXTERNAL_hton(void* ptr, unsigned int size);
    virtual void EXTERNAL_ntoh(void* ptr, unsigned int size);

    //PHY APIs
    void GetExternalPhyTxPower(
        Node* node,
        int phyIndex,
        double * txPowerPtr);

    void SetExternalPhyTxPower(
        Node* node,
        int phyIndex,
        double newTxPower);

    virtual int GetRxDataRate(Node *node, int phyIndex);

    //MAC APIS
    virtual unsigned short GetDestNPGId(Node* node, int interfaceIndex);

    //Clock APIs
    void PrintClockInSecond(clocktype clock, char stringInSecond[]);
    clocktype GetCurrentSimTime(Node* node);
    clocktype ConvertDoubleToClocktype (double timeInSeconds);
    double ConvertClocktypeToDouble (clocktype timeInNS);

    //Other APIs
    void MoveHierarchyInGUI(
        int hierarchyID,
        Coordinates coordinates,
        Orientation orientation,
        clocktype   time);

    Node* GetNodePtrFromOtherNodesHash(Node* node, NodeAddress nodeId, bool remote);

    BOOL ReturnNodePointerFromPartition(
        PartitionData* partitionData,
        Node** node,
        NodeId nodeId,
        BOOL remoteOK = FALSE);

    NodeAddress GetDefaultInterfaceAddressFromNodeId(
        Node *node,
        NodeAddress nodeId);

    virtual bool IsMilitaryLibraryEnabled();

    virtual const MetricLayerData &getMetricLayerData(int idx);

    virtual int GetCoordinateSystemType(PartitionData* p);

    virtual int GetNumInterfacesForNode(Node* node);

    virtual NodeAddress GetInterfaceAddressForInterface(Node* node, int index);

    virtual int GetPartitionIdForIface(EXTERNAL_Interface *iface);

    virtual PartitionData* GetPartitionForIface(EXTERNAL_Interface *iface);

    virtual void GetMinAndMaxLatLon(EXTERNAL_Interface* iface,
        double& minLat, double& maxLat, double& minLon, double& maxLon);

    virtual NodeId GetNodeId(Node* node);

    virtual NodeInput* GetNodeInputFromIface(EXTERNAL_Interface* iface);

    virtual void GetNodeInputDetailForIndex(NodeInput* input, int index, char** variableName, char** value);

    virtual int GetNumLinesOfNodeInput(NodeInput* input);

    virtual int GetLocalPartitionIdForNode(Node* node);

    virtual int GetRealPartitionIdForNode(Node* node);

    virtual Node* GetFirstNodeOnPartition(EXTERNAL_Interface* iface);

    virtual NodePointerCollection* GetAllNodes(EXTERNAL_Interface* iface);

    virtual ExternalScheduleType GetExternalScheduleSafeType();

    virtual MessageEventTypesUsedInVRLink GetMessageEventTypesUsedInVRLink(int i);

    virtual int GetMessageEventType(MessageEventTypesUsedInVRLink val);

    virtual int GetExternalLayerId();

    virtual int GetVRLinkExternalInterfaceType();

    virtual clocktype GetCurrentTimeForPartition(EXTERNAL_Interface* iface);

    virtual bool GetGuiOption(EXTERNAL_Interface* iface);

    virtual void* GetDataForExternalIface(EXTERNAL_Interface* iface);

    virtual MetricDataTypesUsedInVRLink GetMetricDataTypesUsedInVRLink(int i);

    virtual Int32 GetGlobalSeed(Node* node);

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
        unsigned short numFrags);
};

#endif /* VRLINK_SIM_IFACE_H */
