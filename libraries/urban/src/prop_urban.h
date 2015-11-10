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

#ifndef PROP_URBAN
#define PROP_URBAN

#include "api.h"
#include "partition.h"

#include "prop_data.h"
#include "prop_cost_wi.h"
#include "prop_itu_r.h"

/****************** Urban Propagation Model Definitions **********/

#define DEFAULT_URBAN_PROPAGATION_ENVIRONMENT METROPOLITAN

#define PI_SQUARE pow(PI,2)

// /**
// ENUM        :: UrbanModelType
// DESCRIPTION :: Different types of Urban propagation pathloss models.
// **/

enum UrbanModelType {
    FREESPACE,
    TWORAY,
    ITU_R_LOS,
    ITU_R_NLOS1,
    ITU_R_NLOS2,
    ITU_R_UHF_LOS,
    ITU_R_UHF_NLOS,
    ITU_R_VHFUHF,
    URBAN_COST_WI_LoS,
    URBAN_COST_WI_NLoS,
    URBAN_STREET_MICROCELL_LoS,
    URBAN_STREET_MICROCELL_NLoS,
    URBAN_STREET_M_TO_M,
    COST231_INDOOR,
    ITU_R_INDOOR
};


// /**
// FUNCTION :: UrbanProp_Initialize
// LAYER :: Physical Layer.
// PURPOSE :: Initialize Urban Propagation Models
// PARAMETERS ::
// + propChannel: PropChannel* : Pointer to Propagation Channel data-struct
// + channelIndex: Int: Channel Index
// + nodeInput: NodeInput*: Pointer to Node Input
// RETURN :: void : NULL
//
// **/
void UrbanProp_Initialize(PropChannel *propChannel,
                          int channelIndex,
                          const NodeInput *nodeInput);

// /**
// FUNCTION :: Pathloss_UrbanProp
// LAYER :: Physical Layer.
// PURPOSE :: Calculate Urban Propagation Pathloss
//            by calling environment specific model
// PARAMETERS ::
// + node: Node* : Pointer to Node
// + waveLength: double: waveLength
// + txAntennaHeight: float: Transmitter Antenna Height
// + rxAntennaHeight: float: Receiver Antenna Height
// + propProfile: PropProfile *: Pointer to propagation profile
// + pathProfile: PropPathProfile *: Pointer to propagation path profile
// RETURN :: double : Pathloss value in dB
//
// **/
double Pathloss_UrbanProp(const Node *node,
                          const NodeId txNodeId,
                          const NodeId rxNodeId,
                          const double waveLength,
                          const float txAntennaHeight,
                          const float rxAntennaHeight,
                          PropProfile *propProfile,
                          const PropPathProfile *pathProfile);

double Pathloss_Foliage(const Node *node,
                        const double distance,
                        const double wavelength,
                        Coordinates fromPosition,
                        Coordinates toPosition,
                        const float txAntennaHeight,
                        const float rxAntennaHeight,
                        PropProfile *propProfile);

// /**
// FUNCTION :: StreetMicrocell_Initialize
// LAYER :: Physical Layer.
// PURPOSE :: Initialize Street Microcell model
// PARAMETERS ::
// + propChannel: PropChannel * : Pointer to propagation channel
// + channelIndex: int : Channel Index
// + nodeInput:  NodeInput * : Node input
// RETURN :: void : void value
//
// **/
void StreetMicrocell_Initialize(PropChannel *propChannel,
                                int channelIndex,
                                const NodeInput *nodeInput);

// /**
// FUNCTION :: Pathloss_StreetMicrocell_LoS
// LAYER :: Physical Layer.
// PURPOSE :: Propagation pathloss for LoS, Street Microcell model
// PARAMETERS ::
// + h1: double : Height of node first node
// + h2: double : Height of node second node
// + waveLength: double : wavelength
// + distance: double : distance between nodes or from wall corner
//          (when called from Pathloss_StreetMicrocell_NLoS())
// RETURN :: double : Pathloss value
//
// **/
double Pathloss_StreetMicrocell_LoS(double h1,
                    double h2,
                    double waveLength,
                    double distance);

// /**
// FUNCTION :: Pathloss_StreetMicrocell_NLoS
// LAYER :: Physical Layer.
// PURPOSE :: Propagation pathloss for NLoS, Street Microcell model
// PARAMETERS ::
// + h1: double : Height of node first node
// + h2: double : Height of node second node
// + waveLength: double : wavelength
// + txDistanceToBuilding: double : Transmitter distance from building corner
// + rxDistanceToBuilding: double : Receiver distance from building corner
// + distanceThruBuilding: double : Distance through building
// RETURN :: double : Pathloss value
//
// **/
double Pathloss_StreetMicrocell_NLoS(double h1,
                                     double h2,
                                     double waveLength,
                                     double txDistanceToBuilding,
                                     double rxDistanceToBuilding,
                                     double distanceThruBuilding);

// /**
// FUNCTION :: Street_M_to_M_Initialize
// LAYER :: Physical Layer.
// PURPOSE :: Initialize Street mobile-to-mobile model
// PARAMETERS ::
// + propChannel: PropChannel * : Pointer to propagation channel
// + channelIndex: int : Channel Index
// + nodeInput:  NodeInput * : Node input
// RETURN :: void : void value
//
// **/
void Street_M_to_M_Initialize(PropChannel *propChannel,
                              int channelIndex,
                              const NodeInput *nodeInput);


