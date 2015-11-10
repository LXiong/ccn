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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "antenna.h"
#include "antenna_switched.h"
#include "antenna_steerable.h"
#include "antenna_patterned.h"
#include "phy_gsm.h"


static //inline//
void PhyGsmLockSignal(
    Node* node,
    PhyDataGsm* phyGsm,
    Message* msg,
    double rxPower_mW)
{
    phyGsm->rxMsg = msg;
    phyGsm->rxMsgPower_mW = rxPower_mW;
    phyGsm->rxTimeEvaluated = node->getNodeTime();
}



static //inline//
void PhyGsmUnlockSignal(
    PhyDataGsm* phyGsm)
{
    phyGsm->rxMsg = NULL;
    phyGsm->rxMsgPower_mW = 0.0;
    phyGsm->rxTimeEvaluated = 0;
}


static /*inline*/
BOOL PhyGsmCarrierSensing(
    PhyDataGsm* phyGsm)
{
    if (phyGsm->interferencePower_mW + phyGsm->noisePower_mW >
           phyGsm->rxSensitivity_mW) {
        return TRUE;
    }
    return FALSE;
}


static //inline//
BOOL PhyGsmCheckRxPacketError(
    Node* node,
    PhyDataGsm* phyGsm)
{
    double  sinr;
    double  BER;
    double  noise   = phyGsm->thisPhy->noise_mW_hz*
                          PHY_GSM_CHANNEL_BANDWIDTH;

    sinr = (phyGsm->rxMsgPower_mW /
               (phyGsm->interferencePower_mW + noise));

    BER = PHY_BER(phyGsm->thisPhy, 0,/* ber table index */
            sinr);

    if (BER != 0.0) {
        double  numBits             = (
                 (double) (node->getNodeTime() -
                 phyGsm->rxTimeEvaluated) *
                 (double) phyGsm->dataRate /
                 (double) SECOND);

        double  errorProbability    = 1.0 - pow((1.0 - BER), numBits);
        double  rand                = RANDOM_erand(phyGsm->thisPhy->seed);

        assert((errorProbability >= 0.0) && (errorProbability <= 1.0));

        if (errorProbability > rand) {
            return TRUE;
        }
    }

    return FALSE;
}



static
void PhyGsmChangeState(
Node* node,
int phyIndex,
PhyStatusType newStatus)
{
        PhyData* thisPhy = node->phyData[phyIndex];
        PhyDataGsm *phyGsm = (PhyDataGsm *)thisPhy->phyVar;

        phyGsm->previousMode = phyGsm->mode;
        phyGsm->mode = newStatus;

        Phy_ReportStatusToEnergyModel(
        node,
        phyIndex,
        phyGsm->previousMode,
        newStatus);
}

