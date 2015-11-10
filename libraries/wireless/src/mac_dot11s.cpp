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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "mac_dot11.h"
#include "mac_dot11-sta.h"
#include "mac_dot11-ap.h"
#include "mac_dot11-mgmt.h"
#include "mac_dot11-mib.cpp"
#include "mac_dot11s-frames.h"
#include "mac_dot11s-hwmp.h"
#include "mac_dot11s.h"


// Enable for debug trace.
#define DOT11s_TraceComments 0

static
void Dot11sAssoc_SetState(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_NeighborItem* neighborItem,
    DOT11s_AssocState state);


// ------------------------------------------------------------------
// General purpose utility functions


/**
FUNCTION   :: Dot11sIO_ReadTime
LAYER      :: MAC
PURPOSE    :: Read user configured time value.
              Reports an error if value is negative.
              Reports an error for unacceptable zero value.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ nodeId    : NodeAddress   : pointer to node
+ address   : Address*      : Pointer to link layer address structure
+ nodeInput : NodeInput*    : pointer to user configuration structure
+ parameter : char*         : parameter to be read
+ wasFound  : BOOL*         : TRUE if parameter was found and read
+ value     : clocktype*    : value if parameter was read
+ isZeroValid : BOOL        : permit a zero value of parameter
RETURN     :: void
**/
void Dot11sIO_ReadTime(
    Node* node,
    NodeAddress nodeId,
    Address* address,
    const NodeInput* nodeInput,
    const char* parameter,
    BOOL* wasFound,
    clocktype* value,
    BOOL isZeroValid)
{
    IO_ReadTime(
        node,
        nodeId,
        MAPPING_GetInterfaceIndexFromInterfaceAddress(
            node,
            *address),
        nodeInput,
        parameter,
        wasFound,
        value);

    if (*wasFound)
    {
        if (*value < 0)
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                "Invalid input value for %s.\n"
                "Negative value not permitted.\n",
                parameter);
            ERROR_ReportError(errStr);
        }
        if (isZeroValid == FALSE && *value == 0)
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                "Invalid input value for %s.\n"
                "Zero value not permitted.\n",
                parameter);
            ERROR_ReportError(errStr);
        }
    }
}


/**
FUNCTION   :: Dot11sIO_ReadInt
LAYER      :: MAC
PURPOSE    :: Read user configured integer value.
              Reports an error for unacceptable negative value.
              Reports an error for unacceptable zero value.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ nodeId    : NodeAddress   : pointer to node
+ address   : Address*      : Pointer to link layer address structure
+ nodeInput : NodeInput*    : pointer to user configuration structure
+ parameter : char*         : parameter to be read
+ wasFound  : BOOL*         : TRUE if parameter was found and read
+ value     : int*          : value if parameter was read
+ isZeroValid : BOOL        : permit a zero value of parameter
+ isNegativeValid : BOOL    : permit a negative value of parameter
RETURN     :: void
**/
void Dot11sIO_ReadInt(
    Node* node,
    NodeAddress nodeId,
    Address* address,
    const NodeInput* nodeInput,
    const char* parameter,
    BOOL* wasFound,
    int* value,
    BOOL isZeroValid,
    BOOL isNegativeValid)
{
    IO_ReadInt(
        node,
        nodeId,
        MAPPING_GetInterfaceIndexFromInterfaceAddress(
            node,
            *address),
        nodeInput,
        parameter,
        wasFound,
        value);

    if (*wasFound)
    {
        if (isNegativeValid == FALSE && *value < 0)
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                "Invalid input value for %s.\n"
                "Negative value not permitted.\n",
                parameter);
            ERROR_ReportError(errStr);
        }
        if (isZeroValid == FALSE && *value == 0)
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                "Invalid input value for %s.\n"
                "Zero value not permitted.\n",
                parameter);
            ERROR_ReportError(errStr);
        }
    }
}



/**
FUNCTION   :: ListPrepend
LAYER      :: MAC
PURPOSE    :: Insert data at the beginning of list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ list      : LinkedList*   : Pointer to the LinkedList structure
+ timeStamp : clocktype     : Time stamp of insertion
+ data      : void*         : Data to be iserted
RETURN     :: void
NOTES      :: This function augments the LinkedList data structure
                that provides a ListInsert that actually appends
                to the list.
              Useful for Data Seen list, etc., where the items
                near the beginning of the list are most likely to
                be seen next.
**/
void ListPrepend(
    Node *node,
    LinkedList* list,
    clocktype timeStamp,
    void *data)
{
    ListItem *listItem = (ListItem *) MEM_malloc(sizeof(ListItem));

    listItem->data = data;
    listItem->timeStamp = timeStamp;
    listItem->prev = NULL;

    if (list->size == 0)
    {
        /* Only item in the list. */
        listItem->next = NULL;
        list->first = listItem;
        list->last = list->first;
    }
    else
    {
        /* Insert at beginning of list. */
        listItem->next = list->first;
        list->first->prev = listItem;
        list->first = listItem;
    }

    list->size++;
}


/**
FUNCTION   :: ListAppend
LAYER      :: MAC
PURPOSE    :: Wrapper for the ListInsert function.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ list      : LinkedList*   : Pointer to the LinkedList structure
+ timeStamp : clocktype     : Time stamp of insertion
+ data      : void*         : Data to be iserted
RETURN     :: void
NOTES      :: Added for symmetry with ListPrepend
**/

void ListAppend(
    Node *node,
    LinkedList* list,
    clocktype timeStamp,
    void *data)
{
    ListInsert(node, list, timeStamp, data);
}


/**
FUNCTION   :: ListDelete
LAYER      :: MAC
PURPOSE    :: Wrapper for ListGet function; removes listItem from list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ list      : LinkedList*   : Pointer to the LinkedList structure
+ listItem  : ListItem*     : Item to be deleted
+ isMsg     : BOOL          : TRUE is listItem contains a Message
RETURN     :: void
**/

void ListDelete(
    Node *node,
    LinkedList* list,
    ListItem* listItem,
    BOOL isMsg)
{
    ListGet(node, list, listItem, TRUE, isMsg);
}


/**
FUNCTION   :: Dot11s_GetDataRateInMbps
LAYER      :: MAC
PURPOSE    :: Get data rate in Mbps given dataRateType and phy model.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ phyModel  : PhyModel      : Physical model type
+ phyIndex  : int           : PhyIndex
+ dataRateType : int        : value of data rate enum
RETURN     :: int           : Data rate in Mbps
NOTES      :: For 802.11b, data rate type 2, returns integer 5
                for 5.5 Mbps
**/

static
int Dot11s_GetDataRateInMbps(
    Node* node,
    PhyModel phyModel,
    int phyIndex,
    int dataRateType)
{
    switch (phyModel)
    {
        case PHY802_11a:
        {
            switch (dataRateType)
            {
                case 0: return 6;
                case 1: return 9;
                case 2: return 12;
                case 3: return 18;
                case 4: return 24;
                case 5: return 36;
                case 6: return 48;
                case 7: return 54;
                default:
                    ERROR_ReportError("Dot11s_GetDataRateInMbps : "
                        "Unknown data rate type for 802.11a.\n");
                    break;
            }
            break;
        }
        case PHY802_11b:
        {
            switch (dataRateType)
            {
                case 0: return 1;
                case 1: return 2;
                case 2: return 5;   // truncated; helps distinguish
                                    // from 802.11a 6Mbps
                case 3: return 11;
                default:
                    ERROR_ReportError("Dot11s_GetDataRateInMbps: "
                        "Unknown data rate type for 802.11b.\n");
                    break;
            }
            break;
        }
        case PHY_ABSTRACT:
            {
                return (PHY_GetTxDataRate(node,phyIndex)/1000000);
            break;
        }
        default:
        {
            ERROR_ReportError("Dot11s_GetDataRateInMbps: "
                "Unknown physical model.\n");
            break;
        }
    }

    return 0; // keep compiler happy
}


/**
FUNCTION   :: Dot11s_IsSelfOrBssStationAddr
LAYER      :: MAC
PURPOSE    :: Determine if a given address is own address
                or of one of the associated stations.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ addr      : Mac802Address : Address being queried
RETURN     :: BOOL          : TRUE if addr is in BSS
**/

BOOL Dot11s_IsSelfOrBssStationAddr(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address addr)
{
    ERROR_Assert(dot11->isMP,
        "Dot11s_IsSelfOrBssStationAddr: "
        "Node is not a Mesh Point.\n");

    // Fine grain if self can have multiple addresses
    // or if the station should currently be in an associated state.
    if (addr == dot11->selfAddr)
    {
        return TRUE;
    }
    else if (dot11->isMAP)
    {
        //DOT11_ApStationListItem* stationListItem =
        //    MacDot11ApStationListGetItemWithGivenAddress(
        //    node, dot11, addr);
        DOT11s_StationItem* stationItem =
            Dot11sStationList_Lookup(node, dot11, addr);
        if (stationItem != NULL)
        {
            return TRUE;
        }
    }

    return FALSE;
}


/**
FUNCTION   :: Dot11s_AddrAsDotIP
LAYER      :: MAC
PURPOSE    :: Convert addr to hex dot IP format e.g. [aa.bb.cc.dd]
              With change to 6 byte MAC address, the format is
              [aa-bb-cc-dd-ee-ff]
PARAMETERS ::
+ addrStr   : char*         : Converted address string
+ addr      : Mac802Address : Address of node
RETURN     :: void
NOTES      :: This is useful in debug trace for consistent
                columnar formatting.
**/

void Dot11s_AddrAsDotIP(
    char* addrStr,
    const Mac802Address* macAddr)
{
    /*sprintf(addrStr, "[%02X.%02X.%02X.%02X]",
        (addr & 0xff000000) >> 24,
        (addr & 0xff0000) >> 16,
        (addr & 0xff00) >> 8,
        (addr & 0xff));*/
    sprintf(addrStr, "[%02X-%02X-%02X-%02X-%02X-%02X]",
        macAddr->byte[0],
        macAddr->byte[1],
        macAddr->byte[2],
        macAddr->byte[3],
        macAddr->byte[4],
        macAddr->byte[5]);
}


/**
FUNCTION   :: Dot11s_TraceFrameInfo
LAYER      :: MAC
PURPOSE    :: Utility fn to trace values in a FrameInfo structure.
                Useful for queued items in data & management queues.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo* : FrameInfo structure
+ prependStr: char*         : String to prepend to line of trace
RETURN     :: void
**/

void Dot11s_TraceFrameInfo(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameInfo,
    const char* prependStr)
{
    if (!DOT11s_TraceComments)
    {
        return;
    }

    char addrRA[MAX_STRING_LENGTH];
    char addrTA[MAX_STRING_LENGTH];
    char addrDA[MAX_STRING_LENGTH];
    char addrSA[MAX_STRING_LENGTH];
    Dot11s_AddrAsDotIP(addrRA, &frameInfo->RA);
    Dot11s_AddrAsDotIP(addrTA, &frameInfo->TA);
    Dot11s_AddrAsDotIP(addrDA, &frameInfo->DA);
    Dot11s_AddrAsDotIP(addrSA, &frameInfo->SA);

    char clockStr[MAX_STRING_LENGTH];
    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

    //char insertClockStr[MAX_STRING_LENGTH];
    //TIME_PrintClockInSecond(frameInfo->insertTime, insertClockStr);

    char traceStr[MAX_STRING_LENGTH];

    if (frameInfo->frameType == DOT11_MESH_DATA)
    {
        sprintf(traceStr, "%s : "
            "Frame= %d, "
            "RA=%s, TA=%s, DA=%s, SA=%s, "
            "E2E=%d TTL=%d, "
            //"at %s (inserted=%s)",
            "at %s",
            prependStr,
            frameInfo->frameType,
            addrRA, addrTA, addrDA, addrSA,
            frameInfo->fwdControl.GetE2eSeqNo(),
            frameInfo->fwdControl.GetTTL(),
            //clockStr, insertClockStr);
            clockStr);
    }
    else
    {
        sprintf(traceStr, "%s : "
            "Frame= %d, "
            "RA=%s, TA=%s, DA=%s, SA=%s, "
            //"at %s (inserted=%s)",
            "at %s",
            prependStr,
            frameInfo->frameType,
            addrRA, addrTA, addrDA, addrSA,
            //clockStr, insertClockStr);
            clockStr);
    }

    MacDot11Trace(node, dot11, NULL, traceStr);
}


/**
FUNCTION   :: Dot11s_MemFreeFrameInfo
LAYER      :: MAC
PURPOSE    :: Utility fn to free memory used by DOT11s_FrameInfo.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ frameInfo : DOT11s_FrameInfo* : FrameInfo structure
+ deleteMsg : BOOL              : TRUE to delete message, default FALSE
RETURN     :: void
**/

void Dot11s_MemFreeFrameInfo(
    Node* node,
    DOT11s_FrameInfo** frameInfo,
    BOOL deleteMsg)
{
    ERROR_Assert(*frameInfo != NULL,
        "Dot11s_MemFreeFrameInfo: parameter frameInfo is NULL.\n");

    if (deleteMsg)
    {
        MESSAGE_Free(node, (*frameInfo)->msg);
    }

    MEM_free(*frameInfo);
    *frameInfo = NULL;
}


// ------------------------------------------------------------------

/**
FUNCTION   :: Dot11sMgmtQueue_DequeuePacket
LAYER      :: MAC
PURPOSE    :: Dequeue a frame from the management queue,
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo** : Parameter for dequeued frame info
RETURN     :: BOOL          : TRUE if dequeue is successful
**/

BOOL Dot11sMgmtQueue_DequeuePacket(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo** frameInfo)
{
    if (ListGetSize(node, dot11->mgmtQueue) == 0)
    {
        return FALSE;
    }

    ListItem *listItem = dot11->mgmtQueue->first;
    ListGet(node, dot11->mgmtQueue, listItem, FALSE, FALSE);
    *frameInfo = (DOT11s_FrameInfo*) (listItem->data);
    MEM_free(listItem);

    //dot11->pktsDequeuedFromMgmtQueue++;

    if (DOT11s_TraceComments)
    {
        Dot11s_TraceFrameInfo(node, dot11, *frameInfo,
            "Dot11sMgmtQueue_DequeuePacket: ");
    }

    return TRUE;
}


/**
FUNCTION   :: Dot11sMgmtQueue_EnqueuePacket
LAYER      :: MAC
PURPOSE    :: Enqueue a frame in the management queue.
                Notifies if it is the only item in the queue.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo* : Enqueued frame info
RETURN     :: BOOL          : TRUE if enqueue is successful.
**/

BOOL Dot11sMgmtQueue_EnqueuePacket(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameInfo)
{
    frameInfo->insertTime = node->getNodeTime();
    ListAppend(node, dot11->mgmtQueue, 0, frameInfo);

    //dot11->pktsEnqueuedInMgmtQueue++;

    if (DOT11s_TraceComments)
    {
        Dot11s_TraceFrameInfo(node, dot11, frameInfo,
            "Dot11sMgmtQueue_EnqueuePacket: ");
    }

    if (ListGetSize(node, dot11->mgmtQueue) == 1)
    {
        MacDot11MgmtQueueHasPacketToSend(node, dot11);
    }

    // No current limit to queue, etc., insertion always successful.
    return TRUE;
}


/**
FUNCTION   :: Dot11sMgmtQueue_DeletePacketsToNode
LAYER      :: MAC
PURPOSE    :: Delete packets from queue going to a particular next hop
                and/or destination.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ nextHopAddr : Mac802Address : next hop address, ANY_MAC802 to ignore
+ destAddr  : Mac802Address : destination address, ANY_MAC802 to ignore
RETURN     :: void
**/
static
void Dot11sMgmtQueue_DeletePacketsToNode(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address nextHopAddr,
    Mac802Address destAddr)
{
    LinkedList* list = dot11->mgmtQueue;

    if (ListGetSize(node, list) == 0)
    {
        return;
    }

    int matchType = 0;
    if (nextHopAddr != ANY_MAC802)
    {
        matchType += 1;
    }
    if (destAddr != ANY_MAC802)
    {
        matchType += 2;
    }
    if (matchType == 0)
    {
        return;
    }

    DOT11s_FrameInfo* frameInfo;
    BOOL toDelete;
    ListItem* listItem = list->first;
    ListItem* tempItem;
    int count = 0;

    while (listItem != NULL)
    {
        frameInfo = (DOT11s_FrameInfo*) (listItem->data);
        toDelete = FALSE;

        switch (matchType)
        {
            case 1: // only check next hop
                if (frameInfo->RA == nextHopAddr)
                {
                    toDelete = TRUE;
                }
                break;
            case 2: // only check dest address
                if (frameInfo->DA == destAddr)
                {
                    toDelete = TRUE;
                }
                break;
            case 3: // check both next hop and dest address
                if (frameInfo->RA == nextHopAddr
                    && frameInfo->DA == destAddr)
                {
                    toDelete = TRUE;
                }
                break;
            default:
                ERROR_ReportError("Dot11sMgmtQueue_DeletePacketsToNode: "
                    "Unexpected parameters for deletion.\n");
        }

        tempItem = listItem;
        listItem = listItem->next;
        if (toDelete)
        {
            count++;
            frameInfo = (DOT11s_FrameInfo*) tempItem->data;
            if (frameInfo->RA == ANY_MAC802)
            {
                dot11->mp->stats.mgmtQueueBcDropped++;
            }
            else
            {
                dot11->mp->stats.mgmtQueueUcDropped++;
            }
#ifdef ADDON_DB
            HandleMacDBEvents(        
                node, frameInfo->msg, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

            MESSAGE_Free(node, frameInfo->msg);
            ListDelete(node, list, tempItem, FALSE);
        }
    }

    if (DOT11s_TraceComments && count > 0)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr,
            "Dot11sMgmtQueue_DeletePacketsToNode: Deleted %d items", count);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }
}


/**
FUNCTION   :: Dot11sMgmtQueue_Finalize
LAYER      :: MAC
PURPOSE    :: Deletes items in management queue & queue itself.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/

void Dot11sMgmtQueue_Finalize(
    Node* node,
    MacDataDot11* dot11)
{
    LinkedList* list = dot11->mgmtQueue;
    ListItem *item;
    DOT11s_FrameInfo* frameInfo;

    if (list == NULL)
    {
        return;
    }

    if (DOT11s_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sMgmtQueue_Finalize: "
            "Freed %d items",
            ListGetSize(node, list));
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    item = list->first;

    while (item)
    {
        frameInfo = (DOT11s_FrameInfo*) item->data;
        MESSAGE_Free(node, frameInfo->msg);
        item = item->next;
    }

    ListFree(node, list, FALSE);
    list = NULL;
}


/**
FUNCTION   :: Dot11sMgmtQueue_AgeItems
LAYER      :: MAC
PURPOSE    :: Remove items from mesh management queue older than
                aging time.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
NOTE       :: The granularity depends on the maintenance timer
                and is not exactly aging time.
**/

static
void Dot11sMgmtQueue_AgeItems(
    Node* node,
    MacDataDot11* dot11)
{
    LinkedList* list = dot11->mgmtQueue;
    if (ListGetSize(node, list) == 0)
    {
        return;
    }

    clocktype agingTime = node->getNodeTime() - DOT11s_QUEUE_AGING_TIME;
    if (agingTime < 0)
    {
        return;
    }

    DOT11s_FrameInfo* frameInfo;
    ListItem* listItem = list->first;
    int count = 0;

    while (listItem != NULL)
    {
        frameInfo = (DOT11s_FrameInfo*) (listItem->data);
        if (frameInfo->insertTime < agingTime)
        {
            if (frameInfo->RA == ANY_MAC802)
            {
                dot11->mp->stats.mgmtQueueBcDropped++;
            }
            else
            {
                dot11->mp->stats.mgmtQueueUcDropped++;
            }

            count++;
#ifdef ADDON_DB
            HandleMacDBEvents(        
                node, frameInfo->msg, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif
            MESSAGE_Free(node, frameInfo->msg);
            ListDelete(node, list, listItem, FALSE);
            listItem = list->first;
        }
        else
        {
            break;
        }
    }

    if (count > 0)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr,
            "Dot11sMgmtQueue_AgeItems: Deleted %d items", count);
        MacDot11Trace(node, dot11, NULL, traceStr);

        if (ListGetSize(node, list) > 0)
        {
            MacDot11MgmtQueueHasPacketToSend(node, dot11);
        }
    }
}


/**
FUNCTION   :: Dot11sDataQueue_DequeuePacket
LAYER      :: MAC
PURPOSE    :: Dequeue a frame from the data queue.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo** : Parameter for dequeued frame info
RETURN     :: BOOL          : TRUE if dequeue is successful
**/

BOOL Dot11sDataQueue_DequeuePacket(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo** frameInfo)
{
    if (ListGetSize(node, dot11->dataQueue) == 0)
    {
        return FALSE;
    }

    ListItem *listItem = dot11->dataQueue->first;
    *frameInfo = (DOT11s_FrameInfo*) (listItem->data);
    ListGet(node, dot11->dataQueue, listItem, FALSE, FALSE);
    MEM_free(listItem);

    //dot11->pktsDequeuedFromDataQueue++;

    if (DOT11s_TraceComments)
    {
        Dot11s_TraceFrameInfo(node, dot11, *frameInfo,
            "Dot11sDataQueue_DequeuePacket: ");
    }

    return TRUE;
}


/**
FUNCTION   :: Dot11sDataQueue_EnqueuePacket
LAYER      :: MAC
PURPOSE    :: Enqueue a frame in the single data queue.
                Notifies if it is the only item in the queue.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo* : Enqueued frame info
RETURN     :: BOOL          : TRUE if enqueue is successful.
**/

BOOL Dot11sDataQueue_EnqueuePacket(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameInfo)
{
    frameInfo->insertTime = node->getNodeTime();;
    ListAppend(node, dot11->dataQueue, node->getNodeTime(), frameInfo);

    //dot11->pktsEnqueuedInDataQueue++;

    if (DOT11s_TraceComments)
    {
        Dot11s_TraceFrameInfo(node, dot11, frameInfo,
            "Dot11sDataQueue_EnqueuePacket: ");
    }

    // Notify if single item in the queue.
    if (ListGetSize(node, dot11->dataQueue) == 1)
    {
        MacDot11DataQueueHasPacketToSend(node, dot11);
    }

    // No current queue size/packet limit, etc.,
    // insertion is always successful.
    return TRUE;
}


/**
FUNCTION   :: Dot11sDataQueue_DeletePacketsToNode
LAYER      :: MAC
PURPOSE    :: Delete packets from queue going to a particular next hop
                and/or destination.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ nextHopAddr : Mac802Address : next hop address, ANY_MAC802 to ignore
+ destAddr  : Mac802Address : destination address, ANY_MAC802 to ignore
RETURN     :: void
**/
static
void Dot11sDataQueue_DeletePacketsToNode(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address nextHopAddr,
    Mac802Address destAddr)
{
    LinkedList* list = dot11->dataQueue;

    if (ListGetSize(node, list) == 0)
    {
        return;
    }

    int matchType = 0;
    if (nextHopAddr != ANY_MAC802)
    {
        matchType += 1;
    }
    if (destAddr != ANY_MAC802)
    {
        matchType += 2;
    }
    if (matchType == 0)
    {
        return;
    }

    DOT11s_FrameInfo* frameInfo;
    BOOL toDelete;
    ListItem *listItem = list->first;
    ListItem *tempItem;
    int count = 0;

    while (listItem != NULL)
    {
        frameInfo = (DOT11s_FrameInfo*) (listItem->data);
        toDelete = FALSE;

        switch (matchType)
        {
            case 1: // only check next hop
                if (frameInfo->RA == nextHopAddr)
                {
                    toDelete = TRUE;
                }
                break;
            case 2: // only check dest address
                if (frameInfo->DA == destAddr)
                {
                    toDelete = TRUE;
                }
                break;
            case 3: // check both next hop and dest address
                if (frameInfo->RA == nextHopAddr
                    && frameInfo->DA == destAddr)
                {
                    toDelete = TRUE;
                }
                break;
            default:
                ERROR_ReportError("Dot11sDataQueue_DeletePacketsToNode: "
                    "Unexpected parameters for deletion.\n");
        }

        tempItem = listItem;
        listItem = listItem->next;
        if (toDelete)
        {
            count++;
            frameInfo = (DOT11s_FrameInfo*) tempItem->data;
            if (frameInfo->RA == ANY_MAC802)
            {
                dot11->mp->stats.dataQueueBcDropped++;
            }
            else
            {
                dot11->mp->stats.dataQueueUcDropped++;
            }
#ifdef ADDON_DB
            HandleMacDBEvents(        
                node, frameInfo->msg, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

            MESSAGE_Free(node, frameInfo->msg);
            ListDelete(node, list, tempItem, FALSE);
        }
    }

    if (DOT11s_TraceComments && count > 0)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr,
            "Dot11sDataQueue_DeletePacketsToNode: Deleted %d items", count);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

}


/**
FUNCTION   :: Dot11sDataQueue_Finalize
LAYER      :: MAC
PURPOSE    :: Free the local data queue, including items in it.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/

void Dot11sDataQueue_Finalize(
    Node* node,
    MacDataDot11* dot11)
{
    LinkedList* list = dot11->dataQueue;
    ListItem *item;
    DOT11s_FrameInfo* frameInfo;

    if (list == NULL)
    {
        return;
    }

    if (DOT11s_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sDataQueue_Finalize: "
            "Freed %d items",
            ListGetSize(node, list));
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    item = list->first;

    while (item)
    {
        frameInfo = (DOT11s_FrameInfo*) item->data;
        MESSAGE_Free(node, frameInfo->msg);
        item = item->next;
    }

    ListFree(node, list, FALSE);
    list = NULL;
}


/**
FUNCTION   :: Dot11sDataQueue_AgeItems
LAYER      :: MAC
PURPOSE    :: Remove items from mesh data queue older than
                aging time.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
NOTE       :: The granularity depends on the aging timer
                and could be more than aging time.
**/

static
void Dot11sDataQueue_AgeItems(
    Node* node,
    MacDataDot11* dot11)
{
    LinkedList* list = dot11->dataQueue;
    if (ListGetSize(node, list) == 0)
    {
        return;
    }

    clocktype agingTime = node->getNodeTime() - DOT11s_QUEUE_AGING_TIME;
    if (agingTime < 0)
    {
        return;
    }

    DOT11s_FrameInfo* frameInfo;
    ListItem* listItem = list->first;
    int count = 0;

    while (listItem != NULL)
    {
        frameInfo = (DOT11s_FrameInfo*) (listItem->data);
        if (frameInfo->insertTime < agingTime)
        {
            if (frameInfo->RA == ANY_MAC802)
            {
                dot11->mp->stats.dataQueueBcDropped++;
            }
            else
            {
                dot11->mp->stats.dataQueueUcDropped++;
            }

            count++;
            MESSAGE_Free(node, frameInfo->msg);
            ListDelete(node, list, listItem, FALSE);
            listItem = list->first;
        }
        else
        {
            break;
        }
    }

    if (count > 0)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr,
            "Dot11sDataQueue_AgeItems: Deleted %d items", count);
        MacDot11Trace(node, dot11, NULL, traceStr);

        if (ListGetSize(node, list) > 0)
        {
            MacDot11DataQueueHasPacketToSend(node, dot11);
        }
    }
}


/**
FUNCTION   :: Dot11s_DeletePacketsToNode
LAYER      :: MAC
PURPOSE    :: Delete packets from queues going to a particular next hop
                and/or destination.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ nextHopAddr : Mac802Address : next hop address, ANY_MAC802 to ignore
+ destAddr  : Mac802Address : destination address, ANY_MAC802 to ignore
RETURN     :: void
**/

void Dot11s_DeletePacketsToNode(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address nextHopAddr,
    Mac802Address destAddr)
{
    Dot11sMgmtQueue_DeletePacketsToNode(node, dot11,
        nextHopAddr, destAddr);
    Dot11sDataQueue_DeletePacketsToNode(node, dot11,
        nextHopAddr, destAddr);
}



/**
FUNCTION   :: Dot11s_SendDataFrameToMesh
LAYER      :: MAC
PURPOSE    :: Construct data frame for mesh delivery and enqueue.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo : frame values
RETURN     :: void
**/

static
void Dot11s_SendDataFrameToMesh(
    Node* node,
    MacDataDot11* dot11,
    const DOT11s_FrameInfo* const frameInfo,
    Message* msg)
{
    DOT11s_FrameInfo* newFrameInfo;
    Dot11s_MallocMemset0(DOT11s_FrameInfo, newFrameInfo);
    *newFrameInfo = *frameInfo;

    newFrameInfo->frameType = (unsigned char) DOT11_MESH_DATA;
    newFrameInfo->msg = msg;

    MESSAGE_AddHeader(node,
        newFrameInfo->msg,
        DOT11s_DATA_FRAME_HDR_SIZE,
        TRACE_DOT11);

    Dot11s_SetFieldsInDataFrameHdr(node, dot11,
        MESSAGE_ReturnPacket(newFrameInfo->msg), newFrameInfo);

    // Since initiating, insert to seen list
    Dot11sDataSeenList_Insert(node, dot11, newFrameInfo->msg);

    // Enqueue the mesh frame for transmission
    Dot11sDataQueue_EnqueuePacket(node, dot11, newFrameInfo);
}


/**
FUNCTION   :: Dot11s_SendFrameToBss
LAYER      :: MAC
PURPOSE    :: Construct data frame for BSS delivery and enqueue.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo : frame values
RETURN     :: void
**/

static
void Dot11s_SendFrameToBss(
    Node* node,
    MacDataDot11* dot11,
    const DOT11s_FrameInfo* const frameInfo,
    Message* msg)
{
    DOT11s_FrameInfo* newFrameInfo;
    Dot11s_MallocMemset0(DOT11s_FrameInfo, newFrameInfo);
    *newFrameInfo = *frameInfo;


    newFrameInfo->frameType = DOT11_DATA;
    newFrameInfo->fwdControl = 0;

    // Enqueue the frame for transmission

    //dot11->currentNextHopAddress = newFrameInfo->RA;
    //dot11->ipNextHopAddr = newFrameInfo->RA;

    dot11->ipDestAddr = newFrameInfo->DA;
    dot11->ipSourceAddr = newFrameInfo->SA;

    MESSAGE_AddHeader(node,
                    msg,
                    sizeof(DOT11_FrameHdr),
                    TRACE_DOT11);


     MacDot11StationSetFieldsInDataFrameHdr(dot11,
                                        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg),
                                        newFrameInfo->RA,
                                        DOT11_DATA);
     newFrameInfo->msg = msg;

    Dot11sDataQueue_EnqueuePacket(node, dot11, newFrameInfo);
}


/**
FUNCTION   :: Dot11s_SendFrameToRoutingFunction
LAYER      :: MAC
PURPOSE    :: Send a mesh frame to routing function.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mp        : DOT11s_Data   : pointer to MP structure
+ frameInfo : DOT11s_FrameInfo : frame values
RETURN     :: BOOL             : TRUE if frame was routed.
**/

BOOL Dot11s_SendFrameToRoutingFunction(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp,
    const DOT11s_FrameInfo* const frameInfo,
    Message* msg)
{
    DOT11s_FrameInfo* newFrameInfo;
    Dot11s_MallocMemset0(DOT11s_FrameInfo, newFrameInfo);
    *newFrameInfo = *frameInfo;
    newFrameInfo->msg = msg;

    newFrameInfo->frameType = DOT11_MESH_DATA;

    BOOL packetWasRouted;
    mp->activeProtocol.routerFunction(node, dot11,
        newFrameInfo, &packetWasRouted);

    if (packetWasRouted == FALSE)
    {
        // Message is not freed as it would typically be
        // sent to the upper layer / BSS.
        Dot11s_MemFreeFrameInfo(node, &newFrameInfo, FALSE);
    }

    return packetWasRouted;
}


/**
FUNCTION   :: Dot11s_SendFrameToUpperLayer
LAYER      :: MAC
PURPOSE    :: Send a mesh frame to upper layer.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo : frame values
RETURN     :: void
**/

static
void Dot11s_SendFrameToUpperLayer(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* const frameInfo,
    Message* msg)
{

    MAC_HandOffSuccessfullyReceivedPacket(node,
        dot11->myMacData->interfaceIndex, msg, &(frameInfo->SA));
}


