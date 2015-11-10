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

#if defined(_WIN32) || defined(_WIN64)
#include <Winsock2.h>
#else
#include <errno.h>
#include <sys/socket.h>
#endif

#include <stdio.h>
#include <SPConnection.h>
#include <SopsProperty.h>
#include <SopsRpcInterface.h>

#ifdef SOPS_DEBUG
extern void SopsLog(const char *printstr, ...);
#endif

// Receive data from the connection. This is a non-blocking read, so it
// may receive zero bytes. If anything is received, complete commands are
// processed by the SopsRpcInterface::processRpcCommand. Partial commands
// are saved in m_RpcCmd until recv is called again.
//     -1:   error or disconnect
//     >= 0: number of bytes received.
int SPConnection::recv()
{
    SopsRpcInterface *sri = SopsPropertyManager::getInstance()->getInterface();
    int              nBytes;
    char             *pCmdEnd;
    char             *pCmdStart;
    char             buf[BUFFER_SIZE];
    int              error_no = 0;
    // In Windows the port is set to non-blocking via ioctlsocket, so no
    // additional flags are needed. In Linux, the MSG_DONTWAIT flag calls
    // for non-blocking i/o.
#if defined(_WIN32) || defined (_WIN64)
    const int        flags = 0;
#else // Linux
    const int        flags = MSG_DONTWAIT;
#endif

    if (m_type != SharedMemory)
    {
        nBytes = ::recv(m_sock, buf, BUFFER_SIZE - 1, flags);
#ifdef SOPS_EXTRA_DEBUG
        SopsLog("SPConnection::recv(sock=%d) nBytes=%d\n",
            m_sock, nBytes);
#endif
        if (nBytes == 0)
        {
            // The remote has gracefully shutdown the connection.
            // Return -1 so that this SPConnection will be dropped.
            return -1;
        }

        if (nBytes < 0)
        {
            // An error occured or the socket was disconnected. 
#if defined(_WIN32) || defined(_WIN64)
            error_no = WSAGetLastError();
            if (error_no == WSAEWOULDBLOCK )
            {
                // non-blocking read when no data ready
                m_errno = error_no;
                return 0;
            }

            if (error_no != WSAECONNRESET)
            {
                // Report error except for disconnection
#ifdef SOPS_DEBUG
                SopsLog(
#else
                fprintf(stderr,
#endif
                    "SPConnection::recv "
                    "Winsock recv returned errno %d on socket %d\n",
                    error_no, m_sock);
            }
#else
            error_no = errno;
            if (error_no == EWOULDBLOCK)
            {
                // non-blocking read found no data ready
                m_errno = error_no;
                return 0;
            }

            if (error_no != ECONNRESET)
            {
#ifdef SOPS_DEBUG
                SopsLog(
#else
                fprintf(stderr,
#endif
                    "SPConnection::recv TCP connection error, errno=%d",
                    error_no);
            }            
#endif
            m_errno = error_no;
            return nBytes;
        }
    }
    else
    {
        // Shared memory read
#ifdef SOPS_DEBUG
        SopsLog(
#else
        fprintf(stderr,
#endif
            "Shared memory not supported in current version of "
            "SPConnection.\n");
        return -1; // 
    }

    // Process the received data
    sri->m_lastErrno = 0;

    // Vops sent something. 
    buf[nBytes] = '\0';
    pCmdStart = buf;
#ifdef SOPS_EXTRA_DEBUG
    SopsLog("SPConnection::recv sock=%d res=%d %s\n", m_sock, nBytes, buf);
#endif

    while (*pCmdStart != '\0') // Loop while there is something
    {
        pCmdEnd = strchr(pCmdStart, ETX);
        if (pCmdEnd == NULL)
        {
            // There is no end of command in the buff, so save
            // the incomplete command in the std::string for
            // storage size management.
            m_RpcCmd += pCmdStart;

            // Return for now - future recv's will complete the command.
            return nBytes;
        }

        // There is an end of command character in the buffer.
        // Check for leftover from last buffer.
        *pCmdEnd = '\0'; // Change ETX to terminator.
        if (m_RpcCmd.size() > 0)
        {
            // Append the end of the command to the saved part.
            m_RpcCmd += pCmdStart;

            // Process the completed command
            sri->processRpcCommand(m_RpcCmd.c_str(), this);
            m_RpcCmd.clear();
        }
        else
        {
            // At this point the entire command is in the buffer, 
            // so using the slower std::string is not needed. 
            sri->processRpcCommand(pCmdStart, this);
        }

        // Point to the start of the next comnmand in the buffer,
        // or a nil character if the buffer is done.
        pCmdStart = pCmdEnd + 1;
    }
    return nBytes;
}


// Send data from the connection and return status:
//     -1 on error or disconnect
//    >=0 number of bytes sent on success
int SPConnection::send(const void *buf, int len)
{ 
    int error_no = 0;
    if (m_type != SharedMemory)
    {
        // Send data over the SOPS/VOPS interface.
        // Note that if the receiver is not ready,
        // send() may fail with error no. WSAEWOULDBLOCK,
        // since we're using a non-blocking socket.
        // In such cases, we must try again.
        int res = -1;
#if defined(_WIN32) || defined(_WIN64)
        pthread_mutex_t* sendMutex = SPConnectionManager::getInstance()->getSendMutex();
#endif
        while (res < 0)
        {
#if defined(_WIN32) || defined(_WIN64)
            pthread_mutex_lock(sendMutex);
#endif
            res = ::send(m_sock, (const char*)buf, len, 0);
#if defined(_WIN32) || defined(_WIN64)
            pthread_mutex_unlock(sendMutex);
#endif
            if (res < 0)
            {
                // An error occured or the socket was disconnected. 
#if defined(_WIN32) || defined(_WIN64)
                error_no = WSAGetLastError();
                
                // Report error if other than a disconnect or
                // receiver not ready
                if (error_no == WSAECONNRESET)
                {
                    m_errno = error_no;
                    return -1;
                }
                if (error_no != WSAEWOULDBLOCK)
                {
#ifdef SOPS_DEBUG
                    SopsLog(
#else
                    fprintf(stderr,
#endif
                        "SPConnection::send "
                        "Winsock send returned errno %d on socket %d\n",
                        error_no, m_sock);
                    m_errno = error_no;
                    return -1;
                }
            }
#else
                error_no = errno;
                if (error_no != ECONNRESET)
                {
#ifdef SOPS_DEBUG
                    SopsLog(
#else
                    fprintf(stderr,
#endif
                        "SPConnection::send TCP connection error - errno=%d",
                        error_no);
                    
                }
                m_errno = error_no;
            }
#endif
        }
#ifdef SOPS_EXTRA_DEBUG
        SopsLog("SPConnection::send(sock=%d) buf=%s\n", m_sock, buf);
#endif
        return res;
    }
    else
    {
        // Shared memory read -- should never come here until 
        // SHMEM is implemented.
#ifdef SOPS_DEBUG
        SopsLog(
#else
        fprintf(stderr,
#endif
            "Shared memory not supported in current version of "
            "SPConnection.\n");
        return -1; // 
    }
}

void SPConnection::disconnect()
{
    if (m_cntd)
    {
        m_cntd = false;
#if defined(_WIN32) || defined(_WIN64)
        ::shutdown(m_sock, SD_BOTH);
#else
        ::shutdown(m_sock, SHUT_RDWR);
#endif
    }
}

// Destructor -- close the socket.
SPConnection::~SPConnection()
{
    disconnect();
}


// Singleton 
SPConnectionManager* SPConnectionManager::m_single = NULL;

SPConnectionManager *SPConnectionManager::getInstance()
{
    if (m_single == NULL)
    {
        m_single = new SPConnectionManager;
    }
    return m_single;
}

// Add a connection to be managed
void SPConnectionManager::add(SPConnection *spc)
{
    pthread_rwlock_wrlock(&m_rwlock);
    m_connections.push_back(spc);
    pthread_rwlock_unlock(&m_rwlock);
    m_ncons++;
    
#ifdef SOPS_DEBUG
    SopsLog("SPConnectionManager::add(sock=%d)\n", spc->getSock());
#endif
}


// Drop a managed connection using an iterator and return the next 
// connection from the iterator sequence.
std::vector<SPConnection*>::iterator 
SPConnectionManager::drop(std::vector<SPConnection*>::iterator dropit)
{
#ifdef SOPS_DEBUG
    SopsLog("SPConnectionManager::drop(sock=%d) subbed=%s nconnections=%d\n", 
        (*dropit)->getSock(), (m_subbed ? "true" : "false"), m_ncons);
#endif

    // The std::vector erase operation was not behaving well with
    // multiple threads, resulting in a number of problems. For now, 
    // connections are disconnected and set so isConnected() returns
    // false, but are not removed from the vector using erase(). 
    // TBD use a thread-safe container.

    SPConnection *dropspc = *dropit;
    if (dropspc->isConnected())
    {
        dropspc->disconnect(); // Closes the connection
        m_ncons--;
        // Account for dropped subscribers.
        if (dropspc->isSubscribed())
        {
            m_nsubs--;
        }
    }

#ifdef SOPS_DEBUG
    SopsLog("Remaining subscribers: %d\n", m_nsubs);
#endif

    // If this was ever subscribed and now there are no connections, 
    // it's time to exit.
    if (m_subbed && m_ncons < 1)
    {
#ifdef SOPS_DEBUG
        SopsLog(" -- Shutdown.\n");
#endif
        SPConnectionManager::shutdown();
    }
    
    dropit++;
    return dropit;
}

// Receive using a non-blocking read from each Scenario Player connection.
void SPConnectionManager::recv()
{
    int res;
    SPConnection *spc;

    pthread_rwlock_rdlock(&m_rwlock); 
    std::vector<SPConnection*>::iterator it = m_connections.begin();
    while (it != m_connections.end())
    {
        spc = *it;
        // check for connected and subscribed
        if (spc->isConnected())
        {
            res = spc->recv();
            if (res < 0)
            {
                // Detected an error or disconnecton.
                // drop return next iterator
                it = drop(it);
                continue;
            }
        }

        // Get next connection
        if (it != m_connections.end()) it++;
    }
    pthread_rwlock_unlock(&m_rwlock); 
}

// Send data to each subscribed Scenario Player connection.
void SPConnectionManager::send(const void *buf, int len)
{
    int res = 0;

    // Loop thru each connection and repeat the send.
    // TBD -- consider multicasting, multicast with assured delivery, etc.
    // TBD -- Also consider implementing a service/daemon for multi connections.

    pthread_rwlock_rdlock(&m_rwlock);
    for (std::vector<SPConnection*>::iterator it = m_connections.begin();
         it != m_connections.end(); it++)
    {
        // check for connected and subscribed
        if ((*it)->isConnected() && (*it)->isSubscribed())
        {
            res = (*it)->send(buf, len);
            // For thread-safety, connections will only be dropped 
            // when recv detects a disconnection
        }
    }
    pthread_rwlock_unlock(&m_rwlock);
}

// Send info about the current scenario to this SP connection
void SPConnection::sendScenarioInfo(std::string scenarioName)
{
    char scenName[2048];    
    int cmdLen;
    
    cmdLen = sprintf(scenName, "%s%c", 
            scenarioName.c_str(), ETX);
    this->send(scenName, cmdLen);
}
// Subscribe a connection
void SPConnectionManager::subscribe(SPConnection *spc)
{
    // Must be connected and not subscribed)
    if (spc->isConnected() && !spc->isSubscribed())
    {
        // Enable sending future property changes
        spc->setSubscribed(true);
        m_subbed = true;
        m_nsubs++;

        // Initialize all the properties
        SopsPropertyManager::getInstance()->sendAll(spc);
    }
}

// Remove a connection
void SPConnectionManager::unsubscribe(SPConnection *spc)
{
    // Find the connection and drop it
    for (std::vector<SPConnection*>::iterator it = m_connections.begin();
         it != m_connections.end();
         ++it)
    {
        if (*it == spc)
        {
            drop(it);
            break;
        }
    }
}

// Close all connections -- program will exit.
void SPConnectionManager::shutdown()
{
#ifdef SOPS_DEBUG
    SopsLog("SPConnectionManager::shutdown()\n");
#endif

#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif

    ::exit(0);
}
