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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "mobility_waypoint.h"

/*
 * FUNCTION     MOBILITY_WaypointInit
 * PURPOSE      Initialization function for random drunken waypoint model.
 *
 * Parameters:
 *     nodeId: node identifier
 *     mobilityData: pointer to mobilityData
 *     terrainData: pointer to terrainData
 *     nodeInput: structure containing contents of input file
 *     maxSimClock: maximum simulation time
 */
void MOBILITY_WaypointInit(
    NodeAddress nodeId,
    MobilityData* mobilityData,
    TerrainData* terrainData,
    NodeInput *nodeInput,
    clocktype maxSimClock)
{
    clocktype   simClock;
    Coordinates dest1;
    Coordinates dest2;
    Orientation orientation;

    clocktype mobilityPause;
    double    minSpeed;
    double    maxSpeed;
    char      buf[MAX_STRING_LENGTH];
    BOOL      wasFound;
#ifdef ADDON_BOEINGFCS
    double zValue = 0.0;
#endif

    // no orientation for this model right now.
    orientation.azimuth = 0;
    orientation.elevation = 0;

    /* Read the pause time after reaching destination */

    IO_ReadString(
        nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MOBILITY-WP-PAUSE",
        &wasFound,
        buf);

    assert(wasFound == TRUE);

    mobilityPause = TIME_ConvertToClock(buf);


    /* Read the speed arrange(Min,Max) */

    IO_ReadDouble(
        nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MOBILITY-WP-MIN-SPEED",
        &wasFound,
        &minSpeed);

    assert(wasFound == TRUE);

    IO_ReadDouble(
        nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MOBILITY-WP-MAX-SPEED",
        &wasFound,
        &maxSpeed);

    assert(wasFound == TRUE);

    simClock = 0;
    dest1 = mobilityData->current->position;

    while (simClock < maxSimClock) {
        double           distance;
        double           speed;

        dest2 = dest1;

        dest2.common.c1 =
            terrainData->getOrigin().common.c1 +
            (terrainData->getDimensions().common.c1 * RANDOM_erand(mobilityData->seed));

        dest2.common.c2 =
            terrainData->getOrigin().common.c2 +
            (terrainData->getDimensions().common.c2 * RANDOM_erand(mobilityData->seed));

        dest2.common.c3 = 0.0;
#ifdef ADDON_BOEINGFCS
    zValue = dest2.common.c3;
#endif
        if (terrainData->getCoordinateSystem() == LATLONALT) {
            COORD_NormalizeLongitude(&dest2);
        }

        // This model assumes that the third coordinate is always 0.0
        COORD_CalcDistance(
            terrainData->getCoordinateSystem(),
            &dest1, &dest2, &distance);


        speed = minSpeed
                + (RANDOM_erand(mobilityData->seed) * (maxSpeed - minSpeed));

        simClock += (clocktype)(distance / speed * SECOND);

        if (mobilityData->groundNode == TRUE) {
            TERRAIN_SetToGroundLevel(terrainData, &dest2);
        }
        MOBILITY_AddANewDestination(mobilityData,
                                    simClock,
                                    dest2,
                                    orientation
#ifdef ADDON_BOEINGFCS
                    , zValue
#endif
                    );

        simClock += mobilityPause;

        if (mobilityPause > 0) {
            MOBILITY_AddANewDestination(
                mobilityData,
                simClock,
                dest2,
                orientation
#ifdef ADDON_BOEINGFCS
                , zValue
#endif
        );
        }

        dest1 = dest2;
    }

    return;
}

