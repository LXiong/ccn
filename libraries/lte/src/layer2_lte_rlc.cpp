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

#include "api.h"

#include "layer2_lte.h"
#include "layer2_lte_rlc.h"

#ifdef LTE_LIB_LOG
#include "log_lte.h"
#endif

#define LTE_LIB_RLC_ENTITY_VAR // Ticket #874

//#define UNIT_TEST
//#define LTE_RLC_BUFF_SIZE_TEST
//#define LTE_RLC_MSG_TEST
//#define LTE_RLC_MSG2LOG_TEST

#ifdef LTE_LIB_LOG

#define LTE_LIB_RLC_CHECK_EVENT (0)
#define DEBUG_RLC_LTE (0)

#include <sstream>
#include <iostream>

void MESSAGE_logMessage(Node* node,
                        int iface, Message* msg,
                        const char* catType,
                        const char* text)
{
    std::stringstream  ss;
    int i;
    int j;

    char timeBuffer[MAX_STRING_LENGTH];

#ifdef LTE_RLC_MSG2LOG_TEST
    ss.str("");
    ss << text;
    ss << " PrintMsg-general:,";
    ss << hex << showbase << msg;
    ss << ",next message:,";
    ss << hex << showbase << msg->next;
    ss << ",nodeId:,," << msg->nodeId;
    ss << ",layerType:," << msg->layerType;
    ss << ",protocolType:," << msg->protocolType;
    ss << ",instanceId:," << msg->instanceId;
    ss << ",eventType:," << msg->eventType;
    lte::LteLog::DebugFormat(
        node,
        iface,
        LTE_STRING_LAYER_TYPE_RLC,
        "%s,%s",
        catType,
        ss.str().c_str());
    ss.str("");
    ss << text;
    ss << " PrintMsg-error:,";
    ss << ",error:," << msg->error;
    lte::LteLog::DebugFormat(
        node,
        iface,
        LTE_STRING_LAYER_TYPE_RLC,
        "%s,%s",
        catType,
        ss.str().c_str());
    ss.str("");
    ss << text;
    ss << " PrintMsg-info:,";
    for (i = 0; i < msg->infoArray.size(); ++i)
    {
        ss.setf(std::ios::dec, std::ios::basefield);
        ss << ",infoArray[" << i << "].infoType:,";
        ss << dec << msg->infoArray[i].infoType;
        ss << ",infoArray[" << i << "].infoSize:,";
        ss << dec << msg->infoArray[i].infoSize;
        ss << ",infoArray[" << i << "].info:,";
        ss << hex << showbase << (int*)msg->infoArray[i].info;
    }
    lte::LteLog::DebugFormat(
        node,
        iface,
        LTE_STRING_LAYER_TYPE_RLC,
        "%s,%s",
        catType,
        ss.str().c_str());
    ss.str("");
    ss << text;
    ss << " PrintMsg-packet:,";
    ss << ",packetSize:,";
    ss << dec << msg->packetSize;
    ss << ",packet*:,";
    ss << hex << showbase << (int*)msg->packet;
    ss << ",payLoadSize:,";
    ss << dec << msg->payloadSize;
    ss << ",payload*:,";
    ss << hex << showbase << (int*)msg->payLoad;
    ss << ",virtualPayLoadSize:,";
    ss << dec << MESSAGE_ReturnVirtualPacket(msg);
    ctoa(msg->packetCreationTime, timeBuffer);
    ss << ",packetCreationTime:," << timeBuffer;
    ss << ",cancelled:,";
    ss << boolalpha << msg->cancelled;
    ss << ",originatingNodeId:," << msg->originatingNodeId;
    ss << ",sequenceNumber:," << msg->sequenceNumber;
    ss << ",originatingProtocol:," << msg->originatingProtocol;
    ss << ",numberOfHeaders:," << msg->numberOfHeaders;
    for (i = 0; i < MAX_HEADERS; i++)
        ss << ",headerProtocols[" << i <<"]:," << msg->headerProtocols[i];
    for (i = 0; i < MAX_HEADERS; i++)
        ss << ",headerSizes["<< i <<"]:," << msg->headerSizes[i];
    lte::LteLog::DebugFormat(
        node,
        iface,
        LTE_STRING_LAYER_TYPE_RLC,
        "%s,%s",
        catType,
        ss.str().c_str());
#else // LTE_RLC_MSG2LOG_TEST
    ss.str("");
    //ss.setf(std::ios::hex, std::ios::basefield);
    //ss.setf(std::ios::showbase);
    //ss << "Printing message:\t" << msg       << std::endl;
    //ss << "\tnext message:\t"   << msg->next << std::endl;
    ss << text;
    ss << "Printing message:\t";
    ss << hex << showbase << msg << std::endl;
    ss << "\tnext message:\t";
    ss << hex << showbase << msg->next << std::endl;
    ss << "\tnodeId:\t\t"     << msg->nodeId       << std::endl;
    ss << "\tlayerType:\t"    << msg->layerType    << std::endl;
    ss << "\tprotocolType:\t" << msg->protocolType << std::endl;
    ss << "\tinstanceId:\t"   << msg->instanceId   << std::endl;
    ss << "\teventType:\t"    << msg->eventType    << std::endl;
    printf("%s",ss.str().c_str());
    ss.str("");
    ss << "\terror:\t\t" << msg->error << std::endl;
    printf("%s",ss.str().c_str());
    ss.str("");
    for (i = 0; i < msg->infoArray.size(); ++i)
    {
        ss.setf(std::ios::dec, std::ios::basefield);
        ss << "\tinfoArray["<< i <<"].infoType:\t";
        ss << dec << msg->infoArray[i].infoType << std::endl;
        ss << "\tinfoArray["<< i <<"].infoSize:\t";
        ss << dec << msg->infoArray[i].infoSize << std::endl;
        ss << "\tinfoArray["<< i <<"].info:\t";
        ss << hex << showbase << (int*)msg->infoArray[i].info << std::endl;
        //ss << "\tinfoArray["<< i <<"].info:\t";
        //for (j = 0; j < msg->infoArray[i].infoSize; j++)
        //{
        //    ss << hex << msg->infoArray[i].info[j];
        //}
    }
    printf("%s",ss.str().c_str());
    ss.str("");
    ss << "\tpacketSize:\t";
    ss << dec << msg->packetSize << std::endl;
    ss << "\tpacket*:\t";
    ss << hex << showbase << (int*)msg->packet << std::endl;
    ss << "\tpayLoadSize:\t";
    ss << dec << msg->payloadSize << std::endl;
    ss << "\tpayload*:\t";
    ss << hex << showbase << (int*)msg->payload << std::endl;
    //{
    //    ss << ("packet: \t\t");
    //    for (i = 0; i < msg->packetSize; i++)
    //    {
    //        ss << dec << (int) msg->packet[i];
    //    }
    //    ss <<("\n");
    //}
    ss << "\tvirtualPayLoadSize:\t";
    ss << dec << MESSAGE_ReturnVirtualPacketSize(msg) << std::endl;
    printf("%s",ss.str().c_str());
    ss.str("");
    ctoa(msg->packetCreationTime, timeBuffer);
    ss << "\tpacketCreationTime:\t" << timeBuffer << std::endl;
    ss << "\tcancelled:\t";
    ss << boolalpha << msg->cancelled << std::endl;
    ss << "\toriginatingNodeId:\t" << msg->originatingNodeId << std::endl;
    ss << "\tsequenceNumber:\t" << msg->sequenceNumber << std::endl;
    ss << "\toriginatingProtocol:\t" << msg->originatingProtocol;
    ss << std::endl;
    printf("%s",ss.str().c_str());
    ss.str("");
    ss << "\tnumberOfHeaders:\t" << msg->numberOfHeaders << std::endl;
    for (i = 0; i < MAX_HEADERS; i++)
    {
        ss << "\theaderProtocols[" << i;
        ss << "]:\t" << msg->headerProtocols[i] << std::endl;
    }
    for (i = 0; i < MAX_HEADERS; i++)
    {
        ss << "\theaderSizes[" << i;
        ss << "]:\t" << msg->headerSizes[i] << std::endl;
    }
    printf("%s",ss.str().c_str());
    printf("\n");
#endif // LTE_RLC_MSG2LOG_TEST
}

#endif // LTE_LIB_LOG


///////////////////////////////////////////////////////////////////////////
// LteRlcHeader utility functions (proto type)
///////////////////////////////////////////////////////////////////////////

// set header to the message's Info-field.

void LteRlcSetMessageRlcHeaderInfo(Node* node,
                                   int iface,
                                   LteRlcHeader& rlcHeader,
                                   Message* msg);

void LteRlcSetMessageRlcHeaderFixedInfo(Node* node,
                                        int iface,
                                        LteRlcHeader& rlcHeader,
                                        Message* msg);

void LteRlcSetMessageRlcHeaderSegmentFormatFixedInfo(Node* node,
                                                     int iface,
                                                     LteRlcHeader& rlcHeader,
                                                     Message* msg);

void LteRlcSetMessageRlcHeaderExtensionInfo(Node* node,
                                            int iface,
                                            LteRlcHeader& rlcHeader,
                                            Message* msg);

void LteRlcSetMessageStatusPduFormatFixedInfo(
    Node* node,
    int iface,
    LteRlcStatusPduFormatFixed& fixed,
    Message* msg);

void LteRlcSetMessageStatusPduFormatExtensionInfo(
    Node* node,
    int iface,
    std::multimap < UInt16, LteRlcStatusPduSoPair > &nackSoMap,
    Message* msg);

void LteRlcSetMessageStatusPduFormatInfo(Node* node,
                                         int iface,
                                         Message* msg,
                                         LteRlcStatusPduFormat& status);

// get header from message's Info-field.

void LteRlcGetMessageRlcHeaderFixedInfo(Node* node,
                                        int iface,
                                        LteRlcHeader& rlcHeader,
                                        Message* msg);

void LteRlcGetMessageRlcHeaderSegmentFormatFixedInfo(
                                              Node* node,
                                              int iface,
                                              LteRlcHeader& rlcHeader,
                                              Message* msg);

void LteRlcGetMessageRlcHeaderExtensionInfo(Node* node,
                                            int iface,
                                            LteRlcHeader& rlcHeader,
                                            Message* msg);

// get PDU information

int LteRlcGetAmPduSize(Node* node,
                       int iface,
                       LteRlcHeader& rlcHeader,
                       Message* msg);

int LteRlcGetAmPduHeaderSize(Node* node,
                             int iface,
                             LteRlcHeader& rlcHeader);

int LteRlcGetAmPduFormatSize(Node* node,
                             int iface,
                             LteRlcHeader& rlcHeader);

int LteRlcGetAmPduFormatFixedSize(Node* node,
                                  int iface,
                                  LteRlcHeader& rlcHeader);

int LteRlcGetAmPduLiAndESize(Node* node,
                             int iface,
                             LteRlcHeader& rlcHeader,
                             int size);

int LteRlcGetAmPduFormatExtensionSize(
                             Node* node,
                             int iface,
                             LteRlcHeader& rlcHeader);

int LteRlcGetAmPduSegmentFormatSize(Node* node,
                                    int iface,
                                    LteRlcHeader& rlcHeader);

int LteRlcGetAmPduSegmentFormatFixedSize(Node* node,
                                         int iface,
                                         LteRlcHeader& rlcHeader);

int LteRlcGetAmPduSegmentFormatExtensionSize(
                                             Node* node,
                                             int iface,
                                             LteRlcHeader& rlcHeader);

int LteRlcGetAmPduDataSize(Node* node,
                           int iface,
                           LteRlcHeader& rlcHeader,
                           Message* msg);

int LteRlcGetAmPduListDataSize(Node* node,
                               int iface,
                               LteRlcHeader& rlcHeader,
                               Message* msgList);

int LteRlcGetExpectedAmPduSize(Node* node,
                               int iface,
                               LteRlcHeader& rlcHeader,
                               BOOL reSegment,
                               Message* msgList,
                               //Message* addMsg);
                               LteRlcAmTxBuffer& txBuffData);

int LteRlcGetExpectedAmPduHeaderSize(Node* node,
                                     int iface,
                                     LteRlcHeader& rlcHeader,
                                     BOOL reSegment);

int LteRlcGetExpectedAmPduDataSize(Node* node,
                                   int iface,
                                   LteRlcHeader& rlcHeader,
                                   Message* msgList,
                                   Message* addMsg);

void LteRlcAllocAmPduLi(Node* node,
                        int iface,
                        LteRlcHeader& rlcHeader,
                        UInt16 size);

void LteRlcAddAmPduLi(Node* node,
                      int iface,
                      LteRlcHeader& rlcHeader,
                      Message* msg);



// /**
// FUNCTION   :: LteRlcAmNotifyPdCpOfPdcpStatusReport
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: if positive acknowledgements have been received for all AMD
//               PDUs associated with a transmitted RLC SDU, send an
//               indication to the upper layers of successful delivery of
//               the RLC SDU.
// PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const LteRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// + rlcAmEntity        : LteRlcAmEntity : The RLC AM entity
// + receivedRlcAckSn   : const Int32    : SN of received RLC ACK
// RETURN     :: void     : NULL
// **/
static
void LteRlcAmNotifyPdCpOfPdcpStatusReport(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        LteRlcAmEntity* rlcAmEntity,
        const Int32 receivedRlcAckSn);



// /**
// FUNCTION   :: LteRlcDelAmPduHdrObj
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: delete the data-structure in RLC header
// PARAMETERS ::
//  + rlcHeader : LteRlcHeader& : RLC header
// RETURN     :: void     : NULL
// **/
void LteRlcDelAmPduHdrObj(LteRlcHeader& rlcHeader)
{
    if (rlcHeader.fixed != NULL)
    {
        delete rlcHeader.fixed;
        rlcHeader.fixed = NULL;
    }

    if (rlcHeader.segFixed != NULL)
    {
        delete rlcHeader.segFixed;
        rlcHeader.segFixed = NULL;
    }

    if (rlcHeader.extension != NULL)
    {
        delete rlcHeader.extension;
        rlcHeader.extension = NULL;
    }
}


// /**
// FUNCTION   :: LteRlcCpyAmPduHdrObj
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: copy the data-structure in RLC header
// PARAMETERS ::
//  + obj       : LteRlcHeader& : RLC header(src)
//  + rlcHeader : LteRlcHeader& : RLC header(dst)
// RETURN     :: void     : NULL
// **/
void LteRlcCpyAmPduHdrObj(const LteRlcHeader& obj, LteRlcHeader& rlcHeader)
{
    if (obj.fixed != NULL)
    {
        rlcHeader.fixed = new LteRlcAmPduFormatFixed();
        *rlcHeader.fixed = *obj.fixed;
    }

    if (obj.segFixed != NULL)
    {
        rlcHeader.segFixed = new LteRlcAmPduSegmentFormatFixed();
        *rlcHeader.segFixed = *obj.segFixed;
    }

    if (obj.extension != NULL)
    {
        rlcHeader.extension = new LteRlcAmPduFormatExtension();
        *rlcHeader.extension = *obj.extension;
    }
    rlcHeader.firstSdu = obj.firstSdu;
    rlcHeader.firstSduLi = obj.firstSduLi;
}


///////////////////////////////////////////////////////////////////////////
// Utility functions for message Info-field operations
///////////////////////////////////////////////////////////////////////////


// /**
// FUNCTION   :: LteRlcAmGetDCBit
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: get D/Cbit data from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
BOOL LteRlcAmGetDCBit(Message* msg)
{
    BOOL   ret = FALSE;

    LteRlcAmPduFormatFixed* info =
        (LteRlcAmPduFormatFixed*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduFormatFixed);

    if (info != NULL)
    {
        ret = info->dcPduFlag;
    }
    else
    {
        // status pdu
    }

    return ret;
}

// /**
// FUNCTION   :: LteRlcAmGetRFBit
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: get RFbit data from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
BOOL LteRlcAmGetRFBit(Message* msg)
{
    BOOL   ret = FALSE;

    LteRlcAmPduFormatFixed* info =
        (LteRlcAmPduFormatFixed*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduFormatFixed);

    if (info != NULL)
    {
        ret = info->resegmentationFlag;
    }
    else
    {
        ERROR_Assert(FALSE, "LteRlcAmGetRFBit: no fixed header field!");
    }

    return ret;
}

// /**
// FUNCTION   :: LteRlcAmGetPollBit
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: get Poll bit data from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
BOOL LteRlcAmGetPollBit(Message* msg)
{
    BOOL   ret = FALSE;

    LteRlcAmPduFormatFixed* info =
        (LteRlcAmPduFormatFixed*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduFormatFixed);

    if (info != NULL)
    {
        ret = info->pollingBit;
    }
    else
    {
        ERROR_Assert(FALSE, "LteRlcAmGetPollBit: no fixed header field!");
    }

    return ret;
}

// /**
// FUNCTION   :: LteRlcAmGetFramingInfo
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: get FI data from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
LteRlcAmFramingInfo LteRlcAmGetFramingInfo(Message* msg)
{
    LteRlcAmFramingInfo   ret = LTE_RLC_AM_FRAMING_INFO_NO_SEGMENT;

    LteRlcAmPduFormatFixed* info =
        (LteRlcAmPduFormatFixed*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduFormatFixed);

    if (info != NULL)
    {
        ret = info->FramingInfo;
    }
    else
    {
        ERROR_Assert(FALSE,
                    "LteRlcAmGetFramingInfo: no fixed header field!");
    }

    return ret;
}

// /**
// FUNCTION   :: LteRlcAmGetLIlist
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: get LI-list data from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
void LteRlcAmGetLIlist(Message* msg, std::vector < UInt16 >* liList)
{
    LteRlcAmPduFormatExtensionMsgInfo* info =
        (LteRlcAmPduFormatExtensionMsgInfo*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduFormatExtension);

    if ((info != NULL) && (info->numLengthIndicator > 0))
    {
        UInt16 tmpLiData;
        for (size_t i = 0; i < info->numLengthIndicator; i++)
        {
            memcpy(&tmpLiData,
                   &(info->lengthIndicator) + i,
                   sizeof(UInt16));
            liList->push_back(tmpLiData);
        }
    }
    else
    {
        liList->clear();
    }
}

// /**
// FUNCTION   :: LteRlcAmGetLastSegflag
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: get Last segment flag from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
BOOL LteRlcAmGetLastSegflag(Message* msg)
{
    BOOL   ret = 0;

    LteRlcAmPduSegmentFormatFixed* info =
        (LteRlcAmPduSegmentFormatFixed*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduSegmentFormatFixed);

    if (info != NULL)
    {
        ret = info->lastSegmentFlag;
    }
    else
    {
        // do nothing
    }
    return ret;
}

// /**
// FUNCTION   :: LteRlcAmGetSegOffset
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: get segment offset from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
UInt16 LteRlcAmGetSegOffset(Message* msg)
{
    UInt16   ret = 0;

    LteRlcAmPduSegmentFormatFixed* info =
        (LteRlcAmPduSegmentFormatFixed*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduSegmentFormatFixed);

    if (info != NULL)
    {
        ret = info->segmentOffset;
    }
    else
    {
        // do nothing
    }

    return ret;
}

// /**
// FUNCTION   :: LteRlcAmGetHeaderSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: get RLC header size from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
UInt16 LteRlcAmGetHeaderSize(Message* msg)
{
    UInt16   ret = 0;

    LteRlcAmPduFormatFixed* info =
        (LteRlcAmPduFormatFixed*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduFormatFixed);

    if (info != NULL)
    {
        ret = (UInt16)info->headerSize;
    }
    else
    {
        ERROR_Assert(FALSE, "LteRlcAmGetHeaderSize: no fixed header field!");
    }

    return ret;
}

// /**
// FUNCTION   :: LteRlcAmGetCPT
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: get CPT from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
UInt8 LteRlcAmGetCPT(Message* msg)
{
    UInt8   ret = 0;

    LteRlcStatusPduFormatFixed* info =
        (LteRlcStatusPduFormatFixed*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LteRlcAmStatusPduPduFormatFixed);

    if (info != NULL)
    {
        ret = (UInt8)info->cpt;
    }
    else
    {
        ERROR_Assert(FALSE, "LteRlcAmGetCPT: no fixed header field!");
    }

    return ret;
}

// /**
// FUNCTION   :: LteRlcAmGetAckSn
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: get ACK-SN from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
// RETURN     :: BOOL     : NULL
// **/
Int16 LteRlcAmGetAckSn(Message* msg)
{
    Int16   ret = LTE_RLC_INVALID_SEQ_NUM;

    LteRlcStatusPduFormatFixed* info =
        (LteRlcStatusPduFormatFixed*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LteRlcAmStatusPduPduFormatFixed);

    if (info != NULL)
    {
        ret = (Int16)info->ackSn;
    }
    else
    {
        ERROR_Assert(FALSE, "LteRlcAmGetAckSn: no fixed header field!");
    }

    return ret;
}

// /**
// FUNCTION   :: LteRlcAmGetNacklist
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: get NACK list from the info-feild of message.
// PARAMETERS ::
//  + msg       : Message* : message
//  + nackSoMap : std::multimap< UInt16, LteRlcStatusPduSoPair >* : NACK-list
// RETURN     :: BOOL     : NULL
// **/
void LteRlcAmGetNacklist(Message* msg,
     std::multimap < UInt16, LteRlcStatusPduSoPair >* nackSoMap)
{
    LteRlcStatusPduFormatExtensionInfo* info =
        (LteRlcStatusPduFormatExtensionInfo*)MESSAGE_ReturnInfo(
        msg,
        (unsigned short)INFO_TYPE_LteRlcAmStatusPduPduFormatExtension);

    if ((info != NULL) && (info->nackInfoSize > 0))
    {
        LteRlcStatusPduFormatExtension nackInfoWork;
        for (size_t i = 0; i < info->nackInfoSize; i++)
        {
            memcpy(&nackInfoWork,
                   &(info->nackInfo) + i,
                   sizeof(LteRlcStatusPduFormatExtension));
            (*nackSoMap).insert(
                pair < UInt16, LteRlcStatusPduSoPair >
                (nackInfoWork.nackSn, nackInfoWork.soOffset));
        }
    }
    else
    {
        nackSoMap->clear();
    }
}

///////////////////////////////////////////////////////////////////////////
// Utility functions
///////////////////////////////////////////////////////////////////////////

// /**
// FUNCTION   :: LteRlcSerializeMessage
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Dump a single message into a buffer so that the orignal
//               message can be recovered from the buffer
// PARAMETERS ::
//  + node      : Node*    : Pointer to node.
//  + msg       : Message* : Pointer to a messages
//  + buffer    : string&  : The string buffer the message will be
//                           serialzed into (append to the end)
// RETURN     :: void     : NULL
// **/
void LteRlcSerializeMessage(Node* node,
                          Message* msg,
                          std::string& buffer)
{
    MESSAGE_Serialize(node, msg, buffer);
}

// FUNCTION   :: LteRlcUnSerializeMessage
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: recover the orignal message from the buffer
// PARAMETERS ::
//  + node      : Node*    : Pointer to node.
//  + buffer    : const char* : The string buffer containing the message
//                           was serialzed into
//  + bufIndex  : int&  : the start position in the buffer pointing
//                           to the message updated to the end of
//                           the message after the unserialization.
// RETURN     :: Message* : Message pointer to be recovered
// **/
Message* LteRlcUnSerializeMessage(Node* node,
                                const char* buffer,
                                int& bufIndex)
{
    return MESSAGE_Unserialize(node->partitionData, buffer, bufIndex);
}

// FUNCTION   :: LteRlcSerializeMessageList
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Dump a list of message into a buffer so that the orignal
//               messages can be recovered from the buffer
// PARAMETERS ::
//  + node      : Node*    : Pointer to node.
//  + msgList   : Message* : Pointer to a message list
//  + buffer    : string&  : The string buffer the messages will be
//                           serialzed into (append to the end)
// RETURN     :: int      : number of messages in the list
// **/
int LteRlcSerializeMessageList(Node* node,
                             Message* msgList,
                             std::string& buffer)
{
    return MESSAGE_SerializeMsgList(node, msgList, buffer);
}

// FUNCTION   :: LteRlcUnSerializeMessageList
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: recover the orignal message from the buffer
// PARAMETERS ::
//  + node      : Node*    : Pointer to node.
//  + buffer    : const char* : The string buffer containing the message
//                              list serialzed into
//  + bufIndex  : int&  : the start position in the buffer pointing to
//                        the message list
//                        updated to the end of the message list after the
//                        unserialization.
//  + numMsgs   : int   : Number of messages in the list
// RETURN     :: Message* : Pointer to the message list to be recovered
// **/
Message* LteRlcUnSerializeMessageList(Node* node,
                                    const char* buffer,
                                    int& bufIndex,
                                    int numMsgs)
{
    return MESSAGE_UnserializeMsgList(node->partitionData, buffer, bufIndex, numMsgs);
}

// FUNCTION   :: LteRlcPackMessage
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Pack a list of messages to be one message structure
// PARAMETERS ::
//  + node         : Node*    : Pointer to node.
//  + msgList      : list<Message*> : a list of messages
//  + origProtocol : TraceProtocolType : Protocol allocating this packet
// RETURN     :: Message* : The super msg contains a list of msgs as payload
// **/
Message* LteRlcPackMessage(Node* node,
                         std::list < Message* > &msgList,
                         TraceProtocolType origProtocol)
{
    std::string buffer;
    buffer.reserve(5000);

    if (msgList.empty())
    {
        return NULL;
    }

    std::list < Message* > ::iterator itPos;
    for (itPos = msgList.begin(); itPos != msgList.end(); ++itPos)
    {
        Message* nextMsg = *itPos;

        LteRlcSerializeMessage(node, nextMsg, buffer);

        // free the orignal message
        MESSAGE_Free(node, nextMsg);
    }
    msgList.clear();

    Message* newMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(newMsg != NULL, "RLC LTE: Out of memory!");
    MESSAGE_PacketAlloc(node, newMsg, buffer.size(), origProtocol);
    memcpy(MESSAGE_ReturnPacket(newMsg),
           buffer.data(),
           buffer.size());
    return newMsg;
}

// /**
// FUNCTION   :: LteRlcUnpackMessage
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Unpack a message to the original list of messages
// PARAMETERS ::
//  + node      : Node*    : Pointer to node.
//  + msg      : Message* : Pointer to the supper msg containing list of msgs
//  + copyInfo  : BOOL     : Whether copy info from old msg to first msg
//  + freeOld   : BOOL     : Whether the original message should be freed
//  + msgList   : list<Message*> : an empty stl list to be used to
//                contain unpacked messages
// RETURN     :: NULL
// **/
void LteRlcUnpackMessage(Node* node,
                       Message* msg,
                       BOOL copyInfo,
                       BOOL freeOld,
                       std::list < Message* > &msgList)
{
    msgList.clear();

    Message* firstMsg = NULL;

    Message* newMsg;

    char* payload;
    int payloadSize;
    int msgSize;

    payload = MESSAGE_ReturnPacket(msg);
    payloadSize = MESSAGE_ReturnPacketSize(msg);
    msgSize = 0;

    while (msgSize < payloadSize)
    {
        newMsg = LteRlcUnSerializeMessage(node,
                                        payload,
                                        msgSize);

        // add new msg to msgList
        if (firstMsg == NULL)
        {
            firstMsg = newMsg;
        }
        msgList.push_back(newMsg);
    }

    if (copyInfo)
    {
        // copy over the info field
        MESSAGE_InfoAlloc(node, firstMsg, MESSAGE_ReturnInfoSize(msg));
        memcpy(MESSAGE_ReturnInfo(firstMsg),
               MESSAGE_ReturnInfo(msg),
               MESSAGE_ReturnInfoSize(msg));
    }

    // free original message
    if (freeOld)
    {
        MESSAGE_Free(node, msg);
    }
}

// FUNCTION   :: LteRlcDuplicateMessageList
// LAYER      :: ANY
// PURPOSE    :: Duplicate a list of message
// PARAMETERS ::
//  + node    : Node*    : Pointer to node.
//  + msgList : Message* : Pointer to a list of messages
// RETURN     :: Message* : The duplicated message list header
// **/
Message* LteRlcDuplicateMessageList(Node* node, Message* msgList)
{
    Message* dupMsgHead = NULL;
    Message* nextMsg = msgList;
    Message* prevDupMsg = NULL;
    while (nextMsg)
    {
        Message* dupMsg = MESSAGE_Duplicate(node, nextMsg);
        if (!dupMsgHead)
        {
            dupMsgHead = dupMsg;
        }
        else
        {
            prevDupMsg->next = dupMsg;
        }
        prevDupMsg = dupMsg;
        nextMsg = nextMsg->next;
    }
    return dupMsgHead;
}


// FUNCTION   :: LteRlcSduListToPdu
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Dump a list of message into a new message's info-field.
//               (Make RLC-PDU from RLC-SDU segment list.)
// PARAMETERS ::
//  + node : Node*   : Pointer to node.
//  + msg : Message* : Pointer to a RLC-SDU(segment) message list
// RETURN     :: Message* : RLC-PDU
// **/
Message* LteRlcSduListToPdu(Node* node, Message* msgList)
{
    Message* nextMsg = msgList;
    Message* lastMsg;
    Message* newMsg;

    std::string buffer;
    int numMsgs = 0;
    int actualPktSize = 0;
    while (nextMsg != NULL)
    {
        actualPktSize += MESSAGE_ReturnPacketSize(nextMsg);

        LteRlcSerializeMessage(node, nextMsg, buffer);

        lastMsg = nextMsg;
        nextMsg = nextMsg->next;

        MESSAGE_Free(node, lastMsg);
        numMsgs++;
    }
    newMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(newMsg != NULL, "RLC LTE: Out of memory!");
    MESSAGE_PacketAlloc(node, newMsg, 0, TRACE_RLC_LTE);
    int tmpSize = MESSAGE_ReturnVirtualPacketSize(newMsg);
    if (tmpSize > 0)
    {
        MESSAGE_RemoveVirtualPayload(node, newMsg, tmpSize);
    }
    MESSAGE_AddVirtualPayload(node, newMsg, actualPktSize);
    char* info = MESSAGE_AddInfo(node,
                                 newMsg,
                                 (int)buffer.size(),
                                 INFO_TYPE_LteRlcAmSduToPdu);
    ERROR_Assert(info != NULL, "MESSAGE_AddInfo is failed.");
    memcpy(info, buffer.data(), buffer.size());

    return newMsg;
}


// /**
// FUNCTION   :: LteRlcPduToSduList
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Unpack a message to the original list of messages
// PARAMETERS ::
//  + node      : Node*    : Pointer to node.
//  + msg      : Message* : Pointer to the supper msg containing list of msgs
//  + freeOld   : BOOL     : Whether the original message should be freed
//  + msgList   : list<Message*> : an empty stl list to be used to
//                contain unpacked messages
// RETURN     :: NULL
// **/
void LteRlcPduToSduList(Node* node,
                        Message* msg,
                        BOOL freeOld,
                        std::list < Message* > &msgList)
{
    msgList.clear();

    Message* newMsg;

    char* info;
    int infoSize;
    int msgSize;

    info = MESSAGE_ReturnInfo(
        msg, (unsigned short)INFO_TYPE_LteRlcAmSduToPdu);
    infoSize = MESSAGE_ReturnInfoSize(
        msg, (unsigned short)INFO_TYPE_LteRlcAmSduToPdu);
    msgSize = 0;
    ERROR_Assert(info != NULL, "INFO_TYPE_LteRlcAmSduToPdu is not found.");
    while (msgSize < infoSize)
    {
        newMsg = LteRlcUnSerializeMessage(node,
                                          info,
                                          msgSize);

        // add new msg to msgList
        msgList.push_back(newMsg);
    }

    // free original message
    if (freeOld)
    {
        MESSAGE_Free(node, msg);
    }
}

// /**
// FUNCTION   :: LteRlcGetOrgPduSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get a message size from the original list of messages
// PARAMETERS ::
//  + node      : Node*    : Pointer to node.
//  + msg       : Message* : Pointer to the supper msg containing list of msgs
// RETURN     :: Int16 orgMessageSize
// **/
Int16 LteRlcGetOrgPduSize(Node* node,
                          Message* msg)
{
    std::list<Message*> msgList;
    msgList.clear();

    Message* newMsg;

    char* info;
    int infoSize;
    int msgSize;

    Int16 orgMessageSize = 0;

    info = MESSAGE_ReturnInfo(
        msg, (unsigned short)INFO_TYPE_LteRlcAmSduToPdu);
    infoSize = MESSAGE_ReturnInfoSize(
        msg, (unsigned short)INFO_TYPE_LteRlcAmSduToPdu);
    msgSize = 0;
    ERROR_Assert(info != NULL, "INFO_TYPE_LteRlcAmSduToPdu is not found.");
    while (msgSize < infoSize)
    {
        newMsg = LteRlcUnSerializeMessage(node,
                                          info,
                                          msgSize);
        // add new msg to msgList
        msgList.push_back(newMsg);
    }
    std::list<Message*>::iterator it = msgList.begin();
    while (it != msgList.end())
    {
        Message* tmpMsg = *it;
        orgMessageSize += (Int16)MESSAGE_ReturnPacketSize(tmpMsg);
        MESSAGE_Free(node, tmpMsg);
        it++;
    }
    return orgMessageSize;
}


// /**
// FUNCTION::       LteRlcAmIncSN
// LAYER::          LTE Layer2 RLC
// PURPOSE::        increment a sequence number (AM)
// PARAMETERS::
//  + seqNum : UInt32: The sequence number which is renewed
// RETURN::   void:      NULL
// **/
inline
void LteRlcAmIncSN(UInt32& seqNum)
{
    ++seqNum;
    seqNum %= LTE_RLC_AM_SN_UPPER_BOUND;
}

// /**
// FUNCTION::       LteRlcAmDecSN
// LAYER::          LTE Layer2 RLC
// PURPOSE::        increment a sequence number (AM)
// PARAMETERS::
//  + seqNum : UInt32: The sequence number which is renewed
// RETURN::   void:      NULL
// **/
inline
void LteRlcAmDecSN(UInt32& seqNum)
{
    if (seqNum == 0)
    {
        seqNum += LTE_RLC_AM_SN_UPPER_BOUND;
    }
    --seqNum;
}

