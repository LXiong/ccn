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
#include <ctype.h>
#include <time.h>

#include "api.h"
#include "partition.h"
#include "external.h"
#include "external_util.h"
#include "scheduler.h"

#include "WallClock.h"
#include "gui.h"

#ifdef VRLINK_INTERFACE
#include "vrlink.h"
#endif /* VRLINK_INTERFACE */

#ifdef TESTTHREAD_INTERFACE
#include "testthread.h"
#endif /* TESTTHREAD_INTERFACE */

#ifdef DXML_INTERFACE
#include "dxml.h"
#endif /* DXML_INTERFACE */

#ifdef IPNE_INTERFACE
#include "ipnetworkemulator.h"
#include "voip_interface.h"
#endif /* IPNE_INTERFACE */
#ifdef AUTO_IPNE_INTERFACE
#include "auto-ipnetworkemulator.h"
#include "ipne_qualnet_interface.h"
#endif

#ifdef GATEWAY_INTERFACE
#include "internetgateway.h"
#endif

#ifdef TUTORIAL_INTERFACE
#include "interfacetutorial.h"
#endif /* TUTORIAL_INTERFACE */

#ifdef HITL_INTERFACE
#include "hitl.h"
#endif /* HITL_INTERFACE */

#ifdef SOCKET_INTERFACE
#include "socket-interface.h"
#endif

#ifdef ADDON_NGCNMS
#include "ngc_stats.h"
#endif

#ifdef EXATA
#include "record_replay.h"
#endif
#ifdef HLA_INTERFACE
#include "hla.h"
#endif /* HLA_INTERFACE */

#ifdef DIS_INTERFACE
#include "dis.h"
#endif /* DIS_INTERFACE */

#ifdef QSH_INTERFACE
#include "qsh_interface.h"
#endif /* QSH_INTERFACE */

#ifdef MINI_PARSER
#include "configparser.h"
#endif

#ifdef AGI_INTERFACE
#include "agi_interface.h"
#endif

#ifdef UPA_INTERFACE
#include "qualnet_upa.h"
#endif


#ifdef PAS_INTERFACE
#include "packet_analyzer.h"
#endif

#ifdef RT_INDICATOR_INTERFACE
#include "rt_indicator.h"
#endif

#ifdef ADDON_MA
#include "ma_interface.h"
#endif

#ifdef ADDON_JSR
#include "jsr_interface.h"
#endif

#ifdef JNE_BLACKSIDE_INTEROP_INTERFACE
#include "mimulticast_adapter.h"
#endif


#ifdef INTERFACE_JNE_JREAP
#include "jreap-api.h"
#endif // JREAP

#ifdef OCULA_INTERFACE
#include "spinterface.h"
#endif

//---------------------------------------------------------------------------
// Static Functions
//---------------------------------------------------------------------------

static void ProcessMobilityEvent(Node* node, Message* msg)
{
    // Notes:
    // - The velocity vector is assumed to be using the same distance
    //   units as is used for the position and terrain coordinate system.
    // - The speed is always assumed to be in m/s.

    // This function calls MOBILITY_InsertANewEvent() to schedule the change
    // in mobility state at current simTime + 1.
    // (This is the minimum time permitted by MOBILITY_InsertANewEvent().)

    // Don't perform the above and exit early if:

    // - there is a pending change to mobility state (normally, that means
    //   this fcn was just called at the current simTime)
    // - this is an estimated position event (from dead reckoning) AND the
    //   node's lastExternalTrueMobilityTime is > the mobility event's
    //   truePositionTime (this means the estimated position event is
    //   outdated / has been superseded by a recent true-position event)

    // Otherwise:

    // - if this is a velocity-only event, then estimate the position (for
    //   simTime + 1) based on the node's lastExternalMobilityTime and
    //   lastExternalVelocity.  Leave the orientation unchanged.
    // - if the mobility event's delaySpeedCalculation flag was set, then
    //   compute the speed now that the node's position is known (either an
    //   estimated or true position)
    // - schedule the change in mobility state at simTime + 1
    // - set the node's lastExternalMobilityTime to simTime + 1
    // - set the node's lastExternalTrueMobilityTime to simTime + 1, IF this
    //   is a true-position event (check mobility event's isTruePosition
    //   variable)
    // - set the node's lastExternal{Velocity,Speed}
    // - schedule the next estimated position event, IF this an event with
    //   a non-zero velocity vector and non-zero speed
    //   - the new next estimated position event should retain the original
    //     truePositionTime value
    //   - we want to avoid the situation where the node is moving so slowly
    //     and/or the distance granularity so large that the next move is
    //     scheduled at a negative time.

    TerrainData*  terrainData  = NODE_GetTerrainPtr(node);
    MobilityData* mobilityData = node->mobilityData;

    if (mobilityData->next->time != CLOCKTYPE_MAX)
    {
#if !defined(HITL_INTERFACE) && !defined(ADDON_BOEINGFCS)
        char warningString[MAX_STRING_LENGTH];
        sprintf(
            warningString,
            "Received several mobility events for node %u,"
            " only the first mobility event will be used.", node->nodeId);
        ERROR_ReportWarning(warningString);
#endif
        return;
    }

    EXTERNAL_MobilityEvent* mobilityEvent
        = (EXTERNAL_MobilityEvent*) MESSAGE_ReturnInfo(msg);

    // Ignore this update if this is not a true position (it is a dead reckoning position)
    // and we have a newer true position
    if (!mobilityEvent->isTruePosition
        && mobilityData->lastExternalTrueMobilityTime > mobilityEvent->truePositionTime)
    {
        return;
    }

    clocktype simTime = node->getNodeTime();

    clocktype mobilityEventTime = simTime + 1;

    if (mobilityEvent->velocityOnly)
    {
        assert(mobilityEvent->isTruePosition);
        assert(mobilityData->lastExternalSpeed >= 0.0);

        // Set mobility-event position to current position, then update
        // this position if the node was in motion.

        mobilityEvent->coordinates = mobilityData->current->position;

        if (mobilityData->lastExternalSpeed > 0.0)
        {
            const double delay
                = (double)(mobilityEventTime - mobilityData->lastExternalMobilityTime);

            assert(delay > 0);

            const double dblDelay = delay / SECOND;

            mobilityEvent->coordinates.common.c1 +=
                mobilityData->lastExternalVelocity.common.c1 * dblDelay;

            mobilityEvent->coordinates.common.c2 +=
                mobilityData->lastExternalVelocity.common.c2 * dblDelay;

            mobilityEvent->coordinates.common.c3 +=
                mobilityData->lastExternalVelocity.common.c3 * dblDelay;

            if (terrainData->getCoordinateSystem() == LATLONALT) {
                COORD_NormalizeLongitude(&(mobilityEvent->coordinates));
            }

            if (node->mobilityData->groundNode == TRUE)
            {
                TERRAIN_SetToGroundLevel(
                    terrainData, &(mobilityEvent->coordinates));
            }
        }

        mobilityEvent->orientation = mobilityData->current->orientation;
    }

    if (mobilityEvent->delaySpeedCalculation)
    {
        if (NODE_GetTerrainPtr(node)->getCoordinateSystem() == LATLONALT)
        {
            // Get the GCC (geocentric Cartesian) coordinates for the
            // current and next position (after 1 second), then calculate
            // the speed in m/s.
            //
            // The calculation assumes that the velocity vector is
            // provided in geodetic (latlonalt) coordinates.

            Coordinates currentGeodetic = mobilityEvent->coordinates;
            Coordinates currentGcc;

            COORD_GeodeticToGeocentricCartesian(
                &currentGeodetic,
                &currentGcc);

            Coordinates nextGeodetic = currentGeodetic;
            Coordinates nextGcc;

            nextGeodetic.common.c1 += mobilityEvent->velocity.common.c1;
            nextGeodetic.common.c2 += mobilityEvent->velocity.common.c2;
            nextGeodetic.common.c3 += mobilityEvent->velocity.common.c3;

            if (terrainData->getCoordinateSystem() == LATLONALT) {
                COORD_NormalizeLongitude(&nextGeodetic);
            }

            COORD_GeodeticToGeocentricCartesian(
                &nextGeodetic,
                &nextGcc);

            mobilityEvent->speed
                = sqrt(pow(nextGcc.common.c1
                           - currentGcc.common.c1, 2)
                       + pow(nextGcc.common.c2
                             - currentGcc.common.c2, 2)
                       + pow(nextGcc.common.c3
                             - currentGcc.common.c3, 2));
        }
        else
        {
            mobilityEvent->speed
                = sqrt(pow(mobilityEvent->velocity.common.c1, 2)
                       + pow(mobilityEvent->velocity.common.c2, 2)
                       + pow(mobilityEvent->velocity.common.c3, 2));
        }//if//
    }//if//

    mobilityData->lastExternalMobilityTime = simTime;

    if (mobilityEvent->isTruePosition)
    {
        mobilityData->lastExternalTrueMobilityTime = simTime;
        mobilityData->lastExternalCoordinates = mobilityEvent->coordinates;
    }

    MOBILITY_InsertANewEvent(
        node,
        mobilityEventTime,
        mobilityEvent->coordinates,
        mobilityEvent->orientation,
        mobilityEvent->speed);

#ifdef DEBUG
    char mobilityEventTimeString[20];
    ctoa(mobilityEventTime, mobilityEventTimeString);

    printf(
        "EXT: Moving node %u (%.6f, %.6f, %.6f) (%d), simTime %s\n",
        node->nodeId,
        mobilityEvent->coordinates.common.c1,
        mobilityEvent->coordinates.common.c2,
        mobilityEvent->coordinates.common.c3,
        (int) mobilityEvent->speed,
        mobilityEventTimeString);
#endif /* DEBUG */

    // ** All remaining code is for the next estimated position event **

    // Return if this update has no velocity component
    if (mobilityEvent->speed == 0.0
        || (mobilityEvent->velocity.common.c1 == 0.0
            && mobilityEvent->velocity.common.c2 == 0.0
            && mobilityEvent->velocity.common.c3 == 0.0))
    {
        memset(
            &mobilityData->lastExternalVelocity,
            0,
            sizeof(mobilityData->lastExternalVelocity));

        mobilityData->lastExternalSpeed = 0.0;

        return;
    }

    mobilityData->lastExternalVelocity = mobilityEvent->velocity;
    mobilityData->lastExternalSpeed = mobilityEvent->speed;

    // Compute delay based on provided speed value (which is always
    // in meters per second) and distance granularity (also always
    // in meters).

    const double dblDelay = mobilityData->distanceGranularity
                             / mobilityEvent->speed;
    const clocktype delay = (clocktype) (dblDelay * SECOND);

    // Total delay in seconds since last true (non dead-reckoning)
    // position
    double totalDblDelay = (double) (simTime - mobilityData->lastExternalTrueMobilityTime + delay) / SECOND;

    // Also return early if the computed delay overflows the variable
    // (e.g., if the node has a non-zero speed but is still moving very
    // slowly).

    if (mobilityEventTime + delay < 0)
    {
        return;
    }

    Message* newMsg = MESSAGE_Duplicate(node, msg);

    EXTERNAL_MobilityEvent* newMobilityEvent
        = (EXTERNAL_MobilityEvent*) MESSAGE_ReturnInfo(newMsg);

    newMobilityEvent->isTruePosition = FALSE;
    newMobilityEvent->velocityOnly = FALSE;

    // Compute new position based on time since the original position update.
    // (Note the velocity vector is assumed to be in the same units as the
    // position.) (Note also that orientation is not dead reckoned and an
    //acceleration vector is not used.)

    newMobilityEvent->coordinates.common.c1 =
        mobilityData->lastExternalCoordinates.common.c1 +
        mobilityEvent->velocity.common.c1 * totalDblDelay;

    newMobilityEvent->coordinates.common.c2 =
        mobilityData->lastExternalCoordinates.common.c2 +
        mobilityEvent->velocity.common.c2 * totalDblDelay;

    newMobilityEvent->coordinates.common.c3 =
        mobilityData->lastExternalCoordinates.common.c3 +
        mobilityEvent->velocity.common.c3 * totalDblDelay;

    // Bound to terrain.
    if (terrainData->getCoordinateSystem() == LATLONALT) {
        COORD_NormalizeLongitude(&(newMobilityEvent->coordinates));
    }

    TERRAIN_BoundCoordinatesToTerrain(
        terrainData,
        &(newMobilityEvent->coordinates),
        FALSE);  // printWarnings

    // If node sticks to ground, fix node to ground at next estimated
    // position.

    if (node->mobilityData->groundNode == TRUE)
    {
        TERRAIN_SetToGroundLevel(
            terrainData, &(newMobilityEvent->coordinates));
    }

    MESSAGE_Send(node, newMsg, delay);
}

