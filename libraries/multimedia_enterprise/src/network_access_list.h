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


//--------------------------------------------------------------------------
// File     : access_list.h
// Objective: Header file for access_list.c (CISCO Access List
//              implementation)
// Reference: CISCO document Set
//              http://www.cisco.com/univercd/cc/td/doc/product/software/
//                  ios122/122cgcr/fipras_r/1rfip1.htm#xtocid2
//              For static and Standard Access List refer:
//              http://www.cisco.com/univercd/cc/td/doc/product/software/
//                  ios113ed/113ed_cr/np1_c/1cip.htm#xtocid847415
//              For Reflexive Access Lists refer:
//              http://www.cisco.com/univercd/cc/td/doc/product/software/
//                  ios120/12cgcr/secur_c/scprt3/screflex.htm#13069
//--------------------------------------------------------------------------


#ifndef ACCESS_LIST_H
#define ACCESS_LIST_H

#include "network_ip.h"

// 1-99 for Standard AL, 100-199 for Extended AL
#define ACCESS_LIST_MAX_NUM 199
#define ACCESS_LIST_MIN_STANDARD 1
#define ACCESS_LIST_MIN_EXTENDED 100
#define ACCESS_LIST_MAX_EXTENDED 199
#define ACCESS_LIST_TCP_MAX_PORT 31
#define ACCESS_LIST_UDP_MAX_PORT 23
#define ACCESS_LIST_ICMP_MAX_MESSAGE 47
#define ACCESS_LIST_IGMP_MAX_MESSAGE 4
#define ACCESS_LIST_PRECEDENCE_MAX 8
#define ACCESS_LIST_TOS_MAX 5
#define ACCESS_LIST_LOG_INTERVAL (5 * MINUTE)
#define ACCESS_LIST_REMARK_STRING_SIZE 100
#define ACCESS_LIST_ADDRESS_STRING 20

#define ACCESS_LIST_MIN_REFLEX_TIMEOUT (0 * SECOND)
#define ACCESS_LIST_MAX_REFLEX_TIMEOUT (2147483 * SECOND)
#define ACCESS_LIST_DEFAULT_REFLEX_TIMEOUT (300 * SECOND)
#define ACCESS_LIST_TCP_FIN_SET_REFLEX_TIMEOUT (5 * SECOND)


//--------------------------------------------------------------------------
//                  ENUMERATIONS's
//--------------------------------------------------------------------------


// We create a three state variable, TRUE, FALSE, ACCESS_LIST_NA
typedef enum
{
    ACCESS_LIST_MATCH,
    ACCESS_LIST_NO_MATCH,
    ACCESS_LIST_NA
}AccessListTriStateType;


// Holding the rules of access list
typedef enum
{
    ACCESS_LIST_PERMIT,
    ACCESS_LIST_DENY
} AccessListFilterType;


// Operators for the port
typedef enum
{
    ACCESS_LIST_EQUALITY,       // if ports are equal, eq
    ACCESS_LIST_GREATER_THAN,   // if ports are greater than, gt
    ACCESS_LIST_LESS_THAN,      // if ports are less than, lt
    ACCESS_LIST_NOT_EQUAL,      // if ports are not equal, ne
    ACCESS_LIST_RANGE           // needs two ports, range
} AccessListOperatorType;


// Configured at input or output interface
typedef enum
{
    ACCESS_LIST_IN,
    ACCESS_LIST_OUT
} AccessListInterfaceType;


// Whether log or type of log is enabled
typedef enum
{
    ACCESS_LIST_NO_LOG,     // log option is missing
    ACCESS_LIST_LOG,        // log is enabled
    ACCESS_LIST_LOG_INPUT   // input-type log
} AccessListLogType;


// Type of timers present
typedef enum
{
    ACCESS_LIST_LOG_TIMER,
    ACCESS_LIST_REFLEX_TIMEOUT_TIMER,
    ACCESS_LIST_REFLEX_TCP_TIMEOUT_TIMER
} AccessListTimerType;



//--------------------------------------------------------------------------
//                  STRUCT TYPEDEF's
//--------------------------------------------------------------------------


// name value pair for reading port name and number
typedef struct
struct_access_list_tcp_port
{
    char* portName;
    int portNumber;
} AccessListTCPPortName;


typedef struct
struct_access_list_udp_port
{
    char* portName;
    int portNumber;
} AccessListUDPPortName;


// these fields are specific to TCP / UDP
typedef struct
struct_access_list_TCP_or_UDP_parameters
{
    AccessListOperatorType srcOperator; // type of operator
    int srcPortNumber;                  // source port
    AccessListOperatorType destOperator;// type of operator
    int destPortNumber;                 // destination port

    // these are typically used for range operator type
    // port from where range begins
    int initialPortNumber;
    int finalPortNumber;                // port from where range ends
} AccessListTCPorUDPParams;

//--------------------------------------------------------------------------
// ref: CISCO user guide
//      RFC-792
//      http://www.iana.org/assignments/icmp-parameters
// 256 and 0 means type and code not found in RFC
// this table is not accurate
//--------------------------------------------------------------------------

