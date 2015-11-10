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
#include "node.h"
#include "partition.h"

#include "api.h"
#include "mac_cellular.h"
#include "layer2_umts.h"
#include "layer2_umts_mac.h"
#include "layer2_umts_rlc.h"
#include "layer2_umts_mac_phy.h"
#include "phy_umts.h"
#include "layer3_umts.h"
#include "umts_constants.h"
#include <algorithm>
#include <list>
#include <deque>

#define DEBUG_SPECIFIED_NODE_ID 0 // DEFAULT shoud be 0
#define DEBUG_SINGLE_NODE   (node->nodeId == DEBUG_SPECIFIED_NODE_ID)
#define DEBUG_MAC_GENERAL 0
#define DEUBG_MAC_TIMER 0
#define DEBUG_MAC_CHANNEL 0
#define DEBUG_MAC_INTERLAYER 0
#define DEBUG_MAC_RB (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_MAC_RACH 0
#define DEBUG_MAC_DATA (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_MAC_HSDPA (0 || (0 &&  DEBUG_SINGLE_NODE))

////////////////////////////////////////////////////////////////////////////
// Utility functions
////////////////////////////////////////////////////////////////////////////

// /**
// FUNCTION   :: UmtsMacGetSubLayerData
// LAYER      :: UMTS L2 MAC sublayer
// PURPOSE    :: get the pointer of the MAC data structure
// PARAMETERS ::
// + node            : Node*    :   Pointer to node.
// + interfaceIndex  : UInt32   :   interface index
// RETURN     :: umtsMacDat : UmtsMacData* : pointer to the MAC data
// **/
static
UmtsMacData* UmtsMacGetSubLayerData(Node* node, UInt32 interfaceIndex)
{
    MacCellularData* macCellularData;
    CellularUmtsLayer2Data *layer2Data;

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;

    layer2Data = macCellularData->cellularUmtsL2Data;

    return (layer2Data->umtsMacData);
}

// /**
// FUNCTION   :: UmtsMacGetLogChInfoFromList
// LAYER      :: UMTS L2 MAC sublayer
// PURPOSE    :: Get the transport channel info from the channel info list
// PARAMETERS ::
// + node        : Node*                     : Pointer to the node
// + chList      :std::list<UmtsLogChInfo*>* : Point to channel List
// + chType      : UmtsTransportChannelType  : transport channel type
// + chId        : unsigned int              : channel Idetifier
// RETURN     :: UmtsLogChInfo* : pointer to the channel info
// **/
static
UmtsLogChInfo* UmtsMacGetLogChInfoFromList(
                    Node* node,
                    std::list<UmtsLogChInfo*>* chList,
                    UmtsLogicalChannelType chType,
                    unsigned int chId)
{
    UmtsLogChInfo* chInfo = NULL;
    std::list<UmtsLogChInfo*>::iterator it;

    for (it = chList->begin();
        it != chList->end();
        it ++)
    {
        if ((*it)->logChType == chType &&
            (*it)->logChId == chId)
        {
            break;
        }
    }

    if (it != chList->end())
    {
        chInfo = *it;
    }

    return chInfo;
}

#if 0
// /**
// FUNCTION   :: UmtsMacNodeBGetUeInfoFromList
// LAYER      :: UMTS L2 MAC sublayer
// PURPOSE    :: Get the ue info from the ue info list at Node B
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// ueInfoList  : std::list <UmtsMacNodeBUeInfo*>* : Point to UE Info List
// priSc       : unsinged int              : UE's primary scrambling code
// RETURN     :: UmtsMacNodeBUeInfo*       : pointer to the UE info
// **/
static
UmtsMacNodeBUeInfo* UmtsMacNodeBGetUeInfoFromList(
                        Node* node,
                        std::list <UmtsMacNodeBUeInfo*>* ueInfoList,
                        int priSc)
{
    UmtsMacNodeBUeInfo* ueInfo = NULL;
    std::list<UmtsMacNodeBUeInfo*>::iterator it;
    for (it = ueInfoList->begin();
        it !=  ueInfoList->end();
        it ++)
    {
        if ((*it)->priSc == priSc)
        {
            break;
        }
    }

    if (it != ueInfoList->end())
    {
        ueInfo = *it;
    }

    return ueInfo;
}
#endif

// /**
// FUNCTION   :: UmtsMacNodeBInitUeNodeBInfo
// LAYER      :: UMTS MAC
// PURPOSE    :: Inititate a UE Info data in Nodeb
// PARAMETERS ::
// + node     : Node*             : Pointer to node.
// + ueId     : NodeId            : The UE Id
// + priSc    : unsigned int      : Primary scrambling code
// RETURN     :: UmtsMacNodeBUeInfo* : The allocated nodeBUeInfo pointer
// **/
static
UmtsMacNodeBUeInfo* UmtsMacNodeBInitUeNodeBInfo(
                        Node* node,
                        NodeId ueId,
                        unsigned int priSc)
{
    UmtsMacNodeBUeInfo* nodeBUeInfo =
        (UmtsMacNodeBUeInfo*)MEM_malloc(sizeof(UmtsMacNodeBUeInfo));

    memset(nodeBUeInfo, 0, sizeof(UmtsMacNodeBUeInfo));
    nodeBUeInfo->ueId = ueId;
    nodeBUeInfo->priSc = priSc;
    nodeBUeInfo->ueLogTrPhMap = new std::list<UmtsRbLogTrPhMapping*>;
    nodeBUeInfo->txBurstBuf = new std::deque<Message*>;
    nodeBUeInfo->rxBurstBuf = new std::deque<Message*>;

    return nodeBUeInfo;
}

// /**
// FUNCTION   :: UmtsMacNodeBFinalizeUeNodeBInfo
// LAYER      :: UMTS MAC
// PURPOSE    :: Finalize a UE Info data in Nodeb
// PARAMETERS ::
// + node       : Node*                 : Pointer to node.
// + nodeBUeInfo: UmtsMacNodeBUeInfo*   : Pointer to the nodeBInfo
//                                        to be finalized
// RETURN       :: void      : NULL
// **/
static
void UmtsMacNodeBFinalizeUeNodeBInfo(
        Node* node,
        UmtsMacNodeBUeInfo* nodeBUeInfo)
{
    UmtsClearStlContDeep(nodeBUeInfo->ueLogTrPhMap);
    delete nodeBUeInfo->ueLogTrPhMap;
    std::for_each(nodeBUeInfo->txBurstBuf->begin(),
                  nodeBUeInfo->txBurstBuf->end(),
                  UmtsFreeStlMsgPtr(node));
    delete nodeBUeInfo->txBurstBuf;
    std::for_each(nodeBUeInfo->rxBurstBuf->begin(),
                  nodeBUeInfo->rxBurstBuf->end(),
                  UmtsFreeStlMsgPtr(node));
    delete nodeBUeInfo->rxBurstBuf;
    MEM_free(nodeBUeInfo);
}

// /**
// FUNCTION   :: UmtsMacNodeBGetUeInfoFromList
// LAYER      :: UMTS L2 MAC sublayer
// PURPOSE    :: Get the ue info from the ue info list at Node B
// PARAMETERS ::
// + node        : Node*             : Pointer to the node
// + ueInfoList  : std::list <UmtsMacNodeBUeInfo*>* : Point to UE Info List
// + ueId        : NodeAddress       : UE's primary scrambling code
// RETURN     :: UmtsMacNodeBUeInfo* : pointer to the UE info
// **/
static
UmtsMacNodeBUeInfo* UmtsMacNodeBGetUeInfoFromList(
                        Node* node,
                        std::list <UmtsMacNodeBUeInfo*>* ueInfoList,
                        NodeAddress ueId)
{
    UmtsMacNodeBUeInfo* ueInfo = NULL;
    std::list<UmtsMacNodeBUeInfo*>::iterator it;
    for (it = ueInfoList->begin();
        it !=  ueInfoList->end();
        it ++)
    {
        if ((*it)->ueId == ueId)
        {
            break;
        }
    }

    if (it != ueInfoList->end())
    {
        ueInfo = *it;
    }

    return ueInfo;
}

// /**
// FUNCTION   :: UmtsMacCalculateTrChWeight
// LAYER      :: UMTS LAYER2 MAC
// PURPOSE    :: calcualte the TrCh weight.
// PARAMETERS ::
// + node       : Node*                   : Pointer to the node structure
// + entityInfo : UmtsRlcEntityInfoToMac  : RLC entity information
// RETURN     :: void : NULL
// **/
static
double UmtsMacCalculateTrChWeight(Node* node,
                                  UmtsRlcEntityInfoToMac  entityInfo)
{
    // the following just give an exmaple how to calculate the weight
    double alpha = 0.5;
    double beta = 0.5;

    if (entityInfo.bufOccupancy > 0)
    {
        return (alpha * (double)entityInfo.bufOccupancy /
                UMTS_RLC_DEF_MAXBUFSIZE +
                beta * (double)(node->partitionData->maxSimClock -
                entityInfo.lastAccessTime) / TIME_ReturnMaxSimClock(node));
    }
    else
    {
        return 0;
    }
}

//--------------------------------------------------------------------------
// UE Handler part
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: UmtsMacUeUpdateTrChWeight
// LAYER      :: UMTS LAYER2 MAC sublayer at UE
// PURPOSE    :: Handle timers out messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + mac              : UmtsMacData*      : Mac Data
// RETURN     :: void : NULL
// **/
static
void UmtsMacUeUpdateTrChWeight(Node* node,
                              UInt32 interfaceIndex,
                              UmtsMacData* mac)
{

    if (!mac->ueMacVar->ueTrChList->empty())
    {
        UeTrChList::iterator itTr;

        for (itTr = mac->ueMacVar->ueTrChList->begin();
             itTr != mac->ueMacVar->ueTrChList->end();
             itTr ++)
        {
            if ((*itTr)->trChType == UMTS_TRANSPORT_DCH
                && (*itTr)->logPrio > (char)UMTS_QOS_CLASS_STREAMING)
            {
                // only weight DCHs for background and interactive apps
                std::list <UmtsRbLogTrPhMapping*>::iterator itMap;
                std::list <UmtsRbLogTrPhMapping*>* mapList;
                unsigned char rbId = 0;
                mapList = mac->ueMacVar->ueLogTrPhMap;

                for (itMap = mapList->begin();
                     itMap != mapList->end();
                     itMap ++)
                {
                    if ((*itMap)->ulTrChId == (*itTr)->trChId &&
                        (*itMap)->ulTrChType == (*itTr)->trChType)
                    {
                        UmtsRlcEntityInfoToMac  entityInfo;

                        rbId  = (*itMap)->rbId;
                        if (UmtsRlcIndicateStatusToMac(
                            node,
                            interfaceIndex,
                            rbId,
                            entityInfo))
                        {
                            (*itTr)->trChWeight =
                                UmtsMacCalculateTrChWeight(
                                    node,
                                    entityInfo);
                        }
                        else
                        {
                            (*itTr)->trChWeight = 0.0;
                        }

                        if (DEBUG_MAC_DATA)
                        {
                            UmtsLayer2PrintRunTimeInfo(
                                node,
                                interfaceIndex,
                                UMTS_LAYER2_SUBLAYER_MAC);
                            printf("weight for rb %d is %f\n",
                                rbId, (*itTr)->trChWeight);
                        }
                    } // if ulTrId, ulTrType
                } // for (itMap)
            } // if UMTS_TRANSPORT_DCH && UMTS_QOS_CLASS_STREAMING
        } // for (itTr)
    } // if (!mac->ueMacVar->ueTrChList->empty())
}

