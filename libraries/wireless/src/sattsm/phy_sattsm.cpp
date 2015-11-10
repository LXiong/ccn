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
// PROTOCOL :: PHY_SATTSM
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

#include "phy_sattsm.h"


///
/// @file phy_sattsm.cpp
///
/// @brief Interface to phy.cpp for SATTSM PHY
///

///
/// @def DEBUG
///
/// @brief Debugging macro for this file
///

#ifdef DEBUG
# undef DEBUG
#endif

#define DEBUG 0

using namespace PhySattsm;

// /**
// FUNCTION :: PhySattsmGetData
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
PhyData* PhySattsmGetData(Node* node,
                          int phyNum)
{
    return (PhyData*)(node->phyData[phyNum]);
}

// /**
// FUNCTION :: PhySattsmGetState
// LAYER :: PHY
// PURPOSE :: This routine returns the data structure associated with
//            the satellite RSV PHY process for the provided <node, phyNum>
//            pair.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyNum : int : The integer index of the interface associated with
//          this physical layer process.
// RETURN :: PhySattsmState* : A pointer to the specific data structure
//           for the satellite physical layer for this <node,phyNum> pair.
// **/

static
State* PhySattsmGetState(Node* node,
                         int phyNum)
{
    PhyData* phyData = PhySattsmGetData(node,
                                        phyNum);

    return (State*)phyData->phyVar;
}

// /**
// FUNCTION :: PhySattsmSetState
// LAYER :: PHY
// PURPOSE :: This routine sets the data structure to be associated with
//            the satellite RSV PHY process for the provided
//            <node, phyNum>
//            pair.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyNum : int : The integer index of the interface associated
//          with
//          this physical layer process.
// +        state : PhySattsmState* : A pointer to the data structure
//          to be associated with this physical layer state machine.
// RETURN :: void : This routine does not return a value.
// **/

static
void PhySattsmSetState(Node *node,
                       int phyNum,
                       State* state)
{
    PhyData* phyData = PhySattsmGetData(node,
                                        phyNum);

    phyData->phyVar = (void*)state;
}

// /**
// FUNCTION :: PhySattsmReportExtendedStatusToMac
// LAYER :: PHY
// PURPOSE :: This routine reports additional status to the MAC
//            process including a copy of the packet from the
//            MAC layer above if available.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface
//          associated with
//          this physical layer process.
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

void
PhySattsmReportExtendedStatusToMac(Node* node,
                                   int  phyIndex,
                                   PhyStatusType status,
                                   clocktype receiveDuration,
                                   Message* potentialIncomingPacket)
{
    // This function provides a lookahead into the incoming
    // packet before an equivalent physical system would have such
    // information.  As such, it is to be used with some caution when
    // developing algorithms and other numerical calculations that are
    // intended to be directly implemented in a physically-realizable
    // system.

    PhyData* phyData = PhySattsmGetData(node,
                                        phyIndex);

    State* state = PhySattsmGetState(node,
                                     phyIndex);


    // When delivering the packet for review by the MAC layer, the
    // existing PHY header must be stripped.  Since this packet is
    // shared among multiple processes, the header must be replaced
    // after the review is done.  Note that it is not necessary to
    // recopy the header information or save it locally in the process.
    // Rather, the header stays in the packet and the indices that
    // control the header queue merely are manipulated for improved
    // computational efficiency.

    if (potentialIncomingPacket != NULL)
    {
        MESSAGE_RemoveHeader(node,
                             potentialIncomingPacket,
                             sizeof(PhySattsm::AnalogHeader),
                             TRACE_SATTSM);

        MESSAGE_RemoveHeader(node,
                             potentialIncomingPacket,
                             sizeof(PhySattsm::AnalogHeader),
                             TRACE_SATTSM);
    }

    if (potentialIncomingPacket != NULL && DEBUG)
    {
        printf("PhySattsmReportExtendedStatusToMac()\n");
    }

    MAC_ReceivePhyStatusChangeNotification(node,
                                           phyData->macInterfaceIndex,
                                           state->mode.getPhyStatus(),
                                           status,
                                           receiveDuration,
                                           potentialIncomingPacket);

    if (potentialIncomingPacket != NULL)
    {
        MESSAGE_AddHeader(node,
                          potentialIncomingPacket,
                          sizeof(PhySattsm::DigitalHeader),
                          TRACE_SATTSM);

        MESSAGE_AddHeader(node,
                          potentialIncomingPacket,
                          sizeof(PhySattsm::AnalogHeader),
                          TRACE_SATTSM);
    }
}

