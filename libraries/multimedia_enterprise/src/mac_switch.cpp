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

#include "network_ip.h"
#include "ipv6.h"
#include "if_queue.h"
#include "if_scheduler.h"
#include "sch_strictprio.h"
#include "mac_switch.h"
#include "mac_gvrp.h"
#include "mac_link.h"
#include "mac_802_3.h"
#include "mac_background_traffic.h"

#define FULL_DUPLEX_DEBUG 0


// Standard STP switch group address is 01-80-c2-00-00-00
const char stpGroupAddress[6] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x00};


//--------------------------------------------------------------------------
// Switch library functions


// NAME:        Switch_GetSwitchData
//
// PURPOSE:     Return switch data for a node.
//
// PARAMETERS:  This node
//
// RETURN:      Pointer to switch data.

MacSwitch* Switch_GetSwitchData(
    Node* node)
{
    return node->switchData;
}


// NAME:        Switch_GetPortDataFromInterface
//
// PURPOSE:     Return port data for an interface.
//
// PARAMETERS:  Switch data.
//              Interface index.
//
// RETURN:      Pointer to port data for this interface.

SwitchPort* Switch_GetPortDataFromInterface(
    MacSwitch* sw,
    int interfaceIndex)
{
    SwitchPort* port = sw->firstPort;

    while (port)
    {
        // Loop through all port structures to find the matching interface
        if (port->macData->interfaceIndex == interfaceIndex)
        {
            return port;
        }
        port = port->nextPort;
    }

    ERROR_ReportError(
        "Switch_GetPortDataFromInterface: interface index not found.\n");

    return NULL;
}


// NAME:        Switch_GetPortDataFromPortNumber
//
// PURPOSE:     Return port data for a given port number.
//
// PARAMETERS:  Switch data
//              Port number
//
// RETURN:      Pointer to port data for this port.

SwitchPort* Switch_GetPortDataFromPortNumber(
    MacSwitch* sw,
    unsigned short portNumber)
{
    SwitchPort* port = sw->firstPort;

    while (port)
    {
        // Loop through all ports to find the matching port number
        if (port->portNumber == portNumber)
        {
            return port;
        }
        port = port->nextPort;
    }

    ERROR_ReportError(
        "Switch_GetPortDataFromPort: port number not found.\n");

    return NULL;
}


// NAME:        Switch_PortState
//
// PURPOSE:     Return state of a port
//
// PARAMETERS:  Port data.
//
// RETURN:      Current port state

static
StpPortState Switch_PortState(
    SwitchPort* port)
{
    StpPortState portState = STP_PORT_S_DISCARDING;

    if (!port->portEnabled)
    {
        return STP_PORT_S_DISCARDING;
    }

    switch (port->role)
    {

    case STP_ROLE_ROOT:
    case STP_ROLE_DESIGNATED:
        if (port->learning)
        {
            portState = STP_PORT_S_LEARNING;
        }
        if (port->forwarding)
        {
            portState = STP_PORT_S_FORWARDING;
        }
        break;

    case STP_ROLE_UNKNOWN:
    case STP_ROLE_ALTERNATEORBACKUP:
    case STP_ROLE_DISABLED:
        portState = STP_PORT_S_DISCARDING;
        break;

    default:
        ERROR_ReportError("Switch_PortState: Unknown port state.\n");
        break;

    } //switch

    return portState;
}


// NAME:        Switch_AssignSwAddress
//
// PURPOSE:     Return switch address for a switch number.
//              The switch (mac/ip) address is a combination
//              of the switch group address and the node ID.
//
// PARAMETERS:  Switch number.
//
// RETURN:      Switch address.

static
void Switch_AssignSwAddress(
    unsigned short swNumber, Mac802Address *swAddr, Mac802Address stpaddr)
{

    char str[3];
    strcpy(str, &stpGroupAddress[3]);

    unsigned int addr = atoi(str);
    addr += swNumber;

    memcpy(&(swAddr->byte[0]), &stpaddr, sizeof(NodeAddress));
    swAddr->byte[3] = ((unsigned char )(addr));
    swAddr->byte[4] = ((unsigned char )(addr >> 8));
    swAddr->byte[5] = ((unsigned char )(addr >> 16));
}


// NAME:        Switch_IsStpGroupAddr
//
// PURPOSE:     Checks whether an address is a Switch
//              Protocol Entity group address.
//
// PARAMETERS:  Address going to be checked
//
// RETURN:      TRUE, if group address
//              FALSE, otherwise

BOOL Switch_IsStpGroupAddr(Mac802Address destAddr, Mac802Address stpAddress)
{
    return MAC_IsIdenticalMac802Address(&destAddr, &stpAddress);
}

//Obsolete Function Remoave affer changes in all protocols
BOOL Switch_IsStpGroupAddr(NodeAddress destAddr)
{
    return FALSE;
}


// NAME:        Switch_IsGarpGroupAddr
//
// PURPOSE:     Checks if an address is a Garp group address.
//
// PARAMETERS:  Address to be checked
//
// RETURN:      TRUE, if group address
//              FALSE, otherwise

BOOL Switch_IsGarpGroupAddr(Mac802Address destAddr,
                            MacSwitch* sw )
{

    if (SwitchGvrp_GetGvrpData(sw))
    {
        return MAC_IsIdenticalMac802Address(&destAddr,
                                            &sw->gvrp->GVRP_Group_Address);
    }
    else
    {
        return FALSE;
    }
}


// NAME:        Switch_IsGarpAware
//
// PURPOSE:     Checks if switch is Garp aware.
//
// PARAMETERS:  Switch data.
//
// RETURN:      TRUE, if Garp aware.
//              FALSE, otherwise
//
// NOTE: This function should be modified, if other GARP application,
//       (viz. GMRP) are implemented.
//

BOOL Switch_IsGarpAware(
    MacSwitch* sw)
{
    return sw->runGvrp;
}

// NAME:        Switch_CheckUniquenessOfPortAddress
//
// PURPOSE:     Check whether the proposed address is
//              a unique address by looking up addresses
//              of ports already assinged. Used during
//              parsing of user input.
//
// PARAMETERS:  Proposed address of the port.
//
// RETURN:      None

static
void Switch_CheckUniquenessOfPortAddress(
    Node* node,
    MacSwitch* sw,
    Mac802Address anIpAddress)
{
    char errString[MAX_STRING_LENGTH];

    SwitchPort* aPort = sw->firstPort;

    while (aPort)
    {
        if (MAC_IsIdenticalMac802Address(&aPort->portAddr, &anIpAddress))
        {

            sprintf(errString,
                "Switch_CheckUniquenessOfPortAddress: "
                "Error in SWITCH-PORT-MAP in user configuration.\n"
                "Node %u has more than one port with port address ",
                node->nodeId);
            MAC_PrintMacAddr(&anIpAddress);

            ERROR_ReportError(errString);
        }
        aPort = aPort->nextPort;
    }
}


// NAME:        Switch_MapUserToQueuePriority
//
// PURPOSE:     Iterate over user priorities to create
//              the user priority to output queue map.
//              Follows Table 8-2
//
// PARAMETERS:  Array of priority to queue map
//              No of user specified queues
//
// RETURN:      None

static
void Switch_MapUserToQueuePriority(
    QueuePriorityType* priorityToQueueMap,
    unsigned short queueCount)
{
    TosType priority;

    for (priority = 0;
         priority < SWITCH_QUEUE_NUM_PRIORITIES_MAX;
         priority++)
    {
        priorityToQueueMap[priority] = 0;

        switch (queueCount)
        {

        case 1:
            break;

        case 2:
            if (priority >= 4)
            {
                priorityToQueueMap[priority] = 1;
            }
            break;

        case 3:
            if (priority >= 6)
            {
                priorityToQueueMap[priority] = 2;
            }
            else if (priority >= 4)
            {
                priorityToQueueMap[priority] = 1;
            }
            break;

        case 4:
            if (priority >= 6)
            {
                priorityToQueueMap[priority] = 3;
            }
            else if (priority >= 4)
            {
                priorityToQueueMap[priority] = 2;
            }
            else if (priority == 3 || priority == 0)
            {
                priorityToQueueMap[priority] = 1;
            }
            break;

        case 5:
            if (priority >= 6)
            {
                priorityToQueueMap[priority] = 4;
            }
            else if (priority == 5)
            {
                priorityToQueueMap[priority] = 3;
            }
            else if (priority == 4)
            {
                priorityToQueueMap[priority] = 2;
            }
            else if (priority == 3 || priority == 0)
            {
                priorityToQueueMap[priority] = 1;
            }
            break;

        case 6:
            if (priority >= 6)
            {
                priorityToQueueMap[priority] = 5;
            }
            else if (priority >= 3)
            {
                priorityToQueueMap[priority] = (unsigned char)(priority - 1);
            }
            else if (priority == 0)
            {
                priorityToQueueMap[priority] = 1;
            }
            break;

        case 7:
            if (priority >= 3)
            {
                priorityToQueueMap[priority] = (unsigned char)(priority - 1);
            }
            else if (priority == 0)
            {
                priorityToQueueMap[priority] = 1;
            }
            break;

        case 8:
            if (priority >= 3)
            {
                priorityToQueueMap[priority] = priority;
            }
            else if (priority >= 2)
            {
                priorityToQueueMap[priority] = 1;
            }
            else if (priority == 0)
            {
                priorityToQueueMap[priority] = 2;
            }
            break;

        default:
            ERROR_ReportError("Switch_AssignUserPriorityToMap: "
                "Queue count is not in the 0 to 7 acceptable range.\n");
            break;
        }
    }
}



// NAME:        Switch_ConvertUserPriorityToQueuePriority
//
// PURPOSE:     Convert user priority to queue priority.
//              The mapping of user priority to traffic classes
//              is standardized in Tables 7.1, 7.2 and 7.3
//
// PARAMETERS:  User priority
//              Equivalent queue priority for the port
//
// RETURN:      None

static
void Switch_ConvertUserPriorityToQueuePriority(
    QueuePriorityType* priorityMap,
    TosType userPriority,
    QueuePriorityType* queuePriority)
{
    *queuePriority = priorityMap[userPriority];
}


// NAME:        Switch_GetStatVariableForVlan
//
// PURPOSE:     Find stat variable to collect stat for given vlan.
//
// PARAMETERS:  Address of the port.
//              Vlan id
//
// RETURN:      None

static
SwitchPortStat* Switch_GetStatVariableForVlan(
    Node* node,
    SwitchPort* port,
    VlanId vlanId)
{
    ListItem* listItem = NULL;
    SwitchPerVlanPortStat* statItem = NULL;

    // Frame with null vlanId will be classified by pVid
    if (vlanId == SWITCH_VLAN_ID_NULL)
    {
        vlanId = port->pVid;
    }

    listItem = port->portVlanStatList->first;
    while (listItem)
    {
        statItem = (SwitchPerVlanPortStat*) listItem->data;

        if (statItem->vlanId == vlanId)
        {
            return &statItem->stats;
        }

        listItem = listItem->next;
    }

    // No item found for this vlan. Create a new item.
    statItem = (SwitchPerVlanPortStat*)
        MEM_malloc(sizeof(SwitchPerVlanPortStat));
    memset(statItem, 0, sizeof(SwitchPerVlanPortStat));

    statItem->vlanId = vlanId;
    ListInsert(node, port->portVlanStatList, 0,(void *) statItem);

    return &statItem->stats;
}


//--------------------------------------------------------------------------
// Scheduler / Queue functions


// NAME:        Switch_QueueIsEmpty
//
// PURPOSE:     Check whether queue is empty or not.
//
// PARAMETERS:  Interface index whose queue is checking.
//
// RETURN:      TRUE, if queue empty
//              FLASE, otherwise

BOOL Switch_QueueIsEmpty(
    Scheduler* scheduler)
{
    // To check whether switch queue is empty or not
    return ((*scheduler).isEmpty(ALL_PRIORITIES));
}


// NAME:        Switch_QueueNumInQueue
//
// PURPOSE:     Return number of packet in a queue.
//
// PARAMETERS:  Interface index whose queue is checking.
//              Flag to indicate looking for specific priority only.
//              Priority of the packet.
//
// RETURN:      Number of packets present.

static
int Switch_QueueNumInQueue(
    Scheduler* scheduler,
    QueuePriorityType priority)
{
    // Number of packets in the queue
    return ((*scheduler).numberInQueue(priority));
}


// NAME:        Switch_InEnqueueFrameWithPriorityMapping
//
// PURPOSE:     Enqueue a frame in the Input queue with user and
//              queue priority mapping
//
// PARAMETERS:  The scheduler in which to enqueue
//              Destination address
//              Source address
//              Vlan id
//              User requested priority
//              Port pointer, incoming port in case of own packet and
//              outgoing port in case of forwarded packet
//
// RETURN:      TRUE, if frame enqueued
//              FALSE, otherwise

static
void Switch_InEnqueueFrameWithPriorityMapping(
    Node* node,
    Scheduler* scheduler,
    QueuePriorityType* priorityMap,
    Message* msg,
    Mac802Address destAddr,
    Mac802Address sourceAddr,
    VlanId vlanId,
    TosType userPriority,
    int outgoingInterfaceIndex,
    BOOL* isEnqueued)
{
    QueuePriorityType queuePriority;
    BOOL queueIsFull = FALSE;

    SwitchInQueueInfo* info = NULL;

    MESSAGE_InfoAlloc(node, msg, sizeof(SwitchInQueueInfo));

    info = (SwitchInQueueInfo *) MESSAGE_ReturnInfo(msg);

    // Store necessary information that will be required while dequeueing
    // from Input/Cpu queue


    MAC_CopyMac802Address(&info->sourceAddr, &sourceAddr);
    MAC_CopyMac802Address(&info->destAddr, &destAddr);
    info->outgoingInterfaceIndex = outgoingInterfaceIndex;
    info->vlanId = vlanId;
    info->userPriority = userPriority;

    // Convert user priority to queue priority

    Switch_ConvertUserPriorityToQueuePriority(
        priorityMap,
        userPriority,
        &queuePriority);

    // Call scheduler insert function
    (*scheduler).insert(msg,
                        &queueIsFull,
                        queuePriority,
                        NULL, //infoField,
                        node->getNodeTime());

    *isEnqueued = !queueIsFull;
}


// NAME:        Switch_OutEnqueueFrameWithPriorityMapping
//
// PURPOSE:     Enqueue a frame to appropriate queue in the
//              outgoing port after user and queue priority mapping.
//
// PARAMETERS:  Destination address
//              Source address
//              Vlan id
//              User requested priority
//
// RETURN:      TRUE, if frame enqueued
//              FALSE, otherwise

static
void Switch_OutEnqueueFrameWithPriorityMapping(
    Node* node,
    SwitchPort* port,
    Message* msg,
    Mac802Address destAddr,
    Mac802Address sourceAddr,
    VlanId vlanId,
    TosType userPriority,
    BOOL* isEnqueued)
{
    QueuePriorityType queuePriority;
    BOOL queueIsFull = FALSE;
    Scheduler* scheduler = port->outPortScheduler;
    SwitchOutQueueInfo* info = NULL;

    MESSAGE_InfoAlloc(node, msg, sizeof(SwitchOutQueueInfo));

    info = (SwitchOutQueueInfo *) MESSAGE_ReturnInfo(msg);

    // Store necessary information that will be required while dequeueing
    // from Output queue

    MAC_CopyMac802Address(&info->sourceAddr, &sourceAddr);
    MAC_CopyMac802Address(&info->destAddr, &destAddr);
    info->vlanId = vlanId;
    info->userPriority = userPriority;

    // Convert user priority to queue priority
    Switch_ConvertUserPriorityToQueuePriority(
        port->outPortPriorityToQueueMap,
        userPriority,
        &queuePriority);

    // Call Scheduler insertion funtion
    (*scheduler).insert(msg,
                        &queueIsFull,
                        queuePriority,
                        NULL, //infoField,
                        node->getNodeTime());

    *isEnqueued = !queueIsFull;

    if (Switch_QueueNumInQueue(scheduler, ALL_PRIORITIES) == 1)
    {
        // If queue was previously empty then trigger mac to
        // dequeue a packet from queue

        MAC_SwitchHasPacketToSend(
            node,
            port->macData->interfaceIndex);
    }
}


