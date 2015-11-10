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
 * \file mac_dot11-ap.cpp
 * \brief Access Point routines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "network_ip.h"
#include "mac_dot11-sta.h"
#include "mac_dot11-ap.h"
#include "mac_dot11-mgmt.h"
#include "mac_dot11-hcca.h"
#include "mac_dot11s-frames.h"
#include "mac_dot11-pc.h"
#include "phy_802_11.h"
#include "mac_phy_802_11n.h"
#ifdef ENTERPRISE_LIB
#include "mpls_shim.h"
#endif // ENTERPRISE_LIB

//-------------------------------------------------------------------------
// STATIC FUNCTIONS
//-------------------------------------------------------------------------
static // inline//
void MacDot11SetBit(unsigned char* data, unsigned int associationId);

//--------------------------------------------------------------------------
//  NAME:        MacDot11ApStationListInit
//  PURPOSE:     Initialize the station link list data structure.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_ApStationList **list
//                  Pointer to polling list
//  RETURN:      Poll list with memory allocated
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11ApStationListInit(
    Node* node,
    MacDataDot11* dot11,
    DOT11_ApStationList** list)
{
    DOT11_ApStationList* tmpList;

    tmpList = (DOT11_ApStationList*) MEM_malloc(sizeof(DOT11_ApStationList));

    tmpList->size = 0;
    tmpList->first = tmpList->last = NULL;
    *list = tmpList;
}// MacDot11ApStationListInit


//--------------------------------------------------------------------------
//  NAME:        MacDot11ApStationListEmpty
//  PURPOSE:     See if the list is empty.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_ApStationList* list
//                  Pointer to station list
//  RETURN:      TRUE if size is zero
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11ApStationListIsEmpty(
    Node* node,
    MacDataDot11* dot11,
    DOT11_ApStationList* list)
{
    return (list->size == 0);
}// MacDot11ApStationListIsEmpty


//--------------------------------------------------------------------------
//  NAME:        MacDot11ApStationListGetSize
//  PURPOSE:     Query the size of station list
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      Number of items in station list
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
int MacDot11ApStationListGetSize(
    Node* node,
    MacDataDot11* dot11)
{
    return dot11->apVars->apStationList->size;
}// MacDot11ApStationListGetSize


//--------------------------------------------------------------------------
//  NAME:        MacDot11ApStationListInsert
//  PURPOSE:     Insert an item to the end of the list.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_ApStationList* list
//                  Pointer to polling list
//               DOT11_ApStation* data
//                  Poll station data for item to insert
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11ApStationListInsert(
    Node* node,
    MacDataDot11* dot11,
    DOT11_ApStationList* list,
    DOT11_ApStation* data)
{
    DOT11_ApStationListItem* listItem =
        (DOT11_ApStationListItem*)
        MEM_malloc(sizeof(DOT11_ApStationListItem));

    listItem->data = data;

    listItem->next = NULL;

    if (list->size == 0)
    {
        // Only item in the list
        listItem->prev = NULL;
        list->last = listItem;
        list->first = list->last;
    }
    else
    {

        // Insert at end of list
        listItem->prev = list->last;
        list->last->next = listItem;
        list->last = listItem;
    }

    list->size++;
}// MacDot11ApStationListInsert


//--------------------------------------------------------------------------
//  NAME:        MacDot11ApStationListItemRemove
//  PURPOSE:     Remove an item from the station list.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_ApStationList* list
//                  Pointer to polling list
//               DOT11_ApStationList* listItem
//                  List item to remove
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11ApStationListItemRemove(
    Node* node,
    MacDataDot11* dot11,
    DOT11_ApStationList* list,
    DOT11_ApStationListItem* listItem)
{
    DOT11_ApStationListItem* nextListItem;

    if (list == NULL || listItem == NULL) {
        return;
    }

    if (list->size == 0) {
        return;
    }

    if (listItem->data != NULL && MacDot11IsPC(dot11))
    {
        DOT11_CfPollListItem* pcStationItem;
        DOT11_PcVars* pc = dot11->pcVars;

        pcStationItem =
            MacDot11CfPollListGetItemWithGivenAddress(
            node,
            dot11,
            listItem->data->macAddr);
        if (pcStationItem != NULL)
        {
            MacDot11CfPollListItemRemove(
                        node,
                        dot11,
                        pc->cfPollList,
                        pcStationItem);
            pc->pollableCount--;
        }
    }
    if (listItem->data != NULL &&
        listItem->data->isSTAInPowerSaveMode == TRUE)
    {
        // Delete the PS mode queue
        //Drop Buffered packet
        while (listItem->data->noOfBufferedPacket > 0)
        {
            Message* msg = NULL;
            BOOL isMsgRetrieved = FALSE;
            isMsgRetrieved = listItem->data->queue->retrieve(
                &msg,
                0,
                DEQUEUE_PACKET,
                node->getNodeTime());
            if (isMsgRetrieved)
            {
                // count in drop
                dot11->psModeMacQueueDropPacket++;
                listItem->data->noOfBufferedPacket--;
                MESSAGE_Free(node, msg);
            }
            else
            {
                break;
            }
        }
        delete listItem->data->queue;
    }// end of power save mode if


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
}// MacDot11ApStationListItemRemove


//--------------------------------------------------------------------------
//  NAME:        MacDot11ApStationListFree
//  PURPOSE:     Free the entire list.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_ApStationList* list
//                  Pointer to station list
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11ApStationListFree(
    Node* node,
    MacDataDot11* dot11,
    DOT11_ApStationList* list)
{
    DOT11_ApStationListItem *item;
    DOT11_ApStationListItem *tempItem;

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
}// MacDot11ApStationListFree


//--------------------------------------------------------------------------
//  NAME:        MacDot11ApStationListGetFirst
//  PURPOSE:     Retrieve the first item in the station list
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      First list item
//               NULL for empty list
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
DOT11_ApStationListItem * MacDot11ApStationListGetFirst(
    Node* node,
    MacDataDot11* dot11)
{
    return dot11->apVars->apStationList->first;
}// MacDot11ApStationListGetFirst


//--------------------------------------------------------------------------
//  NAME:        MacDot11ApStationListGetNextItem
//  PURPOSE:     Retrieve the next item given the previous item.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_ApStationListItem *prev
//                  Pointer to prev item.
//  RETURN:      First item when previous item is NULL
//                          when previous item is the last
//                          when there is only one item in the list
//               NULL if the list is empty
//               Next item after previous item
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
DOT11_ApStationListItem * MacDot11ApStationListGetNextItem(
    Node* node,
    MacDataDot11* dot11,
    DOT11_ApStationListItem *prev)
{
    DOT11_ApStationListItem* next = NULL;
    DOT11_ApStationList* stationList = dot11->apVars->apStationList;

    if (!MacDot11ApStationListIsEmpty(node, dot11, stationList)) {
        if (prev == stationList->last || prev == NULL) {
            next = stationList->first;
        }
        else {
            next = prev->next;
        }
    }

    return next;
}// MacDot11ApStationListGetNextItem


//--------------------------------------------------------------------------
//  NAME:        MacDot11ApStationListResetFlags
//  PURPOSE:     Reset flag values in the station list
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11ApStationListResetFlags(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11_ApVars* ap = dot11->apVars;
    DOT11_ApStationListItem* next = NULL;
    DOT11_ApStationListItem* first;

    ERROR_Assert(MacDot11IsAp(dot11),
        "MacDot11ApStationListResetFlags: "
        "station list not applicable to configuration.\n");

    if (!MacDot11ApStationListIsEmpty(node, dot11, ap->apStationList)) {

        // It is simpler to reset all items
        next = first = MacDot11ApStationListGetFirst(node, dot11);

        do {
            next->data->flags = 0;
            next = MacDot11ApStationListGetNextItem(node, dot11, next);

        } while (next != first);

    }
}// MacDot11ApStationListResetFlags


//--------------------------------------------------------------------------
//  NAME:        MacDot11ApStationListGetItemWithGivenAddress
//  PURPOSE:     Retrieve the list item related to given address
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address lookupAddress
//                  Address to lookup
//  RETURN:      List item corresponding to address
//               NULL otherwise
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
//---------------------DOT11e--Updates------------------------------------//

DOT11_ApStationListItem* MacDot11ApStationListGetItemWithGivenAddress(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address lookupAddress)
{
    DOT11_ApVars* ap = dot11->apVars;
    DOT11_ApStationListItem* next = NULL;
    DOT11_ApStationListItem* first;

    if (!MacDot11ApStationListIsEmpty(node, dot11, ap->apStationList)) {
        next = first = MacDot11ApStationListGetFirst(node, dot11);

        do {
            if (next->data->macAddr == lookupAddress) {
                return next;
            }
            next = MacDot11ApStationListGetNextItem(node, dot11, next);

        } while (next != first);

    }

    return NULL;
}// MacDot11ApStationListGetItemWithGivenAddress

