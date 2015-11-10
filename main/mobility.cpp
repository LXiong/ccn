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
#include <limits.h>
#include <limits>

#include "api.h"
#include "partition.h"

#ifdef PARALLEL //Parallel
#include "parallel.h"
#endif

#ifdef WIRELESS_LIB
#include "mobility_group.h"
#include "mobility_waypoint.h"
#endif // WIRELESS_LIB

#ifdef CELLULAR_LIB
#include "cellular_layer3.h"
#include "cellular_abstract_layer3.h"
#endif // CELLULAR_LIB

#ifdef AGI_INTERFACE
#include "agi_interface_util.h"
#endif

#define DEST_ARRAY_INCREMENTS 100

#define MOBILITY_POSITION_DEBUG 0

// /**
// FUNCTION :: MOBILITY_AllocateNodePositions
// PURPOSE :: Allocates memory for nodePositions and mobilityData
//            Note: This function is called before NODE_CreateNode().
//                  It cannot access Node structure
// PARAMETERS ::
// + numNodes                : int : number of nodes
// + nodeIdArray             : NodeAddress* : array of nodeId
// + nodePositions           : NodePositions** : pointer to the array
//                                               to be allocated.
// + nodePlacementTypeCounts : int** : array of placement type counts
// + nodeInput               : NodeInput* : configuration input
// + seedVal                 : int : seed for random number seeds
// RETURN :: void
// **/
void MOBILITY_AllocateNodePositions(
    int numNodes,
    NodeAddress* nodeIdArray,
    NodePositions** nodePositionsPtr,
    int** nodePlacementTypeCountsPtr,
    NodeInput* nodeInput,
    int seedVal)
{
    BOOL wasFound;
    char buf[MAX_STRING_LENGTH];
    int i;
    NodePositions* nodePositions =
        (NodePositions*)MEM_malloc(sizeof(NodePositions) * numNodes);
    int* nodePlacementTypeCounts =
        (int*)MEM_malloc(sizeof(int) * NUM_NODE_PLACEMENT_TYPES);

    // Initialize nodePlacementTypeCounts.
    for (i = 0; i < NUM_NODE_PLACEMENT_TYPES; i++) {
        nodePlacementTypeCounts[i] = 0;
    }

    for (i = 0; i < numNodes; i++) {
        nodePositions[i].nodeId = nodeIdArray[i];
        nodePositions[i].partitionId = 0;

        //
        // Set up nodePositions[i].nodePlacementType.
        //
        IO_ReadString(
            nodeIdArray[i],
            ANY_ADDRESS,
            nodeInput,
            "NODE-PLACEMENT",
            &wasFound,
            buf);

        if (wasFound != TRUE) {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage,
                    "NODE-PLACEMENT is not found for a node (Id: %u)\n",
                    nodeIdArray[i]);

            ERROR_ReportError(errorMessage);
        }


        if (strcmp(buf, "RANDOM") == 0) {
            nodePositions[i].nodePlacementType = RANDOM_PLACEMENT;
            nodePlacementTypeCounts[RANDOM_PLACEMENT]++;
        }
        else if (strcmp(buf, "UNIFORM") == 0) {
            nodePositions[i].nodePlacementType = UNIFORM_PLACEMENT;
            nodePlacementTypeCounts[UNIFORM_PLACEMENT]++;
        }
        else if (strcmp(buf, "GRID") == 0) {
            nodePositions[i].nodePlacementType = GRID_PLACEMENT;
            nodePlacementTypeCounts[GRID_PLACEMENT]++;
        }
        else if (strcmp(buf, "FILE") == 0) {
            nodePositions[i].nodePlacementType = FILE_BASED_PLACEMENT;
            nodePlacementTypeCounts[FILE_BASED_PLACEMENT]++;
        }
        else if (strcmp(buf, "GROUP") == 0) {
            nodePositions[i].nodePlacementType = GROUP_PLACEMENT;
            nodePlacementTypeCounts[GROUP_PLACEMENT]++;
        }
        else if (strcmp(buf, "EXTERNAL") == 0) {
            nodePositions[i].nodePlacementType = EXTERNAL_PLACEMENT;
            nodePlacementTypeCounts[EXTERNAL_PLACEMENT]++;
        }
        else {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage, "Unknown NODE-PLACEMENT type: %s.\n", buf);
            ERROR_ReportError(errorMessage);
        }

        //
        // Allocate and pre-initialize nodePositions[i].mobilityData.
        //
        nodePositions[i].mobilityData =
            (MobilityData*)MEM_malloc(sizeof(MobilityData));
        memset(nodePositions[i].mobilityData, 0, sizeof(MobilityData));

        MOBILITY_PreInitialize(
            nodeIdArray[i],
            nodePositions[i].mobilityData,
            nodeInput,
            seedVal);

        //
        // Consistency check.
        //
        if (nodePositions[i].nodePlacementType != FILE_BASED_PLACEMENT &&
            nodePositions[i].mobilityData->mobilityType == FILE_BASED_MOBILITY)
        {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage,
                    "Bad combination of NODE-PLACEMENT and MOBILITY "
                    "for a node (Id: %u):\n"
                    "\"MOBILITY FILE\" requires \"NODE-PLACEMENT FILE\"",
                    nodeIdArray[i]);

            ERROR_ReportError(errorMessage);
        }
    }

    *nodePositionsPtr = nodePositions;
    *nodePlacementTypeCountsPtr = nodePlacementTypeCounts;
}


