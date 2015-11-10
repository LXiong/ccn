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
#include <string>

#include "api.h"
#include "partition.h"
#include "app_util.h"
#include "app_zigbeeApp.h"


// /**
// FUNCTION   :: ZigbeeAppInitTrace
// LAYER      :: APPLCATION
// PURPOSE    :: ZigbeeApp initialization  for tracing
// PARAMETERS ::
// + node : Node* : Pointer to node
// + nodeInput    : const NodeInput* : Pointer to NodeInput
// RETURN ::  void : NULL
// **/


void ZigbeeAppInitTrace(Node* node, const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    BOOL traceAll = TRACE_IsTraceAll(node);
    BOOL trace = FALSE;
    BOOL writeMap = TRUE;

    IO_ReadString(node->nodeId,
                 ANY_ADDRESS,
                 nodeInput,
                 "TRACE-ZIGBEEAPP",
                 &retVal,
                 buf);

    if (retVal)
    {
        if (strcmp(buf, "YES") == 0)
        {
            trace = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            trace = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "TRACE-ZIGBEEAPP should be either \"YES\" or \"NO\".\n");
        }
    }
    else
    {
        if (traceAll || node->traceData->layer[TRACE_APPLICATION_LAYER])
        {
            trace = TRUE;
        }
    }

    if (trace)
    {
            TRACE_EnableTraceXML(node,
                                TRACE_ZIGBEEAPP,
                                "ZIGBEEAPP",
                                ZigbeeAppPrintTraceXML,
                                writeMap);
    }
    else
    {
            TRACE_DisableTraceXML(node,
                                  TRACE_ZIGBEEAPP,
                                  "ZIGBEEAPP",
                                  writeMap);
    }
    writeMap = FALSE;
}


 // **
// FUNCTION   :: ZigbeeAppPrintTraceXML
// LAYER      :: NETWORK
// PURPOSE    :: Print packet trace information in XML format
// PARAMETERS ::
// + node     : Node*    : Pointer to node
// + msg      : Message* : Pointer to packet to print headers from
// RETURN     ::  void   : NULL
// **/

void ZigbeeAppPrintTraceXML(Node* node, Message* msg)
{
    char buf[MAX_STRING_LENGTH];
    char clockStr[MAX_STRING_LENGTH];

    ZigbeeAppData* data = (ZigbeeAppData*) MESSAGE_ReturnPacket(msg);

    if (data == NULL)
    {
        return;
    }

    TIME_PrintClockInSecond(data->txTime, clockStr);

    sprintf(buf, "<zigbeeApp>%u %c %u %d %s</zigbeeApp>",
            data->sourcePort,
            data->type,
            data->seqNo,
            (msg->packetSize + msg->virtualPayloadSize),
            clockStr);
    TRACE_WriteToBufferXML(node, buf);
}

/*
 * NOTE: ZIGBEEAPP does not attempt to reorder packets.  Any out of order
 *       packets will be dropped.
 */

