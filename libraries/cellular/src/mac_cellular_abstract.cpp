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
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network.h"
#include "network_ip.h"

#ifdef PARALLEL //Parallel
#include "parallel.h"
#endif //endParallel

#include "cellular.h"
#include "cellular_abstract.h"
#include "cellular_layer3.h"
#include "cellular_abstract_layer3.h"
#include "mac_cellular.h"
#include "mac_cellular_abstract.h"

#define DEBUG_MAC       0
#define DEBUG_TIMER     0
#define DEBUG_CHANNEL   0
#define DEBUG_HANDOVER  0
#define DEBUG_PACKET    0
#define DEBUG_PROGRESS  0

// /**
// FUNCTION   :: MacCellularAbstractStartListening
// LAYER      :: MAC
// PURPOSE    :: Start listening to a channel
// PARAMETERS ::
// + node : Node* : Pointer to node
// + phyNumber : PHY number
// + channelIndex : int : channel to listen to
// RETURN     :: void : NULL
// **/
static
void MacCellularAbstractStartListening(Node* node,
                                       int phyNumber,
                                       int channelIndex)
{
    if (!PHY_IsListeningToChannel(node, phyNumber, channelIndex))
    {
        if (DEBUG_CHANNEL)
        {
            printf("at %015" TYPES_64BITFMT "d Node%u starts listening"
                   "to channel %d\n",
                    node->getNodeTime(), node->nodeId, channelIndex);
            fflush(stdout);
        }

        PHY_StartListeningToChannel(node, phyNumber, channelIndex);
    }
}

// /**
// FUNCTION   :: MacCellularAbstractStopListening
// LAYER      :: MAC
// PURPOSE    :: Stop listening to a channel
// PARAMETERS ::
// + node : Node* : Pointer to node
// + phyNumber : PHY number of the interface
// + channelIndex : int : channel to stop listening to
// RETURN     :: void : NULL
// **/
static
void MacCellularAbstractStopListening(Node* node,
                                      int phyNumber,
                                      int channelIndex)
{
    if (PHY_IsListeningToChannel(node, phyNumber, channelIndex))
    {
        if (DEBUG_CHANNEL)
        {
            printf("Node%u stops listening to channel %d\n",
                   node->nodeId, channelIndex);
            fflush(stdout);
        }

        PHY_StopListeningToChannel(node, phyNumber, channelIndex);
    }
}

// /**
// FUNCTION   :: MacCellularAbstractInitTimer
// LAYER      :: MAC
// PURPOSE    :: Set a timer with given type and delay
// PARAMETERS ::
// + node : Node* : Pointer to node
// + interfaceIndex : int : index of the interface where the MAC is running
// + timerType : MacCellularAbstractTimerType : type of the timer
// RETURN     :: Message * : Pointer to the allocatd timer message
// **/
static
Message* MacCellularAbstractInitTimer(
             Node* node,
             int interfaceIndex,
             MacCellularAbstractTimerType timerType)
{
    Message* timerMsg = NULL;
    MacCellularAbstractTimer* timerInfo = NULL;

    // allocate the timer message and send out
    timerMsg = MESSAGE_Alloc(node,
                             MAC_LAYER,
                             MAC_PROTOCOL_CELLULAR,
                             MSG_MAC_TimerExpired);

    MESSAGE_SetInstanceId(timerMsg, (short) interfaceIndex);

    MESSAGE_InfoAlloc(node, timerMsg, sizeof(MacCellularAbstractTimer));
    timerInfo = (MacCellularAbstractTimer*) MESSAGE_ReturnInfo(timerMsg);

    timerInfo->timerType = timerType;

    return timerMsg;
}

// /**
// FUNCTION   :: MacCellularAbstractTransmitOnTCH
// LAYER      :: MAC
// PURPOSE    :: Transmit a packet on the traffic channel (TCH)
//               Assume the header of the packet has been properly set.
//               Current implementation sends packets directly to the
//               destination node with a fixed delay.
// PARAMETERS ::
// + node : Node* : Pointer to node
// + interfaceIndex : int : index of the interface where the MAC is running
// + msg : Message* : Packet to be sent on TCH
// RETURN     :: void : NULL
// **/
static
void MacCellularAbstractTransmitOnTCH(Node *node,
                                      int interfaceIndex,
                                      Message *msg)
{
    MacCellularData *macCellularData =
        (MacCellularData *) node->macData[interfaceIndex]->macVar;
    MacCellularAbstractData *macData =
        (MacCellularAbstractData *)macCellularData->macCellularAbstractData;

    MacCellularAbstractHeader *headerPtr = NULL;
    Node *nextNode = NULL;

    // to emulate the transmisison delay
    // currently it is set to 0
    clocktype txDelay = 0;
    int i = 0;

     headerPtr = (MacCellularAbstractHeader *) MESSAGE_ReturnPacket(msg);
    if (DEBUG_PROGRESS)
    {
        printf("at %015" TYPES_64BITFMT "d node %d TCH trans to dest %d:"
               "msg size %d info size %d\n",
               node->getNodeTime(), node->nodeId,  headerPtr->destId,
               MESSAGE_ReturnActualPacketSize(msg),
               MESSAGE_ReturnInfoSize(msg));
        fflush(stdout);
    }

    ERROR_Assert(headerPtr->destId != ANY_DEST, "Wrong destination node Id");

    if (PARTITION_NodeExists(node->partitionData, headerPtr->destId)
        == FALSE)
    {
        char errMsg[MAX_STRING_LENGTH];

        sprintf(errMsg, "Wrong destination node%d for a TCH packet!",
                headerPtr->destId);
        ERROR_ReportWarning(errMsg);

        MESSAGE_Free(node, msg);

        return;
    }

    BOOL nodeIsLocal = FALSE;

    nodeIsLocal = PARTITION_ReturnNodePointer(node->partitionData,
                                               &nextNode,
                                               headerPtr->destId,
                                               FALSE);

#ifdef PARALLEL //Parallel
    if (!nodeIsLocal) {
        MESSAGE_SetEvent(msg, MSG_MAC_CELLULAR_FromTch);
        MESSAGE_SetLayer(msg, MAC_LAYER, 0);
        MESSAGE_RemoteSend(
            node,
            headerPtr->destId,
            msg,
            MAC_CELLULAR_ABSTRACT_TCH_DELAY +
            txDelay +
            node->macData[interfaceIndex]->propDelay);
        macData->stats.numPktSentOnTCH ++;
        return;
    }
#endif //endParallel

    // this is the intended destination

    for (i = 0; i < nextNode->numberInterfaces; i ++)
    {
        if (nextNode->macData[i]->macProtocol ==
            MAC_PROTOCOL_CELLULAR)
        {
            break;
        }
    }

    if (i >= nextNode->numberInterfaces)
    { // destination node is not running Cellular MAC
        char errMsg[MAX_STRING_LENGTH];

        sprintf(errMsg, "Dest node%d is not running cellular MAC!",
                nextNode->nodeId);
        ERROR_ReportWarning(errMsg);

        MESSAGE_Free(node, msg);
    }
    else
    {
        // corresponding interface on dest is found
        /*txDelay = PHY_GetTransmissionDelay(
                            node,
                            node->macData[interfaceIndex]->phyNumber,
                            MESSAGE_ReturnPacketSize(msg));*/

        MESSAGE_SetEvent(msg, MSG_MAC_CELLULAR_FromTch);
        MESSAGE_SetLayer(msg, MAC_LAYER, 0);
        MESSAGE_SetInstanceId(msg, (short) i);

#if 0
        MESSAGE_Send(nextNode,
                     msg,
                     MAC_CELLULAR_ABSTRACT_TCH_DELAY +
                     txDelay +
                     node->macData[interfaceIndex]->propDelay);
#endif
        MacCellularScheduleTCH(nextNode,
                               msg,
                               MAC_CELLULAR_ABSTRACT_TCH_DELAY +
                               txDelay +
                               node->macData[interfaceIndex]->propDelay);

        // increase the statistics
        macData->stats.numPktSentOnTCH ++;
    }
}

// /**
// FUNCTION   :: MacCellularAbstractCalculateSignalStrength
// LAYER      :: MAC
// PURPOSE    :: Calculate the signal strength. In theory, the strength
//               should be reported by the PHY model. Here, we simply
//               estimate signal strength by calculating the distance to BS
// PARAMETERS ::
// + bsLocation : Coordinates : location of the BS
// + msLocation : Coordinates : location of myself (MS)
// + signalStrength : double* : for return the signal strength calculated
// + numSector : short : number of sectors of this BS
// + sectorId : short* : for return sector id of this measurement
// RETURN     :: void : NULL
// **/
static
void MacCellularAbstractCalculateSignalStrength(Node *node,
                                                Coordinates *bsLocation,
                                                Coordinates *msLocation,
                                                double *signalStrength,
                                                short numSector,
                                                short  *sectorId)
{
    Orientation DOA1;
    Orientation DOA2;
    *sectorId = CELLULAR_ABSTRACT_INVALID_SECTOR_ID;

    COORD_CalcDistanceAndAngle(
        NODE_GetTerrainPtr(node)->getCoordinateSystem(),
        bsLocation,
        msLocation,
        signalStrength,
        &DOA1,
        &DOA2);

    if (DEBUG_MAC)
    {
        printf("The BS to MS angle is %d numSector is %d so the "
               "sectorId is %d\n",
               DOA1.azimuth,numSector, *sectorId);
        fflush(stdout);
    }

    //determine the sector id that closest to the ms
    *sectorId = (short)(DOA1.azimuth /
                (double)(ANGLE_RESOLUTION / numSector)) + 1;

    if (DEBUG_MAC)
    {
        printf("The BS to MS angle is %d numSector is %d so the "
               "sectorId is %d\n",
               DOA1.azimuth,numSector,*sectorId);
        fflush(stdout);
    }

}

