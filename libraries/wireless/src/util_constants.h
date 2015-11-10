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
#ifndef __UTIL_CONSTANTS_H__
# define __UTIL_CONSTANTS_H__

#include "main.h"
#include "clock.h"

// #include "base.h"

/// 
/// @file util_constants.h
///
/// @brief Definition of common utility and scientific constants and
/// routines
///

/// Taken from Satellite Communications Systems Engineering, 2nd Edition
/// W.L. Pritchard, et al.
/// Prentice Hall, 1993
/// ISBN 0-13-791468-7
/// pgs. 518-519

///
/// @namespace UTIL
///
/// @brief Utility namespace
///

namespace UTIL
{
    ///
    /// @namespace Constants
    ///
    /// @brief Definition of utility and scientific constants
    ///
    
    namespace Constants
    {
        /// \f$4 \pi\f$
        static const double FourPi = 12.56637061;
        
        /// \f$\pi\f$
        static const double Pi = 3.14159265;
        
        /// \f$2 \pi \f$
        static const double TwoPi = 6.2831853;
        
        /// Earth radius \f$R_e \f$ in km
        static const double Re = 6378.137; 
        
        /// Speed of light \f$c \f$ in \f$\frac{m}{sec}\f$
        static const double C = 2.99792458e8;
        
        /// Boltzmann's constant \f$K_B\f$ in \f$\frac{J}{K}\f$
        static const double Kb = 1.3807e-23; // J/K
        
        ///
        /// @{
        /// Taken from Pritchard
        /// pg. 49
        ///
        
        /// GSO Orbit radius \f$r_{GSO}\f$ in km
        static const double GsoOrbitRadius = 35786.0;
        
        /// An approximation of \f$log 0\f$ in dB
        static const double Log0 = -120.0;
        
        /// A calculated value for \f$10 log_{10} K_B\f$
        static const double LogKb = -228.6;
        
        /// Room temperature \f$T_0\f$ in K
        static const double T0 = 290.0;
        
        /// @}
        
        /// @{
        ///
        /// Taken from Antennas, 2nd Edition
        /// John D. Kraus
        /// McGraw-Hill, 1988
        /// ISBN 0-07-035422-7
        /// back cover (more accurate values)
        
        /// Astronomical Unit Au in km
        static const double AU = 1.496e8;
        
        /// Mass of the earth in kg
        static const double EarthMass = 5.98e24;
        
        /// Charge of the electron \f$q_e\f$ in C
        static const double ElectronCharge = -1.602e-19;
        
        /// Electron rest mass \f$m_e\f$ in kg
        static const double ElectronRestMass = 9.10956e-31;
        
        /// Electron Charge Mass Ratio \f$\gamma_e\f$
        static const double ElectronChargeMassRatio = 1.756603e11;
        
        /// Flux Density in \f$\frac{W}{m^2 Hz}\f$
        static const double FluxDensity = 1e-26;
        
        /// Mass of the Hydrogen atom in kg
        static const double MassHydrogenAtom = 1.673e-27; 
        
        /// Hydrogen Rest Frequency \f$f_H\f$ in MHz
        static const double HydrogenRestFrequency = 1420.405;
        
        /// Light Year in km
        static const double LightYear = 9.4605e12;
        
        /// base of natural logarithm \f$e\f$
        static const double E = 2.718282;
        
        /// Average distance to moon from earth in km
        static const double MoonDistanceAvg = 380.0e3;
        
        /// Mass of moon in kg
        static const double MoonMass = 6.7e22;
        
        /// Average radius of moon in km
        static const double MoonRadiusAvg = 1.738e3;
        
        /// Value of one Parsec in km
        static const double Parsec = 3.1e13;
        
        /// \f$\mu_0\f$
        static const double Mu0 = 1256.63706144;
        
        /// \f$\epsilon_0\f$ in \f$\frac{F}{m}\f$
        static const double Eps0 = 8.854185e12;
        
        /// Planck's constant \f$h\f$
        static const double Plank = 6.62620e-34;
        
        /// Proton rest mass in kg
        static const double ProtonRestMass = 1.67261e-27;
        
        /// Intrinsic impedence \f$\eta_0\f$ in \f$\Omega\f$
        static const double IntrinsicImpedance = 376.7304;
        
        /// Average distance to sun from earth in km
        static const double SunDistanceAvg = 1.496e8;
        
        /// Mass of sun in kg
        static const double SolarMass = 2e30;
        
        /// Average radius of sun in km
        static const double SunRadiusAvg = 700e7; 
            
        /// Length of year in seconds
        static const double YearInSeconds = 3.1556925e7; 
        
        /// @}
    }

    ///
    /// @brief Convert dB level to absolute power
    ///
    /// @param dbpwr Power expressed in dB (dBW)
    /// @returns power in absolute terms (W)
    ///

    static double db2power(const double dbpwr)
    {
        return pow(10.0, dbpwr/10.0);
    }
    
    ///
    /// @brief Convert dB level to absolute volts
    ///
    /// @param dbvoltPower expressed in dB (dBV)
    /// @returns power in absolute terms (W)
    ///

    static double db2volt(const double dbvolt)
    {
        return pow(10.0, dbvolt/20.0);
    }
    
