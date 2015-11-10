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

/*
 *
 * This file contains initialization function, message processing
 * function, and finalize function used by the NeighborProtocol application.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "network_ip.h"

#include "app_util.h"
#include "network_neighbor_prot.h"


#define NUM_INITIAL_NEIGHBOR_RECORDS 15
#define MAX_INITIAL_JITTER (1 * SECOND)


void
NeighborProtocolRecvdPacket(
    Node* node,
    Message * msg);


static void
NeighborProtocolRecvdTimer(
    Node* node,
    Message * msg);


NodeAddress
AppReturnInterfaceBroadcastAddress(
    Node *node,
    NodeAddress localAddr)
{
    int i;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (localAddr == NetworkIpGetInterfaceAddress(node, i))
        {
            if (node->macData[i]->macProtocol == MAC_PROTOCOL_LINK)
            {
                return NetworkIpGetInterfaceBroadcastAddress(node, i);
            }
            else
            {
                return ANY_DEST;
            }
        }
    }
    return ANY_DEST;
}



static void
SendNextPacket(
    Node* node,
    NodeAddress localAddr)
{
    char PacketToSend[1024];
    strcpy(PacketToSend, "N");

    Message *msg = APP_UdpCreateMessage(
        node,
        localAddr,
        APP_NEIGHBOR_PROTOCOL,
        AppReturnInterfaceBroadcastAddress(node, localAddr),
        APP_NEIGHBOR_PROTOCOL,
        TRACE_NEIGHBOR_PROT);

    APP_AddPayload(node, msg, PacketToSend, strlen(PacketToSend));

    APP_UdpSend(node, msg);
}


static NeighborProtocolData *
NeighborProtocolReturnInfo(
    Node *node)
{
    AppInfo *appPtr = node->appData.appPtr;

    while (appPtr)
    {
        if (appPtr->appType == APP_NEIGHBOR_PROTOCOL)
        {
            return (NeighborProtocolData *) appPtr->appDetail;
        }
        appPtr = appPtr->appNext;
    }

    return NULL;
}


static NeighborInfoRecord *
NeighborProtocolReturnNeighborInfoRecord(
    NeighborProtocolData *neighborProt,
    NodeAddress ipAddr)
{
    int i;

    for (i = 0; i < neighborProt->numNbrInfoRecords; i++)
    {
        if ((neighborProt->nbrInfo[i].neighbor == ipAddr) &&
            (neighborProt->nbrInfo[i].lastHeard < CLOCKTYPE_MAX))
        {
            return &(neighborProt->nbrInfo[i]);
        }
    }

    return NULL;
}


static clocktype
ExpirationTime(
    NeighborProtocolData *neighborProt,
    NodeAddress ipAddr)
{
    NeighborInfoRecord *nbrInfoRecord;

    nbrInfoRecord = NeighborProtocolReturnNeighborInfoRecord(
                        neighborProt,
                        ipAddr);

    return (nbrInfoRecord->lastHeard + neighborProt->entryLifetime);

}


static void
NeighborProtocolAddNewNeighbor(
    Node *node,
    NeighborProtocolData *neighborProt,
    NodeAddress ipAddr)
{
    int index = neighborProt->numNbrInfoRecords;
    int i;
    clocktype expirationTime;
    BOOL added = FALSE;


#ifdef DEBUG
    printf("#%d: NeighborProtocolAddNewNeighbor()\n", node->nodeId);
#endif

    for (i = 0; i < neighborProt->numNbrInfoRecords; i++)
    {
        if ((neighborProt->nbrInfo[i].neighbor == ipAddr) &&
            (neighborProt->nbrInfo[i].lastHeard == CLOCKTYPE_MAX))
        {
            neighborProt->nbrInfo[i].lastHeard = node->getNodeTime();
            added = TRUE;
        }

    }


    if (!added)
    {
        if (index == neighborProt->maxNbrInfoRecords)
        {
            NeighborInfoRecord *newRecords =
                (NeighborInfoRecord *)
                MEM_malloc(sizeof(NeighborInfoRecord) *
                                  (index + NUM_INITIAL_NEIGHBOR_RECORDS));

            memcpy(newRecords, neighborProt->nbrInfo,
                   sizeof(NeighborInfoRecord) * index);


            MEM_free(neighborProt->nbrInfo);

            neighborProt->nbrInfo = newRecords;
            neighborProt->maxNbrInfoRecords += NUM_INITIAL_NEIGHBOR_RECORDS;
        }
        neighborProt->nbrInfo[index].neighbor = ipAddr;
        neighborProt->nbrInfo[index].lastHeard = node->getNodeTime();

        neighborProt->numNbrInfoRecords++;
    }


    if (neighborProt->updateFunction)
    {
        (neighborProt->updateFunction) (node, ipAddr, TRUE);
    }

    expirationTime = ExpirationTime(neighborProt, ipAddr);

    if (neighborProt->nextTimerExpiration < expirationTime)
    {
        // Don't set a timer, since the current timer will return
        // before this ip address is set to expire
    }
    else
    {

        neighborProt->nextTimerExpiration = expirationTime;

        APP_SetTimer(
            node,
            APP_NEIGHBOR_PROTOCOL,
            0,
            0,
            APP_TIMER_UPDATE_TABLE,
            expirationTime - node->getNodeTime());

    }
}


static void
NeighborProtocolInitStats(
    NeighborProtocolData *neighborProt)
{
    neighborProt->stats =
        (NeighborProtocolStats *) MEM_malloc(sizeof(NeighborProtocolStats));

    neighborProt->stats->pktsRecv = 0;
    neighborProt->stats->pktsSent = 0;
}


NeighborProtocolData*
NeighborProtocolInit(
    Node* node,
    NodeAddress localAddr,
    clocktype sendFrequency,
    clocktype entryLifetime)
{
    NeighborProtocolData* neighborProt;
    AppInfo* appInfo;
    RandomSeed seed;
    RANDOM_SetSeed(seed,
                   node->globalSeed,
                   node->nodeId,
                   APP_NEIGHBOR_PROTOCOL);
    clocktype initial_jitter =
        (clocktype) (RANDOM_erand(seed) * MAX_INITIAL_JITTER);

    neighborProt = (NeighborProtocolData *)
        MEM_malloc(sizeof(NeighborProtocolData));
    memset(neighborProt, 0, sizeof(NeighborProtocolData));

    neighborProt->localAddr = localAddr;
    neighborProt->sendFrequency = sendFrequency;
    neighborProt->entryLifetime = entryLifetime;

    neighborProt->nextTimerExpiration = CLOCKTYPE_MAX;

    neighborProt->nbrInfo =
        (NeighborInfoRecord *)
        MEM_malloc(sizeof(NeighborInfoRecord) * NUM_INITIAL_NEIGHBOR_RECORDS);

    neighborProt->numNbrInfoRecords = 0;
    neighborProt->maxNbrInfoRecords = NUM_INITIAL_NEIGHBOR_RECORDS;

    NeighborProtocolInitStats(neighborProt);

    appInfo = APP_RegisterNewApp(
                  node,
                  APP_NEIGHBOR_PROTOCOL,
                  (void *) neighborProt);

    APP_SetTimer(
        node,
        APP_NEIGHBOR_PROTOCOL,
        0,
        0,
        APP_TIMER_SEND_PKT,
        sendFrequency + initial_jitter);

#ifdef DEBUG
    printf("#%d: NeighborProtocolInit()\n", node->nodeId);
    {
        NeighborProtocolData *neighborProt =
            (NeighborProtocolData *) NeighborProtocolReturnInfo(node);

        printf("initial_jitter = %lf\n", initial_jitter);
    }
#endif

    return neighborProt;
}

void
NeighborProtocolRegisterNeighborUpdateFunction(
    Node* node,
    NeighborProtocolUpdateFunctionType updateFunction,
    NodeAddress localAddr,
    clocktype updateFrequency,
    clocktype deleteInterval)
{
    NeighborProtocolData *neighborProt =
        (NeighborProtocolData *) NeighborProtocolReturnInfo(node);

#ifdef DEBUG
    printf("#%d: NeighborProtocolRegisterNeighborUpdateFunction()\n",
           node->nodeId);
#endif

    if (!neighborProt)
    {
        node->appData.appPtr = NULL;

        neighborProt = NeighborProtocolInit(
                           node,
                           localAddr,
                           updateFrequency,
                           deleteInterval);
    }

    neighborProt->updateFunction = updateFunction;
}


void
NeighborProtocolRecvdPacket(
    Node* node,
    Message * msg)
{
    UdpToAppRecv *udpInfo = (UdpToAppRecv *) MESSAGE_ReturnInfo(msg);
    NeighborProtocolData *neighborProt;
    NeighborInfoRecord *infoRecord;

    neighborProt = (NeighborProtocolData *)
                   NeighborProtocolReturnInfo(node);

    if (!neighborProt)
    {
        MESSAGE_Free(node, msg);
        return;
    }

    if (node->appData.appStats == TRUE)
    {
        neighborProt->stats->pktsRecv++;
    }

#ifdef DEBUG
    printf("#%d: NeighborProtocolRecvdPacket()\n", node->nodeId);
#endif

    infoRecord = NeighborProtocolReturnNeighborInfoRecord(
                 neighborProt, GetIPv4Address(udpInfo->sourceAddr));

    if (!infoRecord)
    {
#ifdef DEBUG
        printf("\tNew Neighbor %d\n",
            GetIPv4Address(udpInfo->sourceAddr));
#endif
        NeighborProtocolAddNewNeighbor(
            node,
            neighborProt,
            GetIPv4Address(udpInfo->sourceAddr));
    }
    else
    {
        infoRecord->lastHeard = node->getNodeTime();
    }

    MESSAGE_Free(node, msg);
}


static void
NeighborProtocolUpdateEntries(
    Node* node)
{
    NeighborProtocolData *neighborProt;
    clocktype nextTimerTime = CLOCKTYPE_MAX;
    clocktype tooOldTime;
    int i;

    neighborProt = (NeighborProtocolData *)
             NeighborProtocolReturnInfo(node);

    tooOldTime = node->getNodeTime() - neighborProt->entryLifetime;

#ifdef DEBUG
    printf("#%d: UpdateTable\n", node->nodeId);
#endif

    for (i = 0; i < neighborProt->numNbrInfoRecords; i++)
    {
        clocktype *lastHeard = &(neighborProt->nbrInfo[i].lastHeard);

        if (*lastHeard <= tooOldTime)
        {
#ifdef DEBUG
            printf("#%d: Removing neighbor %d\n", node->nodeId,
                   neighborProt->nbrInfo[i].neighbor);
#endif

            *lastHeard = CLOCKTYPE_MAX;

            if (neighborProt->updateFunction)
            {
                (neighborProt->updateFunction) (
                    node,
                    neighborProt->nbrInfo[i].neighbor,
                    FALSE);
            }

        }
        else
        if (*lastHeard == CLOCKTYPE_MAX)
        {
            // ignore this unused entry
        }
        else
        {
            if (nextTimerTime > (*lastHeard + neighborProt->entryLifetime))
            {
                nextTimerTime = (*lastHeard + neighborProt->entryLifetime);
            }
        }
    }

    if (nextTimerTime < CLOCKTYPE_MAX)
    {
        neighborProt->nextTimerExpiration = nextTimerTime;

        APP_SetTimer(
            node,
            APP_NEIGHBOR_PROTOCOL,
            0,
            0,
            APP_TIMER_UPDATE_TABLE,
            nextTimerTime - node->getNodeTime());
    }
    else
    {
        neighborProt->nextTimerExpiration = CLOCKTYPE_MAX;
    }
}


static void
NeighborProtocolRecvdTimer(
    Node* node,
    Message * msg)
{
    NeighborProtocolData *neighborProt;

    neighborProt = (NeighborProtocolData *)
             NeighborProtocolReturnInfo(node);

    if (!neighborProt)
    {
        MESSAGE_Free(node, msg);
        return;
    }

    switch (APP_GetTimerType(msg))
    {
        case APP_TIMER_SEND_PKT:
        {
            SendNextPacket(node, neighborProt->localAddr);

            if (node->appData.appStats == TRUE)
            {
                neighborProt->stats->pktsSent++;
            }

            APP_SetTimer(
                node,
                APP_NEIGHBOR_PROTOCOL,
                0,
                0,
                APP_TIMER_SEND_PKT,
                neighborProt->sendFrequency);

            break;
        }
        case APP_TIMER_UPDATE_TABLE:
        {
            NeighborProtocolUpdateEntries(node);
            break;
        }
        default:
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr, "Unknown APP_TIMER type %d\n",
                              APP_GetTimerType(msg));

            ERROR_ReportError(errorStr);
        }
    }

     MESSAGE_Free(node, msg);
}


void
NeighborProtocolProcessMessage(
    Node* node,
    Message * msg)
{
    switch (MESSAGE_GetEvent(msg))
    {
        case MSG_APP_FromTransport:
            NeighborProtocolRecvdPacket(node, msg);
            break;
        case MSG_APP_TimerExpired:
            NeighborProtocolRecvdTimer(node, msg);
            break;
        default:
            ERROR_ReportErrorArgs("(node %d) NeighborProtocolProcessMessage"
                                  " received unknown Message Event %d\n",
                                  node->nodeId,
                                  MESSAGE_GetEvent(msg));
    }
}

static void
NeighborProtocolPrintStats(
    Node *node,
    NeighborProtocolData *neighborProt)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "Packets Sent = %d", neighborProt->stats->pktsSent);
    IO_PrintStat(
        node,
        "Application",
        "Neighbor Protocol",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Packets Received = %d", neighborProt->stats->pktsRecv);
    IO_PrintStat(
        node,
        "Application",
        "Neighbor Protocol",
        ANY_DEST,
        -1 /* instance Id */,
        buf);
}

void
NeighborProtocolFinalize(
    Node *node,
    AppInfo* appInfo)
{
    NeighborProtocolData *neighborProt =
        (NeighborProtocolData*)appInfo->appDetail;

    if (node->appData.appStats == TRUE)
    {
        NeighborProtocolPrintStats(node, neighborProt);
    }
}

