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
#include "user_profile_parser.h"

#define NUM_USER_BEHAVIOR_LINES 2
#define DEFAULT_STRING {0}

//static functions
static UserProfileDataMapping* ReadUserProfileData (
                                        UserProfileDataMapping *dataMapping,
                                        int count,
                                        const NodeInput userInput,
                                        char *userName);


static int UserProfileDataHash (char *userProfName, int hashSize)
{
    int hash = 0;
    while (*userProfName != 0)
    {
        hash += *userProfName;
        userProfName ++;
    }

    return hash % hashSize;
}

static UserProfileDataMapping *UserProfileDataMappingInitialize (
                                            UserProfileDataMapping *mapping, 
                                            int hashSize)
{

    //Create a new hash table, and initialize it to zero.
    mapping->hashSize = hashSize;
    mapping->table = (UserProfileDataMappingRecord **) 
              MEM_malloc (sizeof (UserProfileDataMappingRecord*) * hashSize);
    memset (mapping->table,
            0 ,
            sizeof (UserProfileDataMappingRecord*) * hashSize);
    return mapping;
}

static UserProfileDataMapping* UserProfileDataMappingAdd (
                                    UserProfileDataMapping *dataMapping,
                                    UserProfileDataMappingRecord *userRec)
{
    int hash = 0;

    assert (dataMapping != NULL);
    assert (dataMapping->table != NULL);

    //add the record on to the hash table
    hash = UserProfileDataHash (userRec->data.userProfileName ,
                                dataMapping->hashSize);
    userRec->next = dataMapping->table [hash];
    dataMapping->table[hash] = userRec;

    return dataMapping;
}

//-----------------------------------------------------
// NAME:        UserProfileDataMappingLookup
// PURPOSE:     This function performs the lookup task for the user-profiles.
//              It is called by the node to set its user data based on the
//              user-profile set in the main configuration file.
// PARAMETERS:  node--pointer to the node data structure
//              name --name of the user-profile.
// RETURN:      UserProfileDataMappingRecord
//-----------------------------------------------------

