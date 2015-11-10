/*  Copyright (c) October 2005 Jean Gressmann (jsg@rz.uni-potsdam.de)
 *
 *  This is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version. 
 * 
 *	This file is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this file; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <sys/time.h>
#include <unistd.h>
#include <sched.h>
#include <cassert>

namespace PortableThreads 
{
	void pt_second_sleep(unsigned time)
	{
		/*
		From Single Unix Spec:
		If sleep() returns because the requested time has elapsed, the value 
		returned shall be 0. If sleep() returns due to delivery of a signal, 
		the return value shall be the "unslept" amount (the requested time minus
		the time actually slept) in seconds.
		*/
		for(unsigned t = time; t > 0; t = sleep(t));
	}
	void pt_milli_sleep(unsigned time)
	{
		// only defined error is parameter 
		// greater than 1000000
		pt_second_sleep(time / 1000);
		usleep((time - time / 1000)*1000);
	}
	void pt_micro_sleep(unsigned time)
	{
		pt_milli_sleep(time / 1000);
		usleep(time); 
	}
	void pt_yield()
	{
		sched_yield();
	}

	///////////////////////////////////////////////////////////////////////
	// Time
	///////////////////////////////////////////////////////////////////////

	PTime::time_type PTime::stamp()
	{
		timeval tv;

		// POSIX doesn't define errors for gettimeofday 
		// but Solaris does, alas...
	#ifndef NDEBUG
		int ret = gettimeofday(&tv, 0);
		assert(ret == 0);
	#else
		gettimeofday(&tv, 0);
	#endif

		uint64 t = tv.tv_usec;
		t += tv.tv_sec*1000000;
		return t;
	}

	PTime::time_type PTime::calculateFrequency()
	{
		return 1000000; // see gettimeofday man-page
	}
	const PTime::time_type PTime::frequency_ = PTime::calculateFrequency();

}
