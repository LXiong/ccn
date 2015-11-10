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
#ifndef __SATTSM_DIGITAL_H__
# define __SATTSM_DIGITAL_H__

#include "sattsm.h"

#include "afe-base.h"
#include "afe-header.h"

/// 
/// @file digital.h
///
/// @brief Digital communication class definition file
///

///
/// @namespace PhySattsm
///
/// @brief Default namespace for PHY SATTSM
/// 

namespace PhySattsm 
{
    /// @enum Modulation
    ///
    /// @brief An enumerated set of support modulation methods
    ///
    
    enum Modulation 
    {
        PhySattsmPSK = 0x101,
        PhySattsmQAM = 0x102
    };
    
    ///
    /// @struct ModulationInfo
    ///
    /// @brief A data structure that holds information on a modulation
    /// profile defined by the SATTSM PHY
    ///
    
    struct ModulationInfo 
    {
        /// The type of modulation employed
        PhySattsm::Modulation type;
        
        /// The number of bits per symbol (uncoded)
        double bitsPerSymbol;
        
        /// The excess bandwidth factor of the modulation waveform
        double excessBandwidth;
        
        /// The symbol rate of the modulator
        double baud;
    } ; 
    
    ///
    /// @struct CodingInfo
    ///
    /// @brief A data structure defined to hold information coding
    /// data for a code profile
    ///
    struct CodingInfo
    {
        /// A flag whether the coding module is block coded
        BOOL isBlockCoded;
        
        ///
        /// A flag whether the coding module can shorted the 
        /// last codeword of a transmission
        ///
        BOOL shortenedLastCodeword;
        
        /// The block size of the coding module
        int blockSize;
        
        ///
        /// The number of tail symbols required to clear the
        /// coder
        ///
        int tailSymbols;
        
        /// The overall code rate of the code module
        double rate;
        
        /// The symbol size of the coder in bits
        int symbolSize;             // bits
    } ;
    
    
    ///
    /// @struct PreambleInfo
    ///
    /// @brief A data structure defined to specify how 
    /// a premable signal is transmitted
    ///
    
    struct PreambleInfo
    {
        /// The modulation profile
        PhySattsm::ModulationInfo modulation;
        
        /// The coding profile
        PhySattsm::CodingInfo coding;
        
        /// The length of the preamble in (modulation) symbols
        int lengthInSymbols;
    } ;
    
    ///
    /// @struct PayloadInfo
    ///
    /// @brief A structure defined to specify how a given
    /// service data unit of the PHY is tranmitted
    ///
    
    struct PayloadInfo
    {
        /// Modulation profile for payload
        PhySattsm::ModulationInfo modulation;
        
        /// Coding profile for the payload
        PhySattsm::CodingInfo coding;
        
        /// The length of the payload in symbols
        int lengthInSymbols;
    } ;
    
    ///
    /// @brief A structure defining how the current frame
    /// is digitally modulated and coded from a digital
    /// communications perspective
    ///
    /// @struct DigitalHeader
    ///
    
    struct DigitalHeader
    {
        ///
        /// A flag indicating the current frame is a burst
        /// transmission
        ///
        BOOL isBurstTransmission;
        
        /// The preamble specification
        PreambleInfo preamble;
        
        /// The payload specification
        PayloadInfo payload;
        
        ///
        /// The total amount of time (including all guard
        /// times and ramp-up) required for transmission
        ///
        double totalEmissionTime;
        
        /// 
        /// @brief Deserialize the current message into a
        /// DigitalHeader data structure
        ///
        /// @param h A pointer to an empty and valid 
        /// DigitalHeader data structure
        /// @param msg A pointer to a Message data structure
        /// that contains a valid DigitalHeader in the outermost
        /// protocol header
        ///
        
        static void read(DigitalHeader* h, 
                         Message* msg)
        {
            memcpy((void*)h,
                   MESSAGE_ReturnPacket(msg),
                   sizeof(DigitalHeader));
            
        }
        
        ///
        /// @brief Serialize the given DigitalHeader into the
        /// given Message data structure
        /// 
        /// @param h A pointer to an DigitalHeader that is to be
        /// serialized to the Message data structure
        /// @param msg A pointer with sufficient memory available
        /// in the outmost header to accommodate the information
        /// in the digital header data structure
        ///
        
        static void write(DigitalHeader* h,
                          Message* msg)
        {
            memcpy(MESSAGE_ReturnPacket(msg),
                   (void*)h,
                   sizeof(DigitalHeader));
        }
        
