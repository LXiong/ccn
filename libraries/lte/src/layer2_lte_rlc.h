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

#ifndef LAYER2_LTE_RLC_H
#define LAYER2_LTE_RLC_H

#include <node.h>

#include <deque>

#include "lte_common.h"


////////////////////////////////////////////////////////////////////////////
// const parameter
////////////////////////////////////////////////////////////////////////////

#define LTE_RLC_SN_MAX         (1023) // 10bit
#define LTE_RLC_RESET_TIMER_DELAY_MSEC (10)

// status PDU header
//
// D/C(1bit) + CPE(3bit) + ACK_SN(10bit) + E1(1bit)
#define LTE_RLC_AM_STATUS_PDU_FIXED_BIT_SIZE (15)
// NACK_SN(10bit) + E1(1bit) + E2(1bit)
#define LTE_RLC_AM_STATUS_PDU_EXTENSION_E1_E2_BIT_SIZE (12)
// SOstart(15bit) + SOend(15bit)
#define LTE_RLC_AM_STATUS_PDU_EXTENSION_SO_BIT_SIZE (30)

// PDU header fixed part
//
// D/C(1bit) + RF(1bit) + P(1bit) + FI(2bit) + E(1bit) + SN(10bit)
#define LTE_RLC_AM_DATA_PDU_FORMAT_FIXED_BIT_SIZE (16)
// PDU header fixed part size.(byte)
#define LTE_RLC_AM_DATA_PDU_FORMAT_FIXED_BYTE_SIZE \
            (LTE_RLC_AM_DATA_PDU_FORMAT_FIXED_BIT_SIZE / 8)

// PDU header extendion part
//
// E(1bit) + Li(11bit)
#define LTE_RLC_AM_DATA_PDU_FORMAT_EXTENSION_BIT_SIZE (12)
// E+Li minimum length(1extension) = 12bit = 1.5octet = 2Byte
#define LTE_RLC_AM_DATA_PDU_FORMAT_EXTENSION_MIN_BYTE_SIZE (2)
// E+Li padding = 4bit (If the number of Li is odd numbers)
#define LTE_RLC_AM_DATA_PDU_FORMAT_EXTENSION_PADDING_BIT_SIZE (4)

// PDU header resegment part
//
// LSF(1bit) + SO(15bit)
#define LTE_RLC_AM_DATA_PDU_RESEGMENT_FORMAT_EXTENSIO_BIT_SIZE (16)
// PDU header resegment part size.(byte)
#define LTE_RLC_AM_DATA_PDU_RESEGMENT_FORMAT_EXTENSIO_BYTE_SIZE \
            (LTE_RLC_AM_DATA_PDU_RESEGMENT_FORMAT_EXTENSIO_BIT_SIZE / 8)

// Minimum RLC-PDUsegment data size.(without header)
#define LTE_RLC_AM_DATA_PDU_MIN_BYTE (0)

// const parameter AM
#define LTE_RLC_AM_WINDOW_SIZE (512)
#define LTE_RLC_AM_SN_UPPER_BOUND (LTE_RLC_SN_MAX + 1)

// invalid
#define LTE_RLC_INVALID_BEARER_ID   (-1)
#define LTE_RLC_INVALID_SEQ_NUM (-1)
#define LTE_RLC_INVALID_RLC_SN (LTE_RLC_SN_MAX + 1)
#define LTE_RLC_INVALID_STATUS_PDU_FORMAT_NACK_SEGMENT_OFFSET (-1)

// parameter infinity
#define LTE_RLC_INFINITY_POLL_PDU   (-1)
#define LTE_RLC_INFINITY_POLL_BYTE  (-1)

// parameter strings
#define LTE_RLC_STRING_MAX_RETX_THRESHOLD \
        "RLC-LTE-MAX-RETX-THRESHOLD"
#define LTE_RLC_STRING_POLL_PDU          "RLC-LTE-POLL-PDU"
#define LTE_RLC_STRING_POLL_BYTE         "RLC-LTE-POLL-BYTE"
#define LTE_RLC_STRING_POLL_RETRANSMIT   "RLC-LTE-T-POLL-RETRANSMIT"
#define LTE_RLC_STRING_T_REORDERING      "RLC-LTE-T-REORDERING"
#define LTE_RLC_STRING_T_STATUS_PROHIBIT "RLC-LTE-T-STATUS-PROHIBIT"

// default parameter
#define LTE_RLC_DEFAULT_MAX_RETX_THRESHOLD (8)
#define LTE_RLC_DEFAULT_POLL_PDU           (16)
#define LTE_RLC_DEFAULT_POLL_BYTE          (250)
#define LTE_RLC_DEFAULT_POLL_RETRANSMIT    (100) //msec
#define LTE_RLC_DEFAULT_T_REORDERING       (100) //msec
#define LTE_RLC_DEFAULT_T_STATUS_PROHIBIT  (12) //msec

