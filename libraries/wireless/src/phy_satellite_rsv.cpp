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

// /**
// PROTOCOL :: PHY-SATELLITE-RSV
// LAYER :: PHY
// REFERENCES :: DVS-S
// COMMENTS ::
// **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "api.h"
#include "antenna.h"

// #define DEBUG 1
#define DEBUG 0

#include "phy_satellite_rsv.h"
#include "mac_satellite_bentpipe.h"


// /**
// FUNCTION :: PhySatelliteRsvGetData
// LAYER :: PHY
// PURPOSE :: This routine returns the data structure associated with
//            the PHY layer for the provided <node, phyNum> pair.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyNum : int : The integer index of the interface associated with
//          this physical layer process.
// RETURN :: PhyData* : A pointer to the physical layer data for this
//           <node,phyNum> pair.
// **/

static
PhyData* PhySatelliteRsvGetData(Node *node,
                                int phyNum)
{
    return (PhyData*)(node->phyData[phyNum]);
}

// /**
// FUNCTION :: PhySatelliteRsvGetState
// LAYER :: PHY
// PURPOSE :: This routine returns the data structure associated with
//            the satellite RSV PHY process for the provided <node, phyNum>
//            pair.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyNum : int : The integer index of the interface associated with
//          this physical layer process.
// RETURN :: PhySatelliteRsvState* : A pointer to the specific data structure
//           for the satellite physical layer for this <node,phyNum> pair.
// **/

static
PhySatelliteRsvState* PhySatelliteRsvGetState(Node *node,
                                              int phyNum)
{
    PhyData* phyData = PhySatelliteRsvGetData(node,
                                              phyNum);
    return (PhySatelliteRsvState*)phyData->phyVar;
}

// /**
// FUNCTION :: PhySatellite// LAYER :: PHY
// PURPOSE :: This routine sets the data structure to be associated with
//            the satellite RSV PHY process for the provided <node, phyNum>
//            pair.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyNum : int : The integer index of the interface associated with
//          this physical layer process.
// +        state : PhySatelliteRsvState* : A pointer to the data structure
//          to be associated with this physical layer state machine.
// RETURN :: void : This routine does not return a value.
// **/

static
void PhySatelliteRsvSetState(Node *node,
                             int phyNum,
                             PhySatelliteRsvState* state)
{
    PhyData* phyData = PhySatelliteRsvGetData(node,
                                              phyNum);

    phyData->phyVar = (void*)state;
}

// /**
// FUNCTION :: PhySatelliteRsvReportExtendedStatusToMac
// LAYER :: PHY
// PURPOSE :: This routine reports additional status to the MAC
//            process including a copy of the packet from the
//            MAC layer above if available.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface associated
//          with this physical layer process.
// +        status : PhyStatusType : The new status of the physical
//          interface.
// +        receiveDuration : clocktype : The length of time that the
//          frame is expected to continue to be received until the end of
//          frame signal is event is processed.
// +        potentialIncomingPacket : Message* : A pointer to a message
//          data structure that represents a packet that has just begun
//          transmission on the uplink channel.
// RETURN :: void : This routine does not return a value.
// **/

static
void PhySatelliteRsvReportExtendedStatusToMac(
                                            Node* node,
                                            int  phyIndex,
                                            int channelIndex,
                                            PhyStatusType status,
                                            clocktype receiveDuration,
                                            Message* potentialIncomingPacket)
{
    // Clearly this function provides a lookahead into the incoming
    // packet before an equivalent physical system would have such
    // information.  As such, it is to be used with some caution when
    // developing algorithms and other numerical calculations that are
    // intended to be directly implemented in a physically-realizable
    // system.

    PhyData* phyData = PhySatelliteRsvGetData(node,
                                              phyIndex);

    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);

    ERROR_Assert(status ==
                 PhySatelliteRsvGetStatus(node, phyIndex, channelIndex),
                 "PHY status should not have changed.");

    // When delivering the packet for review by the MAC layer, the
    // existing PHY header must be stripped.  Since this packet is
    // shared among multiple processes, the header must be replaced
    // after the review is done.  Note that it is not necessary to
    // recopy the header information or save it locally in the process.
    // Rather, the header stays in the packet and the indices that
    // control the header queue merely are manipulated for improved
    // computational efficiency.

    if (potentialIncomingPacket != 0) {
        MESSAGE_RemoveHeader(node,
                             potentialIncomingPacket,
                             sizeof(PhySatelliteRsvHeader),
                             TRACE_SATELLITE_RSV);
    }

    if (potentialIncomingPacket != 0 && DEBUG) {
        printf("PhySatelliteRsvReportExtendedStatusToMac()\n");
    }

    PhySatelliteRsvChannel* channelInfo =
        PhySatelliteRsvGetLinkChannelInfo(
            node,
            phyIndex,
            channelIndex);

    MacSatelliteBentpipeReceivePhyStatusChangeNotification(
        node,
        channelIndex,
        phyData->macInterfaceIndex,
        channelInfo->previousMode,
        status);

    if (potentialIncomingPacket != 0) {
        MESSAGE_AddHeader(node,
                          potentialIncomingPacket,
                          sizeof(PhySatelliteRsvHeader),
                          TRACE_SATELLITE_RSV);
    }
}

// /**
// FUNCTION :: PhySatelliteRsvReportStatusToMac
// LAYER :: PHY
// PURPOSE :: This routine reports general status to the MAC
//            process, notably the change in the PhyStatus.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface associated
//          with this physical layer process.
// +        status : PhyStatusType : The new status of the physical
//          interface.
// RETURN :: void : This routine does not return a value.
// **/

static
void PhySatelliteRsvReportStatusToMac(Node* node,
                                      int phyIndex,
                                      int channelIndex,
                                      PhyStatusType status)
{
    //
    // The standard status is just a change in the mode.
    //

    PhySatelliteRsvReportExtendedStatusToMac(node,
                                             phyIndex,
                                             channelIndex,
                                             status,
                                             0,
                                             0);
}

// /**
// FUNCTION :: PhySatelliteRsvSetMode
// LAYER :: PHY
// PURPOSE :: This routine sets the status mode of the local physical
//            layer process.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface associated
//          with this physical layer process.
// +        newmode : PhyStatusType : The new status of the physical
//          interface.
// RETURN :: void : This routine does not return a value.
// **/

static
void PhySatelliteRsvSetMode(Node* node,
                            int phyIndex,
                            int channelIndex,
                            PhyStatusType newmode)
{

    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                 phyIndex,
                                 channelIndex);

    channel->previousMode = channel->currentMode;
    channel->currentMode = newmode;

    //Informing the energy model about the change in PhyStatus
    Phy_ReportStatusToEnergyModel(
        node,
        phyIndex,
        channel->previousMode,
        newmode);

    // Currently the mode of the physical layer is not noted as
    // changed unless the currentMode and previousMode do not match.
    // This reduces the number of times the MAC is apprised of status
    // changes but may reduce the MAC's real-time understanding of changes
    // in signal level, interference and noise.

    if (channel->currentMode != channel->previousMode) {
        PhySatelliteRsvReportStatusToMac(node,
                                         phyIndex,
                                         channelIndex,
                                         channel->currentMode);
    }

}

// /**
// FUNCTION :: PhySatelliteRsvGetMode
// LAYER :: PHY
// PURPOSE :: This routine returns the current status mode of the local
//            physical layer process.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface associated
//          with this physical layer process.
// RETURN :: PhyStatusType : The current mode of the physical layer process.
// **/

static
PhyStatusType PhySatelliteRsvGetMode(Node* node,
                                     int phyIndex,
                                     int channelIndex)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);

    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                 phyIndex,
                                 channelIndex);

    return channel->currentMode;

}

// /**
// FUNCTION :: PhySatelliteRsvInitializeStaticParameters
// LAYER :: PHY
// PURPOSE :: This routine initializes the static parameters of the
//            physical layer process.  This includes various profiles
//            that are copied into the layer process data structure from
//            constants and defaults specified in the programming header.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface associated
//          with this physical layer process.
// RETURN :: void : This routine does not return a value.
// **/

static
void PhySatelliteRsvInitializeStaticParameters(Node* node,
                                               int phyIndex)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);

    PhySatelliteRsvStateProfiles[0] = &PhySatelliteRsvDefaultBpskProfile;
    PhySatelliteRsvStateProfiles[1] = &PhySatelliteRsvDefaultQpskProfile;
    PhySatelliteRsvStateProfiles[2] = &PhySatelliteRsvDefault8pskProfile;

    state->dataRateCount = PhySatelliteRsvDefaultModulationProfileCount;
}

// /**
// CONSTANT :: BITS_PER_BYTE : 8
// DESCRIPTION :: The number of bits in an byte of data
// **/

#define BITS_PER_BYTE (8)
// /**
// FUNCTION :: PhySatelliteRsvInitializeConfigurableParameters
// LAYER :: PHY
// PURPOSE :: This routine initializes the configurable parameters of the
//            physical layer process.  This includes data obtained from
//            queries to the simulation configuration database.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface associated
//          with this physical layer process.
// +        nodeInput : NodeInput* : A pointer to the simulation
//          configuration data store that has been deserialized from the
//          simulation configuration file and any auxillary configuration
//          files specified within that file.
// RETURN :: void : This routine does not return a value.
// **/

static
void PhySatelliteRsvInitializeUserDefinedParameters(Node* node,
         int phyIndex,
         const NodeInput* nodeInput,
         PhySatelliteRsvModulationProfile *PhySatelliteRsvUserDefinedProfile)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);
    Address* networkAddr = node->phyData[phyIndex]->networkAddress;

    BOOL wasFound;
    char errStr[MAX_STRING_LENGTH];

    //IO_ReadDouble(node->nodeId,
    //              networkAddr,
    //              nodeInput,
    //              "PHY-SATELLITE-RSV-DATA-RATE",
    //              &wasFound,
    //              &PhySatelliteRsvUserDefinedProfile->dataRate);
    //if (wasFound == FALSE) {
    //    sprintf(errStr,"Error:Node %d : Input Required for DataRate\n",
    //                    node->nodeId);
    //    ERROR_Assert(FALSE , errStr);
    //}

    IO_ReadDouble(node->nodeId,
                  networkAddr,
                  nodeInput,
                  "PHY-SATELLITE-RSV-TRANSMIT-POWER",
                  &wasFound,
                  &PhySatelliteRsvUserDefinedProfile->transmitPowermW);
    if (wasFound == FALSE) {
        sprintf(errStr,"Error:Node %d : Input Required for TransmitPower\n",
            node->nodeId);
        ERROR_Assert(FALSE , errStr);
    }

    IO_ReadDouble(node->nodeId,
                  networkAddr,
                  nodeInput,
                  "PHY-SATELLITE-RSV-SENSITIVITY",
                  &wasFound,
                  &PhySatelliteRsvUserDefinedProfile->sensitivitydBm);
    if (wasFound == FALSE) {
        sprintf(errStr,"Error:Node %d : Input Required for Sensitivity\n",
            node->nodeId);
        ERROR_Assert(FALSE , errStr);
    }

    IO_ReadDouble(node->nodeId,
                  networkAddr,
                  nodeInput,
                  "PHY-SATELLITE-RSV-BITS-PER-SYMBOL",
                  &wasFound,
                  &PhySatelliteRsvUserDefinedProfile->bitsPerSymbol);
    if (wasFound == FALSE) {
        sprintf(errStr,"Error:Node %d : Input Required for BitsPerSymbol\n",
            node->nodeId);
        ERROR_Assert(FALSE , errStr);
    }
}

// /**
// FUNCTION :: PhySatelliteRsvSetTxDataRateTypeUserDefined
// LAYER :: PHY
// PURPOSE :: This function implements model logic to set the current
//            data rate for transmission to a value consistent with
//            the provided data rate type.  This function is called
//            by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// +        dateRateType : int : An enumerated set of available data
//          rate types.  Note that this must be a valid data rate
//          type of the program will exhibit unexpected behavior.
// RETURN :: void : This function does not return any value.
// **/

void PhySatelliteRsvSetTxDataRateTypeUserDefined(Node *node, int phyIndex,
          int dataRateType,
          PhySatelliteRsvModulationProfile PhySatelliteRsvUserDefinedProfile)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);
    ERROR_Assert(dataRateType < state->dataRateCount,
                 "The data rate requested by MAC is not supported "
                 "by this physical layer model.");

    state->transmitDataRateType = (PhySatelliteRsvDataRateType)dataRateType;
    state->receiveDataRateType = (PhySatelliteRsvDataRateType)dataRateType;

    //state->transmitDataRate = state->[dataRateType].dataRate;
    state->transmitDataRate
        = PhySatelliteRsvUserDefinedProfile.bitsPerSymbol
        * state->channelBandwidth;
    state->transmitPowermW
        = PhySatelliteRsvUserDefinedProfile.transmitPowermW;

    state->receiveDataRate = state->transmitDataRate;
    state->receiveSensitivitydBm
        = PhySatelliteRsvUserDefinedProfile.sensitivitydBm;
}

// /**
// FUNCTION :: PhySatelliteRsvInitializeConfigurableParameters
// LAYER :: PHY
// PURPOSE :: This routine initializes the configurable parameters of the
//            physical layer process.  This includes data obtained from
//            queries to the simulation configuration database.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface associated
//          with this physical layer process.
// +        nodeInput : NodeInput* : A pointer to the simulation
//          configuration data store that has been deserialized from the
//          simulation configuration file and any auxillary configuration
//          files specified within that file.
// RETURN :: void : This routine does not return a value.
// **/

