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
#include <limits.h>
#include <signal.h>

#include "qualnet_mutex.h"

#include "api.h"
#include "clock.h"
#include "external.h"
#include "gui.h"
#include "main.h"
#include "mapping.h"

#ifdef OCULA_INTERFACE
#include "ops_util.h"
#endif

#ifdef PARALLEL //Parallel
#include "parallel.h"
#include "parallel_private.h"
#endif //endParallel

#include "node.h"
#include "partition.h"
#include "external_util.h"
#include "scheduler.h"
#include "WallClock.h"
#include "stats_global.h"
#include "context.h"

#ifdef ADDON_DB
#include "dbapi.h"
#include "db.h"
#include "db_multimedia.h"
#include "db-interlock.h"
#include "db_developer.h"
#endif

#ifdef CELLULAR_LIB
#include "user_profile_parser.h"
#include "user_trafficpattern_parser.h"
#endif // CELLULAR_LIB

#include "phy.h"
#include "phy_connectivity.h"
#include "prop_flat_binning.h"

#ifdef WIRELESS_LIB
#include "battery_model.h"
#endif // WIRELESS_LIB
#ifdef INTERFACE_JNE_C2ADAPTER
#include "c2adapter-api.h"
#endif /* INTERFACE_JNE_C2ADAPTER */

#ifdef INTERFACE_JNE_C3
#include "c3-api.h"
#endif /* INTERFACE_JNE_C3 */

#ifdef CYBER_CORE
#include "network_pki_header.h"
#endif // CYBER_CORE

#ifdef CYBER_LIB
#include "os_resource_manager.h"
#endif


#ifdef PAS_INTERFACE
#include "packet_analyzer.h"
#endif // PAS INTERFACE

#ifdef JNE_LIB
#include "jne.h"
#include "visualization_filter.h"
#endif /* JNE_LIB */

#ifdef EXATA
#include "record_replay.h"
#endif
#define DEBUG 0

#ifdef LTE_LIB
#include "epc_lte.h"
#endif // LTE_LIB


#include "product_info.h"

#ifdef INTERFACE_JNE_JREAP
#include "jreap-api.h"
#endif // JREAP
#ifdef AGI_INTERFACE
#include "agi_interface.h"
#endif

// DHCP
#include "app_dhcp.h"


/* FUNCTION     PARTITION_PrintUsage
 * PURPOSE      Prints the command-line usage of the application.
 *
 * Parameters
 *    commandName: const char*: The command name of the application.
 */
void PARTITION_PrintUsage( const char* commandName ) {
    ProductType productType = Product::GetProduct();
    ProductKernelType kernelType = Product::GetProductKernel();
    printf(
        "Usage:  %s  input-filename  [options]  [experiment-name] \n", 
        commandName
    );
    printf(
        "Options: \n"
        "  -h                        Prints command line usage \n"
        "  -version                  Prints product version \n"
        "  -version_number_only      Prints only the version number of the product \n"
        "  -print_libraries          Prints information about available libraries \n"
        "  -seed N                   Execute with specified random number seed: N \n"
        "  -animate                  Print animation events \n"
        "  -np X                     Run in multiprocessor mode with X number of \n"
        "                            partitions \n"
        "  -interactive host port    Run in interative mode with an external GUI \n"
        "                            communicating over TCP sockets on the \n"
        "                            specified host and port \n"
        "  -with_ocula_gui port      Run with the Scenario Player GUI \n"
        "                            communicating over TCP sockets on the \n"
        "                            specified port \n"
#ifdef PARALLEL //Parallel
        "  -lookahead time           Adjusts the parallel lookahead time in order\n"
        "                            to trade timing precision for greater program\n"
        "                            performance. The value for time is a whole\n"
        "                            number with an optional time unit indicator:\n"
        "                            NS (nanoseconds), US (microseconds), or MS\n"
        "                            (milliseconds). The default is NS.\n"
        "                            Example: -lookahead 1MS\n"
        "  -loose time               Same as lookahead, provided for campatibility\n"
        "                            with previous versions.\n"
        "  -forceeot                 Force the simulator to run in EOT mode even\n"
        "                            though there are ECOT models in a scenario.\n"
        "                            This is most useful when a scenario contains a mix \n"
        "                            of TDMA and CSMA wireless or wired models.  Use \n"
        "                            this parameter with caution as it will not have \n"
        "                            repeatable results.\n"
        "  -friendly                 Friendly in a shared memory context means \n"
        "                            that each thread will not try to use as \n"
        "                            much CPU as possible.  This makes it \n"
        "                            possible to run with more threads than you \n"
        "                            have cores. \n"
        "  -greedy                   Opposite of friendly. Try to grab as much\n"
        "                            CPU to run as fast possible. This is the\n"
        "                            default synchronization mode. \n"
#endif //endParallel
        "  experiment-name           The name of the experiment.  Used to name   \n"
        "                            output result files.\n"
    );
    if (kernelType == KERNEL_EXATA 
    || productType == PRODUCT_EXATA) {
        printf(
            "  -emulation                Run in emulation mode \n"
            "  -simulation               Run in simulation mode \n"
        );
    }
    printf("\n\n");
}


/* FUNCTION     PARTITION_ParseArgv
 * PURPOSE      Reads arguments from command line.  Prints usage if arugment
 *              processing fails for any reason.  Returns FALSE if execution 
 *              should stop due to the processed arguments, returns TRUE
 *              otherwise.
 *
 * Parameters
 *    argc: int: Number of arguments to the command line
 *    argv: char **: Array of arguments to the command line
 *    seedVal: int: Used to set seedVal in the kernel
 *    seedValSet: BOOL: Test if seedVal is set in the kernel
 *    simProps: SimulationProperties: Used to set simProps in kernel
 *    onlyVerifyLicense: BOOL: Used to set onlyVerifyLicense in kernel
 *    onlyPrintLibraries: BOOL: Used to set onlyPrintLibraries in kernel
 *    onlyForGUI: BOOL: Used to set onlyForGUI in kernel
 *    oculaInterface: BOOL: If the ocula interface should be enabled
 *    oculaPort: int: Used to set oculaPort in the kernel
 *    isEmulationMode: BOOL: Used to set isEmulationMode in kernel
 *    numberOfProcessors: int: Used to set numberOfProcessors in kernel
 *    experimentPrefix: char *: Used to set experimentPrefix in kernel
 *    statFileName: char *: Used to set statFileName in kernel
 *    traceFileName: char *: Used to set traceFileName in kernel
 *    product_info: std::string: The product info printed if -version is an argument
 *    g_looseSynchronization: bool: Used to set g_looseSynchronization in kernel
 *    g_forceEot: bool: Force lookahead calculator to use eot mode
 *    g_allowLateDeliveries: bool: Used to set g_allowLateDeliveries in kernel
 *    g_useDynamicPropDelay: bool: Used to set g_useDynamicPropDelay in kernel
 *    g_useRealTimeThread: bool: Used to set g_useRealTimeThread in kernel
 *    g_looseLookahead: clocktype Used to set g_looseLookahead in kernel
 *    g_syncAlgorithm: SynchronizationAlgorithm: Used to set g_syncAlgorithm in kernel
 */
