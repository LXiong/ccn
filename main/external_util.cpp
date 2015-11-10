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

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#endif

#include "api.h"
#include "partition.h"
#include "ipv6.h"
#include "external.h"
#include "external_util.h"
#include "app_forward.h"
#include "WallClock.h"
#include "stats_net.h"
#include "if_ndp6.h"

// for tcp
#include "transport_tcp.h"
#include "tcpapps.h"
#include "app_util.h"

//#define DEBUG

#ifdef PARALLEL
#include "parallel.h"
#endif
#ifdef EXATA
#include "record_replay.h"
#endif

static void RotateNodeRight(EXTERNAL_Tree* splayPtr, EXTERNAL_TreeNode* nodePtr)
{
    EXTERNAL_TreeNode* nodeLeftPtr = nodePtr->leftPtr;

    nodePtr->leftPtr = nodeLeftPtr->rightPtr;

    if (nodeLeftPtr->rightPtr != NULL) {
        nodeLeftPtr->rightPtr->parentPtr = nodePtr;
    }

    nodeLeftPtr->rightPtr = nodePtr;
    nodeLeftPtr->parentPtr = nodePtr->parentPtr;

    if (nodePtr->parentPtr != 0) {
        if (nodePtr->parentPtr->leftPtr == nodePtr) {
            nodePtr->parentPtr->leftPtr = nodeLeftPtr;
        }
        else {
            nodePtr->parentPtr->rightPtr = nodeLeftPtr;
        }
    }

    nodePtr->parentPtr = nodeLeftPtr;
}


static void RotateNodeLeft(EXTERNAL_Tree* splayPtr, EXTERNAL_TreeNode* nodePtr)
{
    EXTERNAL_TreeNode* nodeRightPtr = nodePtr->rightPtr;

    nodePtr->rightPtr = nodeRightPtr->leftPtr;

    if (nodeRightPtr->leftPtr != NULL) {
        nodeRightPtr->leftPtr->parentPtr = nodePtr;
    }

    nodeRightPtr->leftPtr = nodePtr;
    nodeRightPtr->parentPtr = nodePtr->parentPtr;

    if (nodePtr->parentPtr != NULL) {
        if (nodePtr->parentPtr->leftPtr == nodePtr) {
            nodePtr->parentPtr->leftPtr = nodeRightPtr;
        }
        else {
            nodePtr->parentPtr->rightPtr = nodeRightPtr;
        }
    }

    nodePtr->parentPtr = nodeRightPtr;
}

static void EXTERNAL_TreeAtNode(EXTERNAL_Tree *tree, EXTERNAL_TreeNode* treeNode)
{
    EXTERNAL_TreeNode* parentPtr = treeNode->parentPtr;

    while ((parentPtr != NULL) && (parentPtr->parentPtr != NULL)) {
        EXTERNAL_TreeNode* grandParentPtr = parentPtr->parentPtr;

        if (grandParentPtr->leftPtr == parentPtr) {
            if (parentPtr->leftPtr == treeNode) {
                RotateNodeRight(tree, grandParentPtr);
                RotateNodeRight(tree, parentPtr);
            }
            else {
                RotateNodeLeft(tree, parentPtr);
                RotateNodeRight(tree, grandParentPtr);
            }
        }
        else {
            if (parentPtr->rightPtr == treeNode) {
                RotateNodeLeft(tree, grandParentPtr);
                RotateNodeLeft(tree, parentPtr);
            }
            else {
                RotateNodeRight(tree, parentPtr);
                RotateNodeLeft(tree, grandParentPtr);
            }
        }

        parentPtr = treeNode->parentPtr;
    }

    if (parentPtr != NULL) {
        if (parentPtr->leftPtr == treeNode) {
            RotateNodeRight(tree, parentPtr);
        }
        else {
            RotateNodeLeft(tree, parentPtr);
        }
    }

    tree->rootPtr = treeNode;
}

void EXTERNAL_TreeInitialize(
    EXTERNAL_Tree* tree,
    BOOL useStore,
    int maxStore)
{
    tree->heapPos = 0;
    tree->rootPtr = NULL;
    tree->useStore = useStore;
    tree->maxStore = maxStore;
    tree->store = NULL;
    tree->storeSize = 0;
}

void EXTERNAL_TreeInsert(EXTERNAL_Tree* tree, EXTERNAL_TreeNode* treeNode)
{
    BOOL newLeast = FALSE;

    if (tree->rootPtr == 0) {
        tree->rootPtr = treeNode;
        tree->leastPtr = treeNode;
        newLeast = TRUE;
    }
    else {
        BOOL itemInserted = FALSE;
        EXTERNAL_TreeNode* currentPtr = tree->rootPtr;

        while (itemInserted == FALSE) {
            if (treeNode->time < currentPtr->time) {
                if (currentPtr->leftPtr == NULL) {
                    itemInserted = TRUE;
                    currentPtr->leftPtr = treeNode;
                    if (currentPtr == tree->leastPtr) {
                        tree->leastPtr = treeNode;
                        newLeast = TRUE;
                    }
                }
                else {
                    currentPtr = currentPtr->leftPtr;
                }
            }
            else {
                if (currentPtr->rightPtr == NULL) {
                    itemInserted = TRUE;
                    currentPtr->rightPtr = treeNode;
                }
                else {
                    currentPtr = currentPtr->rightPtr;
                }
            }
        }

        treeNode->parentPtr = currentPtr;
        EXTERNAL_TreeAtNode(tree, treeNode);
    }

    /*if (newLeast == TRUE) {
        HeapSplayFixUp(&(node->partitionData->heapSplayTree),
                       node->splayTree.heapPos);
    }*/
}

EXTERNAL_TreeNode* EXTERNAL_TreePeekMin(EXTERNAL_Tree* tree)
{
    return tree->leastPtr;
}

EXTERNAL_TreeNode* EXTERNAL_TreeExtractMin(EXTERNAL_Tree* tree)
{
    EXTERNAL_TreeNode* outPtr;

    outPtr = tree->leastPtr;

    if (outPtr == NULL)
    {
        return NULL;
    }

    if (outPtr->parentPtr == NULL) {
        tree->rootPtr = outPtr->rightPtr;

        if (tree->rootPtr == NULL) {
            tree->leastPtr = NULL;
        }
        else {
            tree->rootPtr->parentPtr = NULL;
            tree->leastPtr = tree->rootPtr;

            while (tree->leastPtr->leftPtr != NULL) {
                tree->leastPtr = tree->leastPtr->leftPtr;
            }
        }
    }
    else {
        outPtr->parentPtr->leftPtr = outPtr->rightPtr;
        if (outPtr->rightPtr == 0) {
            tree->leastPtr = outPtr->parentPtr;
        }
        else {
            outPtr->rightPtr->parentPtr = outPtr->parentPtr;
            tree->leastPtr= outPtr->rightPtr;

            while (tree->leastPtr->leftPtr != NULL) {
                tree->leastPtr = tree->leastPtr->leftPtr;
            }
        }
    }

    /*if ((tree->leastPtr == NULL) ||
        (outPtr->timeValue != tree->leastPtr->timeValue)) {
        HeapSplayFixDown(&(node->partitionData->heapSplayTree),
                         node->splayTree.heapPos);
    }*/

    return outPtr;
}

EXTERNAL_TreeNode* EXTERNAL_TreeAllocateNode(EXTERNAL_Tree* tree)
{
    EXTERNAL_TreeNode* splayNodePtr;

    if (tree->store == NULL)
    {
        // If the store is empty then allocate a new node
        splayNodePtr =
            (EXTERNAL_TreeNode*) MEM_malloc(sizeof(EXTERNAL_TreeNode));
    }
    else
    {
        // If the store is not empty, then take a node from the store
        splayNodePtr = tree->store;
        tree->store = tree->store->rightPtr;
        tree->storeSize--;
    }

    assert(splayNodePtr != NULL);
    memset(splayNodePtr, 0, sizeof(EXTERNAL_TreeNode));
    return splayNodePtr;
}

void EXTERNAL_TreeFreeNode(EXTERNAL_Tree* tree, EXTERNAL_TreeNode* treeNode)
{
    if (!tree->useStore
        || (tree->storeSize != 0
            && tree->storeSize == EXTERNAL_MAX_TREE_STORE))
    {
        MEM_free(treeNode);
    }
    else
    {
        // Zero out tree node
        memset(treeNode, 0, sizeof(EXTERNAL_TreeNode));
        // Add treeNode to the store
        treeNode->rightPtr = tree->store;
        tree->store = treeNode;
        tree->storeSize++;
    }
}

//---------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------

// /**
// API       :: HashKey
// PURPOSE   :: This function comes up with a hash value for the given key.
//              It is usable for an interface's hash table.
// PARAMETERS::
// + key : char* : Pointer to the key (not necessarily ASCII)
// + keySize : int : Size of the key
// RETURN    :: int : The location of the key in the hash table
// **/
static int HashKey(char *key, int keySize)
{
    unsigned int hash = 0;
    int i;

    // Hash the key
    for (i = 0; i < keySize; i++)
    {
        hash = ((hash<<29) ^ (hash>>3)) ^ key[i] ^ (key[i]<<17);
    }

#ifdef DEBUG
    printf("key: \"");
    for (i = 0; i < keySize; i++)
    {
        if (isgraph(key[i]))
        {
            printf("%c", key[i]);
        }
        else
        {
            printf(" ");
        }
    }
    printf("\" hashes to %d\n", hash % EXTERNAL_MAPPING_TABLE_SIZE);
#endif

    return hash % EXTERNAL_MAPPING_TABLE_SIZE;
}

// /**
// API       :: CompareKeys
// PURPOSE   :: This function will compare 2 keys
// PARAMETERS::
// + key1 : char* : Pointer to the first key (not necessarily ASCII)
// + keySize1 : int : Size of the first key
// + key2 : char* : Pointer to the second key (not necessarily ASCII)
// + keySize2 : int : Size of the second key
// RETURN    :: BOOL : TRUE if the keys are identical, FALSE otherwise
// **/
static BOOL CompareKeys(char *key1, int keySize1, char *key2, int keySize2)
{
    // First check that the keys are the same size
    if (keySize1 != keySize2)
    {
        return FALSE;
    }

    // Compare keys 4 bytes at a time
    while (keySize1 >= 4)
    {
        if (*((Int32*) key1) != *((Int32*)key2))
        {
            return FALSE;
        }

        key1 += 4;
        key2 += 4;
        keySize1 -= 4;
    }

    // Compare next 2 bytes (if present)
    if (keySize1 >= 2)
    {
        if (*((short*) key1) != *((short*) key2))
        {

            return FALSE;
        }

        key1 += 2;
        key2 += 2;
        keySize1 -= 2;
    }

    // Compare final byte (if present)
    if (keySize1 == 1)
    {
        return *((char*) key1) == *((char*) key2);
    }

    // No differences, return TRUE
    return TRUE;
}

// /**
// API       :: AddMobilityEvent
// PURPOSE   :: This function will schedule a mobility event.
// PARAMETERS::
// + iface : EXTERNAL_Interface* : The external interface
// + node : Node* : The node that will receive the mobility event
// + mobilityEventTime : clocktype : The absolute simulation time (not delay)
// at which the mobility event should execute
// + coordinates : Coordinates : The coordinates to move the node to
// + orientation : Orientation : The orientation to point the node in
// + speed : double : The speed at which the node is moving in m/s
// + velocity : Velocity : The velocity vector of the node, in units of the
// coordinates parameter, per second
// + velocityOnly : BOOL : Indicates whether this a velocity-only event (no
// new information on the position or orientation)
// + delaySpeedCalculation : BOOL : Indicates whether the speed calculation
// should be delayed until mobilityEventTime
// RETURN    :: void
// **/

