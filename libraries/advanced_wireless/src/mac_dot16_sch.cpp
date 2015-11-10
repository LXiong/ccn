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
// This is the implementation of schedulers used by 802.16 MAC.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "network_ip.h"
#include "transport_udp.h"

#include "mac_dot16.h"
#include "mac_dot16_bs.h"
#include "mac_dot16_ss.h"
#include "mac_dot16_cs.h"
#include "mac_dot16_phy.h"
#include "mac_dot16_sch.h"
#include "phy_dot16.h"


#define DEBUG             0
#define DEBUG_BWREQ        0
#define DEBUG_BURST        0
#define DEBUG_NBR_SCAN     0

#define DEBUG_CRC                      0
#define DEBUG_PACKING_FRAGMENTATION    0

#define DEBUG_ARQ                      0// && (node->nodeId == 0)
#define DEBUG_ARQ_INIT                 0
#define DEBUG_ARQ_WINDOW               0
#define DEBUG_ARQ_TIMER                0// && (node->nodeId == 0)

// /**
// FUNCTION   :: MacDot16SchCidToPriority
// LAYER      :: MAC
// PURPOSE    :: Convert a cid to queue priority. We try to generate
//               a priority which is unique as well as can reflect the
//               QoS categories. The uniqueness is guaranteed by the fact
//               that the CID is unqiue to a connection. Since priority is
//               4 bytes while the CID is 2 bytes, we then multiply the
//               CID with service type to differentiate QoS categories.
// PARAMETERS ::
// + cid       : Dot16CIDType    : CID of the connection
// + serviceType : MacDot16ServiceType : service type of the connection
// RETURN     :: int : priority for queue associated with the conn.
// **/
static inline
int MacDot16SchCidToPriority(Dot16CIDType cid,
                             MacDot16ServiceType serviceType)
{
    int priority;

    priority = DOT16_NUM_SERVICE_TYPES - serviceType - 1;
    priority *= 65536;
    priority += cid;

    return priority;
}
// /**
// FUNCTION   :: MacDot16SchPriorityToServiceType
// LAYER      :: MAC
// PURPOSE    :: Convert to queue priority to service type.
// PARAMETERS ::
// + priority : int : priority for queue associated with the conn.
// RETURN     :: MacDot16ServiceType : service type associated
//                                     with the conn.
// **/
static inline
MacDot16ServiceType MacDot16SchPriorityToServiceType(int priority)
{

    MacDot16ServiceType serviceType;

    serviceType = (MacDot16ServiceType)
                  (DOT16_NUM_SERVICE_TYPES - 1 - priority / 65536);

    return serviceType;
}

// /**
// FUNCTION   :: MacDot16SchPriorityToCid
// LAYER      :: MAC
// PURPOSE    :: Convert to queue priority to cid.
// PARAMETERS ::
// + priority : int : priority for queue associated with the conn.
// RETURN     :: Dot16CIDType : cid with the conn.
// **/
//static inline
Dot16CIDType MacDot16SchPriorityToCid(int priority)
{

    Dot16CIDType cid;

    cid = (Dot16CIDType) (priority % 65536);

    return cid;
}

// /**
// FUNCTION   :: MacDot16SchServiceFlowWeight
// LAYER      :: MAC
// PURPOSE    :: Decide the weight of a service flow. This weight is useful
//               when the outbound scheduler is weighted fair queuing. The
//               weight should be proportional to the bandwidth allocated
//               to each connection. In this implementation, we simply take
//               the minimal reserved rate of the flow.
// PARAMETERS ::
// + node      : Node*                : Pointer to node.
// + dot16     : MacDataDot16*        : Pointer to DOT16 structure
// + sFlow     : MacDot16ServiceFlow* : Pointer to the service flow
// RETURN     :: double : Weight of the service flow
// **/
static
double MacDot16SchServiceFlowWeight(Node* node,
                                    MacDataDot16* dot16,
                                    MacDot16ServiceFlow* sFlow)
{
    double weight;

    weight = sFlow->qosInfo.minReservedRate;

    return weight;
}


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
                     const NodeInput* nodeInput)
{
    MacDataDot16* dot16 = (MacDataDot16*)
                          node->macData[interfaceIndex]->macVar;
    Queue* queue = NULL;
    //Address interfaceAddress;
    BOOL wasFound = FALSE;
    char buff[MAX_STRING_LENGTH];
    int i;

    //NetworkGetInterfaceInfo(node, interfaceIndex, &interfaceAddress);

    if (dot16->stationType == DOT16_BS)
    {
        // this is a base station. It needs an outbound scheduler for
        // downlink data transmissions and a burst scheduler for uplink
        // burst scheduling

        if (DEBUG)
        {

            printf("Node%u(BS) is initializing scheduler\n", node->nodeId);
        }

        MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

/* Disabled as only WFQ is tested.
        IO_ReadString(node,
                      node->nodeId,
                      interfaceIndex,
                      nodeInput,
                      "MAC-802.16-BS-OUTBOUND-SCHEDULER",
                      &wasFound,
                      buff);
*/
        if (!wasFound)
        {
            // if not specified, set the default scheduler type
            strcpy(buff, DOT16_BS_DEFAULT_OUTBOUND_SCHEDULER);
        }

        // create one scheduler for each service type
        for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
        {
            SCHEDULER_Setup(&(dot16Bs->outScheduler[i]),
                            buff,
                            FALSE,
                            "NA");
        }

        // create a scheduler for mgmt messages
        SCHEDULER_Setup(&(dot16Bs->mgmtScheduler),
                        DOT16_DEFAULT_MGMT_SCHEDULER,
                        FALSE,
                        "NA");

        // now, add some permanent queues for managment purpose

        // 1. Add broadcast mgmt queue
        queue = new Queue;
        queue->SetupQueue(node,
                          "FIFO",
                          DOT16_DEFAULT_MGMT_QUEUE_SIZE);

        dot16Bs->mgmtScheduler->addQueue(
            queue,
            DOT16_SCH_BCAST_MGMT_QUEUE_PRIORITY);

        // 2. Add queue for mgmt messages on basic connection with any SS
        queue = new Queue;
        queue->SetupQueue(node,
                          "FIFO",
                          DOT16_DEFAULT_MGMT_QUEUE_SIZE);
        dot16Bs->mgmtScheduler->addQueue(
            queue,
            DOT16_SCH_BASIC_MGMT_QUEUE_PRIORITY);

        // 3. Add queue for mgmt messages on primary connection with any SS
        queue = new Queue;
        queue->SetupQueue(node,
                          "FIFO",
                          DOT16_DEFAULT_MGMT_QUEUE_SIZE);
        dot16Bs->mgmtScheduler->addQueue(
            queue,
            DOT16_SCH_PRIMARY_MGMT_QUEUE_PRIORITY);

        // 4. Add queue for mgmt message on secondary connection with any SS
        queue = new Queue;
        queue->SetupQueue(node,
                          "FIFO",
                          DOT16_DEFAULT_MGMT_QUEUE_SIZE);
        dot16Bs->mgmtScheduler->addQueue(
            queue,
            DOT16_SCH_SECONDARY_MGMT_QUEUE_PRIORITY);
    }
    else
    {
        // this is a subscriber station. Only needs an outbound scheduler
        // for uplink data transmissions.

        if (DEBUG)
        {

            printf("Node%u(BS) is initializing scheduler\n", node->nodeId);
        }

        MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

/* Disabled as only WFQ was tested.
        IO_ReadString(node,
                      node->nodeId,
                      interfaceIndex,
                      nodeInput,
                      "MAC-802.16-SS-OUTBOUND-SCHEDULER",
                      &wasFound,
                      buff);
*/

        if (!wasFound)
        {
            // if not specified, set the default scheduler type
            strcpy(buff, DOT16_SS_DEFAULT_OUTBOUND_SCHEDULER);
        }

        // create one scheduler for each service type
        for (i = 0; i <DOT16_NUM_SERVICE_TYPES; i ++)
        {
            SCHEDULER_Setup(&(dot16Ss->outScheduler[i]),
                            buff,
                            FALSE,
                            "NA");
        }

        // create a scheduler for mgmt messages
        SCHEDULER_Setup(&(dot16Ss->mgmtScheduler),
                        DOT16_DEFAULT_MGMT_SCHEDULER,
                        FALSE,
                        "NA");

         // now, add some permanent queues for managment purpose

        // 1. Add a queue for mgmt messages on the basic connection
        queue = new Queue;
        queue->SetupQueue(node,
                          "FIFO",
                          DOT16_DEFAULT_MGMT_QUEUE_SIZE);
        dot16Ss->mgmtScheduler->addQueue(
            queue,
            DOT16_SCH_BASIC_MGMT_QUEUE_PRIORITY);

        // 2. Add a queue for mgmt messages on the primary connection
        queue = new Queue;
        queue->SetupQueue(node,
                          "FIFO",
                          DOT16_DEFAULT_MGMT_QUEUE_SIZE);
        dot16Ss->mgmtScheduler->addQueue(
            queue,
            DOT16_SCH_PRIMARY_MGMT_QUEUE_PRIORITY);

        // 3. If managed, add a queue for mgmt msgs on the secondary conn.
        if (dot16Ss->para.managed)
        {
            queue = new Queue;
            queue->SetupQueue(node,
                              "FIFO",
                              DOT16_DEFAULT_MGMT_QUEUE_SIZE);
            dot16Ss->mgmtScheduler->addQueue(
                queue,
                DOT16_SCH_SECONDARY_MGMT_QUEUE_PRIORITY);
        }
    }
}

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
         MacDot16ServiceFlow* sFlow)
{
    Scheduler* scheduler;
    Queue* queue;
    Message* msg;
    Message* tmpMsg;
    BOOL queueIsFull = FALSE;
    clocktype currentTime;
    QueueBehavior qBehavior = RESUME;
    MacDot16ePSClasses* psClass = NULL;

    // Decide the priority for the queue. The priority needs to be unique.
    // In addition, if the scheduler type is strict priority etc, then this
    // priority will be used in scheduling too. Thus, it is important to
    // give higher priority to higher QoS flows. For example,
    // mgmt > UGS >ertPS> rtPS > nrtPS > BE.
    sFlow->queuePriority = MacDot16SchCidToPriority(
                               sFlow->cid,
                               sFlow->serviceType);

    // Decide the weight for the queue. This weight will be used by weighted
    // fair schedulers such as WFQ, WRR, etc. The weight of a queue should
    // be corresponding to its bandwidth requirements. In this
    // implementation, we use the min reserved rate of the flow.
    sFlow->queueWeight = MacDot16SchServiceFlowWeight(node, dot16, sFlow);

    // create the queue
    queue = new Queue;
    queue->SetupQueue(node,
                      "FIFO",
                      DOT16_DEFAULT_DATA_QUEUE_SIZE);

    // add into the scheduler
    if (dot16->stationType == DOT16_BS)
    {
        // this is a base station
        MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

        MacDot16BsSsInfo* ssInfo = MacDot16BsGetSsByCid(node, dot16,
                                                        sFlow->cid);

        scheduler = dot16Bs->outScheduler[sFlow->serviceType];
        if ((ssInfo !=NULL) && (!ssInfo->isRegistered)
            && dot16->dot16eEnabled && dot16Bs->isIdleEnabled)
        {
            qBehavior = SUSPEND;
        }
        if (dot16->dot16eEnabled && dot16Bs->isSleepEnabled
            && ssInfo && ssInfo->isSleepEnabled)
        {
            psClass = &ssInfo->psClassInfo[sFlow->psClassType - 1];
        }
    }
    else
    {
        // this is a subscriber station
        MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
        scheduler = dot16Ss->outScheduler[sFlow->serviceType];
        if (dot16->dot16eEnabled && dot16Ss->isSleepModeEnabled)
        {
            psClass = &dot16Ss->psClassParameter[sFlow->psClassType - 1];
        }
    }

    scheduler->addQueue(queue, sFlow->queuePriority);
    scheduler->setRawWeight(sFlow->queuePriority, sFlow->queueWeight);
    scheduler->normalizeWeight();

    if (psClass && psClass->isSleepDuration == TRUE)
    {
        qBehavior = SUSPEND;
    }

    // move packets from temp queue to created queue in scheduler
    msg = sFlow->tmpMsgQueueHead;
    currentTime = node->getNodeTime();
    while (msg != NULL)
    {
        tmpMsg = msg->next;
        msg->next = NULL;

        scheduler->insert(msg,
                          &queueIsFull,
                          sFlow->queuePriority,
                          NULL,
                          currentTime);
        if (queueIsFull)
        {
            ERROR_ReportWarning(
                "Drop the packet beacuse scheduler queue is full");

            MESSAGE_Free(node, msg);
        }

        msg = tmpMsg;
    }
    scheduler->setQueueBehavior(sFlow->queuePriority, qBehavior);
    sFlow->tmpMsgQueueHead = NULL;
    sFlow->tmpMsgQueueTail = NULL;
    sFlow->numBytesInTmpMsgQueue = 0;
}

// /**
// FUNCTION   :: MacDot16BsCheckOutgoingMsg
// LAYER      :: MAC
// PURPOSE    :: BS node check the dequeued packet and take certain action
//               for certain types of message, only mgmt msg are supported
// PARAMETERS ::
// + node      : Node*          : Pointer to the node
// + dot16     : MacDataDot16*  : Pointer to DOT16 structure
// + msg       : Message*       : message going to be sent out
// + msgType   : unsigned       : message type
// RETURN     :: BOOL           : TRUE  if it is the specified type;
//                                FASLE if not
// **/
BOOL MacDot16BsCheckOutgoingMsg(Node* node,
                                MacDataDot16* dot16,
                                Message* msg,
                                unsigned char msgType)
{
    MacDot16MacHeader* macHeader;
    Dot16CIDType cid;
    unsigned char* payload;
    unsigned char mgmtMsgType;
    int macHeaderLen;

    macHeaderLen = sizeof(MacDot16MacHeader);
    payload = (unsigned char*) MESSAGE_ReturnPacket(msg);
    macHeader = (MacDot16MacHeader*) payload;

    cid = MacDot16MacHeaderGetCID(macHeader);

    // check if it is management message & not BW request
    if (MacDot16MacHeaderGetHT(macHeader) == 0 &&
        MacDot16IsManagementCid(node, dot16, cid))
    {
        // it is a management message, get its type
        mgmtMsgType = payload[macHeaderLen];

        if (mgmtMsgType == msgType)
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
        return FALSE;
    }
}

// /**
// FUNCTION   :: MacDot16AddDataGrantMgmtandGenericMacSubHeader
// LAYER      :: MAC
// PURPOSE    :: Add Generic and data grant management (if needed) header
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + sFlow     : MacDot16ServiceFlow* : Pointer to the service flow
// + msgIn     : Message* : incoming msg
// + pduSize   : int : incoming msg PDU size
// RETURN     :: Message* : Return msg pointer
// **/
static
Message* MacDot16AddDataGrantMgmtandGenericMacSubHeader(
    Node* node,
    MacDataDot16* dot16,
    MacDot16ServiceFlow* sFlow,
    Message* msgIn,
    int pduSize)
{
    MacDot16DataGrantSubHeader* grantMgmtSubheader;
    MacDot16MacHeader* macHeader;
    BOOL isCRCEnabled = FALSE;
    MacDot16Ss* dot16Ss;
    MacDot16Bs* dot16Bs;

    if (dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
        isCRCEnabled = dot16Ss->isCRCEnabled;
    }
    else
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
        isCRCEnabled = dot16Bs->isCRCEnabled;
    }
    if ((sFlow != NULL) && sFlow->needPiggyBackBwReq)
    {
       // add the data grant subheader
        MESSAGE_AddHeader(node,
                          msgIn,
                          sizeof(MacDot16DataGrantSubHeader),
                          TRACE_DOT16);

        grantMgmtSubheader =
            (MacDot16DataGrantSubHeader*) MESSAGE_ReturnPacket(msgIn);
        memset((char*) grantMgmtSubheader, 0,
            sizeof(MacDot16DataGrantSubHeader));

        MacDot16DataGrantSubHeaderSetPBR(grantMgmtSubheader,
            sFlow->bwRequested - sFlow->lastBwRequested);
    }// end of if "sFlow->needPiggyBackBwReq"

    // add the generic MAC header first
    MESSAGE_AddHeader(node,
                      msgIn,
                      sizeof(MacDot16MacHeader),
                      TRACE_DOT16);
    macHeader = (MacDot16MacHeader*) MESSAGE_ReturnPacket(msgIn);
    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));
    // set the CID field and type
    MacDot16MacHeaderSetCID(macHeader, sFlow->cid);
    if (isCRCEnabled)
    {
        MacDot16MacHeaderSetCI(macHeader, TRUE);
        if (DEBUG_CRC)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("Add %d bytes virtual CRC information\n", DOT16_CRC_SIZE);
        }
        MESSAGE_AddVirtualPayload(node, msgIn, DOT16_CRC_SIZE);
    }

    MacDot16MacHeaderSetLEN(macHeader, pduSize);

    if (sFlow->needPiggyBackBwReq)
    {
        MacDot16MacHeaderSetGeneralType(
            macHeader,
            DOT16_FAST_FEEDBACK_ALLOCATION_SUBHEADER);
        if (DEBUG_BWREQ)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("piggyback a BW req %d "
                   "for sFlow with cid %d, need BW %d\n",
                    sFlow->bwRequested -
                    sFlow->lastBwRequested,
                    sFlow->cid,
                    sFlow->bwRequested);
        }
                      // increamental BW req only
        sFlow->needPiggyBackBwReq = FALSE;
        if (sFlow->serviceType == DOT16_SERVICE_ertPS)
        {
            sFlow->lastBwRequested = sFlow->bwRequested;
        }
    }// end of if "sFlow->needPiggyBackBwReq"
    return msgIn;
}// end of MacDot16AddDataGrantMgmtandGenericMacSubHeader

// /**
// FUNCTION   :: MacDot16SSPutPacketInToOutBuff
// LAYER      :: MAC
// PURPOSE    :: SS puts packet in to output buffer
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + msgIn     : Message* : incoming msg
// + pduSize   : int : incoming msg PDU size
// RETURN     :: Void* : NULL
// **/
static inline
void MacDot16SSPutPacketInToOutBuff(
    Node* node,
    MacDot16Ss* dot16Ss,
    Message* msgIn,
    int pduSize)
{
    if (msgIn == NULL)
    {
        return;
    }
    //We have info for ARQ timers in the Message
    //MESSAGE_InfoAlloc(node, msgIn, 0);
    if (dot16Ss->outBuffHead == NULL)
    {
        dot16Ss->outBuffHead = msgIn;
    }
    else
    {
        dot16Ss->outBuffTail->next = msgIn;
    }
    while (msgIn->next != NULL)
    {
        msgIn = msgIn->next;
    }
    dot16Ss->outBuffTail = msgIn;
    dot16Ss->ulBytesAllocated += pduSize;
    return;
}// end of MacDot16SSPutPacketInToOutBuff

// /**
// FUNCTION   :: MacDot16BSPutPacketInToOutBuff
// LAYER      :: MAC
// PURPOSE    :: BS puts packet in to output buffer
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16    : MacDataDot16* : Pointer to DOT16 structure
// + ssInfo   : MacDot16BsSsInfo* : Pinter to SS Info
// + msgIn     : Message* : incoming msg
// + pduSizeInPs : int : incoming msg PDU size in PS
// + isBroadCastMsg : BOOL : TRUE / FALSE
// RETURN     :: Void* : NULL
// **/
static inline
void MacDot16BSPutPacketInToOutBuff(
    Node* node,
    MacDot16Bs* dot16Bs,
    MacDot16BsSsInfo* ssInfo,
    Message* msgIn,
    int pduSizeInPs,
    int pduSize,
    BOOL isBroadCastMsg)
{
    if (msgIn == NULL)
    {
        return;
    }

    if (isBroadCastMsg == TRUE)
    {
        // currently, we treat multicast same as bcast
        if (dot16Bs->bcastOutBuffHead == NULL)
        {
            dot16Bs->bcastOutBuffHead = msgIn;
        }
        else
        {
            dot16Bs->bcastOutBuffTail->next = msgIn;
        }
        while (msgIn->next != NULL)
        {
            msgIn = msgIn->next;
        }
        dot16Bs->bcastOutBuffTail = msgIn;
        dot16Bs->bcastPsAllocated = dot16Bs->bcastPsAllocated
            + (UInt16)pduSizeInPs;
        dot16Bs->bytesInBcastOutBuff += pduSize;
    }
    else
    {
        // put into the buffer to the SS
        if (ssInfo->outBuffHead == NULL)
        {
            ssInfo->outBuffHead = msgIn;
        }
        else
        {
            ssInfo->outBuffTail->next = msgIn;
        }
        while (msgIn->next != NULL)
        {
            msgIn = msgIn->next;
        }
        ssInfo->outBuffTail = msgIn;
        if (ssInfo->dlPsAllocated == 0)
        {
            dot16Bs->numDlMapIEScheduled ++;
        }
        ssInfo->dlPsAllocated += dot16Bs->bcastPsAllocated +
            (UInt16)pduSizeInPs;
        ssInfo->bytesInOutBuff += pduSize;
    }
}// end of MacDot16BSPutPacketInToOutBuff


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
                       MacDataDot16* dot16,
                       MacDot16ServiceFlow* sFlow)
{
    sFlow->arqControlBlock =
       (MacDot16ARQControlBlock* )MEM_malloc(sizeof(MacDot16ARQControlBlock));

    memset(sFlow->arqControlBlock,0,sizeof (MacDot16ARQControlBlock));
    sFlow->arqControlBlock->isARQBlockTransmisionEnable = TRUE;
    sFlow->arqControlBlock->numOfARQResetRetry = 0;
    if (dot16->stationType == DOT16_BS)
    {   if (sFlow->sfDirection == DOT16_UPLINK_SERVICE_FLOW)
        {
            sFlow->arqControlBlock->direction = DOT16_ARQ_RX ;
        }
        else
        {
            sFlow->arqControlBlock->direction = DOT16_ARQ_TX;
        }
    }
    else
    {
        if (sFlow->sfDirection == DOT16_UPLINK_SERVICE_FLOW)
        {
            sFlow->arqControlBlock->direction = DOT16_ARQ_TX ;
            }
        else
        {
            sFlow->arqControlBlock->direction = DOT16_ARQ_RX;
        }
    }


    //We will use the ARQ window as a circular queue therefore
    //arqWindowSize + 1 size is used to keep check of overflow/underflow.

    sFlow->arqControlBlock->arqBlockArray =
        (MacDot16ARQBlock* )MEM_malloc( DOT16_ARQ_ARRAY_SIZE *
                                                   sizeof(MacDot16ARQBlock));
    memset(sFlow->arqControlBlock->arqBlockArray,
        0,
        DOT16_ARQ_ARRAY_SIZE * sizeof(MacDot16ARQBlock));
    if (sFlow->arqSyncLossTimer != NULL)
    {
        MESSAGE_CancelSelfMsg(node, sFlow->arqSyncLossTimer);
        sFlow->arqSyncLossTimer = NULL;
    }
    //Starting the SYNC LOSS timer for the service flow
    if (DOT16_ARQ_SYNC_LOSS_TIMEOUT > 0)
    {
        sFlow->arqSyncLossTimer = MacDot16SetTimer(
            node,
            dot16,
            DOT16_ARQ_SYNC_LOSS_TIMER,
            DOT16_ARQ_SYNC_LOSS_TIMEOUT,
            NULL,
            sFlow->cid);
    }
    return;
}

// /**
// FUNCTION   :: MacDot16BuildARQBlock
// LAYER      :: MAC
// PURPOSE    :: Build ARQ block
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + sFlow     : MacDot16ServiceFlow*: Pointer to service flow
// + sduPacket : Message* : Pointer to msg
// RETURN     :: void : NULL
// **/
static
void MacDot16BuildARQBlock(Node* node,
                           MacDot16ServiceFlow *sFlow,
                           Message* sduPacket)
{
    UInt8 fcVal ;
    int numFrags = 0;
    int i = 0 ;
    MacDot16ARQBlock* arqBlockPtr = NULL;
    Message** fragList = NULL;

    MESSAGE_FragmentPacket(
         node,
         sduPacket,
         DOT16_ARQ_BLOCK_SIZE,
         &fragList,
         &numFrags,
         (TraceProtocolType)sduPacket->originatingProtocol);

    //we need to deallocate the memory used by the above list

    if (DEBUG_ARQ)
    {
        printf("The block is fragmented into %d fragments \n",numFrags);

    }

    for (i = 0; i < numFrags ; i++)
    {
        MacDot16ARQSetARQBlockPointer(DOT16_ARQ_WINDOW_REAR);

        arqBlockPtr->blockPtr = fragList[i];

        arqBlockPtr->arqBlockState = DOT16_ARQ_BLOCK_NOT_SENT;
        //fcVal calculation

        if ((i==0) && (numFrags == 1))
        {
            fcVal = DOT16_NO_FRAGMENTATION ;
        }
        else if (i==0)
        {
            fcVal = DOT16_FIRST_FRAGMENT ;
        }
        else if (i+1 == numFrags)
        {
            fcVal = DOT16_LAST_FRAGMENT ;
        }
        else
        {
             fcVal = DOT16_MIDDLE_FRAGMENT ;
        }
        MacDot16FragSubHeaderSetFC(DOT16_ARQ_FRAG_SUBHEADER,fcVal);
        MacDot16Set11bit(DOT16_ARQ_FRAG_SUBHEADER, DOT16_ARQ_TX_NEXT_BSN);
        MacDot16ARQIncBsnId(DOT16_ARQ_TX_NEXT_BSN);
        MacDot16ARQIncIndex(DOT16_ARQ_WINDOW_REAR);
    }//end of for
    //We need to free memory of the array
    MEM_free(fragList);
    return;
}//end of function

