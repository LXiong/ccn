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
#ifndef __SATTSM_ANTENNA_H__
# define __SATTSM_ANTENNA_H__

#include "sattsm.h"

#include "analog.h"

#include "afe-base.h"
#include "afe-header.h"

/// 
/// @file antenna_parabolic.h
///
/// @brief Class definition file for parabolic antenna class
///



///
/// @def DEBUG
///
/// @brief Debugging MACRO
///

#ifdef DEBUG
# under DEBUG
#endif

#define DEBUG 0

///
/// @namespace PhySattsm
/// 
/// @brief The default PHY Sattsm namespace
///

namespace PhySattsm
{
    
    ///
    /// @namespace AntennaParabolicDefaults
    ///
    /// @brief The namespace for default parameters for the 
    /// AntennaParabolic module
    ///
    namespace AntennaParabolicDefaults
    {
        
        /// Default diameter in meters
        static const double Diameter = 0.66;
        
        /// Default antenna efficiency $0\leq Efficiency \leq 1$
        static const double Efficiency = 0.65;
        
        /// Default temperature $T \geq 0$ in K
        static const double Temperature = 20;
        
        /// Default azimuth error in radians
        static const double BoresightAzimuthError = 0.0;
        
        /// Default elevation error in radians
        static const double BoresightElevationError = 0.0;
        
        /// Default RX Polarization
        static const Polarization RxPolarization = PolarizationVLP;
        
        /// Default TX Polarization
        static const Polarization TxPolarization = PolarizationVLP;
        
    }


    ///
    /// @class AntennaParabolic
    ///
    /// @brief Class definition file for a parabolic antenna
    ///
    
    class  AntennaParabolic : public ProcessingStage, public Gain
    {
        /// Diameter of antenna in meters
        double m_diameter;
        
        /// Efficiency of the antenna in a ratio (0 < eff <= 1)
        double m_efficiency;
        
        /// Log efficiency of antenna $log(efficiency) \leq 0$
        double m_logEfficiency;
        
        ///  Antenna temperature in K
        double m_taInK;
        
        /// Antenna temperature in $10 log_{10} K$
        double m_taInDb;
        
        ///
        /// A flag indicating whether fast (log parabolic) lookup is 
        /// enabled
        ///
        
        bool m_fastLookupIsEnabled; 
        
        /// 
        /// @var m_boresighAzimuthError
        /// @brief Pointing errors (presently static)
        /// @var m_boresightElevationError
        /// @brief Pointing errors (presently static)

        double m_boresightAzimuthError;
        double m_boresightElevationError;
        
        ///
        /// Angular offset (in radians) from boresight in
        /// radial terms $r = |angular error|$
        /// 
        
        double m_pointingError;
        
        /// Current TX Polarization
        Polarization m_txPolarization;
        
        /// Current RX Polarization
        Polarization m_rxPolarization;
        
        ///
        /// @brief Transform static and dynamic parameters into
        /// calculated values within the model
        ///
        
        void deriveOperationalParameters()
        {
            m_logEfficiency = UTIL::abs2db(m_efficiency);
            
            m_pointingError = sqrt(m_boresightAzimuthError *
                                   m_boresightAzimuthError + 
                                   m_boresightElevationError *
                                   m_boresightElevationError);
            
            m_taInDb = UTIL::abs2db(m_taInK);
            
            m_fastLookupIsEnabled = true;
        }
        
        ///
        /// @brief Parse a text string into an enumerated
        /// polarization type
        ///
        /// @param str a string containing enumerated text data
        ///
        /// @returns the polarization type as an enumerated value
        ///
        
        static Polarization parsePolarizationString(std::string str)
        {
            if (str == "VLP")
            {
                return PolarizationVLP;
            }
            else if (str == "HLP")
            {
                return PolarizationHLP;
            }
            else if (str == "LHCP")
            {
                return PolarizationLHCP;
            }
            else if (str == "RHCP")
            {   
                return PolarizationRHCP;
            }
            else 
            {
                ERROR_ReportError("The PhySattsm PHY model has"
                                  " encountered an unknown polarization"
                                  " and cannot initialize the parabolic"
                                  " antenna module.  Please check the"
                                  " configuration file.  The polarization"
                                  " type must be either HLP, VLP,"
                                  " LHCP or RHCP");
            }
            
            /* NOTREACHED */
            
            return PolarizationUnspecified;
        }
        
        ///
        /// @brief Calculate the maximum gain for this antenna
        ///
        /// @param freq the frequency of interest
        ///
        /// @returns the maximum gain for this antenna before
        /// offset losses in dBi
        ///
        