static void AddMobilityEvent(
    EXTERNAL_Interface* iface,
    Node* node,
    clocktype mobilityEventTime,
    Coordinates coordinates,
    Orientation orientation,
    double speed,
    Velocity velocity,
    BOOL velocityOnly,
    BOOL delaySpeedCalculation)
{
    Message* newMsg = MESSAGE_Alloc(node,
                                    EXTERNAL_LAYER,
                                    EXTERNAL_NONE,
                                    MSG_EXTERNAL_Mobility,
                                    iface->threaded);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(EXTERNAL_MobilityEvent));
    EXTERNAL_MobilityEvent* mobilityEvent
        = (EXTERNAL_MobilityEvent*) MESSAGE_ReturnInfo(newMsg);

    mobilityEvent->coordinates = coordinates;

    if (!velocityOnly)
    {

        if (node->mobilityData->groundNode == TRUE)
        {
            TerrainData* terrainData;
            terrainData = NODE_GetTerrainPtr(node);
            TERRAIN_SetToGroundLevel(
                terrainData, &(mobilityEvent->coordinates));
        }
    }

    mobilityEvent->orientation           = orientation;
    mobilityEvent->speed                 = speed;
    mobilityEvent->velocity              = velocity;
    mobilityEvent->truePositionTime      = mobilityEventTime;
    mobilityEvent->isTruePosition        = TRUE;
    mobilityEvent->velocityOnly          = velocityOnly;
    mobilityEvent->delaySpeedCalculation = delaySpeedCalculation;

    clocktype delay = mobilityEventTime - node->getNodeTime();
    assert(delay >= 0);
    EXTERNAL_MESSAGE_SendAnyNode(iface, node, newMsg, delay, EXTERNAL_SCHEDULE_SAFE);
}

// /**
// API       :: mymodi
// PURPOSE   :: This function implements the modulo function for integers.
//              It works for positive and negative values, but the divisor
//              must be greater than zero.  So mymodi(-361, 360) returns 359.
// PARAMETERS::
// + val : int : The value (quotiont)
// + div : int : The divisor
// RETURN    :: int : The remainder
// **/
static int mymodi(int val, int div)
{
    ERROR_Assert(div > 0, "Invalid divisor");

    if (val >= 0)
    {
        return val % div;
    }
    else
    {
        return val + (-val / div + 1) * div;
    }
}

static void CheckExternalMobilityPointers(
    const EXTERNAL_Interface* iface,
    const Node* node)
{
    ERROR_Assert(iface != NULL, "Invalid interface pointer");
    ERROR_Assert(node != NULL, "Invalid node pointer");
    ERROR_Assert(iface->interfaceList != NULL, "Invalid interface");
}

static void ModOrientation(
    OrientationType &azimuth,
    OrientationType &elevation)
{
    if (azimuth < 0 || azimuth >= 360)
    {
        ERROR_ReportWarning("Azimuth out of range");
        azimuth = (OrientationType) mymodi(azimuth, 360);
    }

    if (elevation < -180 || elevation > 180)
    {
        ERROR_ReportWarning("Elevation out of range");
        elevation = (OrientationType) (mymodi(elevation + 180, 360) - 180);
    }
}

static void BoundMobilityEventTime(
    const EXTERNAL_Interface *iface,
    const Node *node,
    clocktype& mobilityEventTime)
{
    ERROR_Assert(iface != NULL, "Invalid interface pointer");
    ERROR_Assert(node != NULL, "Invalid node pointer");

    const clocktype simTime = node->getNodeTime();

    clocktype newMobilityEventTime = MAX(mobilityEventTime, simTime + 1);

#ifdef PARALLEL //Parallel
    if (iface->partition->safeTime != CLOCKTYPE_MAX)
    {
        if (newMobilityEventTime < iface->partition->safeTime)
        {
            newMobilityEventTime = iface->partition->safeTime;
        }
    }
#endif

    if (mobilityEventTime != newMobilityEventTime)
    {
        mobilityEventTime = newMobilityEventTime;
    }
}

static void BoundSpeed(double& speed)
{
    if (speed < 0.0)
    {
        ERROR_ReportWarning("Speed out of range (negative value)");
        speed = 0.0;
    }
}

//---------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------

void EXTERNAL_InitializeTable(
    EXTERNAL_Table *table,
    int size)
{
    int i;

    // Set the table size
    table->size = size;

    // Allocate memory for packet table
    table->table = (EXTERNAL_TableRecord*)
        MEM_malloc(sizeof(EXTERNAL_TableRecord) * size);
    memset(table->table, 0, sizeof(EXTERNAL_TableRecord) * size);

    // Set the used record list to be empty (NULL) and the empty record list
    // to point to the first record (which will be a list of all the records)
    table->usedRecordList = NULL;
    table->emptyRecordList = &table->table[0];

    // Initialize the first node, and the last if there is more than one node
    table->table[0].prev = NULL;
    if (size > 1)
    {
        table->table[0].next = &table->table[1];
        table->table[size - 1].next = NULL;
        table->table[size - 1].prev = &table->table[size - 2];
    }
    else
    {
        table->table[0].next = NULL;
    }

    // Initialize the in-between nodes
    for (i = 1; i < size - 2; i++)
    {
        table->table[i].next = &table->table[i + 1];
        table->table[i].prev = &table->table[i - 1];
    }
}

void EXTERNAL_FinalizeTable(EXTERNAL_Table *table)
{
    // Free the memory used by the record table
    MEM_free(table->table);

    // Set contents of table to zero
    memset(table, 0, sizeof(EXTERNAL_Table));
}

EXTERNAL_TableRecord* EXTERNAL_GetUnusedRecord(EXTERNAL_Table *table)
{
    EXTERNAL_TableRecord *rec;
    int i;

    // If there is an empty record available
    if (table->emptyRecordList != NULL)
    {
        // Use the first empty record
        rec = table->emptyRecordList;

        // Remove it from the empty record list
        table->emptyRecordList = rec->next;
        if (table->emptyRecordList != NULL)
        {
            table->emptyRecordList->prev = NULL;
        }
        rec->next = NULL;

        // Add it to the beginning of the full list
        if (table->usedRecordList == NULL)
        {
            table->usedRecordList = rec;
            rec->prev = NULL;
            rec->next = NULL;
        }
        else
        {
            rec->next = table->usedRecordList;
            rec->next->prev = rec;
            rec->prev = NULL;
            table->usedRecordList = rec;
        }
    }
    else
    {
        // We ran out of free records.  Create an overflow table twice the
        // size of the last overflow table (if it exists) or twice the size
        // of the table.  Add all the newly created records to the unused
        // list for the table.

        // Allocate memory for the overflow table
        EXTERNAL_TableOverflow* overflow;
        overflow = (EXTERNAL_TableOverflow*)
            MEM_malloc(sizeof(EXTERNAL_TableOverflow));

        // Determine the size of the new overflow table
        if (table->overflow == NULL)
        {
            overflow->size = table->size * 2;
        }
        else
        {
            overflow->size = table->overflow->size * 2;
        }

        // Allocate space for records
        overflow->records = (EXTERNAL_TableRecord*)
            MEM_malloc(sizeof(EXTERNAL_TableRecord) * overflow->size);

        // Choose the 0th record allocated.  This record will be returned to
        // the calling function.  Add it to the beginning of the full list.
        rec = &overflow->records[0];
        rec->next = table->usedRecordList;
        rec->next->prev = rec;
        rec->prev = NULL;
        table->usedRecordList = rec;

        // Add the rest of the records of this overflow table to the table's
        // empty record list
        table->emptyRecordList = &overflow->records[1];

        // Initialize the second and last node (if there are more than
        // 2 nodes)
        overflow->records[1].prev = NULL;
        if (overflow->size > 2)
        {
            overflow->records[1].next = &overflow->records[2];
            overflow->records[overflow->size - 1].next = NULL;
            overflow->records[overflow->size - 1].prev =
                &overflow->records[overflow->size - 2];
        }
        else
        {
            overflow->records[1].next = NULL;
        }

        // Initialize the in-between nodes
        // record except the very last record in the array
        for (i = 2; i < overflow->size - 1; i++)
        {
            overflow->records[i].next = &overflow->records[i + 1];
            overflow->records[i].prev = &overflow->records[i - 1];
        }

        // Add the new overflow table to the front of the overflow list
        overflow->next = table->overflow;
        table->overflow = overflow;
    }

    // Return the record
    return rec;
}

EXTERNAL_TableRecord* EXTERNAL_GetEarliestRecord(EXTERNAL_Table *table)
{
    EXTERNAL_TableRecord *rec;
    EXTERNAL_TableRecord *earliestRec;

    // If the table is empty return NULL
    if (table->usedRecordList == NULL)
    {
        return NULL;
    }

    //printf("list:\n");

    // Return the earliest record
    rec = table->usedRecordList;
    earliestRec = rec;
    while (rec != NULL)
    {
        if (rec->time < earliestRec->time)
        {
            earliestRec = rec;
        }

        rec = rec->next;
    }

    return earliestRec;
}

BOOL EXTERNAL_IsDataInTable(EXTERNAL_Table *table, char *data)
{
    EXTERNAL_TableRecord *rec;

    // Loop through the table looking for the data
    rec = table->usedRecordList;
    while (rec != NULL)
    {
        // If found, return true
        if (rec->data == data)
        {
            return TRUE;
        }

        rec = rec->next;
    }

    // Return false
    return FALSE;
}

void EXTERNAL_FreeRecord(EXTERNAL_Table *table, EXTERNAL_TableRecord *rec)
{
    // Remove the record from the used record list
    if (table->usedRecordList == rec)
    {
        table->usedRecordList = rec->next;
        if (table->usedRecordList != NULL)
        {
            table->usedRecordList->prev = NULL;
        }
    }
    if (rec->prev != NULL)
    {
        rec->prev->next = rec->next;
    }
    if (rec->next != NULL)
    {
        rec->next->prev = rec->prev;
    }

    // Add the record to the beginning of the empty record list
    memset(rec, 0, sizeof(EXTERNAL_TableRecord));
    rec->next = table->emptyRecordList;
    if (rec->next != NULL)
    {
        rec->next->prev = rec;
    }
    table->emptyRecordList = rec;
}

void EXTERNAL_SendDataAppLayerUDP(
    EXTERNAL_Interface *iface,
    NodeAddress from,
    NodeAddress to,
    char *data,
    int dataSize,
    clocktype timestamp,
    AppType app,
    TraceProtocolType trace,
    TosType priority,
    UInt8 ttl)
{
    NodeAddress nodeId;
    Node *srcNode = NULL;
    int virtualSize;

    // Get source node
    // GetNodeIdFromInterfaceAddress will work for local and remote nodes
    nodeId = MAPPING_GetNodeIdFromInterfaceAddress(
        iface->partition->firstNode,
        from);
    assert(nodeId != INVALID_MAPPING);
    srcNode = MAPPING_GetNodePtrFromHash(
        iface->partition->nodeIdHash,
        nodeId);
#ifdef PARALLEL //Parallel
    if (srcNode == NULL)
    {
        // The node Id might be for a node on a remote partition.
        srcNode = MAPPING_GetNodePtrFromHash(
            iface->partition->remoteNodeIdHash, nodeId);
    }
#endif //Parallel
    assert(srcNode != NULL);

    // If no data is given everything is virtual.  If data is given then
    // nothing is virtual.
    if (data == NULL)
    {
        virtualSize = dataSize;
        dataSize = 0;
    }
    else
    {
        virtualSize = 0;
    }

    EXTERNAL_SendVirtualDataAppLayerUDP(
        iface,
        srcNode,
        from,
        to,
        data,
        dataSize,
        virtualSize,
        timestamp,
        app,
        trace,
        priority,
        ttl);
}

// src and dest Nodes can be nodes on remote partitions
void EXTERNAL_SendDataAppLayerUDP(
    EXTERNAL_Interface *iface,
    Node* srcNode,
    NodeAddress srcAddress,
    NodeAddress destAddress,
    char *data,
    int dataSize,
    clocktype timestamp,
    AppType app,
    TraceProtocolType trace,
    TosType priority,
    UInt8 ttl)
{
    int virtualSize;

    // If no data is given everything is virtual.  If data is given then
    // nothing is virtual.
    if (data == NULL)
    {
        virtualSize = dataSize;
        dataSize = 0;
    }
    else
    {
        virtualSize = 0;
    }

    EXTERNAL_SendVirtualDataAppLayerUDP(
        iface,
        srcNode,
        srcAddress,
        destAddress,
        data,
        dataSize,
        virtualSize,
        timestamp,
        app,
        trace,
        priority,
        ttl);
}