typedef struct
struct_access_list_icmp_message_name
{
    char* icmpName;
    int icmpType;
    int icmpCode;
} AccessListICMPMessageName;


// these fields are specific to ICMP
typedef struct
struct_access_list_ICMP_parameters
{
    // number between 0 to 255
    int icmpType;
    int icmpCode;
} AccessListICMPParams;


typedef struct
struct_access_list_igmp_message_name
{
    char* igmpName;
    int igmpType;
} AccessListIGMPMessageName;


// these fields are specific to IGMP
typedef struct
struct_acess_list_IGMP_parameters
{
    unsigned char igmpType;
} AccessListIGMPParams;


// holds precedence name and values
typedef struct
struct_access_list_precedence
{
    char* precedenceName;
    int precedenceValue;
} AccessListPrecedence;


// name value for TOS
typedef struct
struct_access_list_tos
{
    char* tosName;
    int tosValue;
} AccessListTOS;


// over all structure to hold all the nme value pairs used in access_list.c
typedef struct
struct_access_list_name_value_list
{
    AccessListTCPPortName tcpPortNameAndNumber[ACCESS_LIST_TCP_MAX_PORT];
    AccessListUDPPortName udpPortNameAndNumber[ACCESS_LIST_UDP_MAX_PORT];

    // ICMP and IGMP are not properly initialized for insufficiency of data
    AccessListICMPMessageName icmpMessageNameTypeAndCode
        [ACCESS_LIST_ICMP_MAX_MESSAGE];

    AccessListIGMPMessageName igmpMessageNameType
        [ACCESS_LIST_IGMP_MAX_MESSAGE];

    AccessListPrecedence precedenceList[ACCESS_LIST_PRECEDENCE_MAX];
    AccessListTOS tosList[ACCESS_LIST_TOS_MAX];
} AccessListParameterNameValue;

//--------------------------------------------------------------------------
// for reflexive access list
// ref: http://www.cisco.com/univercd/cc/td/doc/product/software/ios120/
//  12cgcr/secur_c/scprt3/screflex.htm#14798
//--------------------------------------------------------------------------

typedef struct
struct_access_list_reflex
{
    int sessionId;              // associated session id
    int position;               // the position of plugging
    char* name;                 // name of reflex access List
    clocktype timeout;          // timeout time
    clocktype lastPacketSent;   // last packet sent time

    // check for TWO set FIN bits for session close
    int countFIN;

    struct struct_access_list_reflex* next;
} AccessListReflex;


typedef struct
struct_access_list_reflex_nest
{
    int position;           // occurence of nest
    char* name;             // name of reflex access list
    char* accessListName;   // extended access list holding te reflex
    struct struct_access_list_reflex_nest* next;
} AccessListNest;


typedef struct
struct_access_list_session_info
{
    int id; // session id for removing the dynamic access lists
    unsigned char protocol; // protocol
    NodeAddress srcAddr;    // holds source address
    NodeAddress destAddr;   // holds desination address
    int srcPortNumber;      // source port
    int destPortNumber;     // destination port
    BOOL isRemoved;
    struct struct_access_list_session_info* next;
} AccessListSessionList;

// pointer to access list
typedef struct
struct_access_list
{
    AccessListFilterType filterType;
    BOOL isStandard;
    unsigned char protocol;     // refer #defines in ip.h
    NodeAddress srcAddr;        // holds source address
    NodeAddress srcMaskAddr;    // holds source wildcard
    NodeAddress destAddr;       // holds desination address
    NodeAddress destMaskAddr;   // holds destination wildcard

    // individual protocols have specific sub-fields in Access List
    AccessListICMPParams* icmpParameters;
    AccessListIGMPParams* igmpParameters;
    AccessListTCPorUDPParams* tcpOrUdpParameters;

    signed char precedence;     // holds precendence value
    signed char tos;            // holds tos value

    // this one is specific to TCP. Whether Conxn is established or not
    BOOL established;

    AccessListLogType logType;  // type of log asked, if any, to display
    BOOL logOn;                 // whether log is already triggered
    int packetCount;            // number of packets matched for log display
    BOOL timerSet;              // is timer set for log

    // for name access list, criterias at next line
    BOOL continueNextLine;

    // CISCO allows a 100 character size string to add comments to the
    // access lists for commented access list
    char remark[MAX_STRING_LENGTH];

    // used for logging
    int accessListNumber;
    char* accessListName;

    BOOL isFreed;
    BOOL isProxy;

    // used in reflex access list
    // does this access list has a reflect
    BOOL isReflectDefined;
    BOOL isDynamicallyCreated;

    // hold reflex list parameters
    AccessListReflex* reflexParams;

    struct struct_access_list* next;
} AccessList;


//--------------------------------------------------------------------------
// The name list follows that a given name can be either standard or
// extended, but not both for a given node. No such example found in CISCO
// doc which shows the other
//--------------------------------------------------------------------------

