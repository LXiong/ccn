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
#ifndef __SATTSM_TWTA_H__
# define __SATTSM_TWTA_H__

#include "sattsm.h"
#include "afe-base.h"
#include "afe-header.h"

///
/// @file twta.h
///
/// @brief The class header definition file for the TWTA module
///

///
/// @namespace PhySattsm
/// 
/// @brief The default PHY Sattsm namespace
///

namespace PhySattsm
{
    
    ///
    /// @namespace TwtaDefaults
    ///
    /// @brief The namespace for default parameters for the 
    /// Travelling Wave Tube Amplifer (TWTA) module
    ///
    
    namespace TwtaDefaults 
    {
        /// Default bandwidth of the TWTA
        static const double Bandwidth = 500e6;
        
        /// Default output power of the TWTA in dBW
        static const double OutputPower = UTIL::abs2db(100);
    }
    
    ///
    /// @class Twta
    ///
    /// @brief This class implements a model fo a Travelling Wave
    /// Tube Amplifier (TWTA) commonly used for high power 
    /// transmission on ground and satellite stations
    ///
    
    class Twta : public ProcessingStage
    {
        /// Current output power in dBW
        double m_outputPower;
        
        /// Current bandwidth in Hertz
        double m_bandwidth;
        
        /// 
        /// @brief Current output power per Hertz 
        /// 
        /// This assumes that the TWTA if fully loaded and
        /// power is uniformly distributed between all carriers
        ///
        
        double m_pwrPerHz;
        
        ///
        /// @brief Transform static and dynamic parameters into
        /// calculated values within the model
        ///
        
        void deriveOperationalParameters()
        {
            m_pwrPerHz = m_outputPower - UTIL::abs2db(m_bandwidth);
        }

        
    public:
            
        /// 
        /// @brief Return current transmit power in dBW
        ///
        /// @returns current transmit power in dBW
        ///
            
        double getTransmitPower()
        {
                return m_outputPower;
        }
        
        ///
        /// @brief Set the current transmit power of module
        ///
        /// @param tx_power the desired transmit power in dBW
        ///
        
        void setTransmitPower(double tx_power)
        {
            m_outputPower = tx_power;
        }
        
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
        
        Twta(Node* node,
             int phyIndex,
             Address* nodeAddress,
             const NodeInput* nodeInput)
        {
            double temp_dbl;
            BOOL wasFound;
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-TWTA-BANDWIDTH",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_bandwidth = TwtaDefaults::Bandwidth; 
            }
            else
            {
                m_bandwidth = temp_dbl;
            }
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-TWTA-OUTPUT-POWER",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_outputPower = TwtaDefaults::OutputPower; 
            }
            else
            {
                m_bandwidth = temp_dbl;
            }
            
            deriveOperationalParameters();
        }
        
        /// 
        /// @brief Constructor given direct parameterization
        ///
        /// @param bandwidth the total bandwidth of the TWTA in Hertz
        /// @param outputPower the output power of the TWTA in dBW
        ///
        
        Twta(double bandwidth = TwtaDefaults::Bandwidth,
             double outputPower = TwtaDefaults::OutputPower)
            : m_outputPower(outputPower), m_bandwidth(bandwidth)
        {
                deriveOperationalParameters();
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
        
        void onSend(Node *node,
                    int phyIndex,
                    Message* msg,
                    bool& ok)
        {
            AnalogHeader hdr(msg);
            
            // 
            // Processing commands
            // 
            
            double inputPower = 
                UTIL::dbAddAbs(hdr.signalPower,
                               UTIL::dbAddAbs(hdr.noisePower,
                                              hdr.interferencePower));
            
            double outputPower = UTIL::abs2db(hdr.bandwidth) + m_pwrPerHz;
            double gain = outputPower - inputPower;
            
            PhySattsmAnalogAmplify(hdr, gain);
            
            ok = true;
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
                       int phyIndex,
                       Message* msg,
                       bool& ok,
                       SignalRecord& srec)
        {
            ERROR_ReportError("The TWTA is not part of the satellite"
                              " receive chain.  Please verify"
                              " the program code that invokes this"
                              " method.  The simulation cannot"
                              " continue.");
            
            ok = false;
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
            static const char* layerName = "PHY";
            static const char* moduleName = "PhySattsmTwta";

            
        }
             
    };
}

#endif /* __TWTA_H__ */

