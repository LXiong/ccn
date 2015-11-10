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

#include <boost/unordered_map.hpp>

#include <stdio.h>
#include <stdlib.h>
#include "ocula_tester.h"
#include "vops.h"

static bool printProperties = false;

void MoveNodeCallback(const char* key, const char* oldVal, const char* newVal, void** userData)
{
    //printf("Moved node %s to %s\n", OculaState::getSegment(key, 1).c_str(), newVal);
    //printf("Moved node \n");
    fflush(stdout);
}

void SimTimeCallback(const char* key, const char* oldVal, const char* newVal, void** userData)
{
    /*
    printf("Simulation time is now %s\n", newVal);
    fflush(stdout);
    /*if (*userData == NULL)
    {
        *userData = (void*) 0x12345678;
    }
    else
    {
        ;
    }*/    
}

void DataDumpCallback(const char* key, const char* oldVal, const char* newVal, void** userData)
{
    printf("%s = %s\n", key, newVal);
    fflush(stdout);
}

void LockedCallback(const char* key, const char* oldVal, const char* newVal, void** userData)
{
    // Set this so that the properties will be printed from main
    if (atoi(newVal) == 0)
    {
        printProperties = true;
    }
}

int main(int argc, char** argv)
{
    ExecSettings execSettings;
    int thisArg = 2;
 
    execSettings.numberOfProcessors = 1;
    execSettings.isEmulationMode = true;
    execSettings.interactivePort = DEFAULT_OCULA_PORT;    
    
    if (argc < 2)
    {
        printf("Usage: ocula_tester <scenario.config> [-np X] [-simulation]"
                                                "\n [-with_ocula_gui PORT_NUMBER]\n");
        exit(-1);
    }
    else if (argc > 2)
    {
        while(thisArg < argc)
        {
            if (!strcmp(argv[thisArg], "-np"))
            {
                if (argc < thisArg + 2)
                {
                    printf("Not enough arguments to -np.\n"
                                          "Correct Usage:\t -np X.\n");
                    exit(-1);
                }
                execSettings.numberOfProcessors = atoi(argv[thisArg+1]);
                thisArg += 2;
            }
            else if (!strcmp(argv[thisArg], "-simulation"))
            {
                execSettings.isEmulationMode = false;
                thisArg++;
            }
            else if (!strcmp(argv[thisArg], "-with_ocula_gui")) 
            {
                if (argc < thisArg + 2) 
                {
                    printf("Not enough arguments to -with_ocula_gui.\n"
                        "Correct usage:\t -with_ocula_gui PORT_NUMBER.\n");
                    exit(-1);
                }
                execSettings.interactivePort = atoi(argv[thisArg+1]);
                thisArg += 2;
            }
        }
    }
    execSettings.scenarioFilePath = argv[1];
    
    Vops* v = new Vops();
    PropertyManager::PropertyManagerIterator it;
    //v->m_state->registerCallback("/node/*/position", MoveNodeCallback);
    //v->m_state->registerCallback("/partition/0/theCurrentTime", SimTimeCallback);
    //v->m_state->registerCallback("/node/*/gui/model", DataDumpCallback);
    //v->m_state->registerCallback("/node/*/gui/color", DataDumpCallback);
    //v->m_state->registerCallback("/node/*/gui/icons", DataDumpCallback);
    //v->m_state->registerCallback("/node/*/exata/netLinkCount", DataDumpCallback);
    //v->m_state->registerCallback("/node/*/exata/netLink/*/previousHopId", DataDumpCallback);
    //v->m_state->registerCallback("/node/*/exata/netLink/*/packetsReceived", DataDumpCallback);
    //v->m_state->registerCallback("/node/*/exata/netLink/*/bytesReceived", DataDumpCallback);
    //v->m_state->registerCallback("/node/*/exata/appLinkCount", DataDumpCallback);
    //v->m_state->registerCallback("/node/*/exata/appLink/*/appId", DataDumpCallback);
    //v->m_state->registerCallback("/node/*/exata/appLink/*/sourceId", DataDumpCallback);
    //v->m_state->registerCallback("/node/*/exata/appLink/*/packetsReceived", DataDumpCallback);
    //v->m_state->registerCallback("/node/*/exata/appLink/*/bytesReceived", DataDumpCallback);
    //v->m_state->registerCallback("/partition/0/theRealTime", DataDumpCallback);
    v->m_state->registerCallback("/partition/*/global/cpuUsage", DataDumpCallback);
    v->m_state->registerCallback("/partition/*/global/app/unicastThroughput", DataDumpCallback);
    v->registerStdoutCallback(DataDumpCallback);
    v->m_state->registerCallback("/partition/*/global/eventCount", DataDumpCallback);
    // Callback to display all the data once the simulation is finished initializing
    v->m_state->registerCallback("/locked", LockedCallback);
    printf("\nLaunching Simulator\n"); fflush(stdout);
    
    execSettings.interactiveHostAddress = "127.0.0.1";
    v->launchSimulator(execSettings);
    v->connectToSimulator();

    bool first = true;
    pthread_rwlock_t* rwlock_properties = v->m_state->getLock();        
    
    // Wait on this Global variable to be set
    while (!printProperties)
    {
        v->readFromSimulator();
    }

    // Print out the properties since we got /locked
    pthread_rwlock_rdlock(rwlock_properties);
    printf("Before simulation size : %u\n",v->m_state->size());

    for (it = v->m_state->begin(); it != v->m_state->end(); ++it)
    {
        printf("%s = %s\n", it->first.c_str(), it->second->getValue().c_str());
        
    }
    pthread_rwlock_unlock(rwlock_properties);

    printf("Continuing simulation\n");
    fflush(stdout);

    v->play();

    v->issueHitlCommand("jammer");

    while (v->connectedToSimulator())      
    {
        v->readFromSimulator();
        OPS_Sleep(1);
    }

    pthread_rwlock_rdlock(rwlock_properties);         

    printf("Simulation has finished\n");
    printf("After simulation size : %u\n", v->m_state->size());
    for (it = v->m_state->begin(); it != v->m_state->end(); ++it)
    {
        printf("%s = %s\n", it->first.c_str(), it->second->getValue().c_str());
    }
    pthread_rwlock_unlock(rwlock_properties);
    v->m_state->getInterface()->exit();
}
