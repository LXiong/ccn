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

/*
 *
 * Layer 2 Protocol for GSM
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "cellular_gsm.h"
#include "phy_gsm.h"
#include "mac_gsm.h"
#include "layer3_gsm.h"
#include "network_ip.h"
#include "phy.h"

#ifdef PARALLEL //Parallel
#include "parallel.h"
#endif //endParallel

#define DEBUG 0

// Lower value means more debug information.

#define DEBUG_GSM_MAC_0 0
#define DEBUG_GSM_MAC_1 0
#define DEBUG_GSM_MAC_2 0
#define DEBUG_GSM_MAC_3 0


static const uddtChannelIndex   DefaultGsmChannelIndex  = 0;
static const uddtChannelIndex   GsmInvalidChannelIndex  = -1;

// Reference: GSM 05.08 Section 8.4
// Indexed by [assignedSlotNumber][iteration: 0-3]
const int                       GsmSacchMsgFrame[8][4]  =
{
    {12, 38, 64, 90 },  // slot 0
    {25, 51, 77, 103},  // slot 1
    {38, 64, 90, 12 },  // slot 2
    {51, 77, 103, 25},  // slot 3
    {64, 90, 12, 38 },  // slot 4
    {77, 103, 25, 51},  // slot 5
    {90, 12, 38, 64 },  // slot 6
    {103, 25, 51, 77}
}; // slot 7

// Idle TCH slot frames can be derived from the SACCH blocks
// by adding 13 and taking the modulo 104 value.
// Indexed by [assignedSlotNumber][iteration: 0-3]
const int                       GsmIdleTchFrame[8][4]   =
{
    {25, 51, 77, 103},  // slot 0
    {38, 64, 90, 12 },  // slot 1
    {51, 77, 103, 25},  // slot 2
    {64, 90, 12, 38 },  // slot 3
    {77, 103, 25, 51},  // slot 4
    {90, 12, 38, 64 },  // slot 5
    {103, 25, 51, 77},  // slot 6
    {12, 38, 64, 90 }
}; // slot 7


// Measurement Reporting Period (start, end) for assigned slot.
// The start & end values are frameNumber modulo 104 values.
const int                       GsmMeasRepPeriod[8][2]  =
{
    {0, 103},   // slot 0
    {13, 12},   // slot 1
    {26, 25},   // slot 2
    {39, 38},   // slot 3
    {52, 51},   // slot 4
    {65, 64},   // slot 5
    {78, 77},   // slot 6
    {91, 90}
};  // slot 7


/* Static functions in this file */

static
Message* MacGsmStartTimer(
    Node* node,
    MacDataGsm* gsm,
    int         timerType,
    clocktype   delay)
{
    Message*                timerMsg;


    timerMsg = MESSAGE_Alloc( node,MAC_LAYER,0,timerType );

    MESSAGE_SetInstanceId( timerMsg,
        ( short )gsm->myMacData->interfaceIndex );
    MESSAGE_Send( node,timerMsg,delay );

    return timerMsg;
} // End of MacGsmStartTimer //


/*
 * FUNCTION:   MacGsmStopListeningToChannel
 *
 * PURPOSE:    Causes node to stop listening on specified channel
 */

static
void MacGsmStopListeningToChannel(
    Node* node,
    MacDataGsm* gsm,
    int         phyNumber,
    uddtChannelIndex       channelIndex)
{
    BOOL    phyIsListening  = FALSE;


    phyIsListening = PHY_IsListeningToChannel( node,phyNumber,channelIndex );

    if (phyIsListening == TRUE ){
        PHY_StopListeningToChannel( node,phyNumber,channelIndex );
    }
} // End of MacGsmStopListeningToChannel //

/*
 * FUNCTION:   MacGsmMsStartListeningToBcchListChannels
 *
 * PURPOSE:    Causes MS to start listening on the specified
 *             list of BCCH channels given in the config file.
 */
static
void MacGsmMsStartListeningToBcchListChannels(
    Node* node,
    MacDataGsm* gsm)
{
    BOOL    phyIsListening;
    int     i;
    // Bug # 3436 fix - start
    // Listen to control channels (BA-BCCH list) for cell selection
    if (gsm->msInfo->isDedicatedChannelAssigned == FALSE )
    {
        for (i = 0; i < gsm->msInfo->numBcchChannels; i++ )
        {
            phyIsListening = PHY_IsListeningToChannel( node,
                             gsm->myMacData->phyNumber,
                             gsm->msInfo->bcchList[i].channelIndex );

            if (phyIsListening == FALSE )
            {
                PHY_StartListeningToChannel( node,
                    gsm->myMacData->phyNumber,
                    gsm->msInfo->bcchList[i].channelIndex );
            }
        }
    }
    else
    {
        int indexToListen = 0;
        int lastListen = -1;
        if (gsm->msInfo->isDedicatedChannelAssigned &&
             gsm->msInfo->assignedChannelType == GSM_CHANNEL_TCH )
        {
            MacGsmStopListeningToChannel(
                                    node,
                                    gsm,
                                    gsm->myMacData->phyNumber,
                                    gsm->msInfo->assignedChannelIndex - 1 );
        }
        for (i = 0; i < gsm->msInfo->numBcchChannels; i++ )
        {
            if (gsm->msInfo->bcchList[i].listeningStartTime != 0 )
            {
                if (( node->getNodeTime() - 
                       gsm->msInfo->bcchList[i].listeningStartTime )
                       < 2080* (DefaultGsmSlotDuration) && 
                       gsm->msInfo->bcchList[i].isSchDetected == FALSE )
                {
                    if (( gsm->msInfo->idleFramePassed == FALSE) ||
                         ( gsm->msInfo->idleFramePassed == TRUE &&
                           gsm->msInfo->bcchList[i].lastUpdatedAt > 
                           gsm->msInfo->bcchList[i].listeningStartTime ) )
                    {
                        // Listen this channel
                        phyIsListening = PHY_IsListeningToChannel(
                                    node,
                                    gsm->myMacData->phyNumber,
                                    gsm->msInfo->bcchList[i].channelIndex );

                        if (phyIsListening == FALSE )
                        {
                            PHY_StartListeningToChannel(
                                node,
                                gsm->myMacData->phyNumber,
                                gsm->msInfo->bcchList[i].channelIndex );
                        }
                        gsm->msInfo->bcchList[i].numSlotsListened++;
                        if (gsm->msInfo->idleFramePassed == TRUE )
                        {
                            gsm->msInfo->bcchList[i].listeningStartTime = 
                                    node->getNodeTime();
                        }
                        gsm->msInfo->idleFramePassed = FALSE;
                        return;
                    }
                    else
                    {
                         gsm->msInfo->bcchList[i].listeningStartTime = 0;
                         lastListen = i;
                    }
                }
                else
                {
                    gsm->msInfo->bcchList[i].listeningStartTime = 0;
                    lastListen = i;
                }
            }
        }
        if (lastListen >= 0 && 
             ( lastListen +1 ) < gsm->msInfo->numBcchChannels )
        {
            indexToListen = lastListen + 1;
            gsm->msInfo->bcchList[lastListen].numSlotsListened = 0;
        }
        gsm->msInfo->bcchList[indexToListen].numSlotsListened = 1;
        gsm->msInfo->idleFramePassed = FALSE;
        // Listen this channel
        gsm->msInfo->bcchList[indexToListen].listeningStartTime = 
                                                            node->getNodeTime();
        phyIsListening = PHY_IsListeningToChannel(
                        node,
                        gsm->myMacData->phyNumber,
                        gsm->msInfo->bcchList[indexToListen].channelIndex );

        if (phyIsListening == FALSE )
        {
             PHY_StartListeningToChannel(
                    node,
                    gsm->myMacData->phyNumber,
                    gsm->msInfo->bcchList[indexToListen].channelIndex );
        }
    }
    // Bug # 3436 fix - end
} // End of MacGsmMsStartListeningToBcchListChannels


/*
 * FUNCTION:    MacGsmBsGetPhyNumberFromChannel
 * PURPOSE:     Find the base station's physical device number
 *              to be used for a given channel.
 */

static
int MacGsmBsGetPhyNumberFromChannel(
    Node* node,
    MacGsmBsInfo* bsInfo,
    uddtChannelIndex             channelIndex)
{
    if (channelIndex < bsInfo->channelIndexStart ||
           channelIndex > bsInfo->channelIndexEnd ){
        char    errorString[MAX_STRING_LENGTH];
        sprintf( errorString,
            "Mac_GSM BS[%d]: ChannelIndex %d out of range\n",
            node->nodeId,
            channelIndex );
        ERROR_ReportError( errorString );
    }
    return( channelIndex - bsInfo->channelIndexStart );
} // End of MacGsmBsGetPhyNumberFromChannel //


/*
 * FUNCTION:  MacGsmGetUpLinkSlotNumber
 *
 * PURPOSE:   Returns the 3 time slot delayed value.
 *            Used by MS to delay its transmission by 3 time slots
 *            from the assigned slot.
 */

static
short MacGsmGetUpLinkSlotNumber(
    uddtSlotNumber assignedDlSlotNumber)
{
    if (assignedDlSlotNumber != GSM_INVALID_SLOT_NUMBER ){
        if (assignedDlSlotNumber + 3 > 7 ){
            return ( short )( assignedDlSlotNumber - 5 );
        }
        else{
            return ( short )( assignedDlSlotNumber + 3 );
        }
    }
    return GSM_INVALID_SLOT_NUMBER;
} // End of MacGsmGetUpLinkSlotNumber //


/*
 * FUNCTION:  MacGsmGetReverseUpLinkSlotNumber
 *
 * PURPOSE:   Given a transmission slot, find its assigned slot number.
 *            Used by MS to determine if anything is to be transmitted
 *            in the current timeslot.
 */

static
short MacGsmGetReverseUpLinkSlotNumber(
    uddtSlotNumber slotNumber)
{
    if (slotNumber != GSM_INVALID_SLOT_NUMBER ){
        if (slotNumber - 3 >= 0 ){
            return ( short )( slotNumber - 3 );
        }
        else{
            return ( short )( slotNumber + 5 );
        }
    }
    return GSM_INVALID_SLOT_NUMBER;
} // End of MacGsmGetReverseUpLinkSlotNumber //

/*
 * FUNCTION:  GsmMacSendMessageOverPhy
 *
 * PURPOSE:   Sends the provided packet/message over the physical
 *            interface on the specified channel.
 */

static
void GsmMacSendMessageOverPhy(
    Node* node,
    Message* msg,
    int         phyNumber,
    uddtChannelIndex       channelIndex,
    clocktype   allowedTransmissionDuration)
{
    clocktype   transmissionDelay;


    PHY_SetTransmissionChannel( node,phyNumber,channelIndex );

    transmissionDelay = PHY_GetTransmissionDelay( node,
                            phyNumber,
                            MESSAGE_ReturnPacketSize( msg ) );

    ERROR_Assert( transmissionDelay <= allowedTransmissionDuration,
        "GSM_MAC: Message size is greater than allowed duration\n" );


    PHY_StartTransmittingSignal( node,phyNumber,msg,FALSE,0 );
} // End of GsmMacSendMessageOverPhy //

/*
 * FUNCTION:  GsmMacBsBuildSysInfoType3Msg
 * PURPOSE:   Creates the SYSTEM INFORMATION TYPE 3 message and
 *            saves a pointer for later use.
 */
static
void GsmMacBsBuildSysInfoType3Msg(
    Node* node,
    MacDataGsm* gsm)
{
    Message*            msg;
    GsmSysInfoType3Msg* sysInfoType3Msg;


    msg = MESSAGE_Alloc( node,MAC_LAYER,0,MSG_MAC_StartTransmission );

    MESSAGE_PacketAlloc( node,msg,sizeof( GsmSysInfoType3Msg ),TRACE_GSM );

    sysInfoType3Msg = ( GsmSysInfoType3Msg * )MESSAGE_ReturnPacket( msg );

    sysInfoType3Msg->protocolDiscriminator = GSM_PD_RR;
    sysInfoType3Msg->messageType =
    GSM_RR_MESSAGE_TYPE_SYSTEM_INFORMATION_TYPE3;


    sysInfoType3Msg->cellIdentity = returnGSMNodeCellId( node );
    sysInfoType3Msg->lac = returnGSMNodeLAC( node );

    // Synchronization info is set before sending the msg

    // Set a pointer for repeated use
    gsm->bsInfo->sysInfoType3Msg = msg;
} // End of GsmMacBsBuildSysInfoType3Msg //


/*
 * FUNCTION:  GsmMacBsSendSysInfoType3Msg
 *
 * PURPOSE:   Sent by a Base Station (BS) to all MS's within
 *            range listening on downlink control channel channel
 */
static
void GsmMacBsSendSysInfoType3Msg(
    Node* node,
    MacDataGsm* gsm)
{
    Message*            msg;
    GsmSysInfoType3Msg* sysInfoType3Msg;


    msg = MESSAGE_Duplicate( node,gsm->bsInfo->sysInfoType3Msg );

    sysInfoType3Msg = ( GsmSysInfoType3Msg * )MESSAGE_ReturnPacket( msg );


    GsmMacSendMessageOverPhy( node,
        msg,
        MacGsmBsGetPhyNumberFromChannel( node,
            gsm->bsInfo,
            gsm->bsInfo->downLinkControlChannelIndex ),
        gsm->bsInfo->downLinkControlChannelIndex,
        DefaultGsmSlotDuration );
} // End of GsmMacBsSendSysInfoType3Msg //


/*
 * FUNCTION:   GsmMacBsBuildSysInfoType2Msg
 *
 * PURPOSE:    Builds the SYSTEM INFORMATION TYPE 2 message and saves
 *             a pointer in the MsInfo struct. Provides neighbouring
 *             cell information.
 */
static
void GsmMacBsBuildSysInfoType2Msg(
    Node* node,
    MacDataGsm* gsm)
{
    int                 i;
    Message*            msg;
    GsmSysInfoType2Msg* sysInfoType2Msg;


    msg = MESSAGE_Alloc( node,MAC_LAYER,0,MSG_MAC_StartTransmission );

    MESSAGE_PacketAlloc( node,
        msg,
        sizeof( GsmSysInfoType2Msg ) + gsm->bsInfo->numNeighBcchChannels - 1,
        TRACE_GSM );

    sysInfoType2Msg = ( GsmSysInfoType2Msg * )MESSAGE_ReturnPacket( msg );

    sysInfoType2Msg->protocolDiscriminator = GSM_PD_RR;
    sysInfoType2Msg->messageType =
    GSM_RR_MESSAGE_TYPE_SYSTEM_INFORMATION_TYPE2;

    sysInfoType2Msg->txInteger = GSM_TX_INTEGER;
    sysInfoType2Msg->maxReTrans = GSM_MAX_RETRANS;

    // Neighbour cell BCCH information
    sysInfoType2Msg->numNeighBcchChannels = (
        unsigned char ) ( gsm->bsInfo->numNeighBcchChannels );

    for (i = 0; i < gsm->bsInfo->numNeighBcchChannels; i++ ){
        sysInfoType2Msg->neighBcchChannels[i] = (
            unsigned char ) gsm->bsInfo->neighBcchChannels[i];
    }

    // Set a pointer for repeated use
    gsm->bsInfo->sysInfoType2Msg = msg;
} // End of GsmMacBsBuildSysInfoType2Msg //


/*
 * FUNCTION:   GsmMacBsBuildDummyBurstMsg
 *
 * PURPOSE:    Pre-build the Dummy burst message and save a pointer
 */
static
void GsmMacBsBuildDummyBurstMsg(
    Node* node,
    MacDataGsm* gsm)
{
    Message*            msg;
    GsmDummyBurstMsg*   dBMsg;


    msg = MESSAGE_Alloc( node,MAC_LAYER,0,MSG_MAC_StartTransmission );

    MESSAGE_PacketAlloc( node,msg,sizeof( GsmDummyBurstMsg ),TRACE_GSM );

    dBMsg = ( GsmDummyBurstMsg * )MESSAGE_ReturnPacket( msg );

    memset( dBMsg,GSM_DUMMY_BURST_OCTET,GSM_BURST_SIZE );

    gsm->bsInfo->dummyBurstMsg = msg;
} // End of GsmMacBsBuildDummyBurstMsg //


