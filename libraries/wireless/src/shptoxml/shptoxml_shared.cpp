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
#include <cstring>

#include "shptoxml_shared.h"

void
ShptoxmlVerify(
    bool condition,
    const char* errorString,
    const char* path,
    unsigned lineNumber)
{
    if (!condition)
    {
        ShptoxmlReportError(errorString, path, lineNumber);
    }
}

void
ShptoxmlReportWarning(
    char* warningString,
    const char* path,
    unsigned lineNumber)
{
    cerr << "Shptoxml warning:";

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
ShptoxmlReportError(
    const char* errorString,
    const char* path,
    unsigned lineNumber)
{
    cerr << "Shptoxml error:";

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

char*
ShptoxmlStripFileExtension(char* s)
{
    char* p = strrchr(s, '.');
    if (p == NULL) { return s; }

    *p = 0;
    return s;
}

char*
ShptoxmlSetFileSuffix(char* path, const char* extension)
{
    ShptoxmlStripFileExtension(path);
    strcat(path, extension);

    return path;
}
