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

/*!
 * \file mac_dot11-pc.cpp
 * \brief Point controller and PCF access routines.
 *
 * \bug Disabled for now. Beaconing is broken.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "network_ip.h"
#include "mac_dot11-sta.h"
#include "mac_dot11-ap.h"
#include "mac_dot11-pc.h"
#include "mac_dot11-mgmt.h"


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpIsInTransmitState
//  PURPOSE:     Determine if currently in CFP transmit state.
//               Used for asserts when receiving frames.
//
//  PARAMETERS:  DOT11_CfpStates state
//                  Current CFP state
//  RETURN:      TRUE if in tranmit state
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11CfpIsInTransmitState(
    DOT11_CfpStates state)
{
    return state >= DOT11_X_CFP_BROADCAST
        && state <= DOT11_X_CFP_END;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11IsCfFrameAnAckType
//  PURPOSE:     Determine if the CF frame is one of those which
//               acknowledges previously sent data e.g. CF_DATA_ACK
//               or CF_END_ACK.
//
//  PARAMETERS:  DOT11_MacFrameType frameType
//                  Frame type
//  RETURN:      TRUE if frame type has a piggy-back ack
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11IsCfFrameAnAckType(
    DOT11_MacFrameType frameType)
{
    BOOL isAckType = FALSE;

    switch (frameType) {
        // This check could use the bit pattern in frame type.
        case DOT11_CF_DATA_ACK:
        case DOT11_CF_DATA_POLL_ACK:
        case DOT11_CF_ACK:
        case DOT11_CF_POLL_ACK:
        case DOT11_CF_END_ACK:
            isAckType = TRUE;
            break;

        default:
            break;
    } //switch

    return isAckType;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpAdjustFrameTypeForAck
//  PURPOSE:     Adjust the PCF frame type in case an ack is to
//               piggy-back on it.
//  PARAMETERS:  const MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType * const frameType
//                  Frame type
//  RETURN:      Adjusted frame type
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpAdjustFrameTypeForAck(
    const MacDataDot11* const dot11,
    DOT11_MacFrameType* const frameType)
{
    if (*frameType == DOT11_CF_NONE) {
        return;
    }

    if (dot11->cfpAckIsDue) {
        // Could use bit patterns of frame types (*frameType++)
        //
        switch (*frameType) {

            case DOT11_DATA:
                *frameType = DOT11_CF_DATA_ACK;
                break;

            case DOT11_CF_DATA_POLL:
                *frameType = DOT11_CF_DATA_POLL_ACK;
                break;

            case DOT11_CF_POLL:
                *frameType = DOT11_CF_POLL_ACK;
                break;

            case DOT11_CF_END:
                *frameType = DOT11_CF_END_ACK;
                break;

            default:
                break;

        } //switch
    } //if
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcSetTransmitState
//  PURPOSE:     Set the state of the PC based on the frame about to
//               be transmitted during CFP.
//  PARAMETERS:  const MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType * const frameType
//                  Frame type
//               Mac802Address destAddr
//                  Destination address
//  RETURN:      Adjusted frame type
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcSetTransmitState(
     MacDataDot11* const dot11,
    DOT11_MacFrameType frameType,
    Mac802Address destAddr)
{
    dot11->cfpLastFrameType = frameType;

    switch (frameType) {

        case DOT11_DATA:
        case DOT11_CF_DATA_ACK:
            if (destAddr == ANY_MAC802) {
                MacDot11CfpSetState(dot11, DOT11_X_CFP_BROADCAST);
            }
            else {
                MacDot11CfpSetState(dot11, DOT11_X_CFP_UNICAST);
            }
            break;

        case DOT11_CF_DATA_POLL:
        case DOT11_CF_DATA_POLL_ACK:
            MacDot11CfpSetState(dot11, DOT11_X_CFP_UNICAST_POLL);
            break;

        case DOT11_CF_POLL:
        case DOT11_CF_POLL_ACK:
            MacDot11CfpSetState(dot11, DOT11_X_CFP_POLL);
            break;

        case DOT11_CF_END:
        case DOT11_CF_END_ACK:
            MacDot11CfpSetState(dot11, DOT11_X_CFP_END);
            break;

        case DOT11_ACK:
            // To cater to relay
            MacDot11CfpSetState(dot11, DOT11_X_CFP_ACK);
            break;

        default:
            ERROR_ReportError("MacDot11CfpPcSetTransmitState: "
                "Unknown frame type.\n");
            break;

    } //switch
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcSetPostTransmitState
//  PURPOSE:     Set that state of PC during CFP after frame
//               transmission is complete.
//
//               Also reset ack flag is the sent frame was an ack type.
//
//  PARAMETERS:  MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcSetPostTransmitState(
     MacDataDot11* const dot11)
{
    switch (dot11->cfpState) {

        case DOT11_X_CFP_BROADCAST:
            MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
            break;

        case DOT11_X_CFP_BEACON:
            MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
            break;

        case DOT11_X_CFP_UNICAST:
            MacDot11CfpSetState(dot11, DOT11_S_CFP_WF_ACK);
            break;

        case DOT11_X_CFP_UNICAST_POLL:
            MacDot11CfpSetState(dot11, DOT11_S_CFP_WF_DATA_ACK);
            break;

        case DOT11_X_CFP_POLL:
            MacDot11CfpSetState(dot11, DOT11_S_CFP_WF_DATA);
            break;

        case DOT11_X_CFP_END:
            MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
            break;

        case DOT11_X_CFP_ACK:
            MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
            break;

        default:
            ERROR_ReportError("MacDot11CfpPcSetPostTransmitState: "
                "Unknown transmit state.\n");
            break;

    } //switch

    switch (dot11->cfpLastFrameType) {
        case DOT11_CF_DATA_ACK:
        case DOT11_CF_DATA_POLL_ACK:
        case DOT11_CF_POLL_ACK:
        case DOT11_CF_END_ACK:
            dot11->cfpAckIsDue = FALSE;
            break;
        default:
            break;
    } //switch
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationSetTransmitState
//  PURPOSE:     Set the state of the BSS station based on the frame
//               about to be transmitted during CFP.
//
//  PARAMETERS:  Frame type
//               Destination address
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationSetTransmitState(
     MacDataDot11* const dot11,
    DOT11_MacFrameType frameType)
{
    dot11->cfpLastFrameType = frameType;

    switch (frameType) {

        case DOT11_DATA:
        case DOT11_CF_DATA_ACK:
            MacDot11CfpSetState(dot11, DOT11_X_CFP_UNICAST);
            break;

        case DOT11_CF_NULLDATA:
            MacDot11CfpSetState(dot11, DOT11_X_CFP_NULLDATA);
            break;

        case DOT11_CF_ACK:
            MacDot11CfpSetState(dot11, DOT11_X_CFP_ACK);
            break;

        default:
            ERROR_ReportError("MacDot11CfpStationSetTransmitState: "
                "Unknown frame type.\n");
            break;

    } //switch
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationSetPostTransmitState
//  PURPOSE:     Set that state of BSS station during CFP after frame
//               transmission is complete.
//
//               Also reset ack flag is the sent frame was an ack type.
//
//  PARAMETERS:   MacDataDot11 * const dot11
//                  Pointer to Dot11 strcuture
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationSetPostTransmitState(
     MacDataDot11* const dot11)
{
    switch (dot11->cfpState) {

        case DOT11_X_CFP_UNICAST:
            MacDot11CfpSetState(dot11, DOT11_S_CFP_WF_ACK);
            break;

        case DOT11_X_CFP_NULLDATA:
        case DOT11_X_CFP_ACK:
            MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
            break;

        default:
            ERROR_ReportError("MacDot11CfpStationSetPostTransmitState: "
                "Unknown transmit state.\n");
            break;

    } //switch

    switch (dot11->cfpLastFrameType) {
        case DOT11_CF_DATA_ACK:
        case DOT11_CF_ACK:
            dot11->cfpAckIsDue = FALSE;
            break;
        default:
            break;
    } //switch
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfPollListInit
//  PURPOSE:     Initialize the poll link list data structure.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_CfPollList **list
//                  Pointer to polling list
//  RETURN:      Poll list with memory allocated
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfPollListInit(
    Node* node,
     MacDataDot11* dot11,
    DOT11_CfPollList** list)
{
    DOT11_CfPollList* tmpList;

    tmpList = (DOT11_CfPollList*) MEM_malloc(sizeof(DOT11_CfPollList));

    tmpList->size = 0;
    tmpList->first = tmpList->last = NULL;
    *list = tmpList;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfPollListEmpty
//  PURPOSE:     See if the list is empty.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_CfPollList* list
//                  Pointer to polling list
//  RETURN:      TRUE if size is zero
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11CfPollListIsEmpty(
    Node* node,
     MacDataDot11* dot11,
    DOT11_CfPollList* list)
{
    return (list->size == 0);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfPollListGetSize
//  PURPOSE:     Query the size of poll list
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      Number of items in poll list
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
int MacDot11CfPollListGetSize(
    Node* node,
     MacDataDot11* dot11)
{
    return dot11->pcVars->cfPollList->size;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfPollListInsert
//  PURPOSE:     Insert an item to the end of the list.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_CfPollList* list
//                  Pointer to polling list
//               DOT11_CfPollStation* data
//                  Poll station data for item to insert
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfPollListInsert(
    Node* node,
     MacDataDot11* dot11,
    DOT11_CfPollList* list,
    DOT11_CfPollStation* data)
{
    DOT11_CfPollListItem* listItem =
        (DOT11_CfPollListItem*)
        MEM_malloc(sizeof(DOT11_CfPollListItem));

    listItem->data = data;

    listItem->next = NULL;

    if (list->size == 0) {
        // Only item in the list
        listItem->prev = NULL;
        list->last = listItem;
        list->first = list->last;
    }
    else {
        // Insert at end of list
        listItem->prev = list->last;
        list->last->next = listItem;
        list->last = listItem;
    }

    list->size++;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfPollListItemRemove
//  PURPOSE:     Remove an item from the poll list.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_CfPollList* list
//                  Pointer to polling list
//               DOT11_CfPollListItem* listItem
//                  List item to remove
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11CfPollListItemRemove(
    Node* node,
     MacDataDot11* dot11,
    DOT11_CfPollList* list,
    DOT11_CfPollListItem* listItem)
{
    DOT11_CfPollListItem* nextListItem;

    if (list == NULL || listItem == NULL) {
        return;
    }

    if (list->size == 0) {
        return;
    }

    nextListItem = listItem->next;

    if (list->size == 1) {
        list->first = list->last = NULL;
    }
    else if (list->first == listItem) {
        list->first = listItem->next;
        list->first->prev = NULL;
    }
    else if (list->last == listItem) {
        list->last = listItem->prev;
        if (list->last != NULL) {
            list->last->next = NULL;
        }
    }
    else {
        listItem->prev->next = listItem->next;
        if (listItem->prev->next != NULL) {
            listItem->next->prev = listItem->prev;
        }
    }

    list->size--;

    if (listItem->data != NULL) {
        MEM_free(listItem->data);
    }

    MEM_free(listItem);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfPollListFree
//  PURPOSE:     Free the entire list.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_CfPollList* list
//                  Pointer to polling list
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfPollListFree(
    Node* node,
     MacDataDot11* dot11,
    DOT11_CfPollList* list)
{
    DOT11_CfPollListItem *item;
    DOT11_CfPollListItem *tempItem;

    item = list->first;

    while (item) {
        tempItem = item;
        item = item->next;

        MEM_free(tempItem->data);
        MEM_free(tempItem);
    }

    if (list != NULL) {
        MEM_free(list);
    }

    list = NULL;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfPollListGetFirst
//  PURPOSE:     Retrieve the first item in the poll list
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      First list item
//               NULL for empty list
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
DOT11_CfPollListItem * MacDot11CfPollListGetFirst(
    Node* node,
     MacDataDot11* dot11)
{
    return dot11->pcVars->cfPollList->first;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfPollListGetNextItem
//  PURPOSE:     Retrieve the next item given the previous item.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_CfPollListItem *prev
//                  Pointer to prev item.
//  RETURN:      First item when previous item is NULL
//                          when previous item is the last
//                          when there is only one item in the list
//               NULL if the list is empty
//               Next item after previous item
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
DOT11_CfPollListItem * MacDot11CfPollListGetNextItem(
    Node* node,
     MacDataDot11* dot11,
    DOT11_CfPollListItem *prev)
{
    DOT11_CfPollListItem* next = NULL;
    DOT11_CfPollList* pollList = dot11->pcVars->cfPollList;

    if (!MacDot11CfPollListIsEmpty(node, dot11, pollList)) {
        if (prev == pollList->last || prev == NULL) {
            next = pollList->first;
        }
        else {
            next = prev->next;
        }
    }

    return next;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfPollListResetAtStartOfCfp
//  PURPOSE:     Reset values of the poll list at start of CFP
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfPollListResetAtStartOfCfp(
    Node* node,
     MacDataDot11* dot11)
{
    DOT11_PcVars* pc = dot11->pcVars;
    DOT11_CfPollListItem* next = NULL;
    DOT11_CfPollListItem* first;

    ERROR_Assert(MacDot11IsPC(dot11) && MacDot11CfpPcSendsPolls(pc),
        "MacDot11CfPollListResetAtStartOfCfp: "
        "Poll list not applicable to PC configuration.\n");

    if (!MacDot11CfPollListIsEmpty(node, dot11, pc->cfPollList)) {
        // It is simpler to reset all items
        next = first = MacDot11CfPollListGetFirst(node, dot11);

        do {
            next->data->flags = 0;
            next = MacDot11CfPollListGetNextItem(node, dot11, next);

        } while (next != first);

    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11CfPollListGetItemWithGivenAddress
//  PURPOSE:     Retrieve the list item related to given address
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address lookupAddress
//                  Address to lookup
//  RETURN:      List item corresponding to address
//               NULL otherwise
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
DOT11_CfPollListItem* MacDot11CfPollListGetItemWithGivenAddress(
    Node* node,
     MacDataDot11* dot11,
    Mac802Address lookupAddress)
{
    DOT11_PcVars* pc = dot11->pcVars;
    DOT11_CfPollListItem* next = NULL;
    DOT11_CfPollListItem* first;

    if (!MacDot11CfPollListIsEmpty(node, dot11, pc->cfPollList)) {
        next = first = MacDot11CfPollListGetFirst(node, dot11);

        do {
            if (next->data->macAddr == lookupAddress) {
                return next;
            }
            next = MacDot11CfPollListGetNextItem(node, dot11, next);

        } while (next != first);

    }

    return NULL;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfPollListIsAddressPollable
//  PURPOSE:     Determine if given address is pollable
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address address,
//                  Node address
//  RETURN:      TRUE if address is found and is pollable
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11CfPollListIsAddressPollable(
    Node* node,
     MacDataDot11* dot11,
    Mac802Address address)
{
    DOT11_PcVars* pc = dot11->pcVars;
    BOOL isPollable = FALSE;
    DOT11_CfPollListItem* item = NULL;

    ERROR_Assert(MacDot11IsPC(dot11) && MacDot11CfpPcSendsPolls(pc),
        "MacDot11CfPollListIsAddressPollable: "
        "Poll list not applicable to PC configuration.\n");

    if (address != ANY_MAC802) {
        item = MacDot11CfPollListGetItemWithGivenAddress(node,
                    dot11, address);

        if (item != NULL
            && item->data->pollType == DOT11_STA_POLLABLE_POLL)
        {
            isPollable = TRUE;
        }
    }
    return isPollable;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11CfPollListUpdateForUnicastReceived
//  PURPOSE:     Update poll list when PC receives a unicast from BSS
//               station.
//
//               When poll save is by count, reset nulldata counter.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address sourceAddr
//                  Node address of unicast sender station
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11CfPollListUpdateForUnicastReceived(
    Node* node,
     MacDataDot11* dot11,
    Mac802Address sourceAddr)
{
    DOT11_PcVars* pc = dot11->pcVars;
    DOT11_CfPollListItem* matchedListItem;

    if (!(MacDot11IsPC(dot11) && MacDot11CfpPcSendsPolls(pc))) {
        return;
    }

    if (pc->pollSaveMode == DOT11_POLL_SAVE_BYCOUNT) {
        matchedListItem = MacDot11CfPollListGetItemWithGivenAddress(
            node, dot11, sourceAddr);

        if (matchedListItem) {
            matchedListItem->data->nullDataCount = 0;
        }
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfPollListUpdateForNullDataResponse
//  PURPOSE:     Update poll list when station responds with null data
//               and also when PC timeouts in response to a poll.
//
//               When poll save is by count, increment counter upto
//               a max value.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address address,
//                  Node address of station
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfPollListUpdateForNullDataResponse(
    Node* node,
     MacDataDot11* dot11,
    Mac802Address address)
{
    DOT11_PcVars* pc = dot11->pcVars;
    DOT11_CfPollListItem* matchedListItem;

    ERROR_Assert(MacDot11IsPC(dot11) && MacDot11CfpPcSendsPolls(pc),
        "MacDot11CfPollListUpdateForNullDataResponse: "
        "Poll list not applicable to PC configuration.\n");

    if (pc->pollSaveMode == DOT11_POLL_SAVE_BYCOUNT) {
        matchedListItem = MacDot11CfPollListGetItemWithGivenAddress(
            node, dot11, address);

        // Increment nulldata count upto a max value
        if (matchedListItem
            && matchedListItem->data->nullDataCount
                < pc->pollSaveMaxCount)
        {
            matchedListItem->data->nullDataCount++;
        }
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfPcStationListAddStation
//  PURPOSE:     Add a station to list of given address
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address stationAddress
//                  Address of ststion to add
//  RETURN:      association id of station
//               NULL otherwise
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
 void MacDot11cfPcPollListAddStation(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address stationAddress)
            {
    //NodeId      sourceNodeId;
    //DOT11_CfPollStation pollStation;

    DOT11_PcVars* pc = dot11->pcVars;
                DOT11_CfPollStation *aStation = (DOT11_CfPollStation*)
                    MEM_malloc(sizeof(DOT11_CfPollStation));

    aStation->nullDataCount = 0;
    aStation->pollCount = 0;

    // Set Poll Type
    if (dot11->cfPollable == 0 &&    dot11->cfPollRequest == 0 )
    {
        aStation->pollType = DOT11_STA_NOT_POLLABLE ;
            }

    if (dot11->cfPollable == 1 &&    dot11->cfPollRequest == 0 )
    {
    aStation->pollType = DOT11_STA_POLLABLE_DONT_POLL;
        }

    if (dot11->cfPollable == 1 &&    dot11->cfPollRequest == 1 )
    {
    aStation->pollType = DOT11_STA_POLLABLE_POLL;
    }

//---------------------DOT11e--Updates------------------------------------//
    aStation->flags = 0;
//--------------------DOT11e-End-Updates---------------------------------//
    aStation->macAddr = stationAddress;
          if (aStation->pollType == DOT11_STA_POLLABLE_POLL)
            {

                MacDot11CfPollListInsert(node, dot11,
                    pc->cfPollList, aStation);
            pc->pollableCount++;
            }
     aStation->assocId = pc->pollableCount;

}//  MacDot11cfpPcPollListAddStation

//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcPollSaveInit
//  PURPOSE:     Initialize PC poll save mode values from user configuration
//  PARAMETERS:  Node* node
//                  Pointer to node
//               const NodeInput* nodeInput
//                  Pointer to node input
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcPollSaveInit(
    Node* node,
    const NodeInput* nodeInput,
     MacDataDot11* dot11,
     NetworkType networkType)
{
    DOT11_PcVars* pc = dot11->pcVars;
    char input[MAX_STRING_LENGTH];
    BOOL wasFound = FALSE;
    int pollSaveMin, pollSaveMax;
    Address address;

    // Read PC poll save mode.
    // Format is :
    //       MAC-DOT11-PC-POLL-SAVE NONE | BY-COUNT
    // Default is DOT11_POLL_SAVE_MODE_DEFAULT
    NetworkGetInterfaceInfo(
                node,
                dot11->myMacData->interfaceIndex,
                &address,
                networkType);

    IO_ReadString(
         node,
         node->nodeId,
         dot11->myMacData->interfaceIndex,
         nodeInput,
         "MAC-DOT11-PC-POLL-SAVE",
         &wasFound,
         input);

    if (wasFound == TRUE)
    {
        if (!strcmp(input, "NONE"))
        {
            pc->pollSaveMode = DOT11_POLL_SAVE_NONE;
        }
        else if (!strcmp(input, "BY-COUNT"))
        {
            pc->pollSaveMode = DOT11_POLL_SAVE_BYCOUNT;
        }
        else
        {
            ERROR_ReportError("MacDot11CfpPcPollSaveInit: "
                "Invalid value for MAC-DOT11-PC-POLL-SAVE "
                "in configuration file.\n"
                "Expecting NONE or BY-COUNT\n");
        }
    }
    else
    {
        pc->pollSaveMode = DOT11_POLL_SAVE_MODE_DEFAULT;
    }

    if (pc->pollSaveMode == DOT11_POLL_SAVE_BYCOUNT)
    {
        // Read min threshold for the BY-COUNT PC poll save mode.
        // Format is :
        //      MAC-DOT11-PC-POLL-SAVE-MIN 1
        // Default is DOT11_POLL_SAVE_MIN_COUNT_DEFAULT
        IO_ReadInt(
            node,
            node->nodeId,
            dot11->myMacData->interfaceIndex,
            nodeInput,
            "MAC-DOT11-PC-POLL-SAVE-MIN",
            &wasFound,
            &pollSaveMin);

        if (wasFound) {
            ERROR_Assert(pollSaveMin >= 0,
                "MacDot11CfpPcPollSaveInit: "
                "MAC-DOT11-PC-POLL-SAVE-MIN in "
                "configuration file is not positive.\n");
            pc->pollSaveMinCount = pollSaveMin;
        }
        else
        {
            pc->pollSaveMinCount = DOT11_POLL_SAVE_MIN_COUNT_DEFAULT;
        }

        // Read max threshold for the BY-COUNT PC poll save mode.
        // Format is :
        //       MAC-DOT11-PC-POLL-SAVE-MAX 10
        //  Default is DOT11_POLL_SAVE_MAX_COUNT_DEFAULT
        IO_ReadInt(
            node,
            node->nodeId,
            dot11->myMacData->interfaceIndex,
            nodeInput,
            "MAC-DOT11-PC-POLL-SAVE-MAX",
            &wasFound,
            &pollSaveMax);

        if (wasFound) {
            ERROR_Assert(pollSaveMax >= 0,
                "MacDot11CfpPcPollSaveInit: "
                "MAC-DOT11-PC-POLL-SAVE-MAX in "
                "configuration file is not a positive value.\n");
            pc->pollSaveMaxCount = pollSaveMax;
        }
        else {
            pc->pollSaveMaxCount = DOT11_POLL_SAVE_MAX_COUNT_DEFAULT;
        }

        ERROR_Assert(pc->pollSaveMinCount < pc->pollSaveMaxCount,
            "MacDot11CfpPcPollSaveInit: "
            "MAC-DOT11-PC-POLL-SAVE-MAX is <= "
            "MAC-DOT11-PC-POLL-SAVE-MIN.\n");
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpDurationInit
//  PURPOSE:     Read the user specified duration of the CFP.
//  PARAMETERS:  Node* node
//                  Pointer to current node
//               const NodeInput* nodeInput
//                  Pointer to node input
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               clocktype* cfpMaxDuration
//                  Read or default value of max. duration.
//               NetworkType networkType
//                  network Type.
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpDurationInit(
    Node* node,
    const NodeInput* nodeInput,
     MacDataDot11* dot11,
    clocktype* cfpMaxDuration,
    int* cfpPeriod,
    NetworkType networkType)

{
    BOOL wasFound = FALSE;
    int cfpDuration;
    int cfpPeriodvalue;
    Address address;

    // The contention free duration is specified in TUs (Time units)
    // where each time unit is 1024US
    //    MAC-DOT11-PC-CF-REPETITION-INTERVAL 3
    //    [0.6] MAC-DOT11-PC-CF-DURATION 50
    // and should be in the range 1 to DOT11_BEACON_INTERVAL_MAX TUs
    // Default is DOT11_CFP_MAX_DURATION_DEFAULT
    //
    NetworkGetInterfaceInfo(
        node,
        dot11->myMacData->interfaceIndex,
        &address,
        networkType);

    IO_ReadInt(
        node,
        //MAPPING_GetNodeIdFromInterfaceAddress(node, dot11->bssAddr),
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11-PC-CF-REPETITION-INTERVAL",
        &wasFound,
        &cfpPeriodvalue);

    if (wasFound)
    {
        ERROR_Assert(cfpPeriodvalue> 0
            && cfpPeriodvalue < DOT11_CFP_REPETITION_INTERVAL_MAX ,
            "MacDot11CfpPcInit: "
            "Out of range value for MAC-DOT11-PC-CF-REPETITION-INTERVAL  "
            "in configuration file.\n");
    }
    else
    {
        cfpPeriodvalue = DOT11_CFP_REPETITION_INTERVAL_DEFAULT ;
    }
        *cfpPeriod = cfpPeriodvalue;

        dot11->cfpRepetionInterval = ( (dot11->beaconInterval)
                                 *(cfpPeriodvalue)*(dot11->DTIMperiod));
    IO_ReadInt(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11-PC-CF-DURATION",
        &wasFound,
        &cfpDuration);

    if (wasFound)
    {
        ERROR_Assert(cfpDuration > 0
              && cfpDuration < dot11->cfpRepetionInterval,
            "MacDot11CfpPcInit: "
            "Out of range value for MAC-DOT11-PC-CF-DURATION "
            "in configuration file.\n");
    }
    else
    {
        cfpDuration = DOT11_CFP_MAX_DURATION_DEFAULT;
    }

    *cfpMaxDuration = MacDot11TUsToClocktype((unsigned short) cfpDuration);
    return;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcInit
//  PURPOSE:     Initialize Point Coordinator related user configuration.
//  PARAMETERS:  Node* node
//                  Pointer to current node
//               const NodeInput* nodeInput
//                  Pointer to node input
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               SubnetMemberData* subnetList
//                  Pointer to subnet list
//               int nodesInSubnet
//                  Number of nodes in subnet
//               int subnetListIndex
//                  Subnet list index
//               NodeAddress subnetAddress
//                  Subnet address
//               int numHostBits
//                  Number of host bits
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcInit(
    Node* node,
    const NodeInput* nodeInput,
     MacDataDot11* dot11,
    SubnetMemberData* subnetList,
    int nodesInSubnet,
    int subnetListIndex,
    NodeAddress subnetAddress,
    int numHostBits,
    NetworkType networkType = NETWORK_IPV4,
    in6_addr* ipv6SubnetAddr = 0,
    unsigned int prefixLength = 0)
{
    DOT11_PcVars* pc = dot11->pcVars;
    char input[MAX_STRING_LENGTH];
    BOOL wasFound = FALSE;
    clocktype cfpDuration;
    Address address;

    // A PC may operate as
    // send only where
    //      it does not poll stations but only delivers queued data
    // or as poll-only where
    //      buffered data may piggyback on the polls
    // or as poll+deliver where
    //      the poll only mode is followed by send only
    //
    // MAC-DOT11-PC-DELIVERY-MODE DELIVER-ONLY
    //                             | POLL-ONLY
    //                             | POLL-AND-DELIVER
    // Default is DOT11_PC_DELIVERY_MODE_DEFAULT

    NetworkGetInterfaceInfo(
        node,
        dot11->myMacData->interfaceIndex,
        &address,
        networkType);

    IO_ReadString(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11-PC-DELIVERY-MODE",
        &wasFound,
        input);

    if (wasFound) {
        if (!strcmp(input, "DELIVER-ONLY"))
        {
            pc->deliveryMode = DOT11_PC_DELIVER_ONLY;
        }
        else if (!strcmp(input, "POLL-AND-DELIVER"))
        {
            pc->deliveryMode = DOT11_PC_POLL_AND_DELIVER;
        }
        else if (!strcmp(input, "POLL-ONLY"))
        {
            pc->deliveryMode = DOT11_PC_POLL_ONLY;
        }
        else
        {
            ERROR_ReportError("MacDot11CfpPcInit: "
                "Invalid value for MAC-DOT11-PC-DELIVERY-MODE "
                "in configuration file.\n "
                "Expecting DELIVER-ONLY | POLL-ONLY | POLL-AND-DELIVER.\n");
        }
    }
    else
    {
        pc->deliveryMode = DOT11_PC_DELIVERY_MODE_DEFAULT;
    }
    dot11->printCfpStatistics = DOT11_PRINT_PC_STATS_DEFAULT ;

     if (MacDot11CfpPcSendsPolls(pc))
    {
         MacDot11CfPollListInit(node, dot11, &pc->cfPollList);
        MacDot11CfpPcPollSaveInit(node, nodeInput, dot11, networkType);
    }

    // Read in the contention free duration, e.g.
    //    [0.6] MAC-DOT11-PC-CF-DURATION 50
    //
    MacDot11CfpDurationInit(node, nodeInput, dot11,
                        &dot11->cfpMaxDuration,
                        &dot11->cfpPeriod , networkType);

    // The duration should be smaller than the beacon interval
    // to allow at least one data frame to be sent during the
    // contention period. Note that min BO is added here.
    //
    cfpDuration =
        (dot11->extraPropDelay + dot11->difs
         + dot11->cwMin * dot11->slotTime
         + PHY_GetTransmissionDuration(node, dot11->myMacData->phyNumber,
                                       dot11->lowestDataRateType,
                                       DOT11_LONG_CTRL_FRAME_SIZE)
         + dot11->extraPropDelay + dot11->sifs
         + PHY_GetTransmissionDuration(node, dot11->myMacData->phyNumber,
                                       dot11->lowestDataRateType,
                                       DOT11_SHORT_CTRL_FRAME_SIZE)
         + dot11->extraPropDelay + dot11->sifs
         + PHY_GetTransmissionDuration(node, dot11->myMacData->phyNumber,
                                       dot11->lowestDataRateType,
                                       MAX_NW_PKT_SIZE)
         + dot11->extraPropDelay + dot11->sifs
         + PHY_GetTransmissionDuration(node, dot11->myMacData->phyNumber,
                                       dot11->lowestDataRateType,
                                       DOT11_SHORT_CTRL_FRAME_SIZE)
         + dot11->extraPropDelay);
    ERROR_Assert(
        dot11->cfpRepetionInterval - dot11->cfpMaxDuration >= cfpDuration,
        "MacDot11CfpPcInit: "
        "The contention free duration does not allow "
        "enough time for the contention period.\n"
        "Either reduce the value of MAC-DOT11-PC-CF-DURATION\n"
        "or increase the beacon interval (MAC-DOT11-BEACON-INTERVAL).\n");

    // The duration should be large enough to allow 2 full
    // frames plus time for a beacon and CF-End
    // The duration should be large enough to allow 2 full frames
    // plus time for a beacon and CF-End
    cfpDuration =
       (dot11->pifs + dot11->extraPropDelay
        + PHY_GetTransmissionDuration(
            node, dot11->myMacData->phyNumber,
            dot11->lowestDataRateType,
            sizeof(DOT11_BeaconFrame))
        + dot11->sifs
        + 2 * (dot11->extraPropDelay
               + PHY_GetTransmissionDuration(
                   node, dot11->myMacData->phyNumber,
                   dot11->lowestDataRateType,
                   MAX_NW_PKT_SIZE)
               + dot11->sifs
               + dot11->extraPropDelay)
        + PHY_GetTransmissionDuration(
            node, dot11->myMacData->phyNumber,
            dot11->lowestDataRateType,
            DOT11_LONG_CTRL_FRAME_SIZE)
        + dot11->extraPropDelay);

    ERROR_Assert(dot11->cfpMaxDuration > cfpDuration,
        "MacDot11CfpPcInit: "
        "The contention free duration is too small for packet exchange.\n"
        "Increase the value of MAC-DOT11-PC-CF-DURATION.\n");

    dot11->pcVars->lastCfpCount = 0;
    dot11->pcVars->cfSet.cfpCount = 0;
    dot11->pcVars->cfSet.cfpPeriod = dot11->cfpPeriod;
    dot11->pcVars->cfSet.cfpMaxDuration = MacDot11ClocktypeToTUs(dot11->cfpMaxDuration);
    dot11->pcVars->cfSet.cfpBalDuration = MacDot11ClocktypeToTUs(dot11->cfpMaxDuration);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationInit
//  PURPOSE:     Initialize Station related values from user configuration.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               const NodeInput* nodeInput
//                  Pointer to node input
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationInit(
    Node* node,
    const NodeInput* nodeInput,
     MacDataDot11* dot11,
     NetworkType networkType)
{
    // For static association, read the contention free duration of the PC
    //    [0.6] MAC-DOT11-PC-CF-DURATION 50
    //
    MacDot11CfpDurationInit(node, nodeInput, dot11, &dot11->cfpMaxDuration ,
        &dot11->cfpPeriod, networkType);
}



//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcPrintStats
//  PURPOSE:     Print CFP statistics for a PC.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int interfaceIndex
//                  Interface index
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcPrintStats(
    Node* node,
     MacDataDot11* dot11,
    int interfaceIndex)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "Unicasts sent and acked = %d",
           dot11->unicastPacketsSentCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Broadcasts sent = %d",
           dot11->broadcastPacketsSentCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Unicasts received = %d",
           dot11->unicastPacketsGotCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Broadcasts received = %d",
           dot11->broadcastPacketsGotCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Data-Polls transmitted = %d",
           dot11->dataPollPacketsSentCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Polls transmitted = %d",
           dot11->pollPacketsSentCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Data packets transmitted = %d",
           dot11->dataPacketsSentCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Data packets received = %d",
           dot11->dataPacketsGotCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "NullData received = %d",
           dot11->nulldataPacketsGotCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Beacons transmitted = %d",
           dot11->beaconPacketsSentCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "CF Ends transmitted = %d",
           dot11->endPacketsSentCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Beacons received = %d",
           dot11->beaconPacketsGotCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "CF Ends received = %d",
           dot11->endPacketsGotCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    ERROR_Assert(
        dot11->nulldataPacketsSentCfp == 0 &&
        dot11->dataPollPacketsGotCfp == 0 &&
        dot11->pollPacketsGotCfp == 0,
        "MacDot11PrintStatsPc: "
        "One or more stat values is unexpectedly non-zero.\n");
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationPrintStats
//  PURPOSE:     Print CFP statistics for a station in BSS.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               int interfaceIndex
//                  Interface index
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationPrintStats(
    Node* node,
     MacDataDot11* dot11,
    int interfaceIndex)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "Unicasts sent and acked = %d",
           dot11->unicastPacketsSentCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Unicasts received = %d",
           dot11->unicastPacketsGotCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Broadcasts received = %d",
           dot11->broadcastPacketsGotCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Data packets transmitted = %d",
           dot11->dataPacketsSentCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "NullData transmitted = %d",
           dot11->nulldataPacketsSentCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Data-Polls received = %d",
           dot11->dataPollPacketsGotCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Polls received = %d",
           dot11->pollPacketsGotCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Data packets received = %d",
           dot11->dataPacketsGotCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Beacons received = %d",
           dot11->beaconPacketsGotCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "CF Ends received = %d",
           dot11->endPacketsGotCfp);
    IO_PrintStat(node, "MAC", "DOT11-PCF", ANY_DEST, interfaceIndex, buf);

}



//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcGetTxDataRate
//  PURPOSE:     Determine transmit data rate based on frame type,
//               destination address and ack destination (if ack is due).
//  PARAMETERS:  Node* node
//                  Pointer to node
//               const  MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Destination address of frame
//               DOT11_MacFrameType frameToSend
//                  Frame type to send
//               int* dataRate
//                  Data rate to use for transmit.
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcGetTxDataRate(
    Node* node,
     MacDataDot11* dot11,
    Mac802Address destAddr,
    DOT11_MacFrameType frameToSend,
    int* dataRateType)
{
    DOT11_PcVars* pc = dot11->pcVars;
    DOT11_DataRateEntry* dataRateEntry;

    *dataRateType = dot11->lowestDataRateType;

    if (frameToSend == DOT11_CF_NONE) {
        return;
    }

    switch (frameToSend) {

        case DOT11_DATA:
        case DOT11_CF_DATA_POLL:
            dot11->dataRateInfo =
                MacDot11StationGetDataRateEntry(node, dot11, destAddr);
            *dataRateType = dot11->dataRateInfo->dataRateType;
            break;

        case DOT11_CF_POLL:
            dataRateEntry =
                MacDot11StationGetDataRateEntry(node, dot11, destAddr);
            *dataRateType = dataRateEntry->dataRateType;
            break;

        case DOT11_CF_END:
            *dataRateType = dot11->highestDataRateTypeForBC;
            break;

        case DOT11_CF_NULLDATA:
        case DOT11_CF_ACK:
        default:
            ERROR_ReportError("MacDot11CfpPcSetDataRateForFrameType: "
                "Unknown or unexpected frame type.\n");
            break;
    }

    if (dot11->cfpAckIsDue) {
        // It may occur that the destination for Ack is
        // also the destination for the current frame and
        // the current data rate is higher.
        // A conservative approach is taken here.
        *dataRateType = MIN(*dataRateType, pc->dataRateTypeForAck);
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpSetEndVariables
//  PURPOSE:     Set states and variables when ending CFP.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpSetEndVariables(
    Node* node,
     MacDataDot11* dot11)
{
    MacDot11StationCancelTimer(node, dot11);
    dot11->cfpEndSequenceNumber = 0;

    // The variables were set when beacon was received.
    // Repeated here in case beacon was missed.
    MacDot11CfpResetBackoffAndAckVariables(node, dot11);

    MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
    MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcHasSufficientTime
//  PURPOSE:     Determine if PC has sufficient time to send a frame type.
//               The frame type determines the frame exchange sequence
//               that would occur. If time was insufficient, the PC would
//               attempt to send a CF-End to terminate the CFP.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame types
//               clocktype sendDelay
//                  Delay before sending the frame
//               Mac802Address destAddr
//                  Destination address of frame type
//  RETURN:      TRUE if there is sufficient time in CFP to send the frame
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11CfpPcHasSufficientTime(
    Node* node,
     MacDataDot11* dot11,
    DOT11_MacFrameType frameType,
    clocktype sendDelay,
    Mac802Address destAddr,
    clocktype cfpBalanceDuration,
    clocktype cfEndTime,
    int dataRateType)
{
    clocktype cfpRequiredDuration = 0;
    clocktype currentMessageDuration;
    int dataRateTypeForResponse;

    dataRateTypeForResponse = dataRateType;

    switch (frameType) {
        case DOT11_DATA:
        case DOT11_CF_DATA_POLL:
            currentMessageDuration =
                PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    dataRateType,
                    MESSAGE_ReturnPacketSize(dot11->currentMessage) +
                    DOT11_DATA_FRAME_HDR_SIZE);

            if (destAddr == ANY_MAC802) {
                // Broadcast needs sendDelay + msg time + SIFS + CfEnd
                cfpRequiredDuration =
                    sendDelay
                    + currentMessageDuration
                    + dot11->extraPropDelay
                    + dot11->sifs
                    + cfEndTime;
            }
            else {
                // Unicast needs
                //  sendDelay + msg time + SIFS + ack + SIFS + CfEnd
                cfpRequiredDuration =
                    sendDelay
                    + currentMessageDuration
                    + dot11->extraPropDelay
                    + dot11->sifs
                    + PHY_GetTransmissionDuration(
                        node, dot11->myMacData->phyNumber,
                        dataRateTypeForResponse,
                        DOT11_DATA_FRAME_HDR_SIZE)
                    + dot11->extraPropDelay
                    + dot11->sifs
                    + cfEndTime;
            }
            break;

        case DOT11_CF_POLL:
            // PC poll needs
            //  sendDelay + Poll + SIFS + nullData + SIFS + CfEnd
            cfpRequiredDuration =
                sendDelay
                + PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    dataRateType,
                    DOT11_DATA_FRAME_HDR_SIZE)
                + dot11->extraPropDelay
                + dot11->sifs
                + PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    dataRateTypeForResponse,
                    DOT11_DATA_FRAME_HDR_SIZE)
                + dot11->extraPropDelay
                + dot11->sifs
                + cfEndTime;
            break;

        case DOT11_CF_END:
            cfpRequiredDuration =
                sendDelay
                + cfEndTime;
            break;

        default:
            ERROR_ReportError("MacDot11CfpPcHasSufficientTime: "
                "Unknown frame type.\n");
            break;
    } //switch

    return cfpRequiredDuration <= cfpBalanceDuration;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationHasSufficientTime
//  PURPOSE:     Determine if station has sufficient time to send frame type
//               The frame type determines the frame exchange sequence
//               that would occur. If time was insufficient, the station
//               would not send data bu would send an ack/nulldata.
//               send a CF-End to terminate the CFP.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame types
//               clocktype sendDelay
//                  Delay before sending the frame
//               int* dataRateType
//                  Data rate to use for transmit.
//  RETURN:      TRUE if there is sufficient time in CFP to send the frame
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11CfpStationHasSufficientTime(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType frameType,
    clocktype sendDelay,
    int dataRateType)
{
    clocktype cfpBalanceDuration;
    clocktype cfpRequiredDuration = 0;
    clocktype currentMessageDuration;
    clocktype currentTime = node->getNodeTime();
    clocktype cfEndTime;

    // Potential time for CF-End frame
    cfEndTime =
        PHY_GetTransmissionDuration(
            node, dot11->myMacData->phyNumber,
            dot11->lowestDataRateType,
            DOT11_LONG_CTRL_FRAME_SIZE)
        + dot11->extraPropDelay
        + dot11->slotTime;

    cfpBalanceDuration = dot11->cfpEndTime - currentTime;

    switch (frameType) {
        case DOT11_DATA:
        case DOT11_CF_DATA_ACK:
            currentMessageDuration =
                PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    dataRateType,
                    MESSAGE_ReturnPacketSize(dot11->currentMessage) +
                    DOT11_DATA_FRAME_HDR_SIZE);

            // Station data needs sendDelay + msg time + SIFS + CfEnd
            cfpRequiredDuration =
                sendDelay
                + currentMessageDuration
                + dot11->extraPropDelay
                + dot11->sifs
                + cfEndTime;
            break;

        case DOT11_CF_NULLDATA:
        case DOT11_CF_ACK:
            // Need sendDelay + ack + SIF + CfEnd
            cfpRequiredDuration =
                sendDelay
                + PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    dataRateType,
                    DOT11_DATA_FRAME_HDR_SIZE)
                + dot11->extraPropDelay
                + dot11->sifs
                + cfEndTime;
            break;

        default:
            ERROR_ReportError("MacDot11CfpStationHasSufficientTime: "
                "Unknown frame type.\n");
            break;

    } //switch

    return cfpRequiredDuration <= cfpBalanceDuration;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcUpdateForPollSentOrSkipped
//  PURPOSE:     Update tracking values when a poll is sent or skipped.
//               For poll save by-count mode, update counters.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_CfPollListItem *itemToUpdate
//                  Item to be updated
//               BOOL pollWasSkipped
//                  Was polling skipped, FALSE for transmit poll
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcUpdateForPollSentOrSkipped(
    Node* node,
    MacDataDot11* dot11,
    DOT11_CfPollListItem *itemToUpdate,
    BOOL pollWasSkipped)
{
    DOT11_PcVars* pc = dot11->pcVars;

    ERROR_Assert(MacDot11IsPC(dot11) && MacDot11CfpPcSendsPolls(pc),
        "MacDot11CfpPcUpdateForPollSentOrSkipped: "
        "Poll list not applicable to PC configuration.\n");

    // In poll+deliver mode, the itemToUpdate is null during
    // the deliver phase (once poll round is complete)
    if (itemToUpdate == NULL) {
        return;
    }

    // Check if this is the first poll of the CFP
    if (pc->firstPolledItem == NULL) {
        pc->firstPolledItem = itemToUpdate;
    }

    pc->prevPolledItem = itemToUpdate;

    if (pc->pollSaveMode == DOT11_POLL_SAVE_BYCOUNT) {
        itemToUpdate->data->pollCount++;
        if (!pollWasSkipped) {
            // Poll count wraps around once it reaches max
            itemToUpdate->data->pollCount = 0;
        }
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcPollCanBeSkipped
//  PURPOSE:     Determine if poll can be skipped in case message
//               in buffer is not for the destination and poll-save
//               mode indicates accordingly.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_CfPollListItem *itemToCheck
//                  Poll list item which is the destination
//  RETURN:      TRUE if poll can be skipped
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11CfpPcPollCanBeSkipped(
    Node* node,
    MacDataDot11* dot11,
    DOT11_CfPollListItem *itemToCheck)
{
    DOT11_PcVars* pc = dot11->pcVars;
    DOT11_CfPollStation *station = itemToCheck->data;

    if (pc->pollSaveMode == DOT11_POLL_SAVE_NONE) {
        // Not if the poll save mode is off.
        return FALSE;
    }
    else if (dot11->currentMessage != NULL &&
             dot11->currentNextHopAddress == station->macAddr)
    {
        // Not if current message in buffer is for the station
        return FALSE;
    }

    if (pc->pollSaveMode == DOT11_POLL_SAVE_BYCOUNT) {
        // Not if below min threshold or skip cylce has been reached
        if (station->nullDataCount <= pc->pollSaveMinCount
            || station->pollCount == station->nullDataCount)
        {
            return FALSE;
        }
        else {
            //char cancelComment[MAX_STRING_LENGTH];
            //sprintf(cancelComment, "CF-Poll cancelled "
            //    "for [%02X.%02X.%02X.%02X]",
            //    (station->macAddr & 0xff000000) >> 24,
            //    (station->macAddr & 0xff0000) >> 16,
            //    (station->macAddr & 0xff00) >> 8,
            //    (station->macAddr & 0xff));
            //MacDot11Trace(node, dot11, NULL, cancelComment);

            dot11->pollsSavedCfp++;
            return TRUE;
        }
    }
    else {
        // for future poll save options
        return FALSE;
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcGetNextItemToPoll
//  PURPOSE:     Determine the next item to poll from the poll list
//               considering the previously polled station, the
//               pollable status, the poll save mode and whether a
//               cycle of poll has completed.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      NULL if there are no pollable items
//                  NULL if the list has been cycled
//                  List item if other parameters permit polling
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
DOT11_CfPollListItem * MacDot11CfpPcGetNextItemToPoll(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11_PcVars* pc = dot11->pcVars;
    DOT11_CfPollListItem* next;

    DOT11_CfPollListItem* itemToPoll; /* =
        (DOT11_CfPollListItem*)MEM_malloc(sizeof(DOT11_CfPollListItem)); */
    itemToPoll = NULL;

    ERROR_Assert(MacDot11IsPC(dot11) && MacDot11CfpPcSendsPolls(pc),
        "MacDot11CfpPcGetNextItemToPoll: "
        "Poll list not applicable to PC configuration.\n");

    if (pc->pollableCount > 0 && !pc->allStationsPolled) {
        next = MacDot11CfPollListGetNextItem(node, dot11,
            pc->prevPolledItem);

        while (next != pc->firstPolledItem) {
            if (next->data->pollType == DOT11_STA_POLLABLE_POLL) {
                if (MacDot11CfpPcPollCanBeSkipped(node, dot11, next)) {
                    // TRUE parameter indicates skipped poll
                    MacDot11CfpPcUpdateForPollSentOrSkipped(
                        node, dot11, next, TRUE);
                }
                else {
                    itemToPoll = next;
                    break;
                }
            }
            next = MacDot11CfPollListGetNextItem(node, dot11, next);
        } //while

        if (next == pc->firstPolledItem) {
            pc->allStationsPolled = TRUE;
            itemToPoll = NULL;
        }
    }

    return itemToPoll;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpCheckForOutgoingPacket.
