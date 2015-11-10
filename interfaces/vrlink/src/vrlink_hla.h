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

#ifndef VRLINK_HLA_H
#define VRLINK_HLA_H


// HLA Dynamic Statistics ICD draft 1 value.
const unsigned g_maxOutgoingCommentIxnVariableDatumsSize = 1000;

// QualNet only sends one VRLinkVariableDatum per Comment interaction, so the
// valuebelow is calculated correctly.
const unsigned g_maxOutgoingCommentIxnDatumValueSize
    = g_maxOutgoingCommentIxnVariableDatumsSize
      - sizeof(unsigned)   // DatumId
      - sizeof(unsigned);  // DatumLength

// QualNet only handles one VariableDatum in the VariableDatums field.
const unsigned g_maxVariableDatums = 1;

const unsigned g_variableDatumSetBufSize
    = sizeof(unsigned)        // NumberOfVariableDatums
      + ((sizeof(unsigned)    // DatumID
          + sizeof(unsigned)  // DatumLength
          + g_DatumValueBufSize)
         * g_maxVariableDatums);

const unsigned g_metricUpdateBufSize
    = g_maxOutgoingCommentIxnDatumValueSize;

//Various Datum IDs used for VRLink
const DtDatumParam nodeIdDescriptionNotificationDatumId =DtDatumParam(60101);
const DtDatumParam metricDefinitionNotificationDatumId = DtDatumParam(60102);
const DtDatumParam metricUpdateNotificationDatumId = DtDatumParam(60103);
const DtDatumParam terminateCesNotificationDatumId = DtDatumParam(66900);

// /**
// STRUCT :: VRLinkVariableDatumInfo
// DESCRIPTION  :: Structure of the variable datum info.
// **/
struct VRLinkVariableDatumInfo
{
    DtDatumParam datumId;
    unsigned datumLength;
    unsigned char datumValue[g_DatumValueBufSize];
};

// /**
// STRUCT :: VRLinkVariableDatumSetInfo
// DESCRIPTION :: Structure of the variable datum set info.
// **/
struct VRLinkVariableDatumSetInfo
{
    unsigned numberOfVariableDatums;
    VRLinkVariableDatumInfo variableDatumsInfo[g_maxVariableDatums];
};

// /**
// STRUCT :: VRLinkCommentIxnInfo
// DESCRIPTION :: Structure of the comment interaction info.
// **/
struct VRLinkCommentIxnInfo
{
    VRLinkVariableDatumSetInfo variableDatumSetInfo;

    // No storage for unused parameters:
    // OriginatingEntity
    // ReceivingEntity
};

// /**
// STRUCT :: VRLinkDataIxnInfo
// DESCRIPTION :: Structure of the data interaction info.
// **/
struct VRLinkDataIxnInfo
{
    DtEntityIdentifier originatingEntity;
    VRLinkVariableDatumSetInfo variableDatumSetInfo;

    // No storage for unused parameters:
    // ReceivingEntity
    // RequestIdentifier
    // FixedDatums
};



// /**
// CLASS :: VRLinkHLA
// DESCRIPTION :: Class to store HLA information.
// **/
class VRLinkHLA : public VRLink
{
    protected:
    string execName;
    string fedName;
    string fedPath;

    double rprFomVersion;   
    clocktype tickInterval;

    bool dynamicStats;
    bool verboseMetricUpdates;
    bool sendNodeIdDescriptions;
    bool sendMetricDefinitions;
    char metricUpdateBuf[g_metricUpdateBufSize];
    unsigned int metricUpdateSize;
    clocktype checkMetricUpdateInterval;

    //COMMENTED OUT NAWC GATEWAY CODE, currently unsupported in vrlink
    //bool nawcGatewayCompatibility;
    //clocktype attributeUpdateRequestDelay;
    //clocktype attributeUpdateRequestInterval;
    //unsigned int maxAttributeUpdateRequests;
    //unsigned int numAttributeUpdateRequests;

    public:
    VRLinkHLA(EXTERNAL_Interface* ifacePtr, NodeInput* nodeInput, VRLinkSimulatorInterface *simIface);
    void InitVariables(EXTERNAL_Interface* ifacePtr, NodeInput* nodeInput);
    void CreateFederation();
    void ScheduleTasksIfAny();
    void ProcessEvent(Node* node, Message* msg);
    unsigned int GetMetricUpdateSize();
    unsigned int GetMetricUpdateBufSize();
    void SetMetricUpdateSize(unsigned int size);
    void AppendMetricUpdateBuf(const char* line);
    bool IsVerboseMetricUpdates();
    void RegisterProtocolSpecificCallbacks();
    void ProcessSignalIxn(DtSignalInteraction* inter);

    void AppProcessCheckMetricUpdateEvent(Node* node, Message* msg);
    void SendMetricUpdateNotification();
    void ScheduleCheckMetricUpdate();
    void SendNodeIdDescriptionNotifications();
    void SendMetricDefinitionNotifications();
    void SendCommentIxn(VRLinkVariableDatumSetInfo* vdsInfo);
    void SendDataIxn(VRLinkDataIxnInfo& dataIxnInfo);
    bool GetNawcGatewayCompatibility();

    //callback function(s)
    static void CbDataIxnReceived(DtDataInteraction* inter, void* usr);

    void InitConfigurableParameters(NodeInput* nodeInput);

    void ReadLegacyParameters(NodeInput* nodeInput);

    RadioIdKey GetRadioIdKey(DtSignalInteraction* inter);
    RadioIdKey GetRadioIdKey(DtRadioTransmitterRepository* radioTsr);

    void UpdateMetric(unsigned nodeId,
        const MetricData& metric,
        void* metricValue,
        clocktype updateTime);

    virtual ~VRLinkHLA();
    void DeregisterCallbacks();

    void AppProcessSendRtssEvent(Node* node);
};

// /**
// CLASS :: VRLinkHLA13
// DESCRIPTION :: Class to store HLA1.3 information.
// **/
class VRLinkHLA13 : public VRLinkHLA
{
    public:
    VRLinkHLA13(EXTERNAL_Interface* ifacePtr, NodeInput* nodeInput, VRLinkSimulatorInterface *simIface);
};

// /**
// CLASS :: VRLinkHLA1516
// DESCRIPTION :: Class to store HLA1516 information.
// **/
class VRLinkHLA1516 : public VRLinkHLA
{
    public:
    VRLinkHLA1516(EXTERNAL_Interface* ifacePtr, NodeInput* nodeInput, VRLinkSimulatorInterface *simIface);
};

#endif /* VRLINK_HLA_H */