// /**
// FUNCTION   :: MacDot16FillARQWindow
// LAYER      :: MAC
// PURPOSE    :: Fill ARQ Window
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + scheduler : Scheduler* : Pointer to service type Scheduler
// + sFlow     : MacDot16ServiceFlow*: Pointer to service flow
// + ssInfo    : MacDot16BsSsInfo* : Pointer to SS Info
// RETURN     :: void : NULL
// **/
static
void MacDot16FillARQWindow(Node* node,
                           MacDataDot16* dot16,
                           Scheduler* scheduler,
                           MacDot16ServiceFlow* sFlow,
                           MacDot16BsSsInfo* ssInfo)
{
    Message* nextPkt = NULL;
    BOOL notEmpty = FALSE;
    clocktype currentTime = node->getNodeTime();
    int priority;
    int freeWindowSpace = 0 ;
    UInt16 numARQBlocks =0;
    int pduSize ;

    if (DEBUG_ARQ)
    {
        MacDot16PrintRunTimeInfo(node,dot16);
        printf("Start MacDot16FillARQWindow\n");
        MacDot16ARQPrintControlBlock(node, sFlow);
    }
    if ((sFlow == NULL)|| (sFlow->arqControlBlock == NULL))
    {
        return;
    }
    else if (sFlow->arqControlBlock->isARQBlockTransmisionEnable == FALSE)
    {
        return;
    }

    while (1)
    {
        notEmpty = scheduler->retrieve(sFlow->queuePriority,
            0,
            &nextPkt,
            &priority,
            PEEK_AT_NEXT_PACKET,
            currentTime);
        if (!notEmpty)
        {
            break;
        }
        pduSize = MESSAGE_ReturnPacketSize(nextPkt);
        //Calculating Avalible Space in the Window
        MacDot16ARQWindowFreeSpace(freeWindowSpace,
                                 DOT16_ARQ_WINDOW_FRONT,
                                 DOT16_ARQ_WINDOW_REAR,
                                 DOT16_ARQ_WINDOW_SIZE);
       if (freeWindowSpace <= 0)
       {
            if (DEBUG_ARQ)
            {
                MacDot16PrintRunTimeInfo(node,dot16);
                printf("No Window Space.!! Returning \n");
            }
            break;
       }
       //Free space available,now we need to check if we can fit
       //the ARQ block into the window.

       //Calculating Number of ARQ blocks Required by the SDU.

        MacDot16ARQCalculateNumBlocks(numARQBlocks, (UInt16)pduSize) ;

        if (numARQBlocks <= freeWindowSpace)
        {
            //the ARQ block can be fit into the window.
            //Dequeue packet.
            scheduler->retrieve(sFlow->queuePriority,
                                0,
                                &nextPkt,
                                &priority,
                                DEQUEUE_PACKET,
                                currentTime);
            MESSAGE_RemoveInfo(node, nextPkt, INFO_TYPE_Dot16BurstInfo);
            //build arq blocks.
            MacDot16BuildARQBlock(node,
                                  sFlow,
                                  nextPkt);

            if (DEBUG_ARQ)
            {
                MacDot16PrintRunTimeInfo(node,dot16);
                printf("PDU received of size %d Service Flow %d."
                    "Available Space in Window %d\n" ,
                    pduSize,sFlow->cid, freeWindowSpace);
            }
        }
        else
        {
            //No space in window ,hence we need not proceed
            if (DEBUG_ARQ)
            {
                printf("The blocks cannot be fit in the window "
                       "numARQBlocks = %d freeWindowSpace = %d \n",
                       numARQBlocks, freeWindowSpace);
            }
            return;
        }
    }//End of while
    if (DEBUG_ARQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("End MacDot16FillARQWindow\n");
        MacDot16ARQPrintControlBlock(node, sFlow);
    }
}//End of Function MacDot16FillARQWindow

// /**
// FUNCTION   :: MacDot16ARQBuildPDUDataPacket
// LAYER      :: MAC
// PURPOSE    :: ARQ build PDU data packet
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + sFlow     : MacDot16ServiceFlow*: Pointer to service flow
// + ssInfo    : MacDot16BsSsInfo*: Pointer to SS Info
// + numFreeBytes : int: Free bytes
// + sduSize    : int&: SDU size
// RETURN     :: void : NULL
// **/
static
Message* MacDot16ARQBuildPDUDataPacket(Node* node,
                                       MacDataDot16* dot16,
                                       MacDot16ServiceFlow* sFlow,
                                       MacDot16BsSsInfo* ssInfo,
                                       int numFreeBytes,
                                       int &sduSize)
{
    BOOL validNotSentStartBsn = FALSE ;
    UInt8 startFcVal = 0;
    UInt8 endFcVal= 0;
    UInt8 fcValPDU = 0;
    int tempArqIndex = DOT16_ARQ_WINDOW_FRONT ;
    UInt16 numARQBlocksToAddToPDU = 0;

    //The following 3 variables are needed to keep track of the number of ARQ
    //blocks which are being sent for the first time and for whom the ARQ
    //Block Lifetime timer needs to be started later.
    UInt8 numARQNotSentBlocks = 0 ;
    UInt16 notSentStartBsn = 0;
    UInt32 arqPDUSize = 0;
    MacDot16Ss* dot16Ss = NULL;
    MacDot16Bs* dot16Bs = NULL;
    Message* msgOut = NULL;
    Message* nextMsg = NULL;
    MacDot16ARQBlock* arqBlockPtr = NULL ;//pointer to ARQ block.
    MacDot16ExtendedorARQEnableFragSubHeader* arqFragSubHeaderPDU = NULL ;

    if (DOT16_ARQ_WINDOW_FRONT == DOT16_ARQ_WINDOW_REAR)
    {
         //Window is empty.No ARQ Block in the window.
        sduSize = 0;
        return NULL;
    }

    //Check if we can fit in any ARQ blocks in the PDU.

    if (dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
        ssInfo = NULL ;
    }
    else
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
    }

    UInt32 arqFragHeaderSize =
        sizeof (MacDot16ExtendedorARQEnableFragSubHeader);
    arqPDUSize = arqFragHeaderSize ;

    if (DEBUG_ARQ)
    {
        printf("Creating PDU,sFlow = %d , Number of free bytes = %d\n",
                sFlow->cid, numFreeBytes);
        MacDot16PrintRunTimeInfo(node,dot16);
    }

    numFreeBytes -= arqFragHeaderSize ;
    MacDot16ARQSetARQBlockPointer(tempArqIndex);
    BOOL flag = FALSE;
    while ((tempArqIndex!= DOT16_ARQ_WINDOW_REAR) &&
          (arqPDUSize <= DOT16_MAX_PDU_SIZE)&&
           ((numFreeBytes)>0) )
    {
        MacDot16ARQSetARQBlockPointer(tempArqIndex);
        if (!((arqBlockPtr->arqBlockState == DOT16_ARQ_BLOCK_NOT_SENT)||
             ( arqBlockPtr->arqBlockState ==
                        DOT16_ARQ_BLOCK_WAIT_FOR_RETRANSMISSION)))
        {//We need to send ARQ blocks only in NOT_SENT state and
         //WAIT_FOR_RETRANSMISSION state
            MacDot16ARQIncIndex(tempArqIndex);
            if (flag == TRUE)
            {
                break;
            }
            continue;
        }

         if (MESSAGE_ReturnPacketSize(arqBlockPtr->blockPtr) >
                                                       numFreeBytes )
         {
             break;
         }
        flag = TRUE;
         arqPDUSize += MESSAGE_ReturnPacketSize(arqBlockPtr->blockPtr);
         numFreeBytes -=
                MESSAGE_ReturnPacketSize(arqBlockPtr->blockPtr);

         if (arqBlockPtr->arqBlockState == DOT16_ARQ_BLOCK_NOT_SENT)
         {
            numARQNotSentBlocks++;
            if (!validNotSentStartBsn)
            {
                notSentStartBsn =
                            MacDot16FragSubHeaderGet11bitBSN(
                            DOT16_ARQ_FRAG_SUBHEADER);
                validNotSentStartBsn = TRUE;
            }
         }

         if (msgOut == NULL)
         {
              msgOut = MESSAGE_Duplicate(node, arqBlockPtr->blockPtr);
              msgOut->next = NULL;
              nextMsg = msgOut;

              MESSAGE_AddHeader
                 (node,
                  msgOut,
                  sizeof(MacDot16ExtendedorARQEnableFragSubHeader),
                  TRACE_DOT16);

              arqFragSubHeaderPDU =
                  (MacDot16ExtendedorARQEnableFragSubHeader*)
                                            MESSAGE_ReturnPacket(msgOut);
              memset(arqFragSubHeaderPDU, 0 ,
                  sizeof(MacDot16ExtendedorARQEnableFragSubHeader));
              startFcVal =
                  MacDot16FragSubHeaderGetFC(DOT16_ARQ_FRAG_SUBHEADER);

              memcpy(arqFragSubHeaderPDU,
                     DOT16_ARQ_FRAG_SUBHEADER,
                     sizeof(MacDot16ExtendedorARQEnableFragSubHeader));
              numARQBlocksToAddToPDU++ ;
        }
        else
        {
             nextMsg->next =
                 MESSAGE_Duplicate(node, arqBlockPtr->blockPtr);
             nextMsg = nextMsg->next;
             nextMsg->next = NULL;
             numARQBlocksToAddToPDU++ ;
             endFcVal =
                 MacDot16FragSubHeaderGetFC(DOT16_ARQ_FRAG_SUBHEADER);
        }

        arqBlockPtr->arqBlockState = DOT16_ARQ_BLOCK_OUTSTANDING ;
        MacDot16ARQIncIndex(tempArqIndex);

        if ((MacDot16FragSubHeaderGetFC(DOT16_ARQ_FRAG_SUBHEADER)==
                                                DOT16_NO_FRAGMENTATION)||
           (MacDot16FragSubHeaderGetFC(DOT16_ARQ_FRAG_SUBHEADER)==
                                                   DOT16_LAST_FRAGMENT))
        {
            //we need to add the PDU/PDUs to the output message buffer
             break;
        }
    }//End of while ((arqBlockCounter < numARQBlocksInWindow)

    if (numARQBlocksToAddToPDU > 0)
    {
        fcValPDU = startFcVal;
        if ((startFcVal == DOT16_FIRST_FRAGMENT) &&
                                        (endFcVal == DOT16_LAST_FRAGMENT))
       {
           //The whole SDU packed into 1 PDU hence fcVal in PDU fragHeader
            // to be set DOT16_NO_FRAGMENTATION
            fcValPDU = DOT16_NO_FRAGMENTATION;
        }
        else if ((startFcVal == DOT16_MIDDLE_FRAGMENT) &&
            (endFcVal == DOT16_LAST_FRAGMENT))
        {
            //Last fragment being sent with this PDU hence fcVal in PDU
            //fragHeader to be set to DOT16_NO_FRAGMENTATION.
            fcValPDU =DOT16_LAST_FRAGMENT;
        }
        else
        {
            //Nothing to do as the fcVal in the PDU header is set correctly
        }

        MacDot16FragSubHeaderSetFC(arqFragSubHeaderPDU, fcValPDU);

        if (DEBUG_ARQ)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("Added %d arqBlocks to the PDU\n",numARQBlocksToAddToPDU);
            printf("Fragmentation Header Contents. startBSNId = %d fcVal = %x\n",
                    MacDot16Get11bit(arqFragSubHeaderPDU),
                    MacDot16FragSubHeaderGetFC(arqFragSubHeaderPDU));
            MacDot16ARQPrintControlBlock(node, sFlow);
            printf("Free Bytes Left = %d \n",numFreeBytes);
        }

        //We need to add info to the message ,the number of ARQ blocks
        // being sent, which were in the not sent state so that
        // we can start the block life time timer for them.

        if (validNotSentStartBsn)
        {
            char* infoPtr = NULL;
            infoPtr = MESSAGE_InfoAlloc(node, msgOut, 3);
            infoPtr[0] = (unsigned char)numARQNotSentBlocks;
            MacDot16ShortToTwoByte(infoPtr[1],infoPtr[2], notSentStartBsn);
        }
        else
        {
            MESSAGE_RemoveInfo(node, msgOut, INFO_TYPE_Dot16BurstInfo);
        }

        if (dot16->stationType == DOT16_SS)
        {
             dot16Ss->stats.numARQBlockSent+= numARQBlocksToAddToPDU;
         }
        else
        {
            dot16Bs->stats.numARQBlockSent+= numARQBlocksToAddToPDU;
            //Starting the ARQ RETRY and BLOCK LIFETIME timers for BS
            MacDot16ARQSetRetryAndBlockLifetimeTimer(node,
                                     dot16,
                                     sFlow,
                                     MESSAGE_ReturnInfo(msgOut),
                                     numARQBlocksToAddToPDU,
                                     MacDot16FragSubHeaderGet11bitBSN(
                                     arqFragSubHeaderPDU));
            MESSAGE_RemoveInfo(node, msgOut, INFO_TYPE_Dot16BurstInfo);
        }
    }//end of (numARQBlocksToAddToPDU > 0)
    else
    {
        numFreeBytes += arqFragHeaderSize;
        if (DEBUG_ARQ)
        {
            printf("The PDU cannot be fit into the available"
                   " bandwidth = %d\n",numFreeBytes);
        }
        arqPDUSize = 0;
    }
    sduSize = arqPDUSize ;
    return msgOut ;
}//end of MacDot16ARQBuildPDUDataPacket

// /**
// FUNCTION   :: MacDot16ARQCreatePDU
// LAYER      :: MAC
// PURPOSE    :: ARQ create PDU data packet
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + sFlow     : MacDot16ServiceFlow*: Pointer to service flow
// + ssInfo    : MacDot16BsSsInfo*: Pointer to SS Info
// + freeBytes : int&: Free bytes
// RETURN     :: void : NULL
// **/
static
void MacDot16ARQCreatePDU(Node* node,
                          MacDataDot16* dot16,
                          MacDot16ServiceFlow* sFlow,
                          MacDot16BsSsInfo* ssInfo,
                          int& freeBytes)
{
    BOOL isCRCEnabled = FALSE;
    MacDot16Ss* dot16Ss = NULL;
    MacDot16Bs* dot16Bs = NULL;
    int pduSize = sizeof(MacDot16MacHeader);
    Message* msgOut = NULL;
    Message* msgInHead = NULL;
    Message* msgInTail = NULL;
    int noOfSDUsPacked = 0;
    int sduSize;
    int numFreeBytes = freeBytes;
    int pduSizeArray[256] = {0, 0};

    if (sFlow->arqControlBlock->isARQBlockTransmisionEnable == FALSE)
    {
        return;
    }

    //Checking whether CRC is Enabled.
    if (dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
        isCRCEnabled = dot16Ss->isCRCEnabled;
        ssInfo = NULL ;
    }
    else
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
        isCRCEnabled = dot16Bs->isCRCEnabled;
    }

    if (numFreeBytes > DOT16_MAX_PDU_SIZE)
    {
        // adjust the num of free bytes as per the pdu size
        numFreeBytes = DOT16_MAX_PDU_SIZE;
    }

    if (isCRCEnabled == TRUE)
    {
        // Add four bytes for CRC
        pduSize += DOT16_CRC_SIZE;
    }
    if (sFlow->needPiggyBackBwReq)
    {
        pduSize += sizeof(MacDot16DataGrantSubHeader);
    }

    if (numFreeBytes <= (pduSize + (int)sizeof(
        MacDot16ExtendedorARQEnableFragSubHeader)))
    {
        // numFreeBytes is less then the requirement
        return;
    }
    numFreeBytes = numFreeBytes - pduSize -
        sizeof(MacDot16ExtendedorARQEnableFragSubHeader);
    msgOut = NULL;
    msgInHead = NULL;
    msgInTail = NULL;
    while (numFreeBytes > 0)
    {
        sduSize = 0;
        if (noOfSDUsPacked > 0)
        {
            int tempNumFreeBytes = numFreeBytes;
            if (noOfSDUsPacked == 1)
            {
                numFreeBytes +=
                    sizeof(MacDot16ExtendedorARQEnableFragSubHeader);
                numFreeBytes -=
                    sizeof(MacDot16ExtendedorARQEnablePackSubHeader);
            }
            numFreeBytes -= sizeof(MacDot16ExtendedorARQEnablePackSubHeader);
            if (numFreeBytes <= 0)
            {
                numFreeBytes = tempNumFreeBytes;
                break;
            }
            //sduSize = sizeof(MacDot16ExtendedorARQEnablePackSubHeader);
        }
        msgOut = MacDot16ARQBuildPDUDataPacket(node,
                                               dot16,
                                               sFlow,
                                               ssInfo,
                                               numFreeBytes,
                                               sduSize);
                // return only length of data (sduSize)
        if (msgOut == NULL)
        {
            break;
        }
        numFreeBytes -= sduSize;
        pduSizeArray[noOfSDUsPacked] = sduSize;
        noOfSDUsPacked++;
        if (msgInHead == NULL)
        {
            msgInHead = msgOut;
        }
        else
        {
            msgInTail->next = msgOut;
        }
        while (msgOut->next != NULL)
        {
            msgOut = msgOut->next;
        }
        msgInTail = msgOut;

        if (!sFlow->isPackingEnabled)
        {
            break;
        }
    }// end of while

    if (noOfSDUsPacked == 0)
    {
        return;
    }

    // Add packing header if req
    msgOut = msgInHead;
    if (noOfSDUsPacked > 1 && sFlow->isPackingEnabled)
    {
        MacDot16ExtendedorARQEnablePackSubHeader* subHeader = NULL;
        MacDot16ExtendedorARQEnableFragSubHeader* fragSubHeader = NULL;
        fragSubHeader = (MacDot16ExtendedorARQEnableFragSubHeader*)
            MEM_malloc(sizeof(MacDot16ExtendedorARQEnableFragSubHeader));
        for (int i =0; i < noOfSDUsPacked; i++)
        {
            Message* startPDU = msgOut;
            Message* endPDU = msgOut;
            pduSizeArray[i] -=
                sizeof(MacDot16ExtendedorARQEnableFragSubHeader);
            int tempPDUSize = pduSizeArray[i];

            // copy the fragmentation header info in to local header
            memcpy(fragSubHeader, MESSAGE_ReturnPacket(startPDU),
                sizeof(MacDot16ExtendedorARQEnableFragSubHeader));
            // remove fragmentation header
            MESSAGE_RemoveHeader(node,
                startPDU,
                sizeof(MacDot16ExtendedorARQEnableFragSubHeader),
                TRACE_DOT16);
            // skip pduSize[i] length of data msg
            while (tempPDUSize > 0)
            {
                tempPDUSize -= MESSAGE_ReturnPacketSize(endPDU);
                endPDU = endPDU->next;
            }
            msgOut = endPDU;

            // add packing subheader in startPDU
            if (DEBUG_ARQ && DEBUG_PACKING_FRAGMENTATION)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("ARQ add Packing Subheader\n");
            }
            // Add variable header
            MESSAGE_AddHeader(
                node,
                startPDU,
                sizeof(MacDot16ExtendedorARQEnablePackSubHeader),
                TRACE_DOT16);
            subHeader = (MacDot16ExtendedorARQEnablePackSubHeader*)
                MESSAGE_ReturnPacket(startPDU);
            memset(subHeader,
                0,
                sizeof(MacDot16ExtendedorARQEnablePackSubHeader));
            UInt8 fc = MacDot16FragSubHeaderGetFC(fragSubHeader);
            MacDot16PackSubHeaderSetFC(subHeader, fc);
            UInt16 bsn = MacDot16FragSubHeaderGet11bitBSN(fragSubHeader);
            MacDot16PackSubHeaderSet11bitBSN(subHeader, bsn);

            MacDot16ExtendedARQEnablePackSubHeaderSet11bitLength(
                subHeader,
                pduSizeArray[i]);

            pduSize += pduSizeArray[i] +
                sizeof(MacDot16ExtendedorARQEnablePackSubHeader);
        }// end of for
        MEM_free(fragSubHeader);
        fragSubHeader = NULL;
    }
    else
    {
        int tempPduSize = pduSize;
        pduSize = 0;
        for (int i = 0; i < noOfSDUsPacked; i++)
        {
            pduSizeArray[i] += tempPduSize;
            pduSize += pduSizeArray[i];
        }
    }// end of else

// add genric mac header and  data grant mgmt subheader
    if (sFlow->isPackingEnabled)
    {
        msgInHead = MacDot16AddDataGrantMgmtandGenericMacSubHeader(
            node,
            dot16,
            sFlow,
            msgInHead,
            pduSize);

        MacDot16MacHeader* macHeader =
            (MacDot16MacHeader*) MESSAGE_ReturnPacket(msgInHead);
        if (noOfSDUsPacked > 1)
        {
            MacDot16MacHeaderSetGeneralType(macHeader,
                DOT16_PACKING_SUBHEADER);
        }
        if (dot16->stationType == DOT16_BS)
        {
            dot16Bs->stats.numPktsToLower ++;
            // update dynamic stats
            if (node->guiOption)
            {
                GUI_SendIntegerData(node->nodeId,
                                    dot16Bs->stats.numPktToPhyGuiId,
                                    dot16Bs->stats.numPktsToLower,
                                    node->getNodeTime());
            }
        }
        else
        {
            dot16Ss->stats.numPktsToLower ++;
            // update dynamic stats
            if (node->guiOption)
            {
                GUI_SendIntegerData(node->nodeId,
                                    dot16Ss->stats.numPktToPhyGuiId,
                                    dot16Ss->stats.numPktsToLower,
                                    node->getNodeTime());
            }
        }
    }
    else
    {
        msgOut = msgInHead;
        int tempPDUSize;
        Message* startPDU;
        Message* endPDU;
        for (int i = 0; i < noOfSDUsPacked; i++)
        {
            tempPDUSize = pduSizeArray[i];
            startPDU = msgOut;
            endPDU = msgOut;
            msgOut = MacDot16AddDataGrantMgmtandGenericMacSubHeader(
                node,
                dot16,
                sFlow,
                msgOut,
                tempPDUSize);
            while (tempPDUSize > 0)
            {
                tempPDUSize -= MESSAGE_ReturnPacketSize(endPDU);
                endPDU = endPDU->next;
            }

            // skip pduSize[i] length of data msg

            if (dot16->stationType == DOT16_BS)
            {
                dot16Bs->stats.numPktsToLower ++;
                // update dynamic stats
                if (node->guiOption)
                {
                    GUI_SendIntegerData(node->nodeId,
                                        dot16Bs->stats.numPktToPhyGuiId,
                                        dot16Bs->stats.numPktsToLower,
                                        node->getNodeTime());
                }
            }
            else
            {
                dot16Ss->stats.numPktsToLower ++;
                // update dynamic stats
                if (node->guiOption)
                {
                    GUI_SendIntegerData(node->nodeId,
                                        dot16Ss->stats.numPktToPhyGuiId,
                                        dot16Ss->stats.numPktsToLower,
                                        node->getNodeTime());
                }
            }

            msgOut = endPDU;
        }// end of for
    }
    // add pdus in to out buff head
    if (dot16->stationType == DOT16_SS)
    {
         MacDot16SSPutPacketInToOutBuff(node,
                                        dot16Ss,
                                        msgInHead,
                                        pduSize);
     }
    else
    {
        int pduSizeInPs = 0;
        MacDot16DlBurstProfile* burstProfile = NULL;
        burstProfile = &(dot16Bs->dlBurstProfile[ssInfo->diuc]);
        pduSizeInPs = MacDot16PhyBytesToPs(node,
                    dot16,
                    pduSize,
                    burstProfile,
                    DOT16_DL);

        MacDot16BSPutPacketInToOutBuff(
                        node,
                        dot16Bs,
                        ssInfo,
                        msgInHead,
                        pduSizeInPs,
                        pduSize,
                        FALSE);
    }
    // reduce the freeBytes variable
    freeBytes -= pduSize;
    return;
}//end of MacDot16ARQCreatePDU

