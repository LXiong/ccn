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

//
// Objectives: Dot16 Traffic Conditioning by remarking/dropping based on
//             bandwidth/delay policy
//


#ifndef MAC_DOT16_TC_H
#define MAC_DOT16_TC_H

//----- Start Internal Macro Definations -----//

#define  DOT16_NUM_INITIAL_TC_INFO_ENTRIES 21

#define   DOT16_TC_NUM_INITIAL_MFTC_PARA_ENTRIES 10
#define   DOT16_TC_CONFORMING                    1
#define   DOT16_TC_PART_CONFORMING               2
#define   DOT16_TC_NON_CONFORMING                3

#define   DOT16_TC_SET_TOS_FIELD(i, tos)        ((i << 2) | ((tos) & 3))
#define   DOT16_TC_UNUSED_FIELD_ADDRESS         INVALID_ADDRESS
#define   DOT16_TC_UNUSED_FIELD                 0xff
#define   DOT16_TC_MAX_STATION_CLASS_NAME       20

//----- End Internal Macro Definations -----//


// /**
// ENUM        :: MacDot16TcConditionType
// DESCRIPTION :: Type of station packet conditioning
// **/
typedef enum
{
    DOT16_TC_CONDITION_HOLD,        // Hold packets that are out of policy
    DOT16_TC_CLASSIFIER_DROP,       // Drop packets that are out of policy
    DOT16_TC_CLASSIFIER_MARK        // Remark packets that are out of policy
} MacDot16TcConditionType;


//----------------------------------------------------------
// Traffic Conditioner Structures
//----------------------------------------------------------

// Function pointer definition for meters
typedef BOOL (*MacDot16MeterFunctionType)(
    Node* node,
    struct struct_tc_data_str* tc,
    Message* msg,
    int conditionClass,
    int* preference);

// Variables of the structure define a unique condition class

// /**
// STRUCT      :: TcParameter
// DESCRIPTION :: Variables of the structure define a unique condition class
// **/
typedef
struct
{
    NodeAddress                 sourceNodeId;
    NodeAddress                 destAddress;
    TosType                     tos;
    unsigned char               protocolId;
    int                         sourcePort;
    int                         destPort;
    MacDot16ServiceFlowDirection  sfDirection;
    int                         conditionClass;
} MacDot16TcParameter;

// /**
// STRUCT      :: ServiceMapInfo
// DESCRIPTION :: Structure used to store tos to service class.
// **/
typedef
struct
{
    TosType             tos;
    // Variables used for service.
    MacDot16ServiceType serviceClass;
    char                serviceClassName[DOT16_TC_MAX_STATION_CLASS_NAME];

}MacDot16TcInfo;

// /**
// STRUCT      :: StationInfo
// DESCRIPTION :: Structure used to store station conditioning info.
// **/
typedef
struct
{
    int                stationClass;                 
    char               stationClassName[DOT16_TC_MAX_STATION_CLASS_NAME];

    // Variables used for metering.
    Int32               dlRate;
    Int32               dlCommittedBurstSize;
    Int32               dlTokens;
    clocktype           dlLastTokenUpdateTime;

    Int32               ulRate;
    Int32               ulCommittedBurstSize;
    Int32               ulTokens;
    clocktype           ulLastTokenUpdateTime;

    MacDot16MeterFunctionType   meterFunction;
    // TODO: add additional meter types

    // Variables used for statistics collection.
    unsigned int        numPacketsIncoming;
    unsigned int        numPacketsConforming;
    unsigned int        numPacketsPartConforming;
    unsigned int        numPacketsNonConforming;

    // Variables used for action taken on profiling.
    MacDot16TcConditionType packetProfile;
    unsigned char       dsToMarkOutProfilePackets;


}MacDot16TcStationInfo;

// STRUCT      :: NetworkData;
// DESCRIPTION :: Main structure of traffic conditioner data.
// **/
typedef
struct struct_tc_data_str
{

    
    MacDot16TcStationInfo*       tcStationInfo;
    int                          numTcStationInfo;
    int                          maxTcStationInfo;

    MacDot16TcInfo*              tcInfo;
    int                          numTcInfo;
    int                          maxTcInfo;

    MacDot16TcParameter*         tcParameter;
    int                          numTcParameters;
    int                          maxTcParameters;

    
    BOOL                         tcStatisticsEnabled;

} MacDot16TcData;

//--------------------------------------------------------------------------
// FUNCTION:    MacDot16TcInit
// PURPOSE:     Initialize the Traffic Classifier/Conditioner
// PARAMETERS:  node - Pointer to the node,
//              nodeInput - configuration Information.
// RETURN:      None.
//--------------------------------------------------------------------------

void MacDot16TcInit(
    Node* node,
    int interfaceIndex,
    const NodeInput* nodeInput);

//--------------------------------------------------------------------------
// FUNCTION    MacDot16TcFinalize()
// PURPOSE     Print out statistics
// PARAMETERS: node    Diffserv statistics of this node
//--------------------------------------------------------------------------

void MacDot16TcFinalize(    
    Node* node,
    int interfaceIndex);


//--------------------------------------------------------------------------
// FUNCTION:    MacDot16GetServiceClass.
// PURPOSE :    the ds belongs to which service class.
// PARAMETERS:  node - pointer to the node,
//              tos - value from the TOS field.
// RETURN:      return Service class or ZERO.
//--------------------------------------------------------------------------

MacDot16ServiceType MacDot16TcGetServiceClass(
    Node* node,
    MacDataDot16* dot16, 
    TosType tos);


//--------------------------------------------------------------------------
// FUNCTION:    MacDot16TcProfilePacketAndPassOrDrop
// PURPOSE :    Analyze outgoing packet to determine whether or not it
//              is in/out-profile, and either pass or drop the packet
//              as indicated by the class of station and policy defined
//              in the configuration file.
// PARAMETERS:  node - pointer to the node,
//              ip - pointer to the IP state variables
//              msg - the outgoing packet
//              interfaceIndex - the outgoing interface
//              packetWasDropped - pointer to a BOOL which is set to
//                   TRUE if the Traffic Conditioner drops the packet,
//                   FALSE otherwise (even if the packet was marked)
//--------------------------------------------------------------------------
/*
void MacDot16TcProfilePacketAndPassOrDrop(
    Node* node,
    MacDot16TcData* tcData,
    Message* msg,
    char* stationClassName,
    MacDot16ServiceFlowDirection sfDirection,
    BOOL* packetWasDropped);
*/
#endif // DOT16_TRAFFIC_CONDITIONER_H
