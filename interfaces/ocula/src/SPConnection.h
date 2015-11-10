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

#ifndef _SPCONNECTION_H_
#define _SPCONNECTION_H_

#include <pthread.h>
#include <string>
#include <vector>

#define MAX_SHMEM_NAME_LEN 32

// /**
// CLASS       :: SPConnection
// DESCRIPTION :: Contains information about one Scenario Player
//                connection.
// NOTE        :: Shared Memory connection is not implemented yet.
//                Only TCP is used. When type is "undefined" TCP is default.
// **/
class SPConnection
{
public:
    enum Type
    {
        undefined,
        TCP,
        SharedMemory
    };

    SPConnection(int _sock)
    {
        m_addr = NULL;
        m_cntd = true;
        m_ctrl = false;
        m_errno = 0;
        m_name[0] = '\0';
        m_sock = _sock;
        m_subs = false;
        m_type = TCP;
    }

    int getLastErrno() {return m_errno;}
    int getSock() {return m_sock;}
    bool isConnected() {return m_cntd;}
    bool isSubscribed() {return m_subs;}

    ~SPConnection();

    // FUNCTION   :: disconnect
    // PURPOSE    :: Shuts down the connection.
    // PARAMETERS :: none.
    void disconnect();

    // FUNCTION   :: recv
    // PURPOSE    :: Perform a non-blocking read from the connection
    //               using either the socket or shared memory if
    //               designated for the connection. Process complete
    //               RPC commands
    // PARAMETERS :: none
    // RETURNS    :: number of bytes on success; -1 on error or disconnect
    // NOTE       :: Called from the EXTERNAL_Interface receive function.
    int recv();

    // FUNCTION   :: send
    // PURPOSE    :: Sends data to the connection using either the socket
    //               or shared memory if designated for the connection.
    // PARAMETERS :: buf     : void *   : address of the output data
    //            :: len     : int      : number of bytes to send
    // RETURNS    :: number of bytes on success; -1 on error
    int send(const void *buf, int len);
    void sendScenarioInfo(std::string scenarioName);

    void setConnected(bool cntd) {m_cntd = cntd;}
    void setSubscribed(bool subs) {m_subs = subs;}

    // TBD -- provide a function to switch to Shared Memory mode. 
    // e.g., enableSharedMemoryMode()

private:
    void  *m_addr;  // Shared memory address
    bool  m_cntd;  // Connected flag
    bool  m_ctrl;  // Accept control RPC calls (play, pause, ...)
    int   m_errno; // Last errno
    char  m_name[MAX_SHMEM_NAME_LEN];  // Shared memory name
    std::string m_RpcCmd; // Holds incomplete commands waiting for more data
    int   m_sock;  // TCP socket - also key value for the class
    bool  m_subs;  // Has subscribed to properties
    Type  m_type;  // Select Shared Memory or TCP
};


// /**
// CLASS       :: SPConnectionManager
// DESCRIPTION :: Manages a set of SPConnection objects.
// **/
class SPConnectionManager
{
public:
    // Singleton
    static SPConnectionManager *getInstance();

    // FUNCTION   :: add
    // PURPOSE    :: Adds an SPConnection object to the manager.
    // PARAMETERS :: p  : SPConnection * : object pointer
    void add(SPConnection *spc);

    // FUNCTION   :: drop
    // PURPOSE    :: Remove an SPConnection onject from the manager. 
    // PARAMETERS :: dropit : std::vector<SPConnection*>::iterator
    // RETURN     :: std::vector<SPConnection*>::iterator pointing to the
    //               next connection in the iteration sequence
    std::vector<SPConnection*>::iterator
    drop(std::vector<SPConnection*>::iterator dropit);

    // FUNCTION   :: recv
    // PURPOSE    :: Called by the Scenario-Player EXTERNAL_Interface.
    //               Calls the receive function for each SP connection.
    // PARAMETERS :: none
    void recv();

    // FUNCTION   :: send
    // PURPOSE    :: Send data to each Scenario Player connection that
    //               indicates it has subscribed.
    // PARAMETERS :: buf     : void *   : address of the output data
    //            :: len     : int      : number of bytes to send
    void send(const void *buf, int len);

    // FUNCTION   :: size
    // RETURNS    :: number of connections
    int size() {return m_ncons;}

    // FUNCTION   :: subscribe
    // PURPOSE    :: Sets up a new subscriber.
    // PARAMETERS :: p   : SPConnection* : address of subscribing connection
    void subscribe(SPConnection *spc);

    // FUNCTION   :: unsubscribe
    // PURPOSE    :: Removes a subscriber.
    // PARAMETERS :: p   : SPConnection* : address of subscribing connection
    void unsubscribe(SPConnection *spc);

    // FUNCTION   :: shutdown
    // PURPOSE    :: Closes all the connections and causes the program to exit.
    void shutdown();

    int getNumberOfSubscribers() {return m_nsubs;}
    pthread_mutex_t* getSendMutex() {return &m_sendMutex;}

private:

    SPConnectionManager()
    {
        pthread_rwlock_init(&m_rwlock, NULL);
        pthread_mutex_init(&m_sendMutex, NULL);
        m_subbed = false;
        m_ncons = 0;
        m_nsubs = 0;
    }

    std::vector<SPConnection*>  m_connections;
    int                         m_ncons;  // Number of connections
    int                         m_nsubs;  // Number of subscribed connections
    pthread_rwlock_t            m_rwlock;
    pthread_mutex_t             m_sendMutex;
    static SPConnectionManager *m_single;
    bool                        m_subbed; // Ever been subscribed?
};

#endif // ifdef _SPCONNECTION_H_