/**
// FUNCTION   :: MacDot16PrintARQParameter
// LAYER      :: MAC
// PURPOSE    :: Print ARQ parameters
// PARAMETERS ::
   + arqParam : MacDot16ARQParam* : Pointer to an ARQ structure
// RETURN     :: void : NULL
**/
void MacDot16PrintARQParameter(MacDot16ARQParam* arqParam)
{
    char clockStr[MAX_STRING_LENGTH];
    if (arqParam)
    {
        printf("    ARQ-WINDOW-SIZE  = %d\n", arqParam->arqWindowSize);

        ctoa(arqParam->arqRetryTimeoutTxDelay, clockStr);
        printf("    ARQ-RETRY-TIMEOUT-TRANSMITTER-DELAY = %s\n",
                                                        clockStr);

        ctoa(arqParam->arqRetryTimeoutRxDelay, clockStr);
        printf("    ARQ-RETRY-TIMEOUT-RECEIVER-DELAY = %s\n", clockStr);

        ctoa(arqParam->arqBlockLifeTime, clockStr);
        printf("    ARQ-BLOCK-LIFE-TIME = %s\n", clockStr);

        ctoa(arqParam->arqSyncLossTimeout, clockStr);
        printf("    ARQ-SYNC-LOSS-INTERVAL = %s\n", clockStr);

        printf("    ARQ-DELIVER-IN-ORDER = %d\n",
                        arqParam->arqDeliverInOrder);

        ctoa(arqParam->arqRxPurgeTimeout, clockStr);
        printf("    ARQ-RX-PURGE-TIMEOUT = %s\n", clockStr);

        printf("    ARQ-BLOCK-SIZE = %d\n", arqParam->arqBlockSize);
    }
    else
    {
        printf("    ARQ-Pointer is Null: Error \n");
    }
}
/**
// FUNCTION   :: MacDot16ARQPrintControlBlock
// LAYER      :: MAC
// PURPOSE    :: Print ARQ control blocks
// PARAMETERS ::
// + node      : Node* : Pointer to node
// + sFlow     : MacDot16ServiceFlow*: Pointer to service flow
// RETURN     :: void : NULL
**/
void MacDot16ARQPrintControlBlock(Node* node, MacDot16ServiceFlow* sFlow)
{
    int i = 0 ;
    UInt16 tempBsnId;
    UInt8 tempFcVal ;

    if (!DEBUG_ARQ_WINDOW || sFlow->activated == FALSE || sFlow->admitted
        == FALSE)
    {
        return;
    }

    printf("\nThe Contents of the Control Block:\n"
            "cid = %3d, Front =%3d, Rear =%3d,direction = %s, ",sFlow->cid,
           DOT16_ARQ_WINDOW_FRONT,DOT16_ARQ_WINDOW_REAR,
           sFlow->arqControlBlock->direction?"RECEIVER":"TRANSMITTER");

    if (sFlow->arqControlBlock->direction == DOT16_ARQ_TX)
    {
        printf(" arqTxNextBsn =%3d , arqTxWindowStart =%3d\n",
            DOT16_ARQ_TX_NEXT_BSN,DOT16_ARQ_TX_WINDOW_START);
    }
    else
    {
        printf(" arqRxHighestBsn =%3d , arqRxWindowStart =%3d\n",
            DOT16_ARQ_RX_HIGHEST_BSN,DOT16_ARQ_RX_WINDOW_START);
    }

    MacDot16ARQBlock* arqBlockPtr ;
    printf("The Contents of the ARQ Window:\n");
    if (DOT16_ARQ_WINDOW_FRONT == DOT16_ARQ_WINDOW_REAR)
     {
        printf("Window is Empty\n");

     }
    for (i = DOT16_ARQ_WINDOW_FRONT;i!=DOT16_ARQ_WINDOW_REAR;
                                                    MacDot16ARQIncIndex(i))
        {
            MacDot16ARQSetARQBlockPointer(i);
            printf("index =%3d",i);
            tempBsnId = MacDot16Get11bit(DOT16_ARQ_FRAG_SUBHEADER);
            printf(" BsnId =%3u",
                            tempBsnId);
            printf(" State =%3u",arqBlockPtr->arqBlockState);
            if (arqBlockPtr->blockPtr != NULL)
            {
                printf(" Message Size =%3d",
                MESSAGE_ReturnPacketSize(arqBlockPtr->blockPtr));
            }
            else
            {
                printf(" Message Size =%3d",0);
            }

            tempFcVal = MacDot16FragSubHeaderGetFC(DOT16_ARQ_FRAG_SUBHEADER);
            printf(" fcVal = %2x\n",tempFcVal);
        }
    printf("\n");

    if ((Int16)((DOT16_ARQ_WINDOW_REAR + 1)% DOT16_ARQ_WINDOW_SIZE) ==
                                                    DOT16_ARQ_WINDOW_FRONT)
     {
        printf("Window is Full.\n");
     }

  }//end of function MacDot16ARQPrintControlBlock


// /**
// FUNCTION   :: MacDot16ARQCheckIfBSNInWindow
// LAYER      :: MAC
// PURPOSE    :: Check incoming BSN id present in ARQ window
// PARAMETERS ::
// + sFlow     : MacDot16ServiceFlow*: Pointer to service flow
// + incomingBsnId : Int16: Incoming BSN ID
// RETURN     :: BOOL : TRUE / FALSE
// **/
BOOL MacDot16ARQCheckIfBSNInWindow(MacDot16ServiceFlow* sFlow,
                                   Int16 incomingBsnId)
{
    UInt16 frontBsnId;
    // As per section 6.3.4.6.1 normalization of BSN is accomplished by
    // BSN' = (BSN - BSNbase) mod ARQ_BSN_MODULUS
    // where base values for trasmitter and receiver state machines are
    // ARQ_TX_WINDOW_START and ARQ_RX_WINDOW_START respectively
    if (sFlow->arqControlBlock->direction == DOT16_ARQ_TX)
    {
        frontBsnId = DOT16_ARQ_TX_WINDOW_START;
    }
    else
    {
        frontBsnId = DOT16_ARQ_RX_WINDOW_START;
    }

    UInt16 normalizedBSN = ((UInt16)incomingBsnId -
                           frontBsnId +
                           DOT16_ARQ_BSN_MODULUS)%
                           DOT16_ARQ_BSN_MODULUS;

    if (normalizedBSN < DOT16_ARQ_WINDOW_SIZE)
    {
        return TRUE;
    }
    return FALSE ;
}
// /**
// FUNCTION   :: MacDot16ARQSDUStartIsRight
// LAYER      :: MAC
// PURPOSE    :: Check new SDu started
// PARAMETERS ::
// + arqBlockPtr : MacDot16ARQBlock* : Pointer to an ARQ block
// RETURN     :: BOOL : TRUE / FALSE
// **/
static
BOOL MacDot16ARQSDUStartIsRight(MacDot16ARQBlock* arqBlockPtr)
{

    BOOL a = (MacDot16FragSubHeaderGetFC(DOT16_ARQ_FRAG_SUBHEADER) ==
                                                    DOT16_NO_FRAGMENTATION);
    BOOL b = (MacDot16FragSubHeaderGetFC(DOT16_ARQ_FRAG_SUBHEADER) ==
                                                     DOT16_FIRST_FRAGMENT);
    BOOL c = (arqBlockPtr->blockPtr != NULL);

    if ((a||b)&& c)
    {
        return TRUE;
    }
    return FALSE ;
}
// PURPOSE    ::To make an SDU block out of ARQ blocks
//              that have been successively received in the
//              ARQ window
// /**
// FUNCTION   :: MacDot16ARQBuildSDU
// LAYER      :: MAC
// PURPOSE    :: ARQ build SDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + sFlow     : MacDot16ServiceFlow*: Pointer to service flow
// + ssInfo    : MacDot16BsSsInfo*: Pointer to SS Info
// RETURN     :: void : NULL
// **/
void MacDot16ARQBuildSDU(Node* node,
                    MacDataDot16* dot16,
                    MacDot16ServiceFlow* sFlow,
                    MacDot16BsSsInfo* ssInfo)
{
    BOOL sduFound = FALSE;
    int i = 0;
    int j = 0;
    int startIndex = 0;
    int endIndex = 0;
    UInt16 startBsnId = 0;
    UInt16 endBsnId = 0;
    Message** fragList ;
    Message* sduMsg;
    MacDot16ARQBlock* arqBlockPtr = NULL;
    BOOL missingBlock = FALSE;
    if (DEBUG_ARQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Building SDU to send to upper Layer."
                " cid = %d \n",sFlow->cid);
        MacDot16ARQPrintControlBlock(node, sFlow);
    }

    for (i = DOT16_ARQ_WINDOW_FRONT; i != DOT16_ARQ_WINDOW_REAR;
        MacDot16ARQIncIndex(i))
    {
        MacDot16ARQSetARQBlockPointer(i);
        sduFound=FALSE;
        //We need to discard such Blocks
        if (!MacDot16ARQSDUStartIsRight(arqBlockPtr))
        {
            // Blocks should be discarded only when a discard message is
            // received Also while building the SDU we must search for
            // starting point of SDU no matter whether subsequeent blocks
            // have been received. This will ensure that SDU are handed to
            // upper layer sequentially. This implies if some blocks got
            // missing then susequent SDU shall not be handed untill the
            // missing block is received
            break;
        }

        if (((MacDot16FragSubHeaderGetFC(DOT16_ARQ_FRAG_SUBHEADER) ==
            DOT16_NO_FRAGMENTATION)) && (arqBlockPtr->blockPtr != NULL))
        {
            //sduMsg = MESSAGE_Duplicate(node, arqBlockPtr->blockPtr);
            sduMsg = arqBlockPtr->blockPtr;
            if (dot16->stationType == DOT16_BS)
            {
                MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
                MacDot16CsPacketFromLower(node,
                                          dot16,
                                          sduMsg,
                                          ssInfo->macAddress,
                                          ssInfo);
                // increase statistics
                dot16Bs->stats.numPktsFromLower++;
                // update dynamic stats
                if (node->guiOption)
                {
                    GUI_SendIntegerData(node->nodeId,
                                        dot16Bs->stats.numPktFromPhyGuiId,
                                        dot16Bs->stats.numPktsFromLower,
                                        node->getNodeTime());
                }
            }
            else
            {
                MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
                // pass to upper layer
                MacDot16CsPacketFromLower(
                    node,
                    dot16,
                    sduMsg,
                    dot16Ss->servingBs->bsId);
                // increase statistics
                dot16Ss->stats.numPktsFromLower ++;
                // update dynamic stats
                if (node->guiOption)
                {
                    GUI_SendIntegerData(node->nodeId,
                                        dot16Ss->stats.numPktFromPhyGuiId,
                                        dot16Ss->stats.numPktsFromLower,
                                        node->getNodeTime());
                }
            }
            //Delete ARQ block;
            if (arqBlockPtr->arqRxPurgeTimer != NULL)
            {
                MESSAGE_CancelSelfMsg(node, arqBlockPtr->arqRxPurgeTimer);
                arqBlockPtr->arqRxPurgeTimer = NULL;
            }
            MacDot16ResetARQBlockPtr(node, arqBlockPtr, FALSE);
            MacDot16ARQIncIndex(DOT16_ARQ_WINDOW_FRONT);
            sduFound = TRUE ;
         }
        else if (((MacDot16FragSubHeaderGetFC(DOT16_ARQ_FRAG_SUBHEADER) ==
            DOT16_FIRST_FRAGMENT)) && (arqBlockPtr->blockPtr != NULL))
        {
            startIndex = i ;
            MacDot16ARQSetARQBlockPointer(startIndex);
            startBsnId = MacDot16Get11bit(DOT16_ARQ_FRAG_SUBHEADER);

            for (j = startIndex; j!=DOT16_ARQ_WINDOW_REAR;
                MacDot16ARQIncIndex(j))
            {
                MacDot16ARQSetARQBlockPointer(j);
                if (arqBlockPtr->blockPtr == NULL)
                {
                    i = startIndex;
                    missingBlock = TRUE;
                    break;
                }
                if (arqBlockPtr->blockPtr &&
                    (MacDot16FragSubHeaderGetFC(DOT16_ARQ_FRAG_SUBHEADER) ==
                        DOT16_LAST_FRAGMENT))
                {
                    endIndex = j;
                    MacDot16ARQSetARQBlockPointer(endIndex);
                    endBsnId = MacDot16Get11bit(DOT16_ARQ_FRAG_SUBHEADER);
                    sduFound = TRUE;
                    break;
                }
            }
            if ((!sduFound && (j == DOT16_ARQ_WINDOW_REAR)) || missingBlock)
            {
                break;
            }
            if ((sduFound)&& (((UInt16)endIndex - startIndex +
                (UInt16)DOT16_ARQ_WINDOW_SIZE) % (UInt16)DOT16_ARQ_WINDOW_SIZE
                == (endBsnId - startBsnId + DOT16_ARQ_BSN_MODULUS)
                % DOT16_ARQ_BSN_MODULUS))
            {
                int numFrags =
                    ((endBsnId - startBsnId +
                        DOT16_ARQ_BSN_MODULUS )%
                                    DOT16_ARQ_BSN_MODULUS) + 1;
                i = (i + numFrags - 1) % DOT16_ARQ_WINDOW_SIZE;

                fragList = (Message**)
                        MEM_malloc(sizeof(Message*) * numFrags);
                ERROR_Assert
                    (fragList != NULL, "Out of memory!");

                j = startIndex;
                UInt16 temp;
                for (temp = 0 ; temp < numFrags ; temp++)
                {
                    MacDot16ARQSetARQBlockPointer(j);
                    fragList[temp] = arqBlockPtr->blockPtr;
                    MacDot16ARQIncIndex(j);
                    if (arqBlockPtr->arqRxPurgeTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(node,
                            arqBlockPtr->arqRxPurgeTimer);
                        arqBlockPtr->arqRxPurgeTimer = NULL;
                    }
                    MacDot16ResetARQBlockPtr(node, arqBlockPtr, FALSE);
                }

                if (startIndex == DOT16_ARQ_WINDOW_FRONT)
                {
                    DOT16_ARQ_WINDOW_FRONT = (Int16)MacDot16ARQIncIndex(
                        endIndex);
                }

                //reassembling the SDU packet.
                sduMsg = MESSAGE_ReassemblePacket(
                                        node,
                                        fragList,
                                        numFrags,
                                        (TraceProtocolType)
                                        fragList[0]->originatingProtocol);

                unsigned char* lastHopMacAddr;
                if (dot16->stationType == DOT16_SS)
                {
                    MacDot16Ss* dot16Ss =
                            (MacDot16Ss*) dot16->ssData;
                    lastHopMacAddr = dot16Ss->servingBs->bsId;
                    // increase statistics
                    dot16Ss->stats.numPktsFromLower++;
                    // update dynamic stats
                    if (node->guiOption)
                    {
                        GUI_SendIntegerData(node->nodeId,
                                            dot16Ss->stats.numPktFromPhyGuiId,
                                            dot16Ss->stats.numPktsFromLower,
                                            node->getNodeTime());
                    }
                }
                else
                {
                    MacDot16Bs* dot16Bs =
                                (MacDot16Bs*) dot16->bsData;
                    lastHopMacAddr = ssInfo->macAddress ;
                    // increase statistics
                    dot16Bs->stats.numPktsFromLower++;
                    // update dynamic stats
                    if (node->guiOption)
                    {
                        GUI_SendIntegerData(node->nodeId,
                                            dot16Bs->stats.numPktFromPhyGuiId,
                                            dot16Bs->stats.numPktsFromLower,
                                            node->getNodeTime());
                    }
                }

                MacDot16CsPacketFromLower(node,
                                          dot16,
                                          sduMsg,
                                          lastHopMacAddr,
                                          ssInfo);
                MEM_free(fragList);
            }//end of if ((sduFound)&&
        }// end of else if
        if ((DEBUG_ARQ)&& sduFound)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("SDU sent to upper Layer sflow cid = %d.\n",
                 sFlow->cid);
            MacDot16ARQPrintControlBlock(node, sFlow);
        }
        else if (DEBUG_ARQ)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("No SDU to send to upper layer \n");
        }
    }//end of for
}// end of function MacDot16ARQBuildSDU()

// /**
// FUNCTION   :: MacDot16ARQAddToArray
// LAYER      :: MAC
// PURPOSE    :: To make an SDU block out of ARQ blocks that have been
//               successively received in the ARQ window
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + msg       : Message*: Msg pointer
// + sFlow     : MacDot16ServiceFlow*: Pointer to service flow
// + incomingBsnId : UInt16: Incoming BSN ID
// + incomingFcVal : UInt8: Incoming FC val
// + numARQBlocksInPDU : Int16: No of ARQ blocks in PDU
// RETURN     :: Message* : Return msg pointer
// **/
static
Message* MacDot16ARQAddToArray(Node* node,
                               MacDataDot16* dot16,
                               Message* msg,
                               MacDot16ServiceFlow *sFlow,
                               UInt16 incomingBsnId,
                               UInt8 incomingFcVal,
                               Int16 numARQBlocksInPDU)
{
    BOOL isDuplicate = FALSE;
    UInt16 bsnId = incomingBsnId;
    UInt8 fcVal = incomingFcVal;
    Int16 freeWindowSpace = 0;
    Int16 arqWindowStart = DOT16_ARQ_RX_WINDOW_START ;
    Int16 arqBlocksAddedToWindow = 0;
    MacDot16ARQBlock* arqBlockPtr = NULL ;
    Message* tempMsg = NULL ;
    Int16 numARQBlocks = numARQBlocksInPDU ;
    MacDot16Bs* dot16Bs = NULL;
    MacDot16Ss* dot16Ss = NULL;
    // Bug fix # 5434 start
    BOOL storeBlock = TRUE;
    // Bug fix # 5434 end

    if (dot16->stationType == DOT16_BS)
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
    }
    else
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
    }

     if (DEBUG_ARQ)
     {
         MacDot16PrintRunTimeInfo(node, dot16);
         printf("Adding PDU to ARQ window. "
                 "IncomingBsnId = %d numARQBlocks =%d\n",
                  incomingBsnId,numARQBlocks);
         MacDot16ARQPrintControlBlock(node, sFlow);
     }

    do
    {
        if (sFlow->arqControlBlock->isARQBlockTransmisionEnable == TRUE)
        {
            BOOL inWindow = FALSE;
            UInt16 numunAckArqBlocks = 0;
            int index = MacDot16ARQFindBSNInWindow(sFlow,
                bsnId,
                1,
                inWindow,
                numunAckArqBlocks);

            isDuplicate = FALSE;
            MacDot16ARQWindowFreeSpace(freeWindowSpace,
                                         DOT16_ARQ_WINDOW_FRONT,
                                         DOT16_ARQ_WINDOW_REAR,
                                         (Int16) DOT16_ARQ_WINDOW_SIZE);
            // Bug fix # 5434 start
            MacDot16ARQSetARQBlockPointer(index);

            // As per fiigure 36 section 6.3.4.6.3 check for duplicate
            if ((arqBlockPtr->arqBlockState == DOT16_ARQ_BLOCK_RECEIVED)
                    && arqBlockPtr->blockPtr != NULL)
            {
                isDuplicate = TRUE ;
                if (DEBUG_ARQ)
                {
                    printf("Duplicate Message Received.\n");
                }

                if (arqBlockPtr->arqRxPurgeTimer !=NULL)
                {
                    MESSAGE_CancelSelfMsg(node,
                        arqBlockPtr->arqRxPurgeTimer);
                    arqBlockPtr->arqRxPurgeTimer = NULL;
                }

                if (DOT16_ARQ_RX_PURGE_TIMEOUT > 0)
                {
                    arqBlockPtr->arqRxPurgeTimer = MacDot16SetTimer(
                        node,
                        dot16,
                        DOT16_ARQ_RX_PURGE_TIMER,
                        DOT16_ARQ_RX_PURGE_TIMEOUT,
                        NULL,
                        UInt16(sFlow->cid),
                        UInt16(bsnId));
                }
            }

            if (((inWindow) && (freeWindowSpace > 0)) && (!isDuplicate))
            // Bug fix # 5434 end
            {
                if (bsnId >= DOT16_ARQ_RX_HIGHEST_BSN)
                {
                    DOT16_ARQ_RX_HIGHEST_BSN =
                        (bsnId + 1)%DOT16_ARQ_BSN_MODULUS;
                }

                if (bsnId == arqWindowStart)
                {
                    MacDot16ARQIncBsnId(arqWindowStart);
                    // Bug fix # 5434 start
                    //we need to reset the sync loss timer in here
                    if (sFlow->arqSyncLossTimer != NULL)
                    {
                        MESSAGE_CancelSelfMsg(node, sFlow->arqSyncLossTimer);
                        sFlow->arqSyncLossTimer = NULL;
                    }
                    if (DOT16_ARQ_SYNC_LOSS_TIMEOUT > 0)
                    {
                        sFlow->arqSyncLossTimer = MacDot16SetTimer(
                            node,
                            dot16,
                            DOT16_ARQ_SYNC_LOSS_TIMER,
                            DOT16_ARQ_SYNC_LOSS_TIMEOUT,
                            NULL,
                            sFlow->cid);
                    }
                    // Increment rear only when block corresponding to rear
                    // is received. If any misssing block is received then
                    // rear should not be incremented.
                    if (index == DOT16_ARQ_WINDOW_REAR)
                    {
                        MacDot16ARQIncIndex(DOT16_ARQ_WINDOW_REAR);
                    }
                    storeBlock = TRUE;
                    // Bug fix # 5434 end
                }
                else
                {
                    MacDot16ARQSetARQBlockPointer(index);
                    if (arqBlockPtr->arqRxPurgeTimer !=NULL)
                    {
                        // Bug fix # 5395 start
                        MESSAGE_CancelSelfMsg(node,
                            arqBlockPtr->arqRxPurgeTimer);
                        // Bug fix # 5395 end
                        arqBlockPtr->arqRxPurgeTimer = NULL;
                    }
                    if (DOT16_ARQ_RX_PURGE_TIMEOUT > 0)
                    {
                        arqBlockPtr->arqRxPurgeTimer = MacDot16SetTimer(
                            node,
                                 dot16,
                                 DOT16_ARQ_RX_PURGE_TIMER,
                                 DOT16_ARQ_RX_PURGE_TIMEOUT,
                                 NULL,
                                 UInt16(sFlow->cid),
                                 UInt16(bsnId));
                        if (DEBUG_ARQ)
                        {
                            printf("Setting arqPurgeTimer for node %d"
                            " Sflow %d StartingBSNId %d \n" ,
                            node->nodeId, sFlow->cid, bsnId);
                        }
                    }
                    // Bug fix # 5434 start
                    DOT16_ARQ_WINDOW_REAR = MacDot16ARQIncIndex(index);
                    storeBlock = TRUE;
                    // Bug fix # 5434 end
                }
            }
            // Bug fix # 5434 start
            if (!isDuplicate && storeBlock && inWindow)
            // Bug fix # 5434 end
            {
                 arqBlockPtr->blockPtr = MESSAGE_Duplicate(node, msg);
                 arqBlockPtr->blockPtr->next = NULL;
                 arqBlockPtr->arqBlockState = DOT16_ARQ_BLOCK_RECEIVED;
                 MacDot16Set11bit(DOT16_ARQ_FRAG_SUBHEADER, bsnId);

                 arqBlocksAddedToWindow++;

                if (incomingFcVal == DOT16_NO_FRAGMENTATION)
                {
                    if (numARQBlocksInPDU == 1)
                    {
                        fcVal = DOT16_NO_FRAGMENTATION;
                    }
                    else
                    {
                        if (numARQBlocks == numARQBlocksInPDU)
                        {
                            fcVal = DOT16_FIRST_FRAGMENT;
                        }
                        else if (numARQBlocks != 1)
                        {
                            fcVal = DOT16_MIDDLE_FRAGMENT;
                        }
                        else if (numARQBlocks == 1)
                        {
                            fcVal = DOT16_LAST_FRAGMENT;
                        }
                    }
                }
                else if (incomingFcVal == DOT16_FIRST_FRAGMENT)
                {
                     if (numARQBlocks == numARQBlocksInPDU)
                     {
                         fcVal = DOT16_FIRST_FRAGMENT;
                     }
                     else
                     {
                         fcVal = DOT16_MIDDLE_FRAGMENT;
                     }
                }
                else if (incomingFcVal == DOT16_MIDDLE_FRAGMENT)
                {
                    fcVal = DOT16_MIDDLE_FRAGMENT;
                }
                else if (incomingFcVal == DOT16_LAST_FRAGMENT)
                {
                    if ((numARQBlocksInPDU == 1) || (numARQBlocks == 1))
                    {
                        fcVal = DOT16_LAST_FRAGMENT;
                    }
                    else
                    {
                        fcVal = DOT16_MIDDLE_FRAGMENT;
                    }
                }
                MacDot16FragSubHeaderSetFC(DOT16_ARQ_FRAG_SUBHEADER,
                    fcVal);
            }
            else
            {
             //if not in window or if its a duplicate message,
             //then discard the PDU.
                    // discard the pdu
            }
        }// end of if sFlow->arqControlBlock->isARQBlockTransmisionEnable
         bsnId = (bsnId + 1) % DOT16_ARQ_BSN_MODULUS;
         tempMsg = msg;
         msg = msg->next;
         MESSAGE_Free(node, tempMsg);
    }while (--numARQBlocks);

    if (dot16->stationType == DOT16_BS)
    {
        dot16Bs->stats.numARQBlockRcvd +=(arqBlocksAddedToWindow);
        dot16Bs->stats.numARQBlockDiscard += (numARQBlocksInPDU
            - arqBlocksAddedToWindow);
    }
    else
    {
        dot16Ss->stats.numARQBlockRcvd+=(arqBlocksAddedToWindow);
        dot16Ss->stats.numARQBlockDiscard += (numARQBlocksInPDU
            - arqBlocksAddedToWindow);
    }
    // Bug fix # 5434 start
    // As per section 6.3.4.6.3 when ARQ_RX_WINDOW is adnaveced any
    // BSN value that corresponds
    // to block that have not been received residing in the interval between
    // the previous and current ARQ_RX_WINDOW should be marked received.
    if (DOT16_ARQ_RX_WINDOW_START != arqWindowStart)
    {
         for (Int32 index = (DOT16_ARQ_RX_WINDOW_START) %
                         (Int16) DOT16_ARQ_WINDOW_SIZE;
              index != arqWindowStart % (Int16) DOT16_ARQ_WINDOW_SIZE;
              MacDot16ARQIncIndex(index))
         {
             MacDot16ARQSetARQBlockPointer(index);
             if (arqBlockPtr->arqBlockState != DOT16_ARQ_BLOCK_RECEIVED)
             {
                arqBlockPtr->arqBlockState = DOT16_ARQ_BLOCK_RECEIVED;
             }
         }
    }
    // Bug fix # 5434 end
    DOT16_ARQ_RX_WINDOW_START = arqWindowStart;
    // DOT16_ARQ_RX_WINDOW_START is pointing to expected BSN. If any missing
    // block is received then DOT16_ARQ_RX_WINDOW_START needs to be advanced
    // to BSN value which now becomes the new expected.
    Int32 index = 0;
    index = DOT16_ARQ_RX_WINDOW_START % DOT16_ARQ_WINDOW_SIZE;
    
    MacDot16ARQSetARQBlockPointer(index);
    Int32 tempIndex = index;
    if (tempIndex == 0)
    {
        tempIndex = DOT16_ARQ_WINDOW_SIZE;
    }
    else
    {
        MacDot16ARQDecIndex(tempIndex);
    }
    while (arqBlockPtr->arqBlockState == DOT16_ARQ_BLOCK_RECEIVED &&
           index != tempIndex)
    {
        DOT16_ARQ_RX_WINDOW_START++;
        MacDot16ARQIncIndex(index);
        MacDot16ARQSetARQBlockPointer(index);
    }
    if (DEBUG_ARQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Updated ARQ Window. \n");
        MacDot16ARQPrintControlBlock(node, sFlow);
    }
    return msg;
}//End Of MacDot16ARQAddToArray

