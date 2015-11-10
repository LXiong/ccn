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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "app_util.h"
#include "partition.h"
#include "network_ip.h"
#include "qualnet_error.h"
#include "transport_tcp.h"
#include "multimedia_h323.h"
#include "app_voip.h"
#include "multimedia_h225_ras.h"
#include "application.h"
// Pseudo traffic sender layer
#include "app_trafficSender.h"
#define DEBUG 0
#define H323_DEBUG_DETAIL 0
#define DEBUG_TCP 0

// NAME:        H323CreateCircularBufferInVector
// PURPOSE:     Create CircularBuffer object and push it to vector
// PARAMETERS:  node:   node which is receiving H323 call signal
//      connectionId:   Connection Id
// RETURN:      None.
// ASSUMPTION:  None.

static
void H323CreateCircularBufferInVector(
    Node* node,
    H323Data* h323)
{
    int index;
    //Type casted connectionId to unsigned short bcz CircularBuffer takes
    //param connectionId as unsigned short
    unsigned short connectionId = (unsigned short)h323->connectionId;
    int vectorSize = (int)(h323->msgBufferVector->size());
    CircularBuffer* tempMsgBuffer;

    for (index = 0; (index < vectorSize); index++)
    {
        tempMsgBuffer = h323->msgBufferVector->at(index);
        if (connectionId == tempMsgBuffer->getIndex())
        {
            //connection Id Already exist
            return;
        }
    }

    CircularBuffer* msgBuffer
                = new CircularBuffer(connectionId,H323_BUFFER_SIZE);

    h323->msgBufferVector->push_back(msgBuffer);
}


// NAME:        H323GetCircularBufferFromVector
// PURPOSE:     Get CircularBuffer object according to Connection Id
// PARAMETERS:  node:   node which is receiving H323 call signal
//      connectionId:   Connection Id
// RETURN:      A pointer to CircularBuffer.
// ASSUMPTION:  None.


static
CircularBuffer* H323GetCircularBufferFromVector(
    Node* node,
    unsigned short connectionId,
    H323Data* h323,
    Int32& indexInVector )
{
    int index;
    int vectorSize = (int)(h323->msgBufferVector->size());
    CircularBuffer* tempMsgBuffer;

    for (index = 0; (index < vectorSize); index++)
    {
        tempMsgBuffer = h323->msgBufferVector->at(index);
        if (connectionId == tempMsgBuffer->getIndex())
        {
            //Return Circular Buffer according to connection Id.
            indexInVector = index;
            return tempMsgBuffer;
        }
    }
    return NULL;
}


// NAME:        H323DeleteCircularBuffer
// PURPOSE:     Delete CircularBuffer object according to Connection ID
// PARAMETERS:  node:   node which is receiving H323 call signal
//      connectionId:   Connection Id
// RETURN:      none.
// ASSUMPTION:  None.

static
void H323DeleteCircularBuffer(Node* node,
                              H323Data* h323,
                              unsigned short connectionId)
{
    CircularBuffer* tempMsgBuffer = NULL;
    Int32 indexInVector = -1;

    tempMsgBuffer = H323GetCircularBufferFromVector(
                                            node,
                                            connectionId,
                                            h323,
                                            indexInVector);

    if ((indexInVector != -1) && (tempMsgBuffer != NULL))
    {
        delete tempMsgBuffer;
        h323->msgBufferVector->erase(h323->msgBufferVector->begin()
                                        + indexInVector);
    }
}


// NAME:        H323DeleteCircularBufferVector
// PURPOSE:     delete the Vector with all its elements.
// PARAMETERS:  node:   node which is receiving H323 call signal
// RETURN:      none.
// ASSUMPTION:  None.

static
void H323DeleteCircularBufferVector(Node* node)
{
    int index;
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    //Typecasted Vector size to an int value.
    int vectorSize = (int)h323->msgBufferVector->size();
    CircularBuffer* tempMsgBuffer;

    for (index = 0; (index < vectorSize); index++)
    {
        tempMsgBuffer = h323->msgBufferVector->at(index);
        if (tempMsgBuffer)
        {
            delete tempMsgBuffer;
        }
    }
    h323->msgBufferVector->clear();
}


//
// NAME:        H323PutClocktype
//
// PURPOSE:     Set clocktype field in the message structure
//
// PARAMETERS:  ptr:    reference pointer to the message pointer
//              value:  clocktype value
//
// RETURN:      None
//
// ASSUMPTION:  None.
//
void H323PutClocktype(unsigned char** ptr, clocktype value)
{
    memcpy(*ptr, &value, sizeof(clocktype));
    *ptr += sizeof(clocktype);
}


//
// NAME:        H323PutInt
//
// PURPOSE:     Set int field in the message structure
//
// PARAMETERS:  ptr:    reference pointer to the message pointer
//              value:  int value
//
// RETURN:      None
//
// ASSUMPTION:  None.
//
void H323PutInt(unsigned char** ptr, unsigned int value)
{
    memcpy(*ptr, &value, sizeof(unsigned int));
    *ptr += sizeof(unsigned int);
}


//
// NAME:        H323PutShort
//
// PURPOSE:     Set unsigned short field in the message structure
//
// PARAMETERS:  ptr:    reference pointer to the message pointer
//              value:  short value
//
// RETURN:      None
//
// ASSUMPTION:  None.
//
void H323PutShort(unsigned char** ptr, unsigned short value)
{
    memcpy(*ptr, &value, sizeof(unsigned short));
    *ptr += sizeof(unsigned short);
}


//
// NAME:        H323PutChar
//
// PURPOSE:     Set char field in the message structure
//
// PARAMETERS:  ptr:    reference pointer to the message pointer
//              value:  char value
//
// RETURN:      None
//
// ASSUMPTION:  None.
//
void H323PutChar(unsigned char** ptr, unsigned char value)
{
    memcpy(*ptr, &value, sizeof(unsigned char));
    *ptr += sizeof(unsigned char);
}


//
// NAME:        H323PutString
//
// PURPOSE:     Set string field in the message structure
//
// PARAMETERS:  ptr:    reference pointer to the message pointer
//              string:  string value
//              maxSize: string size
//
// RETURN:      None
//
// ASSUMPTION:  None.
//
void H323PutString(unsigned char** ptr, const char* string, int maxSize)
{
    memcpy(*ptr, string, strlen(string));
    *ptr += maxSize;
}


//
// NAME:        H323PutTransAddr
//
// PURPOSE:     Set transport address in the message structure
//
// PARAMETERS:  ptr:        reference pointer to the message pointer
//              transAddr:  transport address
//
// RETURN:      None
//
// ASSUMPTION:  None.
//
void H323PutTransAddr(
    unsigned char** ptr,
    const H225TransportAddress* transAddr)
{
    H323PutInt(ptr, transAddr->ipAddr);
    H323PutShort(ptr, transAddr->port);
}


//
// NAME:        H323PutTransAddrItemwise
//
// PURPOSE:     Set transport address in the message structure itemwise
//
// PARAMETERS:  ptr:    reference pointer to the message pointer
//              ipAddr: transport IP address
//              port:   transport port
//
// RETURN:      None
//
// ASSUMPTION:  None.
//
void H323PutTransAddrItemwise(
    unsigned char** ptr,
    NodeAddress ipAddr,
    unsigned short port)
{
    H323PutInt(ptr, ipAddr);
    H323PutShort(ptr, port);
}


//
// NAME:        H323PutTransAddrItemwise
//
// PURPOSE:     Copy transport address into another structure
//
// PARAMETERS:  ptr:    reference pointer to the message pointer
//              ipAddr: transport IP address
//              port:   transport port
//
// RETURN:      None
//
// ASSUMPTION:  None.
//
static
void H323CopyTransAddrX(char* destPtr,
                        const char* srcPtr)
{
    memcpy(destPtr, srcPtr, SIZE_OF_TRANS_ADDR);
}

void H323CopyTransAddr(H225TransportAddress* destPtr,
                       const H225TransportAddress* srcPtr)
{
    H323CopyTransAddrX((char*) destPtr,(const char*) srcPtr);
}



//
// NAME:        H323IsHostInitiator
//
// PURPOSE:     Check whether node is in initiator mode
//
// PARAMETERS:  node:   Node for which state is required
//
// RETURN:      TRUE if initiator, otherwise FALSE
//
// ASSUMPTION:  None.
//
BOOL H323IsHostInitiator(Node* node, short localPort)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    if (h323->hostState == H323_INITIATOR)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//
// NAME:        H323IsHostReceiver
//
// PURPOSE:     Check whether node is in receiver mode
//
// PARAMETERS:  node:       Node for which state is required
//
// RETURN:      TRUE if receiver, otherwise FALSE
//
// ASSUMPTION:  None.
//
BOOL H323IsHostReceiver(Node* node, short localPort)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    if (h323->hostState == H323_RECEIVER)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//
// NAME:        Q931PduCreate
//
// PURPOSE:     Create Q931 PDU header
//
// PARAMETERS:  q931Pdu:    Q931 PDU header pointer
//              callRefFlag:    Call reference flag
//              callRefValue:   call reference value
//              msgType:        message type
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
void Q931PduCreate(Q931Message* q931Pdu,
              char callRefFlag,
              unsigned char callRefValue,
              unsigned int msgType)
{
    q931Pdu->protocolDiscriminator = Q931_PROTOCOL_DISCRIMINATOR;

    // 0000 reserved
    Q931MessageSetCallRefReserved(&(q931Pdu->callRefResvlen), 0);
    // 0001 assume one octet connection
    Q931MessageSetCallRefLength(&(q931Pdu->callRefResvlen), 1);
      // 0 for initiator, 1 for receiver
    Q931MessageSetCallRefFlag(&(q931Pdu->callRefFlgVal), callRefFlag);
    Q931MessageSetCallRefValue(&(q931Pdu->callRefFlgVal), callRefValue);

    // 0 reserved
    Q931MessageSetMsgFlag(&(q931Pdu->msgFlgType), 0);
    // message type
    Q931MessageSetMsgType(&(q931Pdu->msgFlgType), msgType);
}