// /**
// FUNCTION :: MOBILITY_PreInitialize
// PURPOSE :: Initializes most variables in mobilityData.
//            (Node positions are set in MOBILITY_SetNodePositions().)
//            Note: This function is called before NODE_CreateNode().
//                  It cannot access Node structure
// PARAMETERS ::
// + nodeId       : NodeAddress : nodeId
// + mobilityData : MobilityData* : mobilityData to be initialized
// + nodeInput    : NodeInput* : configuration input
// + seedVal      : int : seed for random number seeds
// RETURN :: void
// **/
void MOBILITY_PreInitialize(
    NodeAddress nodeId,
    MobilityData* mobilityData,
    NodeInput* nodeInput,
    int seedVal)
{
    int i;
    BOOL wasFound;
    char buf[MAX_STRING_LENGTH];
    BOOL returnVal;

    // Set mobilityType.
    IO_ReadString(
        nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MOBILITY",
        &wasFound,
        buf);

    if (wasFound) {
        if (strcmp(buf, "NONE") == 0) {
            mobilityData->mobilityType = NO_MOBILITY;
        }
        else if (strcmp(buf, "RANDOM-WAYPOINT") == 0) {
            mobilityData->mobilityType = RANDOM_WAYPOINT_MOBILITY;
        }
        else if (strcmp(buf, "GROUP-MOBILITY") == 0) {
            mobilityData->mobilityType = GROUP_MOBILITY;
        }
        else if (strcmp(buf, "TRACE") == 0) {
            ERROR_ReportWarning(
                "\"MOBILITY TRACE\" is obsolete; "
                "use \"MOBILITY FILE\" instead.\n");
            mobilityData->mobilityType = FILE_BASED_MOBILITY;
        }
        else if (strcmp(buf, "FILE") == 0) {
            mobilityData->mobilityType = FILE_BASED_MOBILITY;
        }
        else {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage, "Unknown MOBILITY type: %s.\n", buf);
            ERROR_ReportError(errorMessage);
        }
    }
    else {
        mobilityData->mobilityType = NO_MOBILITY;
    }

    // Set distanceGranularity.
    Float32 distGran = 0;

    // Always set the distanceGranularity because dead reckoning will read
    // this value to determine steps.

    IO_ReadFloat(
        nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MOBILITY-POSITION-GRANULARITY",
        &wasFound,
        &distGran);

    mobilityData->distanceGranularity = distGran;

    if (!wasFound) {
        mobilityData->distanceGranularity = DEFAULT_DISTANCE_GRANULARITY;
    }

    // Set groundNode.
    IO_ReadBool(
        nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MOBILITY-GROUND-NODE",
        &wasFound,
        &returnVal);

    if (wasFound != TRUE) {
        mobilityData->groundNode = FALSE;
    } else {
        mobilityData->groundNode = returnVal;
    }

    // Ignore deprecated mobilityStats.
    BOOL mobilityStats = FALSE;
    // static to print warning message once only
    static BOOL mobilityStats_warningWasPrinted = FALSE;
    IO_ReadBool(
        nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MOBILITY-STATISTICS",
        &wasFound,
        &mobilityStats);
    if (wasFound && !mobilityStats_warningWasPrinted) {
       mobilityStats_warningWasPrinted = TRUE;
       ERROR_ReportWarning("\"MOBILITY-STATISTICS\" has been deprecated.\n");
    }

    // Set random number seeds.
    RANDOM_SetSeed(mobilityData->seed,
                   seedVal,
                   nodeId);

    // Set initial sequenceNum.
    mobilityData->sequenceNum = 0;

    // Allocate next, current and past.
    // Actual positions are assigned in MOBILITY_SetNodePositions()
    // so negative (invalid) sequence number is set.
    mobilityData->next =
        (MobilityElement*)MEM_malloc(sizeof(MobilityElement));
    memset(mobilityData->next, 0, sizeof(MobilityElement));

    mobilityData->next->sequenceNum = -1;
    mobilityData->next->time = CLOCKTYPE_MAX;

    mobilityData->current =
        (MobilityElement*)MEM_malloc(sizeof(MobilityElement));
    memset(mobilityData->current, 0, sizeof(MobilityElement));

    mobilityData->current->sequenceNum = -1;
    mobilityData->current->time = 0;

    for (i = 0; i < NUM_PAST_MOBILITY_EVENTS; i++) {
        mobilityData->past[i] =
            (MobilityElement*)MEM_malloc(sizeof(MobilityElement));
        memset(mobilityData->past[i], 0, sizeof(MobilityElement));

        mobilityData->past[i]->sequenceNum = -1;
        mobilityData->past[i]->time = 0;
    }

    // Initialize destArray.
    mobilityData->numDests = 0;
    mobilityData->destArray = NULL;

    // Initialize mobilityVar.

    mobilityData->mobilityVar = NULL;
}


