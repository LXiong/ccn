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

#ifndef __PROP_DATA_H__
# define __PROP_DATA_H__

// This ifdef is so I don't have to put lots of ifdefs all over the code
// for passing this around as a parameter.
#ifdef ADDON_DB
#include "db.h"
struct SelectionData : public StatsDBUrbanPropData {
};
#else
struct SelectionData
{
    int txNodeId;
    int rxNodeId;
    Coordinates txPosition;
    Coordinates rxPosition;
    double frequency;
    double distance;
    bool freeSpace;
    bool LoS;
    bool txInCanyon;
    bool rxInCanyon;
    bool txIndoors;
    bool rxIndoors;
    std::string modelSelected;
    double pathloss;
    double freeSpacePathloss;
    double twoRayPathloss;
    double itmPathloss;
    int numWalls;
    int numFloors;
    double outsideDistance;
    double insideDistance1;
    double insideDistance2;
    double outsidePathloss;
    double insidePathloss1;
    double insidePathloss2;

    SelectionData() {
        frequency = 0.0;
        distance = 0.0;
        freeSpace = false;
        LoS = false;
        txInCanyon = false;
        rxInCanyon = false;
        txIndoors = false;
        rxIndoors = false;
        pathloss = 0.0;
        freeSpacePathloss = 0.0;
        twoRayPathloss = 0.0;
        itmPathloss = 0.0;
        numWalls = 0;
        numFloors = 0;
        outsideDistance = 0;
        insideDistance1 = 0;
        insideDistance2 = 0;
        outsidePathloss = 0;
        insidePathloss1 = 0;
        insidePathloss2 = 0;
    }
};
#endif

#endif