BOOL PARTITION_ParseArgv(int argc, char **argv,
        int &seedVal,
        BOOL &seedValSet,
        SimulationProperties &simProps,
        BOOL &onlyVerifyLicense,
        BOOL &onlyPrintLibraries,
        BOOL &onlyForGUI,
        BOOL &oculaInterface,
        int &oculaPort,
        BOOL &isEmulationMode,
        BOOL &dbRegression,
        int &numberOfPartitions,
        int &numberOfThreadsPerPartition,
        char *experimentPrefix,
        char *statFileName,
        char *traceFileName,
        const std::string &product_info
#ifdef PARALLEL //Parallel
        ,
        bool &g_looseSynchronization,
        bool &g_forceEot,
        bool &g_allowLateDeliveries,
        bool &g_useDynamicPropDelay,
        bool &g_useRealTimeThread,
        clocktype &g_looseLookahead,
        SynchronizationAlgorithm &g_syncAlgorithm
#endif //endParallel
        ) {
    int numNonDash = 0; /* number of non-dash options, used to find out
                           config file name and experiment name */
    int thisArg = 1;
    ProductType productType = Product::GetProduct();
    ProductKernelType kernelType = Product::GetProductKernel();
    BOOL isHelpPrinted = FALSE;
    BOOL isConfigFileSet = FALSE;
    dbRegression = FALSE;
    while (thisArg < argc) {
        /*
         * Check whether a seed value for the random number generator
         * is specified.  If one is, it overrides any .config file setting.
         */
        if (!strcmp(argv[thisArg], "-seed")) {
            if (argc < thisArg + 2) {
                ERROR_ReportError(
                    "Not enough arguments to -seed.\n"
                    "Correct usage:\t -seed N.\n");
            }

            seedVal = atoi(argv[thisArg+1]);
            seedValSet = TRUE;
            thisArg += 2;
        }
        else if (!strcmp(argv[thisArg], "-nomini")) {
            simProps.noMiniParser = TRUE;
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-checkmini")) {
            // If "-checkmini" option is processed as a command line 
            // argument, exit after check, ignore otherwise.
#ifdef ADDON_BOEINGFCS
            printf("CONFIG PARSE ENABLED\n");
            exit(0);
#elif MINI_PARSER
            printf("CONFIG PARSE ENABLED\n");
            exit(0);
#endif // ADDON_BOEINGFCS/MINI_PARSER
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-verifylicense")) {
            onlyVerifyLicense = TRUE;
            isHelpPrinted = TRUE;
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-print_libraries")) {
            onlyPrintLibraries = TRUE;
            onlyForGUI = FALSE;
            isHelpPrinted = TRUE;
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-print_libraries_gui")) {
            onlyPrintLibraries = TRUE;
            onlyForGUI = TRUE;
            isHelpPrinted = TRUE;
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-version")) {
            std::cout << product_info;
            isHelpPrinted = TRUE;
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-version_number_only")) {
            std::cout << Product::GetVersionString() 
                << std::endl << std::endl;
            isHelpPrinted = TRUE;
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-h")) {
            PARTITION_PrintUsage(argv[0]);
            isHelpPrinted = TRUE;
            thisArg++;
        }
#ifdef PARALLEL //Parallel
        else if (!strcmp(argv[thisArg], "-lookahead") ||
                 !strcmp(argv[thisArg], "-loose")) {
            g_looseSynchronization = true;
            g_allowLateDeliveries = true;
            char *opstr = argv[thisArg++];

            // error check here instead of crashing.
            if (thisArg == argc)
            {
                char errstr[MAX_STRING_LENGTH];
                sprintf(errstr,
                    "Not enough arguments to %s\n"
                    "Correct usage:\t %s time\n",
                    opstr, opstr);
                ERROR_ReportError(errstr);
            } 

            g_looseLookahead = TIME_ConvertToClock(argv[thisArg]);
            if (g_looseLookahead <= 0)
            {
                char errstr[MAX_STRING_LENGTH];
                sprintf(errstr,
                      "Invalid time for %s\n"
                      "Time must be greater than zero.\n", opstr);
                ERROR_ReportError(errstr);
            } 

            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-forceeot")) {
            g_looseSynchronization = true;
            g_allowLateDeliveries = true;
            g_forceEot = true;
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-usepropdelay")) {
            g_useDynamicPropDelay = true;
            g_allowLateDeliveries = true;
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-realtimesafe")) {
            g_syncAlgorithm = REAL_TIME;
            g_allowLateDeliveries = false;
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-realtime")) {
            g_syncAlgorithm = REAL_TIME;
            g_allowLateDeliveries = true;
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-realtimeThread")) {
            g_syncAlgorithm = REAL_TIME;
            g_allowLateDeliveries = true;
            g_useRealTimeThread = true;
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-besteffort")) {
            g_syncAlgorithm = BEST_EFFORT;
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-friendly")) {
            PARALLEL_SetGreedy(false);
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-greedy")) {
            PARALLEL_SetGreedy(true);
            //g_syncAlgorithm = GREEDY_WORKER_THREAD;
            thisArg++;
        }
#endif //endParallel
        else if (!strcmp(argv[thisArg], "-socketports") 
        || !strcmp(argv[thisArg], "-alcesports")) {
            thisArg += 2; // CES option, not used here
        }
        else if (!strcmp(argv[thisArg], "-interactive")) {
            thisArg += 3; // GUI option, not used here
        }
        else if (!strcmp(argv[thisArg], "-animate")) {
            thisArg += 1; // GUI option, not used here
        }
        else if (!strcmp(argv[thisArg], "-with-snt-gui")) {
            thisArg += 1; // GUI option, not used here
        }
        else if (!strcmp(argv[thisArg], "-with_ocula_gui")) {
            oculaInterface = true;
            if (argc < thisArg + 2) {
                ERROR_ReportError(
                    "Not enough arguments to -with_ocula_gui.\n"
                    "Correct usage:\t -with_ocula_gui PORT_NUMBER.\n");
            }
            oculaPort = atoi(argv[thisArg+1]);
            thisArg += 2;
        }
        else if (!strcmp(argv[thisArg], "-nomini")) {
            simProps.noMiniParser = TRUE;
            thisArg++;
        }
        else if (!strcmp(argv[thisArg], "-np")) {
            if (argc < thisArg + 2) {
                ERROR_ReportError("Not enough arguments to -np.\n"
                                  "Correct Usage:\t -np X.\n");
            }
            numberOfPartitions = atoi(argv[thisArg+1]);
            ERROR_Assert(numberOfPartitions >= 1,
                         "Argument to -np must be a positive integer.");
            thisArg += 2;
        }
        else if (!strcmp(argv[thisArg], "-npt")) {
            if (argc < thisArg + 2) {
                ERROR_ReportError("Not enough arguments to -npt.\n"
                                  "Correct Usage:\t -npt X.\n");
            }
            numberOfThreadsPerPartition = atoi(argv[thisArg+1]);
            ERROR_Assert(numberOfThreadsPerPartition >= 1,
                         "Argument to -npt must be a positive integer.");
            thisArg += 2;
        }
        else if (productType == PRODUCT_CES &&
                 kernelType == KERNEL_EXATA &&
                 !strcmp(argv[thisArg], "-emulation")) {
            isEmulationMode = TRUE;
            thisArg += 1;
        }
        else if (!strcmp(argv[thisArg], "-simulation")) {
            if (productType == PRODUCT_EXATA
            || productType == PRODUCT_JNE) {  
                isEmulationMode = FALSE; 
            }
            thisArg += 1;
        }
        else if (!strcmp(argv[thisArg], "-db_regression")) {
            dbRegression = TRUE;
            thisArg += 1;
        }
        else if (argv[thisArg][0] != '-') {
            // Non-dash option
            if (numNonDash == 0)
            {
                // Config file name
                simProps.configFileName = argv[thisArg];
                isConfigFileSet = TRUE;
            }
            else if (numNonDash == 1)
            {
                // Experiment name.  Save room for .stat and .trace.
                strncpy(experimentPrefix,
                        argv[thisArg], MAX_STRING_LENGTH - 6);
                experimentPrefix[MAX_STRING_LENGTH - 7] = '\0';
                sprintf(statFileName, "%s.stat", experimentPrefix);
                sprintf(traceFileName, "%s.trace", experimentPrefix);
            }
            else
            {
                printf("unknown option %s\n", argv[thisArg]);
            }
            thisArg++;
            numNonDash++;
        }
#ifdef AGI_INTERFACE
        else if (!strcmp(argv[thisArg], "-agi_interface_max_stk_ver")) {
            AgiInterfaceCommandlineHandler(argv[thisArg]);
        }
#endif
        else
        {
            printf("unknown option %s\n", argv[thisArg]);
            thisArg++;
        }
    }
    if (!isConfigFileSet && !isHelpPrinted) {
        //print usage if no input file or other help option is set 
        PARTITION_PrintUsage(argv[0]);
    }
    return isConfigFileSet;
}


/* FUNCTION     PARTITION_SetSimulationEndTime
 * PURPOSE      To end the simulation in middle of execution
 *                Typically called by external interfaces, or upon
 *                external interrupts.
 *
 * Parameters
 *    partitionData: ParitionData *: pointer to partitionData
 *    duration : clocktype : interval after which the simulation must end
 */