        ///
        /// @brief Deserialize the message into the current
        /// DigitalHeader
        ///
        /// @param msg A pointer containing an DigitalHeader
        /// in the outermost protocol header
        ///
        /// @see read(DigitalHeader*, Message*)
        ///
        
        void read(Message* msg)
        {
            read(this, msg);
        }
        
        ///
        /// @brief Serialize the current DigitalHeader into the 
        /// given Message
        ///
        /// @param msg A pointer with sufficient memory available
        /// in the outmost header to accommodate the information
        /// in the digital header data structure
        ///
        /// @see read(DigitalHeader*, Message*);
        
        void write(Message* msg)
        {
            write(this, msg);
        }
        
        ///
        /// @brief Construct a DigitalHeader using information
        /// in the given Message data structure
        ///
        /// @param msg A pointer containing an DigitalHeader
        /// in the outermost protocol header
        ///
        
        DigitalHeader(Message* msg)
        {
            read(msg);
        }
    } ;
    
    namespace DigitalBurstRsvDefaults {
        
        /// Default premable length in symbols
        static int const PreambleLength = 128;
        
        /// Default Correction Capability of coder
        static int const ReedSolomonT = 8;
        
        ///  Default Symbol Size in bits
        static int const SymbolSize = 8; 
        
        /// Default number of tail symbols for modulator
        static int const TailSymbols = 4;
        
        /// Default excess bandwidth factor
        static double const ExcessBandwidthFactor = 0.50;
        
        /// Default Symbol Rate for modulator
        static double const SymbolRate = 128e3;
    }
    
    /// 
    /// @brief A concatenated Reed-Solomon Viterbi communications
    /// system
    ///
    /// @class DigitalBurstRsv
    ///
    
    class DigitalBurstRsv : public ProcessingStage
    {
        
    public:
        
        /// The preamble length in symbols
        int m_preambleLength;
        
        /// The RS T (error correcting capability) in symbols
        int m_rsT; 
        
        /// The maximum block size based on symbol size
        int m_maxBlockSize; 
        
        /// The RS symbol size (in bits)
        int m_symbolSize; 
        
        /// Number of bits per symbol in the modulator
        double m_bitsPerSymbol; 
        
        /// Number of tail symbols per transmission
        int m_tailSymbols; 
        
        /// Current excess bandwidth factor
        double m_excessBandwidthFactor;
        
        /// The current symbol rate (symbols/second)
        double m_symbolRate; 
        
        ///
        /// @brief Perform post-processing to calculate any
        /// operational parameters not specified dynamically
        /// or statically
        ///
        
        void deriveOperationalParameters()
        {
            // The maximum block size is the the maximum size of a GF(2^N)
            // code word (2^N-1) minutes two times the error correcting
            // capability of the code (2T).  Note that the code can
            // be shorted by k bytes and produce a (2^N-1-k, 2^N-1-2T-k) 
            // shortened code.  Each word is N bits long.
            
            m_maxBlockSize = (int) pow(2.0, 
                                       (double) m_symbolSize)
                - 2 * m_rsT - 1;     /* symbols */
        }
        
        ///
        /// @brief Constructor the burst RSV module
        /// based on the QualNet configuration file
        ///
        /// @param node A pointer to the local Node data structure
        /// @param ifidx The logical index of the local PHY process
        /// @param nodeAddress The network address of the local
        /// interface process.
        /// @param nodeInput An internal representation of the 
        /// QualNet configuration file
        ///
        
        DigitalBurstRsv(Node* node,
                        int ifidx,
                        Address* nodeAddress,
                        const NodeInput* nodeInput)
        {
            double temp_dbl(0);
            int temp_int(0);
            
            BOOL wasFound(FALSE);
            
            IO_ReadInt(node->nodeId,
                       nodeAddress,
                       nodeInput,
                       "PHY-SATTSM-PREAMBLE-LENGTH",
                       &wasFound,
                       &temp_int);
            
            if (wasFound == FALSE)
            {
                m_preambleLength 
                = DigitalBurstRsvDefaults::PreambleLength;
            }
            else
            {
                m_preambleLength = temp_int;
            }
            
            IO_ReadInt(node->nodeId,
                       nodeAddress,
                       nodeInput,
                       "PHY-SATTSM-REED-SOLOMON-T",
                       &wasFound,
                       &temp_int);
            
            if (wasFound == FALSE)
            {
                m_rsT 
                = DigitalBurstRsvDefaults::ReedSolomonT;
            }
            else
            {
                m_rsT = temp_int;
            }
            
            /*
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-REED-SOLOMON-N",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_rsN 
                = PhySattsmDigitalBurstRsvDefaultReedSolomonN;
            }
            else
            {
                m_rsN = (int)temp_dbl;
            }
             
            */
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-EXCESS-BANDWIDTH",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_excessBandwidthFactor 
                = DigitalBurstRsvDefaults::ExcessBandwidthFactor;
            }
            else
            {
                m_excessBandwidthFactor = temp_dbl;
            }
            
