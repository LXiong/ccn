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

#include "sattsm.h"

#include "analog.h"
#include "antenna_parabolic.h"
#include "digital.h"
#include "lna.h"
#include "mixer.h"
#include "sspa.h"
#include "twta.h"

// /**
// PROTOCOL :: PHY-SATTSM
// SUMMARY :: An implementation of a modular physical layer with 
//            analog and digital digital communications effects.  
// LAYER :: PHY
// STATISTICS :: 
//+              Signals transmitted : The number of signals transmitted 
//               by this physical layer process.  
//+              Signals received and forwarded to MAC : The number of 
//               signals received by this physical layer process and 
//               subsequently forward to the MAC layer for further
//               processing.
//+              Signals locked on by PHY : The number of signals that
//               triggered logic to lock the transceiver onto an
//               incoming signal.
//+              Signals received but with errors : The number of signals
//               that were successfully received by the MAC but had 
//               errors due to interference or noise corruption. 
// APP_PARAM ::
// CONFIG_PARAM ::
// **/

#ifndef PHY_SATTSM_H
#define PHY_SATTSM_H

// Forward reference outside of namespaces
void PhySattsmReportExtendedStatusToMac(Node* node,
                                        int phyIndex,
                                        PhyStatusType status,
                                        clocktype receiveDuration,
                                        Message* incomingPacket);
///
/// @file phy_sattsm.h
///
/// @brief The class header definition file for the PHY SATTSM module
///

///
/// @namespace PhySattsm
/// 
/// @brief The default PHY Sattsm namespace
///

namespace PhySattsm
{
    ///
    /// @class GroundPhy
    ///
    /// @brief A PHY of a ground system element
    ///
    
    class GroundPhy : public PhyEntity
    {
        /// The analog front end
        AnalogFrontEnd* afe;
        
        /// The parabolic antenna system
        AntennaParabolic* ap;
        
        /// The digital communications module
        DigitalBurstRsv* rsv;
        
        /// The low noise amplifier
        Lna* lna;
        
        /// A mixer
        Mixer* mixer;
        
        /// The solid state power amplifier
        Sspa* sspa;
        
    public:
            
        ///
        /// @brief Constructor based on QualNet configuration
        ///
        /// @param node a pointer to the local node data structre
        /// @param phyIndex a logical index of the local PHY interface
        /// @param nodeAddress a pointer to the address of the 
        /// local network interface
        /// @param nodeInput a pointer to the QualNet configuration
        /// information structure
        ///
            
        GroundPhy(Node* node,
                  int phyIndex,
                  Address* nodeAddress,
                  const NodeInput* nodeInput)
        {
            afe = new AnalogFrontEnd(node,
                                     phyIndex,
                                     nodeAddress,
                                     nodeInput);
            
            ap = new AntennaParabolic(node,
                                      phyIndex,
                                      nodeAddress,
                                      nodeInput);
            
            rsv = new DigitalBurstRsv(node,
                                      phyIndex,
                                      nodeAddress,
                                      nodeInput);
            
            lna = new Lna(node,
                          phyIndex,
                          nodeAddress,
                          nodeInput);
            
            mixer = new Mixer(node,
                              phyIndex,
                              nodeAddress,
                              nodeInput);
            
            sspa = new Sspa(node,
                            phyIndex,
                            nodeAddress,
                            nodeInput);
            
            
            push_tx(ap);
            push_tx(sspa);
            push_tx(mixer);
            push_tx(afe);
            push_tx(rsv);
            
            push_rx(ap);
            push_rx(lna);
            push_rx(mixer);
            push_rx(afe);
            push_rx(rsv);
        }
        
        ///
        /// @brief Calculate tranmission duration of abstract
        /// frame size provided
        ///
        /// @param sizeInBits Size of abstract frame in bits
        /// 
        /// @returns the total length of transmission in seconds
        ///
        
        clocktype getTransmissionDuration(int sizeInBits)
        {
            clocktype estimatedDuration =
                rsv->getTransmissionDuration(sizeInBits);
            
            return estimatedDuration;
        }
        
