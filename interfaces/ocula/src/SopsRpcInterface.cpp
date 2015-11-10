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

#if defined (_WIN32) || defined (_WIN64)
// Winsock headers and library directives

// The macro below prevents a re-definition error of in6_addr_struct in
// include/main.h.
#define _NETINET_IN_H

#include <Winsock2.h>
#include <WS2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#define err_no (WSAGetLastError())

#else
// Linux type of socket connection definitions
#include <sys/socket.h>
#include <netinet/in.h>
#define SOCKET int
#define err_no errno
#endif // WIN32 || WIN64

#include "SopsRpcInterface.h"
#include "SPConnection.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <sys/types.h> 
#include "gui.h"

#ifdef SOPS_DEBUG
extern void SopsLog(const char *printstr, ...);
#endif

//---------------------------------------------------------------------------
// SPReceive is the EXTERNAL_Function for the SPInterface receive
//---------------------------------------------------------------------------
void SPReceive(EXTERNAL_Interface *iface)
{
    // Call the connection manager function to check for data on all
    // SP connections.
    SPConnectionManager::getInstance()->recv();
}

//---------------------------------------------------------------------------
// SPSimulationHorizon is the EXTERNAL_Function for advancing the 
// SPInterface simulation horizon 
//---------------------------------------------------------------------------
void SPSimulationHorizon(EXTERNAL_Interface *iface)
{
    PartitionData* partitionData = iface->partition;
    clocktype nextEventTime = GetNextInternalEventTime(partitionData);
    if (!partitionData->wallClock->isPaused())
    {
        iface->horizon = nextEventTime + 10 * MILLI_SECOND;
    }
}

//---------------------------------------------------------------------------
// TCP
//---------------------------------------------------------------------------
static pthread_t theListenerThread;
// This is the thread for the TCP implementation. It will listen for a client
// to connect. When one does it will accept and create an SPConnection to 
// handle it, then loop back to accept more connections.
void *listenerThread(void *p)
{
    SopsRpcInterface *sri = SopsPropertyManager::getInstance()->getInterface();
    struct sockaddr_in cli_addr;
    unsigned int       clilen = sizeof(cli_addr);
    SOCKET             connectFd;
    SOCKET             listenFd;
    pthread_t          thread;

#if defined (_WIN32) || defined (_WIN64)
    WSADATA            wsaData;
#endif

    // Create a socket and listen for a connection.
#if defined (_WIN32) || defined (_WIN64)
    listenFd = INVALID_SOCKET;
    // When on Windows, init Winsock 2.
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        assert(false);
    }
#endif 

    // Set up socket
    listenFd = socket(AF_INET, SOCK_STREAM, 0);

#if defined (_WIN32) || defined (_WIN64)
    if (listenFd == INVALID_SOCKET)
    {
        char errmsg_buf[256]; 
        strerror_s(errmsg_buf, 255, err_no);
        WSACleanup();
        assert(false);
    }
#else
    assert(listenFd >= 0);
#endif

#ifdef SOPS_DEBUG
    SopsLog("SopsRpcInterface::listenerThread Listen socket = %d\n", listenFd);
#endif

    // Create address
    memset(&cli_addr, 0, sizeof(sockaddr_in));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = INADDR_ANY;
    cli_addr.sin_port = htons(sri->m_portno);

    // Set RESUSEADDR socket option to allow rebind despite the socket
    // being in TIME_WAIT.
    int optval = 1;
    setsockopt (listenFd, SOL_SOCKET, SO_REUSEADDR, 
        (const char *) &optval, sizeof(optval));

    // Bind address to the socket.
    // ::bind disambiguates it for the MSVC10 compiler due to confusion
    // between the socket bind defined in the global namespace (and used 
    // here) and the bind template defined in std namespace.
    int err = ::bind(listenFd, (sockaddr*) &cli_addr, sizeof(sockaddr));
#if defined (_WIN32) || defined (_WIN64)
    assert(err != SOCKET_ERROR);
#else
    if (err < 0)
    {
        ERROR_ReportErrorArgs("Could not bind: %s", strerror(errno));
    }
#endif

    // Listen for a connection
    err = listen(listenFd, 2);
#if defined (_WIN32) || defined (_WIN64)
    if (err == SOCKET_ERROR)
    {
        char errmsg_buf[256]; 
        strerror_s(errmsg_buf, 255, err_no);
        WSACleanup();
        assert(false);
    }
#else
    if (err < 0)
    {
        ERROR_ReportErrorArgs("Could not listen: %s", strerror(errno));
    }
#endif

    // Listen/service connection loop. 
    while (true)
    {
        // Accept connections
#if defined (_WIN32) || defined (_WIN64)
        connectFd = accept(listenFd, NULL, NULL);
        if (connectFd == INVALID_SOCKET)
        {
            char errmsg_buf[256]; 
            strerror_s(errmsg_buf, 255, err_no);
            WSACleanup();
            assert(false);
        }

        // Make the connection socket non-blocking
        u_long iMode=1;
        ioctlsocket(connectFd, FIONBIO, &iMode); 
#else
        connectFd = accept(listenFd, NULL, NULL);
        if (err < 0)
        {
            ERROR_ReportErrorArgs("Could not connect: %s", strerror(errno));
        }
#endif

#ifdef SOPS_DEBUG
        SopsLog("SopsRpcInterface::listenerThread connected on socket %d\n", 
            connectFd);
#endif

        if (sri->m_connectionState == 0)
        {
            sri->m_connectionState = 2;
        }

        // Connected to a client - create a new SPConnection and add
        // it to the connection manager.
        SPConnection *spc = new SPConnection(connectFd);
        SPConnectionManager::getInstance()->add(spc);
    }
    return NULL;
}


