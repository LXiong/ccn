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

#include "api.h"
#include "partition.h"

#include "mac.h"
#include "mac_switch.h"

#include "mac_garp.h"
#include "mac_gvrp.h"


//--------------------------------------------------------------------------
// Vlan library functions


// NAME:        Switch_IsValidPortNumber
//
// PURPOSE:     Check validity for the port number
//
// PARAMETERS:  Switch data
//              Port number
//
// RETURN:      TRUE, if an existing port for the switch
//              FALSE, otherwise

static
BOOL Switch_IsValidPortNumber(
    MacSwitch* sw,
    unsigned short portNumber)
{
    SwitchPort* port = sw->firstPort;

    while (port)
    {
        if (port->portNumber == portNumber)
        {
            return TRUE;
        }
        port = port->nextPort;
    }
    return FALSE;
}


// NAME:        Switch_IsValidVlanId
//
// PURPOSE:     Check validity for the vlan id
//
// PARAMETERS:  Switch data
//              Vlan id
//
// RETURN:      TRUE, if vlan present in database
//              FALSE, otherwise

static
BOOL Switch_IsValidVlanId(
    MacSwitch* sw,
    VlanId vlanId)
{
    ListItem* listItem;
    SwitchVlanDbItem* dbItem = NULL;

    listItem = sw->vlanDbList->first;
    while (listItem)
    {
        dbItem = (SwitchVlanDbItem*) listItem->data;

        if (dbItem->vlanId == vlanId)
        {
            return TRUE;
        }
        listItem = listItem->next;
    }
    return FALSE;
}


// NAME:        SwitchVlan_GetVlanConfigType
//
// PURPOSE:     Get vlan configuration of a switch.
//
// PARAMETERS:  Switch data
//
// RETURN:      vlan configuration type.

static
SwitchVlanConfigurationType SwitchVlan_GetVlanConfigType(
    MacSwitch* sw)
{
    if (sw->runGvrp)
    {
        return SWITCH_VLAN_CONFIG_DYNAMIC;
    }
    return SWITCH_VLAN_CONFIG_STATIC;
}


// NAME:        SwitchVlan_GetDbItem
//
// PURPOSE:     Get database item for given vlan
//
// PARAMETERS:  Switch data
//              Vlan id
//
// RETURN:      Pointer to db structure, if vlan present in db
//              NULL, otherwise

SwitchVlanDbItem* SwitchVlan_GetDbItem(
    MacSwitch* sw,
    VlanId vlanId)
{
    ListItem* listItem = NULL;
    SwitchVlanDbItem* dbItem = NULL;

    listItem = sw->vlanDbList->first;
    while (listItem)
    {
        dbItem = (SwitchVlanDbItem*) listItem->data;

        if (dbItem->vlanId == vlanId)
        {
            return dbItem;
        }
        listItem = listItem->next;
    }

    // VLAN ID not present in database
    return NULL;
}


// NAME:        SwitchDb_GetFidForGivenVlan
//
// PURPOSE:     Finds filter database id for given vlan
//
// PARAMETERS:  Switch data
//              Vlan number
//
// RETURN:      Filter database id, if a known vlan for the switch
//              SWITCH_FID_UNKNOWN, otherwise

unsigned short SwitchDb_GetFidForGivenVlan(
    MacSwitch* sw,
    VlanId vlanId)
{
    SwitchVlanDbItem* dbItemForGivenVlan = SwitchVlan_GetDbItem(sw, vlanId);

    return (unsigned short)(dbItemForGivenVlan != NULL ?
        dbItemForGivenVlan->currentFid : SWITCH_FID_UNKNOWN);
}


// NAME:        SwitchVlan_IsPortInMemberList
//
// PURPOSE:     Determine if given port number is in the member's list
//
// PARAMETERS:  List to check
//              Port number
//
// RETURN:      TRUE, if port is present in list
//              FALSE, otherwise

BOOL SwitchVlan_IsPortInMemberList(
    LinkedList* list,
    unsigned short thePortNumber)
{
    ListItem* listItem = NULL;

    listItem = list->first;
    while (listItem)
    {
        if (*((unsigned short*)(listItem->data))
            == thePortNumber)
        {
            return TRUE;
        }
        listItem = listItem->next;
    }

    return FALSE;
}

// NAME:        SwitchVlan_IsPortInMemberSet
//
// PURPOSE:     Check if port present in member set for given vlan.
//
// PARAMETERS:  Switch data
//              Port number
//              Vlan number
//
// RETURN:      TRUE, if port present in member set
//              FALSE, otherwise

BOOL SwitchVlan_IsPortInMemberSet(
    MacSwitch* sw,
    unsigned short thePortNumber,
    VlanId vlanId)
{
    BOOL isInMemberSet = FALSE;
    SwitchVlanDbItem* dbItemForGivenVlan = SwitchVlan_GetDbItem(sw, vlanId);

    // Unknown Vlan
    if (!dbItemForGivenVlan)
    {
        return FALSE;
    }

    switch (SwitchVlan_GetVlanConfigType(sw))
    {

    case SWITCH_VLAN_CONFIG_STATIC:
        isInMemberSet = SwitchVlan_IsPortInMemberList(
            dbItemForGivenVlan->memberSet, thePortNumber);
        break;

    case SWITCH_VLAN_CONFIG_DYNAMIC:
        isInMemberSet = SwitchVlan_IsPortInMemberList(
            dbItemForGivenVlan->dynamicMemberSet, thePortNumber);
        break;

    default:
        ERROR_ReportError(
            "SwitchVlan_IsPortInMemberSet: "
            "Unknown VLAN configuration type.\n");
        break;
    }

    return isInMemberSet;
}


