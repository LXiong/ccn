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
#ifndef __SATTSM_ANALOG_HEADER_H__
# define __SATTSM_ANALOG_HEADER_H__

#include "afe-base.h"
#include "viewpoint.h"

namespace PhySattsm
{

    /// @brief ChannelData Information on the channel state
    /// 
    /// @class ChannelData
    ///
    /// This class represents the an individual channel on the
    /// system.  
    ///
    struct ChannelData 
    {
        /// The sequence of the channel data (unique identifier)
        int seq;
        
        /// The number of transmitters simultaneously transmitting
        int simultaneousTransmitters;
        
        ///
        /// The number of transmitters that are sending in adjacent
        /// channels
        ///
        
        int adjacentTransmitters;
        
        /// The total number of transmitters in this channel
        int totalTransmitters;
        
        /// The time that the signal enters channel
        clocktype timeEnterChannel;
        
        /// The time that the signal exits the channel
        clocktype timeExitChannel;
    } ;
    
    ///
    /// @brief A header that is used to model analog effects in the
    /// SATTSM PHY
    ///
    /// @class AnalogHeader
    ///
    /// This structure is prepended to each packet passing through
    /// the PHY to store analog effects for later processing.
    ///

    struct AnalogHeader 
    {
        /// A sequence number for the header (unique identifier)
        long seq;
        
        /// The simulation time a transmission starts
        clocktype transmissionStartTime;
        
        /// The simulation time a transmission ends
        clocktype transmissionEndTime;
        
        /// The polarization of this transmission
        Polarization polarization;
        
        /// The carrier (center) frequency of this transmission
        double carrierFrequency;
        
        /// The bandwidth (in Hz) of this signal
        double bandwidth;
        
        /// The signal power of this signal in dBW
        double signalPower;
        
        /// The noise power of this signal in dBW
        double noisePower;
        
        /// The net interference power in this signal in dBW
        double interferencePower; 
        
        ///
        /// A data structure representing the geometric point of
        /// origin for this signal
        ///
        SolidPath entryPoint;
        
        /// Channel conditions encountered by this signal
        ChannelData channel;
        
        /// The analog error flag
        unsigned int analogError;
        
        /// The dispersion cached by this signal
        double cacheDispersion;
        
        ///
        /// @brief Deserialize this message into the given
        /// AnalogHeader
        ///
        /// @param h A pointer to an empty AnalogHeader data 
        /// structure
        /// @param msg A pointer to a message that contains a 
        /// serialized AnalogHeader in the outermost 
        /// memory position
        ///
        static void read(AnalogHeader* h, 
                         Message* msg)
        {
            memcpy((void*)h,
                   MESSAGE_ReturnPacket(msg),
                   sizeof(AnalogHeader));
            
        }
        
        ///
        /// @brief Serialize the given AnalogHeader into the
        /// given Message data structure
        /// 
        /// @param h A pointer to an AnalogHeader that is to be
        /// serialized to the Message data structure
        /// @param msg A pointer with sufficient memory available
        /// in the outmost header to accommodate the information
        /// in the analog header data structure
        ///
        
        static void write(AnalogHeader* h,
                          Message* msg)
        {
            memcpy(MESSAGE_ReturnPacket(msg),
                   (void*)h,
                   sizeof(AnalogHeader));
        }
        
        ///
        /// @brief Deserialize the message into the current
        /// AnalogHeader
        ///
        /// @param msg A pointer containing an AnalogHeader
        /// in the outermost protocol header
        ///
        /// @see read(AnanlogHeader*, Message*)
        ///
        
        void read(Message* msg)
        {
            read(this, msg);
        }
        
        ///
        /// @brief Serialize the current AnalogHeader into the 
        /// given Message
        ///
        /// @param msg A pointer with sufficient memory available
        /// in the outmost header to accommodate the information
        /// in the analog header data structure
        ///
        /// @see read(AnalogHeader*, Message*);
        ///
        
        void write(Message* msg)
        {
            write(this, msg);
        }
        
        /// 
        /// @brief Construct an AnalogHeader data structure from the
        /// given message.
        ///
        /// @param msg A pointer to a message that contains a 
        /// serialized AnalogHeader in the outermost 
        /// memory position
        ///
        
        AnalogHeader(Message* msg)
        {
            read(msg);
        }
        
        ///
        /// @brief Construct an empty AnalogHeader
        ///
        
        AnalogHeader() 
        {
            
        }
        
        ///
        /// @brief Return the Signal To Noise Ratio (SNR) given
        /// current header data
        ///
        /// @returns The SNR raio in dB
        ///
        
        double snr()
        {
            double nPlusI = NON_DB(interferencePower)
            + NON_DB(noisePower);
            double c = NON_DB(signalPower);
            
            return IN_DB(c / nPlusI);
        }
        
        ///
        /// @brief Calculates the error status of the current
        /// AnalogHeader
        ///
        /// @returns True if the frame referenced by this header
        /// is errored.  False otherwise
        ///
        
        bool isErrored()
        {
            return analogError != 0;
        }
        
    } ; 
}


#endif /* __SATTSM_ANALOG_HEADER_H__ */

