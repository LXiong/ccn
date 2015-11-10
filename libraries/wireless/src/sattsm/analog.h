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
#ifndef __SATTSM_ANALOG_H__
# define __SATTSM_ANALOG_H__

#include "sattsm.h"

#include "afe-base.h"
#include "afe-header.h"

#include "digital.h"

namespace PhySattsm
{
    
    namespace AfeDefaults
    {
        /// The default transmit frequency in Hertz
        static const double TransmitFrequency = 1450e6;
        
        /// The default receive frequency in Hertz
        static const double ReceiveFrequency = 950e6;
        
        /// The default AFE bandwidth in Hertz
        static const double Bandwidth = 500e6;
        
        /// The default AFE transmit signal power in dBW
        static const double TransmitSignalPower = -30.0;
        
        /// The default receiver sensitivity in dBW
        static const double ReceiveSensitivity = -110.0;
        
    }
    
    ///
    /// @brief A data structure that defines an Analog Front
    /// End (AFE)
    ///
    /// @class AnalogFrontEnd
    ///
    
    class AnalogFrontEnd : public ProcessingStage
    {
        /// The current transmit frequency in Hertz
        double m_transmitFrequency;
        
        /// The current receive frequency in Hertz
        double m_receiveFrequency;
        
        /// The current bandwidth of the AFE in Hertz
        double m_bandwidth;
        
        /// The current AFE transmit power in dBW
        double m_transmitSignalPower;
        
        /// The current AFE receiver sensitivity in dBW
        double m_receiveSensitivity;
        
    public:
           
            
        /// 
        /// @brief Constructor for QualNet configuration
        /// file
        ///
        /// @param node a pointer to the local node data structure
        /// @ifidx a logical index of the local PHY process
        /// @nodeAddress a pointer to an Address data structure for
        /// the local network process
        /// @param nodeInput a pointer to an internal data structure
        /// representing the current QualNet configuration file
        ///
            
