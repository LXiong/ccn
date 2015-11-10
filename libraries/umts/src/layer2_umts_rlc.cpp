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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include "api.h"
#include "mac.h"
#include "cellular.h"
#include "mac_cellular.h"
#include "layer2_umts.h"
#include "layer2_umts_rlc.h"
#include "layer3_umts.h"
#include "umts_constants.h"
#include "stats_net.h"

#define DEBUG_SPECIFIED_NODE_ID 0 // DEFAULT shoud be 0
#define DEBUG_SINGLE_NODE   (node->nodeId == DEBUG_SPECIFIED_NODE_ID)
#define DEBUG_UE_ID 0
#define DEBUG_RB_ID 0
#define DEBUG_RB_DIRECT  0x2   // 0x1 as uplink, 
                               // 0x2 as downlink, 
                               // (ox1 | ox2 = 0x3) as both direction
#define DEBUG 0
#define DEBUG_FRAGASSEMB 0
#define DEBUG_STATUSPDU  0
#define DEBUG_TIMER      0
#define DEBUG_RLC_PACKET 0

///////////////////////////////////////////////////////////////////////////
// Inline functions facilitating PDU header operations
///////////////////////////////////////////////////////////////////////////
inline
UInt8 UmtsRlcGetHead7BitFromOctet (UInt8 octet)
{
    return (octet & 0xFE) >> 1;
}

inline
void UmtsRlcSetHead7BitInOctet (UInt8& octet, UInt8 val)
{
    octet &= 0x01;
    octet |= (val << 1);
}

inline
UInt16 UmtsRlcGetHead15BitFromDbOctet (UInt16 dbOctets)
{
    return (dbOctets & 0xFFFE) >> 1;
}

inline
void UmtsRlcSetHead15BitInDbOctet (UInt16& dbOctets, const UInt16 val)
{
    dbOctets &= 0x0001;
    dbOctets |= (val << 1);
}

inline
void UmtsRlcSetHead5BitInOctet (UInt8& octet, UInt8 val)
{
    octet &= (~0xF8);
    octet |= val;
}

inline
void UmtsRlcSetTail7BitInOctet (UInt8& octet, UInt8 val)
{
    octet &= ~(0x7F);
    octet |= (val & 0x7F);
}

inline
unsigned int UmtsRlcGetDCBitFromOctet (UInt8 octet)
{
    return octet >> 7;
}

inline
void UmtsRlcSetDCBitInOctet (UInt8& octet)
{
    octet |= 0x80;
}

inline
void UmtsRlcResetDCBitInOctet (UInt8& octet)
{
    octet &= 0x7F;
}

inline
unsigned int UmtsRlcGetPBitFromOctet (UInt8 octet)
{
    return (octet&0x04)>>2;
}

inline
void UmtsRlcSetPBitInOctet (UInt8& octet)
{
    octet |= 0x04;
}

inline
void UmtsRlcResetPBitInOctet (UInt8& octet)
{
    octet &= (~0x04);
}

inline
UInt8 UmtsRlcGetEbitFromOctet (UInt8 octet)
{
    return octet & 0x01;
}

inline
void UmtsRlcSetEbitInOctet (UInt8& octet)
{
    octet |= 0x01;
}

inline
void UmtsRlcResetEbitInOctet (UInt8& octet)
{
    octet &= 0xFE;
}

inline
unsigned int UmtsRlcGetRsnBitFromOctet (UInt8 octet)
{
    return (octet&0x08)>>3;
}

inline
void UmtsRlcSetRsnBitInOctet (UInt8& octet)
{
    octet |= 0x08;
}

inline
void UmtsRlcResetRsnBitInOctet (UInt8& octet)
{
    octet &= (~0x08);
}

inline
UInt16 UmtsRlcGetEbitFromDbOctet (UInt16 dbOctets)
{
    return dbOctets & 0x0001;
}

inline
void UmtsRlcSetEbitInDbOctet (UInt16& dbOctets)
{
    dbOctets |= 0x0001;
}

inline
void UmtsRlcResetEbitInDbOctet (UInt16& dbOctets)
{
    dbOctets &= 0xFFFE;
}

inline
UInt8 UmtsRlcGet4BitDataFromOctet (UInt8 octet, UInt8 head)
{
    ERROR_Assert (head == 0 || head == 1,
        "UmtsRlcGet4BitDataFromOctet: head must be 0 or 1.");
    if (head == 0)
    {
        return octet >> 4;
    }
    else
    {
        return octet&0x0F;
    }
}

inline
void UmtsRlcSet4BitDataInOctet(UInt8& octet, UInt8 head, UInt8 val)
{
    ERROR_Assert(val <= 0x0F,
        "UmtsRlcSet4BitDataInOctet: val is over limit.");
    ERROR_Assert (head == 0 || head == 1,
        "UmtsRlcSet4BitDataInOctet: head must be 0 or 1.");
    if (head == 0)
    {
        octet &= 0x0F;
        octet |= (val << 4);
    }
    else
    {
        octet &= 0xF0;
        octet |= val;
    }
}

///////////////////////////////////////////////////////////////////////////
// Utility functions for sequence number operations
///////////////////////////////////////////////////////////////////////////
inline
void UmtsRlcUmIncSN (unsigned int& sn)
{
    ++sn;
    sn %= UMTS_RLC_UM_SNUPPERBOUND;
}

inline
void UmtsRlcAmIncSN (unsigned int& sn)
{
    ++sn;
    sn %= UMTS_RLC_AM_SNUPPERBOUND;
}

inline
void UmtsRlcAmSetPollBit(Message* msg)
{
    UmtsRlcSetPBitInOctet((UInt8&)(*(MESSAGE_ReturnPacket(msg) + 1)));
}

inline
unsigned int UmtsRlcAmGetPollBit(Message* msg)
{
    return UmtsRlcGetPBitFromOctet(
                (UInt8)(*(MESSAGE_ReturnPacket(msg) + 1)));
}

inline
unsigned int UmtsRlcAmGetDCBit(Message* msg)
{
    return UmtsRlcGetDCBitFromOctet(
                (UInt8)(*MESSAGE_ReturnPacket(msg)));
}

inline
unsigned int UmtsRlcUmGetSeqNum(const Message* msg)
{
    UInt8* octet = (UInt8*)MESSAGE_ReturnPacket(msg);
    return static_cast<unsigned int>
        (UmtsRlcGetHead7BitFromOctet(*octet));
}

inline
void UmtsRlcUmSetSeqNum(
    Message* msg,
    unsigned int seqNum)
{
    UInt8* octet = (UInt8*)MESSAGE_ReturnPacket(msg);
    UmtsRlcSetHead7BitInOctet(*octet, (UInt8)seqNum);
}

// /**
// FUNCTION::       UmtsRlcAmGetSeqNum
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Get a sequence number (AM)  
// PARAMETERS::
//      +msg: Message*: the msg where the The sequence number to be get
// RETURN::   unsigned int:     the sequence number 
// **/
static
unsigned int UmtsRlcAmGetSeqNum(const Message* msg)
{
    UInt8 highOct = 
        static_cast<UInt8>((*MESSAGE_ReturnPacket(msg))) & 0x7F;
    UInt8 lowOct = 
        static_cast<UInt8>(*(MESSAGE_ReturnPacket(msg) + 1)) & 0xF8;
    unsigned int seqNum = (unsigned int)highOct;
    seqNum <<= 8;
    seqNum |= lowOct;
    seqNum >>= 3;
    if (DEBUG_FRAGASSEMB && 0)
    {
        UInt8 hdrOct1 = static_cast<UInt8>(*MESSAGE_ReturnPacket(msg));
        UInt8 hdrOct2 = static_cast<UInt8>(*(MESSAGE_ReturnPacket(msg)+1));
        std::cout << "Get sequence number: " << seqNum 
             << std::hex << std::showbase
             << "   high octet: " << static_cast<unsigned int>(highOct)
             << "   low octet: " << static_cast<unsigned int>(lowOct)
             << "   *msg: " << static_cast<unsigned int>(hdrOct1)
             << "   *(msg+1): " << static_cast<unsigned int>(hdrOct2)
             << std::dec
             << std::endl;
    }
    return seqNum;
}

// /**
// FUNCTION::       UmtsRlcAmSetSeqNum
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Set a sequence number (AM)
// PARAMETERS::
//      +msg: Message*: the msg where the The sequence number to be set
//      +seqNum: unsigned int: The sequence number to be set
// RETURN::   void:      NULL
// **/
static
void UmtsRlcAmSetSeqNum(
    Message* msg,
    unsigned int seqNum)
{
    UInt8 lowOct = static_cast<UInt8>(seqNum) << 3;
    UInt8 highOct = static_cast<UInt8>(seqNum >> 5) & 0x7F;

    UmtsRlcSetTail7BitInOctet(
        (UInt8&)(*MESSAGE_ReturnPacket(msg)),
        highOct);
    UmtsRlcSetHead5BitInOctet(
        (UInt8&)(*(MESSAGE_ReturnPacket(msg) + 1)),
        lowOct);
    if (DEBUG_FRAGASSEMB && 0)
    {
        UInt8 hdrOct1 = static_cast<UInt8>(*MESSAGE_ReturnPacket(msg));
        UInt8 hdrOct2 = static_cast<UInt8>(*(MESSAGE_ReturnPacket(msg)+1));
        std::cout << "Set equence number: " << seqNum<< std::hex <<std::showbase
             << "   high octet: " << static_cast<unsigned int>(highOct)
             << "   low octet: " << static_cast<unsigned int>(lowOct)
             << "   *msg: " << static_cast<unsigned int>(hdrOct1)
             << "   *(msg+1): " << static_cast<unsigned int>(hdrOct2)
             << std::dec
             << std::endl;
    }
}

// /**
// FUNCTION::       UmtsRlcSeqNumLess
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Return whether a sequence number is less than the other
// PARAMETERS::
//      +sn1: unsigned int: The sequence number to be compared
//      +sn2: unsigned int: The sequence number to be compared with
//      +wnd: unsigned int: The window size the sequence number are limited
// RETURN::     BOOL: less or not
// **/
static
bool UmtsRlcSeqNumLess(
    unsigned int sn1,
    unsigned int sn2,
    unsigned int wnd)
{
    if (sn1 < sn2)
    {
        if (sn1 + wnd >= sn2)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if (sn1 > sn2)
    {
        if (sn2 + wnd >= sn1)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}

// /**
// FUNCTION::       UmtsRlcAmSeqNumDist
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Return the distance between sn1 and sn2,
//                  assuming sn1 is a "smaller" than sn2
// PARAMETERS::
//      +sn1: unsigned int: The sequence number in the front
//      +sn2: unsigned int: The sequence number in the back
// RETURN:: unsigned int : Sequence Number distance
// **/
static
unsigned int UmtsRlcAmSeqNumDist(
    unsigned int sn1,
    unsigned int sn2)
{
    if (sn1 <= sn2)
    {
        return sn2 - sn1;
    }
    else
    {
        return sn2 + (UMTS_RLC_AM_SNUPPERBOUND - sn1);
    }
}

// /**
// FUNCTION::       UmtsRlcSeqNumInWnd
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Return whether a sequence number is within the window
// PARAMETERS::
//      +sn: unsigned int: The sequence number to be tested
//      +snStart: unsigned int: The sequence number of window starting point
//      +wnd: unsigned int: The window size
//      +snBound: unsigned int: The upperbound of sequence number
// RETURN::    void:     BOOL
// **/
static
BOOL UmtsRlcSeqNumInWnd(
    unsigned int sn,
    unsigned int snStart,
    unsigned int wnd,
    unsigned int snBound)
{
    ERROR_Assert(wnd > 0 && wnd < snBound/2,
        "UmtsRlcSeqNumInWnd: window size must be larger than 0 "
        "and smaller than half of the maximum sequence number. ");
    unsigned int snEnd = (snStart + wnd)%snBound;
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
// FUNCTION::           UmtsRlcCopyMessageTraceInfo
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Copy simulation related message information
//                      (excluding those packet related info)
//                      during segmentation or reassembly.
// PARAMETERS::
//      +node: Node* :         Node pointer
//      +destMessage: Message* : The message to be copied to
//      +srcMessage: Message* : The message to be copied from
// RETURN::    void:         NULL
// **/
static
void UmtsRlcCopyMessageTraceInfo(
            Node* node,
            Message* destMessage,
            Message* srcMessage)
{
    destMessage->sequenceNumber = srcMessage->sequenceNumber;
    destMessage->originatingProtocol = srcMessage->originatingProtocol;
    destMessage->numberOfHeaders = srcMessage->numberOfHeaders;

    for (int i = 0; i < srcMessage->numberOfHeaders; i ++)
    {
        destMessage->headerProtocols[i] = srcMessage->headerProtocols[i];
        destMessage->headerSizes[i] = srcMessage->headerSizes[i];
    }
}

// /**
// FUNCTION   :: UmtsRlcInitTimer
// LAYER      :: MAC
// PURPOSE    :: Set a timer with given type and delay
// PARAMETERS ::
//      +node: Node*:  Pointer to node
//      +interfaceIndex: int: the interface index
//      +timerType: UmtsLayer2TimerType: type of the timer
//      +infoLen: unsigned int: Length of the info attached 
//                              behind the info header
//      +info: const char*: info data pointer
//      +rbId: unsigned int: radio bearer ID of the entity that 
//                           will receive the timer message
//      +direction: UmtsRlcEntityDirect: Entity traffic direction
//      +ueId: NodeAddress: UE ID, used to differentiate entities at RNC
// RETURN     :: Message * : Pointer to the allocatd timer message
// **/
static
Message* UmtsRlcInitTimer(
             Node* node,
             int interfaceIndex,
             UmtsLayer2TimerType timerType,
             unsigned int infoLen,
             const char* info,
             unsigned int rbId,
             UmtsRlcEntityDirect direction,
             NodeAddress ueId = 0)
{
    unsigned int length = sizeof(UmtsRlcTimerInfoHdr) + infoLen;

    char aregInfo[MAX_STRING_LENGTH];
    UmtsRlcTimerInfoHdr* infoHdr = (UmtsRlcTimerInfoHdr*)aregInfo;
    infoHdr->ueId = ueId;
    infoHdr->rbId = (char)rbId;
    infoHdr->direction = direction;

    memcpy(aregInfo+sizeof(UmtsRlcTimerInfoHdr),
           info,
           infoLen);

    Message* timerMsg = UmtsLayer2InitTimer(node,
                                            interfaceIndex,
                                            UMTS_LAYER2_SUBLAYER_RLC,
                                            timerType,
                                            length,
                                            aregInfo);
    return timerMsg;
}

// /**
// FUNCTION::       UmtsRlcGetLayer2Data
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Get UMTS layer2 pointer from node and interface Id
// PARAMETERS::
//      +node: Node*:      node pointer
//      +ifac: int:        interface id
// RETURN::         CellularUmtsLayer2Data : RLC data pointer
// **/
static
CellularUmtsLayer2Data* UmtsRlcGetLayer2Data(
    Node* node,
    int iface)
{
    MacCellularData* macCellularData = (MacCellularData*)
                      node->macData[iface]->macVar;
    CellularUmtsLayer2Data* layer2Data = 
                      macCellularData->cellularUmtsL2Data;
    return layer2Data;
}

// /**
// FUNCTION::       UmtsRlcGetSubLayerData
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Get RLC sublayer pointer from node and interface Id
// PARAMETERS::
//      +node:  Node*:    node pointer
//      +ifac:  int  :    interface id
// RETURN::         UmtsRlcData : RLC sublayer data
// **/
static
UmtsRlcData* UmtsRlcGetSubLayerData(
    Node* node,
    int iface)
{
    CellularUmtsLayer2Data* umtsLayer2 = UmtsRlcGetLayer2Data(node, iface);
    return umtsLayer2->umtsRlcData;
}

// /**
// FUNCTION::           UmtsRlcAmInitPollTimer
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Init Poll Timer
// PARAMETERS::
//      +node: Node* :  Node pointer
//      +iface: int : Inteface index
//      +rlcEntity:  UmtsRlcAmEntity*:   The RLC entity
//      +sndNextSn: unsigned int : Next sequence number
// RETURN::    void:         NULL
// **/
static
void UmtsRlcAmInitPollTimer(
    Node* node,
    int iface,
    UmtsRlcAmEntity* amEntity,
    unsigned int sndNextSn)
{
    UmtsRlcEntity* rlcEntity = (UmtsRlcEntity*)(amEntity->entityVar);
    if (amEntity->pollTimerMsg)
    {
        MESSAGE_CancelSelfMsg(node, amEntity->pollTimerMsg);
        amEntity->pollTimerMsg = NULL;
    }
    amEntity->pollTimerInfo.sndNextSn = sndNextSn;
    amEntity->pollTimerMsg =
                UmtsRlcInitTimer(
                        node,
                        iface,
                        UMTS_RLC_TIMER_POLL,
                        sizeof(amEntity->pollTimerInfo),
                        (char*)&amEntity->pollTimerInfo,
                        rlcEntity->rbId,
                        UMTS_RLC_ENTITY_TX,
                        rlcEntity->ueId);
    amEntity->pollTriggered = FALSE;
    MESSAGE_Send(node,
                 amEntity->pollTimerMsg,
                 UMTS_RLC_DEF_POLLTIMER_DUR);
}

// /**
// FUNCTION::           UmtsRlcEntity::reset
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            reset the RLC entity
// PARAMETERS::
//      +node: Node*:   Node pointer
// RETURN::     void:        NULL
// **/
void UmtsRlcTmTransEntity::reset(Node* node)
{
    bufSize = 0;
    std::for_each(transSduBuf->begin(),
                  transSduBuf->end(),
                  UmtsRlcFreeMessage(node));
    transSduBuf->clear();
}

// /**
// FUNCTION::           UmtsRlcTmTransEntity::store
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            store the RLC TM transmission entity
// PARAMETERS::
//      +node:  Node* : Node Pointer
//      +buffer: std::string&:       string buffer
// RETURN::    void:         NULL
// **/
void UmtsRlcTmTransEntity::store(Node* node, std::string& buffer)
{
    buffer.reserve(5000);

    buffer.append((char*)&bufSize, sizeof(bufSize));
    int containerSize = (int)transSduBuf->size();
    buffer.append((char*)&containerSize, sizeof(containerSize));
    std::deque<Message*>::iterator msgIter = transSduBuf->begin();
    while (msgIter != transSduBuf->end())
    {
        UmtsSerializeMessageList(node, *msgIter, buffer);
        ++msgIter;
    }
    transSduBuf->clear();
}

// /**
// FUNCTION::           UmtsRlcTmTransEntity::store
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            restore the RLC TM transmission entity
// PARAMETERS::
//      +node: Node*: Node Pointer
//      +buffer: const char*:        string buffer
//      +bufSize: size_t : buffer size
// RETURN::      void:       NULL
// **/
void UmtsRlcTmTransEntity::restore(Node* node,
                                   const char* buffer,
                                   size_t bufSize)
{
    int index = 0;
    int containerSize;

    memcpy(&bufSize, &buffer[index], sizeof(bufSize));
    index += sizeof(bufSize);

    std::for_each(transSduBuf->begin(),
                  transSduBuf->end(),
                  UmtsRlcFreeMessage(node));
    transSduBuf->clear();
    memcpy(&containerSize, &buffer[index], sizeof(containerSize));
    index += sizeof(containerSize);
    for (int i = 0; i < containerSize; ++i)
    {
        Message* transSdu = UmtsUnSerializeMessage(
                                   node,
                                   buffer,
                                   index);
        transSduBuf->push_back(transSdu);
    }
}

// /**
// FUNCTION::           UmtsRlcEntity::reset
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            reset the RLC entity
// PARAMETERS::
//      +node: Node*:         Node pointer
// RETURN::     void:        NULL
// **/
void UmtsRlcTmEntity::reset(Node* node)
{
    txEntity->reset(node);
    rxEntity->reset(node);
}

// /**
// FUNCTION::           UmtsRlcEntity::store
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            store the RLC entity
// PARAMETERS::
//      +node: Node*: Node Pointer
//      +buffer: std::string&: string buffer
// RETURN::     void:        NULL
// **/
void UmtsRlcTmEntity::store(Node* node, std::string& buffer)
{
    std::string txBuf;
    txEntity->store(node, txBuf);
    size_t txBufSize = txBuf.size();
    std::string rxBuf;
    rxEntity->store(node, rxBuf);
    size_t rxBufSize = rxBuf.size();
    buffer.reserve(txBuf.size() +
                        rxBuf.size() + 2*sizeof(size_t));
    buffer.append((char*)&txBufSize, sizeof(txBufSize));
    buffer.append(txBuf);
    buffer.append((char*)&rxBufSize, sizeof(rxBufSize));
    buffer.append(rxBuf);
}

// /**
// FUNCTION::           UmtsRlcEntity::restore
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            restore the RLC entity
// PARAMETERS::
//      +node: Node*: Node Pointer
//      +buffer: const char*: The string buffer
//      +bufSize: size_t:      The string buffer size
// RETURN::      void:       NULL
// **/
void UmtsRlcTmEntity::restore(Node* node,
                              const char* buffer,
                              size_t bufSize)
{
    size_t index = 0;
    size_t txBufSize;
    memcpy(&txBufSize, &buffer[index], sizeof(txBufSize));
    index += sizeof(txBufSize);
    txEntity->restore(node, &buffer[index], txBufSize);
    index += txBufSize;
    size_t rxBufSize;
    memcpy(&rxBufSize, &buffer[index], sizeof(rxBufSize));
    index += sizeof(rxBufSize);
    rxEntity->restore(node, &buffer[index], rxBufSize);
    index += rxBufSize;
    ERROR_Assert(index == bufSize, "incorrect format");
}

// /**
// FUNCTION::           UmtsRlcEntity::reset
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            reset the RLC entity
// PARAMETERS::
//      +node:  Node*:        Node pointer
// RETURN::      void:       NULL
// **/
void UmtsRlcUmTransEntity::reset(Node* node)
{
    fragmentEntity->RequestLastPdu(node);
    bufSize = 0;
    sndNext = 0;
    std::for_each(transSduBuf->begin(),
                  transSduBuf->end(),
                  UmtsRlcFreeMessage(node));
    transSduBuf->clear();
    std::for_each(pduBuf->begin(), pduBuf->end(),
                  UmtsRlcFreeMessage(node));
    pduBuf->clear();
}

// /**
// FUNCTION::           UmtsRlcEntity::store
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            store the RLC entity
// PARAMETERS::
//      +node: Node*: Node pointer
//      +buffer: std::string&: string buffer
// RETURN::   void: NULL
// **/
void UmtsRlcUmTransEntity::store(Node* node, std::string& buffer)
{
    int containerSize;
    buffer.reserve(5000);
    buffer.append((char*)&bufSize, sizeof(bufSize));
    buffer.append((char*)&maxUlPduSize, sizeof(maxUlPduSize));
    buffer.append((char*)&liOctetsLen, sizeof(liOctetsLen));
    buffer.append((char*)&alterEbit, sizeof(alterEbit));
    buffer.append((char*)&sndNext, sizeof(sndNext));

    fragmentEntity->Store(node, buffer);

    containerSize = (int)transSduBuf->size();
    buffer.append((char*)&containerSize, sizeof(containerSize));
    std::deque<Message*>::iterator msgIter = transSduBuf->begin();
    while (msgIter != transSduBuf->end())
    {
        UmtsSerializeMessageList(node, *msgIter, buffer);
        ++msgIter;
    }
    transSduBuf->clear();

    containerSize = (int)pduBuf->size();
    buffer.append((char*)&containerSize, sizeof(containerSize));
    msgIter = pduBuf->begin();
    while (msgIter != pduBuf->end())
    {
        std::string childBuf;
        childBuf.reserve(2000);
        int numSegs = UmtsSerializeMessageList(node,
                                               *msgIter,
                                               childBuf);
        buffer.append((char*)&numSegs, sizeof(numSegs));
        buffer.append(childBuf);
        ++msgIter;
    }
    pduBuf->clear();
}

// /**
// FUNCTION::           UmtsRlcEntity::restore
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            restore the RLC entity
// PARAMETERS::
//      +node: Node*: Node Pointer
//      +buffer: const char*: The string buffer
//      +bufSize: size_t: The string buffer size
// RETURN::     void:        NULL
// **/
void UmtsRlcUmTransEntity::restore(Node* node,
                                   const char* buffer,
                                   size_t bufferSize)
{
    int index = 0;
    int containerSize;

    memcpy(&bufSize, &buffer[index], sizeof(bufSize));
    index += sizeof(bufSize);
    memcpy(&maxUlPduSize, &buffer[index], sizeof(maxUlPduSize));
    index += sizeof(maxUlPduSize);
    memcpy(&liOctetsLen, &buffer[index], sizeof(liOctetsLen));
    index += sizeof(liOctetsLen);
    memcpy(&alterEbit, &buffer[index], sizeof(alterEbit));
    index += sizeof(alterEbit);
    memcpy(&sndNext, &buffer[index], sizeof(sndNext));
    index += sizeof(sndNext);

    fragmentEntity->Restore(node, buffer, index);

    std::for_each(transSduBuf->begin(),
                  transSduBuf->end(),
                  UmtsRlcFreeMessage(node));
    transSduBuf->clear();
    memcpy(&containerSize, &buffer[index], sizeof(containerSize));
    index += sizeof(containerSize);
    for (int i = 0; i < containerSize; ++i)
    {
        Message* transSdu = UmtsUnSerializeMessage(
                                   node,
                                   buffer,
                                   index);
        transSduBuf->push_back(transSdu);
    }

    std::for_each(pduBuf->begin(),
                  pduBuf->end(),
                  UmtsRlcFreeMessage(node));
    pduBuf->clear();
    memcpy(&containerSize, &buffer[index], sizeof(containerSize));
    index += sizeof(containerSize);
    for (int i = 0; i < containerSize; ++i)
    {
        int numSegs;
        memcpy(&numSegs,  &buffer[index], sizeof(numSegs));
        index += sizeof(numSegs);
        Message* transPdu = UmtsUnSerializeMessageList(
                                   node,
                                   buffer,
                                   index,
                                   numSegs);
        pduBuf->push_back(transPdu);
    }

    ERROR_Assert((unsigned int)index == bufferSize, "Incorrect format");
}

// /**
// FUNCTION::           UmtsRlcEntity::reset
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            reset the RLC entity
// PARAMETERS::
//      +node: Node*: Node pointer
// RETURN::   void:          NULL
// **/
void UmtsRlcUmRcvEntity::reset(Node* node)
{
    rcvNext = 0;
    std::for_each(sduSegmentList->begin(),
                  sduSegmentList->end(),
                  UmtsRlcFreeMessageData<UmtsRlcSduSegment>(node));
    sduSegmentList->clear();
}

// /**
// FUNCTION::           UmtsRlcEntity::store
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            store the RLC entity
// PARAMETERS::
//      +node: Node*: Node Pointer
//      +buffer: std::string&: string buffer
// RETURN::   void:          NULL
// **/
void UmtsRlcUmRcvEntity::store(Node* node, std::string& buffer)
{
    int containerSize;
    buffer.reserve(1000);
    buffer.append((char*)&maxUlPduSize, sizeof(maxUlPduSize));
    buffer.append((char*)&liOctetsLen, sizeof(liOctetsLen));
    buffer.append((char*)&alterEbit, sizeof(alterEbit));
    buffer.append((char*)&rcvNext, sizeof(rcvNext));

    containerSize = (int)sduSegmentList->size();
    buffer.append((char*)&containerSize, sizeof(containerSize));
    std::list<UmtsRlcSduSegment*>::iterator segIter;
    segIter = sduSegmentList->begin();
    while (segIter != sduSegmentList->end())
    {
        (*segIter)->store(node, buffer);
        MEM_free(*segIter);
        ++segIter;
    }
    sduSegmentList->clear();
}

// /**
// FUNCTION::           UmtsRlcEntity::restore
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            restore the RLC entity
// PARAMETERS::
//      +node: Node* : Node Pointer
//      +buffer: const char*: The string buffer
//      +bufSize: size_t: The string buffer size
// RETURN::    void:         NULL
// **/
void UmtsRlcUmRcvEntity::restore(Node* node,
                                 const char* buffer,
                                 size_t bufSize)
{
    int index = 0;
    int containerSize;

    memcpy(&maxUlPduSize, &buffer[index], sizeof(maxUlPduSize));
    index += sizeof(maxUlPduSize);
    memcpy(&liOctetsLen, &buffer[index], sizeof(liOctetsLen));
    index += sizeof(liOctetsLen);
    memcpy(&alterEbit, &buffer[index], sizeof(alterEbit));
    index += sizeof(alterEbit);
    memcpy(&rcvNext, &buffer[index], sizeof(rcvNext));
    index += sizeof(rcvNext);

    std::for_each(sduSegmentList->begin(),
                  sduSegmentList->end(),
                  UmtsRlcFreeMessageData
                    <UmtsRlcSduSegment>(node));
    sduSegmentList->clear();
    memcpy(&containerSize, &buffer[index], sizeof(containerSize));
    index += sizeof(containerSize);
    for (int i = 0; i < containerSize; ++i)
    {
        UmtsRlcSduSegment* segment = (UmtsRlcSduSegment*)
                         MEM_malloc(sizeof(UmtsRlcSduSegment));
        segment->restore(node, buffer, index);
        sduSegmentList->push_back(segment);
    }

    ERROR_Assert((unsigned int)index == bufSize, "Incorrect format");
}

 // /**
// FUNCTION::           UmtsRlcEntity::reset
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            reset the RLC entity
// PARAMETERS::
//      +node:  Node*:  Node pointer
// RETURN::      void:       NULL
// **/
void UmtsRlcUmEntity::reset(Node* node)
{
    txEntity->reset(node);
    rxEntity->reset(node);
}

// /**
// FUNCTION::           UmtsRlcEntity::store
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            store the RLC entity
// PARAMETERS::
//      +node: Node*: Node Pointer
//      +buffer: std::string&: string buffer
// RETURN::  void:    NULL
// **/
void UmtsRlcUmEntity::store(Node* node, std::string& buffer)
{
    std::string txBuf;
    txEntity->store(node, txBuf);
    size_t txBufSize = txBuf.size();
    std::string rxBuf;
    rxEntity->store(node, rxBuf);
    size_t rxBufSize = rxBuf.size();
    buffer.reserve(txBuf.size() +
                        rxBuf.size() + 2*sizeof(size_t));
    buffer.append((char*)&txBufSize, sizeof(txBufSize));
    buffer.append(txBuf);
    buffer.append((char*)&rxBufSize, sizeof(rxBufSize));
    buffer.append(rxBuf);
}

// /**
// FUNCTION::           UmtsUmRlcEntity::restore
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            restore the RLC entity
// PARAMETERS::
//      +node: Node*: Node Pointer
//      +buffer: const char*: The string buffer
//      +bufSize: size+t: The string buffer size
// RETURN::    void:         NULL
// **/
void UmtsRlcUmEntity::restore(Node* node,
                              const char* buffer,
                              size_t bufSize)
{
    int index = 0;
    size_t txBufSize;
    memcpy(&txBufSize, &buffer[index], sizeof(txBufSize));
    index += sizeof(txBufSize);
    txEntity->restore(node, &buffer[index], txBufSize);
    index += (int)txBufSize;
    size_t rxBufSize;
    memcpy(&rxBufSize, &buffer[index], sizeof(rxBufSize));
    index += sizeof(rxBufSize);
    rxEntity->restore(node, &buffer[index], rxBufSize);
    index += (int)rxBufSize;
    ERROR_Assert((unsigned int)index == bufSize, "incorrect format");
}

// /**
// FUNCTION::           UmtsRlcEntity::reset
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            reset the RLC entity
// PARAMETERS::
//      +node: Node*:   Node pointer
// RETURN::    void:         NULL
// **/
void UmtsRlcAmEntity::reset(Node* node)
{
    fragmentEntity->RequestLastPdu(node);
    std::for_each(transPduBuf->begin(),
                  transPduBuf->end(),
                  UmtsRlcFreeMessage(node));
    transPduBuf->clear();
    std::for_each(rtxPduBuf->begin(),
                  rtxPduBuf->end(),
                  UmtsRlcFreeMessageData
                    <UmtsRlcAmRtxPduData>(node));
    rtxPduBuf->clear();
    bufSize = 0;
    sndNextSn = 0;

    rcvNextSn = 0;
    rcvHighestExpSn = 0;
    std::for_each(sduSegmentList->begin(),
                  sduSegmentList->end(),
                  UmtsRlcFreeMessageData
                    <UmtsRlcSduSegment>(node));
    sduSegmentList->clear();
    pduRcptFlag->clear();
    pduRcptFlag->insert(pduRcptFlag->end(),
                        rcvWnd,
                        FALSE);

    statusPduFlag = 0;
}

// /**
// FUNCTION::           UmtsRlcEntity::store
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            store the RLC entity
// PARAMETERS::
//      +node: Node*: Node Pointer
//      +buffer: std:string& : string buffer
// RETURN::     void:        NULL
// **/
void UmtsRlcAmEntity::store(Node* node, std::string& buffer)
{
    std::string childBuf;
    int containerSize;
    buffer.reserve(5000);
    childBuf.reserve(500);

    buffer.append((char*)&state, sizeof(state));
    buffer.append((char*)&bufSize, sizeof(bufSize));

    buffer.append((char*)&sndNextSn, sizeof(sndNextSn));
    buffer.append((char*)&ackSn, sizeof(ackSn));
    buffer.append((char*)&sndWnd, sizeof(sndWnd));
    buffer.append((char*)&sndPduSize, sizeof(sndPduSize));
    buffer.append((char*)&sndLiOctetsLen, sizeof(sndLiOctetsLen));

    buffer.append((char*)&rcvNextSn, sizeof(rcvNextSn));
    buffer.append((char*)&rcvHighestExpSn, sizeof(rcvHighestExpSn));
    buffer.append((char*)&rcvWnd, sizeof(rcvWnd));
    buffer.append((char*)&rcvPduSize, sizeof(rcvPduSize));
    buffer.append((char*)&rcvLiOctetsLen, sizeof(rcvLiOctetsLen));

    buffer.append((char*)&pollTriggered, sizeof(pollTriggered));
    BOOL pollTimerOn = FALSE;
    if (pollTimerMsg)
    {
        pollTimerOn = TRUE;
    }
    buffer.append((char*)&pollTimerOn, sizeof(pollTimerOn));
    buffer.append((char*)&pollTimerInfo, sizeof(pollTimerInfo));

    fragmentEntity->Store(node, buffer);

    containerSize = (int)transPduBuf->size();
    buffer.append((char*)&containerSize, sizeof(containerSize));
    std::deque<Message*>::iterator msgIter = transPduBuf->begin();
    while (msgIter != transPduBuf->end())
    {
        std::string tmpBuffer;
        int numSegs = UmtsSerializeMessageList(node,
                                               *msgIter,
                                               tmpBuffer);
        buffer.append((char*)&numSegs, sizeof(numSegs));
        buffer.append(tmpBuffer);
        ++msgIter;
    }
    transPduBuf->clear();

    containerSize = (int)rtxPduBuf->size();
    buffer.append((char*)&containerSize, sizeof(containerSize));
    std::list<UmtsRlcAmRtxPduData*>::iterator rtxIter
                                            = rtxPduBuf->begin();
    while (rtxIter != rtxPduBuf->end())
    {
        (*rtxIter)->store(node, buffer);
        MEM_free(*rtxIter);
        ++rtxIter;
    }
    rtxPduBuf->clear();


    containerSize = (int)pduRcptFlag->size();
    buffer.append((char*)&containerSize, sizeof(containerSize));
    std::deque<BOOL>::iterator rcptFlagIter = pduRcptFlag->begin();
    while (rcptFlagIter != pduRcptFlag->end())
    {
        buffer.append((char*)&(*rcptFlagIter), sizeof(*rcptFlagIter));
        (*rcptFlagIter) = FALSE;
        rcptFlagIter++;
    }

    containerSize = (int)sduSegmentList->size();
    buffer.append((char*)&containerSize, sizeof(containerSize));
    std::list<UmtsRlcSduSegment*>::iterator segIter;
    segIter = sduSegmentList->begin();
    while (segIter != sduSegmentList->end())
    {
        (*segIter)->store(node, buffer);
        MEM_free(*segIter);
        ++segIter;
    }
    sduSegmentList->clear();
}

// /**
// FUNCTION::           UmtsRlcEntity::restore
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            restore the RLC entity
// PARAMETERS::
//      +node: Node*: Node Pointer
//      +buffer: const char*: The string buffer
//      +bufSize: size_t: The string buffer size
// RETURN::    void :         NULL
// **/
void UmtsRlcAmEntity::restore(Node* node, 
                              const char* buffer, 
                              size_t bufSize)
{
    int index = 0;
    int containerSize;

    memcpy(&state, &buffer[index], sizeof(state));
    index += sizeof(state);
    memcpy(&(this->bufSize), &buffer[index], sizeof(this->bufSize));
    index += sizeof(this->bufSize);

    memcpy(&sndNextSn, &buffer[index], sizeof(sndNextSn));
    index += sizeof(sndNextSn);
    memcpy(&ackSn, &buffer[index], sizeof(ackSn));
    index += sizeof(ackSn);
    memcpy(&sndWnd, &buffer[index], sizeof(sndWnd));
    index += sizeof(sndWnd);
    memcpy(&sndPduSize, &buffer[index], sizeof(sndPduSize));
    index += sizeof(sndPduSize);
    memcpy(&sndLiOctetsLen, &buffer[index], sizeof(sndLiOctetsLen));
    index += sizeof(sndLiOctetsLen);

    memcpy(&rcvNextSn, &buffer[index], sizeof(rcvNextSn));
    index += sizeof(rcvNextSn);
    memcpy(&rcvHighestExpSn, &buffer[index], sizeof(rcvHighestExpSn));
    index += sizeof(rcvHighestExpSn);
    memcpy(&rcvWnd, &buffer[index], sizeof(rcvWnd));
    index += sizeof(rcvWnd);
    memcpy(&rcvPduSize, &buffer[index], sizeof(rcvPduSize));
    index += sizeof(rcvPduSize);
    memcpy(&rcvLiOctetsLen, &buffer[index], sizeof(rcvLiOctetsLen));
    index += sizeof(rcvLiOctetsLen);

    memcpy(&pollTriggered, &buffer[index], sizeof(pollTriggered));
    index += sizeof(pollTriggered);
    BOOL pollTimerOn;
    memcpy(&pollTimerOn, &buffer[index], sizeof(pollTimerOn));
    index += sizeof(pollTimerOn);
    memcpy(&pollTimerInfo, &buffer[index], sizeof(pollTimerInfo));
    index += sizeof(pollTimerInfo);

    fragmentEntity->Restore(node, buffer, index);

    std::for_each(transPduBuf->begin(),
                  transPduBuf->end(),
                  UmtsRlcFreeMessage(node));
    transPduBuf->clear();
    memcpy(&containerSize, &buffer[index], sizeof(containerSize));
    index += sizeof(containerSize);
    for (int i = 0; i < containerSize; ++i)
    {
        int numSegs;
        memcpy(&numSegs,  &buffer[index], sizeof(numSegs));
        index += sizeof(numSegs);
        Message* transPdu = UmtsUnSerializeMessageList(
                                   node,
                                   buffer,
                                   index,
                                   numSegs);
        transPduBuf->push_back(transPdu);
    }

    std::for_each(rtxPduBuf->begin(),
                  rtxPduBuf->end(),
                  UmtsRlcFreeMessageData
                    <UmtsRlcAmRtxPduData>(node));
    rtxPduBuf->clear();
    memcpy(&containerSize, &buffer[index], sizeof(containerSize));
    index += sizeof(containerSize);
    for (int i = 0; i < containerSize; ++i)
    {
        UmtsRlcAmRtxPduData* rtxPduData = (UmtsRlcAmRtxPduData*)
                            MEM_malloc(sizeof(UmtsRlcAmRtxPduData));
        rtxPduData->restore(node, buffer, index);
        rtxPduBuf->push_back(rtxPduData);
    }

    pduRcptFlag->clear();
    memcpy(&containerSize, &buffer[index], sizeof(containerSize));
    index += sizeof(containerSize);
    for (int i = 0; i < containerSize; ++i)
    {
        BOOL flag;
        memcpy(&flag, &buffer[index], sizeof(flag));
        index += sizeof(flag);
        pduRcptFlag->push_back(flag);
    }

    std::for_each(sduSegmentList->begin(),
                  sduSegmentList->end(),
                  UmtsRlcFreeMessageData
                    <UmtsRlcSduSegment>(node));
    sduSegmentList->clear();
    memcpy(&containerSize, &buffer[index], sizeof(containerSize));
    index += sizeof(containerSize);
    for (int i = 0; i < containerSize; ++i)
    {
        UmtsRlcSduSegment* segment = (UmtsRlcSduSegment*)
                         MEM_malloc(sizeof(UmtsRlcSduSegment));
        segment->restore(node, buffer, index);
        sduSegmentList->push_back(segment);
    }

    ERROR_Assert((unsigned int)index == bufSize, "Incorrect format");

    if (pollTimerOn)
    {
        pollTimerMsg = NULL;
        UmtsRlcAmInitPollTimer(node,
                               0,
                               this,
                               sndNextSn);
    }
}

// /**
// FUNCTION::           UmtsRlcEntity::reset
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            reset the RLC entity
// PARAMETERS::
//      +node: Node*:   Node pointer
// RETURN::     void:        NULL
// **/
void UmtsRlcEntity::reset(Node* node)
{
    if (type == UMTS_RLC_ENTITY_TM)
    {
        UmtsRlcTmEntity* tmEntity =
            (UmtsRlcTmEntity*)entityData;
        tmEntity->reset(node);
    }
    else if (type == UMTS_RLC_ENTITY_UM)
    {
        UmtsRlcUmEntity* umEntity =
            (UmtsRlcUmEntity*)entityData;
        umEntity->reset(node);
    }
    else if (type == UMTS_RLC_ENTITY_AM)
    {
        UmtsRlcAmEntity* amEntity =
            (UmtsRlcAmEntity*)entityData;
        amEntity->reset(node);
    }
    else
    {
        ERROR_ReportError("Wrong RLC entity");
    }
}

// /**
// FUNCTION::           UmtsRlcEntity::store
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            store the RLC entity
// PARAMETERS::
//      +node: Node*: Node Pointer
//      +buffer: std:string& : string buffer
// RETURN::   void :          NULL
// **/
void UmtsRlcEntity::store(Node* node, std::string& buffer)
{
    buffer.append((char*)&sduSeqNum, sizeof(sduSeqNum));

    std::string buf;
    if (type == UMTS_RLC_ENTITY_TM)
    {
    }
    else if (type == UMTS_RLC_ENTITY_UM)
    {
        UmtsRlcUmEntity* umEntity =
            (UmtsRlcUmEntity*)entityData;
        umEntity->store(node, buf);
    }
    else if (type == UMTS_RLC_ENTITY_AM)
    {
        UmtsRlcAmEntity* amEntity =
            (UmtsRlcAmEntity*)entityData;
        amEntity->store(node, buf);
    }
    else
    {
        ERROR_ReportError("Wrong RLC entity");
    }
    size_t size = buf.size();
    buffer.append((char*)&size, sizeof(size));
    buffer.append(buf);
}

// /**
// FUNCTION::           UmtsRlcEntity::restore
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            restore the RLC entity
// PARAMETERS::
//      +node: Node*: Node Pointer
//      +buffer: const char*: The string buffer
//      +bufSize: size_t: The string buffer size
// RETURN::    void:         NULL
// **/
void UmtsRlcEntity::restore(Node* node, const char* buffer, size_t bufSize)
{
    size_t index = 0;
    memcpy(&sduSeqNum, &buffer[index], sizeof(sduSeqNum));
    index += sizeof(sduSeqNum);

    size_t size;
    memcpy(&size, &buffer[index], sizeof(size));
    index += sizeof(size);
    if (type == UMTS_RLC_ENTITY_TM)
    {
    }
    else if (type == UMTS_RLC_ENTITY_UM)
    {
        UmtsRlcUmEntity* umEntity =
            (UmtsRlcUmEntity*)entityData;
        umEntity->restore(node, &buffer[index], size);
    }
    else if (type == UMTS_RLC_ENTITY_AM)
    {
        UmtsRlcAmEntity* amEntity =
            (UmtsRlcAmEntity*)entityData;
        amEntity->restore(node, &buffer[index], size);
    }
    else
    {
        ERROR_ReportError("Wrong RLC entity");
    }
    index += size;
    ERROR_Assert(index == bufSize, "wrong format");
}

// /**
// FUNCTION::       UmtsRlcSubmitSduToUpperLayer
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        To submit a SDU to upper layer
// PARAMETERS::
//      +node:   Node*:    node pointer
//      +interfaceIndex: int:  the interface index
//      +rlcEntity: UmtsRlcEntity*: The RLC entity
//      +msg: Message*:   The SDU message to be submitted
// RETURN::  void:       NULL
// **/
static
void UmtsRlcSubmitSduToUpperLayer(
    Node* node,
    int interfaceIndex,
    UmtsRlcEntity* rlcEntity,
    Message* msg)
{
    MESSAGE_SetLayer(msg, MAC_LAYER, MAC_PROTOCOL_CELLULAR);

    UmtsLayer2PktToUpperlayerInfo* info
        = (UmtsLayer2PktToUpperlayerInfo*)
          MESSAGE_InfoAlloc(node,
                            msg,
                            sizeof(UmtsLayer2PktToUpperlayerInfo));
    info->rbId = rlcEntity->rbId;
    info->ueId = rlcEntity->ueId;

    UmtsRlcData* rlcData = UmtsRlcGetSubLayerData(node, interfaceIndex);
    rlcData->stats.numSdusToUpperLayer++;

    if (DEBUG_FRAGASSEMB
        && rlcEntity->ueId == DEBUG_UE_ID
        && rlcEntity->rbId == DEBUG_RB_ID
        && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x2)
            || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x1)))
    {
        UmtsRlcSduInfo* sduInfo = (UmtsRlcSduInfo*)
                                   MESSAGE_ReturnInfo(
                                        msg,
                                        INFO_TYPE_UmtsRlcSduInfo);
        ERROR_Assert(sduInfo,
            "cannot get sequence info from assemblied SDU message\n");

        printf("\nNode: %d RB: %d RLC reassembled the "
            "received messages, SN %d: \n",
            node->nodeId, rlcEntity->rbId, sduInfo->sduSeqNum);
        //UmtsPrintMessage(std::cout, msg);
    }

    //A delay is appied here to submit the packet to layer3
    //to prevent the layer3 from modifying layer2/layer1 configuration
    //instantly
    MESSAGE_SetEvent(msg, MSG_MAC_UMTS_LAYER2_HandoffPacket);
    MESSAGE_Send(node, msg, UMTS_LAYER2_HANDOFFPKT_DELAY);
    MESSAGE_SetInstanceId(msg, (short)interfaceIndex);
}