/**
FUNCTION   :: Dot11s_SetNeighborState
LAYER      :: MAC
PURPOSE    :: Sets new neighbor state. Also tracks previous state.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ neighborItem : DOT11s_NeighborItem* : Pointer to neighbor item
+ state     : DOT11s_NeighborState : new state to be set
RETURN     :: void
**/

static
void Dot11s_SetNeighborState(
    Node* node,
    DOT11s_NeighborItem* neighborItem,
    DOT11s_NeighborState state)
{
    neighborItem->prevState = neighborItem->state;
    neighborItem->state = state;
}


/**
FUNCTION   :: Dot11sNeighborList_Lookup
LAYER      :: MAC
PURPOSE    :: Search Neighbor list for an address.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ neighborAddr : Mac802Address : address to be looked up
RETURN     :: DOT11s_NeighborItem* : neighbor item or NULL
**/

static
DOT11s_NeighborItem* Dot11sNeighborList_Lookup(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address neighborAddr)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* neighborList = mp->neighborList;

    ListItem* listItem = neighborList->first;
    DOT11s_NeighborItem* neighborItem = NULL;
    BOOL isNeighborFound = FALSE;

    while (listItem != NULL)
    {
        neighborItem = (DOT11s_NeighborItem*) listItem->data;
        if (neighborItem->neighborAddr == neighborAddr)
        {
            isNeighborFound = TRUE;
            break;
        }
        listItem = listItem->next;
    }

    return (isNeighborFound ? neighborItem : NULL);
}


/**
FUNCTION   :: Dot11sNeighborList_Finalize
LAYER      :: MAC
PURPOSE    :: Free neighbor list and contained items.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/

static
void Dot11sNeighborList_Finalize(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* neighborList = mp->neighborList;

    if (neighborList == NULL)
    {
        return;
    }

    if (DOT11s_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sNeighborList_Finalize: "
            "Freed %d items",
            ListGetSize(node, neighborList));
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    ListFree(node, neighborList, FALSE);
}


/**
FUNCTION   :: Dot11s_IsAssociatedNeighbor
LAYER      :: MAC
PURPOSE    :: Check if neighbor is associated.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ neighborAddr : Mac802Address : neighbor address to check for association.
RETURN     :: BOOL          : TRUE if neighbor is associated.
NOTES      :: Assumes neighbor is associated as well as in UP state.
**/

static
BOOL Dot11s_IsAssociatedNeighbor(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address neighborAddr)
{
    DOT11s_NeighborItem* neighborItem =
        Dot11sNeighborList_Lookup(node, dot11, neighborAddr);

    if (neighborItem == NULL)
    {
        return FALSE;
    }
    if (neighborItem->state == DOT11s_NEIGHBOR_SUBORDINATE_LINK_UP
        || neighborItem->state == DOT11s_NEIGHBOR_SUPERORDINATE_LINK_UP)
    {
        return TRUE;
    }
    return FALSE;
}


/**
FUNCTION   :: Dot11sPortalList_AgeItems
LAYER      :: MAC
PURPOSE    :: Mark portals as inactive if portal active time has
                elapsed since the last PANN was received.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/

static
void Dot11sNeighborList_AgeItems(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* neighborList = mp->neighborList;

    clocktype agingTime = node->getNodeTime() - DOT11s_ASSOC_ACTIVE_TIMEOUT;
    if (agingTime < 0)
    {
        return;
    }

    ListItem* listItem = neighborList->first;
    DOT11s_NeighborItem* neighborItem = NULL;
    BOOL ageItem;

    while (listItem != NULL)
    {
        ageItem = FALSE;

        neighborItem = (DOT11s_NeighborItem*) listItem->data;
        if ((neighborItem->state == DOT11s_NEIGHBOR_SUBORDINATE_LINK_UP
                || neighborItem->state == DOT11s_NEIGHBOR_SUPERORDINATE_LINK_UP)
            && neighborItem->lastLinkStateTime < agingTime)
        {
            ageItem = TRUE;
        }

        if (DOT11s_TraceComments)
        {
            //char traceStr[MAX_STRING_LENGTH];
            //char neighborStr[MAX_STRING_LENGTH];
            //Dot11s_AddrAsDotIP(neighborStr, neighborItem->neighborAddr);
            //sprintf(traceStr, "Dot11sNeighborList_AgeItems: "
            //    "Neighbor %s, state %d", neighborStr, neighborItem->state);
            //MacDot11Trace(node, dot11, NULL, traceStr);
        }

        if (ageItem)
        {
            if (DOT11s_TraceComments)
            {
                char traceStr[MAX_STRING_LENGTH];
                char neighborStr[MAX_STRING_LENGTH];
                Dot11s_AddrAsDotIP(neighborStr, &neighborItem->neighborAddr);
                sprintf(traceStr, "Dot11sNeighborList_AgeItems: "
                    "Closing link with %s ", neighborStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            mp->assocStateData.event = DOT11s_ASSOC_S_EVENT_CANCEL_LINK;
            mp->assocStateData.neighborItem = neighborItem;
            neighborItem->assocStateFn(node, dot11, mp);
        }
        listItem = listItem->next;
    }
}


/**
FUNCTION   :: Dot11s_GetAssociatedNeighborCount
LAYER      :: MAC
PURPOSE    :: Get count of associated neighbors.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ neighborAddr : Mac802Address : neighbor address to check for association.
RETURN     :: int           : count of associated neighbors.
NOTES      :: Neighbor is both associated as well as in UP state.
**/

int Dot11s_GetAssociatedNeighborCount(
    Node* node,
    MacDataDot11* dot11)
{
    LinkedList* list = dot11->mp->neighborList;
    ListItem* listItem = list->first;
    DOT11s_NeighborItem* item = NULL;
    int count = 0;

    while (listItem != NULL)
    {
        item = (DOT11s_NeighborItem*) listItem->data;
        if (item->state == DOT11s_NEIGHBOR_SUBORDINATE_LINK_UP
            || item->state == DOT11s_NEIGHBOR_SUPERORDINATE_LINK_UP)
        {
            count++;
        }
        listItem = listItem->next;
    }

    return count;
}


/**
FUNCTION   :: Dot11sPortalList_Lookup
LAYER      :: MAC
PURPOSE    :: Search Portal list for an address.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ portalAddr : Mac802Address : address to be looked up
RETURN     :: DOT11s_PortalItem* : portal item or NULL
**/

static
DOT11s_PortalItem* Dot11sPortalList_Lookup(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address portalAddr)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* portalList = mp->portalList;

    ListItem* listItem = portalList->first;
    DOT11s_PortalItem* portalItem = NULL;
    BOOL isFound = FALSE;

    while (listItem != NULL)
    {
        portalItem = (DOT11s_PortalItem*) listItem->data;
        if (portalItem->portalAddr == portalAddr)
        {
            isFound = TRUE;
            break;
        }
        listItem = listItem->next;
    }

    return (isFound ? portalItem : NULL);
}


/**
FUNCTION   :: Dot11sPortalList_Finalize
LAYER      :: MAC
PURPOSE    :: Free portal list and contained items.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/

static
void Dot11sPortalList_Finalize(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* portalList = mp->portalList;

    if (portalList == NULL)
    {
        return;
    }

    if (DOT11s_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sPortalList_Finalize: "
            "Freed %d items",
            ListGetSize(node, portalList));
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    ListFree(node, portalList, FALSE);
}


/**
FUNCTION   :: Dot11sPortalList_AgeItems
LAYER      :: MAC
PURPOSE    :: Mark portals as inactive if portal active time has
                elapsed since the last PANN was received.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/

static
void Dot11sPortalList_AgeItems(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* portalList = mp->portalList;

    clocktype agingTime =
        node->getNodeTime() - mp->portalTimeout;
    if (agingTime < 0)
    {
        return;
    }

    ListItem* listItem = portalList->first;
    DOT11s_PortalItem* portalItem = NULL;

    while (listItem != NULL)
    {
        portalItem = (DOT11s_PortalItem*) listItem->data;
        if (portalItem->lastPannTime < agingTime)
        {
            if (DOT11s_TraceComments)
            {
                char traceStr[MAX_STRING_LENGTH];
                char portalStr[MAX_STRING_LENGTH];
                Dot11s_AddrAsDotIP(portalStr, &portalItem->portalAddr);
                sprintf(traceStr, "Dot11sPortalList_AgeItems: "
                    "Marking %s as inactive", portalStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            portalItem->isActive = FALSE;
        }
        listItem = listItem->next;
    }
}

/**
FUNCTION   :: Dot11sProxyList_Lookup
LAYER      :: MAC
PURPOSE    :: Search proxy list for a station address.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ staAddr   : Mac802Address : address of station
RETURN     :: DOT11s_ProxyItem* : proxy item or NULL
**/

DOT11s_ProxyItem* Dot11sProxyList_Lookup(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* proxyList = mp->proxyList;

    ListItem* listItem = proxyList->first;
    DOT11s_ProxyItem* proxyItem = NULL;
    BOOL isProxyFound = FALSE;

    while (listItem != NULL)
    {
        proxyItem = (DOT11s_ProxyItem*) listItem->data;
        if (proxyItem->staAddr == staAddr)
        {
            isProxyFound = TRUE;
            break;
        }
        listItem = listItem->next;
    }

    return (isProxyFound ? proxyItem : NULL);
}


/**
FUNCTION   :: Dot11sProxyList_Insert
LAYER      :: MAC
PURPOSE    :: Insert a proxy item in the proxy list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ staAddr   : Mac802Address : address of station
+ inMesh    : BOOL          : TRUE if station is within mesh
+ isProxied : BOOL          : TRUE if station is proxied
+ proxyAddr : Mac802Address : proxy address if station is proxied
RETURN     :: void
NOTES      :: If the proxy item exists in the list, the values are
                checked for any change and a trace print occurs.
                This can happen if a station changes MP association
                without the previous MP's knowledge.
**/

void Dot11sProxyList_Insert(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr,
    BOOL inMesh,
    BOOL isProxied,
    Mac802Address proxyAddr)
{
    DOT11s_Data* mp = dot11->mp;

    DOT11s_ProxyItem* proxyItem = NULL;

    proxyItem = Dot11sProxyList_Lookup(node, dot11, staAddr);
    if (proxyItem == NULL)
    {
        Dot11s_Malloc(DOT11s_ProxyItem, proxyItem);
        proxyItem->staAddr = staAddr;
        proxyItem->inMesh = inMesh;
        proxyItem->isProxied = isProxied;
        proxyItem->proxyAddr = proxyAddr;

        ListAppend(node, mp->proxyList, 0, proxyItem);
    }
    else
    {
        if (proxyItem->inMesh != inMesh
            || proxyItem->isProxied != isProxied
            || proxyItem->proxyAddr != proxyAddr)
        {
            if (DOT11s_TraceComments)
            {
                char traceStr[MAX_STRING_LENGTH];
                char staAddrStr[MAX_STRING_LENGTH];
                char proxyAddrStr1[MAX_STRING_LENGTH];
                char proxyAddrStr2[MAX_STRING_LENGTH];
                Dot11s_AddrAsDotIP(staAddrStr, &staAddr);
                Dot11s_AddrAsDotIP(proxyAddrStr1, &proxyItem->proxyAddr);
                Dot11s_AddrAsDotIP(proxyAddrStr2, &proxyAddr);
                sprintf(traceStr, "Dot11sProxyList_Insert: "
                    "Prev proxy item mismatch.\n"
                    "StaAddr=%s,\n"
                    "in mesh=%s/%s,\n"
                    "is proxied=%s/%s,\n"
                    "proxyAddr=%s/%s",
                    staAddrStr,
                    proxyItem->inMesh ? "T" : "F", inMesh ? "T" : "F",
                    proxyItem->isProxied ? "T" : "F", isProxied ? "T" : "F",
                    proxyAddrStr1, proxyAddrStr2);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            proxyItem->staAddr = staAddr;
            proxyItem->inMesh = inMesh;
            proxyItem->isProxied = isProxied;
            proxyItem->proxyAddr = proxyAddr;
        }
    }
}


/**
FUNCTION   :: Dot11sProxyList_DeleteItems
LAYER      :: MAC
PURPOSE    :: Delete proxied items for an MAP.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ proxyAddr : Mac802Address : proxy address if station is proxied
RETURN     :: void
**/

void Dot11sProxyList_DeleteItems(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address proxyAddr)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* proxyList = mp->proxyList;

    ListItem* listItem = proxyList->first;
    ListItem* tempItem;
    DOT11s_ProxyItem* proxyItem = NULL;
    int count = 0;

    while (listItem != NULL)
    {
        proxyItem = (DOT11s_ProxyItem*) listItem->data;
        tempItem = listItem;
        listItem = listItem->next;

        if (proxyItem->proxyAddr == proxyAddr)
        {
            count++;
            ListDelete(node, proxyList, tempItem, FALSE);
        }
    }

    if (DOT11s_TraceComments && count > 0)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr,
            "Dot11sProxyList_DeleteItems: Deleted %d items", count);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }
}


/**
FUNCTION   :: Dot11sProxyList_Finalize
LAYER      :: MAC
PURPOSE    :: Free proxy list and items.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/

static
void Dot11sProxyList_Finalize(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* proxyList = mp->proxyList;

    if (proxyList == NULL)
    {
        return;
    }

    if (DOT11s_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sProxyList_Finalize: "
            "Freed %d items",
            ListGetSize(node, proxyList));
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    ListFree(node, proxyList, FALSE);
}


/**
FUNCTION   :: Dot11sFwdList_Lookup
LAYER      :: MAC
PURPOSE    :: Search forwarding list for an address.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ addr      : Mac802Address : address to lookup
RETURN     :: DOT11s_FwdItem* : item if found or NULL
**/

DOT11s_FwdItem* Dot11sFwdList_Lookup(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address addr)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* fwdList = mp->fwdList;

    ListItem* listItem = fwdList->first;
    DOT11s_FwdItem* fwdItem = NULL;
    BOOL isFound = FALSE;

    while (listItem != NULL)
    {
        fwdItem = (DOT11s_FwdItem*) listItem->data;
        if (fwdItem->mpAddr == addr)
        {
            isFound = TRUE;
            break;
        }
        listItem = listItem->next;
    }

    return (isFound ? fwdItem : NULL);
}


/**
FUNCTION   :: Dot11sFwdList_Insert
LAYER      :: MAC
PURPOSE    :: Add an item to the forwarding list
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mpAddr    : Mac802Address : address of MP
+ nextHopAddr : Mac802Address : address of next hop towards the MP
+ itemType  : DOT11s_FwdItemType : item type
RETURN     :: void
NOTES      :: In case the item exists in the list, the values are
                validated and a trace print occurs.
**/

void Dot11sFwdList_Insert(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address mpAddr,
    Mac802Address nextHopAddr,
    DOT11s_FwdItemType itemType)
{
    DOT11s_Data* mp = dot11->mp;
    DOT11s_FwdItem* fwdItem = NULL;

    fwdItem = Dot11sFwdList_Lookup(node, dot11, mpAddr);
    if (fwdItem == NULL)
    {
        Dot11s_Malloc(DOT11s_FwdItem, fwdItem);
        fwdItem->mpAddr = mpAddr;
        fwdItem->nextHopAddr = nextHopAddr;
        fwdItem->itemType = itemType;

        ListAppend(node, mp->fwdList, 0, fwdItem);
    }
    else
    {
        if (fwdItem->nextHopAddr != nextHopAddr
            || fwdItem->itemType != itemType)
        {
            if (DOT11s_TraceComments)
            {
                char traceStr[MAX_STRING_LENGTH];
                char mpAddrStr[MAX_STRING_LENGTH];
                char nextHopAddrStr1[MAX_STRING_LENGTH];
                char nextHopAddrStr2[MAX_STRING_LENGTH];
                Dot11s_AddrAsDotIP(mpAddrStr, &mpAddr);
                Dot11s_AddrAsDotIP(nextHopAddrStr1, &fwdItem->nextHopAddr);
                Dot11s_AddrAsDotIP(nextHopAddrStr2, &nextHopAddr);
                sprintf(traceStr, "!! Dot11sFwdList_Insert: "
                    "Prev Fwd item mismatch. "
                    "MpAddr=%s, "
                    "nextHop=%s/%s, "
                    "itemType=%d/%d",
                    mpAddrStr,
                    nextHopAddrStr1, nextHopAddrStr2,
                    fwdItem->itemType, itemType);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            if (fwdItem->itemType != DOT11s_FWD_UNKNOWN
                && fwdItem->itemType != itemType)
            {
                ERROR_ReportError("Dot11sFwdList_Insert: "
                    "Item has switched between in-mesh and ex-mesh.\n");
            }

            fwdItem->mpAddr = mpAddr;
            fwdItem->nextHopAddr = nextHopAddr;
            fwdItem->itemType = itemType;
        }
    }
}


/**
FUNCTION   :: Dot11sFwdList_Finalize
LAYER      :: MAC
PURPOSE    :: Free forwarding list and contained items.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/

static
void Dot11sFwdList_Finalize(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* fwdList = mp->fwdList;

    if (fwdList == NULL)
    {
        return;
    }

    if (DOT11s_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sFwdList_Finalize: "
            "Freed %d items",
            ListGetSize(node, fwdList));
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    ListFree(node, fwdList, FALSE);
}


/**
FUNCTION   :: Dot11sDataSeenList_Insert
LAYER      :: MAC
PURPOSE    :: Add message details to the data seen list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : message to be inserted
RETURN     :: void
NOTES      :: Assumes a 4 address mesh data frame.
**/

void Dot11sDataSeenList_Insert(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11s_DataSeenItem* dataSeenItem =
        Dot11sDataSeenList_Lookup(node, dot11, msg);
    if (dataSeenItem != NULL)
    {
        return;
    }

    DOT11s_Data* mp = dot11->mp;
    LinkedList* dataSeenList = mp->dataSeenList;

    DOT11_ShortControlFrame* hdr =
        (DOT11_ShortControlFrame*) MESSAGE_ReturnPacket(msg);
    ERROR_Assert(hdr->frameType == DOT11_MESH_DATA,
        "Dot11sDataSeenList_Insert: Not a mesh data frame.\n");

    DOT11s_FrameHdr meshHdr;
    Dot11s_ReturnMeshHeader(&meshHdr, msg);

    Dot11s_MallocMemset0(DOT11s_DataSeenItem, dataSeenItem);
    dataSeenItem->RA = meshHdr.destAddr;
    dataSeenItem->TA = meshHdr.sourceAddr;
    dataSeenItem->DA = meshHdr.address3;
    dataSeenItem->SA = meshHdr.address4;
    dataSeenItem->fwdControl.SetE2eSeqNo(meshHdr.fwdControl.GetE2eSeqNo());
    dataSeenItem->fwdControl.SetTTL(meshHdr.fwdControl.GetTTL());
    dataSeenItem->insertTime = node->getNodeTime();

    // Insert at the beginning
    ListPrepend(node, dataSeenList, 0, dataSeenItem);
}


/**
FUNCTION   :: Dot11sDataSeenList_Lookup
LAYER      :: MAC
PURPOSE    :: Search for message in data seen list using the
                tuple DA/SA/E2E sequence number.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : message to be looked up
RETURN     :: DOT11s_DataSeenItem* : value or NULL
**/

DOT11s_DataSeenItem* Dot11sDataSeenList_Lookup(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* dataSeenList = mp->dataSeenList;

    DOT11_ShortControlFrame* hdr =
        (DOT11_ShortControlFrame*) MESSAGE_ReturnPacket(msg);
    ERROR_Assert(hdr->frameType == DOT11_MESH_DATA,
        "Dot11sDataSeenList_Lookup: Not a mesh data frame.\n");

    DOT11s_FrameHdr meshHdr;
    Dot11s_ReturnMeshHeader(&meshHdr, msg);

    ListItem* listItem = dataSeenList->first;
    DOT11s_DataSeenItem* dataSeenItem = NULL;
    BOOL isDataItemFound = FALSE;

    while (listItem != NULL)
    {
        dataSeenItem = (DOT11s_DataSeenItem*) listItem->data;
        if (dataSeenItem->DA == meshHdr.address3
            && dataSeenItem->SA == meshHdr.address4
            && dataSeenItem->fwdControl.GetE2eSeqNo()
                == meshHdr.fwdControl.GetE2eSeqNo() )
        {
            if (DOT11s_TraceComments)
            {
                char traceStr[MAX_STRING_LENGTH];
                char addr1Str[MAX_STRING_LENGTH];
                char addr2Str[MAX_STRING_LENGTH];
                Dot11s_AddrAsDotIP(addr1Str, &meshHdr.address3);
                Dot11s_AddrAsDotIP(addr2Str, &meshHdr.address4);
                sprintf(traceStr, "Dot11sDataSeenList_Lookup: "
                    "found DA=%s, SA=%s, E2E=%d",
                    addr1Str,
                    addr2Str,
                    meshHdr.fwdControl.GetE2eSeqNo());
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            isDataItemFound = TRUE;
            break;
        }
        listItem = listItem->next;
    }

    return (isDataItemFound ? dataSeenItem : NULL);
}


/**
FUNCTION   :: Dot11sDataSeenList_AgeItems
LAYER      :: MAC
PURPOSE    :: Remove items from seen list that are older than
                aging time.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
NOTES      :: Aging time is approximate 4 * net traversal time.
                Traverses the list from the rear as new items are
                prepended to the list.
**/

static
void Dot11sDataSeenList_AgeItems(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* dataSeenList = mp->dataSeenList;

    clocktype agingTime = node->getNodeTime() - DOT11s_DATA_SEEN_AGING_TIME;
    if (agingTime < 0)
    {
        return;
    }

    ListItem* listItem = dataSeenList->last;
    DOT11s_DataSeenItem* dataSeenItem = NULL;

    while (listItem != NULL)
    {
        dataSeenItem = (DOT11s_DataSeenItem*) listItem->data;
        if (dataSeenItem->insertTime < agingTime)
        {
            ListDelete(node, dataSeenList, listItem, FALSE);
            listItem = dataSeenList->last;
        }
        else
        {
            break;
        }
    }
}


/**
FUNCTION   :: Dot11sDataSeenList_Finalize
LAYER      :: MAC
PURPOSE    :: Clear and free the data seen list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/

static
void Dot11sDataSeenList_Finalize(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* dataSeenList = mp->dataSeenList;

    if (dataSeenList == NULL)
    {
        return;
    }

    if (DOT11s_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sDataSeenList_Finalize: "
            "Freed %d items",
            ListGetSize(node, dataSeenList));
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    ListFree(node, dataSeenList, FALSE);
}


/**
FUNCTION   :: Dot11sStationList_Lookup
LAYER      :: MAC
PURPOSE    :: Search for station in list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ staAddr   : Mac802Address : address of station to lookup
RETURN     :: DOT11s_StationItem* : value or NULL
**/

DOT11s_StationItem* Dot11sStationList_Lookup(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* list = mp->stationList;

    ListItem* listItem = list->first;
    DOT11s_StationItem* item = NULL;
    BOOL isItemFound = FALSE;

    while (listItem != NULL)
    {
        item = (DOT11s_StationItem*) listItem->data;
        if (item->staAddr == staAddr)
        {
            if (DOT11s_TraceComments)
            {
                //char traceStr[MAX_STRING_LENGTH];
                //char staStr[MAX_STRING_LENGTH];
                //Dot11s_AddrAsDotIP(staStr, item->staAddr);
                //char prevApStr[MAX_STRING_LENGTH];
                //Dot11s_AddrAsDotIP(prevApStr, item->prevApAddr);
                //sprintf(traceStr, "Dot11sStationList_Lookup: "
                //    "found sta=%s, prevAp=%s, status=%d",
                //    staStr, prevApStr, item->status);
                //MacDot11Trace(node, dot11, NULL, traceStr);
            }

            isItemFound = TRUE;
            break;
        }
        listItem = listItem->next;
    }

    return (isItemFound ? item : NULL);
}


/**
FUNCTION   :: Dot11sStationList_Insert
LAYER      :: MAC
PURPOSE    :: Add station details to the list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ staAddr   : Mac802Address : address of station to add
+ seqNo     : int           : sequence number
+ status    : DOT11s_StationStatus : status of station
RETURN     :: void
**/

static
void Dot11sStationList_Insert(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr,
    Mac802Address previousAp,
    DOT11s_StationStatus status = DOT11s_STATION_ACTIVE)
{
    ERROR_Assert(dot11->isMAP,
        "Dot11sStationList_Insert: Not an MAP.\n");

    DOT11s_StationItem* item =
        Dot11sStationList_Lookup(node, dot11, staAddr);
    if (item != NULL)
    {
        if (DOT11s_TraceComments)
        {
            char traceStr[MAX_STRING_LENGTH];
            char staStr[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(staStr, &item->staAddr);
            char prevApStr[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(prevApStr, &item->prevApAddr);
            sprintf(traceStr, "Dot11sStationList_Insert: "
                "exists sta=%s, prevAp=%s, status=%d",
                staStr, prevApStr, item->status);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }

        return;
    }

    DOT11s_Data* mp = dot11->mp;
    LinkedList* list = mp->stationList;

    Dot11s_MallocMemset0(DOT11s_StationItem, item);
    item->staAddr = staAddr;
    item->prevApAddr = previousAp;
    item->status = status;

    ListAppend(node, list, 0, item);
}


/**
FUNCTION   :: Dot11sStationList_Delete
LAYER      :: MAC
PURPOSE    :: Delete a station from the list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ staAddr   : Mac802Address : address of station to add
RETURN     :: void
**/

static
void Dot11sStationList_Delete(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr)
{
    ERROR_Assert(dot11->isMAP,
        "Dot11sStationList_Delete: Not an MAP.\n");

    LinkedList* list = dot11->mp->stationList;
    ListItem* listItem = list->first;
    DOT11s_StationItem* item = NULL;

    while (listItem != NULL)
    {
        item = (DOT11s_StationItem*) listItem->data;
        if (item->staAddr == staAddr)
        {
            if (DOT11s_TraceComments)
            {
                char traceStr[MAX_STRING_LENGTH];
                char staStr[MAX_STRING_LENGTH];
                Dot11s_AddrAsDotIP(staStr, &item->staAddr);
                sprintf(traceStr, "Dot11sStationList_Delete: "
                    "deleting sta=%s", staStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            ListDelete(node, list, listItem, FALSE);
            break;
        }
        listItem = listItem->next;
    }
}


/**
FUNCTION   :: Dot11sStationList_Finalize
LAYER      :: MAC
PURPOSE    :: Clear and free the staton list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/

static
void Dot11sStationList_Finalize(
    Node* node,
    MacDataDot11* dot11)
{
    LinkedList* list = dot11->mp->stationList;

    if (list == NULL)
    {
        return;
    }

    if (DOT11s_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sStationList_Finalize: "
            "Freed %d items",
            ListGetSize(node, list));
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    ListFree(node, list, FALSE);
}


/**
FUNCTION   :: Dot11s_GetAssociatedStationCount
LAYER      :: MAC
PURPOSE    :: Get count of associated stations.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: int           : count of associated stations.
**/

int Dot11s_GetAssociatedStationCount(
    Node* node,
    MacDataDot11* dot11)
{
    if (dot11->isMAP)
    {
        //return MacDot11ApStationListGetSize(node, dot11);
        return ListGetSize(node, dot11->mp->stationList);
    }
    else
    {
        return 0;
    }
}

/**
FUNCTION   :: Dot11sE2eList_Insert
LAYER      :: MAC
PURPOSE    :: Add e2e details to the e2e list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ DA        : Mac802Address : destination address
+ SA        : Mac802Address : source address
+ seqNo     : int           : sequence number
RETURN     :: DOT11s_E2eItem* : item inserted
**/

DOT11s_E2eItem* Dot11sE2eList_Insert(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address DA,
    Mac802Address SA,
    int seqNo)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* list = mp->e2eList;

    DOT11s_E2eItem* item;
    Dot11s_MallocMemset0(DOT11s_E2eItem, item);
    item->DA = DA;
    item->SA = SA;
    item->seqNo = seqNo;

    ListAppend(node, list, 0, item);

    return item;
}


/**
FUNCTION   :: Dot11sE2eList_Lookup
LAYER      :: MAC
PURPOSE    :: Search for item in end to end list given DA, SA.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ DA        : Mac802Address : destination address
+ SA        : Mac802Address : source address
RETURN     :: DOT11s_E2eItem* : value or NULL
**/

DOT11s_E2eItem* Dot11sE2eList_Lookup(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address DA,
    Mac802Address SA)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* list = mp->e2eList;

    ListItem* listItem = list->first;
    DOT11s_E2eItem* e2eItem = NULL;
    BOOL isFound = FALSE;

    while (listItem != NULL)
    {
        e2eItem = (DOT11s_E2eItem*) listItem->data;
        if (e2eItem->DA == DA
            && e2eItem->SA == SA)
        {
            //if (DOT11s_TraceComments)
            //{
            //    char traceStr[MAX_STRING_LENGTH];
            //    char addr1Str[MAX_STRING_LENGTH];
            //    char addr2Str[MAX_STRING_LENGTH];
            //    Dot11s_AddrAsDotIP(addr1Str, DA);
            //    Dot11s_AddrAsDotIP(addr2Str, SA);
            //    sprintf(traceStr, "Dot11sE2eList_Lookup: "
            //        "found DA=%s, SA=%s, E2E=%d",
            //        addr1Str, addr2Str, e2eItem->seqNo);
            //    MacDot11Trace(node, dot11, NULL, traceStr);
            //}

            isFound = TRUE;
            break;
        }
        listItem = listItem->next;
    }

    return (isFound ? e2eItem : NULL);
}


/**
FUNCTION   :: Dot11sE2eList_NextSeqNo
LAYER      :: MAC
PURPOSE    :: Increase sequence number and return next value.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ item      : DOT11s_E2eItem* : pointer to e2e item
RETURN     :: unsigned short : next sequence number
**/

unsigned short Dot11sE2eList_NextSeqNo(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address DA,
    Mac802Address SA)
{
    DOT11s_E2eItem* item = Dot11sE2eList_Lookup(node, dot11, DA, SA);
    if (item == NULL)
    {
        item = Dot11sE2eList_Insert(node, dot11, DA, SA, 0);
    }

    item->seqNo++;
    if (item->seqNo > 0xFFFF)
    {
        item->seqNo = 0;
    }

    return (unsigned short) item->seqNo;
}


/**
FUNCTION   :: Dot11sE2eList_Finalize
LAYER      :: MAC
PURPOSE    :: Clear and free the end to end list.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/

static
void Dot11sE2eList_Finalize(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;
    LinkedList* list = mp->e2eList;

    if (list == NULL)
    {
        return;
    }

    if (DOT11s_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sE2eList_Finalize: "
            "Freed %d items",
            ListGetSize(node, list));
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    ListFree(node, list, FALSE);
}


// ------------------------------------------------------------------

/**
FUNCTION   :: Dot11s_SetMpState
LAYER      :: MAC
PURPOSE    :: Set the mesh point state. Save previous state.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ state     : DOT11s_MpState : new state to be set
RETURN     :: void
**/

static
void Dot11s_SetMpState(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_MpState state)
{
    dot11->mp->prevState = dot11->mp->state;
    dot11->mp->state = state;
}


/**
FUNCTION   :: Dot11s_StartTimer
LAYER      :: MAC
PURPOSE    :: Start a self timer message.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ timerInfo : DOT11s_TimerInfo* : info to be added to message
+ delay     : clocktype     : timer delay
+ timerType : int           : timer type
RETURN     :: Message*      : timer message
**/

Message* Dot11s_StartTimer(
    Node* node,
    MacDataDot11* dot11,
    const DOT11s_TimerInfo* const timerInfo,
    clocktype delay,
    int timerType)
{
    ERROR_Assert(timerInfo != NULL,
        "Dot11s_StartTimer: timer info is NULL.");

    Message* msg = MESSAGE_Alloc(
        node, MAC_LAYER, MAC_PROTOCOL_DOT11, timerType);
    MESSAGE_SetInstanceId(msg, (short) dot11->myMacData->interfaceIndex);

    MESSAGE_InfoAlloc(node, msg, sizeof(DOT11s_TimerInfo));
    DOT11s_TimerInfo* msgInfo = (DOT11s_TimerInfo*) MESSAGE_ReturnInfo(msg);
    memcpy(msgInfo, timerInfo, sizeof(DOT11s_TimerInfo));

    MESSAGE_Send(node, msg, delay);

    return msg;
}


/**
FUNCTION   :: Dot11s_SetFieldsInMgmtFrameHdr
LAYER      :: MAC
PURPOSE    :: Set fields in frame header from info details
                suitable for a mesh management frame.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ fHdr      : DOT11_FrameHdr*   : pointer to frame header
+ frameInfo : DOT11s_FrameInfo* : frame related info
RETURN     :: void
NOTES      :: The transmit function would fill in duration and
                sequence number. The proposed mesh BSS ID
                (all 0s) is used.
**/

void Dot11s_SetFieldsInMgmtFrameHdr(
    Node* node,
    const MacDataDot11* const dot11,
    DOT11_FrameHdr* const fHdr,
    DOT11s_FrameInfo* frameInfo)
{
    fHdr->frameType = frameInfo->frameType;

    fHdr->frameFlags = 0;   // other flags assumed 0
    fHdr->frameFlags |= DOT11_TO_DS | DOT11_FROM_DS;

    fHdr->duration = 0;
    fHdr->seqNo = 0;
    fHdr->fragId = 0;

    fHdr->destAddr = frameInfo->RA;
    fHdr->sourceAddr = frameInfo->TA;
    fHdr->address3 = DOT11s_MESH_BSS_ID;
}


/**
FUNCTION   :: Dot11s_SetFieldsInDataFrameHdr
LAYER      :: MAC
PURPOSE    :: Based on values in frameInfo, set fields in frame
                header suitable for a mesh data frame.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ fHdr      : DOT11s_FrameHdr*   : pointer to mesh frame header
+ frameInfo : DOT11s_FrameInfo*  : frame related info
RETURN     :: void
NOTES      :: Duration and sequence number are inserted by the
                "transmit function" depending on values
                prevalent at the time of dequeue.
**/

void Dot11s_SetFieldsInDataFrameHdr(
    Node* node,
    const MacDataDot11* const dot11,
    char* const dot11Hdr,
    DOT11s_FrameInfo* frameInfo)
{

    DOT11s_FrameHdr fHdr;
    fHdr.frameType = frameInfo->frameType;

    fHdr.frameFlags = 0;   // other flags assumed 0
    fHdr.frameFlags |= DOT11_TO_DS | DOT11_FROM_DS;

    // Duration, sequence number, and fragment number
    // are set in the transmit function.
    fHdr.duration = 0;
    fHdr.seqNo = 0;
    fHdr.fragId = 0;

    fHdr.destAddr = frameInfo->RA;
    fHdr.sourceAddr = frameInfo->TA;
    fHdr.address3 = frameInfo->DA;
    fHdr.address4 = frameInfo->SA;

    fHdr.fwdControl = frameInfo->fwdControl;
    memcpy(dot11Hdr, &fHdr, sizeof(DOT11s_FrameHdr));
}


// Sending mesh elements in a beacon frame is hooked
// into MacDot11ApTransmitBeacon().
static
void Dot11s_SendBeaconFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    ERROR_ReportError("Dot11s_SendBeaconFrame: Not implemented.\n");
}


/**
FUNCTION   :: Dot11s_ReceiveBeaconFrame
LAYER      :: MAC
PURPOSE    :: Receive and process a beacon frame if it has mesh elements.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received beacon message
+ isProcessd : BOOL*        : will be set to TRUE is message is processed
RETURN     :: void
NOTES      :: The mesh elements are matched for an acceptable profile.
                The sender, if new, is added to the neighbor list.
                Link Setup would, subsequently, start the association process.
                The code also checks for beacon overlap during the initial
                startup state and adjusts beacon offset if required.
**/

void Dot11s_ReceiveBeaconFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    BOOL* isProcessed)
{
    DOT11s_Data* mp = dot11->mp;
    *isProcessed = FALSE;

    DOT11_FrameHdr* frameHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    ERROR_Assert(frameHdr->frameType == DOT11_BEACON,
        "Dot11s_ReceiveBeaconFrame: Wrong frame type.\n");
    DOT11_BeaconFrame* beaconFrame = (DOT11_BeaconFrame*) frameHdr;

    int offset = sizeof(DOT11_BeaconFrame);
    int rxFrameSize = MESSAGE_ReturnPacketSize(msg);

    if (offset >= rxFrameSize)
    {
        if (DOT11s_TraceComments)
        {
            char addrTA[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(addrTA, &frameHdr->sourceAddr);

            char traceStr[MAX_STRING_LENGTH];
            sprintf(traceStr, "Dot11s_ReceiveBeaconFrame "
                "Beacon from TA = %s contains only fixed length fields",
                addrTA);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }

        return;
    }

    Mac802Address sourceAddr = frameHdr->sourceAddr;
    DOT11_Ie element;
    DOT11_IeHdr ieHdr;
    unsigned char* rxFrame = (unsigned char *) frameHdr;

    ieHdr = rxFrame + offset;

    // The first expected mesh IE is Mesh ID
    while (ieHdr.id != DOT11_IE_MESH_ID)
    {
        offset += ieHdr.length + 2;
        ieHdr = rxFrame + offset;
        if (offset >= rxFrameSize)
        {
            return;
        }
    }

    // Process the mesh elements in the beacon

    char meshId[DOT11s_MESH_ID_LENGTH_MAX + 1] = DOT11s_MESH_ID_INVALID;
    DOT11s_PathProfile pathProfile;
    int peerCapacity = 0;
    DOT11s_NeighborItem* neighborItem;

    // Parse Mesh ID element
    if (ieHdr.id == DOT11_IE_MESH_ID)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11s_ParseMeshIdElement(node, &element, meshId);

        ieHdr = rxFrame + offset;
    }
    else
    {
        if (DOT11s_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! Mesh ID element not found");
        }
    }

    // Parse Mesh Capability, if present
    if (ieHdr.id == DOT11_IE_MESH_CAPABILITY)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11s_ParseMeshCapabilityElement(node, &element,
            &pathProfile, &peerCapacity);

        ieHdr = rxFrame + offset;
    }
    else
    {
        if (DOT11s_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! Mesh Capability element not found");
        }
    }

    // Neighbor list is applicable to DTIM beacon frames; ignore for now.

    // Compare match to any profile; currently, there is only one profile.
    // Version number in Mesh Capability is assumed to be the same.
    // MP is assumed to be active.
    // Note that there are no SSID considerations.
    if ((strcmp(mp->meshId, meshId) == 0)
        && (mp->pathProtocol == pathProfile.pathProtocol)
        && (mp->pathMetric == pathProfile.pathMetric))
    {
        mp->stats.beaconsReceived++;
        dot11->beaconPacketsGot++;

        MacDot11Trace(node, dot11, msg, "Receive");

        // Check if the MP is already a neighbor
        neighborItem = Dot11sNeighborList_Lookup(node, dot11, sourceAddr);

        if (neighborItem == NULL)
        {
            // Add neighbor to list
            Dot11s_MallocMemset0(DOT11s_NeighborItem, neighborItem);
            ListAppend(node, mp->neighborList, 0, neighborItem);

            neighborItem->neighborAddr = sourceAddr;
            neighborItem->primaryAddr = dot11->selfAddr;
            // Authentication frame exchange not implemented.
            neighborItem->isAuthenticated = TRUE;

            neighborItem->phyModel =
                PHY_GetModel(node, dot11->myMacData->phyNumber);
            // Temporary. Overwritten later during Link State.
            neighborItem->dataRateInMbps = Dot11s_GetDataRateInMbps(
                node, neighborItem->phyModel,
                dot11->myMacData->phyNumber,
                PHY_GetRxDataRateType(node, dot11->myMacData->phyNumber));

            if (DOT11s_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL, "Add new neighbor");
            }

            Dot11s_SetNeighborState(node,
                neighborItem, DOT11s_NEIGHBOR_NO_CAPABILITY);
            Dot11sAssoc_SetState(node, dot11,
                neighborItem, DOT11s_ASSOC_S_IDLE);

            neighborItem->beaconInterval = beaconFrame->beaconInterval;
            neighborItem->firstBeaconTime = node->getNodeTime();

        }

        if (peerCapacity == 0
            && neighborItem->state == DOT11s_NEIGHBOR_CANDIDATE_PEER)
        {
            Dot11s_SetNeighborState(node,
                neighborItem, DOT11s_NEIGHBOR_NO_CAPABILITY);
            // Need this?
            mp->assocStateData.event = DOT11s_ASSOC_S_EVENT_CANCEL_LINK;
            mp->assocStateData.neighborItem = neighborItem;
            neighborItem->assocStateFn(node, dot11, mp);
        }

        if (peerCapacity > 0
            && neighborItem->state == DOT11s_NEIGHBOR_NO_CAPABILITY)
        {
            Dot11s_SetNeighborState(node,
                neighborItem, DOT11s_NEIGHBOR_CANDIDATE_PEER);
            mp->assocStateData.event = DOT11s_ASSOC_S_EVENT_PASSIVE_OPEN;
            mp->assocStateData.neighborItem = neighborItem;
            neighborItem->assocStateFn(node, dot11, mp);
        }

        // Capture data; use for decisions if required
        neighborItem->beaconInterval = beaconFrame->beaconInterval;
        neighborItem->lastBeaconTime = node->getNodeTime();
        neighborItem->beaconsReceived++;

        // Check beacon timing for overlap during the Init state.
        if (mp->state == DOT11s_S_INIT_START)
        {
            int timeStamp =
                MacDot11ClocktypeToTUs(beaconFrame->timeStamp);
            int beaconInterval = beaconFrame->beaconInterval;

            // If neighbor's beacon interval is not the same,
            // it would be difficult to avoid overlap.
            if (beaconInterval ==
                MacDot11ClocktypeToTUs(dot11->beaconInterval))
            {
                int neighborOffset = timeStamp % beaconInterval;
                int ourOffset =
                    MacDot11ClocktypeToTUs(dot11->beaconTime)
                    % beaconInterval;

                // For 802.11b, a beacon takes about 1 TU.
                // For 802.11a, a beacon takes about 0.1 TU.
                // Simply, make the difference at least + or - 1 TU.
                if (neighborOffset == ourOffset)
                {
                    if (dot11->beaconTime < node->getNodeTime() && 
                       dot11->beaconInterval > 0)
                    {
                        dot11->beaconTime += dot11->beaconInterval;

                        while (dot11->beaconTime < node->getNodeTime())
                        {
                            dot11->beaconTime += dot11->beaconInterval;
                        }
                    }

                    char traceStr[MAX_STRING_LENGTH];
                    char neighborStr[MAX_STRING_LENGTH];
                    Dot11s_AddrAsDotIP(
                        neighborStr, &neighborItem->neighborAddr);
                    sprintf(traceStr,
                        "Beacon offset change. Neighbor %s was %d TUs",
                        neighborStr, neighborOffset);
                    MacDot11Trace(node, dot11, NULL, traceStr);

                    // Start a beacon timer with new beacon time.
                    MacDot11StationStartTimerOfGivenType(
                        node, dot11,
                        dot11->beaconTime - node->getNodeTime(),
                        MSG_MAC_DOT11_Beacon);

                }
            }
        } // if (mp->state == DOT11s_S_INIT_START)
    }
    else
    {
        if (DOT11s_TraceComments)
        {
            char addrTA[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(addrTA, &frameHdr->sourceAddr);
            char traceStr[MAX_STRING_LENGTH];
            sprintf(traceStr, "Dot11s_ReceiveBeaconFrame "
                "Beacon from TA = %s does not contain a matching "
                "Mesh ID or metric or protocol", addrTA);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }
    }

    *isProcessed = TRUE;
}


// The mesh code does not use authentication between MPs.
// Note: Authorization between a STA and AP is handled separately.
static
void Dot11s_SendAuthRequestFrame(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr)
{
    ERROR_ReportError("Dot11s_SendAuthRequestFrame: Not implemented.\n");
}


// There is currently no authentication between MPs.
// The authentication exchange between STA and APs is handled
// separately in mac_dot11-mgmt.cpp
static
void Dot11s_SendAuthResponseFrame(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr)
{
    ERROR_ReportError("Dot11s_SendAuthResponseFrame: Not implemented.\n");
}


// Currently, mesh neighbors do not authenticate.
static
void Dot11s_ReceiveAuthenticateFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    BOOL* isProcessed)
{
    MacDot11Trace(node, dot11, NULL,
        "Dot11s_ReceiveAuthenticateFrame: Not implemented.\n");
    *isProcessed = FALSE;
}