void PARTITION_SetSimulationEndTime(
    PartitionData *partitionData,
    clocktype duration)
{
#ifdef PARALLEL //Parallel
    // examine safe time for the earliest allowed time for termination.
    if (partitionData->safeTime != CLOCKTYPE_MAX && duration != CLOCKTYPE_MAX)
    {
        // examine safe time for the earliest allowed time for termination.
        if (duration < partitionData->safeTime)
        {
            duration = partitionData->safeTime;
        }
        // Add one so that the current event will complete on all partitions.
        // This will end up calling EXTERNAL_handleSetSimulationDuration ()
        duration += 1;
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
        partitionData->interfaceList.isStopping = TRUE;
        partitionData->interfaceList.stopTime = duration;

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

/* FUNCTION     PARTITION_HandleCtrlC
 * PURPOSE      Handler function invoked when the process is stopped after pressing CtrlC
 *
 * Parameters
 *    signum    The signal id received by this process
 */
void PARTITION_HandleCtrlC(int signum)
{
    // If this is not the Ctrl-C signal, ignore.
    if (signum != SIGINT)
    {
        return;
    }

    // Disable SIGINT signal handling. To ensure that this function is called
    // multiple time, if Ctrl-C is handled multiple times
    signal(SIGINT, SIG_DFL);

    PARTITION_RequestEndSimulation();
}

/* FUNCTION     PARTITION_GlobalInit
 * PURPOSE      Call pre-initialiation routines before partitions are
 *              created.
 *
 * Parameters
 *    nodeInput              configuration data for the simulation
 *    numPartitions          the number of active partitions in this process
 */
#include "gestalt.h"

UTIL::Gestalt::Support::MapType UTIL::Gestalt::Support::attr_map;
UTIL::Gestalt::Support::SetType UTIL::Gestalt::Support::attr_set;

void InitializeSopsVopsGlobals(PartitionData* partitionData)
{
    Message* globalInfoTimer = MESSAGE_Alloc(partitionData,
                                    DEFAULT_LAYER,
                                    MSG_SPECIAL_Timer,
                                    MSG_OCULA_Global);
    PARTITION_SchedulePartitionEvent(partitionData,
                                    globalInfoTimer,
                                    partitionData->getGlobalTime() + SECOND,
                                    false);
}

void PARTITION_GlobalInit(NodeInput* nodeInput,
                          int numberOfProcessors,
                          char* experimentPrefix)
{
    UTIL::Gestalt::Support::initialize(nodeInput,
                                       numberOfProcessors,
                                       experimentPrefix);

#if defined(ADDON_DB)
    extern DatabaseInterlock dbInterlock;

    dbInterlock.start();
#endif /* ADDON_DB */

    if (OculaInterfaceEnabled())
    {
        char value[MAX_STRING_LENGTH];

        SetOculaProperty(NULL, "/scenario/name", experimentPrefix);
        
        sprintf(value, "%d", numberOfProcessors);
        SetOculaProperty(NULL, "/global/processorCount", value);
    }
}

/* FUNCTION     StatsDb_FinalizeLock
 * PURPOSE      Call finalization routines after partitions are
 *              destroyed.
 *
 */
void StatsDb_FinalizeLock(void)
{
#if defined(ADDON_DB)

    StatsDbFinalize();

#endif /* ADDON_DB */
}

PartitionData::PartitionData(int thePartitionId, int numPartitions)
{
    partitionId = thePartitionId;
    numNodes = 0;
    seedVal = 0;
    nodePositions = NULL;
    terrainData = NULL;
    addressMapPtr = NULL;
    memset(&nodeIdHash, 0, sizeof(IdToNodePtrMap) * 32);
    safeTime = 0;
    nextInternalEvent = 0;
    externalInterfaceHorizon = 0;
    setTime(0);
    theCurrentTimeDynamic = 0;
    maxSimClock = 0;
    startRealTime = 0;
    guiOption = FALSE;
    interfaceList.isActive = FALSE;
    nodeInput = NULL;
    nodeData = NULL;
    memset(&globalData, 0,
           sizeof(void*) * PartitionGlobalDataCount);
    numChannels = 0;
    numFixedChannels = 0;
    propChannel = NULL;

#ifdef ADDON_NGCNMS
    gridInfo = NULL;
    gridUpdateTime = NULL;
    gridAutoBuild = NULL;
    pathlossMatrixGrid = NULL;
    numUsedMatrix = 0;
    numMatrixErased = 0;
    numMatrixAdded = 0;
#endif

    numProfiles = 0;
    firstNode = NULL;
    firstRemoteNode = NULL;

    allNodes = new NodePointerCollection ();

    msgFreeListNum = 0;
    msgFreeList = NULL;
    msgPayloadFreeListNum = 0;
    msgPayloadFreeList = NULL;
    msgInfoFreeListNum = 0;
    msgInfoFreeList = NULL;
    splayNodeFreeListNum = 0;
    splayNodeFreeList = NULL;
    eventSequence = 0;
    memset(&heapSplayTree, 0, sizeof(HeapSplayTree));
    heapStdlib = NULL;
    memset(&mobilityHeap, 0, sizeof(MobilityHeap));
    looseEvsHeap = NULL;
    memset(&genericEventTree, 0, sizeof(SimpleSplayTree));
    memset(&processLastEventTree, 0, sizeof(SimpleSplayTree));
    numberOfEvents = 0;
    numberOfMobilityEvents = 0;
    weatherPatterns = 0;
    numberOfWeatherPatterns = 0;
    weatherMobilitySequenceNumber = 0;
    weatherMovementInterval = 0;
    statFd = NULL;
    traceEnabled = FALSE;
    traceFd = NULL;
    activeNode = NULL;
    realTimeLogEnabled = FALSE;
    realTimeFd = NULL;
    memset(&interfaceList, 0, sizeof(EXTERNAL_InterfaceList));
    memset(interfaceTable, 0, sizeof(EXTERNAL_Interface*) * EXTERNAL_TYPE_MAX);
    externalForwardCommunicator = 0;
    externalSimulationDurationCommunicator = 0;
    schedulerInfo = NULL;
    numAntennaPatterns = 0;
    numAntennaModels = 0;
    antennaPatterns = NULL;
    antennaModels = NULL;
    wallClock = new WallClock(this);
    userProfileDataMapping = NULL;
    trafficPatternMapping = NULL;
    isRealTime = FALSE;


    // Communicators: Cross partition message handlers, dispatched directly
    // without a large switch statement.
    // The new oper used as the memset clobbers key internals if actual decl.
    communicators = new PartitionCommunicatorMap ();
    communicatorArraySize = 2;
    handlerArray = (PartitionCommunicationHandler *)
        MEM_malloc (sizeof (PartitionCommunicationHandler) * 32);
    handlerArray [0] = NULL;
    // NOTE, we skip slot 0, because we want an ID of 0 to be invalid
    nextAvailableCommunicator = 1;
    // mutex       communicatorsLock;
    communicatorsFrozen = false;

    clientState = new ClientStateDictionary ();

#ifdef PARALLEL
    lookaheadCalculator = new LookaheadCalculator;
    memset(&remoteNodeIdHash, 0, sizeof(IdToNodePtrMap) * 32);
    reportedEOT = 0;
    looseSynchronization = FALSE;
#endif

#ifdef ADDON_NGCNMS
    memset(ngcHistFilename, 0, sizeof(char) * MAX_STRING_LENGTH);
    memset(ngcStatFilename, 0, sizeof(char) * MAX_STRING_LENGTH);
    ngcHistUpdateInterval = 0;
#endif

    memset(&subnetData, 0, sizeof(PartitionSubnetData));

#ifdef ADDON_NGCNMS
    isLazy = FALSE;
    isGreedy = FALSE;
    errorBound = FALSE;
#endif
#ifdef ADDON_STATS_MANAGER
    statsManagerData = NULL;
#endif
#ifdef ADDON_DB
    statsDb = NULL;
#endif
    m_numPartitions = numPartitions;
    m_numThreadsPerPartition = 1;
    looseSynchronization = g_looseSynchronization;
    looseEvsHeap = new StlHeap ();

    isAgiInterfaceEnabled = FALSE;

    dynamicPropDelayNodePositionData = 
        new PARALLEL_PropDelay_NodePositionData;

    // SendMT
    sendMTListMutex      = new QNThreadMutex ();
    sendMTList           = new std::list <Message *> ();

#ifdef EXATA
    virtualLanGateways = new std::map<int, int>;
#endif
}

/*
 * FUNCTION     PARTITION_CreateEmptyPartition
 * PURPOSE      Function used to allocate and perform inititlaization of
 *              of an empty partition data structure.
 *
 * Parameters
 *     partitionId:          the partition ID, used for parallel
 *     numPartitions:        for parallel
 */
PartitionData* PARTITION_CreateEmptyPartition(
    int             partitionId,
    int             numPartitions)
{
    if (DEBUG)
    {
        printf("Creating partition %d\n", partitionId);
    }

    return new PartitionData(partitionId, numPartitions);
}

/*
 * FUNCTION     PARTITION_CreatePartition
 * PURPOSE      Function used to initialize a partition.
 *
 * Parameters
 *     terrainData:          dimensions, terrain database, etc.
 *     maxSimClock:          length of the scenario
 *     startRealTime:        for synchronizing with the realtime
 *     numNodes:             number of nodes in the simulation
 *     guiOption:            is GUI output enabled?
 *     traceEnabled:         is packet tracing enabled?
 *     addressMapPtr:        contains Node ID <--> IP address mappings
 *     nodePositions:        initial node locations and partition assignments
 *     nodeInput:            contains all the input parameters
 *     seedVal:              the global random seed
 *     experimentPrefix:     the experiment name
 */
void PARTITION_InitializePartition(
    PartitionData * partitionData,
    TerrainData*    terrainData,
    clocktype       maxSimClock,
    double          startRealTime,
    int             numNodes,
    BOOL            traceEnabled,
    AddressMapType* addressMapPtr,
    NodePositions*  nodePositions,
    NodeInput*      nodeInput,
    int             seedVal,
    int*            nodePlacementTypeCounts,
    char*           experimentPrefix,
    clocktype       startSimClock)
{
    BOOL           wasFound;
    char           buf[MAX_STRING_LENGTH];
    BOOL           guiOption;

    signal(SIGINT, &PARTITION_HandleCtrlC);

#if defined(SATELLITE_LIB)
    if (partitionData->partitionId == 0) {
        extern void UTIL_GlobalInit(int);

        UTIL_GlobalInit(partitionData->getNumPartitions());
    }
#endif
    guiOption = GUI_isAnimateOrInteractive ();

    partitionData->numNodes             = numNodes;
    partitionData->seedVal              = seedVal;
    partitionData->numberOfEvents       = 0;
    partitionData->numberOfMobilityEvents = 0;
    partitionData->terrainData          = terrainData;
    partitionData->maxSimClock          = maxSimClock;
    partitionData->startRealTime        = startRealTime;
    partitionData->nodeInput            = nodeInput;
    partitionData->addressMapPtr        = addressMapPtr;
    partitionData->nodePositions        = nodePositions;
    partitionData->guiOption            = guiOption;
    partitionData->traceEnabled         = traceEnabled;

    partitionData->safeTime                 = 0;
    partitionData->nextInternalEvent        = CLOCKTYPE_MAX;
    partitionData->externalInterfaceHorizon = CLOCKTYPE_MAX;

    /* NOTE: Dynamic objects may be created ONLY after this point */
    // Initialize the dynamic hierarchy
    partitionData->dynamicHierarchy.Initialize(partitionData, 1000);

    // Here we determine which partition(s) (if any) will be interfaced
    // to external applications.  Partition 0 is used by default.
    if (partitionData->partitionId == 0) {
        partitionData->interfaceList.isActive = TRUE;
    }
    else {
        partitionData->interfaceList.isActive = FALSE;
    }
    IO_ReadString(
        partitionData->partitionId,
        ANY_ADDRESS,
        nodeInput,
        "ACTIVATE-EXTERNAL-INTERFACE",
        &wasFound,
        buf);

    if (wasFound) {
        if (strcmp(buf, "YES") == 0) {
            partitionData->interfaceList.isActive = TRUE;
        }
        else if (strcmp(buf, "NO") == 0) {
            partitionData->interfaceList.isActive = FALSE;
        }
        else {
            // this is an error.
        }
    }

    partitionData->safeTime                 = 0;
    partitionData->nextInternalEvent        = CLOCKTYPE_MAX;
    partitionData->externalInterfaceHorizon = CLOCKTYPE_MAX;

#ifdef ADDON_NGCNMS
    IO_ReadString(
        partitionData->partitionId,
        ANY_ADDRESS,
        nodeInput,
        "LAZY-SCHEDULER-ENABLED",
        &wasFound,
        buf);

    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            partitionData->isLazy = TRUE;
        }
        else
        {
            partitionData->isLazy = FALSE;
        }
    }
    else
    {
        partitionData->isLazy = FALSE;
    }

    IO_ReadString(
        partitionData->partitionId,
        ANY_ADDRESS,
        nodeInput,
        "GREEDY-EVALUATION-ENABLED",
        &wasFound,
        buf);

    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            partitionData->isGreedy = TRUE;
        }
        else
        {
            partitionData->isGreedy = FALSE;
        }
    }
    else
    {
        partitionData->isGreedy = FALSE;
    }

    // disabled greedy evaluation if lazy scheduler is disabled
    if (partitionData->isGreedy == TRUE)
    {
        if (partitionData->isLazy == FALSE)
        {
            char errorStr[MAX_STRING_LENGTH] = "";
            sprintf(errorStr,
                    "Greedy Evaluation require Lazy Scheduler to enable: Greedy evaluation is now disabled");
            ERROR_ReportWarning(errorStr);
        }
    }

    double floatbuf = 0.0;
    IO_ReadDouble(
        partitionData->partitionId,
        ANY_ADDRESS,
        nodeInput,
        "GREEDY-EVALUATION-ERROR-BOUND",
        &wasFound,
        &floatbuf);

    if (wasFound)
    {
        partitionData->errorBound = floatbuf;

        if (partitionData->errorBound > 1 ||
            partitionData->errorBound < 0)
        {
            char errorStr[MAX_STRING_LENGTH] = "";
            sprintf(errorStr,
                    "GREEDY-EVALUATION-ERROR-BOUND be larger than or equals to 0 and smaller than or equal to 0");
            ERROR_ReportError(errorStr);
        }
    }
    else
    {
        partitionData->errorBound = 0.001;
    }
#endif
#ifdef ADDON_DB
    partitionData->pathlossSampleArray = NULL;
    partitionData->pathlossSampleTimeInterval = 0;
    partitionData->pathlossSampleIndex = 0;
    partitionData->isCreatePathLossMatrix = FALSE;

    STATSDB_Initialize(partitionData,
                       nodeInput,
                       experimentPrefix);
#endif
    // Someday there will be a designed method to configure which
    // (if any) external interfaces should be on/off for this partition.
    // There currently isn't such a scheme beyond each specific external
    // interface using its own config parameters for activation.

    partitionData->nodeData = (Node**) MEM_malloc(sizeof(Node*) * numNodes);
    memset(partitionData->nodeData, 0, sizeof(Node*) * numNodes);

    partitionData->propChannel = NULL;
#ifdef ADDON_NGCNMS
    partitionData->gridInfo = NULL;
    partitionData->gridAutoBuild = NULL;
    partitionData->pathlossMatrixGrid = NULL;
    partitionData->numUsedMatrix = 0;
    partitionData->numMatrixErased = 0;
    partitionData->numMatrixAdded = 0;
#endif
    partitionData->numChannels = 0;
    partitionData->numProfiles = 0;

    partitionData->mobilityHeap.minTime = CLOCKTYPE_MAX;
    partitionData->mobilityHeap.heapNodePtr = NULL;
    partitionData->mobilityHeap.heapSize = 0;
    partitionData->mobilityHeap.length = 0;

    partitionData->genericEventTree.rootPtr   = NULL;
    partitionData->genericEventTree.leastPtr  = NULL;
    partitionData->genericEventTree.timeValue = CLOCKTYPE_MAX;

    partitionData->processLastEventTree.rootPtr   = NULL;
    partitionData->processLastEventTree.leastPtr  = NULL;
    partitionData->processLastEventTree.timeValue = CLOCKTYPE_MAX;
    // Phy connecticity.
