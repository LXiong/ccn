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
//
// Purpose: Support PHBs described in RFC 2475, RFC 2597, RFC 2598
//

// /**
// PROTOCOL     :: Queue-Scheduler
// LAYER        ::
// REFERENCES   ::
// + RFC 2475, RFC 2597, RFC 2598
// COMMENTS     :: None
// **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "network_ip.h"
#include "mf_traffic_conditioner.h"
#include "sch_strictprio.h"
#include "sch_wrr.h"
#include "sch_wfq.h"
#include "sch_diffserv.h"
#include "sch_graph.h"

// /**
// FUNCTION   :: DiffservScheduler::insert
// LAYER      ::
// PURPOSE    :: Inserts a packet in the queue.This function prototype
//               determines the arguments that need to be passed to a
//               scheduler data structure in order to insert a message into
//               it. The scheduler is responsible for routing this packet to
//               an appropriate queue that it controls, and returning
//               whether or not the procedure was successful. The changes
//               here are the replacement of the dscp parameter
//               with the infoField pointer.  The size of this infoField is
//               stored in infoFieldSize.
// PARAMETERS ::
// + msg : Message* : Pointer to Message structure
// + QueueIsFull : BOOL* : returns Queue occupancy status
// + queueIndex : const int : Queue index
// + infoField : const void* : The infoField parameter
// + currentTime : const clocktype : current Simulation time
// RETURN     :: void : Null
// **/

void DiffservScheduler::insert(
    Message* msg,
    BOOL* QueueIsFull,
    const int priority,
    const void *infoField,
    const clocktype currentTime)
{
    insert(msg,QueueIsFull,priority,infoField,currentTime,NULL);
}

void DiffservScheduler::insert(
    Node* node,
    int interfaceIndex,
    Message* msg,
    BOOL* QueueIsFull,
    const int priority,
    const void *infoField,
    const clocktype currentTime)
{
    insert(msg,QueueIsFull,priority,infoField,currentTime,NULL);
}

void DiffservScheduler::insert(
    Node* node,
    int interfaceIndex,
    Message* msg,
    BOOL* QueueIsFull,
    const int priority,
    const void *infoField,
    const clocktype currentTime,
    TosType* tos)
{
    insert(msg,QueueIsFull,priority,infoField,currentTime,NULL);
}



void DiffservScheduler::insert(
    Message* msg,
    BOOL* QueueIsFull,
    const int priority,
    const void* infoField,
    const clocktype currentTime,
    TosType* tos)
{
    int queueIndex = numQueues;
    int queueInternalIndex;
    int i;

    for (i = 0; i < numQueues; i ++)
    {
        if (queueData[i].priority == priority)
        {
            queueIndex = i;
            break;
        }
    }
    queueInternalIndex = queueIndex;

    ERROR_Assert((queueIndex >= 0) && (queueIndex < numQueues),
        "Queue does not exist!!!\n");

    // First check inner(Second) scheduler has the control on that
    // particular priority queue,
    // if yes, then call insert function of inner(Second) scheduler
    // if not, call insert function of outer(Main) scheduler

    if (innerScheduler != NULL)
    {
        queueInternalIndex = queueIndex - (*innerScheduler).numQueue();
    }

    if(tos == NULL)
    {
        if (queueInternalIndex < 0)
        {
            (*innerScheduler).insert(msg,
                                    QueueIsFull,
                                    priority,
                                    infoField,
                                    currentTime);
        }
        else
        {
            (*outerScheduler).insert(msg,
                                    QueueIsFull,
                                    priority,
                                    infoField,
                                    currentTime);
        }

    }
    else
    {
        if (queueInternalIndex < 0)
        {
            (*innerScheduler).insert(msg,
                                    QueueIsFull,
                                    priority,
                                    infoField,
                                    currentTime,
                                    tos);
        }
        else
        {
            (*outerScheduler).insert(msg,
                                    QueueIsFull,
                                    priority,
                                    infoField,
                                    currentTime,
                                    tos);
        }

    }
}