// /**
// FUNCTION :: PhySattsmReportStatusToMac
// LAYER :: PHY
// PURPOSE :: This routine reports general status to the MAC
//            process, notably the change in the PhyStatus.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface associated with
//          this physical layer process.
// +        status : PhyStatusType : The new status of the physical
//          interface.
// RETURN :: void : This routine does not return a value.
// **/

void PhySattsmReportStatusToMac(Node* node,
                                int phyIndex,
                                Status& status)
{
    //
    // The standard status is just a change in the mode.
    //

    PhySattsmReportExtendedStatusToMac(node,
                                       phyIndex,
                                       status.getPhyStatus(),
                                       0,
                                       0);
}

// /**
// CONSTANT :: BITS_PER_BYTE : 8
// DESCRIPTION :: The number of bits in an byte of data
// **/

#define BITS_PER_BYTE (8)

// /**
// FUNCTION :: PhySattsmInitializeConfigurableParameters
// LAYER :: PHY
// PURPOSE :: This routine initializes the configurable parameters of the
//            physical layer process.  This includes data obtained from
//            queries to the simulation configuration database.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface
//          associated with
//          this physical layer process.
// +        nodeInput : NodeInput* : A pointer to the simulation
//          configuration
//          data store that has been deserialized from the simulation
//          configuration file and any auxillary configuration files
//          specified within that file.
// RETURN :: void : This routine does not return a value.
// **/

static void
PhySattsmInitializeConfigurableParameters(Node* node,
                                          int phyIndex,
                                          Address* networkAddr,
                                          const NodeInput* nodeInput)
{
    State* state = PhySattsmGetState(node,
                                     phyIndex);


    state->p_chain = (PhyEntity*) new GroundPhy(node,
                                                phyIndex,
                                                networkAddr,
                                                nodeInput);

}

// /**
// FUNCTION :: PhySattsmResetStatistics
// LAYER :: PHY
// PURPOSE :: This routine resets the run-time statistic counters to
//            a known (zero) state.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : The integer index of the interface
//          associated with this physical layer process.
// RETURN :: void : This routine does not return a value.
// **/

static
void PhySattsmResetStatistics(Node* node,
                              int phyIndex)
{
    State* state = PhySattsmGetState(node,
                                                phyIndex);
    Statistics* stats = &state->stats;

    stats->totalTxSignals = 0;
    stats->totalRxSignalsToMac = 0;
    stats->totalSignalsLocked = 0;
    stats->totalSignalsWithErrors = 0;
}

// /**
// FUNCTION :: PhySattsmInit
// LAYER :: PHY
// PURPOSE :: This function initializes the Satellite RSV PHY process.  It is
//            generally called at the beginning of the simulation at time
//            t = 0 but may be called at other times under dynamic node
//            creation conditions.  This function is called by other functions
//            outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySattsmState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// +        nodeInput : NodeInput* : A pointer to the deserialized data
//          structure corresponding to the current simulation configuration
//          file and and any auxillary files references by the simulation
//          configuration file
// RETURN :: void : This function does not return a value.
// **/