#ifdef ADDON_DB
    partitionData->connectSampleTimeInterval = 0;
    partitionData->isCreateConnectFile = FALSE;
    partitionData->ConnectionTable = NULL;
    partitionData->NodeMappingArray = NULL;
    partitionData->ConnectionTablePerChannel = NULL;

    IO_ReadString(
        partitionData->partitionId,
        ANY_ADDRESS,
        nodeInput,
        "CREATE-PATHLOSS-MATRIX",
        &wasFound,
        buf);

    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            partitionData->isCreatePathLossMatrix = TRUE;
        }
        else
        {
            partitionData->isCreatePathLossMatrix = FALSE;
        }
    }
    else
    {
        partitionData->isCreatePathLossMatrix = FALSE;
    }

    IO_ReadString(
        partitionData->partitionId,
        ANY_ADDRESS,
        nodeInput,
        "CREATE-PHY-CONNECTIVITY",
        &wasFound,
        buf);

    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            partitionData->isCreateConnectFile = TRUE;
        }
        else
        {
            partitionData->isCreateConnectFile = FALSE;
        }
    }
    else
    {
        partitionData->isCreateConnectFile = FALSE;
    }

    IO_ReadString(
        partitionData->partitionId,
        ANY_ADDRESS,
        nodeInput,
        "CONNECTIVITY-TABLE-OUTPUT-FILENAME",
        &wasFound,
        partitionData->connectFilename);

    if (wasFound == FALSE)
    {
        sprintf(partitionData->matrixFilename, "default.matrix");
    }

#endif

#ifdef CELLULAR_LIB
    partitionData->userProfileDataMapping = NULL;
    partitionData->userProfileDataMapping =
        USER_InitializeUserProfile(partitionData->userProfileDataMapping,
                                   nodeInput);

    partitionData->trafficPatternMapping = NULL;
    partitionData->trafficPatternMapping =
        USER_InitializeTrafficPattern(partitionData->trafficPatternMapping,
                                      nodeInput);
#endif // CELLULAR_LIB

#ifdef PARALLEL //Parallel
    PARALLEL_InitLookaheadCalculator(partitionData->lookaheadCalculator);
    partitionData->nextInternalEvent = 0;
    partitionData->reportedEOT       = 0;
#endif //endParallel

    /* File where stats will be stored for this partition.
     * Should use process id to avoid conflicts with other
     * qualnet sessions in this directory. */

    sprintf(buf, ".%s.%d", experimentPrefix, partitionData->partitionId);
    partitionData->statFd = fopen(buf, "w");
    ERROR_Assert(partitionData->statFd != NULL,
                 "Unable to create statistics file.");

    if (traceEnabled)
    {
        sprintf(buf, ".TRACE.%d", partitionData->partitionId);
        partitionData->traceFd = fopen(buf, "w");
        ERROR_Assert(partitionData->traceFd != NULL,
                     "Unable to create packet trace file.");
    }

#ifdef ADDON_STATS_MANAGER
    StatsManager_Initialize(partitionData,
                            experimentPrefix);
#endif
    partitionData->realTimeLogEnabled = FALSE;
    EXTERNAL_InitializeInterfaceList(&partitionData->interfaceList,
                                     partitionData);

#if defined(SATELLITE_LIB)
    void UTIL_PartitionInit(PartitionData *);
    UTIL_PartitionInit(partitionData);
#endif /* SATELLITE_LIB */

    partitionData->isAgiInterfaceEnabled = FALSE;

#ifdef EXATA
    NodeInput hostFile;

    // Initiatize the record-replay interface
    partitionData->rrInterface = new RecordReplayInterface();
    memset((char *)&partitionData->partitionOnSameCluster[0], 0, sizeof(BOOL) * 32);

    IO_ReadCachedFile(ANY_NODEID,
                      ANY_ADDRESS,
                      nodeInput,
                      "CLUSTER-HOST-FILE",
                      &wasFound,
                      &hostFile);

    if (!wasFound)
    {
        partitionData->clusterId = 0;
        partitionData->masterPartitionForCluster = 0;
        for (int i = 0; i < partitionData->getNumPartitions(); i++)
        {
            partitionData->partitionOnSameCluster[i] = TRUE;
        }
    } 
    else
    {
        int numCpus = 0;
        int cpuNum = 0;
        int index = 0;
        int oldNumCpus = 0;
        for (; index < hostFile.numLines; ++index)
        {                                  
            char iotoken[MAX_STRING_LENGTH];
            const char* delims = "= \t";
            char* next;
            char* token;
            BOOL find_a_num = FALSE; 
            int numTokens = 0 ;

            next = hostFile.inputStrings[index];    
            while (!find_a_num)
            {        
                token = IO_GetDelimitedToken(iotoken, next, delims, &next);
                if (token  == NULL)
                {
                    break ;
                }
                else
                {
                    ++numTokens;
                    if (!strcmp(token, "cpu"))
                    {                        
                        find_a_num = TRUE ;          
                        token = IO_GetDelimitedToken(iotoken, next, delims, &next);
                        if (token)
                        {
                            cpuNum = atoi(token) ; 
                        }
                        else 
                        {
                            ERROR_Assert(FALSE, "Please check the CLUSTER-HOST-FILE, cpu is "
                                         "not properly configured.");
                        }
                    }
                }
            }

            if (numTokens == 0)
            {
                ERROR_Assert(FALSE, "Please check the CLUSTER-HOST-FILE.");
            }

            if (find_a_num == FALSE)
            {
                cpuNum = 1;
            }
            oldNumCpus = numCpus;
            numCpus += cpuNum ;
            
            if (numCpus >= partitionData->partitionId + 1)
            {
                break;
            }
        }
        if (numCpus < partitionData->partitionId + 1)
        {
            ERROR_Assert(FALSE, "Please check the CLUSTER-HOST-FILE, number of configured"
                         " cpus is less than that of partitions.");
        }
        partitionData->clusterId = ++index ;
        partitionData->masterPartitionForCluster = numCpus - cpuNum ;

        if (DEBUG)
        {
            printf("partition %d, has the following partitions on the same cluster \n",
                   partitionData->partitionId);
            for (int i = oldNumCpus; i < numCpus; ++i)
            {
                //partitionData->partitionOnSameCluster.push_back(i);
                partitionData->partitionOnSameCluster[i] = TRUE; 
                printf("\t %d", i);
            }
            printf("\n");
        }
        IO_ReadString(
            partitionData->partitionId,
            ANY_ADDRESS,
            nodeInput,
            "EXTERNAL-DELAY",
            &wasFound,
            buf);

        if (!wasFound)
        {
            partitionData->delayExt = -1;
        }
        else
        {
            partitionData->delayExt = TIME_ConvertToClock(buf);
        }

        double floatbuf = 0.0;

        IO_ReadDouble(
            partitionData->partitionId,
            ANY_ADDRESS,
            nodeInput,
            "EXTERNAL-DROP-PROB",
            &wasFound,
            &floatbuf);

        if (!wasFound)
        {
            partitionData->dropProbExt = -1;
        }
        else
        {
            partitionData->dropProbExt = floatbuf;
        }
    }
#endif

    if (OculaInterfaceEnabled() && partitionData->partitionId == 0)
    {
        InitializeSopsVopsGlobals(partitionData);
    }
    return;
}

static void AddNodeToList(Node*          newNode,
                          Node*          previousNode,
                          PartitionData* partitionData)
{
    if (partitionData->firstNode == NULL)
    {
        /* firstNode is currently NULL, so make this node
           firstNode. */

        partitionData->firstNode = newNode;
        partitionData->firstNode->prevNodeData = NULL;
        partitionData->firstNode->nextNodeData = NULL;
    }
    else
    {
        /* We encountered a node previously.
           Update the prevNodeData and nextNodeData fields. */

        assert(previousNode != NULL);
        newNode->prevNodeData      = previousNode;
        previousNode->nextNodeData = newNode;
        newNode->nextNodeData      = NULL;
    }
}

static void AddNodeToRemoteNodeList(Node*          newNode,
                          Node*          previousNode,
                          PartitionData* partitionData)
{
    if (partitionData->firstRemoteNode == NULL)
    {
        /* firstRemote is currently NULL, so make this node
           firstRemote. */

        partitionData->firstRemoteNode = newNode;
        partitionData->firstRemoteNode->prevNodeData = NULL;
        partitionData->firstRemoteNode->nextNodeData = NULL;
    }
    else
    {
        /* We encountered a node previously.
           Update the prevNodeData and nextNodeData fields to add this
           newNode at the tail of the list. */

        assert(previousNode != NULL);
        newNode->prevNodeData      = previousNode;
        previousNode->nextNodeData = newNode;
        newNode->nextNodeData      = NULL;
    }
}

static void RemoveNodeFromList(Node*          node,
                               PartitionData* partitionData) {
    if (partitionData->firstNode == node) {
        partitionData->firstNode = node->nextNodeData;
    }
    if (node->nextNodeData != NULL) {
        node->nextNodeData->prevNodeData = node->prevNodeData;
    }
    if (node->prevNodeData != NULL) {
        node->prevNodeData->nextNodeData = node->nextNodeData;
    }
}

void InitializeNodes_Sops(PartitionData* partitionData)
{
    char str[MAX_STRING_LENGTH];
    char str2[MAX_STRING_LENGTH];
    NodeInput* nodeInput = partitionData->nodeInput;
    
    // Configure interfaces
    for (Node* node = partitionData->firstNode;
        node != NULL;
        node = node->nextNodeData)
    {
        sprintf(str, "/node/%d/numberPhys", node->nodeId);
        sprintf(str2, "%d", node->numberPhys);
        SetOculaProperty(
            NULL,
            str,
            str2);

        for (int i = 0; i < node->numberPhys; i++)
        {
            sprintf(str, "/node/%d/phy/%d/protocol", node->nodeId, i);
            SetOculaProperty(
                NULL,
                str,
                PHY_GetPhyName(node, i));

            // Loop over all channels for this phy
            double maxRange = -1;
            double range;
            for (int channelIndex = 0;
                channelIndex < PROP_NumberChannels(node);
                channelIndex++)
            {
                range = PHY_PropagationRange(node, node, i, i, channelIndex, FALSE);
                if (range > maxRange)
                {
                    maxRange = range;
                }
            }

            sprintf(str, "/node/%d/phy/%d/propagationRange", node->nodeId, i);
            sprintf(str2, "%f", maxRange);
            SetOculaProperty(
                NULL,
                str,
                str2);
        }
    
        // Set icon
        BOOL wasFound = FALSE;
        // First reading GUI-NODE-3D-ICON
        IO_ReadString(node->nodeId, ANY_ADDRESS, nodeInput, "GUI-NODE-3D-ICON", &wasFound, str2);
        if (wasFound)
        {
            sprintf(str, "/node/%d/gui/model", node->nodeId);
            SetOculaProperty(
                NULL,
                str,
                str2);
        }
        if (!wasFound)
        {
            // Try reading GUI-NODE-2D-ICON
            IO_ReadString(node->nodeId, ANY_ADDRESS, nodeInput, "GUI-NODE-2D-ICON", &wasFound, str2);
            if (wasFound)
            {
                sprintf(str, "/node/%d/gui/model", node->nodeId);
                SetOculaProperty(
                    NULL,
                    str,
                    str2);
            }
        }
        if (!wasFound)
        {
            // Try reading NODE-ICON
            IO_ReadString(node->nodeId, ANY_ADDRESS, nodeInput, "NODE-ICON", &wasFound, str2);
            if (wasFound)
            {
                sprintf(str, "/node/%d/gui/model", node->nodeId);
                SetOculaProperty(
                    NULL,
                    str,
                    str2);
            }
        }

        // Set scale
        wasFound = FALSE;
        IO_ReadString(node->nodeId, ANY_ADDRESS, nodeInput, "GUI-NODE-SCALE", &wasFound, str2);
        if (wasFound)
        {
            sprintf(str, "/node/%d/gui/scale", node->nodeId);
            SetOculaProperty(
                NULL,
                str,
                str2);
        }

        // Set ground mobility
        sprintf(str, "/node/%d/groundNode", node->nodeId);
        if (node->mobilityData->groundNode)
        {
            strcpy(str2, "yes");
        }
        else
        {
            strcpy(str2, "no");
        }
        SetOculaProperty(
            NULL,
            str,
            str2);
    }

    // Synchronize then unlock
    PARALLEL_SynchronizePartitions(partitionData, BARRIER_TYPE_InitializeSopsVops);
    if (partitionData->partitionId == 0)
    {
        SetOculaProperty(
            NULL,
            "/locked",
            "0");
    }
}