// /**
// FUNCTION   :: DiffservScheduler::retrieve
// LAYER      ::
// PURPOSE    :: Dequeues a packet from the queue.This function prototype
//               determines the arguments that need to be passed to a
//               scheduler data structure in order to dequeue or peek at a
//               message in its array of stored messages. I've reordered the
//               arguments slightly, and incorporated the "DropFunction"
//               functionality as well.
// PARAMETERS ::
// + priority : const int : Queue priority
// + index : const int : The position of the packet in the queue
// + msg : Message** : The retrieved message
// + msgPriority : int* : Message's priority
// + operation : const QueueOperation : the retrieval mode
// + currentTime : const clocktype : current Simulation time
// RETURN     :: BOOL : TRUE or FALSE
// **/

BOOL DiffservScheduler::retrieve(
    const int priority,
    const int index,
    Message** msg,
    int* msgPriority,
    const QueueOperation operation,
    const clocktype currentTime)
{
    BOOL isMsgRetrieved = FALSE;
    *msg = NULL;

    if (!(index < numberInQueue(priority)) || (index < 0))
    {
        return isMsgRetrieved;
    }

    if (priority == ALL_PRIORITIES)
    {
        int pktIndex = index;

        if (!(*outerScheduler).isEmpty(priority))
        {
            pktIndex = index - (*outerScheduler).numberInQueue(priority);
        }

        if (pktIndex < 0)
        {
            isMsgRetrieved = (*outerScheduler).retrieve(priority,
                                                        index,
                                                        msg,
                                                        msgPriority,
                                                        operation,
                                                        currentTime);
        }
        else
        {
            if (innerScheduler != NULL)
            {
                isMsgRetrieved = (*innerScheduler).retrieve(priority,
                                                        pktIndex,
                                                        msg,
                                                        msgPriority,
                                                        operation,
                                                        currentTime);
            }
        }
    }
    else // specificPriorityOnly
    {
        int qIndex = GenericPacketClassifier(this, priority);

        // First check inner(Second) scheduler has the control on that
        // particular priority queue,
        // if yes, then call numberInQueue() of inner(Second) scheduler
        // if not, call numberInQueue() of outer(Main) scheduler

        if ((innerScheduler != NULL) &&
            (qIndex < (*innerScheduler).numQueue()))
        {
            isMsgRetrieved = (*innerScheduler).retrieve(priority,
                                                        index,
                                                        msg,
                                                        msgPriority,
                                                        operation,
                                                        currentTime);
        }
        else
        {
            isMsgRetrieved = (*outerScheduler).retrieve(priority,
                                                        index,
                                                        msg,
                                                        msgPriority,
                                                        operation,
                                                        currentTime);
        }

    }

    return isMsgRetrieved;
}


// /**
// FUNCTION   :: DiffservScheduler::isEmpty
// LAYER      ::
// PURPOSE    ::  Returns a Boolean value of TRUE if the array of stored
//                messages in each queue that the scheduler controls are
//                empty, and FALSE otherwise
// PARAMETERS ::
// + priority : const int : Priority of a queue
// RETURN     :: BOOL : TRUE or FALSE
// **/

BOOL DiffservScheduler::isEmpty(const int priority)
{
    if ((*outerScheduler).isEmpty(priority))
    {
        if (innerScheduler != NULL)
        {
            return ((*innerScheduler).isEmpty(priority));
        }
        else
        {
            return TRUE;
        }
    }
    return FALSE;
}


// /**
// FUNCTION   :: DiffservScheduler::bytesInQueue
// LAYER      ::
// PURPOSE    :: This function prototype returns the total number of bytes
//               stored in the array of either a specific queue, or all
//               queues that the scheduler controls.
// PARAMETERS ::
// + priority : const int : priority of the queue.
// RETURN     :: int : Integer
// **/

