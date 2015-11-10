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

#include <limits.h>
#include "types.h"
#include "api.h"
#include "memory.h"
#include "node.h"
#include "partition.h"
#include "user_trafficpattern_parser.h"

#define NUM_TRAFFIC_PATTERN 3
#define NUM_TRAFFIC_PATTERN_BEH 4
#define DEFAULT_STRING {0}

//static functions
static TrafficPatternDataMapping* ReadTrafficPatternData(
                                    const NodeInput* nodeInput,
                                    TrafficPatternDataMapping *dataMapping,
                                    int count,
                                    const NodeInput trafficInput,
                                    char *TrafficPatternName);

static char* SkipToken(char* token, const char* tokenSep, int skip)
{
    int idx = 0;

    if (token == NULL)
    {
        return NULL;
    }

    // Eliminate preceding space
    idx = (int)strspn(token, tokenSep);
    token = (idx < (signed int)strlen(token)) ? token + idx : NULL;

    while (skip > 0 && token != NULL)
    {
        token = strpbrk(token, tokenSep);

        if (token != NULL)
        {
            idx = (int)strspn(token, tokenSep);
            token = (idx < (signed int)strlen(token)) ? token + idx : NULL;
        }
        skip--;
    }
    return token;
}

static PatternAppData *ReadPatternAppData(
                        TrafficPatternDataMappingRecord *rec,
                        char *cellInput);

static int TrafficPatternDataHash(char *TrafficPatternName, int hashSize)
{
    int hash = 0;
    while (*TrafficPatternName != 0)
    {
        hash += *TrafficPatternName;
        TrafficPatternName ++;
    }

    return hash % hashSize;
}

static TrafficPatternDataMapping *TrafficPatternDataMappingInitialize(
                                    TrafficPatternDataMapping *mapping,
                                    int hashSize)
{

    //Create a new hash table, and initialize it to zero.
    mapping->hashSize = hashSize;
    mapping->table = (TrafficPatternDataMappingRecord **)
             MEM_malloc(sizeof(TrafficPatternDataMappingRecord*) * hashSize);
    memset(mapping->table,
           0 ,
           sizeof(TrafficPatternDataMappingRecord*) * hashSize);
    return mapping;
}

static TrafficPatternDataMapping* TrafficPatternDataMappingAdd(
                        TrafficPatternDataMapping *dataMapping,
                        TrafficPatternDataMappingRecord *trafficPatternRec)
{
    int hash = 0;

    assert(dataMapping != NULL);
    assert(dataMapping->table != NULL);

    //add the record on to the hash table
    hash = TrafficPatternDataHash(trafficPatternRec->data.trafficPatternName
                                    , dataMapping->hashSize);
    trafficPatternRec->next = dataMapping->table [hash];
    dataMapping->table[hash] = trafficPatternRec;

    return dataMapping;
}

//-----------------------------------------------------
// NAME:        TrafficPatternDataMappingLookup
// PURPOSE:     This function performs the lookup task for the user-profiles.
//              It is called by the node to set its user data based on the
//              user-profile set in the main configuration file.
// PARAMETERS:  node--.pointer to the node data structure.
//              name-- name of the traffic-pattern
// RETURN:      TrafficPatternDataMappingRecord
//-----------------------------------------------------

TrafficPatternDataMappingRecord* TrafficPatternDataMappingLookup(
                                    Node *node,
                                    char *name)
{
    TrafficPatternDataMapping *mapping;
    mapping = node->partitionData->trafficPatternMapping;
    TrafficPatternDataMappingRecord *rec;
    assert(mapping != NULL);
    assert(mapping->table != NULL);

    int hash = 0;

    hash = TrafficPatternDataHash(name, mapping->hashSize);
    rec = mapping->table [hash];

    while (rec != NULL)
    {
        if (strcmp(name, rec->data.trafficPatternName) == 0)
        {
            return rec;
        }

        rec = rec->next ;
    }

    return NULL;

}

//This is unused Function.So it is in #if 0.Whenever developer want to use
//in future ,uncomment it.