// /**
// FUNCTION   :: MacDot16ARQHandleDataPdu
// LAYER      :: MAC
// PURPOSE    :: ARQ Handle data PDU
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + sFlow     : MacDot16ServiceFlow*: Pointer to service flow
// + ssInfo    : MacDot16BsSsInfo*: Pointer to SS Info
// + msg       : Message*: Msg pointer
// + pduLength : Int16: PDU length
// + isPackingHeaderPresent : BOOL: TRUE if packing header present
// RETURN     :: Message* : Return msg pointer
// **/
Message* MacDot16ARQHandleDataPdu(Node* node,
                              MacDataDot16* dot16,
                              MacDot16ServiceFlow* sFlow,
                              MacDot16BsSsInfo* ssInfo,
                              Message* msg,
                              int pduLength,
                              BOOL isPackingHeaderPresent)
{
    UInt8 fcVal =0 ;
    Int16 incomingBsnId = 0;
    UInt16 numARQBlocks = 0;
    BOOL isARQBlockPresent = FALSE;
    Message* retMsg = msg;

    if (DEBUG_ARQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Received ARQ Data PDU.sFlow = %d\n", sFlow->cid);
        MacDot16ARQPrintControlBlock(node, sFlow);
    }

    if (isPackingHeaderPresent)
    {
        UInt16 packedSDULength = 0;
        MacDot16ExtendedorARQEnablePackSubHeader* arqPackSubHeader = NULL;
        while (pduLength > 0 && msg != NULL)
        {
            arqPackSubHeader = (MacDot16ExtendedorARQEnablePackSubHeader*)
                MESSAGE_ReturnPacket(msg);
            //get fc val
            fcVal = MacDot16PackSubHeaderGetFC(arqPackSubHeader);
            //get bsn id
            incomingBsnId = MacDot16PackSubHeaderGet11bitBSN(
                arqPackSubHeader);
            packedSDULength =
                MacDot16ExtendedARQEnablePackSubHeaderGet11bitLength(
                arqPackSubHeader);
            pduLength -= packedSDULength;
            // remove fragmentation subheader
            MESSAGE_RemoveHeader(node,
                msg,
                sizeof(MacDot16ExtendedorARQEnablePackSubHeader),
                TRACE_DOT16);
            if (DEBUG_ARQ && DEBUG_PACKING_FRAGMENTATION)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("ARQ remove packing subheader\n");
            }
            pduLength -= sizeof(MacDot16ExtendedorARQEnablePackSubHeader);
            MacDot16ARQCalculateNumBlocks(numARQBlocks, packedSDULength);
            if (numARQBlocks > 0 && sFlow->arqControlBlock != NULL)
            {
                retMsg = MacDot16ARQAddToArray(node,
                    dot16,
                    msg,
                    sFlow,
                    incomingBsnId,
                    fcVal,
                    numARQBlocks);
                msg = retMsg;
                isARQBlockPresent = TRUE;
            }
            else
            {
                while (numARQBlocks > 0)
                {
                    retMsg = msg->next;
                    MESSAGE_Free(node, msg);
                    msg = retMsg;
                    numARQBlocks--;
                }
            }
        }
    }
    else
    {
        MacDot16ExtendedorARQEnableFragSubHeader* arqFragSubheader = NULL;
        arqFragSubheader = (MacDot16ExtendedorARQEnableFragSubHeader*)
            MESSAGE_ReturnPacket(msg);
        //get fc val
        fcVal = MacDot16FragSubHeaderGetFC(arqFragSubheader);
        //get bsn id
        incomingBsnId = MacDot16Get11bit(arqFragSubheader);
        // remove fragmentation subheader
        MESSAGE_RemoveHeader(node,
            msg,
            sizeof(MacDot16ExtendedorARQEnableFragSubHeader),
            TRACE_DOT16);
        pduLength -= sizeof(MacDot16ExtendedorARQEnableFragSubHeader);
        MacDot16ARQCalculateNumBlocks(numARQBlocks, (unsigned int)pduLength);
        if (numARQBlocks > 0 && sFlow->arqControlBlock != NULL)
        {
            retMsg = MacDot16ARQAddToArray(node,
                dot16,
                msg,
                sFlow,
                incomingBsnId,
                fcVal,
                numARQBlocks);
            isARQBlockPresent = TRUE;
        }
        else
        {
            while (numARQBlocks > 0)
            {
                retMsg = msg->next;
                MESSAGE_Free(node, msg);
                msg = retMsg;
                numARQBlocks--;
            }
        }
    }

    //Handling ARQ BLock Reception

    if (isARQBlockPresent == TRUE)
    {
        //We will send a collective Feedback
        MacDot16BuildAndSendARQFeedback(node,
            dot16,
            sFlow,
            ssInfo);
        MacDot16ARQBuildSDU(node,
            dot16,
            sFlow,
            ssInfo);
    }
    return retMsg;
}
// /**
// FUNCTION   :: MacDot16BuildAndSendARQFeedback
// LAYER      :: MAC
// PURPOSE    :: ARQ build and send feedback msg
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + sFlow     : MacDot16ServiceFlow*: Pointer to service flow
// + ssInfo    : MacDot16BsSsInfo*: Pointer to SS Info
// RETURN     :: void : NULL
// **/
void MacDot16BuildAndSendARQFeedback(Node *node,
                                     MacDataDot16* dot16,
                                     MacDot16ServiceFlow* sFlow,
                                     MacDot16BsSsInfo* ssInfo)
{
    int freeWindowSpace = 0;
    MacDot16ARQFeedbackMsg* arqFeedBack = NULL;
    MacDot16MacHeader* macHeader;
    Message* mgmtMsg;
    unsigned char payload[DOT16_MAX_MGMT_MSG_SIZE];
    memset(payload, 0, DOT16_MAX_MGMT_MSG_SIZE);
    int index = 0;

    MacDot16ARQWindowFreeSpace(freeWindowSpace,
                                     DOT16_ARQ_WINDOW_FRONT,
                                     DOT16_ARQ_WINDOW_REAR,
                                     DOT16_ARQ_WINDOW_SIZE);

    if (freeWindowSpace == (int)DOT16_ARQ_WINDOW_SIZE)
    {
        //No elements in queue
    }
    else
    {
        //We need to build a Feedback packet.
        arqFeedBack =(MacDot16ARQFeedbackMsg*)
                            MEM_malloc(sizeof (MacDot16ARQFeedbackMsg));
        memset(arqFeedBack,0,sizeof (MacDot16ARQFeedbackMsg));
        arqFeedBack->type = DOT16_ARQ_FEEDBACK ;
        MacDot16ARQFeedbackIESetAckType(arqFeedBack,
                                        DOT16_ARQ_CUMULATIVE_ACK_TYPE);

        UInt16 tempBsn = DOT16_ARQ_RX_WINDOW_START ;
        MacDot16ARQDecBsnId(tempBsn);
        MacDot16ARQFeedbackIESetBSN(arqFeedBack,tempBsn);
        MacDot16ARQFeedbackIESetCID(arqFeedBack,sFlow->cid);
        MacDot16ARQFeedbackIESetLast(arqFeedBack,1);
        MacDot16ARQFeedbackIESetNumAckMaps(arqFeedBack,1);

        //now we need to send this as a management packet.
        macHeader = (MacDot16MacHeader*) &(payload[index]);
        index += sizeof(MacDot16MacHeader);
        memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));

        if (dot16->stationType == DOT16_BS)
        {
            MacDot16MacHeaderSetCID(macHeader,ssInfo->basicCid );
        }
        else
        {
            MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
            MacDot16MacHeaderSetCID(macHeader,
                                        dot16Ss->servingBs->basicCid);
        }
        //Setting the type of the ARQ Feedback Management message

        payload[index ++] = arqFeedBack->type;
        payload[index ++]= arqFeedBack->byte1 ;
        payload[index ++]= arqFeedBack->byte2 ;
        payload[index ++]= arqFeedBack->byte3 ;
        payload[index ++]= arqFeedBack->byte4 ;

        MacDot16MacHeaderSetLEN(macHeader, index);
        mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
        ERROR_Assert(mgmtMsg != NULL, "MAC802.16: Out of memory!");

        MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
        memcpy(MESSAGE_ReturnPacket(mgmtMsg), payload, index);

        if (DEBUG_ARQ)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("Sending ARQ Feed Back Message.");
            printf("sflow cid = %d ,BsnId (Cumulative Ack) = %d"
                    "\n",sFlow->cid,tempBsn);
            //MacDot16ARQPrintControlBlock(sFlow);
        }

        if (dot16->stationType == DOT16_BS)
         {
              MacDot16BsScheduleMgmtMsgToSs(node,
                                            dot16,
                                            ssInfo,
                                            ssInfo->basicCid,
                                            mgmtMsg);
          }
         else
         {
              MacDot16SsEnqueueMgmtMsg(node,
                                       dot16,
                                       DOT16_CID_BASIC,
                                       mgmtMsg);
         }
         MEM_free(arqFeedBack);
    }// end of else
}//end of function.

// /**
// FUNCTION   :: MacDot16ARQHandleDiscardMsg
// LAYER      :: MAC
// PURPOSE    :: ARQ handle discard msg
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + macFrame     : unsigned char*: Pointer to an unsigned character
// + startIndex    : int: starting location
// RETURN     :: void : NULL
// **/
int MacDot16ARQHandleDiscardMsg(Node* node,
                                MacDataDot16* dot16,
                                unsigned char* macFrame,
                                int startIndex)
{
    BOOL isPresent = FALSE;
    UInt16 incomingBsnId = 0;
    int index = startIndex;
    int bsnIndex ;
    int tempARQIndex ;
    UInt16 arqWindowStart ;
    Dot16CIDType cid ;
    MacDot16ARQDiscardMsg* arqDiscardMsg;
    MacDot16ARQBlock* arqBlockPtr = NULL;
    MacDot16BsSsInfo* ssInfo = NULL;
    MacDot16ServiceFlow* sFlow = NULL;
    UInt16 numUnACKArqBloacks = 0;
    int numARQBlocksDiscarded = 0;
    UInt16 startBsnId;

    //MacDot16MacHeader* macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    arqDiscardMsg = (MacDot16ARQDiscardMsg*)&(macFrame[index]);
    index += sizeof(MacDot16ARQDiscardMsg);
    cid = MacDot16ARQDiscardMsgGetCID(arqDiscardMsg);
    incomingBsnId = MacDot16ARQDiscardMsgGetBSN(arqDiscardMsg);

    if (dot16->stationType == DOT16_BS)
    {
        MacDot16BsGetServiceFlowByCid(node,
            dot16,
            cid,
            &ssInfo,
            &sFlow);
    }
    else
    {
        sFlow =MacDot16SsGetServiceFlowByCid(node,
            dot16,
            cid);
    }
    // Bug fix # 5434 start
    // if the control block has not been initialized because of non-receipt
    // of ,amagement DSA ACK or RSP
    if (sFlow == NULL || sFlow->activated == FALSE ||
        sFlow->admitted == FALSE || sFlow->arqControlBlock == NULL)
    {
        return (index - startIndex);
    }
    // Bug fix # 5434 end
    if (DEBUG_ARQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Handling arq discard message ");
        printf("sflow cid = %d ,BsnId  = %d"
                "\n",sFlow->cid,incomingBsnId);
        MacDot16ARQPrintControlBlock(node, sFlow);
    }

    if (DOT16_ARQ_WINDOW_FRONT==DOT16_ARQ_WINDOW_REAR)//Window is empty
    {
        startBsnId = DOT16_ARQ_RX_WINDOW_START ;
    }
    else
    {
        MacDot16ARQSetARQBlockPointer(DOT16_ARQ_WINDOW_FRONT);
        startBsnId = MacDot16Get11bit(DOT16_ARQ_FRAG_SUBHEADER);
    }

    //Build SDU if possible else discard the incomplete messages.
    MacDot16ARQBuildSDU(node,
                dot16,
                sFlow,
                ssInfo);

    bsnIndex = MacDot16ARQFindBSNInWindow(sFlow,
        incomingBsnId,
        1,// check for single ARQ block
        isPresent,
        numUnACKArqBloacks);


    if (!isPresent)
    {
        if (DEBUG_ARQ)
        {
            printf("ARQ Block not in window.\n");
        }
        //ARQ block is not in window.ie it has been acknowledged.
       // so nothing needs to be done
        return (index - startIndex);
    }
    else if (MacDot16ARQNormalizedBsn(incomingBsnId, startBsnId)<
        MacDot16ARQNormalizedBsn(DOT16_ARQ_RX_WINDOW_START, startBsnId))
    {
        if (DEBUG_ARQ)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("ARQ Blocks have already been received\n");
        }
        // so nothing needs to be done
        return (index - startIndex);
    }

    //else  we need to mark the blocks till the incomingBsnId as received
    //and shift the window. ARQ_RX_WINDOW_START
    MacDot16ARQIncIndex(bsnIndex);
    arqWindowStart = DOT16_ARQ_RX_WINDOW_START;
    //Starting from rear as Blocls till rear have already been received
     // Bug fix # 5434 start
    // AS per section 6.3.4.3 blocks need to be marked from
    // old ARQ_RX_WINDOW start to new
    // ARQ_RX_WINDOW_START = bsnIndex
    int oldArqWindowStartIndex = (DOT16_ARQ_RX_WINDOW_START) %
                                  (Int16) DOT16_ARQ_WINDOW_SIZE;
    DOT16_ARQ_RX_WINDOW_START = incomingBsnId + 1;
    int newArqWindowStartIndex = (bsnIndex) %
                                  (Int16) DOT16_ARQ_WINDOW_SIZE;
    for (tempARQIndex = oldArqWindowStartIndex;
        tempARQIndex != newArqWindowStartIndex;
                                        MacDot16ARQIncIndex(tempARQIndex))
    {
        MacDot16ARQSetARQBlockPointer(tempARQIndex);
        numARQBlocksDiscarded++;
        arqBlockPtr->arqBlockState = DOT16_ARQ_BLOCK_RECEIVED;
        MacDot16ARQIncBsnId(arqWindowStart);
    }
     // Bug fix # 5434 end
    DOT16_ARQ_WINDOW_REAR = (UInt16)bsnIndex;
     // Bug fix # 5434 start
    if (MacDot16ARQNormalizedBsn(incomingBsnId, startBsnId) >=
        MacDot16ARQNormalizedBsn(DOT16_ARQ_RX_HIGHEST_BSN, startBsnId))
    {
        DOT16_ARQ_RX_HIGHEST_BSN = DOT16_ARQ_RX_WINDOW_START;
    }
     // Bug fix # 5434 end
    if (DEBUG_ARQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("arq discard message processed.\n");
        MacDot16ARQPrintControlBlock(node, sFlow);
    }
    // We need to send a feedback message.
    MacDot16BuildAndSendARQFeedback(node,
                                    dot16,
                                    sFlow,
                                    ssInfo);
     // Bug fix # 5434 start
    MacDot16ARQBuildSDU(node,
                dot16,
                sFlow,
                ssInfo);
     // Bug fix # 5434 end
    return (index - startIndex) ;
}//end of function MacDot16ARQHandleDiscardMsg().

// /**
// FUNCTION   :: MacDot16ARQHandleFeedback
// LAYER      :: MAC
// PURPOSE    :: ARQ handle feedback msg
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + macFrame     : unsigned char*: Pointer to an unsigned character
// + startIndex    : int: starting location
// RETURN     :: int :
// **/
int MacDot16ARQHandleFeedback (Node* node,
                               MacDataDot16* dot16,
                               unsigned char* macFrame,
                               int startIndex)
{
    BOOL shiftWindow = FALSE ;
    Dot16CIDType cid ;
    UInt8 lastBit;
    UInt8 numAckMaps;
    UInt8 ackType ;
    UInt16 incomingBsnId = 0;
    int i ;
    MacDot16ARQFeedbackMsg* arqFeedBack ;
    Int16 oldWindowFront = 0 ;
    Int16 oldWindowStart = 0 ;
    MacDot16ARQBlock* arqBlockPtr = NULL ;
    MacDot16BsSsInfo* ssInfo;
    MacDot16ServiceFlow* sFlow;
    MacDot16MacHeader* macHeader;


    Int16 index = (Int16)startIndex;
    macHeader = (MacDot16MacHeader*) &(macFrame[index]);
    index += sizeof(MacDot16MacHeader);

    arqFeedBack = (MacDot16ARQFeedbackMsg*) &(macFrame[index]);
    index += sizeof(MacDot16ARQFeedbackMsg);
    cid = MacDot16ARQFeedbackIEGetCID(arqFeedBack);
    lastBit = MacDot16ARQFeedbackIEGetLast(arqFeedBack);
    ackType = MacDot16ARQFeedbackIEGetAckType(arqFeedBack);
    incomingBsnId = MacDot16ARQFeedbackIEGetBSN(arqFeedBack);
    numAckMaps = MacDot16ARQFeedbackIEGetNumAckMaps(arqFeedBack);

        if (dot16->stationType == DOT16_BS)
        {
            MacDot16BsGetServiceFlowByCid(
                                          node,
                                          dot16,
                                          cid,
                                          &ssInfo,
                                          &sFlow);
        }
        else
        {
            sFlow =MacDot16SsGetServiceFlowByCid(
                                                 node,
                                                 dot16,
                                                 cid);
        }

        if (sFlow == NULL)
        {
            ERROR_ReportWarning("no sflow associated with the ARQ feedback"
                                "msg");
           return (index - startIndex) ;
        }

        if (DEBUG_ARQ)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("Handling arq feedback message ");
            printf("sflow cid = %d ,BsnId received(Cumulative Ack) = %d"
                    "\n",sFlow->cid,incomingBsnId);
            MacDot16ARQPrintControlBlock(node, sFlow);
        }

        oldWindowFront = DOT16_ARQ_WINDOW_FRONT ;
        oldWindowStart = DOT16_ARQ_TX_WINDOW_START ;

        //check the validity of the incoming BSN
        //it should lie between
        //DOT16_ARQ_TX_WINDOW_START and DOT16_ARQ_TX_NEXT_BSN -1 (inclusive)

        if (
           MacDot16ARQNormalizedBsn(incomingBsnId,DOT16_ARQ_TX_WINDOW_START)<
            MacDot16ARQNormalizedBsn((DOT16_ARQ_TX_NEXT_BSN ),
                                                DOT16_ARQ_TX_WINDOW_START))
         {
            DOT16_ARQ_TX_WINDOW_START =
                            MacDot16ARQIncBsnId(incomingBsnId);

            DOT16_ARQ_WINDOW_FRONT = (oldWindowFront +
                ((DOT16_ARQ_TX_WINDOW_START - oldWindowStart +
                    DOT16_ARQ_BSN_MODULUS)% DOT16_ARQ_BSN_MODULUS))%
                (Int16)DOT16_ARQ_WINDOW_SIZE;
            shiftWindow = TRUE ;
         }

        //we need to discard the Blocks which have been acknowledged.
        //also we need to reset the ARQ SYNC LOSS Timer for the Service Flow.
        if (shiftWindow)
        {
            for (i = oldWindowFront ;i!=DOT16_ARQ_WINDOW_FRONT;
                                MacDot16ARQIncIndex(i))
             {
                MacDot16ARQSetARQBlockPointer(i);
                MacDot16ResetARQBlockPtr(node, arqBlockPtr, TRUE);
             }
            if (sFlow->arqSyncLossTimer != NULL)
            {
                MESSAGE_CancelSelfMsg(node, sFlow->arqSyncLossTimer);
                sFlow->arqSyncLossTimer = NULL;
            }
            if (DOT16_ARQ_SYNC_LOSS_TIMEOUT > 0)
            {
                sFlow->arqSyncLossTimer = MacDot16SetTimer(node,
                    dot16,
                    DOT16_ARQ_SYNC_LOSS_TIMER,
                    DOT16_ARQ_SYNC_LOSS_TIMEOUT,
                    NULL,
                    sFlow->cid);
            }
        }//end of if (shiftWindow)

        if (DEBUG_ARQ)
        {
            MacDot16PrintRunTimeInfo(node, dot16);
            printf("arq feedback message processed.\n");
            MacDot16ARQPrintControlBlock(node, sFlow);
        }

   return (index - startIndex) ;
}//end of MacDot16ARQHandleFeedback.