///////////////////////////////////////////////////////////////////////////
// Class UmtsRlcHeader member functions
///////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION::   UmtsRlcHeader::UmtsRlcHeader
// LAYER::      UMTS Layer2 RLC
// PURPOSE::    RLC PDU header class contructor
// PARAMETERS::
//      +type:  AM/UM
//      +liLen: Number of octets used to contain length indicator
// RETURN::     NONE
// **/
UmtsRlcHeader::UmtsRlcHeader(
    UmtsRlcEntityType type,
    unsigned int liOctetLen):mode(type), liLen(liOctetLen)
{
    oct1 = 0;
    oct2 = 0;
    liHead = NULL;
    liSeekPtr = liHead;

    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(type == UMTS_RLC_ENTITY_UM
        || type == UMTS_RLC_ENTITY_AM, errorMsg);
    ERROR_Assert(liOctetLen == 1 || liOctetLen == 2, errorMsg);

    if (mode == UMTS_RLC_ENTITY_UM)
    {
        hdrSize = 1;
    }
    else if (mode == UMTS_RLC_ENTITY_AM)
    {
        hdrSize = 2;
        UmtsRlcSetDCBitInOctet(oct1);
    }
}

// /**
// FUNCTION::   UmtsRlcHeader::~UmtsRlcHeader
// LAYER::      UMTS Layer2 RLC
// PURPOSE::    RLC PDU header class destructor
// PARAMETERS::
// RETURN::     NONE
// **/
UmtsRlcHeader::~UmtsRlcHeader()
{
    LiList* liPtr = liHead;
    while (liPtr)
    {
        LiList* liNext = liPtr->next;
        delete liPtr;
        liPtr = liNext;
    }
}

// /**
// FUNCTION::   UmtsRlcHeader::clear
// LAYER::      UMTS Layer2 RLC
// PURPOSE::    clear all the header data stored
// PARAMETERS::
// RETURN::     NONE
// **/
void UmtsRlcHeader::clear()
{
    LiList* liPtr = liHead;
    while (liPtr)
    {
        LiList* liNext = liPtr->next;
        delete liPtr;
        liPtr = liNext;
    }
    oct1 = 0;
    oct2 = 0;
    if (mode == UMTS_RLC_ENTITY_UM)
    {
        hdrSize = 1;
    }
    else if (mode == UMTS_RLC_ENTITY_AM)
    {
        hdrSize = 2;
        UmtsRlcSetDCBitInOctet(oct1);
    }
    liHead = NULL;
    liSeekPtr = liHead;
}

// /**
// FUNCTION::   UmtsRlcHeader::GetHdr
// LAYER::      UMTS Layer2 RLC
// PURPOSE::    return a PDU header from the structure
// PARAMETERS::
//      +hdr: char*: RLC header
// RETURN::     void : NULL
// **/
void UmtsRlcHeader::GetHdr(char* hdr) const
{
    int index = 0;
    if (mode == UMTS_RLC_ENTITY_UM)
    {
        *hdr = oct1;
        index += 1;
    }
    else
    {
        *hdr = oct1;
        *(hdr+1) = oct2;
        index += 2;
    }

    LiList* liPtr = liHead;
    while (liPtr)
    {
        if (liLen == 1)
        {
            memcpy(hdr+index, &(liPtr->val.octet), liLen);
        }
        else
        {
            memcpy(hdr+index, &(liPtr->val.doubleOctet), liLen);
        }
        liPtr = liPtr->next;
        index += liLen;
    }
}

// /**
// FUNCTION::   UmtsRlcHeader::AddLiOctet
// LAYER::      UMTS Layer2 RLC
// PURPOSE::    Add a length indicator and E bit fields in
//              the header, set the LI value
// PARAMETERS::
//      +liVal: UInt8: The LI value to be set
// RETURN::   void:  NULL
// **/
void UmtsRlcHeader::AddLiOctet(UInt8 liVal)
{
    LiList* newLi = new LiList;
    UmtsRlcSetHead7BitInOctet(newLi->val.octet, liVal);
    newLi->next = NULL;
    if (liHead)
    {
        LiList* liPtr = getLastLi();
        UmtsRlcSetEbitInOctet(liPtr->val.octet);
        liPtr->next = newLi;
    }
    else
    {
        liHead = newLi;
        if (mode == UMTS_RLC_ENTITY_UM)
        {
            UmtsRlcSetEbitInOctet(oct1);
        }
        else
        {
            UmtsRlcSetEbitInOctet(oct2);
        }
        liSeekPtr = liHead;
    }
    hdrSize += liLen;
}

// /**
// FUNCTION::   UmtsRlcHeader::AddLiDbOctet
// LAYER::      UMTS Layer2 RLC
// PURPOSE::    Add a length indicator and E bit fields in
//              the header, set the LI value
// PARAMETERS::
//      +liVal: UInt16: The LI value to be set
// RETURN::   void:  NULL
// **/
void UmtsRlcHeader::AddLiDbOctet(UInt16 liVal)
{
    LiList* newLi = new LiList;
    UmtsRlcSetHead15BitInDbOctet(newLi->val.doubleOctet, liVal);
    newLi->next = NULL;
    if (liHead)
    {
        LiList* liPtr = getLastLi();
        UmtsRlcSetEbitInDbOctet(liPtr->val.doubleOctet);
        liPtr->next = newLi;
    }
    else
    {
        liHead = newLi;
        if (mode == UMTS_RLC_ENTITY_UM)
        {
            UmtsRlcSetEbitInOctet(oct1);
        }
        else
        {
            UmtsRlcSetEbitInOctet(oct2);
        }
        liSeekPtr = liHead;
    }
    hdrSize += liLen;
}

// /**
// FUNCTION::   UmtsRlcHeader::BuildHdrStruct
// LAYER::      UMTS Layer2 RLC
// PURPOSE::    Construct a UmtsRlcHeader struct from the real header
// PARAMETERS::
//      +rlcHead:  const char*:  The real header in byte stream
// RETURN::  void:   NONE
// **/
void UmtsRlcHeader::BuildHdrStruct(const char* rlcHeader)
{
    unsigned int eBit = 0;
    int sizeCount = 0;
    if (mode == UMTS_RLC_ENTITY_UM)
    {
        oct1 = *rlcHeader;
        eBit = UmtsRlcGetEbitFromOctet(oct1);
        sizeCount += 1;
    }
    else
    {
        oct1 = *rlcHeader;
        oct2 = *(rlcHeader + 1);
        eBit = UmtsRlcGetEbitFromOctet(oct2);
        sizeCount += 2;
    }

    LiList* currentLi = NULL;
    while (eBit)
    {
        LiList* liPtr = new LiList;
        liPtr->next = NULL;
        if (liLen ==  1)
        {
            memcpy(&(liPtr->val.octet), rlcHeader+sizeCount, 1);
            eBit = UmtsRlcGetEbitFromOctet(liPtr->val.octet);
            sizeCount += 1;
        }
        else
        {
            memcpy(&(liPtr->val.doubleOctet), rlcHeader+sizeCount, 2);
            eBit = UmtsRlcGetEbitFromDbOctet(liPtr->val.doubleOctet);
            sizeCount += 2;
        }
        if (!liHead)
        {
            liHead = liPtr;
            liSeekPtr = liHead;
        }
        else
        {
            currentLi->next = liPtr;
        }
        currentLi = liPtr;
    }
    hdrSize = sizeCount;
}

// /**
// FUNCTION::   UmtsRlcHeader::ReadNextLi
// LAYER::      UMTS Layer2 RLC
// PURPOSE::    Read the next LI value
// PARAMETERS::
//      +liVal:  unsigned int&: The LI value
// RETURN::     BOOL : success or not
// **/
BOOL UmtsRlcHeader::ReadNextLi(unsigned int& liVal)
{
    if (liSeekPtr)
    {
        if (liLen == 1)
        {
            liVal = static_cast<unsigned int>
                (UmtsRlcGetHead7BitFromOctet(liSeekPtr->val.octet));
        }
        else
        {
            liVal = static_cast<unsigned int>
               (UmtsRlcGetHead15BitFromDbOctet(liSeekPtr->val.doubleOctet));
        }
        liSeekPtr = liSeekPtr->next;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION::   UmtsRlcHeader::PeekNextLi
// LAYER::      UMTS Layer2 RLC
// PURPOSE::    Read the next LI value without moving the seek pointer
// PARAMETERS::
//      +liVal: unsigned int&:  The LI value
// RETURN::     BOOL : success or not
// **/
BOOL UmtsRlcHeader::PeekNextLi(unsigned int& liVal)
{
    if (liSeekPtr)
    {
        if (liLen == 1)
        {
            liVal = static_cast<unsigned int>
                (UmtsRlcGetHead7BitFromOctet(liSeekPtr->val.octet));
        }
        else
        {
            liVal = static_cast<unsigned int>
               (UmtsRlcGetHead15BitFromDbOctet(liSeekPtr->val.doubleOctet));
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION::   operator<<
// LAYER::      UMTS Layer2 RLC
// PURPOSE::    Print out UmtsRlcHeader Struct
// PARAMETERS::
//      +os:    std::ostream&: Output stream
//      +hdr:   const UmtsRlcHeader&: A UmtsRlcHeader Struct to be printed
// RETURN::     std::ostream&
// **/
std::ostream& operator<<(std::ostream& os, const UmtsRlcHeader& hdr)
{
    os << "mode: " << hdr.mode << "  "
        << "Number of octet for each Length indicator field: "
        << hdr.liLen << std::endl;
    os << "Header Size: " << hdr.hdrSize << std::endl;
    //os << "Sequence number: ";
    if (hdr.mode == UMTS_RLC_ENTITY_UM)
    {
        //os << (int)UmtsRlcGetHead7BitFromOctet(hdr.oct1) << "   ";
        os << "E bit: "
           << (int)UmtsRlcGetEbitFromOctet(hdr.oct1)
           << std::endl;
    }
    else
    {
        os << "E bit: "
           << (int)UmtsRlcGetEbitFromOctet(hdr.oct1)
           << std::endl;
    }

    UmtsRlcHeader::LiList* liPtr = hdr.liHead;
    while (liPtr)
    {
        os << "Length indicator: ";
        if (hdr.liLen == 1)
        {
            os << (int)UmtsRlcGetHead7BitFromOctet(liPtr->val.octet) << " ";
            os << "E bit: "
               << (int)UmtsRlcGetEbitFromOctet(liPtr->val.octet)
               << std::endl;
        }
        else
        {
            os << (int)UmtsRlcGetHead15BitFromDbOctet(liPtr->val.doubleOctet)
               << "   ";
            os << "E bit: "
               << (int)UmtsRlcGetEbitFromDbOctet(liPtr->val.doubleOctet)
               << std::endl;
        }
        liPtr = liPtr->next;
    }
    os << std::endl;
    return os;
}

// /**
// FUNCTION::       UmtsRlcFindTmTransEntity
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Search for a RLC TM transmitting entity
// PARAMETERS::
//      +entity:    UmtsRlcEntity*: RLC entity structure
// RETURN::         UmtsRlcTmTransEntity: TM transmitting entity
// **/
static
UmtsRlcTmTransEntity* UmtsRlcFindTmTransEntity(UmtsRlcEntity* entity)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->entityData, errorMsg);

    return ((UmtsRlcTmEntity*)entity->entityData)->txEntity;
}

// /**
// FUNCTION::       UmtsRlcFindTmRcvEntity
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Search for a RLC TM receiving entity
// PARAMETERS::
//      +entity:    UmtsRlcEntity*: RLC entity structure
// RETURN::         UmtsRlcTmRcvEntity: TM receiving entity
// **/
static
UmtsRlcTmRcvEntity* UmtsRlcFindTmRcvEntity(UmtsRlcEntity* entity)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->entityData, errorMsg);

    return ((UmtsRlcTmEntity*)entity->entityData)->rxEntity;
}

// /**
// FUNCTION::       UmtsRlcFindUmTransEntity
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Search for a RLC UM transmitting entity
// PARAMETERS::
//      +entity:    UmtsRlcEntity*: RLC entity structure
// RETURN::         UmtsRlcUmTransEntity: UM transmitting entity
// **/
static
UmtsRlcUmTransEntity* UmtsRlcFindUmTransEntity(UmtsRlcEntity* entity)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->entityData, errorMsg);

    return ((UmtsRlcUmEntity*)entity->entityData)->txEntity;
}

// /**
// FUNCTION::       UmtsRlcFindUmRcvEntity
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Search for a RLC UM receiving entity
// PARAMETERS::
//      +entity:    UmtsRlcEntity*: RLC entity structure
// RETURN::         UmtsRlcUmRcvEntity : UM receiving entity
// **/
static
UmtsRlcUmRcvEntity* UmtsRlcFindUmRcvEntity(UmtsRlcEntity* entity)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->entityData, errorMsg);

    return ((UmtsRlcUmEntity*)entity->entityData)->rxEntity;
}

#if 0
// /**
// FUNCTION::       UmtsRlcFindAmEntity
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Search for a RLC AM entity
// PARAMETERS::
//      +entity:    RLC entity structure
// RETURN::         UmtsRlcAmEntity
// **/
static
UmtsRlcAmEntity* UmtsRlcFindAmEntity(UmtsRlcEntity* entity)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->entityData, errorMsg);

    return (UmtsRlcAmEntity*)entity->entityData;
}
#endif

// /**
// FUNCTION::           UmtsRlcUmEnquePdu
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Insert fragmented PDU into transmission buffer
// PARAMETERS::
//      +node:   Node*:       Node pointer
//      +rlcEntity:  UmtsRlcEntity*:   The RLC entity
//      +pduMsg:   Message*:     The PDU
// RETURN::       void:      NULL
// **/
static
void UmtsRlcUmEnquePdu(
    Node* node,
    UmtsRlcEntity* rlcEntity,
    Message* pduMsg)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcUmEnquePdu: ";
    UmtsRlcUmTransEntity* umTxEntity =
        UmtsRlcFindUmTransEntity(rlcEntity);
    ERROR_Assert(umTxEntity, errorMsg);
    std::deque<Message*>* txPduBuf = umTxEntity->pduBuf;

    if (DEBUG_FRAGASSEMB
        && rlcEntity->ueId == DEBUG_UE_ID
        && rlcEntity->rbId ==  DEBUG_RB_ID
        && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x1)
            || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x2)))
    {
        UmtsLayer2PrintRunTimeInfo(node, 0, UMTS_LAYER2_SUBLAYER_RLC);
        UmtsRlcHeader pduHdrStruct(UMTS_RLC_ENTITY_UM,
                                   umTxEntity->liOctetsLen);
        pduHdrStruct.BuildHdrStruct(MESSAGE_ReturnPacket(pduMsg));

        std::cout << "Node: "<< node->nodeId
            << " RB: " << (int)rlcEntity->rbId
            << " inserts fragmented PDU into transmitting buffer, "
            << std::endl
            << "Sequence number: " << umTxEntity->sndNext << std::endl
            << " PDU header: "<< std::endl;

        std::cout << pduHdrStruct;
        UmtsRlcPrintPdu(pduMsg);
        printf("\n");
    }

    //Sequence number can be set for UMP PDU during fragmentation,
    //because once the fragmentation is done, PDUs are all sent out.
    unsigned int& seqNum = umTxEntity->sndNext;
    UmtsRlcUmSetSeqNum(pduMsg, seqNum);
    UmtsRlcUmIncSN(seqNum);
    txPduBuf->push_back(pduMsg);
}

// /**
// FUNCTION::           UmtsRlcAmEnquePdu
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Insert fragmented PDU into transmission buffer
// PARAMETERS::
//      +node: Node*:         Node pointer
//      +rlcEntity:  UmtsRlcEntity*:   The RLC entity
//      +pduMsg:  Message*:      The PDU
// RETURN::    void:         NULL
// **/
static
void UmtsRlcAmEnquePdu(
    Node* node,
    UmtsRlcEntity* rlcEntity,
    Message* pduMsg)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcAmEnquePdu: ";
    UmtsRlcAmEntity* amEntity =
        (UmtsRlcAmEntity*)rlcEntity->entityData;
    ERROR_Assert(amEntity, errorMsg);
    std::deque<Message*>* txPduBuf = amEntity->transPduBuf;

    txPduBuf->push_back(pduMsg);
    amEntity->bufSize += amEntity->sndPduSize;
}

// /**
// FUNCTION::       UmtsRlcAmHandlePollTimeOut
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Handle a reset timeout event
// PARAMETERS::
//      +node:  Node*:    The node pointer
//      +iface: int:    The interface index
//      +amEntity: UmtsRlcAmEntity*: AM entity
// RETURN::    void:     NULL
// **/
static
void UmtsRlcAmHandlePollTimeOut(
    Node* node,
    int iface,
    UmtsRlcAmEntity* amEntity,
    char* additionalInfo)
{
    UmtsRlcPollTimerInfo* pollInfo =
        (UmtsRlcPollTimerInfo*)additionalInfo;
    if (pollInfo->sndNextSn == amEntity->pollTimerInfo.sndNextSn)
    {
        amEntity->pollTriggered = TRUE;
        //After the trigger poll is sent, the poll timer
        //will be started
    }
    amEntity->pollTimerMsg = NULL;
}

// /**
// FUNCTION::       UmtsRlcHandleUpdateGuiTimer
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Handle GUI Update timer
// PARAMETERS::
//      +node:  Node*:    The node pointer
//      +iface: int:    The interface index
// RETURN::     void:    NULL
// **/
static
void UmtsRlcHandleUpdateGuiTimer(
    Node* node,
    int iface)
{
    if (!node->guiOption)
    {
        return;
    }

    UmtsRlcData* rlcData = UmtsRlcGetSubLayerData(node, iface);

    if (UmtsGetNodeType(node) == CELLULAR_UE)
    {
        double avgThruput;

        avgThruput = (double)(rlcData->stats.numOutControlByte +
                      rlcData->stats.numOutDataByte) * 8 /
                      (UMTS_DYNAMIC_STAT_AVG_TIME_WINDOW /SECOND) ;
        GUI_SendRealData(
            node->nodeId,
            rlcData->stats.ulThruputGuiId,
            avgThruput,
            node->getNodeTime());

        avgThruput = (double)(rlcData->stats.numInControlByte +
                      rlcData->stats.numInDataByte) * 8 /
                      (UMTS_DYNAMIC_STAT_AVG_TIME_WINDOW /SECOND);
        GUI_SendRealData(
            node->nodeId,
            rlcData->stats.dlThruputGuiId,
            avgThruput,
            node->getNodeTime());
    }
    else
    {
        double avgThruput;

        avgThruput = (double)(rlcData->stats.numOutControlByte +
                      rlcData->stats.numOutDataByte) * 8 /
                      (UMTS_DYNAMIC_STAT_AVG_TIME_WINDOW /SECOND);
        GUI_SendRealData(
            node->nodeId,
            rlcData->stats.dlThruputGuiId,
            avgThruput,
            node->getNodeTime());

        avgThruput = (double)(rlcData->stats.numInControlByte +
                      rlcData->stats.numInDataByte) * 8 /
                      (UMTS_DYNAMIC_STAT_AVG_TIME_WINDOW /SECOND);
        GUI_SendRealData(
            node->nodeId,
            rlcData->stats.ulThruputGuiId,
            avgThruput,
            node->getNodeTime());
    }

    // reset the statistics for next round calculation
    rlcData->stats.numInControlByte = 0;
    rlcData->stats.numInDataByte = 0;
    rlcData->stats.numOutControlByte = 0;
    rlcData->stats.numOutDataByte = 0;
}

///////////////////////////////////////////////////////////////////////////
// Class UmtsRlcUmAmFragment member functions
///////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION::           UmtsRlcUmAmFragmentEntity::AddRlcHeader
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Add a RLC header into the packet header field
//                      of the PDU message
// PARAMETERS::
//      +node:          Node pointer
//      +destMessage:   The message to be copied to
//      +srcMessage:    The message to be copied from
// RETURN::             NULL
// **/
void UmtsRlcUmAmFragmentEntity::AddRlcHeader(
                                    Node* node,
                                    const UmtsRlcHeader& pduHdrStruct,
                                    Message* pduMsg)
{
    int pduHdrSize = pduHdrStruct.GetHdrSize();
    char* pduHdr = (char*)MEM_malloc(pduHdrSize*sizeof(char));
    pduHdrStruct.GetHdr(pduHdr);
    MESSAGE_AddHeader(node,
                    pduMsg,
                    pduHdrSize,
                    TRACE_UMTS_LAYER2);
    memcpy(MESSAGE_ReturnPacket(pduMsg), pduHdr, pduHdrSize);
    MEM_free(pduHdr);
}

// /**
// FUNCTION::           UmtsRlcUmAmFragmentEntity::Fragement
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Fragment or concatenate UM/AM SDUs into PDUs
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         the interface index
//      +sduMsg:        The next SDU to be fragmented
//      +umPduSize:     The UMD PDU size
// RETURN::             BOOL
// **/
BOOL UmtsRlcUmAmFragmentEntity::Fragment(
                                    Node* node,
                                    int iface,
                                    Message* sduMsg,
                                    unsigned int umNumPdus,
                                    unsigned int umPduSize)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcUmAmFragmentEntity::Fragment: ";
    ERROR_Assert(sduMsg, errorMsg);

    UmtsRlcEntityType mode = rlcEntity->type;

    unsigned int pduSize = 0;
    unsigned int liLen = 0;
    BOOL alterEbit = FALSE;

    typedef void (* UmtsRlcEnquePduFunc)(Node*, UmtsRlcEntity*, Message*);
    UmtsRlcEnquePduFunc enquePduFunc = NULL;

    if (mode == UMTS_RLC_ENTITY_UM)
    {
        UmtsRlcUmTransEntity* umTxEntity =
            UmtsRlcFindUmTransEntity(rlcEntity);
        ERROR_Assert(umTxEntity, errorMsg);
        pduSize = umPduSize;
        liLen = umTxEntity->liOctetsLen;
        alterEbit = umTxEntity->alterEbit;

        enquePduFunc = &UmtsRlcUmEnquePdu;
        ERROR_Assert(pduSize > 1 + 2*liLen, errorMsg);
        // pduSize - # seqNum octets -# 1 LI octets < 
        // the first UM special LI value
        ERROR_Assert(pduSize <= UMTS_RLC_LI_15BIT_PREVSHORT, errorMsg);
        if (liLen == 1)
        {
            ERROR_Assert(pduSize <= UMTS_RLC_UM_LI_PDUTHRESH, errorMsg);
        }
    }
    else if (mode == UMTS_RLC_ENTITY_AM)
    {
        UmtsRlcAmEntity* amEntity =
            (UmtsRlcAmEntity*)rlcEntity->entityData;
        ERROR_Assert(amEntity, errorMsg);
        pduSize = amEntity->sndPduSize;
        liLen = amEntity->sndLiOctetsLen;

        enquePduFunc = &UmtsRlcAmEnquePdu;
        ERROR_Assert(pduSize > 2 + liLen, errorMsg);
        //pduSize - # seqNum octets -# 1 LI octets < 
        // the first AM special LI value
        ERROR_Assert(pduSize <= UMTS_RLC_LI_15BIT_ONEPDU, errorMsg);
        if (liLen == 1)
        {
            ERROR_Assert(pduSize <= UMTS_RLC_AM_LI_PDUTHRESH,
                errorMsg);
        }
    }
    else
    {
        ERROR_ReportError(errorMsg);
    }

    UmtsRlcHeader pduHdrStruct(mode, liLen);

    Message* currentPdu;            //The first message belonging to 
                                    // the PDU currently being handled
    Message* currentMsg;            //The message currently being handled,
                                    //which belongs to the PDU
    int sduRealPayloadSize = MESSAGE_ReturnActualPacketSize(sduMsg);
    int sduVirtualPayloadSize = MESSAGE_ReturnVirtualPacketSize(sduMsg);
    int sduPacketSize = sduRealPayloadSize + sduVirtualPayloadSize;
    int sduRealIndex = 0;
    int sduVirtualIndex = 0;

    int pduFreeSpaceLen = 0;
    //First get free space left in the last PDU
    if (lastPdu)
    {
        if (prevPduFill == FALSE && prevPduShort == FALSE)
        {
            //Build a header struct from the header info in the first msg
            //of the PDU,do not remove the header since we don't know 
            //whether we will add segment of new SDU into the last PDU.
            pduHdrStruct.BuildHdrStruct(MESSAGE_ReturnPacket(lastPdu));
            //MESSAGE_RemoveHeader(node,
            //                     lastPdu,
            //                     pduHdrStruct.GetHdrSize(),
            //                     TRACE_UMTS_LAYER2);

            pduFreeSpaceLen = pduSize - pduHdrStruct.GetHdrSize();
            unsigned int liVal;
            do { } while (pduHdrStruct.ReadNextLi(liVal));
            if (liLen == 1)
            {
                ERROR_Assert(liVal != UMTS_RLC_LI_7BIT_PREVFILL
                    && liVal != UMTS_RLC_LI_7BIT_SDUSTART
                    && liVal != UMTS_RLC_LI_7BIT_SDUFILL
                    && liVal != UMTS_RLC_LI_7BIT_ONEPDU
                    && liVal != UMTS_RLC_LI_7BIT_PADDING
                    && liVal <= pduFreeSpaceLen - liLen,
                    errorMsg);
            }
            else
            {
                ERROR_Assert(liVal != UMTS_RLC_LI_7BIT_PREVFILL
                    && liVal != UMTS_RLC_LI_15BIT_PREVFILL
                    && liVal != UMTS_RLC_LI_15BIT_SDUSHORT
                    && liVal != UMTS_RLC_LI_15BIT_PREVSHORT
                    && liVal != UMTS_RLC_LI_15BIT_SDUSTART
                    && liVal != UMTS_RLC_LI_15BIT_SDUFILL
                    && liVal != UMTS_RLC_LI_15BIT_ONEPDU
                    && liVal != UMTS_RLC_LI_15BIT_PADDING
                    && liVal <= pduFreeSpaceLen - liLen,
                    errorMsg);
            }
            pduFreeSpaceLen -= liVal;
        }
    }

    //Then decide if the fragmentation of the next SDU should be
    //proceeded.
    if (mode == UMTS_RLC_ENTITY_UM)
    {
        UmtsRlcUmTransEntity* umTxEntity =
            UmtsRlcFindUmTransEntity(rlcEntity);
        unsigned int numPdusDone = (unsigned int)umTxEntity->pduBuf->size();

        if (lastPdu)
        {
            numPdusDone += 1;
            if (numPdusDone >= umNumPdus)
            {
                //Call this function because the next SDU won't be
                //fragmented this TTI
                RequestLastPdu(node);
                return FALSE;
            }
        }

        int numPdusForNextSdu = 0;
        if (pduFreeSpaceLen == 0)
        {
            //Conservatively estimate each PDU header should have 2 length
            //indicators
            //Normally it is true for the scenarios that SDU should 
            // be fragmented
            //into multiple PDUs, except for the last PDU may have 3.
            //Since we only deal one SDU each time, this assumption holds.
            numPdusForNextSdu = sduPacketSize/(pduSize - 1 - 2*liLen);
            if (sduPacketSize%(pduSize - 1 -2*liLen) != 0)
            {
                numPdusForNextSdu += 1;
            }
        }
        else if (pduFreeSpaceLen > 0)
        {
            //When previous PDU has some space left for the new SDU, if
            //its size is smaller than that of the SDU, estimate additional
            //number of PDUs needed keeping in mind that part of SDU packet
            //is filled
            //into the last PDU
            if (sduPacketSize > pduFreeSpaceLen)
            {
                numPdusForNextSdu = (sduPacketSize
                    - pduFreeSpaceLen)/(pduSize - 1 - 2*liLen);
                if ((sduPacketSize-pduFreeSpaceLen)
                          %(pduSize - 1- 2*liLen) != 0)
                {
                    numPdusForNextSdu += 1;
                }
            }
            //If the SDU can be filled into the last PDU 
            //with a length indicator
            //missing, conservatively consider an 
            //additional PDU may be needed
            //in case that next SDU is missing or 
            //cannot be fragmented this time
            else if (sduPacketSize > (pduFreeSpaceLen - (int)liLen))
            {
                numPdusForNextSdu += 1;
            }
            //Otherwise, no new PDU is needed.
        }
        //If the estimated number of PDUs needed 
        //for fragment the SDU is larger
        //than requested, stop fragmentation.
        if (numPdusForNextSdu + numPdusDone > umNumPdus)
        {
            if (lastPdu)
            {
                //Call this function because the next SDU won't be
                //fragmented this TTI
                RequestLastPdu(node);
            }
            return FALSE;
        }
    }

    if (DEBUG_FRAGASSEMB
        && rlcEntity->ueId == DEBUG_UE_ID
        && rlcEntity->rbId == DEBUG_RB_ID
        && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x1)
            || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x2)))
    {
        UmtsLayer2PrintRunTimeInfo(
            node,
            0,
            UMTS_LAYER2_SUBLAYER_RLC);
        UmtsRlcSduInfo* sduInfo = (UmtsRlcSduInfo*)
                           MESSAGE_ReturnInfo(
                                sduMsg,
                                INFO_TYPE_UmtsRlcSduInfo);
        printf(
            "Node: %d RB: %d RLC entity is fragmenting the SDU: SN %d: \n",
            node->nodeId, rlcEntity->rbId, sduInfo->sduSeqNum);

        //UmtsPrintMessage(std::cout, sduMsg);
    }

    int sduDataLeftLen = sduPacketSize;
    currentMsg = lastPdu;

#if 0
    //If previous PDU has no length indicator for the last segment
    //now it can be push into buffer since next PDU can be set LI
    //to indicate that for it.
    if (lastPdu && (prevPduFill == TRUE || prevPduShort == TRUE))
    {
        currentMsg = lastPdu;
        Message* nextMsg = currentMsg->next;
        while (nextMsg)
        {
            currentMsg = nextMsg;
            nextMsg = nextMsg->next;
        }
        UmtsRlcSduSegInfo* info = (UmtsRlcSduSegInfo*)
                                    MESSAGE_ReturnInfo(
                                        currentMsg,
                                        INFO_TYPE_UmtsRlcSduSegInfo);
        info->traits |= UMTS_RLC_SDUSEGMENT_END;

        enquePduFunc(node, rlcEntity, lastPdu);
        lastPdu = NULL;
    }