#if 0
static TrafficPatternDataMappingRecord* TrafficPatternDataMappingLookup(
                                        TrafficPatternDataMapping *mapping,
                                        char *name)
{
    TrafficPatternDataMappingRecord *rec;
    assert(mapping != NULL);
    assert(mapping->table != NULL);

    int hash = 0;

    hash = TrafficPatternDataHash(name, mapping->hashSize);
    rec = mapping->table [hash];

    while (rec != NULL)
    {
        if (strcmp(name, rec->data.trafficPatternName) == 0)
        {
            return rec;
        }

        rec = rec->next ;
    }

    return NULL;
}
#endif
//-----------------------------------------------------
// NAME:        USER_InitializeTrafficPattern
// PURPOSE:     Initializes the distribution data structure
// PARAMETERS:  trafficPatternDataMapping--pointer to the traffic Pattern
//              data mapping
//              nodeInput -- pointer to main configuration file data.
// RETURN:      TrafficPatternDataMapping
//-----------------------------------------------------
TrafficPatternDataMapping* USER_InitializeTrafficPattern(
                        TrafficPatternDataMapping *trafficPatternDataMapping,
                        const NodeInput *nodeInput)
{

    NodeInput trafficInput;
    char trafficStr[MAX_STRING_LENGTH] = DEFAULT_STRING;
    int i = 0;
    int k = 0;
    int numtrafficFiles = 0;

    trafficPatternDataMapping = (TrafficPatternDataMapping*)
                               MEM_malloc(sizeof(TrafficPatternDataMapping));
    //Initialize the hash table first
    trafficPatternDataMapping = TrafficPatternDataMappingInitialize(
                                        trafficPatternDataMapping,
                                        31);

    BOOL retVal = FALSE;

    IO_ReadCachedFileInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "TRAFFIC-PATTERN-FILE",
        0,
        TRUE,
        &retVal,
        &trafficInput);

    if (retVal != TRUE) {
        trafficPatternDataMapping = NULL;
        return trafficPatternDataMapping;
    }

    assert(retVal == TRUE);

    numtrafficFiles++;

    // Scan all PHY-RX-BER-TABLE-FILE variables
    // and see how many of them are defined
    while (TRUE) {
        IO_ReadCachedFileInstance(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "TRAFFIC-PATTERN-FILE",
            numtrafficFiles,
            FALSE,
            &retVal,
            &trafficInput);

        if (!retVal)
        {
            break;
        }
        numtrafficFiles++;
    }

    for (k = 0; k < numtrafficFiles ; k++)
    {

        IO_ReadCachedFileInstance(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "TRAFFIC-PATTERN-FILE",
            k,
            (k == 0),
            &retVal,
            &trafficInput);

        if (!retVal)
        {
            printf("Distribution file not found\n");
            trafficPatternDataMapping = NULL;
            return NULL;
        }

        char trafficPatternName [MAX_STRING_LENGTH] = DEFAULT_STRING;

        for (i = 0; i < trafficInput.numLines; i++)
        {
            sscanf(trafficInput.inputStrings[i], "%s %s", trafficStr,
                    trafficPatternName);

            if (strcmp(trafficStr, "TRAFFIC-PATTERN") == 0)
            {
                trafficPatternDataMapping = ReadTrafficPatternData(
                                                nodeInput,
                                                trafficPatternDataMapping,
                                                i+1,
                                                trafficInput,
                                                trafficPatternName);
            }
            else
            {
                //printf("The file not structured properly\n");
                //exit(0);
                continue;
            }
        }
    }

    return trafficPatternDataMapping;

}