// /**
// FUNCTION   :: MacDot16ARQBuildAndSendDiscardMsg
// LAYER      :: MAC
// PURPOSE    :: ARQ build and send discard msg
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + sFlow     : MacDot16ServiceFlow*: Pointer to service flow
// + ssInfo    : MacDot16BsSsInfo*: Pointer to SS Info
// + cid       : Dot16CIDType: CID type
// + bsnId     : UInt16: BSN Id
// RETURN     :: int :
// **/
void MacDot16ARQBuildAndSendDiscardMsg(Node *node,
                                     MacDataDot16* dot16,
                                     MacDot16ServiceFlow* sFlow,
                                     MacDot16BsSsInfo* ssInfo,
                                     Dot16CIDType cid,
                                     UInt16 bsnId)
{
    //Build discard Message
    int index = 0;
    Message* mgmtMsg;
    MacDot16ARQDiscardMsg* arqDiscardMsg = NULL;
    MacDot16MacHeader* macHeader;
    unsigned char payload[DOT16_MAX_MGMT_MSG_SIZE];

    if (DEBUG_ARQ)
    {
        MacDot16PrintRunTimeInfo(node, dot16);
        printf("Sending Discard Message.");
        printf("sflow cid = %d ,BsnId (Cumulative) = %d"
                "\n",sFlow->cid, bsnId);
    }

    memset(payload, 0, DOT16_MAX_MGMT_MSG_SIZE);
    arqDiscardMsg = (MacDot16ARQDiscardMsg*)
                    MEM_malloc(sizeof (MacDot16ARQDiscardMsg));
    ERROR_Assert(arqDiscardMsg != NULL, "MAC802.16: Out of memory!");
    memset(arqDiscardMsg, 0, sizeof(MacDot16ARQDiscardMsg));
    arqDiscardMsg->type = DOT16_ARQ_DISCARD ;
    MacDot16ARQDiscardMsgSetCID(arqDiscardMsg, cid);
    MacDot16ARQDiscardMsgSetBSN(arqDiscardMsg, bsnId);

    //now we need to send this as a management packet.
    macHeader = (MacDot16MacHeader*) &(payload[index]);
    index += sizeof(MacDot16MacHeader);
    memset((char*) macHeader, 0, sizeof(MacDot16MacHeader));

    if (dot16->stationType == DOT16_BS)
    {
        MacDot16MacHeaderSetCID(macHeader, ssInfo->basicCid );
    }
    else
    {
        MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
        MacDot16MacHeaderSetCID(macHeader, dot16Ss->servingBs->basicCid);
    }

    payload[index++] = arqDiscardMsg->type;
    payload[index++] = arqDiscardMsg->byte1 ;
    payload[index++] = arqDiscardMsg->byte2 ;
    payload[index++] = arqDiscardMsg->byte3 ;
    payload[index++] = arqDiscardMsg->byte4 ;

    MacDot16MacHeaderSetLEN(macHeader, index);
    mgmtMsg = MESSAGE_Alloc(node, 0, 0, 0);
    ERROR_Assert(mgmtMsg != NULL, "MAC802.16: Out of memory!");
    MESSAGE_PacketAlloc(node, mgmtMsg, index, TRACE_DOT16);
    memcpy(MESSAGE_ReturnPacket(mgmtMsg), payload, index);

    if (dot16->stationType == DOT16_BS)
     {
          MacDot16BsScheduleMgmtMsgToSs(node,
                                        dot16,
                                        ssInfo,
                                        ssInfo->basicCid,
                                        mgmtMsg);
      }
     else
     {
          MacDot16SsEnqueueMgmtMsg(node,
                                   dot16,
                                   DOT16_CID_BASIC,
                                   mgmtMsg);
     }
     MEM_free(arqDiscardMsg);
     return;
}//end of function MacDot16ARQBuildAndSendDiscardMsg

// /**
// FUNCTION   :: MacDot16BsARQCheckIfServiceTypePacketInSflow
// LAYER      :: MAC
// PURPOSE    :: check all the arq enabled service flows
//               which have the same service type as "serviceType"
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + numFreePs : int&: num of free Ps
// + serviceType  : MacDot16ServiceType: Service type
// RETURN     :: Void : NULL
// **/
static
void MacDot16BsARQCheckIfServiceTypePacketInSflow(
    Node* node,
    MacDataDot16* dot16,
    int& numFreePs,
    MacDot16ServiceType serviceType)
{
//For this particular node, check all the arq enabled service flows
//which have the same service type as "serviceType". If there is any packet
//to be sent in that flow,Call CreatePDU function.
    UInt16 sFlowBwReq = 0;
    MacDot16ServiceFlow* sFlow =  NULL ;
    UInt16 crcSize = 0;
    MacDot16BsSsInfo* ssInfo;
    int numFreeBytes;
    MacDot16DlBurstProfile* dlBurstProfile;

    if (dot16->stationType == DOT16_BS)
    {
        int i;
        // this is a base station
        MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
        if (dot16Bs->isCRCEnabled)
        {
            crcSize = DOT16_CRC_SIZE;
        }
        for (i = 0; i < DOT16_BS_SS_HASH_SIZE; i++)
        {
            ssInfo = dot16Bs->ssHash[i];
            while (ssInfo != NULL)
            {
                sFlow = ssInfo->dlFlowList[serviceType].flowHead;
                dlBurstProfile = &(dot16Bs->dlBurstProfile[ssInfo->diuc]);
                numFreeBytes = numFreePs * (
                        MacDot16PhyBitsPerPs(node,
                                             dot16,
                                             dlBurstProfile,
                                             DOT16_DL)
                                             / 8);
                if (sFlow != NULL)
                {
                    if (sFlow->isARQEnabled)
                    {
                        sFlowBwReq = (UInt16)MacDot16ARQCalculateBwReq(sFlow,
                            crcSize);
                        if ((sFlowBwReq > 0) && (numFreeBytes > 0))
                        {
                            while (numFreeBytes > (int)sizeof(MacDot16MacHeader))
                            {
                                int tempFreeBytes = numFreeBytes;
                                MacDot16ARQCreatePDU(node,
                                                     dot16,
                                                     sFlow,
                                                     ssInfo,
                                                     numFreeBytes);
                                if (tempFreeBytes == numFreeBytes)
                                {
                                    break;
                                }
                            }// end of while
                        }
                    }
                    sFlow = sFlow->next;
                }
                numFreePs = MacDot16PhyBytesToPs(node,
                                                 dot16,
                                                 numFreeBytes,
                                                 dlBurstProfile,
                                                 DOT16_DL);
                ssInfo = ssInfo->next;
            }
        }
    }
} // end of function MacDot16BsARQCheckIfServiceTypePacketInSflow
// /**
// FUNCTION   :: MacDot16SsARQCheckIfServiceTypePacketInSflow
// LAYER      :: MAC
// PURPOSE    :: check all the arq enabled service flows
//               which have the same service type as "serviceType"
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + numFreeBytes : int&: num of free bytes
// + serviceType  : MacDot16ServiceType: Service type
// RETURN     :: Void : NULL
// **/
static
void MacDot16SsARQCheckIfServiceTypePacketInSflow(
    Node* node,
    MacDataDot16* dot16,
    int& numFreeBytes,
    MacDot16ServiceType serviceType)
{
    UInt16 sFlowBwReq = 0;
    MacDot16ServiceFlow* sFlow =  NULL ;
    UInt16 crcSize = 0;

    if (dot16->stationType == DOT16_SS)
    {
        // this is a subscriber station
        MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;
        sFlow = dot16Ss->ulFlowList[serviceType].flowHead;
        if (dot16Ss->isCRCEnabled)
        {
            crcSize = DOT16_CRC_SIZE;
        }
        while (sFlow != NULL)
        {
            if (sFlow->isARQEnabled && sFlow->serviceType == serviceType)
            {
                sFlowBwReq = (UInt16)MacDot16ARQCalculateBwReq(sFlow,
                    crcSize);
                if ((sFlowBwReq > 0) && (numFreeBytes > 0))
                {
                    while (numFreeBytes > (int)sizeof(MacDot16MacHeader))
                    {
                        int tempFreeBytes = numFreeBytes;
                        MacDot16ARQCreatePDU(node,
                                             dot16,
                                             sFlow,
                                             NULL,
                                             numFreeBytes);
                        if (tempFreeBytes == numFreeBytes)
                        {
                            break;
                        }
                    }// end of while
                }
            }
            sFlow = sFlow->next;
        }
    }
}// end of function MacDot16SsARQCheckIfServiceTypePacketInSflow


// /**
// FUNCTION   :: MacDot16BuildPDUDataPacket
// LAYER      :: MAC
// PURPOSE    :: Node will call this function to pack the data packets.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + sFlow     : MacDot16ServiceFlow* : Pointer to service flow
// + scheduler : Scheduler*    : Pointer to scheduler, from where data
//                               packets need to retrieved
// + msgIn     : Message* : Incoming msg
// + pduSize   : int&     : Pdu size
// + numFreeBytes : int   : no of free bytes # of bytes
// RETURN     :: Message* : Return the MAC PDU
// **/
static
Message* MacDot16BuildPDUDataPacket(
    Node* node,
    MacDataDot16* dot16,
    MacDot16ServiceFlow* sFlow,
    Scheduler* scheduler,
    int& pduSize,
    int recvNumFreeBytes)
{
    Message* msgIn = NULL;
    MacDot16Ss* dot16Ss = NULL;
    MacDot16Bs* dot16Bs = NULL;
    Message* msgOut = NULL;
    Message* tempMsgIn = NULL;
    unsigned char* payload;
    MacDot16NotExtendedARQDisablePackSubHeader* subHeader;
    int payloadLen;
    int tempPDUSize;
    UInt16 bytesSent;
    int priority;
    int numFreeBytes = recvNumFreeBytes;
    UInt8 storedFC[DOT16_MAX_PDU_SIZE / 3] = {0, 0};
    int storedFSN[DOT16_MAX_PDU_SIZE / 3] = {0, 0};
    clocktype currentTime = node->getNodeTime();
    BOOL notEmpty = FALSE;
    BOOL isFragEnabled = FALSE;
    BOOL isCRCEnabled = FALSE;
    int numOfPackedPacket = 0;
    BOOL isPackingEnabled = sFlow->isPackingEnabled;
    int variableHeaderSize =
        sizeof(MacDot16NotExtendedARQDisablePackSubHeader);

    if (dot16->stationType == DOT16_SS)
    {
        dot16Ss = (MacDot16Ss*) dot16->ssData;
        isFragEnabled = dot16Ss->isFragEnabled;
        isCRCEnabled = dot16Ss->isCRCEnabled;
    }
    else
    {
        dot16Bs = (MacDot16Bs*) dot16->bsData;
        isFragEnabled = dot16Bs->isFragEnabled;
        isCRCEnabled = dot16Bs->isCRCEnabled;
    }

    if (sFlow->needPiggyBackBwReq)
    {
        pduSize += sizeof(MacDot16DataGrantSubHeader);
    }

    if ((isPackingEnabled || (isFragEnabled &&
        sFlow->isFragStart == TRUE))
        &&
        numFreeBytes > DOT16_MAX_PDU_SIZE)
    {
        // adjust the num of free bytes as per the pdu size
        numFreeBytes = DOT16_MAX_PDU_SIZE;
    }

    if (pduSize > numFreeBytes && (isFragEnabled &&
        sFlow->isFragStart == FALSE))
    {
        numFreeBytes = recvNumFreeBytes;
        isPackingEnabled = FALSE;
    }

    if (isPackingEnabled && isFragEnabled && sFlow->isFragStart == TRUE)
    {
        pduSize -= variableHeaderSize;
    }


    tempPDUSize = pduSize;
    while ((tempPDUSize + (numOfPackedPacket* variableHeaderSize))
        <= numFreeBytes)
    {
        if ((numOfPackedPacket > 0) &&
            ((tempPDUSize + (numOfPackedPacket + 1)* variableHeaderSize)
            > numFreeBytes))
        {
            break;
        }
        // Dequeue the packet from queue
        scheduler->retrieve(sFlow->queuePriority,
            0,
            &msgOut,
            &priority,
            DEQUEUE_PACKET,
            currentTime);
        payload = (unsigned char*) MESSAGE_ReturnPacket(msgOut);
        payloadLen = MESSAGE_ReturnPacketSize(msgOut);
        pduSize = tempPDUSize;
        MESSAGE_RemoveInfo(node, msgOut, INFO_TYPE_Dot16BurstInfo);
        if (dot16->stationType == DOT16_SS)
        {
            dot16Ss->stats.numPktsToLower ++;
            // update dynamic stats
            if (node->guiOption)
            {
                GUI_SendIntegerData(
                     node->nodeId,
                     dot16Ss->stats.numPktToPhyGuiId,
                     dot16Ss->stats.numPktsToLower,
                     node->getNodeTime());
            }
        }
        else
        {
            dot16Bs->stats.numPktsToLower ++;
            // update dynamic stats
            if (node->guiOption)
            {
                GUI_SendIntegerData(
                     node->nodeId,
                     dot16Bs->stats.numPktToPhyGuiId,
                     dot16Bs->stats.numPktsToLower,
                     node->getNodeTime());
            }
        }

        if (isPackingEnabled)
        {
            if (sFlow->isFragStart)
            {
                sFlow->isFragStart = FALSE;
                bytesSent = sFlow->bytesSent;
                if (bytesSent > MESSAGE_ReturnActualPacketSize(msgOut))
                {
                    int temp = bytesSent -
                        MESSAGE_ReturnActualPacketSize(msgOut);
                    MESSAGE_ShrinkPacket(node, msgOut,
                        MESSAGE_ReturnActualPacketSize(msgOut));
                    MESSAGE_RemoveVirtualPayload(node, msgOut, temp);
                }
                else
                {
                    MESSAGE_ShrinkPacket(node, msgOut, bytesSent);
                }

                sFlow->bytesSent = 0;
                storedFC[numOfPackedPacket] = DOT16_LAST_FRAGMENT;
                storedFSN[numOfPackedPacket] = sFlow->fragFSNno;
                sFlow->fragFSNno++;
                if (sFlow->fragFSNno >= DOT16_MAX_NO_FRAGMENTED_PACKET)
                {
                    sFlow->fragFSNno = sFlow->fragFSNno
                        % DOT16_MAX_NO_FRAGMENTED_PACKET;
                }
                if (dot16->stationType == DOT16_SS)
                {
                    dot16Ss->stats.numFragmentsSent++;
                }
                else
                {
                    dot16Bs->stats.numFragmentsSent++;
                }
            }
            else
            {
                storedFC[numOfPackedPacket] = DOT16_NO_FRAGMENTATION;
                sFlow->fragFSNno++;
                if (sFlow->fragFSNno >= DOT16_MAX_NO_FRAGMENTED_PACKET)
                {
                    sFlow->fragFSNno = sFlow->fragFSNno
                        % DOT16_MAX_NO_FRAGMENTED_PACKET;
                }
            }
            tempMsgIn = msgIn;
            if (tempMsgIn == NULL)
            {
                tempMsgIn = msgOut;
                msgIn = tempMsgIn;
            }
            else
            {
                while (tempMsgIn->next != NULL)
                {
                    tempMsgIn = tempMsgIn->next;
                }
                tempMsgIn->next = msgOut;
            }
            msgOut->next = NULL;
            numOfPackedPacket++;
        }
        else
        {
            if ((isFragEnabled == TRUE) && (sFlow->isFragStart == TRUE)
                && (numFreeBytes >= pduSize))
            {
                MacDot16NotExtendedARQDisableFragSubHeader* fragSubHeader;
                MacDot16MacHeader* macHeader;
                sFlow->isFragStart = FALSE;
                bytesSent = sFlow->bytesSent;
                sFlow->bytesSent = 0;
                if (bytesSent > MESSAGE_ReturnActualPacketSize(msgOut))
                {
                    int temp = bytesSent -
                        MESSAGE_ReturnActualPacketSize(msgOut);
                    MESSAGE_ShrinkPacket(node, msgOut,
                        MESSAGE_ReturnActualPacketSize(msgOut));
                    MESSAGE_RemoveVirtualPayload(node, msgOut, temp);
                }
                else
                {
                    MESSAGE_ShrinkPacket(node, msgOut, bytesSent);
                }

                // Add Fragmentation header
                MESSAGE_AddHeader(
                    node,
                    msgOut,
                    sizeof(MacDot16NotExtendedARQDisableFragSubHeader),
                    TRACE_DOT16);
                if (DEBUG_PACKING_FRAGMENTATION)
                {
                    MacDot16PrintRunTimeInfo(node, dot16);
                    printf("Add last fragmented subheader\n");
                }
                fragSubHeader = (MacDot16NotExtendedARQDisableFragSubHeader*)
                    MESSAGE_ReturnPacket(msgOut);
                memset(
                    fragSubHeader,
                    0,
                    sizeof(MacDot16NotExtendedARQDisableFragSubHeader));
                MacDot16FragSubHeaderSet3bitFSN(
                    fragSubHeader,
                    sFlow->fragFSNno);
                sFlow->fragFSNno++;
                if (sFlow->fragFSNno >= DOT16_MAX_NO_FRAGMENTED_PACKET)
                {
                    sFlow->fragFSNno = sFlow->fragFSNno
                        % DOT16_MAX_NO_FRAGMENTED_PACKET;
                }

                MacDot16FragSubHeaderSetFC(fragSubHeader, DOT16_LAST_FRAGMENT);
                msgOut = MacDot16AddDataGrantMgmtandGenericMacSubHeader(
                    node,
                    dot16,
                    sFlow,
                    msgOut,
                    pduSize);
                macHeader =
                    (MacDot16MacHeader*) MESSAGE_ReturnPacket(msgOut);
                MacDot16MacHeaderSetGeneralType(macHeader,
                    DOT16_FRAGMENTATION_SUBHEADER);
                if (dot16->stationType == DOT16_SS)
                {
                    dot16Ss->stats.numFragmentsSent++;
                }
                else
                {
                    dot16Bs->stats.numFragmentsSent++;
                }
                return msgOut;
            }
            msgIn = msgOut;
            msgOut->next = NULL;
            break;
        }
        // peek next packet

        notEmpty = scheduler->retrieve(sFlow->queuePriority,
                                       0,
                                       &msgOut,
                                       &priority,
                                       PEEK_AT_NEXT_PACKET,
                                       currentTime);

        if (notEmpty)
        {
            payload = (unsigned char*) MESSAGE_ReturnPacket(msgOut);
            payloadLen = MESSAGE_ReturnPacketSize(msgOut);
            tempPDUSize += payloadLen;
            if (((tempPDUSize + (numOfPackedPacket + 1) * variableHeaderSize)
                 > numFreeBytes)
                && (isFragEnabled == TRUE)
                && (!sFlow->isFixedLengthSDU))
            {
                Message* dupMsgOut = MESSAGE_Duplicate(node, msgOut);
                MESSAGE_RemoveInfo(node, dupMsgOut, INFO_TYPE_Dot16BurstInfo);
                MESSAGE_RemoveVirtualPayload(node, dupMsgOut,
                    MESSAGE_ReturnVirtualPacketSize(msgOut));

                payloadLen = numFreeBytes -
                    (pduSize + (numOfPackedPacket * variableHeaderSize))
                    - sizeof(MacDot16NotExtendedARQDisablePackSubHeader);
                if (payloadLen <= 0)
                {
                    MESSAGE_Free(node, dupMsgOut);
                    break;
                }

                sFlow->isFragStart = TRUE;
                storedFC[numOfPackedPacket] = DOT16_FIRST_FRAGMENT;
                storedFSN[numOfPackedPacket] = sFlow->fragFSNno;
                sFlow->fragFSNno++;
                if (sFlow->fragFSNno >= DOT16_MAX_NO_FRAGMENTED_PACKET)
                {
                    sFlow->fragFSNno = sFlow->fragFSNno
                        % DOT16_MAX_NO_FRAGMENTED_PACKET;
                }

                sFlow->bytesSent = (UInt16)payloadLen;
                if (payloadLen > MESSAGE_ReturnActualPacketSize(msgOut))
                {
                    int temp = payloadLen -
                        MESSAGE_ReturnActualPacketSize(msgOut);
                    MESSAGE_AddVirtualPayload(node, dupMsgOut, temp);
                }
                else
                {
                    dupMsgOut->packetSize = payloadLen;
                }
                pduSize += MESSAGE_ReturnPacketSize(dupMsgOut);
                tempMsgIn = msgIn;
                if (tempMsgIn == NULL)
                {
                    tempMsgIn = dupMsgOut;
                    msgIn = dupMsgOut;
                }
                else
                {
                    while (tempMsgIn->next !=NULL)
                    {
                        tempMsgIn = tempMsgIn->next;
                    }
                    tempMsgIn->next = dupMsgOut;
                }
                numOfPackedPacket++;

                if (dot16->stationType == DOT16_SS)
                {
                    dot16Ss->stats.numFragmentsSent++;
                }
                else
                {
                    dot16Bs->stats.numFragmentsSent++;
                }

                break;
            }
        }
        else
        {
            break;
        }// end of if-else "notEmpty"
    }// end of while

    // add header

    tempMsgIn = msgIn;
    if ((numOfPackedPacket > 1 || storedFC[0] == DOT16_LAST_FRAGMENT)
        && (tempMsgIn != NULL
        && !sFlow->isFixedLengthSDU))
    {
        for (int i = 0; i < numOfPackedPacket; i++)
        {
            if (DEBUG_PACKING_FRAGMENTATION)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("Add Packing Subheader\n");
            }
            payloadLen = MESSAGE_ReturnPacketSize(tempMsgIn);
            // Add variable header
            MESSAGE_AddHeader(
                node,
                tempMsgIn,
                sizeof(MacDot16NotExtendedARQDisablePackSubHeader),
                TRACE_DOT16);
            subHeader = (MacDot16NotExtendedARQDisablePackSubHeader*)
                MESSAGE_ReturnPacket(tempMsgIn);
            memset(subHeader,
                0,
                sizeof(MacDot16NotExtendedARQDisablePackSubHeader));
            MacDot16FragSubHeaderSet3bitFSN(subHeader, storedFSN[i]);
            MacDot16PackSubHeaderSetFC(subHeader, storedFC[i]);
            MacDot16NotExtendedARQDisablePackSubHeaderSet11bitLength(
                subHeader,
                payloadLen);
            tempMsgIn = tempMsgIn->next;
        }
        pduSize = pduSize + (numOfPackedPacket * variableHeaderSize);
    }

    msgOut = MacDot16AddDataGrantMgmtandGenericMacSubHeader(
        node,
        dot16,
        sFlow,
        msgIn,
        pduSize);

    if (isPackingEnabled && !sFlow->isFixedLengthSDU
        && (numOfPackedPacket > 1 || storedFC[0] == DOT16_LAST_FRAGMENT))
    {
        MacDot16MacHeader* macHeader =
            (MacDot16MacHeader*) MESSAGE_ReturnPacket(msgOut);
        MacDot16MacHeaderSetGeneralType(macHeader, DOT16_PACKING_SUBHEADER);
    }
    return msgOut;
}// end of MacDot16BuildPDUDataPacket