static
void PhySatelliteRsvInitializeConfigurableParameters(Node* node,
                                                 int phyIndex,
                                                 const NodeInput* nodeInput)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);
    Address* networkAddr = node->phyData[phyIndex]->networkAddress;

    BOOL wasFound;
    char buf[BUFSIZ];

    IO_ReadDouble(node->nodeId,
                  networkAddr,
                  nodeInput,
                  "PHY-SATELLITE-RSV-CHANNEL-BANDWIDTH",
                  &wasFound,
                  &state->channelBandwidth);
    if (wasFound == FALSE) {
        state->channelBandwidth = PhySatelliteRsvDefaultChannelBandwidth;
    }

    IO_ReadInt(node->nodeId,
               networkAddr,
               nodeInput,
               "PHY-SATELLITE-RSV-PREAMBLE-SIZE",
               &wasFound,
               &state->preambleSize);
    if (wasFound == FALSE) {
        state->preambleSize = PhySatelliteRsvDefaultPreambleSize;
    }
    else {
        if (state->preambleSize % BITS_PER_BYTE != 0) {
            char buf[BUFSIZ];

            sprintf(buf,
                    "Preamble size must in multiples of %d bits.",
                    BITS_PER_BYTE);
            ERROR_ReportError(buf);
        }
    }

    IO_ReadString(node->nodeId,
                  networkAddr,
                  nodeInput,
                  "PHY-SATELLITE-RSV-USE-SHORTEN-LAST-CODEWORD",
                  &wasFound,
                  buf);
    if (wasFound == FALSE) {
        state->useShortenLastCodeword =
            PhySatelliteRsvDefaultShortenLastCodeword;
    }
    else if (strcmp(buf, "YES") == 0 || strcmp(buf, "TRUE") == 0) {
        state->useShortenLastCodeword = TRUE;
    }
    else if (strcmp(buf, "NO") == 0 || strcmp(buf, "FALSE") == 0) {
        state->useShortenLastCodeword = FALSE;
    }

    IO_ReadString(node->nodeId,
                  networkAddr,
                  nodeInput,
                  "PHY-SATELLITE-RSV-BLOCK-CODING",
                  &wasFound,
                  buf);
    if (wasFound == FALSE) {
        state->bcType = PhySatelliteRsvDefaultBlockCoding;
    }
    else if (strcmp(buf, "REED-SOLOMON-204-188") == 0) {
        state->bcType = PhySatelliteRsvBlockCoding188_204;
        state->codingRate = SATELLITE_CODE_RATE_188_204;
    }
    else {
        ERROR_ReportError("Unsupported block code type.");
    }

    IO_ReadString(node->nodeId,
                  networkAddr,
                  nodeInput,
                  "PHY-SATELLITE-RSV-TRANSMISSION-MODE",
                  &wasFound,
                  buf);
    if (wasFound == FALSE) {
        state->transmissionMode = PhySatelliteRsvDefaultTransmissionMode;
        PHY_SetTxDataRateType(node, phyIndex, PhySatelliteRsvDataRateZero);
    }
    else if (strcmp(buf, "BPSK") == 0) {
        state->transmissionMode = PhySatelliteRsvDataRateZero;
        state->modType = PhySatelliteRsvModulationBpsk;
        state->bitsPerSymbol = SATELLITE_BITS_PER_SYMBOL_BPSK;
        PHY_SetTxDataRateType(node,
                              phyIndex,
                              PhySatelliteRsvDataRateZero);
    }
    else if (strcmp(buf, "QPSK") == 0) {
        state->transmissionMode = PhySatelliteRsvDataRateOne;
        state->modType = PhySatelliteRsvModulationQpsk;
        state->bitsPerSymbol = SATELLITE_BITS_PER_SYMBOL_QPSK;
        PHY_SetTxDataRateType(node,
                              phyIndex,
                              PhySatelliteRsvDataRateOne);
    }
    else if (strcmp(buf, "8PSK") == 0) {
        state->transmissionMode = PhySatelliteRsvDataRateTwo;
        state->modType = PhySatelliteRsvModulation8psk;
        state->bitsPerSymbol = SATELLITE_BITS_PER_SYMBOL_8PSK;
        PHY_SetTxDataRateType(node,
                              phyIndex,
                              PhySatelliteRsvDataRateTwo);
    }
    else if (strcmp(buf, "USER-DEFINED") == 0) {
        state->transmissionMode = PhySatelliteRsvUserDefined;
        state->modType = PhySatelliteRsvModulationUserDefined;

        PhySatelliteRsvModulationProfile PhySatelliteRsvUserDefinedProfile;
        memset((void *)&PhySatelliteRsvUserDefinedProfile,
               0,
               sizeof(PhySatelliteRsvModulationProfile));
        PhySatelliteRsvInitializeUserDefinedParameters(node,
                                         phyIndex,
                                         nodeInput,
                                         &PhySatelliteRsvUserDefinedProfile);

        state->bitsPerSymbol =
            PhySatelliteRsvUserDefinedProfile.bitsPerSymbol;
        PhySatelliteRsvSetTxDataRateTypeUserDefined(node,
                              phyIndex,
                              PhySatelliteRsvUserDefined,
                              PhySatelliteRsvUserDefinedProfile);
    }
    else {
        ERROR_ReportError("Unsupported transmission rate.");
    }

    IO_ReadString(node->nodeId,
                  networkAddr,
                  nodeInput,
                  "PHY-SATELLITE-RSV-CONVOLUTIONAL-CODING",
                  &wasFound,
                  buf);
    if (wasFound == FALSE) {
        state->ccType = PhySatelliteRsvDefaultConvolutionalCoding;
        state->codingRate *= SATELLITE_CODE_RATE_VITERBI_1_2;
    }
    else if (strcmp(buf, "VITERBI-1-2") == 0) {
        state->ccType = PhySatelliteRsvConvolutionalCoding1_2;
        state->codingRate *= SATELLITE_CODE_RATE_VITERBI_1_2;
        if (state->transmissionMode == PhySatelliteRsvDataRateTwo) {
            ERROR_ReportError("Unsupported coding/modulation profile.");
        }
    }
    else if (strcmp(buf, "VITERBI-2-3") == 0) {
        state->ccType = PhySatelliteRsvConvolutionalCoding2_3;
        state->codingRate *= SATELLITE_CODE_RATE_VITERBI_2_3;

        if (state->transmissionMode != PhySatelliteRsvDataRateTwo) {
            ERROR_ReportError("Unsupported coding/modulation profile.");
        }
    }
    else if (strcmp(buf, "VITERBI-3-4") == 0) {
        state->ccType = PhySatelliteRsvConvolutionalCoding3_4;
        state->codingRate *= SATELLITE_CODE_RATE_VITERBI_3_4;

        if (state->transmissionMode == PhySatelliteRsvDataRateTwo) {
            ERROR_ReportError("Unsupported coding/modulation profile.");
        }
    }
    else if (strcmp(buf, "VITERBI-5-6") == 0) {
        state->ccType = PhySatelliteRsvConvolutionalCoding5_6;
        state->codingRate *= SATELLITE_CODE_RATE_VITERBI_5_6;

        if (state->transmissionMode != PhySatelliteRsvDataRateTwo) {
            ERROR_ReportError("Unsupported coding/modulation profile.");
        }
    }
    else if (strcmp(buf, "VITERBI-7-8") == 0) {
        state->ccType = PhySatelliteRsvConvolutionalCoding7_8;
        state->codingRate *= SATELLITE_CODE_RATE_VITERBI_7_8;

        if (state->transmissionMode == PhySatelliteRsvDataRateTwo) {
            ERROR_ReportError("Unsupported coding/modulation profile.");
        }
    }
    else if (strcmp(buf, "VITERBI-8-9") == 0) {
        state->ccType = PhySatelliteRsvConvolutionalCoding8_9;
        state->codingRate *= SATELLITE_CODE_RATE_VITERBI_8_9;

        if (state->transmissionMode != PhySatelliteRsvDataRateTwo) {
            ERROR_ReportError("Unsupported coding/modulation profile.");
        }
    }
    else {
        ERROR_ReportError("Unsupported block code type.");
    }

    IO_ReadTime(node->nodeId,
                networkAddr,
                nodeInput,
                "PHY-SATELLITE-RSV-GUARD-TIME",
                &wasFound,
                &state->guardTime);
    if (wasFound == FALSE) {
        state->guardTime = PhySatelliteRsvDefaultGuardTime;
    }

    IO_ReadDouble(node->nodeId,
                  networkAddr,
                  nodeInput,
                  "PHY-SATELLITE-RSV-ADJACENT-CHANNEL-INTERFERENCE",
                  &wasFound,
                  &state->aci);
    if (wasFound == FALSE) {
        state->aci = PhySatelliteRsvDefaultAci;
    }

    IO_ReadDouble(node->nodeId,
                  networkAddr,
                  nodeInput,
                  "PHY-SATELLITE-RSV-ADJACENT-SATELLITE-INTERFERENCE",
                  &wasFound,
                  &state->asi);
    if (wasFound == FALSE) {
        state->asi = PhySatelliteRsvDefaultAsi;
    }

    IO_ReadDouble(node->nodeId,
                  networkAddr,
                  nodeInput,
                  "PHY-SATELLITE-RSV-INTERMODULATION-INTERFERENCE",
                  &wasFound,
                  &state->im3);
    if (wasFound == FALSE) {
        state->im3 = PhySatelliteRsvDefaultIm3;
    }

    IO_ReadDouble(node->nodeId,
                  networkAddr,
            nodeInput,
            "PHY-SATELLITE-RSV-EBNO-THRESHOLD",
            &wasFound,
            &state->rxThreshold);
    if (wasFound == FALSE) {
        state->rxThreshold = PhySatelliteRsvDefaultEbnoThreshold;
    }
}

// /**
// FUNCTION :: PhySatelliteRsvResetStatistics
// LAYER :: PHY
// PURPOSE :: This routine resets the run-time statistic counters to
//            a known (zero) state.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface associated
//          with this physical layer process.
// RETURN :: void : This routine does not return a value.
// **/

static
void PhySatelliteRsvResetStatistics(
                    PhySatelliteRsvTxRxChannelInfo* rxTxChannelInfoPtr)
{

    PhySatelliteRsvStatistics *txStats =
                                &rxTxChannelInfoPtr->txChannelInfo->stats;

    txStats->totalTxSignals = 0;
    txStats->totalRxSignalsToMac = 0;
    txStats->totalSignalsLocked = 0;
    txStats->totalSignalsWithErrors = 0;

    PhySatelliteRsvStatistics *rxStats =
                                &rxTxChannelInfoPtr->rxChannelInfo->stats;
    rxStats->totalTxSignals = 0;
    rxStats->totalRxSignalsToMac = 0;
    rxStats->totalSignalsLocked = 0;
    rxStats->totalSignalsWithErrors = 0;
}

// /**
// FUNCTION :: PhySatelliteRsvInitializeState
// LAYER :: PHY
// PURPOSE :: This routine resets the physical layer state machine to
//            a known (IDLE) state.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface associated
//          with this physical layer process.
// RETURN :: void : This routine does not return a value.
// **/

static
void PhySatelliteRsvInitializeState(Node* node,
                                    int phyIndex)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);

    state->channelBandwidth = PhySatelliteRsvDefaultChannelBandwidth;

    state->averageEbnoStat = 0.0;
    state->averageEbnoCount = 0;
}

// /**
// FUNCTION :: PhySatelliteRsvLockSignal
// LAYER :: PHY
// PURPOSE :: This routine simulates the acquisition of a target signal that
//            was suitably strong to be detected by the local receiver.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface associated
//          with this physical layer process.
// +        msg : Message* : A pointer to a data structure representing a
//          frame undergoing reception.
// +        rxPowermW : double : The current reception power of the frame
// +        rxEndTime : clocktype : The expected time at which the frame
//          transmission will be complete.
// RETURN :: void : This routine does not return a value.
// **/

static
void PhySatelliteRsvLockSignal(Node* node,
                               int phyIndex,
                               int channelIndex,
                               Message* msg,
                               double rxPowermW,
                               clocktype rxEndTime)
{
    // This routine provides information to the Physical layer process
    // that would not normally be available in a physically realizable
    // system.  Care should be taken to restrict the use of this routine
    // for simulation enhancements and abstractions and avoid the use
    // of algorithms that are destined to be directly implemented in a
    // physical communications product.

    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);

    if (DEBUG) {
        printf(
            "PhySatelliteRsvLockSignal[%d%d] locks message %p (%lf, %lf)\n",
            node->nodeId,
            phyIndex,
            msg,
            rxPowermW,
            (double)rxEndTime / (double)SECOND);
    }

    // When the signal is locked, the frame undergoing reception is
    // stored in the local state variable storage area.  A flag is
    // set to indicate that the frame is being received successfully
    // and the current power and time is noted.

    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                          phyIndex,
                                          channelIndex);

    channel->rxMsg = msg;
    channel->rxMsgError = FALSE;
    channel->rxMsgPowermW = rxPowermW;
    channel->rxTimeEvaluated = node->getNodeTime();

    channel->stats.totalSignalsLocked++;

}