// /**
// FUNCTION::       LteRlcAmGetSeqNum
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Get a sequence number (AM)
// PARAMETERS::
//  + msg : Message* : the msg where the The sequence number to be get
// RETURN::   unsigned int:     the sequence number
// **/
static
Int16 LteRlcAmGetSeqNum(const Message* msg)
{
    Int16   seqNum = LTE_RLC_INVALID_SEQ_NUM;

    LteRlcAmPduFormatFixed* info =
        (LteRlcAmPduFormatFixed*)MESSAGE_ReturnInfo(msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduFormatFixed);

    if (info != NULL)
    {
        seqNum = info->seqNum;
    }
    else
    {
        ERROR_Assert(FALSE, "LteRlcAmGetSeqNum: no fixed header field!");
    }

    return seqNum;
}

// /**
// FUNCTION::       LteRlcAmSeqNumDist
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Return the distance between sn1 and sn2,
//                  assuming sn1 is a "smaller" than sn2
// PARAMETERS::
//  + sn1 : unsigned int : The sequence number in the front
//  + sn2 : unsigned int : The sequence number in the back
// RETURN:: unsigned int : Sequence Number distance
// **/
static
UInt32 LteRlcAmSeqNumDist(UInt32 sn1, UInt32 sn2)
{
    if (sn1 <= sn2)
    {
        return sn2 - sn1;
    }
    else
    {
        return sn2 + (LTE_RLC_AM_SN_UPPER_BOUND - sn1);
    }
}

// /**
// FUNCTION::       LteRlcSeqNumInWnd
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Return whether a sequence number is within the window
// PARAMETERS::
//  + sn      : unsigned int : The sequence number to be tested
//  + snStart : unsigned int : The sequence number of window starting point
//  + wnd     : unsigned int : The window size
//  + snBound : unsigned int : The upperbound of sequence number
// RETURN::    void:     BOOL
// **/
//
static
BOOL LteRlcSeqNumInWnd(unsigned int sn,
                       unsigned int snStart,
                       unsigned int wnd,
                       unsigned int snBound)
{
    ERROR_Assert(wnd > 0 && wnd <= snBound/2,
        "LteRlcSeqNumInWnd: window size must be larger than 0 "
        "and smaller than half of the maximum sequence number. ");
    unsigned int snEnd = (snStart + wnd) % snBound;
    if (snStart < snEnd)
    {
        return (snStart <= sn && sn < snEnd);
    }
    else
    {
        return (snStart <= sn || sn < snEnd);
    }
}

// /**
// FUNCTION::       LteRlcAmSeqNumInWnd
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Return whether a sequence number is within the window
// PARAMETERS::
//  + sn      : unsigned int : The sequence number to be tested
//  + snStart : unsigned int : The sequence number of window starting point
// RETURN::    void:     BOOL
// **/
//
static
BOOL LteRlcAmSeqNumInWnd(unsigned int sn,
                         unsigned int snStart)
{
    return LteRlcSeqNumInWnd(sn,
                             snStart,
                             LTE_RLC_AM_WINDOW_SIZE,
                             LTE_RLC_AM_SN_UPPER_BOUND);
}

// /**
// FUNCTION::       LteRlcSeqNumInRange
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Return whether a sequence number is within the range
//                  test is, rStart <= sn < rEnd.
// PARAMETERS::
//  + sn     : unsigned int : The sequence number to be tested
//  + rStart : unsigned int : The sequence number of range starting point
//  + rEnd   : unsigned int : The sequence number of range end point
// RETURN::    void:     BOOL
// **/
//
static
BOOL LteRlcSeqNumInRange(unsigned int sn,
                         unsigned int rStart,
                         unsigned int rEnd)
{
    if (rStart <= rEnd)
    {
        return (rStart <= sn && sn < rEnd);
    }
    else
    {
        return (rStart <= sn || sn < rEnd);
    }
}

// /**
// FUNCTION::       LteRlcSeqNumLessChkInWnd
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Return whether a sequence number A is less than B,
//                  within the window.
//                  snA is allow out of the window, that case is allways TRUE
//                  Test is, snA < snB.(in window)
// PARAMETERS::
//  + snA     : unsigned int : The sequence number A to be tested
//  + snB     : unsigned int : The sequence number B to be tested
//  + wStart  : unsigned int : The sequence number of window starting point
//  + wEnd    : unsigned int : The sequence number of window end point
//  + snBound : unsigned int : The upperbound of sequence number
// RETURN::    void:     BOOL
// **/
//
static
BOOL LteRlcSeqNumLessChkInWnd(unsigned int snA,
                              unsigned int snB,
                              unsigned int wStart,
                              unsigned int wEnd,
                              unsigned int snBound)
{
//// Case (1)-(5) : snA is in the window
//(1)
//                  A,B
//  0<===========================================>1023
//       S<----------window---------->E
//
//(2)
//                                        A,B
//  0<===========================================>1023
//  ---->E                            S<----------
//
//(3)
//     A,B
//  0<===========================================>1023
//  -------->E                         S<---------
//
//(4)
//     B                                      A
//  0<===========================================>1023
//  ------>E                            S<--------
//
//(5)
//     A                                    B
//  0<===========================================>1023
//  ------>E                            S<--------

//// Case (6),(7) : snA is out of the window
//(6)
//     A                 or                 A
//  0<===========================================>1023
//       S<----------window---------->E
//(7)
//                       A
//  0<===========================================>1023
//  ------>E                            S<--------

    if (wStart > wEnd) {
        if (wEnd >= snA && wStart <= snB) {    // case (5)
            return FALSE;
        } else if (wEnd >= snB && wStart <= snA) {    // case (4)
            return TRUE;
        } else if ((wEnd >= snA && wEnd >= snB)        // case (3)
                 || (wStart <= snB && wStart <= snA)) { // case (2)
            return (snA < snB);
        } else if (wEnd < snA && wStart > snA) {       // case (7)
            return TRUE;
        }
    } else if (wEnd < snA || wStart > snA) {             // case (6)
        return TRUE;
    }
    return (snA < snB);                                  // case (1)
}


// /**
// FUNCTION::       LteRlcGetSubLayerData
// LAYER::          LTE LAYER2 RLC
// PURPOSE::        Get RLC sublayer pointer from node and interface Id
// PARAMETERS::
//  + node : Node* : node pointer
//  + ifac : int   : interface id
// RETURN::         lteRlcVar : RLC sublayer data
// **/
static
LteRlcData* LteRlcGetSubLayerData(Node* node, int iface)
{
    Layer2DataLte* layer2DataLte
        = LteLayer2GetLayer2DataLte(node, iface);
    return (LteRlcData*)layer2DataLte->lteRlcVar;
}

// /**
// FUNCTION::           findSnRetxBuffer
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Insert fragmented PDU into transmission buffer
// PARAMETERS::
//  + node       : Node* : Node pointer
//  + ifac       : int   : interface id
//  + retxBuffer : std::list<LteRlcAmReTxBuffer>& : re-transmittion buffer
//  + sn         : int   : Sequens number
//  + it         : std::list<LteRlcAmReTxBuffer>::iterator&
//                       : re-transmittion buffer's itarator
// RETURN::    void:         NULL
// **/
BOOL findSnRetxBuffer(Node* node,
                      int iface,
                      std::list < LteRlcAmReTxBuffer > &retxBuffer,
                      int sn,
                      std::list < LteRlcAmReTxBuffer > ::iterator& it)
{
    for (it = retxBuffer.begin();
        it != retxBuffer.end();
        it++)
    {
        if (it->rlcHeader.fixed->seqNum == sn)
        {
            return TRUE;
        }
    }

    return FALSE;
}

// /**
// FUNCTION::           DeleteSnRetxBuffer
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Delete ACKed PDU from retransmission buffer
// PARAMETERS::
//  + node       : Node*          : Node pointer
//  + iface      : int            : Interfase index
//  + amEntity   : LteRlcAmEntity : The RLC AM entity
//  + retxBuffer : std::list<LteRlcAmReTxBuffer>& : re-transmission buffer
// RETURN::    void:         NULL
// **/
void DeleteSnRetxBuffer(Node* node,
                        int iface,
                        LteRlcAmEntity* amEntity,
                        std::list < LteRlcAmReTxBuffer > &retxBuffer)
{
    std::list < LteRlcAmReTxBuffer > ::iterator it = retxBuffer.begin();
    while (it != retxBuffer.end())
    {
#ifdef LTE_LIB_LOG
        UInt16 retxSeq = it->rlcHeader.fixed->seqNum;
#endif
        if (it->pduStatus == LTE_RLC_AM_DATA_PDU_STATUS_ACK)
        {
#ifdef LTE_LIB_LOG
            lte::LteLog::DebugFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "DelReTx,"
                "%d",
                retxSeq);
#endif
            amEntity->retxBufNackSize -= it->getNackedSize(node, iface);
            amEntity->retxBufSize -= MESSAGE_ReturnPacketSize(it->message);
            MESSAGE_FreeList(node, it->message);
            LteRlcDelAmPduHdrObj(it->rlcHeader);
            it = retxBuffer.erase(it);
        }
        else
        {
            it++;
        }
    }

    return;
}


// /**
// FUNCTION::           SetAckedRetxBuffer
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Set PDU status=ACKed in the retransmission buffer
// PARAMETERS::
//  + node       : Node*          : Node pointer
//  + iface      : int            : Interface index
//  + amEntity   : LteRlcAmEntity : The RLC AM entity
//  + retxBuffer : std::list<LteRlcAmReTxBuffer>& : retransmission buffer
//  + sn         : int            : Sequens number
// RETURN::    void:           NULL
// **/
void SetAckedRetxBuffer(Node* node,
                        int iface,
                        LteRlcAmEntity& amEntity,
                        std::list < LteRlcAmReTxBuffer > &retxBuffer,
                        int sn)
{
    std::list < LteRlcAmReTxBuffer > ::iterator it;
    if (findSnRetxBuffer(node,
                         iface,
                         retxBuffer,
                         sn,
                         it))
    {
        it->pduStatus = LTE_RLC_AM_DATA_PDU_STATUS_ACK;
    }
}

// /**
// FUNCTION::           SetNackedRetxBuffer
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Set PDU status=NACKed in the retransmission buffer
// PARAMETERS::
//  + node       : Node*          : Node pointer
//  + iface      : int            : Interface index
//  + amEntity   : LteRlcAmEntity : The RLC AM entity
//  + retxBuffer : std::list<LteRlcAmReTxBuffer>& : retransmission buffer
//  + sn         : int            : Sequens number
//  + nacked     : LteRlcStatusPduSoPair : Nacked info (SOstart, SOend)
// RETURN::    void:           NULL
// **/
void SetNackedRetxBuffer(Node* node,
                         int iface,
                         LteRlcAmEntity& amEntity,
                         std::list < LteRlcAmReTxBuffer > &retxBuffer,
                         int sn,
                         LteRlcStatusPduSoPair nacked)
{
    BOOL addFlag = TRUE;
    std::list < LteRlcAmReTxBuffer > ::iterator it;
    if (findSnRetxBuffer(node,
                         iface,
                         retxBuffer,
                         sn,
                         it))
    {
        // Whole parts is NACK.
        if (nacked.soStart
            == LTE_RLC_INVALID_STATUS_PDU_FORMAT_NACK_SEGMENT_OFFSET)
        {
            nacked.soStart = 0;
            nacked.soEnd = MESSAGE_ReturnPacketSize((*it).message);
        }

        amEntity.retxBufNackSize -= it->getNackedSize(node, iface);
        std::map < int, LteRlcStatusPduSoPair > ::iterator nackIt;
        nackIt = it->nackField.lower_bound(nacked.soStart);
        BOOL first = TRUE;
        if (it->nackField.begin() != nackIt)
        {
            nackIt--;
            // check front
            while (TRUE)
            {
                if (nackIt->second.soEnd >= nacked.soStart)
                {
                    if (nackIt->second.soEnd <= nacked.soEnd)
                    {
                        nackIt->second.soEnd = nacked.soEnd;
                        addFlag = FALSE;

                        if (first == TRUE)
                        {
                            first = FALSE;
                        }
                        else
                        {
                            nackIt++;
                            it->nackField.erase(nackIt++);
                        }
                    }
                    else
                    {
                        return;
                    }

                }

                if (it->nackField.begin() == nackIt)
                {
                    break;
                }

                if (it->nackField.begin() != nackIt)
                {
                    nackIt--;
                }
            }
        }

        nackIt = it->nackField.lower_bound(nacked.soStart);
        if (first == FALSE)
        {
            if (it->nackField.begin() != nackIt){
                nackIt--;
            }
            nacked.soStart = nackIt->second.soStart;
            nacked.soEnd   = nackIt->second.soEnd;
            if (it->nackField.begin() == nackIt)
            {
                nackIt++;
            }

        }
        // check back
        while (it->nackField.end() != nackIt)
        {
            if (nackIt->second.soStart <= nacked.soEnd)
            {
                addFlag = FALSE;
                nackIt->second.soStart = nacked.soStart;

                if (nackIt->second.soEnd >= nacked.soEnd)
                {
                    nacked.soEnd = nackIt->second.soEnd;
                }
                else
                {
                    nackIt->second.soEnd = nacked.soEnd;
                }

                if (first == TRUE)
                {
                    first = FALSE;
                    it->nackField.erase(nackIt++);
                    if (it->nackField.begin() != nackIt){
                        nackIt--;
                        nackIt = it->nackField.insert(nackIt,
                                     std::pair < int, LteRlcStatusPduSoPair >
                                     (nacked.soStart, nacked));
                        nackIt++;
                    }
                    else
                    {
                        nackIt = it->nackField.insert(nackIt,
                                 std::pair < int, LteRlcStatusPduSoPair >
                                 (nacked.soStart, nacked));
                    }
                }
                else
                {
                    nacked.soEnd = nackIt->second.soEnd;
                    if (it->nackField.begin() != nackIt){
                        nackIt--;
                        nackIt->second.soEnd = nacked.soEnd;
                        nackIt++;
                        it->nackField.erase(nackIt++);
                    }
                    else
                    {
                        nackIt->second.soEnd = nacked.soEnd;
                        nackIt++;
                    }
                }
            }
            else
            {
                nackIt++;
            }
        }

        if (addFlag == TRUE)
        {
            it->nackField.insert(std::pair < int, LteRlcStatusPduSoPair >
                (nacked.soStart, nacked));
        }

        amEntity.retxBufNackSize
            += it->getNackedSize(node, iface);
        it->pduStatus = LTE_RLC_AM_DATA_PDU_STATUS_NACK;
    }
}
// /**
// FUNCTION::           SetNackedRetxBuffer
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Set PDU status=NACKed in the retransmission buffer
// PARAMETERS::
//  + node       : Node*         : Node pointer
//  + iface      : int           : Interface index
//  + rlcEntity  : LteRlcEntity* :   The RLC entity
//  + retxBuffer : std::list<LteRlcAmReTxBuffer>& : retransmission buffer
//  + nackSoMap  : std::multimap< UInt16, LteRlcStatusPduSoPair >&
//                               : Nacked info - SN & SO(SOstart, SOend)
// RETURN::    void:         NULL
// **/
void SetNackedRetxBuffer(
    Node* node,
    int iface,
    LteRlcAmEntity& amEntity,
    std::list < LteRlcAmReTxBuffer > &retxBuffer,
    std::multimap < UInt16, LteRlcStatusPduSoPair > &nackSoMap)
{
    std::multimap < UInt16, LteRlcStatusPduSoPair > ::iterator it;

    for (it = nackSoMap.begin(); it != nackSoMap.end(); it++)
    {
        SetNackedRetxBuffer(node,
                            iface,
                            amEntity,
                            retxBuffer,
                            it->first,
                            it->second);

#ifdef LTE_LIB_LOG
        {
            std::stringstream  log;
            log << "SetNackedRetxBuffer";
            log << ",it->first(SN)," << it->first;
            log << ",it->second.soStart," << it->second.soStart;
            log << ",it->second.soEnd," << it->second.soEnd;
            lte::LteLog::DebugFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "%s,%s",
                LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
                log.str().c_str());
        }
#endif
    }
}
// /**
// FUNCTION::           OverlapBindingNackedRetxBuffer
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            collect NACKed info in the retransmission buffer
// PARAMETERS::
//  + node       : Node*          : Node pointer
//  + iface      : int            : Interface index
//  + retxBuffer : std::list<LteRlcAmReTxBuffer>& : retransmission buffer
//  + sn         : int            : Sequens number
//  + nacked     : LteRlcStatusPduSoPair : Nacked info (SOstart, SOend)
// RETURN::    void:         NULL
// **/
void OverlapBindingNackedRetxBuffer(Node* node,
                      int iface,
                      std::list < LteRlcAmReTxBuffer > &retxBuffer,
                      int sn,
                      LteRlcStatusPduSoPair nacked)
{
    std::list < LteRlcAmReTxBuffer > ::iterator it;
    if (findSnRetxBuffer(node,
                     iface,
                     retxBuffer,
                     sn,
                     it))
    {
        std::map < int, LteRlcStatusPduSoPair > ::iterator nackIt;
        for (nackIt = it->nackField.begin();
            nackIt != it->nackField.end();
            nackIt++)
        {
            if (nackIt->second.soEnd == nacked.soStart)
            {
                nackIt->second.soEnd = nacked.soEnd;
                break;
            }
        }
    }
}

// /**
// FUNCTION   :: LteRlcInitTimer
// LAYER      :: LTE LAYER2 RLC
// PURPOSE    :: Set a timer with given type
// PARAMETERS ::
//  + node      : Node*        :  Pointer to node
//  + iface     : int          : the interface index
//  + timerType : Event/message types : type of the timer
//  + rnti      : LteRnti&     : RNTI
//  + bearerId  : unsigned int : radio bearer ID of the entity that
//                               will receive the timer message
//  + direction : LteRlcEntityDirect : Entity traffic direction
// RETURN     :: Message * : Pointer to the allocatd timer message
// **/
static
Message* LteRlcInitTimer(Node* node,
                         int iface,
                         int timerType,
                         const LteRnti& rnti,
                         UInt32 bearerId,
                         LteRlcEntityDirect direction)
{
    Message* timerMsg = NULL;

    // allocate the timer message and send out
    timerMsg = MESSAGE_Alloc(node,
                             MAC_LAYER,
                             MAC_PROTOCOL_LTE,
                             timerType);

    MESSAGE_SetInstanceId(timerMsg, (short)iface);

    MESSAGE_InfoAlloc(node,
                      timerMsg,
                      sizeof(LteRlcTimerInfoHdr));

    char* infoHead = MESSAGE_ReturnInfo(timerMsg);

    LteRlcTimerInfoHdr* infoHdr = (LteRlcTimerInfoHdr*)infoHead;

    infoHdr->rnti = rnti;
    infoHdr->bearerId = (char)bearerId;
    infoHdr->direction = direction;

    return timerMsg;
}

// /**
// FUNCTION::           LteRlcAmInitPollRetrunsmitTimer
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Init PollRetrunsmit Timer
// PARAMETERS::
//  + node     : Node*           : Node pointer
//  + iface    : int             : Inteface index
//  + amEntity : LteRlcAmEntity* : The RLC entity
// RETURN::    void:         NULL
// **/
static
void LteRlcAmInitPollRetrunsmitTimer(Node* node,
                                     int iface,
                                     LteRlcAmEntity* amEntity)
{
    LteRlcEntity* rlcEntity = (LteRlcEntity*)(amEntity->entityVar);
    if (amEntity->pollRetransmitTimerMsg)
    {
        MESSAGE_CancelSelfMsg(node, amEntity->pollRetransmitTimerMsg);
        amEntity->pollRetransmitTimerMsg = NULL;
    }
    amEntity->pollRetransmitTimerMsg =
        LteRlcInitTimer(node,
                        iface,
                        MSG_RLC_LTE_PollRetransmitTimerExpired,
                        rlcEntity->oppositeRnti,
                        rlcEntity->bearerId,
                        LTE_RLC_ENTITY_TX);
    amEntity->exprioryTPollRetransmitFlg = FALSE;
    MESSAGE_Send(node,
                 amEntity->pollRetransmitTimerMsg,
                 GetLteRlcConfig(node, iface)->tPollRetransmit *
                    MILLI_SECOND);
#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmInitPollRetrunsmitTimer[start]:" << ",";
        log << GetLteRlcConfig(node, iface)->tPollRetransmit << ",";
        log << "msec";
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_CONTROL,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG
}

// /**
// FUNCTION::           LteRlcAmInitReoderingTimer
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Init Reordering Timer
// PARAMETERS::
//  + node     : Node*           : Node pointer
//  + iface    : int             : Inteface index
//  + amEntity : LteRlcAmEntity* : The RLC entity
// RETURN::    void:         NULL
// **/
static
void LteRlcAmInitReoderingTimer(Node* node,
                                int iface,
                                LteRlcAmEntity* amEntity)
{
    LteRlcEntity* rlcEntity = (LteRlcEntity*)(amEntity->entityVar);
    if (amEntity->reoderingTimerMsg)
    {
        MESSAGE_CancelSelfMsg(node, amEntity->reoderingTimerMsg);
        amEntity->reoderingTimerMsg = NULL;
    }
    amEntity->reoderingTimerMsg =
        LteRlcInitTimer(node,
                        iface,
                        MSG_RLC_LTE_ReorderingTimerExpired,
                        rlcEntity->oppositeRnti,
                        rlcEntity->bearerId,
                        LTE_RLC_ENTITY_RX);
    MESSAGE_Send(node,
                 amEntity->reoderingTimerMsg,
                 GetLteRlcConfig(node, iface)->tReordering * MILLI_SECOND);
#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmInitReoderingTimer[start]:,";
        log << GetLteRlcConfig(node, iface)->tReordering;
        log << ",msec";
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_CONTROL,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG
}

// /**
// FUNCTION::           LteRlcAmInitStatusProhibitTimer
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Init StatusProhibit Timer
// PARAMETERS::
//  + node     : Node*           : Node pointer
//  + iface    : int             : Inteface index
//  + amEntity : LteRlcAmEntity* : The RLC entity
// RETURN::    void:         NULL
// **/
static
void LteRlcAmInitStatusProhibitTimer(Node* node,
                                     int iface,
                                     LteRlcAmEntity* amEntity)
{
    LteRlcEntity* rlcEntity = (LteRlcEntity*)(amEntity->entityVar);
    if (amEntity->statusProhibitTimerMsg)
    {
        MESSAGE_CancelSelfMsg(node, amEntity->statusProhibitTimerMsg);
        amEntity->statusProhibitTimerMsg = NULL;
    }
    amEntity->statusProhibitTimerMsg =
        LteRlcInitTimer(node,
                        iface,
                        MSG_RLC_LTE_StatusProhibitTimerExpired,
                        rlcEntity->oppositeRnti,
                        rlcEntity->bearerId,
                        LTE_RLC_ENTITY_RX);
    amEntity->waitExprioryTStatusProhibitFlg = FALSE;
    MESSAGE_Send(node,
                 amEntity->statusProhibitTimerMsg,
                 GetLteRlcConfig(node, iface)->tStatusProhibit *
                    MILLI_SECOND);
#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmInitStatusProhibitTimer[start]:,";
        log << GetLteRlcConfig(node, iface)->tStatusProhibit;
        log << ",msec";
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_CONTROL,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG
}

// /**
// FUNCTION::           LteRlcAmInitResetTimer
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Init Reset Timer
// PARAMETERS::
//  + node     : Node*           : Node pointer
//  + iface    : int             : Inteface index
//  + amEntity : LteRlcAmEntity* : The RLC entity
//  + direct   : LteRlcEntityDirect : The RLC entity direction
// RETURN::    void:         NULL
// **/
static
void LteRlcAmInitResetTimer(Node* node,
                            int iface,
                            LteRlcAmEntity* amEntity,
                            LteRlcEntityDirect direct)
{
    ERROR_Assert(direct != LTE_RLC_ENTITY_BI,
        "Wrong LteRlcEntityDirect(LTE_RLC_ENTITY_BI).");

    LteRlcEntity* rlcEntity = (LteRlcEntity*)(amEntity->entityVar);
    if (amEntity->resetTimerMsg)
    {
        MESSAGE_CancelSelfMsg(node, amEntity->resetTimerMsg);
        amEntity->resetTimerMsg = NULL;
    }
    amEntity->resetTimerMsg =
        LteRlcInitTimer(node,
                        iface,
                        MSG_RLC_LTE_ResetTimerExpired,
                        rlcEntity->oppositeRnti,
                        rlcEntity->bearerId,
                        direct);
    clocktype delay = (direct == LTE_RLC_ENTITY_TX)
        ? LTE_RLC_RESET_TIMER_DELAY_MSEC * MILLI_SECOND
        : (LTE_RLC_RESET_TIMER_DELAY_MSEC - 1) * MILLI_SECOND;

    MESSAGE_Send(node,
                 amEntity->resetTimerMsg,
                 delay);
#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmInitResetTimer[start]:,";
        log << delay;
        log << ",msec";
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_CONTROL,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG
}

// /**
// FUNCTION::           LteRlcEntity::setRlcEntityToXmEntity
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            set own pointer to Xm(Tm/Um/Am)'s entityVar
// **/
void LteRlcEntity::setRlcEntityToXmEntity()
{
    char errorMsg[MAX_STRING_LENGTH] =
        "LteRlcEntity::setRlcEntityToXmEntity: ";
    switch (entityType)
    {
        case LTE_RLC_ENTITY_AM:
        {
            LteRlcAmEntity* amEntity = (LteRlcAmEntity*)entityData;
            amEntity->entityVar = this;
            break;
        }
        default:
        {
            sprintf(errorMsg+strlen(errorMsg), "wrong RLC entity type.");
            ERROR_ReportError(errorMsg);
        }
    }

}


// /**
// FUNCTION::           LteRlcEntity::store
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            store RLC-SDU to TxBuffer in the RLC-entity
// PARAMETERS::
//  + node  : Node*    : Node Pointer
//  + iface : int      : Inteface index
//  + msg   : Message* : RLC-SDU message
// RETURN::     void:        NULL
// **/
void LteRlcAmEntity::store(Node* node,int iface, Message* msg)
{
    LteRlcData* lteRlcData =
                       LteLayer2GetLteRlcData(node, iface);

    if (txBufSize + MESSAGE_ReturnPacketSize(msg) > maxTransBufSize)
    {
#ifdef ADDON_DB
        HandleMacDBEvents(
            node,
            msg,
            node->macData[iface]->phyNumber,
            iface,
            MAC_Drop,
            node->macData[iface]->macProtocol,
            TRUE,
            "Discard due to RlcAmEntity Overflow");
#endif
        lteRlcData->stats.numSdusDiscardedByOverflow++;
#ifdef LTE_LIB_LOG
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "SduFromPdcp,Discarded by overflow,"
            "txBufSize=,%llu,sduSize=,%d",
            txBufSize, MESSAGE_ReturnPacketSize(msg));
#endif
        MESSAGE_Free(node, msg);
        return;
    }
#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    LteLayer2ValidationData* valData =
        LteLayer2GetValidataionData(node, iface, entityVar->oppositeRnti);
    if (valData != NULL)
    {
        valData->rlcNumSduToTxQueue++;
    }
#endif
#endif

    txBufSize += MESSAGE_ReturnPacketSize(msg);
#ifdef LTE_LIB_LOG
    lte::LteLog::DebugFormat(
        node,
        iface,
        LTE_STRING_LAYER_TYPE_RLC,
        "SduFromPdcp,Enqueue,"
        "txBufSize=,%llu,sduSize=,%d",
        txBufSize, MESSAGE_ReturnPacketSize(msg));
#endif

    LteRlcAmTxBuffer txMsg;
    txMsg.store(node, iface, msg);

    txBuffer.push_back(txMsg);

#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmEntity::store:";
        log << ",txBuffer.size()," << txBuffer.size();
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG


    return;
}


// /**
// FUNCTION::           LteRlcEntity::restore
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            restore the RLC entity
// PARAMETERS::
//  + node  : Node* : Node Pointer
//  + iface : int   : Inteface index
//  + size  : int   : request size of TXOP Notification
//  + txMsg : std::list<Message*>& : RLC-PDU messages list
// RETURN::    void :         NULL
// **/
void LteRlcAmEntity::restore(Node* node,
                             int iface,
                             int size,
                             std::list < Message* > &txMsg)
{
    int restSize = size;
    int txPduByte = 0;
    int retxPduByte = 0;
    int statusPduByte = 0;
    int pollPduByte = 0;
    int macHeaderByte = MAC_LTE_RRELCIDFL_WITH_15BIT_SUBHEADER_SIZE;
    int numTxPdu = 0;
    int numRetxPdu = 0;
    int numStatusPdu = 0;

    if (resetFlg == TRUE)
    {
        return;
    }

    BOOL makedPdu = FALSE;
    BOOL makedPduSegment = FALSE;

#ifdef LTE_LIB_LOG
    {
        std::stringstream  log;
        log << "LteRlcAmEntity::restore";
        log << ",size,"                << size;
        log << ",restSize,"            << restSize;
        log << ",txBufSize,"           << txBufSize;
        log << ",txBuffer.size(),"     << txBuffer.size();
        log << ",statusBuffer.size()," << statusBuffer.size();
        log << ",retxBufSize,"         << retxBufSize;
        log << ",retxBuffer.size(),"   << retxBuffer.size();
        log << ",retxBufNackSize,"     << retxBufNackSize;
        log << ",statusBufSize,"       << statusBufSize;
#ifdef LTE_RLC_BUFF_SIZE_TEST
        log << ",getTestBufferSize()," << getTestTxBufferSize();
        log << ",getTestRetxBufferSize()," << getTestRetxBufferSize();
        log << ",getTestNackedSize()," << getTestNackedSize();
        log << ",getTestStatusSize()," << getTestStatusSize();
#endif
        LteRlcData* lteRlc = LteLayer2GetLteRlcData(node, iface);
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
            log.str().c_str());
    }
#endif

    // 1.) Make Status-PDU
    int reqSizeWithoutMacHeader = restSize - macHeaderByte;
    reqSizeWithoutMacHeader = restSize > macHeaderByte
        ? restSize - macHeaderByte
        : 0;
    getStatusBuffer(node,
                    iface,
                    reqSizeWithoutMacHeader,
                    statusBuffer,
                    txMsg,
                    statusPduByte);
    numStatusPdu = txMsg.size();;
    restSize = statusPduByte > 0
        ? reqSizeWithoutMacHeader
        : restSize;

#ifdef LTE_LIB_LOG
    {
        std::stringstream  log;
        log << "LteRlcAmEntity:getStatusBuffer";
        log << ",size,"                    << size;
        log << ",restSize,"                << restSize;
        log << ",txBufSize,"               << txBufSize;
        log << ",txBuffer.size(),"         << txBuffer.size();
        log << ",statusBuffer.size(),"     << statusBuffer.size();
        log << ",retxBufSize,"             << retxBufSize;
        log << ",retxBuffer.size(),"       << retxBuffer.size();
        log << ",retxBufNackSize,"         << retxBufNackSize;
        log << ",statusBufSize,"           << statusBufSize;
#ifdef LTE_RLC_BUFF_SIZE_TEST
        log << ",getTestBufferSize(),"     << getTestTxBufferSize();
        log << ",getTestRetxBufferSize()," << getTestRetxBufferSize();
        log << ",getTestNackedSize(),"     << getTestNackedSize();
        log << ",getTestStatusSize(),"     << getTestStatusSize();
#endif

        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
            log.str().c_str());
    }
#endif

    // 2.) Make RLC-PDU from retransmission buffer.
    int availableNumPdu = getNumAvailableRetxPdu(node, iface, restSize);
    reqSizeWithoutMacHeader = restSize > availableNumPdu * macHeaderByte
        ? restSize - availableNumPdu * macHeaderByte
        : 0;
    makedPduSegment = getRetxBuffer(node,
                                    iface,
                                    reqSizeWithoutMacHeader,
                                    retxBuffer,
                                    txMsg,
                                    retxPduByte);
    numRetxPdu = txMsg.size() - numStatusPdu;
    // if numRetxPdu is less than availableNumPdu,
    // restSize is adjusted by numRetxPdu and macHeaderSize.
    restSize = reqSizeWithoutMacHeader +
        (availableNumPdu - numRetxPdu) * macHeaderByte;

    if (resetFlg == TRUE)
    {
#ifdef LTE_LIB_LOG
        lte::LteLog::InfoFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "Reset,"
            LTE_STRING_FORMAT_RNTI","
            "TxSide,"
            "BearerId=,%d",
            entityVar->oppositeRnti.nodeId,
            entityVar->oppositeRnti.interfaceIndex,
            entityVar->bearerId);
#endif
        return;
    }
#ifdef LTE_LIB_LOG
    {
        std::stringstream  log;
        log << "LteRlcAmEntity:getRetxBuffer";
        log << ",size,"                << size;
        log << ",restSize,"            << restSize;
        log << ",txBufSize,"           << txBufSize;
        log << ",txBuffer.size(),"     << txBuffer.size();
        log << ",statusBuffer.size()," << statusBuffer.size();
        log << ",retxBufSize,"         << retxBufSize;
        log << ",retxBuffer.size(),"   << retxBuffer.size();
        log << ",retxBufNackSize,"     << retxBufNackSize;
        log << ",statusBufSize,"       << statusBufSize;
#ifdef LTE_RLC_BUFF_SIZE_TEST
        log << ",getTestBufferSize()," << getTestTxBufferSize();
        log << ",getTestRetxBufferSize()," << getTestRetxBufferSize();
        log << ",getTestNackedSize()," << getTestNackedSize();
        log << ",getTestStatusSize()," << getTestStatusSize();
#endif


        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
            log.str().c_str());
    }
