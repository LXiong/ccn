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
 
#ifndef UNIX_THREAD_H
#define UNIX_THREAD_H
#include <portablethreads/config.h>
#include <portablethreads/exception.h>
#include <pthread.h>
#include <sched.h>
#include <cassert>

namespace PortableThreads 
{
	namespace OSSpecific
	{
		struct ThreadTraits
		{
			struct thread_t
			{
				volatile pthread_t thread_;
				pthread_attr_t attr_;
				void* arg_;
			};
			typedef pthread_t os_id_t;
			typedef void* (entry_function_t)(void*);
			inline static void create(thread_t& t, entry_function_t* entry, void* arg)
			{
				pthread_attr_init(&t.attr_);
				pthread_attr_setdetachstate(&t.attr_, PTHREAD_CREATE_DETACHED);	

				/*
				From Single Unix Spec:
				There is no requirement on the implementation that the ID of
				the created thread be available before the newly created thread 
				starts executing. The calling thread can obtain the ID of the created
				thread through the return value of the pthread_create() function,
				and the newly created thread can obtain its ID by a call to pthread_self().
				*/

				// Hence we pass a the pthread_t structure so the
				// newly created thread can set it's id in case the (presumable)
				// main thread that created it doesn't return from this function
				// before the created thread needs to find out it's id!
				assert(t.arg_ == 0);
				t.arg_ = arg;
				if(pthread_create(const_cast<pthread_t*>(&t.thread_), &t.attr_, entry, &t) != 0)
				{
					pthread_attr_destroy(&t.attr_);
					throw PTResourceError("[PThread] Could not create thread");
				}
			}
			static inline void kill(thread_t& t)
			{
				pthread_cancel(t.thread_);
			}
			static inline void initialize(thread_t& t)
			{
				t.arg_ = 0;
			}
			static inline os_id_t self() { return pthread_self(); }
			static inline void yield() { sched_yield(); }
			static inline void free_resource(thread_t& t) 
			{
				pthread_attr_destroy(&t.attr_);
				t.arg_ = 0;
			}
			static inline bool equal(os_id_t lhs, os_id_t rhs)
			{
				return pthread_equal(lhs, rhs) != 0;
			}
			static os_id_t thread_id(const thread_t& t)
			{
				return t.thread_;
			}
			static inline void join(thread_t& t)
			{
				pthread_join(t.thread_, 0);
			}
			static inline void detach(thread_t& t)
			{
				pthread_detach(t.thread_); 
			}
		};
	}
}

#endif