// NAME:        Switch_InQueueDequeuePacket
//
// PURPOSE:     Dequeue a packet from Input queue.
//
// PARAMETERS:  Scheduler pointer
//              Message pointer to assign message.
//              Pointer to collect source of the packet.
//              Pointer to collect destination of the packet.
//              Pointer to collect vlan id of the packet.
//              Pointer to collect user priority of the packet.
//              Pointer to collect incoming port
//
// RETURN:      Packet, is present in queue.
//              Null, otherwise.

static
void Switch_InQueueDequeuePacket(
    Node* node,
    Scheduler* scheduler,
    Message** msg,
    Mac802Address* sourceAddr,
    Mac802Address* destAddr,
    VlanId* vlanId,
    TosType* userPriority,
    int* outgoingInterfaceIndex)
{
    SwitchInQueueInfo* info = NULL;
    QueuePriorityType queuePriority = ALL_PRIORITIES;

    // Call scheduler message retrieve function
    (*scheduler).retrieve(ALL_PRIORITIES,
        DEQUEUE_NEXT_PACKET,
        msg,
        &queuePriority,
        DEQUEUE_PACKET,
        node->getNodeTime());

     info = (SwitchInQueueInfo *) MESSAGE_ReturnInfo((*msg));

     // Retrieve information about source, destination, user priority
     // and incoming port from the dequeued message

     MAC_CopyMac802Address(sourceAddr, &info->sourceAddr);
     MAC_CopyMac802Address(destAddr, &info->destAddr);
     *vlanId = info->vlanId;
     *userPriority = info->userPriority;
     *outgoingInterfaceIndex = info->outgoingInterfaceIndex;
}


// NAME:        Switch_OutQueueDequeuePacket
//
// PURPOSE:     Dequeue a packet from  queue.
//
// PARAMETERS:  Scheduler pointer
//              Message pointer to assign message.
//              Pointer to collect source of the packet.
//              Pointer to collect destination of the packet.
//              Pointer to collect vlan id of the packet.
//              Pointer to collect queue priority of the packet.
//
// RETURN:      Packet, is present in queue.
//              Null, otherwise.

static
void Switch_OutQueueDequeuePacket(
    Node* node,
    Scheduler* scheduler,
    Message** msg,
    Mac802Address* sourceAddr,
    Mac802Address* destAddr,
    VlanId* vlanId,
    TosType* userPriority)
{
    SwitchOutQueueInfo* info = NULL;
    QueuePriorityType queuePriority;

    // Call scheuler message retrieve funtion
    (*scheduler).retrieve(ALL_PRIORITIES,
        DEQUEUE_NEXT_PACKET,
        msg,
        &queuePriority,
        DEQUEUE_PACKET,
        node->getNodeTime());

     info = (SwitchOutQueueInfo*) MESSAGE_ReturnInfo((*msg));

     // Retrieve information about source, destination and vlanId
     // from the dequeued packet.
     MAC_CopyMac802Address(sourceAddr, &info->sourceAddr);
     MAC_CopyMac802Address(destAddr, &info->destAddr);
     *vlanId = info->vlanId;
     *userPriority = info->userPriority;
}


// NAME:        Switch_InQueueTopPacket
//
// PURPOSE:     To see the content of the top packet in the scheudler
//
// PARAMETERS:  Scheduler pointer
//              Message pointer to assign message.
//              Pointer to collect queue priority of the packet.
//
// RETURN:      Packet, is present in queue.
//              Null, otherwise.

static
void Switch_InQueueTopPacket(
    Node* node,
    Scheduler* scheduler,
    Message** msg,
    QueuePriorityType* priority)
{
    // Call scheduler retrieve function with dequeue flag FALSE
    (*scheduler).retrieve(ALL_PRIORITIES,
        DEQUEUE_NEXT_PACKET,
        msg,
        priority,
        PEEK_AT_NEXT_PACKET,
        node->getNodeTime());
}


// NAME:        Switch_QueueDropPacket
//
// PURPOSE:     Drop a packet from queue.
//
// PARAMETERS:  Scheduler pointer.
//              Message pointer to collect the packet.
//
// RETURN:      Dropped packet.

static
void Switch_QueueDropPacket(
         Node* node,
         Scheduler* scheduler,
         Message** msg)
{
    QueuePriorityType queuePriority = ALL_PRIORITIES;

    (*scheduler).retrieve(ALL_PRIORITIES,
                        DEQUEUE_NEXT_PACKET,
                        msg,
                        &queuePriority,
                        DISCARD_PACKET,
                        node->getNodeTime());
}


// NAME:        Switch_ClearMessagesFromQueue
//
// PURPOSE:     Clear messages present in queue.
//
// PARAMETERS:  Scheduler pointer
//
// RETURN:      None.

void Switch_ClearMessagesFromQueue(
    Node* node,
    Scheduler* scheduler)
{
    Message* msg = NULL;
    SwitchInQueueInfo* info = NULL;

    while (!Switch_QueueIsEmpty(scheduler))
    {
        // Until the queue is empty, delete messages one by one
        Switch_QueueDropPacket(
            node,
            scheduler,
            &msg);

        if (msg)
        {
            info = (SwitchInQueueInfo*) MESSAGE_ReturnInfo(msg);
#ifdef ADDON_DB
            if (node->macData[info->outgoingInterfaceIndex])
            {

                HandleMacDBEvents(
                              node,
                              msg,
                              node->macData[info->outgoingInterfaceIndex]->
                                                                  phyNumber,
                              info->outgoingInterfaceIndex,
                              MAC_Drop,
                              node->macData[info->outgoingInterfaceIndex]->
                                                                macProtocol,
                              TRUE,
                              "Unable to forward data packet",
                              FALSE);
            }
#endif
            MESSAGE_Free(node, msg);
        }
    }
}


// NAME:        Switch_QueueIsEmptyForInterface
//
// PURPOSE:     Check whether queue is empty or not for an interface.
//
// PARAMETERS:  Interface index whose queue is checking.
//
// RETURN:      TRUE, if queue empty
//              FLASE, otherwise

BOOL Switch_QueueIsEmptyForInterface(
    Node* node,
    int interfaceIndex)
{
    MacSwitch* sw = Switch_GetSwitchData(node);
    SwitchPort* port = Switch_GetPortDataFromInterface(
                           sw,
                           interfaceIndex);

    Scheduler* scheduler = port->outPortScheduler;

    return Switch_QueueIsEmpty(scheduler);
}


// NAME:        Switch_QueueDropPacketForInterface
//
// PURPOSE:     Drop a packet from an interface queue.
//
// PARAMETERS:  Interface index of the queue where packet is to be dropped.
//              Message pointer to collect the packet.
//
// RETURN:      Dropped packet.

void Switch_QueueDropPacketForInterface(
    Node* node,
    int interfaceIndex,
    Message** msg)
{
    MacSwitch* sw = Switch_GetSwitchData(node);
    SwitchPort* port = Switch_GetPortDataFromInterface(
                           sw,
                           interfaceIndex);

    Scheduler* scheduler = port->outPortScheduler;

    // Call queue drop function
    Switch_QueueDropPacket(
        node,
        scheduler,
        msg);
}


//--------------------------------------------------------------------------
// Filter database functions


// NAME:        SwitchDb_InitStats
//
// PURPOSE:     Initialise stat variables for filter database
//
// PARAMETERS:  Pointer to filtering database
//
// RETURN:      None

static
void SwitchDb_InitStats(
    SwitchDb* flDb)
{
    memset(&flDb->stats, 0, sizeof(SwitchDbStats));
}


// NAME:        SwitchDb_Init
//
// PURPOSE:     Initialize the variables of filter database
//
// PARAMETERS:  Pointer to filtering database
//              Max number of entries kept by this database.
//
// RETURN:      None

static
void SwitchDb_Init(
    SwitchDb* flDb,
    unsigned int maxEntries)
{
   flDb->flDbEntry = NULL;
   flDb->numEntries = 0;
   flDb->maxEntries = maxEntries;
   flDb->lastAccessTime = 0;
   SwitchDb_InitStats(flDb);
}


// NAME:        SwitchDb_GetLRUEntry
//
// PURPOSE:     Find LRU entry in the database
//              The least recently used entry is the oldest
//              one, and would the first to be aged.
//
// PARAMETERS:  Pointer to filtering database
//
// RETURN:      Informations of LRU entry

static
SwitchDbEntry* SwitchDb_GetLRUEntry(
    SwitchDb* flDb)
{
    // Search the LRU entry and delete it.
    SwitchDbEntry* LRUEntry = NULL;
    SwitchDbEntry* current = flDb->flDbEntry;

    while (current)
    {
        if (!LRUEntry || LRUEntry->lastAccessTime >
            current->lastAccessTime)
        {
            LRUEntry = current;
        }
        current = current->nextEntry;
    }

    return LRUEntry;
}


// NAME:        SwitchDb_DeleteEntry
//
// PURPOSE:     Delete a entry from database
//
// PARAMETERS:  Pointer to filtering database
//              Previous to the entry to delete
//              Entry to delete
//              Current time.
//
// RETURN:      None

static
void SwitchDb_DeleteEntry(
    SwitchDb* flDb,
    SwitchDbEntry* prev,
    SwitchDbEntry* toDelete,
    clocktype currentTime)
{
    flDb->numEntries--;
    if (!prev)
    {
        // First entry in the database is the LRU entry
        flDb->flDbEntry = toDelete->nextEntry;
    }
    else
    {
        prev->nextEntry = toDelete->nextEntry;
    }

    if (toDelete->lastAccessTime == flDb->lastAccessTime)
    {
        SwitchDbEntry* LRUEntry = SwitchDb_GetLRUEntry(flDb);

        if (LRUEntry)
        {
            flDb->lastAccessTime = LRUEntry->lastAccessTime;
        }
        else
        {
            flDb->lastAccessTime = currentTime;
        }
    }

    MEM_free(toDelete);
}


// NAME:        SwitchDb_DeleteLRUEntry
//
// PURPOSE:     Delete LRU entry from the database
//
// PARAMETERS:  Pointer to filtering database
//              Current time
//
// RETURN:      None

static
void SwitchDb_DeleteLRUEntry(
    SwitchDb* flDb,
    clocktype currentTime)
{
    // search the LRU entry and delete it.
    SwitchDbEntry* LRUEntry = NULL;
    SwitchDbEntry* prevToCurrent = NULL;
    SwitchDbEntry* current = flDb->flDbEntry;
    SwitchDbEntry* prevToLRUEntry = NULL;

    while (current)
    {
        if (!LRUEntry
            || LRUEntry->lastAccessTime > current->lastAccessTime)
        {
            LRUEntry = current;
            prevToLRUEntry = prevToCurrent;
        }
        prevToCurrent = current;
        current = current->nextEntry;
    }

    // Squeeze the database
    SwitchDb_DeleteEntry(
        flDb,
        prevToLRUEntry,
        LRUEntry,
        currentTime);
}


// NAME:        SwitchDb_GetCountForPort
//
// PURPOSE:     Counts number of entries for a particular
//              port and type.
//
// PARAMETERS:  Pointer to filtering database
//              Port number
//              Entry type
//
// RETURN:      Number of entries present for this port number
//              and type.

unsigned int SwitchDb_GetCountForPort(
    SwitchDb* flDb,
    unsigned short port,
    SwitchDbEntryType type)
{
    unsigned int count = 0;

    SwitchDbEntry* current = flDb->flDbEntry;

    while (current)
    {
        if (current->port == port && current->type == type)
        {
            count++;
        }
        current = current->nextEntry;
    }

    return count;
}


// NAME:        SwitchDb_EntryAgeOut
//
// PURPOSE:     Check and delete entries which are aged out
//
// PARAMETERS:  Pointer to filtering database
//              Current time.
//              Age time
//
// RETURN:      None

static
void SwitchDb_EntryAgeOut(
    SwitchDb* flDb,
    clocktype currentTime,
    clocktype ageTime)
{
    if (currentTime >= flDb->lastAccessTime + ageTime)
    {
        // Need to check and delete entries which are aged out
        SwitchDbEntry* current = flDb->flDbEntry;
        SwitchDbEntry* prev = NULL;
        while (current)
        {
            if (current->lastAccessTime + ageTime <= currentTime)
            {
                SwitchDbEntry* toDelete = current;
                current = current->nextEntry;

                flDb->stats.numEntryEdgedOut++;

                SwitchDb_DeleteEntry(
                    flDb,
                    prev,
                    toDelete,
                    currentTime);
            }
            else
            {
                prev = current;
                current = current->nextEntry;
            }
        }
    }
}


// NAME:        SwitchDb_EntryAgeOutByPort
//
// PURPOSE:     Check and delete which have aged out for
//              a particular port
//
// PARAMETERS:  Pointer to filtering database
//              Port whose entry to be deleted
//              Type of Entry
//              Current time.
//              Age time
//
// RETURN:      None

void SwitchDb_EntryAgeOutByPort(
    SwitchDb* flDb,
    unsigned short portNumber,
    SwitchDbEntryType type,
    clocktype currentTime,
    clocktype ageTime)
{
    if (currentTime >= flDb->lastAccessTime + ageTime)
    {
        // Check and delete entries which are aged out
        SwitchDbEntry* current = flDb->flDbEntry;
        SwitchDbEntry* prev = NULL;

        while (current)
        {
            if (current->port == portNumber
                && current->type == type
                && (current->lastAccessTime + ageTime <= currentTime))
            {
                SwitchDbEntry* toDelete = current;
                current = current->nextEntry;

                flDb->stats.numEntryEdgedOut++;

                SwitchDb_DeleteEntry(
                    flDb,
                    prev,
                    toDelete,
                    currentTime);
            }
            else
            {
                prev = current;
                current = current->nextEntry;
            }
        }
    }
}


// NAME:        SwitchDb_DeleteAllEntries
//
// PURPOSE:     Delete all entries from the database
//
// PARAMETERS:  Pointer to filtering database
//              Current time
//
// RETURN:      None

static
void SwitchDb_DeleteAllEntries(
    SwitchDb* flDb,
    clocktype currentTime)
{
    SwitchDbEntry* current = flDb->flDbEntry;
    SwitchDbEntry* temp = NULL;

    while (current)
    {
        flDb->stats.numEntryDeleted++;

        temp = current->nextEntry;
        MEM_free(current);
        current = temp;
    }
    flDb->flDbEntry = NULL;
    flDb->numEntries = 0;
    flDb->lastAccessTime = currentTime;
}


// NAME:        SwitchDb_DeleteEntriesByPortAndType
//
// PURPOSE:     Flush all entries for a particular port and type.
//
// PARAMETERS:  Pointer to filtering database
//              Port whose entry to be deleted
//              Type of Entry
//              Current time.
//
// RETURN:      None

void SwitchDb_DeleteEntriesByPortAndType(
    SwitchDb* flDb,
    unsigned short port,
    SwitchDbEntryType type,
    clocktype currentTime)
{
    SwitchDbEntry* current = flDb->flDbEntry;
    SwitchDbEntry* prevToCurrent = NULL;
    SwitchDbEntry* toDelete = NULL;

    while (current)
    {
        if (current->port == port && current->type == type)
        {
            toDelete = current;
            current = current->nextEntry;

            flDb->stats.numEntryDeleted++;

            // Squeeze the database
            SwitchDb_DeleteEntry(
                flDb,
                prevToCurrent,
                toDelete,
                currentTime);
        }
        else
        {
            prevToCurrent = current;
            current = current->nextEntry;
        }
    }
}


