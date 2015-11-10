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

#ifdef _WIN32
#include <winsock2.h>

// recv() is supposed to return an ssize_t, but on Windows (even
// 64-bit Windows) ssize_t is missing and recv() returns an int.
typedef int ssize_t;

// recvfrom() requires a socklen_t parameter on linux
typedef int socklen_t;
#else /* unix/linux */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#ifdef __APPLE__
#include <sys/select.h>
#endif

#ifdef QT_CORE_LIB

// This file is being compiled by the GUI.   Define some constants and
// functions that are GUI specific.

#include "SVisualizerApp.h"
#include "S3DGui.h"

#ifndef MAX_STRING_LENGTH
#define MAX_STRING_LENGTH 200
#endif

#ifndef MICRO_SECOND
#define MICRO_SECOND 1000
#endif

#ifndef SECOND
#define SECOND 1000000000
#endif

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

void ERROR_ReportWarning(const char* warning)
{
    SVisualizerApp::instance().gui3D()->writeToErrorLog(warning);
}

void ERROR_ReportError(const char* error)
{
    SVisualizerApp::instance().gui3D()->writeToErrorLog(error);
}

void ERROR_ReportWarningArgs(const char* warning, ...) 
{
    QString _warning;
    va_list ap;
    va_start(ap, warning);
    _warning.vsprintf(warning, ap);
    va_end(ap);
    SVisualizerApp::instance().gui3D()->writeToErrorLog(_warning);
}

void ERROR_ReportErrorArgs(const char* error, ...) 
{
    QString _error;
    va_list ap;
    va_start(ap, error);
    _error.vsprintf(error, ap);
    va_end(ap);
    SVisualizerApp::instance().gui3D()->writeToErrorLog(_error);
}

// This class allows us to force the current thread to sleep
class ForceSleep : public QThread
{
public:
    static void usleep(clocktype c) 
    {
        QThread::usleep(c);
    }
};

void EXTERNAL_Sleep(clocktype c)
{
    ForceSleep::usleep(c);
}

// Disable threaded sockets when running under the GUI.
// The GUI does its own threading using QThread.
#define NO_SOCKET_THREADING

#else

// This file is being compiled by QualNet
#include "main.h"
#include "api.h"

#endif /* QT_CORE_LIB */

#include "external_socket.h"

//---------------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------------

// The default socket option size to get
#define DEFAULT_OPT_SIZE 512

// #define DEBUG

// #define DEBUG_THREADED_SOCKET 1

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

#ifdef _WIN32
// Keep track of num sockets; used for initializing gWsaData
static int gNumSockets = 0;

// Data for windows socket library
static WSADATA gWsaData;
#endif

//---------------------------------------------------------------------------
// Static Functions
//---------------------------------------------------------------------------

// /**
// API       :: EXTERNAL_VarArrayIncreaseSize
// PURPOSE   :: This function increases the size of a VarArray.  This should
//              only be called if the minSize parameter is guaranteed to be
//              greater than the VarArray's maxSize.
// PARAMETERS::
// + socket : EXTERNAL_Socket* : Pointer to the socket
// + data : EXTERNAL_VarArray* : Pointer to the VarArray to send
// RETURN    :: EXTERNAL_SocketErrorType : EXTERNAL_NoSocketError if
//              successful, different error if not successful which could
//              be due to a number of reasons.
// **/
static void EXTERNAL_VarArrayIncreaseSize(
    EXTERNAL_VarArray *array,
    unsigned int minSize)
{
    char *newData;

    while (array->maxSize < minSize)
    {
        array->maxSize *= 2;
    }

    newData = new char[array->maxSize];
    memcpy(newData, array->data, array->size);
    memset(newData + array->size, 0, array->maxSize - array->size);
    delete [] array->data;
    array->data = newData;
}

// /**
// API       :: EXTERNAL_SetNonBlocking
// PURPOSE   :: Set the given file descriptor to be non-blocking
// PARAMETERS::
// + fd : int : The file descriptor
// RETURN    :: EXTERNAL_SocketErrorType : EXTERNAL_NoSocketError if
//              successful, different error if not successful which could
//              be due to a number of reasons.
// **/
static EXTERNAL_SocketErrorType SetNonBlocking(SocHandle fd)
{
    int err;
#ifdef _WIN32
    u_long arg = 1;
    char errString[MAX_STRING_LENGTH];

    err = ioctlsocket(fd, FIONBIO, &arg);
    if (err)
    {
        sprintf(errString, "Error setting socket to non-blocking mode, err = \"%s\"",
                           WSAGetLastError());
        ERROR_ReportWarning(errString);
        return EXTERNAL_SocketError;
    }
#else /* unix/linux */
    err = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (err == -1)
    {
        ERROR_ReportWarningArgs("Error calling fcntl, err = %s",
            strerror(errno));
        return EXTERNAL_SocketError;
    }
#endif

    return EXTERNAL_NoSocketError;
}

//---------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------

void EXTERNAL_VarArrayInit(
    EXTERNAL_VarArray *array,
    unsigned int size)
{
    array->maxSize = size;
    array->size = 0;
    array->data = new char[size];
    memset(array->data, 0, size);
}

void EXTERNAL_VarArrayAccomodateSize(
    EXTERNAL_VarArray *array,
    unsigned int size)
{
    if (size > array->maxSize)
    {
        EXTERNAL_VarArrayIncreaseSize(array, size);
    }
}

void EXTERNAL_VarArrayAppendData(
    EXTERNAL_VarArray *array,
    char *data,
    unsigned int size)
{
    if (size + array->size > array->maxSize)
    {
        EXTERNAL_VarArrayIncreaseSize(array, size + array->size);
    }

    memcpy(array->data + array->size, data, size);
    array->size += size;
}