            IO_ReadDouble(node->nodeId,
                          nodeAddress,
                          nodeInput,
                          "PHY-SATTSM-SYMBOL-RATE",
                          &wasFound,
                          &temp_dbl);
            
            if (wasFound == FALSE)
            {
                m_symbolRate 
                = PhySattsm::DigitalBurstRsvDefaults::SymbolRate;
            }
            else
            {
                m_symbolRate = temp_dbl;
            }
            
            IO_ReadInt(node->nodeId,
                       nodeAddress,
                       nodeInput,
                       "PHY-SATTSM-TAIL-SYMBOLS",
                       &wasFound,
                       &temp_int);
            
            if (wasFound == FALSE)
            {
                m_tailSymbols 
                = PhySattsm::DigitalBurstRsvDefaults::TailSymbols;
            }
            else
            {
                m_tailSymbols = temp_int;
            }
            
            deriveOperationalParameters();
        }
        
        ///
        /// @brief Constructor for directly specified
        /// parameters
        ///
        /// @param premableLength The preamble length of 
        /// the burst transmitters
        /// @param rsT The error correcting capability of
        /// the RSV coder
        /// @param symbolSize The symbol size of the RS
        /// block coder
        /// @param tailSymbols The number of tail symbols
        /// required to clear the Viterbi encoder
        /// @excessBandwidthFactor The excess bandwidth
        /// factor ($0 \leq \alpha \leq 1$)
        /// @symbolRate The symbol rate of the transmitter
        ///
        
        DigitalBurstRsv(int preambleLength 
                            = DigitalBurstRsvDefaults::PreambleLength,
                        int rsT 
                            = DigitalBurstRsvDefaults::ReedSolomonT,
                        int symbolSize = 
                            DigitalBurstRsvDefaults::SymbolSize,
                        int tailSymbols = 
                            DigitalBurstRsvDefaults::TailSymbols,
                        double excessBandwidthFactor = 
                            DigitalBurstRsvDefaults::ExcessBandwidthFactor,
                        double symbolRate = 
                            DigitalBurstRsvDefaults::SymbolRate)
        : m_preambleLength(preambleLength),
          m_rsT(rsT),
          m_symbolSize(symbolSize),
          m_tailSymbols(tailSymbols),
          m_excessBandwidthFactor(excessBandwidthFactor),
          m_symbolRate(symbolRate)
        {
            deriveOperationalParameters();
        }
        
        ///
        /// @brief Destroy a DigitalBurstRsv data structure
        ///
        
        ~DigitalBurstRsv()
        {
            
        }
        
        ///
        /// @brief Estimate the coded packet size for a given
        /// unencapsulated packet
        ///
        /// @param msg A pointer to a message data structure
        /// containing the unencoded frame.
        /// @returns The estimated size of the transmission in
        /// native modulation symbols
        ///
        
        int estimateCodedPacketSize(Message* msg)
        {
            return estimateCodedPacketSize(MESSAGE_ReturnPacketSize(msg)
                                           * 8);
        }
        
        ///
        /// @brief Estimated a transmission size in bits for 
        /// a given size frame
        ///
        /// @param size The size of an unspecified frame
        /// @returns The estimated size of the tranmission in
        /// native modulation symbols
        ///
        