int DiffservScheduler::bytesInQueue(const int priority)
{

    if (priority == ALL_PRIORITIES)
    {
        int totalBytesInQueues = 0;

        totalBytesInQueues = (*outerScheduler).bytesInQueue(priority);

        if (innerScheduler!= NULL)
        {
            totalBytesInQueues += (*innerScheduler).bytesInQueue(priority);
        }
        return totalBytesInQueues;
    }
    else // specificPriorityOnly
    {
        int qIndex = GenericPacketClassifier(this, priority);

        // First check inner(Second) scheduler has the control on that
        // particular priority queue,
        // if yes, then call numberInQueue() of inner(Second) scheduler
        // if not, call numberInQueue() of outer(Main) scheduler

        if ((innerScheduler != NULL) &&
            (qIndex < (*innerScheduler).numQueue()))
        {
            return ((*innerScheduler).bytesInQueue(priority));
        }
        else
        {
            return ((*outerScheduler).bytesInQueue(priority));
        }
    }
    
}


// /**
// FUNCTION   :: DiffservScheduler::numberInQueue
// LAYER      ::
// PURPOSE    :: This function prototype returns the number of messages
//               stored in the array of either a specific queue, or all
//               queues that the scheduler controls.
// PARAMETERS ::
// + priority : const int : Priority of a queue
// RETURN     :: int : Integer
// **/

int DiffservScheduler::numberInQueue(const int priority)
{

    if (priority == ALL_PRIORITIES)
    {
        int numInQueue = 0;

        numInQueue = (*outerScheduler).numberInQueue(priority);

        if (innerScheduler!= NULL)
        {
            numInQueue += (*innerScheduler).numberInQueue(priority);
        }
        return numInQueue;
    }
    else // specificPriorityOnly
    {
        int qIndex = GenericPacketClassifier(this, priority);

        // First check inner(Second) scheduler has the control on that
        // particular priority queue,
        // if yes, then call numberInQueue() of inner(Second) scheduler
        // if not, call numberInQueue() of outer(Main) scheduler

        if ((innerScheduler != NULL) &&
            (qIndex < (*innerScheduler).numQueue()))
        {
            return ((*innerScheduler).numberInQueue(priority));
        }
        else
        {
            return ((*outerScheduler).numberInQueue(priority));
        }
    }    
}


// /**
// FUNCTION   :: DiffservScheduler::addQueue
// LAYER      ::
// PURPOSE    :: This function adds a queue to a scheduler.
//               This can be called either at the beginning of the
//               simulation (most likely), or during the simulation (in
//               response to QoS situations).  In either case, the
//               scheduling algorithm must accommodate the new queue, and
//               behave appropriately (recalculate turns, order, priorities)
//               immediately. This queue must have been previously setup
//               using QUEUE_Setup. Notice that the default value for
//               priority is the ALL_PRIORITIES constant, which implies that
//               the scheduler should assign the next appropriate value to
//               the queue.  This could be an error condition, if it is not
//               immediately apparent what the next appropriate value would
//               be, given the scheduling algorithm running.
// PARAMETERS ::
// + queue : Queue* : Pointer of Queue class
// + priority : const int : Priority of the queue
// + weight : const double : Weight of the queue
// RETURN     :: int : Integer
// **/

