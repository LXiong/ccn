#include<mac_dot11n.h>
#include<mac_dot11-sta.h>
#include <mac_dot11-mgmt.h>
#include "phy_802_11.h"
#include "mac_phy_802_11n.h"
#define DEBUG_AMSDU 0
#define DEBUG_AMPDU 0
#define DEBUG_DATA_BURST 0
#define DEBUG_AD_HOC 0

//--------------------------------------------------------------------------
//  NAME:        MacDot11nRemoveReferenceOfFrameFromMap.
//  PURPOSE:     Removes the reference of a frame from Map
//  PARAMETERS:  std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator& keyItr
//                  Pointer to the Key
//               std::list<DOT11_FrameInfo*>::iterator& listItr
//                  Pointer to the list
//  RETURN:      DOT11_FrameInfo*
//                  Pointer tp the frameinfo containing the message
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
DOT11_FrameInfo*
MacDot11nRemoveReferenceOfFrameFromMap(
                        std::map<std::pair<Mac802Address,TosType>,MapValue>
                        ::iterator &keyItr,
                        std::list<DOT11_FrameInfo*>::iterator &listItr)
{
    DOT11_FrameInfo* tmpFrameInfo = (*listItr);
    keyItr->second.frameQueue.erase(listItr);
    return(tmpFrameInfo);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nSetMoreFramesFieldForAnAC.
//  PURPOSE:     Sets the 'isACHasPacket' field of an access category
//  PARAMETERS:  MacDataDot11* dot11
//                  pointer to Dot11 data structure
//               UInt8 acIndex
//                  index of the access category
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nSetMoreFramesFieldForAnAC(MacDataDot11* dot11,
                                        UInt8 acIndex)
{
    // check if map has more packets & set the parameter isACHasPacket
    // accordingly
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = outputBuffer->OutputBufferMap.begin();
    while (keyItr != outputBuffer->OutputBufferMap.end())
    {
        if (!(keyItr->second.frameQueue.empty()))
        {
            if (keyItr->second.blockAckVrbls
                && keyItr->second.blockAckVrbls->state
                    >= BAA_STATE_ADDBA_REQUEST_QUEUED
                && keyItr->second.blockAckVrbls->state
                    <= BAA_STATE_DELBA_QUEUED)
            {
                dot11->ACs[acIndex].isACHasPacket = FALSE;
            }
            else
            {
                dot11->ACs[acIndex].isACHasPacket = TRUE;
                break;
            }
        }
        else
        {
            dot11->ACs[acIndex].isACHasPacket = FALSE;
        }
        keyItr++;
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nGetMaximumMcs.
//  PURPOSE:     Gets the maximum value of the two MCS indices
//  PARAMETERS:  Node* node
//                  pointer to node
//                MacDataDot11* dot11
//                  pointer to Dot11 data structure
//               int maxMcsIdx
//                  max mcs id parameter
//  RETURN:      UInt8
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
UInt8 MacDot11nGetMaximumMcs(Node* node,
                             MacDataDot11* dot11,
                             int maxMcsIdx)
{

    UInt8 maxMcs
        = std::min(maxMcsIdx,(Phy802_11nGetNumAtnaElems(
                    node->phyData[dot11->myMacData->phyNumber])*8 - 1));
    return maxMcs;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nEnqueuePacketInOutputBuffer
//  PURPOSE:     Enqueues packet in the output buffer
//  PARAMETERS:  Node* node
//                  Pointer to the Node
//               MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//               UInt8 acIndex
//                  Index of the access category
//               DOT11_FrameInfo *frameInfo
//                  Frameinfo containing the Packet to enqueue
//               TosType priority
//                  Priority of the packet
//               Mac802Address NextHopAddress
//                  Nexthop address of the packet
//               BOOL isKeyAlreadyPresent
//                  Speifies the existence of the key in the input
//                  buffer map
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nEnqueuePacketInOutputBuffer(
                           Node* node,
                           MacDataDot11* dot11,
                           DOT11_FrameInfo *frameInfo,
                           TosType priority,
                           Mac802Address NextHopAddress,
                           BOOL isKeyAlreadyPresent)
{
    UInt8  acIndex;
    acIndex = (UInt8)MacDot11ReturnAccessCategory(priority);
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
    dot11->numPktsQueuedInOutputBuffer++;
    DOT11n_FrameHdr* hdr =
            (DOT11n_FrameHdr *)MESSAGE_ReturnPacket(frameInfo->msg);
    if (isKeyAlreadyPresent)
    {
        hdr->seqNo = outputBuffer->Mapitr->second.seqNum;
    }
    else
    {
        hdr->seqNo = 0;
    }
    ERROR_Assert(hdr->seqNo <= 4095,"Header Sequence no is above 4095");
    outputBuffer->Enqueue(dot11,
                          node,
                          NextHopAddress,
                          frameInfo,
                          isKeyAlreadyPresent);
    dot11->ACs[acIndex].isACHasPacket = TRUE;
    MacDot11nSetMoreFramesFieldForAnAC(dot11, acIndex);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nSetFromAndToDsInHdr
//  PURPOSE:     Sets the FromDs and ToDs fields in the MAC header
//  PARAMETERS:  const MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//               DOT11n_FrameHdr* const fHdr
//                  Pointer to the Mac header
//  RETURN:      None
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static
void MacDot11nSetFromAndToDsInHdr(const MacDataDot11* const dot11,
                                  DOT11n_FrameHdr* const fHdr)
{
    fHdr->frameFlags = 0;
    switch (fHdr->frameType)
    {
        case DOT11_QOS_DATA:
        {
            if (MacDot11IsAp(dot11))
            {
                fHdr->frameFlags |= DOT11_FROM_DS;
            }
            else if (MacDot11IsBssStation(dot11))
            {
                fHdr->frameFlags |= DOT11_TO_DS;
            }
            break;
        }
        default:
            break;
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nOtherAcsHaveData
//  PURPOSE:     Checks if other access categories apart from 'acIndex' have
//               data to send
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//               UInt8 acIndex
//                  Index of the access category
//  RETURN:      BOOL
//                  Status of other access categories
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11nOtherAcsHaveData(MacDataDot11* dot11,
                               int acIndex)
{
    int NoPacket = 0;
    BOOL Flag = TRUE;
    int currentAcCount = 0;
    for (currentAcCount = 0;
         currentAcCount < DOT11e_NUMBER_OF_AC;
         currentAcCount++)
    {
        if (currentAcCount != acIndex
            && dot11->ACs[currentAcCount].isACHasPacket == FALSE
            && dot11->ACs[currentAcCount].frameInfo == NULL)
        {
            NoPacket++;
        }
    }
    if (NoPacket == 3)
    {
        Flag = FALSE;
    }
    return(Flag);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nSetFieldsInMacHeader
//  PURPOSE:     Enqueues packet in the output buffer
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//               DOT11n_FrameHdr* const fHdr
//                  pointer to mac header
//               UInt8 acIndex
//                  Index of the access category
//               DOT11_FrameInfo *frameInfo
//                  Frameinfo containing the Packet to enqueue
//               Mac802Address destAddr
//                  Nexthop address of the packet
//               DOT11_MacFrameType frameType
//                  frame type
//               TosType priority
//                  Priority of the packet
//               int AmsduPresent
//                  Speifies if the packet is an aggregated msdu
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nSetFieldsInMacHeader(
                    const MacDataDot11* const dot11,
                    DOT11n_FrameHdr* const fHdr,
                    Mac802Address destAddr,
                    DOT11_MacFrameType frameType,
                    int acIndex,
                    TosType priority,
                    int AmsduPresent)
{
    ERROR_Assert(dot11->isHTEnable,"High Throughput not Enabled");
    memset(fHdr, 0, sizeof(DOT11n_FrameHdr));
    fHdr->frameType = (unsigned char) frameType;
    MacDot11nSetFromAndToDsInHdr(dot11, fHdr);
    fHdr->destAddr = destAddr;
    fHdr->sourceAddr = dot11->selfAddr;
    fHdr->address3 = dot11->bssAddr;
    if (dot11->selfAddr != dot11->bssAddr)
    {
        fHdr->address3 = dot11->ipDestAddr;
    }
    else if (MacDot11IsAp(dot11))
    {
        fHdr->address3 = dot11->ipSourceAddr;
    }
    if (MacDot11IsIBSSStation(dot11))
    {
        fHdr->address3 = dot11->bssAddr;
    }
    
    // Set the QoS Control Field Information
    memset(&(fHdr->qoSControl), 0, sizeof(DOT11n_QoSControl));
    if (AmsduPresent == 1)
    {
        fHdr->qoSControl.AmsduPresent = 1;
    }
    else{
        fHdr->qoSControl.AmsduPresent = 0;
    }
    fHdr->qoSControl.TID = priority;
    fHdr->qoSControl.EOSP = 1;
    fHdr->qoSControl.TXOP
        = (unsigned char)(dot11->ACs[acIndex].TXOPLimit / MILLI_SECOND);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nMgmtQueue_EnqueuePacket
//  PURPOSE:     Enqueues a packet in management queue
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_FrameInfo* frameInfo
//                  Pointre to the frameInfo
//               BOOL dequeue
//                  Specifies if dequeue should be called
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11nMgmtQueue_EnqueuePacket(
    Node* node,
    MacDataDot11* dot11,
    DOT11_FrameInfo* frameInfo,
    BOOL dequeue = TRUE)
{
    if (frameInfo == NULL)
    {
        return FALSE;
    }

    ListAppend(node, dot11->mngmtQueue, 0, frameInfo);

    if (ListGetSize(node, dot11->mngmtQueue) == 1 && dequeue)
    {
        MacDot11MngmtQueueHasPacketToSend(node, dot11);
    }

    // No current limit to queue, etc., insertion always successful.
    return TRUE;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nIncrementSeqNumber
//  PURPOSE:     Increment sequence number
//  PARAMETERS:  UInt16 *seqNum
//                  Pointer to seq number
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nIncrementSeqNumber(UInt16 *seqNum)
{
    if (*seqNum == 4095)
    {
        *seqNum = 0;
    }
    else
    {
        *seqNum += 1;
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nDecrementSeqNumber
//  PURPOSE:     Decrement sequence number
//  PARAMETERS:  UInt16 *seqNum
//                  Pointer to seq number
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nDecrementSeqNumber(UInt16 *seqNum)
{
    if (*seqNum == 0)
    {
        *seqNum = 4095;
    }
    else
    {
        *seqNum -= 1;
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nAddIbssStationInfo
//  PURPOSE:     Adds a new station information at an STA
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Address of the STA whose information is been added
//  RETURN:      DOT11n_IBSS_Station_Info*
//                  Pointer to the newly added station information
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
DOT11n_IBSS_Station_Info* MacDot11nAddIbssStationInfo(
                            MacDataDot11* dot11,
                            Mac802Address destAddr)
{
    DOT11n_IBSS_Station_Info *stationInfo = NULL;
    stationInfo = (DOT11n_IBSS_Station_Info*) MEM_malloc(
                                sizeof(DOT11n_IBSS_Station_Info));
    ERROR_Assert(stationInfo != NULL, "MAC 802.11: Out of memory!");
    memset((char*) stationInfo, 0, sizeof(DOT11n_IBSS_Station_Info));
    stationInfo->macAddr = destAddr;
    if (!dot11->ibssConnectedStationsInfo)
    {
        dot11->ibssConnectedStationsInfo = stationInfo;
        stationInfo->next = NULL;
    }
    else
    {
         DOT11n_IBSS_Station_Info *stationInfoItr1
              = dot11->ibssConnectedStationsInfo;
         DOT11n_IBSS_Station_Info *stationInfoItr2 = NULL;
         while (stationInfoItr1)
         {
            stationInfoItr2 = stationInfoItr1;
            stationInfoItr1 = stationInfoItr1->next;
         }
        stationInfoItr2->next = stationInfo;
    }
    return stationInfo;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nGetIbssStationInfo
//  PURPOSE:     Get station information 
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Address of the STA whose information is been added
//  RETURN:      DOT11n_IBSS_Station_Info*
//                  Pointer to the newly added station information
//  ASSUMPTION:  None
//--------------------------------------------------------------------------

DOT11n_IBSS_Station_Info* MacDot11nGetIbssStationInfo(
                            MacDataDot11* dot11,
                            Mac802Address destAddr)
{
    DOT11n_IBSS_Station_Info *stationInfo
                    = dot11->ibssConnectedStationsInfo;
    while (stationInfo)
    {
         if (destAddr == stationInfo->macAddr)
         {
             return stationInfo;
         }
         else
         {
            stationInfo = stationInfo->next;
         }
    }
    return MacDot11nAddIbssStationInfo(dot11,
                                       destAddr);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nIbssTransmitProbeRequest
//  PURPOSE:     Sends a unicast probe request
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Destination addres of the probe request
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nIbssTransmitProbeRequest(Node* node,
                                       MacDataDot11* dot11,
                                       Mac802Address destAddr)
{
    DOT11_MacFrame assoc;
    memset(&assoc, 0, sizeof(DOT11_MacFrame));
    DOT11_Ie element;
    memset(&element, 0, sizeof(DOT11_Ie));
    DOT11_Ie* elementPtr = &element;
    int probeFrameSize = sizeof(DOT11_ProbeReqFrame);
    MacDot11AddHTCapability(node, dot11, elementPtr,
                            &assoc, &probeFrameSize);

    DOT11_ProbeReqFrame* probeFrame;
    
    // Allocate new probe Request frame
    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(
            node,
            msg,
            probeFrameSize,
            TRACE_DOT11);

    probeFrame = (DOT11_ProbeReqFrame*)MESSAGE_ReturnPacket(msg);
    memset(probeFrame, 0, probeFrameSize);
    memcpy(probeFrame, &assoc, probeFrameSize);
    
    // Fill Probe header
    probeFrame->hdr.frameType = DOT11_PROBE_REQ;
    probeFrame->hdr.destAddr = destAddr;
    probeFrame->hdr.sourceAddr = dot11->selfAddr;
    probeFrame->hdr.address3 = dot11->selfAddr;
    strcpy(probeFrame->SSID,dot11->stationMIB->dot11DesiredSSID);

    DOT11_FrameInfo* newFrameInfo =
    (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
    memset(newFrameInfo, 0, sizeof(DOT11_FrameInfo));

    newFrameInfo->msg = msg;
    newFrameInfo->frameType = DOT11_PROBE_REQ;
    newFrameInfo->RA = probeFrame->hdr.destAddr;
    newFrameInfo->TA = dot11->selfAddr;
    newFrameInfo->DA = probeFrame->hdr.destAddr;
    newFrameInfo->SA = dot11->selfAddr;
    newFrameInfo->insertTime = node->getNodeTime();
    DOT11_ManagementVars * mngmtVars =
        (DOT11_ManagementVars*) dot11->mngmtVars;
    mngmtVars->ProbReqGnrated++;
    dot11->mgmtSendResponse = FALSE;
    MacDot11nMgmtQueue_EnqueuePacket(node,
                                     dot11,
                                     newFrameInfo,
                                     FALSE);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11nInitializeBap
//  PURPOSE:     Initializes the block ack aggregment
//  PARAMETERS:  Node* node
//                  Pointer to the Node
//               MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//               MapValue *keyValues
//                  key values
//               Mac802Address destAddr
//                  Nexthop address
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void OutputBuffer::MacDot11nInitializeBap(Node* node,
                                          MacDataDot11* dot11,
                                          MapValue* keyValues,
                                          Mac802Address destAddr)
{
    
    BOOL resetBap = FALSE;
    if (keyValues->blockAckVrbls)
    {
        // Return if Block Ack Policy already initialized
        return;
    }
    if (MacDot11IsAp(dot11))
    {
        DOT11_ApStationListItem* stationItem = NULL;
        stationItem = MacDot11ApStationListGetItemWithGivenAddress(node,
                                                                   dot11,
                                                                   destAddr);
        if ((stationItem
           && (stationItem->data->staCapInfo.ImmidiateBlockACK
           || stationItem->data->staCapInfo.DelayedBlockACK))
           && (dot11->enableImmediateBAAgreement
           || dot11->enableDelayedBAAgreement))
        {
            keyValues->blockAckVrbls = (DOT11n_BlockAckAggrementVrbls*)
                MEM_malloc(sizeof(DOT11n_BlockAckAggrementVrbls));
            memset(keyValues->blockAckVrbls,
                0,
                sizeof(DOT11n_BlockAckAggrementVrbls));
            if (stationItem->data->staCapInfo.DelayedBlockACK)
            {
                keyValues->blockAckVrbls->type = BA_DELAYED;
            }
            else
            {
                keyValues->blockAckVrbls->type = BA_IMMEDIATE;
            }
            resetBap = TRUE;
       }
   }
   else
   {
       if (dot11->stationType == DOT11_STA_IBSS)
       {
           if (destAddr == ANY_MAC802)
           {
               // Return if Destination address is Broadcast
               return;
           }
           DOT11n_IBSS_Station_Info *stationInfo =
                        MacDot11nGetIbssStationInfo(dot11, destAddr);
           ERROR_Assert(stationInfo,"Station info doesnt exist");
           if (stationInfo->probeStatus == IBSS_PROBE_COMPLETED)
           {
                if (stationInfo->staCapInfo.ImmidiateBlockACK
                    || stationInfo->staCapInfo.DelayedBlockACK)
                {
                    keyValues->blockAckVrbls =
                        (DOT11n_BlockAckAggrementVrbls*)
                    MEM_malloc(sizeof(DOT11n_BlockAckAggrementVrbls));
                    memset(keyValues->blockAckVrbls,
                           0,
                           sizeof(DOT11n_BlockAckAggrementVrbls));
                    if (stationInfo->staCapInfo.DelayedBlockACK)
                    {
                        keyValues->blockAckVrbls->type = BA_DELAYED;
                    }
                    else
                    {
                        keyValues->blockAckVrbls->type = BA_IMMEDIATE;
                    }
                    resetBap = TRUE;
                }
           }
           else
           {
               // Do nothing
           }
       }
       else
       {
           ERROR_Assert(dot11->associatedAP,"Not Associated with Any AP");
           ERROR_Assert(destAddr == dot11->associatedAP->bssAddr,
                        "destAddr not equal to bssAddr");
           if ((dot11->associatedAP->apCapInfo.ImmidiateBlockACK
                || dot11->associatedAP->apCapInfo.DelayedBlockACK))
           {
                keyValues->blockAckVrbls = (DOT11n_BlockAckAggrementVrbls*)
                MEM_malloc(sizeof(DOT11n_BlockAckAggrementVrbls));
                memset(keyValues->blockAckVrbls,
                       0,
                       sizeof(DOT11n_BlockAckAggrementVrbls));
                if (dot11->associatedAP->apCapInfo.DelayedBlockACK)
                {
                    keyValues->blockAckVrbls->type = BA_DELAYED;
                }
                else
                {
                    keyValues->blockAckVrbls->type = BA_IMMEDIATE;
                }
                resetBap = TRUE;
            }
        }
    }
    if (resetBap == TRUE)
    {
        keyValues->blockAckVrbls->startingSeqNo
            = MACDOT11N_INVALID_SEQ_NUM;
        keyValues->blockAckVrbls->numPktsNegotiated = 0;
        keyValues->blockAckVrbls->numPktsSent = 0;
        keyValues->blockAckVrbls->
            numPktsLeftToBeSentInCurrentTxop = 0;
        keyValues->blockAckVrbls->
            numPktsToBeSentInCurrentSession = 0;
        keyValues->blockAckVrbls->state = BAA_STATE_DISABLED;
    }
}

//--------------------------------------------------------------------------
//  NAME:        OutputBuffer::Enqueue
//  PURPOSE:     Enqueues packet in the output buffer
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//               Node* node
//                  Pointer to the Node
//               Mac802Address ipDestAddr
//                  Destination address of the packet
//               TosType priority
//                  Priority of the packet
//               DOT11_FrameInfo *frameInfo
//                  Frameinfo containing the Packet to enqueue
//               BOOL isKeyAlreadyPresent
//                  Speifies the existence of the key in the output
//                  buffer map
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void OutputBuffer::Enqueue(
            MacDataDot11* dot11,
            Node* node,
            Mac802Address ipDestAddr,
            DOT11_FrameInfo* frameInfo,
            BOOL isKeyAlreadyPresent)
{
    if (isKeyAlreadyPresent)
    {
        Mapitr->second.frameQueue.push_back(frameInfo);
        MacDot11nIncrementSeqNumber(&Mapitr->second.seqNum);
        Mapitr->second.aggSize +=
            DOT11nMessage_ReturnPacketSize(frameInfo->msg);
        Mapitr->second.numPackets++;
        OutputBuffer::MacDot11nInitializeBap(node,
                                             dot11,
                                             &Mapitr->second,
                                             ipDestAddr);
    }
    else
    {
        MapValue tmpMapValue;
        tmpMapValue.aggSize =
            DOT11nMessage_ReturnPacketSize(frameInfo->msg);
        MacDot11nIncrementSeqNumber(&tmpMapValue.seqNum);
        tmpMapValue.numPackets++;
        OutputBuffer::MacDot11nInitializeBap(node,
                                             dot11,
                                             &tmpMapValue,
                                             ipDestAddr);

        tmpMapValue.creationTime = node->getNodeTime();
        tmpMapValue.frameQueue.push_back(frameInfo);
        OutputBufferMap.insert(std::pair<std::pair<Mac802Address,
                               TosType>,
                               MapValue>(tmpKey,tmpMapValue));
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nCanPacketBeQueuedInOutputBuffer
//  PURPOSE:     Checks if packet can be queued in output buffer
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//               UInt32 packetSize
//                  Size of the packet
//               Mac802Address nextHopAddress
//                  Nexthop address of the packet
//               TosType priority
//                  Priority of the packet
//               BOOL *isKeyAlreadyPresent
//                  Pointer that speifies the existence of the key in the
//                  output buffer map
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11nCanPacketBeQueuedInOutputBuffer(
                        MacDataDot11* dot11,
                        Mac802Address nextHopAddress,
                        TosType priority,
                        BOOL* isKeyAlreadyPresent)
{
    UInt8  acIndex = 0;
    acIndex = (UInt8)MacDot11ReturnAccessCategory(priority);
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
    outputBuffer->tmpKey.first = nextHopAddress;
    outputBuffer->tmpKey.second = priority;
    
    UInt32 numPacketsInMacQueue = 0;
    if (outputBuffer->checkAndGetKeyPosition())
    {
        *isKeyAlreadyPresent = TRUE;
        ERROR_Assert(outputBuffer->Mapitr->second.numPackets
                     <= dot11->macOutputQueueSize,
                     "Outputbuffer exceeds mac output queue");
        ERROR_Assert(outputBuffer->Mapitr->second.frameQueue.size() ==
                     outputBuffer->Mapitr->second.numPackets,
                     "Frame queue size is not equal to numpackets");
        
        numPacketsInMacQueue = outputBuffer->Mapitr->second.numPackets;
        if (dot11->isAmsduEnable)
        {
            std::pair<Mac802Address,TosType> tmpKey;
            tmpKey.first = nextHopAddress;
            tmpKey.second = priority;
            std::map<std::pair<Mac802Address,
                TosType>,MapValue>::iterator tempMapitr;
            tempMapitr = dot11->amsduBuffer->AmsduBufferMap.find(tmpKey);
            if (tempMapitr != dot11->amsduBuffer->AmsduBufferMap.end())
            {
                numPacketsInMacQueue += tempMapitr->second.numPackets;
            }
        }
        if (numPacketsInMacQueue < dot11->macOutputQueueSize)
        {
           return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        *isKeyAlreadyPresent = FALSE;
        if (dot11->isAmsduEnable)
        {
            std::pair<Mac802Address,TosType> tmpKey;
            tmpKey.first = nextHopAddress;
            tmpKey.second = priority;
            std::map<std::pair<Mac802Address,TosType>,MapValue>
                                                      ::iterator tempMapitr;
            tempMapitr = dot11->amsduBuffer->AmsduBufferMap.find(tmpKey);
            if (tempMapitr != dot11->amsduBuffer->AmsduBufferMap.end())
            {
                numPacketsInMacQueue += tempMapitr->second.numPackets;
            }
        }
        if (numPacketsInMacQueue < dot11->macOutputQueueSize)
        {
           return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nIsACsHasMessage
//  PURPOSE:     Checks if any of the access category has messages
//               data to send
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//  RETURN:      BOOL
//                  TRUE of any access category has message, FALSE otherwise
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11nIsACsHasMessage(MacDataDot11* dot11)
{
    
    int acCounter = 0;
    for (acCounter = 0;
         acCounter < DOT11e_NUMBER_OF_AC;
         acCounter++)
    {
        if (dot11->ACs[acCounter].isACHasPacket
            || dot11->ACs[acCounter].frameInfo != NULL)
        {
            return TRUE;
        }
    }
    return FALSE;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nDequeuePacketFromOutputBuffer.
//  PURPOSE:     Dequeues a single packet from output buffer
//  PARAMETERS:  Node* node
//                  Pointer to the Node
//               MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//               int acIndex
//                  Index of the access category
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator& keyItr
//                  Pointer to the Key
//               std::list<DOT11_FrameInfo*>::iterator& listItr
//                  Pointer to the list
//  RETURN:      DOT11_FrameInfo*
//                  Pointer tp the frameinfo containing the message
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
BOOL MacDot11nDequeuePacketFromOutputBuffer(Node* node,
                                 MacDataDot11* dot11,
                                 int acIndex,
                                 std::map<std::pair<Mac802Address,
                                 TosType>,
                                 MapValue>::iterator keyItr)
{
    if (keyItr->second.blockAckVrbls
        &&(keyItr->second.blockAckVrbls->state
            >= BAA_STATE_ADDBA_REQUEST_QUEUED
        && keyItr->second.blockAckVrbls->state
            <= BAA_STATE_TRANSMITTING))
    {
        return FALSE;
    }
    ERROR_Assert(keyItr->second.numPackets > 0,
                 "numpackets less than zero");
    DOT11_FrameInfo *tempFrameInfo
        = keyItr->second.frameQueue.front();
    int pktsize = DOT11nMessage_ReturnPacketSize(tempFrameInfo->msg);
    MAC_PHY_TxRxVector tempTxVector;
    tempTxVector = MacDot11nSetTxVector(
                       node,
                       dot11,
                       pktsize,
                       DOT11_QOS_DATA,
                       keyItr->first.first,
                       NULL);
    clocktype duration = PHY_GetTransmissionDuration(
                              node,
                              dot11->myMacData->phyNumber,
                              tempTxVector);

    if (acIndex > 1 && duration >= dot11->ACs[acIndex].TXOPLimit)
    {
        std::list<struct DOT11_FrameInfo*>::iterator listItr =
                            keyItr->second.frameQueue.begin();
        tempFrameInfo = (*listItr);
        MESSAGE_Free(node, tempFrameInfo->msg);
        tempFrameInfo->msg = NULL;
        MEM_free(tempFrameInfo);
        tempFrameInfo = NULL;
        keyItr->second.frameQueue.erase(listItr);
        keyItr->second.numPackets--;
        ERROR_ReportWarning("Packet bigger than TxOp. Dropping");
        MacDot11nIncrementSeqNumber(&keyItr->second.winStarts);
        keyItr->second.aggSize -= pktsize;
        listItr = keyItr->second.frameQueue.begin();
        duration = 0;
        while (listItr != keyItr->second.frameQueue.end())
        {
            tempFrameInfo = *listItr;
            pktsize = DOT11nMessage_ReturnPacketSize(tempFrameInfo->msg);
            tempTxVector = MacDot11nSetTxVector(
                                   node,
                                   dot11,
                                   pktsize,
                                   DOT11_QOS_DATA,
                                   keyItr->first.first,
                                   NULL);
            duration = PHY_GetTransmissionDuration(
                           node,
                           dot11->myMacData->phyNumber,
                           tempTxVector);
            if (duration > dot11->ACs[acIndex].TXOPLimit)
            {
                MESSAGE_Free(node, tempFrameInfo->msg);
                tempFrameInfo->msg = NULL;
                MEM_free(tempFrameInfo);
                tempFrameInfo = NULL;
                ERROR_ReportWarning("Packet bigger than TxOp. Dropping");
                keyItr->second.frameQueue.erase(listItr);
                keyItr->second.numPackets--;
                keyItr->second.aggSize -= pktsize;
                MacDot11nIncrementSeqNumber(&keyItr->second.winStarts);
                listItr = keyItr->second.frameQueue.begin();
            }
            else
            {
                // if duration is less than the transmission opportunity
                break;
            }
        }
        if (!tempFrameInfo)
        {
            ERROR_Assert(keyItr->second.numPackets == 0,
                         "numPackets still not empty");
            MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                               (UInt8)acIndex);
            return FALSE;
        }
    }
    DOT11n_FrameHdr* hdr
        = (DOT11n_FrameHdr *)MESSAGE_ReturnPacket(tempFrameInfo->msg);
    hdr->qoSControl.AckPolicy = MACDOT11N_ACK_POLICY_NA;
    if (tempFrameInfo->state == STATE_AWAITING_ACK)
    {
        tempFrameInfo->baActive = FALSE;
    }
    else
    {
        ERROR_Assert(!tempFrameInfo->baActive,
                     "tempFrameInfo!->baActive");
    }
    ERROR_Assert(keyItr->second.winEnds == MACDOT11N_INVALID_SEQ_NUM,
                 "winEnds != MACDOT11N_INVALID_SEQ_NUM");
    ERROR_Assert(keyItr->second.winSizes == 0,
                 "winSizes !=0");
    ERROR_Assert(keyItr->second.winStarts == hdr->seqNo,
                 "winStarts != hdr->seqNo");
    ERROR_Assert(dot11->ACs[acIndex].frameInfo == NULL,"AC has packet");
    ERROR_Assert(keyItr->second.aggSize >= 0,"size less than zero");
    dot11->ACs[acIndex].totalNoOfthisTypeFrameQueued++;
    tempFrameInfo->state = STATE_AWAITING_ACK;
    dot11->ACs[acIndex].frameInfo = tempFrameInfo;
    dot11->ACs[acIndex].frameToSend = tempFrameInfo->msg;
    dot11->ACs[acIndex].priority = keyItr->first.second;
    dot11->ACs[acIndex].currentNextHopAddress = keyItr->first.first;
    ERROR_Assert(MAC_IsIdenticalMac802Address(
                 &dot11->ACs[acIndex].currentNextHopAddress,
                 &tempFrameInfo->RA),"next hop is not recievers address");
    return TRUE;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nSetCurrentAC
//  PURPOSE:     Selects the next access category for sending packets
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nSetCurrentAC(MacDataDot11* dot11)
{
    clocktype TotalTime = 0;
    int acCounter = 0;
    for (acCounter = 0;
         acCounter < DOT11e_NUMBER_OF_AC;
         acCounter++)
     {
        if (dot11->ACs[acCounter].isACHasPacket
            || dot11->ACs[acCounter].frameInfo != NULL)
        {
            if (TotalTime == 0)
            {
                TotalTime=dot11->ACs[acCounter].BO
                    + dot11->ACs[acCounter].AIFS;
                dot11->currentACIndex = acCounter;
            }
            else
            {
                if (TotalTime>dot11->ACs[acCounter].BO +
                    dot11->ACs[acCounter].AIFS)
                {
                    TotalTime=dot11->ACs[acCounter].BO
                        + dot11->ACs[acCounter].AIFS;
                    dot11->currentACIndex = acCounter;
                }
            }
        }
    }
}


//--------------------------------------------------------------------------
//  NAME:        Macdot11nCancelAmsduBufferTimer
//  PURPOSE:     Cancels the amsdu buffer timer
//  PARAMETERS:  Node* node
//                  Pointer to the Node
//               MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//               Mac802Address nextHopAddress
//                  Nexthop address
//               TosType priority
//                  Priority of the packet
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void Macdot11nCancelAmsduBufferTimer(Node* node,
                            MacDataDot11* dot11,
                            Mac802Address tempNextHopAddress,
                            TosType priority)
{
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first = tempNextHopAddress;
    tmpKey.second = priority;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator Mapitr;
    Mapitr = dot11->amsduBuffer->AmsduBufferMap.find(tmpKey);
    ERROR_Assert(Mapitr != dot11->amsduBuffer->AmsduBufferMap.end(),
                 "End of Amsdu Buffer");
    ERROR_Assert(Mapitr->second.numPackets ==
                 Mapitr->second.frameQueue.size(),
                 "numpackets not equal to framequeue size");
    if (Mapitr->second.timerMsg != NULL)
    {
        MESSAGE_CancelSelfMsg(node, Mapitr->second.timerMsg);
        Mapitr->second.timerMsg = NULL;
    }
}

//--------------------------------------------------------------------------
//  NAME:        Macdot11nSetAmsduBufferTimer
//  PURPOSE:     Sets the amsdu buffer timer
//  PARAMETERS:  Node* node
//                  Pointer to the Node
//               MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//               Mac802Address tempNextHopAddress
//                  Nexthop address
//               TosType priority
//                  Priority of the packet
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void Macdot11nSetAmsduBufferTimer(Node* node,
                          MacDataDot11* dot11,
                          Mac802Address tempNextHopAddress,
                          TosType priority)
{
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first = tempNextHopAddress;
    tmpKey.second = priority;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator Mapitr;
    Mapitr = dot11->amsduBuffer->AmsduBufferMap.find(tmpKey);
    ERROR_Assert(Mapitr != dot11->amsduBuffer->AmsduBufferMap.end(),
                 "End of Amsdu Buffer");
    ERROR_Assert(Mapitr->second.numPackets ==
                 Mapitr->second.frameQueue.size(),
                 "numpackets not equal to framequeue size");
    ERROR_Assert(Mapitr->second.numPackets > 0,
                 "numpackets not exist");
    if (Mapitr->second.timerMsg == NULL)
    {
        ERROR_Assert(Mapitr->second.timerMsg == NULL,
                     "Timer Message already exist");
        Message* newMsg;
        newMsg = MESSAGE_Alloc(node,
                               MAC_LAYER, MAC_PROTOCOL_DOT11,
                       MSG_MAC_AMSDU_BUFFER_Timer);
        MESSAGE_SetInstanceId(newMsg,
            (short) dot11->myMacData->interfaceIndex);

        MESSAGE_AddInfo(node, newMsg, sizeof(timerMsg), INFO_TYPE_Dot11nTimerInfo);
        timerMsg *timer2
            = (timerMsg*)MESSAGE_ReturnInfo(newMsg, INFO_TYPE_Dot11nTimerInfo);
        timer2->addr =  Mapitr->first.first;
        timer2->priority =  Mapitr->first.second;
        timer2->seqNo = 0;
        Mapitr->second.timerMsg = newMsg;
        MESSAGE_Send(node, newMsg, dot11->amsduBufferTimerExpireInterval);
    }
}

//--------------------------------------------------------------------------
//  NAME:        Macdot11nDequeueAllPacketsFromAmsduBuffer
//  PURPOSE:     Dequeues all packets from amsdu buffer and enqueues in
//               output buffer
//  PARAMETERS:  Node* node
//                  Pointer to the Node
//               MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//               Mac802Address tempNextHopAddress
//                  Nexthop address
//               TosType priority
//                  Priority of the packet
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void Macdot11nDequeueAllPacketsFromAmsduBuffer(Node* node,
                                       MacDataDot11* dot11,
                                       Mac802Address tempNextHopAddress,
                                       TosType priority)
{
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first = tempNextHopAddress;
    tmpKey.second = priority;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = dot11->amsduBuffer->AmsduBufferMap.find(tmpKey);
    ERROR_Assert(keyItr != dot11->amsduBuffer->AmsduBufferMap.end(),
                 "End of Amsdu Buffer");
    ERROR_Assert(keyItr->second.numPackets ==
                 keyItr->second.frameQueue.size(),
                 "numpackets not equal to framequeue size");
    int acIndex = MacDot11ReturnAccessCategory(priority) ;
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
    outputBuffer->tmpKey.first = keyItr->first.first;
    outputBuffer->tmpKey.second = priority;
    BOOL isKeyAlreadyPresent = FALSE;
    if (outputBuffer->checkAndGetKeyPosition())
    {
        isKeyAlreadyPresent = TRUE;
    }
    UInt32 numPacketsInMacQueue = 0;
    numPacketsInMacQueue = keyItr->second.numPackets;
    if (isKeyAlreadyPresent)
    {
        // numPacketsInMacQueue will be the total of all packets
        // in AMSDU buffer and in the output buffer

        numPacketsInMacQueue += outputBuffer->Mapitr->second.numPackets;
    }
    ERROR_Assert(numPacketsInMacQueue <=
                 dot11->macOutputQueueSize,
                 "numPacketsInMacQueue > macOutputQueueSize");
    std::list<DOT11_FrameInfo*>::iterator listItr;
    listItr = keyItr->second.frameQueue.begin();
    while (listItr != keyItr->second.frameQueue.end())
    {
        DOT11_FrameInfo *frameInfo =
            MacDot11nRemoveReferenceOfFrameFromMap(keyItr,listItr);
        MESSAGE_AddHeader(node,
                          frameInfo->msg,
                          sizeof(DOT11n_FrameHdr),
                          TRACE_DOT11);
        MacDot11nSetFieldsInMacHeader(dot11,
              (DOT11n_FrameHdr*) MESSAGE_ReturnPacket(frameInfo->msg),
              frameInfo->RA,
              frameInfo->frameType,
              acIndex,priority, FALSE);
        dot11->numPktsDeQueuedFromAmsduBufferAsMsdu++;
        MacDot11nEnqueuePacketInOutputBuffer(node,
                                             dot11,
                                             frameInfo,
                                             priority,
                                             keyItr->first.first,
                                             isKeyAlreadyPresent);
        isKeyAlreadyPresent = FALSE;
        if (outputBuffer->checkAndGetKeyPosition())
        {
            // if key is still present reset "isKeyAlreadyPresent"
           isKeyAlreadyPresent = TRUE;
        }   
        keyItr->second.numPackets--;
        if (keyItr->second.frameQueue.size() > 0)
        {
            // If there are more packets in Queue, reset list iterator
            listItr = keyItr->second.frameQueue.begin();
        }
        else
        {
            // if queue is empty , then break
            ERROR_Assert(keyItr->second.numPackets == 0,
                         "numpackets not equal to 0");
            break;
        }
    }
}

//--------------------------------------------------------------------------
//  NAME:        Macdot11nCreateAmsdu
//  PURPOSE:     Creates an amsdu of 'numPackets' msdus
//  PARAMETERS:  Node* node
//                  Pointer to the Node
//               MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//               Mac802Address tempNextHopAddress
//                  Nexthop address
//               TosType priority
//                  Priority of the packet
//               UInt8 numPackets
//                  number of msdus in the amsdu
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void Macdot11nCreateAmsdu(Node* node,
                          MacDataDot11* dot11,
                          Mac802Address tempNextHopAddress,
                          TosType priority,
                          UInt8 numPackets)
{
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first = tempNextHopAddress;
    tmpKey.second = priority;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = dot11->amsduBuffer->AmsduBufferMap.find(tmpKey);
    ERROR_Assert(keyItr != dot11->amsduBuffer->AmsduBufferMap.end(),
                 "Amsdu Buffer end");
    ERROR_Assert(keyItr->second.numPackets
        == keyItr->second.frameQueue.size(),"numpackets ! = Queue size");
    int acIndex = MacDot11ReturnAccessCategory(priority) ;
    std::list<struct DOT11_FrameInfo*>::iterator itr
        = keyItr->second.frameQueue.begin();
    Message *ListRoot = NULL;
    Message *ItrList = NULL;
    Message *MsgList = NULL;
    Mac802Address RA;
    Mac802Address TA;
    RA = (*itr)->RA;
    TA = (*itr)->TA;
    
    int index = 0;
    for (index = 0; index < numPackets; index++)
    {
        DOT11_FrameInfo *tempFrameInfo
            = MacDot11nRemoveReferenceOfFrameFromMap(keyItr,itr);
        MESSAGE_AddHeader(node,
                          tempFrameInfo->msg,
                          sizeof(aMsduSubFrameHdr),
                          TRACE_AMSDU_SUB_HDR);
        aMsduSubFrameHdr* subFrameHdr
            = (aMsduSubFrameHdr*)MESSAGE_ReturnPacket(tempFrameInfo->msg);
        subFrameHdr->DA = tempFrameInfo->DA;
        subFrameHdr->SA = tempFrameInfo->SA;
        subFrameHdr->Length
            = (UInt16)DOT11nMessage_ReturnPacketSize(tempFrameInfo->msg);
        if (index == 0)
        {
            ListRoot = tempFrameInfo->msg;
            ItrList = ListRoot;
            MsgList = ItrList;
        }
        else
        {
            ItrList = tempFrameInfo->msg;
            ItrList->next = NULL;
            MsgList->next = ItrList;
            MsgList = MsgList->next;
        }
        MEM_free(tempFrameInfo);
        tempFrameInfo = NULL;
        keyItr->second.numPackets--;
        dot11->numMsdusSentUnderAmsdu++;
        dot11->numPktsDeQueuedFromAmsduBufferAsAmsdu++;
        if (keyItr->second.frameQueue.size() > 0)
        {
            itr = keyItr->second.frameQueue.begin();
        }
    }
    int AmsduSize;
    Message* AmsduMsg;
    AmsduMsg = MESSAGE_PackMessage(node,ListRoot,TRACE_DOT11,&AmsduSize);
    dot11->numAmsdusCreated++;

    DOT11_FrameInfo* frameInfo =
        (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
    memset(frameInfo, 0, sizeof(DOT11_FrameInfo));
    frameInfo->msg = AmsduMsg;
    frameInfo->RA = RA;
    frameInfo->TA = TA;
    frameInfo->insertTime = node->getNodeTime();
    frameInfo->isAmsdu = TRUE;
    frameInfo->frameType = DOT11_QOS_DATA;
    MESSAGE_AddHeader(node,
                      frameInfo->msg,
                      sizeof(DOT11n_FrameHdr),
                      TRACE_DOT11);
    MacDot11nSetFieldsInMacHeader(dot11,
                  (DOT11n_FrameHdr*) MESSAGE_ReturnPacket(frameInfo->msg),
                   frameInfo->RA,
                   frameInfo->frameType,
                   acIndex,priority, TRUE);
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
    outputBuffer->tmpKey.first = keyItr->first.first;
    outputBuffer->tmpKey.second = keyItr->first.second;
    BOOL isKeyAlreadyPresent = FALSE;
    if (outputBuffer->checkAndGetKeyPosition())
    {
        isKeyAlreadyPresent = TRUE;
    }
    MacDot11nEnqueuePacketInOutputBuffer(node,
                                         dot11,
                                         frameInfo,
                                         priority,
                                         keyItr->first.first,
                                         isKeyAlreadyPresent);
    if (keyItr->second.frameQueue.size() > 0)
    {
        // more packets in output buffer
        // set the timer
        Macdot11nSetAmsduBufferTimer(node,
                                     dot11,
                                     tempNextHopAddress,
                                     priority);
    }
}

//--------------------------------------------------------------------------
//  NAME:        Macdot11nCanAmsduBeCreated
//  PURPOSE:     Checks if amsdu can be created and if yes, returns the
//               number of msdus that can be aggregated in 'numPackets'
//  PARAMETERS:  Node* node
//                  Pointer to the Node
//               MacDataDot11* dot11
//                  Pointer to Dot11 data structure
//               Mac802Address tempNextHopAddress
//                  Nexthop address
//               TosType priority
//                  Priority of the packet
//               UInt8 *numPackets
//                  number of msdus in the amsdu
//  RETURN:      BOOL
//                  TRUE if amsdu can be created, FALSE otherwise
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL Macdot11nCanAmsduBeCreated(Node* node,
                                MacDataDot11* dot11,
                                Mac802Address tempNextHopAddress,
                                TosType priority,
                                UInt8 *numPackets)
{
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first = tempNextHopAddress;
    tmpKey.second = priority;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = dot11->amsduBuffer->AmsduBufferMap.find(tmpKey);
    ERROR_Assert(keyItr !=
        dot11->amsduBuffer->AmsduBufferMap.end(),
        "End of AMSDU buffer");
    ERROR_Assert(keyItr->second.numPackets
        == keyItr->second.frameQueue.size(),
        "Num packets not equal to framequeue size");
    if (DEBUG_AMSDU)
    {
        char dstAddress[MAX_STRING_LENGTH];
        MacDot11MacAddressToStr(dstAddress, &tempNextHopAddress);
        printf("%" TYPES_64BITFMT "d Node %d Checking if"
            " Amsdu can be created"
            " towards destination/priority %s/%d\n",
                node->getNodeTime(),
                node->nodeId,
                dstAddress,
                priority);
    }
    if (keyItr->second.numPackets >= MIN_MSDUS_IN_AMSDU)
    {
        // start aggregation
        UInt16 maxAmsduSize = 0;
        UInt8 minMcsIndex = 0;
        if (MacDot11IsAp(dot11))
        {
            DOT11_ApStationListItem* stationItem = NULL;
            stationItem
                = MacDot11ApStationListGetItemWithGivenAddress(
                                                   node,
                                                   dot11,
                                                   tempNextHopAddress);

            if (!stationItem || !stationItem->data->isHTEnabledSta)
            {
                return FALSE;
            }
            
            int tempMcs =
                stationItem->data->staHtCapabilityElement.
                          supportedMCSSet.mcsSet.maxMcsIdx;
            
            UInt8 maxMcs = MacDot11nGetMaximumMcs(node,
                                                  dot11,
                                                  tempMcs);
            minMcsIndex = maxMcs - 7;

            BOOL staSupportBigAmsdu =
            stationItem->data->staHtCapabilityElement.
                htCapabilitiesInfo.maxAmsduLength;
            if (staSupportBigAmsdu)
            {
                maxAmsduSize = AMSDU_SIZE_2;
            }
            else
            {
                maxAmsduSize = AMSDU_SIZE_1;
            }
       }
       else
       {
            if (dot11->stationType == DOT11_STA_IBSS)
            {
                DOT11n_IBSS_Station_Info *stationInfo
                    = MacDot11nGetIbssStationInfo(dot11, tempNextHopAddress);
                if (stationInfo->probeStatus == IBSS_PROBE_COMPLETED)
                {
                    
                    int tempMcs =
                        stationInfo->staHtCapabilityElement.
                            supportedMCSSet.mcsSet.maxMcsIdx;
                    UInt8 maxMcs = MacDot11nGetMaximumMcs(node,
                                                          dot11,
                                                          tempMcs);
                    minMcsIndex = maxMcs - 7;

                     BOOL apSupportBigAmsdu
                       = stationInfo->staHtCapabilityElement.
                                htCapabilitiesInfo.maxAmsduLength;
                    if (apSupportBigAmsdu)
                    {
                        maxAmsduSize = AMSDU_SIZE_2;
                    }
                    else
                    {
                        maxAmsduSize = AMSDU_SIZE_1;
                    }
                }
                else
                {
                    maxAmsduSize = AMSDU_SIZE_1;
                    minMcsIndex = 0;
                }
            }
            else
            {
                ERROR_Assert(dot11->associatedAP != NULL,
                    "Sta not associated");
                ERROR_Assert(dot11->associatedAP->htEnableAP,
                    " AP is not HT enabled");
                Mac802Address addr = dot11->associatedAP->bssAddr;
                ERROR_Assert(tempNextHopAddress
                    == dot11->associatedAP->bssAddr,
                    "Next hop not associated");
                
                int tempMcs =
                    dot11->associatedAP->staHtCapabilityElement.
                        supportedMCSSet.mcsSet.maxMcsIdx;

                UInt8 maxMcs = MacDot11nGetMaximumMcs(node,
                                                      dot11,
                                                      tempMcs);
                minMcsIndex = maxMcs - 7;

                BOOL apSupportBigAmsdu =
                    dot11->associatedAP->staHtCapabilityElement.
                    htCapabilitiesInfo.maxAmsduLength;
                if (apSupportBigAmsdu)
                {
                    maxAmsduSize = AMSDU_SIZE_2;
                }
                else
                {
                    maxAmsduSize = AMSDU_SIZE_1;
                }
            }
        }
        ERROR_Assert(minMcsIndex == 0
            || minMcsIndex == 8
            || minMcsIndex == 16
            || minMcsIndex == 24,
            "MCS index is not minimum as per given attenna elements");

            UInt32 pktsize = 0;
            clocktype duration = 0;
            std::list<struct DOT11_FrameInfo*>::iterator listItr =
                    keyItr->second.frameQueue.begin();
            UInt8 numPkts = 0;
            MAC_PHY_TxRxVector tempTxVector;
            tempTxVector
                    = MacDot11nSetTxVector(node,
                                           dot11,
                                           DOT11_LONG_CTRL_FRAME_SIZE,
                                           DOT11_RTS,
                                           keyItr->first.first,
                                           NULL);
            duration += PHY_GetTransmissionDuration(
                                    node, dot11->myMacData->phyNumber,
                                    tempTxVector);
            duration += dot11->sifs;
            MAC_PHY_TxRxVector tempTxVector2 = MacDot11nSetTxVector(node,
                                           dot11,
                                           DOT11_SHORT_CTRL_FRAME_SIZE,
                                           DOT11_CTS,
                                           keyItr->first.first,
                                           &tempTxVector);
            duration
                += PHY_GetTransmissionDuration(
                                       node, dot11->myMacData->phyNumber,
                                       tempTxVector2);
            duration += dot11->sifs;
            pktsize += sizeof(DOT11n_FrameHdr);
            int acIndex = MacDot11ReturnAccessCategory(priority) ;
            while ((acIndex < 2
                    || duration < dot11->ACs[acIndex].TXOPLimit)
                    && listItr != keyItr->second.frameQueue.end()
                    && pktsize < maxAmsduSize)
            {
                // Note: Padding octets for Amsdu subframe is currently
                // ignored.

                UInt32 tempPktsize
                 = DOT11nMessage_ReturnPacketSize((*listItr)->msg);
                tempPktsize += sizeof(aMsduSubFrameHdr);
                MAC_PHY_TxRxVector tempTxVector;
                tempTxVector = MacDot11nSetTxVector(node,
                                                  dot11,
                                                  pktsize + tempPktsize,
                                                  DOT11_QOS_DATA,
                                                  keyItr->first.first,
                                                  NULL);
                tempTxVector.mcs = minMcsIndex;
                duration += PHY_GetTransmissionDuration(
                                    node, dot11->myMacData->phyNumber,
                                    tempTxVector);
                tempTxVector2 = MacDot11nSetTxVector(
                                       node,
                                       dot11,
                                       DOT11_SHORT_CTRL_FRAME_SIZE,
                                       DOT11_ACK,
                                       keyItr->first.first,
                                       &tempTxVector);
                duration += PHY_GetTransmissionDuration(
                        node, dot11->myMacData->phyNumber,
                        tempTxVector2);
                 if ((acIndex < 2
                     || duration < dot11->ACs[acIndex].TXOPLimit)
                     && (pktsize + tempPktsize) < maxAmsduSize)
                 {
                     listItr++;
                     numPkts++;
                     pktsize += tempPktsize;
                 }
                 else
                 {
                     break;
                 }
            }
            *numPackets = numPkts;
            if (*numPackets <= 1)
            {
                *numPackets = 0;
                if (DEBUG_AMSDU)
                {
                    char dstAddress[MAX_STRING_LENGTH];
                    MacDot11MacAddressToStr(dstAddress,
                        &tempNextHopAddress);
                    printf("%" TYPES_64BITFMT "d Node %d Amsdu"
                        " cannot be created using mcs %d "
                        " with max amsdu size %d"
                        " towards destination/priority %s/%d\n",
                            node->getNodeTime(),
                            node->nodeId,
                            minMcsIndex,
                            maxAmsduSize,
                            dstAddress,
                            priority);
                }
                return FALSE;
            }
            if (*numPackets >= MIN_MSDUS_IN_AMSDU)
            {
                if (DEBUG_AMSDU)
                {
                    char dstAddress[MAX_STRING_LENGTH];
                    MacDot11MacAddressToStr(dstAddress, &tempNextHopAddress);
                    printf("%" TYPES_64BITFMT "d Node %d Creating Amsdu"
                        " of size %d num packets packed %d mcs %d"
                        " with max supported Amsdu size %d"
                        " towards destination/priority %s/%d\n",
                            node->getNodeTime(),
                            node->nodeId,
                            pktsize,
                            *numPackets,
                            minMcsIndex,
                            maxAmsduSize,
                            dstAddress,
                            priority);
                }
                return TRUE;
            }
            if (DEBUG_AMSDU)
            {
                char dstAddress[MAX_STRING_LENGTH];
                MacDot11MacAddressToStr(dstAddress, &tempNextHopAddress);
                printf("%" TYPES_64BITFMT "d Node %d Amsdu"
                    " cannot be created using mcs %d "
                    " with max supported Amsdu size %d"
                    " towards destination/priority %s/%d\n",
                        node->getNodeTime(),
                        node->nodeId,
                        minMcsIndex,
                        maxAmsduSize,
                        dstAddress,
                        priority);
            }
            return FALSE;
    }
    else
    {
        // dont do anything as if now
        if (DEBUG_AMSDU)
        {
            char dstAddress[MAX_STRING_LENGTH];
            MacDot11MacAddressToStr(dstAddress, &tempNextHopAddress);
            printf("%" TYPES_64BITFMT "d Node %d Amsdu"
                " cannot be created. Num packets in Amsdu buffer less"
                " than %d"
                " towards destination/priority %s/%d\n",
                    node->getNodeTime(),
                    node->nodeId,
                    MIN_MSDUS_IN_AMSDU,
                    dstAddress,
                    priority);
        }
        return FALSE;
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nEnqueuePacketInAmsduBuffer
//  PURPOSE:     Move a packet into the Stagging Buffer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Pointer to the message
//               TosType Currpriority
//                  current priority for which the message has been fetched
//               Mac802Address NextHopAddress
//                  Next hop adress of the peeked packet
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nEnqueuePacketInAmsduBuffer(Node* node,
                                         MacDataDot11* dot11,
                                         Message* tempMsg,
                                         Mac802Address tempNextHopAddress,
                                         TosType priority)
{
    DOT11_FrameInfo* frameInfo =
    (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
    memset(frameInfo, 0, sizeof(DOT11_FrameInfo));
    frameInfo->msg = tempMsg;
    frameInfo->RA = tempNextHopAddress;
    frameInfo->TA = dot11->selfAddr;
    frameInfo->SA = dot11->selfAddr;
    frameInfo->DA = dot11->ipDestAddr;
    frameInfo->insertTime = node->getNodeTime();
    frameInfo->frameType = DOT11_QOS_DATA;
    dot11->amsduBuffer->Enqueue(dot11,
                                node,
                                tempNextHopAddress,
                                (TosType)priority,
                                frameInfo);
    dot11->numPktsQueuedInAmsduBuffer++;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nClassifyPacket
//  PURPOSE:     Classifies packets from network layer into mac buffers
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nClassifyPacket(Node* node, MacDataDot11* dot11)
{
    int interfaceIndex = dot11->myMacData->interfaceIndex;
    Message* tempMsg = NULL;
    Mac802Address tempNextHopAddress;
    TosType priority = 0;
    int networkType = 0;
    BOOL isTopPacketPresent = FALSE;
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    Scheduler *schedulerPtr
        = ip->interfaceInfo[interfaceIndex]->scheduler;
     int highestPriority = schedulerPtr->numQueue() - 1;
     int currPriority = highestPriority;
     BOOL IsBroadcast = FALSE;
     BOOL dequeued = FALSE;
     BOOL pktQueuedInAmsduBuffer = FALSE;
     while (currPriority >= 0)
     {
         tempMsg = NULL;
         isTopPacketPresent = MAC_OutputQueueTopPacketForAPriority(
                                 node,
                                 interfaceIndex,
                                 (TosType) currPriority,
                                 &(tempMsg),
                                 &(tempNextHopAddress));
        if (isTopPacketPresent)
        {
            BOOL isKeyAlreadyPresent = FALSE;
            if (MacDot11IsBssStation(dot11))
            {
                if (!MAC_IsBroadcastMac802Address(&tempNextHopAddress) &&
                        tempNextHopAddress != dot11->bssAddr)
                {
                    Message* msg = MESSAGE_Duplicate(node, tempMsg);
                    MacDot11StationInformNetworkOfPktDrop(node,
                                                     dot11,
                                                     msg);
                  }
                tempNextHopAddress = dot11->bssAddr;
            }
            priority = MAC_GetPacketsPriority(node,  tempMsg);
            BOOL canBeQueued =
               MacDot11nCanPacketBeQueuedInOutputBuffer(
                                                  dot11,
                                                   tempNextHopAddress,
                                                   priority,
                                                   &isKeyAlreadyPresent);
            if (canBeQueued)
            {
                dequeued = MAC_OutputQueueDequeuePacketForAPriority(
                                                    node,
                                                    interfaceIndex,
                                                    currPriority,
                                                    &(tempMsg),
                                                    &(tempNextHopAddress),
                                                    &networkType);
                if (dequeued)
                {
                    dot11->pktsToSend++;
                    if (MacDot11IsBssStation(dot11))
                    {
                        if (!MAC_IsBroadcastMac802Address(
                           &tempNextHopAddress)
                            && tempNextHopAddress != dot11->bssAddr)
                        {
                            Message* msg = MESSAGE_Duplicate(node, tempMsg);
                            MacDot11StationInformNetworkOfPktDrop(node,
                                                                  dot11,
                                                                  msg);
                        }
                        tempNextHopAddress = dot11->bssAddr;
                    }
                    IsBroadcast
                        = MAC_IsBroadcastMac802Address(&tempNextHopAddress);
                    if (dot11->isAmsduEnable && !IsBroadcast)
                    {
                        pktQueuedInAmsduBuffer = TRUE;
                        MacDot11nEnqueuePacketInAmsduBuffer(node,
                                               dot11,
                                              tempMsg,
                                              tempNextHopAddress,
                                               priority);
                    }
                    else
                    {
                        priority = MAC_GetPacketsPriority(node,  tempMsg);
                        DOT11_FrameInfo *frameInfo
                            =(DOT11_FrameInfo*)
                                MEM_malloc(sizeof(DOT11_FrameInfo));
                         memset(frameInfo, 0, sizeof(DOT11_FrameInfo));
                        frameInfo->msg = tempMsg;
                        frameInfo->RA = tempNextHopAddress;
                        frameInfo->TA = dot11->selfAddr;
                        frameInfo->SA = dot11->selfAddr;
                        frameInfo->DA = dot11->ipDestAddr;
                        frameInfo->insertTime = node->getNodeTime();
                        frameInfo->frameType = DOT11_QOS_DATA;
                        MESSAGE_AddHeader(node,
                                      frameInfo->msg,
                                      sizeof(DOT11n_FrameHdr),
                                      TRACE_DOT11);
                        UInt8 acIndex
                            = (UInt8)MacDot11ReturnAccessCategory(priority);
                        MacDot11nSetFieldsInMacHeader(dot11,
                                      (DOT11n_FrameHdr*)
                                      MESSAGE_ReturnPacket( frameInfo->msg),
                                      frameInfo->RA,
                                      frameInfo->frameType,
                                      acIndex,
                                      priority,
                                      FALSE);
                        MacDot11nEnqueuePacketInOutputBuffer(
                                                      node,
                                                      dot11,
                                                      frameInfo,
                                                      priority,
                                                      tempNextHopAddress,
                                                      isKeyAlreadyPresent);
                    } // end of isamsdu else
                }// end of if dequeue
           
            }
            else
            {
                currPriority--;
            }
         }
        else
        {
            currPriority--;
        }
    }
    if (pktQueuedInAmsduBuffer)
    {
        std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr
            = dot11->amsduBuffer->AmsduBufferMap.begin();
        while (keyItr != dot11->amsduBuffer->AmsduBufferMap.end())
        {
            if (keyItr->second.numPackets > 0)
            {
                UInt8 numPackets = 0;
                BOOL canAmsduBeCreated =
                Macdot11nCanAmsduBeCreated(node,
                                           dot11,
                                           keyItr->first.first,
                                           keyItr->first.second,
                                           &numPackets);
                if (canAmsduBeCreated)
                {
                    ERROR_Assert(numPackets >= 2,
                        "num packets is less than 2");
                    Macdot11nCancelAmsduBufferTimer(node,
                                                    dot11,
                                                    keyItr->first.first,
                                                    keyItr->first.second);
                    Macdot11nCreateAmsdu(node,
                                         dot11,
                                         keyItr->first.first,
                                         keyItr->first.second,
                                         numPackets);
                }
                else
                {
                    if (numPackets != 0)
                    {
                        // cant send an amsdu packet
                        Macdot11nCancelAmsduBufferTimer(
                            node,
                                                        dot11,
                                                        keyItr->first.first,
                                                        keyItr->first.second);
                        Macdot11nDequeueAllPacketsFromAmsduBuffer(
                                                        node,
                                                        dot11,
                                                        keyItr->first.first,
                                                        keyItr->first.second);
                    }
                    else
                    {
                        // one packet in the amsdu buffer queue.
                        // start timer
                        Macdot11nSetAmsduBufferTimer(node,
                                                     dot11,
                                                     keyItr->first.first,
                                                     keyItr->first.second);
                    }
                }
            }
            keyItr++;
        }
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nDeleteAckedFramesFromOutputBuffer
//  PURPOSE:     Deletes the acked frame from the output buffer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator& keyItr
//                  Pointer to the Key
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nDeleteAckedFramesFromOutputBuffer(Node* node,
                                                MacDataDot11* dot11,
                                                std::map<std::pair
                                                <Mac802Address,TosType>
                                                ,MapValue>
                                                ::iterator &keyItr)
{
    // Remove acked packets in framequeue if any
    // packets with state = STATE_ACKED
    std::list<struct DOT11_FrameInfo*>::iterator listItr
        = keyItr->second.frameQueue.begin();
    while (listItr != keyItr->second.frameQueue.end()
                && (*listItr)->state == STATE_ACKED)
    {
        DOT11_FrameInfo *frameToBeFreed
            = MacDot11nRemoveReferenceOfFrameFromMap(keyItr, listItr);
        keyItr->second.aggSize
            -= DOT11nMessage_ReturnPacketSize(frameToBeFreed->msg);
        DOT11n_FrameHdr* hdr
            = (DOT11n_FrameHdr *)MESSAGE_ReturnPacket(frameToBeFreed->msg);
        ERROR_Assert(keyItr->second.winStarts == hdr->seqNo,
                    "Header seq no doesnt match");
        MESSAGE_Free(node, frameToBeFreed->msg);
        frameToBeFreed->msg = NULL;
        MEM_free(frameToBeFreed);
        frameToBeFreed = NULL;
        MacDot11nIncrementSeqNumber(&keyItr->second.winStarts);
        dot11->numPktsDequeuedFromOutputBuffer++;
        keyItr->second.numPackets--;
        if (keyItr->second.frameQueue.size() > 0)
        {
            listItr = keyItr->second.frameQueue.begin();
        }
        else
        {
            ERROR_Assert(keyItr->second.numPackets == 0,
                         "numPackets not equal to zero");
            break;
        }
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nUpdateOutputBuffer
//  PURPOSE:     Updates the output buffer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Next hop address of the packet
//               TosType priority
//                  priority of the packet
//               UInt16 seqNo
//                  SeqNo of the packet whose status has to be updated
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nUpdateOutputBuffer(Node* node,
                                 MacDataDot11* dot11,
                                 Mac802Address destAddr,
                                 TosType priority,
                                 UInt16 seqNo)
{
    UInt8 acIndex = (UInt8)MacDot11ReturnAccessCategory(priority);
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first = destAddr;
    tmpKey.second = priority;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = outputBuffer->OutputBufferMap.find(tmpKey);
    ERROR_Assert(keyItr != outputBuffer->OutputBufferMap.end(),
        "End of output buffer");
    ERROR_Assert(keyItr->second.winStarts == seqNo,
        "Sequence number doesnt match");
    std::list<struct DOT11_FrameInfo*>::iterator listItr
        = keyItr->second.frameQueue.begin();
    DOT11n_FrameHdr* hdr
        = (DOT11n_FrameHdr*)MESSAGE_ReturnPacket((*listItr)->msg);
    ERROR_Assert(hdr->seqNo == seqNo,"Header seq no doesnt match");
    ERROR_Assert((*listItr)->state == STATE_AWAITING_ACK,
        "State is not equal to STATE_AWAITING_ACK");
    keyItr->second.aggSize
        -= DOT11nMessage_ReturnPacketSize((*listItr)->msg);
    MacDot11nRemoveReferenceOfFrameFromMap(keyItr, listItr);
    MacDot11nIncrementSeqNumber(&keyItr->second.winStarts);
    dot11->numPktsDequeuedFromOutputBuffer++;
    keyItr->second.numPackets--;
    MacDot11nDeleteAckedFramesFromOutputBuffer(node,
                                              dot11,
                                              keyItr);
    MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                       acIndex);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nHandleSinglePacketDrop
//  PURPOSE:     Handles the case when an mpdu is dropped
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the mpdu
//               BOOL updateOutputBuffer
//                  Specifies if output buffer needs to be updated
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void  MacDot11nHandleSinglePacketDrop(Node* node,
                                       MacDataDot11* dot11,
                                       Message *msg,
                                       BOOL updateOutputbuffer)
{
    DOT11n_FrameHdr* hdr
        = (DOT11n_FrameHdr*)MESSAGE_ReturnPacket(msg);
    switch(hdr->frameType)
    {
        case DOT11_DATA:
        {
            // Do nothing
            break;
        }
        case DOT11_QOS_DATA:
        {
            if (updateOutputbuffer)
            {
                MacDot11nUpdateOutputBuffer(node,
                                            dot11,
                                            hdr->destAddr,
                                            hdr->qoSControl.TID,
                                            hdr->seqNo);
            }
            MESSAGE_RemoveHeader(node,
                                 msg,
                                 sizeof(DOT11n_FrameHdr),
                                 TRACE_DOT11);
            MacDot11StationInformNetworkOfPktDrop(node,
                                                  dot11,
                                                  msg);
            break;
        }
        default:
        {
            //management frame
            DOT11_FrameInfo *frameInfo =
            (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
            memset(frameInfo, 0, sizeof(DOT11_FrameInfo));
            frameInfo->msg = msg;
            frameInfo->RA = hdr->destAddr;
            frameInfo->TA = hdr->sourceAddr;
            frameInfo->SA = hdr->sourceAddr;
            frameInfo->DA = hdr->destAddr;
            frameInfo->frameType = (DOT11_MacFrameType)hdr->frameType;
            frameInfo->insertTime = node->getNodeTime();
            MacDot11StationInformManagementOfPktDrop(node,
                                                     dot11,
                                                     frameInfo);
            MESSAGE_Free(node, msg);
            msg = NULL;
            MEM_free(frameInfo);
            frameInfo = NULL;
            break;
        }
    }//switch
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nHandleAmsduPacketDrop
//  PURPOSE:     Handles the case when an amsdu is dropped
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the mpdu
//               BOOL updateOutputBuffer
//                  Specifies if output buffer needs to be updated
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void  MacDot11nHandleAmsduPacketDrop(Node* node,
                                     MacDataDot11* dot11,
                                     Message *msg,
                                     BOOL updateOutputbuffer)
{
    dot11->numAmsdusDropped++;
    ERROR_Assert(dot11->isHTEnable,
        "High throughput not Enabled");
    DOT11n_FrameHdr* hdr =(DOT11n_FrameHdr*) MESSAGE_ReturnPacket(msg);
    BOOL isAmsduMsg = hdr->qoSControl.AmsduPresent;
    ERROR_Assert(isAmsduMsg,"AMSDU Not Present");
    ERROR_Assert(hdr->frameType == DOT11_QOS_DATA,"Not Qos Data");
    Mac802Address srcAddress = hdr->sourceAddr;
    if (updateOutputbuffer)
    {
        MacDot11nUpdateOutputBuffer(node,
                                   dot11,
                                   hdr->destAddr,
                                   hdr->qoSControl.TID,
                                   hdr->seqNo);
    }
    MESSAGE_RemoveHeader(node,
                         msg,
                         sizeof(DOT11n_FrameHdr),
                         TRACE_DOT11);
    Message* listItem = MESSAGE_UnpackMessage(node, msg, TRUE, TRUE);
    while (listItem != NULL)
    {
        msg = listItem;
        MESSAGE_RemoveHeader(node,
                             msg,
                             sizeof(aMsduSubFrameHdr),
                             TRACE_AMSDU_SUB_HDR);
        listItem = listItem->next;
        MacDot11StationInformNetworkOfPktDrop(node,
                                              dot11,
                                              msg);
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nHandleAmpduPacketDrop
//  PURPOSE:     Handles the case when an ampdu is dropped
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the mpdu
//               int acIndex
//                  Index of the access category
//               BOOL dropInternalCollision
//                  Specifies if drop is due to contention between ACs
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void  MacDot11nHandleAmpduPacketDrop(Node* node,
                                     MacDataDot11* dot11,
                                     Message *msg,
                                     int acIndex,
                                     BOOL dropInternalCollision)
{
    dot11->numAmpdusDropped++;
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
    if (dropInternalCollision)
    {
        outputBuffer->tmpKey.first = dot11->ACs[acIndex].frameInfo->RA;
        outputBuffer->tmpKey.second = dot11->ACs[acIndex].priority;
    }
    else
    {
        ERROR_Assert(dot11->dot11TxFrameInfo->msg
            == dot11->ACs[acIndex].frameToSend,
            "Frame to send is not equal to dot11TxFrameInfo->msg");
        outputBuffer->tmpKey.first = dot11->dot11TxFrameInfo->RA;
        outputBuffer->tmpKey.second = dot11->ACs[acIndex].priority;
    }
    if (outputBuffer->checkAndGetKeyPosition())
    {
        ERROR_Assert(outputBuffer->Mapitr->second.winStarts
            != MACDOT11N_INVALID_SEQ_NUM,
            "Invalid sequence number");
        ERROR_Assert(outputBuffer->Mapitr->second.winEnds
            != MACDOT11N_INVALID_SEQ_NUM,
            "Invalid Seq Number");

        ERROR_Assert(outputBuffer->Mapitr->second.numPackets > 0,
            "NumPackets equal to zero");
        std::list<struct DOT11_FrameInfo*>::iterator listItr =
            outputBuffer->Mapitr->second.frameQueue.begin();
        unsigned int i = 0;
        for (i = 0;
             i < outputBuffer->Mapitr->second.winSizes;
             i++)
        {
            ERROR_Assert(listItr
                != outputBuffer->Mapitr->second.frameQueue.end(),
                "End of List");
            DOT11_FrameInfo *tempFrameInfo = *listItr;
            DOT11n_FrameHdr* hdr
                = (DOT11n_FrameHdr *)MESSAGE_ReturnPacket(tempFrameInfo->msg);
            if (i==0)
            {
                ERROR_Assert(outputBuffer->Mapitr->second.winStarts ==
                             hdr->seqNo,
                             "winstart not equal to header seq no");
            }
            outputBuffer->Mapitr->second.aggSize
                -= DOT11nMessage_ReturnPacketSize(tempFrameInfo->msg);
            outputBuffer->Mapitr->second.frameQueue.erase(listItr);
            listItr = outputBuffer->Mapitr->second.frameQueue.begin();
            if (tempFrameInfo->isAmsdu)
            {
                MacDot11nHandleAmsduPacketDrop(node,
                                               dot11,
                                               tempFrameInfo->msg,
                                               FALSE);
            }
            else
            {
                MacDot11nHandleSinglePacketDrop(node,
                                                dot11,
                                                tempFrameInfo->msg,
                                                FALSE);
            }
            MEM_free(tempFrameInfo);
            tempFrameInfo = NULL;
            dot11->numPktsDequeuedFromOutputBuffer++;
            outputBuffer->Mapitr->second.numPackets--;
        }
    }
    unsigned int index = 0;
    for (index = 0; index < outputBuffer->Mapitr->second.winSizes; index++)
    {
        MacDot11nIncrementSeqNumber(&outputBuffer->Mapitr->second.winStarts);
    }
    outputBuffer->Mapitr->second.winEnds
        = MACDOT11N_INVALID_SEQ_NUM;
    outputBuffer->Mapitr->second.winSizes = 0;
    MESSAGE_Free(node, msg);
    MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                       (UInt8)acIndex);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nResetACVariables
//  PURPOSE:     Resets the access category variables holding the message
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int acIndex
//                  Index of the access category
//               BOOL free
//                  Specifies if frameInfo needs to be free
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void
MacDot11nResetACVariables(MacDataDot11* dot11,
                          int acIndex,
                          BOOL free)
{
    dot11->ACs[acIndex].frameToSend = NULL;
    dot11->ACs[acIndex].BO = 0;
    dot11->ACs[acIndex].QSRC = 0;
    dot11->ACs[acIndex].QLRC = 0;
    dot11->ACs[acIndex].currentNextHopAddress = INVALID_802ADDRESS;
    if (free)
    {
        MEM_free(dot11->ACs[acIndex].frameInfo);
    }
    dot11->ACs[acIndex].frameInfo = NULL;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11nRetryForInternalCollision
//  PURPOSE:     Handles the case when an AC doesn't get the Txop while
//               contention between ACs that have message pending to be send
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int acIndex
//                  Index of the access category
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nRetryForInternalCollision(
    Node* node,
    MacDataDot11* dot11,
    int acIndex)
{
    ERROR_Assert(dot11->isHTEnable,"Hight throughput disabled");
    BOOL UseShortPacketRetryCounter;
    BOOL RetryTheTransmission = FALSE;

    ERROR_Assert(acIndex >= DOT11e_AC_BK,
        "MacDot11eStationRetryForInternalCollision: "
        "the received AC index is incorrect. \n");

    // Long retry count only applies to data frames greater
    // than RTS_THRESH.  RTS uses short retry count.
    // Support Fix: RTS Threshold should be applied on Mac packet

    if (dot11->ACs[acIndex].frameToSend == NULL)
    {
        ERROR_Assert(dot11->ACs[acIndex].isACHasPacket,
            "Ac is empty");
        MacDot11nSetMessageInAC(node, dot11, acIndex);
        if (dot11->ACs[acIndex].frameToSend == NULL)
        {
            MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                               (UInt8)acIndex);
            return;
        }
    }
    UseShortPacketRetryCounter
        = (DOT11nMessage_ReturnPacketSize(dot11->ACs[acIndex].frameToSend)
            <= dot11->rtsThreshold);

    // If not greater than maximum retry count allowed, retransmit frame
    if (UseShortPacketRetryCounter)
    {
        dot11->ACs[acIndex].QSRC = dot11->ACs[acIndex].QSRC + 1;
        if (dot11->ACs[acIndex].QSRC < dot11->shortRetryLimit)
        {
            RetryTheTransmission = TRUE;
        }
        else
        {
            RetryTheTransmission = FALSE;
            dot11->ACs[acIndex].QSRC = 0;
        }
    }
    else
    {
        dot11->ACs[acIndex].QLRC++;
        if (dot11->ACs[acIndex].QLRC < dot11->longRetryLimit)
        {
            RetryTheTransmission = TRUE;
        }
        else
        {
            RetryTheTransmission = FALSE;
            dot11->ACs[acIndex].QLRC = 0;
        }
    }
    if (RetryTheTransmission)
    {
        if (dot11->dot11TxFrameInfo->isAMPDU)
        {
            dot11->numAmpdusRetried++;
        }
        else if (dot11->dot11TxFrameInfo->isAmsdu)
        {
            dot11->numAmsdusRetried++;
        }
        dot11->ACs[dot11->currentACIndex].totalNoOfthisTypeFrameRetried++;
        MacDot11eStationIncreaseCWForAC(node, dot11, acIndex);
        MacDot11StationSetBackoffIfZero(node,
                                        dot11,
                                        acIndex);
    }
    else
    {
        // Exceeded maximum retry count allowed, so drop frame
        // dot11->pktsDroppedDcf++;
        // Drop frame from queue

        MacDot11Trace(node, dot11, NULL, "Drop, exceeds retransmit count");
        ERROR_Assert(dot11->txFrameInfo == NULL,"MP not supported");
        if (dot11->ACs[acIndex].frameInfo->isAMPDU)
        {
            dot11->pktsDroppedDcf++;
            MacDot11nHandleAmpduPacketDrop(node,
                                          dot11,
                                          dot11->ACs[acIndex].frameInfo->msg,
                                          acIndex,
                                          TRUE);
            dot11->ACs[acIndex].totalNoOfthisTypeFrameDeQueued++;
            MacDot11nResetACVariables(dot11, acIndex);
            MacDot11eStationResetCWForAC(node, dot11, acIndex);
            MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                               (UInt8)acIndex);
        }
        else
        {
            if (dot11->ACs[acIndex].frameInfo->baActive)
            {
                OutputBuffer *outputBuffer
                     = dot11->ACs[acIndex].outputBuffer;
                std::pair<Mac802Address,TosType> tmpKey;
                tmpKey.first = dot11->ACs[acIndex].frameInfo->RA;
                tmpKey.second = dot11->ACs[acIndex].priority;
                std::map<std::pair<Mac802Address,TosType>,
                                             MapValue>::iterator keyItr;
                keyItr = outputBuffer->OutputBufferMap.find(tmpKey);
                ERROR_Assert(keyItr
                    != outputBuffer->OutputBufferMap.end(),
                    "End of output buffer");
                dot11->ACs[acIndex].frameInfo->state = STATE_AWAITING_ACK;
                ERROR_Assert(keyItr->second.blockAckVrbls->
                    numPktsToBeSentInCurrentSession != 0,
                    "packet cannot be sent in current session");
                keyItr->second.blockAckVrbls->numPktsSent++;
                keyItr->second.blockAckVrbls->
                                    numPktsLeftToBeSentInCurrentTxop--;
                if (keyItr->second.blockAckVrbls->
                    numPktsLeftToBeSentInCurrentTxop > 0)
                {
                    keyItr->second.blockAckVrbls->
                        numPktsLeftToBeSentInCurrentTxop = 0;
                }
                else
                {
                    if (keyItr->second.blockAckVrbls->numPktsSent
                         == keyItr->second.blockAckVrbls->
                                        numPktsToBeSentInCurrentSession)
                    {
                        keyItr->second.blockAckVrbls->navDuration = 0;
                        MacDot11nSendBlockAckRequest(node,
                          dot11,
                          keyItr);
                    }
                    else
                    {
                        ERROR_Assert(
                            keyItr->second.blockAckVrbls->

                                numPktsSent
                                < keyItr->second.blockAckVrbls->
                                numPktsToBeSentInCurrentSession,
                                "packets cant be sent in current session");
                    }
                }
                MacDot11nResetACVariables(dot11, acIndex, FALSE);
                MacDot11eStationResetCWForAC(node, dot11, acIndex);
            }
            else
            {
                if (dot11->ACs[acIndex].frameInfo->isAmsdu)
                {
                    dot11->pktsDroppedDcf++;
                    MacDot11nHandleAmsduPacketDrop(
                                       node,
                                       dot11,
                                       dot11->ACs[acIndex].frameInfo->msg,
                                       TRUE);
                    dot11->ACs[acIndex].totalNoOfthisTypeFrameDeQueued++;
                }
                else
                {
                    dot11->pktsDroppedDcf++;
                    MacDot11nHandleSinglePacketDrop(
                                        node,
                                        dot11,
                                        dot11->ACs[acIndex].frameInfo->msg,
                                        TRUE);
                   dot11->ACs[acIndex].totalNoOfthisTypeFrameDeQueued++;
                }
                MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                                  (UInt8)acIndex);
                MacDot11nResetACVariables(dot11, acIndex);
                MacDot11eStationResetCWForAC(node, dot11, acIndex);
            }
        }
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nPauseOtherAcsBo
//  PURPOSE:     Pause backoff of access categories
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               clocktype backoff
//                  backoff value
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nPauseOtherAcsBo(Node* node,
                             MacDataDot11* dot11,
                             clocktype backoff)
{
    clocktype totalTime = 0;
    int acCounter = 0;
    for (acCounter = 0;
          acCounter < DOT11e_NUMBER_OF_AC;
          acCounter++){
            MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                               (UInt8)acCounter);
        if (acCounter!=dot11->currentACIndex) {
            totalTime = (dot11->ACs[dot11->currentACIndex].AIFS
                - dot11->ACs[acCounter].AIFS) + backoff;
            if (totalTime>0 && (dot11->ACs[acCounter].frameToSend != NULL
                 || dot11->ACs[acCounter].isACHasPacket))
            {
                dot11->ACs[acCounter].BO
                    = dot11->ACs[acCounter].BO - totalTime;
                if (dot11->ACs[acCounter].BO<0)
                {
                    dot11->ACs[acCounter].BO = 0;
                    MacDot11nRetryForInternalCollision(
                                node, dot11, acCounter);
                }
            }
        }
    }
} //MacDot11nPauseOtherAcsBo

//--------------------------------------------------------------------------
//  NAME:        MacDot11nHandleDIFSorEIFSTimerExpire
//  PURPOSE:     Handles difs or eifs timer expire event
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nHandleDIFSorEIFSTimerExpire(Node* node,
                                          MacDataDot11* dot11)
{
    if (MacDot11StationHasFrameToSend(dot11))
    {
        if (dot11->currentACIndex >= DOT11e_AC_BK)
        {
            MacDot11nPauseOtherAcsBo(node, dot11, 0);
        }
        MacDot11StationTransmitFrame(node, dot11);
    }
    else
    {
        MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nHandleBOTimerExpire
//  PURPOSE:     Handles backoff timer expire event
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nHandleBOTimerExpire(Node* node,
                                  MacDataDot11* dot11)
{
    if (dot11->currentACIndex >= DOT11e_AC_BK)
    {
         MacDot11nPauseOtherAcsBo(node,
                                  dot11,
                                  dot11->ACs[dot11->currentACIndex].BO);
        dot11->ACs[dot11->currentACIndex].BO = 0;
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nResetCurrentMessageVariables
//  PURPOSE:     Resets the variables that hold the message last sent
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nResetCurrentMessageVariables(
    MacDataDot11* const dot11)
{
    dot11->dot11TxFrameInfo = NULL;
    dot11->currentMessage = NULL;
    dot11->currentNextHopAddress = ANY_MAC802;
    dot11->ipNextHopAddr = ANY_MAC802;
    dot11->ipDestAddr = ANY_MAC802;
    dot11->ipSourceAddr = ANY_MAC802;
    dot11->dataRateInfo = NULL;
}// MacDot11nResetCurrentMessageVariables


//--------------------------------------------------------------------------
//  NAME:        MacDot11nRetransmit
//  PURPOSE:     Handles the case when mac is not able send a packet due to
//               non reception of cts or ack
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nRetransmit(
    Node* node,
    MacDataDot11* dot11)
{
    BOOL UseShortPacketRetryCounter;
    BOOL RetryTheTransmission = FALSE;

    ERROR_Assert(MacDot11StationHasFrameToSend(dot11),
        "MacDot11StationRetransmit: "
        "There is no message in the local buffer.\n");

    // Long retry count only applies to data frames greater
    // than RTS_THRESH.  RTS uses short retry count.
    // Support Fix: RTS Threshold should be applied on Mac packet

    UseShortPacketRetryCounter =
        ((dot11->state == DOT11_S_WFCTS) ||
         (MacDot11StationGetCurrentMessageSize(dot11) <=
          dot11->rtsThreshold));

    // If not greater than maximum retry count allowed, retransmit frame
    if (UseShortPacketRetryCounter)
    {
        dot11->SSRC++;
        if (MacDot11IsQoSEnabled(node, dot11)
                                && dot11->currentACIndex >= DOT11e_AC_BK)
        {
            dot11->ACs[dot11->currentACIndex].QSRC = dot11->SSRC;
        }
        if (dot11->SSRC < dot11->shortRetryLimit)
        {
            RetryTheTransmission = TRUE;
            if ((dot11->useDvcs) &&
                (dot11->SSRC >= dot11->directionalShortRetryLimit))
            {
                AngleOfArrivalCache_Delete(
                    &dot11->directionalInfo->angleOfArrivalCache,
                    dot11->currentNextHopAddress);
            }
        }
        else
        {
            RetryTheTransmission = FALSE;
            dot11->SSRC = 0;
            if (dot11->state == DOT11_S_WFCTS)
            {
                dot11->totalNoPacketsDroppedDueToNoCts++;
            }
        }
    }
    else
    {
        dot11->SLRC++;
        if (MacDot11IsQoSEnabled(node, dot11) && dot11->currentACIndex
            >= DOT11e_AC_BK)
        {
            dot11->ACs[dot11->currentACIndex].QLRC = dot11->SLRC;
        }
        if (dot11->SLRC < dot11->longRetryLimit)
        {
            RetryTheTransmission = TRUE;
        }
        else
        {
            RetryTheTransmission = FALSE;
            dot11->SLRC = 0;
        }
    }
    if (RetryTheTransmission)
    {
        MacDot11StationIncreaseCW(node, dot11);
        if (dot11->state == DOT11_S_WFACK)
        {
            if (dot11->dot11TxFrameInfo->isAMPDU)
            {
                dot11->numAmpdusRetried++;
            }
            else if (dot11->dot11TxFrameInfo->isAmsdu)
            {
                dot11->numAmsdusRetried++;
            }
        }
        dot11->ACs[dot11->currentACIndex].totalNoOfthisTypeFrameRetried++;
        MacDot11StationSetBackoffIfZero(node,
                                        dot11,
                                        dot11->currentACIndex);
        MacDot11StationAttemptToGoIntoWaitForDifsOrEifsState(node,
                                                             dot11);
    }
    else
    {
        // Exceeded maximum retry count allowed, so drop frame
        // Drop frame from queue

        MacDot11Trace(node, dot11, NULL, "Drop, exceeds retransmit count");
        ERROR_Assert(dot11->txFrameInfo == NULL,"MP not supported");
        ERROR_Assert(MacDot11StationHasNetworkMsgToSend(dot11),
                     "no message to send");
        if (dot11->dot11TxFrameInfo->isAMPDU)
        {
            dot11->pktsDroppedDcf++;
            MacDot11nHandleAmpduPacketDrop(node,
                                           dot11,
                                           dot11->currentMessage,
                                           dot11->currentACIndex,
                                           FALSE);
            dot11->ACs[dot11->currentACIndex].
                totalNoOfthisTypeFrameDeQueued++;
        }
        else
        {
            if (dot11->dot11TxFrameInfo->baActive)
            {
                OutputBuffer *outputBuffer
                    = dot11->ACs[dot11->currentACIndex].outputBuffer;
                std::pair<Mac802Address,TosType> tmpKey;
                tmpKey.first = dot11->dot11TxFrameInfo->RA;
                tmpKey.second = dot11->ACs[dot11->currentACIndex].priority;
                std::map<std::pair<Mac802Address,TosType>,
                    MapValue>::iterator keyItr;
                keyItr = outputBuffer->OutputBufferMap.find(tmpKey);
                dot11->dot11TxFrameInfo->state = STATE_AWAITING_ACK;
                keyItr->second.blockAckVrbls->numPktsSent++;
                keyItr->second.blockAckVrbls->
                                    numPktsLeftToBeSentInCurrentTxop--;
                if (keyItr->second.blockAckVrbls->
                    numPktsLeftToBeSentInCurrentTxop > 0)
                {
                keyItr->second.blockAckVrbls->
                                    numPktsLeftToBeSentInCurrentTxop = 0;
                }
                else
                {
                    if (keyItr->second.blockAckVrbls->numPktsSent
                         == keyItr->second.blockAckVrbls->
                                        numPktsToBeSentInCurrentSession)
                    {
                        keyItr->second.blockAckVrbls->navDuration = 0;
                        MacDot11nSendBlockAckRequest(node,
                          dot11,
                          keyItr);
                    }
                    else
                    {
                       ERROR_Assert(keyItr->second.blockAckVrbls->numPktsSent
                           < keyItr->second.blockAckVrbls->
                           numPktsToBeSentInCurrentSession,
                           "packets sent is less than allowed session");
                    }
                }
                MacDot11nResetACVariables(dot11,
                                          dot11->currentACIndex,
                                          FALSE);
                MacDot11StationResetAckVariables(dot11);
                MacDot11nResetCurrentMessageVariables(dot11);
                MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
                MacDot11StationResetCW(node, dot11);
                MacDot11StationCheckForOutgoingPacket(node, dot11, TRUE);
                return;
            }
            else
            {
                if (dot11->dot11TxFrameInfo->isAmsdu)
                {
                    dot11->pktsDroppedDcf++;
                    MacDot11nHandleAmsduPacketDrop(node,
                                                    dot11,
                                                    dot11->currentMessage,
                                                    TRUE);
                    dot11->ACs[dot11->currentACIndex].
                                           totalNoOfthisTypeFrameDeQueued++;
                }
                else
                {
                    dot11->pktsDroppedDcf++;
                    MacDot11nHandleSinglePacketDrop(node,
                                                    dot11,
                                                    dot11->currentMessage,
                                                    TRUE);
                   dot11->ACs[dot11->currentACIndex].
                                           totalNoOfthisTypeFrameDeQueued++;
                   MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                                      (UInt8)dot11->currentACIndex);
                }
            }
        }
        dot11->dot11TxFrameInfo = NULL;
        if (dot11->useDvcs)
        {
            AngleOfArrivalCache_Delete(
            &dot11->directionalInfo->angleOfArrivalCache,
            dot11->currentNextHopAddress);
        }
        MacDot11nResetACVariables(dot11, dot11->currentACIndex);
        MacDot11StationResetAckVariables(dot11);
        MacDot11nResetCurrentMessageVariables(dot11);
        MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        MacDot11StationResetCW(node, dot11);
        MacDot11StationCheckForOutgoingPacket(node, dot11, TRUE);
    }
} // MacDot11nRetransmit

//--------------------------------------------------------------------------
//  NAME:        MacDot11nHandleTimeout
//  PURPOSE:     Handles the timeout event
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nHandleTimeout(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    switch (dot11->state)
    {
        case DOT11_S_WF_DIFS_OR_EIFS:
        {
            if (dot11->BO == 0)
            {
                if (dot11->isHTEnable)
                {
                    MacDot11nHandleDIFSorEIFSTimerExpire(node,
                                                         dot11);
                }
                else
                {
                    ERROR_Assert(FALSE,"Invalid Condition");
                }
            }
            else
            {
                MacDot11StationContinueBackoff(node, dot11);
                MacDot11StationSetState(node, dot11, DOT11_S_BO);
                MacDot11StationStartTimer(node, dot11, dot11->BO);
                if (DEBUG_PS && DEBUG_PS_BO)
                {
                    MacDot11Trace(node, dot11, NULL, "BO");
                }
            }
            break;
        }
        case DOT11_S_BO:
        {
            ERROR_Assert((dot11->lastBOTimeStamp + dot11->BO)
                == node->getNodeTime(),
                "MacDot11HandleTimeout: "
                "Backoff period does not match simulation time.\n");
            if (dot11->isHTEnable)
            {
                MacDot11nHandleBOTimerExpire(node,
                                             dot11);
            }
            dot11->BO = 0;
            if (MacDot11StationHasFrameToSend(dot11))
            {
                MacDot11StationTransmitFrame(node, dot11);
            }
            else
            {
                MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            }
            break;
        }
        case DOT11_S_WFACK:
        {
            ERROR_Assert(dot11->dataRateInfo->ipAddress
                == dot11->currentNextHopAddress,
                "Address does not match");
            dot11->dataRateInfo->numAcksFailed++;
            MacDot11StationAdjustDataRateForResponseTimeout(node, dot11);
            dot11->retxDueToAckDcf++;
            MacDot11Trace(node, dot11, NULL, "Retransmit wait for ACK");
            MacDot11nRetransmit(node, dot11);
            break;
        }
        case DOT11_S_NAV_RTS_CHECK_MODE:
        case DOT11_S_WFNAV:
        {
            dot11->NAV = 0;
            if (!MacDot11HcAttempToStartCap(node,dot11))
            {
                if (MacDot11StationHasFrameToSend(dot11))
                {
                    MacDot11StationAttemptToGoIntoWaitForDifsOrEifsState(
                        node, dot11);
                }
                else
                {
                    MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
                }
            }
            else
            {
                    ERROR_Assert(FALSE,"Invalid Condition");
            }
            break;
        }
        case DOT11_S_WFCTS:
        {
            ERROR_Assert(dot11->dataRateInfo->ipAddress
                == dot11->currentNextHopAddress,
                    "Address does not match");
            dot11->dataRateInfo->numAcksFailed++;
            MacDot11StationAdjustDataRateForResponseTimeout(node, dot11);
            dot11->retxDueToCtsDcf++;
            MacDot11Trace(node, dot11, NULL, "Retransmit wait for CTS");
            MacDot11nRetransmit(node, dot11);
            break;
        }
        case DOT11_S_WFDATA:
        {
            MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
            break;
        }
        case DOT11_CFP_START:
        {
            break;
        }
        case DOT11e_WF_CAP_START:
        {
            break;
         }
        case DOT11e_CAP_START:
        {
            break;
        }
        case DOT11_S_HEADER_CHECK_MODE:
        {
            const DOT11_LongControlFrame* hdr
                =(const DOT11_LongControlFrame*)
                MESSAGE_ReturnHeader(dot11->potentialIncomingMessage, 1);
            BOOL frameHeaderHadError;
            clocktype endSignalTime;
            BOOL packetForMe =
                ((hdr->destAddr == dot11->selfAddr) ||
                 (hdr->destAddr == ANY_MAC802));
            ERROR_Assert((hdr->frameType != DOT11_CTS) &&
                         (hdr->frameType != DOT11_ACK),
                         "Frame type should not be CTS or ACK");
            PHY_TerminateCurrentReceive(
                node, dot11->myMacData->phyNumber, packetForMe,
                &frameHeaderHadError, &endSignalTime);
            if (MacDot11StationPhyStatus(node, dot11) == PHY_RECEIVING)
            {
                // Keep Receiving (wait).
                MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            }
            else
            {
                dot11->noOutgoingPacketsUntilTime = endSignalTime;
                if (DEBUG)
                {
                    char timeStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
                    if (packetForMe)
                    {
                        printf("%d : %s For Me/Error\n",
                            node->nodeId, timeStr);
                    }
                    else if (frameHeaderHadError)
                    {
                        printf("%d : %s For Other/Error\n",
                               node->nodeId, timeStr);
                    }
                    else if (dot11->currentMessage != NULL)
                    {
                        printf("%d : %s For Other/Have Message\n",
                               node->nodeId, timeStr);
                    }
                    else
                    {
                        printf("%d : %s For Other/No Message\n",
                            node->nodeId, timeStr);
                    }
                }
                if (!frameHeaderHadError)
                {
                    clocktype headerCheckDelay = 0;
                    if (dot11->isHTEnable)
                    {
                        MAC_PHY_TxRxVector rxVector;
                        PHY_GetRxVector(node,
                                        dot11->myMacData->phyNumber,
                                        rxVector);
                        rxVector.length
                            = (size_t)DOT11_SHORT_CTRL_FRAME_SIZE;
                        headerCheckDelay
                            = PHY_GetTransmissionDuration(
                                                node,
                                                dot11->myMacData->phyNumber,
                                                rxVector);
                    }

                    // Extended IFS mode if for backing off more when a
                    // collision happens and a packet is garbled.
                    // We are forcably terminating the packet and thus
                    // we should not use extended IFS delays.

                    dot11->IsInExtendedIfsMode = FALSE;

                    // It should be able to learn AOA, but temporarily
                    // disabled.

                    MacDot11ProcessNotMyFrame(
                            node,
                            dot11,
                            (MacDot11MicroToNanosecond(hdr->duration)
                                - headerCheckDelay),
                            (hdr->frameType == DOT11_RTS),
                            (hdr->frameType == DOT11_QOS_CF_POLL));
                }
                else
                {
                    // Header had error.  Assume no information received.
                }
                MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
            }
            break;
        }
        case DOT11_S_WFJOIN:
        {
            MacDot11ManagementHandleTimeout(node, dot11,msg);
            break;
        }
        case DOT11_S_WFMANAGEMENT:
        {
            dot11->dataRateInfo->numAcksFailed++;
            MacDot11StationAdjustDataRateForResponseTimeout(node, dot11);
            dot11->retxDueToAckDcf++;
            MacDot11Trace(node, dot11, NULL,
                "Retransmit wait for management frame");
            MacDot11nRetransmit(node, dot11);
            break;
        }
        case DOT11_S_WFBEACON:
        {
            ERROR_Assert(MacDot11IsAp(dot11),
                         "MacDot11HandleTimeout: Is not AP");
            MacDot11ApAttemptToTransmitBeacon(node, dot11);
            break;
        }
        case DOT11_S_WFIBSSBEACON:
        {
            break;
        }
        case DOT11_S_WFIBSSJITTER:
        {
            break;
        }
        case DOT11_S_WFPSPOLL_ACK:
        {
            break;
        }
        default:
        {
            printf("MAC_DOT11: Node %u got unknown state type %d\n",
                   node->nodeId, dot11->state);
            ERROR_ReportError("MacDot11HandleTimeout: "
                "Timeout in unexpected state.\n");
        }
    }
}//MacDot11HandleTimeout//

//--------------------------------------------------------------------------
//  NAME:        MacDot11nStaMoveAPacketFromTheMgmtQueueToTheLocalBuffer
//  PURPOSE:     Moves a packet from management queue to appropriater access
//               category
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11nStaMoveAPacketFromTheMgmtQueueToTheLocalBuffer(
        Node* node,
        MacDataDot11* dot11)
{
    int currentACIndex = 0;
    if (dot11->isQosEnabled)
    {
        currentACIndex = DOT11e_AC_VO;
    }
    if (dot11->ACs[currentACIndex].frameToSend != NULL)
    {
        return FALSE;
    }
    BOOL dequeued = FALSE;
    dot11->managementFrameDequeued = FALSE;
    DOT11_FrameInfo* tempFrameInfo = NULL;
    dequeued = MacDot11MgmtQueue_PeekPacket(node, dot11, &(tempFrameInfo));
    if (dequeued == FALSE)
    {
        return FALSE;
    }
    if (tempFrameInfo!= NULL)
    {
        if (MacDot11IsBssStation(dot11)
        && dot11->bssAddr!= dot11->selfAddr)
        {
            tempFrameInfo->RA = dot11->bssAddr;
        }
        dot11->ACs[currentACIndex].frameInfo = tempFrameInfo;
        dot11->ACs[currentACIndex].frameToSend = tempFrameInfo->msg;
        dot11->ACs[currentACIndex].priority = 6;

        //Reset AC veriables
        dot11->ACs[currentACIndex].priority = 6;
        dot11->ACs[currentACIndex].totalNoOfthisTypeFrameQueued++;
        dot11->ACs[currentACIndex].QSRC = 0;
        dot11->ACs[currentACIndex].QLRC = 0;
        if (dot11->ACs[currentACIndex].CW == 0)
        {
            dot11->ACs[currentACIndex].CW = dot11->ACs[currentACIndex].cwMin;
        }
        MacDot11StationSetBackoffIfZero(node,
                                        dot11,
                                        currentACIndex);
        MacDot11MgmtQueue_DequeuePacket(node, dot11);
    }
    else
    {
        return FALSE;
    }
    return TRUE;
} //MacDot11StaMoveAPacketFromTheMgmtQueueToTheLocalBuffer

//--------------------------------------------------------------------------
//  NAME:        MacDot11nCancelInputBufferTimer
//  PURPOSE:     Cancels the input buffer timer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator& keyItr
//                  Pointer to the Key
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nCancelInputBufferTimer(Node* node,
                                     std::map<std::pair<Mac802Address,
                                     TosType>,
                                     MapValue>::iterator keyItr)
{
    if (keyItr->second.timerMsg)
    {
        MESSAGE_CancelSelfMsg(node, keyItr->second.timerMsg);
        keyItr->second.timerMsg = NULL;
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nStartInputBufferTimer
//  PURPOSE:     Starts the input buffer timer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator& keyItr
//                  Pointer to the Key
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nStartInputBufferTimer(Node* node,
                                    MacDataDot11* dot11,
                                    std::map<std::pair<Mac802Address,
                                    TosType>,
                                    MapValue>::iterator keyItr)
{
    ERROR_Assert(keyItr->second.numPackets > 0,
        "num packets equal to 0");
    ERROR_Assert(!keyItr->second.timerMsg,
        "timer doesnt exist");
    Message* newMsg;
    newMsg =
        MESSAGE_Alloc(node, MAC_LAYER, MAC_PROTOCOL_DOT11, MSG_MAC_INPUT_BUFFER_Timer);
    MESSAGE_SetInstanceId(newMsg, (short) dot11->myMacData->interfaceIndex);
    MESSAGE_AddInfo(node,
                    newMsg,
                    sizeof(timerMsg), INFO_TYPE_Dot11nTimerInfo);
    timerMsg *timer2 = (timerMsg*)MESSAGE_ReturnInfo(newMsg, INFO_TYPE_Dot11nTimerInfo);
    timer2->addr =  keyItr->first.first;
    timer2->priority =  keyItr->first.second;
    timer2->seqNo = keyItr->second.winStarts;
    newMsg->originatingNodeId = node->nodeId;
    keyItr->second.timerMsg = newMsg;
    MESSAGE_Send(node, newMsg, dot11->inputBufferTimerExpireInterval);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nHandleInputBufferTimer
//  PURPOSE:     Handles the input buffer timeout event
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the timer message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nHandleInputBufferTimer(Node* node,
                                     MacDataDot11* dot11,
                                     Message *msg)
{
    timerMsg *timer2 = (timerMsg*)MESSAGE_ReturnInfo(msg, INFO_TYPE_Dot11nTimerInfo);
    InputBuffer *inputBuffer = dot11->inputBuffer;
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first = timer2->addr;
    tmpKey.second = timer2->priority;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = inputBuffer->InputBufferMap.find(tmpKey);
    ERROR_Assert(keyItr != inputBuffer->InputBufferMap.end(),
        "End of inputbuffer");
    ERROR_Assert(keyItr->second.timerMsg == msg,
        "invalid timer message");
    keyItr->second.timerMsg = NULL;
    std::list<struct DOT11_FrameInfo*>::iterator listItr
        = keyItr->second.frameQueue.begin();
    DOT11n_FrameHdr* hdr
         = (DOT11n_FrameHdr *)MESSAGE_ReturnPacket((*listItr)->msg);
    do
    {
        keyItr->second.winStarts = hdr ->seqNo;
        MacDot11nIncrementSeqNumber(&keyItr->second.winStarts);
        DOT11_FrameInfo *frameInfo = (*listItr);
        MacDot11nHandleDequeuePacketFromInputBuffer(node, dot11, frameInfo);
        keyItr->second.frameQueue.erase(listItr);
        dot11->numPktsDequeuedFromInputBuffer++;
        keyItr->second.numPackets--;
        if (keyItr->second.frameQueue.size() > 0)
        {
            listItr = keyItr->second.frameQueue.begin();
            hdr = (DOT11n_FrameHdr *)MESSAGE_ReturnPacket((*listItr)->msg);
        }
        else
        {
            ERROR_Assert(keyItr->second.numPackets == 0,
                "numPackets greater than zero");
            break;
        }
    }while (listItr != keyItr->second.frameQueue.end()
            && keyItr->second.winStarts == hdr->seqNo);
    if (keyItr->second.numPackets > 0 && !keyItr->second.timerMsg)
    {
        MacDot11nStartInputBufferTimer(node, dot11, keyItr);
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nHandleAmsduBufferTimer
//  PURPOSE:     Handles the amsdu buffer timeout event
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the timer message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nHandleAmsduBufferTimer(Node* node,
                                     MacDataDot11* dot11,
                                     Message *msg)
{
    timerMsg *timer2 = (timerMsg*)MESSAGE_ReturnInfo(msg, INFO_TYPE_Dot11nTimerInfo);
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first = timer2->addr;
    tmpKey.second = timer2->priority;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = dot11->amsduBuffer->AmsduBufferMap.find(tmpKey);
    ERROR_Assert(keyItr != dot11->amsduBuffer->AmsduBufferMap.end(),
        "End of Amsdu buffer");
    ERROR_Assert(keyItr->second.timerMsg == msg,
        "invalid timer message");
    keyItr->second.timerMsg = NULL;
    UInt8 numPackets = 0;
    BOOL canAmsduBeCreated = Macdot11nCanAmsduBeCreated(node,
                                                        dot11,
                                                        timer2->addr,
                                                        timer2->priority,
                                                        &numPackets);
    if (canAmsduBeCreated)
    {
        Macdot11nCreateAmsdu(node,
                            dot11,
                            timer2->addr,
                            timer2->priority,
                            numPackets);
    }
    else
    {
            // cant send an amsdu packet
        Macdot11nDequeueAllPacketsFromAmsduBuffer(node,
                                                  dot11,
                                                  timer2->addr,
                                                  timer2->priority);
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nCancelBapTimer
//  PURPOSE:     Handles the block ack policy currently active timer
//  PARAMETERS:  UInt16 *timerSeqNo
//                  Pointer to the timer sequence number
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void  MacDot11nCancelBapTimer(UInt16 *timerSeqNo)
{
    (*timerSeqNo)++;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nSetBapTimer
//  PURPOSE:     Starts the block ack policy timer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               UInt16 *timerSeqNo
//                  Pointer to the timer sequence number
//               TosType priority
//                  priority
//               clocktype delay
//                  timer delay
//               BOOL initiator
//                  Specifies if timer is to be set at a sender ends or
//                  receivers
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void  MacDot11nSetBapTimer(Node* node,
                         MacDataDot11* dot11,
                         UInt16 *timerSeqNo,
                         Mac802Address destAddr,
                         TosType priority,
                         clocktype delay,
                         BOOL initiator)
{
    (*timerSeqNo)++;
    Message* newMsg;
    newMsg = MESSAGE_Alloc(node, MAC_LAYER, MAC_PROTOCOL_DOT11,
                   MSG_MAC_BAA_Timer);
    MESSAGE_SetInstanceId(newMsg, (short) dot11->myMacData->interfaceIndex);
    MESSAGE_AddInfo(node, newMsg, sizeof(timerMsg), INFO_TYPE_Dot11nTimerInfo);
    timerMsg *timer2 = (timerMsg*)MESSAGE_ReturnInfo(newMsg, INFO_TYPE_Dot11nTimerInfo);
    timer2->addr =  destAddr;
    timer2->priority =  priority;
    timer2->seqNo = *timerSeqNo;
    timer2->initiator = initiator;
    MESSAGE_Send(node,
                 newMsg,
                 delay);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessAddbaResponse
//  PURPOSE:     Process an addba reponse frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_Frame* rxFrame
//                  Pointer to the addba frame
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nProcessAddbaResponse(Node* node,
                                  MacDataDot11* dot11,
                                  DOT11_Frame* rxFrame)
{
    DOT11n_ADDBAResponseFrame *resFrame
        = (DOT11n_ADDBAResponseFrame*)MESSAGE_ReturnPacket(rxFrame);
    TosType priority =  resFrame->blockAckParams.TID;
    UInt8 acIndex = (UInt8)MacDot11ReturnAccessCategory(priority);
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first =resFrame->sourceAddr;
    tmpKey.second = priority;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = outputBuffer->OutputBufferMap.find(tmpKey);
    ERROR_Assert(keyItr != outputBuffer->OutputBufferMap.end(),
        "End of Output buffer");
    switch(keyItr->second.blockAckVrbls->state)
    {
        case BAA_STATE_WF_ADDBA_RESPONSE:
        {
            ERROR_Assert(keyItr->second.blockAckVrbls->startingSeqNo
                == MACDOT11N_INVALID_SEQ_NUM,
                "invalid sequence number");
            MacDot11nCancelBapTimer(&keyItr->second.blockAckVrbls->
                                    baTimerSequenceNumber);
            if (resFrame->statusCode == SUCCESSFUL)
            {
                keyItr->second.blockAckVrbls->state = BAA_STATE_IDLE;
                keyItr->second.blockAckVrbls->numPktsNegotiated =
                    resFrame->blockAckParams.BufferSize;
                keyItr->second.blockAckVrbls->numPktsSent = 0;
                keyItr->second.blockAckVrbls->
                    numPktsLeftToBeSentInCurrentTxop = 0;
                MacDot11nSetBapTimer(node,
                                    dot11,
                                    &keyItr->second.blockAckVrbls->
                                    baTimerSequenceNumber,
                                    tmpKey.first,
                                    tmpKey.second,
                                    dot11->blockAckPolicyTimeout,
                                    TRUE);
            }
            else
            {
                keyItr->second.blockAckVrbls->state
                    = BAA_STATE_WF_RETRY_TIMER_EXPIRE;
                keyItr->second.blockAckVrbls->numPktsNegotiated = 0;
                MacDot11nSetBapTimer(node,
                                     dot11,
                                     &keyItr->second.blockAckVrbls->
                                     baTimerSequenceNumber,
                                     tmpKey.first,
                                     tmpKey.second,
                                     MACDOT11N_BAP_REINITIATE_TIMEOUT,
                                     TRUE);
            }
            break;
        }
        default:
        {
            // Not waiting for any addba response
            // Currently Ignore
            // send DELBA if required.
            
            break;
        }
    }
    MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                       acIndex);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessAddbaRequest
//  PURPOSE:     Process an addba request frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_Frame* rxFrame
//                  Pointer to the addba request frame
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nProcessAddbaRequest(Node* node,
                                 MacDataDot11* dot11,
                                 DOT11_Frame* rxFrame)
{
    DOT11n_ADDBARequestFrame* reqFrame
        = (DOT11n_ADDBARequestFrame*)MESSAGE_ReturnPacket(rxFrame);

    InputBuffer *inputBuffer = dot11->inputBuffer;
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first =reqFrame->sourceAddr;
    tmpKey.second = reqFrame->blockAckParams.TID;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = inputBuffer->InputBufferMap.find(tmpKey);
    if (keyItr == inputBuffer->InputBufferMap.end())
    {
         MapValue tmpMapValue;
         memset(&tmpMapValue, 0, sizeof(MapValue));
         tmpMapValue.winEnds = MACDOT11N_INVALID_SEQ_NUM;
         tmpMapValue.creationTime = node->getNodeTime();
         inputBuffer->InputBufferMap.insert(std::pair<std::pair<Mac802Address,
                            TosType>,
                            MapValue>(tmpKey,tmpMapValue));
         keyItr = inputBuffer->InputBufferMap.find(tmpKey);
    }
    ERROR_Assert(keyItr != inputBuffer->InputBufferMap.end(),
        "End of input buffer");

    ERROR_Assert(dot11->enableImmediateBAAgreement,
        "dot11->enableImmediateBAAgreement is True");
    if (!reqFrame->blockAckParams.BAPolicy)
    {
        ERROR_Assert(dot11->enableDelayedBAAgreement,
            "dot11->enableDelayedBAAgreement is True");
    }

    // dequeue all packets from input queue as reception of
    // ADDBS request means there are no packets at the source
    // that will be resent.

    if (keyItr->second.numPackets > 0)
    {
       std::list<struct DOT11_FrameInfo*>::iterator listItr
                = keyItr->second.frameQueue.begin();
       do
        {
            DOT11_FrameInfo *frameInfo = (*listItr);
            MacDot11nHandleDequeuePacketFromInputBuffer(node, dot11, frameInfo);
            keyItr->second.frameQueue.erase(listItr);
            dot11->numPktsDequeuedFromInputBuffer++;
            keyItr->second.numPackets--;
            MacDot11nCancelInputBufferTimer(node, keyItr);
            if (keyItr->second.frameQueue.size() > 0)
            {
                listItr = keyItr->second.frameQueue.begin();
            }
            else
            {
                break;
            }
        }while (listItr != keyItr->second.frameQueue.end());
    }

    BOOL reject = FALSE;
    UInt32 numBuffersLeft = dot11->macOutputQueueSize
                                                - keyItr->second.numPackets;

    if (numBuffersLeft < BLOCK_ACK_POLICY_THRESHOLD)
    {
        reject = TRUE;
    }
    UInt32 numBuffers = MIN(numBuffersLeft,
                                   reqFrame->blockAckParams.BufferSize);

    if (!keyItr->second.blockAckVrbls)
    {
          if (MacDot11IsAp(dot11))
          {
               DOT11_ApStationListItem* stationItem = NULL;
               stationItem =
                       MacDot11ApStationListGetItemWithGivenAddress(
                                                       node,
                                                       dot11,
                                                         tmpKey.first);
               if (stationItem
                   && (stationItem->data->staCapInfo.ImmidiateBlockACK
                   || stationItem->data->staCapInfo.DelayedBlockACK))
               {
                   keyItr->second.blockAckVrbls
                            = (DOT11n_BlockAckAggrementVrbls*)
                            MEM_malloc(sizeof(DOT11n_BlockAckAggrementVrbls));
                            memset(keyItr->second.blockAckVrbls,
                                 0,
                                 sizeof(DOT11n_BlockAckAggrementVrbls));
               }
           }
           else
           {
               if (dot11->stationType == DOT11_STA_IBSS)
               {
                    keyItr->second.blockAckVrbls
                        = (DOT11n_BlockAckAggrementVrbls*)
                            MEM_malloc(sizeof(DOT11n_BlockAckAggrementVrbls));
                            memset(keyItr->second.blockAckVrbls,
                                   0,
                                   sizeof(DOT11n_BlockAckAggrementVrbls));
               }
               else
               {
                    if ((dot11->associatedAP->apCapInfo.ImmidiateBlockACK
                        || dot11->associatedAP->apCapInfo.DelayedBlockACK))
                    {
                       keyItr->second.blockAckVrbls
                           = (DOT11n_BlockAckAggrementVrbls*)
                           MEM_malloc(sizeof(DOT11n_BlockAckAggrementVrbls));
                       memset(keyItr->second.blockAckVrbls,
                                     0,
                                     sizeof(DOT11n_BlockAckAggrementVrbls));
                    }
               }
           }
    }

    if (keyItr->second.blockAckVrbls->state
            != BAA_STATE_DISABLED)
    {
        return;
    }

    if (!reject)
    {
          keyItr->second.blockAckVrbls->startingSeqNo
              = reqFrame->startingSeqControl.SeqNo;
          keyItr->second.blockAckVrbls->numPktsNegotiated
              = numBuffers;
          keyItr->second.blockAckVrbls->state
                                    = BAA_STATE_RECEIVER_WF_BA_SETUP;
          keyItr->second.blockAckVrbls->role = ROLE_RECEIVER;
          if (dot11->enableDelayedBAAgreement)
          {
            keyItr->second.blockAckVrbls->type = BA_DELAYED;
          }
          else
          {
            keyItr->second.blockAckVrbls->type = BA_IMMEDIATE;
          }
    }
    else
    {
       keyItr->second.winStartr = MACDOT11N_INVALID_SEQ_NUM;
       keyItr->second.winEndr = MACDOT11N_INVALID_SEQ_NUM;
       keyItr->second.winSizer = 0;
       keyItr->second.winStarts
                = reqFrame->startingSeqControl.SeqNo;
       keyItr->second.winEnds = keyItr->second.winStarts;
       keyItr->second.winSizes = 64;
       keyItr->second.blockAckVrbls->state = BAA_STATE_RECEIVER_WF_BA_SETUP;
       keyItr->second.blockAckVrbls->role = ROLE_NONE;
       keyItr->second.blockAckVrbls->type = BA_NONE;
    }

    Message *msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(DOT11n_ADDBAResponseFrame),
                        TRACE_DOT11);

    //prepare addba response
    DOT11n_ADDBAResponseFrame *addbaFrame =
          (DOT11n_ADDBAResponseFrame*) MESSAGE_ReturnPacket(msg);
    memset(addbaFrame, 0, sizeof(DOT11n_ADDBAResponseFrame));

    addbaFrame->frameType = DOT11_ACTION;
    addbaFrame->frameFlags |= DOT11_TO_DS;
    addbaFrame->duration = 0;
    addbaFrame->destAddr = keyItr->first.first;
    addbaFrame->sourceAddr = dot11->selfAddr;
    addbaFrame->fragId = 0;
    addbaFrame->seqNo = 0;
    addbaFrame->category = CATEGORY_BLOCK_ACK;
    addbaFrame->action = DOT11n_ADDBA_Response;
    addbaFrame->statusCode = reject? FAILURE : SUCCESSFUL;
    addbaFrame->blockAckParams.AMSDUSupported = dot11->isAmsduEnable;
    addbaFrame->blockAckParams.BAPolicy
                                = dot11->enableDelayedBAAgreement? 0 : 1;
    addbaFrame->blockAckParams.TID = keyItr->first.second;
    addbaFrame->blockAckParams.BufferSize = numBuffers;
    addbaFrame->blockAckTimeout
                    = MacDot11ClocktypeToTUs( dot11->blockAckPolicyTimeout);

    DOT11_FrameInfo *frameInfo =
            (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
     memset(frameInfo, 0, sizeof(DOT11_FrameInfo));
     frameInfo->msg = msg;
     frameInfo->RA = addbaFrame->destAddr;
     frameInfo->TA = addbaFrame->sourceAddr;
     frameInfo->SA = addbaFrame->sourceAddr;
     frameInfo->DA = addbaFrame->destAddr;
     frameInfo->frameType = (DOT11_MacFrameType)addbaFrame->frameType;
     frameInfo->insertTime = node->getNodeTime();

    MacDot11nMgmtQueue_EnqueuePacket(node, dot11, frameInfo);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nResetbap
//  PURPOSE:     Starts the input buffer timer
//  PARAMETERS:  std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator& keyItr
//                  Pointer to the Key
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nResetbap(std::map<std::pair<Mac802Address,
                        TosType>,
                         MapValue>::iterator keyItr)
{

    keyItr->second.blockAckVrbls->startingSeqNo
                                = MACDOT11N_INVALID_SEQ_NUM;
    keyItr->second.blockAckVrbls->numPktsNegotiated
                                = 0;
    keyItr->second.blockAckVrbls->numPktsSent
                                = 0;
    keyItr->second.blockAckVrbls->numPktsLeftToBeSentInCurrentTxop
                            = 0;
    keyItr->second.blockAckVrbls->numPktsToBeSentInCurrentSession
                            = 0;
    keyItr->second.blockAckVrbls->baTimerSequenceNumber++;
    keyItr->second.blockAckVrbls->state = BAA_STATE_DISABLED;
    keyItr->second.blockAckVrbls->navDuration = 0;
    keyItr->second.blockAckVrbls->role = ROLE_NONE;
    keyItr->second.BAPActiveAtReceiver = FALSE;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessDelba
//  PURPOSE:     Process delba frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_Frame* rxFrame
//                  Pointer to the delba frame
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nProcessDelba(Node* node,
                          MacDataDot11* dot11,
                          DOT11_Frame* rxFrame)
{
       DOT11n_DELBAFrame *delbaFrame =
              (DOT11n_DELBAFrame*) MESSAGE_ReturnPacket(rxFrame);
        if (delbaFrame->delbaParams.initiator)
        {
           // reset at the receivers end
            DOT11n_DELBAFrame *delbaFrame =
              (DOT11n_DELBAFrame*) MESSAGE_ReturnPacket(rxFrame);
           InputBuffer *inputBuffer = dot11->inputBuffer;
           std::pair<Mac802Address,TosType> tmpKey;
           tmpKey.first = delbaFrame->sourceAddr;
           tmpKey.second = delbaFrame->delbaParams.TID;
           std::map<std::pair<Mac802Address,TosType>,MapValue>
                                                      ::iterator keyItr;
           keyItr = inputBuffer->InputBufferMap.find(tmpKey);
           ERROR_Assert(keyItr != inputBuffer->InputBufferMap.end(),
               "End of input buffer");
           if (keyItr->second.blockAckVrbls->state
                                    == BAA_STATE_RECEIVING)
           {
               keyItr->second.winStartr = MACDOT11N_INVALID_SEQ_NUM;
               keyItr->second.winEndr = MACDOT11N_INVALID_SEQ_NUM;
               keyItr->second.winSizer = 0;
               keyItr->second.winEnds = keyItr->second.winStarts;
               keyItr->second.winSizes = 64;
               keyItr->second.blockAckVrbls->state = BAA_STATE_DISABLED;
               keyItr->second.blockAckVrbls->role = ROLE_NONE;
               keyItr->second.blockAckVrbls->type = BA_NONE;
               keyItr->second.BAPActiveAtReceiver = FALSE;
               keyItr->second.BABitmap = 0;
               MacDot11nCancelBapTimer(&keyItr->second.blockAckVrbls->
                                        baTimerSequenceNumber);
           }
        }
        else
        {
            TosType priority = delbaFrame->delbaParams.TID;
            UInt8 acIndex = (UInt8)MacDot11ReturnAccessCategory(priority);
            OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
            std::pair<Mac802Address,TosType> tmpKey;
            tmpKey.first = delbaFrame->sourceAddr;
            tmpKey.second = priority;
            std::map<std::pair<Mac802Address,TosType>,MapValue>
                                                        ::iterator keyItr;
            keyItr = outputBuffer->OutputBufferMap.find(tmpKey);
            if (keyItr->second.blockAckVrbls->state == BAA_STATE_IDLE
                || (keyItr->second.blockAckVrbls->state
                                    == BAA_STATE_WF_ADDBA_RESPONSE
                && keyItr->second.blockAckVrbls->state
                                    <= BAA_STATE_TRANSMITTING))
           {

                std::list<struct DOT11_FrameInfo*>::iterator listItr
                                        = keyItr->second.frameQueue.begin();
                while (listItr !=  keyItr->second.frameQueue.end()
                        && ((*listItr)->state == STATE_AWAITING_ACK
                          ||(*listItr)->state == STATE_ACKED))
                {
                    DOT11_FrameInfo *tempFrameInfo = *listItr;
                    if (dot11->ACs[acIndex].frameInfo
                        && dot11->ACs[acIndex].frameInfo == tempFrameInfo)
                    {
                        if (dot11->dot11TxFrameInfo
                            && dot11->dot11TxFrameInfo == tempFrameInfo)
                        {
                            MacDot11nResetCurrentMessageVariables(dot11);
                            MacDot11StationResetAckVariables(dot11);
                        }
                        MacDot11nResetACVariables(dot11, acIndex, FALSE);
                        keyItr->second.aggSize
                            -= DOT11nMessage_ReturnPacketSize(
                                                        tempFrameInfo->msg);
                        keyItr->second.frameQueue.erase(listItr);
                        MESSAGE_Free(node, tempFrameInfo->msg);
                        tempFrameInfo->msg = NULL;

                        MEM_free(tempFrameInfo);
                        tempFrameInfo = NULL;

                        dot11->numPktsDequeuedFromOutputBuffer++;
                        keyItr->second.numPackets--;

                        MacDot11nIncrementSeqNumber(&keyItr->second.winStarts);
                    }
                    else
                    {
                        keyItr->second.aggSize
                            -= DOT11nMessage_ReturnPacketSize(
                                                        tempFrameInfo->msg);
                          keyItr->second.frameQueue.erase(listItr);
                        MESSAGE_Free(node, tempFrameInfo->msg);
                        tempFrameInfo->msg = NULL;

                        MEM_free(tempFrameInfo);
                        tempFrameInfo = NULL;

                        dot11->numPktsDequeuedFromOutputBuffer++;
                        keyItr->second.numPackets--;

                        MacDot11nIncrementSeqNumber(&keyItr->second.winStarts);
                    }
                    if (keyItr->second.frameQueue.size() > 0)
                    {
                        listItr = keyItr->second.frameQueue.begin();
                    }
                    else
                    {
                        break;
                    }
                }
                keyItr->second.winEnds
                    = MACDOT11N_INVALID_SEQ_NUM;
                keyItr->second.winSizes = 0;
                MacDot11nResetbap(keyItr);
                MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                                   acIndex);
            }
            else
            {
            }
        }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessActionFrame
//  PURPOSE:     Process action frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_Frame* rxFrame
//                  Pointer to the action frame
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nProcessActionFrame(Node* node,
                                MacDataDot11* dot11,
                                DOT11_Frame* rxFrame)
{
    DOT11n_ADDBARequestFrame* reqFrame
            = (DOT11n_ADDBARequestFrame*)MESSAGE_ReturnPacket(rxFrame);

      DOT11_ManagementVars * mngmtVars =
            (DOT11_ManagementVars*) dot11->mngmtVars;

    switch(reqFrame->action)
    {
        case DOT11n_ADDBA_Request:
        {
            MacDot11nProcessAddbaRequest(node,
                                        dot11,
                                        rxFrame);
            mngmtVars->ADDBSRequestReceived++;
            break;
        }
        case DOT11n_ADDBA_Response:
        {
            MacDot11nProcessAddbaResponse(node,
                                        dot11,
                                        rxFrame);
            mngmtVars->ADDBSResponseReceived++;
            break;
        }
        case DOT11n_DELBA:
        {
            MacDot11nProcessDelba(node,
                                dot11,
                                rxFrame);
             mngmtVars->DELBARequestReceived++;
            break;
        }
        default:
        {
            ERROR_Assert(FALSE,"Default case");
        }
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nManagementProcessFrame
//  PURPOSE:     Process a management frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_Frame* rxFrame
//                  Pointer to the management frame
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nManagementProcessFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{
    DOT11_FrameHdr* hdr =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(rxFrame);

    // Stop retransmit timer
    if (hdr->destAddr != ANY_MAC802) {

        MacDot11StationCancelTimer(
        node,
        dot11);
    }

    DOT11_ManagementVars * mngmtVars =
        (DOT11_ManagementVars*) dot11->mngmtVars;

    switch (hdr->frameType) {

        case DOT11_ASSOC_REQ:
        {
            dot11->mgmtSendResponse = TRUE;
            mngmtVars->assocReqReceived++;
            MacDot11ManagementProcessAssociateRequest(node, dot11, rxFrame);
            break;
        }
        case DOT11_ASSOC_RESP:
        {
            if (mngmtVars->state == DOT11_S_M_WFASSOC)
            {
                dot11->mgmtSendResponse = FALSE;
                mngmtVars->assocRespReceived++;
                MacDot11ManagementProcessAssociateResponse(node,
                                                           dot11,
                                                           rxFrame);
            }
            break;
        }
        case DOT11_REASSOC_REQ:
        {
            dot11->mgmtSendResponse = TRUE;
            mngmtVars->reassocReqReceived++;
            MacDot11ManagementProcessReassociateRequest(node, dot11, rxFrame);
            break;
        }
        case DOT11_REASSOC_RESP:
        {
            if (mngmtVars->state == DOT11_S_M_WFREASSOC)
            {
                mngmtVars->reassocRespReceived++;
                MacDot11ManagementProcessReassociateResponse(node,
                                                             dot11,
                                                             rxFrame);
            }
            break;
        }
        case DOT11_PROBE_REQ:
        {
            // dot11n:The additional check for broadcast messages is added
            // in case of IBSS stations to avoid IBSS stations to respond to
            // broadcast probe request generated in a scenario where IBSS
            // and infrastructure mode coexists. The statistic
            // 'ProbReqReceived' is still updated as its a valid packet
            // received though no response will be generated for this probe
            // request.

            mngmtVars->ProbReqReceived++;
            if (MacDot11IsAp(dot11)
                || (dot11->stationType == DOT11_STA_IBSS
                    && hdr->destAddr != ANY_MAC802))
            {
                dot11->mgmtSendResponse = TRUE;
                MacDot11ManagementProcessProbeRequest(node, dot11, rxFrame);
            }
            break;
        }
        case DOT11_PROBE_RESP:
        {
            mngmtVars->ProbRespReceived++;
            MacDot11ManagementProcessProbeResponse(node, dot11, rxFrame);
            break;
        }
        case DOT11_BEACON:
        {
            MacDot11ManagementProcessBeacon(node, dot11, rxFrame);
            break;
        }
//--------------------DOT11e-End-Updates---------------------------------//
//---------------------------Power-Save-Mode-Updates---------------------//
        case DOT11_ATIM:
        {
            break;
        }
//---------------------------Power-Save-Mode-End-Updates-----------------//
        case DOT11_DISASSOCIATION:
        {
            ERROR_ReportError("MacDot11ManagementProcessFrame:"
                              " Disassociation not implemented.\n");
            break;
        }
        case DOT11_AUTHENTICATION:
        {
            if (((!MacDot11IsAp(dot11)&& mngmtVars->state == DOT11_S_M_WFAUTH))
                ||(MacDot11IsAp(dot11)))
            {
                if (MacDot11IsAp(dot11))
                {
                    mngmtVars->AuthReqReceived++;
                }
                else
                {
                    mngmtVars->AuthRespReceived++;
                }
                MacDot11ManagementProcessAuthenticate(node, dot11, rxFrame);
            }
            break;
        }
        case DOT11_DEAUTHENTICATION:
        {
            ERROR_ReportError("MacDot11ManagementProcessFrame:"
                              " Deauthentication not implemented.\n");
            break;
        }
        case DOT11_ACTION:
        {
            MacDot11nProcessActionFrame(node, dot11, rxFrame);
            break;
        }
        default:
        {
            char errString[MAX_STRING_LENGTH];

            sprintf(errString,
                "MacDot11ManagemantProcessFrame: "
                "Received unknown management frame type %d\n",
                hdr->frameType);
            ERROR_ReportError(errString);
            break;
        }

    }
}// MacDot11nProcessManagemantFrame

//--------------------------------------------------------------------------
//  NAME:        MacDot11nHandOffUnicast
//  PURPOSE:     Send a received unicast packet to the network layer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Pointer to the msg
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nHandOffUnicast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    ERROR_Assert(dot11->isHTEnable,"HT Disabled");
    DOT11n_FrameHdr* dot11nHdr  =
            (DOT11n_FrameHdr*) MESSAGE_ReturnPacket(msg);

    BOOL isAmsduMsg = dot11nHdr->qoSControl.AmsduPresent;

    ERROR_Assert(dot11nHdr->frameType == DOT11_QOS_DATA,
        "Not QOS Data");
    Mac802Address srcAddress = dot11nHdr->sourceAddr;
    MESSAGE_RemoveHeader(node,
                       msg,
                       sizeof(DOT11n_FrameHdr),
                       TRACE_DOT11);

    if (isAmsduMsg)
    {
        dot11->numAmsdusReceived++;
        Message* listItem = MESSAGE_UnpackMessage(node, msg, TRUE, TRUE);

        while (listItem != NULL)
        {
            msg = listItem;
            MESSAGE_RemoveHeader(node,
                             msg,
                             sizeof(aMsduSubFrameHdr),
                             TRACE_AMSDU_SUB_HDR);

            listItem = listItem->next;
            MAC_HandOffSuccessfullyReceivedPacket(node,
                         dot11->myMacData->interfaceIndex, msg, &srcAddress);
        }
    }
    else
    {
       MAC_HandOffSuccessfullyReceivedPacket(node,
                         dot11->myMacData->interfaceIndex, msg, &srcAddress);
    }
    dot11->totalNoOfQoSDataFrameReceived++;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nHandleDequeuePacketFromInputBuffer
//  PURPOSE:     Handles a packet dequeued from input buffer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_FrameInfo *frameInfo
//                  Pointer to frameinfo conatining the message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nHandleDequeuePacketFromInputBuffer(Node* node,
                                         MacDataDot11* dot11,
                                         DOT11_FrameInfo *frameInfo)
{
      Message *msg = frameInfo->msg;
      MEM_free(frameInfo);
      frameInfo = NULL;

     DOT11_ShortControlFrame* hdr =
        (DOT11_ShortControlFrame*) MESSAGE_ReturnPacket(msg);

    if (hdr->frameType == DOT11_QOS_DATA)
    {
        MacDot11nHandOffUnicast(node, dot11, msg);
            dot11->unicastPacketsGotDcf++;
      }
    else if (MacDot11IsManagementFrame(msg))
    {
         MacDot11nManagementProcessFrame(
            node,
            dot11,
            msg);

        // Update stats
        DOT11_ManagementVars * mngmtVars =
            (DOT11_ManagementVars*) dot11->mngmtVars;

        mngmtVars->managementPacketsGot++;

        // Free received message
        MESSAGE_Free(node,msg);
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nUpdateScoreBoardContext
//  PURPOSE:     Update the score board for received mpdu
//  PARAMETERS:  std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator keyItr
//                  Pointer to the key
//               UInt16 seqNo
//                  Sequence number of mpdu
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11nUpdateScoreBoardContext(std::map<std::pair<Mac802Address,
                                      TosType>,
                                      MapValue>::iterator keyItr,
                                      UInt16 seqNo)
{
    if (!keyItr->second.blockAckVrbls
        || keyItr->second.blockAckVrbls->state
             != BAA_STATE_RECEIVING)
    {
        return FALSE;
    }
    UInt8 bitCount = 0;
    UInt64 tempBitMap = 0;
    BOOL seqNoInRange = FALSE;
    BOOL seqNoAdvance = FALSE;
    UInt16 tempSeqNo = keyItr->second.winStartr;
    UInt32 i =0;
    for (i =0; i < keyItr->second.winSizer; i++)
    {
        if (tempSeqNo == seqNo)
        {
            seqNoInRange = TRUE;
            break;
        }
        MacDot11nIncrementSeqNumber(&tempSeqNo);
        bitCount++;
    }

    if (seqNoInRange)
    {
        tempBitMap = (UInt64)pow(2.0, bitCount);
        keyItr->second.BABitmap |= tempBitMap;
    }
    else
    {
        bitCount = 0;
        UInt16 seqNoUpperLimit = keyItr->second.winStartr;
        
        int index = 0;
        for (index =0; index < 2048 - 1; index++)
        {
             MacDot11nIncrementSeqNumber(&seqNoUpperLimit);
        }
        tempSeqNo = keyItr->second.winEndr;

        while (tempSeqNo != seqNoUpperLimit)
        {
          if (tempSeqNo == seqNo)
          {
             seqNoAdvance = TRUE;
             break;
          }
          MacDot11nIncrementSeqNumber(&tempSeqNo);
        }

        if (seqNoAdvance)
        {
            UInt16 newWinStartr = seqNo;
            
            UInt32 interator = 0;
            for (interator =0;
                 interator < keyItr->second.winSizer - 1;
                 interator++)
            {
                 MacDot11nDecrementSeqNumber(&newWinStartr);
            }

            UInt16 oldWinStartr = keyItr->second.winStartr;
           do
            {
                keyItr->second.BABitmap >>= 1;
                MacDot11nIncrementSeqNumber(&oldWinStartr);
            } while (oldWinStartr != newWinStartr);
            
            keyItr->second.winStartr = newWinStartr;
            keyItr->second.winEndr = seqNo;
            bitCount = 0;
            tempSeqNo = keyItr->second.winStartr;
            BOOL bitSet = FALSE;
            
            UInt32 tempIndex = 0;
            for (tempIndex =0;
                 tempIndex < keyItr->second.winSizer;
                 tempIndex++)
            {
                if (tempSeqNo == seqNo)
                {
                    tempBitMap = (UInt64)pow(2.0, bitCount);
                    keyItr->second.BABitmap |= tempBitMap;
                    bitSet = TRUE;
                    break;
                }
                MacDot11nIncrementSeqNumber(&tempSeqNo);
                bitCount++;
            }
        }
        else
        {
            return FALSE;
        }
    }
    return TRUE;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nUpdateBapAtReceiversEnd
//  PURPOSE:     Update the block ack policy at receivers end
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *frame
//                  Pointer to the frame
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nUpdateBapAtReceiversEnd(Node* node,
                                  MacDataDot11* dot11,
                                  Message *frame)
{
    DOT11n_FrameHdr* hdr =
                (DOT11n_FrameHdr*)MESSAGE_ReturnPacket(frame);
    InputBuffer *inputBuffer = dot11->inputBuffer;
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first =hdr->sourceAddr;
    tmpKey.second = hdr->qoSControl.TID;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = inputBuffer->InputBufferMap.find(tmpKey);
    ERROR_Assert(keyItr != inputBuffer->InputBufferMap.end(),
        "End of input buffer");

    MacDot11nSetBapTimer(node,
                        dot11,
                        &keyItr->second.blockAckVrbls->
                        baTimerSequenceNumber,
                        tmpKey.first,
                        tmpKey.second,
                        dot11->blockAckPolicyTimeout,
                        FALSE);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nSendDelba
//  PURPOSE:     Sends delba request
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator keyItr
//                  Pointer to the key
//               int acIndex
//                  Index of the access category
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nSendDelba(Node* node,
                        MacDataDot11* dot11,
                        std::map<std::pair<Mac802Address,
                        TosType>,
                         MapValue>::iterator keyItr,
                         BOOL initiator,
                         int acIndex)
{
    Message *msg = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(DOT11n_ADDBARequestFrame),
                        TRACE_DOT11);

     DOT11n_DELBAFrame *delbaFrame =
          (DOT11n_DELBAFrame*) MESSAGE_ReturnPacket(msg);
    memset(delbaFrame, 0, sizeof(DOT11n_DELBAFrame));

    delbaFrame->frameType = DOT11_ACTION;
    delbaFrame->frameFlags |= DOT11_TO_DS;
    delbaFrame->destAddr = keyItr->first.first;
    delbaFrame->sourceAddr = dot11->selfAddr;
    delbaFrame->fragId = 0;
    delbaFrame->seqNo = 0;
    delbaFrame->category = CATEGORY_BLOCK_ACK;
    delbaFrame->action = DOT11n_DELBA;
     delbaFrame->reason = TIMEOUT;
    delbaFrame->delbaParams.initiator = initiator;
    delbaFrame->delbaParams.TID = keyItr->first.second;

    // duration will be sent while transmitting this frame
    delbaFrame->duration = 0;
     DOT11_FrameInfo *frameInfo =
            (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
     memset(frameInfo, 0, sizeof(DOT11_FrameInfo));
     frameInfo->msg = msg;
     frameInfo->RA = delbaFrame->destAddr;
     frameInfo->TA = delbaFrame->sourceAddr;
     frameInfo->SA = delbaFrame->sourceAddr;
     frameInfo->DA = delbaFrame->destAddr;
     frameInfo->frameType = (DOT11_MacFrameType)delbaFrame->frameType;
     frameInfo->insertTime = node->getNodeTime();
    keyItr->second.blockAckVrbls->state = BAA_STATE_DELBA_QUEUED;
    
    if (initiator)
    {
        MacDot11nSetMoreFramesFieldForAnAC(dot11, (UInt8)acIndex);
    }
    MacDot11nMgmtQueue_EnqueuePacket(node, dot11, frameInfo);
}



//--------------------------------------------------------------------------
//  NAME:        MacDot11nUpdateReorderingBuffer
//  PURPOSE:     Update reordering buffer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_FrameInfo *frameInfo
//                  Pointer to the frameinfo
//               UInt16 seqNoOfArrivedPacket
//                  Sequence number of arrived packet
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator& keyItr
//                  Pointer to the key
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nUpdateReorderingBuffer(Node* node,
                                     MacDataDot11* dot11,
                                     DOT11_FrameInfo *frameInfo,
                                     UInt16 seqNoOfArrivedPacket,
                                     std::map<std::pair<Mac802Address,
                                     TosType>,
                                     MapValue>::iterator &keyItr)
{
    BOOL process = FALSE;
    std::list<struct DOT11_FrameInfo*>::iterator listItr;
    UInt16 tempSeqNo = keyItr->second.winStarts;
    
    unsigned int index = 0;
    for (index =0;
         index < keyItr->second.winSizes;
         index++)
    {
        if (tempSeqNo == seqNoOfArrivedPacket)
        {
            // seq no. in reception window enqueue
            process = TRUE;
            break;
        }
        MacDot11nIncrementSeqNumber(&tempSeqNo);
     }
    if (process)
    {
        listItr = keyItr->second.frameQueue.begin();
        if (listItr != keyItr->second.frameQueue.end())
        {
              DOT11n_FrameHdr* hdr =
            (DOT11n_FrameHdr *)MESSAGE_ReturnPacket((*listItr)->msg);
            while (seqNoOfArrivedPacket > hdr->seqNo)
            {
                listItr++;
                if (listItr != keyItr->second.frameQueue.end())
                {
                    hdr =
                    (DOT11n_FrameHdr *)MESSAGE_ReturnPacket((*listItr)->msg);
                }
                else
                {
                    break;
                }
            }
            keyItr->second.frameQueue.insert(listItr, frameInfo);
        }
        else
        {
            keyItr->second.frameQueue.push_back(frameInfo);
        }
        keyItr->second.numPackets++;
        dot11->numPktsQueuedInInputBuffer++;

        // checkdequeue
        listItr = keyItr->second.frameQueue.begin();
        do
         {
              DOT11n_FrameHdr* hdr =
                (DOT11n_FrameHdr *)MESSAGE_ReturnPacket((*listItr)->msg);
             if (keyItr->second.winStarts == hdr->seqNo)
             {
                DOT11_FrameInfo *frameInfo = (*listItr);
                MacDot11nHandleDequeuePacketFromInputBuffer(node,
                                                            dot11,
                                                            frameInfo);
                keyItr->second.frameQueue.erase(listItr);
                dot11->numPktsDequeuedFromInputBuffer++;
                keyItr->second.numPackets--;
                MacDot11nIncrementSeqNumber(&keyItr->second.winStarts);
                UInt16 endSeqNo = keyItr->second.winStarts;
                
                unsigned int tempindex = 0;
                for (tempindex = 0;
                     tempindex < keyItr->second.winSizes - 1;
                     tempindex++)
                {
                    MacDot11nIncrementSeqNumber(&endSeqNo);
                }
                keyItr->second.winEnds = endSeqNo;
                MacDot11nCancelInputBufferTimer(node, keyItr);
                if (keyItr->second.frameQueue.size() > 0)
                {
                    listItr = keyItr->second.frameQueue.begin();
                }
                else
                {
                    ERROR_Assert(keyItr->second.numPackets == 0,
                        "numpacket > zero");
                    break;
                }
             }
             else
             {
                // start Input buffer timer
                if (!keyItr->second.timerMsg)
                {
                    MacDot11nStartInputBufferTimer(node, dot11, keyItr);
                }
                break;
             }
        }while (listItr != keyItr->second.frameQueue.end());
    }
    else
    {
        UInt16 seqNoUpperLimit = keyItr->second.winStarts;
        
        int tempItr = 0;
        for (tempItr =0; tempItr < 2048 - 1; tempItr++)
        {
             MacDot11nIncrementSeqNumber(&seqNoUpperLimit);
        }
        UInt16 tempSeqNo = keyItr->second.winEnds;
        while (tempSeqNo != seqNoUpperLimit)
        {
          if (tempSeqNo == seqNoOfArrivedPacket)
          {
             process = TRUE;
             break;
          }
          MacDot11nIncrementSeqNumber(&tempSeqNo);
        }
        if (process)
        {
            // enqueue
            listItr = keyItr->second.frameQueue.begin();
            if (listItr != keyItr->second.frameQueue.end())
            {
                  DOT11n_FrameHdr* hdr =
                (DOT11n_FrameHdr *)MESSAGE_ReturnPacket((*listItr)->msg);
                while (seqNoOfArrivedPacket > hdr->seqNo)
                {
                    listItr++;
                    if (listItr != keyItr->second.frameQueue.end())
                    {
                        hdr = (DOT11n_FrameHdr *)
                                MESSAGE_ReturnPacket((*listItr)->msg);
                    }
                    else
                    {
                        break;
                    }
                }
                keyItr->second.frameQueue.insert(listItr, frameInfo);
            }
            else
            {
                keyItr->second.frameQueue.push_back(frameInfo);
            }
            keyItr->second.numPackets++;
            dot11->numPktsQueuedInInputBuffer++;

            // update reordering buffer
            UInt16 tempEndSeqNo = seqNoOfArrivedPacket;
            UInt16 tempWinStarts = tempEndSeqNo;
            unsigned int i=0;
            for (i=0; i < keyItr->second.winSizes - 1; i++)
            {
                MacDot11nDecrementSeqNumber(&tempWinStarts);
            }

            // dequeue if required
            listItr = keyItr->second.frameQueue.begin();
            do
             {
                  DOT11n_FrameHdr* hdr = (DOT11n_FrameHdr *)
                                    MESSAGE_ReturnPacket((*listItr)->msg);
                 if (hdr ->seqNo < tempWinStarts)
                 {
                    DOT11_FrameInfo *frameInfo = (*listItr);
                    MacDot11nHandleDequeuePacketFromInputBuffer(node,
                                                         dot11,
                                                         frameInfo);
                    keyItr->second.frameQueue.erase(listItr);
                    dot11->numPktsDequeuedFromInputBuffer++;
                    keyItr->second.numPackets--;
                    MacDot11nCancelInputBufferTimer(node,keyItr);
                    if (keyItr->second.frameQueue.size() > 0)
                    {
                        listItr = keyItr->second.frameQueue.begin();
                    }
                    else
                    {
                        break;
                    }
                 }
                 else
                 {
                    break;
                 }
            }while (listItr != keyItr->second.frameQueue.end());

            keyItr->second.winStarts = tempWinStarts;
            listItr = keyItr->second.frameQueue.begin();
            if (listItr != keyItr->second.frameQueue.end())
            {
                do
                {
                    DOT11n_FrameHdr* hdr =
                    (DOT11n_FrameHdr *)MESSAGE_ReturnPacket((*listItr)->msg);
                    if (keyItr->second.winStarts == hdr ->seqNo)
                    {
                        DOT11_FrameInfo *frameInfo = (*listItr);
                        MacDot11nHandleDequeuePacketFromInputBuffer(node,
                                                            dot11,
                                                            frameInfo);
                        keyItr->second.frameQueue.erase(listItr);
                        dot11->numPktsDequeuedFromInputBuffer++;
                        keyItr->second.numPackets--;
                        MacDot11nIncrementSeqNumber(&keyItr->second.winStarts);
                        MacDot11nCancelInputBufferTimer(node, keyItr);
                        if (keyItr->second.frameQueue.size() > 0)
                        {
                            listItr = keyItr->second.frameQueue.begin();
                        }
                        else
                        {
                            ERROR_Assert(keyItr->second.numPackets == 0,
                                "num packets greater than 0");
                            break;
                        }
                    }
                    else
                    {
                        // start Input buffer timer
                        MacDot11nCancelInputBufferTimer(node,keyItr);
                        MacDot11nStartInputBufferTimer(node, dot11, keyItr);
                        break;
                    }
                }while (listItr != keyItr->second.frameQueue.end());
            }
            tempSeqNo = keyItr->second.winStarts;
            for (unsigned int i=0; i < keyItr->second.winSizes - 1; i++)
            {
                MacDot11nIncrementSeqNumber(&tempSeqNo);
            }
            keyItr->second.winEnds = tempSeqNo;
        }
        else
        {
            // ignore the received packet
            dot11->numPktsDroppedDueToSeqMismatch++;
            MESSAGE_Free(node, frameInfo->msg);
            frameInfo->msg = NULL;
            MEM_free(frameInfo);
            frameInfo = NULL;
        }
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessReceivedPacket
//  PURPOSE:     Process received packet
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *frame
//                  Pointer to the received frame
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nProcessReceivedPacket(Node* node,
                                  MacDataDot11* dot11,
                                  Message *frame)
{
    DOT11n_FrameHdr* hdr = (DOT11n_FrameHdr*)MESSAGE_ReturnPacket(frame);
    InputBuffer *inputBuffer = dot11->inputBuffer;
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first =hdr->sourceAddr;
    tmpKey.second = hdr->qoSControl.TID;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = inputBuffer->InputBufferMap.find(tmpKey);

    if (hdr->qoSControl.AckPolicy == MACDOT11N_ACK_POLICY_BA)
    {
       ERROR_Assert(keyItr != inputBuffer->InputBufferMap.end(),
           "End of input buffer");
    }

    if (keyItr == inputBuffer->InputBufferMap.end())
    {
        MapValue tmpMapValue;
        tmpMapValue.winEnds = 63;
        tmpMapValue.winSizes = 64;
        tmpMapValue.creationTime = node->getNodeTime();
        inputBuffer->InputBufferMap.insert(std::pair<std::pair<Mac802Address,
                        TosType>,
                        MapValue>(tmpKey,tmpMapValue));
    }
    keyItr = inputBuffer->InputBufferMap.find(tmpKey);
    if (keyItr->second.numPackets < dot11->macOutputQueueSize)
    {
        BOOL baUpdated = FALSE;
        if (keyItr->second.blockAckVrbls
            && keyItr->second.blockAckVrbls->state == BAA_STATE_RECEIVING)
        {
            if (hdr->qoSControl.AckPolicy == MACDOT11N_ACK_POLICY_BA)
            {
                if (keyItr->second.blockAckVrbls->blockAckSent)
                {
                    keyItr->second.blockAckVrbls->blockAckSent = FALSE;
                    keyItr->second.BABitmap = 0;
                }
            }
             baUpdated =
                MacDot11nUpdateScoreBoardContext(keyItr,
                                                hdr->seqNo);
            if (!baUpdated)
            {
                // got to decide if reordering buffer gets access to the packet
            }
            if (hdr->qoSControl.AckPolicy == MACDOT11N_ACK_POLICY_BA)
            {
                MacDot11nUpdateBapAtReceiversEnd(node, dot11,frame);
                dot11->totalNoPacketsReceivedAckPolicyBA++;
            }
        }
        else
        {
            // no ba policy exists. dont do anything here
        }
    }
    else
    {
        //with ack policy as block ack, there must be room for packets as
        //the size has already been negotiated during ADDBA request/response
        MESSAGE_Free(node, frame);
        frame = NULL;
        return;
    }
    UInt16 seqNoOfArrivedPacket = hdr->seqNo;
    if (keyItr->second.numPackets < dot11->macOutputQueueSize)
    {
        DOT11_FrameInfo *frameInfo =
                (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
        memset(frameInfo, 0, sizeof(DOT11_FrameInfo));
        frameInfo->msg = frame;
        frameInfo->RA = dot11->selfAddr;
        frameInfo->TA = hdr->sourceAddr;
        frameInfo->insertTime =
                                node->getNodeTime();
        frameInfo->frameType = DOT11_QOS_DATA;

        if (keyItr->second.blockAckVrbls &&
                keyItr->second.blockAckVrbls->state
                  == BAA_STATE_RECEIVING)
        {
            MacDot11nUpdateReorderingBuffer(node,
                                            dot11,
                                            frameInfo,
                                            seqNoOfArrivedPacket,
                                            keyItr);
        }
        else
        {
            if (keyItr->second.blockAckVrbls)
            {
                if (keyItr->second.blockAckVrbls->state != BAA_STATE_RECEIVING)
                {
                    if (hdr->qoSControl.AckPolicy == MACDOT11N_ACK_POLICY_BA)
                    {
                        if (keyItr->second.blockAckVrbls->state
                                                != BAA_STATE_DELBA_QUEUED)
                        {
                            // send delba request if its already not been sent
                            MacDot11nSendDelba(node,
                                               dot11,
                                               keyItr,
                                               FALSE,
                                               DOT11e_INVALID_AC);
                        }
                        MESSAGE_Free(node, frameInfo->msg);
                        frameInfo->msg = NULL;
                        MEM_free(frameInfo);
                        frameInfo = NULL;
                    }
                    else
                    {
                          MacDot11nUpdateReorderingBuffer(node,
                                                        dot11,
                                                        frameInfo,
                                                        seqNoOfArrivedPacket,
                                                        keyItr);

                    }
                }
            }
            else
            {
                // Ampdu & Mpdu processing
                 MacDot11nUpdateReorderingBuffer(node,
                                                 dot11,
                                                 frameInfo,
                                                 seqNoOfArrivedPacket,
                                                 keyItr);
            }
        }
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessDataFrame
//  PURPOSE:     Process data frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *frame
//                  Pointer to the received frame
//               RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nProcessDataFrame(Node* node,
                               MacDataDot11* dot11,
                               Message *frame)
{
    Mac802Address sourceNodeAddress =
        ((DOT11_FrameHdr*)MESSAGE_ReturnPacket(frame))
        ->sourceAddr;

     if (MacDot11IsAp(dot11))
     {
       DOT11_ApStationListItem* stationItem =
       MacDot11ApStationListGetItemWithGivenAddress(
            node,
            dot11,
            sourceNodeAddress);
        if (stationItem) {

                stationItem->data->LastFrameReceivedTime =
                                              node->getNodeTime();
        }
     }

    if (dot11->useDvcs) {
        BOOL nodeIsInCache;

        MacDot11StationUpdateDirectionCacheWithCurrentSignal(
            node, dot11, sourceNodeAddress,
            &nodeIsInCache);
    }

    DOT11n_FrameHdr* hdr = (DOT11n_FrameHdr*)MESSAGE_ReturnPacket(frame);
    Mac802Address sourceAddr = hdr->sourceAddr;

     if ((dot11->state != DOT11_S_WFDATA) &&
        (MacDot11IsWaitingForResponseState(dot11->state)))
        {
            MacDot11Trace(node, dot11, NULL,
                "Drop, waiting for non-data response");
            dot11->numPktsDroppedDueToInvalidState++;
            MESSAGE_Free(node, frame);
            return;
         }


    MacDot11Trace(node, dot11, frame, "Receive");

     if (hdr->destAddr == ANY_MAC802)
     {
         MacDot11StationHandOffSuccessfullyReceivedBroadcast(
                node, dot11, frame);
          dot11->broadcastPacketsGotDcf++;
     }
     else
     {
        ERROR_Assert(hdr->frameType == DOT11_QOS_DATA,
            "Not QOS Data");
        MacDot11nProcessReceivedPacket(node, dot11, frame);
     }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessManagementFrame
//  PURPOSE:     Process management frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the msg
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nProcessManagementFrame(Node* node,
                                     MacDataDot11* dot11,
                                     Message* msg)
{
    ERROR_Assert(MacDot11IsManagementFrame(msg),"Not Managment Frame");

      if (dot11->useDvcs) {
            BOOL nodeIsInCache;
            Mac802Address sourceNodeAddress =
                    ((DOT11_FrameHdr*)MESSAGE_ReturnPacket(msg))
                    ->sourceAddr;

            MacDot11StationUpdateDirectionCacheWithCurrentSignal(
                node,
                dot11,
                sourceNodeAddress,
                &nodeIsInCache);

        }
        dot11->mgmtSendResponse = FALSE;

    DOT11n_FrameHdr* hdr=
        (DOT11n_FrameHdr*)MESSAGE_ReturnPacket(msg);

    if (MacDot11IsAp(dot11))
    {

        DOT11_ApStationListItem* stationItem =
        MacDot11ApStationListGetItemWithGivenAddress(
            node,
            dot11,
            hdr->sourceAddr);
        if (stationItem) {

            stationItem->data->LastFrameReceivedTime =
                             node->getNodeTime();
        }
    }
    Mac802Address sourceAddr = hdr->sourceAddr;
    DOT11_FrameInfo *frameInfo =
                (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
    memset(frameInfo, 0, sizeof(DOT11_FrameInfo));
     frameInfo->msg = msg;
     frameInfo->RA = dot11->selfAddr;
     frameInfo->TA = sourceAddr;
    frameInfo->insertTime = node->getNodeTime();
    frameInfo->frameType = DOT11_QOS_DATA;
    MacDot11nHandleDequeuePacketFromInputBuffer(node, dot11, frameInfo);
 }

//--------------------------------------------------------------------------
//  NAME:        MacDot11nCalculateMaxAmpduLength
//  PURPOSE:     Calculate maximum ampdu length
//  PARAMETERS:  int ampduLengthExponent
//                  ampdu length exponent
//  RETURN:      UInt16
//                  Ampdu length in Octets
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
UInt16 MacDot11nCalculateMaxAmpduLength(int ampduLengthExponent)
{
    int pwrOfTwo = 13 + ampduLengthExponent;
    double maxAmpduLength = pow(2.0, pwrOfTwo) - 1;
    return (UInt16)maxAmpduLength;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nCanAmpduBeCreated
//  PURPOSE:     Check if ampdu can be created
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int acIndex
//                  Index of access category
//               UInt8 *numPackets
//                  num mpdus in an ampdu
//  RETURN:      BOOL
//                  TRUE if ampdu can be created, FALSE otherwise
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11nCanAmpduBeCreated(Node* node,
                                       MacDataDot11* dot11,
                                       int acIndex,
                                       UInt8 *numPackets)
{
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator tempIterator;
    tempIterator = outputBuffer->roundRobinKeyItr;
    tempIterator++;
    if (tempIterator == outputBuffer->OutputBufferMap.end())
    {
       tempIterator = outputBuffer->OutputBufferMap.begin();
    }
    int count = outputBuffer->OutputBufferMap.size();
    while (count > 0)
    {
        count --;
        if (!(tempIterator->second.frameQueue.empty()))
        {
            break;
        }
        tempIterator++;
        if (tempIterator == outputBuffer->OutputBufferMap.end())
        {
           tempIterator = outputBuffer->OutputBufferMap.begin();
        }
    }

    if (tempIterator->second.blockAckVrbls
        && (tempIterator->second.blockAckVrbls->state
                >= BAA_STATE_ADDBA_REQUEST_QUEUED
            && tempIterator->second.blockAckVrbls->state
                <= BAA_STATE_DELBA_QUEUED))
    {
        return FALSE;
    }
    outputBuffer->MapitrAmsdu = tempIterator;
    Mac802Address nextHopAddress;
    nextHopAddress = tempIterator->first.first;
    if (MAC_IsBroadcastMac802Address(&nextHopAddress))
    {
       // broadcast packets cannot be clubbed
       return FALSE;
    }
    if (DEBUG_AMPDU)
    {
        char dstAddress[MAX_STRING_LENGTH];
        MacDot11MacAddressToStr(dstAddress, &nextHopAddress);
        printf("%" TYPES_64BITFMT "d Node %d Checking if"
            " Ampdu can be created"
            " towards destination/priority %s/%d\n",
                node->getNodeTime(),
                node->nodeId,
                dstAddress,
                tempIterator->first.second);
    }
    UInt16 maxAmpduSize = 0;
    if (MacDot11IsAp(dot11))
    {
        DOT11_ApStationListItem* stationItem = NULL;
        stationItem =
               MacDot11ApStationListGetItemWithGivenAddress(
                                               node,
                                               dot11,
                                               nextHopAddress);
        if (!stationItem || !stationItem->data->isHTEnabledSta)
        {
            if (DEBUG_AMPDU)
            {
                char dstAddress[MAX_STRING_LENGTH];
                MacDot11MacAddressToStr(dstAddress, &nextHopAddress);
                printf("%" TYPES_64BITFMT "d Node %d Ampdu"
                    " cannot be created. No HT related information"
                    " available"
                    " for destination/priority %s/%d\n",
                        node->getNodeTime(),
                        node->nodeId,
                        dstAddress,
                        tempIterator->first.second);
            }
            return FALSE;
        }
        UInt8 ampduLengthExponent
           = stationItem->data->staHtCapabilityElement.
                ampduParams.ampduMaxLengthExponent;
        maxAmpduSize
            = MacDot11nCalculateMaxAmpduLength(ampduLengthExponent);
   }
   else
   {
       if (dot11->stationType == DOT11_STA_IBSS)
       {
           DOT11n_IBSS_Station_Info *stationInfo
               = MacDot11nGetIbssStationInfo(dot11, nextHopAddress);
           if (stationInfo->probeStatus == IBSS_PROBE_COMPLETED)
           {
                   UInt8 ampduLengthExponent
                       = stationInfo->staHtCapabilityElement.
                            ampduParams.ampduMaxLengthExponent;
                    maxAmpduSize
                     = MacDot11nCalculateMaxAmpduLength(ampduLengthExponent);
           }
           else
           {
                if (DEBUG_AMPDU)
                {
                    char dstAddress[MAX_STRING_LENGTH];
                    MacDot11MacAddressToStr(dstAddress, &nextHopAddress);
                    printf("%" TYPES_64BITFMT "d Node %d No HT related"
                        " information is currently available"
                        " for destination/priority %s/%d. Using 0 as "
                        " ampdu exponent\n",
                            node->getNodeTime(),
                            node->nodeId,
                            dstAddress,
                            tempIterator->first.second);
                }
                maxAmpduSize
                    = MacDot11nCalculateMaxAmpduLength(0);
           }
       }
       else
       {
           if (!dot11->associatedAP || !dot11->associatedAP->htEnableAP)
           {
               return FALSE;
           }
            UInt8 ampduLengthExponent =
        dot11->associatedAP->staHtCapabilityElement.
                                    ampduParams.ampduMaxLengthExponent;
         maxAmpduSize =
                     MacDot11nCalculateMaxAmpduLength(ampduLengthExponent);
       }
   }

    if (tempIterator->second.numPackets >= MIN_MPDUS_IN_AMPDU)
    {
        UInt32 pktsize = 0;
        clocktype duration = 0;
        std::list<struct DOT11_FrameInfo*>::iterator itr =
                tempIterator->second.frameQueue.begin();

        UInt8 numPkts = 0;
        UInt8 prevSubFramePaddingOctets = 0;
        while ((acIndex < 2 || duration < dot11->ACs[acIndex].TXOPLimit)
              && itr != tempIterator->second.frameQueue.end()
              && numPkts <= MAX_MPDUS_IN_AMPDU
              && pktsize < maxAmpduSize
              && (*itr)->frameType == DOT11_QOS_DATA
              && (*itr)->state == STATE_FRESH)
        {
            UInt32 tempPktsize = DOT11nMessage_ReturnPacketSize((*itr)->msg);
            tempPktsize += MACDOT11N_MPDU_DELIMITER;
            UInt8 paddingOctets
              = tempPktsize % MACDOT11N_AMPDU_SUBFRAME_MAX_PADDING;
            MAC_PHY_TxRxVector tempTxVector;
            tempTxVector = MacDot11nSetTxVector(
                               node,
                               dot11,
                               pktsize + tempPktsize,
                               DOT11_QOS_DATA,
                               tempIterator->first.first,
                               NULL);

            duration += PHY_GetTransmissionDuration(
                        node, dot11->myMacData->phyNumber,
                        tempTxVector);

             if ((acIndex < 2 || duration < dot11->ACs[acIndex].TXOPLimit)
                  && (pktsize + tempPktsize) < maxAmpduSize
                  && (*itr)->state == STATE_FRESH
                  && numPkts < MAX_MPDUS_IN_AMPDU)
             {
                 itr++;
                 numPkts++;
                 prevSubFramePaddingOctets = paddingOctets;
                 pktsize += tempPktsize + paddingOctets;
             }
             else
             {
                 pktsize -= prevSubFramePaddingOctets;
                 break;
             }
            }
           *numPackets = numPkts;
           if (*numPackets >= MIN_MPDUS_IN_AMPDU)
           {
                if (DEBUG_AMPDU)
                {
                    char dstAddress[MAX_STRING_LENGTH];
                    MacDot11MacAddressToStr(dstAddress, &nextHopAddress);
                    printf("%" TYPES_64BITFMT "d Node %d Creating"
                        " Ampdu of size %d num packets packed %d"
                        " with max supported Ampdu size %d"
                        " for destination/priority %s/%d\n",
                            node->getNodeTime(),
                            node->nodeId,
                            pktsize,
                            *numPackets,
                            maxAmpduSize,
                            dstAddress,
                            tempIterator->first.second);
                }
               return TRUE;
           }
            if (DEBUG_AMPDU)
            {
                char dstAddress[MAX_STRING_LENGTH];
                MacDot11MacAddressToStr(dstAddress, &nextHopAddress);
                printf("%" TYPES_64BITFMT "d Node %d Ampdu"
                    " cannot be created. Min num of Mpdus (%d) for Ampdu"
                    " when packed cannot be sent in the allocated TxOp"
                    " for destination/priority %s/%d\n",
                        node->getNodeTime(),
                        node->nodeId,
                        MIN_MPDUS_IN_AMPDU,
                        dstAddress,
                        tempIterator->first.second);
            }
           return FALSE;
    }
    else
    {
        if (DEBUG_AMPDU)
        {
            char dstAddress[MAX_STRING_LENGTH];
            MacDot11MacAddressToStr(dstAddress, &nextHopAddress);
            printf("%" TYPES_64BITFMT "d Node %d Ampdu"
                " cannot be created. Num packets are less"
                " than %d"
                " for destination/priority %s/%d\n",
                    node->getNodeTime(),
                    node->nodeId,
                    MIN_MPDUS_IN_AMPDU,
                    dstAddress,
                    tempIterator->first.second);
        }
        return FALSE;
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nCreateAmpdu
//  PURPOSE:     Creates an ampdu
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int acIndex
//                  Index of the access category
//               UInt8 numPackets
//                  num mpdus in an ampdu
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nCreateAmpdu(Node* node,
                          MacDataDot11* dot11,
                          int acIndex,
                          UInt8 numPackets)
{
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;

    // roundRobinKeyItr is an iterator which iterates through the keys
    // in round robin way to dequeue packets.
    
    outputBuffer->roundRobinKeyItr++;
    if (outputBuffer->roundRobinKeyItr == outputBuffer->OutputBufferMap.end())
    {
        outputBuffer->roundRobinKeyItr = outputBuffer->OutputBufferMap.begin();
    }
    int count = outputBuffer->OutputBufferMap.size();
    while (count > 0)
    {
        count --;
        if (!(outputBuffer->roundRobinKeyItr->second.frameQueue.empty()))
        {
            break;
        }
        outputBuffer->roundRobinKeyItr++;
        if (outputBuffer->roundRobinKeyItr == outputBuffer->OutputBufferMap.end())
        {
           outputBuffer->roundRobinKeyItr = outputBuffer->OutputBufferMap.begin();
        }
    }

    std::list<struct DOT11_FrameInfo*>::iterator itr =
    outputBuffer->roundRobinKeyItr->second.frameQueue.begin();
    Message* ListRoot = NULL;
    Message* ItrList = NULL;
    Message* MsgList = NULL;
    ERROR_Assert(outputBuffer->roundRobinKeyItr->second.winEnds
        == MACDOT11N_INVALID_SEQ_NUM,"Invalis Seq no");
    DOT11_FrameInfo *frameInfo
        = (DOT11_FrameInfo*)MEM_malloc(sizeof(DOT11_FrameInfo));
    memset(frameInfo, 0, sizeof(DOT11_FrameInfo));
    
    int index = 0;
    for (index = 0; index < numPackets; index++)
    {
        DOT11_FrameInfo *tempFrameInfo = *itr;
        Message* dupMsg = MESSAGE_Duplicate(node, tempFrameInfo->msg);
        (*itr)->state = STATE_AWAITING_ACK;
        DOT11n_FrameHdr* hdr =
           (DOT11n_FrameHdr *)MESSAGE_ReturnPacket(dupMsg);
        
        if (index == 0)
        {
            ListRoot = dupMsg;
            ItrList = ListRoot;
            MsgList = ItrList;
            ERROR_Assert(outputBuffer->roundRobinKeyItr->second.winStarts
                == hdr->seqNo,"invalid seq no");
        }
        else
        {
            ItrList = dupMsg;
            ItrList->next = NULL;
            MsgList->next = ItrList;
            MsgList = MsgList->next;
        }
        outputBuffer->roundRobinKeyItr->second.winSizes++;
        frameInfo->ampduOverhead += MACDOT11N_MPDU_DELIMITER;
        if (index < (numPackets - 1))
        {
             // Do not add padding octets for the last Ampdu subframe
             UInt8 paddingOctets = DOT11nMessage_ReturnPacketSize(dupMsg)
                                    % MACDOT11N_AMPDU_SUBFRAME_MAX_PADDING;
             frameInfo->ampduOverhead += paddingOctets;
        }
        if (index ==(numPackets - 1))
        {
            outputBuffer->roundRobinKeyItr->second.winEnds = hdr->seqNo;
        }
        itr++;
        if (hdr->qoSControl.AmsduPresent)
        {
            dot11->numAmsdusSentUnderAmpdu++;
        }
        else
        {
            dot11->numMpdusSentUnderAmpdu++;
        }
    }
    int ampduSize = 0;
    Message *ampduMsg = MESSAGE_PackMessage(node,
                                          ListRoot,
                                          TRACE_DOT11,
                                          &ampduSize);
    dot11->numAmpdusCreated++;
    frameInfo->RA = outputBuffer->roundRobinKeyItr->first.first;
    frameInfo->TA = dot11->selfAddr;
    frameInfo->isAMPDU = TRUE;
    frameInfo->msg = ampduMsg;
    frameInfo->frameType = DOT11_QOS_DATA;
    dot11->ACs[acIndex].frameInfo = frameInfo;
    dot11->ACs[acIndex].frameToSend = frameInfo->msg;
    dot11->ACs[acIndex].priority = outputBuffer->roundRobinKeyItr->first.second;
    dot11->ACs[acIndex].currentNextHopAddress
        = outputBuffer->roundRobinKeyItr->first.first;
    dot11->ACs[acIndex].totalNoOfthisTypeFrameQueued++;
}

//--------------------------------------------------------------------------
//  NAME:        Macdot11nHandleSuccessfullyTransmittedAMPDU
//  PURPOSE:     Updates the output buffer for a received block ack frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the block ack frame
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void Macdot11nHandleSuccessfullyTransmittedAMPDU(Node* node,
                                                MacDataDot11* dot11,
                                                Message *msg)
{

    DOT11n_BlockAckControlFrame *blockAckFrame =
          (DOT11n_BlockAckControlFrame*) MESSAGE_ReturnPacket(msg);

    UInt8 acIndex =
        (UInt8)MacDot11ReturnAccessCategory(
                                    blockAckFrame->BAControlField.TID_INFO);
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first =  blockAckFrame->sourceAddr;
    tmpKey.second = blockAckFrame->BAControlField.TID_INFO;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = outputBuffer->OutputBufferMap.find(tmpKey);
    BOOL process = FALSE;
     BOOL erase = TRUE;
    
    UInt16 seqNoOfFirstPktInQueueNotAcked = MACDOT11N_INVALID_SEQ_NUM;
    if (keyItr != outputBuffer->OutputBufferMap.end())
    {
         process
                = keyItr->second.numPackets == 0? FALSE: TRUE;
        if (process)
        {
            ERROR_Assert(keyItr->second.winStarts
                         == blockAckFrame->BAInfoField.BAStartSeqNumber,
                         "invalid seq no");
            
            // winStarts can be zero but winEnds cannot be
            // MACDOT11N_INVALID_SEQ_NUM

            std::list<struct DOT11_FrameInfo*>::iterator itr =
                            keyItr->second.frameQueue.begin();
            UInt64 tempBitMap = 0;
            BOOL firstPktDropDetected = FALSE;

            UInt64 tempBitMap2 = 0;
            int byteCount = 0;
            char *bitmapPointer = (char*)(&tempBitMap2);
            
            int tempItr = 0;
            for (tempItr = 0;
                 tempItr < MACDOT11N_BLOCK_ACK_BITMAP_SIZE;
                 tempItr++)
            {
                memcpy(bitmapPointer,
                       &blockAckFrame->BAInfoField.BABitmap[byteCount],
                       sizeof(UInt8));
                byteCount++;
                bitmapPointer++;
            }

            unsigned int tempItr2 = 0;
            for (tempItr2 = 0; tempItr2 < keyItr->second.winSizes; tempItr2++)
             {
                  DOT11_FrameInfo *tempFrameInfo = *itr;
                   DOT11n_FrameHdr* hdr = (DOT11n_FrameHdr *)
                                   MESSAGE_ReturnPacket(tempFrameInfo->msg);

                 tempBitMap = (UInt64)pow(2.0, (int)tempItr2);
                 if (tempItr2==0)
                 {
                   ERROR_Assert(keyItr->second.winStarts == hdr->seqNo,
                       "Invalid seq no");
                 }
                 if (tempBitMap2 & tempBitMap)
                 {
                      if (erase)
                      {
                           keyItr->second.aggSize
                              -= DOT11nMessage_ReturnPacketSize(
                                                    tempFrameInfo->msg);
                              ERROR_Assert(tempFrameInfo->state
                                           == STATE_AWAITING_ACK,
                                           "Invalid state");
                            keyItr->second.frameQueue.erase(itr);

                            if (keyItr->second.frameQueue.size() > 0)
                            {
                                itr = keyItr->second.frameQueue.begin();
                            }
                            MESSAGE_Free(node, tempFrameInfo->msg);
                            tempFrameInfo->msg = NULL;

                            MEM_free(tempFrameInfo);
                            tempFrameInfo = NULL;

                            dot11->numPktsDequeuedFromOutputBuffer++;
                            keyItr->second.numPackets--;
                      }
                      else
                      {
                          tempFrameInfo->state = STATE_ACKED;  //acked
                          if (keyItr->second.frameQueue.size() > 0)
                          {
                              itr++;
                          }
                          else
                          {
                              ERROR_Assert(tempItr2 ==
                              keyItr->second.winSizes -1,
                              "Invalid winsize");
                          }
                      }
                  }
                  else
                  {
                      if (!firstPktDropDetected)
                      {
                          firstPktDropDetected = TRUE;
                          seqNoOfFirstPktInQueueNotAcked = hdr->seqNo;
                      }
                      erase = FALSE;
                      if (keyItr->second.frameQueue.size() > 0)
                      {
                          itr++;
                      }
                      else
                      {
                          ERROR_Assert(tempItr2 ==keyItr->second.winSizes -1,
                              "invalid win size");
                      }
                  }
                  if (tempItr2 < (keyItr->second.winSizes - 1))
                  {
                      ERROR_Assert(itr != keyItr->second.frameQueue.end(),
                          "End of frame queue");
                  }
             }
        } // process
    }
    if (process)
    {
        if (erase)
        {
            unsigned int tmpItr = 0;
            for (tmpItr =0; tmpItr < keyItr->second.winSizes; tmpItr++)
            {
                MacDot11nIncrementSeqNumber(&keyItr->second.winStarts);
            }
        }
        else
        {
          keyItr->second.winStarts = seqNoOfFirstPktInQueueNotAcked;
        }
    }
    keyItr->second.winEnds = MACDOT11N_INVALID_SEQ_NUM;
    keyItr->second.winSizes = 0;
    dot11->numAmpdusSent++;
    MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                       (UInt8)dot11->currentACIndex);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nUpdateBapForBar
//  PURPOSE:     Updates the block ack policy for block ack request
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_FrameInfo *frameInfo
//                  Pointer to the frame info
//               BOOL success
//                  Specifies if transmission attemp for bloack ack request
//                  was a success or failure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nUpdateBapForBar(Node* node,
                                      MacDataDot11* dot11,
                                      DOT11_FrameInfo *frameInfo,
                                      BOOL success)
{

    DOT11n_BlockAckRequestFrame *barFrame =
      (DOT11n_BlockAckRequestFrame*)
        MESSAGE_ReturnPacket(frameInfo->msg);

     UInt8 acIndex =
        (UInt8)MacDot11ReturnAccessCategory(barFrame->barControl.TID_INFO);
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first = barFrame->destAddr;
    tmpKey.second = barFrame->barControl.TID_INFO;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = outputBuffer->OutputBufferMap.find(tmpKey);

    if (keyItr == outputBuffer->OutputBufferMap.end())
    {
        ERROR_Assert(keyItr->second.blockAckVrbls->state ==
                            BAA_STATE_WF_BLOCK_ACK,
                            "Invalid state");
    }
    if (success)
    {
          MacDot11nSetMoreFramesFieldForAnAC(dot11, acIndex);
    }
    else
    {
        // block ack request failed to send
        // resend
       switch(barFrame->barControl.BARAckPolicy)
       {
       case BA_IMMEDIATE:
           dot11->totalNoImmBlockAckRequestDropped++;
            break;
        case BA_DELAYED:
          dot11->totalNoDelBlockAckRequestDropped++;
        break;
       }
       if (keyItr->second.blockAckVrbls->state ==
                            BAA_STATE_WF_BLOCK_ACK)
       {
           // resend
             switch(barFrame->barControl.BARAckPolicy)
               {
               case BA_IMMEDIATE:
                   dot11->totalNoImmBlockAckRequestSent++;
                    break;
                case BA_DELAYED:
                  dot11->totalNoDelBlockAckRequestSent++;
                break;
               }
            Message *msg = MESSAGE_Alloc(node, 0, 0, 0);
            MESSAGE_PacketAlloc(node,
                            msg,
                            sizeof(DOT11n_BlockAckRequestFrame),
                            TRACE_DOT11);
            DOT11n_BlockAckRequestFrame *newAddbaFrame
                = (DOT11n_BlockAckRequestFrame*) MESSAGE_ReturnPacket(msg);
            memcpy(newAddbaFrame,
                            barFrame,
                            sizeof(DOT11n_BlockAckRequestFrame));
         DOT11_FrameInfo *frameInfo
             = (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
         memset(frameInfo, 0, sizeof(DOT11_FrameInfo));
         frameInfo->msg = msg;
         frameInfo->RA = newAddbaFrame->destAddr;
         frameInfo->TA = newAddbaFrame->sourceAddr;
         frameInfo->SA = newAddbaFrame->sourceAddr;
         frameInfo->DA = newAddbaFrame->destAddr;
         frameInfo->frameType = (DOT11_MacFrameType)newAddbaFrame->frameType;
         frameInfo->insertTime = node->getNodeTime();
         MacDot11nSetMoreFramesFieldForAnAC(dot11, acIndex);
         MacDot11nMgmtQueue_EnqueuePacket(node, dot11, frameInfo);
       }
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nUpdateBapForActionFrames
//  PURPOSE:     Updates the block ack policy for action frames
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_FrameInfo *frameInfo
//                  Pointer to the frame info
//               BOOL success
//                  Specifies if transmission attemp for action frame
//                  was a success or failure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nUpdateBapForActionFrames(Node* node,
                               MacDataDot11* dot11,
                               DOT11_FrameInfo *frameInfo,
                               BOOL success)
{
    DOT11n_ADDBARequestFrame* addbaFrame
        = (DOT11n_ADDBARequestFrame*) MESSAGE_ReturnPacket(frameInfo->msg);
    DOT11_ManagementVars * mngmtVars
        = (DOT11_ManagementVars*) dot11->mngmtVars;
    switch(addbaFrame->action)
    {
        case DOT11n_ADDBA_Request:
        {
            TosType priority =  addbaFrame->blockAckParams.TID;
            UInt8 acIndex = (UInt8)MacDot11ReturnAccessCategory(priority);
            OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
            std::pair<Mac802Address,TosType> tmpKey;
            tmpKey.first = frameInfo->RA;
            tmpKey.second = priority;
            std::map<std::pair<Mac802Address,TosType>,MapValue>
                ::iterator keyItr;
            keyItr = outputBuffer->OutputBufferMap.find(tmpKey);
            clocktype timerDelay = 0;
            if (success)
            {
                keyItr->second.blockAckVrbls->state
                    = BAA_STATE_WF_ADDBA_RESPONSE;
                timerDelay = MACDOT11N_ADDBS_REQUEST_TIMEOUT;
            }
            else
            {
                mngmtVars->ADDBSRequestDropped++;
                keyItr->second.blockAckVrbls->startingSeqNo
                    = MACDOT11N_INVALID_SEQ_NUM;
                keyItr->second.blockAckVrbls->state
                    = BAA_STATE_WF_RETRY_TIMER_EXPIRE;
                timerDelay = MACDOT11N_BAP_REINITIATE_TIMEOUT;
            }
            MacDot11nSetBapTimer(node,
                                 dot11,
                                 &keyItr->second.blockAckVrbls->
                                 baTimerSequenceNumber,
                                 tmpKey.first,
                                 tmpKey.second,
                                 timerDelay,
                                 TRUE);
            MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                               acIndex);
            break;
        }
        case DOT11n_ADDBA_Response:
        {
            DOT11n_ADDBAResponseFrame *addbaFrame
                = (DOT11n_ADDBAResponseFrame*)
                    MESSAGE_ReturnPacket(frameInfo->msg);
            InputBuffer *inputBuffer = dot11->inputBuffer;
            std::pair<Mac802Address,TosType> tmpKey;
            tmpKey.first =addbaFrame->destAddr;
            tmpKey.second = addbaFrame->blockAckParams.TID;
            std::map<std::pair<Mac802Address,TosType>,
                                             MapValue>::iterator keyItr;
            keyItr = inputBuffer->InputBufferMap.find(tmpKey);
            memset(&keyItr->second.BABitmap, 0, sizeof(UInt64));
            if (success)
            {
                if (addbaFrame->statusCode != FAILURE)
                {
                    memset(&keyItr->second.BABitmap, 0, sizeof(UInt64));
                    keyItr->second.BAPActiveAtReceiver = TRUE;
                    keyItr->second.blockAckVrbls->state
                        = BAA_STATE_RECEIVING;
                    keyItr->second.winStarts
                        = keyItr->second.blockAckVrbls->startingSeqNo;
                    keyItr->second.winSizes
                       = keyItr->second.blockAckVrbls->numPktsNegotiated;
                    UInt16 endSeqNo = keyItr->second.winStarts;
                    UInt32 i= 0;
                    for (i= 0; i < keyItr->second.winSizes -1; i++)
                    {
                        MacDot11nIncrementSeqNumber(&endSeqNo);
                    }
                    keyItr->second.winEnds = endSeqNo;
                    keyItr->second.winStartr
                        = keyItr->second.winStarts;
                    keyItr->second.winSizer
                       = keyItr->second.winSizes;
                    keyItr->second.winEndr = keyItr->second.winEnds;
                    MacDot11nSetBapTimer(node,
                                         dot11,
                                         &keyItr->second.blockAckVrbls->
                                         baTimerSequenceNumber,
                                         tmpKey.first,
                                         tmpKey.second,
                                         dot11->blockAckPolicyTimeout,
                                         FALSE);
                }
                else
                {
                    keyItr->second.winStartr = MACDOT11N_INVALID_SEQ_NUM;
                    keyItr->second.winEndr = MACDOT11N_INVALID_SEQ_NUM;
                    keyItr->second.winSizer = 0;
                    keyItr->second.winStarts
                            = keyItr->second.blockAckVrbls->startingSeqNo;
                    keyItr->second.winEnds = keyItr->second.winStarts;
                    keyItr->second.winSizes = 1;
                    keyItr->second.blockAckVrbls->state
                        = BAA_STATE_DISABLED;
                }
            }
            else
            {
                mngmtVars->ADDBSResponseDropped++;
                keyItr->second.winStartr = MACDOT11N_INVALID_SEQ_NUM;
                keyItr->second.winEndr = MACDOT11N_INVALID_SEQ_NUM;
                keyItr->second.winSizer = 0;
                keyItr->second.winStarts
                    = keyItr->second.blockAckVrbls->startingSeqNo;
                keyItr->second.winEnds = keyItr->second.winStarts;
                keyItr->second.winSizes = 1;
                keyItr->second.blockAckVrbls->state = BAA_STATE_DISABLED;
                keyItr->second.blockAckVrbls->role = ROLE_NONE;
                keyItr->second.blockAckVrbls->type = BA_NONE;
            }
            MacDot11nCancelInputBufferTimer(node, keyItr);
            break;
        }
        case DOT11n_DELBA:
        {
            if (success)
            {
                // do nothing
            }
            else
            {
                mngmtVars->DELBARequestDropped++;
            }
            DOT11n_DELBAFrame *delbaFrame
                = (DOT11n_DELBAFrame*)
                    MESSAGE_ReturnPacket(frameInfo->msg);
            if (delbaFrame->delbaParams.initiator)
            {
                TosType priority = delbaFrame->delbaParams.TID;
                UInt8 acIndex = (UInt8)MacDot11ReturnAccessCategory(priority);
                OutputBuffer *outputBuffer
                    = dot11->ACs[acIndex].outputBuffer;
                std::pair<Mac802Address,TosType> tmpKey;
                tmpKey.first = frameInfo->RA;
                tmpKey.second = priority;
                std::map<std::pair<Mac802Address,TosType>,MapValue>
                                                  ::iterator keyItr;
                keyItr = outputBuffer->OutputBufferMap.find(tmpKey);
                keyItr->second.winEnds = MACDOT11N_INVALID_SEQ_NUM;
                keyItr->second.winSizes = 0;
                MacDot11nResetbap(keyItr);
                MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                                   acIndex);
            }
            else
            {
                // reset at the receivers end
                DOT11n_DELBAFrame *delbaFrame =
                  (DOT11n_DELBAFrame*) MESSAGE_ReturnPacket(frameInfo->msg);
                InputBuffer *inputBuffer = dot11->inputBuffer;
                std::pair<Mac802Address,TosType> tmpKey;
                tmpKey.first =delbaFrame->destAddr;
                tmpKey.second = delbaFrame->delbaParams.TID;
                std::map<std::pair<Mac802Address,TosType>,MapValue>
                    ::iterator keyItr;
                keyItr = inputBuffer->InputBufferMap.find(tmpKey);
                keyItr->second.winStartr = MACDOT11N_INVALID_SEQ_NUM;
                keyItr->second.winEndr = MACDOT11N_INVALID_SEQ_NUM;
                keyItr->second.winSizer = 0;
                keyItr->second.blockAckVrbls->state = BAA_STATE_DISABLED;
                keyItr->second.blockAckVrbls->role = ROLE_NONE;
                keyItr->second.blockAckVrbls->type = BA_NONE;
                keyItr->second.BAPActiveAtReceiver = FALSE;
                keyItr->second.winEnds = keyItr->second.winStarts;
                keyItr->second.winSizes = 64;
                keyItr->second.BABitmap = 0;
                keyItr->second.blockAckVrbls->startingSeqNo
                    = MACDOT11N_INVALID_SEQ_NUM;
                keyItr->second.blockAckVrbls->numPktsNegotiated = 0;
            }
            break;
        }
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nIbssStartProbeTimer
//  PURPOSE:     Starts the probe response timeout at an STA
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_FrameInfo* dot11TxFrameInfo
//                  Frame info of last sent probe request
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nIbssStartProbeTimer(Node* node,
                                  MacDataDot11* dot11,
                                  DOT11n_IBSS_Station_Info *stationInfo)
{
    Message* newMsg;
    newMsg = MESSAGE_Alloc(node,
                           MAC_LAYER,
                           MAC_PROTOCOL_DOT11,
                           MSG_MAC_IBSS_PROBE_Timer);
    MESSAGE_SetInstanceId(newMsg, (short) dot11->myMacData->interfaceIndex);
    MESSAGE_AddInfo(node,
                    newMsg,
                    sizeof(timerMsg), INFO_TYPE_Dot11nTimerInfo);
    timerMsg* info = (timerMsg*)MESSAGE_ReturnInfo(newMsg, INFO_TYPE_Dot11nTimerInfo);
    info->addr = stationInfo->macAddr;
    newMsg->originatingNodeId = node->nodeId;
    stationInfo->timerMessage = newMsg;
    MESSAGE_Send(node,
                 newMsg,
                 DOT11_MANAGEMENT_PROBE_RESPONSE_TIMEOUT);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessAck
//  PURPOSE:     Process the received acknowledgement
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nProcessAck(Node* node,
                                MacDataDot11* dot11)
{
    Mac802Address sourceAddr;
    sourceAddr = dot11->waitingForAckOrCtsFromAddress;
    if (dot11->useDvcs)
    {
        PHY_UnlockAntennaDirection(node, dot11->myMacData->phyNumber);
    }
     switch (dot11->state)
     {
        case DOT11_S_WFACK:
        {
            MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            if (dot11->dot11TxFrameInfo!= NULL)
            {
                if (dot11->dot11TxFrameInfo->frameType >= DOT11_ASSOC_REQ
                    && dot11->dot11TxFrameInfo->frameType
                    <= DOT11_DEAUTHENTICATION)
                {
                    MESSAGE_Free(node, dot11->currentMessage);
                    dot11->managementFrameDequeued = FALSE;
                    if (dot11->stationType == DOT11_STA_IBSS
                        && dot11->dot11TxFrameInfo->frameType
                            == DOT11_PROBE_REQ)
                    {
                        DOT11n_IBSS_Station_Info *stationInfo
                            = MacDot11nGetIbssStationInfo(
                                 dot11,
                                 dot11->dot11TxFrameInfo->RA);
                        ERROR_Assert(stationInfo->probeStatus
                            == IBSS_PROBE_REQUEST_QUEUED,
                            "Probe request not queued");
                        stationInfo->probeStatus = IBSS_PROBE_IN_PROGRESS;
                        MacDot11nIbssStartProbeTimer(node,
                                                     dot11,
                                                     stationInfo);
                         dot11->dot11TxFrameInfo = NULL;
                    }
                    else if (!MacDot11IsAp(dot11))
                    {
                         MacDot11ManagementStartTimerOfGivenFrameType(
                         node,
                         dot11,
                         dot11->dot11TxFrameInfo->frameType);
                         dot11->dot11TxFrameInfo = NULL;
                    }
                    MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                                    (UInt8)dot11->currentACIndex);
                    dot11->ACs[dot11->currentACIndex].
                        totalNoOfthisTypeFrameDeQueued++;
                    MacDot11StationResetAckVariables(dot11);
                    MacDot11nResetACVariables(dot11,
                                              dot11->currentACIndex);
                    MacDot11nResetCurrentMessageVariables(dot11);
                    MacDot11StationResetCW(node, dot11);
                    MacDot11StationCheckForOutgoingPacket(node,
                                                          dot11,
                                                          TRUE);
                }
                else if (dot11->dot11TxFrameInfo->frameType
                    == DOT11N_BLOCK_ACK_REQUEST)
                {
                    MacDot11nUpdateBapForBar(node,
                                             dot11,
                                             dot11->dot11TxFrameInfo,
                                             TRUE);
                    MESSAGE_Free(node, dot11->currentMessage);
                    dot11->ACs[dot11->currentACIndex].
                        totalNoOfthisTypeFrameDeQueued++;
                    dot11->unicastPacketsSentDcf++;
                    MacDot11StationResetAckVariables(dot11);
                    MacDot11nResetACVariables(dot11,
                                              dot11->currentACIndex);
                    MacDot11nResetCurrentMessageVariables(dot11);
                    MacDot11StationResetCW(node, dot11);
                    MacDot11StationCheckForOutgoingPacket(node,
                                                          dot11,
                                                          TRUE);
                }
                else if (dot11->dot11TxFrameInfo->frameType == DOT11_ACTION)
                {
                    MacDot11nUpdateBapForActionFrames(
                                               node,
                                               dot11,
                                               dot11->dot11TxFrameInfo,
                                               TRUE);
                    MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                                (UInt8)dot11->currentACIndex);
                    MESSAGE_Free(node, dot11->currentMessage);
                    dot11->ACs[dot11->currentACIndex].
                        totalNoOfthisTypeFrameDeQueued++;
                    dot11->unicastPacketsSentDcf++;
                    MacDot11StationResetAckVariables(dot11);
                    MacDot11nResetACVariables(dot11,
                                              dot11->currentACIndex);
                    MacDot11nResetCurrentMessageVariables(dot11);
                    MacDot11StationResetCW(node, dot11);
                    MacDot11StationCheckForOutgoingPacket(node,
                                                          dot11,
                                                          TRUE);
                }
                else if (dot11->dot11TxFrameInfo->frameType
                            == DOT11N_DELAYED_BLOCK_ACK)
                {
                    DOT11n_BlockAckControlFrame *hdr
                        = (DOT11n_BlockAckControlFrame*)
                            MESSAGE_ReturnPacket(
                                dot11->dot11TxFrameInfo->msg);
                    InputBuffer *inputBuffer = dot11->inputBuffer;
                    std::pair<Mac802Address,TosType> tmpKey;
                    tmpKey.first =hdr->destAddr;
                    tmpKey.second = hdr->BAControlField.TID_INFO;
                    std::map<std::pair<Mac802Address,TosType>,MapValue>::
                        iterator keyItr;
                    keyItr = inputBuffer->InputBufferMap.find(tmpKey);
                    ERROR_Assert(keyItr
                        != inputBuffer->InputBufferMap.end(),
                        "End of input buffer");
                    keyItr->second.blockAckVrbls->blockAckSent = TRUE;
                    MESSAGE_Free(node, dot11->currentMessage);
                    dot11->ACs[dot11->currentACIndex].
                        totalNoOfthisTypeFrameDeQueued++;
                    dot11->unicastPacketsSentDcf++;
                    MacDot11StationResetAckVariables(dot11);
                    MacDot11nResetACVariables(dot11,
                                              dot11->currentACIndex);
                    MacDot11nResetCurrentMessageVariables(dot11);
                    MacDot11StationResetCW(node, dot11);
                    MacDot11StationCheckForOutgoingPacket(node,
                                                          dot11,
                                                          TRUE);
                }
                else
                {
                    ERROR_Assert(dot11->dot11TxFrameInfo->frameType
                        == DOT11_QOS_DATA,"not QOS data");
                    MAC_MacLayerAcknowledgement(
                                        node,
                                        dot11->myMacData->interfaceIndex,
                                        dot11->currentMessage,
                                    sourceAddr);
                    DOT11n_FrameHdr* hdr = (DOT11n_FrameHdr*)
                        MESSAGE_ReturnPacket(dot11->dot11TxFrameInfo->msg);
                    if (hdr->qoSControl.AmsduPresent)
                    {
                        // updting Amsdu statistics
                        dot11->numAmsdusSent++;
                    }
                    MacDot11nUpdateOutputBuffer(node,
                                               dot11,
                                               hdr->destAddr,
                                               hdr->qoSControl.TID,
                                               hdr->seqNo);
                    Mac802Address destAddr = hdr->destAddr;
                    TosType priority =  hdr->qoSControl.TID;
                    MESSAGE_Free(node, dot11->currentMessage);
                    dot11->ACs[dot11->currentACIndex].
                        totalNoOfthisTypeFrameDeQueued++;
                    dot11->unicastPacketsSentDcf++;
                    MacDot11StationResetAckVariables(dot11);
                    MacDot11nResetACVariables(dot11,
                                              dot11->currentACIndex);
                    MacDot11nResetCurrentMessageVariables(dot11);
                    MacDot11StationResetCW(node, dot11);
                    UInt8 acIndex = (UInt8)MacDot11ReturnAccessCategory(priority);
                    OutputBuffer *outputBuffer
                        = dot11->ACs[acIndex].outputBuffer;
                    std::pair<Mac802Address,TosType> tmpKey;
                    tmpKey.first =  destAddr;
                    tmpKey.second =  priority;
                    std::map<std::pair<Mac802Address,TosType>,MapValue>::
                        iterator keyItr;
                    keyItr = outputBuffer->OutputBufferMap.find(tmpKey);
                    ERROR_Assert(keyItr
                        != outputBuffer->OutputBufferMap.end(),
                        "End of output buffer");
                    if (keyItr->second.blockAckVrbls
                        && keyItr->second.blockAckVrbls->state
                            == BAA_STATE_ADDBA_REQUEST_PENDING)
                    {
                        UInt8 numPkts;
                        BOOL canUseBlockAckPolicy =
                        MacDot11nBlockAckPolicyUsable(node,
                                                      dot11,
                                                      keyItr,
                                                      &numPkts,
                                                      TRUE);
                            if (canUseBlockAckPolicy)
                            {
                                MacDot11nSendAddbaRequest(node,
                                                          dot11,
                                                          keyItr,
                                                          numPkts);
                            }
                            else
                            {
                                keyItr->second.blockAckVrbls->state
                                    = BAA_STATE_DISABLED;
                                MacDot11StationCheckForOutgoingPacket(node,
                                                                      dot11,
                                                                      TRUE);
                            }
                        }
                        else
                        {
                            MacDot11StationCheckForOutgoingPacket(node,
                                                                  dot11,
                                                                  TRUE);
                        }
                    }
                }
            break;
        }
        case DOT11_S_WFPSPOLL_ACK:
        {
            ERROR_Assert(FALSE,"invalid state");
        }
        default:
        {
            MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
            MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
            break;
        }
    } // switch
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nHandlePromiscuousMode
//  PURPOSE:     Process the sniffed packets when promiscuous mode is on
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nHandlePromiscuousMode(Node* node,
                                   MacDataDot11* dot11,
                                   Message *msg)
{
    DOT11n_FrameHdr* hdr = (DOT11n_FrameHdr*)MESSAGE_ReturnPacket(msg);
    BOOL sendForPromiscuousMode
        = MacDot11IsFrameAUnicastDataTypeForPromiscuousMode(
            (DOT11_MacFrameType)hdr->frameType, msg);
    if (sendForPromiscuousMode)
    {
        MacDot11HandlePromiscuousMode(node,
                                      dot11,
                                      msg,
                                      hdr->sourceAddr,
                                      hdr->destAddr);
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessSniffedPackets
//  PURPOSE:     Process the sniffed packets
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nProcessSniffedPackets(Node* node,
                                   MacDataDot11* dot11,
                                   Message* msg)
{
    DOT11_ShortControlFrame* hdr
        = (DOT11_ShortControlFrame*) MESSAGE_ReturnPacket(msg);
    MacDot11ProcessNotMyFrame(node,
                              dot11,
                              MacDot11MicroToNanosecond(hdr->duration),
                              (hdr->frameType == DOT11_RTS),
                              (hdr->frameType == DOT11_QOS_CF_POLL));
    if (dot11->useDvcs)
    {
        Mac802Address sourceNodeAddress = INVALID_802ADDRESS;
        if (hdr->frameType == DOT11_RTS)
        {
            sourceNodeAddress
                = ((DOT11_LongControlFrame*)MESSAGE_ReturnPacket(msg))
                    ->sourceAddr;
        }
        else if (hdr->frameType == DOT11_DATA
                    || hdr->frameType == DOT11_MESH_DATA)
        {
            ERROR_Assert(FALSE,"invalid type");
        }
        else if (hdr->frameType == DOT11_QOS_DATA)
        {
            sourceNodeAddress =
                ((DOT11e_FrameHdr*)MESSAGE_ReturnPacket(msg))
                ->sourceAddr;
        }
        if (sourceNodeAddress != INVALID_802ADDRESS) {
            BOOL nodeIsInCache;
            MacDot11StationUpdateDirectionCacheWithCurrentSignal(
                node, dot11, sourceNodeAddress,
                &nodeIsInCache);
        }
    }
    if (dot11->myMacData->promiscuousMode == TRUE)
    {
        MacDot11nHandlePromiscuousMode(node, dot11, msg);
    }
    MESSAGE_Free(node, msg);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessMpdu
//  PURPOSE:     Process the received mpdu
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nProcessMpdu(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11_ShortControlFrame* hdr
        = (DOT11_ShortControlFrame*) MESSAGE_ReturnPacket(msg);
    BOOL isMyAddr = FALSE;
    if (NetworkIpIsUnnumberedInterface(
            node, dot11->myMacData->interfaceIndex))
    {
        MacHWAddress linkAddr;
        Convert802AddressToVariableHWAddress(
                                node, &linkAddr, &(hdr->destAddr));
        isMyAddr = MAC_IsMyAddress(node, &linkAddr);
    }
    else
    {
        isMyAddr = (dot11->selfAddr == hdr->destAddr);
    }
    if (isMyAddr)
    {
        MacDot11ProcessMyFrame (node, dot11, msg);
    }
    else if (hdr->destAddr == ANY_MAC802)
    {
        MacDot11ProcessAnyFrame (node, dot11, msg);
    }
    else
    {
        // handle promiscous mode
        MacDot11nProcessSniffedPackets(node, dot11, msg);
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nSendBlockAck
//  PURPOSE:     Creates and sends block ack in repsonse of an ampdu
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11n_BlockAckControlFrame *ackFrame
//                  Pointer to the block ack frame
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nSendBlockAck(Node* node,
                           MacDataDot11* dot11,
                           DOT11n_BlockAckControlFrame *ackFrame)
{
    Message* pktToPhy;
    MAC_PHY_TxRxVector rxVector;
    if (dot11->isHTEnable)
    {
        PHY_GetRxVector(node,
                        dot11->myMacData->phyNumber,
                        rxVector);
    }
    else
    {
        ERROR_Assert(FALSE,"Invalid State");
    }
    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node,
                        pktToPhy,
                        sizeof(DOT11n_BlockAckControlFrame),
                        TRACE_DOT11);
    DOT11n_BlockAckControlFrame *blockAckframe =
          (DOT11n_BlockAckControlFrame*) MESSAGE_ReturnPacket(pktToPhy);
    memset(blockAckframe, 0, sizeof(DOT11n_BlockAckControlFrame));
    memcpy(blockAckframe, ackFrame, sizeof(DOT11n_BlockAckControlFrame));
    MEM_free(ackFrame);
    ackFrame = NULL;
    MacDot11StationSetState(node, dot11, DOT11_X_ACK);
    dot11->totalNoOfBlockAckSent++;
    MAC_PHY_TxRxVector tempTxVector;
    tempTxVector = MacDot11nSetTxVector(
                                   node,
                                   dot11,
                                   DOT11nMessage_ReturnPacketSize(pktToPhy),
                                   DOT11N_BLOCK_ACK,
                                   blockAckframe->destAddr,
                                   &rxVector);
    PHY_SetTxVector(node,
                    dot11->myMacData->phyNumber,
                    tempTxVector);
    if (!dot11->useDvcs)
    {
        MacDot11StationStartTransmittingPacket(
            node,
            dot11,
            pktToPhy,
            dot11->sifs);
    }
    else
    {
        MacDot11StationStartTransmittingPacketDirectionally(
                    node, dot11, pktToPhy, dot11->sifs,
                    MacDot11StationGetLastSignalsAngleOfArrivalFromRadio(
                    node,
                    dot11));
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nIsMyFrame
//  PURPOSE:     Checks of the destination address in the received mpdu is
//               of this interface
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  destination address
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11nIsMyFrame(Node* node,
                        MacDataDot11* dot11,
                        Mac802Address destAddr)
{
    if (NetworkIpIsUnnumberedInterface(
            node, dot11->myMacData->interfaceIndex))
    {
        MacHWAddress linkAddr;
        Convert802AddressToVariableHWAddress(node,
                                             &linkAddr,
                                             &(destAddr));
        return MAC_IsMyAddress(node, &linkAddr);
    }
    else
    {
        return(dot11->selfAddr == destAddr);
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessSniffedAmpdu
//  PURPOSE:     Process sniffed ampdu
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msgList
//                  List of messages in an ampdu
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nProcessSniffedAmpdu(Node* node,
                                  MacDataDot11* dot11,
                                  Message *msgList)
{
    MAC_PHY_TxRxVector rxVector;
    PHY_GetRxVector(node,
                    dot11->myMacData->phyNumber,
                    rxVector);
    rxVector.length = sizeof(DOT11n_BlockAckControlFrame);
    clocktype duration =
    dot11->extraPropDelay
        + dot11->sifs
        + PHY_GetTransmissionDuration(node,
                                      dot11->myMacData->phyNumber,
                                      rxVector)
        + dot11->extraPropDelay;
    Message *msgItr = msgList;
    Message *tempList = NULL;
    UInt8 count = 0;
    do
    {
        tempList = msgItr->next;
        DOT11n_FrameHdr* hdr
            = (DOT11n_FrameHdr*)MESSAGE_ReturnPacket(msgItr);
        if (count == 0)
        {
            MacDot11ProcessNotMyFrame(node,
                                      dot11,
                                      duration,
                                      FALSE,
                                      FALSE);
            if (dot11->useDvcs)
            {
                BOOL nodeIsInCache = FALSE;
                MacDot11StationUpdateDirectionCacheWithCurrentSignal(
                                    node, dot11, hdr->sourceAddr,
                                     &nodeIsInCache);
            }
        }
        if (dot11->myMacData->promiscuousMode == TRUE)
        {
            MacDot11nHandlePromiscuousMode(node, dot11, msgItr);
        }
        MESSAGE_Free(node, msgItr);
        msgItr = tempList;
        count++;
    }while (msgItr != NULL);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessAmpdu
//  PURPOSE:     Process ampdu
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nProcessAmpdu(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
        double BER = *((double *)MESSAGE_ReturnInfo( msg,
                                   INFO_TYPE_Dot11nBER));
        Message *msgList = MESSAGE_UnpackMessage(node,
                                                msg,
                                                TRUE,
                                                TRUE);
        // send a single packet as if now
        DOT11n_FrameHdr* hdr = (DOT11n_FrameHdr *)MESSAGE_ReturnPacket(msgList);
        BOOL isMyAddr = MacDot11nIsMyFrame(node, dot11, hdr->destAddr);
        if (!isMyAddr)
        {
        // AMPDU recived destined to someother node. drop the packets as if
        // now
            if (DEBUG_AMPDU)
            {
                char dstAddress[MAX_STRING_LENGTH];
                MacDot11MacAddressToStr(dstAddress, &hdr->destAddr);
                printf("%" TYPES_64BITFMT "d Node %d Dropping"
                " Ampdu. Ampdu destined for %s\n",
                    node->getNodeTime(),
                    node->nodeId,
                    dstAddress);
            }
            MacDot11nProcessSniffedAmpdu(node, dot11, msgList);
            return;
        }
        if (DEBUG_AMPDU)
        {
            printf("%" TYPES_64BITFMT "d Node %d Ampdu"
                " received. Processing...\n",
                    node->getNodeTime(),
                    node->nodeId);
        }
        Message *msgItr = msgList;
        Message *tempList = NULL;
        MacDot11StationCancelTimer(node, dot11);
        dot11->numAmpdusReceived++;
        DOT11n_BlockAckControlFrame *blockAckFrame
        = (DOT11n_BlockAckControlFrame*)
                               MEM_malloc(sizeof(DOT11n_BlockAckControlFrame));
        memset(blockAckFrame, 0, sizeof(DOT11n_BlockAckControlFrame));
        UInt64 tempBitMap = 0;
       UInt64 tempBitMap2 = 0;
       UInt8 bitCount = 0;
       blockAckFrame->frameType = DOT11N_BLOCK_ACK;
       blockAckFrame->frameFlags |= DOT11_TO_DS;
       blockAckFrame->destAddr = hdr->sourceAddr;
       blockAckFrame->sourceAddr = dot11->selfAddr;
       blockAckFrame->BAControlField.BAAckPolicy = TRUE;
       blockAckFrame->BAControlField.MultiTID = 0;
       blockAckFrame->BAControlField.CompressedBitmap = 1;
       blockAckFrame->BAControlField.TID_INFO = hdr->qoSControl.TID;
       blockAckFrame->BAInfoField.BAStartSeqNumber = hdr->seqNo;
        do
        {
            // estimate each frame's error rate
            if (BER)
            {
                double numBits = DOT11nMessage_ReturnPacketSize(msgItr) * 8;
                double errorProbability = 1.0 - pow((1.0 - BER), numBits);
                double rand = RANDOM_erand(dot11->seed);
                assert((errorProbability >= 0.0) && (errorProbability <= 1.0));
                if (errorProbability > rand) {
                    
                    // this frame is corrupt
                    bitCount++;
                    msgItr->error = TRUE;
                    msgItr = msgItr->next;
                    continue;
                }
            }
            tempBitMap = (UInt64)pow(2.0, bitCount);
            tempBitMap2 |= tempBitMap;
            bitCount++;
            msgItr = msgItr->next;
    }while (msgItr != NULL);

    int byteCount = 0;
    char *bitmapPointer = (char*)(&tempBitMap2);
    for (int i = 0;i < MACDOT11N_BLOCK_ACK_BITMAP_SIZE; i++)
    {
        memcpy(&blockAckFrame->BAInfoField.BABitmap[byteCount],
               bitmapPointer,
               sizeof(UInt8));
        byteCount++;
        bitmapPointer++;
    }

        MacDot11nSendBlockAck(node, dot11, blockAckFrame);
        msgItr = msgList;
        do
        {
            tempList = msgItr->next;
        DOT11n_FrameHdr* hdr = (DOT11n_FrameHdr *)MESSAGE_ReturnPacket(msgItr);
            switch(hdr->frameType)
            {
            case DOT11_RTS:
                {
                    ERROR_Assert(FALSE,"invalid state");
                    break;
                }
            case DOT11_CTS:
                {
                    ERROR_Assert(FALSE,"invalid state");
                    break;
                }
            case DOT11_QOS_DATA:
                {
                       if (!msgItr->error)
                       {
                       MacDot11nProcessDataFrame(node, dot11, msgItr);
                       }
                       else
                       {
                           if (DEBUG_AMPDU)
                           {
                             DOT11n_FrameHdr* hdr =
                             (DOT11n_FrameHdr *)MESSAGE_ReturnPacket(msgItr);
                              printf("%" TYPES_64BITFMT "d Node %d Dropping"
                                " Mpdu with seq no. %d within the Ampdu\n",
                                    node->getNodeTime(),
                                    node->nodeId,
                                    hdr->seqNo);
                           }
                           MESSAGE_Free(node,msgItr);
                           msgItr = NULL;
                       }
                       break;
                }
            default:
                {
                    ERROR_Assert(FALSE,"invalid state");
                    break;
                }
            }
            msgItr = tempList;
        }while (msgItr != NULL);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessBlockAckUnderBAA
//  PURPOSE:     Processes block ack received under block ack aggregment
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Pointer to the message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nProcessBlockAckUnderBAA(Node* node,
                                      MacDataDot11* dot11,
                                      Message* msg)
{
    DOT11n_BlockAckControlFrame* blockAckFrame =
          (DOT11n_BlockAckControlFrame*) MESSAGE_ReturnPacket(msg);
    UInt8 acIndex = (UInt8)MacDot11ReturnAccessCategory
                        (blockAckFrame->BAControlField.TID_INFO);
    OutputBuffer* outputBuffer = dot11->ACs[acIndex].outputBuffer;
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first = blockAckFrame->sourceAddr;
    tmpKey.second = blockAckFrame->BAControlField.TID_INFO;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = outputBuffer->OutputBufferMap.find(tmpKey);
    if (keyItr->second.blockAckVrbls->state != BAA_STATE_WF_BLOCK_ACK)
    {
        return;
    }
    if (blockAckFrame->BAInfoField.BAStartSeqNumber
        != keyItr->second.winStarts)
    {
        return;
    }
    BOOL process = keyItr->second.numPackets == 0? FALSE: TRUE;
    BOOL erase = TRUE;
    UInt16 seqNoOfFirstPktInQueueNotAcked =
        MACDOT11N_INVALID_SEQ_NUM;
    if (process)
    {
        std::list<struct DOT11_FrameInfo*>::iterator listItr
            = keyItr->second.frameQueue.begin();
       UInt64 tempBitMap = 0;
       BOOL firstPktDropDetected = FALSE;
       UInt64 tempBitMap2 = 0;
       int byteCount = 0;
       char* bitmapPointer = (char*)(&tempBitMap2);
       int i = 0;
       for (i = 0; i < MACDOT11N_BLOCK_ACK_BITMAP_SIZE; i++)
       {
            memcpy(bitmapPointer,
                   &blockAckFrame->BAInfoField.BABitmap[byteCount],
                   sizeof(UInt8));
            byteCount++;
            bitmapPointer++;
        }
        UInt32 j = 0;
        for (j = 0; j < keyItr->second.winSizes; j++)
        {
             DOT11_FrameInfo *tempFrameInfo = *listItr;
             DOT11n_FrameHdr* hdr =
                 (DOT11n_FrameHdr *)
                  MESSAGE_ReturnPacket(tempFrameInfo->msg);
                  ERROR_Assert(tempFrameInfo->state
                               == STATE_AWAITING_ACK,
                               "invalid state");
                 tempBitMap = (UInt64)pow(2.0, (int)j);
                 if (j==0)
                 {
                     ERROR_Assert(keyItr->second.winStarts == hdr->seqNo,
                                  "invalid seq no");
                 }
            if (tempBitMap2 & tempBitMap)
            {
                if (erase)
                {
                    keyItr->second.aggSize
                        -= DOT11nMessage_ReturnPacketSize(
                               tempFrameInfo->msg);
                    keyItr->second.frameQueue.erase(listItr);
                    if (keyItr->second.frameQueue.size() > 0)
                    {
                        listItr = keyItr->second.frameQueue.begin();
                    }
                     MESSAGE_Free(node, tempFrameInfo->msg);
                     tempFrameInfo->msg = NULL;
                     MEM_free(tempFrameInfo);
                     tempFrameInfo = NULL;
                     dot11->numPktsDequeuedFromOutputBuffer++;
                     keyItr->second.numPackets--;
                  }
                  else
                  {
                       tempFrameInfo->state = STATE_ACKED;
                       if (keyItr->second.frameQueue.size() > 0)
                       {
                           listItr++;
                       }
                       else
                       {
                           ERROR_Assert(j ==keyItr->second.winSizes -1,
                              "invalid state");
                       }
                   }
              }
              else
              {
                  if (!firstPktDropDetected)
                  {
                     firstPktDropDetected = TRUE;
                     seqNoOfFirstPktInQueueNotAcked = hdr->seqNo;
                  }
                  erase = FALSE;
                  ERROR_Assert(tempFrameInfo->state
                      == STATE_AWAITING_ACK,
                      "Not Acked");
                  if (keyItr->second.frameQueue.size() > 0)
                  {
                      listItr++;
                  }
                  else
                  {
                      ERROR_Assert(j == keyItr->second.winSizes -1,
                          "invalid state");
                  }
              }
              if (j < (keyItr->second.winSizes - 1))
              {
                  ERROR_Assert(listItr != keyItr->second.frameQueue.end(),
                               "End of frame queue");
              }
         }
    } // process
    if (process)
    {
        if (erase)
        {
            UInt32  i = 0;
            for (i =0; i < keyItr->second.winSizes; i++)
            {
                MacDot11nIncrementSeqNumber(&keyItr->second.winStarts);
            }
        }
        else
        {
            keyItr->second.winStarts = seqNoOfFirstPktInQueueNotAcked;
        }
    }
    keyItr->second.winEnds = MACDOT11N_INVALID_SEQ_NUM;
    keyItr->second.winSizes = 0;
    keyItr->second.blockAckVrbls->startingSeqNo = MACDOT11N_INVALID_SEQ_NUM;
    keyItr->second.blockAckVrbls->numPktsToBeSentInCurrentSession = 0;
    keyItr->second.blockAckVrbls->numPktsSent = 0;
    keyItr->second.blockAckVrbls->state = BAA_STATE_IDLE;
    
    // reset the ba timer
    MacDot11nSetBapTimer(node,
                         dot11,
                         &keyItr->second.blockAckVrbls->
                         baTimerSequenceNumber,
                         keyItr->first.first,
                         keyItr->first.second,
                         dot11->blockAckPolicyTimeout,
                         TRUE);
    MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                       acIndex);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessBlockAck
//  PURPOSE:     Processes block ack received as a resopnse to ampdu
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nProcessBlockAck(Node* node,
                             MacDataDot11* dot11,
                             Message* msg)
{
    DOT11n_BlockAckControlFrame *hdr
        = (DOT11n_BlockAckControlFrame*)MESSAGE_ReturnPacket(msg);
    Mac802Address sourceAddr;
    if (hdr->BAControlField.BAAckPolicy)
    {
        sourceAddr = dot11->waitingForAckOrCtsFromAddress;
        dot11->dataRateInfo->numAcksInSuccess++;
    }
    dot11->totalNoOfBlockAckReceived++;
    if (MacDot11IsAp(dot11))
    {
         DOT11_ApStationListItem* stationItem =
        MacDot11ApStationListGetItemWithGivenAddress(node,
              dot11,
              hdr->sourceAddr);
        if (stationItem)
        {
            stationItem->data->LastFrameReceivedTime = node->getNodeTime();
          }
    }
    if (dot11->useDvcs)
    {
        BOOL nodeIsInCache;
        MacDot11StationUpdateDirectionCacheWithCurrentSignal(
           node, dot11, hdr->sourceAddr,
                 &nodeIsInCache);
          }
    if (dot11->useDvcs) {
        
        // For smart antennas, go back to sending/receiving
        // omnidirectionally.
        PHY_UnlockAntennaDirection(
            node, dot11->myMacData->phyNumber);
    }
     switch (dot11->state)
     {
        case DOT11_S_WFACK:
            {
                MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
                if (dot11->dot11TxFrameInfo!= NULL)
                {
                   if (dot11->dot11TxFrameInfo->frameType >= 
                       DOT11_ASSOC_REQ
                       && dot11->dot11TxFrameInfo->frameType
                       <= DOT11_ACTION)
                   {
                        ERROR_Assert(FALSE,"Invalid condition");
                   }
                   if (dot11->dot11TxFrameInfo->frameType ==
                       DOT11N_BLOCK_ACK_REQUEST)
                   {
                         MacDot11nProcessBlockAckUnderBAA(node,
                                                          dot11,
                                                          msg);
                   }
                   else
                   {
                        Macdot11nHandleSuccessfullyTransmittedAMPDU(node,
                                                                    dot11,
                                                                    msg);
                   }
                    MAC_MacLayerAcknowledgement(
                                            node,
                                            dot11->myMacData->interfaceIndex,
                                            dot11->currentMessage,
                                            sourceAddr);
                    MESSAGE_Free(node, dot11->currentMessage);
                    dot11->ACs[dot11->currentACIndex].
                                           totalNoOfthisTypeFrameDeQueued++;
                    dot11->unicastPacketsSentDcf++;
                    MacDot11StationResetAckVariables(dot11);
                    MacDot11nResetACVariables(dot11, dot11->currentACIndex);
                    MacDot11nResetCurrentMessageVariables(dot11);
                    MacDot11StationResetCW(node, dot11);
                    MacDot11StationCheckForOutgoingPacket(node,
                                                          dot11,
                                                          TRUE);
                }
                break;
            }
        default:
            {
                ERROR_Assert(!hdr->BAControlField.BAAckPolicy,
                    "BA policy exist");
                MacDot11nProcessBlockAckUnderBAA(node,
                                                 dot11,
                                                 msg);
                break;
            }
     }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nProcessBlockAckRequest
//  PURPOSE:     Processes a block ack request
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nProcessBlockAckRequest(Node* node,
                                    MacDataDot11* dot11,
                                    Message* msg)
{
    DOT11n_BlockAckRequestFrame *barframe
        = (DOT11n_BlockAckRequestFrame*)MESSAGE_ReturnPacket(msg);
    InputBuffer *inputBuffer = dot11->inputBuffer;
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first =barframe->sourceAddr;
    tmpKey.second = barframe->barControl.TID_INFO;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = inputBuffer->InputBufferMap.find(tmpKey);
    UInt16 startSeqNo = barframe->startingSeqControl.SeqNo;
    UInt8 bitCount = 0;
    BOOL sendBA = FALSE;
    BOOL updateBA = FALSE;
     if (barframe->barControl.BARAckPolicy)
     {
        dot11->totalNoImmBlockAckRequestReceived++;
     }
     else
     {
        dot11->totalNoDelBlockAckRequestReceived++;
     }
     if (keyItr->second.blockAckVrbls->state != BAA_STATE_RECEIVING
         && keyItr->second.blockAckVrbls->state != BAA_STATE_DELBA_QUEUED)
     {
         return;
     }
    if (startSeqNo == keyItr->second.winStartr)
    {
        sendBA = TRUE;
    }
    else
    {
         UInt16 tempSeqNo = keyItr->second.winStartr;
         UInt32 i = 0;
         for (i =0; i < keyItr->second.winSizer; i++)
         {
            if (tempSeqNo == startSeqNo)
            {
                updateBA = TRUE;
                sendBA = TRUE;
                break;
            }
            MacDot11nIncrementSeqNumber(&tempSeqNo);
        }
        if (updateBA)
        {
            UInt16 oldWinStartr = keyItr->second.winStartr;
            UInt16 newWinStartr = startSeqNo;
            do
            {
                keyItr->second.BABitmap >>= 1;
                MacDot11nIncrementSeqNumber(&oldWinStartr);
            }while (oldWinStartr != newWinStartr);

            keyItr->second.winStartr = newWinStartr;
            UInt16 newWinEndr =  keyItr->second.winStartr;
            UInt32 j = 0;
            for (j =0; j < keyItr->second.winSizer -1; j++)
            {
                MacDot11nIncrementSeqNumber(&newWinEndr);
            }
             keyItr->second.winEndr = newWinEndr;
        }
        else
        {
            bitCount = 0;
            UInt16 seqNoUpperLimit = keyItr->second.winStartr;
            int i =0;
            for (i =0; i < 2048 - 1; i++)
            {
                 MacDot11nIncrementSeqNumber(&seqNoUpperLimit);
            }
            tempSeqNo = keyItr->second.winEndr;
            while (tempSeqNo != seqNoUpperLimit)
            {
                  if (tempSeqNo == startSeqNo)
                  {
                     updateBA = TRUE;
                     sendBA = TRUE;
                     break;
                 }
                 MacDot11nIncrementSeqNumber(&tempSeqNo);
            }
            if (updateBA)
            {
                UInt16 oldWinStartr = keyItr->second.winStartr;
                 UInt16 newWinStartr =startSeqNo;
                do
                {
                    keyItr->second.BABitmap >>= 1;
                    MacDot11nIncrementSeqNumber(&oldWinStartr);
                }while (oldWinStartr != newWinStartr);
                keyItr->second.winStartr = newWinStartr;
                UInt16 newWinEndr =  keyItr->second.winStartr;
                UInt32 i = 0;
                for (i =0; i < keyItr->second.winSizer - 1; i++)
                {
                    MacDot11nIncrementSeqNumber(&newWinEndr);
                }
                 keyItr->second.winEndr = newWinEndr;
            }
        }
    }
    if (sendBA)
    {
       DOT11n_BlockAckControlFrame *blockAckFrame
            = (DOT11n_BlockAckControlFrame*)
                            MEM_malloc(sizeof(DOT11n_BlockAckControlFrame));
            memset(blockAckFrame, 0, sizeof(DOT11n_BlockAckControlFrame));
       blockAckFrame->frameType = DOT11N_BLOCK_ACK;
       blockAckFrame->frameFlags |= DOT11_TO_DS;
       blockAckFrame->destAddr = barframe->sourceAddr;
       blockAckFrame->sourceAddr = dot11->selfAddr;
       blockAckFrame->BAControlField.BAAckPolicy
           = barframe->barControl.BARAckPolicy;
       blockAckFrame->BAControlField.MultiTID
                                            = barframe->barControl.MultiTID;
       blockAckFrame->BAControlField.CompressedBitmap
                                     = barframe->barControl.CompressedBitmap;
       blockAckFrame->BAControlField.TID_INFO
                                           = barframe->barControl.TID_INFO;
       blockAckFrame->BAInfoField.BAStartSeqNumber
                                                = keyItr->second.winStartr;

        int byteCount = 0;
        char *bitmapPointer = (char*)&keyItr->second.BABitmap;
        int i = 0;
        for (i = 0; i < MACDOT11N_BLOCK_ACK_BITMAP_SIZE; i++)
        {
            memcpy(&blockAckFrame->BAInfoField.BABitmap[byteCount],
                   bitmapPointer,
                   sizeof(UInt8));
            byteCount++;
            bitmapPointer++;
        }

       if (!barframe->barControl.BARAckPolicy)
       {
           //delayed block ack
           blockAckFrame->frameType = DOT11N_DELAYED_BLOCK_ACK;
           Message * msg = MESSAGE_Alloc(node, 0, 0, 0);
            MESSAGE_PacketAlloc(node,
                                msg,
                                sizeof(DOT11n_BlockAckControlFrame),
                                TRACE_DOT11);
            DOT11n_BlockAckControlFrame *frame
                = (DOT11n_BlockAckControlFrame*) MESSAGE_ReturnPacket(msg);
            memset(frame, 0, sizeof(DOT11n_BlockAckControlFrame));
            memcpy(frame,
                   blockAckFrame,
                   sizeof(DOT11n_BlockAckControlFrame));
            MEM_free(blockAckFrame);
            blockAckFrame = NULL;
            DOT11_FrameInfo *frameInfo
                = (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
             memset(frameInfo, 0, sizeof(DOT11_FrameInfo));
             frameInfo->msg = msg;
             frameInfo->RA = frame->destAddr;
             frameInfo->TA = frame->sourceAddr;
             frameInfo->SA = frame->sourceAddr;
             frameInfo->DA = frame->destAddr;
             frameInfo->frameType = (DOT11_MacFrameType)frame->frameType;
             frameInfo->insertTime = node->getNodeTime();
             MacDot11nMgmtQueue_EnqueuePacket(node, dot11, frameInfo);
       }
       else
       {
            // immediate block ack
           keyItr->second.blockAckVrbls->blockAckSent = TRUE;
            MacDot11nSendBlockAck(node, dot11, blockAckFrame);
       }
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nReceivePacketFromPhy
//  PURPOSE:     Processes a packet received from Phy layer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Pointer to the message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nReceivePacketFromPhy(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
 {
     dot11->IsInExtendedIfsMode = FALSE;
       MAC_PHY_TxRxVector rxVector;
        PHY_GetRxVector(node,
                        dot11->myMacData->phyNumber,
                        rxVector);
        if (rxVector.containAMPDU)
        {
            MacDot11nProcessAmpdu(node, dot11, msg);
        }
        else
        {
            DOT11n_FrameHdr* hdr =
                (DOT11n_FrameHdr*)MESSAGE_ReturnPacket(msg);
            switch (hdr->frameType)
            {
                  case DOT11_ASSOC_REQ:
                  case DOT11_ASSOC_RESP:
                  case DOT11_REASSOC_REQ:
                  case DOT11_REASSOC_RESP:
                  case DOT11_PROBE_RESP:
                  case DOT11_QOS_DATA:
                  case DOT11_AUTHENTICATION:
                  case DOT11_ACTION:
                  case DOT11_PROBE_REQ:
                      {
                          BOOL isMyframe = FALSE;
                               isMyframe
                                   = MacDot11nIsMyFrame(node,
                                                        dot11,
                                                        hdr->destAddr);
                           if (isMyframe
                                &&  hdr->qoSControl.AckPolicy
                                                  != MACDOT11N_ACK_POLICY_BA)
                           {
                               MacDot11StationCancelTimer(node, dot11);
                               MacDot11StationTransmitAck(node,
                                                          dot11,
                                                          hdr->sourceAddr);
                           }
                           
                           //follow through
                           //break;
                      }
                  case DOT11_BEACON:
                  case DOT11_RTS:
                  case DOT11_CTS:
                  case DOT11_ACK:
                       {
                           MacDot11nProcessMpdu(node, dot11, msg);
                           break;
                       }
                  case DOT11N_BLOCK_ACK_REQUEST:
                      {
                  DOT11n_BlockAckRequestFrame *barframe
                      = (DOT11n_BlockAckRequestFrame*)
                      MESSAGE_ReturnPacket(msg);
                  BOOL isMyframe
                      = MacDot11nIsMyFrame(node,
                                           dot11,
                                           barframe->destAddr);
                           if (isMyframe)
                           {
                                MacDot11StationCancelTimer(node, dot11);
                               if (!barframe->barControl.BARAckPolicy)
                               {
                                   // block ack request for delayed block ack
                                   MacDot11StationTransmitAck(
                                                       node,
                                                       dot11,
                                                       barframe->sourceAddr);
                               }
                               MacDot11nProcessBlockAckRequest(node,
                                                            dot11,
                                                            msg);
                                MESSAGE_Free(node, msg);
                           }
                           else
                           {
                               MacDot11nProcessSniffedPackets(
                                   node,
                                   dot11,
                                   msg);
                           }
                          break;
                      }
                  case DOT11N_BLOCK_ACK:
                  case DOT11N_DELAYED_BLOCK_ACK:
                      {
                           BOOL isMyframe =
                                MacDot11nIsMyFrame(node, dot11, hdr->destAddr);
                           if (hdr->sourceAddr == dot11->selfAddr)
                           {
                               ERROR_Assert(FALSE,"Invlaid Address");
                           }
                            if (isMyframe)
                            {
                                 DOT11n_BlockAckControlFrame *blockAckFrame
                                     = (DOT11n_BlockAckControlFrame*)
                                                   MESSAGE_ReturnPacket(msg);

                                 if (!blockAckFrame->
                                     BAControlField.BAAckPolicy)
                                 {
                                    //delayed block ack received
                                     MacDot11StationTransmitAck(
                                         node,
                                                       dot11,
                                                  blockAckFrame->sourceAddr);
                                 }
                                 else
                                 {
                                   // immediate block ack
                                     MacDot11StationCancelTimer(node, dot11);
                                 }
                                MacDot11nProcessBlockAck(node,
                                                         dot11,
                                                         msg);
                                MESSAGE_Free(node, msg);
                            }
                            else
                            {
                                MacDot11nProcessSniffedPackets(node,
                                                               dot11,
                                                               msg);
                            }

                         break;
                      }
                  default:
                      {
                          ERROR_Assert(FALSE,"invalid state");
                          break;
                      }
            }
        }
    if (dot11->useDvcs && dot11->state == DOT11_S_IDLE)
    {
        MacDot11StationSetPhySensingDirectionForNextOutgoingPacket(node,
                                                                   dot11);
    }
    if (MacDot11StationPhyStatus(node, dot11) == PHY_IDLE
        && dot11->state == DOT11_S_IDLE)
    {
        MacDot11StationPhyStatusIsNowIdleStartSendTimers(node, dot11);
    }
 }

//--------------------------------------------------------------------------
//  NAME:        MacDot11nInit
//  PURPOSE:     Initializes the dot11n
//  PARAMETERS:  Node* node
//                  Pointer to node
//               NetworkType networkType
//                  Network type
//               const NodeInput* nodeInput
//                 Pointer to nodeInput
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nInit(Node* node,
                   NetworkType networkType,
                   const NodeInput* nodeInput,
                   MacDataDot11* dot11)
{
    Address address;
    BOOL wasFound = FALSE;
    BOOL parameterValue = FALSE;
    NetworkGetInterfaceInfo(
        node,
        dot11->myMacData->interfaceIndex,
        &address,
        networkType);

    IO_ReadBool(node,
                node->nodeId,
                dot11->myMacData->interfaceIndex,
                nodeInput,
                "MAC-DOT11N-AMSDU-ENABLE",
                &wasFound,
                &parameterValue);
    if (wasFound)
    {
        if (parameterValue == TRUE)
        {
            dot11->isAmsduEnable = TRUE;
        }
        else
        {
           dot11->isAmsduEnable = FALSE;
        }
    }
    else
    {
       dot11->isAmsduEnable = FALSE;
    }

    IO_ReadBool(node,
                node->nodeId,
                dot11->myMacData->interfaceIndex,
                nodeInput,
                "MAC-DOT11N-AMPDU-ENABLE",
                &wasFound,
                &parameterValue);
    if (wasFound)
    {
        if (parameterValue == TRUE)
        {
            dot11->isAmpduEnable = TRUE;
        }
        else
        {
           dot11->isAmpduEnable = FALSE;
        }
    }
    else
    {
       dot11->isAmpduEnable = FALSE;
    }

    if (dot11->isAmsduEnable)
    {
       dot11->amsduBuffer = new AmsduBuffer;
    }
    dot11->inputBuffer = new InputBuffer;

    clocktype inputBufferTimerExpireInterval;
    IO_ReadTime(
            node,
            node->nodeId,
            dot11->myMacData->interfaceIndex,
            nodeInput,
            "MAC-DOT11N-REORDERING-BUFFER-TIMER-INTERVAL",
            &wasFound,
            &inputBufferTimerExpireInterval);

    if (wasFound)
    {
        if (inputBufferTimerExpireInterval <= 0)
        {
             ERROR_ReportError("Invalid Value Of Parameter"
                            " MAC-DOT11N-REORDERING-BUFFER-TIMER-INTERVAL"
                            " Specified In The Configuration File");
        }
        dot11->inputBufferTimerExpireInterval
            = inputBufferTimerExpireInterval;
    }
    else
    {
       dot11->inputBufferTimerExpireInterval = 100*MILLI_SECOND;
    }

    clocktype amsduBufferTimerExpireInterval;
    IO_ReadTime(
            node,
            node->nodeId,
            dot11->myMacData->interfaceIndex,
            nodeInput,
            "MAC-DOT11N-AMSDU-BUFFER-TIMER-INTERVAL",
            &wasFound,
            &amsduBufferTimerExpireInterval);

    if (wasFound)
    {
        if (amsduBufferTimerExpireInterval <= 0)
        {
             ERROR_ReportError("Invalid Value Of Parameter"
                 " MAC-DOT11N-AMSDU-BUFFER-TIMER-INTERVAL Specified"
                 " In The Configuration File");
        }
        dot11->amsduBufferTimerExpireInterval
            = amsduBufferTimerExpireInterval;
    }
    else
    {
       dot11->amsduBufferTimerExpireInterval = 5*MILLI_SECOND;
    }

    int macOutputQueueSize;
    IO_ReadInt(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11N-QUEUE-SIZE-PER-DESTINATION",
        &wasFound,
        &macOutputQueueSize);

    if (wasFound)
    {
        if (macOutputQueueSize <= 0)
        {
             ERROR_ReportError("Invalid Value Of Parameter"
                 " MAC-DOT11N-QUEUE-SIZE-PER-DESTINATION Specified"
                 " In The Configuration File");
        }
        dot11->macOutputQueueSize = macOutputQueueSize;
    }
    else
    {
       dot11->macOutputQueueSize = DEFAULT_MAC_QUEUE_SIZE;
    }

    BOOL enableBigAmsdu = FALSE;
    IO_ReadBool(node,
                node->nodeId,
                dot11->myMacData->interfaceIndex,
                nodeInput,
                "MAC-DOT11N-ENABLE-BIG-AMSDU",
                &wasFound,
                &enableBigAmsdu);
    if (wasFound)
    {
        if (enableBigAmsdu == TRUE)
        {
            dot11->enableBigAmsdu = TRUE;
            dot11->maxAmsduSize = AMSDU_SIZE_2;
        }
        else
        {
           dot11->enableBigAmsdu = FALSE;
            dot11->maxAmsduSize = AMSDU_SIZE_1;
        }
    }
    else
    {
       dot11->enableBigAmsdu = FALSE;
        dot11->maxAmsduSize = AMSDU_SIZE_1;
    }

    int ampduLengthExponent;
    IO_ReadInt(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11N-AMPDU-LENGTH-EXPONENT",
        &wasFound,
        &ampduLengthExponent);

    if (wasFound)
    {
        if (ampduLengthExponent < 0 || ampduLengthExponent > 3)
        {
              ERROR_ReportError("Invalid Value Of Parameter"
                 " MAC-DOT11N-AMPDU-LENGTH-EXPONENT Specified"
                 " In The Configuration File");
        }
        else
        {
            dot11->ampduLengthExponent = (UInt8)ampduLengthExponent;
        }
    }
    else
    {
       dot11->ampduLengthExponent = 1;
    }
    dot11->maxAmpduSize =
       MacDot11nCalculateMaxAmpduLength(dot11->ampduLengthExponent);

    BOOL enableImmediateBAAgreement;
    IO_ReadBool(node,
                node->nodeId,
                dot11->myMacData->interfaceIndex,
                nodeInput,
                "MAC-DOT11N-ENABLE-DATA-BURSTING",
                &wasFound,
                &enableImmediateBAAgreement);
    if (wasFound)
    {
        if (enableImmediateBAAgreement == TRUE)
        {
            dot11->enableImmediateBAAgreement = TRUE;
            if (dot11->isAmsduEnable)
            {
                char buf[100];
                sprintf(buf," Node %d: Amsdu Creation Is Currently Not"
                    " Supported With Data Burst. Disabling Amsdu Creation.",
                    node->nodeId);
                ERROR_ReportWarning(buf);
            dot11->isAmsduEnable = FALSE;
            }
             if (dot11->isAmpduEnable)
            {
                  char buf[100];
                sprintf(buf," Node %d: Ampdu Creation Is Currently Not"
                    " Supported With Data Burst. Disabling Ampdu Creation.",
                    node->nodeId);
                ERROR_ReportWarning(buf);
            dot11->isAmpduEnable = FALSE;
            }
            BOOL enableDelayedBAAgreement;
            IO_ReadBool(node,
                        node->nodeId,
                        dot11->myMacData->interfaceIndex,
                        nodeInput,
                        "MAC-DOT11N-ENABLE-DELAYED-BLOCK-ACK",
                        &wasFound,
                        &enableDelayedBAAgreement);
                if (wasFound)
                {
                    if (enableDelayedBAAgreement == TRUE)
                    {
                        dot11->enableDelayedBAAgreement = TRUE;
                    }
                    else
                    {
                       dot11->enableDelayedBAAgreement = FALSE;
                    }
                }
                else
                {
                   dot11->enableDelayedBAAgreement = FALSE;
                }

                BOOL rifsMode = FALSE;
                IO_ReadBool(node,
                            node->nodeId,
                            dot11->myMacData->interfaceIndex,
                            nodeInput,
                            "MAC-DOT11N-ENABLE-RIFS-MODE",
                            &wasFound,
                            &rifsMode);
                if (wasFound)
                {
                    if (rifsMode == TRUE)
                    {
                        dot11->rifsMode = TRUE;
                    }
                    else
                    {
                       dot11->rifsMode = FALSE;
                    }
                }
                else
                {
                   dot11->rifsMode = FALSE;
                }

                clocktype blockAckPolicyTimeout;
                IO_ReadTime(
                    node,
                    node->nodeId,
                    dot11->myMacData->interfaceIndex,
                    nodeInput,
                    "MAC-DOT11N-BLOCK-ACK-POLICY-TIMEOUT",
                    &wasFound,
                    &blockAckPolicyTimeout);
                if (wasFound)
                {
                    if (blockAckPolicyTimeout <= 0)
                    {
                        ERROR_ReportError("Invalid Value Of Parameter"
                            " MAC-DOT11N-BLOCK-ACK-POLICY-TIMEOUT Specified"
                            " In The Configuration File");
                    }
                    else
                    {
                        dot11->blockAckPolicyTimeout = blockAckPolicyTimeout;
                    }
                }
                else
                {
                    dot11->blockAckPolicyTimeout = BLOCK_ACK_POLICY_TIMEOUT;
                }
        }
        else
        {
           dot11->enableImmediateBAAgreement = FALSE;
           dot11->enableDelayedBAAgreement = FALSE;
        }
    }
    else
    {
       dot11->enableImmediateBAAgreement = FALSE;
       dot11->enableDelayedBAAgreement = FALSE;
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nResetInputBuffer
//  PURPOSE:     Resets the input buffer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               BOOL updateStats
//                  Specifies if statistics needs to be updated
//               BOOL freeInputBuffer
//                  Specifies if buffer needs to freed
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void
MacDot11nResetInputBuffer(Node* node,
                 MacDataDot11* dot11,
                 BOOL updateStats,
                 BOOL freeInputBuffer)
{
        std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr
             = dot11->inputBuffer->InputBufferMap.begin();
        int debugCount = dot11->inputBuffer->InputBufferMap.size();
        while (keyItr != dot11->inputBuffer->InputBufferMap.end())
         {
             keyItr->second.aggSize = 0;
             if (keyItr->second.timerMsg)
            {
                MESSAGE_CancelSelfMsg(node,
                                    keyItr->second.timerMsg);
                keyItr->second.timerMsg = NULL;
            }
             keyItr->second.winStarts = 0;
             keyItr->second.winEnds = 63;
             keyItr->second.winSizes = 64;
             keyItr->second.aggSize = 0;
             keyItr->second.seqNum = 0;
             keyItr->second.winStartr = 0;
             keyItr->second.winEndr = 0;
             keyItr->second.winSizer = 0;
             if (keyItr->second.blockAckVrbls)
             {
                MacDot11nResetbap(keyItr);
                if (freeInputBuffer)
                {
                    MEM_free(keyItr->second.blockAckVrbls);
                    keyItr->second.blockAckVrbls = NULL;
                }
             }
             if (!(keyItr->second.frameQueue.empty()))
            {
                std::list<DOT11_FrameInfo*>::iterator listItr;
                listItr = keyItr->second.frameQueue.begin();
                 while (listItr != keyItr->second.frameQueue.end())
                 {
                     DOT11_FrameInfo *frameInfo
                    = MacDot11nRemoveReferenceOfFrameFromMap(keyItr,listItr);
                     MESSAGE_Free(node, frameInfo->msg);
                     frameInfo->msg = NULL;
                     MEM_free(frameInfo);
                     frameInfo = NULL;
                     keyItr->second.numPackets--;
                     if (updateStats)
                     {
                         // do nothing
                    }
                    listItr = keyItr->second.frameQueue.begin();
                 }
            }
            keyItr++;
            debugCount--;
         }
         if (freeInputBuffer)
         {
            dot11->inputBuffer->InputBufferMap.clear();
            delete dot11->inputBuffer;
            dot11->inputBuffer = NULL;
         }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nResetAmsduBuffer
//  PURPOSE:     Resets the amsdu buffer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               BOOL updateStats
//                  Specifies if statistics needs to be updated
//               BOOL freeAmsduBuffer
//                  Specifies if buffer needs to freed
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void
MacDot11nResetAmsduBuffer(Node* node,
                 MacDataDot11* dot11,
                 BOOL updateStats,
                 BOOL freeAmsduBuffer)
{
    if (dot11->isAmsduEnable)
    {
        std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr
            = dot11->amsduBuffer->AmsduBufferMap.begin();
        int debugCount = dot11->amsduBuffer->AmsduBufferMap.size();
        while (keyItr != dot11->amsduBuffer->AmsduBufferMap.end())
        {
            keyItr->second.aggSize = 0;
            if (keyItr->second.timerMsg)
            {
                MESSAGE_CancelSelfMsg(node, keyItr->second.timerMsg);
                keyItr->second.timerMsg = NULL;
            }
            if (!(keyItr->second.frameQueue.empty()))
            {
                std::list<DOT11_FrameInfo*>::iterator listItr;
                listItr = keyItr->second.frameQueue.begin();
                while (listItr != keyItr->second.frameQueue.end())
                {
                     DOT11_FrameInfo *frameInfo
                        = MacDot11nRemoveReferenceOfFrameFromMap(keyItr,
                                                                 listItr);
                     MESSAGE_Free(node, frameInfo->msg);
                     frameInfo->msg = NULL;
                     MEM_free(frameInfo);
                     frameInfo = NULL;
                     keyItr->second.numPackets--;
                     if (updateStats)
                     {
                        dot11->numPktsDroppedFromAmsduBuffer++;
                     }
                    listItr = keyItr->second.frameQueue.begin();
                }
            }
            keyItr++;
            debugCount--;
        }
        if (freeAmsduBuffer)
        {
            dot11->amsduBuffer->AmsduBufferMap.clear();
            delete dot11->amsduBuffer;
            dot11->amsduBuffer = NULL;
        }
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nResetOutputBuffer
//  PURPOSE:     Resets the output buffer
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               BOOL updateStats
//                  Specifies if statistics needs to be updated
//               BOOL freeAmsduBuffer
//                  Specifies if buffer needs to freed
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void
MacDot11nResetOutputBuffer(Node* node,
                 MacDataDot11* dot11,
                 BOOL updateStats,
                 BOOL freeOutputBuffer)
{
    int i = 0;
    for (i=0; i < DOT11e_NUMBER_OF_AC; i++)
    {
        OutputBuffer *outputBuffer = dot11->ACs[i].outputBuffer;
        if (outputBuffer->OutputBufferMap.empty())
        {
            continue;
        }
        std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
        keyItr = outputBuffer->OutputBufferMap.begin();
        int debugCount = outputBuffer->OutputBufferMap.size();
        while (keyItr != outputBuffer->OutputBufferMap.end())
        {
            keyItr->second.winStarts = 0;
            keyItr->second.winEnds = MACDOT11N_INVALID_SEQ_NUM;
            keyItr->second.winSizes = 0;
            keyItr->second.aggSize = 0;
            keyItr->second.seqNum = 0;
            if (keyItr->second.blockAckVrbls)
            {
                MacDot11nResetbap(keyItr);
                if (freeOutputBuffer)
                {
                    MEM_free(keyItr->second.blockAckVrbls);
                    keyItr->second.blockAckVrbls = NULL;
                }
            }
            if (!(keyItr->second.frameQueue.empty()))
            {
                std::list<DOT11_FrameInfo*>::iterator listItr;
                listItr = keyItr->second.frameQueue.begin();
                while (listItr != keyItr->second.frameQueue.end())
                {
                    DOT11_FrameInfo *frameInfo
                        = MacDot11nRemoveReferenceOfFrameFromMap(keyItr,
                                                                 listItr);
                    MESSAGE_Free(node, frameInfo->msg);
                    frameInfo->msg = NULL;
                    MEM_free(frameInfo);
                    frameInfo = NULL;
                    keyItr->second.numPackets--;
                    if (updateStats)
                    {
                        dot11->numPktsDroppedFromOutputBuffer++;
                    }
                    listItr = keyItr->second.frameQueue.begin();
                }
            }
            keyItr++;
            debugCount--;
         }
         ERROR_Assert(debugCount == 0, "invalid debug count");
         dot11->ACs[i].isACHasPacket = FALSE;
         MacDot11nResetACVariables(dot11, i, FALSE);
         if (freeOutputBuffer)
         {
            outputBuffer->OutputBufferMap.clear();
            delete outputBuffer;
            outputBuffer = NULL;
         }
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nManagementIncrementSendFrame.
//  PURPOSE:     Updates the statistics for management frames
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_FrameInfo *frameInfo
//                  Pointer to the frameInfo
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void
MacDot11nManagementIncrementSendFrame(MacDataDot11* dot11,
                                     DOT11_FrameInfo *frameInfo)
{
     DOT11_ManagementVars* mngmtVars =
            (DOT11_ManagementVars*) dot11->mngmtVars;

     switch(frameInfo->frameType)
     {
         case DOT11_ASSOC_REQ:
            mngmtVars->assocReqSend++;
            break;
         case DOT11_ASSOC_RESP:
             mngmtVars->assocRespSend++;
             break;
         case DOT11_REASSOC_REQ:
             mngmtVars->reassocReqSend++;
             break;
         case DOT11_REASSOC_RESP:
            mngmtVars->reassocRespSend++;
            break;
         case DOT11_PROBE_REQ:
            mngmtVars->ProbReqSend++;
            break;
         case DOT11_PROBE_RESP:
             mngmtVars->ProbRespSend++;
             break;
         case DOT11_AUTHENTICATION:
             if (MacDot11IsAp(dot11))
             {
                 mngmtVars->AuthRespSend++;
             }
             else
             {
                mngmtVars->AuthReqSend++;
             }
             break;
         case DOT11_ACTION:
             {
                DOT11n_ADDBARequestFrame* actionFrame =
                (DOT11n_ADDBARequestFrame*)MESSAGE_ReturnPacket(frameInfo->msg);
                switch(actionFrame->action)
                {
                    case DOT11n_ADDBA_Request:
                        mngmtVars->ADDBSRequestSent++;
                        break;
                    case DOT11n_ADDBA_Response:
                        mngmtVars->ADDBSResponseSent++;
                    break;
                    case DOT11n_DELBA:
                        mngmtVars->DELBARequestSent++;
                    break;
                    default:
                        ERROR_Assert(FALSE,"invalid state");
                    break;
                }
             break;
             }
         default:
             break;
     }
}//MacDot11nManagementIncrementSendFrame

//--------------------------------------------------------------------------
//  NAME:        MacDot11nSendMpdu.
//  PURPOSE:     Sends an mpdu/ampdu to Phy layer
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               BOOL useRifs
//                  Specifies if rifs should be used instead of sifs
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nSendMpdu(
    Node* node,
    MacDataDot11* dot11,
    BOOL useRifs)
{
    Mac802Address destAddr;
    MAC_PHY_TxRxVector txVector;
    ERROR_Assert(dot11->currentMessage != NULL,
        "MacDot11StationTransmitDataFrame: "
        "There is no message in the local buffer.\n");
    destAddr = dot11->currentNextHopAddress;
    int pktSize = DOT11nMessage_ReturnPacketSize(dot11->currentMessage);
    pktSize += dot11->dot11TxFrameInfo->ampduOverhead;
    MAC_PHY_TxRxVector tempTxVector;
    tempTxVector = MacDot11nSetTxVector(
                       node,
                       dot11,
                       pktSize,
                       dot11->dot11TxFrameInfo->frameType,
                       destAddr,
                       NULL);
    if (dot11->dot11TxFrameInfo->isAMPDU)
    {
        tempTxVector.containAMPDU = TRUE;
    }

    PHY_SetTxVector(node,
                    dot11->myMacData->phyNumber,
                    tempTxVector);
    PHY_GetTxVector(node,
                    dot11->myMacData->phyNumber,
                    txVector);
    ERROR_Assert(dot11->currentMessage != NULL,
                 "MacDot11StationTransmitDataFrame: "
                 "There is no message in the local buffer.\n");
    if (destAddr == ANY_MAC802)
    {
        Message* dequeuedPacket = NULL;
        DOT11_FrameHdr* hdr;
        if (MacDot11IsIBSSStationSupportPSMode(dot11))
        {
            ERROR_Assert(FALSE,"invalid state");
        }
        else
        {
            dequeuedPacket = MESSAGE_Duplicate(node, dot11->currentMessage);
        }
        ERROR_Assert(dot11->dot11TxFrameInfo,"frame info not present");
        if (dot11->dot11TxFrameInfo->isAMPDU)
        {
            ERROR_Assert(FALSE,"invalid");
        }
        else
        {
            hdr = (DOT11_FrameHdr*)MESSAGE_ReturnPacket(dequeuedPacket);
            if (!dot11->beaconSet)
            {
                hdr->duration = 0;
            }
        }
        if (MacDot11IsAp(dot11))
        {
            if (dot11->dot11TxFrameInfo->frameType == DOT11_BEACON)
            {
                if (DEBUG_BEACON)
                {
                     char timeStr[100];
                     ctoa(node->getNodeTime(), timeStr);
                     printf("\n%d Transmitting Beacon "
                           "\t  at %s n",
                            node->nodeId,
                            timeStr);
                }
             MacDot11StationSetState(node, dot11, DOT11_X_BEACON);
            }
            else
            {
             MacDot11StationSetState(node, dot11, DOT11_X_BROADCAST);
            }
        }
        else
        {
            MacDot11StationSetState(node, dot11, DOT11_X_BROADCAST);
        }
        ERROR_Assert(txVector.mcs == dot11->txVectorForBC.mcs,
                 "Wrong data rate");
        MacDot11StationStartTransmittingPacket(
                                            node,
                                            dot11,
                                            dequeuedPacket,
                                            dot11->delayUntilSignalAirborn);
    }
    else
    {
        clocktype transmitDelay = 0;
        Message* pktToPhy = MESSAGE_Duplicate(node, dot11->currentMessage);
        if (dot11->dot11TxFrameInfo!= NULL)
        {
            if ((dot11->dot11TxFrameInfo->frameType == DOT11_DATA) ||
                ((dot11->dot11TxFrameInfo->frameType >=DOT11_ASSOC_REQ) &&
                (dot11->dot11TxFrameInfo->frameType <= DOT11_ACTION)))
            {
                MacDot11nManagementIncrementSendFrame(
                                                dot11,
                                                dot11->dot11TxFrameInfo);
                dot11->TID = 0;
                dot11->QosFrameSent = TRUE;
            }
            else
            {
                dot11->QosFrameSent = TRUE;
                if (dot11->dot11TxFrameInfo->frameType
                    == DOT11N_BLOCK_ACK_REQUEST)
                {
                    DOT11n_BlockAckRequestFrame *baFrame
                       = (DOT11n_BlockAckRequestFrame*)
                          MESSAGE_ReturnPacket(dot11->dot11TxFrameInfo->msg);
                   switch(baFrame->barControl.BARAckPolicy)
                   {
                       case BA_IMMEDIATE:
                            dot11->totalNoImmBlockAckRequestSent++;
                            break;
                        case BA_DELAYED:
                        dot11->totalNoDelBlockAckRequestSent++;
                        break;
                   }
                }
                else if (dot11->dot11TxFrameInfo->frameType
                    == DOT11N_DELAYED_BLOCK_ACK)
                {
                    dot11->totalNoDelBlockAckSent++;
                }
            }
        }
        dot11->waitingForAckOrCtsFromAddress = destAddr;
        if (!dot11->dot11TxFrameInfo->isAMPDU)
        {
            DOT11n_FrameHdr* hdr
                = (DOT11n_FrameHdr*)MESSAGE_ReturnPacket(pktToPhy);
            if (dot11->dot11TxFrameInfo->baActive)
            {
                 hdr->duration
                    = (UInt16)MacDot11NanoToMicrosecond(
                                            MacDot11nGetNavDuration(node,
                                            dot11,
                                            dot11->dot11TxFrameInfo));
            }
            else
            {
                MAC_PHY_TxRxVector tempTxVector;
                tempTxVector = txVector;
                tempTxVector.length = DOT11_SHORT_CTRL_FRAME_SIZE;
                hdr->duration = (UInt16)
                MacDot11NanoToMicrosecond(
                    dot11->extraPropDelay
                    + dot11->sifs
                    + PHY_GetTransmissionDuration(node,
                                                  dot11->myMacData->phyNumber,
                                                  tempTxVector)
                    + dot11->extraPropDelay);
            }
        }
        else
        {
            // cant add duration in an ampdu
        }
        int fragThreshold = 0;
        fragThreshold = DOT11N_FRAG_THRESH;
        if (pktSize >
            fragThreshold)
        {
            // Fragmentation Needs to be rewritten. (Jay)
            ERROR_ReportError("MacDot11StationTransmitDataFrame: "
                "Fragmentation is not currently supported.\n");
        }
        else
        {
            DOT11_MacFrameType frameType;
            frameType = dot11->dot11TxFrameInfo->frameType;
            MacDot11StationSetState(node, dot11, DOT11_X_UNICAST);
            // RTS Threshold should be applied on Mac packet
            // uddate stats regarding ampdu
            if (MAC_InterfaceIsEnabled(node,
                                   dot11->myMacData->interfaceIndex))
            {
                if (dot11->dot11TxFrameInfo->frameType == DOT11_QOS_DATA)
                {
                    dot11->totalNoOfQoSDataFrameSend++;
                }
            }
            if (pktSize >
                dot11->rtsThreshold)
            {
                // Since using RTS-CTS, data packets have to wait
                // an additional SIFS.
                if (useRifs)
                {
                   transmitDelay += dot11->rifs;
                }
                else
                {
                   transmitDelay += dot11->sifs;
                }
            }
            else
            {
                // Not using RTS-CTS, so don't need to wait for SIFS
                // since already waited for DIFS or BO.
                if (useRifs)
                {
                   transmitDelay += dot11->rifs;
                }
                else
                {
                   transmitDelay += dot11->delayUntilSignalAirborn;
                }
            }
            if (DEBUG_AMPDU && dot11->dot11TxFrameInfo->isAMPDU)
            {
                printf("%" TYPES_64BITFMT "d Node %d Sending Ampdu"
                    " with mcs %d length %" TYPES_SIZEOFMFT 
                    "d gi %s ChBandwidth %s \n",
                    node->getNodeTime(),
                    node->nodeId,
                    tempTxVector.mcs,
                    tempTxVector.length,
                    tempTxVector.gi ? "Gi_Short":"Gi_Long",
                    tempTxVector.chBwdth ? "40Mhz":"20Mhz");
            }
            else if (DEBUG_AMPDU || DEBUG_AMSDU || DEBUG_DATA_BURST)
            {
                printf("%" TYPES_64BITFMT "d Node %d Sending Mpdu of type %d"
                    " with mcs %d length %" TYPES_SIZEOFMFT 
                    "d gi %s ChBandwidth %s \n",
                    node->getNodeTime(),
                    node->nodeId,
                    dot11->dot11TxFrameInfo->frameType,
                    tempTxVector.mcs,
                    tempTxVector.length,
                    tempTxVector.gi ? "Gi_Short":"Gi_Long",
                    tempTxVector.chBwdth ? "40Mhz":"20Mhz");
            }
            if (!dot11->useDvcs)
            {
                MacDot11StationStartTransmittingPacket(
                    node, dot11, pktToPhy, transmitDelay);
            }
            else
            {
                BOOL isCached = FALSE;
                double directionToSend = 0.0;
                AngleOfArrivalCache_GetAngle(
                    &dot11->directionalInfo->angleOfArrivalCache,
                    destAddr,
                    node->getNodeTime(),
                    &isCached,
                    &directionToSend);
                if (isCached)
                {
                    MacDot11StationStartTransmittingPacketDirectionally(
                        node, dot11, pktToPhy, transmitDelay,
                        directionToSend);

                    PHY_LockAntennaDirection(node,
                                             dot11->myMacData->phyNumber);
                }
                else
                {
                    // Don't know which way to send, send omni.
                    MacDot11StationStartTransmittingPacket(node,
                                                           dot11,
                                                           pktToPhy,
                                                           transmitDelay);
                }
            }
        }
    }
}// MacDot11StationTransmitDataFrame


//--------------------------------------------------------------------------
//  NAME:        MacDot11nCheckForOutgoingPacket.
//  PURPOSE:     Checks if more packets pending to be sent
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               BOOL backoff
//                  Specifies if Mac should perform backoff before sending
//                  next packet
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nCheckForOutgoingPacket(
    Node* node,
    MacDataDot11* dot11,
    BOOL backoff)
{
    if (backoff)
    {
      MacDot11StationSetBackoffIfZero(node, dot11,dot11->currentACIndex);
    }
    if (dot11->ManagementResetPending == TRUE)
    {
        dot11->ManagementResetPending = FALSE;
        MacDot11ManagementReset(node, dot11);
    }
    if (dot11->stationAssociated
        && (!MacDot11IsAp(dot11))
        && (dot11->associatedAP->rssMean < dot11->thresholdSignalStrength)
        && dot11->state == DOT11_S_IDLE
        && dot11->currentMessage == NULL
        && dot11->ScanStarted == FALSE)
    {
        MacDot11nResetOutputBuffer(node, dot11, TRUE, FALSE);
        MacDot11nResetAmsduBuffer(node, dot11, TRUE, FALSE);
        MacDot11nResetCurrentMessageVariables(dot11);
        MacDot11ManagementStartJoin(node, dot11);
    }
    BOOL mgmtPacketTobeSent = FALSE;
    mgmtPacketTobeSent =
        MacDot11nStaMoveAPacketFromTheMgmtQueueToTheLocalBuffer(node, dot11);
   if (MacDot11IsStationJoined(dot11) &&  !dot11->ScanStarted )
   {
         MacDot11StationMoveAPacketFromTheNetworkLayerToTheLocalBuffer(
            node,
            dot11);
   }
     if (dot11->currentMessage == NULL && MacDot11IsACsHasMessage(node,dot11))
    {
        if (dot11->state == DOT11_S_IDLE)
        {
            MacDot11eSetCurrentMessage(node,dot11);
        }
    }
    if ((dot11->beaconIsDue)
        && (MacDot11StationCanHandleDueBeacon(node, dot11)))
    {
        return;
    }
    if (dot11->currentMessage == NULL)
    {
        return;
    }
    if (backoff
        || dot11->BO != 0
        || MacDot11StationHasFrameToSend(dot11))
    {
        MacDot11StationSetBackoffIfZero(node, dot11,dot11->currentACIndex);
        MacDot11StationAttemptToGoIntoWaitForDifsOrEifsState(node, dot11);
    }
}// MacDot11nStationCheckForOutgoingPacket

//--------------------------------------------------------------------------
//  NAME:        MacDot11nBroadcastTramsmitted.
//  PURPOSE:     Handles the control for dot11n when a broadcast packet
//               is transmitted
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nBroadcastTramsmitted(Node* node,
                                   MacDataDot11* dot11)
{
    if ((dot11->dot11TxFrameInfo->frameType >= DOT11_ASSOC_REQ)
        &&(dot11->dot11TxFrameInfo->frameType <= DOT11_ACTION))
    {
        MacDot11ManagementIncrementSendFrame(
                                        node,
                                        dot11,
                                        dot11->dot11TxFrameInfo->frameType);
        MacDot11ManagementFrameTransmitted(node, dot11);
    }
    if (dot11->dot11TxFrameInfo->frameType == DOT11_PROBE_REQ)
    {
        DOT11_ManagementVars * mngmtVars
            = (DOT11_ManagementVars*) dot11->mngmtVars;
        MacDot11ManagementStartTimerOfGivenType(
                     node,
                     dot11,
                     mngmtVars->channelInfo->dwellTime,
                     MSG_MAC_DOT11_Active_Scan_Short_Timer);
    }
    if (dot11->dot11TxFrameInfo->frameType == DOT11_QOS_DATA)
    {
        DOT11n_FrameHdr* hdr
            = (DOT11n_FrameHdr*)MESSAGE_ReturnPacket(dot11->dot11TxFrameInfo->msg);
        MacDot11nUpdateOutputBuffer(node,
                                    dot11,
                                    hdr->destAddr,
                                    hdr->qoSControl.TID,
                                    hdr->seqNo);
    }
    MESSAGE_Free(node, dot11->currentMessage);
    dot11->ACs[dot11->currentACIndex].totalNoOfthisTypeFrameDeQueued++;
    dot11->broadcastPacketsSentDcf++;
    MacDot11nResetACVariables(dot11, dot11->currentACIndex);
    MacDot11nResetCurrentMessageVariables(dot11);
    MacDot11StationResetCW(node, dot11);
    MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
    MacDot11StationCheckForOutgoingPacket(node,
                                          dot11,
                                          TRUE);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nBlockAckPolicyExists.
//  PURPOSE:     Checks if block ack policy is exists
//  PARAMETERS:  MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int acIndex
//                  Index of the access category
//  RETURN:      BOOL
//                  TRUE if block ack policy exists, FALSE otherwise
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL
 MacDot11nBlockAckPolicyExists(MacDataDot11* dot11,
                                     int acIndex)
 {
     if (acIndex < DOT11e_AC_VI)
     {
         return FALSE;
     }
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = outputBuffer->roundRobinKeyItr;
    keyItr++;
    if (keyItr == outputBuffer->OutputBufferMap.end())
    {
       keyItr = outputBuffer->OutputBufferMap.begin();
    }
    int count = outputBuffer->OutputBufferMap.size();
    while (count > 0)
    {
        count --;
        if (!(keyItr->second.frameQueue.empty()))
        {
            break;
        }
        keyItr++;
        if (keyItr == outputBuffer->OutputBufferMap.end())
        {
           keyItr = outputBuffer->OutputBufferMap.begin();
        }
    }
    if (keyItr->second.blockAckVrbls->state
                        == BAA_STATE_IDLE
                  || keyItr->second.blockAckVrbls->state
                        == BAA_STATE_TRANSMITTING)
    {
        return TRUE;
    }
    return FALSE;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nBlockAckPolicyAvailable.
//  PURPOSE:     Checks if block ack policy is available
//  PARAMETERS:  int acIndex
//                  Index of the access category
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator keyItr
//                  Pointer to the key
//  RETURN:      BOOL
//                  TRUE if block ack policy available, FALSE otherwise
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL
 MacDot11nBlockAckPolicyAvailable(int acIndex,
                                  std::map<std::pair<Mac802Address,
                                  TosType>,
                                  MapValue>::iterator keyItr)
{
    if (acIndex >= 2 && keyItr->second.blockAckVrbls)
    {
        return TRUE;
    }
    return FALSE;
 }

//--------------------------------------------------------------------------
//  NAME:        MacDot11nBlockAckPolicyUsable.
//  PURPOSE:     Checks if block ack policy can be used to send mpdus
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator keyItr
//                  Pointer to the key
//               UInt8 *numPackets
//                  Num packets that can be sent under block ack policy
//               BOOL reCalculating
//                  Specifies if we are recalculating the usability
//  RETURN:      BOOL
//                  TRUE if block ack policy can be used, FALSE otherwise
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL
MacDot11nBlockAckPolicyUsable(Node* node,
                              MacDataDot11* dot11,
                              std::map<std::pair<Mac802Address,
                              TosType>,
                              MapValue>::iterator keyItr,
                              UInt8 *numPackets,
                              BOOL reCalculating = FALSE)
{
    if (keyItr->second.numPackets > BLOCK_ACK_POLICY_THRESHOLD)
    {
        UInt8 minMcsIndex = 0;
        if (MacDot11IsAp(dot11))
        {
            DOT11_ApStationListItem* stationItem = NULL;
            stationItem
                = MacDot11ApStationListGetItemWithGivenAddress(
                                                   node,
                                                   dot11,
                                                   keyItr->first.first);
            UInt8 maxMcs
                = std::min(stationItem->data->staHtCapabilityElement.
                    supportedMCSSet.mcsSet.maxMcsIdx,
                    (UInt8)(Phy802_11nGetNumAtnaElems(
                        node->phyData[dot11->myMacData->phyNumber])*8 - 1));
            minMcsIndex = maxMcs - 7;
        }
        else
        {
            if (dot11->stationType == DOT11_STA_IBSS)
            {
                DOT11n_IBSS_Station_Info *stationInfo
                    = MacDot11nGetIbssStationInfo(dot11,
                                                  keyItr->first.first);
                if (stationInfo->probeStatus == IBSS_PROBE_COMPLETED)
                {
                    UInt8 maxMcs
                        = std::min(stationInfo->staHtCapabilityElement.
                            supportedMCSSet.mcsSet.maxMcsIdx,
                            (UInt8)(Phy802_11nGetNumAtnaElems(
                        node->phyData[dot11->myMacData->phyNumber])*8 - 1));
                    minMcsIndex = maxMcs - 7;
                }
                else
                {
                    ERROR_Assert(FALSE,"Invalid State");
                }
            }
            else
            {
                 UInt8 maxMcs
                    = std::min(dot11->associatedAP->staHtCapabilityElement.
                        supportedMCSSet.mcsSet.maxMcsIdx,
                        (UInt8)(Phy802_11nGetNumAtnaElems(
                        node->phyData[dot11->myMacData->phyNumber])*8 - 1));
                minMcsIndex = maxMcs - 7;

            }
        }
        ERROR_Assert(minMcsIndex == 0
            || minMcsIndex == 8
            || minMcsIndex == 16
            || minMcsIndex == 24,
            "not Min MCS");
        std::list<struct DOT11_FrameInfo*>::iterator listItr
            = keyItr->second.frameQueue.begin();
        UInt8 numPkts = 0;
        UInt8 acIndex
         = (UInt8)MacDot11ReturnAccessCategory(keyItr->first.second);
        while (listItr != keyItr->second.frameQueue.end()
                && numPkts < MAX_MPDUS_NEGOTIATED_VIA_BA_POLICY
                && (*listItr)->state == STATE_FRESH)
        {
            int pktsize = DOT11nMessage_ReturnPacketSize((*listItr)->msg);
            MAC_PHY_TxRxVector tempTxVector;
            tempTxVector = MacDot11nSetTxVector(
                               node,
                               dot11,
                               pktsize,
                               DOT11_QOS_DATA,
                               keyItr->first.first,
                               NULL);
            tempTxVector.mcs = minMcsIndex;
            clocktype duration = PHY_GetTransmissionDuration(
            node, dot11->myMacData->phyNumber,
            tempTxVector);
            if (duration < dot11->ACs[acIndex].TXOPLimit)
            {
                numPkts++;
                listItr++;
            }
            else
            {
                break;
            }
        }
        if (numPkts > BLOCK_ACK_POLICY_THRESHOLD)
        {
            ERROR_Assert(numPkts
                <= MAX_MPDUS_NEGOTIATED_VIA_BA_POLICY,
                " num packets are greater than mpdu's negotiated");
            *numPackets = numPkts;
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nSendAddbaRequest.
//  PURPOSE:     Sends an addba request
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator keyItr
//                  Pointer to the key
//               UInt8 numPackets
//                  Num packets to can be sent under block ack policy
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void
MacDot11nSendAddbaRequest(Node* node,
                          MacDataDot11* dot11,
                          std::map<std::pair<Mac802Address,
                          TosType>,
                          MapValue>::iterator keyItr,
                          UInt8 numPackets)
{
    keyItr->second.blockAckVrbls->state
        = BAA_STATE_ADDBA_REQUEST_QUEUED;
    Message *msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(DOT11n_ADDBARequestFrame),
                        TRACE_DOT11);
     DOT11n_ADDBARequestFrame* addbaFrame
         = (DOT11n_ADDBARequestFrame*) MESSAGE_ReturnPacket(msg);
     memset(addbaFrame, 0, sizeof(DOT11n_ADDBARequestFrame));
    addbaFrame->frameType = DOT11_ACTION;
    addbaFrame->frameFlags |= DOT11_TO_DS;
    addbaFrame->destAddr = keyItr->first.first;
    addbaFrame->sourceAddr = dot11->selfAddr;
    addbaFrame->fragId = 0;
    addbaFrame->seqNo = 0;
    addbaFrame->category = CATEGORY_BLOCK_ACK;
    addbaFrame->action = DOT11n_ADDBA_Request;
    addbaFrame->blockAckParams.AMSDUSupported = dot11->isAmsduEnable;
    addbaFrame->blockAckParams.BAPolicy = keyItr->second.blockAckVrbls->type;
    addbaFrame->blockAckParams.TID = keyItr->first.second;
    addbaFrame->blockAckParams.BufferSize = numPackets;
    addbaFrame->blockAckTimeout
        = MacDot11ClocktypeToTUs(dot11->blockAckPolicyTimeout);
    addbaFrame->startingSeqControl.FragId = 0;
    addbaFrame->startingSeqControl.SeqNo = keyItr->second.winStarts;
    
    //duration will be sent while transmitting this frame
    addbaFrame->duration = 0;
    DOT11_FrameInfo *frameInfo
        = (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
    memset(frameInfo, 0, sizeof(DOT11_FrameInfo));
    frameInfo->msg = msg;
    frameInfo->RA = addbaFrame->destAddr;
    frameInfo->TA = addbaFrame->sourceAddr;
    frameInfo->SA = addbaFrame->sourceAddr;
    frameInfo->DA = addbaFrame->destAddr;
    frameInfo->frameType = (DOT11_MacFrameType)addbaFrame->frameType;
    frameInfo->insertTime = node->getNodeTime();
    MacDot11nSetMoreFramesFieldForAnAC(dot11, (UInt8)dot11->currentACIndex);
    MacDot11nMgmtQueue_EnqueuePacket(node, dot11, frameInfo);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nHandleBapTimer.
//  PURPOSE:     Handle block ack timer
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               timerMsg *timer2
//                  Pointer to timer structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void
MacDot11nHandleBapTimer(Node* node,
                        MacDataDot11* dot11,
                        timerMsg *timer2)
{
    TosType priority =  timer2->priority;
    UInt8 acIndex = (UInt8)MacDot11ReturnAccessCategory(priority);
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first =   timer2->addr;
    tmpKey.second =  timer2->priority;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    if (timer2->initiator)
    {
        OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
        keyItr = outputBuffer->OutputBufferMap.find(tmpKey);
    }
    else
    {
        InputBuffer *inputBuffer = dot11->inputBuffer;
        keyItr = inputBuffer->InputBufferMap.find(tmpKey);
    }
    if (timer2->seqNo
        != keyItr->second.blockAckVrbls->baTimerSequenceNumber)
    {
        //timer has been cancelled
        return;
    }
    switch(keyItr->second.blockAckVrbls->state)
    {
        case BAA_STATE_WF_ADDBA_RESPONSE:
        {
            keyItr->second.blockAckVrbls->state
                = BAA_STATE_WF_RETRY_TIMER_EXPIRE;
            keyItr->second.blockAckVrbls->startingSeqNo
                = MACDOT11N_INVALID_SEQ_NUM;
            MacDot11nSetBapTimer(node,
                                 dot11,
                                 &keyItr->second.blockAckVrbls->
                                 baTimerSequenceNumber,
                                 tmpKey.first,
                                 tmpKey.second,
                                 MACDOT11N_BAP_REINITIATE_TIMEOUT,
                                 TRUE);
            MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                               acIndex);
            break;
        }
        case BAA_STATE_WF_RETRY_TIMER_EXPIRE:
        {
            keyItr->second.blockAckVrbls->state = BAA_STATE_DISABLED;
            MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                               acIndex);
            break;
        }
        case BAA_STATE_IDLE:
        case BAA_STATE_WF_BLOCK_ACK:
        {
            // send delba and drop all pkts from queue
            MacDot11nSendDelba(node,
                               dot11,
                               keyItr,
                               TRUE,
                               acIndex);
            MacDot11nSetMoreFramesFieldForAnAC(dot11,
                                       acIndex);
            break;
        }
        case BAA_STATE_RECEIVER_WF_BA_SETUP:
        case BAA_STATE_RECEIVING:
        {
            MacDot11nSendDelba(node,
                               dot11,
                               keyItr,
                               FALSE,
                               DOT11e_INVALID_AC);
            break;
        }
        case BAA_STATE_TRANSMITTING:
        case BAA_STATE_BLOCKACK_REQUEST_QUEUED:
        {
            MacDot11nSetBapTimer(node,
                                 dot11,
                                 &keyItr->second.blockAckVrbls->
                                 baTimerSequenceNumber,
                                 tmpKey.first,
                                 tmpKey.second,
                                 dot11->blockAckPolicyTimeout,
                                 TRUE);
            break;
        }
        default:
        {
            break;
        }
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nSendPacketsUnderBaa.
//  PURPOSE:     Dequeues and sends packets as burst of mpdus
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator keyItr
//                  Pointer to the key
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nSendPacketsUnderBaa(Node* node,
                                  MacDataDot11* dot11,
                                  int acIndex,
                                  std::map<std::pair<Mac802Address,
                                  TosType>,
                                  MapValue>::iterator &keyItr)
{
    std::list<struct DOT11_FrameInfo*>::iterator listItr
        = keyItr->second.frameQueue.begin();
    int i = 0;
    for (i =0;
        i < keyItr->second.blockAckVrbls->numPktsSent;
        i++)
    {
        listItr++;
    }
    DOT11n_FrameHdr* hdr
        = (DOT11n_FrameHdr *)MESSAGE_ReturnPacket((*listItr)->msg);
    hdr->qoSControl.AckPolicy = MACDOT11N_ACK_POLICY_BA;
    (*listItr)->state = STATE_AWAITING_ACK;
    (*listItr)->baActive = TRUE;
    dot11->ACs[acIndex].frameInfo = (*listItr);
    dot11->ACs[acIndex].frameToSend = (*listItr)->msg;
    dot11->ACs[acIndex].priority = keyItr->first.second;
    dot11->ACs[acIndex].currentNextHopAddress = keyItr->first.first;
    dot11->ACs[acIndex].totalNoOfthisTypeFrameQueued++;
    dot11->ACs[acIndex].totalNoOfthisTypeFrameQueuedUnderBAA++;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nDestRifsEnable.
//  PURPOSE:     Checks if nexthop supports rifs mode
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddress
//                  Destinaion address
//  RETURN:      BOOL
//                  TRUE if destination supports rifs mode, FALSE otherwise
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11nDestRifsEnable(Node* node,
                        MacDataDot11* dot11,
                        Mac802Address destAddress)
{
    if (MacDot11IsAp(dot11))
    {
        DOT11_ApStationListItem* stationItem = NULL;
        stationItem
            = MacDot11ApStationListGetItemWithGivenAddress(node,
                                                           dot11,
                                                           destAddress);
       if (!stationItem || !stationItem->data->isHTEnabledSta)
       {
           return FALSE;
       }
       return stationItem->data->rifsMode;
    }
    else
    {
        if (dot11->stationType == DOT11_STA_IBSS)
        {
            DOT11n_IBSS_Station_Info *stationInfo
                = MacDot11nGetIbssStationInfo(dot11, destAddress);
            return stationInfo->rifsMode;
        }
        else
        {
            if (!dot11->associatedAP || !dot11->associatedAP->htEnableAP)
            {
               return FALSE;
            }
            return dot11->associatedAP->rifsMode;
        }
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nDequeuePacketsUnderBaa.
//  PURPOSE:     Checks if nexthop supports rifs mode
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator keyItr
//                  Pointer to the key
//               int acIndex
//                  Index of the access category
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nDequeuePacketsUnderBaa(Node* node,
                                     MacDataDot11* dot11,
                                     std::map<std::pair<Mac802Address,
                                     TosType>,
                                     MapValue>::iterator &keyItr,
                                     int acIndex)
{
    std::list<struct DOT11_FrameInfo*>::iterator listItr
        = keyItr->second.frameQueue.begin();
    int i = 0;
    for (i =0;
        i < keyItr->second.blockAckVrbls->numPktsSent;
        i++)
    {
        listItr++;
    }
    DOT11n_FrameHdr* hdr
        = (DOT11n_FrameHdr *)MESSAGE_ReturnPacket((*listItr)->msg);
    hdr->qoSControl.AckPolicy = MACDOT11N_ACK_POLICY_BA;
    (*listItr)->state = STATE_AWAITING_ACK;
    (*listItr)->baActive = TRUE;
    dot11->ACs[acIndex].frameInfo = (*listItr);
    dot11->ACs[acIndex].frameToSend = (*listItr)->msg;
    dot11->ACs[acIndex].priority = keyItr->first.second;
    dot11->ACs[acIndex].currentNextHopAddress = keyItr->first.first;
    dot11->ACs[acIndex].totalNoOfthisTypeFrameQueued++;
    dot11->ACs[acIndex].totalNoOfthisTypeFrameQueuedUnderBAA++;
    if (dot11->ACs[dot11->currentACIndex].frameInfo->msg == NULL)
    {
        ERROR_ReportError("There is no frame in AC\n");
    }
    dot11->dot11TxFrameInfo = dot11->ACs[dot11->currentACIndex].frameInfo;
    dot11->currentMessage = dot11->ACs[dot11->currentACIndex].frameInfo->msg;
    dot11->ipNextHopAddr = dot11->ACs[dot11->currentACIndex].frameInfo->RA;
    if (MacDot11IsBssStation(dot11)&&
        (dot11->bssAddr!= dot11->selfAddr))
    {
        dot11->currentNextHopAddress = dot11->bssAddr;
    }
    else
    {
        dot11->currentNextHopAddress =
              dot11->ACs[dot11->currentACIndex].frameInfo->RA;
    }
    dot11->BO = dot11->ACs[dot11->currentACIndex].BO;
    dot11->SSRC = dot11->ACs[dot11->currentACIndex].QSRC;
    dot11->SLRC = dot11->ACs[dot11->currentACIndex].QLRC;
    MacDot11nSendMpdu(node,
                      dot11,
                      MacDot11nDestRifsEnable(node,
                                              dot11,
                                              dot11->dot11TxFrameInfo->RA));
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nUnicastTransmitted.
//  PURPOSE:     Handles the control for dot11n when a unicast packet
//               is transmitted
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nUnicastTransmitted(Node* node,
                                 MacDataDot11* dot11)
{
    BOOL startAckTimer = TRUE;
    if (dot11->dot11TxFrameInfo->frameType == DOT11_QOS_DATA
        && !dot11->dot11TxFrameInfo->isAMPDU)
    {
        DOT11n_FrameHdr* hdr
            = (DOT11n_FrameHdr*)
                MESSAGE_ReturnPacket(dot11->dot11TxFrameInfo->msg);
        OutputBuffer *outputBuffer
            = dot11->ACs[dot11->currentACIndex].outputBuffer;
        std::pair<Mac802Address,TosType> tmpKey;
        tmpKey.first = dot11->dot11TxFrameInfo->RA;
        tmpKey.second = hdr->qoSControl.TID;
        std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
        keyItr = outputBuffer->OutputBufferMap.find(tmpKey);
        if (keyItr->second.blockAckVrbls)
        {
            if (keyItr->second.blockAckVrbls->state == BAA_STATE_TRANSMITTING)
            {
                keyItr->second.blockAckVrbls->numPktsSent++;
                dot11->totalNoPacketsSentAckPolicyBA++;
                dot11->unicastPacketsSentDcf++;
                dot11->ACs[dot11->currentACIndex].
                    totalNoOfthisTypeFrameDeQueued++;
                keyItr->second.blockAckVrbls->
                    numPktsLeftToBeSentInCurrentTxop--;
                if (keyItr->second.blockAckVrbls->
                    numPktsLeftToBeSentInCurrentTxop
                        > 0)
                {
                    //Update BANAV
                    MAC_PHY_TxRxVector txVector;
                    PHY_GetTxVector(node,
                                    dot11->myMacData->phyNumber,
                                    txVector);
                    clocktype pktduration
                        = PHY_GetTransmissionDuration(
                                                node,
                                                dot11->myMacData->phyNumber,
                                                txVector);
                    if (MacDot11nDestRifsEnable(node,
                                               dot11,
                                               keyItr->first.first))
                    {
                        keyItr->second.blockAckVrbls->navDuration
                            -= dot11->rifs;
                    }
                    else
                    {
                       keyItr->second.blockAckVrbls->navDuration
                           -= dot11->sifs;
                    }
                    keyItr->second.blockAckVrbls->navDuration
                        -= pktduration;
                    startAckTimer = FALSE;
                    MacDot11nResetACVariables(dot11,
                                              dot11->currentACIndex,
                                              FALSE);
                    MacDot11StationResetAckVariables(dot11);
                    MacDot11nResetCurrentMessageVariables(dot11);
                    MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
                    MacDot11StationResetCW(node, dot11);
                    MacDot11nDequeuePacketsUnderBaa(node,
                                                   dot11,
                                                   keyItr,
                                                   dot11->currentACIndex);
                }
                else
                {
                    // all packets sent
                    // Send blockack in next txop
                    startAckTimer = FALSE;
                    MacDot11nResetACVariables(dot11,
                                              dot11->currentACIndex,
                                              FALSE);
                    MacDot11StationResetAckVariables(dot11);
                    MacDot11nResetCurrentMessageVariables(dot11);
                    MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
                    MacDot11StationResetCW(node, dot11);
                    if (keyItr->second.blockAckVrbls->numPktsSent
                         == keyItr->second.blockAckVrbls->
                            numPktsToBeSentInCurrentSession)
                    {
                        keyItr->second.blockAckVrbls->navDuration = 0;
                        MacDot11nSendBlockAckRequest(node,
                                                     dot11,
                                                     keyItr);
                    }
                    else
                    {
                        ERROR_Assert(keyItr->second.blockAckVrbls->
                            numPktsSent < keyItr->second.blockAckVrbls->
                                numPktsToBeSentInCurrentSession,
                                "Packets cant be sent in current session");
                    }
                    MacDot11StationCheckForOutgoingPacket(node,
                                                          dot11,
                                                          TRUE);
                }
            }
            else
            {
                // state debug
            }
         }
        else
        {
            // no block ack policy
        }
    }
    else if (dot11->dot11TxFrameInfo->frameType == DOT11N_BLOCK_ACK_REQUEST)
    {
    }
    if (startAckTimer)
    {
        clocktype holdForAck = 0;
        MacDot11StationSetState(node, dot11, DOT11_S_WFACK);
        if (dot11->isHTEnable)
        {
            MAC_PHY_TxRxVector txVector;
            PHY_GetTxVector(node,
                            dot11->myMacData->phyNumber,
                            txVector);
            if (dot11->dot11TxFrameInfo->isAMPDU
                || dot11->dot11TxFrameInfo->frameType
                    == DOT11N_BLOCK_ACK_REQUEST)
            {
                txVector.length = sizeof(DOT11n_BlockAckControlFrame);
            }
            else
            {
                txVector.length = DOT11_SHORT_CTRL_FRAME_SIZE;
            }
            holdForAck = dot11->extraPropDelay
                + dot11->sifs
                + PHY_GetTransmissionDuration(node,
                                              dot11->myMacData->phyNumber,
                                              txVector)
                + dot11->extraPropDelay
                + dot11->slotTime;
        }
        else
        {
            ERROR_Assert(FALSE,"invalid state");
        }
        MacDot11StationStartTimer(node, dot11, holdForAck);
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nCalculateNumPacketsToBeSentInTxOp.
//  PURPOSE:     Calculates number of packets that can sent in a current
//               TxOp
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator keyItr
//                  Pointer to the key
//               BOOL reCalculating
//                  Specifies whether calculations is done after reception
//                  of a cts
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void
MacDot11nCalculateNumPacketsToBeSentInTxOp(Node* node,
                                           MacDataDot11* dot11,
                                           int acIndex,
                                           std::map<std::pair<Mac802Address,
                                           TosType>,
                                           MapValue>::iterator keyItr,
                                           BOOL reCalculating)
{
    if (keyItr->second.blockAckVrbls->numPktsSent
        < keyItr->second.blockAckVrbls->numPktsToBeSentInCurrentSession)
    {
        if (!reCalculating)
        {
            ERROR_Assert(keyItr->second.blockAckVrbls->
                numPktsLeftToBeSentInCurrentTxop == 0,
                "More packets can be sent in TXOP");
        }
        keyItr->second.blockAckVrbls->numPktsLeftToBeSentInCurrentTxop = 0;
        std::list<struct DOT11_FrameInfo*>::iterator listItr
            = keyItr->second.frameQueue.begin();
        UInt8 pktsLefttoBeSent
            = keyItr->second.blockAckVrbls->numPktsToBeSentInCurrentSession
                - keyItr->second.blockAckVrbls->numPktsSent;
        int i = 0;
        for (i = 0;
                i < keyItr->second.blockAckVrbls->numPktsSent;
                i++)
        {
            listItr++;
        }
        if (!reCalculating)
        {
           ERROR_Assert((*listItr)->state == STATE_FRESH,
               "State Not Fresh");
        }
        UInt8 numpktsInCurrentTxop = 0;
        clocktype duration = 0;
        clocktype rtsDuration = 0;
        clocktype lastPktDuration = 0;
        MAC_PHY_TxRxVector tempTxVector;
        tempTxVector = MacDot11nSetTxVector(node,
                                            dot11,
                                            DOT11_LONG_CTRL_FRAME_SIZE,
                                            DOT11_RTS,
                                            keyItr->first.first,
                                            NULL);
        duration += PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    tempTxVector);
        rtsDuration = duration;
        duration += dot11->sifs;
        MAC_PHY_TxRxVector tempTxVector2;
        tempTxVector2 = MacDot11nSetTxVector(node,
                                             dot11,
                                             DOT11_SHORT_CTRL_FRAME_SIZE,
                                             DOT11_CTS,
                                             keyItr->first.first,
                                             &tempTxVector);
        duration += PHY_GetTransmissionDuration(node,
                                                dot11->myMacData->phyNumber,
                                                tempTxVector2);
        duration += dot11->sifs;
        int pktsize = DOT11nMessage_ReturnPacketSize((*listItr)->msg);
        tempTxVector = MacDot11nSetTxVector(node,
                                            dot11,
                                            pktsize,
                                            DOT11_QOS_DATA,
                                            keyItr->first.first,
                                            NULL);
        duration += PHY_GetTransmissionDuration(node,
                                                dot11->myMacData->phyNumber,
                                                tempTxVector);
        while (duration < dot11->ACs[acIndex].TXOPLimit
            && numpktsInCurrentTxop < pktsLefttoBeSent)
        {
            numpktsInCurrentTxop++;
            if (numpktsInCurrentTxop == pktsLefttoBeSent)
            {
                lastPktDuration = 0;
                break;
            }
            if (MacDot11nDestRifsEnable(node, dot11,keyItr->first.first))
            {
                lastPktDuration = dot11->rifs;
                duration += dot11->rifs;
            }
            else
            {
               lastPktDuration = dot11->sifs;
               duration += dot11->sifs;
            }
            listItr++;
            pktsize = DOT11nMessage_ReturnPacketSize((*listItr)->msg);
            tempTxVector = MacDot11nSetTxVector(node,
                                                dot11,
                                                pktsize,
                                                DOT11_QOS_DATA,
                                                keyItr->first.first,
                                                NULL);
            lastPktDuration
                += PHY_GetTransmissionDuration(node,
                                               dot11->myMacData->phyNumber,
                                               tempTxVector);
            duration += lastPktDuration;
            if (duration < dot11->ACs[acIndex].TXOPLimit)
            {
            }
            else
            {
                break;
            }
         }//while
        if (numpktsInCurrentTxop > 0)
        {
            keyItr->second.blockAckVrbls->numPktsLeftToBeSentInCurrentTxop
                = numpktsInCurrentTxop;
            keyItr->second.blockAckVrbls->navDuration
                = duration - rtsDuration - lastPktDuration;
            if (DEBUG_DATA_BURST)
            {

                char dstAddress[MAX_STRING_LENGTH];
                MacDot11MacAddressToStr(dstAddress, &keyItr->first.first);
                if (reCalculating)
                {
                    printf("%" TYPES_64BITFMT "d Node %d Num packets to be"
                        " sent in current TxOp as burst of data %d"
                        " (recalculating)"
                        " towards destination/priority %s/%d\n",
                            node->getNodeTime(),
                            node->nodeId,
                            numpktsInCurrentTxop,
                            dstAddress,
                            keyItr->first.second);
                }
                else
                {
                    printf("%" TYPES_64BITFMT "d Node %d Num packets to be"
                        " sent in current TxOp as burst of data %d"
                        " towards destination/priority %s/%d\n",
                            node->getNodeTime(),
                            node->nodeId,
                            numpktsInCurrentTxop,
                            dstAddress,
                            keyItr->first.second);
                }
            }
        }
        else
        {
            ERROR_Assert(FALSE,"Coudn't send the packet in the current Txopt"
                "fragmentation is not supported");
        }
    }
    else
    {
        ERROR_Assert(FALSE,"invalid state");
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nCalcNumPacketsSentUnderBap.
//  PURPOSE:     Calculates number of packets that can sent under block ack
//               policy session
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int acIndex
//                  Index of the access category
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator keyItr
//                  Pointer to the key
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11nCalcNumPacketsSentUnderBap(Node* node,
                                  MacDataDot11* dot11,
                                  int acIndex,
                                  std::map<std::pair<Mac802Address,
                                  TosType>,
                                  MapValue>::iterator keyItr)
{
    if (keyItr->second.blockAckVrbls->state == BAA_STATE_IDLE)
    {
        UInt8 minMcsIndex = 0;
        if (DEBUG_DATA_BURST)
        {
            char dstAddress[MAX_STRING_LENGTH];
            MacDot11MacAddressToStr(dstAddress, &keyItr->first.first);
            printf("%" TYPES_64BITFMT "d Node %d Checking if"
                " packets can be sent as a burst of data"
                " towards destination/priority %s/%d\n",
                    node->getNodeTime(),
                    node->nodeId,
                    dstAddress,
                    keyItr->first.second);
        }
        if (MacDot11IsAp(dot11))
        {
            DOT11_ApStationListItem* stationItem = NULL;
            stationItem
                = MacDot11ApStationListGetItemWithGivenAddress(
                                                   node,
                                                   dot11,
                                                   keyItr->first.first);
            UInt8 maxMcs
                = std::min(stationItem->data->staHtCapabilityElement.
                    supportedMCSSet.mcsSet.maxMcsIdx,
                    (UInt8)(Phy802_11nGetNumAtnaElems(
                        node->phyData[dot11->myMacData->phyNumber])*8 - 1));
            minMcsIndex = maxMcs - 7;
        }
        else
        {
            if (dot11->stationType == DOT11_STA_IBSS)
            {
                DOT11n_IBSS_Station_Info *stationInfo
                    = MacDot11nGetIbssStationInfo(dot11,
                                                  keyItr->first.first);
                if (stationInfo->probeStatus == IBSS_PROBE_COMPLETED)
                {
                    UInt8 maxMcs
                        = std::min(stationInfo->staHtCapabilityElement.
                           supportedMCSSet.mcsSet.maxMcsIdx,
                           (UInt8)(Phy802_11nGetNumAtnaElems(
                         node->phyData[dot11->myMacData->phyNumber])*8 - 1));
                    minMcsIndex = maxMcs - 7;
                }
                else
                {
                    ERROR_Assert(FALSE,"invalid");
                }
            }
            else
            {
                UInt8 maxMcs
                    = std::min(dot11->associatedAP->staHtCapabilityElement.
                           supportedMCSSet.mcsSet.maxMcsIdx,
                           (UInt8)(Phy802_11nGetNumAtnaElems(
                         node->phyData[dot11->myMacData->phyNumber])*8 - 1));
                    minMcsIndex = maxMcs - 7;
             }
        }
        ERROR_Assert(minMcsIndex == 0
            || minMcsIndex == 8
            || minMcsIndex == 16
            || minMcsIndex == 24,
            "Not Minimum MCS");
        if (keyItr->second.numPackets >= BLOCK_ACK_POLICY_THRESHOLD)
        {
            UInt8 numPktsInCurrentSession = 0;
            UInt32 maxNumPktsInCurrentSession
                = MIN(keyItr->second.numPackets,
                      keyItr->second.blockAckVrbls->numPktsNegotiated);
            std::list<struct DOT11_FrameInfo*>::iterator listItr
                 = keyItr->second.frameQueue.begin();
            while (numPktsInCurrentSession < maxNumPktsInCurrentSession)
            {
                if ((*listItr)->state == STATE_FRESH)
                {
                    int pktsize
                        = DOT11nMessage_ReturnPacketSize((*listItr)->msg);
                    MAC_PHY_TxRxVector tempTxVector;
                    tempTxVector = MacDot11nSetTxVector(node,
                                                        dot11,
                                                        pktsize,
                                                        DOT11_QOS_DATA,
                                                        keyItr->first.first,
                                                        NULL);
                    tempTxVector.mcs = minMcsIndex;
                    clocktype duration
                        = PHY_GetTransmissionDuration(
                                                node,
                                                dot11->myMacData->phyNumber,
                                                tempTxVector);
                    if (duration < dot11->ACs[acIndex].TXOPLimit)
                    {
                        numPktsInCurrentSession++;
                        listItr++;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            if (numPktsInCurrentSession >= BLOCK_ACK_POLICY_THRESHOLD)
            {
                keyItr->second.blockAckVrbls->
                numPktsToBeSentInCurrentSession = numPktsInCurrentSession;
                DOT11_FrameInfo *tempFrameInfo
                    = keyItr->second.frameQueue.front();
                DOT11n_FrameHdr* hdr
                    = (DOT11n_FrameHdr *)MESSAGE_ReturnPacket(
                                                        tempFrameInfo->msg);
                keyItr->second.winStarts = hdr->seqNo;
                keyItr->second.winSizes
                    = keyItr->second.blockAckVrbls->
                        numPktsToBeSentInCurrentSession;
                keyItr->second.blockAckVrbls->startingSeqNo = hdr->seqNo;
                keyItr->second.winEnds = keyItr->second.winStarts;
                int i = 0;
                for (i =0;
                    i < keyItr->second.blockAckVrbls->
                        numPktsToBeSentInCurrentSession;
                    i++)
                {
                    MacDot11nIncrementSeqNumber(&keyItr->second.winEnds);
                }
                keyItr->second.blockAckVrbls->state
                    = BAA_STATE_TRANSMITTING;
                if (DEBUG_DATA_BURST)
                {
                    char dstAddress[MAX_STRING_LENGTH];
                    MacDot11MacAddressToStr(dstAddress, &keyItr->first.first);
                    printf("%" TYPES_64BITFMT "d Node %d Data bursting"
                        " can be used. Packets to be sent in current"
                        " session %d"
                        " towards destination/priority %s/%d\n",
                            node->getNodeTime(),
                            node->nodeId,
                            keyItr->second.blockAckVrbls->
                            numPktsToBeSentInCurrentSession,
                            dstAddress,
                            keyItr->first.second);
                }
            }
            else
            {
                if (DEBUG_DATA_BURST)
                {
                    char dstAddress[MAX_STRING_LENGTH];
                    MacDot11MacAddressToStr(dstAddress, &keyItr->first.first);
                    printf("%" TYPES_64BITFMT "d Node %d Data bursting"
                        " cannot be used. Num packets calculated are"
                        " less than %d"
                        " towards destination/priority %s/%d\n",
                            node->getNodeTime(),
                            node->nodeId,
                            BLOCK_ACK_POLICY_THRESHOLD,
                            dstAddress,
                            keyItr->first.second);
                }
                return FALSE;
            }
        }
        else
        {
            if (DEBUG_DATA_BURST)
            {
                char dstAddress[MAX_STRING_LENGTH];
                MacDot11MacAddressToStr(dstAddress, &keyItr->first.first);
                printf("%" TYPES_64BITFMT "d Node %d Data bursting"
                    " cannot be used. Num packets in output buffer"
                    " are less than %d"
                    " towards destination/priority %s/%d\n",
                        node->getNodeTime(),
                        node->nodeId,
                        BLOCK_ACK_POLICY_THRESHOLD,
                        dstAddress,
                        keyItr->first.second);
            }
            return FALSE;
        }
    }
    if (keyItr->second.blockAckVrbls->state == BAA_STATE_TRANSMITTING)
    {
         // calculate no. packets to be send in current txopt
         MacDot11nCalculateNumPacketsToBeSentInTxOp(node,
                                                    dot11,
                                                    acIndex,
                                                    keyItr,
                                                    FALSE);
    }
    return TRUE;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nGetBapState.
//  PURPOSE:     Returns the current block ack policy state
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator keyItr
//                  Pointer to the key
//  RETURN:      BlockAckAggStates
//                  Current state of block ack policy
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BlockAckAggStates MacDot11nGetBapState(Node* node,
                                       MacDataDot11* dot11,
                                       std::map<std::pair<Mac802Address,
                                       TosType>,
                                       MapValue>::iterator keyItr)
{
    return keyItr->second.blockAckVrbls->state;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nSendBlockAckRequest.
//  PURPOSE:     Sends a block ack request
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator keyItr
//                  Pointer to the key
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nSendBlockAckRequest(Node* node,
                                  MacDataDot11* dot11,
                                  std::map<std::pair<Mac802Address,
                                  TosType>,
                                  MapValue>::iterator keyItr)
{
    std::list<struct DOT11_FrameInfo*>::iterator listItr
        = keyItr->second.frameQueue.begin();
    UInt32 i = 0;
    for (i =0; i < keyItr->second.winSizes; i++)
    {
        listItr++;
    }
    Message *msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(DOT11n_BlockAckRequestFrame),
                        TRACE_DOT11);
    DOT11n_BlockAckRequestFrame *addbaFrame
        = (DOT11n_BlockAckRequestFrame*) MESSAGE_ReturnPacket(msg);
    memset(addbaFrame, 0, sizeof(DOT11n_BlockAckRequestFrame));
    addbaFrame->frameType = DOT11N_BLOCK_ACK_REQUEST;
    addbaFrame->frameFlags |= DOT11_TO_DS;
    addbaFrame->destAddr = keyItr->first.first;
    addbaFrame->sourceAddr = dot11->selfAddr;
    addbaFrame->barControl.BARAckPolicy
        = keyItr->second.blockAckVrbls->type;
    addbaFrame->barControl.MultiTID = FALSE;
    addbaFrame->barControl.CompressedBitmap = TRUE;
    addbaFrame->barControl.TID_INFO = keyItr->first.second;
    addbaFrame->startingSeqControl.FragId = 0;
    addbaFrame->startingSeqControl.SeqNo = keyItr->second.winStarts;
    
    // duration will be sent while transmitting this frame
    addbaFrame->duration = 0;
    DOT11_FrameInfo *frameInfo
        = (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
    memset(frameInfo, 0, sizeof(DOT11_FrameInfo));
    frameInfo->msg = msg;
    frameInfo->RA = addbaFrame->destAddr;
    frameInfo->TA = addbaFrame->sourceAddr;
    frameInfo->SA = addbaFrame->sourceAddr;
    frameInfo->DA = addbaFrame->destAddr;
    frameInfo->frameType = (DOT11_MacFrameType)addbaFrame->frameType;
    frameInfo->insertTime = node->getNodeTime();
    keyItr->second.blockAckVrbls->state = BAA_STATE_WF_BLOCK_ACK;
    MacDot11nSetMoreFramesFieldForAnAC(dot11, (UInt8)dot11->currentACIndex);
    MacDot11nMgmtQueue_EnqueuePacket(node, dot11, frameInfo, FALSE);
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nGetNavDuration.
//  PURPOSE:     Returns the NAV duration
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_FrameInfo *frameInfo
//                  Pointer to the frameInfo
//  RETURN:      clocktype
//                  NAV duration
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
clocktype
MacDot11nGetNavDuration(Node* node,
                        MacDataDot11* dot11,
                        DOT11_FrameInfo *frameInfo)
{
    DOT11n_FrameHdr* hdr
         = (DOT11n_FrameHdr*)MESSAGE_ReturnPacket(frameInfo->msg);
    UInt8 acIndex = (UInt8)MacDot11ReturnAccessCategory(hdr->qoSControl.TID);
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;
    std::pair<Mac802Address,TosType> tmpKey;
    tmpKey.first = hdr->destAddr;
    tmpKey.second = hdr->qoSControl.TID;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    keyItr = outputBuffer->OutputBufferMap.find(tmpKey);
    return keyItr->second.blockAckVrbls->navDuration;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nGetKey.
//  PURPOSE:     Iterates and returns the next key to be used to send data
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int acIndex
//                  Index of the access category
//               std::map<std::pair<Mac802Address,TosType>,MapValue>
//               ::iterator *keyItrPtr
//                  Pointer to the key
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11nGetKey(Node* node,
                     MacDataDot11* dot11,
                     int acIndex,
                     std::map<std::pair<Mac802Address,
                     TosType>,
                     MapValue>::iterator *keyItrPtr)
{
    OutputBuffer *outputBuffer = dot11->ACs[acIndex].outputBuffer;

    // roundRobinKeyItr is an iterator which iterates through the keys in
    // round robin fashion to dequeue packets.
    outputBuffer->roundRobinKeyItr++;
    if (outputBuffer->roundRobinKeyItr == outputBuffer->OutputBufferMap.end())
    {
        outputBuffer->roundRobinKeyItr
            = outputBuffer->OutputBufferMap.begin();
    }
    int count = outputBuffer->OutputBufferMap.size();
    while (count > 0)
    {
        count --;
        if (!(outputBuffer->roundRobinKeyItr->second.frameQueue.empty()))
        {
             if (outputBuffer->roundRobinKeyItr->second.blockAckVrbls)
             {
                 if (outputBuffer->roundRobinKeyItr->
                        second.blockAckVrbls->state
                        < BAA_STATE_ADDBA_REQUEST_QUEUED
                        || outputBuffer->roundRobinKeyItr
                        -> second.blockAckVrbls->state
                        > BAA_STATE_DELBA_QUEUED)
                {
                    break;
                }
            }
            else
            {
                 break;
            }
        }
        outputBuffer->roundRobinKeyItr++;
        if (outputBuffer->roundRobinKeyItr
                == outputBuffer->OutputBufferMap.end())
        {
           outputBuffer->roundRobinKeyItr
               = outputBuffer->OutputBufferMap.begin();
        }
        ERROR_Assert(count >= 0,"invalid count");
    }
    *keyItrPtr = outputBuffer->roundRobinKeyItr;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nSetMessageInAC.
//  PURPOSE:     Dequeues the message from output buffer and sets it in the
//               access category
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int acIndex
//                  Index of the access category
//  RETURN:      BOOL
//                  TRUE if message was set in the AC,FALSE othereise
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL
MacDot11nSetMessageInAC(Node* node,
                        MacDataDot11* dot11,
                        int acIndex)
{
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator keyItr;
    MacDot11nGetKey(node, dot11,acIndex, &keyItr);
    BOOL setCurrentMsg = TRUE;
    ERROR_Assert(dot11->ACs[acIndex].isACHasPacket,
        "AC doesnt has any packet");
    BOOL blockAckPolicyAvailable = FALSE;
    BOOL addbaRequestEnqueued = FALSE;

    if (keyItr->first.first != ANY_MAC802
        && dot11->stationType == DOT11_STA_IBSS)
    {
        DOT11n_IBSS_Station_Info *stationInfo
            = MacDot11nGetIbssStationInfo(dot11, keyItr->first.first);
        if (stationInfo->probeStatus == IBSS_PROBE_NONE)
        {
           stationInfo->probeStatus = IBSS_PROBE_REQUEST_QUEUED;
           MacDot11nIbssTransmitProbeRequest(node,
                                             dot11,
                                             keyItr->first.first);
        }
    }

    blockAckPolicyAvailable =
    MacDot11nBlockAckPolicyAvailable(acIndex,
                                     keyItr);
    BOOL processOtherOptions = TRUE;
    if (blockAckPolicyAvailable)
    {
        BlockAckAggStates state =
            MacDot11nGetBapState(node,
                                 dot11,
                                 keyItr);
        if (state == BAA_STATE_IDLE
            || state == BAA_STATE_TRANSMITTING)
        {
            BOOL blockAckPolicyExists = TRUE;
            if (blockAckPolicyExists)
            {
                // send packets via burst
                BOOL sendViaBAA = FALSE;
                sendViaBAA  = MacDot11nCalcNumPacketsSentUnderBap(
                                  node,
                                  dot11,
                                  acIndex,
                                  keyItr);
                if (sendViaBAA)
                {
                    MacDot11nSendPacketsUnderBaa(node,
                                                 dot11,
                                                 acIndex,
                                                 keyItr);
                    setCurrentMsg = TRUE;
                    processOtherOptions = FALSE;
                }
            }
        }
        else if (state == BAA_STATE_DISABLED)
        {
            BOOL canUseBlockAckPolicy =  FALSE;
            UInt8 numPkts;
            canUseBlockAckPolicy =
            MacDot11nBlockAckPolicyUsable(node,
                    dot11,
                    keyItr,
                    &numPkts);
            if (canUseBlockAckPolicy)
            {
                keyItr->second.blockAckVrbls->state
                    = BAA_STATE_ADDBA_REQUEST_PENDING;
                addbaRequestEnqueued = TRUE;
            }
            else
            {
                // no block ack policy. send normally
            }
        }
    }
    if (dot11->isAmpduEnable && processOtherOptions)
    {
        UInt8 numPkts;
        BOOL ampduPossible =
        MacDot11nCanAmpduBeCreated(node,
        dot11,
        acIndex,
        &numPkts);
        if (ampduPossible)
        {
            //create AMSDU
            setCurrentMsg = TRUE;
            MacDot11nCreateAmpdu(node,
                                 dot11,
                                 acIndex,
                                 numPkts);
            if (((!((MacDot11StationPhyStatus(node, dot11) == PHY_IDLE)
                    && (MacDot11IsPhyIdle(node, dot11)))))
                    ||MacDot11StationWaitForNAV(node, dot11)
                    || MacDot11nOtherAcsHaveData(dot11,acIndex))
            {
                MacDot11StationSetBackoffIfZero(node,dot11,acIndex);
            }
        }
        else
        {
            setCurrentMsg =
            MacDot11nDequeuePacketFromOutputBuffer(node,
                                                   dot11,
                                                   acIndex,
                                                   keyItr);
            if (setCurrentMsg)
            {
                if (((!((MacDot11StationPhyStatus(node, dot11) == PHY_IDLE)
                        && (MacDot11IsPhyIdle(node, dot11)))))
                        ||MacDot11StationWaitForNAV(node, dot11)
                        || MacDot11nOtherAcsHaveData(dot11,acIndex))
                {
                    MacDot11StationSetBackoffIfZero(node,dot11,acIndex);
                }
            }
        }
    }
    else if (processOtherOptions)
    {
        setCurrentMsg =
            MacDot11nDequeuePacketFromOutputBuffer(node,
                                                   dot11,
                                                   acIndex,
                                                   keyItr);
        if (((!((MacDot11StationPhyStatus(node, dot11) == PHY_IDLE)
                && (MacDot11IsPhyIdle(node, dot11)))))
                ||MacDot11StationWaitForNAV(node, dot11)
                || MacDot11nOtherAcsHaveData(dot11,acIndex))
        {
            MacDot11StationSetBackoffIfZero(node,dot11,acIndex);
        }
    }
    return setCurrentMsg;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nIbssHandleProbeRequestTimer
//  PURPOSE:     Handles probe request for a failed probing attempt
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Timer message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nIbssHandleProbeRequestTimer(Node* node,
                                          MacDataDot11* dot11,
                                          Message *msg)
{
    timerMsg *timer2 = (timerMsg*)MESSAGE_ReturnInfo(msg, INFO_TYPE_Dot11nTimerInfo);
    DOT11n_IBSS_Station_Info *stationInfo =
        MacDot11nGetIbssStationInfo(dot11, timer2->addr);
    ERROR_Assert(stationInfo->probeStatus == IBSS_PROBE_IN_PROGRESS,
                 "Probing not in progress : Invalid Case");
    stationInfo->probeStatus = IBSS_PROBE_NONE;
    stationInfo->timerMessage = NULL;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11nResetIbssStationInfo
//  PURPOSE:     Resets the ibss station information at an STA
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *msg
//                  Timer message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11nResetIbssStationInfo(Node* node,
                                   MacDataDot11* dot11)
{
    DOT11n_IBSS_Station_Info *stationInfoItr
        = dot11->ibssConnectedStationsInfo;
    while (stationInfoItr)
    {
        DOT11n_IBSS_Station_Info *tempStationInfo
            = stationInfoItr->next;
        MEM_free(stationInfoItr);
        stationInfoItr = tempStationInfo;
    }
    dot11->ibssConnectedStationsInfo = NULL;
}