/*
 * NAME:        ZigbeeAppClient
 * PURPOSE:     Models the behaviour of zigbeeAppClient Client on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  node - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
ZigbeeAppClient(Node* node, Message* msg)
{
    char buf[MAX_STRING_LENGTH];
    char error[MAX_STRING_LENGTH];
    AppDataZigbeeappClient* clientPtr;

#ifdef DEBUG
    TIME_PrintClockInSecond(node->getNodeTime(), buf);
    printf("ZIGBEEAPP Client: Node %ld got a message at %sS\n",
           node->nodeId, buf);
#endif /* DEBUG */

    switch (msg->eventType)
    {
        case MSG_APP_TimerExpired:
        {
            AppTimer* timer;

            timer = (AppTimer *) MESSAGE_ReturnInfo(msg);

#ifdef DEBUG
            printf("ZIGBEEAPP Client: Node %ld at %s got timer %d\n",
                   node->nodeId, buf, timer->type);
#endif /* DEBUG */

            clientPtr = ZigbeeAppClientGetClient(node, timer->sourcePort);

            if (clientPtr == NULL)
            {
                sprintf(error, "ZigbeeApp Client: Node %d cannot find zigbeeApp"
                    " client\n", node->nodeId);

                ERROR_ReportError(error);
            }

            switch (timer->type)
            {
                case APP_TIMER_SEND_PKT:
                {
                    ZigbeeAppData data;
                    //char ZigbeeAppData[ZIGBEEAPP_HEADER_SIZE];

                    memset(&data, 0, sizeof(data));

                    ERROR_Assert(sizeof(data) <= ZIGBEEAPP_HEADER_SIZE,
                                 "ZigbeeAppData size cant be greater than"
                                 " ZIGBEEAPP_HEADER_SIZE");

#ifdef DEBUG
                    printf("ZIGBEEAPP Client: Node %u has %u items left to"
                        " send\n", node->nodeId, clientPtr->itemsToSend);
#endif /* DEBUG */

                    if (((clientPtr->itemsToSend > 1)
                        && (node->getNodeTime() + clientPtr->interval
                            < clientPtr->endTime))
                        || ((clientPtr->itemsToSend == 0)
                        && (node->getNodeTime() + clientPtr->interval
                            < clientPtr->endTime))
                        || ((clientPtr->itemsToSend > 1)
                        &&  (clientPtr->endTime == 0))
                        || ((clientPtr->itemsToSend == 0)
                        &&  (clientPtr->endTime == 0)))
                    {
                        data.type = 'd';
                    }
                    else
                    {
                        data.type = 'c';
                        clientPtr->sessionIsClosed = TRUE;
                        clientPtr->sessionFinish = node->getNodeTime();
                    }

                    data.sourcePort = clientPtr->sourcePort;
                    data.txTime = node->getNodeTime();
                    data.seqNo = clientPtr->seqNo++;

#ifdef DEBUG
                    {
                        char clockStr[MAX_STRING_LENGTH];
                        char addrStr[MAX_STRING_LENGTH];

                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr, node);
                        IO_ConvertIpAddressToString(
                            &clientPtr->remoteAddr, addrStr);

                        printf("ZIGBEEAPP Client: node %ld sending data packet"
                               " at time %sS to ZIGBEEAPP server %s\n",
                               node->nodeId, clockStr, addrStr);
                        printf("    size of payload is %d\n",
                               clientPtr->itemSize);
                    }
#endif /* DEBUG */
                    // Note: An overloaded Function
                    //memset(ZigbeeAppData, 0, ZIGBEEAPP_HEADER_SIZE);
                    //memcpy(ZigbeeAppData, (char *) &data, sizeof(data));

                    clocktype zigbeeAppEndTime = 0;
                    if (clientPtr->endTime == 0 && clientPtr->itemsToSend == 0)
                    {
                            // We cant predict the ZIGBEEAPP end time
                            // end time will be simulation time

                            zigbeeAppEndTime = TIME_ReturnMaxSimClock(node);
                    }
                    else if (clientPtr->endTime == 0)
                    {
                        zigbeeAppEndTime = clientPtr->sessionStart
                            + ((clientPtr->itemsToSend 
                                + clientPtr->numPktsSent)
                                    * clientPtr->interval);
                    }
                    else
                    {
                        zigbeeAppEndTime = clientPtr->endTime;
                    }

                    Message *msg = APP_UdpCreateMessage(
                        node,
                        clientPtr->localAddr,
                        (short) clientPtr->sourcePort,
                        clientPtr->remoteAddr,
                        (short) APP_ZIGBEEAPP_SERVER,
                        TRACE_ZIGBEEAPP,
                        clientPtr->tos);

                    APP_AddHeader(
                        node,
                        msg,
                        &data,
                        ZIGBEEAPP_HEADER_SIZE);

                    ZigbeeAppInfo zainfo = ZigbeeAppInfo();
                    zainfo.priority = clientPtr->tos;
                    zainfo.endTime = zigbeeAppEndTime;
                    zainfo.zigbeeAppInterval = clientPtr->interval;
                    zainfo.zigbeeAppPktSize = clientPtr->itemSize;
                    zainfo.srcAddress = 
                        clientPtr->localAddr.interfaceAddr.ipv4;
                    zainfo.srcPort = clientPtr->sourcePort;
                    APP_AddInfo(
                        node,
                        msg,
                        &zainfo,
                        sizeof(ZigbeeAppInfo),
                        INFO_TYPE_ZigbeeApp_Info);

                    int payloadSize = 
                        clientPtr->itemSize - ZIGBEEAPP_HEADER_SIZE;

                    APP_AddVirtualPayload(node, msg, payloadSize);

                    APP_AddInfo(
                        node,
                        msg,
                        &payloadSize,
                        sizeof(int),
                        INFO_TYPE_DataSize);

                    APP_UdpSend(node, msg);

                    clientPtr->numBytesSent += clientPtr->itemSize;
                    clientPtr->numPktsSent++;
                    clientPtr->sessionLastSent = node->getNodeTime();

                    if (clientPtr->itemsToSend > 0)
                    {
                        clientPtr->itemsToSend--;
                    }

                    if (clientPtr->sessionIsClosed == FALSE)
                    {
                        ZigbeeAppClientScheduleNextPkt(node, clientPtr);
                    }

                    break;
                }

                default:
                {
#ifndef EXATA
                    assert(FALSE);
#endif
                }
            }
            break;
        }
        default:
           TIME_PrintClockInSecond(node->getNodeTime(), buf, node);
           sprintf(error, "ZIGBEEAPP Client: at time %sS, node %d "
                   "received message of unknown type"
                   " %d\n", buf, node->nodeId, msg->eventType);
#ifndef EXATA
           ERROR_ReportError(error);
#endif
    }

    MESSAGE_Free(node, msg);
}