void PhyGsmInit(
    Node* node,
    const int phyIndex,
    const NodeInput* nodeInput)
{
    double      rxSensitivity_dBm;
    double      rxThreshold_dBm;
    double      txPower_dBm;
    int         dataRate;
    BOOL        wasFound;

    PhyDataGsm* phyGsm  = (PhyDataGsm*) MEM_malloc(sizeof(PhyDataGsm));
    memset(phyGsm, 0, sizeof(PhyDataGsm));
    node->phyData[phyIndex]->phyVar = (void*) phyGsm;

    phyGsm->thisPhy = node->phyData[phyIndex];

    //
    // Antenna model initialization
    //
    ANTENNA_Init(node, phyIndex, nodeInput);

    //
    // Set PHY-GSM-DATA-RATE
    //
    IO_ReadInt(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY-GSM-DATA-RATE",
        &wasFound,
        &dataRate);

    if (wasFound) {
        phyGsm->dataRate = dataRate;
    }
    else{
        phyGsm->dataRate = PHY_GSM_DEFAULT_DATA_RATE;
    }

    //
    // Set PHY-GSM-TX-POWER
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY-GSM-TX-POWER",
        &wasFound,
        &txPower_dBm);

    if (wasFound) {
        phyGsm->txPower_dBm = (float) txPower_dBm;
    }
    else{
        phyGsm->txPower_dBm = PHY_GSM_DEFAULT_TX_POWER_dBm;
    }


    //
    // Set PHY-GSM-RX-SENSITIVITY
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY-GSM-RX-SENSITIVITY",
        &wasFound,
        &rxSensitivity_dBm);

    if (wasFound) {
        phyGsm->rxSensitivity_mW = NON_DB(rxSensitivity_dBm);
    }
    else {
        phyGsm->rxSensitivity_mW = NON_DB(
            PHY_GSM_DEFAULT_RX_SENSITIVITY_dBm);
    }


    //
    // Set PHY-GSM-RX-THRESHOLD
    //
    IO_ReadDouble(
        node,
        node->nodeId,
        node->phyData[phyIndex]->macInterfaceIndex,
        nodeInput,
        "PHY-GSM-RX-THRESHOLD",
        &wasFound,
        &rxThreshold_dBm);

    if (wasFound) {
        phyGsm->rxThreshold_mW = NON_DB(rxThreshold_dBm);
    }
    else{
        phyGsm->rxThreshold_mW = NON_DB(PHY_GSM_DEFAULT_RX_THRESHOLD_dBm);
    }


    //
    // Initialize phy statistics variables
    //
    phyGsm->stats.totalRxSignalsToMac = 0;
    phyGsm->stats.totalTxSignals = 0;
    phyGsm->stats.energyConsumed = 0.0;
    phyGsm->stats.turnOnTime = node->getNodeTime();
#ifdef CYBER_LIB
    phyGsm->stats.totalRxSignalsToMacDuringJam = 0;
#endif

    //
    // Initialize status of phy
    //
    phyGsm->rxMsg = NULL;
    phyGsm->rxMsgPower_mW = 0.0;
    phyGsm->interferencePower_mW = 0.0;
    phyGsm->noisePower_mW = phyGsm->thisPhy->noise_mW_hz *
                                PHY_GSM_CHANNEL_BANDWIDTH;
    phyGsm->rxTimeEvaluated = 0;
    phyGsm->previousMode = PHY_IDLE;
    phyGsm->mode = PHY_IDLE;
    PhyGsmChangeState(node, phyIndex, PHY_IDLE);

    return;
}



void PhyGsmTransmissionEnd(
    Node* node,
    int phyIndex)
{
    PhyData*    thisPhy = node->phyData[phyIndex];
    PhyDataGsm* phyGsm  = (PhyDataGsm*) thisPhy->phyVar;
    int         channelIndex;

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    //GuiStart
    if (node->guiOption == TRUE) {
        GUI_EndBroadcast(node->nodeId,
            GUI_PHY_LAYER,
            GUI_DEFAULT_DATA_TYPE,
            thisPhy->macInterfaceIndex,
            node->getNodeTime());
    }
    //GuiEnd

    // Updated the following line from previous version, probably 802.11
    // feature PHY_StartListeningToChannel(node, phyIndex, channelIndex);


    PHY_SignalInterference(node,
        phyIndex,
        channelIndex,
        NULL,
        NULL,
        &(phyGsm->interferencePower_mW));

    if (PhyGsmCarrierSensing(phyGsm) == TRUE) {
        PhyGsmChangeState(node, phyIndex, PHY_SENSING);
    }
    else{
        PhyGsmChangeState(node, phyIndex, PHY_IDLE);
    }
}



