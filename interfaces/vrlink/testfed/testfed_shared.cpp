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

// iostream.h is needed to support DMSO RTI 1.3v6 (non-NG).

#ifdef NOT_RTI_NG
#include <iostream.h>
#else /* NOT_RTI_NG */
#include <iostream>
using namespace std;
#endif /* NOT_RTI_NG */

#include "testfed_shared.h"

// /**
//FUNCTION :: Verify
//PURPOSE :: Do a conditional check, if fails ouput an error and exit
//PARAMETERS ::
// + condition : bool : conditional result
// + warningString : const void* : warning explanation
// + path : const char* : path to file where error occurred
// + lineNumber : unsigned : line number error occurred
//RETURN :: void : NULL
// **/
void Verify(
    bool condition,
    char* errorString,
    const char* path,
    unsigned lineNumber)
{
    if (!condition)
    {
        ReportError(errorString, path, lineNumber);
    }
}

// /**
//FUNCTION :: CheckMalloc
//PURPOSE :: check if pointer value is a non NULL memory address
//PARAMETERS ::
// + warningString : const void* : warning explanation
// + path : const char* : path to file where error occurred
// + lineNumber : unsigned : line number error occurred
//RETURN :: void : NULL
// **/
void CheckMalloc(
    const void* ptr,
    const char* path,
    unsigned lineNumber)
{
    if (ptr == NULL)
    {
        ReportError("Out of memory", path, lineNumber);
    }
}

// /**
//FUNCTION :: CheckNoMalloc
//PURPOSE :: check pointer if pointer is to NULL
//PARAMETERS ::
// + warningString : const void* : warning explanation
// + path : const char* : path to file where error occurred
// + lineNumber : unsigned : line number error occurred
//RETURN :: void : NULL
// **/
void CheckNoMalloc(
    const void* ptr,
    const char* path,
    unsigned lineNumber)
{
    if (ptr != NULL)
    {
        ReportError(
            "Attempting to allocate memory for non-null pointer",
            path,
            lineNumber);
    }
}

// /**
//FUNCTION :: ReportWarning
//PURPOSE :: Output an warning
//PARAMETERS ::
// + warningString : char* : warning explanation
// + path : const char* : path to file where error occurred
// + lineNumber : unsigned : line number error occurred
//RETURN :: void : NULL
// **/
void ReportWarning(
    char* warningString,
    const char* path,
    unsigned lineNumber)
{
    cerr << "Testfed warning:";

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

// /**
//FUNCTION :: ReportError
//PURPOSE :: Output an error and exit execution
//PARAMETERS ::
// + errorString : char* : error explanation
// + path : const char* : path to file where error occurred
// + lineNumber : unsigned : line number error occurred
//RETURN :: void : NULL
// **/
void ReportError(
    char* errorString,
    const char* path,
    unsigned lineNumber)
{
    cerr << "Testfed error:";

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

/*
 * FUNCTION      TIME_ConvertToClock
 * PURPOSE       Read the string in "buf" and provide the corresponding
 *               clocktype value for the string. Use the following conversions:
 *               NS - nano-seconds
 *               MS - milli-seconds
 *               S  - seconds (default if no specification)
 *               H  - hours
 *               D  - days
 */
clocktype TIME_ConvertToClock(const char *buf)
{
    char errorStr[MAX_STRING_LENGTH];
    const char *temp;
    double clockDouble;
    bool error = false;

    if ((temp = strstr(buf, "NS")) != NULL)
    {
        // Check that the units is NS and not NSX
        if (temp[2] != 0)
        {
            error = true;
        }

        clockDouble = atof(buf) + 0.5;
    }
    else
    if ((temp = strstr(buf, "US")) != NULL)
    {
        if (temp[2] != 0)
        {
            error = true;
        }

        clockDouble = atof(buf) * MICRO_SECOND + 0.5;
    }
    else
    if ((temp = strstr(buf, "MS")) != NULL)
    {
        if (temp[2] != 0)
        {
            error = true;
        }

        clockDouble = atof(buf) * MILLI_SECOND + 0.5;
    }
    else
    if ((temp = strstr(buf, "S")) != NULL)
    {
        if (temp[1] != 0)
        {
            error = true;
        }

        clockDouble = atof(buf) * SECOND + 0.5;
    }
    else
    if ((temp = strstr(buf, "M")) != NULL)
    {
        if (temp[1] != 0)
        {
            error = true;
        }

        clockDouble = atof(buf) * MINUTE + 0.5;
    }
    else
    if ((temp = strstr(buf, "H")) != NULL)
    {
        if (temp[1] != 0)
        {
            error = true;
        }

        clockDouble = atof(buf) * HOUR + 0.5;
    }
    else
    if ((temp = strstr(buf, "D")) != NULL)
    {
        if (temp[1] != 0)
        {
            error = true;
        }

        clockDouble = atof(buf) * DAY + 0.5;
    }
    else
    {
        clockDouble = atof(buf) * SECOND + 0.5;
    }

    // Check for incorrect time format
    if (error)
    {
        sprintf(
            errorStr,
            "An invalid time value of \"%s\" was supplied.  Please check your configuration.",
            buf);
        ReportError(errorStr);
    }

    // Check for overflow
    if (clockDouble > (double) CLOCKTYPE_MAX)
    {
        sprintf(
            errorStr,
            "An invalid time value of \"%s\" was supplied.  The maximum allowable time is 106,751D (days).",
            buf);
        ReportError(errorStr);
    }

    return (clocktype) clockDouble;
}

/*
 * FUNCTION      TIME_PrintClockInSecond
 * PURPOSE       Print the value of clocktype as seconds to a char array
 * PARAMS        clocktype: clock value
 *               char[] : output char array
 */
void TIME_PrintClockInSecond(clocktype clock, char stringInSecond[])
{
    char TimeString[MAX_STRING_LENGTH];
    char TimeStringSecond[MAX_STRING_LENGTH];
    char TimeStringNanoSecond[MAX_STRING_LENGTH];
    int LengthOfTimeString;

    ctoa (clock, TimeString);

    LengthOfTimeString = (int)strlen(TimeString);
    if (LengthOfTimeString <= 9)
    {
        strcpy(TimeStringSecond, "0");
        strncpy(TimeStringNanoSecond, "000000000", 9 - LengthOfTimeString);
        strncpy(
            &TimeStringNanoSecond[9 - LengthOfTimeString],
            TimeString,
            LengthOfTimeString);
        TimeStringNanoSecond[9] = 0;
    }
    else
    {
        strncpy(
            TimeStringSecond,
            TimeString,
            LengthOfTimeString - 9);
        TimeStringSecond[LengthOfTimeString - 9] = 0;
        strncpy(
            TimeStringNanoSecond,
            &TimeString[LengthOfTimeString - 9],
            9);
        TimeStringNanoSecond[9] = 0;
    }

    sprintf(stringInSecond, "%s.%s", TimeStringSecond, TimeStringNanoSecond);
}