// NAME:        Switch_IsOutgoingFrameTagged
//
// PURPOSE:     Check if vlan tagging required in frame header.
//
// PARAMETERS:  Node data
//              Vlan number
//              Port number
//
// RETURN:      TRUE, if port present in untagged set for given vlan
//              FALSE, otherwise

BOOL Switch_IsOutgoingFrameTagged(
    Node* node,
    MacSwitch* sw,
    VlanId vlanClassification,
    unsigned short portNumber)
{
    SwitchVlanDbItem* dbItem = NULL;

    if (SwitchVlan_GetVlanConfigType(sw) == SWITCH_VLAN_CONFIG_DYNAMIC)
    {
        return TRUE;
    }

    // Find untagged set for this Vlan
    dbItem = SwitchVlan_GetDbItem(sw, vlanClassification);

    // Check if outgoing frame header is to be VLAN tagged
    if (dbItem)
    {
        ListItem* untaggedItem = dbItem->untaggedSet->first;

        while (untaggedItem)
        {
            if (*((unsigned short*)(untaggedItem->data)) == portNumber)
            {
                // Port present in untagged set
                return FALSE;
            }
            untaggedItem = untaggedItem->next;
        }
    }

    // Either untagged set not present or port not present
    // in untagged set. So outgoing frame should be tagged.
    return TRUE;
}


// NAME:        SwitchVlan_GetStaticMembership
//
// PURPOSE:     Get statically configured Vlans for a given port.
//
// PARAMETERS:  Switch data
//              Port number
//              Flag control finding of first and next items.
//              Previous Vlan ID already found.
//
// RETURN:      VlanId, if configured for the given port.
//              SWITCH_VLAN_ID_INVALID, otherwise.

VlanId SwitchVlan_GetStaticMembership(
    MacSwitch* sw,
    unsigned short thePortNumber,
    BOOL isFirst,
    VlanId prevVlanId)
{
    ListItem* listItem = NULL;
    SwitchVlanDbItem* dbItem = NULL;

    listItem = sw->vlanDbList->first;
    while (listItem)
    {
        dbItem = (SwitchVlanDbItem*) listItem->data;

        if (isFirst || dbItem->vlanId == prevVlanId)
        {
            // This block is used to skip the dbItem
            // of prevVlanId. This helps to find if the
            // next VLANs exist in static database after
            // a particular VLAN.
            if (!isFirst)
            {
                // Next item not present.
                if (listItem->next == NULL)
                {
                    return SWITCH_VLAN_ID_INVALID;
                }

                listItem = listItem->next;
                dbItem = (SwitchVlanDbItem*) listItem->data;
                isFirst = TRUE;
            }

            if (SwitchVlan_IsPortInMemberList(
                    dbItem->memberSet, thePortNumber))
            {
                return dbItem->vlanId;
            }
        }
        listItem = listItem->next;
    }

    return SWITCH_VLAN_ID_INVALID;
}


//--------------------------------------------------------------------------


// NAME:        SwitchVlan_AddAllPortsToMemberSet
//
// PURPOSE:     Add all the active port in member set.
//
// PARAMETERS:  Node data
//              Switch data
//              List to add
//
// RETURN:      NONE
//

static
void SwitchVlan_AddAllPortsToMemberSet(
    Node* node,
    MacSwitch* sw,
    LinkedList* list)
{
    SwitchPort* port = sw->firstPort;
    unsigned short* portNumber;

    while (port)
    {
        portNumber = (unsigned short*) MEM_malloc(sizeof(short));

        *portNumber = port->portNumber;
        ListInsert(node, list, 0, (void*) portNumber);

        port = port->nextPort;
    }
}


// NAME:        SwitchVlan_ParseInputPorts
//
// PURPOSE:     Add ports provided by token string. in given list
//
// PARAMETERS:  Node data
//              Switch data
//              List to add
//
// RETURN:      NONE
//

static
void SwitchVlan_ParseInputPorts(
    Node* node,
    MacSwitch* sw,
    LinkedList* list,
    const char* tokenString)
{
    char* next;
    const char* delim = " \t,{}";
    char* token = NULL;
    char iotoken[MAX_STRING_LENGTH];
    char tempString[MAX_STRING_LENGTH];
    char errString[MAX_STRING_LENGTH];

    unsigned givenNum;
    unsigned short* portNumber = NULL;

    strcpy(tempString, tokenString);
    token = IO_GetDelimitedToken(iotoken, tempString, delim, &next);

    if (!strcmp(token, "ALL"))
    {
        SwitchVlan_AddAllPortsToMemberSet(node, sw, list);
    }
    else
    {
        strcpy(tempString, tokenString);
        token = IO_GetDelimitedToken(iotoken, tempString, delim, &next);

        while (token)
        {
            if (!isdigit(*token))
            {
                sprintf(errString,
                    "SwitchVlan_ParseInputPorts: "
                    "Input for tagged/untagged set should be ALL or "
                    "list of port numbers.\n"
                    "Input was %s", tokenString);

                ERROR_ReportError(errString);
            }

            givenNum = atoi(token);

            if (Switch_IsValidPortNumber(sw, (unsigned short)givenNum))
            {
                portNumber = (unsigned short*)
                    MEM_malloc(sizeof(short));
                *portNumber = (unsigned short)givenNum;
                ListInsert(node, list, 0, (void*) portNumber);
            }
            else
            {
                sprintf(errString,
                    "SwitchVlan_ParseInputPorts: "
                    "Input for tagged/untagged set has an "
                    "invalid port %d.\n"
                    "Input was %s", givenNum, tokenString);
                ERROR_ReportError(errString);
            }
            token = IO_GetDelimitedToken(iotoken, next, delim, &next);
        }
    }
}