// parameter LIMIT
#define LTE_RLC_MIN_MAX_RETX_THRESHOLD (0)
#define LTE_RLC_MAX_MAX_RETX_THRESHOLD (INT_MAX)
#define LTE_RLC_MIN_POLL_PDU           (0)
#define LTE_RLC_MAX_POLL_PDU           (INT_MAX)
#define LTE_RLC_MIN_POLL_BYTE          (0)
#define LTE_RLC_MAX_POLL_BYTE          (INT_MAX)
#define LTE_RLC_MIN_POLL_RETRANSMIT    (1*MILLI_SECOND)
#define LTE_RLC_MAX_POLL_RETRANSMIT    (CLOCKTYPE_MAX)
#define LTE_RLC_MIN_T_REORDERING      (1*MILLI_SECOND)
#define LTE_RLC_MAX_T_REORDERING      (CLOCKTYPE_MAX)
#define LTE_RLC_MIN_T_STATUS_PROHIBIT  (1*MILLI_SECOND)
#define LTE_RLC_MAX_T_STATUS_PROHIBIT  (CLOCKTYPE_MAX)


// /**
// CONSTANT    :: LTE_RLC_DEF_MAXBUFSIZE
// DESCRIPTION :: Default value of the maximum buffer size
// **/
const UInt64 LTE_RLC_DEF_MAXBUFSIZE = 50000;


////////////////////////////////////////////////////////////////////////////
// typedef
////////////////////////////////////////////////////////////////////////////

// FUNCTION   :: LteRlcDuplicateMessageList
// LAYER      :: ANY
// PURPOSE    :: Duplicate a list of message
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msgList   : Message* : Pointer to a list of messages
// RETURN     :: Message* : The duplicated message list header
// **/
Message* LteRlcDuplicateMessageList(
    Node* node,
    Message* msgList);


////////////////////////////////////////////////////////////////////////////
// Enumlation
////////////////////////////////////////////////////////////////////////////
// /**
// ENUM::       LteRlcStatus
// DESCRIPTION::RLC Status
// **/
typedef enum
{
    LTE_RLC_POWER_OFF,
    LTE_RLC_POWER_ON
} LteRlcStatus;

// /**
// ENUM::       LteRlcEntityDirect
// DESCRIPTION::Traffic direction of a RLC entity
// **/
typedef enum
{
    LTE_RLC_ENTITY_TX,
    LTE_RLC_ENTITY_RX,
    LTE_RLC_ENTITY_BI
} LteRlcEntityDirect;

// /**
// ENUM:: LteRlcTimerType
// DESCRIPTION:: RLC timer Type
// **/
typedef enum
{
    LTE_RLC_TIMER_RST,
    LTE_RLC_TIMER_POLL_RETRANSEMIT,
    LTE_RLC_TIMER_REODERING,
    LTE_RLC_TIMER_STATUS_PROHIBIT,
}LteRlcTimerType;

// /**
// ENUM:: LteRlcEntityType
// DESCRIPTION:: RLC Entites
// **/
typedef enum
{
    LTE_RLC_ENTITY_INVALID,
    LTE_RLC_ENTITY_TM,
    LTE_RLC_ENTITY_UM,
    LTE_RLC_ENTITY_AM
} LteRlcEntityType;

// /**
// ENUM::LteRlcPduType
// DESCRIPTION:: RLC PDU states
//
typedef enum
{
    LTE_RLC_AM_PDU_TYPE_STATUS,           // Status PDU
    LTE_RLC_AM_PDU_TYPE_PDU,              // DATA without Segment
    LTE_RLC_AM_PDU_TYPE_PDU_SEGMENTATION, // DATA with Segment
    LTE_RLC_AM_PDU_TYPE_INVALID
} LteRlcPduType;

// /**
// ENUM::LteRlcPduState
// DESCRIPTION:: RLC PDU states
//
// **/
typedef enum
{
    LTE_RLC_AM_FRAMING_INFO_NO_SEGMENT    = 0x00, // 00
    LTE_RLC_AM_FRAMING_INFO_IN_FIRST_BYTE = 0x01, // 01
    LTE_RLC_AM_FRAMING_INFO_IN_END_BYTE   = 0x02, // 10
    LTE_RLC_AM_FRAMING_INFO_FRAGMENT      = 0x03  // 11
} LteRlcAmFramingInfo;

#define LTE_RLC_AM_FRAMING_INFO_CHK_FIRST_BYTE  0x02 //10(mask)
#define LTE_RLC_AM_FRAMING_INFO_CHK_END_BYTE    0x01 //01(mask)

// /**
// ENUM::LteRlcPduState
// DESCRIPTION:: RLC PDU states
//
// **/
typedef enum
{
    LTE_RLC_AM_CONTROL_PDU_TYPE_STATUS = 0x000,  // 00
    LTE_RLC_AM_CONTROL_PDU_TYPE_LEN              // reserved 001-111
} LteRlcAmControlPduType;

// /**
// ENUM::
// DESCRIPTION:: RLC PDU states
//
// **/
typedef enum
{
    LTE_RLC_AM_DATA_PDU_STATUS_WAIT,
    LTE_RLC_AM_DATA_PDU_STATUS_NACK,
    LTE_RLC_AM_DATA_PDU_STATUS_ACK
} LteRlcAmDataPduStatus;


