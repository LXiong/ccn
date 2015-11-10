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

#ifndef PROP_SUBURBAN
#define PROP_SUBURBAN

#include "propagation.h"
#include "prop_urban.h"

#define DEFAULT_VEG_CUTOFF_PERCENT 65
#define DEFAULT_FREESPACE_PATHLOSS_DISTANCE_METER 50 //in meters


// /**
// FUNCTION :: Suburban_Initialize
// LAYER :: Physical Layer.
// PURPOSE :: Initialize Suburban Propagation Models
// PARAMETERS ::
// + propChannel: PropChannel* : Pointer to Propagation Channel data-struct
// + channelIndex: Int: Channel Index
// + nodeInput: NodeInput*: Pointer to Node Input
// RETURN :: void : NULL
//
// **/
void Suburban_Initialize(PropChannel *propChannel,
                         int channelIndex,
                         const NodeInput *nodeInput);


// /**
// FUNCTION :: PROP_Pathloss_Suburban
// LAYER :: Physical Layer.
// PURPOSE :: Calculate Suburban Propagation Loss
// PARAMETERS ::
// + wavelength: double : Wavelength
// + frequency_MHz: double : Carrier frequency in mega-Hz
// + distance: double : Tx-Rx distance
// + h_base_station: double : Height of base-station
// + h_mobile: double : Height of Mobile
// + TerrainType: SuburbanTerrainType : Enum type indicating type of terrain
// RETURN :: double : pathloss
//
// **/
double PROP_Pathloss_Suburban(double wavelength,
                              double freq_MHz,
                              double distance,
                              double h_base_station,
                              double h_mobile,
                              SuburbanTerrainType TerrainType);


#endif