// /**
// FUNCTION :: Pathloss_Street_M_to_M
// LAYER :: Physical Layer.
// PURPOSE :: Propagation pathloss for mobile-to-mobile in urban canyon
// PARAMETERS ::
// + numBuildingsInPath: int : number of buildings in propagation path
// + h1: double : Height of node first node
// + h2: double : Height of node second node
// + buildingHeight: double : Average building height in propagation path
// + streetWidth: double : Average street-width height in propagation path
// + txRxdistance: double : distance between nodes
// + waveLength: double: waveLength
// RETURN :: double : Pathloss value
//
// **/
double Pathloss_Street_M_to_M(int numBuildingsInPath,
                              double h1,
                              double h2,
                              double buildingHeight,
                              double streetWidth,
                              double txRxDistance,
                              double waveLength);

// /**
// FUNCTION :: UrbanProp_CheckFreeSpace
// LAYER :: Physical Layer.
// PURPOSE :: Check if propagation path is through freespace
// PARAMETERS ::
// + h1Effective: double : Height of node first node
// + h2Effective: double : Height of node second node
// + maxRoofHeight: double : Maximum roof height between nodes
// RETURN :: BOOL : TRUE if Freespace path exists, FALSE otherwise
//
// **/
bool UrbanProp_CheckFreeSpace(double distance,
                              double h1Effective,
                              double h2Effective,
                              double maxRoofHeight);

// /**
// FUNCTION :: UrbanProp_CheckCanyon
// LAYER :: Physical Layer.
// PURPOSE :: Check if propagation path is through urban canyon
// PARAMETERS ::
// + groundHeight: double : Height of ground
// + effectiveHeight: double : Effective height of node (antenna+node height)
// + minRoofHeight: double: Minimum roof height between nodes
// RETURN :: BOOL : TRUE if path through Canyon exists, FALSE otherwise
//
// **/
BOOL UrbanProp_CheckCanyon(double groundHeight,
                           double effectiveHeight,
                           double minRoofHeight);


/******* Subroutines to obtain Model parameters from Terrain Data ******/

/* Get the relative node orientation
        |  o  |Node1
        |     |
----------------|     |-------------------
            Corner      o Node2
--------------------------------------------
The subroutine should return
(1) Distance between (Node1-Corner)
(2) The same subroutine will also be used to get distance between (Corner-Node2)

The subroutine will be called ONLY IF there is only 1 building between
Node1 and Node2.
*/


/* Get the relative node orientation
o Node1 |   |
--------|   |-------

------------------
        o Node2
The lines represent road. The orientation is the angle between:
(1) Straight lines between nodes and
(2) Direction of motion - assumed to be strictly on the road (road can be
    assumed parallel to the face of the building)
*/

/* Following functions set and get the values of parameters
 in Urban Structure */

static void UrbanProp_SetOrientation(PropProfile* propProfile, double val)
{ propProfile->RelativeNodeOrientation = val; }

static double UrbanProp_GetOrientation(PropProfile* propProfile)
{ return propProfile->RelativeNodeOrientation; }


/* Average width of the street between two nodes */
static void UrbanProp_SetBuildingSep(PropProfile* propProfile, double val)
{ propProfile->buildingSeparation = val; }

static double UrbanProp_GetBuildingSep(PropProfile* propProfile)
{ return propProfile->roofHeight; }

static void UrbanProp_SetStreetWidth(PropProfile* propProfile, double val)
{ propProfile->streetWidth = val; }

static double UrbanProp_GetStreetWidth(PropProfile* propProfile)
{ return propProfile->streetWidth; }


/* Average Building Height in the propagation path between two nodes */
static void UrbanProp_SetAvgHRoof(PropProfile* propProfile, double avgHRoof)
{ propProfile->roofHeight = avgHRoof; }

static double UrbanProp_GetAvgHRoof(PropProfile* propProfile)
{ return propProfile->roofHeight; }


/* Maximum Roof Height */
static void UrbanProp_SetMaxHRoof(double maxHRoof, PropProfile* propProfile)
{ propProfile->MaxRoofHeight = maxHRoof; }

static double UrbanProp_GetMaxHRoof(PropProfile* propProfile)
{ return propProfile->MaxRoofHeight ; }


/* Minimum Roof Height */
static void UrbanProp_SetMinHRoof(double minHRoof, PropProfile* propProfile)
{ propProfile->MaxRoofHeight = minHRoof; }

static double UrbanProp_GetMinHRoof(PropProfile* propProfile)
{ return propProfile->MaxRoofHeight; }


// /**
// FUNCTION :: UrbanProp_PrintModelInfo
// LAYER :: Physical Layer.
// PURPOSE :: Prints model information for debugging purposes
// PARAMETERS ::
// + isFreeSpace: BOOL : Flag to indicate Freespace
// + isLoS: BOOL : Flag to indicate LoS
// + isCanyon1: BOOL : Flag to indicate Node 1 in Canyon
// + isCanyon2: BOOL : Flag to indicate Node 2 in Canyon
// + urbanModelType: UrbanModelType (Enum): Urban Model Type
// RETURN :: void : void
//
// **/
void UrbanProp_PrintModelInfo(BOOL isFreeSpace,
                              BOOL isLoS,
                              BOOL isCanyon1,
                              BOOL isCanyon2,
                              UrbanModelType urbanModelType);

#endif /* PROP_URBAN */
