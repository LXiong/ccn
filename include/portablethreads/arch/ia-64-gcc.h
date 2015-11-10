/*  Copyright (c) January 2006 Jean Gressmann (jsg@rz.uni-potsdam.de)
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

#ifndef IA_64_GCC_H
#define IA_64_GCC_H

#ifndef __GNUC__
#	error "You must use a GNU C++ compatible compiler in order to use this header file!"
#endif

#define PT_HAVE_CAS

#include <cassert>
#include <sys/time.h>
#include <portablethreads/arch/64-gcc.h>
/*
#define ia64_cmpxchg4_acq(ptr, new, old)						\
({											\
	__u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg4.acq %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(new) : "memory");	\
	ia64_intri_res;									\
})

#define ia64_cmpxchg8_acq(ptr, new, old)						\
({											\
	__u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg8.acq %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(new) : "memory");	\
	ia64_intri_res;									\
})

#define ia64_cmpxchg1_acq(ptr, new, old)						\
({											\
	__u64 ia64_intri_res;								\
	asm volatile ("mov ar.ccv=%0;;" :: "rO"(old));					\
	asm volatile ("cmpxchg1.acq %0=[%1],%2,ar.ccv":					\
			      "=r"(ia64_intri_res) : "r"(ptr), "r"(new) : "memory");	\
	ia64_intri_res;									\
})
*/

namespace PortableThreads 
{
	// Most of these functions are due to the Linux kernel.
	namespace LockFree
	{
		namespace Private
		{
			inline void pt_atomic_clear_lock(volatile uint8* lock)
			{
				uint64 prev;	
				__asm__ __volatile__("mov ar.ccv=%0;;\t\n" :: "rO"(0xff));
				__asm__ __volatile__(
					"cmpxchg1.acq %0=[%1],%2,ar.ccv\t\n"
					: "=r"(prev) 
					: "r"(lock), "r"(0) 
					: "memory"
				);
			}
			
			inline bool pt_atomic_set_lock(volatile uint8* lock)
			{
				uint64 prev;	
				__asm__ __volatile__("mov ar.ccv=%0;;\t\n" :: "rO"(0));
				__asm__ __volatile__(
					"cmpxchg1.acq %0=[%1],%2,ar.ccv\t\n"
					: "=r"(prev) 
					: "r"(lock), "r"(0xff) 
					: "memory"
				);
				return static_cast<uint8>(prev) == 0;
			}
			
			inline int32 pt_atomic_cas_return_memory(volatile int32* mem, int32 nv, int32 ov)
			{
				int64 prev;	
				__asm__ __volatile__("mov ar.ccv=%0;;\t\n" :: "rO"(ov));
				__asm__ __volatile__(
					"cmpxchg4.acq %0=[%1],%2,ar.ccv"
					: "=r"(prev) 
					: "r"(mem), "r"(nv) 
					: "memory"
				);
				return static_cast<int32>(prev);
			}
			
			inline int64 pt_atomic_cas_return_memory(volatile int64* mem, int64 nv, int64 ov)
			{
				int64 prev;	
				__asm__ __volatile__("mov ar.ccv=%0;;\t\n" :: "rO"(ov));
				__asm__ __volatile__(
					"cmpxchg8.acq %0=[%1],%2,ar.ccv"
					: "=r"(prev) 
					: "r"(mem), "r"(nv) 
					: "memory"
				);
				return prev;
			}
			
			inline bool pt_atomic_cas(volatile int32* mem, int32 nv, int32 ov)
			{
				return pt_atomic_cas_return_memory(mem, nv, ov) == ov;
			}
			inline bool pt_atomic_cas(volatile int64* mem, int64 nv, int64 ov)
			{
				return pt_atomic_cas_return_memory(mem, nv, ov) == ov;
			}


			inline void pt_barrier()
			{
				__asm__ __volatile__("mf" ::: "memory");
			}

			inline int64 pt_atomic_add(volatile int64* counter, int64 value)
			{
				int64 current;
				do
				{
					current = *counter;
				}
				while(!pt_atomic_cas(counter, current + value, current));
				return current + value;
			}

			inline int64 pt_atomic_sub(volatile int64* counter, int64 value)
			{
				return pt_atomic_add(counter, -value);
			}

			inline int64 pt_atomic_inc(volatile int64* counter)
			{
				return pt_atomic_add(counter, 1);
			}

			inline int64 pt_atomic_dec(volatile int64* counter)
			{
				return pt_atomic_add(counter, -1);
			}

			

			inline int64 pt_atomic_set(volatile int64* mem, int64 nv)
			{
				int64 current;
				do
				{
					current = *mem;
				}
				while(!pt_atomic_cas(mem, nv, current));
				return current;
			}

			const unsigned HARDWARE_POINTER_BITS = 50; // Intel Itanium2 Programming Manual
			const unsigned ALIGNMENT_BITS = 3;
		}
	}
	inline uint64 pt_seed()
	{
		timeval tv;
		gettimeofday(&tv, 0);
		
		uint64 t = tv.tv_usec;
		t += tv.tv_sec*1000000;
		return t;
	}
}

#include <portablethreads/arch/arch-common.h>
#include <portablethreads/arch/native-pointer-cas.h>
#include <portablethreads/arch/native-atomic-number.h>
#include <portablethreads/arch/ia-64-muxer.h>


namespace PortableThreads
{
	namespace LockFree
	{
		namespace Private
		{
			typedef PointerCAS< IA64Muxer<int64, HARDWARE_POINTER_BITS, ALIGNMENT_BITS, 0> > PTPointerCAS;

			template<unsigned USER_BITS>
			struct PTPointerCASType
			{
				typedef PointerCAS< IA64Muxer<int64, HARDWARE_POINTER_BITS, ALIGNMENT_BITS, USER_BITS> > PTPointerCAS;
			};
		}
	}
}


#endif