// /**
// API       :: GetNextInternalEventTime
// PURPOSE   :: Get the next internal event on the given partition.  This
//              includes both regular events and mobility events.
// PARAMETERS::
// + partitionData : PartitionData* : Pointer to the partition
// RETURN    :: clocktype : The next internal event
// **/
clocktype GetNextInternalEventTime(PartitionData* partitionData)
{
    // This function should do the same thing as what the function of the same
    // name does in addons/par/partition_private.cpp.
    // If the other function is modified, the function in this file should be
    // updated to match it.

    return MIN(
        SCHED_NextEventTime(partitionData),
        partitionData->mobilityHeap.minTime);
}

// /**
// API       :: EXTERNAL_RTTimeFunction
// PURPOSE   :: This function can be used as the interface's "time"
//              function.  It counts the number of real elapsed
//              nanoseconds from the first call to this function.
//              Allows for pauses in simulation execution.
// PARAMETERS::
// + iface : EXTERNAL_Interface* : Pointer to the interface
// RETURN    :: clocktype : The next internal event
// **/
static clocktype EXTERNAL_RTTimeFunction(EXTERNAL_Interface *iface)
{
    assert(iface != NULL);

    if (iface->initializeTime == 0)
    {
        iface->initializeTime =
            iface->partition->wallClock->getRealTime ();
        return 0;
    }
    else
    {
        // This will return the current time (adjusted for pauses,
        // so if we are currently paused this value won't be increasing)
        return (iface->partition->wallClock->getRealTime ()
                    - iface->initializeTime);
    }
}

// /**
// API       :: EXTERNAL_RTSimulationHorizonFunction
// PURPOSE   :: This function can be used as the interface's "simulation
//              horizon" function.  It sets the simulation horizon to the
//              number of real elapsed nanoseconds from the first call to
//              EXTERNAL_RTTimeFunction plus some lookahead value.
// PARAMETERS::
// + iface : EXTERNAL_Interface* : Pointer to the interface
// RETURN    :: clocktype : The next internal event
// **/
static void EXTERNAL_RTSimulationHorizonFunction(EXTERNAL_Interface *iface)
{
    clocktype tempHorizon;

    // Do not set horizon if in lookahead period
    if (EXTERNAL_IsInWarmup(iface))
    {
        return;
    }

    // Set the simulation horizon.  Make sure it never decreases.
    tempHorizon = EXTERNAL_RTTimeFunction(iface) + iface->lookahead;
    if (EXTERNAL_QueryWarmupTime(iface) > 0)
    {
        tempHorizon += EXTERNAL_QueryWarmupTime(iface);
    }
    if (tempHorizon > iface->horizon)
    {
        iface->horizon = tempHorizon;
    }
}

// /**
// API       :: EXTERNAL_ReceiveThread
// PURPOSE   :: This function is used when an external interface is running
//              in threaded mode.  This function is launched as a separate
//              thread and will continue calling the interface's receive
//              function as long as the interface is active.
// PARAMETERS::
// + ifaceVoid : void* : Pointer to the interface
// RETURN    :: clocktype : The next internal event
// **/
static void* EXTERNAL_ReceiveThread(void* ifaceVoid)
{
    EXTERNAL_Interface* iface = (EXTERNAL_Interface*) ifaceVoid;

    ERROR_Assert(
        iface->receiveFunction != NULL,
        "Should not start receive thread without receive function");

    // Keep calling receive function
    while (iface->running)
    {
        (iface->receiveFunction)(iface);
    }

    return NULL;
}

// /**
// API       :: EXTERNAL_ForwardThread
// PURPOSE   :: This function is used when an external interface is running
//              in threaded mode.  This function is launched as a separate
//              thread.  It will block until there is data ready to be
//              forwarded.  It will call the interface's forward function
//              then go back to sleep.
// PARAMETERS::
// + ifaceVoid : void* : Pointer to the interface
// RETURN    :: clocktype : The next internal event
// **/
static void* EXTERNAL_ForwardThread(void* ifaceVoid)
{

    EXTERNAL_Interface* iface = (EXTERNAL_Interface*) ifaceVoid;
    EXTERNAL_ThreadedForward f;

    ERROR_Assert(
        iface->forwardFunction != NULL,
        "Should not start forward thread without forward function");

    // Get elements to forward.  pop() call blocks when forwards queue is
    // empty.  The ordering of events in this loop ensures that the
    // interface will exit gracefully if the forwards queue has signal()
    // called (e.g., when the interface is finalized)
    iface->forwards->pop(f);
    while (iface->running || !iface->forwards->empty())
    {
        (iface->forwardFunction)(iface, iface->partition->firstNode,
                                 f.forwardData, f.forwardSize);
        MEM_free(f.forwardData);
        iface->forwards->pop(f);
    }

    return NULL;
}

// /**
// API       :: EXTERNAL_ReceiveForwardThread
// PURPOSE   :: This function is used when an external interface is running
//              in threaded mode.  This function is launched as a separate
//              thread and will continue calling the interface's receive
//              function and forward function as long as the interface is
//              active.
// PARAMETERS::
// + ifaceVoid : void* : Pointer to the interface
// RETURN    :: clocktype : The next internal event
// **/
static void* EXTERNAL_ReceiveForwardThread(void* ifaceVoid)
{

    EXTERNAL_ThreadedForward f;
    EXTERNAL_Interface* iface = (EXTERNAL_Interface*) ifaceVoid;
    bool receive = iface->receiveFunction != NULL;
    bool forward = iface->forwardFunction != NULL;

    ERROR_Assert(
        receive || forward,
        "Should not start ReceiveForward thread without receive or forward function");

    // Keep calling receive and forward function
    while (iface->running)
    {
        if (receive)
        {
            (iface->receiveFunction)(iface);
        }

        if (forward)
        {
            while (!iface->forwards->empty())
            {
                // Get next element to forward
                iface->forwards->pop(f);
                (iface->forwardFunction)(iface, iface->partition->firstNode,
                                         f.forwardData, f.forwardSize);
                MEM_free(f.forwardData);
            }
        }
    }

    return NULL;
}

//---------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------

EXTERNAL_Interface* EXTERNAL_RegisterExternalInterface(
    EXTERNAL_InterfaceList *list,
    const char *name,
    EXTERNAL_PerformanceParameters params,
    ExternalInterfaceType type)
{
    EXTERNAL_Interface *n;

    assert(list != NULL);
    assert(name != NULL);

    // Create a new list node
    n = (EXTERNAL_Interface*) MEM_malloc(sizeof(EXTERNAL_Interface));
    memset(n, 0, sizeof(EXTERNAL_Interface));

    // Check if this interface is a cpu hog
    if (params & EXTERNAL_CPU_HOG)
    {
        list->cpuHog = TRUE;
    }

    // Check if this interface should not call receive during warmup
    if (params & EXTERNAL_NO_WARMUP_RECEIVE)
    {
        n->warmupNoReceive = TRUE;
    }
    else
    {
        n->warmupNoReceive = FALSE;
    }

    // Fill in external interface information
    n->partition = list->partition;
    strcpy(n->name, name);
    n->type = type;
    if (type != EXTERNAL_TYPE_NONE)
    {
        n->partition->interfaceTable [int (type)] = n;
    }
    n->interfaceId = ++list->numInterfaces;
    n->params = params;
    n->interfaceList = list;

    // Add to the front of the list
    n->next = list->interfaces;
    list->interfaces = n;

    // Check if this is a threaded interface
    if ((params & EXTERNAL_THREADED)
        || (params & EXTERNAL_THREADED_MULTIPLE_THREADS)
        || (params & EXTERNAL_THREADED_SINGLE_THREAD))
    {
        n->threaded = TRUE;
        n->running = TRUE;

        // Create queues based on threading type
        if (params & EXTERNAL_THREADED_MULTIPLE_THREADS)
        {
            n->receiveMessages = new EXTERNAL_MultipleWritersQueue<EXTERNAL_ThreadedMessage>;
            n->forwards = new EXTERNAL_BlockingQueue<EXTERNAL_ThreadedForward>;
        }
        else if (params & EXTERNAL_THREADED_SINGLE_THREAD)
        {
            n->receiveMessages = new EXTERNAL_LockFreeQueue<EXTERNAL_ThreadedMessage>;
            n->forwards = new EXTERNAL_LockFreeQueue<EXTERNAL_ThreadedForward>;
        }
        else
        {
            n->receiveMessages = new EXTERNAL_LockFreeQueue<EXTERNAL_ThreadedMessage>;
            n->forwards = new EXTERNAL_BlockingQueue<EXTERNAL_ThreadedForward>;
        }
        list->messageQueues.push_back(n->receiveMessages);
    }

    // Return a pointer to the interface
    return n;
}

EXTERNAL_Interface* EXTERNAL_RegisterExternalInterface(
    EXTERNAL_InterfaceList *list,
    const char *name,
    EXTERNAL_PerformanceParameters params)
{
    return (EXTERNAL_RegisterExternalInterface(list, name, params,
        EXTERNAL_TYPE_NONE));
}

EXTERNAL_Interface* EXTERNAL_GetInterfaceByName(
    EXTERNAL_InterfaceList *list,
    const char *name)
{
    EXTERNAL_Interface *iface;

    // Loop through the list of interfaces
    iface = list->interfaces;
    while (iface != NULL)
    {
        // If the names match return the interface
        if (strcmp(iface->name, name) == 0)
        {
            return iface;
        }

        iface = iface->next;
    }

    // Not found, return NULL
    return NULL;
}

EXTERNAL_Interface* EXTERNAL_GetInterfaceByUniqueId(
    EXTERNAL_InterfaceList *list,
    int uniqueId)
{
    EXTERNAL_Interface *iface;

    // Loop through the list of interfaces
    iface = list->interfaces;
    while (iface != NULL)
    {
        // If the ids match return the interface
        if (iface->interfaceId == uniqueId)
        {
            return iface;
        }

        iface = iface->next;
    }

    // Not found, return NULL
    return NULL;
}