#endif // 0

    //If the previous PDU has free space for new SDU,
    //Create a new message as the last part of the PDU.
    if (lastPdu)
    {
        //Remove the RLC header in the last PDU, so we can
        //add new LI for the segment to be added
        MESSAGE_RemoveHeader(node,
                             lastPdu,
                             pduHdrStruct.GetHdrSize(),
                             TRACE_UMTS_LAYER2);

        currentPdu = lastPdu;
        currentMsg = lastPdu;
        Message* nextMsg = currentMsg->next;
        while (nextMsg)
        {
            currentMsg = nextMsg;
            nextMsg = nextMsg->next;
        }
        //We need to adjust the previous message's virtualPayloadSize
        //back by removing those padding because of free space
        currentMsg->virtualPayloadSize -= pduFreeSpaceLen;
        ERROR_Assert(currentMsg->virtualPayloadSize>=0, errorMsg);

        //Allocate a new message to contain data from the new SDU.
        //A PDU could contain a list of messages, each relating to
        //different SDU
        nextMsg = MESSAGE_Alloc(node,
                                MAC_LAYER,
                                0,
                                0);
        currentMsg->next = nextMsg;
        currentMsg = nextMsg;
        //Copy SDU message simulation-related 
        //information into the PDU message
        //that carries the first segment of the SDU
        MESSAGE_CopyInfo(node, currentMsg, sduMsg);

        //add info indicating this segment is start of an SDU
        UmtsRlcSduSegInfo* info = (UmtsRlcSduSegInfo*)
                                  MESSAGE_AddInfo(
                                        node,
                                        currentMsg,
                                        sizeof(UmtsRlcSduSegInfo),
                                        INFO_TYPE_UmtsRlcSduSegInfo);
        ERROR_Assert(info, "cannot allocate enough space for needed info");
        info->traits = 0;
        info->traits |= UMTS_RLC_SDUSEGMENT_START;
    }
    //If the lastPdu has no free space or it has been
    //transmitted before new SDU arrives, a new PDU message need to be
    //allocated.
    else
    {
        currentPdu = MESSAGE_Alloc(node,
                                   MAC_LAYER,
                                   0,
                                   0);
        currentMsg = currentPdu;
        MESSAGE_CopyInfo(node, currentPdu, sduMsg);

        UmtsRlcSduSegInfo* info = (UmtsRlcSduSegInfo*)
                                  MESSAGE_AddInfo(
                                        node,
                                        currentMsg,
                                        sizeof(UmtsRlcSduSegInfo),
                                        INFO_TYPE_UmtsRlcSduSegInfo);
        ERROR_Assert(info, "cannot allocate enough space for needed info");
        info->traits = 0;
        info->traits |= UMTS_RLC_SDUSEGMENT_START;

        //Reduce the available PDU space (for SDU segment) by the size of
        //fixed PDU header
        pduFreeSpaceLen = pduSize - pduHdrStruct.GetHdrSize();

        //If last segment of an SDU is one 
        //octet short of filling lastPDU, and
        //there is no length indicator in previous PDU indicating that,
        //this PDU must indicate that in its 
        //first length indicator no matter
        //what
        if (prevPduShort)
        {
            if (liLen == 2)
            {
                pduHdrStruct.AddLiDbOctet(UMTS_RLC_LI_15BIT_PREVSHORT);
            }
            else
            {
                ERROR_ReportError(
                    "Number of bits used for Length Indicator "
                    "is not allowed to be changed during RLC session.");
                pduHdrStruct.AddLiOctet(UMTS_RLC_LI_7BIT_PREVFILL);
            }
            pduFreeSpaceLen -= liLen;
        }
        //Otherwise, a) if the mode is UM and 
        //alternative E bit interpretation is configured,
        //and if the sdu can be completely filled into 
        //the SDU without segmentation,
        //concatentation and padding, then fill it, 
        //set the first Ebit as 0;
        //b) or this new PDU can be filled with 
        //the whole SDU after a LI is added,
        //c) or if the new PDU can be 1-octet short 
        //filled with the whole SDU
        //after a 2-octet LI is added into the header, 
        //fill the PDU with the SDU
        //and set the first LI with the special LI values 
        //accordingly to indicate that.
        else if (alterEbit
            && (sduPacketSize == pduFreeSpaceLen
                || sduPacketSize == (pduFreeSpaceLen - (int)liLen)
                || (liLen == 2 && sduPacketSize == pduFreeSpaceLen - 3)))
        {
            if (sduRealPayloadSize)
            {
                MESSAGE_PacketAlloc(node,
                                currentMsg,
                                sduRealPayloadSize,
                                TRACE_UMTS_LAYER2);
                memcpy(MESSAGE_ReturnPacket(currentMsg),
                    MESSAGE_ReturnPacket(sduMsg), sduRealPayloadSize);
            }
            else
            {
                MESSAGE_PacketAlloc(
                    node,
                    currentMsg,
                    0,
                    TRACE_UMTS_LAYER2);
            }

            if (sduPacketSize == (pduFreeSpaceLen - (int)liLen))
            {
                if (liLen == 1)
                {
                    pduHdrStruct.AddLiOctet(UMTS_RLC_LI_7BIT_SDUFILL);
                }
                else
                {
                    pduHdrStruct.AddLiDbOctet(UMTS_RLC_LI_15BIT_SDUFILL);
                }
                currentMsg->virtualPayloadSize = sduVirtualPayloadSize;
            }
            else if (sduPacketSize == pduFreeSpaceLen - 3)
            {
                pduHdrStruct.AddLiDbOctet(UMTS_RLC_LI_15BIT_SDUSHORT);
                currentMsg->virtualPayloadSize = sduVirtualPayloadSize + 1;
            }
            else
            {
                currentMsg->virtualPayloadSize = sduVirtualPayloadSize;
            }
            UmtsRlcCopyMessageTraceInfo(node, currentPdu, sduMsg);

            AddRlcHeader(node, pduHdrStruct, currentPdu);

            //Add info indicating that this PDU ends an SDU
            UmtsRlcSduSegInfo* info = (UmtsRlcSduSegInfo*)
                                      MESSAGE_ReturnInfo(
                                          currentPdu,
                                          INFO_TYPE_UmtsRlcSduSegInfo);
            info->traits |= UMTS_RLC_SDUSEGMENT_END;

            enquePduFunc(node, rlcEntity, currentPdu);
            lastPdu = NULL;

            prevPduFill = FALSE;
            prevPduShort = FALSE;

            pduFreeSpaceLen = 0;
            sduDataLeftLen = 0;
        }
        //Otherwise, if previous PDU is completely 
        //filled with the last segment of
        //a SDU and no LI indicates that, 
        //this PDU needs to set the first LI the
        //special value to indicate that
        else if (prevPduFill)
        {
            if (liLen == 1)
            {
                pduHdrStruct.AddLiOctet(UMTS_RLC_LI_7BIT_PREVFILL);
            }
            else
            {
                pduHdrStruct.AddLiDbOctet(UMTS_RLC_LI_15BIT_PREVFILL);
            }
            pduFreeSpaceLen -= liLen;
        }
        //Otherwise, if it is UMD PDU, we need to set the first LI to
        // indicate a SDU starts from the first octet in this PDU
        else if (mode == UMTS_RLC_ENTITY_UM)// && isNodeTypeRnc(node, iface))
        {
            if (liLen == 1)
            {
                pduHdrStruct.AddLiOctet(UMTS_RLC_LI_7BIT_SDUSTART);
            }
            else
            {
                pduHdrStruct.AddLiDbOctet(UMTS_RLC_LI_15BIT_SDUSTART);
            }
            pduFreeSpaceLen -= liLen;
        }
    }

    while (sduDataLeftLen)
    {
        if (pduFreeSpaceLen == 0
            /*|| (pduFreeSpaceLen == 1 && liLen == 2)*/)
        {
            pduHdrStruct.clear();
            currentPdu = MESSAGE_Alloc(node,
                                       MAC_LAYER,
                                       0,
                                       0);
            currentMsg = currentPdu;
            pduFreeSpaceLen = pduSize - pduHdrStruct.GetHdrSize();

            //add info field for each SDU segment
            UmtsRlcSduSegInfo* info = (UmtsRlcSduSegInfo*)
                                      MESSAGE_AddInfo(
                                            node,
                                            currentMsg,
                                            sizeof(UmtsRlcSduSegInfo),
                                            INFO_TYPE_UmtsRlcSduSegInfo);
            ERROR_Assert(info, "cannot allocate enough space for needed info");
            info->traits = 0;
        }

        int pduRealSize = sduRealPayloadSize - sduRealIndex;
        int pduVirtualSize = sduVirtualPayloadSize - sduVirtualIndex;

        //If the left SDU data cannot be filled in the current PDU,
        //Then we fill the PDU with all the SDU data allowed.
        //If alternative Ebit is configured for UM transmission,
        //Add an special LI to indicate that neither the first octet
        //or the last octet of a SDU is in this PDU, if it is the case.
        //Note: we expect this is the most common case, so that it is
        //handled at the begining
        if (sduDataLeftLen > pduFreeSpaceLen)
        {
            if (alterEbit && currentMsg == currentPdu
                && sduDataLeftLen < sduPacketSize)
            {
                if (liLen == 1)
                {
                    pduHdrStruct.AddLiOctet(UMTS_RLC_LI_7BIT_ONEPDU);
                }
                else
                {
                    pduHdrStruct.AddLiDbOctet(UMTS_RLC_LI_15BIT_ONEPDU);
                }
                pduFreeSpaceLen -= liLen;
            }

            if (pduRealSize > pduFreeSpaceLen)
            {
                pduRealSize = pduFreeSpaceLen;
                pduVirtualSize = 0;
            }
            else
            {
                if (pduVirtualSize > pduFreeSpaceLen - pduRealSize)
                {
                    pduVirtualSize = pduFreeSpaceLen - pduRealSize;
                }
            }

            if (pduRealSize > 0)
            {
                MESSAGE_PacketAlloc(node, currentMsg, pduRealSize,
                    TRACE_UMTS_LAYER2);
                memcpy(MESSAGE_ReturnPacket(currentMsg),
                        MESSAGE_ReturnPacket(sduMsg)+sduRealIndex,
                        pduRealSize);
            }
            else
            {
                MESSAGE_PacketAlloc(node, currentMsg, 0,
                   TRACE_UMTS_LAYER2);
            }

            if (pduVirtualSize > 0)
            {
                currentMsg->virtualPayloadSize = pduVirtualSize;
            }
            //If this is the first segment of SDU, we need to copy the trace
            //information of the original SDU message into the PDU message
            if (sduDataLeftLen == sduPacketSize)
            {
                UmtsRlcCopyMessageTraceInfo(node, currentMsg, sduMsg);
            }
            pduFreeSpaceLen -= (pduRealSize + pduVirtualSize);
            AddRlcHeader(node, pduHdrStruct, currentPdu);

            enquePduFunc(node, rlcEntity, currentPdu);
            lastPdu = NULL;

            prevPduShort = FALSE;
            prevPduFill = FALSE;

            sduRealIndex += pduRealSize;
            sduVirtualIndex += pduVirtualSize;
        }
        //Otherwise, copy all left data in the SDU into the PDU
        else
        {
            if (pduRealSize)
            {
                MESSAGE_PacketAlloc(node,
                                    currentMsg,
                                    pduRealSize,
                                    TRACE_UMTS_LAYER2);
                memcpy(MESSAGE_ReturnPacket(currentMsg),
                       MESSAGE_ReturnPacket(sduMsg)+sduRealIndex,
                       pduRealSize);
            }
            else
            {
                MESSAGE_PacketAlloc(node,
                                    currentMsg,
                                    0,
                                    TRACE_UMTS_LAYER2);
            }

            if (sduDataLeftLen == sduPacketSize)
            {
                UmtsRlcCopyMessageTraceInfo(node, currentMsg, sduMsg);
            }

            //a) If the left SDU data can be completely 
            //filled into the current PDU,
            //then don't add LI for this PDU, raise a flag for the next PDU
            //to take care of this
            if (sduDataLeftLen == pduFreeSpaceLen)
            {
                currentMsg->virtualPayloadSize = pduVirtualSize;
                prevPduFill = TRUE;
                prevPduShort = FALSE;
                pduFreeSpaceLen -= sduDataLeftLen;
                AddRlcHeader(node, pduHdrStruct, currentPdu);

                UmtsRlcSduSegInfo* info = (UmtsRlcSduSegInfo*)
                          MESSAGE_ReturnInfo(
                              currentMsg,
                              INFO_TYPE_UmtsRlcSduSegInfo);
                info->traits |= UMTS_RLC_SDUSEGMENT_END;

                enquePduFunc(node, rlcEntity, currentPdu);
                lastPdu = NULL;
            }
            //b) or if the left SDU data can be one-octet-short
            //filled into the PDU, then fill it, and raise a flag
            else if (liLen == 2 && sduDataLeftLen+1 == pduFreeSpaceLen)
            {
                currentMsg->virtualPayloadSize = pduVirtualSize + 1;
                prevPduShort = TRUE;
                prevPduFill = FALSE;
                pduFreeSpaceLen -= sduDataLeftLen;
                AddRlcHeader(node, pduHdrStruct, currentPdu);

                UmtsRlcSduSegInfo* info = (UmtsRlcSduSegInfo*)
                                          MESSAGE_ReturnInfo(
                                              currentMsg,
                                              INFO_TYPE_UmtsRlcSduSegInfo);
                info->traits |= UMTS_RLC_SDUSEGMENT_END;
                info->traits |= UMTS_RLC_SDUSEGMENT_PREVSHORT;
                enquePduFunc(node, rlcEntity, currentPdu);
                lastPdu = NULL;
            }
            //c) If adding a LI can make the left 
            //SDU data completly filled into
            //the PDU, add a LI to indicate the end of the SDU
            else if ((sduDataLeftLen + (int)liLen) == pduFreeSpaceLen)
            {
                currentMsg->virtualPayloadSize = pduVirtualSize;
                prevPduFill = FALSE;
                prevPduShort = FALSE;
                pduFreeSpaceLen -= (liLen + sduDataLeftLen);
                int len = pduSize - pduFreeSpaceLen
                    - pduHdrStruct.GetHdrSize() - liLen;
                if (liLen == 1)
                {
                    pduHdrStruct.AddLiOctet((UInt8)len);
                }
                else
                {
                    pduHdrStruct.AddLiDbOctet((UInt16)len);
                }
                AddRlcHeader(node, pduHdrStruct, currentPdu);

                UmtsRlcSduSegInfo* info = (UmtsRlcSduSegInfo*)
                                          MESSAGE_ReturnInfo(
                                              currentMsg,
                                              INFO_TYPE_UmtsRlcSduSegInfo);
                info->traits |= UMTS_RLC_SDUSEGMENT_END;
                enquePduFunc(node, rlcEntity, currentPdu);
                lastPdu = NULL;
            }
#if 0
            //d) If adding a LI can make the left SDU 
            //data 1-octet-short filled into
            //the PDU, add a LI to indicate the end of the SDU segment
            else if (liLen == 2 && sduDataLeftLen + 3 == pduFreeSpaceLen)
            {
                currentMsg->virtualPayloadSize = pduVirtualSize + 1;
                prevPduFill = FALSE;
                prevPduShort = FALSE;
                pduFreeSpaceLen -= (liLen + sduDataLeftLen);
                int len = pduSize - pduFreeSpaceLen
                    - pduHdrStruct.GetHdrSize() - liLen;
                pduHdrStruct.AddLiDbOctet(len);
                AddRlcHeader(node, pduHdrStruct, currentPdu);

                UmtsRlcSduSegInfo* info = (UmtsRlcSduSegInfo*)
                                          MESSAGE_ReturnInfo(
                                              currentMsg,
                                              INFO_TYPE_UmtsRlcSduSegInfo);
                info->traits |= UMTS_RLC_SDUSEGMENT_END;

                enquePduFunc(node, rlcEntity, currentPdu);
                lastPdu = NULL;
            }
#endif // 0
            //e) This PDU has space left for filling 
            //the next SDU data, so it is
            //kept in the Fragment entity as the lastPdu, 
            //waiting for arrival of
            //next SDU. If there is no SDU arriving by the time
            //the PDU needs to
            //be sent, a PADDING LI needs to be added into the header.
            else
            {
                prevPduFill = FALSE;
                prevPduShort = FALSE;
                pduFreeSpaceLen -= (liLen + sduDataLeftLen);
                //Add all the left space in the current PDU into the
                //virtual payload of the last message in the current PDU
                currentMsg->virtualPayloadSize =
                            pduFreeSpaceLen + pduVirtualSize;
                int len = pduSize - pduFreeSpaceLen
                    - pduHdrStruct.GetHdrSize() - liLen;
                if (liLen == 1)
                {
                    pduHdrStruct.AddLiOctet((UInt8)len);
                }
                else
                {
                    pduHdrStruct.AddLiDbOctet((UInt16)len);
                }
                AddRlcHeader(node, pduHdrStruct, currentPdu);

                UmtsRlcSduSegInfo* info = (UmtsRlcSduSegInfo*)
                                          MESSAGE_ReturnInfo(
                                              currentMsg,
                                              INFO_TYPE_UmtsRlcSduSegInfo);
                info->traits |= UMTS_RLC_SDUSEGMENT_END;
                lastPdu = currentPdu;
            }
        }
        sduDataLeftLen -= (pduRealSize+pduVirtualSize);
    }
    MESSAGE_Free(node, sduMsg);
    return TRUE;
}

// /**
// FUNCTION::           UmtsRlcUmAmFragmentEntity::getLastPdu
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Retrieve the last PDU held by the Fragment Entity
//                      Invoke when the transmission buffer is empty and
//                      MAC request more PDUs. After operation, the RLC
//                      entity's transmission buffer will be filled with
//                      one or two PDUs
// PARAMETERS::
// RETURN::             BOOL
// **/
BOOL UmtsRlcUmAmFragmentEntity::RequestLastPdu(Node* node)
{
    if (!lastPdu)
    {
       return FALSE;
    }

    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcFragment::RequestLastPdu: ";
    UmtsRlcEntityType mode = rlcEntity->type;
    unsigned int pduSize = MESSAGE_ReturnPacketSize(lastPdu);   
                               //pduSize should be obtained from lastPdu

    Message* currentPdu = lastPdu;
    Message* currentMsg = lastPdu;
    Message* nextMsg = currentMsg->next;
    Message* prevMsg = NULL;
    while (nextMsg)
    {
        pduSize += MESSAGE_ReturnPacketSize(nextMsg);
        prevMsg = currentMsg;
        currentMsg = nextMsg;
        nextMsg = nextMsg->next;
    }

    unsigned int liLen = 0;
    BOOL alterEbit = FALSE;
    typedef void (* UmtsRlcEnquePduFunc)(Node*, UmtsRlcEntity*, Message*);
    UmtsRlcEnquePduFunc enquePduFunc = NULL;
    if (mode == UMTS_RLC_ENTITY_UM)
    {
        UmtsRlcUmTransEntity* umTxEntity =
            UmtsRlcFindUmTransEntity(rlcEntity);
        ERROR_Assert(umTxEntity, errorMsg);
        liLen = umTxEntity->liOctetsLen;
        enquePduFunc = &UmtsRlcUmEnquePdu;
        alterEbit = umTxEntity->alterEbit;
    }
    else if (mode == UMTS_RLC_ENTITY_AM)
    {
        UmtsRlcAmEntity* amEntity =
            (UmtsRlcAmEntity*)rlcEntity->entityData;
        ERROR_Assert(amEntity, errorMsg);
        ERROR_Assert(pduSize == amEntity->sndPduSize, errorMsg);
        liLen = amEntity->sndLiOctetsLen;
        enquePduFunc = &UmtsRlcAmEnquePdu;
    }
    else
    {
        ERROR_ReportError(errorMsg);
    }

    UmtsRlcHeader pduHdrStruct(mode, liLen);
    pduHdrStruct.BuildHdrStruct(MESSAGE_ReturnPacket(lastPdu));
    MESSAGE_RemoveHeader(node,
                        lastPdu,
                        pduHdrStruct.GetHdrSize(),
                        TRACE_UMTS_LAYER2);

#if 0
    //a) If the last PDU has been completely or 1-octet short filled
    // with the last segment of an SDU and there is no LIs in its header
    // indicating that, by the time the function is called, next SDU
    // hasn't arrived, so we have split the PDU into two PDUs
    if (prevPduFill || prevPduShort)
    {
        //The current PDU needs to add a LI to indicate the length of
        //the last SDU segment, so a new PDU needs to be created to
        //accommdate LiLen (when prevPduFill) or LiLen -1
        //(when prevPduShort) octets data from the last part of the SDU
        Message* nextPdu = MESSAGE_Alloc(node,
                                         MAC_LAYER,
                                         0,
                                         0);
        //add info field for each SDU segment
        MESSAGE_InfoAlloc(node, nextPdu, sizeof(UmtsRlcSduSegInfo));
        UmtsRlcSduSegInfo* info = (UmtsRlcSduSegInfo*)
                                  MESSAGE_ReturnInfo(
                                      nextPdu,
                                      INFO_TYPE_UmtsRlcSduSegInfo);
        info->traits = 0;
        info->traits |= UMTS_RLC_SDUSEGMENT_END;

        //a) The current message or previous PDU messages have enough space
        //to contain the all the real SDU packet
        if (currentMsg->virtualPayloadSize >= liLen)
        {
            currentMsg->virtualPayloadSize -= liLen;
            unsigned int len = pduSize -
                pduHdrStruct.GetHdrSize() - liLen;
            if (liLen == 1)
            {
                pduHdrStruct.AddLiOctet(len);
            }
            else
            {
                pduHdrStruct.AddLiDbOctet(len);
            }
            AddRlcHeader(node, pduHdrStruct, currentPdu);
            enquePduFunc(node, rlcEntity, currentPdu);

            pduHdrStruct.clear();
            MESSAGE_PacketAlloc(node,
                                nextPdu,
                                0,
                                TRACE_UMTS_LAYER2);
            if (liLen == 1)
            {
                pduHdrStruct.AddLiOctet(liLen);
                pduHdrStruct.AddLiOctet(UMTS_RLC_LI_7BIT_PADDING);
            }
            else
            {
                if (prevPduFill)
                {
                    pduHdrStruct.AddLiDbOctet(liLen);
                }
                else
                {
                    pduHdrStruct.AddLiDbOctet(liLen - 1);
                }
                pduHdrStruct.AddLiDbOctet(UMTS_RLC_LI_15BIT_PADDING);
            }
            AddRlcHeader(node, pduHdrStruct, nextPdu);
            //Next PDU size is fixed, so all the left space are used to
            //contain virtual data (padding)
            nextPdu->virtualPayloadSize = pduSize - pduHdrStruct.GetHdrSize();
            enquePduFunc(node, rlcEntity, nextPdu);
            lastPdu = NULL;
        }
        //b) The new PDU needs to allocate space to accommdate the real
        //data pushed out by LI in the current message.
        else
        {
            unsigned int nextPduRealPacketSize = liLen
                                    - currentMsg->virtualPayloadSize;

            unsigned int currentMsgRealPacketSize =
                            MESSAGE_ReturnActualPacketSize(currentMsg)
                                - nextPduRealPacketSize;
            //The current message's real packet size is reduced, so
            //must move out the real data that have been pushed out
            Message* newMsg = MESSAGE_Alloc(node,
                                            MAC_LAYER,
                                            0,
                                            0);
            MESSAGE_CopyInfo(node, newMsg, currentMsg);
            MESSAGE_PacketAlloc(node,
                                newMsg,
                                currentMsgRealPacketSize,
                                TRACE_UMTS_LAYER2);
            memcpy(MESSAGE_ReturnPacket(newMsg),
                MESSAGE_ReturnPacket(currentMsg), currentMsgRealPacketSize);
            newMsg->virtualPayloadSize = 0;
            UmtsRlcCopyMessageTraceInfo(node, newMsg, currentMsg);
            unsigned int len = pduSize -
                pduHdrStruct.GetHdrSize() - liLen;
            if (liLen == 1)
            {
                pduHdrStruct.AddLiOctet(len);
            }
            else
            {
                pduHdrStruct.AddLiDbOctet(len);
            }

            if (currentPdu != currentMsg)
            {
                ERROR_Assert(prevMsg, errorMsg);
                prevMsg->next = newMsg;
            }
            else
            {
                currentPdu = newMsg;
            }
            AddRlcHeader(node, pduHdrStruct, currentPdu);
            enquePduFunc(node, rlcEntity, currentPdu);

            pduHdrStruct.clear();
            MESSAGE_PacketAlloc(node,
                            nextPdu,
                            nextPduRealPacketSize,
                            TRACE_UMTS_LAYER2);
            memcpy(MESSAGE_ReturnPacket(nextPdu),
                MESSAGE_ReturnPacket(currentMsg)+currentMsgRealPacketSize,
                nextPduRealPacketSize);
            if (liLen == 1)
            {
                pduHdrStruct.AddLiOctet(liLen);
                pduHdrStruct.AddLiOctet(UMTS_RLC_LI_7BIT_PADDING);
            }
            else
            {
                if (prevPduFill)
                {
                    pduHdrStruct.AddLiDbOctet(liLen);
                }
                else
                {
                    pduHdrStruct.AddLiDbOctet(liLen - 1);
                }
                pduHdrStruct.AddLiDbOctet(UMTS_RLC_LI_15BIT_PADDING);
            }
            AddRlcHeader(node, pduHdrStruct, nextPdu);
            nextPdu->virtualPayloadSize = pduSize - nextPduRealPacketSize
                                    - pduHdrStruct.GetHdrSize();
            enquePduFunc(node, rlcEntity, nextPdu);
            lastPdu = NULL;
            MEM_free(currentMsg);
        }
        prevPduFill = FALSE;
        prevPduShort = FALSE;
    }
#endif
    //b) The PDU has free space left to be filled by next SDU. Since no
    //SDU arrives by the time, a padding LI needs to be added into the
    //RLC header
//    else
    {
        currentMsg->virtualPayloadSize -= liLen;
        unsigned int liVal;
        do { } while (pduHdrStruct.ReadNextLi(liVal));

        if (liLen == 1)
        {
            ERROR_Assert(liVal != UMTS_RLC_LI_7BIT_PREVFILL
                && liVal != UMTS_RLC_LI_7BIT_SDUSTART
                && liVal != UMTS_RLC_LI_7BIT_SDUFILL
                && liVal != UMTS_RLC_LI_7BIT_ONEPDU
                && liVal != UMTS_RLC_LI_7BIT_PADDING
                && pduSize - pduHdrStruct.GetHdrSize() - liVal >= liLen,
                errorMsg);
            pduHdrStruct.AddLiOctet(UMTS_RLC_LI_7BIT_PADDING);
        }
        else
        {
            ERROR_Assert(liVal != UMTS_RLC_LI_7BIT_PREVFILL
                && liVal != UMTS_RLC_LI_15BIT_PREVFILL
                && liVal != UMTS_RLC_LI_15BIT_SDUSHORT
                && liVal != UMTS_RLC_LI_15BIT_PREVSHORT
                && liVal != UMTS_RLC_LI_15BIT_SDUSTART
                && liVal != UMTS_RLC_LI_15BIT_SDUFILL
                && liVal != UMTS_RLC_LI_15BIT_ONEPDU
                && liVal != UMTS_RLC_LI_15BIT_PADDING
                && pduSize - pduHdrStruct.GetHdrSize() - liVal >= liLen,
                errorMsg);
            pduHdrStruct.AddLiDbOctet(UMTS_RLC_LI_15BIT_PADDING);
        }
        AddRlcHeader(node, pduHdrStruct, currentPdu);
        enquePduFunc(node, rlcEntity, currentPdu);
        lastPdu = NULL;
    }
    return TRUE;
}

// /**
// FUNCTION::           UmtsRlcUmAmFragmentEntity::Store
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Serialze the entity into a string buffer
// PARAMETERS::
// RETURN::             void
// **/
void UmtsRlcUmAmFragmentEntity::Store(Node* node,
                                      std::string& buffer)
{
    BOOL lastPduAvail = FALSE;

    if (lastPdu)
    {
        lastPduAvail = TRUE;
    }
    buffer.append((char*)&lastPduAvail, sizeof(lastPduAvail));
    if (lastPdu)
    {
        std::string childBuf;
        childBuf.reserve(2000);
        int numSegs = UmtsSerializeMessageList(node,
                                               lastPdu,
                                               childBuf);
        buffer.append((char*)&numSegs, sizeof(numSegs));
        buffer.append(childBuf);
        lastPdu = NULL;
    }
    buffer.append((char*)&prevPduFill, sizeof(prevPduFill));
    buffer.append((char*)&prevPduShort, sizeof(prevPduFill));
}

// /**
// FUNCTION::           UmtsRlcUmAmFragmentEntity::Restore
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            UnSerialze the entity from a string buffer
// PARAMETERS::
// RETURN::             void
// **/
void UmtsRlcUmAmFragmentEntity::Restore(Node* node,
                                        const char* buffer,
                                        int& index)
{
    releaseLastPdu();

    BOOL lastPduAvail;
    memcpy(&lastPduAvail,  &buffer[index], sizeof(lastPduAvail));
    index += sizeof(lastPduAvail);
    if (lastPduAvail)
    {
        int numSegs;
        memcpy(&numSegs, &buffer[index], sizeof(numSegs));
        index += sizeof(numSegs);
        lastPdu = UmtsUnSerializeMessageList(
                        node,
                        buffer,
                        index,
                        numSegs);
    }
    memcpy(&prevPduFill, &buffer[index], sizeof(prevPduFill));
    index += sizeof(prevPduFill);
    memcpy(&prevPduShort, &buffer[index], sizeof(prevPduShort));
    index += sizeof(prevPduShort);
}

// /**
// FUNCTION::           UmtsRlcUmScreenRcvdPdu
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            When receiving a new PDU, the UM receiving entity
//                      decides whether to drop the PDU or drop some buffered
//                      PDU depend on the sequence number of received PDU and
//                      the state variables
// PARAMETERS::
//      +node:          Node pointer
//      +umRxEntity:    The UM receiving entity
//      +rcvdSeqNum:    The seqence number of the PDU, from which the 
//                      sdu segments are extracted
//      +pduMsg:        The received PDU
// RETURN::             BOOL
// **/
static
BOOL UmtsRlcUmScreenRcvdPdu(
    Node* node,
    UmtsRlcUmRcvEntity* umRxEntity,
    unsigned int rcvdSeqNum,
    Message* pduMsg)
{
    if (DEBUG_FRAGASSEMB
        && ((UmtsRlcEntity*)umRxEntity->entityVar)->ueId == DEBUG_UE_ID
        && ((UmtsRlcEntity*)umRxEntity->entityVar)->rbId == DEBUG_RB_ID
        && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x2)
            || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x1)))
    {
        UmtsLayer2PrintRunTimeInfo(node, 0, UMTS_LAYER2_SUBLAYER_RLC);
        std::cout << "Node: "<< node->nodeId
                  << " RB: "
                  << (int)((UmtsRlcEntity*)umRxEntity->entityVar)->rbId
                  << " RLC UM entity: "
                  << " is screenning received PDUs, " << std::endl
                  << " rcvNext: " << umRxEntity->rcvNext << std::endl
                  << " sequence nubmer: " << rcvdSeqNum << std::endl;
    }


    BOOL ret = FALSE;
    if (umRxEntity->rcvNext == rcvdSeqNum)
    {
        UmtsRlcUmIncSN(rcvdSeqNum);
        umRxEntity->rcvNext = rcvdSeqNum;

        ret = TRUE;
    }
    //If received sequence number is larger than expected,
    //then one or more PDUs are missing, the SDU segments waiting
    //to be assembled shall be dropped.
    else if (UmtsRlcSeqNumLess(umRxEntity->rcvNext,
                               rcvdSeqNum,
                               UMTS_RLC_UM_SNUPPERBOUND/2 - 1))
    {
        std::for_each(umRxEntity->sduSegmentList->begin(),
                      umRxEntity->sduSegmentList->end(),
                      UmtsRlcFreeMessageData<UmtsRlcSduSegment>(node));
        umRxEntity->sduSegmentList->clear();

        UmtsRlcUmIncSN(rcvdSeqNum);
        umRxEntity->rcvNext = rcvdSeqNum;

        ret = TRUE;
    }
    //Or if the received sequence number is smaller than expected,
    //then the received pdu is outdated.
    return ret;
}

// /**
// FUNCTION::           UmtsRlcAmScreenRcvdPdu
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            When receiving a new PDU, the AM entity
//                      decides to drop the PDU or drop some buffered
//                      PDU depend on the seq. number of received PDU and
//                      the state variables
// PARAMETERS::
//      +node:          Node pointer
//      +amEntity:      The AM entity
//      +rcvdSeqNum:    The seqence number of the PDU, from which 
//                      the sdu segments are extracted
//      +pduMsg:        The received PDU
// RETURN::             BOOL
// **/
static
BOOL UmtsRlcAmScreenRcvdPdu(
    Node* node,
    UmtsRlcAmEntity* amEntity,
    unsigned int rcvdSeqNum,
    Message* pduMsg,
    BOOL* pduExpectedNext)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcAmScreenRcvdPdu: ";

    BOOL ret = FALSE;

    *pduExpectedNext = FALSE;
    //Most likely the PDU arrives in-sequence
    if (amEntity->rcvNextSn == rcvdSeqNum)
    {
        *pduExpectedNext = TRUE;
        std::deque<BOOL>::iterator itOldHead =
                        amEntity->pduRcptFlag->begin();
        ERROR_Assert(*itOldHead == FALSE, errorMsg);
        *itOldHead = TRUE;
        std::deque<BOOL>::iterator itNewHead =
            std::find(amEntity->pduRcptFlag->begin(),
                      amEntity->pduRcptFlag->end(),
                      FALSE);
        int seqInc = (int)(itNewHead - itOldHead); 
        amEntity->pduRcptFlag->erase(itOldHead, itNewHead);
        amEntity->pduRcptFlag->insert(amEntity->pduRcptFlag->end(),
                                      seqInc,
                                      FALSE);
        ERROR_Assert(amEntity->pduRcptFlag->size() == amEntity->rcvWnd,
                     errorMsg);

        //Update VR(S)
        amEntity->rcvNextSn += seqInc;
        amEntity->rcvNextSn %= UMTS_RLC_AM_SNUPPERBOUND;

        //If updated expected SN is larger than highest expected SN
        //Update the highest expected SN as expected SN
        if (!UmtsRlcSeqNumInWnd(amEntity->rcvHighestExpSn,
                                amEntity->rcvNextSn,
                                amEntity->rcvWnd,
                                UMTS_RLC_AM_SNUPPERBOUND))
        {
            amEntity->rcvHighestExpSn = amEntity->rcvNextSn;
        }
        else if (amEntity->rcvHighestExpSn != amEntity->rcvNextSn)
        {
            //If the updated expected next SN is still smaller than
            //the expected highest SN, raise flag to initiate
            //next status report
            if (DEBUG_STATUSPDU
                && ((UmtsRlcEntity*)amEntity->entityVar)->ueId ==DEBUG_UE_ID
                && ((UmtsRlcEntity*)amEntity->entityVar)->rbId ==DEBUG_RB_ID
                && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x1)
                    || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x2)))
            {
                UmtsLayer2PrintRunTimeInfo(
                    node,
                    0,
                    UMTS_LAYER2_SUBLAYER_RLC);
                std::cout << "Node: " << node->nodeId <<
                        " detects an missing PDU"
                    << "    rcvNext: " << amEntity->rcvNextSn
                    << "    rcvHighest: " << amEntity->rcvHighestExpSn
                    << "    rcvWnd: "  << amEntity->rcvWnd
                    << std::endl;
                std::cout << std::endl;
            }
            amEntity->statusPduFlag |= UMTS_RLC_STATUSPDU_ACK;
        }
        ret = TRUE;
    }
    //Or if PDU arrives within acceptable window
    else if (UmtsRlcSeqNumInWnd(rcvdSeqNum,
                                amEntity->rcvNextSn,
                                amEntity->rcvWnd,
                                UMTS_RLC_AM_SNUPPERBOUND))
    {
        //If this PDU has been received before, drop it here
        //If piggybacked status PDU is implemented, 
        //we need to drop this PDU
        //after the piggybacked status PDU is picked up

        //Check whether this PDU is received before
        unsigned int distToRcvNext = UmtsRlcAmSeqNumDist(
                                         amEntity->rcvNextSn,
                                         rcvdSeqNum);
        ERROR_Assert(distToRcvNext < amEntity->rcvWnd, errorMsg);
        std::deque<BOOL>::iterator itPos = amEntity->pduRcptFlag->begin();
        itPos += distToRcvNext;
        if (*itPos == TRUE)
        {
            return FALSE;
        }
        else
        {
            *itPos = TRUE;
        }

        //Check whether this PDU has the highest 
        //sequence number ever received
        if (UmtsRlcAmSeqNumDist(
                amEntity->rcvNextSn,
                amEntity->rcvHighestExpSn) <= (unsigned int)distToRcvNext)
        {
            UmtsRlcAmIncSN(rcvdSeqNum);
            amEntity->rcvHighestExpSn = rcvdSeqNum;
        }

        //Because the received PDU does not 
        //have the expected sequence number,
        //so raise flag to report missing PDUs.
        if (DEBUG_STATUSPDU
            && ((UmtsRlcEntity*)amEntity->entityVar)->ueId == DEBUG_UE_ID
            && ((UmtsRlcEntity*)amEntity->entityVar)->rbId == DEBUG_RB_ID
            && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x1)
                || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x2)))
        {
            std::cout << "Node: " << node->nodeId <<
                    " detects an missing PDU"
                << "    rcvNext: " << amEntity->rcvNextSn
                << "    rcvHighest: " << amEntity->rcvHighestExpSn
                << "    rcvWnd: "  << amEntity->rcvWnd
                << std::endl;
            std::cout << std::endl;
        }
        amEntity->statusPduFlag |= UMTS_RLC_STATUSPDU_ACK;

        ret = TRUE;
    }
    return ret;
}

// /**
// FUNCTION::           UmtsRlcFitConnectedSduSegments
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            When two SDU segment has connected sequence number,
//                      adjust the front segment according to addtional info
//                      carried by the back segment
// PARAMETERS::
//      +front:         the front segment
//      +back:          the back segment
// RETURN::             NULL
// **/
inline
void UmtsRlcFitConnectedSduSegments(
    UmtsRlcSduSegment* front,
    UmtsRlcSduSegment* back)
{
    if (back->traits & UMTS_RLC_SDUSEGMENT_START)
    {
        if (back->traits & UMTS_RLC_SDUSEGMENT_PREVSHORT)
        {
            Message* msg = front->message;
            msg->virtualPayloadSize -= 1;
            ERROR_Assert(msg->virtualPayloadSize >= 0,
                "Invalid message virtual payload size");
        }
        front->traits |= UMTS_RLC_SDUSEGMENT_END;
    }
    else if (front->traits & UMTS_RLC_SDUSEGMENT_END)
    {
        back->traits |= UMTS_RLC_SDUSEGMENT_START;
    }
}