// NAME:        SwitchDb_GetEntry
//
// PURPOSE:     Search the database for a particular entry
//
// PARAMETERS:  Pointer to filtering database
//              Filter database id
//              Address of the entry looking for
//              Current time
//
// RETURN:      Entry, if address matched
//              Null, if address not matched

static
SwitchDbEntry* SwitchDb_GetEntry(
    SwitchDb* flDb,
    unsigned short fid,
    Mac802Address srcAddr,
    clocktype currentTime)
{
    SwitchDbEntry* current = flDb->flDbEntry;

    flDb->stats.numTimesSearched++;

    while (current &&
            !MAC_IsIdenticalMac802Address(&current->srcAddr, &srcAddr))
    {
        current = current->nextEntry;
    }

    if (current
        && current->fid == fid
        && MAC_IsIdenticalMac802Address(&current->srcAddr, &srcAddr))
    {
        flDb->stats.numEntryFound++;
        current->lastAccessTime = currentTime;
        return current;
    }
    else
    {
        return NULL;
    }
}


// NAME:        SwitchDb_InsertEntry
//
// PURPOSE:     Insert a entry in the database
//
// PARAMETERS:  Pointer to filtering database
//              Source address of the entry
//              Port whose entry to be inserted
//              Filter database id
//              Vlan ID of source address
//              Type of Entry
//              Current time
//              Flag to indicate if entry is inserted.
//
// RETURN:      Entry, with flag as TRUE, for a new entry
//              Entry, with flag as FALSE, for an existing entry

static
SwitchDbEntry* SwitchDb_InsertEntry(
    SwitchDb* flDb,
    Mac802Address srcAddr,
    unsigned short port,
    unsigned short fid,
    VlanId vlanId,
    SwitchDbEntryType type,
    clocktype currentTime,
    BOOL* isInserted)
{
    SwitchDbEntry* current = flDb->flDbEntry;
    SwitchDbEntry* prev = NULL;

    *isInserted = FALSE;

    while (current)
    {

        // The entry already exists, update information.
        if (MAC_IsIdenticalMac802Address(&current->srcAddr, &srcAddr))
        {
            current->port = port;
            current->lastAccessTime = currentTime;
            return current;
        }
        else
        {
            prev = current;
            current = current->nextEntry;
        }
    }

        // At this point we are going to add a new entry in the table
        SwitchDbEntry* newEntry =
            (SwitchDbEntry*) MEM_malloc(sizeof(SwitchDbEntry));

        // Assign different fields of the new entry
        MAC_CopyMac802Address(&newEntry->srcAddr, &srcAddr);
        newEntry->port = port;
        newEntry->fid = fid;
        newEntry->vlanId = vlanId;
        newEntry->type = type,
        newEntry->lastAccessTime = currentTime;
        newEntry->nextEntry = NULL;


        // increment the size of the Database
        flDb->numEntries++;

        // Update stats for number of entry inserted
        flDb->stats.numEntryInserted++;

        // The entry is to be inserted.
        // So set the variable isInserted to true
        *isInserted = TRUE;

        // Link the new entry in the Database
        if (!prev)
        {
            // Earlier Database was empty or the new entry
            // is to be added in front of the list
            flDb->flDbEntry = newEntry;
            if (!current)
            {
                // Database was empty
                flDb->lastAccessTime = currentTime;
            }
        }
        else
        {
            // prev is pointing to the previous Entry
            prev->nextEntry = newEntry;
        }

        if (flDb->numEntries > flDb->maxEntries)
        {
            // Number of entries is greater than maximum possible entry
            // Delete the entry which was least recently used

            // Update stats for number of LRU entry deleted
            flDb->stats.numLRUEntryDeleted++;

            SwitchDb_DeleteLRUEntry(flDb, currentTime);
        }
        return newEntry;
    //}
}


// NAME:        SwitchDb_PrintCurrentEntries
//
// PURPOSE:     Print current entries of the filter database
//
// PARAMETERS:  This node
//              Pointer to switch data
//
// RETURN:      None

static
void SwitchDb_PrintCurrentEntries(
    Node* node,
    MacSwitch* sw)
{
    SwitchDbEntry* entry = sw->db->flDbEntry;
    char lastTime[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(node->getNodeTime(), lastTime);
    printf("\nFilter database at switch %d on %s\n\n",
        sw->swNumber, lastTime);

    while (entry)
    {
        TIME_PrintClockInSecond(entry->lastAccessTime, lastTime);
        MAC_PrintMacAddr(&entry->srcAddr);
        printf("  [ :: %u :: %u :: %u :: %u :: %s ]\n",
            entry->port, entry->fid,
            entry->vlanId, entry->type, lastTime);

        entry = entry->nextEntry;
    }
}


// NAME:        SwitchDb_PrintStats
//
// PURPOSE:     Print statistics for the database
//
// PARAMETERS:  This node
//              Pointer to switch data
//
// RETURN:      None

static
void SwitchDb_PrintStats(
    Node* node,
    MacSwitch* sw)
{
    SwitchDb* flDb = sw->db;

    char buf[MAX_STRING_LENGTH];

    sprintf(buf,
        "Number of entry inserted = %u",
        flDb->stats.numEntryInserted);

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Database",
        ANY_DEST,
        ANY_INSTANCE,
        buf);

    sprintf(buf,
        "Number of LRU entry deleted = %u",
        flDb->stats.numLRUEntryDeleted);

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Database",
        ANY_DEST,
        ANY_INSTANCE,
        buf);

    sprintf(buf,
        "Number of entry aged out = %u",
        flDb->stats.numEntryEdgedOut);

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Database",
        ANY_DEST,
        ANY_INSTANCE,
        buf);

    sprintf(buf,
        "Number of entry flushed = %u",
        flDb->stats.numEntryDeleted);

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Database",
        ANY_DEST,
        ANY_INSTANCE,
        buf);

    if (flDb->stats.numTimesSearched)
    {
        sprintf(buf,
            "Hit ratio = %f",
            ((double) flDb->stats.numEntryFound)
            / flDb->stats.numTimesSearched);
    }
    else
    {
        sprintf(buf,
            "Hit ratio = %f", 0.0);
    }

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Database",
        ANY_DEST,
        ANY_INSTANCE,
        buf);
}


//--------------------------------------------------------------------------
// Receive / Transmit / Relay functions


// NAME:        Switch_UseBackplaneIfPossible
//
// PURPOSE:     If backplane Idle trigger switch to dequeue
//              a packet from Input queue
//
// PARAMETERS:  Switch data
//              Scheduler pointer
//              Incoming interface
//
// RETURN:      NONE

static
void Switch_UseBackplaneIfPossible(
    Node* node,
    int incomingInterfaceIndex)
{
    SwitchBackplaneStatusType* backplaneStatus = NULL;

    MacSwitch* sw = Switch_GetSwitchData(node);

    Scheduler* incomingPortScheduler = NULL;

    Message* queuedMsg = NULL;
    QueuePriorityType priority = 0;

    SwitchInQueueInfo* info= NULL;

    if (incomingInterfaceIndex == CPU_INTERFACE)
    {
        incomingPortScheduler = sw->cpuScheduler;
        backplaneStatus = &sw->backplaneStatus;
    }
    else
    {
        SwitchPort* incomingPort = Switch_GetPortDataFromInterface(
                                       sw,
                                       incomingInterfaceIndex);
        incomingPortScheduler = incomingPort->inPortScheduler;
        backplaneStatus = &incomingPort->backplaneStatus;
    }

    if (Switch_QueueIsEmpty(incomingPortScheduler))
    {
        // If scheduler is queue is empty then return
        return;
    }


    // Process request if backplne is idle
    if (*backplaneStatus == SWITCH_BACKPLANE_IDLE)
    {
        Message* backplaneMsg = NULL;
        clocktype backplaneDelay = 0;
        int packetSize = 0;
        SwitchBackplaneInfo* backplaneInfo = NULL;

        // See the top packet in the Input queue
        Switch_InQueueTopPacket(
            node,
            incomingPortScheduler,
            &queuedMsg,
            &priority);

        info = (SwitchInQueueInfo*) MESSAGE_ReturnInfo(queuedMsg);


        // Allocate and send backplane message
        packetSize = MESSAGE_ReturnPacketSize(queuedMsg);
        backplaneDelay = (clocktype) (packetSize * 8 * SECOND
            / sw->swBackplaneThroughputCapacity);

        backplaneMsg = MESSAGE_Alloc(
            node,
            MAC_LAYER,
            MAC_SWITCH,
            MSG_MAC_FromBackplane);

        MESSAGE_InfoAlloc(
            node,
            backplaneMsg,
            sizeof(SwitchBackplaneInfo));

        backplaneInfo = (SwitchBackplaneInfo *)
                         MESSAGE_ReturnInfo(backplaneMsg);

        // Send incoming port pointer and information if the triggering
        // is for input queue or cpu queue with backplane information
        backplaneInfo->incomingInterfaceIndex = incomingInterfaceIndex;

        *backplaneStatus = SWITCH_BACKPLANE_BUSY;

        MESSAGE_Send(node, backplaneMsg, backplaneDelay);
    }
}


// NAME:        Switch_ReceivePacketFromBackplane
//
// PURPOSE:     Dequeue a packet from Input queue or Cpu queue
//
// PARAMETERS:  Switch data
//              Receive packet from backplane
//
// RETURN:      NONE

static
void Switch_ReceivePacketFromBackplane(
    Node* node,
    MacSwitch* sw,
    Message* msg)
{
    SwitchBackplaneInfo* backplaneInfo =
        (SwitchBackplaneInfo *) MESSAGE_ReturnInfo(msg);

    Mac802Address macdestAddr ;
    Mac802Address macsrcAddr ;
    TosType userPriority = 0;
    VlanId vlanId = SWITCH_VLAN_ID_NULL;

    SwitchPort* incomingPort = NULL;
    SwitchPort* outgoingPort = NULL;

    Message* queuedMsg = NULL;
    SwitchBackplaneStatusType* backplaneStatus = NULL;
    Scheduler* scheduler = NULL;

    int outgoingInterfaceIndex = 0;
    // Get backplane status and scheduler pointer
    if (backplaneInfo->incomingInterfaceIndex == CPU_INTERFACE)
    {
        backplaneStatus = &sw->backplaneStatus;
        incomingPort = NULL;
        scheduler = sw->cpuScheduler;
    }
    else
    {
        incomingPort = Switch_GetPortDataFromInterface(
                           sw,
                           backplaneInfo->incomingInterfaceIndex);

        backplaneStatus = &incomingPort->backplaneStatus;
        scheduler = incomingPort->inPortScheduler;
    }

    if (!Switch_QueueIsEmpty(scheduler))
    {
        Switch_InQueueDequeuePacket(
            node,
            scheduler,
            &queuedMsg,
            &macsrcAddr,
            &macdestAddr,
            &vlanId,
            &userPriority,
            &outgoingInterfaceIndex);

        if (outgoingInterfaceIndex == CPU_INTERFACE)
        {
            if (!MAC_InterfaceIsEnabled(
                     node, backplaneInfo->incomingInterfaceIndex))
            {
#ifdef ADDON_DB
                HandleMacDBEvents(
                  node,
                  msg,
                  node->macData[backplaneInfo->incomingInterfaceIndex]->
                                                                  phyNumber,
                  backplaneInfo->incomingInterfaceIndex,
                  MAC_Drop,
                  node->macData[backplaneInfo->incomingInterfaceIndex]->
                                                                macProtocol,
                  TRUE,
                  "Interface Disabled",
                  FALSE);
#endif
                sw->packetDroppedAtBackplane++;
                MESSAGE_Free(node, queuedMsg);
            }
            else if (Switch_IsStpGroupAddr(macdestAddr,
                                           sw->Switch_stp_address))
            {
                // If it is switch group address send packet to Stp
                SwitchStp_ReceiveBpdu(
                    node,
                    sw,
                    incomingPort,
                    queuedMsg);
            }
            else if (MAC_IsIdenticalMac802Address(&macdestAddr,
                                                  &incomingPort->portAddr))
            {
                if (Switch_PortState(incomingPort) ==
                    STP_PORT_S_DISCARDING)
                {
#ifdef ADDON_DB
                    HandleMacDBEvents(
                      node,
                      msg,
                      node->macData[backplaneInfo->incomingInterfaceIndex]->
                                                                  phyNumber,
                      backplaneInfo->incomingInterfaceIndex,
                      MAC_Drop,
                      node->macData[backplaneInfo->incomingInterfaceIndex]->
                                                                macProtocol,
                      TRUE,
                      "Port state is Discarding",
                      FALSE);
#endif
                    sw->packetDroppedAtBackplane++;
                    MESSAGE_Free(node, queuedMsg);
                }
                else
                {
                    // If own unicast packet, send to upper layer
                    MacHWAddress hwsrcAddr;
                    Convert802AddressToVariableHWAddress(node, &hwsrcAddr,
                                                                &macsrcAddr);
                    MAC_HandOffSuccessfullyReceivedPacket(
                        node,
                        incomingPort->macData->interfaceIndex,
                        queuedMsg,
                        &hwsrcAddr);
                }
            }
        }
        else
        {
            BOOL isEnqueued = FALSE;

            outgoingPort = Switch_GetPortDataFromInterface(
                               sw,
                               outgoingInterfaceIndex);

            if (backplaneInfo->incomingInterfaceIndex != CPU_INTERFACE
                && Switch_PortState(outgoingPort) ==
                    STP_PORT_S_DISCARDING)
            {
#ifdef ADDON_DB
                HandleMacDBEvents(
                  node,
                  msg,
                  node->macData[backplaneInfo->incomingInterfaceIndex]->
                                                                  phyNumber,
                  backplaneInfo->incomingInterfaceIndex,
                  MAC_Drop,
                  node->macData[backplaneInfo->incomingInterfaceIndex]->
                                                                macProtocol,
                  TRUE,
                  "Port state is Discarding",
                  FALSE);
#endif
                sw->packetDroppedAtBackplane++;
                MESSAGE_Free(node, queuedMsg);
            }
            else
            {
                // Enqueue the packet in output queue
                Switch_OutEnqueueFrameWithPriorityMapping(
                    node,
                    outgoingPort,
                    queuedMsg,
                    macdestAddr,
                    macsrcAddr,
                    vlanId,
                    userPriority,
                    &isEnqueued);

                if (!isEnqueued)
                {
#ifdef ADDON_DB
                    HandleMacDBEvents(
                      node,
                      msg,
                      node->macData[backplaneInfo->incomingInterfaceIndex]->
                                                                  phyNumber,
                      backplaneInfo->incomingInterfaceIndex,
                      MAC_Drop,
                      node->macData[backplaneInfo->incomingInterfaceIndex]->
                                                                macProtocol,
                      TRUE,
                      "Unable to queue the packet",
                      FALSE);
#endif
                    sw->packetDroppedAtBackplane++;
                    MESSAGE_Free(node, queuedMsg);
                }
            }
        }
    }

    ERROR_Assert(*backplaneStatus == SWITCH_BACKPLANE_BUSY,
        "Switch_ReceivePacketFromBackplane: "
        "Backplane dequeue when backplane is idle.\n");

    *backplaneStatus = SWITCH_BACKPLANE_IDLE;

    // Try to send the packet through backplane
    Switch_UseBackplaneIfPossible(
        node,
        backplaneInfo->incomingInterfaceIndex);
}


// NAME:        Switch_SendPacketOnBackplane
//
// PURPOSE:     If backplane is enable then, send then enqueue packet in
//              input or cpu queue, and try to send the packet through
//              backplane
//
// PARAMETERS:  Incoming port interface index
//              Scheduler pointer
//              Message to enqueue
//              Destination address
//              Source address
//              User priority
//
// RETURN:      NONE