/*
 * NAME:        ZigbeeAppClientInit.
 * PURPOSE:     Initialize a zigbeeAppClient session.
 * PARAMETERS:  node - pointer to the node,
 *              serverAddr - address of the server,
 *              itemsToSend - number of items to send,
 *              itemSize - size of each packet,
 *              interval - interval of packet transmission rate.
 *              startTime - time until the session starts,
 *              endTime - time until the session ends,
 *              tos - the contents for the type of service field.
 * RETURN:      none.
 */
void
ZigbeeAppClientInit(Node* node,
                    Address clientAddr,
                    Address serverAddr,
                    Int32 itemsToSend,
                    Int32 itemSize,
                    clocktype interval,
                    clocktype startTime,
                    clocktype endTime,
                    unsigned tos)
{
    char error[MAX_STRING_LENGTH];
    AppTimer* timer = NULL;
    AppDataZigbeeappClient* clientPtr = NULL;
    Message* timerMsg = NULL;
    Int32 minSize = 0;
    startTime -= getSimStartTime(node);
    endTime -= getSimStartTime(node);

    ERROR_Assert(sizeof(ZigbeeAppData) <= ZIGBEEAPP_HEADER_SIZE,
                 "ZigbeeAppData size cant be greater than ZIGBEEAPP_HEADER_SIZE");
    minSize = ZIGBEEAPP_HEADER_SIZE;

    // Check to make sure the number of zigbeeApp items is a correct value.
    if (itemsToSend < 0)
    {
        sprintf(error, "ZIGBEEAPP Client: Node %d item to sends needs"
            " to be >= 0\n", node->nodeId);

        ERROR_ReportError(error);
    }

    // Make sure that packet is big enough to carry zigbeeApp data information.
    if (itemSize < minSize)
    {
        sprintf(error, "ZIGBEEAPP Client: Node %d item size needs to be >= %d.\n",
                node->nodeId, minSize);
        ERROR_ReportError(error);
    }


    // Make sure that packet is within max limit.
    if (itemSize > APP_MAX_DATA_SIZE)
    {
        sprintf(error, "ZIGBEEAPP Client: Node %d item size needs to be <= %d.\n",
                node->nodeId, APP_MAX_DATA_SIZE);
        ERROR_ReportError(error);
    }

    // Make sure interval is valid.
    if (interval <= 0)
    {
        sprintf(error, "ZIGBEEAPP Client: Node %d interval needs to be > 0.\n",
                node->nodeId);
        ERROR_ReportError(error);
    }

    // Make sure start time is valid. 
    if (startTime < 0)
    {
        sprintf(error, "ZIGBEEAPP Client: Node %d start time needs to be >= 0.\n",
                node->nodeId);
        ERROR_ReportError(error);
    }

    // Check to make sure the end time is a correct value.
    if (!((endTime > startTime) || (endTime == 0)))
    {
        sprintf(error, "ZIGBEEAPP Client: Node %d end time needs to be > "
            "start time or equal to 0.\n", node->nodeId);

        ERROR_ReportError(error);
    }

    // Validate the given tos for this application.
    if (/*tos < 0 || */tos > 255)
    {
        sprintf(error, "ZIGBEEAPP Client: Node %d should have tos value "
            "within the range 0 to 255.\n",
            node->nodeId);
        ERROR_ReportError(error);
    }

    clientPtr = ZigbeeAppClientNewClient(node,
                                         clientAddr,
                                         serverAddr,
                                         itemsToSend,
                                         itemSize,
                                         interval,
                                         startTime,
                                         endTime,
                                         (TosType) tos);

    if (clientPtr == NULL)
    {
        sprintf(error,
                "ZIGBEEAPP Client: Node %d cannot allocate memory for "
                    "new client\n", node->nodeId);

        ERROR_ReportError(error);
    }

    timerMsg = MESSAGE_Alloc(node,
                             APP_LAYER,
                             APP_ZIGBEEAPP_CLIENT,
                             MSG_APP_TimerExpired);

    MESSAGE_InfoAlloc(node, timerMsg, sizeof(AppTimer));

    timer = (AppTimer *)MESSAGE_ReturnInfo(timerMsg);

    timer->sourcePort = clientPtr->sourcePort;
    timer->type = APP_TIMER_SEND_PKT;

#ifdef DEBUG
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(startTime, clockStr, node);
        printf("ZIGBEEAPP Client: Node %ld starting client at %sS\n",
               node->nodeId, clockStr);
    }
#endif /* DEBUG */

    MESSAGE_Send(node, timerMsg, startTime);
}

/*
 * NAME:        ZigbeeAppClientPrintStats.
 * PURPOSE:     Prints statistics of a zigbeeAppClient session.
 * PARAMETERS:  node - pointer to the this node.
 *              clientPtr - pointer to the zigbeeApp client data structure.
 * RETURN:      none.
 */