void EXTERNAL_RegisterFunction(
    EXTERNAL_Interface *iface,
    EXTERNAL_FunctionType type,
    EXTERNAL_Function function)
{
    char err[MAX_STRING_LENGTH];

    assert(iface != NULL);

    // Set the function based on the type
    switch (type)
    {
        case EXTERNAL_INITIALIZE:
            iface->initializeFunction = (EXTERNAL_InitializeFunction) function;
            break;

        case EXTERNAL_INITIALIZE_NODES:
            iface->initializeNodesFunction = (EXTERNAL_InitializeNodesFunction) function;
            break;

        case EXTERNAL_TIME:
            iface->timeFunction = (EXTERNAL_TimeFunction) function;
            break;

        case EXTERNAL_SIMULATION_HORIZON:
            iface->simulationHorizonFunction = (EXTERNAL_SimulationHorizonFunction) function;
            break;

        case EXTERNAL_PACKET_DROPPED:
            iface->packetDroppedFunction = (EXTERNAL_PacketDroppedFunction) function;
            break;

        case EXTERNAL_RECEIVE:
            iface->receiveFunction = (EXTERNAL_ReceiveFunction) function;
            break;

        case EXTERNAL_FORWARD:
            iface->forwardFunction = (EXTERNAL_ForwardFunction) function;
            break;

        case EXTERNAL_FINALIZE:
            iface->finalizeFunction = (EXTERNAL_FinalizeFunction) function;
            break;

        default:
            sprintf(err, "Invalid EXTERNAL_FunctionType \"%d\"", type);
            ERROR_ReportWarning(err);
    }
}

void EXTERNAL_SetTimeManagementRealTime(
    EXTERNAL_Interface *iface,
    clocktype lookahead)
{
    // Register the necessary timing functions
    EXTERNAL_RegisterFunction(
        iface,
        EXTERNAL_TIME,
        (EXTERNAL_Function) EXTERNAL_RTTimeFunction);
    EXTERNAL_RegisterFunction(
        iface,
        EXTERNAL_SIMULATION_HORIZON,
        (EXTERNAL_Function) EXTERNAL_RTSimulationHorizonFunction);

    // Set the lookahead time
    EXTERNAL_ChangeRealTimeLookahead(
        iface,
        lookahead);

    PARTITION_SetRealTimeMode (iface->partition, true);
}

void EXTERNAL_ChangeRealTimeLookahead(
    EXTERNAL_Interface *iface,
    clocktype lookahead)
{
    assert(iface != NULL);

    // Update the interface's lookahead time.  Note that this should not
    // cause the interface's simulation horizon to decrease, however this is
    // the responsibility of the simulation horizon function.
    iface->lookahead = lookahead;
}


// /**
// API       :: EXTERNAL_SetWarmupTime
// PURPOSE   :: Sets this interface's warmup time.  The actual warmup time
//              used is the maximum of all interface's.  The default is no
//              warmup time (warmup == -1).  This function must be called
//              before or during the initialize nodes step.  It will have
//              no effect during the simulation.
// PARAMETERS ::
// + iface  : EXTERNAL_Interface* : The external interface
// + warmup : clocktype : The warmup time for this interface
// RETURN   :: void :
// **/
void EXTERNAL_SetWarmupTime(
    EXTERNAL_Interface *iface,
    clocktype warmup)
{
    // Do not set if simulation has started
    if (EXTERNAL_QuerySimulationTime(iface) > 0)
    {
        return;
    }

    iface->partition->interfaceList.numWarmupInterfaces++;
    ERROR_Assert(
        iface->partition->interfaceList.warmupPhase == EXTERNAL_NoWarmup ||
        iface->partition->interfaceList.warmupPhase == EXTERNAL_WaitingToBeginWarmup,
        "WarmuptTime must be set during or before InitializeNodes");
    iface->partition->interfaceList.warmupPhase = EXTERNAL_WaitingToBeginWarmup;

    // Set new warmup time if it is greater than the last
    if (warmup > iface->partition->interfaceList.warmupTime)
    {
        iface->partition->interfaceList.warmupTime = warmup;
        iface->partition->isRealTime = FALSE;
    }
}

// /**
// API       :: EXTERNAL_BeginWarmup
// PURPOSE   :: Each interface that calls EXTERNAL_SetWarmupTime must call
//              EXTERNAL_BeginWarmup when it is ready to enter warmup time.
// PARAMETERS ::
// + iface  : EXTERNAL_Interface* : The external interface
// RETURN   :: void :
// **/
void EXTERNAL_BeginWarmup(EXTERNAL_Interface *iface)
{
    iface->partition->interfaceList.numWarmupInterfaces--;
}

// /**
// API       :: EXTERNAL_QueryWarmupTime
// PURPOSE   :: Get the warmup time for the entire simulation.  Interfaces
//              should use this function to test when warmup time is over.
// PARAMETERS ::
// + iface  : EXTERNAL_Interface* : The external interface
// RETURN   :: clocktype : The inclusive end of warmup time.  -1 if no
//             warmup time.
// **/
clocktype EXTERNAL_QueryWarmupTime(EXTERNAL_Interface *iface)
{
    return iface->partition->interfaceList.warmupTime;
}

// /**
// API       :: EXTERNAL_IsInWarmup
// PURPOSE   :: Check if QualNet is in the warmup phase
// PARAMETERS ::
// + iface  : EXTERNAL_Interface* : The external interface
// RETURN   :: BOOL : TRUE if in warmup, FALSE if not
// **/
BOOL EXTERNAL_IsInWarmup(EXTERNAL_Interface *iface)
{

    return (EXTERNAL_IsInWarmup(iface->partition));

}


BOOL EXTERNAL_IsInWarmup(PartitionData *partitionData)
{
    EXTERNAL_Interface* iface = partitionData->interfaceList.interfaces;
    return EXTERNAL_QuerySimulationTime(iface) <
        EXTERNAL_QueryWarmupTime(iface);

}

BOOL EXTERNAL_WarmupTimeEnabled(EXTERNAL_Interface *iface)
{
    return (EXTERNAL_QueryWarmupTime(iface) != -1);
}


void EXTERNAL_Pause(EXTERNAL_Interface *iface)
{
    iface->partition->wallClock->pause ();
}

void EXTERNAL_Resume(EXTERNAL_Interface *iface)
{
    iface->partition->wallClock->resume ();
}

void EXTERNAL_SetCpuHog(EXTERNAL_Interface *iface)
{
    iface->partition->interfaceList.cpuHog = TRUE;
}

clocktype EXTERNAL_QueryExternalTime(EXTERNAL_Interface *iface)
{
    assert(iface != NULL);

    // Return the value provided by the interface's time function if it
    // exists, EXTERNAL_MAX_TIME otherwise.
    if (iface->timeFunction != NULL)
    {
        return iface->timeFunction(iface);
    }
    else
    {
        return EXTERNAL_MAX_TIME;
    }
}

clocktype EXTERNAL_QuerySimulationTime(EXTERNAL_Interface *iface)
{
    // Return the current simulation time
    return iface->partition->getGlobalTime();
}

void EXTERNAL_SetReceiveDelay(
    EXTERNAL_Interface *iface,
    clocktype delay)
{
    // Set the receive delay
    iface->receiveCallDelay = delay;
}

void EXTERNAL_SendMessage(
    EXTERNAL_Interface *iface,
    Node* node,
    Message* msg,
    clocktype timestamp)
{
    if (msg->mtWasMT)
    {
      //printf("sending mt message\n");
        // This is a mutli-threaded message. Add to outgoing message list.
        // This message will be processed the next time external events are
        // checked.
        EXTERNAL_ThreadedMessage tm;
        tm.node = node;
        tm.msg = msg;
        tm.timestamp = timestamp;
        iface->receiveMessages->push(tm);
    }
    else
    {
        // Sent from main thread.  Send message with delay.
        clocktype delay;
        clocktype simTime = EXTERNAL_QuerySimulationTime(iface);
        if (timestamp > simTime)
        {
            delay = timestamp - simTime;
        }
        else
        {
            delay = 0;
        }

        MESSAGE_Send(
            node,
            msg,
            delay);
    }
}

void EXTERNAL_ForwardData(
    EXTERNAL_Interface *iface,
    Node *node,
    void *forwardData,
    int forwardSize,
    EXTERNAL_ForwardData_ReceiverOpt FwdReceiverOpt)
{

#ifdef IPNE_INTERFACE
    if (strcmp(iface->name,"IPNE") ==0)
    {
        IPNECheckMulticastAndDoNAT(node, iface, forwardData, forwardSize);
    }
#endif

    // Call the forward function if it exists, otherwise report a warning.
    if (iface->forwardFunction != NULL)
    {
        if (iface->threaded)
        {
            EXTERNAL_ThreadedForward f;

            void* data = MEM_malloc(forwardSize);
            memcpy(data, forwardData, forwardSize);

            f.node = node;
            f.forwardData = data;
            f.forwardSize = forwardSize;
            iface->forwards->push(f);
        }
        else
        {
            if (FwdReceiverOpt == EXTERNAL_ForwardDataAssignNodeID_On)
            {
                node->partitionData->EXTERNAL_lastIdToInvokeForward = node->nodeId;
            }
            iface->forwardFunction(iface, node, forwardData, forwardSize);
        }
    }
    else
    {
        ERROR_ReportWarning("Interface does not have a forward function");
    }
}

void EXTERNAL_ForwardDataTimeStamped(
    EXTERNAL_Interface *iface,
    Node *node,
    Message *message,
    clocktype timeStamp)
{
    // Not implemented yet, report error
    ERROR_ReportError("EXTERNAL_ForwardDataTimeStamped not implemented");
}

#if !defined HLA_INTERFACE && !defined VRLINK_INTERFACE
void exitWithErrorOnHlaScenario(NodeInput *nodeInput)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "HLA",
        &retVal,
        buf);

    if (retVal && strcmp(buf, "YES") == 0)
    {
        sprintf(buf, "The HLA interface requires installing the Standard "
                     "Interfaces Library and rebuilding QualNet with HLA "
                     "support");
        ERROR_ReportError(buf);
    }
}
#endif // !HLA_INTERFACE

#if !defined DIS_INTERFACE && !defined VRLINK_INTERFACE
void exitWithErrorOnDisScenario(NodeInput *nodeInput)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "DIS",
        &retVal,
        buf);

    if (retVal && strcmp(buf, "YES") == 0)
    {
        sprintf(buf, "The DIS interface requires installing the Standard "
                     "Interfaces Library and rebuilding QualNet with DIS "
                     "support");
        ERROR_ReportError(buf);
    }
}
#endif // !DIS_INTERFACE

#if !defined VRLINK_INTERFACE
void exitWithErrorOnVRLinkScenario(NodeInput *nodeInput)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "VRLINK",
        &retVal,
        buf);

    if (retVal && strcmp(buf, "YES") == 0)
    {
        sprintf(buf, "The VRLink interface requires rebuilding QualNet with "
                     "VR-Link support");
        ERROR_ReportError(buf);
    }
}
#endif // !DIS_INTERFACE

void EXTERNAL_UserFunctionRegistration(
    EXTERNAL_InterfaceList *list,
    NodeInput *nodeInput)
{
    EXTERNAL_Interface *iface;

    // Always register GUI interface.  If the GUI is active then it will
    // register additional functions in its initializaion function.
    // and if not, it will deactivate itself so that the simulation
    // can run faster.
    iface = EXTERNAL_RegisterExternalInterface(
        list,
        "<Qualnet-Developer-Ide>",
        EXTERNAL_NONE,
        EXTERNAL_QUALNET_GUI);
    EXTERNAL_RegisterFunction(
        iface,
        EXTERNAL_INITIALIZE,
        (EXTERNAL_Function) GUI_EXTERNAL_Registration);

    // begin user-modifiable code

#ifdef DXML_INTERFACE
    if (EXTERNAL_ConfigStringIsYes(nodeInput, "DXML"))
    {
        // Register DXML
        iface = EXTERNAL_RegisterExternalInterface(
            list,
            "DXML",
            EXTERNAL_NONE);

        // Register InitializeNodes function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE_NODES,
            (EXTERNAL_Function) DXML_InitializeNodes);

        // Register Receive function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_RECEIVE,
            (EXTERNAL_Function) DXML_Receive);

        // Register Forward function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FORWARD,
            (EXTERNAL_Function) DXML_Forward);

        // Register Finalize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FINALIZE,
            (EXTERNAL_Function) DXML_Finalize);

        // NOTE: DXML time management will be set depending on config file

        // Set the minimum delay between calls
        EXTERNAL_SetReceiveDelay(
            iface,
            1 * MILLI_SECOND);
    }
