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


#ifndef USERPROFILE_PARSER_H
#define USERPROFILE_PARSER_H

#include "random.h"

struct TrafficBehData {
    char name[MAX_STRING_LENGTH];
    char statusName[MAX_STRING_LENGTH];
    TrafficBehData* next;
};

struct UserProfileData {
    char userProfileName[MAX_STRING_LENGTH];
    float disSatifactionDegree;
    RandomDistribution<Int32> *distRecAge;
    RandomDistribution<Int32> *distRecSex;
    TrafficBehData *trafficBehData;
};

struct UserProfileDataMappingRecord {
    UserProfileData data;
    UserProfileDataMappingRecord* next;
};

struct UserProfileDataMapping {
    int hashSize;
    UserProfileDataMappingRecord **table;
};

UserProfileDataMapping* USER_InitializeUserProfile (
                        UserProfileDataMapping *dataMapping,
                        const NodeInput *nodeInput);

UserProfileDataMappingRecord* UserProfileDataMappingLookup (
                                Node *node,
                                char *name);

#endif
