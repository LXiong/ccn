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

#include <iostream>
using namespace std;
#include <cstdio>
#include <cstdlib>

#include "terrain_qualnet_urban_parser.h"

#include "terrain_esri_shp_shared.h"

void
EsriShpVerify(
    bool condition,
    const char* errorString,
    const char* path,
    unsigned lineNumber)
{
    if (!condition)
    {
        EsriShpReportError(errorString, path, lineNumber);
    }
}

void
EsriShpReportWarning(
    char* warningString,
    const char* path,
    unsigned lineNumber)
{
    cerr << "esri_shp warning:";

    if (path != NULL)
    {
        cerr << path << ":";

        if (lineNumber > 0)
        {
            cerr << lineNumber << ":";
        }
    }

    cerr << " " << warningString << endl;
}

void
EsriShpReportError(
    const char* errorString,
    const char* path,
    unsigned lineNumber)
{
    cerr << "esri_shp error:";

    if (path != NULL)
    {
        cerr << path << ":";

        if (lineNumber > 0)
        {
            cerr << lineNumber << ":";
        }
    }

    cerr << " " << errorString << endl;

    exit(EXIT_FAILURE);
}
