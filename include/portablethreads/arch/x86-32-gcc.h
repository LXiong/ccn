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

#ifndef X86_32_GCC_H
#define X86_32_GCC_H

#ifndef __GNUC__
#	error "You must use a GNU C++ compatible compiler in order to use this header file!"
#endif

#define PT_HAVE_CAS
#define PT_HAVE_CAS2

#include <portablethreads/arch/32-gcc.h>

namespace PortableThreads 
{
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

			// Most of these functions are due to the Linux kernel.
			inline void pt_barrier()
			{
				__asm__ __volatile__("mfence\n\t": : :"memory");
			}

			inline int32 pt_atomic_add(volatile int32* counter, int32 value)
			{
				const int32 res = value;
				__asm__ __volatile__(
					"lock; xaddl %0, %1	\n\t"
					:"=r"(value)
					:"m"(*counter), "0"(value)
				);
				return res + value;
			}

			inline int32 pt_atomic_sub(volatile int32* counter, int32 value)
			{
				return pt_atomic_add(counter, -value);
			}

			inline int32 pt_atomic_inc(volatile int32* counter)
			{
				return pt_atomic_add(counter, 1);
			}

			inline int32 pt_atomic_dec(volatile int32* counter)
			{
				return pt_atomic_add(counter, -1);
			}

			/*
			From Intel Pentium Manual: cmpxchg
			Operation
			(* accumulator = AL, AX, or EAX, depending on whether *)
			(* a byte, word, or doubleword comparison is being performed*)
			IF accumulator = DEST
			THEN
				ZF = 1
				DEST = SRC
			ELSE
				ZF = 0
				accumulator = DEST
			FI;
			*/
			inline int32 pt_atomic_cas_return_memory(volatile int32* inMemory, int32 newValue, int32 oldValue)
			{
				int32 prev;	
				__asm__ __volatile__(
					"lock; cmpxchgl %1, %2	\n\t"
					:	"=a"(prev)
					:	"q"(newValue), "m"(*inMemory), "0"(oldValue)
					:	"memory"
				);
				return prev;
			}

			inline bool pt_atomic_cas(volatile int32* mem, int32 nv, int32 ov)
			{
				return pt_atomic_cas_return_memory(mem, nv, ov) == ov;
			}

			inline int32 pt_atomic_set(volatile int32* inMemory, int32 newValue)
			{
				// This always asserts a lock (Pentium Manual)
				__asm__ __volatile__(
					"xchgl %0, %1	\n\t"
					:	"=r"(newValue)
					:	"m"(*inMemory), "0"(newValue)
					:	"memory"
				);
				return newValue;
			}

			// 64 bit
			inline bool pt_atomic_cas(volatile int64* mem, int64 newValue, int64 oldValue)
			{
			/* (from Pentium Manual)
			Compares the 64-bit value in EDX:EAX with the operand (destination operand). If the values
			are equal, the 64-bit value in ECX:EBX is stored in the destination operand. Otherwise, the
			value in the destination operand is loaded into EDX:EAX. The destination operand is an 8-byte
			memory location. For the EDX:EAX and ECX:EBX register pairs, EDX and ECX contain the
			high-order 32 bits and EAX and EBX contain the low-order 32 bits of a 64-bit value.
			*/

				// NOTE: This code is only so complicated b/c in PIC mode we cannot
				// use the ebx register, not even in the clobber list. However, b/c
				// cmpxchg8b REQUIRES the ebx register we need to store its content
				// first, do the op and then restore ebx to its previous content.
				int32 buf[] = {	static_cast<int32>(oldValue),
								static_cast<int32>(oldValue >> 32),
								static_cast<int32>(newValue),
								static_cast<int32>(newValue >> 32),
								0,
								0};
				__asm__ __volatile__(
					"movl %%ebx, 16(%0)		\n\t"
					"movl 0(%0), %%eax		\n\t"
					"movl 4(%0), %%edx		\n\t"
					"movl 8(%0), %%ebx		\n\t"
					"movl 12(%0), %%ecx		\n\t"
					"lock; cmpxchg8b (%1)   \n\t"
					"sete 20(%0)			\n\t"
					"movl 16(%0), %%ebx		\n\t"
					:
					: "S"(buf), "D"(mem)
					: "eax", "edx", "ecx", "cc", "memory"
				);

				return buf[5] != 0; 
			}

			inline uint64 pt_ticks()
			{
				/*
				Loads the current value of the processor’s time-stamp counter into the EDX:EAX registers. The
				time-stamp counter is contained in a 64-bit MSR. The high-order 32 bits of the MSR are loaded
				into the EDX register, and the low-order 32 bits are loaded into the EAX register.
				*/
				uint32 low, high;
				__asm__ (
					"rdtsc			\n\t"
					:"=a"(low), "=d"(high)
					:
					/* empty clobber list */
				);
				
				uint64 tsc = high;
				tsc <<= 32;
				tsc |= low;
				return tsc; 
			}

			const unsigned HARDWARE_POINTER_BITS = 32;
			const unsigned ALIGNMENT_BITS = 2;
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
			template<unsigned USER_BITS>
			struct PTPointerCASType
			{
				typedef PointerCAS< FreeHighBitsMuxer<int64, 64, 32, 2, USER_BITS> > PTPointerCAS;
			};

			typedef PTPointerCASType<0>::PTPointerCAS PTPointerCAS;
		}
	}
}

#endif