int DiffservScheduler::addQueue(
    Queue* queue,
    const int priority,
    const double weight)
{
    int addedQueuePriority = ALL_PRIORITIES;
    int i = ALL_PRIORITIES;

    if ((weight <= 0.0) || (weight > 1.0))
    {
        // Considering  1.0 as this is the default value
        ERROR_ReportError(
            "Diffserv: QUEUE-WEIGHT value must stay between <0.0 - 1.0>\n");
    }

    if ((numQueues == 0) || (numQueues == maxQueues))
    {
        QueueData* newQueueData
            = new QueueData[maxQueues + DEFAULT_QUEUE_COUNT];

        if (queueData != NULL)
        {
            memcpy(newQueueData, queueData, sizeof(QueueData) * maxQueues);
            delete[] queueData;
        }

        queueData = newQueueData;
        maxQueues += DEFAULT_QUEUE_COUNT;
    }

    ERROR_Assert(maxQueues > 0, "Scheduler addQueue function maxQueues <= 0");

    if (priority == ALL_PRIORITIES)
    {
        int newPriority = ALL_PRIORITIES;
        i = numQueues;

        if (numQueues > 0)
        {
            newPriority = queueData[i-1].priority + 1;
        }
        else
        {
            newPriority = 0;
        }

        queueData[i].priority = newPriority;
        queueData[i].weight = (float)weight;
        queueData[i].queue = queue;

        numQueues++;
    }
    else //manualPrioritySelection
    {
        BOOL queueAdded = FALSE;
        for (i = 0; i < numQueues; i++)
        {
            ERROR_Assert(queueData[i].priority != priority,
                "Priority queue already exists");
        }

        for (i = 0; i < numQueues; i++)
        {
            if (queueData[i].priority > priority)
            {
                int afterEntries = numQueues - i;
                QueueData* newQueueData = new QueueData[afterEntries];

                memcpy(newQueueData, &queueData[i],
                       sizeof(QueueData) * afterEntries);

                memcpy(&queueData[i+1], newQueueData,
                       sizeof(QueueData) * afterEntries);

                queueData[i].priority = priority;
                queueData[i].weight = (float)weight;
                queueData[i].queue = queue;

                numQueues++;
                delete[] newQueueData;
                queueAdded = TRUE;
                break;
            }
        }

        if (!queueAdded)
        {
            i = numQueues;

            queueData[i].priority = priority;
            queueData[i].weight = (float)weight;
            queueData[i].queue = queue;

            numQueues++;
        }
    }

    if (innerSchedNumQueue &&
        (innerSchedNumQueue > (*innerScheduler).numQueue()))
    {
        addedQueuePriority = innerScheduler->addQueue(queue,
                                            queueData[i].priority,
                                            queueData[i].weight);
    }
    else
    {
        addedQueuePriority = outerScheduler->addQueue(queue,
                                            queueData[i].priority,
                                            queueData[i].weight);
    }

    return addedQueuePriority;
}


// /**
// FUNCTION   :: DiffservScheduler::removeQueue
// LAYER      ::
// PURPOSE    :: This function removes a queue from the scheduler.
//               This function does not automatically finalize the queue,
//               so if the protocol modeler would like the queue to print
//               statistics before exiting, it must be explicitly finalized.
// PARAMETERS ::
// + priority : const int : Priority of the queue
// RETURN     :: void : Null
// **/

void DiffservScheduler::removeQueue(const int priority)
{
    int i = ALL_PRIORITIES;
    int qIndex = GenericPacketClassifier(this, priority);

    // First check inner(Second) scheduler has the control on that
    // particular priority queue,
    // if yes, then call removeQueue() of inner(Second) scheduler
    // if not, call removeQueue() of outer(Main) scheduler

    if ((innerScheduler != NULL) &&
        (qIndex < (*innerScheduler).numQueue()))
    {
        (*innerScheduler).removeQueue(priority);
    }
    else
    {
        (*outerScheduler).removeQueue(priority);
    }

    for (i = 0; i < numQueues; i++)
    {
        if (queueData[i].priority == priority)
        {
            int afterEntries = numQueues - (i + 1);

            if (afterEntries > 0)
            {
                memmove(&queueData[i], &queueData[i+1],
                    sizeof(QueueData) * afterEntries);
            }
            numQueues--;
            return;
        }
    }
}


// /**
// FUNCTION   :: DiffservScheduler::swapQueue
// LAYER      ::
// PURPOSE    :: This function substitutes the newly initialized queue
//               passed into this function, for the existing queue in
//               the scheduler, of type priority.  If there are packets
//               in the existing queue, they are transferred one-by-one
//               into the new queue.  This is done to attempt to replicate
//               the state of the queue, as if it had been the operative
//               queue all along.  This can result in additional drops of
//               packets that had previously been stored, and for
//               which there may be physical space in the queue.
//               If there is no such queue, it is added.
// PARAMETERS ::
// + queue : Queue* : Pointer of the Queue class
// + priority : const int : Priority of the queue
// RETURN     :: void : Null
// **/

void DiffservScheduler::swapQueue(Queue* queue, const int priority)
{
    int qIndex = GenericPacketClassifier(this, priority);

    // First check inner(Second) scheduler has the control on that
    // particular priority queue,
    // if yes, then call removeQueue() of inner(Second) scheduler
    // if not, call removeQueue() of outer(Main) scheduler

    if ((innerScheduler != NULL) &&
        (qIndex < (*innerScheduler).numQueue()))
    {
        (*innerScheduler).swapQueue(queue, priority);
        return;
    }
    else
    {
        (*outerScheduler).swapQueue(queue, priority);
        return;
    }
   
}