//
// NAME:        H323SendH225SetupMessage
//
// PURPOSE:     Create and send Setup message from the call initiator to
//              the destination. Initiator sends this message when it is
//              trying to establish a VoIP connection with the receiver
//
// PARAMETERS:  node, node which is creating Setup message
//              Open result structure received from TCP
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
void H323SendH225SetupMessage(Node* node,
                              int connectionId)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    // A node only can send Setup message while it is in IDLE state. For
    // all other cases it is engaged by some other calls, and does not send
    // Setup message
    if (h323->callSignalState == H323_IDLE)
    {
        H225Setup* setupPacket;
        unsigned char* tempPtr = NULL;

        // get voip applicatino pointer for this connection ID
        VoipHostData* voip = VoipCallInitiatorGetDataForConnectionId(
                                node,
                                connectionId);

        ERROR_Assert(voip, "Invalid Call initiator\n");

        h323->callIdentifier = node->getNodeTime();

        if (DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            ctoa(node->getNodeTime(), clockStr);
            printf("Node %u sends Setup message at connection %d at %s\n",
                node->nodeId, connectionId, clockStr);
        }

        // Allocating space for the Setup packet
        setupPacket = (H225Setup *) MEM_malloc (SIZE_OF_H225_SETUP);

        memset(setupPacket, 0, SIZE_OF_H225_SETUP);

        tempPtr = (unsigned char*) setupPacket;

        Q931PduCreate((Q931Message*) setupPacket,
                      0,
                      (unsigned char) connectionId,
                      Q931_SETUP);

        // Fill the value in Setup packet
        tempPtr += sizeof(Q931Message);

        // H225_ProtocolID
        H323PutString(&tempPtr, H225_ProtocolID, IDENTIFIER_SIZE);

        // sourceInfo
        H323PutInt(&tempPtr, Terminal);

        // activeMC
        // this terminal is not connecting to any MCU
        H323PutInt(&tempPtr, FALSE);


        // conference ID is not considered now
        H323PutClocktype(&tempPtr, 0);

        // conferenceGoal
        H323PutInt(&tempPtr, FALSE);

        // callType
        // default setting of call type is point to point
        H323PutInt(&tempPtr, FALSE);

        // call signal transport address of initiator
        H323PutTransAddrItemwise(&tempPtr,
                                 voip->callSignalAddr,
                                 voip->callSignalPort);

        // callIdentifier
        H323PutClocktype(&tempPtr, h323->callIdentifier);

        // mediaWaitForConnect
        // wait to transmit voice untill sending the Connect message
        H323PutInt(&tempPtr, TRUE);

        // canOverlapSend
        // overlap sending not supported
        H323PutInt(&tempPtr, FALSE);

        // endpointIdentifier
        H323PutClocktype(&tempPtr, 0);

        // multipleCalls
        H323PutInt(&tempPtr, FALSE);

        // maintainConnection
        H323PutInt(&tempPtr, FALSE);

        // initiator changed to SETUP state
        h323->callSignalState = H323_SETUP;

        // Sending the Setup messsage to the receiver
        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            connectionId,
            TRACE_H323);

        APP_AddPayload(
            node,
            tcpmsg,
            setupPacket,
            SIZE_OF_H225_SETUP);

        APP_TcpSend(node, tcpmsg);

        MEM_free(setupPacket);
    }
    else
    {
        // already engaged by some other connection
        if (DEBUG)
        {
            printf("Node %u is not in IDLE state to initiate a"
                " new connection\n", node->nodeId);
        }
    }
}


//
// NAME:        H323SendH225CallProceedingMessage
//
// PURPOSE:     Create and send CallProceeding message by the receiver
//              of the call. This call is initiated when receiver gets
//              a Setup and informs the initiator by sending this message
//
// PARAMETERS:  node:           node which is creating Setup message
//              connectionId:   TCP connection ID
//              cllInditifier:  global call indetifier
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323SendH225CallProceedingMessage(Node* node,
                                  int connectionId)
{
    unsigned char* tempPtr = NULL;
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    // Allocating space for the CallProceeding packet
    H225CallProceeding *callProceedingPacket =
        (H225CallProceeding *) MEM_malloc (sizeof(H225CallProceeding));

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), clockStr);
        printf("Node %u sends CallProceeding message at connection "
            "%d at %s\n", node->nodeId, connectionId, clockStr);
    }

    memset(callProceedingPacket, 0, sizeof(H225CallProceeding));

    tempPtr = (unsigned char*) callProceedingPacket;

    Q931PduCreate((Q931Message*) callProceedingPacket,
                  1,
                  (unsigned char) connectionId,
                  Q931_CALL_PROCEEDING);

    // Fill the value in Call Proceeding packet
    tempPtr += sizeof(Q931Message);

    // H225_ProtocolID
    H323PutString(&tempPtr, H225_ProtocolID, IDENTIFIER_SIZE);

    // destinationInfo
    H323PutInt(&tempPtr, Terminal);

    // callIdentifier
    H323PutClocktype(&tempPtr, h323->callIdentifier);

    // multipleCalls
    H323PutInt(&tempPtr, FALSE);

    // maintainConnection
    H323PutInt(&tempPtr, FALSE);

    // Sending back CallProceeding message to the initiator
    Message *tcpmsg = APP_TcpCreateMessage(
        node,
        connectionId,
        TRACE_H323);

    APP_AddPayload(
        node,
        tcpmsg,
        callProceedingPacket,
        SIZE_OF_H225_CALL_PROCEEDING);

    APP_TcpSend(node, tcpmsg);

    MEM_free(callProceedingPacket);
}


//
// NAME:        H323SendH225AlertingMessage
//
// PURPOSE:     Create and send Alerting message by the receiver
//              of the call. This call is initiated when receiver starts
//              ringing it informs the initiator by sending this message
//
// PARAMETERS:  node:           node which is creating Alerting message
//              connectionId:   TCP connection ID
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323SendH225AlertingMessage(Node* node, int connectionId)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    // receiver only can send this message from CALL_PROCEEDING state
    if (h323->callSignalState == H323_CALL_PROCEEDING)
    {
        unsigned char* tempPtr = NULL;

        // Allocating space for the Alerting packet
        H225Alerting *alertingPacket =
            (H225Alerting *) MEM_malloc (sizeof(H225Alerting));

        if (DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            ctoa(node->getNodeTime(), clockStr);
            printf("Node %u sends Alerting message at connection "
                "%d at %s\n", node->nodeId, connectionId, clockStr);
        }

        memset(alertingPacket, 0, sizeof(H225Alerting));

        tempPtr = (unsigned char*) alertingPacket;

        Q931PduCreate((Q931Message*) alertingPacket,
                      1,
                      (unsigned char) connectionId,
                      Q931_ALERTING);

        // Fill the value in alerting packet
        tempPtr += sizeof(Q931Message);

        // H225_ProtocolID
        H323PutString(&tempPtr, H225_ProtocolID, IDENTIFIER_SIZE);

        // destinationInfo
        H323PutInt(&tempPtr, Terminal);

        // callIdentifier
        H323PutClocktype(&tempPtr, h323->callIdentifier);

        // multipleCalls
        H323PutInt(&tempPtr, FALSE);

        // maintainConnection
        H323PutInt(&tempPtr, FALSE);

        // receiver sending Alerting message to initiator
        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            connectionId,
            TRACE_H323);

        APP_AddPayload(
            node,
            tcpmsg,
            alertingPacket,
            SIZE_OF_H225_ALERTING);

        APP_TcpSend(node, tcpmsg);

        // change receivers state to ALERTING
        h323->callSignalState = H323_ALERTING;

        MEM_free(alertingPacket);
    }
    else
    {
        // going to send Alerting message while not in CALL_PROCEEDING
        // state is not possible. so current TCP connection is closed
        VoipHostData* voip =
            VoipCallReceiverGetDataForConnectionId(node, connectionId);

        ERROR_Assert(voip, "Invalid Call receiver\n");

        voip->connectionId = INVALID_ID;

        if (DEBUG)
        {
            printf("Node %u not in Call Proceeding state to start Alerting."
                "Connection closed.\n", node->nodeId);
            printf("Node %u sends TCP close\n", node->nodeId);
        }

        APP_TcpCloseConnection(node, connectionId);
    }
}


//
// NAME:        H323SendH225ConnectMessage
//
// PURPOSE:     Create and send Connect message by the receiver
//              of the call. This call is initiated when user in the
//              receiver end pick up the phone. Now receiver sends
//              Connect message to initiator and creates the connection
//
// PARAMETERS:  node:           node which is creating Connect message
//              connectionId:   TCP connection ID
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323SendH225ConnectMessage(Node* node, int connectionId)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    // Connect message only can be sent by the receiver while it is
    // in ALERTING state only.
    if (h323->callSignalState == H323_ALERTING)
    {
        unsigned char* tempPtr = NULL;

        // Allocating space for the Connect packet
        H225Connect *connectPacket =
            (H225Connect *) MEM_malloc (sizeof(H225Connect));

        if (DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            ctoa(node->getNodeTime(), clockStr);
            printf("Node %u sends Connect message at connection %d at %s\n",
                node->nodeId, connectionId, clockStr);
        }

        memset(connectPacket, 0, sizeof(H225Connect));

        tempPtr = (unsigned char*) connectPacket;

        Q931PduCreate((Q931Message*) connectPacket,
                      1,
                      (unsigned char) connectionId,
                      Q931_CONNECT);

        // Fill the value in Connect packet
        tempPtr += sizeof(Q931Message);

        // H225_ProtocolID
        H323PutString(&tempPtr, H225_ProtocolID, IDENTIFIER_SIZE);

        // destinationInfo
        H323PutInt(&tempPtr, Terminal);

        // conferenceId
        // conference ID is not considered now
        H323PutClocktype(&tempPtr, 0);

        // callIdentifier
        H323PutClocktype(&tempPtr, h323->callIdentifier);

        // multipleCalls
        H323PutInt(&tempPtr, FALSE);

        // maintainConnection
        H323PutInt(&tempPtr, FALSE);


        // receiver sending Connect message to initiator
        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            connectionId,
            TRACE_H323);

        APP_AddPayload(
            node,
            tcpmsg,
            connectPacket,
            SIZE_OF_H225_CONNECT);

        APP_TcpSend(node, tcpmsg);

        // Now receiver is in CONNECT state
        h323->callSignalState = H323_CONNECT;

        MEM_free(connectPacket);
    }
    else
    {
        // going to send Connect message while not in ALERTING
        // state is not possible. so current TCP connection is closed
        VoipHostData* voip =
            VoipCallReceiverGetDataForConnectionId(node, connectionId);

        ERROR_Assert(voip, "Invalid Call receiver\n");

        voip->connectionId = INVALID_ID;

        if (DEBUG)
        {
            printf("Node %u not in Alerting state to start Connect."
                "Connection closed.\n", node->nodeId);
            printf("Node %u sends TCP close\n", node->nodeId);
        }

        APP_TcpCloseConnection(node, connectionId);
    }
}