////////////////////////////////////////////////////////////////////////////
// Structure
////////////////////////////////////////////////////////////////////////////


// /**
// STRUCT:: LteRlcStats
// DESCRIPTION:: RLC statistics
// **/
typedef struct
{
    UInt32 numSdusReceivedFromUpperLayer;
    UInt32 numSdusDiscardedByOverflow;
    UInt32 numSdusSentToUpperLayer;
    UInt32 numDataPdusSentToMacSublayer;
    UInt32 numDataPdusDiscardedByRetransmissionThreshold;
    UInt32 numDataPdusReceivedFromMacSublayer;
    UInt32 numDataPdusReceivedFromMacSublayerButDiscardedByReset;
    UInt32 numAmStatusPdusSendToMacSublayer;
    UInt32 numAmStatusPdusReceivedFromMacSublayer;
    UInt32 numAmStatusPdusReceivedFromMacSublayerButDiscardedByReset;
} LteRlcStats;

// /**
// STRUCT:: LteRlcTimerInfoHdr
// DESCRIPTION::
//      The first part of RLC timer info
// **/
typedef struct {
    LteRnti rnti;
    int bearerId;
    LteRlcEntityDirect direction;
    UInt32 infoLen;
} LteRlcTimerInfoHdr;

// /**
// STRUCT::     LteRlcFreeMessage
// DESCRIPTION::
//      Function object used to free Message pointer in STL container
// **/
class LteRlcFreeMessage
{
    Node* node;
public:
    explicit LteRlcFreeMessage(Node* ndPtr):node(ndPtr) { }
    void operator() (Message* msg)
    {
        while (msg)
        {
            Message* nextMsg = msg->next;
            MESSAGE_Free(node, msg);
            msg = nextMsg;
        }
    }
};

// /**
// STRUCT::     LteRlcFreeMessageData
// DESCRIPTION::
//      Function object used to delete sduSegment in the list
// **/
template < typename T >
class LteRlcFreeMessageData
{
    Node* node;
public:
    explicit LteRlcFreeMessageData(Node* ndPtr):node(ndPtr) { }
    void operator() (T messageData)
    {
        Message* msg = messageData.message;
        while (msg)
        {
            Message* nextMsg = msg->next;
            MESSAGE_Free(node, msg);
            msg = nextMsg;
        }
        //MEM_free(messageData);
    }
};

// /**
// STRUCT::     LteRlcAmPduFormatFixed
// DESCRIPTION:: RLC PDU header struct (nomal fixed part)
// **/
struct LteRlcAmPduFormatFixed
{
    // D/C(Data/Control)  TRUE:Data PDU  FALSE:Control PDU.
    BOOL dcPduFlag;

    // RF(resegmentation flag)   TRUE:PDU SEGMENT   FALSE:PDU.
    BOOL resegmentationFlag;

    // P(Polling Bit)  TRUE:Request status PDU  FALSE:NoRequest.
    BOOL pollingBit;

    // FI(Framing Info)
    LteRlcAmFramingInfo FramingInfo;

    // SN(Sequence Number)
    Int16 seqNum;

    // LTE RLC Header Size (LTE Lib Specific item)
    UInt32 headerSize;

    LteRlcAmPduFormatFixed()
    {
        dcPduFlag          = FALSE;
        resegmentationFlag = FALSE;
        pollingBit         = FALSE;
        FramingInfo        = LTE_RLC_AM_FRAMING_INFO_NO_SEGMENT;
        seqNum             = LTE_RLC_INVALID_SEQ_NUM;
        headerSize         = 0;
    }

    LteRlcAmPduFormatFixed(LteRlcPduType& pduType)
    {
        switch (pduType)
        {
            case LTE_RLC_AM_PDU_TYPE_PDU:
            {
                dcPduFlag          = TRUE;
                resegmentationFlag = FALSE;
                pollingBit         = FALSE;
                FramingInfo        = LTE_RLC_AM_FRAMING_INFO_NO_SEGMENT;
                seqNum             = LTE_RLC_INVALID_SEQ_NUM;
                headerSize      = LTE_RLC_AM_DATA_PDU_FORMAT_FIXED_BYTE_SIZE;
                break;
            }

            case LTE_RLC_AM_PDU_TYPE_PDU_SEGMENTATION:
            {
                dcPduFlag          = TRUE;
                resegmentationFlag = FALSE;
                pollingBit         = FALSE;
                FramingInfo        = LTE_RLC_AM_FRAMING_INFO_FRAGMENT;
                seqNum             = LTE_RLC_INVALID_SEQ_NUM;
                headerSize      = LTE_RLC_AM_DATA_PDU_FORMAT_FIXED_BYTE_SIZE;
                break;
            }
            default:
            {
                ERROR_Assert(FALSE, "LteRlcAmPduFormatFixed:"
                                    "pduType No Support");
            }
        }
    }
};

// /**
// STRUCT::     LteRlcAmPduFormatExtension
// DESCRIPTION:: RLC PDU header struct (extentional part)
// **/
struct LteRlcAmPduFormatExtension
{
    // number of LI(Length Indicator)
    size_t numLengthIndicator;
    // LI(Length Indicator)  RLC SDU length
    std::vector < UInt16 > lengthIndicator;