static
void Switch_SendPacketOnBackplane(
    Node* node,
    MacSwitch* sw,
    int incomingInterfaceIndex,
    int outgoingInterfaceIndex,
    Message* msg,
    Mac802Address destAddr,
    Mac802Address sourceAddr,
    VlanId vlanId,
    TosType priority)
{
    BOOL isEnqueued = FALSE;

    Scheduler* scheduler = NULL;
    QueuePriorityType* priorityMap = NULL;

    if (incomingInterfaceIndex == CPU_INTERFACE)
    {
        scheduler = sw->cpuScheduler;
        priorityMap = sw->cpuSchedulerPriorityToQueueMap;
    }
    else
    {
        SwitchPort* incomingPort = Switch_GetPortDataFromInterface(sw,
                                       incomingInterfaceIndex);
        scheduler = incomingPort->inPortScheduler;
        priorityMap = incomingPort->inPortPriorityToQueueMap;
    }

    // Enqueue the packet in Cpu or Input queue
    Switch_InEnqueueFrameWithPriorityMapping(
        node,
        scheduler,
        priorityMap,
        msg,
        destAddr,
        sourceAddr,
        vlanId,
        priority,
        outgoingInterfaceIndex,
        &isEnqueued);

    if (!isEnqueued)
    {
#ifdef ADDON_DB
        HandleMacDBEvents(node,
                          msg,
                          node->macData[incomingInterfaceIndex]->phyNumber,
                          incomingInterfaceIndex,
                          MAC_Drop,
                          node->macData[incomingInterfaceIndex]->macProtocol,
                          TRUE,
                          "Unable to queue the packet",
                          FALSE);
#endif
        sw->packetDroppedAtBackplane++;
        MESSAGE_Free(node, msg);
    }
    else
    {
        // Try to use backplane if possible
        Switch_UseBackplaneIfPossible(
            node,
            incomingInterfaceIndex);
    }
}


// NAME:        Switch_HandleReceivedFrameForSwitch
//
// PURPOSE:     Send different bpdu frames received by
//              the switch to proper place.
//
// PARAMETERS:  Incoming port interface index
//              Scheduler pointer
//              Message to enqueue
//              Source address
//              Destination address
//              Frame vlan id
//              User priority
//
// RETURN:      NONE

static
void Switch_HandleReceivedFrameForSwitch(
    Node* node,
    Message* msg,
    Mac802Address sourceAddr,
    Mac802Address destAddr,
    int incomingInterfaceIndex,
    VlanId vlanId,
    TosType priority)
{
    MacSwitch* sw = Switch_GetSwitchData(node);
    SwitchPort* incomingSwPort =
        Switch_GetPortDataFromInterface(sw, incomingInterfaceIndex);

    // This a packet for switch
    if (sw->swBackplaneThroughputCapacity !=
            SWITCH_UNLIMITED_BACKPLANE_THROUGHPUT)
    {
        // If backplane is enabled, send the packet through backplane
        Switch_SendPacketOnBackplane(
            node,
            sw,
            incomingInterfaceIndex,
            CPU_INTERFACE,
            msg,
            destAddr,
            sourceAddr,
            vlanId,
            priority);
    }
    else
    {
        // Send the packet to STP
        SwitchStp_ReceiveBpdu(node, sw, incomingSwPort, msg);
    }
}



// NAME:        Switch_SendFrameToMac
//
// PURPOSE:     Arrange to queue up frames either originated by the
//              switch or forwarded by the switch.
//
// PARAMETERS:  Message to enqueue
//              Incoming interface index
//              Outgoing interface index
//              Destination address
//              Source address
//              Frame vlan id
//              User priority
//
// RETURN:      TRUE, if frame enqueued
//              FALSE, otherwise

void Switch_SendFrameToMac(
    Node* node,
    Message* msg,
    int incomingInterfaceIndex,
    int outgoingInterfaceIndex,
    Mac802Address destAddr,
    Mac802Address sourceAddr,
    VlanId vlanId,
    TosType priority,
    BOOL* isEnqueued)
{
    MacSwitch* sw = Switch_GetSwitchData(node);
    SwitchPort* outgoingPort = Switch_GetPortDataFromInterface(
                                   sw, outgoingInterfaceIndex);

    if (sw->swBackplaneThroughputCapacity !=
        SWITCH_UNLIMITED_BACKPLANE_THROUGHPUT)
    {
        // If backplane is enable then try to send
        // the packet through backplane
        Switch_SendPacketOnBackplane(
            node,
            sw,
            incomingInterfaceIndex,
            outgoingInterfaceIndex,
            msg,
            destAddr,
            sourceAddr,
            vlanId,
            priority);

         *isEnqueued = TRUE;
    }
    else
    {
        // Send the outgoing packet
        Switch_OutEnqueueFrameWithPriorityMapping(
            node,
            outgoingPort,
            msg,
            destAddr,
            sourceAddr,
            vlanId,
            priority,
            isEnqueued);
    }
}


// NAME:        Switch_IsMsgFilteredByIngressRule
//
// PURPOSE:     Check if incoming frame should be filtered by ingress rule.
//
// PARAMETERS:  Message to check
//              Incoming port
//              Incoming frame vlan id
//              Pointer to collect frma vlan classification
//
// RETURN:      TRUE, if frame filtered by ingress rule
//              FALSE, otherwise

static
BOOL Switch_IsMsgFilteredByIngressRule(
    Node* node,
    Message* msg,
    SwitchPort* incomingPort,
    unsigned short frameVlanId,
    unsigned short* frameVlanClassification)
{
    MacSwitch* sw = Switch_GetSwitchData(node);

    if (incomingPort->acceptFrameType == SWITCH_TAGGED_FRAMES_ONLY
        && frameVlanId == SWITCH_VLAN_ID_NULL)
    {
        // Discard frame
        return TRUE;
    }
    else if (frameVlanId == SWITCH_VLAN_ID_RESERVED)
    {
        // Discard frame
        return TRUE;
    }

    // Find proper vlan classifier
    if (frameVlanId > SWITCH_VLAN_ID_NULL)
    {
        *frameVlanClassification = frameVlanId;
    }
    else
    {
        *frameVlanClassification = incomingPort->pVid;
    }

    // Check if frame discarded by ingress filtering
    if (incomingPort->enableIngressFiltering)
    {
        if (!SwitchVlan_IsPortInMemberSet(
                sw, incomingPort->portNumber, *frameVlanClassification))
        {
            return TRUE;
        }
    }

    // i.e. Either vlan id present in member set or
    // frame is not discarded by ingress filtering
    return FALSE;
}


// NAME:        Switch_HandOffSuccessfullyReceivedPacket
//
// PURPOSE:     Deliver frame, addressed to this switch, to upper layer.
//
// PARAMETERS:  Node data
//              Incoming interface index
//              Message to deliver
//              Source address
//              Destination address
//              Frame vlan id
//              User priority
//
// RETURN:      NONE

static
void Switch_HandOffSuccessfullyReceivedPacket(
    Node* node,
    int incomingInterfaceIndex,
    Message* msg,
    Mac802Address sourceAddr,
    Mac802Address destAddr,
    VlanId vlanId,
    TosType priority)
{
    MacSwitch* sw = Switch_GetSwitchData(node);

    if (sw->swBackplaneThroughputCapacity !=
        SWITCH_UNLIMITED_BACKPLANE_THROUGHPUT)
    {
        // Backplane is enabled so try to send the packet through
        // backplane
        Switch_SendPacketOnBackplane(
            node,
            sw,
            incomingInterfaceIndex,
            CPU_INTERFACE,
            msg,
            destAddr,
            sourceAddr,
            vlanId,
            priority);
    }
    else
    {
        // Handoff the packet to upper layer
       MacHWAddress hwsrcAddr;
       Convert802AddressToVariableHWAddress(node, &hwsrcAddr, &sourceAddr);
       MAC_HandOffSuccessfullyReceivedPacket(
            node,
            incomingInterfaceIndex,
            msg,
            &hwsrcAddr);
    }
}


// NAME:        Switch_QueueFrameBasedOnMemberSet
//
// PURPOSE:     Enqueue frame to all those port present in member set.
//
// PARAMETERS:  Node data
//              Switch data
//              Incoming interface index
//              Message to enqueue
//              Source address
//              Destination address
//              Frame vlan id
//              User priority
//
// RETURN:      TRUE, if frame enqueued at least in one port
//              FALSE, otherwise

static
void Switch_QueueFrameBasedOnMemberSet(
    Node* node,
    MacSwitch* sw,
    int incomingInterfaceIndex,
    Message* msg,
    Mac802Address sourceAddr,
    Mac802Address destAddr,
    VlanId vlanClassification,
    TosType priority,
    BOOL* isEnqueuedOne)
{
    SwitchPort* outgoingPort = NULL;
    SwitchPort* incomingSwPort =
        Switch_GetPortDataFromInterface(sw, incomingInterfaceIndex);

    outgoingPort = sw->firstPort;
    while (outgoingPort)
    {
        // Forward frame to each port present in member set and in
        // forwarding state, except the port from where this frame
        // is received by the switch.
        //
        if (SwitchVlan_IsPortInMemberSet(sw, outgoingPort->portNumber,
                vlanClassification)
            && outgoingPort->portNumber != incomingSwPort->portNumber
            && Switch_PortState(outgoingPort) == STP_PORT_S_FORWARDING)
        {
            // Enqueue the packet in port queue
            BOOL isEnqueued = FALSE;
            Message* duplicateMsg = MESSAGE_Duplicate(node, msg);

            Switch_SendFrameToMac(
                node,
                duplicateMsg,
                incomingInterfaceIndex,
                outgoingPort->macData->interfaceIndex,
                destAddr,
                sourceAddr,
                vlanClassification,
                priority,
                &isEnqueued);

            if (!isEnqueued)
            {
                MESSAGE_Free(node, duplicateMsg);
            }
            else
            {
                *isEnqueuedOne = TRUE;
            }
        }

        outgoingPort = outgoingPort->nextPort;
    }

#ifdef ADDON_DB
    if (!(*isEnqueuedOne))
    {
        HandleMacDBEvents(node,
                          msg,
                          node->macData[incomingInterfaceIndex]->
                                                       phyNumber,
                          incomingInterfaceIndex,
                          MAC_Drop,
                          node->macData[incomingInterfaceIndex]->
                                                     macProtocol,
                          TRUE,
                          "Unable to queue the packet",
                          FALSE);
    }
#endif
    MESSAGE_Free(node, msg);
}


// NAME:        Switch_QueueFrameBasedOnFilterDb
//
// PURPOSE:     Enqueue frame to port indicated by filter database entry.
//
// PARAMETERS:  Node data
//              Switch data
//              Incoming interface index
//              Message to enqueue
//              Source address
//              Destination address
//              Frame vlan id
//              User priority
//
// RETURN:      TRUE, if frame enqueued.
//              FALSE, otherwise

static
void Switch_QueueFrameBasedOnFilterDb(
    Node* node,
    int incomingInterfaceIndex,
    Message* msg,
    SwitchDbEntry* destEntry,
    Mac802Address sourceAddr,
    Mac802Address destAddr,
    VlanId vlanClassification,
    TosType priority,
    BOOL *isEnqueued)
{
    MacSwitch* sw = Switch_GetSwitchData(node);
    unsigned short outgoingPortNumber = destEntry->port;
    SwitchPort* incomingSwPort =
        Switch_GetPortDataFromInterface(sw, incomingInterfaceIndex);

    // The information is there in the Filter Database, so
    // send the packet to destination port

    if (outgoingPortNumber == incomingSwPort->portNumber)
    {
        // outgoing and incoming port are same so
        // drop the packet
        *isEnqueued = FALSE;
    }
    else
    {
        SwitchPort* outgoingPort =
            Switch_GetPortDataFromPortNumber(
                sw, outgoingPortNumber);

        // Enqueue the packet in the outgoing port only if
        // the outgoing port is in forwarding state

        if (Switch_PortState(outgoingPort) == STP_PORT_S_FORWARDING)
        {
            // enqueue the frame in outgoing port

            Switch_SendFrameToMac(
                node,
                msg,
                incomingInterfaceIndex,
                outgoingPort->macData->interfaceIndex,
                destAddr,
                sourceAddr,
                vlanClassification,
                priority,
                isEnqueued);

            // If isEnqueued is returned as false from function
            // Switch_SendFrameToMac then drop the packet.
        }
        else
        {
             // Drop the frame
            *isEnqueued = FALSE;
        }
    }

    if (!(*isEnqueued))
    {
#ifdef ADDON_DB
        HandleMacDBEvents(
                  node,
                  msg,
                  node->macData[incomingInterfaceIndex]->
                                               phyNumber,
                  incomingInterfaceIndex,
                  MAC_Drop,
                  node->macData[incomingInterfaceIndex]->
                                             macProtocol,
                  TRUE,
                  "Unable to queue the packet",
                  FALSE);
#endif
             MESSAGE_Free(node, msg);
        }
    }


// NAME:        Switch_ReceiveFrameFromMac
//
// PURPOSE:     Relay the incoming frame to one or more ports
//              depending on whether it is a broadcast frame,
//              whether the destination address has been learnt,
//              whether the destination port is forwarding,
//              whether the queue permits.
//
// PARAMETERS:  Source address of the frame
//              destination address of the frame
//              Interface index of the incoming port
//              Vlan id of the incoming frame
//              User priority of the frame
//
// RETURN:      None

