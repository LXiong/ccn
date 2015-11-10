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
// software, hardware, product or service

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network.h"
#include "network_ip.h"
#include "cellular_gsm.h"
#include "phy_gsm.h"
#include "mac_gsm.h"
#include "layer3_gsm.h"

// Lower value means more debug information.

#define DEBUG 0
#define DEBUG_GSM_LAYER3_0 0
#define DEBUG_GSM_LAYER3_1 0
#define DEBUG_GSM_LAYER3_2 0
#define DEBUG_GSM_LAYER3_3 0
#define DEBUG_GSM_LAYER3_TIMERS 0

#define GSM_ADDITIONAL_STATS 0

/* NOTES:
 *
 * 1.)
 * Timer events meant for GSM Layer 3 - Network layer should
 * set protocolType field of a Message to NETWORK_PROTOCOL_GSM
 * to ensure they are delivered to the GSM Layer 3 layer function
 * rather than the IP layer function.
 *
 * THIS APPLIES TO ALL MESSAGES (MAC & LAYER 3) OF GSM.
 */

clocktype   DefaultGsmConnectionReleaseDelay    = 30 * SECOND;

//
// Utility functions
//

static
Message* GsmLayer3StartTimer(
    Node* node,
    int         timerType,
    clocktype   delay,
    void* msgData,
    // NULL if not required
    int         msgDataSize)    // 0 if no msgData is passed
{
    Message*            timerMsg;
    GsmLayer3Data*      nwGsmData;
    void*               timerData;

    GsmSlotUsageInfo*   slot;

    slot = ( GsmSlotUsageInfo * )msgData;

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    timerMsg = MESSAGE_Alloc( node,
                   NETWORK_LAYER,
                   NETWORK_PROTOCOL_GSM,
                   timerType );

    if (msgData != NULL && msgDataSize > 0 ){
        MESSAGE_InfoAlloc( node,timerMsg,msgDataSize );
        timerData = ( unsigned char* )MESSAGE_ReturnInfo( timerMsg );

        memcpy( timerData,msgData,msgDataSize );
    }

    timerMsg->protocolType = NETWORK_PROTOCOL_GSM;
    MESSAGE_SetInstanceId( timerMsg,( short )nwGsmData->interfaceIndex );
    MESSAGE_Send( node,timerMsg,delay );

    return timerMsg;
} // End of GsmLayer3StartTimer //


static
void GsmLayer3MsStopCallControlTimers(
    Node* node,
    GsmLayer3MsInfo* msInfo)
{
    if (DEBUG_GSM_LAYER3_TIMERS ){
        //  printf("GSM_MS[%d]: Stopping Call Control Timers\n", node->nodeId);
    }

    if (msInfo->timerT303Msg != NULL ){
        // printf("GSM_MS[%d]: Cancelling T303\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,msInfo->timerT303Msg );
        msInfo->timerT303Msg = NULL;
    }

    if (msInfo->timerT305Msg != NULL ){
        // printf("GSM_MS[%d]: Cancelling T305\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,msInfo->timerT305Msg );
        msInfo->timerT305Msg = NULL;
    }

    if (msInfo->timerT308Msg != NULL ){
        // printf("GSM_MS[%d]: Cancelling T308\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,msInfo->timerT308Msg );
        msInfo->timerT308Msg = NULL;
    }

    if (msInfo->timerT310Msg != NULL ){
        // printf("GSM_MS[%d]: Cancelling T310\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,msInfo->timerT310Msg );
        msInfo->timerT310Msg = NULL;
    }

    if (msInfo->timerT313Msg != NULL ){
        // printf("GSM_MS[%d]: Cancelling T313\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,msInfo->timerT313Msg );
        msInfo->timerT313Msg = NULL;
    }
} // End of GsmLayer3MsStopCallControlTimers //


static
void GsmLayer3MscStopCallControlTimers(
    Node* node,
    GsmMscCallInfo* callInfo)
{
    if (DEBUG_GSM_LAYER3_TIMERS ){
        //  printf("GSM_MSC[%d]: Stopping Call Control Timers, OrigImsi: ",
        //     node->nodeId);
        printGsmImsi( callInfo->originMs.imsi,10 );
        //  printf("\n");
    }

    if (callInfo->originMs.timerT301Msg != NULL ){
        // printf("GSM_MSC[%d]: Cancelling T301\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,callInfo->originMs.timerT301Msg );
        callInfo->originMs.timerT301Msg = NULL;
    }

    if (callInfo->originMs.timerT303Msg != NULL ){
        // printf("GSM_MSC[%d]: Cancelling T303\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,callInfo->originMs.timerT303Msg );
        callInfo->originMs.timerT303Msg = NULL;
    }

    if (callInfo->originMs.timerT310Msg != NULL ){
        // printf("GSM_MSC[%d]: Cancelling T310\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,callInfo->originMs.timerT310Msg );
        callInfo->originMs.timerT310Msg = NULL;
    }

    if (callInfo->originMs.timerT313Msg != NULL ){
        // printf("GSM_MSC[%d]: Cancelling T313\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,callInfo->originMs.timerT313Msg );
        callInfo->originMs.timerT313Msg = NULL;
    }

    if (callInfo->termMs.timerT301Msg != NULL ){
        // printf("GSM_MSC[%d]: Cancelling T301\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,callInfo->termMs.timerT301Msg );
        callInfo->termMs.timerT301Msg = NULL;
    }

    if (callInfo->termMs.timerT303Msg != NULL ){
        // printf("GSM_MSC[%d]: Cancelling T303\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,callInfo->termMs.timerT303Msg );
        callInfo->termMs.timerT303Msg = NULL;
    }

    if (callInfo->termMs.timerT310Msg != NULL ){
        // printf("GSM_MSC[%d]: Cancelling T310\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,callInfo->termMs.timerT310Msg );
        callInfo->termMs.timerT310Msg = NULL;
    }

    if (callInfo->termMs.timerT313Msg != NULL ){
        // printf("GSM_MSC[%d]: Cancelling T313\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,callInfo->termMs.timerT313Msg );
        callInfo->termMs.timerT313Msg = NULL;
    }

    if (callInfo->originMs.timerUDT1Msg != NULL ){
        // printf("GSM_MSC[%d]: Cancelling T313\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,callInfo->originMs.timerUDT1Msg );
        callInfo->originMs.timerUDT1Msg = NULL;
    }
    if (callInfo->termMs.timerUDT1Msg != NULL ){
        // printf("GSM_MSC[%d]: Cancelling T313\n", node->nodeId);
        MESSAGE_CancelSelfMsg( node,callInfo->termMs.timerUDT1Msg );
        callInfo->termMs.timerUDT1Msg = NULL;
    }
} // End of GsmMscStopCallControlTimers //


static
void GsmLayer3SendInterLayerMsg(
    Node* node,
    int             gsmInterfaceIndex,
    int             servicePrimitive,
    unsigned char* msgData,
    int             msgDataSize)
{
    GsmMacLayer3InternalMessage*
    internalMsg;


    internalMsg = (
        GsmMacLayer3InternalMessage * )MEM_malloc(
        sizeof( GsmMacLayer3InternalMessage ) + msgDataSize );

    internalMsg->servicePrimitive = ( unsigned char ) servicePrimitive;

    memcpy( internalMsg->msgData,msgData,msgDataSize );

    MacGsmReceiveInternalMessageFromLayer3( node,
        internalMsg,
        gsmInterfaceIndex );

    MEM_free( internalMsg );
} // End of GsmLayer3SendInterLayerMsg //

static
void GsmParseChannelRange(
    char* channelRangeString,
    uddtChannelIndex* channelRangeStart,
    uddtChannelIndex* channelRangeEnd)
{
    char*   firstPart;
    char    errorString[MAX_STRING_LENGTH];


    firstPart = channelRangeString;

    while (firstPart++ != NULL ){
        if (*firstPart == '-' ){
            char*   secondPart;

            secondPart = firstPart + 1;
            *firstPart = 0;

            *channelRangeStart = ( short )strtoul( channelRangeString,
                                              NULL,
                                              10 );
            *channelRangeEnd = ( short )strtoul( secondPart,NULL,10 );

            if (*channelRangeStart < 0 || *channelRangeEnd < 0
                || *channelRangeStart >= *channelRangeEnd
                || ( *channelRangeEnd - *channelRangeStart + 1 ) % 2 != 0 ){
                sprintf( errorString,
                    "Illegal channel range: %s-%s.\n"
                        "Channel range (inclusive) must be an even set "
                        "created in the main config file.",
                    channelRangeString,
                    secondPart );
                ERROR_ReportError( errorString );
            }

            return;
        }
    }

    *channelRangeStart = 0;
    *channelRangeEnd = 0;

    sprintf( errorString,
        "Incorrect format for channel range: %s",
        channelRangeString );
    ERROR_ReportError( errorString );
} // End of GsmParseChannelRange //


// Pack a string of single digit char's into a char string
// half the size such that: newString[0] = (old[1], old[0])
//                          newString[1] = (old[3], old[2])
void GsmLayer3ConvertToBcdNumber(
    unsigned char* bcdNumber,
    unsigned char* inputNumber,
    int inputNumLength)
{
    int i;

    for (i = 0; i < inputNumLength / 2; i++ ){
        bcdNumber[i] = ( unsigned char ) ( inputNumber[i * 2 + 1] << 4 );
        bcdNumber[i] = ( unsigned char )
        ( bcdNumber[i] | ( 0x0f & inputNumber[i * 2] ) );
    }
    // If inputNumLength is Odd then, padded 0xFX in the last digit
    if (inputNumLength % 2 ){
        bcdNumber[i] = ( unsigned char ) ( 0xF0
            | ( 0x0F & inputNumber[i * 2] ) );
    }
} // End of GsmLayer3ConvertToBcdNumber //


void GsmLayer3ConvertFromBcdNumber(
    unsigned char* outputNumber,
    unsigned char* bcdNumber,
    int inputNumLength)
{
    int i;

    for (i = 0; i < inputNumLength / 2; i++ ){
        outputNumber[i * 2] = ( unsigned char ) ( bcdNumber[i] & 0x0f );
        outputNumber[i * 2 + 1] = ( unsigned char )
        ( ( bcdNumber[i] & 0xf0 ) >> 4 );
    }

    // If inputNumLength is Odd then, padded extarct the last digit value
    if (inputNumLength % 2 ){
        outputNumber[i * 2] = ( unsigned char ) ( bcdNumber[i] & 0x0F );
    }
} // End of GsmLayer3ConvertFromBcdNumber //


// now Virtual traffic packet alloc only one byte as a data part.
// previously it was allocating GSM_BURST_DATA_SIZE bytes
// No need to initilize all dummy data with nodeId
static
void GsmLayer3BuildVirtualTrafficPacket(
    Node* node,
    Message** msg)
{
    unsigned char*  msgData;

    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,( *msg ),1,TRACE_GSM );

    msgData = ( unsigned char* )MESSAGE_ReturnPacket( ( *msg ) );

    // Traffic message (Set flag to FALSE)
    // This flag is used to simulate the steal bits
    // in a physical layer GSM normal burst frame.

    msgData[0] = FALSE;

    // Add virtual data
    MESSAGE_AddVirtualPayload( node,( *msg ),( GSM_BURST_DATA_SIZE - 1 ) );
} // End of GsmLayer3BuildVirtualTrafficPacket //

static
void GsmLayer3BuildOverAirTrafficMsg(
    Node* node,
    unsigned char* msgContent,
    Message** newTrafficMsg)
{
    unsigned char*  trafficPacketData;

    *newTrafficMsg = MESSAGE_Alloc( node,
                         NETWORK_LAYER,
                         0,
                         MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,
        ( *newTrafficMsg ),
        GSM_BURST_DATA_SIZE,
        TRACE_GSM );

    trafficPacketData = (
        unsigned char* )MESSAGE_ReturnPacket( ( *newTrafficMsg ) );

    memcpy( trafficPacketData,msgContent,GSM_BURST_DATA_SIZE );
} // End of GsmLayer3BuildOverAirTrafficMsg //


static
BOOL GsmLayer3BsLookupMsByTrafficConnId(
    GsmLayer3BsInfo* bsInfo,
    int trafficConnectionId,
    GsmSlotUsageInfo** msSlotInfo)
{
    if (trafficConnectionId >= 0
        && trafficConnectionId < GSM_MAX_MS_PER_BS ){
        if (bsInfo->trafficConnectionDesc[trafficConnectionId].inUse
            == TRUE ){
            *msSlotInfo =
            bsInfo->trafficConnectionDesc[trafficConnectionId].msSlotInfo;

            return TRUE;
        }
    }

    *msSlotInfo = NULL;

    return FALSE;
} // End of GsmLayer3BsLookupMsByTrafficConnId //


static
BOOL GsmLayer3BsLookupMsBySccpConnId(
    GsmLayer3BsInfo* bsInfo,
    int sccpConnectionId,
    GsmSlotUsageInfo** msSloInfo)
{
    ERROR_Assert( ( sccpConnectionId > 0 &&
                      sccpConnectionId <= GSM_MAX_MS_PER_BS ),
        "GSM_BS: Illegal SCCP Connection Id lookup" );

    if (bsInfo->sccpConnectionDesc[sccpConnectionId].inUse == TRUE ){
        *msSloInfo = bsInfo->sccpConnectionDesc[sccpConnectionId].msSlotInfo;
        return TRUE;
    }

    *msSloInfo = NULL;

    return FALSE;
} // End of GsmLayer3BsLookupMsBySccpConnId //


// Lookup a BS channel, timeslot that has been allocated (in use)
static
BOOL GsmLayer3BsLookupMsBySlotAndChannel(
    GsmLayer3BsInfo* bsInfo,
    uddtSlotNumber          slotNumber,
    uddtChannelIndex        upLinkChannelIndex,
    GsmChannelDescription** retChannelDesc)
{
    short                   channelPairIndex;

    GsmChannelDescription*  channelDesc;


    // find the channel pair index from the uplink channelIndex
    channelPairIndex = ( short )( ( upLinkChannelIndex -
                                      bsInfo->channelIndexStart - 1 ) / 2 );

    if (bsInfo->channelDesc[channelPairIndex].inUse == TRUE ){
        channelDesc = &( bsInfo->channelDesc[channelPairIndex] );

        if (channelDesc->slotUsageInfo[slotNumber].inUse == TRUE ){
            if (channelDesc->slotUsageInfo[slotNumber].ulChannelIndex ==
                   upLinkChannelIndex ){
                *retChannelDesc = &( bsInfo->channelDesc[channelPairIndex] );

                return TRUE;
            }
        }
    }

    *retChannelDesc = NULL;

    return FALSE;
} // End of GsmLayer3BsLookupMsBySlotAndChannel //


static
BOOL GsmLayer3MscLookupMsInVlrByIdentity(
    Node* node,
    GsmVlrEntry** retVlrEntry,
    unsigned char* identity,
    unsigned char   identityType)
{
    GsmLayer3Data*  nwGsmData;
    GsmVlrEntry*    vlrEntry;
    int             i;

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    for (i = 0; i < GSM_MAX_MS_PER_MSC; i++ ){
        vlrEntry = &( nwGsmData->mscInfo->gsmVlr[i] );

        if (vlrEntry->inUse == TRUE &&              // if entry is in use
        ( identityType == GSM_IMSI &&        // if imsi was passed in
        memcmp( vlrEntry->imsi,
            identity,
            GSM_MAX_IMSI_LENGTH ) == 0 ) ||
               ( identityType == GSM_MSISDN &&      // if msisdn was passed
               memcmp( vlrEntry->msIsdn,
                   identity,
                   GSM_MAX_MSISDN_LENGTH ) == 0 ) ){
            *retVlrEntry = vlrEntry;

            return TRUE;
        }
    }

    *retVlrEntry = NULL;

    return FALSE;
} // End of GsmLayer3MscLookupMsInVlrByIdentity //


static
void GsmLayer3AddTrafficHeader(
    Node* node,
    Message* msg,
    int     trafficConnectionId,
    unsigned char   messageTypeCode,
    int     packetSequenceNumber)
{
    GsmTrafficMessageOnNetworkHeader*   trafficHeader;

    MESSAGE_AddHeader( node,
        msg,
        sizeof( GsmTrafficMessageOnNetworkHeader ),
        TRACE_GSM );

    trafficHeader = (
        GsmTrafficMessageOnNetworkHeader * )MESSAGE_ReturnPacket( msg );
    memset( trafficHeader,0,sizeof( GsmTrafficMessageOnNetworkHeader ) );

    trafficHeader->trafficConnectionId = trafficConnectionId;

    trafficHeader->messageTypeCode = messageTypeCode;
    trafficHeader->packetSequenceNumber = packetSequenceNumber;
} // End of GsmLayer3AddTrafficHeader //


static
void GsmLayer3RemoveTrafficHeader(
    Node* node,
    Message* msg,
    int* trafficConnectionId,
    unsigned char* messageTypeCode,
    int* packetSequenceNumber)
{
    GsmTrafficMessageOnNetworkHeader*   trafficHeader;

    trafficHeader = (
        GsmTrafficMessageOnNetworkHeader * )MESSAGE_ReturnPacket( msg );

    *trafficConnectionId = trafficHeader->trafficConnectionId;

    *messageTypeCode = trafficHeader->messageTypeCode;

    *packetSequenceNumber = trafficHeader->packetSequenceNumber;

    MESSAGE_RemoveHeader( node,
        msg,
        sizeof( GsmTrafficMessageOnNetworkHeader ),
        TRACE_GSM );
} // End of GsmLayer3RemoveTrafficHeader //


/*
    The MSC will assign unique traffic connection Id's
    between 1 and GSM_MAX_SIMALT_CALLS.
*/
static
BOOL GsmLayer3MscAssignTrafficConnId(
    GsmLayer3MscInfo* mscInfo,
    int* trafficConnectionId)
{
    int i;

    for (i = 0; i < GSM_MAX_MS_PER_MSC ; i++ ){
        if (mscInfo->trafficConnInfo[i].inUse == TRUE ){
            continue;
        }
        else{
            *trafficConnectionId = i;
            mscInfo->numTrafficConnections++;
            mscInfo->trafficConnInfo[i].inUse = TRUE;

            if (DEBUG_GSM_LAYER3_2 ){
                printf( "GSM_MSC: Assigned TrafficId %d\n",i );
            }

            return TRUE;
        }
    }

    return FALSE;
} // End of GsmLayer3MscAssignTrafficConnId //


static
BOOL GsmLayer3MscReleaseTrafficConnId(
    GsmLayer3MscInfo* mscInfo,
    int trafficConnectionId)
{
    if (DEBUG_GSM_LAYER3_2 ){
        //  printf("GSM_MSC: Releasing TrafficId %d\n", trafficConnectionId);
    }

    if ((trafficConnectionId >= 0 &&
           trafficConnectionId <
           GSM_MAX_SIMALT_CALLS) &&
           (mscInfo->trafficConnInfo[trafficConnectionId].inUse == TRUE) ){
        mscInfo->trafficConnInfo[trafficConnectionId].inUse = FALSE;
        mscInfo->trafficConnInfo[trafficConnectionId].callInfo = NULL;
        mscInfo->numTrafficConnections--;

        return TRUE;
    }

    return FALSE;
} // End of GsmLayer3MscReleaseTrafficConnId //


static
BOOL GsmLayer3MscAssignVlrEntry(
    Node* node,
    GsmVlrEntry** vlrEntry)
{
    GsmLayer3Data*      nwGsmData;
    GsmLayer3MscInfo*   mscInfo;
    int                 i;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    ERROR_Assert( nwGsmData->nodeType == GSM_MSC,
        "GsmLayer3MscAssignVlrEntry called by a non-MSC node" );

    mscInfo = nwGsmData->mscInfo;

    // IMSI & MSISDN values are set to the same value.
    // A HLR lookup would be needed here to get the MSISDN & auth.
    // information for the MS.

    // Allocate an entry if one does not exist
    for (i = 0; i < GSM_MAX_MS_PER_MSC; i++ ){
        if (mscInfo->gsmVlr[i].inUse == FALSE ){
            mscInfo->gsmVlr[i].inUse = TRUE;
            *vlrEntry = &( mscInfo->gsmVlr[i] );

            return TRUE;
        }
    }

    *vlrEntry = NULL;
    return FALSE;
} // End of GsmLayer3MscAssignVlrEntry //


/*
    A BS assigns an SCCP connection Id between 1 to GSM_MAX_MS_PER_BS
    The MSC will identify a connection based on SCCP Id and BS Node
    address.
*/
static
BOOL GsmLayer3BsAssignSccpConnId(
    Node* node,
    GsmLayer3BsInfo* bsInfo,
    GsmSlotUsageInfo* msSlotInfo,
    int* sccpConnectionId)
{
    int i;

    ERROR_Assert( bsInfo->numSccpConnections <= GSM_MAX_MS_PER_BS,
        "GSM_BS ERROR: GSM_MAX_MS_PER_BS SCCP connections"
            " already assigned\n" );

    // SCCP id values are from 0 to GSM_MAX_MS_PER_BS - 1
    for (i = 1; i < GSM_MAX_MS_PER_BS; i++ ){
        if (bsInfo->sccpConnectionDesc[i].inUse == TRUE ||
               bsInfo->sccpConnectionDesc[i].useAfter > node->getNodeTime()){
            continue;
        }
        else{
            *sccpConnectionId = i;
            bsInfo->numSccpConnections++;
            bsInfo->sccpConnectionDesc[i].inUse = TRUE;

            // Set cross-reference to the channel desc
            msSlotInfo->sccpConnectionId = i;
            bsInfo->sccpConnectionDesc[i].msSlotInfo = msSlotInfo;
            bsInfo->stats.numSccpConnAssigned++;

            return TRUE;
        }
    }

    return FALSE;
} // End of GsmLayer3BsAssignSccpConnId //


static
BOOL GsmLayer3BsReleaseSccpConnId(
    Node* node,
    int        sccpConnectionId,
    GsmLayer3BsInfo* bsInfo)
{
    if (bsInfo->numSccpConnections <= 0 &&
           bsInfo->numSccpConnections >
           GSM_MAX_MS_PER_BS ||
           bsInfo->sccpConnectionDesc[sccpConnectionId].inUse == FALSE ){
        if (DEBUG_GSM_LAYER3_3 ){
            printf( "GSM_BS[%d] ERROR: "
                        "GsmLayer3BsReleaseSccpConnId %d failed\n",
                node->nodeId,
                sccpConnectionId );
        }

        return FALSE;
    }

    bsInfo->sccpConnectionDesc[sccpConnectionId].inUse = FALSE;
    bsInfo->sccpConnectionDesc[sccpConnectionId].useAfter =
        node->getNodeTime() + DefaultGsmConnectionReleaseDelay;
    bsInfo->sccpConnectionDesc[sccpConnectionId].msSlotInfo = NULL;
    bsInfo->numSccpConnections--;

    bsInfo->stats.numSccpConnReleased++;

    return TRUE;
} // End of GsmLayer3BsReleaseSccpConnId //


static
BOOL GsmLayer3BsAssignChannel(
    Node* node,
    GsmLayer3BsInfo* bsInfo,
    GsmSlotUsageInfo** retSlotInfo)
{
    BOOL                    wasAssigned = FALSE;

    char                    errorString[MAX_STRING_LENGTH];

    int                     chPairIndex;
    int                     slotIndex = -1;

    GsmChannelDescription*  channelDesc = NULL;


    // Channel pair#0 is C0 - Control channel
    for (chPairIndex = 0;
            chPairIndex < bsInfo->totalNumChannelPairs;
            chPairIndex++ ){
        channelDesc = &( bsInfo->channelDesc[chPairIndex] );

        if (channelDesc->inUse == TRUE ){
            for (slotIndex = 1;
                    slotIndex < GSM_SLOTS_PER_FRAME;
                    slotIndex++ ){
                if (channelDesc->slotUsageInfo[slotIndex].inUse == TRUE ){
                    continue;
                }
                else{
                    wasAssigned = TRUE;
                    break;
                }
            }
        }
        else{
            // Use the first timeslot available, 0 is reserved
            slotIndex = 1;
            channelDesc->inUse = TRUE;
            bsInfo->numChannelPairsInUse++;
            wasAssigned = TRUE;
            break;
        }

        if (wasAssigned == TRUE ){
            break;
        }
    } // End of search thru all channel pairs //

    if (wasAssigned == TRUE ){
        *retSlotInfo = &( channelDesc->slotUsageInfo[slotIndex] );

        memset(
        *retSlotInfo, 0, sizeof(GsmSlotUsageInfo));

        ( *retSlotInfo )->inUse = TRUE;
        ( *retSlotInfo )->slotNumber = ( uddtSlotNumber )slotIndex;

        ( *retSlotInfo )->channelPairIndex = ( short )chPairIndex;
        (
            *retSlotInfo )->ulChannelIndex = ( short )(
            bsInfo->channelIndexStart + chPairIndex * 2 + 1 );
        (
            *retSlotInfo )->dlChannelIndex = ( short )(
            bsInfo->channelIndexStart + chPairIndex * 2 );

        bsInfo->stats.numChannelsAssigned++;

        if (DEBUG_GSM_LAYER3_3 ){
            printf( "\nGSM_BS[%d]: AssignChannel Slot %d, Downlink.Ch %d\n",
                node->nodeId,
                ( *retSlotInfo )->slotNumber,
                ( *retSlotInfo )->dlChannelIndex );
        }

        return TRUE;
    }
    else{
        bsInfo->stats.channelAssignmentAttemptsFailed++;
        if (DEBUG_GSM_LAYER3_3 ){
            sprintf( errorString,
                "GSM_BS[%d] ERROR: AssignChannel Failed\n",
                node->nodeId );
            ERROR_ReportWarning( errorString );
        }

        return FALSE;
    }
} // End of GsmLayer3BsAssignChannel //


// Release the channel pair associated with the downLinkChannelIndex in BS
static
BOOL GsmLayer3BsReleaseChannel(
    Node* node,
    GsmSlotUsageInfo* slotInfo)
{
    int                     i;
    GsmLayer3Data*          nwGsmData;
    GsmLayer3BsInfo*        bsInfo;
    GsmChannelDescription*  channelDesc;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    bsInfo = nwGsmData->bsInfo;

    channelDesc = &( bsInfo->channelDesc[slotInfo->channelPairIndex] );

    if (DEBUG_GSM_LAYER3_3 ){
        printf( "\nGSM_BS[%d] ReleaseChannel: TS %d, CH %d\n",
            node->nodeId,
            slotInfo->slotNumber,
            slotInfo->ulChannelIndex );
    }

    if (bsInfo->numChannelPairsInUse < 1 || channelDesc->inUse == FALSE ||
           channelDesc->slotUsageInfo[slotInfo->slotNumber].inUse == FALSE ){
        if (DEBUG_GSM_LAYER3_3 ){
            printf( "GSM_BS[%d] ERROR: Attempt to release channel\n"
                        "TS %d, UL.Ch %d, CH.inUse %d, TS.inUse%d",
                node->nodeId,
                slotInfo->slotNumber,
                slotInfo->ulChannelIndex,
                channelDesc->inUse,
                slotInfo->inUse );
        }

        return FALSE;
    }

    slotInfo->inUse = FALSE;

    for (i = 0; i < GSM_SLOTS_PER_FRAME; i++ ){
        if (channelDesc->slotUsageInfo[i].inUse == TRUE ){
            // Atleast one slot in use
            break;
        }
    }

    // Deallocate entire channel
    if (i == GSM_SLOTS_PER_FRAME ){
        GsmChannelInfo  channelInfo;


        channelDesc->inUse = FALSE;
        channelInfo.channelIndex = slotInfo->ulChannelIndex;

        GsmLayer3SendInterLayerMsg( node,
            bsInfo->interfaceIndex,
            GSM_L3_MAC_CHANNEL_STOP_LISTEN,
            ( unsigned char* )&channelInfo,
            sizeof( GsmChannelInfo ) );
    }

    bsInfo->stats.numChannelsReleased++;

    return TRUE;
} // End of GsmLayer3BsReleaseChannel //


static
void GsmLayer3MsReleaseChannel(
    Node* node,
    GsmLayer3MsInfo* msInfo)
{
    char    errorString[MAX_STRING_LENGTH];

    // MS should send Layer 2 / Data Link messages to complete
    // the release/disconnect procedure. See GSM 04.06

    if (msInfo->rrState != GSM_MS_RR_STATE_DEDICATED ){
        if (DEBUG_GSM_LAYER3_3 ){
            sprintf( errorString,
                "GSM_MS[%d] ERROR: not in DEDICATED state\n",
                node->nodeId );
            ERROR_ReportWarning( errorString );
        }

        return;
    }
    else{
        GsmChannelInfo  channelInfo;

        if (DEBUG_GSM_LAYER3_3 ){
            printf( "GSM_MS[%d] ### End of RR connection ###.\n",
                node->nodeId );
        }

        channelInfo.channelIndex = msInfo->assignedChannelIndex;
        channelInfo.channelType = msInfo->assignedChannelType;
        channelInfo.slotNumber = msInfo->assignedSlotNumber;

        // Decode BCCH & perform cell selection upon release of TCH
        if (channelInfo.channelType == GSM_CHANNEL_TCH ){
            msInfo->isBcchDecoded = FALSE;
            msInfo->isSysInfoType2Decoded = FALSE;
            msInfo->isSysInfoType3Decoded = FALSE;
        }

        GsmLayer3SendInterLayerMsg( node,
            msInfo->interfaceIndex,
            GSM_L3_MAC_CHANNEL_RELEASE,
            ( unsigned char* )&channelInfo,
            sizeof( GsmChannelInfo ) );

        // Set T3212 - Periodic Update Timer
        if (DEBUG_GSM_LAYER3_TIMERS ){
            printf( "GSM_MS[%d]: Setting T3212\n",node->nodeId );
        }

        if (msInfo->timerT3212Msg != NULL ){
            MESSAGE_CancelSelfMsg( node, msInfo->timerT3212Msg );
            msInfo->timerT3212Msg = NULL;
        }

        msInfo->timerT3212Msg = GsmLayer3StartTimer(
            node,
            MSG_NETWORK_GsmPeriodicLocationUpdateTimer_T3212,
            DefaultGsmPeriodicLocationUpdateTimer_T3212Time,
            NULL,
            // Optional
            0 );

        msInfo->isDedicatedChannelAssigned = FALSE;
        msInfo->isAssignedChannelForTraffic = FALSE;
        msInfo->assignedSlotNumber = GSM_INVALID_SLOT_NUMBER;
        msInfo->assignedChannelIndex = GSM_INVALID_CHANNEL_INDEX;
        msInfo->assignedChannelType = GSM_INVALID_CHANNEL_TYPE;

        msInfo->stats.channelReleaseRcvd++;
    }
} // End of GsmLayer3MsReleaseChannel //


static
void GsmLayer3AddSccpHeader(
    Node* node,
    Message* msg,
    int sccpConnectionId,
    NodeAddress sourceNodeId,
    GsmSccpRoutingLabel* routingLabel,
    unsigned char   messageTypeCode)
{
    GsmSccpMsg* sccpHeader;


    MESSAGE_AddHeader( node,msg,sizeof( GsmSccpMsg ),TRACE_GSM );

    sccpHeader = ( GsmSccpMsg * )MESSAGE_ReturnPacket( msg );
    memset( sccpHeader,0,sizeof( GsmSccpMsg ) );

    sccpHeader->sccpConnectionId = sccpConnectionId;
    sccpHeader->sourceNodeId = sourceNodeId;
    memcpy( &( sccpHeader->routingLabel ),
        routingLabel,
        sizeof( GsmSccpRoutingLabel ) );

    sccpHeader->messageTypeCode = messageTypeCode;
} // End of GsmLayer3AddSccpHeader //


static
void GsmLayer3RemoveSccpHeader(
    Node* node,
    Message* msg,
    int* sccpConnectionId,
    NodeAddress* sourceNodeId,
    GsmSccpRoutingLabel* routingLabel,
    unsigned char* messageTypeCode)
{
    GsmSccpMsg* sccpMsg;

    sccpMsg = ( GsmSccpMsg * )MESSAGE_ReturnPacket( msg );
    *sccpConnectionId = sccpMsg->sccpConnectionId;
    *sourceNodeId = sccpMsg->sourceNodeId;
    memcpy( routingLabel,
        &( sccpMsg->routingLabel ),
        sizeof( GsmSccpRoutingLabel ) );

    *messageTypeCode = sccpMsg->messageTypeCode;

    MESSAGE_RemoveHeader( node,msg,sizeof( GsmSccpMsg ),TRACE_GSM );
} // End of GsmLayer3RemoveSccpHeader //


static
BOOL GsmLayer3MscLookupCallInfoBySccpConnId(
    Node* node,
    int             sccpConnectionId,
    NodeAddress     sourceNodeAddress,
    BOOL            isHandoverInProgress,
    GsmMscCallInfo** retCallInfo)
{
    GsmLayer3Data*      nwGsmData;
    GsmLayer3MscInfo*   mscInfo;
    GsmMscCallInfo*     callInfo;

    int                 i;

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;
    mscInfo = nwGsmData->mscInfo;


    for (i = 0; i < GSM_MAX_MS_PER_MSC; i++ ){
        callInfo = &( mscInfo->callInfo[i] );

        if (callInfo->inUse == TRUE ){
            if (isHandoverInProgress == FALSE &&
                   ( callInfo->originMs.sccpConnectionId ==
                       sccpConnectionId &&
                       callInfo->originMs.bsNodeAddress ==
                       sourceNodeAddress ) ||
                   (
                   callInfo->termMs.sccpConnectionId ==
                sccpConnectionId &&
                callInfo->termMs.bsNodeAddress == sourceNodeAddress ) ){
                *retCallInfo = callInfo;
                return TRUE;
            }
            else if (( callInfo->originMs.isBsTryingHo == TRUE &&
                          callInfo->originMs.hoSccpConnectionId ==
                          sccpConnectionId &&
                          callInfo->originMs.hoTargetBsNodeAddress ==
                          sourceNodeAddress ) ||
                        (
                        callInfo->termMs.isBsTryingHo ==
                    TRUE &&
                    callInfo->termMs.hoSccpConnectionId ==
                    sccpConnectionId &&
                    callInfo->termMs.hoTargetBsNodeAddress ==
                    sourceNodeAddress ) ){
                *retCallInfo = callInfo;
                return TRUE;
            }
        }
    }

    *retCallInfo = NULL;
    return FALSE;
} // End of GsmLayer3MscLookupCallInfoBySccpConnId //


static
BOOL GsmLayer3MscLookupCallInfoByTrafficConnId(
    GsmLayer3MscInfo* mscInfo,
    int     trafficConnectionId,
    GsmMscCallInfo** retCallInfo)
{
    if (mscInfo->trafficConnInfo[trafficConnectionId].inUse == TRUE ){
        *retCallInfo =
        mscInfo->trafficConnInfo[trafficConnectionId].callInfo;
        return TRUE;
    }

    return FALSE;
} // End of GsmLayer3MscLookupCallInfoByTrafficConnId //


static
BOOL GsmLayer3MscLookupCallInfoByImsi(
    Node* node,
    unsigned char* imsi,
    GsmMscCallInfo** retCallInfo)
{
    GsmLayer3Data*  nwGsmData;
    int             i;

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    if (nwGsmData->nodeType == GSM_MSC ){
        GsmLayer3MscInfo*   mscInfo;
        GsmMscCallInfo*     callInfo;


        mscInfo = nwGsmData->mscInfo;

        for (i = 0; i < GSM_MAX_MS_PER_MSC; i++ ){
            callInfo = &( mscInfo->callInfo[i] );

            if (callInfo->inUse == TRUE &&
                   ( memcmp( callInfo->originMs.imsi,
                         imsi,
                         GSM_MAX_IMSI_LENGTH ) == 0 ||
                       memcmp( callInfo->termMs.imsi,
                           imsi,
                           GSM_MAX_IMSI_LENGTH ) == 0 ) ){
                *retCallInfo = callInfo;
                return TRUE;
            }
        }
    }
    *retCallInfo = NULL;
    return FALSE;
} // End of GsmLayer3MscLookupCallInfoByImsi //


static
void GsmLayer3SendMsgOverIp(
    Node* node,
    NodeAddress destNodeAddress,
    Message* msg)
{
    unsigned char*  packetData;

    NodeAddress     sourceAddr;


    packetData = ( unsigned char* )MESSAGE_ReturnPacket( msg );

    sourceAddr = MAPPING_GetDefaultInterfaceAddressFromNodeId( node,
                     node->nodeId );
    NetworkIpSendRawMessage( node,
        msg,
        ANY_IP,
        //sourceAddr,
        destNodeAddress,
        //destinationAddress,
        ANY_INTERFACE,
        //outgoingInterface,
        IPTOS_PREC_INTERNETCONTROL,
        // priority
        IPPROTO_GSM,
        0 );                         // ttl, default is used
} // End of GsmLayer3SendMsgOverIp //


/* Name:        GsmLayer3AInterfaceSendTrafficMsg
 * Purpose:     Used to send traffic between the BSS & MSC.
 */
static
void GsmLayer3AInterfaceSendTrafficMsg(
    Node* node,
    NodeAddress     destNodeAddress,
    int             trafficConnectionId,
    unsigned char   messageTypeCode,
    int             packetSequenceNumber,
    unsigned char*  msgData,
    int             msgDataSize,
    int             actualMsgSize = 0)
{
    GsmLayer3Data*  nwGsmData;
    Message*        msg;

    unsigned char*  packetData;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    msg = MESSAGE_Alloc( node,
              NETWORK_LAYER,
              NETWORK_PROTOCOL_IP,
              MSG_NETWORK_FromTransportOrRoutingProtocol );

    MESSAGE_PacketAlloc( node,msg,msgDataSize,TRACE_GSM );

    packetData = ( unsigned char* )MESSAGE_ReturnPacket( msg );
    if (actualMsgSize == 0 ||
        (actualMsgSize != 0 && actualMsgSize == msgDataSize))
    memcpy( packetData,msgData,msgDataSize );
    else
        if (actualMsgSize != 0 && actualMsgSize != msgDataSize && 
            actualMsgSize < msgDataSize)
        {
            memset(packetData, 0 , msgDataSize);
            memcpy(packetData, msgData, actualMsgSize);
        }

    GsmLayer3AddTrafficHeader( node,
        msg,
        trafficConnectionId,
        messageTypeCode,
        packetSequenceNumber );

    GsmLayer3SendMsgOverIp( node,destNodeAddress,msg );
} // End of GsmLayer3AInterfaceSendTrafficMsg //


static
int GsmLayer3SetupTrafficConn(
    Node* node,
    NodeAddress destNodeAddress,
    int         sccpConnectionId)
{
    GsmLayer3Data*  nwGsmData;

    int             newTrafficConnectionId  = 0;

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    ERROR_Assert( GsmLayer3MscAssignTrafficConnId( nwGsmData->mscInfo,
                      &newTrafficConnectionId ) == TRUE,
        "GSM_MSC ERROR: Could not allocate a traffic connection Id" );

    //newTrafficConnectionId = nwGsmData->mscInfo->numTrafficConnections;
    //nwGsmData->mscInfo->numTrafficConnections++;

    GsmLayer3AInterfaceSendTrafficMsg( node,
        destNodeAddress,
        newTrafficConnectionId,
        GSM_TRAFFIC_MESSAGE_TYPE_CONNECT_REQUEST,
        0,
        // packet seq#
        ( unsigned char* )( char* )&sccpConnectionId,
        // data
        sizeof( int ) ); // data size

    return newTrafficConnectionId;
} // End of GsmLayer3SetupTrafficConn //


static
BOOL GsmLayer3MscAllocateCallInfo(
    Node* node,
    GsmMscCallInfo** retCallInfo)
{
    GsmLayer3Data*      nwGsmData;
    GsmLayer3MscInfo*   mscInfo;
    GsmMscCallInfo*     callInfo;

    int                 i;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;
    mscInfo = nwGsmData->mscInfo;

    for (i = 0; i < GSM_MAX_MS_PER_MSC; i++ ){
        callInfo = &( mscInfo->callInfo[i] );

        if (callInfo->inUse == TRUE ){
            continue;
        }
        else{
            memset( callInfo,0,sizeof( GsmMscCallInfo ) );

            callInfo->inUse = TRUE;

            *retCallInfo = callInfo;

            return TRUE;
        }
    }

    *retCallInfo = NULL;
    return FALSE;
} // End of GsmLayer3MscAllocateCallInfo //


//
// Functions to build Layer 3 GSM messages
//


/*
 * FUNCTION:    GsmLayer3BuildImmediateAssignmentMsg
 * PURPOSE:     Sent by BS to MS which sent the Channel Request Msg
 *
 * ASSUMPTION: The function is called at the 'right' time i.e. when
 *              the time slot for transmitting this message is
 *              scheduled).
 *
 * COMMENT:
 */
static
BOOL GsmLayer3BuildImmediateAssignmentMsg(
    Node* node,
    Message** msg,
    GsmChannelRequestMsg* chanReq,
    clocktype               timingAdvance)
{
    GsmLayer3Data*              nwGsmData;
    GsmLayer3BsInfo*            bsInfo;

    GsmChannelInfo              channelInfo;

    GsmImmediateAssignmentMsg*  immAssignMsg;
    GsmSlotUsageInfo*           assignedSlotInfo;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;
    bsInfo = nwGsmData->bsInfo;

    *msg = MESSAGE_Alloc( node,MAC_LAYER,0,MSG_MAC_StartTransmission );

    MESSAGE_PacketAlloc( node,
        *msg,
        sizeof( GsmImmediateAssignmentMsg ),
        TRACE_GSM );

    immAssignMsg = (
        GsmImmediateAssignmentMsg * )MESSAGE_ReturnPacket( ( *msg ) );

    memset( ( char* )immAssignMsg,0,sizeof( GsmImmediateAssignmentMsg ) );
    immAssignMsg->protocolDiscriminator = GSM_PD_RR;
    immAssignMsg->messageType = GSM_RR_MESSAGE_TYPE_IMMEDIATE_ASSIGNMENT;

    // Timing Advance in bit periods. 1 bit period = 48/13 US
    immAssignMsg->timingAdvance = ( unsigned char )
    ( ( timingAdvance )*( 13.0 / 48000.0 ) );

    if (GsmLayer3BsAssignChannel( node,bsInfo,&assignedSlotInfo ) == FALSE ){
        return FALSE;
    }

    bsInfo->stats.immediateAssignmentSent++;

    channelInfo.channelIndex = assignedSlotInfo->ulChannelIndex;
    channelInfo.slotNumber = assignedSlotInfo->slotNumber;

    // Set Timer T3101 with channel, timeslot context information.
    // It is stopped when the MS has correctly seized the channels.
    GsmLayer3StartTimer( node,
        MSG_NETWORK_GsmImmediateAssignmentTimer_T3101,
        DefaultGsmImmediateAssignmentTimer_T3101Time,
        &channelInfo,
        // passed as NULL if not required
        sizeof( GsmChannelInfo ) );

    GsmLayer3SendInterLayerMsg( node,
        bsInfo->interfaceIndex,
        GSM_L3_MAC_CHANNEL_START_LISTEN,
        ( unsigned char* )&channelInfo,
        sizeof( GsmChannelInfo ) );

    immAssignMsg->slotNumber = assignedSlotInfo->slotNumber;

    // Channel type variable in Imm.Assign. is not set since
    // only Early Assignment (of traffic channel) is supported.

    // ARFCN 0 => channel 0 for downlink, 1 for uplink
    //       1 => channel 2  "      "   , 3  "     "

    immAssignMsg->channelDescription = (
        short )( assignedSlotInfo->dlChannelIndex / 2 );

    // Copy from the values received in the Channel Request
    immAssignMsg->est_cause = chanReq->est_cause;
    immAssignMsg->rand_disc = chanReq->rand_disc;

    return TRUE;
} // End of GsmLayer3BuildImmediateAssignmentMsg //


static
void GsmLayer3SendChannelRequestMsg(
    Node* node,
    Message** msg,
    unsigned char   estCause)
{
    GsmLayer3MsInfo*        msInfo;
    GsmChannelRequestMsg*   chanReqMsg;

    GsmLayer3Data*          nwGsmData   = (
            GsmLayer3Data* )node->networkData.gsmLayer3Var;

    msInfo = nwGsmData->msInfo;

    *msg = MESSAGE_Alloc( node,MAC_LAYER,0,MSG_MAC_StartTransmission );

    MESSAGE_PacketAlloc( node,
        *msg,
        sizeof( GsmChannelRequestMsg ),
        TRACE_GSM );

    chanReqMsg = ( GsmChannelRequestMsg * )MESSAGE_ReturnPacket( ( *msg ) );

    memset( chanReqMsg,0,sizeof( GsmChannelRequestMsg ) );

    chanReqMsg->est_cause = estCause;

    // generate random number
    chanReqMsg->rand_disc =
        (unsigned char)(RANDOM_nrand( nwGsmData->randSeed ) % 256);
    msInfo->est_cause = chanReqMsg->est_cause;
    msInfo->rand_disc = chanReqMsg->rand_disc;

    // Initiation of Immediate Assignment
    msInfo->channelRequestMsg = *msg;
    msInfo->isRequestingChannel = TRUE;
    msInfo->numChannelRequestAttempts = 0;

    msInfo->slotsToWait = ( int )( RANDOM_erand( nwGsmData->randSeed ) *
                                     ( msInfo->txInteger - 1 ) );

    if (DEBUG_GSM_LAYER3_2 ){
        printf( "%d slots to wait new before CH-REQ, rand_disc %d\n",
            msInfo->slotsToWait,
            chanReqMsg->rand_disc );
    }

    msInfo->channelRequestTimer = GsmLayer3StartTimer( node,
                                      MSG_NETWORK_GsmChannelRequestTimer,
                                      msInfo->slotsToWait *
                                      DefaultGsmSlotDuration,
                                      NULL,
                                      // Optional
                                      0 );

    msInfo->stats.channelRequestSent++;
} // End of GsmLayer3SendChannelRequestMsg //


/* NAME:        GsmLayer3BuildCmServiceRequestMsg
 * PURPOSE:     Build CM Service Request message for initiating call setup
 * DIRECTION:   Call Originating MS -> Network
 * REFERENCE:   See GsmServiceRequestMsg in gsm_layer3.h
 * REFERENCE:   GSM 04.08 Sec 9.2.9
 *
 * COMMENT:     Should be sent in a L2/Mac SABM frame
 */
static
void GsmLayer3BuildCmServiceRequestMsg(
    Node* node,
    Message** msg)
{
    int                     i;

    GsmLayer3Data*          nwGsmData;
    GsmCmServiceRequestMsg* reqMsg;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    // Allocate the message & packet
    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );
    MESSAGE_PacketAlloc( node,
        ( *msg ),
        sizeof( GsmCmServiceRequestMsg ),
        TRACE_GSM );
    reqMsg = ( GsmCmServiceRequestMsg * )MESSAGE_ReturnPacket( ( *msg ) );

    // Set the values
    reqMsg->protocolDiscriminator = GSM_PD_MM;
    reqMsg->messageType = GSM_MM_MESSAGE_TYPE_CM_SERVICE_REQUEST;
    reqMsg->cmServiceType = GSM_CM_SERVICE_REQUEST_TYPE_MO_CALL;

    for (i = 0; i < GSM_MAX_IMSI_LENGTH; i++ ){
        reqMsg->gsmImsi[i] = nwGsmData->msInfo->gsmImsi[i];
    }
} // End of GsmLayer3BuildCmServiceRequestMsg //


static
void GsmLayer3BuildPagingResponseMsg(
    Node* node,
    Message** msg,
    unsigned char* destImsi)
{
    GsmPagingResponseMsg*   pageRespMsg;


    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,
        ( *msg ),
        sizeof( GsmPagingResponseMsg ),
        TRACE_GSM );

    pageRespMsg = ( GsmPagingResponseMsg * )MESSAGE_ReturnPacket( ( *msg ) );

    pageRespMsg->protocolDiscriminator = GSM_PD_RR;
    pageRespMsg->messageType = GSM_RR_MESSAGE_TYPE_PAGING_RESPONSE;
    memcpy( pageRespMsg->imsi,destImsi,GSM_MAX_IMSI_LENGTH );
} // End of GsmLayer3BuildPagingResponseMsg //


static
void GsmLayer3BuildLocationUpdateAcceptMsg(
    Node* node,
    GsmLocationUpdateAcceptMsg** locUpAcceptMsg,
    GsmLocationUpdateRequestMsg* locUpReqMsg,
    BOOL    followOnProceed)
{
    *locUpAcceptMsg = (
        GsmLocationUpdateAcceptMsg * )MEM_malloc(
        sizeof( GsmLocationUpdateAcceptMsg ) );

    ( *locUpAcceptMsg )->protocolDiscriminator = GSM_PD_MM;
    (
        *locUpAcceptMsg )->messageType =
        GSM_MM_MESSAGE_TYPE_LOCATION_UPDATE_ACCEPT;
    memcpy( ( *locUpAcceptMsg )->imsi,
        locUpReqMsg->imsi,
        GSM_MAX_IMSI_LENGTH );
    ( *locUpAcceptMsg )->lac = locUpReqMsg->lac;

    // Not used yet.
    //(*locUpAcceptMsg)->followOnProceed = followOnProceed;
} // End of GsmLayer3BuildLocationUpdateAcceptMsg //


/*
 * Reference:   GSM 04.08 Section 4
 *
 */
static
void GsmLayer3BuildLocationUpdateRequestMsg(
    Node* node,
    Message** msg,
    GsmLocationUpdatingType_e   locUpdType)
{
    GsmLocationUpdateRequestMsg*
    locUpReqMsg;
    GsmLayer3MsInfo*            msInfo;
    GsmLayer3Data*              nwGsmData;

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    msInfo = nwGsmData->msInfo;

    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,
        ( *msg ),
        sizeof( GsmLocationUpdateRequestMsg ),
        TRACE_GSM );

    locUpReqMsg = (
        GsmLocationUpdateRequestMsg * )MESSAGE_ReturnPacket( ( *msg ) );

    locUpReqMsg->protocolDiscriminator = GSM_PD_MM;
    locUpReqMsg->messageType = GSM_MM_MESSAGE_TYPE_LOCATION_UPDATE_REQUEST;

    locUpReqMsg->locationUpdatingType = ( unsigned char ) locUpdType;

    memcpy( locUpReqMsg->imsi,msInfo->gsmImsi,GSM_MAX_IMSI_LENGTH );
} // End of GsmLayer3BuildLocationUpdateRequestMsg //


/* NAME:        GsmLayer3BuildMoSetupMsg
 * PURPOSE:     Build the Mobile Originated (MO) Call Setup Message
 * DIRECTION:   Call MO MS -> Network
 * REFERENCE:   GSM 04.08 sec
 *
 */
static
void GsmLayer3BuildMoSetupMsg(
    Node* node,
    Message** msg)
{
    GsmMoSetupMsg*  setupMsg;
    GsmLayer3MsInfo*                        msInfo;
    GsmLayer3Data*  nwGsmData;

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    msInfo = nwGsmData->msInfo;

    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,( *msg ),sizeof( GsmMoSetupMsg ),TRACE_GSM );

    setupMsg = ( GsmMoSetupMsg * )MESSAGE_ReturnPacket( ( *msg ) );

    setupMsg->protocolDiscriminator = GSM_PD_CC;
    setupMsg->messageType = GSM_CC_MESSAGE_TYPE_SETUP;
    msInfo->numTransactions++;
    setupMsg->transactionIdentifier = (
        unsigned char ) msInfo->numTransactions;

    // See GSM 04.08, Section 7.3.3 & 5.2.2.3.2
    // (*setupMsg)->signal

    memcpy( setupMsg->calledPartyNumber,
        msInfo->destMsIsdn,
        GSM_MAX_MSISDN_LENGTH );
} // End of GsmLayer3BuildMoSetupMsg //


static
void GsmLayer3BuildCallConfirmedMsg(
    Node* node,
    Message** msg)
{
    GsmCallConfirmedMsg*                            callConfMsg;



    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,
        ( *msg ),
        sizeof( GsmCallConfirmedMsg ),
        TRACE_GSM );

    callConfMsg = ( GsmCallConfirmedMsg * )MESSAGE_ReturnPacket( ( *msg ) );

    callConfMsg->protocolDiscriminator = GSM_PD_CC;
    callConfMsg->messageType = GSM_CC_MESSAGE_TYPE_CALL_CONFIRMED;
    callConfMsg->transactionIdentifier = 1;
} // End of GsmLayer3BuildCallConfirmedMsg //


static
void GsmLayer3BuildMtAlertingMsg(
    Node* node,
    Message** msg)
{
    GsmMtAlertingMsg*   alertingMsg;


    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,
        ( *msg ),
        sizeof( GsmMtAlertingMsg ),
        TRACE_GSM );

    alertingMsg = ( GsmMtAlertingMsg * )MESSAGE_ReturnPacket( ( *msg ) );

    alertingMsg->protocolDiscriminator = GSM_PD_CC;
    alertingMsg->messageType = GSM_CC_MESSAGE_TYPE_ALERTING;
    alertingMsg->transactionIdentifier = 1;
} // End of GsmLayer3BuildMtAlertingMsg //


/* Purpose:     Call Terminating MS to Network
 * Reference:
 */
static
void GsmLayer3BuildMtConnectMsg(
    Node* node,
    Message** msg)
{
    GsmMtConnectMsg*                        connectMsg;


    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,( *msg ),sizeof( GsmMtConnectMsg ),TRACE_GSM );

    connectMsg = ( GsmMtConnectMsg * )MESSAGE_ReturnPacket( ( *msg ) );

    connectMsg->protocolDiscriminator = GSM_PD_CC;
    connectMsg->messageType = GSM_CC_MESSAGE_TYPE_CONNECT;
    connectMsg->transactionIdentifier = 1;
} // End of GsmLayer3BuildMtConnectMsg //


/* Purpose:     Call Originating MS to Network to ack start of call.
 * Reference:   GSM 04.08, Section 9.3.6
 */
static
void GsmLayer3BuildMoConnectAcknowledgeMsg(
    Node* node,
    Message** msg)
{
    GsmConnectAcknowledgeMsg*   connectAckMsg;


    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,( *msg ),sizeof( GsmMtConnectMsg ),TRACE_GSM );

    connectAckMsg = (
        GsmConnectAcknowledgeMsg * )MESSAGE_ReturnPacket( ( *msg ) );

    connectAckMsg->protocolDiscriminator = GSM_PD_CC;
    connectAckMsg->messageType = GSM_CC_MESSAGE_TYPE_CONNECT_ACKNOWLEDGE;
    connectAckMsg->transactionIdentifier = 1;
} // End of GsmLayer3BuildConnectAcknowledgeMsg //


static
void GsmLayer3BuildReleaseCompleteByMsMsg(
    Node* node,
    Message** msg)
{
    unsigned char*              msgContent;
    GsmReleaseCompleteByMsMsg*  relCompMsg;


    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,( *msg ),GSM_BURST_DATA_SIZE,TRACE_GSM );

    msgContent = ( unsigned char* )MESSAGE_ReturnPacket( ( *msg ) );

    // Set the 'stealing flag' to SIGNALLING, not traffic (voice/data)
    msgContent[0] = TRUE;

    // Set the rest of the disconnect message
    relCompMsg = ( GsmReleaseCompleteByMsMsg * )( msgContent + 1 );

    relCompMsg->protocolDiscriminator = GSM_PD_CC;
    relCompMsg->messageType = GSM_CC_MESSAGE_TYPE_RELEASE_COMPLETE;
    relCompMsg->transactionIdentifier = 1;

    relCompMsg->cause[0] = GSM_LOCATION_USER;
    relCompMsg->cause[1] = GSM_CAUSE_VALUE_NORMAL_CALL_CLEARING;
} // End of GsmLayer3BuildReleaseCompleteByMsMsg //


static
void GsmLayer3BuildReleaseByMsMsg(
    Node* node,
    Message** msg,
    unsigned char* cause)
{
    unsigned char*      msgContent;
    GsmReleaseByMsMsg*  relMsg;


    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,( *msg ),GSM_BURST_DATA_SIZE,TRACE_GSM );

    msgContent = ( unsigned char* )MESSAGE_ReturnPacket( ( *msg ) );

    // Set the 'stealing flag' to SIGNALLING, not traffic (voice/data)
    msgContent[0] = ( BOOL )TRUE;

    // Set the rest of the disconnect message
    relMsg = ( GsmReleaseByMsMsg * )( msgContent + 1 );

    relMsg->protocolDiscriminator = GSM_PD_CC;
    relMsg->messageType = GSM_CC_MESSAGE_TYPE_RELEASE;
    relMsg->transactionIdentifier = 1;

    relMsg->cause[0] = cause[0];
    relMsg->cause[1] = cause[1];
} // End of GsmLayer3BuildReleaseByMsMsg //


static
void startGsmVirtualTrafficPacketTimer(
    Node* node,
    GsmLayer3MsInfo* msInfo,
    clocktype           startTime)
{
    Message*                trafficTimerMsg;


    // Set a timer to initiate traffic
    trafficTimerMsg = MESSAGE_Alloc( node,
                          NETWORK_LAYER,
                          0,
                          MSG_NETWORK_GsmSendTrafficTimer );

    trafficTimerMsg->protocolType = NETWORK_PROTOCOL_GSM;

    MESSAGE_Send( node,trafficTimerMsg,startTime );
} // End of startGsmVirtualTrafficPacketTimer //


/*
 * Purpose:     Sent from MSC to BS to initiate terminating MS paging
 * Reference:   GSM 08.08, sec 3.2.1.19
 */

static
void GsmLayer3BuildPagingMsg(
    Node* node,
    GsmPagingMsg** pagingMsg,
    unsigned char   destImsi[GSM_MAX_IMSI_LENGTH],
    uddtLAC   lac,
    uddtCellId   cellIdentity)
{
    *pagingMsg = ( GsmPagingMsg * )MEM_malloc( sizeof( GsmPagingMsg ) );

    ( *pagingMsg )->messageTypeCode = GSM_PAGING;
    memcpy( ( *pagingMsg )->imsi,destImsi,GSM_MAX_IMSI_LENGTH );
    ( *pagingMsg )->lac = lac;
    ( *pagingMsg )->cellIdentity = cellIdentity;
} // End of GsmPagingMsg //


// Network -> MT
static
void GsmLayer3BuildMtSetupMsg(
    Node* node,
    GsmMtSetupMsg** mtSetupMsg,
    int                 transactionIdentifier,
    unsigned char* callOriginMsIsdn,
    unsigned char* callTerminatingMsIsdn)
{
    *mtSetupMsg = ( GsmMtSetupMsg * )MEM_malloc( sizeof( GsmMtSetupMsg ) );

    ( *mtSetupMsg )->protocolDiscriminator = GSM_PD_CC;
    ( *mtSetupMsg )->messageType = GSM_CC_MESSAGE_TYPE_SETUP;

    (
        *mtSetupMsg )->transactionIdentifier =
        ( unsigned char ) transactionIdentifier;

    GsmLayer3ConvertToBcdNumber( ( *mtSetupMsg )->callingPartyBcdNumber,
        callOriginMsIsdn,
        GSM_MAX_MSISDN_LENGTH );

    GsmLayer3ConvertToBcdNumber( ( *mtSetupMsg )->calledPartyBcdNumber,
        callTerminatingMsIsdn,
        GSM_MAX_MSISDN_LENGTH );
} // End of GsmLayer3BuildMtSetupMsg //


// suitableCellsIndex - int array which indexes the neighbour cell/channel
//                      information in GsmLayer3BsInfo struct
static
void GsmLayer3BuildHandoverRequiredMsg(
    GsmLayer3BsInfo* bsInfo,
    unsigned char       hoCause,
    int                 numCells,
    int* suitableCellsIndex,
    GsmHandoverRequiredMsg** hoReqdMsg)
{
    int n;

    *hoReqdMsg = (
        GsmHandoverRequiredMsg * )MEM_malloc(
        sizeof( GsmHandoverRequiredMsg ) );

    ( *hoReqdMsg )->messageType = GSM_HANDOVER_REQUIRED;
    ( *hoReqdMsg )->cause = hoCause;
    ( *hoReqdMsg )->numCells = numCells;

    for (n = 0; n < numCells; n++ ){
        ( *hoReqdMsg )->lac[n] = bsInfo->neighLac[suitableCellsIndex[n]];
        (
            *hoReqdMsg )->cellIdentity[n] =
            bsInfo->neighCellIdentity[suitableCellsIndex[n]];
    }
} // End of GsmLayer3BuildHandoverRequiredMsg //

static
void GsmLayer3BuildHandoverRequestMsg(
    uddtLAC servingBsLac,
    uddtCellId servingBsCellIdentity,
    uddtLAC targetBsLac,
    uddtCellId targetBsCellIdentity,
    unsigned char hoCause,
    int tempHoId,
    GsmHandoverRequestMsg** hoRequestMsg)
{
    *hoRequestMsg = (
        GsmHandoverRequestMsg * )MEM_malloc(
        sizeof( GsmHandoverRequestMsg ) );

    ( *hoRequestMsg )->messageType = GSM_HANDOVER_REQUEST;
    ( *hoRequestMsg )->servingBsLac = servingBsLac;
    ( *hoRequestMsg )->servingBsCellIdentity = servingBsCellIdentity;
    ( *hoRequestMsg )->targetBsLac = targetBsLac;
    ( *hoRequestMsg )->targetBsCellIdentity = targetBsCellIdentity;
    ( *hoRequestMsg )->cause = hoCause;
    ( *hoRequestMsg )->tempHoId = tempHoId;
} // End of GsmLayer3BuildHandoverRequestMsg //


// MSC to source BS
static
void GsmLayer3BuildHandoverRequiredRejectMsg(
    unsigned char                   cause,
    GsmHandoverRequiredRejectMsg** hoRejectMsg)
{
    *hoRejectMsg = (
        GsmHandoverRequiredRejectMsg * )MEM_malloc(
        sizeof( GsmHandoverRequiredRejectMsg ) );

    ( *hoRejectMsg )->messageType = GSM_HANDOVER_REQUIRED_REJECT;
    ( *hoRejectMsg )->cause = cause;
} // End of GsmLayer3BuildHandoverRequiredRejectMsg //

// BS to MSC
static
void GsmLayer3BuildHandoverFailureMsg(
    Node* node,
    GsmLayer3BsInfo* bsInfo,
    GsmSlotUsageInfo* assignedSlotInfo,
    unsigned char           cause,
    unsigned char           rrCause,
    GsmHandoverFailureMsg** hoFailureMsg)
{
    *hoFailureMsg = (
        GsmHandoverFailureMsg * )MEM_malloc(
        sizeof( GsmHandoverFailureMsg ) );

    ( *hoFailureMsg )->messageType = GSM_HANDOVER_FAILURE;
    ( *hoFailureMsg )->cause = cause;
    ( *hoFailureMsg )->rrCause = rrCause;
} // End of GsmLayer3BuildHandoverFailureMsg //


// Handover Target BS to MSC
static
void GsmLayer3BuildHandoverRequestAckMsg(
    Node* node,
    GsmLayer3BsInfo* bsInfo,
    GsmSlotUsageInfo* assignedSlotInfo,
    int                 tempHoId,
    GsmHandoverRequestAckMsg** hoReqAckMsg)
{
    GsmRIHandoverCommandMsg*                                riHoCmdMsg;


    *hoReqAckMsg = (
        GsmHandoverRequestAckMsg * )MEM_malloc(
        sizeof( GsmHandoverRequestAckMsg ) );

    ( *hoReqAckMsg )->messageType = GSM_HANDOVER_REQUEST_ACK;
    ( *hoReqAckMsg )->tempHoId = tempHoId;

    //(*hoReqAckMsg)->chosenChannel = GSM_CHANNEL_TCH;
    riHoCmdMsg = &( ( *hoReqAckMsg )->riHoCmdMsg );

    // build the GsmRIHandoverCommandMsg
    riHoCmdMsg->protocolDiscriminator = GSM_PD_RR;
    riHoCmdMsg->messageType = GSM_RR_MESSAGE_TYPE_RI_HANDOVER_COMMAND;
    riHoCmdMsg->bcchArfcn = ( unsigned char ) ( bsInfo->controlChannelIndex /
                                                  2 );
    riHoCmdMsg->slotNumber = ( unsigned char ) assignedSlotInfo->slotNumber;
    riHoCmdMsg->channelArfcn = (
        unsigned char ) ( assignedSlotInfo->dlChannelIndex / 2 );
    riHoCmdMsg->channelType = GSM_CHANNEL_TCH;

    riHoCmdMsg->startTime = node->getNodeTime() +
                                DefaultGsmHandoverStartTimeDelay;

    riHoCmdMsg->hoReference = ( unsigned char ) ( 0xff &
                                                    node->getNodeTime() );

    assignedSlotInfo->isHandoverInProgress = TRUE;
    assignedSlotInfo->hoReference = riHoCmdMsg->hoReference;
} // End of GsmLayer3BuildHandoverRequestAckMsg //


static
void GsmLayer3BuildHandoverCommandMsg(
    Node* node,
    //int         lac,
    //int         cellIdentity,
    GsmHandoverRequestAckMsg* hoReqAckMsg,
    GsmHandoverCommandMsg** hoCmdMsg)
{
    *hoCmdMsg = (
        GsmHandoverCommandMsg * )MEM_malloc(
        sizeof( GsmHandoverCommandMsg ) );

    ( *hoCmdMsg )->messageType = GSM_HANDOVER_COMMAND;
    //(*hoCmdMsg)->chosenChannel =
    //(*hoCmdMsg)->lac =
    //(*hoCmdMsg)->cellIdentity =
    memcpy( &( ( *hoCmdMsg )->riHoCmdMsg ),
        &( hoReqAckMsg->riHoCmdMsg ),
        sizeof( GsmRIHandoverCommandMsg ) );
} // End of GsmLayer3BuildHandoverCommandMsg //


// Send from HO originating BS to MS
static
void GsmLayer3BuildRIHandoverCommandMsg(
    Node* node,
    GsmHandoverCommandMsg* hoCmd,
    Message** msg)
{
    unsigned char*  packetData;


    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,
        ( *msg ),
        sizeof( GsmRIHandoverCommandMsg ) + 1,
        TRACE_GSM );

    packetData = ( unsigned char* )MESSAGE_ReturnPacket( ( *msg ) );
    *packetData = TRUE;    // set the steal flag for sig msg on TCH

    memcpy( packetData + 1,
        &( hoCmd->riHoCmdMsg ),
        sizeof( GsmRIHandoverCommandMsg ) );
} // End of GsmLayer3BuildRIHandoverCommandMsg //


static
void GsmLayer3BuildRIHandoverAccessMsg(
    Node* node,
    unsigned char hoReference,
    Message** msg)
{
    unsigned char*          packetData;
    GsmRIHandoverAccessMsg* riHoAccessMsg;


    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,
        ( *msg ),
        sizeof( GsmRIHandoverAccessMsg ) + 1,
        TRACE_GSM );

    packetData = ( unsigned char* )MESSAGE_ReturnPacket( ( *msg ) );
    *packetData = TRUE;
    riHoAccessMsg = ( GsmRIHandoverAccessMsg * )( packetData + 1 );
    riHoAccessMsg->hoReference = hoReference;
}  // End of GsmLayer3BuildRIHandoverAccessMsg //


static
void GsmLayer3BuildRIHandoverCompleteMsg(
    Node* node,
    Message** msg)
{
    unsigned char*              packetData;
    GsmRIHandoverCompleteMsg*   riHoCompMsg;


    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,
        ( *msg ),
        sizeof( GsmRIHandoverCompleteMsg ) + 1,
        TRACE_GSM );

    packetData = ( unsigned char* )MESSAGE_ReturnPacket( ( *msg ) );
    *packetData = TRUE;    // set the steal flag for sig msg on TCH

    riHoCompMsg = ( GsmRIHandoverCompleteMsg * )( packetData + 1 );
    riHoCompMsg->protocolDiscriminator = GSM_PD_RR;
    riHoCompMsg->messageType = GSM_RR_MESSAGE_TYPE_RI_HANDOVER_COMPLETE;
    riHoCompMsg->rrCause = GSM_RR_CAUSE_NORMAL_EVENT;
} // End of GsmLayer3BuildRIHandoverCompleteMsg //


static
void GsmLayer3BuildHandoverCompleteMsg(
    Node* node,
    GsmHandoverCompleteMsg** hoCompMsg)
{
    *hoCompMsg = (
        GsmHandoverCompleteMsg * )MEM_malloc(
        sizeof( GsmHandoverCompleteMsg ) );
    ( *hoCompMsg )->messageType = GSM_HANDOVER_COMPLETE;
    ( *hoCompMsg )->rrCause = GSM_RR_CAUSE_NORMAL_EVENT;
} // End of GsmLayer3BuildHandoverCompleteMsg //


static
void GsmLayer3BuildClearCommandMsg(
    Node* node,
    GsmClearCommandMsg** clearComMsg)
{
    *clearComMsg = (
        GsmClearCommandMsg * )MEM_malloc( sizeof( GsmClearCommandMsg ) );
    ( *clearComMsg )->messageType = GSM_CLEAR_COMMAND;
} // End of GsmLayer3BuildClearCommandMsg //


static
void GsmLayer3BuildCallClearCommandMsg(
    Node* node,
    GsmCallClearCommandMsg** callclearComMsg)
{
    *callclearComMsg = (
        GsmCallClearCommandMsg * )MEM_malloc(
        sizeof( GsmCallClearCommandMsg ) );
    ( *callclearComMsg )->messageType = GSM_CALL_CLEAR_COMMAND;
    // Call completion failed
    ( *callclearComMsg )->cause = GSM_CAUSE_CALL_COMPLETION_FAILED;
} // End of GsmLayer3BuildClearCommandMsg //


static
void GsmLayer3BuildClearCompleteMsg(
    Node* node,
    GsmClearCompleteMsg** clearCompMsg)
{
    *clearCompMsg = (
        GsmClearCompleteMsg * )MEM_malloc( sizeof( GsmClearCompleteMsg ) );
    ( *clearCompMsg )->messageType = GSM_CLEAR_COMPLETE;
} // End of GsmLayer3BuildClearCompleteMsg //

static
void GsmLayer3BuildCompleteLayer3InfoMsg(
    Node* node,
    GsmLayer3BsInfo* bsInfo,
    unsigned char* layer3MsgFromMs,
    int                  sizeoflayer3MsgFromMs,
    GsmCompleteLayer3InfoMsg** completeL3Msg)
{
    /* layer3MsgFromMs can be
    GSM_RR_MESSAGE_TYPE_PAGING_RESPONSE
    GSM_MM_MESSAGE_TYPE_CM_SERVICE_REQUEST
    */

    *completeL3Msg = (
        GsmCompleteLayer3InfoMsg * )MEM_malloc(
        sizeof( GsmCompleteLayer3InfoMsg ) + sizeoflayer3MsgFromMs );

    ( *completeL3Msg )->messageType = GSM_COMPLETE_LAYER3_INFORMATION;

    ( *completeL3Msg )->lac = returnGSMNodeLAC( node );
    ( *completeL3Msg )->cellIdentity = returnGSMNodeCellId( node );
    // Should the entire message be copied???

    memcpy( &( *completeL3Msg )->layer3Message,
           layer3MsgFromMs,
        sizeoflayer3MsgFromMs );
} // End of GsmLayer3BuildCompleteLayer3InfoMsg //


/*
 * Purpose: From BS to MS to inform of impending call.
 *          Causing MS to request a channel to be setup.
 */
static
void GsmLayer3BuildPagingRequestMsg(
    Node* node,
    Message** msg,
    unsigned char* destImsi)
{
    GsmPagingRequestType1Msg*   pageReq;

    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,
        ( *msg ),
        sizeof( GsmPagingRequestType1Msg ),
        TRACE_GSM );

    pageReq = ( GsmPagingRequestType1Msg * )MESSAGE_ReturnPacket( ( *msg ) );

    pageReq->protocolDiscriminator = GSM_PD_RR;
    pageReq->messageType = GSM_RR_MESSAGE_TYPE_PAGING_REQUEST_TYPE1;
    pageReq->pageMode = GSM_PAGING_MODE_NORMAL;
    pageReq->channelNeeded = GSM_CHANNEL_TCH;
    memcpy( pageReq->imsi,destImsi,GSM_MAX_IMSI_LENGTH );
} // End of GsmLayer3BuildPagingRequestMsg //


/* Purpose:     Sent by the MS to request the network to clear an
 *              end-to-end connection.
 *
 * Comment:     Sent by Call Originating MS when it wants to end the call.
 *
 *              It is sent as a signalling message in the traffic channel.
 *              So the first byte of the message is set to TRUE (signalling)
 *              instead of the GSM standard way: set the two bits closest to
 *              the normal burst training sequence to 1 in the physical
 *              layer frame.
 */
static
void GsmLayer3BuildDisconnectByMsMsg(
    Node* node,
    Message** msg,
    unsigned char* cause)
{
    unsigned char*          msgContent;
    GsmDisconnectByMsMsg*   disconnectMsg;


    *msg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

    MESSAGE_PacketAlloc( node,( *msg ),GSM_BURST_DATA_SIZE,TRACE_GSM );

    msgContent = ( unsigned char* )MESSAGE_ReturnPacket( ( *msg ) );

    // Set the 'stealing flag' to SIGNALLING, not traffic (voice/data)
    msgContent[0] = ( BOOL )TRUE;

    // Set the rest of the disconnect message
    disconnectMsg = ( GsmDisconnectByMsMsg * )( msgContent + 1 );

    disconnectMsg->protocolDiscriminator = GSM_PD_CC;
    disconnectMsg->messageType = GSM_CC_MESSAGE_TYPE_DISCONNECT;
    disconnectMsg->transactionIdentifier = 1;

    disconnectMsg->cause[0] = cause[0];
    disconnectMsg->cause[1] = cause[1];
} // End of GsmLayer3BuildDisconnectByMsMsg //


static
void GsmLayer3BuildReleaseCompleteByNetworkMsg(
    Node* node,
    GsmReleaseCompleteByNetworkMsg** relCompMsg)
{
    *relCompMsg = (
        GsmReleaseCompleteByNetworkMsg * )MEM_malloc(
        sizeof( GsmReleaseCompleteByNetworkMsg ) );

    ( *relCompMsg )->protocolDiscriminator = GSM_PD_CC;
    ( *relCompMsg )->messageType = GSM_CC_MESSAGE_TYPE_RELEASE_COMPLETE;
    ( *relCompMsg )->transactionIdentifier = 1;

    ( *relCompMsg )->cause[0] = GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER;
    ( *relCompMsg )->cause[1] = GSM_CAUSE_VALUE_NORMAL_CALL_CLEARING;
} // End of GsmLayer3BuildReleaseCompleteByNetworkMsg //


static
void GsmLayer3BuildReleaseByNetworkMsg(
    Node* node,
    GsmReleaseByNetworkMsg** releaseMsg,
    unsigned char cause1,
    unsigned char cause2)
{
    *releaseMsg = (
        GsmReleaseByNetworkMsg * )MEM_malloc(
        sizeof( GsmReleaseByNetworkMsg ) );

    ( *releaseMsg )->protocolDiscriminator = GSM_PD_CC;
    ( *releaseMsg )->messageType = GSM_CC_MESSAGE_TYPE_RELEASE;
    ( *releaseMsg )->transactionIdentifier = 1;

    ( *releaseMsg )->cause[0] = cause1;
    ( *releaseMsg )->cause[1] = cause2;
} // End of GsmLayer3BuildReleaseMsg //


static
void GsmLayer3BuildChannelReleaseMsg(
    Node* node,
    GsmChannelReleaseMsg** channelRelMsg)
{
    *channelRelMsg = (
        GsmChannelReleaseMsg * )MEM_malloc( sizeof( GsmChannelReleaseMsg ) );

    ( *channelRelMsg )->protocolDiscriminator = GSM_PD_RR;
    ( *channelRelMsg )->messageType = GSM_RR_MESSAGE_TYPE_CHANNEL_RELEASE;

    ( *channelRelMsg )->rrCause = GSM_RR_CAUSE_NORMAL_EVENT;
} // End of GsmLayer3BuildChannelReleaseMsg //


static
void GsmLayer3BuildDisconnectByNetworkMsg(
    Node* node,
    GsmDisconnectByNetworkMsg** disconnectMsg,
    unsigned char cause1,
    unsigned char cause2)
{
    *disconnectMsg = (
        GsmDisconnectByNetworkMsg * )MEM_malloc(
        sizeof( GsmDisconnectByNetworkMsg ) );

    ( *disconnectMsg )->protocolDiscriminator = GSM_PD_CC;
    ( *disconnectMsg )->messageType = GSM_CC_MESSAGE_TYPE_DISCONNECT;
    ( *disconnectMsg )->transactionIdentifier = 1;

    //(*disconnectMsg)->cause[0] = GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER;
    //(*disconnectMsg)->cause[1] = GSM_CAUSE_VALUE_NORMAL_CALL_CLEARING;

    ( *disconnectMsg )->cause[0] = cause1;
    ( *disconnectMsg )->cause[1] = cause2;

    (
        *disconnectMsg )->progressIndicator[0] =
        GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER;
    (
        *disconnectMsg )->progressIndicator[1] =
        GSM_PROGRESS_INDICATOR_END_TO_END_PLMN_ISDN_CALL;
} // End of GsmLayer3BuildDisconnectByNetworkMsg //


static
void GsmLayer3BuildMoAlertingMsg(
    Node* node,
    GsmMoAlertingMsg** moAlertingMsg,
    int transactionIdentifier)
{
    *moAlertingMsg = (
        GsmMoAlertingMsg * )MEM_malloc( sizeof( GsmMoAlertingMsg ) );

    ( *moAlertingMsg )->protocolDiscriminator = GSM_PD_CC;
    ( *moAlertingMsg )->messageType = GSM_CC_MESSAGE_TYPE_ALERTING;
    (
        *moAlertingMsg )->transactionIdentifier =
        ( unsigned char ) transactionIdentifier;
} // End of GsmLayer3BuildMoAlertingMsg //


static
void GsmLayer3BuildMtConnectAcknowledgeMsg(
    Node* node,
    GsmConnectAcknowledgeMsg** conAckMsg)
{
    *conAckMsg = (
        GsmConnectAcknowledgeMsg * )MEM_malloc(
        sizeof( GsmConnectAcknowledgeMsg ) );

    ( *conAckMsg )->protocolDiscriminator = GSM_PD_CC;
    ( *conAckMsg )->messageType = GSM_CC_MESSAGE_TYPE_CONNECT_ACKNOWLEDGE;
    ( *conAckMsg )->transactionIdentifier = 1;
} // End of GsmLayer3BuildMtConnectAcknowledgeMsg //


static
void GsmLayer3BuildMoConnectMsg(
    Node* node,
    GsmMoConnectMsg** moConnectMsg,
    unsigned char* connectedNumber)
{
    *moConnectMsg = (
        GsmMoConnectMsg * )MEM_malloc( sizeof( GsmMoConnectMsg ) );

    ( *moConnectMsg )->protocolDiscriminator = GSM_PD_CC;
    ( *moConnectMsg )->messageType = GSM_CC_MESSAGE_TYPE_CONNECT;
    ( *moConnectMsg )->transactionIdentifier = 1;

    memcpy( ( *moConnectMsg )->connectedNumber,
        connectedNumber,
        GSM_MAX_MSISDN_LENGTH );
} // End of GsmLayer3BuildMoConnectMsg //



static
void GsmLayer3BuildCallProceedingMsg(
    Node* node,
    GsmCallProceedingMsg** callProcMsg,
    unsigned char   transactionIdentifier)
{
    *callProcMsg = (
        GsmCallProceedingMsg * )MEM_malloc( sizeof( GsmCallProceedingMsg ) );

    ( *callProcMsg )->protocolDiscriminator = GSM_PD_CC;
    ( *callProcMsg )->messageType = GSM_CC_MESSAGE_TYPE_CALL_PROCEEDING;
    ( *callProcMsg )->transactionIdentifier = transactionIdentifier;

    (
        *callProcMsg )->progressIndicator =
        GSM_PROGRESS_INDICATOR_END_TO_END_PLMN_ISDN_CALL;
} // End of GsmLayer3BuildCallProceedingMsg //


/* NAME:        GsmLayer3BuildCmServiceAcceptMsg
 * PURPOSE:     Build the Cm Service Accept Message
 * DIRECTION:   Network -> Call Originating MS which sent CM Serv.Req.
 * REFERENCE:   GSM 04.08 sec
 * ASSUMPTION:
 *
 * COMMENT:
 */
static
void GsmLayer3BuildCmServiceAcceptMsg(
    Node* node,
    GsmCmServiceAcceptMsg** acceptMsg)
{
    GsmLayer3Data*  nwGsmData;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    *acceptMsg = (
        GsmCmServiceAcceptMsg * )MEM_malloc(
        sizeof( GsmCmServiceAcceptMsg ) );

    // Set the values
    ( *acceptMsg )->protocolDiscriminator = GSM_PD_MM;
    ( *acceptMsg )->messageType = GSM_MM_MESSAGE_TYPE_CM_SERVICE_ACCEPT;
} // End of GsmLayer3BuildCmServiceAcceptMsg //


void printGsmImsi(
    unsigned char* imsiStr,
    int length)
{
    int i;

    for (i = 0; i < length; i++ ){
        printf( "%d",imsiStr[i] );
    }
}


// Layer 3 processing functions

// RR - Radio Resource management sub-layer functions

/*
 * Name:        gsmMsProcessRrMessagesFromMac
 * Purpose:     Provides the RR sublayer (Layer3) functionality of a GSM MS
 *              Processes Radio Resource Management messages received from
 *              the Mac/Layer2 in the GSM MS.
 *
 * Comment:     Called from function GsmLayer3ReceivePacketFromMac
 */
static
void gsmMsProcessRrMessagesFromMac(
    Node* node,
    unsigned char* msgContent,
    uddtSlotNumber   receivedSlotNumber,
    uddtChannelIndex   receivedChannelIndex)
{
    char                    errorString[MAX_STRING_LENGTH];

    clocktype               currentTime;
    GsmLayer3Data*          nwGsmData;
    GsmLayer3MsInfo*        msInfo;

    Message*                newMsg;
    GsmGenericLayer3MsgHdr* genL3MsgHdr;


    currentTime = node->getNodeTime();

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    msInfo = nwGsmData->msInfo;

    genL3MsgHdr = ( GsmGenericLayer3MsgHdr * )msgContent;

    if (DEBUG_GSM_LAYER3_0 ){
        printf( "GSM_MS[%d] TS %d CH %d Received a RR Msg: ",
            node->nodeId,
            receivedSlotNumber,
            receivedChannelIndex );
    }

    switch ( genL3MsgHdr->messageType ){
        case GSM_RR_MESSAGE_TYPE_SYSTEM_INFORMATION_TYPE2:
            {
                int                 i;

                Message*            reqMsg;
                GsmSysInfoType2Msg* sysInfoType2Msg;


                if (msInfo->isBcchDecoded == TRUE &&
                       msInfo->downLinkControlChannelIndex ==
                       receivedChannelIndex ){
                    return;
                }

                if (msInfo->isSysInfoType2Decoded == TRUE &&
                       msInfo->isBcchDecoded == FALSE ){
                    return;
                }

                // System info messages  (2 & 3) reach here from MAC
                // only if a new cell has been selected.

                if (DEBUG_GSM_LAYER3_3 ){
                    printf( "MS[%d] SYS2: BCCH Decoded, CH %d\n",
                        node->nodeId,
                        receivedChannelIndex );
                }

                sysInfoType2Msg = ( GsmSysInfoType2Msg * )msgContent;

                msInfo->isSysInfoType2Decoded = TRUE;
                msInfo->sysInfoType2DecodedAt = currentTime;
                msInfo->txInteger = sysInfoType2Msg->txInteger;
                msInfo->maxReTrans = sysInfoType2Msg->maxReTrans;

                msInfo->numNeighBcchChannels =
                sysInfoType2Msg->numNeighBcchChannels;

                for (i = 0; i < msInfo->numNeighBcchChannels; i++ ){
                    msInfo->downLinkNeighBcchChannels[i] =
                    sysInfoType2Msg->neighBcchChannels[i];
                }

                if (msInfo->isSysInfoType3Decoded == TRUE ){
                    if (msInfo->downLinkControlChannelIndex ==
                           receivedChannelIndex ){
                        GsmCellInfo cellInfo;


                        msInfo->isBcchDecoded = TRUE;
                        msInfo->rrState = GSM_MS_RR_STATE_IDLE;
                        msInfo->mmState = GSM_MS_MM_STATE_MM_IDLE;
                        msInfo->mmIdleSubState =
                        GSM_MS_MM_IDLE_LOCATION_UPDATE_NEEDED;
                        msInfo->mmUpdateStatus =
                        GSM_MS_MM_UPDATE_STATUS_U2_NOT_UPDATED;

                        msInfo->bcchDecodedAt = currentTime;
                        msInfo->locationUpdateAttemptCounter = 0;

                        cellInfo.numNeighBcchChannels =
                        msInfo->numNeighBcchChannels;

                        for (i = 0; i < msInfo->numNeighBcchChannels; i++ ){
                            cellInfo.neighBcchChannelIndex[i] =
                            msInfo->downLinkNeighBcchChannels[i];
                        }

                        GsmLayer3SendInterLayerMsg( node,
                            msInfo->interfaceIndex,
                            GSM_L3_MAC_CELL_INFORMATION,
                            ( unsigned char* )&cellInfo,
                            sizeof( GsmCellInfo ) );
                    }
                    else{
                        msInfo->downLinkControlChannelIndex =
                        receivedChannelIndex;
                        msInfo->isSysInfoType3Decoded = FALSE;
                        msInfo->isBcchDecoded = FALSE;

                        return;
                    }
                }
                else{
                    msInfo->downLinkControlChannelIndex =
                    receivedChannelIndex;
                    return;
                }

                msInfo->mmState =
                GSM_MS_MM_STATE_WAIT_FOR_RR_CONNECTION_LOCATION_UPDATE;
                msInfo->locationUpdatingType = GSM_NORMAL_LOCATION_UPDATING;


                // Initiation of Immediate Assignment for location updates
                GsmLayer3SendChannelRequestMsg( node,
                    &reqMsg,
                    GSM_EST_CAUSE_LOCATION_UPDATING );

                msInfo->rrState = GSM_MS_RR_STATE_CON_PEND;
                msInfo->stats.sysInfoType2Rcvd++;
            } // End of SYSTEM_INFORMATION_TYPE2 //
            break;

        case GSM_RR_MESSAGE_TYPE_SYSTEM_INFORMATION_TYPE3:
            {
                int                 i;

                Message*            reqMsg;
                GsmSysInfoType3Msg* sysInfoType3Msg;


                if (msInfo->isBcchDecoded == TRUE &&
                       msInfo->downLinkControlChannelIndex ==
                       receivedChannelIndex ){
                    return;
                }

                if (msInfo->isSysInfoType3Decoded == TRUE &&
                       msInfo->isBcchDecoded == FALSE ){
                    return;
                }

                if (DEBUG_GSM_LAYER3_2 ){
                    printf( "MS[%d] SYS3: BCCH Decoded, CH %d\n",
                        node->nodeId,
                        receivedChannelIndex );
                }

                sysInfoType3Msg = ( GsmSysInfoType3Msg * )msgContent;

                msInfo->isSysInfoType3Decoded = TRUE;
                msInfo->sysInfoType3DecodedAt = currentTime;

                setGSMNodeLAC( node,sysInfoType3Msg->lac );
                setGSMNodeCellId( node,sysInfoType3Msg->cellIdentity );

                if (msInfo->isSysInfoType2Decoded == TRUE ){
                    if (msInfo->downLinkControlChannelIndex ==
                           receivedChannelIndex ){
                        GsmCellInfo cellInfo;

                        msInfo->isBcchDecoded = TRUE;
                        msInfo->rrState = GSM_MS_RR_STATE_IDLE;
                        msInfo->mmState = GSM_MS_MM_STATE_MM_IDLE;
                        msInfo->mmIdleSubState =
                        GSM_MS_MM_IDLE_LOCATION_UPDATE_NEEDED;
                        msInfo->mmUpdateStatus =
                        GSM_MS_MM_UPDATE_STATUS_U2_NOT_UPDATED;

                        msInfo->bcchDecodedAt = currentTime;
                        msInfo->locationUpdateAttemptCounter = 0;

                        cellInfo.numNeighBcchChannels =
                        msInfo->numNeighBcchChannels;
                        for (i = 0; i < msInfo->numNeighBcchChannels; i++ ){
                            cellInfo.neighBcchChannelIndex[i] =
                            msInfo->downLinkNeighBcchChannels[i];
                        }

                        GsmLayer3SendInterLayerMsg( node,
                            msInfo->interfaceIndex,
                            GSM_L3_MAC_CELL_INFORMATION,
                            ( unsigned char* )&cellInfo,
                            sizeof( GsmCellInfo ) );
                    }
                    else{
                        msInfo->downLinkControlChannelIndex =
                        receivedChannelIndex;
                        msInfo->isSysInfoType2Decoded = FALSE;
                        msInfo->isBcchDecoded = FALSE;
                        return;
                    }
                }
                else{
                    msInfo->downLinkControlChannelIndex =
                    receivedChannelIndex;
                    return;
                }

                msInfo->mmState =
                GSM_MS_MM_STATE_WAIT_FOR_RR_CONNECTION_LOCATION_UPDATE;
                msInfo->locationUpdatingType = GSM_NORMAL_LOCATION_UPDATING;

                // Initiation of Immediate Assignment
                GsmLayer3SendChannelRequestMsg( node,
                    &reqMsg,
                    GSM_EST_CAUSE_LOCATION_UPDATING );

                msInfo->rrState = GSM_MS_RR_STATE_CON_PEND;
                msInfo->stats.sysInfoType3Rcvd++;
            } // End of SYSTEM_INFORMATION_TYPE3 //
            break;

        case GSM_RR_MESSAGE_TYPE_IMMEDIATE_ASSIGNMENT:
            {
                GsmChannelInfo              channelInfo;
                GsmImmediateAssignmentMsg*  immAssignMsg;


                immAssignMsg = ( GsmImmediateAssignmentMsg * )msgContent;

                if (immAssignMsg->rand_disc != msInfo->rand_disc ||
                     immAssignMsg->est_cause != msInfo->est_cause ||
                       msInfo->isDedicatedChannelAssigned == TRUE ){
                    return;
                }

                // Only Early Assignment is supported
                // Set to signalling mode untill traffic starts
                msInfo->isAssignedChannelForTraffic = FALSE;

                msInfo->isRequestingChannel = FALSE;
                if (msInfo->channelRequestMsg != NULL)
                {
                    MESSAGE_Free(node, msInfo->channelRequestMsg);
                }
                msInfo->channelRequestMsg = NULL;

                if (msInfo->timerT3213Msg != NULL ){
                    MESSAGE_CancelSelfMsg( node,msInfo->timerT3213Msg );
                    msInfo->timerT3213Msg = NULL;
                }

                msInfo->assignedSlotNumber = immAssignMsg->slotNumber;

                msInfo->assignedChannelIndex = (
                    short )( ( immAssignMsg->channelDescription * 2 ) + 1 );

                msInfo->isDedicatedChannelAssigned = TRUE;
                msInfo->rrState = GSM_MS_RR_STATE_DEDICATED;

                if (DEBUG_GSM_LAYER3_3 ){
                    char    clockStr2[MAX_CLOCK_STRING_LENGTH];

                    ctoa( ( clocktype )( immAssignMsg->timingAdvance *
                                           48000.0 / 13.0 ),
                        clockStr2 );
                    printf( "MS[%d]: IMMEDIATE_ASSIGNMENT:\n"
                                "\tTS %d, UpCH %d \n",
                        node->nodeId,
                        msInfo->assignedSlotNumber,
                        msInfo->assignedChannelIndex );
                    //immAssignMsg->timingAdvance, clockStr2);
                }

                channelInfo.channelIndex = msInfo->assignedChannelIndex;
                channelInfo.slotNumber = msInfo->assignedSlotNumber;

                // Stop the T3126 Timer, if running

                if (immAssignMsg->est_cause == GSM_EST_CAUSE_NORMAL_CALL ){
                    if (DEBUG_GSM_LAYER3_1 ){
                        printf( "\tGSM_EST_CAUSE_NORMAL_CALL, "
                                    "sending CM_SERVICE_REQUEST\n" );
                    }

                    channelInfo.channelType = GSM_CHANNEL_SDCCH;
                    msInfo->assignedChannelType = GSM_CHANNEL_SDCCH;

                    msInfo->mmState =
                    GSM_MS_MM_STATE_WAIT_FOR_OUTGOING_MM_CONNECTION;
                    msInfo->ccState = GSM_MS_CC_STATE_MM_CONNECTION_PENDING;

                    msInfo->timerT3230Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmCmServiceRequestTimer_T3230,
                        DefaultGsmCmServiceRequestTimer_T3230Time,
                        NULL,
                        // Optional
                        0 );

                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MS[%d]: Setting T3230\n",node->nodeId );
                        printf( "GSM_MS[%d]: Setting T303\n",node->nodeId );
                    }

                    msInfo->timerT303Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmCallPresentTimer_T303,
                        DefaultGsmCallPresentTimer_T303Time,
                        NULL,
                        // Optional
                        0 );

                    // Start call setup: Setup an MM connection
                    GsmLayer3BuildCmServiceRequestMsg( node,&newMsg );

                    GsmLayer3SendPacket( node,
                        newMsg,
                        msInfo->assignedSlotNumber,
                        msInfo->assignedChannelIndex,
                        GSM_CHANNEL_SDCCH );

                    msInfo->stats.cmServiceRequestSent++;
                    msInfo->stats.numCallsInitiated++;
                }
                else if (immAssignMsg->est_cause ==
                            GSM_EST_CAUSE_ANSWER_TO_PAGING_TCH_CHANNEL ){
                    if (DEBUG_GSM_LAYER3_2 ){
                        printf(
                            "\tGSM_EST_CAUSE_ANSWER_TO_PAGING_TCH_CHANNEL, "
                                "sending PAGING_RESPONSE\n" );
                    }

                    channelInfo.channelType = GSM_CHANNEL_SDCCH;
                    msInfo->assignedChannelType = GSM_CHANNEL_SDCCH;

                    GsmLayer3BuildPagingResponseMsg( node,
                        &newMsg,
                        msInfo->gsmImsi );

                    GsmLayer3SendPacket( node,
                        newMsg,
                        msInfo->assignedSlotNumber,
                        msInfo->assignedChannelIndex,
                        GSM_CHANNEL_SDCCH );
                    msInfo->stats.pagingResponseSent++;
                } // End of GSM_EST_CAUSE_ANSWER_TO_PAGING_TCH_CHANNEL //
                else if (immAssignMsg->est_cause ==
                            GSM_EST_CAUSE_LOCATION_UPDATING ){
                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( "\tGSM_EST_CAUSE_LOCATION_UPDATING, "
                                    "sending LOCATION_UPDATE_REQUEST\n" );
                    }

                    channelInfo.channelType = GSM_CHANNEL_SDCCH;
                    msInfo->assignedChannelType = GSM_CHANNEL_SDCCH;

                    GsmLayer3BuildLocationUpdateRequestMsg( node,
                        &newMsg,
                        msInfo->locationUpdatingType );



                    GsmLayer3SendPacket( node,
                        newMsg,
                        msInfo->assignedSlotNumber,
                        msInfo->assignedChannelIndex,
                        GSM_CHANNEL_SDCCH );

                    // Normal Stop: LOC_UPD_ACC, LOC_UPD_REJ, Lower Layer failure
                    // Upon Expiry: Start T3211
                    msInfo->timerT3210Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmLocationUpdateRequestTimer_T3210,
                        DefaultGsmLocationUpdateRequestTimer_T3210Time,
                        NULL,
                        // Optional
                        0 );

                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MS[%d]: Setting T3210\n",node->nodeId );
                    }

                    msInfo->mmState =
                    GSM_MS_MM_STATE_LOCATION_UPDATING_INITIATED;

                    msInfo->stats.locationUpdateRequestSent++;
                }

                GsmLayer3SendInterLayerMsg( node,
                    msInfo->interfaceIndex,
                    GSM_L3_MAC_SET_CHANNEL,
                    ( unsigned char* )&channelInfo,
                    sizeof( GsmChannelInfo ) );

                msInfo->stats.immediateAssignmentRcvd++;
            }
            break; // End of receiving Immediate Assignment //

        case GSM_RR_MESSAGE_TYPE_PAGING_REQUEST_TYPE1:
            {
                // Call Terminating MS

                GsmPagingRequestType1Msg*   pageReq;


                pageReq = ( GsmPagingRequestType1Msg * )msgContent;

                if (memcmp( pageReq->imsi,
                         nwGsmData->msInfo->gsmImsi,
                         GSM_MAX_IMSI_LENGTH ) == 0 ){
                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( " PAGING_REQUEST_TYPE1 For " );
                        printGsmImsi( pageReq->imsi,GSM_MAX_IMSI_LENGTH );
                        printf( "\n\tsending CHANNEL_REQUEST\n" );
                    }

                    if (msInfo->rrState == GSM_MS_RR_STATE_IDLE ){
                        GsmLayer3SendChannelRequestMsg( node,
                            &newMsg,
                            GSM_EST_CAUSE_ANSWER_TO_PAGING_TCH_CHANNEL );

                        msInfo->rrState = GSM_MS_RR_STATE_CON_PEND;
                        msInfo->stats.pagingRequestType1Rcvd++;
                        msInfo->stats.numCallsReceived++;
                    }
                    else{
                        if (DEBUG_GSM_LAYER3_3 ){
                            sprintf( errorString,
                                "GSM_MS[%d] ERROR: MS in non-IDLE(RR) state",
                                node->nodeId );
                            ERROR_ReportWarning( errorString );
                        }
                    }
                }
            } // End of PAGING_REQUEST_TYPE1 //
            break;

        case GSM_RR_MESSAGE_TYPE_CHANNEL_RELEASE:
            {
                if (DEBUG_GSM_LAYER3_2 ){
                    printf( "CHANNEL_RELEASE\n" );
                }

                if (msInfo->isDedicatedChannelAssigned == FALSE ){
                    return;
                }

                GsmLayer3MsReleaseChannel( node,msInfo );

                if (msInfo->timerT3240Msg != NULL ){
                    MESSAGE_CancelSelfMsg( node,msInfo->timerT3240Msg );
                    msInfo->timerT3240Msg = NULL;
                }

                msInfo->rrState = GSM_MS_RR_STATE_IDLE;
                msInfo->mmState = GSM_MS_MM_STATE_MM_IDLE;
                msInfo->mmIdleSubState = GSM_MS_MM_IDLE_NORMAL_SERVICE;
                msInfo->ccState = GSM_MS_CC_STATE_NULL;
            } // End of CHANNEL_RELEASE //
            break;

        case GSM_RR_MESSAGE_TYPE_RI_HANDOVER_COMMAND:
            {
                Message*                timerMsg;

                GsmHandoverInfo         hoInfo;
                GsmRIHandoverCommandMsg riHoCmdMsg;

                if (msInfo->isDedicatedChannelAssigned == FALSE ){
                    return;
                }

                memcpy( &riHoCmdMsg,
                    msgContent,
                    sizeof( GsmRIHandoverCommandMsg ) );

                if (riHoCmdMsg.startTime < node->getNodeTime()){
                    return;
                }

                // Listen to new channel & time slot
                msInfo->assignedSlotNumber = riHoCmdMsg.slotNumber;
                msInfo->assignedChannelIndex = (
                    short )( riHoCmdMsg.channelArfcn * 2 + 1 );
                msInfo->assignedChannelType = GSM_CHANNEL_TCH;
                msInfo->hoReference = riHoCmdMsg.hoReference;
                // Bug # 3436 fix - start
                msInfo->isHandoverInProgress = TRUE;
                // Bug # 3436 fix - end
                hoInfo.channelIndex = msInfo->assignedChannelIndex;
                hoInfo.channelType = riHoCmdMsg.channelType;
                hoInfo.slotNumber = msInfo->assignedSlotNumber;
                hoInfo.bcchChannelIndex = ( short )( riHoCmdMsg.bcchArfcn *
                                                       2 );

                GsmLayer3SendInterLayerMsg( node,
                    msInfo->interfaceIndex,
                    GSM_L3_MAC_HANDOVER,
                    ( unsigned char* )&hoInfo,
                    sizeof( GsmHandoverInfo ) );

                // Sync time to new BS
                msInfo->downLinkControlChannelIndex = riHoCmdMsg.bcchArfcn;

                timerMsg = MESSAGE_Alloc( node,
                               NETWORK_LAYER,
                               0,
                               MSG_NETWORK_GsmHandoverTimer );

                timerMsg->protocolType = NETWORK_PROTOCOL_GSM;
                MESSAGE_SetInstanceId( timerMsg,
                    ( short )msInfo->interfaceIndex );
                MESSAGE_Send(node,
                    timerMsg,
                    riHoCmdMsg.startTime - node->getNodeTime());

                msInfo->stats.riHandoverCommandRcvd++;

                if (DEBUG_GSM_LAYER3_3 ){
                    printf( "GSM_MS[%d] RI_HANDOVER_COMMAND, hoRef %d, "
                                "traf Sent %d, Rcvd %d\n\t New TS %d, CH %d,"
                                " BCCH %d\n",
                        node->nodeId,
                        riHoCmdMsg.hoReference,
                        msInfo->stats.pktsSentTraffic,
                        msInfo->stats.pktsRcvdTraffic,
                        riHoCmdMsg.slotNumber,
                        riHoCmdMsg.channelArfcn * 2 + 1,
                        riHoCmdMsg.bcchArfcn * 2 );
                }
            } // end of RI_HANDOVER_COMMAND //
            break;

        default:
            {
                if (DEBUG_GSM_LAYER3_3 ){
                    sprintf( errorString,
                        "GSM_MS[%d]: Unknown RR msg (%d, %d)",
                        node->nodeId,
                        receivedSlotNumber,
                        receivedChannelIndex );
                    ERROR_ReportWarning( errorString );
                }

                msInfo->stats.msgsRcvdUnknownControl++;
                return;
            }
            break;
    } // End of switch

    msInfo->stats.msgsRcvdControl++;
} // End of gsmMsprocessRrMessageFromMac //


// MM - Mobility Management sub-layer functions

static
void gsmMsProcessMmFailureLocationUpdate(
    Node* node,
    GsmLayer3MsInfo* msInfo)
{
    msInfo->locationUpdateAttemptCounter++;

    // Release RR connection
    GsmLayer3MsReleaseChannel( node,msInfo );

    msInfo->rrState = GSM_MS_RR_STATE_IDLE;
    msInfo->mmState = GSM_MS_MM_STATE_MM_IDLE;
    msInfo->mmIdleSubState = GSM_MS_MM_IDLE_NORMAL_SERVICE;


    if (msInfo->mmUpdateStatus == GSM_MS_MM_UPDATE_STATUS_U1_UPDATED &&
        // stored LAI matches one received serving cell BCCH &&
        msInfo->locationUpdateAttemptCounter < 4 ){
        msInfo->mmState = GSM_MS_MM_STATE_MM_IDLE;
        msInfo->mmIdleSubState = GSM_MS_MM_IDLE_NORMAL_SERVICE;

        if (DEBUG_GSM_LAYER3_TIMERS ){
            printf( "GSM_MS[%d]: MM Failure, Setting T3211\n",node->nodeId );
        }
        // Start T3211, upon expiry start location update
        msInfo->timerT3211Msg = GsmLayer3StartTimer(
            node,
            MSG_NETWORK_GsmLocationUpdateFailureTimer_T3211,
            DefaultGsmLocationUpdateFailureTimer_T3211Time,
            NULL,
            // Optional
            0 );
    }
    else if (msInfo->mmUpdateStatus != GSM_MS_MM_UPDATE_STATUS_U1_UPDATED ||
        // stored LAI is diff from one received on BCCH of serving cell ||
        msInfo->locationUpdateAttemptCounter >= 4 ){
        // Delete LAI... info
        msInfo->mmUpdateStatus = GSM_MS_MM_UPDATE_STATUS_U2_NOT_UPDATED;
        msInfo->mmIdleSubState = GSM_MS_MM_IDLE_ATTEMPTING_TO_UPDATE;

        if (msInfo->locationUpdateAttemptCounter < 4 ){
            if (DEBUG_GSM_LAYER3_TIMERS ){
                printf( "GSM_MS[%d]: MM Failure, Setting T3211\n",
                    node->nodeId );
            }
            // Start T3211, upon expiry start location update
            msInfo->timerT3211Msg = GsmLayer3StartTimer(
                node,
                MSG_NETWORK_GsmLocationUpdateFailureTimer_T3211,
                DefaultGsmLocationUpdateFailureTimer_T3211Time,
                NULL,
                // Optional
                0 );
        }
        else{
            if (DEBUG_GSM_LAYER3_TIMERS ){
                printf( "GSM_MS[%d]: MM Failure, Setting T3212\n",
                    node->nodeId );
            }

            msInfo->stats.locationUpdateFailures++;

            if (msInfo->timerT3212Msg != NULL ){
                MESSAGE_CancelSelfMsg( node, msInfo->timerT3212Msg );
                msInfo->timerT3212Msg = NULL;
            }

            msInfo->timerT3212Msg = GsmLayer3StartTimer(
                node,
                MSG_NETWORK_GsmPeriodicLocationUpdateTimer_T3212,
                DefaultGsmPeriodicLocationUpdateTimer_T3212Time,
                NULL,
                // Optional
                0 );
        }
    }
} // End of gsmMsProcessMmFailure //

static
void gsmMsProcessMmMessagesFromMac(
    Node* node,
    unsigned char* msgContent,
    uddtSlotNumber   receivedSlotNumber,
    uddtChannelIndex   receivedChannelIndex)
{
    char                    errorString[MAX_STRING_LENGTH];

    GsmLayer3Data*          nwGsmData;
    GsmLayer3MsInfo*        msInfo;

    Message*                newMsg;
    GsmGenericLayer3MsgHdr* genL3MsgHdr;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    msInfo = nwGsmData->msInfo;

    genL3MsgHdr = ( GsmGenericLayer3MsgHdr * )msgContent;

    if (DEBUG_GSM_LAYER3_2 ){
        printf( "GSM_MS[%d] TS %d CH %d Rcvd a MM Msg: ",
            node->nodeId,
            receivedSlotNumber,
            receivedChannelIndex );
    }

    switch ( genL3MsgHdr->messageType ){
        case GSM_MM_MESSAGE_TYPE_CM_SERVICE_ACCEPT:
            {
                GsmCmServiceAcceptMsg*  acceptMsg;

                if (DEBUG_GSM_LAYER3_2 ){
                    printf( "CM_SERVICE_ACCEPT, sending SETUP\n" );
                }

                acceptMsg = ( GsmCmServiceAcceptMsg * )msgContent;

                // MM Connection established, send Setup (Basic call)
                msInfo->mmState = GSM_MS_MM_STATE_MM_CONNECTION_ACTIVE;
                msInfo->ccState = GSM_MS_CC_STATE_CALL_INITIATED;

                if (DEBUG_GSM_LAYER3_TIMERS ){
                    printf( "GSM_MS[%d]: Cancelling T3230\n",node->nodeId );
                }

                if (msInfo->timerT3230Msg != NULL ){
                    MESSAGE_CancelSelfMsg( node,msInfo->timerT3230Msg );
                    msInfo->timerT3230Msg = NULL;
                }

                GsmLayer3BuildMoSetupMsg( node,&newMsg );

                GsmLayer3SendPacket( node,
                    newMsg,
                    nwGsmData->msInfo->assignedSlotNumber,
                    nwGsmData->msInfo->assignedChannelIndex,
                    GSM_CHANNEL_SDCCH );

                msInfo->stats.cmServiceAcceptRcvd++;
                msInfo->stats.setupSent++;
            } // End of GSM_MM_MESSAGE_TYPE_CM_SERVICE_ACCEPT //
            break;

        case GSM_MM_MESSAGE_TYPE_CM_SERVICE_REJECT:
            {
                if (DEBUG_GSM_LAYER3_TIMERS ){
                    printf( "GSM_MS[%d]: Cancelling T3230\n",node->nodeId );
                }

                if (msInfo->timerT3230Msg != NULL ){
                    MESSAGE_CancelSelfMsg( node,msInfo->timerT3230Msg );
                    msInfo->timerT3230Msg = NULL;
                }
            }
            break;

        case GSM_MM_MESSAGE_TYPE_LOCATION_UPDATE_ACCEPT:
            {
                if (DEBUG_GSM_LAYER3_2 ){
                    printf( "LOCATION_UPDATE_ACCEPT\n" );
                }

                msInfo->stats.locationUpdateAcceptRcvd++;
                msInfo->mmState = GSM_MS_MM_STATE_WAIT_FOR_NETWORK_COMMAND;
                msInfo->mmUpdateStatus = GSM_MS_MM_UPDATE_STATUS_U1_UPDATED;

                msInfo->locationUpdateAttemptCounter = 0;

                if (DEBUG_GSM_LAYER3_TIMERS ){
                    printf( "GSM_MS[%d]: Cancelling T3210\n",node->nodeId );
                }

                if (msInfo->timerT3210Msg != NULL ){
                    MESSAGE_CancelSelfMsg( node,msInfo->timerT3210Msg );
                    msInfo->timerT3210Msg = NULL;
                }


                msInfo->timerT3240Msg = GsmLayer3StartTimer( node,
                                            MSG_NETWORK_GsmTimer_T3240,
                                            DefaultGsmTimer_T3240Time,
                                            NULL,
                                            // Optional
                                            0 );
            } // End of LOCATION_UPDATE_ACCEPT //
            break;

        case GSM_MM_MESSAGE_TYPE_LOCATION_UPDATE_REJECT:
            {
                if (DEBUG_GSM_LAYER3_3 ){
                    printf( "GSM_MS[%d]: LOCATION_UPDATE_REJECT\n",
                        node->nodeId );
                }
            }
            break;

        default:
            {
                if (DEBUG_GSM_LAYER3_3 ){
                    sprintf( errorString,
                        "GSM_MS[%d]: Unknown MM msg (%d, %d)",
                        node->nodeId,
                        receivedSlotNumber,
                        receivedChannelIndex );
                    ERROR_ReportWarning( errorString );
                }

                msInfo->stats.msgsRcvdUnknownControl++;
                return;
            }
            break;
    } // End of switch //

    msInfo->stats.msgsRcvdMmLayer++;
    msInfo->stats.msgsRcvdControl++;
} // End of gsmMsProcessMmMessagesFromMac //


// CM - Call Management sub-layer functions
//      Call Control, supplementary services, SMS
static
void gsmMsProcessCcMessagesFromMac(
    Node* node,
    unsigned char* msgContent,
    uddtSlotNumber   receivedSlotNumber,
    uddtChannelIndex   receivedChannelIndex)
{
    char                    clockStr[MAX_STRING_LENGTH];
    char                    errorString[MAX_STRING_LENGTH];

    GsmLayer3Data*          nwGsmData;
    GsmLayer3MsInfo*        msInfo;
    Message*                newMsg;
    GsmGenericLayer3MsgHdr* genL3MsgHdr;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    msInfo = nwGsmData->msInfo;

    genL3MsgHdr = ( GsmGenericLayer3MsgHdr * )msgContent;

    if (DEBUG_GSM_LAYER3_2 ){
        printf( "GSM_MS[%d] Received a CC Msg: ",node->nodeId );
    }

    switch ( genL3MsgHdr->messageType ){
        case GSM_CC_MESSAGE_TYPE_ALERTING:
            {
                // Call Originating MS
                if (DEBUG_GSM_LAYER3_2 ){
                    printf( " ALERTING: MO\n" );
                }

                if (!( msInfo->ccState == GSM_MS_CC_STATE_CALL_INITIATED ||
                          msInfo->ccState ==
                          GSM_MS_CC_STATE_MOBILE_ORIG_CALL_PROCEEDING ) ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        sprintf( errorString,
                            "GSM_MS[%d] ERROR: Incorrect state",
                            node->nodeId );
                        ERROR_ReportWarning( errorString );
                    }
                    return;
                }

                if (msInfo->timerT303Msg != NULL ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MS[%d]: Cancelling T303\n",
                            node->nodeId );
                    }

                    if (msInfo->timerT303Msg != NULL ){
                        MESSAGE_CancelSelfMsg( node,msInfo->timerT303Msg );
                        msInfo->timerT303Msg = NULL;
                    }
                }

                if (msInfo->timerT310Msg != NULL ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MS[%d]: Cancelling T310\n",
                            node->nodeId );
                    }

                    if (msInfo->timerT310Msg != NULL ){
                        MESSAGE_CancelSelfMsg( node,msInfo->timerT310Msg );
                        msInfo->timerT310Msg = NULL;
                    }
                }

                msInfo->ccState = GSM_MS_CC_STATE_CALL_DELIVERED;

                // Generate tone in handset, update MS state
                msInfo->stats.alertingRcvd++;
            } // End of ALERTING //
            break;

        case GSM_CC_MESSAGE_TYPE_CALL_PROCEEDING:
            {
                GsmCallProceedingMsg*   callProceedingMsg;


                // Call Originating MS
                if (msInfo->ccState != GSM_MS_CC_STATE_CALL_INITIATED ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        sprintf( errorString,
                            "GSM_MS[%d] ERROR: Incorrect state",
                            node->nodeId );
                        ERROR_ReportWarning( errorString );
                    }
                    return;
                }

                callProceedingMsg = ( GsmCallProceedingMsg * )msgContent;
                msInfo->ccState =
                GSM_MS_CC_STATE_MOBILE_ORIG_CALL_PROCEEDING;

                if (DEBUG_GSM_LAYER3_2 ){
                    printf( " CALL_PROCEEDING, ti = %d\n",
                        callProceedingMsg->transactionIdentifier );
                }

                if (DEBUG_GSM_LAYER3_TIMERS ){
                    printf( "GSM_MS[%d]: Setting T310\n",node->nodeId );
                }
                msInfo->timerT310Msg = GsmLayer3StartTimer(
                    node,
                    MSG_NETWORK_GsmCallProceedingTimer_T310,
                    DefaultGsmCallProceedingTimer_T310Time,
                    NULL,
                    // Optional
                    0 );

                if (DEBUG_GSM_LAYER3_TIMERS ){
                    printf( "GSM_MS[%d]: Cancelling T303\n",node->nodeId );
                }

                if (msInfo->timerT303Msg != NULL ){
                    MESSAGE_CancelSelfMsg( node,msInfo->timerT303Msg );
                    msInfo->timerT303Msg = NULL;
                }

                msInfo->stats.callProceedingRcvd++;
            } // End of CALL_PROCEEDING //
            break;

        case GSM_CC_MESSAGE_TYPE_CONNECT:
            {
                GsmChannelInfo  channelInfo;


                if (!( msInfo->ccState == GSM_MS_CC_STATE_CALL_DELIVERED ||
                          msInfo->ccState ==
                          GSM_MS_CC_STATE_MOBILE_ORIG_CALL_PROCEEDING ) ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        sprintf( errorString,
                            "GSM_MS[%d] ERROR: Incorrect state",
                            node->nodeId );
                        ERROR_ReportWarning( errorString );
                    }
                    return;
                }

                msInfo->ccState = GSM_MS_CC_STATE_ACTIVE;
                msInfo->isAssignedChannelForTraffic = TRUE;

                if (msInfo->timerT303Msg != NULL ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MS[%d]: Cancelling T303\n",
                            node->nodeId );
                    }

                    if (msInfo->timerT303Msg != NULL ){
                        MESSAGE_CancelSelfMsg( node,msInfo->timerT303Msg );
                        msInfo->timerT303Msg = NULL;
                    }
                }

                if (msInfo->timerT310Msg != NULL ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MS[%d]: Cancelling T310\n",
                            node->nodeId );
                    }

                    if (msInfo->timerT303Msg != NULL ){
                        MESSAGE_CancelSelfMsg( node,msInfo->timerT310Msg );
                        msInfo->timerT310Msg = NULL;
                    }
                }

                GsmLayer3BuildMoConnectAcknowledgeMsg( node,&newMsg );

                GsmLayer3SendPacket( node,
                    newMsg,
                    msInfo->assignedSlotNumber,
                    msInfo->assignedChannelIndex,
                    GSM_CHANNEL_SDCCH );

                msInfo->assignedChannelType = GSM_CHANNEL_TCH;
                channelInfo.channelType = GSM_CHANNEL_TCH;
                channelInfo.slotNumber = msInfo->assignedSlotNumber;
                channelInfo.channelIndex = msInfo->assignedChannelIndex;

                if (DEBUG_GSM_LAYER3_2 ){
                    ctoa(node->getNodeTime(), clockStr);
                    printf( " CONNECT, sent CONNECT_ACK to Network\n"
                                "### Start of call ###: Orig MS: %s\n",
                        clockStr );
                }

                GsmLayer3SendInterLayerMsg( node,
                    msInfo->interfaceIndex,
                    GSM_L3_MAC_SET_CHANNEL,
                    ( unsigned char* )&channelInfo,
                    sizeof( GsmChannelInfo ) );

                // Start sending voice traffic
                startGsmVirtualTrafficPacketTimer(node,
                    msInfo,
                    node->getNodeTime() - msInfo->callStartTime + 1 * NANO_SECOND );

                msInfo->stats.connectRcvd++;
                msInfo->stats.connectAckSent++;
                msInfo->stats.numCallsConnected++;
            }
            break; // End of CONNECT //

        case GSM_CC_MESSAGE_TYPE_CONNECT_ACKNOWLEDGE:
            {
                GsmChannelInfo  channelInfo;


                ctoa(node->getNodeTime(), clockStr);


                if (DEBUG_GSM_LAYER3_2 ){
                    printf( " CONNECT_ACKNOWLEDGE\n"
                                "### Start of call ###: Term MS: %s\n\n",
                        clockStr );
                }

                msInfo->ccState = GSM_MS_CC_STATE_ACTIVE;
                msInfo->isAssignedChannelForTraffic = TRUE;

                if (msInfo->timerT313Msg != NULL ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MS[%d]: Cancelling T313\n",
                            node->nodeId );
                    }

                    MESSAGE_CancelSelfMsg( node,msInfo->timerT313Msg );
                    msInfo->timerT313Msg = NULL;
                }

                msInfo->assignedChannelType = GSM_CHANNEL_TCH;
                channelInfo.channelType = GSM_CHANNEL_TCH;
                channelInfo.slotNumber = msInfo->assignedSlotNumber;
                channelInfo.channelIndex = msInfo->assignedChannelIndex;

                GsmLayer3SendInterLayerMsg( node,
                    msInfo->interfaceIndex,
                    GSM_L3_MAC_SET_CHANNEL,
                    ( unsigned char* )&channelInfo,
                    sizeof( GsmChannelInfo ) );

                msInfo->stats.connectAckRcvd++;
                msInfo->stats.numCallsConnected++;
            } // End of CONNECT_ACK //
            break;

        case GSM_CC_MESSAGE_TYPE_DISCONNECT:
            {
                unsigned char   cause[2];

                if (DEBUG_GSM_LAYER3_2 ){
                    printf( " DISCONNECT, \n" "\tsending RELEASE\n" );
                }

                if (msInfo->ccState == GSM_MS_CC_STATE_NULL ||
                       msInfo->ccState ==
                       GSM_MS_CC_STATE_DISCONNECT_INDICATION ||
                       msInfo->ccState == GSM_MS_CC_STATE_RELEASE_REQUEST ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        sprintf( errorString,
                            "GSM_MS[%d] ERROR: Incorrect state",
                            node->nodeId );
                        ERROR_ReportWarning( errorString );
                    }
                    fflush( stdout );
                    return;
                }

                // release the transactionIdentifier

                GsmLayer3MsStopCallControlTimers( node,msInfo );

                msInfo->ccState = GSM_MS_CC_STATE_DISCONNECT_INDICATION;
                cause[0] = GSM_LOCATION_USER;
                cause[1] = GSM_CAUSE_VALUE_NORMAL_CALL_CLEARING;

                GsmLayer3BuildReleaseByMsMsg( node,&newMsg,cause );

                GsmLayer3SendPacket( node,
                    newMsg,
                    msInfo->assignedSlotNumber,
                    msInfo->assignedChannelIndex,
                    GSM_CHANNEL_TCH );

                if (msInfo->timerT305Msg != NULL ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MS[%d]: Cancelling T305\n",
                            node->nodeId );
                    }
                    MESSAGE_CancelSelfMsg( node,msInfo->timerT305Msg );
                    msInfo->timerT305Msg = NULL;
                }

                if (DEBUG_GSM_LAYER3_TIMERS ){
                    printf( "GSM_MS[%d]: Setting T308\n",node->nodeId );
                }
                msInfo->timerT308Expirations = 0;
                msInfo->timerT308Msg = GsmLayer3StartTimer( node,
                                           MSG_NETWORK_GsmReleaseTimer_T308,
                                           DefaultGsmReleaseTimer_T308Time,
                                           NULL,
                                           0 );

                msInfo->ccState = GSM_MS_CC_STATE_RELEASE_REQUEST;
                msInfo->stats.disconnectRcvd++;
                msInfo->stats.releaseSent++;
                msInfo->stats.numCallsCompleted++;
            } // End of DISCONNECT //
            break;

        case GSM_CC_MESSAGE_TYPE_RELEASE:
            {
                if (DEBUG_GSM_LAYER3_2 ){
                    printf( " RELEASE, \n" "\tsending RELEASE_COMPLETE\n" );
                }

                // release the transactionIdentifier

                if (msInfo->ccState == GSM_MS_CC_STATE_NULL ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        sprintf( errorString,
                            "GSM_MS[%d] ERROR: Incorrect state",
                            node->nodeId );
                        ERROR_ReportWarning( errorString );
                    }
                    return;
                }

                msInfo->ccState = GSM_MS_CC_STATE_RELEASE_REQUEST;

                if (msInfo->timerT305Msg != NULL ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MS[%d]: Cancelling T305\n",
                            node->nodeId );
                    }
                    MESSAGE_CancelSelfMsg( node,msInfo->timerT305Msg );
                    msInfo->timerT305Msg = NULL;
                }

                if (msInfo->timerT308Msg != NULL ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MS[%d]: Cancelling T308\n",
                            node->nodeId );
                    }
                    MESSAGE_CancelSelfMsg( node,msInfo->timerT308Msg );
                    msInfo->timerT308Msg = NULL;
                }

                GsmLayer3BuildReleaseCompleteByMsMsg( node,&newMsg );

                GsmLayer3SendPacket( node,
                    newMsg,
                    msInfo->assignedSlotNumber,
                    msInfo->assignedChannelIndex,
                    GSM_CHANNEL_TCH );

                msInfo->ccState = GSM_MS_CC_STATE_NULL;

                msInfo->stats.releaseRcvd++;
                msInfo->stats.releaseCompleteSent++;
                msInfo->stats.numCallsCompleted++;
            } // End of RELEASE //
            break;

        case GSM_CC_MESSAGE_TYPE_RELEASE_COMPLETE:
            {
                if (DEBUG_GSM_LAYER3_2 ){
                    printf( " RELEASE_COMPLETE, MT\n" );
                }

                if (msInfo->timerT303Msg != NULL ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MS[%d]: Cancelling T303\n",
                            node->nodeId );
                    }
                    MESSAGE_CancelSelfMsg( node,msInfo->timerT303Msg );
                    msInfo->timerT303Msg = NULL;
                }

                if (msInfo->timerT308Msg != NULL ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MS[%d]: Cancelling T308\n",
                            node->nodeId );
                    }
                    MESSAGE_CancelSelfMsg( node,msInfo->timerT308Msg );
                    msInfo->timerT308Msg = NULL;
                }

                msInfo->ccState = GSM_MS_CC_STATE_NULL;
                msInfo->stats.releaseCompleteRcvd++;
            } // End of RELEASE_COMPLETE //
            break;

        case GSM_CC_MESSAGE_TYPE_SETUP:
            {
                unsigned char   calledBcdNumber[GSM_MAX_MSISDN_LENGTH];

                Message*        alertingMsg;
                Message*        connectMsg;
                GsmMtSetupMsg*  setupMsg;


                // Call Terminating MS
                setupMsg = ( GsmMtSetupMsg * )msgContent;

                if (DEBUG_GSM_LAYER3_2 ){
                    printf( " SETUP: MT, ti = %d, "
                                "\n\tsending CALL_CONFIRMED msg\n",
                        setupMsg->transactionIdentifier );
                }

                if (msInfo->ccState != GSM_MS_CC_STATE_NULL ){
                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        sprintf( errorString,
                            "GSM_MS[%d] ERROR: Incorrect state",
                            node->nodeId );
                        ERROR_ReportWarning( errorString );
                    }
                    return;
                }

                GsmLayer3ConvertFromBcdNumber( calledBcdNumber,
                    setupMsg->calledPartyBcdNumber,
                    GSM_MAX_MSISDN_LENGTH );

                // If for this MS
                if (memcmp( msInfo->gsmMsIsdn,
                         calledBcdNumber,
                         GSM_MAX_MSISDN_LENGTH ) != 0 ){
                    if (DEBUG_GSM_LAYER3_3 ){
                        printf( "GSM_MS[%d]: Not for this MS\n",
                            node->nodeId );
                    }

                    return;
                }

                // Perform compatibility checking
                msInfo->ccState = GSM_MS_CC_STATE_CALL_PRESENT;
                msInfo->mmState = GSM_MS_MM_STATE_MM_CONNECTION_ACTIVE;

                GsmLayer3BuildCallConfirmedMsg( node,&newMsg );

                GsmLayer3SendPacket( node,
                    newMsg,
                    msInfo->assignedSlotNumber,
                    msInfo->assignedChannelIndex,
                    GSM_CHANNEL_SDCCH );

                msInfo->ccState = GSM_MS_CC_STATE_MOBILE_TERM_CALL_CONFIRMED;

                // If the Setup Msg has a 'signal' element then the MS
                // will generate an alerting tone (non-early assignment).

                // Otherwise(late assignment), the MS shall wait untill
                // a traffic channel is assigned and then will send the
                // alerting message.
                // See GSM 04.08, Section 5.2.2.3.2, Section 7.3.3

                if (DEBUG_GSM_LAYER3_2 ){
                    printf( "\tSending MT ALERTING\n" );
                }

                GsmLayer3BuildMtAlertingMsg( node,&alertingMsg );

                GsmLayer3SendPacket( node,
                    alertingMsg,
                    nwGsmData->msInfo->assignedSlotNumber,
                    nwGsmData->msInfo->assignedChannelIndex,
                    GSM_CHANNEL_SDCCH );

                msInfo->ccState = GSM_MS_CC_STATE_CALL_RECEIVED;

                if (DEBUG_GSM_LAYER3_2 ){
                    printf( "\tSending MT CONNECT\n" );
                }

                GsmLayer3BuildMtConnectMsg( node,&connectMsg );

                GsmLayer3SendPacket( node,
                    connectMsg,
                    msInfo->assignedSlotNumber,
                    msInfo->assignedChannelIndex,
                    GSM_CHANNEL_SDCCH );

                msInfo->ccState = GSM_MS_CC_STATE_CONNECT_REQUEST;

                if (DEBUG_GSM_LAYER3_TIMERS ){
                    printf( "GSM_MS[%d]: Setting T313\n",node->nodeId );
                }
                msInfo->timerT313Msg = GsmLayer3StartTimer( node,
                                           MSG_NETWORK_GsmConnectTimer_T313,
                                           DefaultGsmConnectTimer_T313Time,
                                           NULL,
                                           // Optional
                                           0 );

                msInfo->stats.setupRcvd++;
                msInfo->stats.alertingSent++;
                msInfo->stats.connectSent++;
            } // End of SETUP //
            break;

        default:
            {
                if (DEBUG_GSM_LAYER3_3 ){
                    printf( "GSMLayer3 MS[%d]: Unknown MM msg (%d, %d)",
                        node->nodeId,
                        receivedSlotNumber,
                        receivedChannelIndex );
                }

                msInfo->stats.msgsRcvdUnknownControl++;
                return;
            }
    } // End of switch //

    msInfo->stats.msgsRcvdControl++;
    msInfo->stats.msgsRcvdCcLayer++;
} // End of gsmMsProcessCcMessagesFromMac //

static
void gsmBsProcessCcMessageFromMac(
    Node* node,
    unsigned char* msgContent,
    GsmChannelDescription* channelDesc,
    uddtSlotNumber          receivedSlotNumber,
    uddtChannelIndex        receivedChannelIndex)
{
    // Since msg is received on the air/radio interface
    // by the BS it is a DTAP msg, forward it to MSC.

    char                    errorString[MAX_STRING_LENGTH];
    int                     currentConnId;
    int                     dtapMsgSize = -1;

    GsmLayer3Data*          nwGsmData;
    GsmLayer3BsInfo*        bsInfo;

    GsmGenericLayer3MsgHdr* genL3MsgHdr;
    GsmSccpRoutingLabel     routingLabel;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    bsInfo = nwGsmData->bsInfo;

    genL3MsgHdr = ( GsmGenericLayer3MsgHdr * )msgContent;

    currentConnId =
    channelDesc->slotUsageInfo[receivedSlotNumber].sccpConnectionId;

    memset( &routingLabel,0,sizeof( GsmSccpRoutingLabel ) );
    switch ( genL3MsgHdr->messageType ){
        case GSM_CC_MESSAGE_TYPE_SETUP:
            {
                // printf("GSM_BS[%d]<-MS: SETUP\n", node->nodeId);
                dtapMsgSize = sizeof( GsmMoSetupMsg );
            } // End of MO Setup message //
            break;

        case GSM_CC_MESSAGE_TYPE_CALL_CONFIRMED:
            {
                // printf("GSM_BS[%d]<-MS: CALL_CONFIRMED\n", node->nodeId);
                dtapMsgSize = sizeof( GsmCallConfirmedMsg );
            }
            break;

        case  GSM_CC_MESSAGE_TYPE_ALERTING:
            {
                // From Call Terminating MS
                // printf("GSM_BS[%d]<-MS: ALERTING\n", node->nodeId);
                dtapMsgSize = sizeof( GsmMtAlertingMsg );
            }
            break;

        case GSM_CC_MESSAGE_TYPE_CONNECT:
            {
                // From Call Terminating MS
                // printf("GSM_BS[%d]<-MS: CONNECT\n", node->nodeId);
                dtapMsgSize = sizeof( GsmMtConnectMsg );
            }
            break;

        case GSM_CC_MESSAGE_TYPE_CONNECT_ACKNOWLEDGE:
            {
                dtapMsgSize = sizeof( GsmConnectAcknowledgeMsg );

                // printf("GSM_BS[%d]<-MS: CONNECT_ACKNOWLEDGE\n", node->nodeId);
                // Set the channel in traffic mode at the BS
                channelDesc->slotUsageInfo[receivedSlotNumber].\
                isUsedForTraffic = TRUE;
            }
            break;

            // 1 byte space is added as the steal flag for FACCH msgs
        case GSM_CC_MESSAGE_TYPE_DISCONNECT:
            {
                // printf("GSM_BS[%d]<-MS: DISCONNECT\n", node->nodeId);
                dtapMsgSize = sizeof( GsmDisconnectByMsMsg ) + 1;
            }
            break;

        case GSM_CC_MESSAGE_TYPE_RELEASE_COMPLETE:
            {
                // ditto as for DISCONNECT
                // printf("GSM_BS[%d]<-MS RELEASE_COMPLETE\n", node->nodeId);
                dtapMsgSize = sizeof( GsmReleaseCompleteByMsMsg ) + 1;
            }
            break;

        case GSM_CC_MESSAGE_TYPE_RELEASE:
            {
                // printf("GSM_BS[%d]<-MS RELEASE\n", node->nodeId);
                dtapMsgSize = sizeof( GsmReleaseByMsMsg ) + 1;
            }
            break;

        default:
            {
                if (DEBUG_GSM_LAYER3_3 ){
                    sprintf( errorString,
                        "GSM_Layer3 BS[%d]: Unknown CC msg TS %d, CH %d",
                        node->nodeId,
                        receivedSlotNumber,
                        receivedChannelIndex );
                    ERROR_ReportWarning( errorString );
                }

                return;
            }
    } // End of switch

    GsmLayer3SendPacketViaGsmAInterface( node,
        bsInfo->mscNodeAddress,
        node->nodeId,
        currentConnId,
        routingLabel,
        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
        ( unsigned char* )msgContent,
        dtapMsgSize );
} // End of gsmBsProcessCcMessageFromMac //


static
void GsmLayer3InitStats(
    Node* node,
    const NodeInput* nodeInput)
{
    BOOL            retVal;
    char            buf[MAX_STRING_LENGTH];
    GsmLayer3Data*  nwGsmData;

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    IO_ReadString( node->nodeId,
        ANY_DEST,
        nodeInput,
        "GSM-STATISTICS",
        &retVal,
        buf );

    if (( retVal == FALSE ) || ( strcmp( buf,"NO" ) == 0 ) ){
        nwGsmData->collectStatistics = FALSE;
    }
    else if (strcmp( buf,"YES" ) == 0 ){
        nwGsmData->collectStatistics = TRUE;
    }
} // End of GsmLayer3InitStats //


// Intra-MSC External Handover decision algorithm
// based on GSM 05.08 Annex A
static
BOOL isHandoverNeeded(
    Node* node,
    GsmLayer3BsInfo* bsInfo,
    GsmSlotUsageInfo* slotInfo,
    unsigned char* hoCause)
{
    unsigned char   lRxLevUlH           = GsmMapRxLev( GSM_L_RXLEV_UL_H );
    unsigned char   lRxLevDlH           = GsmMapRxLev( GSM_L_RXLEV_DL_H );
    unsigned char   lRxQualUlH          = GsmMapRxQual( GSM_L_RXQUAL_UL_H );
    unsigned char   lRxQualDlH          = GsmMapRxQual( GSM_L_RXQUAL_DL_H );

    int             i;
    int             numRxLevUlBelow     = 0;
    int             numRxLevDlBelow     = 0;
    int             numRxQualUlAbove    = 0;
    int             numRxQualDlAbove    = 0;
    int             N5                  = GSM_HREQAVE;
    int             P5                  = N5 - 1;

    // Algorithm: (XX is DL or UL)
    // For RxLevXX:
    // If atleast P5 averaged values out of N5 are lower than L_RXLEV_XX_H
    // a handover might be required.
    //
    // For RxQualXX:
    // If atleast P6 averaged values out of N6 are greater (worse quality)
    // than L_RXQUAL_XX_H a handover might be required.

    for (i = 0; i < N5; i++ ){
        if (slotInfo->avgRxLevUl[i] / GSM_HREQAVE < lRxLevUlH ){
            numRxLevUlBelow++;
        }

        if (slotInfo->avgRxLevDl[i] / GSM_HREQAVE < lRxLevDlH ){
            numRxLevDlBelow++;
        }

        if (slotInfo->avgRxQualUl[i] / GSM_HREQAVE > lRxQualUlH ){
            numRxQualUlAbove++;
        }

        if (slotInfo->avgRxQualDl[i] / GSM_HREQAVE > lRxQualDlH ){
            numRxQualDlAbove++;
        }
    }

    // Check if HO is needed
    if (numRxLevDlBelow >= P5 ){
        *hoCause = GSM_CAUSE_DOWNLINK_STRENGTH;
        return TRUE;
    }
    else if (numRxLevUlBelow >= P5 ){
        *hoCause = GSM_CAUSE_UPLINK_STRENGTH;
        /**
        printf("[%d] TS %d, DL.CH %d: GSM_CAUSE_UPLINK_STRENGTH\n",
            node->nodeId, (*slotInfo).slotNumber,
            (*slotInfo).channelPairIndex * 2);
        **/
        return TRUE;
    }
    else if (numRxQualUlAbove >= P5 ){
        *hoCause = GSM_CAUSE_UPLINK_QUALITY;
        return TRUE;
    }
    else if (numRxQualDlAbove >= P5 ){
        *hoCause = GSM_CAUSE_DOWNLINK_QUALITY;
        return TRUE;
    }
    return FALSE;
} // End of isHandoverNeeded //


// External (inter-BTS) intra-MSC handover
static
void startHandover(
    Node* node,
    GsmLayer3BsInfo* bsInfo,
    GsmSlotUsageInfo* slotInfo,
    unsigned char       hoCause)
{
    int                     n                   = 0;
    int                     lastAvgValueIndex   = 0;
    int                     numSuitableCells    = 0;
    int                     suitableCells[GSM_NUM_NEIGHBOUR_CELLS];

    GsmHandoverRequiredMsg* hoReqdMsg;
    GsmSccpRoutingLabel     routingLabel;


    memset( &routingLabel,0,sizeof( GsmSccpRoutingLabel ) );
    // Identify neighbouring cells suitable for handover using Annex A
    // of GSM 05.08.
    // BSS decision algorithm

    for (n = 0; n < bsInfo->numNeighBcchChannels; n++ ){
        lastAvgValueIndex = slotInfo->numAvgBcchValues[n];

        // 1.) RXLEV_NCELL(n) > RXLEV_MIN(n) + Max (O, Pa)
        //      where: Pa = (MS_TXPWR_MAX(n)-P)
        if (( slotInfo->avgRxLevBcchDl[n][lastAvgValueIndex] /
                 GSM_HREQAVE ) >
               ( GSM_RXLEV_MIN_DEF + MAX( 0,
                                         // Bug # 3436 fix - start
                                         ( PHY_GSM_MS_TXPWR_MAX_N_dBm ) -
                                         ( PHY_GSM_P_dBm ) ) ) ){
                                         // Bug # 3436 fix - end
            float   line1   = 0.0;
            float   line2   = 0.0;
            float   line3   = 0.0;

            if (DEBUG_GSM_LAYER3_2 ){
                printf( "GSM_BS[%d] HO.1 Ok: N.Cell[%d], CH %d, TS %d\n",
                    node->nodeId,
                    n,
                    bsInfo->neighBcchChannels[n],
                    slotInfo->slotNumber );
            }

            // 2.) (Min(MS_TXPWR_MAX,P) - RXLEV_DL - PWR_C_D) - // line 1
            //      (Min(MS_TXPWR_MAX(n),P) - RXLEV_NCELL(n)) - // line 2
            //          HO_MARGIN(n) > 0                        // line 3

            // Can retrieve some of these variables from the config file
            // i.e. can be BS specific.

            // Basically the following lines check if the received BCCH
            // signal level should exceed the current serving cell TCH
            // signal level by a certain margin.
             // Bug # 3436 fix - start
            line1 = (
                float )( ( MIN( PHY_GSM_MS_TXPWR_MAX_dBm,
                                     PHY_GSM_P_dBm ) ) -
                           slotInfo->avgRxLevDl[slotInfo->numAvgValues] /
                           GSM_HREQAVE - ( -0.0 ) );
             // Bug # 3436 fix - end
             // Bug # 3436 fix - start
            line2 = (
                float )( ( MIN( PHY_GSM_MS_TXPWR_MAX_N_dBm,
                                     PHY_GSM_P_dBm ) ) -
                           slotInfo->avgRxLevBcchDl[n][lastAvgValueIndex] /
                           GSM_HREQAVE );
             // Bug # 3436 fix - end
            line3 = ( float )bsInfo->handoverMargin; // GSM_HO_MARGIN_DEF;

            // Handover condition 2.
            if (line1 - line2 - line3 > 0 ){
                suitableCells[numSuitableCells] = n;

                numSuitableCells++;

                if (DEBUG_GSM_LAYER3_2 ){
                    printf(
                        "HO.2 ok: cause %d N.Cell[%d], numSuitable %d, %f\n",
                        hoCause,
                        n,
                        numSuitableCells,
                        line1 - line2 - line3 );
                }

                // Should add sorting to determine which cell is the most
                // suitable one.
            }
        } // Handover condition 1.
    } // for loop to find suitable cells //

    if (numSuitableCells >= 1 && slotInfo->isHandoverInProgress == FALSE ){
        // Initate HO with MSC
        GsmLayer3BuildHandoverRequiredMsg( bsInfo,
            hoCause,
            numSuitableCells,
            suitableCells,
            &hoReqdMsg );

        GsmLayer3SendPacketViaGsmAInterface( node,
            bsInfo->mscNodeAddress,
            node->nodeId,
            slotInfo->sccpConnectionId,
            routingLabel,
            GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
            ( unsigned char* )hoReqdMsg,
            sizeof(
            GsmHandoverRequiredMsg ));

        MEM_free( hoReqdMsg );
        slotInfo->isHandoverInProgress = TRUE;

        bsInfo->stats.handoverRequiredSent++;
    }
    else{
        if (DEBUG_GSM_LAYER3_2 ){
            printf( "GSM_BS[%d]: No Cells(s) for handover found,"
                        " TS %d, Up.CH %d\n",
                node->nodeId,
                slotInfo->slotNumber,
                slotInfo->ulChannelIndex );
        }
    }
} // End of startHandover //

static
void GsmLayer3MsInitiateCallClearing(
    Node* node,
    GsmLayer3MsInfo* msInfo,
    unsigned char* cause)
{
    Message*                newMsg;


    GsmLayer3MsStopCallControlTimers( node,msInfo );

    if (DEBUG_GSM_LAYER3_TIMERS ){
        // printf("GSM_MS[%d]: Setting T305\n", node->nodeId);
    }

    msInfo->timerT305Msg = GsmLayer3StartTimer( node,
                               MSG_NETWORK_GsmDisconnectTimer_T305,
                               DefaultGsmDisconnectTimer_T305Time,
                               NULL,
                               0 );

    msInfo->ccState = GSM_MS_CC_STATE_DISCONNECT_REQUEST;

    GsmLayer3BuildDisconnectByMsMsg( node,&newMsg,cause );

    GsmLayer3SendPacket( node,
        newMsg,
        msInfo->assignedSlotNumber,
        msInfo->assignedChannelIndex,
        GSM_CHANNEL_FACCH );

    msInfo->stats.disconnectSent++;
} // End of GsmLayer3MsInitiateCallClearing //


static
void GsmLayer3MscInitiateCallClearing(
    Node* node,
    GsmMscCallInfo* callInfo,
    unsigned char       originMsCause1,
    unsigned char       originMsCause2,
    unsigned char       termMsCause1,
    unsigned char       termMsCause2)
{
    GsmDisconnectByNetworkMsg*  disconnectMsg;
    GsmSccpRoutingLabel         routingLabel;
    GsmMscTimerData             newMscTimerData;


    memset( &routingLabel,0,sizeof( GsmSccpRoutingLabel ) );

    // For Term. MS
    callInfo->termMs.nwCcState = (
        GsmNwCcState_e )GSM_MS_CC_STATE_DISCONNECT_INDICATION;

    GsmLayer3BuildDisconnectByNetworkMsg( node,
        &disconnectMsg,
        termMsCause1,
        //GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
        termMsCause2 ); //GSM_CAUSE_VALUE_USER_ALERTING_NO_ANSWER);

    GsmLayer3SendPacketViaGsmAInterface( node,
        callInfo->termMs.bsNodeAddress,
        node->nodeId,
        callInfo->termMs.sccpConnectionId,
        routingLabel,
        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
        ( unsigned char* )disconnectMsg,
        sizeof( GsmDisconnectByNetworkMsg ) );

    MEM_free( disconnectMsg );

    newMscTimerData.callInfo = callInfo;
    newMscTimerData.isSetForOriginMs = FALSE;
    //newMscTimerData.cause[0] =
    //      GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER;
    //newMscTimerData.cause[1] =
    //      GSM_CAUSE_VALUE_USER_ALERTING_NO_ANSWER;

    callInfo->termMs.timerT305Msg = GsmLayer3StartTimer( node,
                                        MSG_NETWORK_GsmDisconnectTimer_T305,
                                        DefaultGsmDisconnectTimer_T305Time,
                                        &newMscTimerData,
                                        sizeof( GsmMscTimerData * ) );

    // Now for Origin MS
    callInfo->originMs.nwCcState = (
        GsmNwCcState_e )GSM_MS_CC_STATE_DISCONNECT_INDICATION;

    GsmLayer3BuildDisconnectByNetworkMsg( node,
        &disconnectMsg,
        originMsCause1,
        //GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
        originMsCause2 ); //GSM_CAUSE_VALUE_RECOVERY_ON_TIMER_EXPIRY);

    GsmLayer3SendPacketViaGsmAInterface( node,
        callInfo->originMs.bsNodeAddress,
        node->nodeId,
        callInfo->originMs.sccpConnectionId,
        routingLabel,
        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
        ( unsigned char* )disconnectMsg,
        sizeof( GsmDisconnectByNetworkMsg ) );

    newMscTimerData.isSetForOriginMs = TRUE;
    //newMscTimerData.cause[0] =
    //      GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER;
    //newMscTimerData.cause[1] =
    //      GSM_CAUSE_VALUE_RECOVERY_ON_TIMER_EXPIRY;

    callInfo->originMs.timerT305Msg = GsmLayer3StartTimer(
                                          node,
                                          MSG_NETWORK_GsmDisconnectTimer_T305,
                                          DefaultGsmDisconnectTimer_T305Time,
                                          &newMscTimerData,
                                          sizeof( GsmMscTimerData * ) );

    MEM_free( disconnectMsg );

    if (DEBUG_GSM_LAYER3_TIMERS ){
        printf( "GSM_MSC[%d]: Setting T305 for Origin MS & "
                    "Term. MS\n",
            node->nodeId );
    }
} // End of GsmLayer3MscInitiateCallClearing //

/*
 * FUNCTION:   GsmLayer3BsBuildSACCHTimer
 *
 * PURPOSE:    BS's Build SACCH packet
 */
static
void GsmLayer3BsBuildSACCHTimer(
    Node* node,
    GsmLayer3BsInfo* bsInfo)
{
    Message* msg;
    GsmLayer3Data*      nwGsmData;
    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    msg = MESSAGE_Alloc( node,
             NETWORK_LAYER,
             NETWORK_PROTOCOL_GSM,
             MSG_NETWORK_GsmTimer_BSSACCH );

    msg->protocolType = NETWORK_PROTOCOL_GSM;
    MESSAGE_SetInstanceId( msg, ( short ) nwGsmData->interfaceIndex );
    bsInfo->timerSACCHMsg = msg;
}

/*
 * FUNCTION:   GsmLayer3BsSendSACCHTimer
 *
 * PURPOSE:    BS's send SACCH Timer
 */
static
void GsmLayer3BsSendSACCHTimer(
    Node* node,
    GsmLayer3BsInfo* bsInfo)
{
    Message*                msg;

    msg = MESSAGE_Duplicate( node, bsInfo->timerSACCHMsg );
    MESSAGE_Send( node,msg,DefaultGsmBsTimer_SACCHTime );
} // End of GsmLayer3BsSendSACCHTimer

/*
 * GSM Layer 3 functions
 */


void GsmLayer3Init(
    Node* node,
    const NodeInput* nodeInput)
{
    BOOL            isNodeId;
    BOOL            wasFound            = FALSE;

    char            nodeIdString[MAX_STRING_LENGTH];
    char            gsmStr[MAX_STRING_LENGTH];
    char            errorString[MAX_STRING_LENGTH];
    char*           getTokenVal;
    char*           next;
    char            token[MAX_STRING_LENGTH];

    int             lineCount           = 0;
    int             index;
    int             retVal;
    int             bsCounter           = 0;
    int             numBs               = 0;
    int             interfaceCount;

    GsmLayer3Data*  nwGsmData;
    NodeInput*      gsmNodeInput;
    NodeId          nodeId;
    NodeAddress     nodeAddress;

    int             numMsSpecified      = 0;
    int             numBsSpecified      = 0;
    int             numMscSpecified     = 0;

    // Allocate & initialize
    nwGsmData = ( GsmLayer3Data * )MEM_malloc( sizeof( GsmLayer3Data ) );

    memset( nwGsmData,0,sizeof( GsmLayer3Data ) );

    node->networkData.gsmLayer3Var = nwGsmData;
    RANDOM_SetSeed(nwGsmData->randSeed,
                   node->globalSeed,
                   node->nodeId,
                   NETWORK_PROTOCOL_GSM);

    // Read the config file.
    // MS's will read their IMEI, IMSI, MSISDN, PLMN info ... and
    // the call setup information.

    gsmNodeInput = ( NodeInput * )MEM_malloc( sizeof( NodeInput ) );

    IO_ReadCachedFile( ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "GSM-NODE-CONFIG-FILE",
        &wasFound,
        gsmNodeInput );

    if (wasFound == FALSE ){
        sprintf( errorString,
            "Node [%d] Could not find GSM-NODE-CONFIG-FILE\n",
            node->nodeId );
        ERROR_ReportError( errorString );
    }

    for (lineCount = 0; lineCount < gsmNodeInput->numLines; lineCount++ ){
        sscanf( gsmNodeInput->inputStrings[lineCount],"%s",gsmStr );

        if (strcmp( gsmStr,"GSM-MS" ) == 0 ){
            /***
            Format of MS input
            GSM-MS [Nodeid] <IMSI(10 digits)>
                // optional: {<Called MSISDN> <StartTime> <EndTime>}
            ***/
            numMsSpecified++;

            retVal = sscanf( gsmNodeInput->inputStrings[lineCount],
                         "%*s %s",
                         // %s %s %s %s",
                         nodeIdString
                         /** from .app file
                         imsiString
                         destMsIsdnString,
                         callStartTimeStr,
                         callEndTimeStr **/ );

            IO_ParseNodeIdOrHostAddress( nodeIdString,&nodeId,&isNodeId );

            if (!isNodeId ){
                sprintf( errorString,
                    "GSM-NODE-CONFIG-FILE: '%s'\n"
                        "Second parameter needs to be a node id",
                    gsmNodeInput->inputStrings[lineCount] );

                ERROR_ReportError( errorString );
            }

            nodeAddress = MAPPING_GetDefaultInterfaceAddressFromNodeId( node,
                              nodeId );
            if (nodeAddress == INVALID_MAPPING ){
                sprintf( errorString,
                    "GSM-NODE-CONFIG-FILE: '%s'\n"
                        "Node %d does not exist",
                    gsmNodeInput->inputStrings[lineCount],
                    nodeId );
                ERROR_ReportError( errorString );
            }

            // If for this MS node
            if (isNodeId == TRUE && nodeId == node->nodeId ){
                GsmLayer3MsInfo*        msInfo;


                nwGsmData->nodeType = GSM_MS;

                nwGsmData->msInfo = (
                    GsmLayer3MsInfo * )MEM_malloc(
                    sizeof( GsmLayer3MsInfo ) );

                msInfo = nwGsmData->msInfo;

                memset( msInfo->numMsgsToSendToMac,
                    0,
                    sizeof( int ) * GSM_SLOTS_PER_FRAME );
                memset( msInfo,0,sizeof( GsmLayer3MsInfo ) );

                msInfo->rrState = GSM_MS_RR_STATE_NULL;
                msInfo->mmState = GSM_MS_MM_STATE_NULL;
                msInfo->mmIdleSubState = GSM_MS_MM_IDLE_PLMN_SEARCH;
                msInfo->mmUpdateStatus =
                GSM_MS_MM_UPDATE_STATUS_U2_NOT_UPDATED;
                msInfo->ccState = GSM_MS_CC_STATE_NULL;

                msInfo->isDedicatedChannelAssigned = FALSE;
                msInfo->assignedChannelIndex = GSM_INVALID_CHANNEL_INDEX;
                msInfo->assignedSlotNumber = GSM_INVALID_SLOT_NUMBER;
                msInfo->assignedChannelType = GSM_INVALID_CHANNEL_TYPE;

                msInfo->downLinkControlChannelIndex =
                GSM_INVALID_CHANNEL_INDEX;

                msInfo->timerT303Msg = NULL;
                msInfo->timerT305Msg = NULL;
                msInfo->timerT308Msg = NULL;
                msInfo->timerT310Msg = NULL;
                msInfo->timerT313Msg = NULL;
                msInfo->timerT3210Msg = NULL;
                msInfo->timerT3211Msg = NULL;
                msInfo->timerT3212Msg = NULL;
                msInfo->timerT3230Msg = NULL;
                msInfo->timerT3240Msg = NULL;
                msInfo->timerT3213Msg = NULL;
                msInfo->channelRequestTimer = NULL;

                msInfo->locationUpdateAttemptCounter = 0;

                // Set the MS's IMSI
                {
                    char    imsiStr[GSM_MAX_IMSI_LENGTH + 1];
                    int     imsiLength;


                    // Create IMSI. "0000000021" for nodeId 21
                    sprintf( imsiStr,"%u",node->nodeId );
                    imsiLength = (int)strlen( imsiStr );

                    for (index = 0; index < imsiLength; index++ ){
                        msInfo->gsmImsi[GSM_MAX_IMSI_LENGTH - 1 - index] = (
                            unsigned char ) (
                            imsiStr[imsiLength - 1 - index] - '0' );
                    }
                }

                // MSISDN is set the same as the MS's IMSI
                memcpy( msInfo->gsmMsIsdn,
                    msInfo->gsmImsi,
                    GSM_MAX_MSISDN_LENGTH );


                if (DEBUG_GSM_LAYER3_3 ){
                    printf( "GSM_MS[%d] Layer3 Init: IMSI: ",node->nodeId );
                    printGsmImsi( msInfo->gsmImsi,GSM_MAX_IMSI_LENGTH );
                    printf( "\n" );
                }

                // Identify the GSM interface of this node
                for (interfaceCount = 0;
                        interfaceCount < node->numberInterfaces;
                        interfaceCount++ ){
                    if (node->macData[interfaceCount]->macProtocol ==
                           MAC_PROTOCOL_GSM ){
                        msInfo->interfaceIndex =
                        node->macData[interfaceCount]->interfaceIndex;
                        break;
                    }
                }

                if (interfaceCount == node->numberInterfaces ){
                    sprintf( errorString,
                        "GSM_MS[%d] Layer3: MAC interface not found\n",
                        node->nodeId );
                    ERROR_ReportError( errorString );
                }

                GsmLayer3InitStats( node,nodeInput );
            } // End of this node's processing //
        } // End of GSM-MS //
        else if (strcmp( gsmStr,"GSM-BS" ) == 0 ){
            GsmLayer3BsInfo*        bsInfo;
            GsmChannelDescription*  channelDesc;

            numBsSpecified++;

            // Read BS parameters

            /***
                 Format of BS parameters:
                 GSM-BS <NodeId> <LAC> <CellId> <ChannelRange>
                     <MSC NodeId> <Neigh.BS Info>
            ***/

            // skip past the "GSM-BS" token
            getTokenVal = IO_GetToken( token,
                              gsmNodeInput->inputStrings[lineCount],
                              &next );

            getTokenVal = IO_GetToken( token,next,&next );

            IO_ParseNodeIdOrHostAddress( token,&nodeId,&isNodeId );

            if (!isNodeId ){
                sprintf( errorString,
                    "%s: Second parameter needs to be a node Id: \nLine: %s",
                    "GSM-NODE-CONFIG-FILE",
                    gsmNodeInput->inputStrings[lineCount] );

                ERROR_ReportError( errorString );
            }
            nodeAddress = MAPPING_GetDefaultInterfaceAddressFromNodeId( node,
                              nodeId );
            if (nodeAddress == INVALID_MAPPING ){
                sprintf( errorString,
                    "%s: Node %d does not exist\n %s\n",
                    "GSM-NODE-CONFIG-FILE",
                    nodeId,
                    gsmNodeInput->inputStrings[lineCount] );
                ERROR_ReportError( errorString );
            }

            // If for this node
            if (isNodeId == TRUE && nodeId == node->nodeId ){
                short       numChannelsAssigned;
                short       channel;


                GsmCellInfo cellInfo;


                nwGsmData->nodeType = GSM_BS;
                nwGsmData->bsInfo = (
                    GsmLayer3BsInfo * )MEM_malloc(
                    sizeof( GsmLayer3BsInfo ) );
                bsInfo = nwGsmData->bsInfo;
                memset( bsInfo,0,sizeof( GsmLayer3BsInfo ) );

                // Read node config file information for this BS
                getTokenVal = IO_GetToken( token,next,&next );

                setGSMNodeLAC( node,( short )strtoul( token,NULL,10 ) );

                if (!IO_IsStringNonNegativeInteger( token ) ){
                    sprintf( errorString,
                        "Illegal Location Area Code %s for BS node %d",
                        token,
                        nodeId );
                    ERROR_ReportError( errorString );
                }

                getTokenVal = IO_GetToken( token,next,&next );

                setGSMNodeCellId( node,strtoul( token,NULL,10 ) );

                if (!IO_IsStringNonNegativeInteger( token ) ){
                    sprintf( errorString,
                        "Illegal CellIdentity %s for BS node %d",
                        token,
                        nodeId );
                    ERROR_ReportError( errorString );
                }

                // Read the channels assigned to this BS
                getTokenVal = IO_GetToken( token,next,&next );
                GsmParseChannelRange( token,
                    &bsInfo->channelIndexStart,
                    &bsInfo->channelIndexEnd );

                numChannelsAssigned = (
                    short )( bsInfo->channelIndexEnd -
                               bsInfo->channelIndexStart + 1 );

                if (DEBUG_GSM_LAYER3_3 ){
                    printf( "GSM_BS[%d] Layer3 Init:"
                                " LAC %d, CellId %d, %d Channels: %d-%d\n",
                        node->nodeId,
                        returnGSMNodeLAC( node ),
                        returnGSMNodeCellId( node ),
                        numChannelsAssigned,
                        bsInfo->channelIndexStart,
                        bsInfo->channelIndexEnd );
                }

                if (numChannelsAssigned < GSM_MIN_NUM_CHANNELS_PER_BS ||
                       numChannelsAssigned > GSM_MAX_NUM_CHANNELS_PER_BS ){
                    sprintf(
                        errorString,
                        "GSM-NODE-CONFIG-FILE: Number of allowed channels per"
                            " BS is %d-%d",
                        GSM_MIN_NUM_CHANNELS_PER_BS,
                        GSM_MAX_NUM_CHANNELS_PER_BS );
                    ERROR_ReportError( errorString );
                }

                memset( bsInfo->numMsgsToSendToMac,
                    0,
                    sizeof(
                    int ) * GSM_SLOTS_PER_FRAME * numChannelsAssigned );

                bsInfo->totalNumChannelPairs = (
                    short )( numChannelsAssigned / 2 );
                bsInfo->controlChannelIndex = bsInfo->channelIndexStart;

                // Parse, verify the controlling MSC info.
                getTokenVal = IO_GetToken( token,next,&next );
                IO_AppParseDestString( node,
                    gsmNodeInput->inputStrings[lineCount],
                    token,
                    &( bsInfo->mscNodeId ),
                    &( bsInfo->mscNodeAddress ) );

                getTokenVal = IO_GetToken( token,next,&next );

                // Parse neighbouring BS/cell information
                numBs = 0;
                while (getTokenVal ){
                    if (GsmParseAdditionalInfo( token,
                             &( bsInfo->neighBcchChannels[numBs] ),
                             &bsInfo->neighLac[numBs],
                             &bsInfo->neighCellIdentity[numBs] ) == FALSE ){
                        sprintf( errorString,
                            "GSM-NODE-CONFIG-FILE: "
                                "Incorrectly formatted BS node %d string %s",
                            node->nodeId,
                            token );
                        ERROR_ReportError( errorString );
                    }
                    numBs++;
                    getTokenVal = IO_GetToken( token,next,&next );
                }


                if (numBs > GSM_NUM_NEIGHBOUR_CELLS ){
                    ERROR_ReportError(
                        "More than GSM_NUM_NEIGHBOUR_CELLS specified" );
                }

                bsInfo->numNeighBcchChannels =
                    (uddtNumNeighBcchChannels) numBs;

                // Initialize slot information with channel values
                for (channel = 0;
                        channel < bsInfo->totalNumChannelPairs;
                        channel++ ){
                    int slotIndex;


                    channelDesc = &( bsInfo->channelDesc[channel] );

                    for (slotIndex = 0;
                            slotIndex < GSM_SLOTS_PER_FRAME;
                            slotIndex++ ){
                        channelDesc->slotUsageInfo[slotIndex].\
                        channelPairIndex = channel;

                        channelDesc->slotUsageInfo[slotIndex].dlChannelIndex
                            = ( short )(
                            bsInfo->channelIndexStart + channel * 2 );

                        channelDesc->slotUsageInfo[slotIndex].ulChannelIndex
                            = ( short )(
                            bsInfo->channelIndexStart + channel * 2 + 1 );
                    }
                }

                // Set the C0 Channel - downlink beacon channel
                // which is never deallocated/freed up.
                // The downlink channel is used for BCCH & CCCH msgs,
                // the uplink channel is used for RACCH by MS locked
                // to that BS.

                bsInfo->numChannelPairsInUse = 1;
                channelDesc = &( bsInfo->channelDesc[0] );
                channelDesc->inUse = TRUE;
                channelDesc->slotUsageInfo[0].inUse = TRUE;

                bsInfo->channelDesc[0].slotUsageInfo[0].isUsedForTraffic =
                FALSE;    // Only for signalling, always.

                bsInfo->numSccpConnections = 0;

                // Identify the GSM interface
                for (interfaceCount = 0;
                        interfaceCount < node->numberInterfaces;
                        interfaceCount++ ){
                    if (node->macData[interfaceCount]->macProtocol ==
                           MAC_PROTOCOL_GSM ){
                        nwGsmData->bsInfo->interfaceIndex =
                        node->macData[interfaceCount]->interfaceIndex;
                        break;
                    }
                }

                if (interfaceCount == node->numberInterfaces ){
                    sprintf( errorString,
                        "GSM_MS[%d] MAC interface not found\n",
                        node->nodeId );
                    ERROR_ReportError( errorString );
                }

                // Send neighbouring cell info (Control channels)
                // to MAC layer for Sys Info message broadcasts.


                cellInfo.cellIdentity = returnGSMNodeCellId( node );
                cellInfo.lac = returnGSMNodeLAC( node );

                cellInfo.txInteger = GSM_TX_INTEGER;
                cellInfo.maxReTrans = GSM_MAX_RETRANS;
                cellInfo.channelIndexStart = bsInfo->channelIndexStart;
                cellInfo.channelIndexEnd = bsInfo->channelIndexEnd;
                cellInfo.numNeighBcchChannels = bsInfo->numNeighBcchChannels;

                memcpy( cellInfo.neighBcchChannelIndex,
                    bsInfo->neighBcchChannels,
                    sizeof( int ) * bsInfo->numNeighBcchChannels );

                GsmLayer3SendInterLayerMsg( node,
                    bsInfo->interfaceIndex,
                    GSM_L3_MAC_CELL_INFORMATION,
                    ( unsigned char* )&cellInfo,
                    sizeof( GsmCellInfo ) );

                // Handover Margin
                IO_ReadDouble( node->nodeId,
                    ANY_DEST,
                    nodeInput,
                    "GSM-HANDOVER-MARGIN",
                    &wasFound,
                    &bsInfo->handoverMargin );

                if (wasFound == FALSE || bsInfo->handoverMargin < 0.0 ||
                       bsInfo->handoverMargin > 24.0 ){
                    bsInfo->handoverMargin = DefaultGsmHandoverMargin;
                }

                // this timer used by BS to identify the unused resource
               GsmLayer3BsBuildSACCHTimer(node,bsInfo);
               GsmLayer3BsSendSACCHTimer(node,bsInfo);

                GsmLayer3InitStats( node,nodeInput );
            } // Done for this node //
        } // End of GSM_BS //
        else if (strcmp( gsmStr,"GSM-MSC" ) == 0 ){
            /*** Read MSC parameters of the form
                 GSM-MSC <Nodeid> <Linked BSInfo>
                 where <Linked BSInfo> is of the form:
                     "BSNodeId-Lac-CellIdentity"
            ***/

            numMscSpecified++;

            // skip past the "GSM-MSC" token
            getTokenVal = IO_GetToken( token,
                              gsmNodeInput->inputStrings[lineCount],
                              &next );

            getTokenVal = IO_GetToken( token,next,&next );

            IO_ParseNodeIdOrHostAddress( token,&nodeId,&isNodeId );

            if (!isNodeId ){
                sprintf( errorString,
                    "GSM-NODE-CONFIG-FILE: Second parameter needs to be"
                        " a node Id: \nLine: %s",
                    gsmNodeInput->inputStrings[lineCount] );

                ERROR_ReportError( errorString );
            }
            nodeAddress = MAPPING_GetDefaultInterfaceAddressFromNodeId( node,
                              nodeId );
            if (nodeAddress == INVALID_MAPPING ){
                sprintf( errorString,
                    "GSM-NODE-CONFIG-FILE: Node %d does not exist\n %s\n",
                    nodeId,
                    gsmNodeInput->inputStrings[lineCount] );
                ERROR_ReportError( errorString );
            }

            // If for this node
            if (nodeId == node->nodeId ){
                GsmLayer3MscInfo*   mscInfo;


                nwGsmData->nodeType = GSM_MSC;
                nwGsmData->mscInfo = (
                    GsmLayer3MscInfo * )MEM_malloc(
                    sizeof( GsmLayer3MscInfo ) );
                mscInfo = nwGsmData->mscInfo;
                memset( mscInfo,0,sizeof( GsmLayer3MscInfo ) );

                if (DEBUG_GSM_LAYER3_3 ){
                    printf( "GSM-MSC [%d] Layer3 Init\n",node->nodeId );
                }

                // skip past the "GSM-MSC" token
                getTokenVal = IO_GetToken( token,next,&next );

                // Parse info on BS's controlled by the MSC
                numBs = 0;
                while (getTokenVal ){
                    if (GsmParseAdditionalInfo( token,
                             ( int* )&mscInfo->bsNodeId[numBs],
                             &mscInfo->bsLac[numBs],
                             &mscInfo->bsCellIdentity[numBs] ) == FALSE ){
                        sprintf(
                            errorString,
                            "GSM-NODE-CONFIG-FILE: "
                                "Incorrectly formatted MSC node %d string %s",
                            node->nodeId,
                            token );
                        ERROR_ReportError( errorString );
                    }

                    mscInfo->bsNodeAddress[numBs] =
                        MAPPING_GetDefaultInterfaceAddressFromNodeId(
                            node,
                            mscInfo->bsNodeId[numBs] );

                    // Validate that BS node exists
                    if (mscInfo->bsNodeAddress[numBs] == INVALID_MAPPING ){
                        sprintf( errorString,
                            "GSM-NODE-CONFIG-FILE: "
                                "Non-existent BS node %d in MSC"
                                " node %d string",
                            mscInfo->bsNodeId[numBs],
                            node->nodeId );
                        ERROR_ReportError( errorString );
                    }

                    numBs++;
                    getTokenVal = IO_GetToken( token,next,&next );
                }

                if (numBs > GSM_MAX_BS_PER_MSC ){
                    sprintf( errorString,
                        "GSM-NODE-CONFIG-FILE: "
                            "More than %d BS's specified for MSC node %d",
                        GSM_MAX_BS_PER_MSC,
                        nodeId );
                    ERROR_ReportError( errorString );
                }

                mscInfo->numBs = numBs;

                if (DEBUG_GSM_LAYER3_3 ){
                    printf( "\t%d BS(s) linked to MSC [%d]\n",
                        mscInfo->numBs,
                        node->nodeId );
                    for (bsCounter = 0; bsCounter < numBs; bsCounter++ ){
                        printf(
                            "\tBS NodeId [%d] Lac %d, Cell Identity %d\n",
                            mscInfo->bsNodeId[bsCounter],
                            mscInfo->bsLac[bsCounter],
                            mscInfo->bsCellIdentity[bsCounter] );
                    }
                }

                GsmLayer3InitStats( node,nodeInput );
            } // Node specific initialization //
        } // End of GSM_MSC //
        else{
            sprintf( errorString,
                "%s: Incorrect node type: %s",
                "GSM-NODE-CONFIG-FILE",
                gsmStr );
            ERROR_ReportError( errorString );
        }
    } // End of for line processing of gsm config file //


    // Identify if the node has more than one GSM interface
    int interfacesFound = 0;
    for (interfaceCount = 0;
            interfaceCount < node->numberInterfaces;
            interfaceCount++ ){
        if (node->macData[interfaceCount]->macProtocol == MAC_PROTOCOL_GSM ){
            interfacesFound++;
        }
    }

    if (interfacesFound > 1)
    {
        sprintf( errorString,
            "Node %d has more than one GSM MAC interface.",
            node->nodeId);
        ERROR_ReportError( errorString );
    }

    if (numMsSpecified > ( GSM_MAX_MS_PER_MSC * GSM_MAX_NUM_MSC ) ){
        sprintf( errorString,
            "GSM-NODE-CONFIG-FILE: '%s'\n"
                "Max MS's allowed limit: %d reached. "
                "Change value of GSM_MAX_MS_PER_MSC in "
                "cellular_gsm.h",
            gsmNodeInput->inputStrings[lineCount],
            GSM_MAX_MS_PER_MSC );
        ERROR_ReportError( errorString );
    }

    if (numBsSpecified > ( GSM_MAX_BS_PER_MSC * GSM_MAX_NUM_MSC ) ){
        ERROR_ReportError( "More than (GSM_MAX_BS_PER_MSC *"\
            "GSM_MAX_NUM_MSC) BS are specified in the given sceneraio" );
    }

    if (numBsSpecified < 1 ){
        sprintf( errorString,
            "%s: Atleast one BS needs to be specified",
            "GSM-NODE-CONFIG-FILE" );
        ERROR_ReportError( errorString );
    }
    else if (numMscSpecified != 1 ){
        sprintf( errorString,
            "%s: Exactly one MSC should be specified",
            "GSM-NODE-CONFIG-FILE" );
        ERROR_ReportError( errorString );
    }

    MEM_free(gsmNodeInput);
} // End of GsmLayer3Init //



void GsmLayer3Layer(
    Node* node,
    Message* msg)
{
    char clockStr[MAX_STRING_LENGTH];
    char errorString[MAX_STRING_LENGTH];

    GsmLayer3Data* nwGsmData;

    nwGsmData = (GsmLayer3Data *)node->networkData.gsmLayer3Var;
    ctoa(node->getNodeTime(), clockStr);



    // Only timer expiration messages expected
    if (nwGsmData->nodeType == GSM_MS)
    {
        GsmLayer3MsInfo* msInfo;
        msInfo = nwGsmData->msInfo;

        switch (msg->eventType)
        {
            case MSG_NETWORK_GsmChannelRequestTimer:
            {
                Message* channelReqMsg;
                msInfo->channelRequestTimer = NULL;

                if (msInfo->isRequestingChannel == TRUE &&
                    msInfo->channelRequestMsg != NULL)
                {
                    if (msInfo->numChannelRequestAttempts >
                        msInfo->maxReTrans + 1)
                    {
                        GsmChannelRequestMsg* chanReqMsg;

                        if (DEBUG_GSM_LAYER3_3)
                        {
                            printf("GSM_MS[%d]: Max CH-REQ %d+1 att. %sS\n",
                                node->nodeId,
                                msInfo->maxReTrans + 1,
                                clockStr);
                        }
                        if (DEBUG_GSM_LAYER3_TIMERS)
                        {
                            printf("GSM_MS[%d] TODO: Start T3213\n",
                                node->nodeId);
                        }

                        // Random Access Failure: start T3213 TODO:
                        chanReqMsg = (GsmChannelRequestMsg *)
                            MESSAGE_ReturnPacket(msInfo->channelRequestMsg);

                        if (chanReqMsg->est_cause == 
                            GSM_EST_CAUSE_LOCATION_UPDATING)
                        {
                            msInfo->numtimerT3213Attempts++;
                            msInfo->timerT3213Msg = GsmLayer3StartTimer(
                                node,
                                MSG_NETWORK_GsmTimer_T3213,
                                DefaultGsmPeriodTimer_T3213Time,
                                NULL,
                                0);    // Optional size of data
                        }

                        MESSAGE_Free(node, msg);

                        if (msInfo->channelRequestMsg != NULL)
                        {
                            MESSAGE_Free(node, msInfo->channelRequestMsg);
                        }

                        msInfo->channelRequestMsg = NULL;
                        msInfo->stats.numChannelRequestAttemptsFailed++;
                        return;
                    }

                    msInfo->slotsToWait = (unsigned char)(GSM_RANDACCESS_S +
                        RANDOM_erand(nwGsmData->randSeed) * 
                        (msInfo->txInteger - 1));

                    if (DEBUG_GSM_LAYER3_2)
                    {
                        // printf("MS[%d]: Sending CH-REQ %sS, next %d TS\n",
                            // node->nodeId, clockStr, msInfo->slotsToWait);
                    }

                    msInfo->channelRequestTimer = GsmLayer3StartTimer(node,
                        MSG_NETWORK_GsmChannelRequestTimer,
                        msInfo->slotsToWait * DefaultGsmSlotDuration,
                        NULL,
                        0);    // Optional size of data

                    channelReqMsg = MESSAGE_Duplicate(node,
                        msInfo->channelRequestMsg);

                    GsmLayer3SendPacket(node,
                        channelReqMsg,
                        0,
                        //msInfo->slotsToWait % 8,
                        msInfo->downLinkControlChannelIndex + 1,
                        GSM_CHANNEL_RACCH);

                    msInfo->numChannelRequestAttempts++;
                } // End of channel request //
                else
                {
                    // Not requesting channel anymore;
                    // received immediate assignment or rejection.
                }
            }
            break;

            case MSG_NETWORK_GsmCallStartTimer:
            {
                unsigned int* info;
                char destMsIsdnStr[GSM_MAX_MSISDN_LENGTH + 1];
                int index;
                int msIsdnLength;
                unsigned int destNodeId;

                clocktype* endTime;
                Message* reqMsg;


                if (DEBUG_GSM_LAYER3_3)
                {
                    printf("GSM_MS[%d] Start of Call Setup:\n",
                        node->nodeId);
                }

                msInfo->callStartTime = node->getNodeTime();

                // retrieve dest node & call end time
                info = (unsigned int*)MESSAGE_ReturnInfo(msg);
                destNodeId = (unsigned int) *info;
                sprintf(destMsIsdnStr, "%u", destNodeId);
                msIsdnLength = (int) strlen(destMsIsdnStr);

                endTime = (clocktype *)(info + sizeof(unsigned int));
                msInfo->callEndTime = *endTime;
                msInfo->isCallOriginator = TRUE;

                for (index = 0; index < msIsdnLength; index++ )
                {
                    msInfo->destMsIsdn[GSM_MAX_MSISDN_LENGTH - 1 - index] = 
                        (unsigned char)
                        (destMsIsdnStr[msIsdnLength - 1 - index] - '0');
                }

                if (DEBUG_GSM_LAYER3_3 )
                {
                    char clockStr2[MAX_STRING_LENGTH];

                    ctoa(msInfo->callEndTime,clockStr2);
                    printf("\nGSM_MS[%d] -> ",node->nodeId);
                    printGsmImsi(msInfo->destMsIsdn, GSM_MAX_MSISDN_LENGTH);
                    printf(" Start %s, End %s\n",clockStr,clockStr2);
                }

                if (msInfo->rrState != GSM_MS_RR_STATE_IDLE)
                {
                    if (DEBUG_GSM_LAYER3_3)
                    {
                        char clockStr2[MAX_STRING_LENGTH];

                        ctoa(msInfo->callEndTime,clockStr2);
                        sprintf( errorString,
                            "GSM_MS[%d] In non-IDLE(RR) state at %s\n",
                            node->nodeId,
                            clockStr);
                        ERROR_ReportWarning(errorString);
                    }

                    MESSAGE_Free(node,msg);
                    //break;
                    return;
                }

                if (DEBUG_GSM_LAYER3_2)
                {
                    printf("\tsending Channel Request\n");
                }

                // TODO: Start T3126, see GSM 04.08, sec 3.3.1.2, 11.1.1
                GsmLayer3SendChannelRequestMsg(node,
                    &reqMsg,
                    GSM_EST_CAUSE_NORMAL_CALL);

                msInfo->rrState = GSM_MS_RR_STATE_CON_PEND;
                msInfo->mmState = 
                    GSM_MS_MM_STATE_WAIT_FOR_RR_CONNECTION_MM_CONN;
            } // End of CallStartTimer //
            break;

            case MSG_NETWORK_GsmCallEndTimer:
            {
                // Check MS state to see if it is connected or in another
                // state in which it can send a disconnect message.


                if (DEBUG_GSM_LAYER3_3)
                {
                    printf("GSM_MS[%d] End of call timer at %s:\n"
                        "\tsending a DISCONNECT msg "
                        "(MS initiated call clearing)\n",
                        node->nodeId,
                        clockStr);
                }

                if (msInfo->isDedicatedChannelAssigned == TRUE)
                {
                    msInfo->disconnectCause[0] = GSM_LOCATION_USER;
                    msInfo->disconnectCause[1] = 
                        GSM_CAUSE_VALUE_NORMAL_CALL_CLEARING;

                    GsmLayer3MsInitiateCallClearing(node,
                        msInfo,
                        msInfo->disconnectCause);
                    msInfo->isCallOriginator = FALSE;
                }
                else
                {
                    if (DEBUG_GSM_LAYER3_3)
                    {
                        printf("GSM_MS[%d] ERROR: MS not connected.\n"
                            "\tDisconnect msg not sent\n",
                            node->nodeId);
                    }
                }
            }
            break;

            case MSG_NETWORK_GsmSendTrafficTimer:
            {
                Message*  trafficMessage;

                if (msInfo->isDedicatedChannelAssigned == TRUE)
                {
                    // Check if handover in progress
                    if (msInfo->isHandoverInProgress == FALSE)
                    {
                        GsmLayer3BuildVirtualTrafficPacket(
                            node,
                            &trafficMessage);
                        GsmLayer3SendPacket(
                            node,
                            trafficMessage,
                            msInfo->assignedSlotNumber,
                            msInfo->assignedChannelIndex,
                            GSM_CHANNEL_TCH);
                        msInfo->stats.pktsSentTraffic++;
                    }
                    if (msInfo->callEndTime >= node->getNodeTime() + 
                        GSM_TRAFFIC_INPUT_RATE_DELAY)
                    {
                        Message* trafficTimerMsg;
                        trafficTimerMsg = MESSAGE_Alloc(
                            node,
                            NETWORK_LAYER,
                            0,
                            MSG_NETWORK_GsmSendTrafficTimer);
                        trafficTimerMsg->protocolType = NETWORK_PROTOCOL_GSM;
                        MESSAGE_Send(node,
                                     trafficTimerMsg,
                                     GSM_TRAFFIC_INPUT_RATE_DELAY );
                    }
                }
                else
                {
                    if (DEBUG_GSM_LAYER3_3)
                    {
                        printf("GSM_MS[%d] not connected. "
                            "Traffic msg %d not sent\n",
                            node->nodeId,
                            msInfo->stats.pktsSentTraffic);
                    }
                }
            } // End of SendTrafficTimer //
            break;

            case MSG_NETWORK_GsmHandoverTimer:
            {
                Message* newMsg1;
                Message* newMsg2;

                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MS[%d]: Handover Timer\n",
                        node->nodeId);
                }

                GsmLayer3BuildRIHandoverAccessMsg(node,
                                                  msInfo->hoReference,
                                                  &newMsg1);
                GsmLayer3SendPacket(node,
                                    newMsg1,
                                    msInfo->assignedSlotNumber,
                                    msInfo->assignedChannelIndex,
                                    GSM_CHANNEL_FACCH);
                GsmLayer3BuildRIHandoverCompleteMsg( node,&newMsg2 );
                GsmLayer3SendPacket(node,
                                    newMsg2,
                                    msInfo->assignedSlotNumber,
                                    msInfo->assignedChannelIndex,
                                    GSM_CHANNEL_FACCH);
                msInfo->isHandoverInProgress = FALSE;
            } // End of HandoverTimer //
            break;

            case MSG_NETWORK_GsmCallPresentTimer_T303:
            {
                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MS[%d]: T303 Expiration at %s\n",
                        node->nodeId,
                        clockStr);
                }
                msInfo->timerT303Msg = NULL;

                if (msInfo->ccState == GSM_MS_CC_STATE_MM_CONNECTION_PENDING)
                {
                    // CM_SERVICE_ACCEPT has not been received.
                    // Abort MM Connection in progress.
                    // Release the channel, reset states, update stats

                    GsmLayer3MsReleaseChannel(node, msInfo);
                    msInfo->rrState = GSM_MS_RR_STATE_IDLE;
                    msInfo->mmState = GSM_MS_MM_STATE_MM_IDLE;
                    msInfo->mmIdleSubState = GSM_MS_MM_IDLE_NORMAL_SERVICE;
                    msInfo->ccState = GSM_MS_CC_STATE_NULL;
                    msInfo->stats.mmConnectionFailures++;
                }
                else if (msInfo->ccState == GSM_MS_CC_STATE_CALL_INITIATED)
                {
                    unsigned char cause[2];
                    // CALL_PROCEEDING has not been received.
                    cause[0] = GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER;
                    cause[1] = GSM_CAUSE_VALUE_USER_NOT_RESPONDING;
                    GsmLayer3MsInitiateCallClearing(node, msInfo, cause);
                }
            } // End of T303 //
            break;

            case MSG_NETWORK_GsmDisconnectTimer_T305:
            {
                Message* newMsg;
                msInfo->timerT305Msg = NULL;

                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MS[%d]: T305 Expiration at %s, "
                        "send RELEASE\n",
                        node->nodeId,
                        clockStr );
                }

                if (msInfo->ccState != GSM_MS_CC_STATE_DISCONNECT_REQUEST)
                {
                    if (DEBUG_GSM_LAYER3_TIMERS)
                    {
                        sprintf(errorString, 
                            "GSM_MS[%d] ERROR: Incorrect state",
                            node->nodeId );
                        ERROR_ReportWarning(errorString);
                    }
                    MESSAGE_Free(node, msg);
                    return;
                }

                GsmLayer3BuildReleaseByMsMsg(node,
                                             &newMsg,
                                             msInfo->disconnectCause);

                GsmLayer3SendPacket(node,
                                    newMsg,
                                    msInfo->assignedSlotNumber,
                                    msInfo->assignedChannelIndex,
                                    GSM_CHANNEL_TCH);

                msInfo->ccState = GSM_MS_CC_STATE_RELEASE_REQUEST;

                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf( "GSM_MS[%d]: Setting T308\n",node->nodeId );
                }
                msInfo->timerT308Expirations = 0;
                msInfo->timerT308Msg = GsmLayer3StartTimer(
                    node,
                    MSG_NETWORK_GsmReleaseTimer_T308,
                    DefaultGsmReleaseTimer_T308Time,
                    NULL,
                    0 );
            } // End of T305 //
            break;

            case MSG_NETWORK_GsmReleaseTimer_T308:
            {
                Message* newMsg;
                msInfo->timerT308Msg = NULL;

                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MS[%d]: T308 Expiration at %s\n",
                        node->nodeId,
                        clockStr);
                }

                msInfo->timerT308Expirations++;

                if (msInfo->timerT308Expirations == 1)
                {
                    // Stay in RELEASE_REQUEST state

                    GsmLayer3BuildReleaseByMsMsg(node,
                                                 &newMsg,
                                                 msInfo->disconnectCause);

                    GsmLayer3SendPacket(node,
                                        newMsg,
                                        msInfo->assignedSlotNumber,
                                        msInfo->assignedChannelIndex,
                                        GSM_CHANNEL_TCH );

                    if (DEBUG_GSM_LAYER3_TIMERS)
                    {
                        printf("GSM_MS[%d]: Setting T308\n",
                            node->nodeId);
                    }
                    msInfo->timerT308Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmReleaseTimer_T308,
                        DefaultGsmReleaseTimer_T308Time,
                        NULL,
                        0);
                }
                else if (msInfo->timerT308Expirations >= 2)
                {
                    // Release MM Connection & return to NULL
                    if (DEBUG_GSM_LAYER3_TIMERS)
                    {
                        printf("GSM_MS[%d]: 2 Expirations of T308 at %s.\n"
                            "Releasing connections.\n",
                            node->nodeId,
                            clockStr);
                    }

                    GsmLayer3MsReleaseChannel(node, msInfo);
                    msInfo->rrState = GSM_MS_RR_STATE_IDLE;
                    msInfo->mmState = GSM_MS_MM_STATE_MM_IDLE;
                    msInfo->ccState = GSM_MS_CC_STATE_NULL;
                }
            } // End of T308 //
            break;

            case MSG_NETWORK_GsmCallProceedingTimer_T310:
            {
                unsigned char cause[2] = {0};
                msInfo->timerT310Msg = NULL;

                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MS[%d]: T310 Expiration at %s.\n"
                        "Releasing connections.\n",
                        node->nodeId,
                        clockStr);
                }

                // Perform Call Clearing:
                /***
                GsmLayer3MsReleaseChannel(node, msInfo);
                msInfo->rrState = GSM_MS_RR_STATE_IDLE;
                msInfo->mmState = GSM_MS_MM_STATE_MM_IDLE;
                msInfo->ccState = GSM_MS_CC_STATE_NULL;
                ***/

                //cause[0] =
                //cause[1] =
                GsmLayer3MsInitiateCallClearing(node, msInfo, cause);
            } // End of T310 //
            break;

            case MSG_NETWORK_GsmConnectTimer_T313:
            {
                unsigned char cause[2];
                msInfo->timerT313Msg = NULL;

                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MS[%d]: T313 Expiration at %s.\n"
                        "Releasing connections.\n",
                        node->nodeId,
                        clockStr);
                }

                // Perform Call Clearing:
                cause[0] = GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER;
                cause[1] = GSM_CAUSE_VALUE_NORMAL_UNSPECIFIED;
                GsmLayer3MsInitiateCallClearing( node,msInfo,cause );
            } // End of T313 //
            break;

            case MSG_NETWORK_GsmLocationUpdateRequestTimer_T3210:
            {
                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MS[%d]: T3210 Expiration, %s\n",
                        node->nodeId,
                        clockStr);
                }

                msInfo->timerT3210Msg = NULL;
                gsmMsProcessMmFailureLocationUpdate( node,msInfo );
            } // End of T3210 //
            break;

            case MSG_NETWORK_GsmLocationUpdateFailureTimer_T3211:
            {
                Message* reqMsg;
                msInfo->timerT3211Msg = NULL;

                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MS[%d]: T3211 Expiration, %s\n",
                        node->nodeId,
                        clockStr);
                }

                GsmLayer3SendChannelRequestMsg(
                    node,
                    &reqMsg,
                    GSM_EST_CAUSE_LOCATION_UPDATING);
            } // End of T3211 //
            break;

            case MSG_NETWORK_GsmPeriodicLocationUpdateTimer_T3212:
            {
                Message* reqMsg; 

                msInfo->timerT3212Msg = NULL;

                if (DEBUG_GSM_LAYER3_TIMERS )
                {
                    printf("GSM_MS[%d]: T3212 Expiration, %s\n",
                        node->nodeId,
                        clockStr);
                }


                if (msInfo->mmState != GSM_MS_MM_STATE_MM_IDLE)
                {
                    MESSAGE_Free(node, msg);
                    return;
                }

                if (msInfo->mmIdleSubState == 
                    GSM_MS_MM_IDLE_ATTEMPTING_TO_UPDATE)
                {
                    msInfo->locationUpdateAttemptCounter = 0;
                }

                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf( "\tMS in MM_IDLE\n");
                }

                msInfo->locationUpdatingType = 
                    GSM_PERIODIC_LOCATION_UPDATING;
                GsmLayer3SendChannelRequestMsg(
                    node,
                    &reqMsg,
                    GSM_EST_CAUSE_LOCATION_UPDATING);
                    //msInfo->timerT3212Msg = NULL;
            } // End of T3212 //
            break;

            case MSG_NETWORK_GsmCmServiceRequestTimer_T3230:
            {
                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MS[%d]: T3230 Expired at %s\n",
                        node->nodeId,
                        clockStr);
                }

                msInfo->timerT3210Msg = NULL;
                GsmLayer3MsReleaseChannel(node, msInfo);

                msInfo->rrState = GSM_MS_RR_STATE_IDLE;
                msInfo->mmState = GSM_MS_MM_STATE_MM_IDLE;
                msInfo->mmIdleSubState = GSM_MS_MM_IDLE_NORMAL_SERVICE;
            } // End of T3230 //
            break;

            case MSG_NETWORK_GsmTimer_T3240:
            {
                msInfo->timerT3240Msg = NULL;
                GsmLayer3MsReleaseChannel(node, msInfo);
                msInfo->rrState = GSM_MS_RR_STATE_IDLE;
                msInfo->mmState = GSM_MS_MM_STATE_MM_IDLE;
                msInfo->mmIdleSubState = GSM_MS_MM_IDLE_NORMAL_SERVICE;
            } // End of T3240 //
            break;

            // Re-send the Location update request message
            case MSG_NETWORK_GsmTimer_T3213:
            {
                Message* reqMsg = NULL;
                GsmLayer3MsInfo* msInfo;
                GsmLayer3Data* nwGsmData;

                nwGsmData = (GsmLayer3Data *)node->networkData.gsmLayer3Var;
                msInfo = nwGsmData->msInfo;

                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MS[%d]: T3213 Expiration, %s\n",
                        node->nodeId,
                        clockStr);
                }

                msInfo->timerT3213Msg = NULL;

                if (msInfo->numtimerT3213Attempts > 3)
                {
                    msInfo->numtimerT3213Attempts = 0;
                    GsmLayer3MsStopCallControlTimers(node, msInfo);

                    // send the command to select the cell again
                    // Decode BCCH
                    msInfo->isBcchDecoded = FALSE;
                    msInfo->isSysInfoType2Decoded = FALSE;
                    msInfo->isSysInfoType3Decoded = FALSE;

                    GsmLayer3SendInterLayerMsg(node,
                                               msInfo->interfaceIndex,
                                               GSM_L3_MAC_CELL_SELECTION,
                                               NULL,
                                               0);

                    if (msInfo->timerT3240Msg != NULL)
                    {
                        MESSAGE_CancelSelfMsg(node,
                                msInfo->timerT3240Msg);
                        msInfo->timerT3240Msg = NULL;
                    }

                    memset(msInfo->numMsgsToSendToMac,
                           0,
                           sizeof( int ) * GSM_SLOTS_PER_FRAME);
                    for (int i = 0; i < GSM_SLOTS_PER_FRAME; i++)
                    {
                        msInfo->msgListToMac[i] = NULL;
                    }

                    msInfo->rrState = GSM_MS_RR_STATE_IDLE;
                    msInfo->mmState = GSM_MS_MM_STATE_MM_IDLE;
                    msInfo->mmIdleSubState = GSM_MS_MM_IDLE_NORMAL_SERVICE;
                    msInfo->ccState = GSM_MS_CC_STATE_NULL;
                }
                else
                {
                    GsmLayer3SendChannelRequestMsg(
                        node,
                        &reqMsg,
                        GSM_EST_CAUSE_LOCATION_UPDATING);
                }
            }
            break;

            default:
            {
                sprintf(errorString,
                        "GSM_MS[%d] Unknown timer/event %d occured at %s\n",
                        node->nodeId,
                        msg->eventType,
                        clockStr);
                ERROR_ReportWarning( errorString );
            }
        } // End of MS eventType switch //
    } // End of GSM_MS //
    else if (nwGsmData->nodeType == GSM_BS)
    {
        GsmLayer3BsInfo* bsInfo = nwGsmData->bsInfo;

        switch (msg->eventType)
        {
            case MSG_NETWORK_GsmImmediateAssignmentTimer_T3101:
            {
                GsmChannelInfo* channelInfo;
                GsmChannelDescription* channelDesc;
                GsmSlotUsageInfo* slotInfo;
                channelInfo = (GsmChannelInfo *)MESSAGE_ReturnInfo(msg);
                if (GsmLayer3BsLookupMsBySlotAndChannel(bsInfo,
                    channelInfo->slotNumber,
                    channelInfo->channelIndex,
                    &channelDesc) == FALSE)
                {
                    // Channel's not in use anymore. Do nothing.
                    MESSAGE_Free(node,msg);
                    return;
                }

                slotInfo = 
                    &(channelDesc->slotUsageInfo[channelInfo->slotNumber]);

                if (slotInfo->isSeizedByMs == TRUE)
                {
                    // MS has started using it. Do nothing.
                    MESSAGE_Free(node, msg);
                    return;
                }
                else
                {
                    // MS has not seized the channel, release it.
                    if (DEBUG_GSM_LAYER3_3)
                    {
                        printf("GSM_BS[%d]: T3101 Expired for "
                               "channel (%d, %d), releasing it.\n",
                               node->nodeId,
                               channelInfo->slotNumber,
                               channelInfo->channelIndex );
                    }

                    GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmBsChannelReleaseTimer_T3111,
                        DefaultGsmBsChannelReleaseTimer_T3111Time,
                        &slotInfo,
                        sizeof(GsmSlotUsageInfo *));

                    bsInfo->stats.numT3101Expirations++;
                }
            } // End of Imm.AssignTimer_T3101 //
            break;

            case MSG_NETWORK_GsmBsChannelReleaseTimer_T3111:
            {
                GsmSlotUsageInfo** slotInfo;


                slotInfo = (GsmSlotUsageInfo **)MESSAGE_ReturnInfo(msg);

                if ((*slotInfo)->inUse == FALSE)
                {
                        // Do nothing.
                        MESSAGE_Free( node,msg );
                        return;
                }
                else
                {
                    GsmLayer3BsReleaseChannel(node, *slotInfo);
                }
            } // End of BS Channel Release Timer_T3111 //
            break;

            // Timer to identify the unused resource at BS end
            case MSG_NETWORK_GsmTimer_BSSACCH:
            {
                unsigned int l_uint_loop1, l_uint_loop2;

                for (l_uint_loop1 = 0; l_uint_loop1 < GSM_MAX_ARFCN_PER_BS;
                    l_uint_loop1++ )
                {
                    if (bsInfo->channelDesc[l_uint_loop1].inUse ==
                        TRUE)
                    {
                        for (l_uint_loop2 = 0; 
                            l_uint_loop2 < GSM_SLOTS_PER_FRAME;
                            l_uint_loop2++ )
                        {
                            if ((bsInfo->channelDesc[l_uint_loop1].\
                                slotUsageInfo[l_uint_loop2].inUse ==
                                TRUE) && (bsInfo->channelDesc[l_uint_loop1].\
                                slotUsageInfo[l_uint_loop2].\
                                isMeasReportReceived == TRUE) &&
                                ((node->getNodeTime() -
                                bsInfo->channelDesc[l_uint_loop1].\
                                slotUsageInfo[l_uint_loop2].\
                                lastSACCHupdate ) >
                                GSM_BSMinimumSACCH_UpdationPeriod))
                            {
                                // free the channel & other resource here
                                GsmCallClearCommandMsg* callclearCommMsg;
                                GsmSccpRoutingLabel routingLabel;
                                memset(&routingLabel,
                                       0,
                                       sizeof(GsmSccpRoutingLabel));

                                GsmLayer3BuildCallClearCommandMsg(
                                    node,
                                    &callclearCommMsg);

                                GsmLayer3SendPacketViaGsmAInterface(
                                    node,
                                    bsInfo->mscNodeAddress,
                                    node->nodeId,
                                    bsInfo->channelDesc[l_uint_loop1].\
                                    slotUsageInfo[l_uint_loop2].\
                                    sccpConnectionId,
                                    routingLabel,
                                    GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                                    (unsigned char*) callclearCommMsg,
                                    sizeof(GsmCallClearCommandMsg));

                                MEM_free(callclearCommMsg);

                                bsInfo->channelDesc[l_uint_loop1].\
                                    slotUsageInfo[l_uint_loop2].inUse = 
                                    FALSE;
                            }
                        }
                    }
                }

                GsmLayer3BsSendSACCHTimer(node, bsInfo);
            }
            break;
            default:
            {
                sprintf( errorString, "GSM_BS[%d]: Unknown event %d",
                    node->nodeId,
                    msg->eventType);
                ERROR_ReportError(errorString);
            }
        } // End of BS eventType switch //
    } // End of GSM_BS //
    else if (nwGsmData->nodeType == GSM_MSC)
    {
        int destSccpConnectionId;
        NodeAddress destBsNodeAddress;

        GsmMscCallInfo* callInfo;
        GsmMscTimerData* mscTimerData;
        GsmMscTimerData newMscTimerData;
        GsmSccpRoutingLabel routingLabel;


        mscTimerData = (GsmMscTimerData *)MESSAGE_ReturnInfo(msg);
        callInfo = mscTimerData->callInfo;
        memset(&routingLabel, 0, sizeof(GsmSccpRoutingLabel));

        if (DEBUG_GSM_LAYER3_3)
        {
            printf("GSM_MSC[%d] GsmLayer3Layer(), event %d\n",
                node->nodeId,
                msg->eventType);
        }

        switch (msg->eventType)
        {
            case MSG_NETWORK_GsmAlertingTimer_T301:
            {
                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MSC[%d]: T301 (Alerting) Expired\n",
                        node->nodeId );
                    printf("GSM_MSC[%d]: Clearing Call. Sending "
                        "DISCONNECT to Origin & Term. MS's\n",
                        node->nodeId);
                }

                GsmLayer3MscInitiateCallClearing(
                    node,
                    callInfo,
                    GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
                    GSM_CAUSE_VALUE_USER_ALERTING_NO_ANSWER,
                    GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
                    GSM_CAUSE_VALUE_RECOVERY_ON_TIMER_EXPIRY);

                    // Clear the call
                    // Initiate clearing towards calling user;
                    //      cause#19: user alerting, no answer
                    //  Initiate clearing towards called user;
                    //      cause#102: recovery on timer expiry
            } // End of T301 //
            break;

            case MSG_NETWORK_GsmCallPresentTimer_T303:
            {
                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MSC[%d]: T303 Expired. Clear call\n",
                        node->nodeId );
                    printf( "GSM_MSC[%d]: T303 Expired, IMSI \n",
                        node->nodeId );
                    printGsmImsi( callInfo->termMs.imsi,10 );
                    printf( "\n" );
                }

                // Initiate clearing towards calling user;
                // cause#18: no user responding
                // Initiate clearing towards called user;
                // cause#102: recovery on timer expiry

                // Clear the call:

                GsmLayer3MscInitiateCallClearing( 
                    node,
                    callInfo,
                    GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
                    GSM_CAUSE_VALUE_USER_NOT_RESPONDING,
                    GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
                    GSM_CAUSE_VALUE_RECOVERY_ON_TIMER_EXPIRY );
            } // End of T303 //
            break;

            case MSG_NETWORK_GsmDisconnectTimer_T305:
            {
                GsmReleaseByNetworkMsg* releaseMsg;

                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MSC[%d]: T305 Expired.TermMS.IMSI ",
                        node->nodeId );
                    printGsmImsi( callInfo->termMs.imsi,10 );
                    printf( "\n\tSending RELEASE\n");
                }
                // Send Release, start T308
                // Re-Send Release, restart T308:

                newMscTimerData.callInfo = callInfo;

                // Upon expiry of T308 Send RELEASE to term.MS
                if (DEBUG_GSM_LAYER3_TIMERS )
                {
                    printf("GSM_MSC[%d]: Setting T308. %d\n",
                        node->nodeId,
                        mscTimerData->isSetForOriginMs );
                }

                    
                if (mscTimerData->isSetForOriginMs == TRUE)
                {
                    destBsNodeAddress = callInfo->originMs.bsNodeAddress;
                    destSccpConnectionId = 
                        callInfo->originMs.sccpConnectionId;
                    newMscTimerData.isSetForOriginMs = TRUE;

                    callInfo->originMs.timerT308Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmReleaseTimer_T308,
                        DefaultGsmReleaseTimer_T308Time,
                        &newMscTimerData,
                        sizeof(GsmMscTimerData *));
                }
                else
                {
                    destBsNodeAddress = callInfo->termMs.bsNodeAddress;
                    destSccpConnectionId = callInfo->termMs.sccpConnectionId;
                    newMscTimerData.isSetForOriginMs = FALSE;

                    callInfo->termMs.timerT308Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmReleaseTimer_T308,
                        DefaultGsmReleaseTimer_T308Time,
                        &newMscTimerData,
                        sizeof( GsmMscTimerData *));
                }

                GsmLayer3BuildReleaseByNetworkMsg( 
                    node,
                    &releaseMsg,
                    GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
                    GSM_CAUSE_VALUE_NORMAL_CALL_CLEARING );

                GsmLayer3SendPacketViaGsmAInterface( 
                    node,
                    //callInfo->termMs.bsNodeAddress,
                    destBsNodeAddress,
                    node->nodeId,
                    //callInfo->termMs.sccpConnectionId,
                    destSccpConnectionId,
                    routingLabel,
                    GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                    (unsigned char*)releaseMsg,
                    sizeof(GsmReleaseByNetworkMsg));
                MEM_free(releaseMsg);
            } // End of T305 //
            break;

            case MSG_NETWORK_GsmReleaseTimer_T308:
            {
                GsmMscCallState* timerMs;

                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MSC[%d]: T308 Expired %d %d, origSCCP %d, "
                        "IMSI ",
                        node->nodeId,
                        callInfo->originMs.numT308Expirations,
                        callInfo->termMs.numT308Expirations,
                        callInfo->originMs.sccpConnectionId );
                    printGsmImsi( callInfo->originMs.imsi,10 );
                    printf( "\n" );
                }

                newMscTimerData.callInfo = callInfo;

                if (mscTimerData->isSetForOriginMs == TRUE)
                {
                    timerMs = &(callInfo->originMs);
                    newMscTimerData.isSetForOriginMs = TRUE;
                }
                else
                {
                    timerMs = &(callInfo->termMs);
                    newMscTimerData.isSetForOriginMs = FALSE;
                }

                timerMs->numT308Expirations++;

                if (timerMs->numT308Expirations >= 2)
                {
                    if (DEBUG_GSM_LAYER3_TIMERS)
                    {
                        printf("GSM_MSC[%d]: Releasing CallInfo\n",
                            node->nodeId );
                    }

                    // Release Call Reference
                    callInfo->inUse = FALSE;
                    memset(callInfo, 0, sizeof(GsmMscCallInfo));
                }
                else
                {
                    GsmReleaseByNetworkMsg* releaseMsg;


                    if (DEBUG_GSM_LAYER3_3)
                    {
                        printf("GSM_MSC[%d]: Sending RELEASE\n",
                            node->nodeId );
                    }
                    // Re-Send Release, restart T308
                    GsmLayer3BuildReleaseByNetworkMsg(
                        node,
                        &releaseMsg,
                        GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
                        GSM_CAUSE_VALUE_NORMAL_CALL_CLEARING );

                    GsmLayer3SendPacketViaGsmAInterface(
                        node,
                        timerMs->bsNodeAddress,
                        node->nodeId,
                        timerMs->sccpConnectionId,
                        routingLabel,
                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                        (unsigned char*)releaseMsg,
                        sizeof( GsmReleaseByNetworkMsg));
                    MEM_free( releaseMsg );

                    if (DEBUG_GSM_LAYER3_TIMERS)
                    {
                        printf("GSM_MSC[%d]: Setting T308\n",
                            node->nodeId);
                    }

                    timerMs->timerT308Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmReleaseTimer_T308,
                        DefaultGsmReleaseTimer_T308Time,
                        &newMscTimerData,
                        sizeof( GsmMscTimerData *));
                }
            } // End of T308 //
            break;

            case MSG_NETWORK_GsmCallProceedingTimer_T310:
            {
                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MSC[%d]: T310 Expired\n", node->nodeId);
                    printf( "GSM_MSC[%d]: Clear the Call\n",
                        node->nodeId);
                }
                // Clear the call:
            } // End of T310 //
            break;

            case MSG_NETWORK_GsmConnectTimer_T313:
            {
                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MSC[%d]: T313 Expired\n",node->nodeId);
                    printf("GSM_MSC[%d]: Clear the Call\n",
                        node->nodeId);
                }
                // Clear the call:
            } // End of T313 //
            break;

            case MSG_NETWORK_GsmHandoverTimer_T3103:
            {
                unsigned int sccpConnectionId;
                NodeAddress bsNodeAddress;
                GsmChannelReleaseMsg* channelRelMsg;
                callInfo = mscTimerData->callInfo;

                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MSC[%d]: T3103 Expired, origSCCPId %d\n",
                        node->nodeId,
                        callInfo->originMs.sccpConnectionId);
                }

                // Release Channels ???
                GsmLayer3BuildChannelReleaseMsg(node, &channelRelMsg);

                if (callInfo->originMs.isBsTryingHo == TRUE)
                {
                    sccpConnectionId = 
                        callInfo->originMs.hoSccpConnectionId;
                    bsNodeAddress =
                        callInfo->originMs.hoTargetBsNodeAddress;
                }
                else if (callInfo->termMs.isBsTryingHo == TRUE)
                {
                    sccpConnectionId = callInfo->termMs.hoSccpConnectionId;
                    bsNodeAddress = callInfo->termMs.hoTargetBsNodeAddress;
                }

                GsmLayer3SendPacketViaGsmAInterface(
                    node,
                    callInfo->termMs.bsNodeAddress,
                    node->nodeId,
                    callInfo->termMs.sccpConnectionId,
                    routingLabel,
                    GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                    (unsigned char*)channelRelMsg,
                    sizeof(GsmChannelReleaseMsg));
                MEM_free(channelRelMsg);

                // Call is dropped ???
                callInfo->inUse = FALSE;
                memset(callInfo, 0, sizeof(GsmMscCallInfo));
            } // End of T3103 //
            break;

            case MSG_NETWORK_GsmPagingTimer_T3113:
            {
                GsmReleaseByNetworkMsg* releaseMsg;
                if (DEBUG_GSM_LAYER3_TIMERS)
                {
                    printf("GSM_MSC[%d]: T3113 Expired(%d) origSCCP %d, \n",
                        node->nodeId,
                        callInfo->numPagingAttempts,
                        callInfo->originMs.sccpConnectionId);
                }

                // Network may repeat paging request
                // If 'x' paging requests have been sent:
                // End the call

                if (callInfo->numPagingAttempts < 3)
                {
                    // Send to BS of Call Terminating MS
                    GsmLayer3SendPacketViaGsmAInterface(
                        node,
                        callInfo->termMs.bsNodeAddress,
                        node->nodeId,
                        -1,
                        // Should be in ConnectionLess mode
                        routingLabel,
                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                        (unsigned char*)callInfo->pagingMsg,
                        sizeof(GsmPagingMsg));

                    callInfo->numPagingAttempts++;

                    callInfo->timerT3113Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmPagingTimer_T3113,
                        DefaultGsmPagingTimer_T3113Time,
                        &callInfo,
                        sizeof(GsmMscCallInfo *));
                }
                else
                {
                    if (DEBUG_GSM_LAYER3_TIMERS)
                    {
                        printf("%d Expirations (>=3), Releasing CallInfo\n",
                            callInfo->numPagingAttempts );
                    }

                    GsmLayer3BuildReleaseByNetworkMsg(
                        node,
                        &releaseMsg,
                        GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
                        GSM_CAUSE_VALUE_USER_NOT_RESPONDING);

                    GsmLayer3SendPacketViaGsmAInterface(
                        node,
                        callInfo->originMs.bsNodeAddress,
                        node->nodeId,
                        callInfo->originMs.sccpConnectionId,
                        routingLabel,
                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                        (unsigned char*)releaseMsg,
                        sizeof(GsmReleaseByNetworkMsg));
                    MEM_free(releaseMsg);

                    MEM_free(callInfo->pagingMsg);
                }
            } // End of T3113 //
            break;

            case MSG_NETWORK_GsmCmServiceAcceptTimer_UDT1:
            {
                GsmLayer3MscInitiateCallClearing(
                    node,
                    callInfo,
                    GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
                    GSM_CAUSE_VALUE_USER_NOT_RESPONDING,
                    GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
                    GSM_CAUSE_VALUE_RECOVERY_ON_TIMER_EXPIRY);

                callInfo->originMs.timerUDT1Msg = NULL;
            }
            break;
            default:
            {
                assert(FALSE);
            }
        }; // End of MSC eventType switch //
    }

    MESSAGE_Free(node,msg);
} // End of GsmLayer3Layer //


void GsmLayer3Finalize(
    Node* node)
{
    char                buf[MAX_STRING_LENGTH];

    GsmLayer3Data*      nwGsmData;
    GsmLayer3MsInfo*    msInfo;
    GsmLayer3BsInfo*    bsInfo;
    GsmLayer3MscInfo*   mscInfo;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;


    if (nwGsmData->collectStatistics == FALSE ){
        return;
    }

    if (nwGsmData->nodeType == GSM_MS ){
        msInfo = nwGsmData->msInfo;

        sprintf( buf,
            "Traffic Packets Sent = %d",
            msInfo->stats.pktsSentTraffic );
        IO_PrintStat( node,"Network","GSM_MS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Traffic Packets Received = %d",
            msInfo->stats.pktsRcvdTraffic );
        IO_PrintStat( node,"Network","GSM_MS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Channel Request Sent = %d",
            msInfo->stats.channelRequestSent );
        IO_PrintStat( node,"Network","GSM_MS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Channel Request Attempts Failed = %d",
            msInfo->stats.numChannelRequestAttemptsFailed );

        IO_PrintStat( node,"Network","GSM_MS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Channel Assignments Received = %d",
            msInfo->stats.immediateAssignmentRcvd );
        IO_PrintStat( node,"Network","GSM_MS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Channel Release Received = %d",
            msInfo->stats.channelReleaseRcvd );
        IO_PrintStat( node,"Network","GSM_MS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Location Update Request Sent = %d",
            msInfo->stats.locationUpdateRequestSent );
        IO_PrintStat( node,"Network","GSM_MS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Location Update Accept Received = %d",
            msInfo->stats.locationUpdateAcceptRcvd );
        IO_PrintStat( node,"Network","GSM_MS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Calls Initiated = %d",
            msInfo->stats.numCallsInitiated );
        IO_PrintStat( node,"Network","GSM_MS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,"Calls Received = %d",msInfo->stats.numCallsReceived );
        IO_PrintStat( node,"Network","GSM_MS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Calls Connected = %d",
            msInfo->stats.numCallsConnected );
        IO_PrintStat( node,"Network","GSM_MS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Calls Completed = %d",
            msInfo->stats.numCallsCompleted );
        IO_PrintStat( node,"Network","GSM_MS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Handovers Performed = %d",
            msInfo->stats.riHandoverCommandRcvd );
        IO_PrintStat( node,"Network","GSM_MS",ANY_DEST,node->nodeId,buf );


        // Before enabling any statstic
        // ensure that they are collected.

        if (GSM_ADDITIONAL_STATS ){
            // RR Stats

            sprintf( buf,
                "Immediate Assignment Received = %d",
                msInfo->stats.immediateAssignmentRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Paging Request Type1 Received = %d",
                msInfo->stats.pagingRequestType1Rcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Paging Response Sent = %d",
                msInfo->stats.pagingResponseSent );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Radio Interface Handover Received = %d",
                msInfo->stats.riHandoverCommandRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );


            // MM Stats

            sprintf( buf,
                "CM Service Request Sent = %d",
                msInfo->stats.cmServiceRequestSent );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "CM Service Accept Received = %d",
                msInfo->stats.cmServiceAcceptRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            // CC Stats

            sprintf( buf,"Setup Sent = %d",msInfo->stats.setupSent );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,"Setup Received = %d",msInfo->stats.setupRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,"Alerting Sent = %d",msInfo->stats.alertingSent );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Alerting Received = %d",
                msInfo->stats.alertingRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Call Proceeding Received = %d",
                msInfo->stats.callProceedingRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,"Connect Sent = %d",msInfo->stats.connectSent );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,"Connect Received = %d",msInfo->stats.connectRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Connect Acknowledge Sent = %d",
                msInfo->stats.connectAckSent );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Connect Acknowledge Received = %d",
                msInfo->stats.connectAckRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,"Release Received = %d",msInfo->stats.releaseRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Release Complete Sent = %d",
                msInfo->stats.releaseCompleteSent );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,"Release Sent = %d",msInfo->stats.releaseSent );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Release Complete Received = %d",
                msInfo->stats.releaseCompleteRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Disconnect Sent = %d",
                msInfo->stats.disconnectSent );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Disconnect Received = %d",
                msInfo->stats.disconnectRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_MS",
                ANY_DEST,
                node->nodeId,
                buf );
        } // End of GSM_ADDITIONAL_STATS //
    } // GSM_MS //
    else if (nwGsmData->nodeType == GSM_BS ){
        bsInfo = nwGsmData->bsInfo;

        sprintf( buf,
            "Traffic packets (On Air) Sent = %d",
            bsInfo->stats.pktsSentOnAirTraffic );
        IO_PrintStat( node,"Network","GSM_BS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Traffic packets (On Air) Received = %d",
            bsInfo->stats.pktsRcvdOnAirTraffic );
        IO_PrintStat( node,"Network","GSM_BS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Channel Requests Received = %d",
            bsInfo->stats.channelRequestRcvd );
        IO_PrintStat( node,"Network","GSM_BS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Channel Assignment Attempts Failed = %d",
            bsInfo->stats.channelAssignmentAttemptsFailed );
        IO_PrintStat( node,"Network","GSM_BS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Channels Assigned = %d",
            bsInfo->stats.numChannelsAssigned );
        IO_PrintStat( node,"Network","GSM_BS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Channels Released = %d",
            bsInfo->stats.numChannelsReleased );
        IO_PrintStat( node,"Network","GSM_BS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Channels Not Seized (T3101 Expirations) = %d",
            bsInfo->stats.numT3101Expirations );
        IO_PrintStat( node,"Network","GSM_BS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Paging Request Sent = %d",
            bsInfo->stats.pagingRequestSent );
        IO_PrintStat( node,"Network","GSM_BS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Location Update Received = %d",
            bsInfo->stats.locationUpdateRequestRcvd );
        IO_PrintStat( node,"Network","GSM_BS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Measurement Report Received = %d",
            bsInfo->stats.measurementReportRcvd );
        IO_PrintStat( node,"Network","GSM_BS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Handovers Completed (Incoming MS) = %d",
            bsInfo->stats.handoverAccessRcvd );
        IO_PrintStat( node,"Network","GSM_BS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Handovers Attempted (Outgoing MS) = %d",
            bsInfo->stats.handoverRequiredSent );
        IO_PrintStat( node,"Network","GSM_BS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Handovers Completed (Outgoing MS) = %d",
            bsInfo->stats.clearCommandRcvd );
        IO_PrintStat( node,"Network","GSM_BS",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Handovers Failed (Outgoing MS) = %d",
            bsInfo->stats.handoverRequiredRejectRcvd );
        IO_PrintStat( node,"Network","GSM_BS",ANY_DEST,node->nodeId,buf );

        // Before enabling any statstic
        // ensure that they are collected.

        if (GSM_ADDITIONAL_STATS ){
            sprintf( buf,
                "SCCP Connections Assigned = %d",
                bsInfo->stats.numSccpConnAssigned );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "SCCP Connections Released = %d",
                bsInfo->stats.numSccpConnReleased );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Channel Release Sent = %d",
                bsInfo->stats.channelReleaseSent );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Immediate Assignment Sent = %d",
                bsInfo->stats.immediateAssignmentSent );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Paging Response Received = %d",
                bsInfo->stats.pagingResponseRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,"Paging Received = %d",bsInfo->stats.pagingRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Location Update Received = %d",
                bsInfo->stats.locationUpdateRequestRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Radio Interface Handover Complete Received = %d",
                bsInfo->stats.riHandoverCompleteRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Radio Interface Handover Command Sent = %d",
                bsInfo->stats.riHandoverCommandSent );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Handover Access Received = %d",
                bsInfo->stats.handoverAccessRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Handover Detect Sent = %d",
                bsInfo->stats.handoverDetectSent );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Handover Complete Sent = %d",
                bsInfo->stats.handoverCompleteSent );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Handover Request Received = %d",
                bsInfo->stats.handoverRequestRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Handover Request Acknowledgement Received = %d",
                bsInfo->stats.handoverRequestAckSent );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Handover Command Received = %d",
                bsInfo->stats.handoverCommandRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Clear Complete Sent = %d",
                bsInfo->stats.clearCompleteSent );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );

            sprintf( buf,
                "Complete Layer3 Information Sent = %d",
                bsInfo->stats.completeLayer3InfoSent );
            IO_PrintStat( node,
                "Network",
                "GSM_BS",
                ANY_DEST,
                node->nodeId,
                buf );
        } // End of GSM_ADDITIONAL_STATS //
    } // GSM_BS //
    else if (nwGsmData->nodeType == GSM_MSC ){
        mscInfo = nwGsmData->mscInfo;


        sprintf( buf,
            "Location Update Request Received = %d",
            mscInfo->stats.locationUpdateRequestRcvd );
        IO_PrintStat( node,"Network","GSM_MSC",ANY_DEST,node->nodeId,buf );

        sprintf( buf,"Calls Requested = %d",mscInfo->stats.numCallRequests );
        IO_PrintStat( node,"Network","GSM_MSC",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Calls Connected = %d",
            mscInfo->stats.numCallRequestsConnected );
        IO_PrintStat( node,"Network","GSM_MSC",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Calls Completed = %d",
            mscInfo->stats.numCallsCompleted );
        IO_PrintStat( node,"Network","GSM_MSC",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Handover Required Received = %d",
            mscInfo->stats.handoverRequiredRcvd );
        IO_PrintStat( node,"Network","GSM_MSC",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Handovers Completed = %d",
            mscInfo->stats.numHandovers );
        IO_PrintStat( node,"Network","GSM_MSC",ANY_DEST,node->nodeId,buf );

        sprintf( buf,
            "Handovers Failed = %d",
            mscInfo->stats.handoverFailureRcvd );
        IO_PrintStat( node,"Network","GSM_MSC",ANY_DEST,node->nodeId,buf );


        sprintf( buf,
            "Traffic Packets Transferred = %d",
            mscInfo->stats.numTrafficPkts );
        IO_PrintStat( node,"Network","GSM_MSC",ANY_DEST,node->nodeId,buf );

        // Before enabling any statstic
        // ensure that they are collected.
        if (GSM_ADDITIONAL_STATS ){
            sprintf( buf,
                "Paging Response Received = %d",
                mscInfo->stats.pagingResponseRcvd );
            IO_PrintStat( node,
                "Network",
                "GSM_MSC",
                ANY_DEST,
                node->nodeId,
                buf );
        }
    } // GSM_MSC //
} // End of GsmLayer3Finalize //


/* NAME:    GsmLayer3DequeuePacket
 * PURPOSE: Dequeue a packet from the network layer
 *          Called by the GSM MAC layer when (begining of a TS)
 *          it is ready to send a message.
 *
 */
void GsmLayer3DequeuePacket(
    Node* node,
    short   slotToCheck,
    short   channelToCheck,
    Message** msg,
    uddtSlotNumber* slotNumber,
    uddtChannelIndex* channelIndex,
    GsmChannelType_e* channelType)
{
    MessageListItem*                        dequeuedItem;
    GsmLayer3Data*  nwGsmData   = (
            GsmLayer3Data* )node->networkData.gsmLayer3Var;

    if (nwGsmData->nodeType == GSM_MS ){
        GsmLayer3MsInfo*                            msInfo  =
            nwGsmData->msInfo;


        if (msInfo->numMsgsToSendToMac[slotToCheck] > 0 ){
            // Dequeue the first packet in the queue/list

            *msg = msInfo->msgListToMac[slotToCheck]->msg;
            *slotNumber = msInfo->msgListToMac[slotToCheck]->slotNumber;
            *channelIndex = msInfo->msgListToMac[slotToCheck]->channelIndex;
            *channelType = msInfo->msgListToMac[slotToCheck]->channelType;

            dequeuedItem = msInfo->msgListToMac[slotToCheck];

            // If the only msg in queue
            if (msInfo->numMsgsToSendToMac[slotToCheck] == 1 ){
                msInfo->msgListToMac[slotToCheck] = NULL;
            }
            else // More than 1 msg in queue
            {
                msInfo->msgListToMac[slotToCheck] =
                msInfo->msgListToMac[slotToCheck]->nextListItem;
            }

            MEM_free( dequeuedItem );
            msInfo->numMsgsToSendToMac[slotToCheck]--;
        }
        else{
            *msg = NULL;
        }
    }
    else if (nwGsmData->nodeType == GSM_BS ){
        GsmLayer3BsInfo*                            bsInfo  =
            nwGsmData->bsInfo;
        int             chanLoc = channelToCheck - bsInfo->channelIndexStart;


        if (bsInfo->numMsgsToSendToMac[slotToCheck][chanLoc] > 0 ){
            // Dequeue the first packet in the queue/list

            *msg = bsInfo->msgListToMac[slotToCheck][chanLoc]->msg;
            *slotNumber = bsInfo->msgListToMac[slotToCheck] \
            [chanLoc]->slotNumber;
            *channelIndex = bsInfo->msgListToMac[slotToCheck] \
            [chanLoc]->channelIndex;
            *channelType = bsInfo->msgListToMac[slotToCheck] \
            [chanLoc]->channelType;

            dequeuedItem = bsInfo->msgListToMac[slotToCheck] \
            [chanLoc];

            // If the only msg in queue
            if (bsInfo->numMsgsToSendToMac[slotToCheck][chanLoc] == 1 ){
                bsInfo->msgListToMac[slotToCheck][chanLoc] = NULL;
            }
            else // More than 1 msg in queue
            {
                bsInfo->msgListToMac[slotToCheck][chanLoc] =
                bsInfo->msgListToMac[slotToCheck] \
                [chanLoc]->nextListItem;
            }

            MEM_free( dequeuedItem );
            bsInfo->numMsgsToSendToMac[slotToCheck][chanLoc]--;
        }
        else{
            *msg = NULL;
        }
    }
} // End of GsmLayer3DequeuePacket //


/* NAME:    GsmLayer3PeekAtNextPacket
 * PURPOSE: Peek at next packet in the network layer queue
 *          Called by the GSM MAC layer when (begining of a TS)
 *          it is ready to send a message.
 */
void GsmLayer3PeekAtNextPacket(
    Node* node,
    short   slotToPeek,
    short   channelToPeek,
    Message** msg,
    uddtSlotNumber* slotNumber,
    uddtChannelIndex* channelIndex,
    GsmChannelType_e* channelType)
{
    GsmLayer3Data*  nwGsmData;

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    if (nwGsmData->nodeType == GSM_MS ){
        GsmLayer3MsInfo*                            msInfo  =
            nwGsmData->msInfo;


        if (msInfo->numMsgsToSendToMac[slotToPeek] > 0 ){
            *msg = msInfo->msgListToMac[slotToPeek]->msg;
            *slotNumber = msInfo->msgListToMac[slotToPeek]->slotNumber;
            *channelIndex = msInfo->msgListToMac[slotToPeek]->channelIndex;
            *channelType = msInfo->msgListToMac[slotToPeek]->channelType;
        }
        else{
            *msg = NULL;
        }
    }
    else if (nwGsmData->nodeType == GSM_BS ){
        GsmLayer3BsInfo*                            bsInfo  =
            nwGsmData->bsInfo;
        int             chanLoc = channelToPeek - bsInfo->channelIndexStart;


        if (bsInfo->numMsgsToSendToMac[slotToPeek][chanLoc] > 0 ){
            *msg = bsInfo->msgListToMac[slotToPeek][chanLoc]->msg;
            *slotNumber =
            bsInfo->msgListToMac[slotToPeek][chanLoc]->slotNumber;
            *channelIndex =
            bsInfo->msgListToMac[slotToPeek][chanLoc]->channelIndex;
            *channelType =
            bsInfo->msgListToMac[slotToPeek][chanLoc]->channelType;
        }
        else{
            *msg = NULL;
        }
    }
} // End of GsmLayer3PeekAtNextPacket //


/* NAME:    GsmLayer3SendPacket
 * PURPOSE: Enqueue packet for sending to MAC layer
 *          for sending it over air.
 *          Currently the queue holds only one packet.
 *          Memory allocated at GSM Network layer initialization.
 *
 * COMMENT: Should there be priorities in the queue (when it is
 *          implemented)??? FIFO might be okay.
 */
void GsmLayer3SendPacket(
    Node* node,
    Message* msg,
    uddtSlotNumber   slotNumber,
    uddtChannelIndex   channelIndex,
    GsmChannelType_e   channelType)
{
    GsmLayer3Data*  nwGsmData;

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    if (nwGsmData->nodeType == GSM_MS ){
        GsmLayer3MsInfo*                            msInfo  =
            nwGsmData->msInfo;


        if (msInfo->numMsgsToSendToMac[slotNumber] == 0 ){
            msInfo->msgListToMac[slotNumber] = (
                MessageListItem * )MEM_malloc( sizeof( MessageListItem ) );

            msInfo->msgListToMac[slotNumber]->msg = msg;
            msInfo->msgListToMac[slotNumber]->slotNumber = slotNumber;
            msInfo->msgListToMac[slotNumber]->channelIndex = channelIndex;
            msInfo->msgListToMac[slotNumber]->channelType = channelType;
            msInfo->msgListToMac[slotNumber]->nextListItem = NULL;
        }
        else{
            MessageListItem*                                nextItem;
            MessageListItem*                                newItem;

            // Go to the end of the list
            nextItem = msInfo->msgListToMac[slotNumber];

            while (nextItem->nextListItem != NULL ){
                nextItem = nextItem->nextListItem;
            }

            newItem = (
                MessageListItem * )MEM_malloc( sizeof( MessageListItem ) );

            // set the new item as the last one
            nextItem->nextListItem = newItem;

            newItem->msg = msg;
            newItem->slotNumber = slotNumber;
            newItem->channelIndex = channelIndex;
            newItem->channelType = channelType;
            newItem->nextListItem = NULL;
        }

        msInfo->numMsgsToSendToMac[slotNumber]++;
    }
    else if (nwGsmData->nodeType == GSM_BS ){
        GsmLayer3BsInfo*                            bsInfo  =
            nwGsmData->bsInfo;
        int             chanLoc = channelIndex - bsInfo->channelIndexStart;


        if (bsInfo->numMsgsToSendToMac[slotNumber][chanLoc] == 0 ){
            bsInfo->msgListToMac[slotNumber][chanLoc] = (
                MessageListItem * )MEM_malloc( sizeof( MessageListItem ) );

            bsInfo->msgListToMac[slotNumber][chanLoc]->msg = msg;
            bsInfo->msgListToMac[slotNumber][chanLoc]->slotNumber =
            slotNumber;
            bsInfo->msgListToMac[slotNumber][chanLoc]->channelIndex =
            channelIndex;
            bsInfo->msgListToMac[slotNumber][chanLoc]->channelType =
            channelType;
            bsInfo->msgListToMac[slotNumber][chanLoc]->nextListItem = NULL;
        }
        else{
            MessageListItem*                                nextItem;
            MessageListItem*                                newItem;

            // Go to the end of the list
            nextItem = bsInfo->msgListToMac[slotNumber][chanLoc];

            while (nextItem->nextListItem != NULL ){
                nextItem = nextItem->nextListItem;
            }

            newItem = (
                MessageListItem * )MEM_malloc( sizeof( MessageListItem ) );

            // set the new item as the last one
            nextItem->nextListItem = newItem;

            newItem->msg = msg;
            newItem->slotNumber = slotNumber;
            newItem->channelIndex = channelIndex;
            newItem->channelType = channelType;
            newItem->nextListItem = NULL;
        }
        bsInfo->numMsgsToSendToMac[slotNumber][chanLoc]++;
    }
} // End of GsmLayer3SendPacket //


// Callback function to handle SACCH messsages on BS
void gsmBsProcessMeasReport(
    Node* node,
    uddtSlotNumber   slotNumber,
    uddtChannelIndex   channelIndex,
    GsmChannelMeasurement* chMeas)
{
    int                     i;
    int                     j;
    int                     cellIndex;
    int                     numAvgBcchValues;

    GsmChannelDescription*  channelDesc;
    GsmSlotUsageInfo*       slotInfo;

    GsmLayer3Data*          nwGsmData   = (
            GsmLayer3Data* )node->networkData.gsmLayer3Var;

    GsmLayer3BsInfo*        bsInfo      = nwGsmData->bsInfo;


    if (GsmLayer3BsLookupMsBySlotAndChannel( bsInfo,
             slotNumber,
             channelIndex,
             &channelDesc ) == FALSE ){
        return;
    }

    if (channelDesc->slotUsageInfo[slotNumber].isMeasReportReceived ==
           FALSE ){
        return;
    }

    // Uplink measurements
    slotInfo = &( channelDesc->slotUsageInfo[slotNumber] );

    slotInfo->rxLevUl[slotInfo->measUlSampleNum] = GsmMapRxLev(
        chMeas->rxLevel_dBm / chMeas->numRecorded );
    slotInfo->rxQualUl[slotInfo->measUlSampleNum] = GsmMapRxQual(
        chMeas->ber / chMeas->numRecorded );

    slotInfo->measUlSampleNum = ( slotInfo->measUlSampleNum + 1 ) %
                                    GSM_NUM_MEAS_SAMPLES;

    slotInfo->lastSACCHupdate = node->getNodeTime();


    if (DEBUG_GSM_LAYER3_1 ){
        printf( "BS [%d], fr %d CH %d, TS %d UL %d:\n",
            node->nodeId,
            bsInfo->frameNumber,
            channelIndex,
            slotNumber,
            slotInfo->measUlSampleNum );

        for (i = 0; i < GSM_NUM_MEAS_SAMPLES; i++ ){
            printf( " %d",slotInfo->rxLevUl[i] );
        }

        printf( "\nBS [%d] CH %d, TS %d DL %d:\n",
            node->nodeId,
            channelIndex,
            slotNumber,
            slotInfo->measDlSampleNum );

        for (i = 0; i < GSM_NUM_MEAS_SAMPLES; i++ ){
            printf( " %d",slotInfo->rxLevDl[i] );
        }

        printf( "\nNeigh.Cells %d:\n",bsInfo->numNeighBcchChannels );

        for (i = 0; i < bsInfo->numNeighBcchChannels; i++ ){
            printf( "[%d] CH %d, samp %d, RxLev ",
                i,
                bsInfo->neighBcchChannels[i],
                slotInfo->neighCellSampleNum[i] );
            for (j = 0; j < GSM_NUM_MEAS_SAMPLES; j++ ){
                printf( " %d",slotInfo->rxLevBcchDl[i][j] );
            }
            printf( "\n" );
        }

        printf( "\n" );
    }

    // SACCH Multiframe (as judged by the end of the
    // assigned slot's reporting period). (103, 12, 25,...90)
    if (GsmSacchMsgFrame[slotNumber][3] == (int)bsInfo->frameNumber %
           GSM_SACCH_MULTIFRAME_SIZE ){
        unsigned char   hoCause = 0;


        // Initialize location where the new avg values will be stored.
        slotInfo->avgRxLevUl[slotInfo->numAvgValues] = 0;
        slotInfo->avgRxQualUl[slotInfo->numAvgValues] = 0;
        slotInfo->avgRxLevDl[slotInfo->numAvgValues] = 0;
        slotInfo->avgRxQualDl[slotInfo->numAvgValues] = 0;

        for (cellIndex = 0;
                cellIndex < bsInfo->numNeighBcchChannels;
                cellIndex++ ){
            slotInfo->avgRxLevBcchDl[cellIndex]\
            [slotInfo->numAvgBcchValues[cellIndex]] = 0;
        }

        // Calculate average values using last GSM_HREQAVE measured values.
        for (i = 0; i < GSM_HREQAVE; i++ ){
            j = ( slotInfo->measDlSampleNum - i - 1 +
                    GSM_NUM_MEAS_SAMPLES ) % GSM_NUM_MEAS_SAMPLES;
            slotInfo->avgRxLevDl[slotInfo->numAvgValues] +=
            slotInfo->rxLevDl[j];
            slotInfo->avgRxQualDl[slotInfo->numAvgValues] +=
            slotInfo->rxQualDl[j];

            j = ( slotInfo->measUlSampleNum - i - 1 +
                    GSM_NUM_MEAS_SAMPLES ) % GSM_NUM_MEAS_SAMPLES;
            slotInfo->avgRxLevUl[slotInfo->numAvgValues] +=
            slotInfo->rxLevUl[j];
            slotInfo->avgRxQualUl[slotInfo->numAvgValues] +=
            slotInfo->rxQualUl[j];


            if (DEBUG_GSM_LAYER3_1 ){
                printf( "BS [%d] smb %d, avgval %d: DL %d, %d, UL %d, %d\n",
                    node->nodeId,
                    GsmMeasRepPeriod[slotNumber][1],
                    ( slotInfo->numAvgValues + 1 ) % GSM_HREQAVE,
                    slotInfo->avgRxLevDl[slotInfo->numAvgValues],
                    slotInfo->avgRxQualDl[slotInfo->numAvgValues],
                    slotInfo->avgRxLevUl[slotInfo->numAvgValues],
                    slotInfo->avgRxQualUl[slotInfo->numAvgValues] );
            }

            for (cellIndex = 0;
                    cellIndex < bsInfo->numNeighBcchChannels;
                    cellIndex++ ){
                numAvgBcchValues = slotInfo->numAvgBcchValues[cellIndex];

                j = ( slotInfo->neighCellSampleNum[cellIndex] - i - 1 +
                        GSM_NUM_MEAS_SAMPLES ) % GSM_NUM_MEAS_SAMPLES;

                slotInfo->avgRxLevBcchDl[cellIndex][numAvgBcchValues] +=
                slotInfo->rxLevBcchDl[cellIndex][j];
            }

            if (DEBUG_GSM_LAYER3_1 ){
                printf( "Neigh.CH %d, %d avg BCCH-DL values, new val %d\n",
                    bsInfo->neighBcchChannels[cellIndex],
                    slotInfo->numAvgBcchValues[cellIndex],
                    slotInfo->avgRxLevBcchDl[cellIndex][numAvgBcchValues] );
            }
        } // End of avg.values calc //

        if (DEBUG_GSM_LAYER3_1 ){
            printf( "------------------------------\n" );
            printf( "BS [%d] %d avg values\nDL\t\tUL \n",
                node->nodeId,
                ( slotInfo->numAvgValues + 1 ) % GSM_HREQAVE );
            for (i = 0; i < GSM_HREQAVE; i++ ){
                printf( "%3d %3d\t%3d %3d\n",
                    slotInfo->avgRxLevDl[i],
                    slotInfo->avgRxQualDl[i],
                    slotInfo->avgRxLevUl[i],
                    slotInfo->avgRxQualUl[i] );
            }

            printf( "Neigh.BCCH DL avg values\nCH RxLev-DL\n" );
            for (i = 0; i < bsInfo->numNeighBcchChannels; i++ ){
                printf( "%2d ",bsInfo->neighBcchChannels[i] );
                for (j = 0; j < GSM_HREQAVE; j++ ){
                    printf( "%2d ",slotInfo->avgRxLevBcchDl[i][j] );
                }
                printf( "\n" );
            }
            printf( "------------------------------\n" );
        }

        slotInfo->numAvgValues = ( slotInfo->numAvgValues + 1 ) %
                                     GSM_HREQAVE;

        for (cellIndex = 0;
                cellIndex < bsInfo->numNeighBcchChannels;
                cellIndex++ ){
            // Should happen only for BCCH channels whose  data was updated.
            slotInfo->numAvgBcchValues[cellIndex] = (
                slotInfo->numAvgBcchValues[cellIndex] + 1 ) % GSM_HREQAVE;

            if (slotInfo->numAvgBcchValues[cellIndex] >= GSM_HREQAVE - 1 ){
                slotInfo->atLeastHreqAvgBcchValues[cellIndex] = TRUE;
            }
        }

        // To avoid HO check until atleast GSM_HREQAVE averages have
        // been computed.
        if (slotInfo->numAvgValues >= GSM_HREQAVE - 1 ){
            // Happens after the first GSM_HREQAVE avgs have been computed
            // on start of a call.  Needs to be reset on end of call or
            // channel release.
            slotInfo->atLeastHreqAvgValues = TRUE;
        }



        // Intra-MSC External Handovers only.
        if (slotInfo->isHandoverInProgress == FALSE &&
               slotInfo->atLeastHreqAvgValues == TRUE &&
               isHandoverNeeded( node,
                   bsInfo,
                   slotInfo,
                   &hoCause ) == TRUE ){
            if (DEBUG_GSM_LAYER3_1 ){
                printf( "GSM_BS[%d] HO needed, cause %d, TS %d, CH[%d]\n",
                    node->nodeId,
                    hoCause,
                    slotNumber,
                    channelIndex );
            }

            startHandover( node,bsInfo,slotInfo,hoCause );
        }
    } // SACCH Multiframe //
} // End of gsmBsProcessMeasReport //


/* NAME:    GsmLayer3ReceivePacketFromMac
 * PURPOSE: Process packets sent by GSM MAC (via air/radio)
 */
void GsmLayer3ReceivePacketFromMac(
    Node* node,
    Message* msg,
    uddtSlotNumber   recvSlotNumber,
    uddtFrameNumber     recvFrameNumber,
    uddtChannelIndex   recvChannelIndex,
    int     recvInterfaceIndex)
{
    unsigned char*  msgContent;
    char            clockStr[MAX_STRING_LENGTH];
    char            errorString[MAX_STRING_LENGTH];

    GsmLayer3Data*  nwGsmData;
    Message*        newMsg;

    msgContent = ( unsigned char* )MESSAGE_ReturnPacket( msg );
    ctoa(node->getNodeTime(), clockStr);

    if (DEBUG_GSM_LAYER3_0 ){
        printf( "\nNode[%d] GsmLayer3ReceivePacketFromMac:"
                    "\n\tReceived on slot %d, fr %d, channel %d",
            node->nodeId,
            recvSlotNumber,
            recvFrameNumber,
            recvChannelIndex );
    }

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    if (nwGsmData->nodeType == GSM_MS ){
        GsmLayer3MsInfo*                            msInfo;


        msInfo = nwGsmData->msInfo;

        if (msInfo->isDedicatedChannelAssigned == TRUE &&
               msInfo->isAssignedChannelForTraffic ==
               TRUE &&
               recvSlotNumber ==
               msInfo->assignedSlotNumber &&
               recvChannelIndex == msInfo->assignedChannelIndex - 1 ){
            // Traffic channel msg, check stealing flag.
            if (msgContent[0] == FALSE ){
                Message*                            trafficMessage;


                msInfo->stats.pktsRcvdTraffic++;

                if (msInfo->isCallOriginator == FALSE ){
                    // Send traffic packets to calling MS
                    GsmLayer3BuildVirtualTrafficPacket( node,
                        &trafficMessage );

                    GsmLayer3SendPacket( node,
                        trafficMessage,
                        msInfo->assignedSlotNumber,
                        msInfo->assignedChannelIndex,
                        GSM_CHANNEL_TCH );

                    msInfo->stats.pktsSentTraffic++;
                }

                //return;
            }
            else{
                if (DEBUG_GSM_LAYER3_1 ){
                    printf( "GSM_MS[%d] Received a sig msg on TCH\n",
                        node->nodeId );
                }

                switch ( msgContent[1] ){
                    case GSM_PD_RR:
                        {
                            gsmMsProcessRrMessagesFromMac( node,
                                msgContent + 1,
                                recvSlotNumber,
                                recvChannelIndex );
                        }
                        break;

                    case GSM_PD_CC:
                        {
                            gsmMsProcessCcMessagesFromMac( node,
                                msgContent + 1,
                                recvSlotNumber,
                                recvChannelIndex );
                        }
                        break;

                    default:
                        {
                        if (DEBUG_GSM_LAYER3_3 && 
                                  // Bug # 3436 fix - start
                                  msgContent[0] != GSM_DUMMY_BURST_OCTET){
                                 // Bug # 3436 fix - end
                                sprintf( errorString,
                                    "GSMLayer3 MS[%d]: "
                                        "Unknown PD 0x%x in msg on TS %d,"
                                        " CH %d",
                                    node->nodeId,
                                    msgContent[1],
                                    recvSlotNumber,
                                    recvChannelIndex );
                                ERROR_ReportWarning( errorString );
                            }
                        }
                }
            }

            return;
        }

        switch ( msgContent[0] ){
                // Layer 3 sub-layer processing
            case(GSM_PD_RR):
                {
                    gsmMsProcessRrMessagesFromMac( node,
                        msgContent,
                        recvSlotNumber,
                        recvChannelIndex );
                } // End of RR sublayer //
                break;

            case GSM_PD_MM:
                {
                    if (msInfo->isDedicatedChannelAssigned == TRUE ){
                        gsmMsProcessMmMessagesFromMac( node,
                            msgContent,
                            recvSlotNumber,
                            recvChannelIndex );
                    }
                } // End of MM sublayer //
                break;

            case GSM_PD_CC:
                {
                    if (msInfo->isDedicatedChannelAssigned == TRUE ){
                        gsmMsProcessCcMessagesFromMac( node,
                            msgContent,
                            recvSlotNumber,
                            recvChannelIndex );
                    }
                } // End of CC sublayer //
                break;

            default:
                {
                    if (DEBUG_GSM_LAYER3_0 &&
                           msgContent[0] != GSM_DUMMY_BURST_OCTET 
                           // Bug # 3436 fix - start
                           &&
                           msInfo->isDedicatedChannelAssigned == TRUE ){
                           // Bug # 3436 fix - end
                        sprintf( errorString,
                            "GSMLayer3 MS[%d]: "
                                "Unknown PD 0x%x in msg on TS %d, CH %d",
                            node->nodeId,
                            msgContent[0],
                            recvSlotNumber,
                            recvChannelIndex );
                        ERROR_ReportWarning( errorString );
                    }
                }
                break;
        } // End of sublayer switching //
    } // End of GSM_MS //
    else if (nwGsmData->nodeType == GSM_BS ){
        BOOL                    wasFound;
        GsmLayer3BsInfo*        bsInfo;
        GsmChannelDescription*  channelDesc;


        bsInfo = nwGsmData->bsInfo;
        bsInfo->frameNumber = recvFrameNumber;

        if (DEBUG_GSM_LAYER3_1 ){
            printf( "GSM_BS[%d] Rcvd msg on 'Um': TS %d, fr %d, CH %d\n",
                node->nodeId,
                recvSlotNumber,
                recvFrameNumber,
                recvChannelIndex );
        }

        if (recvChannelIndex == bsInfo->controlChannelIndex + 1 &&
               recvSlotNumber ==
               0 &&
               (
               msgContent[0] ==
            GSM_EST_CAUSE_NORMAL_CALL ||
            msgContent[0] ==
            GSM_EST_CAUSE_ANSWER_TO_PAGING_TCH_CHANNEL ||
            msgContent[0] == GSM_EST_CAUSE_LOCATION_UPDATING ) ){
            clocktype   timingAdvance;

            timingAdvance = *( clocktype * )MESSAGE_ReturnInfo( msg );

            bsInfo->stats.channelRequestRcvd++;

            if (GsmLayer3BuildImmediateAssignmentMsg( node,
                     &newMsg,
                     ( GsmChannelRequestMsg * )msgContent,
                     timingAdvance ) == FALSE ){
                // Send a failure message ???

                if (DEBUG_GSM_LAYER3_3 ){
                    sprintf( errorString,
                        "GSM_BS[%d] ERROR: "
                            "Immediate Assignment Failure\n",
                        node->nodeId );
                    ERROR_ReportWarning( errorString );
                }
                MESSAGE_Free(node, newMsg);
                return;
            }

            GsmLayer3SendPacket( node,
                newMsg,
                0,
                //GSM_INVALID_SLOT_NUMBER,
                bsInfo->channelIndexStart,
                //GSM_INVALID_CHANNEL_INDEX,  // MAC decides
                GSM_CHANNEL_PAGCH );
        } // End of processing channel requests //
        else{
            GsmSlotUsageInfo*           slotInfo;
            GsmSccpRoutingLabel         routingLabel;

            int                         currentConnId; // SCCP Connection Id
            int                         newSccpConnectionId;

            GsmCompleteLayer3InfoMsg*   completeL3Msg;

            memset( &routingLabel,0,sizeof( GsmSccpRoutingLabel ) );
            // Lookup MS based on channel & TS the message
            // was received on.

            wasFound = GsmLayer3BsLookupMsBySlotAndChannel( bsInfo,
                           recvSlotNumber,
                           recvChannelIndex,
                           // UL channelIndex, change var.name
                           &channelDesc );

            if (wasFound == FALSE ){
                if (DEBUG_GSM_LAYER3_3 ){
                    sprintf( errorString,
                        "GSM_BS[%d] ERROR: "
                            "Unrelated msg 0x%x 0x%x rcvd TS %d, CH %d,"
                            " at %s\n",
                        node->nodeId,
                        msgContent[0],
                        msgContent[1],
                        recvSlotNumber,
                        recvChannelIndex,
                        clockStr );
                    ERROR_ReportWarning( errorString );
                }

                return;
            }

            slotInfo = &( channelDesc->slotUsageInfo[recvSlotNumber] );

            // Check if this channel/slot is used for traffic
            if (slotInfo->isUsedForTraffic == TRUE ){
                //slotInfo->count++;
                //printf("GSM_BS[%d]: TS %d, Ch %d, count %d\n",
                //    node->nodeId, slotInfo->slotNumber,
                //    slotInfo->ulChannelIndex, slotInfo->count);

                if (msgContent[0] != TRUE &&
                       MacGsmIsSacchMsgFrame( recvSlotNumber,
                           recvFrameNumber ) == TRUE ){
                    GsmMeasurementInfo  measInfo;


                    if (msgContent[0] == GSM_PD_RR &&
                           msgContent[1] ==
                           GSM_RR_MESSAGE_TYPE_MEASUREMENT_REPORT ){
                        int                     i,
                                                j;
                        GsmMeasurementReportMsg*
                        measReport;

                        measReport = ( GsmMeasurementReportMsg * )msgContent;

                        slotInfo->rxLevDl[slotInfo->measDlSampleNum] =
                        measReport->rxLevFullServingCell;
                        slotInfo->rxQualDl[slotInfo->measDlSampleNum] =
                        measReport->rxQualFullServingCell;

                        for (i = 0; i < measReport->numNeighCells; i++ ){
                            for (j = 0;
                                    j < bsInfo->numNeighBcchChannels;
                                    j++ ){
                                if (measReport->neighCellChannel[i] ==
                                       bsInfo->neighBcchChannels[j] ){
                                    int neighCellSampleNum  =
                                        slotInfo->neighCellSampleNum[j];


                                    slotInfo->rxLevBcchDl[j]\
                                    [neighCellSampleNum] =
                                    measReport->rxLevNeighCell[i];

                                    slotInfo->neighCellSampleNum[j] = (
                                        slotInfo->neighCellSampleNum[j] +
                                        1 ) % GSM_NUM_MEAS_SAMPLES;
                                    break;
                                }
                            }
                        }

                        slotInfo->measDlSampleNum = (
                            slotInfo->measDlSampleNum +
                            1 ) % GSM_NUM_MEAS_SAMPLES;

                        slotInfo->isMeasReportReceived = TRUE;

                        slotInfo->lastSACCHupdate = node->getNodeTime();

                        bsInfo->stats.measurementReportRcvd++;

                        measInfo.slotNumber = recvSlotNumber;
                        measInfo.channelIndex = recvChannelIndex;

                        measInfo.callBackFuncPtr = &gsmBsProcessMeasReport;

                        GsmLayer3SendInterLayerMsg( node,
                            bsInfo->interfaceIndex,
                            GSM_L3_MAC_MEASUREMENT_REPORT,
                            ( unsigned char* )&measInfo,
                            sizeof( GsmMeasurementInfo ) );
                    }
                    else{
                        // printf("GSM_BS[%d]: SACCH slot %d, fr %d, no MEAS\n",
                        //    node->nodeId, recvSlotNumber, recvFrameNumber);

                        slotInfo->isMeasReportReceived = FALSE;
                    }
                } // SACCH //
                else if (msgContent[0] == TRUE ) // Stealing flag
                {
                    if (slotInfo->isHandoverInProgress == TRUE ){
                        if (msgContent[1] == slotInfo->hoReference ){
                            unsigned char   msgType = GSM_HANDOVER_DETECT;


                            if (DEBUG_GSM_LAYER3_3 ){
                                printf(
                                    "GSM_BS[%d]: HOAccess hoRef %d"
                                        " TS %d CH %d, Sending HO_DETECT to"
                                        " MSC\n",
                                    node->nodeId,
                                    msgContent[1],
                                    recvSlotNumber,
                                    recvChannelIndex );
                            }

                            // Handover Target BS
                            slotInfo->isHandoverInProgress = FALSE;

                            // GSM_HANDOVER_DETECT has only one element
                            // message type.
                            GsmLayer3SendPacketViaGsmAInterface( node,
                                bsInfo->mscNodeAddress,
                                node->nodeId,
                                slotInfo->sccpConnectionId,
                                routingLabel,
                                GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                                ( unsigned char* )&msgType,
                                sizeof( unsigned char ) );

                            bsInfo->stats.handoverAccessRcvd++;
                            bsInfo->stats.handoverDetectSent++;

                            return;
                        }
                    }
                    // If a signalling message (FACCH)
                    switch ( msgContent[1] ){
                        case GSM_PD_RR:
                            {
                                GsmHandoverCompleteMsg* hoCompMsg;


                                if (
                                    msgContent[2] ==
                                    GSM_RR_MESSAGE_TYPE_RI_HANDOVER_COMPLETE ){
                                    if (DEBUG_GSM_LAYER3_3 ){
                                        printf(
                                            "GSM_BS[%d] RI_HANDOVER_COMPLETE\n",
                                            node->nodeId );
                                    }

                                    slotInfo->isHandoverInProgress = FALSE;

                                    GsmLayer3BuildHandoverCompleteMsg( node,
                                        &hoCompMsg );

                                    GsmLayer3SendPacketViaGsmAInterface(
                                        node,
                                        bsInfo->mscNodeAddress,
                                        node->nodeId,
                                        slotInfo->sccpConnectionId,
                                        routingLabel,
                                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                                        ( unsigned char* )hoCompMsg,
                                        sizeof( GsmHandoverCompleteMsg ) );

                                    // slotInfo->isHandoverInProgress;

                                    MEM_free(hoCompMsg);

                                    bsInfo->stats.riHandoverCompleteRcvd++;
                                    bsInfo->stats.handoverCompleteSent++;
                                }
                            }
                            break;

                        case GSM_PD_CC:
                            {
                                gsmBsProcessCcMessageFromMac( node,
                                    msgContent + 1,
                                    // skip 'stealing flag'
                                    channelDesc,
                                    recvSlotNumber,
                                    recvChannelIndex );
                            }
                            break;
                    };
                } // FACCH //
                else{
                    // TCH //
                    bsInfo->stats.pktsRcvdOnAirTraffic++;

                    // Traffic packet: relay it to the MSC
                    GsmLayer3AInterfaceSendTrafficMsg( node,
                        bsInfo->mscNodeAddress,
                        slotInfo->networkTrafficConnectionId,
                        GSM_TRAFFIC_MESSAGE_TYPE_TRAFFIC,
                        1,
                        // packet seq #
                        msgContent,
                        GSM_BURST_DATA_SIZE,
                        msg->packetSize);
                } // TCH //

                return;
            } // Processed messages received on the TCH/FACCH/SACCH //

            currentConnId = slotInfo->sccpConnectionId;

            // Layer 3 sublayer processing for non-TCH messages
            switch ( msgContent[0] ){
                case GSM_PD_RR:
                    {
                        if (DEBUG_GSM_LAYER3_3 ){
                            printf( "GSM_BS[%d] TS %d CH %d (%s) RR msg: ",
                                node->nodeId,
                                recvSlotNumber,
                                recvChannelIndex,
                                clockStr );
                        }

                        if (msgContent[1] ==
                               GSM_RR_MESSAGE_TYPE_PAGING_RESPONSE ){
                            GsmPagingResponseMsg*   pagingResp;


                            pagingResp = (
                                GsmPagingResponseMsg * )msgContent;

                            if (DEBUG_GSM_LAYER3_3 ){
                                printf( "PAGING_RESPONSE\n" );
                            }

                            slotInfo->isSeizedByMs = TRUE;

                            GsmLayer3BuildCompleteLayer3InfoMsg( node,
                                bsInfo,
                                msgContent,
                                msg->packetSize,
                                &completeL3Msg );

                            // Create a new SCCP connection id from the term-
                            // inating MS's BS to its controlling MSC.

                            if (GsmLayer3BsAssignSccpConnId( node,
                                     bsInfo,
                                     slotInfo,
                                     &newSccpConnectionId ) == TRUE ){
                                // Store information about this MS' call
                                memcpy( slotInfo->msImsi,
                                    pagingResp->imsi,
                                    GSM_MAX_IMSI_LENGTH );

                                if (DEBUG_GSM_LAYER3_2 ){
                                    printf(
                                        "GSM_BS[%d]"
                                            " GsmLayer3BsAssignSccpConnId"
                                            " new = %d, total = %d\n",
                                        node->nodeId,
                                        newSccpConnectionId,
                                        bsInfo->numSccpConnections );
                                }
                            }
                            else{
                                if (DEBUG_GSM_LAYER3_3 ){
                                    printf( "GSM_BS[%d] ERROR:"
                                                " SCCPId NOT ASSIGNED\n",
                                        node->nodeId );
                                }
                                MEM_free(completeL3Msg);
                                return;
                            }

                            GsmLayer3SendPacketViaGsmAInterface( node,
                                bsInfo->mscNodeAddress,
                                node->nodeId,
                                newSccpConnectionId,
                                routingLabel,
                                GSM_SCCP_MSG_TYPE_CONNECT_REQUEST,
                                ( unsigned char* )completeL3Msg,
                                sizeof(
                                GsmCompleteLayer3InfoMsg ) +
                                sizeof( GsmPagingResponseMsg ) );

                            MEM_free(completeL3Msg);

                            bsInfo->stats.pagingResponseRcvd++;
                            bsInfo->stats.completeLayer3InfoSent++;
                        } // End of GSM_RR_MESSAGE_TYPE_PAGING_RESPONSE //
                    }
                    break;

                case GSM_PD_MM:
                    {
                        unsigned char*  mmMsgData;
                        int             mmMsgDataSize;


                        if (DEBUG_GSM_LAYER3_2 ){
                            printf(
                                "GSM_BS[%d] TS %d CH %d Rcvd a MM msg: \n",
                                node->nodeId,
                                recvSlotNumber,
                                recvChannelIndex );
                        }

                        if (GsmLayer3BsAssignSccpConnId( node,
                                 bsInfo,
                                 slotInfo,
                                 &newSccpConnectionId ) == TRUE ){
                            if (DEBUG_GSM_LAYER3_2 ){
                                printf(
                                    "GSM_BS[%d] GsmLayer3BsAssignSccpConnId:"
                                        " new= %d total= %d\n",
                                    node->nodeId,
                                    newSccpConnectionId,
                                    bsInfo->numSccpConnections );
                            }
                        }
                        else{
                            if (DEBUG_GSM_LAYER3_3 ){
                                printf(
                                    "GSM_BS[%d] ERROR: SCCPId COULD NOT BE "
                                        "ASSIGNED\n",
                                    node->nodeId );
                            }

                            return;
                        }

                        if (msgContent[1] ==
                               GSM_MM_MESSAGE_TYPE_CM_SERVICE_REQUEST ){
                            GsmCmServiceRequestMsg* cmReq;


                            cmReq = (
                                GsmCmServiceRequestMsg * )MESSAGE_ReturnPacket(
                                msg );

                            if (DEBUG_GSM_LAYER3_2 ){
                                printf( "CM_SERVICE_REQUEST_TYPE_MO_CALL "
                                            "from IMSI " );
                                printGsmImsi( cmReq->gsmImsi,
                                    GSM_MAX_IMSI_LENGTH );
                                printf( " on TS %d, Channel %d\n",
                                    recvSlotNumber,
                                    recvChannelIndex );
                            }

                            // Store information about this MS' call.
                            memcpy( slotInfo->msImsi,
                                cmReq->gsmImsi,
                                GSM_MAX_IMSI_LENGTH );

                            mmMsgData = ( unsigned char* )cmReq;
                            mmMsgDataSize = sizeof(
                                GsmCompleteLayer3InfoMsg ) +
                                                sizeof(
                                GsmCmServiceRequestMsg );

                            bsInfo->stats.cmServiceRequestRcvd++;
                        }
                        else if (
                                msgContent[1] ==
                                GSM_MM_MESSAGE_TYPE_LOCATION_UPDATE_REQUEST ){
                            GsmLocationUpdateRequestMsg*
                            locUpdateReq;

                            locUpdateReq = (
                                GsmLocationUpdateRequestMsg * )
                                    MESSAGE_ReturnPacket(
                                        msg );

                            if (DEBUG_GSM_LAYER3_2 ){
                                printf(
                                    "\tLOCATION_UPDATE_REQUEST from IMSI " );
                                printGsmImsi( locUpdateReq->imsi,
                                    GSM_MAX_IMSI_LENGTH );
                                printf( "\n" );
                            }

                            mmMsgData = ( unsigned char* )locUpdateReq;
                            mmMsgDataSize = sizeof(
                                GsmCompleteLayer3InfoMsg ) +
                                                sizeof(
                                GsmLocationUpdateRequestMsg );

                            bsInfo->stats.locationUpdateRequestRcvd++;
                        }
                        else{
                            if (DEBUG_GSM_LAYER3_3 ){
                                printf( "GSM_BS[%d] ERROR:"
                                            "UNKNOWN MM message type %d\n",
                                    node->nodeId,
                                    msgContent[1] );
                            }

                            return;
                        }

                        slotInfo->isSeizedByMs = TRUE;

                        GsmLayer3BuildCompleteLayer3InfoMsg( node,
                            bsInfo,
                            ( unsigned char* )mmMsgData,
                            msg->packetSize,
                            &completeL3Msg );

                        GsmLayer3SendPacketViaGsmAInterface( node,
                            bsInfo->mscNodeAddress,
                            node->nodeId,
                            newSccpConnectionId,
                            routingLabel,
                            GSM_SCCP_MSG_TYPE_CONNECT_REQUEST,
                            ( unsigned char* )completeL3Msg,
                            mmMsgDataSize );
                        MEM_free(completeL3Msg);

                        bsInfo->stats.completeLayer3InfoSent++;
                    } // End of MM sublayer processing //
                    break;

                case GSM_PD_CC:
                    // Call Control Messages
                    {
                        gsmBsProcessCcMessageFromMac( node,
                            msgContent,
                            channelDesc,
                            recvSlotNumber,
                            recvChannelIndex );
                    }   // CC messages //
                    break;

                default:
                    {
                        if (DEBUG_GSM_LAYER3_3 ){
                            sprintf( errorString,
                                "GSMLayer3 BS[%d]: "
                                    "Unknown PD 0x%x in msg: TS %d,"
                                    " CH %d at %s",
                                node->nodeId,
                                msgContent[0],
                                recvSlotNumber,
                                recvChannelIndex,
                                clockStr );
                            ERROR_ReportWarning( errorString );
                        }
                    }
            } // End of sublayer switch //
        } // End of all non-Channel Request Messages //
    } // End of GSM_BS //
} // End of GsmLayer3ReceivePacketFromMac //


static
void gsmMscProcessDtapMessages(
    Node* node,
    unsigned char* packetData,
    int             sccpConnectionId,
    NodeAddress     sourceNodeId,
    NodeAddress     sourceNodeAddress,
    GsmSccpRoutingLabel routingLabel,
    unsigned char   messageTypeCode)
{
    char                    clockStr[MAX_STRING_LENGTH];
    char                    errorString[MAX_STRING_LENGTH];

    GsmLayer3Data*          nwGsmData;
    GsmLayer3MscInfo*       mscInfo;
    GsmGenericLayer3MsgHdr* layer3MsgHdr;
    GsmMscCallInfo*         callInfo;
    GsmMscCallState*        messagedMs  = NULL; // MS for whom the message is sent
    GsmMscCallState*        peerMs      = NULL;     // peer MS to the one above
    GsmMscTimerData         mscTimerData;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    mscInfo = nwGsmData->mscInfo;

    ctoa(node->getNodeTime(), clockStr);

    // lookup call context
    if (GsmLayer3MscLookupCallInfoBySccpConnId( node,
             sccpConnectionId,
             sourceNodeAddress,
             FALSE,
             &callInfo ) == FALSE ){
        if (DEBUG_GSM_LAYER3_2 ){
            sprintf( errorString,
                "GSM-MSC [%d] ERROR: "
                    "GsmLayer3MscLookupCallInfoBySccpConnId "
                    "failed.\nSccp conn.id %d, source addr 0x%x\n",
                node->nodeId,
                sccpConnectionId,
                sourceNodeAddress );

            ERROR_ReportWarning( errorString );
        }

        return;
    }

    // Identify the MS for whom this message was sent
    if (sccpConnectionId == callInfo->originMs.sccpConnectionId &&
           sourceNodeAddress == callInfo->originMs.bsNodeAddress ){
        messagedMs = &( callInfo->originMs );
        peerMs = &( callInfo->termMs );
    }
    else if (sccpConnectionId == callInfo->termMs.sccpConnectionId &&
                sourceNodeAddress == callInfo->termMs.bsNodeAddress ){
        messagedMs = &( callInfo->termMs );
        peerMs = &( callInfo->originMs );
    }

    layer3MsgHdr = ( GsmGenericLayer3MsgHdr * )packetData;

    if (packetData[0] == GSM_PD_CC ){
        GsmVlrEntry*                        vlrEntry;


        switch ( layer3MsgHdr->messageType ){
            case GSM_CC_MESSAGE_TYPE_ALERTING:
                {
                    GsmMoAlertingMsg*   moAlertingMsg;


                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( " ALERTING: MT\n"
                                    "\tSending ALERTING: MO\n" );
                    }

                    if (callInfo->termMs.nwCcState !=
                           GSM_NW_CC_STATE_MOBILE_TERM_CALL_CONFIRMED ){
                        if (DEBUG_GSM_LAYER3_TIMERS ){
                            sprintf( errorString,
                                "GSM_MSC[%d] ERROR: Incorrect state",
                                node->nodeId );
                            ERROR_ReportWarning( errorString );
                        }
                        return;
                    }

                    if (callInfo->termMs.timerT310Msg != NULL ){
                        MESSAGE_CancelSelfMsg( node,
                            callInfo->termMs.timerT310Msg );
                        callInfo->termMs.timerT310Msg = NULL;
                    }

                    mscTimerData.callInfo = callInfo;
                    mscTimerData.isSetForOriginMs = FALSE; // Set for Term. MS

                    callInfo->termMs.timerT301Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmAlertingTimer_T301,
                        DefaultGsmAlertingTimer_T301Time,
                        &mscTimerData,
                        sizeof( GsmMscTimerData * ) );

                    callInfo->termMs.nwCcState =
                    GSM_NW_CC_STATE_CALL_RECEIVED;

                    GsmLayer3BuildMoAlertingMsg( node,
                        &moAlertingMsg,
                        callInfo->originMs.transactionIdentifier );

                    // Alerting is received from Call Terminating
                    // MS and the new alerting msg needs to be sent
                    // to the Call Originating MS.

                    GsmLayer3SendPacketViaGsmAInterface( node,
                        callInfo->originMs.bsNodeAddress,
                        node->nodeId,
                        callInfo->originMs.sccpConnectionId,
                        routingLabel,
                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                        ( unsigned char* )moAlertingMsg,
                        sizeof( GsmMoAlertingMsg ) );
                    MEM_free( moAlertingMsg );

                    callInfo->originMs.nwCcState =
                    GSM_NW_CC_STATE_CALL_DELIVERED;

                    mscInfo->stats.alertingRcvd++;
                    mscInfo->stats.alertingSent++;
                } // End of ALERTING //
                break;

            case GSM_CC_MESSAGE_TYPE_CALL_CONFIRMED:
                {
                    int termBsTrafficConnId;


                    if (callInfo->termMs.nwCcState !=
                           GSM_NW_CC_STATE_CALL_PRESENT ){
                        fflush( stdout );
                        if (DEBUG_GSM_LAYER3_TIMERS ){
                            sprintf( errorString,
                                "GSM_MSC[%d] ERROR: Incorrect state",
                                node->nodeId );
                            ERROR_ReportWarning( errorString );
                        }
                        return;
                    }

                    // setup traffic/terrestrial circuit to the
                    // terminating MS's BS
                    termBsTrafficConnId = GsmLayer3SetupTrafficConn( node,
                                              callInfo->termMs.bsNodeAddress,
                                              sccpConnectionId );

                    if (callInfo->termMs.timerT303Msg != NULL ){
                        MESSAGE_CancelSelfMsg( node,
                            callInfo->termMs.timerT303Msg );
                        callInfo->termMs.timerT303Msg = NULL;
                    }

                    callInfo->termMs.timerT310Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmCallProceedingTimer_T310,
                        DefaultGsmCallProceedingTimer_T310Time,
                        &callInfo,
                        sizeof( GsmMscCallInfo * ) );

                    if (DEBUG_GSM_LAYER3_2 ){
                        printf(
                            " CALL_CONFIRMED: MT\n"
                                "\tSetting up traffic conn. %d to Term.MS ",
                            termBsTrafficConnId );
                    }

                    mscInfo->trafficConnInfo[termBsTrafficConnId].callInfo =
                    callInfo;
                    callInfo->termMs.trafficConnectionId =
                    termBsTrafficConnId;

                    callInfo->termMs.nwCcState =
                    GSM_NW_CC_STATE_MOBILE_TERM_CALL_CONFIRMED;

                    mscInfo->stats.callConfirmedRcvd++;
                } // End of CALL_CONFIRMED //
                break;

            case GSM_CC_MESSAGE_TYPE_CONNECT:
                {
                    GsmMoConnectMsg*
                    moConnectMsg;


                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( " CONNECT: MT\n" "\tsending CONNECT: MO\n" );
                    }

                    if (callInfo->termMs.timerT301Msg != NULL ){
                        MESSAGE_CancelSelfMsg( node,
                            callInfo->termMs.timerT301Msg );
                        callInfo->termMs.timerT301Msg = NULL;
                    }

                    if (callInfo->termMs.timerT310Msg != NULL ){
                        MESSAGE_CancelSelfMsg( node,
                            callInfo->termMs.timerT310Msg );
                        callInfo->termMs.timerT310Msg = NULL;
                    }


                    if (callInfo->originMs.timerUDT1Msg != NULL ){
                        MESSAGE_CancelSelfMsg( node,
                            callInfo->originMs.timerUDT1Msg );
                        callInfo->originMs.timerUDT1Msg = NULL;
                    }

                    // Send to Call Originating MS
                    GsmLayer3BuildMoConnectMsg( node,
                        &moConnectMsg,
                        callInfo->termMs.msIsdn );

                    GsmLayer3SendPacketViaGsmAInterface( node,
                        callInfo->originMs.bsNodeAddress,
                        node->nodeId,
                        callInfo->originMs.sccpConnectionId,
                        routingLabel,
                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                        ( unsigned char* )moConnectMsg,
                        sizeof( GsmMoConnectMsg ) );
                    MEM_free( moConnectMsg );

                    callInfo->originMs.timerT313Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmConnectTimer_T313,
                        DefaultGsmConnectTimer_T313Time,
                        &callInfo,
                        sizeof( GsmMscCallInfo * ) );

                    callInfo->termMs.nwCcState =
                    GSM_NW_CC_STATE_CONNECT_REQUEST;
                    callInfo->originMs.nwCcState =
                    GSM_NW_CC_STATE_CONNECT_INDICATION;

                    mscInfo->stats.connectRcvd++;
                    mscInfo->stats.connectSent++;
                } // End of CONNECT //
                break;

            case GSM_CC_MESSAGE_TYPE_CONNECT_ACKNOWLEDGE:
                {
                    GsmConnectAcknowledgeMsg*   callAckMsg;


                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( " CONNECT_ACKNOWLEDGE: MO\n"
                                    "\tSending CONNECT_ACKNOWLEDGE: MT\n" );
                    }

                    if (DEBUG_GSM_LAYER3_3 ){
                        printf( "Network: ### Call Setup Done ###: %s\n",
                            clockStr );
                    }

                    callInfo->originMs.nwCcState = GSM_NW_CC_STATE_ACTIVE;
                    callInfo->termMs.nwCcState =
                    GSM_NW_CC_STATE_CONNECT_REQUEST;

                    if (callInfo->originMs.timerT313Msg != NULL ){
                        MESSAGE_CancelSelfMsg( node,
                            callInfo->originMs.timerT313Msg );
                        callInfo->originMs.timerT313Msg = NULL;
                    }

                    // From & to Call Terminating MS
                    GsmLayer3BuildMtConnectAcknowledgeMsg( node,
                        &callAckMsg );

                    GsmLayer3SendPacketViaGsmAInterface( node,
                        callInfo->termMs.bsNodeAddress,
                        node->nodeId,
                        callInfo->termMs.sccpConnectionId,
                        routingLabel,
                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                        ( unsigned char* )callAckMsg,
                        sizeof( GsmConnectAcknowledgeMsg ) );
                    MEM_free( callAckMsg );

                    callInfo->termMs.nwCcState = GSM_NW_CC_STATE_ACTIVE;

                    mscInfo->stats.numCallRequestsConnected++;
                    mscInfo->stats.connectAckRcvd++;
                    mscInfo->stats.connectAckSent++;
                } // End of CONNECT_ACKNOWLEDGE //
                break;

            case GSM_CC_MESSAGE_TYPE_DISCONNECT:
                {
                    GsmReleaseByNetworkMsg*     releaseMsg;
                    GsmDisconnectByNetworkMsg*  disconnectMsg;


                    if (DEBUG_GSM_LAYER3_3 ){
                        printf( " DISCONNECT msg (Call Clearing by MS)\n"
                                    "\tSending DISCONNECT: to peer MS\n" );
                    }

                    // Lookup the the other MS in the call
                    // initiate clearing at the this MS

                    if (sccpConnectionId !=
                           callInfo->originMs.sccpConnectionId ){
                        sprintf( errorString,
                            "GSM_MSC[%d]: DISCONNECT From Term.MS: ",
                            node->nodeId );
                        printGsmImsi( callInfo->originMs.imsi,10 );
                        fflush( stdout );
                        ERROR_ReportWarning( errorString );
                        return;
                    }

                    if (messagedMs->nwCcState == GSM_NW_CC_STATE_NULL ||
                           messagedMs->nwCcState ==
                           GSM_NW_CC_STATE_RELEASE_REQUEST ){
                        fflush( stdout );
                        if (DEBUG_GSM_LAYER3_3 ){
                            printf(
                                "GSM_MSC[%d] ERROR: Incorrect Call CC State"
                                    " %d\n",
                                node->nodeId,
                                messagedMs->nwCcState );
                        }

                        return;
                    }

                    GsmLayer3MscStopCallControlTimers( node,callInfo );

                    messagedMs->nwCcState = (
                        GsmNwCcState_e )GSM_MS_CC_STATE_RELEASE_REQUEST;
                    peerMs->nwCcState = (
                        GsmNwCcState_e )GSM_MS_CC_STATE_DISCONNECT_INDICATION;

                    // Initiate Call Clearing with peer MS
                    GsmLayer3BuildDisconnectByNetworkMsg( node,
                        &disconnectMsg,
                        GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
                        GSM_CAUSE_VALUE_NORMAL_CALL_CLEARING );

                    GsmLayer3SendPacketViaGsmAInterface( node,
                        peerMs->bsNodeAddress,
                        node->nodeId,
                        peerMs->sccpConnectionId,
                        routingLabel,
                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                        ( unsigned char* )disconnectMsg,
                        sizeof( GsmDisconnectByNetworkMsg ) );
                    MEM_free( disconnectMsg );

                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MSC[%d]: Setting T305\n",node->nodeId );
                    }

                    mscTimerData.callInfo = callInfo;

                    if (peerMs->isCallOriginator == TRUE ){
                        mscTimerData.isSetForOriginMs = TRUE; //for Term. MS
                    }
                    else{
                        mscTimerData.isSetForOriginMs = FALSE;
                    }
                    // mscTimerData->cause[0] =
                    // mscTimerData->cause[1] =

                    peerMs->timerT305Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmDisconnectTimer_T305,
                        DefaultGsmDisconnectTimer_T305Time,
                        &mscTimerData,
                        sizeof( GsmMscTimerData * ) );

                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( "\tSending RELEASE\n" );
                    }

                    // Proceed with release towards MS which sent the
                    // disconnect msg

                    GsmLayer3BuildReleaseByNetworkMsg( node,
                        &releaseMsg,
                        GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
                        GSM_CAUSE_VALUE_NORMAL_CALL_CLEARING );

                    GsmLayer3SendPacketViaGsmAInterface( node,
                        messagedMs->bsNodeAddress,
                        node->nodeId,
                        messagedMs->sccpConnectionId,
                        routingLabel,
                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                        ( unsigned char* )releaseMsg,
                        sizeof( GsmReleaseByNetworkMsg ) );
                    MEM_free( releaseMsg );

                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MSC[%d]: Setting T308\n",node->nodeId );
                    }

                    mscTimerData.callInfo = callInfo;
                    if (messagedMs->isCallOriginator == TRUE ){
                        mscTimerData.isSetForOriginMs = TRUE; //for Orig. MS
                    }
                    else{
                        mscTimerData.isSetForOriginMs = FALSE;
                    }
                    // mscTimerData.isSetForOriginMs = TRUE; // Set for Origin MS
                    // mscTimerData->cause[0] =
                    // mscTimerData->cause[1] =
                    messagedMs->timerT308Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmReleaseTimer_T308,
                        DefaultGsmReleaseTimer_T308Time,
                        &callInfo,
                        sizeof( GsmMscCallInfo * ) );

                    mscInfo->stats.disconnectRcvd++;
                    mscInfo->stats.disconnectSent++;
                    mscInfo->stats.releaseSent++;
                } // End of DISCONNECT //
                break;

            case GSM_CC_MESSAGE_TYPE_RELEASE:
                {
                    GsmReleaseCompleteByNetworkMsg* relCompMsg;
                    GsmChannelReleaseMsg*           channelRelMsg;


                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( " RELEASE msg\n"
                                    "\tSending RELEASE_COMPLETE to term MS\n"
                                    "\tSending CHANNEL_RELEASE to term MS,"
                                    " time %s\n",
                            clockStr );
                    }

                    // release of transaction identifier and all
                    // the rest of call related information.

                    if (messagedMs->timerT305Msg != NULL ){
                        if (DEBUG_GSM_LAYER3_TIMERS ){
                            printf( "GSM_MSC[%d]: Cancelling T305\n",
                                node->nodeId );
                        }

                        MESSAGE_CancelSelfMsg( node,
                            messagedMs->timerT305Msg );
                        messagedMs->timerT305Msg = NULL;
                    }

                    /***
                    if (callInfo->termMs.timerT308Msg != NULL)
                    {
                        printf("GSM_MSC[%d]: Cancelling T308\n", node->nodeId);
                        MESSAGE_CancelSelfMsg(node, callInfo->termMs.timerT308Msg);
                        callInfo->termMs.timerT308Msg = NULL;
                    }
                    ***/

                    GsmLayer3BuildReleaseCompleteByNetworkMsg( node,
                        &relCompMsg );

                    GsmLayer3SendPacketViaGsmAInterface( node,
                        messagedMs->bsNodeAddress,
                        node->nodeId,
                        messagedMs->sccpConnectionId,
                        routingLabel,
                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                        ( unsigned char* )relCompMsg,
                        sizeof( GsmReleaseCompleteByNetworkMsg ) );
                    MEM_free( relCompMsg );

                    messagedMs->nwRrState = GSM_NW_RR_STATE_IDLE;
                    messagedMs->nwMmState = GSM_NW_MM_STATE_IDLE;
                    messagedMs->nwCcState = GSM_NW_CC_STATE_NULL;

                    GsmLayer3BuildChannelReleaseMsg( node,&channelRelMsg );

                    GsmLayer3SendPacketViaGsmAInterface( node,
                        messagedMs->bsNodeAddress,
                        node->nodeId,
                        messagedMs->sccpConnectionId,
                        routingLabel,
                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                        ( unsigned char* )channelRelMsg,
                        sizeof( GsmChannelReleaseMsg ) );
                    MEM_free( channelRelMsg );

                    GsmLayer3MscReleaseTrafficConnId( mscInfo,
                        callInfo->termMs.trafficConnectionId );

                    // if originating MS's RELEASE_COMPLETE msg has been
                    // received then call is complete.
                    if (peerMs->nwRrState == GSM_NW_RR_STATE_IDLE ){
                        mscInfo->stats.numCallsCompleted++;
                        callInfo->inUse = FALSE;
                        memset( callInfo,0,sizeof( GsmMscCallInfo ) );

                        if (DEBUG_GSM_LAYER3_3 ){
                            printf( "GSM_MSC[%d]: Call Released\n",
                                node->nodeId );
                        }
                    }

                    mscInfo->stats.releaseRcvd++;
                    mscInfo->stats.releaseCompleteSent++;
                    mscInfo->stats.channelReleaseSent++;
                } // End of RELEASE //
                break;

            case GSM_CC_MESSAGE_TYPE_RELEASE_COMPLETE:
                {
                    GsmChannelReleaseMsg*   channelReleaseMsg;


                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( " RELEASE_COMPLETE msg\n"
                                    "\tSending CHANNEL_RELEASE, time %s\n",
                            clockStr );
                    }

                    // From MS that requested the DISCONNECT (Origin MS)

                    messagedMs->nwRrState = GSM_NW_RR_STATE_IDLE;
                    messagedMs->nwMmState = GSM_NW_MM_STATE_IDLE;
                    messagedMs->nwCcState = GSM_NW_CC_STATE_NULL;

                    /***
                    if (callInfo->termMs.timerT303Msg != NULL)
                    {
                        if (DEBUG_GSM_LAYER3_TIMERS) {
                            printf("GSM_MSC[%d]: Cancelling T303\n",
                                node->nodeId);
                        }
                        MESSAGE_CancelSelfMsg(node,
                            callInfo->termMs.timerT303Msg);
                        callInfo->termMs.timerT303Msg = NULL;
                    }
                    ***/

                    if (messagedMs->timerT308Msg != NULL ){
                        if (DEBUG_GSM_LAYER3_TIMERS ){
                            printf( "GSM_MSC[%d]: Cancelling T308\n",
                                node->nodeId );
                        }
                        MESSAGE_CancelSelfMsg( node,
                            messagedMs->timerT308Msg );
                        messagedMs->timerT308Msg = NULL;
                    }

                    GsmLayer3BuildChannelReleaseMsg( node,
                        &channelReleaseMsg );

                    GsmLayer3SendPacketViaGsmAInterface( node,
                        messagedMs->bsNodeAddress,
                        node->nodeId,
                        messagedMs->sccpConnectionId,
                        routingLabel,
                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                        ( unsigned char* )channelReleaseMsg,
                        sizeof( GsmChannelReleaseMsg ) );
                    MEM_free( channelReleaseMsg );

                    GsmLayer3MscReleaseTrafficConnId( mscInfo,
                        messagedMs->trafficConnectionId );

                    // if terminating MS's RELEASE msg has been received then
                    // call is complete.
                    if (peerMs->nwRrState == GSM_NW_RR_STATE_IDLE ){
                        mscInfo->stats.numCallsCompleted++;
                        callInfo->inUse = FALSE;
                        memset( callInfo,0,sizeof( GsmMscCallInfo ) );

                        if (DEBUG_GSM_LAYER3_3 ){
                            printf( "GSM_MSC[%d]: Call Released\n",
                                node->nodeId );
                        }
                    }

                    mscInfo->stats.releaseCompleteRcvd++;
                    mscInfo->stats.channelReleaseSent++;
                } // End of RELEASE_COMPLETE //
                break;

            case GSM_CC_MESSAGE_TYPE_SETUP:
                {
                    GsmMoSetupMsg*          moSetupMsg;
                    GsmCallProceedingMsg*   callProceedMsg;
                    GsmPagingMsg*           pagingMsg;
                    GsmMscCallInfo*         tempcallInfo;


                    int                     originBsTrafficConnId;

                    moSetupMsg = ( GsmMoSetupMsg * )packetData;

                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( " SETUP: MO\n" );
                        printf( "\tSending CALL_PROCEEDING\n" );
                    }

                    callInfo->originMs.nwCcState =
                    GSM_NW_CC_STATE_CALL_INITIATED;

                    // Check with VLR if this MS is allowed to make
                    // calls via SIFOC command & await the response
                    // 'Complete Call' msg.
                    // If allowed, send the Call Proceeding command
                    // to indicate acceptance of the call Setup
                    // request.

                    callInfo->originMs.transactionIdentifier =
                    moSetupMsg->transactionIdentifier;

                    callInfo->numTransactions++;

                    if (GsmLayer3MscLookupMsInVlrByIdentity( node,
                             &vlrEntry,
                             moSetupMsg->calledPartyNumber,
                             GSM_MSISDN ) == FALSE ){
                        if (DEBUG_GSM_LAYER3_3 ){
                            printf( "GSM-MSC [%d] ERROR: "
                                        "GsmLayer3MscLookupMsInVlrByIdentity"
                                        " failed for called MS\n",
                                node->nodeId );
                        }

                        // Should send back a failure message
                        return;
                    }
                    // check if Destination MS is busy then terminate the connection
                    if (GsmLayer3MscLookupCallInfoByImsi( node,
                             moSetupMsg->calledPartyNumber,
                             &tempcallInfo ) == TRUE ){
                        if (DEBUG_GSM_LAYER3_3 ){
                            printf( "GSM-MSC [%d] ERROR: "
                                        "GsmLayer3MscLookupCallInfoByImsi: ",
                                node->nodeId );
                            printGsmImsi( moSetupMsg->calledPartyNumber,
                                GSM_MAX_IMSI_LENGTH );
                            printf( "\n" );
                        }
                        return;
                    }

                    if (callInfo->originMs.timerUDT1Msg != NULL ){
                        MESSAGE_CancelSelfMsg( node,
                            callInfo->originMs.timerUDT1Msg );
                        callInfo->originMs.timerUDT1Msg = NULL;
                    }

                    // For now the IMSI & MSISDN values are the same.
                    memcpy( callInfo->termMs.imsi,
                        moSetupMsg->calledPartyNumber,
                        GSM_MAX_MSISDN_LENGTH );

                    memcpy( callInfo->termMs.msIsdn,
                        moSetupMsg->calledPartyNumber,
                        GSM_MAX_MSISDN_LENGTH );


                    GsmLayer3BuildCallProceedingMsg( node,
                        &callProceedMsg,
                        moSetupMsg->transactionIdentifier );

                    GsmLayer3SendPacketViaGsmAInterface( node,
                        callInfo->originMs.bsNodeAddress,
                        node->nodeId,
                        sccpConnectionId,
                        routingLabel,
                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                        ( unsigned char* )callProceedMsg,
                        sizeof( GsmCallProceedingMsg ) );
                    MEM_free( callProceedMsg );

                    callInfo->originMs.nwCcState =
                    GSM_NW_CC_STATE_MOBILE_ORIG_CALL_PROCEEDING;

                    if (DEBUG_GSM_LAYER3_2 ){
                        printf(
                            "\tSetting up a traffic conn. to orig.MS. Id = " );
                    }

                    // Setup traffic path to call originating MS's BS
                    originBsTrafficConnId = GsmLayer3SetupTrafficConn(
                        node,
                        callInfo->originMs.bsNodeAddress,
                        sccpConnectionId );

                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( " %d\n",originBsTrafficConnId );
                    }

                    // Maintain a pointer for easy lookup
                    mscInfo->trafficConnInfo[originBsTrafficConnId].callInfo =
                    callInfo;
                    callInfo->originMs.trafficConnectionId =
                    originBsTrafficConnId;
                    callInfo->termMs.bsNodeAddress = vlrEntry->bsNodeAddress;
                    callInfo->termMs.bsNodeId = vlrEntry->bsNodeId;

                    // Upon receipt of the Assignment Complete Cmd
                    // from the call originating MS (MO), initiate
                    // the MT MS paging.

                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( "\tSending a PAGING msg, to IMSI " );
                        printGsmImsi( vlrEntry->imsi,GSM_MAX_IMSI_LENGTH );
                        printf( "\n\n" );
                    }

                    GsmLayer3BuildPagingMsg( node,
                        &pagingMsg,
                        moSetupMsg->calledPartyNumber,
                        vlrEntry->lac,
                        vlrEntry->cellIdentity );

                    // Send to BS of Call Terminating MS
                    GsmLayer3SendPacketViaGsmAInterface( node,
                        callInfo->termMs.bsNodeAddress,
                        node->nodeId,
                        -1,
                        // Should be in ConnectionLess mode
                        routingLabel,
                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                        ( unsigned char* )pagingMsg,
                        sizeof( GsmPagingMsg ) );

                    // MEM_free(pagingMsg);
                    callInfo->pagingMsg = pagingMsg;
                    callInfo->numPagingAttempts++;

                    callInfo->termMs.nwMmState =
                    GSM_NW_MM_STATE_WAIT_FOR_RR_CONNECTION;
                    callInfo->termMs.nwCcState =
                    GSM_NW_CC_MM_CONNECTION_PENDING;

                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MS[%d]: Setting T3113, origSCCP %d\n",
                            node->nodeId,
                            sccpConnectionId );
                    }

                    callInfo->timerT3113Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmPagingTimer_T3113,
                        DefaultGsmPagingTimer_T3113Time,
                        &callInfo,
                        sizeof( GsmMscCallInfo * ) );

                    mscInfo->stats.numCallRequests++;
                    mscInfo->stats.setupRcvd++;
                    mscInfo->stats.pagingSent++;
                    mscInfo->stats.callProceedingSent++;
                } // End of GSM_MO_SETUP //
                break;

            default:
                {
                    sprintf( errorString,
                        "GSMLayer3 MSC node %d: "
                            "Unknown DTAP msg 0x%x received",
                        node->nodeId,
                        layer3MsgHdr->messageType );
                    ERROR_ReportWarning( errorString );
                }
                printf( "GSM_MSC [%d] ERROR: UNKNOWN MESSAGE RECEIVED\n",
                    node->nodeId );
        } // End of switch //
    } // End of CC msg processing //
} // End of gsmMscProcessDtapMessages //


/*
 * Purpose: Distributes messages from the Bs<->MSC interface
 *          It handles both the 'A' interface and
 *          traffic messages
 */
void GsmLayer3ReceivePacketOverIp(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress)
{
    unsigned char*  msgContent;

    msgContent = ( unsigned char* )MESSAGE_ReturnPacket( msg );

    // Distribute messages
    // Signalling messages on the 'A' interface
    if (( ( GsmSccpMsg * )msgContent )->messageTypeCode > 0 &&
           ( ( GsmSccpMsg * )msgContent )->messageTypeCode < 10 ){
        GsmLayer3ReceivePacketFromGsmAInterface( node,msg,sourceAddress );
    }
    else    // Traffic (voice/data) messages
    {
        GsmLayer3ReceiveTrafficPacket( node,msg,sourceAddress );
    }
    MESSAGE_Free( node,msg );
} // End of GsmLayer3ReceivePacketFromIp //


void GsmLayer3ReceiveTrafficPacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress)
{
    GsmLayer3Data*  nwGsmData;

    unsigned char   messageTypeCode;
    unsigned char*  msgContent;
    char            errorString[MAX_STRING_LENGTH];
    char            clockStr[MAX_STRING_LENGTH];
    int             packetSequenceNumber;

    int             trafficConnectionId;

    clocktype       currentTime;


    GsmLayer3RemoveTrafficHeader( node,
        msg,
        &trafficConnectionId,
        &messageTypeCode,
        &packetSequenceNumber );


    msgContent = ( unsigned char* )MESSAGE_ReturnPacket( msg );
    currentTime = node->getNodeTime();
    ctoa( currentTime,clockStr );

    if (DEBUG_GSM_LAYER3_1 ){
        printf( "Node Traf on Air [%d] Rcvd a Traffic msg:"
                    " id %d, type %d, source 0x%x at %s\n",
            node->nodeId,
            trafficConnectionId,
            messageTypeCode,
            sourceAddress,
            clockStr );
    }

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    if (nwGsmData->nodeType == GSM_BS ){
        GsmLayer3BsInfo*    bsInfo;
        GsmSlotUsageInfo*   msSlotInfo;

        int                 sccpConnectionId;


        bsInfo = nwGsmData->bsInfo;
        // Sent only from MSC to call originating & terminating BS's
        // to setup traffic/terrestrial circuits.
        // The receiving BS does not have to know whether it has access
        // to the originating or terminating MS.

        if (messageTypeCode == GSM_TRAFFIC_MESSAGE_TYPE_CONNECT_REQUEST ){
            int*                    temp;


            temp = ( int* )msgContent;
            sccpConnectionId = *temp;

            if (DEBUG_GSM_LAYER3_2 ){
                printf( "GSM_BS[%d] TRAFFIC_CONNECT_REQUEST, "
                            "ConnId: SCCP %d, traffic %d\n",
                    node->nodeId,
                    sccpConnectionId,
                    trafficConnectionId );
            }

            // store the connection information so that traffic
            // pkts can be sent to the correct MS.
            ERROR_Assert( GsmLayer3BsLookupMsBySccpConnId( bsInfo,
                              sccpConnectionId,
                              &msSlotInfo ) == TRUE,
                "GSM_BS ERROR: Could not find MS Info" );

            msSlotInfo->networkTrafficConnectionId = trafficConnectionId;

            if (bsInfo->trafficConnectionDesc[trafficConnectionId].inUse ==
                   FALSE ){
                bsInfo->trafficConnectionDesc[trafficConnectionId].inUse =
                TRUE;

                bsInfo->trafficConnectionDesc[trafficConnectionId].msSlotInfo =
                msSlotInfo;
            }
            else{
                if (DEBUG_GSM_LAYER3_2 ){
                    printf( "GSM_BS[%d] ERROR: Traffic conn %d"
                                " already in use\n",
                        node->nodeId,
                        trafficConnectionId );
                }
                return;
            }

            if (DEBUG_GSM_LAYER3_2 ){
                printf( " Setting traffic connId %d for looked up imsi ",
                    trafficConnectionId );
                printGsmImsi( ( unsigned char* )msSlotInfo->msImsi,10 );
                printf( "\n" );
            }
        } // End of GSM_TRAFFIC_MESSAGE_TYPE_CONNECT_REQUEST //
        else if (messageTypeCode == GSM_TRAFFIC_MESSAGE_TYPE_TRAFFIC ){
            Message*                        newTrafficMsg;


            //lookup MS to send the packet to.
            if (GsmLayer3BsLookupMsByTrafficConnId( bsInfo,
                     trafficConnectionId,
                     &msSlotInfo ) == FALSE ){
                if (DEBUG_GSM_LAYER3_3 ){
                    sprintf( errorString,
                        "GSM_BS[%d] ERROR: "
                            "GsmLayer3BsLookupMsByTrafficConnId %d failed,"
                            " %s\n",
                        node->nodeId,
                        trafficConnectionId,
                        clockStr );
                    ERROR_ReportWarning( errorString );
                }

                return;
            }

            // Convert packet to 'Um'/over-air format
            GsmLayer3BuildOverAirTrafficMsg( node,
                msgContent,
                &newTrafficMsg );

            //MESSAGE_Free(node, msg);

            GsmLayer3SendPacket( node,
                newTrafficMsg,
                msSlotInfo->slotNumber,
                msSlotInfo->dlChannelIndex,
                GSM_CHANNEL_TCH );
            bsInfo->stats.pktsSentOnAirTraffic++;
        } // End of GSM_TRAFFIC_MESSAGE_TYPE_TRAFFIC //
        else{
            printf( "GSM_BS[%d] ERROR: UNKNOWN message received\n",
                node->nodeId );
        }
    } // End of BS //
    else if (nwGsmData->nodeType == GSM_MSC ){
        int                 destTrafficConnectionId;
        GsmLayer3MscInfo*   mscInfo;
        GsmMscCallInfo*     callInfo    = NULL;
        NodeAddress         destNodeAddress;


        mscInfo = nwGsmData->mscInfo;

        // lookup call context
        if (GsmLayer3MscLookupCallInfoByTrafficConnId( mscInfo,
                 trafficConnectionId,
                 &callInfo ) == FALSE ){
            if (DEBUG_GSM_LAYER3_3 ){
                printf( "GSM-MSC [%d] ERROR: "
                            "GsmLayer3MscLookupCallInfoByTrafficConnId\n"
                            " failed. Id = %d\n",
                    node->nodeId,
                    trafficConnectionId );
            }

            return;
        }

        if (messageTypeCode == GSM_TRAFFIC_MESSAGE_TYPE_TRAFFIC ){
            // Send it to the 'other' side/end of the flow
            if (trafficConnectionId ==
                   callInfo->originMs.trafficConnectionId ){
                destNodeAddress = callInfo->termMs.bsNodeAddress;
                destTrafficConnectionId =
                callInfo->termMs.trafficConnectionId;
                callInfo->trafPktsFromOrigMs++;
            }
            else if (trafficConnectionId ==
                        callInfo->termMs.trafficConnectionId ){
                destNodeAddress = callInfo->originMs.bsNodeAddress;
                destTrafficConnectionId =
                callInfo->originMs.trafficConnectionId;
                callInfo->trafPktsFromTermMs++;
            }
            else{
                if (DEBUG_GSM_LAYER3_3 ){
                    printf( "GSM_MSC[%d] ERROR: "
                                "GsmLayer3ReceiveTrafficPacket:\n"
                                "\tUnknown traffic connId %d\n",
                        node->nodeId,
                        trafficConnectionId );
                }

                return;
            }

            GsmLayer3AInterfaceSendTrafficMsg( node,
                destNodeAddress,
                destTrafficConnectionId,
                messageTypeCode,
                packetSequenceNumber,
                msgContent,
                GSM_BURST_DATA_SIZE,
                msg->packetSize);

            //MESSAGE_Free(node, msg);

            mscInfo->stats.numTrafficPkts++;
        } // End of GSM_TRAFFIC_MESSAGE_TYPE_TRAFFIC //
        else{
            printf( "GSM_MSC [%d] ERROR: UNKNOWN message",node->nodeId );
        }
    } // End of MSC */
} // End of GsmLayer3ReceiveTrafficPacket //


void GsmLayer3MscProcessAInterfaceMsgs(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress)
{
    char                errorString[MAX_STRING_LENGTH];
    unsigned char*      packetData;
    unsigned char       sccpMessageTypeCode;
    int                 sccpConnectionId;

    GsmLayer3Data*      nwGsmData;
    NodeAddress         sourceNodeId;
    GsmSccpRoutingLabel routingLabel;
    GsmLayer3MscInfo*   mscInfo;
    GsmMscTimerData     mscTimerData;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    GsmLayer3RemoveSccpHeader( node,
        msg,
        &sccpConnectionId,
        &sourceNodeId,
        &routingLabel,
        &sccpMessageTypeCode );


    // Connectionless SCCP service messages will arrive with the
    // sccpConnectionId == -1. TODO: Improve the conn.less service.

    // Point to the encapsulated message
    packetData = ( unsigned char* )MESSAGE_ReturnPacket( msg );


    mscInfo = nwGsmData->mscInfo;

    if (DEBUG_GSM_LAYER3_2 ){
        printf( "GSM_MSC[%d]: 'A' i/f msg,"
                    " SCCpId %d, source 0x%x: \n",
            node->nodeId,
            sccpConnectionId,
            sourceAddress );
    }

    switch ( sccpMessageTypeCode ){
        case GSM_SCCP_MSG_TYPE_CONNECT_REQUEST:
            {
                if (packetData[0] == GSM_COMPLETE_LAYER3_INFORMATION ){
                    GsmCompleteLayer3InfoMsg*   compL3Msg;
                    unsigned char*              layer3Message;


                    compL3Msg = ( GsmCompleteLayer3InfoMsg * )packetData;

                    layer3Message = ( unsigned char* )&
                                        ( compL3Msg->layer3Message );

                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( "\tGSM_SCCP_MSG_TYPE_CONNECT_REQUEST\n" );
                        printf( "\tGSM_COMPLETE_LAYER3_INFORMATION with\n" );
                    }

                    if (layer3Message[0] == GSM_PD_RR &&
                           layer3Message[1] ==
                           GSM_RR_MESSAGE_TYPE_PAGING_RESPONSE ){
                        GsmPagingResponseMsg*   pageRespMsg;
                        GsmMtSetupMsg*          mtSetupMsg;
                        GsmMscCallInfo*         callInfo;


                        pageRespMsg = ( GsmPagingResponseMsg * )&
                                          ( compL3Msg->layer3Message );

                        // From Call Terminating MS (MT)

                        if (DEBUG_GSM_LAYER3_2 ){
                            printf( "\tGSM_RR_MESSAGE_TYPE_PAGING_RESPONSE: "
                                        "From Call Terminating MS\n"
                                        "\tSending SETUP msg\n" );
                        }

                        if (GsmLayer3MscLookupCallInfoByImsi( node,
                                 pageRespMsg->imsi,
                                 &callInfo ) == FALSE ){
                            if (DEBUG_GSM_LAYER3_3 ){
                                printf(
                                    "GSM-MSC [%d] ERROR: "
                                        "GsmLayer3MscLookupCallInfoByImsi: ",
                                    node->nodeId );
                                printGsmImsi( pageRespMsg->imsi,
                                    GSM_MAX_IMSI_LENGTH );
                                printf( "\n" );
                            }

                            return;
                        }

                        if (callInfo->timerT3113Msg != NULL ){
                            MESSAGE_CancelSelfMsg( node,
                                callInfo->timerT3113Msg );
                            callInfo->timerT3113Msg = NULL;
                        }
                        MEM_free( callInfo->pagingMsg );

                        if (callInfo->termMs.nwRrState !=
                               GSM_NW_RR_STATE_IDLE ){

                           if (DEBUG_GSM_LAYER3_3){
                                printf(
                                "GSM-MSC [%d] ERROR: Network call state is "
                                    "non-IDLE for IMSI ",
                                node->nodeId );
                                printGsmImsi( pageRespMsg->imsi,
                                GSM_MAX_IMSI_LENGTH );
                                printf( "\n" );
                           }


                            return;
                        }

                        callInfo->termMs.sccpConnectionId = sccpConnectionId;

                        // Need to generate transaction identifier
                        // per GSM 04.07, Section 11.2.3.1.3
                        callInfo->termMs.transactionIdentifier =
                            (unsigned char)(++(callInfo->numTransactions));

                        mscTimerData.callInfo = callInfo;
                        mscTimerData.isSetForOriginMs = FALSE;

                        callInfo->termMs.timerT303Msg = GsmLayer3StartTimer(
                            node,
                            MSG_NETWORK_GsmCallPresentTimer_T303,
                            DefaultGsmCallPresentTimer_T303Time,
                            &mscTimerData,
                            sizeof( GsmMscCallInfo * ) );

                        GsmLayer3BuildMtSetupMsg( node,
                            &mtSetupMsg,
                            callInfo->numTransactions,
                            callInfo->originMs.imsi,
                            pageRespMsg->imsi );

                        GsmLayer3SendPacketViaGsmAInterface( node,
                            callInfo->termMs.bsNodeAddress,
                            node->nodeId,
                            sccpConnectionId,
                            routingLabel,
                            GSM_SCCP_MSG_TYPE_CONNECT_CONFIRM,
                            ( unsigned char* )mtSetupMsg,
                            sizeof( GsmMtSetupMsg ) );
                        MEM_free( mtSetupMsg );

                        // update network call states
                        callInfo->termMs.nwRrState = GSM_NW_RR_STATE_DT_1;
                        callInfo->termMs.nwMmState =
                        GSM_NW_MM_STATE_MM_CONNECTION_ACTIVE;
                        callInfo->termMs.nwCcState =
                        GSM_NW_CC_STATE_CALL_PRESENT;

                        mscInfo->stats.pagingResponseRcvd++;
                        mscInfo->stats.setupSent++;
                    } // End of PAGING_RESPONSE //
                    else if (layer3Message[0] == GSM_PD_MM &&
                                layer3Message[1] ==
                                GSM_MM_MESSAGE_TYPE_CM_SERVICE_REQUEST ){
                        GsmCmServiceRequestMsg* cmReq;
                        GsmCmServiceAcceptMsg*  cmAccept;
                        GsmMscCallInfo*         callInfo;
                        GsmVlrEntry*            vlrEntry;
                        GsmMscTimerData         mscTimerData;

                        cmReq = ( GsmCmServiceRequestMsg * )&
                                    ( compL3Msg->layer3Message );

                        // From Call Originating MS (MO)
                        if (DEBUG_GSM_LAYER3_2 ){
                            printf( "\tGSM_CM_SERVICE_REQUEST: "
                                        "From Call Orig. MS\n"
                                        "\tsending CM_SERVICE_ACCEPT\n" );
                        }

                        // Send a process access request msg to VLR.
                        // Authentication procedures follow this.
                        // Not Implemented.

                        if (GsmLayer3MscLookupMsInVlrByIdentity( node,
                                 &vlrEntry,
                                 cmReq->gsmImsi,
                                 GSM_IMSI ) == FALSE ){
                            if (DEBUG_GSM_LAYER3_3){

                                printf(
                                "GSM-MSC [%d] ERROR: "
                                    "GsmLayer3MscLookupMsInVlrByIdentity,"
                                    " IMSI: ",
                                node->nodeId );
                                printGsmImsi( cmReq->gsmImsi,
                                GSM_MAX_IMSI_LENGTH );
                            }

                            return;
                        }

                        if (GsmLayer3MscLookupCallInfoByImsi( node,
                                 cmReq->gsmImsi,
                                 &callInfo ) == FALSE ){
                            // create call context, set values as the
                            // call setup proceeds.
                            ERROR_Assert( GsmLayer3MscAllocateCallInfo( node,
                                              &callInfo ) == TRUE,
                                "GsmLayer3MscAllocateCallInfo failed" );
                        }

                        if (callInfo->originMs.nwRrState !=
                               GSM_NW_RR_STATE_IDLE ){

                            if (DEBUG_GSM_LAYER3_3){
                                printf( "GSM-MSC [%d] ERROR: Network state is "
                                        "non-IDLE for IMSI ",
                                node->nodeId );
                                printGsmImsi( cmReq->gsmImsi,
                                GSM_MAX_IMSI_LENGTH );
                                printf( "\n" );
                            }
                            return;
                        }
                        // store caller's info
                        callInfo->originMs.isCallOriginator = TRUE;
                        memcpy( callInfo->originMs.imsi,
                            cmReq->gsmImsi,
                            GSM_MAX_IMSI_LENGTH );

                        // set SCCP connection Id with MS's BS
                        callInfo->originMs.sccpConnectionId =
                        sccpConnectionId;
                        callInfo->originMs.bsNodeAddress =
                        vlrEntry->bsNodeAddress;
                        callInfo->originMs.bsNodeId = vlrEntry->bsNodeId;

                        // update network call states
                        callInfo->originMs.nwRrState = GSM_NW_RR_STATE_DT_1;
                        callInfo->originMs.nwMmState =
                        GSM_NW_MM_STATE_WAIT_FOR_MOBILE_ORIG_MM_CONNECTION;

                        // send response message
                        GsmLayer3BuildCmServiceAcceptMsg( node,&cmAccept );

                        GsmLayer3SendPacketViaGsmAInterface( node,
                            callInfo->originMs.bsNodeAddress,
                            node->nodeId,
                            // source
                            sccpConnectionId,
                            routingLabel,
                            GSM_SCCP_MSG_TYPE_CONNECT_CONFIRM,
                            ( unsigned char* )cmAccept,
                            sizeof( GsmCmServiceAcceptMsg ) );
                        MEM_free( cmAccept );

                        mscTimerData.callInfo = callInfo;
                        mscTimerData.isSetForOriginMs = TRUE; // Set for origin. MS


                        callInfo->originMs.timerUDT1Msg = GsmLayer3StartTimer(
                            node,
                            MSG_NETWORK_GsmCmServiceAcceptTimer_UDT1,
                            DefaultGsmCmServiceAcceptTimer_UDT1Time,
                            &mscTimerData,
                            sizeof( GsmMscTimerData * ) );

                        mscInfo->stats.cmServiceRequestRcvd++;
                        mscInfo->stats.cmServiceAcceptSent++;
                    } // End of GSM_MM_MESSAGE_TYPE_CM_SERVICE_REQUEST_MSG //
                    else if (layer3Message[0] == GSM_PD_MM &&
                                layer3Message[1] ==
                                GSM_MM_MESSAGE_TYPE_LOCATION_UPDATE_REQUEST ){
                        GsmLocationUpdateRequestMsg*
                        locUpReqMsg;
                        GsmLocationUpdateAcceptMsg* locUpAcceptMsg;
                        GsmChannelReleaseMsg*       channelReleaseMsg;
                        GsmVlrEntry*                vlrEntry;


                        locUpReqMsg = ( GsmLocationUpdateRequestMsg * )&
                                          ( compL3Msg->layer3Message );

                        if (DEBUG_GSM_LAYER3_2 ){
                            printf( "\tLOCATION_UPDATE_REQUEST: "
                                        "BS source 0x%x, IMSI ",
                                sourceAddress );
                            printGsmImsi( locUpReqMsg->imsi,
                                GSM_MAX_IMSI_LENGTH );
                            printf( "\n" );
                        }

                        if (GsmLayer3MscLookupMsInVlrByIdentity( node,
                                 &vlrEntry,
                                 locUpReqMsg->imsi,
                                 GSM_IMSI ) == TRUE ){
                            // Location update for a known MS
                            // Cell re-selection.
                        }
                        else if (GsmLayer3MscAssignVlrEntry( node,
                                      &vlrEntry ) == FALSE ){
                            sprintf( errorString,
                                "GSM_MSC [%d]: GsmLayer3MscAssignVlrEntry: "
                                    "Could not assign an entry in VLR.\n",
                                node->nodeId );
                            ERROR_ReportWarning( errorString );

                            return;
                        }

                        vlrEntry->bsNodeAddress = sourceAddress;
                        vlrEntry->bsNodeId = sourceNodeId;

                        memcpy( vlrEntry->imsi,
                            locUpReqMsg->imsi,
                            GSM_MAX_IMSI_LENGTH );

                        // MSISDN is set to the same as IMSI
                        memcpy( vlrEntry->msIsdn,
                            locUpReqMsg->imsi,
                            GSM_MAX_MSISDN_LENGTH );

                        vlrEntry->lac = locUpReqMsg->lac;

                        if (DEBUG_GSM_LAYER3_2 ){
                            printf( "\tSending LOCATION_UPDATE_ACCEPT\n" );
                        }

                        GsmLayer3BuildLocationUpdateAcceptMsg( node,
                            &locUpAcceptMsg,
                            locUpReqMsg,
                            FALSE ); // followOnProceed not supported

                        GsmLayer3SendPacketViaGsmAInterface( node,
                            sourceAddress,
                            node->nodeId,
                            sccpConnectionId,
                            routingLabel,
                            GSM_SCCP_MSG_TYPE_CONNECT_CONFIRM,
                            ( unsigned char* )locUpAcceptMsg,
                            sizeof( GsmLocationUpdateAcceptMsg ) );
                        MEM_free( locUpAcceptMsg );

                        if (DEBUG_GSM_LAYER3_2 ){
                            printf( "\tSending CHANNEL_RELEASE\n" );
                        }

                        GsmLayer3BuildChannelReleaseMsg( node,
                            &channelReleaseMsg );

                        GsmLayer3SendPacketViaGsmAInterface( node,
                            vlrEntry->bsNodeAddress,
                            node->nodeId,
                            sccpConnectionId,
                            routingLabel,
                            GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                            ( unsigned char* )channelReleaseMsg,
                            sizeof( GsmChannelReleaseMsg ) );
                        MEM_free( channelReleaseMsg );

                        mscInfo->stats.channelReleaseSent++;
                        mscInfo->stats.locationUpdateRequestRcvd++;
                        mscInfo->stats.locationUpdateAcceptSent++;
                    } // Location Update Message //

                    mscInfo->stats.completeLayer3InfoRcvd++;
                } // Complete Layer 3 Message //
                else if (packetData[0] == GSM_HANDOVER_REQUEST_ACK ){
                    GsmHandoverRequestAckMsg*   hoReqAckMsg;
                    GsmHandoverCommandMsg*      hoCmdMsg;
                    GsmMscCallInfo*             callInfo;
                    GsmMscTimerData             mscTimerData;


                    hoReqAckMsg = ( GsmHandoverRequestAckMsg * )packetData;

                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( "GSM_MSC [%d] HANDOVER_REQUEST_ACK, "
                                    "tempHoId %d, sccp %d\n",
                            node->nodeId,
                            hoReqAckMsg->tempHoId,
                            sccpConnectionId );
                    }

                    callInfo = mscInfo->tempHoInfo \
                    [hoReqAckMsg->tempHoId].callInfo;

                    mscInfo->tempHoId--;

                    GsmLayer3BuildHandoverCommandMsg( node,
                        hoReqAckMsg,
                        &hoCmdMsg );

                    if (DEBUG_GSM_LAYER3_TIMERS ){
                        printf( "GSM_MSC[%d]: Setting T3103\n",
                            node->nodeId );
                    }

                    if (callInfo->originMs.isBsTryingHo == TRUE ){
                        callInfo->originMs.hoSccpConnectionId =
                        sccpConnectionId;
                        callInfo->originMs.hoTargetBsNodeAddress =
                        sourceAddress;

                        mscTimerData.isSetForOriginMs = TRUE;

                        GsmLayer3SendPacketViaGsmAInterface( node,
                            callInfo->originMs.bsNodeAddress,
                            node->nodeId,
                            callInfo->originMs.sccpConnectionId,
                            routingLabel,
                            GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                            ( unsigned char* )hoCmdMsg,
                            sizeof( GsmHandoverCommandMsg ) );
                    }
                    else if (callInfo->termMs.isBsTryingHo == TRUE ){
                        callInfo->termMs.hoSccpConnectionId =
                        sccpConnectionId;
                        callInfo->termMs.hoTargetBsNodeAddress =
                        sourceAddress;

                        mscTimerData.isSetForOriginMs = FALSE;

                        GsmLayer3SendPacketViaGsmAInterface( node,
                            callInfo->termMs.bsNodeAddress,
                            node->nodeId,
                            callInfo->termMs.sccpConnectionId,
                            routingLabel,
                            GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                            ( unsigned char* )hoCmdMsg,
                            sizeof( GsmHandoverCommandMsg ) );
                    }
                    else{
                        assert( FALSE );
                    }

                    mscTimerData.callInfo = callInfo;
                    callInfo->timerT3103Msg = GsmLayer3StartTimer(
                        node,
                        MSG_NETWORK_GsmHandoverTimer_T3103,
                        DefaultGsmHandoverTimer_T3103Time,
                        &mscTimerData,
                        sizeof( GsmMscTimerData * ) );

                    MEM_free(hoCmdMsg);

                    mscInfo->stats.handoverRequestAckRcvd++;
                    mscInfo->stats.handoverCommandSent++;
                } // End of GSM_HANDOVER_REQUEST_ACK //
            } // End of case GSM_SCCP_MSG_TYPE_CONNECT_REQUEST //
            break;

        case GSM_SCCP_MSG_TYPE_CONNECT_CONFIRM:
            {
                if (DEBUG_GSM_LAYER3_2 ){
                    printf( "SCCP CONNECT_CONFIRM rcvd in MSC\n" );
                }
            }
            break;

        case GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE:
            {
                GsmMscCallInfo* callInfo;


                if (packetData[0] == GSM_HANDOVER_REQUIRED ){
                    int                     hoSourceBsIndex;
                    int                     hoTargetBsIndex;
                    int                     traffId;
                    GsmHandoverRequiredMsg* hoReqdMsg;
                    GsmHandoverRequestMsg*  hoRequestMsg;


                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( "GSM_MSC [%d] HANDOVER_REQUIRED\n",
                            node->nodeId );
                    }

                    hoReqdMsg = ( GsmHandoverRequiredMsg * )packetData;

                    if (hoReqdMsg->numCells >= 1 ){
                        for (hoSourceBsIndex = 0;
                                hoSourceBsIndex < mscInfo->numBs;
                                hoSourceBsIndex++ ){
                            if (mscInfo->bsNodeId[hoSourceBsIndex] ==
                                   sourceNodeId ){
                                break;
                            }
                        }
                        if (hoSourceBsIndex == mscInfo->numBs ){

                            if (DEBUG_GSM_LAYER3_3){

                                printf(
                                "GSM_MSC [%d] ERROR: \n\tCould not find "
                                    "source BS info for HO_Required from BS"
                                    " %d\n",
                                node->nodeId,
                                sourceNodeId );
                            }

                            return;
                        }

                        // Lookup target BS's nodeId
                        for (hoTargetBsIndex = 0;
                                hoTargetBsIndex < mscInfo->numBs;
                                hoTargetBsIndex++ ){
                            if (
                                mscInfo->bsLac[hoTargetBsIndex] ==
                                hoReqdMsg->lac[0] &&
                                mscInfo->bsCellIdentity[hoTargetBsIndex] ==
                                hoReqdMsg->cellIdentity[0] ){
                                break;
                            }
                        }

                        if (hoTargetBsIndex == mscInfo->numBs ){

                            if (DEBUG_GSM_LAYER3_3){
                                printf(
                                "GSM_MSC [%d] ERROR: \n\tCould not find"
                                    " targetBS(%d, %d) for HORequired from"
                                    " BS %d\n",
                                node->nodeId,
                                hoReqdMsg->lac[0],
                                hoReqdMsg->cellIdentity[0],
                                sourceNodeId );
                            }

                            return;
                        }

                        if (DEBUG_GSM_LAYER3_2 ){
                            printf(
                                "\tSending HANDOVER_REQUEST to Target BS %d\n",
                                mscInfo->bsNodeId[hoTargetBsIndex] );
                        }

                        if (GsmLayer3MscLookupCallInfoBySccpConnId( node,
                                 sccpConnectionId,
                                 sourceAddress,
                                 FALSE,
                                 &callInfo ) == FALSE ){
                            if (DEBUG_GSM_LAYER3_3 ){
                                printf(
                                    "MSC ERROR: Could not lookup callInfo. "
                                        "SCCPId %d, source 0x%x\n",
                                    sccpConnectionId,
                                    sourceAddress );
                            }

                            return;
                        }

                        if (DEBUG_GSM_LAYER3_2 ){
                            printf( " traffic count: orig %d, term %d\n",
                                callInfo->trafPktsFromOrigMs++,
                                callInfo->trafPktsFromTermMs++ );
                        }

                        // Update call information for possible handover

                        if (sccpConnectionId ==
                               callInfo->originMs.sccpConnectionId &&
                               callInfo->originMs.bsNodeAddress ==
                               sourceAddress ){
                            if (DEBUG_GSM_LAYER3_3 ){
                                printf( "GSM_MSC[%d]: OrigBs 0x%x, IMSI ",
                                    node->nodeId,
                                    sourceAddress );
                                printGsmImsi( callInfo->originMs.imsi,10 );
                                printf( "\n" );
                            }

                            callInfo->originMs.isBsTryingHo = TRUE;

                            if (callInfo->originMs.nwCcState !=
                                   GSM_NW_CC_STATE_ACTIVE ){
                                if (DEBUG_GSM_LAYER3_3 ){
                                    printf( "GSM_MSC[%d] ERROR: "
                                                "Orig.NwState != ACTIVE\n",
                                        node->nodeId );
                                }

                                return;
                            }

                            callInfo->originMs.hoTargetBsNodeId =
                            mscInfo->bsNodeId[hoTargetBsIndex];
                            traffId = callInfo->originMs.trafficConnectionId;
                        }
                        else if (sccpConnectionId ==
                                    callInfo->termMs.sccpConnectionId &&
                                    callInfo->termMs.bsNodeAddress ==
                                    sourceAddress ){
                            callInfo->termMs.isBsTryingHo = TRUE;

                            if (callInfo->termMs.nwCcState !=
                                   GSM_NW_CC_STATE_ACTIVE ){
                                if (DEBUG_GSM_LAYER3_3 ){
                                    printf(
                                        "ERROR: Term.MS NwState != ACTIVE\n" );
                                }

                                return;
                            }

                            callInfo->termMs.hoTargetBsNodeId =
                            mscInfo->bsNodeId[hoTargetBsIndex];
                            traffId = callInfo->termMs.trafficConnectionId;
                        }
                        else{
                            if (DEBUG_GSM_LAYER3_3 ){
                                printf(
                                    "GSM_MSC [%d] ERROR: Cannot find callInfo\n",
                                    node->nodeId );
                            }

                            return;
                        }

                        // Maintain a temporary ref until an
                        // sccp_connect_request is received
                        if (mscInfo->tempHoId == GSM_MAX_MS_PER_MSC - 1 ){

                            if (DEBUG_GSM_LAYER3_3){
                                printf(
                                "GSM_MSC[%d] ERROR: Max simultaneous "
                                    "handovers %d. Increase GSM_MAX_MS_PER_MSC.\n",
                                node->nodeId,
                                GSM_MAX_MS_PER_MSC );
                            }

                            return;
                        }
                        mscInfo->tempHoInfo[mscInfo->tempHoId].callInfo =
                        callInfo;
                        mscInfo->tempHoInfo[mscInfo->tempHoId].tempHoId =
                        mscInfo->tempHoId;

                        // Send to target BS
                        GsmLayer3BuildHandoverRequestMsg(
                            mscInfo->bsLac[hoSourceBsIndex],
                            mscInfo->bsCellIdentity[hoSourceBsIndex],
                            hoReqdMsg->lac[0],
                            hoReqdMsg->cellIdentity[0],
                            hoReqdMsg->cause,
                            mscInfo->tempHoId,
                            // temp. reference parameter
                            &hoRequestMsg );

                        hoRequestMsg->trafficConnectionId = traffId;

                        GsmLayer3SendPacketViaGsmAInterface( node,
                            MAPPING_GetDefaultInterfaceAddressFromNodeId(
                            node,
                                mscInfo->bsNodeId[hoTargetBsIndex] ),
                            node->nodeId,
                            -1,
                            // Connectionless
                            routingLabel,
                            GSM_SCCP_MSG_TYPE_CONNECTIONLESS,
                            ( unsigned char* )hoRequestMsg,
                            sizeof( GsmHandoverRequestMsg ) );

                        MEM_free( hoRequestMsg );

                        mscInfo->stats.handoverRequestSent++;

                        mscInfo->tempHoId++;
                    } // If atleast one suitable cell was suggested //

                    mscInfo->stats.handoverRequiredRcvd++;
                } // End of HANDOVER_REQUIRED //
                else if (packetData[0] == GSM_HANDOVER_FAILURE ){
                    GsmHandoverFailureMsg*          hoFailureMsg;
                    GsmHandoverRequiredRejectMsg*   hoRejectMsg;


                    hoFailureMsg = ( GsmHandoverFailureMsg * )packetData;
                    if (GsmLayer3MscLookupCallInfoBySccpConnId( node,
                             sccpConnectionId,
                             sourceAddress,
                             FALSE,
                             &callInfo ) == FALSE ){
                        if (DEBUG_GSM_LAYER3_3 ){
                            printf(
                                "GSM_MSC ERROR: "
                                    "GsmLayer3MscLookupCallInfoBySccpConnId. "
                                    "SCCPId %d, source 0x%x\n",
                                sccpConnectionId,
                                sourceAddress );
                        }

                        return;
                    }

                    GsmLayer3BuildHandoverRequiredRejectMsg(
                        hoFailureMsg->cause,
                        &hoRejectMsg );

                    // Await the next handover required message
                    if (sccpConnectionId ==
                           callInfo->originMs.sccpConnectionId &&
                           sourceAddress ==
                           callInfo->originMs.bsNodeAddress &&
                           callInfo->originMs.isBsTryingHo == TRUE ){
                        callInfo->originMs.isBsTryingHo = FALSE;

                        GsmLayer3SendPacketViaGsmAInterface( node,
                            callInfo->originMs.bsNodeAddress,
                            node->nodeId,
                            callInfo->originMs.sccpConnectionId,
                            routingLabel,
                            GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                            ( unsigned char* )hoRejectMsg,
                            sizeof( GsmHandoverRequiredRejectMsg ) );
                    }
                    else{
                        callInfo->termMs.isBsTryingHo = FALSE;

                        GsmLayer3SendPacketViaGsmAInterface( node,
                            callInfo->termMs.bsNodeAddress,
                            node->nodeId,
                            callInfo->termMs.sccpConnectionId,
                            routingLabel,
                            GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                            ( unsigned char* )hoRejectMsg,
                            sizeof( GsmHandoverRequiredRejectMsg ) );
                    }

                    mscInfo->tempHoId--;
                    mscInfo->stats.handoverFailureRcvd++;
                } // End of HANDOVER_FAILURE //
                else if (packetData[0] == GSM_HANDOVER_DETECT ){
                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( "GSM_MSC [%d] GSM_HANDOVER_DETECT rcvd\n",
                            node->nodeId );
                    }

                    mscInfo->stats.handoverDetectRcvd++;
                } // End of HANDOVER_DETECT //
                else if (packetData[0] == GSM_HANDOVER_COMPLETE ){
                    int                 oldBsNodeAddress    = 0;
                    int                 oldBsSccpId         = 0;

                    GsmClearCommandMsg* clearCommMsg;


                    if (DEBUG_GSM_LAYER3_2 ){
                        printf(
                            "GSM_MSC[%d] HANDOVER_COMPLETE, SCCP %d, Addr 0x%x "
                                "sending CLEAR_COMMAND\n",
                            node->nodeId,
                            sccpConnectionId,
                            sourceAddress );
                    }

                    if (GsmLayer3MscLookupCallInfoBySccpConnId( node,
                             sccpConnectionId,
                             sourceAddress,
                             TRUE,
                             &callInfo ) == FALSE ){
                        if (DEBUG_GSM_LAYER3_3 ){
                            printf( "MSC ERROR: Could not lookup callInfo. "
                                        "SCCPId %d, Addr 0x%x\n",
                                sccpConnectionId,
                                sourceAddress );
                        }

                        return;
                    }

                    if (callInfo->termMs.isBsTryingHo == TRUE ){
                        oldBsSccpId = callInfo->termMs.sccpConnectionId;
                        oldBsNodeAddress = callInfo->termMs.bsNodeAddress;
                    }
                    else if (callInfo->originMs.isBsTryingHo == TRUE ){
                        oldBsSccpId = callInfo->originMs.sccpConnectionId;
                        oldBsNodeAddress = callInfo->originMs.bsNodeAddress;
                    }
                    else{
                        if (DEBUG_GSM_LAYER3_TIMERS){
                            printf( "GSM_MSC[%d] ERROR in CallInfo "
                                    "Sccp %d, source 0x%x\n",
                            node->nodeId,
                            sccpConnectionId,
                            sourceAddress );
                            fflush( stdout );
                        }

                        if (DEBUG_GSM_LAYER3_TIMERS ){
                            sprintf( errorString,
                                "GSM_MSC[%d] ERROR: Incorrect state",
                                node->nodeId );
                            ERROR_ReportWarning( errorString );
                        }
                    }

                    if (callInfo->timerT3103Msg != NULL ){
                        if (DEBUG_GSM_LAYER3_TIMERS ){
                            printf( "GSM_MSC[%d]: Cancelling T3103\n",
                                node->nodeId );
                        }
                        MESSAGE_CancelSelfMsg( node,
                            callInfo->timerT3103Msg );
                        callInfo->timerT3103Msg = NULL;
                    }

                    // Free Old Channel

                    GsmLayer3BuildClearCommandMsg( node,&clearCommMsg );
                    GsmLayer3SendPacketViaGsmAInterface( node,
                        oldBsNodeAddress,
                        node->nodeId,
                        oldBsSccpId,
                        routingLabel,
                        GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                        ( unsigned char* )clearCommMsg,
                        sizeof( GsmClearCommandMsg ) );

                    MEM_free(clearCommMsg);

                    mscInfo->stats.handoverCompleteRcvd++;
                    mscInfo->stats.clearCommandSent++;
                } // End of HANDOVER_COMPLETE //
                else if (packetData[0] == GSM_CLEAR_COMPLETE ){
                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( "GSM_MSC [%d] CLEAR_COMPLETE\n",
                            node->nodeId );
                    }

                    if (GsmLayer3MscLookupCallInfoBySccpConnId( node,
                             sccpConnectionId,
                             sourceAddress,
                             FALSE,
                             &callInfo ) == FALSE ){
                        if (DEBUG_GSM_LAYER3_3 ){
                            printf(
                                "MSC ERROR: "
                                    "GsmLayer3MscLookupCallInfoBySccpConnId "
                                    "failed, SCCPId %d\n",
                                sccpConnectionId );
                        }

                        return;
                    }

                    if (callInfo->termMs.isBsTryingHo == TRUE ){
                        callInfo->termMs.sccpConnectionId =
                        callInfo->termMs.hoSccpConnectionId;
                        callInfo->termMs.bsNodeId =
                        callInfo->termMs.hoTargetBsNodeId;
                        callInfo->termMs.bsNodeAddress =
                        callInfo->termMs.hoTargetBsNodeAddress;

                        callInfo->termMs.hoSccpConnectionId = -1;
                        callInfo->termMs.hoTargetBsNodeAddress = 0;
                        callInfo->termMs.isBsTryingHo = FALSE;
                    }
                    else if (callInfo->originMs.isBsTryingHo == TRUE ){
                        callInfo->originMs.sccpConnectionId =
                        callInfo->originMs.hoSccpConnectionId;
                        callInfo->originMs.bsNodeId =
                        callInfo->originMs.hoTargetBsNodeId;
                        callInfo->originMs.bsNodeAddress =
                        callInfo->originMs.hoTargetBsNodeAddress;

                        callInfo->originMs.hoSccpConnectionId = -1;
                        callInfo->originMs.hoTargetBsNodeAddress = 0;
                        callInfo->originMs.isBsTryingHo = FALSE;
                    }
                    else{
                        assert( FALSE );
                    }

                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( "\t Completed Handover\n" );
                    }

                    mscInfo->stats.clearCompleteRcvd++;
                    mscInfo->stats.numHandovers++;
                } // End of CLEAR_COMPLETE //

                else if (packetData[0] == GSM_CALL_CLEAR_COMMAND ){
                    if (DEBUG_GSM_LAYER3_2 ){
                        printf( "GSM_MSC [%d] CLEAR_COMPLETE\n",
                            node->nodeId );
                    }

                    if (GsmLayer3MscLookupCallInfoBySccpConnId( node,
                             sccpConnectionId,
                             sourceAddress,
                             FALSE,
                             &callInfo ) == FALSE ){
                        if (DEBUG_GSM_LAYER3_3 ){
                            printf(
                                "MSC ERROR: "
                                    "GsmLayer3MscLookupCallInfoBySccpConnId "
                                    "failed, SCCPId %d\n",
                                sccpConnectionId );
                        }

                        return;
                    }
                    /*
                        GsmLayer3MscInitiateCallClearing(
                            node,
                            callInfo,
                            GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
                            GSM_CAUSE_VALUE_USER_ALERTING_NO_ANSWER,
                            GSM_LOCATION_PUBLIC_NETWORK_LOCALUSER,
                            GSM_CAUSE_VALUE_RECOVERY_ON_TIMER_EXPIRY );
                    */
                    GsmLayer3MscInitiateCallClearing( node,
                        callInfo,
                        packetData[1],
                        packetData[1],
                        packetData[1],
                        packetData[1] );
                }// end of GSM_CALL_CLEAR_COMMAND
                else{
                    gsmMscProcessDtapMessages( node,
                        packetData,
                        sccpConnectionId,
                        sourceNodeId,
                        sourceAddress,
                        routingLabel,
                        sccpMessageTypeCode );
                }
            } // End of GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE //
            break;

        default:
            assert( FALSE );
    } // End of switch on sccpMessageType //
} // End of GsmLayer3MscProcessAInterfaceMsgs //


void GsmLayer3BsProcessAInterfaceMsgs(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress)
{
    unsigned char*      packetData;
    unsigned char       sccpMessageTypeCode;
    unsigned char*      newMsgData;
    char                errorString[MAX_STRING_LENGTH];

    int                 dtapMsgDataSize = -1;
    int                 sccpConnectionId;

    GsmLayer3Data*      nwGsmData;
    NodeAddress         sourceNodeId;
    GsmSccpRoutingLabel routingLabel;

    Message*            newMsg;    // GSM msg sent on air to MS
    GsmLayer3BsInfo*    bsInfo;
    GsmSlotUsageInfo*   msSlotInfo;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    GsmLayer3RemoveSccpHeader( node,
        msg,
        &sccpConnectionId,
        &sourceNodeId,
        &routingLabel,
        &sccpMessageTypeCode );

    // Connectionless SCCP service messages will arrive with the
    // sccpConnectionId == -1. TODO: Improve the conn.less service.

    // Point to the encapsulated message
    packetData = ( unsigned char* )MESSAGE_ReturnPacket( msg );

    if (DEBUG_GSM_LAYER3_2 ){
        printf( "GSM_BS[%d] Rcvd msg from MSC: ",node->nodeId );
    }


    bsInfo = nwGsmData->bsInfo;

    if (packetData[0] == GSM_PAGING ){
        GsmPagingMsg*   pagingMsg;

        if (DEBUG_GSM_LAYER3_2 ){
            printf( "GSM_PAGING\n\tSending PAGE_REQUEST to MS\n" );
        }

        pagingMsg = ( GsmPagingMsg * )packetData;

        GsmLayer3BuildPagingRequestMsg( node,&newMsg,pagingMsg->imsi );

        // Send to the paging sub-group of the MS
        GsmLayer3SendPacket( node,newMsg,0, // slot number
            bsInfo->channelIndexStart, // downlink control channel
            GSM_CHANNEL_PAGCH );

        bsInfo->stats.pagingRcvd++;
        bsInfo->stats.pagingRequestSent++;

        return;
    } // End of GSM_PAGING //
    else if (packetData[0] == GSM_HANDOVER_REQUEST ){
        int                         newSccpConnectionId;

        GsmChannelInfo              channelInfo;
        GsmHandoverRequestMsg*      hoReqMsg;
        GsmHandoverRequestAckMsg*   hoReqAckMsg;
        GsmSlotUsageInfo*           assignedSlotInfo;

        // Handover Target BS
        hoReqMsg = ( GsmHandoverRequestMsg * )packetData;

        if (DEBUG_GSM_LAYER3_2 ){
            printf( "GSM_BS[%d]: HANDOVER_REQUEST, tempHoId %d\n"
                        "\tSending HANDOVER_REQUEST_ACK (with RI-HO CMD)\n",
                node->nodeId,
                hoReqMsg->tempHoId );
        }

        if (GsmLayer3BsAssignChannel( node,
                 bsInfo,
                 &assignedSlotInfo ) == FALSE ){
            GsmHandoverFailureMsg*  hoFailureMsg;


            sprintf( errorString,
                "GSM_BS[%d]: Handover targetBS cannot assign a channel",
                node->nodeId );
            ERROR_ReportWarning( errorString );

            GsmLayer3BuildHandoverFailureMsg( node,
                bsInfo,
                assignedSlotInfo,
                GSM_AIF_CAUSE_NO_RADIO_RESOURCE_AVAILABLE,
                GSM_RR_CAUSE_NORMAL_EVENT,
                &hoFailureMsg );

            GsmLayer3SendPacketViaGsmAInterface( node,
                bsInfo->mscNodeAddress,
                node->nodeId,
                sccpConnectionId,
                routingLabel,
                GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                ( unsigned char* )hoFailureMsg,
                sizeof( GsmHandoverFailureMsg ) );
            MEM_free( hoFailureMsg );

            return;
        }

        // Start listening on this channel
        channelInfo.channelIndex = assignedSlotInfo->ulChannelIndex;
        channelInfo.slotNumber = assignedSlotInfo->slotNumber;

        GsmLayer3SendInterLayerMsg( node,
            bsInfo->interfaceIndex,
            GSM_L3_MAC_CHANNEL_START_LISTEN,
            ( unsigned char* )&channelInfo,
            sizeof( GsmChannelInfo ) );

        assignedSlotInfo->isHandoverInProgress = TRUE;
        assignedSlotInfo->isUsedForTraffic = TRUE;

        assignedSlotInfo->networkTrafficConnectionId =
        hoReqMsg->trafficConnectionId;

        bsInfo->trafficConnectionDesc \
        [hoReqMsg->trafficConnectionId].inUse = TRUE;

        bsInfo->trafficConnectionDesc \
        [hoReqMsg->trafficConnectionId].msSlotInfo = assignedSlotInfo;

        GsmLayer3BuildHandoverRequestAckMsg( node,
            bsInfo,
            assignedSlotInfo,
            hoReqMsg->tempHoId,
            &hoReqAckMsg );

        if (GsmLayer3BsAssignSccpConnId( node,
                 bsInfo,
                 assignedSlotInfo,
                 &newSccpConnectionId ) == FALSE ){
            GsmHandoverFailureMsg*  hoFailureMsg;


            sprintf( errorString,
                "GSM_BS[%d] ERROR: Could not allocate SCCPId for Handover",
                node->nodeId );
            ERROR_ReportWarning( errorString );

            GsmLayer3BuildHandoverFailureMsg( node,
                bsInfo,
                assignedSlotInfo,
                GSM_AIF_CAUSE_REQUESTED_TERRESTRIAL_RESOURCE_UNAVIL,
                GSM_RR_CAUSE_NORMAL_EVENT,
                &hoFailureMsg );

            GsmLayer3SendPacketViaGsmAInterface( node,
                bsInfo->mscNodeAddress,
                node->nodeId,
                sccpConnectionId,
                routingLabel,
                GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
                ( unsigned char* )hoFailureMsg,
                sizeof( GsmHandoverFailureMsg ) );
            MEM_free( hoFailureMsg );

            return;
        }

        assignedSlotInfo->sccpConnectionId = newSccpConnectionId;

        GsmLayer3SendPacketViaGsmAInterface( node,
            bsInfo->mscNodeAddress,
            node->nodeId,
            newSccpConnectionId,
            routingLabel,
            GSM_SCCP_MSG_TYPE_CONNECT_REQUEST,
            ( unsigned char* )hoReqAckMsg,
            sizeof( GsmHandoverRequestAckMsg ) );
        MEM_free( hoReqAckMsg );

        bsInfo->stats.handoverRequestRcvd++;
        bsInfo->stats.handoverRequestAckSent++;
    } // End of HANDOVER_REQUEST //
    else if (packetData[0] == GSM_HANDOVER_REQUIRED_REJECT ){
        ERROR_Assert( GsmLayer3BsLookupMsBySccpConnId( bsInfo,
                          sccpConnectionId,
                          &msSlotInfo ) == TRUE,
            "GSM_BS ERROR: Could not find MS Info" );

        msSlotInfo->isHandoverInProgress = FALSE;
        bsInfo->stats.handoverRequiredRejectRcvd++;
    } // End of HANDOVER_REQUIRED_REJECT //
    else if (packetData[0] == GSM_HANDOVER_COMMAND ){
        GsmHandoverCommandMsg*  hoCmd;


        if (DEBUG_GSM_LAYER3_2 ){
            printf( "GSM_BS[%d] HANDOVER_COMMAND\n",node->nodeId );
        }

        hoCmd = ( GsmHandoverCommandMsg * )packetData;
        GsmLayer3BuildRIHandoverCommandMsg( node,hoCmd,&newMsg );

        ERROR_Assert( GsmLayer3BsLookupMsBySccpConnId( bsInfo,
                          sccpConnectionId,
                          &msSlotInfo ) == TRUE,
            "GSM_BS ERROR: Could not find MS Info" );

        GsmLayer3SendPacket( node,
            newMsg,
            msSlotInfo->slotNumber,
            msSlotInfo->dlChannelIndex,
            GSM_CHANNEL_TCH );

        bsInfo->stats.handoverCommandRcvd++;
        bsInfo->stats.riHandoverCommandSent++;
    } // End of HANDOVER_COMMAND //
    else if (packetData[0] == GSM_CLEAR_COMMAND ){
        GsmClearCompleteMsg*                                clearCompMsg;


        if (DEBUG_GSM_LAYER3_2 ){
            printf( "CLEAR_COMMAND, sccp %d\n"
                        "\tsending back CLEAR_COMPLETE\n",
                sccpConnectionId );
        }

        ERROR_Assert( GsmLayer3BsLookupMsBySccpConnId( bsInfo,
                          sccpConnectionId,
                          &msSlotInfo ) == TRUE,
            "GSM_BS ERROR: Could not find MS Info" );

        bsInfo->trafficConnectionDesc \
        [msSlotInfo->networkTrafficConnectionId].inUse = FALSE;

        bsInfo->trafficConnectionDesc \
        [msSlotInfo->networkTrafficConnectionId].msSlotInfo = NULL;

        GsmLayer3BuildClearCompleteMsg( node,&clearCompMsg );

        GsmLayer3SendPacketViaGsmAInterface( node,
            sourceAddress,
            node->nodeId,
            sccpConnectionId,
            routingLabel,
            GSM_SCCP_MSG_TYPE_CONNECTION_REFERENCE,
            ( unsigned char* )clearCompMsg,
            sizeof( GsmClearCompleteMsg ) );

        GsmLayer3StartTimer( node,
            MSG_NETWORK_GsmBsChannelReleaseTimer_T3111,
            DefaultGsmBsChannelReleaseTimer_T3111Time,
            &msSlotInfo,
            sizeof( GsmSlotUsageInfo * ) );

        GsmLayer3BsReleaseSccpConnId( node,sccpConnectionId,bsInfo );

        bsInfo->stats.clearCommandRcvd++;
        bsInfo->stats.clearCompleteSent++;
    } // End of GSM_CLEAR_COMMAND //
    else if (packetData[0] == GSM_PD_RR ){
        if (packetData[1] == GSM_RR_MESSAGE_TYPE_CHANNEL_RELEASE ){
            GsmChannelReleaseMsg*   chanRelMsg;
            GsmChannelType_e        channelType;


            // printf(" CHANNEL_RELEASE\n");
            if (GsmLayer3BsLookupMsBySccpConnId( bsInfo,
                     sccpConnectionId,
                     &msSlotInfo ) == FALSE ){
                if (DEBUG_GSM_LAYER3_3 ){
                    sprintf( errorString,
                        "GSM_BS[%d] ERROR: "
                            " GsmLayer3BsLookupMsBySccpConnId %d failed\n",
                        node->nodeId,
                        sccpConnectionId );
                    ERROR_ReportWarning( errorString );
                }

                return;
            }

            // Send it as a signaling msg over the traffic channel
            newMsg = MESSAGE_Alloc( node,
                         NETWORK_LAYER,
                         0,
                         MSG_MAC_FromNetwork );

            if (msSlotInfo->isUsedForTraffic == TRUE ){
                MESSAGE_PacketAlloc( node,
                    newMsg,
                    sizeof( GsmChannelReleaseMsg ) + 1,
                    TRACE_GSM );

                chanRelMsg = ( GsmChannelReleaseMsg * )packetData;
                packetData = (
                    unsigned char* )MESSAGE_ReturnPacket( newMsg );

                packetData[0] = TRUE;
                memcpy( packetData + 1,
                    chanRelMsg,
                    sizeof( GsmChannelReleaseMsg ) );

                channelType = GSM_CHANNEL_TCH;

                bsInfo->trafficConnectionDesc \
                [msSlotInfo->networkTrafficConnectionId].inUse = FALSE;

                bsInfo->trafficConnectionDesc \
                [msSlotInfo->networkTrafficConnectionId].msSlotInfo = NULL;
            }
            else{
                MESSAGE_PacketAlloc( node,
                    newMsg,
                    sizeof( GsmChannelReleaseMsg ),
                    TRACE_GSM );

                chanRelMsg = ( GsmChannelReleaseMsg * )packetData;
                packetData = (
                    unsigned char* )MESSAGE_ReturnPacket( newMsg );
                memcpy( packetData,
                    chanRelMsg,
                    sizeof( GsmChannelReleaseMsg ) );

                channelType = GSM_CHANNEL_SDCCH;
            }

            GsmLayer3SendPacket( node,
                newMsg,
                msSlotInfo->slotNumber,
                msSlotInfo->dlChannelIndex,
                channelType );

            if (DEBUG_GSM_LAYER3_2 ){
                printf( "GSM_BS[%d]: Release Slot %d, DownlinkCh %d, "
                            "Sccp %d, traf %d\n",
                    node->nodeId,
                    msSlotInfo->slotNumber,
                    msSlotInfo->dlChannelIndex,
                    sccpConnectionId,
                    msSlotInfo->networkTrafficConnectionId );
            }

            GsmLayer3BsReleaseSccpConnId( node,sccpConnectionId,bsInfo );

            GsmLayer3StartTimer( node,
                MSG_NETWORK_GsmBsChannelReleaseTimer_T3111,
                DefaultGsmBsChannelReleaseTimer_T3111Time,
                &msSlotInfo,
                sizeof( GsmSlotUsageInfo * ) );

            bsInfo->stats.channelReleaseRcvd++;
            bsInfo->stats.channelReleaseSent++;
        } // End of CHANNEL_RELEASE //
    } // End of GSM_PD_RR messages //
    else if (packetData[0] == GSM_PD_CC || packetData[0] == GSM_PD_MM ){
        GsmSlotUsageInfo*   msSlotInfo;
        BOOL flag = FALSE;

        // DTAP messages, sent directly to/from MS/MSC
        // without interpretation by BS

        // Should check to see if it return TRUE???
        // What about connectionless messages?
        if (GsmLayer3BsLookupMsBySccpConnId( bsInfo,
                 sccpConnectionId,
                 &msSlotInfo ) == FALSE ){
            if (DEBUG_GSM_LAYER3_3 ){
                printf( "GSM_BS[%d] ERROR: "
                            "GsmLayer3BsLookupMsBySccpConnId "
                            "Could not find MS info, sccpId %d\n",
                    node->nodeId,
                    sccpConnectionId );
            }
            return;
        }

        switch ( packetData[0] ){
            case GSM_PD_MM:
                {
                    if (packetData[1] ==
                           GSM_MM_MESSAGE_TYPE_CM_SERVICE_ACCEPT ){
                        // Forward the message over to MS

                        dtapMsgDataSize = sizeof( GsmCmServiceAcceptMsg );
                    }
                    else if (packetData[1] ==
                                GSM_MM_MESSAGE_TYPE_LOCATION_UPDATE_ACCEPT ){
                        dtapMsgDataSize = sizeof(
                            GsmLocationUpdateAcceptMsg );
                        if (DEBUG_GSM_LAYER3_2 ){
                            GsmLocationUpdateAcceptMsg* locUpAcceptMsg  = (
                                    GsmLocationUpdateAcceptMsg* )packetData;
                            printf( "GSM_BS[%d]LOCATION_UPDATE_ACCEPT: "
                                        "IMSI ",
                                node->nodeId );
                            printGsmImsi( locUpAcceptMsg->imsi,
                                GSM_MAX_IMSI_LENGTH );
                            printf( "\n" );
                        }
                    }
                    else{
                        if (DEBUG_GSM_LAYER3_2 ){
                            printf( "GSM_BS[%d] ERROR: Unknown MM message\n",
                                node->nodeId );
                        }
                    }
                }
                break;

            case GSM_PD_CC:
                {
                    if (packetData[1] ==
                           GSM_CC_MESSAGE_TYPE_CALL_PROCEEDING ){
                        // printf(" CALL_PROCEEDING\n");
                        dtapMsgDataSize = sizeof( GsmCallProceedingMsg );
                    }
                    else if (packetData[1] == GSM_CC_MESSAGE_TYPE_SETUP ){
                        // printf(" SETUP: MT\n");
                        dtapMsgDataSize = sizeof( GsmMtSetupMsg );
                    } // MT SETUP MSG from MSC //
                    else if (packetData[1] == GSM_CC_MESSAGE_TYPE_ALERTING ){
                        // For Call Originating MS
                        // printf(" ALERTING\n");
                        dtapMsgDataSize = sizeof( GsmMoAlertingMsg );
                    }
                    else if (packetData[1] == GSM_CC_MESSAGE_TYPE_CONNECT ){
                        // Towards Call Originating MS
                        // printf(" CONNECT: MO\n");
                        dtapMsgDataSize = sizeof( GsmMoConnectMsg );
                    }
                    else if (packetData[1] ==
                                GSM_CC_MESSAGE_TYPE_CONNECT_ACKNOWLEDGE ){
                        // Towards Call Terminating MS
                        // printf(" CONNECT_ACKNOWLEDGE: MT\n");
                        dtapMsgDataSize = sizeof( GsmConnectAcknowledgeMsg );
                        msSlotInfo->isUsedForTraffic = TRUE;
                    }
                    else if (packetData[1] == GSM_CC_MESSAGE_TYPE_RELEASE ){
                        unsigned char*  relData;


                        // printf(" RELEASE\n");
                        dtapMsgDataSize = sizeof( GsmReleaseByNetworkMsg ) +
                                              1;

                        // Need to send this message with steal bit set
                        relData = packetData;
                        packetData = (
                            unsigned char* )MEM_malloc( dtapMsgDataSize );
                        packetData[0] = TRUE;
                        memcpy( packetData + 1,
                            relData,
                            sizeof( GsmReleaseByNetworkMsg ) );
                        flag = TRUE;
                    }
                    else if (packetData[1] ==
                                GSM_CC_MESSAGE_TYPE_DISCONNECT ){
                        unsigned char*  disconnectMsg;


                        // printf(" DISCONNECT\n");
                        dtapMsgDataSize = sizeof(
                            GsmDisconnectByNetworkMsg ) + 1;

                        disconnectMsg = packetData;
                        packetData = (
                            unsigned char* )MEM_malloc( dtapMsgDataSize );

                        // Need to send this message with steal bit set
                        packetData[0] = TRUE;
                        memcpy( packetData + 1,
                            disconnectMsg,
                            sizeof( GsmDisconnectByNetworkMsg ) );
                        flag = TRUE;
                    }
                    else if (packetData[1] ==
                                GSM_CC_MESSAGE_TYPE_RELEASE_COMPLETE ){
                        unsigned char*  relMsg;


                        // printf(" RELEASE_COMPLETE from MSC \n");

                        dtapMsgDataSize = sizeof(
                            GsmReleaseCompleteByNetworkMsg ) + 1;
                        relMsg = packetData;
                        packetData = (
                            unsigned char* )MEM_malloc( dtapMsgDataSize );
                        // Need to send this message with steal bit set
                        packetData[0] = TRUE;
                        memcpy( packetData + 1,
                            relMsg,
                            sizeof( GsmReleaseCompleteByNetworkMsg ) );
                        flag = TRUE;
                    }
                    else{
                        sprintf( errorString,
                            "GSMLayer3 BS node %d: "
                                "Unknown message 0x%x received ",
                            node->nodeId,
                            packetData[1] );
                        ERROR_ReportWarning( errorString );
                        return;
                    }
                }
                break;

            default:
                {
                    sprintf( errorString,
                        "GSMLayer3 BS[%d]: "
                            "Unknown protocol discriminator 0x%x received",
                        node->nodeId,
                        packetData[0] );
                    ERROR_ReportWarning( errorString );
                }
        }; // end of switch for MM & CC messages //

        // Rebuild the Message data to send over the air/radio
        // to the MS related to the sccpConnectionId

        newMsg = MESSAGE_Alloc( node,NETWORK_LAYER,0,MSG_MAC_FromNetwork );

        MESSAGE_PacketAlloc( node,newMsg,dtapMsgDataSize,TRACE_GSM );

        newMsgData = ( unsigned char* )MESSAGE_ReturnPacket( newMsg );
        memcpy( newMsgData,packetData,dtapMsgDataSize );
        if (flag == TRUE)
        {
            MEM_free(packetData);
        }
        GsmLayer3SendPacket( node,
            newMsg,
            msSlotInfo->slotNumber,
            msSlotInfo->dlChannelIndex,
            GSM_CHANNEL_SDCCH ); // or TCH
    } // End of DTAP messages //
} // End of GsmLayer3BsProcessAInterfaceMsgs //


void GsmLayer3ReceivePacketFromGsmAInterface(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress)
{

    GsmLayer3Data*  nwGsmData;


    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    switch ( nwGsmData->nodeType ){
        case GSM_MSC:
            {
                GsmLayer3MscProcessAInterfaceMsgs( node,msg,sourceAddress );
            }
            break;

        case GSM_BS:
            {
                GsmLayer3BsProcessAInterfaceMsgs( node,msg,sourceAddress );
            } // End of processing  BS msgs //
            break;

        default:
            {
                ERROR_Assert( FALSE,"Function: \
                    GsmLayer3ReceivePacketFromGsmAInterface \
                    is valid for only GSM Node BS and MSC" );
            }
    } // End of switch //

    //MESSAGE_Free(node, msg);
} // End of GsmLayer3ReceivePacketFromGsmAInterface //


/* Name:        GsmLayer3SendPacketViaGsmAInterface
 * Purpose:     Used to transfer signalling packets
 *              on the GSM 'A' interface between the BSS & MSC.
 */
void GsmLayer3SendPacketViaGsmAInterface(
    Node* node,
    NodeAddress         destNodeAddress,
    NodeAddress         sourceNodeId,
    int                 sccpConnectionId,
    GsmSccpRoutingLabel routingLabel,
    unsigned char       messageTypeCode,
    unsigned char* msgData,
    int                 msgDataSize)
{
    GsmLayer3Data*  nwGsmData;
    Message*        msg;

    unsigned char*  packetData;


    ERROR_Assert( msgDataSize > 0,
        "GsmLayer3SendPacketViaGsmAInterface: msgDataSize <= 0" );

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    msg = MESSAGE_Alloc( node,
              NETWORK_LAYER,
              NETWORK_PROTOCOL_IP,
              MSG_NETWORK_FromTransportOrRoutingProtocol );

    MESSAGE_PacketAlloc( node,msg,msgDataSize,TRACE_GSM );

    packetData = ( unsigned char* )MESSAGE_ReturnPacket( msg );

    memcpy( packetData,msgData,msgDataSize );

    GsmLayer3AddSccpHeader( node,
        msg,
        sccpConnectionId,
        sourceNodeId,
        &routingLabel,
        messageTypeCode );



    GsmLayer3SendMsgOverIp( node,destNodeAddress,msg );
} // End of GsmLayer3SendPacketViaGsmAInterface //