/*
 * FUNCTION:   GsmMacBsSendDummyBurstMsg
 *
 * PURPOSE:    BS's send dummy burst messages when not transmitting
 *             anything.
 */
static
void GsmMacBsSendDummyBurstMsg(
    Node* node,
    MacDataGsm* gsm)
{
    Message*                msg;

    msg = MESSAGE_Duplicate( node,gsm->bsInfo->dummyBurstMsg );
    GsmMacSendMessageOverPhy( node,
        msg,
        MacGsmBsGetPhyNumberFromChannel( node,
            gsm->bsInfo,
            gsm->bsInfo->downLinkControlChannelIndex ),
        gsm->bsInfo->downLinkControlChannelIndex,
        DefaultGsmSlotDuration );
} // End of GsmMacBsSendDummyBurstMsg //


/*
 * FUNCTION:   GsmMacMsSendMeasurementReportMsg
 *
 * PURPOSE:    MS's send MEASUREMENT REPORTs at fixed intervals
 *             while on a dedicated channel.
 */
static
void  GsmMacMsSendMeasurementReportMsg(
    Node* node,
    MacDataGsm* gsm)
{
    char                    clockStr[MAX_STRING_LENGTH];
    int                     i;
    short                   numNeighCells   = 0;

    Message*                msg;
    MacGsmMsInfo*           msInfo          = gsm->msInfo;
    GsmMeasurementReportMsg*                                measReport;



    ctoa( node->getNodeTime(),clockStr );

    msg = MESSAGE_Alloc( node,MAC_LAYER,0,MSG_MAC_StartTransmission );

    MESSAGE_PacketAlloc( node,
        msg,
        sizeof( GsmMeasurementReportMsg ),
        TRACE_GSM );

    measReport = ( GsmMeasurementReportMsg * )MESSAGE_ReturnPacket( msg );
    memset( measReport,0,sizeof( GsmMeasurementReportMsg ) );

    measReport->protocolDiscriminator = GSM_PD_RR;
    measReport->messageType = GSM_RR_MESSAGE_TYPE_MEASUREMENT_REPORT;

    measReport->rxLevFullServingCell = GsmMapRxLev(
        msInfo->assignedChRxLevel_dBm / msInfo->numAssignedChMeasurements );

    measReport->rxQualFullServingCell = GsmMapRxQual(
        msInfo->assignedChBer / msInfo->numAssignedChMeasurements );


    if ((msInfo->sendSChMeasFailure == FALSE) &&
            msInfo->assignedChMeasLastUpdatedAt + 10 * SECOND <
                node->getNodeTime() ){
        Message*                    callEndTimerMsg;

        if (DEBUG_GSM_MAC_2 ){
            printf( "MS [%d] Outdated Serving cell values\n",node->nodeId );
        }

        msInfo->assignedChRxLevel_dBm = 0.0;
        msInfo->assignedChBer = 0.0;
        msInfo->numAssignedChMeasurements = 0;

        // free the MS here, due to radio link failure,
        //  Not received signal info since last five seconds
        // Set a timer to initiate call disconnect

        callEndTimerMsg = MESSAGE_Alloc( node,
                              NETWORK_LAYER,
                              NETWORK_PROTOCOL_GSM,
                              MSG_NETWORK_GsmCallEndTimer );

        callEndTimerMsg->protocolType = NETWORK_PROTOCOL_GSM;

        MESSAGE_Send( node,callEndTimerMsg,( 1 * MILLI_SECOND ) );

        msInfo->sendSChMeasFailure = TRUE;
        return;
    }

    if (DEBUG_GSM_MAC_1 ){
        printf( "MS [%d] MEASUREMENTS: ServCH %d RxLev %d (%f), "
                    "RxQual %d (%e)\n",
            node->nodeId,
            msInfo->downLinkControlChannelIndex,
            measReport->rxLevFullServingCell,
            msInfo->assignedChRxLevel_dBm /
            msInfo->numAssignedChMeasurements,
            measReport->rxQualFullServingCell,
            msInfo->assignedChBer / msInfo->numAssignedChMeasurements );
    }

    for (i = 0; i < msInfo->numBcchChannels; i++ ){
        // Should be of 6 strongest carriers in last 5 seconds
        if (msInfo->bcchList[i].isNeighChannel == TRUE ){
            if (msInfo->bcchList[i].lastUpdatedAt + 5 * SECOND <
                   node->getNodeTime() ){
                msInfo->bcchList[i].rxLevel_dBm = 0.0;
                msInfo->bcchList[i].ber = 0.0;
                msInfo->bcchList[i].C1 = 0.0;
                msInfo->bcchList[i].numRecorded = 0;
                msInfo->bcchList[i].lastUpdatedAt = 0;
                // Bug # 3436 fix - start
                continue;
                // Bug # 3436 fix - end
            }
            else{

                if (msInfo->bcchList[i].numRecorded == 0){
                    continue;
                }
                measReport->neighCellChannel[numNeighCells] = (
                    unsigned char )msInfo->bcchList[i].channelIndex;
                measReport->rxLevNeighCell[numNeighCells] = GsmMapRxLev(
                    msInfo->bcchList[i].rxLevel_dBm /
                    msInfo->bcchList[i].numRecorded );

                if (DEBUG_GSM_MAC_1 ){
                    printf( "\tN.CH %d, RxLev %d (%f)\n",
                        measReport->neighCellChannel[numNeighCells],
                        measReport->rxLevNeighCell[numNeighCells],
                        msInfo->bcchList[i].rxLevel_dBm /
                        msInfo->bcchList[i].numRecorded );
                }
            }

            // Reset measured values before the next reporting period
            if (msInfo->sacchBlockNumber == 3 ) // 0-3: 4 x 26 = 104 frames
            {
                msInfo->bcchList[i].rxLevel_dBm = 0.0;
                msInfo->bcchList[i].ber = 0.0;
                msInfo->bcchList[i].C1 = 0.0;
                msInfo->bcchList[i].numRecorded = 0;
            }
            numNeighCells++;
        }
    } // For neighbour cells //

    measReport->numNeighCells = ( unsigned char )numNeighCells;

    GsmMacSendMessageOverPhy( node,
        msg,
        gsm->myMacData->phyNumber,
        gsm->msInfo->assignedChannelIndex,
        DefaultGsmSlotDuration );

    // Bug # 3436 fix - start
    msInfo->supportFlag = TRUE;
    // Bug # 3436 fix - end
} // End of GsmMacMsSendMeasurementReportMsg //


/*
 * FUNCTION:   MacGsmStartListeningToChannel
 *
 * PURPOSE:    Allows node to start listening on specified channel
 */

void MacGsmStartListeningToChannel(
    Node* node,
    MacDataGsm* gsm,
    int         phyNumber,
    uddtChannelIndex       channelIndex)
{
    BOOL    phyIsListening  = TRUE;


    // ARFCN 0 => channel 0 for downlink, 1 for uplink
    // So MS listens to these channels
    //       1 => channel 2  "      "   , 3  "     "

    phyIsListening = PHY_IsListeningToChannel( node,phyNumber,channelIndex );

    if (phyIsListening == FALSE ){
        PHY_StartListeningToChannel( node,phyNumber,channelIndex );
    }
} // End of MacGsmStartListeningToChannel //


/*
 * FUNCTION:   MacGsmMsStopListeningToBcchListChannels
 *
 * PURPOSE:    Causes MS to stop listening on the specified
 *             list of BCCH channels given in the config file.
 */
static
void MacGsmMsStopListeningToBcchListChannels(
    Node* node,
    MacDataGsm* gsm)
{
    int i;

    // Listen to control channels (BA-BCCH list) for cell selection
    for (i = 0; i < gsm->msInfo->numBcchChannels; i++ ){
        MacGsmStopListeningToChannel( node,
            gsm,
            gsm->myMacData->phyNumber,
            gsm->msInfo->bcchList[i].channelIndex );
    }
} // End of MacGsmMsStopListeningToBcchListChannels //


static
BOOL MacGsmIsFcchMsg(
    unsigned char* msgContent)
{
    unsigned char   fcchBurst[GSM_BURST_SIZE];


    memset( fcchBurst,0,GSM_BURST_SIZE );

    if (memcmp( msgContent,fcchBurst,GSM_BURST_SIZE ) == 0 ){
        return TRUE;
    }

    return FALSE;
} // MacGsmIsFcchMsg //


static
void MacGsmBuildFcchMsg(
    Node* node,
    MacDataGsm* gsm)
{
    Message*        msg;
    unsigned char*  burst;

    msg = MESSAGE_Alloc( node,MAC_LAYER,0,MSG_MAC_StartTransmission );

    MESSAGE_PacketAlloc( node,msg,GSM_BURST_SIZE,TRACE_GSM );

    burst = ( unsigned char* )MESSAGE_ReturnPacket( msg );

    memset( burst,0,GSM_BURST_SIZE );

    gsm->bsInfo->fcchMsg = msg;
} // MacGsmBuildFcchMsg //


static
void MacGsmBsSendFcchMsg(
    Node* node,
    MacDataGsm* gsm)
{
    Message*                fcchMsg;

    if (DEBUG_GSM_MAC_0 ){
        printf( "BS[%d]: send FCCH at TS%d, fr %d\n",
            node->nodeId,
            gsm->bsInfo->slotNumber,
            gsm->bsInfo->frameNumber );
    }

    fcchMsg = MESSAGE_Duplicate( node,gsm->bsInfo->fcchMsg );
    GsmMacSendMessageOverPhy( node,
        fcchMsg,
        MacGsmBsGetPhyNumberFromChannel( node,
            gsm->bsInfo,
            gsm->bsInfo->downLinkControlChannelIndex ),
        gsm->bsInfo->downLinkControlChannelIndex,
        DefaultGsmSlotDuration );
} // MacGsmBsSendFcchMsg //


/*
 * FUNCTION:   MacGsmIsBcchFcchSchSlot
 *
 * PURPOSE:    Checks if current slot & frame combination is
 *             a BCCH, FCCH or SCH channel slot.
 */

static
BOOL MacGsmIsBcchFcchSchSlot(
    uddtSlotNumber  slotNumber,
    uddtFrameNumber controlFrameNumber)
{
    if (slotNumber == 0 &&
           (
           (
        controlFrameNumber >=
        2 && controlFrameNumber <= 5 ) ||    // 2-5: BCCH
        controlFrameNumber % 10 == 0 ||    // 0, 10, 20,...50 : FCCH
        controlFrameNumber % 10 == 1 )      // 1, 11, 21,...41 : SCH
        ){
        return TRUE;
    }

    return FALSE;
} // End of MacGsmIsBcchFcchSchSlot //


/*
 * FUNCTION:   MacGsmIsBcchSlot
 *
 * PURPOSE:    Checks if current slot & frame combination is
 *             a BCCH channel slot.
 */

static
BOOL MacGsmIsBcchSlot(
    uddtSlotNumber   slotNumber,
    uddtFrameNumber   controlFrameNumber)
{
    // Control Frames (51 multiframe: 0-50) 2, 3, 4, 5: BCCH
    if (slotNumber == 0 &&
           ( controlFrameNumber >= 2 && controlFrameNumber <= 5 ) ){
        return TRUE;
    }

    return FALSE;
} // End of MacGsmIsBcchSlot //


/*
 * FUNCTION:   MacGsmIsSchSlot
 *
 * PURPOSE:    Checks if current slot & frame combination is
 *             a SCH channel slot.
 */

static
BOOL MacGsmIsSchSlot(
    uddtSlotNumber   slotNumber,
    uddtFrameNumber   controlFrameNumber)
{
    // Control Frames (51 multiframe: 0-50) 1, 11, 21, 31, 41
    if (slotNumber == 0 && controlFrameNumber % 10 == 1 ){
        return TRUE;
    }

    return FALSE;
} // End of MacGsmIsSchSlot //


// Only for BS's
static
void MacGsmBsSendSchMsg(
    Node* node,
    MacDataGsm* gsm)
{
    Message*        msg;
    GsmSchMessage*  schMsg;


    msg = MESSAGE_Alloc( node,MAC_LAYER,0,MSG_MAC_StartTransmission );

    MESSAGE_PacketAlloc( node,msg,sizeof( GsmSchMessage ),TRACE_GSM );

    schMsg = ( GsmSchMessage * )MESSAGE_ReturnPacket( msg );

    if (DEBUG_GSM_MAC_0 ){
        printf( "BS[%d]: send SCH at TS%d, fr %d\n",
            node->nodeId,
            gsm->bsInfo->slotNumber,
            gsm->bsInfo->frameNumber );
    }

    schMsg->frameNumber = gsm->bsInfo->frameNumber;

    schMsg->startTimeOfFrame = node->getNodeTime();

    GsmMacSendMessageOverPhy( node,
        msg,
        MacGsmBsGetPhyNumberFromChannel( node,
            gsm->bsInfo,
            gsm->bsInfo->downLinkControlChannelIndex ),
        gsm->bsInfo->downLinkControlChannelIndex,
        DefaultGsmSlotDuration );
} // End of MacGsmBsSendSchMsg //


/*
 * FUNCTION:   MacGsmIsFcchSlot
 *
 * PURPOSE:    Checks if current slot & frame combination is
 *             a FCCH channel slot.
 */

static
BOOL MacGsmIsFcchSlot(
    uddtSlotNumber   slotNumber,
    uddtFrameNumber   controlFrameNumber)
{
    // Control Frames (51 multiframe: 0-50 ) 0, 10, 20, 30, 40
    if (slotNumber == 0 && controlFrameNumber % 10 == 0 &&
           controlFrameNumber != 50 ){
        return TRUE;
    }

    return FALSE;
} // End of MacGsmIsFcchSlot //


/*
 * FUNCTION:  MacGsmIsPagchSlot
 *
 * PURPOSE:   Checks if current slot & frame combination is
 *            a FCCH channel slot.
 */

static
BOOL MacGsmIsPagchSlot(
    uddtSlotNumber slotNumber,
    uddtFrameNumber controlFrameNumber)
{
    if (( slotNumber == 0 ) &&
           ( ( controlFrameNumber >= 6 &&     // 6-9: 4 slots
             controlFrameNumber <=
                 9 ) ||
               (
               controlFrameNumber >=
        12 && controlFrameNumber <= 19 ) ||   // 12-19: 8 slots
        ( controlFrameNumber >= 32 &&    // 32-39: 8 slots
        controlFrameNumber <=
            39 ) || ( controlFrameNumber >= 42 &&    // 42-49: 8 slots
            controlFrameNumber <= 49 ) ) ){
        return TRUE;
    }

    return FALSE;
} // End of MacGsmIsPagchSlot //


/*
 * FUNCTION:  MacGsmIsSacchMsgFrame
 *
 * PURPOSE:   Checks if current frame is the Sacch Message i.e.
 *            reporting frame for the given time slot.
 *
 * REFERENCE: GSM 05.08 Section 8.1, 8.4
 */

BOOL MacGsmIsSacchMsgFrame(
    uddtSlotNumber   assignedSlotNumber,
    uddtFrameNumber     frameNumber)
{
    int i;

    for (i = 0; i < 4; i++ ){
        if (GsmSacchMsgFrame[assignedSlotNumber][i] ==
               ( (int)frameNumber % GSM_SACCH_MULTIFRAME_SIZE ) ){
            return TRUE;
        }
    }
    return FALSE;
} // End of MacGsmIsSacchMsgFrame //


/*
 * FUNCTION:  MacGsmIsCurrentTchFrameWithIdleSlot
 *
 * PURPOSE:   Checks if the current slot/frame is an 'idle'
 *            one. Called when an MS is on a dedicated channel.
 */

BOOL MacGsmIsCurrentTchFrameWithIdleSlot(
    uddtSlotNumber   assignedSlotNumber,
    uddtFrameNumber     frameNumber)
{
    int i;

    for (i = 0; i < 4; i++ ){
        if (GsmIdleTchFrame[assignedSlotNumber][i] == ( (int)frameNumber ) %
               GSM_SACCH_MULTIFRAME_SIZE ){
            return TRUE;
        }
    }
    return FALSE;
} // End of MacGsmIsCurrentTchFrameWithIdleSlot //


