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

#ifndef MAC_DOT16_QOS_H
#define MAC_DOT16_QOS_H

//
// This is the header file of the implementation of QOS of IEEE 802.16 MAC
//

// /**
// CONSTANT    :: DOT16_QOS_NUM_SERVICE_CLASS_RECORD : 5
// DESCRIPTION :: Initial size of the array for keeping the list
//                of established service class records
// **/
#define DOT16_QOS_NUM_SERVICE_CLASS_RECORD    5

// /**
// CONSTANT    :: DOT16_QOS_NUM_APP_SERVICE_RECORD : 20
// DESCRIPTION :: Initial size of the array for keeping the list
//                of established service class records
// **/
#define DOT16_QOS_NUM_APP_SERVICE_RECORD    20

// /**
// STRUCT      :: MacDot16QosTableEntry
// DESCRIPTION :: Data structure to hold information for aggregate table
// **/
typedef struct struct_dot16_qos_table_entry {
    int totalBytes;
    float usedTput;
    clocktype avgExpDelay;
} MacDot16QosTableEntry;

// /**
// STRUCT      :: MacDot16QosAggTable
// DESCRIPTION :: Table containing information from aggregation of flows
// **/
typedef struct struct_dot16_qos_agg_table {
    MacDot16QosTableEntry usg;
    MacDot16QosTableEntry rtps;
    MacDot16QosTableEntry nrtps;
    MacDot16QosTableEntry be;
    float availableTput;
} MacDot16QosAggTable;

// /**
// STRUCT      :: MacDot16QosServiceRecord
// DESCRIPTION :: Data structure contains the QOS service and related info
// **/
typedef struct mac_dot16_qos_service_record_str
{
    MacDot16ServiceClass classType;     // indicate service class type
    unsigned int appServiceId;          // to uniquely and easily identify
                                        // a app service record

    MacDot16QoSParameter* parameters; // pointer to the actual QOS service
                                      // parameters structure

    AppType appType;        // Application protocol types

    clocktype invalidTime;  // The time stamp when the record is marked
                            // as invalidated
} MacDot16QosAppServiceRecord;

// /**
// STRUCT      :: MacDot16QosServiceClassRecord
// DESCRIPTION :: Data structure contains the defined QOS service class info
// **/
typedef struct mac_dot16_qos_service_class_record_str
{
    unsigned int serviceClassId;                    // record class id
    char stationClassName[20];                      // station class name
    MacDot16StationClass stationClass;              // indicate stations
                                                    // service class type
    MacDot16QoSParameter* parameters;               // pointer to the actual
                                                    // QOS service
                                                    // parameters structure
} MacDot16QosServiceClassRecord;

// /**
// STRUCT      :: MacDot16QosData
// DESCRIPTION :: Data structure of Dot16 QOS
// **/
typedef struct struct_mac_dot16_qos_str
{
    int appServiceIds; // counter used to assign ID to app services

    // list of possable service classes
    MacDot16QosAppServiceRecord** appServiceRecords;
    int numAppServiceRecords;
    int allocAppServiceRecords;

    int serviceClassIds; // counter used to assign ID to service class

    // list of possable service classes
    MacDot16QosServiceClassRecord** serviceClassRecords;
    int numServiceClassRecords;
    int allocServiceClassRecords;

    MacDot16QosAggTable* qosAggTable;

    MacDot16StationClass stationClass;
    char* stationClassName[20];

} MacDot16Qos;


//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: MacDot16QosUpdateQoSParameters
// LAYER      :: MAC
// PURPOSE    :: Retrieve QoS parameters of a service flow.
//
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + dot16          : MacDataDot16*          : Pointer to DOT16 structure
// + serviceType    : MacDot16ServiceType    : Data service type of this
//                                             flow
// + classifierInfo : MacDot16CsClassifier*  : classifer info of this flow
// + payload        : char*                  : Packet payload
// + serviceClass   : MacDot16StationClass   : Assigned station class
// + qosInfo        : MacDot16QoSParameter*  : To store returned QoS
//                                             parameter
// RETURN     :: void : NULL
// **/
void MacDot16QosUpdateParameters(
         Node* node,
         MacDataDot16* dot16,
         TosType priority,
         TraceProtocolType appType,
         MacDot16CsClassifier* classifierInfo,
         char** payload,
         MacDot16StationClass* stationClass,
         MacDot16QoSParameter* qosInfo,
         int recPacketSize);

// /**
// FUNCTION   :: MacDot16QosAddService
// LAYER      :: MAC
// PURPOSE    :: Add a service for an application.
//
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// RETURN     :: void : NULL
// **/
void MacDot16QosAddService(
         Node* node);

// /**
// FUNCTION   :: MacDot16QosRemoveService
// LAYER      :: MAC
// PURPOSE    :: Remove service for an application.
//
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// RETURN     :: void : NULL
// **/
void MacDot16QosRemoveService(
         Node* node);

// /**
// FUNCTION   :: MacDot16QosSetServiceParameters
// LAYER      :: MAC
// PURPOSE    :: Set service for an application.
//
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + qosInfo   : MacDot16QoSParameter* : To store returned QoS parameter
// RETURN     :: void : NULL
// **/
void MacDot16QosSetServiceParameters(
         Node* node,
         MacDot16QoSParameter* qosInfo);

