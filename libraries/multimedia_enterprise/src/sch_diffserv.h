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


// /**
// PROTOCOL :: Queue-Scheduler
//
// SUMMARY  :: Differentiated Services (DiffServ) is an architecture for
// deploying IP QoS (Quality of Service), where QoS is a set of service
// requirements (e.g., bandwidth, link delay) to be met by the network
// during transmission of traffic. This architecture is flexible and allows
// for either end-to-end QoS or intra-domain QoS by implementing complex
// classification and mapping functions at the network boundary or access
// points. Within the DiffServ network, packet behavior is regulated by
// classification and mapping.
//
// LAYER ::
//
// STATISTICS::
//
// CONFIG_PARAM ::
//
// VALIDATION :: $QUALNET_HOME/verification/diffserv
//
// IMPLEMENTED_FEATURES :: When it enters the network, flow of traffic is
// classified and assigned a PHB (Per-Hop Behavior) based on that
// classification. PHB is the externally observable forwarding treatment
// that packets with the same DiffServ Code Point (DSCP) receive from a
// network node. The contents of the DSCP are the six most significant bits
// in the IP header Type of Service field. Within the IP packet, the DSCP
// tells how the packet should be treated at each network node. Additionally
// the mapping of DSCP to PHB is configurable, and DSCP may be re-marked as
// it passes into a DiffServ network. Re-marking of DSCP allows for the
// variable treatment of packets based on network specifications or desired
// levels of service.
//
// OMITTED_FEATURES :: Same as bellow
//
// ASSUMPTIONS ::
// + We take two standard PHB, Assure forwarding (AF) and Expedite
//   forwarding (EF). Those packets, which are not in one of these two PHB
//   groups, consider as best effort forwarding (BE).
// + We assume Diffserv apply only on unicast packets.
// + In the rfc 2475 of Diffserv tell the concept of traffic stream, which
//   is the set of mrico flows, but we use only packet concept in existing
//   Qualnet.
// + In each output interface Diffserv takes the Scheduler as the
//   combination of Outer (Main) and Inner (Second) Scheduler. It uses
//   Strict priority as Outer and WFQ/WRR as Inner Scheduler.

//
// STANDARD :: RFC 2475, RFC 2597, RFC 2598
//
// RELATED ::
// **/


#ifndef SCHEDULER_DIFFSERV_H
#define SCHEDULER_DIFFSERV_H

#include "if_scheduler.h"

// /**
// CONSTANT    :: DIFFSERV_MAX_NUM_QUEUE : 8
// DESCRIPTION :: Maximum number of queue for DIFFSERV Scheduler
// **/

#define DIFFSERV_MAX_NUM_QUEUE              8

// /**
// CONSTANT    :: DIFFSERV_FIRST_SCHED_MIN_QUEUE : 1
// DESCRIPTION :: Minimum number of queue for inner and outer scheduler
// **/

#define DIFFSERV_FIRST_SCHED_MIN_QUEUE      1

// /**
// CONSTANT    :: DIFFSERV_SECOND_SCHED_MAX_QUEUE : 5
// DESCRIPTION :: Maximum number of queue for inner and outer scheduler.
// **/

#define DIFFSERV_SECOND_SCHED_MAX_QUEUE     5


// /**
// CLASS  : DiffservScheduler
// PURPOSE: Diffserv Scheduler Class
// **/

class DiffservScheduler : public Scheduler
{
  protected:
    int         innerSchedNumQueue;
    int         outerSchedNumQueue;
    Scheduler*  innerScheduler;
    Scheduler*  outerScheduler;

  public:
    DiffservScheduler(int numIntfQueues,
                      const char dsSecondSchedulerStr[],
                      BOOL enableSchedulerStat,
                      const char graphDataStr[]);
    virtual ~DiffservScheduler();
    virtual void insert(Message* msg,
                      BOOL* QueueIsFull,
                      const int queueIndex,
                      const void* infoField,
                      const clocktype currentTime);

    virtual void insert(Message* msg,
                          BOOL* QueueIsFull,
                          const int queueIndex,
                          const void* infoField,
                          const clocktype currentTime,
                          TosType* tos);

      virtual void insert(Node* node,
                          int interfaceIndex,
                          Message* msg,
                          BOOL* QueueIsFull,
                          const int queueIndex,
                          const void* infoField,
                          const clocktype currentTime);

      virtual void insert(Node* node,
                          int interfaceIndex,
                          Message* msg,
                          BOOL* QueueIsFull,
                          const int queueIndex,
                          const void* infoField,
                          const clocktype currentTime,
                          TosType* tos);


    virtual BOOL retrieve(const int priority,
                        const int index,
                        Message** msg,
                        int* msgPriority,
                        const QueueOperation operation,
                        const clocktype currentTime);

    virtual BOOL isEmpty(const int priority);

    virtual int bytesInQueue(const int priority);

    virtual int numberInQueue(const int priority);

    virtual int addQueue(Queue* queue,
                          const int priority = ALL_PRIORITIES,
                          const double weight = 1.0);

    virtual void removeQueue(const int priority);

    virtual void swapQueue(Queue* queue, const int priority);

    virtual void finalize(Node* node,
                      const char* layer,
                      const int interfaceIndex,
                      const char* invokingProtocol = "Diffserv",
                      const char* splStatStr = NULL);
};


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
    const char* graphDataStr = "NA");

// /**
// FUNCTION   :: ReadDiffservSchedulerConfiguration
// LAYER      ::
// PURPOSE    :: Read Diffserv Scheduler Configuration.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interfaceIndex : int : Interface Index
// + nodeInput : const NodeInput* : pointer to nodeInput
// + secondSchedTypeString : char* : The secondSchedTypeString parameter
// RETURN     :: void : Null
// **/

void ReadDiffservSchedulerConfiguration(
    Node* node,
    int interfaceIndex,
    const NodeInput* nodeInput,
    char* secondSchedTypeString);

#endif //SCHEDULER_DIFFSERV_H