        ///
        /// @brief Requests the current tranmission chain to 
        /// change its power level
        ///
        /// @param power_mW specified power in mW
        ///
        
        void setTransmitPower(double power_mW)
        {
            double pwr = UTIL::abs2db(power_mW / 1000.0);
            
            sspa->setTransmitPower(pwr);
        }
        
        /// 
        /// @brief Requests the processing chain to provide the
        /// current output power
        ///
        /// @returns the current transmit power in mW
        ///
        
        double getTransmitPower()
        {
            double transmitPower = sspa->getTransmitPower();
            
            return 1000 * UTIL::db2abs(transmitPower);
        }
        
        ///
        /// @brief Requests the processing chain to return the
        /// current transmission rate
        ///
        /// @returns the current transmission rate in bits/second
        ///
        /// This represents the raw transmission rate of the modulated
        /// channel and does not account for user headers, coding,
        /// guard times, synchronization, etc.
        ///        
        
        double getTransmitRate()
        {
            return rsv->getTransmitRate();
        }
        
        ///
        /// @brief Requests the processing chain to return the 
        /// current receive rate
        ///
        /// 
        /// @returns the current transmission rate in bits/second
        ///
        /// This represents the raw reception rate of the modulated
        /// channel and does not account for user headers, coding,
        /// guard times, synchronization, etc.
        ///
        
        double getReceiveRate()
        {
            return rsv->getReceiveRate();
        }
        
    };
    
    ///
    /// @class SatellitePhy
    ///
    /// @brief A PHY of a satellite system element
    ///
    
    class SatellitePhy : public PhyEntity
    {
        
        /// The analog front end
        AnalogFrontEnd* afe;
        
        /// The parabolic antenna system
        AntennaParabolic* ap;
        
        /// The digital communications module
        DigitalBurstRsv* rsv;
        
        /// The low noise amplifier
        Lna* lna;
        
        /// A mixer
        Mixer* mixer;
        
        // Travelling Wave Tube Amplifier Model
        Twta* twta;
        
public:
        
        ///
        /// @brief Constructor based on QualNet configuration
        ///
        /// @param node a pointer to the local node data structre
        /// @param phyIndex a logical index of the local PHY interface
        /// @param nodeAddress a pointer to the address of the 
        /// local network interface
        /// @param nodeInput a pointer to the QualNet configuration
        /// information structure
        ///
            
        SatellitePhy(Node* node,
                     int phyIndex,
                     Address* nodeAddress,
                     const NodeInput* nodeInput)
        {
            afe = new AnalogFrontEnd(node,
                                     phyIndex,
                                     nodeAddress,
                                     nodeInput);
            
            ap = new AntennaParabolic(node,
                                      phyIndex,
                                      nodeAddress,
                                      nodeInput);
            
            rsv = new DigitalBurstRsv(node,
                                      phyIndex,
                                      nodeAddress,
                                      nodeInput);
            
            lna = new Lna(node,
                          phyIndex,
                          nodeAddress,
                          nodeInput);
            
            mixer = new Mixer(node,
                              phyIndex,
                              nodeAddress,
                              nodeInput);
            
            twta = new Twta(node,
                            phyIndex,
                            nodeAddress,
                            nodeInput);
                
            push_tx(ap);
            push_tx(twta);
            push_tx(mixer);
            push_tx(afe);
            push_tx(rsv);
            
            push_rx(ap);
            push_rx(lna);
            push_rx(mixer);
            push_rx(afe);
            push_rx(rsv);
            
        }
        
        ///
        /// @brief Calculate tranmission duration of abstract
        /// frame size provided
        ///
        /// @param sizeInBits Size of abstract frame in bits
        /// 
        /// @returns the total length of transmission in seconds
        ///        
        
        clocktype getTransmissionDuration(int sizeInBits)
        {
                clocktype result =
                rsv->getTransmissionDuration(sizeInBits);
                
                return result;
        }
        
        ///
        /// @brief Requests the current tranmission chain to 
        /// change its power level
        ///
        /// @param power_mW specified power in mW
        ///
        