    LteRlcAmPduFormatExtension()
    {
        numLengthIndicator = 0;
    }
};

// /**
// STRUCT::     LteRlcAmPduFormatExtension
// DESCRIPTION:: RLC PDU header struct (lengthIndicator-info part)
//               for massage info-field.
// **/
struct LteRlcAmPduFormatExtensionMsgInfo
{
    // number of LI(Length Indicator)
    size_t numLengthIndicator;
    // LI(Length Indicator)  RLC SDU length
    UInt16 lengthIndicator;
};

// /**
// STRUCT::     LteRlcAmPduSegmentFormatFixed
// DESCRIPTION:: RLC PDU header struct (extentional fixed part)
// **/
struct LteRlcAmPduSegmentFormatFixed
{
    // LSF(Last Segment Flag)
    // TRUE :PDU segment at the end of the last RLC PDU Byte Byte and match.
    // FALSE:do not match.
    BOOL lastSegmentFlag;

    // SO(Segment Offset).
    UInt16 segmentOffset;

    LteRlcAmPduSegmentFormatFixed()
    {
        lastSegmentFlag = FALSE;
        segmentOffset   = 0;
    }

};

// /**
// STRUCT::     LteRlcStatusPduFormatFixed
// DESCRIPTION:: RLC status PDU header struct (fixed part)
// **/
struct LteRlcStatusPduFormatFixed
{
    // D/C(Data/Control)  TRUE:Data PDU  FALSE:Control PDU.
    BOOL dcPduFlag;

    // Control PDU type
    LteRlcAmControlPduType cpt;

    // Ack sequens number
    Int32 ackSn;

    LteRlcStatusPduFormatFixed()
    {
        dcPduFlag = FALSE;
        cpt = LTE_RLC_AM_CONTROL_PDU_TYPE_STATUS;
        ackSn = LTE_RLC_INVALID_SEQ_NUM;
    }
};

// /**
// STRUCT::     LteRlcStatusPduSoPair
// DESCRIPTION:: RLC status PDU header struct (nacked-info part)
// **/
struct LteRlcStatusPduSoPair
{
    // SOstart(Segment Offset Start)
    Int32 soStart;

    // SOend(Segment Offset End)
    Int32 soEnd;
};

// /**
// STRUCT::     LteRlcStatusPduFormatExtension
// DESCRIPTION:: RLC status PDU header struct (fixed part)
// **/
struct LteRlcStatusPduFormatExtension
{
    UInt16 nackSn;
    LteRlcStatusPduSoPair soOffset;
};

// /**
// STRUCT::     LteRlcStatusPduFormat
// DESCRIPTION:: RLC status PDU header struct
// **/
struct LteRlcStatusPduFormat
{
    // RLC status PDU header struct (fixed part)
    LteRlcStatusPduFormatFixed fixed;

    // Nack info
    //    first  : sequens numbers
    //    second : Segment Offset
    std::multimap < UInt16, LteRlcStatusPduSoPair > nackSoMap;
};

// /**
// STRUCT::     LteRlcStatusPduFormatExtensionInfo
// DESCRIPTION:: RLC status PDU header struct (nacked-info part)
//               for massage info-field.
// **/
struct LteRlcStatusPduFormatExtensionInfo
{
    // NACKed SO(Segment Offset) infomation-str size
    size_t nackInfoSize;
    // NACKed SO(Segment Offset) infomation-str
    LteRlcStatusPduFormatExtension nackInfo;
};

// /**
// STRUCT::     LteRlcHeader
// DESCRIPTION:: Rlc Header
// **/
struct LteRlcHeader
{
    LteRlcAmPduFormatFixed*                fixed;
    LteRlcAmPduSegmentFormatFixed*         segFixed;
    LteRlcAmPduFormatExtension*            extension;

    BOOL firstSdu;
    UInt16 firstSduLi;

    LteRlcHeader()
    {
        fixed = NULL;
        segFixed = NULL;
        extension = NULL;

        firstSdu = TRUE;
        firstSduLi = 0;
    }

    LteRlcHeader(LteRlcPduType pduType)
    {

        switch (pduType)
        {
        case LTE_RLC_AM_PDU_TYPE_PDU:
            {
                fixed = new LteRlcAmPduFormatFixed(pduType);
                segFixed = NULL;
                extension = NULL;

                firstSdu = TRUE;
                firstSduLi = 0;

                return;
            }
        case LTE_RLC_AM_PDU_TYPE_STATUS:
            {
                fixed = NULL;
                segFixed = NULL;
                extension = NULL;

                firstSdu = FALSE;
                firstSduLi = 0;

                return;
            }
        default:
            {
                // TODO
                ERROR_ReportError("LteRlcHeader(): entity type error.\n");
                return;
            }
        }
    }

    void LteRlcSetPduSeqNo(int sn)
    {
        if (fixed != NULL)
        {
            fixed->seqNum = (Int16)sn;
        }
    };

};

// /**
// STRUCT::     LteRlcAmTxBuffer
// DESCRIPTION:: RLC Sdu struct
// **/
struct LteRlcAmTxBuffer
{
    Message*    message;
    int         offset;

