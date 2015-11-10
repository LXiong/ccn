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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "prop_plmatrix.h"

#define ROUNDING_ERROR_ALLOWANCE 1.0e-5
#define HIGH_PATHLOSS       1000

#ifndef USE_MPI
#include <pthread.h>

// Mutex used to protect matrix when updating in shared memory mode
pthread_mutex_t updateMutex;
#endif


BOOL sortPathLoss(pathLossMatrixValue p1, pathLossMatrixValue p2)
{

        return p1.simTime < p2.simTime;
}

// /**
// FUNCTION :: PathlossMatrixUpdate
// LAYER :: Propagation Layer
// PURPOSE :: Reads the PathLoss Matrix from a file
// PARAMETERS ::
// + partitionData : PartitionData* : PartitionData where matrix value is stored
// + propChannel : PropChannel* : Proapagation channel
// + totalNumChannels : int : total number of channels
// RETURN :: void : NULL
// **/

static void PathlossMatrixUpdate(
    PartitionData* partitionData,
    PropChannel* propChannel,
    int totalNumChannels,
    clocktype currentTime)
{

#ifndef USE_MPI
    // Protect the matrix since it is global in shared memory
    if (partitionData->isRunningInParallel())
    {
        pthread_mutex_lock(&updateMutex);
    }
#endif

    PropProfile* propProfile0 = propChannel[0].profile;
    char buffer[MAX_STRING_LENGTH];
    char* StrPtr;

    int j = 0;
    int srcNode;
    int dstNode;

    pair <NodeId, NodeId> srcdstPair;
    map < pair <NodeId, NodeId>, double> ::iterator it;

    while (TRUE)
    {
        if (partitionData->plCurrentIndex >= (int)propProfile0->matrixList.size())
        {
            break;
        }

        pathLossMatrixValue pathLossData =
                    propProfile0->matrixList.at(partitionData->plCurrentIndex);

        if (pathLossData.simTime > currentTime)
        {
            break;
        }

        partitionData->plCurrentIndex++;

        IO_GetToken(buffer,
                    pathLossData.values.c_str(),
                    &StrPtr);

        srcNode = atoi(buffer);

        IO_GetToken(buffer, StrPtr, &StrPtr);
        dstNode = atoi(buffer);

        srcdstPair = make_pair(srcNode, dstNode);

        j = 0;

        while (StrPtr != NULL && j < propProfile0->numChannelsInMatrix)
        {
            IO_GetToken(buffer, StrPtr, &StrPtr);

            if (strcmp(buffer, ""))
            {
                int channelIndex = propProfile0->channelIndexArray[j];
                it = partitionData->pathLossMatrix[channelIndex]->find(srcdstPair);

                if (it == partitionData->pathLossMatrix[channelIndex]->end())
                {

                    partitionData->pathLossMatrix[channelIndex]->insert(
                            map< pair<NodeId ,NodeId>, double>::value_type(
                                             srcdstPair,
                                             atof(buffer)));
                }
                else
                {
                    it->second = atof(buffer);
                }
            }
            j++;
        }
    }

    if (partitionData->plCurrentIndex >= (int)propProfile0->matrixList.size()) {
        partitionData->plNextLoadTime = CLOCKTYPE_MAX;
    }
    else {
        pathLossMatrixValue pathLossData =
                    propProfile0->matrixList.at(partitionData->plCurrentIndex);
        partitionData->plNextLoadTime = pathLossData.simTime;
    }

#ifndef USE_MPI
    if (partitionData->isRunningInParallel())
    {
        pthread_mutex_unlock(&updateMutex);
    }
#endif
}

// /**
// FUNCTION :: PathlossMatrixInitialize
// LAYER :: Propagation Layer
// PURPOSE :: Initializes the pathloss matrix
// PARAMETERS ::
// + propChannel : PropChannel* : Proapagation channel
// + totalNumChannels : int : total number of channels
// + nodeInput : NodeInput :Pointer to node input
// RETURN :: void : NULL
// **/