#endif

    // 3.) Make RLC-PDU from trunsmission buffer.
    //     The next SN must be in a window.
    if (!LteRlcAmSeqNumInWnd(this->sndNextSn, this->ackSn))
    {
#ifdef LTE_LIB_LOG
        {
            std::stringstream  log;
            log << "LteRlcAmEntity::restore:LteRlcAmSeqNumInWnd ";
            log << "out of window";
            lte::LteLog::DebugFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "%s,%s",
                LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
                log.str().c_str());
        }
#endif
    }
    else
    {
        reqSizeWithoutMacHeader = restSize - macHeaderByte  > 0
            ? restSize - macHeaderByte
            : 0;
        makedPdu = getTxBuffer(node,
                               iface,
                               reqSizeWithoutMacHeader,
                               txBuffer,
                               txMsg,
                               txPduByte);
        numTxPdu = txMsg.size() - numStatusPdu - numRetxPdu;
        restSize = txPduByte > 0
            ? reqSizeWithoutMacHeader
            : restSize;
    }

#ifdef LTE_LIB_LOG
    {
        std::stringstream  log;
        log << "LteRlcAmEntity:getTxBuffer";
        log << ",size,"                << size;
        log << ",restSize,"            << restSize;
        log << ",txBufSize,"           << txBufSize;
        log << ",txBuffer.size(),"     << txBuffer.size();
        log << ",statusBuffer.size()," << statusBuffer.size();
        log << ",retxBufSize,"         << retxBufSize;
        log << ",retxBuffer.size(),"   << retxBuffer.size();
        log << ",retxBufNackSize,"     << retxBufNackSize;
        log << ",statusBufSize,"       << statusBufSize;
#ifdef LTE_RLC_BUFF_SIZE_TEST
        log << ",getTestBufferSize()," << getTestTxBufferSize();
        log << ",getTestRetxBufferSize()," << getTestRetxBufferSize();
        log << ",getTestNackedSize()," << getTestNackedSize();
        log << ",getTestStatusSize()," << getTestStatusSize();
#endif


        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
            log.str().c_str());
    }
#endif

    // Polling function.
    polling(node, iface, restSize,
            makedPdu, makedPduSegment, txMsg, pollPduByte);

#ifdef LTE_LIB_LOG
    lte::LteLog::DebugFormat(
        node,
        iface,
        LTE_STRING_LAYER_TYPE_RLC,
        "%s,"
        "ReqSize=,%d,"
        "TxPdu=,%d,TxBuf=,%llu,"
        "RetxPdu=,%d,PollPdu=,%d,RetxBuf=,%llu,"
        "StatusPdu=,%d,StatusBuf=,%llu,"
        "TotalPdu=,%d",
        LTE_STRING_CATEGORY_TYPE_RLC_DEQUEUE_INFO,
        size,
        txPduByte, txBufSize,
        retxPduByte, pollPduByte, retxBufSize,
        statusPduByte, statusBufSize,
        txPduByte + retxPduByte + pollPduByte+statusPduByte);

    if (txMsg.size() > 0)
    {
        dump(node, iface, "AfterSend");
    }
#endif

    return;
}

// /**
// FUNCTION::           LteRlcEntity::getBufferSize
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            get buffer size of RLC entity
// PARAMETERS::
//  + node  : Node* : Node Pointer
//  + iface : int   : Inteface index
// RETURN::    void :         NULL
// **/
int LteRlcAmEntity::getBufferSize(Node* node, int iface)
{
    UInt64 retSize = 0;
    retSize += txBufSize;
    retSize += retxBufNackSize;
    retSize += statusBufSize;

    return (int)retSize;
}

// /**
// FUNCTION::           LteRlcEntity::discardPdu
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            discard PDU by re-transmit upper limit.
// PARAMETERS::
//  + node        : Node* : Node Pointer
//  + iface       : int   : Inteface index
//  + it          : std::list<LteRlcAmReTxBuffer>::iterator&
//                        : re-transmittion buffer iterator
//  + buf         : std::list<LteRlcAmReTxBuffer>& : re-transmittion buffer
//  + txMsg       : std::list<Message*>& : RLC-PDU messages list
//  + retxPduByte : int&  : re-transmit PDU size
// RETURN::    void :         NULL
// **/
BOOL LteRlcAmEntity::discardPdu(
    Node* node,
    int iface,
    std::list < LteRlcAmReTxBuffer > ::iterator& it,
    std::list < LteRlcAmReTxBuffer > &buf,
    std::list < Message* > &pdu,
    int& retxPduByte)
{
    BOOL ret = FALSE;

#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
    lte::LteLog::InfoFormat(
        node, iface, LTE_STRING_LAYER_TYPE_RLC,
        "ReachedRetxMax,"
        LTE_STRING_FORMAT_RNTI","
        "%d",
        entityVar->oppositeRnti.nodeId,
        entityVar->oppositeRnti.interfaceIndex,
        it->rlcHeader.fixed->seqNum);
#endif
#endif
    LteRlcData* lteRlc =
        LteLayer2GetLteRlcData(node, iface);
    lteRlc->stats.numDataPdusDiscardedByRetransmissionThreshold++;
    retxBufSize -= MESSAGE_ReturnPacketSize(it->message);
    MESSAGE_FreeList(node, it->message);
    it->message = NULL;
    LteRlcDelAmPduHdrObj(it->rlcHeader);
    it = buf.erase(it);
    // Reset
    {
        resetAll(TRUE);
        LteRlcAmSetResetData(node, iface, this);
        std::list < Message* > ::iterator pduItr;
        for (pduItr  = pdu.begin();
            pduItr != pdu.end();
            ++pduItr)
        {
            MESSAGE_FreeList(node, (*pduItr));
        }
        pdu.clear();
        resetFlg = TRUE;
        sendResetFlg = TRUE;
        retxPduByte = 0;
        LteRlcAmInitResetTimer(node, iface, this, LTE_RLC_ENTITY_TX);
        return ret;
    }
}

// /**
// FUNCTION::           LteRlcEntity::getRetxBuffer
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            get PDU from re-transmit buffer.
// PARAMETERS::
//  + node        : Node* : Node Pointer
//  + iface       : int   : Inteface index
//  + restSize    : int&  : request data size
//  + buf         : std::list<LteRlcAmReTxBuffer>& : re-transmittion buffer
//  + pdu         : std::list<Message*>& : RLC-PDU messages list
//  + retxPduByte : int&  : re-transmit PDU size
// RETURN::    void :         NULL
// **/
BOOL LteRlcAmEntity::getRetxBuffer(Node* node,
                                   int iface,
                                   int& restSize,
                                   std::list < LteRlcAmReTxBuffer > &buf,
                                   std::list < Message* > &pdu,
                                   int& retxPduByte)
{
    LteRlcData* lteRlc = LteLayer2GetLteRlcData(node, iface);
    LteRlcConfig* rlcConfig = GetLteRlcConfig(node, iface);
    BOOL ret = FALSE;
    BOOL resegment = FALSE;
    std::list < LteRlcAmReTxBuffer > ::iterator it;
    for (it = buf.begin(); it != buf.end();)
    {
        // NackData size
        int nackDataSize = 0;
        std::map < int, LteRlcStatusPduSoPair > ::iterator nackIt;

        for (nackIt = it->nackField.begin();
             nackIt != it->nackField.end();)
        {
            resegment = FALSE;
            ERROR_Assert((*nackIt).second.soEnd >=
                           (*nackIt).second.soStart,
                         "RLC-LTE SegmentOffset field: start > end !");
            nackDataSize = (*nackIt).second.soEnd
                            - (*nackIt).second.soStart;
            if (nackDataSize == 0)
            {
                break;
            }

#ifdef LTE_LIB_LOG
            {
                std::stringstream log;
                log << "getRetxBuffer:start";
                log << ",seqNum," << it->rlcHeader.fixed->seqNum;
                log << ",resegment," << resegment;
                log << ",nackDataSize," << nackDataSize;
                log << ",MESSAGE_ReturnPacketSize(it->message),";
                log << MESSAGE_ReturnPacketSize(it->message);
                lte::LteLog::DebugFormat(
                    node,
                    iface,
                    LTE_STRING_LAYER_TYPE_RLC,
                    "%s,%s",
                    LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
                    log.str().c_str());
            }
#endif // LTE_LIB_LOG

            if (nackDataSize == MESSAGE_ReturnPacketSize(it->message))
            {
                // no resegmentation
                int size = nackDataSize +
                           LteRlcGetExpectedAmPduHeaderSize(node,
                                              iface,
                                              it->rlcHeader,
                                              FALSE);

#ifdef LTE_LIB_LOG
                {
                    std::stringstream log;
                    log.str("");
                    log << "getRetxBuffer:";
                    log << ",nack Pud expected size," << size;
                    log << ",restSize ," << restSize;
                    lte::LteLog::DebugFormat(
                        node,
                        iface,
                        LTE_STRING_LAYER_TYPE_RLC,
                        "%s,%s",
                        LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
                        log.str().c_str());
                }
#endif // LTE_LIB_LOG

                // size less than request size
                // no resegment
                if (restSize > size)
                {
                    restSize -= size;

                    // this message is not included header size
                    Message* dupMsg = MESSAGE_Duplicate(node, it->message);
                    // add header size and set to info
                    LteRlcSetMessageRlcHeaderInfo(node,
                                                  iface,
                                                  it->rlcHeader,
                                                  dupMsg);

                    it->retxCount++;
                    if (it->retxCount > rlcConfig->maxRetxThreshold)
                    {
                        if (dupMsg) {
                            MESSAGE_Free(node, dupMsg);
                            dupMsg = NULL;
                        } else {
                            // do nothing
                        }
                        ret = discardPdu(
                                    node, iface, it, buf, pdu, retxPduByte);
                        return ret;
                    }

                    pdu.push_back(dupMsg);
                    retxPduByte += MESSAGE_ReturnPacketSize(dupMsg);
                    lteRlc->stats.numDataPdusSentToMacSublayer++;
                    ret = TRUE;

                    // update nack size
                    retxBufNackSize -= it->getNackedSize(node, iface);
                    it->nackField.erase(nackIt++);

#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
                    lte::LteLog::InfoFormat(
                        node,
                        iface,
                        LTE_STRING_LAYER_TYPE_RLC,
                        "SendRetx,"
                        LTE_STRING_FORMAT_RNTI","
                        "%d,%d",
                        entityVar->oppositeRnti.nodeId,
                        entityVar->oppositeRnti.interfaceIndex,
                        it->rlcHeader.fixed->seqNum,
                        it->retxCount);
#endif
#endif // LTE_LIB_LOG

                    if (it->nackField.empty())
                    {
                        it->pduStatus = LTE_RLC_AM_DATA_PDU_STATUS_WAIT;
                    }
                    continue;
                }
                else
                {
                    resegment = TRUE;
                }
            }
            else
            {
                resegment = TRUE;
            }

            if (resegment == TRUE)
            {
                int headerSize = LteRlcGetExpectedAmPduHeaderSize(node,
                                                 iface,
                                                 it->rlcHeader,
                                                 TRUE);
                int size = nackDataSize + headerSize;


#ifdef LTE_LIB_LOG
                {
                    std::stringstream log;
                    log.str("");
                    log << "getRetxBuffer:Resegment";
                    log << ",seqNum," << it->rlcHeader.fixed->seqNum;
                    log << ",resegment," << resegment;
                    log << ",retxBufSize," << retxBufSize;
                    log << ",headerSize," << headerSize;
                    log << ",expect pdu size," << size;
                    log << ",it->retxCount," << it->retxCount;
                    log << ",it->pduStatus," << it->pduStatus;

                    lte::LteLog::DebugFormat(
                        node,
                        iface,
                        LTE_STRING_LAYER_TYPE_RLC,
                        "%s,%s",
                        LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
                        log.str().c_str());
                }
#endif // LTE_LIB_LOG

                if (restSize >= size)
                {
                    restSize -= size;
                    LteRlcHeader txRlcHeader;
                    LteRlcCpyAmPduHdrObj(it->rlcHeader,txRlcHeader);
                    if (txRlcHeader.segFixed == NULL)
                    {
                        txRlcHeader.segFixed =
                            new LteRlcAmPduSegmentFormatFixed();
                    }
                    txRlcHeader.fixed->resegmentationFlag = TRUE;
                    if (((*nackIt).second.soStart + nackDataSize)
                        == MESSAGE_ReturnPacketSize(it->message))
                    {
                        txRlcHeader.segFixed->lastSegmentFlag = TRUE;
                    }

                    txRlcHeader.segFixed->segmentOffset
                        = (UInt16)(*nackIt).second.soStart;

                    // this message is not included header size
                    Message* dupMsg = MESSAGE_Duplicate(node, it->message);
                    int tmpSize = MESSAGE_ReturnVirtualPacketSize(dupMsg);
                    if (tmpSize > 0)
                    {
                        MESSAGE_RemoveVirtualPayload(node, dupMsg, tmpSize);
                    }
                    MESSAGE_AddVirtualPayload(node, dupMsg, nackDataSize);
                    // add header size and set to info
                    LteRlcSetMessageRlcHeaderInfo(node,
                                                  iface,
                                                  txRlcHeader,
                                                  dupMsg);

                    LteRlcDelAmPduHdrObj(txRlcHeader);

                    it->retxCount++;
                    if (it->retxCount > rlcConfig->maxRetxThreshold)
                    {
                        if (dupMsg) {
                            MESSAGE_Free(node, dupMsg);
                            dupMsg = NULL;
                        } else {
                            // do nothing
                        }
                        ret = discardPdu(
                                    node, iface, it, buf, pdu, retxPduByte);
                        return ret;
                    }

                    pdu.push_back(dupMsg);
                    retxPduByte += MESSAGE_ReturnPacketSize(dupMsg);
                    lteRlc->stats.numDataPdusSentToMacSublayer++;
                    ret = TRUE;
#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
                    lte::LteLog::InfoFormat(
                        node,
                        iface,
                        LTE_STRING_LAYER_TYPE_RLC,
                        "SendRetx,"
                        LTE_STRING_FORMAT_RNTI","
                        "%d,%d",
                        entityVar->oppositeRnti.nodeId,
                        entityVar->oppositeRnti.interfaceIndex,
                        it->rlcHeader.fixed->seqNum,
                        it->retxCount);
#endif
#endif

                    // remove Nack parts
                    retxBufNackSize -= it->getNackedSize(node, iface);
                    it->nackField.erase(nackIt++);
                    if (it->nackField.empty())
                    {
                        it->pduStatus = LTE_RLC_AM_DATA_PDU_STATUS_WAIT;
                    }
                    else
                    {
                        retxBufNackSize += it->getNackedSize(node, iface);
                    }
                    continue;
                }
                // resegment
                else
                {
                    if (restSize < headerSize + LTE_RLC_AM_DATA_PDU_MIN_BYTE)
                    {
                        return ret;
                    }
                    else
                    {
                        retxBufNackSize -= it->getNackedSize(node, iface);
                        size = restSize - headerSize;
                        restSize -= (size + headerSize);

                        LteRlcHeader txRlcHeader;
                        LteRlcCpyAmPduHdrObj(it->rlcHeader, txRlcHeader);
                        if (txRlcHeader.segFixed == NULL)
                        {
                            txRlcHeader.segFixed =
                                new LteRlcAmPduSegmentFormatFixed();
                        }
                        txRlcHeader.fixed->resegmentationFlag = TRUE;
                        if (((*nackIt).second.soStart + size)
                            == MESSAGE_ReturnPacketSize(it->message))
                        {
                            txRlcHeader.segFixed->lastSegmentFlag = TRUE;
                        }

                        txRlcHeader.segFixed->segmentOffset =
                                                    (UInt16)(*nackIt).second.soStart;

                        // this message is not included header size
                        Message* dupMsg = MESSAGE_Duplicate(
                                                        node, it->message);
                        int tmpSize = MESSAGE_ReturnVirtualPacketSize(dupMsg);
                        if (tmpSize > 0)
                        {
                            MESSAGE_RemoveVirtualPayload(node, dupMsg, tmpSize);
                        }
                        MESSAGE_AddVirtualPayload(node, dupMsg, size);
                        // add header size and set to info
                        LteRlcSetMessageRlcHeaderInfo(node,
                                                      iface,
                                                      txRlcHeader,
                                                      dupMsg);

                        LteRlcDelAmPduHdrObj(txRlcHeader);

                        it->retxCount++;
                        if (it->retxCount > rlcConfig->maxRetxThreshold)
                        {
                        if (dupMsg) {
                            MESSAGE_Free(node, dupMsg);
                            dupMsg = NULL;
                        } else {
                            // do nothing
                        }
                            ret = discardPdu(
                                    node, iface, it, buf, pdu, retxPduByte);
                            return ret;
                        }

                        pdu.push_back(dupMsg);
                        retxPduByte += MESSAGE_ReturnPacketSize(dupMsg);
                        lteRlc->stats.numDataPdusSentToMacSublayer++;
                        ret = TRUE;

#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
                        lte::LteLog::InfoFormat(
                            node,
                            iface,
                            LTE_STRING_LAYER_TYPE_RLC,
                            "SendRetx,"
                            LTE_STRING_FORMAT_RNTI","
                            "%d,%d",
                            entityVar->oppositeRnti.nodeId,
                            entityVar->oppositeRnti.interfaceIndex,
                            it->rlcHeader.fixed->seqNum,
                            it->retxCount);
#endif
#endif
                        // update OffsetStart
                        nackIt->second.soStart += size;
                        retxBufNackSize += it->getNackedSize(node, iface);

                        return ret;
                    }
                }
            }

            nackIt++;
        }
        if (it == buf.end()) {
            break;
        }
        it++;
    }

    return ret;
}

// /**
// FUNCTION::           LteRlcGetExpectedAmStatusPduSize
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            get status PDU size(byte).
// PARAMETERS::
//  + statusPdu : LteRlcStatusPduFormat& : status PDU
// RETURN::    void :         NULL
// **/
int LteRlcGetExpectedAmStatusPduSize(LteRlcStatusPduFormat& statusPdu)
{
    int sizeBit = LTE_RLC_AM_STATUS_PDU_FIXED_BIT_SIZE;

    if (statusPdu.nackSoMap.empty() != TRUE)
    {
        std::multimap < UInt16, LteRlcStatusPduSoPair >
            ::iterator it;
        for (it = statusPdu.nackSoMap.begin();
            it != statusPdu.nackSoMap.end();
            it++)
        {
            sizeBit += LTE_RLC_AM_STATUS_PDU_EXTENSION_E1_E2_BIT_SIZE;
            if ((*it).second.soStart !=
                LTE_RLC_INVALID_STATUS_PDU_FORMAT_NACK_SEGMENT_OFFSET)
            {
                sizeBit += LTE_RLC_AM_STATUS_PDU_EXTENSION_SO_BIT_SIZE;
            }
        }
    }

    return (int)(ceil((double)sizeBit / 8));
}

#ifdef LTE_RLC_BUFF_SIZE_TEST
int LteRlcAmEntity::getTestStatusSize()
{
    int size = 0;
    std::list < LteRlcStatusPduFormat > ::iterator it;
    for (it = statusBuffer.begin(); it != statusBuffer.end(); it++)
    {
        size += LteRlcGetExpectedAmStatusPduSize(*it);
    }
    return size;
}
#endif

// /**
// FUNCTION::           LteRlcEntity::getStatusBuffer
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            get status PDU from statusPDU buffer.
// PARAMETERS::
//  + node        : Node* : Node Pointer
//  + iface       : int   : Inteface index
//  + restSize    : int&  : request data size
//  + buf         : std::list<LteRlcStatusPduFormat>& : statusPDU buffer
//  + pdu         : std::list<Message*>& : RLC-PDU messages list
//  + retxPduByte : int&  : transmit statusPDU size
// RETURN::    void :         NULL
// **/
void LteRlcAmEntity::getStatusBuffer(
    Node* node,
    int iface,
    int& restSize,
    std::list < LteRlcStatusPduFormat > &buf,
    std::list < Message* > &pdu,
    int& statusPduByte)
{
    LteRlcData* lteRlc = LteLayer2GetLteRlcData(node, iface);
    std::list < LteRlcStatusPduFormat > ::iterator it = buf.begin();
    while (it != buf.end())
    {
        Message* pduMsg = NULL;
        int size = LteRlcGetExpectedAmStatusPduSize(*it);

        // Status PDU size is less than request size
        if (restSize <= size)
        {
#ifdef LTE_LIB_LOG
            lte::LteLog::InfoFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "SendStatus,"
                LTE_STRING_FORMAT_RNTI","
                "ForceSend,StatusSize(%d),RestSize(%d)",
                entityVar->oppositeRnti.nodeId,
                entityVar->oppositeRnti.interfaceIndex,
                size, restSize);
#endif
            size = restSize;
        }

        pduMsg = MESSAGE_Alloc(node, 0, 0, 0);
        ERROR_Assert(pduMsg != NULL, "RLC LTE: Out of memory!");
        MESSAGE_PacketAlloc(node,
                            pduMsg,
                            size,
                            TRACE_RLC_LTE);

        LteRlcSetMessageStatusPduFormatInfo(node,
                                            iface,
                                            pduMsg,
                                            (*it));
#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
        lte::LteLog::InfoFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "SendStatus,"
            LTE_STRING_FORMAT_RNTI","
            "ACK_SN,%d",
            entityVar->oppositeRnti.nodeId,
            entityVar->oppositeRnti.interfaceIndex,
            it->fixed.ackSn);
        std::string nackString("");
        std::multimap < UInt16, LteRlcStatusPduSoPair >
                                                    ::iterator nackItr;
        for (nackItr  = it->nackSoMap.begin();
            nackItr != it->nackSoMap.end();
            ++nackItr)
        {
            lte::LteLog::InfoFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "SendStatus,"
                LTE_STRING_FORMAT_RNTI","
                "NACK_SN,%d,%d,%d",
                entityVar->oppositeRnti.nodeId,
                entityVar->oppositeRnti.interfaceIndex,
                nackItr->first,
                nackItr->second.soStart,
                nackItr->second.soEnd);
        }

#endif
#endif
        statusBufSize -= size;
        it = buf.erase(it);
        restSize -= size;


        if (pduMsg != NULL)
        {
#ifdef ADDON_DB
            // first add stats DB info for this control packet
            LteMacAddStatsDBInfo(node, pduMsg);
            LteMacUpdateStatsDBInfo(node,
                                    pduMsg,
                                    0,
                                    MESSAGE_ReturnPacketSize(pduMsg),
                                    TRUE);
#endif
            pdu.push_back(pduMsg);
            statusPduByte += MESSAGE_ReturnPacketSize(pduMsg);
            lteRlc->stats.numAmStatusPdusSendToMacSublayer++;
        }
    }
}

// /**
// FUNCTION::           LteRlcEntity::getTxBuffer
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            get PDU from transmittion buffer.
// PARAMETERS::
//  + node        : Node* : Node Pointer
//  + iface       : int   : Inteface index
//  + restSize    : int&  : request data size
//  + buf         : std::list<LteRlcAmTxBuffer>& : transmittion buffer
//  + pdu         : std::list<Message*>& : RLC-PDU messages list
//  + retxPduByte : int&  : transmit PDU size
// RETURN::    void :         NULL
// **/
BOOL LteRlcAmEntity::getTxBuffer(Node* node,
                 int iface,
                 int& restSize,
                 std::list < LteRlcAmTxBuffer > &buf,
                 std::list < Message* > &pdu,
                 int& txPduByte)
{
    LteRlcData* lteRlc = LteLayer2GetLteRlcData(node, iface);
    BOOL ret = FALSE;
    Message* pduMsg = NULL;
    LteRlcHeader rlcHeader = LteRlcHeader(LTE_RLC_AM_PDU_TYPE_PDU);
    BOOL loopFinalIsSegment = FALSE;

    std::list < LteRlcAmTxBuffer > ::iterator it;
#ifdef ADDON_DB
    UInt32 dataSize = 0;
    UInt32 ctrlSize = 0;
#endif

#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log << "getTxBuffer:start";
        log << ",restSize,"            << restSize;
        log << ",txBufSize,"           << txBufSize;
        log << ",txBuffer.size(),"     << txBuffer.size();
        log << ",statusBuffer.size()," << statusBuffer.size();
        log << ",retxBufSize,"         << retxBufSize;
        log << ",retxBuffer.size(),"   << retxBuffer.size();
        log << ",retxBufNackSize,"     << retxBufNackSize;
        log << ",statusBufSize,"       << statusBufSize;
#ifdef LTE_RLC_BUFF_SIZE_TEST
        log << ",getTestBufferSize()," << getTestTxBufferSize();
        log << ",getTestRetxBufferSize()," << getTestRetxBufferSize();
        log << ",getTestNackedSize()," << getTestNackedSize();
        log << ",getTestStatusSize()," << getTestStatusSize();
#endif

        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG

    for (it = buf.begin(); it != buf.end();)
    {
        int size = LteRlcGetExpectedAmPduSize(node,
                                              iface,
                                              rlcHeader,
                                              FALSE,
                                              pduMsg,
                                              *it);

        // Data PDU size less than request size
        // need concatenation
        if (restSize >= size)
        {
            txBufSize -= MESSAGE_ReturnPacketSize(it->message);
            Message* rtxPduMsg = it->message;

            if (rlcHeader.firstSdu == TRUE)
            {
                // set flag & FI field
                rlcHeader.firstSdu = FALSE;
                rlcHeader.firstSduLi
                    = (UInt16)MESSAGE_ReturnPacketSize(rtxPduMsg);
                if ((rlcHeader.extension == NULL)
                    && ((*it).offset != 0))
                {
                    LteRlcAllocAmPduLi(node,
                                       iface,
                                       rlcHeader,
                                       rlcHeader.firstSduLi);
                    rlcHeader.firstSduLi = 0;
                    rlcHeader.fixed->FramingInfo
                        = LTE_RLC_AM_FRAMING_INFO_IN_END_BYTE;
                }
                else
                {
                    rlcHeader.fixed->FramingInfo
                        = LTE_RLC_AM_FRAMING_INFO_NO_SEGMENT;
                }

                concatenationMessage(node,
                                     iface,
                                     pduMsg,
                                     rtxPduMsg);
            }
            else
            {
                // add LI of first SDU
                if (rlcHeader.firstSduLi != 0)
                {
                    LteRlcAllocAmPduLi(node,
                                       iface,
                                       rlcHeader,
                                       rlcHeader.firstSduLi);
                    rlcHeader.firstSduLi = 0;
                }

                concatenation(node,
                              iface,
                              rlcHeader,
                              pduMsg,
                              rtxPduMsg);
            }

            loopFinalIsSegment = FALSE;
#ifdef ADDON_DB
            // before deleting the mesage from the list,
            // calculate the data and control info of original message
            // received from PDCP layer
            LteStatsDbSduPduInfo* info = (LteStatsDbSduPduInfo*)
                            MESSAGE_ReturnInfo(
                                     rtxPduMsg,
                                    (UInt16)INFO_TYPE_LteStatsDbSduPduInfo);
            dataSize += info->dataSize;
            ctrlSize += info->ctrlSize;
#endif
            it = buf.erase(it);
        }
        // need segmentation
        else
        {
            if (rlcHeader.firstSdu == TRUE)
            {
                // set flag & FI field
                rlcHeader.firstSdu = FALSE;
                if (it->offset == 0)
                {
                    rlcHeader.fixed->FramingInfo
                        = LTE_RLC_AM_FRAMING_INFO_NO_SEGMENT;
                }
                else
                {
                    rlcHeader.fixed->FramingInfo
                        = LTE_RLC_AM_FRAMING_INFO_IN_END_BYTE;
                }
            }
            // add LI of first SDU
            if (rlcHeader.firstSduLi != 0)
            {
                LteRlcAllocAmPduLi(node,
                                   iface,
                                   rlcHeader,
                                   rlcHeader.firstSduLi);
                rlcHeader.firstSduLi = 0;
            }

            // calculate remaining size which is enough to segment
            size = LteRlcGetAmPduListDataSize(node,
                                              iface,
                                              rlcHeader,
                                              pduMsg);
            size += LteRlcGetExpectedAmPduHeaderSize(node,
                                                    iface,
                                                    rlcHeader,
                                                    FALSE);
            if (rlcHeader.extension == NULL)
            {
                size += LTE_RLC_AM_DATA_PDU_FORMAT_EXTENSION_MIN_BYTE_SIZE;
            }

            restSize -= size;
            if (restSize > LTE_RLC_AM_DATA_PDU_MIN_BYTE)
            {
                Message* fragHead = MESSAGE_Alloc(node, 0, 0, 0);
                Message* fragTail = MESSAGE_Alloc(node, 0, 0, 0);

                BOOL divisonFlag = FALSE;
                divisonFlag = divisionMessage(node,
                                              iface,
                                              restSize,
                                              it->message,
                                              fragHead,
                                              fragTail,
                                              TRACE_RLC_LTE,
                                              TRUE);
                if (divisonFlag == TRUE)
                {
#ifdef ADDON_DB
                    // setting/resetting stats-db info values
                    UInt32 dataSizeHead = 0;
                    UInt32 ctrlSizeHead = 0;

                    LteMacSetStatsDBInfoInFragMsg(node,
                                                  NULL,
                                                  fragHead,
                                                  fragTail,
                                                  restSize,
                                                  dataSizeHead,
                                                  ctrlSizeHead);
                    dataSize += dataSizeHead;
                    ctrlSize += ctrlSizeHead;
#endif
                    concatenation(node,
                                  iface,
                                  rlcHeader,
                                  pduMsg,
                                  fragHead);
                    // update SDU -> SDUsegment
                    it->message = fragTail;
                    it->offset += MESSAGE_ReturnPacketSize(fragHead);
                    txBufSize -= MESSAGE_ReturnPacketSize(fragHead);
                    restSize = 0;
                }
                else
                {
                    MESSAGE_Free(node, fragHead);
                    MESSAGE_Free(node, fragTail);

                    ERROR_Assert(FALSE, "The SDU size check has mistaken.");
                }
                loopFinalIsSegment = TRUE;
            }

            break;
        }
    }

    if (loopFinalIsSegment == TRUE)
    {
        // segment & set FI
        if (rlcHeader.fixed->FramingInfo
            & LTE_RLC_AM_FRAMING_INFO_CHK_FIRST_BYTE)
        {
            rlcHeader.fixed->FramingInfo
                = LTE_RLC_AM_FRAMING_INFO_FRAGMENT;
        }
        else
        {
            rlcHeader.fixed->FramingInfo
                = LTE_RLC_AM_FRAMING_INFO_IN_FIRST_BYTE;
        }
    }

    if (pduMsg != NULL)
    {
#ifdef LTE_RLC_MSG_TEST
        Message* msg1 = pduMsg;
        while (msg1 != NULL)
        {
            MESSAGE_logMessage(node,
                               iface,
                               msg1,
                               LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
                               "getTxBuffer(before serialize)");
            msg1 = msg1->next;
        }
#endif // LTE_RLC_MSG_TEST

#ifdef ADDON_DB
        // Now before making RLC PDU, calculate RLC header size
        ctrlSize += LteRlcGetAmPduHeaderSize(node, iface, rlcHeader);
#endif

        int rlcSn = this->getSndSn();
#ifdef LTE_LIB_LOG
        stringstream log;
        stringstream commonLog;
        commonLog << rlcSn << "(";
#endif // LTE_LIB_LOG

        // set unreceived RLC ACK map
        // in oreder to notify PDCP of received PDCP ACK
        Message* current = pduMsg;
        while (current) {
            unreceivedRlcAckList.push_back(pair<UInt16, UInt16>(
                        (UInt16)rlcSn, 
                        (UInt16)((LteRlcAmPdcpPduInfo*) MESSAGE_ReturnInfo(
                            current,
                            INFO_TYPE_LteRlcAmPdcpPduInfo))->pdcpSn));
#ifdef LTE_LIB_LOG
            commonLog << ((LteRlcAmPdcpPduInfo*) MESSAGE_ReturnInfo(
                        current, INFO_TYPE_LteRlcAmPdcpPduInfo))->pdcpSn
                << " ";
#endif // LTE_LIB_LOG
            current = current->next;
        }
#ifdef LTE_LIB_LOG
        {
            commonLog << "),->[";

            UInt16 debugRlcSn = LTE_RLC_INVALID_RLC_SN;
            list<pair<UInt16, UInt16> >::const_iterator debugItr
                = unreceivedRlcAckList.begin();
            list<pair<UInt16, UInt16> >::const_iterator debugItrEnd
                = unreceivedRlcAckList.end();
            log << debugItr->first << "(" << debugItr->second << " ";
            debugRlcSn = debugItr->first;
            ++debugItr;
            while (debugItr != debugItrEnd) {
                if (debugRlcSn != debugItr->first) {
                    log << ")]";
                    lte::LteLog::InfoFormat(node, iface,
                            LTE_STRING_LAYER_TYPE_RLC,
                            "setUnRxRlcAck,"
                            LTE_STRING_FORMAT_RNTI","
                            "%s%s",
                            entityVar->oppositeRnti.nodeId,
                            entityVar->oppositeRnti.interfaceIndex,
                            commonLog.str().c_str(),
                            log.str().c_str());
                    log.str("");
                    log << debugItr->first << "(" << debugItr->second << " ";
                } else {
                    log << debugItr->second << " ";
                }
                debugRlcSn = debugItr->first;
                ++debugItr;
            }
            log << ")]";
            lte::LteLog::InfoFormat(node, iface,
                    LTE_STRING_LAYER_TYPE_RLC,
                    "setUnRxRlcAck,"
                    LTE_STRING_FORMAT_RNTI","
                    "%s%s",
                    entityVar->oppositeRnti.nodeId,
                    entityVar->oppositeRnti.interfaceIndex,
                    commonLog.str().c_str(),
                    log.str().c_str());
        }
#endif // LTE_LIB_LOG

        Message* newMsg = LteRlcSduListToPdu(node, pduMsg);

#ifdef ADDON_DB
        // add stats-DB info on the final PDU mesage
        LteMacAddStatsDBInfo(node, newMsg);
        LteMacUpdateStatsDBInfo(node,
                                newMsg,
                                dataSize,
                                ctrlSize,
                                FALSE);
#endif
        rlcHeader.LteRlcSetPduSeqNo(rlcSn);

        Message* dupMsg = MESSAGE_Duplicate(node, newMsg);

        LteRlcAmReTxBuffer retx;
        retx.store(node, iface, rlcHeader, dupMsg);
        retxBuffer.push_back(retx);
        retxBufSize += MESSAGE_ReturnPacketSize(dupMsg);

        if (latestPdu.message != NULL)
        {
            LteRlcDelAmPduHdrObj(latestPdu.rlcHeader);
            MESSAGE_FreeList(entityVar->node, latestPdu.message);
            latestPdu.message = NULL;
        }
        LteRlcCpyAmPduHdrObj(rlcHeader, latestPdu.rlcHeader);
        latestPdu.message = MESSAGE_Duplicate(node, dupMsg);

        // Polling function.
        polling(node, iface, rlcHeader, dupMsg);

        LteRlcSetMessageRlcHeaderInfo(node,
                                      iface,
                                      rlcHeader,
                                      newMsg);

        pdu.push_back(newMsg);
        txPduByte += MESSAGE_ReturnPacketSize(newMsg);
        lteRlc->stats.numDataPdusSentToMacSublayer++;

#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
        lte::LteLog::InfoFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "SendTx,"
            LTE_STRING_FORMAT_RNTI","
            "%d",
            entityVar->oppositeRnti.nodeId,
            entityVar->oppositeRnti.interfaceIndex,
            rlcHeader.fixed->seqNum);
#endif
#endif

        ret = TRUE;
    }
    else
    {
        LteRlcDelAmPduHdrObj(rlcHeader);

        ret = FALSE;
    }
