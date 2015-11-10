// Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
//                          600 Corporate Pointe
//                          Suite 1200
//                          Culver City, CA 90230
//                          info@scalable-networks.com
//
// This source code is licensed, not sold, and is subject to a written
// license agreement.  Among other things, no portion of this source
// code may be copied, transmitted, disclosed, displayed, distributed,
// translated, used as the basis for a derivative work, or used, in
// whole or in part, for any program or purpose other than its intended
// use in compliance with the license agreement as part of the QualNet
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

#if !defined(__X86_64_ATOMIC_H__)
#define __X86_64_ATOMIC_H__

#define UTIL_AtomicValue(x) { (x) }

typedef struct { volatile int value; } UTIL_AtomicInteger;
typedef struct { volatile float value; } UTIL_AtomicReal;

#if defined(NEEDS_SMP)
# define LOCK_PREFIX "lock\n\t"
#else
# define LOCK_PREFIX ""
#endif

static void UTIL_AtomicSet(UTIL_AtomicInteger *v, int x)
{ v->value = x; }
static int UTIL_AtomicRead(UTIL_AtomicInteger *v)
{ return v->value; }
static void UTIL_AtomicAdd(UTIL_AtomicInteger *v, int i)
{
    __asm__ __volatile__(
        LOCK_PREFIX
        "addl %1, %0"
        :
        : "m" (v->value) , "ir" (i)
        : "cc", "memory"
    );
}
static void UTIL_AtomicSubtract(UTIL_AtomicInteger *v, int i)
{
    __asm__ __volatile__(
        LOCK_PREFIX
        "subl %1, %0"
        : 
        : "m" (v->value), "ir" (i)
        : "cc", "memory"
    );
}

static void UTIL_AtomicIncrement(UTIL_AtomicInteger *v)
{
    __asm__ __volatile__(
        LOCK_PREFIX
        "incl %0"
        : 
        : "m" (v->value)
        : "cc", "memory"
    );
}

static void UTIL_AtomicDecrement(UTIL_AtomicInteger *v)
{
    __asm__ __volatile__(
        LOCK_PREFIX
        "decl %0"
        : 
        : "m" (v->value)
        : "cc", "memory"
    );
}

static int UTIL_AtomicDecrementAndTest(UTIL_AtomicInteger *v)
{
    unsigned char isZero;

    __asm__ __volatile__(
        LOCK_PREFIX
        "decl %1\t\n"
        "setz %0"
        : "=qm" (isZero)
        : "m" (v->value)
        : "cc", "memory"
    );

    return isZero == 1 ? 1 : 0;
}

#endif // __X86_64_ATOMIC_H__