        void setTransmitPower(double power_mW)
        {
            double pwr = UTIL::abs2db(power_mW / 1000.0);
            
            twta->setTransmitPower(pwr);
        }
        
        /// 
        /// @brief Requests the processing chain to provide the
        /// current output power
        ///
        /// @returns the current transmit power in mW
        ///
        
        double getTransmitPower()
        {
            double transmitPower = twta->getTransmitPower();
            
            return 1000 * UTIL::db2abs(transmitPower);
        }
        
        ///
        /// @brief Requests the processing chain to return the
        /// current transmission rate
        ///
        /// @returns the current transmission rate in bits/second
        ///
        /// This represents the raw transmission rate of the modulated
        /// channel and does not account for user headers, coding,
        /// guard times, synchronization, etc.
        ///  
        
        double getTransmitRate()
        {
            return rsv->getTransmitRate();
        }
        
        ///
        /// @brief Requests the processing chain to return the 
        /// current receive rate
        ///
        /// 
        /// @returns the current transmission rate in bits/second
        ///
        /// This represents the raw reception rate of the modulated
        /// channel and does not account for user headers, coding,
        /// guard times, synchronization, etc.
        ///        
        
        double getReceiveRate()
        {
            return rsv->getReceiveRate();
        }
        
    } ;
    
}

// /**
// STRUCT :: PhySattsmStatistics 
// DESCRIPTION :: This data structure holds all counters for 
// statistics gathered by the physical layer process.  These are 
// printed out when the RuntimeStat API is used. 
// **/

/// @namespace PhySattsm
namespace PhySattsm
{
    
    /// 
    /// @struct Statistics
    ///
    /// @brief The statistics maintained by this PHY
    ///
    struct Statistics
    {
        /// Total number of signals sent
        UInt32 totalTxSignals;
        
        /// Total number of signals received and sent to MAC
        UInt32 totalRxSignalsToMac;
        
        /// Total number of signals locked (i.e. detected)
        UInt32 totalSignalsLocked;
        
        /// Total number of signals with errors
        UInt32 totalSignalsWithErrors;
    } ;
}

// /**
// STRUCT :: PhySattsmState
// DESCRIPTION :: The state data for the Satellite RSV physical
// layer process.  This state data is part of the overall node and 
// generic physical layer state.  
// **/

/// @namespace PhySattsm
namespace PhySattsm
{
    ///
    /// Enumerated values for PHY mode processing
    ///
    /// @var StatusTranmissing
    /// @brief PHY is transmitting currently
    /// @var StatusReceiving
    /// @brief PHY is receiving currently
    /// @var StatusIdle 
    /// @brief PHY is idle currently
    ///
    
    static const unsigned int StatusTransmitting = 1;
    static const unsigned int StatusReceiving = 2;
    static const unsigned int StatusIdle = 0;
    
    ///
    /// @class Status
    ///
    /// @brief A class that implements mode logic for the PHY
    ///
    
    class Status
    {
        /// Current mode
        unsigned int m_mode;
        
        /// 
        /// @brief Notify MAC of status chain
        ///
        /// @param node a pointer to the local node data structure
        /// @param phyIndex the logical index of the current interface
        /// @param status the new status of the PHY
        ///
        
        void notify(Node* node,
                    int phyIndex,
                    PhyStatusType status)
        {
            PhySattsmReportExtendedStatusToMac(node,
                                                 phyIndex,
                                                 status,
                                                 0,
                                                 NULL);

        }
        
    public:
        
        ///
        /// @brief Default (empty) constructor
        ///
        /// @param mode Initial mode of the status variable
        ///
            
        Status(int mode = StatusIdle)
        : m_mode(mode)
        {
                
        }
            
        ///
        /// @brief Class destructor
        ///
        
        ~Status()
        {
            
        }

        ///
        /// @brief Request the status variable reset its mode to idle
        ///
        
        void reset()
        {
            m_mode = StatusIdle;
        }
        