#ifdef LTE_RLC_BUFF_SIZE_TEST
#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log << "getTxBuffer:end";
        log << ",restSize,"            << restSize;
        log << ",txBufSize,"           << txBufSize;
        log << ",txBuffer.size(),"     << txBuffer.size();
        log << ",statusBuffer.size()," << statusBuffer.size();
        log << ",retxBufSize,"         << retxBufSize;
        log << ",retxBuffer.size(),"   << retxBuffer.size();
        log << ",retxBufNackSize,"     << retxBufNackSize;
        log << ",statusBufSize,"       << statusBufSize;
#ifdef LTE_RLC_BUFF_SIZE_TEST
        log << ",getTestBufferSize()," << getTestTxBufferSize();
        log << ",getTestRetxBufferSize()," << getTestRetxBufferSize();
        log << ",getTestNackedSize()," << getTestNackedSize();
        log << ",getTestStatusSize()," << getTestStatusSize();
#endif

        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG
#endif // LTE_RLC_BUFF_SIZE_TEST

    return ret;
}

// /**
// FUNCTION::           LteRlcEntity::polling
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            polling function.
// PARAMETERS::
//  + node      : Node*         : Node Pointer
//  + iface     : int           : Inteface index
//  + rlcHeader : LteRlcHeader& : RLC header data
//  + newMsg    : Message*      : RLC-PDU messages list
// RETURN::    void :         NULL
// **/
void LteRlcAmEntity::polling(Node* node,
                             int iface,
                             LteRlcHeader& rlcHeader,
                             Message* newMsg)
{
    LteRlcConfig* rlcConfig = GetLteRlcConfig(node, iface);
    std::list < LteRlcAmReTxBuffer > ::iterator it;

    pduWithoutPoll += 1;
    byteWithoutPoll += LteRlcGetAmPduSize(node,
                                          iface,
                                          rlcHeader,
                                          newMsg);
    if ((pduWithoutPoll >= (UInt32)rlcConfig->pollPdu)
        || (byteWithoutPoll >= (UInt32)rlcConfig->pollByte) )
    {
        rlcHeader.fixed->pollingBit = TRUE;
        pduWithoutPoll = 0;
        byteWithoutPoll = 0;
        pollSn = sndNextSn;
        LteRlcAmDecSN(pollSn);
#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
        lte::LteLog::InfoFormat(
            node, iface, LTE_STRING_LAYER_TYPE_RLC,
            "PollingTxBuffer,"
            LTE_STRING_FORMAT_RNTI","
            "%d",
            entityVar->oppositeRnti.nodeId,
            entityVar->oppositeRnti.interfaceIndex,
            pollSn);
#endif
#endif
        resetExpriory_t_PollRetransmit();
        if (pollRetransmitTimerMsg != NULL)
        {
            MESSAGE_CancelSelfMsg(node, pollRetransmitTimerMsg);
            pollRetransmitTimerMsg = NULL;
        }

        LteRlcAmInitPollRetrunsmitTimer(node,
                                        iface,
                                        this);
    }
#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmEntity::polling:";
        log << ",PDU_WITHOUT_POLL," << pduWithoutPoll;
        log << ",BYTE_WITHOUT_POLL," << byteWithoutPoll;
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_CONTROL,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG
}

// /**
// FUNCTION::           LteRlcEntity::polling
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            get PDU from transmittion buffer.
// PARAMETERS::
//  + node            : Node* : Node Pointer
//  + iface           : int   : Inteface index
//  + restSize        : int&  : request data size
//  + makedPdu        : BOOL  : flag of making PDU
//  + makedPduSegment : BOOL  : flag of making PDUsegment
//  + pdu             : std::list<Message*>& : RLC-PDU messages list
//  + pollPduByte     : int&  : transmit PDU size
// RETURN::    void :         NULL
// **/
void LteRlcAmEntity::polling(Node* node,
             int iface,
             int& restSize,
             BOOL makedPdu,
             BOOL makedPduSegment,
             std::list < Message* > &pdu,
             int& pollPduByte)
{
    LteRlcData* lteRlc = LteLayer2GetLteRlcData(node, iface);
    BOOL poolFlag = chkExpriory_t_PollRetransmit();

    // "not dequeued from tx&retx buffer" and
    // "Expriory-t-PollRetransmit" is expired
    if ((makedPdu != TRUE) && (makedPduSegment != TRUE)
        && (poolFlag == TRUE) )
    {
        UInt32 latestSn = sndNextSn;
        LteRlcAmDecSN(latestSn);

        if (latestPdu.message == NULL)
        {
            ERROR_ReportError("LTE-RLC: Not found sendable PDU.");
        }
        else
        {
            LteRlcHeader txRlcHeader;
            LteRlcCpyAmPduHdrObj(latestPdu.rlcHeader, txRlcHeader);
            Message* dupMsg = MESSAGE_Duplicate(node, latestPdu.message);

            txRlcHeader.fixed->pollingBit = TRUE;
            pduWithoutPoll = 0;
            byteWithoutPoll = 0;
            pollSn = latestSn;
#ifdef LTE_LIB_LOG
            {
                std::stringstream log;
                log.str("");
                log << "LteRlcAmEntity::polling:PollRetransmit";
                log << ",POLL_SN," << pollSn;
                lte::LteLog::DebugFormat(
                    node,
                    iface,
                    LTE_STRING_LAYER_TYPE_RLC,
                    "%s,%s",
                    LTE_STRING_CATEGORY_TYPE_RLC_CONTROL,
                    log.str().c_str());
            }
#endif // LTE_LIB_LOG

            resetExpriory_t_PollRetransmit();
            if (pollRetransmitTimerMsg != NULL)
            {
                MESSAGE_CancelSelfMsg(node, pollRetransmitTimerMsg);
                pollRetransmitTimerMsg = NULL;
            }
            LteRlcAmInitPollRetrunsmitTimer(node,
                                            iface,
                                            this);


            LteRlcSetMessageRlcHeaderInfo(node,
                                          iface,
                                          txRlcHeader,
                                          dupMsg);

            LteRlcDelAmPduHdrObj(txRlcHeader);

            pdu.push_back(dupMsg);
            pollPduByte += MESSAGE_ReturnPacketSize((dupMsg));
            lteRlc->stats.numDataPdusSentToMacSublayer++;

#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
            lte::LteLog::InfoFormat(
                node, iface, LTE_STRING_LAYER_TYPE_RLC,
                "PollingNotSendPdu,"
                LTE_STRING_FORMAT_RNTI","
                "%d",
                entityVar->oppositeRnti.nodeId,
                entityVar->oppositeRnti.interfaceIndex,
                pollSn);
#endif
#endif
        }
    }
}

// /**
// FUNCTION::           LteRlcEntity::concatenationMessage
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            concatenation message.
// PARAMETERS::
//  + node            : Node*     : Node Pointer
//  + iface           : int       : Inteface index
//  + prev            : Message*& : messages list (for base)
//  + next            : Message*  : messages list
// RETURN::    void :         NULL
// **/
void LteRlcAmEntity::concatenationMessage(Node* node,
                                          int iface,
                                          Message*& prev,
                                          Message* next)
{
    if (prev == NULL)
    {
        prev = next;
    }
    else
    {
        Message* org = prev;
        while (prev->next != NULL)
        {
            prev = prev->next;
        }
        prev->next = next;
        prev = org;
    }
}

// /**
// FUNCTION::           LteRlcEntity::concatenation
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            concatenation message, and add LI-info.
// PARAMETERS::
//  + node      : Node*         : Node Pointer
//  + iface     : int           : Inteface index
//  + rlcHeader : LteRlcHeader& : RLC header data
//  + prev      : Message*&     : messages list (for base)
//  + next      : Message*      : messages list
// RETURN::    void :         NULL
// **/
void LteRlcAmEntity::concatenation(Node* node,
                                   int iface,
                                   LteRlcHeader& rlcHeader,
                                   Message*& prev,
                                   Message* next)
{
    //Message
    concatenationMessage(node,
                         iface,
                         prev,
                         next);

    LteRlcAddAmPduLi(node,
                     iface,
                     rlcHeader,
                     next);

}

// /**
// FUNCTION   :: LteRlcAmEntity::getSndSn
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: get next VT(S).
// RETURN     :: int     : next VT(S)
// **/
int LteRlcAmEntity::getSndSn()
{
    int sndSn = sndNextSn;
    sndNextSn++;
    sndNextSn = sndNextSn % LTE_RLC_AM_SN_UPPER_BOUND;

    return sndSn;
}

// /**
// FUNCTION   :: LteRlcAmEntity::txWindowCheck
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: check SN, inside of window.
// RETURN     :: BOOL
// **/
BOOL LteRlcAmEntity::txWindowCheck()
{
    int sn = ackSn % LTE_RLC_AM_SN_UPPER_BOUND;
    if ((0 <= sn) && (sn < LTE_RLC_AM_WINDOW_SIZE))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION::           LteRlcEntity::divisionMessage
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            division message. (into two parts)
// PARAMETERS::
//  + node         : Node*             : Node Pointer
//  + iface        : int               : Inteface index
//  + offset       : int               : offset of dividing position
//  + msg          : Message*          : messages(orignal)
//  + fragHead     : Message*          : messages(divided first-part)
//  + fragTail     : Message*          : messages(divided second-part)
//  + protocolType : TraceProtocolType : add TraceProtocolType
//  + msgFreeFlag  : BOOL              : orignal message free
// RETURN::    void :         NULL
// **/
BOOL LteRlcAmEntity::divisionMessage(Node* node,
                                     int iface,
                                     int offset,
                                     Message* msg,
                                     Message* fragHead,
                                     Message* fragTail,
                                     TraceProtocolType protocolType,
                                     BOOL msgFreeFlag)
{
    int pktSize = MESSAGE_ReturnPacketSize(msg);

    if (pktSize <= offset)
    {
        return FALSE;
    }
    else
    {
        int realPayloadSize = MESSAGE_ReturnActualPacketSize(msg);
        int virtualPayloadSize = MESSAGE_ReturnVirtualPacketSize(msg);
        int realIndex = 0;
        int virtualIndex = 0;
        int fragRealSize;
        int fragVirtualSize;
        int i;

        int numFrags = 2;
        int divSize[2];

        divSize[0] = offset;
        divSize[1] = realPayloadSize + virtualPayloadSize - offset;

        Message* fragMsg[2] = {fragHead, fragTail};

        for (i = 0; i < numFrags; i ++)
        {
            // copy info fields
            MESSAGE_CopyInfo(node, fragMsg[i], msg);

            fragRealSize = realPayloadSize - realIndex;
            if ((fragRealSize >= divSize[i]) && i==0)
            {
                fragRealSize = divSize[i];
                fragVirtualSize = 0;
            }
            else
            {
                fragVirtualSize = virtualPayloadSize - virtualIndex;
                if (fragVirtualSize + fragRealSize > divSize[i])
                {
                    fragVirtualSize = divSize[i] - fragRealSize;
                }
            }

            if (fragRealSize > 0)
            {
                MESSAGE_PacketAlloc(node,
                                    fragMsg[i],
                                    fragRealSize,
                                    protocolType);
                memcpy(MESSAGE_ReturnPacket(fragMsg[i]),
                       MESSAGE_ReturnPacket(msg) + realIndex,
                       fragRealSize);
            }
            else
            {
                MESSAGE_PacketAlloc(node, fragMsg[i], 0, protocolType);
            }

            if (fragVirtualSize > 0)
            {
                MESSAGE_AddVirtualPayload(node, fragMsg[i], fragVirtualSize);
            }

            realIndex += fragRealSize;
            virtualIndex += fragVirtualSize;
        }

        // copy other simulation specific information to the first fragment
        fragMsg[0]->sequenceNumber = msg->sequenceNumber;
        fragMsg[0]->originatingProtocol = msg->originatingProtocol;
        fragMsg[0]->numberOfHeaders = msg->numberOfHeaders;

        for (i = 0; i < msg->numberOfHeaders; i ++)
        {
            fragMsg[0]->headerProtocols[i] = msg->headerProtocols[i];
            fragMsg[0]->headerSizes[i] = msg->headerSizes[i];
        }
        if (msgFreeFlag)
        {
            MESSAGE_Free(node, msg);
        }
    }
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////
// Class LteRlcUmAmFragment member functions
///////////////////////////////////////////////////////////////////////////

// /**
// FUNCTION::           LteRlcAmPduSegmentReassemble
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Insert the PDU segments extracted
//                      from the newly received PDU segment
//                      into the PDU list (receiving buffer).
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         The interface index
//      +amEntity:      The AM entity
//      +rcvdSeqNum:    The seqence number of the PDU, from which
//                      the sdu segments are extracted
//      +pduMsg:        The received PDU
// RETURN::             BOOL
// **/
static
BOOL LteRlcAmPduSegmentReassemble(
    Node* node,
    int iface,
    LteRlcAmEntity* amEntity,
    unsigned int rcvdSeqNum,
    Message* pduMsg)
{
    BOOL doReassemble = TRUE;

    // Get current RLC-PDUsegment data info.
    LteRlcAmRcvPduData* currentPduData
        = &(amEntity->rcvPduBuffer[rcvdSeqNum]);

    int pduSegDataSize = MESSAGE_ReturnPacketSize(pduMsg)
                         - LteRlcAmGetHeaderSize(pduMsg);

    LteRlcAmRcvPduSegmentPart currentSoInfo;
    currentSoInfo.rcvdHead = LteRlcAmGetSegOffset(pduMsg);
    currentSoInfo.rcvdTail = currentSoInfo.rcvdHead + (UInt16)pduSegDataSize;

    // check the overlap RLC-PDUsegments.
    // CH:currentSoInfo.rcvdHead, CT:currentSoInfo.rcvdTail
    // IH:it->second.rcvdHead, IT:it->second.rcvdTail
    std::map < UInt16, LteRlcAmRcvPduSegmentPart > ::iterator it
        = currentPduData->rcvdPduSegmentInfo.begin();
    while (it != currentPduData->rcvdPduSegmentInfo.end())
    {
        if (currentSoInfo.rcvdHead < it->second.rcvdHead)
        {
            if (currentSoInfo.rcvdTail >= it->second.rcvdHead)
            {
                if (currentSoInfo.rcvdTail <= it->second.rcvdTail)
                {
                    // CurHead < ItHead < CurTail < ItTail
                    // -> CurHead ~ ItTail
                    currentSoInfo.rcvdTail = it->second.rcvdTail;
                }

                currentPduData->rcvdPduSegmentInfo.erase(it++);
                continue;
            }
        }
        else // if (currentSoInfo.rcvdHead >= it->second.rcvdHead)
        {
            if (currentSoInfo.rcvdHead <= it->second.rcvdTail)
            {
                if (currentSoInfo.rcvdTail >= it->second.rcvdTail)
                {
                    // ItHead < CurHead < ItTail < CurTail
                    // -> ItHead ~ CurTail
                    currentSoInfo.rcvdHead = it->second.rcvdHead;
                    currentPduData->rcvdPduSegmentInfo.erase(it++);
                    continue;
                }
                else
                {
                    // ItHead < CurHead < CurTail < ItTail
                    // -> already received
                    doReassemble = FALSE;
                }
            }
        }
        ++it;
    }


    // Update end position of RLC-PDU.
    if (currentPduData->endPduPos < currentSoInfo.rcvdTail)
    {
        currentPduData->endPduPos = currentSoInfo.rcvdTail;
    }

    // Update received RLC-PDUsegments information.
    if (doReassemble)
    {
        currentPduData->rcvdPduSegmentInfo[currentSoInfo.rcvdHead]
            = currentSoInfo;
    }

    // received LastSegmentFlag -> Later, check reassembling completion.
    if (LteRlcAmGetLastSegflag(pduMsg))
    {
        currentPduData->gotLsf = TRUE;
    }

    if ((currentPduData->gotLsf == TRUE)
        && (currentPduData->rcvdPduSegmentInfo.size() == 1)
        && (currentPduData->rcvdPduSegmentInfo.find(0)
            != currentPduData->rcvdPduSegmentInfo.end())
        && doReassemble == TRUE)
    {
        ERROR_Assert(currentPduData->message == NULL,
            "LteRlcAmPduSegmentReassemble: current message is not NULL!");
        currentPduData->rlcPduRcvFinished = TRUE;
        currentPduData->message = pduMsg;
#ifdef LTE_LIB_LOG
        {
            std::stringstream log;
            log.str("");
            log << "LteRlcAmPduSegmentReassemble:";
            log << ",end of PDUsegment assy.,";
            log << ",PDU-SeqNum=," << rcvdSeqNum;
            lte::LteLog::DebugFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "%s,%s",
                LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                log.str().c_str());
        }
#endif // LTE_LIB_LOG
    }

    return doReassemble;
}

// /**
// FUNCTION::           LteRlcAmAddSduBuffer
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Insert the SDU to buffer for send PDCP.
// PARAMETERS::
//      +node:       Node pointer
//      +iface:      The interface index
//      +amEntity:   The AM receiving entity
//      +seqNum:     Sequens number
//      +rcvMsgList: The message list to be inserted
// RETURN::             NULL
// **/
static
void LteRlcAmAddSduBuffer(Node* node,
                          int iface,
                          LteRlcAmEntity* amEntity,
                          unsigned int seqNum,
                          std::list < Message* > rcvMsgList)
{
    std::list < Message* > ::iterator it = rcvMsgList.begin();
    while (it != rcvMsgList.end())
    {
        std::pair < unsigned int, Message* > sdu(seqNum, (*it));
        amEntity->rcvRlcSduList.insert(sdu);
#ifdef LTE_LIB_LOG
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,"
            "InsertToSduList,%d",
            LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
            seqNum);
#endif
        ++it;
    }
}

// /**
// FUNCTION::           LteRlcAmSDUsegmentReassemble
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Insert the SDU segments extracted from
//                      the newly received PDU
//                      into the SDU segment list (receiving buffer).
//                      After this,
//                      re-assembly shall take place if possible.
// PARAMETERS::
//      +node:           Node pointer
//      +iface:          The interface index
//      +LteRlcAmEntity: The AM receiving entity
// RETURN::             NULL
// **/
static
void LteRlcAmSDUsegmentReassemble(
    Node* node,
    int iface,
    LteRlcAmEntity* amEntity)
{
#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmSDUsegmentReassemble:[before],";
        log << ",numSDUseg(first part)=,";
        log << amEntity->rcvRlcSduSegList.size();
        log << ",numSDUseg(scond or later part)=,";
        log << amEntity->rcvRlcSduTempList.size();
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG

    // get SN which is less than rcvNext
    // -> reasemble rcvRlcPdu
    std::map < unsigned int, LteRlcAmRcvPduData > ::iterator
        itPduBuff = amEntity->rcvPduBuffer.begin();
    while (itPduBuff != amEntity->rcvPduBuffer.end())
    {
        if ((LteRlcAmSeqNumInWnd(itPduBuff->first,
                                    amEntity->rcvNextSn))
            || (itPduBuff->second.rlcPduRcvFinished == FALSE))
        {
            ++itPduBuff;
            continue;
        }

        // Reassemble SDU/SDUsegment from received PDU
        std::list < Message* > rcvMsgList;
        LteRlcPduToSduList(node,
                           itPduBuff->second.message,
                           TRUE,
                           rcvMsgList);

#ifdef LTE_RLC_MSG_TEST
        if (rcvMsgList.empty() != TRUE)
        {
            std::list < Message* > ::iterator it = rcvMsgList.begin();
            while (it != rcvMsgList.end())
            {
                MESSAGE_logMessage(
                    node,
                    iface,
                    *it,
                    LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                    "LteRlcAmSDUsegmentReassemble(from MAC->unserialize)");
                ++it;
            }
        }
#endif // LTE_RLC_MSG_TEST

        // Number of Li is zero -> SDU.
        if (itPduBuff->second.lengthIndicator.empty())
        {
            LteRlcAmAddSduBuffer(node,
                                 iface,
                                 amEntity,
                                 itPduBuff->first,
                                 rcvMsgList);
        }
        // Number of Li is one.
        else if (itPduBuff->second.lengthIndicator.size() == 1)
        {
            ERROR_Assert(rcvMsgList.size() > 0, "rcvMsgList is empty!");
            LteRlcSduSegment workSdu;
            workSdu.seqNum = (Int16)itPduBuff->first;
            workSdu.FramingInfo = itPduBuff->second.FramingInfo;
            workSdu.newMsg = rcvMsgList.front();
            // FramingInfo is "IN_FIRST_BYTE"
            // -> SDUsegment(Head).
            if (itPduBuff->second.FramingInfo
                == LTE_RLC_AM_FRAMING_INFO_IN_FIRST_BYTE)
            {
                amEntity->rcvRlcSduSegList.push_back(workSdu);
            }
            // FramingInfo is "IN_END_BYTE" or "FRAGMENT"
            // -> SDUsegment(tail), so puts on the temporary buffer.
            else
            {
                amEntity->rcvRlcSduTempList.push_back(workSdu);
            }
        }
        // Number of Li is two or more.
        else
        {
            LteRlcSduSegment workSdu;
            workSdu.seqNum = (Int16)itPduBuff->first;
            workSdu.FramingInfo = itPduBuff->second.FramingInfo;
            // FramingInfo is "IN_FIRST_BYTE" or "FRAGMENT",
            // and last element.
            // -> SDUsegment(Head).
            if ((itPduBuff->second.FramingInfo
                    == LTE_RLC_AM_FRAMING_INFO_IN_FIRST_BYTE)
                || (itPduBuff->second.FramingInfo
                    == LTE_RLC_AM_FRAMING_INFO_FRAGMENT))
            {
                ERROR_Assert(rcvMsgList.size() > 0, "rcvMsgList is empty!");
                workSdu.newMsg = rcvMsgList.back();
                workSdu.FramingInfo = LTE_RLC_AM_FRAMING_INFO_IN_FIRST_BYTE;
                amEntity->rcvRlcSduSegList.push_back(workSdu);
                rcvMsgList.pop_back();
            }
            // FramingInfo is "IN_END_BYTE" or "FRAGMENT",
            // and first element.
            // -> SDUsegment(tail), so puts on the temporary buffer.
            if ((itPduBuff->second.FramingInfo
                    == LTE_RLC_AM_FRAMING_INFO_IN_END_BYTE)
                || (itPduBuff->second.FramingInfo
                    == LTE_RLC_AM_FRAMING_INFO_FRAGMENT))
            {
                ERROR_Assert(rcvMsgList.size() > 0, "rcvMsgList is empty!");
                workSdu.newMsg = rcvMsgList.front();
                workSdu.FramingInfo = LTE_RLC_AM_FRAMING_INFO_IN_END_BYTE;
                amEntity->rcvRlcSduTempList.push_back(workSdu);
                rcvMsgList.pop_front();
            }

            // The remaining element is SDU.
            if (!rcvMsgList.empty())
            {
                LteRlcAmAddSduBuffer(node,
                                     iface,
                                     amEntity,
                                     itPduBuff->first,
                                     rcvMsgList);
            }
        }
        amEntity->rcvPduBuffer.erase(itPduBuff++);
    }
    // Reassemble SDUsegment
    std::list < LteRlcSduSegment > ::iterator itSduSeg;
    std::list < LteRlcSduSegment > ::iterator itSduTemp;

    for (itSduSeg = amEntity->rcvRlcSduSegList.begin();
         itSduSeg != amEntity->rcvRlcSduSegList.end();)
    {
        BOOL eraseItSduSeg = FALSE;
        for (itSduTemp = amEntity->rcvRlcSduTempList.begin();
             itSduTemp != amEntity->rcvRlcSduTempList.end();)
        {
            BOOL eraseItSduTemp = FALSE;
            UInt32 reAssySn = itSduSeg->seqNum;
            LteRlcAmIncSN(reAssySn);
            if (reAssySn == (UInt32)itSduTemp->seqNum)
            {
                Message* flagList[2]
                    = {itSduSeg->newMsg, itSduTemp->newMsg};
                Message* reAssembleMsg
                    = MESSAGE_ReassemblePacket(node,
                                               flagList,
                                               2,
                                               TRACE_RLC_LTE);
                itSduSeg->newMsg = reAssembleMsg;
                if (itSduTemp->FramingInfo
                    == LTE_RLC_AM_FRAMING_INFO_FRAGMENT)
                {
                    itSduSeg->seqNum = itSduTemp->seqNum;
                }
                else
                {
                    std::pair < unsigned int, Message* >
                        sdu(itSduSeg->seqNum, reAssembleMsg);
                    amEntity->rcvRlcSduList.insert(sdu);
#ifdef LTE_LIB_LOG
                    lte::LteLog::DebugFormat(
                        node,
                        iface,
                        LTE_STRING_LAYER_TYPE_RLC,
                        "%s,"
                        "InsertToSduList,%d",
                        LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                        itSduSeg->seqNum);
#endif
                    eraseItSduSeg = TRUE;
                }
                eraseItSduTemp = TRUE;
            }

            if (eraseItSduTemp)
            {
                itSduTemp = amEntity->rcvRlcSduTempList.erase(itSduTemp);
            }
            else
            {
                ++itSduTemp;
            }
        }

        if (eraseItSduSeg)
        {
            itSduSeg = amEntity->rcvRlcSduSegList.erase(itSduSeg);
        }
        else
        {
            ++itSduSeg;
        }
    }
    // clean-up SDU list
    for (itSduSeg = amEntity->rcvRlcSduSegList.begin();
         itSduSeg != amEntity->rcvRlcSduSegList.end();)
    {
        UInt32 keepSn = amEntity->rcvNextSn;
        LteRlcAmDecSN(keepSn);
        if ((LteRlcAmSeqNumInWnd(itSduSeg->seqNum,
                                    amEntity->rcvNextSn) == FALSE)
            && (itSduSeg->seqNum != (Int16)keepSn))
        {
            itSduSeg = amEntity->rcvRlcSduSegList.erase(itSduSeg);
        }
        else
        {
            ++itSduSeg;
        }
    }
    for (itSduTemp = amEntity->rcvRlcSduTempList.begin();
         itSduTemp != amEntity->rcvRlcSduTempList.end();)
    {
        if (LteRlcAmSeqNumInWnd(itSduTemp->seqNum,
                                amEntity->rcvNextSn) == FALSE)
        {
            itSduTemp = amEntity->rcvRlcSduTempList.erase(itSduTemp);
        }
        else
        {
            ++itSduTemp;
        }
    }

#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmSDUsegmentReassemble:[after]";
        log << ",numSDUseg(first part)=,";
        log << amEntity->rcvRlcSduSegList.size();
        log << ",numSDUseg(scond or later part)=,";
        log << amEntity->rcvRlcSduTempList.size();
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG
}

// /**
// FUNCTION::           LteRlcSDUsegmentReassemble
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Insert the SDU segments extracted
//                      from the newly received PDU
//                      into the SDU segment list (receiving buffer).
//                      After this,
//                      re-assembly shall take place if possible.
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         The interface index
//      +rlcEntity:     The RLC entity
// RETURN::             NULL
// **/
static
void LteRlcSDUsegmentReassemble(
    Node* node,
    int iface,
    LteRlcEntity* rlcEntity)
{
    LteRlcEntityType mode = rlcEntity->entityType;
    if (mode == LTE_RLC_ENTITY_AM)
    {
        LteRlcAmSDUsegmentReassemble(
            node,
            iface,
            (LteRlcAmEntity*)rlcEntity->entityData);
    }
}


// /**
// FUNCTION::       LteRlcAmCreateStatusPdu
// LAYER::          LTE LAYER2 RLC
// PURPOSE::        Create a STATUS PDU message
//                  and stack to the status buffer.
// PARAMETERS::
//      +node:      The node pointer
//      +iface:     The interface index
//      +amEntity:  The AM entity
// RETURN::         NULL
// **/
static
void LteRlcAmCreateStatusPdu(
    Node* node,
    int iface,
    LteRlcAmEntity* amEntity)
{
#ifdef LTE_LIB_LOG
    std::stringstream seqNumLog;
#endif
    if (amEntity->statusProhibitTimerMsg != NULL)
    {
#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
        lte::LteLog::InfoFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "CreateStatusPdu,"
            LTE_STRING_FORMAT_RNTI","
            "But t-StatusProhibit is runnning.",
            amEntity->entityVar->oppositeRnti.nodeId,
            amEntity->entityVar->oppositeRnti.interfaceIndex);
#endif
#endif
        amEntity->waitExprioryTStatusProhibitFlg = TRUE;
    }
    else
    {
#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
        lte::LteLog::InfoFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "CreateStatusPdu,"
            LTE_STRING_FORMAT_RNTI,
            amEntity->entityVar->oppositeRnti.nodeId,
            amEntity->entityVar->oppositeRnti.interfaceIndex);
#endif
#endif
        LteRlcStatusPduFormat statusPdu;
        std::map < unsigned int, LteRlcAmRcvPduData > ::iterator
            itRlcPduBuff;

        UInt32 chkNum = 0;
        BOOL isMaxStatusSnLessThanRcvNextSn =
            LteRlcSeqNumLessChkInWnd(
                amEntity->maxStatusSn,
                amEntity->rcvNextSn,
                amEntity->rcvNextSn,
                amEntity->rcvWnd,
                LTE_RLC_AM_SN_UPPER_BOUND);
        // NACK settings VR(R):rcvNextSn ~ VR(MS):maxStatusSn
        chkNum = LteRlcAmSeqNumDist(amEntity->rcvNextSn,
                                           amEntity->maxStatusSn) + 1;
        BOOL isExistRlcPduSeg = FALSE;
        UInt32 lastRlcPduSegSn = amEntity->rcvNextSn;
        for (UInt32 i = 0; i < chkNum; i++)
        {
            UInt32 chkSn = (amEntity->rcvNextSn + i)
                           % LTE_RLC_AM_SN_UPPER_BOUND;
            itRlcPduBuff = amEntity->rcvPduBuffer.find(chkSn);
            if (itRlcPduBuff != amEntity->rcvPduBuffer.end())
            {
                isExistRlcPduSeg = TRUE;
                lastRlcPduSegSn = itRlcPduBuff->first;
            }

        }
        if (isMaxStatusSnLessThanRcvNextSn == FALSE
            && isExistRlcPduSeg == TRUE)
        {
            //// NACK settings VR(R):rcvNextSn ~ VR(MS):maxStatusSn
            chkNum = LteRlcAmSeqNumDist(amEntity->rcvNextSn,
                                               lastRlcPduSegSn) + 1;
            for (UInt32 i = 0; i < chkNum; i++)
            {
                UInt32 chkSn = (amEntity->rcvNextSn + i)
                               % LTE_RLC_AM_SN_UPPER_BOUND;
                LteRlcStatusPduSoPair workSegInfo = {0,0};
                itRlcPduBuff = amEntity->rcvPduBuffer.find(chkSn);

                // whole parts is NACKed
                if (itRlcPduBuff == amEntity->rcvPduBuffer.end())
                {
                    workSegInfo.soStart
                        = LTE_RLC_INVALID_STATUS_PDU_FORMAT_NACK_SEGMENT_OFFSET;
                    workSegInfo.soEnd
                        = LTE_RLC_INVALID_STATUS_PDU_FORMAT_NACK_SEGMENT_OFFSET;

                    statusPdu.nackSoMap.insert(
                        pair < UInt16, LteRlcStatusPduSoPair >
                        ((UInt16)chkSn, workSegInfo));
#ifdef LTE_LIB_LOG
                    seqNumLog << chkSn << ",";
#endif
                }
                else if (itRlcPduBuff->second.rlcPduRcvFinished == FALSE)
                {
                    UInt16 minRcvHead = 0;

                    std::map < UInt16, LteRlcAmRcvPduSegmentPart > ::iterator
                        itSegInfo
                        = itRlcPduBuff->second.rcvdPduSegmentInfo.begin();
                    while (itSegInfo
                        != itRlcPduBuff->second.rcvdPduSegmentInfo.end())
                    {
                        if (minRcvHead == 0)
                        {
                            if (itSegInfo->first != 0)
                            {
                                workSegInfo.soStart = 0;
                                workSegInfo.soEnd
                                    = itSegInfo->first;

                                statusPdu.nackSoMap.insert(
                                    pair < UInt16, LteRlcStatusPduSoPair >
                                    ( (UInt16)itRlcPduBuff->first, workSegInfo));
#ifdef LTE_LIB_LOG
                                seqNumLog << itRlcPduBuff->first << ",";
#endif
                            }
                        }
                        else
                        {
                            workSegInfo.soEnd
                                = itSegInfo->first;

                            statusPdu.nackSoMap.insert(
                                pair < UInt16, LteRlcStatusPduSoPair >
                                ((UInt16)itRlcPduBuff->first, workSegInfo));
#ifdef LTE_LIB_LOG
                                seqNumLog << itRlcPduBuff->first << ",";
#endif
                        }
                        // update for next processing
                        minRcvHead = itSegInfo->first;
                        workSegInfo.soStart
                            = itSegInfo->second.rcvdTail;
                        ++itSegInfo;
                    }
                    // set status to NACKed
                    if (workSegInfo.soStart < itRlcPduBuff->second.orgMsgSize)
                    {
                        workSegInfo.soEnd = itRlcPduBuff->second.orgMsgSize;

                        statusPdu.nackSoMap.insert(
                            pair < UInt16, LteRlcStatusPduSoPair >
                            ((UInt16)itRlcPduBuff->first, workSegInfo));
#ifdef LTE_LIB_LOG
                        seqNumLog << itRlcPduBuff->first << ",";
#endif
                    }
                }
            }
        }

        // ACK settings
        chkNum = LteRlcAmSeqNumDist(amEntity->rcvNextSn, amEntity->rcvWnd);
        UInt32 snWork = amEntity->rcvNextSn;
        for (UInt32 i = 0; i < chkNum; i++)
        {
            itRlcPduBuff = amEntity->rcvPduBuffer.find(snWork);
            if ((itRlcPduBuff != amEntity->rcvPduBuffer.end())
                && (itRlcPduBuff->second.rlcPduRcvFinished == TRUE))
            {
                LteRlcAmIncSN(snWork);
            }
            else
            {
                statusPdu.fixed.ackSn = snWork;
                break;
            }
        }

