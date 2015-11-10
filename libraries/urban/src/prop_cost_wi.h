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

#ifndef PROP_COST_WI_H
#define PROP_COST_WI_H

#include "api.h"
#include "partition.h"
#include "prop_urban.h"

#define DEFAULT_PROPAGATION_COST231_WI_ENVIRONMENT METROPOLITAN
//DEFAULT_ROOF_HEIGHT = 3* Typical Number of floors in Urban environment
#define DEFAULT_ROOF_HEIGHT  3.*7
#define DEFAULT_BUILDING_SEPARATION 40.0
#define DEFAULT_STREET_WIDTH 20.0
#define DEFAULT_URBAN_PROP_MODEL FREESPACE
#define VERY_SMALL_STREET_WIDTH 0.5     //in meters
#define VERY_SMALL_ANTENNA_HEIGHT 0.001  //in meters, for outdoor urban
#define DEFAULT_NUM_OF_BUILDINGS_IN_PATH 2

// /**
// FUNCTION :: COST231_WIInitialize
// LAYER :: Physical Layer.
// PURPOSE :: Initialize COST 231 Walfish Ikegami Propagation Models
// PARAMETERS ::
// + propChannel: PropChannel* : Pointer to Propagation Channel data-struct
// + channelIndex: Int: Channel Index
// + nodeInput: NodeInput*: Pointer to Node Input
// RETURN :: void : NULL
//
// **/
void COST231_WIInitialize(PropChannel *propChannel,
                          int channelIndex,
                          const NodeInput *nodeInput);

// /**
// FUNCTION :: PathlossCOST231_WI
// LAYER :: Physical Layer.
// PURPOSE :: Calculate COST231 Walfish Ikegami Propagation Loss
// PARAMETERS ::
// + node: Node* : Pointer to Node structure
// + distance: double : Tx-Rx distance
// + wavelength: double : Wavelength
// + txAntennaHeight: float : Transmitter Antenna height
// + rxAntennaHeight: float : Receiver Antenna height
// + propProfile: PropProfile * : Propagation profile
// RETURN :: double : pathloss in dB
//
// **/
double PathlossCOST231_WI(Node *node,
                          double distance,
                          double waveLength,
                          float txAntennaHeight,
                          float rxAntennaHeight,
                          PropProfile *propProfile);


/* Loss in line-of-sight conditions */
double COST231_WI_LoS(double distanceKm, double frequencyMhz);

/* Loss in non-line-of-sight conditions */
double COST231_WI_NLoS (int NumOfBuildingsInPath,
                        double  distanceKm,
                        double frequencyMhz,
                        double h1,
                        double h2,
                        double orientation,
                        double roofHeight,
                        double streetWidth,
                        double buildingSeparation,
                        PropagationEnvironment environment);
/* Rooftop to Street Loss */
double COST231_WI_RooftoStreetLoss(double frequencyMhz,
                                   short incidentAngle,
                                   double h_ms,
                                   PropProfile *propProfile);

/* Multi-Screen Loss */
double COST231_WI_MultiScreenLoss(
    double distanceKm,
    double frequencyMhz,
    double h_bs,
    PropProfile *propProfile);


#endif

