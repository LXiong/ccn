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

#ifndef X86_64_GCC_H
#define X86_64_GCC_H

#ifndef __GNUC__
#	error "You must use a GNU C++ compatible compiler in order to use this header file!"
#endif

#define PT_HAVE_CAS

#include <cassert>
#include <portablethreads/arch/64-gcc.h>


namespace PortableThreads 
{
	// Most of these functions are due to the Linux kernel.
	namespace LockFree
	{
		namespace Private
		{
			inline void pt_atomic_clear_lock(volatile uint8* lock)
			{
				__asm__ __volatile__(
					"lock; cmpxchgb %b0, %1 \n\t"
					:
					: "q"(0), "m"(*lock), "a"(0xff)
					: "memory"
                );
			}

			inline bool pt_atomic_set_lock(volatile uint8* lock)
			{
				uint8 prev;
				__asm__ __volatile__(
					"lock; cmpxchgb %b1, %2 \n\t"
					: "=a"(prev)
					: "q"(0xff), "m"(*lock), "0"(0)
					: "memory"
				);
 
				return prev == static_cast<uint8>(0);
			}

			inline void pt_barrier()
			{
				__asm__ __volatile__("mfence": : :"memory");
			}

			inline int64 pt_atomic_add(volatile int64* counter, int64 value)
			{
				const int64 res = value;
				__asm__ __volatile__(
					"lock; xaddq %0, %1"
					:"=r"(value)
					:"m"(*counter), "0"(value)
				);
				return res + value;
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

			/*
			* Atomic compare and exchange.  Compare OLD with MEM, if identical,
			* store NEW in MEM.  Return the initial value in MEM.  Success is
			* indicated by comparing RETURN with OLD.
			*/
			inline int64 pt_atomic_cas_return_memory(volatile int64* inMemory, int64 newValue, int64 oldValue)
			{
				int64 prev;	
				__asm__ __volatile__(
					"lock; cmpxchgq %1, %2"
					: "=a"(prev)
					: "q"(newValue), "m"(*inMemory), "0"(oldValue)
					: "memory"
				);
				return prev;
			}

			inline bool pt_atomic_cas(volatile int64* mem, int64 nv, int64 ov)
			{
				return pt_atomic_cas_return_memory(mem, nv, ov) == ov;
			}

			inline int64 pt_atomic_set(volatile int64* inMemory, int64 newValue)
			{
				__asm__ __volatile__(
					"xchgq %0, %1"
					:"=r" (newValue)
					:"m" (*inMemory), "0" (newValue)
					:"memory"
				);
				return newValue;
			}

			inline uint64 pt_ticks()
			{
				uint32 low, high;
				__asm__ (
					"rdtsc	\n\t"
					"movl %%eax, %0\n\t"
					"movl %%edx, %1\n\t"
					:"=r"(low), "=r"(high)
					:
					:"eax", "edx"
				);
				
				return (static_cast<uint64>(high)<<32) | static_cast<uint64>(low);
			}

			const unsigned HARDWARE_POINTER_BITS = 48;
			const unsigned ALIGNMENT_BITS = 3;
		}
	}
	inline uint64 pt_seed()
	{
		return LockFree::Private::pt_ticks();
	}


}

#include <portablethreads/arch/arch-common.h>
#include <portablethreads/arch/native-pointer-cas.h>
#include <portablethreads/arch/native-atomic-number.h>
#include <portablethreads/arch/free-high-bits-muxer.h>

namespace PortableThreads
{
	namespace LockFree
	{
		namespace Private
		{
			// AMD64 & Intel's EM64T:
			// Both currently use 48 bit for addressing. Hence we may
			// safely use most significant 16 bits as well as the least significant 3 bits
			// for reference counting. It is assumed that pointers only point to data alligned
			// on at least 8 byte boundaries.
			// 
			// NOTE: Intel's EM64T cpus support cmpxchg16b, AMD's cpus not (yet).
			// AMD64 and Intel EM64T define an address to be in cannonical form 
			// if it is either [0...0|pointer] or [1...1|pointer]. Where pointer is in 
			// both cases 48-bit wide.

			typedef PointerCAS< FreeHighBitsMuxer<int64, 64, 48, 3, 0> > PTPointerCAS;

			template<unsigned USER_BITS>
			struct PTPointerCASType
			{
				typedef PointerCAS< FreeHighBitsMuxer<int64, 64, 48, 3, USER_BITS> > PTPointerCAS;
			};
		}
	}
}


#endif
