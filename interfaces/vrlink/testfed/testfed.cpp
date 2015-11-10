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

#ifdef NOT_RTI_NG
#include <iostream.h>
#else /* NOT_RTI_NG */
#include <iostream>
using namespace std;
#endif /* NOT_RTI_NG */

#include "fileio.h"

#include "testfed_shared.h"
#include "testfed_data.h"
#include "vrlink.h"

void EnterCommandLoop()
{
    clocktype simTime = 0;

    Testfed_CommandList* curCmd = g_testfedData->RemoveFirstNode();

    if (curCmd == NULL)
    {
        ReportWarning("No commands in the testfed command list\n");
        return;
    }

    while (1)
    {
        //if any entities velocity is non zero, update the position
        g_testfedData->GetVRLink()->IncrementEntitiesPositionIfVelocityIsSet(
            g_testfedData->GetTickDelta());

        //always set the sim time to that of the loop
        g_testfedData->GetVRLink()->SetSimTime(simTime);

        //receive from other federates/exercise participants
        g_testfedData->GetVRLink()->Receive();
        g_testfedData->GetVRLink()->Tick();
        
        while (curCmd->time <= simTime)
        {
            char timeStr[65];
            TIME_PrintClockInSecond(curCmd->time, timeStr);

            if (curCmd->message->GetType() == Testfed_CommandType_RegisterObjects)
            {
                if (!g_testfedData->IsRegistered())
                {
                    g_testfedData->GetVRLink()->PublishObjects();                
                    g_testfedData->SetRegisteredFlag(true);
                }
                else
                {
                    ReportWarning("Objects already registered, ignoring command.\n");
                }
            }
            else
            if (curCmd->message->GetType() == Testfed_CommandType_MoveEntity)
            {                
                if (!g_testfedData->IsRegistered())
                {
                    ReportWarning("Objects need to be registered first, skipping command\n");
                }
                else
                {
                    Testfed_MoveEntityCommand* tmp =
                        (Testfed_MoveEntityCommand*)curCmd->message;
                    cout << "Time: " << timeStr << " - ";
                    g_testfedData->GetVRLink()->MoveEntity(
                        tmp->nodeId, tmp->lat, tmp->lon, tmp->alt);
                }
            }
            else
            if (curCmd->message->GetType() ==
                Testfed_CommandType_ChangeEntityOrientation)
            {                
                if (!g_testfedData->IsRegistered())
                {
                    ReportWarning("Objects need to be registered first, skipping command\n");
                }
                else
                {
                    Testfed_ChangeEntityOrientationCommand* tmp =
                        (Testfed_ChangeEntityOrientationCommand*)curCmd->message;
                    cout << "Time: " << timeStr << " - ";

                    // IMPORTANT:
                    // DtEntityStateRepository::setOrientation(...) expects
                    // all TaitBryan angles to be represented in RADIANS.
                    g_testfedData->GetVRLink()->ChangeEntityOrientation(tmp->nodeId,
                        tmp->psi,
                        tmp->theta,
                        tmp->phi);
                }
            }
            else
            if (curCmd->message->GetType() == Testfed_CommandType_SendCER)
            {                
                if (!g_testfedData->IsRegistered())
                {
                    ReportWarning("Objects need to be registered first, skipping command\n");
                }
                else
                {
                    Testfed_SendCERCommand* tmp =
                        (Testfed_SendCERCommand*)curCmd->message;
                    cout << "Time: " << timeStr << " - ";
                    g_testfedData->GetVRLink()->SendCommEffectsRequest(tmp->nodeId,
                        tmp->isData,
                        tmp->size,
                        tmp->timeoutDelay,
                        tmp->dstNodeIdPresent,
                        tmp->dstNodeId);
                }
            }
            else
            if (curCmd->message->GetType() ==
                Testfed_CommandType_ChangeEntityDamage)
            {
                
                if (!g_testfedData->IsRegistered())
                {
                    ReportWarning("Objects need to be registered first, skipping command\n");
                }
                else
                {
                    Testfed_ChangeEntityDamageCommand* tmp =
                        (Testfed_ChangeEntityDamageCommand*)curCmd->message;
                    cout << "Time: " << timeStr << " - ";
                    g_testfedData->GetVRLink()->ChangeDamageState(
                        tmp->nodeId, tmp->damageState);
                }
            }
            else
            if (curCmd->message->GetType() == Testfed_CommandType_ChangeRadioTxState)
            {                
                if (!g_testfedData->IsRegistered())
                {
                    ReportWarning("Objects need to be registered first, skipping command\n");
                }
                else
                {
                    Testfed_ChangeRadioTxStateCommand* tmp =
                        (Testfed_ChangeRadioTxStateCommand*)curCmd->message;
                    cout << "Time: " << timeStr << " - ";
                    g_testfedData->GetVRLink()->ChangeTxOperationalStatus(
                        tmp->nodeId, tmp->txState);
                }
            }
            else
            if (curCmd->message->GetType() == Testfed_CommandType_Quit)
            {
                return;
            }
            else
            if (curCmd->message->GetType() ==
                Testfed_CommandType_ChangeEntityVelocity)
            {
                if (!g_testfedData->IsRegistered())
                {
                    ReportWarning("Objects need to be registered first, skipping command\n");
                }
                else
                {
                    Testfed_ChangeEntityVelocityCommand* tmp =
                        (Testfed_ChangeEntityVelocityCommand*)curCmd->message;
                    cout << "Time: " << timeStr << " - ";
                    g_testfedData->GetVRLink()->ChangeEntityVelocity(
                        tmp->nodeId, tmp->x, tmp->y, tmp->z);
                }
            }
            if (g_testfedData->GetCmdListSize() > 0)
            {
                curCmd = g_testfedData->RemoveFirstNode();
            }
            else
            {
                //no more commands in the cmd list
                ReportWarning("No more pending command and Quit command was not found, exiting now\n");
                simTime+=g_testfedData->GetTickDelta();
                g_testfedData->GetVRLink()->IterationSleepOperation(simTime);
                return;
            }
        }
                
        simTime+=g_testfedData->GetTickDelta();
        g_testfedData->GetVRLink()->IterationSleepOperation(simTime);
    }//while//
}

int main(int argc, char* argv[])
{   
    //validate constants
    assert(sizeof(bool) == 1);
    assert(sizeof(char) == 1);
    assert(sizeof(short) == 2);
    assert(sizeof(int) == 4);
    assert(sizeof(double) == 8);

    CheckNoMalloc(g_testfedData, __FILE__, __LINE__);
    g_testfedData = new TestfedData;
    CheckMalloc(g_testfedData, __FILE__, __LINE__);

    g_testfedData->ProcessCommandLineArguments(argc, argv);
    g_testfedData->ReadTestfedCommandFile();
           
    g_testfedData->SetVRLink(
        VRLinkInit(g_testfedData->GetDebug(),
            g_testfedData->GetProtocol(),
            g_testfedData->GetScenarioName(),
            g_testfedData->GetConnectionVariable(1),
            g_testfedData->GetConnectionVariable(2),
            g_testfedData->GetConnectionVariable(3),
            g_testfedData->GetConnectionVariable(4)
        )
    );

    EnterCommandLoop();

    g_testfedData->GetVRLink()->Finalize();
    delete g_testfedData;
    g_testfedData = NULL;

    return 0;
}