// NAME:        SwitchVlan_CreateSpecialMemberSets
//
// PURPOSE:     Create special member and/or untagged set
//
// PARAMETERS:  Node data
//              Switch data
//              Vlan whose member and/or untagged set going to create
//              Flag indicates if untagged set is required
//
// RETURN:      NONE

static
void SwitchVlan_CreateSpecialMemberSets(
    Node* node,
    MacSwitch* sw,
    VlanId vlanId,
    BOOL isUntaggedSetRequired)
{
    SwitchVlanDbItem* specialDb = NULL;

    // This function, called after initializing the vlanDbList,
    // collects database item for different vlans.

    ERROR_Assert(sw->vlanDbList,
        "SwitchVlan_CreateSpecialMemberSets: "
        "Vlan database list not initialized");

    // For reserved Vlan only.
    specialDb = (SwitchVlanDbItem*) MEM_malloc(sizeof(SwitchVlanDbItem));

    specialDb->vlanId = vlanId;
    ListInit(node, &specialDb->memberSet);
    ListInit(node, &specialDb->untaggedSet);
    ListInit(node, &specialDb->dynamicMemberSet);

    // Collects ports in member set and dynamic member set.
    SwitchVlan_AddAllPortsToMemberSet(node, sw, specialDb->memberSet);
    SwitchVlan_AddAllPortsToMemberSet(
        node, sw, specialDb->dynamicMemberSet);

    if (isUntaggedSetRequired)
    {
        // Collects ports in untagged set
        SwitchVlan_AddAllPortsToMemberSet(node, sw, specialDb->untaggedSet);
    }

    ListInsert(node, sw->vlanDbList, 0, (void *)specialDb);
}


// NAME:        SwitchVlan_ValidateSets
//
// PURPOSE:     Check user specified configuration for
//              valid tagged and untagged sets
//
// PARAMETERS:  Switch data
//
// RETURN:      NONE

static
void SwitchVlan_ValidateSets(
    MacSwitch* sw)
{
    ListItem* listItem = NULL;
    SwitchVlanDbItem* dbItem = NULL;
    char errString[MAX_STRING_LENGTH];
    SwitchPort* aPort;
    VlanId aPvid;

    listItem = sw->vlanDbList->first;
    while (listItem)
    {
        dbItem = (SwitchVlanDbItem*) listItem->data;

        // Member set should be given for every untagged set
        if (dbItem->memberSet->size == 0)
        {
            sprintf(errString,
                "SwitchVlan_ValidateSets: "
                "No member set found for vlan %u in switch %d.\n"
                "A member set should exist for every untagged set.\n",
                dbItem->vlanId, sw->swNumber);

            ERROR_ReportError(errString);
        }

        // Member sets need at least two valid port to forward.
        // Note: Currently this check is commented out, as
        //       it is meaningless for GVRP. This could be
        //       used if post Init checking is required.
        //if (dbItem->memberSet->size < 2)
        //{
        //    sprintf(errString,
        //        "SwitchVlan_ValidateSets: "
        //        "Less than 2 ports in vlan %u's member set at switch %u.\n"
        //        "This switch will not be able to forward "
        //        "frames for this VLAN.\n",
        //        dbItem->vlanId, sw->swNumber);
        //    ERROR_ReportWarning(errString);
        //}

        // Check untagged set
        if (dbItem->untaggedSet->size)
        {
            ListItem* item = NULL;
            unsigned short* portNumber = NULL;

            item = dbItem->untaggedSet->first;
            while (item)
            {
                portNumber = (unsigned short*)(item->data);

                if (SwitchVlan_GetVlanConfigType(sw) ==
                        SWITCH_VLAN_CONFIG_STATIC
                    && !SwitchVlan_IsPortInMemberSet(
                        sw,
                        *((unsigned short*)(item->data)),
                        dbItem->vlanId))
                {
                    sprintf(errString,
                        "SwitchVlan_ValidateSets: "
                        "Improper configuration for untagged set "
                        "for vlan %u in switch %u\n"
                        "Port %u does not exist in member set for vlan %u",
                        dbItem->vlanId, sw->swNumber,
                        *((unsigned short*)(item->data)), dbItem->vlanId);

                    ERROR_ReportError(errString);
                }
                item = item->next;
            }
        }

        listItem = listItem->next;
    }

    aPort = sw->firstPort;
    while (aPort != NULL)
    {
        aPvid = aPort->pVid;
        if (aPvid != SWITCH_VLAN_ID_DEFAULT)
        {
            if (SwitchVlan_GetVlanConfigType(sw) ==
                    SWITCH_VLAN_CONFIG_STATIC
                && !SwitchVlan_IsPortInMemberSet(
                    sw, aPort->portNumber, aPvid))
            {
                sprintf(errString,
                    "SwitchVlan_ValidateSets: "
                    "Switch %u, port %u with VLAN ID %u "
                    "missing from that VLAN's member set.\n",
                    sw->swNumber, aPort->portNumber, aPvid);

                ERROR_ReportWarning(errString);
            }
        }
        aPort = aPort->nextPort;
    }
}


// NAME:        SwitchVlan_ReadMemberSets
//
// PURPOSE:     Read user specified member sets and untagged
//              sets for a switch.
//
// PARAMETERS:  Node data
//              Switch data
//              NodeInput
//
// RETURN:      NONE
//