// /**
// FUNCTION::           UmtsRlcUmInSeqAddSegmentAndReassemble
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Insert the SDU segments extracted from the 
//                      newly received PDU
//                      into the SDU segment list (receiving buffer). 
//                      After this,
//                      re-assembly shall take place if possible.
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         The interface index
//      +umRxEntity:    The UM receiving entity
//      +segFromPduList:    The segment list to be inserted
// RETURN::             NULL
// **/
static
void UmtsRlcUmInSeqAddSegmentAndReassemble(
    Node* node,
    int iface,
    UmtsRlcUmRcvEntity* umRxEntity,
    std::list<UmtsRlcSduSegment*>* segFromPduList)
{
    char errorMsg[MAX_STRING_LENGTH] =
        "UmtsRlcUmInSeqAddSegmentAndReassemble: ";

    std::list<UmtsRlcSduSegment*>* sduSegmentList
                         = umRxEntity->sduSegmentList;

    bool bufferWasEmpty = sduSegmentList->empty();
    //Record the position the new segments will be inserted after. After
    //insertion, the iterator should not be changed
    std::list<UmtsRlcSduSegment*>::iterator itInsBefore
                                = sduSegmentList->end();
    if (!bufferWasEmpty)
    {
        itInsBefore--;
    }

    sduSegmentList->insert(sduSegmentList->end(),
                           segFromPduList->begin(),
                           segFromPduList->end());

    std::list<UmtsRlcSduSegment*>::iterator itInsAfter;
    if (!bufferWasEmpty)
    {
        itInsAfter = itInsBefore;
        ++itInsAfter;
    }
    else
    {
        itInsAfter = sduSegmentList->begin();
    }

    std::list<UmtsRlcSduSegment*>::iterator itSegmentStart;
    std::list<UmtsRlcSduSegment*>::iterator itSegmentEnd;
    std::list<UmtsRlcSduSegment*>::iterator itPos;

    BOOL eraseNeeded = FALSE;
    std::list<UmtsRlcSduSegment*>::iterator itEraseStart;
    std::list<UmtsRlcSduSegment*>::iterator itEraseEnd;

    if (!bufferWasEmpty)
    {
        unsigned int seqNum = (*itInsBefore)->seqNum;
        UmtsRlcUmIncSN(seqNum);
        ERROR_Assert(seqNum == (*itInsAfter)->seqNum, errorMsg);

        //Starts from the insertion position, search any segments that can
        //be reassembled toward both forward and reverse direction
        if ((*itInsAfter)->traits & UMTS_RLC_SDUSEGMENT_START)
        {
            if ((*itInsAfter)->traits & UMTS_RLC_SDUSEGMENT_PREVSHORT)
            {
                Message* lastSduSegment = (*itInsBefore)->message;
                lastSduSegment->virtualPayloadSize -= 1;
                ERROR_Assert(lastSduSegment->virtualPayloadSize >= 0,
                                errorMsg);
            }
            (*itInsBefore)->traits |= UMTS_RLC_SDUSEGMENT_END;
            itSegmentEnd = itInsBefore;
        }
        else
        {
            itSegmentEnd = itInsAfter;
            if (!((*itSegmentEnd)->traits & UMTS_RLC_SDUSEGMENT_END))
            {
                ++itSegmentEnd;
                ERROR_Assert(itSegmentEnd == sduSegmentList->end(),
                             errorMsg);
            }
        }

        itSegmentStart = itInsBefore;
        while (itSegmentStart != sduSegmentList->begin())
        {
            if ((*itSegmentStart)->traits & UMTS_RLC_SDUSEGMENT_START)
            {
               break;
            }
            --itSegmentStart;
        }
        ERROR_Assert(itSegmentStart == sduSegmentList->begin()
            && (*itSegmentStart)->traits & UMTS_RLC_SDUSEGMENT_START,
            errorMsg);

        //If inserting new segments makes reassembling old segments possible
        if (itSegmentEnd != sduSegmentList->end())
        {
            int numSegments = (int)std::distance(
                                     itSegmentStart,
                                     itSegmentEnd) + 1;

            Message** segments = (Message**) MEM_malloc(
                            sizeof(Message*)*numSegments);

            itPos = itSegmentStart;
            int i = 0;
            while (itPos != itSegmentEnd)
            {
                *(segments+i) = MESSAGE_Duplicate(node, (*itPos)->message);
                ++i;
                ++itPos;
            }
            *(segments+i) = MESSAGE_Duplicate(node, (*itPos)->message);
            Message* assembledSdu = MESSAGE_ReassemblePacket(
                                        node,
                                        segments,
                                        numSegments,
                                        TRACE_UMTS_LAYER2);

            UmtsRlcSubmitSduToUpperLayer(
                node,
                iface,
                umRxEntity->entityVar,
                assembledSdu);
            MEM_free(segments);

            //Record positions for segs. needed to be erased from the list
            eraseNeeded = TRUE;
            itEraseStart = itSegmentStart;
            itEraseEnd = itSegmentEnd;
            ++itEraseEnd;

            //Search further old segments for possible 
            //reassembly is not needed
            //so we move the searching iterator forward to 
            //search newly added segments
            itSegmentStart = itSegmentEnd;
            ++itSegmentStart;
        }
        //If reassembly is not possible, 
        //move reassembly starting position to the
        //end of the segment list
        else
        {
            itSegmentStart = itSegmentEnd;
        }
    }
    else
    {
        //If the first new segment does not start a segment when
        //there is no old segment in the buffer. It's not possible to
        //reassemble it into an SDU, so throw it away.
        itSegmentStart = itInsAfter;
        if (!((*itSegmentStart)->traits & UMTS_RLC_SDUSEGMENT_START))
        {
            eraseNeeded = TRUE;
            itEraseStart = itSegmentStart;
            itEraseEnd = itEraseStart;
            ++itEraseEnd;

            ++itSegmentStart;
        }
        if (itSegmentStart != sduSegmentList->end())
        {
            ERROR_Assert((*itSegmentStart)->traits
                            & UMTS_RLC_SDUSEGMENT_START,
                          errorMsg);
        }
    }

    //Search new inserted segments for those available for reassembly
    while (itSegmentStart != sduSegmentList->end())
    {
        itSegmentEnd = itSegmentStart;
        while (itSegmentEnd != sduSegmentList->end())
        {
            if ((*itSegmentEnd)->traits & UMTS_RLC_SDUSEGMENT_END)
            {
                break;
            }
            ++itSegmentEnd;
        }

        if (itSegmentEnd != sduSegmentList->end())
        {
            //All new segments belong to one PDU
            ERROR_Assert(itSegmentEnd == itSegmentStart, errorMsg);

            int numSegments = (int)std::distance(
                                     itSegmentStart,
                                     itSegmentEnd) + 1;

            Message** segments = (Message**) MEM_malloc(
                            sizeof(Message*)*numSegments);

            itPos = itSegmentStart;
            int i = 0;
            while (itPos != itSegmentEnd)
            {
                *(segments+i) = (*itPos)->message;
                (*itPos)->message = NULL;       
                              //Make it NULL so it won't be freed twice
                ++i;
                ++itPos;
            }

            *(segments+i) = (*itPos)->message;
            (*itPos)->message = NULL;       
                             //Make it NULL so it won't be freed twice
            Message* assembledSdu = MESSAGE_ReassemblePacket(
                                        node,
                                        segments,
                                        numSegments,
                                        TRACE_UMTS_LAYER2);
            MEM_free(segments);

            UmtsRlcSubmitSduToUpperLayer(
                node,
                iface,
                umRxEntity->entityVar,
                assembledSdu);

            if (eraseNeeded == FALSE)
            {
                eraseNeeded = TRUE;
                itEraseStart = itSegmentStart;
            }
            itEraseEnd = itSegmentEnd;
            ++itEraseEnd;

            itSegmentStart = itSegmentEnd;
        }
        else
        {
            break;
        }
        ++itSegmentStart;
    }

    //Erase segments reassembled or dropped from the buffer list
    //For Out-of-sequence delivery in UM or AM, we may not remove those
    //segments from list imediately after they are reassemblied, because
    //some of them may indicate that their previous SDU segment ends in
    //the PDU without LI indicating that.
    if (eraseNeeded)
    {
        std::for_each(itEraseStart,
                      itEraseEnd,
                      UmtsRlcFreeMessageData<UmtsRlcSduSegment>(node));
        sduSegmentList->erase(itEraseStart, itEraseEnd);
    }
}

// /**
// FUNCTION::           UmtsRlcAmAddSegmentAndReassemble
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Insert the SDU segments extracted from 
//                      the newly received PDU
//                      into the SDU segment list (receiving buffer). 
//                      After this,
//                      re-assembly shall take place if possible.
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         The interface index
//      +umRxEntity:    The UM receiving entity
//      +segFromPduList:    The segment list to be inserted
// RETURN::             NULL
// **/
static
void UmtsRlcAmAddSegmentAndReassemble(
    Node* node,
    int iface,
    UmtsRlcAmEntity* amEntity,
    std::list<UmtsRlcSduSegment*>* segFromPduList,
    BOOL pduExpectedNext)
{
    char errorMsg[MAX_STRING_LENGTH]
                = "UmtsRlcAmAddSegmentAndReassemble ";
    ERROR_Assert(amEntity, errorMsg);
    std::list<UmtsRlcSduSegment*>* sduSegmentList
                                    = amEntity->sduSegmentList;
    ERROR_Assert(sduSegmentList, errorMsg);

    unsigned int rcvSeqNum = segFromPduList->front()->seqNum;
    std::list<UmtsRlcSduSegment*>::iterator itPos;
    std::list<UmtsRlcSduSegment*>::iterator itInsertAfter
                                        = sduSegmentList->end();

    if (sduSegmentList->empty())
    {
        //If the reassembly queue is empty and the 
        //received PDU is an in-sequence
        //PDU, it should starts an SDU
        if ((segFromPduList->front()->seqNum + 1)
                % UMTS_RLC_AM_SNUPPERBOUND == amEntity->rcvNextSn)
        {
            segFromPduList->front()->traits
                |= UMTS_RLC_SDUSEGMENT_START;
        }
        sduSegmentList->insert(sduSegmentList->end(),
                               segFromPduList->begin(),
                               segFromPduList->end());
    }
    else
    {
        for (itPos = sduSegmentList->begin();
            itPos != sduSegmentList->end();
            ++itPos)
        {
            //If the arriving segment's sequence number is no less than
            //rcvNextSn, skip those segments with sequence number less
            //than rcvNextSn.
            if (UmtsRlcSeqNumInWnd(rcvSeqNum,
                                   amEntity->rcvNextSn,
                                   amEntity->rcvWnd,
                                   UMTS_RLC_AM_SNUPPERBOUND))
            {
                if (UmtsRlcSeqNumLess((*itPos)->seqNum,
                                      amEntity->rcvNextSn,
                                      amEntity->rcvWnd))
                {
                    continue;
                }
            }
            //Assume duplicate received PDU has been dropped
            if ((*itPos)->seqNum == rcvSeqNum)
            {
                ERROR_ReportError("Sceening procedure fails to "
                    "drop duplicately received PDU.");

                std::for_each(
                    segFromPduList->begin(),
                    segFromPduList->end(),
                    UmtsRlcFreeMessageData<UmtsRlcSduSegment>(node));
                segFromPduList->clear();
            }
            //find a position to insert the new segment list.
            if (UmtsRlcSeqNumLess(rcvSeqNum,
                                  (*itPos)->seqNum,
                                  amEntity->rcvWnd))
            {
                if ((rcvSeqNum + 1) % UMTS_RLC_AM_SNUPPERBOUND
                             == (*itPos)->seqNum)
                {
                    UmtsRlcFitConnectedSduSegments(
                        segFromPduList->back(),
                        *itPos);
                }

                itInsertAfter = itPos;
                if (itPos != sduSegmentList->begin())
                {
                    --itPos;
                    if (rcvSeqNum == ((*itPos)->seqNum+1)
                                        %UMTS_RLC_AM_SNUPPERBOUND)
                    {
                        UmtsRlcFitConnectedSduSegments(
                            *itPos,
                            segFromPduList->front());
                    }
                }
                //If the received PDU should be inserted 
                //in the front of reassembly
                //queue and its sequence number is the 
                //expected sequence number
                //it starts an SDU
                else
                {
                    if (pduExpectedNext)
                    {
                        segFromPduList->front()->traits
                            |= UMTS_RLC_SDUSEGMENT_START;
                    }
                }
                break;
            }
        }
        //If new segment list should be inserted at the end
        if (itInsertAfter == sduSegmentList->end())
        {
            itPos = sduSegmentList->end();
            --itPos;
            if (rcvSeqNum == ((*itPos)->seqNum+1)%UMTS_RLC_AM_SNUPPERBOUND)
            {
                UmtsRlcFitConnectedSduSegments(
                    *itPos,
                    segFromPduList->front());
            }
        }

        sduSegmentList->insert(itInsertAfter,
                               segFromPduList->begin(),
                               segFromPduList->end());
    }

    //assembly starts here
    BOOL eraseNeeded = FALSE;
    std::list<UmtsRlcSduSegment*>::iterator itEraseStart;
    std::list<UmtsRlcSduSegment*>::iterator itEraseEnd;
    std::list<UmtsRlcSduSegment*>::iterator itSegmentStart;
    std::list<UmtsRlcSduSegment*>::iterator itSegmentEnd;
    itSegmentStart = sduSegmentList->begin();

    //If the first segment's sequence number is no less than
    //the next expected sequence number which has been updated,
    //then no assembly can be done
    if (UmtsRlcSeqNumInWnd((*itSegmentStart)->seqNum,
                           amEntity->rcvNextSn,
                           amEntity->rcvWnd,
                           UMTS_RLC_AM_SNUPPERBOUND))
    {
        return;
    }

    //To keep in-quence delivery, only start to reassembly from
    //the PDU with oldest sequence number
    if (!((*itSegmentStart)->traits & UMTS_RLC_SDUSEGMENT_START))
    {
        return;
    }

    itEraseStart = itSegmentStart;
    itEraseEnd = itEraseStart;

    //Start to reassembly PDUs
    while (itSegmentStart != sduSegmentList->end() &&
            ((*itSegmentStart)->traits & UMTS_RLC_SDUSEGMENT_START) &&
            UmtsRlcSeqNumLess((*itSegmentStart)->seqNum,
                              amEntity->rcvNextSn,
                              amEntity->rcvWnd))
    {
        itSegmentEnd = itSegmentStart;
        bool segEnoughForReass = false;
        unsigned int prevSeqNum;
        while (1)
        {
            if ((*itSegmentEnd)->traits & UMTS_RLC_SDUSEGMENT_END)
            {
                segEnoughForReass = true;
                break;
            }
            prevSeqNum = (*itSegmentEnd)->seqNum;
            ++itSegmentEnd;
            if (itSegmentEnd == sduSegmentList->end()
                ||((prevSeqNum + 1)%UMTS_RLC_AM_SNUPPERBOUND
                    != (*itSegmentEnd)->seqNum))
            {
                break;
            }
        }

        if (segEnoughForReass == true)
        {
            int numSegments = (int)std::distance(
                                     itSegmentStart,
                                     itSegmentEnd) + 1;

            Message** segments = (Message**) MEM_malloc(
                            sizeof(Message*)*numSegments);

            itPos = itSegmentStart;
            int i = 0;
            while (itPos != itSegmentEnd)
            {
                *(segments+i) = (*itPos)->message;
                (*itPos)->message = NULL; 
                                   //Make it NULL so it won't be free twice
                ++i;
                ++itPos;
            }
            *(segments+i) = (*itPos)->message;
            (*itPos)->message = NULL; 
                                   //Make it NULL so it won't be free twice
            Message* assembledSdu = MESSAGE_ReassemblePacket(
                                        node,
                                        segments,
                                        numSegments,
                                        TRACE_UMTS_LAYER2);
            MEM_free(segments);

            UmtsRlcSubmitSduToUpperLayer(
                node,
                iface,
                amEntity->entityVar,
                assembledSdu);

            if (eraseNeeded == FALSE)
            {
                eraseNeeded = TRUE;
            }
            itEraseEnd = itSegmentEnd;

            ++itEraseEnd;
            itSegmentStart = itSegmentEnd;
        }
        else
        {
            break;
        }

        ++itSegmentStart;
        //If the next segment's sequence is not continuous with
        //that of the end of the segment just reassembled, stop
        if (itSegmentStart != sduSegmentList->end() &&
            ((*itSegmentEnd)->seqNum + 1) % UMTS_RLC_AM_SNUPPERBOUND
                != (*itSegmentStart)->seqNum
             && (*itSegmentEnd)->seqNum != (*itSegmentStart)->seqNum)
        {
            break;
        }
    }
    if (eraseNeeded)
    {
        std::for_each(itEraseStart,
                      itEraseEnd,
                      UmtsRlcFreeMessageData<UmtsRlcSduSegment>(node));
        sduSegmentList->erase(itEraseStart, itEraseEnd);
    }
}

// /**
// FUNCTION::           UmtsRlcAddSegmentAndReassemble
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Insert the SDU segments extracted 
//                      from the newly received PDU
//                      into the SDU segment list (receiving buffer).
//                      After this,
//                      re-assembly shall take place if possible.
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         The interface index
//      +rlcEntity:     The RLC entity
//      +segFromPduList:    The segment list to be inserted
// RETURN::             NULL
// **/
static
void UmtsRlcAddSegmentAndReassemble(
    Node* node,
    int iface,
    UmtsRlcEntity* rlcEntity,
    std::list<UmtsRlcSduSegment*>* segFromPduList,
    BOOL pduExpectedNext)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcAddSegmentAndReassemble: ";
    ERROR_Assert(!segFromPduList->empty(), errorMsg);

    std::list<UmtsRlcSduSegment*>::iterator it;
    for (it = segFromPduList->begin();
         it != segFromPduList->end(); ++it)
    {
        (*it)->message->next = NULL;
    }

    UmtsRlcEntityType mode = rlcEntity->type;
    if (mode == UMTS_RLC_ENTITY_UM)
    {
        UmtsRlcUmRcvEntity* umRxEntity =
            UmtsRlcFindUmRcvEntity(rlcEntity);
        UmtsRlcUmInSeqAddSegmentAndReassemble(
            node,
            iface,
            umRxEntity,
            segFromPduList);
    }
    else
    {
        UmtsRlcAmAddSegmentAndReassemble(
            node,
            iface,
            (UmtsRlcAmEntity*)rlcEntity->entityData,
            segFromPduList,
            pduExpectedNext);
    }
}

// /**
// FUNCTION::           UmtsRlcExtractSduSegmentFromPdu
// LAYER::              UMTS LAYER2 RLC
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
void UmtsRlcExtractSduSegmentFromPdu(
    Node* node,
    int iface,
    UmtsRlcEntity* rlcEntity,
    Message* pduMsg)
{
    char errorMsg[MAX_STRING_LENGTH];
    UmtsRlcEntityType mode = rlcEntity->type;

    unsigned int liLen = 0;
    unsigned int rcvdSeqNum = 0;
    BOOL alterEbit = FALSE;
    std::list<UmtsRlcSduSegment*> segFromPduList;

    if (mode == UMTS_RLC_ENTITY_UM)
    {
        UmtsRlcUmRcvEntity* umRxEntity =
            UmtsRlcFindUmRcvEntity(rlcEntity);
        ERROR_Assert(umRxEntity, errorMsg);
        liLen = umRxEntity->liOctetsLen;
        alterEbit = umRxEntity->alterEbit;
        rcvdSeqNum = UmtsRlcUmGetSeqNum(pduMsg);
    }
    else if (mode == UMTS_RLC_ENTITY_AM)
    {
        UmtsRlcAmEntity* amEntity =
            (UmtsRlcAmEntity*)rlcEntity->entityData;
        ERROR_Assert(amEntity, errorMsg);
        liLen = amEntity->rcvLiOctetsLen;
        rcvdSeqNum = UmtsRlcAmGetSeqNum(pduMsg);
    }
    else
    {
        ERROR_ReportError(errorMsg);
    }

    if (DEBUG_FRAGASSEMB
        && rlcEntity->ueId == DEBUG_UE_ID
        && rlcEntity->rbId == DEBUG_RB_ID
        && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x2)
            || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x1)))
    {
        UmtsLayer2PrintRunTimeInfo(node, 0, UMTS_LAYER2_SUBLAYER_RLC);
        std::cout << "Node: "<< node->nodeId
                  << " RB: " << (int)rlcEntity->rbId << " RLC entity: "
                  << "received a data PDU, " << std::endl
                  << "sequence number: " << rcvdSeqNum << std::endl
                  /*<< "receiving PDU size: " << pduPacketSize << std::endl
                  << "    PDU header: "<< std::endl;
        std::cout << pduHdrStruct*/;
        //UmtsRlcPrintPdu(pduMsg);
        printf("\n");
    }

    BOOL screenRes = TRUE;
    BOOL pduExpectedNext = FALSE;        
                          //whether this PDU is the expected next one
    if (mode == UMTS_RLC_ENTITY_UM)
    {
        screenRes = UmtsRlcUmScreenRcvdPdu(
                        node,
                        UmtsRlcFindUmRcvEntity(rlcEntity),
                        rcvdSeqNum,
                        pduMsg);
    }
    else
    {
        screenRes = UmtsRlcAmScreenRcvdPdu(
                        node,
                        (UmtsRlcAmEntity*)rlcEntity->entityData,
                        rcvdSeqNum,
                        pduMsg,
                        &pduExpectedNext);
    }
    if (screenRes == FALSE)
    {
        Message* msg = pduMsg;
        while (msg)
        {
            Message* nextMsg = msg->next;
            MESSAGE_Free(node, msg);
            msg = nextMsg;
        }
        return;
    }

    UmtsRlcHeader pduHdrStruct(mode, liLen);
    pduHdrStruct.BuildHdrStruct(MESSAGE_ReturnPacket(pduMsg));

    Message* currentMsg = pduMsg;
    unsigned int pduRealSize = 0;
    unsigned int pduVirtualSize = 0;
    while (currentMsg)
    {
        pduRealSize += MESSAGE_ReturnActualPacketSize(currentMsg);
        pduVirtualSize += MESSAGE_ReturnVirtualPacketSize(currentMsg);
        currentMsg = currentMsg->next;
    }
    unsigned int pduPacketSize = pduRealSize + pduVirtualSize;

    MESSAGE_RemoveHeader(node,
                         pduMsg,
                         pduHdrStruct.GetHdrSize(),
                         TRACE_UMTS_LAYER2);
    pduPacketSize -= pduHdrStruct.GetHdrSize();

    currentMsg = pduMsg;
    UmtsRlcSduSegment* currentSegment = NULL;
    //If this PDU does not contain any LI
    if (!pduHdrStruct.nextLiExist())
    {
        //If alternative Ebit configuration is configured for the
        //receiving UM entity, then this is an complete SDU
        if (alterEbit)
        {
            ERROR_Assert(!pduMsg->next, errorMsg);

            currentSegment = (UmtsRlcSduSegment*)
                MEM_malloc(sizeof(UmtsRlcSduSegment));
            memset(currentSegment, 0, sizeof(UmtsRlcSduSegment));
            currentSegment->seqNum = rcvdSeqNum;
            currentSegment->traits = UMTS_RLC_SDUSEGMENT_START
                                     |UMTS_RLC_SDUSEGMENT_END;
            currentSegment->message = currentMsg;
            segFromPduList.push_back(currentSegment);

            UmtsRlcAddSegmentAndReassemble(
                node,
                iface,
                rlcEntity,
                &segFromPduList,
                pduExpectedNext);
            return;
        }
        //Otherwise, neither the first octet or the last octet of an SDU
        //is in the current PDU, or the end of an SDU fills into the PDU
        //without length indication
        else
        {
            ERROR_Assert(!pduMsg->next, errorMsg);

            currentSegment = (UmtsRlcSduSegment*)
                MEM_malloc(sizeof(UmtsRlcSduSegment));
            memset(currentSegment, 0, sizeof(UmtsRlcSduSegment));
            currentSegment->seqNum = rcvdSeqNum;
            currentSegment->message = currentMsg;

            UmtsRlcSduSegInfo* info = (UmtsRlcSduSegInfo*)
                                      MESSAGE_ReturnInfo(
                                          currentMsg,
                                          INFO_TYPE_UmtsRlcSduSegInfo);
            if (info->traits & UMTS_RLC_SDUSEGMENT_END)
            {
                currentSegment->traits |= UMTS_RLC_SDUSEGMENT_END;
                if (info->traits & UMTS_RLC_SDUSEGMENT_PREVSHORT)
                {
                    currentMsg->virtualPayloadSize -= 1;
                    ERROR_Assert(currentMsg->virtualPayloadSize >= 0,
                        "Invalid message virtual payload size");
                }
            }
            segFromPduList.push_back(currentSegment);

            UmtsRlcAddSegmentAndReassemble(
                node,
                iface,
                rlcEntity,
                &segFromPduList,
                pduExpectedNext);
            return;
        }
    }

    BOOL firstLi = TRUE;
    unsigned int liVal;
    while (pduHdrStruct.ReadNextLi(liVal))
    {
        if ((liLen == 1 && liVal == UMTS_RLC_LI_7BIT_PREVFILL)
            || (liLen == 2 && liVal == UMTS_RLC_LI_15BIT_PREVFILL))
        {
            if (!firstLi)
            {
                ERROR_ReportError(errorMsg);
            }
            ERROR_Assert(currentMsg == pduMsg, errorMsg);

            currentSegment = (UmtsRlcSduSegment*)
                MEM_malloc(sizeof(UmtsRlcSduSegment));
            memset(currentSegment, 0, sizeof(UmtsRlcSduSegment));
            currentSegment->seqNum = rcvdSeqNum;
            currentSegment->message = currentMsg;
            currentSegment->traits = UMTS_RLC_SDUSEGMENT_START;
        }
        else if ((liLen == 1 && liVal == UMTS_RLC_LI_7BIT_SDUSTART)
            || (liLen == 2 && liVal == UMTS_RLC_LI_15BIT_SDUSTART))
        {
            //If this is UM
            if (firstLi && mode == UMTS_RLC_ENTITY_UM)
            {
                ERROR_Assert(currentMsg == pduMsg, errorMsg);

                currentSegment = (UmtsRlcSduSegment*)
                    MEM_malloc(sizeof(UmtsRlcSduSegment));
                memset(currentSegment, 0, sizeof(UmtsRlcSduSegment));
                currentSegment->seqNum = rcvdSeqNum;
                currentSegment->message = currentMsg;
                currentSegment->traits = UMTS_RLC_SDUSEGMENT_START;
            }
            //Otherwise, drop the PDU
            else
            {
                ERROR_ReportError(errorMsg);
            }
        }
        else if ((liLen == 1 && liVal == UMTS_RLC_LI_7BIT_SDUFILL)
            || (liLen == 2 && liVal == UMTS_RLC_LI_15BIT_SDUFILL))
        {
            if (firstLi && mode == UMTS_RLC_ENTITY_UM && alterEbit)
            {
                ERROR_Assert(currentMsg == pduMsg, errorMsg);
                ERROR_Assert(!currentMsg->next, errorMsg);
                ERROR_Assert(!pduHdrStruct.nextLiExist(), errorMsg);

                currentSegment = (UmtsRlcSduSegment*)
                    MEM_malloc(sizeof(UmtsRlcSduSegment));
                memset(currentSegment, 0, sizeof(UmtsRlcSduSegment));
                currentSegment->seqNum = rcvdSeqNum;
                currentSegment->traits = UMTS_RLC_SDUSEGMENT_START
                                        |UMTS_RLC_SDUSEGMENT_END;
                currentSegment->message = currentMsg;
                segFromPduList.push_back(currentSegment);

                UmtsRlcAddSegmentAndReassemble(
                    node,
                    iface,
                    rlcEntity,
                    &segFromPduList,
                    pduExpectedNext);
                return;
            }
            else
            {
                ERROR_ReportError(errorMsg);
            }
        }
        else if ((liLen == 1 && liVal == UMTS_RLC_LI_7BIT_ONEPDU)
            || (liLen == 2 && liVal == UMTS_RLC_LI_15BIT_ONEPDU))
        {
            if (firstLi && mode == UMTS_RLC_ENTITY_UM && alterEbit)
            {
                ERROR_Assert(firstLi, errorMsg);
                ERROR_Assert(currentMsg == pduMsg, errorMsg);
                ERROR_Assert(!currentMsg->next, errorMsg);
                ERROR_Assert(!pduHdrStruct.nextLiExist(), errorMsg);

                currentSegment = (UmtsRlcSduSegment*)
                    MEM_malloc(sizeof(UmtsRlcSduSegment));
                memset(currentSegment, 0, sizeof(UmtsRlcSduSegment));
                currentSegment->seqNum = rcvdSeqNum;
                currentSegment->message = currentMsg;
                segFromPduList.push_back(currentSegment);

                UmtsRlcAddSegmentAndReassemble(
                    node,
                    iface,
                    rlcEntity,
                    &segFromPduList,
                    pduExpectedNext);
                return;
            }
            else
            {
                ERROR_ReportError(errorMsg);
            }
        }
        else if ((liLen == 1 && liVal == UMTS_RLC_LI_7BIT_PADDING)
            || (liLen == 2 && liVal == UMTS_RLC_LI_15BIT_PADDING))
        {
            ERROR_ReportError(errorMsg);
        }
        else if (liLen == 2 && liVal == UMTS_RLC_LI_15BIT_PREVSHORT)
        {
            if (!firstLi)
            {
                ERROR_ReportError(errorMsg);
            }
            ERROR_Assert(currentMsg == pduMsg, errorMsg);

            currentSegment = (UmtsRlcSduSegment*)
                MEM_malloc(sizeof(UmtsRlcSduSegment));
            memset(currentSegment, 0, sizeof(UmtsRlcSduSegment));
            currentSegment->seqNum = rcvdSeqNum;
            currentSegment->message = currentMsg;
            currentSegment->traits = UMTS_RLC_SDUSEGMENT_PREVSHORT
                                    |UMTS_RLC_SDUSEGMENT_START;
        }
        else if (liLen == 2 && liVal == UMTS_RLC_LI_15BIT_SDUSHORT)
        {
            if (firstLi && mode == UMTS_RLC_ENTITY_UM && alterEbit)
            {
                ERROR_Assert(currentMsg == pduMsg, errorMsg);
                ERROR_Assert(!currentMsg->next, errorMsg);
                ERROR_Assert(!pduHdrStruct.nextLiExist(), errorMsg);

                currentSegment = (UmtsRlcSduSegment*)
                    MEM_malloc(sizeof(UmtsRlcSduSegment));
                memset(currentSegment, 0, sizeof(UmtsRlcSduSegment));
                currentSegment->seqNum = rcvdSeqNum;
                currentSegment->traits = UMTS_RLC_SDUSEGMENT_START
                                        |UMTS_RLC_SDUSEGMENT_END;
                currentSegment->message = currentMsg;
                currentSegment->message->virtualPayloadSize -= 1;
                ERROR_Assert(currentSegment->message->virtualPayloadSize>=0,
                                errorMsg);
                segFromPduList.push_back(currentSegment);

                UmtsRlcAddSegmentAndReassemble(
                    node,
                    iface,
                    rlcEntity,
                    &segFromPduList,
                    pduExpectedNext);
                return;
            }
            else
            {
                ERROR_ReportError(errorMsg);
            }
        }
        //If a normal LI presents, it indicates an SDU ending in the
        //current PDU
        else
        {
            if (liVal > pduPacketSize)
            {
                ERROR_ReportError(errorMsg);
            }

            if (!currentSegment)
            {
                currentSegment = (UmtsRlcSduSegment*)
                    MEM_malloc(sizeof(UmtsRlcSduSegment));
                memset(currentSegment, 0, sizeof(UmtsRlcSduSegment));
                currentSegment->seqNum = rcvdSeqNum;
                currentSegment->message = currentMsg;
            }

            currentSegment->traits |= UMTS_RLC_SDUSEGMENT_END;
            if (currentMsg != pduMsg)
            {
                currentSegment->traits |= UMTS_RLC_SDUSEGMENT_START;
            }

            //If the PDU is exactly or 1-octet short-filled, no other
            //LI except PADDING may present after this LI.
            if (liVal == pduPacketSize ||
                (liVal == pduPacketSize - 1 && liLen == 2))
            {
                ERROR_Assert(!pduHdrStruct.PeekNextLi(liVal)
                    || liVal == UMTS_RLC_LI_7BIT_PADDING
                    || liVal == UMTS_RLC_LI_15BIT_PADDING, errorMsg);
                ERROR_Assert(!currentMsg->next, errorMsg);
                if (liVal == pduPacketSize - 1)
                {
                    currentSegment->message->virtualPayloadSize -= 1;
                    ERROR_Assert(
                        currentSegment->message->virtualPayloadSize>= 0,
                        errorMsg);
                }
                segFromPduList.push_back(currentSegment);

                UmtsRlcAddSegmentAndReassemble(
                    node,
                    iface,
                    rlcEntity,
                    &segFromPduList,
                    pduExpectedNext);
                return;
            }
            else
            {
                unsigned int nextLiVal;
                BOOL nextLiExist = pduHdrStruct.PeekNextLi(nextLiVal);

                //If there is no more LI in the header, just insert current
                //segment into buffer. Another segment may exist, which may
                //not end in this PDU, or exactly or 1-octet-short end
                //in this PDU without LI present in the PDU to indicate that.
                if (!nextLiExist)
                {
                    segFromPduList.push_back(currentSegment);
                }
                //If the left PDU data are padding
                else if (nextLiVal == UMTS_RLC_LI_7BIT_PADDING
                    || nextLiVal == UMTS_RLC_LI_15BIT_PADDING)
                {
                    ERROR_Assert(!currentMsg->next, errorMsg);
                    currentSegment->message->virtualPayloadSize -=
                        (pduPacketSize - liVal);
                    ERROR_Assert(currentSegment
                        ->message->virtualPayloadSize >= 0, errorMsg);
                    segFromPduList.push_back(currentSegment);

                    UmtsRlcAddSegmentAndReassemble(
                        node,
                        iface,
                        rlcEntity,
                        &segFromPduList,
                        pduExpectedNext);
                    return;
                }
                //If the left PDU data are piggybacked AM status PDU
                else if (mode == UMTS_RLC_ENTITY_AM &&
                    (nextLiVal == UMTS_RLC_LI_7BIT_ONEPDU ||
                     nextLiVal == UMTS_RLC_LI_15BIT_ONEPDU))
                {
                    //Need to copy the piggybacked status PDU

                    segFromPduList.push_back(currentSegment);

                    UmtsRlcAddSegmentAndReassemble(
                        node,
                        iface,
                        rlcEntity,
                        &segFromPduList,
                        pduExpectedNext);
                    return;
                }
                //Otherwise, there are more SDU segment in the left
                //PDU data
                else
                {
                    ERROR_Assert(currentMsg->next, errorMsg);
                    segFromPduList.push_back(currentSegment);
                }
            }
            currentSegment = NULL;
            currentMsg = currentMsg->next;
        }
        firstLi = FALSE;
    }
    //LI for the last segment may not be present if the SDU
    //is not, or 1-octet-short or exactly ending in the current
    //PDU
    if (currentMsg)
    {
        ERROR_Assert(!currentMsg->next, errorMsg);

        //If this PDU only has one SDU segment but has a special LI
        //in its header, we need to insert the segment into the buffer
        if (currentSegment)
        {
            ERROR_Assert(currentMsg == pduMsg, errorMsg);
        }
        else
        {
            ERROR_Assert(currentMsg != pduMsg, errorMsg);

            currentSegment = (UmtsRlcSduSegment*)
                MEM_malloc(sizeof(UmtsRlcSduSegment));
            memset(currentSegment, 0, sizeof(UmtsRlcSduSegment));
            currentSegment->seqNum = rcvdSeqNum;
            currentSegment->traits |= UMTS_RLC_SDUSEGMENT_START;

            UmtsRlcSduSegInfo* info = (UmtsRlcSduSegInfo*)
                                      MESSAGE_ReturnInfo(
                                          currentMsg,
                                          INFO_TYPE_UmtsRlcSduSegInfo);
            if (info->traits & UMTS_RLC_SDUSEGMENT_END)
            {
                currentSegment->traits |= UMTS_RLC_SDUSEGMENT_END;
                if (info->traits & UMTS_RLC_SDUSEGMENT_PREVSHORT)
                {
                    currentMsg->virtualPayloadSize -= 1;
                    ERROR_Assert(currentMsg->virtualPayloadSize >= 0,
                        "Invalid message virtual payload size");
                }
            }

            currentSegment->message = currentMsg;
        }
        segFromPduList.push_back(currentSegment);
        UmtsRlcAddSegmentAndReassemble(
            node,
            iface,
            rlcEntity,
            &segFromPduList,
            pduExpectedNext);
        return;
    }
}

// /**
// FUNCTION::       UmtsRlcAmInitResetTimer
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Initiate a reset timer
// PARAMETERS::
//      +node:      The node pointer
//      +iface:     The interface index
//      +amEntity:  AM entity
// RETURN::         NULL
// **/
static
void UmtsRlcAmInitResetTimer(
    Node* node,
    int iface,
    UmtsRlcAmEntity* amEntity)
{
    UmtsLayer2TimerType timerType = UMTS_RLC_TIMER_RST;
    UmtsRlcEntity* rlcEntity = amEntity->entityVar;
    Message* timerMsg = UmtsRlcInitTimer(
                            node,
                            iface,
                            timerType,
                            0,
                            NULL,
                            rlcEntity->rbId,
                            UMTS_RLC_ENTITY_TX,
                            rlcEntity->ueId);

    amEntity->rstTimerMsg = timerMsg;
    MESSAGE_Send(node, timerMsg, amEntity->rstTimerVal);
}

// /**
// FUNCTION::       UmtsRlcAmDiscardSdu
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Discard transmitted SDUs
// PARAMETERS::
//      +node:      The node pointer
//      +amEntity:  AM entity
// RETURN::         NULL
// **/
static
void UmtsRlcAmDiscardSdu(
    Node* node,
    UmtsRlcData* rlcData,
    UmtsRlcAmEntity* amEntity)
{
    Message* currentMsg;
    Message* nextMsg;

    unsigned int sdusDiscarded = 0;
    std::list<UmtsRlcAmRtxPduData*>::iterator it;

    BOOL lastPduEndSdu = FALSE;
    //Counting number of SDUs in the retransmission buffer
    for (it = amEntity->rtxPduBuf->begin();
         it != amEntity->rtxPduBuf->end();
         ++ it)
    {
        currentMsg = (*it)->message;
        nextMsg = currentMsg->next;
        while (nextMsg)
        {
            UmtsRlcSduSegInfo* info =
                (UmtsRlcSduSegInfo*)MESSAGE_ReturnInfo(
                                        currentMsg,
                                        INFO_TYPE_UmtsRlcSduSegInfo);

            if (info->traits & UMTS_RLC_SDUSEGMENT_END)
            {
                sdusDiscarded++;
                std::list<UmtsRlcAmRtxPduData*>::iterator itNow = it;
                ++itNow;
                if (itNow == amEntity->rtxPduBuf->end()
                    && currentMsg->next == NULL)
                {
                    lastPduEndSdu = TRUE;
                }
            }
            currentMsg = nextMsg;
            nextMsg = currentMsg->next;
        }
    }

    //If the last PDU of the retransmission is the last segment of an SDU,
    //just discard PDUs in the retransmission buffer
    std::for_each(amEntity->rtxPduBuf->begin(),
                  amEntity->rtxPduBuf->end(),
                  UmtsRlcFreeMessageData<UmtsRlcAmRtxPduData>(node));
    amEntity->rtxPduBuf->clear();

    //Otherwise, find PDUs that compose the last SDU in the transmission
    //buffer and remove all of them
    if (!lastPduEndSdu)
    {
        std::deque<Message*>::iterator itTxBuf;
        std::deque<Message*>::iterator itTxEraseStart
                                       = amEntity->transPduBuf->begin();
        std::deque<Message*>::iterator itTxEraseEnd
                                       = itTxEraseStart;

        for (itTxBuf = amEntity->transPduBuf->begin();
             itTxBuf != amEntity->transPduBuf->end();
             ++itTxBuf)
        {
            currentMsg = *itTxBuf;
            nextMsg = currentMsg->next;
            while (nextMsg)
            {
                UmtsRlcSduSegInfo* info =
                    (UmtsRlcSduSegInfo*)MESSAGE_ReturnInfo(
                                            currentMsg,
                                            INFO_TYPE_UmtsRlcSduSegInfo);

                if (info->traits & UMTS_RLC_SDUSEGMENT_END)
                {
                    itTxEraseEnd = itTxBuf;
                    sdusDiscarded++;
                    break;
                }
                currentMsg = nextMsg;
                nextMsg = currentMsg->next;
            }
        }

        if (itTxEraseEnd == amEntity->transPduBuf->end())
        {
            amEntity->fragmentEntity->RequestLastPdu(node);
            itTxEraseEnd = amEntity->transPduBuf->end();
        }
        std::for_each(itTxEraseStart,
                      itTxEraseEnd,
                      UmtsRlcFreeMessage(node));
        amEntity->transPduBuf->erase(itTxEraseStart, itTxEraseEnd);
    }

    rlcData->stats.numSdusDiscarded += sdusDiscarded;
}

