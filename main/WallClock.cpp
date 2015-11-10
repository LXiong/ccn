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

#include <time.h>
#include <clock.h>

#include "WallClock.h"
#include "qualnet_error.h"

// includes for high resolution timer
#ifdef _WIN32
#define LOCAL_WINDOWS_DEFINES
#ifndef LOCAL_WINDOWS_DEFINES
#include <windows.h>
#else   // LOCAL_WINDOWS_DEFINES
#ifndef  _WINDOWS_          // prevent redef errors if a file includes
                            // windows.h itself
#if defined(MIDL_PASS)
typedef struct _LARGE_INTEGER {
#else // MIDL_PASS
typedef union _LARGE_INTEGER {
    struct {
        Int32 LowPart;
        Int32 HighPart;
    };
    struct {
        Int32 LowPart;
        Int32 HighPart;
    } u;
#endif //MIDL_PASS
    Int64 QuadPart;
} LARGE_INTEGER;

extern "C"
_declspec(dllimport)
BOOL
__stdcall
QueryPerformanceCounter(
    LARGE_INTEGER *lpPerformanceCount
    );

// __declspec(dllimport)
// __stdcall
extern "C"
_declspec(dllimport)
BOOL
__stdcall
QueryPerformanceFrequency(
    LARGE_INTEGER *lpFrequency
    );
#endif // _WINDOWS_
#endif // LOCAL_WINDOWS_DEFINES
#else // unix
#include <sys/time.h>
#include <unistd.h>
#endif


WallClock::WallClock(void* partitionData)
{
    m_partitionData = partitionData;
    m_numPauses = 0;
    m_durationPaused = 0.0;
    m_allowPause = true;
    m_realTimeMultiple = 1.0; 
    m_initializedTime = getTrueRealTime();
}
// /**
// FUNCTION   :: WallClock::getTrueRealTimeAsDouble
// PURPOSE    :: This static method returns the real wall clock time. The
//               value returned is not modified even if the simulation
//               was paused.
// PARAMETERS ::
// + timeDiff :: clocktype : The Initialization time is set back by 
//                           timeDiff.
// + void : : None
// RETURN     :: double : The value for now in seconds.
// **/
void WallClock::SetRealTime(clocktype timeDiff)
{
    m_initializedTime = getTrueRealTime() - timeDiff;
}

// /**
// FUNCTION   :: WallClock::getTrueRealTimeAsDouble
// PURPOSE    :: This static method returns the real wall clock time. The
//               value returned is not modified even if the simulation
//               was paused.
// PARAMETERS :: 
// + void : : None
// RETURN     :: double : The value for now in seconds.
// **/
double WallClock::getTrueRealTimeAsDouble()
{
#ifdef _WIN32
    LARGE_INTEGER t;
    LARGE_INTEGER freq;
    BOOL success;
    double value;

    success = QueryPerformanceCounter(&t);
    assert (success);

    success = QueryPerformanceFrequency(&freq);
    assert (success);

    // Compute number of seconds
    return (double) (t.QuadPart / (double)freq.QuadPart);
#else // unix
    struct timeval tval;
    int err;

    // get time and convert to qualnet time
    err = gettimeofday(&tval, NULL);
    if (err == -1)
    {
        assert(0);
    }
    return (double) tval.tv_sec + ((double) tval.tv_usec / (double)1e6);
#endif
}

// /**
// FUNCTION   :: WallClock::getTrueRealTime
// PURPOSE    :: This static method returns the real wall clock time. The
//               value returned is not modified even if the simulation
//               was paused.
// PARAMETERS :: 
// + void : : None
// RETURN     :: clocktype : The value for now as a clocktype.
// **/
clocktype WallClock::getTrueRealTime()
{
#ifdef _WIN32
    LARGE_INTEGER t;
    LARGE_INTEGER freq;
    BOOL success;
    clocktype val;
    clocktype fraction;

    success = QueryPerformanceCounter(&t);
    assert (success);

    success = QueryPerformanceFrequency(&freq);
    assert (success);

    // Compute number of seconds
    fraction = t.QuadPart / freq.QuadPart;
    val = fraction * SECOND;

    // Compute fractions of seconds
    fraction = t.QuadPart - fraction * freq.QuadPart;
    val += fraction * SECOND / freq.QuadPart;

    return val;
#else // unix/linux
    struct timeval t;
    int err;

    // get time and convert to qualnet time
    err = gettimeofday(&t, NULL);
    if (err == -1)
    {
        assert(0);
    }

    return t.tv_sec * SECOND + t.tv_usec * MICRO_SECOND;
#endif
}

// /**
// FUNCTION   :: WallClock::getRealTimeAsDouble
// PURPOSE    :: This method returns the real wall clock time. The
//               value returned is adjusted so that the time spent in
//               the paused state is subtracted.
// PARAMETERS :: 
// + void : : None
// RETURN     :: double : The value for now adjusted for pauses in seconds.
// **/
double WallClock::getRealTimeAsDouble()         // in seconds
{
    if (m_numPauses > 0)
    {
        return (m_pauseStartTime - convertToDouble(m_initializedTime) - m_durationPaused) * m_realTimeMultiple;
    }
    else
    {
        return (getTrueRealTimeAsDouble() - convertToDouble(m_initializedTime) - m_durationPaused) * m_realTimeMultiple;
    }
}

// /**
// FUNCTION   :: WallClock::getRealTime
// PURPOSE    :: This method returns the real wall clock time. The
//               value returned is adjusted so that the time spent in
//               the paused state is subtracted.
// PARAMETERS :: 
// + void : : None
// RETURN     :: clocktype : The value for now adjusted for pauses
//                           as a clocktype.
// **/
clocktype WallClock::getRealTime()
{
    if (m_numPauses > 0)
    {
        return (clocktype) ((convertToClocktype(m_pauseStartTime) - m_initializedTime - convertToClocktype(m_durationPaused)) * m_realTimeMultiple);
    }
    else
    {
        return (clocktype) ((getTrueRealTime() - m_initializedTime - convertToClocktype(m_durationPaused)) * m_realTimeMultiple);
    }
}

// /**
// FUNCTION   :: WallClock::pause
// PURPOSE    :: This method pauses the WallClock. The WallClock is
//               placed into the paused state (unless pausing has been
//               disabled by a call to disallowPause ).
// PARAMETERS :: 
// + void : : None
// RETURN     :: void :
// **/
void WallClock::pause()
{
    if (m_numPauses == 0)
    {
        m_pauseStartTime = getTrueRealTimeAsDouble();
    }
    m_numPauses++;
}

// /**
// FUNCTION   :: WallClock::resume
// PURPOSE    :: This method resumes the WallClock. The WallClock is
//               placed into the running state.
// PARAMETERS :: 
// + void : : None
// RETURN     :: void :
// **/
void WallClock::resume()
{
    if (m_numPauses == 0)
    {
        ERROR_ReportWarning("Call to WallClock::resume() without a "
                            "corresponding call to WallClock::pause()");
        return;
    }

    if (m_numPauses == 1)
    {
        m_durationPaused += (getTrueRealTimeAsDouble() - m_pauseStartTime);
    }
    m_numPauses--;
}

// /**
// FUNCTION   :: WallClock::setRealTimeMultiple
// PURPOSE    :: Sets the real time multiple which indicates how fast
//               QualNet sould run relative to real time
// PARAMETERS :: None
// RETURN     :: None
// **/
void WallClock::setRealTimeMultiple (double multiple)
{
    // Update real time multiple such that the current real time stays
    // the same
    clocktype now = getTrueRealTime();

    m_initializedTime = (clocktype) (-(now - m_initializedTime - convertToClocktype(m_durationPaused)) * m_realTimeMultiple / multiple + now - convertToClocktype(m_durationPaused));
    m_realTimeMultiple = multiple;
}