static
void SwitchVlan_ReadMemberSets(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput)
{
    BOOL memberFlag;
    BOOL untaggedFlag;
    char memberString[MAX_STRING_LENGTH];
    char untaggedString[MAX_STRING_LENGTH];
    int vlanId;
    SwitchVlanDbItem *tempItem;

    // Initialize database list
    ListInit(node, &sw->vlanDbList);

    for (vlanId = SWITCH_VLAN_ID_MIN;
         vlanId <= SWITCH_VLAN_ID_MAX;
         vlanId++)
    {
        // Read Member set
        // [node] SWITCH-VLAN-MEMBER-SET[vlan id] <port number list or ALL>
        IO_ReadStringInstance(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "SWITCH-VLAN-MEMBER-SET",
            vlanId,
            FALSE,
            &memberFlag,
            memberString);

        // Read untagged set
        // [node] SWITCH-VLAN-UNTAGGED-MEMBER-SET[vlan id]
        //                                  <port number list or ALL>
        IO_ReadStringInstance(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "SWITCH-VLAN-UNTAGGED-MEMBER-SET",
            vlanId,
            FALSE,
            &untaggedFlag,
            untaggedString);

        if (memberFlag || untaggedFlag)
        {
            tempItem = (SwitchVlanDbItem*)
                    MEM_malloc(sizeof(SwitchVlanDbItem));
            memset(tempItem, 0, sizeof(SwitchVlanDbItem));

            // Read and initialize variables
            tempItem->vlanId = (VlanId) vlanId;
            ListInit(node, &tempItem->memberSet);
            ListInit(node, &tempItem->untaggedSet);
            ListInit(node, &tempItem->dynamicMemberSet);

            if (memberFlag)
            {
                SwitchVlan_ParseInputPorts(
                    node, sw, tempItem->memberSet, memberString);
            }

            if (untaggedFlag)
            {
                SwitchVlan_ParseInputPorts(
                    node, sw, tempItem->untaggedSet, untaggedString);
            }

            ListInsert(node, sw->vlanDbList, 0,(void *) tempItem);
        }
    }

    // Create special set for STP BPDUs and GVRP PDUs.
    SwitchVlan_CreateSpecialMemberSets(
        node, sw, SWITCH_VLAN_ID_STP, FALSE);
    SwitchVlan_CreateSpecialMemberSets(
        node, sw, SWITCH_VLAN_ID_GVRP, FALSE);
}


// NAME:        SwitchDb_AddFidVlanMap
//
// PURPOSE:     Creates a filter database mapping between fidMapList
//              vlanDbList in given switch.
//
// PARAMETERS:  Switch data
//
// RETURN:      NONE
//

static
void SwitchDb_AddFidVlanMap(
    MacSwitch* sw)
{
    ListItem* fidMapItem = NULL;
    ListItem* vlanIdItem = NULL;
    VlanId* vlanIdPtr = NULL;
    SwitchDbFidMap* fidMapData = NULL;
    SwitchVlanDbItem* dbItemToMapFid = NULL;

    fidMapItem = sw->fidMapList->first;
    while (fidMapItem)
    {
        fidMapData = (SwitchDbFidMap*) fidMapItem->data;

        // Retrieve all vlanId present in this fid and
        // modify the database item with this fid

        vlanIdItem = fidMapData->vlanList->first;
        while (vlanIdItem)
        {
            vlanIdPtr = (VlanId*) vlanIdItem->data;

            // Find database item for this vlan id
            dbItemToMapFid = SwitchVlan_GetDbItem(sw, *vlanIdPtr);

            // Map fid of this database item
            if (dbItemToMapFid)
            {
                dbItemToMapFid->currentFid = fidMapData->fid;
            }

            vlanIdItem = vlanIdItem->next;
        }

        fidMapItem = fidMapItem->next;
    }
}


// NAME:        SwitchDb_InsertVlan
//
// PURPOSE:     Insert vlan number in given list.
//
// PARAMETERS:  Node data
//              List to insert
//              Vlan number
//
// RETURN:      NONE
//

static
void SwitchDb_InsertVlan(
    Node* node,
    LinkedList* list,
    VlanId vlanId)
{
    VlanId* aVlanIdPtr = NULL;

    aVlanIdPtr = (VlanId *) MEM_malloc(sizeof(VlanId));
    *aVlanIdPtr = vlanId;
    ListInsert(node, list, 0,(void *) aVlanIdPtr);
}


// NAME:        SwitchDb_MappingForSharedDb
//
// PURPOSE:     Initialize fidMapList and map primarily with
//              vlanDbList for shared database learning
//
// PARAMETERS:  Node data
//              Switch data
//              NodeInput
//
// RETURN:      NONE
//

static
void SwitchDb_MappingForSharedDb(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput)
{
    ListItem* listItem = NULL;
    SwitchVlanDbItem* dbItem = NULL;
    SwitchDbFidMap* tempFidMap = NULL;

    ListInit(node, &sw->fidMapList);

    // Allocate space for a common Fid map and collects VlanIds
    tempFidMap = (SwitchDbFidMap*)
            MEM_malloc(sizeof(SwitchDbFidMap));

    tempFidMap->fid = SWITCH_FID_DEFAULT;
    ListInit(node, &tempFidMap->vlanList);

    listItem = sw->vlanDbList->first;
    while (listItem)
    {
        dbItem = (SwitchVlanDbItem*) listItem->data;
        SwitchDb_InsertVlan(node, tempFidMap->vlanList, dbItem->vlanId);

        listItem = listItem->next;
    }

    ListInsert(node, sw->fidMapList, 0, (void *) tempFidMap);

    // Map Fid field in vlan database
    SwitchDb_AddFidVlanMap(sw);
}


// NAME:        SwitchDb_MappingForIndependentDb
//
// PURPOSE:     Initialize fidMapList and map primarily with
//              vlanDbList for independent database learning
//
// PARAMETERS:  Node data
//              Switch data
//              NodeInput
//
// RETURN:      NONE

