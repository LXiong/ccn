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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <math.h>

#include "api.h"
#include "memory.h"
#include "parallel.h"

#define MEMSET_FULL_POOL (0)

//#define MEMORY_SYSTEM
//#define MEM_DEBUG

#ifdef MEMORY_SYSTEM

#include <malloc.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#ifdef _WIN32
unsigned long MC_ThreadLocalStorageIndex;
#else
pthread_key_t threadKey;
#endif

int keyCreated = 0;

#endif //MEMORY_SYSTEM

UInt32 totalAllocatedMemory = 0;
UInt32 totalFreedMemory = 0;
UInt32 totalPeakUsage = 0;
UInt32 totalForPeakUsage = 0;

static void* MEM_SystemCheckedMalloc(size_t size, const char * filename, int lineno) {
    void *ptr = NULL;

    if (size == 0) {
        ptr = malloc(4);
        //ERROR_ReportWarning("Allocating 0 bytes");
    }
    else if (size < 0)
    {
        ERROR_ReportError("Size should be positive");
    }
    else {
        ptr = malloc(size);
    }

    if (ptr == NULL) {
        ERROR_ReportError("Ran out of Memory. "
                          "Run in debugger to see the location.");
    }

    if (MEMSET_FULL_POOL) {
        memset(ptr, 0, size);
    }

#ifdef MEMORY_SYSTEM

#ifdef MEM_DEBUG
//        if (size > 1024)
            printf("%d bytes memory allocated at %s : %d\n", size, filename, lineno);
#endif //MEM_DEBUG

    totalAllocatedMemory += size;
    totalForPeakUsage += size;
    if (totalForPeakUsage > totalPeakUsage)
        totalPeakUsage = totalForPeakUsage;

#ifdef PARALLEL
    if (keyCreated == 1)
    {
        MemoryUsageData *usageDataPtr = NULL;

#ifdef _WIN32
        usageDataPtr = (MemoryUsageData *)TlsGetValue(MC_ThreadLocalStorageIndex);
#else
        usageDataPtr = (MemoryUsageData *)pthread_getspecific(threadKey);
#endif //_WIN32

        if (usageDataPtr == NULL)
            ERROR_ReportError("Failed to get specific value for thread key");
        usageDataPtr->totalAllocated += size;
        usageDataPtr->forPeakUsage += size;
        if (usageDataPtr->forPeakUsage > usageDataPtr->peakUsage)
            usageDataPtr->peakUsage = usageDataPtr->forPeakUsage;
    }
#endif //PARALLEL
#endif //MEMORY_SYSTEM

    return ptr;
}

void MEM_free(void *ptr) {

#ifdef MEMORY_SYSTEM

    size_t size = 0;

    #ifdef __linux__
        size = malloc_usable_size(ptr);
    #endif
    #ifdef __MACH__
        size = malloc_size(ptr);
    #endif
    #ifdef __sparc__
        unsigned int headSize = sizeof(int) + sizeof(unsigned int);
        size = *((unsigned int *) ((char *)ptr - headSize));
    #endif
    #ifdef _WIN32
        size = _msize(ptr);
    #endif

    totalForPeakUsage -= size;
    totalFreedMemory += size;

#ifdef PARALLEL
    if (keyCreated == 1)
    {
        MemoryUsageData *usageDataPtr = NULL;

#ifdef _WIN32
        usageDataPtr = (MemoryUsageData *)TlsGetValue(MC_ThreadLocalStorageIndex);
#else
        usageDataPtr = (MemoryUsageData *)pthread_getspecific(threadKey);
#endif

        if (usageDataPtr == NULL)
            ERROR_ReportError("Failed to get specific value for thread key");
        usageDataPtr->forPeakUsage -= size;
        usageDataPtr->totalFreed += size;
    }
#endif //PARALLEL
#endif //MEMORY_SYSTEM

    free(ptr);
}

void *MEM_Malloc(size_t size, const char *filename, int lineno)
{
    void *ptr = MEM_SystemCheckedMalloc(size, filename, lineno);

    return ptr;
}

void MEM_CreateThreadData() {
#ifdef MEMORY_SYSTEM
#ifdef PARALLEL
#ifdef _WIN32
    MC_ThreadLocalStorageIndex = TlsAlloc();
    if (MC_ThreadLocalStorageIndex == TLS_OUT_OF_INDEXES)
        ERROR_ReportError("Failed to create thread key");
    else
        keyCreated = 1;
#else
    if (pthread_key_create(&threadKey, NULL) != 0)
        ERROR_ReportError("Failed to create thread key");
    else
        keyCreated = 1;
#endif // _WIN32
#endif // PARALLEL
#endif // MEMORY_SYSTEM
}

void MEM_InitializeThreadData(MemoryUsageData *usageData)
{
#ifdef MEMORY_SYSTEM

#ifdef PARALLEL
    if (keyCreated == 1)
    {
#ifdef _WIN32
        if (TlsSetValue(MC_ThreadLocalStorageIndex, usageData) == 0)
#else
        if (pthread_setspecific(threadKey, usageData) != 0)
#endif
        {
            ERROR_ReportError("Failed to store thread specific value");
        }
    }
#endif //PARALLEL
#endif //MEMORY_SYSTEM
}

void MEM_PrintThreadData()
{
#ifdef MEMORY_SYSTEM

#ifdef PARALLEL
    if (keyCreated == 1)
    {
        MemoryUsageData *usageDataPtr = NULL;

#ifdef _WIN32
        usageDataPtr = (MemoryUsageData *)TlsGetValue(MC_ThreadLocalStorageIndex);
#else
        usageDataPtr = (MemoryUsageData *)pthread_getspecific(threadKey);
#endif

        if (usageDataPtr == NULL) {
            ERROR_ReportError("Failed to get specific value for thread key");
        }
        MEM_ReportPartitionUsage(usageDataPtr->partitionId,
                                 usageDataPtr->totalAllocated,
                                 usageDataPtr->totalFreed,
                                 usageDataPtr->peakUsage);
    }
#endif //PARALLEL

#endif //MEMORY_SYSTEM
}

void MEM_ReportPartitionUsage(int    partitionId,
                              UInt32 totalAllocatedMemory,
                              UInt32 totalFreedMemory,
                              UInt32 totalPeakUsage) {
#ifdef MEMORY_SYSTEM
    printf("Memory Usage for Partion %d : Allocated Memory (%ld), Freed Memory (%ld), Peak Usage (%ld)\n",
           partitionId, totalAllocatedMemory, totalFreedMemory, totalPeakUsage);
#endif
}

void MEM_ReportTotalUsage(UInt32 totalAllocatedMemory,
                          UInt32 totalFreedMemory,
                          UInt32 totalPeakUsage) {
#ifdef MEMORY_SYSTEM
    printf("Total Memory Usage : Allocated Memory(%ld), Freed Memory(%ld), Peak Usage(%ld).\n",
           totalAllocatedMemory, totalFreedMemory, totalPeakUsage);
#endif
}