// For Virtual Data
void EXTERNAL_SendVirtualDataAppLayerUDP(
    EXTERNAL_Interface *iface,
    NodeAddress from,
    NodeAddress to,
    char *header,
    int headerSize,
    int virtualDataSize,
    clocktype timestamp,
    AppType app,
    TraceProtocolType trace,
    TosType priority,
    UInt8 ttl)
{
    NodeAddress nodeId;
    Node *srcNode = NULL;

    // Get source node
    // GetNodeIdFromInterfaceAddress will work for local and remote nodes
    nodeId = MAPPING_GetNodeIdFromInterfaceAddress(
        iface->partition->firstNode,
        from);
    assert(nodeId != INVALID_MAPPING);
    srcNode = MAPPING_GetNodePtrFromHash(
        iface->partition->nodeIdHash,
        nodeId);
#ifdef PARALLEL //Parallel
    if (srcNode == NULL)
    {
        // The node Id might be for a node on a remote partition.
        srcNode = MAPPING_GetNodePtrFromHash(
            iface->partition->remoteNodeIdHash, nodeId);
    }
#endif //Parallel
    assert(srcNode != NULL);


    // Now send the packet
    EXTERNAL_SendVirtualDataAppLayerUDP(
        iface,
        srcNode,
        from,
        to,
        header,
        headerSize,
        virtualDataSize,
        timestamp,
        app,
        trace,
        priority,
        ttl);
}


// VIRTUAL DATA src and dest Nodes can be nodes on remote partitions
void EXTERNAL_SendVirtualDataAppLayerUDP(
    EXTERNAL_Interface *iface,
    Node* srcNode,
    NodeAddress srcAddress,
    NodeAddress destAddress,
    char *header,
    int headerLength,
    int virtualDataSize,
    clocktype timestamp,
    AppType app,
    TraceProtocolType trace,
    TosType priority,
    UInt8 ttl)
{
    clocktype delay;

    // Determine the delay for this message
    if (timestamp <= EXTERNAL_QuerySimulationTime(iface))
    {
        // This case also handles timestamp == 0
        delay = 0;
    }
    else
    {
        delay = timestamp - EXTERNAL_QuerySimulationTime(iface);
    }

    if (app == APP_FORWARD)
    {
        Message* sendUdpDataMsg;                                                      
        EXTERNAL_ForwardSendUdpData* sendData;
                                                                                          
        // Create a sending data message used with app forward
        sendUdpDataMsg = MESSAGE_Alloc(
            srcNode,
            APP_LAYER,
            APP_FORWARD,
            MSG_EXTERNAL_ForwardSendUdpData);

        // Allocate data packet no matter size
        MESSAGE_PacketAlloc(
            iface->partition->firstNode,
            sendUdpDataMsg,
            headerLength,
            trace);

        // Copy the given data to the message's data packet, if present
        if (headerLength > 0)
        {
            ERROR_Assert(header != NULL, "header should not be NULL");
            memcpy(MESSAGE_ReturnPacket(sendUdpDataMsg), header, headerLength);
        }
                                                                                          
        // Add virtual data if present
        if (virtualDataSize > 0)
        {
            MESSAGE_AddVirtualPayload(srcNode, sendUdpDataMsg, virtualDataSize);
        }
                                                                                          
        // Info field for sending Data msg
        MESSAGE_InfoAlloc(
            srcNode,
            sendUdpDataMsg,
            sizeof(EXTERNAL_ForwardSendUdpData));
        sendData = (EXTERNAL_ForwardSendUdpData*) MESSAGE_ReturnInfo(sendUdpDataMsg);
        sendData->localAddress = srcAddress;
        sendData->remoteAddress = destAddress;
        strcpy(sendData->interfaceName, iface->name);
        sendData->interfaceId = iface->interfaceId;
        sendData->priority = priority;
        sendData->ttl = ttl;
        sendData->messageSize = (UInt8)MESSAGE_ReturnPacketSize(sendUdpDataMsg);
        sendData->isVirtual = virtualDataSize > 0;

#ifdef DEBUG
        printf("now %Ld: EXTERNAL sending UDP packet, node %d to send (%d bytes) start delayed by %Ld\n",
            EXTERNAL_QuerySimulationTime(iface), srcNode->nodeId,
            MESSAGE_ReturnPacketSize(sendUdpDataMsg),
            delay);
        fflush (stdout);
#endif
        if (!EXTERNAL_IsInWarmup(iface))
        {
            EXTERNAL_MESSAGE_SendAnyNode(
                iface,
                srcNode,
                sendUdpDataMsg,
                delay,
                EXTERNAL_SCHEDULE_LOOSELY);
        }
        else
        {
            BOOL wasPktDropped;
            EXTERNAL_SendDataDuringWarmup (iface, srcNode, 
                MESSAGE_Duplicate(srcNode, sendUdpDataMsg), &wasPktDropped);
            if (wasPktDropped)
            {
                MESSAGE_Free(srcNode, sendUdpDataMsg);
            }
        }
    }
    else
    {
        Message *msg;
        AppToUdpSend *info;

        // Allocate memory for the message and data packet
        msg = MESSAGE_Alloc(
            srcNode,
            TRANSPORT_LAYER,
            TransportProtocol_UDP,
            MSG_TRANSPORT_FromAppSend,
            iface->threaded);

        // Allocate data packet no matter size
        MESSAGE_PacketAlloc(
            srcNode,
            msg,
            headerLength,
            trace);

        // Copy the given data to the message's data packet, if present
        if (header != NULL)
        {
            memcpy(MESSAGE_ReturnPacket(msg), header, headerLength);
        }
        else
        {
            memset(MESSAGE_ReturnPacket(msg), 0, headerLength);
        }

        // Create info for the transport protocol
        MESSAGE_InfoAlloc(
            srcNode,
            msg,
            sizeof(AppToUdpSend));
        info = (AppToUdpSend *) MESSAGE_ReturnInfo(msg);

        SetIPv4AddressInfo(&info->sourceAddr, srcAddress);

        // Set source port to be unique interface id
        info->sourcePort = (short)iface->interfaceId;

        SetIPv4AddressInfo(&info->destAddr, destAddress);
        info->destPort = (short) app;

        info->priority = priority;
        info->outgoingInterface = ANY_INTERFACE;
        info->ttl = ttl;

#ifdef DEBUG
        printf("now %Ld: FORWARD UDP MSG_TRANSPORT_FromAppSend sent to node %d to send (%d bytes) start delayed by %Ld\n",
            EXTERNAL_QuerySimulationTime(iface), srcNode->nodeId,
            MESSAGE_ReturnPacketSize(msg),
            delay);
        fflush (stdout);
#endif

        // Send the message with the given delay.
        // We must delay at least one clock tick so that the instantiate messages
        // can be delivered first.
        if (delay == 0)
        {
            delay = 1;
        }
        EXTERNAL_MESSAGE_SendAnyNode (iface, srcNode, msg, delay,
            EXTERNAL_SCHEDULE_LOOSELY);
    }
}

// This function will handle from and to addresses for nodes that are
// on remote partitions or the local partition.
void EXTERNAL_SendDataAppLayerTCP(
    EXTERNAL_Interface *iface,
    NodeAddress from,
    NodeAddress to,
    char *data,
    int dataSize,
    clocktype timestamp,
    UInt8 ttl)
{
    // Use the send virtual data function with a virtual size of 0
    EXTERNAL_SendVirtualDataAppLayerTCP(
        iface,
        from,
        to,
        data,
        dataSize,
        0,
        timestamp,
        ttl);
}

void EXTERNAL_SendVirtualDataAppLayerTCP(
    EXTERNAL_Interface *iface,
    NodeAddress from,
    NodeAddress to,
    char *header,
    int dataSize,
    int virtualSize,
    clocktype timestamp,
    UInt8 ttl)
{
    NodeAddress fromId;
    Node *fromNode;

    // Get originating node
    fromId = MAPPING_GetNodeIdFromInterfaceAddress(
        iface->partition->firstNode,
        from);
    assert(fromId != INVALID_MAPPING);
    fromNode = MAPPING_GetNodePtrFromHash(
        iface->partition->nodeIdHash,
        fromId);
#ifdef PARALLEL //Parallel
    if (fromNode == NULL)
    {
        // The source is on a remote partition.
        fromNode = MAPPING_GetNodePtrFromHash(
            iface->partition->remoteNodeIdHash,
            fromId);
    }
#endif //Parallel
    assert(fromNode != NULL);

    // We send a Qualnet Message APP_FORWARD that will be processed
    // at the source node for injecting TCP traffic.
    // Any delay for the start of sending TCP data will be
    // modeled via this cross-partition message.
    // The forward application will receive this cross-partition
    // message and then call AppLayerForwardSendTcpData ()

    Message* msg = MESSAGE_Alloc(
        fromNode,
        APP_LAYER,
        APP_FORWARD,
        MSG_EXTERNAL_ForwardSendTcpData,
        iface->threaded);

    // Create packet data.  At the very least this will contain a Forward
    // app TCP header.  Add an other actual data after the Forward TCP
    // Header.
    MESSAGE_PacketAlloc(
        fromNode,
        msg,
        dataSize,
        TRACE_FORWARD);

    // Copy packet data if some data was provided
    if (dataSize > 0)
    {
        ERROR_Assert(header != NULL, "header should not be NULL");
        memcpy(
            MESSAGE_ReturnPacket(msg),
            header,
            dataSize);
    }

    // Add virtual payload if specified
    if (virtualSize > 0)
    {
        MESSAGE_AddVirtualPayload(fromNode, msg, virtualSize);
    }

    // Fill in send info for forward app.  This contains enough information
    // for the forward app to know where to send the data
    EXTERNAL_ForwardSendTcpData* sendInfo =
        (EXTERNAL_ForwardSendTcpData *) MESSAGE_InfoAlloc(
            fromNode,
            msg,
            sizeof(EXTERNAL_ForwardSendTcpData));
    sendInfo->fromAddress = from;
    sendInfo->toAddress = to;
    strcpy(sendInfo->interfaceName, iface->name);
    sendInfo->interfaceId = iface->interfaceId;
    sendInfo->ttl = ttl;

    // Determine delay
    clocktype delay;
    clocktype now;
    now = EXTERNAL_QuerySimulationTime(iface);
    if (timestamp < now)
    {
        delay = 0;
    }
    else
    {
        delay = timestamp - now;
    }

    // Send the message.
    if (!EXTERNAL_IsInWarmup(iface))
    {
        EXTERNAL_MESSAGE_SendAnyNode(iface, fromNode, msg, delay,
            EXTERNAL_SCHEDULE_LOOSELY);
    }
    else
    {
        BOOL wasPktDropped;
        EXTERNAL_SendDataDuringWarmup(iface, fromNode, MESSAGE_Duplicate(fromNode, msg),
            &wasPktDropped);
        if (wasPktDropped)
        {
            MESSAGE_Free(fromNode, msg);
        }
    }
}

