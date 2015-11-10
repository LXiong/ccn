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

#ifndef TESTFED_SHARED_H
#define TESTFED_SHARED_H
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>

#include "testfed_types.h"

#define MAX_STRING_LENGTH 512

void Verify(
    bool condition,
    char* errorString,
    const char* path = NULL,
    unsigned lineNumber = 0);

void CheckMalloc(
    const void* ptr,
    const char* path,
    unsigned lineNumber);

void CheckNoMalloc(
    const void* ptr,
    const char* path,
    unsigned lineNumber);

void ReportWarning(
    char* warningString,
    const char* path = NULL,
    unsigned lineNumber = 0);

void ReportError(
    char* errorString,
    const char* path = NULL,
    unsigned lineNumber = 0);


// /**
// MACRO       :: ctoa
// DESCRIPTION :: like sprintf, prints a clocktype to a string
// **/
#define ctoa(clock, string) CHARTYPE_sprintf( \
                        string, \
                        CHARTYPE_Cast("%" TYPES_64BITFMT "d"), \
                        clock)


clocktype TIME_ConvertToClock(const char *buf);

void TIME_PrintClockInSecond(clocktype clock, char stringInSecond[]);

#endif /* TESTFED_SHARED_H */
