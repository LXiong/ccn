/*  Copyright (c) January 2005 Jean Gressmann (jsg@rz.uni-potsdam.de)
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
 
#include <portablethreads/unix/thread.h>
#include <cstdio>
#include <cassert>
#include <signal.h>
#include <errno.h>
#include <string.h>

namespace PortableThreads 
{
	void* PThread::entry(void* arg)
	{
		// Disable all signals for this thread. Signals are handled in the main thread.
		sigset_t allSignals, temp;
		sigfillset(&allSignals);
		if(pthread_sigmask(SIG_BLOCK, &allSignals, &temp) != 0)
			std::printf("[PThread] Could not disable signals. Reason: %s\n", strerror(errno));

		// set id in case this threads needs to use it an the thread that
		// created it did not yet return from pthread_create().
		OSThreadTraits::thread_t* thread = static_cast<OSThreadTraits::thread_t*>(arg);
		assert(thread);
		thread->thread_ = pthread_self();

		entry_common(thread->arg_);
		return 0;
	}
}