void PhyGsmFinalize(
    Node* node,
    const int phyIndex)
{
    PhyData*    thisPhy = node->phyData[phyIndex];
    PhyDataGsm* phyGsm  = (PhyDataGsm*)thisPhy->phyVar;
    char        buf[MAX_STRING_LENGTH];

    if (thisPhy->phyStats == FALSE) {
        return;
    }

    assert(thisPhy->phyStats == TRUE);

    sprintf(buf, "Signals transmitted = %d", phyGsm->stats.totalTxSignals);
    IO_PrintStat(node, "Physical", "GSM", ANY_DEST, phyIndex, buf );

    sprintf(buf,
        "Signals received and forwarded to MAC = %d",
        phyGsm->stats.totalRxSignalsToMac);
    IO_PrintStat(node, "Physical", "GSM", ANY_DEST, phyIndex, buf);
#ifdef CYBER_LIB
    if (node->phyData[phyIndex]->jammerStatistics == TRUE)
    {
        char durationStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->phyData[phyIndex]->jamDuration, durationStr, node);
        sprintf(buf, "Signals received and forwarded to MAC during the jam duration = %d",
            phyGsm->stats.totalRxSignalsToMacDuringJam);
        IO_PrintStat(node, "Physical", "GSM", ANY_DEST, phyIndex, buf);
        sprintf(buf, "Total jam duration in (s) = %s", durationStr);
        IO_PrintStat(node, "Physical", "GSM", ANY_DEST, phyIndex, buf);
    }
#endif

    PhyGsmChangeState(node, phyIndex, phyGsm->mode);
}



void PhyGsmSignalArrivalFromChannel(
    Node* node,
    int phyIndex,
    uddtChannelIndex channelIndex,
    PropRxInfo* propRxInfo)
{
    PhyData*                thisPhy         = node->phyData[phyIndex];
    PhyDataGsm*             phyGsm          = (PhyDataGsm*)thisPhy->phyVar;

    double                  rxAntennaGain   = 0.0;

    double                  rxPower_mW      = NON_DB(
            ANTENNA_GainForThisSignal(node,
                phyIndex,
                propRxInfo) + propRxInfo->rxPower_dBm);

    switch (phyGsm->mode) {
        case PHY_RECEIVING:
        {
            BOOL errorOccurred = PhyGsmCheckRxPacketError(node, phyGsm);
            phyGsm->rxMsgError = errorOccurred;

            if ((errorOccurred == TRUE) ||
                (rxPower_mW >
                phyGsm->rxMsgPower_mW * PHY_GSM_CAPTURE_THRESHOLD))
            {
                // Should update some stat for this
                //ctoa(node->getNodeTime(), clockStr);
                //printf("GSM_PHY [%ld]: Error occurred in packet at %s\n",
                //    node->nodeId, clockStr);

                if (thisPhy->antennaData->antennaModelType
                    == ANTENNA_OMNIDIRECTIONAL)
                {
                    phyGsm->interferencePower_mW
                        += phyGsm->rxMsgPower_mW;
                }
                else{
                    ANTENNA_SetToDefaultMode(node, phyIndex);

                    PHY_SignalInterference(node,
                        phyIndex,
                        channelIndex,
                        NULL,
                        NULL,
                        &(phyGsm->interferencePower_mW));
                }
                PhyGsmUnlockSignal(phyGsm);

                PhyGsmChangeState(node, phyIndex, PHY_SENSING);

                //
                // Go down to the next case statement without break
                //
            }
            else
            {
                phyGsm->rxTimeEvaluated = node->getNodeTime();
                phyGsm->interferencePower_mW += rxPower_mW;

                break;
            }
        }

        //
        // If the phy is idle or sensing,
        // check if it can receive this signal.
        //
        case PHY_IDLE:
        case PHY_SENSING:
        {
            rxAntennaGain = ANTENNA_DefaultGainForThisSignal
                (node, phyIndex, propRxInfo);
            rxPower_mW = NON_DB( propRxInfo->rxPower_dBm + rxAntennaGain);
            if (rxPower_mW >= phyGsm->rxSensitivity_mW)
            {
                if (!ANTENNA_IsLocked(node, phyIndex))
                {
                    ANTENNA_SetToBestGainConfigurationForThisSignal(
                            node, phyIndex, propRxInfo);
                    rxAntennaGain = ANTENNA_GainForThisSignal
                        (node, phyIndex, propRxInfo);
                    rxPower_mW = NON_DB
                        ( propRxInfo->rxPower_dBm + rxAntennaGain);

                }

                if (rxPower_mW >= phyGsm->rxThreshold_mW)
                {
                    PHY_SignalInterference(node,
                        phyIndex,
                        channelIndex,
                        propRxInfo->txMsg,
                        &rxPower_mW,
                        &(phyGsm->interferencePower_mW));

                    PhyGsmLockSignal(node,
                        phyGsm,
                        propRxInfo->txMsg,
                        rxPower_mW);
                    PhyGsmChangeState(node, phyIndex, PHY_RECEIVING);


                }
            }
            //
            // Otherwise, check if the signal change its status
            //
            else if (rxPower_mW <= phyGsm->rxThreshold_mW)
            {
                phyGsm->interferencePower_mW += rxPower_mW;

                if (phyGsm->mode == PHY_IDLE
                    && PhyGsmCarrierSensing(phyGsm) == TRUE) {
                    PhyGsmChangeState(node, phyIndex, PHY_SENSING);
                }
            }

            break;
        }

        default:
        {
            break;
        }
    }//switch (phyGsm->mode)//
}