/*
 * FUNCTION:  MacGsmIsNextTchFrameWithIdleSlot
 *
 * PURPOSE:   Checks if the next slot/frame is an 'idle'
 *            one. Called when an MS is on a dedicated channel.
 */

BOOL MacGsmIsNextTchFrameWithIdleSlot(
    uddtSlotNumber   assignedSlotNumber,
    uddtFrameNumber  frameNumber)
{
    int i;

    for (i = 0; i < 4; i++ ){
        if (GsmIdleTchFrame[assignedSlotNumber][i] ==
            ( (int)frameNumber + 1 ) % GSM_SACCH_MULTIFRAME_SIZE ){
            return TRUE;
        }
    }
    return FALSE;
} // End of MacGsmIsNextTchFrameWithIdleSlot //


/*
 * FUNCTION:  MacGsmIsNotDownLinkCtrlSlot
 *
 * PURPOSE:   Checks if current slot/frame is one of the
 *            control channel slot/frames.
 */

static
BOOL MacGsmIsNotDownLinkCtrlSlot(
    uddtSlotNumber   slotNumber,
    uddtFrameNumber   controlFrameNumber)
{
    if (MacGsmIsBcchFcchSchSlot( slotNumber,
             controlFrameNumber ) == FALSE && MacGsmIsPagchSlot( slotNumber,
                                                  controlFrameNumber ) ==
           FALSE ){
        return TRUE;
    }
    else{
        return FALSE;
    }
} // End of MacGsmIsNotDownLinkCtrlSlot //


/* Name:        MacGsmInitializeSlotTimer
 *
 * PARAMETERS:  node - pointer to calling node's data
 *              gsm - pointer to calling node's GSM-MAC data
 *              frameStartTime - absolute start time of next frame

 * REFERENCE:   Set timers to wake MS/BS up at the begining of a frame
 *              (time slot 0)
 *
 * COMMENT:     BS: This function is called upon completion of intialization.
 *
 *              MS: This function is called as soon as a MS synchronizes
 *              (gets tied) with a BS, i.e. it can be called at any
 *              time, unlike the MacGsmUpdateSlotTimer function that
 *              is called subsequently at end of each time slot.
 */

static
void MacGsmInitializeSlotTimer(
    Node* node,
    MacDataGsm* gsm,
    clocktype   frameStartTime)
{
    char        errorString[MAX_STRING_LENGTH];
    char        clockStr[MAX_STRING_LENGTH];
    char        clockStr2[MAX_STRING_LENGTH];

    Message*    timerMsg;
    clocktype   startTime;
    clocktype   currentTime;


    startTime = frameStartTime;
    currentTime = node->getNodeTime();
    ctoa( currentTime,clockStr2 );

    if (DEBUG_GSM_MAC_2 ){
        printf( "Node [%d] Intial Timer: current time %s\n",
            node->nodeId,
            clockStr2 );
    }

    // should not happen

    if (frameStartTime < currentTime ){
        startTime = frameStartTime + DefaultGsmFrameDuration;

        sprintf( errorString,
            "Node [%d] ERROR, frame start time (%d) < current (%d)\n"
                "resetting new start time %d\n",
            node->nodeId,
            ( int )frameStartTime,
            ( int )currentTime,
            ( int )startTime );
        ERROR_ReportError( errorString );
    }


    timerMsg = MESSAGE_Alloc( node,MAC_LAYER,0,MSG_MAC_GsmSlotTimer );
    gsm->slotTimerMsg = timerMsg;

    MESSAGE_SetInstanceId( timerMsg,
        ( short )gsm->myMacData->interfaceIndex );

    MESSAGE_Send( node,timerMsg,( startTime - currentTime ) );

    if (DEBUG_GSM_MAC_3 ){
        ctoa( startTime,clockStr );
        ctoa( currentTime,clockStr2 );

        printf( "Node [%d] MacGsmInitializeSlotTimer:\n"
                    "\tTS0 starts at: %s, current time: %s\n",
            node->nodeId,
            clockStr,
            clockStr2 );
    }
} // End of MacGsmInitializeSlotTimer //


/* FUNCTION:    MacGsmUpdateSlotTimer
 * PURPOSE: Update the MS/BS time slot timers
 * ASSUMPTION:  This function is called at the end of a time slot
 *              to schedule the next one, i.e. it should not be called
 *              at any other time.
 */
static
void MacGsmUpdateSlotTimer(
    Node* node,
    MacDataGsm* gsm,
    Message* timerMsg)
{
    int     slot;

    char    clockStr[MAX_STRING_LENGTH];


    MESSAGE_Send( node,timerMsg,DefaultGsmSlotDuration );

    if (DEBUG_GSM_MAC_0 ){
        if (gsm->nodeType == GSM_MS ){
            slot = gsm->msInfo->slotNumber;
        }
        else if (gsm->nodeType == GSM_BS ){
            slot = gsm->bsInfo->slotNumber;
        }

        ctoa( DefaultGsmSlotDuration,clockStr );
        printf( "Node [%d]: MacGsmUpdateSlotTimer:\n"
                    "\tDelay for next slot:%s, slot %d\n",
            node->nodeId,
            clockStr,
            slot );
    }
} // End of MacGsmUpdateSlotTimer //


/***
* From GSM 05.08 Section 6.4
* All values are expressed in dBm.
* Compute The path loss criterion is satisfied if C1 > 0.
***/
static
double MacGsmComputeC1(
    MacGsmMsInfo* msInfo,
    GsmBcchListEntry* bcchInfo)
{
    double  A,
            B;

    A = ( bcchInfo->rxLevel_dBm / bcchInfo->numRecorded ) -
            msInfo->rxLevAccessMin_dBm;

    B = mapGsmPowerControlLevelToDbm( PHY_GSM_MS_TXPWR_MAX_CCH ) -
            PHY_GSM_P_dBm;
    return ( A - MAX( B,0.0 ) );
} // End of MacGsmComputeC1 //





static
BOOL MacGsmIsCellReselectionNeeded(
    Node* node,
    MacGsmMsInfo* msInfo)
{
    if (( msInfo->isCellSelected == TRUE ) ){
        if (MacGsmComputeC1( msInfo,
                 msInfo->controlChannelBcchEntry ) <=
               GSM_CellReselectionParameter_C2 ){
            return TRUE;
        }
    }
    return FALSE;
} // End of MacGsmIsCellReselectionNeeded //


static
BOOL MacGsmPerformCellSelection(
    Node* node,
    MacGsmMsInfo* msInfo,
    GsmBcchListEntry** retBcchInfo)
{
    int                 i;

    uddtChannelIndex    selectedChannelIndex    = -1;
    double              maxC1                   = 0.0;

    GsmBcchListEntry*   bcchInfo                = NULL;


    // Find max C1 value, higher than 0.0
    for (i = 0; i < msInfo->numBcchChannels; i++ ){
        bcchInfo = &( msInfo->bcchList[i] );
        if (bcchInfo->numRecorded >= 5 &&
               (
               bcchInfo->lastUpdatedAt +
            GSM_BCCH_REFRESH_PERIOD ) > node->getNodeTime() ){
            bcchInfo->C1 = MacGsmComputeC1( msInfo,bcchInfo );

            if (maxC1 < bcchInfo->C1 ){
                maxC1 = bcchInfo->C1;
                selectedChannelIndex = i;
            }
        }
    }

    if (selectedChannelIndex >= 0
            && maxC1 > GSM_CellReselectionParameter_C2 ){
        msInfo->bcchList[selectedChannelIndex].isSelected = TRUE;
        *retBcchInfo = &( msInfo->bcchList[selectedChannelIndex] );

        return TRUE;
    }

    *retBcchInfo = NULL;

    return FALSE;
} // MacGsmPerformCellSelection //


static
void MacGsmCheckForCellSelection(
    Node* node,
    MacDataGsm* gsm)
{
    char                clockStr[MAX_STRING_LENGTH];
    char                clockStr2[MAX_STRING_LENGTH];

    MacGsmMsInfo*       msInfo  = gsm->msInfo;
    GsmBcchListEntry*   selCellBcchInfo;


    ctoa( node->getNodeTime(),clockStr );

    // if MS on SDCCH or TCH...
    if (msInfo->isDedicatedChannelAssigned == TRUE ){
        return;
    }


    // New cell selection
    if (msInfo->isCellSelected == FALSE ||

        /* Check if reselection needed (isCellSelected is TRUE) */

        ( MacGsmIsCellReselectionNeeded( node,msInfo ) == TRUE ) ){
        if (msInfo->isCellSelected == TRUE ){
            // Reselection has occurred
            if (DEBUG_GSM_MAC_3 ){
                printf( "GSM_MAC [%d] Reselecting cell at: time %s, "
                            "TS %d, CtrlFr %d Fr %d\n",
                    node->nodeId,
                    clockStr,
                    msInfo->slotNumber,
                    msInfo->controlFrameNumber,
                    msInfo->frameNumber );
            }
            msInfo->stats.numCellReSelectionAttempts++;
        }


        for (int i = 0; i < msInfo->numBcchChannels; i++ ){
            if (msInfo->bcchList[i].channelIndex ==
                   msInfo->downLinkControlChannelIndex ){
                msInfo->bcchList[i].isSelected = TRUE;
            }
            else{
                msInfo->bcchList[i].isSelected = FALSE;
            }
        }

        if (MacGsmPerformCellSelection( node,
                 msInfo,
                 &selCellBcchInfo ) == TRUE ){

            msInfo->isCellSelected = TRUE;
            msInfo->isFcchDetected = FALSE;
            msInfo->isSchDetected = FALSE;
            msInfo->isBcchDecoded = FALSE;

            msInfo->sendSChMeasFailure = FALSE;

            msInfo->timeSinceCellSelected = node->getNodeTime();

            // Set quick reference ptr for this channel
            msInfo->controlChannelBcchEntry = selCellBcchInfo;
            msInfo->downLinkControlChannelIndex =
            selCellBcchInfo->channelIndex;
            msInfo->upLinkControlChannelIndex = (
                short )( selCellBcchInfo->channelIndex + 1 );

            if (DEBUG_GSM_MAC_3 ){
                ctoa( selCellBcchInfo->lastUpdatedAt,clockStr2 );
                printf( "GSM_MAC [%d] Cell selected(%s): CH %d, C1 %f, %s\n",
                    node->nodeId,
                    clockStr,
                    selCellBcchInfo->channelIndex,
                    selCellBcchInfo->C1,
                    clockStr2 );
            }

            selCellBcchInfo->numRecorded = 0;
            selCellBcchInfo->rxLevel_dBm = 0.0;

            selCellBcchInfo->ber = 0.0;
            selCellBcchInfo->C1 = 0.0;
            selCellBcchInfo->isNeighChannel = FALSE;

            msInfo->stats.numCellSelections++;
        }
        else{
            // if cell selection failed keep trying

            if (DEBUG_GSM_MAC_3 ){
                printf( "GSM_MAC [%d] Cell selection failed, %s.\n\n",
                    node->nodeId,
                    clockStr );
            }

            MESSAGE_Send(
                node,
                msInfo->cellSelectionTimer,
                DefaultGsmCellSelectionTime);
            // if cell reselection failed
            // TODO: do cleanup of existing cell.
            //       notify upper layers
            msInfo->isCellSelected = FALSE;
            msInfo->stats.numCellSelectionsFailed++;
            return;
        }
    }

    // Start timer for cell re-selection
    MESSAGE_Send(
        node,
        msInfo->cellSelectionTimer,
        DefaultGsmCellReSelectionTime);
} // End of MacGsmCheckForCellSelection //


static
void MacGsmHandleTchIdleSlotStartTimer(
    Node* node,
    MacDataGsm* gsm)
{
    char            clockStr[MAX_STRING_LENGTH];
    clocktype       timeToListen;

    MacGsmMsInfo*   msInfo  = gsm->msInfo;


    ctoa( node->getNodeTime(),clockStr );

    // Start listening to the BCCH channels
    if (msInfo->isDedicatedChannelAssigned &&
           msInfo->assignedChannelType == GSM_CHANNEL_TCH ){
        // Can/Should cycle thru the BCCH carriers list, listening
        // to one at a time?
        MacGsmMsStartListeningToBcchListChannels( node, gsm );

        // Account for the 3 TS gap (Should factor in timing advance)
        if (msInfo->isIdleSlot_Rx_Tx_Interval == TRUE ){
            // If this is the interval between the end (hence 2 & 4)
            // of Downlink(Rx) and start of Uplink(Tx).

            timeToListen = 2 * ( DefaultGsmSlotDuration );
            msInfo->isIdleSlot_Rx_Tx_Interval = FALSE;
        }
        else{
            // Interval between end of Tx and start of Rx.
            if (msInfo->assignedSlotNumber <
                   msInfo->assignedUplinkSlotNumber ){
                if (
                    MacGsmIsNextTchFrameWithIdleSlot(
                    msInfo->assignedSlotNumber,
                        msInfo->frameNumber ) == TRUE ){
                    // Listen to an additional frame since the next slot
                    // is 'idle'.

                    timeToListen = 12 * ( DefaultGsmSlotDuration );
                    // Bug # 3436 fix - start
                    msInfo->idleFramePassed = TRUE;
                    // Bug # 3436 fix - end
                }
                else{
                    timeToListen = 4 * ( DefaultGsmSlotDuration );
                }
            }
            else // assignedSlotNumber > assignedUplinkSlotNumber //
            {
                // happens when assigned slot 5, 6 or 7.
                if (
                    MacGsmIsCurrentTchFrameWithIdleSlot(
                    msInfo->assignedSlotNumber,
                        msInfo->frameNumber ) == TRUE ){
                    timeToListen = 12 * ( DefaultGsmSlotDuration );
                    // Bug # 3436 fix - start
                    msInfo->idleFramePassed = TRUE;
                    // Bug # 3436 fix - end
                }
                else{
                    timeToListen = 4 * ( DefaultGsmSlotDuration );
                }
            }

            msInfo->isIdleSlot_Rx_Tx_Interval = TRUE;
        }

        // Set timer to stop listening just before
        // assigned uplink slot is about to start.
        // Bug # 3436 fix - start
        gsm->idleTimerMsg = MacGsmStartTimer( node,
            gsm,
            MSG_MAC_GsmIdleSlotEndTimer,
            timeToListen );
        // Bug # 3436 fix - end
    }
} // End of MacGsmHandleTchIdleSlotStartTimer //


static
void MacGsmHandleTchIdleSlotEndTimer(
    Node* node,
    MacDataGsm* gsm)
{
    char            clockStr[MAX_STRING_LENGTH];
    MacGsmMsInfo*   msInfo  = gsm->msInfo;


    ctoa( node->getNodeTime(),clockStr );

    // Occurs before the start of the
    // assigned time slot (Downlink-Rx or Uplink-Tx).

    // Stop listening to the BCCH channels.

    if (msInfo->isDedicatedChannelAssigned &&
           msInfo->assignedChannelType == GSM_CHANNEL_TCH ){
        MacGsmMsStopListeningToBcchListChannels( node,gsm );

        MacGsmStartListeningToChannel( node,
            gsm,
            gsm->myMacData->phyNumber,
            msInfo->assignedChannelIndex - 1 );
        // Bug # 3436 fix - start
        gsm->idleTimerMsg = MacGsmStartTimer( node,
            gsm,
            MSG_MAC_GsmIdleSlotStartTimer,
            DefaultGsmSlotDuration );
        // Bug # 3436 fix - end
    }
} // End of MacGsmHandleTchIdleSlotEndTimer //


/*
 * FUNCTION:    MacGsmIsThereAMsgToBeSent
 * PURPOSE: Checks to see if there is a Layer 3 packet scheduled
 *          to be sent by this node in the given time slot
 *          and for the specified channel type.
 *
 * COMMENT: If either of slotToCheck or channelTypeToCheck is
 *          passed with their respective INVALID values then
 *          the check for it is ignored (by the first OR condition
 *          in each case).
 *
 *          Similarly for the two stored values that are peeked
 *          at (by the second OR condition).
 */

