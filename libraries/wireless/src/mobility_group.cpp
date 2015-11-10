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
// PROTOCOL   :: GROUP MOBILITY
// LAYER      :: PHYSICAL
// REFERENCES ::
// + "A Group Mobility Model for Ad Hoc Wireless Networks"
//   by X. Hong, M. Gerla, G. Pei, and C.-C. Chiang
//   In Proc of ACM/IEEE MSWiM'99, Seattle, WA, Aug. 1999.
// COMMENTS   :: This is the implementation of a group mobility model.
// **/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "api.h"
#include "partition.h"

#include "mobility_group.h"

#ifdef MGRS_ADDON
#include "mgrs_utility.h"
#endif // MGRS_ADDON

void SetNodePositionsRandomly(
    int numNodes,
    int numNodesToDistribute,
    NodePositions *nodePositions,
    TerrainData* terrainData,
    Coordinates* groupTerrainOrigin,
    Coordinates* groupTerrainDimensions,
    NodeInput *nodeInput,
    RandomSeed seed,
    clocktype maxSimTime);

void SetNodePositionsUniformly(
    int numNodes,
    int numNodesToDistribute,
    NodePositions *nodePositions,
    TerrainData* terrainData,
    Coordinates* groupTerrainOrigin,
    Coordinates* groupTerrainDimensions,
    NodeInput *nodeInput,
    RandomSeed seed,
    clocktype maxSimTime);

void SetNodePositionsInGrid(
    int numNodes,
    int numNodesToDistribute,
    NodePositions *nodePositions,
    TerrainData* terrainData,
    Coordinates* groupTerrainOrigin,
    Coordinates* groupTerrainDimensions,
    NodeInput *nodeInput,
    clocktype maxSimTime,
    double gridUnit);


void SetNodePositionsWithFileInputs(
    int numNodes,
    int numNodesToDistribute,
    NodePositions *nodePositions,
    TerrainData* terrainData,
    Coordinates* groupTerrainOrigin,
    Coordinates* groupTerrainDimensions,
    NodeInput *nodeInput,
    clocktype maxSimTime,
    clocktype startSimTime);

// /**
// FUNCTION   :: GroupDebugNodeDistribution
// LAYER      :: PHYSICAL
// PURPOSE    :: Whether debugging the group node distribution.
// PARAMETERS ::
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/

static
BOOL GroupDebugNodeDistribution()
{
    return FALSE;
}


// /**
// FUNCTION   :: GroupDebugMobility
// LAYER      :: PHYSICAL
// PURPOSE    :: Whether debugging the group mobility.
// PARAMETERS ::
// + node : Node* : Node pointer that the protocol is being instantiated in
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/
static
BOOL GroupDebugNodeMobility(NodeAddress nodeId)
{
    return FALSE;
}


// /**
// FUNCTION   :: GroupDebug
// LAYER      :: PHYSICAL
// PURPOSE    :: For general debug purpose.
// PARAMETERS ::
// RETURN     :: BOOL : TRUE for debugging, FALSE for no deubgging
// **/

static
BOOL GroupDebug()
{
    return FALSE;
}


// /**
// FUNCTION   :: GROUP_ParseInputFileGroupLine
// LAYER      :: PHYSICAL
// PURPOSE    :: One group definition is treated as one group under
//               group mobility model. This function parse one group line.
// PARAMETERS ::
// + nodeInput : const NodeInput* : Pointer to NodeInput
// + groupLine : const char[] : One line of a group definition
// + groupIndex : int : The index of the group
// + nodeIdList : NodeAddress* : A pre-allocated list for returing node IDs
// defined in this group.
// + numNodeIds : int* : For returning how many node IDs in the nodeIdList
// + groupTerrainOrigin : Coordinates* : For returning the origin of the
// corresponding group area, if defined.
// + groupTerrainDimensions : Coordinates* : For returning the dimensions
// of the corresponding group area, if defined.
// + nodePositions : NodePositions* : The list of node positions
// + numNodes : int : Number of nodes
// RETURN     :: BOOL : TRUE, group area information is defined
//                      FALSE, group area information is not defined
// **/