void PhyGsmSignalEndFromChannel(
    Node* node,
    int phyIndex,
    uddtChannelIndex channelIndex,
    PropRxInfo* propRxInfo)
{
    PhyData*    thisPhy = node->phyData[phyIndex];
    PhyDataGsm* phyGsm  = (PhyDataGsm*) thisPhy->phyVar;

    if (phyGsm->mode == PHY_RECEIVING)
    {
        BOOL    errorOccurred   = PhyGsmCheckRxPacketError(node, phyGsm);
        phyGsm->rxMsgError = errorOccurred;

        if (errorOccurred == TRUE)
        {
            if (thisPhy->antennaData->antennaModelType
                == ANTENNA_OMNIDIRECTIONAL) {
                phyGsm->interferencePower_mW += phyGsm->rxMsgPower_mW;
            }
            else
            {
                ANTENNA_SetToDefaultMode(node, phyIndex);

                PHY_SignalInterference(node,
                    phyIndex,
                    channelIndex,
                    NULL,
                    NULL,
                    &(phyGsm->interferencePower_mW));
            }
            PhyGsmUnlockSignal(phyGsm);
            PhyGsmChangeState(node, phyIndex, PHY_SENSING);

        }
        else{
            phyGsm->rxTimeEvaluated = node->getNodeTime();
        }
    }

    //
    // If the phy is still receiving this signal, forward the frame
    // to the MAC layer.
    //
    if (phyGsm->mode == PHY_RECEIVING
        && phyGsm->rxMsg == propRxInfo->txMsg)
    {
        Message*        newMsg;
        unsigned int*   info;

        if (thisPhy->antennaData->antennaModelType
            != ANTENNA_OMNIDIRECTIONAL)
        {
            ANTENNA_SetToDefaultMode(node, phyIndex);

            PHY_SignalInterference(node,
                phyIndex,
                channelIndex,
                NULL,
                NULL,
                &(phyGsm->interferencePower_mW));
        }
        else{
            phyGsm->rxLev = (float) propRxInfo->rxPower_dBm;
        }

        PhyGsmUnlockSignal(phyGsm);

        if (PhyGsmCarrierSensing(phyGsm) == TRUE) {
            PhyGsmChangeState(node,phyIndex,PHY_SENSING);
        }
        else{
            PhyGsmChangeState(node, phyIndex, PHY_IDLE);
        }
        if (phyGsm->rxMsgError != TRUE) {
            newMsg = MESSAGE_Duplicate(node, propRxInfo->txMsg);

            // Add Received Channel Id
            MESSAGE_InfoAlloc(node, newMsg, sizeof(unsigned int));

            info = (NodeAddress*) MESSAGE_ReturnInfo(newMsg);
            *info = channelIndex;

            MESSAGE_SetInstanceId(newMsg, (short) phyIndex);

            MAC_ReceivePacketFromPhy(node,
                node->phyData[phyIndex]->macInterfaceIndex,
                newMsg);

            phyGsm->stats.totalRxSignalsToMac++;
#ifdef CYBER_LIB
            if (node->phyData[phyIndex]->jammerStatistics == TRUE)
            {
                if (node->phyData[phyIndex]->jamInstances > 0)
                {
                    phyGsm->stats.totalRxSignalsToMacDuringJam++;
                }
            }
#endif
        }
    }
    else{
        double  rxPower_mW  = NON_DB(ANTENNA_GainForThisSignal(node,
                                          phyIndex,
                                          propRxInfo) +
                                  propRxInfo->rxPower_dBm);

        phyGsm->interferencePower_mW -= rxPower_mW;

        if (phyGsm->interferencePower_mW < 0.0) {
            phyGsm->interferencePower_mW = 0.0;
        }

        if (phyGsm->mode == PHY_SENSING
            && PhyGsmCarrierSensing(phyGsm) == FALSE) {
            PhyGsmChangeState(node, phyIndex, PHY_IDLE);
        }
    }
}