void EXTERNAL_VarArrayConcatString(
    EXTERNAL_VarArray *array,
    const char *string)
{
    unsigned int len = (unsigned int)strlen(string);

    if (array->size == 0)
    {
        if (len + 1 > array->maxSize)
        {
            EXTERNAL_VarArrayIncreaseSize(array, len + 1);
        }
        array->size = len + 1;
    }
    else
    {
        if (len + array->size > array->maxSize)
        {
            EXTERNAL_VarArrayIncreaseSize(array, len + array->size);
        }
        array->size += len;
    }

    strcat(array->data, string);
    assert(strlen(array->data) + 1 == array->size);
}

void EXTERNAL_VarArrayFree(EXTERNAL_VarArray *array)
{
    if (array->maxSize > 0)
    {
        assert(array->data != NULL);
        delete [] array->data;
        memset(array, 0, sizeof(EXTERNAL_VarArray));
    }
}

void EXTERNAL_hton(void* ptr, unsigned size)
{
    char t;
    unsigned int i;
    char *a = (char*) ptr;

    UInt32 hostInt = 0xfeedbeef;
    UInt32 netInt = htonl(hostInt);

    if (hostInt != netInt)
    {
        for (i = 0; i < size / 2; i++)
        {
            t = a[i];
            a[i] = a[size - 1 - i];
            a[size - 1 - i] = t;
        }
    }
}

void EXTERNAL_ntoh(void* ptr, unsigned size)
{
    EXTERNAL_hton(ptr, size);
}

void EXTERNAL_swapBitfield(void *ptr, unsigned size)
{
    if (size == 1)
    {
        unsigned char tmp = 0;
        unsigned char *data = (unsigned char *)ptr;
        int i;

        for (i = 0; i < 8; i++)
        {
            if ((*data) & (1 << i))
            {
                tmp = tmp | (1 << (7 - i));
            }
        }
        *data = tmp;
    }
}

void EXTERNAL_SocketInit(
    EXTERNAL_Socket *s,
    bool blocking,
    bool threaded)
{
#ifdef _WIN32
    // Initialize the windows socket library
    if (gNumSockets == 0)
    {
        if (WSAStartup(MAKEWORD(1, 1), &gWsaData) != 0)
        {
            ERROR_ReportError("Error loading winsock");
        }
    }
    gNumSockets++;
#endif

    s->isOpen = false;
    s->error = false;
    s->socketFd = -1;
    s->blocking = blocking;

    if (threaded)
    {
#ifndef NO_SOCKET_THREADING
        s->threaded = true;
        s->head = 0;
        s->tail = 0;
        s->size = 0;
        s->sendHead = 0;
        s->sendTail = 0;
        s->sendSize = 0;

        s->mutex = new pthread_mutex_t;
        pthread_mutex_init (s->mutex, NULL);
        s->notFull = new pthread_cond_t;
        pthread_cond_init(s->notFull, NULL);
        s->notEmpty = new pthread_cond_t;
        pthread_cond_init(s->notEmpty, NULL);

        s->sendMutex = new pthread_mutex_t;
        pthread_mutex_init (s->sendMutex, NULL);
        s->sendNotFull = new pthread_cond_t;
        pthread_cond_init(s->sendNotFull, NULL);
        s->sendNotEmpty = new pthread_cond_t;
        pthread_cond_init(s->sendNotEmpty, NULL);
#else
        // Cannot use threading when NO_SOCKET_THREADING is defined
        assert(0);
#endif /* NO_SOCKET_THREADING */
    }
    else
    {
        s->threaded = false;
#ifndef NO_SOCKET_THREADING
        s->head = -1;
        s->tail = -1;
        s->size = -1;
        s->mutex = NULL;
        s->notFull = NULL;
        s->notEmpty = NULL;
#endif /* NO_SOCKET_THREADING */
    }
}

// /**
// API       :: EXTERNAL_EXTERNAL_SocketInitUDP
// PURPOSE   :: This function creates a UDP socket. 
// PARAMETERS::
// + s : EXTERNAL_Socket* : Pointer to the socket
// + MYPORT : EXTERNAL_VarArray* : Pointer to the VarArray to send
// + blocking: bool : blocking or non-blocking
// + threaded: bool :
// RETURN    :: void
// **/
void EXTERNAL_SocketInitUDP(
    EXTERNAL_Socket *s,
    int MYPORT,
    bool blocking,
    bool threaded)
{
    struct sockaddr_in my_addr;
    EXTERNAL_SocketErrorType Err;
    int err;
    char errString[MAX_STRING_LENGTH];
    s->isOpen = false;
    s->error = false;
    s->socketFd = -1;
    s->blocking = blocking;

#ifdef _WIN32 
    // Initialize the windows socket library
    if (WSAStartup(MAKEWORD(1, 1), &gWsaData) != 0)
    {
        ERROR_ReportError("Error loading winsock");
    }
#endif
    s->socketFd= socket(AF_INET,SOCK_DGRAM,0);
#ifdef _WIN32
    if (s->socketFd == INVALID_SOCKET)
    {
        ERROR_ReportError("Error creating socket");
    }
#else /* unix/linux */
    if (s->socketFd == -1)
    {
        ERROR_ReportError("Error creating socket");
    }
#endif
    my_addr.sin_family=AF_INET;
    my_addr.sin_port=htons(MYPORT);
    //my_addr.sin_addr.s_addr=inet_addr("192.168.0.1");
    my_addr.sin_addr.s_addr=INADDR_ANY;
    memset (my_addr.sin_zero,'\0',sizeof my_addr.sin_zero);
    // The ::bind disambiguates it for the MSVC10 compiler
    // due to confusion between the socket bind defined in
    // the global namespace (and used here) and bind template
    // defined in the std namespace.
    err = ::bind( s->socketFd, (struct sockaddr*)&my_addr,sizeof my_addr);
#ifdef _WIN32
    if (err == SOCKET_ERROR)
    {
        sprintf(errString, "Could not bind socket, err = %d",
                            WSAGetLastError());
        ERROR_ReportWarning(errString);
        ERROR_ReportError("Error binding for socket connection");
    }
#else /* unix/linux */
    if (err == -1)
    {
        ERROR_ReportErrorArgs("Error binding for socket connection, err = %s",
            strerror(errno));
    }
#endif
    s->isOpen = true;
    if (!s->blocking)
    {
        Err = SetNonBlocking(s->socketFd);
        /*if (Err != EXTERNAL_NoSocketError)
        {
            return Err;
        }*/
    }
}