void PathlossMatrixInitialize(
    PartitionData* partitionData,
    PropChannel* propChannel,
    int totalNumChannels,
    const NodeInput *nodeInput)
{
    PropProfile* propProfile0 = propChannel[0].profile;
    char* buffer;
    BOOL wasFound;
    int i, j;
    char* StrStartPtr;
    char* StrEndPtr;
    NodeInput matrixInput;
    clocktype time = 0;

    char* StrPtr;

#ifndef USE_MPI
    pthread_mutex_init(&updateMutex, NULL);
#endif

    IO_ReadCachedFile(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-PATHLOSS-MATRIX-FILE",
        &wasFound,
        &matrixInput);

    if (!wasFound) {
        ERROR_ReportError(
            "PROPAGATION-PATHLOSS-MATRIX-FILE is not specified");
    }

    buffer = (char*) MEM_malloc(
        sizeof(char) * matrixInput.maxLineLen);
    strcpy(buffer, matrixInput.inputStrings[0]);
    assert(strncmp(buffer, "Freq:", 5) == 0);

    StrStartPtr = strchr(buffer, ':');
    StrStartPtr++;

    StrEndPtr = strchr(StrStartPtr, ':');
    *StrEndPtr = 0;
    StrEndPtr++;

    assert(propProfile0->numChannelsInMatrix == atoi(StrStartPtr));

    propProfile0->channelIndexArray =
        (int*)MEM_malloc(propProfile0->numChannelsInMatrix * sizeof(int));

    j = 0;

    for (i = 0; i < propProfile0->numChannelsInMatrix; i++) {
        double frequency;
        BOOL channelFound = FALSE;

        StrStartPtr = StrEndPtr;

        if (i != propProfile0->numChannelsInMatrix - 1) {
            StrEndPtr = strchr(StrStartPtr, ':');
            *StrEndPtr = 0;
            StrEndPtr++;
        }

        frequency = atof(StrStartPtr) * 1.0e9;

        for (; j < totalNumChannels; j++) {
            if (fabs(frequency - propChannel[j].profile->frequency) /
                frequency < ROUNDING_ERROR_ALLOWANCE)
            {
                channelFound = TRUE;
                break;
            }
        }
        if (channelFound == FALSE) {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage,
                    "could not find a channel at %f Hz",
                    frequency);
            ERROR_ReportError(errorMessage);
        }

        propProfile0->channelIndexArray[i] = j++;
    }

    strcpy(buffer, matrixInput.inputStrings[1]);
    assert(strncmp(buffer, "Nodes:", 6) == 0);

    StrStartPtr = strchr(buffer, ':');
    StrStartPtr++;

    propProfile0->numNodesInMatrix = atoi(StrStartPtr);

    //
    // Create pathlossMatrix
    //
    int listSize = sizeof(map <pair<NodeId, NodeId>, double>*) *
                   totalNumChannels;
    partitionData->pathLossMatrix = (map <pair<NodeId, NodeId>, double>**) MEM_malloc(listSize);
    memset(partitionData->pathLossMatrix, 0, listSize);

    for (i = 0; i < propProfile0->numChannelsInMatrix; i++) {
        partitionData->pathLossMatrix[propProfile0->channelIndexArray[i]] =
            new map<pair<NodeId, NodeId>, double>();
    }


    for (i = 2; i < matrixInput.numLines; i++)
    {
        pathLossMatrixValue pathLossData;
        IO_GetToken(buffer,
                    matrixInput.inputStrings[i],
                    &StrPtr);

        pathLossData.simTime = (clocktype)(atof(buffer) * SECOND);
        pathLossData.values.assign(StrPtr);

        propProfile0->matrixList.push_back(pathLossData);
    }


    if (propProfile0->matrixList.empty())
    {
        partitionData->plNextLoadTime = CLOCKTYPE_MAX;
    }
    else
    {
        sort(propProfile0->matrixList.begin(),
             propProfile0->matrixList.end(),
             sortPathLoss);
        pathLossMatrixValue pathLossData = propProfile0->matrixList.front();
        time = pathLossData.simTime;
        partitionData->plCurrentIndex = 0;
        partitionData->plNextLoadTime = pathLossData.simTime;
    }

    if (time == 0)
    {
        PathlossMatrixUpdate(
            partitionData,
            propChannel,
            totalNumChannels,
            0);
    }

    MEM_free(buffer);
}

