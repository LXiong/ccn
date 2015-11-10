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

#ifndef MAC_DOT16_SCH_H
#define MAC_DOT16_SCH_H

//
// This is the header file of the implementation of the scheduling functions
// of IEEE 802.16 MAC. In theory, there are 3 schedulers needed. One is for
// outbound transmission scheduling at the BS for downlink, and one is for
// uplink burst scheduling at the BS. And the last is the outbound
// transmission scheduling at the SS.
//

//--------------------------------------------------------------------------
// Default values of various parameters
//--------------------------------------------------------------------------

// /**
// CONSTANT    :: DOT16_BS_DEFAULT_OUTBOUND_SCHEDULER: WFQ
// DESCRIPTION :: Default outbound transmission scheduler at the BS node
// **/
#define DOT16_BS_DEFAULT_OUTBOUND_SCHEDULER    "WEIGHTED-FAIR"

// /**
// CONSTANT    :: DOT16_SS_DEFAULT_OUTBOUND_SCHEDULER: WFQ
// DESCRIPTION :: Default outbound transmission scheduler at the SS node
// **/
#define DOT16_SS_DEFAULT_OUTBOUND_SCHEDULER    "WEIGHTED-FAIR"

// /**
// CONSTANT    :: DOT16_DEFAULT_MGMT_SCHEDULER: Strict-Priority
// DESCRIPTION :: Default outbound transmission scheduler for mgmt msgs
// **/
#define DOT16_DEFAULT_MGMT_SCHEDULER    "STRICT-PRIORITY"

// /**
// CONSTANT    :: DOT16_DEFAULT_DATA_QUEUE_SIZE: 500K bytes
// DESCRIPTION :: Default size of the queue for holding data PDUs of a conn.
// **/
#define DOT16_DEFAULT_DATA_QUEUE_SIZE    500000

// /**
// CONSTANT    :: DOT16_DEFAULT_MGMT_QUEUE_SIZE: 30K bytes
// DESCRIPTION :: Default size of the queue for holding mgmt PDUs of a conn.
// **/
#define DOT16_DEFAULT_MGMT_QUEUE_SIZE    30000

// /**
// CONSTANT    :: DOT16_SCH_BCAST_MGMT_QUEUE_PRIORITY: 0x7FFFFFFF
// DESCRIPTION :: Priority of the queue for holding broadcast mgmt messages
//                We give the highest priority to broadcast mgmt messages
// **/
#define DOT16_SCH_BCAST_MGMT_QUEUE_PRIORITY    0x7FFFFFFF

// /**
// CONSTANT    :: DOT16_SCH_BASIC_MGMT_QUEUE_PRIORITY: 0x7FFFFFFE
// DESCRIPTION :: Priority of the queue for holding mgmt msgs on basic conn.
//                We give the second highest priority to such messages
// **/
#define DOT16_SCH_BASIC_MGMT_QUEUE_PRIORITY    0x7FFFFFFE

// /**
// CONSTANT    :: DOT16_SCH_PRIMARY_MGMT_QUEUE_PRIORITY: 0x7FFFFFFD
// DESCRIPTION :: Priority of the queue for mgmt msgs on primary conn.
//                We give the third highest priority to such messages
// **/
#define DOT16_SCH_PRIMARY_MGMT_QUEUE_PRIORITY    0x7FFFFFFD

// /**
// CONSTANT    :: DOT16_SCH_SECONDARY_MGMT_QUEUE_PRIORITY: 0x7FFFFFFC
// DESCRIPTION :: Priority of the queue for mgmt msgs on secondary conn.
//                We give the fourth highest priority to such messages
// **/
#define DOT16_SCH_SECONDARY_MGMT_QUEUE_PRIORITY    0x7FFFFFFC

// /**
// CONSTANT    :: DOT16_SCH_BCAST_MGMT_QUEUE_WEIGHT: 500000.0
// DESCRIPTION :: Weight to the queue for broadcast mgmt messages
// **/
#define DOT16_SCH_BCAST_MGMT_QUEUE_WEIGHT    500000.0

// /**
// CONSTANT    :: DOT16_SCH_BASIC_MGMT_QUEUE_WEIGHT: 400000.0
// DESCRIPTION :: Weight to the queue for mgmt messages on basic conn.
// **/
#define DOT16_SCH_BASIC_MGMT_QUEUE_WEIGHT    400000.0

// /**
// CONSTANT    :: DOT16_SCH_PRIMARY_MGMT_QUEUE_WEIGHT: 300000.0
// DESCRIPTION :: Weight to the queue for mgmt messages on primary conn.
// **/
#define DOT16_SCH_PRIMARY_MGMT_QUEUE_WEIGHT    300000.0