// /**
// FUNCTION   :: MacCellularAbstractMeasureBCCHSignal
// LAYER      :: MAC
// PURPOSE    :: Measure the signal strength when a packet is received
//               from the BCCH channel
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : interface running this MAC
// + msg              : Message*          : Packet received from BCCH
// RETURN     :: void : NULL
// **/
static
void MacCellularAbstractMeasureBCCHSignal(Node *node,
                                          int interfaceIndex,
                                          Message *msg)
{
    NodeAddress bsId;
    short sectorId;
    short numSector;
    Coordinates bsLocation;
    Coordinates msLocation;
    MacCellularData *macCellularData;
    MacCellularAbstractData *macCellularAbstractData;
    CellularAbstractBsSystemInfo *sysInfo;
    CellularAbstractDownlinkMeasurement *newMeas;
    CellularAbstractDownlinkMeasurement *currentMeas;
    double signalStrength;
    BOOL found;

    if (DEBUG_MAC)
    {
        printf("Node %d MAC: measure the BCCh signal strength\n",
               node->nodeId);
        fflush(stdout);
    }

    macCellularData = (MacCellularData *)
                      node->macData[interfaceIndex]->macVar;
    macCellularAbstractData = (MacCellularAbstractData *)
                               macCellularData->macCellularAbstractData;

    // access the content of the packets
    sysInfo = (CellularAbstractBsSystemInfo *)
              (MESSAGE_ReturnPacket(msg) +
              sizeof(CellularGenericLayer3MsgHdr));

    bsId=sysInfo->bsNodeId;
    numSector = (short)sysInfo->numSector;
    bsLocation.common.c1 = sysInfo->bsLocationC1;
    bsLocation.common.c2 = sysInfo->bsLocationC2;
    bsLocation.common.c3 = sysInfo->bsLocationC3;
    if (DEBUG_MAC)
    {
        printf("Node %d: BCCH from bsid %d  numsector is %d\n",
               node->nodeId,sysInfo->bsNodeId,numSector);
        fflush(stdout);
    }

    // get my location
    MOBILITY_ReturnCoordinates(node, &msLocation);

    // get signal strength to the BS
    MacCellularAbstractCalculateSignalStrength(
        node,
        &bsLocation,
        &msLocation,
        &signalStrength,
        numSector,
        &sectorId);

    if (DEBUG_MAC)
    {
        printf("Node %d: measure the BCCh signal strength is %f\n",
               node->nodeId,signalStrength);
        fflush(stdout);
    }

    if (macCellularAbstractData->downlinkMeasurement == NULL)
    {
        newMeas = (CellularAbstractDownlinkMeasurement *)
                  MEM_malloc(sizeof(CellularAbstractDownlinkMeasurement));

        newMeas->measurementTime = node->getNodeTime();
        newMeas->monitoredBsId = bsId;
        newMeas->monitoredSectorId = sectorId;
        newMeas->nextMeasure = NULL;
        newMeas->numMeasurement = 1;
        newMeas->receivedSignalStrength = signalStrength;
        macCellularAbstractData->downlinkMeasurement = newMeas;

        if (DEBUG_MAC)
        {
            printf("Node %d: create the list and the firs record\n",
                   node->nodeId);
            fflush(stdout);
        }

        // attach with the first BS
        if (macCellularAbstractData->associated == FALSE)
        {
            macCellularAbstractData->associated = TRUE;
            macCellularAbstractData->selectedBsId = bsId;
            macCellularAbstractData->selectedSectorId = sectorId;
            macCellularAbstractData->currentULChannel =
                sysInfo->controlULChannelIndex;
            macCellularAbstractData->currentDLChannel =
                macCellularAbstractData->dlChannelList[macCellularAbstractData->scanIndex];

            if (DEBUG_HANDOVER)
            {
                char clockStr[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);
                printf("Node%d(MS) attaches to cell (id=%d, sid=%d, "
                       "DL=%d, UL=%d) at time %s\n",
                       node->nodeId,
                       bsId,
                       sectorId,
                       macCellularAbstractData->currentDLChannel,
                       macCellularAbstractData->currentULChannel,
                       clockStr);
               fflush(stdout);
            }
        }
    }
    else
    {
        //look for this bs and id to see if we do have recorder
        currentMeas = macCellularAbstractData->downlinkMeasurement;
        found = FALSE;
        while (currentMeas != NULL)
        {
            if (currentMeas->monitoredBsId == bsId)
            {
                if (DEBUG_MAC)
                {
                    printf("Node %d: find a existing record for the BS %d\n",
                           node->nodeId,bsId);
                    fflush(stdout);
                }

                found = TRUE;
                break;
            }

            currentMeas = currentMeas->nextMeasure;
        }

        if (found == TRUE)
        {
            if (DEBUG_MAC)
            {
                printf("Node %d: find a existing record for the BS %d\n",
                       node->nodeId,bsId);
                fflush(stdout);
            }

            if ((currentMeas->measurementTime +
                 CELLULAR_ABSTRACT_MAX_SIGNAL_RECORD_EFFECTIVE_TIME_COEFFICIENT *
                 CELLULAR_ABSTRACT_MAX_SIGNAL_RECORD_EFFECTIVE_TIME <
                 node->getNodeTime()) ||
                (currentMeas->monitoredSectorId != sectorId))
            {
                // last measurement is too old,
                // or different sector of the same BS

                currentMeas->monitoredSectorId = sectorId;
                currentMeas->numMeasurement = 1;
                currentMeas->receivedSignalStrength = signalStrength;
                currentMeas->measurementTime = node->getNodeTime();

                if (DEBUG_MAC)
                {
                    printf("Node %d: update to brand new measurement of "
                           "a existing record\n",
                           node->nodeId);
                    fflush(stdout);
                }
            }
            else
            {
                currentMeas->receivedSignalStrength =
                    (currentMeas->receivedSignalStrength *
                     currentMeas->numMeasurement + signalStrength) /
                    (currentMeas->numMeasurement + 1);

                currentMeas->receivedSignalStrength = 
                    currentMeas->receivedSignalStrength * 1000000;
                currentMeas->receivedSignalStrength=(RoundToInt(
                    currentMeas->receivedSignalStrength));
                currentMeas->receivedSignalStrength=
                    currentMeas->receivedSignalStrength/1000000;


                currentMeas->numMeasurement += 1;

                if (currentMeas->numMeasurement >
                    CELLULAR_ABSTRACT_MAX_SIGNAL_RECORD)
                {
                    currentMeas->numMeasurement =
                        CELLULAR_ABSTRACT_MAX_SIGNAL_RECORD;
                }

                currentMeas->measurementTime = node->getNodeTime();

                if (DEBUG_MAC)
                {
                    printf("Node %d:average the measurement in the list\n",
                           node->nodeId);
                fflush(stdout);
                }
            }
        }
        else // a new base station learned
        {
            if (DEBUG_MAC)
            {
                printf("Node %d: not find a existing record for the BS %d "
                       "in the list\n",
                       node->nodeId,bsId);
                fflush(stdout);
            }

            newMeas = (CellularAbstractDownlinkMeasurement *)
                      MEM_malloc(sizeof(CellularAbstractDownlinkMeasurement));

            newMeas->measurementTime = node->getNodeTime();
            newMeas->monitoredBsId = bsId;
            newMeas->monitoredSectorId = sectorId;
            newMeas->nextMeasure = NULL;
            newMeas->receivedSignalStrength = signalStrength;
            newMeas->numMeasurement = 1;

            // add to the end of the list
            currentMeas = macCellularAbstractData->downlinkMeasurement;
            while (currentMeas->nextMeasure != NULL)
            {
                currentMeas = currentMeas->nextMeasure;
            }

            currentMeas->nextMeasure = newMeas;

            if (DEBUG_MAC)
            {
                printf("Node %d: Add a new measurement in the list\n",
                       node->nodeId);
                fflush(stdout);
            }
        }
    }

    if (DEBUG_MAC)
    {
        printf("Node %d: measure the BCCh signalcomplete\n",
               node->nodeId);
        fflush(stdout);
    }
}