#endif /* DXML_INTERFACE */

#ifdef AGI_INTERFACE
    if (EXTERNAL_ConfigStringIsYes(nodeInput, "AGI-INTERFACE"))
    {
        // Register InterfaceTutorial interface
        iface = EXTERNAL_RegisterExternalInterface(
            list,
            "AGIInterface",
            EXTERNAL_NONE,
            EXTERNAL_AGI);

        // Register InitializeNodes function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE,
            (EXTERNAL_Function) AgiInterfaceInitialize);

        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FINALIZE,
            (EXTERNAL_Function) AgiInterfaceFinalize);

        // Register InitializeNodes function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE_NODES,
            (EXTERNAL_Function) AgiInterfaceInitializeNodes);
    }
#endif

#ifdef EXATA
    if (iface->partition->isEmulationMode)
    {

#ifdef GATEWAY_INTERFACE
        if (EXTERNAL_ConfigStringPresent(nodeInput, "INTERNET-GATEWAY"))
        {
            /*******************************************************
             *   Internet gateway Interface
             ******************************************************/
            iface = EXTERNAL_RegisterExternalInterface(
                    list,
                    "INTERNET-GATEWAY",
                    EXTERNAL_CPU_HOG,
                    EXTERNAL_INTERNET_GATEWAY);

            // Register Initialize function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_INITIALIZE,
                    (EXTERNAL_Function) GATEWAY_Initialize);

            // Register InitializeNodes function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_INITIALIZE_NODES,
                    (EXTERNAL_Function) GATEWAY_InitializeNodes);

            // Register Receive function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_RECEIVE,
                    (EXTERNAL_Function) GATEWAY_Receive);

            // Register Forward function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_FORWARD,
                    (EXTERNAL_Function) GATEWAY_Forward);

            // Register Finalize function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_FINALIZE,
                    (EXTERNAL_Function) GATEWAY_Finalize);

            // Set the time management to real-time, with 0 lookahead
            if (list->partition->partitionId == 0)
            {
                EXTERNAL_SetTimeManagementRealTime(
                        iface,
                        0);

                // Set the minimum delay between calls
                EXTERNAL_SetReceiveDelay(
                        iface,
                        1000 * MICRO_SECOND);
            }
        }
#endif
#ifdef AUTO_IPNE_INTERFACE
        /*******************************************************
         *   IPNE (auto or manual) Interface
         ******************************************************/
        BOOL isAutoIpne = TRUE;

        if (EXTERNAL_ConfigStringPresent(nodeInput, "IPNE"))
        {
            char ipneType[MAX_STRING_LENGTH];
            BOOL wasFound;

            IO_ReadString(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "IPNE",
                    &wasFound,
                    ipneType);

            if (wasFound && !strcmp(ipneType, "YES"))
            {
                ERROR_ReportWarning("IPNE feature is deprecated.");
                isAutoIpne = FALSE;
            }
        }

        if (isAutoIpne)
        {
            // Register IPNetworkEmulator
            iface = EXTERNAL_RegisterExternalInterface(
                    list,
                    "IPNE",
                    EXTERNAL_CPU_HOG,
                    EXTERNAL_IPNE);

            // Register Initialize function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_INITIALIZE,
                    (EXTERNAL_Function) AutoIPNE_Initialize);

            // Register InitializeNodes function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_INITIALIZE_NODES,
                    (EXTERNAL_Function) AutoIPNE_InitializeNodes);

            // Register Receive function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_RECEIVE,
                    (EXTERNAL_Function) AutoIPNE_Receive);

            // Register Forward function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_FORWARD,
                    (EXTERNAL_Function) AutoIPNE_Forward);

            // Register Finalize function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_FINALIZE,
                    (EXTERNAL_Function) AutoIPNE_Finalize);

            // Set the time management to real-time, with 0 lookahead
            if (list->partition->partitionId == 0)
            {
                EXTERNAL_SetTimeManagementRealTime(
                        iface,
                        0);

                // Set the minimum delay between calls
                EXTERNAL_SetReceiveDelay(
                        iface,
                        1 * MILLI_SECOND);
            }
            iface->partition->isAutoIpne = TRUE;
        }
        else
        {
            // Register IPNetworkEmulator
            iface = EXTERNAL_RegisterExternalInterface(
                    list,
                    "IPNE",
                    EXTERNAL_CPU_HOG,
                    EXTERNAL_IPNE);

            // Register Initialize function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_INITIALIZE,
                    (EXTERNAL_Function) IPNE_Initialize);

            // Register InitializeNodes function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_INITIALIZE_NODES,
                    (EXTERNAL_Function) IPNE_InitializeNodes);

            // Register Receive function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_RECEIVE,
                    (EXTERNAL_Function) IPNE_Receive);

            // Register Forward function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_FORWARD,
                    (EXTERNAL_Function) IPNE_Forward);

            // Register Finalize function
            EXTERNAL_RegisterFunction(
                    iface,
                    EXTERNAL_FINALIZE,
                    (EXTERNAL_Function) IPNE_Finalize);

            if (list->partition->partitionId == 0)
            {
                // Set the time management to real-time, with 0 lookahead
                EXTERNAL_SetTimeManagementRealTime(
                        iface,
                        0);

                // Set the minimum delay between calls
                EXTERNAL_SetReceiveDelay(
                        iface,
                        1 * MILLI_SECOND);
            }
            iface->partition->isAutoIpne = FALSE;
        }
#endif


#ifdef UPA_INTERFACE
        /*******************************************************
         *   UPA Interface
         ******************************************************/
        iface = EXTERNAL_RegisterExternalInterface(
                list,
                "UPA",
                EXTERNAL_CPU_HOG,
                EXTERNAL_UPA);

        // Register Initialize function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_INITIALIZE,
                (EXTERNAL_Function) UPA_Initialize);

        // Register InitializeNodes function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_INITIALIZE_NODES,
                (EXTERNAL_Function) UPA_InitializeNodes);

        // Register Receive function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_RECEIVE,
                (EXTERNAL_Function) UPA_Receive);

        // Register Forward function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_FORWARD,
                (EXTERNAL_Function) UPA_Forward);

        // Register Finalize function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_FINALIZE,
                (EXTERNAL_Function) UPA_Finalize);

        if (list->partition->partitionId == 0)
        {
            // Set the time management to real-time, with 0 lookahead
            EXTERNAL_SetTimeManagementRealTime(
                    iface,
                    0);

            // Set the minimum delay between calls
            EXTERNAL_SetReceiveDelay(
                    iface,
                    5 * MILLI_SECOND);
        }
#endif  //UPA_INTERFACE
#ifdef RT_INDICATOR_INTERFACE
        /*******************************************************
         *   RT_INDICATOR Interface
         ******************************************************/

        iface = EXTERNAL_RegisterExternalInterface(
                list,
                "RT_INDICATOR",
                EXTERNAL_CPU_HOG,
                EXTERNAL_RT_INDICATOR);

        // Register Initialize function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_INITIALIZE,
                (EXTERNAL_Function) RT_INDICATOR_Initialize);

        if (list->partition->partitionId == 0)
        {
            // Set the time management to real-time, with 0 lookahead
            EXTERNAL_SetTimeManagementRealTime(
                    iface,
                    0);
        }
#endif
#ifdef PAS_INTERFACE
        /*******************************************************
         *   PAS Interface
         ******************************************************/

        iface = EXTERNAL_RegisterExternalInterface(
                list,
                "PAS",
                EXTERNAL_CPU_HOG,
                EXTERNAL_PAS);

        // Register Initialize function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_INITIALIZE,
                (EXTERNAL_Function) PAS_Initialize);

        // Register InitializeNodes function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_INITIALIZE_NODES,
                (EXTERNAL_Function) PAS_InitializeNodes);

        // Register Receive function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_RECEIVE,
                (EXTERNAL_Function) PAS_Receive);

        // Register Forward function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_FORWARD,
                (EXTERNAL_Function) PAS_Forward);

        // Register Finalize function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_FINALIZE,
                (EXTERNAL_Function) PAS_Finalize);


        if (list->partition->partitionId == 0)
        {
            // Set the time management to real-time, with 0 lookahead
            EXTERNAL_SetTimeManagementRealTime(
                    iface,
                    0);
            // Set the minimum delay between calls
            EXTERNAL_SetReceiveDelay(
                    iface,
                    5 * MILLI_SECOND);
        }

#endif //PAS_INTERFACE
    } //if
#endif // EXATA

#ifdef TUTORIAL_INTERFACE
    if (EXTERNAL_ConfigStringPresent(nodeInput, "INTERFACE-TUTORIAL"))
    {
        // Register InterfaceTutorial interface
        iface = EXTERNAL_RegisterExternalInterface(
                list,
                "InterfaceTutorial",
                EXTERNAL_NONE,
                EXTERNAL_INTERFACETUTORIAL);

        // Register InitializeNodes function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_INITIALIZE_NODES,
                (EXTERNAL_Function) InterfaceTutorialInitializeNodes);

        // Register Receive function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_RECEIVE,
                (EXTERNAL_Function) InterfaceTutorialReceive);

        // Register Forward function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_FORWARD,
                (EXTERNAL_Function) InterfaceTutorialForward);

        // Register Finalize function
        EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_FINALIZE,
                (EXTERNAL_Function) InterfaceTutorialFinalize);

        // Set the time management to real-time, with 0 lookahead
        EXTERNAL_SetTimeManagementRealTime(
            iface,
            0);

        // Set the minimum delay between calls
        EXTERNAL_SetReceiveDelay(
            iface,
            500 * MILLI_SECOND);
    }
#endif

#ifdef HITL_INTERFACE
    if (EXTERNAL_ConfigStringIsYes(nodeInput, "HITL"))
    {
        iface = EXTERNAL_RegisterExternalInterface(
            list,
            "HITL",
            EXTERNAL_NONE,
            EXTERNAL_HITL);

        // Register Initialize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE,
            (EXTERNAL_Function) HITL_Initialize);

        // Register InitializeNodes function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE_NODES,
            (EXTERNAL_Function) HitlInitializeNodes);

        // Register Receive function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_RECEIVE,
            (EXTERNAL_Function) HitlReceive);

        // Register Forward function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FORWARD,
            (EXTERNAL_Function) HitlForward);

        // Register Finalize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FINALIZE,
            (EXTERNAL_Function) HitlFinalize);

        // Set the time management to real-time, with 0 lookahead
        if (list->partition->partitionId == 0)
        {
            EXTERNAL_SetTimeManagementRealTime(
                iface,
                0);
        }

        // Set the minimum delay between calls
        EXTERNAL_SetReceiveDelay(
                iface,
                500 * MILLI_SECOND);
    }
#endif

#ifdef SOCKET_INTERFACE
    if (EXTERNAL_ConfigStringIsYes(nodeInput, "SOCKET-INTERFACE") ||
        EXTERNAL_ConfigStringIsYes(nodeInput, "CES-SOCKET"))
    {
        // Register Socket interace.  May become a CPU hog if specified in config file.
        iface = EXTERNAL_RegisterExternalInterface(
            list,
            "SOCKET-INTERFACE",
            EXTERNAL_NONE,
            EXTERNAL_SOCKET);

        // Register Time function.  This is only used for time managed mode.
        // This time function will be overridden when using real-time mode.
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_TIME,
            (EXTERNAL_Function) SocketInterface_FederationTime);

        // Register Initialize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE,
            (EXTERNAL_Function) SocketInterface_Initialize);

        // Register InitializeNodes function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE_NODES,
            (EXTERNAL_Function) SocketInterface_InitializeNodes);

        if (iface->partition->partitionId == 0)
        {
            // Register SimulationHorizon function
            EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_SIMULATION_HORIZON,
                (EXTERNAL_Function) SocketInterface_SimulationHorizon);

            // Register Receive function
            EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_RECEIVE,
                (EXTERNAL_Function) SocketInterface_Receive);
        }

        // Register Forward function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FORWARD,
            (EXTERNAL_Function) SocketInterface_Forward);

        // Register Finalize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FINALIZE,
            (EXTERNAL_Function) SocketInterface_Finalize);
    }