// /**
// FUNCTION :: PhySatelliteRsvUnlockSignal
// LAYER :: PHY
// PURPOSE :: This routine simulates the deacquisition of a target signal
//            that was suitably strong to be detected by the local receiver.
//            This could occur to an abnormal termination of the signal,
//            interference from other transmitters or sources or normal
//            termination of the signal due to completed transmission.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface associated
//          with this physical layer process.
// RETURN :: void : This routine does not return a value.
// **/

static
void PhySatelliteRsvUnlockSignal(Node* node,
                                 int phyIndex)
{
    if (DEBUG) {
        printf("PhySatelliteRsvUnlockSignal[%d%d] unlocks packet reception\n",
               node->nodeId,
               phyIndex);
    }
}

// /**
// FUNCTION :: PhySatelliteRsvIsSensingCarrier
// LAYER :: PHY
// PURPOSE :: This routine returns whether or not the current signal
//            at the detector is sufficiently high to indicate that a
//            valid signal may be in the process of being received.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface associated
//          with this physical layer process.
// RETURN :: void : This routine does not return a value.
// **/

static
BOOL PhySatelliteRsvIsSensingCarrier(Node* node,
                                     int phyIndex,
                                     int channelIndex)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);
    double rxSensitivitymW =
        NON_DB(PhySatelliteRsvStateProfiles[0]->sensitivitydBm);

    if (ANTENNA_IsInOmnidirectionalMode(node, phyIndex) == 0) {
        rxSensitivitymW =
            NON_DB(PhySatelliteRsvStateProfiles[0]->sensitivitydBm +
                PhySatelliteRsvStateDirectionalAntennaGaindB);
    }

    // Currently if the interference and noise power is greater than
    // the sensitivity of the local receiver, the system is considered to
    // be sensing a carrier even though that carrier does not exist.
    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                          phyIndex,
                                          channelIndex);

    if (channel->interferencePowermW + channel->noisePowermW >
                                                    rxSensitivitymW) {
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION :: PhySatelliteRsvGetFrameSizeInBits
// LAYER :: PHY
// PURPOSE :: This routine returns the total number of bits required for
//            a physical layer transmission not including guard time and
//            preamble.  Essentially it converts the user data to coded data
//            length (also in bits).
// PARAMETERS ::
// +        bytes : int : The number of user data bytes to encode
// +        mt : PhySatelliteRsvModulationType : The current modulation profile
//          of the transmitter
// +        bc : PhySatelliteRsvBlockCodingType : The current block (inner)
//          code type of the transmitter
// +        cc : PhySatelliteRsvConvolutionalCodingType : The current
//          convolutional (outer) code type of the transmitter
// +        useShortenLastCodeword : BOOL : Whether or not to use RS
//          code shortening techniques
// +        virtualOverheadBytesPtr : int* : The number of bytes of overhead
//          to add to an existing packet to account for coding overhead
// RETURN :: int : The coded frame size of the packet
// **/

static
int PhySatelliteRsvGetFrameSizeInBits(Node* node,
                                  int phyNum,
                                  int bytes,
                                  PhySatelliteRsvModulationType mt,
                                  PhySatelliteRsvBlockCodingType bc,
                                  PhySatelliteRsvConvolutionalCodingType cc,
                                  BOOL useShortenLastCodeword,
                                  int* virtualOverheadBytesPtr)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyNum);
    int bn = -1;
    int bk = -1;
    int cn = -1;
    int ck = -1;

    int blocks = 0;
    int remainder = 0;
    int lastBlock = 0;

    int rsCodedBits = 0;
    int ccCodedBits = 0;

    int adder = 0;

    // In a concatenated code, the code is serially processed.  The
    // first code encountered by the user frame is the "outer" code and
    // is the block code.  The second code is termed the "inner" code since
    // it is last encoded and first decoded.

    switch(bc) {
        case PhySatelliteRsvBlockCoding188_204:
            bn = 204;
            bk = 188;
            break;
        default:
            ERROR_ReportError("Unsupported block coding rate.");
    }

    switch(cc) {
        case PhySatelliteRsvConvolutionalCoding1_2:
            cn = 2;
            ck = 1;
            if (mt == PhySatelliteRsvModulation8psk) {
                ERROR_ReportError("Unsupported modulation/coding profile.");
            }
                break;
        case PhySatelliteRsvConvolutionalCoding2_3:
            cn = 3;
            ck = 2;
            if (mt != PhySatelliteRsvModulation8psk
                && mt != PhySatelliteRsvModulationBpsk)
            {
                ERROR_ReportError("Unsupported modulation/coding profile.");
            }
                break;
        case PhySatelliteRsvConvolutionalCoding3_4:
            cn = 4;
            ck = 3;
            if (mt == PhySatelliteRsvModulation8psk) {
                ERROR_ReportError("Unsupported modulation/coding profile.");
            }
                break;
        case PhySatelliteRsvConvolutionalCoding5_6:
            cn = 6;
            ck = 5;
            if (mt != PhySatelliteRsvModulation8psk && mt !=
                PhySatelliteRsvModulationBpsk) {
                ERROR_ReportError("Unsupported modulation/coding profile.");
            }
                break;
        case PhySatelliteRsvConvolutionalCoding7_8:
            cn = 8;
            ck = 7;
            if (mt == PhySatelliteRsvModulation8psk) {
                ERROR_ReportError("Unsupported modulation/coding profile.");
            }
                break;
        case PhySatelliteRsvConvolutionalCoding8_9:
            cn = 9;
            ck = 8;
            if (mt != PhySatelliteRsvModulation8psk && mt !=
                PhySatelliteRsvModulationBpsk) {
                ERROR_ReportError("Unsupported modulation/coding profile.");
            }
                break;
        default:
            ERROR_ReportError("Unsupported coding profile.");
    }

    blocks = bytes / bk;
    remainder = blocks % bk;
    lastBlock = bn;

    if (useShortenLastCodeword == TRUE) {
        lastBlock = remainder + bn - bk;
    }

    rsCodedBits = BITS_PER_BYTE*(blocks * bn + lastBlock);
    ccCodedBits = rsCodedBits * cn / ck;

    switch(mt) {
        case PhySatelliteRsvModulationBpsk:
            break;
        case PhySatelliteRsvModulationQpsk:
            if (ccCodedBits % 2 != 0) {
                ccCodedBits++;
            }
            break;
        case PhySatelliteRsvModulation8psk:
            if (ccCodedBits % 3 != 0) {
                adder = 3 - (ccCodedBits % 3);

                ccCodedBits += adder;
            }
            break;
        case PhySatelliteRsvModulation16qam:
            if (ccCodedBits % 4 != 0) {
                adder = 4 - (ccCodedBits % 4);

                ccCodedBits += adder;
            }
            break;
        case PhySatelliteRsvModulationUserDefined:
        {
            if (ccCodedBits % (int)state->bitsPerSymbol != 0) {
                adder = (int)state->bitsPerSymbol -
                    (ccCodedBits % (int)state->bitsPerSymbol);

                ccCodedBits += adder;
            }
            break;
        }
        default:
            ERROR_ReportError("Unsupported modulation format.");
    }

    if (ccCodedBits % 8 != 0) {
        adder = 8 - (ccCodedBits % 8);
        ccCodedBits += adder;
    }

    if (virtualOverheadBytesPtr != 0) {
        *virtualOverheadBytesPtr = ccCodedBits/8 - bytes;
    }

    return ccCodedBits;
}

// /**
// FUNCTION :: PhySatelliteRsvGetFrameDuration
// LAYER :: PHY
// PURPOSE :: This routine implements a function to translate from
//            coded message size to overall frame duration.  This includes
//            the actual transmission rate, the preamble size and the amount
//            of guard time required on the channel.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyNum : int : The integer index of the interface associated with
//          this physical layer process
// +        packetsize : int : The size of the packet (in bytes) that is
//          associated with upper level data.
// RETURN :: clocktype : The integer number of nanoseconds required to
//           transmit the entire packet.
// **/

static
clocktype PhySatelliteRsvGetFrameDuration(Node* node,
                                          int phyNum,
                                          int packetsize)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyNum);
    clocktype duration = 0;

    int payloadbits
        = PhySatelliteRsvGetFrameSizeInBits(node,
                                            phyNum,
                                            packetsize,
                                            state->modType,
                                            state->bcType,
                                            state->ccType,
                                            state->useShortenLastCodeword,
                                            NULL);
    payloadbits += state->preambleSize * BITS_PER_BYTE;

    duration = SECOND * payloadbits / (clocktype)state->transmitDataRate;

    if (DEBUG) {
        printf("-->frame was %d bits (%0.3f microsecond).\n",
               payloadbits,
               (double)duration / (double)MICRO_SECOND);
    }

    return duration;
}

// /**
// FUNCTION :: PhySatelliteRsvAddSNR
// LAYER :: PHY
// PURPOSE :: This routine implements a function to add two sources of
//            All White Gaussian Noise (AWGN) cast as Signal to Noise
//            Ratios (SNR).
// PARAMETERS ::
// +        oldval : double : The original SNR value
// +        newval : double : The addend for the additional noise
//          source
// RETURN :: double : The aggregate SNR value for the two levels.  It must
//           be less than either values individually.
// **/

static
double PhySatelliteRsvAddSNR(double oldval, double newval)
{
    double ao = pow(10.0, oldval/10.0);
    double an = pow(10.0, newval/10.0);
    double ar = 1.0/ao + 1.0/an;

    return 10.0*log10(1.0/ar);
}

// /**
// FUNCTION :: PhySatelliteRsvCalculateOverallSNR
// LAYER :: PHY
// PURPOSE :: This routine implements a function to calculate the
//            overall SNR of a packet across an entire satellite
//            subnetwork.  This may either be the one-hop value or the
//            two-hop value.
// PARAMETERS ::
// +        hdr : PhySatelliteRsvHeader* : The packet header of the frame
//          that has been received.
// +        rxSnr : double : The SNR of the frame as it was recieved on this
//          wireless segment hop.
// RETURN :: double : The aggregate SNR value for the two levels.  It must
//           be less than either values individually.
// **/

static
double PhySatelliteRsvCalculateOverallSnr(PhySatelliteRsvHeader* hdr,
                                          double rxSnr)
{
    double aggsnr = rxSnr;

    if (DEBUG) {
        printf("Original SINR = %0.1lf\n",
               rxSnr);
    }

    if (hdr->im3Valid == TRUE) {
        aggsnr = PhySatelliteRsvAddSNR(aggsnr,
                                       (double)hdr->im3);
        if (DEBUG) {
            printf("im3 [VALID] value %0.1lf new sinr %0.1lf.\n",
                   (double)hdr->im3,
                   aggsnr);
        }
    }
    else if (DEBUG) {
        printf("im3 [INVALID] sinr remains %0.1lf.\n",
               aggsnr);
    }

    if (hdr->aciValid == TRUE) {
        aggsnr = PhySatelliteRsvAddSNR(aggsnr,
                                       (double)hdr->aci);
        if (DEBUG) {
            printf("aci [VALID] value %0.1lf new sinr %0.1lf\n",
                   (double)hdr->aci,
                   aggsnr);
        }
    }
    else if (DEBUG) {
        printf("aci [INVALID] sinr remains %0.1lf.\n",
               aggsnr);
    }

    if (hdr->asiValid == TRUE) {
        aggsnr = PhySatelliteRsvAddSNR(aggsnr,
                                       (double)hdr->asi);
        if (DEBUG) {
            printf("asi [VALID] value %0.1lf new sinr %0.1lf\n",
                   (double)hdr->asi,
                   aggsnr);
        }
    }
    else if (DEBUG) {
        printf("asi [INVALID] sinr remains %0.1lf.\n",
               aggsnr);
    }

    if (hdr->uplinkSnrValid == TRUE) {
        aggsnr = PhySatelliteRsvAddSNR(aggsnr,
                                       (double)hdr->uplinkSnr);
        if (DEBUG) {
            printf("uplinkSnr [VALID] value %0.1lf new sinr %0.1lf\n",
                   (double)hdr->uplinkSnr,
                   aggsnr);
        }
    }
    else if (DEBUG) {
        printf("uplinkSNR [INVALID] sinr remains %0.1lf.\n",
               aggsnr);
    }
    return aggsnr;
}

// /**
// FUNCTION :: PhySatelliteRsvMergeRatio
// LAYER :: PHY
// PURPOSE :: This routine implements a gated adder.  If the header value
//            is valid (as identified by the flag), then add the two
//            SNR values using the routines defined above.  If not, return
//            only the newest value of the SNR.
// PARAMETERS ::
// +        valid : bool : Whether or not the existing valud of SNR is
//          valid or not.
// +        oldval : double : The existing value of the SNR usually taken from
//          the received physical layer header
// +        newval : double : The current value of the SNR as measured by the
//          packet reception logic of the physical layer process.
// RETURN :: double : The aggregate SNR value for the two levels.  It must
//           be less than either values individually.
// **/

static
double PhySatelliteRsvMergeRatio(BOOL valid,
                                 double oldval,
                                 double newval)
{
    if (valid == TRUE) {
        return PhySatelliteRsvAddSNR(oldval, newval);
    }
    return newval;
}

// /**
// FUNCTION :: PhySatelliteRsvGetIm3
// LAYER :: PHY
// PURPOSE :: This routine returns the intermodulation distortion ratio of
//             the physical layer channel.
// PARAMETERS ::
// +        state : PhySatelliteRsvState* : A pointer to the physical layer
//          state machine state data for the current state machine
// RETURN :: double : The equivalent SNR for intermodulation noise as a
//           power ratio.
// **/