//--------------------DOT11e-End-Updates---------------------------------//
//--------------------------------------------------------------------------
//  NAME:        MacDot11RetrieveAssociationId
//  PURPOSE:     Add a station to list of given address
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      association id of station
//               NULL otherwise
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
int MacDot11RetrieveAssociationId(Node* node,
    MacDataDot11* dot11)
{
    int count;
    int AssocID = 0;

//    DOT11_ApVars* ap = dot11->apVars;
    // Set first bit one, because association id 0 is not valid
    
    dot11->apVars->AssociationVector[0] |= 0x01;
    for (count = 0; count < DOT11_PS_SIZE_OF_MAX_PARTIAL_VIRTUAL_BITMAP;
        count++)
    {
        if ((dot11->apVars->AssociationVector[count] ^ 0xFF) != 0x00)
        {
            break;
        }
    }
    if (count == DOT11_PS_SIZE_OF_MAX_PARTIAL_VIRTUAL_BITMAP)
    {
        return AssocID;
    }
    AssocID = count * 8;
    UInt8 tempVar1 = dot11->apVars->AssociationVector[count];
    for (int i = 0; i < 8; i++)
    {
        if (((tempVar1 >> i) & 0x01) == 0x00)
        {
            AssocID += i;
            MacDot11SetBit(dot11->apVars->AssociationVector, (unsigned)AssocID);
            break;
        }
    }
    if (AssocID > DOT11_AP_ASSIGN_MAX_ASSOCIATION_Id)
    {
        AssocID = 0;
    }
    return AssocID;

}
//--------------------------------------------------------------------------
//  NAME:        MacDot11ApStationListAddStation
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
int MacDot11ApStationListAddStation(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address stationAddress,
    unsigned char qoSEnabled,
    unsigned short listenInterval,
    BOOL psModeEnabled)
{
//---------------------------Power-Save-Mode-Updates---------------------//
    Queue*      queue = NULL;
//---------------------------Power-Save-Mode-End-Updates-----------------//
    DOT11_ApVars* ap = dot11->apVars;

    DOT11_ApStation *aStation = (DOT11_ApStation*)
        MEM_malloc(sizeof(DOT11_ApStation));

    ap->stationCount++;
    if (MacDot11IsAPSupportPSMode(dot11))
    {
        ERROR_Assert(ap->stationCount
             < (8 * DOT11_PS_SIZE_OF_MAX_PARTIAL_VIRTUAL_BITMAP),\
            "In PS Mode AP: Unable to assign association ID to STA\n");
    }

    //Retrieve Assoc id
    aStation->assocId = MacDot11RetrieveAssociationId(node,dot11);
    if (aStation->assocId == 0)
    {
        int retVal = aStation->assocId;
        ERROR_ReportWarning("AP can not associate node\n");

        // No need to add this rwquest
        MEM_free(aStation);
        return retVal;
    }
    aStation->stationType = DOT11_STA_IBSS;
//---------------------DOT11e--Updates------------------------------------//
    aStation->flags = qoSEnabled;
    
    
//--------------------DOT11e-End-Updates---------------------------------//
    aStation->macAddr = stationAddress;
    aStation->LastFrameReceivedTime = node->getNodeTime();
//---------------------------Power-Save-Mode-Updates---------------------//
    aStation->isSTAInPowerSaveMode = psModeEnabled;
    if ((dot11->isAnySTASupportPSMode == FALSE) && psModeEnabled)
    {
        dot11->isAnySTASupportPSMode = TRUE;
    }
    aStation->listenInterval = listenInterval;
    aStation->noOfBufferedPacket = 0;

    //queue = new Queue; // "FIFO"
    if (psModeEnabled == TRUE)
    {
    queue = new Queue; // "FIFO"
        ERROR_Assert(queue != NULL,\
            "AP: Unable to allocate memory for station queue");
        queue->SetupQueue(node, "FIFO", dot11->unicastQueueSize, 0, 0, 0,
            FALSE);
    }
    aStation->queue = queue;
//---------------------------Power-Save-Mode-End-Updates-----------------//
    MacDot11ApStationListInsert(node, dot11,
            ap->apStationList, aStation);
    if (MacDot11IsPC(dot11)&&
        (dot11->pcVars->deliveryMode != DOT11_PC_DELIVER_ONLY))
    {
        MacDot11cfPcPollListAddStation(node, dot11, stationAddress);
    }
    return aStation->assocId;

}//  MacDot11ApStationListAddStation


//-------------------------------------------------------------------------
// Init Functions
//-------------------------------------------------------------------------

//--------------------------------------------------------------------------
//  NAME:        MacDot11ApInit
//  PURPOSE:     Initialize AP related variables from user configuration.
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
void MacDot11ApInit(
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

    BOOL wasFound = FALSE;
    char input[MAX_STRING_LENGTH];
    Address address;
    // An AP can relay frames to/from stations outside the BSS.
    // Such stations can be other AP/PCs or IBSS stations.
    // BSS stations never relay. IBSS stations always relay.
    //       MAC-DOT11-RELAY-FRAMES YES | NO
    // Default is DOT11_RELAY_FRAMES_DEFAULT
        NetworkGetInterfaceInfo(
                node,
                dot11->myMacData->interfaceIndex,
                &address,
                networkType);

    dot11->relayFrames = DOT11_RELAY_FRAMES_DEFAULT;

    IO_ReadString(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11-RELAY-FRAMES",
        &wasFound,
        input);

    if (wasFound)
    {
        if (!strcmp(input, "YES"))
        {
            dot11->relayFrames = TRUE;
        }
        else if (!strcmp(input, "NO")) {
            dot11->relayFrames = FALSE;
        }
        else
        {
            ERROR_ReportError("MacDot11ApInit: "
                "Invalid value for MAC-DOT11-RELAY-FRAMES "
                "in configuration file.\n"
                "Expecting YES or NO.\n");
        }
    }

    // An AP can be a Point Coordinator, this is disabled for now
    IO_ReadString(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11-PC",
        &wasFound,
        input);
     if (wasFound)
     {
            if (!strcmp(input, "YES"))
            {
                dot11->stationType = DOT11_STA_PC;

            }
            else if (!strcmp(input, "NO"))
            {
                dot11->stationType = DOT11_STA_AP;
            }
            else
            {
                ERROR_ReportError("MacDot11ApInit: "
                    "Invalid value for MAC-DOT11-PC in configuration file.\n"
                    "Expecting YES or NO.\n");
            }
    }

    // Initialize AP related variables
    DOT11_ApVars* ap =
        (DOT11_ApVars*) MEM_malloc(sizeof(DOT11_ApVars));

    ERROR_Assert(ap != NULL,
        "MacDot11ApInit: Unable to allocate AP structure.\n");

    memset(ap, 0, sizeof(DOT11_ApVars));

    ap->broadcastsHavePriority = FALSE;
    ap->apStationList = NULL;
    ap->stationCount = 0;
    ap->firstStationItem = NULL;
    ap->prevStationItem = NULL;
    memset(&ap->AssociationVector, 0, sizeof(ap->AssociationVector));

    // Init the APs associated station list
    MacDot11ApStationListInit(node, dot11, &ap->apStationList);

    dot11->apVars = ap;
//---------------------------Power-Save-Mode-Updates---------------------//
    dot11->apVars->dtimPeriod = DOT11_DTIM_PERIOD;
    dot11->apVars->isAPSupportPSMode = FALSE;
    dot11->apVars->lastDTIMCount = 0;
    dot11->apVars->isBroadcastPacketTxContinue = FALSE;
    dot11->apVars->DTIMCount = 0;

    if (dot11->isPSModeEnabled == TRUE){
        dot11->isPSModeEnabled = FALSE;
    }// end of if

    int dtimPeriod;

    IO_ReadString(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11-AP-SUPPORT-PS-MODE",
        &wasFound,
        input);

    if (wasFound){
        Queue*      queue = NULL;
        if (strcmp(input, "YES") == 0) {
            dot11->apVars->isAPSupportPSMode = TRUE;


            ERROR_Assert(dot11->stationType != DOT11_STA_PC,\
            "AP: PCF not supported in PS mode");



            // Set up the broadcast queue here
            queue = new Queue; // "FIFO"
            ERROR_Assert(queue != NULL,\
                "AP: Unable to allocate memory for station queue");
            queue->SetupQueue(node, "FIFO", dot11->broadcastQueueSize, 0, 0, 0,
                FALSE);
            dot11->broadcastQueue = queue;

            // dot11s. PS mode not supported.
            if (dot11->isMP)
            {
                ERROR_ReportError(
                    "Mesh Points (both MPs and MAPs) do not support Power "
                    "Save mode.\n"
                    "Invalid use of MAC-DOT11-AP-SUPPORT-PS-MODE in "
                    "configuration for this node.\n");
            }
        }
        else if (strcmp(input, "NO") != 0){
            ERROR_ReportError("MacDot11ApInit: "
                "Invalid value for MAC-DOT11-AP-SUPPORT-PS-MODE "
                "in configuration file.\n"
                "Expecting YES or NO.\n");
        }
    }

    IO_ReadInt(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11-DTIM-PERIOD",
        &wasFound,
        &dtimPeriod);
    if (wasFound){
        if ((dtimPeriod <= 0) || (dtimPeriod > 255)){
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
                "Since given DTIM period is 0 or more then 255, "
                "So AP(node id: %d) consider default DTIM period",
                node->nodeId);
            ERROR_ReportWarning(errString);
        }
        else{
            dot11->apVars->dtimPeriod = (unsigned)dtimPeriod;
            dot11->DTIMperiod = (unsigned)dtimPeriod ;
        }
         dot11->apVars->timFrame.dTIMCount = 0;
         dot11->apVars->timFrame.dTIMPeriod =
             (unsigned char)dot11->apVars->dtimPeriod;
         dot11->apVars->timFrame.bitmapControl=0;
         dot11->apVars->timFrame.partialVirtualBitMap =0;
    }
//---------------------------Power-Save-Mode-End-Updates-----------------//
}// MacDot11ApInit


