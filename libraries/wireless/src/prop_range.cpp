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

/*
 * Calculates prop range
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "clock.h"
#include "memory.h"
#include "partition.h"

#include "WallClock.h"

int main(int argc, char **argv) {

    char  buf[MAX_STRING_LENGTH];   // used for reading data from file
    char  error[MAX_STRING_LENGTH]; // used for reporting errors
    BOOL  wasFound;

    NodeInput       nodeInput;
    int          numNodes;           // number of nodes in simulation
    NodeAddress *nodeIdArray = NULL; // will contain valid node identifiers

    int        seedVal;              // seed read from file
    BOOL       seedValSet = FALSE;   // seed set from command line

    AddressMapType *addressMapPtr;
    SimulationProperties    simProps;

    RandomSeed seed;
    PartitionData*  partitionData;

    char        experimentPrefix[MAX_STRING_LENGTH];
    NodePositions *nodePositions;
    int* nodePlacementTypeCounts;
    PropProfile*    propProfile;
    TerrainData terrainData;

    int rxNodeId = 1;
    int txNodeId = 2;

    if (argc > 2)
    {
        if (argv[2])
        {
            txNodeId = atoi(argv[2]);
        }
    }

    if (argc > 3)
    {
        if (argv[3])
        {
            rxNodeId = atoi(argv[3]);
        }
    }

    ERROR_Assert(rxNodeId > 0,
                "\nThe receiver node ID should be greater than 0\n");
    ERROR_Assert(txNodeId > 0,
                "\nThe transmitter node ID should be greater than 0\n");
    ERROR_Assert(txNodeId != rxNodeId,
                "\nPlease change the transmitter node ID or the receiver node ID\n");

    double startRealTime = WallClock::getTrueRealTimeAsDouble ();
    addressMapPtr = MAPPING_MallocAddressMap();

    simProps.configFileName =  argv[1];
    strcpy(experimentPrefix, "Radio_range.stat");

    // Read the configuration file.
    IO_InitializeNodeInput(&nodeInput, true);

    // Create partitions.
    partitionData = (PartitionData*)MEM_malloc(sizeof(PartitionData*));
    memset(partitionData, 0, sizeof(PartitionData*));
    partitionData = PARTITION_CreateEmptyPartition(0, 1);

    IO_ReadNodeInput(&nodeInput, argv[1]);

    // Read terrain information.
    terrainData.initialize(&nodeInput, true);

    // Read the seed value and initialize the seed.
    if (!seedValSet)
    {
        IO_ReadInt(
            ANY_NODEID,
            ANY_ADDRESS,
            &nodeInput,
            "SEED",
            &wasFound,
            &seedVal);

        if (wasFound == FALSE) {
            ERROR_ReportError("\"SEED\" must be specified in the "
                "configuration file");
        }

        seedValSet = TRUE;
    }

    RANDOM_SetSeed(seed, seedVal);
    RANDOM_LoadUserDistributions(&nodeInput);

    clocktype configMaxSimClock;

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        &nodeInput,
        "SIMULATION-TIME",
        &wasFound,
        buf);

    if (wasFound == FALSE) {
        ERROR_ReportError("\"SIMULATION-TIME\" must be specified in the "
            "configuration file");
    }

    char startSimClockStr[MAX_STRING_LENGTH] = "";
    char maxSimClockStr[MAX_STRING_LENGTH] = "";
    int nToken = 0;

    nToken = sscanf(buf, "%s %s", startSimClockStr, maxSimClockStr);

    if (nToken == 1) {
        simProps.startSimClock = 0;
        configMaxSimClock = TIME_ConvertToClock(startSimClockStr);
    } else {
        simProps.startSimClock = TIME_ConvertToClock(startSimClockStr);
        configMaxSimClock = TIME_ConvertToClock(maxSimClockStr);
        configMaxSimClock = configMaxSimClock - simProps.startSimClock;
    }

    nodeInput.startSimClock = simProps.startSimClock;
    simProps.maxSimClock = configMaxSimClock;

    // Build nodeId <-> IP address map.
    // Get valid nodeIds for this scenario.

    numNodes = 0;
    MAPPING_BuildAddressMap(
       &nodeInput,
       &numNodes,
       &nodeIdArray,
       addressMapPtr);

    if (numNodes == 0) {
        ERROR_ReportError("There are no nodes in the simulation"
            " specified by the configuration file.\n"
            "Check for existing and valid \"LINK\""
            ", \"SUBNET\" and \"BENT-PIPE\" statements\n");
    }

    // Allocate memory for NodePositions array.
    MOBILITY_AllocateNodePositions(
        numNodes,
        nodeIdArray,
        &nodePositions,
        &nodePlacementTypeCounts,
        &nodeInput,
        seedVal);

    // Free and sanitize nodeIdArray.
    MEM_free(nodeIdArray);
    nodeIdArray = NULL;

    // Determine node positions.
    MOBILITY_SetNodePositions(
        numNodes,
        nodePositions,
        nodePlacementTypeCounts,
        &terrainData,
        &nodeInput,
        seed,
        simProps.maxSimClock,
        simProps.startSimClock);

    int numberOfPartitions = PARALLEL_AssignNodesToPartitions(
                             numNodes,
                             1,
                             &nodeInput,
                             nodePositions,
                             (const AddressMapType*) addressMapPtr);

    PARTITION_InitializePartition(partitionData,
                                  &terrainData,
                                  simProps.maxSimClock,
                                  startRealTime,
                                  numNodes,
                                  FALSE,
                                  addressMapPtr,
                                  nodePositions,
                                  &nodeInput,
                                  seedVal,
                                  nodePlacementTypeCounts,
                                  experimentPrefix,
                                  simProps.startSimClock);

    // Perform global propagation initialization.
    PROP_GlobalInit(partitionData, &nodeInput);

    // Loading BER Tables
    PHY_GlobalBerInit(&nodeInput);
    PARTITION_InitializeNodes(partitionData);

    Node *rxnode = NULL;
    Node *txnode = NULL;

    int i;
    BOOL nodeExisting = FALSE;

    for (i = 0; i < numNodes; i++)
    {
        rxnode = partitionData->nodeData[i];

        if (rxnode->nodeId == (unsigned)rxNodeId)
        {
            nodeExisting = TRUE;
            break;
        }
    }

    ERROR_Assert(nodeExisting == TRUE,
                "\nPlease verify the receiver node ID, "
                "specified receiver node does not exist\n");

    nodeExisting = FALSE;

    for (i = 0; i < numNodes; i++)
    {
        txnode = partitionData->nodeData[i];

        if (txnode->nodeId == (unsigned)txNodeId)
        {
            nodeExisting = TRUE;
            break;
        }
    }

    ERROR_Assert(nodeExisting == TRUE,
                "\nPlease verify the transmitter node ID, "
                "specified transmitter node does not exist\n");

    int numChannels = partitionData->numChannels;
    int k = 0;
    int j;
    int m;
    double distance;

    for (k = 0; k < numChannels; k++)
    {
        if ((rxnode) && (txnode))
        {
            propProfile = txnode->partitionData->propChannel[k].profile;
            double waveLength = propProfile->wavelength;
            double frequencyMhz = 1.0e-6 * SPEED_OF_LIGHT / waveLength;

            if ((rxnode->numberPhys == 0) ||
                (txnode->numberPhys == 0))
            {
                printf("\n There is no wireless interface for "
                        "node %d or node %d\n", rxNodeId,
                        txNodeId);

                return 0;
            }

            for (m = 0; m < txnode->numberPhys; m++)
            {
                for (j = 0; j < rxnode->numberPhys; j++)
                {
                    printf("\n Channel Index = %d, Frequency = %f MHz,"
                           "\n Tx Node = %d, Interface Index = %d,"
                           "\n Rx Node = %d, Interface Index = %d,\n\n",
                           k, frequencyMhz, txNodeId, m,
                           rxNodeId, j);

                    PhyModel   rxPhyModel = rxnode->phyData[j]->phyModel;
                    PhyModel   txPhyModel = txnode->phyData[m]->phyModel;

                    if ((txPhyModel == rxPhyModel) &&
                        (rxnode->phyData[j]->channelListenable[k]
                         == TRUE) &&
                        (txnode->phyData[m]->channelListenable[k]
                         == TRUE))
                    {

                        distance =
                            PHY_PropagationRange(txnode,
                                                 rxnode,
                                                 m,
                                                 j,
                                                 k,
                                                 TRUE);
                    }
                    else
                    {
                        printf(" Radio range:  NONE.\n\n");

                    }
                }
            }
        }
    }

    terrainData.finalize();
    return 0;
}