/**
FUNCTION   :: Dot11s_SendAssocRequestFrame
LAYER      :: MAC
PURPOSE    :: Send an association request to a mesh neighor.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ destAddr  : Mac802Address : destination address for association
+ neighborItem : DOT11s_NeighborItem* : mesh neighbor details
RETURN     :: void
**/

static
void Dot11s_SendAssocRequestFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_NeighborItem* neighborItem)
{
    DOT11_MacFrame fullFrame;
    Dot11s_Memset0(DOT11_MacFrame, &fullFrame);
    int txFrameSize;

    DOT11s_AssocData assocData;
    assocData.linkId = neighborItem->linkId;

    Dot11s_CreateAssocRequestFrame(node, dot11,
        &assocData, &fullFrame, &txFrameSize);

    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, msg, txFrameSize, TRACE_DOT11);
    DOT11_AssocReqFrame* txFrame =
        (DOT11_AssocReqFrame*) MESSAGE_ReturnPacket(msg);
    memcpy(txFrame, &fullFrame, (size_t)txFrameSize);
    memset(txFrame, 0, sizeof(DOT11_AssocReqFrame));

    DOT11s_FrameInfo* newFrameInfo;
    Dot11s_MallocMemset0(DOT11s_FrameInfo, newFrameInfo);
    newFrameInfo->msg = msg;
    newFrameInfo->frameType = DOT11_ASSOC_REQ;
    newFrameInfo->RA = neighborItem->neighborAddr;
    newFrameInfo->TA = dot11->selfAddr;
    newFrameInfo->SA = INVALID_802ADDRESS;

    // Fill in frame body
    //assocFrame->capInfo = NULL;
    txFrame->listenInterval = 1;
    strcpy(txFrame->ssid, dot11->stationMIB->dot11DesiredSSID);
    //assocFrame->supportedRates = NULL;

    Dot11s_SetFieldsInMgmtFrameHdr(
        node, dot11, &(txFrame->hdr), newFrameInfo);
    Dot11sMgmtQueue_EnqueuePacket(node, dot11, newFrameInfo);

    dot11->mp->stats.assocRequestsSent++;
}


/**
FUNCTION   :: Dot11s_SendAssocConfirmFrame
LAYER      :: MAC
PURPOSE    :: Send an association confirmation to a mesh neighor.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ neighborItem : DOT11s_NeighborItem* : mesh neighbor details
RETURN     :: void
NOTES      :: STAs to AP assoc. request/response is handled separately.
**/

static
void Dot11s_SendAssocConfirmFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_NeighborItem* neighborItem)
{
    DOT11_MacFrame fullFrame;
    Dot11s_Memset0(DOT11_MacFrame, &fullFrame);
    int txFrameSize;

    DOT11s_AssocData assocData;
    assocData.linkId = neighborItem->linkId;
    assocData.peerLinkId = neighborItem->peerLinkId;

    Dot11s_CreateAssocResponseFrame(node, dot11,
        &assocData, &fullFrame, &txFrameSize);

    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, msg, txFrameSize, TRACE_DOT11);
    DOT11_AssocRespFrame* txFrame =
        (DOT11_AssocRespFrame*) MESSAGE_ReturnPacket(msg);
    memcpy(txFrame, &fullFrame, (size_t)txFrameSize);
    memset(txFrame, 0, sizeof(DOT11_AssocRespFrame));

    DOT11s_FrameInfo* newFrameInfo;
    Dot11s_MallocMemset0(DOT11s_FrameInfo, newFrameInfo);
    newFrameInfo->msg = msg;
    newFrameInfo->frameType = DOT11_ASSOC_RESP;
    newFrameInfo->RA = neighborItem->neighborAddr;
    newFrameInfo->TA = dot11->selfAddr;
    newFrameInfo->SA = INVALID_802ADDRESS;

    // Fill in frame body
    // Do not particularly care about association ID or status
    // assocFrame->capInfo = NULL;
    // txFrame->assocId = 0;
    // txFrame->statusCode = DOT11_SC_SUCCESSFUL;
    // assocFrame->supportedRates = NULL;

    // Note: No check for matching SSID

    Dot11s_SetFieldsInMgmtFrameHdr(node, dot11,
        &(txFrame->hdr), newFrameInfo);
    Dot11sMgmtQueue_EnqueuePacket(node, dot11, newFrameInfo);

    dot11->mp->stats.assocResponsesSent++;

}


/**
FUNCTION   :: Dot11s_SendAssocCloseFrame
LAYER      :: MAC
PURPOSE    :: Send an disassociation frame to close peer link.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ neighborItem : DOT11s_NeighborItem* : mesh neighbor details
+ status    : DOT11s_PeerLinkStatus : close reason code
RETURN     :: void
**/

static
void Dot11s_SendAssocCloseFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_NeighborItem* neighborItem,
    DOT11s_PeerLinkStatus status)
{
    DOT11_MacFrame fullFrame;
    Dot11s_Memset0(DOT11_MacFrame, &fullFrame);
    int txFrameSize;

    DOT11s_AssocData assocData;
    assocData.linkId = neighborItem->linkId;
    assocData.peerLinkId = neighborItem->peerLinkId;
    assocData.status = (unsigned char) status;

    Dot11s_CreateAssocCloseFrame(node, dot11,
        &assocData, &fullFrame, &txFrameSize);

    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, msg, txFrameSize, TRACE_DOT11);
    DOT11_DisassocFrame* txFrame =
        (DOT11_DisassocFrame*) MESSAGE_ReturnPacket(msg);
    memcpy(txFrame, &fullFrame, (size_t)txFrameSize);
    memset(txFrame, 0, sizeof(DOT11_DisassocFrame));

    DOT11s_FrameInfo* newFrameInfo;
    Dot11s_MallocMemset0(DOT11s_FrameInfo, newFrameInfo);
    newFrameInfo->msg = msg;
    newFrameInfo->frameType = DOT11_DISASSOCIATION;
    newFrameInfo->RA = neighborItem->neighborAddr;
    newFrameInfo->TA = dot11->selfAddr;
    newFrameInfo->SA = INVALID_802ADDRESS;

    // Do not particularly care about disassociation reason
    // txFrame->reasonCode = DOT11_RC_UNSPEC_REASON;

    Dot11s_SetFieldsInMgmtFrameHdr(node, dot11,
        &(txFrame->hdr), newFrameInfo);
    Dot11sMgmtQueue_EnqueuePacket(node, dot11, newFrameInfo);

    dot11->mp->stats.assocCloseSent++;
}


/**
FUNCTION   :: Dot11s_ReceiveAssocRequestFrame
LAYER      :: MAC
PURPOSE    :: Process an association request from a mesh neighbor.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received assoc. request message
+ isProcessd : BOOL*        : will be set to TRUE is message is processed
RETURN     :: void
NOTES      :: If the frame, does not have mesh elements, it must
                be a request from a STA to an AP, and is passed to
                the mgmt code for further action (isProcessed is
                set to FALSE).
                Currently, if the profile does not match, the
                code simply asserts. An association response is
                assumed to be a success response.
**/

static
void Dot11s_ReceiveAssocRequestFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    BOOL* isProcessed)
{
    DOT11s_Data* mp = dot11->mp;
    *isProcessed = FALSE;

    DOT11_FrameHdr* frameHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    ERROR_Assert(frameHdr->frameType == DOT11_ASSOC_REQ,
        "Dot11s_ReceiveAssocRequestFrame: Wrong frame type.\n");

    // Skip over fixed length fields
    int offset = sizeof(DOT11_AssocReqFrame);
    int rxFrameSize = MESSAGE_ReturnPacketSize(msg);

    if (offset >= rxFrameSize)
    {
        if (DOT11s_TraceComments)
        {
            char addrTA[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(addrTA, &frameHdr->sourceAddr);

            char traceStr[MAX_STRING_LENGTH];
            sprintf(traceStr, "!! Dot11s_ReceiveAssocRequestFrame: "
                "Request from %s, without mesh elements",
                addrTA);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }

        return;
    }

    Mac802Address sourceAddr = frameHdr->sourceAddr;
    DOT11_Ie element;
    DOT11_IeHdr ieHdr;

    unsigned char* rxFrame = (unsigned char*) frameHdr;

    ieHdr = rxFrame + offset;

    // The first expected mesh IE is Mesh ID
    while (ieHdr.id != DOT11_IE_MESH_ID)
    {
        offset += ieHdr.length + 2;
        ieHdr = rxFrame + offset;

        if (offset >= rxFrameSize)
        {
            return;
        }
    }

    char meshId[DOT11s_MESH_ID_LENGTH_MAX + 1] = DOT11s_MESH_ID_INVALID;
    DOT11s_PathProfile pathProfile;
    int peerCapacity = 0;
    DOT11s_AssocData assocData = {0};

    // Check for Mesh ID, if present
    if (ieHdr.id == DOT11_IE_MESH_ID)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11s_ParseMeshIdElement(node, &element, meshId);

        ieHdr = rxFrame + offset;
    }
    else
    {
        if (DOT11s_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! Mesh ID element not found");
        }
    }

    // Check if Mesh Capability is present
    if (ieHdr.id == DOT11_IE_MESH_CAPABILITY)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11s_ParseMeshCapabilityElement(node, &element,
            &pathProfile, &peerCapacity);

        ieHdr = rxFrame + offset;
    }
    else
    {
        if (DOT11s_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! Mesh Capability element not found");
        }
    }

    // Check Active Profile Announcement
    // Note: Path Protocol/Metric overwrites Mesh Capability values.
    if (ieHdr.id == DOT11_IE_ACTIVE_PROFILE_ANNOUNCEMENT)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11s_ParseActiveProfileElement(node, &element, &pathProfile);

        ieHdr = rxFrame + offset;
    }
    else
    {
        if (DOT11s_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! Active Profile Announcement element not found");
        }
    }

    // Check Peer Link Open Request
    if (ieHdr.id == DOT11_IE_PEER_LINK_OPEN)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11s_ParsePeerLinkOpenElement(node, &element, &assocData);

        ieHdr = rxFrame + offset;
    }
    else
    {
        ERROR_ReportError("Dot11s_ReceiveAssocRequestFrame: "
            "Peer Link Open element not found");
    }

    // A matching profile check is currently omitted.

    DOT11s_NeighborItem* neighborItem =
        Dot11sNeighborList_Lookup(node, dot11, sourceAddr);

    if (neighborItem == NULL)
    {
        // Assume that we did not receive the neighbor's beacon
        // though it seems to have received our beacon(s).

        if (DOT11s_TraceComments)
        {
            char addrTA[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(addrTA, &frameHdr->sourceAddr);

            char traceStr[MAX_STRING_LENGTH];
            sprintf(traceStr,
                "Association Request received from %s, "
                "though it is not in neighbor list",
                addrTA);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }

        // Add neighbor to list
        Dot11s_MallocMemset0(DOT11s_NeighborItem, neighborItem);
        ListAppend(node, mp->neighborList, 0, neighborItem);

        // Fill in neighbor values
        neighborItem->neighborAddr = sourceAddr;
        neighborItem->primaryAddr = dot11->selfAddr;
        neighborItem->isAuthenticated = TRUE;
        Dot11s_SetNeighborState(node,
            neighborItem, DOT11s_NEIGHBOR_CANDIDATE_PEER);

        neighborItem->phyModel =
            PHY_GetModel(node, dot11->myMacData->phyNumber);
        // Temporary. To be overwritten later during Link State.
        neighborItem->dataRateInMbps = Dot11s_GetDataRateInMbps(
            node, neighborItem->phyModel,
            dot11->myMacData->phyNumber,
            PHY_GetRxDataRateType(node, dot11->myMacData->phyNumber));

        Dot11sAssoc_SetState(node, dot11,
            neighborItem, DOT11s_ASSOC_S_IDLE);

        neighborItem->beaconInterval = 0;
        neighborItem->firstBeaconTime = 0;
        neighborItem->lastBeaconTime = 0;
        neighborItem->beaconsReceived = 0;
    }

    // Call association state machine for appropriate action.
    mp->assocStateData.event = DOT11s_ASSOC_S_EVENT_OPEN_RECEIVED;
    mp->assocStateData.neighborItem = neighborItem;
    mp->assocStateData.peerMeshId = meshId;
    mp->assocStateData.peerPathProtocol = pathProfile.pathProtocol;
    mp->assocStateData.peerPathMetric = pathProfile.pathMetric;
    mp->assocStateData.peerCapacity = peerCapacity;
    mp->assocStateData.peerLinkId = assocData.linkId;
    neighborItem->assocStateFn(node, dot11, mp);

    mp->stats.assocRequestsReceived++;

    *isProcessed = TRUE;
}


/**
FUNCTION   :: Dot11s_ReceiveAssocResponseFrame
LAYER      :: MAC
PURPOSE    :: Process an assoc. response if it from a mesh neighbor.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received response message
+ isProcessd : BOOL*        : will be set to TRUE is message is processed
RETURN     :: void
**/

static
void Dot11s_ReceiveAssocResponseFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    BOOL* isProcessed)
{
    DOT11s_Data* mp = dot11->mp;
    *isProcessed = FALSE;

    DOT11_FrameHdr* frameHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    ERROR_Assert(frameHdr->frameType == DOT11_ASSOC_RESP,
        "Dot11s_ReceiveAssocResponseFrame: Wrong frame type.\n");

    // Skip fixed length fields
    int offset = sizeof(DOT11_AssocRespFrame);
    int rxFrameSize = MESSAGE_ReturnPacketSize(msg);

    if (offset >= rxFrameSize)
    {
        if (DOT11s_TraceComments)
        {
            char addrTA[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(addrTA, &frameHdr->sourceAddr);
            char traceStr[MAX_STRING_LENGTH];
            sprintf(traceStr, "!! Dot11s_ReceiveAssocResponseFrame: "
                "Response from %s, without mesh elements",
                addrTA);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }

        return;
    }

    Mac802Address sourceAddr = frameHdr->sourceAddr;
    DOT11_Ie element;
    DOT11_IeHdr ieHdr;
    unsigned char* rxFrame = (unsigned char *) frameHdr;

    ieHdr = rxFrame + offset;
    // The first expected mesh IE is Mesh ID
    while (ieHdr.id != DOT11_IE_MESH_ID)
    {
        offset += ieHdr.length + 2;
        ieHdr = rxFrame + offset;
        if (offset >= rxFrameSize)
        {
            return;
        }
    }

    char meshId[DOT11s_MESH_ID_LENGTH_MAX + 1] = DOT11s_MESH_ID_INVALID;
    DOT11s_PathProfile pathProfile;
    int peerCapacity = 0;
    DOT11s_AssocData assocData = {0};

    if (ieHdr.id == DOT11_IE_MESH_ID)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11s_ParseMeshIdElement(node, &element, meshId);

        ieHdr = rxFrame + offset;
    }
    else
    {
        if (DOT11s_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! Mesh ID element not found");
        }
    }

    // Check if Mesh Capability is present
    if (ieHdr.id == DOT11_IE_MESH_CAPABILITY)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11s_ParseMeshCapabilityElement(node, &element,
            &pathProfile, &peerCapacity);

        ieHdr = rxFrame + offset;
    }
    else
    {
        if (DOT11s_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! Mesh Capability element not found");
        }
    }

    // Check if Active Profile Announcement is present
    if (ieHdr.id == DOT11_IE_ACTIVE_PROFILE_ANNOUNCEMENT)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11s_ParseActiveProfileElement(node, &element, &pathProfile);

        ieHdr = rxFrame + offset;
    }
    else
    {
        if (DOT11s_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! Active Profile Announcement element not found");
        }
    }

    // Check if Peer Confirm IE is present
    if (ieHdr.id == DOT11_IE_PEER_LINK_CONFIRM)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11s_ParsePeerLinkConfirmElement(node, &element, &assocData);

        ieHdr = rxFrame + offset;
    }
    else
    {
        ERROR_ReportError("Dot11s_ReceiveAssocResponseFrame: "
            "Peer Link Confirm element not found");
    }

    // A matching profile check is currently omitted.

    DOT11s_NeighborItem* neighborItem =
        Dot11sNeighborList_Lookup(node, dot11, sourceAddr);
    ERROR_Assert(neighborItem != NULL,
        "Dot11s_ReceiveAssocResponseFrame: "
        "Response from unknown neighbor.\n");

    mp->assocStateData.event = DOT11s_ASSOC_S_EVENT_CONFIRM_RECEIVED;
    mp->assocStateData.neighborItem = neighborItem;
    mp->assocStateData.peerMeshId = meshId;
    mp->assocStateData.peerPathProtocol = pathProfile.pathProtocol;
    mp->assocStateData.peerPathMetric = pathProfile.pathMetric;
    mp->assocStateData.peerCapacity = peerCapacity;
    mp->assocStateData.linkId = assocData.peerLinkId;
    mp->assocStateData.peerLinkId = assocData.linkId;
    neighborItem->assocStateFn(node, dot11, mp);

    mp->stats.assocResponsesReceived++;

    *isProcessed = TRUE;
}


/**
FUNCTION   :: Dot11s_ReceiveAssocCloseFrame
LAYER      :: MAC
PURPOSE    :: Process an disassociation frame if it from a mesh neighbor.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received response message
+ isProcessd : BOOL*        : will be set to TRUE is message is processed
RETURN     :: void
**/

static
void Dot11s_ReceiveAssocCloseFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    BOOL* isProcessed)
{
    DOT11s_Data* mp = dot11->mp;
    *isProcessed = FALSE;

    DOT11_FrameHdr* frameHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    ERROR_Assert(frameHdr->frameType == DOT11_DISASSOCIATION,
        "Dot11s_ReceiveAssocCloseFrame: Wrong frame type.\n");

    // Skip fixed length fields
    int offset = sizeof(DOT11_DisassocFrame);
    int rxFrameSize = MESSAGE_ReturnPacketSize(msg);

    if (offset >= rxFrameSize)
    {
        if (DOT11s_TraceComments)
        {
            char addrTA[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(addrTA, &frameHdr->sourceAddr);
            char traceStr[MAX_STRING_LENGTH];
            sprintf(traceStr, "!! Dot11s_ReceiveAssocCloseFrame: "
                "Close from %s, without mesh elements",
                addrTA);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }

        return;
    }

    Mac802Address sourceAddr = frameHdr->sourceAddr;
    DOT11_Ie element;
    DOT11_IeHdr ieHdr;
    unsigned char* rxFrame = (unsigned char *) frameHdr;

    ieHdr = rxFrame + offset;
    // The first expected mesh IE is Link Close
    while (ieHdr.id != DOT11_IE_PEER_LINK_CLOSE)
    {
        offset += ieHdr.length + 2;
        ieHdr = rxFrame + offset;
        if (offset >= rxFrameSize)
        {
            return;
        }
    }

    DOT11s_AssocData assocData;

    if (ieHdr.id == DOT11_IE_PEER_LINK_CLOSE)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11s_ParsePeerLinkCloseElement(node, &element, &assocData);

        ieHdr = rxFrame + offset;
    }
    else
    {
        if (DOT11s_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "!! Link Close element not found");
        }
        return;

    }

    DOT11s_NeighborItem* neighborItem =
        Dot11sNeighborList_Lookup(node, dot11, sourceAddr);

    if (neighborItem != NULL)
    {
        mp->assocStateData.event = DOT11s_ASSOC_S_EVENT_CLOSE_RECEIVED;
        mp->assocStateData.neighborItem = neighborItem;
        mp->assocStateData.linkId = assocData.peerLinkId;
        mp->assocStateData.peerLinkId = assocData.linkId;
        mp->assocStateData.peerStatus =
            (DOT11s_PeerLinkStatus)assocData.status;
        neighborItem->assocStateFn(node, dot11, mp);

        mp->stats.assocCloseReceived++;
    }

    *isProcessed = TRUE;
}


/**
FUNCTION   :: Dot11s_SendLinkStateFrame
LAYER      :: MAC
PURPOSE    :: Send a link state frame to neighbor with data rate and PER.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ destAddr  : Mac802Address : destination address for frame
+ neighborItem : DOT11s_NeighborItem* : mesh neighbor details
RETURN     :: void
**/