typedef struct
struct_access_list_name
{
    char* name;             // name of access List
    BOOL isStandardType;    // type of access list, TRUE for Standard
    AccessList* accessList; // list of criteria's with same name
    struct struct_access_list_name* next;   // get next name
} AccessListName;


typedef struct
struct_access_list_pointer
{
    AccessList** accessList;
    struct struct_access_list_pointer* next;
} AccessListPointer;


// message info structure
typedef struct
struct_access_list_message_info
{
    int sessionId;
    AccessList* accessList;
    AccessListTimerType timerType;
} AccessListMessageInfo;


// structure for Access list statistics
typedef struct
struct_access_list_stats
{
    // packets dropped by standard access list at IN interface
    UInt32 packetDroppedByStdAtIN;

    // packets dropped by standard access list at OUT interface
    UInt32 packetDroppedByStdAtOut;

    // packets dropped by extended access list at IN interface
    UInt32 packetDroppedByExtdAtIN;

    // packets dropped by extended access list at OUT interface
    UInt32 packetDroppedByExtdAtOUT;

        // packets dropped for mismatch
    UInt32 packetDroppedForMismatchAtIN;

    // packets dropped for mismatch
    UInt32 packetDroppedForMismatchAtOut;
} AccessListStats;



//--------------------------------------------------------------------------
//                  FUNCTION DECLARATION'S FOLLOW
//--------------------------------------------------------------------------

//-----------------------------------------------------------
//  FUNCTION  AccessListInit
//  PURPOSE   To initialize networks access list reading from router
//            configuratoin file
//  ARGUMENTS node, The node initializing
//            nodeInput, Main configuration file
//  RETURN    None
//  NOTE      The router configuration file should be in the following format
//            # followed by configuration for node <node id1>
//            ROUTER <node id1>
//            ACCESS-LIST <access list number> .....
//              .
//              .
//            IP ACCESS-GROUP <access list number> .....
//              .
//              .
//            # followed by configuration for node <node id1>
//            ROUTER <node id1>
//              .
//              .
//-----------------------------------------------------------
void AccessListInit(Node* node, const NodeInput* nodeInput);

//-----------------------------------------------------------
// FUNCTION  AccessListFilterPacket
// PURPOSE   Filter a packet on the basis of its match with the access list
// ARGUMENTS node, The node initializing
//           msg, packet pointer
//           accessListPointer, pointer to access list pointer
//           interfaceIndex, interface ID
//           interfaceType, type of interface, for trace
// RETURN    BOOL, returns result of whether we should drop the packet. TRUE
//              for dropping.
//-----------------------------------------------------------
BOOL AccessListFilterPacket(Node* node,
                            Message* msg,
                            AccessListPointer* accessListPointer,
                            int interfaceIndex,
                            AccessListInterfaceType interfaceType);


//-----------------------------------------------------------
// FUNCTION   AccessListHandleEvent
// PURPOSE    Handle the various access list event
// PARAMETERS node - this node.
//            msg, message to recieve or transmit
// RETURN     None
//-----------------------------------------------------------
void AccessListHandleEvent(Node* node, Message* msg);


//--------------------------------------------------------------------------
// FUNCTION  AccessListFinalize
// PURPOSE   Access list functionalities handled during termination.
//              Here we print the various access list statistics.
// ARGUMENTS node, current node
// RETURN    None
//--------------------------------------------------------------------------
void AccessListFinalize(Node* node);





//--------------------------------------------------------------------------
//                     CISCO IOS COMMAND'S
//--------------------------------------------------------------------------

//-----------------------------------------------------------
// FUNCTION  AccessListClearCounters
// PURPOSE   Check a packet for matching with the access list
// ARGUMENTS node, node concerned
//           accessListId, name or number characterising the Access List
// RETURN    None
//-----------------------------------------------------------
void AccessListClearCounters(Node* node, char* accessListId);

//-----------------------------------------------------------
// FUNCTION  AccessListNoACL
// PURPOSE   remove the access list
// ARGUMENTS node, node concerned
//           accessListId, name or number characterising the Access List
// RETURN    None
//-----------------------------------------------------------
void AccessListNoACL(Node* node, char* accessListId);

//-----------------------------------------------------------
// FUNCTION  AccessListShowACL
// PURPOSE   Show the access list (IOS function)
// ARGUMENTS node, node concerned
//           accessListId, name or number characterising the Access List
// RETURN    None
//-----------------------------------------------------------
void AccessListShowACL(Node* node, char* accessListId);


//--------------------------------------------------------------------------
// FUNCTION    AccessListVerifyRouteMap
// PURPOSE     Verify the match conditions of route map with the given
//              access list id (name / number)
// ARGUMENTS   node, node concerned
//             accessListId, name or number charactersing the Access List
//             destAdd, the destination address or network address to be
//                  matched
// RETURN      BOOL, the match result
//--------------------------------------------------------------------------
BOOL AccessListVerifyRouteMap(Node* node,
                              char* accessListId,
                              NodeAddress srcAddress,
                              NodeAddress destAddress);
#endif // ACCESS_LIST_H