//--------------------------------------------------------------------------
//  NAME:        MacDot11BeaconInit
//  PURPOSE:     Initialize beacon related variables from user configuration
//  PARAMETERS:  Node* node
//                  Pointer to current node
//               const NodeInput* nodeInput
//                  Pointer to node input
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11BeaconInit(
    Node* node,
    const NodeInput* nodeInput,
    MacDataDot11* dot11,
    NetworkType networkType)
{
    BOOL wasFound = FALSE;
    int beaconInterval;
    int beaconStart;
    Address address;
    NodeId nodeId;

    // The beacon interval is specified in TUs (Time units)
    //  where each time unit is 1024US
    //       [0.6] MAC-DOT11-BEACON-INTERVAL 200
    //  and should be in the range 1 to DOT11_BEACON_INTERVAL_MAX TUs
    //  Default is DOT11_BEACON_INTERVAL_DEFAULT

    beaconInterval = DOT11_BEACON_INTERVAL_DEFAULT;
     NetworkGetInterfaceInfo(
        node,
        dot11->myMacData->interfaceIndex,
        &address,
        networkType);

    nodeId = node->nodeId;
    IO_ReadInt(
        node,
        nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11-BEACON-INTERVAL",
        &wasFound,
        &beaconInterval);

    if (wasFound)
    {
        ERROR_Assert(beaconInterval >= 0
            && beaconInterval <= DOT11_BEACON_INTERVAL_MAX,
            "MacDot11BeaconInit: "
            "Out of range value for MAC-DOT11-BEACON-INTERVAL in "
            "configuration file.\n"
            "Valid range is 0 to 32767.\n");
    }

    dot11->beaconInterval = MacDot11TUsToClocktype(
        (unsigned short) beaconInterval);

    // For overlapping BSSs, the beacon start time (in TUs)
    // is best configured to eliminate overlapping beacon periods.
    // The value offsets the first beacon from time 0 and
    // should be in the range 1 to beacon interval.
    //       [0.6] MAC-DOT11-BEACON-START-TIME 30
    //  Default is DOT11_BEACON_START_DEFAULT

    //randomly decide a value between 0 and beacon interval
    //beaconStart = DOT11_BEACON_START_DEFAULT;
    beaconStart = RANDOM_nrand(dot11->seed) % beaconInterval;

    IO_ReadInt(
        node,
        nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11-BEACON-START-TIME",
        &wasFound,
        &beaconStart);

    if (wasFound)
    {
        ERROR_Assert(beaconStart >= 0
            && beaconStart
             <= DOT11_BEACON_INTERVAL_MAX,
            "MacDot11BeaconStartInit: "
            "Out of range value for MAC-DOT11-BEACON-START-TIME "
            "in configuration file.\n"
            "Valid range is 0 to beacon interval.\n");
    }
    dot11->beaconTime =
            MacDot11TUsToClocktype((unsigned short)beaconStart);
      dot11->lastCfpBeaconTime = 0 ;
    // Start beacon timer
    //Support Fix Start: MacDot11StationStartBeaconTimer is not the correct
    //function to call for AP. It doesn't take care of beacon start time.
    if (!MacDot11IsAp(dot11)){
        MacDot11StationStartBeaconTimer(node, dot11);
    }
    else
    {
        //Schedule beacon at beacon start time. Current time is zero so the
        //delay is same as beaconTime
        MacDot11StationStartTimerOfGivenType(node, dot11,
            dot11->beaconTime , MSG_MAC_DOT11_Beacon);
    }

}// MacDot11BeaconInit

//---------------------------Power-Save-Mode-Updates---------------------//
//--------------------------------------------------------------------------
//  NAME:        MacDot11SetBit
//  PURPOSE:     set the bit according to association id.
//
//  PARAMETERS:  char* data
//                  Partial virtual aaray starting pointer
//               unsigned int associationId
//                  association id need to be set
//  RETURN:      Void
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static // inline//
void MacDot11SetBit(unsigned char* data, unsigned int associationId)
{
    // Here 8 represents no of bit in one byte
    int tempIntVar1 = associationId / 8;
    int tempIntVar2 = associationId % 8;
    ERROR_Assert(associationId <
        (DOT11_PS_SIZE_OF_MAX_PARTIAL_VIRTUAL_BITMAP * 8), "MacDot11SetBit: "
        "Invalid association id.\n");

    data[tempIntVar1] |= (0x01 << tempIntVar2);
    return;
}// end of MacDot11SetBit

//--------------------------------------------------------------------------
//  NAME:        MacDot11SetBitMapOffsetAndPartialVirtualBitMap
//  PURPOSE:     Construct partial bitmap and set Bitmap offset.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               UInt8* beacon
//                  Pointer to the beacon packet
//               DOT11_PS_TIMFrame* timFrame
//                  Pointer to the timFrame
//  RETURN:      int
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static // inline//
int MacDot11SetBitMapOffsetAndPartialVirtualBitMap(
    Node* node,
    MacDataDot11* dot11,
    UInt8* beacon,
    DOT11_PS_TIMFrame *timFrame)
{
// These two variabls will use to trimming the partial virtual bitmap data
    Int32 minAssociationId = 0;
    Int32 maxAssociationId = 0;
    Int32 length = 0;
    DOT11_ApVars* ap = dot11->apVars;
    DOT11_ApStationListItem* next = NULL;
    DOT11_ApStationListItem* first = NULL;
    UInt8 partialVirtualBitmap[DOT11_PS_SIZE_OF_MAX_PARTIAL_VIRTUAL_BITMAP]
        = {0, 0};
    // Construct the partial virtual bitmap here
    if (!MacDot11ApStationListIsEmpty(node, dot11, ap->apStationList))
    {
        next = first = MacDot11ApStationListGetFirst(node, dot11);
        do
        {
            if (next->data->noOfBufferedPacket > 0)
            {
                // set the bit in partial virtual bitmap field, if unicast
                // packet is buffered on AP for this association id
                MacDot11SetBit(partialVirtualBitmap,
                               (UInt32)(next->data->assocId));
                if (!minAssociationId)
                {
                    minAssociationId = next->data->assocId;
                    maxAssociationId = minAssociationId;
                }
                else if (minAssociationId > next->data->assocId)
                {
                    minAssociationId = next->data->assocId;
                }
                else if (maxAssociationId < next->data->assocId)
                {
                    maxAssociationId = next->data->assocId;
                }
            }
            next = MacDot11ApStationListGetNextItem( node, dot11, next );
        } while (next != first);
    }// end of if

    Int32 N1 = (minAssociationId / 8);
    Int32 N2 = (maxAssociationId / 8);

    // N1 is the largest even number
    if (N1 % 2)
    {
        N1--;
    }

    length = N2 - N1 + 1;
    memcpy(beacon, partialVirtualBitmap + N1, (size_t)(length));

    // set the Bitmap offset field
    timFrame->bitmapControl |= ((N1 / 2) << 1);
    return length;
}// end of MacDot11SetBitMapOffsetAndPartialVirtualBitMap


//--------------------------------------------------------------------------
//  NAME:        MacDOT11APBufferPacketAtSTAQueue
//  PURPOSE:     insert the packet in to Unicast queue
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDOT11APBufferPacketAtSTAQueue(
    Node* node,
    MacDataDot11* dot11)
{
    BOOL ret = FALSE;
    DOT11_ApVars* ap = dot11->apVars;
    DOT11_ApStationListItem* next = NULL;
    DOT11_ApStationListItem* first = NULL;
    DOT11_ApStation* data = NULL;

    if (!MacDot11ApStationListIsEmpty(node, dot11, ap->apStationList)) {
        next = first = MacDot11ApStationListGetFirst(node, dot11);

        do{
            data = next->data;
            if ((data->isSTAInPowerSaveMode == TRUE)
                && (data->macAddr ==
                dot11->ACs[dot11->currentACIndex].currentNextHopAddress)){
                // insert the packet
                if (!MacDot11BufferPacket(
                    data->queue,
                    dot11->ACs[dot11->currentACIndex].frameToSend,
                    node->getNodeTime())){
                    data->noOfBufferedPacket++;
                    ret = TRUE;
                 }
                else{
                    int noOfPacketDrop = 0;
                    ret = MacDot11PSModeDropPacket(
                        node,
                        dot11,
                        data->queue,
                        dot11->ACs[dot11->currentACIndex].frameToSend,
                        &noOfPacketDrop);
                    data->noOfBufferedPacket -= noOfPacketDrop;
                    if (ret == TRUE){
                        data->noOfBufferedPacket++;
                    }
                }
                dot11->ACs[dot11->currentACIndex].frameToSend = NULL;
                dot11->ACs[dot11->currentACIndex].BO = 0;
                dot11->ACs[dot11->currentACIndex].currentNextHopAddress = INVALID_802ADDRESS;
                break;
            }// end of if
            next = MacDot11ApStationListGetNextItem(node, dot11, next);
        } while (next != first);
    }// end of if
    return ret;
}// end of MacDOT11APBufferPacketAtSTAQueue

//--------------------------------------------------------------------------
//  NAME:        MacDot11APDequeueUnicastPacket
//  PURPOSE:     Dequeue the packet from Unicast queue
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address sourceNodeAddress
//                  sourceNodeAddress node address
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11APDequeueUnicastPacket(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address sourceNodeAddress)
{
    BOOL ret = FALSE;
    BOOL isMsgRetrieved = FALSE;
    Message *msg;
    DOT11_ApVars* ap = dot11->apVars;
    DOT11_ApStationListItem* next = NULL;
    DOT11_ApStationListItem* first = NULL;
    DOT11_ApStation* data = NULL;

    if (!MacDot11ApStationListIsEmpty(node, dot11, ap->apStationList)) {
        next = first = MacDot11ApStationListGetFirst(node, dot11);

        do{
            data = next->data;
            if ((data->isSTAInPowerSaveMode == TRUE)
                && (data->macAddr == sourceNodeAddress)){
                // Dequeue the packet
                // dequeue the unicast packet

                isMsgRetrieved = data->queue->retrieve(
                    &msg,
                    0,
                    DEQUEUE_PACKET,
                    node->getNodeTime());
                if (isMsgRetrieved){
                   data->noOfBufferedPacket--;
                   dot11->unicastDataPacketSentToPSModeSTAs++;
                    if (data->noOfBufferedPacket == 0){
                        dot11->isMoreDataPresent = FALSE;
                    }else{
                        dot11->isMoreDataPresent = TRUE;
                    }
                     ret = TRUE;
                     dot11->currentNextHopAddress = sourceNodeAddress;
                     dot11->currentMessage = msg;
                     dot11->pktsToSend++;
                     // For a BSS station,change next hop address to be that of the AP
                     dot11->ipNextHopAddr = dot11->currentNextHopAddress;
                     dot11->currentACIndex = DOT11e_INVALID_AC;

                     MacDot11UpdateStatsLeftQueue(node, dot11, msg,
                         dot11->myMacData->interfaceIndex, sourceNodeAddress);
                }
                break;
            }// end of if
            next = MacDot11ApStationListGetNextItem( node, dot11, next );
        } while (next != first);
    }// end of if
    return ret;
}// end of MacDot11APDequeueUnicastPacket