static
double PhySatelliteRsvGetIm3(PhySatelliteRsvState* state)
{
    return state->im3;
}

// /**
// FUNCTION :: PhySatelliteRsvUpdatePhyHeader
// LAYER :: PHY
// PURPOSE :: This routine updates the physical layer header upon reception.
//            This routine is normally only called by the satellite
//            node.
// PARAMETERS ::
// +        state : PhySatelliteRsvState* : A pointer to the physical layer
//          state machine state data for the current state machine
// +        hdr : PhySatelliteRsvHeader* : A pointer to the header of the
//          frame currently being processed by the physical layer state
//          machine
// RETURN :: void : This function does not return a value.
// **/

static
void PhySatelliteRsvUpdatePhyHeader(PhySatelliteRsvState* state,
                                    PhySatelliteRsvHeader* hdr)
{
    hdr->im3 = (float)PhySatelliteRsvMergeRatio(hdr->im3Valid,
                                                hdr->im3,
                                                PhySatelliteRsvGetIm3(state));
    hdr->im3Valid = TRUE;

    hdr->aci = (float)PhySatelliteRsvMergeRatio(hdr->aciValid,
                                                hdr->aci,
                                                state->aci);
    hdr->aciValid = TRUE;

    hdr->asi = (float)PhySatelliteRsvMergeRatio(hdr->asiValid,
                                                hdr->asi,
                                                state->asi);
    hdr->asiValid = TRUE;
}

// /**
// FUNCTION :: PhySatelliteRsvCheckRxPacketError
// LAYER :: PHY
// PURPOSE :: This routine calculates the Eb/No of the current packet
//            being received.  If the Eb/No is sufficiently high
//            the routine returns a false value to indicate that the
//            packet should be _kept_ and a true value if the packet
//            should be discarded.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        hdr : PhySatelliteRsvHeader* : A pointer to the header of the
//          frame currently being processed by the physical layer state
//          machine
// RETURN :: BOOL : This function returns TRUE if the packet should be
//           discarded and FALSE otherwise.
// **/

static
BOOL PhySatelliteRsvCheckRxPacketError(Node* node,
                                       PhySatelliteRsvState* state,
                                       PhySatelliteRsvHeader* hdr,
                                       double* sinrp,
                                       int phyIndex,
                                       int channelIndex,
                                       BOOL isHeader,
                                       double *rxPower_mW)
{
    double noise = state->thisPhy->noise_mW_hz * state->channelBandwidth;

    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                          phyIndex,
                                          channelIndex);

    double linksinr;

    if (isHeader)
    {
        linksinr = 10.0*log10(*rxPower_mW /
            (channel->interferencePowermW + noise));

    }
    else {

        linksinr = 10.0*log10(channel->rxMsgPowermW /
            (channel->interferencePowermW + noise));
    }

    double sinr = PhySatelliteRsvCalculateOverallSnr(hdr,
                                                     linksinr);

    double ebno = 0.0;

    if (sinrp != NULL) {
        *sinrp = sinr;
    }

    if (DEBUG) {
        printf("PhySatelliteRsvCheckRxPacketError() "
               "Node[%d] calculates C/N+I=%0.1lf (was %0.1lf)\n",
               node->nodeId,
               sinr,
               linksinr);
        printf("-->interference power is %0.1le mW, noise = %0.1lf dBm.\n",
               channel->interferencePowermW,
               10.0 * log10(noise));
    }

    ebno = sinr - 10 *log10(state->bitsPerSymbol)
        - 10*log10(state->codingRate);

    if (DEBUG) {
        printf("Eb/(No + Io) calculated to be %0.1f dB "
               "(minimum is %lf) \n",
               ebno,
               state->rxThreshold);
    }

    state->averageEbnoStat += ebno;
    state->averageEbnoCount++;

    if (ebno >= state->rxThreshold) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

// /**
// FUNCTION :: PhySatelliteRsvInit
// LAYER :: PHY
// PURPOSE :: This function initializes the Satellite RSV PHY process.  It is
//            generally called at the beginning of the simulation at time
//            t = 0 but may be called at other times under dynamic node
//            creation conditions.  This function is called by other functions
//            outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// +        nodeInput : NodeInput* : A pointer to the deserialized data
//          structure corresponding to the current simulation configuration
//          file and and any auxillary files references by the simulation
//          configuration file
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvInit(Node* node,
                         int phyIndex,
                         const NodeInput* nodeInput)
{
    PhySatelliteRsvState* state
        = (PhySatelliteRsvState*)MEM_malloc(sizeof(PhySatelliteRsvState));
    int myChannel = -1;
    int numChannels = PROP_NumberChannels(node);
    int i = 0;
    BOOL listening = FALSE;

    memset(state,
           0,
           sizeof(PhySatelliteRsvState));

    PhySatelliteRsvSetState(node,
                            phyIndex,
                            state);

    state->thisPhy = PhySatelliteRsvGetData(node,
                                            phyIndex);

    ANTENNA_Init(node,
                 phyIndex,
                 nodeInput);


    ERROR_Assert(state->thisPhy->phyModel == PHY_SATELLITE_RSV,
                 "Model only supports Satellite RSV physical layer.");

    PhySatelliteRsvInitializeStaticParameters(node,
                                              phyIndex);

    PhySatelliteRsvInitializeConfigurableParameters(node,
                                                    phyIndex,
                                                    nodeInput);

    PhySatelliteRsvInitializeState(node,
                                   phyIndex);

    if (DEBUG) {
        printf("Node[%d/%d]: numChannels = %d.\n",
               node->nodeId,
               phyIndex,
               numChannels);
    }

    state->statsWritten = FALSE;

    state->upDownChannelInfoList = NULL;
}

// /**
// FUNCTION :: PhySatelliteRsvFinalize
// LAYER :: PHY
// PURPOSE :: This function finalizes the Satellite RSV PHY process.  It is
//            generally called at the end of the simulation at the epoch but
//            may be called at other times under dynamic node
//            destruction conditions.  This function is called by other functions
//            outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvFinalize(Node* node,
                             int phyNum)
{
    char buf[BUFSIZ];
    int i;
    int channelNum = PhySatelliteRsvGetLinkChannelPairNum(node, phyNum);

    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyNum);

    if (state->statsWritten == TRUE) {
        return;
    }
    state->statsWritten = TRUE;

    for (i = 0; i < channelNum; i++)
    {
        PhySatelliteRsvTxRxChannelInfo* txRxChannelInfo;
        txRxChannelInfo =
            PhySatelliteRsvGetLinkChannelPairInfoByIndex(node,
                                                         phyNum,
                                                         i);
        PhySatelliteRsvStatistics rxStats =
            txRxChannelInfo->rxChannelInfo->stats;
        PhySatelliteRsvStatistics txStats =
            txRxChannelInfo->txChannelInfo->stats;

        sprintf(buf,
                "Signals transmitted [rx channel index: %d] = %d",
                txRxChannelInfo->rxChannelInfo->channelIndex,
                rxStats.totalTxSignals);
        IO_PrintStat(node,
                     "Physical",
                     "SatelliteRsv",
                     ANY_DEST,
                     phyNum,
                     buf);

        sprintf(buf,
                "Signals transmitted [tx channel index: %d] = %d",
                txRxChannelInfo->txChannelInfo->channelIndex,
                txStats.totalTxSignals);
        IO_PrintStat(node,
                     "Physical",
                     "SatelliteRsv",
                     ANY_DEST,
                     phyNum,
                     buf);

        double averageEbno(0.0);

        if (state->averageEbnoCount > 0) {
            averageEbno =
                state->averageEbnoStat / (double)state->averageEbnoCount;
        }

        sprintf(buf,
                "Average Eb/No (dB) = %0.1lf",
                averageEbno);

        IO_PrintStat(node,
                     "Physical",
                     "SatelliteRsv",
                     ANY_DEST,
                     phyNum,
                     buf);

        sprintf(buf,
                "Signals received and forwarded to MAC [rx channel index: %d] = %d",
                txRxChannelInfo->rxChannelInfo->channelIndex,
                rxStats.totalRxSignalsToMac);
        IO_PrintStat(node,
                     "Physical",
                     "SatelliteRsv",
                     ANY_DEST,
                     phyNum,
                     buf);

        sprintf(buf,
                "Signals received and forwarded to MAC [tx channel index: %d] = %d",
                txRxChannelInfo->txChannelInfo->channelIndex,
                txStats.totalRxSignalsToMac);
        IO_PrintStat(node,
                     "Physical",
                     "SatelliteRsv",
                     ANY_DEST,
                     phyNum,
                     buf);

        sprintf(buf,
                "Signals locked on by PHY [rx channel index: %d] = %d",
                txRxChannelInfo->rxChannelInfo->channelIndex,
                rxStats.totalSignalsLocked);
        IO_PrintStat(node,
                     "Physical",
                     "SatelliteRsv",
                     ANY_DEST,
                     phyNum,
                     buf);

        sprintf(buf,
                "Signals locked on by PHY [tx channel index: %d] = %d",
                txRxChannelInfo->txChannelInfo->channelIndex,
                txStats.totalSignalsLocked);
        IO_PrintStat(node,
                     "Physical",
                     "SatelliteRsv",
                     ANY_DEST,
                     phyNum,
                     buf);

        sprintf(buf,
                "Signals received but with errors [rx channel index: %d] = %d",
                txRxChannelInfo->rxChannelInfo->channelIndex,
                rxStats.totalSignalsWithErrors);
        IO_PrintStat(node,
                     "Physical",
                     "SatelliteRsv",
                     ANY_DEST,
                     phyNum,
                     buf);

        sprintf(buf,
                "Signals received but with errors [tx channel index: %d] = %d",
                txRxChannelInfo->txChannelInfo->channelIndex,
                txStats.totalSignalsWithErrors);
        IO_PrintStat(node,
                     "Physical",
                     "SatelliteRsv",
                     ANY_DEST,
                     phyNum,
                     buf);

        txRxChannelInfo->rxChannelInfo->currentMode = PHY_IDLE;
        txRxChannelInfo->rxChannelInfo->previousMode = PHY_IDLE;

        //Informing the energy model about the change in PhyStatus
        Phy_ReportStatusToEnergyModel(
        node,
        phyNum,
        txRxChannelInfo->rxChannelInfo->previousMode,
        PHY_IDLE);

        txRxChannelInfo->txChannelInfo->currentMode = PHY_IDLE;
        txRxChannelInfo->txChannelInfo->previousMode = PHY_IDLE;

        //Informing the energy model about the change in PhyStatus
        Phy_ReportStatusToEnergyModel(
        node,
        phyNum,
        txRxChannelInfo->txChannelInfo->previousMode,
        PHY_IDLE);
    }

}

// /**
// FUNCTION :: PhySatelliteRsvGetStatus
// LAYER :: PHY
// PURPOSE :: This function returns the current transceiver status for the
//            phyisical layer process. This function is called by other
//            functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// RETURN :: PhyStatusType : This function returns the instantaneous sample
//           of the mode of the physical layer transceiver.
// **/
PhyStatusType PhySatelliteRsvGetStatus(Node* node,
                                       int phyNum)
{
    ERROR_ReportError("This routine is deprecated and should not be called.");
    return PHY_IDLE;
}

PhyStatusType PhySatelliteRsvGetStatus(Node* node,
                                       int phyNum,
                                       int channelIndex)
{
    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                          phyNum,
                                          channelIndex);

    return channel->currentMode;
}

// /**
// FUNCTION :: PhySatelliteRsvTransmissionEnd
// LAYER :: PHY
// PURPOSE :: This function implements the model logic to be executed when
//            the system has completed transmitting a packet.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvTransmissionEnd(Node* node,
                                    int phyIndex,
                                    Message* msg)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);
    PhyData* phyData = state->thisPhy;
    int channelIndex = -1;
    NodeAddress address = 0xff;

    if (DEBUG) {
        printf("%0.6f: PhySatelliteRsvTransmissionEnd(): "
            "Node[%d/%d] ceases transmission.\n",
               (double)node->getNodeTime() / (double)SECOND, node->nodeId,
               phyIndex);
    }

    memcpy(&channelIndex,
            MESSAGE_ReturnInfo(msg, INFO_TYPE_PhyIndex),
            sizeof(int));

    ERROR_Assert(PhySatelliteRsvGetMode(node, phyIndex, channelIndex)
                 == PHY_TRANSMITTING,
                 "State machine indicates station not transmitting yet"
                 " ceasing transmission. Logic error.");

    //
    // should it tell if it is sensing?  Right now I think that just changing the
    // mode may not change the PROP environment so it would always change to sensing even
    // if it were not.
    //

    if (node->guiOption == TRUE) {
        switch(phyData->networkAddress->networkType) {
            case NETWORK_IPV4:
                address = phyData->networkAddress->interfaceAddr.ipv4;
                break;
            case NETWORK_IPV6:
                address = phyData->networkAddress->interfaceAddr.ipv4 & 0xff;
                break;
            default:
                ERROR_ReportError("Invalid network type\n");
        }
        GUI_EndBroadcast(node->nodeId,
                         GUI_PHY_LAYER,
                         GUI_DEFAULT_DATA_TYPE,
                         address,
                         node->getNodeTime());
    }

    if (ANTENNA_IsLocked(node, phyIndex) == 0) {
        ANTENNA_SetToDefaultMode(node,
                                 phyIndex);
    }

    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                 phyIndex,
                                 channelIndex);

    PHY_SignalInterference(node,
                           phyIndex,
                           channelIndex,
                           0,
                           0,
                           &(channel->interferencePowermW));

    PhySatelliteRsvSetMode(node,
                           phyIndex,
                           channelIndex,
                           PHY_IDLE);
}