static
void SwitchDb_MappingForIndependentDb(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput)
{
    ListItem* listItem = NULL;
    SwitchVlanDbItem* dbItem = NULL;
    SwitchDbFidMap* tempFidMap = NULL;

    ListInit(node, &sw->fidMapList);

    // Allocate Fid map for each Vlan.
    listItem = sw->vlanDbList->first;
    while (listItem)
    {
        tempFidMap = (SwitchDbFidMap *)
                MEM_malloc(sizeof(SwitchDbFidMap));

        ListInit(node, &tempFidMap->vlanList);

        dbItem = (SwitchVlanDbItem*) listItem->data;

        tempFidMap->fid = dbItem->vlanId;
        SwitchDb_InsertVlan(node, tempFidMap->vlanList, dbItem->vlanId);
        ListInsert(node, sw->fidMapList, 0,(void *) tempFidMap);

        listItem = listItem->next;
    }

    // Map Fid field in vlan database
    SwitchDb_AddFidVlanMap(sw);
}



// NAME:        Switch_IsValidForCombinedLearning
//
// PURPOSE:     Check validity to consider given vlan for
//              combined learning.
//
// PARAMETERS:  Node data
//              Switch data
//              Vlan number
//
// NOTE:        For combined learning, initially all possible vlans are
//              added in default fid database. User specified vlans for
//              different valid fid, are then removed from this list.
//              Remaining vlans are considered for default fid
//
//
// RETURN:      TRUE, if present in default fid vlan list.
//              FALSE, otherwise

static
BOOL Switch_IsValidForCombinedLearning(
    Node* node,
    MacSwitch* sw,
    VlanId checkVId)
{
    ListItem* fidMapItem = NULL;
    ListItem* vlanListItem = NULL;
    SwitchDbFidMap* fidMapData = NULL;
    VlanId* vlanIdPtr = NULL;

    fidMapItem = sw->fidMapList->first;
    while (fidMapItem)
    {
        fidMapData = (SwitchDbFidMap*) fidMapItem->data;

        if (fidMapData->fid == SWITCH_FID_DEFAULT)
        {
            vlanListItem = fidMapData->vlanList->first;
            while (vlanListItem)
            {
                vlanIdPtr = (VlanId*) vlanListItem->data;

                if (*vlanIdPtr == checkVId)
                {
                    // Remove this id and return true
                    ListGet(node, fidMapData->vlanList,
                        vlanListItem, TRUE, FALSE);

                    return TRUE;
                }
                vlanListItem = vlanListItem->next;
            }
            break;
        }
        fidMapItem = fidMapItem->next;
    }
    return FALSE;
}


// NAME:        SwitchDb_MappingForCombinedDb
//
// PURPOSE:     Initialize fidMapList and map primarily with
//              vlanDbList for combined database learning
//
// PARAMETERS:  Node data
//              Switch data
//              NodeInput
//
// RETURN:      NONE

static
void SwitchDb_MappingForCombinedDb(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput)
{
    int fid;
    int vid;
    BOOL retVal;
    char vlanIdList[MAX_STRING_LENGTH];

    SwitchDbFidMap* tempFidMap = NULL;
    char* next;
    const char* delim = " \t,{}";
    char* token = NULL;
    char iotoken[MAX_STRING_LENGTH];
    char tempIdList[MAX_STRING_LENGTH];
    int checkVId;

    ListInit(node, &sw->fidMapList);

    // Create a fid map list contains all vlans. and insert all
    // vlans into that map
    tempFidMap = (SwitchDbFidMap *)
            MEM_malloc(sizeof(SwitchDbFidMap));

    ListInit(node, &tempFidMap->vlanList);
    tempFidMap->fid = SWITCH_FID_DEFAULT;
    for (vid = SWITCH_VLAN_ID_MIN; vid <= SWITCH_VLAN_ID_MAX; vid++)
    {
        SwitchDb_InsertVlan(node, tempFidMap->vlanList, (VlanId) vid);
    }
    ListInsert(node, sw->fidMapList, 0,(void *) tempFidMap);

    // [node] SWITCH-VLAN-COMBINED-LEARNING[dbId] <vlanIdList>
    for (fid = SWITCH_FID_DEFAULT + 1; fid <= SWITCH_FID_MAX; fid++)
    {
        IO_ReadStringInstance(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "SWITCH-VLAN-COMBINED-LEARNING",
            fid,
            FALSE,
            &retVal,
            vlanIdList);

        if (retVal)
        {
            tempFidMap = (SwitchDbFidMap *)
                    MEM_malloc(sizeof(SwitchDbFidMap));

            ListInit(node, &tempFidMap->vlanList);
            tempFidMap->fid = (unsigned short)fid;

            // Parse vlanIdList to get Vlan ids
            strcpy(tempIdList, vlanIdList);
            token = IO_GetDelimitedToken(iotoken, tempIdList, delim, &next);

            while (token)
            {
                if (!isdigit(*token))
                {
                    char err[MAX_STRING_LENGTH];

                    sprintf(err,
                        "SwitchDb_MappingForCombinedDb: "
                        "The VLAN ID set has non-numeric characters.\n"
                        "Input was %s\n",
                        token);

                    ERROR_ReportError(err);
                }

                checkVId = atoi(token);

                if (!Switch_IsValidForCombinedLearning(
                        node, sw, (VlanId)checkVId))
                {
                    char err[MAX_STRING_LENGTH];

                    sprintf(err,
                        "SwitchDb_MappingForCombinedDb: "
                        "Vlan Id %u appears in more than one "
                        "Filter db id\n", checkVId);

                    ERROR_ReportError(err);
                }

                SwitchDb_InsertVlan(
                    node, tempFidMap->vlanList, (VlanId)checkVId);
                token = IO_GetDelimitedToken(iotoken, next, delim, &next);
            }

            ListInsert(node, sw->fidMapList, 0,(void *) tempFidMap);
        }
    }

    // Map Fid field in vlan database
    SwitchDb_AddFidVlanMap(sw);
}