void ZigbeeAppClientPrintStats(Node* node,
                               AppDataZigbeeappClient* clientPtr)
{
    clocktype throughput = 0;
    char addrStr[MAX_STRING_LENGTH];
    char startStr[MAX_STRING_LENGTH];
    char closeStr[MAX_STRING_LENGTH];
    char sessionStatusStr[MAX_STRING_LENGTH];
    char throughputStr[MAX_STRING_LENGTH];

    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(clientPtr->sessionStart, startStr, node);
    TIME_PrintClockInSecond(clientPtr->sessionLastSent, closeStr, node);

    if (clientPtr->sessionIsClosed == FALSE)
    {
        clientPtr->sessionFinish = node->getNodeTime();
        sprintf(sessionStatusStr, "Not closed");
    }
    else
    {
        sprintf(sessionStatusStr, "Closed");
    }

    if (clientPtr->sessionFinish <= clientPtr->sessionStart)
    {
        throughput = 0;
    }
    else
    {
        throughput = (clocktype)
                     ((clientPtr->numBytesSent * 8.0 * SECOND)
                      / (clientPtr->sessionFinish
                      - clientPtr->sessionStart));
    }

    ctoa(throughput, throughputStr);

    if (clientPtr->remoteAddr.networkType == NETWORK_ATM)
    {
        const LogicalSubnet* dstLogicalSubnet =
            AtmGetLogicalSubnetFromNodeId(
            node,
            clientPtr->remoteAddr.interfaceAddr.atm.ESI_pt1,
            0);
        IO_ConvertIpAddressToString(
            dstLogicalSubnet->ipAddress,
            addrStr);
    }
    else
    {
        IO_ConvertIpAddressToString(&clientPtr->remoteAddr, addrStr);

    }

    sprintf(buf, "Server Address = %s", addrStr);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Client",
                 ANY_DEST,
                 clientPtr->sourcePort,
                 buf);

    sprintf(buf, "First Packet Sent at (s) = %s", startStr);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Client",
                 ANY_DEST,
                 clientPtr->sourcePort,
                 buf);

    sprintf(buf, "Last Packet Sent at (s) = %s", closeStr);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Client",
                 ANY_DEST,
                 clientPtr->sourcePort,
                 buf);

    sprintf(buf, "Session Status = %s", sessionStatusStr);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Client",
                 ANY_DEST,
                 clientPtr->sourcePort,
                 buf);

    ctoa((Int64) clientPtr->numBytesSent, buf1);
    sprintf(buf, "Total Bytes Sent = %s", buf1);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Client",
                 ANY_DEST,
                 clientPtr->sourcePort,
                 buf);

    sprintf(buf, "Total Packets Sent = %u", clientPtr->numPktsSent);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Client",
                 ANY_DEST,
                 clientPtr->sourcePort,
                 buf);

    sprintf(buf, "Throughput (bits/s) = %s", throughputStr);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Client",
                 ANY_DEST,
                 clientPtr->sourcePort,
                 buf);
}

/*
 * NAME:        ZigbeeAppClientFinalize.
 * PURPOSE:     Collect statistics of a zigbeeAppClient session.
 * PARAMETERS:  node - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
ZigbeeAppClientFinalize(Node* node, AppInfo* appInfo)
{
    AppDataZigbeeappClient* clientPtr
        = (AppDataZigbeeappClient*)appInfo->appDetail;

    if (node->appData.appStats == TRUE)
    {
        ZigbeeAppClientPrintStats(node, clientPtr);
    }
}

/*
 * NAME:        ZigbeeAppClientGetClient.
 * PURPOSE:     search for a ZIGBEEAPP client data structure.
 * PARAMETERS:  node - pointer to the node.
 *              sourcePort - source port of the zigbeeApp client.
 * RETURN:      the pointer to the zigbeeApp client data structure,
 *              NULL if nothing found.
 */
AppDataZigbeeappClient*
ZigbeeAppClientGetClient(Node* node, short sourcePort)
{
    AppInfo* appList = node->appData.appPtr;
    AppDataZigbeeappClient* zigbeeAppClient;

    for (; appList != NULL; appList = appList->appNext)
    {
        if (appList->appType == APP_ZIGBEEAPP_CLIENT)
        {
            zigbeeAppClient = (AppDataZigbeeappClient *) appList->appDetail;

            if (zigbeeAppClient->sourcePort == sourcePort)
            {
                return zigbeeAppClient;
            }
        }
    }

    return NULL;
}

/*
 * NAME:        ZigbeeAppClientNewClient.
 * PURPOSE:     create a new zigbeeApp client data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  node - pointer to the node.
 *              remoteAddr - remote address.
 *              itemsToSend - number of zigbeeApp items to send in simulation.
 *              itemSize - size of each packet.
 *              interval - time between two packets.
 *              startTime - when the node will start sending.
 * RETURN:      the pointer to the created zigbeeApp client data structure,
 *              NULL if no data structure allocated.
 */
