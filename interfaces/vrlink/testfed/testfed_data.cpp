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

// iostream.h is needed to support DMSO RTI 1.3v6 (non-NG).

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <climits>
#include <csignal>
#include <cassert>
#include <cerrno>
#include <algorithm>

#include "testfed_data.h"
#include "fileio.h"

static double s_convertDegreesToRadians = atan(1.0) / 45.0;
TestfedData*  g_testfedData = NULL;

// /**
// FUNCTION   :: Comp
// PURPOSE    :: Comparison function for message list heap creation
// PARAMETERS ::
// + a : Pointer to the testfed cmd message list data structure.
// + b : Pointer to the testfed cmd message list data structure.
// RETURN     ::  Bool
// **/
bool Comp(Testfed_CommandList*& a, Testfed_CommandList*& b)
{
    return a->time > b->time;
}

// /**
// FUNCTION   :: TestfedData
// PURPOSE    :: Constructor for the TestfedData class
// PARAMETERS :: 
// RETURN     ::  void : NULL
// **/
TestfedData::TestfedData(): m_siteId(1),
      m_applicationId(1),
      m_objectsRegistered(false),
      m_tickDelta(50000000)
{   
}

// /**
// FUNCTION   :: ReadTestfedCommandFile
// PURPOSE    :: Reads the .testfed command file and forms a cmdList 
// PARAMETERS :: 
// RETURN     ::  void : NULL
// **/
void TestfedData::ReadTestfedCommandFile()
{
    char iotoken[MAX_STRING_LENGTH];
    char line[MAX_STRING_LENGTH];
    char* next;
    char *token;
    char err[MAX_STRING_LENGTH];
    FILE *f;
    clocktype time;

    Testfed_RegisterObjectsCommand* registerCommand;
    Testfed_QuitCommand* quitCommand;
    Testfed_MoveEntityCommand* moveCommand;
    Testfed_ChangeEntityOrientationCommand* orientationCommand;
    Testfed_ChangeEntityDamageCommand* damageCommand;
    Testfed_ChangeRadioTxStateCommand* txStateCommand;
    Testfed_SendCERCommand* cerCommand;
    Testfed_ChangeEntityVelocityCommand* velocityCommand;

    char filename[g_vrlinkScenarioNameBufSize];
    strcpy(filename, m_scenarioName);
    strcat(filename, ".testfed");
    f = fopen(filename, "r");
    if (f == NULL)
    {
        sprintf(err, "Testfed could not open testfed command file \"%s\"", filename);
        ReportError(err);
    }

    // Continue reading lines until the end of the file
    char* temp = NULL;
    

    while (!feof(f))
    {
        temp = fgets(line, MAX_STRING_LENGTH, f);
                
        // Read first token
        token = IO_GetToken(iotoken, line, &next);
        if (token == NULL || token[0] == '#')
        {
            // Do nothing if empty line or comment
        }
        else if (strcmp(strupcase(token), "REGISTER") == 0)
        {            
            ParseRegisterCommand(
                iotoken,
                next,
                &time,
                &registerCommand);

            AddCommandToList(time, (Testfed_Command*) registerCommand);
        }
        else if (strcmp(strupcase(token), "QUIT") == 0)
        {
            ParseQuitCommand(
                iotoken,
                next,
                &time,
                &quitCommand);

            AddCommandToList(time, (Testfed_Command*) quitCommand);
        }
        else if (strcmp(strupcase(token), "MOVE") == 0)
        {
            ParseMoveEntityCommand(
                iotoken,
                next,
                &time,
                &moveCommand);

            AddCommandToList(time, (Testfed_Command*) moveCommand);
        }
        else if (strcmp(strupcase(token), "DAMAGE") == 0)
        {
            ParseChangeEntityDamageCommand(
                iotoken,
                next,
                &time,
                &damageCommand);

            AddCommandToList(time, (Testfed_Command*) damageCommand);
        }
        else if (strcmp(strupcase(token), "ORIENTATION") == 0)
        {
            ParseChangeEntityOrientationCommand(
                iotoken,
                next,
                &time,
                &orientationCommand);

            AddCommandToList(time, (Testfed_Command*) orientationCommand);
        }
        else if (strcmp(strupcase(token), "TXSTATE") == 0)
        {
            ParseChangeRadioTxStateCommand(
                iotoken,
                next,
                &time,
                &txStateCommand);

            AddCommandToList(time, (Testfed_Command*) txStateCommand);
        }
        else if (strcmp(strupcase(token), "SENDCOMMREQUEST") == 0)
        {
            ParseSendCommRequestCommand(
                iotoken,
                next,
                &time,
                &cerCommand);

            AddCommandToList(time, (Testfed_Command*) cerCommand);
        }
        else if (strcmp(strupcase(token), "VELOCITY") == 0)
        {
            ParseChangeEntityVelocityCommand(
                iotoken,
                next,
                &time,
                &velocityCommand);
            
            AddCommandToList(time, (Testfed_Command*) velocityCommand);
        }
        else
        {
            sprintf(err, "Unknown command type \"%s\"", token);
            ReportError(err);
        }
    }

    fclose(f);

    cout << "------- Command List for " << m_scenarioName << ".testfed -------\n";
    //print out the command List
    for (unsigned i = 0; i < cmdList.size(); i++)
    {   
        char timeStr[65];
        TIME_PrintClockInSecond(cmdList[i]->time, timeStr);
        cmdList[i]->message->Print(timeStr);
    }
    cout << endl << endl;
}