// /**
// FUNCTION :: PhySatelliteRsvStartTransmittingSignal
// LAYER :: PHY
// PURPOSE :: This function implements the algorithm to transmit a signal
//            on the satellite subnet wireless channel.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyNum : int : The logical index of the current interface
// +        msg : Message* : A pointer to a data structure representing the
//          frame to be sent on the wireless satellite channel.
// +        useMacLayerSpecifiedDelay : BOOL : An indicator to use the
//          standard MAC-specified delay value
// +        delayUntilAirborne : clocktype : The number of nanoseconds until
//          the delay reaches the transmitter.
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvStartTransmittingSignal(Node* node,
                                            int phyNum,
                                            Message* msg,
                                            BOOL useMacLayerSpecifiedDelay,
                                            clocktype delayUntilAirborne)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyNum);
    PhyData* phyData = state->thisPhy;
    clocktype now = node->getNodeTime();
    int channelIndex = -1;

    clocktype duration = 0;
    MacSatelliteBentpipeHeader machdrs;
    MacSatelliteBentpipeHeader* machdr = &machdrs;

    PhySatelliteRsvHeader phyhdrs;
    PhySatelliteRsvHeader* phyhdr = &phyhdrs;

    Message* endMsg =  NULL;

    if (DEBUG) {
        printf("%0.6f: PhySatelliteRsvStartTransmittingSignal[%d/%d] "
               "Start transmission of packet %p/\n",
               (double)now / (double)SECOND,
               node->nodeId,
               phyNum,
               msg);
    }

    memcpy(&channelIndex,
            MESSAGE_ReturnInfo(msg, INFO_TYPE_PhyIndex),
            sizeof(int));

    ERROR_Assert(PhySatelliteRsvGetMode(node, phyNum, channelIndex) !=
        PHY_TRANSMITTING,
        "PHY cannot begin transmission until previous "
        "transmission completes.");

    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                 phyNum,
                                 channelIndex);

    // When this station starts to transmit, all other sources of
    // signal energy are considered to be interference.

    if (PhySatelliteRsvGetMode(node, phyNum, channelIndex) == PHY_RECEIVING) {
        if (phyData->antennaData->antennaModelType == ANTENNA_OMNIDIRECTIONAL) {
            channel->interferencePowermW += channel->rxMsgPowermW;
        }
        else {
            ANTENNA_SetToDefaultMode(node, phyNum);
            PHY_SignalInterference(node,
                                   phyNum,
                                   channelIndex,
                                   NULL,
                                   NULL,
                                   &(channel->interferencePowermW));
        }
        PhySatelliteRsvUnlockSignal(node, phyNum);
    }

    PhySatelliteRsvSetMode(node,
                           phyNum,
                           channelIndex,
                           PHY_TRANSMITTING);

    memcpy(machdr,
           MESSAGE_ReturnPacket(msg),
           sizeof(MacSatelliteBentpipeHeader));

    int packetsize = machdr->size;

    duration = PhySatelliteRsvGetFrameDuration(node,
                                               phyNum,
                                               packetsize);

    MESSAGE_AddHeader(node,
                  msg,
                      sizeof(PhySatelliteRsvHeader),
                      TRACE_SATELLITE_RSV);

    if (machdr->uplinkSnrValid == TRUE) {
        phyhdr->uplinkSnrValid = TRUE;
        phyhdr->uplinkSnr = machdr->uplinkSnr;
    }
    else {
        phyhdr->uplinkSnrValid = FALSE;
        phyhdr->uplinkSnr = -1.0;
    }

    phyhdr->asiValid = FALSE;
    phyhdr->aciValid = FALSE;
    phyhdr->im3Valid = FALSE;

    PhySatelliteRsvUpdatePhyHeader(state,
                                   phyhdr);

    memcpy(MESSAGE_ReturnPacket(msg),
           phyhdr,
           sizeof(PhySatelliteRsvHeader));

    //
    // Cease listening to current channel and prepare to transmit.
    //

    if (PHY_IsListeningToChannel(node, phyNum, channelIndex) == TRUE) {
        PHY_StopListeningToChannel(node,
                                   phyNum,
                                   channelIndex);
    }

    PROP_ReleaseSignal(node,
                       msg,
                       phyNum,
                       channelIndex,
                       (float)IN_DB(state->transmitPowermW),
                       duration,
                       state->guardTime);

    if (node->guiOption == TRUE) {
        GUI_Broadcast(node->nodeId,
                      GUI_PHY_LAYER,
                      GUI_DEFAULT_DATA_TYPE,
                      state->thisPhy->macInterfaceIndex,
                      node->getNodeTime());
    }

    // The PHY must inform itself when the current signal has concluded
    // transmission. It creates a message that is sent back to itself when
    // the transmission is scheduled to be complete.


    endMsg = MESSAGE_Alloc(node,
                           PHY_LAYER,
                           0,
                           MSG_PHY_TransmissionEnd);

    MESSAGE_AddInfo(node, endMsg, sizeof(int), INFO_TYPE_PhyIndex);
    memcpy(MESSAGE_ReturnInfo(endMsg, INFO_TYPE_PhyIndex),
           &channelIndex,
           sizeof(int));


    MESSAGE_SetInstanceId(endMsg,
                          (short)phyNum);
    MESSAGE_Send(node,
                 endMsg,
                 duration + state->guardTime);

    channel->stats.totalTxSignals++;
}

// /**
// FUNCTION :: PhySatelliteRsvSignalArrivalFromChannel
// LAYER :: PHY
// PURPOSE :: This function implements the algorithm to respond to a
//            signal arriving from the channel.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// +        channelIndex : int :  The logical index of the channel upon
//          which the channel is arriving.
// +        propRxInfo : PropRxInfo* : A pointer to a data structure
//          containing the propagation reception information at the
//          receiver.  Note this may contain data that would be
//          unavailable to a physically realizable implementation of a
//          receiver.
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvSignalArrivalFromChannel(Node* node,
                                             int phyIndex,
                                             int channelIndex,
                                             PropRxInfo* propRxInfo)
{
    // This routine provides information to the Physical layer process
    // that would not normally be available in a physically realizable
    // system.  Care should be taken to restrict the use of this routine
    // for simulation enhancements and abstractions and avoid the use
    // of algorithms that are destined to be directly implemented in a
    // physical communications product.

    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);
    clocktype now = node->getNodeTime();

    ERROR_Assert(PhySatelliteRsvGetMode(node, phyIndex, channelIndex) !=
                 PHY_TRANSMITTING,
                 "Satellite model only operates in half-duplex mode.  "
                 "Transmission cannot be received when station is also "
                 "sending.");



    if (DEBUG) {
        printf("%0.6f: Node[%d/%d] signal arrival from channel detected.\n",
               (double)now * 1.0e-9,
               node->nodeId,
               phyIndex);
    }

    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                 phyIndex,
                                 channelIndex);

    // The propagation kernel has informed the model of an arrival of a
    // new signal on the channel.  If the state of the model is:
    //
    // PHY_RECEIVING: This means that the system is already receiving a
    // frame.  Calculate the receiver power and determine if the packet
    // is received in error.  The new signal is considered to be
    // interference to that packet.
    //
    // PHY_IDLE | PHY_SENSING : If the input level of this signal is
    // high enough to trigger a carrier sense event in the PHY, then
    // lock the antenna (if not already locked) and update the
    // interference power.  If the level is not sufficient to trigger
    // the event, add the signal power to the interference level and
    // check to see if it is sufficiently high again.

    switch(PhySatelliteRsvGetMode(node, phyIndex, channelIndex)) {
        case PHY_RECEIVING: {
            double rxdb = ANTENNA_GainForThisSignal(node,
                                                    phyIndex,
                                                    propRxInfo);
            double rxPowermW = NON_DB(rxdb + propRxInfo->rxPower_dBm);

            if (channel->rxMsgError == FALSE) {
                PhySatelliteRsvHeader phyhdrs;
                PhySatelliteRsvHeader* phyhdr = &phyhdrs;

                memcpy(phyhdr,
                       MESSAGE_ReturnPacket(propRxInfo->txMsg),
                       sizeof(PhySatelliteRsvHeader));
                BOOL isheader = FALSE;
                channel->rxMsgError = PhySatelliteRsvCheckRxPacketError(
                                                              node,
                                                              state,
                                                              phyhdr,
                                                              0,
                                                              phyIndex,
                                                              channelIndex,
                                                              isheader,
                                                              NULL);
            }

            channel->rxTimeEvaluated = node->getNodeTime();
            channel->interferencePowermW += rxPowermW;

            break;
        }
        case PHY_IDLE:
        case PHY_SENSING: {
            PropTxInfo* propTxInfo
                = (PropTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg);
            clocktype txDuration = propTxInfo->duration;

            double rxdb = ANTENNA_GainForThisSignal(node,
                                                    phyIndex,
                                                    propRxInfo);
            double omniRxdb = ANTENNA_DefaultGainForThisSignal(node,
                                                               phyIndex,
                                                               propRxInfo);

            if (DEBUG) {
                printf("rxInterferencePower = %lf + %lf = %lf dB\n",
                       rxdb,
                       propRxInfo->rxPower_dBm,
                       rxdb + propRxInfo->rxPower_dBm);
            }

            double rxInterferencePowermW
                = (double)NON_DB(rxdb + propRxInfo->rxPower_dBm);
            double rxPowerInOmnimW
                = (double)NON_DB(omniRxdb + propRxInfo->rxPower_dBm);

            if (DEBUG) {
                printf("-->rxPowerInOmnimW = %lf dBm, receiveSensitivity=%lf dBm\n",
                       IN_DB(rxPowerInOmnimW),
                       state->receiveSensitivitydBm);
            }


            double rxPowermW(0);

            if (ANTENNA_IsLocked(node, phyIndex) == FALSE) {
                ANTENNA_SetToBestGainConfigurationForThisSignal(node,
                                                            phyIndex,
                                                            propRxInfo);
            }

            PHY_SignalInterference(node,
                                   phyIndex,
                                   channelIndex,
                                   propRxInfo->txMsg,
                                   &rxPowermW,
                                   &channel->interferencePowermW);

            BOOL isheader = TRUE;

            PhySatelliteRsvHeader phyhdrs;
            PhySatelliteRsvHeader* phyhdr = &phyhdrs;

            memcpy(phyhdr,
                   MESSAGE_ReturnPacket(propRxInfo->txMsg),
                   sizeof(PhySatelliteRsvHeader));

            BOOL isHeaderError =
                        PhySatelliteRsvCheckRxPacketError(
                                              node,
                                              state,
                                              phyhdr,
                                              0,
                                              phyIndex,
                                              channelIndex,
                                              isheader,
                                              &rxPowermW);

            if ((rxPowerInOmnimW > NON_DB(state->receiveSensitivitydBm)) &&
               (!isHeaderError)) {

                PhySatelliteRsvLockSignal(
                              node,
                              phyIndex,
                              channelIndex,
                              propRxInfo->txMsg,
                              rxPowermW,
                              propRxInfo->rxStartTime + propRxInfo->duration);

                PhySatelliteRsvSetMode(node,
                                       phyIndex,
                                       channelIndex,
                                       PHY_RECEIVING);

                PhySatelliteRsvReportExtendedStatusToMac(node,
                                                         phyIndex,
                                                         channelIndex,
                                                         PHY_RECEIVING,
                                                         txDuration,
                                                         propRxInfo->txMsg);
            }
            else {
                PhyStatusType oldMode = PhySatelliteRsvGetMode(node,
                                                               phyIndex,
                                                               channelIndex);

                channel->interferencePowermW += rxInterferencePowermW;

                if (PhySatelliteRsvIsSensingCarrier(node,
                                                phyIndex,
                                                channelIndex) == TRUE) {
                    PhySatelliteRsvSetMode(node,
                                           phyIndex,
                                           channelIndex,
                                           PHY_SENSING);
                }
                else {
                    PhySatelliteRsvSetMode(node,
                                           phyIndex,
                                           channelIndex,
                                           PHY_IDLE);
                }
            }
            break;
        }
        default:
            ERROR_ReportError("Unsupport PHY Interrupt Type.");
    }
}