bool EXTERNAL_SocketValid(EXTERNAL_Socket *socket)
{
    if (socket->socketFd > -1 && socket->isOpen
        && !socket->error)
    {
        return true;
    }
    else
    {
        return false;
    }
}

EXTERNAL_SocketErrorType EXTERNAL_SocketListen(
    EXTERNAL_Socket *listenSocket,
    int port,
    EXTERNAL_Socket *connectSocket)
{
    EXTERNAL_SocketErrorType err;

    // Set up the listening socket
    err = EXTERNAL_SocketInitListen(listenSocket, port);
    if (err != EXTERNAL_NoSocketError)
    {
        return err;
    }

    // Accept a new connection
    err = EXTERNAL_SocketAccept(listenSocket, connectSocket);
    if (err != EXTERNAL_NoSocketError)
    {
        return err;
    }

    return EXTERNAL_NoSocketError;
}

EXTERNAL_SocketErrorType EXTERNAL_SocketInitListen(
    EXTERNAL_Socket* listenSocket,
    int port,
    bool threaded,
    bool reuseAddr)
{
    struct sockaddr_in listenAddr;
    int err;

    // Initialize the socket
    EXTERNAL_SocketInit(listenSocket, threaded);

    // If the listenSocket is an invalid socket then create a new socket
#ifdef _WIN32
    // Initialize the windows socket library
    if (gNumSockets == 0)
    {
        if (WSAStartup(MAKEWORD(1, 1), &gWsaData) != 0)
        {
            ERROR_ReportError("Error loading winsock");
        }
    }
#endif

    // Create the socket
    listenSocket->socketFd = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (listenSocket->socketFd == INVALID_SOCKET)
    {
        ERROR_ReportError("Error creating listening socket");
    }
#else /* unix/linux */
    if (listenSocket->socketFd == -1)
    {
        ERROR_ReportError("Error creating listening socket");
    }
#endif

    // Enable address reuse
    if (reuseAddr)
    {
        int on = 1;
        err = setsockopt(
            listenSocket->socketFd,
            SOL_SOCKET,
            SO_REUSEADDR,
            (char*) &on,
            sizeof(on));
        if (err == -1)
        {
            ERROR_ReportError("Could not create socket with SO_REUSEADDR");
        }
    }

    // Bind to the port
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_addr.s_addr = INADDR_ANY;
    listenAddr.sin_port = htons((unsigned short) port);
    memset(&listenAddr.sin_zero, 0, 8);

    // REUSEADDR socket option allows rebind despite the socket being in
    // TIME_WAIT. Note, that on windows this option will permit several programs to
    // simultaneously bind, and the first of the running list gets the data (until
    // it exits). On unix however only the first application will bind while others
    // will report being unable to bind.
    int optval = 1;
    setsockopt (listenSocket->socketFd, SOL_SOCKET, SO_REUSEADDR,
        (const char *) &optval, sizeof(optval));

    // The ::bind disambiguates it for the MSVC10 compiler
    // due to confusion between the socket bind defined in
    // the global namespace (and used here) and bind template
    // defined in the std namespace.
    err = ::bind(listenSocket->socketFd,
                (sockaddr*) &listenAddr,
                sizeof(sockaddr));
#ifdef _WIN32
    if (err == SOCKET_ERROR)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Could not bind listening socket, err = %d",
                            WSAGetLastError());
        ERROR_ReportWarning(errString);
        return EXTERNAL_SocketError;
    }
#else /* unix/linux */
    if (err == -1)
    {
        ERROR_ReportWarningArgs("Could not bind socket, err = %s",
            strerror(errno));
        return EXTERNAL_SocketError;
    }
#endif

    // The listen socket has been successfully opened
    listenSocket->isOpen = true;

    // Listen for a connection
    err = listen(listenSocket->socketFd, 1);
#ifdef _WIN32
    if (err == SOCKET_ERROR)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Could not listen, err = %d",
                           WSAGetLastError());
        ERROR_ReportWarning(errString);
        return EXTERNAL_SocketError;
    }
#else /* unix/linux */
    if (err == -1)
    {
        ERROR_ReportWarningArgs("Could not bind socket, err = %s",
            strerror(errno));
        return EXTERNAL_SocketError;
    }
#endif

    return EXTERNAL_NoSocketError;
}

