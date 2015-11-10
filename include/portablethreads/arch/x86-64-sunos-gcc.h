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

#ifndef X86_64_SUNOS_GCC_H
#define X86_64_SUNOS_GCC_H

#include <limits.h>

namespace PortableThreads 
{

	// Solaris doesn't define SEM_VALUE_MAX directly, hence define it now
	const unsigned SEM_VALUE_MAX = _POSIX_SEM_VALUE_MAX;

}

#include <portablethreads/arch/x86-64-gcc.h>

#endif