static Message* EXTERNAL_CreateSendNetworkLayerData(
    EXTERNAL_Interface* iface,
    NodeAddress srcAddr,
    NodeAddress destAddr,
    TosType tos,
    UInt8 protocol,
    UInt8 ttl,
    char* payload,
    int payloadSize,
    int ipHeaderLength,
    char *ipOptions,
    NodeAddress from = 0)
{
    Message* sendMessage;
    EXTERNAL_NetworkLayerPacket* truePacket;

    sendMessage = MESSAGE_Alloc(
        iface->partition->firstNode,
        EXTERNAL_LAYER,
        0,
        MSG_EXTERNAL_SendPacket,
        iface->threaded);
#ifdef EXATA
        sendMessage->isEmulationPacket = true;
#endif

    // Add EXTERNAL_NetworkLayerPacket info to send message
    truePacket = (EXTERNAL_NetworkLayerPacket*) MESSAGE_AddInfo(
        iface->partition->firstNode,
        sendMessage,
        sizeof(EXTERNAL_NetworkLayerPacket),
        INFO_TYPE_ExternalData);

    //Handling the case when the source of the packet is not a node in QualNet
    truePacket->srcAddr.networkType = NETWORK_IPV4;
    if (from)
    {
      truePacket->srcAddr.interfaceAddr.ipv4 = from;
    }
    else
    {
      truePacket->srcAddr.interfaceAddr.ipv4 = srcAddr;
    }

    truePacket->externalId = iface->interfaceId;

    // Create the data portion of the packet.  If the payload is NULL
    // then send 0's.
    if (protocol == IPPROTO_UDP)
    {
        MESSAGE_PacketAlloc(
            iface->partition->firstNode,
            sendMessage,
            payloadSize,
            TRACE_UDP);

//#ifdef HITL_INTERFACE
        if (NetworkIpIsMulticastAddress(iface->partition->firstNode, destAddr))
        {
            sendMessage->originatingProtocol= TRACE_MCBR;
        }
//#endif
    }
    else if (protocol == IPPROTO_ICMP)
    {
        MESSAGE_PacketAlloc(
            iface->partition->firstNode,
            sendMessage,
            payloadSize,
            TRACE_ICMP);
    }
    else
    {
        MESSAGE_PacketAlloc(
            iface->partition->firstNode,
            sendMessage,
            payloadSize,
            TRACE_FORWARD);
    }

    if (payload != NULL)
    {
        memcpy(MESSAGE_ReturnPacket(sendMessage), payload, payloadSize);
    }
    else
    {
        memset(MESSAGE_ReturnPacket(sendMessage), 0, payloadSize);
    }

    // Create the IP header
    if ((unsigned)ipHeaderLength > sizeof(IpHeaderType))
    {
        NetworkIpAddHeaderWithOptions(
            iface->partition->firstNode,
            sendMessage,
            srcAddr,
            destAddr,
            tos,
            protocol,
            ttl,
            ipHeaderLength,
            ipOptions);
    }
    else
    {
    NetworkIpAddHeader(
        iface->partition->firstNode,
        sendMessage,
        srcAddr,
        destAddr,
        tos,
        protocol,
        ttl);
    }

    return sendMessage;
}

static Message* EXTERNAL_CreateSendIpv6NetworkLayerData(
    EXTERNAL_Interface* iface,
    const Address* srcAddr,
    const Address* destAddr,
    TosType tos,
    UInt8 protocol,
    UInt8 ttl,
    char* payload,
    int payloadSize,
    int ipHeaderLength,
    const Address* from = NULL)
{
    Message* sendMessage = NULL;
    EXTERNAL_NetworkLayerPacket* truePacket;
    UInt8 proto = 0;
    ipv6_fraghdr* frgpHeader = NULL;
    int size = 0;

    sendMessage = MESSAGE_Alloc(iface->partition->firstNode,
                                EXTERNAL_LAYER,
                                0,
                                MSG_EXTERNAL_SendPacket,
                                iface->threaded);
#ifdef EXATA
    sendMessage->isEmulationPacket = true;
#endif

    // Add EXTERNAL_NetworkLayerPacket info to send message
    truePacket = (EXTERNAL_NetworkLayerPacket*) MESSAGE_AddInfo(
                                    iface->partition->firstNode,
                                    sendMessage,
                                    sizeof(EXTERNAL_NetworkLayerPacket),
                                    INFO_TYPE_ExternalData);

    // Handling the case when the source of the packet is not a node in QualNet
    if (from)
    {
        truePacket->srcAddr = *from;
    }
    else
    {
        truePacket->srcAddr = *srcAddr;
    }

    truePacket->externalId = iface->interfaceId;

    proto = protocol;
    size = payloadSize;
    // Create the data portion of the packet.  If the payload is NULL
    // then send 0's.
    if (protocol == IP6_NHDR_FRAG)
    {
        frgpHeader = (ipv6_fraghdr*)payload;
        payloadSize -= sizeof(ipv6_fraghdr);
        payload = payload + sizeof(ipv6_fraghdr);
        protocol = (UInt8)frgpHeader->if6_nh;
        proto = IP6_NHDR_FRAG;
    }

    if (protocol == IPPROTO_UDP)
    {
        MESSAGE_PacketAlloc(
            iface->partition->firstNode,
            sendMessage,
            payloadSize,
            TRACE_UDP);

        if (IS_MULTIADDR6(destAddr->interfaceAddr.ipv6))
        {
            sendMessage->originatingProtocol= TRACE_MCBR;
        }
    }
    else if (protocol == IPPROTO_ICMPV6)
    {
        MESSAGE_PacketAlloc(iface->partition->firstNode,
                            sendMessage,
                            payloadSize,
                            TRACE_ICMPV6);
    }
    else
    {
        MESSAGE_PacketAlloc(iface->partition->firstNode,
                            sendMessage,
                            payloadSize,
                            TRACE_FORWARD);
    }

    if (payload != NULL)
    {
        memcpy(MESSAGE_ReturnPacket(sendMessage), payload, payloadSize);
    }
    else
    {
        memset(MESSAGE_ReturnPacket(sendMessage), 0, payloadSize);
    }

    if (proto == IP6_NHDR_FRAG)
    {
        Ipv6AddFragmentHeader(iface->partition->firstNode,
                              sendMessage,
                              frgpHeader->if6_nh,
                              frgpHeader->if6_off,
                              frgpHeader->if6_id);
    }
    Ipv6AddIpv6Header(iface->partition->firstNode,
                      sendMessage,
                      size,
                      srcAddr->interfaceAddr.ipv6,
                      destAddr->interfaceAddr.ipv6,
                      tos,
                      proto,
                      ttl);

    return sendMessage;
}

// /**
// API       :: EXTERNAL_AddHdr
// PURPOSE   :: This function will create qualnet in6_addr and add ipv6
//              header to message
// PARAMETERS::
// + node : Node* : Node pointer
// + sendMessage : Message* : Message on which ipv6 header needs to be added
// + payloadSize : int : Size of payload in packet
// + src : UInt8* : IPv6 Source address
// +.dst : UInt8* : IPv6 destination address
// + tos : TosType : Packet Priority
// + protocol : unsigned char : Protcol after ipv6 header
// + ttl : unsigned : Hop limit
// RETURN    :: void
// **/
void EXTERNAL_AddHdr(
    Node* node,
    Message* sendMessage,
    int payloadSize,
    UInt8* src,
    UInt8* dst,
    TosType tos,
    unsigned char protocol,
    unsigned ttl)
{
    in6_addr src_addr,dst_addr;
    for (int i = 0; i < 16; i++)
    {
        src_addr.s6_addr[i] = src[i];
        dst_addr.s6_addr[i] = dst[i];
    }
    Ipv6AddIpv6Header(node,
                      sendMessage,
                      payloadSize,
                      src_addr,
                      dst_addr,
                      tos,
                      protocol,
                      ttl);
}

void EXTERNAL_SendDataNetworkLayer(
    EXTERNAL_Interface* iface,
    NodeAddress from,
    NodeAddress srcAddr,
    NodeAddress destAddr,
    TosType tos,
    unsigned char protocol,
    unsigned int ttl,
    char* payload,
    int payloadSize,
    clocktype timestamp)
{
    NodeAddress nodeId;
    Node* node;
    Message* sendMessage;

    // Get source node
    nodeId = MAPPING_GetNodeIdFromInterfaceAddress(
        iface->partition->firstNode,
        from);
    assert(nodeId != INVALID_MAPPING);
    node = MAPPING_GetNodePtrFromHash(
        iface->partition->nodeIdHash,
        nodeId);
#ifdef PARALLEL //Parallel
    if (node == NULL)
    {
        // The source is on a remote partition.
        node = MAPPING_GetNodePtrFromHash(
            iface->partition->remoteNodeIdHash,
            nodeId);
    }
#endif //Parallel
    assert(node != NULL);

    // Now create a message to send to the external interface API that will
    // generate a packet.  This will add the delay and take care of
    // parallel issues.  This message will be re-sent as an IP packet.
    sendMessage = EXTERNAL_CreateSendNetworkLayerData(
        iface,
        srcAddr,
        destAddr,
        tos,
        protocol,
        (UInt8) ttl,
        payload,
        payloadSize,
        0,
        NULL,
        from);

    // Determine the delay for this message.  This code considers that the
    // timestamp could be 0 meaning no delay.
    clocktype delay;
    if (timestamp < EXTERNAL_QuerySimulationTime(iface))
    {
        delay = 0;
    }
    else
    {
        delay = timestamp - EXTERNAL_QuerySimulationTime(iface);
    }

    if (!EXTERNAL_IsInWarmup(iface))
    {
        // Send the message.  Will be processed by IPNESendTrueEmulationPacket.
        EXTERNAL_MESSAGE_SendAnyNode(
            iface,
            node,
            sendMessage,
            delay,
            EXTERNAL_SCHEDULE_LOOSELY);
    }
    else
    {
        BOOL wasPktDropped;
        EXTERNAL_SendDataDuringWarmup (iface, node, 
            MESSAGE_Duplicate(node, sendMessage), &wasPktDropped);
        if (wasPktDropped)
        {
             MESSAGE_Free(node, sendMessage);
        }
    }
}

// /**
// API       :: EXTERNAL_SendIpv6DataNetworkLayer
// PURPOSE   :: Sends ipv6 data originating from network layer.No provisions
//              are made for handling this data once it enters the QualNet
//              network.  This is the responsibility of the external
//              interface or protocols the data is sent to.
// PARAMETERS::
// + iface : EXTERNAL_Interface* : The external interface
// + from : Address : The address of the node that will send the
//                    message. Where the packet is sent from within the
//                       QualNet simulation.
// + srcAddr : Address : The IP address of the node originally
//                       creating the packet. May be different than
//                       the from address.
// + destAddr : Address : The address of the receiving node
// + tos : TosType : The Type of Service field in the IPv6 header
// + protocol : unsigned char : The protocol field in the IPv6 header
// + hlim : unsigned int : The hop limit field in the IPv6 header
// + payload : char* : The data that is to be sent. This should include
//                     appropriate transport headers. If NULL the payload
//                     will consist of 0s.
// + payloadSize : int : The size of the data
// + timestamp : clocktype : The time to send this packet. Pass 0 to send
//                           at the interface's current time according to
//                           the interface's time function (If the interface
//                           doesn't have a time function, the packet is
//                           sent immediately.
// RETURN    :: void : None
// **/
void EXTERNAL_SendIpv6DataNetworkLayer(
    EXTERNAL_Interface* iface,
    const Address* from,
    const Address* srcAddr,
    const Address* destAddr,
    TosType tos,
    unsigned char protocol,
    unsigned hlim,
    char* payload,
    Int32 payloadSize,
    clocktype timestamp)
{
    Int32 nodeId = 0;
    Node* node = NULL;
    Message* sendMessage = NULL;
    bool found = false;

    // Get source node
    nodeId = MAPPING_GetNodeIdFromInterfaceAddress(iface->partition->firstNode,
                                                   *from);
    ERROR_Assert(nodeId != INVALID_MAPPING, "Invalid Mapping");

    found = PARTITION_ReturnNodePointer(iface->partition,
                                        &node,
                                        nodeId,
                                        TRUE);

    ERROR_Assert(node != NULL,"Node pointer should not be NULL");

    /* Now create a message to send to the external interface API that will
     * generate a packet.  This will add the delay and take care of
     * parallel issues.  This message will be re-sent as an IP packet.
     */

    sendMessage = EXTERNAL_CreateSendIpv6NetworkLayerData(
                    iface,
                    srcAddr,
                    destAddr,
                    tos,
                    protocol,
                    (UInt8)hlim,
                    payload,
                    payloadSize,
                    0,
                    from);

    /* Determine the delay for this message.  This code considers that the
     * timestamp could be 0 meaning no delay.
     */

    clocktype delay = 0;
    if (timestamp < EXTERNAL_QuerySimulationTime(iface))
    {
        delay = 0;
    }
    else
    {
        delay = timestamp - EXTERNAL_QuerySimulationTime(iface);
    }
    
#ifdef EXATA
     iface->partition->rrInterface->RecordHandleIPv6SniffedPacket(
                                        *from,  //this is the from address
                                        delay,
                                        *srcAddr,
                                        *destAddr,
                                        tos,
                                        protocol,
                                        hlim,
                                        payload,
                                        payloadSize);
#endif
    if (!EXTERNAL_IsInWarmup(iface))
    {
        // Send the message.  Will be processed by IPNESendTrueEmulationPacket.
        EXTERNAL_MESSAGE_SendAnyNode(iface,
                                     node,
                                     sendMessage,
                                     delay,
                                     EXTERNAL_SCHEDULE_LOOSELY);
    }
    else
    {
        BOOL wasPktDropped = FALSE;
        EXTERNAL_SendDataDuringWarmup (iface, node, 
            MESSAGE_Duplicate(node, sendMessage), &wasPktDropped);
        if (wasPktDropped)
        {
             MESSAGE_Free(node, sendMessage);
        }
    }
}