        /// 
        /// @brief Indicate to the status variable that a transmission
        /// has started
        ///
        /// @param node a pointer to the local node data structure
        /// @param phyIndex the logical index of the current interface
        ///
        /// This routine may inform the MAC of a change in status
        ///
            
        void transmissionStart(Node* node,
                               int phyIndex)
        {
            ERROR_Assert(!isTransmitting(),
                         "This PHY cannot sustain multiple"
                         " simultaneous transmissions.  The simulation"
                         " cannot continue.");

            //Informing the Energy Model about the change in PhyStatus
            if (isIdle() )
            {
                Phy_ReportStatusToEnergyModel(node,phyIndex,PHY_IDLE,PHY_TRANSMITTING);
            }
            else if (isReceiving() )
            {
                Phy_ReportStatusToEnergyModel(node,phyIndex,PHY_RECEIVING,PHY_TRANSMITTING);

            }
            else
            {
                printf("\nERROR in transmission start");
            }
            
            m_mode |= StatusTransmitting;
            
            notify(node, 
                   phyIndex,
                   PHY_TRANSMITTING);
        }
        
        /// 
        /// @brief Indicate to the status variable that a transmission
        /// has ended
        ///
        /// @param node a pointer to the local node data structure
        /// @param phyIndex the logical index of the current interface
        ///
        /// This routine may inform the MAC of a change in status
        ///
            
        void transmissionEnd(Node* node,
                             int phyIndex)
        {
            ERROR_Assert(isTransmitting(),
                         "The PHY has encountered a logic error."
                         " The system cannot cease transmission on"
                         " an idle channel.  The simulation cannot"
                         " continue.");
            
            m_mode &= ~StatusTransmitting;
            
            if (isReceiving())
            {

                //Informing the Energy Model about the change in PhyStatus
                Phy_ReportStatusToEnergyModel(node,phyIndex,PHY_TRANSMITTING,PHY_RECEIVING);


                notify(node,
                       phyIndex,
                       PHY_RECEIVING);
            }
            else
            {
                Phy_ReportStatusToEnergyModel(node,phyIndex,PHY_TRANSMITTING,PHY_IDLE);

                notify(node,
                       phyIndex,
                       PHY_IDLE);
            }
        }
         
        
        /// 
        /// @brief Indicate to the status variable that a reception
        /// has started
        ///
        /// @param node a pointer to the local node data structure
        /// @param phyIndex the logical index of the current interface
        ///
        /// This routine may inform the MAC of a change in status
        ///
        
        void receptionStart(Node* node,
                            int phyIndex)
        {
            bool wasIdle = isIdle();
            bool wasTransmitting = isTransmitting();
            
            m_mode |= StatusReceiving;
            //Informing the Energy Model about the change in PhyStatus    
            if (wasIdle)
            {
                Phy_ReportStatusToEnergyModel(node,phyIndex,PHY_IDLE,PHY_RECEIVING);
                notify(node,
                       phyIndex,
                       PHY_RECEIVING);
            }
            else if (wasTransmitting)
            {
                Phy_ReportStatusToEnergyModel(node,phyIndex,PHY_TRANSMITTING,PHY_RECEIVING);
            }
        }
        
        /// 
        /// @brief Indicate to the status variable that a reception
        /// has ended
        ///
        /// @param node a pointer to the local node data structure
        /// @param phyIndex the logical index of the current interface
        ///
        /// This routine may inform the MAC of a change in status
        ///
            
        void receptionEnd(Node* node,
                          int phyIndex)
        {
            m_mode &= ~StatusReceiving;
            
            if (isIdle())
            {
                notify(node,
                       phyIndex,
                       PHY_IDLE);


            }
                Phy_ReportStatusToEnergyModel(node,phyIndex,PHY_RECEIVING,PHY_IDLE);
        }
        
        /// 
        /// @brief Calculate whether the object is transmitting
        ///
        /// @returns True is transmitting, false otherwise
        ///
            
        bool isTransmitting()
        {
            return ( (m_mode & StatusTransmitting) == StatusTransmitting);
        }
        