// /**
// FUNCTION::       UmtsRlcAmResetEntity
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Reset AM entity
// PARAMETERS::
//      +node:      The node pointer
//      +rlcData:   The RLC sublayer data
//      +amEntity:  AM entity
// RETURN::         NULL
// **/
static
void UmtsRlcAmResetEntity(
    Node* node,
    UmtsRlcData* rlcData,
    UmtsRlcAmEntity* amEntity)
{
    amEntity->sndNextSn = 0;
    amEntity->ackSn = 0;
    UmtsRlcAmDiscardSdu(node, rlcData, amEntity);

    amEntity->rcvNextSn = 0;
    amEntity->rcvHighestExpSn = 0;
    std::for_each(amEntity->sduSegmentList->begin(),
                  amEntity->sduSegmentList->end(),
                  UmtsRlcFreeMessageData<UmtsRlcSduSegment>(node));
    amEntity->sduSegmentList->clear();
    amEntity->pduRcptFlag->clear();
    amEntity->pduRcptFlag->insert(amEntity->pduRcptFlag->end(),
                                  amEntity->rcvWnd,
                                  FALSE);

    amEntity->statusPduFlag = 0;
}

// /**
// FUNCTION::       UmtsRlcAmCreateResetPdu
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Create a AM reset/reset ack PDU
// PARAMETERS::
//      +node:      The node pointer
//      +amEntity:  AM entity
//      +ack:       Whether it is a reset ack
// RETURN::         NULL
// **/
static
void UmtsRlcAmCreateResetPdu(
    Node* node,
    UmtsRlcAmEntity* amEntity,
    BOOL ack)
{
    Message* msg = MESSAGE_Alloc(node,
                                 MAC_LAYER,
                                 0,
                                 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        1,
                        TRACE_UMTS_LAYER2);

    UInt8 packet = static_cast<UInt8>(*MESSAGE_ReturnPacket(msg));
    if (ack)
    {
        UmtsRlcSet4BitDataInOctet(packet,
                                  0,
                                  UMTS_RLC_CONTROLPDU_RESET);
        if (amEntity->lastRcvdRstSn)
        {
            UmtsRlcSetRsnBitInOctet(packet);
        }
        else
        {
            UmtsRlcResetRsnBitInOctet(packet);
        }
        if (amEntity->rstAckPduMsg)
        {
            MESSAGE_Free(node, amEntity->rstAckPduMsg);
        }
        amEntity->rstAckPduMsg = msg;
    }
    else
    {
        UmtsRlcSet4BitDataInOctet(packet,
                                  0,
                                  UMTS_RLC_CONTROLPDU_RESETACK);
        if (amEntity->rstSn)
        {
            UmtsRlcSetRsnBitInOctet(packet);
        }
        else
        {
            UmtsRlcResetRsnBitInOctet(packet);
        }
        if (amEntity->rstPduMsg)
        {
            MESSAGE_Free(node, amEntity->rstPduMsg);
        }
        amEntity->rstPduMsg = msg;
    }
}

// /**
// FUNCTION::       UmtsRlcAmInitResetProc
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Initiate an AM Reset procedure, invoked due to
//                  the following reasons
//                  1. Erroneous Sequence number is received in STATUS PDU
//                  2. After MAXDAT number of retransmissions 
//                     and NO_DISCARD is configured
// PARAMETERS::
//      +node:      The node pointer
//      +iface:     The interface index
//      +amEntity:  AM entity
// RETURN::         NULL
// **/
static
void UmtsRlcAmInitResetProc(
    Node* node,
    int iface,
    UmtsRlcAmEntity* amEntity)
{
    //char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcAmInitResetProc: ";

    if (amEntity->state == UMTS_RLC_ENTITYSTATE_RESETPENDING)
    {
        return;
    }

    amEntity->state = UMTS_RLC_ENTITYSTATE_RESETPENDING;
    amEntity->numRst++;
    if (amEntity->numRst == amEntity->maxRst)
    {
        UmtsRlcEntity* entity = amEntity->entityVar;
        UmtsLayer3ReportAmRlcError(node,
                                   entity->ueId,
                                   entity->rbId);
#if 0
        ERROR_ReportError("Reset procedure has been started for"
           " a maximum number of time.");
#endif
    }
    else
    {
        amEntity->rstSn++;
        amEntity->rstSn %= 2;
        UmtsRlcAmCreateResetPdu(node, amEntity, FALSE);
        UmtsRlcAmInitResetTimer(node, iface, amEntity);
    }
}

// /**
// FUNCTION::       UmtsRlcAmHandleResetTimeOut
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Handle a reset timeout event
// PARAMETERS::
//      +node:      The node pointer
//      +iface:     The interface index
//      +amEntity:  AM entity
// RETURN::         NULL
// **/
static
void UmtsRlcAmHandleResetTimeOut(
    Node* node,
    int iface,
    UmtsRlcAmEntity* amEntity)
{
    if (amEntity->state != UMTS_RLC_ENTITYSTATE_RESETPENDING)
    {
        return;
    }
    amEntity->numRst++;
    if (amEntity->numRst == amEntity->maxRst)
    {
        UmtsRlcEntity* entity = amEntity->entityVar;
        UmtsLayer3ReportAmRlcError(node,
                                   entity->ueId,
                                   entity->rbId);
#if 0
        ERROR_ReportError("Reset procedure has been started for"
           " a maximum number of time.");
#endif // 0
    }
    else
    {
        UmtsRlcAmCreateResetPdu(node, amEntity, FALSE);
        UmtsRlcAmInitResetTimer(node, iface, amEntity);
    }
}

// /**
// FUNCTION::       UmtsRlcAmHandleResetPdu
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Handle a received reset PDU
// PARAMETERS::
//      +node:      The node pointer
//      +rlcData:   The RLC sublayer data
//      +amEntity:  AM entity
//      +pduMsg:    The received reset PDU message
// RETURN::         NULL
// **/
static
void UmtsRlcAmHandleResetPdu(
    Node* node,
    UmtsRlcData* rlcData,
    UmtsRlcAmEntity* amEntity,
    Message* pduMsg)
{
    unsigned int rcvdRsn = UmtsRlcGetRsnBitFromOctet(
                                static_cast<UInt8>(
                                    *MESSAGE_ReturnPacket(pduMsg)));
    MESSAGE_Free(node, pduMsg);
    if (rcvdRsn == amEntity->lastRcvdRstSn)
    {
        UmtsRlcAmCreateResetPdu(node, amEntity, TRUE);
    }
    else
    {
        amEntity->lastRcvdRstSn = rcvdRsn;
        UmtsRlcAmCreateResetPdu(node, amEntity, TRUE);
        UmtsRlcAmResetEntity(node, rlcData, amEntity);
    }
}

// /**
// FUNCTION::       UmtsRlcAmHandleResetAckPdu
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Handle a received reset ack PDU
// PARAMETERS::
//      +node:      The node pointer
//      +rlcData:   The RLC sublayer data
//      +amEntity:  AM entity
//      +pduMsg:    The received reset ack PDU message
// RETURN::         NULL
// **/
static
void UmtsRlcAmHandleResetAckPdu(
    Node* node,
    UmtsRlcData* rlcData,
    UmtsRlcAmEntity* amEntity,
    Message* pduMsg)
{
    unsigned int rcvdAckRsn = UmtsRlcGetRsnBitFromOctet(
                                static_cast<UInt8>(
                                    *MESSAGE_ReturnPacket(pduMsg)));
    MESSAGE_Free(node, pduMsg);

    if (amEntity->state != UMTS_RLC_ENTITYSTATE_RESETPENDING ||
            amEntity->rstSn != rcvdAckRsn)
    {
        return;
    }

    ERROR_Assert(amEntity->rstTimerMsg, "UmtsRlcAmHandleResetAckPdu: "
                    "Reset timer should be active when reset ACK arrives.");
    UmtsRlcAmResetEntity(node, rlcData,amEntity);
    MESSAGE_CancelSelfMsg(node, amEntity->rstTimerMsg);
    amEntity->rstTimerMsg = NULL;
    amEntity->state = UMTS_RLC_ENTITYSTATE_READY;
}

// /**
// FUNCTION::       UmtsRlcAmPrepareListSufi
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Prepare List SUFI for status Pdu
// PARAMETERS::
//      +amEntity:  The AM entity
//      +pairList:  A list of pairs containing the start sequence number of
//                  a block of missing PDU and the length of the block
//      +ackSeqNum: The ACK sequence number returned in the ACK SUFI
// RETURN::         NULL
// **/
static
void UmtsRlcAmPrepareListSufi(
    UmtsRlcAmEntity* amEntity,
    std::vector< std::pair<unsigned int, unsigned int> >& pairList)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcAmPrepareListSufi: ";
    ERROR_Assert(pairList.empty(), errorMsg);

    unsigned int maxSearchLen = UmtsRlcAmSeqNumDist(
                                    amEntity->rcvNextSn,
                                    amEntity->rcvHighestExpSn);

    std::deque<BOOL>* pduRcptFlag = amEntity->pduRcptFlag;
    std::deque<BOOL>::iterator searchStart = pduRcptFlag->begin();
    ERROR_Assert(*searchStart == FALSE, errorMsg);
    std::deque<BOOL>::iterator searchEnd = searchStart + maxSearchLen;
    std::deque<BOOL>::iterator searchStop;
    unsigned int missingLen;

    //The total length of (SN, L) pair in a List SUFI should be no
    //greater than 0x0F
    while (searchStart != searchEnd && pairList.size() < 0x0F)
    {
        int distToRcvNext = (int)(searchStart - pduRcptFlag->begin());
        unsigned int seqNum = (amEntity->rcvNextSn + distToRcvNext)
                                % UMTS_RLC_AM_SNUPPERBOUND;

        searchStop = std::find(searchStart, searchEnd, TRUE);
        ERROR_Assert(searchEnd != searchStop, errorMsg);
        missingLen = (unsigned int)(searchStop - searchStart);
        ERROR_Assert(missingLen > 0, errorMsg);
        //Since the maximum values of Length in the List SUFI is 0x0F,
        //If we found a missing PDU list with a longer length, we have
        //to split it into multiple lists
        while (missingLen > 0x0F && pairList.size() < 0x0F)
        {
            pairList.push_back(std::make_pair(seqNum, 0x0F));
            seqNum = (seqNum + 0x0F) % UMTS_RLC_AM_SNUPPERBOUND;
            missingLen -= 0x0F;
        }
        if (pairList.size() < 0x0F)
        {
            pairList.push_back(std::make_pair(seqNum, missingLen));
            searchStart = std::find(searchStop, searchEnd, FALSE);
        }
    }
}

// /**
// FUNCTION::       UmtsRlcAmHandleAck
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Handle negative and positive acknowledgement
// PARAMETERS::
//      +node:      The node pointer
//      +amEntity:  The AM entity
//      +ackSeqNum: The ACK sequence number returned in the ACK SUFI
//      +missingPduList:  A list of pairs containing the 
//                  start sequence number of
//                  a block of missing PDU and the length of the block
// RETURN::         BOOL: TRUE if there is no errorneous 
//                        sequence number in ACK/List SUFI
// **/
static
BOOL UmtsRlcAmHandleAck(
    Node* node,
    UmtsRlcAmEntity* amEntity,
    unsigned int ackSeqNum,
    std::vector< std::pair<unsigned int, unsigned int> >& missingPduList)
{
    //char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcAmHandleAck: ";

    if (DEBUG_STATUSPDU
        && ((UmtsRlcEntity*)amEntity->entityVar)->ueId == DEBUG_UE_ID
        && ((UmtsRlcEntity*)amEntity->entityVar)->rbId == DEBUG_RB_ID
        && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x1)
            || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x2)))
    {
        std::cout << "Node: " << node->nodeId <<
                " received an ACK STATUS PDU" << std::endl;
        std::cout << "ACK sequence number: " << ackSeqNum << std::endl;
        std::cout << "Missing PDU blocks list: " << std::endl;
        for (unsigned int i = 0; i < missingPduList.size(); ++i)
        {
            std::cout << "start seqence number: "
                      << missingPduList[i].first
                      << "  block length: " << missingPduList[i].second
                      << std::endl;
        }
        std::cout << std::endl;
    }

    //Check whether ACK/List SUFI have valid sequence number.
    if (ackSeqNum != amEntity->ackSn &&
        !UmtsRlcSeqNumInWnd(
            ackSeqNum,
            (amEntity->ackSn + 1) % UMTS_RLC_AM_SNUPPERBOUND,
            amEntity->sndWnd,
            UMTS_RLC_AM_SNUPPERBOUND))
    {
        return FALSE;
    }

    for (unsigned int i = 0; i < missingPduList.size(); ++i)
    {
        unsigned int seqNum = missingPduList[i].first
                                    % UMTS_RLC_AM_SNUPPERBOUND;
        //PDU with sequence number as seqNumEnd is positively acked,
        //so it should be within in the sending window
        unsigned int seqNumEnd = (seqNum + missingPduList[i].second)
                                    % UMTS_RLC_AM_SNUPPERBOUND;
        seqNumEnd = seqNumEnd > 0 ? seqNumEnd - 1
                                  : UMTS_RLC_AM_SNUPPERBOUND -1;
        if (!UmtsRlcSeqNumInWnd(
                seqNum,
                amEntity->ackSn % UMTS_RLC_AM_SNUPPERBOUND,
                amEntity->sndWnd,
                UMTS_RLC_AM_SNUPPERBOUND)
            || !UmtsRlcSeqNumInWnd(
                    seqNumEnd,
                    amEntity->ackSn % UMTS_RLC_AM_SNUPPERBOUND,
                    amEntity->sndWnd,
                    UMTS_RLC_AM_SNUPPERBOUND))
        {
            return FALSE;
        }
    }

    //Updating VT(A)
    if (missingPduList.empty() || UmtsRlcSeqNumLess(
                                    ackSeqNum,
                                    missingPduList.front().first,
                                    amEntity->sndWnd))
    {

        amEntity->ackSn = ackSeqNum;
    }
    else
    {
        //unsigned int temp = missingPduList.front().first;
        amEntity->ackSn = missingPduList.front().first;
    }

    //Cancel poll timer if needed
    if (amEntity->pollTimerMsg)
    {
        if (amEntity->pollTimerInfo.sndNextSn == ackSeqNum ||
            UmtsRlcSeqNumLess(amEntity->pollTimerInfo.sndNextSn,
                              ackSeqNum,
                              amEntity->sndWnd))
        {
            MESSAGE_CancelSelfMsg(node, amEntity->pollTimerMsg);
            amEntity->pollTimerMsg = NULL;
        }
        else
        {
            for (unsigned int i = 0; i <missingPduList.size(); ++i)
            {
                unsigned int seqNum = missingPduList[i].first
                                        % UMTS_RLC_AM_SNUPPERBOUND;
                if (UmtsRlcSeqNumInWnd(
                        amEntity->pollTimerInfo.sndNextSn,
                        seqNum,
                        missingPduList[i].second,
                        UMTS_RLC_AM_SNUPPERBOUND))
                {
                    MESSAGE_CancelSelfMsg(node, amEntity->pollTimerMsg);
                    amEntity->pollTimerMsg = NULL;
                    break;
                }
            }
        }
    }

    std::list<UmtsRlcAmRtxPduData*>* rtxPduBuf = amEntity->rtxPduBuf;
    std::list<UmtsRlcAmRtxPduData*>::iterator it;
    std::list<UmtsRlcAmRtxPduData*>::iterator itMissingBegin;
    std::list<UmtsRlcAmRtxPduData*>::iterator itMissingEnd;

    if (!rtxPduBuf->empty())
    {
        //Remove PDUs in retransmission buffer with sequence number
        //smaller than updated VT(A)
        unsigned int seqNum = amEntity->ackSn > 0 ?
                              amEntity->ackSn-1 :UMTS_RLC_AM_SNUPPERBOUND-1;
        it = std::find_if(rtxPduBuf->begin(),
                          rtxPduBuf->end(),
                          UmtsRlcRtxPduSeqNumEqual(seqNum));
        if (it != rtxPduBuf->end())
        {
            ++it;
            std::for_each(rtxPduBuf->begin(),
                          it,
                         UmtsRlcFreeMessageData<UmtsRlcAmRtxPduData>(node));
            rtxPduBuf->erase(rtxPduBuf->begin(), it);
        }

        //Scheduled retransmission of the reported missing PDUs
        for (unsigned int i = 0; i <missingPduList.size(); ++i)
        {
            unsigned int seqNum = missingPduList[i].first
                                    % UMTS_RLC_AM_SNUPPERBOUND;
            itMissingBegin = std::find_if(rtxPduBuf->begin(),
                                          rtxPduBuf->end(),
                                          UmtsRlcRtxPduSeqNumEqual(seqNum));
            itMissingEnd = itMissingBegin;
            std::advance(itMissingEnd, missingPduList[i].second);
            for (it = itMissingBegin; it != itMissingEnd; ++it)
            {
                (*it)->txScheduled = TRUE;
            }
            //The positively acknowledged PDUs that
            //has sequence number larger
            //than VT(A) are not deleted, 
            //since eventually they will be deleted
            //as VT(A) gets updated
        }
    }

    return TRUE;
}

// /**
// FUNCTION::       UmtsRlcAmFillStatusPduData
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Fill the STATUS PDU per each 4bit
// PARAMETERS::
//      +currentOctet: The current octet prepared for the STATUS PDU data
//      +statusPduSize: The count of PDU size (number of 4-bit)
//      +val:           The value to be set into the low/high 
//                      4-bit in the current octet
//      +statusData:    The vector containing the octets for the STATUS PDU
// RETURN::         NULL
// **/
static
void UmtsRlcAmFillStatusPduData(
    UInt8& currentOctet,
    unsigned int& statusPduSize,
    UInt8 val,
    std::vector<UInt8>& statusData)
{
    UmtsRlcSet4BitDataInOctet(
        currentOctet,
        (UInt8)(statusPduSize%2),
        val);
    ++statusPduSize;
    if (statusPduSize%2 == 0)
    {
        statusData.push_back(currentOctet);
        currentOctet = 0;
    }
}

// /**
// FUNCTION::       UmtsRlcAmReadStatusPduData
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Read 4-bit data from the status PDU data at
//                  the current seek point
// PARAMETERS::
//      +currentOct:        The address of the current seek 
//                          pointer in STATUS payload
//      +statusPduSize:     The count of PDU size (number of 4-bit)
// RETURN::         UInt8: Value of the 4-Bit data
// **/
static
UInt8 UmtsRlcAmReadStatusPduData(
    UInt8** currentOct,
    unsigned int& statusPduSize)
{
    UInt8 retVal = UmtsRlcGet4BitDataFromOctet(
                        **currentOct,
                        (UInt8)(statusPduSize%2));
    statusPduSize += 1;
    if (statusPduSize%2 == 0)
    {
        (*currentOct) += 1;
    }
    return retVal;
}

// /**
// FUNCTION::       UmtsRlcFill12BitNumberIntoStatusData
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Write a 12-bit number into the STATUS PDU
// PARAMETERS::
//      +currentOctet: The current octet prepared for the STATUS PDU data
//      +statusPduSize: The count of PDU size (number of 4-bit)
//      +val: The 12-bit-long value to be set into status PDU 
//            (usually sequence number)
//      +statusData: The vector containing the octets for the STATUS PDU
// RETURN::         NULL
// **/
static
void UmtsRlcFill12BitNumberIntoStatusData(
    UInt8& currentOctet,
    unsigned int& statusPduSize,
    unsigned int val,
    std::vector<UInt8>& statusData)
{
    UInt8 seqNumLowOctet = static_cast<UInt8>(val);
    UInt8 seqNumHighOctet = static_cast<UInt8>(val>>8);
    UmtsRlcAmFillStatusPduData(
        currentOctet,
        statusPduSize,
        seqNumHighOctet,
        statusData);
    UmtsRlcAmFillStatusPduData(
        currentOctet,
        statusPduSize,
        seqNumLowOctet>>4,
        statusData);
    UmtsRlcAmFillStatusPduData(
        currentOctet,
        statusPduSize,
        seqNumLowOctet&0x0F,
        statusData);
}

// /**
// FUNCTION::       UmtsRlcGet12BitNumberFromStatusData
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Get 12-bit number from the STATUS PDU payload
// PARAMETERS::
//      +currentOct:        The address of the current seek 
//                          pointer in STATUS payload
//      +statusPduSize:     The count of PDU size (number of 4-bit)
// RETURN::         NULL
// **/
static
unsigned int UmtsRlcGet12BitNumberFromStatusData(
    UInt8** currentOct,
    unsigned int& statusPduSize)
{
    unsigned int highHalfOct;
    unsigned int midHalfOct;
    unsigned int lowHalfOct;

    highHalfOct = static_cast<unsigned int>(
                    UmtsRlcAmReadStatusPduData(
                        currentOct,
                        statusPduSize));
    midHalfOct = static_cast<unsigned int>(
                    UmtsRlcAmReadStatusPduData(
                        currentOct,
                        statusPduSize));
    lowHalfOct = static_cast<unsigned int>(
                    UmtsRlcAmReadStatusPduData(
                        currentOct,
                        statusPduSize));

    unsigned int retVal = (highHalfOct<<8)
                          | (midHalfOct<<4)
                          | lowHalfOct;

    return retVal;
}

// /**
// FUNCTION::       UmtsRlcAmAddNoMoreSufi
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Add NOMORE SUFI into the status PDU
// PARAMETERS::
//      +currentOctet: The current octet prepared for the STATUS PDU data
//      +statusPduSize: The count of PDU size (number of 4-bit)
//      +statusData: The vector containing the octets for the STATUS PDU
// RETURN::         NULL
// **/
static
void UmtsRlcAmAddNoMoreSufi(
    UInt8& currentOctet,
    unsigned int& statusPduSize,
    std::vector<UInt8>& statusData)
{
    UmtsRlcAmFillStatusPduData(
        currentOctet,
        statusPduSize,
        UMTS_RLC_SUFITYPE_NOMORE,
        statusData);
    if (statusPduSize%2 == 1)
    {
        statusData.push_back(currentOctet);
        currentOctet = 0;
    }
}

// /**
// FUNCTION::       UmtsRlcAmAddAckSufi
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Add ACK SUFI into the status PDU
// PARAMETERS::
//      +currentOctet: The current octet prepared for the STATUS PDU data
//      +statusPduSize: The count of PDU size (number of 4-bit)
//      +statusData: The vector containing the octets for the STATUS PDU
//      +ackSeqNum:  The sequence number to be set in the ACK SUFI
// RETURN::         NULL
// **/
static
void UmtsRlcAmAddAckSufi(
    UInt8& currentOctet,
    unsigned int& statusPduSize,
    std::vector<UInt8>& statusData,
    unsigned int ackSeqNum)
{
    UmtsRlcAmFillStatusPduData(
        currentOctet,
        statusPduSize,
        UMTS_RLC_SUFITYPE_ACK,
        statusData);
    UmtsRlcFill12BitNumberIntoStatusData(
        currentOctet,
        statusPduSize,
        ackSeqNum,
        statusData);
    if (statusPduSize%2 == 1)
    {
        statusData.push_back(currentOctet);
        currentOctet = 0;
    }
}

// /**
// FUNCTION::       UmtsRlcAmAddListSufi
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Add ACK SUFI into the status PDU
// PARAMETERS::
//      +currentOctet: The current octet prepared for the STATUS PDU data
//      +statusPduSize: The count of PDU size (number of 4-bit)
//      +statusData: The vector containing the octets for the STATUS PDU
//      +pairList: The list containing missing seq. number and length pair
//      +listLen: The length of pairs to be written into the STATUS PDU
// RETURN::         NULL
// **/
static
void UmtsRlcAmAddListSufi(
    UInt8& currentOctet,
    unsigned int& statusPduSize,
    std::vector<UInt8>& statusData,
    std::vector< std::pair<unsigned int, unsigned int> >& pairList,
    unsigned int listLen)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcAmAddListSufi: ";
    ERROR_Assert(pairList.size() >= listLen, errorMsg);

    UmtsRlcAmFillStatusPduData(
        currentOctet,
        statusPduSize,
        UMTS_RLC_SUFITYPE_LIST,
        statusData);
    UmtsRlcAmFillStatusPduData(
        currentOctet,
        statusPduSize,
        static_cast<UInt8>(listLen),
        statusData);

    for (unsigned int i = 0; i < listLen; i++)
    {
        UmtsRlcFill12BitNumberIntoStatusData(
            currentOctet,
            statusPduSize,
            pairList[i].first,
            statusData);
        UmtsRlcAmFillStatusPduData(
            currentOctet,
            statusPduSize,
            static_cast<UInt8>(pairList[i].second),
            statusData);
    }
    pairList.erase(pairList.begin(), pairList.begin() + listLen);
}

// /**
// FUNCTION::       UmtsRlcAmInitStatusPdu
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Start creating STATUS PDU by adding a new head
// PARAMETERS::
//      +currentOctet: The current octet prepared for the STATUS PDU data
//      +statusPduSize: The count of PDU size (number of 4-bit)
//      +statusData: The vector containing the octets for the STATUS PDU
// RETURN::         NULL
// **/
static
void UmtsRlcAmInitStatusPdu(
    UInt8& currentOctet,
    unsigned int& statusPduSize,
    std::vector<UInt8>& statusData)
{
    statusPduSize = 0;
    currentOctet = 0;
    statusData.clear();
    UmtsRlcAmFillStatusPduData(
        currentOctet,
        statusPduSize,
        UMTS_RLC_CONTROLPDU_STATUS,
        statusData);
}

// /**
// FUNCTION::       UmtsRlcAmCreateStatusPdu
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Create a STATUS PDU message
// PARAMETERS::
//      +node:      The node pointer
//      +amEntity:  The AM entity
//      +statusData:    STATUS PDU payload
//      +statusPduList: STATUS PDU message List
// RETURN::         NULL
// **/
static
void UmtsRlcAmCreateStatusPdu(
    Node* node,
    UmtsRlcAmEntity* amEntity,
    std::vector<UInt8>& statusData,
    std::deque<Message*>& statusPduList)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcAmCreateStatusPdu: ";
    ERROR_Assert(statusData.size() <= amEntity->sndPduSize, errorMsg);
    Message* statusPdu = MESSAGE_Alloc(node,
                            MAC_LAYER,
                            0,
                            0);
    MESSAGE_PacketAlloc(
        node,
        statusPdu,
        (int)statusData.size(),
        TRACE_UMTS_LAYER2);

    char* packet = MESSAGE_ReturnPacket(statusPdu);
    std::vector<UInt8>::const_iterator it;
    int i=0;
    for (it = statusData.begin(); it != statusData.end(); ++it, ++i)
    {
        *(packet+i) = static_cast<char>(*it);
    }
    MESSAGE_AddVirtualPayload(
        node,
        statusPdu,
        (int)(amEntity->sndPduSize - statusData.size()));
    statusPduList.push_back(statusPdu);
    statusData.clear();
}

// /**
// FUNCTION::       UmtsRlcAmPrepareStatusPdu
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Prepare STATUS PDUs for the next transmission
// PARAMETERS::
//      +node:      The node pointer
//      +amEntity:  The AM entity
//      +statusPduList: STATUS PDU message List
// RETURN::         NULL
// **/
static
void UmtsRlcAmPrepareStatusPdu(
    Node* node,
    UmtsRlcAmEntity* amEntity,
    std::deque<Message*>& statusPduList)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcAmPrepareStatusPdu: ";
    ERROR_Assert(statusPduList.empty(), errorMsg);
    ERROR_Assert(amEntity->sndPduSize >= UMTS_RLC_MIN_AMPDUSIZE, errorMsg);

    std::vector<UInt8> statusData;
    statusData.reserve(amEntity->sndPduSize);
    UInt8 currentOctet;
    unsigned int statusPduSize;

    UmtsRlcAmInitStatusPdu(
        currentOctet,
        statusPduSize,
        statusData);

    if (amEntity->statusPduFlag & UMTS_RLC_STATUSPDU_MRW)
    {
        ERROR_Assert(FALSE, errorMsg);
    }
    if (amEntity->statusPduFlag & UMTS_RLC_STATUSPDU_MRWACK)
    {
        ERROR_Assert(FALSE, errorMsg);
    }
    if (amEntity->statusPduFlag & UMTS_RLC_STATUSPDU_WINDOW)
    {
        ERROR_Assert(FALSE, errorMsg);
    }
    if (amEntity->statusPduFlag & UMTS_RLC_STATUSPDU_ACK)
    {
        unsigned int availSize = amEntity->sndPduSize*2 - statusPduSize;
        //At least leave a space for NO_MORE SUFI
        ERROR_Assert(availSize >= 1, errorMsg);

        //Ack SUFI takes 2 octets
        unsigned int ackSufiSize = 4;
        //List SUFI takes one octet for type and length
        //and  2 octets for each reported missing block
        unsigned int listSufiHeadSize = 2;

        std::vector< std::pair<unsigned int, unsigned int> > pairList;
        UmtsRlcAmPrepareListSufi(amEntity, pairList);

        //If no missing PDU blocks, only need add ACK SUFI
        if (pairList.empty())
        {
            //If available space is not enough to insert an ACK SUFI,
            //add a NO_MORE SUFI at the end of this STATUS PDU.
            if (availSize < ackSufiSize)
            {
                UmtsRlcAmAddNoMoreSufi(
                    currentOctet,
                    statusPduSize,
                    statusData);

                UmtsRlcAmCreateStatusPdu(
                    node,
                    amEntity,
                    statusData,
                    statusPduList);

                UmtsRlcAmInitStatusPdu(
                    currentOctet,
                    statusPduSize,
                    statusData);
            }
            UmtsRlcAmAddAckSufi(
                currentOctet,
                statusPduSize,
                statusData,
                amEntity->rcvNextSn);
            if (DEBUG_STATUSPDU
                && ((UmtsRlcEntity*)amEntity->entityVar)->ueId ==DEBUG_UE_ID
                && ((UmtsRlcEntity*)amEntity->entityVar)->rbId ==DEBUG_RB_ID
                && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x2)
                    || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x1)))
            {
                UmtsLayer2PrintRunTimeInfo(
                    node, 0, UMTS_LAYER2_SUBLAYER_RLC);
                std::cout << "Node: " << node->nodeId <<
                        " prepares an ACK STATUS PDU"
                          << std::endl;
                std::cout << "ACK sequence number: "
                          << amEntity->rcvNextSn
                          << std::endl;
                std::cout << std::endl;
            }
            UmtsRlcAmCreateStatusPdu(
                node,
                amEntity,
                statusData,
                statusPduList);
        }
        else
        {
            //If available space is not enough to insert an ACK SUFI and
            //a list SUFI with at least one field,
            //add a NO_MORE SUFI at the end of this STATUS PDU.
            if (availSize < (ackSufiSize + listSufiHeadSize + 2))
            {
                UmtsRlcAmAddNoMoreSufi(
                    currentOctet,
                    statusPduSize,
                    statusData);

                UmtsRlcAmCreateStatusPdu(
                    node,
                    amEntity,
                    statusData,
                    statusPduList);

                UmtsRlcAmInitStatusPdu(
                    currentOctet,
                    statusPduSize,
                    statusData);

                availSize = amEntity->sndPduSize*2 - statusPduSize;
            }

            //If current STATUS PDU can hold ACK SUFI and list SUFI
            //with all missing PDU information
            if (availSize >= (ackSufiSize + listSufiHeadSize
                                          + 4*pairList.size()))
            {
                if (DEBUG_STATUSPDU
                    && ((UmtsRlcEntity*)amEntity->entityVar)->ueId
                    ==DEBUG_UE_ID
                    && ((UmtsRlcEntity*)amEntity->entityVar)->rbId
                    ==DEBUG_RB_ID
                    && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x2)
                        || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x1)))
                {
                    UmtsLayer2PrintRunTimeInfo(
                        node, 0, UMTS_LAYER2_SUBLAYER_RLC);
                    std::cout << "Node: " << node->nodeId <<
                            " prepares an ACK STATUS PDU" << std::endl;
                    std::cout << "ACK sequence number: "
                              << amEntity->rcvHighestExpSn << std::endl;
                    std::cout << "Missing PDU blocks list: " << std::endl;
                    for (unsigned int i = 0; i < pairList.size(); ++i)
                    {
                        std::cout << "start seqence number: "
                                  << pairList[i].first
                                  << "  block length: "
                                  << pairList[i].second
                                  << std::endl;
                    }
                    std::cout << std::endl;
                }
                UmtsRlcAmAddListSufi(currentOctet,
                                     statusPduSize,
                                     statusData,
                                     pairList,
                                     (int)(pairList.size()));
                ERROR_Assert(pairList.empty(), errorMsg);
                UmtsRlcAmAddAckSufi(currentOctet,
                                    statusPduSize,
                                    statusData,
                                    amEntity->rcvHighestExpSn);

                UmtsRlcAmCreateStatusPdu(
                    node,
                    amEntity,
                    statusData,
                    statusPduList);
            }
            //Otherwise, seperate the complete List
            //into multiple List SUFIs
            //and write each into each STATUS PDU
            //with a company ACK SUFI (needed
            //to end a STATUS PDU)
            else
            {
                while (!pairList.empty())
                {
                    availSize = amEntity->sndPduSize*2 - statusPduSize
                                    - (ackSufiSize + listSufiHeadSize);
                    //Calculate how many SN-Len pairs can be written
                    //into the STATUS PDU
                    unsigned int listLen = (unsigned int)
                                           (availSize/4 >= pairList.size() ?
                                            pairList.size() : availSize/4);

                    if (DEBUG_STATUSPDU
                        && ((UmtsRlcEntity*)amEntity->entityVar)->ueId ==
                        DEBUG_UE_ID
                        && ((UmtsRlcEntity*)amEntity->entityVar)->rbId ==
                        DEBUG_RB_ID
                        && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x2)
                            || (UmtsIsNodeB(node) &&
                            DEBUG_RB_DIRECT & 0x1)))
                    {
                        UmtsLayer2PrintRunTimeInfo(
                            node, 0, UMTS_LAYER2_SUBLAYER_RLC);
                        std::cout << "Node: " << node->nodeId <<
                                " prepares an ACK STATUS PDU" << std::endl;
                        std::cout << "ACK sequence number: "
                                  << amEntity->rcvNextSn << std::endl;
                        std::cout << "Missing PDU blocks list: "
                            << std::endl;
                        for (unsigned int i = 0; i < listLen; ++i)
                        {
                            std::cout << "start seqence number: "
                                      << pairList[i].first
                                      << "  block length: "
                                      << pairList[i].second
                                      << std::endl;
                        }
                        std::cout << std::endl;
                    }
                    UmtsRlcAmAddListSufi(currentOctet,
                                         statusPduSize,
                                         statusData,
                                         pairList,
                                         listLen);

                    UmtsRlcAmAddAckSufi(currentOctet,
                                        statusPduSize,
                                        statusData,
                                        amEntity->rcvNextSn);

                    UmtsRlcAmCreateStatusPdu(node,
                                             amEntity,
                                             statusData,
                                             statusPduList);

                    UmtsRlcAmInitStatusPdu(currentOctet,
                                           statusPduSize,
                                           statusData);
                }
            }
        }
    }
    else
    {
        if (statusPduSize > 1)
        {
            UmtsRlcAmAddNoMoreSufi(
                currentOctet,
                statusPduSize,
                statusData);

            UmtsRlcAmCreateStatusPdu(
                node,
                amEntity,
                statusData,
                statusPduList);
        }
    }
    amEntity->statusPduFlag = 0;
}

// /**
// FUNCTION::       UmtsRlcAmReceiveStatusPdu
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Invoked when AM entity receives a STATUS PDU
// PARAMETERS::
//      +node:      The node pointer
//      +iface:     The interface index
//      +amEntity:  AM entity
//      +pduMsg:    The received STATUS PDU message
// RETURN::         NULL
// **/
static
void UmtsRlcAmReceiveStatusPdu(
    Node* node,
    int iface,
    UmtsRlcAmEntity* amEntity,
    Message* pduMsg)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcAmReceiveStatusPdu: ";

    if (DEBUG_STATUSPDU
        && ((UmtsRlcEntity*)amEntity->entityVar)->ueId == DEBUG_UE_ID
        && ((UmtsRlcEntity*)amEntity->entityVar)->rbId == DEBUG_RB_ID
        && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x1)
            || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x2)))
    {
        UmtsLayer2PrintRunTimeInfo(node, 0, UMTS_LAYER2_SUBLAYER_RLC);
        std::cout << "Node: " << node->nodeId <<
                " received a STATUS PDU message "
                << std::hex << std::showbase
                << pduMsg
                << std::dec << std::noshowbase << std::endl;
        //UmtsPrintMessage(std::cout, pduMsg);
    }

    UInt8* statusStart = (UInt8*)(MESSAGE_ReturnPacket(pduMsg));
    UInt8* currentOct = statusStart;
    //int pduSize = MESSAGE_ReturnActualPacketSize(pduMsg);
    unsigned int statusPduSize = 1;
    UInt8 sufiType = UmtsRlcAmReadStatusPduData(
                        &currentOct,
                        statusPduSize);

    std::vector< std::pair<unsigned int, unsigned int> > missingPduList;
    int ackSeqNum = -1;

    while (sufiType != UMTS_RLC_SUFITYPE_NOMORE)
    {
        switch (sufiType)
        {
            case UMTS_RLC_SUFITYPE_MRW:
            {
                ERROR_Assert(FALSE, errorMsg);
                break;
            }
            case UMTS_RLC_SUFITYPE_MRWACK:
            {
                ERROR_Assert(FALSE, errorMsg);
                break;
            }
            case UMTS_RLC_SUFITYPE_WINDOW:
            {
                ERROR_Assert(FALSE, errorMsg);
                break;
            }
            case UMTS_RLC_SUFITYPE_LIST:
            {
                ERROR_Assert(missingPduList.empty(), errorMsg);
                UInt8 listLen = UmtsRlcAmReadStatusPduData(
                                    &currentOct,
                                    statusPduSize);
                for (int i = 0; i < listLen; ++i)
                {
                    unsigned int sn = UmtsRlcGet12BitNumberFromStatusData(
                                        &currentOct,
                                        statusPduSize);
                    unsigned int len = static_cast<unsigned int>(
                                        UmtsRlcAmReadStatusPduData(
                                            &currentOct,
                                            statusPduSize));
                    missingPduList.push_back(std::make_pair(sn, len));
                }
                sufiType = UmtsRlcAmReadStatusPduData(
                                &currentOct,
                                statusPduSize);
                break;
            }
            case UMTS_RLC_SUFITYPE_ACK:
            {
                ackSeqNum = static_cast<int>(
                                UmtsRlcGet12BitNumberFromStatusData(
                                    &currentOct,
                                    statusPduSize));
                break;
            }
            default:
            {
                ERROR_Assert(FALSE, errorMsg);
            }
        }
        if (ackSeqNum != -1)
        {
            if (!UmtsRlcAmHandleAck(node,
                                    amEntity,
                                    static_cast<unsigned int>(ackSeqNum),
                                    missingPduList))
            {
                UmtsRlcAmInitResetProc(node, iface, amEntity);
            }
            break;
        }
    }
    MESSAGE_Free(node,pduMsg);
}