void Initialize_Sops(PartitionData* partitionData)
{
    char str[MAX_STRING_LENGTH];
    NodeInput* nodeInput = partitionData->nodeInput;

    // Only partition 0 sets global parameters
    if (partitionData->partitionId != 0)
    {
        return;
    }

    SetOculaProperty(NULL, 
        "/locked",
        "1");

    sprintf(str, "%d", (int) partitionData->numNodes);
    SetOculaProperty(NULL, 
        "/partition/0/numNodes",
        str);

    SetOculaProperty( NULL,
        "/partition/0/theCurrentTime",
        "0");

    ctoa((clocktype)partitionData->maxSimClock, str);
    SetOculaProperty(NULL, 
        "/partition/0/maxSimClock",
        str);

    // Look for global scale
    BOOL wasFound = FALSE;
    IO_ReadString(ANY_NODEID, ANY_ADDRESS, nodeInput, "GUI-NODE-SCALE", &wasFound, str);
    if (wasFound)
    {
        SetOculaProperty(
            NULL,
            "/partition/0/scale",
            str);
    }

    // Look for terrain scale
    wasFound = FALSE;
    IO_ReadString(ANY_NODEID, ANY_ADDRESS, nodeInput, "GUI-TERRAIN-SCALE", &wasFound, str);
    if (wasFound)
    {
        SetOculaProperty(
            NULL,
            "/partition/0/terrainScale",
            str);
    }

    // Configure terrain

    TerrainData& terrainData = *partitionData->terrainData;
    if (terrainData.getCoordinateSystem() == LATLONALT)
    {
        SetOculaProperty(NULL, 
            "/partition/0/coordinateSystemType",
            "lat/lon/alt");
    }
    else if (terrainData.getCoordinateSystem() == CARTESIAN)
    {
        SetOculaProperty(NULL, 
            "/partition/0/coordinateSystemType",
            "cartesian");
    }
    else
    {
        ERROR_ReportError("Unknown terrain type");
    }

    sprintf(str, "%f, %f", terrainData.getSW().common.c1, terrainData.getSW().common.c2);
    SetOculaProperty(NULL, 
        "/partition/0/southWestCorner",
        str);

    sprintf(str, "%f, %f", terrainData.getNE().common.c1, terrainData.getNE().common.c2);
    SetOculaProperty(NULL, 
        "/partition/0/northEastCorner",
        str);

    // terrains is in form: "DEM path DEM path"
    std::string* terrains = terrainData.terrainFileList(partitionData->nodeInput, false);

    std::vector<std::string> terrainStrings;
    char str2[MAX_STRING_LENGTH];
    Coordinates sw;
    Coordinates ne;

#ifdef OCULA_INTERFACE
    OPS_SplitString(*terrains, terrainStrings, " ");

    sprintf(str, "%u", (UInt32)terrainStrings.size()  - 1);
    SetOculaProperty(NULL, 
        "/partition/0/numTerrains",
        str);
    for (unsigned int i = 1; i < terrainStrings.size(); i++)
    {
        sprintf(str, "/partition/0/terrain/%d/name", i - 1);
        SetOculaProperty(NULL, 
            str,
            terrainStrings[i]);

        terrainData.m_elevationData->getElevationBoundaries(i - 1, &sw, &ne);

        sprintf(str, "/partition/0/terrain/%d/southWestCorner", i - 1);
        sprintf(str2, "%f, %f", sw.common.c1, sw.common.c2);
        SetOculaProperty(NULL, 
            str,
            str2);

        sprintf(str, "/partition/0/terrain/%d/northEastCorner", i - 1);
        sprintf(str2, "%f, %f", ne.common.c1, ne.common.c2);
        SetOculaProperty(NULL, 
            str,
            str2);

        double highest;
        double lowest;
        terrainData.m_elevationData->getHighestAndLowestElevation(&sw,
                                              &ne,
                                              &highest,
                                              &lowest);
        sprintf(str, "/partition/0/terrain/%d/highestElevation", i - 1);
        sprintf(str2, "%f", highest);
        SetOculaProperty(NULL, 
            str,
            str2);
        sprintf(str, "/partition/0/terrain/%d/lowestElevation", i - 1);
        sprintf(str2, "%f", lowest);
        SetOculaProperty(NULL, 
            str,
            str2);
    }
#endif

    IO_ReadString(ANY_NODEID,
                  ANY_ADDRESS,
                  nodeInput,
                  "GUI-BACKGROUND-IMAGE-FILENAME",
                  &wasFound,
                  str);
    if (!wasFound) {
        strcpy(str, "");
    }

    SetOculaProperty(
        NULL,
        "/partition/0/backgroundImage",
        str);

    delete terrains;

    // Send urban terrain file info
    std::string* urbanFiles = terrainData.m_urbanData->fileList(nodeInput);

#ifdef OCULA_INTERFACE
    std::vector<std::string> urbanStrings;
    unsigned int i;
    OPS_SplitString(*urbanFiles, urbanStrings, " ");

    if (urbanStrings.size() > 0)
    {
        sprintf(str, "%u", (UInt32)urbanStrings.size()  - 1);
    }
    else
    {
        strcpy(str, "0");
    }
    SetOculaProperty(NULL, "/partition/0/numUrban", str);
    for (i = 1; i < urbanStrings.size(); i++)
    {
        sprintf(str, "/partition/0/urban/%d/name", i - 1);
        SetOculaProperty(NULL, str, urbanStrings[i]);
    }
#endif

    delete urbanFiles;
}

/*
 * FUNCTION     PARTITION_InitializeNodes
 * PURPOSE      Function used to allocate and initialize the nodes on a
 *              partition.
 *
 * Parameters
 *     partitionData: a pre-initialized partition data structure
 */