// /**
// FUNCTION   :: MacDot16ScheduleFirstPacketFragmentEnable
// LAYER      :: MAC
// PURPOSE    :: BS node will call this function to check if first packet
//               can be scheduled when fragmentation is enabled
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16Bs     : MacDot16Bs* : Pointer to DOT16 BS structure
// + numFreePs  : Int32        : Number of free PS
// + dlMapIESizeInPs : Int32   : Size of dlMapIE in PS
// RETURN     :: BOOL : TRUE : when packet can be scheduled
//......................FALSE : when packet cannot be scheduled
// **/
static
BOOL MacDot16ScheduleFirstPacketFragmentEnable(
        Node *node,
        MacDot16Bs* dot16Bs,
        Int32 numFreePs,
        Int32 dlMapIESizeInPs)
{
    Int32 headerSize = 0;
    if (dot16Bs->isARQEnabled)
    {
        headerSize = (Int32)
           sizeof(MacDot16MacHeader) +
           sizeof(MacDot16ExtendedorARQEnableFragSubHeader);
    }
    else
    {
        headerSize = (Int32)
           sizeof(MacDot16MacHeader) +
           sizeof(MacDot16NotExtendedARQDisableFragSubHeader);
    }
    if ((numFreePs - dlMapIESizeInPs) > headerSize)
    {
        return (TRUE);
    }
    return (FALSE);
}


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
              clocktype duration)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo = NULL;
    MacDot16MacHeader* macHeader;
    MacDot16DlBurstProfile* burstProfile;
    MacDot16DlBurstProfile* bcastBurstProfile;
    int dlMapIESize = 0;
    int dlMapIESizeInPs = 0;
    Scheduler* scheduler;
    Message* nextPkt = NULL;
    int priority;
    BOOL notEmpty = FALSE;
    clocktype currentTime;
    int macHeaderLen;

    int numFreePs;
    Dot16CIDType cid;
    int pduSize;
    int pduSizeInPs;
    int i;
    BOOL isARQDisabled;

    currentTime = node->getNodeTime();
    macHeaderLen = sizeof(MacDot16MacHeader);

    // firstly, we convert the duration to # of physical slots
    numFreePs = (int) (duration / dot16->dlPsDuration);
    numFreePs *= MacDot16PhyGetNumSubchannels(node, dot16);

    bcastBurstProfile = &(dot16Bs->dlBurstProfile[DOT16_DIUC_BCAST]);

    // DL MAP IE size
    dlMapIESize = sizeof(MacDot16PhyOfdmaDlMapIE);
    dlMapIESizeInPs = MacDot16PhyBytesToPs(node,
                                       dot16,
                                       dlMapIESize,
                                       bcastBurstProfile,
                                       DOT16_DL);

    // 1. Discount the overhead of PHY synchronization
    // first symbol on all subchannel are
    // used for Premable deduct Preamble
    numFreePs = numFreePs - MacDot16PhyGetNumSubchannels(node, dot16);

    // 2. Reserve some physical slots for the DL-MAP
    pduSize = sizeof(MacDot16DlMapMsg) + macHeaderLen;
    pduSizeInPs = MacDot16PhyBytesToPs(node,
                                       dot16,
                                       pduSize,
                                       bcastBurstProfile,
                                       DOT16_DL);

    if (numFreePs < pduSizeInPs)
    {
        // no enough space, return;
        return 0;
    }
    numFreePs -= pduSizeInPs;

    // 3. Reserve some physical slots for the UL-MAP
    pduSize = sizeof(MacDot16UlMapMsg)
              + macHeaderLen
              + dot16Bs->numUlMapIEScheduled *
              sizeof(MacDot16PhyOfdmaUlMapIE);
    pduSizeInPs = MacDot16PhyBytesToPs(node,
                                       dot16,
                                       pduSize,
                                       bcastBurstProfile,
                                       DOT16_DL);

    if (numFreePs < pduSizeInPs + dlMapIESizeInPs)
    {
        // no enough space, return;
        return 0;
    }

    // update # of DL-MAP IE, add one IE for UL-MAP
    if (pduSizeInPs > 0)
    {
        dot16Bs->numDlMapIEScheduled ++;
    }
    numFreePs -= (pduSizeInPs + dlMapIESizeInPs);

    // 4. Reserve some physical slots for the DCD message if scheduled
    if (dot16Bs->dcdScheduled)
    {
        pduSize = MESSAGE_ReturnPacketSize(dot16Bs->dcdPdu) + macHeaderLen;
        pduSizeInPs = MacDot16PhyBytesToPs(node,
                                       dot16,
                                       pduSize,
                                       bcastBurstProfile,
                                       DOT16_DL);

        if (numFreePs < pduSizeInPs + dlMapIESizeInPs)
        {
            // no enough space, return;
            return 0;
        }

         // update # of DL-MAP IE, add one IE for DCD
        if (pduSizeInPs > 0)
        {
            dot16Bs->numDlMapIEScheduled ++;
        }

        numFreePs -= (pduSizeInPs + dlMapIESizeInPs);
    }

    // 5. Reserve some physical slots for the UCD message if scheduled
    if (dot16Bs->ucdScheduled)
    {
        pduSize = MESSAGE_ReturnPacketSize(dot16Bs->ucdPdu) + macHeaderLen;
        pduSizeInPs = MacDot16PhyBytesToPs(node,
                                       dot16,
                                       pduSize,
                                       bcastBurstProfile,
                                       DOT16_DL);

        if (numFreePs < pduSizeInPs + dlMapIESizeInPs)
        {
            // no enough space, return;
            return 0;
        }

        // update # of DL-MAP IE, add one IE for UCD
        if (pduSizeInPs > 0)
        {
            dot16Bs->numDlMapIEScheduled ++;
        }

        numFreePs -= (pduSizeInPs + dlMapIESizeInPs);
    }
    ///data can be sent starting from complete column that is empty
    numFreePs = (numFreePs / MacDot16PhyGetNumSubchannels(node, dot16)) *
                    MacDot16PhyGetNumSubchannels(node, dot16);
    // 6. loop to let the outbound schedulers schedule outgoing pkts one by
    //    one until all available slots are used
    for (i = 0; numFreePs > 0 && i <= DOT16_NUM_SERVICE_TYPES; i ++)
    {
        BOOL retriveSuc = TRUE;
        int pktIndexInQueue = 0;

        if (i == 0)
        {
            scheduler = dot16Bs->mgmtScheduler;
        }
        else
        {
            scheduler = dot16Bs->outScheduler[i - 1];
        }

        if (i != 0)
        {
            int pduSizeInPs = 0;
            burstProfile =
                &(dot16Bs->dlBurstProfile[DOT16_DIUC_BCAST]);
            int tempFreePs = numFreePs;

            MacDot16BsARQCheckIfServiceTypePacketInSflow(node,
                dot16,
                numFreePs,
                (MacDot16ServiceType)(i - 1));
            pduSizeInPs = tempFreePs - numFreePs;
            if (pduSizeInPs > 0)
            {
                numFreePs -= pduSizeInPs;
                //dot16Bs->numDlMapIEScheduled++;
            }
        }

        while (numFreePs > 0)
        {
            isARQDisabled = TRUE;
            if (!retriveSuc)
            {
                // move to pkt at the next position
                pktIndexInQueue ++;
            }

            // peek next packet
            notEmpty = scheduler->retrieve(ALL_PRIORITIES,
                                           pktIndexInQueue,
                                           &nextPkt,
                                           &priority,
                                           PEEK_AT_NEXT_PACKET,
                                           currentTime);

            if (notEmpty)
            {
                if (i == 0)
                {
                    // get destination information from the MAC header
                    macHeader =
                        (MacDot16MacHeader*) MESSAGE_ReturnPacket(nextPkt);
                    cid = MacDot16MacHeaderGetCID(macHeader);
                }
                else
                {
                    cid = MacDot16SchPriorityToCid(priority);
                }
                if (cid == DOT16_BROADCAST_CID ||
                    cid == DOT16_INITIAL_RANGING_CID)
                {
                    // this is a broadcast message, we use the most reliable
                    // burst profile for broadcast transmissions
                    burstProfile =
                        &(dot16Bs->dlBurstProfile[DOT16_DIUC_BCAST]);
                    retriveSuc = TRUE;
                }
                else if (MacDot16IsMulticastCid(node, dot16, cid))
                {
                    // this is a multicast message. Here, we use the most
                    // reliable burst profile. For better performance, one
                    // may want to choose based on the member nodes
                    // currently in the multicast group.
                    burstProfile =
                        &(dot16Bs->dlBurstProfile[DOT16_DIUC_BCAST]);

                    retriveSuc = TRUE;
                }
                else
                {
                    // this is unicast message. Get the SS info and its
                    // current active burst profile
                    ssInfo = MacDot16BsGetSsByCid(node, dot16, cid);
                    if (dot16->dot16eEnabled && ssInfo && ssInfo->inNbrScan)
                    {
                        // since the ss is in scan mode, the packet cannot
                        // be pulled out and sent to the SS, move to next
                        // one
                        retriveSuc = FALSE;

                        if (DEBUG_NBR_SCAN)
                        {
                            MacDot16PrintRunTimeInfo(node, dot16);
                            printf("SS with basic CID %d is in scanning,"
                                   "this pkt cannot be pulled, "
                                   "move to next packet\n",
                                   ssInfo->basicCid);
                        }

                        continue;
                    }
                    else
                    {
                        // it is OK to pull the packet out
                        retriveSuc = TRUE;
                    }

                    if (ssInfo == NULL)
                    {
                        // unknown destination SS, may lost the SS already.
                        // so drop the PDU
                        nextPkt = NULL;
                        scheduler->retrieve(ALL_PRIORITIES,
                                            pktIndexInQueue,
                                            &nextPkt,
                                            &priority,
                                            DEQUEUE_PACKET,
                                            currentTime);

                        if (nextPkt != NULL)
                        {
                            MESSAGE_Free(node, nextPkt);

                            // update stats
                            dot16Bs->stats.numPktsDroppedRemovedSs ++;
                        }

                        burstProfile = NULL;
                        continue;
                    }
                    else
                    {
                        // get current effective burst profile of this SS
                        burstProfile =
                                &(dot16Bs->dlBurstProfile[ssInfo->diuc]);

                        if (ssInfo->inHandover)
                        {
                            // update stats
                            dot16Bs->stats.numPktsDroppedSsInHandover ++;
                            // update dynamic stats
                            if (node->guiOption)
                            {
                                GUI_SendIntegerData(
                                    node->nodeId,
                                    dot16Bs->stats.numPktDropInHoGuiId,
                                    dot16Bs->stats.numPktsDroppedSsInHandover,
                                    node->getNodeTime());
                            }

                            // it supposed to be continue here
                            // in this implemetation, packet is not freed
                            // but send out though. pretty much the pakcet
                            // will not received by SS SS is listening to
                            // another channel
                        }
                    }
                }

                // now check if there is enough space for this PDU
                pduSize = MESSAGE_ReturnPacketSize(nextPkt);

                if (i != 0)
                {
                    pduSize += sizeof(MacDot16MacHeader);
                    if (MacDot16IsTransportCid(node, dot16, cid))
                    {
                        if (dot16Bs->isCRCEnabled)
                        {
                            // Add these 4 bytes dat in to virtual payload
                            pduSize += DOT16_CRC_SIZE;
                        }
                        // get sFlow and add variable headersize if req
                        MacDot16ServiceFlow* sFlow = NULL;
                        MacDot16BsGetServiceFlowByCid(
                            node,
                            dot16,
                            MacDot16SchPriorityToCid(priority),
                            &ssInfo,
                            &sFlow);
                        if (ssInfo && sFlow &&
                            dot16Bs->isFragEnabled && sFlow->isFragStart)
                        {
                            pduSize -= sFlow->bytesSent;
                            if (sFlow->isPackingEnabled)
                            {
                                pduSize +=
                                    sizeof(
                                    MacDot16NotExtendedARQDisablePackSubHeader);
                            }
                            else
                            {
                                pduSize +=
                                    sizeof(
                                    MacDot16NotExtendedARQDisableFragSubHeader);
                            }
                        }
                    }
                }
                pduSizeInPs = MacDot16PhyBytesToPs(node,
                                                   dot16,
                                                   pduSize,
                                                   burstProfile,
                                                   DOT16_DL);

                // determine if a new DL-MAP IE is  needed
                if (cid == DOT16_BROADCAST_CID ||
                        cid == DOT16_INITIAL_RANGING_CID)
                {
                    if (pduSizeInPs > 0 && dot16Bs->bcastPsAllocated == 0)
                    {
                        if (pduSizeInPs + dlMapIESizeInPs <= numFreePs)
                        {
                            // when the first packet for bcast flow is
                            // retrieved, a new DL-MAP IE may be needed
                            dot16Bs->numDlMapIEScheduled ++;
                            numFreePs -= dlMapIESizeInPs;
                        }
                        else
                        {
                            // not enough space to hold PDU and IE
                            // if fragmentation is enabled and the free
                            // space is more than headers then the first 
                            // packet must be fragmented and scheduled
                            if (dot16Bs->isFragEnabled &&
                                i != DOT16_SERVICE_UGS)
                            {
                                if (MacDot16ScheduleFirstPacketFragmentEnable
                                        (node,
                                        dot16Bs,
                                        numFreePs,
                                        dlMapIESizeInPs) == TRUE)
                                {
                                    dot16Bs->numDlMapIEScheduled ++;
                                    numFreePs -= dlMapIESizeInPs;
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
                    }
                }
                else if (MacDot16IsMulticastCid(node, dot16, cid))
                {
                    // currently, we treat multicast same as bcast
                    if (pduSizeInPs > 0 && dot16Bs->bcastPsAllocated == 0)
                    {
                        // when the first packet for bcast flow is
                        // retrieved, a new DL-MAP IE may be needed
                        if (pduSizeInPs + dlMapIESizeInPs <= numFreePs)
                        {
                            dot16Bs->numDlMapIEScheduled ++;
                            numFreePs -= dlMapIESizeInPs;
                        }
                        else
                        {
                            // not enough space to hold PDU and IE
                            
                            // if fragmentation is enabled and the free
                            // space is more than headers then the first 
                            // packet must be fragmented and scheduled
                            if (dot16Bs->isFragEnabled &&
                                i != DOT16_SERVICE_UGS)
                            {
                                if (MacDot16ScheduleFirstPacketFragmentEnable
                                        (node,
                                        dot16Bs,
                                        numFreePs,
                                        dlMapIESizeInPs) == TRUE)
                                {
                                    dot16Bs->numDlMapIEScheduled ++;
                                    numFreePs -= dlMapIESizeInPs;
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
                    }
                }
                else
                {
                    // unicast flow
                    ERROR_Assert(ssInfo != NULL, "Unknow packet!");
                    if (pduSizeInPs > 0 && ssInfo->dlPsAllocated == 0)
                    {
                        // when the first packet for this SS is retrieved,
                        // a new DL-MAP IE may be needed
                        if (pduSizeInPs + dlMapIESizeInPs <= numFreePs)
                        {
                            dot16Bs->numDlMapIEScheduled++;
                            numFreePs -= dlMapIESizeInPs;
                        }
                        else
                        {
                            // not enough space to hold PDU and IE
                            
                            // if fragmentation is enabled and the free
                            // space is more than headers then the first 
                            // packet must be fragmented and scheduled
                            if (dot16Bs->isFragEnabled &&
                                i != DOT16_SERVICE_UGS)
                            {
                                if (MacDot16ScheduleFirstPacketFragmentEnable
                                        (node,
                                        dot16Bs,
                                        numFreePs,
                                        dlMapIESizeInPs) == TRUE)
                                {
                                    dot16Bs->numDlMapIEScheduled ++;
                                    numFreePs -= dlMapIESizeInPs;
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
                    }
                }

                // Be note that, in the implemetation, a packet with large
                // size at the head of the queue may block the packets with
                // small size from transmission when the following if
                // clause does not satisfied. Further improvement can be
                // made

                //if (pduSizeInPs <= numFreePs)
                if ((pduSizeInPs <= numFreePs && dot16Bs->isFragEnabled != TRUE) ||
                    (pduSizeInPs <= numFreePs && dot16Bs->isFragEnabled == TRUE &&
                    pduSize <= DOT16_MAX_PDU_SIZE))
                {
                    // schedule this PDU for transmission
                    if (i == 0 || !MacDot16IsTransportCid(node, dot16, cid))
                    {
                        scheduler->retrieve(ALL_PRIORITIES,
                                            pktIndexInQueue,
                                            &nextPkt,
                                            &priority,
                                            DEQUEUE_PACKET,
                                            currentTime);

                        if (i == 0)
                        {
                            int index = 0;
                            unsigned int transId;
                            unsigned char msgType;
                            unsigned char* macFrame = NULL;
                            MacDot16MacHeader* macHeader;

                            MacDot16DsaRspMsg* dsaRsp;
                            MacDot16ServiceFlow* sFlow = NULL;

                            macFrame = (unsigned char*)
                                        MESSAGE_ReturnPacket(nextPkt);
                            macHeader = (MacDot16MacHeader*) &(macFrame[index]);

                            msgType = macFrame[index + sizeof(MacDot16MacHeader)];

                            if (msgType == DOT16_DSA_RSP)
                            {
                                macHeader = (MacDot16MacHeader*)
                                            &(macFrame[index]);
                                index += sizeof(MacDot16MacHeader);

                                dsaRsp = (MacDot16DsaRspMsg*)
                                         &(macFrame[index]);

                                transId =
                                    dsaRsp->transId[0] * 256
                                    + dsaRsp->transId[1];

                                ssInfo =
                                    MacDot16BsGetSsByCid(node,
                                                        dot16,
                                                        cid);

                                MacDot16BsGetServiceFlowByTransId(
                                                        node,
                                                        dot16,
                                                        ssInfo,
                                                        transId,
                                                        &sFlow);

                                // start T8
                                if (sFlow->dsaInfo.timerT8 != NULL)
                                {
                                    MESSAGE_CancelSelfMsg
                                        (node, sFlow->dsaInfo.timerT8);
                                    sFlow->dsaInfo.timerT8 = NULL;
                                }
                                // using transport cid a reference
                                sFlow->dsaInfo.timerT8 =
                                    MacDot16SetTimer(node,
                                                    dot16,
                                                    DOT16_TIMER_T8,
                                                    dot16Bs->para.t8Interval,
                                                    NULL,
                                                    sFlow->cid,
                                                    DOT16_FLOW_XACT_Add);

                                // move the dsx status
                                sFlow->dsaInfo.dsxTransStatus =
                                    DOT16_FLOW_DSA_REMOTE_DsaAckPending;

                                //increase stats
                                dot16Bs->stats.numDsaRspSent ++;
                                // set dsx retry
                                sFlow->dsaInfo.dsxRetry --;

                            }

                        }

                        else //if (i != 0)
                        {
                            // add the generic MAC header first
                            MESSAGE_AddHeader(node,
                                              nextPkt,
                                              sizeof(MacDot16MacHeader),
                                              TRACE_DOT16);
                            macHeader = (MacDot16MacHeader*)
                                MESSAGE_ReturnPacket(nextPkt);
                            memset((char*) macHeader, 0,
                                sizeof(MacDot16MacHeader));
                            // set the CID field and type
                            MacDot16MacHeaderSetCID(macHeader, cid);
                            UInt16 newLen =
                                 (UInt16)MESSAGE_ReturnPacketSize(nextPkt);
                            MacDot16MacHeaderSetLEN(macHeader,newLen);
                        }
                        MESSAGE_RemoveInfo(node, nextPkt, INFO_TYPE_Dot16BurstInfo);
                    }

                    // put into corresponding out buffer
                    if (cid == DOT16_BROADCAST_CID ||
                        cid == DOT16_INITIAL_RANGING_CID)
                    {
                        // put into the buffer for broadcast PDUs
                        if (dot16Bs->bcastOutBuffHead == NULL)
                        {
                            dot16Bs->bcastOutBuffHead = nextPkt;
                        }
                        else
                        {
                            dot16Bs->bcastOutBuffTail->next = nextPkt;
                        }
                        while (nextPkt->next != NULL)
                        {
                            nextPkt = nextPkt->next;
                        }
                        dot16Bs->bcastOutBuffTail = nextPkt;

                        dot16Bs->bcastPsAllocated = dot16Bs->bcastPsAllocated
                            + (UInt16)pduSizeInPs;
                        dot16Bs->bytesInBcastOutBuff += pduSize;
                    }
                    else if (MacDot16IsMulticastCid(node, dot16, cid))
                    {
                        // currently, we treat multicast same as bcast
                        if (dot16Bs->bcastOutBuffHead == NULL)
                        {
                            dot16Bs->bcastOutBuffHead = nextPkt;
                        }
                        else
                        {
                            dot16Bs->bcastOutBuffTail->next = nextPkt;
                        }
                        while (nextPkt->next != NULL)
                        {
                            nextPkt = nextPkt->next;
                        }
                        dot16Bs->bcastOutBuffTail = nextPkt;

                        dot16Bs->bcastPsAllocated = dot16Bs->bcastPsAllocated
                            + (UInt16)pduSizeInPs;
                        dot16Bs->bytesInBcastOutBuff += pduSize;

                        // increase statistics
                        dot16Bs->stats.numPktsToLower ++;
                        // update dynamic stats
                        if (node->guiOption)
                        {
                            GUI_SendIntegerData(node->nodeId,
                                                dot16Bs->stats.numPktToPhyGuiId,
                                                dot16Bs->stats.numPktsToLower,
                                                node->getNodeTime());
                        }
                    }
                    else
                    {
                        ERROR_Assert(ssInfo != NULL, "Unknow packet!");
                        // increase statistics
                        if (MacDot16IsTransportCid(node, dot16, cid))
                        {
                            int numFreeBytes = numFreePs * (
                                MacDot16PhyBitsPerPs(
                                node,
                                dot16,
                                burstProfile,
                                DOT16_DL)
                                / 8);
                            MacDot16ServiceFlow* sFlow = NULL;

                            MacDot16BsGetServiceFlowByCid(
                                node,
                                dot16,
                                MacDot16SchPriorityToCid(priority),
                                &ssInfo,
                                &sFlow);
                            if (ssInfo == NULL || sFlow == NULL)
                            {
                                ERROR_ReportWarning("no ssInfo/sflow"
                                    " associate with transport Cid");
                                // unknown destination SS, may lost the SS already.
                                // so drop the PDU
                                nextPkt = NULL;
                                scheduler->retrieve(ALL_PRIORITIES,
                                                    pktIndexInQueue,
                                                    &nextPkt,
                                                    &priority,
                                                    DEQUEUE_PACKET,
                                                    currentTime);
                                if (nextPkt != NULL)
                                {
                                    MESSAGE_Free(node, nextPkt);
                                    // update stats
                                    dot16Bs->stats.numPktsDroppedRemovedSs++;
                                }
                                burstProfile = NULL;
                                continue;
                            }
                            if (sFlow->isARQEnabled)
                            {
                                int numFreeBytesTemp = numFreeBytes;
                                isARQDisabled = FALSE;
                                MacDot16FillARQWindow(node,
                                                      dot16,
                                                      scheduler,
                                                      sFlow,
                                                      ssInfo);
                                while (numFreeBytes >
                                    (int)sizeof(MacDot16MacHeader))
                                {
                                    int tempFreeBytes = numFreeBytes;
                                    int tempDlPsAllocated = ssInfo->dlPsAllocated;
                                    MacDot16ARQCreatePDU(node,
                                                         dot16,
                                                         sFlow,
                                                         ssInfo,
                                                         numFreeBytes);
                                    if (tempFreeBytes == numFreeBytes)
                                    {
                                        break;
                                    }
                                    else if (tempDlPsAllocated == 0)
                                    {
                                        dot16Bs->numDlMapIEScheduled--;
                                    }
                                }// end of while
                                pduSize = numFreeBytesTemp - numFreeBytes;
                                if (pduSize <= 0 &&
                                    ssInfo->dlPsAllocated == 0)
                                {
                                    dot16Bs->numDlMapIEScheduled--;
                                    numFreePs += dlMapIESizeInPs;
                                }
                            }
                            else
                            {
                                nextPkt = MacDot16BuildPDUDataPacket(
                                    node,
                                    dot16,
                                    sFlow,
                                    scheduler,
                                    pduSize,
                                    numFreeBytes);
                                ERROR_Assert(nextPkt != NULL,
                                    "After add the MacDot16DataGrantSubHeader"
                                    "Packet size is more than num of free"
                                    " bytes\n");
                            }

                            pduSizeInPs = MacDot16PhyBytesToPs(
                                node,
                                dot16,
                                pduSize,
                                burstProfile,
                                DOT16_DL);
                        }//end of if (MacDot16IsTransportCid.....
                        // put into the buffer to the SS
                        if (isARQDisabled == TRUE)
                        {
                            if (ssInfo->outBuffHead == NULL)
                            {
                                ssInfo->outBuffHead = nextPkt;
                            }
                            else
                            {
                                ssInfo->outBuffTail->next = nextPkt;
                            }
                            while (nextPkt->next != NULL)
                            {
                                nextPkt = nextPkt->next;
                            }
                            ssInfo->outBuffTail = nextPkt;

                            ssInfo->dlPsAllocated = ssInfo->dlPsAllocated +
                                (UInt16)pduSizeInPs;
                            ssInfo->bytesInOutBuff += pduSize;
                        }
                    }

                    if (dot16->dot16eEnabled && retriveSuc)
                    {
                        if (MacDot16BsCheckOutgoingMsg(
                                node,
                                dot16,
                                nextPkt,
                                DOT16e_MOB_SCN_RSP))
                        {
                            // now ss is start scanning in next frame
                            ssInfo->inNbrScan = TRUE;

                            ssInfo->numScanFramesLeft =
                                            ssInfo->scanDuration;
                            ssInfo->numScanIterationsLeft =
                                            ssInfo->scanIteration;

                            if (DEBUG_NBR_SCAN)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                printf("SS with cid %d start to scan in "
                                       "next frame\n",
                                       ssInfo->basicCid);
                            }
                        }
                    }
                    numFreePs -= pduSizeInPs;
                } // end of if (numFreePs >= pduSizeInPs)
                else
                {
                    int numFreeBytes = numFreePs * (
                        MacDot16PhyBitsPerPs(
                        node,
                        dot16,
                        burstProfile,
                        DOT16_DL)
                        / 8);


                    if ((numFreeBytes > DOT16_MAX_PDU_SIZE) &&
                        (dot16Bs->isFragEnabled == TRUE))
                    {
                        numFreeBytes = DOT16_MAX_PDU_SIZE;
                    }

                    MacDot16ServiceFlow* sFlow = NULL;

                    MacDot16BsGetServiceFlowByCid(
                        node,
                        dot16,
                        MacDot16SchPriorityToCid(priority),
                        &ssInfo,
                        &sFlow);
                    if (ssInfo == NULL || sFlow == NULL)
                    {
                        ERROR_ReportWarning("no ssInfo/sflow associate\
                                             with transport Cid");
                        // unknown destination SS, may lost the SS
                        // so drop the PDU
                        nextPkt = NULL;
                        scheduler->retrieve(ALL_PRIORITIES,
                                            pktIndexInQueue,
                                            &nextPkt,
                                            &priority,
                                            DEQUEUE_PACKET,
                                            currentTime);
                        if (nextPkt != NULL)
                        {
                            MESSAGE_Free(node, nextPkt);
                            // update stats
                            dot16Bs->stats.numPktsDroppedRemovedSs ++;
                        }
                        //burstProfile = NULL;
                        continue;
                    }
                    if (sFlow->isFixedLengthSDU == TRUE)
                    {
                        break;
                    }


                    if (i != 0 && (dot16Bs->isFragEnabled == TRUE)
                        && MacDot16IsTransportCid(node, dot16, cid)
                        && (numFreeBytes > (int)(sizeof(MacDot16MacHeader) +
                        sizeof(MacDot16ExtendedorARQEnableFragSubHeader)))
                        && sFlow->isARQEnabled)
                    {

                        int numFreeBytesTemp = numFreeBytes;
                        isARQDisabled = FALSE;

                        MacDot16FillARQWindow(node,
                                              dot16,
                                              scheduler,
                                              sFlow,
                                              ssInfo);

                        while (numFreeBytes >
                            (int)sizeof(MacDot16MacHeader))
                        {
                            int tempFreeBytes = numFreeBytes;
                            int tempDlPsAllocated = ssInfo->dlPsAllocated;
                            MacDot16ARQCreatePDU(node,
                                                 dot16,
                                                 sFlow,
                                                 ssInfo,
                                                 numFreeBytes);
                            if (tempFreeBytes == numFreeBytes)
                            {
                                break;
                            }
                            else if (tempDlPsAllocated == 0)
                            {
                                dot16Bs->numDlMapIEScheduled--;
                            }
                        }// end of while

                        pduSize = numFreeBytesTemp - numFreeBytes;

                        if (pduSize <= 0 &&
                            ssInfo->dlPsAllocated == 0)
                        {
                            dot16Bs->numDlMapIEScheduled--;
                            numFreePs += dlMapIESizeInPs;
                        }
                        else
                        {
                            pduSizeInPs = MacDot16PhyBytesToPs(
                                            node,
                                            dot16,
                                            pduSize,
                                            burstProfile,
                                            DOT16_DL);

                            numFreePs -= pduSizeInPs;
                        }

                        if (dot16->dot16eEnabled && retriveSuc)
                        {
                            if (MacDot16BsCheckOutgoingMsg(
                                    node,
                                    dot16,
                                    nextPkt,
                                    DOT16e_MOB_SCN_RSP))
                            {
                                // now ss is start scanning in next frame
                                ssInfo->inNbrScan = TRUE;

                                ssInfo->numScanFramesLeft =
                                                ssInfo->scanDuration;
                                ssInfo->numScanIterationsLeft =
                                                ssInfo->scanIteration;

                                if (DEBUG_NBR_SCAN)
                                {
                                    MacDot16PrintRunTimeInfo(node, dot16);
                                    printf("SS with cid %d start to scan in "
                                           "next frame\n",
                                           ssInfo->basicCid);
                                }//if (DEBUG_NBR_SCAN)
                            }//if (MacDot16BsCheckOutgoingMsg
                        }//if (dot16->dot16eEnabled && retriveSuc)
                    }

                    else if (i != 0 && (dot16Bs->isFragEnabled == TRUE)
                            && MacDot16IsTransportCid(node, dot16, cid)
                            && (numFreeBytes > (int)(sizeof(MacDot16MacHeader) +
                            sizeof(MacDot16NotExtendedARQDisableFragSubHeader)))
                            && !sFlow->isARQEnabled)
                    {
                        UInt8 fcVal= DOT16_FIRST_FRAGMENT;
                        Message* msgOut;
                        int payloadLen;
                        MacDot16NotExtendedARQDisableFragSubHeader* subHeader;

                        if (sFlow->isFixedLengthSDU == TRUE)
                        {
                            break;
                        }
                        // Fragment the data packet here
                        if (sFlow->isFragStart)
                        {
                            if (DEBUG_PACKING_FRAGMENTATION)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                printf("BS Add middle fragmented subheader\n");
                            }
                            // Middle packet
                            fcVal = DOT16_MIDDLE_FRAGMENT;
                        }
                        else
                        {
                            if (DEBUG_PACKING_FRAGMENTATION)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                printf("BS Add first fragmented subheader\n");
                            }
                            // First Packet
                            sFlow->isFragStart = TRUE;
                        }

                        msgOut = MESSAGE_Duplicate(node, nextPkt);
                        MESSAGE_RemoveInfo(node, msgOut, INFO_TYPE_Dot16BurstInfo);
                        payloadLen = numFreeBytes
                            - sizeof(
                            MacDot16NotExtendedARQDisableFragSubHeader)
                            - sizeof(MacDot16MacHeader);
                        if (dot16Bs->isCRCEnabled == TRUE)
                        {
                            payloadLen -= DOT16_CRC_SIZE;
                        }

                        if (payloadLen <= 0)
                        {
                            if (fcVal == DOT16_FIRST_FRAGMENT)
                            {
                                sFlow->isFragStart = FALSE;
                            }
                            MESSAGE_Free(node, msgOut);
                            break;
                        }
                        if (sFlow->bytesSent)
                        {
                            if (sFlow->bytesSent >
                                MESSAGE_ReturnActualPacketSize(msgOut))
                            {
                                int temp = sFlow->bytesSent -
                                    MESSAGE_ReturnActualPacketSize(msgOut);
                                MESSAGE_ShrinkPacket(node, msgOut,
                                    MESSAGE_ReturnActualPacketSize(msgOut));
                                MESSAGE_RemoveVirtualPayload(node, msgOut, temp);
                            }
                            else
                            {
                                MESSAGE_ShrinkPacket(node, msgOut, sFlow->bytesSent);
                            }
                        }

                        MESSAGE_RemoveVirtualPayload(node, msgOut,
                            MESSAGE_ReturnVirtualPacketSize(msgOut));

                        // Add Fragmentation header
                        MESSAGE_AddHeader(
                            node,
                            msgOut,
                            sizeof(
                            MacDot16NotExtendedARQDisableFragSubHeader),
                            TRACE_DOT16);

                        subHeader =
                            (MacDot16NotExtendedARQDisableFragSubHeader*)
                            MESSAGE_ReturnPacket(msgOut);
                        memset(subHeader, 0,
                            sizeof(
                            MacDot16NotExtendedARQDisableFragSubHeader));
                        MacDot16FragSubHeaderSet3bitFSN(subHeader,
                            sFlow->fragFSNno);
                        sFlow->fragFSNno++;
                        if (sFlow->fragFSNno >= DOT16_MAX_NO_FRAGMENTED_PACKET)
                        {
                                sFlow->fragFSNno = sFlow->fragFSNno
                                    % DOT16_MAX_NO_FRAGMENTED_PACKET;
                        }

                        MacDot16FragSubHeaderSetFC(subHeader, fcVal);

                        if (payloadLen > MESSAGE_ReturnActualPacketSize(
                            msgOut))
                        {
                            int temp = payloadLen -
                                MESSAGE_ReturnActualPacketSize(msgOut)+
                                sizeof (MacDot16NotExtendedARQDisableFragSubHeader);
                            MESSAGE_AddVirtualPayload(node, msgOut, temp);
                        }
                        else
                        {
                            msgOut->packetSize = payloadLen +
                                sizeof(
                                MacDot16NotExtendedARQDisableFragSubHeader);
                        }

                        sFlow->bytesSent = sFlow->bytesSent +
                                          (UInt16)payloadLen;
                        // Add Data Grant Mgmt header if required
                        // Add generic MAC header
                        pduSize = MESSAGE_ReturnPacketSize(msgOut)
                            + sizeof(MacDot16MacHeader);
                        if (dot16Bs->isCRCEnabled == TRUE)
                        {
                            // Add four bytes for CRC
                            pduSize += DOT16_CRC_SIZE;
                        }
                        msgOut =
                            MacDot16AddDataGrantMgmtandGenericMacSubHeader(
                            node,
                            dot16,
                            sFlow,
                            msgOut,
                            pduSize);
                        macHeader =
                            (MacDot16MacHeader*) MESSAGE_ReturnPacket(msgOut);
                        MacDot16MacHeaderSetGeneralType(macHeader,
                            DOT16_FRAGMENTATION_SUBHEADER);
                        nextPkt = msgOut;

                        pduSizeInPs = MacDot16PhyBytesToPs(
                            node,
                            dot16,
                            pduSize,
                            burstProfile,
                            DOT16_DL);

                        // put into the buffer to the SS
                        if (ssInfo->outBuffHead == NULL)
                        {
                            ssInfo->outBuffHead = nextPkt;
                        }
                        else
                        {
                            ssInfo->outBuffTail->next = nextPkt;
                        }
                        while (nextPkt->next != NULL)
                        {
                            nextPkt = nextPkt->next;
                        }
                        ssInfo->outBuffTail = nextPkt;

                        ssInfo->dlPsAllocated = ssInfo->dlPsAllocated +
                            (UInt16)pduSizeInPs;
                        ssInfo->bytesInOutBuff += pduSize;

                        if (dot16->dot16eEnabled && retriveSuc)
                        {
                            if (MacDot16BsCheckOutgoingMsg(
                                    node,
                                    dot16,
                                    nextPkt,
                                    DOT16e_MOB_SCN_RSP))
                            {
                                // now ss is start scanning in next frame
                                ssInfo->inNbrScan = TRUE;

                                ssInfo->numScanFramesLeft =
                                                ssInfo->scanDuration;
                                ssInfo->numScanIterationsLeft =
                                                ssInfo->scanIteration;

                                if (DEBUG_NBR_SCAN)
                                {
                                    MacDot16PrintRunTimeInfo(node, dot16);
                                    printf("SS with cid %d start to scan in "
                                           "next frame\n",
                                           ssInfo->basicCid);
                                }
                            }
                        }
                        numFreePs -= pduSizeInPs;
                        dot16Bs->stats.numFragmentsSent++;
                    }
                    else
                    {
                        break;
                    }
                    break;
                } // end of if (pduSizeInPs > numFreePs)
            }
            else
            {
                // no more packets in the queues, stop
                // if move to next queue, reset pktIndexInQueue
                pktIndexInQueue = 0;
                break;
            } // notEmpty
            if (isARQDisabled == FALSE)
            {
                break;
            }
        } // while
    } // for (i)

    // reserve a place a DIUC-END DL-MAP IE
    dot16Bs->numDlMapIEScheduled ++;

    // check if can move packets from upper layer
    MacDot16NetworkLayerHasPacketToSend(node, dot16);

    return numFreePs;
}

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
int MacDot16ScheduleUlBurst(Node* node, MacDataDot16* dot16, int bwGranted)
{
    MacDot16Ss* dot16Ss = (MacDot16Ss*) dot16->ssData;

    Scheduler* scheduler;
    BOOL isARQEnabled = FALSE;
    Message* nextPkt = NULL;
    int priority;
    BOOL notEmpty = FALSE;
    clocktype currentTime;
    MacDot16MacHeader* macHeader;
    int numFreeBytes;
    int pduSize;
    MacDot16ServiceFlow *sFlow = NULL;
    MacDot16BsSsInfo* ssInfo = NULL;
    int i;

    if (DEBUG)
    {
        printf("Node%d schedules PDUs for UL burst,bandwith size = %d bytes\n",
               node->nodeId, bwGranted);
    }

    currentTime = node->getNodeTime();

    // # of free bytes is initially set to bw granted
    numFreeBytes = bwGranted;

    // loop to let the outbound schedulers schedule outgoing pkts one by
    // one until all available slots are used or no more pkts in queue.
    for (i = 0; numFreeBytes > 0 && i <= DOT16_NUM_SERVICE_TYPES; i ++)
    {
        if (i == 0)
        {
            scheduler = dot16Ss->mgmtScheduler;
        }
        else
        {
            scheduler = dot16Ss->outScheduler[i - 1];
        }

        if (i != 0)
        {
            MacDot16SsARQCheckIfServiceTypePacketInSflow(node,
                dot16,
                numFreeBytes,
                (MacDot16ServiceType)(i - 1));
        }
        while (numFreeBytes > 0)
        {
            // peek next packet
            notEmpty = scheduler->retrieve(ALL_PRIORITIES,
                                           0,
                                           &nextPkt,
                                           &priority,
                                           PEEK_AT_NEXT_PACKET,
                                           currentTime);

            if (notEmpty)
            {
                int tempNumFreeBytes = numFreeBytes;
                if (tempNumFreeBytes > DOT16_MAX_PDU_SIZE)
                {
                    tempNumFreeBytes = DOT16_MAX_PDU_SIZE;
                }

                // check if there is enough space for this PDU
                pduSize = MESSAGE_ReturnPacketSize(nextPkt);
                if (i!= 0)
                {
                    sFlow = MacDot16SsGetServiceFlowByCid(
                                        node,
                                        dot16,
                                        MacDot16SchPriorityToCid(priority));
                    if (sFlow == NULL)
                    {
                        ERROR_ReportWarning("no ssInfo/sflow"
                            " associated with transport Cid");
                        // unknown destination SS, may lost the SS
                        // so drop the PDU
                        nextPkt = NULL;
                        scheduler->retrieve(ALL_PRIORITIES,
                                        0,
                                        &nextPkt,
                                        &priority,
                                        DEQUEUE_PACKET,
                                        currentTime);
                        if (nextPkt != NULL)
                        {
                            MESSAGE_Free(node, nextPkt);
                            // update stats
                        }
                        break ;
                    }
                    isARQEnabled = sFlow->isARQEnabled;
                    if (isARQEnabled)
                    {
                        MacDot16FillARQWindow(node,
                                              dot16,
                                              scheduler,
                                              sFlow,
                                              NULL);
                        while (numFreeBytes > (int)sizeof(MacDot16MacHeader))
                        {
                            int tempFreeBytes = numFreeBytes;
                            MacDot16ARQCreatePDU(node,
                                                 dot16,
                                                 sFlow,
                                                 ssInfo,
                                                 numFreeBytes);
                            if (tempFreeBytes == numFreeBytes)
                            {
                                break;
                            }
                        }// end of while
                        break;
                    }
                }

                if (i != 0)
                {
                    // Generic MAC header
                    pduSize +=  sizeof(MacDot16MacHeader);
                    // Check CRC field need to add in to management packet
                    if (dot16Ss->isCRCEnabled == TRUE)
                    {
                        // Add these 4 bytes dat in to virtual payload
                        pduSize += DOT16_CRC_SIZE;
                    }
                    if (sFlow && dot16Ss->isFragEnabled && sFlow->isFragStart)
                    {
                        pduSize -= sFlow->bytesSent;
                        if (dot16Ss->isPackingEnabled)
                    {
                        pduSize +=
                            sizeof(
                            MacDot16NotExtendedARQDisablePackSubHeader);
                    }
                        else
                        {
                            pduSize +=
                                sizeof(
                                MacDot16NotExtendedARQDisableFragSubHeader);
                        }
                    }
                }
                if (pduSize <= tempNumFreeBytes)
                {
                    if (i == 0)
                    {
                        // Handle management packet
                        // schedule this PDU for transmission
                        scheduler->retrieve(ALL_PRIORITIES,
                                            0,
                                            &nextPkt,
                                            &priority,
                                            DEQUEUE_PACKET,
                                            currentTime);


                        unsigned char msgType;
                        unsigned char* macFrame = NULL;

                        //macHeader =
                        //    (MacDot16MacHeader*) MESSAGE_ReturnPacket(nextPkt);
                        macFrame = (unsigned char*) MESSAGE_ReturnPacket(nextPkt);

                        msgType = macFrame[sizeof(MacDot16MacHeader)];

                        if (msgType == DOT16_DSA_RSP)
                        {
                            int index = 0;
                            unsigned int transId;
                            MacDot16DsaRspMsg* dsaRsp;

                            macHeader =
                                (MacDot16MacHeader*) &(macFrame[index]);

                            index += sizeof(MacDot16MacHeader);

                            dsaRsp =
                                (MacDot16DsaRspMsg*) &(macFrame[index]);

                            transId = dsaRsp->transId[0] * 256 +
                                        dsaRsp->transId[1];

                            sFlow =
                                MacDot16SsGetServiceFlowByTransId(node,
                                                                 dot16,
                                                                 transId);

                            // start T8
                            if (sFlow->dsaInfo.timerT8 != NULL)
                            {
                                MESSAGE_CancelSelfMsg(node,
                                                sFlow->dsaInfo.timerT8);
                                sFlow->dsaInfo.timerT8 = NULL;
                            }

                            sFlow->dsaInfo.timerT8 =
                                MacDot16SetTimer(node,
                                                dot16,
                                                DOT16_TIMER_T8,
                                                dot16Ss->para.t8Interval,
                                                NULL,
                                                transId,
                                                // using transport cid a
                                                // reference
                                                DOT16_FLOW_XACT_Add);
                            // set dsx retry
                            sFlow->dsaInfo.dsxRetry --;

                            // move the dsx status
                            sFlow->dsaInfo.dsxTransStatus =
                                DOT16_FLOW_DSA_REMOTE_DsaAckPending;

                            // increase stats
                            dot16Ss->stats.numDsaRspSent ++;

                        }
                        // to see if piggyback bw req is needed for this flow
                        // currently only non-management data can be used for
                        // piggyacking BW REQ
                        if (priority == DOT16_SCH_BASIC_MGMT_QUEUE_PRIORITY)
                        {
                            if (dot16Ss->needPiggyBackBasicBwReq)
                            {
                                // TODO
                            }
                        }
                        else if (priority ==
                                 DOT16_SCH_PRIMARY_MGMT_QUEUE_PRIORITY)
                        {
                            if (dot16Ss->needPiggyBackPriBwReq)
                            {
                                // TODO
                            }
                        }
                        else if (priority ==
                                 DOT16_SCH_SECONDARY_MGMT_QUEUE_PRIORITY)
                        {
                            if (dot16Ss->needPiggyBackSecBwReq)
                            {
                                // TODO
                            }
                        }
                    }// end of if i == 0
                    else
                    {
                        sFlow = MacDot16SsGetServiceFlowByCid(
                                    node,
                                    dot16,
                                    MacDot16SchPriorityToCid(priority));
                        // : put packet in to some buffer
                        // here Packet format is
                        // Generic MAC Header + Sub Header + Payload +
                        if (sFlow == NULL)
                        {
                            ERROR_ReportWarning("no ssInfo/sflow\
                                                associate with transport Cid");
                            // unknown destination SS, may lost the SS already.
                            // so drop the PDU
                            nextPkt = NULL;
                            scheduler->retrieve(ALL_PRIORITIES,
                                            0,
                                            &nextPkt,
                                            &priority,
                                            DEQUEUE_PACKET,
                                            currentTime);
                            if (nextPkt != NULL)
                            {
                                MESSAGE_Free(node, nextPkt);
                            }
                            break;
                        }
                        nextPkt = MacDot16BuildPDUDataPacket(
                            node,
                            dot16,
                            sFlow,
                            scheduler,
                            pduSize,
                            tempNumFreeBytes);
                        ERROR_Assert(nextPkt != NULL,
                            "After add the MacDot16DataGrantSubHeader"
                            "Packet size is more than num of free"
                            " bytes\n");
                    }
                }
                else
                {
                    // Ignore the management packet here
                    if (i != 0 && (dot16Ss->isFragEnabled == TRUE)
                        && (tempNumFreeBytes > (int)(sizeof(MacDot16MacHeader) +
                        sizeof(MacDot16NotExtendedARQDisableFragSubHeader))))
                    {
                        UInt8 fcVal= DOT16_FIRST_FRAGMENT;
                        Message* msgOut;
                        int payloadLen;
                        MacDot16NotExtendedARQDisableFragSubHeader* subHeader;

                        sFlow = MacDot16SsGetServiceFlowByCid(
                                    node,
                                    dot16,
                                    MacDot16SchPriorityToCid(priority));
                        if (sFlow == NULL)
                        {
                            ERROR_ReportWarning("no ssInfo/sflow\
                                                associate with transport Cid");
                            // unknown destination SS, may lost the SS already.
                            // so drop the PDU
                            nextPkt = NULL;
                            scheduler->retrieve(ALL_PRIORITIES,
                                            0,
                                            &nextPkt,
                                            &priority,
                                            DEQUEUE_PACKET,
                                            currentTime);
                            if (nextPkt != NULL)
                            {
                                MESSAGE_Free(node, nextPkt);
                            }
                            break;;
                        }
                        if (sFlow->isFixedLengthSDU == TRUE)
                        {
                            break;
                        }

                        // Fragment the data packet here
                        if (sFlow->isFragStart)
                        {
                            if (DEBUG_PACKING_FRAGMENTATION)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                printf("SS Add middle fragmented subheader\n");
                            }
                            // Middle packet
                            fcVal = DOT16_MIDDLE_FRAGMENT;
                        }
                        else
                        {
                            if (DEBUG_PACKING_FRAGMENTATION)
                            {
                                MacDot16PrintRunTimeInfo(node, dot16);
                                printf("SS Add first fragmented subheader\n");
                            }
                            // First Packet
                            sFlow->isFragStart = TRUE;
                        }

                        msgOut = MESSAGE_Duplicate(node, nextPkt);

                        payloadLen = tempNumFreeBytes
                            - sizeof(MacDot16NotExtendedARQDisableFragSubHeader)
                            - sizeof(MacDot16MacHeader);
                        if (dot16Ss->isCRCEnabled == TRUE)
                        {
                            // Add four bytes for CRC
                            payloadLen -= DOT16_CRC_SIZE;
                        }
                        if (sFlow->needPiggyBackBwReq)
                        {
                            payloadLen -= sizeof(MacDot16DataGrantSubHeader);
                        }

                        if (payloadLen <= 0)
                        {
                            if (fcVal == DOT16_FIRST_FRAGMENT)
                            {
                                sFlow->isFragStart = FALSE;
                            }
                            MESSAGE_Free(node, msgOut);
                            break;
                        }
                        if (sFlow->bytesSent)
                        {
                            if (sFlow->bytesSent >
                                MESSAGE_ReturnActualPacketSize(msgOut))
                            {
                                int temp = sFlow->bytesSent -
                                    MESSAGE_ReturnActualPacketSize(msgOut);
                                MESSAGE_ShrinkPacket(node, msgOut,
                                    MESSAGE_ReturnActualPacketSize(msgOut));
                                MESSAGE_RemoveVirtualPayload(node, msgOut, temp);
                            }
                            else
                            {
                                MESSAGE_ShrinkPacket(node, msgOut, sFlow->bytesSent);
                            }
                        }
                        MESSAGE_RemoveVirtualPayload(node, msgOut,
                            MESSAGE_ReturnVirtualPacketSize(msgOut));

                        // Add Fragmentation header
                        MESSAGE_AddHeader(
                            node,
                            msgOut,
                            sizeof(MacDot16NotExtendedARQDisableFragSubHeader),
                            TRACE_DOT16);
                        subHeader =
                            (MacDot16NotExtendedARQDisableFragSubHeader*)
                            MESSAGE_ReturnPacket(msgOut);
                        memset(
                            subHeader,
                            0,
                            sizeof(MacDot16NotExtendedARQDisableFragSubHeader));
                        MacDot16FragSubHeaderSet3bitFSN(
                            subHeader,
                            sFlow->fragFSNno);
                        MacDot16FragSubHeaderSetFC(subHeader, fcVal);
                        sFlow->fragFSNno++;
                        if (sFlow->fragFSNno >=
                            DOT16_MAX_NO_FRAGMENTED_PACKET)
                        {
                                sFlow->fragFSNno = sFlow->fragFSNno
                                    % DOT16_MAX_NO_FRAGMENTED_PACKET;
                        }

                        if (payloadLen >= MESSAGE_ReturnActualPacketSize(
                            msgOut))
                        {
                            int temp = payloadLen -
                                MESSAGE_ReturnActualPacketSize(msgOut) +
                                sizeof (MacDot16NotExtendedARQDisableFragSubHeader);
                            MESSAGE_AddVirtualPayload(node, msgOut, temp);
                        }
                        else
                        {
                            msgOut->packetSize = payloadLen +
                                sizeof(
                                MacDot16NotExtendedARQDisableFragSubHeader);
                        }

                        sFlow->bytesSent = sFlow->bytesSent +
                                    (UInt16)payloadLen;
                        if (MESSAGE_ReturnPacketSize(nextPkt)
                            - sFlow->bytesSent == 0)
                        {
                           fcVal = DOT16_LAST_FRAGMENT;
                           nextPkt = NULL;
                            scheduler->retrieve(ALL_PRIORITIES,
                                            0,
                                            &nextPkt,
                                            &priority,
                                            DEQUEUE_PACKET,
                                            currentTime);
                            if (nextPkt != NULL)
                            {
                                MESSAGE_Free(node, nextPkt);
                                nextPkt = NULL;
                            }
                            MacDot16FragSubHeaderSetFC(subHeader, fcVal);
                            sFlow->isFragStart = FALSE;
                            sFlow->bytesSent = 0;
                        }

                        // Add Data Grant Mgmt header if required
                        // Add generic MAC header
                        pduSize = MESSAGE_ReturnPacketSize(msgOut)
                            + sizeof(MacDot16MacHeader);
                        if (sFlow->needPiggyBackBwReq)
                        {
                            pduSize += sizeof(MacDot16DataGrantSubHeader);
                        }
                        if (dot16Ss->isCRCEnabled == TRUE)
                        {
                            // Add four bytes for CRC
                            pduSize += DOT16_CRC_SIZE;
                        }

                        msgOut =
                            MacDot16AddDataGrantMgmtandGenericMacSubHeader(
                            node,
                            dot16,
                            sFlow,
                            msgOut,
                            pduSize);
                        macHeader =
                            (MacDot16MacHeader*) MESSAGE_ReturnPacket(msgOut);
                        MacDot16MacHeaderSetGeneralType(macHeader,
                            DOT16_FRAGMENTATION_SUBHEADER);
                        nextPkt = msgOut;
                        dot16Ss->stats.numFragmentsSent++;
                    }
                    else
                    {
                        break;
                    }
                    //break;
                }// end of else
                // put into the buffer to the BS

                MESSAGE_RemoveInfo(node, nextPkt, INFO_TYPE_Dot16BurstInfo);

                if (dot16Ss->outBuffHead == NULL)
                {
                    dot16Ss->outBuffHead = nextPkt;
                }
                else
                {
                    dot16Ss->outBuffTail->next = nextPkt;
                }
                while (nextPkt->next != NULL)
                {
                    nextPkt = nextPkt->next;
                }
                dot16Ss->outBuffTail = nextPkt;

                dot16Ss->ulBytesAllocated += pduSize;
                numFreeBytes -= pduSize;

            }// end of if notEmpty
            else
            {
                // no more packets in the queues, stop
                break;
            }
        } // end of while
    }

    // check if can move packets from upper layer
    if (!dot16->dot16eEnabled)
    {
        MacDot16NetworkLayerHasPacketToSend(node, dot16);
    }
    else if (dot16Ss->hoStatus == DOT16e_SS_HO_None)
    {
        MacDot16NetworkLayerHasPacketToSend(node, dot16);
    }
    return numFreeBytes;
}

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
         clocktype duration)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16BsSsInfo* ssInfo = NULL;
    MacDot16ServiceFlow* sFlow;

    clocktype currentTime;
    clocktype timeDiff;
    int macHeaderLen;
    unsigned int numFreePs;
    unsigned int numPs;
    int dataLen;
    int i;
    int j;

    currentTime = node->getNodeTime();
    macHeaderLen = sizeof(MacDot16MacHeader);

    // firstly, we convert the duration to # of physical slots
    numFreePs = (int) (duration / dot16->ulPsDuration);
    numFreePs *= MacDot16PhyGetUplinkNumSubchannels(node, dot16);

    // 1. Schedule contention based initial ranging tx opps
    numPs = dot16Bs->para.rangeOppSize * DOT16_NUM_INITIAL_RANGE_OPPS;
    if (numPs > numFreePs)
    {
        numPs = numFreePs;
    }
    dot16Bs->rangePsAllocated = (UInt16)numPs;
    dot16Bs->numUlMapIEScheduled ++;
    numFreePs -= numPs;

// Size for CDMA allocation IE packet
    if (dot16Bs->rngType == DOT16_CDMA ||
        dot16Bs->bwReqType == DOT16_BWReq_CDMA)
    {
        //allocate less PS for bandwidth request.
        UInt16 totalRangePackets = 0;
        UInt16 totalBWPackets = 0;
        dot16Bs->cdmaAllocationIEPsAllocated = 0;
        MacDot16RngCdmaInfoList* cdmaInfo = dot16Bs->recCDMARngList;
        while (cdmaInfo != NULL && (cdmaInfo->rngRspSentFlag == TRUE
            || cdmaInfo->codetype != DOT16_CDMA_INITIAL_RANGING_CODE))
        {
            if (cdmaInfo->codetype == DOT16_CDMA_BWREQ_CODE)
            {
                totalBWPackets++;
            }
            else
            {
                totalRangePackets++;
            }
            cdmaInfo = cdmaInfo->next;
        }

        if (totalBWPackets > 0 || totalRangePackets>0)
        {
            dot16Bs->cdmaAllocationIEPsAllocated =
                (dot16Bs->para.rangeOppSize +
                (UInt16)dot16Bs->para.sstgInPs) * totalRangePackets;
            dot16Bs->cdmaAllocationIEPsAllocated =
                dot16Bs->cdmaAllocationIEPsAllocated +
                (UInt16)((dot16Bs->para.requestOppSize +
                dot16Bs->para.sstgInPs) * totalBWPackets);
        }
    }
    numPs = dot16Bs->cdmaAllocationIEPsAllocated;
    if (numPs > numFreePs)
    {
        numPs = numFreePs;
    }
    dot16Bs->cdmaAllocationIEPsAllocated = (UInt16)numPs;
    dot16Bs->numUlMapIEScheduled ++;
    numFreePs -= numPs;
    // 2. Schedule contention based bandwidth request tx opps
    numPs = dot16Bs->para.requestOppSize *
            DOT16_NUM_BANDWIDTH_REQUEST_OPPS;
    if (numPs > numFreePs)
    {
        numPs = numFreePs;
    }
    dot16Bs->requestPsAllocated = (UInt16)numPs;
    dot16Bs->numUlMapIEScheduled ++;
    numFreePs -= numPs;

    // 3. Schedule any other multicast pulling tx opps

    // NOTE: The code below goes through the SS list or flow list multiple
    //       times. This is not efficient. However, this is necessary in
    //       order to guarantee service priorities.

    // 4. Schedule UL burst for each individual SSs
    // 4.1. Allocate periodical ranging opps
    for (i = 0; numFreePs >= dot16Bs->para.rangeOppSize &&
         i < DOT16_BS_SS_HASH_SIZE; i ++)
    {
        ssInfo = dot16Bs->ssHash[i];

        // check all SSs with the same hash ID
        while (ssInfo != NULL)
        {
            if (ssInfo->needInitRangeGrant ||
                ssInfo->needPeriodicRangeGrant)
            {
                numPs = dot16Bs->para.rangeOppSize;
                if (numPs <= numFreePs)
                {
                    ssInfo->rangePsAllocated = (UInt16)numPs;
                    numFreePs -= numPs;
                }
                else
                {
                    ssInfo->rangePsAllocated = 0;
                    // no need to continue
                    break;
                }

                // update # of UL MAP IE
                if (ssInfo->rangePsAllocated)
                {
                    dot16Bs->numUlMapIEScheduled ++;
                }
            }

            ssInfo = ssInfo->next;
        }
    }

    // 4.2. Allocate bandwidth request opps
    for (i = 0; ((numFreePs >= dot16Bs->para.requestOppSize) &&
         (i < DOT16_BS_SS_HASH_SIZE)); i++)
    {
        ssInfo = dot16Bs->ssHash[i];

        // check all SSs with the same hash ID
        while (ssInfo != NULL)
        {
            int numUcastPolls = 0;

            // this is for mgmt connections
            if (ssInfo->needUcastPoll)
            {
                numUcastPolls ++;
            }

            // check rtPS polling
            sFlow = ssInfo->ulFlowList[DOT16_SERVICE_rtPS].flowHead;
            while (sFlow != NULL)
            {
                timeDiff = currentTime - sFlow->lastAllocTime;
                if (sFlow->activated && sFlow->bwRequested == 0 &&
                    (timeDiff >= sFlow->qosInfo.maxLatency ||
                     timeDiff >= SECOND))
                {
                    numUcastPolls ++;
                    sFlow->lastAllocTime = currentTime;
                }
                sFlow = sFlow->next;
            }

            // check nrtPS polling
            sFlow = ssInfo->ulFlowList[DOT16_SERVICE_nrtPS].flowHead;
            while (sFlow != NULL)
            {
                timeDiff = currentTime - sFlow->lastAllocTime;
                if (sFlow->activated && sFlow->bwRequested == 0 &&
                    timeDiff >= 2 * SECOND)
                {
                    numUcastPolls ++;
                    sFlow->lastAllocTime = currentTime;
                }
                sFlow = sFlow->next;
            }

            if (numUcastPolls > 0)
            {
                dataLen = numUcastPolls * sizeof(MacDot16MacHeader);
                numPs = MacDot16PhyBytesToPs(
                            node,
                            dot16,
                            dataLen,
                            &(dot16Bs->ulBurstProfile[
                                DOT16_UIUC_MOST_RELIABLE - 1]),
                            DOT16_UL);
                numPs += dot16Bs->para.sstgInPs;

                if (numPs < dot16Bs->para.requestOppSize)
                {
                    numPs = dot16Bs->para.requestOppSize;
                }

                if (numPs <= numFreePs)
                {
                    ssInfo->requestPsAllocated = (UInt16)numPs;
                    numFreePs -= numPs;
                }
                else if (numFreePs > dot16Bs->para.requestOppSize)
                {
                    ssInfo->requestPsAllocated = (UInt16)numFreePs;
                    numFreePs = 0;
                }
                else
                {
                    // no need to continue
                    ssInfo->requestPsAllocated = 0;
                    break;
                }

                // update # of UL-MAP IE
                if (ssInfo->requestPsAllocated > 0)
                {
                    dot16Bs->numUlMapIEScheduled ++;
                }
            }

            ssInfo = ssInfo->next;
        }
    }

    // 4.3 Schedule data grants for uplink data transmissions
    //     BW is granted service priority based.

    // reset all the ulPsAllocated before moving forward
    for (i = 0; i < DOT16_BS_SS_HASH_SIZE; i ++)
    {
        ssInfo = dot16Bs->ssHash[i];

        while (ssInfo != NULL)
        {
             ssInfo->ulPsAllocated = 0;
             ssInfo = ssInfo->next;
        }
    }

    // 4.3.1 Alloc for management connections. Here we don't distinguish
    //       basic, primary and secondary connections to save overhead.
    for (i = 0; i < DOT16_BS_SS_HASH_SIZE; i ++)
    {
        ssInfo = dot16Bs->ssHash[i];

        // check all SSs with the same hash ID
        while (ssInfo != NULL)
        {
            dataLen = ssInfo->basicMgmtBwRequested +
                      ssInfo->priMgmtBwRequested +
                      ssInfo->secMgmtBwRequested;

            if (numFreePs > 0 && dataLen > 0)
            {
                numPs = MacDot16PhyBytesToPs(
                            node,
                            dot16,
                            dataLen,
                            &(dot16Bs->ulBurstProfile[ssInfo->uiuc - 1]),
                            DOT16_UL);

                if (ssInfo->ulPsAllocated == 0 && numPs > 0)
                {
                    // first one, add sstg
                    numPs += dot16Bs->para.sstgInPs;
                }
                if (numPs > numFreePs)
                {
                    numPs = numFreePs;
                }
                numFreePs -= numPs;
                ssInfo->ulPsAllocated = ssInfo->ulPsAllocated + (UInt16)numPs;
            }

            // reset bw request for mgmt messages
            if (ssInfo->basicMgmtBwRequested > 0)
            {
                ssInfo->lastBasicMgmtBwRequested =
                    ssInfo->basicMgmtBwRequested;
                ssInfo->basicMgmtBwRequested = 0;
            }
            if (ssInfo->priMgmtBwRequested > 0)
            {
                ssInfo->lastPriMgmtBwRequested = ssInfo->priMgmtBwRequested;
                ssInfo->priMgmtBwRequested = 0;
            }
            if (ssInfo->secMgmtBwRequested > 0)
            {
                ssInfo->lastSecMgmtBwRequested = ssInfo->secMgmtBwRequested;
                ssInfo->secMgmtBwRequested = 0;
            }

            ssInfo = ssInfo->next;
        }
    }

    // 4.3.2 Alloc for service flows
    // go through all service types
    for (i = 0; i < DOT16_NUM_SERVICE_TYPES; i ++)
    {
        int numIterations = DOT16_BS_SS_HASH_SIZE;
        int crcSize = 0;
        if (dot16Bs->isCRCEnabled)
        {
            crcSize = DOT16_CRC_SIZE;
        }

        // go through all SSs
        j = dot16Bs->lastSsHashIndex + 1;
        while (numIterations > 0)
        {
            if (j >= DOT16_BS_SS_HASH_SIZE)
            {
                j = 0;
            }

            ssInfo = dot16Bs->ssHash[j];

            while (ssInfo != NULL)
            {
                // go through all service flows from this SS for this
                // service type
                sFlow = ssInfo->ulFlowList[i].flowHead;
                while (sFlow != NULL)
                {
                    // if flow not activated yet, ignore
                    if (!sFlow->activated)
                    {
                        sFlow = sFlow->next;
                        continue;
                    }
                    if (dot16->dot16eEnabled && dot16Bs->isSleepEnabled
                        && ssInfo->isSleepEnabled)
                    {
                        MacDot16ePSClasses* psClass = &ssInfo->psClassInfo[
                            sFlow->psClassType - 1];
                        if (psClass->currentPSClassStatus ==
                            POWER_SAVING_CLASS_ACTIVATE &&
                            psClass->isSleepDuration == TRUE)
                        {
                            sFlow = sFlow->next;
                            break;
                            //continue;
                        }
                    }
                    if (sFlow->serviceType == DOT16_SERVICE_UGS)
                    {
                        // for UGS, no BW request, just allocate
                        timeDiff = currentTime - sFlow->lastAllocTime;
                        dataLen = (int) (sFlow->qosInfo.maxSustainedRate *
                                         timeDiff / SECOND / 8);

                        if (dataLen >= sFlow->qosInfo.minPktSize ||
                            timeDiff >= sFlow->qosInfo.maxLatency)
                        {
                            // now needs to count overhead due to transprot,
                            // IP and MAC headers
                            int actualPktSize = sFlow->qosInfo.minPktSize +
                                sizeof(MacDot16MacHeader) + crcSize;

                            dataLen =
                                (dataLen / sFlow->qosInfo.minPktSize + 1) *
                                 actualPktSize;

                            // need to allocate BW in this frame
                            if (dataLen < actualPktSize)
                            {
                                dataLen = actualPktSize;
                            }
                        }
                        else
                        {
                            dataLen = 0;
                        }
                    }
                    else if (sFlow->serviceType == DOT16_SERVICE_ertPS)
                    {
                        // for ertPS, just allocate as lastRequest
                        timeDiff = currentTime - sFlow->lastAllocTime;
                        dataLen = (int) (sFlow->qosInfo.maxSustainedRate *
                                         timeDiff / SECOND / 8);

                        if (dataLen >= sFlow->qosInfo.minPktSize ||
                            timeDiff >= sFlow->qosInfo.maxLatency)
                        {
                            // now needs to count overhead due to transprot,
                            // IP and MAC headers
                            // dataLen = sFlow->lastBwRequested;
                            // now needs to count overhead due to transprot,
                            // IP and MAC headers

                            int actualPktSize = sFlow->qosInfo.minPktSize +
                                sizeof(MacDot16MacHeader) + crcSize;
                            if (sFlow->lastBwRequested > 0 )
                            {
                                dataLen = sFlow->lastBwRequested;
                            }

                            if (sFlow->lastBwRequested == 0)
                            {
                                sFlow->lastBwRequested = dataLen;
                            }

                            dataLen =
                                (dataLen / sFlow->qosInfo.minPktSize + 1) *
                                 actualPktSize;
                            // need to allocate BW in this frame
                            if (dataLen < actualPktSize)
                            {
                                dataLen = actualPktSize;
                            }
                        }
                        else
                        {
                            dataLen = 0;
                        }
                    }
                    else
                    {
                        dataLen = sFlow->bwRequested;
                    }

                    if (numFreePs > 0 && dataLen > 0)
                    {
                        numPs = MacDot16PhyBytesToPs(
                                    node,
                                    dot16,
                                    dataLen,
                                    &(dot16Bs->ulBurstProfile
                                    [ssInfo->uiuc - 1]),
                                    DOT16_UL);
                        if (ssInfo->ulPsAllocated == 0 && numPs > 0)
                        {
                            // first one, add sstg
                            numPs += dot16Bs->para.sstgInPs;
                        }

                        if (numPs > numFreePs)
                        {
                            numPs = numFreePs;
                            dot16Bs->lastSsHashIndex = j;
                            if (dot16Bs->lastSsHashIndex >=
                                DOT16_BS_SS_HASH_SIZE)
                            {
                                dot16Bs->lastSsHashIndex = 0;
                            }
                            MacDot16DlBurstProfile* burstProfile =
                                &(dot16Bs->dlBurstProfile[ssInfo->uiuc - 1]);
                            dataLen = numPs * (
                                MacDot16PhyBitsPerPs(
                                node,
                                dot16,
                                burstProfile,
                                DOT16_UL)
                                / 8);
                        }

                        if (sFlow->maxBwGranted < dataLen)
                        {
                            sFlow->maxBwGranted = dataLen;
                        }

                        numFreePs -= numPs;
                        ssInfo->ulPsAllocated = ssInfo->ulPsAllocated +
                            (UInt16)numPs;
                        if (numPs > 0 &&
                            (sFlow->serviceType == DOT16_SERVICE_UGS ||
                            sFlow->serviceType == DOT16_SERVICE_ertPS))
                        {
                            // update last allocation time
                            sFlow->lastAllocTime = currentTime;
                        }
                    }

                    // reset bw requested for each ul service flow
                    if (sFlow->bwRequested > 0)
                    {
                        sFlow->lastBwRequested = sFlow->bwRequested;
                        sFlow->bwRequested = 0;
                    }

                    sFlow = sFlow->next;
                } // service flow list

                ssInfo = ssInfo->next;
            } // SS with same hash index

            j ++;
            numIterations --;
        } // SS hash table
    } // service type

    // to update the UL MAP IE
    for (i = 0; i < DOT16_BS_SS_HASH_SIZE; i ++)
    {
        ssInfo = dot16Bs->ssHash[i];

        while (ssInfo != NULL)
        {
            if (ssInfo->ulPsAllocated > 0)
            {
                dot16Bs->numUlMapIEScheduled ++;
            }
            ssInfo = ssInfo->next;
        }
    }

    if (DEBUG_BWREQ)
    {
        for (i = 0; i < DOT16_BS_SS_HASH_SIZE; i ++)
        {
            ssInfo = dot16Bs->ssHash[i];

            while (ssInfo != NULL)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("Alloc BW PS %d for range, PS %d for bw request, "
                       "PS %d UIUC %d for data to ss w/ baic cid %d\n",
                       ssInfo->rangePsAllocated,
                       ssInfo->requestPsAllocated,
                       ssInfo->ulPsAllocated,
                       ssInfo->uiuc,
                       ssInfo->basicCid);

                ssInfo = ssInfo->next;
            }
        }
    }
}

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
                                  clocktype dlDuration)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;

    // check if duration of DL sub-frame is changed
    if (dot16Bs->dlDuration != dlDuration || dot16Bs->dlAllocMap == NULL)
    {
        // changed, re-calculated all variables and re-alloc memory
        dot16Bs->dlBurstIndex = 0;
        dot16Bs->dlDuration = dlDuration;
        dot16Bs->numDlPs = (UInt16) (dlDuration / dot16->dlPsDuration);

        // free old memory if any
        if (dot16Bs->dlAllocMap != NULL)
        {
            MEM_free(dot16Bs->dlAllocMap);
        }

        // alloc memory for the allocation map which records how many
        // subchannels have been used for each PS
        dot16Bs->dlAllocMap = (char*) MEM_malloc(dot16Bs->numDlPs);
        ERROR_Assert(dot16Bs->dlAllocMap != NULL,
                     "MAC 802.16: Out of memory!");

        // clean to 0, which means available
        memset(dot16Bs->dlAllocMap, 0, dot16Bs->numDlPs);
    }
    else
    {
        // sub-frame length is same as last time, just clear the map
        dot16Bs->dlBurstIndex = 0;
        memset(dot16Bs->dlAllocMap, 0, dot16Bs->numDlPs);
    }
}

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
// RETURN     :: int : Actualy size in bytes allocated
// **/
int MacDot16SchAllocDlBurst(Node* node,
                            MacDataDot16* dot16,
                            int sizeInBytes,
                            unsigned char diuc,
                            Dot16BurstInfo* burstInfo)
{
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    MacDot16DlBurstProfile* burstProfile;
    int sizeInPs;
    int numSubchannels;
    int numColNeeded;
    BOOL allocated = FALSE;
    int i;

    // Some general rules:
    // 1. DL allocation is PS based, and 1 DL PS is 2 OFDM symbols
    // 2. The first symbol on all subchannels are for preamble
    // 3. Symbol 1 and symbol 2 on subchannel 0 and 1 are for FCH
    // 4. DL-MAP and UL-MAP are allocated the subchannel first to make
    //    sure they are the first ones on time axis
    // 5. All other DL bursts must be after DL-MAP on time axis

    memset(burstInfo, 0, sizeof(Dot16BurstInfo));

    numSubchannels = MacDot16PhyGetNumSubchannels(node, dot16);

    // get the corresponding burst profile
    burstProfile = &(dot16Bs->dlBurstProfile[diuc]);
    burstInfo->modCodeType = burstProfile->ofdma.fecCodeModuType;

    // convert requested bytes to number of PSs
    sizeInPs = MacDot16PhyBytesToPs(node,
                                    dot16,
                                    sizeInBytes,
                                    burstProfile,
                                    DOT16_DL);

    if (dot16Bs->dlBurstIndex == 0)
    {
        // first burst, we include FCH in the burst of DL-MAP
        sizeInPs += 2;
    }

    if (sizeInPs % numSubchannels == 0)
    {
        numColNeeded = sizeInPs / numSubchannels;
    }
    else
    {
        numColNeeded = sizeInPs / numSubchannels + 1;
    }

    if (dot16Bs->dlBurstIndex == 0)
    {
        // first burst is for DL-MAP & UL-MAP. We include FCH in this burst
        burstInfo->subchannelOffset = 0;
        burstInfo->symbolOffset = 1; // symbol 0 is for preamble

        if (numColNeeded > 1)
        {
            burstInfo->numSubchannels = numSubchannels;
        }
        else
        {
            burstInfo->numSubchannels = sizeInPs;
        }

        burstInfo->numSymbols = numColNeeded *
                                MacDot16PhySymbolsPerPs(DOT16_DL);

        ERROR_Assert(numColNeeded <= dot16Bs->numDlPs,
                     "MAC802.16: DL subframe is too small!");

        // mark these symbols are used on all subchannels
        for (i = 0; i < numColNeeded; i ++)
        {
            dot16Bs->dlAllocMap[i] = (UInt8)numSubchannels;
        }

        allocated = TRUE;
    }
    else
    {
        // other bursts
        if (numColNeeded <= 1)
        {
            // needs less than one column, so we check first to see if
            // can reuse some leftover subchannels on previous columns
            for (i = 0; i < dot16Bs->numDlPs; i ++)
            {
                if ((numSubchannels - dot16Bs->dlAllocMap[i]) > sizeInPs)
                {
                    // found one column can accomodate this burst
                    break;
                }
            }

            if (i < dot16Bs->numDlPs)
            {
                burstInfo->subchannelOffset = dot16Bs->dlAllocMap[i];
                burstInfo->numSubchannels = sizeInPs;
                burstInfo->symbolOffset =
                    i * MacDot16PhySymbolsPerPs(DOT16_DL) + 1;
                burstInfo->numSymbols = MacDot16PhySymbolsPerPs(DOT16_DL);

                dot16Bs->dlAllocMap[i] = dot16Bs->dlAllocMap[i] +
                    (UInt8)sizeInPs;
                allocated = TRUE;

            }
            else
            {
                allocated = FALSE;

                burstInfo->subchannelOffset = 0;
                burstInfo->numSubchannels = 1;
                burstInfo->symbolOffset = 0;
                burstInfo->numSymbols = 1;
            }
        }
        else
        {
            // firstly find a column completely empty
            for (i = 0; i < dot16Bs->numDlPs; i ++)
            {
                if (dot16Bs->dlAllocMap[i] == 0)
                {
                    break;
                }
            }
            if ((i + numColNeeded) <= dot16Bs->numDlPs)
            {
                allocated = TRUE;

                burstInfo->subchannelOffset = 0;
                burstInfo->numSubchannels = numSubchannels;
                burstInfo->symbolOffset =
                        MacDot16PhySymbolsPerPs(DOT16_DL) * i + 1;
                burstInfo->numSymbols =
                        MacDot16PhySymbolsPerPs(DOT16_DL) * numColNeeded;
                Int32 j = i + numColNeeded;
                if (sizeInPs % numSubchannels == 0)
                {
                    for (; i < j; i++)
                    {
                        dot16Bs->dlAllocMap[i] = (UInt8)numSubchannels;
                    }
                }
                else
                {
                    for (; i < j - 1; i ++)
                    {
                        dot16Bs->dlAllocMap[i] = (UInt8)numSubchannels;
                    }
                    dot16Bs->dlAllocMap[i] = sizeInPs % numSubchannels;
                }

            }
            else
            {
                allocated = FALSE;

                burstInfo->subchannelOffset = 0;
                burstInfo->numSubchannels = 1;
                burstInfo->symbolOffset = 0;
                burstInfo->numSymbols = 1;
            }
        }
    }
    if (allocated)
    {
    burstInfo->burstIndex = dot16Bs->dlBurstIndex ++;

    if (DEBUG_BURST)
    {
        printf("Node%d allocate a DL burst for %d bytes (%d PS):\n",
               node->nodeId, sizeInBytes, sizeInPs);
        printf("    burstIndex = %d\n", burstInfo->burstIndex);
        printf("    modu encoding type = %d\n", burstInfo->modCodeType);
        printf("    subchannelOffset = %d\n", burstInfo->subchannelOffset);
        printf("    numSubchannels = %d\n", burstInfo->numSubchannels);
        printf("    symbolOffset = %d\n", burstInfo->symbolOffset);
        printf("    numSymbols = %d\n", burstInfo->numSymbols);
    }
    }

    return allocated;
}
