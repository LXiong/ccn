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
// use in compliance with the license agreement as part of the Qualnet
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

#ifndef _RPC_INTERFACE_H_
#define _RPC_INTERFACE_H_

// Common definitions for the RPC Interface implementation

// Windows may not define all the same errno values as linux. 
// All the specific ones used in the code must be defined.
#include <errno.h>
#ifndef ENOLINK
#define ENOLINK 67
#endif

#ifndef ENOBUFS
#define ENOBUFS 105
#endif

#ifndef EMSGSIZE
#define EMSGSIZE 90
#endif

// This is the buffer size for the RPC reader.
#define BUFFER_SIZE (32 * 1024)

const char ETX = 0x03;

// The RPC interface uses single byte values to represent the function
// being called. This enum defines the values for the functions. It uses
// the printable character range to avoid using control character values.
const char Play = '1';
const char Pause = '2';
const char DeleteProperty = 'D';
const char Hitl = 'H';
const char RequestScenarioInfo = 'R';
const char SetProperty = 'S';
const char Subscribe = '+';
const char Unsubscribe = '-';
const char Exit = 'X';


#endif // _RPC_INTERFACE_H_