//
// NAME:        H323SendH225ReleaseCompleteMessage
//
// PURPOSE:
//
// PARAMETERS:  node:           node which is creating Connect message
//              connectionId:   TCP connection ID
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323SendH225ReleaseCompleteMessage(
    Node* node,
    int connectionId,
    unsigned char callRefFlag)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    // ReleaseComplete message only can be sent by the receiver or
    // sender while it is in CONNECT state only.
    if (h323->callSignalState == H323_CONNECT)
    {
        unsigned char* tempPtr = NULL;

        // Allocating space for the ReleaseComplete packet
        H225ReleaseComplete *releaseCompletePacket =
            (H225ReleaseComplete *)MEM_malloc (sizeof(H225ReleaseComplete));

        if (DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            ctoa(node->getNodeTime(), clockStr);
            printf("Node %u sends ReleaseComplete message at connection "
                "%d at %s\n", node->nodeId, connectionId, clockStr);
        }

        memset(releaseCompletePacket, 0, sizeof(H225ReleaseComplete));

        tempPtr = (unsigned char*) releaseCompletePacket;

        Q931PduCreate((Q931Message*) releaseCompletePacket,
                      callRefFlag,
                      (unsigned char) connectionId,
                      Q931_RELEASE_COMPLETE);

        // Fill the value in Connect packet
        tempPtr += sizeof(Q931Message);

        // H225_ProtocolID
        H323PutString(&tempPtr, H225_ProtocolID, IDENTIFIER_SIZE);

        // callIdentifier
        H323PutClocktype(&tempPtr, h323->callIdentifier);

        // sending ReleaseComplete message to initiator/receiver
        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            connectionId,
            TRACE_H323);

        APP_AddPayload(
            node,
            tcpmsg,
            releaseCompletePacket,
            SIZE_OF_H225_RELEASE_COMPLETE);

        APP_TcpSend(node, tcpmsg);

        // Now node is in IDLE state
        h323->callSignalState = H323_IDLE;

        MEM_free(releaseCompletePacket);
    }
}


//
// NAME:        H323HandleH225SetupForTerminal
//
// PURPOSE:     This function handles when Setup message is received
//              by the receiver. After receiving it should send Call
//              Proceeding message to the initiator terminal
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    Setup message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323HandleH225SetupForTerminal(
    Node* node,
    unsigned char* data,
    TransportToAppDataReceived *dataRecvd)
{

    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;
    H225RasTerminalData* h225RasTerminalData;

    H225Setup* h225Setup = (H225Setup*) data;

    // initiator trans address collected from Setup message
    char* msgPtr = ((char*) h225Setup) + 42;

    H225TransportAddress initiatorCallSignalTransAddr;
    VoipHostData* voip;

    H323CopyTransAddr(&initiatorCallSignalTransAddr,
        (H225TransportAddress*) msgPtr);

    // retrive voip structure in the receiver end
    voip = VoipGetReceiverDataForCallSignal(
                    node,
                    initiatorCallSignalTransAddr.port);

    // Now updating the receiver's Data Structures as voip->callsignalAddress
    // will be used in sending ARQ

    voip->localAddr = MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
                                node,
                                node->nodeId,
                                voip->clientInterfaceIndex);

    voip->callSignalAddr = initiatorCallSignalTransAddr.ipAddr;
    voip->remoteAddr = initiatorCallSignalTransAddr.ipAddr;

    // call receive statistics
    h323->h323Stat.callReceived++;

    if (!voip)
    {
        // if voip not found that indicates call is not established
        // one reason may be REJECT is selected during configuration
        if (DEBUG)
        {
            printf("Call is rejected. Connection closed.\n");
            printf("Node %u sends TCP close\n", node->nodeId);
        }

        APP_TcpCloseConnection(node, dataRecvd->connectionId);
        return;
    }

    // set connection ID in voip structure
    voip->connectionId = dataRecvd->connectionId;

    // Only in IDLE state Setup message will be processed by the
    // receiving terminal. Otherwise it is already engaged by some
    // other call, and current request is ignored
    if (h323->callSignalState == H323_IDLE)
    {
        H225Setup *setupPacket = (H225Setup*) data;

        // receiving terminal changed to CALL_PROCEEDING state
        h323->callSignalState = H323_CALL_PROCEEDING;
        h323->callIdentifier = setupPacket->callIdentifier;

        // host is now received state
        h323->hostState = H323_RECEIVER;

        if (DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            ctoa(node->getNodeTime(), clockStr);
            printf("Node %u receives Setup message at %s\n",
                node->nodeId, clockStr);
        }

        // sends CallProceeding message to initiator
        H323SendH225CallProceedingMessage(node,
                                          dataRecvd->connectionId);

        if (h323->gatekeeperAvailable)
        {
            // gatekeeper available, initiate ARQ from receiver end
            h225RasTerminalData =
                (H225RasTerminalData *) h225Ras->h225RasCommon;

            if (h225RasTerminalData->h225RasGotRegistrationResponse)
            {
                H225RasCreateAndSendARQForReceiver(node, voip);
            }
            else
            {
                if (DEBUG)
                {
                    printf("Receiver (Node %u) is unable to register "
                        "with gatekeeper.\n So do not able to initiate "
                        "a call\n", node->nodeId);
                }
            }
        }
        else
        {
            // gatekeeper not available, started Alerting message
            H323InitiateAlertingMessage(node, dataRecvd->connectionId);
        }
    }
    else
    {
        // in some other state than IDLE, it is due to some error
        // so current TCP connection is closed
        if (voip)
        {
            voip->connectionId = INVALID_ID;
        }

        if (DEBUG)
        {
            printf("Node %u not in Idle state to receive Setup."
                " Connection closed.\n", node->nodeId);
            printf("Node %u sends TCP close\n", node->nodeId);
        }

        APP_TcpCloseConnection(node, dataRecvd->connectionId);
    }
}


//
// NAME:        H323RetryTimerHandleH225SetupForGatekeeper
//
// PURPOSE:     This function handles handles to wait for Setup message
//              when gatekeeper routed and in Gatekeeper
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    Setup message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323RetryTimerHandleH225SetupForGatekeeper(
    Node* node,
    int connectionId,
    unsigned char* data)
{
    Message* setupDelayMsg;
    H225SetupDelayMsg* tmpPtr;

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), clockStr);
        printf("Node %u sending MSG_APP_H225_SETUP_DELAY_Timer"
            " at %s\n", node->nodeId, clockStr);
    }

    // self timer required in the gatekeeper when call model is gatekeeper
    // routed. this is to trap Setup message from initiator and relay
    // to receiver side
    setupDelayMsg = MESSAGE_Alloc(
                            node,
                            APP_LAYER,
                            APP_H323,
                            MSG_APP_H225_SETUP_DELAY_Timer);

    MESSAGE_InfoAlloc(node, setupDelayMsg, (sizeof(H225SetupDelayMsg)));

    tmpPtr = (H225SetupDelayMsg*) MESSAGE_ReturnInfo(setupDelayMsg);

    tmpPtr->data = data;
    tmpPtr->connectionId = connectionId;

    MESSAGE_Send(node, setupDelayMsg, H225_RAS_SETUP_DELAY);
}


//
// NAME:        H323HandleH225SetupForGatekeeper
//
// PURPOSE:     This function handles when Setup message is received
//              by the receiver. After receiving it should send Call
//              Proceeding message to the initiator terminal
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    Setup message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323HandleH225SetupForGatekeeper(
    Node* node,
    unsigned char* data,
    int connectionId)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    // when getting Setup during gatekeeper routed call model, get
    // corresponding outgoing connection ID for forwarding Setup message
    int egresConnectionId =
        H225RasSearchCallListForConnectionId(h225Ras,
                                             connectionId,
                                             INGRES_TO_EGRES);

    if (egresConnectionId == INVALID_ID)
    {
        // if still outgoing connection ID is not established, wait timer
        H323RetryTimerHandleH225SetupForGatekeeper(node,
                                                   connectionId,
                                                   data);
    }
    else
    {
        if (DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            ctoa(node->getNodeTime(), clockStr);
            printf("Node %u relays Setup message at connection %d at %s\n",
                node->nodeId, egresConnectionId, clockStr);
        }

        // Sending the Setup messsage towards receiver
        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            egresConnectionId,
            TRACE_H323);

        APP_AddPayload(
            node,
            tcpmsg,
            data,
            SIZE_OF_H225_SETUP);

        APP_TcpSend(node, tcpmsg);

        MEM_free(data);
    }
}


//
// NAME:        H323HandleH225Setup
//
// PURPOSE:     Handling of Setup message
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    Setup message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323HandleH225Setup(
    Node* node,
    unsigned char* data,
    TransportToAppDataReceived *dataRecvd)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    switch (h323->endpointType)
    {
        case Terminal:
        {
            // processing setup message at Terminal
            H323HandleH225SetupForTerminal(node, data, dataRecvd);
            break;
        }
        case Gatekeeper:
        {
            // processing setup message at Gatekeeper
            H323HandleH225SetupForGatekeeper(
                node,
                data,
                dataRecvd->connectionId);
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Invalid endpoint\n");
            break;
        }
    }
}