// /**
// FUNCTION   :: DiffservScheduler::finalize
// LAYER      ::
// PURPOSE    :: This function prototype outputs the final statistics for
//               this scheduler. The layer, protocol, interfaceAddress, and
//               instanceId parameters are given to IO_PrintStat with each
//               of the queue's statistics.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + layer : const char* : The layer string
// + interfaceIndex : const int : Interface index
// + invokingProtocol : const char* : The protocol string
// RETURN     :: void : Null
// **/

void DiffservScheduler::finalize(
    Node* node,
    const char* layer,
    const int interfaceIndex,
    const char *invokingProtocol,
    const char* splStatStr)
{
    if (schedulerStatEnabled == FALSE)
    {
        return;
    }

    if (innerScheduler != NULL)
    {
        (*innerScheduler).finalize(node, layer, interfaceIndex, "Diffserv");
    }

    (*outerScheduler).finalize(node, layer, interfaceIndex, "Diffserv");
}


//--------------------------------------------------------------------------
// DiffservScheduler Class Constructor
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: DiffservScheduler::DiffservScheduler
// LAYER      ::
// PURPOSE    :: Constractor of DiffservScheduler class to initialize
//               its own data members
// PARAMETERS ::
// + numIntfQueues : int : The numIntfQueues parameter
// + dsSecondSchedulerStr : int : The dsSecondSchedulerStr parameter
// + enableSchedulerStat : BOOL : The DIFFSERVE Scheduler statistics.
// + graphDataStr : const char : The graphDataStr string.
// RETURN     ::

DiffservScheduler::DiffservScheduler(
    int numIntfQueues,
    const char dsSecondSchedulerStr[],
    BOOL enableSchedulerStat,
    const char graphDataStr[])
{
    queueData = NULL;
    numQueues = 0;
    maxQueues = 0;
    infoFieldSize = 0;
    packetsLostToOverflow = 0;
    schedulerStatEnabled = enableSchedulerStat;

    ERROR_Assert(numIntfQueues > 0,
        "Number  of Queues <= 0 under Diffserv Scheduler");

    ERROR_Assert(numIntfQueues <= DIFFSERV_MAX_NUM_QUEUE,
        "DiffServ: Not support more than EIGHT queues");

    // First decided number of queues in Inner and Outer scheduler.
    if (numIntfQueues > DIFFSERV_FIRST_SCHED_MIN_QUEUE)
    {
        if (numIntfQueues > DIFFSERV_SECOND_SCHED_MAX_QUEUE)
        {
            innerSchedNumQueue = DIFFSERV_SECOND_SCHED_MAX_QUEUE;
            outerSchedNumQueue = numIntfQueues - innerSchedNumQueue;
        }
        else
        {
            outerSchedNumQueue = DIFFSERV_FIRST_SCHED_MIN_QUEUE;
            innerSchedNumQueue = numIntfQueues - outerSchedNumQueue;
        }
    }
    else
    {
        outerSchedNumQueue = DIFFSERV_FIRST_SCHED_MIN_QUEUE;
        innerSchedNumQueue = 0;
    }

    if (innerSchedNumQueue)
    {
        // Initialize Diffserv inner scheduler for the node
        if (!strcmp(dsSecondSchedulerStr, "WEIGHTED-ROUND-ROBIN"))
        {
            innerScheduler = new WrrScheduler(enableSchedulerStat,
                                              "NA");
        }
        else if (!strcmp(dsSecondSchedulerStr, "WEIGHTED-FAIR"))
        {
            innerScheduler = new WfqScheduler(enableSchedulerStat,
                                              "NA");
        }
        else
        {
            char errStr[MAX_STRING_LENGTH] = {0};
            sprintf(errStr, "DS-SECOND-SCHEDULER: must be specified"
                            " and it should be either "
                            "WEIGHTED-FAIR or WEIGHTED-ROUND-ROBIN");
            ERROR_Assert(FALSE, errStr);
        }

        if (innerScheduler == NULL)
        {
            char errStr[MAX_STRING_LENGTH] = {0};
            sprintf(errStr, "DiffServ: Failed to assign memory for"
                " scheduler %s", dsSecondSchedulerStr);
            ERROR_Assert(FALSE, errStr);
        }
    }
    else
    {
        innerScheduler = NULL;
    }

    // Initialize Diffserv outer scheduler for the node
    outerScheduler = new StrictPriorityScheduler(enableSchedulerStat,
                                                 "NA");

    if (outerScheduler == NULL)
    {
        char errStr[MAX_STRING_LENGTH] = {0};
        sprintf(errStr, "DiffServ: Failed to assign memory for"
            " scheduler STRICT-PRIORITY\n");
        ERROR_Assert(FALSE, errStr);
    }

    schedGraphStatPtr = NULL;
    // Initialize graphdata collection module
    if (strcmp(graphDataStr, "NA"))
    {
        SchedGraphStat* graphPtr = new SchedGraphStat(graphDataStr);
        schedGraphStatPtr = (SchedGraphStat*) graphPtr;

        if (schedGraphStatPtr == NULL)
        {
            printf("Memory allocation failed\n");
        }
    }
}