AppDataZigbeeappClient*
ZigbeeAppClientNewClient(Node* node,
                         const Address &localAddr,
                         const Address &remoteAddr,
                         Int32 itemsToSend,
                         Int32 itemSize,
                         clocktype interval,
                         clocktype startTime,
                         clocktype endTime,
                         TosType tos)
{
    AppDataZigbeeappClient* zigbeeAppClient;

    zigbeeAppClient = (AppDataZigbeeappClient *)
                MEM_malloc(sizeof(AppDataZigbeeappClient));
    memset(zigbeeAppClient, 0, sizeof(AppDataZigbeeappClient));

    // fill in zigbeeApp info.
    memcpy(&(zigbeeAppClient->localAddr), &localAddr, sizeof(Address));
    memcpy(&(zigbeeAppClient->remoteAddr), &remoteAddr, sizeof(Address));
    zigbeeAppClient->interval = interval;
    zigbeeAppClient->sessionStart = node->getNodeTime() + startTime;
    zigbeeAppClient->sessionIsClosed = FALSE;
    zigbeeAppClient->sessionLastSent = node->getNodeTime();
    zigbeeAppClient->sessionFinish = node->getNodeTime();
    zigbeeAppClient->endTime = endTime;
    zigbeeAppClient->numBytesSent = 0;
    zigbeeAppClient->numPktsSent = 0;
    zigbeeAppClient->itemsToSend = itemsToSend;
    zigbeeAppClient->itemSize = itemSize;
    zigbeeAppClient->sourcePort = node->appData.nextPortNum++;
    zigbeeAppClient->seqNo = 0;
    zigbeeAppClient->tos = tos;

    // Add ZIGBEEAPP variables to hierarchy
    std::string path;
    D_Hierarchy* h = &node->partitionData->dynamicHierarchy;

    if (h->CreateApplicationPath(node,                        // node
                                "zigbeeAppClient",            // protocol name
                                zigbeeAppClient->sourcePort,  // port
                                "interval",                   // object name
                                path))                        // path (output)
    {
        h->AddObject(path,
                     new D_ClocktypeObj(&zigbeeAppClient->interval));
    }

// The HUMAN_IN_THE_LOOP_DEMO is part of a gui user-defined command
// demo.
// The type of service value for this ZIGBEEAPP application is added to
// the dynamic hierarchy so that the user-defined-command can change
// it during simulation.

#ifdef HUMAN_IN_THE_LOOP_DEMO
    if (h->CreateApplicationPath(node,
                                "zigbeeAppClient",
                                zigbeeAppClient->sourcePort,
                                "tos",                  // object name
                                path))                  // path (output)
    {
        h->AddObject(path,
                     new D_UInt32Obj(&zigbeeAppClient->tos));
    }
#endif

    if (h->CreateApplicationPath(node,
                                 "zigbeeAppClient",
                                 zigbeeAppClient->sourcePort,
                                 "numBytesSent",
                                 path))
    {
        h->AddObject(path,
                     new D_Int64Obj(&zigbeeAppClient->numBytesSent));
    }

#ifdef DEBUG
    {
        char clockStr[MAX_STRING_LENGTH];
        char localAddrStr[MAX_STRING_LENGTH];
        char remoteAddrStr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(&zigbeeAppClient->localAddr, localAddrStr);
        IO_ConvertIpAddressToString(&zigbeeAppClient->remoteAddr, remoteAddrStr);

        printf("ZIGBEEAPP Client: Node %u created new ZIGBEEAPP client structure\n",
               node->nodeId);
        printf("    localAddr = %s\n", localAddrStr);
        printf("    remoteAddr = %s\n", remoteAddrStr);
        TIME_PrintClockInSecond(zigbeeAppClient->interval, clockStr);
        printf("    interval = %s\n", clockStr);
        TIME_PrintClockInSecond(zigbeeAppClient->sessionStart, clockStr, node);
        printf("    sessionStart = %sS\n", clockStr);
        printf("    numBytesSent = %u\n", zigbeeAppClient->numBytesSent);
        printf("    numPktsSent = %u\n", zigbeeAppClient->numPktsSent);
        printf("    itemsToSend = %u\n", zigbeeAppClient->itemsToSend);
        printf("    itemSize = %u\n", zigbeeAppClient->itemSize);
        printf("    sourcePort = %ld\n", zigbeeAppClient->sourcePort);
        printf("    seqNo = %ld\n", zigbeeAppClient->seqNo);
    }
#endif /* DEBUG */

    APP_RegisterNewApp(node, APP_ZIGBEEAPP_CLIENT, zigbeeAppClient);
    return zigbeeAppClient;
}

/*
 * NAME:        ZigbeeAppClientScheduleNextPkt.
 * PURPOSE:     schedule the next packet the client will send.  If next packet
 *              won't arrive until the finish deadline, schedule a close.
 * PARAMETERS:  node - pointer to the node,
 *              clientPtr - pointer to the zigbeeApp client data structure.
 * RETRUN:      none.
 */