//-----------------------------------------------------
// NAME:        ReadTrafficPatternData
// PURPOSE:     Parses through the user distribution data and
//              stores in the data structures.
// PARAMETERS:  nodeInput--
//
//              dataMapping--
//              count -- Line number of the file
//              trafficInput --
//              trafficPatternName -- Name of the Traffic Pattern
// RETURN:      TrafficPatternDataMapping
//-----------------------------------------------------
static TrafficPatternDataMapping* ReadTrafficPatternData(
                                    const NodeInput* nodeInput,
                                    TrafficPatternDataMapping *dataMapping,
                                    int count,
                                    const NodeInput trafficInput,
                                    char *trafficPatternName)
{
    char trafficStr[MAX_STRING_LENGTH] = DEFAULT_STRING;
    char value[MAX_STRING_LENGTH] = DEFAULT_STRING;;
    RandomDistribution<Int32>* maxNum;
    RandomDistribution<clocktype>* arrivalInt;

    char* tokenStr = NULL;
    char buf[MAX_STRING_LENGTH]= DEFAULT_STRING;
    char paramStr0[MAX_STRING_LENGTH]= DEFAULT_STRING;

    float probability = 0.0;
    float retryProb = 0.0;
    short maxRetries = 0;
    int numApps = 0;

    int i = 0;

    BOOL wasFound = FALSE;
    int  globalSeed = 0;

    TrafficPatternDataMappingRecord *newRec;
    newRec = (TrafficPatternDataMappingRecord *)
                        MEM_malloc(sizeof(TrafficPatternDataMappingRecord));
    newRec->data.trafficBeh = NULL;
    newRec->next = NULL;

    strcpy(newRec->data.trafficPatternName, trafficPatternName);

    IO_ReadInt(ANY_NODEID,
               ANY_ADDRESS,
               nodeInput,
               "SEED",
               &wasFound,
               &globalSeed);

    for (i = count; i < (count + NUM_TRAFFIC_PATTERN); i++)
    {
        char* tokenStr = trafficInput.inputStrings[i];

        IO_GetToken(trafficStr, tokenStr, &tokenStr);

        if (strcmp(trafficStr , "NUM-APP-TYPES") ==0)
        {
            IO_GetToken(value, tokenStr, &tokenStr);
            newRec->data.numAppTypes = short(atoi(value));
            if(newRec->data.numAppTypes < 0)
            {
                printf("NUM-APP-TYPES shouldn't be negative\n");
                exit(0);
            }
            numApps = (int)newRec->data.numAppTypes ;
        }
        else if (strcmp(trafficStr , "MAX-NUM-APPS") ==0)
        {
            maxNum = new RandomDistribution<Int32>();
            maxNum->setSeed(globalSeed, i);
            maxNum->setDistribution(tokenStr,
                                "Traffic Pattern : MAX-NUM-APPS",
                                RANDOM_INT);
            sscanf(tokenStr, "%s %s",buf, paramStr0);
            if (atol(paramStr0) < 0)
            {
                printf("MAX-NUM-APPS shouldn't be negative\n");
                exit(0);
            }
            newRec->data.distMaxNumApp = maxNum;
        }
        else if (strcmp(trafficStr , "ARRIVAL-INTERVAL") ==0)
        {
            arrivalInt = new RandomDistribution<clocktype>();
            arrivalInt->setSeed(globalSeed, i);
            sscanf(tokenStr, "%s %s",buf, paramStr0);
            if (TIME_ConvertToClock(paramStr0) < 0)
            {
                printf("ARRIVAL-INTERVAL shouldn't be negative\n");
                exit(0);
            }
            arrivalInt->setDistribution(tokenStr,
                                    "Traffic Pattern : ARRIVAL-INTERVAL",
                                    RANDOM_CLOCKTYPE);
            newRec->data.distArrivalInt = arrivalInt;
        }
        else
        {
                printf("Either the Parameters above Traffic descriptor are "
                        " not correct or not in order.");
                exit(0);
        }
    }//end of for

    TrafficPatternBeh *nextTrafficBeh = NULL;
    TrafficPatternBeh *dataTraverse = NULL;

    while (numApps > 0 && i < trafficInput.numLines)
    {
        nextTrafficBeh = (TrafficPatternBeh *)
                                MEM_malloc (sizeof(TrafficPatternBeh));
        dataTraverse = NULL;
        sscanf(trafficInput.inputStrings [i], "%s %f",
                                trafficStr,
                                &probability);

        if (strcmp(trafficStr, "PROBABILITY") == 0)
        {
            if (probability >= 0 && probability <= 1)
            {
                nextTrafficBeh->probability = probability;
            }
            else
            {
                printf("PROBABILITY should lie between 0 and 1\n");
                exit(0);
            }
            i++;
            sscanf(trafficInput.inputStrings [i],
                    "%s %f",
                   trafficStr,
                   &retryProb);
            if (strcmp(trafficStr, "RETRY-PROBABILITY") == 0)
            {
                if (retryProb >= 0 && retryProb <= 1)
                {
                    nextTrafficBeh->retryProbability = retryProb;
                }
                else
                {
                    printf("RETRY-PROBABILITY should lie between 0 and 1\n");
                    exit(0);
                }
                i++;
                tokenStr = trafficInput.inputStrings[i];
                IO_GetToken(trafficStr, tokenStr, &tokenStr);
                if (strcmp(trafficStr, "RETRY-INTERVAL") == 0)
                {
                    nextTrafficBeh->distRetryInt =
                                        new RandomDistribution<clocktype>();
                    nextTrafficBeh->distRetryInt->setSeed(globalSeed, i);
                    sscanf(tokenStr, "%s %s",buf, paramStr0);
                    if (TIME_ConvertToClock(paramStr0) < 0)
                    {
                        printf("RETRY-INTERVAL shouldn't be negative\n");
                        exit(0);
                    }
                    nextTrafficBeh->distRetryInt->setDistribution(tokenStr,
                                        "Traffic Pattern : RETRY-INTERVAL",
                                        RANDOM_CLOCKTYPE);
                    i++;
                    sscanf(trafficInput.inputStrings [i],
                            "%s %hi",
                            trafficStr,
                            &maxRetries);
                    if (strcmp(trafficStr, "MAX-NUM-RETRIES") == 0)
                    {
                        if(maxRetries < 0)
                        {
                           printf("MAX-NUM-RETRIES shouldn't be negative\n");
                           exit(0);
                        }
                        else
                        {
                            nextTrafficBeh->maxNumRetry = maxRetries;
                        }
                        i++;
                        //Now to parse the user app parameters
                        //Call a new functions.
                        nextTrafficBeh->appData = ReadPatternAppData(
                            newRec, trafficInput.inputStrings[i]);
                        nextTrafficBeh->next = NULL;
                        numApps--;
                        i++;
                    }
                    else
                    {
                        printf("Error:Invalid ordering in Traffic Pattern"
                                    " File\n");
                        printf(" -> MAX-NUM-RETRIES expected\n");
                        exit(0);
                    }

                }
                else
                {
                    printf("Error:Invalid ordering in Traffic Pattern"
                                " File\n");
                    printf(" -> RETRY-INTERVAL expected\n");
                    exit(0);
                }
            }
            else
            {
                printf("Error:Invalid ordering in Traffic Pattern File\n");
                printf(" -> RETRY-PROBABILITY expected\n");
                exit(0);
            }
        }
        else
        {
            printf("Error:Invalid ordering in Traffic Pattern File\n");
            printf(" -> PROBABILITY expected\n");
            printf(" %s\n%s\n", trafficStr, trafficInput.inputStrings [i]);
            exit(0);
        }


        TrafficPatternBeh *dataTraverse;
        if (newRec->data.trafficBeh == NULL)
        {
            newRec->data.trafficBeh = nextTrafficBeh;
        }
        else
        {
            dataTraverse = newRec->data.trafficBeh ;

            while (dataTraverse->next)
            {
                dataTraverse = dataTraverse->next;
            }

            dataTraverse->next = nextTrafficBeh;
        }
    }//end of while

    dataMapping = TrafficPatternDataMappingAdd(dataMapping, newRec);
    return dataMapping;
}