void EXTERNAL_SendVirtualDataNetworkLayer(
    EXTERNAL_Interface* iface,
    NodeAddress from,
    NodeAddress srcAddr,
    NodeAddress destAddr,
    TosType tos,
    unsigned char protocol,
    unsigned int ttl,
    char* payload,
    int dataSize,
    int virtualSize,
    clocktype timestamp)
{
    NodeAddress nodeId;
    Node* node;
    Message* sendMessage;

    // Get source node
    nodeId = MAPPING_GetNodeIdFromInterfaceAddress(
        iface->partition->firstNode,
        from);
    assert(nodeId != INVALID_MAPPING);
    node = MAPPING_GetNodePtrFromHash(
        iface->partition->nodeIdHash,
        nodeId);
#ifdef PARALLEL //Parallel
    if (node == NULL)
    {
        // The source is on a remote partition.
        node = MAPPING_GetNodePtrFromHash(
            iface->partition->remoteNodeIdHash,
            nodeId);
    }
#endif //Parallel
    assert(node != NULL);

    // Now create a message to send to the external interface API that will
    // generate a packet.  This will add the delay and take care of
    // parallel issues.  This message will be re-sent as an IP packet.
    sendMessage = EXTERNAL_CreateSendNetworkLayerData(
                        iface,
                        srcAddr,
                        destAddr,
                        tos,
                        protocol,
                        (UInt8) ttl,
                        payload,
                        dataSize,
                        sizeof(IpHeaderType),
                        NULL,
                        from);
    MESSAGE_AddVirtualPayload(node, sendMessage, virtualSize);

    // Determine the delay for this message.  This code considers that the
    // timestamp could be 0 meaning no delay.
    clocktype delay;
    if (timestamp < EXTERNAL_QuerySimulationTime(iface))
    {
        delay = 0;
    }
    else
    {
        delay = timestamp - EXTERNAL_QuerySimulationTime(iface);
    }

    if (!EXTERNAL_IsInWarmup(iface))
    {
        EXTERNAL_MESSAGE_SendAnyNode(
            iface,
            node,
            sendMessage,
            delay,
            EXTERNAL_SCHEDULE_LOOSELY);
    }
    else
    {
        BOOL wasPktDropped;
        EXTERNAL_SendDataDuringWarmup(iface, node, 
            MESSAGE_Duplicate(node, sendMessage), &wasPktDropped);
        if (wasPktDropped)
        {
            MESSAGE_Free(node, sendMessage);
        }
    }
}

void EXTERNAL_SendDataNetworkLayer(
    EXTERNAL_Interface* iface,
    NodeAddress from,
    NodeAddress srcAddr,
    NodeAddress destAddr,
    unsigned short identification,
    BOOL dontFragment,
    BOOL moreFragments,
    unsigned short fragmentOffset,
    TosType tos,
    unsigned char protocol,
    unsigned int ttl,
    char* payload,
    int payloadSize,
    int ipHeaderLength,
    char *ipOptions,
    clocktype timestamp,
    NodeAddress nextHopAddr)
{
    NodeAddress nodeId;
    Node* node;
    Message* sendMessage;

    // Get source node
    nodeId = MAPPING_GetNodeIdFromInterfaceAddress(
        iface->partition->firstNode,
        from);
    assert(nodeId != INVALID_MAPPING);
    node = MAPPING_GetNodePtrFromHash(
        iface->partition->nodeIdHash,
        nodeId);
#ifdef PARALLEL //Parallel
    if (node == NULL)
    {
        // The source is on a remote partition.
        node = MAPPING_GetNodePtrFromHash(
            iface->partition->remoteNodeIdHash,
            nodeId);
    }
#endif //Parallel
    assert(node != NULL);
 
    // Now create a message to send to the external interface API that will
    // generate a packet.  This will add the delay and take care of
    // parallel issues.  This message will be re-sent as an IP packet.
    sendMessage = EXTERNAL_CreateSendNetworkLayerData(
        iface,
        srcAddr,
        destAddr,
        tos,
        protocol,
        (UInt8) ttl,
        payload,
        payloadSize,
        ipHeaderLength,
        ipOptions,
        from);

    // Set additional ip parameters
    IpHeaderType* qualnetIp = (IpHeaderType*) MESSAGE_ReturnPacket(sendMessage);
    qualnetIp->ip_id = identification;

    // Handle fragmentation/offset if necessary
    IpHeaderSetIpDontFrag(&(qualnetIp->ipFragment), dontFragment);
    IpHeaderSetIpMoreFrag(&(qualnetIp->ipFragment), moreFragments);
    IpHeaderSetIpFragOffset(&(qualnetIp->ipFragment), fragmentOffset);


    // Determine the delay for this message.  This code considers that the
    // timestamp could be 0 meaning no delay.
    clocktype delay;
    if (timestamp < EXTERNAL_QuerySimulationTime(iface))
    {
        delay = 0;
    }
    else
    {
       delay = timestamp - EXTERNAL_QuerySimulationTime(iface);
    }

#ifdef EXATA
     iface->partition->rrInterface->RecordHandleSniffedPacket(   
         from,  //this is the from address   
         delay,      
         srcAddr,    
         destAddr,   
         identification,     
         dontFragment,   
         moreFragments,      
         fragmentOffset,     
         tos,    
         protocol,   
         ttl,    
         payload,    
         payloadSize,    
         nextHopAddr,    
         ipHeaderLength,     
         ipOptions);     
    if (nextHopAddr != INVALID_ADDRESS)
    {
        NodeAddress *headers;
        MESSAGE_AddHeader(node, sendMessage, sizeof(NodeAddress) * 3, TRACE_IP);
        headers = (NodeAddress *)MESSAGE_ReturnPacket(sendMessage);
        headers[0] = from;
        headers[1] = destAddr;
        headers[2] = nextHopAddr;
    }
#endif

    sendMessage->stamp(node);

    if (!EXTERNAL_IsInWarmup(iface))
    {
        // Send the message.  Will be processed by IPNESendTrueEmulationPacket.
        EXTERNAL_MESSAGE_SendAnyNode(
        iface,
        node,
        sendMessage,
        delay,
        EXTERNAL_SCHEDULE_LOOSELY);
    }
    else
    {
        BOOL wasPktDropped;
        EXTERNAL_SendDataDuringWarmup (iface, node, 
            MESSAGE_Duplicate(node, sendMessage), &wasPktDropped);
        if (wasPktDropped)
        {
             MESSAGE_Free(node, sendMessage);
        }
    }
}

void EXTERNAL_SendNetworkLayerPacket(Node* node, Message* msg)
{
    EXTERNAL_NetworkLayerPacket* packet;
    int interfaceIndex;

    // Get the packet from info
    packet = (EXTERNAL_NetworkLayerPacket*) MESSAGE_ReturnInfo(msg, INFO_TYPE_ExternalData);

    // Set packet info for sending t
    MESSAGE_SetLayer(msg, NETWORK_LAYER, TransportProtocol_UDP);
    MESSAGE_SetEvent(msg, MSG_TRANSPORT_FromAppSend);

    if (packet->srcAddr.networkType == NETWORK_IPV4)
    {
        // Get outgoing interface.  Check for error.
        interfaceIndex = NetworkIpGetInterfaceIndexFromAddress(node,
                                        packet->srcAddr.interfaceAddr.ipv4);
        ERROR_Assert(interfaceIndex != -1, "Invalid interface index");

        // Re-use msg to send IP packet.  Packet already has data and info
        // created by EXTERNAL_SendDataNetworkLayer.

        // Add packet trace info
        ActionData action;
        action.actionComment = NO_COMMENT;
        action.actionType = SEND;
        TRACE_PrintTrace(node,
                         msg,
                         TRACE_TRANSPORT_LAYER,
                         PACKET_IN,
                         &action);

        // Search for ip header
        IpHeaderType* ipHeader = (IpHeaderType*) msg->packet;

        if (ipHeader)
        {
            NetworkDataIp* ip = (NetworkDataIp *) node->networkData.networkVar;
            ip->newStats->AddPacketSentToMacDataPoints(node,
                  msg,
                  STAT_NodeAddressToDestAddressType(node, ipHeader->ip_dst),
                  IsDataPacket(msg, ipHeader),
                  FALSE);
        }

        // Send the packet to the mac layer
        // Rich, this is not safe for external thread
#ifdef AUTO_IPNE_INTERFACE
        if (node->macData[interfaceIndex]->isVirtualLan)
        {
            NetworkIpReceivePacketFromMacLayer(node,
                                         msg,
                                         packet->srcAddr.interfaceAddr.ipv4,
                                         interfaceIndex);
        }
        else
#endif
        {
            RoutePacketAndSendToMac(
                node,
                msg,
                CPU_INTERFACE,
                interfaceIndex,
                ANY_IP);
        }
    }
    else
    {
        Ipv6RouterFunctionType routerFunction = NULL;
        IPv6Data* ipv6 = (IPv6Data*) node->networkData.networkVar->ipv6;
        BOOL packetWasRouted = FALSE;
        // Get outgoing interface.  Check for error.
        interfaceIndex = Ipv6GetInterfaceIndexFromAddress(node,
                                      &packet->srcAddr.interfaceAddr.ipv6);
        ERROR_Assert(interfaceIndex != -1, "Invalid Interface Index");

        routerFunction = Ipv6GetRouterFunction(node, interfaceIndex);
        // Re-use msg to send IP packet.  Packet already has data and info
        // created by EXTERNAL_SendDataNetworkLayer.

        // Add packet trace info
        ActionData action;
        action.actionComment = NO_COMMENT;
        action.actionType = SEND;
        TRACE_PrintTrace(node,
                         msg,
                         TRACE_TRANSPORT_LAYER,
                         PACKET_IN,
                         &action);

        // Search for IPV6 header
        ip6_hdr* ipHeader = (ip6_hdr*) msg->packet;

#ifdef GATEWAY_INTERFACE
    /* If the following conditions are true:
        a. The destination address does not belong to any QualNet node
        b. Internet gateway is enabled
        c. There is no entry in the routing table for the destination host
        Then: send this packet, via IP-in-IP to the gateway router */

    if ((node->ipv6InternetGateway.networkType == NETWORK_IPV6) &&
        (!IS_MULTIADDR6(ipHeader->ip6_dst))&&
        (MAPPING_GetNodeIdFromGlobalAddr(node, ipHeader->ip6_dst)
            == INVALID_MAPPING))
        {
            if (destinationLookUp(node, ipv6, &ipHeader->ip6_dst) == NULL)
            {
                 Ipv6AddIpv6Header(
                    node,
                    msg,
                    MESSAGE_ReturnPacketSize(msg),
                    ipHeader->ip6_src,
                    GetIPv6Address(node->ipv6InternetGateway),
                    IPTOS_PREC_INTERNETCONTROL,
                    IPPROTO_INTERNET_GATEWAY,
                    ipHeader->ip6_hlim);
                ipHeader = (ip6_hdr *)MESSAGE_ReturnPacket(msg);
            }
        }
#endif

        if (routerFunction && IS_MULTIADDR6(ipHeader->ip6_dst) == FALSE)
        {
            (*routerFunction)(node,
                              msg,
                              ipHeader->ip6_dst,
                              ipv6->broadcastAddr,
                              &packetWasRouted);
        }
        if (!packetWasRouted)
        {
            ndp6_resolve(node,
                         CPU_INTERFACE,
                         &interfaceIndex,
                         NULL,
                         msg,
                         &ipHeader->ip6_dst);
        }
    }
}