// /**
// FUNCTION :: MOBILITY_PostInitialize
// PURPOSE :: Initializes variables in mobilityData not initialized by
//            MOBILITY_PreInitialize().
// PARAMETERS ::
// + node      : Node* : node being initialized
// + nodeInput : NodeInput* : structure containing contents of input file
// RETURN :: void
// **/
void MOBILITY_PostInitialize(Node *node, NodeInput *nodeInput) {
    MobilityData *mobilityData = node->mobilityData;
    MobilityRemainder *remainder = &(mobilityData->remainder);

    D_Hierarchy *hierarchy = &node->partitionData->dynamicHierarchy;

    //
    // Initialize other mobility variables.
    //
    remainder->numMovesToNextDest = 0;
    remainder->destCounter = 0;
    remainder->moveInterval = 0;
    remainder->nextMoveTime = 0;
    remainder->nextPosition = mobilityData->current->position;
    remainder->nextOrientation = mobilityData->current->orientation;

    if (node->mobilityData->numDests > 1) {
        // this is for node placement from file
        MOBILITY_NextPosition(node, mobilityData->next);
    }
    else {
        mobilityData->next->time = CLOCKTYPE_MAX;
    }

    if (hierarchy->IsEnabled()) {
        BOOL createMobilityGroundNodePath = FALSE;
        std::string mobilityGroundNodePath;

        // Create Mobility path
        createMobilityGroundNodePath =
            hierarchy->CreateNodeMobilityPath(
                node,
                "MOBILITY-GROUND-NODE",
                mobilityGroundNodePath);

        // Add object to path
        if (createMobilityGroundNodePath) {
            hierarchy->AddObject(
                mobilityGroundNodePath,
                new D_BOOLObj(&mobilityData->groundNode));
        }


        BOOL createMobilityPosGranPath = FALSE;
        std::string mobilityPosGranPath;

        // Create Mobility path
        createMobilityPosGranPath =
            hierarchy->CreateNodeMobilityPath(
                node,
                "MOBILITY-POSITION-GRANULARITY",
                mobilityPosGranPath);

        // Add object to path
        if (createMobilityPosGranPath) {
            hierarchy->AddObject(
                mobilityPosGranPath,
                new D_Float32Obj(&mobilityData->distanceGranularity));
        }
    }

    MOBILITY_InsertEvent(&(node->partitionData->mobilityHeap), node);
}


// /**
// API :: MOBILITY_Finalize
// PURPOSE :: Called at the end of simulation to collect the results of
//            the simulation of the mobility data.
// PARAMETERS ::
// + node : Node* : Node for which results are to be collected.
// RETURN :: void
// **/
void MOBILITY_Finalize(Node *node) {
    if (node->mobilityData->numDests > 0) {
        MEM_free(node->mobilityData->destArray);
    }

    return;
}


// /**
// API :: MOBILITY_ProcessEvent
// PURPOSE :: Models the behaviour of the mobility models on receiving
//            a message.
// PARAMETERS ::
// + node : Node* : Node which received the message
// RETURN :: void
// **/
void MOBILITY_ProcessEvent(Node* node) {
    MobilityHeap *heapPtr;
    MobilityData *mobilityData = node->mobilityData;
    clocktype currentTime = node->getNodeTime();
    MobilityElement* tmp;
    int index = mobilityData->current->sequenceNum % NUM_PAST_MOBILITY_EVENTS;

    node->partitionData->numberOfMobilityEvents++;

    //
    // Set the next position.
    //
    tmp = mobilityData->past[index];

    mobilityData->past[index] = mobilityData->current;
    mobilityData->current = mobilityData->next;

    MOBILITY_NextPosition(node, tmp);
#ifdef CELLULAR_LIB
    if (node->networkData.cellularLayer3Var
        && node->networkData.cellularLayer3Var->cellularAbstractLayer3Data
        && node->networkData.cellularLayer3Var->cellularAbstractLayer3Data
        ->optLevel == CELLULAR_ABSTRACT_OPTIMIZATION_MEDIUM)
    {
        CellularAbstractLayer3Callback(node);
    }
#endif // CELLULAR_LIB

    mobilityData->next = tmp;

#ifdef ADDON_BOEINGFCS
    // check if mobility ground node is set to yes
    if (mobilityData->groundNode == TRUE &&
        mobilityData->current->movingToGround == FALSE)
    {
        // update current position
        TerrainData* terrainData;
        terrainData = NODE_GetTerrainPtr(node);

        TERRAIN_SetToGroundLevel(terrainData,
            &(mobilityData->current->position));
    }
#endif

//GuiStart
    char key[MAX_STRING_LENGTH];
    char positionString[MAX_STRING_LENGTH];
    sprintf(key, "/node/%d/position", node->nodeId);
    sprintf(positionString, "%.12f,%.12f,%.12f",
        mobilityData->current->position.common.c1,
        mobilityData->current->position.common.c2,
        mobilityData->current->position.common.c3);
    SetOculaProperty(
        node->partitionData,
        key,
        positionString);
    if (node->guiOption == TRUE) {
        GUI_MoveNode(node->nodeId,
                     mobilityData->current->position,
                     currentTime + getSimStartTime(node));
        GUI_SetNodeOrientation(
            node->nodeId,
            mobilityData->current->orientation,
            currentTime + getSimStartTime(node));
    }
//GuiEnd

#ifdef ADDON_BOEINGFCS
    if (MOBILITY_POSITION_DEBUG) {
        char timeStr[24];
        TIME_PrintClockInSecond(node->getNodeTime() +
                getSimStartTime(node),
            timeStr);

        printf("At Time %s NodeId: %d (%f, %f, %f)\n",
               timeStr,
               node->nodeId,
               mobilityData->current->position.common.c1,
               mobilityData->current->position.common.c2,
               mobilityData->current->position.common.c3);

    }
#endif
    heapPtr = &(node->partitionData->mobilityHeap);
    assert(heapPtr->heapNodePtr[1] == node);
    MOBILITY_HeapFixDownEvent(heapPtr, 1);
}