        int estimateCodedPacketSize(int size)
        {
            int maxBlockInBits = m_symbolSize * m_maxBlockSize;
            
            // 
            // Calculate the number of blocks
            // 
            
            int blocks = size / maxBlockInBits;
            
            // 
            // If the last block has any data, it should also
            // be protected.
            // 
            
            int lastBlockBits = size - blocks * maxBlockInBits;
            
            
            // 
            // Get the total number of bits
            // 
            
            int totalBits =
                blocks * (m_maxBlockSize + 2 * m_rsT) * m_symbolSize;
            
            if (lastBlockBits > 0)
            {
                totalBits += lastBlockBits + 2 * m_rsT * m_symbolSize;
            }
            
            // 
            // Calculate total number of symbols and then round up
            // if the total included a partial symbol.
            // 
            
            int totalSymbols 
                = (int) ((double) totalBits / m_bitsPerSymbol);
            
            if (totalBits - totalSymbols * m_bitsPerSymbol > 0)
                totalSymbols++;
            
            return totalSymbols;   
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
            int packetSize 
                = MESSAGE_ReturnPacketSize(msg) * 8; // bits;
                                    
            // 
            // Encapsulate the MAC packet with a physical layer frame
            // This frame contains both real data (like a preamble) and
            // also transmission status data.  It is all placed in the 
            // packet header for right now but could be placed in the
            // info field instead.
            // 
            
            MESSAGE_AddHeader(node,
                              msg, 
                              sizeof(DigitalHeader), 
                              TRACE_SATTSM);
            
            // 
            // Retrieve the new digital header.
            // 
            
            DigitalHeader hd(msg);
            
            // 
            // Set up the transmission.  Right now the RSV runs in 
            // burst mode.
            // 
            
            hd.isBurstTransmission = TRUE;
            
            // 
            // Preamble data can be set separately from payload data.
            // 
            
            hd.preamble.modulation.type = PhySattsmPSK;
            hd.preamble.modulation.bitsPerSymbol = m_bitsPerSymbol;
            
            hd.preamble.modulation.excessBandwidth 
                = m_excessBandwidthFactor;
            
            hd.preamble.modulation.baud = m_symbolRate;
            
            hd.preamble.coding.isBlockCoded = FALSE;
            hd.preamble.coding.shortenedLastCodeword = FALSE;
            
            hd.preamble.coding.blockSize = 0;
            hd.preamble.coding.tailSymbols = 0;
            hd.preamble.coding.rate = 1.0;
            hd.preamble.coding.symbolSize = 0;
            
            hd.preamble.lengthInSymbols = m_preambleLength;
            
            // 
            // Payload digital modulation information
            // 
            
            hd.payload.modulation.type = PhySattsmPSK;
            hd.payload.modulation.bitsPerSymbol = m_bitsPerSymbol;
            
            hd.payload.modulation.excessBandwidth 
                = m_excessBandwidthFactor;
            
            hd.payload.modulation.baud = m_symbolRate;
            
            hd.payload.coding.isBlockCoded = TRUE;
            hd.payload.coding.shortenedLastCodeword = TRUE;
            hd.payload.coding.blockSize = m_maxBlockSize;
            hd.payload.coding.tailSymbols = m_tailSymbols;
            hd.payload.coding.symbolSize = m_symbolSize;
            
            hd.payload.lengthInSymbols
                = estimateCodedPacketSize(packetSize);
            
            hd.totalEmissionTime =
                (double) hd.payload.lengthInSymbols
                / (double) hd.payload.modulation.baud
                + (double) hd.preamble.lengthInSymbols
                / (double) hd.preamble.modulation.baud
                + (double) m_tailSymbols
                / (double) hd.payload.modulation.baud;
            
            ok = true;
            
            hd.write(msg);
            
        }
        
        /// @brief Return transmission duration of an abstract
        /// size
        ///
        /// @param size The size of the abstract frame
        /// @returns The total transmission duration in the
        /// native QualNet time base
        ///
        
        clocktype getTransmissionDuration(int size)
        {
            int lengthInSymbols =
                estimateCodedPacketSize(size);
            
            double totalEmissionTime =
                (double) lengthInSymbols
                / (double) m_symbolRate
                + (double) m_preambleLength
                / (double) m_symbolRate
                + (double) m_tailSymbols
                / (double) m_symbolRate;
            
            return (clocktype)(totalEmissionTime * (double)SECOND);
        }
        
        /// 
        /// @brief Calculate the current transmit rate (uncoded)
        ///
        /// @returns the current uncoded (raw) transmission rate
        /// of the digital modulation module
        ///
        
        double getTransmitRate()
        {
            return m_bitsPerSymbol * m_symbolRate;
        }
        
        /// 
        /// @brief Calculate the current recieve rate of the channel
        /// in raw bit rate
        ///
        /// @returns the current receiver rate of the demodulator
        /// in raw (uncoded) bits/second
        ///
        double getReceiveRate()
        {
            return getTransmitRate();
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
            DigitalHeader dighdr(msg);
            
            AnalogHeader anahdr;
            memcpy((void*)&anahdr,
                   MESSAGE_ReturnInfo(msg, INFO_TYPE_DEFAULT),
                   sizeof(AnalogHeader));
            
            ok = true;
        }
        
        ///
        /// @brief Peform run-time statistics processing
        ///
        /// @param node a pointer to the current node data structure
        ///
        /// @see Processing Stage
        ///
        
        void onRuntimeStat(Node* node)
        {
            
        }
        
    } ;
}

#endif                          /* __SATTSM_DIGITAL_H__ */
