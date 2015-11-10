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

#ifndef _ZigbeeApp_APP_H_
#define _ZigbeeApp_APP_H_

#include "types.h"
#include "dynamic.h"

/*
 * Header size defined to be consistent accross 32/64 bit platforms
 */
// #define ZigbeeApp_HEADER_SIZE 32
#define ZIGBEEAPP_HEADER_SIZE 40
/*
 * Data item structure used by zigbeeApp.
 */
typedef
struct struct_app_zigbeeApp_data
{
    short sourcePort;
    char type;
    BOOL isMdpEnabled;
    Int32 seqNo;
    clocktype txTime;
}
ZigbeeAppData;


/* Structure containing zigbeeApp client information. */

typedef
struct struct_app_zigbeeapp_client_str
{
    Address localAddr;
    Address remoteAddr;
    D_Clocktype interval;
    clocktype sessionStart;
    clocktype sessionFinish;
    clocktype sessionLastSent;
    clocktype endTime;
    BOOL sessionIsClosed;
    D_Int64 numBytesSent;
    UInt32 numPktsSent;
    UInt32 itemsToSend;
    UInt32 itemSize;
    short sourcePort;
    Int32 seqNo;
    D_UInt32 tos;
}
AppDataZigbeeappClient;

/* Structure containing zigbeeApp related information. */

typedef
struct struct_app_zigbeeapp_server_str
{
    Address localAddr;
    Address remoteAddr;
    short sourcePort;
    clocktype sessionStart;
    clocktype sessionFinish;
    clocktype sessionLastReceived;
    BOOL sessionIsClosed;
    D_Int64 numBytesRecvd;
    UInt32 numPktsRecvd;
    clocktype totalEndToEndDelay;
    clocktype maxEndToEndDelay;
    clocktype minEndToEndDelay;
    Int32 seqNo;
    Int32 uniqueId ;

    clocktype totalJitter;

    clocktype lastTransitDelay;
    clocktype actJitter;
}
AppDataZigbeeappServer;


/*
 * NAME:        ZigbeeAppClient.
 * PURPOSE:     Models the behaviour of zigbeeAppClient Client on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  nodePtr - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
ZigbeeAppClient(Node* nodePtr, Message* msg);

/*
 * NAME:        ZigbeeAppClientInit.
 * PURPOSE:     Initialize a ZigbeeAppClient session.
 * PARAMETERS:  nodePtr - pointer to the node,
 *              serverAddr - address of the server,
 *              itemsToSend - number of items to send,
 *              itemSize - size of each packet,
 *              interval - interval of packet transmission rate.
 *              startTime - time until the session starts.
 *              endTime - time until the session end.
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
                    unsigned tos);


/*
 * NAME:        ZigbeeAppClientPrintStats.
 * PURPOSE:     Prints statistics of a ZigbeeAppClient session.
 * PARAMETERS:  nodePtr - pointer to the this node.
 *              clientPtr - pointer to the ZigbeeApp client data structure.
 * RETURN:      none.
 */
void
ZigbeeAppClientPrintStats(Node* nodePtr, AppDataZigbeeappClient* clientPtr);

/*
 * NAME:        ZigbeeAppClientFinalize.
 * PURPOSE:     Collect statistics of a ZigbeeAppClient session.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
ZigbeeAppClientFinalize(Node* nodePtr, AppInfo* appInfo);

/*
 * NAME:        ZigbeeAppClientGetClient.
 * PURPOSE:     search for a ZigbeeApp client data structure.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              sourcePort - source port of the ZigbeeApp client.
 * RETURN:      the pointer to the ZigbeeApp client data structure,
 *              NULL if nothing found.
 */
AppDataZigbeeappClient*
ZigbeeAppClientGetClient(Node* nodePtr, short sourcePort);

/*
 * NAME:        ZigbeeAppClientNewClient.
 * PURPOSE:     create a new ZigbeeApp client data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              remoteAddr - remote address.
 *              itemsToSend - number of items to send,
 *              itemSize - size of data packets.
 *              interval - interdeparture time of packets.
 *              startTime - time when session is started.
 *              endTime - time when session is ended.
 * RETURN:      the pointer to the created ZigbeeApp client data structure,
 *              NULL if no data structure allocated.
 */