static
BOOL MacGsmIsThereAMsgToBeSent(
    Node* node,
    short slotToCheck,
    short channelToCheck,
    GsmChannelType_e channelTypeToCheck)
{
    Message*            msg = NULL;

    uddtSlotNumber      slotNumber;
    uddtChannelIndex    channelIndex;
    GsmChannelType_e    channelType;

    GsmLayer3Data*      nwGsmData;


    GsmLayer3PeekAtNextPacket( node,
        slotToCheck,
        channelToCheck,
        &msg,
        &slotNumber,
        &channelIndex,
        &channelType );

    nwGsmData = ( GsmLayer3Data * )node->networkData.gsmLayer3Var;

    // If either of the passed in values is GSM_INVALID_XX_XX
    // the check for that variable is TRUE

    // Create the 3 time slot (burst period) lag in MS's
    // transmission. So a pkt for slot 2 is sent in slot 5 instead.
    // The BS 'compensates' for this before sending the pkt up from
    // MAC to Layer 3.

    if (msg != NULL &&
           ( slotToCheck == GSM_INVALID_SLOT_NUMBER ||
               slotNumber == GSM_INVALID_SLOT_NUMBER || slotNumber ==
               slotToCheck ) && (
           channelIndex ==
        GSM_INVALID_CHANNEL_INDEX ||
        channelIndex ==
        channelToCheck ) &&
           (
           channelTypeToCheck ==
        GSM_INVALID_CHANNEL_TYPE ||
        channelType ==
        GSM_INVALID_CHANNEL_TYPE || channelType == channelTypeToCheck ) ){
        return TRUE;
    }
    else{
        return FALSE;
    }
} // End of MacGsmIsThereAMsgToBeSent //


static
void MacGsmHandleMsSlotTimer(
    Node* node,
    MacDataGsm* gsm,
    Message* msg)
{
    char                clockStr[MAX_STRING_LENGTH];

    uddtSlotNumber      slotNumber      = GSM_INVALID_SLOT_NUMBER;
    uddtChannelIndex    channelIndex    = GSM_INVALID_CHANNEL_INDEX;
    GsmChannelType_e    channelType     = GSM_INVALID_CHANNEL_TYPE;

    clocktype           currentTime;
    MacGsmMsInfo*       msInfo          = gsm->msInfo;
    Message*            deqMsg;

    currentTime = node->getNodeTime();
    msInfo->currentSlotStartTime = currentTime;
    ctoa( currentTime,clockStr );

    // If this timer expired in a time less than one slot duration
    // since the cell was selected => this slot timer is that of the
    // previous cell. So IGNORE it. Upon selection of the new cell a
    // new slot timer was started.

    if (msInfo->timeSinceCellSelected + DefaultGsmSlotDuration >
           currentTime ){
        if (DEBUG_GSM_MAC_2 ){
            printf( "GSM_MAC [%d]: Cell resel occurred in last TS\n",
                node->nodeId );
        }
        return;
    }
    else if (msInfo->isHandoverInProgress == TRUE ){
        // Do nothing so that the old BS based slot timer is stopped.
        return;
    }

    // Start processing of current slot for this MS
    // Update slot & frame counters
    // Should also update super/hyper frame counts as well.

    if (msInfo->slotNumber == ( GSM_SLOTS_PER_FRAME - 1 ) ) // slots 0-7
    {
        msInfo->slotNumber = 0;

        msInfo->startTimeOfNextFrame = currentTime + DefaultGsmFrameDuration;

        if (msInfo->controlFrameNumber == GSM_CONTROL_MULTIFRAME_SIZE - 1 ){
            msInfo->controlFrameNumber = 0;
        }
        else{
            msInfo->controlFrameNumber++;
        }

        if (msInfo->trafficFrameNumber == GSM_TRAFFIC_MULTIFRAME_SIZE - 1 ){
            msInfo->trafficFrameNumber = 0;
            msInfo->sacchBlockNumber = ( msInfo->sacchBlockNumber + 1 ) % 4;
        }
        else{
            msInfo->trafficFrameNumber++;
        }

        if (msInfo->frameNumber == GSM_MAX_FRAME_SIZE - 1 ){
            msInfo->frameNumber = 0;
        }
        else{
            msInfo->frameNumber++;
        }
    }
    else{
        msInfo->slotNumber++;
    }

    if (DEBUG_GSM_MAC_0 ){
        printf( "MAC_GSM[%d] TimeSlot %d, fr %d at %s, BCCH %d, Ded.CH %d\n",
            node->nodeId,
            msInfo->slotNumber,
            msInfo->frameNumber,
            clockStr,
            msInfo->downLinkControlChannelIndex,
            msInfo->assignedChannelIndex );
    }

    // Perform needed actions for this slot
    // check the network layer if there is a msg to send...

    // TS0 is used to listen to the control channel for
    // broadcasts, meaning the MS cannot transmit on TS0 (
    // on any channel).
    // So untill multiple device support is added
    // use any slot other than 0 for sending out pkts.

    // Perform processing for this slot

    // If there is a packet to be sent in this/current slot
    // on a particular channel

    if (msInfo->isDedicatedChannelAssigned == TRUE ){
        // Cleanup any pending RACH requests if a channel has
        // been assigned.
        while (msInfo->slotNumber == 0 &&
                  MacGsmIsThereAMsgToBeSent( node,
                      0,
                      // slot 0
                      (short)msInfo->upLinkControlChannelIndex,
                      GSM_CHANNEL_RACCH ) == TRUE){
            GsmLayer3DequeuePacket( node,
                0,
                // slot 0
                (short)msInfo->upLinkControlChannelIndex,
                &deqMsg,
                &slotNumber,
                &channelIndex,
                &channelType );

            MESSAGE_Free( node,deqMsg );
        }

        if (msInfo->slotNumber == msInfo->assignedUplinkSlotNumber ){
            if (msInfo->assignedChannelType == GSM_CHANNEL_TCH &&
                   MacGsmIsSacchMsgFrame( msInfo->assignedSlotNumber,
                       msInfo->frameNumber ) == TRUE ){
                // Send the SACCH Measurement Report message.
                GsmMacMsSendMeasurementReportMsg( node,gsm );
            }
            else if (msInfo->slotNumber ==
                        msInfo->assignedUplinkSlotNumber &&
                        ( ( MacGsmIsThereAMsgToBeSent( node,
                                MacGsmGetReverseUpLinkSlotNumber(
                                msInfo->slotNumber ),
                                (short)msInfo->assignedChannelIndex,
                                GSM_CHANNEL_SDCCH ) == TRUE ) ||
                            ( MacGsmIsThereAMsgToBeSent( node,
                                  MacGsmGetReverseUpLinkSlotNumber(
                                  msInfo->slotNumber ),
                                  (short)msInfo->assignedChannelIndex,
                                  GSM_CHANNEL_FACCH ) == TRUE ) ||
                            ( MacGsmIsThereAMsgToBeSent( node,
                                  MacGsmGetReverseUpLinkSlotNumber(
                                  msInfo->slotNumber ),
                                  (short)msInfo->assignedChannelIndex,
                                  GSM_CHANNEL_TCH ) == TRUE ) ) ){
                GsmLayer3DequeuePacket( node,
                    MacGsmGetReverseUpLinkSlotNumber( msInfo->slotNumber ),
                    (short)msInfo->assignedChannelIndex,
                    &deqMsg,
                    &slotNumber,
                    &channelIndex,
                    &channelType );

                GsmMacSendMessageOverPhy( node,
                    deqMsg,
                    gsm->myMacData->phyNumber,
                    channelIndex,
                    DefaultGsmSlotDuration );
            } // End of SDCCH, FACCH, TCH //
        }
    }
    else{
        if (msInfo->slotNumber == 3 &&
               MacGsmIsThereAMsgToBeSent( node,
                   0,
                   // slot 0
                   (short)msInfo->upLinkControlChannelIndex,
                   GSM_CHANNEL_RACCH ) == TRUE ) // Uplink slot 0
        {
            GsmLayer3DequeuePacket( node,
                0,
                // slot 0
                (short)msInfo->upLinkControlChannelIndex,
                &deqMsg,
                &slotNumber,
                &channelIndex,
                &channelType );

            if (msInfo->isDedicatedChannelAssigned == TRUE ){
                MESSAGE_Free( node,deqMsg );
                return;
            }

            GsmMacSendMessageOverPhy( node,
                deqMsg,
                gsm->myMacData->phyNumber,
                ( short )( msInfo->upLinkControlChannelIndex ),
                DefaultGsmSlotDuration );
        } // End of RACH //
    }

    // reset internal timer to wakeup at start of next time slot

    MacGsmUpdateSlotTimer( node,gsm,msg );
}   // End of MacGsmHandleMsSlotTimer //


static
void MacGsmHandleBsSlotTimer(
    Node* node,
    MacDataGsm* gsm,
    Message* msg)
{
    char                clockStr[MAX_STRING_LENGTH];
    unsigned char*      msgContent;
    uddtSlotNumber      slotNumber      = GSM_INVALID_SLOT_NUMBER;
    uddtChannelIndex    channelIndex    = GSM_INVALID_CHANNEL_INDEX;
    GsmChannelType_e    channelType     = GSM_INVALID_CHANNEL_TYPE;
    short               chan;

    clocktype           currentTime;

    MacGsmBsInfo*       bsInfo          = gsm->bsInfo;
    Message*            deqMsg;


    currentTime = node->getNodeTime();
    ctoa( node->getNodeTime(),clockStr );
    bsInfo->startTimeOfCurrentSlot = currentTime;

    if (bsInfo->slotNumber == ( GSM_SLOTS_PER_FRAME - 1 ) ){
        // If current slot is the last slot of each 8 slot cycle
        bsInfo->slotNumber = 0;

        if (bsInfo->controlFrameNumber == GSM_CONTROL_MULTIFRAME_SIZE - 1 ){
            bsInfo->controlFrameNumber = 0;
        }
        else{
            bsInfo->controlFrameNumber++;
        }

        if (bsInfo->frameNumber == GSM_MAX_FRAME_SIZE - 1 ){
            bsInfo->frameNumber = 0;
        }
        else{
            bsInfo->frameNumber++;

            if (DEBUG_GSM_MAC_0 ){
                printf( "BS[%d]: TS %d, fr %d, time %s, BCCH %d\n",
                    node->nodeId,
                    bsInfo->slotNumber,
                    bsInfo->frameNumber,
                    clockStr,
                    bsInfo->downLinkControlChannelIndex );
            }
        }
    }
    else{
        bsInfo->slotNumber++;
    }

    if (DEBUG_GSM_MAC_3 && node->nodeId == 8 && bsInfo->slotNumber == 1 ){
        printf( "Mac_GSM: BS[%d] Layer func: Slot %d, "
                    "ctrlFrame %d, frame %d, time %s\n",
            node->nodeId,
            bsInfo->slotNumber,
            bsInfo->controlFrameNumber,
            bsInfo->frameNumber,
            clockStr );
    }

    // For now only msg expected is the repeated timer expiration
    // for sending BCCH SysInfoType3 message on the control channel
    // announcing BS info and when the TS 0 begins next.
    // This combines the
    // FCCH & SCH functionality with BCCH for now and hence
    // SysInfoType3 is sent in FCCH & SCH time slots also.

    // In a 51 (0-50) cycle of 8 time slots (Burts Periods)
    // the FCCH uses TS0 in cycles 0, 10, 20, ...
    // the SCH uses TS0 in cycles 1, 11, 21, ...
    // the BCCH uses TS0 in cycles 2-5

    if (MacGsmIsFcchSlot( bsInfo->slotNumber,
             bsInfo->controlFrameNumber ) == TRUE ){
        ctoa( node->getNodeTime(),clockStr );

        //printf("\nBS [%ld] sending FCCH, TS %d, CtrlFr %d at %s\n",
        //        node->nodeId, bsInfo->slotNumber, bsInfo->controlFrameNumber,
        //        clockStr);

        MacGsmBsSendFcchMsg( node,gsm );
    }
    else if (MacGsmIsSchSlot( bsInfo->slotNumber,
                  bsInfo->controlFrameNumber ) == TRUE ){
        ctoa( node->getNodeTime(),clockStr );

        //printf("GSM_BS [%ld] sending SCH msg, CtrlFr %d %s\n",
        //        node->nodeId, bsInfo->controlFrameNumber, clockStr);

        MacGsmBsSendSchMsg( node,gsm );
    }
    else if (MacGsmIsBcchSlot( bsInfo->slotNumber,
                  bsInfo->controlFrameNumber ) == TRUE ){
        int tc  = ( bsInfo->frameNumber / GSM_CONTROL_MULTIFRAME_SIZE ) %
                      GSM_SLOTS_PER_FRAME;


        if (tc == 1 ){
            Message*                        sysInfoType2Msg;


            sysInfoType2Msg = MESSAGE_Duplicate( node,
                                  gsm->bsInfo->sysInfoType2Msg );

            GsmMacSendMessageOverPhy( node,
                sysInfoType2Msg,
                MacGsmBsGetPhyNumberFromChannel( node,
                    bsInfo,
                    bsInfo->downLinkControlChannelIndex ),
                bsInfo->downLinkControlChannelIndex,
                DefaultGsmSlotDuration );
        }
        else if (tc == 2 || tc == 6 ){
            GsmMacBsSendSysInfoType3Msg( node,gsm );
        }
    }
    else if (MacGsmIsPagchSlot( bsInfo->slotNumber,
                  bsInfo->controlFrameNumber ) == TRUE &&
                ( MacGsmIsThereAMsgToBeSent( node,
                      bsInfo->slotNumber,
                      (short)bsInfo->downLinkControlChannelIndex,
                      GSM_CHANNEL_PAGCH ) == TRUE ) ){
        // PAGCH, GSM 05.02 Sec 7, Tabe 3 & figure 8
        // Get packet from Network Layer
        GsmLayer3DequeuePacket( node,
            bsInfo->slotNumber,
            (short)bsInfo->downLinkControlChannelIndex,
            &deqMsg,
            &slotNumber,
            &channelIndex,
            &channelType );

        if (deqMsg != NULL ){
            msgContent = ( unsigned char* )MESSAGE_ReturnPacket( deqMsg );

            GsmMacSendMessageOverPhy( node,
                deqMsg,
                MacGsmBsGetPhyNumberFromChannel( node,
                    bsInfo,
                    gsm->bsInfo->downLinkControlChannelIndex ),
                gsm->bsInfo->downLinkControlChannelIndex,
                DefaultGsmSlotDuration );
        } // End of processing dequeued message //
    }  // End of PAGCH slot //
    else{
        if (bsInfo->slotNumber == 0 ){
            GsmMacBsSendDummyBurstMsg( node,gsm );
        }
    }

    for (chan = 0; chan < ( bsInfo->numChannels / 2 ); chan++ ){
        // Check the downlink non-control channels for scheduled transmission

        uddtChannelIndex    channelToCheck  = bsInfo->channelIndexStart +
                                                  ( chan * 2 );

        if (MacGsmIsThereAMsgToBeSent( node,
                 bsInfo->slotNumber,
                 (short)channelToCheck,
                 GSM_CHANNEL_SDCCH ) == TRUE ){
            // Dequeue the packet and send it
            GsmLayer3DequeuePacket( node,
                bsInfo->slotNumber,
                (short)channelToCheck,
                &deqMsg,
                &slotNumber,
                &channelIndex,
                &channelType );

            msgContent = ( unsigned char* )MESSAGE_ReturnPacket( deqMsg );

            GsmMacSendMessageOverPhy( node,
                deqMsg,
                MacGsmBsGetPhyNumberFromChannel( node,
                    bsInfo,
                    channelIndex ),
                channelIndex,
                DefaultGsmSlotDuration );
        }
        else if (MacGsmIsThereAMsgToBeSent( node,
                      bsInfo->slotNumber,
                      (short)channelToCheck,
                      GSM_CHANNEL_TCH ) == TRUE ){
            GsmLayer3DequeuePacket( node,
                bsInfo->slotNumber,
                (short)channelToCheck,
                &deqMsg,
                &slotNumber,
                &channelIndex,
                &channelType );

            if (deqMsg != NULL ){
                // printf("MAC BS[%ld]: Sending TCH %d msg TS %d fr %d at %s\n",
                //     node->nodeId, channelIndex, slotNumber, bsInfo->frameNumber,
                //     clockStr);

                if (DEBUG_GSM_MAC_0 ){
                    printf(
                        "MAC_GSM[%d] Xmit on TS %d, fr %d, at %s CH %d\n",
                        node->nodeId,
                        bsInfo->slotNumber,
                        bsInfo->frameNumber,
                        clockStr,
                        channelIndex );
                }
                GsmMacSendMessageOverPhy( node,
                    deqMsg,
                    MacGsmBsGetPhyNumberFromChannel( node,
                        bsInfo,
                        channelIndex ),
                    channelIndex,
                    DefaultGsmSlotDuration );
            }
        } // TCH messages //
    } // check next channel //

    // reset internal timer to wakeup at begining of next time slot
    MacGsmUpdateSlotTimer( node,gsm,msg );
} // End of MacGsmHandleBsSlotTimer //


