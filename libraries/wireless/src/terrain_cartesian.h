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

#ifndef CARTESIAN_TERRAIN
#define CARTESIAN_TERRAIN

#include "terrain.h"

class CartesianTerrainData: public ElevationTerrainData {
private:
    int m_granularity;
    int m_numRows;
    int m_numColumns;
    double** m_elevationData;

public:
    CartesianTerrainData(TerrainData* td) : ElevationTerrainData(td) {
        m_elevationData = NULL;
        m_modelName = "CARTESIAN";
    }
    virtual ~CartesianTerrainData();

    void initialize(NodeInput* nodeInput);

    bool hasData() { return true; }
    double getElevationAt(const Coordinates* c);
    virtual void getHighestAndLowestElevation(const Coordinates* sw,
            const Coordinates* ne,
            double* highest,
            double* lowest);
};

#endif