// /**
// FUNCTION   :: AddCommandToList
// PURPOSE    :: The function is used to add a testfed command to the cmd list.
// PARAMETERS ::  clocktype : time: time of command
//                Testfed_Command* : msg: pointer to the command object
// RETURN     ::  Void : NULL
// **/
void TestfedData::AddCommandToList(clocktype time,
        Testfed_Command *msg)
{
    Testfed_CommandList *newNode;

    // Create the new node
    newNode = new Testfed_CommandList();
    newNode->time = time;
    newNode->message = msg;

    cmdList.push_back(newNode);
    push_heap(cmdList.begin(), cmdList.end(), Comp);
}


// /**
// FUNCTION   :: RemoveFirstNode
// PURPOSE    :: The function is used to remove the top
//               or the first message from the cmd list.
// PARAMETERS ::
// RETURN     ::  Void : NULL
// **/
Testfed_CommandList* TestfedData::RemoveFirstNode()
{
    Testfed_CommandList *temp;

    pop_heap(cmdList.begin(), cmdList.end(), Comp);
    temp = cmdList.back();
    cmdList.pop_back();
    return temp;
}

// /**
// FUNCTION   :: ParseRegisterCommand
// PURPOSE    :: Parse the register objects command line from the .testfed file
// PARAMETERS :: char* : iotoken :  currently parsed token from line
//               char* : next : remaining input from line
//               clocktype*: time: output variable for time of command
//               Testfed_RegisterObjectsCommand**: message: output variable for msg
// RETURN     ::  void : NULL
// **/
void TestfedData::ParseRegisterCommand(char* iotoken,
        char* next,
        clocktype* time,
        Testfed_RegisterObjectsCommand** message)
{
    clocktype requestTime = 0;

    Testfed_RegisterObjectsCommand* registerCommand;
    registerCommand = new Testfed_RegisterObjectsCommand();

    // get the time
    IO_GetToken(iotoken, next, &next);
    if (iotoken == NULL)
    {
        ReportError("Missing time argument for Register command", __FILE__, __LINE__);
    }
    else
    {
        // we Have timeStamps
        requestTime = TIME_ConvertToClock(iotoken);
        *time = requestTime;
    }

    *message = registerCommand;
}

// /**
// FUNCTION   :: ParseQuitCommand
// PURPOSE    :: Parse the quit command line from the .testfed file
// PARAMETERS :: char* : iotoken :  currently parsed token from line
//               char* : next : remaining input from line
//               clocktype*: time: output variable for time of command
//               Testfed_QuitCommand**: message: output variable for msg
// RETURN     ::  void : NULL
// **/
void TestfedData::ParseQuitCommand(char* iotoken,
        char* next,
        clocktype* time,
        Testfed_QuitCommand** message)
{
    clocktype requestTime = 0;

    Testfed_QuitCommand* quitCommand;
    quitCommand = new Testfed_QuitCommand();

    // get the time
    IO_GetToken(iotoken, next, &next);
    if (iotoken == NULL)
    {
        ReportError("Missing time argument for Register command", __FILE__, __LINE__);
    }
    else
    {
        // we Have timeStamps
        requestTime = TIME_ConvertToClock(iotoken);
        *time = requestTime;
    }

    *message = quitCommand;
}

// FUNCTION   :: ParseMoveEntityCommand
// PURPOSE    :: Parse the mvoe entity command line from the .testfed file
// PARAMETERS :: char* : iotoken :  currently parsed token from line
//               char* : next : remaining input from line
//               clocktype*: time: output variable for time of command
//               Testfed_MoveEntityCommand**: message: output variable for msg
// RETURN     ::  void : NULL
// **/
void TestfedData::ParseMoveEntityCommand(char* iotoken,
    char* next,
    clocktype* time,
    Testfed_MoveEntityCommand** message)
{
    clocktype requestTime = 0;

    Testfed_MoveEntityCommand* moveCommand;
    moveCommand = new Testfed_MoveEntityCommand();

    // get the time
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);
    if (iotoken == NULL)
    {
        ReportError("Missing time argument for Register command", __FILE__, __LINE__);
    }
    else
    {
        // we Have timeStamps
        requestTime = TIME_ConvertToClock(iotoken);
        *time = requestTime;
    }

    iotoken[0] = 0;
    IO_GetToken(iotoken, next, &next);

    char* endPtr = NULL;
    errno = 0;
    moveCommand->nodeId = (unsigned) strtoul(iotoken, &endPtr, 10);

    if (iotoken[0] == 0)
    {
        ReportError("Missing node id argument for move command", __FILE__, __LINE__);
    }

    if (endPtr == iotoken || errno != 0 || moveCommand->nodeId < 1)
    {
        ReportError("Bad node id argument for move command", __FILE__, __LINE__);
    }

    iotoken[0] = 0;
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);

    if (iotoken[0] == 0)
    {
        ReportError("Missing latitude for move command", __FILE__, __LINE__);
    }

    errno = 0;
    moveCommand->lat = strtod(iotoken, &endPtr);

    if (endPtr == iotoken || errno != 0 || moveCommand->lat < -90.0 || moveCommand->lat > 90.0)
    {
        ReportError("Bad latitude for move command", __FILE__, __LINE__);
    }

    iotoken[0] = 0;
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);

    if (iotoken[0] == 0)
    {
        ReportError("Missing longitude for move command", __FILE__, __LINE__);
    }

    errno = 0;
    moveCommand->lon = strtod(iotoken, &endPtr);

    if (endPtr == iotoken || errno != 0 || moveCommand->lon < -180.0 || moveCommand->lon > 180.0)
    {
        ReportError("Bad longitude for move command", __FILE__, __LINE__);
    }

    iotoken[0] = 0;
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);

    if (iotoken[0] == 0)
    {
        ReportError("Missing altitude for move command", __FILE__, __LINE__);
    }

    errno = 0;
    moveCommand->alt = strtod(iotoken, &endPtr);

    if (endPtr == iotoken || errno != 0)
    {
        ReportError("Bad altitude for move command", __FILE__, __LINE__);
    }

    *message = moveCommand;
}