#ifndef NO_SOCKET_THREADING
void* EXTERNAL_SocketReceiverThread(void *voidSocket)
{
    EXTERNAL_Socket *s = (EXTERNAL_Socket*) voidSocket;
    char buf[1000];
    char* currBuf;
    int size;
    int bytesMoved;

    // Continue receiving while the socket is open
    while (s->isOpen && !s->error)
    {
        // Receive up to 1000 bytes
        size = recv(
            s->socketFd,
            buf,
            1000,
            0);
#ifdef _WIN32
        if (size == SOCKET_ERROR || size == 0)
#else /* unix/linux */
        if (size == -1 || size == 0)
#endif
        {
            s->error = true;
        }
        else
        {
            // Move data to the buffer.  Will be done in multiple steps if
            // the buffer is nearly full or it reaches the end of the
            // circular buffer.
            currBuf = buf;
            while (size > 0)
            {
                pthread_mutex_lock(s->mutex);

                // Wait until the buffer is not full
                assert(s->size <= THREADED_BUFFER_SIZE);
                while (s->size == THREADED_BUFFER_SIZE)
                {
                    pthread_cond_wait(s->notFull, s->mutex);
                }

                // Determine how many bytes we can move to the buffer
                if (s->tail < s->head)
                {
                    bytesMoved = s->head - s->tail;
                }
                else
                {
                    bytesMoved = THREADED_BUFFER_SIZE - s->tail;
                }
                if (size < bytesMoved)
                {
                    bytesMoved = size;
                }

#if DEBUG_THREADED_SOCKET
                printf("WRITE tail %d, writing %d", s->tail, bytesMoved);
#endif /* DEBUG_THREADED_SOCKET */

                // Now move the bytes
                memcpy((char*) s->buffer + s->tail, currBuf, bytesMoved);
                currBuf += bytesMoved;
                size -= bytesMoved;
                s->size += bytesMoved;
                s->tail += bytesMoved;
                if (s->tail == THREADED_BUFFER_SIZE)
                {
                    s->tail = 0;
                }

#if DEBUG_THREADED_SOCKET
                printf(", new tail %d, size %d\n", s->tail, s->size);
#endif /* DEBUG_THREADED_SOCKET */

                pthread_mutex_unlock(s->mutex);
                pthread_cond_signal(s->notEmpty);
            }
        }
    }

    return NULL;
}

void* EXTERNAL_SocketSenderThread(void *voidSocket)
{
    EXTERNAL_Socket *s = (EXTERNAL_Socket*) voidSocket;
    int size;

#if DEBUG_THREADED_SOCKET
    printf("sender STARTING\n");
#endif /* DEBUG_THREADED_SOCKET */

    // Continue receiving while the socket is open
    pthread_mutex_lock(s->sendMutex);
    while (s->isOpen && !s->error)
    {
        if (s->sendSize > 0)
        {
            // Send up to end of circular array
            size = MIN(s->sendSize, THREADED_BUFFER_SIZE - s->sendHead);

#if DEBUG_THREADED_SOCKET
            printf("sender SEND %d bytes\n", size);
#endif /* DEBUG_THREADED_SOCKET */

            size = send(
                s->socketFd,
                (char*) s->sendBuffer + s->sendHead,
                size,
                0);
#ifdef _WIN32
            if (size == SOCKET_ERROR || size == 0)
#else /* unix/linux */
            if (size == -1 || size == 0)
#endif
            {
                s->error = true;
            }
            else
            {
                // Move data from the buffer
                s->sendHead += size;
                if (s->sendHead == THREADED_BUFFER_SIZE)
                {
                    s->sendHead = 0;
                }
                s->sendSize -= size;

                // Signal that it is not full
                pthread_cond_signal(s->sendNotFull);
            }
        }
        else
        {
            // There is no data to send, wait for data to arrive
#if DEBUG_THREADED_SOCKET
            printf("sender WAITING\n");
#endif /* DEBUG_THREADED_SOCKET */

            pthread_cond_wait(s->sendNotEmpty, s->sendMutex);
#if DEBUG_THREADED_SOCKET
            printf("sender WAITED\n");
#endif /* DEBUG_THREADED_SOCKET */
        }
    }
    pthread_mutex_unlock(s->sendMutex);

#if DEBUG_THREADED_SOCKET
    printf("sender ENDING\n");
#endif /* DEBUG_THREADED_SOCKET */

    return NULL;
}
#endif /* NO_SOCKET_THREADING */

EXTERNAL_SocketErrorType EXTERNAL_SocketAccept(
    EXTERNAL_Socket* listenSocket,
    EXTERNAL_Socket* connectSocket)
{
    EXTERNAL_SocketErrorType cesErr;

    // Make sure the listening socket is valid
    if (!EXTERNAL_SocketValid(listenSocket))
    {
        return EXTERNAL_InvalidSocket;
    }

    // Accept the connection
    connectSocket->socketFd = accept(
        listenSocket->socketFd,
        NULL,
        NULL);
#ifdef _WIN32
    if (connectSocket->socketFd == SOCKET_ERROR)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Could not accept, err = %d",
                           WSAGetLastError());
        ERROR_ReportWarning(errString);
        return EXTERNAL_SocketError;
    }
#else /* unix/linux */
    if (connectSocket->socketFd == -1)
    {
        ERROR_ReportWarningArgs("Could not accept, err = %s",
            strerror(errno));
        return EXTERNAL_SocketError;
    }
#endif

    // The connect socket has been successfully opened
    connectSocket->isOpen = true;

    // If the connected socket is configured to be non-blocking then set
    // it to non-blocking
    if (!connectSocket->blocking)
    {
        cesErr = SetNonBlocking(connectSocket->socketFd);
        if (cesErr != EXTERNAL_NoSocketError)
        {
            return cesErr;
        }
    }

    // If the connected socket is configured to be threaded then start the
    // receiver thread
