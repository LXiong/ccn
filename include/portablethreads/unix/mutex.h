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
 
#ifndef UNIX_MUTEX_H
#define UNIX_MUTEX_H
#include <portablethreads/config.h>
#include <portablethreads/exception.h>
#include <pthread.h>
#include <stdlib.h>


namespace PortableThreads 
{
	namespace OSSpecific
	{
		class PTMutex
		{
		public:
			PTMutex(bool locked = false) 		
			{
				if(pthread_mutex_init(&mutex_, 0) != 0)
				{
					printf ("[PTMutex] Could not create mutex variable\n");
					// exit (1);
					// throw PTResourceError("[PTMutex] Could not create mutex variable");

				}
				if(locked)
					lock();
			}	
			~PTMutex()
			{
				pthread_mutex_destroy(&mutex_);
			}
			inline bool tryLock() 
			{ 
				return pthread_mutex_trylock(&mutex_) == 0;		
			}
			inline void lock() 
			{ 
				while(pthread_mutex_lock(&mutex_) != 0); 
			}
			inline void unlock() 
			{ 
				// ok, only error according to Single Unix Spec is EPERM
				pthread_mutex_unlock(&mutex_); 
			}
		private:
			PTMutex(const PTMutex&);
			PTMutex& operator=(const PTMutex&);
		private:
			pthread_mutex_t mutex_;
		};
	}
}

#endif