void EXTERNAL_CreateMapping(
    EXTERNAL_Interface *iface,
    char *key,
    int keySize,
    char *value,
    int valueSize)
{
    int hash;
    char *newKey;
    char *newValue;
    EXTERNAL_Mapping *r;
    EXTERNAL_Mapping *newRec;

    assert(iface != NULL);

    // Allocate memory for new mapping
    newRec = (EXTERNAL_Mapping*) MEM_malloc(sizeof(EXTERNAL_Mapping));

    // Create new key/value memory blocks
    newKey = (char*) MEM_malloc(keySize);
    memcpy(newKey, key, keySize);
    newValue = (char*) MEM_malloc(valueSize);
    memcpy(newValue, value, valueSize);

    // Fill in contents of new mapping
    newRec->key = newKey;
    newRec->keySize = keySize;
    newRec->val = newValue;
    newRec->valSize = valueSize;
    newRec->next = NULL;

    // Get the list with the hash value of the given key
    hash = HashKey(key, keySize);
    r = iface->table[hash];



    // If the list is empty
    if (r == NULL)
    {
        // This is the first key with this hash.  Create a new list.
        iface->table[hash] = newRec;
    }
    else
    {
        // Add key to the end of the list.  Avoid adding duplicate keys.
        while (1)
        {
            if (CompareKeys(key, keySize, r->key, r->keySize))
            {
                // Duplicate key, don't add
                ERROR_ReportWarning("Duplicate key");
                return;
            }

            if (r->next != NULL)
            {
                r = r->next;
            }
            else
            {
                break;
            }
        }

        r->next = newRec;
    }
}


int EXTERNAL_ResolveMapping(
    EXTERNAL_Interface *iface,
    char *key,
    int keySize,
    char **value,
    int *valueSize)
{
    EXTERNAL_Mapping *r;

    assert(iface != NULL);

    // Retrieve the first record with this hash value
    r = iface->table[HashKey(key, keySize)];

    // Look through the list for the record with matching key
    while (r != NULL)
    {

        if (CompareKeys(key, keySize, r->key, r->keySize))
        {
            // Found, set value and valueSize, return zero
            *value = r->val;
            *valueSize = r->valSize;
            return 0;
        }
        r = r->next;
    }

    // Not found, return non-zero
    *value = NULL;
    *valueSize = -1;
    return 1;
}

void EXTERNAL_DeleteMapping(
    EXTERNAL_Interface *iface,
    char *key,
    int keySize)
{
    EXTERNAL_Mapping *r, *p;

    assert(iface != NULL);

    // Retrieve the first record with this hash value
    r = iface->table[HashKey(key, keySize)];

    if (r == NULL)
    {
        ERROR_ReportWarning("Deleting key from mapping list which does not exists\n");
        return;
    }
    // This is the only key in this record
    if (r->next == NULL)
    {
        if (!CompareKeys(key, keySize, r->key, r->keySize))
            assert(FALSE);

        iface->table[HashKey(key, keySize)] = NULL;
        MEM_free(r->key);
        MEM_free(r->val);
        MEM_free(r);
        return;
    }

    // If this is the first item in the list
    if (CompareKeys(key, keySize, r->key, r->keySize))
    {
        iface->table[HashKey(key, keySize)] = r->next;
        MEM_free(r->key);
        MEM_free(r->val);
        MEM_free(r);
        return;
    }

    p = r;
    r = r->next;
        
    // Look through the list for the record with matching key
    while (r != NULL)
    {
        if (CompareKeys(key, keySize, r->key, r->keySize))
        {
            p->next = r->next;
            MEM_free(r->key);
            MEM_free(r->val);
            MEM_free(r);

            return;
        }
        r = r->next;
        p = p->next;
    }
}


void EXTERNAL_ActivateNode(
    EXTERNAL_Interface *iface,
    Node *node,
    ExternalScheduleType scheduling,
    clocktype activationEventTime)
{
    unsigned int interfaceIndex;
    MacFaultInfo* macFaultInfo;
    Message* msg;

    // Immediately send a message to each of the node's interfaces.  The
    // message is of type MSG_MAC_EndFault which will activate the interface
    for (interfaceIndex = 0;
         interfaceIndex < (unsigned) node->numberInterfaces;
         interfaceIndex++)
    {
        // The protocol is set to 0 -- it is ignored
        msg = MESSAGE_Alloc(node,
                            MAC_LAYER,
                            0,
                            MSG_MAC_EndFault,
                            iface->threaded);

        MESSAGE_SetInstanceId(msg, (short) interfaceIndex);

        // This information is required to handle static and
        // random fault by one pair of event message
        MESSAGE_InfoAlloc(
            node,
            msg,
            sizeof(MacFaultInfo));
        macFaultInfo = (MacFaultInfo*) MESSAGE_ReturnInfo(msg);
        macFaultInfo->faultType = EXTERNAL_FAULT;

        clocktype simTime = node->getNodeTime();
        clocktype delay = (activationEventTime) - simTime;
        if (activationEventTime == -1)
            delay = 0;
        EXTERNAL_MESSAGE_SendAnyNode (iface, node, msg, delay, scheduling);
    }

#ifndef HITL_INTERFACE
#ifdef SNMP_INTERFACE
    //SNMP trap -> warmStart
    if (iface->partition->isEmulationMode)
    {
        TrapTrigger(node, WARM_START);
    }
#endif
#endif
}

void EXTERNAL_ActivateNodeInterface(
    EXTERNAL_Interface *iface,
    Node *node, 
    ExternalScheduleType scheduling,
    clocktype activationEventTime, int interfaceIndex)
{
    MacFaultInfo* macFaultInfo;
    Message* msg;

    // Immediately send a message to each of the node's interfaces.  The
    // message is of type MSG_MAC_EndFault which will activate the interface

    // The protocol is set to 0 -- it is ignored
    msg = MESSAGE_Alloc(node,
                        MAC_LAYER,
                        0,
                        MSG_MAC_EndFault,
                        iface->threaded);

    MESSAGE_SetInstanceId(msg, (short) interfaceIndex);

    // This information is required to handle static and
    // random fault by one pair of event message
    MESSAGE_InfoAlloc(
        node,
        msg,
        sizeof(MacFaultInfo));
    macFaultInfo = (MacFaultInfo*) MESSAGE_ReturnInfo(msg);
    macFaultInfo->faultType = EXTERNAL_FAULT;

    clocktype simTime = node->getNodeTime();
    clocktype delay = (activationEventTime) - simTime;
    if (activationEventTime == -1)
        delay = 0;
    EXTERNAL_MESSAGE_SendAnyNode (iface, node, msg, delay, scheduling);
}

void EXTERNAL_DeactivateNode(
    EXTERNAL_Interface *iface,
    Node *node,
    ExternalScheduleType scheduling,
    clocktype deactivationEventTime)
{
    unsigned int interfaceIndex;
    MacFaultInfo* macFaultInfo;
    Message* msg;

    // Immediately send a message to each of the node's interfaces.  The
    // message is of type MSG_MAC_StartFault which will activate the
    // interface
    for (interfaceIndex = 0;
         interfaceIndex < (unsigned) node->numberInterfaces;
         interfaceIndex++)
    {
        // The protocol is set to 0 -- it is ignored
        msg = MESSAGE_Alloc(node,
                            MAC_LAYER,
                            0,
                            MSG_MAC_StartFault,
                            iface->threaded);

        MESSAGE_SetInstanceId(msg, (short) interfaceIndex);

        // This information is required to handle static and
        // random fault by one pair of event message
        macFaultInfo = (MacFaultInfo*) MESSAGE_InfoAlloc(
            node,
            msg,
            sizeof(MacFaultInfo));
        macFaultInfo->faultType = EXTERNAL_FAULT;

        clocktype simTime = node->getNodeTime();
        clocktype delay = (deactivationEventTime) - simTime;
        if (deactivationEventTime == -1)
            delay = 0;
        EXTERNAL_MESSAGE_SendAnyNode (iface, node, msg, delay, scheduling);
    }
}

void EXTERNAL_DeactivateNodeInterface(
    EXTERNAL_Interface *iface,
    Node *node, 
    ExternalScheduleType scheduling,
    clocktype deactivationEventTime, int interfaceNum)
{
    MacFaultInfo* macFaultInfo;
    Message* msg;
       
    msg = MESSAGE_Alloc(node,
                            MAC_LAYER,
                            0,
                        MSG_MAC_StartFault,
                        iface->threaded);

        MESSAGE_SetInstanceId(msg, (short) interfaceNum);

        // This information is required to handle static and
        // random fault by one pair of event message
        macFaultInfo = (MacFaultInfo*) MESSAGE_InfoAlloc(
            node,
            msg,
            sizeof(MacFaultInfo));
        macFaultInfo->faultType = EXTERNAL_FAULT;

        clocktype simTime = node->getNodeTime();
        clocktype delay = (deactivationEventTime) - simTime;
        if (deactivationEventTime == -1)
            delay = 0;
        EXTERNAL_MESSAGE_SendAnyNode (iface, node, msg, delay, scheduling);
}


void EXTERNAL_ChangeNodePositionAtSimTime(
    EXTERNAL_Interface *iface,
    Node *node,
    double c1,
    double c2,
    double c3,
    clocktype mobilityEventTime)
{
    CheckExternalMobilityPointers(iface, node);

    BoundMobilityEventTime(iface, node, mobilityEventTime);

    Coordinates coordinates;
    coordinates.common.c1 = c1;
    coordinates.common.c2 = c2;
    coordinates.common.c3 = c3;

    TERRAIN_BoundCoordinatesToTerrain(iface->partition->terrainData,
                                      &coordinates);

    Orientation orientation;
    MOBILITY_ReturnOrientation(node, &orientation);

    double speed;
    MOBILITY_ReturnInstantaneousSpeed(node, &speed);

    Velocity velocity;
    memset(&velocity, 0, sizeof(velocity));

    AddMobilityEvent(
        iface,
        node,
        mobilityEventTime,
        coordinates,
        orientation,
        speed,
        velocity,
        FALSE,   // velocityOnly
        FALSE);  // delaySpeedCalculation
}

void EXTERNAL_ChangeNodePosition(
    EXTERNAL_Interface *iface,
    Node *node,
    double c1,
    double c2,
    double c3,
    clocktype delay)
{
    clocktype mobilityEventTime = EXTERNAL_QueryExternalTime(iface) + delay;

    EXTERNAL_ChangeNodePositionAtSimTime(
        iface,
        node,
        c1,
        c2,
        c3,
        mobilityEventTime);
}

void EXTERNAL_ChangeNodeOrientation(
    EXTERNAL_Interface *iface,
    Node *node,
    short azimuth,
    short elevation)
{
    CheckExternalMobilityPointers(iface, node);

    clocktype mobilityEventTime = EXTERNAL_QueryExternalTime(iface);
    BoundMobilityEventTime(iface, node, mobilityEventTime);

    Coordinates coordinates;
    MOBILITY_ReturnCoordinates(node, &coordinates);

    ModOrientation(azimuth, elevation);

    Orientation orientation;
    orientation.azimuth = azimuth;
    orientation.elevation = elevation;

    double speed;
    MOBILITY_ReturnInstantaneousSpeed(node, &speed);

    Velocity velocity;
    memset(&velocity, 0, sizeof(velocity));

    AddMobilityEvent(
        iface,
        node,
        mobilityEventTime,
        coordinates,
        orientation,
        speed,
        velocity,
        FALSE,   // velocityOnly
        FALSE);  // delaySpeedCalculation
}

