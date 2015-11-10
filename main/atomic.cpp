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


#include "util_atomic.h"

#if defined(USE_ATOMIC_WIN)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// #define WIN32_USE_ASM 


void UTIL_AtomicSet(UTIL_AtomicInteger *v, int x)
{ 
#if defined(WIN32_USE_ASM)
    long *p = (long*)&v->value;
    __asm
    {
        mov EAX, x 
        mov EBX, p
        lock mov [EBX], EAX
    }
#else 
    InterlockedExchange((long*)&v->value, x); 
#endif
}

int UTIL_AtomicRead(UTIL_AtomicInteger *v)
{ 
    int t;
#if defined(WIN32_USE_ASM)
    long *p = (long*)&v->value;
    __asm
    {
        mov EAX, p
        lock mov EBX, [EAX]
        mov t, EBX
    }
    return t;
#else
    return v->value; 
#endif
} 

void UTIL_AtomicIncrement(UTIL_AtomicInteger *v)
{ 
#if defined(WIN32_USE_ASM)
    long *p = (long*)&v->value;
    __asm
    {
        mov EAX, p
        lock inc [EAX]
    }
#else
    InterlockedIncrement((long*)&v->value); 
#endif
}


void UTIL_AtomicDecrement(UTIL_AtomicInteger *v)
{ 
#if defined(WIN32_USE_ASM)
    long *p = (long*)&v->value;
    __asm
    {
        mov EAX, p
        lock dec [EAX]
    }
#else
    InterlockedDecrement((long*)&v->value); 
#endif
}

void UTIL_AtomicAdd(UTIL_AtomicInteger *v, int i)
{
    int dir;
#if defined(WIN32_USE_ASM)
    long *p = (long*)&v->value;
    __asm
    {
        mov EAX, i
        mov EBX, p
        lock add [EBX], EAX
    }
#else
    if (i == 0)
        return;
    
    dir = (i < 0) ? -1 : 1;
    i *= dir;
    
    for (int k = 0; k < i; k++)
        if (dir == 1)
            UTIL_AtomicIncrement(v);
        else
            UTIL_AtomicDecrement(v);
#endif
}

void UTIL_AtomicSubtract(UTIL_AtomicInteger *v, int i)
{ 
    UTIL_AtomicAdd(v, -i); 
}

int UTIL_AtomicDecrementAndTest(UTIL_AtomicInteger *v)
{
#if defined(WIN32_USE_ASM)
    long *p = (long*)&v->value;
    unsigned char r;
    __asm
    {
        mov EAX, p
        lock dec [EAX]
        setz r
    }
    return (r == 1) ? 1 : 0;
#else
    int tmp = InterlockedDecrement((long*)&v->value);
    return tmp == 0 ? 1 : 0;
#endif
}

#endif /* USE_ATOMIC_WIN */