// /**
// FUNCTION   :: MacDot16QosGetClassParameters
// LAYER      :: MAC
// PURPOSE    :: Get class parameters for a service.
//
// PARAMETERS ::
// + node      : Node*                  : pointer to node.
// + serviceId : unsigned int           : service class id
// + qosInfo   : MacDot16QoSParameter*  : to store returned QoS parameter
// RETURN     :: void : NULL
// **/
void MacDot16QosGetServiceParameters(
         Node* node,
         unsigned int serviceId,
         MacDot16QoSParameter* qosInfo);

// /**
// FUNCTION   :: MacDot16QosGetStationClassInfo
// LAYER      :: MAC
// PURPOSE    :: Get service class info.
//
// PARAMETERS ::
// + node           : Node*                  : pointer to node.
// + dot16          : MacDataDot16*          : Pointer to DOT16 structure
// + serviceClass   : MacDot16StationClass   : stations class
// + qosInfo        : MacDot16QoSParameter*  : to store returned QoS
//                                             parameter
// RETURN     :: int : recode id
// **/
int MacDot16QosGetStationClassInfo(
         Node* node,
         MacDataDot16* dot16,
         MacDot16StationClass stationClass,
         MacDot16QoSParameter* qosInfo);


// /**
// FUNCTION   :: Dot16QosAddServiceClass
// LAYER      :: MAC
// PURPOSE    :: Called from app to add a service
//
// PARAMETERS ::
// + node               : Node*         : the node pointer
// + interfaceIndex     : unsigned int  : interface index of DOT16
// + serviceClassType   :MacDot16ServiceClass   : the service class
// + minPktSize         :int            : smallest packet size
// + maxPktSize         :int            : largest packet size
// + maxSustainedRate   :int            : maximum sustained traffic,
//                                        per second
// + minReservedRate    :int            : minimum reserved traffic rate,
//                                        per second
// + maxLatency         :clocktype      : maximum latency
// + toleratedJitter    :clocktype      : tolerated jitter
// RETURN     :: unsigned int : serviceClassId
// **/
unsigned int Dot16QosAddServiceClass(
                   Node* node,
                   unsigned int interfaceIndex,
                   MacDot16ServiceClass serviceClassType,
                   int minPktSize,
                   int maxPktSize,
                   int maxSustainedRate,
                   int minReservedRate,
                   clocktype maxLatency,
                   clocktype toleratedJitter);

// /**
// FUNCTION   :: MacDot16QosInit
// LAYER      :: MAC
// PURPOSE    :: Initialize DOT16 QOS.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacDot16QosInit(
        Node* node,
        int interfaceIndex,
        const NodeInput* nodeInput);



// /**
// FUNCTION   :: MacDot16QosAppAddService
// LAYER      :: MAC
// PURPOSE    :: Called from app to add a service
//
// PARAMETERS ::
// + node               : Node*                 : the node pointer
// + interfaceIndex     : unsigned int          : Interface index of DOT16
// + priority           :TosType                : priority TOS of the flow
// + sourceAddr         :NodeAddres             : sourceAddr host address
// + sourcePort         :short                  : sourcePort host port
// + destAddr           :NodeAddress            : destAddr address
// + destPort           :short                  : destPort port
// + qosInfo            :MacDot16QoSParameter*  : QOS info for new flow
// RETURN     :: int : the serviceId   ( -1 if failed)
// **/
int MacDot16QosAppAddService(
                   Node* node,
                   unsigned int interfaceIndex,
                   TosType priority,
                   NodeAddress sourceAddr,
                   short sourcePort,
                   NodeAddress destAddr,
                   short destPort,
                   MacDot16QoSParameter* qosInfo);


// /**
// FUNCTION   :: MacDot16QosAppUpdateService
// LAYER      :: MAC
// PURPOSE    :: Called from app to update a service's parameters
//
// PARAMETERS ::
// + node           : Node*                 : the node pointer
// + serviceId      :unsigned int           : id of the service to update
// + qosInfo        :MacDot16QoSParameter*  : updated QOS info for flow
// RETURN     :: int : the serviceId   ( -1 if failed)
// **/
int MacDot16QosAppUpdateService(
                   Node* node,
                   unsigned int serviceId,
                   MacDot16QoSParameter* qosInfo);


// /**
// FUNCTION   :: MacDot16QosAppDropService
// LAYER      :: MAC
// PURPOSE    :: Called from app to drop a service flow
//
// PARAMETERS ::
// + node           : Node*         : the node pointer
// + scId           : unsigned int  : the service's id to drop
// RETURN     :: int : serviceId  (-1 if failed)
// **/
int MacDot16QosAppDropService(
                   Node* node,
                   unsigned int serviceId);

#if 0

 // /**
// FUNCTION   :: MacDot16QosAppDone
// LAYER      :: MAC
// PURPOSE    :: Called from app to terminate service flow and cleanup
//
// PARAMETERS ::
// + node           : Node*                 : the node pointer
// + interfaceIndex : unsigned int          : interface index of DOT16
// + serviceId      :unsigned int           : id of the service to update
// RETURN     :: BOOL : TRUE - Removed
//                      FALSE - Failed
// **/
void MacDot16QosAppDone(
                     Node* node,
                     unsigned int interfaceIndex,
                     unsigned int serviceId);
#endif

#endif // MAC_DOT16_QOS_H
