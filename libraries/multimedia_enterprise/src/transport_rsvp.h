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

#ifndef RSVP_H
#define RSVP_H
#include "list.h"

#define RSVPv1                         0x01   // current RSVP is version 1

// Defination of Path and Resv state refresh delay
#define RSVP_STATE_REFRESH_DELAY        30000  // 30 second, in milli second
#define RSVP_MAX_STATE_REFRESH_DELAY    1.5 * RSVP_STATE_REFRESH_DELAY
#define RSVP_MIN_STATE_REFRESH_DELAY    0.5 * RSVP_STATE_REFRESH_DELAY

// The delay of Hello refresh timer is normally set to the following value
#define RSVP_HELLO_TIMER_DELAY    (5 * MILLI_SECOND)

// Delay for explicit route message. required to activate it
#define RSVP_EXPLICIT_ROUTE_TIMER_DELAY (7 * MINUTE)

#define RSVP_REFRESH_TIME_CONSTANT_K   3.0

#define L3PID   100   // TBD

// Maximum count of non matching of RSVP_HELLO message
#define MAX_NON_MATCHING_COUNT  10

// Maximum number of allowed adress in explicit route
#define MAX_EXPLICIT_ADDRESS_COUNT 24

// For debugging
#define MAX_IP_STRING_LENGTH  16

// The following enumeration shows the unique class number for all the RSVP
// objects. The object must contain proper class Num with the C-Type also.
typedef enum
{
    RSVP_NULL_OBJECT       = 0,   // 0
    RSVP_SESSION_OBJECT,          // 1
    RSVP_HOP               = 3,   // 3
    RSVP_INTEGRITY,               // 4
    RSVP_TIME_VALUES,             // 5
    RSVP_ERROR_SPEC,              // 6
    RSVP_SCOPE,                   // 7
    RSVP_STYLE,                   // 8
    RSVP_FLOWSPEC,                // 9
    RSVP_FILTER_SPEC,             // 10
    RSVP_SENDER_TEMPLATE,         // 11
    RSVP_SENDER_TSPEC,            // 12
    RSVP_ADSPEC,                  // 13
    RSVP_POLICY_DATA,             // 14
    RSVP_RESV_CONFIRM,            // 15
    RSVP_LABEL_OBJECT,            // 16
    RSVP_LABEL_REQUEST     = 19,  // 19
    RSVP_EXPLICIT_ROUTE,          // 20
    RSVP_RECORD_ROUTE,            // 21
    RSVP_HELLO,                   // 22
    RSVP_SESSION_ATTRIBUTE = 207  // 207
} RsvpObjectClassNum;


// All the RSVP messages are described in the following enumeration.
typedef enum
{
    RSVP_PATH_MESSAGE = 1,
    RSVP_RESV_MESSAGE,
    RSVP_PATH_ERR_MESSAGE,
    RSVP_RESV_ERR_MESSAGE,
    RSVP_PATH_TEAR_MESSAGE,
    RSVP_RESV_TEAR_MESSAGE,
    RSVP_RESV_CONF_MESSAGE,
    RSVP_HELLO_MESSAGE = 20
} RsvpMsgType;


// Different types of services (general, controlled and guaranteed) are
// described here.
typedef enum
{
    RSVP_DEFAULT_SERVICE    = 1,
    RSVP_GUARANTEED_SERVICE,
    RSVP_CONTROLLED_SERVICE = 5
} RsvpServiceNumber;


// Different possible parameter types to describe any services. This is used
// when creating RSVP_SENDER_TSPEC, RSVP_FLOWSPEC and RSVP_ADSPEC object.
typedef enum
{
    RSVP_NUMBER_OF_IS_HOP          = 4,
    RSVP_PATH_BANDWIDTH            = 6,
    RSVP_MINUMUM_PATH_LATENCY      = 8,
    RSVP_COMPOSED_PATH_MTU         = 10,
    RSVP_TOKEN_BUCKET_T_SPEC       = 127,
    RSVP_GUARANTEED_SERVICE_R_SPEC = 130,
    RSVP_COMPOSED_C_TOT            = 133,
    RSVP_COMPOSED_D_TOT,
    RSVP_COMPOSE_C_SUM,
    RSVP_COMPOSE_D_SUM
} RsvpParameterId;


// Different RSVP error codes are enumerated here.
typedef enum
{
    RSVP_CONFIRMATION_ERR,
    RSVP_ADMISSION_CONTROL_FAILURE,
    RSVP_POLICY_CONTROL_FAILURE,
    RSVP_NO_PATH_INFO_FOR_RESV,
    RSVP_NO_SENDER_INFO_FOR_RESV,
    RSVP_CONFLICTING_RESERVATION_STYLE,
    RSVP_UNKNOWN_RESERVATION_STYLE,
    RSVP_CONFLICTING_DEST_PORT,
    RSVP_CONFLICTING_SENDER_PORT,
    RSVP_SERVICE_PREEMPTED            = 12,
    RSVP_UNKNOWN_OBJECT_CLASS,
    RSVP_UNKNOWN_OBJECT_C_TYPE,
    RSVP_TRAFFIC_CONTROL_ERR          = 21,
    RSVP_TRAFFIC_CONTROL_SYSTEM_ERROR,
    RSVP_SYSTEM_ERR,
    RSVP_ROUTING_PROBLEM,
    RSVP_NOTIFY
} RsvpErrorCode;


// Different types of error value for the error RSVP_ROUTING_PROBLEM are
// enumerated below.
typedef enum
{
    RSVP_TE_BAD_EXPLICIT_ROUTE = 1,
    RSVP_TE_BAD_STRICT_NODE,
    RSVP_TE_BAD_LOOSE_NODE,
    RSVP_TE_BAD_INITIAL_SUB_OBJECT,
    RSVP_TE_NO_ROUTE_TOWARD_DESTINATION,
    RSVP_TE_UNACCEPTABLE_LABEL_VALUE,
    RSVP_TE_RRO_INDICATED_ROUTING_LOOPS,
    RSVP_TE_MPLS_BEING_NEGOTIATED,
    RSVP_TE_MPLS_LABEL_ALLOCATION_FAILURE,
    RSVP_TE_UNSUPPORTED_L3PID
} RsvpTeRoutingProblemErrorValue;


// Different record route type, collected from config.in
typedef enum
{
    RSVP_RECORD_ROUTE_NONE,   // RSVP_RECORD_ROUTE not required
    RSVP_RECORD_ROUTE_NORMAL, // RSVP_RECORD_ROUTE required
    RSVP_RECORD_ROUTE_LABELED // RSVP_RECORD_ROUTE required with label
                              // recording
} RsvpRecordRouteType;


// The three different reservation styles are enumerated as below.
typedef enum
{
    RSVP_FF_STYLE = 0x0a,     // binary combination 01010b
    RSVP_SE_STYLE = 0x12,     // binary combination 10010b
    RSVP_WF_STYLE = 0x11      // binary combination 10001b
} RsvpStyleType;


