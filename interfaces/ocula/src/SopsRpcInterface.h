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

#ifndef _SOPS_RPC_INTERFACE_
#define _SOPS_RPC_INTERFACE_

#include "RpcInterface.h"
#include "SopsProperty.h"
#include <pthread.h>

// /**
// CLASS       :: SopsRpcInterface
// DESCRIPTION :: Runs in SOPS. Provides SOPS with RPC calls to VOPS
//                and implements RPC calls using the method selected
//                in RpcInterface.h
// **/
class SopsRpcInterface
{
public:
    // 
    SopsRpcInterface(
        unsigned short portno,
        SopsPropertyManager *propertyManager);

    // This function is called in a separate thread to listen for connections
    // and eventually wait on a blocking read once the connection has been 
    // established and data initialized.
    void serverThread();

    // These are the RPCs in VOPS that SOPS can call.
    bool deleteProperty(const std::string& key);
    void setProperty(const std::string& key, const std::string& value);
    void exit();

    // Process received RPCs.
    void processRpcCommand(const char*, SPConnection*);

    // getters
    int getLastErrno() {return m_lastErrno;}
    bool isConnected() {return m_connectionState>0;}

    // Manage connection
    int m_connectionState;
    int m_lastErrno;
    unsigned short m_portno;

private:
    SopsPropertyManager *m_propertyManager;
};
#endif