        /// 
        /// @brief Calculate whether the object is receiving
        ///
        /// @returns True is receiving, false otherwise
        ///
            
        bool isReceiving()
        {
            return ( (m_mode & StatusReceiving) == StatusReceiving);
        }
        
        /// 
        /// @brief Calculate whether the object is idle
        ///
        /// @returns True is idle, false otherwise
        ///
            
        bool isIdle()
        {
            return (m_mode == StatusIdle);
        }
        
        ///
        /// @brief Calculate whether the current mode is equivalent
        /// to a provided status variable
        ///
        /// @param s a reference to a status variable
        ///
        /// @returns True if the status is identical, false otherwise
        ///
        
        bool operator ==(Status& s)
        {
            return (s.m_mode == m_mode);
        }
        
        ///
        /// @brief Calculate the current physical layer status
        /// with respect to transmission
        ///
        /// @returns the physical layer status type regarding
        /// transmission
        ///
        
        PhyStatusType getPhyStatusTx()
        {
            if (isTransmitting())
            {
                return PHY_TRANSMITTING;
            }
            
            return PHY_IDLE;
        }
        
        ///
        /// @brief Calculate the current physical layer status
        /// with respect to reception
        ///
        /// @returns the physical layer status type regarding
        /// reception
        ///
        
        PhyStatusType getPhyStatuxRx()
        {
            if (isReceiving())
            {
                return PHY_RECEIVING;
            }
            
            return PHY_IDLE;
        }
        
        ///
        /// @brief Calculate the current physical layer status
        /// with respect to both transmission and reception
        ///
        /// @returns the physical layer status type regarding
        /// both transmission and reception
        ///
        /// In this case, the transmission status is primary
        /// and the reception status is secondary to conform to
        /// the current status API in phy.cpp
        ///
        
        PhyStatusType getPhyStatus()
        {
            if (isTransmitting())
            {
                return PHY_TRANSMITTING;
            }
            
            if (isReceiving())
            {
                return PHY_RECEIVING;
            }
            
            return PHY_IDLE;
        }
    }; 
    
    ///
    /// @class State
    ///
    /// @brief The PHY state data for the SATTSM PHY
    ///
    
    struct State 
    {
        
        /// Back pointer to parent generic PHY data structure
        PhyData* thisPhy; 
        
        /// Status variable
        Status mode;
        
        /// Signals current active
        int activeSignalCount;
        
        /// Statistics storage
        Statistics stats;
        
        /// PHY processing chain
        PhyEntity* p_chain;
        
        /// Current signal list
        SignalList p_signalList;
        
        ///
        /// A flag to indicate whether statistics have been written
        /// for this module
        ///
        
        BOOL statsWritten;
        
        ///
        /// @brief Default (empty) constructor
        ///
        
        State() 
        : thisPhy(NULL), activeSignalCount(0),
          p_chain(NULL), 
          statsWritten(FALSE)
        {
              
        }
        
        ///
        /// @brief Class destructor
        ///
        
        ~State()
        {
            p_chain = NULL;
            thisPhy = NULL;
        }
        
    } ;
}


// /**
// FUNCTION :: PHY_PhyStatusTypeToString
// LAYER :: PHY
// PURPOSE :: This function translates a PhyStatusType enumeration
// into a human-readable text string.
// PARAMETERS ::
// +       type : PhyStatusType : An enumerated value indicating 
//         a status layer type.  If the value is unrecognized, an
//         error handling function will be invoked.
// RETURN : char* : A pointer to a character array containing a
//          text message.  This point should be held const and not modified. 
// **/

static 
const char* PHY_PhyStatusTypeToString(PhyStatusType type) 
{
    switch(type) {
        case PHY_IDLE: return "[IDLE]"; break;
        case PHY_SENSING: return "[SENSING]"; break;
        case PHY_RECEIVING: return "[RECEIVING]"; break;
        case PHY_TRANSMITTING: return "[TRANSMITTING]"; break;
        default: ERROR_ReportError("Unknown PhyStatusType.");
    }
    
    return "[UNKNOWN]";
}