//--------------------------------------------------------------------------
//  NAME:        MacDot11AddTIMFrame
//  PURPOSE:     Add tim frame.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline//
int MacDot11AddTIMFrame(
    Node* node,
    MacDataDot11* dot11,
    unsigned char* beacon)
{
    DOT11_PS_TIMFrame* timFrame = NULL;
    int length = 0;
    ERROR_Assert(beacon != NULL, "MacDot11AddTIMFrame \
        Beacon frame can not be null\n");
   timFrame =
       ( DOT11_PS_TIMFrame* )MEM_malloc(sizeof(DOT11_PS_TIMFrame));

   timFrame->elementId = DOT11_PS_TIM_ELEMENT_ID_TIM;
   timFrame->dTIMPeriod = (unsigned char)dot11->apVars->dtimPeriod;
   timFrame->dTIMCount = (unsigned char)dot11->apVars->lastDTIMCount;

    // Set the default value of bitmap controll parameter here
    timFrame->bitmapControl = 0;
    // Check if current frame is DTIM frame and AP has the
    //   broadcast packet
    if (dot11->apVars->lastDTIMCount == 0){
        if (dot11->noOfBufferedBroadcastPacket > 0){
            dot11->apVars->isBroadcastPacketTxContinue = TRUE;
            // Current TIM Frame is DTIM frame
            timFrame->bitmapControl |= 0x01;
        }
        dot11->psModeDTIMFrameSent++;
    }// end of if
    else{
        dot11->psModeTIMFrameSent++;
    }
    // Set the partial virtual bitmap fields
    length = MacDot11SetBitMapOffsetAndPartialVirtualBitMap(
        node,
        dot11,
        (beacon + sizeof(DOT11_PS_TIMFrame)),
        timFrame);
    // Set the length of data, here 3 is for dtimcount,
    //    dtimperiod and bitmap control size
    timFrame->length = (unsigned char)(length + 3);

    memcpy(beacon, timFrame, sizeof(DOT11_PS_TIMFrame));
    MEM_free(timFrame);
    return(length + sizeof(DOT11_PS_TIMFrame));
}//end of MacDot11AddTIMFrame
//---------------------------Power-Save-Mode-End-Updates-----------------//

//==================================================================
//FUNCTION   :: Dot11_StoreIeLength
//LAYER      :: MAC
//PURPOSE    :: Place length value of IE at appropriate place in data stream.
//PARAMETERS ::
//+ element   : DOT11_Ie*     : IE pointer
//RETURN     :: void
//===================================================================

static
void Dot11_StoreIeLength(
    DOT11_Ie* element)
{
    element->data[1] =
        (unsigned char)(element->length - DOT11_IE_HDR_LENGTH);
}

//====================================================================
//FUNCTION   :: Dot11_AppendToElement
//LAYER      :: MAC
//PURPOSE    :: Append a field to an IE.
//PARAMETERS ::
//+ element   : DOT11_Ie*     : IE pointer
//+ field     : Type T        : field to append
//+ length    : int           : length of field, optional
//RETURN     :: void
//NOTES      ::   Does not work when appending strings.
//                The length parameter, if omitted, defaults to
//                the size of the field parameter.
//                It is good practice to pass the length value to
//                avoid surprises due to sizeof(field) across platforms.
//========================================================================

template <class R>
static
void Dot11_AppendToElement(
    DOT11_Ie* element,
    R field,
    int length = 0)
{
    int fieldLength = (length == 0) ? sizeof(field) : length;

    ERROR_Assert(sizeof(field) == fieldLength,
        "Dot11s_AppendToElement: "
        "size of field does not match length parameter.\n");
    ERROR_Assert(element->length + fieldLength <= DOT11_IE_LENGTH_MAX,
        "Dot11s_AppendToElement: "
        "appending would exceed permissible IE length.\n");

    memcpy(element->data + element->length, &field, (size_t)fieldLength);
    element->length += fieldLength;
}//Dot11_AppendToElement


//===================================================================
//FUNCTION   :: MacDot11DTIMupdate
//LAYER      :: MAC
//Purpose : to Update the DTIMcount and DTIM Period
//PARAMETERS ::
//+ node      : Node*         : pointer to node
//+ dot11     : MacDataDot11* : pointer to Dot11 data structure
//RETURN     :: void
//======================================================================

void MacDot11DTIMupdate( Node* node,
    MacDataDot11* dot11  )
{

    dot11->apVars->lastDTIMCount--;

    if (dot11->apVars->lastDTIMCount == -1){
        dot11->apVars->lastDTIMCount = dot11->apVars->dtimPeriod - 1;
    }
    dot11->apVars->timFrame.dTIMCount =
        (unsigned char) dot11->apVars->lastDTIMCount;
        if (dot11->apVars->lastDTIMCount == 0){
        if (DEBUG_BEACON){
            MacDot11Trace(node, dot11, NULL,
                "AP Send DTIM Frame Beacon");

        }
    }// end of if
    else{
        if (DEBUG_BEACON){
            MacDot11Trace(node, dot11, NULL,
                "AP Send TIM Frame Beacon.");
        }
   }
} // End of DTIM Update

//===================================================================
//FUNCTION   :: MacDot11CfpCountUpdate
//LAYER      :: MAC
//Purpose : to Update the Cfp Count and Cfp BalanceDuration in CfSet
//PARAMETERS ::
//+ node      : Node*         : pointer to node
//+ dot11     : MacDataDot11* : pointer to Dot11 data structure
//RETURN     :: void

//======================================================================
void MacDot11CfpCountUpdate(
                      Node* node,
                      MacDataDot11* dot11  )
{
    if (dot11->apVars->lastDTIMCount == 0)
    {
        dot11->pcVars->lastCfpCount-- ;
        if (dot11->pcVars->lastCfpCount == -1)
        {
            dot11->pcVars->lastCfpCount = dot11->pcVars->cfSet.cfpPeriod - 1;
        }
        dot11->pcVars->cfSet.cfpCount = dot11->pcVars->lastCfpCount;
    }

} // End of cfpCount Update

//===================================================================
//FUNCTION   :: MacDot11CfpParameterSetUpdate
//LAYER      :: MAC
//Purpose : to Update  Cfp Balance Duration in CfSet
//PARAMETERS ::
//+ node      : Node*         : pointer to node
//+ dot11     : MacDataDot11* : pointer to Dot11 data structure
//RETURN     :: void
//======================================================================

void MacDot11CfpParameterSetUpdate( Node* node,
    MacDataDot11* dot11  )
{
    clocktype cfpBalDuration;
    clocktype cfpStartTime;
    clocktype currentTime;
    // CFP Start
    currentTime = node->getNodeTime();
    if (dot11->apVars->lastDTIMCount == 0 && dot11->pcVars->cfSet.cfpCount == 0){
        cfpStartTime = currentTime + dot11->delayUntilSignalAirborn;
        dot11->pcVars->cfpEndTime = dot11->beaconTime + dot11->cfpMaxDuration;
        cfpBalDuration =  (dot11->pcVars->cfpEndTime - cfpStartTime);
        dot11->pcVars->cfSet.cfpBalDuration = MacDot11ClocktypeToTUs(cfpBalDuration);
        dot11->cfpEndTime = cfpStartTime +
        MacDot11TUsToClocktype((unsigned short)
                       dot11->pcVars->cfSet.cfpBalDuration);

    }
    else{
        if (dot11->state == DOT11_CFP_START){

             cfpStartTime = currentTime + dot11->delayUntilSignalAirborn;
             cfpBalDuration =  (dot11->cfpEndTime - cfpStartTime);
             dot11->pcVars->cfSet.cfpBalDuration =
             MacDot11ClocktypeToTUs(cfpBalDuration);
        }
                // Beacon in DCF Duration have
      else{
        dot11->pcVars->cfSet.cfpBalDuration = 0;
      }
    }
}

//===================================================================
//FUNCTION   :: Dot11_CreateCfParameterSet
//LAYER      :: MAC
//PURPOSE    :: To Create CfParameter Set in Beacon
//PARAMETERS ::
//+ node      : Node*         : pointer to node
//+ dot11     : MacDataDot11* : pointer to Dot11 data structure
//+ DOT11_Ie* : element       : IE Element Pointer
//+ DOT11_CFParameterSet*     : cfSet
//RETURN     :: void
//NOTES      :: This function partially creates the beacon frame.
//                It adds mesh related elements. Fixed length fields
//                are filled in separately.
//======================================================================


void Dot11_CreateCfParameterSet(
    Node* node,
    DOT11_Ie* element,
   DOT11_CFParameterSet* cfSet)
{
    element->length = 0;

     DOT11_IeHdr ieHdr(DOT11_CF_PARAMETER_SET_ID );
     Dot11_AppendToElement(element, ieHdr);
     // Ensure that CFParameter set is no more than specified lenght
    int cfParamSetLength = sizeof(DOT11_CFParameterSet);
    memcpy(element->data + element->length, cfSet, cfParamSetLength);
    element->length += cfParamSetLength;
    Dot11_StoreIeLength(element);
}//Dot11_CreateCfParameterSet

//===================================================================
//FUNCTION   :: Dot11_CreateTIMElement
//LAYER      :: MAC
//PURPOSE    :: To Create TIM Element in Beacon
//PARAMETERS ::
//+ node      : Node*         : pointer to node
//+ dot11     : MacDataDot11* : pointer to Dot11 data structure
//+ DOT11_Ie* : element       : IE Element Pointer
//+ DOT11_TIMFrame*           : timFrame:
//RETURN     :: void
//NOTES      :: This function partially creates the beacon frame.
//                It adds mesh related elements. Fixed length fields
//                are filled in separately.
//======================================================================

static
void Dot11_CreateTIMElement(
    Node* node,
    DOT11_Ie* element,
    DOT11_TIMFrame* timFrame)
{
    element->length = 0;

     DOT11_IeHdr ieHdr(DOT11_PS_TIM_ELEMENT_ID_TIM );
     Dot11_AppendToElement(element, ieHdr);

    // Ensure that TIM Element Length  is no more than specified lenght
    int timFrameLength = sizeof(DOT11_TIMFrame);
    memcpy(element->data + element->length,timFrame, timFrameLength);
    element->length += timFrameLength;
    Dot11_StoreIeLength(element);
}//Dot11_CreateTIMElement