        AnalogFrontEnd(Node* node, 
                       int ifidx,
                       Address* nodeAddress,
                       const NodeInput* nodeInput)
        {
            double temp_dbl(0);
            BOOL wasFound = FALSE;
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-AFE-TRANSMIT-FREQUENCY",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_transmitFrequency = AfeDefaults::TransmitFrequency;
            }
            else
            {
                m_transmitFrequency = temp_dbl;
            }
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-AFE-RECEIVE-FREQUENCY",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_receiveFrequency = AfeDefaults::ReceiveFrequency;
            }
            else
            {
                m_receiveFrequency = temp_dbl;
            }
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-AFE-BANDWIDTH",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_bandwidth = AfeDefaults::Bandwidth;
            }
            else
            {
                m_bandwidth = temp_dbl;
            }
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-AFE-TRANSMIT-SIGNAL-POWER",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_transmitSignalPower = AfeDefaults::TransmitSignalPower;
            }
            else
            {
                m_transmitSignalPower = temp_dbl;
            }
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-AFE-RECEIVE-SENSITIVITY",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_receiveSensitivity = AfeDefaults::ReceiveSensitivity;
            }
            else
            {
                m_receiveSensitivity = temp_dbl;
            }
            
        }
        
        /// 
        /// @brief Constructure for directly specified AFE
        /// parameters
        /// 
        /// @param transmitFrequency the AFE transmission frequency
        /// in Hertz
        /// @param receiveFrequency the AFE receive frequency in 
        /// Hertz
        /// @param bandwidth the AFE bandwidth in Hertz
        /// @param tranmitSignalPower the local AFE transmission
        /// power in dBW
        /// @param receiveSensitvity the local AFE receiver
        /// sensitivity in dBW
        ///
        
        AnalogFrontEnd(double transmitFrequency 
                            = AfeDefaults::TransmitFrequency,
                        double receiveFrequency
                            = AfeDefaults::ReceiveFrequency,
                        double bandwidth
                            = AfeDefaults::Bandwidth,
                        double transmitSignalPower
                            = AfeDefaults::TransmitSignalPower,
                        double receiveSensitivity
                            = AfeDefaults::ReceiveSensitivity)
            : m_transmitFrequency(transmitFrequency),
              m_receiveFrequency(receiveFrequency),
              m_bandwidth(bandwidth),
              m_transmitSignalPower(transmitSignalPower),
              m_receiveSensitivity(receiveSensitivity)
        {
                  
        }
        
        ///
        /// @brief Destructor hook
        ///
        
        ~AnalogFrontEnd()
        {
            
        }
        
        ///
        /// @brief Handle a transmission (down stack) event in
        /// the processing stage
        ///
        /// @param node A pointer to the local node data structure
        /// @param ifidx The logical index of the local PHY process
        /// @param A pointer to a data structure holding the current
        /// frame undergoing transmission
        /// @param ok A return flag to indicate whether or not the
        /// current module processing stage is successful (true) or
        /// not (false)
        ///
        /// @see ProcessingStage
        ///
        
        void onSend(Node* node,
                    int ifidx,
                    Message* msg,
                    bool& ok)
        {
            DigitalHeader dighdr(msg);
            
            MESSAGE_AddHeader(node,
                              msg,
                              sizeof(AnalogHeader),
                              TRACE_SATTSM);
            
            AnalogHeader anahdr(msg);
            
            anahdr.polarization = PolarizationUnspecified;
            anahdr.signalPower = m_transmitSignalPower;
            anahdr.noisePower = UTIL::Constants::Log0;
            anahdr.interferencePower = UTIL::Constants::Log0;
            
            anahdr.transmissionStartTime = node->getNodeTime();
            anahdr.transmissionEndTime = node->getNodeTime() 
                + UTIL::double2clocktype(dighdr.totalEmissionTime);
            
            Coordinates coord;
            MOBILITY_ReturnCoordinates(node, &coord);
            
            SolidPath path(coord);
            
            memcpy((void*)&anahdr.entryPoint,
                   (void*)&path,
                   sizeof(SolidPath));
            
            anahdr.carrierFrequency = m_transmitFrequency;
            anahdr.bandwidth = m_bandwidth;
            
            anahdr.analogError = AnalogErrorNone;
            
            ok = true;
            
            anahdr.write(msg);
        }
        
        /// 
        /// @brief Update the local signal record and cache with
        /// a new signal
        ///
        /// @param node a pointer to the local node data structure
        /// @ifidx the logical index of the local PHY process
        /// @msg a pointer to the incoming signal
        ///
                
        void onIncomingSignal(Node* node,
                              int ifidx,
                              Message* msg)
        {
            AnalogHeader h(msg);
            
            if (h.analogError != 0)
            {
                return;
            }
            
            double maxFrequencyError = 
                fabs(h.bandwidth - m_bandwidth)/2.0;
            
            double actualFrequencyError = fabs(h.carrierFrequency -
                                               m_receiveFrequency);
            
            if (actualFrequencyError > maxFrequencyError)
            {
                h.analogError |=
                    AnalogErrorFrequencyErrorTooLarge;
            }
            
            if (h.signalPower < m_receiveSensitivity)
            {
                h.analogError |=
                    AnalogErrorSignalBelowReceiverThreshold;
            }
            
            h.write(msg);
        }
        
        ///
        /// @brief Process a receive (up stack) indication
        ///
        /// @param node a pointer to the local node data structure
        /// @param ifidx the logical interface index of the local
        /// PHY process
        /// @param msg a pointer to the frame undergoing reception
        /// @param ok a reference to a flag that indicates whether
        /// or not the current reception process stage concluded
        /// successfully or not
        /// @param srec a reference to the signal record for 
        /// this frame with reference to the current time, node, 
        /// and antenna configurations
        ///
        /// @see ProcessingStage
        ///
        
        void onReceive(Node* node,
                       int ifidx,
                       Message *msg,
                       bool& ok,
                       SignalRecord& srec)
        {
            onIncomingSignal(node,
                             ifidx,
                             msg);
            
            AnalogHeader h(msg);
            
            MESSAGE_AddInfo(node,
                            msg,
                            sizeof(AnalogHeader),
                            INFO_TYPE_DEFAULT);
            
            memcpy(MESSAGE_ReturnInfo(msg, INFO_TYPE_DEFAULT),
                   (void*)&h,
                   sizeof(AnalogHeader));
            
            MESSAGE_RemoveHeader(node,
                                 msg,
                                 sizeof(AnalogHeader),
                                 TRACE_SATTSM);
            
            ok = (h.analogError == 0);
        }
        
        ///
        /// @brief Peform run-time statistics processing
        ///
        /// @param node a pointer to the current node data structure
        ///
        /// @see Processing Stage
        ///
        
        void onRunTimeStat(Node* node)
        {
            
        }
    } ;
    
}

/// 
/// @brief Amplify an analog signal (all components)
///
/// @param h a reference to the current AnalogHeader
/// @param gain_dB the reference gain in dB
///

void PhySattsmAnalogAmplify(PhySattsm::AnalogHeader& h, 
                            double gain_dB);

///
/// @brief Inject noise power 
///
/// @param h a reference to the current AnalogHeader
/// @param pwr_dB the amount of noise power in dBW
///

void
PhySattsmAnalogInjectNoisePower(PhySattsm::AnalogHeader& h, 
                                double pwr_dB);

///
/// @brief Inject noise power based on a Noise Figure (NF)
///
/// @param h a reference to the current AnalogHeader
/// @param nf a noise source epxressed as a noise figure
///

void
PhySattsmAnalogInjectNoiseByNF(PhySattsm::AnalogHeader& h, 
                               double nf);

/// 
/// @brief Inject noise power by equivalent temperature
///
/// @param h a reference to the current AnalogHeader
/// @param the noise intensity in K
///

void
PhySattsmAnalogInjectNoiseByTemp(PhySattsm::AnalogHeader& h, 
                                 double t);

/// 
/// @brief Inject Noise Power based on noise spectral density
/// 
/// @param a reference to the current AnalogHeader data structure
/// @param n0 the PSD of the noise source (/Hertz)
///

void
PhySattsmAnalogInjectNoiseByNo(PhySattsm::AnalogHeader& h, 
                               double n0);

/// 
/// @brief Calculate the signal to noise level of an
/// AnalogHeader
///
/// @param h a reference to the current AnalogHeader
///
/// @returns the SNR expressed in dB
///

double
PhySattsmAnalogCalculateSNR(PhySattsm::AnalogHeader& h);




#endif                          /* __SATTSM_ANALOG_H__ */
