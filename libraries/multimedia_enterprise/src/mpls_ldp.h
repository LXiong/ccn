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
// Purpose: Simulate MPLS LDP rfc 3036
//
//

#ifndef MPLS_LDP_H
#define MPLS_LDP_H


#include "buffer.h"
#include "main.h"


// define various LDP timers, or timer related constants or timer
// related macros.
#define LDP_DEFAULT_LABEL_REQUEST_TIME_OUT   (90 * SECOND)
#define LDP_DEFAULT_LINK_HELLO_HOLD_TIME     (15 * SECOND)
#define LDP_DEFAULT_KEEP_ALIVE_TIME_INTERVAL (10 * SECOND)
#define LDP_DEFAULT_TARGETED_HELLO_INTERVAL  (45 * SECOND)
#define MPLS_LDP_STARTUP_DELAY               ( 5 * SECOND)
#define DEFAULT_MPLS_SEND_KEEP_ALIVE_DELAY   (10 * MILLI_SECOND)

#define LDP_DEFAULT_LINK_HELLO_INTERVAL \
        (LDP_DEFAULT_LINK_HELLO_HOLD_TIME / 3) // We recommend that the
                                               // interval between Hello
                                               // transmissions be at most
                                               // one third of the Hello
                                               // hold time.

#define KEEP_ALIVE_DELAY(x) (x->keepAliveTime - x->sendKeepAliveDelay)


// define miscellaneous constants used by MPLS_LDP, e.g. Protocol Version,
// length of various header types, max hop counts etc..

#define IP_V4_ADDRESS_FAMILY                1
#define MAX_ALLOWABLE_HOP_COUNT             64
#define MIN_ALLOWABLE_MAX_HOP_COUNT         0
#define MPLS_LDP_MAX_LABEL_VALUE            1048575
#define LDP_RFC_3036_PROTOCOL_VERSION       1
#define LDP_PRE_NEGOTIATION_MAX_PDU_LENGTH  4096
#define LDP_MESSAGE_ID_SIZE                 (sizeof(unsigned int))
#define MIN_PATH_VECTOR_LIMIT               0
#define DEFAULT_PATH_VECTOR_LIMIT           64
#define SMALLEST_ALLOWABLE_MAX_PDU_LENGTH   256
#define MPLS_UNKNOWN_NUM_HOPS               0
#define MPLS_LDP_INVALID_PEER_ADDRESS       0
#define INVALID_TCP_CONNECTION_ID           -1
#define INVALID_INTERFACE_INDEX             -1

#define PLATFORM_WIDE_LABEL_SPACE_ID 0 // The last two octets of LDP
                                       // Identifiers for platform-wide
                                       // label spaces are always both zero

#define LDP_IDENTIFIER_SIZE 6 // If LDP_Identifier_Type struct changes,
                              // modify this parameter and look at
                              // MplsLdpAddPduHeader

#define LDP_HEADER_SIZE 10    // If LDP_Header struct changes, modify this
                              // parameter and look at MplsLdpAddPduHeader


//   define macros used by MPLS-LDP

#define UpdateLastMsgSent(helloPtr) helloPtr->lastMsgSent = node->getNodeTime();
#define UpdateLastHeard(helloPtr) helloPtr->lastHeard = node->getNodeTime();
#define INCREMENT_STAT_VAR(x) ((ldp->ldpStat).x++)
#define MPLS_INCREMENT_HOP_COUNT_IF_KNOWN(X)  \
        ((X == MPLS_UNKNOWN_NUM_HOPS) ? ( X ) : (X + 1)) // For hop count
                                                         // arithmetic,
                                                         // unknown + 1 =
                                                         // unknown.


// define constants used for initializing various
// MPLS LDP buffers and tables

#define MPLS_LDP_APP_INITIAL_NUM_LOCAL_ADDR      2
#define MPLS_LDP_APP_INITIAL_NUM_HELLO_ADJACENCY 8
#define MAX_OUT_GOING_REQUEST_SIZE               8
#define MAX_PENDING_REQUEST_SIZE                 8
#define MAX_LIB_SIZE                             16
#define MPLS_LDP_MAX_INCOMING_REQUEST_CACHE      16
#define MPLS_LDP_MAX_OUTBOUND_CACHE              16


// define various ldp message type

#define LDP_HELLO_MESSAGE_TYPE             0x0100
#define LDP_INITIALIZATION_MESSAGE_TYPE    0x0200
#define LDP_KEEP_ALIVE_MESSAGE_TYPE        0x0201
#define LDP_ADDRESS_LIST_MESSAGE_TYPE      0x0300
#define LDP_LABEL_MAPPING_MESSAGE_TYPE     0x0400
#define LDP_LABEL_REQUEST_MESSAGE_TYPE     0x0401
#define LDP_NOTIFICATION_MESSAGE_TYPE      0x0001
#define LDP_LABEL_RELEASE_MESSAGE_TYPE     0x0403
#define LDP_LABEL_WITHDRAW_MESSAGE_TYPE    0x0402

#define MAX_IP_STRING_LENGTH 16

enum
{
    INCOMING,
    OUTGOING
};


typedef enum
{
    LOOP_DETECTED,
    NO_LOOP_DETECTED
} LoopDetectionStatus;


typedef enum
{
    TCP_PACKET,
    UDP_PACKET
} Packet_Type;


typedef enum
{
    ADVISORY_NOTIFICATION = 0,
    FATAL_ERROR_NOTIFICATION = 1
} NotificationType;


typedef enum
{
    UNIDENTIFIED_ERROR = 0,
    LOOP_DETECTED_ERROR,
    NO_ROUTE_ERROR,
    UNSUPPORTED_ADDRESS_FAMILY,
    NO_LABEL_RESOURCE_ERROR
} NotificationErrorCode;


typedef enum
{
    Request_Never,
    Request_When_Needed,
    Request_On_Request
} LabelRequestStatus;


// define LDP label distribution mode
typedef enum
{
    UNSOLICITED = 0,
    ON_DEMAND = 1
} MplsLabelDistributionMode;


//LDP initialization mode
typedef enum
{
    MPLS_LDP_ACTIVE_INITIALIZATION_MODE  = 0,
    MPLS_LDP_PASSIVE_INITIALIZATION_MODE = 1
} MPLS_LDP_InitializationMode;


// define different types of MPLS LDP label
// distribution control mode
typedef enum
{
    MPLS_CONTROL_MODE_ORDERED     = 1,
    MPLS_CONTROL_MODE_INDEPENDENT = 2
} MPLS_LDP_ControlMode;


// define different types MPLS LDP label
// retention mode.
typedef enum
{
    LABEL_RETENTION_MODE_CONSERVATIVE,
    LABEL_RETENTION_MODE_LIBERAL
} MPLS_LDP_LabelRetentionMode;


typedef enum
{
    MPLS_LDP_NOT_PROPAGATING,
    MPLS_LDP_IS_PROPAGATING
} MplsPropagatingMode;


// define LDP Identifier structure
typedef struct
{
    NodeAddress LSR_Id;      // identify the LSR and must be globally
                             // unique, such as a 32-bit router Id
                             // assigned to the LSR

    unsigned short label_space_id;  // a specific label space
                                    // within the LSR.
} LDP_Identifier_Type;


// Each LDP PDU is an LDP header followed by one or more LDP messages
// define LDP PDU Header structure
typedef struct
{
    unsigned short Version;     // Version Number of this Protocol

    unsigned short PDU_Length;  // total length of this PDU in octets,
                                // excluding the Version and PDU
                                // Length fields.

    LDP_Identifier_Type LDP_Identifier; // uniquely identifies the label
                                        // space of the sending LSR for
                                        // which this PDU applies
} LDP_Header;


// define LDP Message header structure
typedef struct
{
    UInt16 ldpMsg;
                   //U:1,     // Unknown message bit.  Upon receipt of
                                     // an unknown message, if U is clear (=0),
                                     // a notification is returned to the
                                     // message originator;
                                     // if U is set (=1), the unknown message
                                     // is silently ignored.

                   //Message_Type:15;       // Identifies the type of message

    unsigned short Message_Length;   // Specifies the cumulative length in
                                     // octets of the Message ID, Mandatory
                                     // Parameters, and Optional Parameters.

    unsigned int Message_Id;         // 32-bit value used to identify this
                                     // message.  Used by the sending LSR to
                                     // facilitate identifying notification
                                     // messages that may apply to this
                                     // message.
                                     // REMARK: if you change the data type
                                     // for this, you should change the
                                     // value of LDP_MESSAGE_ID_SIZE
                                     // above as well

    // Mandatory Parameters and Optional Parameters Follow
} LDP_Message_Header;