// /**
// FUNCTION   :: UmtsMacUeHandleSlotTimer
// LAYER      :: UMTS LAYER2 MAC sublayer at UE
// PURPOSE    :: Handle time out messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + mac              : UmtsMacData*      : Mac Data
// RETURN     :: void : NULL
// **/
static
void UmtsMacUeHandleSlotTimer(Node* node,
                              UInt32 interfaceIndex,
                              UmtsMacData* mac)
{
    Message* msgToSend = NULL;

    // order the transport Channel List
    if (mac->chipRateType == UMTS_CHIP_RATE_384 &&
        mac->slotIndex == 0)
    {
        // for each DCH transport channel,
        // calculate the latest priority and weight
        // then order them
        if (!mac->ueMacVar->ueTrChList->empty())
        {
            UmtsMacUeUpdateTrChWeight(
                node,
                interfaceIndex,
                mac);

#ifdef _WIN64
            mac->ueMacVar->ueTrChList->sort(
                std::greater<UmtsTransChInfo*>());
#else
            mac->ueMacVar->ueTrChList->sort(UmtsTransChInfoPtrLess());
#endif
        }
        // other types channel give 0 value of weight and prio
    }
    else
    {
        // not support at this time
    }

    // check each transport Channel's TTI to
    if (!mac->ueMacVar->ueTrChList->empty())
    {
        // msgToSend will be constructed here
        // go each TrCh, if TTI meet, retrieve PDU, add CRC
        // make CCTrCh
        // link them togetehr as a msg list
        UeTrChList::iterator itTr;

        for (itTr = mac->ueMacVar->ueTrChList->begin();
             itTr != mac->ueMacVar->ueTrChList->end();
             itTr ++)
        {
            if ((*itTr)->trChType == UMTS_TRANSPORT_RACH)
            {
                // rach channel uses a different procedure
                continue;
            }
            (*itTr)->slotCounter ++;

            if ((*itTr)->slotCounter >= (*itTr)->TTI)
            {
                (*itTr)->slotCounter = 0;

                // time to check if any packets to send from
                // this transport channel
                // local Mapping first
                std::list <UmtsRbLogTrPhMapping*>::iterator itMap;
                std::list <UmtsRbLogTrPhMapping*>* mapList;

                mapList = mac->ueMacVar->ueLogTrPhMap;

                for (itMap = mapList->begin();
                     itMap != mapList->end();
                     itMap ++)
                {
                    if ((*itMap)->ulTrChId == (*itTr)->trChId &&
                        (*itMap)->ulTrChType == (*itTr)->trChType)
                    {
                        unsigned char rbId = 0;
                        int numPdus;
                        int pduSize;
                        std::list<Message*> pduList;
                        pduList.clear();
                        UmtsPhysicalChannelType ulPhChType = 
                            UMTS_PHYSICAL_INVALID_TYPE;

                        unsigned int            ulPhChId = 0;
                        std::list<Message*> pduListPerRb;
                        unsigned char ratePara = 1;
                        unsigned char crcSize = 0;

                        // find the map
                        rbId = (*itMap)->rbId;

                        // Get transport format
                        pduSize = (*itMap)->transFormat.GetTransBlkSize() / 8;
                        numPdus = (int)((*itMap)->
                            transFormat.GetTransBlkSetSize() / 8 / pduSize);

                        ratePara = UmtsGetRateParaFromCodingType(
                                       (*itMap)->transFormat.codingType);
                        crcSize = (unsigned char)(
                                   (*itMap)->transFormat.crcSize);

                        if (rbId == UMTS_CCCH_RB_ID ||
                            rbId >= UMTS_BCCH_RB_ID)
                        {
                            // for special RB
                            numPdus = 5;
                        }
                        else if (rbId > 4 && 
                            (*itTr)->trChType == UMTS_TRANSPORT_ACKCH)
                        {
                            numPdus = 44;
                        }
                        else if (rbId > 4 &&
                            (*itTr)->trChType == UMTS_TRANSPORT_DCH)
                        {
                            // Regular RB
                            if (DEBUG_MAC_DATA)
                            {
                                UmtsLayer2PrintRunTimeInfo(
                                    node, 
                                    interfaceIndex, 
                                    UMTS_LAYER2_SUBLAYER_MAC);
                                printf("\ttest rb %d\n", rbId);
                            }

                            // if it is for conversational/streaming services
                            // continue
                            // otherwise have to make sure there is enough 
                            // capacity to
                            // add some more bits from the 
                            // background/interactive services
                            if ((*itTr)->logPrio > 
                                (char)UMTS_QOS_CLASS_STREAMING)
                            {
                                int numBitsAllowedInOneFrame;
                                int numBitsInBuffer;
                                signed char slotFormat = -1;

                                // get the slotFormat currently used
                                UmtsLayer3GetUlPhChDataBitRate(
                                    UmtsUeGetSpreadFactorInUse(
                                    node, interfaceIndex),
                                    &slotFormat);

                                // calculate the max capacity in one frame
                                numBitsAllowedInOneFrame =
                                    umtsUlDpdchFormat[slotFormat].numData * 
                                    15 *
                                    UmtsMacPhyUeGetDpdchNumber(
                                                   node, interfaceIndex);

                                numBitsInBuffer =
                                    UmtsMacPhyUeGetDataBitSizeInBuffer(
                                        node,
                                        interfaceIndex);
                                if (numBitsAllowedInOneFrame >
                                    numBitsInBuffer)
                                {
                                    // put as many bits as possible in
                                    pduSize = 
                                        (*itMap)->
                                        transFormat.GetTransBlkSize() / 8; 
                                    // use the minimum one
                                    numPdus =
                                        (numBitsAllowedInOneFrame -
                                         numBitsInBuffer) / ratePara /
                                         (pduSize * 8);
                                    if (numPdus == 0)
                                    {
                                        numPdus = 1;
                                    }
                                }
                                else
                                {
                                    // no more
                                    numPdus = 0;
                                    pduSize = 96 / 8;
                                }
                            }
                        }
                        else if (rbId > 4 &&
                                (*itTr)->trChType == UMTS_TRANSPORT_HSDSCH)
                        {
                            // for HSDPA ACK
                            // needs to report CQI in HS-DPCCH
                            UmtsMacPhyUeSetHsdpaCqiReportInd(
                                node,
                                interfaceIndex);

                            numPdus = 1;
                        }

                       // get the packet
                        if (numPdus > 0 )
                        {
                            UmtsRlcHasPacketSentToMac(
                                node,
                                interfaceIndex,
                                rbId,
                                numPdus,
                                pduSize,
                                pduListPerRb,
                                (*itTr)->ueId);
                        }
                        if (!pduListPerRb.empty())
                        {
                            pduList.insert(pduList.end(),
                                           pduListPerRb.begin(),
                                           pduListPerRb.end());

                            // if multipel PhChs, get each PhCh's Id & Type
                            ulPhChType = (*itMap)->ulPhChType;
                            ulPhChId = (*itMap)->ulPhChId;
                        }

                        // put it to the corresponding PhyCh Tx queue
                        if (!pduList.empty())
                        {
                            // if multiple chs, alloc pdus among the PhChs
                            UmtsMacPhyUeEnqueuePacketToPhChTxBuffer(
                                node,
                                interfaceIndex,
                                ulPhChType,
                                ulPhChId,
                                pduList);

                            if (ulPhChType == UMTS_PHYSICAL_DPDCH)
                            {
                                int numBits = 0;

                                std::list<Message*>::iterator msgIt;

                                // update numBitInBuffer
                                // count the msg size
                                for (msgIt = pduList.begin();
                                    msgIt != pduList.end();
                                    msgIt ++)
                                {

                                    UmtsRlcPackedPduInfo* info = 
                                        (UmtsRlcPackedPduInfo*)
                                             MESSAGE_ReturnInfo((*msgIt));
                                    numBits += info->pduSize * 8;

                                    // CRC
                                    numBits += (int)crcSize;
                                    // no MAC header is needed
                                }

                                numBits *= ratePara;

                                // set the numDataBitInBUffer
                                UmtsMacPhyUeSetDataBitSizeInBuffer(
                                    node,
                                    interfaceIndex,
                                    UmtsMacPhyUeGetDataBitSizeInBuffer(
                                        node,
                                        interfaceIndex) + numBits);

                                if (DEBUG_MAC_DATA)
                                {
                                    UmtsLayer2PrintRunTimeInfo(
                                        node, interfaceIndex, 
                                        UMTS_LAYER2_SUBLAYER_MAC);
                                    printf("Put  %d  bits %d from rb %d "
                                        "into physical queue now %d\n",
                                        numBits, numBits / ratePara, rbId,
                                        UmtsMacPhyUeGetDataBitSizeInBuffer(
                                            node,
                                            interfaceIndex));
                                }
                            } // if (ulPhChType == UMTS_PHYSICAL_DPDCH)
                        } // if (!pduList.empty())
                    } // if trChType && trchId
                } // for (itMap)
            } // if ((*itTr)->slotCounter >= (*itTr)->TTI)
        } // for (itTr)
    } // if (!mac->ueMacVar->ueTrChList->empty())

    // becasue interleave/deinterleave is not implemented
    // the transission is only happended once per frame rather that
    // per slot
    // for the ue's slotIndex
    if (mac->chipRateType == UMTS_CHIP_RATE_384 &&
        mac->slotIndex == 0)
    {
        int numDataBitInBuffer;
        signed char slotFormatInUse = -1;

        // adjust the transmission power before each transmission
        UmtsMacPhyUeCheckUlPowerControlAdjustment(node, interfaceIndex);

        UmtsMacUeTransmitMacBurst(node, interfaceIndex, mac, msgToSend);

#ifdef PARALLEL //Parallel
        PARALLEL_SetLookaheadHandleEOT(
            node,
            mac->lookaheadHandle,
            node->getNodeTime() + 
                mac->slotDuration * 
                UMTS_RADIO_FRAME_DURATION_384);
#endif //endParallel

        numDataBitInBuffer =
            UmtsMacPhyUeGetDataBitSizeInBuffer(
                node,
                interfaceIndex);
        if (numDataBitInBuffer > 0)
        {
            // get the slotFormat currently used
            UmtsLayer3GetUlPhChDataBitRate(
                UmtsUeGetSpreadFactorInUse(
                node, interfaceIndex),
                &slotFormatInUse);

            // assume transmit every frame
            numDataBitInBuffer -=
                umtsUlDpdchFormat[slotFormatInUse].numData * 15 *
                UmtsMacPhyUeGetDpdchNumber(node, interfaceIndex);

            if (numDataBitInBuffer <= 0)
            {
                numDataBitInBuffer = 0;
            }

            // update the numDataBitInBuffer
            UmtsMacPhyUeSetDataBitSizeInBuffer(
                node,
                interfaceIndex,
                numDataBitInBuffer);

            if (DEBUG_MAC_DATA)
            {
                UmtsLayer2PrintRunTimeInfo(
                    node, interfaceIndex, UMTS_LAYER2_SUBLAYER_MAC);
                printf("transmit  %d  bits from physical queue now %d\n",
                    umtsUlDpdchFormat[slotFormatInUse].numData * 15 *
                    UmtsMacPhyUeGetDpdchNumber(node, interfaceIndex),
                    UmtsMacPhyUeGetDataBitSizeInBuffer(
                        node,
                        interfaceIndex));
            }
        }
    }
    else
    {
        // not supported
    }

    // support 3.84M only
    mac->slotIndex = (mac->slotIndex + 1) % UMTS_RADIO_FRAME_DURATION_384;
} // UeHandleSlotTimer

// /**
// FUNCTION   :: UmtsMacUeRachPersistenceTest
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Handle RACH access state machine transition
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + ueMacVar         : UmtsMacUeData*    : ue mac var
// RETURN     :: NULL : void
// **/
static
void UmtsMacUeRachPersistenceTest(Node* node,
                                 UInt32 interfaceIndex,
                                 UmtsMacData* umtsMacData,
                                 UmtsMacUeData* ueMacVar)
{
    UmtsMacEntityRachDynamicInfo* activeInfo;
    UmtsMacEntityRachConfigInfo* cfgInfo;

    if (DEBUG_MAC_RACH)
    {
        UmtsLayer2PrintRunTimeInfo(
            node, interfaceIndex, UMTS_LAYER2_SUBLAYER_MAC);
        printf("Persitence Test\n");
    }

    activeInfo = ueMacVar->cshmEntity->activeRachInfo;
    cfgInfo = ueMacVar->cshmEntity->rachConfig;

    if (++ activeInfo->retryCounter >  cfgInfo->Mmax)
    {
        // send RRC CMAC_STATUS_IND
        // indication info
        // TODO, replace NULL with indication info
        UmtsLayer3HandleInterLayerCommand(
            node, interfaceIndex, UMTS_CMAC_STATUS_IND, NULL);

        activeInfo->state = UMTS_MAC_RACH_IDLE;

        return;
    }
    else
    {
        // start T2
        if (activeInfo->timerT2 != NULL)
        {
            MESSAGE_CancelSelfMsg(
                node,
                activeInfo->timerT2);
            activeInfo->timerT2 = NULL;
        }
        activeInfo->timerT2 = UmtsLayer2InitTimer(
                                 node, interfaceIndex,
                                 UMTS_LAYER2_SUBLAYER_MAC,
                                 UMTS_MAC_RACH_T2, 0, NULL);

        MESSAGE_Send(
            node,
            activeInfo->timerT2,
            UMTS_MAC_RACH_T2_DEFAULT_TIME);

        // generate a random number
        double perTest;
        MacCellularData* macCellularData;
        macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;
        perTest = RANDOM_erand(macCellularData->randSeed);

        if (perTest <=  activeInfo->persistFactor)
        {
            if (DEBUG_MAC_RACH)
            {
                printf("    Persistence Test Passed\n");
            }
            // test successed
            // send PHY_ACCESS_REQ to PHY
            // build access req info
            // Put the data part in the tx queue
            UmtsPhyAccessReqInfo accessReqInfo;

            accessReqInfo.ascIndex = activeInfo->activeAscIndex;
            accessReqInfo.maxRetry = cfgInfo->Mmax;
            accessReqInfo.subChAssigned = activeInfo->assignedSubCh;
            accessReqInfo.sigStartIndex = activeInfo->sigStartIndex;
            accessReqInfo.sigEndIndex = activeInfo->sigEndIndex;
            UmtsMacPhyHandleInterLayerCommand(
                node, interfaceIndex, UMTS_PHY_ACCESS_REQ, &accessReqInfo);
            activeInfo->state = UMTS_MAC_RACH_WAIT_AICH;
        }
        else
        {
            if (DEBUG_MAC_RACH)
            {
                printf("    Persistence Test Failed\n");
            }
            activeInfo->state = UMTS_MAC_RACH_WAIT_T2;
        }
    }
}