UserProfileDataMappingRecord* UserProfileDataMappingLookup(Node *node,
                                                           char *name)
{
    UserProfileDataMapping *mapping;
    mapping = node->partitionData->userProfileDataMapping;
    UserProfileDataMappingRecord *rec;
    ERROR_Assert (mapping != NULL, "User Profile Data Mapping is NULL\n");
    assert (mapping->table != NULL);

    int hash = 0;

    hash = UserProfileDataHash(name, mapping->hashSize);
    rec = mapping->table [hash];

    while (rec != NULL)
    {
        if (strcmp(name, rec->data.userProfileName) == 0)
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
static UserProfileDataMappingRecord* UserProfileDataMappingLookup(
                                            UserProfileDataMapping *mapping,
                                            char *name)
{
    UserProfileDataMappingRecord *rec;
    assert (mapping != NULL);
    assert (mapping->table != NULL);

    int hash;

    hash = UserProfileDataHash(name, mapping->hashSize);
    rec = mapping->table [hash];

    while (rec != NULL)
    {
        if (strcmp(name, rec->data.userProfileName) == 0)
        {
            return rec;
        }
        
        rec = rec->next ;
    }

    return NULL;
}
#endif

//-----------------------------------------------------
// NAME:        USER_InitializeUserProfile
// PURPOSE:     This is an initialization function for the user-profile.
//              This function runs through the main configuration file, and
//              looks to see how many user-profile files are defined. Once
//              it has those details it gets into parsing the files and
//              extracting data into the structure.
// PARAMETERS:  userDataMapping--pointer to the user-profile hash table
//              nodeInput --pointer to main configuration file data.
// RETURN:      UserProfileDataMapping
//-----------------------------------------------------
UserProfileDataMapping* USER_InitializeUserProfile(
    UserProfileDataMapping* userDataMapping,
    const NodeInput*        nodeInput)
{

    NodeInput appInput;
    char userStr[MAX_STRING_LENGTH] = DEFAULT_STRING;
    int i = 0;
    int k = 0;
    int numDistFiles = 0;
    
    userDataMapping = (UserProfileDataMapping*)
                        MEM_malloc (sizeof (UserProfileDataMapping));
    //Initialize the hash table first
    userDataMapping = UserProfileDataMappingInitialize (userDataMapping, 31);

    BOOL retVal = FALSE;
    
    IO_ReadCachedFileInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "USER-PROFILE-FILE",
        0,
        TRUE,
        &retVal,
        &appInput);
    
    if (!retVal) {
        userDataMapping = NULL;
        return NULL;
    }
    
    assert(retVal == TRUE);
    
    numDistFiles++;
    
    // Scan all PHY-RX-BER-TABLE-FILE variables
    // and see how many of them are defined
    while (TRUE) {
        IO_ReadCachedFileInstance(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "USER-PROFILE-FILE",
            numDistFiles,
            FALSE,
            &retVal,
            &appInput);
        
        if (!retVal)
        {
            break;
        }
        numDistFiles++;
    }

    for (k = 0; k < numDistFiles ; k++)
    {
   
        IO_ReadCachedFileInstance(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "USER-PROFILE-FILE",
            k,
            (k == 0),
            &retVal,
            &appInput);
        
        if (!retVal)
        {
            printf ("Distribution file not found\n");
            userDataMapping = NULL;
            return NULL;
        }       
        
        char userProfileName [MAX_STRING_LENGTH] = DEFAULT_STRING;
        
        for (i = 0; i < appInput.numLines; i++)
        {
            sscanf(appInput.inputStrings[i],
                    "%s %s",
                    userStr,
                    userProfileName);
            
            if (strcmp(userStr, "USER-PROFILE") == 0)
            {
                userDataMapping = ReadUserProfileData(userDataMapping,
                                                        i+1,
                                                        appInput,
                                                        userProfileName);
            }
            else
            {
                //printf("The file not structured properly\n");
                //exit(0);
                continue;
            }
        }
    }

    return userDataMapping;
}

//-----------------------------------------------------
// NAME:        ReadUserProfileData
// PURPOSE:     Parses through the user distribution data and
//              stores in the data structures.
// PARAMETERS:  dataMapping-- data structure storing the distribution mapping
//              It is the hash table data structure
//              count -- Line number of the file
//              userInput -- The  input string of the file
//              userName -- Name of the distribution.
// RETURN:      UserProfileDataMapping*
//-----------------------------------------------------
static UserProfileDataMapping* ReadUserProfileData(
                                        UserProfileDataMapping *dataMapping,
                                        int count,
                                        const NodeInput userInput,
                                        char *userName)
{
    char userStr[MAX_STRING_LENGTH] = DEFAULT_STRING;
    char statusStr[MAX_STRING_LENGTH] = DEFAULT_STRING;
    char trafficPattern[MAX_STRING_LENGTH] = DEFAULT_STRING;

    char* tokenStr = NULL;
    char buf[MAX_STRING_LENGTH]= DEFAULT_STRING;
    char paramStr0[MAX_STRING_LENGTH]= DEFAULT_STRING;
    char paramStr1[MAX_STRING_LENGTH]= DEFAULT_STRING;

    RandomDistribution<Int32> *age = NULL;
    RandomDistribution<Int32> *sex = NULL;

    TrafficBehData *dataVal = NULL;
    int i = 0;

    UserProfileDataMappingRecord *newRec;
    newRec = (UserProfileDataMappingRecord *)
                MEM_malloc (sizeof (UserProfileDataMappingRecord));
    newRec->data.trafficBehData = NULL;

    strcpy (newRec->data.userProfileName, userName);
    
    for (i = count; i < (count + NUM_USER_BEHAVIOR_LINES); i++)
    {
        tokenStr = userInput.inputStrings[i];

        IO_GetToken(userStr, tokenStr, &tokenStr);
        
        if (strcmp (userStr , "AGE") ==0)
        {
            age = new RandomDistribution<Int32>();
            age->setSeed(i); //RAND, need something better
            age->setDistribution(tokenStr, "User Profile : AGE", RANDOM_INT);
            sscanf(tokenStr, "%s %s %s", buf, paramStr0, paramStr1);
            if (atol(paramStr0) < 0 || atol(paramStr1) < 0)
            {
                printf("AGE shouldn't be negative\n");
                exit(0);
            }
            if (age != NULL)
            {
                newRec->data.distRecAge =  age;
            }
            else
            {
                printf("Error: Age has invalid distribution\n");
                exit(0);
            }
            
        }
        else if (strcmp (userStr , "SEX") ==0)
        {
            sex = new RandomDistribution<Int32>();
            sex->setSeed(i); //RAND, need something better
            sex->setDistribution(tokenStr, "User Profile : SEX", RANDOM_INT);
            sscanf(tokenStr, "%s %s %s", buf, paramStr0, paramStr1);
            if (atol(paramStr0) < 0 || atol(paramStr1) < 0)
            {
                printf("SEX shouldn't be negative\n");
                exit(0);
            }
            if (sex != NULL)
            {
                newRec->data.distRecSex = sex;
            }
            else
            {
                printf("Error: Sex has invalid distribution\n");
                exit(0);
            }
        }
        else
        {
                printf("Error: Either AGE or SEX is not given\n");
                exit(0);
        }
    }//end of for
    
    for (  ; i < userInput.numLines ; i++)
    {
        
        sscanf (userInput.inputStrings [i], "%s %s", userStr, statusStr);
        if (strcmp (userStr, "USER-STATUS") == 0)
        {
            dataVal = (TrafficBehData *)
                                MEM_malloc (sizeof (TrafficBehData));
            strncpy(dataVal->statusName, statusStr, MAX_STRING_LENGTH);
            i++;
            sscanf (userInput.inputStrings [i],
                    "%s %s",
                    userStr,
                    trafficPattern);
            if (strcmp (userStr, "TRAFFIC-PATTERN") == 0)
            {
                strcpy(dataVal->name, trafficPattern);
                dataVal->next = NULL;
            }
            else
            {
                printf ("Error: Invalid ordering in User profile file\n");
                exit(0);
            }
            
            TrafficBehData *dataTraverse;
            if (newRec->data.trafficBehData == NULL)
            {
                newRec->data.trafficBehData = dataVal;
            }
            else
            {
                dataTraverse = newRec->data.trafficBehData ;
                
                while (dataTraverse->next  != NULL)
                {
                    dataTraverse = dataTraverse->next;
                    
                }
                
                dataTraverse->next = dataVal;
            }
            
            
        }//end of if
        else
        {
            i = userInput.numLines ;
        
        }
    
    }//end of for
    
    dataMapping = UserProfileDataMappingAdd (dataMapping, newRec);
    return dataMapping;
}
