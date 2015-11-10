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

#ifndef TESTFED_TYPES_H
#define TESTFED_TYPES_H

// 64bit Windows
// -------------------------------------------------------------
#ifdef P64
#define PTRSIZE         64
#define LONGSIZE        32
typedef char            Int8;
typedef unsigned char   UInt8;
typedef short           Int16;
typedef unsigned short  UInt16;
typedef int             Int32;
typedef unsigned int    UInt32;
typedef _int64          Int64;
typedef unsigned _int64 UInt64;
typedef _int64          TimeStampPtr;
typedef float           Float32;
typedef double          Float64;
typedef Int64           IntPtr; // integer big enough to hold a pointer
#else

// 64bit Unix/MacOS X
// -------------------------------------------------------------
#ifdef LP64
#define PTRSIZE         64
#define LONGSIZE        64
typedef char            Int8;
typedef unsigned char   UInt8;
typedef short           Int16;
typedef unsigned short  UInt16;
typedef int             Int32;
typedef unsigned int    UInt32;
typedef long            Int64;
typedef unsigned long   UInt64;
typedef Int64           TimeStampPtr;
typedef float           Float32;
typedef double          Float64;
typedef Int64           IntPtr; // integer big enough to hold a pointer
#else

// 32bit Windows/Unix
// -------------------------------------------------------------
#define PTRSIZE         32
#define LONGSIZE        32
typedef char            Int8;
typedef unsigned char   UInt8;
typedef short           Int16;
typedef unsigned short  UInt16;
typedef int             Int32;
typedef unsigned int    UInt32;
#ifdef WINDOWS_OS
typedef _int64          Int64;
typedef unsigned _int64 UInt64;
#else
typedef long long       Int64;
typedef unsigned long long UInt64;
#endif
typedef int             TimeStampPtr;
typedef float           Float32;
typedef double          Float64;
typedef Int32           IntPtr; // integer big enough to hold a pointer

#endif // LP64
#endif //  P64

#ifdef _WIN32
typedef unsigned __int64 uint64;
#define UINT64_MAX 0xffffffffffffffffui64

#define uint64toa(value, string) sprintf(string, "%I64u", value)
#define atouint64(string, value) sscanf(string, "%I64u", value)
#define UINT64_C(value)          value##ui64
#else /* _WIN32 */
typedef unsigned long long uint64;
#define UINT64_MAX 0xffffffffffffffffULL

#define uint64toa(value, string) sprintf(string, "%llu", value)
#define atouint64(string, value) sscanf(string, "%llu", value)
#define UINT64_C(value)          value##ULL
#endif /* _WIN32 */

// The following macros help in declaring 64 bit imediate values
// :will append the proper suffix code to indicate a 64bit value
// WINDOWS
// -------------------------------------------------------------
#ifdef WINDOWS_OS
#define TYPES_ToInt64(n)   n##i64
#define TYPES_ToUInt64(n)  n##ui64
// UNIX
// -------------------------------------------------------------
#else
#define TYPES_ToInt64(n)   n##ll
#define TYPES_ToUInt64(n)  n##ull
#endif // WINDOWS_OS


#ifdef WINDOWS_OS
#define TYPES_64BITFMT "I64"
#else /* UNIX */
#ifdef LP64
#define TYPES_64BITFMT "l"
#else
#define TYPES_64BITFMT "ll"
#endif
#endif

// wide chars types and functions mapping
// -------------------------------------------------------------
#ifdef WCHARS
#include <wchar.h>
typedef wchar_t CHARTYPE;
#define CHARTYPE_strstr    wcsstr
#define CHARTYPE_strcpy    wcscpy
#define CHARTYPE_strncpy   wcsncpy
#define CHARTYPE_vsscanf   vswscanf
// these APIs are different between windows and unix
#ifdef WINDOWS_OS
#define CHARTYPE_vsnprintf _vsnwprintf
#else
#define CHARTYPE_vsnprintf vswprintf
#endif // WINDOWS_OS
#define CHARTYPE_sscanf    swscanf
#define CHARTYPE_printf    wprintf
#define CHARTYPE_sprintf   wsprintf
#define CHARTYPE_strlen    wcslen
#define CHARTYPE_Cast(x)      L##x
#else

// single byte chars types and functions mapping
// -------------------------------------------------------------
typedef char    CHARTYPE;
#define CHARTYPE_strstr    strstr
#define CHARTYPE_strcpy    strcpy
#define CHARTYPE_strncpy   strncpy
#define CHARTYPE_vsscanf   vsscanf
// these APIs are different between windows and unix
#ifdef WINDOWS_OS
#define CHARTYPE_vsnprintf _vsnprintf
#else
#define CHARTYPE_vsnprintf vsnprintf
#endif // WINDOWS_OS
#define CHARTYPE_sscanf    sscanf
#define CHARTYPE_printf    printf
#define CHARTYPE_sprintf   sprintf
#define CHARTYPE_strlen    strlen
#define CHARTYPE_Cast(x)      x
#endif

#include <cstddef>
typedef Int64 clocktype;

// /**
// CONSTANT    :: CLOCKTYPE_MAX : Platform dependent
// DESCRIPTION :: CLOCKTYPE_MAX is the maximum value of clocktype.
// This value can be anything as long as it is less than or equal to the
// maximum value of the type which is typedefed to clocktype.
// Users can simulate the model up to CLOCKTYPE_MAX - 1.
// **/

#define CLOCKTYPE_MAX TYPES_ToInt64(0x7fffffffffffffff)
// /**
// CONSTANT    :: NANO_SECOND : ((clocktype) 1)
// DESCRIPTION :: Defined as basic unit of clocktype
// **/
#define NANO_SECOND              ((clocktype) 1)

// /**
// CONSTANT    :: MICRO_SECOND : (1000 * NANO_SECOND)
// DESCRIPTION :: Defined as 1000 times the basic unit of clocktype
// **/
#define MICRO_SECOND             (1000 * NANO_SECOND)

// /**
// CONSTANT    :: MILLI_SECOND  : (1000 * MICRO_SECOND)
// DESCRIPTION :: unit of time equal to 1000 times MICRO_SECOND
// **/
#define MILLI_SECOND             (1000 * MICRO_SECOND)

// /**
// CONSTANT    :: SECOND : (1000 * MILLI_SECOND)
// DESCRIPTION :: simulation unit of time =1000 times MILLI_SECOND
// **/
#define SECOND                   (1000 * MILLI_SECOND)

// /**
// CONSTANT    :: MINUTE : (60 * SECOND)
// DESCRIPTION :: unit of simulation time = 60 times SECOND
// **/
#define MINUTE                   (60 * SECOND)

// /**
// CONSTANT    :: HOUR  :    (60 * MINUTE)
// DESCRIPTION :: unit of simulation time = 60 times MINUTE
// **/
#define HOUR                     (60 * MINUTE)

// /**
// CONSTANT    :: DAY  :    (24 * HOUR)
// DESCRIPTION :: unit of simulation time = 24 times HOUR
// **/
#define DAY                      (24 * HOUR)

#endif //TESTFED_TYPES_H