//===================================================================
//FUNCTION   :: Dot11_CreateQBSSLoadElement
//LAYER      :: MAC
//PURPOSE    :: To Create QBSS Load IE in beacon
//PARAMETERS ::
//+ node      : Node*         : pointer to node
//+ dot11     : MacDataDot11* : pointer to Dot11 data structure
//+ DOT11_Ie* : element       : IE Element Pointer
//+ DOT11e_QBSSLoadElement*   :QBSSLOADElementSet:
//RETURN     :: void
//NOTES      :: This function partially creates the beacon frame.
//                It adds mesh related elements. Fixed length fields
//                are filled in separately.
//======================================================================
static
void Dot11_CreateQBSSLoadElement(
    Node* node,
    DOT11_Ie* element,
    DOT11e_QBSSLoadElement* QBSSLOADElementSet)
{
    element->length = 0;

     DOT11_IeHdr ieHdr(DOT11_QBSSLOAD_ELEMENT_ID);
     Dot11_AppendToElement(element, ieHdr);

    // Ensure that QBSS Load Element is no more than specified lenght
    int QBSSLoadElementLength = sizeof(DOT11e_QBSSLoadElement);
    memcpy(element->data + element->length, QBSSLOADElementSet, QBSSLoadElementLength);
    element->length += QBSSLoadElementLength;
    Dot11_StoreIeLength(element);
}//To Create QBSS Load IE in beacon

///===================================================================
//FUNCTION   :: Dot11_CreateEDCAParameterSet
//LAYER      :: MAC
//PURPOSE    :: To Create EDCA Parameter Set in beacon
//PARAMETERS ::
//+ node      : Node*         : pointer to node
//+ dot11     : MacDataDot11* : pointer to Dot11 data structure
//+ DOT11_Ie* : element       : IE Element Pointer
//+ DOT11e_EDCAParameterSet*  :EDCAParameterSet:
//RETURN     :: void
//NOTES      :: This function partially creates the beacon frame.
//                It adds mesh related elements. Fixed length fields
//                are filled in separately.
//======================================================================
static
void Dot11_CreateEDCAParameterSet(
    Node* node,
    DOT11_Ie* element,
    DOT11e_EDCAParameterSet* EDCAParameterSet)
{
    element->length = 0;
     DOT11_IeHdr ieHdr(DOT11_EDCA_PARAMETER_SET_ID);
     Dot11_AppendToElement(element, ieHdr);

    // Ensure that EDCAParameter set is no more than specified lenght
    int EDCAParameterSetLength = sizeof(DOT11e_EDCAParameterSet);
    memcpy(element->data + element->length, EDCAParameterSet,EDCAParameterSetLength );
    element->length += EDCAParameterSetLength ;
    Dot11_StoreIeLength(element);
}//Dot11_CreateEDCAParameterSet


///===================================================================
//FUNCTION   :: Dot11_CreateTSPEC
//LAYER      :: MAC
//PURPOSE    :: To Create TSPEC element
//PARAMETERS ::
//+ node      : Node*         : pointer to node
//+ dot11     : MacDataDot11* : pointer to Dot11 data structure
//+ DOT11_Ie* : element       : IE Element Pointer
//+ DOT11e_EDCAParameterSet*  :EDCAParameterSet:
//RETURN     :: void
//NOTES      :: This function creates TSPEC element.
//======================================================================

static
void Dot11_CreateTSPEC(
    Node* node,
    DOT11_Ie* element,
    TrafficSpec* TSPEC)
{
    element->length = 0;
     DOT11_IeHdr ieHdr(DOT11_TSPEC_ID);
     Dot11_AppendToElement(element, ieHdr);

    // Ensure that EDCAParameter set is no more than specified lenght
    int TSPECLength = sizeof(TrafficSpec);
    memcpy(element->data + element->length, TSPEC,TSPECLength );
    element->length += TSPECLength ;
    Dot11_StoreIeLength(element);
}

///===================================================================
//FUNCTION   :: Dot11_CreateQosCapability
//LAYER      :: MAC
//PURPOSE    :: To Create Qos Capability element
//PARAMETERS ::
//+ node      : Node*         : pointer to node
//+ dot11     : MacDataDot11* : pointer to Dot11 data structure
//+ DOT11_Ie* : element       : IE Element Pointer
//+ DOT11e_EDCAParameterSet*  :EDCAParameterSet:
//RETURN     :: void
//NOTES      :: This function creates Qos capability element.
//======================================================================

static
void Dot11_CreateQosCapability(
    Node* node,
    DOT11_Ie* element,
    TrafficSpec* TSPEC)
{
    element->length = 0;
     DOT11_IeHdr ieHdr(DOT11_TSPEC_ID);
     Dot11_AppendToElement(element, ieHdr);

    // Ensure that EDCAParameter set is no more than specified lenght
    int TSPECLength = sizeof(TSPEC);
    memcpy(element->data + element->length, TSPEC,TSPECLength );
    element->length += TSPECLength ;
    Dot11_StoreIeLength(element);
}

///===================================================================
//FUNCTION   :: Dot11_CreateBeaconFrame
//LAYER      :: MAC
//PURPOSE    :: Create a beacon frame.
//PARAMETERS ::
//+ node      : Node*         : pointer to node
//+ dot11     : MacDataDot11* : pointer to Dot11 data structure
//+ txFrame   : DOT11_MacFrame* : Tx frame to be created
//+ txFrameSize : int*        : frame size after creation
//RETURN     :: void
//NOTES      :: This function partially creates the beacon frame.
//                It adds mesh related elements. Fixed length fields
//                are filled in separately.
//======================================================================

void Dot11_CreateBeaconFrame(
    Node* node,
    MacDataDot11* dot11, DOT11_Ie* element,
    DOT11_MacFrame* txFrame,
    int* txFrameSize)
{

    *txFrameSize = sizeof(DOT11_BeaconFrame);
    // Add Beacon related IEs
   // DOT11Ie* element = Dot11_Malloc(DOT11Ie);
    element->data = (unsigned char*)txFrame + *txFrameSize;
    //Add Cfp Parameter set

    if (MacDot11IsPC(dot11))
    {
        MacDot11CfpParameterSetUpdate(node, dot11);
        Dot11_CreateCfParameterSet(node, element, &dot11->pcVars->cfSet);
        *txFrameSize += element->length;
        if (!MacDot11IsAPSupportPSMode(dot11))
        {
            element->data = (unsigned char*)txFrame + *txFrameSize;
            Dot11_CreateTIMElement(node, element,&dot11->apVars->timFrame);
            *txFrameSize +=element->length;
        }
    }
}//Dot11_CreateBeaconFrame

//--------------------------------------------------------------------------
//  NAME:        MacDot11AddQBSSLoadElementToBeacon
//  PURPOSE:     To Add QBSSLoad IE in Beacon Frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_Ie* element
//                   Pointer To IE element
//               DOT11_MacFrame* txFrame
//                   Pointer to beacon Frame
//                int* txFrameSize
//                      Pointer To return Frame Size
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------

//
void MacDot11AddQBSSLoadElementToBeacon( Node* node,
            MacDataDot11* dot11,
            DOT11_Ie* element,
            DOT11_MacFrame* txFrame,
            int* txFrameSize)
{
    // Add Beacon related IEs
    element->data = (unsigned char*)txFrame + *txFrameSize;
    //Add QBSSLoad Element to beacon
    Dot11_CreateQBSSLoadElement(node, element, &dot11->qBSSLoadElement);
    *txFrameSize += element->length;
    //Still to add EDCA and Other

}// MacDot11AddQBSSLoadElementToBeacon

//--------------------------------------------------------------------------
//  NAME:        MacDot11AddEDCAParametersetToBeacon
//  PURPOSE:     To Add EDCA Parameter set IE in Beacon Frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_Ie* element
//                   Pointer To IE element
//               DOT11_MacFrame* txFrame
//                   Pointer to beacon Frame
//                int* txFrameSize
//                      Pointer To return Frame Size
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------


void MacDot11AddEDCAParametersetToBeacon(Node* node,
            MacDataDot11* dot11,
            DOT11_Ie* element,
            DOT11_MacFrame* txFrame,
            int* txFrameSize)
{
    element->data = (unsigned char*)txFrame + *txFrameSize;
    //Add EDCA Parameter set to Beacon
    Dot11_CreateEDCAParameterSet(node, element, &dot11->edcaParamSet);
    *txFrameSize += element->length;

}

//--------------------------------------------------------------------------
//  NAME:        MacDot11AddTrafficSpecToframe
//  PURPOSE:     To Add TSPEC in Frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_Ie* element
//                   Pointer To IE element
//               DOT11_MacFrame* txFrame
//                   Pointer to beacon Frame
//                int* txFrameSize
//                      Pointer To return Frame Size
//                 TrafficSpec* TSPEC
//                      pointer to the Traffic specification value
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------

void MacDot11AddTrafficSpecToframe(Node* node,
            MacDataDot11* dot11,
            DOT11_Ie* element,
            DOT11_MacFrame* txFrame,
            int* txFrameSize,
            TrafficSpec* TSPEC)
{
    element->data = (unsigned char*)txFrame + *txFrameSize;
    //Add EDCA Parameter set to Beacon
    Dot11_CreateTSPEC(node, element,TSPEC);
    *txFrameSize += element->length;
}

//--------------------------------------------------------------------------
//  NAME:        MacDot11AddQosCapability
//  PURPOSE:     To Add Qos capability in Frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_Ie* element
//                   Pointer To IE element
//               DOT11_MacFrame* txFrame
//                   Pointer to beacon Frame
//                int* txFrameSize
//                      Pointer To return Frame Size
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------

void MacDot11AddQosCapability(Node* node,
            MacDataDot11* dot11,
            DOT11_Ie* element,
            DOT11_MacFrame* txFrame,
            int* txFrameSize)
{
    element->data = (unsigned char*)txFrame + *txFrameSize;
    //Add EDCA Parameter set to Beacon
    Dot11_CreateTSPEC(node, element, &dot11->TSPEC);
    *txFrameSize += element->length;

}