void PARTITION_InitializeNodes(PartitionData* partitionData) {
    int            i, j;

    Node*          nextNode  = NULL;
    NodeInput*     nodeInput = partitionData->nodeInput;
    NodePositions* nodePos   = partitionData->nodePositions;

    if (DEBUG)
    {
        printf("Creating local nodes on %d\n", partitionData->partitionId);
    }

    if (OculaInterfaceEnabled())
    {
        //Send the simulator initialization data to Sops
        Initialize_Sops(partitionData);
    }

    // Initalize scheduler for this partition
    SCHED_Initalize(partitionData, nodeInput);

    partitionData->stats = new STAT_StatisticsList(partitionData);
    // Add partition (global) objects to the hierarchy
    std::string path;
    D_Hierarchy *h = &partitionData->dynamicHierarchy;

    if (h->CreatePartitionPath(
            partitionData,
            "numNodes",
            path))
    {
        h->AddObject(
            path,
            new D_Int32Obj(&partitionData->numNodes));
        h->SetWriteable(
            path,
            FALSE);
    }

    if (h->CreatePartitionPath(
            partitionData,
            "theCurrentTime",
            path))
    {
        h->AddObject(
            path,
            new D_ClocktypeObj(&partitionData->theCurrentTimeDynamic));
        h->SetWriteable(
            path,
            FALSE);
    }

    if (h->CreatePartitionPath(
            partitionData,
            "maxSimClock",
            path))
    {
        h->AddObject(
            path,
            new D_ClocktypeObj(&partitionData->maxSimClock));
        h->SetWriteable(
            path,
            FALSE);
    }

// openssl one time calls(just done for 1st partition)
#ifdef CYBER_CORE
    if (partitionData->partitionId == 0 &&
       PKIIsEnabledForAnyNode(partitionData->numNodes, nodeInput))
    {
        PKIOpensslInit();
    }
#ifdef USE_MPI
    else if (PKIIsEnabledForAnyNode(partitionData->numNodes, nodeInput))
    {
        PKIOpensslInit();
    }
#endif // USE_MPI
#endif // CYBER_CORE


    /* Go through all nodes and create those that belong to this partition. */

#ifdef WIRELESS_LIB
    ANTENNA_GlobalAntennaModelPreInitialize(partitionData);
    ANTENNA_GlobalAntennaPatternPreInitialize(partitionData);

#endif // WIRELESS_LIB

    for (i = 0; i < partitionData->numNodes; i++) {
        Node* node = NODE_CreateNode(partitionData, nodePos[i].nodeId, nodePos[i].partitionId, i);
        partitionData->allNodes->push_back (node);
        node->partitionId = nodePos[i].partitionId;
        if (nodePos[i].partitionId == partitionData->partitionId) {
            //maybe should use something smaller than i here as an index.
            partitionData->nodeData[i] = node;

            MAPPING_HashNodeId(partitionData->nodeIdHash,
                               nodePos[i].nodeId,
                               partitionData->nodeData[i]);

            SCHED_InsertNode(partitionData,
                             partitionData->nodeData[i]);

            AddNodeToList(node, nextNode, partitionData);

            /* Make the current node we are considering the next node. */

            nextNode = node;
        }

#ifdef PARALLEL //Parallel
        else {
            MAPPING_HashNodeId(partitionData->remoteNodeIdHash,
                               nodePos[i].nodeId,
                               node);
#ifdef USE_MPI
            AddNodeToList(node, nextNode, partitionData);

            /* Make the current node we are considering the next node. */

            nextNode = node;
#endif

            // Maneesh: Read the hostnames for nodes in other partitions also
            // We do not worry about GUI_InitNode (that also reads hostnames) here
            BOOL wasFound;
            node->hostname = (char*)MEM_malloc(sizeof(char) * MAX_STRING_LENGTH);
            sprintf(node->hostname, "host%d", node->nodeId);
            IO_ReadString(
                node->nodeId,
                ANY_ADDRESS,
                nodeInput,
                "HOSTNAME",
                &wasFound,
                node->hostname);
        }
#endif //endParallel

    }

    /* Initialize all protocol layers for all nodes.
       The order of initialization is bottom-up (physical
       layer to application layer). */

    nextNode = partitionData->firstNode;
    BOOL wasFound;
    char name[MAX_STRING_LENGTH];
    while (nextNode != NULL)
    {
        nextNode->numberChannels = partitionData->numChannels;
        nextNode->propChannel = partitionData->propChannel;

        SimContext::setCurrentNode(nextNode);

        TRACE_Initialize(nextNode, nodeInput);
        MOBILITY_PostInitialize(nextNode, nodeInput);

        if (OculaInterfaceEnabled())
        {
            char key[MAX_STRING_LENGTH];
            char positionString[MAX_STRING_LENGTH];
            sprintf(key, "/node/%d/position", nextNode->nodeId);
            sprintf(positionString, "%f,%f,%f",
                nextNode->mobilityData->current->position.common.c1,
                nextNode->mobilityData->current->position.common.c2,
                nextNode->mobilityData->current->position.common.c3);
            SetOculaProperty(
                partitionData,
                key,
                positionString);
        }

        if (nextNode->guiOption) {
            GUI_InitNode(nextNode, nodeInput, nextNode->getNodeTime());
        }
        else
        {
            IO_ReadString(nextNode->nodeId, ANY_ADDRESS,
                nodeInput, "HOSTNAME", &wasFound, name);
            if (!wasFound)
            {
                sprintf(name, "host%u", nextNode->nodeId);
            }
            else
            {
                nextNode->hostname = name;
            }
        }

        if (OculaInterfaceEnabled())
        {
            char key[MAX_STRING_LENGTH];
            sprintf(key, "/node/%d/hostname", nextNode->nodeId);
            SetOculaProperty(
                partitionData,
                key,
                name);
        }

#if defined(SATELLITE_LIB)
        extern void UTIL_NodeInit(Node*, const NodeInput*);
        UTIL_NodeInit(nextNode, nodeInput);
#endif /* SATELLITE_LIB */

        NETWORK_PreInit(nextNode, nodeInput);

        PHY_Init(nextNode, nodeInput);
#ifdef WIRELESS_LIB
        BatteryInit(nextNode, nodeInput);
#endif // WIRELESS_LIB

#ifdef CYBER_LIB
        if (nextNode->partitionId == partitionData->partitionId)
        {
            // OS Resource modeling
            BOOL wasFound;
            BOOL osResourcesEnabled;

            IO_ReadBool(nextNode->nodeId,
                    ANY_ADDRESS,
                    nodeInput,
                    "OS-RESOURCE-MODEL",
                    &wasFound,
                    &osResourcesEnabled);

            if (wasFound && osResourcesEnabled)
            {
                nextNode->resourceManager = new OSResourceManager(nextNode);
                nextNode->resourceManager->init(nodeInput);
            }
        }
#endif

        assert((nextNode->nextNodeData == NULL) ||
               (nextNode->nextNodeData->prevNodeData == nextNode));
        assert((nextNode->prevNodeData == NULL) ||
               (nextNode->prevNodeData->nextNodeData == nextNode));
        nextNode = nextNode->nextNodeData;

        SimContext::unsetCurrentNode();
    }

#ifdef PARALLEL
    // Ensure that all nodes have been created before moving to the next
    // step.  This ensures the GUI has all nodes before networks are
    // created.
    if (partitionData->isRunningInParallel())
    {
#ifdef USE_MPI
        //PARALLEL_MessageBarrier(partitionData);
        PARALLEL_SynchronizePartitions (partitionData,
            BARRIER_TYPE_NodesCreated);
#else
        PARALLEL_GetRemoteMessagesAndBarrier(partitionData, 
            BARRIER_TYPE_NodesCreated);
#endif
    }
#endif

    // STATS DB CODE
#ifdef ADDON_DB
    nextNode = partitionData->firstNode;
    while (nextNode != NULL)
    {
        SimContext::setCurrentNode(nextNode);

        // Add this node's meta data values
        StatsDb* db = partitionData->statsDb;
        if (db != NULL && db->statsDescTable->createNodeDescTable)
        {
            nextNode->meta_data = new MetaDataStruct;
            if (nextNode->partitionId == partitionData->partitionId)
            {
                nextNode->meta_data->AddNodeMetaData(nextNode, partitionData, nodeInput);
                STATSDB_HandleNodeDescTableInsert(nextNode, partitionData);
            }
        }

        nextNode = nextNode->nextNodeData;

        SimContext::unsetCurrentNode();
    }
#endif

    // Write the XMLtrace header data to trace file
    if (partitionData->traceFd && partitionData->partitionId == 0)
    {
        TRACE_WriteXMLTraceHeader(nodeInput, partitionData->traceFd);

    }

//#endif

    // Initialize globally, rather than a node at a time.
    // no enter node needed here
    MAC_Initialize(partitionData->firstNode, nodeInput);

    for (i = 0; i < partitionData->getNumPartitions(); i++) {
        if (partitionData->partitionId == i) {
            nextNode = partitionData->firstNode;
            while (nextNode != NULL)
            {
                SimContext::setCurrentNode(nextNode);

                for (j = 0; j < partitionData->numChannels; j++) {
                    nextNode->propChannel = partitionData->propChannel;
                    PROP_Init(nextNode, j, nodeInput);
                }
                nextNode = nextNode->nextNodeData;

                SimContext::unsetCurrentNode();
            }
        }
#ifdef PARALLEL //Parallel
#ifndef USE_MPI
        PARALLEL_SynchronizePartitions (
            partitionData, BARRIER_TYPE_InitializeNodes);
#endif
#endif //endParallel
    }

    // allNodes holds references to every node, regardless of local/remote.
    // Subsequently, firstNode will hold the list of only local nodes.

#ifdef USE_MPI //Parallel
    Node * temp;
    // here remove all the nodes that don't belong to this partition.
    nextNode = partitionData->firstNode;
    Node * currentTail = NULL;
    while (nextNode != NULL) {
        int p = PARALLEL_GetPartitionForNode(nextNode->nodeId);
        temp = nextNode->nextNodeData;
        if (p != partitionData->partitionId) {
            RemoveNodeFromList(nextNode, partitionData);
            AddNodeToRemoteNodeList(nextNode, currentTail, partitionData);
            currentTail = nextNode;
        }
        nextNode = temp;
    }
#endif //endParallel

#ifdef PARALLEL_DEBUG //Parallel
    PARALLEL_SynchronizePartitions (partitionData, BARRIER_TYPE_Debug);
#endif //endParallel

    // Atm Layer2 initialize globally, rather than a node at a time.
    ATMLAYER2_Initialize(partitionData->firstNode, nodeInput);

    GUI_ReadAppHopByHopFlowAnimationEnabledSetting(nodeInput);

#ifdef JNE_LIB
    // Clear the previous simulation visualization if any
    if (JNE_IsVisualizationEffectEnabled(NULL))
    {
        JNE_ClearVisualization();
    }
#endif /* JNE_LIB */

    nextNode = partitionData->firstNode;
    while (nextNode != NULL)
    {
        SimContext::setCurrentNode(nextNode);
#ifdef JNE_LIB
        JNE_Initialize(nextNode, nodeInput);
#endif /* JNE_LIB */

        ADAPTATION_Initialize(nextNode, nodeInput);

        // If ther is any adaptation protocol initialize
        // the end systems & switch stack differently

        if ((nextNode->adaptationData.adaptationProtocol
            == ADAPTATION_PROTOCOL_NONE)
            || (nextNode->adaptationData.endSystem))
        {
        NETWORK_Initialize(nextNode, nodeInput);
        TRANSPORT_Initialize(nextNode, nodeInput);
#ifdef EXATA
        SocketLayerInit(nextNode, nodeInput);
#endif
        APP_Initialize(nextNode, nodeInput);
        USER_Initialize(nextNode, nodeInput);
        }

#ifdef JNE_LIB
        if (JNE_IsVisualizationEffectEnabled(nextNode))
        {
            JNE_InitVisFilterList(nextNode);
        }
#endif /* JNE_LIB */

        nextNode = nextNode->nextNodeData;

        SimContext::unsetCurrentNode();
    }

#ifdef EXATA
    partitionData->rrInterface->Initialize(partitionData);
#endif
    // Initialize globally, rather than a node at a time.
    APP_InitializeApplications(partitionData->firstNode, nodeInput);

    WEATHER_Init(partitionData, nodeInput);

    for (i = 0; i < partitionData->numNodes; i++) {
        if (partitionData->nodeData[i] != NULL) {
            Node* node = partitionData->nodeData[i];
            int channelIndex;
            BOOL retVal;
            BOOL yes;
            char hostname[MAX_STRING_LENGTH];

            SimContext::setCurrentNode(node);

            NODE_PrintLocation(
                node, PARTITION_GetTerrainPtr(partitionData)->getCoordinateSystem());

            // Print the hostname to the .stat file for easy reference later.
            IO_ReadBool(
                node->nodeId,
                ANY_ADDRESS,
                nodeInput,
                "HOST-STATISTICS",
                &retVal,
                &yes);

            // default is "NO"
            if (retVal == TRUE && yes) {
                sprintf(hostname, "Hostname = %s", node->hostname);
                IO_PrintStat(node,
                             "Network",
                             "Properties",
                             ANY_DEST,
                             -1,
                             hostname);
            }

            for (channelIndex = 0;
                 channelIndex < partitionData->numChannels;
                 channelIndex++)
            {
                int numNodes = node->propChannel[channelIndex].numNodes;
                PropPathProfile* newProfile;

                if (node->propData[channelIndex].limitedInterference) {
                    // If the node is interference limited,
                    // it needs to adjust its nodeListId value.
                    node->propData[channelIndex].nodeListId += numNodes;
                }
                else {
                    // If the node is not interference limited,
                    // allocate the full size of array.
                    numNodes += node->propChannel[channelIndex].numNodesWithLI;
                }

                //printf("Node %d allocating %d element array for channel %d\n",
                //       node->nodeId,
                //       numNodes,
                //       channelIndex);

                newProfile =
                    (PropPathProfile*)MEM_malloc(numNodes *
                                                 sizeof(PropPathProfile));
                for (j = 0; j < numNodes; j++) {
                    newProfile[j].propDelay = 0;
                    newProfile[j].distance = 0.0;
                    newProfile[j].txDOA.azimuth = 0;
                    newProfile[j].txDOA.elevation = 0;
                    newProfile[j].rxDOA.azimuth = 0;
                    newProfile[j].rxDOA.elevation = 0;
                    newProfile[j].pathloss_dB = NEGATIVE_PATHLOSS_dB;

                    // to force the path loss computation
                    // for the initial position
                    newProfile[j].sequenceNum = -1;
                }
                node->propData[channelIndex].pathProfile = newProfile;
            }

            SimContext::unsetCurrentNode();
        }
    }

#if defined(INTERFACE_JNE_C2ADAPTER)
    if (partitionData->partitionId == 0)
    {
        JNE::C2Adapter::construct_partition_subcomponent(partitionData);
    }
#endif /* INTERFACE_JNE_C2ADAPTER */

#if defined(INTERFACE_JNE_C3)
    if (partitionData->partitionId == 0)
    {
        JNE::C3::construct_partition_subcomponent(partitionData);
    }
#endif /* INTERFACE_JNE_C3 */

#if defined(INTERFACE_JNE_JREAP)
    if (partitionData->partitionId == 0)
    {
      JNE::JREAP::construct_partition_subcomponent(partitionData);
    }
#endif // JREAP

#ifdef PARALLEL //Parallel
    PARALLEL_SynchronizePartitions (partitionData, BARRIER_TYPE_GlobalReady);
#endif //endParallel

#if defined(SATELLITE_LIB)
    if (partitionData->partitionId == 0) {
        extern void UTIL_GlobalReady(void);

        UTIL_GlobalReady();
    }
#endif /* SATELLITE_LIB */
#ifdef ADDON_NGCNMS
    NgcStatsInit(partitionData);
#endif

#ifdef ADDON_BOEINGFCS
    //Pcom data structure initialization
    PHY_PcomInit(partitionData);

    nextNode = partitionData->firstNode;
    while (nextNode != NULL)
    {
        SimContext::setCurrentNode(nextNode);

        nextNode = nextNode->nextNodeData;

        SimContext::unsetCurrentNode();
    }
#endif

#ifdef ADDON_NGCNMS
    PropFlatBinningInit(partitionData);
#endif

#ifdef ADDON_DB
    StatsDb* db = partitionData->statsDb;
    if (db != NULL)
    {
    // Send the message for Aggregate stats.
    StatsDBSendAggregateTimerMessage(partitionData);
    StatsDBSendSummaryTimerMessage(partitionData);
        StatsDBSendConnTimerMessage(partitionData);
        StatsDBSendStatusTimerMessage(partitionData);
        StatsDBSendOspfTableTimerMessage(partitionData);
        StatsDBSendIpTableTimerMessage(partitionData);
        StatsDBSendPimTableTimerMessage(partitionData);
        StatsDBSendIgmpTableTimerMessage(partitionData);

#ifdef ADDON_BOEINGFCS
        StatsDBSendRegionTimerMessage(partitionData);
        StatsDBSendRapTimerMessage(partitionData);
        StatsDBSendMalsrTimerMessage(partitionData);
        StatsDBSendUsapTimerMessage(partitionData);
        StatsDBSendMdlQueueTimerMessage(partitionData);
        StatsDBSendMprTimerMessage(partitionData);
        StatsDBSendLinkAdaptationTimerMessage(partitionData);
        StatsDBSendLinkUtilizationTimerMessage(partitionData);
        StatsDBSrwSendUpdateTimerMessage(partitionData);
#endif // ADDON_BOEINGFCS

        if (db->statsStatusTable->createNodeStatusTable)
        {
            nextNode = partitionData->firstNode;
            while (nextNode != NULL)
            {
                SimContext::setCurrentNode(nextNode);

                StatsDBNodeStatus nodeStatus(nextNode, FALSE);
                // Add this node's status information to the database
                STATSDB_HandleNodeStatusTableInsert(nextNode, nodeStatus);
                nextNode = nextNode->nextNodeData;

                SimContext::unsetCurrentNode();
            }
        }
    }
#endif
#ifdef ADDON_STATS_MANAGER
    StatsManager_PostInitialization(partitionData);
#endif
    // DHCP
    nextNode = partitionData->firstNode;
    while (nextNode != NULL)
    {
        SimContext::setCurrentNode(nextNode);
        AppDhcpInitialization(nextNode, nodeInput);
        nextNode = nextNode->nextNodeData;

        SimContext::unsetCurrentNode();
    }

    if (OculaInterfaceEnabled())
    {
        //Send the simulator initialization data to Sops
        InitializeNodes_Sops(partitionData);    
    }
}