//
// NAME:        H323HandleH225CallProceedingForTerminal
//
// PURPOSE:     This function handles when CallProceeding message is
//              received by the initiator. After receiving it should
//              go to CallProceeding state and will wait for call alerting
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    CallProceeding message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323HandleH225CallProceedingForTerminal(
    Node* node,
    TransportToAppDataReceived *dataRecvd)
{

    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    // initiating terminal only handles this message when it is in SETUP
    // state.
    if (h323->callSignalState == H323_SETUP)
    {
        if (DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            ctoa(node->getNodeTime(), clockStr);
            printf("Node %u receives Call Proceeding message at %s\n",
                node->nodeId, clockStr);
        }

        // change to CALL_PROCEEDING state
        h323->callSignalState = H323_CALL_PROCEEDING;
    }
    else
    {
        // in some other state than SETUP, it is due to some error
        // so current TCP connection is closed
        VoipHostData* voip = VoipCallInitiatorGetDataForConnectionId(
            node, dataRecvd->connectionId);

        ERROR_Assert(voip, "Invalid Call initiator\n");

        voip->connectionId = INVALID_ID;

        if (DEBUG)
        {
            printf("Node %u not in Setup state to receive CallProceeding."
                " Connection closed.\n", node->nodeId);
            printf("Node %u sends TCP close\n", node->nodeId);
        }

        APP_TcpCloseConnection(node, dataRecvd->connectionId);
    }
}


//
// NAME:        H323HandleH225CallProceedingForGatekeeper
//
// PURPOSE:
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    CallProceeding message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323HandleH225CallProceedingForGatekeeper(
    Node* node,
    unsigned char* data,
    TransportToAppDataReceived *dataRecvd,
    H225RasData* h225Ras)
{
    // get connection ID towards initiator TCP connection, to forward
    // CallProceeding message
    int ingresConnectionId =
        H225RasSearchCallListForConnectionId(h225Ras,
                                             dataRecvd->connectionId,
                                             EGRES_TO_INGRES);

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), clockStr);
        printf("Node %u relays CallProceeding message at connection"
            " %d at %s\n", node->nodeId, ingresConnectionId, clockStr);
    }

    if (ingresConnectionId != INVALID_ID)
    {
        // Sending the CallProceeding messsage towards initiator
        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            ingresConnectionId,
            TRACE_H323);

        APP_AddPayload(
            node,
            tcpmsg,
            data,
            SIZE_OF_H225_CALL_PROCEEDING);

        APP_TcpSend(node, tcpmsg);
    }
}


//
// NAME:        H323HandleH225CallProceeding
//
// PURPOSE:
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    CallProceeding message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323HandleH225CallProceeding(
    Node* node,
    unsigned char* data,
    TransportToAppDataReceived *dataRecvd)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    switch (h323->endpointType)
    {
        case Terminal:
        {
            // handling of CallProceeding at the Terminal
            H323HandleH225CallProceedingForTerminal(node, dataRecvd);
            break;
        }
        case Gatekeeper:
        {
            // handling of CallProceeding at the Gatekeeper
            H323HandleH225CallProceedingForGatekeeper(node,
                data, dataRecvd, h225Ras);
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Invalid endpoint\n");
            break;
        }
    }
}


//
// NAME:        H323HandleH225Alerting
//
// PURPOSE:     This function handles when Alerting message is
//              received by the initiator. After receiving it should
//              go to Alerting state and will wait for Connection
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    Alerting message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323HandleH225AlertingForTerminal(
    Node* node,
    TransportToAppDataReceived *dataRecvd)
{
    AppMultimedia* multimedia = node->appData.multimedia;
    H323Data* h323 = (H323Data*) multimedia->sigPtr;


    // allowed current state must be CALL_PROCEEDING. For all other cases
    // TCP connection is closed
    if (h323->callSignalState == H323_CALL_PROCEEDING)
    {
        Message* callTimeoutMsg;

        if (DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            ctoa(node->getNodeTime(), clockStr);
            printf("Node %u receives Alerting message at %s\n",
                node->nodeId, clockStr);
        }

        // initiating terminal changed to ALERTING state
        h323->callSignalState = H323_ALERTING;

        // now initiating terminal fires a self timer to wait for the
        // connection. Before the Connect message if this message if fired
        // then call timeout happens and connection will not be established
        callTimeoutMsg = MESSAGE_Alloc(node,
                                       APP_LAYER,
                                       APP_H323,
                                       MSG_APP_H323_Call_Timeout);

        MESSAGE_InfoAlloc(node, callTimeoutMsg, sizeof(int));
        *((int*)MESSAGE_ReturnInfo(callTimeoutMsg)) = dataRecvd->connectionId;

        MESSAGE_Send(node, callTimeoutMsg, multimedia->callTimeout);
    }
    else
    {
        // error connection. TCP connection closed
        VoipHostData* voip = VoipCallInitiatorGetDataForConnectionId(
            node, dataRecvd->connectionId);

        ERROR_Assert(voip, "Invalid Call initiator\n");

        voip->connectionId = INVALID_ID;

        if (DEBUG)
        {
            printf("Node %u not in CallProceeding state to receive Alerting"
                ". Connection closed.\n", node->nodeId);
            printf("Node %u sends TCP close\n", node->nodeId);
        }

        APP_TcpCloseConnection(node, dataRecvd->connectionId);
    }
}


//
// NAME:        H323HandleH225AlertingForGatekeeper
//
// PURPOSE:
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    CallProceeding message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323HandleH225AlertingForGatekeeper(
    Node* node,
    unsigned char* data,
    TransportToAppDataReceived *dataRecvd,
    H225RasData* h225Ras)
{
    // get connection ID towards initiator TCP connection, to forward
    // Alerting message
    int ingresConnectionId =
        H225RasSearchCallListForConnectionId(h225Ras,
                                             dataRecvd->connectionId,
                                             EGRES_TO_INGRES);

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), clockStr);
        printf("Node %u relays Alerting message at connection %d at %s\n",
            node->nodeId, ingresConnectionId, clockStr);
    }

    if (ingresConnectionId != INVALID_ID)
    {
        // Sending the Alerting messsage towards initiator
        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            ingresConnectionId,
            TRACE_H323);

        APP_AddPayload(
            node,
            tcpmsg,
            data,
            SIZE_OF_H225_ALERTING);

        APP_TcpSend(node, tcpmsg);
    }
}


//
// NAME:        H323HandleH225AlertingForGatekeeper
//
// PURPOSE:
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    CallProceeding message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323HandleH225Alerting(
    Node* node,
    unsigned char* data,
    TransportToAppDataReceived *dataRecvd)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    switch (h323->endpointType)
    {
        case Terminal:
        {
            // handling of Alerting message at Terminal
            H323HandleH225AlertingForTerminal(node, dataRecvd);
            break;
        }
        case Gatekeeper:
        {
            // handling of Alerting message at Gatekeeper
            H323HandleH225AlertingForGatekeeper(node,
                data, dataRecvd, h225Ras);
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Invalid endpoint\n");
            break;
        }
    }
}



//
// NAME:        H323HandleH225Connect
//
// PURPOSE:     This function handles when Connect message is
//              received by the initiator. After receiving it should
//              go to Connect state and will be ready for voice or data
//              transfer
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    Alerting message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323HandleH225ConnectForTerminal(
    Node* node,
    TransportToAppDataReceived *dataRecvd)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;


    // valid state is ALERTING to handle this message by initiating terminal
    if (h323->callSignalState == H323_ALERTING)
    {
        VoipHostData* voip = VoipCallInitiatorGetDataForConnectionId(
                                    node, dataRecvd->connectionId);

        if (voip->endTime < node->getNodeTime())
        {
            ERROR_ReportWarning("Call already terminated by the "
            "Initiator\n");
            voip->connectionId = INVALID_ID;
            APP_TcpCloseConnection(node, dataRecvd->connectionId);
            return;
        }

        if (DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            ctoa(node->getNodeTime(), clockStr);
            printf("Node %u receives Connect message at %s\n",
                node->nodeId, clockStr);
        }

        // initiator is in CONNECT state, ready to communicate
        h323->callSignalState = H323_CONNECT;
        h323->h323Stat.connectionEstablished++;

        // connection established, now ask VOIP for voice transmission
        VoipStartTransmission(node, voip);
    }
    else
    {
        // errorneous condition. TCP connection closed.
        VoipHostData* voip = VoipCallInitiatorGetDataForConnectionId(
            node, dataRecvd->connectionId);

        ERROR_Assert(voip, "Invalid Call initiator\n");

        voip->connectionId = INVALID_ID;

        if (DEBUG)
        {
            printf("Node %u not in Alerting state to receive Connect"
                ". Connection closed.\n", node->nodeId);
            printf("Node %u sends TCP close\n", node->nodeId);
        }

        APP_TcpCloseConnection(node, dataRecvd->connectionId);
    }
}


//
// NAME:        H323HandleH225ConnectForGatekeeper
//
// PURPOSE:
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    CallProceeding message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323HandleH225ConnectForGatekeeper(
    Node* node,
    unsigned char* data,
    TransportToAppDataReceived *dataRecvd,
    H225RasData* h225Ras)
{
    // get connection ID towards initiator TCP connection, to forward
    // Connect message
    int ingresConnectionId =
        H225RasSearchCallListForConnectionId(h225Ras,
                                             dataRecvd->connectionId,
                                             EGRES_TO_INGRES);

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), clockStr);
        printf("Node %u relays Connect message at connection %d at %s\n",
            node->nodeId, ingresConnectionId, clockStr);
    }

    if (ingresConnectionId != INVALID_ID)
    {
        // Sending the Setup messsage towards receiver
        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            ingresConnectionId,
            TRACE_H323);

        APP_AddPayload(
            node,
            tcpmsg,
            data,
            SIZE_OF_H225_CONNECT);

        APP_TcpSend(node, tcpmsg);
    }
}


