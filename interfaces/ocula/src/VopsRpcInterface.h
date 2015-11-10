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

#ifndef _VOPS_RPC_INTERFACE_
#define _VOPS_RPC_INTERFACE_

#include "VopsProperty.h"
#include "RpcInterface.h"

// /**
// CLASS       :: VopsRpcInterface
// DESCRIPTION :: Runs in VOPS. Enables VOPS to receive RPC requests
//                from SOPS. The interface is one-way, so VOPS does
//                not call RPCs in SOPS nor does it return any 
//                results of the RPCs to SOPS.
// **/
class VopsRpcInterface
{
public:
    enum ThreadState { THREAD_IDLE, THREAD_RUNNING, THREAD_STOPPING };

    VopsRpcInterface(
        unsigned short portno,
        VopsPropertyManager *propertyManager);

    virtual ~VopsRpcInterface();

    // This thread will run as a client and sit on a blocking read
    // to get RPC calls from Sops.
    void clientThread();

    void connectToSops();
    void exit();
    void pause();
    void play();
    void subscribe();
    void unsubscribe();
    void requestScenarioInfo();

    // Sends a command using <cmdType>(<cmdText>[ETX]
    void sendRpcCommand(char cmdType, const std::string& cmdText);

    // getters
    int getLastErrno() {return m_lastErrno;}
    bool isConnected() {return m_connected;}
    std::string getHostAddress() const {return m_hostAddress;}

    // setter
    void setHostAddress(std::string hostAddress) {m_hostAddress = hostAddress;}

    unsigned short m_portno;
    std::string m_hostAddress;

private:

    VopsPropertyManager *m_propertyManager;

    // Manage connection
    volatile bool m_connected;
    int m_lastErrno;

    void processRpcCommand(char*);

    // Thread management
    volatile ThreadState m_threadState;
    void stopClientThread();
};
#endif