static PatternAppData* ReadCellularAppData(
                        TrafficPatternDataMappingRecord *rec,
                        char *cellInput)
{
    char token[MAX_STRING_LENGTH] = DEFAULT_STRING;
    const char *tokensep = " ";
    char *remString = NULL;

    int nToken = 0;

    CellularAppData *appData;
    appData = (CellularAppData *) MEM_malloc(sizeof(CellularAppData));
    appData->init();

    IO_GetDelimitedToken(token, cellInput, tokensep, &remString);
    strcpy(appData->appName, token);

    nToken = appData->destinationNodeDistribution.setDistribution(
        remString,
        "Traffic Pattern : ABSTRACT-APP",
        RANDOM_DOUBLE);
    remString = SkipToken(remString, tokensep, nToken);

    nToken = appData->durationDistribution.setDistribution(
        remString,
        "Traffic Pattern : ABSTRACT-APP",
        RANDOM_CLOCKTYPE);
    remString = SkipToken(remString, tokensep, nToken);

    if (remString == NULL) {
        appData->numAppParams = 2;
        return appData;
    }

    nToken = appData->appTypeDistribution.setDistribution(
        remString,
        "Traffic Pattern : ABSTRACT-APP",
        RANDOM_INT);
    remString = SkipToken(remString, tokensep, nToken);

    if (remString == NULL) {
        appData->numAppParams = 3;
        return appData;
    }

    nToken = appData->bandwidthDistribution.setDistribution(
        remString,
        "Traffic Pattern : ABSTRACT-APP",
        RANDOM_DOUBLE);
    remString = SkipToken(remString, tokensep, nToken);

    appData->numAppParams = 4;
    return appData;
}

static PatternAppData *ReadPatternAppData(
                        TrafficPatternDataMappingRecord *rec,
                        char *cellInput)
{
    char token[MAX_STRING_LENGTH] = DEFAULT_STRING;
    const char *tokensep = " ";
    char *remString = NULL;

    IO_GetDelimitedToken(token, cellInput, tokensep, &remString);
    if (!strcmp(token, "CELLULAR-ABSTRACT-APP"))
    {
        return ReadCellularAppData(rec, cellInput);
    }
    else if (!strcmp(token, "CBR"))
    {
        //Here we handle the CBR application.
        ERROR_ReportError("Can only process CELLULAR-ABSTRACT-APP\n");
    }
    else
    {
        ERROR_ReportError("Can only process CELLULAR-ABSTRACT-APP\n");
    }

    return NULL;
}