// /**
// FUNCTION   :: ParseChangeEntityOrientationCommand
// PURPOSE    :: Parse the change entity orientaiton command line from
//               the .testfed file
// PARAMETERS :: char* : iotoken :  currently parsed token from line
//               char* : next : remaining input from line
//               clocktype*: time: output variable for time of command
//               Testfed_ChangeEntityOrientationCommand**: message: 
//                      output variable for msg
// RETURN     ::  void : NULL
// **/
void TestfedData::ParseChangeEntityOrientationCommand(char* iotoken,
    char* next,
    clocktype* time,
    Testfed_ChangeEntityOrientationCommand** message)
{
    clocktype requestTime = 0;

    Testfed_ChangeEntityOrientationCommand* orientationCommand;
    orientationCommand = new Testfed_ChangeEntityOrientationCommand();

    // get the time
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);
    if (iotoken == NULL)
    {
        ReportError("Missing time argument for Register command", __FILE__, __LINE__);
    }
    else
    {
        // we Have timeStamps
        requestTime = TIME_ConvertToClock(iotoken);
        *time = requestTime;
    }

    iotoken[0] = 0;
    IO_GetToken(iotoken, next, &next);

    char* endPtr = NULL;
    errno = 0;
    orientationCommand->nodeId = (unsigned) strtoul(iotoken, &endPtr, 10);

    if (iotoken[0] == 0)
    {
        ReportError("Missing node id argument for change orientation command", __FILE__, __LINE__);
    }

    if (endPtr == iotoken || errno != 0 || orientationCommand->nodeId < 1)
    {
        ReportError("Bad node id argument for change orientation command", __FILE__, __LINE__);
    }

    iotoken[0] = 0;
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);

    if (iotoken[0] == 0)
    {
        ReportError("Missing psi for change orientation command", __FILE__, __LINE__);
    }

    errno = 0;
    double angleInDegrees = strtod(iotoken, &endPtr);

    if (endPtr == iotoken || errno != 0 || angleInDegrees < -180.0 || angleInDegrees > 180.0)
    {
        ReportError("Bad psi for change orientation command", __FILE__, __LINE__);
    }

    // IMPORTANT: DtEntityStateRepository::setOrientation(...) expects all TaitBryan angles to be represented in RADIANS.
    orientationCommand->psi = angleInDegrees * s_convertDegreesToRadians;

    iotoken[0] = 0;
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);

    if (iotoken[0] == 0)
    {
        ReportError("Missing theta for change orientation command", __FILE__, __LINE__);
    }

    errno = 0;
    angleInDegrees = strtod(iotoken, &endPtr);

    if (endPtr == iotoken || errno != 0 || angleInDegrees < -180.0 || angleInDegrees > 180.0)
    {
        ReportError("Bad theta for change orientation command", __FILE__, __LINE__);
    }

    // IMPORTANT: DtEntityStateRepository::setOrientation(...) expects all TaitBryan angles to be represented in RADIANS.
    orientationCommand->theta = angleInDegrees * s_convertDegreesToRadians;

    iotoken[0] = 0;
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);

    if (iotoken[0] == 0)
    {
        ReportError("Missing phi for change orientation command", __FILE__, __LINE__);
    }

    errno = 0;
    angleInDegrees = strtod(iotoken, &endPtr);

    if (endPtr == iotoken || errno != 0 || angleInDegrees < -180.0 || angleInDegrees > 180.0)
    {
        ReportError("Bad phi for change orientation command", __FILE__, __LINE__);
    }

    // IMPORTANT: DtEntityStateRepository::setOrientation(...) expects all TaitBryan angles to be represented in RADIANS.
    orientationCommand->phi = angleInDegrees * s_convertDegreesToRadians;

    *message = orientationCommand;
}

