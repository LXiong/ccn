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

#include "prop_suburban.h"

#define DEBUG 0

// /**
// FUNCTION :: PROP_Pathloss_Suburban
// LAYER :: Physical Layer.
// PURPOSE :: Calculate SUburban Propagation Loss
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
double PROP_Pathloss_Suburban(double frequency_MHz,
                              double waveLength,
                              double distance,
                              double h_base_station,
                              double h_mobile,
                              SuburbanTerrainType TerrainType)
{
    double pathloss_dB = 0.0;
    double gamma = 0.0;
    double d0 = 0.0;
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
    double Xf = 0.0;
    double Xh = 0.0;
    double s = 0.0;

    //Freespace propagation in 1st FresnalZone
    d0 =  DEFAULT_FREESPACE_PATHLOSS_DISTANCE_METER; //in meters

    if (h_mobile==0.) {
        h_mobile = VERY_SMALL_ANTENNA_HEIGHT;
    }

    if (h_base_station==0.) {
        h_base_station = VERY_SMALL_ANTENNA_HEIGHT;
    }


    if (distance < DEFAULT_FREESPACE_PATHLOSS_DISTANCE_METER)
    {
        pathloss_dB = PROP_PathlossFreeSpace(distance, waveLength);

        if (DEBUG) {
            printf("Suburban: distance < "
                   "DEFAULT_FREESPACE_PATHLOSS_DISTANCE_METER.\n");
        }
        return pathloss_dB;
    }
    else {
        pathloss_dB = PROP_PathlossFreeSpace(d0, waveLength);
    }
    switch (TerrainType)
    {
    case HILLY_TERRAIN_WITH_MOD_TO_HEAVY_TREE_DENSITY:
    {
        a = 4.6;
        b = 0.0075;
        c = 12.6;
        Xh = -10.8 * log10(h_mobile/2.);
    }

    case FLAT_TERRAIN_WITH_MOD_TO_HEAVY_TREE_DENSITY:
    {
        a = 4.0;
        b = 0.0065;
        c = 17.1;
        Xh = -10.8 * log10(h_mobile/2.);
    }

    case FLAT_TERRAIN_WITH_LIGHT_TREE_DENSITY:
    {
        a = 3.6;
        b = 0.005;
        c = 20.0;
        Xh = -20. * log10(h_mobile/2.);
    }
    }

    gamma = a - b * h_base_station + c/h_base_station;
    Xf = 6. * log10(frequency_MHz/2000.);

    /* This value if required, will be accpunted for using
    the SHADOWING MODEL  that is set separately */
    s = 0.;

    if (distance > d0) {
        pathloss_dB = pathloss_dB + 10 * gamma * log10(distance/d0)
                      + Xf + Xh + s;
    }
    if (DEBUG) {
        printf("Suburban pathloss = %f\n", pathloss_dB);
    }
    return pathloss_dB;
}


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
                         const NodeInput *nodeInput)
{
    char terrainType[MAX_STRING_LENGTH];
    BOOL wasFound_type;
    BOOL wasFound_veg_percent;
    double val;

    PropProfile* propProfile = propChannel->profile;

    IO_ReadStringInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-TERRAIN-TYPE",
        channelIndex,
        (channelIndex == 0),
        &wasFound_type,
        terrainType);

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-PERCENT-AREA-COVERED-BY-VEGETATION",
        channelIndex,
        TRUE,
        &wasFound_veg_percent,
        &val);

    //Default value of suburban terrain if no values are set
    if (!wasFound_veg_percent && !wasFound_type) {
        propProfile->suburbanTerrainType =
            FLAT_TERRAIN_WITH_MOD_TO_HEAVY_TREE_DENSITY;

        if (DEBUG) {
            printf("Init suburban models complete - using "
                   "default settings.\n");
        }

        return;
    }


    //Only one value found
    if (!wasFound_veg_percent || !wasFound_type) {
        ERROR_ReportError("TERRAIN-TYPE or "
                          "PERCENT-AREA-COVERED-BY-VEGETATION missing.\n");
    }

    if (wasFound_veg_percent) {
        ERROR_Assert((val > 0) && (val <=100),
                     "Percent area covered by vegetation is invalid.\n");
    }

    if (strcmp(terrainType, "FLAT")==0)
    {
        if ( val < DEFAULT_VEG_CUTOFF_PERCENT) {
            propProfile->suburbanTerrainType =
                FLAT_TERRAIN_WITH_LIGHT_TREE_DENSITY;
        }
        else {
            propProfile->suburbanTerrainType =
                FLAT_TERRAIN_WITH_MOD_TO_HEAVY_TREE_DENSITY;
        }
    }
    //No need to check percent vegtation
    else if (strcmp(terrainType, "HILLY")==0)
    {
        propProfile->suburbanTerrainType =
            HILLY_TERRAIN_WITH_MOD_TO_HEAVY_TREE_DENSITY;
    }
    else {
        ERROR_ReportError("Invalid Terrain-type.\n");
    }

    if (DEBUG) {
        printf("Init suburban models complete.\n");
    }
    return;
}