void
ZigbeeAppClientScheduleNextPkt(Node* node, AppDataZigbeeappClient* clientPtr)
{
    AppTimer* timer = NULL;
    Message* timerMsg = NULL;

    timerMsg = MESSAGE_Alloc(node,
                             APP_LAYER,
                             APP_ZIGBEEAPP_CLIENT,
                             MSG_APP_TimerExpired);

    MESSAGE_InfoAlloc(node, timerMsg, sizeof(AppTimer));

    timer = (AppTimer*) MESSAGE_ReturnInfo(timerMsg);
    timer->sourcePort = clientPtr->sourcePort;
    timer->type = APP_TIMER_SEND_PKT;

#ifdef DEBUG
    {
        char clockStr[24];
        printf("ZIGBEEAPP Client: Node %u scheduling next data packet\n",
               node->nodeId);
        printf("    timer type is %d\n", timer->type);
        TIME_PrintClockInSecond(clientPtr->interval, clockStr);
        printf("    interval is %sS\n", clockStr);
    }
#endif /* DEBUG */

    MESSAGE_Send(node, timerMsg, clientPtr->interval);
}

/*
 * NAME:        ZigbeeAppServer.
 * PURPOSE:     Models the behaviour of ZigbeeAppServer Server on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  node - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
ZigbeeAppServer(Node* node, Message* msg)
{
    char error[MAX_STRING_LENGTH];
    AppDataZigbeeappServer *serverPtr;

    switch (msg->eventType)
    {
        case MSG_APP_FromTransport:
        {
            UdpToAppRecv* info = NULL;
            ZigbeeAppData data;
            ERROR_Assert(sizeof(data) <= ZIGBEEAPP_HEADER_SIZE,
                         "ZigbeeAppData size cant be greater than"
                         " ZIGBEEAPP_HEADER_SIZE");

            info = (UdpToAppRecv *) MESSAGE_ReturnInfo(msg);
            memcpy(&data, MESSAGE_ReturnPacket(msg), sizeof(data));

            // trace recd pkt
            ActionData acnData;
            acnData.actionType = RECV;
            acnData.actionComment = NO_COMMENT;
            TRACE_PrintTrace(node,
                             msg,
                             TRACE_APPLICATION_LAYER,
                             PACKET_IN,
                             &acnData);

#ifdef DEBUG
            {
                char clockStr[MAX_STRING_LENGTH];
                char addrStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(data.txTime, clockStr, node);
                IO_ConvertIpAddressToString(&info->sourceAddr, addrStr);

                printf("ZIGBEEAPP Server %ld: packet transmitted at %sS\n",
                       node->nodeId, clockStr);
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr, node);
                printf("    received at %sS\n", clockStr);
                printf("    client is %s\n", addrStr);
                printf("    connection Id is %d\n", data.sourcePort);
                printf("    seqNo is %d\n", data.seqNo);
            }
#endif /* DEBUG */

            serverPtr = ZigbeeAppServerGetServer(node,
                                                 info->sourceAddr,
                                                 data.sourcePort);

            // New connection, so create new ZIGBEEAPP server to handle client.
            if (serverPtr == NULL)
            {
                serverPtr = ZigbeeAppServerNewServer(node,
                                                     info->destAddr,
                                                     info->sourceAddr,
                                                     data.sourcePort);

                if (serverPtr == NULL)
                {
                    sprintf(error, "ZIGBEEAPP Server: Node %d unable to "
                            "allocation server\n", node->nodeId);

                    ERROR_ReportError(error);
                }
            }

            if (data.seqNo >= serverPtr->seqNo || data.isMdpEnabled)
            {
                serverPtr->numBytesRecvd += MESSAGE_ReturnPacketSize(msg);
                serverPtr->sessionLastReceived = node->getNodeTime();

                clocktype delay = node->getNodeTime() - data.txTime;

                serverPtr->maxEndToEndDelay = MAX(delay,
                                                  serverPtr->maxEndToEndDelay);
                serverPtr->minEndToEndDelay = MIN(delay,
                                                  serverPtr->minEndToEndDelay);
                serverPtr->totalEndToEndDelay += delay;
                serverPtr->numPktsRecvd++;

                serverPtr->seqNo = data.seqNo + 1;

                // Calculate jitter.
                clocktype transitDelay = node->getNodeTime()- data.txTime;
                clocktype tempJitter = 0;

                // Jitter can only be measured after receiving
                // two packets.

                if (serverPtr->numPktsRecvd > 1)
                {

                    tempJitter = transitDelay -
                            serverPtr->lastTransitDelay;

                if (tempJitter < 0) {
                    tempJitter = -tempJitter;
                }
                if (serverPtr->numPktsRecvd == 2)
                {
                    serverPtr->actJitter = tempJitter;
                }
                else
                {
                    serverPtr->actJitter +=
                        ((tempJitter - serverPtr->actJitter)/16);
                }
                    serverPtr->totalJitter += serverPtr->actJitter;

                }

                serverPtr->lastTransitDelay = transitDelay;
                // serverPtr->lastPacketReceptionTime = node->getNodeTime();

#ifdef DEBUG
                {
                    char clockStr[24];
                    TIME_PrintClockInSecond(
                        serverPtr->totalEndToEndDelay, clockStr);
                    printf("    total end-to-end delay so far is %sS\n",
                           clockStr);
                }
#endif /* DEBUG */

                if (data.type == 'd')
                {
                    // Do nothing
                }
                else if (data.type == 'c')
                {
                    serverPtr->sessionFinish = node->getNodeTime();
                    serverPtr->sessionIsClosed = TRUE;
                }
                else
                {
                    assert(FALSE);
                }
            }
            break;
        }

        default:
        {
            char buf[MAX_STRING_LENGTH];

            TIME_PrintClockInSecond(node->getNodeTime(), buf, node);
            sprintf(error, "ZIGBEEAPP Server: At time %sS, node %d received "
                    "message of unknown type "
                    "%d\n", buf, node->nodeId, msg->eventType);
            ERROR_ReportError(error);
        }
    }

    MESSAGE_Free(node, msg);
}