// Class to sort node ids for finalizing nodes in order
class CompareNodeIds
{
public:
    bool operator () (const Node* a, const Node* b)
    {
        return a->nodeId < b->nodeId;
    }
};

void Ocula_ProcessEvent(PartitionData* partitionData, Message* msg)
{
    char key[MAX_STRING_LENGTH];
    char value[MAX_STRING_LENGTH];
    clocktype now = partitionData->getGlobalTime();

    if (partitionData->partitionId == 0)
    {
        sprintf(key, "/partition/0/theRealTime");
        sprintf(value, "%lf", partitionData->wallClock->getRealTimeAsDouble());        
        SetOculaProperty(partitionData, key, value);
    }

    sprintf(key, "/partition/%d/global/eventCount", partitionData->partitionId);
    sprintf(value, "%"TYPES_64BITFMT"d", partitionData->numberOfEvents);
    SetOculaProperty(partitionData, key, value);

    partitionData->stats->Aggregate(partitionData);

    sprintf(key, "/partition/%d/global/app/unicastThroughput", partitionData->partitionId);
    sprintf(value, "%.lf", partitionData->stats->global.appAggregate.GetThroughput(STAT_Unicast).GetValue(now));
    SetOculaProperty(partitionData, key, value);

    sprintf(key, "/partition/%d/global/app/multicastThroughput", partitionData->partitionId);
    sprintf(value, "%.lf", partitionData->stats->global.appAggregate.GetThroughput(STAT_Multicast).GetValue(now));
    SetOculaProperty(partitionData, key, value);

    sprintf(key, "/partition/%d/global/app/broadcastThroughput", partitionData->partitionId);
    sprintf(value, "%.lf", partitionData->stats->global.appAggregate.GetThroughput(STAT_Broadcast).GetValue(now));
    SetOculaProperty(partitionData, key, value);

    sprintf(key, "/partition/%d/global/net/throughput", partitionData->partitionId);
    sprintf(value, "%.lf", partitionData->stats->global.netAggregate.GetCarriedLoad().GetValue(now));
    SetOculaProperty(partitionData, key, value);

    sprintf(key, "/partition/%d/global/phy/utilization", partitionData->partitionId);
    sprintf(value, "%.9lf", partitionData->stats->global.phyAggregate.GetAverageUtilization().GetValue(now));
    SetOculaProperty(partitionData, key, value);

    if (msg)
    {
        PARTITION_SchedulePartitionEvent(partitionData, msg, partitionData->getGlobalTime() + SECOND, false);
    }
}

/*
 * FUNCTION     PARTITION_Finalize
 * PURPOSE      Finalizes the nodes on the partition.
 *
 * Parameters
 */