//
// NAME:        H323HandleH225Connect
//
// PURPOSE:
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    CallProceeding message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323HandleH225Connect(Node* node,
                           unsigned char* data,
                           TransportToAppDataReceived *dataRecvd)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    switch (h323->endpointType)
    {
        case Terminal:
        {
            // handling of Connect message at Terminal
            H323HandleH225ConnectForTerminal(node, dataRecvd);
            break;
        }
        case Gatekeeper:
        {
            // handling of Connect message at Gatekeeper
            H323HandleH225ConnectForGatekeeper(node,
                data, dataRecvd, h225Ras);
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Invalid endpoint\n");
            break;
        }
    }
}


//
// NAME:        H323HandleH225ReleaseComplete
//
// PURPOSE:     This function handles when ReleaseComplete message is
//              received by the node. After receiving it should
//              go to IDLE state and close TCP connection.
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    ReleaseComplete message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323HandleH225ReleaseComplete(
    Node* node,
    unsigned char* data,
    TransportToAppDataReceived *dataRecvd)
{
    int newConnectionId;
    H225ConectionIdType connectionIdType;
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    switch (h323->endpointType)
    {
        case Gatekeeper:
        {
            // handling of ReleaseComplete message at Gatekeeper
            char callRefFlag = *((char*)data + 2);

            // get the message flow direction
            if (callRefFlag & 0x80)
            {
                connectionIdType = EGRES_TO_INGRES;
            }
            else
            {
                connectionIdType = INGRES_TO_EGRES;
            }

            // get opposite side connection ID
            newConnectionId = H225RasSearchAndMarkCallEntry(
                                    h225Ras,
                                    dataRecvd->connectionId,
                                    connectionIdType);

            if (newConnectionId != INVALID_ID)
            {
                // Sending the messsage towards receiver
                Message *tcpmsg = APP_TcpCreateMessage(
                    node,
                    newConnectionId,
                    TRACE_H323);

                APP_AddPayload(
                    node,
                    tcpmsg,
                    data,
                    SIZE_OF_H225_RELEASE_COMPLETE);

                APP_TcpSend(node, tcpmsg);    
            }
            break;
        }
        case Terminal:
        {
            break;
        }
        default:
            break;
    }

    // valid state is CONNECT to handle this message
    if (h323->callSignalState == H323_CONNECT)
    {
        if (DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            ctoa(node->getNodeTime(), clockStr);
            printf("Node %u receives ReleaseComplete message at %s\n",
                node->nodeId, clockStr);
        }

        // initiator is in IDLE state, ready to new connection
        h323->callSignalState = H323_IDLE;

        // realese CRV. Afterwards, the Call Reference Value (CRV) is
        // available for reuse TBD
    }
}


//
// NAME:        H323InitiateConnection
//
// PURPOSE:     Open request for a TCP connection from the initiating
//              terminal
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    Start message from VOIP application
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
void H323InitiateConnection(Node* node, void* voipData)
{
    VoipHostData* voip = (VoipHostData*) voipData;

    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    ERROR_Assert(voip, "Invalid Call initiator\n");

    if (h323->callSignalState == H323_IDLE)
    {
        if (h323->gatekeeperAvailable)
        {
            if (AppVoipClientGetSessionAddressState(node, voip))
            {
                // gatekeeper available, initiate ARQ
                H225RasAdmissionRequest(node, voip);
            }
        }
        else
        {
            // gatekeeper not available, create direct connection
            if (DEBUG)
            {
                char sourceIpAddr[MAX_STRING_LENGTH];
                char destIpAddr[MAX_STRING_LENGTH];
                char buf[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), buf);

                IO_ConvertIpAddressToString(
                    voip->callSignalAddr, sourceIpAddr);

                IO_ConvertIpAddressToString(
                    voip->remoteAddr, destIpAddr);

                ctoa(node->getNodeTime(), buf);

                printf("Node %u initiates to open TCP connection at %s\n"
                       "SourceAddress = %s\n SourcePort = %u\n"
                       "DestinationAddress = %s\n\n",
                            node->nodeId,
                            buf,
                            sourceIpAddr,
                            voip->callSignalPort,
                            destIpAddr);
            }

            h323->hostState = H323_INITIATOR;
            h323->h323Stat.callInitiated++;

            // open TCP connection
            Address localAddr;
            memset(&localAddr, 0, sizeof(Address));
            localAddr.networkType = NETWORK_IPV4;
            localAddr.interfaceAddr.ipv4 = voip->callSignalAddr;

            Address remoteAddr;
            memset(&remoteAddr, 0, sizeof(Address));
            remoteAddr.networkType = NETWORK_IPV4;
            remoteAddr.interfaceAddr.ipv4 = voip->remoteAddr;
            std::string url;
            url.clear();
            node->appData.appTrafficSender->appTcpOpenConnection(
                                                node,
                                                APP_H323,
                                                localAddr,
                                                voip->callSignalPort,
                                                remoteAddr,
                                                (UInt16) APP_H323,
                                                voip->callSignalPort,
                                                APP_DEFAULT_TOS,
                                                ANY_INTERFACE,
                                                url,
                                                0,
                                                voip->destNodeId,
                                                voip->clientInterfaceIndex,
                                                voip->destInterfaceIndex);
        }
    }
    else
    {
        if (DEBUG)
        {
            printf("Node %u is not idle to initiate another call\n",
                node->nodeId);
        }
    }
}


//
// NAME:        H323TerminateConnection
//
// PURPOSE:     Close TCP connection as requested by VOIP application
//
// PARAMETERS:  node:   node which going to terminate connection
//              voip:   voip structure
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
void H323TerminateConnection(Node* node, void* voipData)
{
    VoipHostData* voip = (VoipHostData*) voipData;

    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    unsigned char callRefFlag = 0;

    ERROR_Assert(voip, "Invalid Call initiator/receiver\n");

    if (DEBUG)
    {
        printf("Node %u receives end of conversation from VOIP\n",
            node->nodeId);
    }

    // get direction of release complete
    if (!voip->initiator)
    {
        callRefFlag = 1;
    }

    // send relese complete message
    H323SendH225ReleaseCompleteMessage(
        node, voip->connectionId, callRefFlag);

    // back to normal state
    h323->callSignalState = H323_IDLE;
    h323->hostState = H323_NORMAL;

    if (DEBUG)
    {
        printf("Node %u sends TCP close\n", node->nodeId);
    }

    APP_TcpCloseConnection(node, voip->connectionId);

    voip->connectionId = INVALID_ID;
}


//
// NAME:        H323InitiateAlertingMessage
//
// PURPOSE:     Receiving terminal is going for ringing, so send Alerting
//              message to initiating terminal
//
// PARAMETERS:  node:           node which is receiving Setup message
//              connectionId:   Connection ID
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
void H323InitiateAlertingMessage(Node* node, int connectionId)
{
    // send Alerting message to initiator
    H323SendH225AlertingMessage(node, connectionId);

    // after sending call alerting, ask VOIP for connection established
    VoipAskForConnectionEstablished(node,
                                    connectionId);
}


//
// NAME:        H323InitiateConnectMessage
//
// PURPOSE:     Initiate Connect message by the receiver
//
// PARAMETERS:  node:           node which is receiving Connect message
//              connectionId:   Connection ID
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323InitiateConnectMessage(Node* node, int connectionId)
{
    VoipHostData* voip = VoipCallReceiverGetDataForConnectionId(
        node, connectionId);

    // if connection ID is valid, then voipo shoud return valid entry
    if (voip)
    {
        H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

        h323->h323Stat.connectionEstablished++;

        if (DEBUG)
        {
            char buf[MAX_STRING_LENGTH];
            ctoa(node->getNodeTime(), buf);
            printf("Node %u receives Connect initiator timer at "
                "%s\n", node->nodeId, buf);
        }

        // send Connect message towards the initiating terminal
        H323SendH225ConnectMessage(node, connectionId);
        VoipConnectionEstablished(node, voip);
    }
}


//
// NAME:        H323CheckCallTimeout
//
// PURPOSE:     Check whether call timeout happens before receiving
//              Connect message
//
// PARAMETERS:  node:           node which is receiving call timeout
//              connectionId:   Connection ID
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323CheckCallTimeout(Node* node, int connectionId)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    // call is established if before the call timeout, connection is created
    // so if terminal is not in connect state, that means connection is
    // going to be rejected.
    if (h323->callSignalState != H323_CONNECT)
    {
        // call time out. TCP connection closed.
        VoipHostData* voip = VoipCallInitiatorGetDataForConnectionId(
            node, connectionId);

        if (voip)
        {
            voip->connectionId = INVALID_ID;

            if (DEBUG)
            {
                printf("Node %u not receiving any Connect within call "
                    "timeout\n", node->nodeId);
                printf("Node %u sends TCP close\n", node->nodeId);
            }
            APP_TcpCloseConnection(node, connectionId);

            // for call time out, change the state to idle, connection refused
            h323->callSignalState = H323_IDLE;
            h323->hostState = H323_NORMAL;
        }
    }
}


//
// NAME:        H323ProcessActiveOpenResult
//
// PURPOSE:
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    CallProceeding message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323ProcessActiveOpenResult(
    Node* node,
    H323Data* h323,
    TransportToAppOpenResult* openResult)
{
    switch (h323->endpointType)
    {
        case Terminal:
        {
            VoipHostData* voip = NULL;

            // TCP connection initiated by the Terminal
            voip = VoipGetInitiatorDataForCallSignal(
                                    node,
                                    openResult->localPort);

            ERROR_Assert(voip, "Invalid Call initiator\n");

            if (!h323->gatekeeperAvailable)
            {
                // gatekeeper not available
                voip->connectionId = openResult->connectionId;
                VoipUpdateInitiator(node, voip, openResult, TRUE);

                H323SendH225SetupMessage(node, openResult->connectionId);
            }
            else
            {
                H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

                H225RasTerminalData* h225RasTerminalData =
                    (H225RasTerminalData *) h225Ras->h225RasCommon;

                // gatekeeper available now check whether registered
                if (h225RasTerminalData->h225RasGotRegistrationResponse)
                {
                    // terminal registered with gatekeeper
                    voip->connectionId = openResult->connectionId;
                    VoipUpdateInitiator(node, voip, openResult, FALSE);
                    H323SendH225SetupMessage(node, voip->connectionId);
                }
                else
                {
                    // registration not done connection failed
                    if (DEBUG)
                    {
                        printf("Initiator (Node %u) is unable to register "
                            "with gatekeeper.\n So do not able to initiate "
                            "a call\n", node->nodeId);
                        printf("Node %u sends TCP close\n", node->nodeId);
                    }

                    APP_TcpCloseConnection(node, openResult->connectionId);
                }
            }
            break;
        }
        case Gatekeeper:
        {
            // TCP connection initiated by Gatekeeper
            H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

            // get call entry from call list for this connection
            if (openResult->localAddr.networkType == NETWORK_IPV6)
            {
                ERROR_ReportError("Presently, h323 does not "
                "support IPv6.\n");
            }

            H225RasCallEntry* callEntry = H225RasSearchCallList(
                h225Ras,
                GetIPv4Address(openResult->localAddr),
                openResult->localPort,
                CURRENT);

            ERROR_Assert(callEntry, "callEntry not found\n");

            // save connection id of the out going TCP connection
            callEntry->egresConnectionId = openResult->connectionId;
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Invalid endpoint\n");
            break;
        }
    }
}