// /**
// FUNCTION :: PhySattsmInit
// LAYER :: PHY
// PURPOSE :: This function initializes the SATTSM process.  It is 
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
                   const NodeInput* nodeInput);

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
                             int phyNum);

// /**
// FUNCTION :: PhySattsmTransmissionEnd
// LAYER :: PHY
// PURPOSE :: This function implements the model logic to be executed when
//            the system has completed transmitting a packet.
// PARAMETERS ::
// +        node : Node* : state : PhySattsmState* : A pointer to the 
//          physical layer state machine state data for the current state 
//          machine
// +        phyIndex : int : The logical index of the current interface
// RETURN :: void : This function does not return a value. 
// **/

void PhySattsmTransmissionEnd(Node* node, 
                                    int phyIndex);

// /**
// FUNCTION :: PhySattsmvGetStatus
// LAYER :: PHY
// PURPOSE :: This function returns the current transceiver status for the 
//            phyisical layer process. This function is called by other 
//            functions outside of this file.  
// PARAMETERS ::
// +        node : Node* : state : PhySattsmState* : A pointer to the 
//          physical layer state machine state data for the current state 
//          machine
// +        phyIndex : int : The logical index of the current interface
// RETURN :: PhyStatusType : This function returns the instantaneous sample
//           of the mode of the physical layer transceiver.
// **/

PhyStatusType PhySattsmGetStatus(Node* node, 
                                       int phyNum);

// /**
// FUNCTION :: PhySattsmStartTransmittingSignal
// LAYER :: PHY
// PURPOSE :: This function implements the algorithm to transmit a signal
//            on the satellite subnet wireless channel.  This function is 
//            called by other functions outside of this file.  
// PARAMETERS ::
// +        node : Node* : state : PhySattsmState* : A pointer to the 
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

void PhySattsmvStartTransmittingSignal(Node* node,
                                       int phyNum,
                                       Message *msg,
                                       BOOL useMacLayerSpecifiedDelay,
                                       clocktype delayUntilAirborne);

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
                                       PropRxInfo* propRxInfo);

// /**
// FUNCTION :: PhySattsmSignalEndFromChannel
// LAYER :: PHY
// PURPOSE :: This function implements the model logic to respond to 
//            a completed reception of a packet.  This function is 
//            called by other functions outside of this file.    
// PARAMETERS ::
// +        node : Node* : state : PhySattsmState* : A pointer to the 
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

void PhySattsmSignalEndFromChannel(Node* node, 
                                   int phyIndex,
                                   int channelIndex,
                                   PropRxInfo *propRxInfo);

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

int PhySattsmGetTxDataRate(PhyData* phyData);

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

int PhySattsmGetRxDataRate(PhyData* phyData);

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
                                int dataRateType);

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

int PhySattsmGetRxDataRateType(PhyData* thisPhy);

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

int PhySattsmGetTxDataRateType(PhyData* thisPhy);

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

void PhySattsmSetLowestTxDataRateType(PhyData* thisPhy);

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
                                            int* dataRateType);

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

void PhySattsmSetHighestTxDataRateType(PhyData* thisPhy);

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
                                             int* dataRateType);

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

void PhySattsmSetHighestTxDataRateTypeForBC(PhyData* thisPhy);

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
                                                  int* dataRateType);

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
                                            int dataRate);

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
                                     double newTxPower_mW);

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

void PhySattsmGetTransmitPower(PhyData* phyData, 
                                     double* txPower_mW);

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

double PhySattsmGetLastAngleOfArrival(Node* node,
                                            int phyNum);

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
                                       clocktype* endSignalTime);

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

void PhySattsmTerminateCurrentReceive(Node* node,
                                            int phyIndex,
                                            const BOOL terminateOnlyOnReceiveError,
                                            BOOL* receiveError,
                                            clocktype* endSignalTime);

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
                                                    double azimuthAngle);

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
                                         int phyNum);

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
                                           int phyNum);

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
                                            double azimuth);

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
                                        double azimuth);



#endif /* PHY_SATELLITE_RSV_H */