void EXTERNAL_ChangeNodePositionAndOrientation(
    EXTERNAL_Interface *iface,
    Node *node,
    double c1,
    double c2,
    double c3,
    short azimuth,
    short elevation)
{
    CheckExternalMobilityPointers(iface, node);

    clocktype mobilityEventTime = EXTERNAL_QueryExternalTime(iface);
    BoundMobilityEventTime(iface, node, mobilityEventTime);

    Coordinates coordinates;
    coordinates.common.c1 = c1;
    coordinates.common.c2 = c2;
    coordinates.common.c3 = c3;

    TERRAIN_BoundCoordinatesToTerrain(iface->partition->terrainData,
                                      &coordinates);

    ModOrientation(azimuth, elevation);

    Orientation orientation;
    orientation.azimuth = azimuth;
    orientation.elevation = elevation;

    double speed;
    MOBILITY_ReturnInstantaneousSpeed(node, &speed);

    Velocity velocity;
    memset(&velocity, 0, sizeof(velocity));

    AddMobilityEvent(
        iface,
        node,
        mobilityEventTime,
        coordinates,
        orientation,
        speed,
        velocity,
        FALSE,   // velocityOnly
        FALSE);  // delaySpeedCalculation
}

void EXTERNAL_ChangeNodePositionOrientationAndSpeedAtTime(
    EXTERNAL_Interface *iface,
    Node *node,
    clocktype mobilityEventTime,
    double c1,
    double c2,
    double c3,
    short azimuth,
    short elevation,
    double speed)
{
    CheckExternalMobilityPointers(iface, node);

    BoundMobilityEventTime(iface, node, mobilityEventTime);

    Coordinates coordinates;
    coordinates.common.c1 = c1;
    coordinates.common.c2 = c2;
    coordinates.common.c3 = c3;

    TERRAIN_BoundCoordinatesToTerrain(iface->partition->terrainData,
                                      &coordinates);

    ModOrientation(azimuth, elevation);

    Orientation orientation;
    orientation.azimuth = azimuth;
    orientation.elevation = elevation;

    BoundSpeed(speed);

    Velocity velocity;
    memset(&velocity, 0, sizeof(velocity));

    AddMobilityEvent(
        iface,
        node,
        mobilityEventTime,
        coordinates,
        orientation,
        speed,
        velocity,
        FALSE,   // velocityOnly
        FALSE);  // delaySpeedCalculation
}

void EXTERNAL_ChangeNodePositionOrientationAndVelocityAtTime(
    EXTERNAL_Interface *iface,
    Node *node,
    clocktype mobilityEventTime,
    double c1,
    double c2,
    double c3,
    short azimuth,
    short elevation,
    double speed,
    double c1Speed,
    double c2Speed,
    double c3Speed)
{
    CheckExternalMobilityPointers(iface, node);

    BoundMobilityEventTime(iface, node, mobilityEventTime);

    Coordinates coordinates;
    coordinates.common.c1 = c1;
    coordinates.common.c2 = c2;
    coordinates.common.c3 = c3;

    TERRAIN_BoundCoordinatesToTerrain(iface->partition->terrainData,
                                      &coordinates);

    ModOrientation(azimuth, elevation);

    Orientation orientation;
    orientation.azimuth = azimuth;
    orientation.elevation = elevation;

    BoundSpeed(speed);

    Velocity velocity;

    if (c1Speed == 0.0 && c2Speed == 0.0 && c3Speed == 0.0)
    {
        speed = 0.0;

        memset(&velocity, 0, sizeof(velocity));
    }
    else
    {
        velocity.common.c1 = c1Speed;
        velocity.common.c2 = c2Speed;
        velocity.common.c3 = c3Speed;
    }

    AddMobilityEvent(
        iface,
        node,
        mobilityEventTime,
        coordinates,
        orientation,
        speed,
        velocity,
        FALSE,   // velocityOnly
        FALSE);  // delaySpeedCalculation
}

void EXTERNAL_ChangeNodePositionOrientationAndVelocityAtTime(
    EXTERNAL_Interface *iface,
    Node *node,
    clocktype mobilityEventTime,
    double c1,
    double c2,
    double c3,
    short azimuth,
    short elevation,
    double c1Speed,
    double c2Speed,
    double c3Speed)
{
    // Handle degenerate case of known zero speed.

    if (c1Speed == 0.0 && c2Speed == 0.0 && c3Speed == 0.0)
    {
        EXTERNAL_ChangeNodePositionOrientationAndVelocityAtTime(
            iface,
            node,
            mobilityEventTime,
            c1,
            c2,
            c3,
            azimuth,
            elevation,
            0.0,  // speed
            c1Speed,
            c2Speed,
            c3Speed);

        return;
    }

    // Resume normal checks.

    CheckExternalMobilityPointers(iface, node);

    BoundMobilityEventTime(iface, node, mobilityEventTime);

    Coordinates coordinates;
    coordinates.common.c1 = c1;
    coordinates.common.c2 = c2;
    coordinates.common.c3 = c3;

    ModOrientation(azimuth, elevation);

    Orientation orientation;
    orientation.azimuth = azimuth;
    orientation.elevation = elevation;

    Velocity velocity;

    velocity.common.c1 = c1Speed;
    velocity.common.c2 = c2Speed;
    velocity.common.c3 = c3Speed;

    AddMobilityEvent(
        iface,
        node,
        mobilityEventTime,
        coordinates,
        orientation,
        0.0,    // speed
        velocity,
        FALSE,  // velocityOnly
        TRUE);  // delaySpeedCalculation
}

void EXTERNAL_ChangeNodeVelocityAtTime(
    EXTERNAL_Interface *iface,
    Node *node,
    clocktype mobilityEventTime,
    double speed,
    double c1Speed,
    double c2Speed,
    double c3Speed)
{
    CheckExternalMobilityPointers(iface, node);

    BoundMobilityEventTime(iface, node, mobilityEventTime);

    Coordinates coordinates;
    Orientation orientation;

    memset(&coordinates, 0, sizeof(coordinates));
    memset(&orientation, 0, sizeof(orientation));

    BoundSpeed(speed);

    Velocity velocity;
    velocity.common.c1 = c1Speed;
    velocity.common.c2 = c2Speed;
    velocity.common.c3 = c3Speed;

    AddMobilityEvent(
        iface,
        node,
        mobilityEventTime,
        coordinates,
        orientation,
        speed,
        velocity,
        TRUE,    // velocityOnlyTrue
        FALSE);  // delaySpeedCalculationFalse
}

void EXTERNAL_ChangeNodeVelocityAtTime(
    EXTERNAL_Interface *iface,
    Node *node,
    clocktype mobilityEventTime,
    double c1Speed,
    double c2Speed,
    double c3Speed)
{
    // Handle degenerate case of known zero speed.

    if (c1Speed == 0.0 && c2Speed == 0.0 && c3Speed == 0.0)
    {
        EXTERNAL_ChangeNodeVelocityAtTime(
            iface,
            node,
            mobilityEventTime,
            0.0,  // speed
            c1Speed,
            c2Speed,
            c3Speed);

        return;
    }

    // Resume normal checks.

    CheckExternalMobilityPointers(iface, node);

    BoundMobilityEventTime(iface, node, mobilityEventTime);

    Coordinates coordinates;
    Orientation orientation;

    memset(&coordinates, 0, sizeof(coordinates));
    memset(&orientation, 0, sizeof(orientation));

    Velocity velocity;
    velocity.common.c1 = c1Speed;
    velocity.common.c2 = c2Speed;
    velocity.common.c3 = c3Speed;

    AddMobilityEvent(
        iface,
        node,
        mobilityEventTime,
        coordinates,
        orientation,
        0.0,    // speed
        velocity,
        TRUE,   // velocityOnly
        TRUE);  // delaySpeedCalculation
}