static
void Switch_ReceiveFrameFromMac(
    Node* node,
    Message* msg,
    Mac802Address sourceAddr,
    Mac802Address destAddr,
    int incomingInterfaceIndex,
    VlanId vlanId,
    TosType priority)
{
    char buf[MAX_STRING_LENGTH];
    MacSwitch* sw = Switch_GetSwitchData(node);
    SwitchPort* incomingSwPort = Switch_GetPortDataFromInterface(
                                     sw,
                                     incomingInterfaceIndex);
    SwitchDbEntry* sourceEntry;

    SwitchPortStat* vlanStats =
        Switch_GetStatVariableForVlan(node, incomingSwPort, vlanId);

    if (!MAC_InterfaceIsEnabled(node, incomingInterfaceIndex))
    {
#ifdef ADDON_DB
        HandleMacDBEvents(node,
                          msg,
                          node->macData[incomingInterfaceIndex]->phyNumber,
                          incomingInterfaceIndex,
                          MAC_Drop,
                          node->macData[incomingInterfaceIndex]->macProtocol,
                          TRUE,
                          "Interface Disabled",
                          FALSE);
#endif
        // If there is an interface fault, drop the packet
        MESSAGE_Free(node, msg);
        return;
    }

    vlanStats->numTotalFrameReceived++;
    incomingSwPort->stats.numTotalFrameReceived++;

    // Protect against TOS other than 0 to 7
    if (priority > SWITCH_FRAME_PRIORITY_MAX)
    {
        priority = SWITCH_FRAME_PRIORITY_DEFAULT;
    }

    if (Switch_IsStpGroupAddr(destAddr,sw->Switch_stp_address))
    {
        Switch_HandleReceivedFrameForSwitch(
            node,
            msg,
            sourceAddr,
            destAddr,
            incomingInterfaceIndex,
            vlanId,
            priority);
        return;
    }

    if (Switch_IsGarpAware(sw) &&
        Switch_IsGarpGroupAddr(destAddr,sw))
    {
        Garp_HandleReceivedFrameFromMac(
            node,
            sw,
            msg,
            sourceAddr,
            destAddr,
            incomingInterfaceIndex,
            vlanId,
            priority);
        return;
    }

    if (Switch_PortState(incomingSwPort) == STP_PORT_S_DISCARDING)
    {
#ifdef ADDON_DB
        HandleMacDBEvents(node,
                          msg,
                          node->macData[incomingInterfaceIndex]->phyNumber,
                          incomingInterfaceIndex,
                          MAC_Drop,
                          node->macData[incomingInterfaceIndex]->macProtocol,
                          TRUE,
                          "Port state is Discarding",
                          FALSE);
#endif
        // The port is disabled so drop any packet
        MESSAGE_Free(node, msg);

        vlanStats->numDroppedInDiscardState++;
        incomingSwPort->stats.numDroppedInDiscardState++;
    }
    else
    {
        BOOL isInserted = FALSE;
        StpPortState incomingPortState = Switch_PortState(incomingSwPort);
        VlanId vlanClassification = 0;

        unsigned short fid;

        // Process frame with ingress filtering
        if (Switch_IsMsgFilteredByIngressRule(
                node,
                msg,
                incomingSwPort,
                vlanId,
                &vlanClassification))
        {
#ifdef ADDON_DB
            HandleMacDBEvents(node,
                              msg,
                              node->macData[incomingInterfaceIndex]->
                                                                  phyNumber,
                              incomingInterfaceIndex,
                              MAC_Drop,
                              node->macData[incomingInterfaceIndex]->
                                                                macProtocol,
                              TRUE,
                              "Dropped by Ingress Filtering",
                              FALSE);
#endif
            MESSAGE_Free(node, msg);

            vlanStats->numDroppedByIngressFiltering++;
            incomingSwPort->stats.numDroppedByIngressFiltering++;
            return;
        }

        // Frame passed ingress filtering. Now get Fid for
        // this vlan id.
        fid = SwitchDb_GetFidForGivenVlan(sw, vlanClassification);

        if (fid == SWITCH_FID_UNKNOWN)
        {
#ifdef ADDON_DB
            HandleMacDBEvents(node,
                              msg,
                              node->macData[incomingInterfaceIndex]->
                                                                  phyNumber,
                              incomingInterfaceIndex,
                              MAC_Drop,
                              node->macData[incomingInterfaceIndex]->
                                                                macProtocol,
                              TRUE,
                              "Frame arrived from unknown Vlan",
                              FALSE);
#endif
            // Frame arrived from unknown Vlan
            MESSAGE_Free(node, msg);
            incomingSwPort->stats.numDroppedDueToUnknownVlan++;
            return;
        }

        // Insert db entry
        sourceEntry = SwitchDb_InsertEntry(
                        sw->db,
                        sourceAddr,
                        incomingSwPort->portNumber,
                        fid,
                        vlanClassification,
                        SWITCH_DB_DYNAMIC,
                        node->getNodeTime(),
                        &isInserted);

        if (sw->stpTrace && isInserted)
        {
            //char addrString[MAX_STRING_LENGTH];
            sprintf(buf, "Fldb: Source [%s] added to db",
                sourceAddr.byte);
            SwitchStp_Trace(node, sw, incomingSwPort, NULL, buf);
        }

        if (incomingPortState == STP_PORT_S_LEARNING)
        {
#ifdef ADDON_DB
            HandleMacDBEvents(node,
                              msg,
                              node->macData[incomingInterfaceIndex]->
                                                                phyNumber,
                              incomingInterfaceIndex,
                              MAC_Drop,
                              node->macData[incomingInterfaceIndex]->
                                                                macProtocol,
                              TRUE,
                              "Port state is learning",
                              FALSE);
#endif
            // Port state is learning so don't need to forward the packet
            MESSAGE_Free(node, msg);

            vlanStats->numDroppedInLearningState++;
            incomingSwPort->stats.numDroppedInLearningState++;
        }
        else if (incomingPortState == STP_PORT_S_FORWARDING)
        {


            if (MAC_IsIdenticalMac802Address(&destAddr,
                                             &incomingSwPort->portAddr))
            {
                // This is a unicast frame for switch.
                // Currently all packets for upper layer are dropped.
                if (SWITCH_DELIVER_PKTS_TO_UPPER_LAYERS)
                {
                vlanStats->numUnicastDeliveredToUpperLayer++;
                incomingSwPort->stats.numUnicastDeliveredToUpperLayer++;

                Switch_HandOffSuccessfullyReceivedPacket(
                    node,
                    incomingInterfaceIndex,
                    msg,
                    sourceAddr,
                    destAddr,
                    vlanClassification,
                    priority);
                }
                else
                {
#ifdef ADDON_DB
                    HandleMacDBEvents(node,
                                      msg,
                                      node->macData[incomingInterfaceIndex]->
                                                                   phyNumber,
                                      incomingInterfaceIndex,
                                      MAC_Drop,
                                      node->macData[incomingInterfaceIndex]->
                                                                 macProtocol,
                                      TRUE,
                                      "Unicast Packet for Upper Layer",
                                      FALSE);
#endif
                    vlanStats->numUnicastDropped++;
                    incomingSwPort->stats.numUnicastDropped++;
                    MESSAGE_Free(node, msg);
                }

            }

            else if (MAC_IsBroadcastMac802Address(&destAddr) ||
                Switch_IsGarpGroupAddr(destAddr, sw))
            {
                BOOL isEnqueuedOne = FALSE;

                // This is a broadcast frame or
                // the switch is not GARP aware.
                Switch_QueueFrameBasedOnMemberSet(
                    node,
                    sw,
                    incomingInterfaceIndex,
                    msg,
                    sourceAddr,
                    destAddr,
                    vlanClassification,
                    priority,
                    &isEnqueuedOne);

                if (isEnqueuedOne)
                {
                    vlanStats->numBroadcastFlooded++;
                    incomingSwPort->stats.numBroadcastFlooded++;
                }
                else
                {
                    vlanStats->numBroadcastDropped++;
                    incomingSwPort->stats.numBroadcastDropped++;
                }

            }
            else
            {
                // This is a unicast frame not for this switch

                // Find if any entry present in filter database
                // for this destination
                SwitchDbEntry* destEntry =
                    SwitchDb_GetEntry(
                        sw->db,
                        fid,
                        destAddr,
                        node->getNodeTime());

                if (destEntry
                    && sw->isMemberSetAware
                    && !SwitchVlan_IsPortInMemberSet(
                            sw, destEntry->port, vlanClassification))
                {
                    // Frame cannot be directly forwarded
                    destEntry = NULL;
                }

                if (destEntry)
                {
                    // Entry found
                    BOOL isEnqueued = FALSE;

                    Switch_QueueFrameBasedOnFilterDb(
                        node,
                        incomingInterfaceIndex,
                        msg,
                        destEntry,
                        sourceAddr,
                        destAddr,
                        vlanClassification,
                        priority,
                        &isEnqueued);

                    if (isEnqueued)
                    {
                        vlanStats->numUnicastDeliveredDirectly++;
                        incomingSwPort->stats.numUnicastDeliveredDirectly++;
                    }
                    else
                    {
                        vlanStats->numUnicastDropped++;
                        incomingSwPort->stats.numUnicastDropped++;
                    }
                }
                else
                {
                    // Entry not found. So forward the frame based on
                    // the member set for this vlan classification
                    BOOL isEnqueuedOne = FALSE;

                    Switch_QueueFrameBasedOnMemberSet(
                        node,
                        sw,
                        incomingInterfaceIndex,
                        msg,
                        sourceAddr,
                        destAddr,
                        vlanClassification,
                        priority,
                        &isEnqueuedOne);

                    if (isEnqueuedOne)
                    {
                        vlanStats->numUnicastFlooded++;
                        incomingSwPort->stats.numUnicastFlooded++;
                    }
                    else
                    {
                        vlanStats->numUnicastDropped++;
                        incomingSwPort->stats.numUnicastDropped++;
                    }
                }
            }
        }
    }
}


//--------------------------------------------------------------------------
// protocol specific routine


// NAME:        Switch_LinkReceivedFrame
//
// PURPOSE:     Received frame on a link port
//
// PARAMETERS:  Interface index of the incoming port
//              Pointer to the received frame
//
// RETURN:      None

static
void Switch_LinkReceivedFrame(
    Node *node,
    int interfaceIndex,
    Message *msg)
{
    MacSwitch* sw = Switch_GetSwitchData(node);
    LinkData *link
        = (LinkData *)node->macData[interfaceIndex]->macVar;

    LinkFrameHeader *header = (LinkFrameHeader*)MESSAGE_ReturnPacket(msg);

    Mac802Address sourceAddr;
    Mac802Address destAddr;

    BOOL isVlanTag = FALSE;
    MacHeaderVlanTag tagInfo;
    VlanId vlanId = SWITCH_VLAN_ID_DEFAULT;
    TosType priority = SWITCH_FRAME_PRIORITY_DEFAULT;

    // Retrieve normal frame header


   MAC_CopyMac802Address(&sourceAddr, &header->sourceAddr);
   MAC_CopyMac802Address(&destAddr, &header->destAddr);
    isVlanTag = header->vlanTag;

    if (link->phyType == WIRELESS) {
        // receiving end of wireless link can calculate/store propDelay
        WirelessLinkInfo* wli = (WirelessLinkInfo*) MESSAGE_ReturnInfo(msg);
        link->myMacData->propDelay = wli->propDelay;
    }

    STAT_DestAddressType type = STAT_Unicast;
    BOOL isMyFrame = FALSE;
    BOOL isAnyFrame = FALSE;

#ifdef ADDON_DB
    HandleMacDBEvents(node,
                      msg,
                      node->macData[interfaceIndex]->phyNumber,
                      interfaceIndex,
                      MAC_ReceiveFromPhy,
                      node->macData[interfaceIndex]->macProtocol);
#endif
    MacLinkGetPacketProperty(node,
                             msg,
                             interfaceIndex,
                             type,
                             isMyFrame,
                             isAnyFrame);

    MESSAGE_RemoveHeader(node, msg, sizeof(LinkFrameHeader), TRACE_LINK);
    Int32 controlSize = sizeof(LinkFrameHeader);

    // Assign default value
    memset(&tagInfo, 0, sizeof(MacHeaderVlanTag));
    tagInfo.userPriority = SWITCH_FRAME_PRIORITY_DEFAULT;

    if (isVlanTag)
    {
        // Vlan tag present in header.
        // So collect vlan information and remove header
        MacHeaderVlanTag* tagHeader =
            (MacHeaderVlanTag *) MESSAGE_ReturnPacket(msg);

        memcpy(&tagInfo, tagHeader, sizeof(MacHeaderVlanTag));

        MESSAGE_RemoveHeader(
            node, msg, sizeof(MacHeaderVlanTag), TRACE_VLAN);
        controlSize += sizeof(MacHeaderVlanTag);
    }

    if (node->macData[interfaceIndex]->macStats)
    {
        node->macData[interfaceIndex]->stats
            ->AddFrameReceivedDataPoints(node,
                                         msg,
                                         type,
                                         controlSize,
                                         MESSAGE_ReturnPacketSize(msg),
                                         interfaceIndex);
    }

    if (sw->isVlanAware)
    {
        vlanId = (unsigned short) (tagInfo.vlanIdentifier);
        priority = (TosType)(tagInfo.userPriority);
    }

    Switch_ReceiveFrameFromMac(
        node,
        msg,
        sourceAddr,
        destAddr,
        interfaceIndex,
        vlanId,
        priority);
}


// NAME:        Switch_LinkSendFrame
//
// PURPOSE:     send frame on a link port
//
// PARAMETERS:  Interface index of the outgoing port
//
// RETURN:      None

static
void Switch_LinkSendFrame(
    Node *node,
    int interfaceIndex)
{
    Message* newMsg = NULL;

    Mac802Address srcAddr;
    Mac802Address macnextHop;
    VlanId vlanId;
    TosType userPriority;
    BOOL isFrameTagged = FALSE;
    Int64 effectiveBW = 0;

    LinkData* link = (LinkData*) node->macData[interfaceIndex]->macVar;
    MacData* thisMac = link->myMacData;

    MacSwitch* sw = Switch_GetSwitchData(node);
    SwitchPort* port = Switch_GetPortDataFromInterface(
                           sw,
                           interfaceIndex);

    Scheduler* scheduler = port->outPortScheduler;

    if (link->status == LINK_IS_BUSY)
    {
        return;
    }

    ERROR_Assert(link->status == LINK_IS_IDLE,
        "Switch_LinkSendFrame: "
        "Sending frame to link when busy.\n");

    Switch_OutQueueDequeuePacket(
        node,
        scheduler,
        &newMsg,
        &srcAddr,
        &macnextHop,
        &vlanId,
        &userPriority);

    ERROR_Assert(newMsg != NULL,
        "Switch_LinkSendFrame: "
        "Attempting to send frame when queue is empty.\n");

    isFrameTagged = Switch_IsOutgoingFrameTagged(
        node, sw, vlanId, port->portNumber);

    // Check BG traffic is in this interface.
    if (thisMac->bgMainStruct)
    {
        int bgUtilizeBW = 0;

        // Calculate total BW used by the BG traffic.
        bgUtilizeBW = BgTraffic_BWutilize(node,
                                          thisMac,
                                          userPriority);

        // Calculate the effective BW will be used by the real traffic.
        effectiveBW = thisMac->bandwidth - bgUtilizeBW;

        if (effectiveBW < 0)
        {
            effectiveBW = 0;
        }
    }
    else
    {
        effectiveBW = thisMac->bandwidth;
    }

    if (isFrameTagged) {
        LinkSendTaggedFrame(node,
                            newMsg,
                            link,
                            interfaceIndex,
                            srcAddr,
                            macnextHop,
                            vlanId,
                            userPriority,
                            effectiveBW);
    }
    else {
        LinkSendFrame(node,
                      newMsg,
                      link,
                      interfaceIndex,
                      srcAddr,
                      macnextHop,
                      FALSE,
                      userPriority,
                      effectiveBW);
    }
}


// NAME:        Switch_802_3ReceivedFrame
//
// PURPOSE:     Received frame on a port supports 802.3
//
// PARAMETERS:  Pointer to the Node structure
//              Interface index of the incoming port
//              Pointer to the received frame
//
// RETURN:      None

static
void Switch_802_3ReceivedFrame(
    Node *node,
    int interfaceIndex,
    Message *msg)
{
    MacSwitch* sw = Switch_GetSwitchData(node);
    MacData802_3 *mac802_3 = (MacData802_3 *)
        node->macData[interfaceIndex]->macVar;

    MacHeaderVlanTag tagInfo;

    Mac802Address destAddr;
    Mac802Address srcAddr;
    VlanId vlanId = SWITCH_VLAN_ID_DEFAULT;
    TosType priority = SWITCH_FRAME_PRIORITY_DEFAULT;

    unsigned char  *macEthernetFramePtr =
        (unsigned char *) MESSAGE_ReturnPacket (msg);

    macEthernetFramePtr += 8;

    // Get destination station address
    memcpy(destAddr.byte, macEthernetFramePtr, MAC_ADDRESS_LENGTH_IN_BYTE);

    macEthernetFramePtr += MAC_ADDRESS_LENGTH_IN_BYTE;

    // Get source station address
    memcpy(srcAddr.byte, macEthernetFramePtr, MAC_ADDRESS_LENGTH_IN_BYTE);

    // Initialize tag info field
    memset(&tagInfo, 0, sizeof(MacHeaderVlanTag));
    tagInfo.userPriority = SWITCH_FRAME_PRIORITY_DEFAULT;

#ifdef ADDON_DB
    HandleMacDBEvents(node,
                      msg,
                      node->macData[interfaceIndex]->phyNumber,
                      interfaceIndex,
                      MAC_ReceiveFromPhy,
                      node->macData[interfaceIndex]->macProtocol);
#endif

    if (!mac802_3->isFullDuplex)
    {
        STAT_DestAddressType type = STAT_Unicast;
        BOOL isMyFrame = FALSE;
        BOOL isAnyFrame = FALSE;
        Int32 destNodeId = ANY_NODEID;
        Int32 controlSize = MAC802_3_SIZE_OF_HEADER;

        Mac802_3GetPacketProperty(node,
                                  msg,
                                  interfaceIndex,
                                  destNodeId,
                                  controlSize,
                                  isMyFrame,
                                  isAnyFrame);

        if (isMyFrame || !isAnyFrame)
        {
            type = STAT_Unicast;
        }
        else
        {
            type = STAT_Broadcast;
        }

        if (node->macData[interfaceIndex]->macStats)
        {
            node->macData[interfaceIndex]->stats
                ->AddFrameReceivedDataPoints(
                             node,
                             msg,
                             type,
                             controlSize,
                             MESSAGE_ReturnPacketSize(msg) - controlSize,
                             interfaceIndex);
        }
    }

    Mac802_3ConvertFrameIntoPacket(node, interfaceIndex, msg, &tagInfo);

    if (sw->isVlanAware)
    {
        vlanId = (unsigned short)(tagInfo.vlanIdentifier);
        priority = (TosType) (tagInfo.userPriority);
    }

    Switch_ReceiveFrameFromMac(
        node,
        msg,
        srcAddr,
        destAddr,
        interfaceIndex,
        vlanId,
        priority);
}