AppDataZigbeeappClient*
ZigbeeAppClientNewClient(Node* nodePtr,
                         const Address &localAddr,
                         const Address &remoteAddr,
                         Int32 itemsToSend,
                         Int32 itemSize,
                         clocktype interval,
                         clocktype startTime,
                         clocktype endTime,
                         TosType tos);

/*
 * NAME:        ZigbeeAppClientScheduleNextPkt.
 * PURPOSE:     schedule the next packet the client will send.  If next packet
 *              won't arrive until the finish deadline, schedule a close.
 * PARAMETERS:  nodePtr - pointer to the node,
 *              clientPtr - pointer to the ZigbeeApp client data structure.
 * RETRUN:      none.
 */
void
ZigbeeAppClientScheduleNextPkt(Node* nodePtr, AppDataZigbeeappClient* clientPtr);


/*
 * NAME:        ZigbeeAppServer.
 * PURPOSE:     Models the behaviour of ZigbeeAppServer Server on receiving the
 *              message encapsulated in msg.
 * PARAMETERS:  nodePtr - pointer to the node which received the message.
 *              msg - message received by the layer
 * RETURN:      none.
 */
void
ZigbeeAppServer(Node* nodePtr, Message* msg);

/*
 * NAME:        ZigbeeAppServerInit.
 * PURPOSE:     listen on ZigbeeAppServer server port.
 * PARAMETERS:  nodePtr - pointer to the node.
 * RETURN:      none.
 */
void
ZigbeeAppServerInit(Node* nodePtr);

/*
 * NAME:        ZigbeeAppServerPrintStats.
 * PURPOSE:     Prints statistics of a ZigbeeAppServer session.
 * PARAMETERS:  nodePtr - pointer to this node.
 *              serverPtr - pointer to the ZigbeeApp server data structure.
 * RETURN:      none.
 */
void
ZigbeeAppServerPrintStats(Node* nodePtr, AppDataZigbeeappServer* serverPtr);

/*
 * NAME:        ZigbeeAppServerFinalize.
 * PURPOSE:     Collect statistics of a ZigbeeAppServer session.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              appInfo - pointer to the application info data structure.
 * RETURN:      none.
 */
void
ZigbeeAppServerFinalize(Node* nodePtr, AppInfo* appInfo);

/*
 * NAME:        ZigbeeAppServerGetServer.
 * PURPOSE:     search for a ZigbeeApp server data structure.
 * PARAMETERS:  nodePtr - ZigbeeApp server.
 *              sourcePort - source port of ZigbeeApp client/server pair.
 *              remoteAddr - ZigbeeApp client sending the data.
 * RETURN:      the pointer to the ZigbeeApp server data structure,
 *              NULL if nothing found.
 */
AppDataZigbeeappServer*
ZigbeeAppServerGetServer(Node* nodePtr,
                         Address remoteAddr,
                         short sourcePort);

/*
 * NAME:        ZigbeeAppServerNewServer.
 * PURPOSE:     create a new ZigbeeApp server data structure, place it
 *              at the beginning of the application list.
 * PARAMETERS:  nodePtr - pointer to the node.
 *              localAddr - local address.
 *              remoteAddr - remote address.
 *              sourcePort - source port of ZigbeeApp client/server pair.
 * RETRUN:      the pointer to the created ZigbeeApp server data structure,
 *              NULL if no data structure allocated.
 */
AppDataZigbeeappServer*
ZigbeeAppServerNewServer(Node* nodePtr,
                         const Address &localAddr,
                         const Address &remoteAddr,
                         short sourcePort);

 /*
 * NAME       :: ZigbeeAppPrintTraceXML
 * PURPOSE    :: Print packet trace information in XML format
 * PARAMETERS :: node     : Node*    : Pointer to node
 *                msg      : Message* : Pointer to packet to print headers
 * RETURN     ::  none
*/

void ZigbeeAppPrintTraceXML(Node* node, Message* msg);

/*
 * FUNCTION   :: ZigbeeAppInitTrace
 * PURPOSE    :: ZigbeeApp initialization  for tracing
 * PARAMETERS :: node : Node* : Pointer to node
 *               nodeInput    : const NodeInput* : Pointer to NodeInput
 * RETURN :: none
*/
void ZigbeeAppInitTrace(Node* node, const NodeInput* nodeInput);

#endif /* _ZigbeeApp_APP_H_ */