void PARTITION_Finalize(PartitionData* partitionData)
{
    if (OculaInterfaceEnabled())
    {
        Ocula_ProcessEvent(partitionData, NULL);
    }
    /* Insert the simulation end time in the stat file */
    if (partitionData->partitionId == 0)
    {
        // Output stat at minimum node
        Node* minNode = partitionData->firstNode;
        for (NodePointerCollectionIter it = partitionData->allNodes->begin();
             it != partitionData->allNodes->end();
             it++)
        {
            if ((**it).nodeId < minNode->nodeId)
            {
                minNode = *it;
            }
        }

        clocktype simEndTime =
            clocktype(partitionData->maxSimClock);
        char timeStrBuf[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(
                simEndTime,
                timeStrBuf,
                minNode);

    }

#ifdef ADDON_DB
    if (partitionData->isCreatePathLossMatrix)
    {
        PROP_FinalizePathlossMatrixOutput(partitionData);
    }

    StatsDbFinalize(partitionData);
#endif

#if defined(INTERFACE_JNE_C2ADAPTER)
   if (partitionData->partitionId == 0)
   {
     JNE::C2Adapter::finalize_partition_subcomponent(partitionData);
   }
#endif /* INTERFACE_JNE_C2ADAPTER */

#if defined(INTERFACE_JNE_C3)
   if (partitionData->partitionId == 0)
   {
     JNE::C3::finalize_partition_subcomponent(partitionData);
   }
#endif /* INTERFACE_JNE_C3 */

#if defined(INTERFACE_JNE_JREAP)
     JNE::JREAP::finalize_partition_subcomponent(partitionData);
#endif // JREAP

    // Create a vector of all nodes, sort the nodes, then finalize the nodes
    // according to node id.  This will guarantee that the .stat.x files
    // will be sorted by node order when running in parallel.
    std::vector<Node*> nodes;
    if (partitionData->firstNode != NULL)
    {
        Node *nextNode = partitionData->firstNode;

        while (nextNode != NULL)
        {
            nodes.push_back(nextNode);
            nextNode = nextNode->nextNodeData;
        }
    }
    CompareNodeIds cmp;
    std::sort(nodes.begin(), nodes.end(), cmp);

    for (std::vector<Node*>::iterator it = nodes.begin();
         it != nodes.end();
         it++)
    {
        Node* nextNode = *it;

        SimContext::setCurrentNode(nextNode);
        GUI_SendFinalizationStatus(nextNode->nodeId);
        PHY_Finalize(nextNode);
        MAC_Finalize(nextNode);
        BatteryFinalize(nextNode);

        ATMLAYER2_Finalize(nextNode);
        ADAPTATION_Finalize(nextNode);

        if ((nextNode->adaptationData.adaptationProtocol
             == ADAPTATION_PROTOCOL_NONE)
            || (nextNode->adaptationData.endSystem))
        {
            NETWORK_Finalize(nextNode);
            TRANSPORT_Finalize(nextNode);
            APP_Finalize(nextNode);
            USER_Finalize(nextNode);
            MOBILITY_Finalize(nextNode);
        }

#ifdef JNE_LIB
        JNE_Finalize(nextNode);
#endif /* JNE_LIB */

        SimContext::unsetCurrentNode();

#ifdef LTE_LIB
        EpcLteFinalize(nextNode);
#endif // LTE_LIB
    }

    // Finalize scheduler for this partition
    SCHED_Finalize(partitionData);

    fclose(partitionData->statFd);

    if (partitionData->traceEnabled)
    {
        if (partitionData->partitionId == (partitionData->getNumPartitions() - 1))
        {
            TRACE_WriteXMLTraceTail(partitionData->traceFd);
        }
        fclose(partitionData->traceFd);
    }

#ifdef ADDON_DB
    StatsDbDriverClose(partitionData) ;
#endif
    // Finalize the utility library
#if defined(SATELLITE_LIB)
    extern void UTIL_PartitionFinalize(PartitionData*);
    extern void UTIL_GlobalEpoch(void);

    UTIL_PartitionFinalize(partitionData);
    // if last one should call UTIL_GlobalEpoch()
#endif /* SATELLITE_LIB */
}

/*
 * FUNCTION     PARTITION_PrintRunTimeStats
 * PURPOSE      If dynamic statistics reporting is enabled,
 *              generates statistics for enabled layers.
 *
 * Parameters
 *     partitionData: a pre-initialized partition data structure
 */
void PARTITION_PrintRunTimeStats(PartitionData* partitionData) {

    if (partitionData->firstNode != NULL)
    {
        Node *nextNode = partitionData->firstNode;

        while (nextNode != NULL)
        {
            SimContext::setCurrentNode(nextNode);

            // if the GUI is active, send only the data requested
            // by the GUI, otherwise print all data.
            if (partitionData->guiOption) {
                if (GUI_NodeIsEnabledForStatistics(nextNode->nodeId)) {
                    if (GUI_LayerIsEnabledForStatistics(GUI_APP_LAYER))
                    {
                        APP_RunTimeStat(nextNode);
                    }
                    if (GUI_LayerIsEnabledForStatistics(GUI_TRANSPORT_LAYER))
                    {
                        TRANSPORT_RunTimeStat(nextNode);
                    }
                    if (GUI_LayerIsEnabledForStatistics(GUI_NETWORK_LAYER))
                    {
                        NETWORK_RunTimeStat(nextNode);
                    }
                    if (GUI_LayerIsEnabledForStatistics(GUI_MAC_LAYER))
                    {
                        MAC_RunTimeStat(nextNode);
                    }
                    if (GUI_LayerIsEnabledForStatistics(GUI_PHY_LAYER))
                    {
#ifdef WIRELESS_LIB
                        BATTERY_RunTimeStat(nextNode);
#endif // WIRELESS_LIB
                    }
#ifdef CYBER_LIB
                    if (nextNode->resourceManager)
                    {
                        nextNode->resourceManager->runTimeStats();
                    }
#endif
                }
            }
            else {
                APP_RunTimeStat(nextNode);
                TRANSPORT_RunTimeStat(nextNode);
                NETWORK_RunTimeStat(nextNode);
                MAC_RunTimeStat(nextNode);
#ifdef WIRELESS_LIB
                        BATTERY_RunTimeStat(nextNode);
#endif // WIRELESS_LIB
#ifdef CYBER_LIB
                if (nextNode->resourceManager)
                {
                    nextNode->resourceManager->runTimeStats();
                }
#endif

            }
            nextNode = nextNode->nextNodeData;

            SimContext::unsetCurrentNode();
        }
    }
}

/*
 * FUNCTION     PARTITION_ReturnNodePointer
 * PURPOSE      Returns a pointer to the node or NULL if the node is not
 *              on this partition.  If remoteOK is TRUE, returns a pointer
 *              to this partition's proxy for a remote node if the node
 *              does not belong to this partition.  This feature should
 *              be used with great care, as the proxy is incomplete.
 *              Returns TRUE if the node was successfully found.
 *
 * Parameters
 *     partitionData: a pre-initialized partition data structure
 */
BOOL PARTITION_ReturnNodePointer(PartitionData* partitionData,
                                 Node**         node,
                                 NodeId         nodeId,
                                 BOOL           remoteOK) {

    *node = MAPPING_GetNodePtrFromHash(partitionData->nodeIdHash,
                                       nodeId);
    if (*node != NULL) {
        return TRUE;
    }
#ifdef PARALLEL //Parallel
    else if (remoteOK) {
        *node = MAPPING_GetNodePtrFromHash(partitionData->remoteNodeIdHash,
                                           nodeId);
        if (*node != NULL) {
            return TRUE;
        }
    }
#endif //endParallel

    return FALSE;
}


/*
 * FUNCTION     PARTITION_NodeExists
 * PURPOSE      Determines whether the node ID exists in the scenario.
 *              Must follow node creation.
 *
 * Parameters
 *     partitionData: a pre-initialized partition data structure
 */
BOOL PARTITION_NodeExists(PartitionData* partitionData,
                          NodeId         nodeId) {
    Node* node = MAPPING_GetNodePtrFromHash(partitionData->nodeIdHash,
                                            nodeId);
    if (node != NULL) {
        return TRUE;
    }
#ifdef PARALLEL //Parallel
    else {
        node = MAPPING_GetNodePtrFromHash(partitionData->remoteNodeIdHash,
                                          nodeId);
        if (node != NULL) {
            return TRUE;
        }
    }
#endif //endParallel
    return FALSE;
}
/*
 * FUNCTION     PARTITION_SchedulePartitionEvent
 * PURPOSE      Schedules a generic partition-level event.
 *
 * Parameters
 *     partitionData: a pre-initialized partition data structure
 *     message: an event
 */
void PARTITION_SchedulePartitionEvent(PartitionData* partitionData,
                                      Message*       msg,
                                      clocktype      eventTime,
                                      bool  scheduleBeforeNodes)
{
    SimpleSplayNode* splayNode = SPLAY_AllocateNode();
    splayNode->element = msg;
    splayNode->timeValue = eventTime;

    MESSAGE_DebugSend(partitionData, NULL, msg);

    if (scheduleBeforeNodes)
    {
        SPLAY_Insert(&(partitionData->genericEventTree), splayNode);
    }
    else
    {
        SPLAY_Insert(&(partitionData->processLastEventTree), splayNode);
    }
}

/*
 * FUNCTION     PARTITION_HandlePartitionEvent
 * PURPOSE      An empty function for protocols to use that need to
 *              schedule and handle partition-level events.
 *
 * Parameters
 *     partitionData: a pre-initialized partition data structure
 *     message: an event
 */
void PARTITION_HandlePartitionEvent(PartitionData* partitionData,
                                    Message*       msg) {

    // Debug and trace this message
    MESSAGE_DebugProcess(partitionData, NULL, msg);

    // user handling code goes here.

    if (msg->eventType == MSG_PROP_SignalReleased)
    {
        PROP_ProcessEvent(partitionData->firstNode, msg);
        return;
    }
#ifdef ADDON_DB
    else if (msg->layerType == STATSDB_LAYER)
    {
        // Stats DB process events
        STATSDB_ProcessEvent(partitionData, msg);
        return;
    }
#endif
    else if (msg->eventType == MSG_OCULA_Global)
    {
        Ocula_ProcessEvent(partitionData, msg);
        return;
    }
    // default behavior is to free the message.
    MESSAGE_Free(partitionData->firstNode, msg);
}

/*
 * function     PARTITION_SetRealTimeMode ()
 * purpose      Simulation should execute keeping up, but not faster than,
 *              real time. Examples are IPNE, HLA, or CES time managed.
 *
 * parameters
 *      partitionData: a pre-initialized partition data structure
 *      runAsRealtime: true to indicate real time execution
 */
void
PARTITION_SetRealTimeMode (PartitionData * partition, bool runAsRealtime)
{
    if (runAsRealtime)
    {
        partition->isRealTime = true;
    }
}

/*
 * FUNCTION     PARTITION_GetRealTimeMode ()
 * PURPOSE      Returns true if the simulation should execute
 *              keeping up, but not faster than,
 *              real time. e.g. IPNE, HLA, or CES time managed.
 *
 * Parameters
 *      partitionData: a pre-initialized partition data structure
 *
 */
bool
PARTITION_GetRealTimeMode (PartitionData * partitionData)
{
    return (partitionData->isRealTime);
}

void
PARTITION_ProcessSendMT (PartitionData * partitionData)
{
    // SendMT
    // if previewOfList > 0
    //      lock the list
    //      localList = partitionData->sendMTList
    //      replace the list with a new empty list
    //      unlock the list
    //      for message in localList
    //          MESSAGE_Send (message)
    if (partitionData->sendMTList->size () > 0)
    {
        std::list <Message *> *     localList;
        {
            // Now we must lock the list to safely access the queued up messages
            // The lock should exist only in this block, but could get
            // optimized away.  That would suck.
            QNThreadLock messageListLock (partitionData->sendMTListMutex);
            localList = partitionData->sendMTList;
            partitionData->sendMTList = new std::list <Message *> ();
        }
        std::list <Message *>::iterator localListIter;
        std::list <Message *>::iterator localListEnd;
        localListIter = localList->begin();
        localListEnd = localList->end();
        clocktype now = partitionData->getGlobalTime();
        clocktype delay;
        while (localListIter != localListEnd)
        {
            Message* msg = *localListIter;
            // add this message into the event queue.
            // NOTE - normally message's fields for nodeId and eventTime
            // are only filled in for message sent between partitons.
            // For SendMT the eventTime is delay, and nodeId is filled in!
            // NOTE we are treating "eventTime" as delay - See MESSAGE_SendMT ()
            Node* node = MAPPING_GetNodePtrFromHash(partitionData->nodeIdHash,
                                                    msg->nodeId);
            delay = msg->eventTime - now;
            if (delay < 0)
            {
                delay = 0;
            }
            // JDL: Bound delay by safetime
            if (partitionData->safeTime != CLOCKTYPE_MAX ) 
            {
                delay = MAX(delay, partitionData->safeTime
                            - partitionData->getGlobalTime());
            }
            if (DEBUG)
            {
                printf("at %" TYPES_64BITFMT "d: partition %d node %3d scheduling thread "
                       "event %3d with delay %" TYPES_64BITFMT "d on interface %2d\n",
                       partitionData->getGlobalTime(),
                       partitionData->partitionId,
                       node->nodeId,
                       msg->eventType,
                       delay,
                       msg->instanceId);
                fflush(stdout);
            }
            SCHED_InsertMessage(node, msg, delay);
            localListIter++;
        }
        delete localList;
    }
}


void PARTITION_ShowProgress(
    PartitionData* partitionData,
    const char* message,
    bool printSimTime,
    bool printDateTime)
{
    char buf[MAX_STRING_LENGTH];

    if (printSimTime)
    {
        double realTime = partitionData->wallClock->getRealTimeAsDouble();

        TIME_PrintClockInSecond(partitionData->getGlobalTime() +
                                partitionData->nodeInput->startSimClock,
                                buf);
        printf(
            "Current Sim Time[s] =%15s  Real Time[s] =%5d  ",
            buf,
           (int) realTime);
    }

    printf("%s", message);

    if (printDateTime)
    {
        time_t rawtime;
        struct tm* timeinfo;

        // Get time in seconds and milliseconds
        time(&rawtime);
        double trueRealTime = partitionData->wallClock->getTrueRealTimeAsDouble();

        int ms = (int) (1000 * (trueRealTime - floor(trueRealTime)));

        timeinfo = localtime(&rawtime);
        strftime(buf, MAX_STRING_LENGTH, "%Y-%m-%d %H:%M:%S", timeinfo);

        printf(" at %s.%03d\n", buf, ms);
    }
    else
    {
        printf("\n");
    }

    fflush(stdout);
}

void PARTITION_RunStatsDbRegression(char* prefix)
{
#ifdef ADDON_DB
    // Run regression.  This function does not return.
    STATSDB_Regression(prefix);
#endif
}

//Runtime check if military libraries is enabled
bool PARTITION_IsMilitaryLibraryEnabled()
{
#ifdef MILITARY_RADIOS_LIB
    return true;
#else
    return false;
#endif
}