// Updates measured values of rxLevel & ber
// Calculates an avg value for the last 5 values
// and resets to 0 after 5 are received.
// Actually it Should maintain a running avg. of values received in
// last 5 seconds (~ 80+ samples for BCCH slots, 4 per 235ms).
// See GSM 05.08 Section 6.6.1

static
void MacGsmUpdateMsBcchMeasurements(
    Node* node,
    MacGsmMsInfo* msInfo,
    uddtChannelIndex           channelIndex,
    float           rxLevel_dBm,
    double          ber,
    GsmBcchListEntry** retBcchEntry)
{
    char                clockStr[MAX_STRING_LENGTH];
    int                 i;

    GsmBcchListEntry*   bcchInfo;


    ctoa( node->getNodeTime(),clockStr );


    for (i = 0; i < msInfo->numBcchChannels; i++ ){
        bcchInfo = &( msInfo->bcchList[i] );

        if (bcchInfo->channelIndex == channelIndex ){
            // While on a dedicated channel, record values for the reporting
            // period only. See GSM 05.08 Section 8.4 for reporting periods.
            if (
                msInfo->isDedicatedChannelAssigned == TRUE &&
                msInfo->assignedChannelType ==
                GSM_CHANNEL_TCH &&
                msInfo->frameNumber %
                GSM_SACCH_MULTIFRAME_SIZE ==
                (
                unsigned )GsmMeasRepPeriod[msInfo->assignedSlotNumber][1] ){
                bcchInfo->numRecorded = 0;
                bcchInfo->rxLevel_dBm = 0.0;
                bcchInfo->ber = 0.0;
                bcchInfo->lastUpdatedAt = 0;
            }

            if (bcchInfo->numRecorded > GSM_BCCHRefreshVariable ){
                bcchInfo->numRecorded = 0;
                bcchInfo->rxLevel_dBm = 0.0;
            }
            if (( bcchInfo->numRecorded > 5 ) &&
                   (
                   (
                bcchInfo->lastUpdatedAt +
                1 * SECOND ) > node->getNodeTime() ) ){
                *retBcchEntry = bcchInfo;
                break;
            }

            bcchInfo->numRecorded++;
            bcchInfo->rxLevel_dBm += rxLevel_dBm;
            bcchInfo->ber += ber;
            bcchInfo->lastUpdatedAt = node->getNodeTime();

            *retBcchEntry = bcchInfo;
            break;
        }
    }
    // Should maintain an array to hold the last 5 received values
    // and use them for a running average.
} // End of MacGsmUpdateMsBcchMeasurements //


static
void MacGsmUpdateMsDedicatedChMeasurements(
    Node* node,
    MacGsmMsInfo* msInfo,
    float           rxLevel_dBm,
    double          ber)
{
    // See GSM 05.08 Section 8.4 for reporting periods based on
    // MS's assigned TS.
    if (msInfo->assignedChannelType == GSM_CHANNEL_TCH &&
           msInfo->frameNumber %
           GSM_SACCH_MULTIFRAME_SIZE ==
           ( unsigned )GsmMeasRepPeriod[msInfo->assignedSlotNumber][1] ){
        msInfo->numAssignedChMeasurements = 0;
        msInfo->assignedChRxLevel_dBm = 0.0;
        msInfo->assignedChBer = 0.0;
        msInfo->assignedChMeasLastUpdatedAt = 0;
    }
    // Bug # 3436 fix - start
    if (msInfo->supportFlag == TRUE )
        {
            msInfo->numAssignedChMeasurements = 0;
            msInfo->assignedChRxLevel_dBm = 0.0;
            msInfo->assignedChBer = 0.0;
            msInfo->assignedChMeasLastUpdatedAt = 0;
            msInfo->supportFlag = FALSE;
        }
    // Bug # 3436 fix - end

    msInfo->assignedChRxLevel_dBm += rxLevel_dBm;
    msInfo->assignedChBer += ber;
    msInfo->numAssignedChMeasurements++;
    msInfo->assignedChMeasLastUpdatedAt = node->getNodeTime();
} // End of MacGsmUpdateMsDedicatedChMeasurements //


static
void MacGsmUpdateBsDedicatedChMeasurements(
    Node* node,
    MacGsmBsInfo* bsInfo,
    uddtSlotNumber           slotNumber,
    uddtChannelIndex           upLinkChannelIndex,
    float           rxLevel_dBm,
    double          ber)
{
    // indexed by slotNumber, channelPair# (not ARFCN)
    GsmChannelMeasurement*  channelMeas = &(
             bsInfo->upLinkChMeas[slotNumber][(
            upLinkChannelIndex - bsInfo->upLinkControlChannelIndex ) / 2] );

    if (bsInfo->frameNumber % GSM_SACCH_MULTIFRAME_SIZE ==
           ( unsigned )GsmSacchMsgFrame[slotNumber][0] - 13 ){
        channelMeas->numRecorded = 0;
        channelMeas->rxLevel_dBm = 0.0;
        channelMeas->ber = 0.0;
        channelMeas->lastUpdatedAt = 0;
    }
    // Bug # 3436 fix - start
    if (bsInfo->supportFlag == TRUE )
    {
        channelMeas->numRecorded = 0;
        channelMeas->rxLevel_dBm = 0.0;
        channelMeas->ber = 0.0;
        channelMeas->lastUpdatedAt = 0;
        bsInfo->supportFlag = FALSE;
    }
    // Bug # 3436 fix - end
    channelMeas->rxLevel_dBm += rxLevel_dBm;
    channelMeas->ber += ber;
    channelMeas->numRecorded++;
    channelMeas->lastUpdatedAt = node->getNodeTime();
} // End of MacGsmUpdateBsDedicatedChMeasurements //


static
void MacGsmInitStats(
    Node* node,
    const NodeInput* nodeInput,
    int interfaceIndex)
{
    BOOL        retVal;
    char        buf[MAX_STRING_LENGTH];

    MacDataGsm* gsm;


    gsm = ( MacDataGsm * )node->macData[interfaceIndex]->macVar;


    IO_ReadString( node->nodeId,
        ANY_DEST,
        nodeInput,
        "GSM-STATISTICS",
        &retVal,
        buf );

    if (( retVal == FALSE ) || ( strcmp( buf,"NO" ) == 0 ) ){
        gsm->collectStatistics = FALSE;
    }
    else if (strcmp( buf,"YES" ) == 0 ){
        gsm->collectStatistics = TRUE;
    }
}


/*
 * Parse a channel list from a string formatted
 * as "[0 4 5]".
 */
int
GsmParseChannelList(
    Node* node,
    char* channelString,
    short* controlChannels)
{
    int     i   = 0;
    int     strLength;
    char    key[MAX_STRING_LENGTH];
    char    errorString[MAX_STRING_LENGTH];
    char*   next;
    Int16   channelIndex = -1;
    Int32   j = 0;

    strLength = ( int )strlen( channelString );

    if (channelString[i] == '[' && channelString[strLength - 1] == ']' ){
        channelString++; // skip '['
        IO_Chop( channelString );  // skip ']'
        IO_TrimRight(channelString);

        next = channelString;
        while (*next != '\0')
        {
            IO_GetToken(key, next, &next);
            if (IO_IsStringNonNegativeInteger(key))
            {
                channelIndex = (short)strtoul(key, NULL, 10);
            }
            else if (isalpha(*key) && PHY_ChannelNameExists(node, key))
            {
                channelIndex = PHY_GetChannelIndexForChannelName(
                                    node,
                                    key);
            }
            else
            {
                ERROR_ReportErrorArgs("GSM-CONTROL-CHANNEL "
                                      "parameter should be an integer "
                                      "between 0 and %d or a valid "
                                      "channel name.",
                                      node->numberChannels - 1);
            }

            if (channelIndex < node->numberChannels)
            {
                BOOL isDuplicateFound = FALSE;

                for (j = 0; j < i; j++)
                {
                    if (controlChannels[j] == channelIndex)
                    {
                        isDuplicateFound = TRUE;
                        break;
                    }
                }

                if (isDuplicateFound)
                {
                    ERROR_ReportWarning("GSM-CONTROL-CHANNEL-LIST "
                                        "contains duplicate values.");
                }
                else
                {
                    controlChannels[i++] = channelIndex;
                }
            }
            else
            {
                ERROR_ReportErrorArgs("GSM-CONTROL-CHANNEL "
                                      "parameter should be an integer "
                                      "between 0 and %d or a valid "
                                      "channel name.",
                                      node->numberChannels - 1);
            }
        }        
    }
    else
    {
        ERROR_ReportErrorArgs("GSM-CONTROL-CHANNEL is incorrectly"
                              " formatted: %s\n", channelString );
    }
    return i;
} // End of GsmParseChannelList //


/*
 * Parse a information of the form
 * "0-3-4 4-3-3 5-3-4".
 */
BOOL GsmParseAdditionalInfo(
    char* inputString,
    int* field1,
    uddtLAC* field2,
    uddtCellId* field3)
{
    int     i   = 0;
    int     j   = 0;
    int     strLength;
    char*   key;//[MAX_STRING_LENGTH];
    char    errorString[MAX_STRING_LENGTH];
    char    tempStr[MAX_STRING_LENGTH];

    key = inputString;
    // key = "0-5-1" of the form "field1-field2-field3"
    sprintf( errorString,"Incorrectly formatted string %s\n",key );
    strLength = ( int )strlen( key );

    // field 1
    for (i = 0, j = 0; i < strLength && key[i] != '-'; i++, j++ ){
        tempStr[j] = key[i];
    }
    tempStr[j] = 0;
    if (IO_IsStringNonNegativeInteger( tempStr ) ){
        *field1 = strtoul( tempStr,NULL,10 );
    }
    else{
        ERROR_ReportWarning( errorString );
        return FALSE;
    }

    if (i == strLength ){
        ERROR_ReportWarning( errorString );
        return FALSE;
    }
    i++;

    // field 2
    for (j = 0 ; i < strLength && key[i] != '-'; i++, j++ ){
        tempStr[j] = key[i];
    }
    tempStr[j] = 0;

    if (IO_IsStringNonNegativeInteger( tempStr ) ){
        *field2 = (uddtLAC)strtoul( tempStr,NULL,10 );
    }
    else{
        ERROR_ReportWarning( errorString );
        return FALSE;
    }

    if (i == strLength ){
        ERROR_ReportWarning( errorString );
        return FALSE;
    }
    i++;

    // field 3
    for (j = 0 ; i < strLength; i++, j++ ){
        tempStr[j] = key[i];
    }
    tempStr[j] = 0;
    if (IO_IsStringNonNegativeInteger( tempStr ) ){
        *field3 = strtoul( tempStr,NULL,10 );
    }
    else{
        ERROR_ReportWarning( errorString );
        return FALSE;
    }

    return TRUE;
} // End of GsmParseAdditionalInfo //


// Checks whether channelIndex is one of known BCCH (including
// neighbouring BS) channels.
BOOL MacGsmIsBcchChannel(
    MacGsmMsInfo* msInfo,
    uddtChannelIndex           channelIndex)
{
    int i;


    /***
    // Check if its an assigned channel
    if (msInfo->isDedicatedChannelAssigned == TRUE &&
            channelIndex == msInfo->assignedChannelIndex - 1)
    {
        return FALSE;
    }
    ***/

    for (i = 0; i < msInfo->numBcchChannels; i++ ){
        if (channelIndex == msInfo->bcchList[i].channelIndex ){
            return TRUE;
        }
    }

    return FALSE;
} // End of MacGsmIsBcchChannel //


/*
 * General functions
 */

/*
 * FUNCTION    MacGsmInit
 * PURPOSE     Initialization for GSM MAC layer (MS, BS).
 *
 * Parameters:
 *     node:      node being initialized.
 *     nodeInput: structure containing contents of input file
 */