// /**
// FUNCTION :: PhySatelliteRsvSignalEndFromChannel
// LAYER :: PHY
// PURPOSE :: This function implements the model logic to respond to
//            a completed reception of a packet.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyNum : int : The logical index of the current interface
// +        msg : Message* : A pointer to a data structure representing the
//          frame to be sent on the wireless satellite channel.
// +        channelIndex : int : The logical channel index the channel
//          that is receiving the frame.
// +        propRxInfo : PropRxInfo* : A pointer to a data structure
//          containing the local receiver environment for frame
//          reception.
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvSignalEndFromChannel(Node *node,
                                         int phyIndex,
                                         int channelIndex,
                                         PropRxInfo *propRxInfo)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);
    clocktype now = node->getNodeTime();
    double sinr = -1.0;

    BOOL receiveErrorOccurred = FALSE;

    if (DEBUG) {
        printf("%0.6f: Node[%d/%d] signal end from channel detected.\n",
               (double)now * 1.0e-9,
               node->nodeId,
               phyIndex);
    }

    ERROR_Assert(PhySatelliteRsvGetMode(node,
                                    phyIndex,
                                    channelIndex) != PHY_TRANSMITTING,
                 "PHY can only operate in half-duplex mode.  A "
                 "signal cannot be received when the station is "
                 "transmitting on the same interface.");

    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                 phyIndex,
                                 channelIndex);

    PropProfile* propProfile = node->propChannel[channelIndex].profile;

    if (propProfile->enableChannelOverlapCheck) {

        PHY_SignalInterference(node,
                           phyIndex,
                           channelIndex,
                           0,
                           0,
                           &channel->interferencePowermW);
    }

    if (PhySatelliteRsvGetMode(node, phyIndex, channelIndex) == PHY_RECEIVING) {
        if (channel->rxMsgError == FALSE) {
            PhySatelliteRsvHeader phyhdrs;
            PhySatelliteRsvHeader* phyhdr = &phyhdrs;

            memcpy(phyhdr,
                   MESSAGE_ReturnPacket(propRxInfo->txMsg),
                   sizeof(PhySatelliteRsvHeader));
            BOOL isheader = FALSE;
            channel->rxMsgError = PhySatelliteRsvCheckRxPacketError(
                                                                node,
                                                                state,
                                                                phyhdr,
                                                                &sinr,
                                                                phyIndex,
                                                                channelIndex,
                                                                isheader,
                                                                NULL);
            channel->rxTimeEvaluated = node->getNodeTime();
        }
    }

    receiveErrorOccurred = channel->rxMsgError;

    if (DEBUG) {
        const char* sameMessageStr = NULL;

        if (propRxInfo->txMsg == channel->rxMsg) {
            sameMessageStr = "TRUE";
        }
        else {
            sameMessageStr = "FALSE";
        }

        printf("PhySatelliteRsvGetMode(%d, %d, %d) = %d (should be %d).\n"
               "-->rxMsg (%p) == txMsg (%p) [%s].\n",
               node->nodeId,
               phyIndex,
               channelIndex,
               PhySatelliteRsvGetMode(node, phyIndex, channelIndex),
               PHY_RECEIVING,
               channel->rxMsg,
               propRxInfo->txMsg,
               sameMessageStr);
    }

    // If the current farme is complete and the same signal as the
    // locked signal (since a number of other shorter frames could have
    // started and ended within the transmission time of the current
    // locked frame), reset the state of the receiver and if the
    // message was successfully recieved, remove the header and forward
    // it on to the MAC process.  If it is not the same packet, remove
    // the signal from the interference power and update the results
    // accordingly.

    if (PhySatelliteRsvGetMode(node, phyIndex, channelIndex) == PHY_RECEIVING
        && channel->rxMsg == propRxInfo->txMsg) {
        if (ANTENNA_IsLocked(node, phyIndex) == FALSE) {
            ANTENNA_SetToDefaultMode(node, phyIndex);
            PHY_SignalInterference(node,
                                   phyIndex,
                                   channelIndex,
                                   0,
                                   0,
                                   &channel->interferencePowermW);
        }

        PhySatelliteRsvUnlockSignal(node,
                                    phyIndex);

        if (PhySatelliteRsvIsSensingCarrier(node,
                                            phyIndex,
                                            channelIndex) == TRUE) {
            PhySatelliteRsvSetMode(node,
                                   phyIndex,
                                   channelIndex,
                                   PHY_SENSING);
        }
        else {
            PhySatelliteRsvSetMode(node,
                                   phyIndex,
                                   channelIndex,
                                   PHY_IDLE);
        }

        if (receiveErrorOccurred == FALSE) {
            Message* newMsg = MESSAGE_Duplicate(node,
                                                propRxInfo->txMsg);

            MESSAGE_RemoveHeader(node,
                                 newMsg,
                                 sizeof(PhySatelliteRsvHeader),
                                 TRACE_SATELLITE_RSV);

            MESSAGE_InfoAlloc(node,
                              newMsg,
                              sizeof(float));

            float fsinr = (float)sinr;
            memcpy(MESSAGE_ReturnInfo(newMsg),
                   &fsinr,
                   sizeof(float));

            MESSAGE_SetInstanceId(newMsg,
                                  (short)phyIndex);

            MESSAGE_AddInfo(node, newMsg, sizeof(int), INFO_TYPE_PhyIndex);
            memcpy(MESSAGE_ReturnInfo(newMsg, INFO_TYPE_PhyIndex),
                   &channelIndex, sizeof(int));

            MAC_ReceivePacketFromPhy(node,
                                     node->phyData[phyIndex]->macInterfaceIndex,
                                     newMsg);

            channel->stats.totalRxSignalsToMac++;
        }
        else {
            channel->stats.totalSignalsWithErrors++;
        }
    }
    else {
        double rxdb = ANTENNA_GainForThisSignal(node,
                                                phyIndex,
                                                propRxInfo);
        double rxPowermW = NON_DB(rxdb + propRxInfo->rxPower_dBm);

        channel->interferencePowermW -= rxPowermW;

        if (channel->interferencePowermW < 0.0) {
            channel->interferencePowermW = 0.0;
        }

        PhyStatusType newMode;
        if (PhySatelliteRsvGetMode(node,
                                   phyIndex,
                                   channelIndex) != PHY_RECEIVING) {
            newMode = PHY_SENSING;
        }
        else {
            newMode = PHY_IDLE;
        }

        if (newMode != PhySatelliteRsvGetMode(node, phyIndex, channelIndex)) {
            PhySatelliteRsvSetMode(node,
                                   phyIndex,
                                   channelIndex,
                                   newMode);
        }
    }
}

// /**
// FUNCTION :: PhySatelliteRsvGetTxDataRate
// LAYER :: PHY
// PURPOSE :: This function calculates the current transmission rate
//            of the currently selected transmission channel.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: int : The current transmission rate expressed as bits/sec
// **/

int PhySatelliteRsvGetTxDataRate(PhyData* phyData)
{
    PhySatelliteRsvState* state = (PhySatelliteRsvState*)phyData->phyVar;

    return (int)state->transmitDataRate;
}

// /**
// FUNCTION :: PhySatelliteRsvGetRxDataRate
// LAYER :: PHY
// PURPOSE :: This function calculates the current signaling rate
//            of the packet currently being received.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: int : The current reception rate expressed as bits/sec
// **/

int PhySatelliteRsvGetRxDataRate(PhyData* phyData)
{
    PhySatelliteRsvState* state = (PhySatelliteRsvState*)phyData->phyVar;

    return (int)state->receiveDataRate;
}

// /**
// FUNCTION :: PhySatelliteRsvSetTxDataRateType
// LAYER :: PHY
// PURPOSE :: This function implements model logic to set the current
//            data rate for transmission to a value consistent with
//            the provided data rate type.  This function is called
//            by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// +        dateRateType : int : An enumerated set of available data
//          rate types.  Note that this must be a valid data rate
//          type of the program will exhibit unexpected behavior.
// RETURN :: void : This function does not return any value.
// **/

void PhySatelliteRsvSetTxDataRateType(PhyData* thisPhy,
                                      int dataRateType)
{
    PhySatelliteRsvState* state = (PhySatelliteRsvState*)thisPhy->phyVar;
    ERROR_Assert(dataRateType < state->dataRateCount,
                 "The data rate requested by MAC is not supported "
                 "by this physical layer model.");

    state->transmitDataRateType = (PhySatelliteRsvDataRateType)dataRateType;
    state->receiveDataRateType = (PhySatelliteRsvDataRateType)dataRateType;

    //state->transmitDataRate = state->[dataRateType].dataRate;
    state->transmitDataRate
        = PhySatelliteRsvStateProfiles[dataRateType]->bitsPerSymbol
        * state->channelBandwidth;
    state->transmitPowermW
        = PhySatelliteRsvStateProfiles[dataRateType]->transmitPowermW;

    state->receiveDataRate = state->transmitDataRate;
    state->receiveSensitivitydBm
        = PhySatelliteRsvStateProfiles[dataRateType]->sensitivitydBm;
}

// /**
// FUNCTION :: PhySatelliteRsvGetRxDataRateType
// LAYER :: PHY
// PURPOSE :: This function returns the current data rate type for
//            the receive function of physical layer process as
//            an enumerated value.
//            This function is called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: int : This function returns an enumerated data rate type.
// **/

int PhySatelliteRsvGetRxDataRateType(PhyData* thisPhy)
{
    PhySatelliteRsvState* state = (PhySatelliteRsvState*)thisPhy->phyVar;

    return state->receiveDataRateType;
}

// /**
// FUNCTION :: PhySatelliteRsvGetTxDataRateType
// LAYER :: PHY
// PURPOSE :: This function returns the current data rate type for
//            the transmit function of physical layer process as
//            an enumerated value.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: int : This function returns an enumerated data rate type.
// **/

int PhySatelliteRsvGetTxDataRateType(PhyData* thisPhy)
{
    PhySatelliteRsvState* state = (PhySatelliteRsvState*)thisPhy->phyVar;

    return state->transmitDataRateType;
}

// /**
// FUNCTION :: PhySatelliteRsvSetLowestTxDataRateType
// LAYER :: PHY
// PURPOSE :: This function executes logic that sets the transmission rate
//            for the channel to be the lowest available rate for that
//            physical layer process.  This function is called by other
//            functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvSetLowestTxDataRateType(PhyData* thisPhy)
{
    ERROR_ReportError("SetLowestTxDataRateType unsupported.\n");
}

// /**
// FUNCTION :: PhySatelliteRsvGetLowestTxDataRateType
// LAYER :: PHY
// PURPOSE :: This function returns the lowest available transmission
//            rate available for the current physical layer process.  This
//            function is called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// +        dataRateType : int* : A pointer to a memory region sufficiently
//          large and aligned to hold an integer data type.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvGetLowestTxDataRateType(PhyData* thisPhy,
                                            int* dataRateType)
{
    ERROR_ReportError("GetLowestTxDataRateType unsupported.\n");
}

// /**
// FUNCTION :: PhySatelliteRsvSetHighestTxDataRateType
// LAYER :: PHY
// PURPOSE :: This function sets the transmission rate of the local
//            physical layer process to its maximum rate possible.  This
//            function is called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvSetHighestTxDataRateType(PhyData* thisPhy)
{
    ERROR_ReportError("SetHighestTxDataRateType unsupported.\n");
}

// /**
// FUNCTION :: PhySatelliteRsvGetHighestTxDataRateType
// LAYER :: PHY
// PURPOSE :: This function calculates the maximum transmission rate
//            of the local physical layer process.  This
//            function is called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// +        dataRateType : int* : A pointer to a memory region sufficiently
//          large and aligned to hold an integer data type.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvGetHighestTxDataRateType(PhyData* thisPhy,
                                          int* dataRateType)
{
    ERROR_ReportError("GetHighestTxDataRateType unsupported.\n");
}

// /**
// FUNCTION :: PhySatelliteRsvSetHighestTxDataRateTypeForBC
// LAYER :: PHY
// PURPOSE :: This function sets the transmission rate of the local
//            physical layer process to its maximum rate possible
//            for broadcast transmissions.  This function is called by
//            other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvSetHighestTxDataRateTypeForBC(PhyData* thisPhy)
{
    ERROR_ReportError("SetHighestTxDataRateTypeForBC unsupported.\n");
}

// /**
// FUNCTION :: PhySatelliteRsvGetHighestTxDataRateTypeForBC
// LAYER :: PHY
// PURPOSE :: This function calculates the maximum transmission rate for
//            broadcast transmissions of the local physical layer process.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// +        dataRateType : int* : A pointer to a memory region sufficiently
//          large and aligned to hold an integer data type.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvGetHighestTxDataRateTypeForBC(PhyData* thisPhy,
                                             int* dataRateType)
{
    ERROR_ReportError("GetHighestTxDataRateTypeForBC unsupported.\n");
}

// /**
// FUNCTION :: PhySatelliteRsvGetTransmissionDuration
// LAYER :: PHY
// PURPOSE :: This function calculates the amount of signaling
//            time required to send a set number of bits through
//            the channel. This function is called by other functions
//            outside of this file.
// PARAMETERS ::
// +         size : int : The number of bits to be sent on the channel.
// +         dataRate : int : The available bit rate on this channel
// RETURN :: clocktype : An integer number of nanoseconds required to
//           complete the transmission.
// **/

clocktype PhySatelliteRsvGetTransmissionDuration(int size,
                                                 int dataRate)
{
    double t = (double)size / (double)dataRate;
    clocktype duration = (clocktype)(t * 1.0e9);

    return duration;
}

// /**
// FUNCTION :: PhySatelliteRsvSetTransmitPower
// LAYER :: PHY
// PURPOSE :: This function sets the internal state of the physical
//            layer process to a new value of the power available to
//            the local transmitter.  This
//            function is called by other functions outside of this file.
// PARAMETERS ::
// +         phyData : PhyData* : A pointer to a data structure containing
//           generic physical layer process data
// +         newTxPower_mW : double : A non-negative value for the power
//           of the transmit process of the transceiver
// RETURN :: void : NULL
// **/

void PhySatelliteRsvSetTransmitPower(PhyData* phyData,
                                     double newTxPower_mW)
{
    PhySatelliteRsvState* state = (PhySatelliteRsvState*)phyData->phyVar;

    state->transmitPowermW = newTxPower_mW;
}