//
// NAME:        H323ProcessPassiveOpenResult
//
// PURPOSE:
//
// PARAMETERS:  node:   node which is receiving Setup message
//              msg:    CallProceeding message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323ProcessPassiveOpenResult(
    Node* node,
    H323Data* h323,
    TransportToAppOpenResult* openResult)
{
    switch (h323->endpointType)
    {
        case Terminal:
        {
            // TCP connection received at Terminal
            // get voip structure for this connection
            if (openResult->remoteAddr.networkType == NETWORK_IPV6)
            {
                ERROR_ReportError("Presently, h323 does not support"
                " IPv6.\n");
            }

            VoipHostData* voipReceiver =
                VoipGetReceiverDataForCallSignal(
                    node,
                    openResult->remotePort);

            if (voipReceiver)
            {
                voipReceiver->connectionId = openResult->connectionId;
            }

            // global setting of connection ID
            h323->connectionId = openResult->connectionId;
            break;
        }
        case Gatekeeper:
        {
            // TCP connection received at Gatekeeper, valid for gatekeeper
            // routed model
            H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

            if (openResult->remoteAddr.networkType == NETWORK_IPV6)
            {
                ERROR_ReportError("Presently, h323 does not"
                " support IPv6.\n");
            }

            H225RasCallEntry* callEntry = H225RasSearchCallList(
                h225Ras,
                GetIPv4Address(openResult->remoteAddr),
                openResult->remotePort,
                PREVIOUS);

            ERROR_Assert(callEntry, "callEntry not found\n");

            // save connection id of the incoming TCP connection
            callEntry->ingresConnectionId = openResult->connectionId;

            if (DEBUG)
            {
                char sourceIpAddr[MAX_STRING_LENGTH];
                char destIpAddr[MAX_STRING_LENGTH];
                char buf[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), buf);

                IO_ConvertIpAddressToString(
                    callEntry->localCallSignalTransAddr.ipAddr,
                    sourceIpAddr);

                IO_ConvertIpAddressToString(
                    callEntry->nextCallSignalDeliverAddr, destIpAddr);

                printf("Node %u initiates to open TCP connection at %s\n"
                       "  sourceAddr             = %s\n"
                       "  sourcePort             = %u\n"
                       "  destAddr               = %s\n\n",
                       node->nodeId,
                       buf,
                       sourceIpAddr,
                       callEntry->localCallSignalTransAddr.port,
                       destIpAddr);
            }

            h323->h323Stat.callForwarded++;

            // relaying TCP connection created
            Address localAddr;
            memset(&localAddr, 0, sizeof(Address));
            localAddr.networkType = NETWORK_IPV4;
            localAddr.interfaceAddr.ipv4 = 
                            callEntry->localCallSignalTransAddr.ipAddr;

            Address remoteAddr;
            memset(&remoteAddr, 0, sizeof(Address));
            remoteAddr.networkType = NETWORK_IPV4;
            remoteAddr.interfaceAddr.ipv4 = 
                                callEntry->nextCallSignalDeliverAddr;
            std::string url;
            url.clear();
            node->appData.appTrafficSender->appTcpOpenConnection(
                                    node,
                                    APP_H323,
                                    localAddr,
                                    callEntry->localCallSignalTransAddr.port,
                                    remoteAddr,
                                    (UInt16) APP_H323,
                                    callEntry->localCallSignalTransAddr.port,
                                    APP_DEFAULT_TOS,
                                    ANY_INTERFACE,
                                    url,
                                    0,
                                    ANY_NODEID,
                                    ANY_INTERFACE,
                                    ANY_INTERFACE);
            break;
        }
        default:
        {
            // invalid type
            ERROR_Assert(FALSE, "Invalid endpoint\n");
            break;
        }
    }
}


//
// NAME:        H323ReceiveOpenResult
//
// PURPOSE:     When TransOpenResult message is received by the H323 terminal
//
// PARAMETERS:  node:   node which is receiving open result
//              msg:    TCP message to the application
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323ReceiveOpenResult(Node* node, Message* msg)
{
    H225RasData* h225Ras;
    TransportToAppOpenResult* openResult;

    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    h225Ras = (H225RasData*) h323->h225Ras;

    openResult = (TransportToAppOpenResult *) MESSAGE_ReturnInfo(msg);

    // connection is still not established, fail status
    if (openResult->connectionId < 0)
    {
        h323->h323Stat.callRejected++;
    }
    else
    {
        if (DEBUG)
        {
            char buf[MAX_STRING_LENGTH];
            ctoa(node->getNodeTime(), buf);
            printf("Node %u at %s got OpenResult\n", node->nodeId, buf);
        }

        if (H323_DEBUG_DETAIL)
        {
            char localIpAddr[MAX_STRING_LENGTH];
            char remoteIpAddr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(&openResult->localAddr, localIpAddr);

            IO_ConvertIpAddressToString(
                &openResult->remoteAddr, remoteIpAddr);

            printf("  localAddr    = %s\n"
                   "  localPort    = %u\n"
                   "  remoteAddr   = %s\n"
                   "  remotePort   = %u\n"
                   "  connectionId = %d\n",
                   localIpAddr,
                   openResult->localPort,
                   remoteIpAddr,
                   openResult->remotePort,
                   openResult->connectionId);
        }

        // initiating terminal sending Setup message
        h323->connectionId = openResult->connectionId;
        //Create CircularBuffer List according to connectionId
        //For Each VOIP Connection Terminals has one CircularBuffer
        // & gatekeepers has two CircularBuffers.

        // initialize message buffer
        H323CreateCircularBufferInVector(node, h323);

        // TCP connection successful
        if (openResult->type == TCP_CONN_ACTIVE_OPEN)
        {
            if (DEBUG)
            {
                printf("openResult type is Active Open\n\n");
            }

            H323ProcessActiveOpenResult(node, h323, openResult);
        }
        else if (openResult->type == TCP_CONN_PASSIVE_OPEN)
        {
            if (DEBUG)
            {
                printf("openResult type is Passive Open\n\n");
            }

            H323ProcessPassiveOpenResult(node, h323, openResult);
        }
    }
}


//
// NAME:        H323HandleH225CallSignal
//
// PURPOSE:     Handling of H225 Call signalling
//
// PARAMETERS:  node:   node which is receiving H225 call signal
//              msg:    call setup message
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323HandleH225CallSignal(Node* node, Message* msg)
{
    Int32 packetSize;
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    CircularBuffer*  currentMsgBuffer = NULL;
    TransportToAppDataReceived* dataRecvd;
    Int32 indexInVector = -1;
    dataRecvd = (TransportToAppDataReceived *) MESSAGE_ReturnInfo(msg);

    currentMsgBuffer = H323GetCircularBufferFromVector(node,
                                     (unsigned short)dataRecvd->connectionId,
                                      h323,
                                      indexInVector);
    if (!currentMsgBuffer)
    {
        return;
    }

    if (!currentMsgBuffer->getCount(packetSize, idRead))
    {
        return;
    }

    unsigned char* localBuf = (unsigned char*) MEM_malloc (packetSize);
    memset(localBuf, 0, packetSize);

    if (currentMsgBuffer->read(localBuf) == FALSE)
    {
        MEM_free(localBuf);
        return;
    }

    Q931Message* packetRecvd = (Q931Message*) (localBuf);
    TransportToAppDataReceived* infoRecvd;
    infoRecvd = (TransportToAppDataReceived *) MESSAGE_ReturnInfo(msg);
    unsigned char* data;

    switch (Q931MessageGetMsgType(packetRecvd->msgFlgType))
    {
        case Q931_SETUP:
        {
            // Setup message received by receiver
            if (packetSize >= SIZE_OF_H225_SETUP)
            {
                data = (unsigned char*) MEM_malloc (SIZE_OF_H225_SETUP);
                memset(data, 0, SIZE_OF_H225_SETUP);
                currentMsgBuffer->readFromBuffer (data, SIZE_OF_H225_SETUP);

                H323HandleH225Setup(node, data, infoRecvd);

                // data will not be freed here, because this data may be
                // required in SETUP_TIMER, when processed properly by that
                // timer, it will be freed in the function
                // H323HandleH225SetupForGatekeeper
            }
            break;
        }
        case Q931_CALL_PROCEEDING:
        {
            // CallProceeding message received by initiator
            if (packetSize >= SIZE_OF_H225_CALL_PROCEEDING)
            {
                data = (unsigned char*)
                    MEM_malloc (SIZE_OF_H225_CALL_PROCEEDING);

                memset(data, 0, SIZE_OF_H225_CALL_PROCEEDING);
                currentMsgBuffer->readFromBuffer (data,
                                    SIZE_OF_H225_CALL_PROCEEDING);

                H323HandleH225CallProceeding(node, data, infoRecvd);
                MEM_free(data);
            }

            break;
        }
        case Q931_ALERTING:
        {
            // Alerting message received by initiator
            if (packetSize >= SIZE_OF_H225_ALERTING)
            {
                data = (unsigned char*)MEM_malloc (SIZE_OF_H225_ALERTING);
                memset(data, 0, SIZE_OF_H225_ALERTING);
                currentMsgBuffer->readFromBuffer(data, SIZE_OF_H225_ALERTING);

                H323HandleH225Alerting(node, data, infoRecvd);
                MEM_free(data);
            }

            break;
        }
        case Q931_CONNECT:
        {
            // Connect message received by initiator
            if (packetSize >= SIZE_OF_H225_CONNECT)
            {
                data = (unsigned char*) MEM_malloc (SIZE_OF_H225_CONNECT);
                memset(data, 0, SIZE_OF_H225_CONNECT);
                currentMsgBuffer->readFromBuffer(data, SIZE_OF_H225_CONNECT);

                H323HandleH225Connect(node, data, infoRecvd);
                MEM_free(data);
            }

            break;
        }
        case Q931_RELEASE_COMPLETE:
        {
            // ReleaseComplete message received by node
            if (packetSize >= SIZE_OF_H225_RELEASE_COMPLETE)
            {
                data = (unsigned char*)
                    MEM_malloc (SIZE_OF_H225_RELEASE_COMPLETE);

                memset(data, 0, SIZE_OF_H225_RELEASE_COMPLETE);
                currentMsgBuffer->readFromBuffer (data,
                                        SIZE_OF_H225_RELEASE_COMPLETE);

                H323HandleH225ReleaseComplete(node, data, infoRecvd);
                MEM_free(data);
            }

            break;
        }
        default:
        {
            // unknown message
            ERROR_Assert(FALSE, "Invalid message\n");
        }
    }
    MEM_free(localBuf);

    if (!currentMsgBuffer->getCount(packetSize, idRead))
    {
        return;
    }

    if (packetSize > 0)
    {
        H323HandleH225CallSignal(node, msg);
    }
}