// /**
// FUNCTION  :: IncreaseDestArrayIfNecessary
// PURPOSE   :: Increase the size of destArray if necessary
//              To be always called before or after setting values to the array
// PARAMETERS::
// + mobilityData  : MobilityData* : mobilityData of the node
// RETURN    :: void
// **/
static
void IncreaseDestArrayIfNecessary(MobilityData *mobilityData) {
    if (mobilityData->numDests % DEST_ARRAY_INCREMENTS == 0) {
        const int numDests = mobilityData->numDests;
        MobilityElement *oldArray = mobilityData->destArray;

        mobilityData->destArray =
            (MobilityElement*)MEM_malloc((numDests + DEST_ARRAY_INCREMENTS)
                                         * sizeof(MobilityElement));

        if (oldArray != NULL) {
            memcpy(mobilityData->destArray,
                   oldArray,
                   numDests * sizeof(MobilityElement));
            MEM_free(oldArray);
        }
    }
    return;
}



// /**
// API :: MOBILITY_AddANewDestination
// PURPOSE :: Adds a new destination.
// PARAMETERS ::
// + mobilityData : MobilityData* : MobilityData of the node
// + arrivalTime  : clocktype     : Arrival time
// + dest         : Coordinates   : Destination
// + orientation  : Orientation   : Orientation
// RETURN :: void
// **/
void MOBILITY_AddANewDestination(
    MobilityData* mobilityData,
    clocktype arrivalTime,
    Coordinates dest,
    Orientation orientation,
    double zValue)
{
    const int index = mobilityData->numDests;

    IncreaseDestArrayIfNecessary(mobilityData);

    if (index > 0) {
        if (mobilityData->destArray[index - 1].time == arrivalTime) {
            ERROR_ReportError(
                "Multiple destinations are specified "
                "with the same timestamp.\n");
        }
        else if (mobilityData->destArray[index - 1].time > arrivalTime) {
            ERROR_ReportError(
                "Destinations are not specified in timestamp order.\n");
        }
    }

    mobilityData->destArray[index].time = arrivalTime;
    mobilityData->destArray[index].position = dest;
    mobilityData->destArray[index].orientation = orientation;
#ifdef ADDON_BOEINGFCS
    mobilityData->destArray[index].zValue = zValue;
#endif
    mobilityData->numDests++;

    return;
}


// /**
// API :: MOBILITY_ReturnCoordinates
// PURPOSE :: Returns the coordinate.
// PARAMETERS ::
// + node     : Node*       : Pointer to node.
// + position : Coordinates : Position of the node.
// RETURN :: void
// **/
void MOBILITY_ReturnCoordinates(const Node* node, Coordinates* position) {
#ifdef AGI_INTERFACE
    if (node->partitionData->isAgiInterfaceEnabled)
    {
        CAgiInterfaceUtil::LLA pos;
        CAgiInterfaceUtil::GetInstance().ComputeNodePosition(node->nodeId , node->getNodeTime(), pos);
        position->latlonalt.latitude = pos.latitude_degrees;
        position->latlonalt.longitude = pos.longitude_degrees;
        position->latlonalt.altitude = pos.altitude_meters;
        position->type = GEODETIC;
    }
    else
#endif
    {
        *position = node->mobilityData->current->position;
    }
}


// /**
// API :: MOBILITY_ReturnOrientation
// PURPOSE :: Returns the node orientation.
// PARAMETERS ::
// + node        : Node*        : Pointer to node.
// + orientation : Orientation* : Pointer to Orientation.
// RETURN :: void
// **/
void MOBILITY_ReturnOrientation(const Node* node, Orientation* orientation) {
#ifdef AGI_INTERFACE
    if (node->partitionData->isAgiInterfaceEnabled)
    {
        CAgiInterfaceUtil::AzEl azEl;
        CAgiInterfaceUtil::GetInstance().ComputeNodeOrientation(node->nodeId , node->getNodeTime(), azEl);
        orientation->azimuth = azEl.azimuth_degrees;
        orientation->elevation = azEl.elevation_degrees;
    }
    else
#endif
    {
        *orientation = node->mobilityData->current->orientation;
    }
}


// /**
// API :: MOBILITY_ReturnInstantaneousSpeed
// PURPOSE :: Returns instantaneous speed of a node.
// PARAMETERS ::
// + node     : Node*     : Pointer to node.
// + speed    : double*   : Speed of the node, double pointer.
// RETURN :: void
// **/
void MOBILITY_ReturnInstantaneousSpeed(const Node* node, double* speed) {
    *speed = node->mobilityData->current->speed;
}


// /**
// API :: MOBILITY_ReturnSequenceNum
// PURPOSE :: Returns a sequence number for the current position.
// PARAMETERS ::
// + node        : Node* : Pointer to node.
// + sequenceNum : int*  : Sequence number.
// RETURN :: void
// **/
void MOBILITY_ReturnSequenceNum(const Node* node, int* sequenceNum) {
    *sequenceNum = node->mobilityData->current->sequenceNum;
}