// /**
// FUNCTION :: PhySatelliteRsvGetTransmitPower
// LAYER :: PHY
// PURPOSE :: This function calculates the internal power available to
//            the transmission component of the physical layer process.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         phyData : PhyData* : A pointer to a data structure containing
//           generic physical layer process data
// +         txPower_mW : double* : A pointer to a memory region that is
//           sufficiently large and properly aligned to store a double
//           length floating point number.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvGetTransmitPower(PhyData* phyData, double* txPower_mW)
{
    PhySatelliteRsvState* state = (PhySatelliteRsvState*)phyData->phyVar;

    if (txPower_mW != NULL) {
        *txPower_mW = state->transmitPowermW;
    }
}

// /**
// FUNCTION :: PhySatelliteRsvGetLastAngleOfArrival
// LAYER :: PHY
// PURPOSE :: This function calculates the angle of arrival of the last frame
//            received by this transceiver process.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to a memory region that contains the
//           generic node state information for the currently executing
//           event.
// +         phyNum : int : The logical index of the physical layer
//           process within the node context.
// RETURN :: double : The return value is the angle of arrival calculated
//           as a rotated angle from boresight.  This is a projection of
//           phi and theta errors into a single off-axis angle.
// **/

double PhySatelliteRsvGetLastAngleOfArrival(Node* node,
                                            int phyNum)
{

    ERROR_ReportError("Get Last AOA not supported in this PHY.\n");
    return 0.0;
}

// /**
// FUNCTION :: PhySatelliteRsvTerminateCurrentReceive
// LAYER :: PHY
// PURPOSE :: This function invokes the process logic to reset the
//            receive state machine during transmission.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to the data structure containing
//           information about the currently executing node context.
// +         phyIndex : int : The logical index of the interface of the
//           currently executing node.
// +         terminateOnlyOnRecieveError : BOOL : A flag to indicate
//           whether or not this call should complete successfully
//           (terminate the reception process) only in the case that
//           the current reception is already known to be in error.
// +         receiveError : BOOL* : A pointer to a suitably sized and
//           appropriately aligned memory region to hold a copy of the
//           current reception status of the active frame.
// +         endSignalTime : clocktype* : A pointer to a memory region that
//           is suitably large and properly aligned to hold a copy of the
//           present time estimated for the packet to complete reception
//           at the local node.
// RETURN :: void : NULL
// **/

void
PhySatelliteRsvTerminateCurrentReceive(Node* node,
                                       int phyIndex,
                                       const BOOL terminateOnlyOnReceiveError,
                                       BOOL* receiveError,
                                       clocktype* endSignalTime)
{
    // This routine provides information to the Physical layer process
    // that would not normally be available in a physically realizable
    // system.  Care should be taken to restrict the use of this routine
    // for simulation enhancements and abstractions and avoid the use
    // of algorithms that are destined to be directly implemented in a
    // physical communications product.

    ERROR_ReportError("Transmission truncation not presently supported.\n");
}

// /**
// FUNCTION :: PhySatelliteRsvStartTransmittingSignalDirectionally
// LAYER :: PHY
// PURPOSE :: This function invokes the process logic to begin the
//            transmission of a frame using the specified azimuth
//            angle.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to the data structure containing
//           information about the currently executing node context.
// +         phyNum : int : The logical index of the interface of the
//           currently executing node.
// +         msg : Message* : A pointer to a memor region holding a
//           Message data structure that represents the frame that is to
//           be sent on the currently configured channel.
// +         useMacLayerSpecifiedDelay : BOOL : A flag to indicate whether
//           or not the process should use the generic value in the MAC
//           layer as the specified latency through the MAC layer.
// +         delayUntilAirborne : clocktype : The number of nanoseconds
//           until the signal should become active at the output of the
//           transceiver.
// +         azimuthAngle : double : The current azimuth of the physical
//           system.  It is assumed that the directional antenna can
//           control this one degree of freedom in the model API.
// RETURN :: void : NULL
// **/

void
PhySatelliteRsvStartTransmittingSignalDirectionally(Node* node,
                                            int phyNum,
                                            Message* msg,
                                            BOOL useMacLayerSpecifiedDelay,
                                            clocktype delayUntilAirborne,
                                            double azimuthAngle)
{
    ERROR_ReportError("Directional antennas are not supported in this version"
                    " of the SATELLITE-RSV PHY.\n");
}

// /**
// FUNCTION :: PhySatelliteRsvLockAntennaDirection
// LAYER :: PHY
// PURPOSE :: This function invokes the process logic to lock the
//            transceiver antenna in its current physical configuration.
//            When this call completes the transceiver will not modify its
//            gain configuration until an equivalent call to
//            unlock the antenna direction.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to the data structure containing
//           information about the currently executing node context.
// +         phyNum : int : The logical index of the interface of the
//           currently executing node.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvLockAntennaDirection(Node* node,
                                         int phyNum)
{
    ANTENNA_LockAntennaDirection(node,
                                 phyNum);
}

// /**
// FUNCTION :: PhySatelliteRsvUnlockAntennaDirection
// LAYER :: PHY
// PURPOSE :: This function invokes the process logic to unlock the
//            transceiver antenna in its current physical configuration.
//            After this call completes, the transceiver will be allowed
//            to freely modify its antenna gain pattern until any future
//            call to lock the gain pattern.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to the data structure containing
//           information about the currently executing node context.
// +         phyNum : int : The logical index of the interface of the
//           currently executing node.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvUnlockAntennaDirection(Node* node,
                                           int phyNum)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyNum);
    int channelIndex = -1;

    ANTENNA_UnlockAntennaDirection(node,
                                   phyNum);
    if (PhySatelliteRsvGetMode(node, phyNum, channelIndex) != PHY_RECEIVING &&
        PhySatelliteRsvGetMode(node, phyNum, channelIndex) != PHY_TRANSMITTING) {
        PHY_GetTransmissionChannel(node,
                                   phyNum,
                                   &channelIndex);

        ANTENNA_SetToDefaultMode(node,
                                 phyNum);

    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                 phyNum,
                                 channelIndex);

        PHY_SignalInterference(node,
                               phyNum,
                               channelIndex,
                               0,
                               0,
                               &channel->interferencePowermW);
    }
}

// /**
// FUNCTION :: PhySatelliteRsvMediumIsIdleInDirection
// LAYER :: PHY
// PURPOSE :: This function invokes the process logic to calculate
//            whether or not the physical, wireless, network is idle
//            in the given azimuth angle.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to the data structure containing
//           information about the currently executing node context.
// +         phyNum : int : The logical index of the interface of the
//           currently executing node.
// +         azimuth : double : The azimuth angle at which the calculation
//           is to occur.
// RETURN :: BOOL : A binary response that indicates whether or not the
//           signal level is sufficiently high to trigger the transceiver
//           or not.
// **/

BOOL PhySatelliteRsvMediumIsIdleInDirection(Node* node,
                                            int phyNum,
                                            double azimuth)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyNum);
    BOOL isIdle = FALSE;
    int channelIndex = -1;
    double oldInterferencePower = -1.0;

    PHY_GetTransmissionChannel(node,
                               phyNum,
                               &channelIndex);

    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                 phyNum,
                                 channelIndex);

    ERROR_Assert(channel->currentMode == PHY_IDLE
                 || channel->currentMode == PHY_SENSING,
                 "This routine should not be called in if the "
                 "current state is not in IDLE or SENSING.");

    // If the channel is locked, then the existing state information
    // can be used to whether the wireless channel is perceived to be
    // idle in the present direction.

    if (ANTENNA_IsLocked(node, phyNum) != 0) {
        if (channel->currentMode == PHY_IDLE) {
            isIdle = TRUE;
        }
        else {
            isIdle = FALSE;
        }

        return isIdle;
    }

    // If the system, however, is not locked currently then the
    // interference calculation has to be performed to ascertain
    // whether or not the incoming signal is sufficiently high
    // to sense carrier.

    ANTENNA_SetBestConfigurationForAzimuth(node,
                                           phyNum,
                                           azimuth);

    oldInterferencePower = channel->interferencePowermW;
    PHY_SignalInterference(node,
                           phyNum,
                           channelIndex,
                           0,
                           0,
                           &(channel->interferencePowermW));

    if (PhySatelliteRsvIsSensingCarrier(node, phyNum, channelIndex) == TRUE) {
        isIdle = FALSE;
    }
    else {
        isIdle = TRUE;
    }

    channel->interferencePowermW = oldInterferencePower;

    ANTENNA_SetToDefaultMode(node,
                             phyNum);

    return isIdle;
}

// /**
// FUNCTION :: PhySatelliteRsvSetSensingDirection
// LAYER :: PHY
// PURPOSE :: This function invokes the process logic to set the
//            sensing direction of the transceiver.
//            This function is called by other functions outside of this
//            file.
// PARAMETERS ::
// +         node : Node* : A pointer to the data structure containing
//           information about the currently executing node context.
// +         phyNum : int : The logical index of the interface of the
//           currently executing node.
// +         azimuth : double : The new azimuth angle for the
//           transceiver.
// RETURN :: void : NULL
// **/

void PhySatelliteRsvSetSensingDirection(Node* node,
                                        int phyNum,
                                        double azimuth)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyNum);

    int channelIndex = -1;
    PHY_GetTransmissionChannel(node,
                               phyNum,
                               &channelIndex);

    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                 phyNum,
                                 channelIndex);

    ERROR_Assert(channel->currentMode == PHY_IDLE ||
                 channel->currentMode == PHY_SENSING,
                 "This model does not support setting the sensing "
                 "directoin unless the PHY is currently in the "
                 "IDLE or SENSING states.");

    ANTENNA_SetBestConfigurationForAzimuth(node,
                                           phyNum,
                                           azimuth);

    PHY_SignalInterference(node,
                           phyNum,
                           channelIndex,
                           0,
                           0,
                           &channel->interferencePowermW);
}

// /**
// FUNCTION :: PhySatelliteRsvSetChannelInfo
// LAYER :: PHY
// PURPOSE :: This function stes the default value to the parameters of
//            rxTxChannelInfoPtr.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyNum : int : The logical index of the current interface
// +        rxTxChannelInfoPtr : PhySatelliteRsvTxRxChannelInfo* : instance of
//          PhySatelliteRsvTxRxChannelInfo which need to be initialized.
// RETURN :: void : NULL.
// **/

void
PhySatelliteRsvSetChannelInfo(
                        Node *node,
                        int phyNum,
                        PhySatelliteRsvTxRxChannelInfo* rxTxChannelInfoPtr)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyNum);

    memset(&rxTxChannelInfoPtr->txChannelInfo->stats,
           0,
           sizeof(PhySatelliteRsvStatistics));

    rxTxChannelInfoPtr->txChannelInfo->interferencePowermW = 0.0;
    rxTxChannelInfoPtr->txChannelInfo->noisePowermW =
        state->thisPhy->noise_mW_hz * state->channelBandwidth;

    rxTxChannelInfoPtr->txChannelInfo->currentMode = PHY_IDLE;
    rxTxChannelInfoPtr->txChannelInfo->previousMode = PHY_IDLE;

    rxTxChannelInfoPtr->txChannelInfo->rxMsg = 0;
    rxTxChannelInfoPtr->txChannelInfo->rxMsgPowermW = 0.0;
    rxTxChannelInfoPtr->txChannelInfo->rxMsgError = FALSE;
    rxTxChannelInfoPtr->txChannelInfo->rxTimeEvaluated = 0;

    Phy_ReportStatusToEnergyModel(
        node,
        phyNum,
        rxTxChannelInfoPtr->txChannelInfo->previousMode,
        PHY_IDLE);

    memset(&rxTxChannelInfoPtr->rxChannelInfo->stats,
           0,
           sizeof(PhySatelliteRsvStatistics));

    rxTxChannelInfoPtr->rxChannelInfo->interferencePowermW = 0.0;
    rxTxChannelInfoPtr->rxChannelInfo->noisePowermW =
        state->thisPhy->noise_mW_hz * state->channelBandwidth;

    rxTxChannelInfoPtr->rxChannelInfo->currentMode = PHY_IDLE;
    rxTxChannelInfoPtr->rxChannelInfo->previousMode = PHY_IDLE;

    rxTxChannelInfoPtr->rxChannelInfo->rxMsg = 0;
    rxTxChannelInfoPtr->rxChannelInfo->rxMsgPowermW = 0.0;
    rxTxChannelInfoPtr->rxChannelInfo->rxMsgError = FALSE;
    rxTxChannelInfoPtr->rxChannelInfo->rxTimeEvaluated = 0;

    Phy_ReportStatusToEnergyModel(
        node,
        phyNum,
        rxTxChannelInfoPtr->rxChannelInfo->previousMode,
        PHY_IDLE);

    PhySatelliteRsvResetStatistics(rxTxChannelInfoPtr);
}

// /**
// FUNCTION :: PhySatelliteRsvAddUpDownLinkChannel
// LAYER :: PHY
// PURPOSE :: This function adds a new TxRxChannelInfoPair to the linked list
//          upDownChannelInfoList.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// +        rxLinkChannelId : int : upLink channel index for satellite.and
//          downlink channel index for ground station
// +        txLinkChannelId : int : downlink channel index for satellite.and
//          upLink channel index for ground station
// RETURN :: void : NULL.
// **/