#ifndef NO_SOCKET_THREADING
    if (connectSocket->threaded)
    {
        pthread_create(
            &connectSocket->reciever,
            NULL,
            EXTERNAL_SocketReceiverThread,
            connectSocket);

        pthread_create(
            &connectSocket->sender,
            NULL,
            EXTERNAL_SocketSenderThread,
            connectSocket);
    }
#endif /* NO_SOCKET_THREADING */

    return EXTERNAL_NoSocketError;
}

EXTERNAL_SocketErrorType EXTERNAL_SocketDataAvailable(
    EXTERNAL_Socket *s,
    bool* available)
{
    int err;
    fd_set readFdSet;
    timeval t;

    // If the socket is invalid (not open) then no dat ais available
    if (!EXTERNAL_SocketValid(s))
    {
        *available = false;
        return EXTERNAL_NoSocketError;
    }

    // If threaded, then data is available if there is something in the
    // input buffer (size > 0).
    // If not threaded then use select to determine if data can be read.
    if (s->threaded)
    {
#ifndef NO_SOCKET_THREADING
        *available = s->size > 0;
#endif /* NO_SOCKET_THREADING */
    }
    else
    {
        t.tv_sec = 0;
        t.tv_usec = 0;

        // Set up the read set and select for data available
        FD_ZERO(&readFdSet);
        assert(s->socketFd >= 0);
        FD_SET((unsigned int) s->socketFd, &readFdSet);
        err = select((int)(s->socketFd + 1), &readFdSet, NULL, NULL, &t);
        if (err == -1)
        {
#ifndef _WIN32
            // In unix EINTR is interrupted system call.  This means that
            // the select call did not finish waiting which is not an error,
            // so set available to false and return no error.
            if (errno == EINTR)
            {
                *available = false;
                return EXTERNAL_NoSocketError;
            }
#endif
            ERROR_ReportWarningArgs("Error calling select, err = %s",
                strerror(errno));
            return EXTERNAL_SocketError;
        }

        // Check if the data is available
        *available = (bool) (FD_ISSET((unsigned int) s->socketFd, &readFdSet) != 0);
    }
    return EXTERNAL_NoSocketError;
}

EXTERNAL_SocketErrorType EXTERNAL_SocketConnect(
    EXTERNAL_Socket *s,
    char *address,
    int port,
    int maxAttempts)
{
    struct sockaddr_in sendAddr;
    int err;
    EXTERNAL_SocketErrorType cesErr;
    bool connected;
    Int32 socketErr;
    fd_set writeFdSet;
    char opt[DEFAULT_OPT_SIZE];
#ifdef _WIN32
    int optSize;
#else
    socklen_t optSize;
#endif
    char errString[MAX_STRING_LENGTH];
    int numFailures;

    // Set up socket
    s->socketFd = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (s->socketFd == INVALID_SOCKET)
#else /* unix/linux */
    if (s->socketFd == -1)
#endif
    {
        ERROR_ReportWarning("Error creating connect socket");
        return EXTERNAL_SocketError;
    }

    // Create address
    sendAddr.sin_family = AF_INET;
    sendAddr.sin_addr.s_addr = inet_addr(address);
    memset(&sendAddr.sin_zero, 0, 8);
    sendAddr.sin_port = htons((unsigned short) port);

    numFailures = 0;
    connected = false;
    while (!connected && (numFailures < maxAttempts))
    {
        // Attempt to connect
        err = connect(s->socketFd,
                      (sockaddr*) &sendAddr,
                      sizeof(sockaddr));
#ifdef _WIN32
        if ((err == SOCKET_ERROR) && (WSAGetLastError() == WSAEINPROGRESS))
#else /* unix/linux */
        if ((err == -1) && (errno == EINPROGRESS))
#endif
        {
            // The connect is in progress.  Select for it to finished.
            FD_ZERO(&writeFdSet);
            assert(s->socketFd >= 0);
            FD_SET((unsigned int) s->socketFd, &writeFdSet);
            err = select((int)(s->socketFd + 1), NULL, &writeFdSet, NULL,
                         NULL);
            if (err == -1)
            {
                ERROR_ReportWarningArgs("Error calling select, err = %s",
                    strerror(errno));
                return EXTERNAL_SocketError;
            }

            // Check if completed successfully
            optSize = DEFAULT_OPT_SIZE;
            err = getsockopt(s->socketFd, SOL_SOCKET, SO_ERROR, opt, &optSize);
            if (err == -1)
            {
                ERROR_ReportWarningArgs("Error calling getsockopt, err = %s",
                    strerror(errno));
                return EXTERNAL_SocketError;
            }

            // Check error
            if (optSize == 4)
            {
                memcpy(&socketErr, opt, sizeof(Int32));
                if (socketErr != 0)
                {
                    // Continue trying to connect to a different port
                }
                else
                {
                    // success!
                    connected = true;
                }
            }
            else
            {
                sprintf(errString, "Unknown error size \"%d\"", optSize);
                ERROR_ReportWarning(errString);
                return EXTERNAL_SocketError;
            }
        }
#ifdef _WIN32
        else if (err == SOCKET_ERROR)
        {
            // Continue trying to connect to a different port
            err = WSAGetLastError();
        }
#else /* unix/linux */
        else if (err == -1)
        {
            // Continue trying to connect to a different port
            err = errno;
        }
#endif
        else
        {
            // success!
            connected = true;
        }

        if (!connected)
        {
            numFailures++;
            printf("Socket connect failure %d of %d, err = %d\n",
                numFailures, maxAttempts, err);
            EXTERNAL_Sleep(3 * SECOND);
        }
    }

    if (!connected)
    {
        printf("Max attempts, giving up...\n");
        return EXTERNAL_SocketError;
    }

    if (!s->blocking)
    {
        cesErr = SetNonBlocking(s->socketFd);
        if (cesErr != EXTERNAL_NoSocketError)
        {
            return cesErr;
        }
    }

    // The socket has successfully connected
    s->isOpen = true;

    // If the connected socket is configured to be threaded then start the
    // receiver thread
#ifndef NO_SOCKET_THREADING
    if (s->threaded)
    {
        pthread_create(
            &s->reciever,
            NULL,
            EXTERNAL_SocketReceiverThread,
            s);

        pthread_create(
            &s->sender,
            NULL,
            EXTERNAL_SocketSenderThread,
            s);
    }
#endif /* NO_SOCKET_THREADING*/


    return EXTERNAL_NoSocketError;
}