// /**
// FUNCTION   :: ParseChangeEntityDamageCommand
// PURPOSE    :: Parse the change entity damage state command line from
//               the .testfed file
// PARAMETERS :: char* : iotoken :  currently parsed token from line
//               char* : next : remaining input from line
//               clocktype*: time: output variable for time of command
//               Testfed_ChangeEntityDamageCommand**: message: output variable for msg
// RETURN     ::  void : NULL
// **/
void TestfedData::ParseChangeEntityDamageCommand(char* iotoken,
        char* next,
        clocktype* time,
        Testfed_ChangeEntityDamageCommand** message)
{
    clocktype requestTime = 0;

    Testfed_ChangeEntityDamageCommand* damageCommand;
    damageCommand = new Testfed_ChangeEntityDamageCommand();

    // get the time
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);
    if (iotoken == NULL)
    {
        ReportError("Missing time argument for Register command", __FILE__, __LINE__);
    }
    else
    {
        // we Have timeStamps
        requestTime = TIME_ConvertToClock(iotoken);
        *time = requestTime;
    }

    iotoken[0] = 0;
    IO_GetToken(iotoken, next, &next);

    char* endPtr = NULL;
    errno = 0;
    damageCommand->nodeId = (unsigned) strtoul(iotoken, &endPtr, 10);

    if (iotoken[0] == 0)
    {
        ReportError("Missing node id argument for change damage command", __FILE__, __LINE__);
    }

    if (endPtr == iotoken || errno != 0 || damageCommand->nodeId < 1)
    {
        ReportError("Bad node id argument for change damage command", __FILE__, __LINE__);
    }

    iotoken[0] = 0;
    IO_GetToken(iotoken, next, &next);

    errno = 0;
    damageCommand->damageState = (unsigned) strtoul(iotoken, &endPtr, 10);

    if (iotoken[0] == 0)
    {
        ReportError("Missing damageState argument for change damage command", __FILE__, __LINE__);
    }

    if (endPtr == iotoken || errno != 0 || damageCommand->damageState < 0 || damageCommand->damageState > 3)
    {
        ReportError("Bad node damageState for change damage command, valid values: 0-3", __FILE__, __LINE__);
    }

    *message = damageCommand;
}

// /**
// FUNCTION   :: ParseChangeRadioTxStateCommand
// PURPOSE    :: Parse the change radio tx state command line from
//               the .testfed file
// PARAMETERS :: char* : iotoken :  currently parsed token from line
//               char* : next : remaining input from line
//               clocktype*: time: output variable for time of command
//               Testfed_ChangeRadioTxStateCommand**: message: output variable for msg
// RETURN     ::  void : NULL
// **/
void TestfedData::ParseChangeRadioTxStateCommand(char* iotoken,
    char* next,
    clocktype* time,
    Testfed_ChangeRadioTxStateCommand** message)
{
    clocktype requestTime = 0;

    Testfed_ChangeRadioTxStateCommand* txStateCommand;
    txStateCommand = new Testfed_ChangeRadioTxStateCommand();

    // get the time
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);
    if (iotoken == NULL)
    {
        ReportError("Missing time argument for Register command", __FILE__, __LINE__);
    }
    else
    {
        // we Have timeStamps
        requestTime = TIME_ConvertToClock(iotoken);
        *time = requestTime;
    }

    iotoken[0] = 0;
    IO_GetToken(iotoken, next, &next);

    char* endPtr = NULL;
    errno = 0;
    txStateCommand->nodeId = (unsigned) strtoul(iotoken, &endPtr, 10);

    if (iotoken[0] == 0)
    {
        ReportError("Missing node id argument for change txState command", __FILE__, __LINE__);
    }

    if (endPtr == iotoken || errno != 0 || txStateCommand->nodeId < 1)
    {
        ReportError("Bad node id argument for change txState command", __FILE__, __LINE__);
    }

    iotoken[0] = 0;
    IO_GetToken(iotoken, next, &next);

    errno = 0;
    txStateCommand->txState = (unsigned) strtoul(iotoken, &endPtr, 10);

    if (iotoken[0] == 0)
    {
        ReportError("Missing txState argument for change txState command", __FILE__, __LINE__);
    }

    if (endPtr == iotoken || errno != 0 || txStateCommand->txState < 0 || txStateCommand->txState > 2)
    {
        ReportError("Bad node txState for change txState command: valid values: 0-2", __FILE__, __LINE__);
    }

    *message = txStateCommand;
}