#endif
#ifdef ADDON_MA
    if (EXTERNAL_ConfigStringIsYes(nodeInput, "MA-INTERFACE"))
    {
        // Register TRL06 interface
        iface = EXTERNAL_RegisterExternalInterface(
            list,
            "MA",
            EXTERNAL_NONE,
            EXTERNAL_MA);

        // Register InitializeNodes function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE_NODES,
            (EXTERNAL_Function) MAInitializeNodes);

        // Register Receive function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_RECEIVE,
            (EXTERNAL_Function) MAReceive);

        // Register Forward function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FORWARD,
            (EXTERNAL_Function) MAForward);

        // Register Finalize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FINALIZE,
            (EXTERNAL_Function) MAFinalize);

        // Set the minimum delay between calls
        EXTERNAL_SetReceiveDelay(
            iface,
            10 * MILLI_SECOND);
    }
#endif
#ifdef ADDON_JSR
    if (EXTERNAL_ConfigStringIsYes(nodeInput, "JSR-INTERFACE"))
    {
        JsrRegister(list);
    }
#endif

#ifdef TESTTHREAD_INTERFACE
    if (EXTERNAL_ConfigStringIsYes(nodeInput, "TEST-THREAD"))
    {
        // Register TestThread interface
        iface = EXTERNAL_RegisterExternalInterface(
            list,
            "TestThread",
            EXTERNAL_THREADED,
            EXTERNAL_TESTTHREAD);

        // Register InitializeNodes function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE_NODES,
            (EXTERNAL_Function) TestThreadInitializeNodes);

        // Register Receive function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_RECEIVE,
            (EXTERNAL_Function) TestThreadReceive);

        // Register Forward function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FORWARD,
            (EXTERNAL_Function) TestThreadForward);

        // Register Finalize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FINALIZE,
            (EXTERNAL_Function) TestThreadFinalize);

        // Set the time management to real-time, with 0 lookahead
        EXTERNAL_SetTimeManagementRealTime(
            iface,
            0);
    }
#endif


#ifdef HLA_INTERFACE
    HlaReadHlaParameter(nodeInput);
    if (HlaIsActive())
    {
        // Register HLA
        iface = EXTERNAL_RegisterExternalInterface(
            list,
            "HLA",
            EXTERNAL_NONE,
            EXTERNAL_HLA);

        // Register Initialize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE,
            (EXTERNAL_Function) HlaInit);

        // Register InitializeNodes function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE_NODES,
            (EXTERNAL_Function) HlaInitNodes);

        if (iface->partition->partitionId == 0)
        {
            // Register Receive function
            EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_RECEIVE,
                (EXTERNAL_Function) HlaReceive);
        }

        // Register Finalize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FINALIZE,
            (EXTERNAL_Function) HlaFinalize);

        // Set the time management to real-time, with 0 lookahead
        EXTERNAL_SetTimeManagementRealTime(
            iface,
            0);
    }
#elif !defined VRLINK_INTERFACE
    exitWithErrorOnHlaScenario(nodeInput);
#endif /* HLA_INTERFACE */

#ifdef DIS_INTERFACE
    DisReadDisParameter(nodeInput);
    if (DisIsActive())
    {
        // Register HLA
        iface = EXTERNAL_RegisterExternalInterface(
            list,
            "DIS",
            EXTERNAL_NONE);

        // Register Initialize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE,
            (EXTERNAL_Function) DisInit);

        // Register InitializeNodes function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE_NODES,
            (EXTERNAL_Function) DisInitNodes);

        // Register Receive function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_RECEIVE,
            (EXTERNAL_Function) DisReceive);

        // Register Finalize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FINALIZE,
            (EXTERNAL_Function) DisFinalize);

        // Set the time management to real-time, with 0 lookahead
        EXTERNAL_SetTimeManagementRealTime(
            iface,
            0);
    }//if//
#elif !defined VRLINK_INTERFACE
    exitWithErrorOnDisScenario(nodeInput);
#endif /* DIS_INTERFACE */

#ifdef VRLINK_INTERFACE
    if (VRLinkIsActive(nodeInput))
    {
        // Register VRLink
        iface = EXTERNAL_RegisterExternalInterface(
            list,
            "VRLINK-PROTOCOL",
            EXTERNAL_NONE,
            EXTERNAL_VRLINK);

        // Register Initialize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE,
            (EXTERNAL_Function) VRLinkInit);

        // Register InitializeNodes function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE_NODES,
            (EXTERNAL_Function) VRLinkInitNodes);

        if (iface->partition->partitionId == 0)
        {
            // Register Receive function
            EXTERNAL_RegisterFunction(
                iface,
                EXTERNAL_RECEIVE,
                (EXTERNAL_Function) VRLinkReceive);
        }

        // Register Finalize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FINALIZE,
            (EXTERNAL_Function) VRLinkFinalize);

        // Set the time management to real-time, with 0 lookahead
        EXTERNAL_SetTimeManagementRealTime(
            iface,
            0);
    }
#else
    exitWithErrorOnVRLinkScenario(nodeInput);
#endif /* VRLINK_INTERFACE */

#ifdef QSH_INTERFACE
    if (EXTERNAL_ConfigStringPresent(nodeInput, "QSH"))
    {
        // Register IPNetworkEmulator
        iface = EXTERNAL_RegisterExternalInterface(
            list,
            "QSH",
            EXTERNAL_NONE,
            EXTERNAL_QSH);

        // Register Initialize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_INITIALIZE,
            (EXTERNAL_Function) QshInitialize);

        // Register Receive function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_RECEIVE,
            (EXTERNAL_Function) QshReceive);

        // Register Forward function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FORWARD,
            (EXTERNAL_Function) QshForward);

        // Register Finalize function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_FINALIZE,
            (EXTERNAL_Function) QshFinalize);

        // Set the time management to real-time, with 0 lookahead
        EXTERNAL_SetTimeManagementRealTime(
            iface,
            0);

        // Set the minimum delay between calls
        EXTERNAL_SetReceiveDelay(
            iface,
            1 * MILLI_SECOND);
    }
#endif

#ifdef JNE_BLACKSIDE_INTEROP_INTERFACE
    if (EXTERNAL_ConfigStringIsYes(nodeInput, "MI-MULTICAST-ADAPTER"))
    {
        MiMulticastAdapterRegister(list);
    }
#endif

#ifdef OCULA_INTERFACE
    if (OculaInterfaceEnabled())
    {
        // Register Scenario-Player interface
        iface = EXTERNAL_RegisterExternalInterface(
            list,
            "SPInterface",
            EXTERNAL_NONE,
            EXTERNAL_SP);

        // Register receive function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_RECEIVE,
            (EXTERNAL_Function) SPReceive);
        
        // Register simulation horizon function
        EXTERNAL_RegisterFunction(
            iface,
            EXTERNAL_SIMULATION_HORIZON,
            (EXTERNAL_Function) SPSimulationHorizon);
    }
#endif

    // end user-modifiable code
}

void EXTERNAL_InitializeInterface(EXTERNAL_Interface *iface)
{
    assert(iface != NULL);

    // Set entire structure to zero
    memset(iface, 0, sizeof(EXTERNAL_Interface));
}

void EXTERNAL_FinalizeExternalInterface(EXTERNAL_Interface *iface)
{
    int i;
    EXTERNAL_Mapping *list;
    EXTERNAL_Mapping *temp;

    assert(iface != NULL);

    // Call the interface's finalize function if one was provided
    if (iface->finalizeFunction != NULL )
    {
        (iface->finalizeFunction)(iface);
    }

    // Free the mappings created by the interface
    for (i = 0; i < EXTERNAL_MAPPING_TABLE_SIZE; i++)
    {
        list = iface->table[i];

        while (list != NULL)
        {
            temp = list;
            list = list->next;

            MEM_free(temp->key);
            MEM_free(temp->val);
            MEM_free(temp);
        }
    }
}

void EXTERNAL_RemoteForwardData(
    EXTERNAL_Interface *iface,
    Node* node,
    void *forwardData,
    int forwardSize,
    int partitionId,
    clocktype delay)
{
#ifdef PARALLEL //Parallel
    Message *       reForwardMessage;
    unsigned int    reForwardMessageSize;

    if (iface->forwardFunction == NULL)
    {
        ERROR_ReportWarning("Interface does not have a forward function");
        return;
    }
    reForwardMessageSize = forwardSize + sizeof (NodeAddress) + sizeof (ExternalInterfaceType);
    reForwardMessage = MESSAGE_Alloc (iface->partition->firstNode,
        PARTITION_COMMUNICATION,     // special layer
        iface->partition->externalForwardCommunicator,  // protccol - our commtor id
        0);     // only 1 msg type, so event type is just 0.
    MESSAGE_SetLooseScheduling (reForwardMessage);
    MESSAGE_InfoAlloc (iface->partition->firstNode, reForwardMessage,
        reForwardMessageSize);
    // Message's format is:
    //  - the data itself
    //  - the node id that forwarded the message
    //  - external interface type -- this is
    memcpy(MESSAGE_ReturnInfo(reForwardMessage), forwardData, forwardSize);
    memcpy(MESSAGE_ReturnInfo(reForwardMessage) + forwardSize,
        &node->nodeId, sizeof (NodeAddress));
    memcpy(MESSAGE_ReturnInfo(reForwardMessage) + forwardSize + sizeof(NodeAddress),
        &iface->type, sizeof (ExternalInterfaceType));
    PARTITION_COMMUNICATION_SendToPartition (iface->partition,
        partitionId, reForwardMessage, delay);
#else
    ERROR_ReportWarning("Parallel function EXTERNAL_RemoteForwardData called in "
                        "non-parallel execution context.");
#endif //Parallel
}

// Receive + handle, from a remote partition, an assignment for
// a new simulation duration value.
static void EXTERNAL_handleSetSimulationDuration (PartitionData * partitionData,
    Message * message)
{
#ifdef PARALLEL //Parallel
    EXTERNAL_SimulationDurationInfo * setDurationInfo;

    setDurationInfo = (EXTERNAL_SimulationDurationInfo *) MESSAGE_ReturnInfo (
        message);
    partitionData->maxSimClock = setDurationInfo->maxSimClock;
    MESSAGE_Free(partitionData->firstNode, message);
#else
    char warningString[MAX_STRING_LENGTH];
    sprintf(
        warningString,
        "Parallel function EXTERNAL_handleSetSimulationDuration called in "
        "non-parallel execution context.");
    ERROR_ReportWarning(warningString);
#endif //Parallel
}