static
void Dot11s_SendLinkStateFrame(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr,
    DOT11s_NeighborItem* neighborItem)
{
    DOT11_MacFrame fullFrame;
    Dot11s_Memset0(DOT11_MacFrame, &fullFrame);
    int txFrameSize;

    // Update neighbor's current rate
    DOT11_DataRateEntry* dataRateEntry =
        MacDot11StationGetDataRateEntry(node, dot11, destAddr);
    neighborItem->dataRateInMbps =
        Dot11s_GetDataRateInMbps(node,
        neighborItem->phyModel,
        dot11->myMacData->phyNumber,
        dataRateEntry->dataRateType);

    float PER = 0;
    if (neighborItem->framesSent > 0)
    {
        PER = neighborItem->framesResent / (float) neighborItem->framesSent;
        // Change this to the same granularity as used in frames
        // else bi-directional comparison could be unequal.
        // Float precision can only affect the limiting case.
        PER = (PER / DOT11s_PER_MULTIPLE) * DOT11s_PER_MULTIPLE;
        if (PER >= 1.0f)
        {
            PER = 1.0f - DOT11s_PER_MULTIPLE;
        }
    }
    neighborItem->PER = PER;

    // Create and build frame.
    Dot11s_CreateLinkStateFrame(node, dot11,
        neighborItem, &fullFrame, &txFrameSize);

    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, msg, txFrameSize, TRACE_DOT11);
    DOT11_FrameHdr* txFrame = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    memcpy(txFrame, &fullFrame, (size_t)txFrameSize);

    // Enqueue management frame for transmit.
    DOT11s_FrameInfo* newFrameInfo;
    Dot11s_MallocMemset0(DOT11s_FrameInfo, newFrameInfo);
    newFrameInfo->msg = msg;
    newFrameInfo->frameType = DOT11_ACTION;
    newFrameInfo->RA = destAddr;
    newFrameInfo->TA = dot11->selfAddr;
    newFrameInfo->SA = INVALID_802ADDRESS;
    newFrameInfo->actionData.category = DOT11_ACTION_MESH;
    newFrameInfo->actionData.fieldId = DOT11_MESH_LINK_STATE_ANNOUNCEMENT;

    Dot11s_SetFieldsInMgmtFrameHdr(node, dot11, txFrame, newFrameInfo);
    Dot11sMgmtQueue_EnqueuePacket(node, dot11, newFrameInfo);

}


/**
FUNCTION   :: Dot11s_ReceiveLinkStateFrame
LAYER      :: MAC
PURPOSE    :: Receive link state frame from a superordinate mesh neighbor.
                This frame, received periodically, is used to update the
                bidirectional values of rate and PER.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received message
+ dropFrame : BOOL*         : TRUE if message is ignored
RETURN     :: void
**/

static
void Dot11s_ReceiveLinkStateFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    BOOL* dropFrame)
{
    DOT11s_Data* mp = dot11->mp;

    *dropFrame = FALSE;

    DOT11_FrameHdr* frameHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    ERROR_Assert(frameHdr->frameType == DOT11_ACTION,
        "Dot11s_ReceiveLinkStateFrame: Wrong frame type.\n");

    Mac802Address sourceAddr = frameHdr->sourceAddr;
    DOT11_Ie element;
    DOT11_IeHdr ieHdr;
    int offset;
    int rxFrameSize = MESSAGE_ReturnPacketSize(msg);
    unsigned char* rxFrame = (unsigned char *) frameHdr;

    offset = sizeof(DOT11_FrameHdr);
    unsigned char rxFrameCategory = *(rxFrame + offset);
    ERROR_Assert(
        rxFrameCategory == DOT11_ACTION_MESH,
        "Dot11s_ReceiveLinkStateFrame: Not a mesh category frame.\n");
    offset++;

    unsigned char rxFrameActionFieldId = *(rxFrame + offset);
    ERROR_Assert(
        rxFrameActionFieldId == DOT11_MESH_LINK_STATE_ANNOUNCEMENT,
        "Dot11s_ReceiveLinkStateFrame: Invalid Action field ID.\n");
    offset++;

    ieHdr = rxFrame + offset;
    ERROR_Assert(
        ieHdr.id == DOT11_IE_LINK_STATE_ANNOUNCEMENT,
        "Dot11s_ReceiveLinkStateFrame: Invalid IE ID.\n");

    int r;
    float e;
    Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
    Dot11s_ParseLinkStateAnnouncementElement(node, &element, &r, &e);

    DOT11s_NeighborItem* neighborItem =
        Dot11sNeighborList_Lookup(node, dot11, sourceAddr);

    if (neighborItem == NULL)
    {
        *dropFrame = TRUE;
        return;
    }

    if (neighborItem->state == DOT11s_NEIGHBOR_SUPERORDINATE_LINK_DOWN)
    {
        Dot11s_SetNeighborState(node,
            neighborItem, DOT11s_NEIGHBOR_SUPERORDINATE_LINK_UP);
    }

    if (neighborItem->state != DOT11s_NEIGHBOR_SUPERORDINATE_LINK_UP)
    {
        MacDot11Trace(node, dot11, NULL,
           "!! Received Link State from subordinate neighbor.");

        *dropFrame = TRUE;
        return;
    }

    neighborItem->dataRateInMbps = r;
    neighborItem->PER = e;
    neighborItem->lastLinkStateTime = node->getNodeTime();

    mp->stats.linkStateReceived++;

    if (mp->activeProtocol.isInitialized)
    {
        unsigned int metric =
            Dot11s_ComputeLinkMetric(node, dot11,
            neighborItem->neighborAddr);
        mp->activeProtocol.linkUpdateFunction(node, dot11,
            neighborItem->neighborAddr,
            metric,
            DOT11s_ASSOC_ACTIVE_TIMEOUT,
            dot11->myMacData->interfaceIndex);
    }
}


/**
FUNCTION   :: Dot11s_GetNextPortalSeqNo
LAYER      :: MAC
PURPOSE    :: Get the next sequence number for PANN
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data
+ mp        : DOT11s_Data   : pointer to DOT11s data
RETURN     :: int           : next sequence number
**/

static
int Dot11s_GetNextPortalSeqNo(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp)
{
    mp->portalSeqNo++;
    return mp->portalSeqNo;
}

/**
FUNCTION   :: Dot11s_SendPannFrame
LAYER      :: MAC
PURPOSE    :: Send a portal announcement frame
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mp        : DOT11s_Data   : pointer to DOT11s data
+ pannData  : DOT11s_PannData* : portal data to used in sent frame
RETURN     :: void
**/

static
void Dot11s_SendPannFrame(
    Node* node,
    MacDataDot11* dot11,
    const DOT11s_PannData* const pannData)
{
    DOT11_MacFrame fullFrame;
    Dot11s_Memset0(DOT11_MacFrame, &fullFrame);
    int txFrameSize;

    // Create and build frame.
    Dot11s_CreatePannFrame(node, dot11, pannData, &fullFrame, &txFrameSize);

    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, msg, txFrameSize, TRACE_DOT11);
    DOT11_FrameHdr* txFrame = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    memcpy(txFrame, &fullFrame, (size_t)txFrameSize);

    DOT11s_FrameInfo* newFrameInfo;
    Dot11s_MallocMemset0(DOT11s_FrameInfo, newFrameInfo);
    newFrameInfo->msg = msg;
    newFrameInfo->frameType = DOT11_ACTION;
    newFrameInfo->RA = ANY_MAC802;
    newFrameInfo->TA = dot11->selfAddr;
    newFrameInfo->SA = INVALID_802ADDRESS;
    newFrameInfo->actionData.category = DOT11_ACTION_MESH;
    newFrameInfo->actionData.fieldId = DOT11_MESH_PANN;

    Dot11s_SetFieldsInMgmtFrameHdr(node, dot11, txFrame, newFrameInfo);
    Dot11sMgmtQueue_EnqueuePacket(node, dot11, newFrameInfo);
}


/**
FUNCTION   :: Dot11s_ReceivePannFrame
LAYER      :: MAC
PURPOSE    :: Receive portal announcement.
                This frame, received periodically, is retransmitted
                subject to hop count / TTL.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received message
+ dropFrame : BOOL*        : will be set to FALSE if message is processed
RETURN     :: void
**/

static
void Dot11s_ReceivePannFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    BOOL* dropFrame)
{
    DOT11s_Data* mp = dot11->mp;
    *dropFrame = TRUE;

    DOT11_FrameHdr* frameHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    ERROR_Assert(frameHdr->frameType == DOT11_ACTION,
        "Dot11s_ReceivePannFrame: Wrong frame type.\n");

    Mac802Address TA = frameHdr->sourceAddr;

    DOT11_Ie element;
    DOT11_IeHdr ieHdr;
    int offset;
    int rxFrameSize = MESSAGE_ReturnPacketSize(msg);
    unsigned char* rxFrame = (unsigned char *) frameHdr;

    offset = sizeof(DOT11_FrameHdr);
    unsigned char rxFrameCategory = *(rxFrame + offset);
    ERROR_Assert(
        rxFrameCategory == DOT11_ACTION_MESH,
        "Dot11s_ReceivePannFrame: Not a mesh category frame.\n");
    offset++;

    unsigned char rxFrameActionFieldId = *(rxFrame + offset);
    ERROR_Assert(
        rxFrameActionFieldId == DOT11_MESH_PANN,
        "Dot11s_ReceivePannFrame: Invalid Action field ID.\n");
    offset++;

    ieHdr = rxFrame + offset;
    ERROR_Assert(
        ieHdr.id == DOT11_IE_PANN,
        "Dot11s_ReceivePannFrame: Invalid IE ID.\n");

    DOT11s_PannData pannData;
    Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
    Dot11s_ParsePannElement(node, &element, &pannData);

    // Drop if own frame
    if (pannData.portalAddr == dot11->selfAddr)
    {
        mp->stats.pannDropped++;
        return;
    }

    DOT11s_PortalItem* portalItem =
        Dot11sPortalList_Lookup(node, dot11, pannData.portalAddr);

    // Add new portal if not seen earlier.
    if (portalItem == NULL)
    {
        Dot11s_MallocMemset0(DOT11s_PortalItem, portalItem);
        ListAppend(node, mp->portalList, 0, portalItem);

        portalItem->portalAddr = pannData.portalAddr;

        if (DOT11s_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL, "Add new portal");
        }
    }

    if (pannData.hopCount > DOT11s_TTL_MAX)
    {
        mp->stats.pannDropped++;
        return;
    }

    pannData.metric += Dot11s_ComputeLinkMetric(node, dot11, TA);
    pannData.hopCount++;
    pannData.ttl--;

    // Discard if old or does not have a better metric.
    if ((pannData.portalSeqNo <= portalItem->lastPannData.portalSeqNo)
        || (pannData.portalSeqNo == portalItem->lastPannData.portalSeqNo
            && pannData.metric >= portalItem->lastPannData.metric))
    {
        mp->stats.pannDropped++;
        return;
    }

    // Record portal announcement
    portalItem->isActive = TRUE;
    portalItem->lastPannTime = node->getNodeTime();
    portalItem->lastPannData = pannData;
    portalItem->nextHopAddr = TA;

    mp->stats.pannReceived++;

    if (pannData.ttl >= 1)
    {
        // Set timer for retransmit of PANN
        clocktype delay =
            RANDOM_nrand(dot11->seed)
            % (mp->pannPeriod / DOT11s_TTL_MAX)
            + DOT11s_PANN_PROPAGATION_DELAY;

        DOT11s_TimerInfo timerInfo(pannData.portalAddr);
        Dot11s_StartTimer(node, dot11, &timerInfo, delay,
            MSG_MAC_DOT11s_PannPropagationTimeout);
    }
    else
    {
        mp->stats.pannDropped++;
        *dropFrame = TRUE;
        return;
    }

    *dropFrame = FALSE;
}


/**
FUNCTION   :: Dot11s_ReceiveMgmtBroadcast
LAYER      :: MAC
PURPOSE    :: Receive a broadcast frame of type management.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received message
RETURN     :: void
**/

BOOL Dot11s_ReceiveMgmtBroadcast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11s_Data* mp = dot11->mp;
    BOOL msgIsProcessed = FALSE;
    BOOL dropFrame = FALSE;

    if (! MacDot11IsManagementFrame(msg))
    {
        ERROR_ReportError(
            "Dot11s_ReceiveMgmtBroadcast: Not a managment frame.\n");
    }

    DOT11_FrameHdr* frameHdr =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    Mac802Address sourceAddr = frameHdr->sourceAddr;

    MacDot11Trace(node, dot11, msg, "Receive");

    switch (frameHdr->frameType)
    {
        case DOT11_ACTION:
        {
            unsigned char* rxFrame = (unsigned char *) frameHdr;
            int offset = sizeof(DOT11_FrameHdr);

            unsigned char rxFrameCategory = *(rxFrame + offset);
            if (rxFrameCategory != DOT11_ACTION_MESH)
            {
                break;
            }

            // Mesh Action frames are not handled elsewhere.
            msgIsProcessed = TRUE;

            offset++;
            unsigned char rxFrameActionFieldId = *(rxFrame + offset);

            switch (rxFrameActionFieldId)
            {
                case DOT11_MESH_RREQ:
                case DOT11_MESH_RERR:
                case DOT11_MESH_RANN:
                {
                    if (mp->state < DOT11s_S_PATH_SELECTION)
                    {
                        dropFrame = TRUE;
                        break;
                    }

                    if (Dot11s_IsAssociatedNeighbor(node, dot11, sourceAddr)
                        == FALSE)
                    {
                        if (DOT11s_TraceComments)
                        {
                            MacDot11Trace(node, dot11, NULL,
                                "Dot11s_ReceiveMgmtBroadcast: "
                                "DROP frame from neighbor not in UP state");
                        }

                        dropFrame = TRUE;
                        break;
                    }

                    if (dot11->mp->activeProtocol.isInitialized == FALSE)
                    {
                        dropFrame = TRUE;
                        break;
                    }

                    Hwmp_ReceiveControlFrame(node, dot11,
                        msg, dot11->myMacData->interfaceIndex);

                    break;
                }

                case DOT11_MESH_PANN:
                {
                    if (mp->state < DOT11s_S_PATH_SELECTION)
                    {
                        dropFrame = TRUE;
                        break;
                    }

                    if (Dot11s_IsAssociatedNeighbor(node, dot11, sourceAddr)
                        == FALSE)
                    {
                        if (DOT11s_TraceComments)
                        {
                            MacDot11Trace(node, dot11, NULL,
                                "Dot11s_ReceiveMgmtBroadcast: "
                                "DROP frame from neighbor not in UP state");
                        }

                        dropFrame = TRUE;
                        break;
                    }

                    Dot11s_ReceivePannFrame(node, dot11, msg, &dropFrame);

                    if (dropFrame == TRUE)
                    {
                        if (DOT11s_TraceComments)
                        {
                            MacDot11Trace(node, dot11, NULL,
                                "Dot11s_ReceiveMgmtBroadcast: "
                                "DROPPING -- PANN frame ");
                        }
#ifdef ADDON_DB
                        HandleMacDBEvents(        
                            node, msg, 
                            node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                            dot11->myMacData->interfaceIndex, MAC_Drop, 
                            node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif
                    }

                    break;
                }
                default:
                {
                    char errorStr[MAX_STRING_LENGTH];
                    sprintf(errorStr, "Dot11s_ReceiveMgmtBroadcast: "
                        "Unexpected Mesh action, field ID=%d.\n",
                        rxFrameActionFieldId);
                    ERROR_ReportError(errorStr);

                    break;
                }
            }

            break;
        }
        default:
        {
            // Ignore non-mesh action frames.
            break;
        }
    }

    if (msgIsProcessed)
    {
        if (dot11->useDvcs)
        {
            BOOL nodeIsInCache;
            MacDot11StationUpdateDirectionCacheWithCurrentSignal(
                node, dot11, sourceAddr, &nodeIsInCache);
        }

        if (dropFrame)
        {
            mp->stats.mgmtBcDropped++;
        }

        MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
    }

    return msgIsProcessed;
}


/**
FUNCTION   :: Dot11s_ReceiveMgmtUnicast
LAYER      :: MAC
PURPOSE    :: Process a unicast management type frame if it contains
                mesh elements i.e. is from a mesh neighbor.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received message
RETURN     :: BOOL          : TRUE if frame is a mesh frame and need
                                not be further processed by other Dot11
                                code; FALSE if not a mesh frame.
**/

BOOL Dot11s_ReceiveMgmtUnicast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11s_Data* mp = dot11->mp;

    if (! MacDot11IsManagementFrame(msg))
    {
        ERROR_ReportError(
            "Dot11s_ReceiveMgmtUnicast: Not a managment frame.\n");
    }

    DOT11_FrameHdr* frameHdr =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    Mac802Address sourceAddr = frameHdr->sourceAddr;

    BOOL dropFrame = FALSE;
    int rxFrameSize = MESSAGE_ReturnPacketSize(msg);
    int offset;

    switch (frameHdr->frameType)
    {
        case DOT11_ASSOC_REQ:
        {
            offset = sizeof(DOT11_AssocReqFrame);
            dropFrame = (offset == rxFrameSize);
            break;
        }
        case DOT11_ASSOC_RESP:
        {
            offset = sizeof(DOT11_AssocRespFrame);
            dropFrame = (offset == rxFrameSize);
            break;
        }
        case DOT11_REASSOC_REQ:
        {
            offset = sizeof(DOT11_ReassocReqFrame);
            dropFrame = (offset == rxFrameSize);
            break;
        }
        case DOT11_REASSOC_RESP:
        {
            offset = sizeof(DOT11_AssocRespFrame);
            dropFrame = (offset == rxFrameSize);
            break;
        }
        case DOT11_DISASSOCIATION:
        {
            offset = sizeof(DOT11_DisassocFrame);
            dropFrame = (offset == rxFrameSize);
            break;
        }
        case DOT11_AUTHENTICATION:
        {
            // Currently MPs do not use authentication
            dropFrame = TRUE;
            break;
        }
        default:
        {
            dropFrame = FALSE;
            break;
        }
    }

    // Initial checks rule out mesh elements.
    if (dropFrame == TRUE)
    {
        return FALSE;
    }

    DOT11_SeqNoEntry *entry;

    if (dot11->useDvcs)
    {
        BOOL nodeIsInCache;
        MacDot11StationUpdateDirectionCacheWithCurrentSignal(
            node, dot11, sourceAddr, &nodeIsInCache);
    }

    // Reuse the transmit cycle of data and also the matching
    // response state as the transmit cycle is tied tightly
    // with data frames.
    if ((dot11->state != DOT11_S_WFDATA) &&
        (MacDot11IsWaitingForResponseState(dot11->state)))
    {
        // Waiting for another packet type. Ignore this one.
        MacDot11Trace(node, dot11, NULL,
            "Dot11s_ReceiveMgmtUnicast: Ignoring frame");

        // Other code processes the message.
        return FALSE;
    }

    MacDot11Trace(node, dot11, msg, "Receive");

/*See how to enable GUI
    if (node->guiOption == TRUE)
    {
        Mac802Address sourceNodeId;
        int sourceInterfaceIndex;

        if (NetworkIpGetInterfaceType(
            node, dot11->myMacData->interfaceIndex)
                == NETWORK_IPV6)
        {
            sourceNodeId =
                MAPPING_GetNodeIdFromLinkLayerAddr(
                node, sourceAddr);
            sourceInterfaceIndex =
                MAPPING_GetInterfaceFromLinkLayerAddress(
                node, sourceAddr);
        }
        else
        {
            sourceNodeId =
                MAPPING_GetNodeIdFromInterfaceAddress(
                node, sourceAddr),
            sourceInterfaceIndex =
                MAPPING_GetInterfaceIndexFromInterfaceAddress(
                node, sourceAddr);
        }

        GUI_Unicast(
            sourceNodeId,
            node->nodeId,
            GUI_MAC_LAYER,
            GUI_DEFAULT_DATA_TYPE,
            sourceInterfaceIndex,
            dot11->myMacData->interfaceIndex,
            node->getNodeTime());
    }
*/
    MacDot11StationCancelTimer(node, dot11);
    MacDot11StationTransmitAck(node, dot11, sourceAddr);

    if (! MacDot11StationCorrectSeqenceNumber(node, dot11,
            sourceAddr, frameHdr->seqNo))
    {
        // Wrong sequence number. Drop.
        MacDot11Trace(node, dot11, NULL,
            "Dot11s_ReceiveMgmtUnicast: Drop, wrong sequence number");
#ifdef ADDON_DB
        HandleMacDBEvents(        
            node, msg, 
            node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
            dot11->myMacData->interfaceIndex, MAC_Drop, 
            node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif
        // Treat as processed.
        return TRUE;
    }


    BOOL isProcessed = FALSE;

    switch (frameHdr->frameType)
    {
        case DOT11_ASSOC_REQ:
        {
            // Process completely if it contains mesh IEs
            Dot11s_ReceiveAssocRequestFrame(node, dot11,
                msg, &isProcessed);

            break;
        }
        case DOT11_ASSOC_RESP:
        {
            // Process completely if it contains mesh IEs
            Dot11s_ReceiveAssocResponseFrame(node, dot11,
                msg, &isProcessed);

            break;
        }
        case DOT11_DISASSOCIATION:
        {
            // Process completely if it contains mesh IEs
            Dot11s_ReceiveAssocCloseFrame(node, dot11,
                msg, &isProcessed);

            break;
        }
        case DOT11_AUTHENTICATION:
        {
            // Currently MPs do not use authentication
            // Should not fall through here.
            ERROR_ReportError("Dot11s_ReceiveMgmtUnicast: "
                "Authentication not implemented.\n");

            break;
        }
        case DOT11_ACTION:
        {
            // Mesh Action frames are not handled elsewhere.
            isProcessed = TRUE;

            unsigned char* rxFrame = (unsigned char *) frameHdr;
            offset = sizeof(DOT11_FrameHdr);

            unsigned char rxFrameCategory = *(rxFrame + offset);

            if (rxFrameCategory != DOT11_ACTION_MESH)
            {
                break;
            }

            offset++;
            unsigned char rxFrameActionFieldId = *(rxFrame + offset);

            switch (rxFrameActionFieldId)
            {
                case DOT11_MESH_LINK_STATE_ANNOUNCEMENT:
                {
                    Dot11s_ReceiveLinkStateFrame(
                        node, dot11, msg, &dropFrame);

                    if (dropFrame)
                    {
                        mp->stats.mgmtUcDropped++;
                    }

                    break;
                }
                case DOT11_MESH_PEER_LINK_DISCONNECT:
                {
                    // Not implemented
                    ERROR_ReportError("Dot11s_ReceiveMgmtUnicast: "
                        "Unknown message event type.\n");

                    break;
                }
                case DOT11_MESH_RREQ:
                case DOT11_MESH_RREP:
                case DOT11_MESH_RERR:
                {
                    if (mp->activeProtocol.isInitialized
                        == FALSE)
                    {
                        mp->stats.mgmtUcDropped++;
                        dropFrame = TRUE;
                        break;
                    }

                    if (Dot11s_IsAssociatedNeighbor(node, dot11, sourceAddr)
                        == FALSE)
                    {
                        if (DOT11s_TraceComments)
                        {
                            MacDot11Trace(node, dot11, NULL,
                                "Dot11s_ReceiveMgmtUnicast: "
                                "DROP frame from neighbor not in UP state");
                        }

                        mp->stats.mgmtUcDropped++;
                        dropFrame = TRUE;
                        break;
                    }

                    Hwmp_ReceiveControlFrame(node, dot11,
                        msg, dot11->myMacData->interfaceIndex);

                    break;
                }
                default:
                {
                    ERROR_ReportError("Dot11s_ReceiveMgmtUnicast: "
                        "Unknown action frame type.\n");

                    break;
                }
            }

            break;
        }
        default:
        {
            ERROR_ReportError(
                "Dot11s_ReceiveMgmtUnicast: Unexpected frame type.\n");

            break;
        }
    }


    if (isProcessed)
    {
        // Update sequence number
        entry = MacDot11StationGetSeqNo(node, dot11, sourceAddr);
        ERROR_Assert(entry,
            "Dot11s_ReceiveMgmtUnicast: "
            "Unable to create or find sequence number entry.\n");
        entry->fromSeqNo += 1;
    }
    else
    {
        // Implies some code assumption is wrong.
        // In general, adding other capabilities would
        // require coding such that some IEs are read
        // and the frame is passed elsewhere for further
        // processing of other IEs.
        ERROR_ReportError("Dot11s_ReceiveMgmtUnicast: "
            "Directed frame not processed !!\n");
    }

    return isProcessed;
}


/**
FUNCTION   :: Dot11s_PacketRetransmitEvent
LAYER      :: MAC
PURPOSE    :: Handle notification of a retransmit from the "transmit fn".
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo* : info of frame retransmitted
RETURN     :: void
NOTES      :: If the retransmit is to a mesh neighbor, update values used
                to capture/measure PER.
**/

void Dot11s_PacketRetransmitEvent(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameInfo)
{
    ERROR_Assert(frameInfo != NULL,
        "Dot11s_PacketRetransmitEvent: Frame Info is NULL.\n");

    DOT11s_Data* mp = dot11->mp;

    if (mp->state < DOT11s_S_INIT_COMPLETE)
    {
        return;
    }

    DOT11s_NeighborItem* neighborItem =
        Dot11sNeighborList_Lookup(node, dot11, frameInfo->RA);
    if (neighborItem != NULL)
    {
        if (neighborItem->state > DOT11s_NEIGHBOR_ASSOC_PENDING)
        {
            // Update retransmit count to measure PER
            neighborItem->framesResent += DOT11s_FRAME_RETRANSMIT_PENALTY;
        }
    }
}


/**
FUNCTION   :: Dot11s_PacketDropEvent
LAYER      :: MAC
PURPOSE    :: Handle notification of a drop event from the "transmit fn".
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo* : info of frame dropped
RETURN     :: void
NOTES      :: If the drop is to a mesh neighbor, updates values used
                to capture/measure PER.
**/

void Dot11s_PacketDropEvent(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameInfo)
{
    DOT11s_Data* mp = dot11->mp;

    ERROR_Assert(frameInfo != NULL,
        "Dot11s_PacketDropEvent: Frame info is NULL.\n");

    if (mp->state < DOT11s_S_INIT_COMPLETE)
    {
        return;
    }

    // Check if RA is a neighbor & update PER related measure.
    DOT11s_NeighborItem* neighborItem =
        Dot11sNeighborList_Lookup(node, dot11, frameInfo->RA);

    if (neighborItem != NULL)
    {
        if (Dot11s_IsAssociatedNeighbor(node, dot11, frameInfo->RA))
        {
            neighborItem->framesResent += DOT11s_FRAME_DROP_PENALTY;

            // Notify active protocol for link failure
            mp->activeProtocol.linkFailureFunction(node, dot11,
                neighborItem->neighborAddr, frameInfo->DA,
                dot11->myMacData->interfaceIndex);
        }

        return;
    }

    // If the transmit is to a BSS station, the active protocol
    // also needs to be notified. This awaits tying up with AP code.
}


/**
FUNCTION   :: Dot11s_PacketAckEvent
LAYER      :: MAC
PURPOSE    :: Handle an Ack Event from the transmit function.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11s_FrameInfo* : info of frame retransmitted
RETURN     :: void
**/

void Dot11s_PacketAckEvent(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_FrameInfo* frameInfo)
{
    ERROR_Assert(frameInfo != NULL,
        "Dot11s_PacketAckEvent: Tx Frame Info is NULL.\n");

    DOT11s_Data* mp = dot11->mp;

    // Check if RA is a neighbor
    DOT11s_NeighborItem* neighborItem
        = Dot11sNeighborList_Lookup(node, dot11, frameInfo->RA);
    if (Dot11s_IsAssociatedNeighbor(node, dot11, frameInfo->RA))
    {
        neighborItem->framesSent++;
    }

    switch (frameInfo->frameType)
    {
        case DOT11_ACTION:
        {
            if (frameInfo->actionData.fieldId
                == DOT11_MESH_LINK_STATE_ANNOUNCEMENT)
            {
                if (neighborItem->state ==
                    DOT11s_NEIGHBOR_SUBORDINATE_LINK_DOWN)
                {
                    Dot11s_SetNeighborState(node, neighborItem,
                        DOT11s_NEIGHBOR_SUBORDINATE_LINK_UP);
                }

                neighborItem->lastLinkStateTime = node->getNodeTime();
                mp->stats.linkStateSent++;

                // Notify active protocol that neighbor is alive.
                // Effectively serves as a hello packet.
                if (mp->activeProtocol.isInitialized)
                {
                    unsigned int metric =
                        Dot11s_ComputeLinkMetric(node, dot11,
                        neighborItem->neighborAddr);
                    mp->activeProtocol.linkUpdateFunction(node, dot11,
                        neighborItem->neighborAddr,
                        metric,
                        DOT11s_ASSOC_ACTIVE_TIMEOUT,
                        dot11->myMacData->interfaceIndex);
                }
            }

            break;
        }
        default:
        {
            break;
        }
    }
}


/**
FUNCTION   :: Dot11s_StationAssociateEvent
LAYER      :: MAC
PURPOSE    :: Handle an station associate event.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ staAddr : NodeAddress : address of station that has associated
+ previousAp  : NodeAddress : address of previous AP or INVALID_ADDRESS
RETURN     :: void
**/

void Dot11s_StationAssociateEvent(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr,
    Mac802Address previousAp)
{
    if (dot11->isMAP == FALSE)
    {
        return;
    }

    DOT11s_Data* mp = dot11->mp;

    Dot11sStationList_Insert(node, dot11, staAddr, previousAp);

    if (mp->state < DOT11s_S_INIT_COMPLETE)
    {
        return;
    }

    // Notify active protocol
    mp->activeProtocol.stationAssociationFunction(node, dot11,
        staAddr, previousAp, dot11->myMacData->interfaceIndex);
}


/**
FUNCTION   :: Dot11s_StationDisssociateEvent
LAYER      :: MAC
PURPOSE    :: Handle an station disassociation event.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ staAddr   : Mac802Address : address of station that has associated
RETURN     :: void
**/

void Dot11s_StationDisassociateEvent(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address staAddr)
{
    if (dot11->isMAP == FALSE)
    {
        return;
    }

    DOT11s_Data* mp = dot11->mp;

    DOT11s_StationItem* stationItem =
        Dot11sStationList_Lookup(node, dot11, staAddr);

    if (stationItem == NULL)
    {
        return;
    }

    Dot11sStationList_Delete(node, dot11, staAddr);

    if (mp->state < DOT11s_S_INIT_COMPLETE)
    {
        return;
    }

    // Notify active protocol
    mp->activeProtocol.stationDisassociationFunction(node, dot11,
        staAddr, dot11->myMacData->interfaceIndex);
}

/**
FUNCTION   :: Dot11sMpp_ReceiveDataUnicast
LAYER      :: MAC
PURPOSE    :: MPP receives a data unicast.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received message
RETURN     :: void
NOTES      :: Since layer 3 internetworking assumed, does not forward
                to other portals.
**/