void PhySattsmInit(Node* node,
                   int phyIndex,
                   const NodeInput* nodeInput)
{

    PhySattsm::State* state = new State();

    PhySattsmSetState(node,
                      phyIndex,
                      state);

    state->thisPhy = PhySattsmGetData(node,
                                      phyIndex);

    ANTENNA_Init(node,
                 phyIndex,
                 nodeInput);

    ERROR_Assert(state->thisPhy->antennaData->antennaModelType
                 == ANTENNA_OMNIDIRECTIONAL,
                 "Model only supports omnidirectional antennas so"
                 " that advanced processing effects can be"
                 " computed by the physical layer processing"
                 " chain.");

    Address* networkAddr = node->phyData[phyIndex]->networkAddress;

    PhySattsmInitializeConfigurableParameters(node,
                                              phyIndex,
                                              networkAddr,
                                              nodeInput);

    PhySattsmResetStatistics(node,
                             phyIndex);


    int numChannels = PROP_NumberChannels(node);

    if (DEBUG)
    {
        printf("Node[%d/%d]: numChannels = %d.\n",
               node->nodeId,
               phyIndex,
               numChannels);
    }

    int uplinkChannel;
    BOOL wasFound;

    IO_ReadInt(node->nodeId,
               phyIndex,
               nodeInput,
               "PHY-SATTSM-UPLINK-CHANNEL",
               &wasFound,
               &uplinkChannel);

    ERROR_Assert(wasFound == TRUE,
                 "This PHY is a full-duplex physical layer."
                 " Therefore, the transmit and receive channel"
                 " must be explicitly identified.");

    int downlinkChannel;
    IO_ReadInt(node->nodeId,
               phyIndex,
               nodeInput,
               "PHY-SATTSM-DOWNLINK-CHANNEL",
               &wasFound,
               &downlinkChannel);

    ERROR_Assert(wasFound == TRUE,
                 "This PHY is a full-duplex physical layer."
                 " Therefore, the transmit and receive channel"
                 " must be explicitly identified.");


    ERROR_Assert(uplinkChannel >= 0 &&
                 downlinkChannel >= 0 &&
                 uplinkChannel < numChannels &&
                 downlinkChannel < numChannels,
                 "The transceiver channel mapping is not"
                 " consistent in the configuration file.  Please"
                 " verify the configuration file to ensure that the"
                 " receive and transmit channel numbers are"
                 " valid channel identifiers.");

    ERROR_Assert(state->thisPhy->channelListenable[uplinkChannel] ==
                 TRUE,
                 "The transmission channel must be listenable to"
                 " support this full-duplex PHY");

    ERROR_Assert(state->thisPhy->channelListenable[downlinkChannel] ==
                 TRUE,
                 "The receiver channel must be listenable to"
                 " support this full-duplex PHY");

    PHY_SetTransmissionChannel(node,
                               phyIndex,
                               uplinkChannel);

    if (state->thisPhy->channelListenable[uplinkChannel] == TRUE)
    {
        PHY_StopListeningToChannel(node,
                                   phyIndex,
                                   uplinkChannel);
    }

    if (state->thisPhy->channelListenable[downlinkChannel] == TRUE)
    {
        PHY_StartListeningToChannel(node,
                                   phyIndex,
                                   downlinkChannel);
    }

    state->statsWritten = FALSE;
}

// /**
// FUNCTION :: PhySattsmFinalize
// LAYER :: PHY
// PURPOSE :: This function finalizes the Satellite RSV PHY process.  It is
//            generally called at the end of the simulation at the epoch but
//            may be called at other times under dynamic node
//            destruction conditions.  This function is called by other functions
//            outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySattsmState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyIndex : int : The logical index of the current interface
// RETURN :: void : This function does not return a value.
// **/

void PhySattsmFinalize(Node* node,
                       int phyNum)
{
    char buf[BUFSIZ];
    State* state = PhySattsmGetState(node,
                                     phyNum);

    if (state->statsWritten == TRUE)
    {
        return;
    }
    state->statsWritten = TRUE;

    sprintf(buf,
            "Signals transmitted = %d",
            state->stats.totalTxSignals);

    IO_PrintStat(node,
                 "Physical",
                 "SATTSM",
                 ANY_DEST,
                 phyNum,
                 buf);

    sprintf(buf,
            "Signals received and forwarded to MAC = %d",
            state->stats.totalRxSignalsToMac);
    IO_PrintStat(node,
                 "Physical",
                 "SATTSM",
                 ANY_DEST,
                 phyNum,
                 buf);

    sprintf(buf,
            "Signals locked on by PHY = %d",
            state->stats.totalSignalsLocked);

    IO_PrintStat(node,
                 "Physical",
                 "SATTSM",
                 ANY_DEST,
                 phyNum,
                 buf);

    sprintf(buf,
            "Signals received but with errors = %d",
            state->stats.totalSignalsWithErrors);
    IO_PrintStat(node,
                 "Physical",
                 "SATTSM",
                 ANY_DEST,
                 phyNum,
                 buf);

    state->mode.reset();
}