void
PhySatelliteRsvAddUpDownLinkChannel(
    Node* node,
    int phyIndex,
    int rxLinkChannelId,
    int txLinkChannelId,
    const char* role)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);

    int channelIndex = 0;

    PhySatelliteRsvTxRxChannelInfo* rxTxChannelInfoPtr =
        (PhySatelliteRsvTxRxChannelInfo*)
        MEM_malloc(sizeof(PhySatelliteRsvTxRxChannelInfo));

    rxTxChannelInfoPtr->rxChannelInfo =
        (PhySatelliteRsvChannel *)
        MEM_malloc(sizeof(PhySatelliteRsvChannel));

    rxTxChannelInfoPtr->txChannelInfo =
        (PhySatelliteRsvChannel *)
        MEM_malloc(sizeof(PhySatelliteRsvChannel));

    rxTxChannelInfoPtr->rxChannelInfo->channelIndex = rxLinkChannelId;
    rxTxChannelInfoPtr->txChannelInfo->channelIndex = txLinkChannelId;

    PhySatelliteRsvSetChannelInfo(node, phyIndex, rxTxChannelInfoPtr);

    if (state->upDownChannelInfoList == NULL)
        {
        ListInit(node,
                 &state->upDownChannelInfoList);
        }

    ListInsert(node,
               state->upDownChannelInfoList,
               node->getNodeTime(),
               (void*)rxTxChannelInfoPtr);

    if (strcmp(role , "GROUND-STATION") == 0)
    {
        PHY_SetTransmissionChannel(node, phyIndex, txLinkChannelId);

        //start listening on downlink channel
        if (PHY_IsListeningToChannel(node, phyIndex, rxLinkChannelId)
                == FALSE)
        {
            PHY_StartListeningToChannel(node, phyIndex, rxLinkChannelId);
        }

        for (int i = 0 ; i < node->numberChannels ; i++)
        {
            if (i == rxLinkChannelId)
            {
                continue;
            }

            if (PHY_IsListeningToChannel(node, phyIndex, i) == TRUE)
            {
                PHY_StopListeningToChannel(node, phyIndex, i);
            }
        }
    }//end of if
    else // SATELLITE
    {
        //start listening on uplink channel
        if (PHY_IsListeningToChannel(node, phyIndex, rxLinkChannelId)
                == FALSE)
        {
            PHY_StartListeningToChannel(node, phyIndex, rxLinkChannelId);
        }

        PHY_SetTransmissionChannel(node, phyIndex, txLinkChannelId);

        if (PHY_IsListeningToChannel(node, phyIndex, txLinkChannelId)
                == TRUE)
        {
            PHY_StopListeningToChannel(node, phyIndex, txLinkChannelId);
        }
    }
}

// /**
// FUNCTION :: PhySatelliteRsvGetRxLinkChannelInfo
// LAYER :: PHY
// PURPOSE :: This function returns RxLinkChannelInfo for channel index
//          channelId.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// +        channelId : int : channel index of required channel info.
// RETURN :: PhySatelliteRsvChannel* : This function returns
//          RxLinkChannelInfo for channel index channelId.
// **/

PhySatelliteRsvChannel*
PhySatelliteRsvGetRxLinkChannelInfo(
    Node* node,
    int phyIndex,
    int channelId)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                      phyIndex);

    if (state->upDownChannelInfoList != NULL)
    {
        ListItem* listItem = state->upDownChannelInfoList->first;
        PhySatelliteRsvTxRxChannelInfo* upDownChannelInfoPtr = NULL;

        while (listItem)
        {
            upDownChannelInfoPtr = (PhySatelliteRsvTxRxChannelInfo*)
                listItem->data;

            if (upDownChannelInfoPtr->rxChannelInfo->channelIndex ==
                    channelId)
            {
                return upDownChannelInfoPtr->rxChannelInfo;
            }

            listItem = listItem->next;
        }
    }

    return NULL;
}

// /**
// FUNCTION :: PhySatelliteRsvGetTxLinkChannelInfo
// LAYER :: PHY
// PURPOSE :: This function returns TxLinkChannelInfo for channel index
//          channelId.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// +        channelId : int : channel index of required channel info.
// RETURN :: PhySatelliteRsvChannel* : This function returns
//          TxLinkChannelInfo for channel index channelId.
// **/

PhySatelliteRsvChannel*
PhySatelliteRsvGetTxLinkChannelInfo(
    Node* node,
    int phyIndex,
    int channelId)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                      phyIndex);

    if (state->upDownChannelInfoList != NULL)
    {
        ListItem* listItem = state->upDownChannelInfoList->first;
        PhySatelliteRsvTxRxChannelInfo* upDownChannelInfoPtr = NULL;

        while (listItem)
        {
            upDownChannelInfoPtr = (PhySatelliteRsvTxRxChannelInfo*)
                listItem->data;

            if (upDownChannelInfoPtr->txChannelInfo->channelIndex ==
                    channelId)
            {
                return upDownChannelInfoPtr->txChannelInfo;
            }

            listItem = listItem->next;
        }
    }

    return NULL;
}

// /**
// FUNCTION :: PhySatelliteRsvGetLinkChannelInfo
// LAYER :: PHY
// PURPOSE :: This function returns ChannelInfo for channel index channelId.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine.
// +        phyIndex : int : The logical index of the current interface
// +        channelId : int : channel index of required channel info.
// RETURN :: PhySatelliteRsvChannel* : This function returns channelInfo for
//          channel index channelId.
// **/

PhySatelliteRsvChannel*
PhySatelliteRsvGetLinkChannelInfo(
    Node* node,
    int phyIndex,
    int channelId)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                      phyIndex);

    if (state->upDownChannelInfoList != NULL)
    {
        ListItem* listItem = state->upDownChannelInfoList->first;
        PhySatelliteRsvTxRxChannelInfo* upDownChannelInfoPtr = NULL;

        while (listItem)
        {
            upDownChannelInfoPtr = (PhySatelliteRsvTxRxChannelInfo*)
                listItem->data;

            if (upDownChannelInfoPtr->rxChannelInfo->channelIndex ==
                    channelId)
            {
                return upDownChannelInfoPtr->rxChannelInfo;
            }
            else if (upDownChannelInfoPtr->txChannelInfo->channelIndex ==
                    channelId)
            {
                return upDownChannelInfoPtr->txChannelInfo;
            }

            listItem = listItem->next;
        }
    }

    ERROR_Assert(0, "channel index neither up-link nor down-link");

    return NULL;
}

// /**
// FUNCTION :: PhySatelliteRsvGetLinkChannelPairNum
// LAYER :: PHY
// PURPOSE :: This function returns number of rx-tx pair in the linked list.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// RETURN :: int : This function returns number of rx-tx pair in
//          the linked list.
// **/

int
PhySatelliteRsvGetLinkChannelPairNum(
    Node* node,
    int phyIndex)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                      phyIndex);
    int channelNum = 0;
    if (state->upDownChannelInfoList != NULL)
    {
        ListItem* listItem = state->upDownChannelInfoList->first;
        PhySatelliteRsvTxRxChannelInfo* upDownChannelInfoPtr = NULL;

        while (listItem)
        {
            channelNum ++;
            listItem = listItem->next;
        }
    }

    return channelNum;
}

// /**
// FUNCTION :: PhySatelliteRsvGetLinkChannelPairInfoByIndex
// LAYER :: PHY
// PURPOSE :: This function returns TxRxChannelInfoPair by index
//          from the linked list.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// +        index : int : index of rx-tx channel pair in the linked list.
// RETURN :: PhySatelliteRsvTxRxChannelInfo* : This function returns
//          TxRxChannelInfoPair by index from the linked list.
// **/

PhySatelliteRsvTxRxChannelInfo*
PhySatelliteRsvGetLinkChannelPairInfoByIndex(
    Node* node,
    int phyIndex,
    int index)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                      phyIndex);
    int channelNum = 0;
    if (state->upDownChannelInfoList != NULL)
    {
        ListItem* listItem = state->upDownChannelInfoList->first;
        PhySatelliteRsvTxRxChannelInfo* upDownChannelInfoPtr = NULL;

        while (listItem)
        {
            upDownChannelInfoPtr = (PhySatelliteRsvTxRxChannelInfo*)
                listItem->data;

            if (channelNum == index)
            {
                return upDownChannelInfoPtr;
            }
            channelNum ++;

            listItem = listItem->next;
        }
    }

    return NULL;
}


// /**
// FUNCTION :: PhySatelliteRsvInterferenceArrivalFromChannel
// LAYER :: PHY
// PURPOSE :: This function implements the algorithm to respond to a
//            interference signal arriving from the channel.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// +        channelIndex : int :  The logical index of the channel upon
//          which the channel is arriving.
// +        propRxInfo : PropRxInfo* : A pointer to a data structure
//          containing the propagation reception information at the
//          receiver.  Note this may contain data that would be
//          unavailable to a physically realizable implementation of a
//          receiver.
// +        sigPowermW : double      : the interference signal power in mW
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvInterferenceArrivalFromChannel(Node* node,
                                             int phyIndex,
                                             int channelIndex,
                                             PropRxInfo* propRxInfo,
                                             double sigPowermW)
{

    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);
    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                 phyIndex,
                                 channelIndex);

    double rxdb = ANTENNA_GainForThisSignal(node,
                                            phyIndex,
                                            propRxInfo);

    double rxPowermW = NON_DB(rxdb + IN_DB(sigPowermW));

    switch(PhySatelliteRsvGetMode(node, phyIndex, channelIndex)) {
        case PHY_RECEIVING: {
            if (channel->rxMsgError == FALSE) {
                PhySatelliteRsvHeader phyhdrs;
                PhySatelliteRsvHeader* phyhdr = &phyhdrs;

                memcpy(phyhdr,
                       MESSAGE_ReturnPacket(propRxInfo->txMsg),
                       sizeof(PhySatelliteRsvHeader));
                BOOL isheader = FALSE;
                channel->rxMsgError = PhySatelliteRsvCheckRxPacketError(
                                                              node,
                                                              state,
                                                              phyhdr,
                                                              0,
                                                              phyIndex,
                                                              channelIndex,
                                                              isheader,
                                                              NULL);
            }

            channel->rxTimeEvaluated = node->getNodeTime();
            channel->interferencePowermW += rxPowermW;

            break;
        }
        case PHY_IDLE:
        case PHY_SENSING: {

            channel->interferencePowermW += rxPowermW;

            if (PhySatelliteRsvIsSensingCarrier(node,
                                            phyIndex,
                                            channelIndex) == TRUE) {
                PhySatelliteRsvSetMode(node,
                                       phyIndex,
                                       channelIndex,
                                       PHY_SENSING);
            }
            else {
                PhySatelliteRsvSetMode(node,
                                       phyIndex,
                                       channelIndex,
                                       PHY_IDLE);
            }

            break;
        }
        default:
            ERROR_ReportError("Unsupport PHY Interrupt Type.");
    }
}

// /**
// FUNCTION :: PhySatelliteRsvInterferenceEndFromChannel
// LAYER :: PHY
// PURPOSE :: This function implements the model logic to respond to
//            a intereference end event.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySatelliteRsvState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyNum : int : The logical index of the current interface
// +        msg : Message* : A pointer to a data structure representing the
//          frame to be sent on the wireless satellite channel.
// +        channelIndex : int : The logical channel index the channel
//          that is receiving the frame.
// +        propRxInfo : PropRxInfo* : A pointer to a data structure
//          containing the local receiver environment for frame
//          reception.
// +        sigPowermW : double : the interference signal power in mW
// RETURN :: void : This function does not return a value.
// **/

void PhySatelliteRsvInterferenceEndFromChannel(Node *node,
                                         int phyIndex,
                                         int channelIndex,
                                         PropRxInfo *propRxInfo,
                                         double sigPower)
{
    PhySatelliteRsvState* state = PhySatelliteRsvGetState(node,
                                                          phyIndex);
     double sinr = -1.0;

    double interferencePower_mW =
            NON_DB(ANTENNA_GainForThisSignal(node, phyIndex, propRxInfo) +
                   IN_DB(sigPower));

    PhySatelliteRsvChannel* channel =
        PhySatelliteRsvGetLinkChannelInfo(node,
                                 phyIndex,
                                 channelIndex);

    if (PhySatelliteRsvGetMode(node, phyIndex, channelIndex) == PHY_RECEIVING) {
        if (channel->rxMsgError == FALSE) {
            PhySatelliteRsvHeader phyhdrs;
            PhySatelliteRsvHeader* phyhdr = &phyhdrs;

            memcpy(phyhdr,
                   MESSAGE_ReturnPacket(propRxInfo->txMsg),
                   sizeof(PhySatelliteRsvHeader));
            BOOL isheader = FALSE;
            channel->rxMsgError = PhySatelliteRsvCheckRxPacketError(
                                                                node,
                                                                state,
                                                                phyhdr,
                                                                &sinr,
                                                                phyIndex,
                                                                channelIndex,
                                                                isheader,
                                                                NULL);
            channel->rxTimeEvaluated = node->getNodeTime();

        }
    }

    channel->interferencePowermW -= interferencePower_mW;

    if (channel->interferencePowermW < 0.0) {
        channel->interferencePowermW = 0.0;
    }

    PhyStatusType newMode;

    if (PhySatelliteRsvGetMode(node,
                               phyIndex,
                               channelIndex) != PHY_RECEIVING) {
        newMode = PHY_SENSING;
    }
    else {
        newMode = PHY_IDLE;
    }

    if (newMode != PhySatelliteRsvGetMode(node, phyIndex, channelIndex)) {
        PhySatelliteRsvSetMode(node,
                               phyIndex,
                               channelIndex,
                               newMode);
    }
}