#ifdef LTE_LIB_LOG
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "SeqNumStatusPdu,"
            "Ack,%d,"
            "NAck,%s",
            statusPdu.fixed.ackSn,
            seqNumLog.str().c_str());
#endif

        amEntity->statusBuffer.push_back(statusPdu);
        amEntity->statusBufSize +=
                            LteRlcGetExpectedAmStatusPduSize(statusPdu);

#ifdef LTE_LIB_LOG
        {
            std::stringstream log;
            log.str("");
            log << "LteRlcAmCreateStatusPdu:";
            log << ",statusPDU size (now maked) =,";
            log << LteRlcGetExpectedAmStatusPduSize(statusPdu);
            log << ",statusPDU buffer size =,";
            log << amEntity->statusBufSize;
            log << ",elements of statusPDU buffer =,";
            log << amEntity->statusBuffer.size();
            lte::LteLog::DebugFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "%s,%s",
                LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                log.str().c_str());
        }
#endif // LTE_LIB_LOG

        // StatusProhibit timer re-start.
        // (Clear waitExprioryTStatusProhibitFlg)
        LteRlcAmInitStatusProhibitTimer(node,
                                        iface,
                                        amEntity);
#ifdef LTE_LIB_LOG
        amEntity->dump(node, iface, "AfterCreateStatusPdu");
#endif
    }
}


// /**
// FUNCTION::       LteRlcAmHandlePollRetrunsmit
// LAYER::          LTE LAYER2 RLC
// PURPOSE::        Handle a pollRetrunsmitTimer event
// PARAMETERS::
//      +node:  Node*:    The node pointer
//      +iface: int:    The interface index
//      +amEntity: LteRlcAmEntity*: AM entity
// RETURN::    void:     NULL
// **/
static
void LteRlcAmHandlePollRetrunsmit(Node* node,
                                  int iface,
                                  LteRlcAmEntity* amEntity)
{
    amEntity->pollRetransmitTimerMsg = NULL;
    amEntity->exprioryTPollRetransmitFlg = TRUE;

#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmHandlePollRetrunsmit:";
        log << "timer expire.";
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG

#ifdef LTE_LIB_LOG
    amEntity->dump(node, iface, "AfterExpirePollRetransmit");
#endif
}

// /**
// FUNCTION::       LteRlcAmHandleReordering
// LAYER::          LTE LAYER2 RLC
// PURPOSE::        Handle a reoderingTimer event
// PARAMETERS::
//      +node:  Node*:    The node pointer
//      +iface: int:    The interface index
//      +amEntity: LteRlcAmEntity*: AM entity
// RETURN::    void:     NULL
// **/
static
void LteRlcAmHandleReordering(Node* node,
                              int iface,
                              LteRlcAmEntity* amEntity)
{
    amEntity->reoderingTimerMsg = NULL;

#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmHandleReordering:";
        log << "timer expire.";
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG

    // Update VR(MS):Maximum STATUS transmit state variable by
    // VR(X):t-Reordering state variable
    if (amEntity->tReorderingSn != LTE_RLC_INVALID_SEQ_NUM)
    {
#ifdef LTE_LIB_LOG
        UInt32 oldMaxStatusSn = amEntity->maxStatusSn;
#endif // LTE_LIB_LOG
        std::map < unsigned int, LteRlcAmRcvPduData > ::iterator
            itRcvdSn;
        UInt32 chkSeqNum = amEntity->tReorderingSn;
        UInt32 chkNum =
            LteRlcAmSeqNumDist(chkSeqNum,amEntity->rcvWnd);

        for (UInt32 i = 0; i < chkNum; i++)
        {
            UInt32 curSn = chkSeqNum + i;
            itRcvdSn
                = amEntity->rcvPduBuffer.find(curSn);

            if ((itRcvdSn == amEntity->rcvPduBuffer.end()) ||
                (itRcvdSn->second.rlcPduRcvFinished == FALSE))
            {
                amEntity->maxStatusSn = curSn;
                break;
            }
        }

#ifdef LTE_LIB_LOG
        if (oldMaxStatusSn != amEntity->maxStatusSn)
        {
            std::stringstream log;
            log.str("");
            log << "LteRlcAmHandleReordering:";
            log << " update VR(MS):";
            log << ",old=," << oldMaxStatusSn;
            log << ",new=," << amEntity->maxStatusSn;
            lte::LteLog::DebugFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "%s,%s",
                LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                log.str().c_str());
        }
#endif // LTE_LIB_LOG
    }


    // update t-ReorderingSn
#ifdef LTE_LIB_LOG
    Int32 oldReorderingSn = amEntity->tReorderingSn;
#endif
    if (LteRlcSeqNumLessChkInWnd(amEntity->maxStatusSn, // VR(MS)
                                 amEntity->rcvSn,     // VR(H)
                                 amEntity->rcvNextSn,
                                 amEntity->rcvWnd,
                                 LTE_RLC_AM_SN_UPPER_BOUND))
    {
        LteRlcAmInitReoderingTimer(node,
                                   iface,
                                   amEntity);
        amEntity->tReorderingSn = amEntity->rcvSn;
    }
    else
    {
        amEntity->tReorderingSn = LTE_RLC_INVALID_SEQ_NUM;
    }
#ifdef LTE_LIB_LOG
        {
            std::stringstream log;
            log.str("");
            log << "LteRlcAmHandleReordering:";
            log << " update VR(X) t-Reordering:";
            log << ",old=," << oldReorderingSn;
            log << ",new=," << amEntity->tReorderingSn;
            lte::LteLog::DebugFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "%s,%s",
                LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                log.str().c_str());
        }
#endif // LTE_LIB_LOG

    // Create status PDU
#ifdef LTE_LIB_LOG
    lte::LteLog::DebugFormat(
        node,
        iface,
        LTE_STRING_LAYER_TYPE_RLC,
        "CreateStatusPdu,"
        "t-Reordering is expired");
#endif
    LteRlcAmCreateStatusPdu(node, iface, amEntity);

#ifdef LTE_LIB_LOG
    amEntity->dump(node, iface, "AfterExpireReordering");
#endif
}

// /**
// FUNCTION::       LteRlcAmHandleStatusProhibit
// LAYER::          LTE LAYER2 RLC
// PURPOSE::        Handle a statusProhibitTimer event
// PARAMETERS::
//      +node:  Node*:    The node pointer
//      +iface: int:    The interface index
//      +amEntity: LteRlcAmEntity*: AM entity
// RETURN::    void:     NULL
// **/
static
void LteRlcAmHandleStatusProhibit(Node* node,
                                  int iface,
                                  LteRlcAmEntity* amEntity)
{
    amEntity->statusProhibitTimerMsg = NULL;

#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmHandleStatusProhibit:";
        log << "timer expire.";
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG

    if (amEntity->waitExprioryTStatusProhibitFlg == TRUE)
    {
        // Create status PDU
#ifdef LTE_LIB_LOG
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,t-StatusProhibit is expired",
            LTE_STRING_CATEGORY_TYPE_RLC_CREATE_STATUS_PDU);
#endif
        LteRlcAmCreateStatusPdu(node, iface, amEntity);
    }
    else
    {
#ifdef LTE_LIB_LOG
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,waitExprioryTStatusProhibitFlg == FALSE",
            LTE_STRING_CATEGORY_TYPE_RLC_NOT_CREATE_STATUS_PDU);
#endif
    }

#ifdef LTE_LIB_LOG
    amEntity->dump(node, iface, "AfterExpireStatusProhibit");
#endif
}

// /**
// FUNCTION::       LteRlcAmHandleReset
// LAYER::          LTE LAYER2 RLC
// PURPOSE::        Handle a resetTimer event
// PARAMETERS::
//      +node:  Node*:    The node pointer
//      +iface: int:    The interface index
//      +amEntity: LteRlcAmEntity*: AM entity
// RETURN::    void:     NULL
// **/
static
void LteRlcAmHandleReset(Node* node,
                                  int iface,
                                  LteRlcAmEntity* amEntity)
{
    amEntity->resetTimerMsg = NULL;

#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmHandleReset:";
        log << "timer expire.";
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_CONTROL,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG

    amEntity->resetFlg = FALSE;

    PdcpLteNotifyRlcReset(
            node,
            iface,
            amEntity->entityVar->oppositeRnti,
            amEntity->entityVar->bearerId);

#ifdef LTE_LIB_LOG
    amEntity->dump(node, iface, "AfterExpireReset");
#endif
}

// /**
// FUNCTION::       LteRlcAmHandleAck
// LAYER::          LTE LAYER2 RLC
// PURPOSE::        Handle negative and positive acknowledgement
// PARAMETERS::
//      +node:      The node pointer
//      +iface:     The interface index
//      +amEntity:  The AM entity
//      +rxStatusPDU: The ACK sequence number returned in the ACK SUFI
// RETURN::         NULL
// **/
static
void LteRlcAmHandleAck(Node* node,
                       int iface,
                       const LteRnti& srcRnti,
                       const int bearerId,
                       LteRlcAmEntity* amEntity,
                       LteRlcStatusPduFormat rxStatusPDU)
{
    std::list < LteRlcAmReTxBuffer > ::iterator itRetxBuff;

#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
    lte::LteLog::InfoFormat(
        node,
        iface,
        LTE_STRING_LAYER_TYPE_RLC,
        "RecvStatus,"
        LTE_STRING_FORMAT_RNTI","
        "ACK_SN,%d",
        amEntity->entityVar->oppositeRnti.nodeId,
        amEntity->entityVar->oppositeRnti.interfaceIndex,
        rxStatusPDU.fixed.ackSn);
    std::string nackString("");
    std::multimap < UInt16, LteRlcStatusPduSoPair > ::iterator nackItr;
    for (nackItr  = rxStatusPDU.nackSoMap.begin();
        nackItr != rxStatusPDU.nackSoMap.end();
        ++nackItr)
    {
        lte::LteLog::InfoFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "RecvStatus,"
            LTE_STRING_FORMAT_RNTI","
            "NACK_SN,%d,%d,%d",
            amEntity->entityVar->oppositeRnti.nodeId,
            amEntity->entityVar->oppositeRnti.interfaceIndex,
            nackItr->first,
            nackItr->second.soStart,
            nackItr->second.soEnd);
    }
#endif
#endif
    //1.Process NACK_SN
    BOOL nackSnEqPollSn = FALSE;
    std::multimap < UInt16, LteRlcStatusPduSoPair > ::iterator
        itNackList = rxStatusPDU.nackSoMap.begin();
    while (itNackList != rxStatusPDU.nackSoMap.end())
    {
        if ((*itNackList).first == amEntity->pollSn)
        {
            nackSnEqPollSn = TRUE;
        }
        if (LteRlcSeqNumInRange((*itNackList).first,
                                amEntity->ackSn,
                                amEntity->sndNextSn)
            == FALSE)
        {
            rxStatusPDU.nackSoMap.erase(itNackList++);
            continue;
        }
        ++itNackList;
    }

    SetNackedRetxBuffer(node,
                        iface,
                        *amEntity,
                        amEntity->retxBuffer,
                        rxStatusPDU.nackSoMap);

    //2.Process ACK_SN
    Int16 ackedSn = LTE_RLC_INVALID_SEQ_NUM;
    itRetxBuff = amEntity->retxBuffer.begin();
    while (itRetxBuff != amEntity->retxBuffer.end())
    {
        Int16 chkSn = itRetxBuff->rlcHeader.fixed->seqNum;
        if (((itRetxBuff->pduStatus
                == LTE_RLC_AM_DATA_PDU_STATUS_WAIT) ||
                (itRetxBuff->pduStatus
                == LTE_RLC_AM_DATA_PDU_STATUS_NACK))
            && (LteRlcSeqNumInRange(chkSn,
                                    amEntity->ackSn,
                                    rxStatusPDU.fixed.ackSn)))
        {
            itRetxBuff->pduStatus = LTE_RLC_AM_DATA_PDU_STATUS_ACK;
            ackedSn = chkSn;
        }
        ++itRetxBuff;
    }

    //3. update transmission Window

#ifdef LTE_LIB_LOG
    UInt32 oldAckSn = amEntity->ackSn;
    UInt32 oldSndWnd = amEntity->sndWnd;
#endif // LTE_LIB_LOG

    //  find not ACKed PDU from ackSn[VT(A)]<=SN<ACK_SN
    UInt32 chkNum = LteRlcAmSeqNumDist(amEntity->ackSn,
                                       rxStatusPDU.fixed.ackSn);
    for (UInt32 i = 0; i < chkNum; i++)
    {
        if (findSnRetxBuffer(node,
                             iface,
                             amEntity->retxBuffer,
                             amEntity->ackSn,
                             itRetxBuff))
        {
            if (itRetxBuff->pduStatus
                != LTE_RLC_AM_DATA_PDU_STATUS_ACK)
            {
                // stop VT(A) updating
                break;
            }
        }
        // update transmit window VT(A).
        LteRlcAmIncSN(amEntity->ackSn);
    }

    // Remove PDU which status is ACK
    DeleteSnRetxBuffer(node,
                       iface,
                       amEntity,
                       amEntity->retxBuffer);

#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(node, iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "rxRlcAck,"
            LTE_STRING_FORMAT_RNTI","
            "%d",
            srcRnti.nodeId,
            srcRnti.interfaceIndex,
            rxStatusPDU.fixed.ackSn);
#endif // LTE_LIB_LOG
    // notify PDCP of PDCP Status Report
    LteRlcAmNotifyPdCpOfPdcpStatusReport(
            node, iface, srcRnti, bearerId,
            amEntity, rxStatusPDU.fixed.ackSn);

#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
    std::list < LteRlcAmReTxBuffer > ::iterator retxItr =
                                                amEntity->retxBuffer.begin();
    while (retxItr != amEntity->retxBuffer.end())
    {
        lte::LteLog::InfoFormat(
            node, iface, LTE_STRING_LAYER_TYPE_RLC,
            "AfterHandleAck,"
            LTE_STRING_FORMAT_RNTI","
            "%d,%d",
            amEntity->entityVar->oppositeRnti.nodeId,
            amEntity->entityVar->oppositeRnti.interfaceIndex,
            retxItr->rlcHeader.fixed->seqNum,
            retxItr->pduStatus);
        ++retxItr;
    }
#endif
#endif

    // update transmit window VT(MS).
    amEntity->sndWnd = (amEntity->ackSn + LTE_RLC_AM_WINDOW_SIZE)
                       % LTE_RLC_AM_SN_UPPER_BOUND;

#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmHandleAck:";
        log << " update VT(A):";
        log << ",old=," << oldAckSn;
        log << ",new=," << amEntity->ackSn;
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
            log.str().c_str());
        log.str("");
        log << "LteRlcAmHandleAck:";
        log << " update VT(MS):";
        log << ",old=," << oldSndWnd;
        log << ",new=," << amEntity->sndWnd;
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG

    // check t-PollRetransmit
    if ((amEntity->pollRetransmitTimerMsg != NULL)
        && ((rxStatusPDU.fixed.ackSn == (Int32)amEntity->pollSn)
            || (nackSnEqPollSn == TRUE)))
    {
        MESSAGE_CancelSelfMsg(node, amEntity->pollRetransmitTimerMsg);
        amEntity->pollRetransmitTimerMsg = NULL;
    }

}


// /**
// FUNCTION::       LteRlcAmReceiveStatusPdu
// LAYER::          LTE LAYER2 RLC
// PURPOSE::        Invoked when AM entity receives a STATUS PDU
// PARAMETERS::
//      +node:      The node pointer
//      +iface:     The interface index
//      +amEntity:  AM entity
//      +pduMsg:    The received STATUS PDU message
// RETURN::         NULL
// **/
//static
void LteRlcAmReceiveStatusPdu(Node* node,
                              int iface,
                              const LteRnti& srcRnti,
                              const int bearerId,
                              LteRlcAmEntity* amEntity,
                              Message* pduMsg)
{
    LteRlcStatusPduFormat rxStatusPDU;
    rxStatusPDU.fixed.ackSn = LteRlcAmGetAckSn(pduMsg);
    LteRlcAmGetNacklist(pduMsg, &(rxStatusPDU.nackSoMap));

#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmReceiveStatusPdu:";
        log << ",ackSn=," << rxStatusPDU.fixed.ackSn;
        log << ",Nacklist size=," << rxStatusPDU.nackSoMap.size();
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG

    LteRlcAmHandleAck(node, iface, srcRnti, bearerId, amEntity, rxStatusPDU);
}


// /**
// FUNCTION::       LteRlcGetRlcEntity
// LAYER::          LTE LAYER2 RLC
// PURPOSE::        Get address-pointer of RLC entity
// PARAMETERS::
//      +node:      Node pointer
//      +iface:     The interface index
//      +rnti:      RNTI
//      +bearerId:  bearer ID
// RETURN::         NULL
// **/
LteRlcEntity* LteRlcGetRlcEntity(
    Node* node,
    int interfaceIndex,
    const LteRnti& rnti,
    const int bearerId)
{
    LteRlcEntity* ret = NULL;

    LteConnectionInfo* connectionInfo = Layer3LteGetConnectionInfo(
        node, interfaceIndex, rnti);

    if (connectionInfo == NULL ||
        connectionInfo->state == LAYER3_LTE_CONNECTION_WAITING)
    {
        return NULL;
    }

    MapLteRadioBearer* radioBearers =
        &(connectionInfo->connectedInfo.radioBearers);

    MapLteRadioBearer::iterator itr;
    itr = radioBearers->find(bearerId);
    if (itr != radioBearers->end())
    {
        ret = &((*itr).second.rlcEntity);
    }
    else
    {
        char errBuf[MAX_STRING_LENGTH] = {0};
        sprintf(errBuf, "Not found Radio Bearer. "
            "Node : %d Interface : %d Bearer : %d",
            rnti.nodeId,
            rnti.interfaceIndex,
            bearerId);
        ERROR_ReportError(errBuf);
    }

    return ret;
}

// /**
// FUNCTION::       LteRlcCreateAmEntity
// LAYER::          LTE LAYER2 RLC
// PURPOSE::        Initialize a RLC AM entity
// PARAMETERS::
//      +node:      Node pointer
//      +iface:     The interface index
//      +entity:    RLC entity
// RETURN::         NULL
// **/
static
void LteRlcCreateAmEntity(Node* node,
                          int iface,
                          LteRlcEntity* entity)
{
    ERROR_Assert(entity != NULL,
        "LteRlcCreateAmEntity: entity == NULL");
    ERROR_Assert(entity->entityType == LTE_RLC_ENTITY_AM,
        "LteRlcCreateAmEntity: entity->entityType != LTE_RLC_ENTITY_AM");

    switch (entity->entityType)
    {
    case LTE_RLC_ENTITY_AM:
        {
            LteRlcAmEntity* amEntity = new LteRlcAmEntity();
            //entity->entityData = MEM_malloc(sizeof(LteRlcAmEntity));
            entity->entityData = (void*)amEntity;
            ERROR_Assert(entity->entityData != NULL,
                        "RLC LTE: Out of memory!");
            amEntity->entityVar = NULL;
            break;
        }
    default:
        ERROR_Assert(FALSE,
                    "RLC LTE: entity type Error!");
    }


}

// /**
// FUNCTION::       LteRlcInitAmEntity
// LAYER::          LTE LAYER2 RLC
// PURPOSE::        Create a RLC entity
// PARAMETERS::
//      +node:      Node pointer
//      +iface:     The interface index
//      +rnti:      Oposite RNTI
//      +lteRlc:    RLC sublayer data structure
//      +direct:    The traffic direction of the entity to be created
//      +entity:    The RLC entity
// RETURN::         BOOL
// **/
void LteRlcInitAmEntity(
    Node* node,
    int iface,
    const LteRnti& rnti,
    const LteRlcData* lteRlc,
    const LteRlcEntityDirect& direct,
    LteRlcEntity* entity)
{
    LteRlcCreateAmEntity(node, iface, entity);
    return;
}


// /**
// FUNCTION::       LteRlcAmFinalize
// LAYER::          LTE LAYER2 RLC
// PURPOSE::        Finalize an AM entity
// PARAMETERS::
//      +node:      Node pointer
//      +iface:     The interface index
//      +amEntity:  AM entity
// RETURN::         NULL
// **/
// /**
static
void LteRlcAmEntityFinalize(
    Node* node,
    int iface,
    LteRlcAmEntity* amEntity)
{
    if (amEntity) {
        amEntity->resetAll();
        delete amEntity;
    } else {
        ERROR_Assert(FALSE, "amEntity is NULL");
    } 
}

// /**
// FUNCTION::           LteRlcAmScreenRcvdPdu
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            When receiving a new PDU, the AM entity
//                      decides to drop the PDU or drop some buffered
//                      PDU depend on the seq. number of received PDU and
//                      the state variables
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         The interface index
//      +amEntity:      The AM entity
//      +rcvdSeqNum:    The seqence number of the PDU, from which
//                      the sdu segments are extracted
//      +pduMsg:        The received PDU
// RETURN::             BOOL
// **/
static
BOOL LteRlcAmScreenRcvdPdu(Node* node,
                           int iface,
                           LteRlcAmEntity* amEntity,
                           unsigned int rcvdSeqNum,
                           Message* pduMsg)
{
    BOOL prepStatusPdu = FALSE;
    BOOL ret = FALSE;
    std::map < unsigned int, LteRlcAmRcvPduData > ::iterator
        itRcvdSn = amEntity->rcvPduBuffer.find(rcvdSeqNum);

    if (LteRlcAmSeqNumInWnd(rcvdSeqNum, amEntity->rcvNextSn) == FALSE)
    {
        // receved sequense number is outside reception window.
#ifdef LTE_LIB_LOG
        {
            std::stringstream log;
            log.str("");
            log << "LteRlcAmScreenRcvdPdu:";
            log << ",receved sequense number [" << rcvdSeqNum;
            log << "] is outside of reception window.";
            lte::LteLog::DebugFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "%s,%s",
                LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                log.str().c_str());
        }
#if LTE_LIB_RLC_CHECK_EVENT
        lte::LteLog::InfoFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "RecvData,"
            LTE_STRING_FORMAT_RNTI","
            "OutOfWindow,"
            "%d",
            amEntity->entityVar->oppositeRnti.nodeId,
            amEntity->entityVar->oppositeRnti.interfaceIndex,
            rcvdSeqNum);
#endif
#endif // LTE_LIB_LOG
        prepStatusPdu = TRUE;
    }
    else
    {
        if ((itRcvdSn != amEntity->rcvPduBuffer.end())
            && (itRcvdSn->second.rlcPduRcvFinished == TRUE) )
        {
            // Received SN(rlc-PDU)
#ifdef LTE_LIB_LOG
            {
                std::stringstream log;
                log.str("");
                log << "LteRlcAmScreenRcvdPdu:";
                log << ",receved sequense number(rlc-PDU) [" << rcvdSeqNum;
                log << "] has already received.";
                lte::LteLog::DebugFormat(
                    node,
                    iface,
                    LTE_STRING_LAYER_TYPE_RLC,
                    "%s,%s",
                    LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                    log.str().c_str());
            }
#if LTE_LIB_RLC_CHECK_EVENT
            lte::LteLog::InfoFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "RecvData,"
                LTE_STRING_FORMAT_RNTI","
                "AlreadyReceived,"
                "%d",
                amEntity->entityVar->oppositeRnti.nodeId,
                amEntity->entityVar->oppositeRnti.interfaceIndex,
                rcvdSeqNum);
#endif
#endif // LTE_LIB_LOG
            prepStatusPdu = TRUE;
        }
    }

    // If there is a polling bit set, prepare status PDU
    if (prepStatusPdu == TRUE)
    {
        if (LteRlcAmGetPollBit(pduMsg))
        {
#ifdef LTE_LIB_LOG
            std::stringstream log;
            log.str("");
            log << "LteRlcAmReceivePduFromMac:";
            log << "receives a poll in the PDU message.";
            lte::LteLog::DebugFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "%s,%s,rcvNext=,%d,rcvHighest=,%d,rcvWnd=,%d",
                LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                log.str().c_str(),
                amEntity->rcvNextSn,
                amEntity->rcvSn,
                amEntity->rcvWnd);
#endif // LTE_LIB_LOG

#ifdef LTE_LIB_LOG
            lte::LteLog::DebugFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "%s,PollBit&&Receiving pdu is discard",
                LTE_STRING_CATEGORY_TYPE_RLC_CREATE_STATUS_PDU,
                rcvdSeqNum, amEntity->rcvNextSn);
#endif
            LteRlcAmCreateStatusPdu(node, iface, amEntity);
        }

        return ret;
    }

    // if reached PDU, process received PDU
    ret = TRUE;
#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
        lte::LteLog::InfoFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "RecvData,"
            LTE_STRING_FORMAT_RNTI","
            "%d,isFrag,%d",
            amEntity->entityVar->oppositeRnti.nodeId,
            amEntity->entityVar->oppositeRnti.interfaceIndex,
            rcvdSeqNum,
            LteRlcAmGetFramingInfo(pduMsg));
#endif
#endif
    LteRlcAmRcvPduData rcvData;
    rcvData.FramingInfo = LteRlcAmGetFramingInfo(pduMsg);
    LteRlcAmGetLIlist(pduMsg, &(rcvData.lengthIndicator));
    rcvData.orgMsgSize = LteRlcGetOrgPduSize(node, pduMsg);
    // first received SN
    if (itRcvdSn == amEntity->rcvPduBuffer.end())
    {
        amEntity->rcvPduBuffer.insert(
            make_pair(rcvdSeqNum, rcvData));
    }

    if (LteRlcAmGetRFBit(pduMsg) == TRUE)
    {
        ret = LteRlcAmPduSegmentReassemble(node,
                                           iface,
                                           amEntity,
                                           rcvdSeqNum,
                                           pduMsg);
    }
    // receives RLC-PDU.(not segment)
    else
    {
        amEntity->rcvPduBuffer[rcvdSeqNum].rlcPduRcvFinished = TRUE;
        amEntity->rcvPduBuffer[rcvdSeqNum].message = pduMsg;
        amEntity->rcvPduBuffer[rcvdSeqNum].FramingInfo
            = rcvData.FramingInfo;
        amEntity->rcvPduBuffer[rcvdSeqNum].lengthIndicator
            = rcvData.lengthIndicator;
#ifdef LTE_LIB_LOG
        {
            std::stringstream log;
            log.str("");
            log << "LteRlcAmScreenRcvdPdu:";
            log << ",received PDU-SeqNum=," << rcvdSeqNum;
            lte::LteLog::DebugFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "%s,%s",
                LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                log.str().c_str());
        }
#endif // LTE_LIB_LOG
    }

    // update VR(H):Highest received state variable.
    if (LteRlcSeqNumLessChkInWnd(amEntity->rcvSn,
                                    rcvdSeqNum,
                                    amEntity->rcvNextSn,
                                    amEntity->rcvWnd,
                                    LTE_RLC_AM_SN_UPPER_BOUND)
        || (amEntity->rcvSn == rcvdSeqNum))
    {
        UInt32 tmpSN = (UInt32)rcvdSeqNum;
        LteRlcAmIncSN(tmpSN);
#ifdef LTE_LIB_LOG
        {
            std::stringstream log;
            log.str("");
            log << "LteRlcAmScreenRcvdPdu:";
            log << ",update VR(H):,";
            log << ",old=," << amEntity->rcvSn;
            log << ",new=," << tmpSN;
            lte::LteLog::DebugFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "%s,%s",
                LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                log.str().c_str());
        }
#endif // LTE_LIB_LOG
        amEntity->rcvSn = tmpSN;
    }

    // update VR(MS):Maximum STATUS transmit state variable.
#ifdef LTE_LIB_LOG
    UInt32 oldMaxStatusSn = amEntity->maxStatusSn;
#endif // LTE_LIB_LOG
    if (rcvdSeqNum == amEntity->maxStatusSn)
    {
        UInt32 chkSeqNum = amEntity->maxStatusSn;
        LteRlcAmIncSN(chkSeqNum);
        UInt32 chkNum =
            LteRlcAmSeqNumDist(chkSeqNum,amEntity->rcvWnd);

        for (UInt32 i = 0; i < chkNum; i++)
        {
            UInt32 curSn = chkSeqNum + i;
            itRcvdSn
                = amEntity->rcvPduBuffer.find(curSn);

            if ((itRcvdSn == amEntity->rcvPduBuffer.end()) ||
                (itRcvdSn->second.rlcPduRcvFinished == FALSE))
            {
                amEntity->maxStatusSn = curSn;
                break;
            }
        }
    }
#ifdef LTE_LIB_LOG
    if (oldMaxStatusSn != amEntity->maxStatusSn)
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcAmScreenRcvdPdu:";
        log << " update VR(MS):";
        log << ",old=," << oldMaxStatusSn;
        log << ",new=," << amEntity->maxStatusSn;
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG

    return ret;
}

// /**
// FUNCTION::           LteRlcExtractSduSegmentFromPdu
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Separate segments belonging to various SDU so that
//                      they can be inserted into a
//                      list to facilate re-assembly
//                      once all segments of an SDU have arrived
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         the interface index
//      +rlcEntity:     The rlc entity doing the reassembly
//      +pduMsg:        The received PDU
// RETURN::             NULL
// **/
static
void LteRlcExtractSduSegmentFromPdu(
    Node* node,
    int iface,
    LteRlcEntity* rlcEntity,
    Message* pduMsg)
{
    char errorMsg[MAX_STRING_LENGTH] = "LteRlcExtractSduSegmentFromPdu: ";

    LteRlcEntityType mode = rlcEntity->entityType;
    Int16 rcvdSeqNum = 0;

    if (mode == LTE_RLC_ENTITY_AM)
    {
        LteRlcAmEntity* amEntity =
            (LteRlcAmEntity*)rlcEntity->entityData;
#ifndef LTE_LIB_RLC_ENTITY_VAR
        amEntity->entityVar = rlcEntity;
#endif
        ERROR_Assert(amEntity, errorMsg);
        rcvdSeqNum = LteRlcAmGetSeqNum(pduMsg);
        if (rcvdSeqNum < 0)
        {
            sprintf(errorMsg + strlen(errorMsg),
                "invalid Sequense number.");
            ERROR_Assert(FALSE, errorMsg);
        }
    }
    else
    {
        sprintf(errorMsg + strlen(errorMsg),
            "worng RLC entity type for Phase-1.");
        ERROR_Assert(FALSE, errorMsg);
    }
#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "LteRlcExtractSduSegmentFromPdu:";
        log << "received a data PDU.";
        log << ",sequence number=," << rcvdSeqNum;
        log << ",receiving PDU size=," << MESSAGE_ReturnPacketSize(pduMsg);
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG

    BOOL screenRes = FALSE;

    if (mode == LTE_RLC_ENTITY_AM)
    {
        LteRlcAmEntity* amEntity
            = (LteRlcAmEntity*)rlcEntity->entityData;
#ifndef LTE_LIB_RLC_ENTITY_VAR
        amEntity->entityVar = rlcEntity;
#endif
        screenRes = LteRlcAmScreenRcvdPdu(node,
                                          iface,
                                          amEntity,
                                          rcvdSeqNum,
                                          pduMsg);
    }

    // get P field
    BOOL pollBitActive = LteRlcAmGetPollBit(pduMsg);

    // outside of window, or aleady received message.
    if (screenRes == FALSE)
    {
#ifdef LTE_LIB_LOG
        LteRlcData* lteRlc = LteLayer2GetLteRlcData(node, iface);
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "RxSeqNum,"
            "%d,OutOfWindowOrAlreadyReceived",
            rcvdSeqNum);
#endif
        MESSAGE_Free(node, pduMsg);
        return;
    }
#ifdef LTE_LIB_LOG
    lte::LteLog::DebugFormat(
        node,
        iface,
        LTE_STRING_LAYER_TYPE_RLC,
        "RxSeqNum,"
        "%d,Success",
        rcvdSeqNum);