// NAME:        Switch_802_3CreateFrame
//
// PURPOSE:     Create the frame 802.3 using dequeued packet.
//
// PARAMETERS:  Message to convert in frame
//              interface index
//              Length of the packet
//              Next hop address
//              Source address
//              Outgoing frame vlan id
//              Priority
//
// RETURN:      None

static
void Switch_802_3CreateFrame(
    Node *node,
    Message *msg,
    MacData802_3 *mac802_3,
    int interfaceIndex,
    unsigned short lengthOfData,
    Mac802Address* nextHopAddress,
    Mac802Address* sourceAddr,
    VlanId vlanId,
    TosType priority,
    BOOL isSendingFrameTagged)
{
    int lengthOfFrameExcludePad = 0;
    int padRequired = 0;

    unsigned char *frameHeader;
    int sizeOfFrameHeader = MAC802_3_SIZE_OF_HEADER;
    int padAndChecksumSize = 0;


    // Check whether padding is required or not.
    // If required then calculate the size of pad.
    lengthOfFrameExcludePad =
        (lengthOfData + MAC802_3_SIZE_OF_HEADER + MAC802_3_SIZE_OF_TAILER
         - MAC802_3_SIZE_OF_PREAMBLE_AND_SF_DELIMITER);

    if (mac802_3->bwInMbps >= MAC802_3_1000BASE_ETHERNET)
    {
        // To make the length of the frame 512 bytes for GB Ethernet
        padRequired = MAC802_3_MINIMUM_FRAME_LENGTH_GB_ETHERNET
                      - lengthOfFrameExcludePad;
    }
    else
    {
        // To make the length of the frame 64 bytes
        padRequired =
            MAC802_3_MINIMUM_FRAME_LENGTH - lengthOfFrameExcludePad;
    }

    // Validate pad value
    padRequired = (padRequired <= 0) ? 0 : padRequired;


    // Create Frame and add vlan tag if required.

    padAndChecksumSize = MAC802_3_SIZE_OF_TAILER + padRequired;

    // If station sends tagged frame, modify header length
    if (isSendingFrameTagged)
    {
        sizeOfFrameHeader += sizeof(MacHeaderVlanTag);

        // need to reduce the padAndChecksumSize with size of VlanTag?
    }

    // Convert the IP datagram into Ethernet Frame.
    // For this purpose header and tailer is required to add
    // with the IP datagram. In this implementation, tailer &
    // optional data padding will be added as virtual payload.
    //
    MESSAGE_AddHeader(node,
                      msg,
                      sizeOfFrameHeader,
                      TRACE_802_3);

    // Get the packet
    frameHeader = (unsigned char *) MESSAGE_ReturnPacket(msg);

    // Assign 10101010... sequence to preamble part of frame header
    memset (frameHeader, 0xAA, 7);
    frameHeader = frameHeader + 7;

    // Assign Start Frame Delimeter
    memset (frameHeader, 0xAB, 1);
    frameHeader++;

    // Assign destAddr to the frame
    memcpy (frameHeader, nextHopAddress->byte, MAC_ADDRESS_LENGTH_IN_BYTE);
    frameHeader = frameHeader + MAC_ADDRESS_LENGTH_IN_BYTE;

    // Assign sourceAddr to the frame
    memcpy (frameHeader, sourceAddr->byte, MAC_ADDRESS_LENGTH_IN_BYTE);
    frameHeader = frameHeader + MAC_ADDRESS_LENGTH_IN_BYTE;

    // Check if vlan tagging required
    if (isSendingFrameTagged)
    {
        MacHeaderVlanTag tagField;

        // Collect tag informations
        tagField.tagProtocolId = 0x8100;
        tagField.canonicalFormatIndicator = 0;
        tagField.userPriority = priority;
        tagField.vlanIdentifier = vlanId;

        // Assign vlan tag
        memcpy(frameHeader, &tagField, sizeof(MacHeaderVlanTag));
        frameHeader = frameHeader + sizeof(MacHeaderVlanTag);
    }

    // If the type is greater than 1536, then this field
    // should contain the type of data
    memcpy (frameHeader, &lengthOfData, sizeof(unsigned short));
    frameHeader = frameHeader + sizeof(unsigned short);

    // Add pad & checksum to virtual payload
    MESSAGE_AddVirtualPayload(node, msg, padAndChecksumSize);
}


// /**
// FUNCTION::    Switch_802_3SendFrame
// PURPOSE::     Send frame on a port supports 802.3
// PARAMETERS::
//      +node : Node* : Pointer to the Node structure
//      +interfaceIndex : int : Interface index of the outgoing port
// RETURN: void
// **/
static
void Switch_802_3SendFrame(
    Node *node,
    int interfaceIndex)
{
    Message *msg = NULL;
    unsigned short lengthOfData;

    TosType userPriority;
    VlanId vlanId;
    BOOL isFrameTagged = FALSE;
    Mac802Address srcMacAddr;
    Mac802Address destMacAddr;

    MacData802_3 *mac802_3 = (MacData802_3 *)
        node->macData[interfaceIndex]->macVar;

    MacSwitch* sw = Switch_GetSwitchData(node);
    SwitchPort* port = Switch_GetPortDataFromInterface(
                           sw,
                           interfaceIndex);

    Scheduler* scheduler = port->outPortScheduler;

    if (mac802_3->stationState == IDLE_STATE)
    {
        // Check if there is any frame into own buffer
        if (mac802_3->msgBuffer != NULL)
        {
            // Frame present, so don't retrieve now
            return;
        }

        // Dequeue frame
        Switch_OutQueueDequeuePacket(
            node,
            scheduler,
            &msg,
            &srcMacAddr,
            &destMacAddr,
            &vlanId,
            &userPriority);

        if (msg == NULL)
        {
            return;
        }

        if (MAC_IsIdenticalMac802Address(&srcMacAddr, &destMacAddr))
        {
            char errorBuf[MAX_STRING_LENGTH];
            sprintf(errorBuf,
                "Node %d is trying to send packet to itself\n",
                node->nodeId);
            ERROR_ReportError(errorBuf);
        }

        lengthOfData = (unsigned short)(MESSAGE_ReturnPacketSize(msg));

        if (lengthOfData <= 0)
        {
            char errorBuf[MAX_STRING_LENGTH];
            sprintf(errorBuf,
                "Packet of length %d has come at Node %d\n",
                lengthOfData, node->nodeId);
            ERROR_ReportWarning(errorBuf);
            return;
        }

        isFrameTagged = Switch_IsOutgoingFrameTagged(
            node, sw, vlanId, port->portNumber);

        // Create Frame using the retrieved packet
        Switch_802_3CreateFrame(
            node,
            msg,
            mac802_3,
            interfaceIndex,
            lengthOfData,
            &destMacAddr,
            &srcMacAddr,
            vlanId,
            userPriority,
            isFrameTagged);

        // Collect it into own buffer
        MESSAGE_SetEvent(msg, MSG_MAC_TransmissionFinished);
        MESSAGE_SetLayer(msg, MAC_LAYER, MAC_PROTOCOL_802_3);
        MESSAGE_SetInstanceId(msg, (short) interfaceIndex);

        if (mac802_3->isFullDuplex)
        {
            Mac802_3FullDupSend(node,
                                interfaceIndex,
                                mac802_3,
                                msg,
                                userPriority,
                                0,
                                SWITCH);
            return;
        }

        mac802_3->msgBuffer = msg;

        // Sense the channel to transmit the frame
        Mac802_3SenseChannel(node, mac802_3);
    }
}


// ------------------------------------------------------------------
// Init routines


// NAME:        Switch_DbInit
//
// PURPOSE:     Initialize filter database
//
// PARAMETERS:  Pointer that holds filter database
//
// RETURN:      None

static
void Switch_DbInit(
    Node* node,
    const NodeInput* nodeInput,
    SwitchDb** db)
{
    BOOL retVal;
    int  intInput;
    char buf[MAX_STRING_LENGTH];
    char errString[MAX_STRING_LENGTH];

    SwitchDb* swDb = (SwitchDb*) MEM_malloc(sizeof(SwitchDb));

    memset(swDb, 0, sizeof(SwitchDb));

    *db = (SwitchDb*) swDb;

    // Read max number of dynamic entries in database
    // Format is SWITCH-DATABASE-MAX-ENTRIES <value>
    //
    IO_ReadInt(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-DATABASE-MAX-ENTRIES",
        &retVal,
        &intInput);

    if (retVal)
    {
        if (intInput < SWITCH_DATABASE_ENTRIES_MIN
            || intInput > SWITCH_DATABASE_ENTRIES_MAX)
        {
            sprintf(errString,
                "MacSwFilterDb_Init: "
                "Error in SWITCH-DATABASE-MAX-ENTRIES "
                "in user configuration.\n"
                "Database max. entries should be between %d to %d\n",
                SWITCH_DATABASE_ENTRIES_MIN,
                SWITCH_DATABASE_ENTRIES_MAX);

            ERROR_ReportError(errString);
        }
    }
    else
    {
        intInput = SWITCH_DATABASE_ENTRIES_DEFAULT;
    }

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-DATABASE-STATISTICS",
        &retVal,
        buf);

    if (retVal && !strcmp(buf, "YES"))
    {
        swDb->statsEnabled = TRUE;
    }
    else
    {
        swDb->statsEnabled = FALSE;
    }

    SwitchDb_Init(swDb, intInput);
}


// NAME:        Switch_BackplaneInit
//
// PURPOSE:     Initialize back plane for this switch
//
// PARAMETERS:  Switch data
//              Node input file
//
// RETURN:      None

static
void Switch_BackplaneInit(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput)
{
    double backplaneThroughput = 0;
    BOOL retVal = FALSE;
    char buf[MAX_STRING_LENGTH];

    IO_ReadDouble(
         node->nodeId,
         ANY_ADDRESS,
         nodeInput,
         "SWITCH-BACKPLANE-THROUGHPUT",
         &retVal,
         &backplaneThroughput);

    if (!retVal)
    {
        // If not specified, we assume infinite backplane throughput.
        sw->swBackplaneThroughputCapacity =
            SWITCH_UNLIMITED_BACKPLANE_THROUGHPUT;

    }
    else
    {
        if (backplaneThroughput
            < SWITCH_UNLIMITED_BACKPLANE_THROUGHPUT)
        {
            char buf[MAX_STRING_LENGTH];

            sprintf(buf,
                    "Switch Backplane throughput must be "
                    "greater than or equal to %u.\n",
                    0);
            ERROR_ReportError(buf);
        }

        sw->swBackplaneThroughputCapacity =
            (clocktype) backplaneThroughput;

        sw->backplaneStatus = SWITCH_BACKPLANE_IDLE;
    }

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-BACKPLANE-STATISTICS",
        &retVal,
        buf);

    if (retVal && !strcmp(buf, "YES"))
    {
        sw->backplaneStatsEnabled = TRUE;
    }
    else
    {
        sw->backplaneStatsEnabled = FALSE;
    }
}


// NAME:        Switch_CpuSchedulerInit
//
// PURPOSE:     Initialise cpu scheduler at a port
//
// PARAMETERS:  Switch data
//
// RETURN:      None

static
void Switch_CpuSchedulerInit(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput)
{
    if (sw->swBackplaneThroughputCapacity !=
        SWITCH_UNLIMITED_BACKPLANE_THROUGHPUT)
    {
        BOOL retVal = FALSE;
        int queueSize = SWITCH_CPU_QUEUE_SIZE_DEFAULT;
        Scheduler* cpuSchedulerPtr = NULL;
        Queue* queue = NULL;

        // We assume only strict priority scheduler for CPU buffer.
        SCHEDULER_Setup(&cpuSchedulerPtr, "STRICT-PRIORITY");
        sw->cpuScheduler = cpuSchedulerPtr;

        IO_ReadInt(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "SWITCH-CPU-QUEUE-SIZE",
            &retVal,
            &queueSize);

        if (!retVal)
        {
            queueSize = SWITCH_CPU_QUEUE_SIZE_DEFAULT;
        }

        if (queueSize <= 0)
        {
            ERROR_ReportError(
                "Cpu Queue Size must be > 0 and < 2147483647\n");
        }

        // We assume only one priority queue for CPU buffer.
        // We assume FIFO queue for CPU buffer.
        queue = new Queue; // FIFO
        queue->SetupQueue(node, "FIFO", queueSize);

        // Scheduler add Queue Functionality
        (*cpuSchedulerPtr).addQueue(queue);

        // Call priority map function
        Switch_MapUserToQueuePriority(
            sw->cpuSchedulerPriorityToQueueMap,
            (unsigned short) (*cpuSchedulerPtr).numQueue());
    }
    else
    {
        sw->cpuScheduler = NULL;
    }
}


// NAME:        SwitchPort_InputSchedulerInit
//
// PURPOSE:     Initialise input scheduler at a port
//
// PARAMETERS:  Switch data
//              Port identifier
//
// RETURN:      None

static
void SwitchPort_InputSchedulerInit(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    const NodeInput* nodeInput)
{
    int queueSize = SWITCH_INPUT_QUEUE_SIZE_DEFAULT;
    unsigned int i;

    BOOL retVal = FALSE;

    // Initialization for input schedulers
    if (sw->swBackplaneThroughputCapacity
        != SWITCH_UNLIMITED_BACKPLANE_THROUGHPUT)
    {
        unsigned int numInPriorities = 1;
        Scheduler* inPortSchedulerPtr = NULL;

        // We assume only strict priority scheduler for input buffer.
        SCHEDULER_Setup(&inPortSchedulerPtr, "STRICT-PRIORITY");
        port->inPortScheduler = inPortSchedulerPtr;

        // Read size of input queues
        // Format is [switch ID] SWITCH-INPUT-QUEUE-SIZE[port ID]   <value>
        //        or [port addr] SWITCH-INPUT-QUEUE-SIZE            <value>
        //
        IO_ReadIntInstance(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "SWITCH-INPUT-QUEUE-SIZE",
            port->portNumber,
            TRUE,
            &retVal,
            &queueSize);

        if (!retVal)
        {
            char buf[MAX_STRING_LENGTH];

            IO_ReadStringUsingIpAddress(
                node,
                port->macData->interfaceIndex,
                nodeInput,
                "SWITCH-INPUT-QUEUE-SIZE",
                &retVal,
                buf);

            if (retVal)
            {
                queueSize = atoi(buf);
            }
        }

        if (!retVal)
        {
            queueSize = SWITCH_INPUT_QUEUE_SIZE_DEFAULT;
        }

        if (queueSize <= 0)
        {
            ERROR_ReportError("Input Queue Size must be > 0 "
                "and < 2147483647\n");
        }

        // Initialization of Fifo with n number of priorities
        // Though num priority is assigned to 1, the loop is there
        // for later enhancement
        for (i = 0; i < numInPriorities; i++)
        {
            Queue* queue = NULL;
            queue = new Queue; // "FIFO"
            // For now there is no Finalize for input queues
            queue->SetupQueue(node,
                              "FIFO",
                              queueSize,
                              port->macData->interfaceIndex,
                              i, //priority
                              0);

            // Scheduler add Queue Functionality
            (*inPortSchedulerPtr).addQueue(queue);
        }

        // Call priority map function
        Switch_MapUserToQueuePriority(
            port->inPortPriorityToQueueMap,
            (unsigned short) (*inPortSchedulerPtr).numQueue());

        // Since backplane is enabled set backplane status idle
        port->backplaneStatus = SWITCH_BACKPLANE_IDLE;
    }
    else
    {
        port->inPortScheduler = NULL;
    }
}