        double getMaxGain(double freq)
        {
            const double l4pi = 
                10.0 * (double)log10(UTIL::Constants::FourPi);
         
            double wavelength = UTIL::Constants::C / freq;
            double dl = m_diameter / wavelength;
            
            double peak = m_logEfficiency * l4pi + 20.0 * log10(dl);
            
            return peak;
        }
        
        ///
        /// @brief Calculate the gain for this antenna at a 
        /// given frequency and offset angle from boresight
        ///
        /// @param offAxisAngle the angular error from feed
        /// boresight
        /// @param freq the frequency of interest
        ///
        /// @returns the calculated gain of this antenna in dBi
        ///
        
        double getGain(double offAxisAngle,
                       double freq)
        {
            double lf = UTIL::Constants::C / freq;
            double dl = m_diameter / lf;
            
            double peak = getMaxGain(freq);
            
            // 
            // Get the half power beam width.  Taken from Kraus Antennas p. 570
            // Also get the peak directivity.
            // 
            
            double hpbw = 58.0 / dl;
            
            double ga(0);
            if (m_fastLookupIsEnabled && offAxisAngle < hpbw / 2.0)
            {                
                double a0  = peak;
                double a1 = 0.0;
                double a2 = -3.0 / 3364.0 * dl * dl;
                
                ga = a0 + a1 * offAxisAngle
                    + a2 * offAxisAngle * offAxisAngle;
                
            }
            else
            {                
                double c = lf / (UTIL::Constants::Pi * m_diameter);
                
                double phi = UTIL::deg2rad(offAxisAngle);
                double sinphi = sin(phi);
                
                assert(sinphi != 0.0);
                
                // 
                // Note that 'e' is actually the gain differential from maximum.  
                // It is important to verify that the results are valid and 
                // that domain of the input is proper to accurately return 
                // results.  
                
                double e = 2.0 * c * j1(sinphi / c) / sinphi;
                
                if (e == 0.0)
                    return UTIL::Constants::Log0;
                
                ga = peak - UTIL::abs2db(e);
            }
            
            return ga;
        }
        
        ///
        /// @brief Calculate the offset angle given the current
        /// orientation of the antenna and node
        ///
        /// @param o the orientation of the received signal
        ///
        /// @returns the boresight error value
        ///
        /// Currently assumes that the antenna perfectly tracks
        ///
        
        double getOffsetAngle(Orientation o)
        {
            return 0;
        }
                
    public:
        
        /// 
        /// @brief Return current output power in W
        ///
        /// @param the current input signal
        ///
        /// @returns current output power in W
        ///
        /// @see Gain
        ///
            
        double getOutputPower(SignalRecord& srec)
        {
            double inputPower = srec.getInputPower();
            double offsetAngle = 
                getOffsetAngle(srec.getInputDirection());
            
            AnalogHeader h(srec.m_msg);
            
            double gain = getGain(offsetAngle,
                                  h.carrierFrequency);
            
            double outputPower = gain * inputPower;
            
            return outputPower;
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
        
        AntennaParabolic(Node* node,
                         int phyIndex,
                         Address* nodeAddress,
                         const NodeInput* nodeInput)
        {
            double temp_dbl;
            BOOL wasFound(FALSE);
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-ANTENNA-PARABOLIC-DIAMETER",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_diameter = AntennaParabolicDefaults::Diameter;
            }
            else
            {
                m_diameter = temp_dbl;
            }
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-ANTENNA-PARABOLIC-EFFICIENCY",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_efficiency = AntennaParabolicDefaults::Efficiency;
            }
            else
            {
                m_diameter = temp_dbl;
            }
            
            char buf[MAX_STRING_LENGTH];
            
            IO_ReadString(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-ANTENNA-PARABOLIC-RX-POLARIZATION",
                          &wasFound,
                          buf);
            
            if (wasFound == FALSE)
            {
                m_rxPolarization = 
                    AntennaParabolicDefaults::RxPolarization;
            }
            else
            {
                m_rxPolarization = parsePolarizationString(buf);
            }
            
            IO_ReadString(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-ANTENNA-PARABOLIC-TX-POLARIZATION",
                          &wasFound,
                          buf);
            
            if (wasFound == FALSE)
            {
                m_txPolarization = 
                    AntennaParabolicDefaults::RxPolarization;
            }
            else
            {
                m_txPolarization = parsePolarizationString(buf);
            }
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-ANTENNA-PARABOLIC-TEMPERATURE",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_taInK = AntennaParabolicDefaults::Temperature;
            }
            else
            {
                m_taInK = temp_dbl;
            }
            