#endif

    if (mode == LTE_RLC_ENTITY_AM)
    {
        LteRlcAmEntity* amEntity
            = (LteRlcAmEntity*)rlcEntity->entityData;
#ifndef LTE_LIB_RLC_ENTITY_VAR
        amEntity->entityVar = rlcEntity;
#endif

        // update reception window if received VR(R)'s SN completely
        if ((UInt32)rcvdSeqNum == amEntity->rcvNextSn)
        {
            std::map < unsigned int, LteRlcAmRcvPduData > ::iterator
                itRlcPduBuff;
            UInt32 chkNum = LteRlcAmSeqNumDist(amEntity->rcvNextSn,
                                               amEntity->rcvWnd);
            for (UInt32 i = 0; i < chkNum; i++)
            {
                itRlcPduBuff
                    = amEntity->rcvPduBuffer.find(amEntity->rcvNextSn);
                if ((itRlcPduBuff == amEntity->rcvPduBuffer.end())
                    || (itRlcPduBuff->second.rlcPduRcvFinished == FALSE))
                {
                    break;
                }
                else
                {
                    LteRlcAmIncSN(amEntity->rcvNextSn);
                }
            }

#ifdef LTE_LIB_LOG
            {
                std::stringstream log;
                log.str("");
                log << "LteRlcExtractSduSegmentFromPdu:";
                log << " update VR(R):";
                log << ",old=," << rcvdSeqNum;
                log << ",new=," << amEntity->rcvNextSn;
                lte::LteLog::DebugFormat(
                    node,
                    iface,
                    LTE_STRING_LAYER_TYPE_RLC,
                    "%s,%s,sequence number=,%d,receiving PDU size=,%d",
                    LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                    log.str().c_str(),
                    rcvdSeqNum,
                    MESSAGE_ReturnPacketSize(pduMsg));
            }
#endif // LTE_LIB_LOG

            // update receive window VR(MR).
#ifdef LTE_LIB_LOG
            UInt32 oldRcvWnd = amEntity->rcvWnd;
#endif // LTE_LIB_LOG

            amEntity->rcvWnd
                = (amEntity->rcvNextSn + LTE_RLC_AM_WINDOW_SIZE)
                  % LTE_RLC_AM_SN_UPPER_BOUND;

#ifdef LTE_LIB_LOG
            {
                std::stringstream log;
                log.str("");
                log << "LteRlcExtractSduSegmentFromPdu:";
                log << " update VR(MR):";
                log << ",old=," << amEntity->rcvWnd;
                log << ",new=," << amEntity->rcvWnd;
                lte::LteLog::DebugFormat(
                    node,
                    iface,
                    LTE_STRING_LAYER_TYPE_RLC,
                    "%s,%s,sequence number=,%d,receiving PDU size=,%d",
                    LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                    log.str().c_str(),
                    rcvdSeqNum,
                    MESSAGE_ReturnPacketSize(pduMsg));
            }
#endif // LTE_LIB_LOG

            // reassemble rlc-SDU(pdcp-PDU).
            LteRlcSDUsegmentReassemble(node,
                                       iface,
                                       rlcEntity);
        }

        // check reorderingTimer
        if (amEntity->reoderingTimerMsg != NULL)
        {
            if ((amEntity->tReorderingSn == (Int32)amEntity->rcvNextSn)
                || ((LteRlcAmSeqNumInWnd(rcvdSeqNum, amEntity->rcvNextSn)
                        == FALSE)
                    && (amEntity->tReorderingSn != (Int32)amEntity->rcvWnd)))
            {
                MESSAGE_CancelSelfMsg(node, amEntity->reoderingTimerMsg);
                amEntity->reoderingTimerMsg = NULL;
                amEntity->tReorderingSn = LTE_RLC_INVALID_SEQ_NUM;
            }
        }
        if (amEntity->reoderingTimerMsg == NULL)
        {
            LteRlcAmInitReoderingTimer(node, iface, amEntity);
            amEntity->tReorderingSn = amEntity->rcvSn;
        }

        // P field=1(TRUE) && SN < VR(MS) -> create statusPDU
        if (pollBitActive
            && LteRlcSeqNumLessChkInWnd(rcvdSeqNum,
                                        amEntity->maxStatusSn,
                                        amEntity->rcvNextSn,
                                        amEntity->rcvWnd,
                                        LTE_RLC_AM_SN_UPPER_BOUND))
        {
#ifdef LTE_LIB_LOG
            lte::LteLog::DebugFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "%s,PollBit&&SN(%d)<VR(MS)(%d)",
                LTE_STRING_CATEGORY_TYPE_RLC_CREATE_STATUS_PDU,
                rcvdSeqNum, amEntity->maxStatusSn);
#endif
            LteRlcAmCreateStatusPdu(node, iface, amEntity);
        }
        else
        {
#ifdef LTE_LIB_LOG
            lte::LteLog::DebugFormat(
                node,
                iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "%s,!PollBit(%d)||SN(%d)>=VR(MS)(%d)",
                LTE_STRING_CATEGORY_TYPE_RLC_NOT_CREATE_STATUS_PDU,
                pollBitActive, rcvdSeqNum, amEntity->rcvNextSn);
#endif
        }
#ifdef LTE_LIB_LOG
        amEntity->dump(node, iface, "AfterRcvDataPdu");
#endif
    }
}

// /**
// FUNCTION::           LteRlcAmReceivePduFromMac
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Invoked when MAC submit a PDU to the entity
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         the interface index
//      +amEntity:      TM RLC AM entity
//      +msgList:       PDU message list
// RETURN::             NULL
// **/
static
void LteRlcAmReceivePduFromMac(
    Node* node,
    int iface,
    const LteRnti& srcRnti,
    const int bearerId,
    LteRlcAmEntity* amEntity,
    std::list < Message* > &msgList)
{
    LteRlcData* rlcData = LteRlcGetSubLayerData(node, iface);

#ifdef LTE_LIB_LOG
        std::stringstream log;
        log.str("");
        log << "LteRlcAmReceivePduFromMac:";
        log << "received messages.";
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s,msg-num=,%d",
            LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
            log.str().c_str(),
            msgList.size());
#endif // LTE_LIB_LOG

    std::list < Message* > ::iterator it;
    for (it = msgList.begin(); it != msgList.end(); )
    {
        Message* pduMsg = *it;

        // inspect what type of PDU it is
        if (LteRlcAmGetDCBit(pduMsg))
        {
            if (amEntity->resetFlg == TRUE)
            {
#ifdef LTE_LIB_LOG
                std::stringstream log;
                log.str("");
                log << "LteRlcAmReceivePduFromMac:";
                log << "received data pdu but discarded by reset.";
                lte::LteLog::DebugFormat(
                    node,
                    iface,
                    LTE_STRING_LAYER_TYPE_RLC,
                    "%s,%s",
                    LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                    log.str().c_str());
#endif
#ifdef ADDON_DB
                HandleMacDBEvents(
                    node,
                    pduMsg,
                    node->macData[iface]->phyNumber,
                    iface,
                    MAC_Drop,
                    node->macData[iface]->macProtocol,
                    TRUE,
                    "Discarded Received RLC PDU due to Reset");
#endif
                rlcData->stats.
                    numDataPdusReceivedFromMacSublayerButDiscardedByReset++;
                MESSAGE_Free(node, pduMsg);
                it = msgList.erase(it);
                continue;
            }
            rlcData->stats.numDataPdusReceivedFromMacSublayer++;

            Message* dupMsg = MESSAGE_Duplicate(node, pduMsg);
            LteRlcExtractSduSegmentFromPdu(node,
                                           iface,
                                           amEntity->entityVar,
                                           dupMsg);
        }
        else
        {
            // Received an control PDU.
            // -> check the CPT(Control PDU Type).
            UInt8 controlPduType = LteRlcAmGetCPT(pduMsg);
            if (controlPduType == LTE_RLC_AM_CONTROL_PDU_TYPE_STATUS)
            {
                if (amEntity->resetFlg == TRUE)
                {
#ifdef LTE_LIB_LOG
                    std::stringstream log;
                    log.str("");
                    log << "LteRlcAmReceivePduFromMac:";
                    log << "received status pdu but discarded by reset.";
                    lte::LteLog::DebugFormat(
                        node,
                        iface,
                        LTE_STRING_LAYER_TYPE_RLC,
                        "%s,%s",
                        LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                        log.str().c_str());
#endif
                    rlcData->stats.
                        numAmStatusPdusReceivedFromMacSublayerButDiscardedByReset++;
                    MESSAGE_Free(node, pduMsg);
                    it = msgList.erase(it);
                    continue;
                }

                rlcData->stats.numAmStatusPdusReceivedFromMacSublayer++;

                LteRlcAmReceiveStatusPdu(node,
                                         iface,
                                         srcRnti,
                                         bearerId,
                                         amEntity,
                                         pduMsg);
#ifdef LTE_LIB_LOG
                amEntity->dump(node, iface, "AfterRcvStatusPdu");
#endif
            }
            else
            {
                std::stringstream log;
                log.str("");
                log << "LteRlcAmReceivePduFromMac:";
                log << "received an invalid type of control PDU.";
#ifdef LTE_LIB_LOG
                lte::LteLog::DebugFormat(
                    node,
                    iface,
                    LTE_STRING_LAYER_TYPE_RLC,
                    "%s,%s",
                    LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                    log.str().c_str());
#endif // LTE_LIB_LOG
                ERROR_Assert(FALSE, log.str().c_str());
            }
        }

        MESSAGE_Free(node, pduMsg);
        it = msgList.erase(it);
    }
}

// /**
// FUNCTION::       LteRlcAmUpperLayerHasSduToSend
// LAYER::          LTE LAYER2 RLC
// PURPOSE::        Invoked when uppler layer deliver a SDU to the
//                  AM entity.
// PARAMETERS::
//      +node:      Node pointer
//      +iface:     the interface index
//      +amEntity:  the AM entity
//      +sduMsg:    The SDU message
// RETURN::         NULL
// **/
static
void LteRlcAmUpperLayerHasSduToSend(
    Node* node,
    int iface,
    LteRlcAmEntity* amEntity,
    Message* sduMsg)
{
    // add info for management received RLC ACK
    LteRlcAmPdcpPduInfo pduInfo = LteRlcAmPdcpPduInfo(
        LteLayer2GetPdcpSnFromPdcpHeader(sduMsg));
    LteAddMsgInfo(node, iface, sduMsg, INFO_TYPE_LteRlcAmPdcpPduInfo,
            pduInfo);
    amEntity->store(node, iface, sduMsg);
    return;
}

// /**
// FUNCTION::           LteRlcAmDeliverPduToMac
// LAYER::              LTE LAYER2 RLC
// PURPOSE::            Prepare RLC PDUs sent to MAC through AM SAP,
//                      invoked for each TTI
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         the interface index
//      +amEntity:      The AM entity
//      +pduSize:       PDU size given by MAC
//      +pduList:       A list containing the pdus to be sent
// RETURN::             NULL
// **/
static
void LteRlcAmDeliverPduToMac(
    Node* node,
    int iface,
    LteRlcAmEntity* amEntity,
    int pduSize,
    std::list < Message* >* pduList)
{
#ifdef LTE_LIB_LOG
    std::stringstream log;
    log.str("");
    log << "LteRlcAmDeliverPduToMac:";
    log << ",Request pduSize," <<  pduSize;

    lte::LteLog::DebugFormat(
        node,
        iface,
        LTE_STRING_LAYER_TYPE_RLC,
        "%s,%s",
        LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
        log.str().c_str());
#endif // LTE_LIB_LOG

    amEntity->restore(node, iface, pduSize, *pduList);
}

// /**
// FUNCTION::       LteRlcInitConfigurableParameters
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Initialize configurable parameters of LTE RLC Layer.
// PARAMETERS::
//    + node:       : Node*             : Pointer to the network node.
//    + iface:      : int               : Interface index.
//    + nodeInput:  : const NodeInput*  : Pointer to node input.
// RETURN::         NULL
// **/
static
void LteRlcInitConfigurableParameters(Node* node,
                                      int iface,
                                      const NodeInput* nodeInput)
{
    BOOL wasFound = false;
    char errBuf[MAX_STRING_LENGTH] = {0};
    LteStationType stationType =
        LteLayer2GetStationType(node, iface);

    clocktype retTime = 0;
    int retInt = 0;

    NodeAddress interfaceAddress =
        MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
            node, node->nodeId, iface);

    LteRlcConfig* rlcConfig = GetLteRlcConfig(node, iface);

    // eNB settings
    if (stationType == LTE_STATION_TYPE_ENB)
    {
        // LteRrcConfig settings
        // LTE_RLC_STRING_MAX_RETX_THRESHOLD
        IO_ReadInt(node->nodeId,
                   interfaceAddress,
                   nodeInput,
                   LTE_RLC_STRING_MAX_RETX_THRESHOLD,
                   &wasFound,
                   &retInt);

        if (wasFound)
        {
            if (LTE_RLC_MIN_MAX_RETX_THRESHOLD <= retInt &&
                retInt <= LTE_RLC_MAX_MAX_RETX_THRESHOLD)
            {
                rlcConfig->maxRetxThreshold = (UInt8)retInt;
            }
            else
            {
                sprintf(errBuf,
                        "LTE-RLC: Max Retx Threshold "
                        "should be %d to %d. "
                        "are available.",
                        LTE_RLC_MIN_MAX_RETX_THRESHOLD,
                        LTE_RLC_MAX_MAX_RETX_THRESHOLD);
                ERROR_ReportError(errBuf);
            }
        }
        else
        {
            // default
            rlcConfig->maxRetxThreshold = LTE_RLC_DEFAULT_MAX_RETX_THRESHOLD;
        }

        // LTE_RLC_STRING_POLL_PDU
        IO_ReadInt(node->nodeId,
                   interfaceAddress,
                   nodeInput,
                   LTE_RLC_STRING_POLL_PDU,
                   &wasFound,
                   &retInt);

        if (wasFound)
        {
            if (LTE_RLC_MIN_POLL_PDU <= retInt){
                if (retInt <= LTE_RLC_MAX_POLL_PDU)
                {
                    rlcConfig->pollPdu = retInt;
                }
                else
                {
                    rlcConfig->pollPdu = LTE_RLC_INFINITY_POLL_PDU;
                }
            }
            else
            {
                sprintf(errBuf,
                        "LTE-RLC: Poll pdu "
                        "should be %d to Infinity. "
                        "are available.",
                        LTE_RLC_MIN_POLL_PDU);
                ERROR_ReportError(errBuf);
            }
        }
        else
        {
            // default
            rlcConfig->pollPdu = LTE_RLC_DEFAULT_POLL_PDU;
        }

        // LTE_RLC_STRING_POLL_BYTE
        IO_ReadInt(node->nodeId,
                   interfaceAddress,
                   nodeInput,
                   LTE_RLC_STRING_POLL_BYTE,
                   &wasFound,
                   &retInt);

        if (wasFound)
        {
            if (LTE_RLC_MIN_POLL_BYTE <= retInt){
                if (retInt <= LTE_RLC_MAX_POLL_BYTE)
                {
                    rlcConfig->pollByte = retInt;
                }
                else
                {
                    rlcConfig->pollByte = LTE_RLC_INFINITY_POLL_BYTE;
                }
            }
            else
            {
                sprintf(errBuf,
                        "LTE-RLC: Poll Byte "
                        "should be %d to Infinity. "
                        "are available.",
                        LTE_RLC_MIN_POLL_BYTE);
                ERROR_ReportError(errBuf);
            }
        }
        else
        {
            // default
            rlcConfig->pollByte = LTE_RLC_DEFAULT_POLL_BYTE;
        }

        // LTE_RLC_STRING_POLL_RETRANSMIT
        IO_ReadTime(node->nodeId,
                   interfaceAddress,
                   nodeInput,
                   LTE_RLC_STRING_POLL_RETRANSMIT,
                   &wasFound,
                   &retTime);

        if (wasFound)
        {
            if (LTE_RLC_MIN_POLL_RETRANSMIT <= retTime &&
                retTime <= LTE_RLC_MAX_POLL_RETRANSMIT)
            {
                rlcConfig->tPollRetransmit =
                    (int)(retTime/MILLI_SECOND);
            }
            else
            {
                sprintf(errBuf,
                        "LTE-RLC: Poll Retransmit "
                        "should be set to more than %" TYPES_64BITFMT "dMS.",
                        LTE_RLC_MIN_POLL_RETRANSMIT/MILLI_SECOND);
                ERROR_ReportError(errBuf);
            }
        }
        else
        {
            // default
            rlcConfig->tPollRetransmit  = LTE_RLC_DEFAULT_POLL_RETRANSMIT;
        }

        // LTE_RLC_STRING_T_REORDERING
        IO_ReadTime(node->nodeId,
                   interfaceAddress,
                   nodeInput,
                   LTE_RLC_STRING_T_REORDERING,
                   &wasFound,
                   &retTime);

        if (wasFound)
        {
            if (LTE_RLC_MIN_T_REORDERING <= retTime &&
                retTime <= LTE_RLC_MAX_T_REORDERING)
            {
                rlcConfig->tReordering =
                    (int)(retTime/MILLI_SECOND);
            }
            else
            {
                sprintf(errBuf,
                        "LTE-RLC: T Reordering "
                        "should be set to more than %" TYPES_64BITFMT "dMS.",
                        LTE_RLC_MIN_T_REORDERING/MILLI_SECOND);
                ERROR_ReportError(errBuf);
            }
        }
        else
        {
            // default
            rlcConfig->tReordering = LTE_RLC_DEFAULT_T_REORDERING;
        }


        // LTE_RLC_STRING_T_STATUS_PROHIBIT
        IO_ReadTime(node->nodeId,
                   interfaceAddress,
                   nodeInput,
                   LTE_RLC_STRING_T_STATUS_PROHIBIT,
                   &wasFound,
                   &retTime);

        if (wasFound)
        {
            if (LTE_RLC_MIN_T_STATUS_PROHIBIT <= retTime &&
                retTime <= LTE_RLC_MAX_T_STATUS_PROHIBIT)
            {
                rlcConfig->tStatusProhibit =
                    (int)(retTime/MILLI_SECOND);
            }
            else
            {
                sprintf(errBuf,
                        "LTE-RLC: T Status Prohibit "
                        "should be set to more than %" TYPES_64BITFMT "dMS.",
                        LTE_RLC_MIN_T_STATUS_PROHIBIT/MILLI_SECOND);
                ERROR_ReportError(errBuf);
            }
        }
        else
        {
            // default
            rlcConfig->tStatusProhibit  = LTE_RLC_DEFAULT_T_STATUS_PROHIBIT;
        }

#ifdef LTE_LIB_LOG

        lte::LteLog::DebugFormat(node, iface,
                                  LTE_STRING_LAYER_TYPE_RLC,
                                  "LteRlcInitConfigurableParameters,"
                                  "rlcConfig->maxRetxThreshold,%d,"
                                  "rlcConfig->pollPdu,%d,"
                                  "rlcConfig->pollByte,%d,"
                                  "rlcConfig->tPollRetransmit,%d,"
                                  "rlcConfig->tReordering,%d,"
                                  "rlcConfig->tStatusProhibit,%d",
                                  rlcConfig->maxRetxThreshold,
                                  rlcConfig->pollPdu,
                                  rlcConfig->pollByte,
                                  rlcConfig->tPollRetransmit,
                                  rlcConfig->tReordering,
                                  rlcConfig->tStatusProhibit);
#endif

    }
}


// /**
// FUNCTION::       LteRlcPrintStat
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Print RLC statistics
// PARAMETERS::
//    + node:       pointer to the network node
//    + iface:      interface index
//    + rlcData:    RLC sublayer data structure
// RETURN::         NULL
// **/
void LteRlcPrintStat(
    Node* node,
    int iface,
    LteRlcData* rlcData)
{
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "Number of SDUs received from upper layer = %u",
        rlcData->stats.numSdusReceivedFromUpperLayer);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of SDUs discarded by overflow = %u",
        rlcData->stats.numSdusDiscardedByOverflow);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of SDUs sent to upper layer = %u",
        rlcData->stats.numSdusSentToUpperLayer);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of data PDUs sent to MAC sublayer = %u",
        rlcData->stats.numDataPdusSentToMacSublayer);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of data PDUs discarded "
                 "by Retransmission threshold = %u",
        rlcData->stats.numDataPdusDiscardedByRetransmissionThreshold);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of data PDUs received from MAC sublayer = %u",
        rlcData->stats.numDataPdusReceivedFromMacSublayer);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of data PDUs received from MAC sublayer "
        "but discarded by RESET = %u",
        rlcData->stats.
            numDataPdusReceivedFromMacSublayerButDiscardedByReset);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of AM STATUS PDUs sent to MAC sublayer = %u",
        rlcData->stats.numAmStatusPdusSendToMacSublayer);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of AM STATUS PDUs received from MAC sublayer = %u",
        rlcData->stats.numAmStatusPdusReceivedFromMacSublayer);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of AM STATUS PDUs received from MAC sublayer "
        "but discarded by RESET = %u",
        rlcData->stats.
            numAmStatusPdusReceivedFromMacSublayerButDiscardedByReset);
    IO_PrintStat(node,
                 "Layer2",
                 "LTE RLC",
                 ANY_DEST,
                 iface,
                 buf);

}
// /**
// FUNCTION   :: LteRlcInitDynamicStats
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Initiate dynamic statistics
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32 : interface index of LTE
// + rlc       : LteRlcData* : Pointer to RLC data
// RETURN     :: void : NULL
// **/
void LteRlcInitDynamicStats(Node* node,
                             UInt32 iface,
                             LteRlcData* rlc)
{
    if (!node->guiOption)
    {
        // do nothing at Phase 1
        return;
    }
    return;
}




#ifdef LTE_RLC_MSG_TEST
static
void LteLteRlcRlcAmReceivePduFromMac(
    Node* node,
    int iface,
    LteRlcAmEntity* amEntity,
    std::list < Message* > &msgList,
    BOOL errorIndi)
{
    //void LteRlcUnpackMessage(Node* node,
    //                   Message* msg,
    //                   BOOL copyInfo,
    //                   BOOL freeOld,
    //                   std::list<Message*>& msgList)
    std::list < Message* > ::iterator it;
    for (it = msgList.begin(); it != msgList.end(); it++)
    {
        //MESSAGE_PrintMessage(*it);
        MESSAGE_logMessage(
            node,
            iface,
            *it,
            LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
            "LteRlcAmReceivePduFromMac(from MAC serialized msg)");
    }
}
#endif // LTE_RLC_MSG_TEST



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
        const LteRnti& targetRnti)
{
    LteRlcEntity* rlcEntity
        = LteRlcGetRlcEntity(node, interfaceIndex, rnti, bearerId);
    ERROR_Assert(rlcEntity, "rlcEntity is NULL");
    LteRlcData* rlcData = LteLayer2GetLteRlcData(node, interfaceIndex);
    ERROR_Assert(rlcData, "rlcData is NULL");

    switch (rlcEntity->entityType) {
        case LTE_RLC_ENTITY_AM: {
            LteRlcAmEntity* rlcAmEntity
                = (LteRlcAmEntity*) rlcEntity->entityData;
            ERROR_Assert(rlcAmEntity, "rlcAmEnity is NULL");
            multimap<unsigned int, Message*>& rcvRlcSduList
                = rlcAmEntity->rcvRlcSduList;
            multimap<unsigned int, Message*>::iterator itr
                = rcvRlcSduList.end();

            // forcibly insert the rcvRlcSduList extracted from the all
            // of the RLC PDUs of receive buffer into the SDU segment list
            {
                rlcAmEntity->dump(node, interfaceIndex, "BeforeReassemble");

                // set VRH to the next RLC SN 
                rlcAmEntity->rcvNextSn = rlcAmEntity->rcvSn;

                // reassemble RLC SDU from rcvPduBuffer
                LteRlcSDUsegmentReassemble(node,
                        interfaceIndex,
                        rlcEntity);

                rlcAmEntity->dump(node, interfaceIndex, "AfterReassemble");
            }

            // deliver all reassembled RLC SDUs to PDCP in ascending order
            // of the RLC SN, if not delivered before
            while (!rcvRlcSduList.empty()) {
                // find RLC SN should be send
                itr = rcvRlcSduList.lower_bound(rlcAmEntity->oldRcvNextSn);
                itr = (itr == rcvRlcSduList.end())
                    ? rcvRlcSduList.begin() : itr;

                // retrieve RLC SDU from rcvRlcSduList
                Message* rlcSdu = itr->second;
#ifdef LTE_LIB_LOG
                unsigned int rlcSn = itr->first;
#endif // LTE_LIB_LOG
                rcvRlcSduList.erase(itr++);

#ifdef LTE_LIB_LOG
                lte::LteLog::InfoFormat(node, interfaceIndex,
                        LTE_STRING_LAYER_TYPE_RLC,
                        "txToPdcp,"
                        LTE_STRING_FORMAT_RNTI","
                        "rlcSn:%d,pdcpSn:%d,reEstablishment",
                        rnti.nodeId,
                        rnti.interfaceIndex,
                        rlcSn,
                        LteLayer2GetPdcpSnFromPdcpHeader(rlcSdu));
#endif // LTE_LIB_LOG
                // send RLC SDU to PDCP as re-establishment
                PdcpLteReceivePduFromRlc(
                        node, interfaceIndex, rnti, bearerId, rlcSdu, TRUE);
                rlcData->stats.numSdusSentToUpperLayer++;

                // update transmit window VT(A).
                LteRlcAmIncSN(rlcAmEntity->oldRcvNextSn);
            }
            // reset all element in AM entity
            rlcAmEntity->resetAll();

            if (LteLayer2GetStationType(node, interfaceIndex)
                    == LTE_STATION_TYPE_UE)
            {
                // set target rnti to oppositeRnti of rlcAmEntity
                rlcEntity->oppositeRnti = targetRnti;
            }
            rlcAmEntity->dump(node, interfaceIndex, "EndReEstablishment");
            break;
        }
        case LTE_RLC_ENTITY_INVALID:
        case LTE_RLC_ENTITY_TM:
        case LTE_RLC_ENTITY_UM: {
            ERROR_Assert(FALSE,
                    "RLC entity type is not AM, in case re-establishment");
            break;
        }
        default: {
            ERROR_Assert(FALSE, "Invalid entity Type");
            break;
        }
    }
}



// FUNCTION:: LteRlcDiscardRlcSduFromTransmissionBuffer
// LAYER::    LTE Layer2 RLC
// PURPOSE::  discard the indicated RLC SDU if no segment of the RLC SDU has
//            been mapped to a RLC data PDU yet, when PDCP notified RLC
//            of PDCP PDU discarded
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
        const UInt16& pdcpSn)
{
    LteRlcEntity* rlcEntity
        = LteRlcGetRlcEntity(node, interfaceIndex, rnti, bearerId);
    ERROR_Assert(rlcEntity, "rlcEntity is NULL");

    switch (rlcEntity->entityType) {
        case LTE_RLC_ENTITY_AM: {
            LteRlcAmEntity* rlcAmEntity
                = (LteRlcAmEntity*) rlcEntity->entityData;
            ERROR_Assert(rlcAmEntity, "rlcAmEnity is NULL");
            list<LteRlcAmTxBuffer>& txBuffer = rlcAmEntity->txBuffer;
            list<LteRlcAmTxBuffer>::iterator itr = txBuffer.begin();
            list<LteRlcAmTxBuffer>::iterator itrEnd = txBuffer.end();

            // check whether transmission buffer is empty
            if (itr == itrEnd) {
                // target RLC SDU is already send
                // and dequeued from transmission buffer
                break;
            } else {
                // do nothing
            }

            // check whether PDCP SN of front RLC SDU in transmission buffer
            // is target
            if ((*((UInt16*) MESSAGE_ReturnInfo(
                                itr->message,
                                INFO_TYPE_LteRlcAmPdcpPduInfo))) == pdcpSn) {
                // check whether target RLC SDU is segmented
                if (itr->offset == 0) {
                    // discard RLC SDU before break this case
                }
                // check whether PDCP SN of second RLC SDU
                // in transmission buffer
                else if (++itr == itrEnd) {
                    // txBuffer has only one RLC SDU segmented
                    // do nothing
                    break;
                } else if (*((UInt16*) MESSAGE_ReturnInfo(
                                itr->message,
                                INFO_TYPE_LteRlcAmPdcpPduInfo))
                        == pdcpSn) {
                    // discard RLC SDU before break this case
                    char warning[MAX_STRING_LENGTH];
                    sprintf(warning, "RLC txBuffer has "
                            "managed duplicate PDCP SN %d",pdcpSn);
                    ERROR_ReportWarning(warning);
                } else {
                    // do nothing
                    break;
                }
            }
            // check whether PDCP SN of second RLC SDU in transmission buffer
            else if (++itr != itrEnd && ((*((UInt16*) MESSAGE_ReturnInfo(
                                    itr->message,
                                    INFO_TYPE_LteRlcAmPdcpPduInfo)))
                        == pdcpSn)) {
                // discard RLC SDU before break this case
            } else {
                // target RLC SDU is already send
                // and dequeued from transmission buffer
                break;
            }
            // discard the indicated RLC SDU that was found and not segmented
            rlcAmEntity->txBufSize
                -= MESSAGE_ReturnPacketSize(itr->message);
            ERROR_Assert(rlcAmEntity->txBufSize >= 0,
                    "txBufSize is invalid value");
            LteFreeMsg(node, &(itr->message));
            txBuffer.erase(itr++);

#ifdef LTE_LIB_LOG
            stringstream log;
            if (txBuffer.empty()) {
                log << "[]";
            } else {
                log << "[";
                for (list<LteRlcAmTxBuffer>::const_iterator
                        debugItr = txBuffer.begin(),
                        debugItrEnd = txBuffer.end();
                        debugItr != debugItrEnd; ++debugItr) {
                    log << *((UInt16*) MESSAGE_ReturnInfo(
                                debugItr->message,
                                INFO_TYPE_LteRlcAmPdcpPduInfo))
                        << "(" << debugItr->offset << ") ";
                }
                log << "]";
            }
            lte::LteLog::InfoFormat(node, interfaceIndex,
                    LTE_STRING_LAYER_TYPE_RLC,
                    "discardFromTxBuf,"
                    LTE_STRING_FORMAT_RNTI","
                    "pdcpSn:%d,<-%s",
                    rnti.nodeId,
                    rnti.interfaceIndex,
                    pdcpSn,
                    log.str().c_str());
#endif // LTE_LIB_LOG
            break;
        }
        case LTE_RLC_ENTITY_INVALID:
        case LTE_RLC_ENTITY_TM:
        case LTE_RLC_ENTITY_UM: {
            ERROR_Assert(FALSE, "RLC entity type is not AM");
            break;
        }
        default: {
            ERROR_Assert(FALSE, "Invalid entity Type");
            break;
        }
    }
}



// /**
// FUNCTION   :: LteRlcAmNotifyPdCpOfPdcpStatusReport
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: if positive acknowledgements have been received for all AMD
//               PDUs associated with a transmitted RLC SDU, send an
//               indication to the upper layers of successful delivery of
//               the RLC SDU.
// PARAMETERS ::
// + node               : Node*          : Pointer to node.
// + interfaceIndex     : const int      : Interface index
// + rnti               : const LteRnti& : RNTI
// + bearerId           : const int      : Radio Bearer ID
// + rlcAmEntity        : LteRlcAmEntity : The RLC AM entity
// + receivedRlcAckSn   : const Int32    : SN of received RLC ACK
// RETURN     :: void     : NULL
// **/
static
void LteRlcAmNotifyPdCpOfPdcpStatusReport(
        Node* node,
        const int interfaceIndex,
        const LteRnti& rnti,
        const int bearerId,
        LteRlcAmEntity* rlcAmEntity,
        const Int32 receivedRlcAckSn)
{
    ERROR_Assert(rlcAmEntity, "rlcAmEntity is NULL");
    list<pair<UInt16, UInt16> >& unreceivedRlcAckList
        = rlcAmEntity->unreceivedRlcAckList;

    // clear received RLC ACK SN of that ACK received prior
    // from unreceivedRlcAckList, and managed that list(unRxPdcpAckSet).
    set<UInt16> unRxPdcpAckSet;
    UInt16 frontSn = LTE_RLC_INVALID_RLC_SN; // front SN of receivedRlcAckSn
    while (!unreceivedRlcAckList.empty()) {
        frontSn = unreceivedRlcAckList.front().first;
        if (((frontSn + LTE_RLC_AM_WINDOW_SIZE < LTE_RLC_AM_SN_UPPER_BOUND)
                    && (frontSn < receivedRlcAckSn)
                    && (receivedRlcAckSn
                        <= frontSn + LTE_RLC_AM_WINDOW_SIZE))
                || ((frontSn + LTE_RLC_AM_WINDOW_SIZE
                        >= LTE_RLC_AM_SN_UPPER_BOUND)
                    && ((frontSn < receivedRlcAckSn)
                        || (receivedRlcAckSn + LTE_RLC_AM_SN_UPPER_BOUND
                            <= frontSn + LTE_RLC_AM_WINDOW_SIZE)))) {
            unRxPdcpAckSet.insert(unreceivedRlcAckList.front().second);
            unreceivedRlcAckList.pop_front();
        } else {
            break;
        }
    }
#ifdef LTE_LIB_LOG
    stringstream log;
    log << "(";
    for (set<UInt16>::const_iterator debugSetItr = unRxPdcpAckSet.begin(),
            debugSetItrEnd = unRxPdcpAckSet.end();
            debugSetItr != debugSetItrEnd; ++debugSetItr) {
        log << *debugSetItr << " ";
    }

    list<pair<UInt16, UInt16> >::const_iterator debugItr
        = unreceivedRlcAckList.begin();
    list<pair<UInt16, UInt16> >::const_iterator debugItrEnd
        = unreceivedRlcAckList.end();
    if (debugItr != debugItrEnd) {
        log << "),<-[" << debugItr->first << "(" << debugItr->second << " ";
        UInt16 debugRlcSn = debugItr->first;
        ++debugItr;
        while (debugItr != debugItrEnd) {
            if (debugRlcSn != debugItr->first) {
                log << ") " << debugItr->first
                    << "(" << debugItr->second << " ";
            } else {
                log << debugItr->second << " ";
            }
            debugRlcSn = debugItr->first;
            ++debugItr;
        }
        log << ")]";
    } else {
        log << "),<-[]";
    }
    lte::LteLog::InfoFormat(node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RLC,
            "updateUnRlcAck,"
            LTE_STRING_FORMAT_RNTI","
            "%s",
            rnti.nodeId,
            rnti.interfaceIndex,
            log.str().c_str());
#endif // LTE_LIB_LOG

    // if all RLC ACK for PDCP PDU is received,
    // notify PDCP of PDCP status report
    UInt16 checkPdcpSn = LTE_RLC_INVALID_RLC_SN;
    list<pair<UInt16, UInt16> >::iterator itr
        = unreceivedRlcAckList.begin();
    list<pair<UInt16, UInt16> >::iterator itrEnd
        = unreceivedRlcAckList.end();
    while (!unRxPdcpAckSet.empty()) {
        checkPdcpSn = *(unRxPdcpAckSet.begin());
        unRxPdcpAckSet.erase(checkPdcpSn);
        for (itr = unreceivedRlcAckList.begin();
                itr != itrEnd  && itr->second != checkPdcpSn; ++itr) {
            ;
        }
        if (itr == itrEnd) {
#ifdef LTE_LIB_LOG
            lte::LteLog::InfoFormat(node, interfaceIndex,
                    LTE_STRING_LAYER_TYPE_RLC,
                    "notifyPdcpOfPdcpSn,"
                    LTE_STRING_FORMAT_RNTI","
                    "%d",
                    rnti.nodeId,
                    rnti.interfaceIndex,
                    checkPdcpSn);
#endif // LTE_LIB_LOG
#ifdef DEBUG_RLC_LTE
            // for test
            {
                checkPdcpSn = (checkPdcpSn == 15) ? 20
                    : (checkPdcpSn == 20) ? 15
                    : checkPdcpSn;
            }
#endif // DEBUG_RLC_LTE
            PdcpLteReceivePdcpStatusReportFromRlc(
                    node, interfaceIndex, rnti, bearerId, checkPdcpSn);
        } else {
            // do nothing
        }
    }
}



