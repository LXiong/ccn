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

//#ifdef INCLUDE_THIS_MODEL
// Temporarily disabled

// /**
// PROTOCOL     :: Group Mobility Model
//
// SUMMARY      :: This is the implementation of a group mobility model
//                 for supporting the LANMAR routing protocol. Nodes are
//                 divided into groups. Each group has a same mobility
//                 vector as a whole following the random waypoint
//                 model. Within each group, a node also moves following
//                 random waypoint model within the area of the group.
//
// LAYER        :: Physical
//
// STATICSTICS  ::
//
// CONFIG_PARAM ::
// + GROUP-AREA : Coordinates Coordinates : Two coordinates. The first
// one is the origin of initial group area. The second one is the
// dimensions of the initial group area. If this parameter is defined,
// group memebers will be initially distributed in the given area.
// Group is assumed to be a subnet. To specify the group, a subnet
// identifier should be specified as the prefix.
// + GROUP-NODE-PLACEMENT : UNIFORM | RANDOM | GRID : Specifies the
// group node distribution method. The original distriution function
// is called. But the area is limited as given in the corresponding
// GROUP-AREA parameter.
// + MOBILITY : GROUP-MOBILITY : GROUP-MOBILITY should be specified
// in order to use the group mobility model.
// + MOBILITY-GROUP-PAUSE : clocktype : Pause time of the group move.
// Subnet can be specified for this parameter.
// + MOBILITY-GROUP-MIN-SPEED : int : The minimum speed of the group move.
// Subnet can be specified for this parameter.
// + MOBILITY-GROUP-MAX-SPEED : int : The maximum speed of the group move.
// Subnet can be specified for this parameter.
// + MOBILITY-GROUP-INTERNAL-PAUSE : clocktype : Pause time of internal
// move of a node within its group. Subnet can be specified.
// + MOBILITY-GROUP-INTERNAL-MIN-SPEED : int : The minimum speed of
// internal move of a node within its group. Subnet can be specified.
// + MOBILITY-GROUP-INTERNAL-MAX-SPEED : int : The maximum speed of
// internal move of a node within its group. Subnet can be specified.
//
// VALIDATION   ::
//
// IMPLEMENTED FEATURES ::
// + Mobility of whole group : Random Waypoint in the whole terrain.
// + Internal mobility : Random Waypoint within group dimensions.
//
// OMITTED FEATURES ::
//
// ASSUMPTIONS  ::
// + Each group is one subnet. Thus, no explicit definition of group
// memeber.
//
// STANDARD     ::
// **/

#ifndef MOBILITY_GROUP_H
#define MOBILITY_GROUP_H

// /**
// CONSTANT    :: MOBILITY_GROUP_DELTA_ZERO
// DESCRIPTION :: Values smaller than this is considered
//                as zero.
// **/
#define MOBILITY_GROUP_DELTA_ZERO      0.000001


// /**
// CONSTANT    :: MOBILITY_GROUP_INITIAL_SIZE
// DESCRIPTION :: The maximum size of a group
// **/
#define MOBILITY_GROUP_INITIAL_SIZE    100


//---------------------------------------------------------------------------
// PROTOTYPES FOR FUNCTIONS WITH EXTERNAL LINKAGE
//---------------------------------------------------------------------------

// /**
// FUNCTION   :: SetNodePositionsInGroup
// LAYER      :: PHYSICAL
// PURPOSE    :: Distribute nodes as groups according to their group area
//               and distribution method.
// PARAMETERS ::
// + numNodes : int : Number of nodes
//          // Modified for a bug fix present in exata20_l3
           // rev 1.2.2.3.24.1.2.1
// + nodePlacementTypeCounts : 
// + (numNodesToDistribute : int : Number of nodes using this distribution) -removed
// + nodePositions : NodePositions* : The list of node positions
// + terrainData : TerrainData : Terrain information
// + nodeInput : NodeInput* : Pointer to NodeInput
// + seed : RandomSeed : Random seed
// + maxSimTime : clocktype : Maximum simulation time
// RETURN     :: void : NULL
// **/

void SetNodePositionsInGroup(int numNodes,
                             // Modified for a bug fix present in exata20_l3
                             // rev 1.2.2.3.24.1.2.1
                             //int numNodesToDistribute,
                             int *nodePlacementTypeCounts,
                             NodePositions* nodePositions,
                             TerrainData* terrainData,
                             NodeInput* nodeInput,
                             RandomSeed seed,
                             clocktype maxSimTime);

// /**
// FUNCTION   :: MOBILITY_GroupMobilityInit
// LAYER      :: PHYSICAL
// PURPOSE    :: Initialization Function for group mobility model
// PARAMETERS ::
// + nodeId : NodeAddress : node ID
// + mobilityData : MobilityData* : Pointer to mobility data
// + terrainData : TerrainData* : Pointer to terrain data
// + nodeInput : const NodeInput* : Pointer to NodeInput
// + maxSimClock : clocktype : The maximum simulation clock
// + nodePositions : NodePositions* : The list of node positions
// + numNodes : int : Number of nodes
// RETURN     :: void : NULL
// **/

void MOBILITY_GroupMobilityInit(NodeAddress nodeId,
                                MobilityData* mobilityData,
                                TerrainData* terrainData,
                                NodeInput* nodeInput,
                                clocktype maxSimClock,
                                NodePositions* nodePositions,
                                int numNodes);

#endif // MOBILITY_GROUP_H
//#endif