    LteRlcAmTxBuffer()
    {
        message = NULL;
        offset = 0;
    }

    void store(Node* node, int interfaceIndex, Message* msg)
    {
        message = msg;
    }

    void restore(Node* node,
                 int interfaceIndex,
                 int seqNum,
                 int size,
                 LteRlcHeader& rlcHeader,
                 Message* msg)
    {
        message = msg;
        rlcHeader.fixed->dcPduFlag = TRUE;
        rlcHeader.fixed->pollingBit = 0;
        rlcHeader.fixed->resegmentationFlag = FALSE;
        rlcHeader.fixed->seqNum = (Int16)seqNum;
    }

    int getSduSize()
    {
        if (message == NULL)
        {
            return 0;
        }
        else
        {
            return MESSAGE_ReturnPacketSize(message);
        }
    }
};

// /**
// STRUCT::     LteRlcAmReTxBuffer
// DESCRIPTION:: RLC Sdu LteRlcAmReTxBuffer
// **/
struct LteRlcAmReTxBuffer
{
    Message*    message;
    int         offset;
    UInt8       retxCount;

    LteRlcHeader rlcHeader;
    LteRlcAmDataPduStatus pduStatus;
    std::map < int, LteRlcStatusPduSoPair > nackField;

    LteRlcAmReTxBuffer()
    {
        message = NULL;
        retxCount = 0;
        offset = 0;
        pduStatus = LTE_RLC_AM_DATA_PDU_STATUS_WAIT;
    }

    int getPduSize(Node* node, int interfaceIndex);
    int getNackedSize(Node* node, int interfaceIndex);

    void store(Node* node,
               int interfaceIndex,
               LteRlcHeader& header,
               Message* msg)
    {
        rlcHeader = header;
        message = msg;
    }
};

// /**
// STRUCT::     LteRlcAmRcvPduSegmentData
// DESCRIPTION:: RLC-PDUsegment received data
// **/
struct LteRlcAmRcvPduSegmentData
{
    Message*    message;
    int         pduSize;    // Hdr+data
    int         pduDataSize;// data only
    BOOL        lastSegmentFlag;
};

// /**
// STRUCT::     LteRlcAmRcvPduSegmentData
// DESCRIPTION:: RLC-PDUsegment received data-part infomation
// **/
struct LteRlcAmRcvPduSegmentPart
{
    UInt16      rcvdHead;   // received data-part head position
    UInt16      rcvdTail;   // received data-part tail position
};

// /**
// STRUCT::     LteRlcAmRcvPduData
// DESCRIPTION:: RLC-PDU received data
// **/
struct LteRlcAmRcvPduData
{
    //LteRlcHeader    header;
    Message*        message;

    Int16           orgMsgSize;
    
    // flaged RLC-PDU receive finished. (positive ACK)
    BOOL        rlcPduRcvFinished;

    // FI(Framing Info)
    LteRlcAmFramingInfo FramingInfo;

    // LI(Length Indicator) list
    std::vector < UInt16 > lengthIndicator;

    // RLC-PDUsegment received part infomation
    //    first  : SO(Segment Offset)start
    //    second : RLC-PDUsegment pair(SOstart,SOend)
    std::map < UInt16, LteRlcAmRcvPduSegmentPart > rcvdPduSegmentInfo;
    UInt16  endPduPos;  // current end-pos.
    BOOL    gotLsf;     // received LastSegmentFlag

    UInt32  reAssembledPos;

    // received Nacked part info (Segment Offset).
    std::deque < LteRlcStatusPduSoPair > rcvNackedSegOffset;

    LteRlcAmRcvPduData()
    {
        message = NULL;
        orgMsgSize = 0;
        rlcPduRcvFinished = FALSE;
        FramingInfo = LTE_RLC_AM_FRAMING_INFO_NO_SEGMENT;
        lengthIndicator.clear();
        rcvdPduSegmentInfo.clear();
        endPduPos = 0;
        gotLsf = FALSE;
        rcvNackedSegOffset.clear();
        reAssembledPos = 0;
    }

};

// /**
// STRUCT::     LteRlcSduSegment
// DESCRIPTION::
//      The sdu segment structure for re-assembly process
// **/
struct LteRlcSduSegment
{
    Int16               seqNum;
    LteRlcAmFramingInfo FramingInfo;
    Message*            newMsg;

    LteRlcSduSegment()
    {
        newMsg = NULL;
        seqNum = LTE_RLC_INVALID_SEQ_NUM;
        FramingInfo = LTE_RLC_AM_FRAMING_INFO_IN_FIRST_BYTE;
    }
};

// /**
// STRUCT:: LteRlcEntity
// DESCRIPTION::RLC entity structure
// **/
struct LteRlcEntity
{
    Node*              node;
    int                bearerId;
    LteRnti            oppositeRnti;
    LteRlcEntityType   entityType;
    void*              entityData;


    LteRlcEntity()
    {
        node = NULL;
        oppositeRnti = LTE_INVALID_RNTI;
        bearerId = LTE_RLC_INVALID_BEARER_ID;
        entityType = LTE_RLC_ENTITY_INVALID;
        entityData = NULL;
    };

