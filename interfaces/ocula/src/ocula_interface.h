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

#ifndef _OCULA_INTERFACE_H_
#define _OCULA_INTERFACE_H_

#include "external.h"
#include "external_socket.h"
#include "ocula_state.h"

// /**
// STRUCT      :: OculaData
// DESCRIPTION :: Data that is used for this external interface
// **/
class OculaData
{
public:
    void setProperty(const std::string key, const std::string& val, clocktype currentTime);

    EXTERNAL_Socket s;

    OculaState* state;
};

//---------------------------------------------------------------------------
// External Interface API Functions
//---------------------------------------------------------------------------

// /**
// API       :: OculaInitialize
// PURPOSE   :: Initialize ocula data structures
// PARAMETERS::
// + iface : EXTERNAL_Interface* : The interface
// + nodeInput : NodeInput* : The configuration file data
// RETURN    :: void
// **/
void OculaInitialize(
    EXTERNAL_Interface *iface,
    NodeInput *nodeInput);

// /**
// API       :: OculaInitializeNodes
// PURPOSE   :: This will wait for a socket connection from Ocula
// PARAMETERS::
// + iface : EXTERNAL_Interface* : The interface
// + nodeInput : NodeInput* : The configuration file data
// RETURN    :: void
// **/
void OculaInitializeNodes(
    EXTERNAL_Interface *iface,
    NodeInput *nodeInput);

// /**
// API       :: OculaReceive
// PURPOSE   :: This function will receive packets through the opened socket
// PARAMETERS::
// + iface : EXTERNAL_Interface* : The interface
// RETURN    :: void
// **/
void OculaReceive(EXTERNAL_Interface *iface);

// /**
// API       :: OculaFinalize
// PURPOSE   :: This function will finalize this interface
// PARAMETERS::
// + iface : EXTERNAL_Interface* : The interface
// RETURN    :: void
// **/
void OculaFinalize(EXTERNAL_Interface *iface);

#endif /* _OCULA_INTERFACE_H_ */