//--------------------------------------------------------------------------
//  NAME:        MacDot11CanTransmitBeacon
//  PURPOSE:     Check if sufficient time is available to send
//                beacon frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//                int   beaconFrameSize
//                      Size of Beacon Frame
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11CanTransmitBeacon(Node* node,
    MacDataDot11* dot11, int beaconFrameSize)
{
    clocktype cfpBalDuration = 0;
    BOOL canTransmit = FALSE;
    clocktype cfpStartTime;
    //clocktype cfpEndTime;
    clocktype currentTime;
    int dataRateType = 0;
    canTransmit = TRUE;


    if (MacDot11IsPC(dot11))
    {
        currentTime = node->getNodeTime();
        //if ((dot11->apVars->lastDTIMCount == 0) && (dot11->pcVars->cfSet.cfpCount == 0))
        {
            cfpStartTime = currentTime + dot11->delayUntilSignalAirborn;

            // Has beacon been delayed for this CFP?
            if (dot11->cfpEndTime > cfpStartTime) {
          

                cfpBalDuration =  (dot11->cfpEndTime - cfpStartTime);
                // Check if there is sufficient time to start CFP
                // Is there sufficient time to send a beacon + CF-End?
                
/*********HT START*************************************************/
                if (dot11->isHTEnable)
                {
                    MAC_PHY_TxRxVector tempVector1 = dot11->txVectorForBC;
                    MAC_PHY_TxRxVector tempVector2 = dot11->txVectorForBC;
                    tempVector1.length=(size_t)beaconFrameSize;
                    tempVector2.length=(size_t)DOT11_LONG_CTRL_FRAME_SIZE;

                    canTransmit = cfpBalDuration >=
                    PHY_GetTransmissionDuration(
                        node, dot11->myMacData->phyNumber,
                        tempVector1)
                        + dot11->extraPropDelay
                        + dot11->sifs
                        + PHY_GetTransmissionDuration(
                            node, dot11->myMacData->phyNumber,
                            tempVector2)
                            + dot11->extraPropDelay
                        + dot11->slotTime;
                }
                else
                {
                    dataRateType = dot11->highestDataRateTypeForBC;
                canTransmit = cfpBalDuration >=
                PHY_GetTransmissionDuration(
                    node, dot11->myMacData->phyNumber,
                    dataRateType,
                    beaconFrameSize)
                    + dot11->extraPropDelay
                    + dot11->sifs
                    + PHY_GetTransmissionDuration(
                        node, dot11->myMacData->phyNumber,
                        dataRateType,
                        DOT11_LONG_CTRL_FRAME_SIZE)
                        + dot11->extraPropDelay
                    + dot11->slotTime;
            }
/*********HT END****************************************************/

            }

        }// if it is not First CFP beacon then beacon can be transmitted immidiatly
       // else
       // {
       //     canTransmit = TRUE;
       // }

    }
    return canTransmit;
}// end of MacDot11CanTransmitBeacon


/*********HT START*************************************************/
void MacDot11AddHTCapability(Node* node,
            MacDataDot11* dot11,
            DOT11_Ie* element,
            DOT11_MacFrame* txFrame,
            int* txFrameSize)
{
    element->data = (unsigned char*)txFrame + *txFrameSize;
    element->length = 0;
    DOT11_IeHdr ieHdr(DOT11N_HT_CAPABILITIES_ELEMENT_ID);
    Dot11_AppendToElement(element, ieHdr);

    // Create HT Capabilities
    // Calculate MCS (Possibly interaction with PHY layer 
    // is required to get the mcs index)
    
    DOT11_HT_CapabilityElement htCapabilityElement;
    memset(&htCapabilityElement,0,sizeof(DOT11_HT_CapabilityElement));
    htCapabilityElement.supportedMCSSet.mcsSet.maxMcsIdx
        = dot11->txVectorForHighestDataRate.mcs;
    htCapabilityElement.htCapabilitiesInfo.htGreenfield = TRUE;
    htCapabilityElement.htCapabilitiesInfo.shortGiFor20Mhz = 
    Phy802_11nShortGiCapable(node->phyData[dot11->myMacData->phyNumber]);
    if ((ChBandwidth)Phy802_11nGetOperationChBwdth(
             node->phyData[dot11->myMacData->phyNumber])== CHBWDTH_20MHZ){
        htCapabilityElement.htCapabilitiesInfo.fortyMhzIntolerent = TRUE;
    }
    else{
        htCapabilityElement.htCapabilitiesInfo.fortyMhzIntolerent = FALSE;
    }
        
    htCapabilityElement.htCapabilitiesInfo.shortGiFor40Mhz = 
    Phy802_11nShortGiCapable(node->phyData[dot11->myMacData->phyNumber]);
    htCapabilityElement.htCapabilitiesInfo.txStbc = 
    Phy802_11nStbcCapable(node->phyData[dot11->myMacData->phyNumber]);
    htCapabilityElement.htCapabilitiesInfo.rxStbc = 
    Phy802_11nStbcCapable(node->phyData[dot11->myMacData->phyNumber]);
    
    // amsdu max size
    htCapabilityElement.htCapabilitiesInfo.maxAmsduLength
        = dot11->enableBigAmsdu;
    
    // ampdu max size
    htCapabilityElement.ampduParams.ampduMaxLengthExponent
        = dot11->ampduLengthExponent;
    htCapabilityElement.htCapabilitiesInfo.htDelayedBlockAck
        = dot11->enableDelayedBAAgreement;
    htCapabilityElement.asesCapabilities.transmitSoundingPpduCapable
        = FALSE;
    int htCapabilityElementLength = sizeof(DOT11_HT_CapabilityElement);
    memcpy(element->data + element->length,
           &htCapabilityElement, 
           htCapabilityElementLength );
    element->length += htCapabilityElementLength ;
    Dot11_StoreIeLength(element);
    *txFrameSize += element->length;
}

void MacDot11AddHTOperation(Node* node,
                            MacDataDot11* dot11,
                            const DOT11_HT_OperationElement* operElem,
                            DOT11_MacFrame* txFrame,
                            int* txFrameSize)
{
    size_t length;
    char* buffer = (char*)txFrame + *txFrameSize;
    *buffer = (char) DOT11N_HT_OPERATION_ELEMENT_ID;
    *(buffer + 1) = (char) DOT11N_HT_OPERATION_ELEMENT_LEN;
    operElem->ToPacket(buffer + 2, &length);
    *txFrameSize += length;
}

/*********HT END****************************************************/

//--------------------------------------------------------------------------
//  NAME:        MacDot11BuiildInstantBeaconFrame
//  PURPOSE:     Send the beacon in DCF mode.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------