// Receive + handle, from a remote partition, a data+size for
// forwarding to an external interface.
static void EXTERNAL_handleReForwardData (PartitionData * partitionData,
    Message * message)
{
#ifdef PARALLEL //Parallel
    int                     dataSize;
    void *                  dataPtr;
    ExternalInterfaceType   interfaceType;
    EXTERNAL_Interface *    iface;
    NodeAddress             nodeId;

    // Message's format is:
    //  - the data itself
    //  - external interface type -- this is
    char *  chunk = MESSAGE_ReturnInfo (message);
    dataPtr = chunk;
    dataSize = MESSAGE_ReturnInfoSize (message) - sizeof (ExternalInterfaceType) - sizeof (NodeAddress);
    memcpy (&nodeId, chunk + dataSize, sizeof (NodeAddress));
    memcpy (&interfaceType, chunk + dataSize + sizeof (NodeAddress), sizeof (ExternalInterfaceType));
    assert (interfaceType < EXTERNAL_TYPE_COUNT);
    iface = partitionData->interfaceTable [interfaceType];
    //node which forwards data
    partitionData->EXTERNAL_lastIdToInvokeForward = nodeId;
    EXTERNAL_ForwardData(iface, partitionData->firstNode, dataPtr, dataSize,EXTERNAL_ForwardDataAssignNodeID_Off);

    MESSAGE_Free(partitionData->firstNode, message);
#else
    char warningString[MAX_STRING_LENGTH];
    sprintf(
        warningString,
        "Parallel function EXTERNAL_handleReForwardData called in non-parallel"
        " execution context.");
    ERROR_ReportWarning(warningString);
#endif //Parallel
}

void EXTERNAL_InitializeInterfaceList(
    EXTERNAL_InterfaceList *list,
    PartitionData *partition)
{
    BOOL wasFound;
    int debugLevel;

    // We are invoked by PARITION_CreatePartition ()
    assert(list != NULL);

    // Set contents of list to zero
    memset(list, 0, sizeof(EXTERNAL_InterfaceList));

    list->numInterfaces = 0;

    list->cpuHog = FALSE;

    list->warmupTime = -1;
    list->warmupDrop = FALSE;

    list->warmupPhase = EXTERNAL_NoWarmup;
    list->numWarmupInterfaces = 0;

    list->isStopping = FALSE;
    list->stopTime = 0;

    // Set the list's partition
    list->partition = partition;

    // Configure debug levels
    list->printPacketLog = FALSE;
    list->printStatistics = FALSE;
    list->debug = FALSE;

    /* Read the debug print level */
    IO_ReadInt(
        ANY_NODEID,
        ANY_ADDRESS,
        partition->nodeInput,
        "EXTERNAL-DEBUG-LEVEL",
        &wasFound,
        &debugLevel);
    if (wasFound)
    {
        switch (debugLevel)
        {
            case 3:
                list->printPacketLog = TRUE;
            case 2:
                list->printStatistics = TRUE;
            case 1:
                list->debug = TRUE;
            default:
                break;
        }
    }


#ifdef PARALLEL //Parallel
    // The EXTERNAL_ package can forward between partitions. To
    // accomplish this EXTERNAL uses partition communication.
    partition->externalForwardCommunicator =
        PARTITION_COMMUNICATION_RegisterCommunicator(
            partition, "External Interface ReForward",
            EXTERNAL_handleReForwardData);

    // The EXTERNAL_ package provides an API to assign a new simulation
    // duration. To disseminate the new duration value, EXTERNAL uses
    // partition communication.
    partition->externalSimulationDurationCommunicator =
        PARTITION_COMMUNICATION_RegisterCommunicator(
            partition, "External Interface SetSimulationDuration",
            EXTERNAL_handleSetSimulationDuration);
#endif //Parallel
}

void EXTERNAL_FinalizeInterfaceList(EXTERNAL_InterfaceList *list)
{
    EXTERNAL_Interface *t;

    // While there is something in the list
    while (list->interfaces != NULL)
    {
        // Remove the first interface from the list
        t = list->interfaces;
        list->interfaces = t->next;

        // Finalize it
        EXTERNAL_FinalizeExternalInterface(t);
        MEM_free(t);
    }
}

void EXTERNAL_CallInitializeFunctions(
    EXTERNAL_InterfaceList *list,
    NodeInput *nodeInput)
{
    EXTERNAL_Interface *iface;
    EXTERNAL_Interface *guiIface = NULL;

    // Loop through all interfaces
    iface = list->interfaces;
    while (iface != NULL)
    {
        // Call the Initialize function if it exists
        if (iface->initializeFunction != NULL)
        {
            (iface->initializeFunction)(iface, nodeInput);
        }
        if (iface->type == EXTERNAL_QUALNET_GUI)
        {
            guiIface = iface;
        }
        iface = iface->next;
    }
    // The GUI interface is always initialized so
    // some globals can be setup correctly, however
    // when there is no actual animation or IDE
    // we want to deactive ourselves so that the
    // simulation can execute with better performance.
    if (guiIface)
    {
        if (PARTITION_ClientStateFind(guiIface->partition, "GuiActive") ==
            NULL)
        {
            // The gui isn't active, so deactivate the external interface
            // so that the simulation can run faster.
            EXTERNAL_DeactivateInterface (guiIface);
        }
    }
}

void EXTERNAL_CallInitializeNodesFunctions(
    EXTERNAL_InterfaceList *list,
    NodeInput *nodeInput)
{
    EXTERNAL_Interface *iface;

    // Loop through all interfaces
    iface = list->interfaces;
    while (iface != NULL)
    {
        // Call the Initialize Nodes function if it exists
        if (iface->initializeNodesFunction != NULL)
        {
            (iface->initializeNodesFunction)(iface, nodeInput);
        }
        iface = iface->next;
    }
}

void EXTERNAL_StartThreads(EXTERNAL_InterfaceList *list)
{
    EXTERNAL_Interface *iface;
    int status;

    // Loop through all interfaces
    iface = list->interfaces;
    while (iface != NULL)
    {
        if (iface->threaded)
        {
            printf("Starting threads for %s\n", iface->name);
            if (iface->params & EXTERNAL_THREADED_SINGLE_THREAD)
            {
                // Launch only one thread
                printf("Starting receive forward thread for %s\n", iface->name);
                status = pthread_create(
                    &iface->receiveForwardThread,
                    NULL,
                    EXTERNAL_ReceiveForwardThread,
                    iface);

                if (status != 0)
                {
                    ERROR_ReportError("Could not create receive thread");
                }
            }
            else
            {
                // Launch Receive and Forward threads
                if (iface->receiveFunction != NULL)
                {
                    printf("Starting receive thread for %s\n", iface->name);
                    status = pthread_create(
                        &iface->receiveThread,
                        NULL,
                        EXTERNAL_ReceiveThread,
                        iface);

                    if (status != 0)
                    {
                        ERROR_ReportError("Could not create receive thread");
                    }
                }
                if (iface->forwardFunction != NULL && 0)
                {
                    printf("Starting forward thread for %s\n", iface->name);
                    status = pthread_create(
                        &iface->forwardThread,
                        NULL,
                        EXTERNAL_ForwardThread,
                        iface);
                    if (status != 0)
                    {
                        ERROR_ReportError("Could not create forward thread");
                    }
                }
            }
        }
        iface = iface->next;
    }
}

clocktype EXTERNAL_CalculateMinSimulationHorizon(
    EXTERNAL_InterfaceList *list,
    clocktype now)
{
    EXTERNAL_Interface *iface;
    clocktype min = EXTERNAL_MAX_TIME;

    // Loop through all interfaces
    iface = list->interfaces;
    while (iface != NULL)
    {
        if (iface->simulationHorizonFunction != NULL)
        {
            // If the horizon has been passed then call the interface
            // horizon function
            if (now >= iface->horizon)
            {
                (iface->simulationHorizonFunction)(iface);
            }

            // If this interface has the minimum horizon, update it
            if (iface->horizon < min)
            {
                min = iface->horizon;
            }
        }
        iface = iface->next;
    }

    // If we are stopping and the minimum horizon is the stop time, increase
    // the horizon by 1.
    if (list->isStopping && list->stopTime == min)
    {
        min++;
    }

    return min;
}

void EXTERNAL_CallReceiveFunctions(EXTERNAL_InterfaceList *list)
{
    EXTERNAL_Interface *iface = NULL;
    clocktype now;

    // If doing warmup, check if still waiting to begin warmup
    if (list->warmupTime >= 0 && (
        list->warmupPhase == EXTERNAL_WaitingToBeginWarmup ||
        list->warmupPhase == EXTERNAL_PrintedWaitingToBeginWarmup))
    {
        if (list->numWarmupInterfaces == 0)
        {
            list->warmupPhase = EXTERNAL_InWarmup;

            PARTITION_ShowProgress(
                list->partition,
                const_cast<char *>("Beginning Warmup"),
                false,
                true);

            iface = list->interfaces;
            while (iface != NULL)
            {

                if (iface->type != EXTERNAL_QUALNET_GUI)
                {
                    // Set horizon to warmupTime + 1 (lets us get to warmupTime)
                    iface->horizon = list->warmupTime + 1;
                }
                iface = iface->next;
            }

            // Send a heartbeat message at the warmup time so we can get to
            // the warmup time
            Message* heartbeat;
            heartbeat = MESSAGE_Alloc(
                list->partition->firstNode,
                EXTERNAL_LAYER,
                EXTERNAL_NONE,
                MSG_EXTERNAL_Heartbeat);
            MESSAGE_Send(
                list->partition->firstNode,
                heartbeat,
                list->warmupTime);
        }
        else if (list->warmupPhase == EXTERNAL_WaitingToBeginWarmup)
        {
            // Have p0 print waiting to start warmup once if using exata
            list->warmupPhase = EXTERNAL_PrintedWaitingToBeginWarmup;

            if (list->partition->partitionId == 0 && list->debug)
            {
                printf("Attempting to begin warmup at sim time = %.9f\n",
                    (double) EXTERNAL_QuerySimulationTime(list->interfaces) / SECOND);
            }
        }
    }

    // Loop through all interfaces
    iface = list->interfaces;
    while (iface != NULL)
    {
        now = iface->partition->wallClock->getTrueRealTime();

        // Call the Receive function if it exists.  Do not call sooner than
        // receive delay unless at the horizon.  Check for now wrapping
        // around.
        if (iface->receiveFunction != NULL
            && (now >= iface->lastReceiveCall + iface->receiveCallDelay
                || now < iface->lastReceiveCall - HOUR))
        {
            // If in warmup time don't call receive function if set
            if (!(iface->warmupNoReceive && !EXTERNAL_IsInWarmup(iface)))
            {
                (iface->receiveFunction)(iface);
                iface->lastReceiveCall = now;
            }

#ifdef DEBUG
            printf("called %s at %f seconds\n",
                   iface->name, (float) now / SECOND);
#endif
        }
        iface = iface->next;
    }
}

void EXTERNAL_CallFinalizeFunctions(EXTERNAL_InterfaceList *list)
{
    EXTERNAL_Interface *iface;

    // Loop through all interfaces
    iface = list->interfaces;
    while (iface != NULL)
    {
        // Call the Finalize function if it exists
        if (iface->finalizeFunction != NULL)
        {
            (iface->finalizeFunction)(iface);
        }

        iface = iface->next;
    }
}

// old EXTERNAL HLA-style function
void EXTERNAL_InitializeExternalInterfaces(PartitionData* partitionData) {
    // We start with assuming external interfaces will be active.
    // If no external interface registers, then we know externalInterfaces
    // aren't active.
    partitionData->interfaceList.isActive = TRUE;

    EXTERNAL_UserFunctionRegistration(&partitionData->interfaceList,
                                      partitionData->nodeInput);

    EXTERNAL_CallInitializeFunctions(
        &partitionData->interfaceList,
        partitionData->nodeInput);


    EXTERNAL_InitializeWarmupParams(
        &partitionData->interfaceList,
        partitionData->nodeInput);

    EXTERNAL_SetActive(&partitionData->interfaceList);

    // Now initialize external interfaces that don't use the callback
    // framework.  If any of these are in use, they must re-enable the
    // partition's external interface handling.
}

// old EXTERNAL HLA-style function
void EXTERNAL_PostInitialize(PartitionData* partitionData)
{
    EXTERNAL_CallInitializeNodesFunctions(
        &partitionData->interfaceList,
        partitionData->nodeInput);

    EXTERNAL_RealtimeIndicator(
        &partitionData->interfaceList,
        partitionData->nodeInput);


    EXTERNAL_StartThreads(&partitionData->interfaceList);
}

