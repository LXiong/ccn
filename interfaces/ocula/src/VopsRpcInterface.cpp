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

#include "VopsRpcInterface.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <sys/types.h> 
#include <time.h>

#ifdef RPC_DEBUG
void VopsLog(const char *printstr, ...);
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <Winsock2.h>
#include <windows.h>
#else
#include <unistd.h>
#include <errno.h>
#endif

#include "CommonFunctions.h"

/*
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
using namespace boost::interprocess;
static shared_memory_object *shared;
static mapped_region *region;

static OPS_SharedMap *pSharedMap = NULL;
static char *pSharedData = NULL; // Using char type for pointer arithmetic


// /**
// STRUCT     :: shutdown_shmem
// PURPOSE    :: Destructor registers disconnect if program is shutdown 
//               gracefully, as by calling exit().
// **
struct shutdown_shmem
{
    ~shutdown_shmem()
    {
        if (pSharedMap)
        {
            pSharedMap->connectionState = 0;
            pSharedMap->connection_change.notify_one();

#ifdef RPC_DEBUG
            printf("shutdown: shmem: Sent connection_change.notify_one(). %d \n", pSharedMap->connectionState);
#endif

        }
    }
} disconnect;

// /**
// FUNCTION   :: VOPS_ShmemRead
// PURPOSE    :: Reads up to maxlen bytes of data from shared memory.
//               This process is designed to be called from one single
//               thread devoted to reading shared memory, so it is not
//               safe to call from multiple threads.
// PARAMETERS ::
// + data      : void *     : pointer to the buffer to receive data
// + maxlen    : int        : maximum bytes to read into data
// RETURN      : Success    : number of bytes read
//             : Failure    : -1
// **
int VOPS_ShmemRead(void *data, int maxlen)
{
    // If the shared memory pipe is empty, wait on the condition variable.
    while (pSharedMap->ifree == pSharedMap->istart)
    {
        // Pipe is empty. Wait for data to be added.
        OPS_Sleep(1);
        
    }
    int ieot = pSharedMap->ifree;  // grab the end index
    int isot = pSharedMap->istart; // grab the start index
    int len = ieot - isot; // length if not wrapped
    if (len < 0) // wrapped
    {
        len = SHARED_MEMORY_MAX - isot + ieot; // wrapped length
    }
    if (len > maxlen) // limit copy size to buffer max
    {
        len = maxlen;
    }

    if (isot + len > SHARED_MEMORY_MAX) // wrapped?
    {
        // Copy up to wrap around point
        int prewrap = SHARED_MEMORY_MAX - isot;
        memcpy(data, pSharedData + isot, prewrap);
        // Copy the rest from start of buffer
        memcpy((char *)data + prewrap, pSharedData, len - prewrap);
    }
    else
    {
        // non-wrapped copy
        memcpy(data, pSharedData + isot, len);
    }

        
    {
        // Update the start of data to after what got removed
#ifdef RPC_DEBUG
        printf("\n istart is : %d, isot = %d, len = %d \n Data that was read : \n%s\n", pSharedMap ->istart, isot, len, (char *)data);
        fflush(stdout);
#endif
        pSharedMap->istart = (isot + len) % SHARED_MEMORY_MAX;
    }
#ifdef RPC_DEBUG
        printf("shmemRead maxlen=%d, sot=%d, eot=%d, len=%d, new sot=%d\n",
        maxlen, isot, ieot, len, pSharedMap->istart);
        fflush(stdout);
#endif
    return len;
}
#endif
*/

#if defined(_WIN32) || defined(_WIN64)
#include <WS2tcpip.h>
#include <WSPiApi.h>
// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#define err_no (WSAGetLastError())
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define err_no errno
#define SOCKET int
#endif

static SOCKET sock;
const char* LOCALHOST = "127.0.0.1";

static pthread_t theClientThread;

// /**
// FUNCTION :: runClientThread
// PURPOSE  :: Runs the object member function in a separate thread.
// **/
void *runClientThread(void *vri)
{
    ((VopsRpcInterface *)vri)->clientThread();
    return NULL;
}

