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
#ifndef __SATTSM_ANALOG_BASE_H__
# define __SATTSM_ANALOG_BASE_H__

namespace PhySattsm
{
    struct State; 
    
    /// An enumerated set of EM polarizations
    enum Polarization 
    {
        PolarizationVLP = 0x0101,
        PolarizationHLP = 0x0102,
        PolarizationLHCP = 0x0201,
        PolarizationRHCP = 0x0202,
        PolarizationUnspecified = 0x0000
    };

    /// Count of enumerated polarizations (less unspecified)
    static const int CHANNEL_POLARIZATIONS = 4;
    
    /// Set of integer flags representing differnet types
    /// of analog errors
    
    static const int AnalogErrorNone = 0x0;
    static const int AnalogErrorPolarizationMismatchAtRx = 0x1;
    static const int AnalogErrorFrequencyErrorTooLarge = 0x2;
    static const int AnalogErrorSignalBelowReceiverThreshold = 0x4;
    
    ///
    /// @brief Gets the physical data for a given node and 
    /// interface number.
    ///
    /// @param node  A pointer to the node data structure
    /// @param phyNum The logical phyiscal layer entity 
    ///
    /// @returns A pointer to the local physical data 
    /// (generic) structure
    ///
    
    static PhyData* getPhyData(Node* node,
                               int phyNum)
    {
        return (PhyData*)(node->phyData[phyNum]);
    }
    
    ///
    /// @brief Returns the SATTSM data structure for the
    /// given node/phyNum enumerated interface
    ///
    /// @param node A pointer to the node data structure
    /// @param phyNum The logical interface index of the PHY
    /// entity
    ///
    /// @returns A SATTSM state entity
    ///
    
    static State* getSattsmData(Node* node,
                                int phyNum)
    {
        PhyData* phyData = getPhyData(node, phyNum);
        
        return (State*)phyData->phyVar;
    }
    
    ///
    /// @brief Set the PHY-specific data pointer in the
    /// generic PHY data structure
    ///
    /// @param node A pointer to the local node data structure
    /// @param phyNum The logical interface index of the local
    /// PHY entity
    /// @param state A pointer to the SATTSM state that is to 
    /// stored in the generic PHY data structure
    ///
    
    static void setSattsmData(Node* node,
                              int phyNum,
                              State* state)
    {
        PhyData* phyData = getPhyData(node, 
                                      phyNum);
        
        phyData->phyVar = (void*)state;
    }
}


#endif /* __SATTSM_ANALOG_BASE_H__ */