// NAME:        SwitchDb_MappingForDefaultDb
//
// PURPOSE:     Initialize fidMapList and map primarily with
//              vlanDbList for vlan unaware switch.
//
// PARAMETERS:  Node data
//              Switch data
//              NodeInput
//
// RETURN:      NONE

static
void SwitchDb_MappingForDefaultDb(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput)
{
    SwitchVlanDbItem* defaultDb = NULL;

    // Create member set for default setting

    ListInit(node, &sw->vlanDbList);

    // For default scenario, only one Vlan should be there,
    // and Vlan Id will be the default value.
    defaultDb = (SwitchVlanDbItem *) MEM_malloc(sizeof(SwitchVlanDbItem));

    defaultDb->vlanId = SWITCH_VLAN_ID_DEFAULT;
    ListInit(node, &defaultDb->memberSet);
    ListInit(node, &defaultDb->untaggedSet);
    ListInit(node, &defaultDb->dynamicMemberSet);

    // Collects ports for membet set
    SwitchVlan_AddAllPortsToMemberSet(node, sw, defaultDb->memberSet);

    // Collects ports for untagged set
    SwitchVlan_AddAllPortsToMemberSet(node, sw, defaultDb->untaggedSet);

    ListInsert(node, sw->vlanDbList, 0,(void *) defaultDb);

    // Call Fid mapping function for default setting.
    SwitchDb_MappingForSharedDb(node, sw, nodeInput);
}


// NAME:        Switch_PortsSetVlanDefaults
//
// PURPOSE:     Initialize default values to vlan port parameters
//
// PARAMETERS:  Node data
//              Switch data
//
// RETURN:      NONE
//

static
void Switch_PortsSetVlanDefaults(
    Node* node,
    MacSwitch* sw)
{
    SwitchPort* aPort = sw->firstPort;

    while (aPort)
    {
        aPort->pVid = SWITCH_VLAN_ID_DEFAULT;
        aPort->acceptFrameType = SWITCH_ACCEPT_ALL_FRAMES;
        aPort->enableIngressFiltering = FALSE;
        aPort = aPort->nextPort;
    }
}


// NAME:        Switch_VlanInit
//
// PURPOSE:     Initialize vlan related variables for a switch.
//
// PARAMETERS:  Node data
//              Switch data
//              Port number
//
// RETURN:      NONE
//

void Switch_VlanInit(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];

    sw->isVlanAware = FALSE;
    sw->fidMapList = NULL;
    sw->vlanDbList = NULL;
    sw->isMemberSetAware = TRUE;

    // [node] SWITCH-VLAN-AWARE YES | NO
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-VLAN-AWARE",
        &retVal,
        buf);

    if (retVal)
    {
        if (!strcmp(buf, "YES"))
        {
            sw->isVlanAware = TRUE;
        }
        else if (!strcmp(buf, "NO"))
        {
            sw->isVlanAware = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "Switch_VlanInit: "
                "Error in SWITCH-VLAN-AWARE specification.\n"
                "Expecting YES or NO\n");
        }
    }

    // [node] SWITCH-FORWARDING-IS-MEMBER-SET-AWARE YES | NO
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-FORWARDING-IS-MEMBER-SET-AWARE",
        &retVal,
        buf);

    if (retVal)
    {
        if (!strcmp(buf, "YES"))
        {
            sw->isMemberSetAware = TRUE;
        }
        else if (!strcmp(buf, "NO"))
        {
            sw->isMemberSetAware = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "Switch_VlanInit: "
                "Error in SWITCH-FORWARDING-MEMBER-SET-AWARE user input.\n"
                "Expecting YES or NO\n");
        }
    }

    if (!sw->isVlanAware)
    {
        // Switch not Vlan aware.
        // Set default values to different vlan port parameters,
        // call default "Mapping Function" and return

        Switch_PortsSetVlanDefaults(node, sw);
        SwitchDb_MappingForDefaultDb(node, sw, nodeInput);
        return;
    }

    SwitchVlan_ReadMemberSets(node, sw, nodeInput);

    // [node] SWITCH-VLAN-LEARNING   SHARED | INDEPENDENT | COMBINED
    // Default learning for vlan aware switch is shared learning
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-VLAN-LEARNING",
        &retVal,
        buf);


    if (retVal)
    {
        if (!strcmp(buf, "SHARED"))
        {
            sw->learningType = SWITCH_LEARNING_SHARED;
            SwitchDb_MappingForSharedDb(node, sw, nodeInput);
        }
        else if (!strcmp(buf, "INDEPENDENT"))
        {
            sw->learningType = SWITCH_LEARNING_INDEPENDENT;
            SwitchDb_MappingForIndependentDb(node, sw, nodeInput);
        }
        else if (!strcmp(buf, "COMBINED"))
        {
            sw->learningType = SWITCH_LEARNING_COMBINED;
            SwitchDb_MappingForCombinedDb(node, sw, nodeInput);
        }
        else
        {
            ERROR_ReportError(
                "Switch_VlanInit: "
                "Error in SWITCH-VLAN-LEARNING specification.\n"
                "Expecting SHARED, INDEPENDENT or COMBINED\n");
        }
    }
    else
    {
        sw->learningType = SWITCH_LEARNING_SHARED;
        SwitchDb_MappingForSharedDb(node, sw, nodeInput);
    }

    // Ckeck and initialize GVRP for this switch.
    SwitchGvrp_Init(node, sw, nodeInput);

    if (!sw->runGvrp)
    {
        // Check user configuration for member and untagged sets.
        SwitchVlan_ValidateSets(sw);
    }
}