#ifdef ADDON_BOEINGFCS
/*
 * FUNCTION    MOBILITY_ChangeGroundNode
 * PURPOSE     Change Current Node to ground level or vice versa and
 *             recalculate path.
 *
*/
void MOBILITY_ChangeGroundNode(Node* node, BOOL before, BOOL after) {
    MobilityData* mobilityData = node->mobilityData;
    MobilityRemainder* remainder = &(mobilityData->remainder);
    MobilityHeap *heapPtr;
    int coordinateSystem =
        NODE_GetTerrainPtr(node)->getCoordinateSystem();

     // if ground node value doesn't change,do nothing.
    if (before == after) {
        return;
    }
    // change ground node level from Yes to No.
    if (before == TRUE) {

        if ((remainder->destCounter < mobilityData->numDests) &&
             (mobilityData->numDests != 1))
        {
            // recalculate new path using current position
            // and next dest in destArray.

            // get current and next position.
            Coordinates* current = NULL;
            Coordinates* next;
            Coordinates distanceDiff;

            current = &mobilityData->current->position;
            next =
                &(mobilityData->destArray[remainder->destCounter].position);

            // change next move to no ground level;
            next->common.c3 =
                mobilityData->destArray[remainder->destCounter].zValue;

            // need to update old value set by next_position in
            // function "MOBILITY_NextPosition"
            // change next position and time to be the current position.
            remainder->nextPosition.common.c1 =current->common.c1;
            remainder->nextPosition.common.c2 =current->common.c2;
            remainder->nextPosition.common.c3 =current->common.c3;
            remainder->nextMoveTime = node->getNodeTime();

            // calculate time difference
            clocktype timeDiff =
                mobilityData->destArray[remainder->destCounter].time -
                node->getNodeTime();

            if (timeDiff < 0) {
                char errorStr[MAX_STRING_LENGTH] = "";
                char currentTimeStr[24] = "";
                char nextTimeStr[24] = "";

                TIME_PrintClockInSecond(node->getNodeTime() +
                        getSimStartTime(node),
                    currentTimeStr);

                TIME_PrintClockInSecond(
                    mobilityData->destArray[remainder->destCounter].time +
                            getSimStartTime(node),
                        nextTimeStr);

                sprintf(errorStr,
                        "ERROR: Current time > next position"
                        " time  (%sS > %sS)\n",
                        currentTimeStr,
                        nextTimeStr);
                ERROR_ReportError(errorStr);
            }

            // calculate distance between two points
            distanceDiff.common.c1 =
                next->common.c1 - current->common.c1;
            distanceDiff.common.c2 =
                next->common.c2 - current->common.c2;
            distanceDiff.common.c3 =
                next->common.c3 - current->common.c3;

            if (coordinateSystem == LATLONALT) {
                COORD_NormalizeLongitude(&(distanceDiff));
            }

            // if the two locations are not the same
            if (distanceDiff.common.c1 != 0.0 ||
                distanceDiff.common.c2 != 0.0 ||
                distanceDiff.common.c3 != 0.0)
            {
                double distance;

                COORD_CalcDistance(
                    NODE_GetTerrainPtr(node)->getCoordinateSystem(),
                    next, current, &distance);

                remainder->numMovesToNextDest =
                    (int) ceil(distance /
                        mobilityData->distanceGranularity);

                remainder->moveInterval =
                    timeDiff / remainder->numMovesToNextDest;

                remainder->delta.common.c1 =
                    distanceDiff.common.c1 / remainder->numMovesToNextDest;
                remainder->delta.common.c2 =
                    distanceDiff.common.c2 / remainder->numMovesToNextDest;
                remainder->delta.common.c3 =
                    distanceDiff.common.c3 / remainder->numMovesToNextDest;

                remainder->speed =
                    distance *
                        ((double)SECOND / (double)timeDiff);

            }
            // if the two locations are the same
            // set delta x,y,z to be 0.0;
            // set moveInterval to be the total time difference
            // set speed to 0 since node is not moving from one
            // point to other
            else {
                remainder->numMovesToNextDest = 1;
                remainder->moveInterval = timeDiff;
                remainder->delta.common.c1 = 0.0;
                remainder->delta.common.c2 = 0.0;
                remainder->delta.common.c3 = 0.0;
                remainder->speed = 0.0;
            }
            remainder->numMovesToNextDest--;


            if (remainder->numMovesToNextDest == 0) {
                remainder->nextMoveTime =
                    mobilityData->destArray[remainder->destCounter].time;
                remainder->nextPosition =
                    mobilityData->destArray[remainder->destCounter].position;
                remainder->nextOrientation =
                    mobilityData->destArray[remainder->destCounter].orientation;
                remainder->speed = 0.0;
            }
            else {
                remainder->nextMoveTime += remainder->moveInterval;

                remainder->nextPosition.common.c1 +=
                    remainder->delta.common.c1;

                remainder->nextPosition.common.c2 +=
                    remainder->delta.common.c2;

                remainder->nextPosition.common.c3 +=
                    remainder->delta.common.c3;

                if (coordinateSystem == LATLONALT) {
                    COORD_NormalizeLongitude(&(remainder->nextPosition));
                }
            }

            // update next mobility event
            mobilityData->sequenceNum++;
            mobilityData->next->sequenceNum = mobilityData->sequenceNum;
            mobilityData->next->time = remainder->nextMoveTime;
            mobilityData->next->position = remainder->nextPosition;
            mobilityData->next->orientation = remainder->nextOrientation;
            mobilityData->next->speed = remainder->speed;
            mobilityData->next->movingToGround = FALSE;

            // update current mobility speed
            mobilityData->current->speed = remainder->speed;
        }

        // if it is the last reference point in (.mobility/.nodes) file,
        // set z-value to be No-ground-level.
        // notices there will be a jump from  ground level
        // to non-ground level.
        else {
            mobilityData->current->position.common.c3 =
                mobilityData->destArray[mobilityData->numDests-1].zValue;
            // update position in GUI
            char key[MAX_STRING_LENGTH];
            char positionString[MAX_STRING_LENGTH];
            sprintf(key, "/node/%d/position", node->nodeId);
            sprintf(positionString, "%.12f,%.12f,%.12f",
                mobilityData->current->position.common.c1,
                mobilityData->current->position.common.c2,
                mobilityData->current->position.common.c3);
            SetOculaProperty(
                node->partitionData,
                key,
                positionString);
            if (node->guiOption == TRUE) {
                GUI_MoveNode(
                    node->nodeId,
                    mobilityData->current->position,
                    node->getNodeTime() + getSimStartTime(node));
                GUI_SetNodeOrientation(
                    node->nodeId,
                    mobilityData->current->orientation,
                    node->getNodeTime() + getSimStartTime(node));
            }
            if (MOBILITY_POSITION_DEBUG) {
                char timeStr[24];

                TIME_PrintClockInSecond(node->getNodeTime()+
                                        getSimStartTime(node),
                                        timeStr);

                printf("At Time %s NodeId: %d (%f, %f, %f)\n",
                       timeStr,
                       node->nodeId,
                       mobilityData->current->position.common.c1,
                       mobilityData->current->position.common.c2,
                       mobilityData->current->position.common.c3);
            }
            mobilityData->next->time = CLOCKTYPE_MAX;
            return;
        }
        // rebuild the heap so that the root will have min. value
        heapPtr = &(node->partitionData->mobilityHeap);
        MOBILITY_BuildMinHeap(node, heapPtr);
    }
    // change ground node level from no to yes
    else {
        if ((remainder->destCounter < mobilityData->numDests) &&
             (mobilityData->numDests != 1) )
        {
            // set moving to ground node = true
            remainder->movingToGround = TRUE;
            mobilityData->current->movingToGround = TRUE;

            // recalculate the intermediate step
            // get current and next position
            Coordinates* current = NULL;
            Coordinates* next = NULL;
            Coordinates distanceDiff;

            current =
                &mobilityData->current->position;
            next =
                &(mobilityData->destArray[remainder->destCounter].position);

            // set next position to ground level
            TerrainData* terrainData;

            // get terrain data
            terrainData = NODE_GetTerrainPtr(node);

            // set to ground level
            TERRAIN_SetToGroundLevel(terrainData, next);

            // need to update old value set by next_position
            // in function "MOBILITY_NextPosition"
            // change next position and time to be the current position.
            remainder->nextPosition.common.c1 = current->common.c1;
            remainder->nextPosition.common.c2 = current->common.c2;
            remainder->nextPosition.common.c3 = current->common.c3;
            remainder->nextMoveTime = node->getNodeTime();

            // calculate time difference
            clocktype timeDiff =
                mobilityData->destArray[remainder->destCounter].time -
                node->getNodeTime();

            if (timeDiff < 0) {
                char errorStr[MAX_STRING_LENGTH] = "";
                char currentTimeStr[24] = "";
                char nextTimeStr[24] = "";

                TIME_PrintClockInSecond(
                    node->getNodeTime()+
                    getSimStartTime(node),
                    currentTimeStr);
                TIME_PrintClockInSecond(
                    mobilityData->destArray[remainder->destCounter].time +
                    getSimStartTime(node),
                    nextTimeStr);

                sprintf(errorStr,
                        "ERROR: Current time > next position"
                        " time  (%sS > %sS)\n",
                        currentTimeStr,
                        nextTimeStr);
                ERROR_ReportError(errorStr);
            }

            // calculate distance between two points
            distanceDiff.common.c1 =
                next->common.c1 - current->common.c1;
            distanceDiff.common.c2 =
                next->common.c2 - current->common.c2;
            distanceDiff.common.c3 =
                next->common.c3 - current->common.c3;

            if (coordinateSystem == LATLONALT) {
                COORD_NormalizeLongitude(&distanceDiff);
            }

            // if the two locations are not the same
            if (distanceDiff.common.c1 != 0.0 ||
                distanceDiff.common.c2 != 0.0 ||
                distanceDiff.common.c3 != 0.0)
            {
                double distance;

                COORD_CalcDistance(
                    NODE_GetTerrainPtr(node)->getCoordinateSystem(),
                    next, current, &distance);

                remainder->numMovesToNextDest =
                    (int) ceil(distance /
                        mobilityData->distanceGranularity);

                remainder->moveInterval =
                    timeDiff / remainder->numMovesToNextDest;

                remainder->delta.common.c1 =
                    distanceDiff.common.c1 / remainder->numMovesToNextDest;
                remainder->delta.common.c2 =
                    distanceDiff.common.c2 / remainder->numMovesToNextDest;
                remainder->delta.common.c3 =
                    distanceDiff.common.c3 / remainder->numMovesToNextDest;

                remainder->speed = distance *
                    ((double)SECOND / (double)timeDiff);
            }
            // if the two locations are the same
            // set delta x, y, z to be 0.0;
            // set moveInterval to be the total time difference
            // set speed to 0 since node is not moving from one point to other
            else {
                remainder->numMovesToNextDest = 1;
                remainder->moveInterval = timeDiff;
                remainder->delta.common.c1 = 0.0;
                remainder->delta.common.c2 = 0.0;
                remainder->delta.common.c3 = 0.0;
                remainder->speed = 0.0;
            }
            remainder->numMovesToNextDest--;

            if (remainder->numMovesToNextDest == 0) {
                remainder->nextMoveTime =
                    mobilityData->destArray[remainder->destCounter].time;
                remainder->nextPosition =
                    mobilityData->destArray[remainder->destCounter].position;
                remainder->nextOrientation =
                    mobilityData->destArray[remainder->destCounter].orientation;
                remainder->speed = 0.0;
            }
            else {
                remainder->nextMoveTime += remainder->moveInterval;

                remainder->nextPosition.common.c1 +=
                    remainder->delta.common.c1;

                remainder->nextPosition.common.c2 +=
                    remainder->delta.common.c2;

                remainder->nextPosition.common.c3 +=
                    remainder->delta.common.c3;

                if (coordinateSystem == LATLONALT) {
                    COORD_NormalizeLongitude(&(remainder->nextPosition));
                }
            }

            // update next mobility event
            mobilityData->sequenceNum++;
            mobilityData->next->sequenceNum = mobilityData->sequenceNum;
            mobilityData->next->time = remainder->nextMoveTime;
            mobilityData->next->position = remainder->nextPosition;
            mobilityData->next->orientation = remainder->nextOrientation;
            mobilityData->next->speed = remainder->speed;
            mobilityData->next->movingToGround = remainder->movingToGround;
            // update current mobility speed
            mobilityData->current->speed = remainder->speed;
        }

        // if it is the last reference point in (.mobility/.nodes) file,
        // set z-value to be ground-level.
        // notices there will be a jump from non- ground level to
        // ground level.
        else {
            mobilityData->current->movingToGround = FALSE;
            // get node to ground level;
            TerrainData* terrainData;
            terrainData = NODE_GetTerrainPtr(node);

            TERRAIN_SetToGroundLevel(terrainData,
                &(mobilityData->current->position));

            // update position for GUI
            char key[MAX_STRING_LENGTH];
            char positionString[MAX_STRING_LENGTH];
            sprintf(key, "/node/%d/position", node->nodeId);
            sprintf(positionString, "%.12f,%.12f,%.12f",
                mobilityData->current->position.common.c1,
                mobilityData->current->position.common.c2,
                mobilityData->current->position.common.c3);
            SetOculaProperty(
                node->partitionData,
                key,
                positionString);
            if (node->guiOption == TRUE) {
                GUI_MoveNode(
                    node->nodeId,
                    mobilityData->current->position,
                    node->getNodeTime() + getSimStartTime(node));
                GUI_SetNodeOrientation(
                    node->nodeId,
                    mobilityData->current->orientation,
                    node->getNodeTime() + getSimStartTime(node));
            }

            if (MOBILITY_POSITION_DEBUG) {
                char timeStr[24];

                TIME_PrintClockInSecond(node->getNodeTime() +
                                        getSimStartTime(node),
                                        timeStr);

                printf("At Time %s NodeId: %d (%f, %f, %f)\n",
                       timeStr,
                       node->nodeId,
                       mobilityData->current->position.common.c1,
                       mobilityData->current->position.common.c2,
                       mobilityData->current->position.common.c3);

            }
            mobilityData->next->time = CLOCKTYPE_MAX;
            return;
        }

        // rebuild the heap so that the root will have min. value
        heapPtr = &(node->partitionData->mobilityHeap);
        MOBILITY_BuildMinHeap(node, heapPtr);
    }
}

