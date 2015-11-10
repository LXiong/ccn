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
#ifndef __SATTSM_SSPA_H__
# define __SATTSM_SSPA_H__

#include "sattsm.h"

#include "afe-header.h"
#include "afe-base.h"
#include "util_constants.h"

///
/// @file sspa.h
///
/// @brief The class header definition file for the SSPA module
///

/// Debugging control MACRO
#define DEBUG 0


///
/// @namespace PhySattsm
/// 
/// @brief The default PHY Sattsm namespace
///

namespace PhySattsm
{
    
    ///
    /// @namespace SspaDefaults
    ///
    /// @brief The namespace for default parameters for the 
    /// Solid State Power Amplifier (SSPA) module
    ///
    
    namespace SspaDefaults
    {
        
        /// Default output power in dBW
        static const double OutputPower = 0.0;
        
        /// Default noise figure in dB
        static const double NoiseFigure = 2.0;
    }


    /// 
    /// @class Sspa
    ///
    /// @brief A module to the implement a SSPA component
    ///
    
    class Sspa : public ProcessingStage
    {
        /// gain in dBW
        double m_outputPower;
        
        /// noise figure in dB
        double m_nf;
        
        /// noisePower in dB(W/Hz)
        double m_noisePowerHz;
        
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
        
        Sspa(Node* node, 
             int ifidx,
             Address* nodeAddress,
             const NodeInput* nodeInput)
        {
            BOOL wasFound = FALSE;
            double temp_dbl;
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-OUTPUT-POWER-DBW",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_outputPower = SspaDefaults::OutputPower;
            }
            else
            {
                m_outputPower = temp_dbl;
            }
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-AMPLIFIER-NOISE-FIGURE",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_nf = SspaDefaults::NoiseFigure;
            }
            else
            {
                m_nf = temp_dbl;
            }
            
            m_noisePowerHz = UTIL::Constants::LogKb 
                + UTIL::abs2db(UTIL::nf2t(m_nf));
            
        }
        
        /// 
        /// @brief Constructor given direct parameterization
        ///
        /// @param outputPower the output power of the SSPA in dBW
        /// @param noiseFigure the noise figure for the SSPA in dB
        ///
        
        Sspa(double outputPower = SspaDefaults::OutputPower,
             double noiseFigure = SspaDefaults::NoiseFigure)
            : m_outputPower(outputPower),
              m_nf(noiseFigure),
              m_noisePowerHz(UTIL::Constants::LogKb + 
                             UTIL::abs2db(UTIL::nf2t(noiseFigure)))
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
            
            AnalogHeader hdr(msg);
            
            double inputPower = 
                UTIL::dbAddAbs(hdr.noisePower, 
                               UTIL::dbAddAbs(hdr.interferencePower,
                                              hdr.signalPower));
            
            double outputPower = m_outputPower;
            double gain = outputPower - inputPower;
            
            PhySattsmAnalogInjectNoiseByNo(hdr, 
                                           m_noisePowerHz);
            
            PhySattsmAnalogAmplify(hdr, 
                                   gain);
            
            if (DEBUG)
            {
                printf("signalPower=%e,"
                       " noisePower=%e, interferencePower=%e",
                       hdr.signalPower, 
                       hdr.noisePower, 
                       hdr.interferencePower);
            }
            
            ok = true;
            
            hdr.write(msg);
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
                       Message* msg,
                       bool& ok,
                       const SignalRecord& srec)
        {
            ERROR_ReportError("The SSPA model does not support"
                              " the reception of a waveform.  This"
                              " indicates a programming logic error."
                              " Please report this error to QualNet"
                              " technical support.");
            
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
            static const char *layerName = "MAC";
            static const char *moduleName = "PhySattsmSspa";
            
            // 
            // Generally uses
            // IO_PrintStat(node, layerName, moduleName, ANY_DEST, 
            // interfaceIndex, char*outputBuffer);
            // 
            
        }
            
        /// 
        /// @brief Destructor for the SSPA class
        ///
        
        ~Sspa() 
        {

            
        }
    } ;
    
}


//
// Header declarations
//

#endif                          /* __SATTSM_SSPA_H__ */
