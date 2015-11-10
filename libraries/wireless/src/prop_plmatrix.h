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

#ifndef PROP_PL_MATRIX_H
#define PROP_PL_MATRIX_H

void PathlossMatrixInitialize(
    PartitionData* partitionData,
    PropChannel* propChannel,
    int totalNumChannels,
    const NodeInput *nodeInput);

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
                                 NodeInput *nodeInput);

double PathlossMatrix(
    Node* node,
    NodeAddress nodeId1,
    NodeAddress nodeId2,
    int channelIndex,
    clocktype currentTime);

#endif /*PROP_PLMATRIX_H*/