// /**
// CONSTANT    :: DOT16_SCH_SECONDARY_MGMT_QUEUE_WEIGHT: 200000.0
// DESCRIPTION :: Weight to the queue for mgmt messages on primary conn.
// **/
#define DOT16_SCH_SECONDARY_MGMT_QUEUE_WEIGHT    200000.0

#define DOT16_ARQ_TX_NEXT_BSN (sFlow->arqControlBlock->arqTxNextBsn)
#define DOT16_ARQ_TX_WINDOW_START (sFlow->arqControlBlock->arqTxWindowStart)

#define DOT16_ARQ_WINDOW_REAR (sFlow->arqControlBlock->rear)
#define DOT16_ARQ_WINDOW_FRONT (sFlow->arqControlBlock->front)
#define DOT16_ARQ_WINDOW_SIZE (sFlow->arqParam->arqWindowSize)
#define DOT16_ARQ_ARRAY_SIZE (sFlow->arqParam->arqWindowSize+ 1 )
#define DOT16_ARQ_BLOCK_SIZE (sFlow->arqParam->arqBlockSize)
#define MacDot16ARQSetARQBlockPointer(arrayIndex)\
arqBlockPtr =\
 &(sFlow->arqControlBlock->arqBlockArray[arrayIndex])

#define DOT16_ARQ_FRAG_SUBHEADER &(arqBlockPtr->arqFragSubHeader)

#define MacDot16ARQWindowFreeSpace(freeWindowSpace, front, rear, windowSize)\
                    do { \
                        if (rear >= front )\
                        {\
                          freeWindowSpace = (windowSize - 1) - (rear - front);\
                        } \
                        else { freeWindowSpace = front - rear - 1 ;} \
                      }\
                      while (0)


#define MacDot16ARQCalculateNumBlocks(numARQBlocks, pduSize) \
       if (pduSize <= DOT16_ARQ_BLOCK_SIZE )\
           {\
               numARQBlocks = 1 ;\
           }\
       else\
           {\
                numARQBlocks = (UInt16)(pduSize /DOT16_ARQ_BLOCK_SIZE);\
                if (pduSize % DOT16_ARQ_BLOCK_SIZE)\
                    {\
                        numARQBlocks++;\
                    }\
            }

#define MacDot16ARQIncIndex(index) \
    (index = (index + 1)% (Int16) DOT16_ARQ_WINDOW_SIZE)

#define MacDot16ARQIncBsnId(bsnId) \
    (bsnId = (bsnId + 1)% DOT16_ARQ_BSN_MODULUS)

#define MacDot16ARQDecIndex(index) \
    (index = (index - 1 + DOT16_ARQ_WINDOW_SIZE)% DOT16_ARQ_WINDOW_SIZE)

#define MacDot16ARQDecBsnId(bsnId) \
    (bsnId = (bsnId - 1 + DOT16_ARQ_BSN_MODULUS)% DOT16_ARQ_BSN_MODULUS)



//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: MacDot16SchInit
// LAYER      :: MAC
// PURPOSE    :: Initialize scheduler
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacDot16SchInit(Node* node,
                     int interfaceIndex,
                     const NodeInput* nodeInput);

// /**
// FUNCTION   :: MacDot16SchAddQueueForServiceFlow
// LAYER      :: MAC
// PURPOSE    :: Initialize scheduler
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + sFlow     : MacDot16ServiceFlow* : Pointer to the service flow
// RETURN     :: void : NULL
// **/
void MacDot16SchAddQueueForServiceFlow(
         Node* node,
         MacDataDot16* dot16,
         MacDot16ServiceFlow* sFlow);

// /**
// FUNCTION   :: MacDot16ScheduleDlSubframe
// LAYER      :: MAC
// PURPOSE    :: BS node will call this function to schedule PDUs to be
//               transmitted in the downlink subframe. The outbound
//               scheduler is triggered to do this.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + duration  : clocktype     : The duration of the DL subframe
// RETURN     :: clocktype : duration left unused. If no enough PDUs to
//                           fully utlize the dl duration, rest of time
//                           can be used for uplink transmissions.
// **/
clocktype MacDot16ScheduleDlSubframe(
              Node* node,
              MacDataDot16* dot16,
              clocktype duration);

// /**
// FUNCTION   :: MacDot16ScheduleUlBurst
// LAYER      :: MAC
// PURPOSE    :: SS node will call this function to schedule PDUs to be
//               transmitted in its uplink burst. The outbound
//               scheduler is triggered to do this.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + bwGranted : int           : UL BW granted in # of bytes
// RETURN     :: int : Number of bytes left unused. There is a possibility
//                     that the node has no enough PDUs to fully utilize
//                     the whole allocation.
// **/
int MacDot16ScheduleUlBurst(Node* node, MacDataDot16* dot16, int bwGranted);

