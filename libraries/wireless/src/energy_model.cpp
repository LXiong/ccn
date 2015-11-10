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
// PACKAGE     :: PHYSICAL LAYER
// DESCRIPTION :: This file describes data structures and functions used by the Physical Layer.
//                Most of this functionality is enabled/used in the Wireless library.
// **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "api.h"

#define ENERGY_DEBUG 0


// /**
// FUNCTION: Phy_ReportStatusToEnergyModel
// LAYER    : PHYSICAL
// PURPOSE: This function should be called whenever a state transition occurs
//    in any place in PHY layer. As input parameters, the function reads the current
//    state and the new state of PHY layer and based on the new sates calculates the cost
//    of the load that should be taken off the battery.
//    The function then interacts with battery model and updates the charge of battery.
// PARAMETERS:
//    +node: Node*: The node received message
//    +phyIndex: index of the interface running this PHY layer
//    +prevStatus:the state from which PHY is exiting
//    +newStatus: int:the state to which PHY is entering
// RETURN   : None
// **/

void
Phy_ReportStatusToEnergyModel(
                  Node* node,
                  const int phyIndex,
                  PhyStatusType prevStatus,
                  PhyStatusType newStatus )

{
  double duration, actDuration, load;
  PhyData* thisPhy = node->phyData[phyIndex];
  clocktype Now = node->getNodeTime();

  if (thisPhy->eType == NO_ENERGY_MODEL ){ return; }



  duration = (double)( Now - thisPhy->curLoad->lastUpdate ) / (double)SECOND;
  load = thisPhy->curLoad->load;
  actDuration = (double)( Now - thisPhy->curLoad->startTime ) / (double)SECOND;;

  if (!node->battery->dead )
    BatteryDecCharge(
             node,
             duration,
             load);
  else
    {
      Now = node->battery->deadTime ;
      actDuration = (double)(Now - thisPhy->curLoad->startTime) / (double)SECOND;
      if (actDuration < 0 )
    return;
    }

  if (thisPhy->eType == GENERIC_ENERGY_MODEL )
    Generic_UpdateCurrentLoad(
                  node,
                  phyIndex);


  switch( prevStatus )
    {
    case PHY_SUCCESS:
    case PHY_IDLE:
      {
    thisPhy->curLoad->powStats.totalIdlePower += (load*actDuration );
    thisPhy->curLoad->powStats.totalIdleDuration +=
      (clocktype )( Now - thisPhy->curLoad->startTime );
    if (ENERGY_DEBUG ){
      printf("Node %d:Total Idle charge consumed: %e \n",
         node->nodeId,
         (thisPhy->curLoad->powStats.totalIdlePower/3600.0));
    }
    break;
      }

    case PHY_BUSY_TX:
    case PHY_TRANSMITTING:
      {
    thisPhy->curLoad->powStats.totalTxPower +=
      ( load * actDuration );
    thisPhy->curLoad->powStats.totalTxDuration +=
      (clocktype )( Now - thisPhy->curLoad->startTime );
    if (ENERGY_DEBUG )
      {
        printf("Node %d:Total Transmit charge consumed: %e \n",
           node->nodeId,
           (thisPhy->curLoad->powStats.totalTxPower/3600.0));
      }
    break;
      }

    case PHY_BUSY_RX:
    case PHY_SENSING:
    case PHY_RECEIVING:
      {
    thisPhy->curLoad->powStats.totalRxPower +=
      ( load * actDuration );
    thisPhy->curLoad->powStats.totalRxDuration +=
      (clocktype)( Now - thisPhy->curLoad->startTime );
    if (ENERGY_DEBUG )
      {
        printf("Node %d:Total Receive charge consumed:%e\n",
           node->nodeId,(thisPhy->curLoad->powStats.totalRxPower/3600.0));
      }
    break;
      }

    case PHY_TRX_OFF:
      {
    thisPhy->curLoad->powStats.totalSleepPower +=
      (load * actDuration  );
    thisPhy->curLoad->powStats.totalSleepDuration += (clocktype )
      ( Now - thisPhy->curLoad->startTime );
    if (ENERGY_DEBUG )
      {
        printf("Node %d: Total Sleep duration:%e\n",
           node->nodeId,(double)(thisPhy->curLoad->powStats.totalSleepDuration/SECOND));
      }
    break;
      }
    }//switch( prevStatus )

  switch(newStatus)
    {
    case PHY_SUCCESS:
    case PHY_IDLE:
      {
    thisPhy->curLoad->load =  thisPhy->powerConsmpTable->idle_current_load;
    thisPhy->curLoad->startTime = node->getNodeTime();
    thisPhy->curLoad->lastUpdate = node->getNodeTime();
    break;
      }

    case PHY_BUSY_TX:
    case PHY_TRANSMITTING:
      {
    thisPhy->curLoad->load =  thisPhy->powerConsmpTable->trx_current_load;
    thisPhy->curLoad->startTime = node->getNodeTime();
    thisPhy->curLoad->lastUpdate = node->getNodeTime();
    break;
      }

    case PHY_BUSY_RX:
    case PHY_SENSING:
    case PHY_RECEIVING:
      {
    thisPhy->curLoad->load =  thisPhy->powerConsmpTable->rcv_current_load;
    thisPhy->curLoad->startTime = node->getNodeTime();
    thisPhy->curLoad->lastUpdate = node->getNodeTime();
    break;
      }

    case PHY_TRX_OFF:
      {
    thisPhy->curLoad->load = thisPhy->powerConsmpTable->sleep_current_load;
    thisPhy->curLoad->startTime = node->getNodeTime();
    thisPhy->curLoad->lastUpdate = node->getNodeTime();
    break;
      }
    } //switch(newStatus)

  if (node->guiOption)
  {
      GUI_SendRealData(node->nodeId,
                       thisPhy->curLoad->RuntimeId,
                       thisPhy->curLoad->load,
                       node->getNodeTime());
  }

}
// /**
// FUNCTION: ENERGY_Init
// LAYER    : PHYSICAL
// PURPOSE:  This function declares energy model variables and initializes them.
//        Moreover, the function read energy model specifications and configures
//        the parameters which are configurable.
// PARAMETERS:
//    +node: Node*: The node received message
//    +phyIndex: index of the interface running this PHY layer
//    +nodeInput:
// RETURN   : None
// **/

