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

#ifndef DEM_INTERFACE
#define DEM_INTERFACE

#include "terrain.h"

#define QUADRANGLE_NAME_LENGTH 144
#define MAX_NUM_DEM_FILES      100
#define DEM_NO_MATCH_FOUND     (MAX_NUM_DEM_FILES + 1)

struct DemTypeBRecordData {
    int rowIndex;
    int columnIndex;
    int numRows;
    int numColumns;
    Coordinates firstPoint;
    float referenceElevation;
    float minElevation;
    float maxElevation;
    short *elevationData;
};


struct  DemTypeCRecordData {
    int value[10];
};


struct DemTypeARecordData {
    char quadrangleName[QUADRANGLE_NAME_LENGTH + 1];
    int levelCode;
    int patternCode;
    int planimetricCode;
    int zoneCode;
    float projection[15];
    int gridUnit;
    int elevationUnit;
    int numSides;
    Coordinates southWestCorner;
    Coordinates northEastCorner;
    float minElevation;
    float maxElevation;
    float angle;
    int accuracy;
    float resolution[3];
    int numRows;
    int numColumns;
    DemTypeBRecordData *b;
    DemTypeCRecordData c;
};


class DemTerrainData : public ElevationTerrainData {
private:
    UInt32 m_numDemFiles;
    UInt32 m_mostRecentFile;
    DemTypeARecordData* m_records[MAX_NUM_DEM_FILES];

    // DEM specific functions

    // returns DEM_NO_MATCH_FOUND if there's no match
    UInt32 findMatchingFile(const Coordinates* c);

public:
    DemTerrainData(TerrainData* td) : ElevationTerrainData(td) {
        m_numDemFiles = 0;
        m_mostRecentFile = 0;
        m_modelName = "DEM";
    }
    virtual ~DemTerrainData();

    void initialize(NodeInput* nodeInput);

    bool hasData() { return true; }
    double getElevationAt(const Coordinates* c);

    virtual void getElevationBoundaries(int index, Coordinates* sw, Coordinates* ne);

    virtual void getHighestAndLowestElevation(const Coordinates* sw,
            const Coordinates* ne,
            double* highest,
            double* lowest);
    int getNumDemFiles() { return m_numDemFiles; }
};

#endif
