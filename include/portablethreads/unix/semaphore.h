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
 
#ifndef UNIX_SEMAPHORE_H
#define UNIX_SEMAPHORE_H
#include <portablethreads/config.h>
#include <portablethreads/exception.h>
#include <limits.h>
#include <semaphore.h> 


namespace PortableThreads 
{
	namespace OSSpecific
	{
		class PTSemaphore
		{
		public:
			static const unsigned MAX = SEM_VALUE_MAX;
			PTSemaphore(unsigned initially = 0)
			{
				if(sem_init(&sem_, 0, initially > MAX ? MAX : initially) == -1)
					throw PTResourceError("[PTSemaphore] Could not create semaphore");
			}
		  	
			// WARNING: make sure there is no thread waiting on the semaphore
			// because doing so will result in undefined behavior when the
			// semaphore is destroyed
			~PTSemaphore()
			{
				// no while loop, no error defined due to interruption
				sem_destroy(&sem_);
			}
			
			inline bool tryDown()
  			{
				// I don't really care why it failed...
				return sem_trywait(&sem_) != -1;
			}
		    
			inline void down()
  			{ 
    			// I don't really care why it failed...
				while(sem_wait(&sem_) == -1);
			}
		    
			inline void up()
  			{
  				// ok, only error as of POSIX is "invalid sem"
    			sem_post(&sem_);
  			}		
		private:
			PTSemaphore(const PTSemaphore&);
  			PTSemaphore& operator=(const PTSemaphore&);
		private:
			sem_t sem_;
		};
	}
}

#endif