void MacDot11BuiildInstantBeaconFrame(Node* node,
    MacDataDot11* dot11)
{
    Message* beaconMsg;
    DOT11_BeaconFrame* beacon = NULL;
    clocktype currentTime;
//    int dataRateType = 0;
    //added for PCF
//    int cfpBalDuration = 0;
//    BOOL inPSmode = FALSE;

//---------------------------Power-Save-Mode-Updates---------------------//
    int length = 0;
    unsigned char beaconData[DOT11_MAX_BEACON_SIZE] = {0, 0};
//---------------------------Power-Save-Mode-End-Updates-----------------//

 //---------------------------Power-Save-Mode-Updates---------------------//
   if (MacDot11IsAPSupportPSMode(dot11)){
        length += MacDot11AddTIMFrame(node, dot11, beaconData + length);
   }// end of if

//---------------------------Power-Save-Mode-End-Updates-----------------//

    currentTime = node->getNodeTime();
    MacDot11StationCancelTimer(node, dot11);

    DOT11_MacFrame beaconFrame = {{0}, {0}};
    int beaconFrameSize = 0;
    DOT11_Ie* element = Dot11_Malloc(DOT11_Ie);

    // dot11s
    if (dot11->isMP) {

        if (MacDot11IsQoSEnabled(node, dot11)) {
            ERROR_ReportError(
                "Current mesh implementation does not support QoS.\n");
        }

        if (length > 0) {
            ERROR_ReportError("MacDot11ApTransmitBeacon: "
                "Current mesh implementation does not support PS mode.\n");
        }
        // Build beacon frame (message) with Dot11s elements
        Dot11s_CreateBeaconFrame(node, dot11, element,
                      &beaconFrame, &beaconFrameSize);

    }

    else {
        // Build beacon frame (message)
        Dot11_CreateBeaconFrame(node, dot11, element, &beaconFrame, &beaconFrameSize);

    }
    if (length > 0){
        beaconFrameSize += length;
    }
    //---------------------DOT11e--Updates------------------------------------//
    if (MacDot11IsQoSEnabled(node, dot11))
    {
//--------------------HCCA-Updates Start---------------------------------//
        if (dot11->isHCCAEnable)
            MacDot11eHcPollListResetAtStartOfBeacon(node,dot11);
//--------------------HCCA-Updates End-----------------------------------//
        //MacDot11AddQBSSLoadElementToBeacon
        MacDot11AddQBSSLoadElementToBeacon(node, dot11, element,
                  &beaconFrame, &beaconFrameSize);
        //MacDot11AddEDCAParametersetToBeacon
        MacDot11AddEDCAParametersetToBeacon(node, dot11, element,
                  &beaconFrame, &beaconFrameSize);

/*********HT START*************************************************/     
        if (dot11->isHTEnable)
        {
            MacDot11AddHTCapability(node,
                                    dot11,
                                    element,
                                    &beaconFrame,
                                    &beaconFrameSize);

            // Add operation element
            DOT11_HT_OperationElement  operElem;
            operElem.opChBwdth =
                (ChBandwidth)Phy802_11nGetOperationChBwdth(
                    node->phyData[dot11->myMacData->phyNumber]);
            operElem.rifsMode = dot11->rifsMode;
            MacDot11AddHTOperation(node,
                                   dot11,
                                   &operElem,
                                   &beaconFrame,
                                   &beaconFrameSize);
        }
/*********HT END****************************************************/

    }
 //--------------------DOT11e-End-Updates---------------------------------//

//---------------------------Power-Save-Mode-Updates---------------------//
        MEM_free(element);
        //element = NULL;
        dot11->beaconSet = FALSE;
        beaconMsg = MESSAGE_Alloc(node, 0, 0, 0);
        MESSAGE_PacketAlloc(node, beaconMsg, beaconFrameSize , TRACE_DOT11);
        beacon = (DOT11_BeaconFrame*) MESSAGE_ReturnPacket(beaconMsg);
        memset(beacon, 0, beaconFrameSize );
        memcpy(beacon, &beaconFrame, beaconFrameSize);
        if (length > 0){
        memcpy((unsigned char*)beacon + sizeof(DOT11_BeaconFrame),
            &beaconData, length);
        }
        beacon->hdr.frameType = DOT11_BEACON;
        beacon->hdr.frameFlags = 0;
        beacon->hdr.duration = 0;
        beacon->hdr.destAddr = ANY_MAC802;
        beacon->hdr.sourceAddr = dot11->selfAddr;
        beacon->hdr.address3 = dot11->selfAddr;
        strcpy(beacon->ssid, dot11->stationMIB->dot11DesiredSSID);  // String copy over the BSSID
        beacon->timeStamp = dot11->beaconTime + dot11->delayUntilSignalAirborn;
        beacon->beaconInterval =
        MacDot11ClocktypeToTUs(dot11->beaconInterval);

//---------------------------Power-Save-Mode-End-Updates-----------------//


        if (dot11->currentMessage == NULL){
              dot11->currentMessage = beaconMsg;
              dot11->dot11TxFrameInfo = (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
              memset(dot11->dot11TxFrameInfo,0,sizeof(DOT11_FrameInfo));
              dot11->dot11TxFrameInfo->DA = ANY_MAC802;
              dot11->dot11TxFrameInfo->frameType = DOT11_BEACON;
              dot11->dot11TxFrameInfo->insertTime = node->getNodeTime();
              dot11->dot11TxFrameInfo->msg = dot11->currentMessage;
              dot11->dot11TxFrameInfo->RA = ANY_MAC802;
              dot11->dot11TxFrameInfo->SA = dot11->selfAddr;
              dot11->dot11TxFrameInfo->TA = dot11->selfAddr;
              dot11->currentNextHopAddress = ANY_MAC802;
              dot11->beaconSet = TRUE;
              dot11->beaconIsDue = FALSE;
              dot11->currentACIndex = 0;

              if ((MacDot11StationPhyStatus(node, dot11) != PHY_IDLE)||
                  MacDot11StationWaitForNAV(node, dot11)){
                MacDot11StationSetBackoffIfZero(node, dot11,dot11->currentACIndex);
              }

        }
        else{
            dot11->beaconSet = FALSE;
            dot11->instantMessage = beaconMsg;
            dot11->beaconIsDue = FALSE;
        }
        MacDot11StationAttemptToGoIntoWaitForDifsOrEifsState(node, dot11);
    }   // MacDot11BuiildInstantBeaconFrame


//--------------------------------------------------------------------------
//  NAME:        MacDot11ApTransmitBeacon
//  PURPOSE:     Send the beacon.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11ApTransmitBeacon(
    Node* node,
    MacDataDot11* dot11)
{
    Message* beaconMsg;
    DOT11_BeaconFrame* beacon = NULL;
    clocktype currentTime;
    int dataRateType = 0;
    //added for PCF
//    int cfpBalDuration = 0;
    BOOL canTransmit = FALSE;
//    BOOL inPSmode = FALSE;

//---------------------------Power-Save-Mode-Updates---------------------//
    int length = 0;
    unsigned char beaconData[DOT11_MAX_BEACON_SIZE] = {0, 0};
//---------------------------Power-Save-Mode-End-Updates-----------------//
    // Check if we can transmit
    if ((MacDot11StationPhyStatus(node, dot11) != PHY_IDLE) ||
         (MacDot11StationWaitForNAV(node, dot11)))
    {
        MacDot11Trace(node, dot11, NULL,
            "Beacon delayed - not idle or waiting for NAV.");
        if (MacDot11IsPC(dot11)){
            if (dot11->apVars->lastDTIMCount == 0 &&
                   dot11->pcVars->lastCfpCount == 0){
            dot11->pcVars->isBeaconDelayed = TRUE;
            }
        }

        // Either wait for timer to expire or for
        // StartSendTimers to reattempt transmission
        MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        return;
    }
//---------------------------Power-Save-Mode-Updates---------------------//
    if (MacDot11IsAPSupportPSMode(dot11)){
        length += MacDot11AddTIMFrame(node, dot11, beaconData + length);

    }// end of if
//---------------------------Power-Save-Mode-End-Updates-----------------//

    currentTime = node->getNodeTime();
    // If AP is PC then stores the time of last Cfp Start Beacon

    if (MacDot11IsPC(dot11))
    {
        if (dot11->apVars->lastDTIMCount == 0 &&
            dot11->pcVars->lastCfpCount == 0){
             dot11->lastCfpBeaconTime = currentTime;
        }
    }
    if (DEBUG_BEACON)
    {
         char timeStr[100];
         ctoa(node->getNodeTime(), timeStr);
         printf("\n%d Transmitting Beacon "
               "\t  at %s \n",
                node->nodeId,
                timeStr);
    }

    DOT11_MacFrame beaconFrame;
    int beaconFrameSize = 0;
    DOT11_Ie* element = Dot11_Malloc(DOT11_Ie);

    // dot11s. Assumed that QoS, PS IEs not present in beacon.
    if (dot11->isMP) {
        if (MacDot11IsQoSEnabled(node, dot11)) {
            ERROR_ReportError(
                "Current 802.11s implementation does not support QoS.\n"
                "802.11e seems enabled for this node.\n");
        }
        if (length > 0) {
            ERROR_ReportError("MacDot11ApTransmitBeacon: "
                "Current 802.11s implementation does not support PS mode.\n"
                "Power Save support seems enabled for this node.\n");
        }
        // Build beacon frame (message) with Dot11s elements
        Dot11s_CreateBeaconFrame(node, dot11, element,
            &beaconFrame, &beaconFrameSize);
    }
    else {
        // Build beacon frame (message)
           Dot11_CreateBeaconFrame(node, dot11, element, &beaconFrame, &beaconFrameSize);
        }

  //---------------------DOT11e--Updates------------------------------------//
    if (MacDot11IsQoSEnabled(node, dot11))
    {
//--------------------HCCA-Updates Start---------------------------------//
        if (dot11->isHCCAEnable)
            MacDot11eHcPollListResetAtStartOfBeacon(node,dot11);
//--------------------HCCA-Updates End-----------------------------------//
        MacDot11AddQBSSLoadElementToBeacon(node, dot11, element,
                  &beaconFrame, &beaconFrameSize);
        MacDot11AddEDCAParametersetToBeacon(node, dot11, element,
                  &beaconFrame, &beaconFrameSize);

/*********HT START*************************************************/     
        if (dot11->isHTEnable)
        {
            MacDot11AddHTCapability(node,
                                    dot11,
                                    element,
                                    &beaconFrame,
                                    &beaconFrameSize);

            DOT11_HT_OperationElement  operElem;
            operElem.opChBwdth =
                (ChBandwidth)Phy802_11nGetOperationChBwdth(
                    node->phyData[dot11->myMacData->phyNumber]);
            operElem.rifsMode = dot11->rifsMode;
            MacDot11AddHTOperation(node,
                                   dot11,
                                   &operElem,
                                   &beaconFrame,
                                   &beaconFrameSize);
        }
/*********HT END****************************************************/

    }
 //--------------------DOT11e-End-Updates---------------------------------//



        MEM_free(element);//Free Memory Allocated to Information Element
        beaconMsg = MESSAGE_Alloc(node, 0, 0, 0);
        MESSAGE_PacketAlloc(node, beaconMsg, beaconFrameSize + length, TRACE_DOT11);
        beacon = (DOT11_BeaconFrame*) MESSAGE_ReturnPacket(beaconMsg);
        memset(beacon, 0, beaconFrameSize);
        memcpy(beacon, &beaconFrame, beaconFrameSize);
        if (length > 0){
            memcpy((unsigned char*)beacon + beaconFrameSize,
            &beaconData, length);
            beaconFrameSize += length;
        }
        beacon->hdr.frameType = DOT11_BEACON;
        beacon->hdr.frameFlags = 0;
        beacon->hdr.duration = 0;
        beacon->hdr.destAddr = ANY_MAC802;
        beacon->hdr.sourceAddr = dot11->selfAddr;
        beacon->hdr.address3 = dot11->selfAddr;
        strcpy(beacon->ssid, dot11->stationMIB->dot11DesiredSSID);  // String copy over the BSSID
        beacon->timeStamp = dot11->beaconTime + dot11->delayUntilSignalAirborn;
        beacon->beaconInterval =
        MacDot11ClocktypeToTUs(dot11->beaconInterval);

/*********HT START*************************************************/
        if (dot11->isHTEnable)
        {
            MAC_PHY_TxRxVector tempTxVector;
            tempTxVector = MacDot11nSetTxVector(
                               node, 
                               dot11, 
                               MESSAGE_ReturnPacketSize(beaconMsg),
                               DOT11_BEACON, 
                               ANY_MAC802, 
                               NULL);
            PHY_SetTxVector(node, 
                            dot11->myMacData->phyNumber,
                            tempTxVector);

        }
        else
        {
            dataRateType = dot11->highestDataRateTypeForBC;
            PHY_SetTxDataRateType(node, dot11->myMacData->phyNumber,
                dataRateType);
        }
/*********HT END****************************************************/

        canTransmit = MacDot11CanTransmitBeacon(node, dot11, beaconFrameSize);

        if (canTransmit)
        {
        if (dot11->state == DOT11_CFP_START)
        {
            dot11->cfpState = DOT11_X_CFP_BEACON;
        }
        else
        {
            MacDot11StationSetState(node, dot11, DOT11_X_BEACON);
        }
        MacDot11StationCancelTimer(node, dot11);
            MacDot11StationStartTransmittingPacket(node,
                        dot11,
                        beaconMsg,
        dot11->delayUntilSignalAirborn);
    }
    else
    {
        if (dot11->state == DOT11_CFP_START)
        {
            //do nothing
        }
        else
        {
        MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
        MacDot11StationCheckForOutgoingPacket(node, dot11, FALSE);
        }
    }
}// MacDot11ApTransmitBeacon



