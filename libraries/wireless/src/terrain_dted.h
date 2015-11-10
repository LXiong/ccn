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

#ifndef DTED_INTERFACE
#define DTED_INTERFACE

#include "terrain.h"

#define MAX_NUM_DTED_FILES      100
#define DTED_NO_MATCH_FOUND     (MAX_NUM_DTED_FILES + 1)

struct DtedRecordData {
    Coordinates southWestCorner;
    Coordinates northEastCorner;
    double resolution[3];
    int numRows;
    int numColumns;
    short**elevationData;
};


class DtedTerrainData : public ElevationTerrainData {
private:
    UInt32 m_numFiles;
    UInt32 m_mostRecentFile;
    DtedRecordData* m_records[MAX_NUM_DTED_FILES];

    // returns DTED_NO_MATCH_FOUND if there's no match
    UInt32 findMatchingFile(const Coordinates* c);
    void   getHighestAndLowestForFile(DtedRecordData* record,
                                      const Coordinates* sw,
                                      const Coordinates* ne,
                                      double* highest,
                                      double* lowest);

public:
    DtedTerrainData(TerrainData* td) : ElevationTerrainData(td) {
        m_numFiles = 0;
        m_mostRecentFile = 0;
        m_modelName = "DTED";
    }
    virtual ~DtedTerrainData();

    bool hasData() { return true; }
    void initialize(NodeInput* nodeInput);
    double getElevationAt(const Coordinates* c);

    virtual void getElevationBoundaries(int index, Coordinates* sw, Coordinates* ne);

    virtual void getHighestAndLowestElevation(const Coordinates* sw,
            const Coordinates* ne,
            double* highest,
            double* lowest);
};

#endif