/*
 * NAME:        ZigbeeAppServerInit.
 * PURPOSE:     listen on ZigbeeAppServer server port.
 * PARAMETERS:  node - pointer to the node.
 * RETURN:      none.
 */
void
ZigbeeAppServerInit(Node* node)
{
    APP_InserInPortTable(node, APP_ZIGBEEAPP_SERVER, APP_ZIGBEEAPP_SERVER);
}

/*
 * NAME:        ZigbeeAppServerPrintStats.
 * PURPOSE:     Prints statistics of a ZigbeeAppServer session.
 * PARAMETERS:  node - pointer to this node.
 *              serverPtr - pointer to the zigbeeApp server data structure.
 * RETURN:      none.
 */
void
ZigbeeAppServerPrintStats(Node* node, AppDataZigbeeappServer* serverPtr)
{
    clocktype throughput = 0;
    clocktype avgJitter = 0;
    char addrStr[MAX_STRING_LENGTH];
    char jitterStr[MAX_STRING_LENGTH];
    char clockStr[MAX_STRING_LENGTH];
    char startStr[MAX_STRING_LENGTH];
    char closeStr[MAX_STRING_LENGTH];
    char sessionStatusStr[MAX_STRING_LENGTH];
    char throughputStr[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(serverPtr->sessionStart, startStr, node);
    TIME_PrintClockInSecond(serverPtr->sessionLastReceived, closeStr, node);

    if (serverPtr->sessionIsClosed == FALSE)
    {
        serverPtr->sessionFinish = node->getNodeTime();
        sprintf(sessionStatusStr, "Not closed");
    }
    else
    {
        sprintf(sessionStatusStr, "Closed");
    }

    if (serverPtr->numPktsRecvd == 0)
    {
        TIME_PrintClockInSecond(0, clockStr);
    }
    else
    {
        TIME_PrintClockInSecond(
                    serverPtr->totalEndToEndDelay / serverPtr->numPktsRecvd,
                    clockStr);
    }

    if (serverPtr->sessionFinish <= serverPtr->sessionStart)
    {
        throughput = 0;
    }
    else
    {
        throughput = (clocktype)
                     ((serverPtr->numBytesRecvd * 8.0 * SECOND)
                      / (serverPtr->sessionFinish
                      - serverPtr->sessionStart));
    }

    ctoa(throughput, throughputStr);

    IO_ConvertIpAddressToString(&serverPtr->remoteAddr, addrStr);

    sprintf(buf, "Client address = %s", addrStr);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Server",
                 ANY_DEST,
                 serverPtr->sourcePort,
                 buf);

    sprintf(buf, "First Packet Received at (s) = %s", startStr);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Server",
                 ANY_DEST,
                 serverPtr->sourcePort,
                 buf);

    sprintf(buf, "Last Packet Received at (s) = %s", closeStr);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Server",
                 ANY_DEST,
                 serverPtr->sourcePort,
                 buf);

    sprintf(buf, "Session Status = %s", sessionStatusStr);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Server",
                 ANY_DEST,
                 serverPtr->sourcePort,
                 buf);


    ctoa((Int64) serverPtr->numBytesRecvd, buf1);
    sprintf(buf, "Total Bytes Received = %s", buf1);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Server",
                 ANY_DEST,
                 serverPtr->sourcePort,
                 buf);

    sprintf(buf, "Total Packets Received = %u", serverPtr->numPktsRecvd);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Server",
                 ANY_DEST,
                 serverPtr->sourcePort,
                 buf);

    sprintf(buf, "Throughput (bits/s) = %s", throughputStr);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Server",
                 ANY_DEST,
                 serverPtr->sourcePort,
                 buf);

    sprintf(buf, "Average End-to-End Delay (s) = %s", clockStr);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Server",
                 ANY_DEST,
                 serverPtr->sourcePort,
                 buf);

    // Jitter can only be measured after receiving two packets.
    if (serverPtr->numPktsRecvd - 1 <= 0)
    {
        avgJitter = 0;
    }
    else
    {
        avgJitter = serverPtr->totalJitter / (serverPtr->numPktsRecvd-1) ;
    }

    TIME_PrintClockInSecond(avgJitter, jitterStr);

    sprintf(buf, "Average Jitter (s) = %s", jitterStr);
    IO_PrintStat(node,
                 "Application",
                 "ZIGBEEAPP Server",
                 ANY_DEST,
                 serverPtr->sourcePort,
                 buf);
}