static
void Dot11sMpp_ReceiveDataUnicast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11s_Data* mp = dot11->mp;
    DOT11_FrameHdr* fHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);

    //DOT11_ApStationListItem* apStationListItem = NULL;
    DOT11s_StationItem* stationItem = NULL;

    DOT11s_FrameInfo rxFrameInfo;
    Message* newMsg;

    rxFrameInfo.frameType = fHdr->frameType;
    rxFrameInfo.RA = fHdr->destAddr;
    rxFrameInfo.TA = fHdr->sourceAddr;
    rxFrameInfo.DA = fHdr->address3;

    if (fHdr->frameType == DOT11_DATA)          // later DOT11_QOS_DATA
    {
        if (dot11->isMAP == FALSE)
        {
            ERROR_ReportError("Dot11sMpp_ReceiveDataUnicast: "
                "non-MAP should not receive DOT11_DATA frame.\n");
        }

        // Ensure that this frame is from an associated station

        //apStationListItem =
        //    MacDot11ApStationListGetItemWithGivenAddress(
        //        node, dot11, rxFrameInfo.TA);
        //if (apStationListItem == NULL)
        //{
        //    ERROR_ReportError("Dot11sMpp_ReceiveDataUnicast: "
        //        "DOT11_DATA frame from a non-associated station.\n");
        //}
        stationItem =
            Dot11sStationList_Lookup(node, dot11, rxFrameInfo.TA);
        if (stationItem == NULL)
        {
            ERROR_ReportError("Dot11sMpp_ReceiveDataUnicast: "
                "DOT11_DATA frame from a non-associated station.\n");
        }

        rxFrameInfo.SA = fHdr->sourceAddr;
        rxFrameInfo.fwdControl = 0;

        MESSAGE_RemoveHeader(node,
            msg,
            sizeof(DOT11_FrameHdr),
            TRACE_DOT11);
    }
    else if (rxFrameInfo.frameType == DOT11_MESH_DATA)
    {
        // Ensure that this frame is from a mesh neighbor
        if (Dot11s_IsAssociatedNeighbor(node, dot11, rxFrameInfo.TA)
            == FALSE)
        {
            if (DOT11s_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "!! Dot11sMpp_ReceiveDataUnicast: "
                    "DOT11_MESH_DATA frame from non-associated MP");
            }

            mp->stats.dataUcDropped++;
#ifdef ADDON_DB
                HandleMacDBEvents(        
                node, msg, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

            MESSAGE_Free(node, msg);
            return;
        }

        DOT11s_FrameHdr meshHdr;
        Dot11s_ReturnMeshHeader(&meshHdr, msg);

        rxFrameInfo.SA = meshHdr.address4;
        rxFrameInfo.fwdControl = meshHdr.fwdControl;

        MESSAGE_RemoveHeader(node,
            msg,
            DOT11s_DATA_FRAME_HDR_SIZE,
            TRACE_DOT11);

    }
    else
    {
        ERROR_ReportError("Dot11sMpp_ReceiveDataUnicast: "
            "Received unknown frame type.\n");
    }

    switch (rxFrameInfo.frameType)
    {
        case DOT11_DATA:
        //case DOT11_QOS_DATA:
        {
            // Check final destination address
            if (rxFrameInfo.DA == ANY_MAC802)
            {
                mp->stats.dataBcReceivedFromBssAsUc++;

                // ........................................
                //Hand off one copy to the upper layer

                newMsg = MESSAGE_Duplicate(node, msg);
                Dot11s_SendFrameToUpperLayer(node, dot11,
                    &rxFrameInfo, newMsg);

                mp->stats.dataBcToNetworkLayer++;

                // Need to rebroadcast it both within BSS and to mesh
                BOOL sendToMesh = FALSE;
                BOOL sendToBss = FALSE;

                if (Dot11s_GetAssociatedNeighborCount(node, dot11) > 0)
                {
                    sendToMesh = TRUE;
                }
                if (Dot11s_GetAssociatedStationCount(node, dot11) > 1)
                {
                    sendToBss = TRUE;
                }

                newMsg = msg;
                if (sendToMesh && sendToBss)
                {
                    newMsg = MESSAGE_Duplicate(node, msg);
                }
                if (!sendToMesh && !sendToBss)
                {
                    mp->stats.dataBcDropped++;

                    MESSAGE_Free(node, msg);
                    return;
                }

                if (sendToMesh)
                {
                    // ........................................
                    // Enqueue a 4 address frame

                    DOT11s_FrameInfo frameInfo = rxFrameInfo;
                    frameInfo.RA = ANY_MAC802;
                    frameInfo.TA = dot11->selfAddr;
                    frameInfo.DA = ANY_MAC802;
                    frameInfo.SA = rxFrameInfo.SA;
                    // For MAPs, use own E2E Sequence Number
                    frameInfo.fwdControl.SetE2eSeqNo(
                        Dot11sE2eList_NextSeqNo(
                        node, dot11, frameInfo.DA, dot11->selfAddr));
                    // Use max TTL
                    frameInfo.fwdControl.SetTTL(DOT11s_TTL_MAX);

                    Dot11s_SendDataFrameToMesh(node, dot11, &frameInfo, newMsg);

                    mp->stats.dataBcSentToMesh++;
                }

                if (sendToBss)
                {
                    // ........................................
                    // Enqueue a 3 address frame for transmit within BSS
                    DOT11s_FrameInfo frameInfo = rxFrameInfo;
                    // No change to frame type
                    frameInfo.RA = rxFrameInfo.DA;
                    frameInfo.TA = dot11->selfAddr;
                    // No change to DA
                    frameInfo.SA = rxFrameInfo.TA;
                    // fwdControl is unimportant
                    frameInfo.fwdControl = 0;

                    Dot11s_SendFrameToBss(node, dot11, &frameInfo, msg);

                    mp->stats.dataBcSentToBss++;
                }
            }
            else    // Not a broadcast address
            {
                mp->stats.dataUcReceivedFromBss++;

                // ........................................
                // Check if destination is self
                if (rxFrameInfo.DA == dot11->selfAddr)
                {
                    Dot11s_SendFrameToUpperLayer(node, dot11,
                        &rxFrameInfo, msg);

                    mp->stats.dataUcToNetworkLayer++;
                    // Update to DOT11 stats?
                }
                else
                {
                    // Check if destination is within BSS for an MAP
                    if (dot11->isMAP
                        && Dot11s_IsSelfOrBssStationAddr(node, dot11,
                            rxFrameInfo.DA))
                    {
                        // Enqueue 3 address frame for station.
                        DOT11s_FrameInfo frameInfo = rxFrameInfo;
                        // No change to frame type
                        frameInfo.RA = rxFrameInfo.DA;
                        frameInfo.TA = dot11->selfAddr;
                        // No change to DA
                        // No change to SA
                        // newFrameInfo->fwdControl = 0;

                        Dot11s_SendFrameToBss(node, dot11, &frameInfo, msg);

                        mp->stats.dataUcRelayedToBssFromBss++;
                        mp->stats.dataUcSentToBss++;
                    }
                    else
                    {
                        // ................................
                        // Destination is outside BSS

                        // Check if it is in the proxy list.

                        DOT11s_FwdItem* fwdItem = NULL;
                        DOT11s_ProxyItem* proxyItem = NULL;
                        BOOL inMesh = FALSE;

                        proxyItem =
                            Dot11sProxyList_Lookup(
                            node, dot11, rxFrameInfo.DA);
                        if (proxyItem != NULL)
                        {
                            inMesh = proxyItem->inMesh;
                            fwdItem =
                                Dot11sFwdList_Lookup(
                                node, dot11, proxyItem->proxyAddr);
                            if (fwdItem == NULL)
                            {
                                ERROR_ReportError(
                                    "Dot11sMpp_ReceiveDataUnicast: "
                                    "Proxy Item does not have a "
                                    "corresponding Fwd Item.\n");
                            }
                        }

                        if (inMesh || proxyItem == NULL)
                        {
                            // Setup and pass to router function
                            DOT11s_FrameInfo frameInfo = rxFrameInfo;
                            frameInfo.TA = dot11->selfAddr;
                            frameInfo.SA = rxFrameInfo.TA;
                            // Use MAP's own E2E Sequence Number
                            frameInfo.fwdControl.SetE2eSeqNo(
                                Dot11sE2eList_NextSeqNo(
                                node, dot11, frameInfo.DA, dot11->selfAddr));
                            // Use max TTL
                            frameInfo.fwdControl.SetTTL(DOT11s_TTL_MAX);

                            Dot11s_SendFrameToRoutingFunction(node, dot11, mp,
                                &frameInfo, msg);
                            // frameInfo->fwdControl.SetExMesh(FALSE);

                            mp->stats.dataUcSentToRoutingFn++;
                            mp->stats.dataUcSentToMesh++;
                            mp->stats.dataUcRelayedToMeshFromBss++;
                        }
                        else
                        {
                            if (proxyItem->proxyAddr == dot11->selfAddr)
                            {
                                // internetwork

                                if (DOT11s_TraceComments)
                                {
                                    char destStr[MAX_STRING_LENGTH];
                                    Dot11s_AddrAsDotIP(
                                        destStr, &rxFrameInfo.DA);
                                    char traceStr[MAX_STRING_LENGTH];
                                    sprintf(traceStr,
                                        "MPP sends BSS unicast to %s "
                                        "to upper layer via proxy",
                                        destStr);
                                    MacDot11Trace(node, dot11, NULL,
                                        traceStr);
                                }
                                Dot11s_SendFrameToUpperLayer(
                                    node, dot11,
                                    &rxFrameInfo, msg);

                                mp->stats.dataUcToNetworkLayer++;
                                // Update to DOT11 stats?
                            }
                            else
                            {
                                DOT11s_FrameInfo frameInfo = rxFrameInfo;
                                frameInfo.RA = fwdItem->nextHopAddr;
                                frameInfo.TA = dot11->selfAddr;
                                frameInfo.SA = rxFrameInfo.TA;
                                frameInfo.fwdControl.SetE2eSeqNo(
                                    Dot11sE2eList_NextSeqNo(
                                    node, dot11, frameInfo.DA,
                                    dot11->selfAddr));
                                frameInfo.fwdControl.SetTTL(DOT11s_TTL_MAX);

                                Dot11s_SendDataFrameToMesh(node, dot11,
                                    &frameInfo, msg);

                                mp->stats.dataUcSentToMesh++;
                            }
                        }
                    }
                }
            }

            break;

        } // case DOT11_DATA:

        case DOT11_MESH_DATA:
        {
            mp->stats.dataUcReceivedFromMesh++;

            if (Dot11s_IsSelfOrBssStationAddr(node, dot11, fHdr->address3))
            {
                // ........................................
                // Self or BSS station is destination
                // Pass to router function; this will update
                // routing tables. Handoff to upper layer or
                // relay to BSS as necessary..
                DOT11s_FrameInfo frameInfo = rxFrameInfo;

                BOOL packetWasRouted =
                    Dot11s_SendFrameToRoutingFunction(node, dot11, mp,
                    &frameInfo, msg);

                ERROR_Assert(packetWasRouted == FALSE,
                    "Dot11s_ReceiveDataUnicast: "
                    "Mesh data unicast with destination as self "
                    "or BSS station should not have been routed.\n");

                mp->stats.dataUcSentToRoutingFn++;

                if (rxFrameInfo.DA == dot11->selfAddr)
                {
                    Dot11s_SendFrameToUpperLayer(node, dot11,
                        &frameInfo, msg);

                    mp->stats.dataUcToNetworkLayer++;
                }
                else
                {
                    // Enqueue a 3 address frame for transmit within BSS
                    frameInfo.RA = rxFrameInfo.DA;
                    frameInfo.TA = dot11->selfAddr;

                    Dot11s_SendFrameToBss(node, dot11, &frameInfo, msg);

                    mp->stats.dataUcRelayedToBssFromMesh++;
                    mp->stats.dataUcSentToBss++;
                }
            }
            else    // not self or BSS address
            {
                // ....................................
                // Destination is outside BSS

                // Check if it is in the proxy list

                DOT11s_FwdItem* fwdItem = NULL;
                DOT11s_ProxyItem* proxyItem = NULL;
                BOOL inMesh = FALSE;

                proxyItem =
                    Dot11sProxyList_Lookup(node, dot11, rxFrameInfo.DA);
                if (proxyItem != NULL)
                {
                    inMesh = proxyItem->inMesh;
                    fwdItem = Dot11sFwdList_Lookup(
                        node, dot11, proxyItem->proxyAddr);
                    if (fwdItem == NULL)
                    {
                        ERROR_ReportError(
                            "Dot11sMpp_ReceiveDataUnicast: "
                            "Proxy Item does not have a "
                            "corresponding Fwd Item.\n");
                    }
                }

                if (inMesh || proxyItem == NULL)
                {
                    // Check for TTL before sending
                    if (rxFrameInfo.fwdControl.GetTTL() <= 1)
                    {
                        if (DOT11s_TraceComments)
                        {
                            MacDot11Trace(node, dot11, NULL,
                                "!! Dot11sMpp_ReceiveDataUnicast: "
                                "Dropped, exceeds TTL limit");
                        }

                        mp->stats.dataUcDropped++;
#ifdef ADDON_DB
                        HandleMacDBEvents(        
                            node, msg, 
                            node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                            dot11->myMacData->interfaceIndex, MAC_Drop, 
                            node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

                        MESSAGE_Free(node, msg);
                        break;
                    }

                    DOT11s_FrameInfo frameInfo = rxFrameInfo;
                    frameInfo.fwdControl.SetTTL(
                        frameInfo.fwdControl.GetTTL() - 1 );

                    //BOOL packetWasRouted =
                        Dot11s_SendFrameToRoutingFunction(node, dot11, mp,
                        &frameInfo, msg);

                    mp->stats.dataUcSentToRoutingFn++;
                    mp->stats.dataUcSentToMesh++;
                    mp->stats.dataUcRelayedToMeshFromMesh++;

                }
                else
                {
                    if (proxyItem->proxyAddr == dot11->selfAddr)
                    {
                        if (DOT11s_TraceComments)
                        {
                            char destStr[MAX_STRING_LENGTH];
                            Dot11s_AddrAsDotIP(destStr, &rxFrameInfo.DA);
                            char traceStr[MAX_STRING_LENGTH];
                            sprintf(traceStr,
                                "MPP sends mesh unicast to %s "
                                "to upper layer via proxy entry",
                                destStr);
                            MacDot11Trace(node, dot11, NULL,
                                traceStr);
                        }

                        Dot11s_SendFrameToUpperLayer(node, dot11,
                            &rxFrameInfo, msg);

                        mp->stats.dataUcToNetworkLayer++;
                    }
                    else
                    {
                        DOT11s_FrameInfo frameInfo = rxFrameInfo;
                        frameInfo.RA = fwdItem->nextHopAddr;
                        frameInfo.TA = dot11->selfAddr;
                        frameInfo.fwdControl.SetTTL(
                            frameInfo.fwdControl.GetTTL() - 1 );

                        Dot11s_SendFrameToRoutingFunction(node, dot11, mp,
                            &frameInfo, msg);

                        mp->stats.dataUcSentToRoutingFn++;
                        mp->stats.dataUcSentToMesh++;
                        mp->stats.dataUcRelayedToMeshFromMesh++;
                    }
                }
            }

            break;

        } //case DOT11_MESH_DATA:

        default:
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
                "Dot11sMpp_ReceiveDataUnicast: "
                "Received unknown unicast frame type %d\n",
                fHdr->frameType);
            ERROR_ReportError(errString);

        }
    } // switch (rxFrameInfo->frameType)

}


/**
FUNCTION   :: Dot11sMpp_ReceiveNetworkLayerPacket
LAYER      :: MAC
PURPOSE    :: Process packet from Network layer at Mesh Portal.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ nextHopAddr : Mac802Address : address of next hop
+ networkType : int         : network type
+ priority  : TosType       : type of service
RETURN     :: void
**/

static
void Dot11sMpp_ReceiveNetworkLayerPacket(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    Mac802Address nextHopAddr,
    int networkType,
    TosType priority)
{
    DOT11s_Data* mp = dot11->mp;
    DOT11s_FrameInfo frameInfo;
    Message* newMsg;

    //mp->stats.pktsFromNetworkLayer++;

    frameInfo.msg = msg;
    // frame type is default
    frameInfo.RA = nextHopAddr;
    frameInfo.TA = dot11->selfAddr;
    frameInfo.DA = nextHopAddr;
    frameInfo.SA = dot11->selfAddr;
    frameInfo.fwdControl.SetE2eSeqNo(
        Dot11sE2eList_NextSeqNo(node, dot11, nextHopAddr, dot11->selfAddr));
    frameInfo.fwdControl.SetTTL(DOT11s_TTL_MAX);

    ERROR_Assert(dot11->txFrameInfo == NULL,
        "Dot11sMpp_ReceiveNetworkLayerPacket: txFrameInfo not NULL");
    MacDot11StationResetCurrentMessageVariables(node, dot11);

    if (nextHopAddr == ANY_MAC802)
    {
        mp->stats.dataBcFromNetworkLayer++;
        frameInfo.RA = nextHopAddr;

        BOOL sendToMesh = FALSE;
        BOOL sendToBss = FALSE;

        if (Dot11s_GetAssociatedNeighborCount(node, dot11) > 0)
        {
            sendToMesh = TRUE;
        }

        if (dot11->isMAP)
        {
            if (Dot11s_GetAssociatedStationCount(node, dot11) > 0)
            {
                sendToBss = TRUE;
            }
        }

        newMsg = msg;
        if (sendToMesh && sendToBss)
        {
            newMsg = MESSAGE_Duplicate(node, msg);
        }
        if (!sendToMesh && !sendToBss)
        {
            mp->stats.dataBcDropped++;

            MESSAGE_Free(node, msg);
            return;
        }

        if (sendToMesh)
        {
            // ................................................
            // Enqueue a 4 address broadcast frame

            Dot11s_SendDataFrameToMesh(node, dot11, &frameInfo, newMsg);

            mp->stats.dataBcSentToMesh++;
        }

        if (sendToBss)
        {
            // ................................................
            // Enqueue a 3 address frame for transmit within BSS

            Dot11s_SendFrameToBss(node, dot11, &frameInfo, msg);

            mp->stats.dataBcSentToBss++;
        }
    }
    else
    {
        mp->stats.dataUcFromNetworkLayer++;

        // For Layer 3 inter-networking, omit ex-mesh proxy.
        // Add the source to proxy list
        //if (ipSourceAddr != ANY_MAC802)
        //{
        //    Dot11sProxyList_Insert(node, dot11,
        //        ipSourceAddr, FALSE, FALSE, dot11->selfAddr);
        //    Dot11sFwdList_Insert(node, dot11, dot11->selfAddr,
        //        dot11->selfAddr, DOT11s_FWD_OUTSIDE_MESH_MPP);
        //}

        if (dot11->isMAP
            && Dot11s_IsSelfOrBssStationAddr(node, dot11, nextHopAddr))
        {
            // ............................................
            // Destination is within BSS, transmit 3 address frame.
            frameInfo.RA = nextHopAddr;

            Dot11s_SendFrameToBss(node, dot11, &frameInfo, msg);

            mp->stats.dataUcSentToBss++;
        }
        else
        {
            // ............................................
            // Destination is outside BSS

            // Check if in in proxy list
            DOT11s_FwdItem* fwdItem = NULL;
            DOT11s_ProxyItem* proxyItem = NULL;

            proxyItem = Dot11sProxyList_Lookup(node, dot11, frameInfo.DA);
            if (proxyItem != NULL)
            {
                fwdItem = Dot11sFwdList_Lookup(
                    node, dot11, proxyItem->proxyAddr);
                if (fwdItem == NULL)
                {
                    ERROR_ReportError(
                        "Dot11sMpp_ReceiveNetworkLayerPacket: "
                        "Proxy Item does not have a "
                        "corresponding Fwd Item.\n");
                }

                frameInfo.RA = fwdItem->nextHopAddr;

                Dot11s_SendDataFrameToMesh(node, dot11, &frameInfo, msg);

                mp->stats.dataUcSentToMesh++;
            }
            else
            {
                Dot11s_SendFrameToRoutingFunction(node, dot11, mp,
                    &frameInfo, msg);

                mp->stats.dataUcSentToRoutingFn++;
                mp->stats.dataUcSentToMesh++;
            }
        }
    }
}


/**
FUNCTION   :: Dot11s_ReceiveDataBroadcast
LAYER      :: MAC
PURPOSE    :: Receive a data broadcast of type DOT11_MESH_DATA.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received message
RETURN     :: void
NOTES      :: An MP does not receive broadcasts from the BSS.
                The broadcast can be
                - sent as a 3 address frame to the BSS
                - forwarded as a 4 address frame to other MPs
                - if required, handed to the upper layer.
**/

void Dot11s_ReceiveDataBroadcast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11s_Data* mp = dot11->mp;

    DOT11s_FrameHdr meshHdr;
    Dot11s_ReturnMeshHeader(&meshHdr, msg);
    DOT11s_FrameHdr* fHdr = &meshHdr;

    // Received data broadcasts can only be mesh frames.
    ERROR_Assert(fHdr->frameType == DOT11_MESH_DATA,
        "Dot11s_ReceiveDataBroadcast: "
        "frame type should be Mesh Data.\n");

    mp->stats.dataBcReceivedFromMesh++;

    // Drop frame if not from associated mesh neighbor.
    if (Dot11s_IsAssociatedNeighbor(node, dot11, fHdr->sourceAddr)
        == FALSE)
    {
        if (DOT11s_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "Dot11s_ReceiveDataBroadcast: "
                "DROPPING -- frame from non-associated neighbor");
        }

        mp->stats.dataBcDropped++;
#ifdef ADDON_DB
        HandleMacDBEvents(        
            node, msg, 
            node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
            dot11->myMacData->interfaceIndex, MAC_Drop, 
            node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

        MESSAGE_Free(node, msg);
        return;
    }

    // Filter out duplicate mesh frames.
    if (Dot11sDataSeenList_Lookup(node, dot11, msg) != NULL)
    {
        mp->stats.dataBcDropped++;
#ifdef ADDON_DB
        HandleMacDBEvents(        
            node, msg, 
            node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
            dot11->myMacData->interfaceIndex, MAC_Drop, 
            node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

        MESSAGE_Free(node, msg);
        return;
    }

    // Protect against broadcast loops in case
    // ageing times are configured low.
    if (Dot11s_IsSelfOrBssStationAddr(node, dot11, fHdr->address4))
    {
        if (DOT11s_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "Dot1s_ReceiveDataBroadcast: "
                "!! Dropped, frame has own/BSS source address");
        }

        mp->stats.dataBcDropped++;
#ifdef ADDON_DB
        HandleMacDBEvents(        
            node, msg, 
            node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
            dot11->myMacData->interfaceIndex, MAC_Drop, 
            node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

        MESSAGE_Free(node, msg);
        return;
    }

    DOT11s_FrameInfo rxFrameInfo;
    rxFrameInfo.frameType = fHdr->frameType;
    rxFrameInfo.RA = fHdr->destAddr;
    rxFrameInfo.TA = fHdr->sourceAddr;
    rxFrameInfo.DA = fHdr->address3;
    rxFrameInfo.SA = fHdr->address4;
    rxFrameInfo.fwdControl = fHdr->fwdControl;

    MESSAGE_RemoveHeader(
        node,
        msg,
        DOT11s_DATA_FRAME_HDR_SIZE,
        TRACE_DOT11);

    Message* newMsg;

    // ....................................................
    // MPPs and MPs always handoff a copy to the upper layer
    // MAPs handoff if configured.

    BOOL handOffToUpperLayer = TRUE;
    if (dot11->isMAP)
    {
        handOffToUpperLayer = DOT11s_MAP_HANDOFF_BC_TO_NETWORK_LAYER;
    }
    if (handOffToUpperLayer)
    {
        if (DOT11s_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "Handoff mesh broadcast to upper layer");
        }

        newMsg = MESSAGE_Duplicate(node, msg);

        Dot11s_SendFrameToUpperLayer(node, dot11,
            &rxFrameInfo, newMsg);

        mp->stats.dataBcToNetworkLayer++;
    }

    BOOL sendToMesh = FALSE;
    BOOL sendToBss = FALSE;
    if ((rxFrameInfo.fwdControl.GetTTL() > 0)
        && (Dot11s_GetAssociatedNeighborCount(node, dot11) > 1))
    {
        sendToMesh = TRUE;
    }
    if (dot11->isMAP)
    {
        if (Dot11s_GetAssociatedStationCount(node, dot11) > 0)
        {
            sendToBss = TRUE;
        }
    }

    newMsg = msg;
    if (sendToMesh && sendToBss)
    {
        newMsg = MESSAGE_Duplicate(node, msg);
    }
    if (!sendToMesh && !sendToBss)
    {
        mp->stats.dataBcDropped++;
#ifdef ADDON_DB
        HandleMacDBEvents(        
            node, msg, 
            node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
            dot11->myMacData->interfaceIndex, MAC_Drop, 
            node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

        MESSAGE_Free(node, msg);
        return;
    }


    if (sendToMesh)
    {
        // ................................................
        // Enqueue a 4 address frame
        DOT11s_FrameInfo frameInfo = rxFrameInfo;
        // No change in frame type.
        // No change in receiving address as it is a broadcast
        frameInfo.TA = dot11->selfAddr;
        // No change in DA or SA
        // No change in E2E sequence number
        frameInfo.fwdControl.SetTTL(frameInfo.fwdControl.GetTTL() - 1);

        Dot11s_SendDataFrameToMesh(node, dot11, &frameInfo, newMsg);

        mp->stats.dataBcSentToMesh++;
    }

    // ....................................................

    if (sendToBss)
    {
        // Enqueue a 3 address frame for the BSS
        // For a 3 address frame, we reuse the existing transmit
        // code and would dequeue a message without the MAC header
        DOT11s_FrameInfo frameInfo = rxFrameInfo;
        // No change in RA as it is a broadcast
        frameInfo.TA = dot11->selfAddr;
        // DA unused for a FromDs frame.
        // Leave SA as is.

        Dot11s_SendFrameToBss(node, dot11, &frameInfo, msg);

        mp->stats.dataBcSentToBss++;
    }
}



/**
FUNCTION   :: Dot11s_ReceiveDataUnicast
LAYER      :: MAC
PURPOSE    :: A Mesh Point receives a data unicast.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : received message
RETURN     :: void
NOTES      :: Hands message to MPP if required.
                This unicast can be a Mesh Data frame or a 3 address frame
                The received unicast is one of:
                1. From an associated station to
                    a. another associated station
                    b. to self
                    c. a non-bss destination within or outside the mesh
                    d. a broadcast
                2. From a mesh neighbor to
                    a. an associated station
                    b. to self
                    c. another destination within or outside the mesh
**/

void Dot11s_ReceiveDataUnicast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11s_Data* mp = dot11->mp;

    if (mp->isMpp)
    {
        Dot11sMpp_ReceiveDataUnicast(node, dot11, msg);
        return;
    }

    DOT11_FrameHdr* fHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);

    //DOT11_ApStationListItem* apStationListItem = NULL;
    DOT11s_StationItem* stationItem = NULL;

    DOT11s_FrameInfo rxFrameInfo;
    Message* newMsg;

    rxFrameInfo.frameType = fHdr->frameType;
    rxFrameInfo.RA = fHdr->destAddr;
    rxFrameInfo.TA = fHdr->sourceAddr;
    rxFrameInfo.DA = fHdr->address3;

    if (rxFrameInfo.frameType == DOT11_DATA)       // to add DOT11_QOS_DATA
    {
        if (dot11->isMAP)
        {
            // Ensure that this frame is from an associated station
            //apStationListItem =
            //    MacDot11ApStationListGetItemWithGivenAddress(
            //        node, dot11, rxFrameInfo.TA);
            //if (apStationListItem == NULL)
            //{
            //    ERROR_ReportError("Dot11s_ReceiveDataUnicast: "
            //        "DOT11_DATA frame from a non-associated station.\n");
            //}
            stationItem =
                Dot11sStationList_Lookup(node, dot11, rxFrameInfo.TA);
            if (stationItem == NULL)
            {
                ERROR_ReportError("Dot11s_ReceiveDataUnicast: "
                    "DOT11_DATA frame from a non-associated station.\n");
            }

            rxFrameInfo.SA = fHdr->sourceAddr;
            rxFrameInfo.fwdControl = 0;

            MESSAGE_RemoveHeader(node,
                msg,
                sizeof(DOT11_FrameHdr),
                TRACE_DOT11);
        }
        else
        {
            ERROR_ReportError("Dot11s_ReceiveDataUnicast: "
                "non-MAP receives DOT11_DATA frame.\n");
        }
    }
    else if (rxFrameInfo.frameType == DOT11_MESH_DATA)
    {
        // Ensure that this frame is from a mesh neighbor
        if (Dot11s_IsAssociatedNeighbor(node, dot11, rxFrameInfo.TA)
            == FALSE)
        {
            if (DOT11s_TraceComments)
            {
                MacDot11Trace(node, dot11, NULL,
                    "Dot11s_ReceiveDataUnicast: "
                    "DOT11_MESH_DATA frame from non-associated MP");
            }

            mp->stats.dataUcDropped++;
#ifdef ADDON_DB
            HandleMacDBEvents(        
                node, msg, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

            MESSAGE_Free(node, msg);
            return;
        }

        DOT11s_FrameHdr meshHdr;
        Dot11s_ReturnMeshHeader(&meshHdr, msg);

        rxFrameInfo.SA = meshHdr.address4;
        rxFrameInfo.fwdControl = meshHdr.fwdControl;

        MESSAGE_RemoveHeader(node,
            msg,
            DOT11s_DATA_FRAME_HDR_SIZE,
            TRACE_DOT11);

    }
    else
    {
        ERROR_ReportError("Dot11s_ReceiveDataUnicast: "
            "Received unknown frame type.\n");
    }

/* See how to enable GUI
    if (node->guiOption == TRUE)
    {
        Mac802Address sourceNodeId;
        int sourceInterfaceIndex;

        if (NetworkIpGetInterfaceType(
            node, dot11->myMacData->interfaceIndex)
                == NETWORK_IPV6)
        {
            sourceNodeId =
                MAPPING_GetNodeIdFromLinkLayerAddr(
                node, rxFrameInfo.TA);
            sourceInterfaceIndex =
                MAPPING_GetInterfaceFromLinkLayerAddress(
                node, rxFrameInfo.TA);
        }
        else
        {
            sourceNodeId =
                MAPPING_GetNodeIdFromInterfaceAddress(
                node, rxFrameInfo.TA),
            sourceInterfaceIndex =
                MAPPING_GetInterfaceIndexFromInterfaceAddress(
                node, rxFrameInfo.TA);
        }

        GUI_Receive(
            sourceNodeId,
            node->nodeId,
            GUI_MAC_LAYER,
            GUI_DEFAULT_DATA_TYPE,
            sourceInterfaceIndex,
            dot11->myMacData->interfaceIndex,
            node->getNodeTime());
    }
*/

    switch (rxFrameInfo.frameType)
    {
        case DOT11_DATA:
        {
            if (rxFrameInfo.DA == ANY_MAC802)
            {
                mp->stats.dataBcReceivedFromBssAsUc++;

                // ........................................
                //Hand off one copy to the upper layer, if configured
                if (DOT11s_MAP_HANDOFF_BC_TO_NETWORK_LAYER)
                {
                    if (DOT11s_TraceComments)
                    {
                        MacDot11Trace(node, dot11, NULL,
                            "Handoff BSS broadcast to upper layer");
                    }

                    newMsg = MESSAGE_Duplicate(node, msg);
                    Dot11s_SendFrameToUpperLayer(node, dot11,
                        &rxFrameInfo, newMsg);

                    mp->stats.dataBcToNetworkLayer++;
                }


                // Need to rebroadcast it both within BSS and to mesh
                BOOL sendToMesh = FALSE;
                BOOL sendToBss = FALSE;

                if (Dot11s_GetAssociatedNeighborCount(node, dot11) > 0)
                {
                    sendToMesh = TRUE;
                }
                if (Dot11s_GetAssociatedStationCount(node, dot11) > 1)
                {
                    sendToBss = TRUE;
                }
                newMsg = msg;
                if (sendToMesh && sendToBss)
                {
                    newMsg = MESSAGE_Duplicate(node, msg);
                }
                if (!sendToMesh && !sendToBss)
                {
                    mp->stats.dataUcDropped++;

                    MESSAGE_Free(node, msg);
                    return;
                }

                if (sendToMesh)
                {
                    // ........................................
                    // Enqueue a 4 address frame
                    // This sends one broadcast to the mesh with the
                    // assumption that receiving MPs that are not
                    // authenticated/associated will not attempt to
                    // receive it.

                    DOT11s_FrameInfo frameInfo = rxFrameInfo;
                    frameInfo.RA = ANY_MAC802;
                    frameInfo.TA = dot11->selfAddr;
                    frameInfo.DA = ANY_MAC802;
                    frameInfo.SA = rxFrameInfo.SA;
                    // Use MAP's own E2E Sequence Number
                    frameInfo.fwdControl.SetE2eSeqNo(
                        Dot11sE2eList_NextSeqNo(
                        node, dot11, frameInfo.DA, dot11->selfAddr));
                    // Use max TTL
                    frameInfo.fwdControl.SetTTL(DOT11s_TTL_MAX);

                    Dot11s_SendDataFrameToMesh(node, dot11, &frameInfo, newMsg);

                    mp->stats.dataBcSentToMesh++;
                }

                if (sendToBss)
                {
                    // ........................................
                    // Enqueue a 3 address frame for transmit within BSS
                    DOT11s_FrameInfo frameInfo = rxFrameInfo;
                    // No change to frame type
                    frameInfo.RA = rxFrameInfo.DA;
                    frameInfo.TA = dot11->selfAddr;
                    // No change to DA
                    frameInfo.SA = rxFrameInfo.TA;
                    // fwdControl is unimportant

                    Dot11s_SendFrameToBss(node, dot11, &frameInfo, msg);

                    mp->stats.dataBcSentToBss++;
                }
            }
            else    // Not a broadcast address
            {
                mp->stats.dataUcReceivedFromBss++;

                // ........................................
                // Check if destination is self
                if (rxFrameInfo.DA == dot11->selfAddr)
                {
                    Dot11s_SendFrameToUpperLayer(node, dot11,
                        &rxFrameInfo, msg);

                    mp->stats.dataUcToNetworkLayer++;
                    // Check for update to DOT11 stats.

                }
                else
                {
                    // Check if destination is within BSS
                    //DOT11_ApStationListItem* destStationListItem =
                    //    MacDot11ApStationListGetItemWithGivenAddress(
                    //        node, dot11, rxFrameInfo.DA);
                    DOT11s_StationItem* stationItem =
                        Dot11sStationList_Lookup(
                        node, dot11, rxFrameInfo.DA);

                    // ....................................
                    //if (destStationListItem != NULL)
                    if (stationItem != NULL)
                    {
                        // Enqueue 3 address frame for transmit within BSS
                        DOT11s_FrameInfo frameInfo = rxFrameInfo;
                        // No change to frame type
                        frameInfo.RA = rxFrameInfo.DA;
                        frameInfo.TA = dot11->selfAddr;
                        // No change to DA
                        // No change to SA

                        Dot11s_SendFrameToBss(node, dot11, &frameInfo, msg);

                        mp->stats.dataUcRelayedToBssFromBss++;
                        mp->stats.dataUcSentToBss++;
                    }
                    else
                    {
                        // ................................
                        // Destination is outside BSS
                        // Call router function of active protocol
                        DOT11s_FrameInfo frameInfo = rxFrameInfo;
                        // frameInfo.frameType = DOT11_MESH_DATA;
                        frameInfo.RA = rxFrameInfo.DA;
                        frameInfo.TA = dot11->selfAddr;
                        frameInfo.SA = rxFrameInfo.TA;
                        //frameInfo.DA = rxFrameInfo.DA;
                        // Use MAP's own E2E Sequence Number
                        frameInfo.fwdControl.SetE2eSeqNo(
                            Dot11sE2eList_NextSeqNo(
                            node, dot11, frameInfo.DA, dot11->selfAddr));
                        // Use max TTL
                        frameInfo.fwdControl.SetTTL(DOT11s_TTL_MAX);

                        Dot11s_SendFrameToRoutingFunction(node, dot11, mp,
                            &frameInfo, msg);

                        mp->stats.dataUcSentToRoutingFn++;
                        mp->stats.dataUcSentToMesh++;
                        mp->stats.dataUcRelayedToMeshFromBss++;
                    }
                }
            }

            break;

        } // case DOT11_DATA:

        case DOT11_MESH_DATA:
        {
            mp->stats.dataUcReceivedFromMesh++;

            if (Dot11s_IsSelfOrBssStationAddr(node, dot11, rxFrameInfo.DA))
            {
                // ........................................
                // Self or BSS station is destination
                // Pass to router function. This will update
                // routing tables and handoff to upper layer.
                DOT11s_FrameInfo frameInfo = rxFrameInfo;

                BOOL packetWasRouted =
                    Dot11s_SendFrameToRoutingFunction(node, dot11, mp,
                    &frameInfo, msg);

                mp->stats.dataUcSentToRoutingFn++;

                // Should always be FALSE since that is the condition
                // under which the code reaches here.
                ERROR_Assert(packetWasRouted == FALSE,
                    "Dot11s_ReceiveDataUnicast: "
                    "Mesh data unicast with destination as self "
                    "or BSS station should not have been routed.\n");

                if (rxFrameInfo.DA == dot11->selfAddr)
                {
                    Dot11s_SendFrameToUpperLayer(node, dot11,
                        &frameInfo, msg);

                    mp->stats.dataUcToNetworkLayer++;
                }
                else
                {
                    // Enqueue a 3 address frame for transmit within BSS
                    frameInfo.RA = rxFrameInfo.DA;
                    frameInfo.TA = dot11->selfAddr;
                    //frameInfo.DA = rxFrameInfo.DA;
                    //frameInfo.SA = rxFrameInfo.SA;

                    Dot11s_SendFrameToBss(node, dot11, &frameInfo, msg);

                    mp->stats.dataUcRelayedToBssFromMesh++;
                    mp->stats.dataUcSentToBss++;

                }
            }
            else    // not self or BSS address
            {
                // ....................................
                // Destination is outside BSS
                // Call router function of active protocol

                // Check for TTL before sending
                if (rxFrameInfo.fwdControl.GetTTL() <= 1)
                {
                    if (DOT11s_TraceComments)
                    {
                        MacDot11Trace(node, dot11, NULL,
                            "!! Dot11s_ReceiveDataUnicast: "
                            "Dropped, exceeds TTL limit");
                    }

                    mp->stats.dataUcDropped++;
#ifdef ADDON_DB
                    HandleMacDBEvents(        
                        node, msg, 
                        node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                        dot11->myMacData->interfaceIndex, MAC_Drop, 
                        node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

                    MESSAGE_Free(node, msg);
                    break;
                }

                DOT11s_FrameInfo frameInfo = rxFrameInfo;
                // TA set in routing fn as metrics need to be calculated.
                frameInfo.fwdControl.SetTTL(
                    frameInfo.fwdControl.GetTTL() - 1 );

                Dot11s_SendFrameToRoutingFunction(node, dot11, mp,
                    &frameInfo, msg);

                mp->stats.dataUcSentToRoutingFn++;
                mp->stats.dataUcSentToMesh++;
                mp->stats.dataUcRelayedToMeshFromMesh++;

            }

            break;

        } //case DOT11_MESH_DATA:

        default:
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
                "Dot11s_ReceiveDataUnicast: "
                "Received unknown unicast frame type %d\n",
                rxFrameInfo.frameType);
            ERROR_ReportError(errString);

        }
    } // switch (rxFrameInfo.frameType)

}