// /**
// FUNCTION   :: ParseSendCommRequestCommand
// PURPOSE    :: Parse the send comm request command line from
//               the .testfed file
// PARAMETERS :: char* : iotoken :  currently parsed token from line
//               char* : next : remaining input from line
//               clocktype*: time: output variable for time of command
//               Testfed_SendCERCommand**: message: output variable for msg
// RETURN     ::  void : NULL
// **/
void TestfedData::ParseSendCommRequestCommand(char* iotoken,
    char* next,
    clocktype* time,
    Testfed_SendCERCommand** message)
{
    clocktype requestTime = 0;

    Testfed_SendCERCommand* cerCommand;
    cerCommand = new Testfed_SendCERCommand();

    // get the time
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);
    if (iotoken == NULL)
    {
        ReportError("Missing time argument for Register command", __FILE__, __LINE__);
    }
    else
    {
        // we Have timeStamps
        requestTime = TIME_ConvertToClock(iotoken);
        *time = requestTime;
    }

    iotoken[0] = 0;
    IO_GetToken(iotoken, next, &next);

    char* endPtr = NULL;
    errno = 0;
    cerCommand->nodeId = (unsigned) strtoul(iotoken, &endPtr, 10);

    if (iotoken[0] == 0)
    {
        ReportError("Missing src node id argument for send comm request command", __FILE__, __LINE__);
    }

    if (endPtr == iotoken || errno != 0 || cerCommand->nodeId < 1)
    {
        ReportError("Bad src node id argument for send comm request command", __FILE__, __LINE__);
    }
    
    iotoken[0] = 0;
    IO_GetToken(iotoken, next, &next);

    if (iotoken[0] == 0)
    {
        //use defaults
        cerCommand->isData = true;
        cerCommand->size = 100;
    }
    else
    {
        if (isalpha(iotoken[0]))
        {
            if (toupper(iotoken[0]) != 'V' || iotoken[1] == 0)
            {
                ReportError("Bad msgSize for data (use DmsgSize format).", __FILE__, __LINE__);
            }

            cerCommand->isData = false;

            errno = 0;
            cerCommand->size = (unsigned) strtoul(&iotoken[1], &endPtr, 10);

            if (endPtr == &iotoken[1] || errno != 0)
            {
                ReportError("Bad msgSize for data (use [V]msgSize format).", __FILE__, __LINE__);
            }
        }
        else
        {
            cerCommand->isData = true;

            errno = 0;
            cerCommand->size = (unsigned) strtoul(iotoken, &endPtr, 10);

            if (endPtr == iotoken || errno != 0)
            {
                ReportError("Bad msgSize.", __FILE__, __LINE__);
            }
        }
    }

    iotoken[0] = 0;
    IO_GetToken(iotoken, next, &next);

    if (iotoken[0] == 0)
    {
        //use default
        cerCommand->timeoutDelay = 10.0;
    }
    else
    {
        errno = 0;
        cerCommand->timeoutDelay = strtod(iotoken, &endPtr);

        if (endPtr == iotoken || errno != 0 || cerCommand->timeoutDelay < 0.0)
        {
            ReportError("Bad timeoutDelay.", __FILE__, __LINE__);
        }
    }

    // dstNodeId.

    iotoken[0] = 0;
    IO_GetToken(iotoken, next, &next);

    if (iotoken[0] == 0)
    {
        cerCommand->dstNodeIdPresent = false;
    }
    else
    {
        cerCommand->dstNodeIdPresent = true;

        errno = 0;
        cerCommand->dstNodeId = (unsigned) strtoul(iotoken, &endPtr, 10);

        if (endPtr == iotoken || errno != 0 || cerCommand->dstNodeId < 1)
        {
            ReportError("Bad dstNodeId.", __FILE__, __LINE__);
        }
    }

    *message = cerCommand;
}