/*
 * NAME:        ZigbeeAppServerFinalize.
 * PURPOSE:     Collect statistics of a ZigbeeAppServer session.
 * PARAMETERS:  node - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
ZigbeeAppServerFinalize(Node* node, AppInfo* appInfo)
{
    AppDataZigbeeappServer* serverPtr
        = (AppDataZigbeeappServer*) appInfo->appDetail;

    if (node->appData.appStats == TRUE)
    {
        ZigbeeAppServerPrintStats(node, serverPtr);
    }
}

/*
 * NAME:        ZigbeeAppServerGetServer.
 * PURPOSE:     search for a zigbeeApp server data structure.
 * PARAMETERS:  node - ZIGBEEAPP server.
 *              remoteAddr - zigbeeApp client sending the data.
 *              sourcePort - sourcePort of zigbeeApp client/server pair.
 * RETURN:      the pointer to the zigbeeApp server data structure,
 *              NULL if nothing found.
 */
AppDataZigbeeappServer*
ZigbeeAppServerGetServer(Node* node, const Address remoteAddr, short sourcePort)
{
    AppInfo* appList = node->appData.appPtr;
    AppDataZigbeeappServer* ZigbeeAppServer;

    for (; appList != NULL; appList = appList->appNext)
    {
        if (appList->appType == APP_ZIGBEEAPP_SERVER)
        {
            ZigbeeAppServer = (AppDataZigbeeappServer*) appList->appDetail;

            if ((ZigbeeAppServer->sourcePort == sourcePort)
                && IO_CheckIsSameAddress(
                                ZigbeeAppServer->remoteAddr, remoteAddr))
            {
                return ZigbeeAppServer;
            }
       }
   }

    return NULL;
}

/*
 * NAME:        ZigbeeAppServerNewServer.
 * PURPOSE:     create a new zigbeeApp server data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  node - pointer to the node.
 *              &localAddr - reference to local address.
 *              &remoteAddr - reference to remote address.
 *              sourcePort - sourcePort of ZIGBEEAPP client/server pair.
 * RETRUN:      the pointer to the created ZIGBEEAPP server data structure,
 *              NULL if no data structure allocated.
 */
AppDataZigbeeappServer*
ZigbeeAppServerNewServer(Node* node,
                         const Address &localAddr,
                         const Address &remoteAddr,
                         short sourcePort)
{
    AppDataZigbeeappServer* ZigbeeAppServer = NULL;

    ZigbeeAppServer = (AppDataZigbeeappServer *)
                        MEM_malloc(sizeof(AppDataZigbeeappServer));
    memset(ZigbeeAppServer, 0, sizeof(AppDataZigbeeappServer));

    // Fill in ZIGBEEAPP info
    memcpy(&(ZigbeeAppServer->localAddr), &localAddr, sizeof(Address));
    memcpy(&(ZigbeeAppServer->remoteAddr), &remoteAddr, sizeof(Address));
    ZigbeeAppServer->sourcePort = sourcePort;
    ZigbeeAppServer->sessionStart = node->getNodeTime();
    ZigbeeAppServer->sessionFinish = node->getNodeTime();
    ZigbeeAppServer->sessionLastReceived = node->getNodeTime();
    ZigbeeAppServer->sessionIsClosed = FALSE;
    ZigbeeAppServer->numBytesRecvd = 0;
    ZigbeeAppServer->numPktsRecvd = 0;
    ZigbeeAppServer->totalEndToEndDelay = 0;
    ZigbeeAppServer->maxEndToEndDelay = 0;
    ZigbeeAppServer->minEndToEndDelay = CLOCKTYPE_MAX;
    ZigbeeAppServer->seqNo = 0;
    ZigbeeAppServer->uniqueId = node->appData.uniqueId++;

    ZigbeeAppServer->totalJitter = 0;
    ZigbeeAppServer->actJitter = 0;

    // Add ZIGBEEAPP variables to hierarchy
    std::string path;
    D_Hierarchy* h = &node->partitionData->dynamicHierarchy;

    if (h->CreateApplicationPath(node,
                                 "ZigbeeAppServer",
                                 ZigbeeAppServer->sourcePort,
                                 "numBytesRecvd",
                                 path))
    {
        h->AddObject(path,
                     new D_Int64Obj(&ZigbeeAppServer->numBytesRecvd));
    }

    APP_RegisterNewApp(node, APP_ZIGBEEAPP_SERVER, ZigbeeAppServer);
    return ZigbeeAppServer;
}