/**
FUNCTION   :: Dot11s_ReceiveNetworkLayerPacket
LAYER      :: MAC
PURPOSE    :: Process packet from Network layer.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : message from network layer
+ nextHopAddr :  Mac802Address : address of next hop
+ networkType : int         : network type
+ priority  : TosType       : type of service
RETURN     :: void
ASSUMPTION :: ipNextHopAddr is the MAC address of final mesh destination.
NOTES      :: The next hop address could be:
                1. broadcast
                    Transmit to BSS and to mesh
                2. unicast
                    If BSS Station, queue it up
                    If in mesh, route
                    If outside mesh, send towards portal.
              IP Internetworking is assumed; all DA run a routing
              protocol; dummy static routes are not configured.
**/

void Dot11s_ReceiveNetworkLayerPacket(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    Mac802Address nextHopAddr,
    int networkType,
    TosType priority)
{
    DOT11s_Data* mp = dot11->mp;

    if (DOT11s_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char nextHopStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(nextHopStr, &nextHopAddr);
        sprintf(traceStr,
            "Pkt from network: next IP Hop %s", nextHopStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    if (mp->isMpp)
    {
        Dot11sMpp_ReceiveNetworkLayerPacket(node, dot11, msg,
            nextHopAddr, networkType, priority);
        return;
    }

    DOT11s_FrameInfo frameInfo;
    Message* newMsg;

    //mp->stats.pktsFromNetworkLayer++;

    frameInfo.msg = msg;
    // frame type is default
    frameInfo.RA = nextHopAddr;
    frameInfo.TA = dot11->selfAddr;
    frameInfo.DA = nextHopAddr;
    frameInfo.SA = dot11->selfAddr;
    frameInfo.fwdControl.SetE2eSeqNo(
        Dot11sE2eList_NextSeqNo(node, dot11,
        nextHopAddr, dot11->selfAddr));
    frameInfo.fwdControl.SetTTL(DOT11s_TTL_MAX);

    // Reset msg related variables
    ERROR_Assert(dot11->txFrameInfo == NULL,
        "Dot11s_ReceiveNetworkLayerPacket: txFrameInfo not NULL");
    MacDot11StationResetCurrentMessageVariables(node, dot11);

    if (nextHopAddr == ANY_MAC802)
    {
        mp->stats.dataBcFromNetworkLayer++;
        frameInfo.RA = nextHopAddr;

        BOOL sendToMesh = FALSE;
        BOOL sendToBss = FALSE;

        if (Dot11s_GetAssociatedNeighborCount(node, dot11) > 0)
        {
            sendToMesh = TRUE;
        }

        if (dot11->isMAP)
        {
            if (Dot11s_GetAssociatedStationCount(node, dot11) > 0)
            {
                sendToBss = TRUE;
            }
        }

        newMsg = msg;
        if (sendToMesh && sendToBss)
        {
            newMsg = MESSAGE_Duplicate(node, msg);
        }
        if (!sendToMesh && !sendToBss)
        {
            mp->stats.dataBcDropped++;

            MESSAGE_Free(node, msg);
            return;
        }

        if (sendToMesh)
        {
            // ................................................
            // Enqueue a 4 address broadcast frame

            Dot11s_SendDataFrameToMesh(node, dot11, &frameInfo, newMsg);

            mp->stats.dataBcSentToMesh++;
        }

        if (sendToBss)
        {
            // ................................................
            // Enqueue a 3 address frame for transmit within BSS

            Dot11s_SendFrameToBss(node, dot11, &frameInfo, msg);

            mp->stats.dataBcSentToBss++;
        }
    }
    else
    {
        mp->stats.dataUcFromNetworkLayer++;

        if (dot11->isMAP
            && Dot11s_IsSelfOrBssStationAddr(node, dot11, nextHopAddr))
        {
            // ............................................
            // Destination is within BSS
            // Enqueue a 3 address frame for transmit within BSS
            frameInfo.RA = nextHopAddr;

            Dot11s_SendFrameToBss(node, dot11, &frameInfo, msg);

            mp->stats.dataUcSentToBss++;
        }
        else
        {
            // ............................................
            // Destination is outside BSS
            // Call router function of active protocol

            Dot11s_SendFrameToRoutingFunction(node, dot11, mp,
                &frameInfo, msg);

            mp->stats.dataUcSentToRoutingFn++;
            mp->stats.dataUcSentToMesh++;
        }
    }
}


/**
FUNCTION   :: Dot11s_BeaconTransmitted
LAYER      :: MAC
PURPOSE    :: A mesh beacon has been transmitted.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
NOTES      :: Change initialization states, if applicable.
                MP initialization states transitions are based
                on beacon counts, for example,
                DOT11s_INIT_START_BEACON_COUNT
**/

void Dot11s_BeaconTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;

    mp->stats.beaconsSent++;

    switch (mp->state)
    {
        case DOT11s_S_INIT_COMPLETE:
        {
            // Do nothing
            break;
        }
        case DOT11s_S_INIT_START:
        {
            if (DOT11s_TraceComments)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char traceStr[MAX_STRING_LENGTH];
                sprintf(traceStr,
                    "Transition to Neighbor Discovery at %s",
                    clockStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            mp->initValues->beaconsSent = 0;
            Dot11s_SetMpState(node, dot11, DOT11s_S_NEIGHBOR_DISCOVERY);

            // Start queue aging timer
            DOT11s_TimerInfo timerInfo;
            Dot11s_StartTimer(node, dot11, &timerInfo,
                DOT11s_QUEUE_AGING_TIME,
                MSG_MAC_DOT11s_QueueAgingTimer);

            break;
        }
        case DOT11s_S_NEIGHBOR_DISCOVERY:
        {
            mp->initValues->beaconsSent++;

            if (mp->initValues->beaconsSent
                > DOT11s_NEIGHBOR_DISCOVERY_BEACON_COUNT)
            {
                if (DOT11s_TraceComments)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                    char traceStr[MAX_STRING_LENGTH];
                    sprintf(traceStr,
                        "Transition to Peer Link Setup at %s",
                        clockStr);
                    MacDot11Trace(node, dot11, NULL, traceStr);
                }

                mp->initValues->beaconsSent = 0;
                Dot11s_SetMpState(node, dot11, DOT11s_S_LINK_SETUP);

                // Start a timer for the first peer link setup
                DOT11s_TimerInfo timerInfo;
                Dot11s_StartTimer(node, dot11, &timerInfo,
                    RANDOM_nrand(dot11->seed) % DOT11s_JITTER_TIME,
                    MSG_MAC_DOT11s_LinkSetupTimer);
            }

            break;
        }
        case DOT11s_S_LINK_SETUP:
        {
            mp->initValues->beaconsSent++;

            if (mp->initValues->beaconsSent
                > DOT11s_LINK_SETUP_BEACON_COUNT)
            {
                if (DOT11s_TraceComments)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                    char traceStr[MAX_STRING_LENGTH];
                    sprintf(traceStr,
                        "Transition to Link State Discovery at %s",
                        clockStr);
                    MacDot11Trace(node, dot11, NULL, traceStr);
                }

                mp->initValues->beaconsSent = 0;
                Dot11s_SetMpState(node, dot11, DOT11s_S_LINK_STATE);

                DOT11s_TimerInfo timerInfo;
                Dot11s_StartTimer(node, dot11, &timerInfo,
                    RANDOM_nrand(dot11->seed) % DOT11s_JITTER_TIME,
                    MSG_MAC_DOT11s_LinkStateTimer);
            }

            break;
        }
        case DOT11s_S_LINK_STATE:
        {
            mp->initValues->beaconsSent++;

            if (mp->initValues->beaconsSent
                > DOT11s_LINK_STATE_BEACON_COUNT)
            {
                if (DOT11s_TraceComments)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                    char traceStr[MAX_STRING_LENGTH];
                    sprintf(traceStr,
                        "Transition to Path Selection at %s",
                        clockStr);
                    MacDot11Trace(node, dot11, NULL, traceStr);
                }

                mp->initValues->beaconsSent = 0;
                Dot11s_SetMpState(node, dot11, DOT11s_S_PATH_SELECTION);

                DOT11s_TimerInfo timerInfo;
                Dot11s_StartTimer(node, dot11, &timerInfo,
                    RANDOM_nrand(dot11->seed) % DOT11s_JITTER_TIME,
                    MSG_MAC_DOT11s_PathSelectionTimer);
            }

            break;
        }
        case DOT11s_S_PATH_SELECTION:
        {
            mp->initValues->beaconsSent++;

            if (mp->initValues->beaconsSent
                > DOT11s_PATH_SELECTION_BEACON_COUNT)
            {
                if (DOT11s_TraceComments)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                    char traceStr[MAX_STRING_LENGTH];
                    sprintf(traceStr,
                        "Transition to Init Complete at %s",
                        clockStr);
                    MacDot11Trace(node, dot11, NULL, traceStr);
                }

                mp->initValues->beaconsSent = 0;

                DOT11s_TimerInfo timerInfo;
                Dot11s_StartTimer(node, dot11, &timerInfo,
                    RANDOM_nrand(dot11->seed) % DOT11s_JITTER_TIME,
                    MSG_MAC_DOT11s_InitCompleteTimeout);
            }

            break;
        }
        default:
        {
            ERROR_ReportError("Dot11s_BeaconTransmitted : "
                "Unexpected MP state.\n");

            break;
        }
    }
}


// ------------------------------------------------------------------
// Peer Association state machine

/**
FUNCTION   :: Dot11sAssoc_GetRandomLinkId
LAYER      :: MAC
PURPOSE    :: Return a random link ID.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: unsigned int  : randome value
NOTES      :: Should be in range [1, 2^32-1], nrand returns int.
**/

static
unsigned int Dot11sAssoc_GetRandomLinkId(
    Node* node,
    MacDataDot11* dot11)
{
    unsigned int linkId = (unsigned)RANDOM_nrand(dot11->seed);
    while (linkId == 0)
    {
        linkId = (unsigned)RANDOM_nrand(dot11->seed);
    }
    return linkId;
}


/**
FUNCTION   :: Dot11sAssoc_CancelTimer
LAYER      :: MAC
PURPOSE    :: Cancels an association self timer.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ timerMsg  : Message**     : timer message to cancel
RETURN     :: void
**/

static
void Dot11sAssoc_CancelTimer(
    Node* node,
    MacDataDot11* dot11,
    Message** timerMsg)
{
    if (*timerMsg != NULL)
    {
        MESSAGE_CancelSelfMsg(node, *timerMsg);
    }
    *timerMsg = NULL;
}


/**
FUNCTION   :: Dot11sAssoc_BackoffTimer
LAYER      :: MAC
PURPOSE    :: Uses exponential backoff for timer.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ timeout   : clocktype     : current timeout value
RETURN     :: clocktype     : new timeout value.
NOTES      :: DOT11s_ASSOC_REQUESTS_MAX, DOT11s_ASSOC_RETRY_TIMEOUT,
                DOT11_ASSOC_HOLD_TIMEOUT are set around this backoff.
                Change if alternate backoff method is desirable.
**/

static
clocktype Dot11sAssoc_BackoffTimer(
    Node* node,
    MacDataDot11* dot11,
    clocktype timeout)
{
    return (timeout * 2);   // (timeout << 1);
}


/**
FUNCTION   :: Dot11sAssoc_IsValidLinkOpen
LAYER      :: MAC
PURPOSE    :: Determine if values in a Link Open message received
                from neighbor are compatible.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mp        : DOT11s_Data*  : pointer to MP structure
+ neighborItem : DOT11s_NeighborItem* : pointer to neighbor
RETURN     :: BOOL          : TRUE if valid, FALSE otherwise.
**/

static
BOOL Dot11sAssoc_IsValidLinkOpen(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp,
    DOT11s_NeighborItem* neighborItem)
{
    BOOL isValid = TRUE;

    if ((strcmp(mp->meshId, mp->assocStateData.peerMeshId) != 0)
        ||
        (mp->pathProtocol != mp->assocStateData.peerPathProtocol)
        ||
        (mp->pathMetric != mp->assocStateData.peerPathMetric)
        ||
        (neighborItem->peerLinkId != 0
        && neighborItem->peerLinkId != mp->assocStateData.peerLinkId))
    {
        isValid = FALSE;

        if (DOT11s_TraceComments)
        {
            char traceStr[MAX_STRING_LENGTH];
            char addrStr[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(addrStr, &neighborItem->neighborAddr);
            sprintf(traceStr,
                "!! Invalid Link Open from %s",
                addrStr);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }
    }

    return isValid;
}


/**
FUNCTION   :: Dot11sAssoc_IsValidLinkClose
LAYER      :: MAC
PURPOSE    :: Determine if values in a Link Close message received
                from neighbor match link formation values.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mp        : DOT11s_Data*  : pointer to MP structure
+ neighborItem : DOT11s_NeighborItem* : pointer to neighbor
RETURN     :: BOOL          : TRUE if valid, FALSE otherwise.
**/

static
BOOL Dot11sAssoc_IsValidLinkClose(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp,
    DOT11s_NeighborItem* neighborItem)
{
    BOOL isValid = TRUE;

    if ((mp->assocStateData.linkId != 0
        && neighborItem->linkId != mp->assocStateData.linkId)
        ||
        (mp->assocStateData.peerLinkId != 0
        && neighborItem->peerLinkId != mp->assocStateData.peerLinkId))
    {
        isValid = FALSE;

        if (DOT11s_TraceComments)
        {
            char traceStr[MAX_STRING_LENGTH];
            char addrStr[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(addrStr, &neighborItem->neighborAddr);
            sprintf(traceStr,
                "!! Invalid Link Close from %s",
                addrStr);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }
    }

    return isValid;
}


/**
FUNCTION   :: Dot11sAssoc_IsValidLinkConfirm
LAYER      :: MAC
PURPOSE    :: Determine if values in a Link Confirm message received
                from neighbor match link open values.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mp        : DOT11s_Data*  : pointer to MP structure
+ neighborItem : DOT11s_NeighborItem* : pointer to neighbor
RETURN     :: BOOL          : TRUE if valid, FALSE otherwise.
**/

static
BOOL Dot11sAssoc_IsValidLinkConfirm(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp,
    DOT11s_NeighborItem* neighborItem)
{
    BOOL isValid = TRUE;

    if ((strcmp(mp->meshId, mp->assocStateData.peerMeshId) != 0)
        ||
        (mp->pathProtocol != mp->assocStateData.peerPathProtocol)
        ||
        (mp->pathMetric != mp->assocStateData.peerPathMetric)
        ||
        (neighborItem->peerLinkId != 0
        && neighborItem->peerLinkId != mp->assocStateData.peerLinkId)
        ||
        (neighborItem->linkId != mp->assocStateData.linkId))
    {
        isValid = FALSE;

        if (DOT11s_TraceComments)
        {
            char traceStr[MAX_STRING_LENGTH];
            char addrStr[MAX_STRING_LENGTH];
            Dot11s_AddrAsDotIP(addrStr, &neighborItem->neighborAddr);
            sprintf(traceStr,
                "!! Invalid Link Confirm from %s",
                addrStr);
            MacDot11Trace(node, dot11, NULL, traceStr);
        }
    }

    return isValid;
}


/**
FUNCTION   :: Dot11sAssoc_IdleState
LAYER      :: MAC
PURPOSE    :: Handle events during the Association Idle state.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mp        : DOT11s_Data*  : pointer to MP structure
+ neighborItem : DOT11s_NeighborItem* : pointer to neighbor
RETURN     :: void
**/

static
void Dot11sAssoc_IdleState(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp)
{
    DOT11s_NeighborItem* neighborItem = mp->assocStateData.neighborItem;
    ERROR_Assert(neighborItem != NULL,
        "Dot11sAssoc_IdleState: NeighborItem is NULL.\n");

    switch (mp->assocStateData.event)
    {

    case DOT11s_ASSOC_S_EVENT_ENTER_STATE:
    {
        neighborItem->linkId = 0;
        neighborItem->peerLinkId = 0;
        neighborItem->retryCount = 0;
        neighborItem->isConfirmSent = FALSE;
        neighborItem->isConfirmReceived = FALSE;
        neighborItem->isCancelled = FALSE;
        neighborItem->retryTimeout = DOT11s_ASSOC_RETRY_TIMEOUT;
        neighborItem->retryTimerMsg = NULL;
        neighborItem->openTimerMsg = NULL;
        neighborItem->cancelTimerMsg = NULL;
        neighborItem->linkStatus = DOT11s_PEER_LINK_STATUS_RESERVED;
        neighborItem->peerLinkStatus = DOT11s_PEER_LINK_STATUS_RESERVED;

        neighborItem->framesSent = 0;
        neighborItem->framesResent = 0;

        break;
    }
    case DOT11s_ASSOC_S_EVENT_CANCEL_LINK:
    {
        ERROR_ReportError("Dot11sAssoc_IdleState: "
            "Invalid cancel event.\n");
        break;
    }
    case DOT11s_ASSOC_S_EVENT_ACTIVE_OPEN:
    {
        neighborItem->linkId = Dot11sAssoc_GetRandomLinkId(node, dot11);

        Dot11s_SendAssocRequestFrame(node, dot11, neighborItem);

        neighborItem->retryCount++;
        DOT11s_TimerInfo timerInfo(neighborItem->neighborAddr);
        neighborItem->retryTimerMsg =
            Dot11s_StartTimer(node, dot11, &timerInfo,
            neighborItem->retryTimeout, MSG_MAC_DOT11s_AssocRetryTimeout);

        Dot11sAssoc_SetState(node, dot11, neighborItem,
            DOT11s_ASSOC_S_OPEN_SENT);

        break;
    }
    case DOT11s_ASSOC_S_EVENT_PASSIVE_OPEN:
    {
        neighborItem->linkId = Dot11sAssoc_GetRandomLinkId(node, dot11);

        Dot11sAssoc_SetState(node, dot11, neighborItem,
            DOT11s_ASSOC_S_LISTEN);

        break;
    }
    case DOT11s_ASSOC_S_EVENT_CLOSE_RECEIVED:
    case DOT11s_ASSOC_S_EVENT_OPEN_RECEIVED:
    case DOT11s_ASSOC_S_EVENT_CONFIRM_RECEIVED:
    {
        break;
    }
    case DOT11s_ASSOC_S_EVENT_RETRY_TIMEOUT:
    case DOT11s_ASSOC_S_EVENT_OPEN_TIMEOUT:
    case DOT11s_ASSOC_S_EVENT_CANCEL_TIMEOUT:
    {
        ERROR_ReportError("Dot11sAssoc_IdleState: "
            "Invalid timer event.\n");
        break;
    }
    default:
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sAssoc_IdleState: "
            "Event ignored or unhandled -- %d.\n",
            mp->assocStateData.event);
        ERROR_ReportError(traceStr);

        break;
    }

    } //switch
}


/**
FUNCTION   :: Dot11sAssoc_ListenState
LAYER      :: MAC
PURPOSE    :: Handle events during the Association Listen state.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mp        : DOT11s_Data*  : pointer to MP structure
+ neighborItem : DOT11s_NeighborItem* : pointer to neighbor
RETURN     :: void
**/

static
void Dot11sAssoc_ListenState(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp)
{
    DOT11s_NeighborItem* neighborItem = mp->assocStateData.neighborItem;
    ERROR_Assert(neighborItem != NULL,
        "Dot11sAssoc_ListenState: NeighborItem is NULL.\n");

    switch (mp->assocStateData.event)
    {

    case DOT11s_ASSOC_S_EVENT_ENTER_STATE:
    {
        break;
    }
    case DOT11s_ASSOC_S_EVENT_CANCEL_LINK:
    {
        Dot11sAssoc_SetState(node, dot11, neighborItem,
            DOT11s_ASSOC_S_IDLE);
        break;
    }
    case DOT11s_ASSOC_S_EVENT_ACTIVE_OPEN:
    {
        // Link ID already set
        Dot11s_SendAssocRequestFrame(node, dot11, neighborItem);

        neighborItem->retryCount++;
        DOT11s_TimerInfo timerInfo(neighborItem->neighborAddr);
        neighborItem->retryTimerMsg =
            Dot11s_StartTimer(node, dot11, &timerInfo,
            neighborItem->retryTimeout, MSG_MAC_DOT11s_AssocRetryTimeout);

        Dot11sAssoc_SetState(node, dot11, neighborItem,
            DOT11s_ASSOC_S_OPEN_SENT);

        break;
    }
    case DOT11s_ASSOC_S_EVENT_PASSIVE_OPEN:
    {
        break;
    }
    case DOT11s_ASSOC_S_EVENT_CLOSE_RECEIVED:
    {
        break;
    }
    case DOT11s_ASSOC_S_EVENT_OPEN_RECEIVED:
    {
        if (Dot11sAssoc_IsValidLinkOpen(node, dot11, mp, neighborItem))
        {
            neighborItem->peerLinkId = mp->assocStateData.peerLinkId;
            Dot11s_SendAssocRequestFrame(node, dot11, neighborItem);

            Dot11s_SendAssocConfirmFrame(node, dot11, neighborItem);
            neighborItem->isConfirmSent = TRUE;

            neighborItem->retryCount++;
            DOT11s_TimerInfo timerInfo(neighborItem->neighborAddr);
            neighborItem->retryTimerMsg =
                Dot11s_StartTimer(node, dot11, &timerInfo,
                neighborItem->retryTimeout,
                MSG_MAC_DOT11s_AssocRetryTimeout);

            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_CONFIRM_SENT);
        }
        else
        {
            neighborItem->linkStatus =
                DOT11s_PEER_LINK_CLOSE_INVALID_PARAMETERS;
            // Do not send local link ID
            neighborItem->linkId = 0;
            neighborItem->peerLinkId = mp->assocStateData.peerLinkId;

            Dot11s_SendAssocCloseFrame(node, dot11,
                neighborItem, neighborItem->linkStatus);

            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_HOLDING);
        }

        break;
    }
    case DOT11s_ASSOC_S_EVENT_CONFIRM_RECEIVED:
    {
        // Must relate to an older attempt.
        break;
    }
    case DOT11s_ASSOC_S_EVENT_RETRY_TIMEOUT:
    case DOT11s_ASSOC_S_EVENT_OPEN_TIMEOUT:
    case DOT11s_ASSOC_S_EVENT_CANCEL_TIMEOUT:
    {
        ERROR_ReportError("Dot11sAssoc_ListenState: "
            "Invalid timer event.\n");
        break;
    }
    default:
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sAssoc_ListenState: "
            "Event ignored or unhandled -- %d",
            mp->assocStateData.event);
        ERROR_ReportError(traceStr);

        break;
    }

    } //switch

}


/**
FUNCTION   :: Dot11sAssoc_OpenSentState
LAYER      :: MAC
PURPOSE    :: Handle events during the Association Open Sent state.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mp        : DOT11s_Data*  : pointer to MP structure
+ neighborItem : DOT11s_NeighborItem* : pointer to neighbor
RETURN     :: void
**/

static
void Dot11sAssoc_OpenSentState(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp)
{
    DOT11s_NeighborItem* neighborItem = mp->assocStateData.neighborItem;
    ERROR_Assert(neighborItem != NULL,
        "Dot11sAssoc_OpenSentState: NeighborItem is NULL.\n");

    switch (mp->assocStateData.event)
    {

    case DOT11s_ASSOC_S_EVENT_ENTER_STATE:
    {
        Dot11s_SetNeighborState(node, neighborItem,
            DOT11s_NEIGHBOR_ASSOC_PENDING);
        break;
    }
    case DOT11s_ASSOC_S_EVENT_CANCEL_LINK:
    {
        neighborItem->linkStatus = DOT11s_PEER_LINK_CLOSE_CANCELLED;
        neighborItem->linkId = 0;
        neighborItem->isCancelled = TRUE;

        Dot11s_SendAssocCloseFrame(node, dot11, neighborItem,
            DOT11s_PEER_LINK_CLOSE_CANCELLED);

        Dot11sAssoc_SetState(node, dot11, neighborItem,
            DOT11s_ASSOC_S_HOLDING);

        break;
    }
    case DOT11s_ASSOC_S_EVENT_ACTIVE_OPEN:
    {
        ERROR_ReportError("Dot11sAssoc_OpenSentState: "
            "Invalid active open event.\n");
        break;
    }
    case DOT11s_ASSOC_S_EVENT_PASSIVE_OPEN:
    {
        ERROR_ReportError("Dot11sAssoc_OpenSentState: "
            "Invalid passive open event.\n");
        break;
    }
    case DOT11s_ASSOC_S_EVENT_CLOSE_RECEIVED:
    {
        if (Dot11sAssoc_IsValidLinkClose(node, dot11, mp, neighborItem))
        {
            neighborItem->linkStatus = DOT11s_PEER_LINK_CLOSE_RECEIVED;
            neighborItem->peerLinkStatus = mp->assocStateData.peerStatus;
            neighborItem->isCancelled = TRUE;
            neighborItem->linkId = mp->assocStateData.linkId;
            neighborItem->peerLinkId = mp->assocStateData.peerLinkId;

            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_HOLDING);
        }

        break;
    }
    case DOT11s_ASSOC_S_EVENT_OPEN_RECEIVED:
    {
        Dot11sAssoc_CancelTimer(node, dot11, &(neighborItem->retryTimerMsg));

        if (Dot11sAssoc_IsValidLinkOpen(node, dot11, mp,
            neighborItem))
        {
            neighborItem->peerLinkId = mp->assocStateData.peerLinkId;
            neighborItem->isConfirmSent = TRUE;
            Dot11s_SendAssocConfirmFrame(node, dot11, neighborItem);

            neighborItem->retryCount++;
            DOT11s_TimerInfo timerInfo(neighborItem->neighborAddr);
            neighborItem->retryTimerMsg =
                Dot11s_StartTimer(node, dot11, &timerInfo,
                neighborItem->retryTimeout,
                MSG_MAC_DOT11s_AssocRetryTimeout);

            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_CONFIRM_SENT);
        }
        else
        {
            neighborItem->linkStatus =
                DOT11s_PEER_LINK_CLOSE_INVALID_PARAMETERS;
            neighborItem->linkId = 0;
            neighborItem->peerLinkId = mp->assocStateData.peerLinkId;

            Dot11s_SendAssocCloseFrame(node, dot11,
                neighborItem, neighborItem->linkStatus);

            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_HOLDING);
        }

        break;
    }
    case DOT11s_ASSOC_S_EVENT_CONFIRM_RECEIVED:
    {
        Dot11sAssoc_CancelTimer(node, dot11, &(neighborItem->retryTimerMsg));

        if (Dot11sAssoc_IsValidLinkConfirm(node, dot11, mp,
            neighborItem))
        {
            neighborItem->peerLinkId = mp->assocStateData.peerLinkId;

            Dot11s_SendAssocConfirmFrame(node, dot11, neighborItem);
            neighborItem->isConfirmReceived = TRUE;

            DOT11s_TimerInfo timerInfo(neighborItem->neighborAddr);
            neighborItem->openTimerMsg =
                Dot11s_StartTimer(node, dot11, &timerInfo,
                DOT11s_ASSOC_OPEN_TIMEOUT,
                MSG_MAC_DOT11s_AssocOpenTimeout);

            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_CONFIRM_RECEIVED);
        }
        else
        {
            neighborItem->linkStatus =
                DOT11s_PEER_LINK_CLOSE_INVALID_PARAMETERS;
            neighborItem->peerLinkId = mp->assocStateData.peerLinkId;

            Dot11s_SendAssocCloseFrame(node, dot11,
                neighborItem, neighborItem->linkStatus);

            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_HOLDING);
        }

        break;
    }
    case DOT11s_ASSOC_S_EVENT_RETRY_TIMEOUT:
    {
        if (neighborItem->retryCount < DOT11s_ASSOC_REQUESTS_MAX)
        {
            Dot11s_SendAssocRequestFrame(node, dot11, neighborItem);

            neighborItem->retryCount++;
            neighborItem->retryTimeout =
                Dot11sAssoc_BackoffTimer(node, dot11,
                neighborItem->retryTimeout);
            DOT11s_TimerInfo timerInfo(neighborItem->neighborAddr);
            neighborItem->retryTimerMsg =
                Dot11s_StartTimer(node, dot11, &timerInfo,
                neighborItem->retryTimeout,
                MSG_MAC_DOT11s_AssocRetryTimeout);
        }
        else
        {
            neighborItem->linkStatus =
                DOT11s_PEER_LINK_CLOSE_EXCEED_MAXIMUM_RETRIES;
            neighborItem->isCancelled = TRUE;
            neighborItem->linkId = 0;

            Dot11s_SendAssocCloseFrame(node, dot11,
                neighborItem, neighborItem->linkStatus);

            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_HOLDING);
        }

        break;
    }
    case DOT11s_ASSOC_S_EVENT_OPEN_TIMEOUT:
    case DOT11s_ASSOC_S_EVENT_CANCEL_TIMEOUT:
    {
        ERROR_ReportError("Dot11sAssoc_OpenSentState: "
            "Invalid timer event.\n");
        break;
    }
    default:
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sAssoc_OpenSentState: "
            "Event ignored or unhandled -- %d",
            mp->assocStateData.event);
        ERROR_ReportError(traceStr);

        break;
    }

    } //switch

}