/*
 * FUNCTION    MOBILITY_ChangePositionGranularity
 * PURPOSE     Recalculated path using the new mobility-position-granulairty
 *
*/
void MOBILITY_ChangePositionGranularity(Node* node) {
    MobilityData* mobilityData = node->mobilityData;
    MobilityRemainder* remainder = &(mobilityData->remainder);
    MobilityHeap *heapPtr;
    int coordinateSystem =
        NODE_GetTerrainPtr(node)->getCoordinateSystem();

    if ((remainder->destCounter < mobilityData->numDests) &&
         (mobilityData->numDests != 1))
    {
        // recalculate new path using current position
        // and next dest in destArray.

        // get current and next position
        Coordinates* current = NULL;
        Coordinates* next;
        Coordinates distanceDiff;

        current = &mobilityData->current->position;
        next =
            &(mobilityData->destArray[remainder->destCounter].position);

        // need to update old value set by next_position in
        // function "MOBILITY_NextPosition"
        // change next position and time to be the current position.
        remainder->nextPosition.common.c1 = current->common.c1;
        remainder->nextPosition.common.c2 = current->common.c2;
        remainder->nextPosition.common.c3 = current->common.c3;
        remainder->nextMoveTime = node->getNodeTime();

        // calculate time difference
        clocktype timeDiff =
            mobilityData->destArray[remainder->destCounter].time -
            node->getNodeTime();

        if (timeDiff < 0) {
            char errorStr[MAX_STRING_LENGTH] = "";
            char currentTimeStr[24] = "";
            char nextTimeStr[24] = "";

            TIME_PrintClockInSecond(node->getNodeTime() +
                                    getSimStartTime(node),
                                    currentTimeStr);

            TIME_PrintClockInSecond(
                mobilityData->destArray[remainder->destCounter].time +
                getSimStartTime(node),
                nextTimeStr);

            sprintf(errorStr,
                    "ERROR: Current time > next position"
                    " time  (%sS > %sS)\n",
                    currentTimeStr,
                    nextTimeStr);
            ERROR_ReportError(errorStr);
        }

        // calculate distance between two points
        distanceDiff.common.c1 =
            next->common.c1 - current->common.c1;
        distanceDiff.common.c2 =
            next->common.c2 - current->common.c2;
        distanceDiff.common.c3 =
            next->common.c3 - current->common.c3;

        if (coordinateSystem == LATLONALT) {
            COORD_NormalizeLongitude(&distanceDiff);
        }

        // if the two locations are not the same

        if (distanceDiff.common.c1 != 0.0 ||
            distanceDiff.common.c2 != 0.0 ||
            distanceDiff.common.c3 != 0.0)
        {
            double distance;

            COORD_CalcDistance(
                NODE_GetTerrainPtr(node)->getCoordinateSystem(),
                next, current, &distance);

            remainder->numMovesToNextDest =
                (int) ceil(distance /
                           mobilityData->distanceGranularity);

            remainder->moveInterval =
                timeDiff / remainder->numMovesToNextDest;

            remainder->delta.common.c1 =
                distanceDiff.common.c1 / remainder->numMovesToNextDest;
            remainder->delta.common.c2 =
                distanceDiff.common.c2 / remainder->numMovesToNextDest;
            remainder->delta.common.c3 =
                distanceDiff.common.c3 / remainder->numMovesToNextDest;

            // speed is unchange.
            remainder->speed =
                distance *
                ((double)SECOND / (double)timeDiff);
        } else {
            // if the two locations are the same
            // set delta x,y,z to be 0.0;
            // set moveInterval to be the total time difference
            // set speed to 0 since node is not moving from one point to other
            remainder->numMovesToNextDest = 1;
            remainder->moveInterval = timeDiff;
            remainder->delta.common.c1 = 0.0;
            remainder->delta.common.c2 = 0.0;
            remainder->delta.common.c3 = 0.0;
            remainder->speed = 0.0;
        }
        remainder->numMovesToNextDest--;

        if (remainder->numMovesToNextDest == 0) {
            remainder->nextMoveTime =
                mobilityData->destArray[remainder->destCounter].time;
            remainder->nextPosition =
                mobilityData->destArray[remainder->destCounter].position;
            remainder->nextOrientation =
                mobilityData->destArray[remainder->destCounter].orientation;
            remainder->speed = 0.0;
        } else {
            remainder->nextMoveTime += remainder->moveInterval;

            remainder->nextPosition.common.c1 +=
                remainder->delta.common.c1;

            remainder->nextPosition.common.c2 +=
                remainder->delta.common.c2;

            remainder->nextPosition.common.c3 +=
                remainder->delta.common.c3;

            if (coordinateSystem == LATLONALT) {
                COORD_NormalizeLongitude(&(remainder->nextPosition));
            }
        }

        // update next mobility event
        mobilityData->sequenceNum++;
        mobilityData->next->sequenceNum = mobilityData->sequenceNum;
        mobilityData->next->time = remainder->nextMoveTime;
        mobilityData->next->position = remainder->nextPosition;
        mobilityData->next->orientation = remainder->nextOrientation;
        mobilityData->next->speed = remainder->speed;
        mobilityData->next->movingToGround = FALSE;

        // update current mobility speed
        mobilityData->current->speed = remainder->speed;
    } else {
        // if it is the last reference point in ( .mobility/.nodes) file,
        // do nothing
        // return function
        return;
    }
    // rebuild the heap so that the root will have min. value
    heapPtr = &(node->partitionData->mobilityHeap);
    MOBILITY_BuildMinHeap(node, heapPtr);
}

/*
 * FUNCTION    MOBILITY_BuildMinHeap
 * PURPOSE     Rebuild the current Heap to maintain Min-heap properties by
 *             Call Min-Heapify from bottoms up.
 *
*/
void MOBILITY_BuildMinHeap(Node* node, MobilityHeap* heapPtr) {
    int i = 0;
    for (i = (heapPtr->length / 2); i > 0; i--) {
        MOBILITY_HeapFixDownEvent(heapPtr, i);
    }
}
#endif


// /**
// API :: MOBILITY_NodeIsIndoors
// PURPOSE :: Returns whether the node is indoors.
// PARAMETERS ::
// + node     : Node*       : Pointer to node.
// RETURN :: bool : returns true if indoors.
// **/
bool MOBILITY_NodeIsIndoors(const Node* node) {
    return node->mobilityData->indoors;
}


// /**
// API :: MOBILITY_SetIndoors
// PURPOSE :: Sets the node's indoor variable.
// PARAMETERS ::
// + node     : Node*       : Pointer to node.
// + indoors  : bool        : true if the node is indoors.
// RETURN :: void :
// **/
void MOBILITY_SetIndoors(Node* node, bool indoors) {
    node->mobilityData->indoors = indoors;
}