// /**
// FUNCTION   :: MacDot16ScheduleUlSubframe
// LAYER      :: MAC
// PURPOSE    :: BS node will call this function to schedule uplink bursts
//               for uplink transmissions. This is mainly based on the BW
//               allocated to each SS.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + duration  : clocktype     : The duration of the UL subframe
// RETURN     :: void : NULL
// **/
void MacDot16ScheduleUlSubframe(
         Node* node,
         MacDataDot16* dot16,
         clocktype duration);

// /**
// FUNCTION   :: MacDot16SchResetDlAllocation
// LAYER      :: MAC
// PURPOSE    :: Reset the allocation map of the DL sub-frame in order
//               to schedule for a new frame.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + dlDuration: clocktype     : Duration of the DL-subframe
// RETURN     :: void : NULL
// **/
void MacDot16SchResetDlAllocation(Node* node,
                                  MacDataDot16* dot16,
                                  clocktype dlDuration);

// /**
// FUNCTION   :: MacDot16SchAllocDlBurst
// LAYER      :: MAC
// PURPOSE    :: Search a space in the DL allocation map for specified size
//               in bytes. Allocation is always Physical Slot (PS) based.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + sizeInBytes: int          : Size in bytes of the requested burst
// + diuc      : unsigned char : DIUC referring to the burst profile
// + msg       : PhyDot16BurstInfo : For returning info of alloced burst
// RETURN     :: BOOL : TRUE: successful, FALSE, failed
// **/
BOOL MacDot16SchAllocDlBurst(Node* node,
                             MacDataDot16* dot16,
                             int sizeInBytes,
                             unsigned char diuc,
                             Dot16BurstInfo* burstInfo);

// FUNCTION   :: MacDot16SchPriorityToCid
// LAYER      :: MAC
// PURPOSE    :: TO convert a priority Value to a CID value.
// PARAMETERS ::
// + int      :: priority:priority Value
// RETURN     :: Dot16CIDType:Dot16CIDType Value to be returned
// **/

Dot16CIDType MacDot16SchPriorityToCid(int priority);

//--------------------------------------------------------------------------
//  ARQ related functions
//--------------------------------------------------------------------------
/**
// FUNCTION   :: MacDot16PrintARQParameter
// LAYER      :: MAC
// PURPOSE    :: Print ARQ parameters
// PARAMETERS ::
   + arqParam : MacDot16ARQParam* : Pointer to an ARQ structure.
// RETURN     :: void : NULL
**/
void MacDot16PrintARQParameter(MacDot16ARQParam* arqParam);

// /**
// FUNCTION   :: MacDot16ARQCbInit
// LAYER      :: MAC
// PURPOSE    :: Initialize ARQ control block
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + sFlow     : MacDot16ServiceFlow*: Pointer to service flow
// RETURN     :: void : NULL
// **/
void MacDot16ARQCbInit(Node* node,
                       MacDataDot16*,
                       MacDot16ServiceFlow* sFlow);

Message* MacDot16ARQHandleDataPdu(Node* node,
                              MacDataDot16* dot16,
                              MacDot16ServiceFlow* sFlow,
                              MacDot16BsSsInfo* ssInfo,
                              Message* msg,
                              int pduLength,
                              BOOL isPackingHeaderPresent);

void MacDot16BuildAndSendARQFeedback(Node *node,
                                     MacDataDot16* dot16,
                                     MacDot16ServiceFlow* sFlow,
                                     MacDot16BsSsInfo* ssInfo);

void MacDot16ARQBuildAndSendDiscardMsg(Node *node,
                                     MacDataDot16* dot16,
                                     MacDot16ServiceFlow* sFlow,
                                     MacDot16BsSsInfo* ssInfo,
                                     Dot16CIDType cid,
                                     UInt16 bsnId);


int MacDot16ARQHandleFeedback (Node* node,
                               MacDataDot16* dot16,
                               unsigned char* macFrame,
                               int startIndex);
int MacDot16ARQHandleDiscardMsg(Node* node,
                                MacDataDot16* dot16,
                                unsigned char* macFrame,
                                int startIndex);

BOOL MacDot16ARQCheckIfBSNInWindow(MacDot16ServiceFlow* sFlow,
                                   Int16 incomingBsnId);

void MacDot16ARQPrintControlBlock(Node* node, MacDot16ServiceFlow* sFlow);

void MacDot16ARQBuildSDU(Node* node,
                    MacDataDot16* dot16,
                    MacDot16ServiceFlow* sFlow,
                    MacDot16BsSsInfo* ssInfo);
#endif // MAC_DOT16_SCH_H