/**
FUNCTION   :: Dot11sAssoc_ConfirmReceivedState
LAYER      :: MAC
PURPOSE    :: Handle events during the Confirm Received state.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mp        : DOT11s_Data*  : pointer to MP structure
+ neighborItem : DOT11s_NeighborItem* : pointer to neighbor
RETURN     :: void
**/

static
void Dot11sAssoc_ConfirmReceivedState(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp)
{
    DOT11s_NeighborItem* neighborItem = mp->assocStateData.neighborItem;
    ERROR_Assert(neighborItem != NULL,
        "Dot11sAssoc_ConfirmReceivedState: NeighborItem is NULL.\n");

    switch (mp->assocStateData.event)
    {

    case DOT11s_ASSOC_S_EVENT_ENTER_STATE:
    {

        break;
    }
    case DOT11s_ASSOC_S_EVENT_CANCEL_LINK:
    {
        neighborItem->linkStatus = DOT11s_PEER_LINK_CLOSE_CANCELLED;
        neighborItem->isCancelled = TRUE;

        Dot11s_SendAssocCloseFrame(node, dot11, neighborItem,
            neighborItem->linkStatus);

        Dot11sAssoc_SetState(node, dot11, neighborItem,
            DOT11s_ASSOC_S_HOLDING);

        break;
    }
    case DOT11s_ASSOC_S_EVENT_ACTIVE_OPEN:
    {
        ERROR_ReportError("Dot11sAssoc_ConfirmReceivedState: "
            "Invalid active open event.\n");
        break;
    }
    case DOT11s_ASSOC_S_EVENT_PASSIVE_OPEN:
    {
        ERROR_ReportError("Dot11sAssoc_ConfirmReceivedState: "
            "Invalid passive open event.\n");
        break;
    }
    case DOT11s_ASSOC_S_EVENT_CLOSE_RECEIVED:
    {
        if (Dot11sAssoc_IsValidLinkClose(node, dot11, mp, neighborItem))
        {
            neighborItem->linkStatus = DOT11s_PEER_LINK_CLOSE_RECEIVED;
            neighborItem->peerLinkStatus = mp->assocStateData.peerStatus;
            neighborItem->isCancelled = TRUE;

            Dot11s_SendAssocCloseFrame(node, dot11, neighborItem,
                neighborItem->linkStatus);

            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_HOLDING);
        }

        break;
    }
    case DOT11s_ASSOC_S_EVENT_OPEN_RECEIVED:
    {
        if (Dot11sAssoc_IsValidLinkOpen(node, dot11, mp,
            neighborItem))
        {
            Dot11sAssoc_CancelTimer(node, dot11,
                &(neighborItem->openTimerMsg));

            neighborItem->linkStatus = DOT11s_PEER_LINK_SUCCESS;
            Dot11s_SendAssocConfirmFrame(node, dot11, neighborItem);

            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_ESTABLISHED);
        }

        break;
    }
    case DOT11s_ASSOC_S_EVENT_CONFIRM_RECEIVED:
    case DOT11s_ASSOC_S_EVENT_RETRY_TIMEOUT:
    {
        break;
    }
    case DOT11s_ASSOC_S_EVENT_OPEN_TIMEOUT:
    {
        neighborItem->linkStatus = DOT11s_PEER_LINK_CLOSE_TIMEOUT;

        Dot11s_SendAssocCloseFrame(node, dot11, neighborItem,
            neighborItem->linkStatus);

        Dot11sAssoc_SetState(node, dot11, neighborItem,
            DOT11s_ASSOC_S_HOLDING);
        break;
    }
    case DOT11s_ASSOC_S_EVENT_CANCEL_TIMEOUT:
    {
        break;
    }
    default:
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sAssoc_ConfirmReceivedState: "
            "Event ignored or unhandled -- %d",
            mp->assocStateData.event);
        ERROR_ReportError(traceStr);

        break;
    }

    } //switch

}


/**
FUNCTION   :: Dot11sAssoc_ConfirmSentState
LAYER      :: MAC
PURPOSE    :: Handle events during the Confirm Sent state.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mp        : DOT11s_Data*  : pointer to MP structure
+ neighborItem : DOT11s_NeighborItem* : pointer to neighbor
RETURN     :: void
**/

static
void Dot11sAssoc_ConfirmSentState(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp)
{
    DOT11s_NeighborItem* neighborItem = mp->assocStateData.neighborItem;
    ERROR_Assert(neighborItem != NULL,
        "Dot11sAssoc_ConfirmSentState: NeighborItem is NULL.\n");

    switch (mp->assocStateData.event)
    {

    case DOT11s_ASSOC_S_EVENT_ENTER_STATE:
    {
        Dot11s_SetNeighborState(node, neighborItem,
            DOT11s_NEIGHBOR_ASSOC_PENDING);

        break;
    }
    case DOT11s_ASSOC_S_EVENT_CANCEL_LINK:
    {
        neighborItem->linkStatus = DOT11s_PEER_LINK_CLOSE_CANCELLED;

        Dot11s_SendAssocCloseFrame(node, dot11, neighborItem,
            neighborItem->linkStatus);

        Dot11sAssoc_SetState(node, dot11, neighborItem,
            DOT11s_ASSOC_S_HOLDING);

        break;
    }
    case DOT11s_ASSOC_S_EVENT_ACTIVE_OPEN:
    {
        ERROR_ReportError("Dot11sAssoc_ConfirmSentState: "
            "Invalid active open event.\n");
        break;
    }
    case DOT11s_ASSOC_S_EVENT_PASSIVE_OPEN:
    {
        ERROR_ReportError("Dot11sAssoc_ConfirmSentState: "
            "Invalid passive open event.\n");
        break;
    }
    case DOT11s_ASSOC_S_EVENT_CLOSE_RECEIVED:
    {
        if (Dot11sAssoc_IsValidLinkClose(node, dot11, mp, neighborItem))
        {
            neighborItem->linkStatus = DOT11s_PEER_LINK_CLOSE_RECEIVED;
            neighborItem->peerLinkStatus = mp->assocStateData.peerStatus;
            neighborItem->isCancelled = TRUE;

            Dot11s_SendAssocCloseFrame(node, dot11, neighborItem,
                neighborItem->linkStatus);

            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_HOLDING);
        }

        break;
    }
    case DOT11s_ASSOC_S_EVENT_OPEN_RECEIVED:
    {
        if (Dot11sAssoc_IsValidLinkOpen(node, dot11, mp,
            neighborItem))
        {
            neighborItem->linkStatus = DOT11s_PEER_LINK_SUCCESS;
            Dot11s_SendAssocConfirmFrame(node, dot11, neighborItem);
        }

        break;
    }
    case DOT11s_ASSOC_S_EVENT_CONFIRM_RECEIVED:
    {
        if (Dot11sAssoc_IsValidLinkConfirm(node, dot11, mp,
            neighborItem))
        {
            Dot11sAssoc_CancelTimer(node, dot11, &(neighborItem->retryTimerMsg));

            neighborItem->linkStatus = DOT11s_PEER_LINK_SUCCESS;
            neighborItem->isConfirmReceived = TRUE;
            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_ESTABLISHED);
        }
        else
        {
            neighborItem->linkStatus =
                DOT11s_PEER_LINK_CLOSE_INVALID_PARAMETERS;

            Dot11s_SendAssocCloseFrame(node, dot11,
                neighborItem, neighborItem->linkStatus);

            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_HOLDING);
        }

        break;
    }
    case DOT11s_ASSOC_S_EVENT_RETRY_TIMEOUT:
    {
        if (neighborItem->retryCount < DOT11s_ASSOC_REQUESTS_MAX)
        {
            Dot11s_SendAssocRequestFrame(node, dot11, neighborItem);

            neighborItem->retryCount++;
            neighborItem->retryTimeout =
                Dot11sAssoc_BackoffTimer(node, dot11,
                neighborItem->retryTimeout);
            DOT11s_TimerInfo timerInfo(neighborItem->neighborAddr);
            neighborItem->retryTimerMsg =
                Dot11s_StartTimer(node, dot11, &timerInfo,
                neighborItem->retryTimeout,
                MSG_MAC_DOT11s_AssocRetryTimeout);
        }
        else
        {
            neighborItem->linkStatus =
                DOT11s_PEER_LINK_CLOSE_EXCEED_MAXIMUM_RETRIES;

            Dot11s_SendAssocCloseFrame(node, dot11,
                neighborItem, neighborItem->linkStatus);

            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_HOLDING);
        }

        break;
    }
    case DOT11s_ASSOC_S_EVENT_OPEN_TIMEOUT:
    case DOT11s_ASSOC_S_EVENT_CANCEL_TIMEOUT:
    {
        ERROR_ReportError("Dot11sAssoc_OpenSentState: "
            "Invalid timer event.\n");
        break;
    }
    default:
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sAssoc_ConfirmSentState: "
            "Event ignored or unhandled -- %d",
            mp->assocStateData.event);
        ERROR_ReportError(traceStr);

        break;
    }

    } //switch

}


/**
FUNCTION   :: Dot11sAssoc_EstablishedState
LAYER      :: MAC
PURPOSE    :: Handle events during the Association Established state.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mp        : DOT11s_Data*  : pointer to MP structure
+ neighborItem : DOT11s_NeighborItem* : pointer to neighbor
RETURN     :: void
**/

static
void Dot11sAssoc_EstablishedState(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp)
{
    DOT11s_NeighborItem* neighborItem = mp->assocStateData.neighborItem;
    ERROR_Assert(neighborItem != NULL,
        "Dot11sAssoc_EstablishedState: NeighborItem is NULL.\n");

    switch (mp->assocStateData.event)
    {

    case DOT11s_ASSOC_S_EVENT_ENTER_STATE:
    {
        neighborItem->linkStatus = DOT11s_PEER_LINK_SUCCESS;

        Dot11sAssoc_CancelTimer(node, dot11, &(neighborItem->retryTimerMsg));
        Dot11sAssoc_CancelTimer(node, dot11, &(neighborItem->openTimerMsg));

        if (neighborItem->neighborAddr > dot11->selfAddr)
        {
            Dot11s_SetNeighborState(node, neighborItem,
                DOT11s_NEIGHBOR_SUPERORDINATE_LINK_DOWN);
        }
        else
        {
            Dot11s_SetNeighborState(node, neighborItem,
                DOT11s_NEIGHBOR_SUBORDINATE_LINK_DOWN);
        }

        mp->peerCapacity--;
/* See how to enable GUI
        // GuiStart
        if (node->guiOption == TRUE)
        {
            Mac802Address neighborNodeId;
            if (NetworkIpGetInterfaceType(
                node, dot11->myMacData->interfaceIndex)
                    == NETWORK_IPV6)
            {
                neighborNodeId =
                    MAPPING_GetNodeIdFromLinkLayerAddr(
                    node, neighborItem->neighborAddr);
            }
            else
            {
                neighborNodeId =
                    MAPPING_GetNodeIdFromInterfaceAddress(
                    node, neighborItem->neighborAddr);
            }

            GUI_AddLink(
                node->nodeId,
                neighborNodeId,
                (GuiLayers) MAC_LAYER,
                GUI_WIRELESS_LINK_TYPE,
                0, 0,                   // subnetAddr and numHostBits
                node->getNodeTime());
        }
        //GuiEnd
*/
        break;
    }
    case DOT11s_ASSOC_S_EVENT_CANCEL_LINK:
    {
        neighborItem->linkStatus = DOT11s_PEER_LINK_CLOSE_CANCELLED;
        neighborItem->isCancelled = TRUE;

        mp->activeProtocol.linkCloseFunction(node, dot11,
            neighborItem->neighborAddr,
            dot11->myMacData->interfaceIndex);

        Dot11s_SendAssocCloseFrame(node, dot11, neighborItem,
            neighborItem->linkStatus);

        mp->peerCapacity++;

        Dot11sAssoc_SetState(node, dot11, neighborItem,
            DOT11s_ASSOC_S_HOLDING);

        break;
    }
    case DOT11s_ASSOC_S_EVENT_ACTIVE_OPEN:
    {
        ERROR_ReportError("Dot11sAssoc_EstablishedState: "
            "Invalid active open event.\n");
        break;
    }
    case DOT11s_ASSOC_S_EVENT_PASSIVE_OPEN:
    {
        ERROR_ReportError("Dot11sAssoc_EstablishedState: "
            "Invalid passive open event.\n");
        break;
    }
    case DOT11s_ASSOC_S_EVENT_CLOSE_RECEIVED:
    {
        if (Dot11sAssoc_IsValidLinkClose(node, dot11, mp, neighborItem))
        {
            neighborItem->linkStatus = DOT11s_PEER_LINK_CLOSE_RECEIVED;
            neighborItem->peerLinkStatus = mp->assocStateData.peerStatus;
            neighborItem->isCancelled = TRUE;

            if (mp->activeProtocol.isInitialized)
            {
                mp->activeProtocol.linkCloseFunction(node, dot11,
                    neighborItem->neighborAddr,
                    dot11->myMacData->interfaceIndex);
            }

            Dot11s_SendAssocCloseFrame(node, dot11, neighborItem,
                neighborItem->linkStatus);

            Dot11sAssoc_SetState(node, dot11, neighborItem,
                DOT11s_ASSOC_S_HOLDING);
        }

        break;
    }
    case DOT11s_ASSOC_S_EVENT_OPEN_RECEIVED:
    {
        if (Dot11sAssoc_IsValidLinkOpen(node, dot11, mp,
            neighborItem))
        {
            Dot11s_SendAssocConfirmFrame(node, dot11, neighborItem);
        }

        break;
    }
    case DOT11s_ASSOC_S_EVENT_CONFIRM_RECEIVED:
    {
        break;
    }
    case DOT11s_ASSOC_S_EVENT_RETRY_TIMEOUT:
    case DOT11s_ASSOC_S_EVENT_OPEN_TIMEOUT:
    case DOT11s_ASSOC_S_EVENT_CANCEL_TIMEOUT:
    {
        ERROR_ReportError("Dot11sAssoc_OpenSentState: "
            "Invalid timer event.\n");
        break;
    }
    default:
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sAssoc_EstablishedState: "
            "Event ignored or unhandled -- %d",
            mp->assocStateData.event);
        ERROR_ReportError(traceStr);

        break;
    }

    } //switch

}


/**
FUNCTION   :: Dot11sAssoc_HoldingState
LAYER      :: MAC
PURPOSE    :: Handle events during the Holding state.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mp        : DOT11s_Data*  : pointer to MP structure
+ neighborItem : DOT11s_NeighborItem* : pointer to neighbor
RETURN     :: void
**/

static
void Dot11sAssoc_HoldingState(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp)
{
    DOT11s_NeighborItem* neighborItem = mp->assocStateData.neighborItem;
    ERROR_Assert(neighborItem != NULL,
        "Dot11sAssoc_HoldingState: NeighborItem is NULL.\n");

    switch (mp->assocStateData.event)
    {

    case DOT11s_ASSOC_S_EVENT_ENTER_STATE:
    {
        Dot11sAssoc_CancelTimer(node, dot11, &(neighborItem->retryTimerMsg));
        Dot11sAssoc_CancelTimer(node, dot11, &(neighborItem->openTimerMsg));

        DOT11s_TimerInfo timerInfo(neighborItem->neighborAddr);
        neighborItem->cancelTimerMsg =
            Dot11s_StartTimer(node, dot11,
            &timerInfo, DOT11s_ASSOC_HOLD_TIMEOUT,
            MSG_MAC_DOT11s_AssocCancelTimeout);

        Dot11s_SetNeighborState(node,
            neighborItem, DOT11s_NEIGHBOR_NO_CAPABILITY);
/*See how to enable GUI
        // GuiStart
        if (node->guiOption == TRUE)
        {
            Mac802Address neighborNodeId;

            if (NetworkIpGetInterfaceType(
                node, dot11->myMacData->interfaceIndex)
                    == NETWORK_IPV6)
            {
                neighborNodeId =
                    MAPPING_GetNodeIdFromLinkLayerAddr(
                    node, neighborItem->neighborAddr);
            }
            else
            {
                neighborNodeId =
                    MAPPING_GetNodeIdFromInterfaceAddress(
                    node, neighborItem->neighborAddr);
            }

            GUI_DeleteLink(
                node->nodeId,
                neighborNodeId,
                (GuiLayers) MAC_LAYER,
                node->getNodeTime());
        }
        //GuiEnd
*/
        break;
    }
    case DOT11s_ASSOC_S_EVENT_CANCEL_LINK:
    case DOT11s_ASSOC_S_EVENT_ACTIVE_OPEN:
    case DOT11s_ASSOC_S_EVENT_PASSIVE_OPEN:
    case DOT11s_ASSOC_S_EVENT_CLOSE_RECEIVED:
    {
        break;
    }
    case DOT11s_ASSOC_S_EVENT_OPEN_RECEIVED:
    {
        if (Dot11sAssoc_IsValidLinkOpen(node, dot11, mp,
                neighborItem))
        {
            ; // do nothing
        }
        else
        {
            Dot11s_SendAssocCloseFrame(node, dot11,
                neighborItem, neighborItem->linkStatus);
        }

        break;
    }
    case DOT11s_ASSOC_S_EVENT_CONFIRM_RECEIVED:
    {
        // No response to response.
        if (Dot11sAssoc_IsValidLinkOpen(node, dot11, mp,
                neighborItem))
        {
            ; // do nothing
        }
        else
        {
            Dot11s_SendAssocCloseFrame(node, dot11,
                neighborItem, neighborItem->linkStatus);
        }
        break;
    }
    case DOT11s_ASSOC_S_EVENT_RETRY_TIMEOUT:
    {
        ERROR_ReportError("Dot11sAssoc_HoldingState: "
            "Invalid retry timer event.\n");
        break;
    }
    case DOT11s_ASSOC_S_EVENT_OPEN_TIMEOUT:
    {
        ERROR_ReportError("Dot11sAssoc_HoldingState: "
            "Invalid open timer event.\n");
        break;
    }
    case DOT11s_ASSOC_S_EVENT_CANCEL_TIMEOUT:
    {
        Dot11sAssoc_SetState(node, dot11, neighborItem,
            DOT11s_ASSOC_S_IDLE);
        Dot11s_SetNeighborState(node,
            neighborItem, DOT11s_NEIGHBOR_NO_CAPABILITY);
        break;
    }
    default:
    {
        char traceStr[MAX_STRING_LENGTH];
        sprintf(traceStr, "Dot11sAssoc_HoldingState: "
            "Event ignored or unhandled -- %d",
            mp->assocStateData.event);
        ERROR_ReportError(traceStr);

        break;
    }

    } //switch

}


/**
FUNCTION   :: Dot11sAssoc_SetState
LAYER      :: MAC
PURPOSE    :: Set and enter an association state.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ neighborItem : DOT11s_NeighborItem* : pointer to neighbor
+ state     : DOT11s_AssocState : new state to enter
RETURN     :: void
**/

static
void Dot11sAssoc_SetState(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_NeighborItem* neighborItem,
    DOT11s_AssocState state)
{
    DOT11s_Data* mp = dot11->mp;

    neighborItem->prevAssocState = neighborItem->assocState;
    neighborItem->assocState = state;

    switch (state)
    {
        case DOT11s_ASSOC_S_IDLE:
            neighborItem->assocStateFn = Dot11sAssoc_IdleState;
            break;
        case DOT11s_ASSOC_S_LISTEN:
            neighborItem->assocStateFn = Dot11sAssoc_ListenState;
            break;
        case DOT11s_ASSOC_S_OPEN_SENT:
            neighborItem->assocStateFn = Dot11sAssoc_OpenSentState;
            break;
        case DOT11s_ASSOC_S_CONFIRM_RECEIVED:
            neighborItem->assocStateFn = Dot11sAssoc_ConfirmReceivedState;
            break;
        case DOT11s_ASSOC_S_CONFIRM_SENT:
            neighborItem->assocStateFn = Dot11sAssoc_ConfirmSentState;
            break;
        case DOT11s_ASSOC_S_ESTABLISHED:
            neighborItem->assocStateFn = Dot11sAssoc_EstablishedState;
            break;
        case DOT11s_ASSOC_S_HOLDING:
            neighborItem->assocStateFn = Dot11sAssoc_HoldingState;
            break;
        default:
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr, "Dot11sAssoc_SetState: Invalid state %d.\n",
                state);
            ERROR_ReportError(errorStr);
            break;
    }

    if (DOT11s_TraceComments)
    {
        char traceStr[MAX_STRING_LENGTH];
        char addrStr[MAX_STRING_LENGTH];
        char clockStr[MAX_STRING_LENGTH];
        Dot11s_AddrAsDotIP(addrStr, &neighborItem->neighborAddr);
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        sprintf(traceStr, "Enter assoc state %d --- Neighbor %s at %s",
            state, addrStr, clockStr);
        MacDot11Trace(node, dot11, NULL, traceStr);
    }

    mp->assocStateData.event = DOT11s_ASSOC_S_EVENT_ENTER_STATE;
    mp->assocStateData.neighborItem = neighborItem;
    neighborItem->assocStateFn(node, dot11, mp);
}


/**
FUNCTION   :: Dot11s_LinkSetupEvent
LAYER      :: MAC
PURPOSE    :: Peer Link Setup event.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
NOTES      :: As neighbors are found during the initial Neighor
                Discovery state and, on an ongoing basis when
                the link setup timer fires, initiate
                authentication / association attempts attempts.
**/

static
void Dot11s_LinkSetupEvent(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;
    DOT11s_NeighborItem* neighborItem;
    int count = 0;

    // Look through neighbor list and initiate peer link setup
    ListItem* listItem = mp->neighborList->first;
    while (listItem != NULL
        && (count < mp->linkSetupRateLimit
            || mp->linkSetupRateLimit == 0))
    {
        neighborItem = (DOT11s_NeighborItem*) listItem->data;

        // Rate limit based on setting.
        if (neighborItem->state == DOT11s_NEIGHBOR_CANDIDATE_PEER)
        {
            // Ignore authentication.

            // Qualify sending a request based on some criteria
            // else association to noisy links will continue to
            // be attempted.
            if (neighborItem->lastBeaconTime >
                node->getNodeTime() - DOT11s_LINK_SETUP_TIMER)
            {
                mp->assocStateData.event = DOT11s_ASSOC_S_EVENT_ACTIVE_OPEN;
                mp->assocStateData.neighborItem = neighborItem;
                neighborItem->assocStateFn(node, dot11, mp);

                count++;
            }
        }
        listItem = listItem->next;
    }
}


/**
FUNCTION   :: Dot11s_LinkStateEvent
LAYER      :: MAC
PURPOSE    :: Local Link State Discovery event.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
NOTES      :: Once associated, a neighbor is considered 'down'
                till the superordinate provides a measure of
                transmit and error rate. This is done
                periodically by sending a Link State frame.
**/

static
void Dot11s_LinkStateEvent(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;
    DOT11s_NeighborItem* neighborItem;

    // Look through neighbor list and initiate peer link discovery
    ListItem* listItem = mp->neighborList->first;
    while (listItem != NULL)
    {
        neighborItem = (DOT11s_NeighborItem*) listItem->data;
        if (neighborItem->state == DOT11s_NEIGHBOR_SUBORDINATE_LINK_DOWN
            || neighborItem->state == DOT11s_NEIGHBOR_SUBORDINATE_LINK_UP)
        {
            clocktype delay =
                RANDOM_nrand(dot11->seed)
                % (DOT11s_LINK_STATE_TIMER / 2);
            DOT11s_TimerInfo timerInfo(neighborItem->neighborAddr);
            Dot11s_StartTimer(node, dot11, &timerInfo, delay,
                MSG_MAC_DOT11s_LinkStateFrameTimeout);
        }
        listItem = listItem->next;
    }
}


/**
FUNCTION   :: Dot11s_PathSelectionEvent
LAYER      :: MAC
PURPOSE    :: Handle Path Selection event.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
NOTES      :: During initialation, an MP selects an active path
                protocol and active path metric using the Link
                Setup procedures. In this state, the protocol
                / metric are initialized.
**/

static
void Dot11s_PathSelectionEvent(
    Node* node,
    MacDataDot11* dot11)
  {
    DOT11s_Data* mp = dot11->mp;

    // Initialize active path protocol
    if (mp->activeProtocol.isInitialized == FALSE)
    {
        mp->activeProtocol.pathProtocol = mp->pathProtocol;

        if (mp->pathProtocol == DOT11s_PATH_PROTOCOL_HWMP)
        {
            Hwmp_InitActiveProtocol(node, dot11);
        }
        mp->activeProtocol.isInitialized = TRUE;
    }

    // Currently, no initialization of active metric.

    // If portal, initiate portal announcements.
    if (mp->isMpp)
    {
        DOT11s_TimerInfo timerInfo;
        Dot11s_StartTimer(node, dot11, &timerInfo,
            RANDOM_nrand(dot11->seed) % DOT11s_JITTER_TIME,
            MSG_MAC_DOT11s_PannTimer);
    }
}

/**
FUNCTION   :: Dot11s_PannEvent
LAYER      :: MAC
PURPOSE    :: Portal Announcement event. Send a PANN.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/

static
void Dot11s_PannEvent(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11s_Data* mp = dot11->mp;

    DOT11s_PannData pannData;
    pannData.portalAddr = dot11->selfAddr;
    pannData.hopCount = 0;
    pannData.ttl = (unsigned char) DOT11s_TTL_MAX;
    pannData.portalSeqNo = Dot11s_GetNextPortalSeqNo(node, dot11, mp);
    pannData.metric = 0;

    Dot11s_SendPannFrame(node, dot11, &pannData);

    mp->stats.pannInitiated++;

}


/**
FUNCTION   :: Dot11s_PannPropagationEvent
LAYER      :: MAC
PURPOSE    :: Portal Announcement propagation event. Retransmit a PANN.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ portalItem : DOT11s_PortalItem*  : pointer to  portal item
RETURN     :: void
**/

static
void Dot11s_PannPropagationEvent(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_PortalItem* portalItem)
{
    DOT11s_Data* mp = dot11->mp;

    if (Dot11s_GetAssociatedNeighborCount(node, dot11) > 1)
    {
        Dot11s_SendPannFrame(node, dot11, &(portalItem->lastPannData));

        mp->stats.pannRelayed++;
    }
    else
    {
        if (DOT11s_TraceComments)
        {
            MacDot11Trace(node, dot11, NULL,
                "PANN relay omitted due to neighbor count.");
        }
    }

    // Inform active protocol
    if (mp->activeProtocol.isInitialized)
    {
        mp->activeProtocol.pathUpdateFunction(node, dot11,
            portalItem->portalAddr,
            0,                                  // Omit seq number
            portalItem->lastPannData.hopCount,
            portalItem->lastPannData.metric,
            mp->portalTimeout,
            portalItem->nextHopAddr,
            TRUE,
            dot11->myMacData->interfaceIndex);
    }
}

/**
FUNCTION   :: Dot11sMpp_ReadUserConfiguration
LAYER      :: MAC
PURPOSE    :: Read mesh portal user specified configuration.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mp        : DOT11s_Data   : pointer to mesh point structure
+ nodeInput : NodeInput     : pointer to user configuration structure
+ address   : Address*      : pointer to link layer address
RETURN     :: void
**/

static
void Dot11sMpp_ReadUserConfiguration(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp,
    const NodeInput* nodeInput,
    Address* address)
{
    ; // Currently, there is no portal specific configuration
}


/**
FUNCTION   :: Dot11s_ReadUserConfiguration
LAYER      :: MAC
PURPOSE    :: Read user specified configuration.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ mp        : DOT11s_Data   : pointer to mesh point structure
+ nodeInput : NodeInput     : pointer to user configuration structure
+ address   : Address*      : pointer to link layer address
RETURN     :: void
**/