void
ENERGY_Init(
        Node *node,
            const int phyIndex,
            const NodeInput *nodeInput)
{
    PhyData*    thisPhy;
    int i;
    BOOL found = FALSE;
    char str[MAX_STRING_LENGTH];
    double txPower_dBm,txPower_mW;
    double load;

    thisPhy =  node->phyData[phyIndex];

    IO_ReadString(
          node->nodeId,
          thisPhy->networkAddress,
          nodeInput,
          "ENERGY-MODEL-SPECIFICATION",
          &found,
          str);
    if (!found || !strcmp(str, "NONE"))
    {
        thisPhy->eType = NO_ENERGY_MODEL;
        return;
    }

    if (ENERGY_DEBUG){
        printf("Node %d:Initiliazing energy model \n",
           node->nodeId);
    }


    thisPhy->curLoad = (LoadProfile*)
        MEM_malloc(sizeof(LoadProfile));
    thisPhy->curLoad->startTime = (clocktype) 0;
    thisPhy->curLoad->lastUpdate = (clocktype) 0;

    thisPhy->curLoad->load = 0.0;
    thisPhy->curLoad->powStats.totalIdlePower = 0.0;
    thisPhy->curLoad->powStats.totalSleepPower = 0.0;
    thisPhy->curLoad->powStats.totalTxPower = 0.0;
    thisPhy->curLoad->powStats.totalRxPower = 0.0;
    thisPhy->curLoad->powStats.totalSleepDuration = (clocktype) 0;
    thisPhy->curLoad->powStats.totalIdleDuration = (clocktype) 0;
    thisPhy->curLoad->powStats.totalRxDuration = (clocktype) 0;
    thisPhy->curLoad->powStats.totalTxDuration = (clocktype) 0;

    PowerCosts* loadTable = (PowerCosts*)
        MEM_malloc(sizeof(PowerCosts));

    thisPhy->eType = TECHNOLOGY_DEFINED_ENERGY_MODEL;

    loadTable->sleep_current_load = 0.0;
    loadTable->idle_current_load = 5.0;
    loadTable->rcv_current_load = 10.0;

    loadTable->trx_current_table = (float*)
        MEM_malloc((NUM_TRX_POWER_STATES)*sizeof(float));

    for (i = 0; i < NUM_TRX_POWER_STATES; i++){
        loadTable->trx_current_table[i] = 12.0;
    }


    if (!strcmp(str, "MICA-MOTES")){
        PHY_GetTransmitPower(
                 node,
                 phyIndex,
                 &txPower_mW);

        txPower_dBm = (double) 10.0 * (log(txPower_mW) / log(10.0));
        switch (RoundToInt(txPower_dBm))
            {
                case 10:
                {
                    loadTable->trx_current_load = 26.7f;
                    break;
                }
                case 5:
                {
                    loadTable->trx_current_load = 14.8f;
                    break;
                }
                case 0:
                {
                    loadTable->trx_current_load = 10.4f;
                    break;
                }
                case -5:
                {
                    loadTable->trx_current_load = 8.9f;
                    break;
                }
                case -20:
                {
                    loadTable->trx_current_load = 5.3f;
                    break;
                }
                default:
                {
                    loadTable->trx_current_load =
                         float ((txPower_mW -1.0)* 1.14 + 10.4);
                }
            }

        loadTable->rcv_current_load = 9.6f;
        loadTable->sleep_current_load = 0.03f;
        loadTable->idle_current_load = 5.0;
        loadTable->voltage = 3.0;

    }else if (!found || !strcmp(str, "MICAZ") ){

        PHY_GetTransmitPower(
                 node,
                 phyIndex,
                 &txPower_mW);

        txPower_dBm = (double) 10.0 * ( log(txPower_mW) / log(10.0));

        switch (RoundToInt(txPower_dBm))
            {
                case 0:
                {
                    loadTable->trx_current_load = 16.0;
                    break;
                }
                case -1:
                {
                    loadTable->trx_current_load = 15.0;
                    break;
                }
                case -3:
                {
                    loadTable->trx_current_load = 14.0;
                    break;
                }
                case -5:
                {
                    loadTable->trx_current_load = 13.0;
                    break;
                }
                case -7:
                {
                    loadTable->trx_current_load = 12.0;
                    break;
                }
                case -10:
                {
                    loadTable->trx_current_load = 11.0;
                    break;
                }
                case -15:
                {
                    loadTable->trx_current_load = 8.8f;
                    break;
                }
                case -25:
                {
                    break;
                }

                default:
                {
                    loadTable->trx_current_load =
                           (float)((txPower_mW -0.1) * 5.56 + 11.0);
                    break;
                }
            }//switch( (int)txPower_dBm )

        loadTable->sleep_current_load = 0.0;
        loadTable->idle_current_load = (float)10.79/3;//mA
        loadTable->rcv_current_load = (float)56.5/3;//mA
        loadTable->voltage = 3.0;

    }else if (!strcmp(str, "USER-DEFINED")){
        thisPhy->eType = USER_DEFINED_ENERGY_MODEL;

        IO_ReadDouble(
              node->nodeId,
              thisPhy->networkAddress,
              nodeInput,
              "ENERGY-TX-CURRENT-LOAD",
              &found,
              &load);
        if (!found) {
            loadTable->trx_current_load = DEFAULT_TRX_CURRENT_LOAD;
        } else {
            loadTable->trx_current_load = (float ) load;
        }


        IO_ReadDouble(
              node->nodeId,
              thisPhy->networkAddress,
              nodeInput,
              "ENERGY-RX-CURRENT-LOAD",
              &found,
              &load);

        if (!found) {
            loadTable->rcv_current_load = DEFAULT_RCV_CURRENT_LOAD;
        } else {
                loadTable->rcv_current_load = (float ) load;
        }

        IO_ReadDouble(
              node->nodeId,
              thisPhy->networkAddress,
              nodeInput,
              "ENERGY-IDLE-CURRENT-LOAD",
              &found,
              &load);

        if (!found) {
            loadTable->idle_current_load = DEFAULT_IDLE_CURRENT_LOAD;
        } else {
            loadTable->idle_current_load = (float ) load;
        }

        IO_ReadDouble(
              node->nodeId,
              thisPhy->networkAddress,
              nodeInput,
              "ENERGY-SLEEP-CURRENT-LOAD",
              &found,
              &load);

        if (!found) {
            loadTable->sleep_current_load = DEFAULT_SLEEP_CURRENT_LOAD;
        } else {
            loadTable->sleep_current_load =(float ) load;
        }

        IO_ReadDouble(
              node->nodeId,
              thisPhy->networkAddress,
              nodeInput,
              "ENERGY-OPERATIONAL-VOLTAGE",
              &found,
              &load);

        if (!found) {
            loadTable->voltage = DEFAULT_OPT_VOLTAGE;
        } else {
            loadTable->voltage =(float)load;
        }
    } else if (!strcmp(str, "GENERIC")) {

        //Generic Energy Model

        thisPhy->eType = GENERIC_ENERGY_MODEL;
        IO_ReadDouble(
              node->nodeId,
              thisPhy->networkAddress,
              nodeInput,
              "ENERGY-POWER-AMPLIFIER-INEFFICIENCY-FACTOR",
              &found,
              &load);

        if (!found) {
            thisPhy->genericEnergyModelParameters.alpha_amp = DEFAULT_ALPHA_AMP;
        } else {
        thisPhy->genericEnergyModelParameters.alpha_amp = (float ) load;
        }

        IO_ReadDouble(
              node->nodeId,
              thisPhy->networkAddress,
              nodeInput,
              "ENERGY-TRANSMIT-CIRCUITRY-POWER-CONSUMPTION",
              &found,
              &load);

        if (!found) {
            thisPhy->genericEnergyModelParameters.Pct = DEFAULT_PCT;
        } else {
            thisPhy->genericEnergyModelParameters.Pct = (float ) load;
        }

        IO_ReadDouble(
              node->nodeId,
              thisPhy->networkAddress,
              nodeInput,
              "ENERGY-RECEIVE-CIRCUITRY-POWER-CONSUMPTION",
              &found,
              &load);
        if (!found) {
            thisPhy->genericEnergyModelParameters.Pcr = DEFAULT_PCR;
        } else {
            thisPhy->genericEnergyModelParameters.Pcr = (float ) load;
        }

        IO_ReadDouble(
              node->nodeId,
              thisPhy->networkAddress,
              nodeInput,
              "ENERGY-SLEEP-CIRCUITRY-POWER-CONSUMPTION",
              &found,
              &load);
        if (!found) {
            thisPhy->genericEnergyModelParameters.Psp = DEFAULT_PSP;
        } else {
            thisPhy->genericEnergyModelParameters.Psp = (float)load;
        }

        IO_ReadDouble(
              node->nodeId,
              thisPhy->networkAddress,
              nodeInput,
              "ENERGY-IDLE-CIRCUITRY-POWER-CONSUMPTION",
              &found,
              &load);
        if (!found) {
            thisPhy->genericEnergyModelParameters.Pid = DEFAULT_PID;
        } else {
            thisPhy->genericEnergyModelParameters.Pid = (float)load;
        }

        IO_ReadDouble(
              node->nodeId,
              thisPhy->networkAddress,
              nodeInput,
              "ENERGY-SUPPLY-VOLTAGE",
              &found,
              &load);
        if (!found) {
            thisPhy->genericEnergyModelParameters.Vs = DEFAULT_VS;
        } else {
            thisPhy->genericEnergyModelParameters.Vs = (float)load;
        }

        thisPhy->powerConsmpTable = loadTable;
        thisPhy->powerConsmpTable->voltage =
                        (float)thisPhy->genericEnergyModelParameters.Vs;
                Generic_UpdateCurrentLoad(node, phyIndex);
        thisPhy->curLoad->load = thisPhy->powerConsmpTable->idle_current_load;
    } else {
        ERROR_ReportError("Unknown ENERGY-MODEL-SPECIFICATION type.");
    }
    thisPhy->powerConsmpTable = loadTable;
    thisPhy->curLoad->load =
        thisPhy->powerConsmpTable->idle_current_load;

    if (node->guiOption)
    {
        thisPhy->curLoad->RuntimeId =
            GUI_DefineMetric(
                "Energy Model: Electrical Load (mA)",
                node->nodeId,
                GUI_PHY_LAYER,
                phyIndex,
                GUI_DOUBLE_TYPE,
                GUI_CUMULATIVE_METRIC);
    }
}