// /**
// FUNCTION::           UmtsRlcAmHandleMaxDat
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Invoked when number of transmissions of a
//                      PDU arrives at maxDAT - 1. AM entity either
//                      invoke reset procedure or SDU discard procedure
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         the interface index
//      +amEntity:      The AM entity
//      +seqNum:        The sequence number of the retransmitted PDU
//      +pduList:       A List containing the pdus to be sent
// RETURN::             NULL
// **/
static
void UmtsRlcAmHandleMaxDat(
    Node* node,
    int iface,
    UmtsRlcAmEntity* amEntity,
    unsigned int seqNum,
    std::list<Message*>* pduList)
{
    UmtsRlcAmInitResetProc(node, iface, amEntity);
}

// /**
// FUNCTION::       UmtsRlcTmTransEntityInit
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Initialize a RLC TM Transmitting entity
// PARAMETERS::
//      +rlcEntity: The general RLC entity containing the TM tx entity
//      +tmTxEntity:  TM transmitting entity
// RETURN::         NULL
// **/
static
void UmtsRlcTmTransEntityInit (
    UmtsRlcEntity*  rlcEntity,
    UmtsRlcTmTransEntity* tmTxEntity)
{
    memset(tmTxEntity, 0, sizeof(UmtsRlcTmTransEntity));
    tmTxEntity->entityVar = rlcEntity;
    tmTxEntity->transSduBuf = new std::deque<Message*>;
}

// /**
// FUNCTION::       UmtsRlcTmTransEntityFinalize
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Finalize a RLC TM transitting entity
// PARAMETERS::
//      +node:      Node pointer
//      +tmTxEntity:  TM transmitting entity
// RETURN::         NULL
// **/
// /**
static
void UmtsRlcTmTransEntityFinalize(
    Node* node,
    UmtsRlcTmTransEntity* tmTxEntity)
{
    std::for_each(tmTxEntity->transSduBuf->begin(),
        tmTxEntity->transSduBuf->end(),
        UmtsRlcFreeMessage(node));

    delete tmTxEntity->transSduBuf;
}

// /**
// FUNCTION::       UmtsRlcTmRcvEntityInit
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Initialize a RLC TM receiving entity
// PARAMETERS::
//      +rlcEntity: The general RLC entity containing the TM rcv entity
//      +tmRxEntity:  TM receiving entity
// RETURN::         NULL
// **/
static
void UmtsRlcTmRcvEntityInit (
    UmtsRlcEntity* rlcEntity,
    UmtsRlcTmRcvEntity* tmRxEntity)
{
    memset(tmRxEntity, 0, sizeof(UmtsRlcTmRcvEntity));
    tmRxEntity->entityVar = rlcEntity;
}

// /**
// FUNCTION::       UmtsRlcTmRcvEntityFinalize
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Finalize a RLC TM receiving entity
// PARAMETERS::
//      +node:      Node pointer
//      +tmRxEntity:  TM receiving entity
// RETURN::         NULL
// **/
// /**
static
void UmtsRlcTmRcvEntityFinalize(
    Node* node,
    UmtsRlcTmRcvEntity* tmRxEntity)
{

}

// /**
// FUNCTION::       UmtsRlcTmEntityFinalize
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Finalize a RLC TM entity
// PARAMETERS::
//      +node:      Node pointer
//      +tmRxEntity:  TM receiving entity
// RETURN::         NULL
// **/
// /**
static
void UmtsRlcTmEntityFinalize(
    Node* node,
    UmtsRlcTmEntity* tmEntity)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(tmEntity, errorMsg);

    if (tmEntity->txEntity)
    {
        UmtsRlcTmTransEntityFinalize(node, tmEntity->txEntity);
        MEM_free(tmEntity->txEntity);
        tmEntity->txEntity = NULL;
    }

    if (tmEntity->rxEntity)
    {
        UmtsRlcTmRcvEntityFinalize(node, tmEntity->rxEntity);
        MEM_free(tmEntity->rxEntity);
        tmEntity->rxEntity = NULL;
    }
}

// /**
// FUNCTION::       UmtsRlcUmTransEntityInit
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Initialize a RLC UM Transmitting entity
// PARAMETERS::
//      +rlcEntity: The general RLC entity containing the UM tx entity
//      +umTxEntity:  UM transmitting entity
// RETURN::         NULL
// **/
static
void UmtsRlcUmTransEntityInit (
    Node* node,
    UmtsRlcEntity* rlcEntity,
    UmtsRlcUmTransEntity* umTxEntity)
{
    memset(umTxEntity, 0, sizeof(UmtsRlcUmTransEntity));
    umTxEntity->entityVar = rlcEntity;
    umTxEntity->transSduBuf = new std::deque<Message*>;
    umTxEntity->pduBuf = new std::deque<Message*>;
    umTxEntity->fragmentEntity = new UmtsRlcUmAmFragmentEntity(
                                    node,
                                    rlcEntity);
    umTxEntity->maxTransBufSize = UMTS_RLC_DEF_MAXBUFSIZE;
    umTxEntity->alterEbit = FALSE;
    umTxEntity->maxUlPduSize = UMTS_RLC_DEF_UMMAXULPDUSIZE;
    umTxEntity->liOctetsLen = 1;
}

// /**
// FUNCTION::       UmtsRlcUmTransEntityFinalize
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Finalize a RLC UM transitting entity
// PARAMETERS::
//      +node:      Node pointer
//      +umTxEntity:  UM transmitting entity
// RETURN::         NULL
// **/
// /**
static
void UmtsRlcUmTransEntityFinalize(
    Node* node,
    UmtsRlcUmTransEntity* umTxEntity)
{
    std::for_each(umTxEntity->transSduBuf->begin(),
        umTxEntity->transSduBuf->end(),
        UmtsRlcFreeMessage(node));

    std::for_each(umTxEntity->pduBuf->begin(),
        umTxEntity->pduBuf->end(),
        UmtsRlcFreeMessage(node));

    delete umTxEntity->transSduBuf;
    delete umTxEntity->pduBuf;
    delete umTxEntity->fragmentEntity;
}

// /**
// FUNCTION::       UmtsRlcUmRcvEntityInit
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Initialize a RLC UM receiving entity
// PARAMETERS::
//      +rlcEntity: The general RLC entity containing the UM rcv entity
//      +umRxEntity:  UM receiving entity
// RETURN::         NULL
// **/
static
void UmtsRlcUmRcvEntityInit (
    UmtsRlcEntity*  rlcEntity,
    UmtsRlcUmRcvEntity* umRxEntity)
{
    memset(umRxEntity, 0, sizeof(UmtsRlcUmRcvEntity));
    umRxEntity->entityVar = rlcEntity;
    umRxEntity->sduSegmentList = new std::list<UmtsRlcSduSegment*>;
    umRxEntity->alterEbit = FALSE;
    umRxEntity->maxUlPduSize = UMTS_RLC_DEF_UMMAXULPDUSIZE;
    umRxEntity->liOctetsLen = 1;
}

// /**
// FUNCTION::       UmtsRlcUmRcvEntityFinalize
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Finalize a RLC UM receiving entity
// PARAMETERS::
//      +node:      Node pointer
//      +umRxEntity:  UM receiving entity
// RETURN::         NULL
// **/
// /**
static
void UmtsRlcUmRcvEntityFinalize(
    Node* node,
    UmtsRlcUmRcvEntity* umRxEntity)
{
    std::for_each(umRxEntity->sduSegmentList->begin(),
            umRxEntity->sduSegmentList->end(),
            UmtsRlcFreeMessageData<UmtsRlcSduSegment>(node));

    delete umRxEntity->sduSegmentList;
}

// /**
// FUNCTION::       UmtsRlcUmEntityFinalize
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Finalize a RLC UM entity
// PARAMETERS::
//      +node:      Node pointer
//      +umRxEntity:  UM entity
// RETURN::         NULL
// **/
// /**
static
void UmtsRlcUmEntityFinalize(
    Node* node,
    UmtsRlcUmEntity* umEntity)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(umEntity, errorMsg);

    if (umEntity->txEntity)
    {
        UmtsRlcUmTransEntityFinalize(node, umEntity->txEntity);
        MEM_free(umEntity->txEntity);
        umEntity->txEntity = NULL;
    }

    if (umEntity->rxEntity)
    {
        UmtsRlcUmRcvEntityFinalize(node, umEntity->rxEntity);
        MEM_free(umEntity->rxEntity);
        umEntity->rxEntity = NULL;
    }
}

// /**
// FUNCTION::       UmtsRlcAmEntityInit
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Initialize an Am entity
// PARAMETERS::
//      +rlcEntity: The general RLC entity containing the AM entity
//      +amEntity:  Am entity
// RETURN::         NULL
// **/
static
void UmtsRlcAmEntityInit (
    Node* node,
    UmtsRlcEntity* rlcEntity,
    UmtsRlcAmEntity* amEntity)
{
    memset(amEntity, 0, sizeof(UmtsRlcAmEntity));
    amEntity->entityVar = rlcEntity;
    amEntity->maxTransBufSize = UMTS_RLC_DEF_MAXBUFSIZE;

    amEntity->transPduBuf = new std::deque<Message*>;
    amEntity->rtxPduBuf = new std::list<UmtsRlcAmRtxPduData*>;
    amEntity->fragmentEntity = new UmtsRlcUmAmFragmentEntity(
                                        node,
                                        rlcEntity);
    amEntity->sduSegmentList = new std::list<UmtsRlcSduSegment*>;
    amEntity->pduRcptFlag = new std::deque<BOOL>;

    amEntity->state = UMTS_RLC_ENTITYSTATE_READY;
    amEntity->sndWnd = UMTS_RLC_DEF_WINSIZE;
    amEntity->rcvWnd = UMTS_RLC_DEF_WINSIZE;
    amEntity->maxDat = UMTS_RLC_DEF_MAXDAT;
    amEntity->maxRst = UMTS_RLC_DEF_MAXRST;
    amEntity->pollTrig = UMTS_RLC_POLL_LASTPDU;
    amEntity->sndPduSize = UMTS_RLC_DEF_PDUSIZE;
    amEntity->rcvPduSize = UMTS_RLC_DEF_PDUSIZE;
    amEntity->rstTimerVal = UMTS_RLC_DEF_RSTTIMER_VAL;
    amEntity->sndLiOctetsLen = 2;
    amEntity->rcvLiOctetsLen = 2;
    amEntity->ackSn = 0;

    amEntity->pduRcptFlag->insert(amEntity->pduRcptFlag->end(),
                                  amEntity->rcvWnd,
                                  FALSE);
}

// /**
// FUNCTION::       UmtsRlcAmFinalize
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Finalize an AM entity
// PARAMETERS::
//      +node:      Node pointer
//      +amEntity:  AM entity
// RETURN::         NULL
// **/
// /**
static
void UmtsRlcAmEntityFinalize(
    Node* node,
    UmtsRlcAmEntity* amEntity)
{
    std::for_each(amEntity->transPduBuf->begin(),
        amEntity->transPduBuf->end(),
        UmtsRlcFreeMessage(node));
    std::for_each(amEntity->rtxPduBuf->begin(),
        amEntity->rtxPduBuf->end(),
        UmtsRlcFreeMessageData<UmtsRlcAmRtxPduData>(node));
    std::for_each(amEntity->sduSegmentList->begin(),
            amEntity->sduSegmentList->end(),
            UmtsRlcFreeMessageData<UmtsRlcSduSegment>(node));

    delete amEntity->transPduBuf;
    delete amEntity->rtxPduBuf;
    delete amEntity->sduSegmentList;
    delete amEntity->fragmentEntity;
    delete amEntity->pduRcptFlag;
}

// /**
// FUNCTION::       UmtsRlcCreateTmTransEntity
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Initialize a RLC TM transmitting entity
// PARAMETERS::
//      +entity:    RLC entity
// RETURN::         NULL
// **/
static
void UmtsRlcCreateTmTransEntity(
    UmtsRlcEntity* entity)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->type == UMTS_RLC_ENTITY_TM, errorMsg);

    if (entity->entityData)
    {
        ERROR_Assert(((UmtsRlcTmEntity*)(entity->entityData))
                        ->txEntity == NULL, errorMsg);
    }
    else
    {
        entity->entityData = MEM_malloc(sizeof(UmtsRlcTmEntity));
        memset(entity->entityData, 0, sizeof(UmtsRlcTmEntity));
    }

    UmtsRlcTmEntity* tmEntity = (UmtsRlcTmEntity*)entity->entityData;

    tmEntity->txEntity = (UmtsRlcTmTransEntity*)
        MEM_malloc(sizeof(UmtsRlcTmTransEntity));
    UmtsRlcTmTransEntityInit(entity, tmEntity->txEntity);
}

// /**
// FUNCTION::       UmtsRlcConfigTmTransEntity
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Config a RLC TM transmitting entity
// PARAMETERS::
//      +entity:    RLC entity
//      +tmConfig:  TM configuration parameters
// RETURN::         NULL
// **/
static
void UmtsRlcConfigTmTransEntity(
    UmtsRlcEntity* entity,
    const UmtsRlcTmConfig* tmConfig)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->type == UMTS_RLC_ENTITY_TM, errorMsg);
    ERROR_Assert(entity->entityData, errorMsg);

    UmtsRlcTmTransEntity* txTmEntity =
        ((UmtsRlcTmEntity*)(entity->entityData)) ->txEntity;
    txTmEntity->segIndi = tmConfig->segIndi;
}

// /**
// FUNCTION::       UmtsRlcCreateTmRcvEntity
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Initialize a RLC TM receiving entity
// PARAMETERS::
//      +entity:    RLC entity
// RETURN::         NULL
// **/
static
void UmtsRlcCreateTmRcvEntity(
    UmtsRlcEntity* entity)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->type == UMTS_RLC_ENTITY_TM, errorMsg);

    if (entity->entityData)
    {
        ERROR_Assert(((UmtsRlcTmEntity*)(entity->entityData))
                        ->rxEntity == NULL, errorMsg);
    }
    else
    {
        entity->entityData = MEM_malloc(sizeof(UmtsRlcTmEntity));
        memset(entity->entityData, 0, sizeof(UmtsRlcTmEntity));
    }

    UmtsRlcTmEntity* tmEntity = (UmtsRlcTmEntity*)entity->entityData;

    tmEntity->rxEntity = (UmtsRlcTmRcvEntity*)
        MEM_malloc(sizeof(UmtsRlcTmRcvEntity));
    UmtsRlcTmRcvEntityInit(entity, tmEntity->rxEntity);
}

// /**
// FUNCTION::       UmtsRlcConfigTmRcvEntity
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Config a RLC TM Receiving entity
// PARAMETERS::
//      +entity:    RLC entity
//      +tmConfig:  TM configuration parameters
// RETURN::         NULL
// **/
static
void UmtsRlcConfigTmRcvEntity(
    UmtsRlcEntity* entity,
    const UmtsRlcTmConfig* tmConfig)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->type == UMTS_RLC_ENTITY_TM, errorMsg);
    ERROR_Assert(entity->entityData, errorMsg);

    UmtsRlcTmRcvEntity* rxTmEntity = ((UmtsRlcTmEntity*)
                                (entity->entityData))->rxEntity;
    rxTmEntity->segIndi = tmConfig->segIndi;
}

// /**
// FUNCTION::       UmtsRlcCreateUmTransEntity
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Initialize a RLC UM transmitting entity
// PARAMETERS::
//      +entity:    RLC entity
// RETURN::         NULL
// **/
static
void UmtsRlcCreateUmTransEntity(
    Node* node,
    UmtsRlcEntity* entity)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->type == UMTS_RLC_ENTITY_UM, errorMsg);

    if (entity->entityData)
    {
        ERROR_Assert(((UmtsRlcUmEntity*)(entity->entityData))
                        ->txEntity == NULL, errorMsg);
    }
    else
    {
        entity->entityData = MEM_malloc(sizeof(UmtsRlcUmEntity));
        memset(entity->entityData, 0, sizeof(UmtsRlcUmEntity));
    }

    UmtsRlcUmEntity* umEntity = (UmtsRlcUmEntity*)entity->entityData;

    umEntity->txEntity = (UmtsRlcUmTransEntity*)
        MEM_malloc(sizeof(UmtsRlcUmTransEntity));
    UmtsRlcUmTransEntityInit(node, entity, umEntity->txEntity);
}

// /**
// FUNCTION::       UmtsRlcConfigUmTransEntity
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Config a RLC UM transmitting entity
// PARAMETERS::
//      +node:      Node pointer
//      +entity:    RLC entity
//      +umConfig:  UM configuration parameters
// RETURN::         NULL
// **/
static
void UmtsRlcConfigUmTransEntity(
    Node* node,
    UmtsRlcEntity* entity,
    const UmtsRlcUmConfig* umConfig)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->type == UMTS_RLC_ENTITY_UM, errorMsg);
    ERROR_Assert(entity->entityData, errorMsg);

    UmtsRlcUmTransEntity* txUmEntity = ((UmtsRlcUmEntity*)
                            (entity->entityData))->txEntity;
    ERROR_Assert(txUmEntity, errorMsg);

    txUmEntity->maxUlPduSize = umConfig->maxUlPduSize;
    txUmEntity->alterEbit = umConfig->alterEbit;

    //Uplink
    if (UmtsIsUe(node))
    {
        if (txUmEntity->maxUlPduSize <= UMTS_RLC_UM_LI_PDUTHRESH)
        {
            txUmEntity->liOctetsLen = 1;
        }
        else {
            txUmEntity->liOctetsLen = 2;
        }
    }
    else if (UmtsIsNodeB(node))
    {
        if (umConfig->dlLiLen == 7)
        {
            txUmEntity->liOctetsLen = 1;
        }
        else if (umConfig->dlLiLen == 15) {
            txUmEntity->liOctetsLen = 2;
        }
        else
        {
            ERROR_Assert(FALSE,
                "Downlink length indicator length should be 7 or 15");
        }
    }
    else
    {
        ERROR_ReportError(errorMsg);
    }
}

// /**
// FUNCTION::       UmtsRlcCreateUmRcvEntity
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Initialize a RLC UM receiving entity
// PARAMETERS::
//      +entity:    RLC entity
// RETURN::         NULL
// **/
static
void UmtsRlcCreateUmRcvEntity(UmtsRlcEntity* entity)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->type == UMTS_RLC_ENTITY_UM, errorMsg);

    if (entity->entityData)
    {
        ERROR_Assert(((UmtsRlcUmEntity*)(entity->entityData))
                        ->rxEntity == NULL, errorMsg);
    }
    else
    {
        entity->entityData = MEM_malloc(sizeof(UmtsRlcUmEntity));
        memset(entity->entityData, 0, sizeof(UmtsRlcUmEntity));
    }

    UmtsRlcUmEntity* umEntity = (UmtsRlcUmEntity*)entity->entityData;

    umEntity->rxEntity = (UmtsRlcUmRcvEntity*)
        MEM_malloc(sizeof(UmtsRlcUmRcvEntity));
    UmtsRlcUmRcvEntityInit(entity, umEntity->rxEntity);

}

// /**
// FUNCTION::       UmtsRlcConfigUmRcvEntity
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Config a RLC UM Receiving entity
// PARAMETERS::
//      +node:      Node pointer
//      +entity:    RLC entity
//      +umConfig:  UM configuration parameters
// RETURN::         NULL
// **/
static
void UmtsRlcConfigUmRcvEntity(
    Node* node,
    UmtsRlcEntity* entity,
    const UmtsRlcUmConfig* umConfig)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->type == UMTS_RLC_ENTITY_UM, errorMsg);
    ERROR_Assert(entity->entityData, errorMsg);

    UmtsRlcUmRcvEntity* rxUmEntity = 
        ((UmtsRlcUmEntity*)(entity->entityData))
                                        ->rxEntity;
    ERROR_Assert(rxUmEntity, errorMsg);

    rxUmEntity->maxUlPduSize = umConfig->maxUlPduSize;
    rxUmEntity->alterEbit = umConfig->alterEbit;

    //Uplink
    if (UmtsIsNodeB(node))
    {
        if (rxUmEntity->maxUlPduSize <= UMTS_RLC_UM_LI_PDUTHRESH)
        {
            rxUmEntity->liOctetsLen = 1;
        }
        else {
            rxUmEntity->liOctetsLen = 2;
        }
    }
    else if (UmtsIsUe(node))
    {
        if (umConfig->dlLiLen == 7)
        {
            rxUmEntity->liOctetsLen = 1;
        }
        else if (umConfig->dlLiLen == 15)
        {
            rxUmEntity->liOctetsLen = 2;
        }
        else
        {
            ERROR_Assert(FALSE,
                "Invalid configuration for downlink length indicator size");
        }
    }
    else
    {
        ERROR_ReportError(errorMsg);
    }
}

// /**
// FUNCTION::       UmtsRlcCreateAmEntity
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Initialize a RLC AM entity
// PARAMETERS::
//      +entity:    RLC entity
// RETURN::         NULL
// **/
static
void UmtsRlcCreateAmEntity(
    Node* node,
    UmtsRlcEntity* entity)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->type == UMTS_RLC_ENTITY_AM, errorMsg);
    ERROR_Assert(!entity->entityData, errorMsg);

    entity->entityData = MEM_malloc(sizeof(UmtsRlcAmEntity));
    UmtsRlcAmEntity* amEntity = (UmtsRlcAmEntity*)entity->entityData;
    UmtsRlcAmEntityInit(node, entity, amEntity);
}

// /**
// FUNCTION::       UmtsRlcConfigAmEntity
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Config a RLC AM entity
// PARAMETERS::
//      +entity:    RLC entity
//      +amConfig:  AM configuration parameters
// RETURN::         NULL
// **/
static
void UmtsRlcConfigAmEntity(
    UmtsRlcEntity* entity,
    const UmtsRlcAmConfig* amConfig)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);
    ERROR_Assert(entity->type == UMTS_RLC_ENTITY_AM, errorMsg);
    ERROR_Assert(entity->entityData, errorMsg);

    UmtsRlcAmEntity* amEntity = (UmtsRlcAmEntity*)entity->entityData;
    ERROR_Assert(amEntity, errorMsg);

    amEntity->sndPduSize = amConfig->sndPduSize;
    amEntity->rcvPduSize = amConfig->rcvPduSize;
    if (amEntity->sndPduSize <= UMTS_RLC_AM_LI_PDUTHRESH)
    {
        amEntity->sndLiOctetsLen = 1;
    }
    else
    {
        amEntity->sndLiOctetsLen = 2;
    }
    if (amEntity->rcvPduSize <= UMTS_RLC_AM_LI_PDUTHRESH)
    {
        amEntity->rcvLiOctetsLen = 1;
    }
    else
    {
        amEntity->rcvLiOctetsLen = 2;
    }
    amEntity->sndWnd = amConfig->sndWnd;
    amEntity->rcvWnd = amConfig->rcvWnd;
    amEntity->maxDat = amConfig->maxDat;
    amEntity->maxRst = amConfig->maxRst;
    amEntity->rstTimerVal = amConfig->rstTimerVal;

    amEntity->pduRcptFlag->clear();
    amEntity->pduRcptFlag->insert(amEntity->pduRcptFlag->begin(),
                                  amEntity->rcvWnd,
                                  FALSE);
}

// /**
// FUNCTION::       UmtsRlcEntityFinalize
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Finalize a RLC entity
// PARAMETERS::
//      +node:      Node pointer
//      +entity:    RLC entity
// RETURN::         NULL
// **/
// /**
static
void UmtsRlcEntityFinalize(
    Node* node,
    UmtsRlcEntity* entity)
{
    char errorMsg[MAX_STRING_LENGTH];
    ERROR_Assert(entity, errorMsg);

    switch (entity->type)
    {
        case UMTS_RLC_ENTITY_TM:
        {
            UmtsRlcTmEntityFinalize(
                node,
                (UmtsRlcTmEntity*)entity->entityData);
            MEM_free(entity->entityData);
            break;
        }
        case UMTS_RLC_ENTITY_UM:
        {
            UmtsRlcUmEntityFinalize(
                node,
                (UmtsRlcUmEntity*)entity->entityData);
            MEM_free(entity->entityData);
            break;
        }
        case UMTS_RLC_ENTITY_AM:
        {
            UmtsRlcAmEntityFinalize(
                node,
                (UmtsRlcAmEntity*)entity->entityData);
            MEM_free(entity->entityData);
            break;
        }
        default:
        {
            ERROR_ReportError(errorMsg);
        }
    }
    memset(entity, 0, sizeof(UmtsRlcEntity));
}

// /**
// FUNCTION::           UmtsRlcTmReceivePduFromMac
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Invoked when MAC submit a PDU to the entity
//                      The receiving TM entity will
//                      1. store the pdu in the receiving buffer
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         the interface index
//      +tmRxEntity:    TM RLC receiving entity
//      +MsgList:       Message list containing PDUs from MAC
//      +errorIndi:     Error indication, whether some packets are lost in the TTI
// RETURN::             NULL
// **/
static
void UmtsRlcTmReceivePduFromMac(
    Node* node,
    int iface,
    UmtsRlcTmRcvEntity* tmRxEntity,
    std::list<Message*>& msgList,
    BOOL errorIndi)
{
    UmtsRlcData* rlcData = UmtsRlcGetSubLayerData(node, iface);

    if (DEBUG_RLC_PACKET)
    {
        if (msgList.size()
            && ((UmtsRlcEntity*)(tmRxEntity->entityVar))->ueId
                    == DEBUG_UE_ID
            && ((UmtsRlcEntity*)(tmRxEntity->entityVar))->rbId
                    == DEBUG_RB_ID
            && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x2)
                || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x1)))
        {
            char timeStr[20];
            TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
            UmtsRlcEntity* entity = (UmtsRlcEntity*)(tmRxEntity->entityVar);
            printf(
                "%s, RLC Entity of Node: %u receives %u messages from MAC "
                "through RB %d \n",
                timeStr, node->nodeId, (unsigned int)msgList.size(),
                (int)entity->rbId);
        }
    }

    rlcData->stats.numPdusFromMac += (unsigned int)msgList.size();
    std::list<Message*>::iterator it;
    if (!tmRxEntity->segIndi)
    {
        UmtsRlcEntity* entity = tmRxEntity->entityVar;
        for (it = msgList.begin(); it != msgList.end(); ++it)
        {
            // update dyanmic stats
            if (node->guiOption)
            {
                if (entity->rbId < UMTS_RBINRAB_START_ID ||
                    entity->rbId >= UMTS_BCCH_RB_ID)
                {
                    rlcData->stats.numInControlByte +=
                        UmtsGetTotalPktSizeFromMsgList((*it));
                }
                else
                {
                    rlcData->stats.numInDataByte +=
                        UmtsGetTotalPktSizeFromMsgList((*it));
                }
            }
            UmtsRlcSubmitSduToUpperLayer(
                node,
                iface,
                entity,
                *it);
        }
    }
    else
    {
        if (errorIndi == FALSE)
        {
            int numFrags = (int)msgList.size();
            Message** fragList = 
                (Message**)MEM_malloc(sizeof(Message*)*numFrags);
            int i;
            for (it = msgList.begin(),  i= 0; 
                it != msgList.end(); ++it, ++i)
            {
                fragList[i] = *it;
            }
            msgList.clear();
            Message* reassembledSdu = MESSAGE_ReassemblePacket(
                                        node,
                                        fragList,
                                        numFrags,
                                        TRACE_UMTS_LAYER2);
            MEM_free(fragList);

            UmtsRlcEntity* entity = tmRxEntity->entityVar;
            UmtsRlcSubmitSduToUpperLayer(node,
                                         iface,
                                         entity,
                                         reassembledSdu);
        }
        else
        {
            std::for_each(msgList.begin(),
                          msgList.end(),
                          UmtsRlcFreeMessage(node));
        }
    }
}

// /**
// FUNCTION::       UmtsRlcTmUpperLayerHasSduToSend
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Invoked when uppler layer deliver a SDU to the
//                  transmitting entity.
// PARAMETERS::
//      +node:      Node pointer
//      +iface:     the interface index
//      +tmTxEntity: TM RLC transmitting entity
//      +sduMsg:    The SDU message
// RETURN::         NULL
// **/
static
void UmtsRlcTmUpperLayerHasSduToSend(
    Node* node,
    int iface,
    UmtsRlcTmTransEntity* tmTxEntity,
    Message* sduMsg)
{
    //UmtsRlcData* rlcData = UmtsRlcGetSubLayerData(node, iface);

    tmTxEntity->transSduBuf->push_back(sduMsg);
    tmTxEntity->bufSize += MESSAGE_ReturnPacketSize(sduMsg);
}

// /**
// FUNCTION::           UmtsRlcTmDeliverPduToMac
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Deliver RLC TMD PDUs to MAC through TM SAP,
//                      invoked when MAC request to transmit PDUs of this
//                      Entity at each TTI. The TM transmitting entity will
//                      1. Segment the first SDU into PDUs if segmentation 
//                      configured, drop the rest SDUs 
//                      in the transmission buffer
//                      2. Otherwise prepare PDUs as required (Number)
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         the interface index
//      +tmTxEntity:    TM RLC transmitting entity
//      +numPdus:       Number of PDUs MAC can receive, given by MAC
//      +pduSize:       PDU size given by MAC
//      +msgList:       The message list containing the PDUs to be sent
// RETURN::             NULL
// **/
static
void UmtsRlcTmDeliverPduToMac(
    Node* node,
    int iface,
    UmtsRlcTmTransEntity* tmTxEntity,
    int numPdus,
    int pduSize,
    std::list<Message*>& msgList)
{
    if (numPdus <= 0 || tmTxEntity->transSduBuf->empty())
        return;

    UmtsRlcData* rlcData = UmtsRlcGetSubLayerData(node, iface);

    if (tmTxEntity->segIndi)
    {
        Message* sduMsg = tmTxEntity->transSduBuf->front();
        tmTxEntity->transSduBuf->pop_front();

        Message** pduMsgs;
        int numFrags;
        MESSAGE_FragmentPacket(node,
                               sduMsg,
                               pduSize,
                               &pduMsgs,
                               &numFrags,
                               TRACE_UMTS_LAYER2);

        for (int i = 0; i < numFrags; ++i)
        {
            msgList.push_back(pduMsgs[i]);
        }
        MEM_free(pduMsgs);

        std::for_each(tmTxEntity->transSduBuf->begin(),
                      tmTxEntity->transSduBuf->end(),
                      UmtsRlcFreeMessage(node));
        tmTxEntity->transSduBuf->clear();
    }
    else
    {
        std::deque<Message*>::iterator itBegin =
                                tmTxEntity->transSduBuf->begin();
        std::deque<Message*>::iterator itEnd;
        if ((unsigned int)numPdus < tmTxEntity->transSduBuf->size())
        {
            itEnd = itBegin + numPdus;
            //drop all SDU arriving this TTI if they cannot
            //be sent to MAC layer
            rlcData->stats.numSdusDiscarded +=
                (int)(tmTxEntity->transSduBuf->size() - numPdus);
        }
        else
        {
            itEnd = tmTxEntity->transSduBuf->end();
        }

        //work around of window 64 compiliation bugs
        std::deque<Message*>::iterator iter;
        for (iter = itBegin; iter != itEnd; ++iter)
        {
            msgList.push_back(*iter);
        }
        //msgList.insert(msgList.end(), itBegin, itEnd);
        tmTxEntity->transSduBuf->erase(itBegin, itEnd);

        std::for_each(tmTxEntity->transSduBuf->begin(),
                      tmTxEntity->transSduBuf->end(),
                      UmtsRlcFreeMessage(node));
        tmTxEntity->transSduBuf->clear();
    }

    if (DEBUG_RLC_PACKET)
    {
        if (msgList.size()
            && ((UmtsRlcEntity*)(tmTxEntity->entityVar))->ueId
                    == DEBUG_UE_ID
            && ((UmtsRlcEntity*)(tmTxEntity->entityVar))->rbId
                    == DEBUG_RB_ID
            &&((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x1)
                || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x2)))
        {
            char timeStr[20];
            TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
            UmtsRlcEntity* entity = (UmtsRlcEntity*)(tmTxEntity->entityVar);
            printf("%s, RLC Entity of Node: %u deliever %u messages to MAC "
                "through RB %d \n",
                timeStr, node->nodeId, (unsigned int)msgList.size(),
                (int)entity->rbId);
        }
    }

    tmTxEntity->bufSize = 0;
    rlcData->stats.numPdusToMac += (unsigned int)msgList.size();

    // update dynamic stats
    if (node->guiOption)
    {
        std::list<Message*>::iterator msgIt;
        UmtsRlcEntity* rlcEntity =
            (UmtsRlcEntity*)(tmTxEntity->entityVar);

        for (msgIt = msgList.begin();
            msgIt != msgList.end();
            msgIt ++)
        {
            if (rlcEntity->rbId < UMTS_RBINRAB_START_ID ||
                rlcEntity->rbId >= UMTS_BCCH_RB_ID)
            {
                rlcData->stats.numOutControlByte +=
                    UmtsGetTotalPktSizeFromMsgList((*msgIt));
            }
            else
            {
                rlcData->stats.numOutDataByte +=
                    UmtsGetTotalPktSizeFromMsgList((*msgIt));
            }
        }
    }
}

// /**
// FUNCTION::           UmtsRlcUmReceivePduFromMac
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Invoked when MAC submit a PDU to the entity
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         the interface index
//      +umRxEntity:    UM receiving entity
//      +msgList:       Received PDU message list
//      +errorIndi:     Error indication, whether some packets
//                      are lost in the TTI
// RETURN::             NULL
// **/
static
void UmtsRlcUmReceivePduFromMac(
    Node* node,
    int iface,
    UmtsRlcUmRcvEntity* umRxEntity,
    std::list<Message*>& msgList,
    BOOL errorIndi)
{
    std::list<Message*>::iterator it;
    UmtsRlcData* rlcData = UmtsRlcGetSubLayerData(node, iface);
    rlcData->stats.numPdusFromMac += (unsigned int)msgList.size();

    if (DEBUG_RLC_PACKET && msgList.size()
        && ((UmtsRlcEntity*)(umRxEntity->entityVar))->ueId
                == DEBUG_UE_ID
        && ((UmtsRlcEntity*)(umRxEntity->entityVar))->rbId
                == DEBUG_RB_ID
        && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x2)
            || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x1)))
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        UmtsRlcEntity* entity = (UmtsRlcEntity*)(umRxEntity->entityVar);
        printf("%s, RLC Entity of Node: %u receives %u messages from MAC "
            "through RB %d \n",
            timeStr, node->nodeId, (unsigned int)msgList.size(),
            (int)entity->rbId);
    }

    for (it = msgList.begin(); it != msgList.end(); ++it)
    {
        if (node->guiOption)
        {
            UmtsRlcEntity* entity =
                (UmtsRlcEntity*)(umRxEntity->entityVar);
            if (entity->rbId < UMTS_RBINRAB_START_ID ||
                entity->rbId >= UMTS_BCCH_RB_ID)
            {
                rlcData->stats.numInControlByte +=
                    UmtsGetTotalPktSizeFromMsgList((*it));
            }
            else
            {
                rlcData->stats.numInDataByte +=
                    UmtsGetTotalPktSizeFromMsgList((*it));
            }
        }
        UmtsRlcExtractSduSegmentFromPdu(
            node,
            iface,
            umRxEntity->entityVar,
            *it);
    }
}

// /**
// FUNCTION::       UmtsRlcUmUpperLayerHasSduToSend
// LAYER::          UMTS LAYER2 RLC
// PURPOSE::        Invoked when uppler layer deliver a SDU to the
//                  UM entity.
// PARAMETERS::
//      +node:      Node pointer
//      +iface:     the interface index
//      +umTxEntity:the UM transmitting entity
//      +sduMsg:    The SDU message from the upper layer
// RETURN::         NULL
// **/
static
void UmtsRlcUmUpperLayerHasSduToSend(
    Node* node,
    int iface,
    UmtsRlcUmTransEntity* umTxEntity,
    Message* sduMsg)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcUmUpperLayerHasSduToSend: ";
    ERROR_Assert(umTxEntity, errorMsg);

    UmtsRlcData* rlcData = UmtsRlcGetSubLayerData(node, iface);

    if (umTxEntity->bufSize + MESSAGE_ReturnPacketSize(sduMsg) >
            umTxEntity->maxTransBufSize)
    {
        rlcData->stats.numSdusDiscarded++;
        MESSAGE_Free(node, sduMsg);
        return;
    }

    umTxEntity->bufSize += MESSAGE_ReturnPacketSize(sduMsg);
    umTxEntity->transSduBuf->push_back(sduMsg);
}