//
// NAME:        H323ProcessCloseResultForTerminal
//
// PURPOSE:     The function handles TCP close result for terminal
//
// PARAMETERS:  node:           node which is receiving TCP close result
//              connectionId:   connection to be closed
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323ProcessCloseResultForTerminal(Node* node,
                            TransportToAppCloseResult* closeResult)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    VoipHostData* voip = NULL;

    if (DEBUG)
    {
        char buf[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), buf);
        printf("Node %u got close result at %s\n",
            node->nodeId, buf);
    }

    // find voip structure
    if (h323->hostState == H323_INITIATOR)
    {
        voip = VoipCallInitiatorGetDataForConnectionId(
                    node,
                    closeResult->connectionId);
    }
    else if (h323->hostState == H323_RECEIVER)
    {
        voip = VoipCallReceiverGetDataForConnectionId(
                    node,
                    closeResult->connectionId);
    }

    if (voip)
    {
        // make the connectino id invalid
        voip->connectionId = INVALID_ID;

        VoipCloseSession(node, voip);

        h323->callSignalState = H323_IDLE;
        h323->hostState = H323_NORMAL;
    }
}


//
// NAME:        H323ProcessCloseResultForGatekeeper
//
// PURPOSE:     The function handles TCP close result for gatekkeeper
//
// PARAMETERS:  node:           node which is receiving TCP close result
//              connectionId:   connection to be closed
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323ProcessCloseResultForGatekeeper(Node* node,
                            TransportToAppCloseResult* closeResult)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H225RasData* h225Ras = (H225RasData*) h323->h225Ras;

    // get opposite side connection ID for this close result receive
    int newConnectionId = H225RasSearchAndMarkCallEntry(
                                h225Ras,
                                closeResult->connectionId,
                                EGRES_TO_INGRES);

    if (newConnectionId == INVALID_ID)
    {
        newConnectionId = H225RasSearchAndMarkCallEntry(
                                h225Ras,
                                closeResult->connectionId,
                                INGRES_TO_EGRES);
    }

    if (newConnectionId != INVALID_ID)
    {
        if (DEBUG)
        {
            char clockStr[MAX_STRING_LENGTH];
            ctoa(node->getNodeTime(), clockStr);
            printf("Node %u receives closeResult at %s\n"
                "Initiate to close other end connection for Gatekeeper "
                "routed model\n", node->nodeId, clockStr);
        }

        // initiate TCP close
        APP_TcpCloseConnection(node, newConnectionId);

        // remove the call entry already marked for delete
        H225RasSearchAndRemoveCallEntry(
                            node,
                            h225Ras,
                            newConnectionId);
    }

}

//
// NAME:        H323ProcessCloseResult
//
// PURPOSE:
//
// PARAMETERS:  node:           node which is receiving TCP close result
//              connectionId:   connection to be closed
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323ProcessCloseResult(Node* node,
                            TransportToAppCloseResult* closeResult)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    switch (h323->endpointType)
    {
        case Terminal:
        {
            // handling CloseResult for terminal
            H323ProcessCloseResultForTerminal(node, closeResult);
            break;
        }
        case Gatekeeper:
        {
            // handling CloseResult for gatekeeper
            H323ProcessCloseResultForGatekeeper(node, closeResult);
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Invalid endpoint\n");
            break;
        }
    }
    if (closeResult->type == TCP_CONN_PASSIVE_CLOSE)
    {
        if (DEBUG)
        {
            printf("Node %u sends TCP close\n", node->nodeId);
        }
        APP_TcpCloseConnection(node, closeResult->connectionId);
    }
    H323DeleteCircularBuffer(node, h323,
        (unsigned short)closeResult->connectionId);
}


//
// NAME:        H323Layer
//
// PURPOSE:     main H323 message and event handling routine
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
void H323Layer(Node *node, Message *msg)
{
    switch (MESSAGE_GetEvent(msg))
    {
        case MSG_APP_H323_Connect_Timer:
        {
            // Alerting timer expired. now receiving terminal is ready
            // for connection establishment
            int* info = (int*)MESSAGE_ReturnInfo(msg);
            H323InitiateConnectMessage(node, *info);
            break;
        }

        case MSG_APP_H323_Call_Timeout:
        {
            // call timeout ewxpired. check whether connection is already
            // done. otherwise connectino is rejected
            int *info = (int*)MESSAGE_ReturnInfo(msg);
            H323CheckCallTimeout(node, *info);
            break;
        }

        case MSG_APP_FromTransOpenResult:
        {
            // TCP open result received by H323
            H323ReceiveOpenResult(node, msg);
            break;
        }

        case MSG_APP_FromTransDataSent:
        {
            // TCP data sent message received by H323
            if (DEBUG_TCP)
            {
                char buf[MAX_STRING_LENGTH];
                ctoa(node->getNodeTime(), buf);
                printf("Node %u sends data at %s\n", node->nodeId, buf);
            }
            break;
        }

        case MSG_APP_FromTransDataReceived:
        {
            // TCP data received message received by H323
            if (DEBUG_TCP)
            {
                char buf[MAX_STRING_LENGTH];
                ctoa(node->getNodeTime(), buf);
                printf("Node %u at %s received data\n", node->nodeId, buf);
            }

            H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

            Q931Message* pktRecd = (Q931Message*) MESSAGE_ReturnPacket(msg);
            int pktLen = MESSAGE_ReturnPacketSize(msg);

            CircularBuffer*  currentMsgBuffer = NULL;
            TransportToAppDataReceived* dataRecvd =
                (TransportToAppDataReceived *) MESSAGE_ReturnInfo(msg);

            Int32 indexInVector = -1;
            currentMsgBuffer = H323GetCircularBufferFromVector(node,
                                     (unsigned short)dataRecvd->connectionId,
                                      h323,
                                      indexInVector);
            if (currentMsgBuffer == NULL)
            {
                break;
            }

            currentMsgBuffer->write ((unsigned char*)pktRecd, pktLen);
            H323HandleH225CallSignal(node, msg);
            break;
        }

        case MSG_APP_FromTransCloseResult:
        {
            // TCP close connection message received by H323
            TransportToAppCloseResult* closeResult =
                (TransportToAppCloseResult *) MESSAGE_ReturnInfo(msg);

            H323ProcessCloseResult(node, closeResult);

            break;
        }

        case MSG_APP_FromTransListenResult:
        {
            // listen message
            break;
        }

        case MSG_APP_H225_SETUP_DELAY_Timer:
        {
            H225SetupDelayMsg* info =
                (H225SetupDelayMsg*) MESSAGE_ReturnInfo(msg);
            H323HandleH225SetupForGatekeeper(
                node,
                info->data,
                info->connectionId);

            break;
        }
        case MSG_APP_TimerExpired:
        {
            AppTimer *timer;

            timer = (AppTimer *) MESSAGE_ReturnInfo(msg);

            // now start listening at the updated address if
            // in valid address state
            Address serverAddress;
            if (AppTcpValidServerAddressState(node, msg, &serverAddress))
            {
                H323TerminalInit(node, serverAddress);
            }
            break;
        }
        default:
        {
#ifndef EXATA
            ERROR_Assert(FALSE, "Unknown message\n");
#endif
        }
    }
    MESSAGE_Free(node, msg);
}

//---------------------------------------------------------------------------
// NAME:        H323TerminalInit.
// PURPOSE:     listen on H323 port.
// PARAMETERS:
// + node       : Node*     : pointer to the node.
// + serverAddr : Address   : server address
// RETURN:      NONE
//---------------------------------------------------------------------------
void
H323TerminalInit(Node* node, Address serverAddr)
{
    APP_TcpServerListen(node,
                        APP_H323,
                        serverAddr,
                        (short) APP_H323);
}

//
// NAME:        H323StatsInit
//
// PURPOSE:
//
// PARAMETERS:  node, node which is message/event appears
//              msg,  the message it has received
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
static
void H323StatsInit(Node* node, const NodeInput* nodeInput, H323Data* h323)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];

    IO_ReadString(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "VOIP-SIGNALLING-STATISTICS",
                  &retVal,
                  buf);

    if (retVal == TRUE)
    {
        if (strcmp(buf, "YES") == 0)
        {
            h323->isH323StatEnabled = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            h323->isH323StatEnabled = FALSE;
        }
        else
        {
            /* invalid entry in config */
            char errorBuf[MAX_STRING_LENGTH];
            sprintf(errorBuf,
                    "VOIP-SIGNALLING-STATISTICS unknown setting %s\n", buf);
            ERROR_ReportError(errorBuf);
        }
    }
    else        // Assume FALSE if not specified
    {
        h323->isH323StatEnabled = FALSE;
    }

    // initialize statistics
    memset(&(h323->h323Stat), 0, sizeof(H323Stat));
}