static
void Dot11s_ReadUserConfiguration(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_Data* mp,
    const NodeInput* nodeInput,
    Address* address)
{
    BOOL wasFound = FALSE;
    char inputStr[MAX_STRING_LENGTH];
    int inputInt;
    clocktype inputTime;
    BOOL isZeroValid = FALSE;
    BOOL isNegativeValid = FALSE;
    // Read the mesh ID
    //       MAC-DOT11s-MESH-ID <string>
    // Default is DOT11s_MESH_ID_DEFAULT

    memset(mp->meshId, 0, DOT11s_MESH_ID_LENGTH_MAX + 1);
    strcpy(mp->meshId, DOT11s_MESH_ID_DEFAULT);

    IO_ReadString(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11s-MESH-ID",
        &wasFound,
        inputStr);

    if (wasFound)
    {
        if (strlen(inputStr) == 0)
        {
            ERROR_ReportError("Dot11s_ReadUserConfiguration: "
                "Invalid value for MAC-DOT11s-MESH-ID "
                "in configuration file.\n"
                "Value is missing.\n");
        }

        if (strlen(inputStr) > DOT11s_MESH_ID_LENGTH_MAX)
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr, "Input length of mesh ID "
                "exceeds maximum permissible length.\n"
                "Node: %u, Length: %" TYPES_SIZEOFMFT "u, Value: %s\n"
                "Using initial %d characters.\n",
                node->nodeId, strlen(inputStr), inputStr,
                (int) DOT11s_MESH_ID_LENGTH_MAX);
            ERROR_ReportWarning(errStr);

            strncpy(mp->meshId, inputStr, DOT11s_MESH_ID_LENGTH_MAX);
        }
        else
        {
            strcpy(mp->meshId, inputStr);
        }
    }

    // Read the path protocol
    //       MAC-DOT11s-PATH-PROTOCOL <string>
    // Default is HWMP.
    // Currently, HWMP is the single option.

    mp->pathProtocol = DOT11s_PATH_PROTOCOL_HWMP;

    IO_ReadString(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11s-PATH-PROTOCOL",
        &wasFound,
        inputStr);

    if (wasFound)
    {
        if (strlen(inputStr) == 0)
        {
            ERROR_ReportError("Dot11s_ReadUserConfiguration: "
                "Invalid value for MAC-DOT11s-PATH-PROTOCOL "
                "in configuration file.\n"
                "Value is missing.\n");
        }

        if (strcmp(inputStr, "HWMP") != 0)
        {
            ERROR_ReportError("Dot11s_ReadUserConfiguration: "
                "Invalid value for MAC-DOT11s-PATH-PROTOCOL "
                "in configuration file.\n"
                "Currently, HWMP is the single available option.\n");
        }
    }

    // Read the path metric
    //       MAC-DOT11s-PATH-METRIC <string>
    // Default is AIRTIME.
    // Currently, AIRTIME is the single option.

    mp->pathMetric = DOT11s_PATH_METRIC_AIRTIME;

    IO_ReadString(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11s-PATH-METRIC",
        &wasFound,
        inputStr);

    if (wasFound)
    {
        if (strlen(inputStr) == 0)
        {
            ERROR_ReportError("Dot11s_ReadUserConfiguration: "
                "Invalid value for MAC-DOT11s-PATH-METRIC "
                "in configuration file.\n"
                "Value is missing.\n");
        }

        if (strcmp(inputStr, "AIRTIME") != 0)
        {
            ERROR_ReportError("Dot11s_ReadUserConfiguration: "
                "Invalid value for MAC-DOT11s-PATH-METRIC "
                "in configuration file.\n"
                "Currently, AIRTIME is the single available option.\n");
        }
    }

    // Read the link setup period.
    //       MAC-DOT11s-LINK-SETUP-PERIOD <time>
    // Default is DOT11s_LINK_SETUP_TIMER.

    mp->linkSetupPeriod = DOT11s_LINK_SETUP_TIMER;

    Dot11sIO_ReadTime(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-LINK-SETUP-PERIOD",
        &wasFound,
        &inputTime,
        isZeroValid);

    if (wasFound)
    {
        // No validation check (e.g. some minimum sensible period).
        mp->linkSetupPeriod = inputTime;
    }

    // Read the link setup rate limit.
    //       MAC-DOT11s-LINK-SETUP-RATE-LIMIT <int>
    // Default is DOT11s_LINK_SETUP_RATE_LIMIT_DEFAULT.
    // Input of 0 indicates no limit.

    mp->linkSetupRateLimit = DOT11s_LINK_SETUP_RATE_LIMIT_DEFAULT;

    Dot11sIO_ReadInt(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-LINK-SETUP-RATE-LIMIT",
        &wasFound,
        &inputInt,
        TRUE,                   // zero is a valid input
        isNegativeValid);

    if (wasFound)
    {
        mp->linkSetupRateLimit = inputInt;
    }

    // Read the link state period.
    //       MAC-DOT11s-LINK-STATE-PERIOD  <time>
    // Default is DOT11s_LINK_STATE_TIMER.

    mp->linkStatePeriod = DOT11s_LINK_STATE_TIMER;

    Dot11sIO_ReadTime(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-LINK-STATE-PERIOD",
        &wasFound,
        &inputTime,
        isZeroValid);

    if (wasFound)
    {
        mp->linkStatePeriod = inputTime;
    }

    // Read net diameter.
    //       MAC-DOT11s-NET-DIAMETER <int>
    // Default is DOT11s_NET_DIAMETER_DEFAULT.

    mp->netDiameter = DOT11s_NET_DIAMETER_DEFAULT;

    Dot11sIO_ReadInt(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-NET-DIAMETER",
        &wasFound,
        &inputInt,
        isZeroValid,
        isNegativeValid);

    if (wasFound)
    {
        if (inputInt > 255)
        {
            ERROR_ReportError("Dot11s_ReadUserConfiguration: "
                "Invalid value for MAC-DOT11s-NET-DIAMETER "
                "in configuration file.\n"
                "Value should be less than 256.\n");
        }

        mp->netDiameter = (unsigned char) inputInt;
    }

    // Read the node traversal time.
    //       MAC-DOT11s-NODE-TRAVERSAL-TIME <time>
    // Default is DOT11s_NODE_TRAVERSAL_TIME_DEFAULT.

    mp->nodeTraversalTime = DOT11s_NODE_TRAVERSAL_TIME_DEFAULT;

    Dot11sIO_ReadTime(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-NODE-TRAVERSAL-TIME",
        &wasFound,
        &inputTime,
        isZeroValid);

    if (wasFound)
    {
        mp->nodeTraversalTime = inputTime;
    }

    // Check if node is a Mesh Point Portal
    //       MAC-DOT11s-MESH-PORTAL YES | NO
    // Default is NO.

    mp->isMpp = FALSE;

    IO_ReadString(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11s-MESH-PORTAL",
        &wasFound,
        inputStr);

    if (wasFound) {
        if (!strcmp(inputStr, "YES"))
        {
            mp->isMpp = TRUE;
        }
        else if (!strcmp(inputStr, "NO"))
        {
            mp->isMpp = FALSE;
        }
        else
        {
            ERROR_ReportError("Dot11s_ReadUserConfiguration: "
                "Invalid value for MAC-DOT11s-MESH-PORTAL "
                "in configuration file.\n"
                "Expecting YES or NO.\n");
        }
    }

    // Read the portal announcement period.
    //       MAC-DOT11s-PORTAL-ANNOUNCEMENT-PERIOD <time>
    // Default is DOT11s_PANN_TIMER.
    // Frames do not carry this value; it has to be set mesh-wide.

    mp->pannPeriod = DOT11s_PANN_TIMER;

    Dot11sIO_ReadTime(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-PORTAL-ANNOUNCEMENT-PERIOD",
        &wasFound,
        &inputTime,
        isZeroValid);

    if (wasFound)
    {
        mp->pannPeriod = inputTime;
    }

    // Read the portal timeout period.
    //       MAC-DOT11s-PORTAL-TIMEOUT <time>
    // Default is DOT11s_PORTAL_TIMEOUT_DEFAULT.

    mp->portalTimeout = DOT11s_PORTAL_TIMEOUT_DEFAULT;

    Dot11sIO_ReadTime(
        node,
        node->nodeId,
        address,
        nodeInput,
        "MAC-DOT11s-PORTAL-TIMEOUT",
        &wasFound,
        &inputTime,
        isZeroValid);


    if (wasFound)
    {
        mp->portalTimeout = inputTime;
    }


    if (mp->isMpp)
    {
        Dot11sMpp_ReadUserConfiguration(node, dot11, mp,
            nodeInput, address);
    }

}


/**
FUNCTION   :: Dot11s_PostInit
LAYER      :: MAC
PURPOSE    :: Mesh Point post initialization.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ nodeInput : NodeInput*    : pointer to user configuration data
+ address   : Address*      : pointer to link layer address
RETURN     :: void
NOTES      :: Set some operational variables needed after other
                initialization is complete e.g beaconing, SSID.
**/
static
void Dot11s_PostInit(
    Node* node,
    MacDataDot11* dot11,
    const NodeInput* nodeInput,
    Address* address,
    NetworkType networkType)
{
    DOT11s_Data* mp = dot11->mp;

    // Protect against relay being enabled for MPs
    dot11->relayFrames = FALSE;

    // Set the SSID to a reserved value
    memset(mp->initValues->configSSID, 0, DOT11_SSID_MAX_LENGTH);
    strcpy(mp->initValues->configSSID,
           MacDot11MibGetSSID(dot11));
    memcpy(dot11->stationMIB->dot11DesiredSSID, DOT11s_SSID_WILDCARD,
           DOT11_SSID_MAX_LENGTH);


    // Set/estimate beacon frame size/duration to anticipate conflicts
    if (PHY_GetModel(node, dot11->myMacData->phyNumber) == PHY802_11b
        || PHY_GetModel(node, dot11->myMacData->phyNumber) == PHY_ABSTRACT)
    {
        mp->initValues->beaconDuration =
            DOT11s_BEACON_DURATION_80211b_APPROX;
    }
    else if (PHY_GetModel(node, dot11->myMacData->phyNumber) == PHY802_11a)
    {
        mp->initValues->beaconDuration =
            DOT11s_BEACON_DURATION_80211a_APPROX;
    }
    else
    {
        ERROR_ReportError("Dot11s_Init: Unsupported PHY model.\n");
    }

    // Initialize beacon start time over a longer interval
    // to minimize overlap with multiple neighboring MPs.
    // Subsequently, during the MP init stages, further
    // collision protection is attempted.
    BOOL wasFound;
    int beaconStart;
    clocktype aBeaconStartOffset;
    clocktype aBeaconIntervalOffset;

    IO_ReadInt(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11-BEACON-START-TIME",
        &wasFound,
        &beaconStart);

    // If beacon start time offset has not been specified
    // generate a random start time.
    if (wasFound)
    {
        aBeaconStartOffset = dot11->beaconTime;
    }
    else
    {
        aBeaconStartOffset = MacDot11TUsToClocktype(
            (unsigned short)
            (RANDOM_nrand(dot11->seed)
            % MacDot11ClocktypeToTUs(dot11->beaconInterval)));
    }

    // Generate a random number of beacon intervals before
    // sending the first beacon.
    aBeaconIntervalOffset =
        dot11->beaconInterval
        * (RANDOM_nrand(dot11->seed) % DOT11s_INIT_START_BEACON_COUNT);

    dot11->beaconTime = aBeaconStartOffset + aBeaconIntervalOffset;

    // Start beacon timer
    MacDot11StationStartTimerOfGivenType(node, dot11, dot11->beaconTime,
        MSG_MAC_DOT11_Beacon);

    MacDot11ManagementSetState(node, dot11, DOT11_S_M_INIT);

    Dot11s_SetMpState(node, dot11, DOT11s_S_INIT_START);

}


/**
FUNCTION   :: Dot11s_Init
LAYER      :: MAC
PURPOSE    :: Mesh Point initialization.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ nodeInput : NodeInput*    : pointer to user configuration data
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
NOTES      :: Check defaults, read relevant user configuration,
                and initialize mesh variables.
**/

void Dot11s_Init(
    Node* node,
    const NodeInput* nodeInput,
    MacDataDot11* dot11,
    NetworkType networkType)
{
    ERROR_Assert(dot11->isMP == TRUE,
        "Dot11s_Init: Not initialized as a mesh point.\n");

    // Sanity checks on defaults
    ERROR_Assert(
        strlen(DOT11s_MESH_ID_DEFAULT) <= DOT11s_MESH_ID_LENGTH_MAX,
        "Dot11s_Init: Default Mesh ID exceeds permissible length.\n");
    ERROR_Assert(
        DOT11s_DATA_FRAME_HDR_SIZE >= sizeof(DOT11s_FrameHdr),
        "Dot11s_Init: Defined mesh data frame size is "
        "smaller than size of frame structure.\n");

    // Initialize Mesh Point related values.
    DOT11s_Data* mp;
    Dot11s_MallocMemset0(DOT11s_Data, mp);
    dot11->mp = mp;

    ListInit(node, &(mp->neighborList));
    ListInit(node, &(mp->portalList));

    Dot11s_MallocMemset0(DOT11s_InitValues, mp->initValues);

    ListInit(node, &(mp->proxyList));
    ListInit(node, &(mp->fwdList));
    ListInit(node, &(mp->dataSeenList));
    ListInit(node, &(mp->e2eList));
    ListInit(node, &(mp->stationList));

    Dot11s_Memset0(DOT11s_Stats, &(mp->stats));

    Address address;
    NetworkGetInterfaceInfo(node,
        dot11->myMacData->interfaceIndex, &address, networkType);

    // Read user configured values
    Dot11s_ReadUserConfiguration(node, dot11, mp, nodeInput, &address);

    mp->peerCapacity = DOT11s_PEER_CAPACITY_MAX;

    // Initialize configured path protocol
    if (mp->pathProtocol == DOT11s_PATH_PROTOCOL_HWMP)
    {
        Hwmp_Init(node, dot11, nodeInput, &address, networkType);
    }

    Dot11s_PostInit(node, dot11, nodeInput, &address, networkType);
}


/**
FUNCTION   :: Dot11s_HandleTimeout
LAYER      :: MAC
PURPOSE    :: Process self timer events & those of active protocol.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ msg       : Message*      : timer message
RETURN     :: void
**/

void Dot11s_HandleTimeout(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11s_Data* mp = dot11->mp;

    switch (msg->eventType)
    {
        case MSG_MAC_DOT11s_LinkSetupTimer:
        {
            if (DOT11s_TraceComments)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char traceStr[MAX_STRING_LENGTH];
                sprintf(traceStr,
                    "Link Setup timer at %s",
                    clockStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            Dot11s_LinkSetupEvent(node, dot11);

            // Reuse message; start next link setup event timer
            MESSAGE_Send(node, msg,
               (RANDOM_nrand(dot11->seed) % DOT11s_JITTER_TIME)
                + DOT11s_LINK_SETUP_TIMER);

            break;
        }
        case MSG_MAC_DOT11s_LinkStateTimer:
        {
            if (DOT11s_TraceComments)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char traceStr[MAX_STRING_LENGTH];
                sprintf(traceStr,
                    "Link State Discovery timer at %s",
                    clockStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            Dot11s_LinkStateEvent(node, dot11);

            // Reuse message; start next link state event timer
            MESSAGE_Send(node, msg,
               (RANDOM_nrand(dot11->seed) % DOT11s_JITTER_TIME)
                + DOT11s_LINK_STATE_TIMER);

            break;
        }
        case MSG_MAC_DOT11s_LinkStateFrameTimeout:
        {
            DOT11s_TimerInfo* timerInfo
                = (DOT11s_TimerInfo*) MESSAGE_ReturnInfo(msg);
            Mac802Address neighborAddr = timerInfo->addr;

            DOT11s_NeighborItem* neighborItem =
                Dot11sNeighborList_Lookup(node, dot11, neighborAddr);
            if (neighborItem == NULL)
            {
                MESSAGE_Free(node, msg);
                break;
            }

            if (DOT11s_TraceComments)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char neighborStr[MAX_STRING_LENGTH];
                Dot11s_AddrAsDotIP(neighborStr, &neighborAddr);
                char traceStr[MAX_STRING_LENGTH];
                sprintf(traceStr,
                    "Link State Frame timer for %s at %s",
                    neighborStr, clockStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            if (neighborItem->state > DOT11s_NEIGHBOR_ASSOC_PENDING)
            {
                Dot11s_SendLinkStateFrame(node, dot11,
                    neighborItem->neighborAddr, neighborItem);
            }
            else
            {
                MacDot11Trace(node, dot11, NULL,
                    "!! Neighbor not associated");
            }

            MESSAGE_Free(node, msg);

            break;
        }
        case MSG_MAC_DOT11s_PathSelectionTimer:
        {
            if (DOT11s_TraceComments)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char traceStr[MAX_STRING_LENGTH];
                sprintf(traceStr,
                    "Path Selection timer at %s",
                    clockStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            Dot11s_PathSelectionEvent(node, dot11);

            // Start next path selection event timer.
            // This has no current utility.
            //MESSAGE_Send(node, msg,
            //    (RANDOM_erand(dot11->seed) >= 0.5f ? 1 : -1)
            //    * (RANDOM_nrand(dot11->seed) % DOT11s_JITTER_TIME)
            //    + DOT11s_PATH_SELECTION_TIMER);

            MESSAGE_Free(node, msg);

            break;
        }
        case MSG_MAC_DOT11s_InitCompleteTimeout:
        {
            if (DOT11s_TraceComments)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char traceStr[MAX_STRING_LENGTH];
                sprintf(traceStr,
                    "Init Complete. Starting all mesh services at %s",
                    clockStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            // Restore SSID value for MAP.
            if (dot11->isMAP)
            {
                memset(dot11->stationMIB->dot11DesiredSSID,
                       0, DOT11_SSID_MAX_LENGTH);
                memcpy(dot11->stationMIB->dot11DesiredSSID,
                       mp->initValues->configSSID, DOT11_SSID_MAX_LENGTH);
            }

            // Delete temporary init values
            MEM_free(mp->initValues);
            mp->initValues = NULL;

            // Start AP services
            Dot11s_SetMpState(node, dot11, DOT11s_S_INIT_COMPLETE);

            dot11->stationAuthenticated = TRUE;
            dot11->stationAssociated = TRUE;
            // FIXME: What is the appropriate AP management state?
            //MacDot11ManagementSetState(node, dot11, DOT11_S_M_IDLE);

            // Inform active protocol
            // Not needed for HWMP.

            // Start maintenance timer
            DOT11s_TimerInfo timerInfo;
            Dot11s_StartTimer(node, dot11, &timerInfo, SECOND,
                MSG_MAC_DOT11s_MaintenanceTimer);

            if (dot11->state == DOT11_S_IDLE
                && MacDot11StationPhyStatus(node, dot11) == PHY_IDLE)
            {
                MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
            }

            MESSAGE_Free(node, msg);

            break;
        }
        case MSG_MAC_DOT11s_MaintenanceTimer:
        {
            Dot11sDataSeenList_AgeItems(node, dot11);

            Dot11sPortalList_AgeItems(node, dot11);

            Dot11sNeighborList_AgeItems(node, dot11);

            // Start next maintenance timer
            MESSAGE_Send(node, msg, SECOND);

            break;
        }
        case MSG_MAC_DOT11s_QueueAgingTimer:
        {
            Dot11sMgmtQueue_AgeItems(node, dot11);
            Dot11sDataQueue_AgeItems(node, dot11);

            // Start next aging timer
            MESSAGE_Send(node, msg, DOT11s_QUEUE_AGING_TIME);

            break;
        }
        case MSG_MAC_DOT11s_AssocRetryTimeout:
        {
            DOT11s_TimerInfo* timerInfo
                = (DOT11s_TimerInfo*) MESSAGE_ReturnInfo(msg);
            Mac802Address destAddr = timerInfo->addr;

            if (DOT11s_TraceComments)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char destStr[MAX_STRING_LENGTH];
                Dot11s_AddrAsDotIP(destStr, &destAddr);
                char traceStr[MAX_STRING_LENGTH];
                sprintf(traceStr,
                    "Assoc Retry timeout for %s at %s",
                    destStr, clockStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            // Find neighbor item with destAddr
            DOT11s_NeighborItem* neighborItem =
                Dot11sNeighborList_Lookup(node, dot11, destAddr);
            if (neighborItem == NULL)
            {
                MESSAGE_Free(node, msg);
                break;
            }

            neighborItem->retryTimerMsg = NULL;
            mp->assocStateData.event = DOT11s_ASSOC_S_EVENT_RETRY_TIMEOUT;
            mp->assocStateData.neighborItem = neighborItem;
            neighborItem->assocStateFn(node, dot11, mp);

            MESSAGE_Free(node, msg);

            break;
        }
        case MSG_MAC_DOT11s_AssocOpenTimeout:
        {
            DOT11s_TimerInfo* timerInfo
                = (DOT11s_TimerInfo*) MESSAGE_ReturnInfo(msg);
            Mac802Address destAddr = timerInfo->addr;

            if (DOT11s_TraceComments)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char destStr[MAX_STRING_LENGTH];
                Dot11s_AddrAsDotIP(destStr, &destAddr);
                char traceStr[MAX_STRING_LENGTH];
                sprintf(traceStr,
                    "Assoc Open timeout for %s at %s",
                    destStr, clockStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            // Find neighbor item with destAddr
            DOT11s_NeighborItem* neighborItem =
                Dot11sNeighborList_Lookup(node, dot11, destAddr);
            if (neighborItem == NULL)
            {
                MESSAGE_Free(node, msg);
                break;
            }

            neighborItem->openTimerMsg = NULL;
            mp->assocStateData.event = DOT11s_ASSOC_S_EVENT_OPEN_TIMEOUT;
            mp->assocStateData.neighborItem = neighborItem;
            neighborItem->assocStateFn(node, dot11, mp);

            MESSAGE_Free(node, msg);

            break;
        }
        case MSG_MAC_DOT11s_AssocCancelTimeout:
        {
            DOT11s_TimerInfo* timerInfo
                = (DOT11s_TimerInfo*) MESSAGE_ReturnInfo(msg);
            Mac802Address destAddr = timerInfo->addr;

            if (DOT11s_TraceComments)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char destStr[MAX_STRING_LENGTH];
                Dot11s_AddrAsDotIP(destStr, &destAddr);
                char traceStr[MAX_STRING_LENGTH];
                sprintf(traceStr,
                    "Assoc Cancel timeout for %s at %s",
                    destStr, clockStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            // Find neighbor item with destAddr
            DOT11s_NeighborItem* neighborItem =
                Dot11sNeighborList_Lookup(node, dot11, destAddr);
            if (neighborItem == NULL)
            {
                MESSAGE_Free(node, msg);
                break;
            }

            neighborItem->cancelTimerMsg = NULL;
            mp->assocStateData.event = DOT11s_ASSOC_S_EVENT_CANCEL_TIMEOUT;
            mp->assocStateData.neighborItem = neighborItem;
            neighborItem->assocStateFn(node, dot11, mp);

            MESSAGE_Free(node, msg);

            break;
        }
        case MSG_MAC_DOT11s_PannTimer:
        {
            ERROR_Assert(mp->isMpp,
                "Dot11s_HandleTimeout: "
                "Portal Announcement Event for non-portal.\n");

            if (DOT11s_TraceComments)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char traceStr[MAX_STRING_LENGTH];
                sprintf(traceStr,
                    "Portal Announcement timer at %s",
                    clockStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            Dot11s_PannEvent(node, dot11);

            // Start next PANN event timer
            MESSAGE_Send(node, msg,
                 (RANDOM_nrand(dot11->seed) % DOT11s_JITTER_TIME)
                + DOT11s_PANN_TIMER);

            break;
        }
        case MSG_MAC_DOT11s_PannPropagationTimeout:
        {
            if (DOT11s_TraceComments)
            {
                char clockStr[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                char traceStr[MAX_STRING_LENGTH];
                sprintf(traceStr,
                    "PANN propagation timer at %s",
                    clockStr);
                MacDot11Trace(node, dot11, NULL, traceStr);
            }

            DOT11s_TimerInfo* timerInfo
                = (DOT11s_TimerInfo*) MESSAGE_ReturnInfo(msg);
            Mac802Address portalAddr = timerInfo->addr;

            DOT11s_PortalItem* portalItem =
                Dot11sPortalList_Lookup(node, dot11, portalAddr);
            if (portalItem != NULL)
            {
                Dot11s_PannPropagationEvent(node, dot11, portalItem);
            }
            else
            {
                if (DOT11s_TraceComments)
                {
                    MacDot11Trace(node, dot11, NULL,
                        "!! Dot11s_HandleTimeout: "
                        "Portal not found -- PANN propagation timeout");
                }
            }

            MESSAGE_Free(node, msg);
            break;
        }

        case MSG_MAC_DOT11s_HwmpActiveRouteTimeout:
        case MSG_MAC_DOT11s_HwmpDeleteRouteTimeout:
        case MSG_MAC_DOT11s_HwmpRreqReplyTimeout:
        case MSG_MAC_DOT11s_HwmpSeenTableTimeout:
        case MSG_MAC_DOT11s_HwmpBlacklistTimeout:
        case MSG_MAC_DOT11s_HwmpTbrRannTimeout:
        case MSG_MAC_DOT11s_HwmpTbrMaintenanceTimeout:
        case MSG_MAC_DOT11s_HmwpTbrRrepTimeout:
        {
            Hwmp_HandleTimeout(node, dot11, msg);

            break;
        }

        default:
        {
            ERROR_ReportError("Dot11s_HandleTimeout: "
                "Unknown message event type.\n");

            break;
        }
    }
}


/**
FUNCTION   :: Dot11s_PrintStats
LAYER      :: MAC
PURPOSE    :: Print end-of-run simulation statistics.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ interfaceIndex : int      : interface index
RETURN     :: void
**/

static
void Dot11s_PrintStats(
    Node* node,
    MacDataDot11* dot11,
    int interfaceIndex)
{
    DOT11s_Data* mp = dot11->mp;
    DOT11s_Stats* stats = &(mp->stats);
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "Mesh beacons sent = %d",
        stats->beaconsSent);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh beacons received = %d",
        stats->beaconsReceived);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh association requests sent = %d",
        stats->assocRequestsSent);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh association requests received = %d",
        stats->assocRequestsReceived);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh association responses sent = %d",
        stats->assocResponsesSent);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh association responses received = %d",
        stats->assocResponsesReceived);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh association close sent = %d",
        stats->assocCloseSent);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh association close received = %d",
        stats->assocCloseReceived);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh link state announcements sent = %d",
        stats->linkStateSent);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh link state announcements received = %d",
        stats->linkStateReceived);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh portal announcements initiated = %d",
        stats->pannInitiated);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh portal announcements relayed = %d",
        stats->pannRelayed);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh portal announcements received = %d",
        stats->pannReceived);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh portal announcements dropped = %d",
        stats->pannDropped);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh management broadcasts dropped = %d",
        stats->mgmtBcDropped);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh management unicasts dropped = %d",
        stats->mgmtUcDropped);
    DOT11s_STATS_PRINT;

    //sprintf(buf, "Mesh packets to Network layer = %d",
    //    stats->pktsToNetworkLayer);
    //DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data broadcasts sent to Network layer = %d",
        stats->dataBcToNetworkLayer);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data unicasts sent to Network layer = %d",
        stats->dataUcToNetworkLayer);
    DOT11s_STATS_PRINT;

    //sprintf(buf, "Mesh packets from Network layer = %d",
    //    stats->pktsFromNetworkLayer);
    //DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data broadcasts received from Network layer = %d",
        stats->dataBcFromNetworkLayer);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data unicasts received from Network layer = %d",
        stats->dataUcFromNetworkLayer);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data broadcasts sent to BSS = %d",
        stats->dataBcSentToBss);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data broadcasts sent to mesh = %d",
        stats->dataBcSentToMesh);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data broadcasts received from BSS as unicasts = %d",
        stats->dataBcReceivedFromBssAsUc);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data broadcasts received from mesh = %d",
        stats->dataBcReceivedFromMesh);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data broadcasts dropped = %d",
        stats->dataBcDropped);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data unicasts sent to BSS = %d",
        stats->dataUcSentToBss);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data unicasts sent to mesh = %d",
        stats->dataUcSentToMesh);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data unicasts received from BSS = %d",
        stats->dataUcReceivedFromBss);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data unicasts received from mesh = %d",
        stats->dataUcReceivedFromMesh);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data unicasts relayed to BSS from BSS = %d",
        stats->dataUcRelayedToBssFromBss);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data unicasts relayed to BSS from mesh = %d",
        stats->dataUcRelayedToBssFromMesh);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data unicasts relayed to mesh from BSS = %d",
        stats->dataUcRelayedToMeshFromBss);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data unicasts relayed to mesh from mesh = %d",
        stats->dataUcRelayedToMeshFromMesh);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data unicasts sent to routing function = %d",
        stats->dataUcSentToRoutingFn);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh data unicasts dropped = %d",
        stats->dataUcDropped);
    DOT11s_STATS_PRINT;

    // Packets dropped due to lifetime in queue.
    sprintf(buf, "Mesh queue management broadcasts dropped = %d",
        stats->mgmtQueueBcDropped);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh queue management unicasts dropped = %d",
        stats->mgmtQueueUcDropped);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh queue data broadcasts dropped = %d",
        stats->dataQueueBcDropped);
    DOT11s_STATS_PRINT;

    sprintf(buf, "Mesh queue data unicasts dropped = %d",
        stats->dataQueueUcDropped);
    DOT11s_STATS_PRINT;

}


/**
FUNCTION   :: Dot11s_Finalize
LAYER      :: MAC
PURPOSE    :: End of simulation finalization.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ interfaceIndex : int      : interface index
RETURN     :: void
NOTES      :: Print stats, release memory.
                In turn, requests finalization for path protocol.
**/

void Dot11s_Finalize(
    Node* node,
    MacDataDot11* dot11,
    int interfaceIndex)
{
    DOT11s_Data* mp = dot11->mp;

    Dot11s_PrintStats(node, dot11, interfaceIndex);

    // Finalize Hwmp
    if (mp->activeProtocol.pathProtocol == DOT11s_PATH_PROTOCOL_HWMP)
    {
        Hwmp_Finalize(node, dot11);
    }

    // If still in Init stage, clear temporary values
    if (mp->initValues != NULL)
    {
        MEM_free(mp->initValues);
    }

    // Clear neighbor list
    Dot11sNeighborList_Finalize(node, dot11);

    // Clear portal list
    Dot11sPortalList_Finalize(node, dot11);

    // Free proxy list memory
    Dot11sProxyList_Finalize(node, dot11);

    // Free forwarding list memory
    Dot11sFwdList_Finalize(node, dot11);

    // Free list of frames seen
    Dot11sDataSeenList_Finalize(node, dot11);

    // Free list of associated stations
    Dot11sStationList_Finalize(node, dot11);

    // Free end to end tracking list
    Dot11sE2eList_Finalize(node, dot11);

    // Free MP protocol data
    MEM_free(mp);
}


//...................................................................
// Airtime path metric is the default metric for 802.11s
// This code should probably be in a separate file
// but it is too small to do so and is implemented here.

/**
FUNCTION   :: ATLM_ComputeLinkMetric
LAYER      :: MAC
PURPOSE    :: Compute AirTime Link Metric cost in microseconds.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ phyModel  : PhyModel      : physical model (802.11a or 802.11b)
+ dataRateInMbps : int      : data rate parameter
+ PER       : float         : packet error rate [0, 1)
RETURN     :: unsigned int  : Airtime cost in microseconds
**/

static
unsigned int ATLM_ComputeLinkMetric(
    Node* node,
    MacDataDot11* dot11,
    PhyModel phyModel,
    int dataRateInMbps,
    float PER)
{
    ERROR_Assert(dataRateInMbps > 0,
        "ATLM_ComputeLinkMetric: Data rate should be greater than 0.\n");
    ERROR_Assert(PER < 1.0f && PER >= 0,
        "ATLM_ComputeLinkMetric: error rate should be [0.0, 1.0).\n");

    unsigned int cost = 0xFFFFFFFF;

    switch (phyModel)
    {
        case PHY802_11a:
        {
            cost = (unsigned)(ATLM_OVERHEAD_80211a
                    + ATLM_PROTOCOL_BITS_PER_TEST_FRAME
                    / dataRateInMbps);
            break;
        }
        case PHY802_11b:
        {
            cost = (unsigned)(ATLM_OVERHEAD_80211b
                    + ATLM_PROTOCOL_BITS_PER_TEST_FRAME
                    / dataRateInMbps);
            break;
        }
        case PHY_ABSTRACT:
        {
            cost = (unsigned)(ATLM_OVERHEAD_80211b
                    + ATLM_PROTOCOL_BITS_PER_TEST_FRAME
                    / dataRateInMbps);
            break;
        }
        default:
        {
            ERROR_ReportError("ATLM_ComputeLinkMetric : "
                "Unknown physical model.\n");
            break;
        }
    }

    cost = (unsigned int) (cost / (1.0f - PER));

    return cost;
}


/**
FUNCTION   :: Dot11s_ComputeLinkMetric
LAYER      :: MAC
PURPOSE    :: Compute link metric for given peer address.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ addr      : Mac802Address : neighbor address
RETURN     :: unsigned int  : computed link metric
NOTES      :: Currently, applies only to Airtime Link Metric
                and returns link cost in in microseconds.
**/

unsigned int Dot11s_ComputeLinkMetric(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address addr)
{
    DOT11s_Data* mp = dot11->mp;
    DOT11s_NeighborItem* neighborItem;
    unsigned int linkMetric = 0xFFFFFFFF;

    neighborItem = Dot11sNeighborList_Lookup(node, dot11, addr);
    if (neighborItem == NULL)
    {
        char errorStr[MAX_STRING_LENGTH];
        char Address[24];
        MacDot11MacAddressToStr(Address, &addr);
        sprintf(errorStr, "Dot11s_ComputeLinkMetric: "
            "Address %s not found in neighbor list.",
            Address);
        ERROR_ReportError(errorStr);
    }

    switch (mp->pathMetric)
    {
        case DOT11s_PATH_METRIC_AIRTIME:
        {
            linkMetric = ATLM_ComputeLinkMetric(node, dot11,
                neighborItem->phyModel,
                neighborItem->dataRateInMbps,
                neighborItem->PER);

            break;
        }
        default:
        {
            ERROR_ReportError("Dot11s_ComputeLinkMetric: "
                "Invalid active path metric");
            break;
        }
    }
    return linkMetric;
}


// ------------------------------------------------------------------