// /**
// FUNCTION: Generic_UpdateCurrentLoad
// LAYER: PHYSICAL
// PURPOSE: To update the current load of generic energy model
// PARAMETERS:
//    +node: Node*: The node received message
//    +phyIndex: index of the interface running this PHY layer
// RETURN: None
// **/

void
Generic_UpdateCurrentLoad(
              Node *node,
              const int phyIndex)
{

  PhyData* thisPhy = node->phyData[phyIndex];
  double txPower_dBm,txPower_mW;

  PHY_GetTransmitPower(
               node,
               phyIndex,
               &txPower_mW);

  txPower_dBm =(double) 10.0 * ( log(txPower_mW) / log(10.0) );


  thisPhy->powerConsmpTable->trx_current_load =
    (float)((thisPhy->genericEnergyModelParameters.alpha_amp * txPower_mW)
    + (thisPhy->genericEnergyModelParameters.Pct /
       thisPhy->genericEnergyModelParameters.Vs));

  thisPhy->powerConsmpTable->rcv_current_load =
   (float)(thisPhy->genericEnergyModelParameters.Pcr / thisPhy->genericEnergyModelParameters.Vs);

  thisPhy->powerConsmpTable->idle_current_load =
    (float)(thisPhy->genericEnergyModelParameters.Pid / thisPhy->genericEnergyModelParameters.Vs);
  thisPhy->powerConsmpTable->sleep_current_load =
    (float)(thisPhy->genericEnergyModelParameters.Psp / thisPhy->genericEnergyModelParameters.Vs);
}