// /**
// FUNCTION   :: UmtsMacUeHandleRachStateTransitionEvent
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Handle RACH access state machine transition
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + ueMacVar         : UmtsMacUeData*    : ue mac var
// + eventType        : UmtsMacRachStateTransitEvent : transition event
// RETURN     :: NULL : void
// **/
static
void UmtsMacUeHandleRachStateTransitionEvent(
                         Node* node,
                         UInt32 interfaceIndex,
                         UmtsMacData* umtsMacData,
                         UmtsMacUeData* ueMacVar,
                         UmtsMacRachStateTransitEvent eventType)
{
    ERROR_Assert(ueMacVar->cshmEntity != NULL,
                 " no cshm MAC entity at this node");
    if (DEBUG_MAC_RACH)
    {
        UmtsLayer2PrintRunTimeInfo(
            node, interfaceIndex, UMTS_LAYER2_SUBLAYER_MAC);
    }

    if (eventType == UMTS_MAC_RACH_EVENT_T2Expire)
    {
        ueMacVar->cshmEntity->activeRachInfo->timerT2 = NULL;
    }
    else if (eventType == UMTS_MAC_RACH_EVENT_TboExpire)
    {
        ueMacVar->cshmEntity->activeRachInfo->timerBO = NULL;
    }

    switch(ueMacVar->cshmEntity->activeRachInfo->state)
    {
        case UMTS_MAC_RACH_IDLE:
        {
            if (eventType == UMTS_MAC_RACH_EVENT_DataToTx)
            {
                if (DEBUG_MAC_RACH)
                {
                    printf("rcvd DataToSend in IDLE sate\n");
                }
                ueMacVar->cshmEntity->activeRachInfo->state =
                        UMTS_MAC_RACH_PERSISTENCE_TEST;

                // persistence test
                UmtsMacUeRachPersistenceTest(
                    node, interfaceIndex, umtsMacData, ueMacVar);
            }

            break;
        }
        case UMTS_MAC_RACH_PERSISTENCE_TEST:
        {
            // it is a non-block state
            break;
        }
        case UMTS_MAC_RACH_WAIT_T2:
        {
            if (eventType == UMTS_MAC_RACH_EVENT_T2Expire)
            {
                if (DEBUG_MAC_RACH)
                {
                    printf("rcvd T2Expire in WAIT_T2 sate\n");
                }
                ueMacVar->cshmEntity->activeRachInfo->state =
                        UMTS_MAC_RACH_PERSISTENCE_TEST;

                // persistence test
                UmtsMacUeRachPersistenceTest(
                    node, interfaceIndex, umtsMacData, ueMacVar);
            }

            break;
        }
        case UMTS_MAC_RACH_BACKOFF:
        {
            if (eventType == UMTS_MAC_RACH_EVENT_T2Expire)
            {
                // do nothing
                if (DEBUG_MAC_RACH)
                {
                    printf("rcvd T2Expire in BACKOFF sate\n");
                }
            }
            else if (eventType == UMTS_MAC_RACH_EVENT_TboExpire)
            {
                if (DEBUG_MAC_RACH)
                {
                    printf("rcvd TBOExpire in BACKOFF sate\n");
                }

                // cancel T2 if still running
                if (ueMacVar->cshmEntity->activeRachInfo->timerT2)
                {
                    MESSAGE_CancelSelfMsg(
                        node,
                        ueMacVar->cshmEntity->activeRachInfo->timerT2);
                    ueMacVar->cshmEntity->activeRachInfo->timerT2 = NULL;
                }

                ueMacVar->cshmEntity->activeRachInfo->state =
                        UMTS_MAC_RACH_PERSISTENCE_TEST;
                
                // persistence test
                UmtsMacUeRachPersistenceTest(
                    node, interfaceIndex, umtsMacData, ueMacVar);
            }

            break;
        }
        case UMTS_MAC_RACH_WAIT_AICH:
        {
            if (eventType == UMTS_MAC_RACH_EVENT_PhyAccessCnfNack)
            {
                if (DEBUG_MAC_RACH)
                {
                    printf("rcvd CFN-NACK in WAIT-AICH sate\n");
                }

                clocktype delay;

                // in case T2 is still runing
                // in case T2 is not runing
                if (ueMacVar->cshmEntity->activeRachInfo->timerBO)
                {

                    MESSAGE_CancelSelfMsg(
                        node,
                        ueMacVar->cshmEntity->activeRachInfo->timerBO);
                    ueMacVar->cshmEntity->activeRachInfo->timerBO = NULL;
                }

                //TODO, give a right BO time
                ueMacVar->cshmEntity->activeRachInfo->curBo =
                    ueMacVar->cshmEntity->rachConfig->NB01min;

                // set a BO timer
                ueMacVar->cshmEntity->activeRachInfo->timerBO =
                    UmtsLayer2InitTimer(
                                 node, interfaceIndex,
                                 UMTS_LAYER2_SUBLAYER_MAC,
                                 UMTS_MAC_RACH_BO, 0, NULL);

                delay = ueMacVar->cshmEntity->activeRachInfo->curBo *
                            umtsMacData->slotDuration *
                            UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME;

                // count in frame
                MESSAGE_Send(
                    node,
                    ueMacVar->cshmEntity->activeRachInfo->timerBO,
                    delay);

                // move to backoff state
                ueMacVar->cshmEntity->activeRachInfo->state =
                    UMTS_MAC_RACH_BACKOFF;

            }
            else if (eventType == UMTS_MAC_RACH_EVENT_PhyAccessCnfNoAichRsp)
            {
                if (DEBUG_MAC_RACH)
                {
                    printf("rcvd NO-AICH-RSP in WAIT-AICH sate\n");
                }
                // in case the T2 has expired move to persitence test
                // else stay in WAIT_T2
                if (ueMacVar->cshmEntity->activeRachInfo->timerT2)
                {
                    ueMacVar->cshmEntity->activeRachInfo->state =
                        UMTS_MAC_RACH_WAIT_T2;
                }
                else
                {
                    ueMacVar->cshmEntity->activeRachInfo->state =
                        UMTS_MAC_RACH_PERSISTENCE_TEST;

                    // persistence test
                    UmtsMacUeRachPersistenceTest(
                        node, interfaceIndex, umtsMacData, ueMacVar);
                }
            }
            else if (eventType == UMTS_MAC_RACH_EVENT_PhyAccessCnfAck)
            {
                if (DEBUG_MAC_RACH)
                {
                    printf("rcvd CFN-ACK in WAIT-AICH sate\n");
                }
                // cancel T2 if still running
                if (ueMacVar->cshmEntity->activeRachInfo->timerT2)
                {
                    MESSAGE_CancelSelfMsg(
                        node,
                        ueMacVar->cshmEntity->activeRachInfo->timerT2);
                    ueMacVar->cshmEntity->activeRachInfo->timerT2 = NULL;
                }
                ueMacVar->cshmEntity->activeRachInfo->state =
                    UMTS_MAC_RACH_WAIT_TX;

                // put the data part in the PRACH Tx queue
                unsigned char rbId = 0;
                int numPdus;
                int pduSize;
                std::list<Message*> pduList;

                if (DEBUG_MAC_RACH)
                {
                    printf("node %d (L2-MAC): rcvd ACCESS ACK, "
                       "get ready to RACH msg part\n",node->nodeId);
                }

                // get the pdu size for this rb
                // TODO: Consult TRANSPORT FORMAT at this time
                // Get the tranpsort foramt to determine the pduSize
                numPdus = 1;
                pduSize = UMTS_RLC_DEF_UMMAXULPDUSIZE;

                // get the pakcet

                UmtsRlcHasPacketSentToMac(
                    node,
                    interfaceIndex,
                    rbId,
                    numPdus,
                    pduSize,
                    pduList);

                // put it to the corresponding PhyCh Tx queue
                if (!pduList.empty())
                {
                    UmtsMacPhyUeEnqueuePacketToPhChTxBuffer(
                        node,
                        interfaceIndex,
                        UMTS_PHYSICAL_PRACH,
                        1,
                        pduList);
                }

                // move the state to IDLE
                ueMacVar->cshmEntity->activeRachInfo->state =
                    UMTS_MAC_RACH_IDLE;
            }

            break;
        }
        case UMTS_MAC_RACH_WAIT_TX:
        {
            if (DEBUG_MAC_RACH)
            {
                printf("in WAIT-AICH sate\n");
            }

            break;
        }
        default:
        {
            if (DEBUG_MAC_RACH)
            {
                printf("in default sate\n");
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsUeHandlePhyAccessCfn
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Handle Interlayer PHY-ACCESS-CFN at Ue
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + ueMacVar         : UmtsMacUeData*    : ue mac var
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
static
void UmtsUeHandlePhyAccessCfn(Node* node,
                              UInt32 interfaceIndex,
                              UmtsMacData* umtsMacData,
                              UmtsMacUeData* ueMacVar,
                              void* cmd)
{
    UmtsPhyAccessCnfInfo* cfnInfo;
    UmtsMacRachStateTransitEvent eventType = UMTS_MAC_RACH_EVENT_Invalid;
    BOOL handle = TRUE;

    if (DEBUG_MAC_INTERLAYER)
    {
        UmtsLayer2PrintRunTimeInfo(
            node, interfaceIndex, UMTS_LAYER2_SUBLAYER_MAC);
    }

    cfnInfo = (UmtsPhyAccessCnfInfo*)cmd;

    if (cfnInfo->cnfType == UMTS_PHY_ACCESS_CFN_NACK)
    {
        eventType = UMTS_MAC_RACH_EVENT_PhyAccessCnfNack;
        if (DEBUG_MAC_INTERLAYER)
        {
            printf("rcvd a PHY-ACCESS-CFN with NACK\n");
        }
    }
    else if (cfnInfo->cnfType == UMTS_PHY_ACCESS_CFN_ACK)
    {
        eventType = UMTS_MAC_RACH_EVENT_PhyAccessCnfAck;
        if (DEBUG_MAC_INTERLAYER)
        {
            printf("rcvd a PHY-ACCESS-CFN with ACK\n");
        }
    }
    else if (cfnInfo->cnfType == UMTS_PHY_ACCESS_CFN_NO_AICH_RSP)
    {
        eventType = UMTS_MAC_RACH_EVENT_PhyAccessCnfNoAichRsp;
        if (DEBUG_MAC_INTERLAYER)
        {
            printf("rcvd a PHY-ACCESS-CFN with No AICH rsp\n");
        }
    }
    else
    {
        handle = FALSE;
    }
    if (handle)
    {
        UmtsMacUeHandleRachStateTransitionEvent(
            node, interfaceIndex, umtsMacData, ueMacVar, eventType);
    }
}

// /**
// FUNCITON   :: UmtsMacUeFinalizeCshmEntity
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Finalize the C-shm MAC enetity at Ue
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + ueMacVar         : UmtsMacUeData*    : ue mac var
// RETURN     :: void : NULL
static
void UmtsMacUeFinalizeCshmEntity(Node* node,
                                 UInt32 interfaceIndex,
                                 UmtsMacData* umtsMacData,
                                 UmtsMacUeData* ueMacVar)
{
    UmtsMacEntity_cshmInfo* cshmEntity;
    cshmEntity = ueMacVar->cshmEntity;

    if (cshmEntity->rachConfig)
    {
        // empty list in rachConfig
        for (std::list<UmtsRachAscInfo*>::iterator achInfoIt =
            cshmEntity->rachConfig->prachPartition->begin();
            achInfoIt != cshmEntity->rachConfig->prachPartition->end();
            achInfoIt ++)
        {
            delete (*achInfoIt);
            *achInfoIt = NULL;
        }
        cshmEntity->rachConfig->prachPartition->remove(NULL);

        // free the list pointer
        delete cshmEntity->rachConfig->prachPartition;

        // free the mem for rachConfig        
        MEM_free(cshmEntity->rachConfig);
        cshmEntity->rachConfig = NULL;
    }

    if (cshmEntity->activeRachInfo)
    {
        // free the timer if not NULL
        if (cshmEntity->activeRachInfo->timerT2)
        {
            MESSAGE_CancelSelfMsg(node,
                         cshmEntity->activeRachInfo->timerT2);
            cshmEntity->activeRachInfo->timerT2 = NULL;
        }
        if (cshmEntity->activeRachInfo->timerBO)
        {
            MESSAGE_CancelSelfMsg(node,
                         cshmEntity->activeRachInfo->timerBO);
            cshmEntity->activeRachInfo->timerBO = NULL;
        }

        // free the mem for activeRachInfo
        MEM_free(cshmEntity->activeRachInfo);
        cshmEntity->activeRachInfo = NULL;
    }

    // free the entity
    MEM_free(cshmEntity);
    ueMacVar->cshmEntity = NULL;
}

//  /**
// FUNCITON   :: UmtsMacUeConfigRach
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Handle Interlayer CMAC-CONFIG-REQ_RACH at Ue
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + ueMacVar         : UmtsMacUeData*    : ue mac var
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
static
void UmtsMacUeConfigRach(Node* node,
                         UInt32 interfaceIndex,
                         UmtsMacData* umtsMacData,
                         UmtsMacUeData* ueMacVar,
                         void* cmd)
{
    UmtsMacEntity_cshmInfo* cshmEntity;
    UmtsCmacConfigReqRachInfo* rachCfgReqInfo;

    rachCfgReqInfo = (UmtsCmacConfigReqRachInfo*) cmd;

    if (!rachCfgReqInfo->releaseRachInfo)
    {
        UmtsMacEntityRachDynamicInfo* activeRachInfo;
        UmtsMacEntityRachConfigInfo* rachCfgInfo;

        // create entity
        // if already exist,free it and create a new one
        if (ueMacVar->cshmEntity)
        {
            UmtsMacUeFinalizeCshmEntity(
                node, interfaceIndex, umtsMacData, ueMacVar);   
        }

        cshmEntity = (UmtsMacEntity_cshmInfo*)
                  MEM_malloc(sizeof(UmtsMacEntity_cshmInfo));
        memset(cshmEntity, 0, sizeof(UmtsMacEntity_cshmInfo));
        ueMacVar->cshmEntity = cshmEntity;

        // configuration from RRC
        rachCfgInfo = (UmtsMacEntityRachConfigInfo*)
                       MEM_malloc(sizeof(UmtsMacEntityRachConfigInfo));
        cshmEntity->rachConfig = rachCfgInfo;

        rachCfgInfo->Mmax = rachCfgReqInfo->Mmax;
        rachCfgInfo->NB01min = rachCfgReqInfo->NB01min;
        rachCfgInfo->NB01max = rachCfgReqInfo->NB01max;
 
         rachCfgInfo->prachPartition =
                new std::list<UmtsRachAscInfo*>;


        // empty list in rachConfig
        for (std::list<UmtsRachAscInfo*>::iterator achInfoIt =
            rachCfgReqInfo->prachPartition->begin();
            achInfoIt != rachCfgReqInfo->prachPartition->end();
            achInfoIt ++)
        {
            UmtsRachAscInfo* ascInfo = new UmtsRachAscInfo;
            memcpy(ascInfo, *(achInfoIt),sizeof(UmtsRachAscInfo));
            rachCfgInfo->prachPartition->push_back(ascInfo);
        }

        // create the active part
        activeRachInfo = (UmtsMacEntityRachDynamicInfo*)
                          MEM_malloc(sizeof(UmtsMacEntityRachDynamicInfo));
        cshmEntity->activeRachInfo = activeRachInfo;
        memset(activeRachInfo,
               0,
               sizeof(UmtsMacEntityRachDynamicInfo));

        // initilize the varibales
        activeRachInfo->activeAscIndex = rachCfgReqInfo->ascIndex;
        
        // if activeRachInfo->activeAscIndex is not
        // within the range of the configuration
        // reselect one at MAC layer
        std::list<UmtsRachAscInfo*>::iterator it;
        for (it = cshmEntity->rachConfig->prachPartition->begin();
            it != cshmEntity->rachConfig->prachPartition->end();
            it ++)
        {
            if ((*it)->ascIndex == activeRachInfo->activeAscIndex)
            {
                break;
            }
        }

        ERROR_Assert(it != cshmEntity->rachConfig->prachPartition->end(),
                     " no prachpartition info link with this asc index");

        activeRachInfo->assignedSubCh = (*it)->assignedSubCh;
        activeRachInfo->sigEndIndex = (*it)->sigEndIndex;
        activeRachInfo->sigStartIndex = (*it)->sigStartIndex;
        activeRachInfo->persistFactor = (*it)->persistFactor;
        activeRachInfo->state = UMTS_MAC_RACH_IDLE;
        activeRachInfo->timerBO = NULL;
        activeRachInfo->timerT2 = NULL;
        activeRachInfo->curBo = 0;
        activeRachInfo->retryCounter = 0;

        if (DEBUG_MAC_RACH)
        {
            UmtsLayer2PrintRunTimeInfo(
                node,
                interfaceIndex,
                UMTS_LAYER2_SUBLAYER_MAC);
            printf("create MAC cshm entity and config the RACH\n");
        }
    }
    else
    {
        if (ueMacVar->cshmEntity)
        {
            UmtsMacUeFinalizeCshmEntity(
                node, interfaceIndex, umtsMacData, ueMacVar);   
        }

        // it could be move the finalize
        if (DEBUG_MAC_RACH)
        {
            UmtsLayer2PrintRunTimeInfo(
                node,
                interfaceIndex,
                UMTS_LAYER2_SUBLAYER_MAC);
            printf("free MAC cshm entity and release the RACH config\n");
        }
    }
}

//  /**
// FUNCITON   :: UmtsMacUeConfigRb
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Handle Interlayer CMAC-CONFIG-REQ_RB at Ue
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + ueMacVar         : UmtsMacUeData*    : ue mac var
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
static
void UmtsMacUeConfigRb(Node* node,
                       UInt32 interfaceIndex,
                       UmtsMacData* umtsMacData,
                       UmtsMacUeData* ueMacVar,
                       void* cmd)
{
    UmtsCmacConfigReqRbInfo* rbInfo;

    rbInfo = (UmtsCmacConfigReqRbInfo*) cmd;

    UmtsRbLogTrPhMapping* logTrPhMap;
    if (DEBUG_MAC_RB)
    {
        UmtsLayer2PrintRunTimeInfo(node,
            interfaceIndex,
            UMTS_LAYER2_SUBLAYER_MAC);
        printf("Attempting to config/release a RB with id %d\n",
               rbInfo->rbId);
    }
    std::list<UmtsRbLogTrPhMapping*>::iterator it;

    if (!rbInfo->releaseRb)
    {
        // to see if this rb has been already built
        for (it = ueMacVar->ueLogTrPhMap->begin();
            it != ueMacVar->ueLogTrPhMap->end();
            it ++)
        {
            if ((*it)->rbId == rbInfo->rbId)
            {
                break;
            }
        }
        if (it != ueMacVar->ueLogTrPhMap->end())
        {
            logTrPhMap = *it;
            if (DEBUG_MAC_RB)
            {
                printf("    LogTrPy Mapping already exists\n");
            }
        }
        else
        {
            logTrPhMap = new UmtsRbLogTrPhMapping;
            memset(logTrPhMap, 0, sizeof(UmtsRbLogTrPhMapping));
            if (DEBUG_MAC_RB)
            {
                printf("    need create a new LogTrPy Mapping\n");
            }
        }

        // populate the mapping
        logTrPhMap->rbId = rbInfo->rbId;
        if (rbInfo->chDir == UMTS_CH_UPLINK)
        {
            logTrPhMap->ulLogChId = rbInfo->logChId;
            logTrPhMap->ulLogChType = rbInfo->logChType;
            logTrPhMap->ulTrChType = rbInfo->trChType;
            logTrPhMap->ulTrChId = rbInfo->trChId;
            if (DEBUG_MAC_RB)
            {
                printf("    Update the Uplink mapping info\n");
            }
        }
        else
        {
            logTrPhMap->dlLogChId = rbInfo->logChId;
            logTrPhMap->dlLogChType = rbInfo->logChType;
            logTrPhMap->dlTrChType = rbInfo->trChType;
            logTrPhMap->dlTrChId = rbInfo->trChId;
            if (DEBUG_MAC_RB)
            {
                printf("    Update the Downlink mapping info\n");
            }
        }

        if (it == ueMacVar->ueLogTrPhMap->end())
        {
            // for new mapping info,add t othe mapping list
            ueMacVar->ueLogTrPhMap->push_back(logTrPhMap);
            if (DEBUG_MAC_RB)
            {
                printf("    add mapping info to the mapList\n");
            }
        }

        if (rbInfo->chDir == UMTS_CH_UPLINK)
        {
            // only UL channles  need to be added to the list
            UmtsLogChInfo* logChInfo = NULL;
            logChInfo = UmtsMacGetLogChInfoFromList(
                            node,
                            ueMacVar->ueLogChList,
                            rbInfo->logChType,
                            rbInfo->logChId);

            if (!logChInfo)
            {
                logChInfo = new UmtsLogChInfo(node->nodeId,
                                              rbInfo->logChType,
                                              rbInfo->chDir,
                                              rbInfo->logChId,
                                              rbInfo->logChPrio);
                ueMacVar->ueLogChList->push_back(logChInfo);
                if (DEBUG_MAC_RB)
                {
                    printf("    create a new LogInfo and add to list with "
                        "type %d, chDir %d, chId %d, chPrio %d\n",
                        rbInfo->logChType, rbInfo->chDir,
                        rbInfo->logChId, rbInfo->logChPrio);
                }
            }
        } // if (rbInfo->chDir == UMTS_CH_UPLINK)
    } // config a rb
    else
    {
        for (it = ueMacVar->ueLogTrPhMap->begin();
             it != ueMacVar->ueLogTrPhMap->end();
             it ++)
        {
            if ((*it)->rbId == rbInfo->rbId)
            {
                break;
            }
        }

        ERROR_Assert(it != ueMacVar->ueLogTrPhMap->end(),
                     "no mapping link with this rbId");

        // remove the logical channel if rb downlink part
        if (rbInfo->chDir == UMTS_CH_UPLINK)
        {
            std::list<UmtsLogChInfo*>::iterator itLog;
            for (itLog = ueMacVar->ueLogChList->begin();
                itLog != ueMacVar->ueLogChList->end();
                itLog ++)
            {
                if ((*itLog)->logChType == (*it)->ulLogChType &&
                    (*itLog)->logChId == (*it)->ulLogChId)
                {
                    break;
                }
            }

            ERROR_Assert(itLog != ueMacVar->ueLogChList->end(),
                         "no logChInfo link with this rb Mapping");

            delete (*itLog);
            ueMacVar->ueLogChList->erase(itLog);
            if (DEBUG_MAC_RB)
            {
                printf("    release a logCh  type %d id %d\n",
                       (*it)->ulLogChType, (*it)->ulLogChId );
            }
        } //  if (rbInfo->chDir == UMTS_CH_UPLINK)

        delete (*it);
        ueMacVar->ueLogTrPhMap->erase(it);
        if (DEBUG_MAC_RB)
        {
            printf("    release a rb & mapping\n");
        }
    } // if (release)
}

//  /**
// FUNCITON   :: UmtsMacUeHandleInterLayerCommand
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Handle Interlayer command at NodeB
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + cmdType          : UmtsInterlayerCommandType : command type
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
static
void UmtsMacUeHandleInterLayerCommand(
            Node* node,
            UInt32 interfaceIndex,
            UmtsInterlayerCommandType cmdType,
            void* cmd)
{
    MacCellularData* macCellularData;
    UmtsMacData* umtsMacData;
    UmtsMacUeData* ueMacVar;

    umtsMacData = UmtsMacGetSubLayerData(node, interfaceIndex);
    macCellularData = (MacCellularData*)umtsMacData->myMacCellularData;

    ueMacVar = (UmtsMacUeData*)umtsMacData->ueMacVar;

    if (DEBUG_MAC_INTERLAYER)
    {
        UmtsLayer2PrintRunTimeInfo(node,
                                   interfaceIndex,
                                   UMTS_LAYER2_SUBLAYER_MAC);
        printf("rcvd a interlayer cmd type %d\n", cmdType);
    }

    switch(cmdType)
    {
        case UMTS_MAC_DATA_REQ:
        {
            // this msg only used when init RACH procedure
            UmtsMacUeHandleRachStateTransitionEvent(
                node, interfaceIndex, umtsMacData,
                ueMacVar, UMTS_MAC_RACH_EVENT_DataToTx);

            break;
        }
        case UMTS_CMAC_CONFIG_REQ_RB:
        {
            UmtsMacUeConfigRb(node,
                              interfaceIndex,
                              umtsMacData,
                              ueMacVar,
                              cmd);

            break;
        }
        case UMTS_CMAC_CONFIG_REQ_RACH:
        {
            UmtsMacUeConfigRach(node,
                                interfaceIndex,
                                umtsMacData,
                                ueMacVar,
                                cmd);

            break;
        }
        case UMTS_PHY_ACCESS_CNF:
        {
            UmtsUeHandlePhyAccessCfn(
                node, interfaceIndex, umtsMacData, ueMacVar, cmd);

            break;
        }
        default:
        {
            break;
        }
    }
}

//--------------------------------------------------------------------------
// NodeB Handler part
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: UmtsMacNodeBUpdateTrChWeight
// LAYER      :: UMTS LAYER2 MAC sublayer at UE
// PURPOSE    :: Update the trnsport channel weight.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + mac              : UmtsMacData*      : Mac Data
// RETURN     :: void : NULL
// **/
static
void UmtsMacNodeBUpdateTrChWeight(Node* node,
                              UInt32 interfaceIndex,
                              UmtsMacData* mac)
{
    if (!mac->nodeBMacVar->nodeBTrChList->empty())
    {
        NodeBTrChList::iterator itTr;

        for (itTr = mac->nodeBMacVar->nodeBTrChList->begin();
             itTr != mac->nodeBMacVar->nodeBTrChList->end();
             itTr ++)
        {
            if (((*itTr)->trChType == UMTS_TRANSPORT_DCH
                && (*itTr)->logPrio > (char)UMTS_QOS_CLASS_STREAMING) ||
                (*itTr)->trChType == UMTS_TRANSPORT_HSDSCH)
            {
                // only weight DCHs for background and interactive apps
                std::list <UmtsRbLogTrPhMapping*>::iterator itMap;
                std::list <UmtsRbLogTrPhMapping*>* mapList;
                unsigned char rbId = 0;
                double cqiEff = 1;

                if ((*itTr)->ueId  == node->nodeId)
                {
                    mapList = mac->nodeBMacVar->nodeBLocalMap;
                }
                else
                {
                    // get the UE
                    UmtsMacNodeBUeInfo* ueInfo;
                    unsigned char cqiVal;

                    ueInfo = UmtsMacNodeBGetUeInfoFromList(
                                 node,
                                 mac->nodeBMacVar->macNodeBUeInfo,
                                 (*itTr)->ueId);
                    ERROR_Assert(
                        ueInfo, "no ueInfo associate with this TrCh");
                    mapList = ueInfo->ueLogTrPhMap;
                    if ((*itTr)->trChType == UMTS_TRANSPORT_HSDSCH)
                    {
                        UmtsMacPhyNodeBGetUeCqiInfo(
                            node, interfaceIndex, ueInfo->priSc, &cqiVal);

                        if (cqiVal == 0)
                        {
                            cqiVal = 1;
                        }

                        cqiEff = cqiVal;
                    }
                }

                for (itMap = mapList->begin();
                     itMap != mapList->end();
                     itMap ++)
                {
                    if ((*itMap)->dlTrChId == (*itTr)->trChId &&
                        (*itMap)->dlTrChType == (*itTr)->trChType)
                    {
                        UmtsRlcEntityInfoToMac  entityInfo;
                        rbId  = (*itMap)->rbId;
                        
                        if (UmtsRlcIndicateStatusToMac(
                            node,
                            interfaceIndex,
                            rbId,
                            entityInfo,
                            (*itTr)->ueId))
                        {
                            // give high weight to node with good CQI
                            // and long queue
                            // more meningful strtegy can be used
                            (*itTr)->trChWeight =
                                cqiEff *
                                UmtsMacCalculateTrChWeight(
                                    node,
                                    entityInfo);
                            (*itTr)->buffLen = entityInfo.bufOccupancy;
                        }
                        else
                        {
                            (*itTr)->trChWeight = (UInt64)0.0;
                            (*itTr)->buffLen = (UInt64)0.0;
                        }

                        if (DEBUG_MAC_DATA || DEBUG_MAC_HSDPA)
                        {
                            UmtsLayer2PrintRunTimeInfo(
                                node,
                                interfaceIndex,
                                UMTS_LAYER2_SUBLAYER_MAC);
                            printf("weight for rb %d is %f\n",
                                rbId, (*itTr)->trChWeight);
                        }
                    } // trChId && trChType
                } // for (itMap)
            } // DCH & STREAMING || HSDSCH
        } // for (itTr)
    } // if (!mac->nodeBMacVar->nodeBTrChList->empty())
}

// /**
// FUNCTION   :: UmtsMacNodeBHandleHsdpaTrCh
// LAYER      :: UMTS LAYER2 MAC sublayer at UE
// PURPOSE    :: Handle timers out messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + mac              : UmtsMacData*      : Mac Data
// + rbId             : unsigned char     : rb  Id
// + trChInfo         : UmtsTransChInfo   : trans. Ch. Info
// RETURN     :: void : NULL
// **/
static
void UmtsMacNodeBHandleHsdpaTrCh(Node* node,
                            UInt32 interfaceIndex,
                            UmtsMacData* mac,
                            unsigned char rbId,
                            UmtsTransChInfo* trChInfo)
{
    unsigned char numHspdsch;
    unsigned int hspdschId[15];
    unsigned char numHsscch;
    unsigned int hsscchId[15];
    unsigned int numActiveCh;

    UmtsMacNodeBUeInfo* ueInfo;
    unsigned char cqiVal;
    unsigned int maxAllowedPerTx = 0;
    int maxSimHsdpsch = 0;

    // get the HSDPA config
    UmtsMacPhyNodeBGetHsdpaConfig(
        node, interfaceIndex, &numHspdsch,
        hspdschId, &numHsscch, hsscchId);

    // get the number of used chnnels
    UmtsMacPhyNodeBGetActiveHsdpaChNum(
        node, interfaceIndex, &numActiveCh);

    if (numActiveCh >= numHspdsch)
    {
        return;
    }

    // get the UE

    ueInfo = UmtsMacNodeBGetUeInfoFromList(
                 node,
                 mac->nodeBMacVar->macNodeBUeInfo,
                 trChInfo->ueId);
    ERROR_Assert(ueInfo, "no ueInfo associate with this TrCh");

    UmtsMacPhyNodeBGetUeCqiInfo(
            node, interfaceIndex, ueInfo->priSc, &cqiVal);

    // how many HSDPSCH support for this CQI
    maxSimHsdpsch = umtsHsdpaTransBlkSizeCat1To6[cqiVal].numHspdsch;

    // tx per 15 slots or per 5 subframe
    maxAllowedPerTx =
        umtsHsdpaTransBlkSizeCat1To6[cqiVal].transBlkSize * 5;

    if (DEBUG_MAC_HSDPA)
    {
        UmtsLayer2PrintRunTimeInfo(
            node, interfaceIndex, UMTS_LAYER2_SUBLAYER_MAC);
        printf("\n\t maxchannel is %d cqi %d\n", maxSimHsdpsch, cqiVal );
    }

    // the trChInfo->buffLen has been update when
    // order the trnsport channel
    while (trChInfo->buffLen &&
          maxSimHsdpsch &&
          numActiveCh < numHspdsch)
    {
        int numPdus;
        int pduSize;
        std::list<Message*> pduList;
        std::list<Message*> pduListPerRb;
        UmtsPhysicalChannelType dlPhChType = UMTS_PHYSICAL_HSPDSCH;
        unsigned int            dlPhChId;

        // Get transport format
        // use the mimimum one as basic unit
        pduSize = umtsHsdpaTransBlkSizeCat1To6[1].transBlkSize / 8;
        numPdus = maxAllowedPerTx /
                  umtsHsdpaTransBlkSizeCat1To6[cqiVal].numHspdsch /
                  8 /
                  pduSize;

        // get the pakcet
        UmtsRlcHasPacketSentToMac(
            node,
            interfaceIndex,
            rbId,
            numPdus,
            pduSize,
            pduListPerRb,
            trChInfo->ueId);

        if (!pduListPerRb.empty())
        {
            pduList.insert(pduList.end(),
                           pduListPerRb.begin(),
                           pduListPerRb.end());
            dlPhChId = hspdschId[numActiveCh];

            // put it to the corresponding PhyCh Tx queue
            if (!pduList.empty())
            {
                UmtsRlcEntityInfoToMac  entityInfo;
                UmtsPhChUpdateInfo upInfo;
                UmtsModulationType modType =
                    umtsHsdpaTransBlkSizeCat1To6[cqiVal].mdType;
                //double gainFactor;
                NodeAddress  nodeId = trChInfo->ueId;

                memset(&upInfo, 0, sizeof(UmtsPhChUpdateInfo));

                //If sending Entity buffer is NULL,
                //fetch PDUs possibly left in the fragment entity
                UmtsRlcAmUpdateEmptyTransPduBuf(node,
                                                interfaceIndex,
                                                rbId,
                                                trChInfo->ueId);
                UmtsMacPhyNodeBEnqueuePacketToPhChTxBuffer(
                    node,
                    interfaceIndex,
                    dlPhChType,
                    dlPhChId,
                    pduList);
                if (DEBUG_MAC_HSDPA)
                {
                    UmtsLayer2PrintRunTimeInfo(
                        node, interfaceIndex, UMTS_LAYER2_SUBLAYER_MAC);
                    printf("\n\t put HSDPA data into phType %d phId %d \n",
                        dlPhChType, dlPhChId);
                }
                // Set the physical channel attributes
                upInfo.modType = &modType;
                upInfo.nodeId = &nodeId;

                // TODO: gainFactor for power adjustment

                UmtsMacPhyNodeBUpdatePhChInfo(
                    node,
                    interfaceIndex,
                    dlPhChType,
                    dlPhChId,
                    &upInfo);

                // increase the Hsdpa ch usage counter
                numActiveCh ++;
                maxSimHsdpsch --;

                // update active HSDPA Ch
                UmtsMacPhyNodeBSetActiveHsdpaChNum(
                    node, interfaceIndex, numActiveCh);

                // update the BuffeLen
                if (UmtsRlcIndicateStatusToMac(
                        node,
                        interfaceIndex,
                        rbId,
                        entityInfo,
                        trChInfo->ueId))
                {
                    trChInfo->buffLen = entityInfo.bufOccupancy;
                }
                else
                {
                    trChInfo->buffLen = (UInt64)0.0;
                }
            }
        }
        else
        {
            break;
        }
    }
}

// /**
// FUNCTION   :: UmtsMacNodeBHandleSlotTimer
// LAYER      :: UMTS LAYER2 MAC sublayer at UE
// PURPOSE    :: Handle timers out messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + mac              : UmtsMacData*      : Mac Data
// RETURN     :: void : NULL
// **/
static
void UmtsMacNodeBHandleSlotTimer(Node* node,
                                 UInt32 interfaceIndex,
                                 UmtsMacData* mac)
{
    Message* msgToSend = NULL;

    // order the transport CHannel List
    if (mac->chipRateType == UMTS_CHIP_RATE_384 &&
        mac->slotIndex == 0)
    {
        // for each DCH transport channel,
        // calculate the latest prio and weight
        // then order them
        if (!mac->nodeBMacVar->nodeBTrChList->empty())
        {
            UmtsMacNodeBUpdateTrChWeight(node, interfaceIndex, mac);

#ifdef _WIN64
            mac->nodeBMacVar->nodeBTrChList->sort(
                std::greater<UmtsTransChInfo*>());
#else
            mac->nodeBMacVar->nodeBTrChList->sort(UmtsTransChInfoPtrLess());
#endif
        }
        // other types channel give 0 value of weight and prio
    }

    // check each transport Channel's TTI to
    if (!mac->nodeBMacVar->nodeBTrChList->empty())
    {
        // msgToSend will be constructed here
        // go each TrCh, if TTI meet, trtrieve PDU, add CRC
        // make CCTrCh
        // link them togetehr as a msg list
        UeTrChList::iterator itTr;

        for (itTr = mac->nodeBMacVar->nodeBTrChList->begin();
             itTr != mac->nodeBMacVar->nodeBTrChList->end();
             itTr ++)
        {
            (*itTr)->slotCounter ++;

            // update SFN if needed
            if ((*itTr)->trChType == UMTS_TRANSPORT_BCH)
            {
                if ((*itTr)->slotCounter == ((*itTr)->TTI)/2)
                {
                    unsigned int sfn;

                    // do not need to supply priSc for nodeB
                    sfn = UmtsMacPhyGetPccpchSfn(node, interfaceIndex);
                    sfn ++;
                    sfn = sfn % UMTS_MAX_SFN;
                    UmtsMacPhySetPccpchSfn(node, interfaceIndex,  sfn + 1);
                }
            }

            if ((*itTr)->slotCounter >= (*itTr)->TTI)
            {

                (*itTr)->slotCounter = 0;

                // time to check if any packets to send from
                // this transport channel
                // local Mapping first
                std::list <UmtsRbLogTrPhMapping*>::iterator itMap;
                std::list <UmtsRbLogTrPhMapping*>* mapList;

                if ((*itTr)->ueId  == node->nodeId)
                {
                    mapList = mac->nodeBMacVar->nodeBLocalMap;
                }
                else
                {
                    // get the UE
                    UmtsMacNodeBUeInfo* ueInfo;
                    ueInfo = UmtsMacNodeBGetUeInfoFromList(
                                 node,
                                 mac->nodeBMacVar->macNodeBUeInfo,
                                 (*itTr)->ueId);
                    ERROR_Assert(
                        ueInfo, 
                        "no ueInfo associate with this TrCh");
                    mapList = ueInfo->ueLogTrPhMap;
                }

                // common mapping
                for (itMap = mapList->begin();
                     itMap != mapList->end();
                     itMap ++)
                {
                    if ((*itMap)->dlTrChId == (*itTr)->trChId &&
                        (*itMap)->dlTrChType == (*itTr)->trChType)
                    {
                        unsigned char rbId = 0;
                        int numPdus;
                        int pduSize;
                        std::list<Message*> pduList;
                        UmtsPhysicalChannelType dlPhChType;
                        unsigned int            dlPhChId;
                        unsigned char ratePara = 1;
                        unsigned char crcSize = 0;

                        std::list<Message*> pduListPerRb;

                        // find the map
                        rbId = (*itMap)->rbId;
                        ratePara = UmtsGetRateParaFromCodingType(
                                       (*itMap)->transFormat.codingType);
                        crcSize = (unsigned char)((*itMap)->
                                                   transFormat.crcSize);

                        if ((*itTr)->trChType != UMTS_TRANSPORT_HSDSCH)
                        {
                            // Get transport format
                            pduSize = (*itMap)->
                                          transFormat.GetTransBlkSize() / 8;
                            numPdus = 
                                (int)((*itMap)->
                                        transFormat.GetTransBlkSetSize() /
                                       8 /
                                       pduSize);

                            if (rbId == UMTS_CCCH_RB_ID ||
                                rbId >= UMTS_BCCH_RB_ID)
                            {
                                numPdus = 5;
                            }

                            // get the pakcet
                            UmtsRlcHasPacketSentToMac(
                                node,
                                interfaceIndex,
                                rbId,
                                numPdus,
                                pduSize,
                                pduListPerRb,
                                (*itTr)->ueId);

                            if (!pduListPerRb.empty())
                            {
                                pduList.insert(pduList.end(),
                                               pduListPerRb.begin(),
                                               pduListPerRb.end());
                                dlPhChType = (*itMap)->dlPhChType;
                                dlPhChId = (*itMap)->dlPhChId;


                                // put it to the PhyCh Tx queue
                                if (!pduList.empty())
                                {
                                 UmtsMacPhyNodeBEnqueuePacketToPhChTxBuffer(
                                        node,
                                        interfaceIndex,
                                        dlPhChType,
                                        dlPhChId,
                                        pduList);

                                    if (DEBUG_MAC_DATA)
                                    {

                                        int numBits = 0;

                                        std::list<Message*>::iterator msgIt;

                                        // count the msg size
                                        for (msgIt = pduList.begin();
                                            msgIt != pduList.end();
                                            msgIt ++)
                                        {

                                            UmtsRlcPackedPduInfo* info =
                                               (UmtsRlcPackedPduInfo*)
                                                MESSAGE_ReturnInfo((*msgIt));
                                            numBits += info->pduSize * 8;

                                            // CRC
                                            numBits += (int)crcSize;
                                            // no MAC header is needed
                                        }

                                        numBits *= ratePara;


                                        UmtsLayer2PrintRunTimeInfo(
                                            node, 
                                            interfaceIndex, 
                                            UMTS_LAYER2_SUBLAYER_MAC);
                                        printf("\n\t trType %d TrId %d "
                                            "put %d bits of message into "
                                            "phType %d phId %d to node%d\n",
                                            (*itTr)->trChType, 
                                            (*itTr)->trChId,
                                            numBits, dlPhChType, dlPhChId, 
                                            (*itTr)->ueId);
                                    }
                                } // if (!pduList.empty())
                            } // if (!pduListPerRb.empty())
                        } // if ((*itTr)->trChType != UMTS_TRANSPORT_HSDSCH)
                        else
                        {
                            // HSDPA
                            UmtsMacNodeBHandleHsdpaTrCh
                                (node, interfaceIndex,
                                 mac, rbId, (*itTr));
                        } // if ((*itTr)->trChType == UMTS_TRANSPORT_HSDSCH)
                    } // dlTrType && dlTrId
                } // itMap
            } // if ((*itTr)->slotCounter >= (*itTr)->TTI)
        } // for (itTr)
    } // if (!mac->nodeBMacVar->nodeBTrChList->empty())


    // becasue interleave/deinterleave is not implemented
    // the transission is only happended once per frame rather that
    // per slot
    // for the ue's slotIndex
    if (mac->chipRateType == UMTS_CHIP_RATE_384 &&
        mac->slotIndex == 0)
    {

        // adjust the transmission power before each transmission
        // not implemented in htis release
        // UmtsMacPhyNodeBCheckDlPowerControlAdjustment(node,
        //                                              interfaceIndex);

        UmtsMacNodeBTransmitMacBurst(
            node, interfaceIndex, mac, msgToSend);
#ifdef PARALLEL //Parallel
        PARALLEL_SetLookaheadHandleEOT(
            node,
            mac->lookaheadHandle,
            node->getNodeTime() + 
            mac->slotDuration * 
            UMTS_RADIO_FRAME_DURATION_384);
#endif //endParallel
    }
    mac->slotIndex = (mac->slotIndex + 1) % 
                      UMTS_RADIO_FRAME_DURATION_384;

}

//  /**
// FUNCITON   :: UmtsMacNodeBConfigRb
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Handle Interlayer CMAC-CONFIG-REQ_RB at NodeB
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : umts Mac Data,
// + nodeBMacVar      : UmtsMacNodeBData* : NodeB mac var
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
static
void UmtsMacNodeBConfigRb(Node* node,
                          UInt32 interfaceIndex,
                          UmtsMacData* umtsMacData,
                          UmtsMacNodeBData* nodeBMacVar,
                          void* cmd)
{
    UmtsCmacConfigReqRbInfo* rbInfo;

    rbInfo = (UmtsCmacConfigReqRbInfo*) cmd;

    UmtsRbLogTrPhMapping* logTrPhMap = NULL;
    BOOL newMap = FALSE;
    std::list <UmtsRbLogTrPhMapping*>* mappingList = NULL;
    std::list<UmtsRbLogTrPhMapping*>::iterator it;

    if (DEBUG_MAC_RB)
    {
        UmtsLayer2PrintRunTimeInfo(node,
            interfaceIndex,
            UMTS_LAYER2_SUBLAYER_MAC);
        printf("Attempting to config a RB with id %d\n", rbInfo->rbId);
    }

    if (rbInfo->ueId == node->nodeId)
    {
        mappingList = nodeBMacVar->nodeBLocalMap;
        if (DEBUG_MAC_RB)
        {
            printf("    it is a common RB\n");
        }
    }
    else
    {
        // get the UE
        UmtsMacNodeBUeInfo* ueInfo;

        ueInfo = UmtsMacNodeBGetUeInfoFromList(node,
                        nodeBMacVar->macNodeBUeInfo,
                        rbInfo->ueId);

        if (!ueInfo)
        {
            ueInfo = UmtsMacNodeBInitUeNodeBInfo(
                        node,
                        rbInfo->ueId,
                        rbInfo->priSc);
            nodeBMacVar->macNodeBUeInfo->push_back(ueInfo);
        }

        mappingList = ueInfo->ueLogTrPhMap;
        if (DEBUG_MAC_RB)
        {
            printf("    it is a dedicated RB for UE  with priSC%d\n",
                   rbInfo->priSc);
        }
    }

    if (!rbInfo->releaseRb)
    {
        for (it = mappingList->begin();
                it != mappingList->end();
                it ++)
        {
            if ((*it)->rbId == rbInfo->rbId)
            {
                break;
            }
        }
        if (it != mappingList->end())
        {
            logTrPhMap = *it;
            if (DEBUG_MAC_RB)
            {
                printf("    LogTrPy Mapping already exists\n");
            }
        }
        else
        {
            logTrPhMap = new UmtsRbLogTrPhMapping;
            newMap = TRUE;
            if (DEBUG_MAC_RB)
            {
                printf("    need create a new LogTrPy Mapping\n");
            }
        }

        // populate the mapping
        logTrPhMap->rbId = rbInfo->rbId;
        if (rbInfo->chDir == UMTS_CH_UPLINK)
        {
            logTrPhMap->ulLogChId = rbInfo->logChId;
            logTrPhMap->ulLogChType = rbInfo->logChType;
            logTrPhMap->ulTrChType = rbInfo->trChType;
            logTrPhMap->ulTrChId = rbInfo->trChId;
            if (DEBUG_MAC_RB)
            {
                printf("    Update the Uplink mapping info\n");
                printf("logChType: %d, logChId: %d,"
                       "trChType: %d, trChId: %d\n",
                        rbInfo->logChType,
                        rbInfo->logChId,
                        rbInfo->trChType,
                        rbInfo->trChId);
            }
        }
        else
        {
            logTrPhMap->dlLogChId = rbInfo->logChId;
            logTrPhMap->dlLogChType = rbInfo->logChType;
            logTrPhMap->dlTrChType = rbInfo->trChType;
            logTrPhMap->dlTrChId = rbInfo->trChId;
            if (DEBUG_MAC_RB)
            {
                printf("    Update the Downlink mapping info\n");
                printf("logChType: %d, logChId: %d,"
                       "trChType: %d, trChId: %d\n",
                       rbInfo->logChType,
                       rbInfo->logChId,
                       rbInfo->trChType,
                       rbInfo->trChId);
            }
        }

        if (newMap)
        {
            mappingList->push_back(logTrPhMap);
            if (DEBUG_MAC_RB)
            {
                printf("    add mapping info to the mapList\n");
            }
        }

        if (rbInfo->chDir == UMTS_CH_DOWNLINK)
        {
            // only DL channles  need to be added to the list
            UmtsLogChInfo* logChInfo = NULL;
            logChInfo = UmtsMacGetLogChInfoFromList(
                            node,
                            nodeBMacVar->nodeBLogChList,
                            rbInfo->logChType,
                            rbInfo->logChId);

            if (!logChInfo)
            {
                logChInfo = new UmtsLogChInfo(node->nodeId,
                                              rbInfo->logChType,
                                              rbInfo->chDir,
                                              rbInfo->logChId,
                                              rbInfo->logChPrio);
                nodeBMacVar->nodeBLogChList->push_back(logChInfo);
                if (DEBUG_MAC_RB)
                {
                    printf("    create a new LogInfo and add to list with "
                        "type %d, chDir %d, chId %d, chPrio %d\n",
                        rbInfo->logChType, rbInfo->chDir,
                        rbInfo->logChId, rbInfo->logChPrio);
                }
            } // new logical channel
        } // Downlink 
    } // config a RB
    else
    {
        for (it = mappingList->begin();
             it != mappingList->end();
             it ++)
        {
            if ((*it)->rbId == rbInfo->rbId)
            {
                break;
            }
        }
        char errorMsg[MAX_STRING_LENGTH];
        sprintf(errorMsg, "no mapping Link with with rb: %d",
                rbInfo->rbId);
        ERROR_Assert(it != mappingList->end(), errorMsg);

        // remove the logical channel if rb downlink part
        if (rbInfo->chDir == UMTS_CH_DOWNLINK)
        {
            std::list<UmtsLogChInfo*>::iterator itLog;
            for (itLog = nodeBMacVar->nodeBLogChList->begin();
                itLog != nodeBMacVar->nodeBLogChList->end();
                itLog ++)
            {
                if ((*itLog)->logChType == (*it)->dlLogChType &&
                    (*itLog)->logChId == (*it)->dlLogChId)
                {
                    break;
                }
            }

            ERROR_Assert(itLog != nodeBMacVar->nodeBLogChList->end(),
                         "no logChInfo link with this rb Mapping");

            delete (*itLog);
            nodeBMacVar->nodeBLogChList->erase(itLog);
            if (DEBUG_MAC_RB)
            {
                printf("    release a logCh type %d id %d\n",
                       (*it)->dlLogChType, (*it)->dlLogChId);
            }
        }

        delete (*it);
        mappingList->erase(it);
        if (DEBUG_MAC_RB)
        {
            printf("    release a rb & mapping\n");
        }
    } // release a RB
}

//  /**
// FUNCITON   :: UmtsMacNodeBHandleInterLayerCommand
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Handle Interlayer command at NodeB
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + cmdType          : UmtsInterlayerCommandType : command type
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
static
void UmtsMacNodeBHandleInterLayerCommand(
            Node* node,
            UInt32 interfaceIndex,
            UmtsInterlayerCommandType cmdType,
            void* cmd)
{
    MacCellularData* macCellularData;
    UmtsMacData* umtsMacData;
    UmtsMacNodeBData* nodeBMacVar;

    umtsMacData = UmtsMacGetSubLayerData(node, interfaceIndex);
    macCellularData = (MacCellularData*)umtsMacData->myMacCellularData;

    nodeBMacVar = (UmtsMacNodeBData*)umtsMacData->nodeBMacVar;

    if (DEBUG_MAC_INTERLAYER)
    {
        UmtsLayer2PrintRunTimeInfo(node,
                                   interfaceIndex,
                                   UMTS_LAYER2_SUBLAYER_MAC);
        printf("rcvd a interlayer cmd type %d\n", cmdType);
    }
    switch(cmdType)
    {
        case UMTS_CMAC_CONFIG_REQ_RB:
        {
            UmtsMacNodeBConfigRb(node,
                                 interfaceIndex,
                                 umtsMacData,
                                 nodeBMacVar,
                                 cmd);

            break;

        }
        default:
        {
            break;
        }
    }

}

//--------------------------------------------------------------------------
// Handle Functions for general
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: UmtsMacHandleTimeout
// LAYER      :: UMTS LAYER2 MAC sublayer
// PURPOSE    :: Handle timers out messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + mac              : UmtsMacData*      : Mac Data
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
static
void UmtsMacHandleTimeout(Node* node,
                          UInt32  interfaceIndex,
                          UmtsMacData* mac,
                          Message* msg)
{
    UmtsLayer2TimerInfoHdr* timerInfo;
    timerInfo = (UmtsLayer2TimerInfoHdr*) MESSAGE_ReturnInfo(msg);

    if (timerInfo->timeType == UMTS_MAC_SLOT_TIMER)
    {
        if (UmtsGetNodeType(node) == CELLULAR_NODEB)
        {
            UmtsMacNodeBHandleSlotTimer(node, interfaceIndex, mac);
        }
        else if (UmtsGetNodeType(node) == CELLULAR_UE)
        {
            UmtsMacUeHandleSlotTimer(node, interfaceIndex, mac);
        }
        else
        {
            ERROR_Assert(FALSE, "SLOT TIMER at NON UE/NODE_B");
        }

        MESSAGE_Send(node,
                     msg,
                     mac->slotDuration);
    }
    else if (timerInfo->timeType == UMTS_MAC_RACH_T2)
    {
        if (UmtsGetNodeType(node) == CELLULAR_UE)
        {
            UmtsMacUeHandleRachStateTransitionEvent(
                node, interfaceIndex, mac,
                mac->ueMacVar, UMTS_MAC_RACH_EVENT_T2Expire);
        }
        else
        {
            ERROR_Assert(FALSE, "SLOT TIMER at NON UE/NODE_B");
        }

        MESSAGE_Free(node, msg);
    }
    else if (timerInfo->timeType == UMTS_MAC_RACH_BO)
    {
        if (UmtsGetNodeType(node) == CELLULAR_UE)
        {
            UmtsMacUeHandleRachStateTransitionEvent(
                node, interfaceIndex, mac,
                mac->ueMacVar, UMTS_MAC_RACH_EVENT_TboExpire);
        }
        else
        {
            ERROR_Assert(FALSE, "SLOT TIMER at NON UE/NODE_B");
        }

        MESSAGE_Free(node, msg);
    }
    else
    {
        MESSAGE_Free(node, msg);
    }
}

// /**
// FUNCTION   :: UmtsMacInitChannelList
// LAYER      :: UMTS MAC
// PURPOSE    :: Init the operational channel list
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void      : NULL
// **/
static
void UmtsMacInitChannelList(Node* node,
                            UInt32 interfaceIndex,
                            const NodeInput* nodeInput)
{
    MacCellularData* macCellularData;
    UmtsMacData* umtsMacData;
    BOOL retVal;
    int i;
    Int32 channelIndex = -1;


    macCellularData = (MacCellularData*)
                  node->macData[interfaceIndex]->macVar;

    umtsMacData = UmtsMacGetSubLayerData(node, interfaceIndex);
    ERROR_Assert(umtsMacData != NULL, "No  MAC data with this node");

    // If NodeB, read downlink and uplink control channels
    if (macCellularData->nodeType == CELLULAR_NODEB)
    {
        char string[MAX_STRING_LENGTH];
        BOOL wasFound;

        IO_ReadString(node,
                      node->nodeId,
                      interfaceIndex,
                      nodeInput,
                      "UMTS-NodeB-DOWNLINK-CHANNEL",
                      &wasFound,
                      string);

        if (wasFound)
        {
            if (IO_IsStringNonNegativeInteger(string))
            {
                channelIndex = (Int32)strtoul(string, NULL, 10);
                if (channelIndex < node->numberChannels)
                {
                    umtsMacData->currentDLChannel = channelIndex;
                }
                else
                {
                    ERROR_ReportErrorArgs("UMTS-NodeB-DOWNLINK-CHANNEL "
                                          "parameter should be an integer "
                                          "value between 0 and %d or a valid "
                                          "channel name.",
                                          node->numberChannels - 1);
                }
            }
            else if (isalpha(*string) && PHY_ChannelNameExists(node, string))
            {
                umtsMacData->currentDLChannel = 
                                PHY_GetChannelIndexForChannelName(node,
                                                                  string);
            }
            else
            {
                ERROR_ReportErrorArgs("UMTS-NodeB-DOWNLINK-CHANNEL "
                                      "parameter should be an integer "
                                      "value between 0 and %d or a valid "
                                      "channel name.",
                                      node->numberChannels - 1);
            }
        }
        else
        {
            ERROR_ReportError("UMTS-NodeB-DOWNLINK-CHANNEL"
                              " is not configured");
        }

        IO_ReadString(node,
                      node->nodeId,
                      interfaceIndex,
                      nodeInput,
                      "UMTS-NodeB-UPLINK-CHANNEL",
                      &wasFound,
                      string);

        if (wasFound)
        {
            if (IO_IsStringNonNegativeInteger(string))
            {
                channelIndex = (Int32)strtoul(string, NULL, 10);

                if (channelIndex < node->numberChannels)
                {
                    umtsMacData->currentULChannel = channelIndex;
                }
                else
                {
                    ERROR_ReportErrorArgs("UMTS-NodeB-UPLINK-CHANNEL "
                                          "parameter should be an integer "
                                          "between 0 and %d or a valid "
                                          "channel name.",
                                          node->numberChannels - 1);
                }
            }
            else if (isalpha(*string) && PHY_ChannelNameExists(node, string))
            {
                umtsMacData->currentULChannel = 
                                PHY_GetChannelIndexForChannelName(node,
                                                                  string);
            }
            else
            {
                ERROR_ReportErrorArgs("UMTS-NodeB-UPLINK-CHANNEL "
                                      "parameter should be an integer "
                                      "between 0 and %d or a valid "
                                      "channel name.",
                                      node->numberChannels - 1);
            }
        }
        else
        {
            ERROR_ReportError("UMTS-NodeB-UPLINK-CHANNEL "
                              "is not configured");
        }

        if (PHY_CanListenToChannel(
                     node,
                     node->macData[interfaceIndex]->phyNumber,
                     umtsMacData->currentULChannel) == FALSE)
        {
            ERROR_ReportError("cannot listen to this uplink channel!");
        }

        if (DEBUG_MAC_CHANNEL)
        {
            printf("Node%d(NodeB), DL ch = %d, UL ch = %d\n",
                   node->nodeId,
                   umtsMacData->currentDLChannel,
                   umtsMacData->currentULChannel);
        }
    }
    else if (macCellularData->nodeType == CELLULAR_UE)
    {
        // if UE, need to know the list of all downlink control channels
        ERROR_Assert(umtsMacData->ueMacVar != NULL,
                     "UE MAC var is NULL");
        umtsMacData->ueMacVar->numDLChannels = 0;
        umtsMacData->ueMacVar->scanIndex = 0;

        // read all downlink control channels for scanning of BSs
        for (i = 0; i < nodeInput->numLines; i ++)
        {
            if (strcmp(nodeInput->variableNames[i],
                       "UMTS-NodeB-DOWNLINK-CHANNEL") == 0)
            {
                int chIndex = atoi(nodeInput->values[i]);

                if (PHY_CanListenToChannel(
                     node,
                     node->macData[interfaceIndex]->phyNumber,
                     chIndex))
                {
                    //set the first scan channel index
                    if (PHY_IsListeningToChannel(
                             node,
                             node->macData[interfaceIndex]->phyNumber,
                             chIndex))
                    {
                        umtsMacData->ueMacVar->scanIndex =
                            umtsMacData->ueMacVar->numDLChannels;
                    }
                    umtsMacData->ueMacVar->dlChannelList->push_back(chIndex);
                    umtsMacData->ueMacVar->numDLChannels ++;
                }
            }
        }

        ERROR_Assert(umtsMacData->ueMacVar->numDLChannels > 0,
                     "no DL is allocated to the PLMN");

        if (DEBUG_MAC_CHANNEL)
        {
            std::vector<int>::iterator it =
                umtsMacData->ueMacVar->dlChannelList->begin();
            printf("Node%d knows %d downlink channels:",
                node->nodeId, umtsMacData->ueMacVar->numDLChannels);
            while (it != umtsMacData->ueMacVar->dlChannelList->end())
            {
                printf("%d ", *it);
                it ++;
            }
            printf("\n");
        }

        // init DL and UL in associated cell. No cell associated beginning
        umtsMacData->currentDLChannel = UMTS_INVALID_CHANNEL_INDEX;
        umtsMacData->currentULChannel = UMTS_INVALID_CHANNEL_INDEX;
    }
    else
    {
        // cellular MAC can only be run on either BS or UE.
        ERROR_ReportError("UMTS MAC can only be run either UE or NODE-B!");
    }

    // stop listen to all channels
    for (i = 0; i < node->numberChannels; i ++)
    {
        UmtsMacPhyStopListeningToChannel(
            node,
            macCellularData->myMacData->phyNumber,
            i);
    }

    // to start the listening
    if (umtsMacData->duplexMode == UMTS_DUPLEX_MODE_FDD)
    {
        if (macCellularData->nodeType == CELLULAR_UE)
        {
            UmtsMacPhyStartListeningToChannel(
                            node,
                            macCellularData->myMacData->phyNumber,
                            umtsMacData->ueMacVar->dlChannelList->at(
                                umtsMacData->ueMacVar->scanIndex));
        }
        else if (macCellularData->nodeType == CELLULAR_NODEB)
        {
            UmtsMacPhyStartListeningToChannel(
                            node,
                            macCellularData->myMacData->phyNumber,
                            umtsMacData->currentULChannel);
        }
    }
    else
    {
        //TODO: TDD
    }
}

// /**
// FUNCTION   :: UmtsMacNodeBInit
// LAYER      :: UMTS MAC
// PURPOSE    :: Init the NodeB MAC data
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + nodeBMacVar      : UmtsMacNodeBData* : NodeB Mac Data
// + interfaceIndex   : UInt32            : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void      : NULL
// **/
static
void UmtsMacNodeBInit(Node* node,
                      UmtsMacData* umtsMacData,
                      UmtsMacNodeBData* nodeBMacVar,
                      UInt32 interfaceIndex,
                      const NodeInput* nodeInput)
{
    // init log ch info list
    nodeBMacVar->nodeBLogChList = new NodeBLogChList;

    // init transport channel Info DL
    nodeBMacVar->nodeBTrChList = new NodeBTrChList;

    // store the mapping for the common logical channel
    // transport channel for this cell
    nodeBMacVar->nodeBLocalMap = new std::list <UmtsRbLogTrPhMapping*>;

    // store the info for each registered UE, in which
    // store the mapping  for the dedicated logical  and transport
    // channel for this registered UE
    nodeBMacVar->macNodeBUeInfo = new std::list <UmtsMacNodeBUeInfo*>;

    // init the tx/rx buffer
    umtsMacData->nodeBMacVar->commonTxBurstBuf = new std::deque<Message*>;
    umtsMacData->nodeBMacVar->commonRxBurstBuf = new std::deque<Message*>;
}

// /**
// FUNCTION   :: UmtsMacNodeBFinalize
// LAYER      :: UMTS MAC
// PURPOSE    :: Init the NodeB MAC data
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsMacData      : UmtsMacData*      : UMTS MAC Data
// + nodeBMacVar      : UmtsMacNodeBData* : NodeB Mac Data
// + interfaceIndex   : UInt32            : Interface index
// RETURN     :: void      : NULL
// **/
static
void UmtsMacNodeBFinalize(Node* node,
                          UmtsMacData* umtsMacData,
                          UmtsMacNodeBData* nodeBMacVar,
                          UInt32 interfaceIndex)
{
    // finalize log ch info list
    std::list<UmtsLogChInfo*>::iterator itLog;
    for (itLog = nodeBMacVar->nodeBLogChList->begin();
        itLog != nodeBMacVar->nodeBLogChList->end();
        itLog ++)
    {
        delete (*itLog);
        *itLog = NULL;
    }
    nodeBMacVar->nodeBLogChList->remove(NULL);
    delete nodeBMacVar->nodeBLogChList;

    // finalize transport channel Info DL
    UeTrChList::iterator itTr;

    for (itTr = nodeBMacVar->nodeBTrChList->begin();
         itTr != nodeBMacVar->nodeBTrChList->end();
         itTr ++)
    {
        delete (*itTr);
        *itTr = NULL;
    }
    nodeBMacVar->nodeBTrChList->remove(NULL);
    delete nodeBMacVar->nodeBTrChList;

    // finalize the mapping for the common logical channel
    // transport channel for this cell
    UmtsClearStlContDeep(nodeBMacVar->nodeBLocalMap);
    delete nodeBMacVar->nodeBLocalMap;

    // store the info for each registered UE, in which
    // store the mapping  for the dedicated logical  and transport
    // channel for this registered UE

    std::list<UmtsMacNodeBUeInfo*>::iterator ueMacInfoIt;
    UmtsMacNodeBUeInfo* nodebUeInfo = NULL;

    for (ueMacInfoIt = nodeBMacVar->macNodeBUeInfo->begin();
        ueMacInfoIt != nodeBMacVar->macNodeBUeInfo->end();
        ueMacInfoIt ++)
    {
        nodebUeInfo = (*ueMacInfoIt);
        UmtsMacNodeBFinalizeUeNodeBInfo(node, nodebUeInfo);
        *ueMacInfoIt = NULL;
    }
    nodeBMacVar->macNodeBUeInfo->remove(NULL);
    delete nodeBMacVar->macNodeBUeInfo;

    // Finalize tx/rx buffer
    // common rx/tx buffer
    std::for_each(umtsMacData->nodeBMacVar->commonTxBurstBuf->begin(),
              umtsMacData->nodeBMacVar->commonTxBurstBuf->end(),
              UmtsFreeStlMsgPtr(node));
    delete umtsMacData->nodeBMacVar->commonTxBurstBuf;
    std::for_each(umtsMacData->nodeBMacVar->commonRxBurstBuf->begin(),
              umtsMacData->nodeBMacVar->commonRxBurstBuf->end(),
              UmtsFreeStlMsgPtr(node));
    delete umtsMacData->nodeBMacVar->commonRxBurstBuf;
}

// /**
// FUNCTION   :: UmtsMacUeInit
// LAYER      :: UMTS MAC
// PURPOSE    :: Init the Ue MAC data
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsMacData      : UmtsMacData*      : UMTS MAC Data
// + ueMacVar         : UmtsMacUeData*    : UE MAC Data
// + interfaceIndex   : UInt32            : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void      : NULL
// **/
static
void UmtsMacUeInit(Node* node,
                   UmtsMacData* umtsMacData,
                   UmtsMacUeData* ueMacVar,
                   UInt32 interfaceIndex,
                   const NodeInput* nodeInput)
{
    // init dlChannelList
    ueMacVar->dlChannelList = new std::vector <int>;

    // init log ch  list
    ueMacVar->ueLogChList = new UeLogChList;

    //init Tr Ch List
    ueMacVar->ueTrChList = new UeTrChList;

    // init log tr phy mapping
    ueMacVar->ueLogTrPhMap = new std::list <UmtsRbLogTrPhMapping*>;

    // rx/tx buffer
    ueMacVar->txBurstBuf = new std::deque<Message*>;
    ueMacVar->rxBurstBuf = new std::deque<Message*>;
}

// /**
// FUNCTION   :: UmtsMacUeFinalize
// LAYER      :: UMTS MAC
// PURPOSE    :: Init the Ue MAC data
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsMacData      : UmtsMacData*      : UMTS MAC Data
// + ueMacVar         : UmtsMacUeData*    : UE MAC Data
// + interfaceIndex   : UInt32            : Interface index
// RETURN     :: void      : NULL
// **/
static
void UmtsMacUeFinalize(Node* node,
                       UmtsMacData* umtsMacData,
                       UmtsMacUeData* ueMacVar,
                       UInt32 interfaceIndex)
{
    // finalize log ch  list
    std::list<UmtsLogChInfo*>::iterator itLog;
    for (itLog = ueMacVar->ueLogChList->begin();
        itLog != ueMacVar->ueLogChList->end();
        itLog ++)
    {
        delete (*itLog);
        *itLog = NULL;
    }
    ueMacVar->ueLogChList->remove(NULL);
    delete ueMacVar->ueLogChList;

    //finalize Tr Ch List
    UeTrChList::iterator itTr;

    for (itTr = ueMacVar->ueTrChList->begin();
         itTr != ueMacVar->ueTrChList->end();
         itTr ++)
    {
        delete (*itTr);
        *itTr = NULL;
    }
    ueMacVar->ueTrChList->remove(NULL);
    delete ueMacVar->ueTrChList;

    // finalize log tr phy mapping
    UmtsClearStlContDeep(ueMacVar->ueLogTrPhMap);
    delete ueMacVar->ueLogTrPhMap;

    // rx/tx buffer
    std::for_each(ueMacVar->txBurstBuf->begin(),
              ueMacVar->txBurstBuf->end(),
              UmtsFreeStlMsgPtr(node));
    delete ueMacVar->txBurstBuf;
    std::for_each(ueMacVar->rxBurstBuf->begin(),
              ueMacVar->rxBurstBuf->end(),
              UmtsFreeStlMsgPtr(node));
    delete ueMacVar->rxBurstBuf;

    // MAC enetity
    if (ueMacVar->cshmEntity)
    {
        UmtsMacUeFinalizeCshmEntity(
            node, interfaceIndex, umtsMacData, ueMacVar);   
    }

    if (ueMacVar->bEntity)
    {
        MEM_free(ueMacVar->bEntity);
        ueMacVar->bEntity = NULL;   
    }

    if (ueMacVar->dEntity)
    {
        MEM_free(ueMacVar->dEntity);
        ueMacVar->dEntity = NULL;   
    }

    if (ueMacVar->dlChannelList != NULL)
    {
        delete ueMacVar->dlChannelList;
        ueMacVar->dlChannelList = NULL;
    }
}

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

//  /**
// FUNCITON   :: UmtsMacHandleInterLayerCommand
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Handle Interlayer command
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + cmdType          : UmtsInterlayerCommandType : command type
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
void UmtsMacHandleInterLayerCommand(
            Node* node,
            UInt32 interfaceIndex,
            UmtsInterlayerCommandType cmdType,
            void* cmd)
{
    if (UmtsIsNodeB(node))
    {
        UmtsMacNodeBHandleInterLayerCommand(
            node,
            interfaceIndex,
            cmdType,
            cmd);
    }
    else if (UmtsIsUe(node))
    {
        UmtsMacUeHandleInterLayerCommand(
            node,
            interfaceIndex,
            cmdType,
            cmd);
    }
}

// /**
// FUNCTION   :: UmtsMacInit
// LAYER      :: UMTS LAYER2 MAC Sublayer
// PURPOSE    :: Initialize UMTS MAC sublayer at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void UmtsMacInit(Node* node,
                 UInt32 interfaceIndex,
                 const NodeInput* nodeInput)
{
    UmtsMacData* umtsMacData;
    MacCellularData* macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;

    CellularUmtsLayer2Data* layer2Data =
        macCellularData->cellularUmtsL2Data;

    umtsMacData = UmtsMacGetSubLayerData(node, interfaceIndex);
    ERROR_Assert(umtsMacData == NULL, "Mac data is not NULL while init");

    umtsMacData = (UmtsMacData*) MEM_malloc(sizeof(UmtsMacData));
    memset(umtsMacData, 0, sizeof(UmtsMacData));
    layer2Data->umtsMacData = umtsMacData;
    umtsMacData->myMacCellularData = macCellularData;

    if (DEBUG_MAC_GENERAL)
    {
        UmtsLayer2PrintRunTimeInfo(
            node,
            interfaceIndex,
            UMTS_LAYER2_SUBLAYER_MAC);
        printf("MAC Init...\n");
    }

    if (UmtsIsRnc(node))
    {
        // TODO
    }
    else // nodeB and UE
    {
        // TODO : read init params
        umtsMacData->chipRateType = UMTS_CHIP_RATE_384;
        umtsMacData->duplexMode = UMTS_DUPLEX_MODE_FDD;

        // start the slot timer
        if (umtsMacData->chipRateType == UMTS_CHIP_RATE_384)
        {
            umtsMacData->chipDuration = UMTS_CHIP_DURATION_384;
            umtsMacData->slotDuration = UMTS_SLOT_DURATION_384;
        }

        // init the Log Trans CH
        if (UmtsIsNodeB(node))
        {
            umtsMacData->nodeBMacVar =
                (UmtsMacNodeBData*) MEM_malloc(sizeof(UmtsMacNodeBData));
            memset(umtsMacData->nodeBMacVar, 0, sizeof(UmtsMacNodeBData));

            UmtsMacNodeBInit(node,
                             umtsMacData,
                             umtsMacData->nodeBMacVar,
                             interfaceIndex,
                             nodeInput);

        }
        else if (UmtsIsUe(node))
        {
            umtsMacData->ueMacVar =
                (UmtsMacUeData*) MEM_malloc(sizeof(UmtsMacUeData));
            memset(umtsMacData->ueMacVar, 0, sizeof(UmtsMacUeData));

            UmtsMacUeInit(node,
                          umtsMacData,
                          umtsMacData->ueMacVar,
                          interfaceIndex,
                          nodeInput);

        }

        // Init channel list
        UmtsMacInitChannelList(node, interfaceIndex, nodeInput);

#ifdef PARALLEL //Parallel
        umtsMacData->lookaheadHandle = 
            PARALLEL_AllocateLookaheadHandle (node);
        PARALLEL_AddLookaheadHandleToLookaheadCalculator(
            node, umtsMacData->lookaheadHandle, 0);

        // Also sepcific a minimum lookahead, 
        // in case EOT can't be used in the sim.
        PARALLEL_SetMinimumLookaheadForInterface(
            node, UMTS_DELAY_UNTIL_SIGNAL_AIRBORN - 1); 
#endif //endParallel

        //Initialize slot timer
        umtsMacData->umtsMacSlotTimer  =
            UmtsLayer2InitTimer(node,
                                interfaceIndex,
                                UMTS_LAYER2_SUBLAYER_MAC,
                                UMTS_MAC_SLOT_TIMER,
                                0,
                                NULL);

        MESSAGE_Send(node, umtsMacData->umtsMacSlotTimer, 0);
#ifdef PARALLEL //Parallel
        PARALLEL_SetLookaheadHandleEOT(
            node,
            umtsMacData->lookaheadHandle,
            node->getNodeTime());
#endif //endParallel
    }
}

// /**
// FUNCTION   :: UmtsMacFinalize
// LAYER      :: UMTS LAYER2 MAC Sublayer
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// RETURN     :: void : NULL
// **/
void UmtsMacFinalize(Node* node, UInt32 interfaceIndex)
{
    MacCellularData* macCellularData;
    CellularUmtsLayer2Data *layer2Data;

    macCellularData = (MacCellularData*)
                      (node->macData[interfaceIndex]->macVar);

    layer2Data = macCellularData->cellularUmtsL2Data;

    UmtsMacData* mac = UmtsMacGetSubLayerData(node, interfaceIndex);
    ERROR_Assert(mac, "No Mac data associate with this interface");

    if (DEBUG_MAC_GENERAL)
    {
        UmtsLayer2PrintRunTimeInfo(
            node,
            interfaceIndex,
            UMTS_LAYER2_SUBLAYER_MAC);
        printf("MAC Finalize...\n");
    }

    if (UmtsIsNodeB(node))
    {
        // free the dynamic memory
        UmtsMacNodeBFinalize(
            node, mac, mac->nodeBMacVar, interfaceIndex);
        // free the main NodeBMacData memory
        MEM_free(mac->nodeBMacVar);
        mac->nodeBMacVar = NULL;
    }
    else if (UmtsIsUe(node))
    {
        // free the dynamic memoty
        UmtsMacUeFinalize(
            node, mac, mac->ueMacVar, interfaceIndex);
        // free the main UeMacData memory
        MEM_free(mac->ueMacVar);
        mac->ueMacVar = NULL;
    }

    // free the memory
    MEM_free(mac);
    layer2Data->umtsMacData = NULL;
}

// /**
// FUNCTION   :: UmtsMacLayer
// LAYER      :: UMTS LAYER2 MAC sublayer
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void UmtsMacLayer(Node* node, UInt32 interfaceIndex, Message* msg)
{
    MacCellularData* macCellularData;
    CellularUmtsLayer2Data *layer2Data;

    macCellularData = (MacCellularData*)
                      (node->macData[interfaceIndex]->macVar);

    layer2Data = macCellularData->cellularUmtsL2Data;

    UmtsMacData* umtsMac =layer2Data->umtsMacData; 
    ERROR_Assert(umtsMac, "No Mac data associate with this interface");

    if (DEBUG_MAC_GENERAL)
    {
        UmtsLayer2PrintRunTimeInfo(
            node,
            interfaceIndex,
            UMTS_LAYER2_SUBLAYER_MAC);
        printf("UMTS MAC Layer function\n");
    }

    switch (msg->eventType)
    {
        case MSG_MAC_TimerExpired:
        {
            UmtsMacHandleTimeout(node, interfaceIndex, umtsMac, msg);
            break;
        }
        default:
        {
            MESSAGE_Free(node, msg);
        }
    }
}