// /**
// FUNCTION   :: ParseChangeEntityVelocityCommand
// PURPOSE    :: Parse the change entity velocity command line from
//               the .testfed file
// PARAMETERS :: char* : iotoken :  currently parsed token from line
//               char* : next : remaining input from line
//               clocktype*: time: output variable for time of command
//               Testfed_ChangeEntityVelocityCommand**: message: output variable for msg
// RETURN     ::  void : NULL
// **/
void TestfedData::ParseChangeEntityVelocityCommand(char* iotoken,
        char* next,
        clocktype* time,
        Testfed_ChangeEntityVelocityCommand** message)
{
    clocktype requestTime = 0;

    Testfed_ChangeEntityVelocityCommand* velocityCommand;
    velocityCommand = new Testfed_ChangeEntityVelocityCommand();

    // get the time
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);
    if (iotoken == NULL)
    {
        ReportError("Missing time argument for Register command", __FILE__, __LINE__);
    }
    else
    {
        // we Have timeStamps
        requestTime = TIME_ConvertToClock(iotoken);
        *time = requestTime;
    }

    iotoken[0] = 0;
    IO_GetToken(iotoken, next, &next);

    char* endPtr = NULL;
    errno = 0;
    velocityCommand->nodeId = (unsigned) strtoul(iotoken, &endPtr, 10);

    if (iotoken[0] == 0)
    {
        ReportError("Missing node id argument for velocity command", __FILE__, __LINE__);
    }

    if (endPtr == iotoken || errno != 0 || velocityCommand->nodeId < 1)
    {
        ReportError("Bad node id argument for move command", __FILE__, __LINE__);
    }

    iotoken[0] = 0;
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);

    if (iotoken[0] == 0)
    {
        ReportError("Missing x for velocity command", __FILE__, __LINE__);
    }

    errno = 0;
    velocityCommand->x = (float) strtod(iotoken, &endPtr);

    if (endPtr == iotoken || errno != 0)
    {
        ReportError("Bad x for velocity command", __FILE__, __LINE__);
    }

    iotoken[0] = 0;
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);

    if (iotoken[0] == 0)
    {
        ReportError("Missing y for velocity command", __FILE__, __LINE__);
    }

    errno = 0;
    velocityCommand->y = (float) strtod(iotoken, &endPtr);

    if (endPtr == iotoken || errno != 0)
    {
        ReportError("Bad y for velocity command", __FILE__, __LINE__);
    }

    iotoken[0] = 0;
    IO_GetDelimitedToken(iotoken, next, " ,()", &next);

    if (iotoken[0] == 0)
    {
        ReportError("Missing z for velocity command", __FILE__, __LINE__);
    }

    errno = 0;
    velocityCommand->z = (float) strtod(iotoken, &endPtr);

    if (endPtr == iotoken || errno != 0)
    {
        ReportError("Bad z for velocity command", __FILE__, __LINE__);
    }

    *message = velocityCommand;
}
// /**
// FUNCTION   :: ProcessCommandLineArguments
// PURPOSE    :: Process the command line arguments supplied to the app
// PARAMETERS :: unsigned : number of parameters
//               char* : actual parameters
// RETURN     ::  void : NULL
// **/
void TestfedData::ProcessCommandLineArguments(unsigned argc, char* argv[])
{
    m_debug = false;
    m_scenarioName[0] = 0;
       
    unsigned i;
    for (i = 1; i < argc; i++)
    {
        char* arg = argv[i];

        if (arg[0] == '-' || arg[0] == '/')
        {
            // This argument has a - or / as the first character.
            // Skip over this character and process the rest of the argument.

            arg++;

            if (arg[0] == 'h' || arg[0] == '?')
            {
                ShowUsage();
                exit(EXIT_SUCCESS);
            }
            else
            if (arg[0] == 'd')
            {
                m_debug = true;
            }
            else
            if (arg[0] == 'P')
            {
                i++;

                if (i == argc)
                {
                    ShowUsage();
                    exit(EXIT_FAILURE);
                }

                arg = argv[i];

                if (strcmp(arg, "DIS") == 0)
                {
                    protocol = VRLINK_TYPE_DIS;
                    strcpy(m_destinationAddress, "127.255.255.255");
                    strcpy(m_deviceAddress, "127.0.0.1");
                    strcpy(m_subnetMask, "");
                    m_portNumber = 3000;
                }
                else if (strcmp(arg, "HLA13") == 0)
                {
                    protocol = VRLINK_TYPE_HLA13;
                    m_rprFomVersion = 1.0;
                    strcpy(m_federationName, "VR-Link");
                    strcpy(m_federateName, "testfed");
                }
                else if (strcmp(arg, "HLA1516") == 0)
                {
                    protocol = VRLINK_TYPE_HLA1516;
                    m_rprFomVersion = 1.0;
                    strcpy(m_federationName, "VR-Link");
                    strcpy(m_federateName, "testfed");
                }
                else
                {
                    ReportError("Invalid protocol (DIS, HLA13 or HLA1516)");
                }
            }
            else
            if (arg[0] == 't')
            {
                i++;

                if (i == argc)
                {
                    ShowUsage();
                    exit(EXIT_FAILURE);
                }
                arg = argv[i];

                m_tickDelta = TIME_ConvertToClock(arg);

                Verify(m_tickDelta > 0, "Invalid value for vrlink tick delta");
            }
            else
            if (arg[0] == 'O')
            {
                i++;
                if (i == argc)
                {
                    ShowUsage();
                    exit(EXIT_FAILURE);
                }
                
                if (protocol == VRLINK_TYPE_DIS)
                {                                                        
                    arg = argv[i];

                    m_portNumber = atoi(arg);

                    Verify(m_portNumber > 0, "Invalid value for port number");
                }
                else
                {
                    ReportWarning("Ignoring -O, DIS specific argument");
                }

            }
            else
            if (arg[0] == 'A')
            {
                i++;

                if (i == argc)
                {
                    ShowUsage();
                    exit(EXIT_FAILURE);
                }

                if (protocol == VRLINK_TYPE_DIS)
                {
                    arg = argv[i];

                    Verify(
                        strlen(arg) + 1 <= sizeof(m_destinationAddress),
                        "Destination address too long");

                    strcpy(m_destinationAddress, arg);
                }
                else
                {
                    ReportWarning("Ignoring -A, DIS specific argument");
                }
            }
            else
            if (arg[0] == 'I')
            {
                i++;

                if (i == argc)
                {
                    ShowUsage();
                    exit(EXIT_FAILURE);
                }

                if (protocol == VRLINK_TYPE_DIS)
                {
                    arg = argv[i];

                    Verify(
                        strlen(arg) + 1 <= sizeof(m_deviceAddress),
                        "Device address too long");

                    strcpy(m_deviceAddress, arg);
                }
                else
                {
                    ReportWarning("Ignoring -I, DIS specific argument");
                }
            }
            else
            if (arg[0] == 'M')
            {
                i++;

                if (i == argc)
                {
                    ShowUsage();
                    exit(EXIT_FAILURE);
                }

                if (protocol == VRLINK_TYPE_DIS)
                {
                    arg = argv[i];

                    Verify(
                        strlen(arg) + 1 <= sizeof(m_subnetMask),
                        "Subnet Mask address too long");

                    strcpy(m_subnetMask, arg);
                }
                else
                {
                    ReportWarning("Ignoring -M, DIS specific argument");
                }
            }
            else
            if (arg[0] == 'f')
            {
                i++;

                if (i == argc)
                {
                    ShowUsage();
                    exit(EXIT_FAILURE);
                }

                if (protocol > VRLINK_TYPE_DIS)
                {
                    arg = argv[i];

                    Verify(
                        strlen(arg) + 1 <= sizeof(m_federationName),
                        "Federation name too long");

                    strcpy(m_federationName, arg);
                }
                else
                {
                    ReportWarning("Ignoring -f, HLA specific argument");
                }
            }
            else
            if (arg[0] == 'F')
            {
                i++; 
                if (i == argc)
                {
                    ShowUsage();
                    exit(EXIT_FAILURE);
                }

                if (protocol == VRLINK_TYPE_HLA13)
                {
                    arg = argv[i];

                    Verify(
                        strlen(arg) + 1 <= sizeof(m_fedFilePath),
                        "FED-file path too long");

                    strcpy(m_fedFilePath, arg);
                }
                else if (protocol == VRLINK_TYPE_HLA1516)
                {
                    arg = argv[i];

                    Verify(
                        strlen(arg) + 1 <= sizeof(m_fedFilePath),
                        "FDD-file path too long");

                    strcpy(m_fedFilePath, arg);
                }
                else
                {
                    ReportWarning("Ignoring -F, HLA specific argument");
                }
            }
            else
            if (arg[0] == 'r')
            {
                i++;

                if (i == argc)
                {
                    ShowUsage();
                    exit(EXIT_FAILURE);
                }

                if (protocol > VRLINK_TYPE_DIS)
                {
                    arg = argv[i];

                    Verify(
                        (strcmp(arg, "1.0") == 0) || (strcmp(arg, "2.0017") == 0),
                        "Invalid RPR Fom Version (1.0 or 2.0017 allowed)");

                    m_rprFomVersion = atof(arg);
                }
                else
                {
                    ReportWarning("Ignoring -r, HLA specific argument");
                }
            }
            else
            {
                ShowUsage();
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            Verify(
                strlen(arg) + 1 <= sizeof(m_scenarioName),
                "Scenario name too long");

            strcpy(m_scenarioName, arg);
        }//if//
    }//for//

    if ((m_scenarioName[0] == 0) ||
        ((protocol > VRLINK_TYPE_DIS) && (m_fedFilePath[0] == 0)))
    {
        ShowUsage();
        exit(EXIT_FAILURE);
    }

    //copy connection information to use for vrlink init call
    if (protocol == VRLINK_TYPE_DIS)
    {
        sprintf(m_testfedConnectionVariable1, "%d", m_portNumber);
        strcpy(m_testfedConnectionVariable2, m_destinationAddress);
        strcpy(m_testfedConnectionVariable3, m_deviceAddress);
        strcpy(m_testfedConnectionVariable4, m_subnetMask);
    }
    else
    {
        sprintf(m_testfedConnectionVariable1, "%g", m_rprFomVersion);
        strcpy(m_testfedConnectionVariable2, m_federationName);
        strcpy(m_testfedConnectionVariable3, m_federateName);
        strcpy(m_testfedConnectionVariable4, m_fedFilePath);
    }
}

// /**
// FUNCTION   :: ShowUsage
// PURPOSE    :: print out the config param options for testfed
// PARAMETERS :: 
// RETURN     ::  void : NULL
// **/
void ShowUsage()
{
    cout << "DIS/HLA13/HLA1516 test federate" << endl
         << endl
         << "Syntax:" << endl
         << endl
         << "    testfed [options] base-filename" << endl
         << endl
         << "    (Default values in parentheses)" << endl
         << "    -h" << endl
         << "\tHelp" << endl
         << "    -d" << endl
         << "\tDebug mode" << endl
         << "    -P" << endl
         << "\tProtocol (DIS/HLA13/HLA1516) REQUIRED" <<endl
         << "    -O port number" << endl
         << "\tFor DIS: Set port number (3000)" << endl
         << "    -A destination address" << endl
         << "\tFor DIS: Set destination address (127.255.255.255)" << endl
         << "    -I device address" << endl
         << "\tFor DIS: Set device address (127.0.0.1)" <<endl
         << "    -M subnet mask" << endl
         << "\tFor DIS: Set subnet mask for destination address" << endl         
         << "    -f federation-name" << endl
         << "\tFor HLA: Set federation name (VR-Link)" << endl
         << "    -F FED or FDD-file path" << endl
         << "\tFor HLA: Set FED or FDD file (VR-Link.fed) REQUIRED" << endl
         << "    -r rprFomVersion" << endl
         << "\tFor HLA: Set RPR FOM version(1.0)" << endl
         << "    -t tick delta" <<endl
         << "\tVR-Link Tick Delta" <<endl;
}

// /**
// FUNCTION   :: GetTickDelta
// PURPOSE    :: accessor for tick delta variable
// PARAMETERS :: 
// RETURN     ::  clocktype
// **/
clocktype TestfedData::GetTickDelta()
{
    return m_tickDelta;
}

// /**
// FUNCTION   :: GetProtocol
// PURPOSE    :: accessor for protocol type variable
// PARAMETERS :: 
// RETURN     ::  VRLinkInterfaceType
// **/
VRLinkInterfaceType TestfedData::GetProtocol()
{
    return protocol;
}

// /**
// FUNCTION   :: GetScenarioName
// PURPOSE    :: accessor for scenario name variable
// PARAMETERS :: 
// RETURN     ::  char*
// **/
char* TestfedData::GetScenarioName()
{
    return m_scenarioName;
}

// /**
// FUNCTION   :: GetVRLink
// PURPOSE    :: accessor for vrlink interface pointer
// PARAMETERS :: 
// RETURN     ::  VRLinkExternalInterface*
// **/
VRLinkExternalInterface* TestfedData::GetVRLink()
{
    return vrlink;
}

// /**
// FUNCTION   :: SetVRLink
// PURPOSE    :: set the point value for vrlink interface object
// PARAMETERS :: VRLinkExternalInterface* : desired object pointer
// RETURN     ::  void : NULL
// **/
void TestfedData::SetVRLink(VRLinkExternalInterface* obj)
{
    vrlink = obj;
}

// /**
// FUNCTION   :: IsRegistered
// PURPOSE    :: accessor for m_objectsRegistered
// PARAMETERS :: 
// RETURN     ::  bool
// **/
bool TestfedData::IsRegistered()
{
    return m_objectsRegistered;
}

// /**
// FUNCTION   :: SetRegisteredFlag
// PURPOSE    :: set the value for m_objectsRegistered
// PARAMETERS :: bool : desired value
// RETURN     ::  void : NULL
// **/
void TestfedData::SetRegisteredFlag(bool val)
{
    m_objectsRegistered = val;
}

// /**
// FUNCTION   :: GetConnectionVariable
// PURPOSE    :: get specific connection variable
// PARAMETERS :: int : variable number
// RETURN     ::  char* : variable value
// **/
char* TestfedData::GetConnectionVariable(int i)
{
    switch(i)
    {
        case 1:
            return m_testfedConnectionVariable1;
        case 2:
            return m_testfedConnectionVariable2;
        case 3:
            return m_testfedConnectionVariable3;
        case 4:
            return m_testfedConnectionVariable4;
        default:
            return "";
    }
}

// /**
// FUNCTION   :: GetCmdListSize
// PURPOSE    :: get current size of cmd List
// PARAMETERS ::
// RETURN     ::  unsigned
// **/
unsigned TestfedData::GetCmdListSize()
{
    return cmdList.size();
}

// /**
// FUNCTION   :: GetDebug
// PURPOSE    :: accessor function for debug setting
// PARAMETERS ::
// RETURN     ::  bool
// **/
bool TestfedData::GetDebug()
{
    return m_debug;
}

// /**
// FUNCTION   :: Print
// PURPOSE    :: print the command to cout
// PARAMETERS :: char*: time : execution time of command
// RETURN     ::  void : NULL
// **/
void Testfed_SendCERCommand::Print(char* time)
{
    cout << time << " Send CER - srcNode:" << nodeId << ", msgSize:" << size;

    if (isData)
    {
        cout << "bytes";
    }
    else
    {
        cout << "sec";
    }
    cout << ", timeout:" << timeoutDelay;
    if (dstNodeIdPresent)
    {
        cout << ", destNodeId:" << dstNodeId;
    }        
    cout << endl;
}

// /**
// FUNCTION   :: Print
// PURPOSE    :: print the command to cout
// PARAMETERS :: char*: time : execution time of command
// RETURN     ::  void : NULL
// **/
void Testfed_ChangeRadioTxStateCommand::Print(char* time)
{
    cout << time << " Change Radio Tx State - node:" << nodeId << " to "
        << txState << endl;   
}

// /**
// FUNCTION   :: Print
// PURPOSE    :: print the command to cout
// PARAMETERS :: char*: time : execution time of command
// RETURN     ::  void : NULL
// **/
void Testfed_ChangeEntityDamageCommand::Print(char* time)
{
    cout << time << " Change Entity Damage State - node:" << nodeId << " to "
        << damageState << endl;    
}

// /**
// FUNCTION   :: Print
// PURPOSE    :: print the command to cout
// PARAMETERS :: char*: time : execution time of command
// RETURN     ::  void : NULL
// **/
void Testfed_ChangeEntityOrientationCommand::Print(char* time)
{
    cout << time << " Change Entity Orientation - node:" << nodeId << " to (";
    cout << psi << "," << theta << "," << phi << ")\n";
}

// /**
// FUNCTION   :: Print
// PURPOSE    :: print the command to cout
// PARAMETERS :: char*: time : execution time of command
// RETURN     ::  void : NULL
// **/
void Testfed_MoveEntityCommand::Print(char* time)
{
    cout << time << " Move Entity - node:" << nodeId << " to (";
    cout << lat << "," << lon << "," << alt << ")\n";
}

// /**
// FUNCTION   :: Print
// PURPOSE    :: print the command to cout
// PARAMETERS :: char*: time : execution time of command
// RETURN     ::  void : NULL
// **/
void Testfed_ChangeEntityVelocityCommand::Print(char* time)
{
    cout << time << " Change Entity Velocity - node:" << nodeId << " to (";
    cout << x << "," << y << "," << z << ")\n";
}

// /**
// FUNCTION   :: Print
// PURPOSE    :: print the command to cout
// PARAMETERS :: char*: time : execution time of command
// RETURN     ::  void : NULL
// **/
void Testfed_QuitCommand::Print(char* time)
{
    cout << time << " Quit\n";
}

// /**
// FUNCTION   :: Print
// PURPOSE    :: print the command to cout
// PARAMETERS :: char*: time : execution time of command
// RETURN     ::  void : NULL
// **/
void Testfed_RegisterObjectsCommand::Print(char* time)
{
    cout << time << " Register\n";
}