// NAME:        SwitchPort_VlanInit
//
// PURPOSE:     Initialize vlan related variables for a port.
//
// PARAMETERS:  Node data
//              Switch data
//              Port structure
//              NodeInput
//
// RETURN:      NONE
//

void SwitchPort_VlanInit(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    const NodeInput* nodeInput)
{
    BOOL retVal;
    int intInput;
    char buf[MAX_STRING_LENGTH];
    char errString[MAX_STRING_LENGTH];
    //Address address;

    // Get network type for this port.
    //
    //NetworkGetInterfaceInfo(
    //    node, port->macData->interfaceIndex, &address);

    //if (address.networkType != NETWORK_IPV4 &&
    //    address.networkType != NETWORK_IPV6)
    //{
    //    sprintf(errString,
    //        "SwitchPort_SchedulerInit: "
    //        "Given network type at interface %d for node %d "
    //        "is unknown type.\n",
    //        port->macData->interfaceIndex, node->nodeId);
    //    ERROR_ReportError(errString);
    //}


    // [node] SWITCH-PORT-VLAN-ID[port#] <number>  ||
    // [port address list] SWITCH-PORT-VLAN-ID <number>

    IO_ReadIntInstance(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-PORT-VLAN-ID",
        port->portNumber,
        TRUE,
        &retVal,
        &intInput);

    if (!retVal)
    {
        char buf[MAX_STRING_LENGTH];

        IO_ReadStringUsingIpAddress(
            node,
            port->macData->interfaceIndex,
            nodeInput,
            "SWITCH-PORT-VLAN-ID",
            &retVal,
            buf);

        if (retVal)
        {
            intInput = atoi(buf);
        }
    }

    port->pVid = SWITCH_VLAN_ID_DEFAULT;
    if (retVal)
    {
        if (intInput >= SWITCH_VLAN_ID_MIN
            && intInput <= SWITCH_VLAN_ID_MAX)
        {
            port->pVid = (VlanId) intInput;
        }
        else
        {
                sprintf(errString,
                    "SwitchPort_VlanInit: "
                    "Error in SWITCH-PORT-VLAN-ID specification.\n"
                    "Vlan IDs can range from %u to %u\n",
                    SWITCH_VLAN_ID_MIN, SWITCH_VLAN_ID_MAX);
                ERROR_ReportError(errString);
        }
    }


    // [node] SWITCH-PORT-VLAN-ADMIT-FRAMES[port#] ALL | TAGGED or
    // [port address list] SWITCH-PORT-ADMIT-FRAMES ALL | TAGGED

    IO_ReadStringInstance(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-PORT-VLAN-ADMIT-FRAMES",
        port->portNumber,
        TRUE,
        &retVal,
        buf);

    if (!retVal)
    {
        IO_ReadStringUsingIpAddress(
            node,
            port->macData->interfaceIndex,
            nodeInput,
            "SWITCH-PORT-VLAN-ADMIT-FRAMES",
            &retVal,
            buf);
    }

    port->acceptFrameType = SWITCH_ACCEPT_ALL_FRAMES;
    if (retVal)
    {
        if (!strcmp(buf, "ALL"))
        {
            port->acceptFrameType = SWITCH_ACCEPT_ALL_FRAMES;
        }
        else if (!strcmp(buf, "TAGGED"))
        {
            port->acceptFrameType = SWITCH_TAGGED_FRAMES_ONLY;
        }
        else
        {
            ERROR_ReportError(
                "SwitchPort_VlanInit: "
                "Error in SWITCH-PORT-VLAN-ADMIT-FRAMES specification.\n"
                "Expecting ALL or TAGGED\n");
        }
    }


    // [node] SWITCH-PORT-VLAN-INGRESS-FILTERING[port#] NONE | VLAN or
    // [port address list] SWITCH-PORT-INGRESS-FILTERING NONE | VLAN

    IO_ReadStringInstance(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-PORT-VLAN-INGRESS-FILTERING",
        port->portNumber,
        TRUE,
        &retVal,
        buf);

    if (!retVal)
    {
        IO_ReadStringUsingIpAddress(
            node,
            port->macData->interfaceIndex,
            nodeInput,
            "SWITCH-PORT-VLAN-INGRESS-FILTERING",
            &retVal,
            buf);
    }

    port->enableIngressFiltering = FALSE;
    if (retVal)
    {
        if (!strcmp(buf, "NONE"))
        {
            port->enableIngressFiltering = FALSE;
        }
        else if (!strcmp(buf, "VLAN"))
        {
            port->enableIngressFiltering = TRUE;
        }
        else
        {
            ERROR_ReportError(
                "SwitchPort_VlanInit: "
                "Error in SWITCH-PORT-VLAN-INGRESS-FILTERING specification.\n"
                "Expecting NONE or VLAN\n");
        }
    }

    // Format is [switch ID] SWITCH-PORT-VLAN-STATISTICS[port ID]   NO | YES
    //        or [port addr] SWITCH-PORT-VLAN-STATISTICS            NO | YES
    IO_ReadStringInstance(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-PORT-VLAN-STATISTICS",
        port->portNumber,
        TRUE,
        &retVal,
        buf);

    if (!retVal)
    {
        IO_ReadStringUsingIpAddress(
            node,
            port->macData->interfaceIndex,
            nodeInput,
            "SWITCH-PORT-VLAN-STATISTICS",
            &retVal,
            buf);
    }

    port->vlanStatEnabled = FALSE;
    if (retVal)
    {
        if (!strcmp(buf, "YES"))
        {
            port->vlanStatEnabled = TRUE;
        }
        else if (!strcmp(buf, "NO"))
        {
            port->vlanStatEnabled = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "SwitchPort_VlanInit: "
                "Error in SWITCH-PORT-VLAN-STATISTICS user input.\n"
                "Expecting YES or NO\n");
        }
    }
}