//-------------------------------------------------------------------------
// FUNCTION     : LDP_Message_HeaderSetU()
//
// PURPOSE      : Set the value of U for LDP_Message_Header
//
// PARAMETERS   :ldpMsg - The variable containing the value of U and
//                        Message_Type
//               U      - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static  void LDP_Message_HeaderSetU(UInt16 *ldpMsg, BOOL U)
{
    UInt16 x = (UInt16) U;

    //masks U within boundry range
    x = x & maskShort(16, 16);

    //clears the first bit
    *ldpMsg = *ldpMsg & (~(maskShort(1, 1)));

    //setting the value of x in ldpMsg
    *ldpMsg = *ldpMsg | LshiftShort(x, 1);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_Message_HeaderSetMsgType()
//
// PURPOSE      : Set the value of Message_Type for LDP_Message_Header
//
// PARAMETERS   : ldpMsg - The variable containing the value of U and
//                         Message_Type
//           messageType - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_Message_HeaderSetMsgType(UInt16 *ldpMsg,
                                          UInt16 messageType)
{
    //masks messageType within boundry range
    messageType = messageType & maskShort(2, 16);

    //clears all bits except first bit
    *ldpMsg = *ldpMsg & maskShort(1, 1);

    //setting the value of message type in ldpMsg
    *ldpMsg = *ldpMsg | messageType;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_Message_HeaderGetMsgType()
//
// PURPOSE      : Returns the value of Message_Type for LDP_Message_Header
//
// PARAMETERS   : ldpMsg - The variable containing the value of U and
//                         Message_Type
//
// RETURN VALUE : UInt16
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt16 LDP_Message_HeaderGetMsgType(UInt16 ldpMsg)
{
    UInt16 messageType = ldpMsg;

    //clears the first bit
    messageType = messageType & (maskShort(2, 16));

    return messageType;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_Message_HeaderGetU()
//
// PURPOSE      : Returns the value of TTL for LDP_Message_Header
//
// PARAMETERS   : ldpMsg - The variable containing the value of U and
//                         Message_Type
//
// RETURN VALUE : BOOL
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL LDP_Message_HeaderGetU(UInt16 ldpMsg)
{
    UInt16 U= ldpMsg;

    //clears all the bits except first
    U = U & maskShort(1, 1);

    //right shift 15 bits
    U = RshiftShort(U, 1);

    return (BOOL)U;
}


// define different kinds of LDP TLV  and LDP TLV header type

#define LDP_FEC_TLV                        0x0100
#define LDP_GENERIC_LABEL_TLV              0x0200
#define LDP_ADDRESS_LIST_TLV               0x0101
#define LDP_COMMON_HELLO_PARAMETERS_TLV    0x0400
#define LDP_COMMON_SESSION_PARAMETER_TLV   0x0500
#define LDP_PATH_VECTOR_TLV                0x0104
#define LDP_HOP_COUNT_TLV                  0x0103
#define LDP_STATUS_TLV                     0x0300
#define LDP_EXTENDED_STATUS_TLV            0x0301
#define LDP_RETURNED_PDU_TLV               0x0302
#define LDP_RETURNED_MSG_TLV               0x0303


//define LDP TLV header Structure
typedef struct
{
    UInt16 tlvHdr ;
                    //U:1,// Unknown TLV bit.  If set to 1, ignore
                               // unknown TLV's silently, else return a
                               // notification to the originator

                   //F:1,// Forward Unknown TLV bit.  If Unknown TLV
                               // bit and F bit set, unknown TLV is forwarded
                               // with the containing message.

                   //Type:14;// Encodes how the value field is to be
                               // interpreted.

    unsigned short Length;     // Specifies the length of the Value field
                               // in octets - the value field follows this
                               // one but is not explicitly included in this
                               // structure;
} LDP_TLV_Header; // V occurs after this header

//-------------------------------------------------------------------------
// FUNCTION     : LDP_TLV_HeaderSetU()
//
// PURPOSE      : Set the value of U for LDP_TLV_Header
//
// PARAMETERS   : tlvHdr - The variable containing the value of U,F and Type
//                U      - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_TLV_HeaderSetU(UInt16 *tlvHdr, BOOL U)
{
    UInt16 x = (UInt16) U;

    //masks U within boundry range
    U = U & maskShort(16, 16);

    //clears the first bit
    *tlvHdr = *tlvHdr & (~(maskShort(1, 1)));

    //setting the value of x in ldpMsg
    *tlvHdr = *tlvHdr | LshiftShort(x, 1);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_TLV_HeaderSetF()
//
// PURPOSE      : Set the value of F for LDP_TLV_Header
//
// PARAMETERS   : tlvHdr - The variable containing the value of U,F and Type
//                F     - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_TLV_HeaderSetF(UInt16 *tlvHdr, BOOL F)
{
    UInt16 x = (UInt16) F;

    //masks F within boundry range
    F = F & maskShort(16, 16);

    //clears the second bit
    *tlvHdr = *tlvHdr & (~(maskShort(2, 2)));

    //setting the value of x in tlvHdr
    *tlvHdr = *tlvHdr | LshiftShort(x, 2);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_TLV_HeaderSetType()
//
// PURPOSE      : Set the value of Type for LDP_TLV_Header
//
// PARAMETERS   : tlvHdr - The variable containing the value of U,F and Type
//                Type   - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_TLV_HeaderSetType(UInt16 *tlvHdr, UInt16 Type)
{
    //masks Type within boundry range
    Type = Type & maskShort(3, 16);

    //clears all the bits except first two
    *tlvHdr = *tlvHdr & maskShort(1, 2);

    //setting the value of Type in tlvHdr
    *tlvHdr = *tlvHdr | Type;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_TLV_HeaderGetU()
//
// PURPOSE      : Returns the value of U for LDP_TLV_Header
//
// PARAMETERS   : tlvHdr - The variable containing the value of U,F and Type
//
// RETURN VALUE : BOOL
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL LDP_TLV_HeaderGetU(UInt16 tlvHdr)
{
    UInt16 U = tlvHdr;

    //clears all the bits except first bit
    U = U & maskShort(1, 1);

    //right shifts 15 bits so that last bit represents U
    U = RshiftShort(U, 1);

    return (BOOL)U;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_TLV_HeaderGetF()
//
// PURPOSE      : Returns the value of F for LDP_TLV_Header
//
// PARAMETERS   : tlvHdr - The variable containing the value of U,F and Type
//
// RETURN VALUE : BOOL
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL LDP_TLV_HeaderGetF(UInt16 tlvHdr)
{
    UInt16 F = tlvHdr;

    //clears all the bits except second bit
    F = F & maskShort(2, 2);

    //right shifts 14 bits so that last bit represents F
    F = RshiftShort(F, 2);

    return (BOOL)F;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_TLV_HeaderGetType()
//
// PURPOSE      : Returns the value of Type for LDP_TLV_Header
//
// PARAMETERS   : tlvHdr - The variable containing the value of U,F and Type
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 LDP_TLV_HeaderGetType(UInt16 tlvHdr)
{
    UInt16 Type = tlvHdr;

    //clears the first two bits
    Type = Type & maskShort(3, 16);

    return Type;
}


// LDP Generic Label TLV:
// An LSR uses Generic Label TLVs to encode labels for use on links for
// which label values are independent of the underlying link technology.
// Examples of such links are PPP and Ether net.
typedef struct
{
    unsigned int Label; // This is a 20-bit label value as specified in
                        // [RFC3032] represented as a 20-bit number in a
                        // 4 octet field.
} LDP_Generic_Label_TLV_Value_Type;


// LDP Common Hello Parameters TLV

#define MPLS_LDP_LINK_HELLO                0
#define MPLS_LDP_TARGETED_HELLO            1
#define MPLS_LDP_NO_TARGETED_HELLO_REQUEST 0
#define MPLS_LDP_TARGETED_HELLO_REQUEST    1

typedef struct
{
    unsigned short Hold_Time;   // Hello Hold Time specifies the time the
                                // sending LSR will maintain its record of
                                // Hellos from the receiving LSR without
                                // receipt of another Hello in Seconds

    UInt16 lchpTlvValType;
                   //T:1,        // A value of 1 specifies that this Hello is a
                                // Targeted Hello.  A value of 0 specifies that
                                // this Hello is a Link Hello.

                   //R:1,         // A value of 1 requests the receiver to send
                                // periodic Targeted Hellos to the source of
                                // this Hello.  A value of 0 makes no request.

                   //Reserved:14; // This field is reserved.  It must be set to
                                // zero on transmission and ignored on receipt.
} LDP_Common_Hello_Parameters_TLV_Value_Type;

//-------------------------------------------------------------------------
// FUNCTION     : LDP_Common_Hello_Parameters_TLV_ValueSetT()
//
// PURPOSE      : Set the value of T for
//................LDP_Common_Hello_Parameters_TLV_Value_Type
//
// PARAMETERS   : lchpTlvValType - The variable containing the value of T,
//                                 R and reserved
//                T              - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_Common_Hello_Parameters_TLV_ValueSetT
(UInt16 *lchpTlvValType, BOOL T)
{
    UInt16 x = (UInt16)T;

    //masks T within boundry range
    x = x & maskShort(16, 16);

    //clears the first bit of lchpTlvValType
    *lchpTlvValType = *lchpTlvValType & (~(maskShort(1, 1)));

    //Setting the value of x at first position in lchpTlvValType
    *lchpTlvValType = *lchpTlvValType | LshiftShort(x, 1);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_Common_Hello_Parameters_TLV_ValueSetR()
//
// PURPOSE      : Set the value of R for
//................LDP_Common_Hello_Parameters_TLV_Value_Type
//
// PARAMETERS   : lchpTlvValType - The variable containing the value of T,
//                                 R and reserved
//                R              - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_Common_Hello_Parameters_TLV_ValueSetR
(UInt16 *lchpTlvValType, BOOL R)
{
    UInt16 x = (UInt16)R;

    //masks R within boundry range
    x = x & maskShort(16, 16);

    //clears the second bit of lchpTlvValType
    *lchpTlvValType = *lchpTlvValType & (~(maskShort(2, 2)));

    //Setting the value of x at second position in lchpTlvValType
    *lchpTlvValType = *lchpTlvValType | LshiftShort(x, 2);

}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_Common_Hello_Parameters_TLV_ValueSetReserved()
//
// PURPOSE      : Set the value of reserved for
//................LDP_Common_Hello_Parameters_TLV_Value_Type
//
// PARAMETERS   : lchpTlvValType - The variable containing the value of T,
//                                 R and reserved
//                reserved       - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_Common_Hello_Parameters_TLV_ValueSetReserved
(UInt16 *lchpTlvValType, UInt16 reserved)
{
    //masks reserved within boundry range
    reserved = reserved & maskShort(3, 16);

    //clears the last 14 bits of lchpTlvValType
    *lchpTlvValType = *lchpTlvValType & (maskShort(1, 2));

    //Setting the value of lchpTlvValType
    *lchpTlvValType = *lchpTlvValType | reserved;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_Common_Hello_Parameters_TLV_ValueGetT()
//
// PURPOSE      : Returns the value of T for
//                LDP_Common_Hello_Parameters_TLV_Value_Type
//
// PARAMETERS   : lchpTlvValType - The variable containing the value of T,
//                                   R and reserved
//
// RETURN VALUE : BOOL
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL LDP_Common_Hello_Parameters_TLV_ValueGetT (UInt16 lchpTlvValType)
{
    UInt16 T = lchpTlvValType;

    //clears all bits except first bit
    T = T & maskShort(1, 1);

    //right shifts 15 bits so that last bit contains the value of T
    T = RshiftShort(T, 1);

    return (BOOL) T;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_Common_Hello_Parameters_TLV_ValueGetR()
//
// PURPOSE      : Returns the value of R for
//                LDP_Common_Hello_Parameters_TLV_Value_Type
//
// PARAMETERS   : lchpTlvValType - The variable containing the value of T,
//                                   R and reserved
//
// RETURN VALUE : BOOL
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL LDP_Common_Hello_Parameters_TLV_ValueGetR (UInt16 lchpTlvValType)
{
    UInt16 R = lchpTlvValType;

    //clears all bits except second bit
    R = R & maskShort(2, 2);

    //right shifts 14 bits so that last bit contains the value of R
    R = RshiftShort(R, 2);

    return (BOOL)R;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_Common_Hello_Parameters_TLV_ValueGetReserved()
//
// PURPOSE      : Returns the value of reserved for
//................LDP_Common_Hello_Parameters_TLV_Value_Type
//
// PARAMETERS   : lchpTlvValType - The variable containing the value of T,
//                                   R and reserved
//
// RETURN VALUE : UInt16
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt16 LDP_Common_Hello_Parameters_TLV_ValueGetReserved
(UInt16 lchpTlvValType)
{
    UInt16 reserved = lchpTlvValType;

    //clears first and second bits
    reserved = reserved & maskShort(3, 16);

    return reserved;
}


// The structure of FEC TLV (Ref : RFC 3036 Section: 3.4.1
// this structure consists of TLV header and FEC Elements
// Structure of FEC Elements are as follows :

#define LDP_FEC_WILDCARD_ELEMENT     0x01
#define LDP_FEC_PREFIX_ELEMENT       0x02
#define LDP_FEC_HOST_ADDRESS_ELEMENT 0x03


// LDP Wildcard FEC Element  Ref : RFC 3036 section 3.4.1
// Size Calculation :
//     Element Type     1 byte
//                   ---------
//              total   1 byte
typedef struct
{
    unsigned char Element_Type;

    // No value- 0 value octets
} LDP_Wildcard_FEC_Element;


// structure defining the LDP prefix value
typedef struct
{
    unsigned short Address_Family; // a value from ADDRESS FAMILY NUMBERS in
                                   // [RFC1700] that encodes the address
                                   // family for the address prefix in the
                                   // Prefix field.

    unsigned char PreLen;    // the length in bits of the address prefix that
                             // follows.  A length of zero indicates a prefix
                             // that matches all addresses (the default
                             // destination)

    // Prefix follows, padded to byte boundary, length given in PreLen
} LDP_FEC_Prefix_Value_Type;


//
// LDP Prefix FEC Element
// Size Calculation
// element type = 1 byte
// prefix       = 3 byte
//            -----------
//          total 4 byte( minimum)   32 bits
typedef struct
{
    unsigned char                Element_Type;
    LDP_FEC_Prefix_Value_Type    Prefix;

    // Prefix follows, padded to byte boundary,
    // length given in Prefix.PreLen
} LDP_Prefix_FEC_Element;

typedef struct
{
    unsigned short Address_Family; // a value from ADDRESS FAMILY NUMBERS in
                                   // [RFC1700] that encodes the address
                                   // family for the address prefix in the
                                   // Prefix field.

    unsigned char HostAddrLen;     // Length of the Host address in octets.

    // Host Address follows, length given in HostAddrLen
} LDP_FEC_HostAddress_Value_Type;


// LDP Host Address FEC Element
// size calculation
// element type = 1 byte
// host address = 3 byte
//           -----------
//                4 byte( minimum)   32 bits
typedef struct
{
    unsigned char Element_Type;
    LDP_FEC_HostAddress_Value_Type  HostAddress;

    // Host Address follows, length given in HostAddress.HostAddrLen
} LDP_HostAddress_FEC_Element;


// The structure of ATM Label TLV RFC 3036 Section 3.4.2.2
// Size Calculation
// res + v_bits + vpi = (1 + 1 + 14) bits = 16 bits= 2 byte
// vci                                    =         2 byte
//                                                --------
//                                             4 byte 32 bits
typedef struct
{
      UInt16 ldpTlv;
                     //res    : 1, //2, // reserved

                     //v_bits : 1, //2, // Two-bit switching indicator.
                                 // If V-bits is 00, both the VPI and
                                 // VCI are significant.  If V-bits is 01,
                                 // only the VPI field is significant. If
                                 // V-bit is 10, only the VCI is
                                 // significant.

                     //vpi :14;    // Virtual Path Identifier.  If VPI is less
                                 // than 12-bits it should be right justified
                                 // in this field and preceding bits should
                                 // be set to 0.

      unsigned short vci;        // Virtual Channel Identifier.  If the VCI
                                 // is less than 16- bits, it should be right
                                 // justified in the field and the preceding
                                 // bits must be set to 0.  If Virtual Path
                                 // switching is indicated in the V-bits
                                 // field,then this field must be ignored
                                 // by the receiver and set to 0 by the
                                 // sender.
} LDP_ATM_TLV;

//-------------------------------------------------------------------------
// FUNCTION     : LDP_ATM_TLVSetRes()
//
// PURPOSE      : Set the value of res for LDP_ATM_TLV
//
// PARAMETERS   : ldpTlv - The variable containing the value of res,v_bits
//                         and vpi
//                res - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_ATM_TLVSetRes(UInt16 *ldpTlv, UInt16 res)
{
    //masks res within boundry range
    res = res & maskShort(16, 16);

    //clears first two bits
    *ldpTlv = *ldpTlv & maskShort(3, 16);

    //setting the value of res in ldpTlv
    *ldpTlv = *ldpTlv | res;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_ATM_TLVSetVBits()
//
// PURPOSE      : Set the value of v_bits for LDP_ATM_TLV
//
// PARAMETERS   : ldpTlv - The variable containing the value of res,v_bits
//                         and vpi
//                v_bits - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_ATM_TLVSetVBits(UInt16* ldpTlv, UInt16 v_bits)
{
    //masks v_bits within boundry range
    v_bits = v_bits & maskShort(16, 16);

    //clears bits 3-4
    *ldpTlv = *ldpTlv & (~(maskShort(3, 4)));

    //setting the value of res in ldpTlv
    *ldpTlv = *ldpTlv | v_bits;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_ATM_TLVSetVpi()
//
// PURPOSE      : Set the value of vpi for LDP_ATM_TLV
//
// PARAMETERS   : ldpTlv - The variable containing the value of res,v_bits
//                         and vpi
//                vpi    - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_ATM_TLVSetVpi(UInt16* ldpTlv, UInt16 vpi)
{
    //masks vpi within boundry range
    vpi = vpi & maskShort(3, 16);

    //clears all bits except first 4 bits
    *ldpTlv = *ldpTlv & maskShort(1, 4);

    //setting the value of vpi in ldpTlv
    *ldpTlv = *ldpTlv | vpi;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_ATM_TLVGetRes()
//
// PURPOSE      : Returns the value of res for LDP_ATM_TLV
//
// PARAMETERS   : ldpTlv - The variable containing the value of res,v_bits
//                          and vpi
//
// RETURN VALUE : UInt16
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt16 LDP_ATM_TLVGetRes(UInt16 ldpTlv)
{
    UInt16 res = ldpTlv;

    //clears all the bits except first two
    res = res & maskShort(1, 2);

    //right shifts 14 bits so that last 2 bits represent res
    res = RshiftShort(res, 2);

    return res;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_ATM_TLVGetVBits()
//
// PURPOSE      : Returns the value of v_bits for LDP_ATM_TLV
//
// PARAMETERS   : ldpTlv - The variable containing the value of res,v_bits
//                          and vpi
//
// RETURN VALUE : UInt16
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt16 LDP_ATM_TLVGetVBits(UInt16 ldpTlv)
{
    UInt16 v_bits = ldpTlv;

    //clears all the bits except first two
    v_bits = v_bits & maskShort(3, 4);

    //right shifts 14 bits so that last 2 bits represent res
    v_bits = RshiftShort(v_bits, 4);

    return v_bits;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_ATM_TLVGetVpi()
//
// PURPOSE      : Returns the value of vpi for LDP_ATM_TLV
//
// PARAMETERS   : ldpTlv - The variable containing the value of res,v_bits
//                          and vpi
//
// RETURN VALUE : UInt16
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt16 LDP_ATM_TLVGetVpi(UInt16 ldpTlv)
{
    UInt16 vpi = ldpTlv;

    //clears all bits except first 4 bits
    vpi = vpi & maskShort(5, 16);

    return vpi;
}



// The structure of Relay TLV RFC 3036  Section :3.4.2.3
// Size Calculation :
// (reserved + len + dlci)=(6 + 2 + 24) bits = 32 bits = 4 bytes = 32 bits
typedef struct
{
    UInt32 frameParam;
                 //reserved : 6,
                 //len      : 2,  // This field specifies the number of bits
                                // of the DLCI.The following values are
                                // supported:
                                //          0 = 10 bits DLCI
                                //          2 = 23 bits DLCI
                                // Len values 1 and 3 are reserved.

                 //dlci     : 24;
} LDP_FrameRelay_TLV;

//-------------------------------------------------------------------------
// FUNCTION     : LDP_FrameRelay_TLVSetResv()
//
// PURPOSE      : Set the value of reserved for LDP_FrameRelay_TLV
//
// PARAMETERS   : frameParam - The variable containing the value of reserved
//                             ,len and dlci
//                resv        - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_FrameRelay_TLVSetResv(UInt32 *frameParam, UInt32 resv)
{
    //masks reserved within boundry range
    resv = resv & maskInt(27, 32);

    //clears bits 1-6
    *frameParam = *frameParam & (~(maskInt(1, 6)));

    //setting the value of resv in frameParam
    *frameParam = *frameParam | LshiftInt(resv, 6);
}
//-------------------------------------------------------------------------
// FUNCTION     : LDP_FrameRelay_TLVSetLen()
//
// PURPOSE      : Set the value of len for LDP_FrameRelay_TLV
//
// PARAMETERS   : frameParam - The variable containing the value of reserved
//                             ,len and dlci
//                len        - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_FrameRelay_TLVSetLen(UInt32 *frameParam, UInt32 len)
{
    //masks len within boundry range
    len = len & maskInt(31, 32);

    //clears bits 7-8
    *frameParam = *frameParam & (~(maskInt(7, 8)));

    //setting the value of len in frameParam
    *frameParam = *frameParam | LshiftInt(len, 8);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_FrameRelay_TLVSetDlci()
//
// PURPOSE      : Set the value of dlci for LDP_FrameRelay_TLV
//
// PARAMETERS   : frameParam - The variable containing the value of reserved
//                             ,len and dlci
//                dlci        - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_FrameRelay_TLVSetDlci(UInt32 *frameParam, UInt32 dlci)
{
    //masks dlci within boundry range
    dlci = dlci & maskInt(9, 32);

    //clears 9-32 bits
    *frameParam = *frameParam & maskInt(1, 8);

    //setting the value of dlci in frameParam
    *frameParam = *frameParam | dlci;
}

//-------------------------------------------------------------------------
// FUNCTION     : LDP_FrameRelay_TLVGetResv()
//
// PURPOSE      : Returns the value of reserved for LDP_FrameRelay_TLV
//
// PARAMETERS   : frameParam - The variable containing the value of reserved
//                             ,len and dlci
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 LDP_FrameRelay_TLVGetResv(UInt32 frameParam)
{
    UInt32 reserved = frameParam;

    //clears all bits except 1-6
    reserved = reserved & maskInt(1,6);

    //right shifts 26 bits
    reserved = RshiftInt(reserved, 6);

    return reserved;
}

//-------------------------------------------------------------------------
// FUNCTION     : LDP_FrameRelay_TLVGetLen()
//
// PURPOSE      : Returns the value of len for LDP_FrameRelay_TLV
//
// PARAMETERS   : frameParam - The variable containing the value of reserved
//                             ,len and dlci
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 LDP_FrameRelay_TLVGetLen(UInt32 frameParam)
{
    UInt32 len = frameParam;

    //clears all bits except 7-8
    len = len & maskInt(7, 8);

    //right shifts 24 bits
    len = RshiftInt(len, 8);

    return len;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_FrameRelay_TLVGetDlci()
//
// PURPOSE      : Returns the value of dlci for LDP_FrameRelay_TLV
//
// PARAMETERS   : frameParam - The variable containing the value of reserved
//                             ,len and dlci
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 LDP_FrameRelay_TLVGetDlci(UInt32 frameParam)
{
    UInt32 dlci = frameParam;

    //clears first 8 bits
    dlci = dlci & maskInt(9, 32);

    return dlci;
}


// LDP Address List TLV: RFC 3036 Section:3.4.3
// The Address List TLV appears in Address and Address Withdraw messages.
// Size Calculation :
// Address family                      = 2 bytes
// variable number of address to follow= ?
//                                    -----------
//                                       2 bytes (minimum) 16 bits
#define LDP_ADDRESS_LIST_TLV_SIZE 2
typedef struct
{
    unsigned short Address_Family; // a value from ADDRESS FAMILY NUMBERS in
                                   // [RFC1700] that encodes the address
                                   // family for the address prefix in the
                                   // Prefix field.

    // Addresses follow
} LDP_Address_List_TLV_Value_Header_Type;


//
// structure of HOP count TLV. RFC 3036 Section:3.4.4
// Size Calculation :
// hop count value = 1 bytes
//              ------------
// total             1 bytes
#define SIZE_OF_HOP_COUNT_TLV 1
typedef struct
{
    unsigned char hop_count_value; // 1 octet unsigned integer
                                   // hop count value.
} LDP_HopCount_TLV_Value_Type;


// path vector TLV . RFC 3036 Section : 3.4.5
// (no structure definition consists of header and LSR ID list)

// LDP Common status TLV. RFC 3036 Section : 3.4.6
// Size calculation :
// E + F + status_data = (1 + 1 + 30)bits= 32 bits = 4 bytes
// message Id          =                             4 bytes
// message type        =                             2 bytes
//                                               -------------
//                                   total          10 bytes      80 bits
typedef struct
{
    UInt32 statusValue;
                //E : 1,   // Fatal error bit.  If set (=1), this is a fatal
                          // error notification.  If clear (=0), this is an
                          // advisory notification.

                //F : 1,   // Forward bit.  If set (=1), the notification
                          // should be forwarded to the LSR for the
                          // next-hop or previous-hop for the LSP, if any,
                          // associated with the event being signaled.
                          // If clear (=0), the notification should not
                          // be forwarded.

                //status_data : 30; // 30-bit unsigned integer which
                                   // specifies the status information.

    unsigned int message_id;// If non-zero, 32-bit value that identifies the
                            // peer message to which the Status TLV refers.If
                            // zero, no specific peer message is
                            // being identified.

    unsigned short message_type; // If non-zero, the type of the peer message
                                 // to which the Status TLV refers.  If zero,
                                 // the Status TLV does not refer to
                                 // any specific message type.
} LDP_Status_TLV_Value_Type;

//-------------------------------------------------------------------------
// FUNCTION     : LDP_Status_TLVSetE()
//
// PURPOSE      : Set the value of E for LDP_Status_TLV_Value_Type
//
// PARAMETERS   : statusValue - The variable containing the value of E,F and
//                              status_data
//                E           - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_Status_TLVSetE(UInt32 *statusValue, BOOL E)
{
    UInt32 x = E;

    //masks E within boundry range
    x = x & maskInt(32, 32);

    //clears first bit
    *statusValue = *statusValue & (~(maskInt(1, 1)));

    //setting the value of x in statusValue
    *statusValue = *statusValue | LshiftInt(x, 1);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_Status_TLVSetF()
//
// PURPOSE      : Set the value of F for LDP_Status_TLV_Value_Type
//
// PARAMETERS   : statusValue - The variable containing the value of E,F and
//                              status_data
//                F           - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_Status_TLVSetF(UInt32 *statusValue, BOOL F)
{
    UInt32 x = F;

    //masks F within boundry range
    x = x & maskInt(32, 32);

    //clears second bit
    *statusValue = *statusValue & (~(maskInt(2, 2)));

    //setting the value of x in statusValue
    *statusValue = *statusValue | LshiftInt(x, 2);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_Status_TLVSetStatus()
//
// PURPOSE      : Set the value of status_data for LDP_Status_TLV_Value_Type
//
// PARAMETERS   : statusValue - The variable containing the value of E,F and
//                              status_data
//                statusData - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDP_Status_TLVSetStatus(UInt32 *statusValue, UInt32 statusData)
{
    //masks statusData within boundry range
    statusData = statusData & maskInt(3, 32);

    //clears all the bits except first 2
    *statusValue = *statusValue & maskInt(1, 2);

    //setting the value of statusData in statusValue
    *statusValue = *statusValue | statusData;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_Status_TLVGetStatus()
//
// PURPOSE      : Returns the value of status_data for
//                LDP_Status_TLV_Value_Type
//
// PARAMETERS   : statusValue - The variable containing the value of E,F and
//                             status_data
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 LDP_Status_TLVGetStatus(UInt32 statusValue)
{
    UInt32 statusData = statusValue;

    //clears first two bits
    statusData = statusData & (maskInt(3, 32));

    return statusData;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_Status_TLVGetE()
//
// PURPOSE      : Returns the value of E for LDP_Status_TLV_Value_Type
//
// PARAMETERS   : statusValue - The variable containing the value of E,F and
//                             status_data
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL LDP_Status_TLVGetE(UInt32 statusValue)
{
    UInt32 E = statusValue;

    //clears all bits except first
    E = E & maskInt(1, 1);

    //right shift 31 places so that last bit represents E
    E = RshiftInt(E, 1);

    return (BOOL)E;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDP_Status_TLVGetF()
//
// PURPOSE      : Returns the value of F for LDP_Status_TLV_Value_Type
//
// PARAMETERS   : statusValue - The variable containing the value of E,F and
//                             status_data
//
// RETURN VALUE : BOOL
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL LDP_Status_TLVGetF(UInt32 statusValue)
{
    UInt32 F = statusValue;

    //clears all bits except second
    F = F & maskInt(2, 2);

    //right shift 30 places so that last bit represents F
    F = RshiftInt(F, 2);

    return (BOOL)F;
}



// LDP common session TLV. RFC 3036 Section :3.5.7
// protocol version                                        = 2 bytes
// keep alive time                                         = 2 bytes
// A + D + reserved + path vector lim =( 1 + 1 + 6+ 8)bits = 2 bytes
// max pdu length                                          = 2 bytes
// ldp identifier (see LDP_Identifier_Type)                = 6 bytes
//                                                       -------------
//                                   total                 14 bytes 112 bits
#define COMMON_SESSION_TLV_SIZE  14
typedef struct
{
    unsigned short protocol_version;// Two octet unsigned integer containing
                                    // the version number of the protocol.
                                    // This version of the specification
                                    // specifies LDP protocol version 1.

    unsigned short keepalive_time;  // Two octet unsigned non zero integer
                                    // that indicates the number of seconds
                                    // that the sending LSR proposes for
                                    // the value of the KeepAlive Time.The
                                    // receiving LSR MUST calculate the
                                    // value of the KeepAlive Timer by
                                    // using the smaller of its proposed
                                    // KeepAlive Time and the KeepAlive
                                    // Time received in the PDU.  The value
                                    // chosen for KeepAlive Time indicates
                                    // the maximum number of seconds that
                                    // may elapse between the receipt of
                                    // successive PDUs from the LDP peer on
                                    // the session TCP connection.The
                                    // KeepAlive Timer is reset
                                    // each time a PDU arrives.

    UInt16 ldpCsTlvValType;
                  //A:1, // Label Advertisement Discipline.Indicates the type
                         // of Label advertisement.  A value of 0 means
                         // Downstream Unsolicited advertisement;
                         // a value of 1 means Downstream On Demand.

                  //D:1, // Indicates whether loop detection based on path
                         // vectors is enabled.  A value of 0 means loop
                         // detection is disabled; a value of 1 means that
                         // loop detection is enabled.

                  //reserved:6;

                  //path_vector_lim:8; //Path vector limit

    unsigned short max_pdu_length; // Two octet unsigned integer that
                                   // proposes the maximum allowable length
                                   // for LDP PDUs for the session.A value
                                   // of 255 or less specifies the default
                                   // maximum length of 4096 octets.

    LDP_Identifier_Type ldp_identifier; // Receiver LDP Identifier

} LDP_Common_Session_TLV_Value_Type;

//-------------------------------------------------------------------------
// FUNCTION     : LDPCommonSessionTLVSetA()
//
// PURPOSE      : Set the value of A for
//................LDP_Common_Session_TLV_Value_Type
//
// PARAMETERS   : ldpCsTlvValType - The variable containing the value of
//                                   A,D,reserved and path_vector_lim
//                A               - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDPCommonSessionTLVSetA (UInt16 *ldpCsTlvValType, BOOL A)
{
    UInt16 x = (UInt16)A;

    //masks A within boundry range
    x = x & maskShort(16, 16);

    //clears the first bit of ldpCsTlvValType
    *ldpCsTlvValType = *ldpCsTlvValType & (~(maskShort(1, 1)));

    //Setting the value of x at first position in ldpCsTlvValType
    *ldpCsTlvValType = *ldpCsTlvValType | LshiftShort(x, 1);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPCommonSessionTLVSetD()
//
// PURPOSE      : Set the value of D for
//................LDP_Common_Session_TLV_Value_Type
//
// PARAMETERS   : ldpCsTlvValType - The variable containing the value of
//                                   A,D,reserved and path_vector_lim
//                D               - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDPCommonSessionTLVSetD (UInt16 *ldpCsTlvValType, BOOL D)
{
    UInt16 x = (UInt16)D;

    //masks D within boundry range
    x = x & maskShort(16, 16);

    //clears the second bit of ldpCsTlvValType
    *ldpCsTlvValType = *ldpCsTlvValType & (~(maskShort(2, 2)));

    //Setting the value of x at second position in ldpCsTlvValType
    *ldpCsTlvValType = *ldpCsTlvValType | LshiftShort(x, 2);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPCommonSessionTLVSetReserved()
//
// PURPOSE      : Set the value of reserved for
//................LDP_Common_Session_TLV_Value_Type
//
// PARAMETERS   : ldpCsTlvValType - The variable containing the value of
//                                   A,D,reserved and path_vector_lim
//                reserved        - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDPCommonSessionTLVSetReserved(UInt16 *ldpCsTlvValType,
                                           UInt16 reserved)
{
    //masks reserved within boundry range
    reserved = reserved & maskShort(11, 16);

    //clears the 3-8 bits of ldpCsTlvValType
    *ldpCsTlvValType = *ldpCsTlvValType & (~(maskShort(3, 8)));

    //Setting the value of reserved in ldpCsTlvValType
    *ldpCsTlvValType = *ldpCsTlvValType | LshiftShort(reserved, 8);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPCommonSessionTLVSetPVL()
//
// PURPOSE      : Set the value of path_vector_lim for
//................LDP_Common_Session_TLV_Value_Type
//
// PARAMETERS   : ldpCsTlvValType - The variable containing the value of
//                                   A,D,reserved and path_vector_lim
//                pvl        - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDPCommonSessionTLVSetPVL(UInt16 *ldpCsTlvValType, UInt16 pvl)
{
    //masks pvl within boundry range
    pvl = pvl & maskShort(9, 16);

    //clears the 1-8 bits of ldpCsTlvValType
    *ldpCsTlvValType = *ldpCsTlvValType & maskShort(1, 8);

    //Setting the value of pvl in ldpCsTlvValType
    *ldpCsTlvValType = *ldpCsTlvValType | pvl;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPCommonSessionTLVGetA()
//
// PURPOSE      : Returns the value of A for
//                LDP_Common_Session_TLV_Value_Type
//
// PARAMETERS   : ldpCsTlvValType -  The variable containing the value of
//                                      A,D,reserved and path_vector_lim
//
// RETURN VALUE : BOOL
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL LDPCommonSessionTLVGetA (UInt16 ldpCsTlvValType)
{
    UInt16 A = ldpCsTlvValType;

    //clears all bits except first bit
    A = A & maskShort(1, 1);

    //right shifts 15 bits so that last bit contains the value of A
    A = RshiftShort(A, 1);

    return (BOOL) A;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPCommonSessionTLVGetD()
//
// PURPOSE      : Returns the value of D for
//                LDP_Common_Session_TLV_Value_Type
//
// PARAMETERS   : ldpCsTlvValType -  The variable containing the value of
//                                      A,D,reserved and path_vector_lim
//
// RETURN VALUE : BOOL
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL LDPCommonSessionTLVGetD (UInt16 ldpCsTlvValType)
{
    UInt16 D = ldpCsTlvValType;

    //clears all bits except second bit
    D = D & maskShort(2, 2);

    //right shifts 14 bits so that last bit contains the value of D
    D = RshiftShort(D, 2);

    return (BOOL)D;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPCommonSessionTLVGetReserved()
//
// PURPOSE      : Returns the value of reserved for
//................LDP_Common_Session_TLV_Value_Type
//
// PARAMETERS   : ldpCsTlvValType -  The variable containing the value of
//                                      A,D,reserved and path_vector_lim
//
// RETURN VALUE : UInt16
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt16 LDPCommonSessionTLVGetReserved
(UInt16 ldpCsTlvValType)
{
    UInt16 reserved = ldpCsTlvValType;

    //clears all except 3-8 bits
    reserved = reserved & maskShort(3, 8);

    //right shift so that last 8 bits represent reserved
    reserved = RshiftShort(reserved, 8);

    return reserved;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPCommonSessionTLVGetPVL()
//
// PURPOSE      : Returns the value of path_vector_lim for
//................LDP_Common_Session_TLV_Value_Type
//
// PARAMETERS   : ldpCsTlvValType -  The variable containing the value of
//                                      A,D,reserved and path_vector_lim
//
// RETURN VALUE : UInt16
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt16 LDPCommonSessionTLVGetPVL(UInt16 ldpCsTlvValType)
{
    UInt16 pvl = ldpCsTlvValType;

    //clears 1-8 bits
    pvl = pvl & maskShort(9, 16);

    return pvl;
}


// TLV ATM common session parameter RFC 3060 Section: 3.5.3
// (M + N + D + Reserved)=(2 + 4 + 1 + 25)bits= 32 bits
typedef struct
{
    UInt32 ldpAtmCsTlvValue;
            //M :2,
            //N :4,
            //D :1,
            //Reserved : 25;

    // followed by ATM label range component
} LDP_ATM_Common_Session_TLV_value;

//-------------------------------------------------------------------------
// FUNCTION     : LDPATMCommonSessionTLVSetM()
//
// PURPOSE      : Set the value of M for
//................LDP_ATM_Common_Session_TLV_value
//
// PARAMETERS   : ldpAtmCsTlvValue - The variable containing the value of
//                                   M,N,D and Reserved
//                M                - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDPATMCommonSessionTLVSetM(UInt32 *ldpAtmCsTlvValue, UInt32 M)
{
    //masks M within boundry range
    M = M & maskInt(31, 32);

    //clears the first two bits of ldpAtmCsTlvValue
    *ldpAtmCsTlvValue = *ldpAtmCsTlvValue & maskInt(3, 32);

    //Setting the value of M in ldpAtmCsTlvValue
    *ldpAtmCsTlvValue = *ldpAtmCsTlvValue | LshiftInt(M, 2);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPATMCommonSessionTLVSetN()
//
// PURPOSE      : Set the value of N for
//................LDP_ATM_Common_Session_TLV_value
//
// PARAMETERS   : ldpAtmCsTlvValue - The variable containing the value of
//                                   M,N,D and Reserved
//                N                - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDPATMCommonSessionTLVSetN(UInt32 *ldpAtmCsTlvValue, UInt32 N)
{
    //masks N within boundry range
    N = N & maskInt(29, 32);

    //clears the 3-6 bits of ldpAtmCsTlvValue
    *ldpAtmCsTlvValue = *ldpAtmCsTlvValue & (~(maskInt(3, 6)));

    //Setting the value of N in ldpAtmCsTlvValue
    *ldpAtmCsTlvValue = *ldpAtmCsTlvValue | LshiftInt(N, 6);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPATMCommonSessionTLVSetD()
//
// PURPOSE      : Set the value of D for
//................LDP_ATM_Common_Session_TLV_value
//
// PARAMETERS   : ldpAtmCsTlvValue - The variable containing the value of
//                                   M,N,D and Reserved
//                D                - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDPATMCommonSessionTLVSetD(UInt32 *ldpAtmCsTlvValue, BOOL D)
{
    //masks D within boundry range
    D = D & maskInt(32, 32);

    //clears the seventh bit of ldpAtmCsTlvValue
    *ldpAtmCsTlvValue = *ldpAtmCsTlvValue & (~(maskInt(7, 7)));

    //Setting the value of D in ldpAtmCsTlvValue
    *ldpAtmCsTlvValue = *ldpAtmCsTlvValue | LshiftInt(D, 7);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPATMCommonSessionTLVSetReserved()
//
// PURPOSE      : Set the value of reserved for
//................LDP_ATM_Common_Session_TLV_value
//
// PARAMETERS   : ldpAtmCsTlvValue - The variable containing the value of
//                                   M,N,D and Reserved
//                reserved         - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDPATMCommonSessionTLVSetReserved(UInt32 *ldpAtmCsTlvValue,
                                              UInt32 reserved)
{
    //masks reserved within boundry range
    reserved = reserved & maskInt(8, 32);

    //clears the 1-7 bit of ldpAtmCsTlvValue
    *ldpAtmCsTlvValue = *ldpAtmCsTlvValue & maskInt(1, 7);

    //Setting the value of reserved in ldpAtmCsTlvValue
    *ldpAtmCsTlvValue = *ldpAtmCsTlvValue | reserved;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPATMCommonSessionTLVGetM()
//
// PURPOSE      : Returns the value of M for
//................LDP_ATM_Common_Session_TLV_value
//
// PARAMETERS   : ldpAtmCsTlvValue - The variable containing the value of
//                                       M,N,D and Reserved
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 LDPATMCommonSessionTLVGetM
(UInt32 ldpAtmCsTlvValue)
{
    UInt32 M = ldpAtmCsTlvValue;

    //clears all the bits except 1-2
    M = M & maskInt(1, 2);

    //right shift so that last 2 bits represents M
    M = RshiftInt(M, 2);

    return M;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPATMCommonSessionTLVGetN()
//
// PURPOSE      : Returns the value of N for
//................LDP_ATM_Common_Session_TLV_value
//
// PARAMETERS   : ldpAtmCsTlvValue - The variable containing the value of
//                                       M,N,D and Reserved
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 LDPATMCommonSessionTLVGetN
(UInt32 ldpAtmCsTlvValue)
{
    UInt32 N = ldpAtmCsTlvValue;

    //clears all the bits except 3-6
    N = N & maskInt(3, 6);

    //right shift so that last 4 bits represents N
    N = RshiftInt(N, 6);

    return N;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPATMCommonSessionTLVGetD()
//
// PURPOSE      : Returns the value of D for
//................LDP_ATM_Common_Session_TLV_value
//
// PARAMETERS   : ldpAtmCsTlvValue - The variable containing the value of
//                                       M,N,D and Reserved
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 LDPATMCommonSessionTLVGetD
(UInt32 ldpAtmCsTlvValue)
{
    UInt32 D = ldpAtmCsTlvValue;

    //clears all the bits except seventh
    D = D & maskInt(7, 7);

    //right shift so that last bits represents D
    D = RshiftInt(D, 7);

    return D;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPATMCommonSessionTLVGetReserved()
//
// PURPOSE      : Returns the value of reserved for
//................LDP_ATM_Common_Session_TLV_value
//
// PARAMETERS   : ldpAtmCsTlvValue - The variable containing the value of
//                                       M,N,D and Reserved
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 LDPATMCommonSessionTLVGetReserved
(UInt32 ldpAtmCsTlvValue)
{
    UInt32 reserved = ldpAtmCsTlvValue;

    //clears all the bits except seventh
    reserved = reserved & maskInt(8, 32);

    return reserved;
}


// TLV FRAME relay common session parameters
typedef struct
{
    UInt32 ldpAtmFrTlvValue;
            //M :2,
            //N :4,
            //D :1,
            //Reserved : 25;

    // followed by FRAME RELAY label range component
} LDP_ATM_FrameRelay_TLV_value;

//-------------------------------------------------------------------------
// FUNCTION     : LDPATMFrameRelayTLVSetM()
//
// PURPOSE      : Set the value of M for
//................LDP_ATM_FrameRelay_TLV_value
//
// PARAMETERS   : ldpAtmFrTlvValue - The variable containing the value of
//                                   M,N,D and Reserved
//                M                - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDPATMFrameRelayTLVSetM(UInt32 *ldpAtmFrTlvValue, UInt32 M)
{
    //masks M within boundry range
    M = M & maskInt(31, 32);

    //clears the first two bits of ldpAtmFrTlvValue
    *ldpAtmFrTlvValue = *ldpAtmFrTlvValue & maskInt(3, 32);

    //Setting the value of M in ldpAtmFrTlvValue
    *ldpAtmFrTlvValue = *ldpAtmFrTlvValue | LshiftInt(M, 2);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPATMFrameRelayTLVSetN()
//
// PURPOSE      : Set the value of N for
//................LDP_ATM_FrameRelay_TLV_value
//
// PARAMETERS   : ldpAtmFrTlvValue - The variable containing the value of
//                                   M,N,D and Reserved
//                N                - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDPATMFrameRelayTLVSetN(UInt32 *ldpAtmFrTlvValue, UInt32 N)
{
    //masks N within boundry range
    N = N & maskInt(29, 32);

    //clears the 3-6 bits of ldpAtmFrTlvValue
    *ldpAtmFrTlvValue = *ldpAtmFrTlvValue & (~(maskInt(3, 6)));

    //Setting the value of N in ldpAtmFrTlvValue
    *ldpAtmFrTlvValue = *ldpAtmFrTlvValue | LshiftInt(N, 6);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPATMFrameRelayTLVSetD()
//
// PURPOSE      : Set the value of D for
//................LDP_ATM_FrameRelay_TLV_value
//
// PARAMETERS   : ldpAtmFrTlvValue - The variable containing the value of
//                                   M,N,D and Reserved
//                D                - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDPATMFrameRelayTLVSetD(UInt32 *ldpAtmFrTlvValue, BOOL D)
{
    //masks D within boundry range
    D = D & maskInt(32, 32);

    //clears the seventh bit of ldpAtmFrTlvValue
    *ldpAtmFrTlvValue = *ldpAtmFrTlvValue & (~(maskInt(7, 7)));

    //Setting the value of D in ldpAtmFrTlvValue
    *ldpAtmFrTlvValue = *ldpAtmFrTlvValue | LshiftInt(D, 7);
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPATMFrameRelayTLVSetReserved()
//
// PURPOSE      : Set the value of reserved for
//................LDP_ATM_FrameRelay_TLV_value
//
// PARAMETERS   : ldpAtmFrTlvValue - The variable containing the value of
//                                   M,N,D and Reserved
//                reserved         - Input value for set operation
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void LDPATMFrameRelayTLVSetReserved(UInt32 *ldpAtmFrTlvValue,
                                           UInt32 reserved)
{
    //masks reserved within boundry range
    reserved = reserved & maskInt(8, 32);

    //clears the 1-7 bit of ldpAtmFrTlvValue
    *ldpAtmFrTlvValue = *ldpAtmFrTlvValue & maskInt(1, 7);

    //Setting the value of reserved in ldpAtmFrTlvValue
    *ldpAtmFrTlvValue = *ldpAtmFrTlvValue | reserved;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPATMFrameRelayTLVGetM()
//
// PURPOSE      : Returns the value of M for
//................LDP_ATM_FrameRelay_TLV_value
//
// PARAMETERS   : ldpAtmFrTlvValue - The variable containing the value of
//                                       M,N,D and Reserved
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 LDPATMFrameRelayTLVGetM
(UInt32 ldpAtmFrTlvValue)
{
    UInt32 M = ldpAtmFrTlvValue;

    //clears all the bits except 1-2
    M = M & maskInt(1, 2);

    //right shift so that last 2 bits represents M
    M = RshiftInt(M, 2);

    return M;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPATMFrameRelayTLVGetN()
//
// PURPOSE      : Returns the value of N for
//................LDP_ATM_FrameRelay_TLV_value
//
// PARAMETERS   : ldpAtmFrTlvValue - The variable containing the value of
//                                       M,N,D and Reserved
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 LDPATMFrameRelayTLVGetN
(UInt32 ldpAtmFrTlvValue)
{
    UInt32 N = ldpAtmFrTlvValue;

    //clears all the bits except 3-6
    N = N & maskInt(3, 6);

    //right shift so that last 4 bits represents N
    N = RshiftInt(N, 6);

    return N;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPATMFrameRelayTLVGetD()
//
// PURPOSE      : Returns the value of D for
//................LDP_ATM_FrameRelay_TLV_value
//
// PARAMETERS   : ldpAtmFrTlvValue - The variable containing the value of
//                                       M,N,D and Reserved
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 LDPATMFrameRelayTLVGetD
(UInt32 ldpAtmFrTlvValue)
{
    UInt32 D = ldpAtmFrTlvValue;

    //clears all the bits except seventh
    D = D & maskInt(7, 7);

    //right shift so that last bits represents D
    D = RshiftInt(D, 7);

    return D;
}


//-------------------------------------------------------------------------
// FUNCTION     : LDPATMFrameRelayTLVGetReserved()
//
// PURPOSE      : Returns the value of reserved for
//................LDP_ATM_FrameRelay_TLV_value
//
// PARAMETERS   : ldpAtmFrTlvValue - The variable containing the value of
//                                       M,N,D and Reserved
//
// RETURN VALUE : UInt32
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 LDPATMFrameRelayTLVGetReserved
(UInt32 ldpAtmFrTlvValue)
{
    UInt32 reserved = ldpAtmFrTlvValue;

    //clears all the bits except seventh
    reserved = reserved & maskInt(8, 32);

    return reserved;
}


// LDP_Label_Request_MessageId_TLV_Value_Type TLV
typedef struct
{
    unsigned int message_id;
} LDP_Label_Request_MessageId_TLV_Value_Type;


// vendor private TLV
typedef struct
{
    unsigned int vendor_id;

    // Data item of variable follows
} LDP_VendorSpecific_TLV_Value_Type;


typedef struct
{
    unsigned int extended_status;
} LDP_Extended_Status_TLV_Type;

//
// This section defines LDP messages .
//

//  LDP notification message structure. RFC 3036 Section :3.5.1
//  An LSR sends a Notification message to inform an LDP peer of a
//  significant event.  A Notification message signals a fatal error or
//  provides advisory information such as the outcome of processing an
//  LDP message or the state of the LDP session.
//  The message contains:
//  1) LDP_Message_Header
//  2) LDP_TLV_Header
//  3) LDP_Status_TLV_Value_Type
//  4) variable length optional data
//
//
//  LDP Hello Messages are exchanged as part of the LDP Discovery
//  Mechanism; see Section "LDP Discovery".RFC 3036 section 3.5.2
//  The message contains :
//  1)LDP_Message_Header
//  2)LDP_TLV_Header
//  3)LDP_Common_Hello_Parameters_TLV_Value_Type
//  4)variable length optional field
//
//
//  The LDP Initialization Message is exchanged as part of the LDP
//  session establishment procedure; see Section "LDP Session
//  Establishment".RFC 3036 , Section 3.5.3
//  The message contains
//  1)LDP_Message_Header
//  2)LDP_TLV_Header
//  3)LDP_Common_Session_TLV_Value_Type
//  4)variable length optional field
//
//
//  An LSR sends KeepAlive Messages as part of a mechanism that monitors
//  the integrity of the LDP session transport connection.
//  RFC 3036 section 3.5.5
//  The structure contains
//  1)LDP_Message_Header
//  2)variable length optional field
//
//
//  An LSR sends the Address Message to an LDP peer to advertise its
//  interface addresses.
//  The structure contains
//  1)LDP_Message_Header
//  2)LDP_TLV_Header
//  3)LDP_Address_List_TLV_Value_Header_Type
//  4)variable length optional field
//
//
//  An LSR sends the Address Withdraw Message to an LDP peer to withdraw
//  previously advertised interface addresses.RFC 3036 section 3.5.6
//  The structure contains
//  1)LDP_Message_Header
//  2)LDP_TLV_Header
//  3)LDP_Address_List_TLV_Value_Header_Type
//  4)variable length optional field
//
//
//  An LSR sends a Label Mapping message to an LDP peer to advertise
//  FEC-label bindings to the peer.RFC 3036 section 3.5.7
//  The structure contains
//  1)LDP_Message_Header
//  2)LDP_TLV_Header
//  3)FEC TLV value type
//  4)LDP_TLV_Header
//  5)LDP_Generic_Label_TLV_Value_Type;
//  6)optional parameters;
//
//
//  An LSR sends the Label Request Message to an LDP peer to request a
//  binding (mapping) for a FEC. RFC 3036 section 3.5.8
//  The structure contains
//  1)LDP_Message_Header
//  2)LDP_TLV_Header
//  3)FEC TLV value type
//  4)optional parameters;
//
//
//  The Label Abort Request message may be used to abort an outstanding
//  Label Request message.RFC 3036 section 3.5.9
//  The structure contains
//  1)LDP_Message_Header
//  2)LDP_TLV_Header
//  3)FEC TLV value type
//  4)LDP_TLV_Header
//  5)LDP_Label_Request_MessageId_TLV_Value_Type;
//  6)Optional parameters.
//
//
//  An LSR sends a Label Withdraw Message to an LDP peer to signal the
//  peer that the peer may not continue to use specific FEC-label
//  mappings the LSR had previously advertised.  This breaks the mapping
//  between the FECs and the labels.RFC 3036 section 3.5.10
//  The structure contains
//  1)LDP_Message_Header
//  2)FEC TLV value type
//  3)LDP_TLV_Header(optional)
//  4)LDP_Label_Request_MessageId_TLV_Value_Type (optional)
//  5)Optional parameters
//
//  An LSR sends a Label Release message to an LDP peer to signal the
//  peer that the LSR no longer needs specific FEC-label mappings
//  previously requested of and/or advertised by the peer.
//  RFC 3036 section 3.5.11
//  The structure contains
//  1)LDP_Message_Header
//  2)LDP_TLV_Header
//  3)FEC TLV value type
//  4)LDP_TLV_Header
//  5)LDP_Label_Request_MessageId_TLV_Value_Type (optional)
//  6)Optional parameters
//


// define the structure "PathVectorType" which is used as one of the
// attributes in the structure "Attribute_Type" defined below
typedef struct
{
    unsigned int length;
    NodeAddress* pathVector; // contiguous list of addresses
} PathVectorType;


// the structure below defines the attribute type,
// which is used by label request/reply message.
typedef struct
{
    BOOL hopCount;
    BOOL pathVector;
    unsigned int numHop;
    PathVectorType pathVectorValue;
} Attribute_Type;


// define structure for MPLS-LDP incoming label request cache
typedef struct
{
    Mpls_FEC fec;
    NodeAddress LSR_Id;
    unsigned int label;
    clocktype entry_time;
} IncomingRequestCache;


// define structure for MPLS-LDP outbound label request cache
typedef struct
{
    Mpls_FEC fec;
    NodeAddress LSR_Id;
    unsigned int label;
    clocktype exit_time;
} OutboundLabelRequestCache;


// define structure for MPLS-LDP label-withdrawn-record table
typedef struct
{
    NodeAddress LSR_Id;
    Mpls_FEC fec;
    unsigned int labelWithdrawn;
} LabelWithdrawRec;


// The exchange of Hellos results in the creation of a Hello adjacency
// at LSR1 that serves to bind the link (L) and the label spaces LSR1:a
// and LSR2:b.
typedef struct
{
    NodeAddress LinkAddress;           // link (L)

    LDP_Identifier_Type LSR2;          // label space LSR2:b

    clocktype helloTimerInterval;
    clocktype lastHeard;
    int connectionId;
    MPLS_LDP_InitializationMode initMode;
    PacketBuffer* buffer;
    clocktype lastMsgSent;
    BOOL OkToSendLabelRequests;

    // For keeping track of incoming label requests
    unsigned int incomingCacheNumEntries;
    unsigned int incomingCacheMaxEntries;
    IncomingRequestCache* incomingCache;

    // For keeping track of outbound label requests.
    unsigned int outboundCacheNumEntries;
    unsigned int outboundCacheMaxEntries;
    OutboundLabelRequestCache* outboundCache;

    // For keeping track of outgoing label mappings
    unsigned int outboundLabelMappingNumEntries;
    unsigned int outboundLabelMappingMaxEntries;
    OutboundLabelRequestCache* outboundLabelMapping;

    BOOL hopCountRequird;

    // For keeping track of all interface Addresses
    // of this hello adjacency
    NodeAddress* interfaceAddresses;
    unsigned int numInterfaceAddress;

    // For re-assembling fragmented messages
    ReassemblyBuffer reassemblyBuffer;

} MplsLdpHelloAdjacency;


// An LSR maintains learned labels in a Label Information Base (LIB).
// When operating in Downstream Unsolicited mode, the LIB entry for an
// address prefix associates a collection of (LDP Identifier, label)
// pairs with the prefix, one such pair for each peer advertising a
// label for the prefix.
typedef struct label_information_base
{
    int length;   // determines the length of the Address Prefix
                  // field in bits.

    NodeAddress prefix;
    NodeAddress nextHop;
    LDP_Identifier_Type LSR_Id;
    unsigned int label;
} LIB;


// define cache structure to keep track of mappings
// from fec to labels.
typedef struct
{
    Mpls_FEC fec;
    unsigned int label;
    NodeAddress peer;
    Attribute_Type Attribute;
} FecToLabelMappingRec;


// define cache structure to keep track of
// the status of label request messages sent.
typedef struct
{
    Mpls_FEC fec;
    NodeAddress LSR_Id;
    BOOL statusOutstanding;
    Attribute_Type Attribute;
} StatusRecord;


// define cache structure for keeping track of
// pending label request messages.
typedef struct
{
    Mpls_FEC fec;
    NodeAddress sourceLSR_Id;
    NodeAddress nextHopLSR_Id;
} PendingRequest;


typedef struct
{
   NodeAddress LSR_Id;
   Mpls_FEC fec;
   Attribute_Type SAttribute;
} DefferedRequest;


typedef struct
{
    // number of label request message sent
    unsigned int numLabelRequestMsgSend;

    // number of label mappings sent
    unsigned int numLabelMappingSend;

    // number of notification message sent.
    unsigned int numNotificationMsgSend;

    // number of keep alive messages sent.
    unsigned int numKeepAliveMsgSend;

    // number of label withdraw message sent
    unsigned int numLabelWithdrawMsgSend;

    // number of label release message sent.
    unsigned int numLabelReleaseMsgSend;

    // number of address message sent
    unsigned int numAddressMsgSend;

    // number of initialization message sent
    unsigned int numInitializationMsgSend;

    // number of UDP hello messages sent
    unsigned int numUdpLinkHelloSend;

    // number of label request message received
    unsigned int numLabelRequestMsgReceived;

    // number of label mapping received.
    unsigned int numLabelMappingReceived;

    // number of notification message received.
    unsigned int numNotificationMsgReceived;

    // number of keep alive message received
    unsigned int numKeepAliveMsgReceived;

    // number of label withdraw message received
    unsigned int numLabelWithdrawMsgReceived;

    // number of label release message received
    unsigned int numLabelReleaseMsgReceived;

    // number of address message received.
    unsigned int numAddressMsgReceived;

    // total number of initilization message received
    unsigned int numInitializationMsgReceived;

    //  number of initialization message received in active mode.
    unsigned int numInitializationMsgReceivedAsActive;

    //  number of initialization message received in passive mode.
    unsigned int numInitializationMsgReceivedAsPassive;

    // number of link hello message received
    unsigned int numUdpLinkHelloReceived;
} MplsLdpStat;


// define the structure MplsLdpApp (the MPLS-LDP main structure)
typedef struct
{
    // space for holding local addresses
    NodeAddress* localAddr;
    unsigned int numLocalAddr;
    unsigned int maxLocalAddr;

    // space for hello adjacency
    MplsLdpHelloAdjacency* helloAdjacency;
    unsigned int numHelloAdjacency;
    unsigned int maxHelloAdjacency;

    // current message Id
    unsigned int currentMessageId;

    // LSR_Id of this LSR
    NodeAddress LSR_Id;

    // next lable to be allocated
    unsigned int  nextLabel;

    // label advertisement mode of the LSR
    unsigned char labelAdvertisementMode;

    // loop detection mode of the LSR
    BOOL loopDetection;

    // if LSR is configured to decrenent TTL
    BOOL decrementsTTL;

    // if LSR is member of TTL decrenenting Domain
    BOOL member_Dec_TTL_Domain;

    // if LSR supports Label Merging
    BOOL supportLabelMerging;

    // if LSR is configured to propagate label release message
    BOOL propagateLabelRelease;

    // The link with Peer may require that Hop Count be included in
    // Label Request messages; for example, see [RFC3035] and
    // [RFC3034].
    BOOL hopCountNeededInLabelRequest;

    // label distribution control mode of the LSR
    MPLS_LDP_ControlMode  labelDistributionControlMode;

    // label retention mode of the LSR
    MPLS_LDP_LabelRetentionMode labelRetentionMode;

    // maximum PDU length supported by LSR
    int maxPDULength;

    // maximum allowable hop count;
    int  maxAllowableHopCount;

    clocktype linkHelloHoldTime;
    clocktype linkHelloInterval;
    clocktype targetedHelloInterval;
    clocktype keepAliveTime;
    clocktype sendKeepAliveDelay;
    clocktype requestRetryTimer;

    unsigned int pathVectorLimit;

    // For Status record of outgoing request
    unsigned int outGoingRequestNumEntries;
    unsigned int outGoingRequestMaxEntries;
    StatusRecord* outGoingRequest;

    // For pending requests
    unsigned int pendingRequestNumEntries;
    unsigned int pendingRequestMaxEntries;
    PendingRequest* pendingRequest;

    // For Label Information Base
    unsigned int LIB_NumEntries;
    unsigned int LIB_MaxEntries;
    LIB* LIB_Entries;

    // For keeping track of Fec to Label mapping outgoing
    // this is called FEC to label mapping record
    unsigned int fecToLabelMappingRecNumEntries;
    unsigned int fecToLabelMappingRecMaxEntries;
    FecToLabelMappingRec* fecToLabelMappingRec;

    // For keeping track of Fec to Label mapping incoming
    unsigned int incomingFecToLabelMappingRecNumEntries;
    unsigned int incomingFecToLabelMappingRecMaxEntries;
    FecToLabelMappingRec* incomingFecToLabelMappingRec;

    //For keeping track of withdrawn labels and fec's
    unsigned int withdrawnLabelRecNumEntries;
    unsigned int withdrawnLabelRecMaxEntries;
    LabelWithdrawRec * withdrawnLabelRec;

    // query statistics to collected or not
    BOOL collectStat;

    // collect statistics
    MplsLdpStat ldpStat;
} MplsLdpApp;


// The structure below defines the optional parameters type
// that will be used by notification message ;
typedef struct
{
    unsigned char* extendedStatus;

    unsigned int sizeReturnedPDU;
    unsigned char* returnedPDU;

    unsigned int sizeReturnedMessage;
    unsigned char* returnedMessage;

    unsigned int otherTLVtype;
    unsigned char* otherdata;
} OptionalParameters;


// This Structure is needed for Scheduling Keep alive messages
// This structure is always encapsulated in info part of the message
// This may require further changes (if needed).
typedef struct
{
    LDP_Identifier_Type LSR_Id;
    int connectionId;
} KeepAliveMessageInfoStructure;


//-------------------------------------------------------------------------
//  FUNCTION     : MplsLdpNewFECRecognized()
//
//  PURPOSE      : Perform Label Request for a newly detected FEC.
//
//  PARAMETERS   : node - the node which has recognized the new fec.
//                 mpls - the mpls structure (in the mac layer).
//                 destAddr - destination address embedded in the fec.
//                 interfaceIndex - incoming interface index.
//                 nextHop - next hop peer
//
//  RETURN VALUE : none.
//
//  ASSUMPTIONS  : This function's parameter list must match
//                 typedef void (*MplsNewFECCallbackFunctionType)
//                 defined in "mpls.h"
//-------------------------------------------------------------------------
void
MplsLdpNewFECRecognized(
    Node* node,
    struct mpls_var_struct* mpls,
    NodeAddress destAddr,
    int interfaceIndex,
    NodeAddress nextHop);


//-------------------------------------------------------------------------
//  FUNCTION     : MplsLdpInit()
//
//  PURPOSE      : initializing MPLS-LDP protocol.
//
//  PARAMETERS   : node - the node which is initializing
//                 nodeInput - the input configuration file
//                 sourceAddr - address of the node which is initializing
//
//  RETURN VALUE : none
//
//  ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void
MplsLdpInit(
    Node* node,
    const NodeInput* nodeInput,
    NodeAddress sourceAddr);


//-------------------------------------------------------------------------
//  FUNCTION     : MplsLdpLayer()
//
//  PURPOSE      : handling MPLS-LPS's event types.
//
//  PARAMETERS   : node - the node which handling the event type
//                 msg  - the message received by the node.
//
//  RETURN VALUE : none
//
//  ASSUMPTIONS  : none
//-------------------------------------------------------------------------
 void
MplsLdpLayer(Node* node, Message* msg);


//-------------------------------------------------------------------------
//  FUNCTION     : MplsLdpFinalize()
//
//  PURPOSE      : finalizing and printing MPLS-LDP statistical results.
//
//  PARAMETERS   : node - the node which finalizing the statistical results
//                 AppInfo - pointer to the AppInfo structure.
//
//  RETURN VALUE : none
//
//  ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void
MplsLdpFinalize(Node* node, AppInfo* appInfo);


//-------------------------------------------------------------------------
// FUNCTION     : MplsLdpAppGetStateSpace()
//
//  PURPOSE      : returning the pointer to the structure MplsLdpApp if
//                 node is given as the argument to the function.
//
//  PARAMETERS   : node - the node which will return the "pointer
//                        to the structure MplsLdpApp"
//
//  RETURN VALUE : pointer to the structure MplsLdpApp if MPLS-LDP
//                 structure initialized, or NULL otherwise.
//
//  ASSUMPTIONS  : none
//-------------------------------------------------------------------------
MplsLdpApp*
MplsLdpAppGetStateSpace(Node* node);

//-------------------------------------------------------------------------
//  FUNCTION     : SearchEntriesInTheLib()
//
//  PURPOSE      : searching for a particular fec in the LIB (Label
//                 Information Base) and returning the outgoing
//                 label encoding for it.
//
//  PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                 fec - the fec to be searched for
//                 outLabel - the outgoing label returned.
//
//  RETURN VALUE : pointer to the row of a LIB table if fec exists in
//                 in the table, or NULL otherwise.
//
//  ASSUMPTIONS  : none
//-------------------------------------------------------------------------
LIB*
SearchEntriesInTheLib(
    MplsLdpApp* ldp,
    Mpls_FEC fec,
    unsigned int* outLabel);


//-------------------------------------------------------------------------
//  FUNCTION     : AddEntriesToLIB()
//
//  PURPOSE      : adding a row into the LIB table
//
//  PARAMETERS   : ldp - pointer to the MplsLdpApp structure.
//                 numSignificantBits - the of significant bits in the IP
//                                      address (prefix length)
//                 prefix - .
//                 nextHop - .
//                 label - .
//
//  RETURN VALUE : pointer to the row of a LIB table if fec exists in
//                 in the table, or NULL otherwise.
//
//  ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void
AddEntriesToLIB(
    MplsLdpApp* ldp,
    int numSignificantBits,
    NodeAddress prefix,
    NodeAddress nextHop,
    unsigned int label);

//-------------------------------------------------------------------------
//  FUNCTION     : IsIngress()
//
//  PURPOSE      : checks if an LSR is Ingress LSR or not for an
//                 Incoming packet
//                 Code used in IP+MPLS integration
//
//  PARAMETERS   : node - the node to check if it is ingress
//                 msg - the message received by the node.
//                 ldp - pointer to the MplsLdpApp structure.
//                 isEdgeRouter - is the node configured as Edge Router
//
//  RETURN VALUE : TRUE if the node is an Edge Route OR node is source
//                 AND next hop is in the helloadjacency of the node,
//                 FALSE otherwise
//
//  ASSUMPTIONS  : none
//-------------------------------------------------------------------------

BOOL IsIngress(
    Node* node,
    Message* msg,
    BOOL isEdgeRouter);

// Event that cause interface state change.
typedef enum
{
    LDP_InterfaceUp,
   /* LDP_WaitTimer,
    LDP_BackupSeen,
    LDP_NeighborChange,
    LDP_LoopInd,
    LDP_UnloopInd,*/
    LDP_InterfaceDown
} LDPInterfaceEvent;

//-------------------------------------------------------------------------
//  NAME         :LDPInterfaceStatusHandler()
//  PURPOSE      :Handle mac alert.
//  PARAMETERS   : node - the node on which fault has occured
//                 interfaceIndex - the index of interface
//                 at which fault has occured
//                 state - State of fault
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------

void LDPInterfaceStatusHandler(
    Node* node,
    int interfaceIndex,
    MacInterfaceState state);

//-------------------------------------------------------------------------
// NAME         :LDPHandleInterfaceEvent()
// PURPOSE      :Handle interface event and change interface state
//               accordingly
//  PARAMETERS   : node - the node on which fault has occured
//                 interfaceIndex - the index of interface
//                 at which fault has occured
//                 eventType - Interface Up / Down
// ASSUMPTION   :None
// RETURN VALUE :Null
//-------------------------------------------------------------------------

void LDPHandleInterfaceEvent(
    Node* node,
    int interfaceIndex,
    LDPInterfaceEvent eventType);
#endif
// MPLS_LDP_H

