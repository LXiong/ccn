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

#ifndef PROP_FOLIAGE_H
#define PROP_FOLIAGE_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "terrain.h"

double PROP_calculate_avg_distance_thru_foliage(
    double AvgNumTreesPerMeter,
    double AvgFoliageWidth);

// /**
// FUNCTION :: PROP_Exponential_Decay_Foliage_Attenuation
// LAYER :: Physical Layer.
// PURPOSE :: Calculate excess propagation loss due to foliage.
//      The model should be used for cases when the foliage
//      is laid out in a linear fashion for example along the road side.
// LAYER :: Physical Layer.
// PARAMETERS ::
// + distance_through_foliage: Propagation distance through foliage
// + frequency_MHz: double : Carrier frequency in mega-Hz
// RETURN :: double : excess pathloss
//
// **/
double PROP_Foliage_Attenuation_Exponential_Decay_Model(
    double distance_through_foliage,
    double frequency_MHz,
    FoliatedState foliatedState);

#endif /* PROP_FOLIAGE_H */