// /**
// FUNCTION   :: MacCellularAbstractBuildDownlinkMeasurementMsgToNetwork
// LAYER      :: MAC
// PURPOSE    :: Build a downlink measurement report to layer3
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + interfaceIndex   : int      : Interface index
// + msg              : Message* : packet to be sent to layer3
// + noSignal         : BOOL*    : whether a signal is present in the report
// RETURN     :: void : NULL
// **/
void MacCellularAbstractBuildDownlinkMeasurementMsgToNetwork(
         Node *node,
         int interfaceIndex,
         Message **msg,
         BOOL *noSignal)
{
    CellularAbstractMeasurementReportMsgInfo
        reportInfo[CELLULAR_ABSTRACT_MAX_BS_SECTOR_CANDIDATE];
    CellularAbstractMeasurementReportMsgInfo *info;

    MacCellularData *macCellularData;
    MacCellularAbstractData *macCellularAbstractData;

    CellularAbstractDownlinkMeasurement *currentMeas;
    BOOL foundUnused=FALSE;

    int i;

    for (i = 0; i < CELLULAR_ABSTRACT_MAX_BS_SECTOR_CANDIDATE; i++)
    {
        reportInfo[i].monitoredBsId =
            (NodeAddress) CELLULAR_ABSTRACT_INVALID_BS_ID;
        reportInfo[i].monitoredSectorId =
            CELLULAR_ABSTRACT_INVALID_SECTOR_ID;
        reportInfo[i].receivedSignalStrength = -1;
    }

    //order the measurement
    macCellularData = (MacCellularData *)
                      node->macData[interfaceIndex]->macVar;
    macCellularAbstractData = (MacCellularAbstractData *)
                              macCellularData->macCellularAbstractData;

    *noSignal = TRUE;

    currentMeas = macCellularAbstractData->downlinkMeasurement;
    while (currentMeas != NULL)
    {
        if (currentMeas->measurementTime +
            10 * CELLULAR_ABSTRACT_MAX_SIGNAL_RECORD_EFFECTIVE_TIME >=
           node->getNodeTime())
        {
            *noSignal=FALSE;

            foundUnused = FALSE;
            for (i = 0; i < CELLULAR_ABSTRACT_MAX_BS_SECTOR_CANDIDATE; i++)
            {
                if (reportInfo[i].monitoredBsId ==
                    (NodeAddress)CELLULAR_ABSTRACT_INVALID_BS_ID)
                {
                    foundUnused = TRUE;
                    break;
                }
            }

            if (foundUnused == TRUE)
            { // found one unused storage
                reportInfo[i].monitoredBsId = currentMeas->monitoredBsId;
                reportInfo[i].monitoredSectorId = currentMeas->monitoredSectorId;
                reportInfo[i].receivedSignalStrength =
                    currentMeas->receivedSignalStrength;
            }
            else
            { // cannot find a unused slot

                //find  the mini strght value, since now the distance is the
                // "strenght", we are going to find the longest distance
                int minIndex = 0;
                double minStrength = 0;
                for (i = 0; i < CELLULAR_ABSTRACT_MAX_BS_SECTOR_CANDIDATE; i++)
                {
                    if (reportInfo[i].receivedSignalStrength > minStrength)
                    {
                        minIndex = i;
                        minStrength = reportInfo[i].receivedSignalStrength;
                    }
                }

                reportInfo[minIndex].monitoredBsId = currentMeas->monitoredBsId;
                reportInfo[minIndex].monitoredSectorId =
                    currentMeas->monitoredSectorId;
                reportInfo[minIndex].receivedSignalStrength =
                    currentMeas->receivedSignalStrength;
            }
        }

        currentMeas = currentMeas->nextMeasure;
    }

    if (*noSignal==FALSE)
    {
        MESSAGE_InfoAlloc(
            node,
            *msg,
            CELLULAR_ABSTRACT_MAX_BS_SECTOR_CANDIDATE *
            sizeof(CellularAbstractMeasurementReportMsgInfo));

        info = (CellularAbstractMeasurementReportMsgInfo *)
               MESSAGE_ReturnInfo((*msg));

        memcpy(info,
               reportInfo,
               CELLULAR_ABSTRACT_MAX_BS_SECTOR_CANDIDATE *
               sizeof(CellularAbstractMeasurementReportMsgInfo));

        if (DEBUG_MAC)
        {
            printf("Node %d: insert %d %d %d %d %d %d into the meg to netowrk\n",
                   node->nodeId,
                   info->monitoredBsId,
                   (info+1)->monitoredBsId,
                   (info+2)->monitoredBsId,
                   (info+3)->monitoredBsId,
                   (info+4)->monitoredBsId,
                   (info+5)->monitoredBsId);
            fflush(stdout);
        }
    }
}//MacCellularAbstractBuildDownlinkMeasurementMsgToNetwork

