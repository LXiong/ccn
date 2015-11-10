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

#ifndef __SATTSM_LNA_H__
# define __SATTSM_LNA_H__

#include "util_constants.h"

#include "sattsm.h"
#include "afe-base.h"
#include "afe-header.h"

///
/// @file lna.h
///
/// @brief The class header definition file for the LNA module
///

///
/// @namespace PhySattsm
/// 
/// @brief The default PHY Sattsm namespace
///

namespace PhySattsm
{
    
    ///
    /// @namespace LnaDefaults
    ///
    /// @brief The namespace for default parameters for the 
    /// LNA module
    ///

    namespace LnaDefaults 
    {
        /// Default Noise Figure (in dB)
        static const double NoiseFigure = 1.0;
        
        /// Default LNA gain (in dB)
        static const double Gain = 10.0;
        
        /// Default bandwidth of the LNA in Hertz
        static const double NoiseBandwidth = 500.0e6;
    }
    
    ///
    /// @brief This class implements a model of a low noise
    /// amplifier
    ///
    /// @class Lna
    ///

    class Lna : public ProcessingStage
    {
        /// current noise figure in dB
        double m_nf;
        
        /// current gain in dB
        double m_gain;
        
        ///
        /// current bandwidth in Hertz.  At this point it may
        /// be more accurate to say this is not particularly used
        /// since a suitable digital communication system would
        /// filter the waveform to select the carrier of choice.
        /// 
        /// The use of this is more how much power is being used
        /// by the LNA on noise transmission into the amplifier
        /// affecting its operating point.  This is not modeled
        /// in this software.
        ///
        
        double m_noiseBandwidth;
        
        /// Noise power of amplifier
        double m_noisePower;
        
        /// Noise power per Hertz.  This is calculated and is used
        double m_noisePowerPerHertz;
        
        ///
        /// @brief Transform static and dynamic parameters into
        /// calculated values within the model
        ///
        
        void deriveOperationalParameters()
        {
            m_noisePowerPerHertz = UTIL::abs2db(UTIL::nf2t(m_nf)) + 
                UTIL::Constants::LogKb;
            
            m_noisePower = m_noisePowerPerHertz + 
                UTIL::abs2db(m_noiseBandwidth);
        }
        
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
            
        Lna(Node* node,
            int phyIndex,
            Address* nodeAddress,
            const NodeInput* nodeInput)
        {
            double temp_dbl(0);
            BOOL wasFound = FALSE;
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-LNA-NOISE-FIGURE",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_nf = LnaDefaults::NoiseFigure;
            }
            else
            {
                m_nf = temp_dbl;
            }
            
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-LNA-GAIN",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_gain = LnaDefaults::Gain;
            }
            else
            {
                m_gain = temp_dbl;
            }
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-LNA-NOISE-BANDWIDTH",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_noiseBandwidth = LnaDefaults::NoiseBandwidth;
            }
            else
            {
                m_noiseBandwidth = temp_dbl;
            }
            
            deriveOperationalParameters();
        }
        
        /// 
        /// @brief Constructor given direct parameterization
        ///
        /// @param nf the noise figure for the LNA in dB
        /// @param gain the gain of the LNA in dB
        /// @param noiseBandwidth the noise bandwidth of the
        /// LNA in Hertz
        /// 
        
        Lna(double nf = LnaDefaults::NoiseFigure,
            double gain = LnaDefaults::Gain,
            double noiseBandwidth = LnaDefaults::NoiseBandwidth)
        : m_nf(nf), m_gain(gain), m_noiseBandwidth(noiseBandwidth)
        {
            deriveOperationalParameters();
        }
        
        /// 
        /// @brief Destructor for the LNA class
        ///
        
        ~Lna() 
        {
            
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
                       const SignalRecord& srec)
        {
            AnalogHeader h(msg);
            
            PhySattsmAnalogInjectNoiseByNo(h, 
                                            m_noisePowerPerHertz);
            PhySattsmAnalogAmplify(h, 
                                   m_gain);
            
            ok = true;
            
            h.write(msg);
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
            ERROR_ReportError("The LNA is not part of the satellite"
                              " transmission chain.  Please verify"
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
            static const char* layerName = "MAC";
            static const char* moduleName = "PhySattsmLna";
            
            
        }
        
    } ;
}


#endif                          /* __SATTSM_LNA_H__ */