//--------------------------------------------------------------------------
//  NAME:        MacDot11ApAttemptToTransmitBeacon
//  PURPOSE:     Attempt to send a beacon.
//               Checks permissible states and sets the beacon timer.
//               Where timer has elapsed, makes a call to transmit beacon.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      TRUE if states permit or beacon transmit occurs
//  ASSUMPTION:  Where DIFS has elapsed or is in back-off, no
//               additional delay is neccary.
//               The beacon can have precedance when waiting for CTS.
//--------------------------------------------------------------------------
BOOL MacDot11ApAttemptToTransmitBeacon(
    Node* node,
    MacDataDot11* dot11)
{
    BOOL couldTransmit = FALSE;

    if (MacDot11StationPhyStatus(node, dot11) != PHY_IDLE
        || MacDot11IsTransmittingState(dot11->state)
        || MacDot11IsWaitingForResponseState(dot11->state)
        ||MacDot11IsCfpTransmittingState(dot11->cfpState))
    {
        return FALSE;
    }
    if (dot11->state != DOT11_CFP_START){
        if (MacDot11StationWaitForNAV(node, dot11)) {

            clocktype currentTime = node->getNodeTime();
            MacDot11StationSetState(node, dot11, DOT11_S_WFNAV);
            MacDot11StationStartTimer(node, dot11, dot11->NAV - currentTime);
            // prevent HandleTimeout from modifying state
            return TRUE;
         }
    }
    if (dot11->BeaconAttempted == TRUE ){
         // Free Message if beacon was set in Current Message
        //and Could not be transmitted
            if (dot11->beaconSet == TRUE)
            {
                dot11->beaconSet = FALSE;
                if (dot11->dot11TxFrameInfo != NULL) {
                    if (dot11->currentMessage == dot11->dot11TxFrameInfo->msg){
                        MEM_free(dot11->dot11TxFrameInfo);
                        dot11->dot11TxFrameInfo = NULL;
                    }
                }
                dot11->currentMessage = NULL;
                dot11->currentNextHopAddress = ANY_MAC802;
                dot11->ipNextHopAddr = ANY_MAC802;
                dot11->ipDestAddr = ANY_MAC802;
                dot11->ipSourceAddr = ANY_MAC802;
                dot11->dataRateInfo = NULL;
            }
            else
            {
                dot11->instantMessage = NULL;
            }
       MacDot11ApTransmitBeacon(node, dot11);
       couldTransmit = TRUE;
    }
    else{
        switch (dot11->state) {
            case DOT11_S_IDLE:
            case DOT11_S_WFNAV:
            case DOT11_S_NAV_RTS_CHECK_MODE:
            case DOT11_S_WF_DIFS_OR_EIFS:

                MacDot11StationSetState(node, dot11, DOT11_S_WFBEACON);
                MacDot11DTIMupdate(node, dot11);

                if (MacDot11IsPC(dot11)){
                     MacDot11CfpCountUpdate( node, dot11  ) ;
                    if (dot11->apVars->lastDTIMCount == 0 &&
                       dot11->pcVars->lastCfpCount == 0){
                    MacDot11StationStartTimer(node, dot11, dot11->txPifs);
                    }

                    else{  // Send beacon using DIFS and Backoff
                      MacDot11BuiildInstantBeaconFrame(node, dot11);
                    }
                }//if
                // If AP then Always Send by DIFS and Backoff
                else{
                 MacDot11BuiildInstantBeaconFrame(node, dot11);
                }
                dot11->BeaconAttempted = TRUE;
                couldTransmit = TRUE;
                break;

                //Transmit beacon during CFP with Sifs interval

            case DOT11_CFP_START:
                if (dot11->cfpState == DOT11_S_CFP_NONE){
                     MacDot11DTIMupdate(node, dot11);
                     if (MacDot11IsPC(dot11)){
                         MacDot11CfpCountUpdate( node, dot11  ) ;
                       }
                     MacDot11CfpSetState(dot11,  DOT11_S_CFP_WFBEACON);
                MacDot11StationStartTimer(node, dot11, dot11->txSifs);
                couldTransmit = TRUE;
                     dot11->BeaconAttempted = TRUE;
                }
                else{
                    couldTransmit = FALSE;
                }
                break;

            case DOT11_S_BO:
                // Have waited enough, can transmit immediately
                dot11->BO = 0;
                MacDot11DTIMupdate(node, dot11);
                if (MacDot11IsPC(dot11)){
                    MacDot11CfpCountUpdate( node, dot11  ) ;
                }

                MacDot11ApTransmitBeacon(node, dot11);
                dot11->BeaconAttempted = TRUE;
                couldTransmit = TRUE;
                break;

            case DOT11_S_WFBEACON:
                MacDot11ApTransmitBeacon(node, dot11);
                couldTransmit = TRUE;
                break;

            default:
                break;
        } //switch
    }// End else
    return couldTransmit;
}// MacDot11ApAttemptToTransmitBeacon


//--------------------------------------------------------------------------
//  NAME:        MacDot11ApBeaconTransmitted
//  PURPOSE:     Set variables and states after the transmit of beacon.
//               Set timer for next beacon.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11ApBeaconTransmitted(
    Node* node,
    MacDataDot11* dot11)
{

    clocktype currentTime = node->getNodeTime();
    clocktype cfpStartDelay;
    clocktype backOff;

      if (dot11->beaconInterval > 0) {

        dot11->beaconTime += dot11->beaconInterval;

        while (dot11->beaconTime < currentTime)
        {
            dot11->beaconTime += dot11->beaconInterval;
        }
    }
    dot11->BeaconAttempted = FALSE;
    dot11->beaconIsDue = FALSE;
    dot11->beaconPacketsSent++;
    if (MacDot11IsPC(dot11)){
        MacDot11CfpPcBeaconTransmitted(node, dot11);
    }

        MacDot11StationStartTimerOfGivenType(node, dot11,
            dot11->beaconTime - currentTime, MSG_MAC_DOT11_Beacon);

    // Start Timer for polling in case of PC

    if ((dot11->state == DOT11_CFP_START)){
      if ((dot11->apVars->lastDTIMCount == 0) && (dot11->pcVars->cfSet.cfpCount == 0)){
            if (dot11->pcVars->isBeaconDelayed == TRUE){

                backOff = (RANDOM_nrand(dot11->seed) % (dot11->cwMin + 1))
                        * dot11->slotTime;
                cfpStartDelay = dot11->txDifs + backOff;
                MacDot11StationStartTimer(node, dot11, cfpStartDelay);

            }
            else{
                MacDot11StationStartTimer(node, dot11, dot11->txSifs);
            }
        }
        else{
            MacDot11StationStartTimer(node, dot11, dot11->txSifs);
        }
    }
    else {
    MacDot11StationSetState(node, dot11, DOT11_S_IDLE);
    }
    if (DEBUG_BEACON)
    {
        char timeStr[100];

        ctoa(node->getNodeTime(), timeStr);

        printf(" \n%d TRANSMITTED beacon frame at %s\n", node->nodeId,
                timeStr);
    }
    if (dot11->beaconSet == TRUE)
    {
        dot11->beaconSet = FALSE;
        if (dot11->dot11TxFrameInfo != NULL) {
            if (dot11->currentMessage == dot11->dot11TxFrameInfo->msg){
                MEM_free(dot11->dot11TxFrameInfo);
                dot11->dot11TxFrameInfo = NULL;
            }
        }
        if (dot11->isHTEnable)
        {
            MESSAGE_Free(node, dot11->currentMessage);
        }
        dot11->currentMessage = NULL;
        dot11->currentNextHopAddress = ANY_MAC802;
        dot11->ipNextHopAddr = ANY_MAC802;
        dot11->ipDestAddr = ANY_MAC802;
        dot11->ipSourceAddr = ANY_MAC802;
        dot11->dataRateInfo = NULL;
    }
    else
    {
        dot11->instantMessage = NULL;
    }
}// MacDot11ApBeaconTransmitted


#if 0

//--------------------------------------------------------------------------
//  NAME:        MacDot11ApGetTxDataRate
//  PURPOSE:     Determine transmit data rate based on frame type,
//               destination address and ack destination (if ack is due).
//  PARAMETERS:  Node* node
//                  Pointer to node
//               const MacDataDot11 * const dot11
//                  Pointer to Dot11 structure
//               Mac802Address destAddr
//                  Destination address of frame
//               dot11_11MacFrameType frameToSend
//                  Frame type to send
//               int* dataRate
//                  Data rate to use for transmit.
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static
void MacDot11ApGetTxDataRate(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr,
    dot11_11MacFrameType frameToSend,
    int* dataRateType)
{
    DOT11_PcVars* ap = dot11->apVars;
    dot11_11DataRateEntry* dataRateEntry;

    *dataRateType = dot11->lowestDataRateType;

    if (frameToSend == DOT11_CF_NONE) {
        return;
    }

    switch (frameToSend) {

        case DOT11_DATA:
        case DOT11_CF_DATA_POLL:
            *dataRateType = dot11->dataRateInfo->dataRateType;
            break;

        case DOT11_CF_POLL:
            dataRateEntry =
                MacDot11GetDataRateEntry(node, dot11, destAddr);
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

#endif

//--------------------------------------------------------------------------
//  NAME:        MacDot11ApFreeStorage.
//  PURPOSE:     Free memory at the end of simulation.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               int interfaceIndex
//                  interfaceIndex, Interface index
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11ApFreeStorage(
    Node* node,
    int interfaceIndex)
{
    MacDataDot11* dot11 =
        (MacDataDot11*)node->macData[interfaceIndex]->macVar;

    if (MacDot11IsAp(dot11)) {
        DOT11_ApVars* ap = dot11->apVars;

        // Clear station list
        MacDot11ApStationListFree(node, dot11, ap->apStationList);

        MEM_free(ap);
    }
}