///////////////////////////////////////////////////////////////////////////
// Class LteRlcHeader member functions
///////////////////////////////////////////////////////////////////////////

// /**
// FUNCTION   :: LteRlcSetMessageRlcHeaderInfo
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Add RLC-PDU (AM entity) header information
//               to the message. (as Info)
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.(source)
// + msg       : Message*      : target message pointer.
// RETURN     :: void : NULL
// **/
void LteRlcSetMessageRlcHeaderInfo(Node* node,
                                   int iface,
                                   LteRlcHeader& rlcHeader,
                                   Message* msg)
{
    int pduHdrSize = LteRlcGetAmPduHeaderSize(node,
                                              iface,
                                              rlcHeader);
    rlcHeader.fixed->headerSize = pduHdrSize;
    MESSAGE_AddVirtualPayload(node, msg, pduHdrSize);

    LteRlcSetMessageRlcHeaderFixedInfo(node,
                                       iface,
                                       rlcHeader,
                                       msg);
    LteRlcSetMessageRlcHeaderSegmentFormatFixedInfo(node,
                                                    iface,
                                                    rlcHeader,
                                                    msg);
    LteRlcSetMessageRlcHeaderExtensionInfo(node,
                                           iface,
                                           rlcHeader,
                                           msg);
}

// /**
// FUNCTION   :: LteRlcSetMessageRlcHeaderFixedInfo
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Add RLC-PDU (AM entity) header information
//               to the message. (as Info)
//               RLC-PDU fixed part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.(source)
// + msg       : Message*      : target message pointer.
// RETURN     :: void : NULL
// **/
void LteRlcSetMessageRlcHeaderFixedInfo(Node* node,
                                        int iface,
                                        LteRlcHeader& rlcHeader,
                                        Message* msg)
{
    if (rlcHeader.fixed != NULL)
    {
        MESSAGE_AppendInfo(node,
            msg,
            sizeof(LteRlcAmPduFormatFixed),
            (unsigned short)INFO_TYPE_LteRlcAmPduFormatFixed);

        LteRlcAmPduFormatFixed* info =
            (LteRlcAmPduFormatFixed*)MESSAGE_ReturnInfo(
            msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduFormatFixed);

        if (info != NULL)
        {
            //*info = *(rlcHeader.fixed);
            memcpy(info,
                   rlcHeader.fixed,
                   sizeof(LteRlcAmPduFormatFixed));
        }
    }
}

// /**
// FUNCTION   :: LteRlcSetMessageRlcHeaderSegmentFormatFixedInfo
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Add RLC-PDU (AM entity) header information
//               to the message. (as Info)
//               RLC-PDU resegment-information part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.(source)
// + msg       : Message*      : target message pointer.
// RETURN     :: void : NULL
// **/
void LteRlcSetMessageRlcHeaderSegmentFormatFixedInfo(Node* node,
                                                     int iface,
                                                     LteRlcHeader& rlcHeader,
                                                     Message* msg)
{
    if (rlcHeader.segFixed != NULL){
        MESSAGE_AppendInfo(
            node,
            msg,
            sizeof(LteRlcAmPduSegmentFormatFixed),
            (unsigned short)INFO_TYPE_LteRlcAmPduSegmentFormatFixed);

        LteRlcAmPduSegmentFormatFixed* info =
            (LteRlcAmPduSegmentFormatFixed*)MESSAGE_ReturnInfo(
            msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduSegmentFormatFixed);

        if (info != NULL)
        {
            //*info = *(rlcHeader.segFixed);
            memcpy(info,
                   rlcHeader.segFixed,
                   sizeof(LteRlcAmPduSegmentFormatFixed));
        }
    }
}

// /**
// FUNCTION   :: LteRlcSetMessageRlcHeaderExtensionInfo
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Add RLC-PDU (AM entity) header information
//               to the message. (as Info)
//               RLC-PDU extention(segment-information:LI) part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.(source)
// + msg       : Message*      : target message pointer.
// RETURN     :: void : NULL
// **/
void LteRlcSetMessageRlcHeaderExtensionInfo(Node* node,
                                            int iface,
                                            LteRlcHeader& rlcHeader,
                                            Message* msg)
{
    if (rlcHeader.extension != NULL)
    {
        rlcHeader.extension->numLengthIndicator
            = rlcHeader.extension->lengthIndicator.size();
        size_t hdrExtSize = sizeof(size_t)
            + sizeof(UInt16) * rlcHeader.extension->numLengthIndicator;

        MESSAGE_AppendInfo(
            node,
            msg,
            hdrExtSize,
            (unsigned short)INFO_TYPE_LteRlcAmPduFormatExtension);

        LteRlcAmPduFormatExtensionMsgInfo* info =
            (LteRlcAmPduFormatExtensionMsgInfo*)MESSAGE_ReturnInfo(
            msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduFormatExtension);

        if (info != NULL)
        {
            memcpy(info,
                   &(rlcHeader.extension->numLengthIndicator),
                   sizeof(size_t));

            UInt16 tmpLiData;
            int i = 0;
            std::vector < UInt16 > ::iterator it
                = rlcHeader.extension->lengthIndicator.begin();
            while (it != rlcHeader.extension->lengthIndicator.end())
            {
                tmpLiData = *it;
                memcpy(&(info->lengthIndicator) + i,
                       &tmpLiData,
                       sizeof(UInt16));
                ++i;
                ++it;
            }
        }

    }
}

// /**
// FUNCTION   :: LteRlcSetMessageStatusPduFormatInfo
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Add RLC-statusPDU to the message. (as Info)
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + msg       : Message*      : target message pointer.
// + status    : LteRlcStatusPduFormat& : RLC-satusPDU structure. (source)
// RETURN     :: void : NULL
// **/
void LteRlcSetMessageStatusPduFormatInfo(
        Node* node,
        int iface,
        Message* msg,
        LteRlcStatusPduFormat& status)
{
    LteRlcSetMessageStatusPduFormatFixedInfo(node,
                                             iface,
                                             status.fixed,
                                             msg);
    LteRlcSetMessageStatusPduFormatExtensionInfo(node,
                                                 iface,
                                                 status.nackSoMap,
                                                 msg);
}

// /**
// FUNCTION   :: LteRlcSetMessageStatusPduFormatFixedInfo
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Add RLC-statusPDU header information to the message.
//               (as Info) RLC-statusPDU fixed part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-statusPDU fixed part structure.(source)
// + msg       : Message*      : target message pointer.
// RETURN     :: void : NULL
// **/
void LteRlcSetMessageStatusPduFormatFixedInfo(
    Node* node,
    int iface,
    LteRlcStatusPduFormatFixed& fixed,
    Message* msg)
{
    MESSAGE_AppendInfo(node,
        msg,
        sizeof(LteRlcStatusPduFormatFixed),
        (unsigned short)INFO_TYPE_LteRlcAmStatusPduPduFormatFixed);
    LteRlcStatusPduFormatFixed* info =
        (LteRlcStatusPduFormatFixed*)MESSAGE_ReturnInfo(
        msg,
        (unsigned short)INFO_TYPE_LteRlcAmStatusPduPduFormatFixed);

    if (info != NULL)
    {
        //*info = fixed;
        memcpy(info,
               &fixed,
               sizeof(LteRlcStatusPduFormatFixed));
    }
}

// /**
// FUNCTION   :: LteRlcSetMessageStatusPduFormatExtensionInfo
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Add RLC-statusPDU header information to the message.
//               (as Info) RLC-statusPDU extention(nack-information) part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + nackSoMap : std::multimap< UInt16, LteRlcStatusPduSoPair >&
//                             : RLC-statusPDU's nack-SO part structure.
//                             : (source)
// + msg       : Message*      : target message pointer.
// RETURN     :: void : NULL
// **/
void LteRlcSetMessageStatusPduFormatExtensionInfo(
    Node* node,
    int iface,
    std::multimap < UInt16, LteRlcStatusPduSoPair > &nackSoMap,
    Message* msg)
{
    if (nackSoMap.empty() != TRUE)
    {
        size_t unitSize = sizeof(LteRlcStatusPduFormatExtension);
        size_t nackInfoSize = nackSoMap.size();

        int size = (int)(unitSize * nackInfoSize + sizeof(size_t));

        MESSAGE_AppendInfo(node,
            msg,
            size,
            (unsigned short)INFO_TYPE_LteRlcAmStatusPduPduFormatExtension);

        LteRlcStatusPduFormatExtensionInfo* info =
            (LteRlcStatusPduFormatExtensionInfo*)MESSAGE_ReturnInfo(
            msg,
            (unsigned short)INFO_TYPE_LteRlcAmStatusPduPduFormatExtension);

        if (info != NULL)
        {
            info->nackInfoSize = nackInfoSize;

            size_t pos = 0;
            LteRlcStatusPduFormatExtension extension;
            std::multimap < UInt16, LteRlcStatusPduSoPair >
                ::iterator it;
            for (it = nackSoMap.begin();
                it != nackSoMap.end();
                it++)
            {
                extension.nackSn = it->first;
                extension.soOffset = it->second;
                memcpy(&(info->nackInfo) + pos, &extension, unitSize);
                ++pos;
            }
        }
    }
}

// /**
// FUNCTION   :: LteRlcGetMessageRlcHeaderFixedInfo
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU (AM entity) header information
//               from the message info-field.
//               RLC-PDU fixed part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// + msg       : Message*      : target message pointer.(source)
// RETURN     :: void : NULL
// **/
void LteRlcGetMessageRlcHeaderFixedInfo(Node* node,
                                        int iface,
                                        LteRlcHeader& rlcHeader,
                                        Message* msg)
{
    if (rlcHeader.fixed != NULL)
    {
        LteRlcAmPduFormatFixed* info =
            (LteRlcAmPduFormatFixed*)MESSAGE_ReturnInfo(
            msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduFormatFixed);

        if (info != NULL)
        {
            //*(rlcHeader.fixed) = *info;
            memcpy(rlcHeader.fixed,
                   info,
                   sizeof(LteRlcAmPduFormatFixed));
        }
    }
}

// /**
// FUNCTION   :: LteRlcGetMessageRlcHeaderSegmentFormatFixedInfo
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU (AM entity) header information
//               from the message info-field.
//               RLC-PDU resegment-information part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// + msg       : Message*      : target message pointer.(source)
// RETURN     :: void : NULL
// **/
void LteRlcGetMessageRlcHeaderSegmentFormatFixedInfo(
                                              Node* node,
                                              int iface,
                                              LteRlcHeader& rlcHeader,
                                              Message* msg)
{
    if (rlcHeader.segFixed != NULL){
        LteRlcAmPduSegmentFormatFixed* info =
            (LteRlcAmPduSegmentFormatFixed*)MESSAGE_ReturnInfo(
            msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduSegmentFormatFixed);

        if (info != NULL)
        {
            //*(rlcHeader.segFixed) = *info;
            memcpy(rlcHeader.segFixed,
                   info,
                   sizeof(LteRlcAmPduSegmentFormatFixed));
        }
    }
}

// /**
// FUNCTION   :: LteRlcGetMessageRlcHeaderExtensionInfo
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) header information
//               from the message info-field.
//               RLC-PDU extension-information(segment information:LI) part.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// + msg       : Message*      : target message pointer.(source)
// RETURN     :: void : NULL
// **/
void LteRlcGetMessageRlcHeaderExtensionInfo(Node* node,
                                            int iface,
                                            LteRlcHeader& rlcHeader,
                                            Message* msg)
{
    if (rlcHeader.extension != NULL)
    {
        LteRlcAmPduFormatExtensionMsgInfo* info =
            (LteRlcAmPduFormatExtensionMsgInfo*)MESSAGE_ReturnInfo(
            msg,
            (unsigned short)INFO_TYPE_LteRlcAmPduFormatExtension);

        if ((info != NULL) && (info->numLengthIndicator > 0))
        {
            rlcHeader.extension->numLengthIndicator
                = info->numLengthIndicator;
            UInt16 tmpLiData;
            for (size_t i = 0; i < info->numLengthIndicator; i++)
            {
                memcpy(&tmpLiData,
                       &(info->lengthIndicator) + i,
                       sizeof(UInt16));
                rlcHeader.extension->lengthIndicator.push_back(tmpLiData);
            }

        }
        else
        {
            rlcHeader.extension->numLengthIndicator = 0;
            rlcHeader.extension->lengthIndicator.clear();
        }
    }
}

// /**
// FUNCTION   :: LteRlcGetAmPduSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) header + data size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// + msg       : Message*      : target message pointer.
// RETURN     :: int : header + data size
// **/
int LteRlcGetAmPduSize(Node* node,
                       int iface,
                       LteRlcHeader& rlcHeader,
                       Message* msg)
{
    return (LteRlcGetAmPduHeaderSize(node, iface, rlcHeader) +
            LteRlcGetAmPduDataSize(node, iface, rlcHeader, msg));
}

// /**
// FUNCTION   :: LteRlcGetAmPduHeaderSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) header size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// RETURN     :: int : header size
// **/
int LteRlcGetAmPduHeaderSize(Node* node,
                             int iface,
                             LteRlcHeader& rlcHeader)
{
    int retSize = 0;

    if (rlcHeader.fixed != NULL)
    {
        if (rlcHeader.fixed->dcPduFlag)
        {
            // PDU format or PDU segment format
            if (rlcHeader.fixed->resegmentationFlag == FALSE){
                retSize = LteRlcGetAmPduFormatSize(node, iface, rlcHeader);
            }
            else
            {
                retSize = LteRlcGetAmPduSegmentFormatSize(
                                                    node, iface, rlcHeader);
            }
        }
    }

    return retSize;
}

// /**
// FUNCTION   :: LteRlcGetAmPduFormatSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) format header size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// RETURN     :: int : PDU format header size
// **/
int LteRlcGetAmPduFormatSize(Node* node,
                             int iface,
                             LteRlcHeader& rlcHeader)
{
    int retSize = 0;

    if (rlcHeader.fixed->resegmentationFlag == FALSE)
    {
        retSize += LteRlcGetAmPduFormatFixedSize(node, iface, rlcHeader);
        if (rlcHeader.extension != NULL){
            if (!rlcHeader.extension->lengthIndicator.empty())
            {
                retSize += LteRlcGetAmPduFormatExtensionSize(node,
                                                             iface,
                                                             rlcHeader);
            }
        }
    }
    return retSize;
}


// /**
// FUNCTION   :: LteRlcGetAmPduFormatFixedSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) header size of no segmented.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// RETURN     :: int : header size
// **/
int LteRlcGetAmPduFormatFixedSize(Node* node,
                                  int iface,
                                  LteRlcHeader& rlcHeader)
{
    return LTE_RLC_AM_DATA_PDU_FORMAT_FIXED_BYTE_SIZE;
}

// /**
// FUNCTION   :: LteRlcGetAmPduFormatFixedSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) header size of no segmented.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// + size      : int           : number of LI
// RETURN     :: int : header size
// **/
int LteRlcGetAmPduLiAndESize(Node* node,
                             int iface,
                             LteRlcHeader& rlcHeader,
                             int size)
{
    if ((size % 2) == 0)
    {
        return (size * LTE_RLC_AM_DATA_PDU_FORMAT_EXTENSION_BIT_SIZE) / 8;
    }
    else
    {
        return ((size * LTE_RLC_AM_DATA_PDU_FORMAT_EXTENSION_BIT_SIZE) +
                LTE_RLC_AM_DATA_PDU_FORMAT_EXTENSION_PADDING_BIT_SIZE) / 8;
    }
}

// /**
// FUNCTION   :: LteRlcGetAmPduFormatExtensionSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) header size of segmented.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// RETURN     :: int : header size
// **/
int LteRlcGetAmPduFormatExtensionSize(
                             Node* node,
                             int iface,
                             LteRlcHeader& rlcHeader)
{
    if (rlcHeader.extension != NULL)
    {
        return LteRlcGetAmPduLiAndESize(
                    node,
                    iface,
                    rlcHeader,
                    rlcHeader.extension->lengthIndicator.size());
    }
    else
    {
        return 0;
    }
}

// /**
// FUNCTION   :: LteRlcGetAmPduFormatExtensionSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) header size of re-segmented.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// RETURN     :: int : header size
// **/
int LteRlcGetAmPduSegmentFormatSize(Node* node,
                                    int iface,
                                    LteRlcHeader& rlcHeader)
{
    int retSize = 0;
    if (rlcHeader.fixed->resegmentationFlag == TRUE)
    {
        retSize += LteRlcGetAmPduSegmentFormatFixedSize(node,
                                                        iface,
                                                        rlcHeader);
        if (rlcHeader.segFixed != NULL)
        {
            retSize += LteRlcGetAmPduSegmentFormatExtensionSize(
                                                        node,
                                                        iface,
                                                        rlcHeader);
        }
    }

    return retSize;
}

// /**
// FUNCTION   :: LteRlcGetAmPduFormatExtensionSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) header's fixed part size of
//               re-segmented.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// RETURN     :: int : header size
// **/
int LteRlcGetAmPduSegmentFormatFixedSize(Node* node,
                                         int iface,
                                         LteRlcHeader& rlcHeader)
{
    return (LTE_RLC_AM_DATA_PDU_FORMAT_FIXED_BYTE_SIZE
            + LTE_RLC_AM_DATA_PDU_RESEGMENT_FORMAT_EXTENSIO_BYTE_SIZE);
}

// /**
// FUNCTION   :: LteRlcGetAmPduFormatExtensionSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) header's extention part size
//               re-segmented.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// RETURN     :: int : header size
// **/
int LteRlcGetAmPduSegmentFormatExtensionSize(Node* node,
                                             int iface,
                                             LteRlcHeader& rlcHeader)
{
    if (rlcHeader.extension != NULL)
    {
    return LteRlcGetAmPduLiAndESize(
                node,
                iface,
                rlcHeader,
                rlcHeader.extension->lengthIndicator.size());
    }
    else
    {
        return 0;
    }
}

// /**
// FUNCTION   :: LteRlcGetAmPduDataSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) data size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// + msg       : Message*      : RLC-PDU messages
// RETURN     :: int : data size
// **/
int LteRlcGetAmPduDataSize(Node* node,
                           int iface,
                           LteRlcHeader& rlcHeader,
                           Message* msg)
{
    return MESSAGE_ReturnPacketSize(msg);
}

// /**
// FUNCTION   :: LteRlcGetAmPduListDataSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) data size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// + msgList   : Message*      : RLC-PDU messages list
// RETURN     :: int : data size
// **/
int LteRlcGetAmPduListDataSize(Node* node,
                               int iface,
                               LteRlcHeader& rlcHeader,
                               Message* msgList)
{
    int size = 0;
    Message* tmp = msgList;

    while (tmp != NULL)
    {
        size += MESSAGE_ReturnPacketSize(tmp);
        tmp = tmp->next;
    }

    return size;
}

// /**
// FUNCTION   :: LteRlcGetExpectedAmPduSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) expected size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// + reSegment : BOOL          : flag of re-segmented
// + msgList   : Message*      : RLC-PDU messages list
// + txBuffData: LteRlcAmTxBuffer& : transmittion buffer
// RETURN     :: int : header + data size
// **/
int LteRlcGetExpectedAmPduSize(Node* node,
                               int iface,
                               LteRlcHeader& rlcHeader,
                               BOOL reSegment,
                               Message* msgList,
                               //Message* addMsg)
                               LteRlcAmTxBuffer& txBuffData)
{
    int size = 0;

    // expect header size
    size += LteRlcGetExpectedAmPduHeaderSize(node,
                                             iface,
                                             rlcHeader,
                                             reSegment);
    if ((rlcHeader.extension == NULL) && (txBuffData.offset != 0))
    {
        size += LTE_RLC_AM_DATA_PDU_FORMAT_EXTENSION_MIN_BYTE_SIZE;
    }

    // expect data size
    size += LteRlcGetExpectedAmPduDataSize(node,
                                           iface,
                                           rlcHeader,
                                           msgList,
                                           //addMsg);
                                           txBuffData.message);

    return size;
}

// /**
// FUNCTION   :: LteRlcGetExpectedAmPduHeaderSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) header expected size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// + reSegment : BOOL          : flag of re-segmented
// RETURN     :: int : header size
// **/
int LteRlcGetExpectedAmPduHeaderSize(Node* node,
                                     int iface,
                                     LteRlcHeader& rlcHeader,
                                     BOOL reSegment)
{
    int size = 0;
    int extSize = 0;

    if (reSegment == FALSE)
    {
        // fixed header size
        size += LTE_RLC_AM_DATA_PDU_FORMAT_FIXED_BYTE_SIZE;

        if ((rlcHeader.firstSdu != TRUE)
            && (rlcHeader.extension == NULL))
        {
            extSize = 1; // first sdu 12bit = 1.5 octet
            extSize++;   // current sdu 12bit = 1.5 octet
        }

        // expand header size
        if (rlcHeader.extension != NULL)
        {
            extSize += rlcHeader.extension->lengthIndicator.size();
            extSize++;
            size += LteRlcGetAmPduLiAndESize(node,
                                             iface,
                                             rlcHeader,
                                             extSize);
        }
    }
    else
    {
        size = rlcHeader.fixed->headerSize;

        if (rlcHeader.segFixed == NULL)
        {
            size += LTE_RLC_AM_DATA_PDU_RESEGMENT_FORMAT_EXTENSIO_BYTE_SIZE;
        }
    }

    return size;
}

// /**
// FUNCTION   :: LteRlcGetExpectedAmPduDataSize
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: Get RLC-PDU(AM entity) data expected size.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// + msgList   : Message*      : RLC-PDU messages list
// + addMsg    : Message*      : RLC-PDU messages
// RETURN     :: int : data size
// **/
int LteRlcGetExpectedAmPduDataSize(Node* node,
                                   int iface,
                                   LteRlcHeader& rlcHeader,
                                   Message* msgList,
                                   Message* addMsg)
{
    int size = 0;
    Message* tmp = msgList;

    while (tmp != NULL)
    {
        size += MESSAGE_ReturnPacketSize(tmp);
        tmp = tmp->next;
    }

    size += MESSAGE_ReturnPacketSize(addMsg);
    return size;
}

// /**
// FUNCTION   :: LteRlcAllocAmPduLi
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: allocate LI field for RLC-PDU(AM entity) header.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// + size      : UInt16        : LI (size of massage)
// RETURN     :: void : NULL
// **/
void LteRlcAllocAmPduLi(Node* node,
                        int iface,
                        LteRlcHeader& rlcHeader,
                        UInt16 size)
{
    if (rlcHeader.extension == NULL)
    {
        rlcHeader.extension = new LteRlcAmPduFormatExtension();
    }

    rlcHeader.extension->numLengthIndicator++;
    rlcHeader.extension->lengthIndicator.push_back(size);

    return;
}


// /**
// FUNCTION   :: LteRlcAddAmPduLi
// LAYER      :: LTE Layer2 RLC
// PURPOSE    :: add LI to RLC-PDU(AM entity) header.
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + iface     : UInt32        : interface index
// + rlcHeader : LteRlcHeader  : RLC-PDU header structure.
// + msg       : Message*      : massage
// RETURN     :: void : NULL
// **/
void LteRlcAddAmPduLi(Node* node,
                      int iface,
                      LteRlcHeader& rlcHeader,
                      Message* msg)
{
    int size = MESSAGE_ReturnPacketSize(msg);

    LteRlcAllocAmPduLi(node, iface, rlcHeader, (UInt16)size);

    return;
}

// /**
// FUNCTION::       LteRlcAmReTxBuffer::getPduSize
// LAYER::          LTE Layer2 RLC
// PURPOSE::        get RLC-PDU size
// PARAMETERS::
//    + node  : Node* : pointer to the network node
//    + iface : int   : interdex of interface
// RETURN::         int : PDU size
// **/
int LteRlcAmReTxBuffer::getPduSize(Node* node, int iface)
{
    return LteRlcGetAmPduSize(node, iface, rlcHeader, message);
}

// /**
// FUNCTION::       LteRlcAmReTxBuffer::getNackedSize
// LAYER::          LTE Layer2 RLC
// PURPOSE::        get RLC-PDU NACKed size
// PARAMETERS::
//    + node  : Node* : pointer to the network node
//    + iface : int   : interdex of interface
// RETURN::         int : NACKed data size
// **/
int LteRlcAmReTxBuffer::getNackedSize(Node* node, int iface)
{
    UInt32 retSize = 0;
    std::map < int, LteRlcStatusPduSoPair > ::iterator
        it = nackField.begin();
    while (it != nackField.end())
    {
        ERROR_Assert((*it).second.soEnd >= (*it).second.soStart,
            "RLC-LTE SegmentOffset field: start > end !");
        retSize = (*it).second.soEnd - (*it).second.soStart;
        it++;
    }
    return retSize;
}


///////////////////////////////////////////////////////////////////////////
// API functions
///////////////////////////////////////////////////////////////////////////

// /**
// FUNCTION::       RlcLteNotifyPowerOn
// LAYER::          LTE Layer2 RLC
// PURPOSE::        RLC Power On
//
// PARAMETERS::
//    + node  : Node* : pointer to the network node
//    + iface : int   : interdex of interface
// RETURN::         NULL
// **/
void RlcLteNotifyPowerOn(Node* node, int iface)
{
    LteRlcData* lteRlcData =
                       LteLayer2GetLteRlcData(node, iface);
    lteRlcData->status = LTE_RLC_POWER_ON;

#ifdef LTE_LIB_LOG
    {
        std::stringstream log;
        log.str("");
        log << "RlcLteNotifyPowerOn:" << ",";
        lte::LteLog::InfoFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_RLC_CONTROL,
            log.str().c_str());
    }
#endif // LTE_LIB_LOG
}

// /**
// FUNCTION::       RlcLteNotifyPowerOff
// LAYER::          LTE Layer2 RLC
// PURPOSE::        RLC Power Off
//
// PARAMETERS::
//    + node  : Node* : pointer to the network node
//    + iface : int   : interdex of interface
// RETURN::         NULL
// **/
void RlcLteNotifyPowerOff(Node* node, int iface)
{
    // Traversing RNTI-Bearer tree to reset entity is done at Layer 3.
    // This function shold clear things
    // independent of upper layer information.
    LteRlcData* lteRlc = LteLayer2GetLteRlcData(node, iface);
    lteRlc->status = LTE_RLC_POWER_OFF;
}

// /**
// FUNCTION::       LteRlcEstablishment
// LAYER::          LTE Layer2 RLC
// PURPOSE::        RLC Establishment
//
// PARAMETERS::
//    + node     : Node* : pointer to the network node
//    + iface    : int   : interdex of interface
//    + srcRnti  : const LteRnti& : RNTI
//    + bearerId : const int      : bearer ID
// RETURN::         NULL
// **/
void LteRlcEstablishment(Node* node,
                         int iface,
                         const LteRnti& srcRnti,
                         const int bearerId)
{

}

// /**
// FUNCTION::       LteRlcInitRlcEntity
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Init RLC Entity
//
// PARAMETERS::
//    + node   : Node*               : Pointer to node.
//    + iface  : unsigned int        : Interdex of interface
//    + rnti   : const LteRnti&      : RNTI
//    + type   : LteRlcEntityType&   : RLC Entity Type
//    + direct : LteRlcEntityDirect& : RLC Entity direction
//    + entity : LteRlcEntity*       : RLC Entity
// RETURN::         NULL
// **/
void LteRlcInitRlcEntity(Node* node,
                         int iface,
                         const LteRnti& rnti,
                         const LteRlcEntityType& type,
                         const LteRlcEntityDirect& direct,
                         LteRlcEntity* entity)
{
    LteRlcData* lteRlcData =
                       LteLayer2GetLteRlcData(node, iface);
    entity->node = node;
    entity->bearerId = LTE_DEFAULT_BEARER_ID;
    entity->oppositeRnti = rnti;
    entity->entityType = type;
    entity->entityData = NULL;

    switch (entity->entityType){
        case LTE_RLC_ENTITY_AM:
        {
            LteRlcInitAmEntity(node,
                               iface,
                               rnti,
                               lteRlcData,
                               direct,
                               entity);
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "RLC LTE: entity type Error!");
        }
    }
}

// /**
// FUNCTION::       LteRlcRelease
// LAYER::          LTE Layer2 RLC
// PURPOSE::        RLC Release
//
// PARAMETERS::
//    + node     : Node*          : pointer to the network node
//    + iface    : unsigned int   : interdex of interface
//    + srcRnti  : const LteRnti& : RNTI
//    + bearerId : const int      : bearer ID
// RETURN::         NULL
// **/
void LteRlcRelease(Node* node,
                         int iface,
                         const LteRnti& srcRnti,
                         const int bearerId)
{
    char errorMsg[MAX_STRING_LENGTH] = "LteRlcRelease: ";
    LteRlcEntity* entity =
        LteRlcGetRlcEntity(node, iface, srcRnti, bearerId);

    switch (entity->entityType)
    {
        case LTE_RLC_ENTITY_AM:
        {
            LteRlcAmEntity* amEntity =
                (LteRlcAmEntity*)entity->entityData;
            ERROR_Assert(amEntity, "amEntity is NULL");
#ifndef LTE_LIB_RLC_ENTITY_VAR
            amEntity->entityVar = entity;
#endif

            LteRlcAmEntityFinalize(node, iface, amEntity);
            entity->entityData = NULL;

            break;
        }
        default:
        {
            sprintf(errorMsg+strlen(errorMsg), "wrong RLC entity type.");
            ERROR_ReportError(errorMsg);
        }
    }
}


// /**
// FUNCTION::       LteRlcInit
// LAYER::          LTE Layer2 RLC
// PURPOSE::        RLC Initalization
//
// PARAMETERS::
//    + node      : Node*            : pointer to the network node
//    + iface     : unsigned int     : interdex of interface
//    + nodeInput : const NodeInput* : Input from configuration file
// RETURN::         NULL
// **/
void LteRlcInit(Node* node,
                 unsigned int iface,
                 const NodeInput* nodeInput)
{
    Layer2DataLte* layer2DataLte =
                       LteLayer2GetLayer2DataLte(node, iface);

    // Init statistics object.
    LteRlcData* lteRlcData
        = (LteRlcData*) MEM_malloc(sizeof(LteRlcData));
    ERROR_Assert(lteRlcData != NULL, "RLC LTE: Out of memory!");
    memset(lteRlcData, 0, sizeof(LteRlcData));
    layer2DataLte->lteRlcVar = lteRlcData;

    // init configureble parameters
    LteRlcInitConfigurableParameters(node, iface, nodeInput);

    // init dyanamic statistics
    LteRlcInitDynamicStats(node, iface, lteRlcData);
}

// /**
// FUNCTION::       LteRlcFinalize
// LAYER::          LTE Layer2 RLC
// PURPOSE::        RLC finalization function
// PARAMETERS::
//    + node  : Node*        : pointer to the network node
//    + iface : unsigned int : interdex of interface
// RETURN::         NULL
// **/
void LteRlcFinalize(Node* node, unsigned int iface)
{
    Layer2DataLte* layer2DataLte =
                       LteLayer2GetLayer2DataLte(node, iface);
    LteRlcData* lteRlcData =
                       LteLayer2GetLteRlcData(node, iface);

    LteRlcPrintStat(node, (int)iface, lteRlcData);

    MEM_free(layer2DataLte->lteRlcVar);
    layer2DataLte->lteRlcVar = NULL;
}