void MacGsmInit(
    Node* node,
    int    interfaceIndex,
    const  NodeInput* nodeInput,
    const  int numNodesInSubnet)
{
    BOOL        wasFound;

    char        clockStr[MAX_STRING_LENGTH];
    char        varStrValue[MAX_STRING_LENGTH];

    int         i;

    double      txPower_dBm; // MS_TXPWR_MAX_CCH
    double      rxThreshold_dBm;

    MacDataGsm* gsm;


    gsm = ( MacDataGsm * )MEM_malloc( sizeof( MacDataGsm ) );

    ctoa( node->getNodeTime(),clockStr );

    if (DEBUG_GSM_MAC_3 ){
        printf( "MacGsmInit: Node %d, ifIndex %d/%d, %s\n",
            node->nodeId,
            interfaceIndex,
            node->numberInterfaces,
            clockStr );
    }

    memset( gsm,0,sizeof( MacDataGsm ) );
    gsm->myMacData = node->macData[interfaceIndex];
    gsm->myMacData->macVar = ( void* )gsm;

    for (i = 0; i < PROP_NumberChannels(node); i++)
    {
        MacGsmStopListeningToChannel(node,
                                     gsm,
                                     gsm->myMacData->phyNumber,
                                     i);
    }

    RANDOM_SetSeed(gsm->randSeed,
                   node->globalSeed,
                   node->nodeId,
                   MAC_PROTOCOL_GSM,
                   interfaceIndex);

    wasFound = FALSE;
    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "GSM-NODE-TYPE",
        &wasFound,
        varStrValue );

    ERROR_Assert( wasFound != FALSE,"GSM-NODE-TYPE not specified" );

    // Set the node type
    // Set the MS's to RX mode for this channel and
    // BS's to TX mode so that the start the BCCH messages

    if (strcmp( varStrValue,"GSM-MS" ) == 0 ){
        MacGsmMsInfo*   msInfo;


        if (DEBUG_GSM_MAC_3 ){
            printf( "Node %d: GSM_MS\n",node->nodeId );
        }

        gsm->nodeType = GSM_MS;
        gsm->msInfo = ( MacGsmMsInfo * )MEM_malloc( sizeof( MacGsmMsInfo ) );
        msInfo = gsm->msInfo;
        memset( msInfo,0,sizeof( MacGsmMsInfo ) );

        msInfo->downLinkControlChannelIndex = GSM_INVALID_CHANNEL_INDEX;
        msInfo->cellSelectionTimer = NULL;

        //read the control channel information
        wasFound = FALSE;
        IO_ReadString(
            node,
            node->nodeId,
            interfaceIndex,
            nodeInput,
            "GSM-CONTROL-CHANNEL-LIST",
            &wasFound,
            varStrValue );

        if (!wasFound)
        {
            ERROR_ReportWarning(
                "For GSM-NODE-TYPE as GSM-MS, GSM-CONTROL-CHANNEL-LIST "
                "should be configured\n");
            IO_ReadString(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "GSM-CONTROL-CHANNEL",
                &wasFound,
                varStrValue );
            ERROR_Assert( wasFound != FALSE,
                "GSM-CONTROL-CHANNEL-LIST not specified" );
        }

        msInfo->numControlChannels = GsmParseChannelList(
                                            node,
                                            varStrValue,
                                            msInfo->controlChannels );

        // Should remove controlChannels from msInfo
        msInfo->numBcchChannels = msInfo->numControlChannels;
        msInfo->nextBcchChannelToListen = 0;

        for (i = 0; i < msInfo->numControlChannels; i++ ){
            msInfo->bcchList[i].channelIndex = msInfo->controlChannels[i];
            msInfo->bcchList[i].isNeighChannel = FALSE;
            msInfo->bcchList[i].isSelected = FALSE;
            msInfo->bcchList[i].rxLevel_dBm = 0.0;
            msInfo->bcchList[i].ber = 0.0;
            msInfo->bcchList[i].numRecorded = 0;
            msInfo->bcchList[i].lastUpdatedAt = 0;
            msInfo->bcchList[i].lastDecodedAt = 0;
            msInfo->bcchList[i].C1 = 0.0;
        }

        MacGsmMsStartListeningToBcchListChannels( node, gsm );

        // Start timer for cell selection
        msInfo->cellSelectionTimer = MacGsmStartTimer( node,
                                         gsm,
                                         MSG_MAC_GsmCellSelectionTimer,
                                         DefaultGsmCellSelectionTime );

        //msInfo->rrState = GSM_RR_STATE_NULL;

        msInfo->isDedicatedChannelAssigned = FALSE;
        msInfo->isCellSelected = FALSE;
        msInfo->timeSinceCellSelected = node->getNodeTime();

        // Should be initialized to 0 so that when MS receives the
        // BS beacon messages (Sys Info Type 3) it can correctly
        // identify the channel type (based on slot & frame number)
        msInfo->slotNumber = 0;
        msInfo->controlFrameNumber = 0;
        msInfo->frameNumber = 0;

        IO_ReadDouble(
            node,
            node->nodeId,
            interfaceIndex,
            nodeInput,
            "PHY-GSM-TX-POWER",
            &wasFound,
            &txPower_dBm );

        if (wasFound ){
            msInfo->txPower_dBm = txPower_dBm;
        }
        else{
            msInfo->txPower_dBm = PHY_GSM_DEFAULT_TX_POWER_dBm;
        }

        IO_ReadDouble(
            node,
            node->nodeId,
            interfaceIndex,
            nodeInput,
            "PHY-GSM-RX-THRESHOLD",
            &wasFound,
            &rxThreshold_dBm );

        if (wasFound ){
            msInfo->rxLevAccessMin_dBm = rxThreshold_dBm;
        }
        else{
            msInfo->rxLevAccessMin_dBm = PHY_GSM_DEFAULT_RX_THRESHOLD_dBm;
        }

        msInfo->maxTxPower_dBm = PHY_GSM_P_dBm;
        setGSMNodeLAC( node,GSM_INVALID_LAC_IDENTITY );
        setGSMNodeCellId( node,GSM_INVALID_CELL_IDENTITY );
    } // End of GSM_MS //
    else if (strcmp( varStrValue,"GSM-BS" ) == 0 ){
        int  bsSlotTimeOffset = 0;
        NodeInput*      gsmNodeInput;
        gsmNodeInput = ( NodeInput * )MEM_malloc( sizeof( NodeInput ) );
        char            buffer[MAX_STRING_LENGTH];
        char            errorString[MAX_STRING_LENGTH];
        int             lineCount           = 0;
        BOOL            isNodeId;
        NodeId          nodeId;

        IO_ReadCachedFile( ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "GSM-NODE-CONFIG-FILE",
            &wasFound,
            gsmNodeInput );

        if (wasFound == FALSE ){
            ERROR_ReportError( "Could not find GSM-NODE-CONFIG-FILE\n" );
        }
        BOOL nodeFound = FALSE;
        for (lineCount = 0; lineCount < gsmNodeInput->numLines; lineCount++ ){
            sscanf( gsmNodeInput->inputStrings[lineCount],"%s",buffer );

            if (strcmp( buffer,"GSM-BS" ) == 0 ){
                sscanf( gsmNodeInput->inputStrings[lineCount],
                         "%*s %s",
                         buffer);

                IO_ParseNodeIdOrHostAddress( buffer,&nodeId,&isNodeId );

                if (!isNodeId ){
                    sprintf( errorString,
                        "GSM-NODE-CONFIG-FILE: '%s'\n"
                            "Second parameter needs to be a node id",
                        gsmNodeInput->inputStrings[lineCount] );

                    ERROR_ReportError( errorString );
                }
                if (nodeId == node->nodeId)
                {
                    nodeFound = TRUE;
                    break;
                }
                bsSlotTimeOffset++;
            }
        }

        if (!nodeFound)
        {
            sprintf(errorString,
                    "GSM-NODE-CONFIG-FILE: GSM-BS not found for node %d\n",
                    node->nodeId);
            ERROR_ReportError( errorString );
        }

        MEM_free(gsmNodeInput);

        short       controlChannels[1];
        int         numControlChannels;


        if (DEBUG_GSM_MAC_3 ){
            printf( "Node %d: GSM_BS\n",node->nodeId );
        }

        gsm->nodeType = GSM_BS;

        gsm->bsInfo = ( MacGsmBsInfo * )MEM_malloc( sizeof( MacGsmBsInfo ) );
        memset( gsm->bsInfo,0,sizeof( MacGsmBsInfo ) );

        //read the control channel information
        wasFound = FALSE;
        IO_ReadString(
            node,
            node->nodeId,
            interfaceIndex,
            nodeInput,
            "GSM-CONTROL-CHANNEL",
            &wasFound,
            varStrValue );

        ERROR_Assert( wasFound != FALSE,
            "GSM-CONTROL-CHANNEL not specified" );

        numControlChannels = GsmParseChannelList(node,
                                                 varStrValue,
                                                 controlChannels);

        gsm->bsInfo->downLinkControlChannelIndex = controlChannels[0];
        gsm->bsInfo->upLinkControlChannelIndex = (
            short )( controlChannels[0] + 1 );

        if (DEBUG_GSM_MAC_3 ){
            printf( " Control Channel %d\n",
                gsm->bsInfo->downLinkControlChannelIndex );
        }

        // Static initialization for now.
        // Read from config files in next update
        // or set up automatically (similar to auto IP assignments).

        gsm->bsInfo->slotNumber = GSM_INVALID_SLOT_NUMBER;
        gsm->bsInfo->controlFrameNumber = 0;
        gsm->bsInfo->frameNumber = 0;

        MacGsmBuildFcchMsg( node,gsm );

        MacGsmInitializeSlotTimer( node,
            gsm,
            1 * SECOND + bsSlotTimeOffset * DefaultGsmSlotDuration );
    }
    else if (strcmp( varStrValue,"GSM-MSC" ) == 0 ){
        if (DEBUG_GSM_MAC_3 ){
            printf( "Node GSM_MSC[%d]: Init\n",node->nodeId );
        }
    }
    else{
        ERROR_Assert( FALSE,"Unknown GSM-NODE-TYPE in config file" );
    }

#ifdef PARALLEL //Parallel
    PARALLEL_SetProtocolIsNotEOTCapable(node);
    PARALLEL_SetMinimumLookaheadForInterface(node, 0);
#endif //endParallel

    MacGsmInitStats( node,nodeInput,interfaceIndex );
} // End of MacGsmInit //


void MacGsmReceivePacketFromPhy(
    Node* node,
    MacDataGsm* gsm,
    Message* msg)
{
    char                clockStr[MAX_STRING_LENGTH];
    char                errorString[MAX_STRING_LENGTH];
    unsigned char*      msgContent;

    uddtChannelIndex    receivedChannelIndex;

    float               rxLevel;
    double              ber;

    clocktype           currentTime = node->getNodeTime();

    int                 phyNumber = -1;


    // Set by phy_gsm.c before sending up the packet
    receivedChannelIndex = (
        short )( *( unsigned int* )MESSAGE_ReturnInfo( msg ) );

    if (gsm->nodeType == GSM_MS ){
        phyNumber = gsm->myMacData->phyNumber;
    }
    else if (gsm->nodeType == GSM_BS ){
        phyNumber = MacGsmBsGetPhyNumberFromChannel( node,
                        gsm->bsInfo,
                        receivedChannelIndex );
    }

    rxLevel = PhyGsmGetRxLevel( node,phyNumber );
    ber = PhyGsmGetRxQuality( node,phyNumber );

    msgContent = ( unsigned char* )MESSAGE_ReturnPacket( msg );

    ctoa( currentTime,clockStr );



    if (DEBUG_GSM_MAC_1 ){
        printf( "MAC_GSM[%d] FromPhy[%d] at %s: "
                    " CH %d, rxLev = %f, ber %e\n",
            node->nodeId,
            phyNumber,
            clockStr,
            receivedChannelIndex,
            rxLevel,
            ber );
    }


    switch ( gsm->nodeType ){
        case GSM_MS:
            {
                MacGsmMsInfo*       msInfo;
                GsmBcchListEntry*   bcchEntry   = NULL;

                msInfo = gsm->msInfo;

                // Synchronization procedures

                if (MacGsmIsBcchChannel( msInfo,
                         receivedChannelIndex ) == TRUE ){
                    // Expect to receive:
                    //   - BCCH msgs from serving cell
                    //   - BCCH msgs from neighbouring cells
                    //   - msgs for this MS on assigned dedicated slot (!= 0)
                    //     if assigned channel is C0 channel of serving cell

                    // The CellSelectionTimer started at initialization will use
                    // these measurement values to select the serving cell/BS
                    MacGsmUpdateMsBcchMeasurements( node,
                        msInfo,
                        receivedChannelIndex,
                        rxLevel,
                        ber,
                        &bcchEntry );

                    // If one of the slots (1-7) on the control channel
                    // was assigned.
                    if (msInfo->isDedicatedChannelAssigned == TRUE &&
                           receivedChannelIndex ==
                           msInfo->downLinkControlChannelIndex &&
                           msInfo->slotNumber == msInfo->assignedSlotNumber ){
                        MacGsmUpdateMsDedicatedChMeasurements( node,
                            msInfo,
                            rxLevel,
                            ber );

                        GsmLayer3ReceivePacketFromMac( node,
                            msg,
                            msInfo->slotNumber,
                            msInfo->frameNumber,
                            receivedChannelIndex,
                            gsm->myMacData->interfaceIndex );

                        MESSAGE_Free( node,msg );

                        return;
                    }

                    // BCCH messages from the serving cell
                    if (msInfo->isDedicatedChannelAssigned == FALSE &&
                           receivedChannelIndex ==
                           msInfo->downLinkControlChannelIndex ){
                        if (MESSAGE_ReturnActualPacketSize( msg ) >=
                            GSM_BURST_SIZE &&
                            msInfo->isCellSelected == TRUE &&
                            msInfo->isSchDetected == FALSE &&
                            msInfo->isFcchDetected == FALSE &&
                               MacGsmIsFcchMsg( msgContent ) == TRUE ){
                            msInfo->isFcchDetected = TRUE;
                            msInfo->fcchDetectedAt = currentTime;
                            // approx boundaries of TS's known
                        } // End of FCCH //
                        else if (msInfo->isCellSelected == TRUE &&
                                    msInfo->isSchDetected ==
                                    FALSE &&
                                    msInfo->isFcchDetected ==
                                    TRUE &&
                                    (
                                    (
                                (
                                currentTime -
                                DefaultGsmSlotDuration ) <
                                msInfo->fcchDetectedAt +
                                DefaultGsmFrameDuration ) &&
                                (
                                (
                                currentTime +
                                DefaultGsmSlotDuration ) >
                                msInfo->fcchDetectedAt +
                                DefaultGsmFrameDuration ) ) ){
                            GsmSchMessage*  schMsg;


                            schMsg = ( GsmSchMessage * )msgContent;

                            // Initialize MS parameters
                            msInfo->downLinkControlChannelIndex =
                            receivedChannelIndex;

                            msInfo->slotNumber = GSM_INVALID_SLOT_NUMBER;
                            msInfo->isDedicatedChannelAssigned = FALSE;
                            msInfo->isSchDetected = TRUE;

                            msInfo->controlFrameNumber = (
                                short )(
                                (
                                schMsg->frameNumber +
                                1 ) % GSM_CONTROL_MULTIFRAME_SIZE );

                            msInfo->frameNumber = schMsg->frameNumber + 1;

                            MacGsmInitializeSlotTimer( node,
                                gsm,
                                schMsg->startTimeOfFrame +
                                DefaultGsmFrameDuration );
                        } // End of SCH //
                        else if (msInfo->isSchDetected == TRUE &&
                                    MacGsmIsFcchSlot( msInfo->slotNumber,
                                        msInfo->controlFrameNumber ) !=
                                    TRUE &&
                                    MacGsmIsSchSlot( msInfo->slotNumber,
                                        msInfo->controlFrameNumber ) !=
                                    TRUE &&
                                    msInfo->isDedicatedChannelAssigned ==
                                    FALSE ){
                            GsmLayer3ReceivePacketFromMac( node,
                                msg,
                                msInfo->slotNumber,
                                msInfo->frameNumber,
                                receivedChannelIndex,
                                gsm->myMacData->interfaceIndex );
                        }
                    } // End of selected cell's downlink channel processing //
                    else{
                        if (( bcchEntry->isSyncInfoDecoded == TRUE &&
                                 bcchEntry->lastDecodedAt +
                                 GSM_BCCH_REFRESH_PERIOD < currentTime ) ){
                            bcchEntry->isSyncInfoDecoded = FALSE;
                            bcchEntry->isFcchDetected = FALSE;
                            bcchEntry->isSchDetected = FALSE;
                        }
                        else if (bcchEntry->isSyncInfoDecoded == FALSE ){
                            // Do nothing, continue down
                        }
                        else{
                            MESSAGE_Free( node,msg );
                            return;
                        }



                        if (MESSAGE_ReturnActualPacketSize( msg ) >=
                            GSM_BURST_SIZE &&
                            bcchEntry->isSchDetected == FALSE &&
                            bcchEntry->isFcchDetected ==
                            FALSE && MacGsmIsFcchMsg( msgContent ) == TRUE ){
                            bcchEntry->isFcchDetected = TRUE;
                            bcchEntry->fcchDetectedAt = currentTime;
                            // printf("GSM_MAC[%ld]: Neigh.FCCH %d\n",
                            //    node->nodeId, receivedChannelIndex);
                        }
                         // Bug # 3436 fix - start
                        else if (bcchEntry->isSchDetected == FALSE &&
                                    bcchEntry->isFcchDetected ==
                                    TRUE &&
                                    MESSAGE_ReturnActualPacketSize( msg ) !=
                                    GSM_BURST_SIZE )
                            {
                                clocktype  fcchDetectedAt = 
                                    bcchEntry->fcchDetectedAt;
                                while (( ( fcchDetectedAt + 
                                           DefaultGsmFrameDuration ) < 
                                           node->getNodeTime() ))
                                {
                                    fcchDetectedAt = fcchDetectedAt + 
                                                    DefaultGsmFrameDuration;
                                }
                                if (( ( currentTime -
                                DefaultGsmSlotDuration ) <
                                         fcchDetectedAt +
                                DefaultGsmFrameDuration ) &&
                                         ( ( currentTime +
                                DefaultGsmSlotDuration ) >
                                             fcchDetectedAt +
                                             DefaultGsmFrameDuration ) )
                                {
                                    // char clockStr2[100];
                                    GsmSchMessage*  schMsg;

                                    schMsg = ( GsmSchMessage * )msgContent;
                                            if (schMsg->bsic != 0)
                                                    return;
                                    bcchEntry->isSyncInfoDecoded = TRUE;
                                    if (!bcchEntry->isSelected)
                                    {
                                        bcchEntry->isNeighChannel = TRUE;
                                    }
                                    bcchEntry->lastDecodedAt = currentTime;
                                    bcchEntry->nextFrameNumber =
                                    schMsg->frameNumber + 1;

                                    // calculate time of start of next frame
                                    bcchEntry->startTimeOfNextFrame =
                                    schMsg->startTimeOfFrame +
                                        DefaultGsmFrameDuration;
                                    // ctoa(bcchEntry->startTimeOfNextFrame, 
                                    //  clockStr2);
                                    // printf("GSM_MAC[%ld]: Neigh.SCH %d, 
                                    // fr %d, starts %s\n",
                                    //    node->nodeId, receivedChannelIndex,
                                    //    schMsg->frameNumber, clockStr2);
                                }
                            }
                            // Bug # 3436 fix - end
                        } // End of Neigh.Bcch Channel //
                } // End of if BCCH channel //
                else{
                    // Assigned /dedicated channel processing

                    if (msInfo->isDedicatedChannelAssigned == TRUE &&
                           receivedChannelIndex ==
                           msInfo->assignedChannelIndex -
                           1 &&
                           msInfo->slotNumber == msInfo->assignedSlotNumber )
                    {
                        MacGsmUpdateMsDedicatedChMeasurements( node,
                            msInfo,
                            rxLevel,
                            ber );

                        GsmLayer3ReceivePacketFromMac( node,
                            msg,
                            msInfo->slotNumber,
                            msInfo->frameNumber,
                            receivedChannelIndex,
                            gsm->myMacData->interfaceIndex );
                    }
                }
            } // GSM_MS //
            break;

        case GSM_BS:
            {
                short           receivedSlot    = GSM_INVALID_SLOT_NUMBER;

                MacGsmBsInfo*   bsInfo          = gsm->bsInfo;

                // Channel on which message was received has already
                // been extracted from the Info field at the start of
                // this function.
                // Identify the time slot

                // Account for 3 slot (burst period) difference between
                // MS Rx-Tx. Should be adjusted for time advancement.
                if (gsm->bsInfo->slotNumber - 3 < 0 ){
                    receivedSlot = ( short )( bsInfo->slotNumber + 5 );
                }
                else{
                    receivedSlot = ( short )( bsInfo->slotNumber - 3 );
                }

                if (DEBUG_GSM_MAC_1 ){
                    char    clockStr2[100];

                    ctoa( bsInfo->startTimeOfCurrentSlot,clockStr2 );
                    printf( "MAC_GSM[%d] Rcvd on TS %d, fr %d,(starts %s), "
                                "at %s on CH %d\n",
                        node->nodeId,
                        bsInfo->slotNumber,
                        bsInfo->frameNumber,
                        clockStr2,
                        clockStr,
                        receivedChannelIndex );
                }

                if (receivedChannelIndex >=
                       bsInfo->upLinkControlChannelIndex &&
                       receivedChannelIndex <= bsInfo->channelIndexEnd ){
                    char    clockStr2[MAX_CLOCK_STRING_LENGTH];


                    if (DEBUG_GSM_MAC_1 ){
                        ctoa( bsInfo->startTimeOfCurrentSlot,clockStr2 );
                        printf( "GSM_MAC BS[%d] Rcvd Ch %d, TS %d, fr %d, "
                                    "at %s, rxLev %f, ber %f\n",
                            node->nodeId,
                            receivedChannelIndex,
                            receivedSlot,
                            bsInfo->frameNumber,
                            clockStr,
                            rxLevel,
                            ber );
                    }

                    MacGsmUpdateBsDedicatedChMeasurements( node,
                        bsInfo,
                        (uddtSlotNumber)receivedSlot,
                        receivedChannelIndex,
                        rxLevel,
                        ber );
                }
                else{
                    if (DEBUG_GSM_MAC_3 ){
                        sprintf( errorString,
                            "GSM_MAC [%d] ERROR: Received a msg on "
                                "unknown Channel %d at time %s\n",
                            node->nodeId,
                            receivedChannelIndex,
                            clockStr );
                        ERROR_ReportWarning( errorString );
                    }

                    MESSAGE_Free( node,msg );

                    return;
                }

                // Pass up the timing advance value
                if (receivedChannelIndex ==
                       bsInfo->upLinkControlChannelIndex ){
                    clocktype*  info;
                    clocktype   timingAdvance;
                    clocktype   transmissionDelay;


                    transmissionDelay = PHY_GetTransmissionDelay( node,
                                            gsm->myMacData->phyNumber,
                                            MESSAGE_ReturnPacketSize(
                                            msg ) );
                    timingAdvance =
                    currentTime -
                        bsInfo->startTimeOfCurrentSlot - transmissionDelay;

                    info = ( clocktype * )MESSAGE_ReturnInfo( msg );
                    *info = timingAdvance;
                }

                // Send all packets up to layer 3.
                GsmLayer3ReceivePacketFromMac( node,
                    msg,
                    (uddtSlotNumber)receivedSlot,
                    bsInfo->frameNumber,
                    receivedChannelIndex,
                    gsm->myMacData->interfaceIndex );
            } // GSM_BS //
            break;

        default:
            {
                sprintf( errorString,
                    "MAC_GSM [%d] Message for unknown GSM node type\n",
                    node->nodeId );
                ERROR_ReportWarning( errorString );
            } // default //
    }// switch on nodeType //

    MESSAGE_Free( node,msg );
} // End of MacGsmReceivePacketFromPhy //