void EXTERNAL_InitializeWarmupParams(
    EXTERNAL_InterfaceList* list,
    NodeInput* nodeInput)
{
    if (list->numInterfaces > 0)
    {
        //char startTimeInput[MAX_STRING_LENGTH];
        clocktype startTime;
        BOOL wasFound;
        BOOL wasDropFound;
        BOOL readVal;

        IO_ReadTime(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "EXTERNAL-WARM-UP-TIME",
                &wasFound,
                &startTime);
        if (wasFound)
        {
            EXTERNAL_Interface* iface = list->interfaces;

            while (iface!=NULL)
            {
                EXTERNAL_SetWarmupTime(
                    iface,
                    startTime);
                /*
                Only socket interface cannot enter warm-up since we wait for
                platforms to load. So except for that we make sure all other external
                interfaces begin warm-up.
                */
                if (iface != EXTERNAL_GetInterfaceByName(list,"SOCKET-INTERFACE"))
                {
                    EXTERNAL_BeginWarmup(iface);
                }
                iface=iface->next;
            }
        }
        // Set all interfaces to have warmup time

        //check if the user has specified whether the external packets must
        //be dropped or sent to destination with zero delay

        IO_ReadBool(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "EXTERNAL-WARM-UP-DROP",
            &wasDropFound,
            &readVal);

        if (!wasFound && wasDropFound)
        {
            ERROR_ReportError ("User must specify a warm up time value before "
                "specifying a behavior during warm-up\n");
        }
        else if (wasDropFound)
        {
            list->warmupDrop = readVal;
        }
    }
}

// /**
// API       :: EXTERNAL_RealtimeIndicator
// PURPOSE   :: for realtime indicator initialization
// PARAMETERS ::
// + iface  : EXTERNAL_Interface* : The external interface
// + nodeInput : NodeInput* : The configuration file.
// RETURN   :: void :
// **/
void EXTERNAL_RealtimeIndicator(
    EXTERNAL_InterfaceList* list,
    NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL wasFound;
    clocktype interval;

    // EXTERNAL_RT_INDICATOR_INTERVAL: 0.1 sec
    interval = (clocktype) EXTERNAL_RT_INDICATOR_INTERVAL;
    IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "EXTERNAL-REALTIME-INDICATOR",
            &wasFound,
            &buf[0]);
    if ((wasFound) && (!strcmp(buf, "YES")) )
    {
        list->partition->isRtIndicator = TRUE;
        //create a timer message
        Message* msg = MESSAGE_Alloc(
            list->partition->firstNode,
            EXTERNAL_LAYER,
            EXTERNAL_NONE,
            MSG_EXTERNAL_RealtimeIndicator);

        //send a message
        MESSAGE_Send(
            list->partition->firstNode,
            msg,
            interval);
    }
    else
    {
        list->partition->isRtIndicator = FALSE;
    }
}

void BusyWait(int usec)
{
    clocktype t1;
    clocktype t2;

    // busy wait
    t1 = WallClock::getTrueRealTime();
    do
    {
        t2 = WallClock::getTrueRealTime ();
    } while ((t2 - t1) / MICRO_SECOND < usec);

}

// old EXTERNAL HLA-style function
void EXTERNAL_GetExternalMessages(PartitionData* partitionData,
                                  clocktype nextInternalEventTime)
{
    clocktype horizon;
    BOOL calledReceive = FALSE;

    if (partitionData->interfaceList.interfaces == NULL)
    {
        return;
    }

    // We slow down the simulation here, for now.  Ideally it should be done
    // in the event loop.
    horizon = EXTERNAL_CalculateMinSimulationHorizon(
                  &partitionData->interfaceList,
                  nextInternalEventTime);
    while (nextInternalEventTime > horizon)
    {
        /*printf("horizon = %8.2fs next = %8.2fs diff = %10dns\n",
               (float) horizon / SECOND,
           (float) nextInternalEventTime / SECOND,
           nextInternalEventTime - horizon);*/

        // Sleep
        if (partitionData->interfaceList.cpuHog)
        {
            BusyWait(1);
        }
        else
        {
            EXTERNAL_Sleep(1 * MILLI_SECOND);
        }

        // Receive external messages
        EXTERNAL_CallReceiveFunctions(&partitionData->interfaceList);
        calledReceive = TRUE;

        // Calculate new horizon and next internal event time
        horizon = EXTERNAL_CalculateMinSimulationHorizon(
                      &partitionData->interfaceList,
                      nextInternalEventTime);
        nextInternalEventTime = GetNextInternalEventTime(partitionData);
    }

    // Receive messages from external entities if not called earlier
    if (!calledReceive)
    {
        EXTERNAL_CallReceiveFunctions(&partitionData->interfaceList);
    }

}

// old EXTERNAL HLA-style function
void EXTERNAL_Finalize(PartitionData* partitionData)
{
    EXTERNAL_CallFinalizeFunctions(&partitionData->interfaceList);
}

// /**
// API         :: EXTERNAL_DeactivateInterface
// PURPOSE     :: Remove the indicated interface for the list of currently
//                activateed interfaces.
// PARAMETERS  ::
// + ifaceToDeactivate : EXTERNAL_Interface* : Pointer to the interface
// RETURN      :: void
// **/
void EXTERNAL_DeactivateInterface (EXTERNAL_Interface * ifaceToDeactivate)
{
    PartitionData * partitionData;
    partitionData = ifaceToDeactivate->partition;
    EXTERNAL_InterfaceList * ifaceList;
    EXTERNAL_Interface * curIface;
    EXTERNAL_Interface * prevIface;
    bool found;

    ifaceList = &partitionData->interfaceList;
    //EXTERNAL_UserFunctionRegistration(&partitionData->interfaceList,
    // search for match, remove the found match
    // Loop through the list of interfaces
    curIface = ifaceList->interfaces;
    prevIface = NULL;
    found = false;
    while (curIface != NULL)
    {
        // This is the interface to deactivate.
        if (curIface == ifaceToDeactivate)
        {
            found = true;
            if (ifaceList->interfaces == curIface)
            {
                ifaceList->interfaces = curIface->next;
            }
            else
            {
                prevIface->next = curIface->next;
            }
            break;
        }
        prevIface = curIface;
        curIface = curIface->next;
    }
    ERROR_Assert (found,
        "Internal error, failed to find external interface for deactive.");
    ifaceList = &partitionData->interfaceList;
    ifaceList->numInterfaces -= 1;

    // This will set isActive to FALSE when no interfaces are left.
    EXTERNAL_SetActive (ifaceList);
}

// /**
// API         :: EXTERNAL_SetActive
// PURPOSE     :: Sets isActive parameter based on interface registration
// PARAMETERS  ::
// + partitionData : partitionData* : pointer to data for this partition
// RETURN      :: void
// **/
void EXTERNAL_SetActive(EXTERNAL_InterfaceList *list)
{
    if ((list->numInterfaces == 0) ||
        (list->interfaces == NULL)) {
        // There were no registered interfaces, so deactivate.
        list->isActive = FALSE;
    }
}

