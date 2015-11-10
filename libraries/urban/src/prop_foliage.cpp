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


// /**
// FUNCTION :: PROP_calculate_avg_distance_thru_foliage.
// PURPOSE :: Calculate avgerage propagation distance due to foliage.
// PARAMETERS ::
// + AvgNumTreesPerMeter: double : Avg. number of trees per meter
// + AvgFoliageWidth: double : Avg width of foliage
// RETURN :: double : avg_distance_thru_foliage
//
// **/

#include "prop_foliage.h"



// /**
// FUNCTION :: PROP_calculate_avg_distance_thru_foliage
// LAYER :: Physical Layer.
// PURPOSE :: Calculate average distance through foliage
// LAYER :: Physical Layer.
// PARAMETERS ::
// + AvgNumTreesPerMeter: Propagation distance through foliage
// + AvgFoliageWidth: double : Carrier frequency in mega-Hz
// RETURN :: double : excess pathloss
//
// **/

double PROP_calculate_avg_distance_thru_foliage(
    double AvgNumTreesPerMeter,
    double AvgFoliageWidth)
{
    double avg_distance_thru_foliage = AvgNumTreesPerMeter
                                       * AvgFoliageWidth;

    return avg_distance_thru_foliage;
}


// /**
// FUNCTION :: PROP_Foliage_Attenuation_Exponential_Decay_Model
// LAYER :: Physical Layer.
// PURPOSE :: Calculate excess propagation loss due to foliage.
//      The model should be used for cases when the foliage
//      is laid out in a linear fashion for example along the road side.
//      This is the case when details about the foliage configuration
//      are not known.
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
    FoliatedState f_state)
{
    double excess_pathloss_dB;
    double frequency_GHz = frequency_MHz/1000.;

    //COST235 Model
    if (frequency_GHz < 11.2) {
        if (f_state == IN_LEAF) {
            excess_pathloss_dB = 15.6 * pow(frequency_MHz, -0.009)
                                 * pow(distance_through_foliage, 0.26);
        }
        else {
            excess_pathloss_dB = 26.6 * pow(frequency_MHz, -0.2)
                                 * pow(distance_through_foliage, 0.5);
        }
    }
    //Fitted ITU-R Model
    else {
        if (f_state == IN_LEAF) {
            excess_pathloss_dB = 0.39 * pow(frequency_MHz, 0.39)
                                 * pow(distance_through_foliage, 0.25);
        }
        else {
            excess_pathloss_dB = 0.37 * pow(frequency_MHz, 0.18)
                                 * pow(distance_through_foliage, 0.59);
        }
    }


    return excess_pathloss_dB;

}