// /**
// FUNCTION::           UmtsRlcUmDeliverPduToMac
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Prepare RLC PDUs sent to MAC through UM SAP,
//                      invoked for each TTI
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         the interface index
//      +umTxEntity:    The UM transmitting entity
//      +numPdus:       Number of PDUs MAC can receive, given by MAC
//      +pduSize:       PDU size given by MAC
//      +pduList:       A list containing the pdus to be sent
// RETURN::             NULL
// **/
static
void UmtsRlcUmDeliverPduToMac(
    Node* node,
    int iface,
    UmtsRlcUmTransEntity* umTxEntity,
    int numPdus,
    int pduSize,
    std::list<Message*>& pduList)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcUmDeliverPduToMac: ";
    ERROR_Assert(umTxEntity, errorMsg);
    ERROR_Assert(numPdus >= 0 && pduSize > 0, errorMsg);
    ERROR_Assert(pduList.size() == 0, errorMsg);
    ERROR_Assert(umTxEntity->pduBuf->size() == 0, errorMsg);

    UmtsRlcData* rlcData = UmtsRlcGetSubLayerData(node, iface);
    UmtsRlcUmAmFragmentEntity* fragmentEntity = umTxEntity->fragmentEntity;

    std::deque<Message*>::iterator itSduBuf
            = umTxEntity->transSduBuf->begin();
    std::deque<Message*>::iterator dequeBegin = itSduBuf;
    std::deque<Message*>::iterator dequeEnd = itSduBuf;

    BOOL continueFragment = TRUE;
    while (itSduBuf != umTxEntity->transSduBuf->end())
    {
        unsigned int sduSize = MESSAGE_ReturnPacketSize((*itSduBuf));
        continueFragment = fragmentEntity->Fragment(
                                node,
                                iface,
                                *itSduBuf,
                                numPdus,
                                pduSize);
        if (!continueFragment)
        {
            break;
        }
        umTxEntity->bufSize -= sduSize;
        ++itSduBuf;
        dequeEnd = itSduBuf;
    }
    if (!fragmentEntity->isLastPduGone())
    {
        fragmentEntity->RequestLastPdu(node);
    }
    ERROR_Assert(umTxEntity->pduBuf->size() <= 
        (unsigned int)numPdus, errorMsg);
    umTxEntity->transSduBuf->erase(dequeBegin, dequeEnd);

    //work around of window 64 compiliation bug
    std::deque<Message*>::iterator iter;
    for (iter = umTxEntity->pduBuf->begin();
            iter != umTxEntity->pduBuf->end(); ++iter)
    {
        pduList.push_back(*iter);
    }
    //pduList.insert(pduList.begin(),
    //               umTxEntity->pduBuf->begin(),
    //               umTxEntity->pduBuf->end());

    if (DEBUG_RLC_PACKET && umTxEntity->pduBuf->empty() 
        && numPdus > 0 && !umTxEntity->transSduBuf->empty())
    {
        char warningStr[250];
        sprintf(warningStr, "UmtsRlcUmDeliverPduToMac: "
            "fails to deliver packet to MAC, number of PDUs allowd: %d "
            "PDU size: %d, SDU size: %d \n",
            numPdus, pduSize, 
            MESSAGE_ReturnPacketSize(umTxEntity->transSduBuf->front()));
        ERROR_ReportWarning(warningStr);
    }
    if (DEBUG_RLC_PACKET && umTxEntity->pduBuf->size() > 0
        && ((UmtsRlcEntity*)(umTxEntity->entityVar))->ueId
                == DEBUG_UE_ID
        && ((UmtsRlcEntity*)(umTxEntity->entityVar))->rbId
                == DEBUG_RB_ID
        &&((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x1)
            || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x2)))
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        UmtsRlcEntity* entity = (UmtsRlcEntity*)(umTxEntity->entityVar);
        printf("%s, RLC Entity of Node: %u deliever %u messages to MAC "
            "through RB %d, number of PDUs allowd: %d \n",
            timeStr, node->nodeId, (unsigned int)umTxEntity->pduBuf->size(),
            (int)entity->rbId, numPdus);
    }

    umTxEntity->pduBuf->clear();
    rlcData->stats.numPdusToMac += (unsigned int)pduList.size();

    // update dynamic stats
    if (node->guiOption)
    {
        std::list<Message*>::iterator msgIt;
        UmtsRlcEntity* rlcEntity =
            (UmtsRlcEntity*)(umTxEntity->entityVar);

        for (msgIt = pduList.begin();
            msgIt != pduList.end();
            msgIt ++)
        {
            if (rlcEntity->rbId < UMTS_RBINRAB_START_ID ||
                rlcEntity->rbId >= UMTS_BCCH_RB_ID)
            {
                rlcData->stats.numOutControlByte +=
                    UmtsGetTotalPktSizeFromMsgList((*msgIt));
            }
            else
            {
                rlcData->stats.numOutDataByte +=
                    UmtsGetTotalPktSizeFromMsgList((*msgIt));
            }
        }
    }
}

// /**
// FUNCTION::           UmtsRlcAmReceivePduFromMac
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Invoked when MAC submit a PDU to the entity
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         the interface index
//      +amEntity:      TM RLC AM entity
//      +msgList:       PDU message list
//      +errorIndi:     Error indication, whether some packets 
//                      are lost in the TTI
// RETURN::             NULL
// **/
static
void UmtsRlcAmReceivePduFromMac(
    Node* node,
    int iface,
    UmtsRlcAmEntity* amEntity,
    std::list<Message*>& msgList,
    BOOL errorIndi)
{
    UmtsRlcData* rlcData = UmtsRlcGetSubLayerData(node, iface);

    if (DEBUG_RLC_PACKET && !msgList.empty()
        && ((UmtsRlcEntity*)(amEntity->entityVar))->ueId == DEBUG_UE_ID
        && ((UmtsRlcEntity*)(amEntity->entityVar))->rbId == DEBUG_RB_ID
        && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x2)
            || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x1)))
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        UmtsRlcEntity* entity = (UmtsRlcEntity*)(amEntity->entityVar);
        printf("%s, RLC Entity of Node: %u receives %u messages from MAC "
            "through RB %d \n",
            timeStr, node->nodeId, (unsigned int)msgList.size(),
            (int)entity->rbId);
        //Message* frontMsg = msgList.front();
        std::for_each(msgList.begin(),
                      msgList.end(),
                      UmtsRlcPrintPdu);
    }

    std::list<Message*>::iterator it;
    for (it = msgList.begin(); it != msgList.end(); ++it)
    {
        Message* pduMsg = *it;

        //Second, inspect what type of PDU it is
        if (UmtsRlcAmGetDCBit(pduMsg))
        {
            rlcData->stats.numPdusFromMac++;
            if (node->guiOption)
            {
                UmtsRlcEntity* entity =
                    (UmtsRlcEntity*)(amEntity->entityVar);

                if (entity->rbId < UMTS_RBINRAB_START_ID ||
                    entity->rbId >= UMTS_BCCH_RB_ID)
                {
                    rlcData->stats.numInControlByte +=
                        UmtsGetTotalPktSizeFromMsgList(pduMsg);
                }
                else
                {
                    rlcData->stats.numInDataByte +=
                        UmtsGetTotalPktSizeFromMsgList(pduMsg);
                }
            }
            if (amEntity->state == UMTS_RLC_ENTITYSTATE_RESETPENDING)
            {
                Message* msg = pduMsg;
                while (msg)
                {
                    Message* nextMsg = msg->next;
                    MESSAGE_Free(node, msg);
                    msg = nextMsg;
                }
                continue;
            }
            //First, inspect PDU to see if there is a polling bit set,
            //If so, prepare status PDU
            if (UmtsRlcAmGetPollBit(pduMsg))
            {
                if (DEBUG_STATUSPDU
                    && ((UmtsRlcEntity*)amEntity->entityVar)->ueId ==
                    DEBUG_UE_ID
                    && ((UmtsRlcEntity*)amEntity->entityVar)->rbId == 
                    DEBUG_RB_ID
                    && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x2)
                        || (UmtsIsNodeB(node) &&
                        DEBUG_RB_DIRECT & 0x1)))
                {
                    UmtsLayer2PrintRunTimeInfo(
                        node, 0, UMTS_LAYER2_SUBLAYER_RLC);
                    std::cout << "Node: " << node->nodeId <<
                            " receives a poll in the PDU message"
                            << "    rcvNext: " << amEntity->rcvNextSn
                            << "    rcvHighest: " 
                            << amEntity->rcvHighestExpSn
                            << "    rcvWnd: "  << amEntity->rcvWnd
                            << std::endl;
                    std::cout << std::endl;
                    //UmtsPrintMessage(std::cout, pduMsg);
                }
                amEntity->statusPduFlag |= UMTS_RLC_STATUSPDU_ACK;
            }
            UmtsRlcExtractSduSegmentFromPdu(
                node,
                iface,
                amEntity->entityVar,
                pduMsg);
        }
        else
        {
            UInt8 hdrOct1 = (UInt8)(*MESSAGE_ReturnPacket(pduMsg));
            UInt8 controlPduType = UmtsRlcGet4BitDataFromOctet(
                                        hdrOct1,
                                        0);
            if (controlPduType == UMTS_RLC_CONTROLPDU_STATUS)
            {
                rlcData->stats.numAmStatusPdusFromMac++;
                if (amEntity->state == UMTS_RLC_ENTITYSTATE_RESETPENDING)
                {
                    MESSAGE_Free(node, pduMsg);
                    continue;
                }
                UmtsRlcAmReceiveStatusPdu(node,
                                          iface,
                                          amEntity,
                                          pduMsg);
            }
            else if (controlPduType ==  UMTS_RLC_CONTROLPDU_RESET)
            {
                rlcData->stats.numAmResetPdusFromMac++;
                UmtsRlcAmHandleResetPdu(node,
                                        rlcData,
                                        amEntity,
                                        pduMsg);
            }
            else if (controlPduType ==  UMTS_RLC_CONTROLPDU_RESETACK)
            {
                rlcData->stats.numAmResetAckPdusFromMac++;
                UmtsRlcAmHandleResetAckPdu(node,
                                           rlcData,
                                           amEntity,
                                           pduMsg);
            }
            else
            {
                ERROR_Assert(FALSE, "UmtsRlcAmReceivePduFromMac: "
                    "received an invalid type of control PDU.");
            }
        }
    }
}

// /**
// FUNCTION::       UmtsRlcAmUpperLayerHasSduToSend
// LAYER::          UMTS LAYER2 RLC
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
void UmtsRlcAmUpperLayerHasSduToSend(
    Node* node,
    int iface,
    UmtsRlcAmEntity* amEntity,
    Message* sduMsg)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcAmUpperLayerHasSduToSend: ";
    ERROR_Assert(sduMsg && amEntity, errorMsg);

    //UmtsRlcData* rlcData = UmtsRlcGetSubLayerData(node, iface);

#if 0
    if (amEntity->bufSize + MESSAGE_ReturnPacketSize(sduMsg) >
                            amEntity->maxTransBufSize)
    {
        rlcData->stats.numSdusDiscarded++;
        MESSAGE_Free(node, sduMsg);
        return;
    }
#endif

    UmtsRlcUmAmFragmentEntity* fragmentEntity = amEntity->fragmentEntity;
    fragmentEntity->Fragment(node, iface, sduMsg);
}

// /**
// FUNCTION::           UmtsRlcAmDeliverPduToMac
// LAYER::              UMTS LAYER2 RLC
// PURPOSE::            Prepare RLC PDUs sent to MAC through AM SAP,
//                      invoked for each TTI
// PARAMETERS::
//      +node:          Node pointer
//      +iface:         the interface index
//      +amEntity:      The AM entity
//      +numPdus:       Number of PDUs MAC can receive, given by MAC
//      +pduSize:       PDU size given by MAC
//      +pduList:       A list containing the pdus to be sent
// RETURN::             NULL
// **/
static
void UmtsRlcAmDeliverPduToMac(
    Node* node,
    int iface,
    UmtsRlcAmEntity* amEntity,
    int numPdus,
    int pduSize,
    std::list<Message*>* pduList)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcAmDeliverPduToMac: ";
    ERROR_Assert(amEntity, errorMsg);

    UmtsRlcData* rlcData = UmtsRlcGetSubLayerData(node, iface);
    UmtsRlcEntity* rlcEntity = (UmtsRlcEntity*)(amEntity->entityVar);

    //std::list<Message*>* pduList = new std::list<Message*>;
    int numPduAvail = numPdus;

    if (amEntity->state == UMTS_RLC_ENTITYSTATE_RESETPENDING)
    {
        if (numPduAvail > 0 && amEntity->rstPduMsg != NULL)
        {
            pduList->push_back(amEntity->rstPduMsg);
            amEntity->rstPduMsg = NULL;
            numPduAvail--;
            rlcData->stats.numAmResetPdusToMac++;
        }
        if (numPduAvail > 0 && amEntity->rstAckPduMsg != NULL)
        {
            pduList->push_back(amEntity->rstAckPduMsg);
            amEntity->rstAckPduMsg = NULL;
            rlcData->stats.numAmResetAckPdusToMac++;
        }
        return;
    }

    int numAmdPdus = 0;            
             //calculate how many number of AM data PDUs are added
    if (numPduAvail > 0)
    {
        if (amEntity->rstAckPduMsg != NULL)
        {
            pduList->push_back(amEntity->rstAckPduMsg);
            amEntity->rstAckPduMsg = NULL;
            numPduAvail--;
            rlcData->stats.numAmResetAckPdusToMac++;
        }

        //Firstly, scheduled retransmission PDUs are dequeued.
        std::list<UmtsRlcAmRtxPduData*>::iterator itRtxBuf
            = amEntity->rtxPduBuf->begin();
        while (numPduAvail > 0 && itRtxBuf != amEntity->rtxPduBuf->end())
        {
            //Prevent sending PDU with seqence number >= VT(MS)
            //Normally this won't happen for retransmitted PDUs except that
            //the sndWnd decreased during transmission
            if (!UmtsRlcSeqNumInWnd(
                     UmtsRlcAmGetSeqNum((*itRtxBuf)->message),
                                         amEntity->ackSn,
                                         amEntity->sndWnd,
                                         UMTS_RLC_AM_SNUPPERBOUND))
            {
                break;
            }
            if ((*itRtxBuf)->txScheduled)
            {
                //MAXDAT event is handled here instead 
                //of where retransmission
                //is scheduled since during this period, new ACK may arrive
                (*itRtxBuf)->dat++;
                if ((*itRtxBuf)->dat == amEntity->maxDat)
                {
                    UmtsRlcAmHandleMaxDat(
                        node,
                        iface,
                        amEntity,
                        (*itRtxBuf)->seqNum,
                        pduList);

                    return;
                }
                if ((DEBUG_FRAGASSEMB || DEBUG_STATUSPDU)
                    && ((UmtsRlcEntity*)amEntity->entityVar)->ueId == 
                    DEBUG_UE_ID
                    && ((UmtsRlcEntity*)amEntity->entityVar)->rbId ==
                    DEBUG_RB_ID
                    && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x1)
                        || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x2)))
                {
                    UmtsLayer2PrintRunTimeInfo(
                        node, 0, UMTS_LAYER2_SUBLAYER_RLC);
                    std::cout << "Node: " << node->nodeId <<
                            " retransmits PDU message"
                            << std::endl;
                    std::cout << "PDU sequence number: "
                        << (*itRtxBuf)->seqNum << std::endl
                        << "Expected in-sequence ACK: "
                        << amEntity->ackSn << std::endl
                        << "Sending window size: "
                        << amEntity->sndWnd << std::endl
                        << "Next PDU sequence number to be sent: "
                        << amEntity->sndNextSn
                        << std::endl;
                    UmtsRlcPrintPdu((*itRtxBuf)->message);
                    printf("\n");
                }
                Message* rtxPduMsg = 
                    UmtsDuplicateMessageList(node, (*itRtxBuf)->message);
                if (amEntity->pollTriggered)
                {
                    UmtsRlcAmSetPollBit(rtxPduMsg);
                    //An poll timeout only triggers one poll
                    UmtsRlcAmInitPollTimer(
                        node,
                        iface,
                        amEntity,
                        (amEntity->sndNextSn)%UMTS_RLC_AM_SNUPPERBOUND);
                }

                // update dyanmic stats
                if (node->guiOption)
                {
                    if (rlcEntity->rbId < UMTS_RBINRAB_START_ID ||
                        rlcEntity->rbId >= UMTS_BCCH_RB_ID)
                    {
                        rlcData->stats.numOutControlByte +=
                            UmtsGetTotalPktSizeFromMsgList(rtxPduMsg);
                    }
                    else
                    {
                        rlcData->stats.numOutDataByte +=
                            UmtsGetTotalPktSizeFromMsgList(rtxPduMsg);
                    }
                }

                pduList->push_back(rtxPduMsg);
                //Make the schedule to rtx false to enable
                //rtx PDU with higher PDU have chance to be rtxed.
                (*itRtxBuf)->txScheduled = FALSE;

                numPduAvail -= 1;
                numAmdPdus++;
                rlcData->stats.numPdusToMac++;
            }

            ++itRtxBuf;
        }

        //Secondly, scheduled status PDUs are dequeued.
        //If piggybacked status PDU feature are to be implemented,
        //When dequeuing retx and tx PDUs, looking through each PDU to
        //see whether there is enough space for status PDUs.
        std::deque<Message*> statusPduList;
        UmtsRlcAmPrepareStatusPdu(
            node,
            amEntity,
            statusPduList);

        std::deque<Message*>::iterator dequeBegin;
        std::deque<Message*>::iterator dequeEnd;
        std::deque<Message*>::iterator itStatusBuf
            = statusPduList.begin();
        dequeBegin = itStatusBuf;
        dequeEnd = itStatusBuf;
        while (numPduAvail > 0 && itStatusBuf != statusPduList.end())
        {
            // update dyanmic stats
            if (node->guiOption)
            {
                if (rlcEntity->rbId < UMTS_RBINRAB_START_ID ||
                    rlcEntity->rbId >= UMTS_BCCH_RB_ID)
                {
                    rlcData->stats.numOutControlByte +=
                        UmtsGetTotalPktSizeFromMsgList((*itStatusBuf));
                }
                else
                {
                    rlcData->stats.numOutDataByte +=
                        UmtsGetTotalPktSizeFromMsgList((*itStatusBuf));
                }
            }

            pduList->push_back(*itStatusBuf);
            if (DEBUG_STATUSPDU
                && ((UmtsRlcEntity*)amEntity->entityVar)->ueId == 
                DEBUG_UE_ID
                && ((UmtsRlcEntity*)amEntity->entityVar)->rbId == 
                DEBUG_RB_ID
                && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x2)
                    || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x1)))
            {
                UmtsLayer2PrintRunTimeInfo(
                    node, 0, UMTS_LAYER2_SUBLAYER_RLC);
                std::cout << "Node: " << node->nodeId <<
                        " is sending STATUS PDU message "
                        << std::hex << std::showbase
                        << *itStatusBuf
                        << std::dec << std::noshowbase << std::endl;
                //UmtsPrintMessage(std::cout, *itStatusBuf);
            }
            numPduAvail -= 1;
            ++itStatusBuf;
            dequeEnd = itStatusBuf;
            rlcData->stats.numAmStatusPdusToMac++;
        }
        statusPduList.erase(dequeBegin, dequeEnd);
        //Release the STATUS PDU message not sent
        std::for_each(statusPduList.begin(),
                      statusPduList.end(),
                      UmtsRlcFreeMessage(node));
        statusPduList.clear();

        //Thirdly, scheduled first-timer-tx PDUs are dequeued.
        //If MAC still can take more PDUs than in pdu buffer,
        //fetch PDUs possibly left in the fragment entity
        if ((unsigned int)numPduAvail > amEntity->transPduBuf->size())
        {
            amEntity->fragmentEntity->RequestLastPdu(node);
        }

        std::deque<Message*>::iterator itTxBuf =
            amEntity->transPduBuf->begin();
        dequeBegin = itTxBuf;
        dequeEnd = itTxBuf;
        unsigned int& seqNum = amEntity->sndNextSn;
        while (numPduAvail > 0 && itTxBuf != amEntity->transPduBuf->end()
            && UmtsRlcSeqNumInWnd(seqNum,
                                  amEntity->ackSn,
                                  amEntity->sndWnd,
                                  UMTS_RLC_AM_SNUPPERBOUND))
        {
            Message* pduMsg = *itTxBuf;
            UmtsRlcAmSetSeqNum(pduMsg, seqNum);

            // update dyanmic stats
            if (node->guiOption)
            {
                if (rlcEntity->rbId < UMTS_RBINRAB_START_ID ||
                    rlcEntity->rbId >= UMTS_BCCH_RB_ID)
                {
                    rlcData->stats.numOutControlByte +=
                        UmtsGetTotalPktSizeFromMsgList(pduMsg);
                }
                else
                {
                    rlcData->stats.numOutDataByte +=
                        UmtsGetTotalPktSizeFromMsgList(pduMsg);
                }
            }
            pduList->push_back(pduMsg);

            rlcData->stats.numPdusToMac++;
            amEntity->bufSize -= amEntity->sndPduSize;

            //Move the first-time transmitted PDU into retransmission buf
            UmtsRlcAmRtxPduData* rtxPduData = (UmtsRlcAmRtxPduData*)
                                    MEM_malloc(sizeof(UmtsRlcAmRtxPduData));
            rtxPduData->seqNum = seqNum;
            rtxPduData->dat = 1;
            rtxPduData->txScheduled = FALSE;
            rtxPduData->message = UmtsDuplicateMessageList(node, pduMsg);
            amEntity->rtxPduBuf->push_back(rtxPduData);

            //If it is the LAST PDU, or a poll is scheduled by a poll timer
            if (amEntity->pollTriggered ||
                (amEntity->pollTrig & UMTS_RLC_POLL_LASTPDU
                && (itTxBuf + 1 == amEntity->transPduBuf->end()
                    || ((seqNum + 1)%UMTS_RLC_AM_SNUPPERBOUND ==
                    (amEntity->ackSn+amEntity->sndWnd)
                        % UMTS_RLC_AM_SNUPPERBOUND))))
            {
                if (DEBUG_STATUSPDU
                    && ((UmtsRlcEntity*)amEntity->entityVar)->ueId == 
                    DEBUG_UE_ID
                    && ((UmtsRlcEntity*)amEntity->entityVar)->rbId == 
                    DEBUG_RB_ID
                    && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x1)
                        || (UmtsIsNodeB(node) && 
                        DEBUG_RB_DIRECT & 0x2)))
                {
                    UmtsLayer2PrintRunTimeInfo(
                        node, 0, UMTS_LAYER2_SUBLAYER_RLC);
                    std::cout << "Node: " << node->nodeId <<
                            " trigers a poll in the PDU message"
                            << std::endl;
                    std::cout << "PDU sequence number: " << seqNum
                        << "    Expected in-sequence ACK: "
                        << amEntity->ackSn << std::endl
                        << "Sending window size: " << amEntity->sndWnd
                        << "    Next PDU sequence number to be sent: "
                        << amEntity->sndNextSn
                        << std::endl;
                    std::cout << std::endl;
                }
                UmtsRlcAmSetPollBit(pduMsg);
                //Start Poll timer to prevent deadlock
                UmtsRlcAmInitPollTimer(
                    node,
                    iface,
                    amEntity,
                    (seqNum + 1)%UMTS_RLC_AM_SNUPPERBOUND);
            }

            if (DEBUG_FRAGASSEMB
                && ((UmtsRlcEntity*)amEntity->entityVar)->ueId ==
                DEBUG_UE_ID
                && ((UmtsRlcEntity*)amEntity->entityVar)->rbId == 
                DEBUG_RB_ID
                && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x1)
                    || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x2)))
            {
                UmtsLayer2PrintRunTimeInfo(node, 0, UMTS_LAYER2_SUBLAYER_RLC);
                UmtsRlcHeader pduHdrStruct(
                    UMTS_RLC_ENTITY_AM,
                    amEntity->sndLiOctetsLen);
                pduHdrStruct.BuildHdrStruct(MESSAGE_ReturnPacket(pduMsg));

                std::cout << "Node: "<< node->nodeId <<
                    " RB: " << (int)rlcEntity->rbId <<
                    " inserts first time transmission"
                    <<" PDU into transmitting buffer, " << std::endl
                    << "sequence number: " << seqNum << std:: endl
                    << " AM sending PDU size: "
                    << amEntity->sndPduSize << std::endl
                /*    << " PDU header: "<< std::endl;
                std::cout << pduHdrStruct*/;
                //UmtsRlcPrintPdu(pduMsg);
                printf("\n");
            }

            UmtsRlcAmIncSN(seqNum);

            ++itTxBuf;
            dequeEnd = itTxBuf;
            numPduAvail -= 1;
            numAmdPdus++;
        }
        amEntity->transPduBuf->erase(dequeBegin, dequeEnd);

        //At last, if a poll is needed and there is no PDU are dequeued
        //yet,dequeue a PDU with sequence number == VT(S)-1 if there is one.
        //Set the polling bit for it.
        if (amEntity->pollTriggered
            && !amEntity->rtxPduBuf->empty()
            && numAmdPdus == 0)
        {
            UmtsRlcAmRtxPduData* rtxPduData = amEntity->rtxPduBuf->front();
            rtxPduData->dat++;
            if (rtxPduData->dat == amEntity->maxDat)
            {
                UmtsRlcAmHandleMaxDat(
                    node,
                    iface,
                    amEntity,
                    rtxPduData->seqNum,
                    pduList);
                return;
            }
            if ((DEBUG_FRAGASSEMB || DEBUG_STATUSPDU)
                && ((UmtsRlcEntity*)amEntity->entityVar)->ueId ==DEBUG_UE_ID
                && ((UmtsRlcEntity*)amEntity->entityVar)->rbId ==DEBUG_RB_ID
                && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x1)
                    || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x2)))
            {
                UmtsLayer2PrintRunTimeInfo(
                    node, 0, UMTS_LAYER2_SUBLAYER_RLC);
                std::cout << "Node: " << node->nodeId <<
                        " retransmits the first unacked PDU"
                        " message for polling"
                        << std::endl;
                std::cout << "PDU sequence number: "
                    << rtxPduData->seqNum << std::endl
                    << "Expected in-sequence ACK: "
                    << amEntity->ackSn << std::endl
                    << "Sending window size: "
                    << amEntity->sndWnd << std::endl
                    << "Next PDU sequence number to be sent: "
                    << amEntity->sndNextSn
                    << std::endl;
                UmtsRlcPrintPdu(rtxPduData->message);
                printf("\n");
            }
            Message* rtxPduMsg = UmtsDuplicateMessageList(
                                    node,
                                    rtxPduData->message);
            UmtsRlcAmSetPollBit(rtxPduMsg);
            UmtsRlcAmInitPollTimer(
                    node,
                    iface,
                    amEntity,
                    (amEntity->sndNextSn)%UMTS_RLC_AM_SNUPPERBOUND);
            // update dyanmic stats
            if (node->guiOption)
            {
                if (rlcEntity->rbId < UMTS_RBINRAB_START_ID ||
                    rlcEntity->rbId >= UMTS_BCCH_RB_ID)
                {
                    rlcData->stats.numOutControlByte +=
                        UmtsGetTotalPktSizeFromMsgList(rtxPduMsg);
                }
                else
                {
                    rlcData->stats.numOutDataByte +=
                        UmtsGetTotalPktSizeFromMsgList(rtxPduMsg);
                }
            }

            pduList->push_back(rtxPduMsg);
            rtxPduData->txScheduled = FALSE;
            numPduAvail -= 1;
            rlcData->stats.numPdusToMac++;
        }
    }

    UmtsRlcEntity* entity = (UmtsRlcEntity*)(amEntity->entityVar);
    if (DEBUG_RLC_PACKET && !pduList->empty()
        && entity->ueId == DEBUG_UE_ID
        && entity->rbId == DEBUG_RB_ID
        && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x1)
            || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x2)))
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, RLC Entity of Node: %u deliever %u messages to MAC "
            "through RB %d, number of PDUs allowed: %d \n",
            timeStr, node->nodeId, (unsigned int)pduList->size(), 
            (int)(entity->rbId), numPdus);

        std::for_each(pduList->begin(),
                      pduList->end(),
                      UmtsRlcPrintPdu);
    }
}

// /**
// FUNCTION::       UmtsRlcFindEntity
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Search for a RLC entity
// PARAMETERS::
//      +node:      Node pointer
//      +rlc:       RLC sublayer data structure
//      +rbId:      radio bearer Id
//      +ueId:      UE ID, default argument
// RETURN::         UmtsRlcEntity
// **/
static
UmtsRlcEntity* UmtsRlcFindEntity(
    Node* node,
    UmtsRlcData* rlc,
    unsigned int rbId,
    NodeAddress ueId)
{
    UmtsRlcEntity* rlcEntity = NULL;
    if (UmtsIsUe(node))
    {
        UmtsRlcEntityList::iterator it
            = find_if(rlc->ueEntityList->begin(),
                rlc->ueEntityList->end(),
                UmtsRlcEntityRbEqual(rbId));

        if (it != rlc->ueEntityList->end())
        {
            rlcEntity =  *it;
        }
    }
    else if (UmtsIsNodeB(node))
    {
        UmtsRlcEntity entityKey(ueId, (char)rbId);

        UmtsRlcEntitySet::iterator it
            = rlc->rncEntitySet->find(&entityKey);
        if (it != rlc->rncEntitySet->end())
        {
            rlcEntity = *it;
        }
    }
    else
    {
        ERROR_ReportError("UmtsRlcFindEntity: wrong node type");
    }
    return rlcEntity;
}

// /**
// FUNCTION::       UmtsRlcHandleTimeout
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Handle expired timer
// PARAMETERS::
//    +node:        Node pointer
//    +interfaceIndex: the interface index
//    +message:     The Timer message
// RETURN::         void
// **/
static
void UmtsRlcHandleTimeout(
    Node* node,
    int interfaceIndex,
    Message* message)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcHandleTimeout: ";
    UmtsRlcData* rlc = UmtsRlcGetSubLayerData(node, interfaceIndex);
    char* info = MESSAGE_ReturnInfo(message);
    UmtsLayer2TimerInfoHdr* layer2Info = (UmtsLayer2TimerInfoHdr*)info;
    UmtsRlcTimerInfoHdr* infoHdr = (UmtsRlcTimerInfoHdr*)
                                   (info+sizeof(UmtsLayer2TimerInfoHdr));
    char* additionalInfo = info + sizeof(UmtsLayer2TimerInfoHdr)
                                + sizeof(UmtsRlcTimerInfoHdr);
    UmtsLayer2TimerType timerType = layer2Info->timeType;


    if (timerType == UMTS_RLC_TIMER_UPDATE_GUI)
    {
        UmtsRlcHandleUpdateGuiTimer(node, interfaceIndex);
        MESSAGE_Send(
            node, message, UMTS_DYNAMIC_STAT_AVG_TIME_WINDOW);

        return;
    }

    NodeAddress ueId = infoHdr->ueId;
    char rbId = infoHdr->rbId;
    UmtsRlcEntityDirect direction;

    direction = infoHdr->direction;
    UmtsRlcEntity* entity = UmtsRlcFindEntity(node,
                                              rlc,
                                              rbId,
                                              ueId);
    //Entity could be released before the timer messager arrives
    if (!entity)
    {
        MESSAGE_Free(node, message);
        return;
    }

    switch (entity->type)
    {
        case UMTS_RLC_ENTITY_TM:
        case UMTS_RLC_ENTITY_UM:
        {
            break;
        }
        case UMTS_RLC_ENTITY_AM:
        {
            UmtsRlcAmEntity* amEntity =
                    (UmtsRlcAmEntity*)(entity->entityData);
            if (timerType == UMTS_RLC_TIMER_RST)
            {
                UmtsRlcAmHandleResetTimeOut(
                    node,
                    interfaceIndex,
                    amEntity);
            }
            else if (timerType == UMTS_RLC_TIMER_POLL)
            {
                UmtsRlcAmHandlePollTimeOut(
                    node,
                    interfaceIndex,
                    amEntity,
                    additionalInfo);
            }
            else
            {
                sprintf(errorMsg+strlen(errorMsg),
                        "wrong rlc timer type.");
                ERROR_ReportError(errorMsg);
            }
            break;
        }
        default:
        {
            sprintf(errorMsg+strlen(errorMsg), "wrong RLC entity type.");
            ERROR_ReportError(errorMsg);
        }
    }

    MESSAGE_Free(node, message);
}

// /**
// FUNCTION::       UmtsRlcAddRbInfoForPduList
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Add RB information for a PDU list into the
//                  head message of the PDU list
// PARAMETERS::
//    + node:       pointer to the network node
//    + ueId:       UE identifier
//    + rbId:       RB ID
//    + pduList:    A list containing the pdus to be sent
// RETURN:: NULL
// **/
static
void UmtsRlcAddRbInfoForPduList(
    Node*   node,
    NodeId  ueId,
    char    rbId,
    std::list<Message*>& pduList)
{
    if (pduList.empty())
    {
        return;
    }

    Message* headMsg = pduList.front();
    MESSAGE_AddInfo(node,
                    headMsg,
                    sizeof(UmtsRlcPduListInfo),
                    INFO_TYPE_UmtsRlcPduListInfo);

    UmtsRlcPduListInfo* info = (UmtsRlcPduListInfo*)
                               MESSAGE_ReturnInfo(
                                   headMsg,
                                   INFO_TYPE_UmtsRlcPduListInfo);
    info->ueId = ueId;
    info->rbId = rbId;
    info->len = (int)pduList.size();
}

// /**
// FUNCTION::       UmtsRlcGetInfoForPduList
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Get RB information for a PDU list from the
//                  head message of the PDU list
// PARAMETERS::
//    + node:       pointer to the network node
//    + ueId:       for returning the UE identifier
//    + rbId:       for returning the RB ID
//    + pduListLen: for returning the number of messages
//                   belonging to the same RB
//    + pduList:    The received RLC PDU list
// RETURN:: NULL
// **/
static
void UmtsRlcGetInfoForPduList(
    Node*   node,
    NodeId* ueId,
    char*   rbId,
    int*    pduListLen,
    std::list<Message*>& pduList)
{
    ERROR_Assert(!pduList.empty(), "UmtsRlcGetInfoForPduList: "
        "PDU list is empty. ");

    Message* headMsg = pduList.front();
    UmtsRlcPduListInfo* info = (UmtsRlcPduListInfo*)
                               MESSAGE_ReturnInfo(
                                   headMsg,
                                   INFO_TYPE_UmtsRlcPduListInfo);
    ERROR_Assert(info, "UmtsRlcGetInfoForPduList: "
        "Head message doesn't contain PDU list info.");
    *ueId = info->ueId;
    *rbId = info->rbId;
    *pduListLen = info->len;
}

// /**
// FUNCTION::       UmtsRlcPackMessage
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Pack message before sending RLC PDUs to MAC
// PARAMETERS::
//    + node:       pointer to the network node
//    + pduList:    A list containing the pdus to be sent
// RETURN:: NULL
// **/
static
void UmtsRlcPackMessage(
    Node*   node,
    std::list<Message*>& pduList)
{
    std::list<Message*> packedPduList;
    std::list<Message*>::iterator it;
    for (it = pduList.begin(); it != pduList.end(); ++it)
    {
        unsigned int pduSize = 0;
        Message* currentMsg = *it;
        while (currentMsg)
        {
            pduSize += MESSAGE_ReturnPacketSize(currentMsg);
            currentMsg = currentMsg->next;
        }
        Message* packedMsg = UmtsPackMessage(node,
                                             (*it),
                                             TRACE_UMTS_LAYER2);
        MESSAGE_InfoAlloc(node,
                          packedMsg,
                          sizeof(UmtsRlcPackedPduInfo));
        UmtsRlcPackedPduInfo* info = (UmtsRlcPackedPduInfo*)
                                        MESSAGE_ReturnInfo(packedMsg);
        info->pduSize = pduSize;
        packedPduList.push_back(packedMsg);
    }

    pduList.clear();
    pduList.insert(pduList.end(),
                   packedPduList.begin(),
                   packedPduList.end());
}

// /**
// FUNCTION::       UmtsRlcUnPackMessage
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Unpack message received from MAC
// PARAMETERS::
//    + node:       pointer to the network node
//    + msgList:    A list containing the packet PDUs received
//                  from MAC layer
// RETURN:: NULL
// **/
static
void UmtsRlcUnPackMessage(
    Node* node,
    std::list<Message*>& msgList)
{
    std::list<Message*> unPackPduList;
    std::list<Message*>::iterator it;

    for (it = msgList.begin(); it!= msgList.end(); ++it)
    {
        Message* unPackMsg = UmtsUnpackMessage(node,
                                               (*it),
                                               FALSE,
                                               TRUE);
        unPackPduList.push_back(unPackMsg);
    }
    msgList.clear();
    msgList.insert(msgList.end(),
                   unPackPduList.begin(),
                   unPackPduList.end());
}