// /**
// API         :: EXTERNAL_ProcessEvent
// PURPOSE     :: Process events meant for external code.
// PARAMETERS  ::
// + node : Node* : Pointer to node data structure.
// + msg : Message* : Message to be processed.
// RETURN      :: void
// **/
void EXTERNAL_ProcessEvent(Node* node, Message* msg)
{
    switch (msg->protocolType)
    {
        case EXTERNAL_NONE:
        {
            switch (msg->eventType)
            {
                case MSG_EXTERNAL_Heartbeat:
                {
                    EXTERNAL_InterfaceList* list =
                        &node->partitionData->interfaceList;

                    // Check if out of warmup time
                    if (list->warmupPhase == EXTERNAL_InWarmup
                        && node->getNodeTime() == list->warmupTime)
                    {
                        node->partitionData->isRealTime = TRUE;
                        list->warmupPhase = EXTERNAL_OutOfWarmup;

                        PARTITION_ShowProgress(
                            node->partitionData,
                            const_cast<char *>("Warmup Completed"),
                            true, true);
                    }
                    break;
                }
#ifdef EXATA
                case MSG_EXTERNAL_RealtimeIndicator:
                {
                    string rtStatusStr;

                    //MESSAGE_Free(node, msg);

                    // compare sim time and wallclock time
                    double realTime =
                        node->partitionData->wallClock->getRealTimeAsDouble();

                    double simTime = (double)
                        (node->partitionData->getGlobalTime() +
                        node->partitionData->nodeInput->startSimClock) /SECOND;

                    if ((simTime - realTime) > EXTERNAL_RT_INDICATOR_THRESHOLD ) // warning
                    {
                        rtStatusStr.append("RED");
                    }
                    else
                    {
                        rtStatusStr.append("GREEN");
                    }

                    //send the message to GUI
                    if (node->guiOption)
                        GUI_RealtimeIndicator(rtStatusStr.c_str());
                    else
                    {
                        if (!rtStatusStr.compare("RED"))
                            printf("%s\n", rtStatusStr.c_str());
                    }

                    if (node->partitionData->isRtIndicator)
                    {

                        //send a message
                        clocktype interval = EXTERNAL_RT_INDICATOR_INTERVAL;

                        MESSAGE_Send(
                            node,
                            msg,
                            interval);
                    }

                    break;
                }
                case MSG_EXTERNAL_DelayFuncTrueEmul:
                {
                    NodeAddress srcNodeId,dstNodeId;
                    Node* dstNode;
                    IpHeaderType * ipHeader = (IpHeaderType *)
                        MESSAGE_ReturnPacket(msg);
                    if (!NetworkIpIsMulticastAddress(node,ipHeader->ip_dst)&&
                        (ipHeader->ip_dst!=ANY_DEST))
                    {
                        dstNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                            node,
                            ipHeader->ip_dst);

                        if (dstNodeId!=INVALID_MAPPING)
                        {
                            assert(dstNodeId != INVALID_MAPPING);
                            dstNode = MAPPING_GetNodePtrFromHash(
                                node->partitionData->nodeIdHash,
                                dstNodeId);
                            if (dstNode == NULL)
                            {
                                dstNode = MAPPING_GetNodePtrFromHash(
                                    node->partitionData->remoteNodeIdHash,
                                    dstNodeId);
                                if (dstNode == NULL)
                                {
                                    return;
                                }
                            }
                            int interfaceIndex =
                                MAPPING_GetInterfaceIdForDestAddress(dstNode,
                                dstNodeId,
                                ipHeader->ip_dst);
                            if (dstNode->partitionId ==
                                dstNode->partitionData->partitionId)
                            {
                                DeliverPacket(dstNode, msg,
                                    interfaceIndex, ipHeader->ip_src);
                            }
                        }
                    }
                    else
                    {
                        Node* tmpNode = node->partitionData->firstNode;
                        srcNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                            node,
                            ipHeader->ip_src);

                        if (srcNodeId!=INVALID_MAPPING)
                        {
                            int interfaceIndex=
                                MAPPING_GetInterfaceIndexFromInterfaceAddress
                                (node,
                                ipHeader->ip_src);

                            while (tmpNode)
                            {
                                if (tmpNode->nodeId!=srcNodeId)
                                {
                                    NetworkIpReceivePacket(tmpNode,
                                        MESSAGE_Duplicate(tmpNode, msg),
                                        ipHeader->ip_src,interfaceIndex);
                                }
                                tmpNode = tmpNode->nextNodeData;
                            }

                            MESSAGE_Free(node, msg);
                        }
                    }
                    break;
                }

                case MSG_EXTERNAL_DelayFunc:
                {
                    EXTERNAL_Interface* iface = NULL;
                    iface = node->partitionData->interfaceTable[EXTERNAL_IPNE];
                    if (iface == NULL)
                    {
                        break;
                    }
                    EXTERNAL_ForwardData(
                        iface,
                        node,
                        MESSAGE_ReturnPacket(msg),
                        msg->packetSize);
                    break;
                }
#endif
#ifdef AGI_INTERFACE
                case MSG_EXTERNAL_AgiUpdatePosition:
                {
                    AgiInterfaceUpdatePosition(node, msg);
                    break;
                }
#endif // AGI_INTERFACE

                case MSG_EXTERNAL_SendPacket:
                {
#ifdef EXATA
                    IpHeaderType* ipHeader = (IpHeaderType*) msg->packet;
                    unsigned int msgVersion =
                           IpHeaderGetVersion(ipHeader->ip_v_hl_tos_len);
                    if (msgVersion == IPVERSION4
                                        && msg->numberOfHeaders > 2)
                    {
                        NodeAddress *headers;
                        NodeAddress srcAddr, ipDestAddr, nextHopAddr;
                        NodeAddress nodeId;
                        Node* srcNode;

                        headers = (NodeAddress *)MESSAGE_ReturnPacket(msg);

                        srcAddr = headers[0];
                        ipDestAddr = headers[1];
                        nextHopAddr = headers[2];

                        nodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                                node,
                                srcAddr);

                        assert(nodeId != INVALID_MAPPING);
                        srcNode = MAPPING_GetNodePtrFromHash(
                                node->partitionData->nodeIdHash,
                                nodeId);
                        assert(srcNode != NULL);

                        //get interface index
                        int interfaceIndex;
                        interfaceIndex = MAPPING_GetInterfaceIndexFromInterfaceAddress(
                                srcNode, srcAddr);

                        //get subnet mask
                        NodeAddress subnetMask;
                        subnetMask = MAPPING_GetSubnetMaskForDestAddress(srcNode,
                                srcNode->nodeId,
                                nextHopAddr);

                        // call NetworkUpdateForwardingTable up update routing table
                        NetworkUpdateForwardingTable(
                                srcNode,
                                ipDestAddr,
                                subnetMask,
                                nextHopAddr,
                                interfaceIndex,
                                1,
                                ROUTING_PROTOCOL_DEFAULT);

                        MESSAGE_RemoveHeader(node, msg, sizeof(NodeAddress) * 3, TRACE_IP);

                    }
#endif
                    EXTERNAL_SendNetworkLayerPacket(node, msg);
                    break;
                }

                case MSG_EXTERNAL_Mobility:
                    ProcessMobilityEvent(node, msg);
                    MESSAGE_Free(node, msg);
                    break;

#ifdef ADDON_NGCNMS
                case MSG_EXTERNAL_HistStatUpdate:
                    NgcUpdateHistPrint(node);
                    MESSAGE_Free(node, msg);
                    break;
#endif
#ifdef ADDON_WIRELESS
                case MSG_EXTERNAL_PhySetTxPower:
                    EXTERNAL_SetPhyTxPowerInfo * setTxInfo;
                    setTxInfo = (EXTERNAL_SetPhyTxPowerInfo *)
                        MESSAGE_ReturnInfo (msg);
                    Node * nodeToChange;
                    nodeToChange = MAPPING_GetNodePtrFromHash(
                        node->partitionData->nodeIdHash, setTxInfo->nodeId);
                    PHY_SetTransmitPower(node, setTxInfo->phyIndex,
                        setTxInfo->txPower);
                    MESSAGE_Free(node, msg);
                    break;
#endif

#ifdef INTERFACE_JNE_JREAP
                case MSG_EXTERNAL_TADIL_JREAP_EgressFrameIndication:
                {
                   EXTERNAL_Interface* ext
                     = EXTERNAL_GetInterfaceByName(
                     &node->partitionData->interfaceList,
                     "IPNE");
                   assert(ext != NULL);

                   JNE::JREAP::eject_frame(ext, msg);
                   MESSAGE_Free(node, msg);
                } break;
#endif // JREAP
                case MSG_EXTERNAL_RecordStats:
                {
                    BOOL * isPktDropped;
                    NetworkDataIp * ip = (NetworkDataIp*) node->networkData.networkVar;
                    isPktDropped = (BOOL*) MESSAGE_ReturnInfo(msg, INFO_TYPE_DidDropPacket);
                    if (*isPktDropped)
                    {
                        ip->stats.ipPktsWarmupDropped++;
                    }
                    else
                    {
                        ip->stats.ipPktsWarmupDelay++;
                    }
                    MESSAGE_Free(node, msg);
                    break;
                }
                default:
                    break;
            }

            break;
        }

#ifdef DXML_INTERFACE
        case EXTERNAL_DXML:
            DXML_ProcessEvent(node, msg);
            break;
#endif /* DXML_INTERFACE */

#ifdef HLA_INTERFACE
        case EXTERNAL_HLA:
            HlaProcessEvent(node, msg);
            break;
#endif /* HLA_INTERFACE */
#ifdef DIS_INTERFACE
        case EXTERNAL_DIS:
            DisProcessEvent(node, msg);
            break;
#endif /* DIS_INTERFACE */
#ifdef VRLINK_INTERFACE
        case EXTERNAL_VRLINK:
            VRLinkProcessEvent(node, msg);
            break;
#endif /* VRLINK_INTERFACE */
#ifdef TESTTHREAD_INTERFACE
        case EXTERNAL_TESTTHREAD:
            TestThreadProcessEvent(node, msg);
            break;
#endif /* TESTTHREAD_INTERFACE */

#ifdef SOCKET_INTERFACE
        case EXTERNAL_SOCKET:
            SocketInterface_ProcessEvent(node, msg);
            break;
#endif

#ifdef EXATA
        case EXTERNAL_IPNE:
        {
            AutoIPNEHandleExternalMessage(node, msg);
            break;
        }
#ifdef INTERFACE_UPA
        case EXTERNAL_UPA:
        {
            UPA_HandleExternalMessage(node, msg);
            break;
        }
#endif
        case EXTERNAL_PAS:
        {
            //printf("Got message %d at %d %d\n", msg->eventType, node->nodeId, node->partitionData->partitionId);
            fflush(stdout);
            PAS_HandleExternalMessage(node, msg);
            break;
        }
        case EXTERNAL_RECORD_REPLAY:
        {
            node->partitionData->rrInterface->ReplayProcessTimerEvent(
                                                    node, msg);
            break;
        }
#endif

#ifdef HITL_INTERFACE
        case EXTERNAL_HITL:
        {
            //printf("[HITL_INTERFACE] Got message %d at %d %d\n", msg->eventType, node->nodeId, node->partitionData->partitionId);
            //fflush(stdout);
            HITL_HandleExternalMessage(node, msg);
            break;
        }
#endif /* HITL_INTERFACE */

        default:
            break;
    }
}

#ifdef MINI_PARSER
static void HandleParserConfiguration(
    int argc,
    char* argv [],
    SimulationProperties* simProps)
{
    if (simProps->noMiniParser)
    {
        return;
    }

    // Now to run the configuration file thru parser. So
    // get the path for the default file
    char defPath[512];
    char miniConfigurationFile[512];
    const char* defTailPath = "/bin/mini.default.config";
    const char* defTailPathWin32 = "\\bin";
    const char* defFileName = "\\mini.default.config";
    const char* defSettingFileName = "\\mini.parser.rc";
    const char* parserDefSettingPath = "/bin/mini.parser.rc";
    char settingPath[512];

    // create the path for the miniparser setting file
    strcpy(settingPath, simProps->qualnetHomePath.c_str());

    // create the path for the CES default config file.
    strcpy(defPath, simProps->qualnetHomePath.c_str());

    const char* found;
    found = strchr(simProps->qualnetHomePath.c_str(), '/');
    if (found == NULL)
    {
        // We have "\" in the home path.
        strcat(defPath, defTailPathWin32);
        strcat(defPath, defFileName);

        strcat(settingPath, defTailPathWin32);
        strcat(settingPath, defSettingFileName);

    }
    else
    {
        // We have "/" in the path
        strcat(defPath, defTailPath);

        strcat(settingPath, parserDefSettingPath);

    }

    printf("CONFIGFILE_PATH=%s\n", defPath);

    // Now use the parser to get the configuration file.
    HandleConfigurationFile((char*) simProps->configFileName.c_str(),
                            defPath,
                            miniConfigurationFile,
                            /*settingPath*/ "mini.parser.rc");

    simProps->configFileName = miniConfigurationFile;
}
#endif

void EXTERNAL_PreBootstrap (int argc, char * argv [], SimulationProperties * simProps,
    PartitionData * partitionData)
{
#ifdef AGI_INTERFACE
    AgiInterfacePreBootstrap(argc, argv, simProps);
#endif //AGI_INTERFACE
    // If "-onlymini" option is passed as a command line argument, exit after parse.
    // Needed for GUI support.
    bool onlymini = false;
    for (int i = 0; i < argc; i++)
    {
        if (strncmp(argv[i],"-onlymini",9) == 0 )
        {
            onlymini = true;
        }
    }

#ifdef SOCKET_INTERFACE
    SocketInterface_BootStrap(argc, argv, simProps, partitionData);
#endif

#ifndef JNE_LIB
    // Test for mini parser if not using JNE.  jne uses mini parser
    // automatically.
#ifdef MINI_PARSER
    // Load the anticipated .config file
    // MERGE - newly added to support mini parser for
    // product.
    HandleParserConfiguration(argc, argv, simProps);
#endif // MINI_PARSER
#endif // JNE_LIB

     if (onlymini)
     {
         printf("CONFIG PARSE COMPLETE\n");
         exit(0);
     }
}

void EXTERNAL_Bootstrap (int argc, char * argv [], NodeInput* nodeInput,
    PartitionData * partitionData)
{
    static int  neverReferenced;

    // This function indicates if an external interface bootstrapped,
    // however other external interfaces (CES) won't indicate
    // if they are active until EXTERNAL_UserFunctionRegistration()
    // is called.
    if (GUI_EXTERNAL_Bootstrap (argc, argv, nodeInput, partitionData->partitionId))
    {
        partitionData->interfaceList.isActive = TRUE;
        // Set state so that later code can do a quick check to
        // determine if the gui is really active (and thus deactivate
        // it if it isn't).
        PARTITION_ClientStateSet(partitionData, "GuiActive",
            &neverReferenced);
    }
}

//---------------------------------------------------------------------------
// ADDON_BOEINGFCS specific
//---------------------------------------------------------------------------

// /**
// API       :: IsBoeingFcs
// PURPOSE   :: Check if the boeingfcs addons is used.  This is needed
//              becausethe pre-compiled code cannot use #ifdef
//              ADDON_BOEINGFCS.  Sections of pre-compiled code that need to
//              test if the FCS addon is being used should use this.
// PARAMETERS::
// none
// RETURN    :: bool : The next internal event
// **/
bool IsBoeingFcs()
{
#ifdef ADDON_BOEINGFCS
    return true;
#else
    return false;
#endif
}
#ifdef MILITARY_RADIOS_LIB
void EXTERNAL_SendRtssNotification(Node* node)
{
#ifdef HLA_INTERFACE
    if (HlaIsActive())
    {
        HlaSendRtssNotificationIfNodeIsHlaEnabled(node);
    }
#endif

#ifdef DIS_INTERFACE
    if (DisIsActive())
    {
        DisSendRtssNotificationIfNodeIsDisEnabled(node);
    }
#endif

#ifdef VRLINK_INTERFACE
    if (VRLinkIsActive(node))
    {
        VRLinkSendRtssNotificationIfNodeIsVRLinkEnabled(node);
    }
#endif
}
#endif