    void reset(Node* node);
    void store(Node* node, std::string& buffer);
    void restore(Node* node, const char* buffer, size_t bufSize);

    void setRlcEntityToXmEntity();
};

// /**
// STRUCT:: LteRlcAmEntity
// DESCRIPTION::RLC entity structure
// **/
struct LteRlcAmEntity
{
    LteRlcEntity*         entityVar;

    UInt64                maxTransBufSize;
    //UInt64                bufSize;
    UInt64                txBufSize;
    UInt64                retxBufSize;
    UInt64                retxBufNackSize;
    UInt64                statusBufSize;

    // The transmitting side of each AM RLC entity shall maintain
    // the following state variables:
    UInt32    ackSn;     // a) VT(A) . Acknowledgement state variable
    UInt32    sndWnd;    // b) VT(MS) . Maximum send state variable
    UInt32    sndNextSn; // c) VT(S) . Send state variable
    UInt32    pollSn;    // d) POLL_SN . Poll send state variable

    // The transmitting side of each AM RLC entity shall maintain
    // the following counters:
    UInt32    pduWithoutPoll;   //   a) PDU_WITHOUT_POLL . Counter
    UInt32    byteWithoutPoll;  //   b) BYTE_WITHOUT_POLL . Counter
    UInt32    retxCount;        //   c) RETX_COUNT . Counter

    // The receiving side of each AM RLC entity shall maintain
    // the following state variables:
    UInt32 rcvNextSn;     // a) VR(R) . Receive state variable
    UInt32 rcvWnd;   // b) VR(MR) . Maximum acceptable receive state variable
    Int32  tReorderingSn; // c) VR(X) . t-Reordering state variable
    UInt32 maxStatusSn; // d) VR(MS) . Maximum STATUS transmit state variable
    UInt32 rcvSn;         // e) VR(H) . Highest received state variable

    UInt32    oldRcvNextSn;  // for Reassemble SDU

    // Timer
    Message* pollRetransmitTimerMsg;
    Message* reoderingTimerMsg;
    Message* statusProhibitTimerMsg;
    Message* resetTimerMsg;

    BOOL exprioryTPollRetransmitFlg;        // Expriory-t-PollRetransmit
    BOOL waitExprioryTStatusProhibitFlg;    // Wait_Expriory_T_satus_prohibit

    BOOL resetFlg; // ON : Waiting Reset timer is expired.
    BOOL sendResetFlg; // ON : Send Reset Msg.

    // Transmission Buffer
    std::list < LteRlcAmTxBuffer > txBuffer;

    // Latest sending PDU
    LteRlcAmReTxBuffer latestPdu;

    // Retransmission Buffer
    std::list < LteRlcAmReTxBuffer > retxBuffer;

    // Status PDU Buffer
    std::list < LteRlcStatusPduFormat > statusBuffer;

    // Reception Buffer
    std::multimap < unsigned int, Message* > rcvRlcSduList;
    std::list < LteRlcSduSegment >   rcvRlcSduSegList;
    std::list < LteRlcSduSegment >   rcvRlcSduTempList;

    // RLC-PDU received buffer
    //    first  : RLC-PDU sequense numbser
    //    second : RLC-PDU received data
    std::map < unsigned int, LteRlcAmRcvPduData >    rcvPduBuffer;

    // unreceived RLC ACK map
    //  first:  RLC SN
    //  second: PDCP SN
    list<pair<UInt16, UInt16> > unreceivedRlcAckList;

    void reset(Node* node);

    // Enqueue to Transmission Buffer
    void store(Node* node, int interfaceIndex, Message* msg);

    // Dequeue
    void restore(
        Node* node,
        int interfaceIndex,
        int size,
        std::list < Message* > &txMsg);

    LteRlcAmEntity()
    {
        txBufSize       = 0;
        retxBufSize     = 0;
        retxBufNackSize = 0;
        statusBufSize   = 0;
        latestPdu.message = NULL;

        maxTransBufSize  = LTE_RLC_DEF_MAXBUFSIZE;
        entityVar        = NULL;
        // The transmitting side of each AM RLC entity shall maintain
        // the following state variables:
        ackSn            = 0;
        sndWnd           = LTE_RLC_AM_WINDOW_SIZE;
        sndNextSn        = 0;
        pollSn           = 0;

        // The transmitting side of each AM RLC entity shall maintain
        // the following counters:
        pduWithoutPoll   = 0;
        byteWithoutPoll  = 0;
        retxCount        = 0;

        // The receiving side of each AM RLC entity shall maintain
        // the following state variables:
        rcvNextSn        = 0;
        rcvWnd           = LTE_RLC_AM_WINDOW_SIZE;
        tReorderingSn    = LTE_RLC_INVALID_SEQ_NUM;
        maxStatusSn      = 0;
        rcvSn            = 0;

        oldRcvNextSn     = 0;

        // Timer
        pollRetransmitTimerMsg = NULL;
        reoderingTimerMsg = NULL;
        statusProhibitTimerMsg = NULL;
        resetTimerMsg = NULL;

        exprioryTPollRetransmitFlg = FALSE;
        waitExprioryTStatusProhibitFlg = FALSE;
        resetFlg = FALSE;
        sendResetFlg = FALSE;
    }