void MacGsmReceiveInternalMessageFromLayer3(
    Node* node,
    GsmMacLayer3InternalMessage* interLayerMsg,
    int     interfaceIndex)
{
    char            clockStr[MAX_STRING_LENGTH];

    MacDataGsm*     gsm;
    GsmChannelInfo  channelInfo;

    ctoa( node->getNodeTime(),clockStr );

    gsm = ( MacDataGsm * )node->macData[interfaceIndex]->macVar;

    if (gsm->nodeType == GSM_MS ){
        MacGsmMsInfo*   msInfo  = gsm->msInfo;


        switch ( interLayerMsg->servicePrimitive ){
            case  GSM_L3_MAC_CHANNEL_START_LISTEN:
                {
                    memcpy( &channelInfo,
                        interLayerMsg->msgData,
                        sizeof( GsmChannelInfo ) );
                    msInfo->assignedSlotNumber = channelInfo.slotNumber;
                    msInfo->assignedUplinkSlotNumber =
                        (uddtSlotNumber)MacGsmGetUpLinkSlotNumber(
                            channelInfo.slotNumber );

                    MacGsmStartListeningToChannel( node,
                        gsm,
                        gsm->myMacData->phyNumber,
                        channelInfo.channelIndex );
                }
                break; // End of GSM_L3_MAC_CHANNEL_START_LISTEN //

            case GSM_L3_MAC_CHANNEL_STOP_LISTEN:
                {
                    memcpy( &channelInfo,
                        interLayerMsg->msgData,
                        sizeof( GsmChannelInfo ) );

                    MacGsmStopListeningToChannel( node,
                        gsm,
                        gsm->myMacData->phyNumber,
                        channelInfo.channelIndex );
                }
                break; // End of GSM_L3_MAC_CHANNEL_STOP_LISTEN //

            case GSM_L3_MAC_CHANNEL_RELEASE:
                {
                    memcpy( &channelInfo,
                        interLayerMsg->msgData,
                        sizeof( GsmChannelInfo ) );

                    if (channelInfo.channelIndex ==
                           msInfo->assignedChannelIndex &&
                           msInfo->isDedicatedChannelAssigned == TRUE ){
                        MacGsmStopListeningToChannel( node,
                            gsm,
                            gsm->myMacData->phyNumber,
                            ( msInfo->assignedChannelIndex - 1 ) );

                        msInfo->isDedicatedChannelAssigned = FALSE;
                        msInfo->assignedSlotNumber = GSM_INVALID_SLOT_NUMBER;
                        msInfo->assignedUplinkSlotNumber =
                        GSM_INVALID_SLOT_NUMBER;
                        msInfo->assignedChannelIndex =
                        GSM_INVALID_CHANNEL_INDEX;
                        msInfo->assignedChannelType =
                        GSM_INVALID_CHANNEL_TYPE;

                        MacGsmMsStartListeningToBcchListChannels( node,gsm );

                        if (channelInfo.channelType == GSM_CHANNEL_TCH ){
                            msInfo->isCellSelected = FALSE;
                            msInfo->isFcchDetected = FALSE;
                            msInfo->isSchDetected = FALSE;
                            msInfo->isBcchDecoded = FALSE;
                        }

                        if (msInfo->cellSelectionTimer != NULL ){
                            MESSAGE_CancelSelfMsg( node,
                                msInfo->cellSelectionTimer );
                            msInfo->cellSelectionTimer = NULL;
                        }

                        msInfo->cellSelectionTimer = MacGsmStartTimer(
                            node,
                            gsm,
                            MSG_MAC_GsmCellSelectionTimer,
                            DefaultGsmCellSelectionTime );

                    }
                }
                break; // End of GSM_L3_MAC_CHANNEL_RELEASE //

                // Sent by Layer 3 upon decoding BCCH
            case GSM_L3_MAC_CELL_INFORMATION:
                {
/*
                    GsmCellInfo cellInfo;
                    int         j;
                    memcpy( &cellInfo,
                        interLayerMsg->msgData,
                        sizeof( GsmCellInfo ) );

                    for (i = 0; i < msInfo->numControlChannels; i++ ){
                        msInfo->bcchList[i].isNeighChannel = FALSE;
                    }

                    for (i =0; i < msInfo->numNeighBcchChannels; i++){
                        for (j = 0; j < msInfo->numControlChannels; j++){
                            if ((msInfo->bcchList[j].channelIndex ==
                                     cellInfo.neighBcchChannelIndex[i] ) &&
                                   (
                                   msInfo->bcchList[j].isSelected ==
                                FALSE)){
                                msInfo->bcchList[j].isNeighChannel = TRUE;
                                break;
                            }
                        }
                    }
*/
/*
                    for (i = 0; i < msInfo->numControlChannels; i++ ){
                        for (j = 0;
                                j < cellInfo.numNeighBcchChannels;
//                                j < GSM_MAX_NUM_CONTROL_CHANNELS_PER_MS;
                                j++ ){
//                            if (( msInfo->bcchList[i].channelIndex ==
//                                     cellInfo.neighBcchChannelIndex[j] ) &&
//
                                   (
                                   msInfo->bcchList[i].isSelected ==
                                FALSE ) ){
                                msInfo->bcchList[i].isNeighChannel = TRUE;
                                break;
                            }
                        }
                    }
*/

                    msInfo->isBcchDecoded = TRUE;
                }
                break; // End of GSM_L3_MAC_CELL_INFORMATION //

            case GSM_L3_MAC_SET_CHANNEL:
                {
                    memcpy( &channelInfo,
                        interLayerMsg->msgData,
                        sizeof( GsmChannelInfo ) );

                    msInfo->isDedicatedChannelAssigned = TRUE;
                    msInfo->assignedChannelIndex = channelInfo.channelIndex;
                    msInfo->assignedChannelType = channelInfo.channelType;
                    msInfo->assignedSlotNumber = channelInfo.slotNumber;
                    msInfo->assignedUplinkSlotNumber =
                        (uddtSlotNumber)MacGsmGetUpLinkSlotNumber(
                        msInfo->assignedSlotNumber );

                    if (msInfo->assignedChannelType == GSM_CHANNEL_SDCCH ){
                        MacGsmMsStopListeningToBcchListChannels( node,gsm );
                    }
                    else if (msInfo->assignedChannelType ==
                                GSM_CHANNEL_TCH ){
                        // While on TCH initiate listening of neighbour BCCH
                        // channels during the idle period.
                        if (msInfo->slotNumber ==
                               msInfo->assignedSlotNumber ){
                            // Only time this message is expected is
                            // during the assigned Receiving slot.
                            char    clockStr2[MAX_STRING_LENGTH];

                            if (DEBUG_GSM_MAC_1 ){
                                ctoa( msInfo->currentSlotStartTime,
                                    clockStr2 );
                                printf(
                                    "MS [%d] Start IdleSlot "
                                        "TS %d(starts at %s) fr%d at %s\n",
                                    node->nodeId,
                                    msInfo->slotNumber,
                                    clockStr2,
                                    msInfo->frameNumber,
                                    clockStr );
                            }

                            msInfo->isIdleSlot_Rx_Tx_Interval = TRUE;

                            // Start timer at the end of this(Rx/DL) slot
                            // Bug # 3436 fix - start
                            gsm->idleTimerMsg = MacGsmStartTimer( node,
                                gsm,
                                MSG_MAC_GsmIdleSlotStartTimer,
                                DefaultGsmSlotDuration -
                                (node->getNodeTime() - msInfo->currentSlotStartTime ) );
                            // Bug # 3436 fix - end
                        }
                    }

                    MacGsmStartListeningToChannel( node,
                        gsm,
                        gsm->myMacData->phyNumber,
                        ( channelInfo.channelIndex - 1 ) );
                }
                break; // End of GSM_L3_MAC_SET_CHANNEL//

            case GSM_L3_MAC_HANDOVER:
                {
                    int             n;
                    int             frameDiv;

                    clocktype       timeDiff;
                    clocktype       startTimeOfNextFrame;

                    GsmHandoverInfo hoInfo;


                    memcpy( &hoInfo,
                        interLayerMsg->msgData,
                        sizeof( GsmHandoverInfo ) );

                    // slotTimer sync
                    for (n = 0; n < msInfo->numBcchChannels; n++ ){
                        if (msInfo->bcchList[n].channelIndex ==
                               hoInfo.bcchChannelIndex ){
                            msInfo->isHandoverInProgress = TRUE;
                            msInfo->hoCellBcchEntry = &(
                                msInfo->bcchList[n] );
                            break;
                        }
                    }

                    if (n == msInfo->numBcchChannels ){
                        ERROR_ReportError(
                            "Slot timer is not synchronized" );
                    }
                    // Bug # 3436 fix - start
                    for (int i = 0; i < msInfo->numBcchChannels; i++ )
                    {
                        if (msInfo->bcchList[i].channelIndex ==
                             hoInfo.channelIndex - 1 )
                        {
                            msInfo->bcchList[i].isNeighChannel = FALSE;
                        }
                        if (msInfo->bcchList[i].channelIndex ==
                             msInfo->assignedChannelIndex - 1 )
                        {
                             msInfo->bcchList[i].isNeighChannel = TRUE;
                        }
                    }
                    // Bug # 3436 fix - end
                    MacGsmStopListeningToChannel( node,
                        gsm,
                        gsm->myMacData->phyNumber,
                        ( msInfo->assignedChannelIndex - 1 ) );

                    MacGsmStartListeningToChannel( node,
                        gsm,
                        gsm->myMacData->phyNumber,
                        ( hoInfo.channelIndex - 1 ) );

                    msInfo->assignedChannelIndex = hoInfo.channelIndex;
                    msInfo->assignedSlotNumber = hoInfo.slotNumber;
                    msInfo->assignedUplinkSlotNumber =
                        (uddtSlotNumber)MacGsmGetUpLinkSlotNumber(
                        msInfo->assignedSlotNumber );

                    timeDiff = node->getNodeTime() -
                                   msInfo->bcchList[n].startTimeOfNextFrame;

                    frameDiv = ( int )( timeDiff / DefaultGsmFrameDuration );

                    // sync for the new BS's slot 1
                    startTimeOfNextFrame =
                    msInfo->bcchList[n].startTimeOfNextFrame +
                        DefaultGsmFrameDuration *
                        frameDiv + DefaultGsmFrameDuration;

                    msInfo->slotNumber = 0; // has passed
                    msInfo->frameNumber =
                    msInfo->bcchList[n].nextFrameNumber + frameDiv + 1;

                    msInfo->controlFrameNumber = (
                        short )(
                        msInfo->frameNumber % GSM_CONTROL_MULTIFRAME_SIZE );
                    msInfo->trafficFrameNumber = (
                        short )(
                        msInfo->frameNumber % GSM_TRAFFIC_MULTIFRAME_SIZE );

                    // set timer to start up then
                    MacGsmStartTimer( node,
                        gsm,
                        MSG_MAC_GsmHandoverTimer,
                        startTimeOfNextFrame - node->getNodeTime() );
                    //- 1 * MICRO_SECOND);

                    // Bug # 3436 fix - start
                    if (gsm->idleTimerMsg != NULL)
                    {
                        MESSAGE_CancelSelfMsg( node, gsm->idleTimerMsg );
                        gsm->idleTimerMsg = NULL;
                    }
                    if (gsm->slotTimerMsg != NULL)
                    {
                       MESSAGE_CancelSelfMsg( node, gsm->slotTimerMsg );
                       gsm->slotTimerMsg = NULL;
                    }
                    // Bug # 3436 fix - end
                    msInfo->assignedChannelType = hoInfo.channelType;
                    msInfo->downLinkControlChannelIndex =
                    hoInfo.bcchChannelIndex;
                    msInfo->upLinkControlChannelIndex = (
                        short )( hoInfo.bcchChannelIndex + 1 );

                    //msInfo->isCellSelected = FALSE;
                }
                break; // End of GSM_L3_MAC_HANDOVER //
                // MS comes in to idel state, and send the request for cell selection
            case  GSM_L3_MAC_CELL_SELECTION:
                {

                    msInfo->nextBcchChannelToListen = 0;

                    for (int i = 0; i < msInfo->numControlChannels; i++ ){
                        msInfo->bcchList[i].channelIndex =
                        msInfo->controlChannels[i];
                        msInfo->bcchList[i].isNeighChannel = FALSE;
                        msInfo->bcchList[i].isSelected = FALSE;
                        msInfo->bcchList[i].rxLevel_dBm = 0.0;
                        msInfo->bcchList[i].ber = 0.0;
                        msInfo->bcchList[i].numRecorded = 0;
                        msInfo->bcchList[i].lastUpdatedAt = 0;
                        msInfo->bcchList[i].lastDecodedAt = 0;
                        msInfo->bcchList[i].C1 = 0.0;
                    }

                    // start list listing channel
                    MacGsmMsStartListeningToBcchListChannels( node, gsm );
                    // start cell selection process
                    // Start timer for cell selection

                    if (msInfo->cellSelectionTimer != NULL ){
                        MESSAGE_CancelSelfMsg( node,
                            msInfo->cellSelectionTimer );
                        msInfo->cellSelectionTimer = NULL;
                    }

                    msInfo->cellSelectionTimer = MacGsmStartTimer(
                        node,
                        gsm,
                        MSG_MAC_GsmCellSelectionTimer,
                        15 * SECOND );

                    msInfo->isDedicatedChannelAssigned = FALSE;
                    msInfo->isCellSelected = FALSE;
                    msInfo->timeSinceCellSelected = node->getNodeTime();

                    // Should be initialized to 0 so that when MS receives the
                    // BS beacon messages (Sys Info Type 3) it can correctly
                    // identify the channel type (based on slot & frame number)
                    msInfo->slotNumber = 0;
                    msInfo->controlFrameNumber = 0;
                    msInfo->frameNumber = 0;
                    setGSMNodeLAC( node,GSM_INVALID_LAC_IDENTITY );
                    setGSMNodeCellId( node,GSM_INVALID_CELL_IDENTITY );
                }
                break; // End of GSM_L3_MAC_CellSelection //
        }; // End of servicePrimitive //
    }  // End of GSM_MS //
    else if (gsm->nodeType == GSM_BS ){
        MacGsmBsInfo*   bsInfo  = gsm->bsInfo;


        switch ( interLayerMsg->servicePrimitive ){
                // Sent by Layer 3 upon its initialization
            case GSM_L3_MAC_CELL_INFORMATION:
                {
                    int         i;
                    int         phys;
                    Address     address;
                    NodeAddress interfaceAddress;
                    GsmCellInfo cellInfo;


                    memcpy( &cellInfo,
                        interLayerMsg->msgData,
                        sizeof( GsmCellInfo ) );

                    setGSMNodeLAC( node,cellInfo.lac );
                    setGSMNodeCellId( node,cellInfo.cellIdentity );

                    bsInfo->channelIndexStart = cellInfo.channelIndexStart;
                    bsInfo->channelIndexEnd = cellInfo.channelIndexEnd;
                    bsInfo->numChannels = cellInfo.channelIndexEnd -
                                              cellInfo.channelIndexStart + 1;
                    bsInfo->numNeighBcchChannels =
                    cellInfo.numNeighBcchChannels;
                    gsm->txInteger = cellInfo.txInteger;
                    gsm->maxReTrans = cellInfo.maxReTrans;

                    if (DEBUG_GSM_MAC_3 ){
                        printf( "GSM_MAC [%d] %d Neigh.Channel(s):\n",
                            node->nodeId,
                            cellInfo.numNeighBcchChannels );
                    }

                    for (i = 0; i < cellInfo.numNeighBcchChannels; i++ ){
                        if (DEBUG_GSM_MAC_3 ){
                            printf( "\tBCCH %d\n",
                                cellInfo.neighBcchChannelIndex[i] );
                        }

                        bsInfo->neighBcchChannels[i] = (
                            short )( cellInfo.neighBcchChannelIndex[i] );
                    }

                    // Create additional phys for BS, 1 per channel.
                    // 1 has already been created during mac.c initialization.
                    interfaceAddress = MAPPING_GetInterfaceAddressForInterface(
                                           node,
                                           node->nodeId,
                                           gsm->myMacData->interfaceIndex );

                    SetIPv4AddressInfo( &address,interfaceAddress );

                    // the first/default phy has been created during mac init.
                    for (phys = 0;
                            phys < ( bsInfo->numChannels - 1 );
                            phys++ ){
                        //printf("GSM[%ld]: Creating dev %d/%d\n",
                        //    node->nodeId, phys, bsInfo->numChannels);
                        PHY_CreateAPhyForMac( node,
                            node->partitionData->nodeInput,
                            interfaceIndex,
                            &address,
                            PHY_GSM,
                            &node->macData[interfaceIndex]->phyNumber );

                        for (Int32 channelIndex = 0;
                             channelIndex < PROP_NumberChannels(node);
                             channelIndex++)
                        {
                            MacGsmStopListeningToChannel(
                                                 node,
                                                 gsm,
                                                 gsm->myMacData->phyNumber,
                                                 channelIndex);
                        }
                    }

                    MacGsmStartListeningToChannel( node,
                        gsm,
                        MacGsmBsGetPhyNumberFromChannel( node,
                            bsInfo,
                            gsm->bsInfo->downLinkControlChannelIndex + 1 ),
                        gsm->bsInfo->downLinkControlChannelIndex + 1 );

                    // pre-build often used messages
                    GsmMacBsBuildSysInfoType2Msg( node,gsm );

                    GsmMacBsBuildSysInfoType3Msg( node,gsm );

                    GsmMacBsBuildDummyBurstMsg( node,gsm );
                }
                break; // End of GSM_L3_MAC_CELL_INFORMATION //

            case GSM_L3_MAC_CHANNEL_START_LISTEN:
                {
                    memcpy( &channelInfo,
                        interLayerMsg->msgData,
                        sizeof( GsmChannelInfo ) );
                    MacGsmStartListeningToChannel( node,
                        gsm,
                        MacGsmBsGetPhyNumberFromChannel( node,
                            bsInfo,
                            channelInfo.channelIndex ),
                        channelInfo.channelIndex );
                }
                break; // End of GSM_L3_MAC_CHANNEL_START_LISTEN //

            case GSM_L3_MAC_CHANNEL_STOP_LISTEN:
                {
                    memcpy( &channelInfo,
                        interLayerMsg->msgData,
                        sizeof( GsmChannelInfo ) );
                    MacGsmStopListeningToChannel( node,
                        gsm,
                        MacGsmBsGetPhyNumberFromChannel( node,
                            bsInfo,
                            channelInfo.channelIndex ),
                        channelInfo.channelIndex );
                }
                break; // End of GSM_L3_MAC_CHANNEL_STOP_LISTEN //

            case GSM_L3_MAC_MEASUREMENT_REPORT:
                {
                    int                 chPair;
                    GsmMeasurementInfo  measInfo;


                    memcpy( &measInfo,
                        interLayerMsg->msgData,
                        sizeof( GsmMeasurementInfo ) );
                    chPair = ( measInfo.channelIndex -
                                 bsInfo->upLinkControlChannelIndex ) / 2;

                    measInfo.callBackFuncPtr(
                        node,
                        measInfo.slotNumber,
                        measInfo.channelIndex,
                        &(
                        bsInfo->upLinkChMeas[measInfo.slotNumber][chPair] ) );
                    // Bug # 3436 fix - start
                    bsInfo->supportFlag = TRUE;
                    // Bug # 3436 fix - end
                }
                break; // End of GSM_L3_MAC_MEASUREMENT_REPORT //
        };
    } // End of GSM_BS //
} // End of MacGsmReceiveInternalMessageFromLayer3 //