// /**
// FUNCTION   :: DIFFSERV_SCHEDULER_Setup
// LAYER      ::
// PURPOSE    :: This function runs Diffserv Scheduler Initialization
///              routine
// PARAMETERS ::
// + scheduler : DiffservScheduler** : The retrieved Diffserve scheduler
//                                     pointer
// + numPriorities : int : Number of queues
// + dsSecondSchedulerStr[] : const char : The dsSecondSchedulerStr string
// + enableSchedulerStat : BOOL : Diffserv scheduler statistics
// + graphDataStr : const char* : The graphDataStr parameter
// RETURN     :: void : Null
// **/

void DIFFSERV_SCHEDULER_Setup(
    DiffservScheduler** scheduler,
    int numPriorities,
    const char dsSecondSchedulerStr[],
    BOOL enableSchedulerStat,
    const char* graphDataStr)
{
    *scheduler = new DiffservScheduler(numPriorities,
                                       dsSecondSchedulerStr,
                                       enableSchedulerStat,
                                       graphDataStr);
}


// /**
// FUNCTION   :: ReadDiffservSchedulerConfiguration
// LAYER      ::
// PURPOSE    :: Read Diffserv Scheduler Configuration.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interfaceAddress : const Address* : Interface Address
// + nodeInput : const NodeInput* : pointer to nodeInput
// + secondSchedTypeString : char* : The secondSchedTypeString parameter
// RETURN     :: void : Null
// **/

void ReadDiffservSchedulerConfiguration(
    Node* node,
    int interfaceIndex,
    const NodeInput* nodeInput,
    char* secondSchedTypeString)
{
    BOOL wasFound = FALSE;

    // Initialize Diffserv inner scheduler for the node
    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "DS-SECOND-SCHEDULER",
        &wasFound,
        secondSchedTypeString);

    if (wasFound &&
        strcmp(secondSchedTypeString, "WEIGHTED-ROUND-ROBIN") &&
        strcmp(secondSchedTypeString, "WEIGHTED-FAIR"))
    {
        char errStr[MAX_STRING_LENGTH] = {0};
        sprintf(errStr, "Diffserv: Unknown DS-SECOND-SCHEDULER Specified\n"
            "\t\t Supported DS-SECOND-SCHEDULER are WEIGHTED-ROUND-ROBIN"
            " | WEIGHTED-FAIR\n");
        ERROR_Assert(FALSE, errStr);
    }
}


// /**
// FUNCTION   :: DiffservScheduler::~DiffservScheduler
// LAYER      ::
// PURPOSE    :: This function will free the memory in the
//               DiffservScheduler
// PARAMETERS ::
// RETURN     :: void : Null
// **/
DiffservScheduler::~DiffservScheduler() 
{
    delete []queueData;
}