// /**
// FUNCTION :: PhySattsmGetStatus
// LAYER :: PHY
// PURPOSE :: This function returns the current transceiver status
//            for the
//            phyisical layer process. This function is called by other
//            functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySattsmState* : A pointer to the
//          physical layer state machine state data for the current
//          state
//          machine
// +        phyIndex : int : The logical index of the current interface
// RETURN :: PhyStatusType : This function returns the instantaneous
//           sample
//           of the mode of the physical layer transceiver.
// **/

PhyStatusType PhySattsmGetStatus(Node* node,
                                 int phyNum)
{
    State* state = PhySattsmGetState(node,
                                     phyNum);

    return state->mode.getPhyStatus();
}

// /**
// FUNCTION :: PhySattsmTransmissionEnd
// LAYER :: PHY
// PURPOSE :: This function implements the model logic to be executed
//            when
//            the system has completed transmitting a packet.
// PARAMETERS ::
// +        node : Node* : state : PhySattsmState* : A pointer to the
//          physical layer state machine state data for the current
//          state
//          machine
// +        phyIndex : int : The logical index of the current interface
// RETURN :: void : This function does not return a value.
// **/

void PhySattsmTransmissionEnd(Node* node,
                              int phyIndex)
{
    State* state = PhySattsmGetState(node,
                                     phyIndex);

    PhyData* phyData = state->thisPhy;
    int channelIndex = -1;
    NodeAddress address = 0;

    if (DEBUG) {
        printf("%0.6f: PhySattsmTransmissionEnd(): Node[%d/%d]"
               " ceases transmission.\n",
               (double)node->getNodeTime() / (double)SECOND, node->nodeId,
               phyIndex);
    }

    ERROR_Assert(state->mode.isTransmitting(),
                 "State machine indicates station not transmitting yet"
                 " ceasing transmission. Logic error.");

    PHY_GetTransmissionChannel(node,
                               phyIndex,
                               &channelIndex);

    if (node->guiOption == TRUE)
    {
        switch(phyData->networkAddress->networkType)
        {
            case NETWORK_IPV4:
                address = phyData->networkAddress->interfaceAddr.ipv4;
                break;

            case NETWORK_IPV6:
                address
                    = phyData->networkAddress->interfaceAddr.ipv4
                    & 0xff;
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

    if (ANTENNA_IsLocked(node, phyIndex) == 0)
    {
        ANTENNA_SetToDefaultMode(node,
                                 phyIndex);
    }

    /*
     // This is a full duplex PHY now.  Therefore listening
     // does not have to cease when transmitting.

    PHY_StartListeningToChannel(node,
                                phyIndex,
                                channelIndex);

    // PHY_SignalInterference is called whenever the state
    // machine has to update the local interference level.

    PHY_SignalInterference(node,
                           phyIndex,
                           channelIndex,
                           0,
                           0,
                           &(state->interferencePowermW));
     */

    state->mode.transmissionEnd(node,
                                phyIndex);

}

// /**
// FUNCTION :: PhySattsmStartTransmittingSignal
// LAYER :: PHY
// PURPOSE :: This function implements the algorithm to transmit
//            a signal
//            on the satellite subnet wireless channel.  This function
//            is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySattsmState* : A pointer to the
//          physical layer state machine state data for the current state
//          machine
// +        phyNum : int : The logical index of the current interface
// +        msg : Message* : A pointer to a data structure representing
//          the
//          frame to be sent on the wireless satellite channel.
// +        useMacLayerSpecifiedDelay : BOOL : An indicator to use the
//          standard MAC-specified delay value
// +        delayUntilAirborne : clocktype : The number of nanoseconds
//          until
//          the delay reaches the transmitter.
// RETURN :: void : This function does not return a value.
// **/

void PhySattsmStartTransmittingSignal(Node* node,
                                      int phyNum,
                                      Message* msg,
                                      BOOL useMacLayerSpecifiedDelay,
                                      clocktype delayUntilAirborne)
{
    State* state = PhySattsmGetState(node,
                                     phyNum);

    clocktype now = node->getNodeTime();
    int channelIndex = -1;

    clocktype duration = 0;

    Message* endMsg =  NULL;

    if (DEBUG)
    {
        printf("%0.6f: PhySattsmStartTransmittingSignal[%d/%d] "
               "Start transmission of packet %p/\n",
               (double)now / (double)SECOND,
               node->nodeId,
               phyNum,
               msg);
    }

    PHY_GetTransmissionChannel(node,
                               phyNum,
                               &channelIndex);

    ERROR_Assert (!state->mode.isTransmitting(),
                  "PHY cannot begin transmission until previous "
                  "transmission completes.");

    // When this station starts to transmit, all other sources of
    // signal energy are considered to be interference.

    state->mode.transmissionStart(node, phyNum);
    bool ok(true);

    state->p_chain->onSend(node,
                           phyNum,
                           msg,
                           ok);

    ERROR_Assert(ok,
                 "Presently this models assumes that the transmission"
                 " chain is always successful.  At run-time the"
                 " logic reported this is not the case.  The program"
                 " logic has to be reviewed for potential errors.  The"
                 " simulation cannot continue.");

    PhySattsm::AnalogHeader ah(msg);


    duration = ah.transmissionEndTime - ah.transmissionStartTime;

    double totalPower_mW = (ah.signalPower +
                            ah.noisePower +
                            ah.interferencePower) * 1000.0;

    state->mode.transmissionStart(node,
                                  phyNum);

    PROP_ReleaseSignal(node,
                       msg,
                       phyNum,
                       channelIndex,
                       (float)IN_DB(totalPower_mW),
                       duration,
                       0);

    if (node->guiOption == TRUE)
    {
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

    MESSAGE_SetInstanceId(endMsg,
                          (short)phyNum);
    MESSAGE_Send(node,
                 endMsg,
                 duration + 1);

    state->stats.totalTxSignals++;
}

// /**
// FUNCTION :: PhySattsmSignalArrivalFromChannel
// LAYER :: PHY
// PURPOSE :: This function implements the algorithm to respond to a
//            signal arriving from the channel.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySattsmState* : A pointer to the
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

void PhySattsmSignalArrivalFromChannel(Node* node,
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

    State* state = PhySattsmGetState(node,
                                     phyIndex);
    clocktype now = node->getNodeTime();

    state->mode.receptionStart(node,
                               phyIndex);

    if (DEBUG)
    {
        printf("%0.6f: Node[%d/%d] signal arrival from channel"
               " detected.\n",
               (double)now * 1.0e-9,
               node->nodeId,
               phyIndex);
    }

    state->p_signalList.start(propRxInfo);

    state->activeSignalCount++;
}

// /**
// FUNCTION :: PhySattsmSignalEndFromChannel
// LAYER :: PHY
// PURPOSE :: This function implements the model logic to respond to
//            a completed reception of a packet.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        node : Node* : state : PhySattsmState* : A pointer to the
//          physical layer state machine state data for the current
//          state
//          machine
// +        phyNum : int : The logical index of the current interface
// +        msg : Message* : A pointer to a data structure representing
//          the
//          frame to be sent on the wireless satellite channel.
// +        channelIndex : int : The logical channel index the channel
//          that is receiving the frame.
// +        propRxInfo : PropRxInfo* : A pointer to a data structure
//          containing the local receiver environment for frame
//          reception.
// RETURN :: void : This function does not return a value.
// **/

void PhySattsmSignalEndFromChannel(Node *node,
                                   int phyIndex,
                                   int channelIndex,
                                   PropRxInfo *propRxInfo)
{
    State* state = PhySattsmGetState(node,
                                     phyIndex);

    state->mode.receptionEnd(node,
                             phyIndex);
    state->activeSignalCount--;

    clocktype now = node->getNodeTime();

    if (DEBUG)
    {
        printf("%0.6f: Node[%d/%d] signal end from"
               " channel detected.\n",
               (double)now * 1.0e-9,
               node->nodeId,
               phyIndex);
    }

    SignalRecord srec = state->p_signalList.end(propRxInfo);
    bool ok(false);

    srec.m_msg = MESSAGE_Duplicate(node,
                                   propRxInfo->txMsg);

    state->p_chain->onReceive(node,
                              phyIndex,
                              srec.m_msg,
                              ok,
                              srec);



    if (ok)
    {
        MAC_ReceivePacketFromPhy(node,
                                 node->phyData[phyIndex]->macInterfaceIndex,
                                 srec.m_msg);

        state->stats.totalRxSignalsToMac++;
    }
    else
    {
        state->stats.totalSignalsWithErrors++;
        MESSAGE_Free(node, srec.m_msg);
    }
}

// /**
// FUNCTION :: PhySattsmGetTxDataRate
// LAYER :: PHY
// PURPOSE :: This function calculates the current transmission rate
//            of the currently selected transmission channel.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: int : The current transmission rate expressed as bits/sec
// **/

int PhySattsmGetTxDataRate(PhyData* phyData)
{
    State* state = (State*)phyData->phyVar;
    ManagedEntity* lme = dynamic_cast<ManagedEntity*> (state->p_chain);

    return (int)lme->getTransmitRate();
}

// /**
// FUNCTION :: PhySattsmGetRxDataRate
// LAYER :: PHY
// PURPOSE :: This function calculates the current signaling rate
//            of the packet currently being received.  This function is
//            called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: int : The current reception rate expressed as bits/sec
// **/

int PhySattsmGetRxDataRate(PhyData* phyData)
{
    State* state = (State*)phyData->phyVar;
    ManagedEntity* lme = dynamic_cast<ManagedEntity*> (state->p_chain);

    return (int) lme->getReceiveRate();
}

// /**
// FUNCTION :: PhySattsmSetTxDataRateType
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

void PhySattsmSetTxDataRateType(PhyData* thisPhy,
                                      int dataRateType)
{
    ERROR_ReportError("The data rate requested by MAC is not supported "
                      "by this physical layer model.");
}

// /**
// FUNCTION :: PhySattsmGetRxDataRateType
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

int PhySattsmGetRxDataRateType(PhyData* thisPhy)
{
    return 0;
}

// /**
// FUNCTION :: PhySattsmGetTxDataRateType
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

int PhySattsmGetTxDataRateType(PhyData* thisPhy)
{
    return 0;
}

// /**
// FUNCTION :: PhySattsmSetLowestTxDataRateType
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

void PhySattsmSetLowestTxDataRateType(PhyData* thisPhy)
{
    ERROR_ReportError("The data rate requested by MAC is not supported "
                      "by this physical layer model.");
}

// /**
// FUNCTION :: PhySattsmGetLowestTxDataRateType
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

void PhySattsmGetLowestTxDataRateType(PhyData* thisPhy,
                                            int* dataRateType)
{
    ERROR_ReportError("The data rate requested by MAC is not supported "
                      "by this physical layer model.");
}

// /**
// FUNCTION :: PhySattsmSetHighestTxDataRateType
// LAYER :: PHY
// PURPOSE :: This function sets the transmission rate of the local
//            physical layer process to its maximum rate possible.  This
//            function is called by other functions outside of this file.
// PARAMETERS ::
// +        phyData : PhyData* : A pointer to the generic PHY state
//          data for the current interface/node.
// RETURN :: void : NULL
// **/

void PhySattsmSetHighestTxDataRateType(PhyData* thisPhy)
{
    ERROR_ReportError("The data rate requested by MAC is not supported "
                      "by this physical layer model.");
}

// /**
// FUNCTION :: PhySattsmGetHighestTxDataRateType
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

void PhySattsmGetHighestTxDataRateType(PhyData* thisPhy,
                                          int* dataRateType)
{
    ERROR_ReportError("The data rate requested by MAC is not supported "
                      "by this physical layer model.");
}

// /**
// FUNCTION :: PhySattsmSetHighestTxDataRateTypeForBC
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

void PhySattsmSetHighestTxDataRateTypeForBC(PhyData* thisPhy)
{
    ERROR_ReportError("The data rate requested by MAC is not supported "
                      "by this physical layer model.");
}

// /**
// FUNCTION :: PhySattsmGetHighestTxDataRateTypeForBC
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

void PhySattsmGetHighestTxDataRateTypeForBC(PhyData* thisPhy,
                                             int* dataRateType)
{
    ERROR_ReportError("The data rate requested by MAC is not supported "
                      "by this physical layer model.");
}

// /**
// FUNCTION :: PhySattsmGetTransmissionDuration
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

clocktype PhySattsmGetTransmissionDuration(PhyData* phyData,
                                           int size,
                                           int dataRate)
{
    State* state = (State*)phyData->phyVar;
    ManagedEntity* lme = dynamic_cast<ManagedEntity*> (state->p_chain);

    clocktype duration = lme->getTransmissionDuration(size);

    return duration;
}

// /**
// FUNCTION :: PhySattsmSetTransmitPower
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

void PhySattsmSetTransmitPower(PhyData* phyData,
                               double newTxPower_mW)
{
    State* state = (State*)phyData->phyVar;
    ManagedEntity* lme = dynamic_cast<ManagedEntity*> (state->p_chain);

    lme->setTransmitPower(newTxPower_mW);
}

// /**
// FUNCTION :: PhySattsmGetTransmitPower
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

void PhySattsmGetTransmitPower(PhyData* phyData, double* txPower_mW)
{
    State* state = (State*)phyData->phyVar;
    ManagedEntity* lme = dynamic_cast<ManagedEntity*> (state->p_chain);

    double transmitPower_mW = lme->getTransmitPower();

    if (txPower_mW != NULL)
    {
        *txPower_mW = transmitPower_mW;
    }
}

// /**
// FUNCTION :: PhySattsmGetLastAngleOfArrival
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

double PhySattsmGetLastAngleOfArrival(Node* node,
                                            int phyNum)
{

    ERROR_ReportError("This routine is not supported by the"
                      " current generation SATTSM PHY. The"
                      " antenna capability is subsumed into"
                      " the transceiver processing chain.");

    /* NOTREACHED */
    return 0.0;
}

// /**
// FUNCTION :: PhySattsmTerminateCurrentReceive
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
PhySattsmTerminateCurrentReceive(Node* node,
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

    ERROR_ReportError("Transmission truncation not presently"
                      " supported.\n");
}

// /**
// FUNCTION :: PhySattsmStartTransmittingSignalDirectionally
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
PhySattsmStartTransmittingSignalDirectionally(Node* node,
                                              int phyNum,
                                              Message* msg,
                                              BOOL useMacLayerSpecifiedDelay,
                                              clocktype delayUntilAirborne,
                                              double azimuthAngle)
{
    ERROR_ReportError("Directional antennas are not supported"
                      " in this version of the SATTSM PHY.\n");
}

// /**
// FUNCTION :: PhySattsmLockAntennaDirection
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

void PhySattsmLockAntennaDirection(Node* node,
                                         int phyNum)
{
    ERROR_ReportError("This routine is not supported by the"
                      " current generation SATTSM PHY. The"
                      " antenna capability is subsumed into"
                      " the transceiver processing chain.");
}

// /**
// FUNCTION :: PhySattsmUnlockAntennaDirection
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

void PhySattsmUnlockAntennaDirection(Node* node,
                                     int phyNum)
{
    ERROR_ReportError("This routine is not supported by the"
                      " current generation SATTSM PHY. The"
                      " antenna capability is subsumed into"
                      " the transceiver processing chain.");
}

// /**
// FUNCTION :: PhySattsmMediumIsIdleInDirection
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

BOOL PhySattsmMediumIsIdleInDirection(Node* node,
                                      int phyNum,
                                      double azimuth)
{
    ERROR_ReportError("This routine is not supported by the"
                      " current generation SATTSM PHY. The"
                      " antenna capability is subsumed into"
                      " the transceiver processing chain.");

    return FALSE;

}

// /**
// FUNCTION :: PhySattsmSetSensingDirection
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

void PhySattsmSetSensingDirection(Node* node,
                                  int phyNum,
                                  double azimuth)
{
    ERROR_ReportError("This routine is not supported by the"
                      " current generation SATTSM PHY. The"
                      " antenna capability is subsumed into"
                      " the transceiver processing chain.");}