BOOL EXTERNAL_ConfigStringPresent(
    NodeInput *nodeInput,
    const char *string)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;

    // Attempt to retrieve string from config file
    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        string,
        &retVal,
        buf);

    // If the value is present return TRUE, else FALSE
    if (retVal)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL EXTERNAL_ConfigStringIsYes(
    NodeInput* nodeInput,
    const char* string)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;

    // Attempt to retrieve string from config file
    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        string,
        &retVal,
        buf);

    // If the value is present and YES return TRUE, else FALSE
    if (retVal && strcmp(buf, "YES") == 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void EXTERNAL_SetSimulationEndTime(
    EXTERNAL_Interface* iface,
    clocktype duration)
{
    PartitionData* partitionData;

    partitionData = iface->interfaceList->partition;

#ifdef PARALLEL //Parallel
    // examine safe time for the earliest allowed time for termination.
    if (partitionData->safeTime != CLOCKTYPE_MAX && duration != CLOCKTYPE_MAX)
    {
        // examine safe time for the earliest allowed time for termination.
        if (duration < partitionData->safeTime + 1)
        {
            duration = partitionData->safeTime + 1;
        }
        // Add one so that the current event will complete on all partitions.
        // This will end up calling EXTERNAL_handleSetSimulationDuration ()
    }

    Message* setDurationMessage = MESSAGE_Alloc(
        partitionData->firstNode,
        PARTITION_COMMUNICATION,     // special layer
        partitionData->externalSimulationDurationCommunicator, // protccol - our commtor id
        0);     // only 1 msg type, so event type is just 0.
    MESSAGE_SetLooseScheduling (setDurationMessage);
    MESSAGE_InfoAlloc(
        partitionData->firstNode,
        setDurationMessage,
        sizeof (EXTERNAL_SimulationDurationInfo));
    EXTERNAL_SimulationDurationInfo * setDurationInfo =
        (EXTERNAL_SimulationDurationInfo *) MESSAGE_ReturnInfo (
        setDurationMessage);
    setDurationInfo->maxSimClock = duration;
    PARTITION_COMMUNICATION_SendToAllPartitions(
        partitionData,
        setDurationMessage,
        (partitionData->safeTime - partitionData->getGlobalTime()) + 1);
#endif //Parallel

    // Record when the external interface stopped
    if (duration != CLOCKTYPE_MAX)
    {
        iface->interfaceList->isStopping = TRUE;
        iface->interfaceList->stopTime = duration;

        // If we are stopping immediately set the stop time 1ns in the future.
        // This will ensure this partition runs through one more event loop so
        // other partitions will get the stop message.
        if (duration <= partitionData->getGlobalTime())
        {
            duration = partitionData->getGlobalTime() + 1;
        }
    }
    partitionData->maxSimClock = duration;

    // Schedule a heartbeat message for the stop time.  This will make sure
    // there is an event for that time.
    if (duration < CLOCKTYPE_MAX)
    {
        Message* heartbeat;
        // We're assuming this function won't be called from a thread
        // even if the interface is threaded.  It should be called during
        // startup.
        heartbeat = MESSAGE_Alloc(
            partitionData->firstNode,
            EXTERNAL_LAYER,
            EXTERNAL_NONE,
            MSG_EXTERNAL_Heartbeat);
        MESSAGE_Send(
            partitionData->firstNode,
            heartbeat,
            duration - partitionData->getGlobalTime());
    }
}

/*
 * Currently nothing for these functions:
 *
 * EXTERNAL_Measurements
 */

//This function should be modified later to use the overloaded function
//that has been added below....
void EXTERNAL_MESSAGE_RemoteSend (
    EXTERNAL_Interface* iface,
    int destinationPartitionId,
    Message* msg,
    clocktype delay,
    ExternalScheduleType scheduling)
{
    if (iface->partition->partitionId == destinationPartitionId)
    {
        // This external message is meant for our own partition,
        // so just use a normal message send.
        msg->nodeId = iface->partition->firstNode->nodeId;
        MESSAGE_Send(iface->partition->firstNode, msg, delay);
    }
    else
    {
#ifdef PARALLEL
        // See MESSAGE_RemoteSend for an analogous set of operations.
        msg->nodeId = ANY_DEST;
        msg->eventTime = iface->partition->getGlobalTime() + delay;
        if (scheduling == EXTERNAL_SCHEDULE_LOOSELY)
        {
            MESSAGE_SetLooseScheduling (msg);
        }
        if (scheduling == EXTERNAL_SCHEDULE_SAFE)
        {
            if (msg->eventTime < iface->partition->safeTime)
            {
                msg->eventTime = iface->partition->safeTime + 1;
            }
        }
        PARALLEL_SendRemoteMessages(msg,
                                    iface->partition,
                                    destinationPartitionId);
#ifdef USE_MPI
        MESSAGE_Free(iface->partition->firstNode, msg);
#endif
#endif
    }
}

void EXTERNAL_MESSAGE_RemoteSend (
    PartitionData* partition,
    int destinationPartitionId,
    Message* msg,
    clocktype delay,
    ExternalScheduleType scheduling)
{
    if (partition->partitionId == destinationPartitionId)
    {
        // This external message is meant for our own partition,
        // so just use a normal message send.
        msg->nodeId = partition->firstNode->nodeId;
        MESSAGE_Send(partition->firstNode, msg, delay);
    }
    else
    {
#ifdef PARALLEL
        // See MESSAGE_RemoteSend for an analogous set of operations.
        msg->nodeId = ANY_DEST;
        msg->eventTime = partition->getGlobalTime() + delay;
        if (scheduling == EXTERNAL_SCHEDULE_LOOSELY)
        {
            MESSAGE_SetLooseScheduling (msg);
        }
        if (scheduling == EXTERNAL_SCHEDULE_SAFE)
        {
            if (msg->eventTime < partition->safeTime)
            {
                msg->eventTime = partition->safeTime + 1;
            }
        }
        PARALLEL_SendRemoteMessages (msg, partition, destinationPartitionId);
#ifdef USE_MPI
        MESSAGE_Free(partition->firstNode, msg);
#endif
#endif
    }
}

// node - the destination to process the event.
void EXTERNAL_MESSAGE_SendAnyNode(
    EXTERNAL_Interface* iface,
    Node* node,
    Message* msg,
    clocktype delay,
    ExternalScheduleType scheduling)
{
    EXTERNAL_MESSAGE_SendAnyNode(iface->partition,
                                 node,
                                 msg,
                                 delay,
                                 scheduling);
}

// node - the destination to process the event.
void EXTERNAL_MESSAGE_SendAnyNode(
    PartitionData* partitionData,
    Node* node,
    Message* msg,
    clocktype delay,
    ExternalScheduleType scheduling)
{
    // Send the message to the indicated node. node can be on a remote
    // partition. Messages must follow many rules to be sendable to
    // remote partitons:
    //  - flat info/packet (i.e. no pointers)
#ifdef PARALLEL
    if (node->partitionData->isRunningInParallel())
    {
        // Node intended to prcoess this StartFault msg
        msg->nodeId = node->nodeId;
        // set event time and/or delay
        clocktype earliestSafeInsertionTime = 0;
        if (scheduling == EXTERNAL_SCHEDULE_SAFE ||
                scheduling == EXTERNAL_SCHEDULE_LOOSELY)
        {
            if (scheduling == EXTERNAL_SCHEDULE_SAFE)
            {
                earliestSafeInsertionTime = MAX(node->getNodeTime() + delay,
                                        node->partitionData->safeTime + 1);
            }
            else    // EXTERNAL_SCHEDULE_LOOSELY
            {
                earliestSafeInsertionTime =
                                MIN(node->partitionData->nextInternalEvent,
                                        node->partitionData->safeTime + 1);
            }
            if (node->getNodeTime() + delay < earliestSafeInsertionTime)
            {
                delay = earliestSafeInsertionTime - node->getNodeTime();
            }
        }
        else
        {
            delay = 0;
        }
        msg->eventTime = node->getNodeTime() + delay;        
    }
#endif
    if (node->partitionId == node->partitionData->partitionId)
    {
        MESSAGE_Send(node, msg, delay, msg->mtWasMT);
    }
    else
    {
#ifdef PARALLEL
        if (scheduling == EXTERNAL_SCHEDULE_LOOSELY)
        {
            MESSAGE_SetLooseScheduling (msg);
        }
        PARALLEL_SendRemoteMessages (msg, partitionData, node->partitionId);
#ifdef USE_MPI
        MESSAGE_Free(node, msg);
#endif
#endif
    }
}

// /**
// API       :: EXTERNAL_PHY_SetTxPower
// PURPOSE   :: Assign a num physical layer transmission powere on the node
//              where node might be on a remote partition.
// PARAMETERS::
// + node : Node * : Pointer to node (local or proxy) that should be changed
// + phyIndex : int : Which physical
// + newTxPower : doulde : New transmission power - see PHY_SetTransmitPower ()
// RETURN    :: void
// **/
void
EXTERNAL_PHY_SetTxPower (Node * node, int phyIndex, double newTxPower)
{
#ifdef ADDON_WIRELESS
    if (node->partitionId == node->partitionData->partitionId)
    {
        // A local node, so call directly.
        PHY_SetTransmitPower(node, phyIndex, newTxPower);
        return;
    }

    // This is for a remote node, have to create a external interface message
    // (partition communication) to send to the remote partition
    // for node Id, phyIndex, and new tx power
    Message *setTxMsg = MESSAGE_Alloc(node,
                                      EXTERNAL_LAYER,
                                      EXTERNAL_NONE,
                                      MSG_EXTERNAL_PhySetTxPower,
                                      iface->threaded);
    MESSAGE_InfoAlloc(node, setTxMsg, sizeof(EXTERNAL_SetPhyTxPowerInfo));
    EXTERNAL_SetPhyTxPowerInfo *setTxPowerInfo =
        (EXTERNAL_SetPhyTxPowerInfo *) MESSAGE_ReturnInfo (setTxMsg);
    setTxPowerInfo->phyIndex = phyIndex;
    setTxPowerInfo->txPower = newTxPower;
    setTxPowerInfo->nodeId = node->nodeId;
    EXTERNAL_MESSAGE_SendAnyNode(node->partitionData,
                                 node,
                                 setTxMsg,
                                 0,
                                 EXTERNAL_SCHEDULE_LOOSELY);
#endif
}

void
EXTERNAL_PHY_GetTxPower  (Node * node, int phyIndex, double * txPowerPtr)
{
    if (node->partitionId == node->partitionData->partitionId)
    {
        // A local node, so just call PHY_GetTransmitPower()
        PHY_GetTransmitPower(node, 0, txPowerPtr);
        return;
    }
    else
    {
        double txPower;
        // execute a dynamic hierarchy command to obtain the remote
        // node's tx power.
        D_Hierarchy *hierarchy = &node->partitionData->dynamicHierarchy;
        if (!hierarchy->IsEnabled ())
        {
            ERROR_ReportWarning("Failed to obtain transmission power for remote node because DYNAMIC-ENABELD isn't set.");
            *txPowerPtr = -1.0;
            return;
        }

        std::string phyPowerByIndexPath;
        hierarchy->BuildNodeInterfaceIndexPath (
            node, phyIndex,
            "TxPower",
            phyPowerByIndexPath);

        std::string result;
        hierarchy->ExecuteAsString (phyPowerByIndexPath, "", result);
        std::istringstream  iss (result);
        iss >> txPower;
        *txPowerPtr = txPower;

        return;
    }
}

clocktype EXTERNAL_QueryRealTime()
{
#ifdef _WIN32
    LARGE_INTEGER t;
    LARGE_INTEGER freq;
    BOOL success;
    clocktype val;
    clocktype fraction;

    success = QueryPerformanceCounter(&t);
    if (!success)
    {
        assert(0);
    }

    success = QueryPerformanceFrequency(&freq);
    if (!success)
    {
        assert(0);
    }

    // Compute number of seconds
    fraction = t.QuadPart / freq.QuadPart;
    val = fraction * SECOND;

    // Compute fractions of seconds
    fraction = t.QuadPart - fraction * freq.QuadPart;
    val += fraction * SECOND / freq.QuadPart;

    return val;
#else /* unix/linux */
    struct timeval t;
    int err;

    // get time and convert to qualnet time
    err = gettimeofday(&t, NULL);
    if (err == -1)
    {
        assert(0);
    }

    return t.tv_sec * SECOND + t.tv_usec * MICRO_SECOND;
#endif
}

clocktype EXTERNAL_QueryRealTime(EXTERNAL_Interface* iface)
{
    return iface->partition->wallClock->getRealTime();
}

clocktype EXTERNAL_QueryCPUTime(EXTERNAL_Interface *iface)
{
    clocktype guess;
    clock_t now;
    int i;

    // If the cpu timing system needs to be initialized
    if (iface->cpuTimingStarted == FALSE)
    {
        iface->cpuTimeStart = clock();

        // A return value of (clock_t) -1 indicates that this function is
        // not supported.  In that case return 0.
        if (iface->cpuTimeStart == -1)
        {
            return 0;
        }

        iface->cpuTimingStarted = TRUE;

        // Make a few guesses at the CPU timing interval
        iface->cpuTimingInterval = 1 * SECOND;
        for (i = 0; i < EXTERNAL_NUM_CPU_TIMING_INTERVAL_GUESSES; i++)
        {
            iface->lastCpuTime = clock();
            now = clock();
            while (now == iface->lastCpuTime)
            {
                now = clock();
            }

            guess = (now - iface->lastCpuTime)
                    * SECOND / CLOCKS_PER_SEC;
            if (guess < iface->cpuTimingInterval)
            {
                iface->cpuTimingInterval = guess;
            }
        }
        /*printf("guess = %f\n",
               (double) iface->cpuTimingInterval / SECOND);*/
    }

    // Get time and convert to qualnet time
    now = clock();
    guess = (now - iface->lastCpuTime)
            * SECOND / CLOCKS_PER_SEC;
    if ((guess != 0) && (guess < iface->cpuTimingInterval))
    {
        iface->cpuTimingInterval = guess;
        printf("updated to %f\n",
               (double) iface->cpuTimingInterval / SECOND);
    }
    iface->lastCpuTime = now;

    // Return CPU time
    return (now - iface->cpuTimeStart) * SECOND / CLOCKS_PER_SEC;
}

void EXTERNAL_Sleep(clocktype amount)
{
    // Sleep for the closest amount of time on the given platform
#ifdef _WIN32
    Sleep((UInt32) (amount / MILLI_SECOND));
#else /* unix/linux */
    usleep(amount / MICRO_SECOND);
#endif // _WIN32
}

void EXTERNAL_SendDataDuringWarmup(EXTERNAL_Interface* iface, Node* node,
                                   Message* msg, BOOL* pktDrop)
{
    Message * statsMessage;
    statsMessage = MESSAGE_Alloc(iface->partition->firstNode,
        EXTERNAL_LAYER,
        0,
        MSG_EXTERNAL_RecordStats);
    
    pktDrop = (BOOL*) MESSAGE_AddInfo(iface->partition->firstNode, 
        statsMessage, sizeof(BOOL), INFO_TYPE_DidDropPacket);
    
    if (!iface->interfaceList->warmupDrop)
    {
        EXTERNAL_MESSAGE_SendAnyNode(
            iface,
            node,
            msg,
            0,
            EXTERNAL_SCHEDULE_LOOSELY);
        *pktDrop = FALSE;
    }
    else
    {
        *pktDrop = TRUE;
    }
    EXTERNAL_MESSAGE_SendAnyNode(
            iface,
            node,
            statsMessage,
            0,
            EXTERNAL_SCHEDULE_LOOSELY);
}
