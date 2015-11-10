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
#ifndef __SATTSM_MIXER_H__
# define __SATTSM_MIXER_H__

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
    /// @namespace MixerDefaults
    ///
    /// @brief The namespace for default parameters for the 
    /// Mixer module
    ///
    
    namespace MixerDefaults
    {
        /// Default noise figure in dB
        static const double NoiseFigure = 5.0;
        
        /// Default gain in dB
        static const double Gain = 10.0;
        
        ///
        /// Default bandwidth in Hertz 
        /// @see LnaDefaults for caveats
        ///
        
        static const double Bandwidth = 500.0e9;
        
        /// Default frequency error in Hertz
        static const double FrequencyError = 0.0;
    
    }

    /// 
    /// @class Mixer
    ///
    /// @brief A class to implement a mixer processing stage
    /// model
    ///
    
    class Mixer : public ProcessingStage 
    {

        /// Current noise figure in dB
        double m_nf;
        
        /// Current gain in dB
        double m_gain;
        
        /// Current noise bandwidth in Hertz
        double m_noiseBandwidth;
        
        /// Current noise power in dBW
        double m_noisePower;
        
        /// Current noise power in dB(W/Hz)
        double m_noisePowerPerHertz;
        
        /// Current frequency offset
        double m_deltaFrequency;
        
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
        /// @brief Constructor given direct parameterization
        ///
        /// @param nf the noise figure for the mixer in dB
        /// @param gain the gain of the mixer in dB
        /// @param noiseBandwidth the noise bandwidth of the
        /// mixer in Hertz
        /// @param deltaFrequency the frequency of the mixer in Hertz
        /// 
            
        Mixer(double nf,
              double gain,
              double noiseBandwidth,
              double deltaFrequency)
        : m_nf(nf), m_gain(gain), m_noiseBandwidth(noiseBandwidth),
          m_deltaFrequency(deltaFrequency)
        {
                deriveOperationalParameters();
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
        
        Mixer(Node* node, 
              int ifidx,
              Address* nodeAddress,
              const NodeInput* nodeInput)
        {
            double temp_dbl(0);
            BOOL wasFound = FALSE;
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-MIXER-NOISE-FIGURE",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_nf = MixerDefaults::NoiseFigure;
            }
            else
            {
                m_nf = temp_dbl;
            }
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-MIXER-GAIN",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_gain = MixerDefaults::Gain;
            }
            else
            {
                m_gain = temp_dbl;
            }
            

            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-MIXER-BANDWIDTH",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_noiseBandwidth = temp_dbl;
            }
            else
            {
                m_noiseBandwidth = MixerDefaults::Bandwidth;
            }
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-MIXER-FREQUENCY-ERROR",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_deltaFrequency = temp_dbl;
            }
            else
            {
                m_deltaFrequency = MixerDefaults::FrequencyError;
            }
            
            deriveOperationalParameters();
        }
        
        /// 
        /// @brief Destructor for the LNA class
        ///
        
        ~Mixer() 
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
            
            PhySattsmAnalogInjectNoiseByNo(h, m_noisePowerPerHertz);
            PhySattsmAnalogAmplify(h, m_gain);
            
            h.carrierFrequency += m_deltaFrequency;
            
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
            ERROR_ReportError("The mixer is not part of the satellite"
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
            static const char* moduleName = "PhySattsmMixer";
            
        }
    } ;
    
}

#endif                          /* __SATTSM_MIXER_H__ */