//-------------------------------------------------------------------------
// NAME:        H323GatekeeperSetup.
//
// PURPOSE:     Handling all Gatekeeper initializations
//              needed for H225Ras
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//-------------------------------------------------------------------------
static
void H323GatekeeperSetup(
    //modified for removing gatekeeper list
    Node* node,
    const NodeInput* nodeInput,
    H323CallModel callModel)
{
    //modified for removing gatekeeper list
    // IdToNodePtrMap* nodeHash = firstNode->partitionData->nodeIdHash;
    // Node* node = MAPPING_GetNodePtrFromHash(nodeHash, nodeId);
    int i;

    if (node != NULL)
    {
        // Allocate and init state.
        H323Data* h323 = (H323Data*) MEM_malloc(sizeof(H323Data));

        memset(h323, 0, sizeof(H323Data));

        h323->msgBufferVector = new CircularBufferVector;

        h323->gatekeeperAvailable = TRUE;
        h323->endpointType = Gatekeeper;

        h323->callModel = callModel;

        // Set pointer in node data structure to H323 state.
        node->appData.multimedia->sigPtr = (void *) h323;

        h323->h225Terminal = NULL;

        H323StatsInit(node, nodeInput, h323);

        H225RasGatekeeperInit(node, h323);

        if (h323->callModel == GatekeeperRouted)
        {
            for (i = 0; i < node->numberInterfaces; i++)
            {
                APP_TcpServerListen(node, APP_H323,
                        NetworkIpGetInterfaceAddress(node, i),
                        (short) APP_H323);
            }
        }
    }
}


//-------------------------------------------------------------------------
// NAME:        H323GatekeeperInit
//
// PURPOSE:     read input paramters of GatekeeperList from config file.
//
// PARAMETERS:  node:       Node for which state is required
//              nodeInput:  Node input
//
// RETURN:      BOOL
//
//
// ASSUMPTION:  None.
//-------------------------------------------------------------------------
BOOL H323GatekeeperInit(Node* firstNode,
                        const NodeInput* nodeInput,
                        H323CallModel callModel)
{
    BOOL gatekeeperFound = FALSE;
    //modified for removing gatekeeper list
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    IO_ReadString(
                firstNode->nodeId,
                ANY_ADDRESS,
                nodeInput,
                "H323-GATEKEEPER",
                &retVal,
                buf);
    if (retVal && !strcmp(buf,"YES"))
    {
        BOOL gatekeeperDhcpClient = FALSE;

        // Checking gatekeeper for DHCP client
        gatekeeperDhcpClient = H323IsDHCPClient( firstNode , nodeInput );

        if (gatekeeperDhcpClient)
        {
           ERROR_ReportError("\nH.323 Gatekeeper Can't be DHCP Client");
        }
        else
        {
            H323GatekeeperSetup(
                            firstNode,
                            nodeInput,
                            callModel);
            gatekeeperFound = TRUE;
        }
    }
    if (retVal && strcmp(buf,"YES") && strcmp(buf,"NO"))
    {
        ERROR_ReportError("\nValue for H323-GATEKEEPER should be YES/NO");
    }
    return gatekeeperFound;
}


//
// NAME:        H323Init.
//
// PURPOSE:     Handling all initializations needed for Bgp.
//              initializing the internal structure
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//
void H323Init(Node* node,
              const NodeInput *nodeInput,
              BOOL gatekeeperFound,
              H323CallModel callModel,
              unsigned short callSignalPort,
              unsigned short srcPort,
              clocktype waitTime,
              NodeId nodeId)
{
    int i;
    Address homeUnicastAdr;
    memset(&homeUnicastAdr, 0, sizeof(Address));

    AppMultimedia* multimedia = node->appData.multimedia;
    char ownAliasAddr[MAX_STRING_LENGTH];
    Address h323Addr;
    memset(&h323Addr, 0, sizeof(Address));

    H323ReadEndpointList(
            node->partitionData,
            node->nodeId,
            nodeInput,
            ownAliasAddr);

    if (gatekeeperFound)
    {
        if ((node->appData.multimedia->sigPtr != NULL) &&
            (((H323Data*)(node->appData.multimedia->sigPtr))->endpointType
                == Gatekeeper))
        {
            char errorMsg[MAX_STRING_LENGTH];

            sprintf(errorMsg,
                    "Gatekeeper should not be receiver %u\n",
                    node->nodeId);
            ERROR_ReportError(errorMsg);
        }
    }

    // Allocate and init state.
    H323Data* h323 = (H323Data*) MEM_malloc(sizeof(H323Data));


    H225TerminalData* h225Terminal =
        (H225TerminalData*) MEM_malloc (sizeof (H225TerminalData));

    memset(h323, 0, sizeof(H323Data));
    h323->msgBufferVector = new CircularBufferVector;

    h323->h225Terminal = (H225TerminalData *) h225Terminal;

    // Set pointer in node data structure to H323 state.
    multimedia->sigPtr = (void *) h323;

    multimedia->initiateFunction  = &H323InitiateConnection;
    multimedia->terminateFunction = &H323TerminateConnection;

    multimedia->hostCallingFunction = &H323IsHostInitiator;
    multimedia->hostCalledFunction  = &H323IsHostReceiver;

    // initially all the nodes are in IDLE state
    h323->callSignalState = H323_IDLE;

    // initially node is not in initiator or receiver mode
    h323->hostState = H323_NORMAL;

    h323->gatekeeperAvailable = gatekeeperFound;
    h323->endpointType = Terminal;

    h323->callModel = callModel;

    // initially connectionId is NULL
    h323->connectionId = 0;

    homeUnicastAdr.networkType = NETWORK_IPV4;
    homeUnicastAdr.interfaceAddr.ipv4 = 
                NetworkIpGetInterfaceAddress(node, DEFAULT_INTERFACE);

    h225Terminal->terminalCallSignalTransAddr.ipAddr = 
                                    homeUnicastAdr.interfaceAddr.ipv4;

    h225Terminal->terminalCallSignalTransAddr.port = callSignalPort;

    strcpy(h225Terminal->terminalAliasAddr, ownAliasAddr);

    H323StatsInit(node, nodeInput, h323);

    // initialize RAS service if required
    if (h323->gatekeeperAvailable)
    {
        H225RasTerminalInit(node,
                            nodeInput,
                            h323,
                            homeUnicastAdr,
                            srcPort
                            );
    }

    for (i = 0; i < node->numberInterfaces; i++)
    {
        // Dynamic address
        // Delay H323 Terminal listening by start time
        // such that server may acquire an address by that time
        // to ensure that server listens earlier than TCP
        // open; therefore listen timer should be set at unit 
        // time (1 ns) before Open
        clocktype listenTime =  waitTime - 1 * NANO_SECOND;
        h323Addr.networkType = NETWORK_IPV4;
        h323Addr.interfaceAddr.ipv4 = NetworkIpGetInterfaceAddress(node, i);
        AppStartTcpAppSessionServerListenTimer(
                                            node,
                                            APP_H323,
                                            h323Addr,
                                            NULL,
                                            listenTime);

    }
}


//
// NAME:        H323PrintTerminalStat.
//
// PURPOSE:
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//
static
void H323PrintTerminalStat(Node* node)
{
    char buf[MAX_STRING_LENGTH];
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;
    H323Stat h323Stat = h323->h323Stat;

    sprintf(buf, "Number Of Calls Initiated = %d",
            h323Stat.callInitiated);

    IO_PrintStat(
        node,
        "Application",
        "H323",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of Calls Received = %d",
            h323Stat.callReceived);

    IO_PrintStat(
        node,
        "Application",
        "H323",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of Calls Established = %d",
            h323Stat.connectionEstablished);

    IO_PrintStat(
        node,
        "Application",
        "H323",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of TCP Connections Rejected = %d",
            h323Stat.callRejected);

    IO_PrintStat(
        node,
        "Application",
        "H323",
        ANY_DEST,
        -1 /* instance Id */,
        buf);
}


//
// NAME:        H323PrintGatekeeperStat.
//
// PURPOSE:
//
// PARAMETERS:  node,      Node which is to be initialized.
//              nodeInput, The input file
//
// RETURN:      None.
//
// ASSUMPTION:
//
static
void H323PrintGatekeeperStat(Node* node)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    if (h323->callModel == GatekeeperRouted)
    {
        char buf[MAX_STRING_LENGTH];
        H323Stat h323Stat = h323->h323Stat;

        sprintf(buf, "Number Of Calls Forwarded = %d",
                h323Stat.callForwarded);

        IO_PrintStat(
            node,
            "Application",
            "H323",
            ANY_DEST,
            -1 /* instance Id */,
            buf);
    }
}


//
// NAME:        H323Finalize
//
// PURPOSE:     Printing statistical information for bgp
//
// PARAMETERS:  node, node which is printing the statistical information
//
// RETURN:      None.
//
// ASSUMPTION:  None.
//
void H323Finalize(Node *node)
{
    H323Data* h323 = (H323Data*) node->appData.multimedia->sigPtr;

    //Delete CircularBuffer Vector
    //Uncommented following line if we need delete the buffer
    //at the end of simulation

    //H323DeleteCircularBufferVector(node);
    if (h323->isH323StatEnabled)
    {
        if (h323->h225Ras)
        {
            H225RasFinalize(node);
        }

        if (h323->endpointType == Terminal)
        {
            H323PrintTerminalStat(node);
        }
        else
        {
            H323PrintGatekeeperStat(node);
        }
    }

    if (h323->msgBufferVector != NULL)
    {
        delete h323->msgBufferVector;
        h323->msgBufferVector = NULL;
    }
}