// NAME:        SwitchPort_SchedulerInit
//
// PURPOSE:     Initialise the queue scheduler at a port
//
// PARAMETERS:  Port identifier
//
// RETURN:      None

static
void SwitchPort_SchedulerInit(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    const NodeInput* nodeInput)
{
    BOOL retVal = FALSE;
    int numPriorities = 0;
    int queueSize = 0;
    char errString[MAX_STRING_LENGTH];
    QueuePriorityType i = ALL_PRIORITIES;
    Scheduler* outPortSchedulerPtr = NULL;
    char buf[MAX_STRING_LENGTH];
    BOOL collectStatForQueue = FALSE;
    BOOL enableSchedulerStat = FALSE;

    // Format is [switch ID] SWITCH-SCHEDULER-STATISTICS[port ID]   NO | YES
    //        or [port addr] SWITCH-SCHEDULER-STATISTICS            NO | YES
    IO_ReadStringInstance(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-SCHEDULER-STATISTICS",
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
            "SWITCH-SCHEDULER-STATISTICS",
            &retVal,
            buf);
    }

    if (retVal && !strcmp(buf, "YES"))
    {
        enableSchedulerStat = TRUE;
    }

    // We assume only strict priority scheduler for outport buffer.
    SCHEDULER_Setup(&outPortSchedulerPtr,
                    "STRICT-PRIORITY",
                    enableSchedulerStat);

    port->outPortScheduler = outPortSchedulerPtr;

    // Read number of priority queues
    // Format is [switch ID] SWITCH-QUEUE-NUM-PRIORITIES[port ID]    <value>
    //        or [port addr] SWITCH-QUEUE-NUM-PRIORITIES             <value>
    IO_ReadIntInstance(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-QUEUE-NUM-PRIORITIES",
        port->portNumber,
        TRUE,
        &retVal,
        &numPriorities);

    if (!retVal)
    {
        char buf[MAX_STRING_LENGTH];

        IO_ReadStringUsingIpAddress(
            node,
            port->macData->interfaceIndex,
            nodeInput,
            "SWITCH-QUEUE-NUM-PRIORITIES",
            &retVal,
            buf);

        if (retVal)
        {
            numPriorities = atoi(buf);
        }
    }

    if (retVal)
    {
        if (numPriorities < SWITCH_QUEUE_NUM_PRIORITIES_MIN
            || numPriorities > SWITCH_QUEUE_NUM_PRIORITIES_MAX)
        {
            sprintf(errString,
                "SwitchPort_SchedulerInit:"
                "SWITCH-QUEUE-NUM-PRIORITIES for node %u is out of range\n"
                "Value of this parameter should be within %u to %u\n",
                node->nodeId,
                SWITCH_QUEUE_NUM_PRIORITIES_MIN,
                SWITCH_QUEUE_NUM_PRIORITIES_MAX);

            ERROR_ReportError(errString);
        }
    }
    else
    {
        numPriorities = SWITCH_QUEUE_NUM_PRIORITIES_DEFAULT;
    }

    // Read size of output queues
    // Format is [switch ID] SWITCH-OUTPUT-QUEUE-SIZE[port ID]   <value>
    //        or [port addr] SWITCH-OUTPUT-QUEUE-SIZE            <value>
    //
    IO_ReadIntInstance(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-OUTPUT-QUEUE-SIZE",
        port->portNumber,
        TRUE,
        &retVal,
        &queueSize);

    if (!retVal)
    {
        char buf[MAX_STRING_LENGTH];

        IO_ReadStringUsingIpAddress(
            node,
            port->macData->interfaceIndex,
            nodeInput,
            "SWITCH-OUTPUT-QUEUE-SIZE",
            &retVal,
            buf);

        if (retVal)
        {
            queueSize = atoi(buf);
        }
    }

    if (!retVal)
    {
        queueSize = SWITCH_OUTPUT_QUEUE_SIZE_DEFAULT;
    }

    if (queueSize <= 0)
    {
        ERROR_ReportError(
            "Output Queue Size must be > 0 and < 2147483647\n");
    }

    // Format is [switch ID] SWITCH-QUEUE-STATISTICS[port ID]   NO | YES
    //        or [port addr] SWITCH-QUEUE-STATISTICS            NO | YES
    IO_ReadStringInstance(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-QUEUE-STATISTICS",
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
            "SWITCH-QUEUE-STATISTICS",
            &retVal,
            buf);
    }

    if (retVal && !strcmp(buf, "YES"))
    {
        collectStatForQueue = TRUE;
    }
    else
    {
        collectStatForQueue = FALSE;
    }

    // Initialization of Fifo with n number of priorities
    for (i = 0; i < numPriorities; i++)
    {
        Queue* queue = NULL;
        queue = new Queue; // "FIFO"
        // For now there is no Finalize for input queues
        queue->SetupQueue(node,
                          "FIFO",
                          queueSize,
                          port->macData->interfaceIndex,
                          i, //queueNumber
                          0, //infoFieldSize,
                          collectStatForQueue);

        // Scheduler add Queue Functionality
        (*outPortSchedulerPtr).addQueue(queue);
    }

    // Call priority map function
    Switch_MapUserToQueuePriority(
        port->outPortPriorityToQueueMap,
        (unsigned short) (*outPortSchedulerPtr).numQueue());

    SwitchPort_InputSchedulerInit(
        node,
        sw,
        port,
        nodeInput);
}


// NAME:        SwitchPort_StatInit
//
// PURPOSE:     Initialize the stat variables of a port
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchPort_StatInit(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;

    ListInit(node, &port->portVlanStatList);

    // Format is [switch ID] SWITCH-PORT-STATISTICS[port ID]   NO | YES
    //        or [port addr] SWITCH-PORT-STATISTICS            NO | YES
    IO_ReadStringInstance(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-PORT-STATISTICS",
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
            "SWITCH-PORT-STATISTICS",
            &retVal,
            buf);
    }

    if (retVal && !strcmp(buf, "YES"))
    {
        port->isPortStatEnabled = TRUE;
    }
    else
    {
        port->isPortStatEnabled = FALSE;
    }
}


// NAME:        Switch_ProtocolSpecificInit
//
// PURPOSE:     Initialize the stat variables of a port
//
// PARAMETERS:
//
// RETURN:      None

static
void Switch_ProtocolSpecificInit(
    Node* node,
    SwitchPort* port)
{
    char errString[MAX_STRING_LENGTH];
    switch (port->macData->macProtocol)
    {
        case MAC_PROTOCOL_802_3:
        {
            port->macData->sendFrameFn = Switch_802_3SendFrame;
            port->macData->receiveFrameFn = Switch_802_3ReceivedFrame;
            break;
        }
        case MAC_PROTOCOL_LINK:
        {
            port->macData->sendFrameFn = Switch_LinkSendFrame;
            port->macData->receiveFrameFn = Switch_LinkReceivedFrame;
            break;
        }
        case MAC_PROTOCOL_SWITCHED_ETHERNET:
        {
            sprintf(errString,
                "Detailed switch cannot be part of a switched Ethernet "
                "(abstract switch) network.");

            ERROR_ReportError(errString);
            break;
        }
        default:
        {
            sprintf(errString,
                "MAC-PROTOCOL not found for port %d",
                port->portNumber);

            ERROR_ReportError(errString);
        }
    }
}


// NAME:        Switch_InitPortsWithNoPortMapInput
//
// PURPOSE:     Initialize the ports without a map of port numbers
//              to interface address. Assigns port number as the
//              interface index.
//
//              Useful in avoiding lengthy port map input for large
//              scenarios. Suggested use is for situations when
//              port specific input is at default. Else, is senstive
//              to the order of SUBNET/LINK statements and not
//              regression testable.
//
// PARAMETERS:
//
// RETURN:      None

static
void Switch_InitPortsWithNoPortMapInput(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput)
{
    int portCount;
    SwitchPort* port;
    SwitchPort* lastPort = NULL;

    for (portCount = 0; portCount < node->numberInterfaces; portCount++)
    {
        port = (SwitchPort *) MEM_malloc(sizeof(SwitchPort));
        memset(port, 0, sizeof(SwitchPort));

        port->macData = node->macData[portCount];

        ConvertVariableHWAddressTo802Address(
                                  node,
                                  &node->macData[portCount]->macHWAddr,
                                  &port->portAddr);

        port->backplaneStatus = SWITCH_BACKPLANE_INVALID;

        if (!lastPort)
        {
            sw->firstPort = port;
        }
        else
        {
            lastPort->nextPort = port;
        }

        port->portNumber = (unsigned short)(portCount + 1);

        port->nextPort = NULL;
        lastPort = port;

        Switch_ProtocolSpecificInit(node, port);

        SwitchPort_StatInit(node, sw, port, nodeInput);
        SwitchStp_PortInit(node, sw, port, nodeInput, portCount + 1);

        SwitchPort_SchedulerInit(node, sw, port, nodeInput);
        SwitchPort_VlanInit(node, sw, port, nodeInput);
    }
}


// NAME:        Switch_PortsInit
//
// PURPOSE:     Initialize all the ports of a switch
//
// PARAMETERS:
//
// RETURN:      None

static
void Switch_PortsInit(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput)
{
    SwitchPort* port;
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];
    char errString[MAX_STRING_LENGTH];
    int portCount;
    BOOL isNodeId = FALSE;
    NodeAddress anIpAddress = 0;
    MacHWAddress aHWAddress;
    BOOL isInterfaceIpMatched;
    int interfaceIndex;
    SwitchPort* lastPort = NULL;
    int numPortInitialized = 0;
    BOOL isMappingTypeAuto = FALSE;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-PORT-MAPPING-TYPE",
        &retVal,
        buf);

    if (retVal)
    {
        if (!strcmp(buf, "AUTO"))
        {
            // Automatic mapping for switch port
            isMappingTypeAuto = TRUE;
        }
        else if (!strcmp(buf, "MANUAL"))
        {
            // Allow user to configure manually.
            isMappingTypeAuto = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "The supported values for "
                "SWITCH-PORT-MAPPING-TYPE are AUTO and MANUAL.\n"
                "MANUAL value requires user input with"
                "SWITCH-PORT-MAP.\n");
        }
    }

    if (isMappingTypeAuto == TRUE)
    {
        Switch_InitPortsWithNoPortMapInput(node, sw, nodeInput);
        return;
    }

    // Read number of ports
    // Format is SWITCH-PORT-COUNT <value>
    // Range is 2 to 255
    //
    // Note: If the standard 4095 max limit is desired,
    // change SWITCH_NUMBER_OF_PORTS_MAX and
    // structure StpPortId
    for (portCount = 1; portCount <= SWITCH_NUMBER_OF_PORTS_MAX;
            portCount++)
    {
        IO_ReadStringInstance(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "SWITCH-PORT-MAP",
            portCount,
            FALSE,
            &retVal,
            buf);

        if (retVal)
        {
            int infIndex = -1;
            NetworkType layer3AddrType;

            numPortInitialized++;

            port = (SwitchPort *) MEM_malloc(sizeof(SwitchPort));
            memset(port, 0, sizeof(SwitchPort));

            isInterfaceIpMatched = FALSE;

            // Check address type given to configure switch port.
            // Convert that address to proper link layer address.
            //
            layer3AddrType = Ipv6GetAddressTypeFromString(buf);

            if (layer3AddrType == NETWORK_IPV4)
            {
                IO_ParseNodeIdOrHostAddress(
                    buf, &anIpAddress, &isNodeId);

                infIndex =
                    MAPPING_GetInterfaceIndexFromInterfaceAddress(
                                                node, anIpAddress);

            }
            else if (layer3AddrType == NETWORK_IPV6)
            {
                in6_addr ipv6Address;
                Address address;

                IO_ConvertStringToAddress(buf, &ipv6Address);
                SetIPv6AddressInfo(&address, ipv6Address);

                infIndex =
                    MAPPING_GetInterfaceIndexFromInterfaceAddress(
                                                node, address);

                // Given string is an ipv6 address.
                isNodeId = FALSE;
            }
            else
            {
                sprintf(errString,
                    "Switch_PortsInit: "
                    "Error in SWITCH-PORT-MAP in user configuration "
                    "for switch %d\n."
                    "Given port address %s for port %d is not a"
                    "valid supported layer3 address.",
                    node->nodeId, buf, portCount);

                ERROR_ReportError(errString);
            }

            aHWAddress = GetMacHWAddress(node, infIndex);

            if (!isNodeId)
            {
                for (interfaceIndex = 0;
                     interfaceIndex < node->numberInterfaces;
                     interfaceIndex ++)
                {
                    if (aHWAddress == GetMacHWAddress(node, interfaceIndex))
                    {

                         Mac802Address aMacAddress;

                         ConvertVariableHWAddressTo802Address(
                            node,
                            &node->macData[interfaceIndex]->macHWAddr,
                            &aMacAddress);

                        Switch_CheckUniquenessOfPortAddress(
                            node, sw, aMacAddress);

                        isInterfaceIpMatched = TRUE;

                        ConvertVariableHWAddressTo802Address(
                            node,
                            &node->macData[interfaceIndex]->macHWAddr,
                            &port->portAddr);
                        port->macData = node->macData[interfaceIndex];
                        break;
                    }
                }

                if (!isInterfaceIpMatched)
                {
                    sprintf(errString,
                        "Switch_PortsInit: "
                        "Error in SWITCH-PORT-MAP in user configuration "
                        "for switch %d\n"
                        "Given port address %s for port %d does not match "
                        "any interface address",
                        node->nodeId, buf, portCount);

                    ERROR_ReportError(errString);
                }
            }
            else
            {
                sprintf(errString,
                    "Switch_PortsInit: "
                    "Error in SWITCH-PORT-MAP in user configuration "
                    "for switch %d\n"
                    "Port address not found for port %d",
                    node->nodeId, portCount);

                ERROR_ReportError(errString);
            }

            if (!lastPort)
            {
                sw->firstPort = port;
            }
            else
            {
                lastPort->nextPort = port;
            }

            port->portNumber = (unsigned short) portCount;
            port->backplaneStatus = SWITCH_BACKPLANE_INVALID;

            port->nextPort = NULL;
            lastPort = port;

            Switch_ProtocolSpecificInit(node, port);

            SwitchPort_StatInit(node, sw, port, nodeInput);
            SwitchStp_PortInit(node, sw, port, nodeInput, portCount);

            SwitchPort_SchedulerInit(node, sw, port, nodeInput);

            SwitchPort_VlanInit(node, sw, port, nodeInput);

            if (numPortInitialized == node->numberInterfaces)
            {
                break;
            }
        }
    }

    if (numPortInitialized != node->numberInterfaces)
    {
        sprintf(errString,
            "Switch_PortsInit: "
            "Error in SWITCH-PORT-MAP configuration for switch %d\n"
            "All available interfaces are not mapped to switch ports.\n",
            node->nodeId);

        ERROR_ReportError(errString);
    }
}


// NAME:        Switch_Init
//
// PURPOSE:     Initialize the switch data of a node
//
// PARAMETERS:
//
// RETURN:      None