// Different flags used in RSVP_SESSION_ATTRIBUTE object are
// enumerated as below.
typedef enum
{
    RSVP_LOCAL_PROTECTION_DESIRED  = 0x01,
    RSVP_LABEL_RECORDING_DESIRED   = 0x02,
    RSVP_SE_STYLE_DESIRED          = 0x04
} RsvpSessionAttributeFlags;


// Two types of Hello objects are enumerated as below.
typedef enum
{
    RSVP_HELLO_REQUEST = 1,
    RSVP_HELLO_ACK
} RsvpTeHelloObjectType;


// The RSVP message consists of a common header with the body with several
// RSVP objects. The structure of the common header is given here.
typedef struct common_header
{
    unsigned char  rsvpCh;//versionNumber:4,   // protocol version number,
                                      // version is 1

                   //flags:4;           // reserved right now
    unsigned char  msgType;           // different message type
    unsigned short rsvpChecksum;      // normally set to 0 in Qualnet
    unsigned char  sendTtl;           // TTL with which message is sent
    unsigned char  reserved;          // reserved bit not used
    unsigned short rsvpLength;        // total length of the RSVP message
} RsvpCommonHeader;

//-------------------------------------------------------------------------
// FUNCTION       RsvpCommonHeaderSetVersionNum()
// PURPOSE        Set the value of version number for RsvpCommonHeader
//
// Return:         void
// Parameters:
//    rsvpCh:    The variable containing the value of versionNumber and flags
//    vNum:      Input value for set operation
//-------------------------------------------------------------------------
static void RsvpCommonHeaderSetVersionNum(unsigned char *rsvpCh, UInt32 vNum)
{
    unsigned char vnum_char = (unsigned char)vNum;

    //masks versionNumber within boundry range
    vnum_char = vnum_char & maskChar(5, 8);

    //clears first four bits
    *rsvpCh = *rsvpCh & maskChar(5, 8);

    //setting the value of version number in rsvpCh
    *rsvpCh = *rsvpCh |LshiftChar(vnum_char, 4);
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpCommonHeaderSetFlags()
// PURPOSE        Set the value of flags for RsvpCommonHeader
//
// Return:         void
// Parameters:
//    rsvpCh:    The variable containing the value of versionNumber and flags
//    flags:     Input value for set operation
//-------------------------------------------------------------------------
static void RsvpCommonHeaderSetFlags(unsigned char *rsvpCh, UInt8 flags)
{
    //masks flags within boundry range
    flags = flags & maskChar(5, 8);

    //clears last four bits
    *rsvpCh = *rsvpCh & maskChar(1, 4);

    //setting the value of flags in rsvpCh
    *rsvpCh = *rsvpCh | flags;
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpCommonHeaderGetVersionNum()
// PURPOSE        Returns the value of version number for RsvpCommonHeader
//
// Return:         UInt32
// Parameters:
//    rsvpCh:    The variable containing the value of versionNumber and flags
//-------------------------------------------------------------------------
static UInt32 RsvpCommonHeaderGetVersionNum(unsigned char rsvpCh)
{
    unsigned char vNum = rsvpCh;

    //clears last 4 bits
    vNum = vNum & maskChar(1, 4);

    //Right shift so that last 4 bits represents version number
    vNum = RshiftChar(vNum, 4);

    return (UInt32)vNum;
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpCommonHeaderGetFlags()
// PURPOSE        Returns the value of flags for RsvpCommonHeader
//
// Return:         UInt8
// Parameters:
//    rsvpCh:    The variable containing the value of versionNumber and flags
//-------------------------------------------------------------------------
static UInt8 RsvpCommonHeaderGetFlags(unsigned char rsvpCh)
{
    UInt8 flags = rsvpCh;

    //clears first 4 bits
    flags = flags & maskChar(5, 8);

    return flags;
}


// Every RSVP object consists of one or more 32-bit words with one-word
// header called object header. The format of that structure is given below
typedef struct object_header
{
    unsigned short objectLength;      // Length of the object, multiple of 4
    unsigned char  classNum;          // identifies RSVP object
    unsigned char  cType;             // object type within same class
} RsvpObjectHeader;


// Following structure describes 32-bit message header specifying the message
// format version number and total length of the message.
typedef struct message_header
{
    UInt16 rsvpMsgVer;
                   //version:4,         // message format version number
                   //reserved1:12;      // next 12 bit is reserved
    unsigned short overalLength;      // total length of the message
} RsvpMessageHeader;

//-------------------------------------------------------------------------
// FUNCTION       RsvpMessageHeaderSetVersion()
// PURPOSE        Set the value of version for RsvpMessageHeader
//
// Return:         void
// Parameters:
//   rsvpMsgVer:  The variable containing the value of version and reserved1
//   version:     Input value for set operation
//-------------------------------------------------------------------------
static void RsvpMessageHeaderSetVersion(UInt16 *rsvpMsgVer, UInt16 version)
{
    //masks version within boundry range
    version = version & maskShort(13, 16);

    //clears first 4 bits
    *rsvpMsgVer = *rsvpMsgVer & maskShort(5, 16);

    //setting the value of version in rsvpMsgVer
    *rsvpMsgVer = *rsvpMsgVer | LshiftShort(version, 4);
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpMessageHeaderSetRsrvd1()
// PURPOSE        Set the value of reserved1 for RsvpMessageHeader
//
// Return:         void
// Parameters:
//   rsvpMsgVer:  The variable containing the value of version and reserved1
//   reserved1:   Input value for set operation
//-------------------------------------------------------------------------
static void RsvpMessageHeaderSetRsrvd1(UInt16 *rsvpMsgVer,
                                            UInt16 reserved1)
{
    //masks reserved1 within boundry range
    reserved1 = reserved1 & maskShort(5, 16);

    //clears last 12 bits
    *rsvpMsgVer = *rsvpMsgVer & maskShort(1, 4);

    //setting the value of reserved in rsvpMsgVer
    *rsvpMsgVer = *rsvpMsgVer | reserved1;
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpMessageHeaderGetVersion()
// PURPOSE        Returns the value of version for RsvpMessageHeader
//
// Return:         UInt16
// Parameters:
//   rsvpMsgVer:  The variable containing the value of version and reserved1
//-------------------------------------------------------------------------
static UInt16 RsvpMessageHeaderGetVersion(UInt16 rsvpMsgVer)
{
    UInt16 version= rsvpMsgVer;

    //clears the first 4 bits
    version = version & maskShort(1, 4);

    //Right shift so that last 4 bits represent version
    version = RshiftShort(version, 4);

    return version;
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpMessageHeaderGetRsrvd1()
// PURPOSE        Returns the value of reserved1 for RsvpMessageHeader
//
// Return:         UInt16
// Parameters:
//   rsvpMsgVer:  The variable containing the value of version and reserved1
//-------------------------------------------------------------------------
static UInt16 RsvpMessageHeaderGetRsrvd1(UInt16 rsvpMsgVer)
{
    UInt16 resv1= rsvpMsgVer;

    //clears the first 4 bits
    resv1 = resv1 & maskShort(5, 16);

    return resv1;
}


// Message header followed by one or more service specific data, containing
// data for specific Qos control service. Each of this section begins with
// per service data header, which is structured as below.
typedef struct per_service_header
{
    unsigned char  serviceHeader;     // service header, service number 1 for
                                      // default, 5 for controlled and
                                      // 2 for guaranteed

    unsigned char  rsvpBreakBit;
                //breakBit:1,        // service unsupported or break in path
                //reserved2:7;       // other 7 bits reserved.

    unsigned short dataLength;        // length of data
} RsvpPerServiceHeader;

//-------------------------------------------------------------------------
// FUNCTION         RsvpPerServiceHeaderSetBreakBit()
// PURPOSE          Set the value of break bit for RsvpPerServiceHeader
//
// Return:           void
// Parameters:
// rsvpBreakBit:    The variable containing the value of break bit and
//                  reserved
// bBit:            Input value for set operation
//-------------------------------------------------------------------------
static void RsvpPerServiceHeaderSetBreakBit(unsigned char *rsvpBreakBit,
                                           BOOL bBit)
{
    unsigned char x = (unsigned char)bBit;

    //masks breakBit within boundry range
    x = x & maskChar(8, 8);

    //clears the first bit
    *rsvpBreakBit = *rsvpBreakBit & maskChar(2, 8);

    //setting the value of break bit in rsvpBreakBit
    *rsvpBreakBit = *rsvpBreakBit |LshiftChar(x, 1);
}


//-------------------------------------------------------------------------
// FUNCTION         RsvpPerServiceHeaderSetReserved2()
// PURPOSE          Set the value of reserved2 for RsvpPerServiceHeader
//
// Return:           void
// Parameters:
// rsvpBreakBit:    The variable containing the value of break bit and
//                  reserved
// reserved2:       Input value for set operation
//-------------------------------------------------------------------------
static void RsvpPerServiceHeaderSetReserved2
(unsigned char *rsvpBreakBit, unsigned char reserved2)
{
    //masks reserved within boundry range
    reserved2 = reserved2 & maskChar(2, 8);

    //clears the last seven bits
    *rsvpBreakBit = *rsvpBreakBit & maskChar(1, 1);

    //setting the value of reserved2 in rsvpBreakBit
    *rsvpBreakBit = *rsvpBreakBit | reserved2;
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpPerServiceHeaderGetBreakBit()
// PURPOSE        Returns the value of bBit for RsvpPerServiceHeader
//
// Return:           BOOL
// Parameters:
// rsvpBreakBit:    The variable containing the value of break bit and
//                  reserved
//-------------------------------------------------------------------------
static BOOL RsvpPerServiceHeaderGetBreakBit(unsigned char rsvpBreakBit)
{
    unsigned char bBit= rsvpBreakBit;

    //clears all the bits except the first bit
    bBit = bBit & maskChar(1, 1);

    //Right shift so that last bit represents bBit
    bBit = RshiftChar(bBit, 1);

    return (BOOL)bBit;
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpPerServiceHeaderGetReserved2()
// PURPOSE        Returns the value of reserved2 for RsvpPerServiceHeader
//
// Return:           unsigned char
// Parameters:
// rsvpBreakBit:    The variable containing the value of break bit and
//                  reserved
//-------------------------------------------------------------------------
static unsigned char RsvpPerServiceHeaderGetReserved2
(unsigned char rsvpBreakBit)
{
    unsigned char resv2= rsvpBreakBit;

    //clears the first bit
    resv2 = resv2 & maskChar(2, 8);

    return resv2;
}


// The per-service header is followed by one or more service parameter
// blocks, each identified by a Parameter Header. This header contains
// the parameter identifier (parameter number), the length of the data
// carrying the parameter's value, and a flag field.
typedef struct parameter_header
{
    unsigned char  parameterId;       // parameter no. in service
                                      // specification

    unsigned char  parameterIdFlag;   // per parameter flag
    unsigned short parameterIdLength; // length of per-parameter data
} RsvpParameterHeader;


// IEEE specific floating point number is structured as below
typedef struct floating_point
{
    UInt32 floatValue;
                //signBit:1,           // sign bit, 0-positive, 1-negative
                 //exponent:8,          // exponent part, in base 2
                 //fraction:23;         // mantissa
} RsvpFloatingPoint;

//-------------------------------------------------------------------------
// FUNCTION       RsvpFloatingPointSetSignBit()
// PURPOSE        Set the value of signBit for RsvpFloatingPoint
//
// Return:         void
// Parameters:
// floatValue:    The variable containing the value of signBit,exponent and
//                fraction
// signBit:       Input value for set operation
//-------------------------------------------------------------------------
static void RsvpFloatingPointSetSignBit(UInt32* floatValue, BOOL signBit)
{
    UInt32 x = signBit;

    //masks signBit within boundry range
    x = x & maskInt(32, 32);

    //clears first bit
    *floatValue = *floatValue & (maskInt(2, 32));

    //setting the value of x in floatValue
    *floatValue = *floatValue | LshiftInt(x, 1);
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpFloatingPointSetExponent()
// PURPOSE        Set the value of exponent for RsvpFloatingPoint
//
// Return:         void
// Parameters:
// floatValue:    The variable containing the value of signBit,exponent and
//                fraction
// exponent:      Input value for set operation
//-------------------------------------------------------------------------
static void RsvpFloatingPointSetExponent(UInt16 exponent, UInt32 *floatValue)
{
    UInt32 exponent_int = (UInt32)exponent;

    //masks exponent within boundry range
    exponent_int = exponent_int & maskInt(25, 32);

    //clears bits 2-9
    *floatValue = *floatValue & (~(maskInt(2, 9)));

    //setting the value of exponent in floatValue
    *floatValue = *floatValue | LshiftInt(exponent_int, 9);
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpFloatingPointSetFraction()
//
// PURPOSE        Set the value of fraction for RsvpFloatingPoint
//
// Return:         void
// Parameters:
// floatValue:    The variable containing the value of signBit,exponent and
//                fraction
// fraction:      Input value for set operation
//-------------------------------------------------------------------------
static void RsvpFloatingPointSetFraction(UInt32* floatValue, UInt32 fraction)
{
    //masks fraction within boundry range
    fraction = fraction & maskInt(10, 32);

    //clears all bits except 1-9
    *floatValue = *floatValue & (maskInt(1, 9));

    //setting the value of fraction in floatValue
    *floatValue = *floatValue | fraction;
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpFloatingPointGetSignBit()
// PURPOSE        Returns the value of signBit for RsvpFloatingPoint
//
// Return:         BOOL
// Parameters:
// floatValue:    The variable containing the value of signBit,exponent and
//                fraction
//-------------------------------------------------------------------------
static BOOL RsvpFloatingPointGetSignBit(UInt32 floatValue)
{
    UInt32 signBit = floatValue;

    //clears all the bits except first bit
    signBit = signBit & maskInt(1, 1);

    //right shift 31 places so that last bit represent signBit
    signBit = RshiftInt(signBit, 1);

    return signBit;
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpFloatingPointGetExponent()
// PURPOSE        Returns the value of exponent for RsvpFloatingPoint
//
// Return:         UInt16
// Parameters:
// floatValue:    The variable containing the value of signBit,exponent and
//                fraction
//-------------------------------------------------------------------------
static UInt16 RsvpFloatingPointGetExponent(UInt32 floatValue)
{
    UInt32 exponent = floatValue;

    //clears all bits except 2-9
    exponent = exponent & maskInt(2, 9);

    //right shift 23 places so that last 9 bits represent exponent
    exponent = RshiftInt(exponent, 9);

    return (UInt16)exponent;
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpFloatingPointGetFraction()
// PURPOSE        Returns the value of fraction for RsvpFloatingPoint
//
// Return:         UInt32
// Parameters:
// floatValue:    The variable containing the value of signBit,exponent and
//                fraction
//-------------------------------------------------------------------------
static UInt32 RsvpFloatingPointGetFraction(UInt32 floatValue)
{
    UInt32 fraction = floatValue;

    //clears first 9 bits
    fraction = fraction & maskInt(10, 32);

    return fraction;
}


// TSpec is the information generated at the sender describing the data
// traffic. The structure is given below.
typedef struct tspec_
{
    RsvpFloatingPoint  tokenBucketRate;
    RsvpFloatingPoint  tokenBucketSize;
    RsvpFloatingPoint  peakDataRate;
    unsigned int       minPolicedUnit;
    unsigned int       maxPacketSize;
} RsvpTSpec;


// The information is carried from the receiver towards the sender and
// has the reservation information in the path.
typedef struct rspec_
{
    RsvpFloatingPoint bandWidth;
    unsigned int      slackTerm;
} TransportRsvpRSpec;


// The different structures of the RSVP objects are described here
//
// The following structure has been used for the object RSVP_FLOWSPEC
// object with controlled load service.
typedef struct flow_spec
{
    RsvpObjectHeader     objectHeader;    // object header
    RsvpMessageHeader    messageHeader;
    RsvpPerServiceHeader perServiceHeader;
    RsvpParameterHeader  tSpecHeader;
    RsvpTSpec            tSpec;
} RsvpObjectFlowSpec;


// The structure of the RSVP_SENDER_TSPEC object is same as that of
// RSVP_FLOWSPEC with controlled load service.
typedef RsvpObjectFlowSpec RsvpObjectSenderTSpec;


// The guaranteed RSVP_FLOWSPEC object is defined below consisting of
// TSpec and RSpec related information
typedef struct guaranteed_flowspec
{
    RsvpObjectFlowSpec  generalFlowSpec;
    RsvpParameterHeader rSpecHeader;
    RsvpTSpec           rSpec;
} RsvpObjectFlowSpecGuaranteed;


// Following structure describes the general format of RSVP_ADSPEC object.
// This structure is compulsory in the RSVP_ADSPEC object.
typedef struct adspec_general
{
    RsvpPerServiceHeader perServiceHeader;
    RsvpParameterHeader  param4Header;
    unsigned int         isHopCount;
    RsvpParameterHeader  param6Header;
    float                pathBandwidthEstimate;
    RsvpParameterHeader  param8Header;
    unsigned int         minPathLatency;
    RsvpParameterHeader  param10Header;
    unsigned int         composedMtu;
} RsvpAdSpecGeneral;


// Following structure describes the guaranteed service of RSVP_ADSPEC
// object. This is optional in the RSVP_ADSPEC object. It is added
// dynamically with the message if required.
typedef struct adspec_guaranteed
{
    RsvpPerServiceHeader perServiceHeader;

    RsvpParameterHeader  param133Header;
    unsigned int         composedCtot;

    RsvpParameterHeader  param134Header;
    unsigned int         composedDtot;

    RsvpParameterHeader  param135Header;
    unsigned int         composedCsum;

    RsvpParameterHeader  param136Header;
    unsigned int         composedDsum;
    // service specific general parameter header/values
} RsvpAdSpecGuaranteed;


// Following structure describes the controlled load service of RSVP_ADSPEC
// object.This is optional in the RSVP_ADSPEC object. It is added dynamically
// with the message if required.
typedef struct adspec_controlled
{
    RsvpPerServiceHeader perServiceHeader;
    //service specific general parameter header/values
} RsvpAdSpecControlled;


// The structure of the RSVP_ADSPEC object is given here.
typedef struct adspec_
{
    RsvpObjectHeader   objectHeader;   // object header
    RsvpMessageHeader  messageHeader;
    RsvpAdSpecGeneral  adSpecGeneral;

    // The guaranteed and controlled load service structure will be added
    // dynamically if required.
} RsvpObjectAdSpec;


// The structure of the RSVP_SENDER_TEMPLATE object is defined as below.
typedef struct sender_template
{
    RsvpObjectHeader objectHeader;    // object header

    NodeAddress      senderAddress;   // sender IPv4 address
    unsigned short   mustBeZero;      // reserved 2 bytes, must be 0
    unsigned short   lspId;           // LSP id
} RsvpTeObjectSenderTemplate;


// RSVP_FILTER_SPEC object has the same structure as in
// the RSVP_SENDER_TEMPLATE.
typedef RsvpTeObjectSenderTemplate RsvpTeObjectFilterSpec;


typedef struct session_
{
    RsvpObjectHeader objectHeader;     // object header

    NodeAddress      receiverAddress;  // egress node IPv4 address
    unsigned short   mustBeZero;       // reserved 2 bytes,must be 0
    unsigned short   tunnelId;         // remain constant in a tunnel
    unsigned int     extendedTunnelId; // normally set to 0
} RsvpTeObjectSession;


typedef struct style_
{
    RsvpObjectHeader objectHeader;     // object header
    unsigned char    flags;            // not used
    unsigned char    reserved1;        // 16 bit reserved out of 19
    unsigned short   rsvpStyleBits;//reserved2:11,     // rest of the 3 bit
                     //styleBits:5;

    // upper 2 bits for sharing control, 00-reserved, 01-distict, 10-shared
    // lower 3 bits for selection control, 11-reserved, 000b-reserved,
    // 001b-wildcard, 010b-explicit
} RsvpObjectStyle;

//-------------------------------------------------------------------------
// FUNCTION:        RsvpObjectStyleSetReserved2()
// PURPOSE:         Set the value of reserved2 for RsvpObjectStyle
//
// Return:           void
// Parameters:
// rsvpStyleBits:   The variable containing the value of reserved and
//                  style bits
// reserved2:       Input value for set operation
//-------------------------------------------------------------------------
static void RsvpObjectStyleSetReserved2(UInt16 *rsvpStyleBits,
                                        UInt16 reserved2)
{
    //masks reserved2 within boundry range
    reserved2 = reserved2 & maskShort(6, 16);

    //clears first 11 bits
    *rsvpStyleBits = *rsvpStyleBits & maskShort(12, 16);

    //setting the value of reserved2 in rsvpStyleBits
    *rsvpStyleBits = *rsvpStyleBits | LshiftShort(reserved2, 11);
}


//-------------------------------------------------------------------------
// FUNCTION:        RsvpObjectStyleSetStyleBits()
// PURPOSE:         Set the value of style bits for RsvpObjectStyle
//
// Return:           void
// Parameters:
// rsvpStyleBits:   The variable containing the value of reserved and
//                  style bits
// styleBit:        Input value for set operation
//-------------------------------------------------------------------------
static void RsvpObjectStyleSetStyleBits(UInt16 *rsvpStyleBits,
                                        UInt32 styleBit)
{
    UInt16 sty_bit_short = (UInt16)styleBit;

    //masks styleBits within boundry range
    sty_bit_short = sty_bit_short & maskShort(12, 16);

    //clears last 5 bits
    *rsvpStyleBits = *rsvpStyleBits & maskShort(1, 11);

    //setting the value of style bit in rsvpStyleBits
    *rsvpStyleBits = *rsvpStyleBits | sty_bit_short;
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpObjectStyleGetReserved2()
// PURPOSE        Returns the value of reserved2 for RsvpObjectStyle
//
// Return:         UInt16
// Parameters:
// rsvpStyleBits:   The variable containing the value of reserved and
//                  style bits
//-------------------------------------------------------------------------
static UInt16 RsvpObjectStyleGetReserved2(UInt16 rsvpStyleBits)
{
    UInt16 resv2= rsvpStyleBits;

    //clears the last 5 bits
    resv2 = resv2 & maskShort(1, 11);

    //Right shifts so that last 11 bits represent resvered
    resv2 = RshiftShort(resv2, 11);

    return resv2;
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpObjectStyleGetStyleBits()
// PURPOSE        Returns the value of style bits for RsvpObjectStyle
//
// Return:         UInt32
// Parameters:
// rsvpStyleBits:   The variable containing the value of reserved and
//                  style bits
//-------------------------------------------------------------------------
static int RsvpObjectStyleGetStyleBits(UInt16 rsvpStyleBits)
{
    UInt16 styleBit= rsvpStyleBits;

    //clears the first 11 bits
    styleBit = styleBit & maskShort(12, 16);

    return (int)styleBit;
}


typedef struct scope_
{
    RsvpObjectHeader objectHeader;     // object header
    NodeAddress      srcAddress;
} RsvpObjectScope;


typedef struct rsvp_hop
{
    RsvpObjectHeader objectHeader;     // object header
    NodeAddress      nextPrevHop;      // next or prev Hop IP address
    unsigned int     lih;              // logical interface handle
} RsvpObjectRsvpHop;


typedef struct time_values
{
    RsvpObjectHeader objectHeader;     // object header
    unsigned int     refreshPeriod;    // refresh period in millisecond
} RsvpObjectTimeValues;


typedef struct error_spec
{
    RsvpObjectHeader objectHeader;     // object header
    NodeAddress      errorNodeAddress; // node address where error
    unsigned char    errorFlag;        // 01-InPlace, 02-NotGuilty

    unsigned char    errorCode;        // RSVP_ROUTING_PROBLEM/RSVP_NOTIFY
                                       // error

    unsigned short   errorValue;       // error details description
} RsvpObjectErrorSpec;


typedef struct resv_confirm
{
    RsvpObjectHeader objectHeader;     // object header
    NodeAddress      receiverAddress;  // node address where error
} RsvpObjectResvConfirm;


typedef struct label_request
{
    RsvpObjectHeader objectHeader;     // object header
    unsigned short   reserved;         // must be set 0
    unsigned short   L3Pid;            // ident. of layer 3 protocol
} RsvpTeObjectLabelRequest;


typedef struct
{
    RsvpObjectHeader objectHeader;     // object header
    unsigned int     label;            // label value
} RsvpTeObjectLabel;


typedef struct sub_object
{
    union
    {
        unsigned char typeExpRoute;//lBit:1,
                      //explicitType:7;  // this for RSVP_EXPLICIT_ROUTE
        unsigned char recordType;      // this for RSVP_RECORD_ROUTE
    } TypeField;

    unsigned char     length;          // length of the subobject
    unsigned short    ipAddress1;      // IP address in two part
    unsigned short    ipAddress2;
    unsigned char     prefixLength;    // length in bit of Ipv4 prefix

    union
    {
        unsigned char reserved;        // zero on transmission. Ignored
                                       // on receipt

        unsigned char flags;           // for RSVP_RECORD_ROUTE object
    } Padding;
} RsvpTeSubObject;

//-------------------------------------------------------------------------
// FUNCTION         RsvpTeSubObjectSetLBit()
// PURPOSE          Set the value of lbit for TypeField in RsvpTeSubObject
//
// Return:           void
// Parameters:
// typeExpRoute:  The variable containing the value of lbit and explicitType
// lBit:           Input value for set operation
//-------------------------------------------------------------------------
static void RsvpTeSubObjectSetLBit(unsigned char *typeExpRoute, BOOL lBit)
{
    unsigned char x = (unsigned char)lBit;

    //masks lBit within boundry range
    x = x & maskChar(8, 8);

    //clears the first bit
    *typeExpRoute = *typeExpRoute & maskChar(2, 8);

    //setting the value of lbit in typeExpRoute
    *typeExpRoute = *typeExpRoute |LshiftChar(x, 1);
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpTeSubObjectSetExpType()
// PURPOSE        Set the value of explicit type for TypeField in
//                RsvpTeSubObject
//
// Return:           void
// Parameters:
// typeExpRoute:  The variable containing the value of lbit and explicitType
// explicitType:  Input value for set operation
//-------------------------------------------------------------------------
static void RsvpTeSubObjectSetExpType(unsigned char *typeExpRoute,
                                      UInt32 explicitType)
{
    unsigned char x = (unsigned char)explicitType;

    //masks explicitType within boundry range
    x = x & maskChar(2, 8);

    //clears the last seven bits
    *typeExpRoute = *typeExpRoute & maskChar(1, 1);

    //setting the value of explicitType in typeExpRoute
    *typeExpRoute = *typeExpRoute | x;
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpTeSubObjectGetLBit()
// PURPOSE        Returns the value of lbit for TypeField in
//                RsvpTeSubObject
//
// Return:         BOOL
// Parameters:
// typeExpRoute:  The variable containing the value of lbit and explicitType
//-------------------------------------------------------------------------
static BOOL RsvpTeSubObjectGetLBit(unsigned char typeExpRoute)
{
    unsigned char lBit= typeExpRoute;

    //clears all the bits except the first bit
    lBit = lBit & maskChar(1, 1);

    //Right shifts so that last bit represents lbit
    lBit = RshiftChar(lBit, 1);

    return (BOOL)lBit;
}


//-------------------------------------------------------------------------
// FUNCTION       RsvpTeSubObjectGetExpType()
// PURPOSE        Returns the value of explicitType for TypeField in
//                RsvpTeSubObject
//
// Return:         UInt32
// Parameters:
// typeExpRoute:  The variable containing the value of lbit and explicitType
//-------------------------------------------------------------------------
static UInt32 RsvpTeSubObjectGetExpType(unsigned char typeExpRoute)
{
    unsigned char exp_type= typeExpRoute;

    //clears the first bit
    exp_type = exp_type & maskChar(2, 8);

    return (UInt32)exp_type;
}


typedef struct explicit_route
{
    RsvpObjectHeader objectHeader;     // object header
    RsvpTeSubObject  subObject;

    // subobject may appear more than one and is allocated dynamically
} RsvpTeObjectExplicitRoute;


typedef RsvpTeObjectExplicitRoute RsvpTeObjectRecordRoute;


// This structure is used when label recording flag is set in the
// SESSION_ATTRBUTES object. Then this structue has to be added with
// RSVP_RECORD_ROUTE object.
typedef struct record_label
{
    unsigned char recordType;          // RSVP_RECORD_ROUTE type when label
                                       // recording is on

    unsigned char length;              // length of the sub-object
    unsigned char flags;               // global label flag
    unsigned char cType;               // same for the label object
    unsigned int  labelContent;        // content of label object
} RsvpTeRecordLabel;


// This is the RSVP_RECORD_ROUTE object structure when label recording
// flag is set in the object SESSION_ATTRIBUTES
typedef struct record_route_label
{
    RsvpObjectHeader  objectHeader;    // object header
    RsvpTeRecordLabel recordLabel;
    RsvpTeSubObject   subObject;

    // record label and sub-object may appear more than one, that has
    // been allocated dynamically
} RsvpTeObjectRecordRouteLabel;


typedef struct session_attribute
{
    RsvpObjectHeader objectHeader;     // object header
    unsigned char    setupPriority;    // priority for taking resource
    unsigned char    holdingPriority;  // priority for holding resource
    unsigned char    flags;            // local protection,label recording
    unsigned char    nameLength;       // length of string before padding

    // display string is variable length and allocated dynamically
} RsvpTeObjectSessionAttribute;


typedef struct hello
{
    RsvpObjectHeader objectHeader;     // object header
    unsigned int     srcInstance;      // source instance
    unsigned int     dstInstance;      // destination instance
} RsvpTeObjectHello;


// One filter_spec as defined below
typedef struct filt_ss
{
    RsvpTeObjectFilterSpec* filterSpec;
    RsvpTeObjectLabel*      label;

    // RsvpTeObjectRecordRoute/RsvpTeObjectRecordRouteLabel
    // depending on the label flag in RSVP_SESSION_ATTRIBUTE
    void* recordRoute;
} RsvpFiltSS;


// FF-style descriptor consists of pair of flowSpec and filtSS
typedef struct ff_descriptor
{
    RsvpObjectFlowSpec* flowSpec;
    RsvpFiltSS*         filtSS;
} RsvpFFDescriptor;


// SE style descriptor consists of one flowSpec and list of filtSS
typedef struct se_descriptor
{
    RsvpObjectFlowSpec* flowSpec;
    LinkedList*         filtSS;
} RsvpSEDescriptor;


// The following structure holds the pointer of all possible objects in a
// message. At the time of parsing of the message this structure is created
// and used by the message later.
typedef struct all_objects
{
    unsigned char*                packetInitialPtr;
    RsvpCommonHeader*             commonHeader;
    RsvpTeObjectSession*          session;
    RsvpTeObjectSenderTemplate*   senderTemplate;
    RsvpObjectSenderTSpec*        senderTSpec;
    RsvpObjectAdSpec*             adSpec;
    RsvpObjectRsvpHop*            rsvpHop;
    RsvpObjectTimeValues*         timeValues;
    RsvpObjectResvConfirm*        resvConf;
    RsvpObjectScope*              scope;
    RsvpObjectStyle*              style;
    RsvpObjectErrorSpec*          errorSpec;
    RsvpTeObjectLabelRequest*     labelRequest;
    RsvpTeObjectExplicitRoute*    explicitRoute;

    // store RSVP_RECORD_ROUTE object appeared in RSVP_PATH_MESSAGE only.
    // RsvpTeObjectRecordRoute/RsvpTeObjectRecordRouteLabel
    // depending on the label flag in RSVP_SESSION_ATTRIBUTE
    void*                         recordRoute;

    // contains descriptor list, different reservation style is considered
    // TransportRsvpFlowDescriptorList*       flowDescriptor;
    union
    {
        // list of FF style descriptor RsvpFFDescriptor
        LinkedList* ffDescriptor;

        // for SE style descriptor RsvpSEDescriptor
        RsvpSEDescriptor* seDescriptor;

        // for WF style
        RsvpObjectFlowSpec* flowSpec;
    } FlowDescriptorList;

    RsvpTeObjectSessionAttribute* sessionAttr;
    RsvpTeObjectHello* hello;
} RsvpAllObjects;

// The basic message structure of RSVP is defined here. For the extension of
// RSVP in Traffic engineering the new objects has to be added dynamically
// with the proper message. By this way the basic message structures can be
// used by RSVP only.


// The sender descriptor structure has been used in Path, PathErr and
// PathTear messages.
typedef struct sender_descriptor
{
    RsvpTeObjectSenderTemplate senderTemplate;
    RsvpObjectSenderTSpec      senderTSpec;
    // RSVP_ADSPEC and RSVP_RECORD_ROUTE object can be added
    // dynamically if required
} RsvpSenderDescriptor;


// The basic structure of Path message is given here. Any additional objects
// required for an extension can be added dynamically.
typedef struct path_message
{
    RsvpCommonHeader     commonHeader;     // common header
    RsvpTeObjectSession  session;          // session object
    RsvpObjectRsvpHop    rsvpHop;          // rsvp Hop
    RsvpObjectTimeValues timeValues;       // timer value
    RsvpSenderDescriptor senderDescriptor; // sender template

    // In case of RSVP-TE RsvpTeObjectLabelRequest object must be
    // added in the Path message beside above objects RSVP_EXPLICIT_ROUTE,
    // RSVP_SESSION_ATTRIBUTE, object can be optionally added with the Path
    // message dynamically if RSVP-TE. The optional object will be added
    // at the end because ordering is no matter in the messages.
} RsvpPathMessage;


// The structure of the basic Resv message is given here.
typedef struct resv_message
{
    RsvpCommonHeader     commonHeader;  // common header
    RsvpTeObjectSession  session;       // session object
    RsvpObjectRsvpHop    rsvpHop;       // rsvp Hop
    RsvpObjectTimeValues timeValues;    // timer value
    RsvpObjectStyle      style;

    // The optional objects RESV_CONFIRM and RSVP_SCOPE can be added
    // dynamically if required. Depending on the style, proper flow
    // descriptor structure can be added dynamically
} RsvpResvMessage;


// For FF style one or more copy of the following structure can be added to
// the Resv message
typedef struct ff_flow_descriptor
{
    RsvpObjectFlowSpec     flowSpec;
    RsvpTeObjectFilterSpec filterSpec;

    // In case of RSVP-TE RsvpTeObjectLabel must be added with the
    // resv message RSVP_RECORD_ROUTE object can be added dynamically
    // if required
} RsvpFFFlowDescriptor;


// For SE style one or more copy of following structure can present in the
// Resv. Message.
typedef struct se_filter_spec
{
    RsvpTeObjectFilterSpec filterSpec;

    // In case of RSVP-TE RsvpTeObjectLabel must be added with the
    // resv message RSVP_RECORD_ROUTE object can be added dynamically
    // if required
} RsvpSEFilterSpec;


// For SE style one copy of this structure must be present in Resv. Message,
// that may be followed by one or more RsvpSEFilterSpec
typedef struct se_flow_descriptor
{
    RsvpObjectFlowSpec flowSpec;
    RsvpSEFilterSpec   filterSpec;

    // RsvpSEFilterSpec object may come more than once,
    // that is added dynamically
} RsvpSEFlowDescriptor;


typedef struct path_err_message
{
    RsvpCommonHeader     commonHeader;      // common header
    RsvpTeObjectSession  session;           // session object
    RsvpObjectErrorSpec  errorSpec;         // error specification
    RsvpSenderDescriptor senderDescriptor;  // sender descriptor
} RsvpPathErrMessage;


typedef struct resv_err_message
{
    RsvpCommonHeader    commonHeader;       // common header
    RsvpTeObjectSession session;            // session object
    RsvpObjectRsvpHop   rsvpHop;            // Rsvp hop
    RsvpObjectErrorSpec errorSpec;          // error specification
    RsvpObjectStyle     style;              // style

    // RSVP_SCOPE object can be added dynamically if required. Depending
    // on the style, proper flow descriptor structure can be
    // added dynamically
} RsvpResvErrMessage;


typedef struct path_tear_message
{
    RsvpCommonHeader     commonHeader;      // common header
    RsvpTeObjectSession  session;           // session object
    RsvpObjectRsvpHop    rsvpHop;           // rsvp hop
    RsvpSenderDescriptor senderDescriptor;  // sender descriptor
} RsvpPathTearMessage;


typedef struct resv_tear_message
{
    RsvpCommonHeader    commonHeader;       // common header
    RsvpTeObjectSession session;            // session object
    RsvpObjectRsvpHop   rsvpHop;            // Rsvp hop
    RsvpObjectStyle     style;              // style

    // RSVP_SCOPE object can be added dynamically if required
    // Depending on the style, proper flow descriptor structure can
    // be added dynamically
} RsvpResvTearMessage;


typedef struct resv_conf_message
{
    RsvpCommonHeader      commonHeader;     // common header
    RsvpTeObjectSession   session;          // session object
    RsvpObjectErrorSpec   errorSpec;        // error specification
    RsvpObjectResvConfirm resvConfirm;      // error specification
    RsvpObjectStyle       style;            // style

    // Depending on the style, proper flow descriptor structure can be
    // added dynamically
} RsvpResvConfMessage;


typedef struct hello_message
{
    RsvpCommonHeader  commonHeader;        // common header
    RsvpTeObjectHello hello;               // hello object
} RsvpTeHelloMessage;


// Different State Block structures
typedef struct path_state_block
{
    RsvpTeObjectSession         session;
    RsvpTeObjectSenderTemplate  senderTemplate;
    RsvpObjectSenderTSpec       senderTSpec;
    RsvpObjectRsvpHop           rsvpHop;
    unsigned char               remainingTtl;
    RsvpObjectAdSpec            adSpec;
    BOOL                        nonRsvpFlag;
    BOOL                        localOnlyFlag;
    IntList*                    outInterfaceList;
    int                         incomingInterface;
    RsvpTeObjectLabelRequest    labelRequest;
    RsvpTeObjectExplicitRoute*  receivingExplicitRoute;
    RsvpTeObjectExplicitRoute*  modifiedExplicitRoute;

    // RsvpTeObjectRecordRoute/RsvpTeObjectRecordRouteLabel
    // depending on the label flag in RSVP_SESSION_ATTRIBUTE
    void*                        recordRoute;
    RsvpTeObjectSessionAttribute *sessionAttr;
    unsigned                     lastTimeValues;
} RsvpPathStateBlock;


typedef struct resv_state_block
{
    RsvpTeObjectSession     session;
    NodeAddress             nextHopAddress;

    // list of RsvpFiltSS that consists of RSVP_FILTER_SPEC,
    // RSVP_LABEL_OBJECT and RSVP_RECORD_ROUTE object
    LinkedList*             filtSSList;
    int                     outgoingLogicalInterface;
    RsvpObjectStyle         style;
    RsvpObjectFlowSpec      flowSpec;
    RsvpObjectScope         scope;
    RsvpObjectResvConfirm   resvConf;
    unsigned                lastTimeValues;
} RsvpResvStateBlock;


typedef struct blockade_state_block
{
    RsvpTeObjectSession        session;
    RsvpTeObjectSenderTemplate senderTemplate;
    NodeAddress                prevHopAddress;
    RsvpObjectFlowSpec         flowSpecQB;
    unsigned int               blockadetimerTb;
    BOOL                       pathRefreshNeeded;
    BOOL                       resvRefreshNeeded;
    BOOL                       tearNeeded;
    BOOL                       needScope;
    BOOL                       bMerge;
    BOOL                       newOrMod;
    LinkedList*                refreshPhopList;
} RsvpBlockadeStateBlock;

// Upcall procedure declaration

typedef struct timer_store
{
    Message* messagePtr;
    RsvpPathStateBlock* psb;
} RsvpTimerStore;

// This is the different info type for the upcall procedures.
typedef enum
{
    PATH_EVENT,
    RESV_EVENT,
    PATH_ERROR,
    RESV_ERROR,
    RESV_CONF
} RsvpUpcallInfoType;


// The main structure for information parameters for the upcall procedures.
// Different events have different parameters list. They are structured by
// using the union.
typedef struct info_parameter
{
    union
    {
        // parameters for PATH_EVENT
        struct
        {
            RsvpObjectSenderTSpec*      senderTSpec;
            RsvpTeObjectSenderTemplate* senderTemplate;
            RsvpObjectAdSpec*           adSpec;
        } PathEvent;

        // parameters for RESV_EVENT
        struct
        {
            RsvpObjectStyle*    style;
            RsvpObjectFlowSpec* flowSpec;
            LinkedList*         filterSpecList;
        } ResvEvent;

        // parameters for PATH_ERROR
        struct
        {
            unsigned char               errorCode;
            unsigned short              errorValue;
            NodeAddress                 errorNode;
            RsvpTeObjectSenderTemplate* senderTemplate;
        } PathError;

        // parameters for RESV_ERROR
        struct
        {
            unsigned char       errorCode;
            unsigned short      errorValue;
            NodeAddress         errorNode;
            unsigned char       errorFlag;
            RsvpObjectFlowSpec* flowSpec;
            LinkedList*         filterSpecList;
        } ResvError;

        // parameters for RESV_CONFIRM
        struct
        {
            RsvpObjectStyle*    style;
            RsvpObjectFlowSpec* flowSpec;
            LinkedList*         filterSpecList;
        } ResvConfirm;
    } Info;
} RsvpUpcallInfoParameter;


// The function pointer declaration for upcall procedure is given below.
typedef void (*RsvpUpcallFunctionType)(
    int sessionId,
    RsvpUpcallInfoType infoType,
    RsvpUpcallInfoParameter infoParam);


typedef struct rsvp_statistics
{
    unsigned int numPathMsgReceived;
    unsigned int numPathMsgSent;

    unsigned int numResvMsgReceived;
    unsigned int numResvMsgSent;

    unsigned int numPathErrMsgReceived;
    unsigned int numPathErrMsgSent;

    unsigned int numResvErrMsgReceived;
    unsigned int numResvErrMsgSent;

    unsigned int numPathTearMsgReceived;
    unsigned int numPathTearMsgSent;

    unsigned int numResvTearMsgReceived;
    unsigned int numResvTearMsgSent;

    unsigned int numResvConfMsgReceived;
    unsigned int numResvConfMsgSent;

    unsigned int numHelloRequestMsgReceived;
    unsigned int numHelloRequestMsgSent;

    unsigned int numHelloAckMsgReceived;
    unsigned int numHelloAckMsgSent;
    unsigned int numLoopsDetected;
} RsvpStatistics;


// This is main data structure to keep the label mapping in each node to
// create the LSP between ingress to egress node.
typedef struct
{
    int             inLabel;
    unsigned short  lspId;
    NodeAddress     sourceAddr;
    NodeAddress     destAddr;
} RsvpLsp;


// This is the session list structure, which is used when a session is
// registered by an application running on a host. For a source destination
// pair a session Id is created and the corresponding upcall function pointer
// received from the application is stored here.
typedef struct session_list
{
    NodeAddress            destinationAddr;
    RsvpUpcallFunctionType upcallFunctionPtr; // store upcall func.
    int                    sessionId;         // used for tunnelId also
    char                   sessionName[MAX_STRING_LENGTH];

    // When session released set it on, if it is ON no refresh
    // message will be generated
    BOOL                            isSessionReleased;
} RsvpSessionList;


// This is the structure kept at each node to store all adjacent neighbor
// instances. This structure is used for Hello message.
typedef struct hello_extension
{
    NodeAddress  neighborAddr;
    int          interfaceIndex;
    unsigned int srcInstance;
    unsigned int dstInstance;
    int          nonMatchingCount;
    BOOL         isFailureDetected;
} RsvpTeHelloExtension;


// This is RSVP layer structure. At the time of initialization of Rsvp, this
// structure will be initialized. LSP contains the label switch path mapping
// from that node to the next hop.
typedef struct struct_transport_rsvp
{
    BOOL rsvpStatsEnabled;
    RsvpStatistics* rsvpStat;
    NodeAddress* abstractNodeList;      // Abs. node for explicit
    int abstractNodeCount;              // count of Abs node
    RsvpRecordRouteType recordRouteType;// RSVP_RECORD_ROUTE required info
    RsvpStyleType styleType;            // Whether style is FF/SE/WF
    LinkedList* sessionList;            // List RsvpSessionList
    LinkedList* psb;                    // List of path state block
    LinkedList* rsb;                    // List of resv state block
    LinkedList* bsb;                    // List of blk state block
    int nextSessionId;                  // available next session
    int nextAvailableLspId;             // next available LSP ID
    int nextAvailableLabel;             // next available label
    unsigned int nextAvailableInstance; // next available inst.
    LinkedList* helloList;              // hello list
    LinkedList* lsp;                    // List contains LSP
    LinkedList* timerStoreList;         // list to store the timer info
    RandomSeed seed;                    // For random refresh delay
} RsvpData;


//-------------------------------------------------------------------------
// FUNCTION    RsvpInit
//
// PURPOSE     Initialization function for RSVP.
//
// Parameters:
//     node:      node being initialized.
//     nodeInput: structure containing contents of input file
//-------------------------------------------------------------------------
void RsvpInit(Node* node, const NodeInput* nodeInput);


//-------------------------------------------------------------------------
// FUNCTION    TransportLayerRsvp.
// PURPOSE     Models the behaviour of RSVP on receiving the
//             message encapsulated in msg
//
// Parameters:
//     node:     node which received the message
//     msg:   message received by the layer
//-------------------------------------------------------------------------
void RsvpLayer(Node* node, Message* msg);


//-------------------------------------------------------------------------
// FUNCTION    TransportRsvpFinalize
// PURPOSE     Called at the end of simulation to collect the results of
//             the simulation of the RSVP protocol of Transport Layer.
//
// Parameter:
//     node:     node for which results are to be collected.
//-------------------------------------------------------------------------
void RsvpFinalize(Node* node);


RsvpTeObjectSenderTemplate RsvpCreateSenderTemplate(
    NodeAddress sourceAddr,
    int lspId);


RsvpObjectSenderTSpec RsvpCreateSenderTSpec(Node* node);


RsvpObjectAdSpec RsvpCreateAdSpec(Node* node);


int RsvpRegisterSession(
    Node* node,
    NodeAddress destinationAddr,
    RsvpUpcallFunctionType upcallFunctionPtr);


void RsvpDefineSender(
    Node* node,
    int sessionId,
    RsvpTeObjectSenderTemplate* senderTemplate,
    RsvpObjectSenderTSpec* senderTSpec,
    RsvpObjectAdSpec* adSpec,
    unsigned char ttl);

//-------------------------------------------------------------------------
// FUNCTION       IsIngressRsvp()
//
// PURPOSE        to check if the node is Ingress for the RSVP application
//
// PARAMETERS: node - the node which is checking the Network
//                       Forwarding Table.
//                msg -  msg/packet to be routed.
//
// RETURN VALUE:   TRUE if the node is an Ingress, FALSE otherwise
//-------------------------------------------------------------------------
BOOL IsIngressRsvp(Node* node, Message* msg);

#endif /* RSVP_H */