float
PhyGsmGetRxLevel(
    Node* node,
    int phyIndex)
{
    PhyData*    thisPhy = node->phyData[phyIndex];
    PhyDataGsm* phyGsm  = (PhyDataGsm*)thisPhy->phyVar;

    phyGsm = (PhyDataGsm*)node->phyData[phyIndex]->phyVar;

    return phyGsm->rxLev;
}


double
PhyGsmGetRxQuality(
    Node* node,
    int phyIndex)
{
    PhyData*    thisPhy = node->phyData[phyIndex];
    PhyDataGsm* phyGsm  = (PhyDataGsm*)thisPhy->phyVar;

    return phyGsm->rxBer;
}


void PhyGsmSetTransmitPower(
    PhyData* thisPhy,
    double newTxPower_mW)
{
    PhyDataGsm* phyGsm  = (PhyDataGsm*) thisPhy->phyVar;

    phyGsm->txPower_dBm = (float) IN_DB (newTxPower_mW);
}


void PhyGsmGetTransmitPower(
    PhyData* thisPhy,
    double* txPower_mW)
{
    PhyDataGsm* phyGsm  = (PhyDataGsm*) thisPhy->phyVar;

    *txPower_mW = NON_DB(phyGsm->txPower_dBm);
}


clocktype
PhyGsmGetTransmissionDuration(
    int size,
    int dataRate)
{
    clocktype   delay   = size * 8 * SECOND / dataRate;

    return delay;
}




//
// Used by the MAC layer to start transmitting a packet.
//
void PhyGsmStartTransmittingSignal(
    Node* node,
    int         phyIndex,
    Message* packet,
    BOOL        useMacLayerSpecifiedDelay,
    clocktype   initDelayUntilAirborne)
{
    BOOL        phyIsListening;
    clocktype   delayUntilAirborne  = initDelayUntilAirborne;
    PhyData*    thisPhy             = node->phyData[phyIndex];
    PhyDataGsm* phyGsm              = (PhyDataGsm*)thisPhy->phyVar;

    int         channelIndex;
    Message*    endMsg;
    int         packetsize          = MESSAGE_ReturnPacketSize(packet);
    clocktype   duration;

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

    if (!useMacLayerSpecifiedDelay) {
        delayUntilAirborne = PHY_GSM_PHY_DELAY;
    }

    if (phyGsm->mode == PHY_RECEIVING) {
        if (thisPhy->antennaData->antennaModelType
            == ANTENNA_OMNIDIRECTIONAL) {
            phyGsm->interferencePower_mW += phyGsm->rxMsgPower_mW;
        }
        else{
            ANTENNA_SetToDefaultMode(node, phyIndex);
            PHY_SignalInterference(node,
                phyIndex,
                channelIndex,
                NULL,
                NULL,
                &(phyGsm->interferencePower_mW));
        }
        PhyGsmUnlockSignal(phyGsm);
    }

    PhyGsmChangeState(node,phyIndex,PHY_TRANSMITTING);

    duration = PhyGsmGetTransmissionDuration(packetsize,
                   PHY_GetTxDataRate(node,
                       phyIndex));

    phyIsListening = PHY_IsListeningToChannel(node, phyIndex, channelIndex);

    if (phyIsListening) {
        PHY_StopListeningToChannel(node, phyIndex, channelIndex);
    }

    PROP_ReleaseSignal(node,
        packet,
        phyIndex,
        channelIndex,
        phyGsm->txPower_dBm,
        duration,
        delayUntilAirborne);
    //GuiStart
    if (node->guiOption == TRUE) {
        GUI_Broadcast(node->nodeId,
            GUI_PHY_LAYER,
            GUI_DEFAULT_DATA_TYPE,
            thisPhy->macInterfaceIndex,
            node->getNodeTime());
    }
    //GuiEnd

    endMsg = MESSAGE_Alloc(node, PHY_LAYER, 0, MSG_PHY_TransmissionEnd);

    MESSAGE_SetInstanceId(endMsg, (short) phyIndex);

    MESSAGE_Send(node, endMsg, delayUntilAirborne + duration + 1);

    /* Keep track of phy statistics and battery computations */
    phyGsm->stats.totalTxSignals++;
    phyGsm->stats.energyConsumed += duration * (
        BATTERY_TX_POWER_COEFFICIENT * NON_DB(
        phyGsm->txPower_dBm) + BATTERY_TX_POWER_OFFSET - BATTERY_RX_POWER);
}