//
// FUNCTION    :: SopsRpcInterface constructor
// DESCRIPTION :: Initialize the interface to wait for a connection from vops.
//
SopsRpcInterface::SopsRpcInterface(
        unsigned short portno,
        SopsPropertyManager *propertyManager)
{
    m_portno = portno;
    m_propertyManager = propertyManager;

    m_connectionState = 0;
    m_lastErrno = 0;

    // Kick off the listener thread
    pthread_create(&theListenerThread, NULL, &listenerThread, NULL); 
}

//
// FUNCTION : processRpcCommand
// PURPOSE  : Common function to process commands however they were received.
//
void SopsRpcInterface::processRpcCommand(const char *cmd, SPConnection *spc)
{
    if (*(cmd + 1) != '(')
    {
        printf("SopsRpcInterface received garbled RPC: %s\n", cmd);
#ifdef SOPS_DEBUG
        SopsLog("SopsRpcInterface received garbled RPC: %s\n", cmd);
#endif
        return;
    }

#ifdef SOPS_DEBUG
    SopsLog("SOPS received RPC: %s\n", cmd);
#endif

    switch (*cmd)
    {
        case Exit:
            // Ignore Exit command from Scenario Player -- disconnection of all
            // scenario players will result in an exit.
            break;

        case Hitl:
            if (m_propertyManager->getPartition() != NULL)
            {
                GUI_HandleHITLInput(cmd + 2, m_propertyManager->getPartition());
            }
            else
            {
                printf("Partition undefined -- Unable to process HITL command. "
                       "Did build include addon/kernal?\n");
            }

        case Play:
            if (m_connectionState == 2)
            {
                if (m_propertyManager->getPartition() != NULL 
                    && m_propertyManager->getPartition()->wallClock != NULL)
                {  
                    // unpause simulation
                    m_propertyManager->getPartition()->wallClock->resume();
#ifdef SOPS_DEBUG
                    SopsLog("SopsRpcInterface : Unpaused\n");
#endif
                    fflush(stdout);
                    m_connectionState = 1;
                    m_propertyManager->setProperty("/remoteControl/playEnabled", "1");
                    m_propertyManager->setProperty("/remoteControl/pauseEnabled", "0");
                }
            }
            break;

        case Pause:
            if(m_connectionState == 1)
            {
                if (m_propertyManager->getPartition() != NULL 
                    && m_propertyManager->getPartition()->wallClock != NULL)
                {  
                    // pause simulation
                    m_propertyManager->getPartition()->wallClock->pause();
#ifdef SOPS_DEBUG
                    SopsLog("\nSopsRpcInterface : Paused\n");
#endif
                    fflush(stdout);
                    m_connectionState = 2;
                    m_propertyManager->setProperty("/remoteControl/pauseEnabled", "1");
                    m_propertyManager->setProperty("/remoteControl/playEnabled", "0");
                }
            }
            break;

        case Subscribe:
            SPConnectionManager::getInstance()->subscribe(spc);
            break;

        case Unsubscribe:
            SPConnectionManager::getInstance()->unsubscribe(spc);
            break;

        case RequestScenarioInfo:
            spc->sendScenarioInfo(SopsPropertyManager::getInstance()->getScenarioName());
            break; 

        default:
            printf("Unrecognized RPC call received from VOPS: %s)\n", cmd);
            fflush(stdout);
#ifdef SOPS_DEBUG
            SopsLog("Unrecognized RPC call received from VOPS: %s)\n", cmd);
#endif
    }
}

// /**
// FUNCTION   :: setProperty
// PURPOSE    :: Call the setProperty function in Vops
// PARAMETERS ::
// + key       : string     : Property key 
// + value     : string     : Property value
// **/
void SopsRpcInterface::setProperty(
        const std::string& key, const std::string& value)
{
    char setCmd[2048];
    int cmdLen = 
        sprintf(setCmd, "S(%s=%s%c", key.c_str(), value.c_str(), ETX);
    SPConnectionManager::getInstance()->send(setCmd, cmdLen);
    m_lastErrno = err_no;

#ifdef SOPS_DEBUG
    SopsLog(
        "SopsRpcInterface::setProperty(%s, %s)\n", key.c_str(), value.c_str());
#endif
}

bool SopsRpcInterface::deleteProperty(const std::string& key)
{
    std::string delCmd;
    delCmd = "D(" + key + ETX;

    SPConnectionManager::getInstance()->send(delCmd.c_str(), delCmd.size());

#ifdef SOPS_DEBUG
    SopsLog("VopsRpcInterface::deleteProperty(%s)\n",key.c_str());
#endif

    return true; // The interface doesn't get return values, so always set true.
}

void SopsRpcInterface::exit()
{
    static char exitCmd[4];
    int cmdLen = sprintf(exitCmd, "X(%c", ETX);

#ifdef SOPS_DEBUG
    SopsLog("SopsRpcInterface::exit(%s)\n",exitCmd);
#endif
    
    SPConnectionManager::getInstance()->send(exitCmd, cmdLen);
    SPConnectionManager::getInstance()->shutdown(); // calls ::exit(0)
}