    ///
    /// @brief convert absolute power to dB
    ///
    /// @param pwr expressed in W
    /// returns power expressed in dBW
    ///

    static double power2db(const double pwr)
    {
        return 10.0 * log10(pwr);
    }
    
    ///
    /// @brief convert absolute power to dBV
    ///
    /// @param pwr expressed in W
    /// returns volts expressed in dBV
    ///

    static double volt2db(const double volts)
    {
        return power2db(volts * volts);
    }
    
    ///
    /// @brief Convert an absolute value to dB
    ///
    /// @param absolute absolute value
    /// @returns value expressed in dB
    ///

    static double abs2db(const double absolute)
    {
        return power2db(absolute);
    }
    
    /// 
    /// @brief Convert a dB value into an absolute value
    ///
    /// @param dbabs dB value
    /// @returns value expressed in absolute terms
    ///

    static double db2abs(const double dbabs)
    {
        return db2power(dbabs);
    }
    
    /// 
    /// @brief Adds two dB values
    ///
    /// @param la $l_a$ as dB
    /// @param lb $l_b$ as dB
    /// @returns absolute addition of $l_a$ and $l_b$ expressed
    /// in absolute terms for addition but return as a dB value
    ///

    static double dbAddAbs(const double la, const double lb)
    {
        double a = db2abs(la);
        double b = db2abs(lb);

        return abs2db(a + b);
    }
    
    /// 
    /// @brief Adds two dB values as volts
    ///
    /// @param la $l_a$ as dBV
    /// @param lb $l_b$ as dBV
    /// @returns absolute addition of $l_a$ and $l_b$ expressed
    /// in absolute terms for addition but return as a dB value
    ///

    static double dbAddVolt(const double la, const double lb)
    {
        double a = db2volt(la);
        double b = db2volt(lb);

        return volt2db(a + b);
    }
    
    /// 
    /// @brief Adds two dB values inversely
    ///
    /// @param la $l_a$
    /// @param lb $l_b$
    /// @returns absolute inverse addition of $l_a$ and $l_b$ expressed
    /// in absolute terms for addition but return as a dB value
    ///

    static double dbAddInverseAbs(const double la, const double lb)
    {
        double a = db2abs(la);
        double b = db2abs(lb);

        return abs2db(1.0/a + 1.0/b);
    }
    
    /// 
    /// @brief Adds two dB values inversely as volts
    ///
    /// @param la $l_a$ in dBV
    /// @param lb $l_b$ in dBV
    /// @returns absolute inverse addition of $l_a$ and $l_b$ expressed
    /// in absolute terms for addition but return as a dBV value
    ///

    static double dbAddInverseVolt(const double la, const double lb)
    {
        double a = db2volt(la);
        double b = db2volt(lb);

        return volt2db(1.0/a + 1.0/b);
    }
    
    ///
    /// @brief Convert a degree angle into its equivalent
    /// radian value
    ///
    /// @param deg the value expressed in degrees
    /// @returns the value expressed in radians
    ///

    static double deg2rad(const double deg)
    {
        return Constants::Pi * deg / 180.0;
    }
    
    ///
    /// @brief Convert a radian angle into its equivalent
    /// degree value
    ///
    /// @param rad the value expressed in radians
    /// @returns the value expression in degrees
    ///

    static double rad2deg(const double rad)
    {
        return 180.0 * rad / Constants::Pi;
    }
    
    ///
    /// @brief Convert a noise figure into temperature
    ///
    /// @param nf a noise figure expressed in dB
    /// @returns the noise figure expressed in temperature (K)
    ///

    static double nf2t(const double nf)
    {
        double enf = db2abs(nf);

        return (1.0 + enf) * Constants::T0;
    }
    
    ///
    /// @brief Convert a temperature into a noise figure
    ///
    /// @param t a value expressed in temperature (K)
    /// @returns the noise figure
    ///

    static double t2nf(const double t)
    {
        double f = t / UTIL::Constants::T0 - 1.0;

        return abs2db(f);
    }
    
    ///
    /// @brief Returns the noise power in a band with equivalent
    /// temperature
    ///
    /// @param t the equivalent temperature of the spectrum in K
    /// @param bw the size of the bandwidth expressed in Hertz
    /// @returns the noise power in W
    ///

    static double noisePowerFromT(const double t, const double bw)
    {
        double lt = abs2db(t);
        double lbw = abs2db(bw);

        return lt + lbw + Constants::LogKb;
    }
    
    /// 
    /// @brief Convert a double time to a clocktype
    ///
    /// @param t the time expressed in seconds
    /// @returns the time expressed in QualNet time quanta
    ///

    static clocktype double2clocktype(double t)
    {
        clocktype tp = (clocktype)(t * (double)SECOND);

        return tp;
    }
    
    ///
    /// @brief Convert the QualNet quanta instant to a time in
    /// seconds
    ///
    /// @param tp the current QualNet time quanta
    /// @returns the simulation time in seconds
    ///

    static double clocktype2double(clocktype tp)
    {
        double t = (double)tp  / (double)SECOND;
        return t;
    }
    
}

#endif /* __UTIL_CONSTANTS_H__ */