// /**
// FUNCTION :: PathlossMatrixPartitionInit
// LAYER :: Propagation Layer
// PURPOSE :: Initializes partition level structure
// PARAMETERS ::
// + partitionData : PartitionData * : Pointer to the partition
// + nodeInput : NodeInput :Pointer to node input
// RETURN :: void : NULL
// **/

void PathlossMatrixPartitionInit(PartitionData *partitionData,
                                 NodeInput *nodeInput)
{
    PropChannel* propChannel = partitionData->propChannel;
    PropProfile* propProfile0 = propChannel[0].profile;

    if (propProfile0->numChannelsInMatrix > 0)
    {
        int listSize = sizeof(map <pair<NodeId, NodeId>, double>*) *
                       partitionData->numChannels;

        partitionData->pathLossMatrix = (map <pair<NodeId, NodeId>, double>**) MEM_malloc(listSize);
        memset(partitionData->pathLossMatrix, 0, listSize);

        // only allocate for channels configured for pathloss matrix
        for (int i = 0; i < propProfile0->numChannelsInMatrix; i++) {
            partitionData->pathLossMatrix[propProfile0->channelIndexArray[i]] =
                new map<pair<NodeId, NodeId>, double>();
        }

        if (propProfile0->matrixList.empty())
        {
            partitionData->plNextLoadTime = CLOCKTYPE_MAX;
        }
        else
        {
            pathLossMatrixValue pathLossData = propProfile0->matrixList.front();
            partitionData->plCurrentIndex = 0;
            partitionData->plNextLoadTime = pathLossData.simTime;
        }

        if (partitionData->plNextLoadTime == 0)
        {
            PathlossMatrixUpdate(
                partitionData,
                propChannel,
                propProfile0->numChannelsInMatrix,
                0);
        }
    }
}

// /**
// FUNCTION :: PathlossMatrix
// LAYER :: Propagation Layer
// PURPOSE :: Returns the pathloss between pair of nodes
// PARAMETERS ::
// + node : Node* : Node Pointer
// + nodeId1 : NodeAddress : NodeId of node 1
// + nodeId2 : NodeAddress : NodeId of node 2
// + channelIndex : int : Channel index on which pathloss is required
// + currentTime : clocktype: current simulation Time.
// RETURN :: float : Pathloss value between pair of nodes.
// **/

double PathlossMatrix(
    Node* node,
    NodeAddress nodeId1,
    NodeAddress nodeId2,
    int channelIndex,
    clocktype currentTime)
{
    PartitionData* partitionData = node->partitionData;
    PropChannel* propChannel = node->partitionData->propChannel;
    PropProfile* propProfile0 = propChannel[0].profile;

    pair <NodeId, NodeId> srcdstPair (nodeId1, nodeId2);
    map < pair<NodeId, NodeId>, double> ::iterator it;


    if (partitionData->plNextLoadTime < currentTime) {
        PathlossMatrixUpdate(
            node->partitionData,
            propChannel,
            propProfile0->numChannelsInMatrix,
            node->getNodeTime());
    }

    it = partitionData->pathLossMatrix[channelIndex]->
                            find(srcdstPair);

    if (it == partitionData->pathLossMatrix[channelIndex]->
                            end())
    {
       srcdstPair = make_pair(nodeId2, nodeId1);

        it = partitionData->pathLossMatrix[channelIndex]->
                            find(srcdstPair);

        if (it == partitionData->pathLossMatrix[channelIndex]->
                            end())
        {
            return HIGH_PATHLOSS;
        }
        else
        {
            return it->second;
        }
    }
    else
    {
        return it->second;
    }
}

