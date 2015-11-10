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
// use in compliance with the license agreement as part of the EXata
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

#include "ops_util.h"
#if defined(_WIN32) || defined (_WIN64)
#include <windows.h>
#else
#include <unistd.h>
#endif

#if !defined(_WIN32) && !defined (_WIN64)
// /**
// FUNCTION :: OPS_GetRealtime
// PURPOSE  :: For Linux systems, returns the time of day in microseconds.
// RETURN   :: wall clock in microseconds
// **/
realtime_t OPS_GetRealtime()
{
    static struct timeval tv;
    gettimeofday(&tv, NULL);
    return (realtime_t)tv.tv_sec * 1000000L + tv.tv_usec;
}
#endif

// /**
// FUNCTION   :: OPS_Sleep
// PURPOSE    :: Provide a common sleep call that is the same in both
//               windows and linux. In timesharing systems, the actual
//               time is approximate.
// PARAMETERS :: 
// + milliseconds : unsigned int : milliseconds to sleep
// **/
void OPS_Sleep(unsigned int milliseconds)
{
#if defined(_WIN32) || defined(_WIN64)
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}


// /**
// FUNCTION   :: OPS_SplitString
// PURPOSE    :: Split a string into a set of strings on a delimiter character.
// PARAMETERS ::
// + str       : string     : string to be split
// + out      :: vector     : output array of one or more strings
// + delim     : string     : character to split the string
// **/
void OPS_SplitString(const std::string& str, std::vector<std::string>& out, const std::string& delim)
{
    unsigned int pos = 0;
    int next;

    out.clear();

    // Get each token up to the last delimiter
    next = (int)str.find(delim, pos);
    while (next != -1)
    {
        if (next != (int)pos)
        {
            out.push_back(str.substr(pos, next - pos));
        }

        pos = next + 1;
        next = (int)str.find(delim, pos);
    }

    // Get the token after the final delimiter
    if (pos < str.size())
    {
        out.push_back(str.substr(pos));
    }
}

std::string OPS_GetSegment(const std::string& key, int index)
{
    std::string str;
    const char* ch = key.c_str();

    // Skip past first '/'
    if (*ch == '/')
    {
        ch++;
    }

    // Advance index
    while (index > 0)
    {
        if (*ch == '/')
        {
            // Reached next index
            index--;
        }
        else if (*ch == 0)
        {
            // Did not reach segment, return empty string
            return str;
        }
        ch++;
    }

    // Now we are at the first char of this index
    // Determine how many characters are in this segment
    const char* ch2 = ch + 1;
    while (*ch2 != 0 && *ch2 != '/')
    {
        ch2++;
    }

    str.insert(0, ch, ch2 - ch);

    return str;
}

//
// FUNCTION: OPS_ToNumber
// PURPOSE:  Converts a string into a double value. If the string is
//           non-numeric the value returned is 0.0.
//
double OPS_ToNumber(const std::string& numstr)
{
    double number;
    if ( !(std::istringstream(numstr) >> number) )
        number = 0.0;
    return number;
}

// /**
// FUNCTION: OPS_WildcardMatch
// PURPOSE:  Compares a key string to a second string that may contain 
//           the wildcard character, '*', within it between slashes.
//           It returns true if the string matches the wildcard string.
//
//           For example:
//           > wildcardMatch("/node/1/waypoint/speed", "/node/*/waypoint/speed")
//                 returns true.
//           > wildcardMatch("/node/1/waypoint/speed", "/node*/waypoint/speed")
//                 returns false
//           > wildcardMatch("/node/1/waypoint/speed", "/node/*/speed")
//                 returns false
// **/
bool OPS_WildcardMatch(const char *key, const char *wild)
{
    const char *keyChar = key;
    const char *wildChar = wild;

    while (*keyChar != 0 && *wildChar != 0)
    {
        if (*keyChar == *wildChar)
        {
            // Chars match.  Continue to next character.
            keyChar++;
            wildChar++;
        }
        else if (*wildChar == '*')
        {
            // Callback is a wildcard
            // Advance past the '*' character
            // Should end up on a '/' character
            wildChar++;

            // Advance i up to the next '/' character
            // key[i] should not begin on a '/' character
            while (*keyChar != 0 && *keyChar != '/')
            {
                keyChar++;
            }
        }
        else
        {
            // Chars do not match
            return false;
        }
    }

    // We got to the end of one string
    // Must be at the end of both strings to match (no remaining chars)
    return *keyChar == 0 && *wildChar == 0;
}


// FUNCTION: OPS_WilcardMatch
// PURPOSE:  Overload OPS_WildcardMatch(const char*, const char*) to
//           support compares of std::string objects.
//
bool OPS_WildcardMatch(const std::string& key, const std::string& wild)
{
    return OPS_WildcardMatch(key.c_str(), wild.c_str());
}