void MacGsmReceivePhyStatusChangeNotification(
    Node* node,
    MacDataGsm* gsm,
    PhyStatusType oldPhyStatus,
    PhyStatusType newPhyStatus)
{
}


/*
 * FUNCTION    MacGsmLayer
 * PURPOSE     Models the behaviour of the GSM MAC Layer
 *             on receiving the message enclosed in msg.
 *
 * Parameters:
 *     node:     node which received the message
 *      msg:     message received by the layer
 *
 * Return:      None.
 *
 * Assumption: Regardless of the slot duration time, the basic slot unit
 *             is a packet. This need not be the case.
 *             TBD: Use only slot duration length as determinant of data
 *             to be sent, including fragmentation
 */
void MacGsmLayer(
    Node* node,
    int         interfaceIndex,
    Message* msg)
{
    char        clockStr[MAX_STRING_LENGTH];
    char        errorString[MAX_STRING_LENGTH];


    // The most common timer is the slot timer. The MS & BS reuse it
    // and hence it is not 'freed'.

    MacDataGsm* gsm = ( MacDataGsm* )node->macData[interfaceIndex]->macVar;

    ctoa(node->getNodeTime(), clockStr);

    if (gsm->nodeType == GSM_MS ){
        switch ( msg->eventType ){
            case MSG_MAC_GsmSlotTimer:
                {
                    MacGsmHandleMsSlotTimer( node,gsm,msg );
                }
                break;

            case MSG_MAC_GsmCellSelectionTimer:
                {
                    if (DEBUG_GSM_MAC_0 ){
                        printf( "GSM_MAC[%d]: Cell Selection Timer, %d %s\n",
                            node->nodeId,
                            gsm->msInfo->isCellSelected,
                            clockStr );
                    }

                    MacGsmCheckForCellSelection( node,gsm );
                }
                break;

            case MSG_MAC_GsmIdleSlotStartTimer:
                {
                    MESSAGE_Free( node,msg );
                    MacGsmHandleTchIdleSlotStartTimer( node,gsm );
                }
                break;

            case MSG_MAC_GsmIdleSlotEndTimer:
                {
                    MESSAGE_Free( node,msg );
                    MacGsmHandleTchIdleSlotEndTimer( node,gsm );
                }
                break;

            case MSG_MAC_GsmHandoverTimer:
                {
                    if (DEBUG_GSM_MAC_3 ){
                        printf(
                            "GSM_MAC[%d]: HOTimer TS %d, fr %d, time %s\n",
                            node->nodeId,
                            gsm->msInfo->slotNumber,
                            gsm->msInfo->frameNumber,
                            clockStr );
                    }

                    MESSAGE_Free( node,msg );
                    // Bug # 3436 fix - start
                    gsm->slotTimerMsg = MacGsmStartTimer( node,
                        gsm,
                        MSG_MAC_GsmSlotTimer,
                        DefaultGsmSlotDuration );
                    gsm->idleTimerMsg = MacGsmStartTimer( node,
                                        gsm,
                                        MSG_MAC_GsmIdleSlotStartTimer,
                                        (gsm->msInfo->assignedSlotNumber + 1)
                                        *DefaultGsmSlotDuration );
                    // Bug # 3436 fix - end
                    gsm->msInfo->isHandoverInProgress = FALSE;
                }
                break;

            case MSG_MAC_GsmTimingAdvanceDelayTimer:
                {
                    MESSAGE_Free( node,msg );
                    // timing advance delayed message
                    GsmMacSendMessageOverPhy( node,
                        gsm->msInfo->msgToSend,
                        gsm->myMacData->phyNumber,
                        gsm->msInfo->assignedChannelIndex,
                        DefaultGsmSlotDuration );
                    gsm->msInfo->msgToSend = NULL;
                }
                break;

            default:
                {
                    sprintf( errorString,
                        "GSM_MAC[%d] ERROR: Unknown timer 0x%x",
                        node->nodeId,
                        msg->eventType );
                    ERROR_ReportWarning( errorString );
                    MESSAGE_Free( node,msg );
                }
        }
    } // End of GSM_MS //

    else if (gsm->nodeType == GSM_BS ){
        switch ( msg->eventType ){
            case MSG_MAC_GsmSlotTimer:
                {
                    MacGsmHandleBsSlotTimer( node,gsm,msg );
                }
                break;

            default:
                {
                    sprintf( errorString,
                        "GSM_MAC[%d] ERROR: Unknown timer 0x%x",
                        node->nodeId,
                        msg->eventType );
                    ERROR_ReportWarning( errorString );
                    MESSAGE_Free( node,msg );
                }
        };
    } // GSM_BS //
    else{
        sprintf( errorString,
            "Node[%d] ERROR: GSM_MAC unknown node type",
            node->nodeId );
        ERROR_ReportWarning( errorString );

        MESSAGE_Free( node,msg );
    }
} //End of MacGsmLayer //


/*
 * FUNCTION    MacGsmFinalize
 * PURPOSE     Called at the end of simulation to collect the results of
 *             the simulation of the TDMA protocol of MAC Layer.
 *
 * Parameter:
 *     node:     node for which results are to be collected.
 */
void MacGsmFinalize(
    Node* node,
    int interfaceIndex)
{
    char        buf[MAX_STRING_LENGTH];

    MacDataGsm* gsm = ( MacDataGsm* )node->macData[interfaceIndex]->macVar;

    if (gsm->collectStatistics == FALSE ){
        return;
    }

    if (gsm->nodeType == GSM_MS ){
        sprintf( buf,
            "Cell Selections = %d",
            gsm->msInfo->stats.numCellSelections );
        IO_PrintStat( node,"MAC","GSM_MS",ANY_DEST,interfaceIndex,buf );

        sprintf( buf,
            "Cell Selection Failures = %d",
            gsm->msInfo->stats.numCellSelectionsFailed );
        IO_PrintStat( node,"MAC","GSM_MS",ANY_DEST,interfaceIndex,buf );

        sprintf( buf,
            "Cell Reselection Attempts = %d",
            gsm->msInfo->stats.numCellReSelectionAttempts );
        IO_PrintStat( node,"MAC","GSM_MS",ANY_DEST,interfaceIndex,buf );
    }
    else if (gsm->nodeType == GSM_BS)
    {
        if (gsm->slotTimerMsg && !gsm->slotTimerMsg->getSent())
            MESSAGE_Free(node, gsm->slotTimerMsg);
        if (gsm->idleTimerMsg && !gsm->idleTimerMsg->getSent())
            MESSAGE_Free(node, gsm->idleTimerMsg);
    }
}


// Map the averaged RxLev (dBm) to the GSM format
// (0-63) used in Measurement Reports.
// Reference GSM 05.08 Section 8.1.4
unsigned char
GsmMapRxLev(
    float rxLev_dBm)
{
    if (rxLev_dBm < -110.0 ){
        return 0;
    }
    else if (rxLev_dBm > -48.0 ){
        return 63;
    }

    return ( unsigned char )( rxLev_dBm + 111.0 );
} // End of GsmMapRxLev //


// Map the averaged ber to the GSM format
// (0-7) used in Measurement Reports.
// See GSM 05.08 Section 8.2.4
unsigned char
GsmMapRxQual(
double ber)
{
    int i;

    for (i = 0; i < 6; i++ ){
        if (ber < pow( 2.0,i ) * 0.002 ){
            return ( unsigned char )i;
        }
    }

    // for all other ber values > 12.8
    return 7;
} // End of GsmMapRxQual //


// GSM 05.05 Section 4.11
// powerControlLevel range is 0-31 for GSM 900
// returns the output power in dBm
float
mapGsmPowerControlLevelToDbm(
    int   powerControlLevel)
{
    if (powerControlLevel <= 2 ){
        return 39.0;
    }
    else if (powerControlLevel >= 19 && powerControlLevel <= 31 ){
        return 5.0;
    }

    return ( float )( 39.0 - ( powerControlLevel - 2 ) * 2.0 );
} // End of mapGsmPowerControlLevelToDbm //


/*
 * FUNCTION:    MacGsmNetworkLayerHasPacketToSend.
 *
 * PURPOSE:     Retrieve packet to send from network layer
 *
 * RETURN:      None.
 *
 */
void MacGsmNetworkLayerHasPacketToSend(
    Node* node,
    MacDataGsm* gsm)
{
}


/* NOTE:
 * FUNCTION:    MacGsmHandlePromiscuousMode.
 *
 * PURPOSE:     Supports promiscuous mode sending remote packets to
 *              upper layers.
 *
 * PARAMETERS:  node, node using promiscuous mode.
 *              frame, packet to send to upper layers.
 *
 * RETURN:      None.
 *
 * ASSUMPTION:  node != NULL.
 */
void MacGsmHandlePromiscuousMode(
    Node* node,
    MacDataGsm* gsm,
    Message* frame)
{
}

/*** End of file mac_gsm.c ***/