// /**
// FUNCTION::       LteRlcProcessEvent
// LAYER::          LTE Layer2 RLC
// PURPOSE::        RLC event handling function
// PARAMETERS::
//    + node    : Node*        : pointer to the network node
//    + iface   : unsigned int : interdex of interface
//    + message : Message*     : Message to be handled
// RETURN::         NULL
// **/
void LteRlcProcessEvent(Node* node, unsigned int iface, Message* msg)
{
    LteRlcData* lteRlcData
        = LteLayer2GetLteRlcData(node, iface);

    if (lteRlcData->status == LTE_RLC_POWER_OFF)
    {
        MESSAGE_Free(node, msg);
        return;
    }

    LteRlcTimerInfoHdr* info =
        (LteRlcTimerInfoHdr*)MESSAGE_ReturnInfo(msg);
    LteRlcEntity* lteRlcEntity;

    if (info != NULL)
    {
        LteRnti rnti = info->rnti;
        int bearerId = info->bearerId;

        lteRlcEntity
            = LteRlcGetRlcEntity(node, iface, rnti, bearerId);

        if (lteRlcEntity->entityType == LTE_RLC_ENTITY_AM)
        {
            LteRlcAmEntity* amEntity
                = (LteRlcAmEntity*)lteRlcEntity->entityData;
#ifndef LTE_LIB_RLC_ENTITY_VAR
            amEntity->entityVar = lteRlcEntity;
#endif

            switch (msg->eventType)
            {
                case MSG_RLC_LTE_PollRetransmitTimerExpired:
                {
                    LteRlcAmHandlePollRetrunsmit(node,
                                                 iface,
                                                 amEntity);
                    break;
                }
                case MSG_RLC_LTE_ReorderingTimerExpired:
                {
                    LteRlcAmHandleReordering(node,
                                             iface,
                                             amEntity);
                    break;
                }
                case MSG_RLC_LTE_StatusProhibitTimerExpired:
                {
                    LteRlcAmHandleStatusProhibit(node,
                                                 iface,
                                                 amEntity);
                    break;
                }
                case MSG_RLC_LTE_ResetTimerExpired:
                {
                    LteRlcAmHandleReset(node,
                                        iface,
                                        amEntity);

                    break;
                }
                default:
                {
                    break;
                }
            }
        }
        else
        {
            ERROR_Assert(FALSE, "RLC LTE: entity type Error!");
        }
    }
    else
    {
        ERROR_Assert(FALSE,
                    "RLC LTE: Timer message info-field is not found.");
    }

    MESSAGE_Free(node, msg);
}


// /**
// FUNCTION::       LteRlcReceiveSduFromPdcp
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Receive SDU from PDCP layer
//
// PARAMETERS::
//    + node     : Node*          : Pointer to node.
//    + iface    : unsigned int   : Interdex of interface
//    + dstRnti  : const LteRnti& : RNTI
//    + bearerId : const int      : bearer ID
//    + rlcSdu   : Message*       : SDU massage list
// RETURN::         NULL
// **/
void LteRlcReceiveSduFromPdcp(
    Node* node,
    int iface,
    const LteRnti& dstRnti,
    const int bearerId,
    Message* rlcSdu)
{
    ERROR_Assert(rlcSdu, "RLC SDU from PDCP is NULL");
#ifdef LTE_LIB_LOG
    {
        lte::LteLog::InfoFormat(node, iface,
                LTE_STRING_LAYER_TYPE_RLC,
                "rxFromPdcp,"
                LTE_STRING_FORMAT_RNTI","
                "%d",
                dstRnti.nodeId,
                dstRnti.interfaceIndex,
                LteLayer2GetPdcpSnFromPdcpHeader(rlcSdu));
    }
#endif // LTE_LIB_LOG

    Message* dupMsg = MESSAGE_Duplicate(node, rlcSdu);
    MESSAGE_FreeList(node, rlcSdu);

    char errorMsg[MAX_STRING_LENGTH] = "LteRlcReceiveSduFromPdcp: ";
    LteRlcData* lteRlc = LteLayer2GetLteRlcData(node, iface);
    LteRlcEntity* entity =
        LteRlcGetRlcEntity(node, iface, dstRnti, bearerId);

    if (!entity)
    {
        MESSAGE_Free(node, dupMsg);
        ERROR_Assert(FALSE, "LteRlcReceiveSduFromPdcp: entity error");
        return;
    }

    lteRlc->stats.numSdusReceivedFromUpperLayer++;

    switch (entity->entityType)
    {
        case LTE_RLC_ENTITY_AM:
        {
            LteRlcAmEntity* amEntity =
                (LteRlcAmEntity*)entity->entityData;
#ifndef LTE_LIB_RLC_ENTITY_VAR
            amEntity->entityVar = entity;
#endif

            LteRlcAmUpperLayerHasSduToSend(
                node,
                iface,
                amEntity,
                dupMsg);
            break;
        }
        default:
        {
            MESSAGE_Free(node, dupMsg);
            sprintf(errorMsg+strlen(errorMsg), "wrong RLC entity type.");
            ERROR_ReportError(errorMsg);
        }
    }
}

// /**
// FUNCTION::       LteRlcDeliverPduToMac
// LAYER::          LTE Layer2 RLC
// PURPOSE::        Deliver SDU to MAC layer
//
// PARAMETERS::
//    + node     : Node*          : Pointer to node.
//    + iface    : unsigned int   : Interdex of interface
//    + dstRnti  : const LteRnti& : RNTI
//    + bearerId : const int      : bearer ID
//    + size     : int            : dequeue size
//    + sduList  : std::list<Message*>* : RLC-PDU massage list
// RETURN::         NULL
// **/
void LteRlcDeliverPduToMac(
    Node* node,
    int iface,
    const LteRnti& dstRnti,
    const int bearerId,
    int size,
    std::list < Message* >* sduList)
{
    sduList->clear();

    char errorMsg[MAX_STRING_LENGTH] = "LteRlcDeliverPduToMac ";
    LteRlcEntity* entity =
        LteRlcGetRlcEntity(node, iface, dstRnti, bearerId);

    sprintf(errorMsg+strlen(errorMsg), "Can't find existing RLC entity "
        "given NodeId: %u, iface: %d, bearerId: %u",
        dstRnti.nodeId, dstRnti.interfaceIndex, bearerId);
    ERROR_Assert(entity, errorMsg);

    switch (entity->entityType)
    {
        case LTE_RLC_ENTITY_AM:
        {
            LteRlcAmEntity* amEntity =
                (LteRlcAmEntity*)entity->entityData;
#ifndef LTE_LIB_RLC_ENTITY_VAR
            amEntity->entityVar = entity;
#endif

            LteRlcAmDeliverPduToMac(node,
                                    iface,
                                    amEntity,
                                    size,
                                    sduList);
#ifdef LTE_RLC_MSG_TEST
            if (sduList->empty() != TRUE)
            {
                std::list < Message* > ::iterator logPduListIt;
                for (logPduListIt = (sduList)->begin();
                    logPduListIt != (sduList)->end();
                    logPduListIt++)
                {
                    MESSAGE_logMessage(
                        node,
                        iface,
                        *logPduListIt,
                        LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION,
                        "LteRlcAmDeliverPduToMac(send to MAC)");
                }
            }
#endif // LTE_RLC_MSG_TEST

            break;
        }
        default:
        {
            sprintf(errorMsg+strlen(errorMsg), "wrong RLC entity type.");
            ERROR_ReportError(errorMsg);
        }
    }
}

// /**
// FUNCTION::       LteRlcSendableByteInQueue
// LAYER::          LTE Layer2 RLC
// PURPOSE::        retuern sendable data size(byte) in queue
//                  (sum of all buffer)
//
// PARAMETERS::
//    + node     : Node*          : Pointer to node.
//    + iface    : unsigned int   : Interdex of interface
//    + dstRnti  : const LteRnti& : RNTI
//    + bearerId : const int      : bearer ID
// RETURN::         UInt32 : sendable data size
// **/
UInt32 LteRlcSendableByteInQueue(
    Node* node,
    int iface,
    const LteRnti& dstRnti,
    const int bearerId)
{
    UInt32 retByte = 0;

    char errorMsg[MAX_STRING_LENGTH] = "LteRlcDeliverPduToMac ";
    LteRlcEntity* entity =
        LteRlcGetRlcEntity(node, iface, dstRnti, bearerId);
    std::list < Message* > msgList;

    sprintf(errorMsg+strlen(errorMsg), "Can't find existing RLC entity "
        "given NodeId: %u, NodeIndex: %u, iface: %d, bearerId: %u",
        dstRnti.nodeId, node->nodeIndex, iface, bearerId);
    ERROR_Assert(entity, errorMsg);

    switch (entity->entityType)
    {
        case LTE_RLC_ENTITY_AM:
        {
            LteRlcAmEntity* amEntity =
                (LteRlcAmEntity*)entity->entityData;
            retByte = amEntity->getBufferSize(node,
                                              iface);
            break;
        }
        default:
        {
            sprintf(errorMsg+strlen(errorMsg), "wrong RLC entity type.");
            ERROR_ReportError(errorMsg);
        }
    }

    return retByte;
}

// /**
// FUNCTION::       LteRlcSendableByteInTxQueue
// LAYER::          LTE Layer2 RLC
// PURPOSE::        retuern sendable data size(byte) in transmittion buffer
//
// PARAMETERS::
//    + node     : Node*          : Pointer to node.
//    + iface    : unsigned int   : Interdex of interface
//    + dstRnti  : const LteRnti& : RNTI
//    + bearerId : const int      : bearer ID
// RETURN::         UInt32 : sendable data size
// **/
UInt32 LteRlcSendableByteInTxQueue(
    Node* node,
    int iface,
    const LteRnti& dstRnti,
    const int bearerId)
{
    UInt32 retByte = 0;

    // Transmission Buffer size
    // (without Retransmission/Status PDU Buffer size)
    LteRlcEntity* entity =
        LteRlcGetRlcEntity(node, iface, dstRnti, bearerId);
    if (entity == NULL)
    {
        retByte = 0;
    }
    else
    {
        switch (entity->entityType)
        {
            case LTE_RLC_ENTITY_AM:
            {
                LteRlcAmEntity* amEntity =
                    (LteRlcAmEntity*)entity->entityData;
                retByte += (UInt32)amEntity->txBufSize;
                break;
            }
            default:
            {
                retByte = 0;
            }
        }
    }

    return retByte;
}

// /**
// FUNCTION::       LteRlcReceivePduFromMac
// LAYER::          LTE Layer2 RLC
// PURPOSE::        receive PDU from MAC layer
//
// PARAMETERS::
//    + node     : Node*          : Pointer to node.
//    + iface    : unsigned int   : Interdex of interface
//    + dstRnti  : const LteRnti& : RNTI
//    + bearerId : const int      : bearer ID
//    + pduList  : std::list<Message*>* : RLC-PDU massage list
// RETURN::         NULL
// **/
void LteRlcReceivePduFromMac(
    Node* node,
    int iface,
    const LteRnti& srcRnti,
    const int bearerId,
    std::list < Message* > &pduList)
{
#ifdef LTE_RLC_MSG_TEST
    if (pduList.empty() != TRUE)
    {
        std::list < Message* > ::iterator logPduListIt;
        for (logPduListIt = (pduList).begin();
           logPduListIt != (pduList).end();
           logPduListIt++)
        {
           MESSAGE_logMessage(node,
                              iface,
                              *logPduListIt,
                              LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                              "LteRlcReceivePduFromMac(recieved from MAC)");
        }
    }
#endif // LTE_RLC_MSG_TEST

    char errorMsg[MAX_STRING_LENGTH] = "LteRlcReceivePduFromMac ";
    LteRlcData* lteRlc = LteLayer2GetLteRlcData(node, iface);
    LteRlcEntity* entity =
        LteRlcGetRlcEntity(node, iface, srcRnti, bearerId);

    switch (entity->entityType)
    {
        case LTE_RLC_ENTITY_AM:
        {
            LteRlcAmEntity* amEntity =
                (LteRlcAmEntity*)entity->entityData;
#ifndef LTE_LIB_RLC_ENTITY_VAR
            amEntity->entityVar = entity;
#endif

            LteRlcAmReceivePduFromMac(node,
                                      iface,
                                      srcRnti,
                                      bearerId,
                                      amEntity,
                                      pduList);

            if (amEntity->rcvRlcSduList.empty())
            {
                break;
            }

            // send to pdcp
            std::multimap < unsigned int, Message* > ::iterator it;
            UInt32 chkPduNum = LteRlcAmSeqNumDist(
                                amEntity->oldRcvNextSn, amEntity->rcvNextSn);
            for (UInt32 i = 0; i < chkPduNum; i++)
            {
                std::multimap < unsigned int, Message* > ::iterator it =
                    amEntity->rcvRlcSduList.lower_bound(
                                                    amEntity->oldRcvNextSn);
                std::multimap < unsigned int, Message* > ::iterator it_end =
                    amEntity->rcvRlcSduList.upper_bound(
                                                    amEntity->oldRcvNextSn);

                while (it != it_end)
                {
                    Message* toPdcpMsg = MESSAGE_Duplicate(node, it->second);
#ifdef LTE_LIB_LOG
                    lte::LteLog::DebugFormat(
                        node,
                        iface,
                        LTE_STRING_LAYER_TYPE_RLC,
                        "%s,"
                        "ExtractFromSduList,%d",
                        LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                        it->first);
#endif


#ifdef LTE_RLC_MSG_TEST
                    MESSAGE_logMessage(
                        node,
                        iface,
                        toPdcpMsg,
                        LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION,
                        "LteRlcReceivePduFromMac(send to PDCP)");
#endif // LTE_RLC_MSG_TEST

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
                    LteLayer2ValidationData* valData =
                        LteLayer2GetValidataionData(
                            node, iface, entity->oppositeRnti);
                    if (valData != NULL)
                    {
                        valData->rlcNumSduToPdcp++;
                        valData->rlcBitsToPdcp +=
                            MESSAGE_ReturnPacketSize(toPdcpMsg) * 8;
                    }
#endif
#endif
                    MESSAGE_Free(node, it->second);
#ifdef LTE_LIB_LOG
                    lte::LteLog::InfoFormat(node, iface,
                            LTE_STRING_LAYER_TYPE_RLC,
                            "txToPdcp,"
                            LTE_STRING_FORMAT_RNTI","
                            "rlcSn:%d,pdcpSn:%d",
                            srcRnti.nodeId,
                            srcRnti.interfaceIndex,
                            it->first,
                            LteLayer2GetPdcpSnFromPdcpHeader(toPdcpMsg));
#endif // LTE_LIB_LOG
                    PdcpLteReceivePduFromRlc(node,
                                             iface,
                                             srcRnti,
                                             bearerId,
                                             toPdcpMsg,
                                             FALSE);
                    lteRlc->stats.numSdusSentToUpperLayer++;
                    amEntity->rcvRlcSduList.erase(it++);
                }
                LteRlcAmIncSN(amEntity->oldRcvNextSn);
            }
            // When Reassemble is finished, if there are remaining SDUsegment
            // SN is decremented because Reassemble
            if (amEntity->rcvRlcSduSegList.size() != 0)
            {
                LteRlcAmDecSN(amEntity->oldRcvNextSn);
            }
            break;
        }
        default:
        {
            sprintf(errorMsg+strlen(errorMsg), "wrong RLC entity type.");
            ERROR_ReportError(errorMsg);
        }
    }
}

// /**
// FUNCTION::       LteRlcCheckDeliverPduToMac
// LAYER::          LTE Layer2 RLC
// PURPOSE::        check deliver PDU to MAC layer
//
// PARAMETERS::
//    + node     : Node*          : Pointer to node.
//    + iface    : unsigned int   : Interdex of interface
//    + dstRnti  : const LteRnti& : RNTI
//    + bearerId : const int      : bearer ID
//    + size     : int            : dequeue size
//    + sduList  : std::list<Message*>* : RLC-PDU massage list
// RETURN::         NULL
// **/
int LteRlcCheckDeliverPduToMac(
    Node* node,
    int iface,
    const LteRnti& dstRnti,
    const int bearerId,
    int size,
    std::list < Message* >* sduList)
{
    int retSize = 0;

    char errorMsg[MAX_STRING_LENGTH] = "LteRlcCheckDeliverPduToMac ";
    LteRlcEntity* entity =
        LteRlcGetRlcEntity(node, iface, dstRnti, bearerId);
    if (entity != NULL)
    {
        switch (entity->entityType)
        {
            case LTE_RLC_ENTITY_AM:
            {
                int size = 0;
                int pduNum = 0;
                LteRlcAmEntity* amEntity =
                    (LteRlcAmEntity*)entity->entityData;
                size += amEntity->txBufSize;
                size += amEntity->retxBufNackSize;
                size += amEntity->statusBufSize;

                pduNum += amEntity->txBuffer.size();
                pduNum += amEntity->statusBuffer.size();

                std::list < LteRlcAmReTxBuffer > ::iterator it;
                std::map < int, LteRlcStatusPduSoPair > ::iterator nackIt;
                for (it = amEntity->retxBuffer.begin();
                    it != amEntity->retxBuffer.end();
                    it++)
                {
                    pduNum += it->nackField.size();
                }
                Message* msg = NULL;
                sduList->resize(pduNum, msg);
                break;
            }
            default:
            {
                sprintf(errorMsg+strlen(errorMsg), "wrong RLC entity type.");
                ERROR_ReportError(errorMsg);
            }
        }
    }

    return retSize;
}

// /**
// FUNCTION::       LteRlcGetQueueSizeInByte
// LAYER::          LTE Layer2 RLC
// PURPOSE::        get dequeue size(byte)
//
// PARAMETERS::
//    + node     : Node*          : Pointer to node.
//    + iface    : unsigned int   : Interdex of interface
//    + dstRnti  : const LteRnti& : RNTI
//    + bearerId : const int      : bearer ID
// RETURN::         UInt32 : dequeue size
// **/
UInt32 LteRlcGetQueueSizeInByte(
    Node* node,
    int iface,
    const LteRnti& dstRnti,
    const int bearerId)
{
    char errorMsg[MAX_STRING_LENGTH] = "LteRlcGetQueueSizeInByte ";
    LteRlcEntity* entity =
        LteRlcGetRlcEntity(node, iface, dstRnti, bearerId);
    if (entity != NULL)
    {
        switch (entity->entityType)
        {
            case LTE_RLC_ENTITY_AM:
            {
                return LTE_RLC_DEF_MAXBUFSIZE;
            }
            default:
            {
                sprintf(errorMsg+strlen(errorMsg), "wrong RLC entity type.");
                ERROR_ReportError(errorMsg);
            }
        }
    }

    return 0;
}

// /**
// FUNCTION::       LteRlcAmIndicateResetWindow
// LAYER::          LTE Layer2 RLC
// PURPOSE::        reset AM entity
//
// PARAMETERS::
//    + node         : Node*          : Pointer to node.
//    + iface        : unsigned int   : Interdex of interface
//    + oppositeRnti : const LteRnti& : RNTI
//    + bearerId     : const int      : bearer ID
// RETURN::         NULL
// **/
void LteRlcAmIndicateResetWindow(
    Node* node,
    int iface,
    const LteRnti& oppositeRnti,
    const int bearerId)
{
    LteRlcEntity* entity =
        LteRlcGetRlcEntity(node, iface, oppositeRnti, bearerId);
    LteRlcAmEntity* amEntity =
        (LteRlcAmEntity*)entity->entityData;

#ifdef LTE_LIB_LOG
        lte::LteLog::InfoFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "Reset,"
            LTE_STRING_FORMAT_RNTI","
            "RxSide,"
            "BearerId=,%d",
            oppositeRnti.nodeId,
            oppositeRnti.interfaceIndex,
            bearerId);
#endif

    amEntity->resetAll(TRUE);
    amEntity->resetFlg = TRUE;
    LteRlcAmInitResetTimer(node, iface, amEntity, LTE_RLC_ENTITY_TX);
}

// /**
// FUNCTION::       LteRlcAmEntity::resetAll
// LAYER::          LTE Layer2 RLC
// PURPOSE::        reset all element in AM entity
//
// PARAMETERS::
//    + withoutResetProcess : BOOL : desabele CancelSelfMsg()
// RETURN::         NULL
// **/
void LteRlcAmEntity::resetAll(BOOL withoutResetProcess)
{
#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(
        entityVar->node,
        ANY_INTERFACE,
        LTE_STRING_LAYER_TYPE_RLC,
        "Reset,"
        LTE_STRING_FORMAT_RNTI","
        "resetAll,"
        "BearerId=,%d",
        entityVar->oppositeRnti.nodeId,
        entityVar->oppositeRnti.interfaceIndex,
        entityVar->bearerId);
#endif
    // reset tx buffer
    std::for_each(
        txBuffer.begin(),
        txBuffer.end(),
        LteRlcFreeMessageData < LteRlcAmTxBuffer > (entityVar->node));
    txBuffer.clear();
    txBufSize = 0;

    // reset latestPdu
    LteRlcDelAmPduHdrObj(latestPdu.rlcHeader);
    MESSAGE_FreeList(entityVar->node, latestPdu.message);
    latestPdu.message = NULL;

    // reset retx buffer
    std::list < LteRlcAmReTxBuffer > ::iterator
        it = retxBuffer.begin();
    while (it != retxBuffer.end())
    {
        LteRlcDelAmPduHdrObj((*it).rlcHeader);
        ++it;
    }
    std::for_each(
        retxBuffer.begin(),
        retxBuffer.end(),
        LteRlcFreeMessageData < LteRlcAmReTxBuffer > (entityVar->node));
    retxBuffer.clear();
    retxBufSize = 0;
    retxBufNackSize = 0;

    // reset status pdu buffer
    statusBuffer.clear();
    statusBufSize = 0;

    // reset rx sdu buffer
    std::multimap < unsigned int, Message* > ::iterator msgItr;
    for (msgItr  = rcvRlcSduList.begin();
        msgItr != rcvRlcSduList.end();
        ++msgItr)
    {
        MESSAGE_FreeList(entityVar->node, msgItr->second);
    }
    rcvRlcSduList.clear();
    std::list < LteRlcSduSegment > ::iterator sduItr;
    for (sduItr  = rcvRlcSduSegList.begin();
        sduItr != rcvRlcSduSegList.end();
        ++sduItr)
    {
        MESSAGE_FreeList(entityVar->node, (*sduItr).newMsg);
    }
    rcvRlcSduSegList.clear();
    for (sduItr  = rcvRlcSduTempList.begin();
        sduItr != rcvRlcSduTempList.end();
        ++sduItr)
    {
        MESSAGE_FreeList(entityVar->node, (*sduItr).newMsg);
    }
    rcvRlcSduTempList.clear();

    // reset unreceived ACK map
    unreceivedRlcAckList.clear();

    // reset rx pdu buffer
    std::map < unsigned int, LteRlcAmRcvPduData > ::iterator pduItr;
    for (pduItr  = rcvPduBuffer.begin();
        pduItr != rcvPduBuffer.end();
        ++pduItr)
    {
        pduItr->second.lengthIndicator.clear();
        MESSAGE_FreeList(entityVar->node, pduItr->second.message);
        pduItr->second.rcvdPduSegmentInfo.clear();
        pduItr->second.rcvNackedSegOffset.clear();
    }
    rcvPduBuffer.clear();

    // reset timer
    if (pollRetransmitTimerMsg != NULL){
        MESSAGE_CancelSelfMsg(entityVar->node,
                              pollRetransmitTimerMsg);
        pollRetransmitTimerMsg = NULL;
    }

    if (reoderingTimerMsg != NULL){
        MESSAGE_CancelSelfMsg(entityVar->node,
                              reoderingTimerMsg);
        reoderingTimerMsg = NULL;
    }

    if (statusProhibitTimerMsg != NULL){
        MESSAGE_CancelSelfMsg(entityVar->node,
                              statusProhibitTimerMsg);
        statusProhibitTimerMsg = NULL;
    }

    // Tx window
    ackSn = 0;
    sndWnd = LTE_RLC_AM_WINDOW_SIZE;
    sndNextSn = 0;
    pollSn = 0;

    // Rx window
    rcvNextSn = 0;
    rcvWnd = LTE_RLC_AM_WINDOW_SIZE;
    tReorderingSn = LTE_RLC_INVALID_SEQ_NUM;
    maxStatusSn = 0;
    rcvSn = 0;

    oldRcvNextSn = 0;

    // reset variable
    pduWithoutPoll = 0;
    byteWithoutPoll = 0;
    retxCount = 0;

    // reset flag
    exprioryTPollRetransmitFlg = FALSE;
    waitExprioryTStatusProhibitFlg = FALSE;

    if (withoutResetProcess == FALSE)
    {
        if (resetTimerMsg != NULL){
            MESSAGE_CancelSelfMsg(entityVar->node,
                                  resetTimerMsg);
            resetTimerMsg = NULL;
            resetFlg = FALSE;
            sendResetFlg = FALSE;
        }
    }
}

// /**
// FUNCTION::       LteRlcAmSetResetData
// LAYER::          LTE Layer2 RLC
// PURPOSE::        set flag of reset AM entity
//
// PARAMETERS::
//  + node           : Node*           : Node Pointer
//  + interfaceIndex : int             : Inteface index
//  + amEntity       : LteRlcAmEntity* : AM entity
// RETURN::         NULL
// **/
void LteRlcAmSetResetData(
    Node* node,
    int interfaceIndex,
    LteRlcAmEntity* amEntity)
{
    amEntity->resetFlg = TRUE;
}

void LteRlcAmGetResetData(
    Node* node,
    int interfaceIndex,
    std::vector < LteRlcAmResetData > & vecAmResetData)
{
    vecAmResetData.clear();
    MapLteConnectionInfo* mapConnectionInfo =
        &LteLayer2GetLayer3DataLte(node, interfaceIndex)->connectionInfoMap;
    MapLteConnectionInfo::iterator conItr;
    for (conItr  = mapConnectionInfo->begin();
        conItr != mapConnectionInfo->end();
        ++conItr)
    {
        LteRnti oppositeRnti = conItr->first;
        MapLteRadioBearer& mapRb = conItr->second.connectedInfo.radioBearers;
        MapLteRadioBearer::iterator rbItr;
        for (rbItr  = mapRb.begin();
            rbItr != mapRb.end();
            ++rbItr)
        {
            LteRadioBearer& rb = rbItr->second;
            LteRlcAmEntity* amEntity =
                (LteRlcAmEntity*)rb.rlcEntity.entityData;
            if (amEntity->sendResetFlg == TRUE)
            {
                ERROR_Assert(amEntity->resetFlg == TRUE,
                    "amEntity->resetFlg should be TRUE.");
                LteRlcAmResetData resetData;
                resetData.bearerId = rb.rlcEntity.bearerId;
                resetData.src = LteRnti(node->nodeId, interfaceIndex);
                resetData.dst = rb.rlcEntity.oppositeRnti;
                vecAmResetData.push_back(resetData);
                amEntity->sendResetFlg = FALSE;
            }
        }
    }
}

// /**
// FUNCTION:: LteRlcAmEntity::getNumAvailableRetxPdu
// LAYER::    LTE Layer2 RLC
// PURPOSE::  available number of PDU from retx buffer which fit into size
//
// PARAMETERS::
//  + node           : Node*   : Node Pointer
//  + interfaceIndex : int     : Inteface index
//  + size           : int     : request size
// RETURN::         UInt32     : Number of PDU
// **/
UInt32 LteRlcAmEntity::getNumAvailableRetxPdu(
    Node* node,
    int interfaceIndex,
    int size)
{
    int ret = 0;
    std::list < LteRlcAmReTxBuffer > ::iterator itr = retxBuffer.begin();
    int tempSize = 0;

    while (itr != retxBuffer.end())
    {
        int nackedSize = itr->getNackedSize(node, interfaceIndex);
        tempSize += nackedSize;
        if (nackedSize > 0)
        {
            ret++;
        }
        if (tempSize > size)
        {
            break;
        }
        ++itr;
    }

    return ret;
}

// for debug
void LteRlcLogPrintWindow(
    Node* node,
    int iface,
    LteRlcEntity* rlcEntity)
{
#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
    char errorMsg[MAX_STRING_LENGTH];
    LteRlcEntityType entityType = rlcEntity->entityType;

    if (entityType == LTE_RLC_ENTITY_AM)
    {
        LteRlcAmEntity* amEntity =
            (LteRlcAmEntity*)rlcEntity->entityData;
        lte::LteLog::DebugFormat(
            node,
            iface,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,"
            LTE_STRING_FORMAT_RNTI","
            "%.9lf,%d,%d,%d,%d,%.9lf,%d,%d,%d,%d,%d",
            "WindowInfo",
            amEntity->entityVar->oppositeRnti.nodeId,
            amEntity->entityVar->oppositeRnti.interfaceIndex,
            node->getNodeTime()/(double)SECOND,
            amEntity->ackSn,
            amEntity->sndWnd,
            amEntity->sndNextSn,
            amEntity->pollSn,
            node->getNodeTime()/(double)SECOND,
            amEntity->rcvNextSn,
            amEntity->rcvWnd,
            amEntity->tReorderingSn,
            amEntity->maxStatusSn,
            amEntity->rcvSn);
    }
    else
    {
        sprintf(errorMsg + strlen(errorMsg),
            "worng RLC entity type for Phase-1.");
        ERROR_Assert(FALSE, errorMsg);
    }
#endif
#endif
}

// for debug
void LteRlcAmEntity::dump(
    Node* node,
    int interfaceIndex,
    const char* procName)
{
#ifdef LTE_LIB_LOG
#if LTE_LIB_RLC_CHECK_EVENT
    lte::LteLog::InfoFormat(
        node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RLC,
        "%s,"
        LTE_STRING_FORMAT_RNTI","
        "Dump::TxBuffer,"
        "TxBuf,%d,%d,"
        "RetxBuf,%d,%d,"
        "StatusBuf,%d,%d",
        procName,
        entityVar->oppositeRnti.nodeId,
        entityVar->oppositeRnti.interfaceIndex,
        txBuffer.size(), txBufSize,
        retxBuffer.size(), retxBufSize,
        statusBuffer.size(), statusBufSize);
    std::list < LteRlcAmReTxBuffer > ::iterator retxItr;
    for (retxItr  = retxBuffer.begin();
        retxItr != retxBuffer.end();
        ++retxItr)
    {
        std::map < int, LteRlcStatusPduSoPair > ::iterator nackItr;
        for (nackItr  = retxItr->nackField.begin();
            nackItr != retxItr->nackField.end();
            ++nackItr)
        {
            lte::LteLog::InfoFormat(
                node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_RLC,
                "%s,"
                LTE_STRING_FORMAT_RNTI","
                "Dump::NACKed,"
                "SN,%d,"
                "retxCount,%d,"
                "SOstart,%d,"
                "SOend,%d",
                procName,
                entityVar->oppositeRnti.nodeId,
                entityVar->oppositeRnti.interfaceIndex,
                retxItr->rlcHeader.fixed->seqNum,
                retxItr->retxCount,
                nackItr->second.soStart,
                nackItr->second.soEnd);
        }
    }
    lte::LteLog::InfoFormat(
        node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RLC,
        "%s,"
        LTE_STRING_FORMAT_RNTI","
        "Dump::RxBuf,"
        "RxBuf,%d,",
        procName,
        entityVar->oppositeRnti.nodeId,
        entityVar->oppositeRnti.interfaceIndex,
        rcvPduBuffer.size());
    std::map< unsigned int, LteRlcAmRcvPduData >::iterator rcvPduItr;
    for (rcvPduItr  = rcvPduBuffer.begin();
        rcvPduItr != rcvPduBuffer.end();
        ++rcvPduItr)
    {
        lte::LteLog::InfoFormat(
            node, interfaceIndex,
            LTE_STRING_LAYER_TYPE_RLC,
            "%s,"
            LTE_STRING_FORMAT_RNTI","
            "Dump::Received,"
            "SN,%d,"
            "isFinished,%d,"
            "FramingInfo,%d,",
            procName,
            entityVar->oppositeRnti.nodeId,
            entityVar->oppositeRnti.interfaceIndex,
            rcvPduItr->first,
            rcvPduItr->second.rlcPduRcvFinished,
            rcvPduItr->second.FramingInfo);
    }

    lte::LteLog::InfoFormat(
        node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RLC,
        "%s,"
        LTE_STRING_FORMAT_RNTI","
        "Dump::SduList,"
        "SduList,%d,"
        "SduSegList,%d,"
        "SduSegTempList,%d",
        procName,
        entityVar->oppositeRnti.nodeId,
        entityVar->oppositeRnti.interfaceIndex,
        rcvRlcSduList.size(),
        rcvRlcSduSegList.size(),
        rcvRlcSduTempList.size());
    lte::LteLog::InfoFormat(
        node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RLC,
        "%s,"
        LTE_STRING_FORMAT_RNTI","
        "Dump::Window,"
        "VT( A),%d,"
        "VT(MS),%d,"
        "VT( S),%d,"
        "POLLSN,%d,"
        "VR( R),%d,"
        "VR(MR),%d,"
        "VR( X),%d,"
        "VR(MS),%d,"
        "VR( H),%d",
        procName,
        entityVar->oppositeRnti.nodeId,
        entityVar->oppositeRnti.interfaceIndex,
        ackSn, sndWnd, sndNextSn, pollSn,
        rcvNextSn, rcvWnd, tReorderingSn, maxStatusSn, rcvSn);
    lte::LteLog::InfoFormat(
        node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RLC,
        "%s,"
        LTE_STRING_FORMAT_RNTI","
        "Dump::Timer,"
        "t-PollRetransmit,%d,"
        "t-Reordering,%d,"
        "t-StatusProhibit,%d,"
        "resetTimer,%d",
        procName,
        entityVar->oppositeRnti.nodeId,
        entityVar->oppositeRnti.interfaceIndex,
        pollRetransmitTimerMsg != NULL ? 1 : 0,
        reoderingTimerMsg != NULL ? 1 : 0,
        statusProhibitTimerMsg != NULL ? 1 : 0,
        resetTimerMsg != NULL ? 1 : 0);
    lte::LteLog::InfoFormat(
        node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RLC,
        "%s,"
        LTE_STRING_FORMAT_RNTI","
        "Dump::Flag,"
        "PollRetransmit,%d,"
        "StatusProhibit,%d,"
        "SendReset,%d,"
        "Reset,%d",
        procName,
        entityVar->oppositeRnti.nodeId,
        entityVar->oppositeRnti.interfaceIndex,
        exprioryTPollRetransmitFlg,
        waitExprioryTStatusProhibitFlg,
        sendResetFlg,
        resetFlg);
#endif
#endif
}