// NAME:        SwitchVlan_PrintStats
//
// PURPOSE:     Statistics of vlan specific frame forwarding
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchVlan_PrintStats(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    // Statistics are collected vlan specific. Iteration over
    // vlan required for details statistics
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    ListItem* item = NULL;
    SwitchPerVlanPortStat* dataItem = NULL;
    char interfaceAddrStr[MAX_STRING_LENGTH];

    NetworkIpGetInterfaceAddressString(
        node, port->macData->interfaceIndex, interfaceAddrStr);

    item = port->portVlanStatList->first;
    while (item)
    {
        dataItem = (SwitchPerVlanPortStat*) item->data;

        ctoa(dataItem->stats.numTotalFrameReceived, buf1);
        sprintf(buf, "Total frames received = %s", buf1);

        IO_PrintStat(
            node,
            "Mac-Switch",
            "Port",
            interfaceAddrStr,
            dataItem->vlanId,
            buf);

        ctoa(dataItem->stats.numUnicastDeliveredDirectly, buf1);
        sprintf(buf, "Unicast frames forwarded directly = %s", buf1);

        IO_PrintStat(
            node,
            "Mac-Switch",
            "Port",
            interfaceAddrStr,
            dataItem->vlanId,
            buf);

        ctoa(dataItem->stats.numUnicastFlooded, buf1);
        sprintf(buf, "Unicast frames flooded = %s", buf1);

        IO_PrintStat(
            node,
            "Mac-Switch",
            "Port",
            interfaceAddrStr,
            dataItem->vlanId,
            buf);

        ctoa(dataItem->stats.numUnicastDeliveredToUpperLayer, buf1);
        sprintf(buf, "Unicast frames delivered to upper layer = %s", buf1);

        IO_PrintStat(
            node,
            "Mac-Switch",
            "Port",
            interfaceAddrStr,
            dataItem->vlanId,
            buf);

        ctoa(dataItem->stats.numUnicastDropped, buf1);
        sprintf(buf, "Unicast frames dropped = %s", buf1);

        IO_PrintStat(
            node,
            "Mac-Switch",
            "Port",
            interfaceAddrStr,
            dataItem->vlanId,
            buf);

        ctoa(dataItem->stats.numBroadcastFlooded, buf1);
        sprintf(buf, "Broadcast frames forwarded = %s", buf1);

        IO_PrintStat(
            node,
            "Mac-Switch",
            "Port",
            interfaceAddrStr,
            dataItem->vlanId,
            buf);

        ctoa(dataItem->stats.numBroadcastDropped, buf1);
        sprintf(buf, "Broadcast frames dropped = %s", buf1);

        IO_PrintStat(
            node,
            "Mac-Switch",
            "Port",
            interfaceAddrStr,
            dataItem->vlanId,
            buf);

        ctoa(dataItem->stats.numDroppedInDiscardState, buf1);
        sprintf(buf, "Frames dropped in discard state = %s", buf1);

        IO_PrintStat(
            node,
            "Mac-Switch",
            "Port",
            interfaceAddrStr,
            dataItem->vlanId,
            buf);

        ctoa(dataItem->stats.numDroppedInLearningState, buf1);
        sprintf(buf, "Frames dropped in learning state = %s", buf1);

        IO_PrintStat(
            node,
            "Mac-Switch",
            "Port",
            interfaceAddrStr,
            dataItem->vlanId,
            buf);

        ctoa(dataItem->stats.numDroppedByIngressFiltering, buf1);
        sprintf(buf, "Frames dropped by ingress filtering = %s", buf1);

        IO_PrintStat(
            node,
            "Mac-Switch",
            "Port",
            interfaceAddrStr,
            dataItem->vlanId,
            buf);

        item = item->next;
    }
}


// NAME:        SwitchPort_VlanFinalize
//
// PURPOSE:     Call function to print portwise vlan statistics and
//              release statistic structure.
//
// PARAMETERS:  Node data
//              Switch data
//              Port structure
//
// RETURN:      NONE
//

void SwitchPort_VlanFinalize(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    if (port->vlanStatEnabled)
    {
        SwitchVlan_PrintStats(node, sw, port);
    }

    ListFree(node, port->portVlanStatList, FALSE);
}


// NAME:        Switch_VlanFinalize
//
// PURPOSE:     Release allocated memory for vlan in switch data.
//
// PARAMETERS:  Node data
//              Switch data
//
// RETURN:      TRUE, if an existing port for the switch
//              FALSE, otherwise

void Switch_VlanFinalize(
    Node* node,
    MacSwitch* sw)
{
    ListItem* listItem = NULL;
    SwitchVlanDbItem* dbItem = NULL;
    SwitchDbFidMap* fidMapItem = NULL;

    // Call GVRP to print stats and release
    // allocated memory.
    SwitchGvrp_Finalize(node, sw);


    // Free database item
    listItem = sw->vlanDbList->first;
    while (listItem)
    {
        dbItem = (SwitchVlanDbItem*) listItem->data;

        // Free member set and untag member set list
        ListFree(node, dbItem->memberSet, FALSE);
        ListFree(node, dbItem->untaggedSet, FALSE);
        ListFree(node, dbItem->dynamicMemberSet, FALSE);

        listItem = listItem->next;
    }

    ListFree(node, sw->vlanDbList, FALSE);


    // Free Fid list
    listItem = sw->fidMapList->first;
    while (listItem)
    {
        fidMapItem = (SwitchDbFidMap*) listItem->data;

        // Free vlan id list
        ListFree(node, fidMapItem->vlanList, FALSE);

        listItem = listItem->next;
    }

    ListFree(node, sw->fidMapList, FALSE);
}

//-------------------------------------------------------