//  PURPOSE:     Check if a packet needs to be dequeued.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static
void MacDot11CfpCheckForOutgoingPacket(
    Node* node,
    MacDataDot11* dot11)
{
    if (dot11->currentMessage == NULL) {
        MacDot11StationMoveAPacketFromTheNetworkLayerToTheLocalBuffer(node, dot11);
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcTransmitEnd
//  PURPOSE:     Transmit a CF-End(Ack) frame marking end of CF period
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame type to send
//               clocktype delay
//                  Send delay
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcTransmitEnd(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType frameType,
    clocktype delay)
{
    DOT11_LongControlFrame* lHdr;
    Message *pktToPhy;

    ERROR_Assert(MacDot11IsPC(dot11),
        "MacDot11TransmitCfEnd: Not a Point Coordinator.\n");

    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node,
                        pktToPhy,
                        DOT11_LONG_CTRL_FRAME_SIZE,
                        TRACE_DOT11);

    lHdr = (DOT11_LongControlFrame*) MESSAGE_ReturnPacket(pktToPhy);

    lHdr->frameType = (unsigned char) frameType;
    lHdr->destAddr = ANY_MAC802;
    lHdr->sourceAddr = dot11->selfAddr;
    lHdr->duration = 0;

    MacDot11CfpPcSetTransmitState(dot11, frameType, ANY_MAC802);

    MacDot11StationStartTransmittingPacket(node, dot11, pktToPhy, delay);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcEndTransmitted
//  PURPOSE:     Transit to DCF state once the CF End is transmitted
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcEndTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    dot11->endPacketsSentCfp++;

    MacDot11CfpSetEndVariables(node, dot11);

    MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcTransmitPoll
//  PURPOSE:     Transmit a CF Poll (may be with Ack)
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame type to send
//               Mac802Address destAddr
//                  Destination address of frame to be sent
//               clocktype delay
//                  Send delay
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcTransmitPoll(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType frameType,
    Mac802Address destAddr,
    clocktype delay)
{
    Message* pktToPhy;
    DOT11_FrameHdr* fHdr;

    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, pktToPhy,
        DOT11_DATA_FRAME_HDR_SIZE, TRACE_DOT11);
    fHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(pktToPhy);

    MacDot11StationSetFieldsInDataFrameHdr(dot11, fHdr, destAddr, frameType);
    fHdr->address3 = dot11->bssAddr;
    fHdr->duration = DOT11_CF_FRAME_DURATION;

    MacDot11CfpPcSetTransmitState(dot11, frameType, destAddr);

    MacDot11StationStartTransmittingPacket(node, dot11, pktToPhy, delay);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcPollTransmitted
//  PURPOSE:     Handle states and timers once a Cf Poll
//               has been transmitted.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcPollTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    dot11->pollPacketsSentCfp++;

    MacDot11CfpPcSetPostTransmitState(dot11);

    MacDot11StationStartTimer(node, dot11, dot11->cfpTimeout);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcTransmitBroadcast
//  PURPOSE:     Transmit a broadcast frame during CFP.
//               An ack may piggyback on the broadcast.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame type to send
//               clocktype delay
//                  Send delay
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcTransmitBroadcast(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType frameType,
    clocktype delay)
{
    Message* dequeuedPacket;
    DOT11_FrameHdr* fHdr;

    ERROR_Assert(dot11->currentMessage != NULL,
        "MacDot11CfpPcTransmitBroadcast: Current message is NULL.\n");

    dequeuedPacket = dot11->currentMessage;
    fHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(dequeuedPacket);

    MacDot11StationSetFieldsInDataFrameHdr(dot11, fHdr, ANY_MAC802, frameType);
    fHdr->duration = DOT11_CF_FRAME_DURATION;

    MacDot11CfpPcSetTransmitState(dot11, frameType, ANY_MAC802);

    MacDot11StationStartTransmittingPacket(node, dot11, dequeuedPacket, delay);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcBroadcastTransmitted
//  PURPOSE:     Clear message variables, set an SIFS timer after
//               transmit of broadcast.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcBroadcastTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    dot11->broadcastPacketsSentCfp++;

    if (dot11->dot11TxFrameInfo!= NULL )
    {
       if (dot11->currentMessage == dot11->dot11TxFrameInfo->msg)
       {
           if ((dot11->currentACIndex >= DOT11e_AC_BK) &&
            dot11->currentMessage ==
            dot11->ACs[dot11->currentACIndex].frameToSend) {
            dot11->ACs[dot11->currentACIndex].totalNoOfthisTypeFrameDeQueued++;
            dot11->ACs[dot11->currentACIndex].frameToSend = NULL;
            dot11->ACs[dot11->currentACIndex].BO = 0;
            dot11->ACs[dot11->currentACIndex].currentNextHopAddress = INVALID_802ADDRESS;
                if (dot11->ACs[dot11->currentACIndex].frameInfo!= NULL)
                {
                    MEM_free(dot11->ACs[dot11->currentACIndex].frameInfo);
                    dot11->ACs[dot11->currentACIndex].frameInfo = NULL;
                }
            }
            dot11->dot11TxFrameInfo = NULL;
        }
      }
    MacDot11StationResetCurrentMessageVariables(node, dot11);

    MacDot11CfpPcSetPostTransmitState(dot11);

    MacDot11StationStartTimer(node, dot11, dot11->txSifs);
}



//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcTransmitAck
//  PURPOSE:     Transmit an ack in case PC receives unicast data
//               from another BSS which is also in CFP.
//               This is an exception scenario.
//
//               Note that ACK is not CF-Ack.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Destination address
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcTransmitAck(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr)
{
    Message* pktToPhy;
    DOT11_ShortControlFrame sHdr;

    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node, pktToPhy,
        DOT11_SHORT_CTRL_FRAME_SIZE, TRACE_DOT11);

    sHdr.frameType = DOT11_ACK;
    sHdr.destAddr = destAddr;
    sHdr.duration = 0;

    memcpy(MESSAGE_ReturnPacket(pktToPhy), &(sHdr),
           DOT11_SHORT_CTRL_FRAME_SIZE);

    MacDot11CfpPcSetTransmitState(dot11, DOT11_ACK, destAddr);

    MacDot11StationStartTransmittingPacket(
        node,
        dot11,
        pktToPhy,
        dot11->sifs);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcAckTransmitted
//  PURPOSE:     Continue in PCF by setting an SIFS timer
//               after transmit of an ACK.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcAckTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    MacDot11CfpPcSetPostTransmitState(dot11);

    MacDot11StationStartTimer(node, dot11, dot11->txSifs);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcTransmitUnicast
//  PURPOSE:     Transmit a unicast frame during CFP.
//               An ack and/or a poll may piggyback on the data frame.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame type
//               Mac802Address destAddr
//                  Destination address
//               clocktype delay
//                  Delay
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcTransmitUnicast(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType frameType,
    Mac802Address destAddr,
    clocktype delay)
{
    Message* pktToPhy;
    DOT11_FrameHdr* fHdr;
    DOT11_SeqNoEntry *entry;

    ERROR_Assert(dot11->currentMessage != NULL,
        "MacDot11CfpPcTransmitUnicast: Current message is NULL.\n");

    pktToPhy = MESSAGE_Duplicate(node, dot11->currentMessage);

    fHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(pktToPhy);
    MacDot11StationSetFieldsInDataFrameHdr(dot11, fHdr, destAddr, frameType);
    fHdr->duration =  DOT11_CF_FRAME_DURATION;

    entry = MacDot11StationGetSeqNo(node, dot11, destAddr);
    ERROR_Assert(entry,
        "MacDot11CfpPcTransmitUnicast: "
        "Unable to find or create sequence number entry.\n");
    fHdr->seqNo = entry->toSeqNo;

    dot11->waitingForAckOrCtsFromAddress = destAddr;

    if (MESSAGE_ReturnPacketSize(dot11->currentMessage) >
        DOT11_FRAG_THRESH)
    {
        // Fragmentation not currently supported
        ERROR_ReportError("MacDot11CfpPcTransmitUnicast: "
            "Fragmentation not supported.\n");
    }
    else {
        fHdr->fragId = 0;
        MacDot11CfpPcSetTransmitState(dot11, frameType, destAddr);

        MacDot11StationStartTransmittingPacket(node, dot11, pktToPhy, delay);
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcUnicastTransmitted
//  PURPOSE:     Set states and timers once a unicast has been transmitted.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcUnicastTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    dot11->dataPacketsSentCfp++;

    MacDot11CfpPcSetPostTransmitState(dot11);

    MacDot11StationStartTimer(node, dot11, dot11->cfpTimeout);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcUnicastPollTransmitted
//  PURPOSE:     Handle states and timers once a unicast+poll
//               has been transmitted.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcUnicastPollTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    dot11->dataPollPacketsSentCfp++;

    MacDot11CfpPcSetPostTransmitState(dot11);

    MacDot11StationStartTimer(node, dot11, dot11->cfpTimeout);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcAdjustFrameTypeForTime
//  PURPOSE:     Adjust frame type intended to be sent for balance CFP.
//               In case time is insufficient, possible transitions are
//                  data+poll   ->  poll
//                  data        ->  end
//                  poll        ->  end
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame type
//               Mac802Address destAddr
//                  Destination address
//               clocktype delay
//                  Delay before transmit
//               int dataRateType
//                  Data rate at which to transmit
//  RETURN:      Modified frame type in case time is insufficient.
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcAdjustFrameTypeForTime(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address* destAddr,
    DOT11_MacFrameType* frameToSend,
    clocktype sendDelay,
    int* dataRateType)
{
    clocktype cfpBalanceDuration;
    clocktype cfEndTime;

    if (*frameToSend == DOT11_CF_NONE) {
        return;
    }

    // Potential time for CF-End frame
    cfEndTime =
        PHY_GetTransmissionDuration(
            node, dot11->myMacData->phyNumber,
            dot11->lowestDataRateType,
            DOT11_LONG_CTRL_FRAME_SIZE)
        + dot11->extraPropDelay
        + dot11->slotTime;

    // After sending a beacon, the PC beacon time indicates the
    // time for the next beacon.
    cfpBalanceDuration =
        dot11->cfpEndTime
        - node->getNodeTime();

    if (! MacDot11CfpPcHasSufficientTime(node, dot11,
            *frameToSend, sendDelay, *destAddr,
            cfpBalanceDuration, cfEndTime, *dataRateType))
    {
        switch (*frameToSend) {
            case DOT11_CF_DATA_POLL:
                *frameToSend = DOT11_CF_POLL;
                if (! MacDot11CfpPcHasSufficientTime(node, dot11,
                        *frameToSend, sendDelay, *destAddr,
                        cfpBalanceDuration, cfEndTime, *dataRateType))
                {
                    *frameToSend = DOT11_CF_END;
                    *destAddr = ANY_MAC802;
                }
                break;

            case DOT11_CF_POLL:
            case DOT11_DATA:
                *frameToSend = DOT11_CF_END;
                *destAddr = ANY_MAC802;
                break;

            case DOT11_CF_END:
                //do nothing
                break;

            default:
                ERROR_ReportError("MacDot11CfpPcAdjustFrameTypeForTime: "
                    "Unexpected frame type.\n");
                break;
        } //switch

        if (*frameToSend == DOT11_CF_END && dot11->cfpAckIsDue == FALSE) {
            *dataRateType =
                MIN(*dataRateType, dot11->highestDataRateTypeForBC);

            if (! MacDot11CfpPcHasSufficientTime(node, dot11,
                    *frameToSend, sendDelay, *destAddr,
                    cfpBalanceDuration, cfEndTime, *dataRateType))
            {
                *frameToSend = DOT11_CF_NONE;
            }
        }
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcAdjustFrameTypeForData
//  PURPOSE:     Adjust frame type intended to be sent for data in buffer.
//               In case there is no data, possible transitions are
//                  data+poll   ->  poll
//                  data        ->  none
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Destination address
//               DOT11_MacFrameType* frameToSend
//                  Frame to be sent
//  RETURN:      Modified frame type in case there is no data in buffer.
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcAdjustFrameTypeForData(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr,
    DOT11_MacFrameType* frameToSend)
{
    switch (*frameToSend) {
        case DOT11_CF_DATA_POLL:
            MacDot11CfpCheckForOutgoingPacket(node,dot11);
            if (dot11->currentMessage == NULL
                || (dot11->currentNextHopAddress != destAddr))
            {
                *frameToSend = DOT11_CF_POLL;
            }
            else{
                if (dot11->dot11TxFrameInfo->frameType != DOT11_DATA){
                    *frameToSend = DOT11_CF_POLL;
                }
            }
            break;

        case DOT11_DATA:
            MacDot11CfpCheckForOutgoingPacket(node,dot11);
            if (dot11->currentMessage == NULL) {
                *frameToSend = DOT11_CF_NONE;
            }
            else{
                if (dot11->dot11TxFrameInfo->frameType != DOT11_DATA){
                    *frameToSend = DOT11_CF_NONE;
                }
            }
            break;

        default:
            ERROR_ReportError("MacDot11CfpPcAdjustFrameTypeForData: "
                "Unexpected frame type.\n");
            break;
    } //switch
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcTransmitDeliverOnly
//  PURPOSE:     Determine the next frame to transmit when PC
//               is in DELIVER-ONLY mode.
//               Fills up frame type to send and the destination address.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType* frameToSend
//                  Frame type to be sent
//               Mac802Address destAddr
//                  Destination address
//               clocktype sendDelay
//                  Delay to be sent
//  RETURN:      Filled up destination address
//               Filled up frame type to send
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcTransmitDeliverOnly(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address* destAddr,
    DOT11_MacFrameType* frameToSend,
    clocktype sendDelay)
{
    DOT11_PcVars* pc = dot11->pcVars;

    ERROR_Assert(pc->deliveryMode == DOT11_PC_DELIVER_ONLY,
        "MacDot11CfpPcTransmitDeliverOnly: "
        "PC is not in DELIVER-ONLY mode.\n");

    // In this mode, there is no polling list to consider
    *frameToSend = DOT11_DATA;
    *destAddr = dot11->currentNextHopAddress;

    MacDot11CfpPcAdjustFrameTypeForData(node, dot11,
        *destAddr, frameToSend);

    if (*frameToSend == DOT11_CF_NONE) {
        *frameToSend = DOT11_CF_END;
        *destAddr = ANY_MAC802;
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcTransmitPollAndDeliver
//  PURPOSE:     Determine the next frame to transmit when
//               PC is in POLL-AND-DELIVER mode.
//               Fills up frame type to send and destination address.
//
//               Gives priority to broadcast packets over polls.
//
//               This is a minimal algorith which can benefits from:
//               -  deeper look into network queue so that data
//                  and polls combine as far as possible.
//               -  poll ageing
//                  e.g. poll saving by count (this is implemented)
//               -  use of more data flag so that stations may be
//                  polled multipe time in a CFP.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Destination address
//               DOT11_MacFrameType* frameToSend
//                  Frame type
//               clocktype sendDelay
//                  Delay
//               DOT11_CfPollListItem **stationItem
//                  Polling list item
//  RETURN:      Filled up destination address
//               Filled up frame type to send
//               Filled up polling list item (may be NULL in case of
//                  broadcasts or when poll cycle is complete)
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcTransmitPollAndDeliver(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address *destAddr,
    DOT11_MacFrameType *frameToSend,
    clocktype sendDelay,
    DOT11_CfPollListItem **stationItem)
{
    DOT11_PcVars* pc = dot11->pcVars;

    ERROR_Assert(pc->deliveryMode == DOT11_PC_POLL_AND_DELIVER,
        "MacDot11CfpPcTransmitDeliverAndPoll: "
        "PC is not in POLL-AND-DELIVER mode.\n");

    // Send a broadcast if it has priority
    if (dot11->currentMessage != NULL
        && pc->broadcastsHavePriority
        && dot11->currentNextHopAddress == ANY_MAC802)
    {
        *frameToSend = DOT11_DATA;
        *destAddr = ANY_MAC802;
        *stationItem = NULL;
        return;
    }

    *stationItem = MacDot11CfpPcGetNextItemToPoll(node, dot11);

    if (*stationItem == NULL) {
        // All station polled at least once.
        // Operate in Deliver Only mode.
        //
        // Modifications related to more data flag can be made here.

        *frameToSend = DOT11_DATA;
        *destAddr = dot11->currentNextHopAddress;

        MacDot11CfpPcAdjustFrameTypeForData(node, dot11,
            *destAddr, frameToSend);

        if (*frameToSend != DOT11_CF_NONE) {
            // Piggyback a poll if station type permits
            if (*frameToSend == DOT11_DATA
                && MacDot11CfPollListIsAddressPollable(
                        node, dot11, *destAddr))
            {
                *frameToSend = DOT11_CF_DATA_POLL;
            }
        }
        else {
            *frameToSend = DOT11_CF_END;
            *destAddr = ANY_MAC802;
        }
    }
    else {
        // There is a pollable station unpolled this CFP
        *destAddr = (*stationItem)->data->macAddr;
        *frameToSend = DOT11_CF_DATA_POLL;

        MacDot11CfpPcAdjustFrameTypeForData(node, dot11,
            *destAddr, frameToSend);
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcTransmitPollOnly
//  PURPOSE:     Determine the next frame to transmit when
//               PC is in POLL-ONLY mode.
//               Fills up frame type to send and destination address
//               Gives priority to broadcast packets over polls.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Destination address
//               DOT11_MacFrameType* frameToSend
//                  Frame type
//               clocktype sendDelay
//                  Delay
//               DOT11_CfPollListItem **stationItem
//                  Polling list item
//  RETURN:      Filled up destination address
//               Filled up frame type to send
//               Filled up polling list item (may be NULL in case of
//                  broadcasts or when poll cycle is complete)
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcTransmitPollOnly(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address* destAddr,
    DOT11_MacFrameType* frameToSend,
    clocktype sendDelay,
    DOT11_CfPollListItem** stationItem)
{
    DOT11_PcVars* pc = dot11->pcVars;

    ERROR_Assert(pc->deliveryMode == DOT11_PC_POLL_ONLY,
        "MacDot11CfpPcTransmitPollOnly: PC is not in POLL-ONLY mode.\n");

    if (dot11->currentMessage != NULL
        && pc->broadcastsHavePriority
        && dot11->currentNextHopAddress == ANY_MAC802)
    {
        *frameToSend = DOT11_DATA;
        *destAddr = ANY_MAC802;
        *stationItem = NULL;
        return;
    }

    *stationItem = MacDot11CfpPcGetNextItemToPoll(node, dot11);

    if (*stationItem != NULL) {
        *destAddr = (*stationItem)->data->macAddr;
        *frameToSend = DOT11_CF_DATA_POLL;

        MacDot11CfpPcAdjustFrameTypeForData(node, dot11,
            *destAddr, frameToSend);
    }
    else {
        *frameToSend = DOT11_CF_END;
        *destAddr = ANY_MAC802;
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcTransmit
//  PURPOSE:     Governing routine for PC transmits during CFP.
//               Determines the send delay to use and calls the
//               configured delivery algorithm to determine what
//               is to be sent.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType receivedFrameType
//                  Received frame, none in case the previous frame
//                  sent by the PC was a beacon, a broadcast, or
//                  there was a timeout
//               clocktype delay
//                  Timer delay, 0 if not applicable.
//  PARAMETERS:
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcTransmit(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType receivedFrameType,
    clocktype timerDelay)
{
    DOT11_PcVars* pc = dot11->pcVars;
    clocktype sendDelay;
    DOT11_MacFrameType frameType = DOT11_CF_NONE;
    Mac802Address destAddr;
    DOT11_CfPollListItem* stationItem = NULL;
    int dataRateType;

    if (MacDot11StationPhyStatus(node, dot11) != PHY_IDLE) {
        MacDot11Trace(node, dot11, NULL, "CFP PC transmit, phy not idle.");

        return;
    }
    if (dot11->beaconIsDue == TRUE){

        if (DEBUG_BEACON)
        {
            char timeStr[100];

            ctoa(node->getNodeTime(), timeStr);

            printf("\n%d  about to transmit beacon in CF Duration at %s"
                , node->nodeId,
                timeStr);
        }
        MacDot11DTIMupdate(node, dot11);
        MacDot11CfpCountUpdate( node, dot11  ) ;
        MacDot11CfpSetState(dot11,  DOT11_S_CFP_WFBEACON);
        MacDot11StationStartTimer(node, dot11, dot11->txSifs);
        dot11->BeaconAttempted = TRUE;
        return;
    }

    // In case of timeout, at a PC, the timer delay is PIFS.
    // In that case, a token delay of 1 is set for transmit.
    if (timerDelay >= dot11->sifs) {
        sendDelay = EPSILON_DELAY;
    }
    else {
        sendDelay = dot11->sifs - timerDelay;
    }

    switch (pc->deliveryMode) {

        case DOT11_PC_DELIVER_ONLY:
            switch (dot11->cfpState) {
                case DOT11_S_CFP_NONE:
                    MacDot11CfpPcTransmitDeliverOnly(node, dot11,
                        &destAddr, &frameType, sendDelay);
                    break;

                case DOT11_S_CFP_WF_ACK:
                    // Since the Mac buffer can hold one packet, options are
                    // - attempt a retransmit
                    // - or drop packet and continue delivery
                    // - or end CFP (current implementation)
                    frameType = DOT11_CF_END;
                    destAddr = ANY_MAC802;
                    break;

                default:
                    ERROR_ReportError("MacDot11CfpPcTransmit: "
                        "Unknown CFP state for DELIVER-ONLY mode.\n");
                    break;
            } //switch
            break;

        case DOT11_PC_POLL_ONLY:
            switch (dot11->cfpState) {
                case DOT11_S_CFP_NONE:
                    MacDot11CfpPcTransmitPollOnly(node, dot11,
                        &destAddr, &frameType, sendDelay, &stationItem);
                    break;

                case DOT11_S_CFP_WF_DATA:
                case DOT11_S_CFP_WF_DATA_ACK:
                    MacDot11CfPollListUpdateForNullDataResponse(node,
                        dot11, pc->prevPolledItem->data->macAddr);

                    // Continue polling
                    MacDot11CfpPcTransmitPollOnly(node, dot11,
                        &destAddr, &frameType, sendDelay, &stationItem);
                    break;

                default:
                    ERROR_ReportError("MacDot11CfpPcTransmit: "
                        "Unknown CFP state for POLL-ONLY mode.\n");
                    break;
            } //switch
            break;

        case DOT11_PC_POLL_AND_DELIVER:
            switch (dot11->cfpState) {
                case DOT11_S_CFP_NONE:
                    MacDot11CfpPcTransmitPollAndDeliver(node, dot11,
                        &destAddr, &frameType, sendDelay, &stationItem);
                    break;

                case DOT11_S_CFP_WF_ACK:
                    // Stop operating in delivery mode
                    frameType = DOT11_CF_END;
                    destAddr = ANY_MAC802;
                    break;

                case DOT11_S_CFP_WF_DATA:
                case DOT11_S_CFP_WF_DATA_ACK:
                    if (!pc->allStationsPolled) {
                        MacDot11CfPollListUpdateForNullDataResponse(node,
                            dot11, pc->prevPolledItem->data->macAddr);

                        // Continue polling
                        MacDot11CfpPcTransmitPollAndDeliver(node, dot11,
                            &destAddr, &frameType, sendDelay, &stationItem);
                    }
                    else {
                        // In delivery mode, the packet in buffer timed out.
                        // End PCF.
                        frameType = DOT11_CF_END;
                        destAddr = ANY_MAC802;
                    }
                    break;
                default:
                    ERROR_ReportError("MacDot11CfpPcTransmit: "
                        "Unknown CFP state for POLL-AND-DELIVER mode.\n");
                    break;
            } //switch

            break;
        default:
            ERROR_ReportError("MacDot11CfpPcTransmit: "
                "Unknown PC delivery mode.\n");
            break;
    } //switch

    MacDot11CfpPcGetTxDataRate(
        node, dot11, destAddr, frameType, &dataRateType);

    MacDot11CfpPcAdjustFrameTypeForTime(node, dot11,
        &destAddr, &frameType, sendDelay, &dataRateType);

    MacDot11CfpAdjustFrameTypeForAck(dot11, &frameType);

    PHY_SetTxDataRateType(
        node,
        dot11->myMacData->phyNumber,
        dataRateType);

 char macAdder[24];
 MacDot11MacAddressToStr(macAdder,&destAddr);

    switch (frameType) {

        case DOT11_CF_DATA_POLL:
        case DOT11_CF_DATA_POLL_ACK:
            MacDot11CfpPcUpdateForPollSentOrSkipped(
                node, dot11, stationItem, FALSE);
            if (DEBUG_PCF)
            {
             char timeStr[100];
             ctoa(node->getNodeTime(), timeStr);
                 if (frameType == DOT11_CF_DATA_POLL){
                 printf("\n%d Sending  DATA_POLL"
                     "\t to %s at %s \n",
                        node->nodeId,
                        macAdder,
                        timeStr);
                 }
                 else{
                     printf("\n%d Sending  DATA_POLL_ACK"
                     "\t to %s at %s \n",
                        node->nodeId,
                        macAdder,
                        timeStr);
                 }
            }

            MacDot11CfpPcTransmitUnicast(node, dot11,
                frameType, destAddr, sendDelay);
            break;

        case DOT11_DATA:
        case DOT11_CF_DATA_ACK:
            if (DEBUG_PCF)
            {
                 char timeStr[100];
                 ctoa(node->getNodeTime(), timeStr);
                 if (frameType == DOT11_DATA)
                 {
                 printf("\n%d Sending DOT11_DATA "
                       "\t to %s at %s \n",
                        node->nodeId,
                        macAdder,
                        timeStr);
                 }
                 else{
                     printf("\n%d Sending  DOT11_CF_DATA_ACK"
                       "\t to %s at %s \n",
                        node->nodeId,
                        macAdder,
                        timeStr);
                 }
            }
            if (destAddr == ANY_MAC802) {
                MacDot11CfpPcTransmitBroadcast(node, dot11,
                    frameType, sendDelay);
            }
            else {
                MacDot11CfpPcTransmitUnicast(node, dot11,
                    frameType, destAddr, sendDelay);
            }
            break;

        case DOT11_CF_POLL:
        case DOT11_CF_POLL_ACK:
            MacDot11CfpPcUpdateForPollSentOrSkipped(
                node, dot11, stationItem, FALSE);
            if (DEBUG_PCF)
            {
             char timeStr[100];
             ctoa(node->getNodeTime(), timeStr);
                 if (frameType == DOT11_CF_POLL){
                 printf("\n%d Sending  POLL"
                     "\t to %s at %s \n",
                        node->nodeId,
                        macAdder,
                        timeStr);
                 }
                 else{
                     printf("\n%d Sending  POLL_ACK"
                     "\t to %s at %s \n",
                        node->nodeId,
                        macAdder,
                        timeStr);
                 }
            }

            MacDot11CfpPcTransmitPoll(node, dot11,
                frameType, destAddr, sendDelay);
            break;

        case DOT11_CF_END:
        case DOT11_CF_END_ACK:
            if (DEBUG_PCF)
            {
                 char timeStr[100];
                 ctoa(node->getNodeTime(), timeStr);
                 if (frameType == DOT11_CF_END)
                 {
                 printf("\n%d Sending CF_END "
                       "\t at %s \n",
                        node->nodeId,
                        timeStr);
                 }
                 else{
                     printf("\n%d Sending  CF_END_ACK"
                       "\t at %s \n",
                        node->nodeId,
                        timeStr);
                 }
            }

            MacDot11CfpPcTransmitEnd(node, dot11,
                frameType, sendDelay);
            break;

        case DOT11_CF_NONE:
            break;

        default:
            ERROR_ReportError("MacDot11CfpPcTransmit: "
                "Unknown frame type.\n");
            break;
    } //switch
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcProcessAck
//  PURPOSE:     Process the received ack.
//               The ack could piggybacks on a data+ack
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address sourceAddr
//                  Source address od Ack
//               BOOL isPureAck
//                  TRUE if it is a CF-Ack frame
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcProcessAck(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address sourceAddr,
    BOOL isPureAck)
{
    if (dot11->waitingForAckOrCtsFromAddress == INVALID_802ADDRESS
        || dot11->waitingForAckOrCtsFromAddress != sourceAddr)
    {
        return;
    }

    ERROR_Assert(dot11->currentMessage != NULL,
        "MacDot11CfpPcProcessAck: "
        "Processing CF Ack without a message in buffer.\n");

    dot11->dataRateInfo->numAcksInSuccess++;
    dot11->dataRateInfo->firstTxAtNewRate = FALSE;

    MacDot11StationUpdateForSuccessfullyReceivedAck(node, dot11, sourceAddr);

    dot11->unicastPacketsSentCfp++;

    MacDot11StationCancelTimer(node, dot11);

    if (isPureAck) {
        MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
    }

    MacDot11CfpCheckForOutgoingPacket(node, dot11);

}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcProcessUnicast
//  PURPOSE:     Process a unicast data frame.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received message
//  RETURN:      TRUE if frame is acceptable
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11CfpPcProcessUnicast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11_PcVars* pc = dot11->pcVars;
    BOOL unicastIsProcessed = FALSE;
    DOT11_FrameHdr *fHdr =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    Mac802Address sourceAddr = fHdr->sourceAddr;

    if (MacDot11StationCorrectSeqenceNumber(node, dot11,
            sourceAddr, fHdr->seqNo))
    {
        MacDot11Trace(node, dot11, msg, "Receive");

        dot11->unicastPacketsGotCfp++;

        MacDot11UpdateStatsReceive(
            node,
            dot11,
            msg,
            dot11->myMacData->interfaceIndex);

        pc->dataRateTypeForAck =
            PHY_GetRxDataRateType(node, dot11->myMacData->phyNumber);

        MacDot11CfPollListUpdateForUnicastReceived(
            node, dot11, sourceAddr);

        MacDot11StationHandOffSuccessfullyReceivedUnicast(node, dot11, msg);

        unicastIsProcessed = TRUE;
    }
    else {
        MacDot11Trace(node, dot11, NULL, "Drop, wrong sequence number");
#ifdef ADDON_DB
        HandleMacDBEvents(        
                node, msg, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif
        MESSAGE_Free(node, msg);

        unicastIsProcessed = FALSE;
    }

    MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
    dot11->cfpAckIsDue = TRUE;

    return unicastIsProcessed;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcReceiveUnicast
//  PURPOSE:     Process a received unicast frame.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpPcReceiveUnicast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11_FrameHdr *fHdr =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    DOT11_MacFrameType frameType = (DOT11_MacFrameType) fHdr->frameType;
    Mac802Address sourceAddr = fHdr->sourceAddr;
    Mac802Address destAddr = fHdr->destAddr;
    unsigned char frameFlags = fHdr->frameFlags;

    switch (frameType) {
        case DOT11_CF_DATA_ACK:
            MacDot11CfpPcProcessAck(node, dot11, sourceAddr, FALSE);

            // no break; continue to process DATA

        case DOT11_DATA:
            MacDot11Trace(node, dot11, msg, "Receive");
            dot11->dataPacketsGotCfp++;

            if (DEBUG_PCF)
            {
                 char timeStr[100];
                 ctoa(node->getNodeTime(), timeStr);
                 if (frameType == DOT11_CF_DATA_ACK)
                 {
                    printf("\n%d Recieved DATA_ACK "
                       "\t  at %s \n",
                        node->nodeId,
                        timeStr);
                 }
                 else
                 {
                    printf("\n%d Recieved DATA"
                       "\t  at %s \n",
                        node->nodeId,
                        timeStr);
                 }
             }

            MacDot11CfpPcProcessUnicast(node, dot11, msg);
            if (MacDot11IsFrameFromStationInMyBSS(dot11,
                    destAddr, frameFlags))
            {
                MacDot11CfpPcTransmit(node, dot11, frameType, 0);
            }
            else {
                MacDot11CfpPcTransmitAck(node, dot11, sourceAddr);
            }
            break;

        case DOT11_CF_NULLDATA:
            MacDot11Trace(node, dot11, msg, "Receive");
            if (DEBUG_PCF)
                {
                 char timeStr[100];
                 ctoa(node->getNodeTime(), timeStr);
                 printf("\n%d Recieved NULLDATA "
                       "\t  at %s \n",
                        node->nodeId,
                        timeStr);
                 }
            dot11->nulldataPacketsGotCfp++;

            if (!(fHdr->frameFlags & DOT11_MORE_DATA_FIELD ))
            {
            MacDot11CfPollListUpdateForNullDataResponse(
                node, dot11, sourceAddr);
            }
            MacDot11StationCancelTimer(node, dot11);
            MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);

            MESSAGE_Free(node, msg);

            MacDot11CfpPcTransmit(node, dot11, frameType, 0);
            break;

        case DOT11_CF_ACK:
            MacDot11Trace(node, dot11, msg, "Receive");

            if (DEBUG_PCF)
              {
                 char timeStr[100];
                 ctoa(node->getNodeTime(), timeStr);
                 printf("\n%d Recieved ACK "
                       "\t  at %s \n",
                        node->nodeId,
                        timeStr);
            }
            MacDot11CfpPcProcessAck(node, dot11, sourceAddr, TRUE);

            MESSAGE_Free(node, msg);

            MacDot11CfpPcTransmit(node, dot11, frameType, 0);
            break;

        default:
            ERROR_ReportError("MacDot11CfpReceiveUnicastPacketFromPhy: "
                "Received unknown unicast frame type.\n");
            break;
    } //switch
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationProcessEnd
//  PURPOSE:     Process a CF-End frame from the PC
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationProcessEnd(
    Node* node,
    MacDataDot11* dot11)
{
    dot11->endPacketsGotCfp++;

    MacDot11CfpSetEndVariables(node, dot11);

    dot11->NAV = 0;

    MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationProcessAck
//  PURPOSE:     Station receives an ack as part of CF-End/Data/DataPoll/Poll.
//               Station cannot get a pure ack.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address sourceAddr
//                  Source address of frame containing ack
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationProcessAck(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address sourceAddr)
{
    if (dot11->waitingForAckOrCtsFromAddress == INVALID_802ADDRESS
        || dot11->waitingForAckOrCtsFromAddress != sourceAddr)
    {
        return;
    }

    ERROR_Assert(dot11->currentMessage != NULL,
        "MacDot11CfpStationProcessAck: "
        "Processing CF Ack without a message in buffer.\n");

    MacDot11StationUpdateForSuccessfullyReceivedAck(node, dot11, sourceAddr);

    dot11->unicastPacketsSentCfp++;

    MacDot11StationCancelTimer(node, dot11);
    MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);

    MacDot11CfpCheckForOutgoingPacket(node, dot11);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationTransmitAckOrNullData
//  PURPOSE:     Transmit of ack by a BSS station.
//               Also used to transmit a nulldata frame.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame type to send
//               clocktype delay
//                  Delay
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationTransmitAckOrNullData(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType frameType,
    clocktype delay)
{
    Message* pktToPhy;
    DOT11_FrameHdr* fHdr;
    Mac802Address destAddr = dot11->bssAddr;

    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node, pktToPhy,
        DOT11_DATA_FRAME_HDR_SIZE, TRACE_DOT11);
    fHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(pktToPhy);

    MacDot11StationSetFieldsInDataFrameHdr(dot11, fHdr, destAddr, frameType);
    fHdr->address3 = dot11->bssAddr;
    fHdr->duration = DOT11_CF_FRAME_DURATION;

    MacDot11CfpStationSetTransmitState(dot11, frameType);

    MacDot11StationStartTransmittingPacket(node, dot11, pktToPhy, delay);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationAckTransmitted
//  PURPOSE:     Set state once an ack transmit is complete.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationAckTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    MacDot11CfpStationSetPostTransmitState(dot11);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationNullDataTransmitted
//  PURPOSE:     Set state once a nulldata transmit is complete.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationNullDataTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    dot11->nulldataPacketsSentCfp++;

    MacDot11CfpStationSetPostTransmitState(dot11);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationTransmitUnicast
//  PURPOSE:     Transmit a unicast frame in response to poll.
//               An ack may piggyback on the data frame.
//  PARAMETERS:  Frame type to send
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Frame type
//               clocktype delay
//                  Delay to be sent
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationTransmitUnicast(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType frameType,
    clocktype delay)
{
    Message* pktToPhy;
    DOT11_FrameHdr* fHdr;
    DOT11_SeqNoEntry* entry;
    Mac802Address destAddr = dot11->bssAddr;

    ERROR_Assert(dot11->currentMessage != NULL,
        "MacDot11CfpStationTransmitUnicast: "
        "Transmit of data when message is NULL.\n");

    pktToPhy = MESSAGE_Duplicate(node, dot11->currentMessage);

    fHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(pktToPhy);
    MacDot11StationSetFieldsInDataFrameHdr(dot11, fHdr, destAddr, frameType);
    fHdr->duration =  DOT11_CF_FRAME_DURATION;

    entry = MacDot11StationGetSeqNo(node, dot11, destAddr);
    ERROR_Assert(entry,
        "MacDot11CfpStationTransmitUnicast: "
        "Unable to find or create sequence number entry.\n");
    fHdr->seqNo = entry->toSeqNo;

    dot11->waitingForAckOrCtsFromAddress = destAddr;
    dot11->dataRateInfo =
            MacDot11StationGetDataRateEntry(node, dot11,
                destAddr);

    ERROR_Assert(dot11->dataRateInfo->ipAddress ==
        destAddr,
        "Address does not match");
    MacDot11StationAdjustDataRateForNewOutgoingPacket(node, dot11);

    if (MESSAGE_ReturnPacketSize(dot11->currentMessage) >
        DOT11_FRAG_THRESH)
    {
        // Fragmentation not currently supported
        ERROR_ReportError("MacDot11CfpStationTransmitUnicast: "
            "Fragmentation not supported.\n");
    }
    else {
        fHdr->fragId = 0;
        MacDot11CfpStationSetTransmitState(dot11, frameType);
        MacDot11StationStartTransmittingPacket(node, dot11, pktToPhy, delay);
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationUnicastTransmitted
//  PURPOSE:     Set states/timers after a station transmits a unicast
//               during CFP.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationUnicastTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    dot11->dataPacketsSentCfp++;

    MacDot11CfpStationSetPostTransmitState(dot11);

    MacDot11StationStartTimer(node, dot11, dot11->cfpTimeout);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationTransmit
//  PURPOSE:     Determine the frame to transmit based on timer delay,
//               available balance CFP duration and received frame type.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_MacFrameType frameType
//                  Received frame type
//               clocktype delay
//                  Delay to be sent
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationTransmit(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameType receivedFrameType,
    clocktype timerDelay)
{
    clocktype sendDelay;
    DOT11_MacFrameType frameType = DOT11_CF_NONE;
    int dataRateType;

    if (MacDot11StationPhyStatus(node, dot11) != PHY_IDLE) {
        MacDot11Trace(node, dot11, NULL,
            "CFP Station transmit, phy not idle.");
        return;
    }

    // The timerDelay should be 0 for a station
    if (timerDelay < dot11->sifs) {
        sendDelay = dot11->sifs - timerDelay;
    }
    else {
        sendDelay = EPSILON_DELAY;
    }

    dataRateType = PHY_GetRxDataRateType(node, dot11->myMacData->phyNumber);

    switch (receivedFrameType) {
        case DOT11_CF_DATA_POLL:
        case DOT11_CF_DATA_POLL_ACK:
            // In response to the poll, send a data frame if possible,
            // else respond with an ACK.
            MacDot11CfpCheckForOutgoingPacket(node, dot11);
            if (dot11->currentMessage == NULL ||
                (dot11->dot11TxFrameInfo!= NULL &&
                dot11->dot11TxFrameInfo->frameType <= DOT11_ACTION)) {
                frameType = DOT11_CF_ACK;
            }
            else {
                frameType = DOT11_DATA;
                if (MacDot11CfpStationHasSufficientTime(node, dot11,
                    frameType, sendDelay, dataRateType))
                {
                    MacDot11CfpAdjustFrameTypeForAck(dot11, &frameType);
                }
                else {
                    frameType = DOT11_CF_ACK;
                }
            }
            break;

        case DOT11_CF_POLL:
        case DOT11_CF_POLL_ACK:
            // Send a data frame if possible
            // else respond with a null data response.
            MacDot11CfpCheckForOutgoingPacket(node, dot11);
            if (dot11->currentMessage == NULL||
                (dot11->dot11TxFrameInfo!= NULL &&
                dot11->dot11TxFrameInfo->frameType <= DOT11_ACTION)) {
                frameType = DOT11_CF_NULLDATA;
            }
            else {
                frameType = DOT11_DATA;
                dot11->isMoreDataPresent = FALSE;
                if (! MacDot11CfpStationHasSufficientTime(node, dot11,
                    frameType, sendDelay, dataRateType))
                {
                    dot11->isMoreDataPresent = TRUE;
                    frameType = DOT11_CF_NULLDATA;
                }
            }
            break;

        case DOT11_DATA:
            // Send a CF-ACK. An option is to send DOT11_ACK.
            frameType = DOT11_CF_ACK;
            break;

        default:
            ERROR_ReportError("MacDot11CfpStationTransmit: "
                "Unexpected received frame type.\n");
            break;
    } //switch


    PHY_SetTxDataRateType(
        node,
        dot11->myMacData->phyNumber,
        dataRateType);

    switch (frameType) {
        case DOT11_DATA:
        case DOT11_CF_DATA_ACK:
            if (DEBUG_PCF)
            {
                 char timeStr[100];
                 ctoa(node->getNodeTime(), timeStr);
                 if (frameType == DOT11_DATA){
                 printf("\nStation %d sending DOT11_DATA "
                       "\t to PC at %s \n",
                        node->nodeId,
                        timeStr);
                 }
                 else {
                 printf("\n Station %d Sending DOT11_CF_DATA_ACK "
                       "\t to PC at %s \n",
                        node->nodeId,
                        timeStr);
                 }
            }
            MacDot11CfpStationTransmitUnicast(node, dot11,
                frameType, sendDelay);
            break;

        case DOT11_CF_NULLDATA:
        case DOT11_CF_ACK:

            if (DEBUG_PCF)
            {
                 char timeStr[100];
                 ctoa(node->getNodeTime(), timeStr);
                 if (frameType == DOT11_CF_NULLDATA){
                 printf("\nStation %d sending DOT11_CF_NULLDATA "
                       "\t to PC at %s \n",
                        node->nodeId,
                        timeStr);
                 }
                 else {
                 printf("\n Station %d Sending DOT11_CF_ACK "
                       "\t to PC at %s \n",
                        node->nodeId,
                        timeStr);
                 }
            }
            if (MacDot11CfpStationHasSufficientTime(node, dot11,
                    frameType, sendDelay, dataRateType))
                {
            MacDot11CfpStationTransmitAckOrNullData(node, dot11,
                frameType, sendDelay);
                }
            break;

        default:
            ERROR_ReportError("MacDot11CfpStationTransmit: "
                "Unknown frame type.\n");
            break;
    } //switch

}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationProcessUnicast
//  PURPOSE:     Receive a directed unicast frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received message
//  RETURN:      TRUE is unicast is acceptable
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
BOOL MacDot11CfpStationProcessUnicast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    BOOL unicastIsProcessed = FALSE;
    DOT11_FrameHdr* fHdr =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    Mac802Address sourceAddr = fHdr->sourceAddr;

    if (MacDot11StationCorrectSeqenceNumber(node, dot11, sourceAddr, fHdr->seqNo)) {

        MacDot11Trace(node, dot11, msg, "Receive");

        MacDot11UpdateStatsReceive(
            node,
            dot11,
            msg,
            dot11->myMacData->interfaceIndex);

        dot11->unicastPacketsGotCfp++;

        MacDot11StationHandOffSuccessfullyReceivedUnicast(node, dot11, msg);

        unicastIsProcessed = TRUE;
    }
    else {
        MacDot11Trace(node, dot11, NULL, "Drop, wrong sequence number");
#ifdef ADDON_DB
        HandleMacDBEvents(        
                node, msg, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif
        unicastIsProcessed = FALSE;
        MESSAGE_Free(node, msg);
    }

    MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);

    dot11->cfpAckIsDue = TRUE;

    return unicastIsProcessed;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcBeaconTransmitted
//  PURPOSE:     Set variables and states after the transmit of beacon
//               marking start of PCF. Set timer for next beacon.
//
//               Also set the self CF-End time to protect against
//               overlapping CFPs in overlapping BSSs.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11CfpPcBeaconTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11_PcVars* pc = dot11->pcVars;
    clocktype currentTime = node->getNodeTime();


    // If it is Fist CFP Beacon
  if ((dot11->apVars->lastDTIMCount == 0) && (dot11->pcVars->lastCfpCount == 0)){
        // Set the Cf End timer. In overlapping BSSs, balance
        // duration can finish without being able to send CF End
        MacDot11StationStartTimerOfGivenType(node, dot11,
                dot11->cfpEndTime - currentTime
                   + EPSILON_DELAY,
            MSG_MAC_DOT11_CfpEnd);
        dot11->cfpEndSequenceNumber = dot11->timerSequenceNumber;

        dot11->beaconPacketsSentCfp++;
        MacDot11CfpResetBackoffAndAckVariables(node, dot11);
        MacDot11StationSetState(node, dot11, DOT11_CFP_START);
        MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
        // Initialize poll list variables if applicable
        if (MacDot11CfpPcSendsPolls(pc)) {
            pc->allStationsPolled = FALSE;
            pc->firstPolledItem = NULL;
            pc->prevPolledItem = NULL;
            MacDot11CfPollListResetAtStartOfCfp(node, dot11);
        }
    }
    else {
        MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
    }
     dot11->cfpAckIsDue = FALSE;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationReceiveBroadcast
//  PURPOSE:     Receive a CF broadcast, either data or CF-End frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationReceiveBroadcast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11_LongControlFrame *fHdr =
        (DOT11_LongControlFrame*) MESSAGE_ReturnPacket(msg);
    Mac802Address sourceAddr = fHdr->sourceAddr;
    DOT11_MacFrameType frameType = (DOT11_MacFrameType) fHdr->frameType;

    switch (frameType) {
        case DOT11_CF_DATA_ACK:
            MacDot11CfpStationProcessAck(node, dot11, sourceAddr);
            // no break; continue to process DATA

        case DOT11_DATA:

            MacDot11Trace(node, dot11, msg, "Receive");
            MacDot11UpdateStatsReceive(
                node,
                dot11,
                msg,
                dot11->myMacData->interfaceIndex);
            dot11->broadcastPacketsGotCfp++;
            MacDot11StationHandOffSuccessfullyReceivedBroadcast(node, dot11, msg);

            break;

        case DOT11_CF_END_ACK:
            MacDot11CfpStationProcessAck(node, dot11, sourceAddr);
            // no break; continue to process DOT11_CF_END

        case DOT11_CF_END:
            MacDot11Trace(node, dot11, msg, "Receive");

            MacDot11CfpStationProcessEnd(node, dot11);

            MESSAGE_Free(node, msg);
            break;

        default:
            ERROR_ReportError("MacDot11CfpStationReceiveBroadcast: "
               "Unexpected frame type.\n");
            MESSAGE_Free(node, msg);
            break;
    } //switch
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationReceiveUnicast
//  PURPOSE:     Depending on the type of directed frame, process it
//               and arrange to respond appropriately
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpStationReceiveUnicast(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11_FrameHdr* fHdr =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    Mac802Address sourceAddr = fHdr->sourceAddr;
    DOT11_MacFrameType frameType = (DOT11_MacFrameType) fHdr->frameType;

    switch (frameType) {
        case DOT11_CF_DATA_POLL_ACK:
            MacDot11CfpStationProcessAck(node, dot11, sourceAddr);
            // no break; continue to process DATA

        case DOT11_CF_DATA_POLL:
            dot11->dataPollPacketsGotCfp++;
            if (DEBUG_PCF)
            {
                 char timeStr[100];
                 ctoa(node->getNodeTime(), timeStr);
                 if (frameType == DOT11_CF_DATA_POLL)
                 {
                    printf("\n%d Recieved DATA_POLL "
                       "\t  at %s \n",
                        node->nodeId,
                        timeStr);
                 }
                 else
                 {
                    printf("\n%d Recieved DATA_POLL_ACK "
                       "\t  at %s \n",
                        node->nodeId,
                        timeStr);
                 }
            }

            MacDot11CfpStationProcessUnicast(node, dot11, msg);

            MacDot11CfpStationTransmit(node, dot11, frameType, 0);
            break;

        case DOT11_CF_POLL_ACK:
            MacDot11CfpStationProcessAck(node, dot11, sourceAddr);
            // no break; continue to process DOT11_CF_POLL

        case DOT11_CF_POLL:
            if (DEBUG_PCF)
            {
                 char timeStr[100];
                 ctoa(node->getNodeTime(), timeStr);
                 if (frameType == DOT11_CF_POLL){
                 printf("\n%d Recieved POLL "
                       "\t  at %s \n",
                        node->nodeId,
                        timeStr);
                 }
                 else
                 {
                     printf("\n%d Recieved POLL_ACK "
                       "\t  at %s \n",
                        node->nodeId,
                        timeStr);
                 }
            }
            MacDot11Trace(node, dot11, msg, "Receive");

            dot11->pollPacketsGotCfp++;
            MESSAGE_Free(node, msg);

            MacDot11CfpStationTransmit(node, dot11, frameType, 0);
            break;

        case DOT11_CF_DATA_ACK:
            MacDot11CfpStationProcessAck(node, dot11, sourceAddr);
            // no break; continue to process DATA

        case DOT11_DATA:

            if (DEBUG_PCF)
            {
                 char timeStr[100];
                 ctoa(node->getNodeTime(), timeStr);
                 if (frameType == DOT11_DATA)
                 {
                 printf("\n%d Recieved DATA "
                       "\t  at %s \n",
                        node->nodeId,
                        timeStr);
                 }
                 else
                 {
                     printf("\n%d Recieved DATA_ACK "
                       "\t  at %s \n",
                        node->nodeId,
                        timeStr);
                 }

            }
            dot11->dataPacketsGotCfp++;
            MacDot11CfpStationProcessUnicast(node, dot11, msg);

            MacDot11CfpStationTransmit(node, dot11, frameType, 0);
            break;

        default:
            ERROR_ReportError("MacDot11CfpStationProcessUnicast: "
                "Received unknown unicast frame type.\n");
            break;
    } //switch
}



//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpStationInitiateCfPeriod
//  PURPOSE:     When a BSS station, which is not in PCF state,
//               receives a CF frame from within its BSS, it
//               implies that the beacon was missed.
//               It guesses a CFP duration and enters PCF state.
//               The guess is accurage if a beacon were previously received.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11CfpStationInitiateCfPeriod(
    Node* node,
    MacDataDot11* dot11)
{
    clocktype newNAV;
    clocktype currentTime = node->getNodeTime();

    // Protection against first CF Start Beacon after Association
     if (dot11->lastCfpBeaconTime == 0){
            newNAV = currentTime + dot11->cfpMaxDuration +
                                                EPSILON_DELAY;
     }

    // Protect against previous missed cf Start beacons
    else{
        while (dot11->lastCfpBeaconTime + dot11->cfpMaxDuration <
                                                        currentTime)
    {
            dot11->lastCfpBeaconTime += dot11->cfpRepetionInterval;
    }

        newNAV = dot11->lastCfpBeaconTime + dot11->cfpMaxDuration
                                                + EPSILON_DELAY;
    }

    if (newNAV > dot11->NAV) {
        dot11->NAV = newNAV;
    }
    else if (newNAV < dot11->NAV) {
        MacDot11Trace(node, dot11, NULL, "New guessed NAV < current NAV.");
    }
    // Enter PCF if station is pollable
    if (dot11->pollType ==DOT11_STA_POLLABLE_POLL){
    MacDot11CfpResetBackoffAndAckVariables(node, dot11);
    MacDot11StationSetState(node, dot11, DOT11_CFP_START);
    MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
    dot11->cfpEndTime = dot11->lastCfpBeaconTime + dot11->cfpMaxDuration;
    // Start the end timer
    MacDot11StationStartTimerOfGivenType(node, dot11,
        newNAV - currentTime, MSG_MAC_DOT11_CfpEnd);
    dot11->cfpEndSequenceNumber = dot11->timerSequenceNumber;
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPcCheckBeaconOverlap
//  PURPOSE:     Checks to see if the CFP would would overlap with
//               its own, and, if so, issues a warning message.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received beacon message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11CfpPcCheckBeaconOverlap(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11_BeaconFrame *beacon =
        (DOT11_BeaconFrame*) MESSAGE_ReturnPacket(msg);
    DOT11_CFParameterSet cfSet;
    DOT11_TIMFrame timFrame = {0};
    if (Dot11_GetCfsetFromBeacon(node, dot11, msg, &cfSet))
    {
      int offset = ( sizeof(DOT11_BeaconFrame)+ sizeof(DOT11_CFParameterSet)+
      DOT11_IE_HDR_LENGTH);
      Dot11_GetTimFromBeacon(node, dot11, msg, offset, &timFrame);
      dot11->DTIMCount =  timFrame.dTIMCount;
    }

    clocktype cfpMaxDuration =
    MacDot11TUsToClocktype (cfSet.cfpMaxDuration);
    clocktype cfpBalanceDuration =
       MacDot11TUsToClocktype ((unsigned short)cfSet.cfpBalDuration);
    dot11->cfpRepetionInterval = (dot11->pcVars->cfSet.cfpPeriod *
                             dot11->apVars->dtimPeriod *
                             dot11->beaconInterval);
    // Check that CFPs would not overlap
    if (cfSet.cfpCount == 0 && timFrame.dTIMCount == 0)
    {

        dot11->NextCfpBeaconTime = dot11->lastCfpBeaconTime +
                            dot11->cfpRepetionInterval;
        if (dot11->NextCfpBeaconTime < node->getNodeTime() + cfpBalanceDuration)
        {
            MacDot11Trace(node, dot11, NULL,
                "WARNING Overlapping CFPs detected.");

            ERROR_ReportWarning(
                "Neighbouring PCs have overlapping Contention Free periods.\n"
                "Use MAC-DOT11-BEACON-START-TIME for PC coordination.\n");
        }

            // Check that there is enough time for DCF.
        if (cfpMaxDuration + dot11->cfpMaxDuration >=
                                      (dot11->pcVars->cfSet.cfpPeriod *
                                      dot11->apVars->dtimPeriod *
                                      dot11->beaconInterval)) {

            MacDot11Trace(node, dot11, NULL,
                "WARNING Overlapping PCF duration does not leave "
                "time for DCF.");

            ERROR_ReportWarning(
                "The PCF duration of neighbouring PCs do not leave time "
                "for the contention period.\n");
        }
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpProcessNotMyFrame
//  PURPOSE:     Process a non-directed DCF frame received during CFP.
//               Set NAV if a BSS station.
//               Assume that it cannot happen for a PC.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               clocktype duration
//                  Duration of DCF frame
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11CfpProcessNotMyFrame(
    Node* node,
    MacDataDot11* dot11,
    clocktype duration)
{
    clocktype currentTime;
    clocktype newNAV;

    if (duration == DOT11_CF_FRAME_DURATION * MICRO_SECOND) {
        return;
    }

    currentTime = node->getNodeTime();
    newNAV = currentTime + duration + EPSILON_DELAY;
    if (newNAV > dot11->NAV) {
        dot11->NAV = newNAV;
    }
}



//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpHandleTimeout
//  PURPOSE:     Handle timeouts during CFP depending on state and
//               occurance at PC or station.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
//static
void MacDot11CfpHandleTimeout(
    Node* node,
    MacDataDot11* dot11)
{
    switch (dot11->cfpState) {
        case DOT11_S_CFP_NONE:
            ERROR_Assert(MacDot11IsPC(dot11),
                "MacDot11CfpHandleTimeout: "
                "Unexpected station timeout in CFP_NONE state.\n");
            MacDot11CfpPcTransmit(node, dot11, DOT11_CF_NONE,
                dot11->txSifs);
            break;

        case DOT11_S_CFP_WF_ACK:
            ERROR_Assert(dot11->dataRateInfo->ipAddress ==
                         dot11->currentNextHopAddress,
                         "MacDot11CfpHandleTimeout: "
                         "Address does not match.\n");

            MacDot11StationResetAckVariables(dot11);

            if (MacDot11IsPC(dot11))
            {
                MacDot11Trace(node, dot11, NULL,
                    "PC timeout waiting for ack.");

                dot11->dataRateInfo->numAcksFailed++;
                MacDot11StationAdjustDataRateForResponseTimeout(node, dot11);

                // Continue with transmit
                MacDot11CfpPcTransmit(node, dot11, DOT11_CF_NONE, 0);
            }
            else
            {
                MacDot11Trace(node, dot11, NULL,
                    "Station timeout waiting for ack.");

                // For stations, wait for next opportunity to send the frame
                MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
            }
            break;

        case DOT11_S_CFP_WF_DATA_ACK:
            ERROR_Assert(dot11->dataRateInfo->ipAddress ==
                         dot11->currentNextHopAddress,
                         "MacDot11CfpHandleTimeout: "
                         "Address does not match.\n");

            dot11->dataRateInfo->numAcksFailed++;
            MacDot11StationAdjustDataRateForResponseTimeout(node, dot11);

            MacDot11StationResetAckVariables(dot11);

            // no break

        case DOT11_S_CFP_WF_DATA:
            ERROR_Assert(MacDot11IsPC(dot11),
                "MacDot11CfpHandleTimeout: "
                "Non-PC times out waiting for data.\n");

            MacDot11Trace(node, dot11, NULL,
                "PC timeout waiting for data (+ack).");

            // Continue with transmit
            MacDot11CfpPcTransmit(node, dot11, DOT11_CF_NONE, 0);
            break;
        case DOT11_S_CFP_WFBEACON:
               MacDot11ApTransmitBeacon(node, dot11);
               ERROR_Assert( MacDot11IsPC(dot11),
                "MacDot11HandleTimeout: "
                "Wait For Beacon state for an PC or non-BSS station.\n");
               break;

        default:
            ERROR_ReportError("MacDot11CfpHandleTimeout: "
                "Unknown CFP timeout.\n");
            break;
    } //switch
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpCancelSomeTimersBecauseChannelHasBecomeBusy
//  PURPOSE:     To cancel timers when channel changes from idle to
//               receiving state.
//
//               Note that unlike the corresponding function for DCF
//               states, this function is not called when channel changes
//               from idle to sensing state.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
//static
void MacDot11CfpCancelSomeTimersBecauseChannelHasBecomeBusy(
    Node* node,
    MacDataDot11* dot11)
{
    switch (dot11->cfpState) {
        case DOT11_S_CFP_NONE:
            break;

        case DOT11_S_CFP_WF_DATA:
        case DOT11_S_CFP_WF_DATA_ACK:
        case DOT11_S_CFP_WFBEACON:
            // Assume that the PC is getting the expected frame.
            MacDot11StationCancelTimer(node, dot11);
            MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
            break;

        case DOT11_S_CFP_WF_ACK:
            // Assume that the expected frame is coming in.
            MacDot11StationCancelTimer(node, dot11);
            MacDot11CfpSetState(dot11, DOT11_S_CFP_NONE);
            break;

        default:
            break;
    } //switch
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPhyStatusIsNowIdleStartSendTimers
//  PURPOSE:     To restart send once the channel transits from busy
//               to idle. Applied only to a PC during PCF.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11CfpPhyStatusIsNowIdleStartSendTimers(
    Node* node,
    MacDataDot11* dot11)
{
    if (MacDot11IsPC(dot11)) {
        switch (dot11->cfpState) {
            case DOT11_S_CFP_NONE:
                // Start a SIFS timer
                MacDot11StationStartTimer(node, dot11, dot11->txSifs);
                break;

            case DOT11_S_CFP_WF_DATA:
            case DOT11_S_CFP_WF_ACK:
            case DOT11_S_CFP_WF_DATA_ACK:
                break;

            default:
                ERROR_ReportError(
                    "MacDot11CfpPhyStatusIsNowIdleStartSendTimers: "
                    "Unexpected busy state.\n");
                break;
        } //switch
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpReceivePacketFromPhy
//  PURPOSE:     Receive a packet from PHY while in CFP state.
//               Process the frame depending on whether it is a
//               directed unicast or broadcast.
//               Call the appropriate routine for a DCF frame type.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11CfpReceivePacketFromPhy(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    DOT11_ShortControlFrame* sHdr =
        (DOT11_ShortControlFrame*) MESSAGE_ReturnPacket(msg);
    Mac802Address destAddr = sHdr->destAddr;
    DOT11_MacFrameType frameType = (DOT11_MacFrameType) sHdr->frameType;

    ERROR_Assert(!MacDot11CfpIsInTransmitState(dot11->cfpState),
        "MacDot11CfpReceivePacketFromPhy: "
        "Receipt of frame while in transmit state.\n");

    if (destAddr == dot11->selfAddr) {
        switch (frameType) {
            case DOT11_CF_DATA_POLL_ACK:
            case DOT11_CF_DATA_POLL:
            case DOT11_CF_POLL_ACK:
            case DOT11_CF_POLL:
                if (!MacDot11IsFrameFromMyAP(dot11,
                    ((DOT11_LongControlFrame *)MESSAGE_ReturnPacket(msg))->sourceAddr))
                {

                ERROR_ReportWarning(
                    "MacDot11CfpReceivePacketFromPhy: \n"
                    "Directed data and/or Poll frame is not from PC.\n");
                }
                else
                {

                    MacDot11CfpStationReceiveUnicast(node, dot11, msg);
                }
                break;

            case DOT11_DATA:
            case DOT11_CF_DATA_ACK:
                if (MacDot11IsFrameFromMyAP(dot11,
                    ((DOT11_LongControlFrame *)MESSAGE_ReturnPacket(msg))
                    ->sourceAddr))
                {
                    MacDot11CfpStationReceiveUnicast(node, dot11, msg);
                }
                else {
                    ERROR_Assert(MacDot11IsPC(dot11),
                        "MacDot11CfpReceivePacketFromPhy: "
                        "Non-PC receives unexpected data during CFP.\n");
                    MacDot11CfpPcReceiveUnicast(node, dot11, msg);
                }
                break;

            case DOT11_CF_NULLDATA:
                ERROR_Assert(MacDot11IsFrameFromStationInMyBSS(dot11,
                    destAddr, sHdr->frameFlags),
                    "MacDot11CfpReceivePacketFromPhy: "
                    "Directed CF Null Data frame not from BSS.\n");

                MacDot11CfpPcReceiveUnicast(node, dot11, msg);
                break;

            case DOT11_CF_ACK:
                ERROR_Assert(MacDot11IsFrameFromStationInMyBSS(dot11,
                    destAddr, sHdr->frameFlags),
                    "MacDot11CfpReceivePacketFromPhy: "
                    "Directed CF Ack frame not from BSS.\n");

                MacDot11CfpPcReceiveUnicast(node, dot11, msg);
                break;

            case DOT11_ACK:
                if (MacDot11IsPC(dot11))
                {
                    if (dot11->cfpPrevState != DOT11_S_CFP_WF_ACK)
                    {
                     ERROR_ReportWarning(
                    "Neighbouring PCs have overlapping Contention Free periods.\n"
                    "Use MAC-DOT11-BEACON-START-TIME for PC coordination.\n");
                    }
                MacDot11Trace(node, dot11, msg, "Receive");

                // Tacit assumption that source is correct.
                MacDot11CfpPcProcessAck(node, dot11,
                    dot11->waitingForAckOrCtsFromAddress, TRUE);

                MacDot11CfpPcTransmit(node, dot11, frameType, 0);
                }

                MESSAGE_Free(node, msg);
                break;

            default:
                MacDot11Trace(node, dot11, NULL,
                    "Ignored directed frame during CFP");
#ifdef ADDON_DB
                HandleMacDBEvents(        
                node, msg, 
                node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                dot11->myMacData->interfaceIndex, MAC_Drop, 
                node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

                MESSAGE_Free(node, msg);
                break;
        } //switch
    }
    else if (destAddr == ANY_MAC802) {
        switch (frameType) {
            case DOT11_DATA:
            case DOT11_CF_DATA_ACK:
                // Can be broadcast from PC or broadcast from outside BSS
                ERROR_Assert(!(MacDot11IsPC(dot11)
                    && MacDot11IsFrameFromStationInMyBSS(dot11,
                            destAddr, sHdr->frameFlags)),
                    "MacDot11CfpReceivePacketFromPhy: "
                    "PC receives broadcast frame from BSS.\n");

                if (MacDot11IsMyBroadcastDataFrame(node,dot11, msg)) {
                    if (MacDot11IsBssStation(dot11)) {
                        MacDot11CfpStationReceiveBroadcast(
                            node, dot11, msg);
                    }
                    else if (MacDot11IsPC(dot11)) {

                        MacDot11Trace(node, dot11, msg, "Receive");
                        MacDot11UpdateStatsReceive(
                            node,
                            dot11,
                            msg,
                            dot11->myMacData->interfaceIndex);
                        dot11->broadcastPacketsGotCfp++;
                        MacDot11StationHandOffSuccessfullyReceivedBroadcast(
                            node, dot11, msg);
                    }
                }
                else {
                    MacDot11Trace(node, dot11, NULL,
                        "Drop, broadcast not for me");
#ifdef ADDON_DB
                    HandleMacDBEvents(        
                        node, msg, 
                        node->macData[dot11->myMacData->interfaceIndex]->phyNumber,
                        dot11->myMacData->interfaceIndex, MAC_Drop, 
                        node->macData[dot11->myMacData->interfaceIndex]->macProtocol);
#endif

                    dot11->waitingForAckOrCtsFromAddress = INVALID_802ADDRESS;

                    MESSAGE_Free(node, msg);
                }
                break;

            case DOT11_BEACON:
             {MacDot11Trace(node, dot11, msg, "Receive");
              //dot11->beaconPacketsGotCfp++;
                    DOT11_BeaconFrame *beacon =
                        (DOT11_BeaconFrame*) MESSAGE_ReturnPacket(msg);
                    clocktype newNAV = 0;
              dot11->beaconsMissed = 0;
              DOT11_CFParameterSet cfSet;
              DOT11_TIMFrame timFrame;

              if (Dot11_GetCfsetFromBeacon(node, dot11, msg, &cfSet))
              {

              // If Cf set is present then set Offset to Point Tim frame
              int offset = ( sizeof(DOT11_BeaconFrame)+ sizeof(
                              DOT11_CFParameterSet)+ DOT11_IE_HDR_LENGTH);
              Dot11_GetTimFromBeacon(node, dot11, msg, offset, &timFrame);
              dot11->DTIMCount =  timFrame.dTIMCount;

              int dataRateType = PHY_GetRxDataRateType(node, dot11->myMacData->phyNumber);

              clocktype beaconTransmitTime = node->getNodeTime() -
                                        PHY_GetTransmissionDuration(
                                            node,
                                            dot11->myMacData->phyNumber,
                                            dataRateType,
                                            MESSAGE_ReturnPacketSize(msg));

                newNAV = beaconTransmitTime;

                if ((unsigned short)cfSet.cfpBalDuration > 0)
                {
                    newNAV += MacDot11TUsToClocktype
                       ((unsigned short)cfSet.cfpBalDuration)+ EPSILON_DELAY;
                }
             }

              if (MacDot11IsBssStation(dot11)){
                    if (newNAV > dot11->NAV) {
                        dot11->NAV = newNAV;
                    }
                 if (MacDot11IsFrameFromMyAP(dot11,
                     ((DOT11_LongControlFrame *)MESSAGE_ReturnPacket(msg))
                     ->sourceAddr)){
                    dot11->beaconTime = beacon->timeStamp;
                    MacDot11StationCancelBeaconTimer(node, dot11);
                    MacDot11StationCancelTimer(node, dot11);
                    dot11->beaconInterval =MacDot11TUsToClocktype
                        ((unsigned short)beacon->beaconInterval);
                    MacDot11StationStartBeaconTimer(node, dot11);
                 }
                 if (DEBUG_BEACON) {
                    char timeStr[100];

                    ctoa(node->getNodeTime(), timeStr);

                    printf("%d RECEIVED beacon frame at %s\n", node->nodeId,
                        timeStr);
                }
            }
      // If it is AP and do not have PC functionality then set NAV
            else if ((MacDot11IsAp(dot11)) && (!MacDot11IsPC(dot11)))
            {
                if (newNAV > dot11->NAV)
                {
                    dot11->NAV = newNAV;
                  }

                }
                else {
                    MacDot11Trace(node, dot11, NULL,
                        "PC receives beacon while in CFP.");

                    ERROR_ReportWarning("PC receives beacon while in CFP. "
                        "Overlapping PCF periods should be avoided.\n");
                }

                MESSAGE_Free(node, msg);
                break;
            }
            case DOT11_CF_END:
            case DOT11_CF_END_ACK:

                if (MacDot11IsFrameFromMyAP(dot11,
                    ((DOT11_LongControlFrame *)MESSAGE_ReturnPacket(msg))
                    ->sourceAddr))
                {
                    MacDot11CfpStationReceiveBroadcast(node, dot11, msg);
                }
                else {
                    // CF End from outside BSS while in CFP.
                    //  There is no track of which beacon had
                    //  what beacon interval.
                    //  Ignore the frame.
                    //
                    MacDot11Trace(node, dot11, NULL,
                        "CF End from outside BSS during CFP");
                    dot11->NAV = 0;
                    if (DEBUG_PCF && DEBUG_BEACON){
                          MacDot11Trace(node, dot11, NULL,
                         "Station recieved CFP End and"
                         "Reset NAV ");
                    }
                    MESSAGE_Free(node, msg);
                }
                break;

            default:
                if (MacDot11IsPC(dot11)){

                  MacDot11ManagementCheckFrame(node, dot11, msg);
                }
                MESSAGE_Free(node, msg);
                break;
        } //switch
    }
    else {
            if (MacDot11IsStationNonAPandJoined(dot11)){
            if (MacDot11IsCfFrameAnAckType(frameType) && MacDot11IsFrameFromMyAP(dot11,
                    ((DOT11_LongControlFrame *)MESSAGE_ReturnPacket(msg))
                    ->sourceAddr))
            {
                MacDot11CfpStationProcessAck(node, dot11,
                    ((DOT11_LongControlFrame *)MESSAGE_ReturnPacket(msg))
                    ->sourceAddr);
            }
        }

        MacDot11CfpProcessNotMyFrame(node, dot11,
            MacDot11MicroToNanosecond(sHdr->duration));

        dot11->waitingForAckOrCtsFromAddress = INVALID_802ADDRESS;

        if (dot11->myMacData->promiscuousMode
            && MacDot11IsFrameAUnicastDataTypeForPromiscuousMode(
                    frameType, msg))
        {
            DOT11_FrameHdr *fHdr =
                (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);

            MacDot11HandlePromiscuousMode(node, dot11, msg,
                fHdr->sourceAddr, fHdr->destAddr);
        }

        MESSAGE_Free(node,msg);
    }
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpFrameTransmitted
//  PURPOSE:     Once PHY indicates that a frame has been transmitted
//               during CFP, call the appropriate function for post
//               transmit processing.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
//static
void MacDot11CfpFrameTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    switch (dot11->cfpState) {

    case DOT11_X_CFP_UNICAST:
        if (MacDot11IsPC(dot11)) {
            MacDot11CfpPcUnicastTransmitted(node, dot11);
        }
        else {
            MacDot11CfpStationUnicastTransmitted(node, dot11);
        }
        break;

    case DOT11_X_CFP_BROADCAST:
        MacDot11CfpPcBroadcastTransmitted(node, dot11);
        break;

    case DOT11_X_CFP_BEACON:
        MacDot11ApBeaconTransmitted(node, dot11);
        break;

    case DOT11_X_CFP_ACK:
        if (MacDot11IsPC(dot11)) {
            MacDot11CfpPcAckTransmitted(node, dot11);
        }
        else {
            MacDot11CfpStationAckTransmitted(node, dot11);
        }
        break;

    case DOT11_X_CFP_POLL:
        MacDot11CfpPcPollTransmitted(node, dot11);
        break;

    case DOT11_X_CFP_UNICAST_POLL:
        MacDot11CfpPcUnicastPollTransmitted(node, dot11);
        break;

    case DOT11_X_CFP_NULLDATA:
        MacDot11CfpStationNullDataTransmitted(node, dot11);
        break;

    case DOT11_X_CFP_END:
        MacDot11CfpPcEndTransmitted(node, dot11);
        break;

    default:
        ERROR_ReportError("MacDot11CfpFrameTransmitted: "
            "Unknown CFP state.\n");
        break;

    } //switch
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpInit
//  PURPOSE:     Initialize PCF related variables.
//               For an AP, read additional user inputs
//
//  PARAMETERS:  Node* node
//                  Pointer to current node
//               const NodeInput* nodeInput
//                  Pointer to node input
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               SubnetMemberData* subnetList
//                  Pointer to subnet list
//               int nodesInSubnet
//                  Number of nodes in subnet
//               int subnetListIndex
//                  Subnet list index
//               NodeAddress subnetAddress
//                  Subnet address
//               int numHostBits
//                  Number of host bits
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11CfpInit(
    Node* node,
    const NodeInput* nodeInput,
    MacDataDot11* dot11,
    SubnetMemberData* subnetList,
    int nodesInSubnet,
    int subnetListIndex,
    NodeAddress subnetAddress,
    int numHostBits,
    NetworkType networkType,
    in6_addr* ipv6SubnetAddr,
    unsigned int prefixLength)
{
    char input[MAX_STRING_LENGTH];
    BOOL wasFound = FALSE;
    BOOL hasCfp = FALSE;
    Address address;

    dot11->pifs = dot11->sifs + dot11->slotTime;
    dot11->txPifs = dot11->txSifs + dot11->slotTime;

    dot11->cfpState = DOT11_S_CFP_NONE;
    dot11->cfpPrevState = DOT11_S_CFP_NONE;

    dot11->cfpTimeout = 2 * dot11->extraPropDelay + dot11->pifs;

    dot11->cfpLastFrameType = DOT11_CF_NONE;

     NetworkGetInterfaceInfo(
        node,
        dot11->myMacData->interfaceIndex,
        &address,
        networkType);

        if (MacDot11IsPC(dot11))
        {
            // Initialize PC related variables
            dot11->pcVars = (DOT11_PcVars*) MEM_malloc(sizeof(DOT11_PcVars));
            ERROR_Assert(dot11->pcVars != NULL,
                "MacDot11ApInit: Unable to allocate PC structure.\n");

            memset(dot11->pcVars, 0, sizeof(DOT11_PcVars));

            dot11->pcVars->broadcastsHavePriority =
                DOT11_BROADCASTS_HAVE_PRIORITY_DURING_CFP;
            dot11->pcVars->cfPollList = NULL;
            dot11->pcVars->pollableCount = 0;
            dot11->pcVars->firstPolledItem = NULL;
            dot11->pcVars->prevPolledItem = NULL;
            dot11->pcVars->allStationsPolled = FALSE;
            dot11->pcVars->isBeaconDelayed = FALSE;

            if (networkType == NETWORK_IPV6)
            {
                MacDot11CfpPcInit(node, nodeInput, dot11,
                        subnetList, nodesInSubnet, subnetListIndex,
                        0, 0,
                        networkType, ipv6SubnetAddr, prefixLength);
            }
            else
            {
                MacDot11CfpPcInit(node, nodeInput, dot11,
                        subnetList, nodesInSubnet, subnetListIndex,
                        subnetAddress, numHostBits);
            }
        }

    else if (MacDot11IsBssStation(dot11))
    {
        //staion poll type
        NetworkGetInterfaceInfo(
                node,
                dot11->myMacData->interfaceIndex,
                &address,
                networkType);

        IO_ReadString(
            node,
            node->nodeId,
            dot11->myMacData->interfaceIndex,
            nodeInput,
            "MAC-DOT11-STATION-POLL-TYPE",
            &wasFound,
            input);

        if (wasFound)
        {
            if (!strcmp(input, "NOT-POLLABLE"))
            {
                dot11->pollType = DOT11_STA_NOT_POLLABLE;
                dot11->cfPollable = 0;
                dot11->cfPollRequest = 0 ;
                hasCfp = TRUE;
            }
            else if (!strcmp(input, "POLLABLE-DONT-POLL"))
            {
               dot11->pollType = DOT11_STA_POLLABLE_DONT_POLL;
                dot11->cfPollable = 1;
                dot11->cfPollRequest = 0;
                hasCfp = TRUE;
            }
            else if (!strcmp(input, "POLLABLE"))
            {
                dot11->pollType = DOT11_STA_POLLABLE_POLL;
                dot11->cfPollable = 1;
                dot11->cfPollRequest = 1;
                hasCfp = TRUE;
            }
            else
            {
                ERROR_ReportError(
                    "MacDot11cfpCreateEmptyPollList: "
                    "Invalid value for MAC-DOT11-STATION-POLL-TYPE "
                    "in configuration file.\n"
                    "Expecting NOT-POLLABLE | POLLABLE-DONT-POLL |"
                    "POLLABLE \n");
            }
        }
        else
        {
            dot11->cfPollable = 1;
            dot11->cfPollRequest = 1;
            dot11->pollType = DOT11_STATION_POLL_TYPE_DEFAULT;
            hasCfp = TRUE;
        }
    dot11->printCfpStatistics = FALSE;

        if (hasCfp)
        {
            // Additional PCF statistics can be output with
            //     MAC-DOT11-STATION-PCF-STATISTICS YES | NO
            // Default is DOT11_PRINT_STATS_DEFAULT
            // Stat values vary for different types of stations
            IO_ReadString(
                node,
                node->nodeId,
                dot11->myMacData->interfaceIndex,
                nodeInput,
                "MAC-DOT11-STATION-PCF-STATISTICS",
                &wasFound,
                input);

            if (wasFound) {
                if (!strcmp(input, "YES")) {
                    dot11->printCfpStatistics = TRUE;
                }
                else if (!strcmp(input, "NO")) {
                    dot11->printCfpStatistics = FALSE;
                }
                else {
                    ERROR_ReportError("MacDot11CfpInit: "
                        "Invalid value for MAC-DOT11-PCF-STATISTICS "
                        "in configuration file.\n"
                        "Expecting YES or NO.\n");
                }
            }
            else {
                dot11->printCfpStatistics = DOT11_PRINT_STATS_DEFAULT;
            }
        }
    }
}



//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpPrintStats
//  PURPOSE:     Print PCF related statistics.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to DOT11 structure
//               int interfaceIndex
//                  interfaceIndex, Interface index
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
//static
void MacDot11CfpPrintStats(
    Node* node,
    MacDataDot11* dot11,
    int interfaceIndex)
{
    // Print PCF stats for PC and BSS station.
    // Skip stats if station is not part of coordinated subnet
    if (!dot11->printCfpStatistics) {
        return;
    }

    if (MacDot11IsPC(dot11)) {
        MacDot11CfpPcPrintStats(node, dot11, interfaceIndex);
    }
    else if (MacDot11IsBssStation(dot11))
    {
        MacDot11CfpStationPrintStats(node, dot11, interfaceIndex);
    }
}
//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpHandleEndTimer
//  PURPOSE:     Handle a timer if a PC was unable to send a CF End
//               or a STA did not receive a CF End
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11CfpHandleEndTimer(
    Node* node,
    MacDataDot11* dot11)
{
    if (!MacDot11IsInCfp(dot11->state)) {
        return;
    }

    if (MacDot11IsPC(dot11)) {
        MacDot11Trace(node, dot11, NULL, "PC CF End timer");

        // CFP end timer expires but unable to send CF End.
        // Switch to DCF state.
        // if already CF end transmision has started then do not change
        // the states
        if (dot11->cfpState != DOT11_X_CFP_END)
        {
            MacDot11CfpSetEndVariables(node, dot11);
            MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
        }
    }
    else if (MacDot11IsBssStation(dot11)) {
        MacDot11Trace(node, dot11, NULL, "Station CF End timer");

        MacDot11CfpSetEndVariables(node, dot11);
        MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11CfpFreeStorage.
//  PURPOSE:     Free memory at the end of simulation.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               int interfaceIndex
//                  interfaceIndex, Interface index
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11CfpFreeStorage(
    Node* node,
    int interfaceIndex)
{
    MacDataDot11* dot11 =
        (MacDataDot11*)node->macData[interfaceIndex]->macVar;

    if (MacDot11IsPC(dot11)) {
        DOT11_PcVars* pc = dot11->pcVars;

        // Clear CF Polling list
        if (MacDot11CfpPcSendsPolls(pc)) {
            MacDot11CfPollListFree(node, dot11, pc->cfPollList);
        }

        MEM_free(pc);
    }
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11ProcessCfpEnd
//  PURPOSE:     Process a CF-End frame when not in CFP.
//               The frame can be received by a IBSS station or
//               station within a BSS, including AP.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Received message
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11ProcessCfpEnd(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    MacDot11Trace(node, dot11, msg, "Receive");

    // Do not increment if not from its AP
    if (MacDot11IsAp(dot11))
    {
        dot11->NAV = 0;
    }
      // If it is BSS
    else {
        if (MacDot11IsFrameFromMyAP(dot11,
                    ((DOT11_LongControlFrame*)MESSAGE_ReturnPacket(msg))
                                                    ->sourceAddr))
        {
            dot11->endPacketsGotCfp++;
            MacDot11CfpSetEndVariables(node, dot11);
            if (dot11->currentMessage != NULL) {
                MacDot11StationSetBackoffIfZero(node, dot11, dot11->currentACIndex);
            }

        }
    dot11->NAV = 0;
    }
}