void Switch_Init(
    Node* node,
    const NodeInput* nodeInput)
{
    MacSwitch* sw;
    char errString[MAX_STRING_LENGTH];
    Message* timerMsg;

    sw = (MacSwitch*) MEM_malloc(sizeof(MacSwitch));
    memset(sw, 0, sizeof(MacSwitch));

    node->switchData = sw;

    if (node->nodeId >= SWITCH_ID_MIN
        && node->nodeId <= SWITCH_ID_MAX)
    {
        sw->swNumber = (unsigned short)(node->nodeId);
    }
    else
    {
        sprintf(errString,
            "Switch_Init: Error in user configuration for Switches.\n"
            "Switch IDs (node IDs) can range from %u to %u",
            SWITCH_ID_MIN, SWITCH_ID_MAX);
        ERROR_ReportError(errString);
    }


    memcpy(&(sw->Switch_stp_address).byte,stpGroupAddress,6);


    Switch_AssignSwAddress(sw->swNumber,&sw->swAddr,(sw->Switch_stp_address));

    sw->backplaneStatus = SWITCH_BACKPLANE_INVALID;
    sw->packetDroppedAtBackplane = 0;

    SwitchStp_Init(node, sw, nodeInput);
    SwitchStp_TraceInit(node, sw, nodeInput);

    Switch_DbInit(node, nodeInput, &sw->db);

    Switch_BackplaneInit(node, sw, nodeInput);

    Switch_CpuSchedulerInit(node, sw, nodeInput);

    Switch_PortsInit(node, sw, nodeInput);

    // VLANs should be init after the ports
    Switch_VlanInit(node, sw, nodeInput);

    if (sw->runStp)
    {
        // Start the tick timer
        // Initial delay is a random number
        timerMsg = MESSAGE_Alloc(node, MAC_LAYER, MAC_SWITCH,
            MSG_MAC_SwitchTick);
        // Assuming this random seed is only used once.
        RandomSeed seed;
        RANDOM_SetSeed(seed,
                       node->globalSeed,
                       node->nodeId,
                       MAC_SWITCH);
        MESSAGE_Send(node, timerMsg,
            (clocktype) (RANDOM_erand(seed) * MILLI_SECOND + 1));
    }
    else
    {
        SwitchStp_SetVarsWhenStpIsOff(node, sw);
    }
}


// NAME:        Switch_CheckAndInit
//
// PURPOSE:     Checks whether a node is configured as a switch
//              in user configuration file. If node considered as
//              switch, initialize it as switch.
//
// PARAMETERS:
//
// RETURN:      None.

void Switch_CheckAndInit(
    Node* firstNode,
    const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    BOOL isSwitch;
    Node* nextNode;

    nextNode = firstNode;
    while (nextNode)
    {
        if (nextNode->partitionId != nextNode->partitionData->partitionId)
        {
            nextNode = nextNode->nextNodeData;
            continue;
        }

        isSwitch = FALSE;

        // Read in nodes that are switches
        // Format is [node list] SWITCH NO | YES
        // Default is NO
        //
        IO_ReadString(
            nextNode->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "SWITCH",
            &retVal,
            buf);

        if (retVal)
        {
            if (!strcmp(buf, "YES"))
            {
                isSwitch = TRUE;
            }
            else if (!strcmp(buf, "NO"))
            {
                isSwitch = FALSE;
            }
            else
            {
                ERROR_ReportError(
                    "Switch_CheckAndInit: "
                    "Error in SWITCH specification.\n"
                    "Expecting YES or NO\n");
            }
        }

        if (isSwitch)
        {
            Switch_Init(nextNode, nodeInput);
        }

        nextNode = nextNode->nextNodeData;
    }
}


//--------------------------------------------------------------------------
// Layer routines


// NAME:        Switch_SendTickTimerMsg
//
// PURPOSE:     Reschedule tick timer message
//
// PARAMETERS:  Old tick messsage
//
// RETURN:      None

static
void Switch_SendTickTimerMsg(
    Node* node,
    MacSwitch* sw,
    Message* oldMsg)
{
    ERROR_Assert(oldMsg != NULL,
        "Switch_StartTickTimer: old message is NULL.\n");

    MESSAGE_Send(node, oldMsg, SECOND);
}


// NAME:        Switch_Layer
//
// PURPOSE:     Switch layer function
//
// PARAMETERS:
//
// RETURN:      None

void Switch_Layer(
    Node* node,
    int interfaceIndex,
    Message* msg)
{
    MacSwitch* sw = Switch_GetSwitchData(node);
    short eventType = MESSAGE_GetEvent(msg);

    switch (eventType)
    {

    case MSG_MAC_SwitchTick:

        SwitchStp_TickTimerEvent(node, sw);

        // Reuse and resend the tick message
        Switch_SendTickTimerMsg(node, sw, msg);

        break;
    case MSG_MAC_FromBackplane:
        Switch_ReceivePacketFromBackplane(node, sw, msg);
        MESSAGE_Free(node, msg);
        break;

    case MSG_MAC_GarpTimerExpired:
        Garp_Layer(node, interfaceIndex, msg);
        break;

    default:
        MESSAGE_Free(node, msg);
        ERROR_ReportError("Switch_Layer: Unexpected protocol message");

        break;

    } //switch
}


// NAME:        Switch_EnableInterface
//
// PURPOSE:     Enable a disabled interface.
//
// PARAMETERS:  Interface index of the interface
//
// RETURN:      None

void Switch_EnableInterface(
    Node* node,
    int interfaceIndex)
{
    MacSwitch* sw = node->switchData;
    SwitchPort* port = Switch_GetPortDataFromInterface(
        sw, interfaceIndex);

    // Give an indication to each Garp application running
    // at this switch that port is operational.
    //
    if (sw->runGvrp)
    {
        SwitchGvrp_EnablePort(node, sw, port);
    }

    SwitchStp_EnableInterface(node, sw, port);
}


// NAME:        Switch_DisableInterface
//
// PURPOSE:     Disable an enabled interface.
//
// PARAMETERS:  Interface index of the interface
//
// RETURN:      None

void Switch_DisableInterface(
    Node* node,
    int interfaceIndex)
{
    MacSwitch* sw = node->switchData;
    SwitchPort* port = Switch_GetPortDataFromInterface(
        sw, interfaceIndex);

    Switch_ClearMessagesFromQueue(node, port->outPortScheduler);

    SwitchStp_DisableInterface(node, sw, port);

    // Indicate to each Garp application running at this switch,
    // to disable the port.
    //
    if (sw->runGvrp)
    {
        SwitchGvrp_DisablePort(node, sw, port);
    }
}


// NAME:        SwitchPort_EnableForwarding
//
// PURPOSE:     Inform related protocols once a port goes into
//              forwarding state.
//
// PARAMETERS:  Pointer of the port.
//
// RETURN:      None

void SwitchPort_EnableForwarding(
    Node* node,
    SwitchPort* thePort)
{
    MacSwitch* sw = node->switchData;

    if (sw->runGvrp)
    {
        GarpGip_ConnectPort(
            node, &(sw->gvrp->garp), thePort->portNumber);
    }
}


// NAME:        SwitchPort_DisableForwarding
//
// PURPOSE:     Inform related protocols once the port moves
//              from forwarding to blocked state.
//
// PARAMETERS:  Pointer of the port.
//
// RETURN:      None

void SwitchPort_DisableForwarding(
    Node* node,
    SwitchPort* thePort)
{
    MacSwitch* sw = node->switchData;

    if (sw->runGvrp)
    {
        GarpGip_DisconnectPort(
            node, &(sw->gvrp->garp), thePort->portNumber);
    }
}


//--------------------------------------------------------------------------
// Finalize routines


// NAME:        Switch_QueuePrintStats
//
// PURPOSE:     Print queue statistics
//
// PARAMETERS:
//
// RETURN:      None

static
void Switch_QueuePrintStats(
    Node* node,
    SwitchPort* port)
{
    int j = 0;
    Scheduler* schedulerPtr = port->outPortScheduler;

    for (j = 0; j < (*schedulerPtr).numQueue(); j++)
    {
        (*schedulerPtr).invokeQueueFinalize(node,
                                            "Mac-Switch",
                                            port->macData->interfaceIndex,
                                            j,
                                            "IP");
    }
}


// NAME:        SwitchPort_SchedulerPrintStats
//
// PURPOSE:     Print scheduler statistics
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchPort_SchedulerPrintStats(
    Node* node,
    SwitchPort* port)
{
    // Printing statistics of Switch Scheduler
    Scheduler* schedulerPtr = port->outPortScheduler;

    (*schedulerPtr).finalize(node,
                             "Mac-Switch",
                             port->macData->interfaceIndex,
                             "IP");
}


// NAME:        SwitchPort_PrintStats
//
// PURPOSE:     Print port statistics
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchPort_PrintStats(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    char interfaceAddrStr[MAX_STRING_LENGTH];

    NetworkIpGetInterfaceAddressString(
        node, port->macData->interfaceIndex, interfaceAddrStr);

    ctoa(port->stats.numTotalFrameReceived, buf1);
    sprintf(buf,"Total frames received = %s",buf1);

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Port",
        interfaceAddrStr,
        ANY_INSTANCE, //port->portNumber,
        buf);

    ctoa(port->stats.numUnicastDeliveredDirectly, buf1);
    sprintf(buf,"Unicast frames forwarded directly = %s",buf1);

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Port",
        interfaceAddrStr,
        ANY_INSTANCE, //port->portNumber,
        buf);

    ctoa(port->stats.numUnicastFlooded, buf1);
    sprintf(buf,"Unicast frames flooded = %s", buf1);

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Port",
        interfaceAddrStr,
        ANY_INSTANCE, //port->portNumber,
        buf);

    ctoa(port->stats.numUnicastDeliveredToUpperLayer, buf1);
    sprintf(buf,
        "Unicast frames delivered to upper layer = %s",buf1);

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Port",
        interfaceAddrStr,
        ANY_INSTANCE, //port->portNumber,
        buf);

    ctoa(port->stats.numUnicastDropped, buf1);
    sprintf(buf,"Unicast frames dropped = %s",buf1);

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Port",
        interfaceAddrStr,
        ANY_INSTANCE, //port->portNumber,
        buf);

    ctoa(port->stats.numBroadcastFlooded, buf1);
    sprintf(buf,"Broadcast frames forwarded = %s", buf1);

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Port",
        interfaceAddrStr,
        ANY_INSTANCE, //port->portNumber,
        buf);

    ctoa(port->stats.numBroadcastDropped, buf1);
    sprintf(buf,"Broadcast frames dropped = %s", buf1);

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Port",
        interfaceAddrStr,
        ANY_INSTANCE, //port->portNumber,
        buf);

    ctoa(port->stats.numDroppedInDiscardState, buf1);
    sprintf(buf,
        "Frames dropped in discard state = %s",buf1);

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Port",
        interfaceAddrStr,
        ANY_INSTANCE, //port->portNumber,
        buf);

    ctoa(port->stats.numDroppedInLearningState, buf1);
    sprintf(buf,"Frames dropped in learning state = %s", buf1);

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Port",
        interfaceAddrStr,
        ANY_INSTANCE, //port->portNumber,
        buf);

    ctoa(port->stats.numDroppedByIngressFiltering, buf1);
    sprintf(buf,"Frames dropped by ingress filtering = %s", buf1);

    IO_PrintStat(
        node,
        "Mac-Switch",
        "Port",
        interfaceAddrStr,
        ANY_INSTANCE, //port->portNumber,
        buf);

    if (port->stats.numDroppedDueToUnknownVlan)
    {
        ctoa(port->stats.numDroppedDueToUnknownVlan, buf1);
        sprintf(buf,"Frames dropped, received from unknown vlan = %s",buf1);

        IO_PrintStat(
            node,
            "Mac-Switch",
            "Port",
            interfaceAddrStr,
            ANY_INSTANCE, //port->portNumber,
            buf);
    }
}



// NAME:        SwitchPort_SchedulerFree
//
// PURPOSE:     Release scheduler and queue structure.
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchPort_SchedulerFree(
    Node* node,
    SwitchPort* port)
{
    int i = 0;
    int numQueues = 0;
    Scheduler* schedulerPtr = port->outPortScheduler;

    // Clear Output Queue
    Switch_ClearMessagesFromQueue(node, schedulerPtr);

    numQueues = (*schedulerPtr).numQueue();
    for (i = 0; i < numQueues; i++)
    {
        (*schedulerPtr).removeQueue(i);
    }

    delete schedulerPtr;

    // Clear Input Queue
    schedulerPtr = port->inPortScheduler;

    if (schedulerPtr)
    {
        Switch_ClearMessagesFromQueue(node, schedulerPtr);

        numQueues = (*schedulerPtr).numQueue();
        for (i = 0; i < numQueues; i++)
        {
            (*schedulerPtr).removeQueue(i);
        }

        delete schedulerPtr;
    }
}


// NAME:        Switch_PortsFinalize
//
// PURPOSE:     Call portwise finalize functions and
//              release port structure.
//
// PARAMETERS:
//
// RETURN:      None

static
void Switch_PortsFinalize(
    Node* node,
    MacSwitch* sw)
{
    SwitchPort* aPort = sw->firstPort;

    while (aPort)
    {
        SwitchPort_VlanFinalize(node, sw, aPort);

        if (node->macData[aPort->macData->interfaceIndex]->macStats
            && aPort->isPortStatEnabled)
        {
            SwitchPort_PrintStats(node, sw, aPort);
        }

        SwitchStp_PortFinalize(node, sw, aPort);

        if (node->macData[aPort->macData->interfaceIndex]->macStats)
        {
            SwitchPort_SchedulerPrintStats(node, aPort);
            Switch_QueuePrintStats(node, aPort);
        }

        SwitchPort_SchedulerFree(node, aPort);

        sw->firstPort = aPort->nextPort;
        MEM_free(aPort);
        aPort = sw->firstPort;
    }
}


// NAME:        Switch_DbFinalize
//
// PURPOSE:     Print filter database statistics and
//              release the filter database
//
// PARAMETERS:
//
// RETURN:      None

static
void Switch_DbFinalize(
    Node* node,
    MacSwitch* sw)
{
    if (node->macData[0]->macStats
        && sw->db->statsEnabled)
    {
        // Print stats
        SwitchDb_PrintStats(node, sw);
    }

    // Release database
    SwitchDb_DeleteAllEntries(sw->db, node->getNodeTime());
    MEM_free(sw->db);
}


// NAME:        Switch_BackplaneFinalize
//
// PURPOSE:     Print backplane statistics
//
// PARAMETERS:
//
// RETURN:      None

static
void Switch_BackplaneFinalize(Node* node, MacSwitch* sw)
{
    char buf[MAX_STRING_LENGTH];
    Scheduler* schedulerPtr = sw->cpuScheduler;

    if (sw->backplaneStatsEnabled)
    {
        sprintf(buf,
            "Frames dropped at Backplane = %u",
            sw->packetDroppedAtBackplane);

        IO_PrintStat(
            node,
            "Mac-Switch",
            "Backplane",
            ANY_DEST,
            ANY_INSTANCE,
            buf);
    }

    if (schedulerPtr)
    {
        int numQueues = (*schedulerPtr).numQueue();
        Switch_ClearMessagesFromQueue(node, schedulerPtr);

        for (int i = 0; i < numQueues; i++)
        {
            (*schedulerPtr).removeQueue(i);
        }

        delete schedulerPtr;
    }
}


// NAME:        Switch_Finalize
//
// PURPOSE:     Call different finalize functions and
//              release switch data.
//
// PARAMETERS:
//
// RETURN:      None

void Switch_Finalize(
    Node* node)
{
    MacSwitch* sw = Switch_GetSwitchData(node);

    SwitchStp_Finalize(node, sw);
    Switch_PortsFinalize(node, sw);
    Switch_BackplaneFinalize(node, sw);
    Switch_DbFinalize(node, sw);
    Switch_VlanFinalize(node, sw);

    MEM_free(sw);
}


//--------------------------------------------------------------------------