            m_boresightAzimuthError = 0;
            m_boresightElevationError = 0;
            
            deriveOperationalParameters();
        }
        
        /// 
        /// @brief Constructor given direct parameterization
        ///
        /// @param diameter the current diameter of the dish in meters
        /// @param efficiency the current efficiency of the dish
        /// @param rx_polarization the current receive polarization
        /// of the dish
        /// @param tx_polarization the current transmit polarization
        /// of the dish
        /// @param taInK the current sky noise temperature of the
        /// dish
        ///
        
        AntennaParabolic(double diameter = 
                         AntennaParabolicDefaults::Diameter,
                         double efficiency = 
                         AntennaParabolicDefaults::Efficiency,
                         Polarization rx_polarization = 
                         AntennaParabolicDefaults::RxPolarization,
                         Polarization tx_polarization =
                         AntennaParabolicDefaults::TxPolarization,
                         double taInK =
                         AntennaParabolicDefaults::Temperature)
            : m_diameter(diameter), m_efficiency(efficiency),
              m_taInK(taInK),
              m_txPolarization(m_txPolarization),
              m_rxPolarization(rx_polarization)
        {
                deriveOperationalParameters();
        }
        
        /// 
        /// @brief Destructor for the class
        ///
        
        ~AntennaParabolic()
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
                    int phyIndex,
                    Message* msg, 
                    bool& ok)
        {
            AnalogHeader h(msg);
            
            double gain = getMaxGain(h.carrierFrequency);
            
            h.polarization = m_txPolarization;
            
            PhySattsmAnalogAmplify(h, gain);
            
            if (DEBUG)
            {
                printf("Max Gain of Antenna is %e", gain);
                printf("Packet signal power is %e", h.signalPower);
                printf("Noise power is %e", h.noisePower);
            }

            ok = (h.analogError == 0);
            
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
                       int phyIndex,
                       Message* msg,
                       bool& ok,
                       SignalRecord& srec)
        {
            // 
            // Processing commands
            // 
            // 
            // Verify that the receiver polarization is what was expected.  
            // 
            // FUTURE: There should be a unified way for SatTsm to destroy a 
            // packet so it can be safely logged and traced.  Right now, 
            // it just sets a flag in the header that this packet cannot be
            // received
            // and so it can be destroyed by whomever does that down the 
            // reception chain.
            //
            
            AnalogHeader h(msg);
            
            if (h.polarization != m_rxPolarization)
            {
                if (DEBUG)
                {
                    printf("Dropping packet due to polarization"
                           " mismatch at RX");
                }
                    
                h.analogError |= AnalogErrorPolarizationMismatchAtRx;
            }
            
            h.signalPower = srec.getSignalPower();
            
            h.noisePower += srec.getNoisePower();
            h.interferencePower += srec.getInterferencePower(*this);
            
            double gain = getMaxGain(h.carrierFrequency);
            
            if (DEBUG)
            {
                printf("Ta=%e, BW=%e", 
                       m_taInK, 
                       h.bandwidth);
            }
            
            PhySattsmAnalogInjectNoiseByTemp(h, m_taInK);
            PhySattsmAnalogAmplify(h, gain);
            
            if (DEBUG)
            {
                printf("Max Gain of Antenna is %e", gain);
                printf("Packet signal power is %e", h.signalPower);
                printf("Noise power is %e", h.noisePower);
            }

            ok = (h.analogError == 0);
                        
            h.write(msg);

        }
        
        ///
        /// @brief Request to set the current RX polarization of the
        /// dish
        ///
        /// @param rxp the proposed new polarization
        ///
        
        void setRxPolarization(Polarization rxp)
        {
            m_rxPolarization = rxp;
        }
        
        ///
        /// @brief Query the current RX polarization of the dish
        ///
        /// @returns the current RX polarization of the dish
        ///
        
        Polarization getRxPolarization()
        {
            return m_rxPolarization;
        }
        
        ///
        /// @brief Request to set the current TX polarization of the
        /// dish
        ///
        /// @param rxp the proposed new polarization
        ///
        
        void setTxPolarization(Polarization txp)
        {
            m_txPolarization = txp;
        }
        
        ///
        /// @brief Query the current TX polarization of the dish
        ///
        /// @returns the current TX polarization of the dish
        ///
        
        Polarization getTxPolarization()
        {
            return m_txPolarization;
        }
        
        ///
        /// @brief Query the current pointing error of the dish
        ///
        /// @returns the current pointing error of the dish
        ///
        
        double getPointingError()
        {
            return m_pointingError;
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
#endif                          /* __SatPhy_Antenna_H__ */