static
BOOL GROUP_ParseInputFileGroupLine(const NodeInput* nodeInput,
                                   const char       groupLine[],
                                   int              groupIndex,
                                   NodeAddress**     nodeIdList,
                                   int*             numGroupNodes,
                                   Coordinates*     groupTerrainOrigin,
                                   Coordinates*     groupTerrainDimensions,
                                   NodePositions*   nodePositions,
                                   int              numNodes)
{
    char nodeIdString[MAX_INPUT_FILE_LINE_LENGTH];
    const char* p = groupLine;

    char delims[] = "{,} \n\t";
    char iotoken[MAX_STRING_LENGTH];
    char* next;
    char* token;
    char* endOfList;

    NodeAddress nodeId;
    NodeAddress maxNodeId;
    int numNodeIds;
    int nodeIdListSize = MOBILITY_GROUP_INITIAL_SIZE;

    BOOL retVal;
    char buf[MAX_STRING_LENGTH];

    BOOL needNodeDistribution = FALSE;

    if (GroupDebug())
    {
        printf("Parsing group line: %s\n", groupLine);
    }

    // Copy "{ ... }" part of GROUP line into nodeIdString.
    p = strchr(p, '{');
    strcpy(nodeIdString, p);

    // mark the end of the list
    endOfList = strchr(nodeIdString, '}');
    if (endOfList != NULL)
    {
        endOfList[0] = '\0';
    }

    // Read first nodeId into nodeId variable.
    // Adding 1 to nodeIdString skips '{'.

    nodeId = strtoul(nodeIdString + 1, NULL, 10);

    if (GroupDebug())
    {
        printf("    Group members: {%d ", nodeId);
    }

    // Scan first nodeId from nodeIdString.  Remember we've already parsed
    // the first nodeId into the nodeId variable.  This call is just to
    // initialize IO_GetDelimitedToken().

    token = IO_GetDelimitedToken(iotoken, nodeIdString, delims, &next);

    numNodeIds = 0;
    while (1)
    {
        if (numNodeIds >= nodeIdListSize)
        {
            int prevNodeIdListSize = nodeIdListSize;

            nodeIdListSize = nodeIdListSize * 2;

            NodeAddress* tempNodeIdList = (NodeAddress* )
                MEM_malloc(nodeIdListSize * sizeof(NodeAddress));
            ERROR_Assert(tempNodeIdList != NULL, "Memory allocation error!");

            memcpy(tempNodeIdList,
                   *nodeIdList,
                   prevNodeIdListSize * sizeof(NodeAddress));

            MEM_free(*nodeIdList);
            *nodeIdList = tempNodeIdList;
        }

        if (nodePositions)
        {
            int j;
            for (j = 0; j < numNodes; j++)
            {
                if (nodePositions[j].nodeId == nodeId)
                {
                     (*nodeIdList)[numNodeIds] = nodeId;
                      numNodeIds++;
                      break;
                }
            }
        }

        // Retrieve next token.

        token = IO_GetDelimitedToken(iotoken, next, delims, &next);

        if (!token)
        {
            // No more tokens.
            break;
        }

        if (strncmp("thru", token, 4) == 0)
        {
            // token is "thru".  Add nodeIds in range "x thru y" to
            // groupList array.

            // Read in maximum nodeId.

            token = IO_GetDelimitedToken(iotoken, next, delims, &next);
            maxNodeId = strtoul(token, NULL, 10);

            nodeId++;
            while (nodeId <= maxNodeId)
            {
                if (GroupDebug())
                {
                    printf("%d ", nodeId);
                }

                if (numNodeIds >= nodeIdListSize)
                {
                    int prevNodeIdListSize = nodeIdListSize;

                    nodeIdListSize = nodeIdListSize * 2;

                    NodeAddress* tempNodeIdList = (NodeAddress* )
                        MEM_malloc(nodeIdListSize * sizeof(NodeAddress));
                    ERROR_Assert(tempNodeIdList != NULL,
                                 "Memory allocation error!");

                    memcpy(tempNodeIdList,
                           *nodeIdList,
                           prevNodeIdListSize * sizeof(NodeAddress));

                    MEM_free(*nodeIdList);
                    *nodeIdList = tempNodeIdList;
                }

                if (nodePositions)
                {
                    int j;
                    for (j = 0; j < numNodes; j++)
                    {
                        if (nodePositions[j].nodeId == nodeId)
                        {
                             (*nodeIdList)[numNodeIds] = nodeId;
                              numNodeIds++;
                              break;
                        }
                    }
                }

                nodeId++;
            }

            // Done adding range of nodeIds to groupList array.
            // Read next nodeId.

            token = IO_GetDelimitedToken(iotoken, next, delims, &next);

            if (!token)
            {
                // No more tokens.
                break;
            }
        }

        // Current token is a nodeId.  Parse it into nodeId variable.
        nodeId = strtoul(token, NULL, 10);
    }

    if (GroupDebug())
    {
        printf("}\n");
    }

    (*numGroupNodes) = numNodeIds;

    // Now read the placement information of this group
    IO_ReadStringInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "GROUP-AREA",
        groupIndex,
        FALSE,
        &retVal,
        buf);

    if (retVal)
    {

        char warningString[MAX_STRING_LENGTH];
            sprintf(
                warningString,
                "Use of GROUP-AREA parameters has been deprecated.");
            ERROR_ReportWarning(warningString);

        char terrainOriginString[MAX_STRING_LENGTH];
        char terrainDimensionsString[MAX_STRING_LENGTH];

        needNodeDistribution = TRUE;

        p = strchr(buf, ')');
        strncpy(terrainOriginString, buf, (p - buf) + 1);
        strcpy(terrainDimensionsString, p + 2);

        // Convert to coordinates
        COORD_ConvertToCoordinates(terrainOriginString,
                                   groupTerrainOrigin);
        COORD_ConvertToCoordinates(terrainDimensionsString,
                                   groupTerrainDimensions);

        if (GroupDebug())
        {
            printf("    origin:(%d, %d), dimensions: (%d, %d)\n",
                   (int)groupTerrainOrigin->common.c1,
                   (int)groupTerrainOrigin->common.c2,
                   (int)groupTerrainDimensions->common.c1,
                   (int)groupTerrainDimensions->common.c2);
        }
    }
    else
    {
        char originString[MAX_STRING_LENGTH];
        BOOL originDefined = FALSE;
        char dimensionString[MAX_STRING_LENGTH];
        BOOL dimensionDefined = FALSE;

        IO_ReadStringInstance(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "GROUP-AREA-ORIGIN",
            groupIndex,
            FALSE,
            &originDefined,
            originString);

        IO_ReadStringInstance(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "GROUP-AREA-DIMENSION",
            groupIndex,
            FALSE,
            &dimensionDefined,
            dimensionString);

        if ((originDefined == TRUE) && (dimensionDefined == TRUE))
        {
            needNodeDistribution = TRUE;

            // Convert to coordinates
            COORD_ConvertToCoordinates(originString,
                                       groupTerrainOrigin);
            COORD_ConvertToCoordinates(dimensionString,
                                       groupTerrainDimensions);

            if (GroupDebug())
            {
                printf("    origin:(%d, %d), dimensions: (%d, %d)\n",
                       (int)groupTerrainOrigin->common.c1,
                       (int)groupTerrainOrigin->common.c2,
                       (int)groupTerrainDimensions->common.c1,
                       (int)groupTerrainDimensions->common.c2);
            }
        }
        else
        {
            //then use the whole terrain
            needNodeDistribution = FALSE;
        }
    }

    return needNodeDistribution;
}


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
// + (numNodesToDistribute : int : Number of nodes using this distribution) - removed
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
                              clocktype maxSimTime)
{
    TerrainData groupTerrainData;
    TerrainData subTerrainBoundData;
    Coordinates groupTerrainOrigin;
    Coordinates groupTerrainDimensions;
    NodeAddress* groupNodeIdList;
    int numGroupNodes;

    int numGroups;
    int groupIndex;

    char groupString[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    BOOL wasFound;
    int i;
    int groupSeedVal;
    Coordinates southwestOrLowerLeft = {{{0,0,0}}};
    Coordinates northeastOrUpperRight;
    char southwestOrLowerLeftBuf[MAX_STRING_LENGTH];
    char northeastOrUpperRightBuf[MAX_STRING_LENGTH];

    RandomSeed groupSeed;
    unsigned short *seedPtr;

    double gridUnit;
    double groupGridUnit;

    // Modified for a bug fix present in exata20_l3
    // rev 1.2.2.3.24.1.2.1

    groupNodeIdList = (NodeAddress* )
        MEM_malloc(numNodes * sizeof(NodeAddress));
    ERROR_Assert(groupNodeIdList != NULL, "Memory allocation error!");

    // Get number of groups
    numGroups = 0;
    IO_ReadInt(ANY_NODEID,
               ANY_ADDRESS,
               nodeInput,
               "NUM-MOBILITY-GROUPS",
               &retVal,
               &numGroups);

    if (numGroups <= 0)
    {
        return;
    }

    if (GroupDebugNodeDistribution())
    {
        printf("Group distribution: number of groups = %d\n",
               numGroups);
    }

    for (groupIndex = 0; groupIndex < numGroups; groupIndex ++)
    {
        IO_ReadStringInstance(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "MOBILITY-GROUP",
            groupIndex,
            FALSE,
            &retVal,
            groupString);

        if (retVal == TRUE)
        {
            numGroupNodes = 0;

            retVal = GROUP_ParseInputFileGroupLine(
                         nodeInput,
                         groupString,
                         groupIndex,
                         &groupNodeIdList,
                         &numGroupNodes,
                         &groupTerrainOrigin,
                         &groupTerrainDimensions,
                         nodePositions,
                         numNodes);


            if (terrainData->getCoordinateSystem() == LATLONALT)
            {
                CoordinateType x = groupTerrainDimensions.common.c1;
                CoordinateType y = groupTerrainDimensions.common.c2;
                CoordinateType z = groupTerrainDimensions.common.c3;

                CoordinateType latitude = (y / EARTH_RADIUS) / IN_RADIAN;
                CoordinateType longitude = (x /(EARTH_RADIUS *
                    cos(latitude * IN_RADIAN)))/ IN_RADIAN ;
                CoordinateType altitude = z;

                groupTerrainDimensions.common.c1 = latitude;
                groupTerrainDimensions.common.c2 = longitude;
                groupTerrainDimensions.common.c3 = altitude;
            }

            if (retVal == FALSE)
            {
                groupTerrainOrigin = terrainData->getOrigin();
                groupTerrainDimensions = terrainData->getDimensions();
            }

            if (numGroupNodes > 0)
            {
                // now needs to distribute these nodes
                // in the specified area

                NodePositions* groupNodePositions;
                int j;
                int k;

                groupNodePositions = (NodePositions* )
                    MEM_malloc(numGroupNodes * sizeof(NodePositions));
                ERROR_Assert(groupNodePositions != NULL,
                             "Memory allocation error!");

                // make the node list for reuse code
                for (k = 0; k < numGroupNodes; k ++)
                {
                    for (j = 0; j < numNodes; j ++)
                    {
                        if (nodePositions[j].nodeId == groupNodeIdList[k])
                        {
                            groupNodePositions[k] = nodePositions[j];
                        }
                    }
                }

                buf[0] = '\0';
                IO_ReadStringInstance(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "GROUP-NODE-PLACEMENT",
                    groupIndex,
                    TRUE,
                    &retVal,
                    buf);

                // MERGE - if no GROUP-NODE-PLACEMENT specified for
                // groupIndex, we shouldnt continue.
                if (retVal == FALSE)
                {
                    char errBuff[MAX_STRING_LENGTH];
                    sprintf(errBuff, "No GROUP-NODE-PLACEMENT specified for Group %d!!\n",
                            groupIndex);
                    ERROR_Assert(FALSE, errBuff);
                }

                //groupTerrainData = *terrainData;
                //groupTerrainData.origin = groupTerrainOrigin;
                //groupTerrainData.dimensions = groupTerrainDimensions;

                if (GroupDebugNodeDistribution())
                {
                    printf("Group[%d], origin=(%f, %f), "
                           "dimension = (%f, %f), "
                           "placement = %s\n",
                           groupIndex,
                           groupTerrainOrigin.common.c1,
                           groupTerrainOrigin.common.c2,
                           groupTerrainDimensions.common.c1,
                           groupTerrainDimensions.common.c2,
                           buf);
                }


                if ((groupTerrainOrigin.common.c1 < terrainData->getOrigin().common.c1)
                    ||
                    (groupTerrainOrigin.common.c2 < terrainData->getOrigin().common.c2)
                    ||
                    (groupTerrainOrigin.common.c1 >
                    (terrainData->getOrigin().common.c1 + terrainData->getDimensions().common.c1))
                    ||
                    (groupTerrainOrigin.common.c2 >
                    (terrainData->getOrigin().common.c2 + terrainData->getDimensions().common.c2)))
                {
                    ERROR_ReportError(
                                    "bounding box origin cannot"
                                    " be outside the whole Terrain");
                }

                if (((groupTerrainOrigin.common.c1 + groupTerrainDimensions.common.c1) >
                    (terrainData->getOrigin().common.c1 + terrainData->getDimensions().common.c1))
                    || ((groupTerrainOrigin.common.c2 + groupTerrainDimensions.common.c2)>
                    (terrainData->getOrigin().common.c2 + terrainData->getDimensions().common.c2)))
                {
                    char warningString[MAX_STRING_LENGTH];
                    sprintf(
                            warningString,
                            "Bounding-box dimensions are too large"
                            " to fit in the whole terrain");
                    ERROR_ReportWarning(warningString);

                    groupTerrainDimensions.common.c1 =
                    terrainData->getDimensions().common.c1 -
                    (groupTerrainOrigin.common.c1 - terrainData->getOrigin().common.c1);

                    groupTerrainDimensions.common.c2 =
                    terrainData->getDimensions().common.c2 -
                    (groupTerrainOrigin.common.c2 - terrainData->getOrigin().common.c2);

                }



                IO_ReadIntInstance(
                           ANY_NODEID,
                           ANY_ADDRESS,
                           nodeInput,
                           "GROUP-SEED",
                            groupIndex,
                            TRUE,
                           &wasFound,
                           &groupSeedVal);



                if (wasFound == TRUE)
                {
                        RANDOM_SetSeed(groupSeed,
                                       groupSeedVal,
                                       GROUP_MOBILITY,
                                       groupIndex);
                        seedPtr = groupSeed;
                }

                else
                {
                    seedPtr = seed;
                }

                if (strcmp(buf, "RANDOM") == 0)
                {
                    int i;

                    for (i = 0; i < numGroupNodes; i ++)
                    {
                        groupNodePositions[i].nodePlacementType =
                            RANDOM_PLACEMENT;
                    }

                    SetNodePositionsRandomly(
                        numGroupNodes,
                        numGroupNodes,
                        groupNodePositions,
                        terrainData,
                        &groupTerrainOrigin,
                        &groupTerrainDimensions,
                        nodeInput,
                        seedPtr,
                        maxSimTime);
                }
                else if (strcmp(buf, "UNIFORM") == 0)
                {
                    int i;

                    for (i = 0; i < numGroupNodes; i ++)
                    {
                        groupNodePositions[i].nodePlacementType =
                            UNIFORM_PLACEMENT;
                    }

                    SetNodePositionsUniformly(
                        numGroupNodes,
                        numGroupNodes,
                        groupNodePositions,
                        terrainData,
                        &groupTerrainOrigin,
                        &groupTerrainDimensions,
                        nodeInput,
                        seedPtr,
                        maxSimTime);
                }
                else if (strcmp(buf, "GRID") == 0)
                {
                    int i;

                    for (i = 0; i < numGroupNodes; i ++)
                    {
                        groupNodePositions[i].nodePlacementType =
                            GRID_PLACEMENT;
                    }

                    IO_ReadDouble(
                            ANY_NODEID,
                            ANY_ADDRESS,
                            nodeInput,
                            "GRID-UNIT",
                            &wasFound,
                            &gridUnit);

                    IO_ReadDoubleInstance(
                           ANY_NODEID,
                           ANY_ADDRESS,
                           nodeInput,
                           "GROUP-GRID-UNIT",
                           groupIndex,
                           TRUE,
                           &wasFound,
                           &groupGridUnit);

                    if (wasFound == FALSE)
                    {
                        groupGridUnit = gridUnit;
                    }

                    else if (groupGridUnit<=0)
                    {
                        ERROR_ReportError(
                                        "GROUP-GRID-UNIT cannot be "
                                        "negative or zero");
                    }

                    SetNodePositionsInGrid(
                        numGroupNodes,
                        numGroupNodes,
                        groupNodePositions,
                        terrainData,
                        &groupTerrainOrigin,
                        &groupTerrainDimensions,
                        nodeInput,
                        maxSimTime,
                        groupGridUnit);
                }

                else if (strcmp(buf, "FILE") == 0)
                {
                    int i;

                    for (i = 0; i < numGroupNodes; i ++)
                    {
                        groupNodePositions[i].nodePlacementType =
                            FILE_BASED_PLACEMENT;
                    }
                    SetNodePositionsWithFileInputs(
                        numGroupNodes,
                        numGroupNodes,
                        groupNodePositions,
                        terrainData,
                        &groupTerrainOrigin,
                        &groupTerrainDimensions,
                        nodeInput,
                        maxSimTime,
                        0);

                }

                else
                {
                    // No group placement method is defined, so assume that the
                    // the default node placement will meet requirement.
                }

                if (terrainData->getCoordinateSystem() == LATLONALT)
                {
                    IO_ReadStringInstance(
                            ANY_NODEID,
                            ANY_ADDRESS,
                            nodeInput,
                            "GROUP-TERRAIN-CONSTRAINT-SOUTH-WEST-CORNER",
                            groupIndex,
                            FALSE,
                            &retVal,
                            southwestOrLowerLeftBuf);

                    if (retVal == TRUE)
                    {
                        IO_ReadStringInstance(
                                ANY_NODEID,
                                ANY_ADDRESS,
                                nodeInput,
                                "GROUP-TERRAIN-CONSTRAINT-NORTH-EAST-CORNER",
                                groupIndex,
                                FALSE,
                                &retVal,
                                northeastOrUpperRightBuf);
                    }
                }
                else if (terrainData->getCoordinateSystem() == CARTESIAN)
                {
                    IO_ReadStringInstance(
                            ANY_NODEID,
                            ANY_ADDRESS,
                            nodeInput,
                            "GROUP-TERRAIN-CONSTRAINT-LOWER-LEFT-CORNER",
                            groupIndex,
                            FALSE,
                            &retVal,
                            southwestOrLowerLeftBuf);

                    if (retVal == TRUE)
                    {
                        IO_ReadStringInstance(
                                ANY_NODEID,
                                ANY_ADDRESS,
                                nodeInput,
                                "GROUP-TERRAIN-CONSTRAINT-UPPER-RIGHT-CORNER",
                                groupIndex,
                                FALSE,
                                &retVal,
                                northeastOrUpperRightBuf);
                    }
                }
#ifdef MGRS_ADDON
                else if (terrainData->getCoordinateSystem() == MGRS)
                {
                    MgrsUtility mgrs;

                    IO_ReadStringInstance(ANY_NODEID, ANY_ADDRESS,
                        nodeInput, "GROUP-MGRS-CONSTRAINT-SOUTH-WEST-CORNER"
                        , groupIndex, FALSE, &retVal,
                        southwestOrLowerLeftBuf);

                    if (retVal && mgrs.isMgrs(southwestOrLowerLeftBuf))
                    {
                        IO_ReadStringInstance(ANY_NODEID, ANY_ADDRESS,
                            nodeInput, "GROUP-MGRS-CONSTRAINT-NORTH-EAST-"
                            "CORNER", groupIndex, FALSE, &retVal,
                            northeastOrUpperRightBuf);

                        if (retVal && !mgrs.isMgrs(northeastOrUpperRightBuf))
                        {
                            retVal = FALSE;
                        }
                    }
                    else
                    {
                        retVal = FALSE;
                    }
                }
#endif // MGRS_ADDON
                else
                {
                    ERROR_ReportError("Group Mobility used in wrong "
                                      "coordinate system.\n");
                }

                // Assume first that there's no boundary for group movement
                // (only bounded by whole terrain).
                subTerrainBoundData = *terrainData;

                // If group movement is bounded to a part of the terrain,
                // then get the boundary.
                if (retVal == TRUE)
                {
#ifdef MGRS_ADDON
                    if (terrainData->getCoordinateSystem() == MGRS)
                    {
                        MgrsUtility mgrs;
                        std::string sw = terrainData->getMgrsSw();

                        // Values are relative to the main MGRS SW corner
                        southwestOrLowerLeft.common.c1 = mgrs.
                            swNeRectRightEdge(sw, southwestOrLowerLeftBuf);
                        southwestOrLowerLeft.common.c2 = mgrs.
                            swNeRectTopEdge(sw, southwestOrLowerLeftBuf);

                        northeastOrUpperRight.common.c1 = mgrs.
                            swNeRectRightEdge(sw, northeastOrUpperRightBuf);

                        northeastOrUpperRight.common.c2 = mgrs.
                            swNeRectTopEdge(sw, northeastOrUpperRightBuf);
                    }
                    else
#endif // MGRS_ADDON
                    {
                        COORD_ConvertToCoordinates(southwestOrLowerLeftBuf,
                                                   &southwestOrLowerLeft);
                        COORD_ConvertToCoordinates(northeastOrUpperRightBuf,
                                                   &northeastOrUpperRight);
                    }

                    subTerrainBoundData.setSW(&southwestOrLowerLeft);
                    subTerrainBoundData.setNE(&northeastOrUpperRight);

                    // Make sure that the south/lower corner is really
                    // south/bottom of the north/upper corner.

                    if (northeastOrUpperRight.common.c1 <
                        southwestOrLowerLeft.common.c1)
                    {
                        char errorStr[MAX_STRING_LENGTH];
                        char buf1[MAX_STRING_LENGTH];
                        char buf2[MAX_STRING_LENGTH];

                        if (terrainData->getCoordinateSystem() == LATLONALT)
                        {
                            sprintf(buf1,
                                    "GROUP-TERRAIN-CONSTRAINT-NORTH-EAST-"
                                    "CORNER");

                            sprintf(buf2,
                                    "GROUP-TERRAIN-CONSTRAINT-SOUTH-WEST-"
                                    "CORNER");
                        }
#ifdef MGRS_ADDON
                        else if (terrainData->getCoordinateSystem() == MGRS)
                        {
                            sprintf(buf1,
                                "GROUP-MGRS-CONSTRAINT-NORTH-EAST-CORNER");

                            sprintf(buf2,
                                "GROUP-MGRS-CONSTRAINT-SOUTH-WEST-CORNER");
                        }
#endif // MGRS_ADDON
                        else
                        {
                            sprintf(buf1,
                                    "GROUP-TERRAIN-CONSTRAINT-UPPER-RIGHT-"
                                    "CORNER");

                            sprintf(buf2,
                                    "GROUP-TERRAIN-CONSTRAINT-LOWER-LEFT-"
                                    "CORNER");
                        }

#ifdef MGRS_ADDON
                        if (terrainData->getCoordinateSystem() == MGRS)
                        {
                            sprintf(errorStr, "The %s (%s) of the "
                                "terrain must be to the northeast of the %s"
                                " (%s).", buf1, northeastOrUpperRightBuf,
                                buf2, southwestOrLowerLeftBuf);
                        }
                        else
#endif //MGRS_ADDON
                        {
                            sprintf(errorStr,
                                "%s (%f, %f) "
                                "is located south of\n"
                                "%s (%f, %f)."
                                "\nSo %f should have been greater than %f",
                                buf1,
                                northeastOrUpperRight.common.c1,
                                northeastOrUpperRight.common.c2,
                                buf2,
                                southwestOrLowerLeft.common.c1,
                                southwestOrLowerLeft.common.c2,
                                northeastOrUpperRight.common.c1,
                                southwestOrLowerLeft.common.c1);
                        }

                        ERROR_ReportError(errorStr);
                    }

                    // Make sure dimensions are correctly calculated for
                    // wrapped around coordindates in LATLONALT.
                    if (terrainData->getCoordinateSystem() == LATLONALT)
                    {
                        if (subTerrainBoundData.getDimensions().common.c2 < 0.0)
                        {
                            Coordinates newCord = subTerrainBoundData.getDimensions();
                            newCord.common.c2 += 360.0;
                            subTerrainBoundData.setDimensions(&newCord);
                            //(subTerrainBoundData.getDimensions().common.c2) += 360.0;
                        }

                        // Don't need to check for latitude since latitude SW
                        // has to be south of NE.
                    }
                }

                for (i = 0; i < numGroupNodes; i ++)
                {
                    MOBILITY_GroupMobilityInit(
                            groupNodePositions[i].nodeId,
                            groupNodePositions[i].mobilityData,
                            &subTerrainBoundData,
                            nodeInput,
                            maxSimTime,
                            nodePositions,
                            numNodes);
                }

                // write back
                for (k = 0; k < numGroupNodes; k ++)
                {
                    for (j = 0; j < numNodes; j ++)
                    {
                        if (nodePositions[j].nodeId == groupNodeIdList[k])
                        {
                            nodePositions[j] = groupNodePositions[k];
                        }
                    }
                }

                MEM_free(groupNodePositions);
            }
        }
    }

    MEM_free(groupNodeIdList);
}


// /**
// FUNCTION   :: MOBILITY_GroupMobility
// LAYER      :: PHYSICAL
// PURPOSE    :: Generate all intermediate positions for current node
//               according to the group mobility model.
// PARAMETERS ::
// + nodeId : NodeAddress : Node ID of the node
// + mobilityData : MobilityData* : pointer to the mobility data
// + terrainData : TerrainData* : pointer to terrain data
// + nodeInput : const NodeInput* : Pointer to NodeInput
// + maxSimClock : clocktype : The maximum simulation clock
// + int groupIndex : int : the index of the group
// + groupTerrainOrigin : Coordinates : Origin of the initial group area
// + groupTerrainDimensions : Coordinates : Dimensions of the initial
//                                          group area
// RETURN     :: void : NULL
// **/

static
void MOBILITY_GroupMobility(NodeAddress nodeId,
                            MobilityData* mobilityData,
                            TerrainData* terrainData,
                            const NodeInput* nodeInput,
                            clocktype maxSimClock,
                            int groupIndex,
                            Coordinates groupTerrainOrigin,
                            Coordinates groupTerrainDimensions)
{
    clocktype simClock;
    Orientation orientation;

    Coordinates refPoint;
    double groupDimensionX;
    double groupDimensionY;

    clocktype groupMobilityPause;
    double groupMinSpeed;
    double groupMaxSpeed;

    clocktype internalMobilityPause;
    double internalMinSpeed;
    double internalMaxSpeed;

    Coordinates current;
    Coordinates dest;

    Coordinates refDest = {{{0,0,0}}};
    clocktype refDeltaTime;
    double refSpeed;
    double refDistance;

    Coordinates internalDest = {{{0,0,0}}};
    clocktype internalDeltaTime;
    double internalSpeed;
    double internalDistance;

    char buf[MAX_STRING_LENGTH];
    BOOL wasFound;
    BOOL beRefPause = FALSE;
    BOOL beInternalPause = FALSE;

#ifdef ADDON_BOEINGFCS
    double zValue = 0.0;
#endif

    // this is for generating the group seed. The purpose of the group
    // seed is to provide the same seed to all group memebers for
    // generating the same group movements.
    RandomSeed groupSeed;
    int seedVal;

    if (GroupDebugNodeMobility(nodeId))
    {
        char clockString[MAX_STRING_LENGTH];

        ctoa(maxSimClock, clockString);

        printf("Computing mobility points for node%d:\n"
               "    maxSimClock = %s\n"
               "    groupIndex = %d\n"
               "    group area origin = (%d, %d)\n"
               "    group area dimensions = (%d, %d)\n"
               "    seed = [%d, %d, %d]\n",
               nodeId,
               clockString,
               groupIndex,
               (int)groupTerrainOrigin.common.c1,
               (int)groupTerrainOrigin.common.c2,
               (int)groupTerrainDimensions.common.c1,
               (int)groupTerrainDimensions.common.c2,
               (int)mobilityData->seed[0],
               (int)mobilityData->seed[1],
               (int)mobilityData->seed[2]);
    }

    // no orientation for this model right now.
    orientation.azimuth = 0;
    orientation.elevation = 0;

    // Read the pause time of the group after reaching destination
    IO_ReadStringInstance(
        nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MOBILITY-GROUP-PAUSE",
        groupIndex,
        TRUE,
        &wasFound,
        buf);

    if (wasFound == FALSE)
    {
        printf("Node%u, no group mobility is defined for group %d, "
               "assume static.\n",
               nodeId,
               groupIndex);

        return;
    }

    groupMobilityPause = TIME_ConvertToClock(buf);


    // Read the speed (min, max) of the group
    IO_ReadDoubleInstance(
        nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MOBILITY-GROUP-MIN-SPEED",
        groupIndex,
        TRUE,
        &wasFound,
        &groupMinSpeed);

    ERROR_Assert(wasFound == TRUE,
                 "MOBILITY-GROUP-MIN-SPEED is not defined!");

    IO_ReadDoubleInstance(
        nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MOBILITY-GROUP-MAX-SPEED",
        groupIndex,
        TRUE,
        &wasFound,
        &groupMaxSpeed);

    ERROR_Assert(wasFound == TRUE,
                 "MOBILITY-GROUP-MAX-SPEED is not defined!");


    // Read the pause time of internal mobility after reaching dest
    IO_ReadStringInstance(
        nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MOBILITY-GROUP-INTERNAL-PAUSE",
        groupIndex,
        TRUE,
        &wasFound,
        buf);

    ERROR_Assert(wasFound == TRUE,
                 "MOBILITY-GROUP-INTERNAL-PAUSE is not defined!");

    internalMobilityPause = TIME_ConvertToClock(buf);


    // Read the speed (min, max) of the internal mobility
    IO_ReadDoubleInstance(
        nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MOBILITY-GROUP-INTERNAL-MIN-SPEED",
        groupIndex,
        TRUE,
        &wasFound,
        &internalMinSpeed);

    ERROR_Assert(wasFound == TRUE,
                 "MOBILITY-GROUP-INTERNAL-MIN-SPEED is not defined!");

    IO_ReadDoubleInstance(
        nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MOBILITY-GROUP-INTERNAL-MAX-SPEED",
        groupIndex,
        TRUE,
        &wasFound,
        &internalMaxSpeed);

    ERROR_Assert(wasFound == TRUE,
                 "MOBILITY-GROUP-INTERNAL-MAX-SPEED is not defined!");

    if (GroupDebugNodeMobility(nodeId))
    {
        ctoa(groupMobilityPause, buf);
        printf("Node%d, group mobility (min=%.2f, max=%.2f, pause=%s) ",
               nodeId, groupMinSpeed, groupMaxSpeed, buf);

        ctoa(internalMobilityPause, buf);
        printf(" internal mobility "
               "(min=%.2f, max=%.2f, pause=%s)\n",
               internalMinSpeed, internalMaxSpeed, buf);
    }

    // Group information

    // Generate the shared group seed
    seedVal = 1;
    IO_ReadInt(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "SEED",
        &wasFound,
        &seedVal);

    RANDOM_SetSeed(groupSeed,
                   seedVal,
                   GROUP_MOBILITY,
                   groupIndex);

    groupDimensionX = groupTerrainDimensions.common.c1;
    groupDimensionY = groupTerrainDimensions.common.c2;

    // Get the center point of the group box
    refPoint.common.c1 = groupTerrainOrigin.common.c1 +
                         groupDimensionX / 2;
    refPoint.common.c2 = groupTerrainOrigin.common.c2 +
                         groupDimensionY / 2;
    refPoint.common.c3 = mobilityData->current->position.common.c3;

    refDeltaTime = 0;

    // Start generating future positions according
    // to the mobility model

    simClock = 0;

    internalDeltaTime = 0;
    current = mobilityData->current->position;

    if (GroupDebugNodeMobility(nodeId))
    {
        printf("Node%d, intermediate points:\n", nodeId);
    }

    while (simClock < maxSimClock)
    {
        if (refDeltaTime <= 0)
        {
            // Next reference point mobility
            if (groupMaxSpeed < MOBILITY_GROUP_DELTA_ZERO)
            {
                refDest = refPoint;

                refDeltaTime = maxSimClock;
            }
            else
            {
                if ((beRefPause == TRUE) && (groupMobilityPause > 0))
                {
                    beRefPause = FALSE;

                    refDest = refPoint;

                    refDeltaTime = groupMobilityPause;
                }
                else
                {
                    beRefPause = TRUE;

                    // Next movement of the reference point

                    // Must be contained within (groupDimensionX / 2)
                    // from border.
                    refDest.common.c1 =
                        terrainData->getOrigin().common.c1 +
                        groupDimensionX / 2 +
                        ((terrainData->getDimensions().common.c1 -
                        groupDimensionX ) *
                        RANDOM_erand(groupSeed));

                    // Must be contained within (groupDimensionY / 2)
                    // from border.
                    refDest.common.c2 =
                        terrainData->getOrigin().common.c2 +
                        groupDimensionY / 2 +
                        ((terrainData->getDimensions().common.c2 -
                        groupDimensionY) *
                        RANDOM_erand(groupSeed));

                    refDest.common.c3 = refPoint.common.c3;
#ifdef ADDON_BOEINGFCS
            zValue = refDest.common.c3;
#endif

                    refSpeed = groupMinSpeed +
                               (RANDOM_erand(groupSeed) *
                               (groupMaxSpeed - groupMinSpeed));

                    COORD_CalcDistance(
                        terrainData->getCoordinateSystem(),
                        &refPoint, &refDest, &refDistance);

                    refDeltaTime =
                        (clocktype)(refDistance / refSpeed * SECOND);
                }
            }
        }

        if (internalDeltaTime <= 0)
        {
            // next internal mobility

            if (internalMaxSpeed < MOBILITY_GROUP_DELTA_ZERO)
            {
                internalDest = current;

                internalDeltaTime = maxSimClock;
            }
            else
            {
                if ((beInternalPause == TRUE) &&
                    (internalMobilityPause > 0))
                {
                    beInternalPause = FALSE;

                    // pause some time
                    internalDeltaTime = internalMobilityPause;

                    internalDest = current;
                }
                else
                {
                    beInternalPause = TRUE;

                    // Move within group box in the X-axis.
                    internalDest.common.c1 =
                        (refPoint.common.c1 - groupDimensionX / 2) +
                        (groupDimensionX * RANDOM_erand(mobilityData->seed));

                    // Move within group box in the Y-axis.
                    internalDest.common.c2 =
                        (refPoint.common.c2 - groupDimensionY / 2) +
                        (groupDimensionY * RANDOM_erand(mobilityData->seed));

                    internalDest.common.c3 = current.common.c3;
#ifdef ADDON_BOEINGFCS
            zValue = internalDest.common.c3;
#endif

                    internalSpeed = internalMinSpeed +
                                    (RANDOM_erand(mobilityData->seed) *
                                    (internalMaxSpeed - internalMinSpeed));

                    COORD_CalcDistance(
                        terrainData->getCoordinateSystem(),
                        &current, &internalDest, &internalDistance);

                    internalDeltaTime = (clocktype)
                        (internalDistance / internalSpeed * SECOND);
                }
            }
        }

        dest = current;

        // If group box moves the same or faster than the individual node...
        if (refDeltaTime >= internalDeltaTime)
        {
            // current internal movement ends first

            dest = internalDest;
            dest.common.c1 += ((refDest.common.c1 - refPoint.common.c1) *
                              ((double)internalDeltaTime /
                              (double)refDeltaTime));

            dest.common.c2 += ((refDest.common.c2 - refPoint.common.c2) *
                              ((double)internalDeltaTime /
                              (double)refDeltaTime));

            dest.common.c3 += refDest.common.c3;
#ifdef ADDON_BOEINGFCS
        zValue = dest.common.c3;
#endif

            simClock += internalDeltaTime;

            // Update variables for next iteration
            refPoint.common.c1 += (refDest.common.c1 - refPoint.common.c1) *
                                  ((double)internalDeltaTime /
                                  (double)refDeltaTime);

            refPoint.common.c2 += (refDest.common.c2 - refPoint.common.c2) *
                                  ((double)internalDeltaTime /
                                  (double)refDeltaTime);

            refDeltaTime -= internalDeltaTime;

            internalDeltaTime = 0;
        }
        // If group box moves moves slower than the individual node...
        else
        {
            // current group movement ends first

            CoordinateType internalDeltaX;
            CoordinateType internalDeltaY;

            internalDeltaX = internalDest.common.c1 - current.common.c1;
            internalDeltaY = internalDest.common.c2 - current.common.c2;

            if (refDeltaTime == 0)
            {
                refDeltaTime = internalDeltaTime;
            }

            dest.common.c1 += internalDeltaX *
                              ((double)refDeltaTime /
                              (double)internalDeltaTime);
            dest.common.c2 += internalDeltaY *
                              ((double)refDeltaTime /
                              (double)internalDeltaTime);
            dest.common.c3 = current.common.c3;
#ifdef ADDON_BOEINGFCS
        zValue = dest.common.c3;
#endif

            // Add the group mobility vector
            dest.common.c1 += (refDest.common.c1 - refPoint.common.c1);
            dest.common.c2 += (refDest.common.c2 - refPoint.common.c2);

            simClock += refDeltaTime;

            // Update variables for next iteration
            internalDest.common.c1 +=
                (refDest.common.c1 - refPoint.common.c1);
            internalDest.common.c2 +=
                (refDest.common.c2 - refPoint.common.c2);

            refPoint = refDest;

            internalDeltaTime -= refDeltaTime;

            refDeltaTime = 0;
        }

        // Add next step into the mobility element list
        if (mobilityData->groundNode == TRUE)
        {
            TERRAIN_SetToGroundLevel(terrainData, &dest);
        }

        MOBILITY_AddANewDestination(mobilityData,
                                    simClock,
                                    dest,
                                    orientation
#ifdef ADDON_BOEINGFCS
                    , zValue
#endif
                    );

        if (GroupDebugNodeMobility(nodeId))
        {
            char clockString[MAX_STRING_LENGTH];

            ctoa(simClock, clockString);

            printf("    %s (%.2f, %.2f)\n",
                   clockString, dest.common.c1, dest.common.c2);
        }

        current = dest;
    }

    return;
}

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
                                int numNodes)
{
    NodeAddress* groupNodeIdList;
    int numGroupNodes;
    Coordinates groupTerrainOrigin = {{{0,0,0}}};
    Coordinates groupTerrainDimensions = {{{0,0,0}}};

    int numGroups;
    char groupString[MAX_STRING_LENGTH];
    int groupIndex;
    BOOL found;
    BOOL retVal;

    groupNodeIdList = (NodeAddress* )
        MEM_malloc(MOBILITY_GROUP_INITIAL_SIZE * sizeof(NodeAddress));
    ERROR_Assert(groupNodeIdList != NULL, "Memory allocation error!");

    // found out in which group is myself.

    // Get number of groups
    numGroups = 0;
    IO_ReadInt(nodeId,
               ANY_ADDRESS,
               nodeInput,
               "NUM-MOBILITY-GROUPS",
               &retVal,
               &numGroups);

    if (numGroups <= 0)
    {
        return;
    }

    groupIndex = 0;
    found = FALSE;
    groupIndex = 0;
    while ((!found) && (groupIndex < numGroups))
    {
        IO_ReadStringInstance(
            nodeId,
            ANY_ADDRESS,
            nodeInput,
            "MOBILITY-GROUP",
            groupIndex,
            FALSE,
            &retVal,
            groupString);

        if (retVal == TRUE)
        {
            int j;

            numGroupNodes = 0;

            retVal = GROUP_ParseInputFileGroupLine(
                         nodeInput,
                         groupString,
                         groupIndex,
                         &groupNodeIdList,
                         &numGroupNodes,
                         &groupTerrainOrigin,
                         &groupTerrainDimensions,
                         nodePositions,
                         numNodes);

            for (j = 0; j < numGroupNodes; j ++)
            {
                if (nodeId == groupNodeIdList[j])
                {
                    found = TRUE;

                    if (retVal == FALSE)
                    {
                        // no definition of the group area
                        groupTerrainOrigin = terrainData->getOrigin();
                        groupTerrainDimensions = terrainData->getDimensions();
                    }
                }
            }
        }

        groupIndex ++;
    }

    if (found)
    {
        MOBILITY_GroupMobility(nodeId,
                               mobilityData,
                               terrainData,
                               nodeInput,
                               maxSimClock,
                               groupIndex - 1,
                               groupTerrainOrigin,
                               groupTerrainDimensions);
    }

    MEM_free(groupNodeIdList);
}