// /**
// FUNCTION::       UmtsRlcUpperLayerHasSduToSend
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Called by upper layers to request
//                  RLC to send SDU
// PARAMETERS::
//    + node : Node* : pointer to the network node
//    + interfaceIndex : int : index of interface
//    + rbId : unsigned int : radio bearer Id assc. with this RLC entity
//    + sduMsg : Message* : The SDU message from upper layer
//    + ueId : NodeAddress : UE identifier, used to look up RLC entity
//                           at UTRAN side
// RETURN::         NULL
// **/
void  UmtsRlcUpperLayerHasSduToSend(
    Node* node,
    int interfaceIndex,
    unsigned int rbId,
    Message* sduMsg,
    NodeAddress ueId)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcUpperLayerHasSduToSend: ";
    UmtsRlcData* rlc = UmtsRlcGetSubLayerData(node, interfaceIndex);
    UmtsRlcEntity* entity = UmtsRlcFindEntity(node,
                                              rlc,
                                              rbId,
                                              ueId);
    rlc->stats.numSdusFromUpperLayer++;

    if (!entity)
    {
        MESSAGE_Free(node, sduMsg);
        rlc->stats.numSdusDiscarded++;
        return;
        //sprintf(errorMsg+strlen(errorMsg), 
        // "Cann't find existing RLC entity "
        //    "given UEID: %u, and RBID: %u", ueId, rbId);
        //ERROR_ReportError(errorMsg);
    }

    UmtsLayer3Data *umtsL3 = UmtsLayer3GetData(node);
    if (umtsL3->newStats)
    {
        umtsL3->newStats->AddPacketSentToMacDataPoints(
            node,
            sduMsg,
            STAT_Unicast, // Treat all packets as unicast
            true, // Is data
            false); // Is not forwarded
    }

    if (DEBUG_FRAGASSEMB
        && ueId == DEBUG_UE_ID
        && rbId == DEBUG_RB_ID
        && ((UmtsIsUe(node) && DEBUG_RB_DIRECT & 0x1)
            || (UmtsIsNodeB(node) && DEBUG_RB_DIRECT & 0x2)))
    {
        MESSAGE_AddInfo(node,
                        sduMsg,
                        sizeof(UmtsRlcSduInfo),
                        INFO_TYPE_UmtsRlcSduInfo);
        UmtsRlcSduInfo* sduInfo = (UmtsRlcSduInfo*)
                                   MESSAGE_ReturnInfo(
                                        sduMsg,
                                        INFO_TYPE_UmtsRlcSduInfo);
        sduInfo->sduSeqNum = entity->sduSeqNum;
        entity->sduSeqNum++;
    }

    switch (entity->type)
    {
        case UMTS_RLC_ENTITY_TM:
        {
            UmtsRlcTmTransEntity* tmTxEntity =
                ((UmtsRlcTmEntity*)entity->entityData)->txEntity;
            ERROR_Assert(tmTxEntity, errorMsg);
            UmtsRlcTmUpperLayerHasSduToSend(
                node,
                interfaceIndex,
                tmTxEntity,
                sduMsg);
            break;
        }
        case UMTS_RLC_ENTITY_UM:
        {
            UmtsRlcUmTransEntity* umTxEntity =
                ((UmtsRlcUmEntity*)entity->entityData)->txEntity;
            ERROR_Assert(umTxEntity, errorMsg);
            UmtsRlcUmUpperLayerHasSduToSend(
                node,
                interfaceIndex,
                umTxEntity,
                sduMsg);
            break;
        }
        case UMTS_RLC_ENTITY_AM:
        {
            UmtsRlcAmEntity* amEntity = 
                (UmtsRlcAmEntity*)entity->entityData;
            ERROR_Assert(amEntity, errorMsg);
            UmtsRlcAmUpperLayerHasSduToSend(
                node,
                interfaceIndex,
                amEntity,
                sduMsg);
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
// FUNCTION::       UmtsRlcReceivePduFromMac
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Called by MAC to submit an received RLC PDU
// PARAMETERS::
//    + node : Node* : pointer to the network node
//    + interfaceIndex : int : interdex of interface
//    + rbId : unsigned int : radio bearer Id asscociated 
//                            with this RLC entity
//    + msgList : std::list<Message*>& : Msg list containing PDUs from MAC
//    + errorIndi : BOOL : Error indication, whether some packets are
//                         lost in the TTI
//    + ueId: NodeAddress : UE identifier, used to look up RLC 
//                          entity at UTRAN side
// RETURN:: NULL
// **/
void UmtsRlcReceivePduFromMac(
    Node* node,
    int interfaceIndex,
    unsigned int rbId,
    std::list<Message*>& msgList,
    BOOL errorIndi,
    NodeAddress ueId)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcReceivPduFromMac ";
    UmtsRlcData* rlc = UmtsRlcGetSubLayerData(node, interfaceIndex);
    UmtsRlcEntity* entity = UmtsRlcFindEntity(node,
                                              rlc,
                                              rbId,
                                              ueId);

    if (!entity)
    {
        //if (rbId == UMTS_CCCH_RB_ID)
        //{
            std::for_each(msgList.begin(),
                          msgList.end(),
                          UmtsFreeStlMsgPtr(node));
            return;
        //}
        //else
        //{
        //    sprintf(errorMsg+strlen(errorMsg), 
        //        "Cann't find existing RLC entity "
        //        "given UEID: %u, and RBID: %u", ueId, rbId);
        //    ERROR_ReportError(errorMsg);
        //}
    }

    UmtsRlcUnPackMessage(node, msgList);

    switch (entity->type)
    {
        case UMTS_RLC_ENTITY_TM:
        {
            UmtsRlcTmRcvEntity* tmRxEntity =
                ((UmtsRlcTmEntity*)entity->entityData)->rxEntity;
            ERROR_Assert(tmRxEntity, errorMsg);
            UmtsRlcTmReceivePduFromMac(
                node,
                interfaceIndex,
                tmRxEntity,
                msgList,
                errorIndi);
            break;
        }
        case UMTS_RLC_ENTITY_UM:
        {
            UmtsRlcUmRcvEntity* umRxEntity =
                ((UmtsRlcUmEntity*)entity->entityData)->rxEntity;
            ERROR_Assert(umRxEntity, errorMsg);
            UmtsRlcUmReceivePduFromMac(
                node,
                interfaceIndex,
                umRxEntity,
                msgList,
                errorIndi);
            break;
        }
        case UMTS_RLC_ENTITY_AM:
        {
            UmtsRlcAmEntity* amEntity = 
                (UmtsRlcAmEntity*)entity->entityData;
            ERROR_Assert(amEntity, errorMsg);
            UmtsRlcAmReceivePduFromMac(
                node,
                interfaceIndex,
                amEntity,
                msgList,
                errorIndi);
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
// FUNCTION::       UmtsRlcReceivePduFromMac
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Called by MAC to submit an received RLC PDU
// PARAMETERS::
//    + node : Node* : pointer to the network node
//    + interfaceIndex : int: interdex of interface
//    + msgList : std::list<Message*>& : Msg list containing PDUs from MAC
//    + errorIndi:  BOOL : Error indication, whether some packets
//                         are lost in the TTI
// RETURN:: NULL
// **/
void UmtsRlcReceivePduFromMac(
    Node* node,
    int interfaceIndex,
    std::list<Message*>& msgList,
    BOOL errorIndi)
{
    NodeId ueId;
    char   rbId;
    int    pduListLen;
    while (!msgList.empty())
    {
        UmtsRlcGetInfoForPduList(node,
                                 &ueId,
                                 &rbId,
                                 &pduListLen,
                                 msgList);
        std::list<Message*> pduListPerRb;
        std::list<Message*>::iterator pos = msgList.begin();
        for (int i = 0; i < pduListLen; i++)
        {
            ++pos;
        }
        pduListPerRb.splice(pduListPerRb.end(),
                            msgList,
                            msgList.begin(),
                            pos);
        UmtsRlcReceivePduFromMac(node,
                                 interfaceIndex,
                                 rbId,
                                 pduListPerRb,
                                 errorIndi,
                                 ueId);
    }
}

// /**
// FUNCTION::       UmtsRlcHasPacketSentToMac
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Called by MAC to send RLC PDUs to MAC
// PARAMETERS::
//    + node : Node* : pointer to the network node
//    + interfaceIndex : int : index of interface
//    + rbId : unsigned int : radio bearer Id asscociated 
//                           with this RLC entity
//    + numPdus : int : number of PDUs the MAC allows this RLC to send
//    + pduSize : int : PDU size the MAC indicates to the RLC
//    + pduList : std::list<Message*>& :
//                A list containing the pdus to be sent
//    + ueId : NodeAddress: UE identifier, used to look up RLC
//                          entity at UTRAN side
// RETURN:: NULL
// **/
void UmtsRlcHasPacketSentToMac(
    Node* node,
    int interfaceIndex,
    unsigned int rbId,
    int numPdus,
    int pduSize,
    std::list<Message*>& pduList,
    NodeAddress ueId)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcHasPacketSentToMac ";
    UmtsRlcData* rlc = UmtsRlcGetSubLayerData(node, interfaceIndex);
    UmtsRlcEntity* entity = UmtsRlcFindEntity(node,
                                              rlc,
                                              rbId,
                                              ueId);
    sprintf(errorMsg+strlen(errorMsg), "Cann't find existing RLC entity "
        "given UEID: %u, and RBID: %u", ueId, rbId);
    ERROR_Assert(entity, errorMsg);

    switch (entity->type)
    {
        case UMTS_RLC_ENTITY_TM:
        {
            UmtsRlcTmTransEntity* tmTxEntity =
                ((UmtsRlcTmEntity*)entity->entityData)->txEntity;
            UmtsRlcTmDeliverPduToMac(
                node,
                interfaceIndex,
                tmTxEntity,
                numPdus,
                pduSize,
                pduList);
            break;
        }
        case UMTS_RLC_ENTITY_UM:
        {
            UmtsRlcUmTransEntity* umTxEntity =
                ((UmtsRlcUmEntity*)entity->entityData)->txEntity;
            ERROR_Assert(umTxEntity, errorMsg);
            UmtsRlcUmDeliverPduToMac(
                node,
                interfaceIndex,
                umTxEntity,
                numPdus,
                pduSize,
                pduList);
            break;
        }
        case UMTS_RLC_ENTITY_AM:
        {
            UmtsRlcAmEntity* amEntity = 
                (UmtsRlcAmEntity*)entity->entityData;
            ERROR_Assert(amEntity, errorMsg);
            UmtsRlcAmDeliverPduToMac(
                node,
                interfaceIndex,
                amEntity,
                numPdus,
                pduSize,
                &pduList);
            break;
        }
        default:
        {
            sprintf(errorMsg+strlen(errorMsg), "wrong RLC entity type.");
            ERROR_ReportError(errorMsg);
        }
    }
    UmtsRlcPackMessage(node, pduList);
    UmtsRlcAddRbInfoForPduList(node,
                               node->nodeId,    //Info is read by peer node
                               (char)rbId,
                               pduList);
    if (!pduList.empty())
    {
        entity->lastDataDequeueTime = node->getNodeTime();
    }
}

// /**
// FUNCTION::       UmtsRlcCreateEntity
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Create a RLC entity
// PARAMETERS::
//      +node:      Node pointer
//      +rlc:       RLC sublayer data structure
//      +ueId:      ID of UE
//      +rbId:      Id of redio bearer
//      +direction: The traffic direction of the entity to be created
//      +type:      The type of the entity to be created
//      +entityConfig: The entity configuration parameter
// RETURN::         BOOL
// **/
static
BOOL UmtsRlcCreateEntity(
    Node* node,
    UmtsRlcData* rlc,
    NodeAddress ueId,
    unsigned int rbId,
    UmtsRlcEntityDirect direction,
    UmtsRlcEntityType type,
    const void* entityConfig)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcCreateEntity: ";

    UmtsRlcEntity* entity = UmtsRlcFindEntity(node,
                                              rlc,
                                              rbId,
                                              ueId);

    if (!entity)
    {
        entity = (UmtsRlcEntity*)MEM_malloc(sizeof(UmtsRlcEntity));
        memset(entity, 0, sizeof(UmtsRlcEntity));
        entity->type = type;
        entity->ueId = ueId;
        entity->rbId = (char)rbId;
        switch (type)
        {
            case UMTS_RLC_ENTITY_TM:
            {
                if (direction == UMTS_RLC_ENTITY_TX)
                {
                    UmtsRlcCreateTmTransEntity(entity);
                    if (entityConfig)
                    {
                        UmtsRlcConfigTmTransEntity(
                            entity,
                            (UmtsRlcTmConfig*)entityConfig);
                    }
                }
                else if (direction == UMTS_RLC_ENTITY_RX)
                {
                    UmtsRlcCreateTmRcvEntity(entity);
                    if (entityConfig)
                    {
                        UmtsRlcConfigTmRcvEntity(
                            entity,
                            (UmtsRlcTmConfig*)entityConfig);
                    }
                }
                break;
            }
            case UMTS_RLC_ENTITY_UM:
            {
                if (direction == UMTS_RLC_ENTITY_TX)
                {
                    UmtsRlcCreateUmTransEntity(node, entity);
                    if (entityConfig)
                    {
                        UmtsRlcConfigUmTransEntity(
                            node,
                            entity,
                            (UmtsRlcUmConfig*)entityConfig);
                    }
                }
                else if (direction == UMTS_RLC_ENTITY_RX)
                {
                    UmtsRlcCreateUmRcvEntity(entity);
                    if (entityConfig)
                    {
                        UmtsRlcConfigUmRcvEntity(
                            node,
                            entity,
                            (UmtsRlcUmConfig*)entityConfig);
                    }
                }
                break;
            }
            case UMTS_RLC_ENTITY_AM:
            {
                UmtsRlcCreateAmEntity(node, entity);
                if (entityConfig)
                {
                    UmtsRlcConfigAmEntity(
                        entity,
                        (UmtsRlcAmConfig*)entityConfig);
                }
                break;
            }
            default:
            {
                sprintf(errorMsg+strlen(errorMsg), 
                        "wrong RLC entity type.");
                ERROR_ReportError(errorMsg);
            }
        }

        if (UmtsIsNodeB(node))
        {
            rlc->rncEntitySet->insert(entity);
        }
        else
        {
            rlc->ueEntityList->push_back(entity);
        }
    }
    else
    {
        ERROR_Assert(entity->type == type, 
                     "Entity exists with different type.");
        switch (type)
        {
            case UMTS_RLC_ENTITY_TM:
            {
                if (direction == UMTS_RLC_ENTITY_TX)
                {
                    if (UmtsRlcFindTmTransEntity(entity))
                    {
                        sprintf(errorMsg+strlen(errorMsg),
                                "entity exists.");
                        ERROR_ReportError(errorMsg);
                    }
                    UmtsRlcCreateTmTransEntity(entity);
                    if (entityConfig)
                    {
                        UmtsRlcConfigTmTransEntity(
                            entity,
                            (UmtsRlcTmConfig*)entityConfig);
                    }
                }
                else if (direction == UMTS_RLC_ENTITY_RX)
                {
                    if (UmtsRlcFindTmRcvEntity(entity))
                    {
                        sprintf(errorMsg+strlen(errorMsg), 
                               "entity exists.");
                        ERROR_ReportError(errorMsg);
                    }
                    UmtsRlcCreateTmRcvEntity(entity);
                    if (entityConfig)
                    {
                        UmtsRlcConfigTmRcvEntity(
                            entity,
                            (UmtsRlcTmConfig*)entityConfig);
                    }
                }
                break;
            }
            case UMTS_RLC_ENTITY_UM:
            {
                if (direction == UMTS_RLC_ENTITY_TX)
                {
                    if (UmtsRlcFindUmTransEntity(entity))
                    {
                        sprintf(errorMsg+strlen(errorMsg), 
                                "entity exists.");
                        ERROR_ReportError(errorMsg);
                    }
                    UmtsRlcCreateUmTransEntity(node, entity);
                    if (entityConfig)
                    {
                        UmtsRlcConfigUmTransEntity(
                            node,
                            entity,
                            (UmtsRlcUmConfig*)entityConfig);
                    }
                }
                else if (direction == UMTS_RLC_ENTITY_RX)
                {
                    if (UmtsRlcFindUmRcvEntity(entity))
                    {
                        sprintf(errorMsg+strlen(errorMsg),
                                "entity exists.");
                        ERROR_ReportError(errorMsg);
                    }
                    UmtsRlcCreateUmRcvEntity(entity);
                    if (entityConfig)
                    {
                        UmtsRlcConfigUmRcvEntity(
                            node,
                            entity,
                            (UmtsRlcUmConfig*)entityConfig);
                    }
                }
                break;
            }
            case UMTS_RLC_ENTITY_AM:
            {
                sprintf(errorMsg+strlen(errorMsg), "entity exists.");
                ERROR_ReportError(errorMsg);
                break;
            }
            default:
            {
                sprintf(errorMsg+strlen(errorMsg), "wrong node type.");
                ERROR_ReportError(errorMsg);
            }
        }
    }
    return TRUE;
}

// /**
// FUNCTION::       UmtsRlcReleaseEntity
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Release a RLC entity
// PARAMETERS::
//      +node:      Node pointer
//      +rlc:       RLC sublayer data structure
//      +ueId:      UE ID
//      +rbId:      Radio bearer ID
//      +direction: The traffic direction of the entity to be created
// RETURN::         BOOL
// **/
static
BOOL UmtsRlcReleaseEntity(
    Node* node,
    UmtsRlcData* rlc,
    NodeAddress ueId,
    unsigned int rbId,
    UmtsRlcEntityDirect direction)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcReleaseEntity: ";
    UmtsRlcEntity* entity = NULL;
    UmtsRlcEntityList::iterator p;
    UmtsRlcEntitySet::iterator q;

    if (UmtsIsUe(node))
    {
        p = find_if(rlc->ueEntityList->begin(),
                    rlc->ueEntityList->end(),
                    UmtsRlcEntityRbEqual(rbId));

        if (p != rlc->ueEntityList->end())
        {
            entity = *p;
        }
        else
        {
            return FALSE;
        }
    }
    else if (UmtsIsNodeB(node))
    {
        UmtsRlcEntity entityKey(ueId, (char)rbId);
        q = rlc->rncEntitySet->find(&entityKey);
        if (q != rlc->rncEntitySet->end())
        {
            entity = *q;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        sprintf(errorMsg+strlen(errorMsg), "wrong node type.");
        ERROR_ReportError(errorMsg);
    }

    switch (entity->type)
    {
        case UMTS_RLC_ENTITY_TM:
        {
            UmtsRlcTmEntity* tmEntity = 
                (UmtsRlcTmEntity*)entity->entityData;
            if (direction == UMTS_RLC_ENTITY_TX ||
                direction == UMTS_RLC_ENTITY_BI)
            {
                UmtsRlcTmTransEntity* tmTxEntity = tmEntity->txEntity;
                if (!tmTxEntity)
                {
                    return FALSE;
                }
                UmtsRlcTmTransEntityFinalize(node, tmTxEntity);
                MEM_free(tmTxEntity);
                tmEntity->txEntity = NULL;
            }
            if (direction == UMTS_RLC_ENTITY_RX ||
                direction == UMTS_RLC_ENTITY_BI)
            {
                UmtsRlcTmRcvEntity* tmRxEntity = tmEntity->rxEntity;
                if (!tmRxEntity)
                {
                    return FALSE;
                }
                UmtsRlcTmRcvEntityFinalize(node, tmRxEntity);
                MEM_free(tmRxEntity);
                tmEntity->rxEntity = NULL;
            }
            if (!tmEntity->rxEntity && !tmEntity->txEntity)
            {
                MEM_free(tmEntity);
                MEM_free(entity);
                if (UmtsIsNodeB(node))
                {
                    rlc->rncEntitySet->erase(q);
                }
                else
                {
                    rlc->ueEntityList->erase(p);
                }
            }
            break;
        }
        case UMTS_RLC_ENTITY_UM:
        {
            UmtsRlcUmEntity* umEntity = 
                (UmtsRlcUmEntity*)entity->entityData;
            if (direction == UMTS_RLC_ENTITY_TX ||
                direction == UMTS_RLC_ENTITY_BI)
            {
                UmtsRlcUmTransEntity* umTxEntity = umEntity->txEntity;
                if (!umTxEntity)
                {
                    return FALSE;
                }
                UmtsRlcUmTransEntityFinalize(node, umTxEntity);
                MEM_free(umTxEntity);
                umEntity->txEntity = NULL;

            }
            if (direction == UMTS_RLC_ENTITY_RX ||
                direction == UMTS_RLC_ENTITY_BI)
            {
                UmtsRlcUmRcvEntity* umRxEntity = umEntity->rxEntity;
                if (!umRxEntity)
                {
                    return FALSE;
                }
                UmtsRlcUmRcvEntityFinalize(node, umRxEntity);
                MEM_free(umRxEntity);
                umEntity->rxEntity = NULL;
            }

            if (!umEntity->rxEntity && !umEntity->txEntity)
            {
                MEM_free(umEntity);
                MEM_free(entity);
                if (UmtsIsNodeB(node))
                {
                    rlc->rncEntitySet->erase(q);
                }
                else
                {
                    rlc->ueEntityList->erase(p);
                }
            }
            break;
        }
        case UMTS_RLC_ENTITY_AM:
        {
            UmtsRlcAmEntity* amEntity = 
                (UmtsRlcAmEntity*)entity->entityData;
            if (!amEntity)
            {
                return FALSE;
            }
            UmtsRlcAmEntityFinalize(node, amEntity);
            MEM_free(amEntity);
            MEM_free(entity);
            if (UmtsIsNodeB(node))
            {
                rlc->rncEntitySet->erase(q);
            }
            else
            {
                rlc->ueEntityList->erase(p);
            }
            break;
        }
        default:
        {
            sprintf(errorMsg+strlen(errorMsg), "wrong node type.");
            ERROR_ReportError(errorMsg);
        }
    }
    return TRUE;
}

// /**
// FUNCTION::       UmtsRlcRcvCommandFromRrc
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Called by RRC to control RLC entities
// PARAMETERS::
//    +node:        Node pointer
//    +iface:       index of interface
//    +cmdType:     RRC control command type
//    +cmdArgs:     Command arguments
// RETURN::         BOOL
// **/
static
BOOL UmtsRlcRcvCommandFromRrc(
    Node* node,
    int iface,
    UmtsInterlayerCommandType cmdType,
    const void* cmdArgs)
{
    BOOL retVal = FALSE;
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcRcvCommandFromRrc: ";
    UmtsRlcData* rlc = UmtsRlcGetSubLayerData(node, iface);
    sprintf(errorMsg+strlen(errorMsg), "cannot RLC sublayer data "
                "in this interface: %d.", iface);
    ERROR_Assert(rlc, errorMsg);

    switch (cmdType)
    {
        case UMTS_CRLC_CONFIG_ESTABLISH:
        {
            UmtsRlcRrcEstablishArgs* args = (UmtsRlcRrcEstablishArgs*)
                                            cmdArgs;
            retVal = UmtsRlcCreateEntity(node,
                                         rlc,
                                         args->ueId,
                                         args->rbId,
                                         args->direction,
                                         args->entityType,
                                         args->entityConfig);
            break;
        }
        case UMTS_CRLC_CONFIG_RELEASE:
        {
            UmtsRlcRrcReleaseArgs* args = (UmtsRlcRrcReleaseArgs*)
                                            cmdArgs;
            retVal = UmtsRlcReleaseEntity(node,
                                          rlc,
                                          args->ueId,
                                          args->rbId,
                                          args->direction);
            break;
        }
        default:
        {
            sprintf(errorMsg+strlen(errorMsg),
                        "unknown RRC command type");
            ERROR_Assert(rlc, errorMsg);
        }
    }
    return retVal;
}

// /**
// FUNCTION::       UmtsRlcInitPrimCellChange
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Start to move the RLC entity (to a UE) status 
//                  in this Cell to another Cell
// PARAMETERS::
//    + node : Node* : pointer to the network node
//    + interfaceIndex : int : index of interface
//    + buffer : std::string& : String buffer to store RLC status
//    + ueId : NodeId : UE identifier, used to look up 
//                      RLC entity at UTRAN side
// RETURN:: NULL
// **/
void UmtsRlcInitPrimCellChange(
    Node* node,
    int iface,
    std::string& buffer,
    NodeId ueId)
{
    UmtsRlcData* rlc = UmtsRlcGetSubLayerData(node, iface);
    if (!UmtsIsNodeB(node))
        return;

    UmtsRlcEntitySet* rlcEntities = rlc->rncEntitySet;
    for (UmtsRlcEntitySet::iterator it = rlcEntities->begin();
         it != rlcEntities->end(); ++it)
    {
        if ((*it)->ueId == ueId && (*it)->rbId != UMTS_CCCH_RB_ID)
        {
            //Store the status of the RLC entity into the buffer
            std::string subBuf;
            (*it)->store(node, subBuf);
            buffer += (char)(*it)->rbId;
            size_t subBufSize = subBuf.size();
            buffer.append((char*)&subBufSize, sizeof(subBufSize));
            buffer.append(subBuf);

            //Clear the packet buffer of this entity
            (*it)->reset(node);
        }
    }
}

#if 0
// /**
// FUNCTION::       UmtsRlcCompletePrimCellChange
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Finishing moving the RLC entity (to a UE)
//                  status in this Cell
//                  to another Cell
// PARAMETERS::
//    + node:       pointer to the network node
//    + interfaceIndex: index of interface
//    + buffer:     String buffer stored with RLC status to be moved here
//    + bufSize:    The size of the buffer
//    + ueId:       UE identifier, used to look up RLC entity at UTRAN side
// RETURN:: NULL
// **/
void UmtsRlcCompletePrimCellChange(
    Node* node,
    int iface,
    BOOL fragHead,
    unsigned int fragNum,
    const char* fragment,
    size_t fragSize,
    NodeId ueId,
    BOOL* restoreDone)
{
    UmtsRlcData* rlc = UmtsRlcGetSubLayerData(node, iface);
    if (!UmtsIsNodeB(node))
        return;

    if (fragHead)
    {
        rlc->cellSwitchFragSum = fragNum;
        rlc->cellSwitchFrags->resize(fragNum);
        rlc->cellSwitchFragNum = 1;
        rlc->cellSwitchFrags->at(0) = std::string(fragment, fragSize);
    }
    else
    {
        if (rlc->cellSwitchFragSum == 0)
        {
            return;
        }
        ERROR_Assert(fragNum < (unsigned int)rlc->cellSwitchFragSum,
            "incorrect fragment number.");
        rlc->cellSwitchFrags->at(fragNum) = std::string(fragment, fragSize);
        rlc->cellSwitchFragNum ++;
    }

    if (rlc->cellSwitchFragNum == rlc->cellSwitchFragSum)
    {
        *restoreDone = TRUE;
        std::string buffer;
        unsigned int bufSize = 0;
        for (int i = 0; i < rlc->cellSwitchFragSum; i++)
        {
            buffer += rlc->cellSwitchFrags->at(i);
            bufSize += rlc->cellSwitchFrags->at(i).size();
        }
        size_t index = 0;
        while (index < bufSize)
        {
            char rbId = buffer[index++];
            size_t subBufSize;
            memcpy(&subBufSize, &buffer[index], sizeof(subBufSize));
            index += sizeof(subBufSize);

            UmtsRlcEntity* rlcEntity = UmtsRlcFindEntity(
                                            node,
                                            rlc,
                                            rbId,
                                            ueId);
            rlcEntity->restore(node, &buffer[index], subBufSize);
            index += subBufSize;
        }
        rlc->cellSwitchFragNum = 0;
        rlc->cellSwitchFragSum = 0;
        rlc->cellSwitchFrags->clear();
    }
}
#endif //Old cell switch code

// /**
// FUNCTION::       UmtsRlcCompletePrimCellChange
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Finishing moving the RLC entity (to a UE) 
//                  status in this Cell to another Cell
// PARAMETERS::
//    + node : Node* : pointer to the network node
//    + iface : int index of interface
//    + buffer :  const char* : String buffer stored with RLC status
//                             to be moved here
//    + bufSize : UInt64 : The size of the buffer
//    + ueId : NodeId : UE identifier, used to look up 
//                      RLC entity at UTRAN side
//    + restoreDone : BOOL* : restore has been done or not?
// RETURN:: NULL
// **/
void UmtsRlcCompletePrimCellChange(
    Node* node,
    int iface,
    const char* buffer,
    UInt64 bufSize,
    NodeId ueId,
    BOOL* restoreDone)
{
    UmtsRlcData* rlc = UmtsRlcGetSubLayerData(node, iface);
    if (!UmtsIsNodeB(node))
        return;

    size_t index = 0;
    while (index < bufSize)
    {
        char rbId = buffer[index++];
        size_t subBufSize;
        memcpy(&subBufSize, &buffer[index], sizeof(subBufSize));
        index += sizeof(subBufSize);

        UmtsRlcEntity* rlcEntity = UmtsRlcFindEntity(
                                        node,
                                        rlc,
                                        rbId,
                                        ueId);
        ERROR_Assert(rlcEntity, "UmtsRlcCompletePrimCellChange: "
            "cannot find RLC entity to restore context.");
        rlcEntity->restore(node, &buffer[index], subBufSize);
        index += subBufSize;
    }
    ERROR_Assert(index == bufSize,
        "UmtsRlcCompletePrimCellChange: "
        "error when restoring RLC context.");

    *restoreDone = TRUE;
}

// /**
// FUNCTION::       UmtsRlcPrintStat
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Print RLC statistics
// PARAMETERS::
//    + node:       pointer to the network node
//    + iface:      interface index
//    + rlcData:    RLC sublayer data structure
// RETURN::         NULL
// **/
static
void UmtsRlcPrintStat(
    Node* node,
    int iface,
    UmtsRlcData* rlcData)
{
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "Number of SDUs received from upper layer = %d",
        rlcData->stats.numSdusFromUpperLayer);
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of SDUs sent to upper layer = %d",
        rlcData->stats.numSdusToUpperLayer);
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of SDUs discarded = %d",
        rlcData->stats.numSdusDiscarded);
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of data PDUs sent to MAC sublayer = %d",
        rlcData->stats.numPdusToMac);
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of data PDUs received from MAC sublayer = %d",
        rlcData->stats.numPdusFromMac);
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of AM STATUS PDUs sent to MAC sublayer = %d",
        rlcData->stats.numAmStatusPdusToMac);
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of AM STATUS PDUs received from MAC sublayer = %d",
        rlcData->stats.numAmStatusPdusFromMac);
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of AM RESET PDUs sent to MAC sublayer = %d",
        rlcData->stats.numAmResetPdusToMac);
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of AM RESET PDUs received from MAC sublayer = %d",
        rlcData->stats.numAmResetPdusFromMac);
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, "Number of AM RESET ACK PDUs sent to MAC sublayer = %d",
        rlcData->stats.numAmResetAckPdusToMac);
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS RLC",
                 ANY_DEST,
                 iface,
                 buf);

    sprintf(buf, 
        "Number of AM RESET ACK PDUs received from MAC sublayer = %d",
        rlcData->stats.numAmResetAckPdusFromMac);
    IO_PrintStat(node,
                 "Layer2",
                 "UMTS RLC",
                 ANY_DEST,
                 iface,
                 buf);
}

// /**
// FUNCTION::       UmtsRlcHandleInterLayerCommand
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Handle interlayer command
// PARAMETERS::
//    +node : Node* : Node pointer
//    +iface : int : index of interface
//    +cmdType : UmtsInterlayerCommandType : RRC control command type
//    +cmdArgs : const void* : Command arguments
// RETURN::         BOOL
// **/
BOOL UmtsRlcHandleInterLayerCommand(
    Node* node,
    int iface,
    UmtsInterlayerCommandType cmdType,
    const void* cmdArgs)
{
    BOOL retVal;
    switch (cmdType)
    {
        case UMTS_CRLC_CONFIG_ESTABLISH:
        case UMTS_CRLC_CONFIG_RELEASE:
        {
            retVal = UmtsRlcRcvCommandFromRrc(node,
                                              iface,
                                              cmdType,
                                              cmdArgs);
        }
        default:
            retVal = FALSE;
    }
    return retVal;
}

// /**
// FUNCTION::       UmtsRlcIndicateStatusToMac
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Called by MAC to get RLC sending entity info
// PARAMETERS::
//    + node : Node* : pointer to the network node
//    + interfaceIndex: int : index of interface
//    + rbId : unsigned int : radio bearer ID, used to find the RLC entity
//    + info : UmtsRlcEntityInfoToMac& : The RLC information MAC needs
//    + ueId : nodeAddress : UE identifier, used to look up RLC 
//                           entity at UTRAN side
// RETURN:: BOOL:   FALSE if there is no transmission entity for this RB
// **/
BOOL UmtsRlcIndicateStatusToMac(
    Node* node,
    int interfaceIndex,
    unsigned int rbId,
    UmtsRlcEntityInfoToMac& info,
    NodeAddress ueId)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsIndicateStatusToMac ";
    UmtsRlcData* rlc = UmtsRlcGetSubLayerData(node, interfaceIndex);
    UmtsRlcEntity* entity = UmtsRlcFindEntity(node,
                                              rlc,
                                              rbId,
                                              ueId);
    if (!entity)
        return FALSE;

    info.lastAccessTime = entity->lastDataDequeueTime;

    switch (entity->type)
    {
        case UMTS_RLC_ENTITY_TM:
        {
            UmtsRlcTmTransEntity* tmTxEntity =
                ((UmtsRlcTmEntity*)entity->entityData)->txEntity;
            if (tmTxEntity)
            {
                info.entityType = UMTS_RLC_ENTITY_TM;
                info.bufOccupancy = tmTxEntity->bufSize;
            }
            else
            {
                return FALSE;
            }
            break;
        }
        case UMTS_RLC_ENTITY_UM:
        {
            UmtsRlcUmTransEntity* umTxEntity =
                ((UmtsRlcUmEntity*)entity->entityData)->txEntity;
            if (umTxEntity)
            {
                info.entityType = UMTS_RLC_ENTITY_UM;
                info.bufOccupancy = umTxEntity->bufSize;
            }
            else
            {
                return FALSE;
            }
            break;
        }
        case UMTS_RLC_ENTITY_AM:
        {
            UmtsRlcAmEntity* amEntity = 
                (UmtsRlcAmEntity*)entity->entityData;
            if (amEntity)
            {
                info.entityType = UMTS_RLC_ENTITY_AM;
                info.bufOccupancy = amEntity->bufSize;
            }
            else
            {
                return FALSE;
            }
            break;
        }
        default:
        {
            sprintf(errorMsg+strlen(errorMsg), "wrong RLC entity type.");
            ERROR_ReportError(errorMsg);
        }
    }
    return TRUE;
}

// /**
// FUNCTION   :: UmtsMacInitDynamicStats
// LAYER      :: Layer2 RLC
// PURPOSE    :: Initiate dynamic statistics
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + interfaceIdnex   : UInt32 : interface index of UMTS
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : POinter to PHY UMTS UE data
// RETURN     :: void : NULL
// **/
static
void UmtsRlcInitDynamicStats(Node *node,
                             UInt32 interfaceIndex,
                             UmtsRlcData* rlc)
{
    if (!node->guiOption)
    {
        return;
    }

    // set the update timer
    Message* updateTimer;
    updateTimer = UmtsLayer2InitTimer(node,
                        interfaceIndex,
                        UMTS_LAYER2_SUBLAYER_RLC,
                        UMTS_RLC_TIMER_UPDATE_GUI,
                        0,
                        NULL);
    MESSAGE_Send(
        node,
        updateTimer,
        UMTS_DYNAMIC_STAT_AVG_TIME_WINDOW);


    // register the dynamic statistics gui Id
    if (UmtsGetNodeType(node) == CELLULAR_UE)
    {
        rlc->stats.dlThruputGuiId =
            GUI_DefineMetric("UMTS UE: DownLink Throughput (bps)",
                         node->nodeId,
                         GUI_MAC_LAYER,
                         interfaceIndex,
                         GUI_DOUBLE_TYPE,
                         GUI_CUMULATIVE_METRIC);

        rlc->stats.ulThruputGuiId =
            GUI_DefineMetric("UMTS UE: UpLink Throughput (bps)",
                         node->nodeId,
                         GUI_MAC_LAYER,
                         interfaceIndex,
                         GUI_DOUBLE_TYPE,
                         GUI_CUMULATIVE_METRIC);
    }
    else if (UmtsGetNodeType(node) == CELLULAR_NODEB)
    {
        rlc->stats.dlThruputGuiId =
            GUI_DefineMetric("UMTS NodeB: DownLink Throughput (bps)",
                         node->nodeId,
                         GUI_MAC_LAYER,
                         interfaceIndex,
                         GUI_DOUBLE_TYPE,
                         GUI_CUMULATIVE_METRIC);

        rlc->stats.ulThruputGuiId =
            GUI_DefineMetric("UMTS NodeB: UpLink Throughput (bps)",
                         node->nodeId,
                         GUI_MAC_LAYER,
                         interfaceIndex,
                         GUI_DOUBLE_TYPE,
                         GUI_CUMULATIVE_METRIC);
    }
    else
    {
        ERROR_ReportError("RLC is running on non UE/NodeB node");
    }
}

// /**
// FUNCTION::       UmtsRlcInit
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        RLC Initalization
//
// PARAMETERS::
//    + node:       Node* : pointer to the network node
//    + interfaceIndex: unsigned int : interdex of interface
//    + nodeInput:  const NodeInput* : Input from configuration file
// RETURN::         NULL
// **/
void UmtsRlcInit(Node* node,
                 unsigned int interfaceIndex,
                 const NodeInput* nodeInput)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcInit: ";

    MacCellularData* macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;
    CellularUmtsLayer2Data* layer2Data =
        macCellularData->cellularUmtsL2Data;
    UmtsRlcData* rlc;

    rlc = (UmtsRlcData*) MEM_malloc(sizeof(UmtsRlcData));
    ERROR_Assert(rlc != NULL, errorMsg);
    memset(rlc, 0, sizeof(UmtsRlcData));
    layer2Data->umtsRlcData  = rlc;

    // init dyanamic statistics
    UmtsRlcInitDynamicStats(node, interfaceIndex, rlc);

#if 0
    rlc->cellSwitchFrags = new UmtsCellSwitchFragVec;
#endif //Old cell switch code
    if (UmtsIsNodeB(node))
    {
        rlc->rncEntitySet = new UmtsRlcEntitySet;
    }
    else if (UmtsIsUe(node))
    {
        rlc->ueEntityList = new UmtsRlcEntityList;
    }
    else
    {
        ERROR_ReportError("UmtsRlcInit: invalid node type.");
    }
}

// /**
// FUNCTION::       UmtsRlcFinalize
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        RLC finalization function
// PARAMETERS::
//    + node:       Node* : pointer to the network node
//    + interfaceIndex: unsigned int : interdex of interface
// RETURN::         NULL
// **/
void UmtsRlcFinalize(Node* node, unsigned int interfaceIndex)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcFinalize: ";
    MacCellularData* macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;
    CellularUmtsLayer2Data* layer2Data = 
        macCellularData->cellularUmtsL2Data;
    UmtsRlcData* rlc = layer2Data->umtsRlcData;
    ERROR_Assert(rlc, errorMsg);

    if (macCellularData->collectStatistics)
    {
        UmtsRlcPrintStat(node, (int)interfaceIndex, rlc);
    }
#if 0
    delete rlc->cellSwitchFrags;
#endif //Old cell switch code

    if (UmtsIsNodeB(node))
    {
        ERROR_Assert(rlc->rncEntitySet, errorMsg);
        UmtsRlcEntitySet::iterator it;
        for (it = rlc->rncEntitySet->begin();
             it != rlc->rncEntitySet->end();
             ++it)
        {
            UmtsRlcEntityFinalize(node, *it);
            MEM_free(*it);
        }
        delete rlc->rncEntitySet;
    }
    else if (UmtsIsUe(node))
    {
        ERROR_Assert(rlc->ueEntityList, errorMsg);
        UmtsRlcEntityList::iterator it;
        for (it = rlc->ueEntityList->begin();
            it != rlc->ueEntityList->end();
            ++it)
        {
            UmtsRlcEntityFinalize(node, *it);
            MEM_free(*it);
        }
        delete rlc->ueEntityList;
    }
    else
    {
        ERROR_ReportError("UmtsRlcFinalize: invalid node type.");
    }
    MEM_free(layer2Data->umtsRlcData);
    layer2Data->umtsRlcData = NULL;
}

// /**
// FUNCTION::       UmtsRlcLayer
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        RLC event handling function
// PARAMETERS::
//    + node:       Node * : pointer to the network node
//    + interfaceIndex: unsigned int : interdex of interface
//    + message     Message* : Message to be handled
// RETURN::         NULL
// **/
void UmtsRlcLayer(Node* node, unsigned int interfaceIndex, Message* msg)
{
    char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcLayer: ";
    MacCellularData* macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;
    CellularUmtsLayer2Data* layer2Data = 
        macCellularData->cellularUmtsL2Data;
    UmtsRlcData* rlc = layer2Data->umtsRlcData;
    ERROR_Assert(rlc, errorMsg);

    switch (msg->eventType)
    {
        case MSG_MAC_TimerExpired:
        {
            UmtsRlcHandleTimeout(node, interfaceIndex, msg);
            break;
        }
        default:
        {
            sprintf(errorMsg+strlen(errorMsg), "Unknown message type.");
            ERROR_ReportError(errorMsg);
        }
    }
}
// /**
// FUNCTION::       UmtsRlcAmUpdateEmptyTransPduBuf
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Get the last pdu from fragment buffer,
//                  when amEntity buffer size is zero
// PARAMETERS::
//      +node:              Node pointer
//      +interfaceIndex    interface Index
//      +rbId:              radio bearer Id
//      +ueId              UE identifier, used to look up RLC
// RETURN::                 NULL
// **/

void UmtsRlcAmUpdateEmptyTransPduBuf(Node* node,
                                     UInt32 interfaceIndex,
                                     unsigned int rbId,
                                     NodeAddress ueId)
{
    UmtsRlcData* rlc = UmtsRlcGetSubLayerData(node, interfaceIndex);
    UmtsRlcEntity* entity = UmtsRlcFindEntity(node,
                                              rlc,
                                              rbId,
                                              ueId);
    if (entity->type == UMTS_RLC_ENTITY_AM)
    {
        UmtsRlcAmEntity* amEntity = 
            (UmtsRlcAmEntity*)entity->entityData;
        if (amEntity->bufSize == 0)
        {
            amEntity->fragmentEntity->RequestLastPdu(node);
        }
    }
}