EXTERNAL_SocketErrorType EXTERNAL_SocketSend(
    EXTERNAL_Socket *s,
    const char *data,
    unsigned int size,
    bool block)
{
    int sentSize;
    const char *remainingData;
    int remainingSize;

#ifdef _WIN32
    char err[MAX_STRING_LENGTH];
#endif

    if (!EXTERNAL_SocketValid(s))
    {
        ERROR_ReportWarning("Invalid socket in EXTERNAL_SocketSend");
        return EXTERNAL_InvalidSocket;
    }

    if (s->threaded)
    {
#ifndef NO_SOCKET_THREADING
        pthread_mutex_lock(s->sendMutex);

        // Move data into sender queue.  Move up until end of array,
        // or until array is full.
        while (size > 0)
        {
            int toMove = MIN(MIN(
                (int) size,
                THREADED_BUFFER_SIZE - s->sendTail),
                THREADED_BUFFER_SIZE - s->sendSize);
            
            memcpy((char*) s->sendBuffer + s->sendTail, data, toMove);
            data += toMove;
            size -= toMove;
            
            s->sendTail += toMove;
            if (s->sendTail == THREADED_BUFFER_SIZE)
            {
                s->sendTail = 0;
            }
            s->sendSize += toMove;

            if (s->sendSize == THREADED_BUFFER_SIZE)
            {
                // If full, wait until it is not full
                pthread_cond_wait(s->sendNotFull, s->sendMutex);
            }
        }

        // Wake up sender thread
        pthread_cond_signal(s->sendNotEmpty);
        pthread_mutex_unlock(s->sendMutex);
#endif /* NO_SOCKET_THREADING*/
    }
    else
    {
        // Send data non-threaded
        remainingData = data;
        remainingSize = size;
        while (remainingSize > 0)
        {
            sentSize = send(s->socketFd, remainingData, remainingSize, 0);
#ifdef _WIN32
            if (sentSize == SOCKET_ERROR)
            {
                if (WSAGetLastError() == WSAEWOULDBLOCK)
                {
                    if (block || remainingSize != (int)size)
                    {
                        EXTERNAL_Sleep(1 * MICRO_SECOND);
                        continue;
                    }
                    else
                    {
                        return EXTERNAL_DataNotSent;
                    }
                }

                sprintf(err, "Could not send err = %d",
                              WSAGetLastError());
                ERROR_ReportWarning(err);
                return EXTERNAL_SocketError;
            }
#else /* unix/linux */
            if (sentSize == -1)
            {
                // This send would result in a block.
                // Return EXTERNAL_DataNotSent.
                if (errno == EAGAIN)
                {
                    if (block || remainingSize != (int)size)
                    {
                        EXTERNAL_Sleep(1 * MICRO_SECOND);
                        continue;
                    }
                    else
                    {
                        return EXTERNAL_DataNotSent;
                    }
                }

                ERROR_ReportWarningArgs("Could not send, err = %s",
                    strerror(errno));
                return EXTERNAL_SocketError;
            }
#endif
            else
            {
                remainingData += sentSize;
                remainingSize -= sentSize;
            }
        }
    }

    return EXTERNAL_NoSocketError;
}

EXTERNAL_SocketErrorType EXTERNAL_SocketSend(
    EXTERNAL_Socket *s,
    EXTERNAL_VarArray *data,
    bool block)
{
    // Send data
    return EXTERNAL_SocketSend(
        s,
        data->data,
        data->size,
        block);
}

EXTERNAL_SocketErrorType EXTERNAL_SocketSendTo(
    int sockFd,
    char *data,
    unsigned int size,
    unsigned int port,
    unsigned int destIp,
    bool block) 
{  
    int sentSize, length;
    struct sockaddr_in their_addr;
#ifdef _WIN32
    char err[MAX_STRING_LENGTH];
#endif

    // Send data
    their_addr.sin_family=AF_INET;
    their_addr.sin_port=htons(port);
    their_addr.sin_addr.s_addr=htonl(destIp);
    memset(their_addr.sin_zero,'\0',sizeof their_addr.sin_zero);
    length=sizeof their_addr;
    sentSize = sendto(sockFd, data, size, 0,(struct sockaddr*)&their_addr,length);
#ifdef _WIN32
        if (sentSize == SOCKET_ERROR)
        {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                if (block)
                {
                    EXTERNAL_Sleep(1 * MICRO_SECOND);
                    //continue;
                }
                else
                {
                    return EXTERNAL_DataNotSent;
                }
            }

            sprintf(err, "Could not send err = %d",
                          WSAGetLastError());
            ERROR_ReportWarning(err);
            return EXTERNAL_SocketError;
        }