int
PhyGsmGetDataRate(
    PhyData* thisPhy)
{
    PhyDataGsm* phyGsm  = (PhyDataGsm*)thisPhy->phyVar;

    return phyGsm->dataRate;
}

void PhyGsmInterferenceArrivalFromChannel(
    Node* node,
    int phyIndex,
    uddtChannelIndex channelIndex,
    PropRxInfo* propRxInfo,
    double sigPower_mW)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataGsm* phyGsm = (PhyDataGsm*)thisPhy->phyVar;

    double  rxInterferencePower_mW = NON_DB(
                ANTENNA_GainForThisSignal( node,
                    phyIndex,
                    propRxInfo) + IN_DB(sigPower_mW));

    switch (phyGsm->mode) {
        case PHY_RECEIVING: {
            if (phyGsm->rxMsgError == FALSE) {
                phyGsm->rxMsgError = PhyGsmCheckRxPacketError( node, phyGsm);
            }

            phyGsm->rxTimeEvaluated = node->getNodeTime();
            phyGsm->interferencePower_mW += rxInterferencePower_mW;

            break;
        }

        case PHY_IDLE:
        case PHY_SENSING: {

            // check if the interference signal change the status
            phyGsm->interferencePower_mW += rxInterferencePower_mW;

            if (phyGsm->mode == PHY_IDLE
                && PhyGsmCarrierSensing(phyGsm) == TRUE) {

                PhyGsmChangeState(node, phyIndex, PHY_SENSING);

            }

            break;
        }

        default: {

            break;
        }
    }//switch (phyGsm->mode)//
}



void PhyGsmInterferenceEndFromChannel(
    Node* node,
    int phyIndex,
    uddtChannelIndex channelIndex,
    PropRxInfo* propRxInfo,
    double sigPower_mw)
{
    PhyData* thisPhy = node->phyData[phyIndex];
    PhyDataGsm* phyGsm = (PhyDataGsm*)thisPhy->phyVar;

    if (phyGsm->mode == PHY_RECEIVING)
    {
        if (phyGsm->rxMsgError == FALSE) {

            phyGsm->rxMsgError = PhyGsmCheckRxPacketError(node,phyGsm);
        }

        phyGsm->rxTimeEvaluated = node->getNodeTime();
    }

    double  interferencePower_mW = NON_DB(ANTENNA_GainForThisSignal(node,
                                   phyIndex,
                                   propRxInfo) +
                                   IN_DB(sigPower_mw));

    phyGsm->interferencePower_mW -= interferencePower_mW;

    if (phyGsm->interferencePower_mW < 0.0 ){
        phyGsm->interferencePower_mW = 0.0;
    }

    if (phyGsm->mode == PHY_SENSING
        && PhyGsmCarrierSensing(phyGsm) == FALSE) {

        PhyGsmChangeState(node, phyIndex, PHY_IDLE);

    }
}