    int getBufferSize(Node* node,
                      int interfaceIndex);

    // test func
    int getTestTxBufferSize()
    {
        int size = 0;
        std::list < LteRlcAmTxBuffer > ::iterator it;
        for (it = txBuffer.begin(); it != txBuffer.end(); it++)
        {
            size += MESSAGE_ReturnPacketSize(it->message);
        }
        return size;
    }

    // test func
    int getTestRetxBufferSize()
    {
        int size = 0;
        std::list < LteRlcAmReTxBuffer > ::iterator it;
        for (it = retxBuffer.begin(); it != retxBuffer.end(); it++)
        {
            size += MESSAGE_ReturnPacketSize(it->message);
        }
        return size;
    }
    // test func
    int getTestNackedSize()
    {
        Node* node;
        int interfaceIndex;

        int size = 0;
        std::list < LteRlcAmReTxBuffer > ::iterator it;
        for (it = retxBuffer.begin(); it != retxBuffer.end(); it++)
        {
            size += it->getNackedSize(node,interfaceIndex);
        }
        return size;
    }
    // test func
    int getTestStatusSize();

    BOOL discardPdu(Node* node,
                    int iface,
                    std::list < LteRlcAmReTxBuffer > ::iterator& it,
                    std::list < LteRlcAmReTxBuffer > &buf,
                    std::list < Message* > &pdu,
                    int& retxPduByte);

    BOOL getRetxBuffer(Node* node,
                       int interfaceIndex,
                       int& restSize,
                       std::list < LteRlcAmReTxBuffer > &buf,
                       std::list < Message* > &pdu,
                       int& retxPduByte);

    void getStatusBuffer(Node* node,
                         int interfaceIndex,
                         int& restSize,
                         std::list < LteRlcStatusPduFormat > &buf,
                         std::list < Message* > &pdu,
                         int& statusPduByte);

    BOOL getTxBuffer(Node* node,
                     int interfaceIndex,
                     int& restSize,
                     std::list < LteRlcAmTxBuffer > &buf,
                     std::list < Message* > &pdu,
                     int& txPduByte);

    BOOL chkExpriory_t_PollRetransmit(void)
    {
        return exprioryTPollRetransmitFlg;
    }

    void resetExpriory_t_PollRetransmit(void)
    {
        exprioryTPollRetransmitFlg = FALSE;
    }

    void polling(Node* node,
                 int interfaceIndex,
                 LteRlcHeader& rlcHeader,
                 Message* newMsg);

    void polling(Node* node,
                 int interfaceIndex,
                 int& restSize,
                 BOOL makedPdu,
                 BOOL makedPduSegment,
                 std::list < Message* > &pdu,
                 int& pollPduByte);

    void concatenationMessage(Node* node,
                              int interfaceIndex,
                              Message*& prev,
                              Message* next);

    void concatenation(Node* node,
                       int interfaceIndex,
                       LteRlcHeader& rlcHeader,
                       Message*& prev,
                       Message* next);

    int getSndSn();

    BOOL txWindowCheck();

    BOOL divisionMessage(Node* node,
                         int interfaceIndex,
                         int offset,
                         Message* msg,
                         Message* fragHead,
                         Message* fragTail,
                         TraceProtocolType protocolType,
                         BOOL msgFreeFlag);

    void resetAll(BOOL withoutResetProcess = FALSE);
    UInt32 getNumAvailableRetxPdu(
        Node* node,
        int interfaceIndex,
        int size);

    void dump(Node* node, int interfaceIndex,  const char* procName);
};


// /**
// STRUCT::     LteRlcPackedPduInfo
// DESCRIPTION::
//      The information stored in the info field
//      of each PDU after packing
// **/
typedef struct
{
    unsigned int pduSize;
} LteRlcPackedPduInfo;

typedef std::list < LteRlcAmEntity* >  LteRlcEntityList;


// /**
// STRUCT:: LteRlcAmResetData
// DESCRIPTION:: Reset Information
//               If src's retx count is reached RetxThreshold,
//               src send this data to dst.
//               When dst receive this data, reset
// **/
typedef struct
{
    LteRnti src; // node reached retx threshold

    LteRnti dst; // node received this data
    int bearerId;
} LteRlcAmResetData;

// /**
// STRUCT:: Layer2LteRlcData
// DESCRIPTION:: RLC sublayer structure
// **/
typedef struct{
    LteRlcStatus status;
    LteRlcStats stats;
} LteRlcData;

// /**
// STRUCT:: LteRlcAmPdcpPduInfo
// DESCRIPTION:: add info when store RLC SDU in transmission buffer
// **/
typedef struct LteRlcAmPdcpPduInfo{
    UInt16 pdcpSn;

    LteRlcAmPdcpPduInfo(const UInt16 argPdcpSn)
        : pdcpSn(argPdcpSn)
    {
    }
} LteRlcAmPdcpPduInfo;