// /**
// FUNCTION :: VopsRpcInterface constructor
// PURPOSE  :: Initializes the data and kicks off the interface.
// **/
VopsRpcInterface::VopsRpcInterface(
    unsigned short portno,
    VopsPropertyManager *propertyManager)
{
    
    m_portno = portno;
    m_hostAddress = LOCALHOST;
    m_propertyManager = propertyManager;

    m_connected = false;
    m_lastErrno = 0;
}

// /**
// FUNCTION :: VopsRpcInterface destructor
// PURPOSE  :: Destroys the interface.
// **/
VopsRpcInterface::~VopsRpcInterface()
{
    stopClientThread();
}

void VopsRpcInterface::connectToSops()
{
    // Kick off the connection to Sops
    pthread_create(&theClientThread, NULL, &runClientThread, this); 
}

// /**
// FUNCTION :: stopClientThread
// PURPOSE  :: Signal thread to stop and wait while it is not
//             completely stopped.
// **/
void VopsRpcInterface::stopClientThread()
{
    m_threadState = THREAD_STOPPING;

    while (m_threadState != THREAD_IDLE)
    {
        OPS_Sleep(50);
    }
}

// /**
// FUNCTION :: clientThread
// PURPOSE  :: This is the version for TCP. It acts as
//             a client of the SopsRpcInterface. It waits to connect,
//             When it connects, it begins to receive data that it 
//             assembles and interprets as remote procedure calls.
// **/
void VopsRpcInterface::clientThread()
{
    m_threadState = THREAD_RUNNING;
    const char *hostname = m_hostAddress.c_str();
    int         nBytes;
    char        readBuffer[BUFFER_SIZE];
    std::string rpcCmd;
    char*       pCmdStart;
    char*       pCmdEnd;
#if defined(_WIN32) || defined(_WIN64)
    int         iResult;
    WSADATA     wsaData;
#else
    sockaddr_in serv_addr;
#endif

    char subscribeCmd[] = {Subscribe,'(',ETX, '\0'};

    // The VopsRpcInterface runs on the Visualizer side and makes
    // remote procedures availabe to Sops. The Vops side
    // of the connection implements the client side of the interface. 

#if defined(_WIN32) || defined(_WIN64)
    // holds address info for socket to connect to
    addrinfo *result = NULL;
    addrinfo *ptr = NULL;
    addrinfo hints;

    // Initialize Winsock2
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);

    assert(iResult == 0);

    // set address info
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    //resolve server address and port on localhost 
    char aport[32];
    sprintf(aport, "%d", m_portno);
    iResult = getaddrinfo(hostname, aport, &hints, &result);

    if( iResult != 0 ) 
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        ::exit(1);
    }

    // Attempt to connect to an address until one succeeds
    sock = INVALID_SOCKET;
    int numSockets = 0;
    int index = 0;
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
    {
        numSockets++;
    }
    SOCKET * socketAry = new SOCKET[numSockets];
    for (index = 0; index < numSockets; index++)
    {
        socketAry[index] = INVALID_SOCKET;
    }

    bool socketConnected = false;
    int attemptNum = 0;
    time_t timeStarted = time(NULL);
    time_t timeNow = timeStarted;
    while (!socketConnected && (timeNow - timeStarted) < 
        SOCKET_CONNECT_TIMEOUT_SECONDS)
    {
        for (ptr = result, index = 0; ptr != NULL; ptr = ptr->ai_next, index++)
        {
            if (socketAry[index] == INVALID_SOCKET)
            {
                socketAry[index] = 
                    socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            }
            attemptNum++;
            iResult = connect(
                socketAry[index], ptr->ai_addr, (int)ptr->ai_addrlen);
            if (iResult == SOCKET_ERROR)
            {
                // Sleep briefly and try again. The server might just be 
                // starting up
                OPS_Sleep(1000);
                std::stringstream buf;
                buf << "Could not connect via socket to " << hostname 
                    << " on port " << m_portno 
                    << ". " << translateErrorCode(WSAGetLastError()) 
                    << " Attempt " << attemptNum << ".";
                logMessageToFile("[warning] ", buf.str());
            }
            else
            {
                sock = socketAry[index];
                socketConnected = true;
                break;
            }
        }
        timeNow = time(NULL);
    }
    delete [] socketAry;
    if (!socketConnected)
    {
        return;
    }
    assert(sock != INVALID_SOCKET); 