#else /* unix/linux */
        if (sentSize == -1)
        {
            // This send would result in a block.
            // Return EXTERNAL_DataNotSent.
            if (errno == EAGAIN)
            {
                if (block)
                {
                    EXTERNAL_Sleep(1 * MICRO_SECOND);
                    //continue;
                }
                else
                {
                    return EXTERNAL_DataNotSent;
                }
            }

            ERROR_ReportWarningArgs("Could not send, err = %s",
                strerror(errno));
            return EXTERNAL_SocketError;
        }
#endif

    return EXTERNAL_NoSocketError;
}

EXTERNAL_SocketErrorType EXTERNAL_SocketRecv(
    EXTERNAL_Socket *s,
    char *data,
    unsigned int size,
    unsigned int *receiveSize,
    bool block)
{
    unsigned int totalReceivedSize;
    int receivedSize;

#ifdef  _WIN32
    char errString[MAX_STRING_LENGTH];
#endif

    // If threaded
    if (s->threaded)
    {
#ifndef NO_SOCKET_THREADING
        // Check for an error from the receiving thread.  If so
        // return error.
        if (s->error)
        {
            *receiveSize = 0;
            return EXTERNAL_SocketError;
        }

        // Now try getting data
        pthread_mutex_lock(s->mutex);

        // If there is not enough data in the buffer
        if ((unsigned int) s->size < size)
        {
            if (block)
            {
                // If blocking, wait until there is enough
                while ((unsigned int) s->size < size)
                {
                    pthread_cond_wait(s->notEmpty, s->mutex);
                }
            }
            else
            {
                // If non-blocking, return nothing.  NOTE: this could
                // cause loss of data.
                pthread_mutex_unlock(s->mutex);
                *receiveSize = 0;
                return EXTERNAL_NoSocketError;
            }
        }

#if DEBUG_THREADED_SOCKET
        printf("READ head %d, reading %d", s->head, size);
#endif /* DEBUG_THREADED_SOCKET */

        // Read the data, checking to see if we have to circle around the
        // circular array
        if (s->head + size > THREADED_BUFFER_SIZE)
        {
            memcpy(data, (char*) s->buffer + s->head, THREADED_BUFFER_SIZE - s->head);
            memcpy(data + THREADED_BUFFER_SIZE - s->head, (char*) s->buffer, size - (THREADED_BUFFER_SIZE - s->head));

            assert(s->size >= 0);
            s->size -= size;
            s->head = size - (THREADED_BUFFER_SIZE - s->head);
        }
        else
        {
            memcpy(data, (char*) s->buffer + s->head, size);

            s->size -= size;
            s->head += size;
            if (s->head == THREADED_BUFFER_SIZE)
            {
                s->head = 0;
            }
        }

#if DEBUG_THREADED_SOCKET
        printf(", new head %d, size %d\n", s->head, s->size);
#endif /* DEBUG_THREADED_SOCKET */

        pthread_mutex_unlock(s->mutex);
        if (s->size < 1000000 * 0.9)
        {
            pthread_cond_signal(s->notFull);
        }
        *receiveSize = size;
#endif /* NO_SOCKET_THREADING*/
        return EXTERNAL_NoSocketError;
    }
    else
    {
        if (!EXTERNAL_SocketValid(s))
        {
            ERROR_ReportWarning("Invalid socket in EXTERNAL_SocketRecv");
            return EXTERNAL_InvalidSocket;
        }

        totalReceivedSize = 0;
        while (totalReceivedSize < size)
        {
            receivedSize = recv(s->socketFd,
                                data + totalReceivedSize,
                                size - totalReceivedSize,
                                0);
            if (receivedSize == 0)
            {
                // EOF, Return EXTERNAL_SocketError
                *receiveSize = totalReceivedSize;
                return EXTERNAL_SocketError;
            }
#ifdef _WIN32
            else if (receivedSize == SOCKET_ERROR)
            {
                if (WSAGetLastError() == WSAEWOULDBLOCK)
#else /* unix/linux*/
            else if (receivedSize == -1)
            {
                if (errno == EAGAIN)
#endif
                {
                    // No data is available to be read.  This is not an error,
                    // return EXTERNAL_NoSocketError if non blocking.  Else sleep and try to
                    // read again.
                    if (block)
                    {
                        EXTERNAL_Sleep(1 * MICRO_SECOND);
                    }
                    else
                    {
                        *receiveSize = totalReceivedSize;
                        return EXTERNAL_NoSocketError;
                    }
                }
                else
                {
                    // Some error occured, return EXTERNAL_SocketError
#ifdef _WIN32
                    ERROR_ReportWarningArgs("Error receiving data, err = %d",
                        WSAGetLastError());
#else /* unix/linux */
                    ERROR_ReportWarningArgs("Error receiving data, err = %s",
                        strerror(errno));
#endif
                    return EXTERNAL_SocketError;
                }
            }
            else
            {
                totalReceivedSize += receivedSize;
            }
        }

        *receiveSize = totalReceivedSize;
        return EXTERNAL_NoSocketError;
    }
}

EXTERNAL_SocketErrorType EXTERNAL_SocketRecvFrom(
    EXTERNAL_Socket *s,
    char *data,
    unsigned int size,
    unsigned int *receiveSize,
    unsigned int *ip,
    unsigned int *port,
    bool block)
{
    unsigned int totalReceivedSize;
    int receivedSize;

#ifdef  _WIN32
    char errString[MAX_STRING_LENGTH];
#endif

    // If threaded
#ifndef QT_CORE_LIB
    ERROR_Assert(!s->threaded, "EXTERNAL_SocketRecvFrom may not be used with threaded sockets");
#else
    if (s->threaded)
    {
        ERROR_ReportError("EXTERNAL_SocketRecvFrom may not be used with threaded sockets");
        return EXTERNAL_InvalidSocket;
    }
#endif

    if (!EXTERNAL_SocketValid(s))
    {
        ERROR_ReportWarning("Invalid socket in EXTERNAL_SocketRecv");
        return EXTERNAL_InvalidSocket;
    }

    totalReceivedSize = 0;
    while (totalReceivedSize == 0)
    {
        sockaddr_in from;
        int fromLen = sizeof(struct sockaddr);
        receivedSize = recvfrom(s->socketFd,
                            data + totalReceivedSize,
                            size - totalReceivedSize,
                            0,
                            (sockaddr*) &from,
                            (socklen_t*) &fromLen);
        if (receivedSize == 0)
        {
            // EOF, Return EXTERNAL_SocketError
            *receiveSize = totalReceivedSize;
            return EXTERNAL_SocketError;
        }
#ifdef _WIN32
        else if (receivedSize == SOCKET_ERROR)
        {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
#else /* unix/linux*/
        else if (receivedSize == -1)
        {
            if (errno == EAGAIN)
#endif
            {
                // No data is available to be read.  This is not an error,
                // return EXTERNAL_NoSocketError if non blocking.  Else sleep and try to
                // read again.
                if (block)
                {
                    EXTERNAL_Sleep(1 * MICRO_SECOND);
                }
                else
                {
                    *receiveSize = totalReceivedSize;
                    return EXTERNAL_NoSocketError;
                }
            }
            else
            {
                // Some error occured, return EXTERNAL_SocketError
#ifdef _WIN32
                ERROR_ReportWarningArgs("Error receiving data, err = %d",
                    WSAGetLastError());
#else /* unix/linux */
                ERROR_ReportWarningArgs("Error receiving data, err = %s",
                    strerror(errno));
#endif
                return EXTERNAL_SocketError;
            }
        }
        else
        {
            totalReceivedSize += receivedSize;
            *ip = ntohl(from.sin_addr.s_addr);
            *port = ntohs(from.sin_port);
        }
    }

    *receiveSize = totalReceivedSize;
    return EXTERNAL_NoSocketError;
}

EXTERNAL_SocketErrorType EXTERNAL_SocketRecvLine(
    EXTERNAL_Socket *s,
    std::string* data)
{
    data->clear();

    if (s->threaded)
    {
#ifndef NO_SOCKET_THREADING
        // Not implemented yet
        assert(0);
#endif /* NO_SOCKET_THREADING */
    }
    else
    {
        // Note: This code is borrowed from GUI_ReceiveCommand
        // The GUI socket code should be re-written to use an EXTERNAL_Socket
        // At that time GUI_ReceiveCommand can be re-implemented using this
        // function
        const int SOCK_MAX_BUFFER_SIZE = 4098;
        char buffer[SOCK_MAX_BUFFER_SIZE];
        char* lineEnd;
        ssize_t lengthRead;
        size_t  lengthProcessed;
        bool foundNewline = false;

        while (!foundNewline)
        {
            lengthRead = recv(s->socketFd,           // Connected socket
                              buffer,                // Receive buffer
                              SOCK_MAX_BUFFER_SIZE,  // Size of receive buffer
                              MSG_PEEK);             // Flags
#ifdef _WIN32
            if (lengthRead == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
#else
            if (lengthRead < 0 && (errno == EINTR || errno == EAGAIN))
#endif /* _WIN32 */
            {
                // Try it again.
                continue;
            }
            else if (lengthRead <= 0)
            {
                // Either the socket was closed prematurely (maybe because
                // the other side crashed) or an unrecognized error occurred.
                return EXTERNAL_SocketError;
            }

            lineEnd = (char*) memchr(buffer, '\n', lengthRead);
            if (lineEnd != NULL)
            {
                foundNewline = true;
                lengthProcessed = lineEnd - buffer + 1;

                // append everything except the final newline
                data->append(buffer, lineEnd - buffer);
            }
            else
            {
                lengthProcessed = lengthRead;

                // append everything read so far
                data->append(buffer, lengthRead);
            }

            // flush processed data from the TCP input buffer
            recv(s->socketFd, buffer, lengthProcessed, 0);
        }
    }

    return EXTERNAL_NoSocketError;
}

EXTERNAL_SocketErrorType EXTERNAL_SocketClose(EXTERNAL_Socket *s)
{
    // Set isOpen to false and socketFd to -1 before closing socket.  This
    // way all socket will become invalid before they are closed.
    s->isOpen = false;

#ifndef NO_SOCKET_THREADING
    if (s->threaded)
    {
        // Wake up sending threads
        pthread_mutex_lock(s->sendMutex);
        pthread_cond_signal(s->sendNotEmpty);
        pthread_mutex_unlock(s->sendMutex);

        // If threaded, wait for thread to finish
        pthread_join(s->reciever, NULL);
        pthread_join(s->sender, NULL);
    }
#endif /* NO_SOCKET_THREADING*/

    // Close the socket if it is open
    if (s->socketFd >= 0)
    {
        int fd = (int)s->socketFd;
        s->socketFd = -1;

#ifdef _WIN32
        gNumSockets--;

        shutdown(fd, 2); // stop sending and receiving
        closesocket(fd);

        if (gNumSockets == 0)
        {
            WSACleanup();
        }
#else /* unix/linux */
        shutdown(fd, SHUT_RDWR);
        close(fd);
#endif
    }

    return EXTERNAL_NoSocketError;
}