////////////////////////////////////////////////////////////////////////////
// Function for RLC Layer
////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////
// eNB/UE - API
////////////////////////////////////////////////////////////////////////////
void RlcLteNotifyPowerOn(Node* node, int interfaceIndex);
void RlcLteNotifyPowerOff(Node* node, int interfaceIndex);
void LteRlcEstablishment(Node* node,
                         int interfaceIndex,
                         const LteRnti& srcRnti,
                         const int bearerId);

LteRlcEntity* LteRlcGetRlcEntity(Node* node,
                                 int interfaceIndex,
                                 const LteRnti& dstRnti,
                                 const int bearerId);

void LteRlcInitRlcEntity(Node* node,
                         int interfaceIndex,
                         const LteRnti& rnti,
                         const LteRlcEntityType& type,
                         const LteRlcEntityDirect& direct,
                         LteRlcEntity* entity);

void LteRlcRelease(Node* node,
                   int interfaceIndex,
                   const LteRnti& srcRnti,
                   const int bearerId);
// /**
// FUNCTION::       LteRlcInit
// LAYER::          LTE Layer2 RLC
// PURPOSE::        RLC Initalization
//
// PARAMETERS::
//    + node:       Node* : pointer to the network node
//    + interfaceIndex: unsigned int : interdex of interface
//    + nodeInput:  const NodeInput* : Input from configuration file
// RETURN::         NULL
// **/
void LteRlcInit(Node* node,
                unsigned int interfaceIndex,
                const NodeInput* nodeInput);

// /**
// FUNCTION::       LteRlcFinalize
// LAYER::          LTE Layer2 RLC
// PURPOSE::        RLC finalization function
// PARAMETERS::
//    + node:       Node* : pointer to the network node
//    + interfaceIndex: unsigned int : interdex of interface
// RETURN::         NULL
// **/
void LteRlcFinalize(Node* node, unsigned int interfaceIndex);

// /**
// FUNCTION::       LteRlcProcessEvent
// LAYER::          LTE Layer2 RLC
// PURPOSE::        RLC event handling function
// PARAMETERS::
//    + node:       Node * : pointer to the network node
//    + interfaceIndex: unsigned int : interdex of interface
//    + message     Message* : Message to be handled
// RETURN::         NULL
// **/
void LteRlcProcessEvent(Node* node,
                  unsigned int interfaceIndex,
                  Message* message);

void LteRlcReceiveSduFromPdcp(
    Node* node,
    int interfaceIndex,
    const LteRnti& dstRnti,
    const int bearerId,
    Message* rlcSdu);

void LteRlcDeliverPduToMac(
    Node* node,
    int interfaceIndex,
    const LteRnti& dstRnti,
    const int bearerId,
    int size,
    std::list < Message* >* sduList);

UInt32 LteRlcSendableByteInQueue(
    Node* node,
    int interfaceIndex,
    const LteRnti& dstRnti,
    const int bearerId);

UInt32 LteRlcSendableByteInTxQueue(
    Node* node,
    int interfaceIndex,
    const LteRnti& dstRnti,
    const int bearerId);

void LteRlcReceivePduFromMac(
    Node* node,
    int interfaceIndex,
    const LteRnti& srcRnti,
    const int bearerId,
    std::list < Message* > &pduList);

int LteRlcCheckDeliverPduToMac(
    Node* node,
    int interfaceIndex,
    const LteRnti& dstRnti,
    const int bearerId,
    int size,
    std::list < Message* >* sduList);

UInt32 LteRlcGetQueueSizeInByte(
    Node* node,
    int interfaceIndex,
    const LteRnti& dstRnti,
    const int bearerId);

void LteRlcAmIndicateResetWindow(
    Node* node,
    int interfaceIndex,
    const LteRnti& oppositeRnti,
    const int bearerId);

void LteRlcAmSetResetData(
    Node* node,
    int interfaceIndex,
    LteRlcAmEntity* amEntity);

void LteRlcAmGetResetData(
    Node* node,
    int interfaceIndex,
    std::vector < LteRlcAmResetData > &vecAmResetData);

void LteRlcLogPrintWindow(
    Node* node,
    int interfaceIndex,
    LteRlcEntity* rlcEntity);



// /**
// FUNCTION:: LteRlcReEstablishment
// LAYER::    LTE Layer2 RLC
// PURPOSE::  send all reassembled RLC SDUs to PDCP
//
// PARAMETERS::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const LteRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// + targetRnti         : const LteRnti& : RNTI of target eNB
// RETURN::         void     : NULL
// **/
void LteRlcReEstablishment(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        const LteRnti& targetRnti);



// FUNCTION:: LteRlcDiscardRlcSduFromTransmissionBuffer
// LAYER::    LTE Layer2 RLC
// PURPOSE::  discard the indicated RLC SDU if no segment of the RLC SDU has
//            been mapped to a RLC data PDU yet.
// PARAMETERS::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const LteRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// + pdcpSn             : const UInt16&  : PDCP SN
// RETURN::         void     : NULL
// **/
void LteRlcDiscardRlcSduFromTransmissionBuffer(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        const UInt16& pdcpSn);
#endif //LAYER2_LTE_RLC_H