#else // LINUX
    // Linux version of the above
    sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    // Create server address
    memset(&serv_addr, 0, sizeof(sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(hostname);
    serv_addr.sin_port = htons(m_portno);
    
    int err;
    int attemptNum = 0;
    time_t timeStarted = time(NULL);
    time_t timeNow = timeStarted;
    bool socketConnected = false;
    while (!socketConnected && (timeNow - timeStarted) < SOCKET_CONNECT_TIMEOUT_SECONDS)
    {
        // Connect to the server
        attemptNum++;
        err = connect(sock, (sockaddr*)&serv_addr, sizeof(sockaddr_in));
        if (err < 0)
        {
            int socket_error_number = errno;
            OPS_Sleep(1000);
            std::stringstream buf;
            buf << "Could not connect via socket to " << hostname 
                << " on port " << m_portno 
                << ". " << translateErrorCode(socket_error_number) 
                << " Attempt " << attemptNum << ".";
            logMessageToFile("[warning] ", buf.str());
        }
        else
        {
            socketConnected = true;
        }
    }
    if (!socketConnected)
    {
        return;
    }
#endif // LINUX

#ifdef RPC_DEBUG
    VopsLog("VopsRpcInterface : clientThread connected on socket = %d\n", sock);
#endif

    m_connected = true;

    // This loop is to read from the connection and translate valid 
    // data received into calls to the callbacks.
    while (m_threadState != THREAD_STOPPING)
    {
        nBytes = recv(sock, readBuffer, BUFFER_SIZE - 1, 0);
        if (nBytes < 0)
        {
            m_lastErrno = err_no;
#if defined(_WIN32) || defined(_WIN64)
            if (m_lastErrno != WSAECONNRESET)
            {
                fprintf(stderr, 
                    "VopsRpcInterface::clientThread: Socket read error #%d\n", 
                    m_lastErrno);
                std::stringstream buf;
                buf << "Socket read error. " 
                    << translateErrorCode(WSAGetLastError());
                logMessageToFile("[warning] ", buf.str());
            }
#else
            if (m_lastErrno != ECONNRESET)
            {
                perror("TCP connection error");
                std::stringstream buf;
                buf << "Socket read error. " 
                    << translateErrorCode(m_lastErrno);
                logMessageToFile("[warning] ", buf.str());
            }
#endif
            m_connected = false;
            break;
        }

#ifdef RPC_DEBUG
        VopsLog("VopsRpcInterface : recv %d bytes.\n", nBytes);
#endif

        m_lastErrno = 0;

        // The buffer may have multiple commands and may have a partial
        // command at the end. Because the command may contain a script, 
        // there is also the potential the whole buffer could be a
        // partial command. The way to find out what is in the buffer
        // is to first look for the ETX that indicates we have the end
        // of a command. 
        readBuffer[nBytes] = '\0';
        pCmdStart = readBuffer;

        while (*pCmdStart != '\0') // Loop while there is something
        {
            pCmdEnd = strchr(pCmdStart, ETX);
            if (pCmdEnd == NULL)
            {
                // There is no end of command in the buff, so save
                // the incomplete command in the std::string for
                // storage size management.
                rpcCmd += pCmdStart;
                // Break this loop to continue the outer loop that
                // will go read more data.
                break;
            }

            // There is an end of command character in the buffer.
            // Check for leftover from last buffer.

            *pCmdEnd = '\0'; // Change ETX to terminator.
            if (rpcCmd.size() > 0)
            {
                // Append the end of the command to the saved part.
                rpcCmd += pCmdStart;

                // Need to use a non-const buffer
                processRpcCommand(&rpcCmd[0]);
                rpcCmd.clear();
            }
            else
            {
                // At this point the entire command is in the buffer, 
                // so using the slower std::string is not needed. 
                processRpcCommand(pCmdStart);
            }

            // Point to the start of the next comnmand in the buffer,
            // or a nil character if the buffer is done.
            pCmdStart = pCmdEnd + 1;
        }
    }

    m_threadState = THREAD_IDLE;
}

// /**
// FUNCTION   :: processRpcCommand
// PURPOSE    :: Provides common code to process commands once they
//               have been received regardless of the selected method
//               of transfer.
// PARAMETERS ::
// + cmd       : char *   : start of command in memory
// **/
void VopsRpcInterface::processRpcCommand(char *cmd)
{
#ifdef RPC_DEBUG
    VopsLog("VopsRpcInterface::processRpcCommand(%s)\n", cmd);
#endif

    char *key = cmd + 2; // Key starts in third byte.
    char *eqpos;

    // Check for valid command
    if (*(cmd + 1) != '(')
    {
        fprintf(stderr, "RPC commmand is garbled: %s\n", cmd);
        fflush(stdout);
        return;
    }

    // Select command
    switch (*cmd)
    {
        case DeleteProperty:
            // there's only one arg, the property key
            m_propertyManager->deleteProperty(std::string(key)); 
            break;

        case SetProperty:
            eqpos = strchr(key, '=');
            if (!eqpos)
            {
                fprintf(stdout, "Invalid RPC call: %s\n", cmd);
            }
            else
            {
                *eqpos = '\0';
                std::string sKey(key);
                std::string sVal(eqpos + 1);
                m_propertyManager->setProperty(sKey, sVal);
            }
            break;

        case Exit:
            fflush(stdout);
            this->exit();
            break;

        default:
            printf("Unrecognized Rpc Command: %s\n", cmd);
    }
}

void VopsRpcInterface::exit()
{
    // Don't send the exit Rpc command -- there may be other VOPS connections
    // so the simulator shouldn't exit just because one Scenario Player ends.
    // SOPS will exit when the last VOPS connection is shutdown.
    //
    //sendRpcCommand(Exit, "");
    //
    //if (m_lastErrno != 0)
    //{
     //   return;
    //}

    int err;

#if defined(_WIN32) || defined(_WIN64)
    err = shutdown(sock, SD_BOTH);
#else
    err = shutdown(sock, SHUT_RDWR);
#endif
    if (err < 0)
    {
        m_lastErrno = err_no;
        std::string errmsg = 
            "VopsRpcInterface::exit() -- Not able to shutdown gracefully ";
        perror(errmsg.c_str());
    }
    else
    {
        m_lastErrno = 0;
    }
    m_connected = false;
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif
}

void VopsRpcInterface::pause()
{
    sendRpcCommand(Pause,"");
}

void VopsRpcInterface::play()
{
    sendRpcCommand(Play,"");
}

void VopsRpcInterface::subscribe()
{
    sendRpcCommand(Subscribe,"");
}

void VopsRpcInterface::unsubscribe()
{
    sendRpcCommand(Unsubscribe,"");
}

void VopsRpcInterface::requestScenarioInfo()
{
    sendRpcCommand(RequestScenarioInfo,"");
}

// /**
// FUNCTION   :: sendRpcCommand
// PURPOSE    :: Sends a command to Sops.
// PARAMETERS ::
// + cmdType   : string   : type of command, usually one character
// + cmdText   : string   : text of the command to be send
// **/
void VopsRpcInterface::sendRpcCommand(char cmdType, const std::string& cmdText)
{
    std::string fullCmd(1, cmdType);
    fullCmd += "(" + cmdText + ETX;

#ifdef RPC_DEBUG
    VopsLog("VopsRpcInterface::send(%s)\n", fullCmd.c_str());
#endif

    if (!isConnected())
    {
#ifdef RPC_DEBUG
        VopsLog("Failed to send command. Not connected.\n");
#endif
        return;
    }

    int err = send(sock, fullCmd.c_str(), fullCmd.size(), 0);
    if (err < 0)
    {
        m_lastErrno = err_no;
        perror("VopsRpcInterface::sendRpc error");
    }
    else
    {
        m_lastErrno = 0;
    }                

}
