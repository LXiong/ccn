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


#ifndef TRAFFICPATTERN_PARSER_H
#define TRAFFICPATTERN_PARSER_H

struct PatternAppData {
    char appName[MAX_STRING_LENGTH];
    short numAppParams;
};

struct CellularAppData : public PatternAppData {
    RandomDistribution<Int32> destinationNodeDistribution;
    RandomDistribution<clocktype> durationDistribution;
    RandomDistribution<Int32> appTypeDistribution;
    RandomDistribution<Float64> bandwidthDistribution;

    void init() {
        destinationNodeDistribution.init();
        durationDistribution.init();
        appTypeDistribution.init();
        bandwidthDistribution.init();
    }
};   

struct CbrAppData : public PatternAppData {
    RandomDistribution<Int32> destinationNodeDistribution;
    RandomDistribution<Int32> numItemsDistribution;
    RandomDistribution<Int32> itemSizeDistribution;
    RandomDistribution<clocktype> intervalDistribution;
    RandomDistribution<clocktype> durationDistribution;
};   

struct TrafficPatternBeh {
    float probability;
    float retryProbability;
    RandomDistribution<clocktype> *distRetryInt;
    short maxNumRetry;
    PatternAppData *appData;
    TrafficPatternBeh* next;
};

struct TrafficPatternData {
    char trafficPatternName[MAX_STRING_LENGTH];
    short numAppTypes;
    RandomDistribution<Int32> *distMaxNumApp;
    RandomDistribution<clocktype> *distArrivalInt;
    TrafficPatternBeh *trafficBeh;
};

struct TrafficPatternDataMappingRecord {
    TrafficPatternData data;
    TrafficPatternDataMappingRecord* next;
};

struct TrafficPatternDataMapping {
    int hashSize;
    TrafficPatternDataMappingRecord **table;
};



TrafficPatternDataMapping* USER_InitializeTrafficPattern (
                            TrafficPatternDataMapping *dataMapping,
                            const NodeInput *nodeInput);

TrafficPatternDataMappingRecord* TrafficPatternDataMappingLookup (
                                    Node *node,
                                    char *name);

#endif
