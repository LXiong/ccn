/*  Copyright (c) September 2005 Jean Gressmann (jsg@rz.uni-potsdam.de)
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

#ifndef SPARC_64_V9_GCC_H
#define SPARC_64_V9_GCC_H

#ifndef __GNUC__
#	error "You must use a GNU C++ compatible compiler in order to use this header file!"
#endif

#define PT_HAVE_CAS

#include <cassert>
#include <portablethreads/arch/64-gcc.h>

namespace PortableThreads 
{
	namespace LockFree
	{
		namespace Private
		{
			// Assembler functions are straight from the linux kernel
			inline bool pt_atomic_set_lock(volatile uint8* lock)
			{
				uint32 result;
				__asm__ __volatile__("ldstub [%1], %0 \n\t"
					: "=r" (result)
					: "r" (lock)
					: "memory");
				return result == 0;
			}

			inline void pt_atomic_clear_lock(volatile uint8* lock)
			{
				__asm__ __volatile__("stb %%g0, [%0] \n\t" : : "r" (lock) : "memory");
			}

			/*
			* Atomic compare and exchange.  Compare OLD with MEM, if identical,
			* store NEW in MEM.  Return the initial value in MEM.  Success is
			* indicated by comparing RETURN with OLD.
			*/
			inline int32 pt_atomic_cas_return_memory(volatile int32* mem, int32 nv, int32 ov)
			{
				__asm__ __volatile__("cas [%2], %3, %0 \n\t"
					"membar #StoreLoad | #StoreStore \n\t"
					: "=&r" (nv)
					: "0" (nv), "r" (mem), "r" (ov)
					: "memory");

				return nv;
			}
			inline int64 pt_atomic_cas_return_memory(volatile int64* mem, int64 nv, int64 ov)
			{
				__asm__ __volatile__("casx [%2], %3, %0 \n\t"
					"membar #StoreLoad | #StoreStore \n\t"
					: "=&r" (nv)
					: "0" (nv), "r" (mem), "r" (ov)
					: "memory");

				return nv;
			}

			inline bool pt_atomic_cas(volatile int32* mem, int32 nv, int32 ov)
			{
				return pt_atomic_cas_return_memory(mem, nv, ov) == ov;
			}

			inline bool pt_atomic_cas(volatile int64* mem, int64 nv, int64 ov)
			{
				return pt_atomic_cas_return_memory(mem, nv, ov) == ov;
			}

			inline int64 pt_atomic_set(volatile int64* mem, int64 nv)
			{
				int64 current;
				do { current = *mem; } while(!pt_atomic_cas(mem, nv, current));
				return current;
			}

			inline void pt_barrier()
			{
				__asm__ __volatile__("membar #LoadLoad | #LoadStore | #StoreStore | #StoreLoad \n\t" : : : "memory");
			}

			// atomically adds value to counter and returns the result
			inline int64 pt_atomic_add(volatile int64* counter, int64 value)
			{
				int64 current;
				do { current = *counter; } while(!pt_atomic_cas(counter, current+value, current));
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

			inline uint64 pt_ticks()
			{
				uint64 ret;
				__asm__ ("rd %%tick, %0 \n\t" : "=r"(ret) );
				return ret;
			}


			const unsigned HARDWARE_POINTER_BITS = 44;
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
			/*
			UltraSPARC-II Manual
			UltraSPARC-IIi implements a 44-bit virtual address space in two equal halves at the
			extreme lower and upper portions of the full 64-bit virtual address space. Virtual
			addresses between 0000 0800 0000 0000(16) and FFFF F7FF FFFF FFFF(16), inclusive, are
			termed “out of range” for UltraSPARC-IIi and are illegal. (In other words, virtual
			address bits VA<63:43> must be either all zeros or all ones.) FIGURE 4-2 on page 25
			illustrates the UltraSPARC-IIi virtual address space.
			*/
			typedef PointerCAS< FreeHighBitsMuxer<int64, 64, 44, 3, 0> > PTPointerCAS;

			template<unsigned USER_BITS>
			struct PTPointerCASType
			{
				typedef PointerCAS< FreeHighBitsMuxer<int64, 64, 44, 3, USER_BITS> > PTPointerCAS;
			};
		}
	}
}


#endif