// /**
// FUNCTION   :: MacCellularAbstractInit
// LAYER      :: MAC
// PURPOSE    :: Init Abstract Cellular MAC protocol at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacCellularAbstractInit(Node *node,
                             int interfaceIndex,
                             const NodeInput* nodeInput)
{
    MacCellularData* macCellularData;
    MacCellularAbstractData *macData;
    char errorString[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    clocktype bcchInterval;
    int i;
    Int32 channelIndex = -1;

    if (DEBUG_MAC)
    {
        printf("Node %d: Cellular Abstract MAC Init\n", node->nodeId);
        fflush(stdout);
    }

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;

    macData = (MacCellularAbstractData *)
              MEM_malloc(sizeof(MacCellularAbstractData));
    ERROR_Assert(macData != NULL, "Memory error!");
    macCellularData->macCellularAbstractData = macData;

    memset(macData, 0, sizeof(MacCellularAbstractData));

    macData->strongestSignalBsId = (NodeAddress)CELLULAR_ABSTRACT_INVALID_BS_ID;
    macData->strongestSignalSectorId = CELLULAR_ABSTRACT_INVALID_SECTOR_ID;
    macData->downlinkMeasurement = NULL;
    macData->cellReselected = FALSE;

    macData->chTypeCount = -1;
    macData->status = MAC_CELLULAR_IDLE;
    macData->commActive = FALSE;

    RANDOM_SetSeed(macData->seed,
                   node->globalSeed,
                   node->nodeId,
                   MAC_PROTOCOL_CELLULAR,
                   interfaceIndex + 1);  // +1 because macCellularData uses interfaceIndex

    // read node type
    IO_ReadString(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "CELLULAR-NODE-TYPE",
                  &retVal,
                  buf);
    if (retVal)
    {
        if (strcmp(buf, "CELLULAR-MS") == 0)
        {
            macCellularData->nodeType = CELLULAR_MS;
        }
        else if (strcmp(buf, "CELLULAR-BS") == 0)
        {
            macCellularData->nodeType = CELLULAR_BS;
        }
        else if (strcmp(buf, "CELLULAR-SC") == 0)
        {
            macCellularData->nodeType = CELLULAR_SC;
        }
        else if (strcmp(buf, "CELLULAR-GATEWAY") == 0)
        {
            macCellularData->nodeType = CELLULAR_GATEWAY;
        }
        else if (strcmp(buf, "CELLULAR-AGGREGATED-NODE") == 0)
        {
            macCellularData->nodeType = CELLULAR_AGGREGATED_NODE;
        }
        else
        {
            sprintf(errorString,
                    "Node%d unknown node type: %s\n"
                    "CELLULAR-NODE-TYPE must be one of CELLULAR-MS, "
                    "CELLULAR-BS, CELLULAR-SC, CELLULAR-GATEWAY, or "
                    "CELLULAR-AGGREGATED-NODE\n",
                    node->nodeId,
                    buf);
            ERROR_ReportError(errorString);
        }
    }
    else
    {
        // node type not specified, assuming MS
        macCellularData->nodeType = CELLULAR_MS;
    }

    //optimization
    IO_ReadString(
                ANY_NODEID,
                ANY_DEST,
                nodeInput,
                "CELLULAR-ABSTRACT-OPTIMIZATION-LEVEL",
                &retVal,
                buf);

    if (retVal == TRUE && strcmp(buf, "MEDIUM") == 0)
    {
        macData->optLevel = CELLULAR_ABSTRACT_OPTIMIZATION_MEDIUM;
    }
    else
    {
        macData->optLevel = CELLULAR_ABSTRACT_OPTIMIZATION_LOW;
    }

    IO_ReadString(
                node->nodeId,
                ANY_DEST,
                nodeInput,
                "CELLULAR-STATISTICS",
                &retVal,
                buf);

    if ((retVal == FALSE) || (strcmp(buf, "YES") == 0))
    {
        macCellularData->collectStatistics = TRUE;
    }
    else if (retVal && strcmp(buf, "NO") == 0)
    {
        macCellularData->collectStatistics = FALSE;
    }
    else
    {
        ERROR_ReportWarning(
            "Value of CELLULAR-STATISTICS should be YES or NO! Default value"
            "YES is used.");
        macCellularData->collectStatistics = TRUE;
    }
    // If I am a base station, read my downlink and uplink control channels
    if (macCellularData->nodeType == CELLULAR_BS)
    {
        char string[MAX_STRING_LENGTH];

        IO_ReadString(node->nodeId,
                      ANY_ADDRESS,
                      nodeInput,
                      "CELLULAR-ABSTRACT-BS-CONTROL-CHANNEL-DOWNLINK",
                      &retVal,
                      string);

        if (retVal)
        {
            if (IO_IsStringNonNegativeInteger(string))
            {
                channelIndex = (Int32)strtoul(string, NULL, 10);
                if (channelIndex < node->numberChannels)
                {
                    macData->currentDLChannel = channelIndex;
                }
                else
                {
                    ERROR_ReportErrorArgs(
                        "CELLULAR-ABSTRACT-BS-CONTROL-CHANNEL-DOWNLINK"
                        " parameter should have an integer value between"
                        " 0 and %d or a valid channel name.",
                        node->numberChannels - 1);
                }
            }
            else if (isalpha(*string) && PHY_ChannelNameExists(node, string))
            {
                macData->currentDLChannel = 
                                    PHY_GetChannelIndexForChannelName(node,
                                                                      string);
            }
            else
            {
                ERROR_ReportErrorArgs(
                    "CELLULAR-ABSTRACT-BS-CONTROL-CHANNEL-DOWNLINK"
                    " parameter should have an integer value between"
                    " 0 and %d or a valid channel name.",
                    node->numberChannels - 1);
            }
        }
        else
        {
            ERROR_ReportError("CELLULAR-ABSTRACT-BS-CONTROL-CHANNEL-DOWNLINK"
                              " is not configured");
        }

        IO_ReadString(node->nodeId,
                      ANY_ADDRESS,
                      nodeInput,
                      "CELLULAR-ABSTRACT-BS-CONTROL-CHANNEL-UPLINK",
                      &retVal,
                      string);

        if (retVal)
        {
            if (IO_IsStringNonNegativeInteger(string))
            {
                channelIndex = (Int32)strtoul(string, NULL, 10);
                if (channelIndex < node->numberChannels)
                {
                    macData->currentULChannel = channelIndex;
                }
                else
                {
                    ERROR_ReportErrorArgs(
                        "CELLULAR-ABSTRACT-BS-CONTROL-CHANNEL-UPLINK"
                        " parameter should have an integer value between"
                        " 0 and %d or a valid channel name.",
                        node->numberChannels - 1);
                }
            }
            else if (isalpha(*string) && PHY_ChannelNameExists(node, string))
            {
                macData->currentULChannel = 
                                    PHY_GetChannelIndexForChannelName(node,
                                                                      string);
            }
            else
            {
                ERROR_ReportErrorArgs(
                    "CELLULAR-ABSTRACT-BS-CONTROL-CHANNEL-UPLINK"
                    " parameter should have an integer value between"
                    " 0 and %d or a valid channel name.",
                    node->numberChannels - 1);
            }
        }
        else
        {
            ERROR_ReportError("CELLULAR-ABSTRACT-BS-CONTROL-CHANNEL-UPLINK"
                              " is not configured");
        }


        if (DEBUG_CHANNEL)
        {
            printf("Node%d(BS), DL control ch = %d, UP control ch = %d\n",
                   node->nodeId,
                   macData->currentDLChannel,
                   macData->currentULChannel);
            fflush(stdout);
        }

        macData->associated = TRUE;
    }
    else if (macCellularData->nodeType == CELLULAR_MS)
    {
        // I am a MS, need to know the list of all downlink control channels

        macData->numDLChannels = 0;
        macData->dlChannelList =
            (int *) MEM_malloc(node->numberChannels * sizeof(int));

        // read all downlink control channels for scanning of BSs
        for (i = 0; i < nodeInput->numLines; i ++)
        {
            if (strcmp(nodeInput->variableNames[i],
                       "CELLULAR-ABSTRACT-BS-CONTROL-CHANNEL-DOWNLINK") == 0)
            {

                if (IO_IsStringNonNegativeInteger(nodeInput->values[i]))
                {
                    channelIndex = (Int32)strtoul(nodeInput->values[i],
                                                  NULL,
                                                  10);
                    if (channelIndex < node->numberChannels)
                    {
                        macData->dlChannelList[macData->numDLChannels] =
                                                                channelIndex;
                    }
                    else
                    {
                        ERROR_ReportErrorArgs(
                            "CELLULAR-ABSTRACT-BS-CONTROL-CHANNEL-DOWNLINK"
                            " parameter should have an integer value between"
                            " 0 and %d or a valid channel name.",
                            node->numberChannels - 1);
                    }
                }
                else if (isalpha(*nodeInput->values[i]) &&
                         PHY_ChannelNameExists(node, nodeInput->values[i]))
                {
                    macData->dlChannelList[macData->numDLChannels] = 
                        PHY_GetChannelIndexForChannelName(node,
                                                          nodeInput->values[i]);
                }
                else
                {
                    ERROR_ReportErrorArgs(
                        "CELLULAR-ABSTRACT-BS-CONTROL-CHANNEL-DOWNLINK"
                        " parameter should have an integer value between"
                        " 0 and %d or a valid channel name.",
                        node->numberChannels - 1);
                }

                macData->numDLChannels ++;
            }
        }

        if (DEBUG_CHANNEL)
        {
            printf("Node%d knows %d downlink control channels:",
                   node->nodeId, macData->numDLChannels);
        fflush(stdout);
            for (i = 0; i < macData->numDLChannels; i ++)
            {
                printf("%d ", macData->dlChannelList[i]);
                fflush(stdout);
            }
            printf("\n");
            fflush(stdout);
        }

        // init DL and UL in associated cell. No cell associated beginning
        macData->currentDLChannel = -1;
        macData->currentULChannel = -1;

        IO_ReadTime(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "CELLULAR-ABSTRACT-BS-CONTROL-INFORMATION-INTERVAL",
            &retVal,
            &bcchInterval);
        if (retVal == FALSE || bcchInterval <= 0)
        {
            bcchInterval = CELLULAR_ABSTRACT_BCCH_INTERVAL;
        }

        // init variables for channel scan in BCCH
        macData->scanIndex = 0;
        macData->numFramePerChannel =
            (int)(bcchInterval /
            (MAC_CELLULAR_ABSTRACT_DOWNLINK_CONTROL_INTERVAL *
             MAC_CELLULAR_ABSTRACT_BCCH_FRAC) + 1);
        macData->frameCount = 0;
        macData->associated = FALSE;
    }
    else
    {
        // cellular MAC can only be run on either BS or MS.
        ERROR_ReportError("Cellular MAC can only be run either MS or BS!");
    }

    macData->bufferPtr = NULL;
    macData->lastPkt = NULL;
    macData->bcchBufferPtr = NULL;
    macData->bcchLastPkt = NULL;

    for (i = 0; i < node->numberChannels; i ++)
    {
        MacCellularAbstractStopListening(
            node,
            macCellularData->myMacData->phyNumber,
            i);
    }

    // init the timer for the downlink control slot
    macData->dlSlotBeginTimer = MacCellularAbstractInitTimer(
                                    node,
                                    interfaceIndex,
                                    DOWNLINK_CONTROL_SLOT_BEGIN);

    // init the timer for the downlink control slot
    macData->dlSlotEndTimer = MacCellularAbstractInitTimer(
                                  node,
                                  interfaceIndex,
                                  DOWNLINK_CONTROL_SLOT_END);

    // init the timer for the uplink control slot
    macData->ulSlotBeginTimer = MacCellularAbstractInitTimer(
                                    node,
                                    interfaceIndex,
                                    UPLINK_CONTROL_SLOT_BEGIN);

    // init the timer for the uplink control slot
    macData->ulSlotEndTimer = MacCellularAbstractInitTimer(
                                  node,
                                  interfaceIndex,
                                  UPLINK_CONTROL_SLOT_END);

    // init tch splay tree
    macData->tchSplayTree.rootPtr = NULL;
    macData->tchSplayTree.leastPtr = NULL;
    macData->tchSplayTree.timeValue = CLOCKTYPE_MAX;

    if (macCellularData->nodeType == CELLULAR_BS)
    {
        // I am a BS
        MESSAGE_Send(node, macData->dlSlotBeginTimer, 0);
        MESSAGE_Send(node,
                     macData->ulSlotBeginTimer,
                     MAC_CELLULAR_ABSTRACT_UPLINK_DOWNLINK_DIFF);
    }
    else
    {
        MESSAGE_Send(node, macData->dlSlotBeginTimer, 0);
    }

#ifdef PARALLEL
    PARALLEL_SetProtocolIsNotEOTCapable(node);
    PARALLEL_SetMinimumLookaheadForInterface(
        node,
        5000);  // 0 is the delay used in PHY_StartTransmittingSignal
             // and is the minimum delay used in this interface.
//    PARALLEL_SetMinimumLookaheadForInterface(
//        node,
//        MAC_CELLULAR_ABSTRACT_TCH_DELAY +
//        node->macData[interfaceIndex]->propDelay);
#endif // PARALLEL
}


// /**
// FUNCTION   :: MacCellularAbstractFinalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacCellularAbstractFinalize(Node *node, int interfaceIndex)
{
    MacCellularData* macCellularData;
    MacCellularAbstractData *macData;
    char buf[MAX_STRING_LENGTH];

    if (DEBUG_MAC)
    {
        printf("Node %d: Cellular Abstract MAC finalizing\n", node->nodeId);
        fflush(stdout);
    }

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;
    macData = (MacCellularAbstractData*)
              macCellularData->macCellularAbstractData;

    // print out statistics if enabled
    if (macCellularData->myMacData->macStats == TRUE ||
        macCellularData->collectStatistics == TRUE)
    {
        // print out number of packets sent on downlink control channel
        sprintf(buf,
                "Number of packets sent on downlink control channel = %d",
                macData->stats.numPktSentOnDL);
        IO_PrintStat(node,
                     "MAC",
                     "Cellular Abstract",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out number of packets received on downlink control channel
        sprintf(
            buf,
            "Number of packets received on downlink control channel = %d",
            macData->stats.numPktRecvOnDL);
        IO_PrintStat(node,
                     "MAC",
                     "Cellular Abstract",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out number of packets sent on uplink control channel
        sprintf(buf,
                "Number of packets sent on uplink control channel = %d",
                macData->stats.numPktSentOnUL);
        IO_PrintStat(node,
                     "MAC",
                     "Cellular Abstract",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out number of packets received on uplink control channel
        sprintf(buf,
                "Number of packets received on uplink control channel = %d",
                macData->stats.numPktRecvOnUL);
        IO_PrintStat(node,
                     "MAC",
                     "Cellular Abstract",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out number of packets sent on TCH
        sprintf(buf, "Number of packets sent on TCH = %d",
                macData->stats.numPktSentOnTCH);
        IO_PrintStat(node,
                     "MAC",
                     "Cellular Abstract",
                     ANY_DEST,
                     interfaceIndex,
                     buf);

        // print out number of packets received on TCH
        sprintf(buf, "Number of packets received on TCH = %d",
                macData->stats.numPktRecvOnTCH);
        IO_PrintStat(node,
                     "MAC",
                     "Cellular Abstract",
                     ANY_DEST,
                     interfaceIndex,
                     buf);
    }
}


// /**
// FUNCTION   :: MacCellularAbstractLayer
// LAYER      :: MAC
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void MacCellularAbstractLayer(Node *node, int interfaceIndex, Message *msg)
{
    MacCellularData* macCellularData;
    MacCellularAbstractData* macData;

    macCellularData=(MacCellularData*)node->macData[interfaceIndex]->macVar;
    macData = (MacCellularAbstractData *)
              macCellularData->macCellularAbstractData;

    if (DEBUG_TIMER)
    {
        char clockStr[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), clockStr);
        printf("Node %d MAC: Cellular Abstract Process Event at %s\n",
               node->nodeId, clockStr);
        fflush(stdout);
    }

    switch(msg->eventType)
    {
        //Timer expired is the most called event, hence put ahead.
        case MSG_MAC_TimerExpired:
        {
            MacCellularAbstractTimer* timerInfo;

            timerInfo = (MacCellularAbstractTimer*)
                        MESSAGE_ReturnInfo(msg);

            switch (timerInfo->timerType)
            {
                case DOWNLINK_CONTROL_SLOT_BEGIN:
                {
                    clocktype txDelay;

                    if (DEBUG_TIMER)
                    {
                        printf("    Downlink Control Slot Begin timer\n");
                        fflush(stdout);
                    }

                    // beginning of the downlink control slot
                    // BS can broadcast the packet and MS should switch to
                    // listen to this channel

                    // decide the channel type, only BCCH and PAGCH are
                    // recognized for downlink in this implementation.
                    // All non-BCCH pkts will be sent on PAGCH slot.

                    if (macCellularData->nodeType == CELLULAR_BS)
                    {
                        // I am a base station

                        macData->chTypeCount ++;
                        macData->chTypeCount %= MAC_CELLULAR_ABSTRACT_BCCH_FRAC;
                        if (macData->chTypeCount == 0)
                        {
                            macData->chType =
                                CELLULAR_ABSTRACT_CONTROL_CHANNEL_BCCH;

                            if (DEBUG_TIMER)
                            {
                                printf("    This is a BCCH slot\n");
                                fflush(stdout);
                            }
                        }
                        else
                        {
                            macData->chType =
                                CELLULAR_ABSTRACT_CONTROL_CHANNEL_PAGCH;

                            if (DEBUG_TIMER)
                            {
                                printf("    This is a PAGCH slot\n");
                                fflush(stdout);
                            }
                        }

                        if ((macData->chType ==
                             CELLULAR_ABSTRACT_CONTROL_CHANNEL_BCCH &&
                             macData->bcchBufferPtr != NULL) ||
                            (macData->chType ==
                             CELLULAR_ABSTRACT_CONTROL_CHANNEL_PAGCH &&
                             macData->bufferPtr != NULL))
                        {
                            // buffer is no empty, send out the packet

                            Message* tmpMsgPtr;

                            if (DEBUG_MAC)
                            {
                                char clockStr[MAX_STRING_LENGTH];
                                ctoa(node->getNodeTime(), clockStr);
                                printf("Node%d tx a packet at time %s\n",
                                       node->nodeId, clockStr);
                                fflush(stdout);
                            }

                            if (macData->chType ==
                                CELLULAR_ABSTRACT_CONTROL_CHANNEL_BCCH)
                            {
                                // BCCH channel
                                tmpMsgPtr = macData->bcchBufferPtr;
                                macData->bcchBufferPtr =
                                    macData->bcchBufferPtr->next;
                                if (macData->bcchBufferPtr == NULL)
                                {
                                    macData->bcchLastPkt = NULL;
                                }
                            }
                            else
                            {
                                // non BCCH DL control channel
                                tmpMsgPtr = macData->bufferPtr;
                                macData->bufferPtr = macData->bufferPtr->next;
                                if (macData->bufferPtr == NULL)
                                {
                                    macData->lastPkt = NULL;
                                }
                            }

                            txDelay = PHY_GetTransmissionDelay(
                                          node,
                                          macCellularData->myMacData->phyNumber,
                                          MESSAGE_ReturnPacketSize(tmpMsgPtr));
                            txDelay += macCellularData->myMacData->propDelay;

                            if (txDelay +
                                MAC_CELLULAR_ABSTRACT_MAC_PROP_DELAY_COMPENSATION >=
                                MAC_CELLULAR_ABSTRACT_DOWNLINK_CONTROL_DURATION)
                            {
                                ERROR_ReportError("DL control slot is too small to tx the packet "
                                                  " Increase MAC_CELLULAR_ABSTRACT_DOWNLINK_CONTROL_DURATION "
                                                  " Or increase TX-DATA-RATE");
                            }

                            PHY_SetTransmissionChannel(
                                node,
                                macCellularData->myMacData->phyNumber,
                                macData->currentDLChannel);

                            PHY_StartTransmittingSignal(
                                node,
                                macCellularData->myMacData->phyNumber,
                                tmpMsgPtr,
                                FALSE,
                                0);

                            if (DEBUG_PACKET)
                            {
                                char clockStr[MAX_STRING_LENGTH];

                                ctoa(node->getNodeTime(), clockStr);
                                if (macData->chType ==
                                    CELLULAR_ABSTRACT_CONTROL_CHANNEL_BCCH)
                                {
                                    printf("Node%d start txing BCCH packet "
                                           "on channel %d at time %s\n",
                                           node->nodeId,
                                           macData->currentDLChannel,
                                           clockStr);
                                    fflush(stdout);
                                }
                                else
                                {
                                    printf("Node%d start txing PAGCH packet "
                                           "on channel %d at time %s\n",
                                           node->nodeId,
                                           macData->currentDLChannel,
                                           clockStr);
                                    fflush(stdout);
                                }
                            }

                            macData->status = MAC_CELLULAR_TRANSMITTING;

                            // increase the stat
                            macData->stats.numPktSentOnDL ++;
                        }

                        // set the next timer, BS schedules all CCHs
                        MESSAGE_Send(
                            node,
                            msg,
                            MAC_CELLULAR_ABSTRACT_DOWNLINK_CONTROL_INTERVAL);
                    }
                    else if (macCellularData->nodeType == CELLULAR_MS)
                    {
                        // I am a mobile station

                        // stop listening to current DL first
                        if (macData->currentDLChannel !=
                            CELLULAR_ABSTRACT_INVALID_CHANNEL_ID)
                        {
                            MacCellularAbstractStopListening(
                                node,
                                macCellularData->myMacData->phyNumber,
                                macData->currentDLChannel);
                        }

                        if (macData->optLevel !=
                                CELLULAR_ABSTRACT_OPTIMIZATION_LOW)
                        {
                            break;
                        }

                        int listenChannel;

                        macData->chType =
                            CELLULAR_ABSTRACT_CONTROL_CHANNEL_BCCH;

                        macData->frameCount ++;
                        if (macData->frameCount > macData->numFramePerChannel)
                        { // scan next channel
                            macData->scanIndex ++;
                            if (macData->scanIndex >= macData->numDLChannels)
                            {
                                macData->scanIndex = 0;
                            }
                            macData->frameCount = 1;
                        }

                        listenChannel =
                            macData->dlChannelList[macData->scanIndex];

                        MacCellularAbstractStartListening(
                            node,
                            macCellularData->myMacData->phyNumber,
                            listenChannel);
                        macData->status = MAC_CELLULAR_RECEIVING;

                        // set the next timer, MS only schedules BCCHs, it
                        // always listening to its current BS other than
                        // BCCH or RACCH, this is for optimize the perf.
                        MESSAGE_Send(
                            node,
                            msg,
                            MAC_CELLULAR_ABSTRACT_BCCH_FRAC *
                            MAC_CELLULAR_ABSTRACT_DOWNLINK_CONTROL_INTERVAL);
                    }
                    else
                    {
                        // unknown type, should not run cellular MAC
                        ERROR_ReportWarning(
                            "Unknown node type to abstract cellular MAC!");
                    }

                    // set the end of slot timer
                    MESSAGE_Send(
                        node,
                        macData->dlSlotEndTimer,
                        MAC_CELLULAR_ABSTRACT_DOWNLINK_CONTROL_DURATION);

                    break;
                }

                case DOWNLINK_CONTROL_SLOT_END:
                {
                    int listenChannel;

                    if (DEBUG_TIMER)
                    {
                        printf("    Downlink Control Slot End timer\n");
                        fflush(stdout);
                    }

                    // end of the downlink control slot.
                    // MS and BS should stop listening to this channel

                    if (macCellularData->nodeType == CELLULAR_BS)
                    {
                        listenChannel = macData->currentDLChannel;
                    }
                    else
                    {
                        listenChannel =
                            macData->dlChannelList[macData->scanIndex];
                    }

                    if (listenChannel !=
                        CELLULAR_ABSTRACT_INVALID_CHANNEL_ID)
                    {
                        MacCellularAbstractStopListening(
                            node,
                            macCellularData->myMacData->phyNumber,
                            listenChannel);
                    }

                    macData->status = MAC_CELLULAR_IDLE;
                    if (macData->cellReselected)
                    {
                        macData->cellReselected = FALSE;
                        macData->currentDLChannel = macData->nextDLChannel;
                        macData->currentULChannel = macData->nextULChannel;
                    }

                    if (macCellularData->nodeType == CELLULAR_MS)
                    {
                        // for optimization, always listen to current BS
                        // without scheduling timer messages
                        if (macData->currentDLChannel !=
                            CELLULAR_ABSTRACT_INVALID_CHANNEL_ID)
                        {
                            MacCellularAbstractStartListening(
                                node,
                                macCellularData->myMacData->phyNumber,
                                macData->currentDLChannel);
                        }
                    }
                    break;
                }

                case UPLINK_CONTROL_SLOT_BEGIN:
                {
                    clocktype txDelay;

                    if (DEBUG_TIMER)
                    {
                        printf("    Uplink Control Slot Begin timer\n");
                        fflush(stdout);
                    }

                    // beginning of the uplink control slot
                    // MS can start transmitting on this channel
                    // BS should start litening to this channel
                    // Only RACCH is supported for uplink

                    if (macCellularData->nodeType == CELLULAR_BS)
                    {
                        macData->status = MAC_CELLULAR_RECEIVING;

                        // I am a base station, switch to listen to uplink
                        // control channel.
                        MacCellularAbstractStartListening(
                            node,
                            macCellularData->myMacData->phyNumber,
                            macData->currentULChannel);

                        // set the next timer
                        MESSAGE_Send(
                            node,
                            msg,
                            MAC_CELLULAR_ABSTRACT_UPLINK_CONTROL_INTERVAL);
                    }
                    else if (macCellularData->nodeType == CELLULAR_MS)
                    {
                        // stop listening to current DL first
                        if (macData->currentDLChannel !=
                            CELLULAR_ABSTRACT_INVALID_CHANNEL_ID)
                        {
                            MacCellularAbstractStopListening(
                                node,
                                macCellularData->myMacData->phyNumber,
                                macData->currentDLChannel);
                        }

                        // I am a mobile station
                        if (macData->associated &&
                            macData->bufferPtr != NULL)
                        {
                            Message* tmpMsgPtr;
                            MacCellularAbstractHeader *headerPtr;

                            macData->status = MAC_CELLULAR_TRANSMITTING;

                            tmpMsgPtr = macData->bufferPtr;
                            macData->bufferPtr = macData->bufferPtr->next;
                            if (macData->bufferPtr == NULL)
                            {
                                macData->lastPkt = NULL;
                            }

                            txDelay = PHY_GetTransmissionDelay(
                                          node,
                                          macCellularData->myMacData->phyNumber,
                                          MESSAGE_ReturnPacketSize(tmpMsgPtr));
                            txDelay += macCellularData->myMacData->propDelay;

                            if (txDelay +
                                MAC_CELLULAR_ABSTRACT_MAC_PROP_DELAY_COMPENSATION >=
                                MAC_CELLULAR_ABSTRACT_UPLINK_CONTROL_DURATION)
                            {
                                ERROR_ReportError("UL control slot is too small to complete tx the packet"
                                                   "Increase MAC_CELLULAR_ABSTRACT_UPLINK_CONTROL_DURATION "
                                                   "Or Increase TX-DATA-RATE");
                            }

                            // buffer is no empty, send out the packet
                            PHY_SetTransmissionChannel(
                                node,
                                macCellularData->myMacData->phyNumber,
                                macData->currentULChannel);

                            // only send to BS
                            headerPtr = (MacCellularAbstractHeader *)
                                        MESSAGE_ReturnPacket(tmpMsgPtr);
                            PHY_StartTransmittingSignal(
                                node,
                                macCellularData->myMacData->phyNumber,
                                tmpMsgPtr,
                                FALSE,
                                (clocktype)0,
                                (NodeId)headerPtr->destId);

                            if (DEBUG_PACKET)
                            {
                                char clockStr[MAX_STRING_LENGTH];

                                ctoa(node->getNodeTime(), clockStr);
                                printf("Node%d(MS) tx a RACCH packet on "
                                       "channel %d at time %s\n",
                                       node->nodeId,
                                       macData->currentULChannel,
                                       clockStr);
                                fflush(stdout);
                            }

                            // increase the stat
                            macData->stats.numPktSentOnUL ++;
                        }

                        if (macData->lastPkt != NULL)
                        {
                            // set the next timer
                            MESSAGE_Send(
                                node,
                                msg,
                                MAC_CELLULAR_ABSTRACT_UPLINK_CONTROL_INTERVAL);
                        }
                    }
                    else
                    {
                        // unknown type, should not run cellular MAC
                        ERROR_ReportWarning(
                            "Unknown node type to abstract cellular MAC!");
                    }

                    // set the end of slot timer
                    MESSAGE_Send(
                        node,
                        macData->ulSlotEndTimer,
                        MAC_CELLULAR_ABSTRACT_UPLINK_CONTROL_DURATION);

                    break;
                }

                case UPLINK_CONTROL_SLOT_END:
                {
                    if (DEBUG_TIMER)
                    {
                        printf("    Uplink Control Slot End timer\n");
                        fflush(stdout);
                    }

                    if (macCellularData->nodeType == CELLULAR_BS ||
                        macData->associated)
                    {
                        // end of the uplink control slot.
                        // BS & MS should stop listening to this channel
                        if (macData->currentULChannel !=
                            CELLULAR_ABSTRACT_INVALID_CHANNEL_ID)
                        {
                            MacCellularAbstractStopListening(
                                node,
                                macCellularData->myMacData->phyNumber,
                                macData->currentULChannel);
                        }
                    }

                    macData->status = MAC_CELLULAR_IDLE;

                    if (macData->cellReselected)
                    {
                        macData->cellReselected = FALSE;
                        macData->currentDLChannel = macData->nextDLChannel;
                        macData->currentULChannel = macData->nextULChannel;
                    }

                    if (macCellularData->nodeType == CELLULAR_MS)
                    {
                        // for optimization, always listen to current BS
                        // without scheduling timer messages
                        if (macData->currentDLChannel !=
                            CELLULAR_ABSTRACT_INVALID_CHANNEL_ID)
                        {
                            MacCellularAbstractStartListening(
                                node,
                                macCellularData->myMacData->phyNumber,
                                macData->currentDLChannel);
                        }
                    }

                    break;
                }


                default:
                {
                    ERROR_ReportWarning("Unknown timer!");
                    MESSAGE_Free(node, msg);

                    break;
                }
            }

            break;
        }

        //from network layer for cellular related
        case MSG_MAC_CELLULAR_FromNetworkScanSignalPerformMeasurement:
        {
            if (DEBUG_TIMER)
            {
                printf("    ScanSignalPerformMeasurement event\n");
                fflush(stdout);
            }

            if (macCellularData->nodeType == CELLULAR_MS
                        && macData->optLevel ==
                            CELLULAR_ABSTRACT_OPTIMIZATION_LOW)
            {
                Message *scanSignalTimer;
                clocktype delay;

                scanSignalTimer= MESSAGE_Alloc(node,
                                    MAC_LAYER,
                                    MAC_PROTOCOL_CELLULAR,
                                    MSG_MAC_CELLULAR_ScanSignalTimer);

                delay = CELLULAR_ABSTRACT_SIGNAL_MEASUREMENT_TIME +
                        (clocktype)(RANDOM_erand(macData->seed) * SECOND);

                MESSAGE_Send(node,
                             scanSignalTimer,
                             delay);
            }//only MS could have such message from network layer

            MESSAGE_Free(node, msg);

            break;
        }

        //message timer from mac layer itself
        case MSG_MAC_CELLULAR_ScanSignalTimer:
        {
            if (DEBUG_TIMER)
            {
                printf("    ScanSignalTimer event\n");
                fflush(stdout);
            }

            // this timer can only happen on a MS
            ERROR_Assert(macCellularData->nodeType == CELLULAR_MS,
                         "Wrong timer type!");

            //check if we got any broadcasted control inforamtion
            if (macData->downlinkMeasurement != NULL)
            {
                Message *macMeasurementReport;
                BOOL noSignal;

                macMeasurementReport =
                    MESSAGE_Alloc(
                        node,
                        NETWORK_LAYER,
                        NETWORK_PROTOCOL_CELLULAR,
                        MSG_NETWORK_CELLULAR_FromMacMeasurementReport);

                MacCellularAbstractBuildDownlinkMeasurementMsgToNetwork(
                    node,
                    interfaceIndex,
                    &macMeasurementReport,
                    &noSignal);

                if (noSignal)
                {
                    MESSAGE_SetEvent(
                        macMeasurementReport,
                        MSG_NETWORK_CELLULAR_FromMacNetworkNotFound);
                }

                MESSAGE_Send(node, macMeasurementReport, 0);
            }
            else
            {
                Message *networkNotFoundMsg;

                networkNotFoundMsg =
                    MESSAGE_Alloc(
                        node,
                        NETWORK_LAYER,
                        NETWORK_PROTOCOL_CELLULAR,
                        MSG_NETWORK_CELLULAR_FromMacNetworkNotFound);

                MESSAGE_Send(node, networkNotFoundMsg, 0);
            }

            // setup the next timer.
            if (macData->commActive)
            {
                // active, use short interval
                MESSAGE_Send(
                    node,
                    msg,
                    MAC_CELLULAR_ABSTRACT_SIGNAL_MEASUREMENT_SHORT_INTERVAL);
            }
            else
            {
                // idle, use long interval
                MESSAGE_Send(
                    node,
                    msg,
                    MAC_CELLULAR_ABSTRACT_SIGNAL_MEASUREMENT_LONG_INTERVAL);
            }

            break;
        }

        case MSG_MAC_CELLULAR_FromNetworkCellSelected:
        {
            CellularAbstractSelectCellInfo *cellInfo;

            if (DEBUG_MAC)
            {
                printf("    Cell selected event\n");
                fflush(stdout);
            }

            cellInfo = (CellularAbstractSelectCellInfo *)
                       MESSAGE_ReturnInfo(msg);

            if (DEBUG_HANDOVER)
            {
                char clockStr[MAX_STRING_LENGTH];

                ctoa(node->getNodeTime(), clockStr);
                printf("Node%d handover from cell (id=%d, sid=%d, DL=%d, "
                       "UL=%d) to cell (id=%d, sid=%d, DL=%d, UL=%d), "
                       "status = %d, time = %s\n",
                       node->nodeId,
                       macData->selectedBsId,
                       macData->selectedSectorId,
                       macData->currentDLChannel,
                       macData->currentULChannel,
                       cellInfo->bsNodeId,
                       cellInfo->sectorId,
                       cellInfo->controlDLChannelIndex,
                       cellInfo->controlULChannelIndex,
                       macData->status,
                       clockStr);
                fflush(stdout);
            }

            // The MS has handover to another cell, update information
            macData->associated = TRUE;
            macData->selectedBsId = cellInfo->bsNodeId;
            macData->selectedSectorId = cellInfo->sectorId;

            if (macData->status == MAC_CELLULAR_IDLE)
            {
                if (macData->currentDLChannel !=
                    cellInfo->controlDLChannelIndex &&
                    macData->currentDLChannel !=
                    CELLULAR_ABSTRACT_INVALID_CHANNEL_ID)
                {
                    MacCellularAbstractStopListening(
                        node,
                        macCellularData->myMacData->phyNumber,
                        macData->currentDLChannel);
                }

                // update the DL and UL channels
                macData->currentDLChannel = cellInfo->controlDLChannelIndex;
                macData->currentULChannel = cellInfo->controlULChannelIndex;

                // for optimization, always listen to current BS
                // without scheduling timer messages
                if (macData->currentDLChannel !=
                    CELLULAR_ABSTRACT_INVALID_CHANNEL_ID)
                {
                    MacCellularAbstractStartListening(
                        node,
                        macCellularData->myMacData->phyNumber,
                        macData->currentDLChannel);
                }
            }
            else
            {
                macData->cellReselected = TRUE;
                macData->nextDLChannel = cellInfo->controlDLChannelIndex;
                macData->nextULChannel = cellInfo->controlULChannelIndex;
            }

            MESSAGE_Free(node, msg);

            break;
        }

        case MSG_MAC_CELLULAR_FromNetworkTransactionActive:
        {
            if (DEBUG_MAC)
            {
                printf("    TransactionActive event\n");
                fflush(stdout);
            }

            macData->commActive = TRUE;

            MESSAGE_Free(node, msg);

            break;
        }

        case MSG_MAC_CELLULAR_FromNetworkNoTransaction:
        {
            if (DEBUG_MAC)
            {
                printf("    NoTransaction event\n");
                fflush(stdout);
            }

            macData->commActive = FALSE;

            MESSAGE_Free(node, msg);

            break;
        }

        case MSG_MAC_CELLULAR_FromNetwork:
        {
            CellularAbstractNetowrkToMacMsgInfo *msgInfo;
            MacCellularAbstractHeader *headerPtr;

            if (DEBUG_TIMER)
            {
                printf("    FromNetwork event\n");
                fflush(stdout);
            }


            msgInfo = (CellularAbstractNetowrkToMacMsgInfo *)
                      MESSAGE_ReturnInfo(msg);

            MESSAGE_AddHeader(node,
                              msg,
                              sizeof(MacCellularAbstractHeader),
                              TRACE_CELLULAR);
            headerPtr = (MacCellularAbstractHeader *)
                        MESSAGE_ReturnPacket(msg);

            headerPtr->chType = msgInfo->channelType;
            if (msgInfo->recvSet == CELLULAR_ABSTRACT_RECEIVER_ALL_MS ||
                msgInfo->recvSet == CELLULAR_ABSTRACT_RECEIVER_ALL_BS_MS)
            {
                headerPtr->destId = ANY_DEST;
            }
            else if (msgInfo->recvSet == CELLULAR_ABSTRACT_RECEIVER_SINGLE)
            {
                headerPtr->destId = msgInfo->recvId;
            }
            else
            {
                headerPtr->destId = ANY_DEST;
                ERROR_ReportWarning("Cellular abstract MAC: Unknow info!");
            }

            if (headerPtr->chType == CELLULAR_ABSTRACT_CONTROL_CHANNEL_TCH)
            {
                // traffic control packets, will be sent to dest directly
                MacCellularAbstractTransmitOnTCH(node, interfaceIndex, msg);
            }
            else
            {
                BOOL buffEmpty = FALSE;

                if (macData->bcchLastPkt == NULL && macData->lastPkt == NULL)
                {
                    buffEmpty = TRUE;
                }

                // control channel packets, will be sent on control channel
                if (macCellularData->nodeType == CELLULAR_MS &&
                    macData->associated == FALSE)
                {
                    // not attached to a BS yet
                    MESSAGE_Free(node, msg);
                }
                else if (macCellularData->nodeType == CELLULAR_BS &&
                         headerPtr->chType ==
                         CELLULAR_ABSTRACT_CONTROL_CHANNEL_BCCH)
                {
                    msg->next = NULL;

                    // BCCH packet will be put into BCCH packet buffer
                    if (macData->bcchLastPkt == NULL)
                    {
                        // empty BCCH buffer
                        macData->bcchBufferPtr = msg;
                        macData->bcchLastPkt = msg;
                    }
                    else
                    {
                        macData->bcchLastPkt->next = msg;
                        macData->bcchLastPkt = msg;
                    }
                }
                else
                {
                    msg->next = NULL;

                    // non BCCH packet, put into regular pkt buffer
                    if (macData->lastPkt == NULL)
                    {
                        // empty buffer
                        macData->bufferPtr = msg;
                        macData->lastPkt = msg;
                    }
                    else
                    {
                        macData->lastPkt->next = msg;
                        macData->lastPkt = msg;
                    }
                }

                if (buffEmpty && (macData->bcchLastPkt != NULL ||
                    macData->lastPkt != NULL))
                {
                    // buffer just changed from empty to non-empty
                    // we should start the timer to schedule the control
                    // slot to send out the packet
                    clocktype delay;
                    clocktype currentTime = node->getNodeTime();

                    if (macCellularData->nodeType == CELLULAR_MS)
                    {
                        // this is a MS, schedule start of uplink slot
                        if (currentTime <=
                            MAC_CELLULAR_ABSTRACT_UPLINK_DOWNLINK_DIFF)
                        {
                            delay = MAC_CELLULAR_ABSTRACT_UPLINK_DOWNLINK_DIFF -
                                    currentTime;
                        }
                        else
                        {
                            delay = MAC_CELLULAR_ABSTRACT_UPLINK_CONTROL_INTERVAL -
                                    ((currentTime -
                                    MAC_CELLULAR_ABSTRACT_UPLINK_DOWNLINK_DIFF) %
                                    MAC_CELLULAR_ABSTRACT_UPLINK_CONTROL_INTERVAL);
                        }

                        MESSAGE_Send(node,
                                     macData->ulSlotBeginTimer,
                                     delay);
                    }
                }
            }

            break;
        }
#if 0
        case MSG_MAC_CELLULAR_FromTch:
        {
            if (DEBUG_TIMER||DEBUG_PROGRESS)
            {
                printf("at %015lld node %d    Process TCH packet\n",node->getNodeTime(),node->nodeId);
                fflush(stdout);
            }

            // packets on TCH are directly sent
            MacCellularAbstractReceivePacketFromPhy(node, interfaceIndex, msg);

            break;
        }
#endif
        case MSG_MAC_CELLULAR_ProcessTchMessages:
        {
            MacCellularProcessTchMessages(node, interfaceIndex);
            MESSAGE_Free(node, msg);
            break;
        }

        default:
        {
            printf("CELLULAR ABSTRACT MAC: Protocol = %d, EventType = %d\n",
                   MESSAGE_GetProtocol(msg), msg->eventType);
            MESSAGE_Free(node, msg);
            assert(FALSE);

            break;
        }
    }

}


// /**
// FUNCTION   :: MacCellularAbstractNetworkLayerHasPacketToSend
// LAYER      :: MAC
// PURPOSE    :: Called when network layer buffers transition from empty.
//               This function is not supported in the abstract cellular MAC
// PARAMETERS ::
// + node             : Node*                    : Pointer to node.
// + interfaceIndex   : int                      : interface running this MAC
// + cellularAbstractMac : MacCellularAbstractData* : Pointer to abstract
//                                                    cellular MAC structure
// RETURN     :: void : NULL
// **/
void MacCellularAbstractNetworkLayerHasPacketToSend(
         Node *node,
         int interfaceIndex,
         MacCellularAbstractData *cellularAbstractMac)
{
    if (DEBUG_MAC)
    {
        printf("Node %d Cellular Abstract MAC: network has pkt to send\n",
               node->nodeId);
        fflush(stdout);
    }
}


// /**
// FUNCTION   :: MacCellularAbstractReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : interface running this MAC
// + msg              : Message*          : Packet received from PHY
// RETURN     :: void : NULL
// **/
void MacCellularAbstractReceivePacketFromPhy(
         Node* node,
         int interfaceIndex,
         Message* msg)
{
    MacCellularData *macCellularData;
    MacCellularAbstractData *macData;
    MacCellularAbstractHeader *headerPtr;

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;
    macData = (MacCellularAbstractData *)
              macCellularData->macCellularAbstractData;

    headerPtr = (MacCellularAbstractHeader *) MESSAGE_ReturnPacket(msg);

    if (DEBUG_PACKET)
    {
        char clockStr[MAX_STRING_LENGTH];
        int chIndex = -1;
        int i;

        for (i = 0; i < node->numberChannels; i ++)
        {
            if (PHY_IsListeningToChannel(
                    node,
                    node->macData[interfaceIndex]->phyNumber,
                    i))
            {
                chIndex = i;
                break;
            }
        }

        ctoa(node->getNodeTime(), clockStr);

        if (headerPtr->chType == CELLULAR_ABSTRACT_CONTROL_CHANNEL_RACCH)
        {
            printf("Node%d recved a RACCH packet (dest = %u) on "
                   "channel %d at %s\n",
                   node->nodeId, headerPtr->destId, chIndex, clockStr);
            fflush(stdout);
        }
        else if (headerPtr->chType == CELLULAR_ABSTRACT_CONTROL_CHANNEL_BCCH)
        {
            printf("Node%d recved a BCCH packet (dest = %u) on "
                   "channel %d at %s\n",
                   node->nodeId, headerPtr->destId, chIndex, clockStr);
            fflush(stdout);
        }
        else if (headerPtr->chType == CELLULAR_ABSTRACT_CONTROL_CHANNEL_PAGCH)
        {
            printf("Node%d recved a PAGCH packet (dest = %u) on "
                   "channel %d at %s\n",
                   node->nodeId, headerPtr->destId, chIndex, clockStr);
            fflush(stdout);
        }
        else if (headerPtr->chType == CELLULAR_ABSTRACT_CONTROL_CHANNEL_TCH)
        {
            printf("Node%d recved a TCH packet (dest = %u) on "
                   "channel %d at %s\n",
                   node->nodeId, headerPtr->destId, chIndex, clockStr);
            fflush(stdout);
        }
        else
        {
            printf("Node%d recved a packet(dest=%u) with unknow type (%d) "
                   "on channel %d at %s\n",
                   node->nodeId, headerPtr->destId, headerPtr->chType,
                   chIndex, clockStr);
            fflush(stdout);
        }
    }

    if (macCellularData->nodeType == CELLULAR_MS)
    {
        if (headerPtr->destId == ANY_DEST ||
            headerPtr->destId == node->nodeId)
        {
            //pass onto the netowrk layer
            MESSAGE_RemoveHeader(node,
                                 msg,
                                 sizeof(MacCellularAbstractHeader),
                                 TRACE_CELLULAR);

            if (headerPtr->chType == CELLULAR_ABSTRACT_CONTROL_CHANNEL_BCCH)
            {
                if (DEBUG_MAC)
                {
                    char timeStr[MAX_STRING_LENGTH];

                    ctoa(node->getNodeTime(), timeStr);
                    printf("node %d MAC: Process Event:BCCH, scanIndex=%d, "
                           "channel=%d at time %s\n",
                           node->nodeId,
                           macData->scanIndex,
                           macData->dlChannelList[macData->scanIndex],
                           timeStr);
                    fflush(stdout);
                }

                //peek at the content, update themeasurement
                MacCellularAbstractMeasureBCCHSignal(
                    node,
                    macCellularData->myMacData->interfaceIndex,
                    msg);
            }

            // increase the statistics
            if (headerPtr->chType == CELLULAR_ABSTRACT_CONTROL_CHANNEL_TCH)
            {
                macData->stats.numPktRecvOnTCH ++;
            }
            else
            {
                macData->stats.numPktRecvOnDL ++;
            }

            MAC_HandOffSuccessfullyReceivedPacket(
                node,
                macCellularData->myMacData->interfaceIndex,
                msg,
                ANY_ADDRESS);
        }
        else
        {
            // not my packet, free it.
            MESSAGE_Free(node, msg);
        }
    }
    else if (macCellularData->nodeType == CELLULAR_BS)
    {
        if (headerPtr->destId == ANY_DEST ||
            headerPtr->destId == node->nodeId)
        {
            if (headerPtr->chType == CELLULAR_ABSTRACT_CONTROL_CHANNEL_RACCH)
            {
                if (DEBUG_MAC)
                {
                    printf("node %d: Process Event:RACCH \n",node->nodeId);
                    fflush(stdout);
                }
            }

            // increase the statistics
            if (headerPtr->chType == CELLULAR_ABSTRACT_CONTROL_CHANNEL_TCH)
            {
                macData->stats.numPktRecvOnTCH ++;
            }
            else
            {
                macData->stats.numPktRecvOnUL ++;
            }

            // pass onto the network layer
            MESSAGE_RemoveHeader(node,
                                 msg,
                                 sizeof(MacCellularAbstractHeader),
                                 TRACE_CELLULAR);
            MAC_HandOffSuccessfullyReceivedPacket(
                node,
                macCellularData->myMacData->interfaceIndex,
                msg,
                ANY_ADDRESS);

            if (DEBUG_MAC)
            {
                printf("node %d BS MAC : Process Event:From Phy\n",
                       node->nodeId);
                fflush(stdout);
            }
        }
    }
    else
    {
        ERROR_ReportWarning("Wrong node type for cellular MAC!");
        MESSAGE_Free(node, msg);
    }
}

#ifdef PARALLEL //Parallel

// /**
// FUNCTION   :: MacCellularAbstractReceiveTCH
// LAYER      :: MAC
// PURPOSE    :: Receive a TCH signal from a remote partition
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Packet received from PHY
// RETURN     :: void : NULL
// **/
void MacCellularAbstractReceiveTCH(
         Node* node,
         Message* msg) {

    int i;
    for (i = 0; i < node->numberInterfaces; i ++)
    {
        if (node->macData[i]->macProtocol ==
            MAC_PROTOCOL_CELLULAR)
        {
            break;
        }
    }

    if (i >= node->numberInterfaces)
    { // destination node is not running Cellular MAC
        char errMsg[MAX_STRING_LENGTH];

        sprintf(errMsg, "Dest node%d is not running cellular MAC!",
                node->nodeId);
        ERROR_ReportWarning(errMsg);

        MESSAGE_Free(node, msg);
    }
    else
    {
        // corresponding interface on dest is found

        MESSAGE_SetLayer(msg, MAC_LAYER, 0);
        MESSAGE_SetInstanceId(msg, (short) i);
#if 0
        MESSAGE_Send(node,
                     msg,
                     msg->eventTime - node->getNodeTime());
#endif
        MacCellularScheduleTCH(
            node,
            msg,
            msg->eventTime - node->getNodeTime());
    }
}
#endif //endParallel

// /**
// FUNCTION   :: MacCellularScheduleTCH
// LAYER      :: MAC
// PURPOSE    :: Scheudle TCH transmission
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + msg              : Message*          : Packet received from PHY
// + delay            : clocktype         : delay for the TCH transmission
// RETURN     :: void : NULL
// **/
void MacCellularScheduleTCH(
    Node* node,
    Message* msg,
    clocktype delay)
{
    // add update into own splay of events

    SimpleSplayNode* splayNode = SPLAY_AllocateNode();
    splayNode->element = msg;
    splayNode->timeValue = node->getNodeTime() + delay;

    BOOL newTime;

    MacCellularSplayInsert(
        &(((MacCellularData*)
            node->macData[MESSAGE_GetInstanceId(msg)]->macVar)
         ->macCellularAbstractData->tchSplayTree),
        splayNode,
        &newTime);

    // first update at this time
    if (newTime)
    {
        Message* newMsg =
            MESSAGE_Alloc(
                node,
                MAC_LAYER,
                MAC_PROTOCOL_CELLULAR,
                MSG_MAC_CELLULAR_ProcessTchMessages);

        MESSAGE_SetInstanceId(newMsg, MESSAGE_GetInstanceId(msg));

        newMsg->eventTime = node->getNodeTime() + delay;

        MESSAGE_Send(node, newMsg, delay);
    }
}

// /**
// FUNCTION   :: MacCellularSplayInsert
// LAYER      :: MAC
// PURPOSE    ::  modified insert function, based on SPLAY_Insert
//                secondary ordering based on originatingNodeId
//                sets if it is the first update with the given time
// PARAMETERS ::
// + tree             : SimpleSplayTree*  : Pointer to the splaytree
// + splayNodePtr     : SimpleSplayNode*  : Point to the node
// + newTime          : BOOL*             : New Time or not
// RETURN     :: void : NULL
// **/
void MacCellularSplayInsert(
    SimpleSplayTree* tree,
    SimpleSplayNode *splayNodePtr,
    BOOL* newTime)
{
    SimpleSplayTree *splayPtr;

    splayPtr = tree;

    *newTime = TRUE;

    if (splayPtr->rootPtr == 0) {
        splayPtr->rootPtr = splayNodePtr;
        splayPtr->leastPtr = splayNodePtr;
        tree->timeValue = splayNodePtr->timeValue;
    }
    else {
        BOOL itemInserted = FALSE;
        SimpleSplayNode *currentPtr = splayPtr->rootPtr;

        while (itemInserted == FALSE) {
            BOOL special = FALSE;
            if (splayNodePtr->timeValue == currentPtr->timeValue) {
                *newTime = FALSE;

                if (((Message*) splayNodePtr->element)->originatingNodeId
                    < ((Message*) currentPtr->element)->originatingNodeId) {
                    special = TRUE;
                }
            }
            if (splayNodePtr->timeValue < currentPtr->timeValue
                || special == TRUE) {
                if (currentPtr->leftPtr == NULL) {
                    itemInserted = TRUE;
                    currentPtr->leftPtr = splayNodePtr;
                    if (currentPtr == splayPtr->leastPtr) {
                        splayPtr->leastPtr = splayNodePtr;
                    }
                }
                else {
                    currentPtr = currentPtr->leftPtr;
                }
            }
            else {
                if (currentPtr->rightPtr == NULL) {
                    itemInserted = TRUE;
                    currentPtr->rightPtr = splayNodePtr;
                }
                else {
                    currentPtr = currentPtr->rightPtr;
                }
            }
        }

        splayNodePtr->parentPtr = currentPtr;
        SPLAY_SplayTreeAtNode(splayPtr, splayNodePtr);
        tree->timeValue = tree->leastPtr->timeValue;
    }
}
// /**
// FUNCTION   :: MacCellularProcessTchMessages
// LAYER      :: MAC
// PURPOSE    :: Scheudle TCH transmission
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : interface to handle the message
// RETURN     :: void : NULL
// **/
void MacCellularProcessTchMessages(
    Node* node,
    int interfaceIndex)
{
    SimpleSplayTree* tchSplayTree =
        &(((MacCellularData*)
            node->macData[interfaceIndex]->macVar)
         ->macCellularAbstractData->tchSplayTree);

    // handle items in heap up to current time
    while (tchSplayTree->timeValue == node->getNodeTime())
    {
        SimpleSplayNode* splayNode =
            SPLAY_ExtractMin(tchSplayTree);

        if (DEBUG_TIMER||DEBUG_PROGRESS)
        {
            printf("at %015" TYPES_64BITFMT "d node %d  Process TCH packet\n",
                   node->getNodeTime(),node->nodeId);
            fflush(stdout);
        }

        MacCellularAbstractReceivePacketFromPhy(
            node,
            interfaceIndex,
            (Message*) splayNode->element);

        SPLAY_FreeNode(splayNode);
    }
}