// /**
// FUNCTION: ENERGY_PrintStats
// LAYER:  PHYSICAL
// PURPOSE: To print the statistic of Energy Model
// PARAMETERS:
//    +node: Node*: The node received message
//    +phyIndex: index of the interface running this PHY layer
// RETURN: None
// **/

void
ENERGY_PrintStats(
          Node *node,
          const int phyIndex)
{
  PhyData* thisPhy ;
  char buf[MAX_STRING_LENGTH];
  float volt;
  double now,duration;

  thisPhy = node->phyData[phyIndex];

  if ((thisPhy->eType != NO_ENERGY_MODEL )&&
      (thisPhy->energyStats))
    {
      volt =  thisPhy->powerConsmpTable->voltage;

      sprintf(buf, "Energy consumed (in mWh)in Transmit mode = %.6f",
          (double)((thisPhy->curLoad->powStats.totalTxPower *volt) / 3600.0) );

      IO_PrintStat(
           node,
           "Physical",
           "Energy Model",
           ANY_DEST,
           phyIndex,
           buf);

      sprintf(buf,"Energy consumed (in mWh)in Receive mode = %.6f",
          (double)((thisPhy->curLoad->powStats.totalRxPower*volt) / 3600.0) );

      IO_PrintStat(
           node,
           "Physical",
           "Energy Model",
           ANY_DEST,
           phyIndex,
           buf);

      sprintf(buf, "Energy consumed (in mWh)in Idle mode = %.6f",
          (double)((thisPhy->curLoad->powStats.totalIdlePower*volt) / 3600.0) );

      IO_PrintStat(
           node,
           "Physical",
           "Energy Model",
           ANY_DEST,
           phyIndex,
           buf);

      sprintf(buf, "Energy consumed (in mWh)in Sleep mode = %.6f",
          (double)((thisPhy->curLoad->powStats.totalSleepPower*volt) / 3600.0) );

      IO_PrintStat(
           node,
           "Physical",
           "Energy Model",
           ANY_DEST,
           phyIndex,
           buf);


      now = (double)
    ((double)node->getNodeTime()/(double)SECOND);

      duration = (double)
    ((double)thisPhy->curLoad->powStats.totalTxDuration / (double)SECOND);

      sprintf(buf, "Percentage of time in Transmit mode = %f",
          (double)(duration / now)*100.0 );
      IO_PrintStat(
           node,
           "Physical",
           "Energy Model",
           ANY_DEST,
           phyIndex,
           buf);

      duration = (double)
    ((double)thisPhy->curLoad->powStats.totalRxDuration / (double)SECOND);

      sprintf(buf, "Percentage of time in Receive mode = %f",
          (double)(duration / now)*100.0 );
      IO_PrintStat(
           node,
           "Physical",
           "Energy Model",
           ANY_DEST,
           phyIndex,
           buf);

      duration = (double)
    ((double)thisPhy->curLoad->powStats.totalIdleDuration / (double)SECOND);

      sprintf(buf, "Percentage of time in Idle mode = %f",
          (double)(duration / now) * 100.0 );
      IO_PrintStat(
           node,
           "Physical",
           "Energy Model",
           ANY_DEST,
           phyIndex,
           buf);

      duration = (double)
    ((double)thisPhy->curLoad->powStats.totalSleepDuration / (double)SECOND);

      sprintf(buf, "Percentage of time in Sleep mode = %f",
          (double)(duration / now) * 100.0 );
      IO_PrintStat(
           node,
           "Physical",
           "Energy Model",
           ANY_DEST,
           phyIndex,
           buf);

    }
}
