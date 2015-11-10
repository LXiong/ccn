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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include <list>

#include "api.h"
#include "phy.h"
#include "mac_cellular.h"
#include "layer2_umts.h"
#include "layer2_umts_rlc.h"
#include "phy_cellular.h"
#include "phy_umts.h"
#include "layer2_umts_mac_phy.h"
#include "layer3_umts.h"
#include "umts_constants.h"
#include "random.h"

#define DEBUG_SPECIFIED_NODE_ID 0 // DEFAULT shoud be 0
#define DEBUG_SINGLE_NODE   (node->nodeId == DEBUG_SPECIFIED_NODE_ID)
#define DEBUG_MAC_PHY_CHANNEL 0
#define DEBUG_MAC_PHY_RX_TX 0
#define DEBUG_MAC_PHY_BURST 0
#define DEBUG_MAC_PHY_INTERLAYER (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_POWER 0
#define DEBUG_TRANSPORT_FORMAT 0
#define DEBUG_MAC_PHY_DATA (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_MAC_PHY_HSDPA (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_MAC_PHY_RACH (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_MAC_PHY_TR_RL_CH (0 || (0 &&  DEBUG_SINGLE_NODE))

//-------------------------------------------------------------------------
// Utility funciton
//-------------------------------------------------------------------------
#if 0
// /**
// FUNCTION   :: UmtsMacPhyGetDefaultTrChTti
// LAYER      :: UMTS L2 MAC sublayer
// PURPOSE    :: get the default TTI of a transport channel from its type
// PARAMETERS ::
// trChType    : UmtsTransportChannelType  : transport channel type
// RETURN     :: int : TTI in slot
// **/
static
int UmtsMacPhyGetDefaultTrChTti(UmtsTransportChannelType  trChType)
{
    // currently FDD 3.84Mcps is  the only option
    // temporary implementation
    if (trChType == UMTS_TRANSPORT_BCH)
    {
        return (UMTS_TRANSPORT_BCH_DEFAULT_TTI / UMTS_SLOT_DURATION_384);
    }
    else if (trChType == UMTS_TRANSPORT_PCH)
    {
        return (UMTS_TRANSPORT_PCH_DEFAULT_TTI / UMTS_SLOT_DURATION_384);
    }
    else if (trChType == UMTS_TRANSPORT_FACH)
    {
        return (UMTS_TRANSPORT_FACH_DEFAULT_TTI / UMTS_SLOT_DURATION_384);
    }
    else if (trChType == UMTS_TRANSPORT_RACH)
    {
        return (UMTS_TRANSPORT_RACH_DEFAULT_TTI / UMTS_SLOT_DURATION_384);
    }
    else if (trChType == UMTS_TRANSPORT_DCH)
    {
        return (UMTS_TRANSPORT_DCH_DEFAULT_TTI / UMTS_SLOT_DURATION_384);
    }
    else if (trChType == UMTS_TRANSPORT_DSCH)
    {
        return (UMTS_TRANSPORT_DSCH_DEFAULT_TTI / UMTS_SLOT_DURATION_384);
    }
    else if (trChType == UMTS_TRANSPORT_USCH)
    {
        return (UMTS_TRANSPORT_USCH_DEFAULT_TTI / UMTS_SLOT_DURATION_384);
    }
    else if (trChType == UMTS_TRANSPORT_HSDSCH)
    {
        return (UMTS_TRANSPORT_HSDSCH_DEFAULT_TTI / UMTS_SLOT_DURATION_384);
    }
    else
    {
        return (10 * MILLI_SECOND / UMTS_SLOT_DURATION_384);
    }
}
#endif

// /**
// FUNCTION   :: UmtsMacPhyNodeBFindUeInfo
// LAYER      :: Layer 2
// PURPOSE    :: return the active ul channel index 
//               of the interface at UE/NodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + priSc     : unsigned int : primary scrambling code
// RETURN     :: PhyUmtsNodeBUeInfo* : Ue Info at nodeB
// **/
static
PhyUmtsNodeBUeInfo* UmtsMacPhyNodeBFindUeInfo(Node*node,
                                 UInt32 interfaceIndex,
                                 unsigned int priSc)
{
    // get Phy data
    PhyUmtsNodeBUeInfo* ueInfo = NULL;
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsNodeBData* phyDataNodeB = phyUmts->phyDataNodeB;

    std::list<PhyUmtsNodeBUeInfo*>::iterator it;
    for ( it = phyDataNodeB->ueInfo->begin();
          it != phyDataNodeB->ueInfo->end();
          it ++)
    {
        if ((*it)->priUlScAssigned == priSc)
        {
            ueInfo = (*it);

            break;
        }
    }

    return ueInfo;
}

// /**
// FUNCTION   :: UmtsMacPhyNodeBCheckPowerControl
// LAYER      :: Layer 2
// PURPOSE    :: return the active ul channel index 
//               of the interface at UE/NodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + phyUmts   : PhyUmtsData* : UMTS Phy data
// + phyDataNodeB : PhyUmtsNodeBData* : NodeB Phy data
// + scCodeIndex  : unsigned int : scrambling code index 
// + sinr      : double  : sinr of the received signal
// + rxPower_mW : double : signal strength of the received signal
// + spreadFactor : double : spread factor
// RETURN     :: void   : NULL
// **/
static
void UmtsMacPhyNodeBCheckPowerControl(
                        Node* node,
                        UInt32 interfaceIndex,
                        PhyUmtsData* phyUmts,
                        PhyUmtsNodeBData* phyDataNodeB,
                        unsigned int scCodeIndex,
                        double sinr,
                        double rxPower_mW,
                        double spreadFactor)
{
    PhyUmtsNodeBUeInfo* ueInfo = NULL;
    ueInfo = UmtsMacPhyNodeBFindUeInfo(
                        node, interfaceIndex, scCodeIndex);

    if (!ueInfo->needUlPowerControl)
    {
        // determine steps. by default is 1
        if (ueInfo->powCtrlAftCellSwitch)
        {
            ueInfo->powCtrlAftCellSwitch = FALSE;
            ueInfo->ulPowCtrlSteps = (int)abs((int)(IN_DB(sinr) - 
                                               ueInfo->targetUlSir));
        }
        else
        {
            ueInfo->ulPowCtrlSteps = 1;

        }

        // determine dec.r inc.

        double threshold = phyDataNodeB->reference_threshold_dB;
        double sf = IN_DB(spreadFactor);

        if ((ueInfo->targetUlSir > IN_DB(sinr)) &&
            (IN_DB(rxPower_mW) < (threshold - sf)))
        {
            ueInfo->needUlPowerControl = TRUE;
            ueInfo->ulPowCtrlCmd = UMTS_POWER_CONTROL_CMD_INC;
            if (DEBUG_POWER )
            {
                UmtsLayer2PrintRunTimeInfo(
                    node, interfaceIndex, UMTS_LAYER2_SUBLAYER_NONE);
                printf("\n\tUL sig SINR from UE %d is %f, target is %f,"
                        "need command to increase power\n",
                        ueInfo->ueId, IN_DB(sinr),
                        ueInfo->targetUlSir);
            }
        }
        else if ((ueInfo->targetUlSir < IN_DB(sinr)) &&
                 (IN_DB(rxPower_mW) >= (threshold - sf)))
        {
            ueInfo->needUlPowerControl = TRUE;
            ueInfo->ulPowCtrlCmd = UMTS_POWER_CONTROL_CMD_DEC;
            if (DEBUG_POWER )
            {
                UmtsLayer2PrintRunTimeInfo(
                    node, interfaceIndex, UMTS_LAYER2_SUBLAYER_NONE);
                printf("\n\tUL sig SINR from UE %d is %f, target is %f,"
                        "need command to decrease power\n",
                        ueInfo->ueId, IN_DB(sinr),
                        ueInfo->targetUlSir);
            }
        }
    } // if (!ueInfo->needUlPowerControl)
}

// /**
// FUNCTION   :: UmtsMacPhyNodeBHandleHsdpcchMsg
// LAYER      :: Layer 2
// PURPOSE    :: return the active ul channel index 
//               of the interface at UE/NodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + pccpchSfn : unsigned int : PCCPCH FSN
// RETURN     :: cellId   : cell Id of the nodeB
// **/
static
void UmtsMacPhyNodeBHandleHsdpcchMsg(
                        Node* node,
                        UInt32 interfaceIndex,
                        PhyUmtsData* phyUmts,
                        PhyUmtsNodeBData* phyDataNodeB,
                        unsigned int scCodeIndex,
                        unsigned char cqiVal)
{
    PhyUmtsNodeBUeInfo* ueInfo = NULL;
    ueInfo = UmtsMacPhyNodeBFindUeInfo(
                        node, interfaceIndex, scCodeIndex);

    if (ueInfo)
    {
        ueInfo->cqiInfo.repVal = cqiVal;
        if (DEBUG_MAC_PHY_HSDPA || 0)
        {
            UmtsLayer2PrintRunTimeInfo(
                node, interfaceIndex,
                UMTS_LAYER2_SUBLAYER_NONE);
            printf("\n\trcvd packet from UE %d HSDPCCH with cqi %d\n",
                ueInfo->ueId, cqiVal);
        }
    }
    else
    {
        // REPORT WARNING
    }
}

// /**
// FUNCTION   :: UmtsMacPhyIsThisBurstForMe
// DECRIPTION :: Determine if the signal is for me or not
// PARAMETERS ::
// + node        : Node*                     : Pointer to the node
// + interfaceIndex : UInt32                 : interfaceIndex
// + phyVar      : void*                     : phy data
// + chType      : UmtsPhysicalChannelType   : physical channel type
// + scCodeIndex : unsigned int              : channel Idetifier
// + phChCode    : UInt32                    : phsical channel code for
//                                             the incoming burst
// + sfFactor    : UmtsSpreadFactor          : Spreading factor
// **/
static
BOOL UmtsMacPhyIsThisBurstForMe(Node* node,
                                UInt32 interfaceIndex,
                                void* phyVar,
                                UmtsPhysicalChannelType chType,
                                unsigned int scCodeIndex,
                                UInt32 phChCode = 0,
                                UmtsSpreadFactor sfFactor = UMTS_SF_1)
{
    BOOL isForMe = FALSE;

    if (UmtsGetNodeType(node) == CELLULAR_UE)
    {
        PhyUmtsUeData*  uePhyVar;

        uePhyVar = (PhyUmtsUeData*)phyVar;
        switch(chType)
        {
            case UMTS_PHYSICAL_PSCH:
            case UMTS_PHYSICAL_SSCH:
            case UMTS_PHYSICAL_CPICH:
            case UMTS_PHYSICAL_PCCPCH:
            case UMTS_PHYSICAL_AICH:
            case UMTS_PHYSICAL_PICH:
            case UMTS_PHYSICAL_SCCPCH:
            {
                isForMe = TRUE;

                break;
            }
            case UMTS_PHYSICAL_HSPDSCH:
            case UMTS_PHYSICAL_HSSCCH:
            case UMTS_PHYSICAL_DPDCH:
            case UMTS_PHYSICAL_DPCCH:
            {
                // check if the scambleing code is from the
                // activeSet
                std::list<PhyUmtsUeNodeBInfo*>::iterator it;
                unsigned int priScCode;

                // each group has 16 codes, the first one is used 
                // for primary scrambling code for DL
                priScCode = ((int)(scCodeIndex / 16)) * 16;
                for(it = uePhyVar->ueNodeBInfo->begin();
                    it != uePhyVar->ueNodeBInfo->end();
                    it ++)
                {
                    if (priScCode == (*it)->primaryScCode)
                    {
                        std::list<UmtsRcvPhChInfo*>* rcvPhChInfoList
                            = (*it)->dlPhChInfoList;
                        if ((*it)->dlPhChInfoList
                            && (std::find_if(
                                rcvPhChInfoList->begin(),
                                rcvPhChInfoList->end(),
                                UmtsFindItemByOvsf<UmtsRcvPhChInfo>
                                    (phChCode, sfFactor))
                            != rcvPhChInfoList->end()))
                        {
                            isForMe = TRUE;
                            break;
                        }
                    }
                }

                break;
            }
            default:
            {
                ERROR_ReportWarning("UE rcve msg from unsupported PHY CH");
            }
        }
    }
    else if (UmtsGetNodeType(node) == CELLULAR_NODEB)
    {
        PhyUmtsNodeBData*  nodebPhyVar;

        nodebPhyVar = (PhyUmtsNodeBData*)phyVar;
        switch(chType)
        {
            case UMTS_PHYSICAL_PRACH:
            {
                // check if the scambleing code is from the nodeB's group
                // each group has 16 code, first one is used as 
                // primary scambling code
                if (scCodeIndex >= nodebPhyVar->pscCodeIndex &&
                    scCodeIndex < nodebPhyVar->pscCodeIndex + 16)
                {
                    if (DEBUG_MAC_PHY_RX_TX)
                    {
                        UmtsLayer2PrintRunTimeInfo(
                            node,
                            interfaceIndex,
                            UMTS_LAYER2_SUBLAYER_NONE);
                        printf("\n\trcvd packet from UE PRACH\n");
                    }
                    isForMe = TRUE;
                }
                break;
            }
            case UMTS_PHYSICAL_HSDPCCH:
            case UMTS_PHYSICAL_DPDCH:
            case UMTS_PHYSICAL_DPCCH:
            {
                // check if the scrambling code in my allocated list
                PhyUmtsNodeBUeInfo* nodebUeInfo =
                    UmtsMacPhyNodeBFindUeInfo(
                        node, interfaceIndex, scCodeIndex);
                if ( nodebUeInfo && nodebUeInfo->isSelfPrimCell)
                {
                    isForMe = TRUE;
                    if (DEBUG_MAC_PHY_RX_TX)
                    {
                        UmtsLayer2PrintRunTimeInfo(
                            node,
                            interfaceIndex,
                            UMTS_LAYER2_SUBLAYER_NONE);
                        printf("\n\trcvd pkt from UE %d's DPCH/HSDPCCH\n",
                            nodebUeInfo->ueId);
                    }
                }
                break;
            }
            default:
            {
                ERROR_ReportWarning(
                    "NodeB: rcve msg from unsupported PHY CH");
            }
        } // switch(chType)
    } // if (UmtsGetNodeType)

    return isForMe;
}

// /**
// FUNCTION   :: UmtsMacGetTrChInfoFromList
// LAYER      :: UMTS L2 MAC sublayer
// PURPOSE    :: Get the transport channel info from the channel info list
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// chList      :std::list<UmtsTransChInfo*>* : Point to channel List
// trChType    : UmtsTransportChannelType  : transport channel type
// chId        : unsigned int              : channel Idetifier
// RETURN     :: UmtsTransChInfo* : pointer to the channel info
// **/
static
UmtsTransChInfo* UmtsMacGetTrChInfoFromList(
                     Node* node,
                     std::list<UmtsTransChInfo*>* chList,
                     UmtsTransportChannelType chType,
                     unsigned int chId)
{
    UmtsTransChInfo* chInfo = NULL;
    std::list<UmtsTransChInfo*>::iterator it;
    for (it = chList->begin();
        it != chList->end();
        it ++)
    {
        if ((*it)->trChType == chType &&
            (*it)->trChId == chId)
        {
            break;
        }
    }

    if (it != chList->end())
    {
        chInfo = *it;
    }

    return chInfo;
}

// /**
// FUNCTION   :: UmtsMacGetPhChInfoFromList
// LAYER      :: UMTS L2 MAC sublayer
// PURPOSE    :: Get the transport channel info from the channel info list
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// chList      :std::list<PhyUmtsChannelData*>* : Point to channel List
// chType      : UmtsPhysicalChannelType   : physical channel type
// chId        : unsigned int              : channel Idetifier
// RETURN     :: PhyUmtsChannelData* : pointer to the channel info
// **/
static
PhyUmtsChannelData* UmtsMacGetPhChInfoFromList(
                     Node* node,
                     std::list <PhyUmtsChannelData*>* phyChList,
                     UmtsPhysicalChannelType chType,
                     unsigned int chId)
{
    PhyUmtsChannelData* chInfo = NULL;
    std::list<PhyUmtsChannelData*>::iterator it;
    for (it = phyChList->begin();
        it != phyChList->end();
        it ++)
    {
        if ((*it)->phChType == chType &&
            (*it)->channelIndex == chId)
        {
            break;
        }
    }

    if (it != phyChList->end())
    {
        chInfo = *it;
    }

    return chInfo;
}

// /**
// FUNCTION   :: UmtsMacPhyNodeBGetUeInfoFromList
// LAYER      :: UMTS L2 MAC sublayer
// PURPOSE    :: Get the ue info from the ue info list at Node B
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// ueInfoList  : std::list <UmtsMacNodeBUeInfo*>* : Point to UE Info List
// priSc       : unsinged int              : UE's primary scrambling code
// RETURN     :: UmtsMacNodeBUeInfo*       : pointer to the UE info
// **/
static
UmtsMacNodeBUeInfo* UmtsMacPhyNodeBGetUeInfoFromList(
                        Node* node,
                        std::list <UmtsMacNodeBUeInfo*>* ueInfoList,
                        unsigned int priSc)
{
    UmtsMacNodeBUeInfo* ueInfo = NULL;
    std::list<UmtsMacNodeBUeInfo*>::iterator it;
    for (it = ueInfoList->begin();
        it !=  ueInfoList->end();
        it ++)
    {
        if ((*it)->priSc == priSc)
        {
            break;
        }
    }

    if (it != ueInfoList->end())
    {
        ueInfo = *it;
    }

    return ueInfo;
}

// /**
// FUNCTION   :: UmtsPhyAddCdmaBurstInfo
// LAYER      :: PHY
// PURPOSE    :: Put burst information in the info field of the message
// PARAMETERS ::
// + node      : Node*    : Pointer to node
// + msg       : Message* : Pointer to the message of the burst
// + burstInfo : PhyUmtsCdmaBurstInfo* : Information of the burst
// RETURN     :: void : NULL
// **/
static
void inline UmtsPhyAddCdmaBurstInfo(Node* node,
                                    Message* msg,
                                    PhyUmtsCdmaBurstInfo* burstInfo)
{
    MESSAGE_InfoAlloc(node, msg, sizeof(PhyUmtsCdmaBurstInfo));
    memcpy(MESSAGE_ReturnInfo(msg),
           burstInfo,
           sizeof(PhyUmtsCdmaBurstInfo));
}

// /**
// FUNCTION   :: UmtsPhyCreateCdmaBurstInfo
// LAYER      :: UMTS LAYER2 MAC sublayer at UE
// PURPOSE    :: Handle timers out messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + phyChList        : std::list <PhyUmtsChannelData*>* : Phy ch list
// + chType           : UmtsPhysicalChannelType : Channel type
// + phChId           : unsigned char     : channel Id
// + burstStartTime   : clocktype         : start time of the burst 
// + burstEndTime     : clocktype         : end time of the burst
// + burstInfo        : PhyUmtsCdmaBurstInfo* : burstInfo to create
// RETURN     :: void : NULL
// **/
static
void UmtsPhyCreateCdmaBurstInfo(
            Node *node,
            UInt32 interfaceIndex,
            std::list <PhyUmtsChannelData*>* phyChList,
            UmtsPhysicalChannelType chType,
            unsigned char phChId,
            clocktype burstStartTime,
            clocktype burstEndTime,
            PhyUmtsCdmaBurstInfo* burstInfo)
{
    std::list<PhyUmtsChannelData*> ::iterator it;

    for( it = phyChList->begin();
        it != phyChList->end();
        it ++)
    {
        if((*it)->channelIndex == phChId &&
           (*it)->phChType == chType)
        {
            break;
        }
    }
    ERROR_Assert(it != phyChList->end(),
        "ERROR");

    memset(burstInfo, 0, sizeof(PhyUmtsCdmaBurstInfo));
    burstInfo->phChType = (*it)->phChType;
    burstInfo->chDirection = (*it)->chDirection;
    burstInfo->spreadFactor = (*it)->spreadFactor;
    burstInfo->chCodeIndex = (*it)->chCodeIndex;
    burstInfo->scCodeIndex = (*it)->scCodeIndex;
    burstInfo->chPhase = (*it)->chPhase;
    burstInfo->gainFactor =
        UmtsPhyCalculateGainFactor((*it)->spreadFactor);
    burstInfo->moduType = (*it)->moduType;
    burstInfo->codingType = (*it)->codingType;
    burstInfo->burstStartTime = burstStartTime;
    burstInfo->burstEndTime = burstEndTime;

    // for sync time info
    burstInfo->syncTimeInfo = node->nodeId;

    // set other info in the burst such as the
    // start time of the busrt
    // end time of the burst

    if (DEBUG_MAC_PHY_BURST)
    {
        UmtsLayer2PrintRunTimeInfo(node,
                                   interfaceIndex,
                                   UMTS_LAYER2_SUBLAYER_NONE);
        printf("\n\t add cdma busrt info "
               "chtype %d, cdDriection %d, sf %d,\n"
               "chCodeIndex %d, scCodeIdnex %d,"
               "chPahse %d, gainFactor %f, moduType %d,"
               "coding type %d\n",
               burstInfo->phChType,burstInfo->chDirection,
               burstInfo->spreadFactor, burstInfo->chCodeIndex,
               burstInfo->scCodeIndex, burstInfo->chPhase,
               burstInfo->gainFactor, burstInfo->moduType,
               burstInfo->codingType);
    }
}

// /**
// FUNCTION   :: UmtsMacUeUpdateSlotInfo
// LAYER      :: UMTS LAYER2 MAC sublayer at UE
// PURPOSE    :: Handle timers out messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + mac              : UmtsMacData*      : Mac Data
// RETURN     :: void : NULL
// **/
static
void UmtsMacUeUpdateSlotInfo(Node* node,
                             UInt32 interfaceIndex,
                             UmtsMacData* mac)
{
    int phyIndex = node->macData[interfaceIndex]->phyNumber;

    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;

    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsUeData* phyDataUe =
        (PhyUmtsUeData*) phyUmts->phyDataUe;
    phyUmts->slotIndex = (phyUmts->slotIndex + 1 ) %
                          UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME;

    if (phyDataUe->raInfo &&
        phyDataUe->raInfo->needSendRachData &&
        phyDataUe->raInfo->rachDataWaitTime > 0)
    {
        phyDataUe->raInfo->rachDataWaitTime --;
    }
    else if (phyDataUe->raInfo &&
        phyDataUe->raInfo->needSendRachPreamble &&
        phyDataUe->raInfo->rachPreambleWaitTime > 0)
    {
        phyDataUe->raInfo->rachPreambleWaitTime --;
    }
    else if (phyDataUe->raInfo &&
        phyDataUe->raInfo->waitForAich &&
        phyDataUe->raInfo->aichWiatTime > 0)
    {
        phyDataUe->raInfo->aichWiatTime --;

        if (phyDataUe->raInfo->aichWiatTime == 0)
        {
            // decrease the retry count
            phyDataUe->raInfo->retryCount --;
            if (phyDataUe->raInfo->retryCount > 0)
            {
                RandomDistribution<Int32> randomInt;
                randomInt.setSeed(phyCellular->randSeed[1]);
                randomInt.setDistributionUniformInteger(
                    phyDataUe->raInfo->sigStartIndex,
                    phyDataUe->raInfo->sigEndIndex);
                phyDataUe->raInfo->preambleSig =
                    (unsigned char)(randomInt.getRandomNumber());

                // TODO: get the slot index in the frame
                phyDataUe->raInfo->preambleSlot =
                    (unsigned char)(RANDOM_erand(phyCellular->randSeed) *
                    UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME);

                // FOR PRACH/AICH, one lsot = 2 regular slot
                phyDataUe->raInfo->rachPreambleWaitTime =
                    phyDataUe->raInfo->preambleSlot * 2;

                phyDataUe->raInfo->needSendRachPreamble = TRUE;
                phyDataUe->raInfo->waitForAich = FALSE;
                phyDataUe->raInfo->cmdPrePower +=
                    PHY_UMTS_DEFAULT_POWER_RAMP_STEP; 

                if (phyDataUe->raInfo->cmdPrePower >
                              phyUmts->maxTxPower_dBm + 6)
                {
                    MEM_free(phyDataUe->raInfo);
                    phyDataUe->raInfo = NULL;
                    UmtsPhyAccessCnfInfo cnfInfo;
                    cnfInfo.cnfType = UMTS_PHY_ACCESS_CFN_NO_AICH_RSP;

                    UmtsMacHandleInterLayerCommand(
                        node, interfaceIndex,
                        UMTS_PHY_ACCESS_CNF, (void*)&cnfInfo);

                    return;
                }

                PhyUmtsSetTransmitPower(
                    node,
                    phyIndex,
                    NON_DB(phyDataUe->raInfo->cmdPrePower));
                if (DEBUG_MAC_PHY_INTERLAYER || 
                    DEBUG_MAC_PHY_RACH || 
                    DEBUG_POWER)
                {
                    UmtsLayer2PrintRunTimeInfo(
                        node, interfaceIndex, UMTS_LAYER2_SUBLAYER_NONE);
                    printf(
                        "node %d PHY: need to resend RACH, retry count %d "
                        "preamble at slot %d with sig %d power %f(dBm)\n",
                         node->nodeId, phyDataUe->raInfo->retryCount,
                         phyDataUe->raInfo->rachPreambleWaitTime,
                         phyDataUe->raInfo->preambleSig,
                         phyDataUe->raInfo->cmdPrePower);
                }
            }
            else
            {
                // exit the rach procedure
                if (phyDataUe->raInfo)
                {
                    MEM_free(phyDataUe->raInfo);
                    phyDataUe->raInfo = NULL;
                }
                UmtsPhyAccessCnfInfo cnfInfo;
                cnfInfo.cnfType = UMTS_PHY_ACCESS_CFN_NO_AICH_RSP;

                UmtsMacHandleInterLayerCommand(
                    node, interfaceIndex,
                    UMTS_PHY_ACCESS_CNF, (void*)&cnfInfo);
            } // if (phyDataUe->raInfo->retryCount)
        } // if (phyDataUe->raInfo->aichWiatTime)
    } // if (phyDataUe->raInfo && .. )
}

// /**
// FUNCTION   :: UmtsMacPhyUeUpdateHsdpaCqiVal
// LAYER      :: UMTS LAYER2 MAC sublayer at UE
// PURPOSE    :: Handle timers out messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + phyDataUe        : PhyUmtsUeData*    : PhyUeData
// + priScCode        : unsigned int      : primary Sc Code
// RETURN     :: unsigned char : cqiVal
// **/
static
unsigned char UmtsMacPhyUeUpdateHsdpaCqiVal
                    (Node* node,
                     UInt32 interfaceIndex,
                     PhyUmtsUeData* phyDataUe,
                     unsigned int priScCode)
{
    PhyUmtsUeNodeBInfo* ueNodeBInfo = NULL;

    std::list<PhyUmtsUeNodeBInfo*>::iterator it;

    for(it = phyDataUe->ueNodeBInfo->begin();
        it != phyDataUe->ueNodeBInfo->end();
        it ++)
    {
        if((*it)->primaryScCode == priScCode)
        {
            ueNodeBInfo = *it;
            break;
        }
    }

    ERROR_Assert(
        ueNodeBInfo, "no UeNodeBInfolink with this priScCode");

    if (IN_DB(ueNodeBInfo->cpichEcNo) +
        PHY_UMTS_CQI_REPORT_TAU <= 0)
    {
        return 0;
    }
    else if (IN_DB(ueNodeBInfo->cpichEcNo) +
        PHY_UMTS_CQI_REPORT_TAU >= 30)
    {
        return 30;
    }
    else
    {
        return ((unsigned char)IN_DB(ueNodeBInfo->cpichEcNo) +
                 PHY_UMTS_CQI_REPORT_TAU);
    }
}

// /**
// FUNCTION   :: UmtsMacUeTransmitMacBurst
// LAYER      :: UMTS LAYER2 MAC sublayer at UE
// PURPOSE    :: Handle timers out messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + mac              : UmtsMacData*      : Mac Data
// + msg              : Message*          : msg List to send to phy
// RETURN     :: void : NULL
// **/
void UmtsMacUeTransmitMacBurst(Node* node,
                               UInt32 interfaceIndex,
                               UmtsMacData* mac,
                               Message* msg)
{
    Message* firstMsg = NULL;
    Message* dpcchMsg = NULL;
    BOOL dpcchNeeded = FALSE;
    clocktype duration = 0;
    BOOL needSendRachPre = FALSE;
    BOOL needSendRachData = FALSE;

    PhyUmtsCdmaBurstInfo burstInfo;
    std::list <PhyUmtsChannelData*>* phyChList;
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    clocktype currentTime = node->getNodeTime();

    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;

    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsUeData* phyDataUe =
        (PhyUmtsUeData*) phyUmts->phyDataUe;

    phyChList = phyDataUe->phyChList;
    memset(&burstInfo, 0, sizeof(PhyUmtsCdmaBurstInfo));

    if (DEBUG_MAC_PHY_RX_TX)
    {
        UmtsLayer2PrintRunTimeInfo(
            node, interfaceIndex, UMTS_LAYER2_SUBLAYER_NONE);
        printf("Node %d MAC-PHY: UE  TX a msg to Phy\n", node->nodeId);
    }

    // to see if PRACH preamble needs to be send
    if (phyDataUe->raInfo
        && phyDataUe->raInfo->needSendRachPreamble &&
        phyDataUe->raInfo->rachPreambleWaitTime == 0)
    {
        ERROR_Assert(!phyDataUe->raInfo->needSendRachData,
            "Rach MS part canno be sent before preamble");

        needSendRachPre = TRUE;
        phyDataUe->raInfo->needSendRachPreamble = FALSE;
        phyDataUe->raInfo->waitForAich = TRUE;
        
        // max waiting time (two frame) for the AICH
        phyDataUe->raInfo->aichWiatTime =
            2 * UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME;
    }

    if (phyDataUe->raInfo &&
        phyDataUe->raInfo->needSendRachData &&
        phyDataUe->raInfo->rachDataWaitTime == 0)
    {
        needSendRachData = TRUE;
        phyDataUe->raInfo->needSendRachData = FALSE;
    }

    //  create PHY Ch msgs and add to the msg list;
    std::list<PhyUmtsChannelData*>::iterator it;

    for (it = phyChList->begin();
         it != phyChList->end();
         it ++)
    {
        if ((*it)->phChType != UMTS_PHYSICAL_PRACH ||
            (needSendRachData &&
            (*it)->phChType == UMTS_PHYSICAL_PRACH) ||
            (needSendRachPre &&
            (*it)->phChType == UMTS_PHYSICAL_PRACH))
        {
            Message* otherMsg = NULL;

            if (needSendRachPre &&
               (*it)->phChType == UMTS_PHYSICAL_PRACH)
            {
                // preamble
                otherMsg = MESSAGE_Alloc(node, 0, 0, 0);
                MESSAGE_PacketAlloc(node, otherMsg, 0, TRACE_UMTS_PHY);
            }
            else if (phyDataUe->hsdpaInfo.repCqiNeeded &&
                (*it)->phChType == UMTS_PHYSICAL_HSDPCCH)
            {
                // CQI value report
                otherMsg = MESSAGE_Alloc(node, 0, 0, 0);
                MESSAGE_PacketAlloc(node, otherMsg, 0, TRACE_UMTS_PHY);
                phyDataUe->hsdpaInfo.repCqiNeeded = FALSE;
            }
            else if (!(*it)->txMsgList->empty())
            {

                if (DEBUG_MAC_PHY_DATA && 0 &&
                    (*it)->phChType == UMTS_PHYSICAL_DPDCH)
                {
                    printf (
                        "Node: %d is transmiting %" TYPES_SIZEOFMFT 
                        "u MAC msgs on DPDCH.\n",
                        node->nodeId, (*it)->txMsgList->size());
                    UmtsPrintMessage(std::cout, (*it)->txMsgList->front());
                }
                otherMsg = UmtsPackMessage(
                               node,
                               *((*it)->txMsgList),
                               TRACE_CELLULAR);
                (*it)->txMsgList->clear();
            }

            if (otherMsg)
            {
                //clocktype txTime;

                // get the
                UmtsPhyCreateCdmaBurstInfo(node,
                               interfaceIndex,
                               phyChList,
                               (*it)->phChType,
                               (*it)->channelIndex,
                               currentTime,
                               currentTime + mac->slotDuration,
                               &burstInfo);
                if (needSendRachPre &&
                    (*it)->phChType == UMTS_PHYSICAL_PRACH)
                {
                    // indicate this is a preamble
                    burstInfo.prachPreamble = TRUE;
                    burstInfo.preSig = phyDataUe->raInfo->preambleSig;
                    burstInfo.preSlot = phyDataUe->raInfo->preambleSlot;
                    burstInfo.chPhase = UMTS_PHY_CH_BRANCH_I;
                    needSendRachPre = FALSE;
              
                    if (DEBUG_MAC_PHY_RACH)
                    {
                        printf(
                            "node %d: send preamble in "
                            "slot %d with sig %d\n",
                            node->nodeId, 
                            phyUmts->slotIndex,
                            burstInfo.preSig);
                    }
                }
                else if ((*it)->phChType == UMTS_PHYSICAL_HSDPCCH)
                {
                    // HSDPA
                    burstInfo.cqiVal =
                        UmtsMacPhyUeUpdateHsdpaCqiVal(
                            node,
                            interfaceIndex,
                            phyDataUe,
                            phyDataUe->hsdpaInfo.ServHsdpaNodeBPriScCode);

                    // update the hsdpaInfo
                    phyDataUe->hsdpaInfo.lastRepCqiVal = burstInfo.cqiVal;
                    phyDataUe->hsdpaInfo.lastRepTime = node->getNodeTime();
                    if (DEBUG_MAC_PHY_HSDPA)
                    {
                        UmtsLayer2PrintRunTimeInfo(
                            node, interfaceIndex, 
                            UMTS_LAYER2_SUBLAYER_NONE);
                        printf("\n\ttransmit cqi %d to nodeB via HSDPCCH\n",
                            burstInfo.cqiVal);
                    }
                }
                else if((*it)->phChType == UMTS_PHYSICAL_DPDCH)
                {
                    // the spread factor for DPDCH should be
                    // dpchSfInUse
                    // burstInfo.spreadFactor = phyDataUe->dpchSfInUse;
                    if (!dpcchNeeded)
                    {
                        dpcchNeeded = TRUE;
                    }
                }
                else if (needSendRachData &&
                    (*it)->phChType == UMTS_PHYSICAL_PRACH)
                {
                    double txPower;
                    PhyUmtsUePrachInfo* prachDesc;

                    burstInfo.chPhase = UMTS_PHY_CH_BRANCH_Q;
                    prachDesc = (PhyUmtsUePrachInfo*)(*it)->moreChInfo;

                    txPower = NON_DB(phyDataUe->raInfo->cmdPrePower +
                                     prachDesc->powerOffset);
                    // set the tx power
                    PhyUmtsSetTransmitPower(node,
                                            phyIndex,
                                            txPower);
                    if (DEBUG_POWER)
                    {
                        UmtsLayer2PrintRunTimeInfo(
                            node, interfaceIndex, 
                            UMTS_LAYER2_SUBLAYER_NONE);

                        printf(" to send PRACH msg set "
                            "Tx Power to %f %f(dBm)\n",
                            txPower, IN_DB(txPower));
                    }
                }
                UmtsPhyAddCdmaBurstInfo(
                    node,
                    otherMsg,
                    &burstInfo);
                ERROR_Assert(otherMsg != NULL, "msg is NULL");

                otherMsg->next = firstMsg;
                firstMsg = otherMsg;

                // TODO: update duration
                duration = mac->slotDuration;
            }

            if (needSendRachData &&
                (*it)->phChType == UMTS_PHYSICAL_PRACH)
            {
                // free rachInfo and exit RACH procedure
                if (DEBUG_MAC_PHY_RACH)
                {
                    printf("node %d: send rach msg part "
                        "in slot %d burst priSc %d\n",
                    node->nodeId, 
                    phyUmts->slotIndex,
                    burstInfo.scCodeIndex);
                }

                if (phyDataUe->raInfo)
                {
                    MEM_free(phyDataUe->raInfo);
                    phyDataUe->raInfo = NULL;
                }

                needSendRachData = FALSE;
            }
        } // if ((*it)->phChType)
    } // for (it)

    if (dpcchNeeded)
    {
        // create DPCCH signal
        dpcchMsg = MESSAGE_Alloc(node, 0, 0, 0);
        // no packet field  is allocated in DPCCH
        // TODO: fill the DPCCH field
        MESSAGE_PacketAlloc(node, dpcchMsg, 0, TRACE_UMTS_PHY);

        memset(&burstInfo, 0, sizeof(PhyUmtsCdmaBurstInfo));

        UmtsPhyCreateCdmaBurstInfo(node,
                                   interfaceIndex,
                                   phyChList,
                                   UMTS_PHYSICAL_DPCCH,
                                   1,
                                   currentTime,
                                   currentTime + mac->slotDuration,
                                   &burstInfo);

        UmtsPhyAddCdmaBurstInfo(node, dpcchMsg, &burstInfo);

        dpcchMsg->next = firstMsg;
        firstMsg = dpcchMsg;
    }

    if (firstMsg)
    {
        // pack the msg list into one msg, add phy header
        firstMsg = UmtsPackMessage(node, firstMsg, TRACE_CELLULAR);

        // set transmission channel
        PHY_SetTransmissionChannel(
            node,
            node->macData[interfaceIndex]->phyNumber,
            mac->currentULChannel);

        // transmit the signal
        // it should be set as the logest burst duration

        PHY_StartTransmittingSignal(
                node,
                node->macData[interfaceIndex]->phyNumber,
                firstMsg,
                duration,
                TRUE,
                UMTS_DELAY_UNTIL_SIGNAL_AIRBORN);

        if (DEBUG_POWER )
        {
            UmtsLayer2PrintRunTimeInfo(
                node, interfaceIndex, UMTS_LAYER2_SUBLAYER_NONE);
            printf("\n\tUE %d Tx... with txPower %f\n",
                node->nodeId, (Float32)phyUmts->txPower_dBm);
        }
    }

    // increase the slot index
    UmtsMacUeUpdateSlotInfo(node, interfaceIndex, mac);
}

//-------------------------------------------------------------------------
// handle function for nodeB
//-------------------------------------------------------------------------
// /**
// FUNCTION   :: UmtsMacPhyNodeBReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer.
//               The PHY sublayer will first handle it
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + interfaceIndex   : int          : Interface Index
// + msg              : Message*     : Packet received from PHY
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyNodeBReceivePacketFromPhy(Node* node,
                                         int interfaceIndex,
                                         Message* msg)
{
    PhyUmtsCdmaBurstInfo* burstInfo;
    Message* tmpMsg;

    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsNodeBData* phyDataNodeB = phyUmts->phyDataNodeB;

    if (DEBUG_MAC_PHY_RX_TX )
    {
        UmtsLayer2PrintRunTimeInfo(
            node, interfaceIndex, UMTS_LAYER2_SUBLAYER_NONE);
        printf("\n");
    }

    while(msg != NULL)
    {
        tmpMsg = msg;
        msg = msg->next;

        burstInfo = (PhyUmtsCdmaBurstInfo*) MESSAGE_ReturnInfo(tmpMsg);

        if (!UmtsMacPhyIsThisBurstForMe(
                     node,
                     interfaceIndex,
                     (void*)phyDataNodeB,
                     burstInfo->phChType,
                     burstInfo->scCodeIndex,
                     burstInfo->chCodeIndex,
                     burstInfo->spreadFactor))
        {
            if (DEBUG_MAC_PHY_RX_TX)
            {
                if (burstInfo->phChType == UMTS_PHYSICAL_DPDCH)
                {
                    printf("\tNode: %u rcvs a DPDCH burst not for me\n",
                        node->nodeId);
                }
            }

            MESSAGE_Free(node, tmpMsg);
        }
        else if (burstInfo->inError)
        {
            if (DEBUG_MAC_PHY_RX_TX)
            {
                if (burstInfo->phChType == UMTS_PHYSICAL_DPDCH)
                {
                    printf("\n\tNode: %u rcvs a "
                        "DPDCH burst in Error sir %f\n",
                        node->nodeId, IN_DB(burstInfo->cinr));
                }
            }
            if (burstInfo->prachPreamble &&
                burstInfo->phChType == UMTS_PHYSICAL_PRACH)
            {
                // for preamble in error
                if (!phyDataNodeB->
                            prachAichInfo.aichMsgNeeded)
                {
                    phyDataNodeB->
                        prachAichInfo.aichMsgNeeded = TRUE;

                    phyDataNodeB->
                        prachAichInfo.rcvdSlotIndex = burstInfo->preSlot;
                    // TODO: Have to make sure the preamble
                    // from all UE with the same slot
                    // should arrive at nodeB at the same slot
                }

                if (phyDataNodeB->
                    prachAichInfo.sigRcvInfo[burstInfo->preSig])
                {
                    // not the first time rcvd this dig in this slot
                    phyDataNodeB->
                        prachAichInfo.sigRcvInfo[burstInfo->preSig] =
                        PHY_UMTS_SIG_COLLISION;
                }
                else
                {
                   phyDataNodeB->
                       prachAichInfo.sigRcvInfo[burstInfo->preSig] =
                       PHY_UMTS_SIG_ERROR;
                }
            }
            else if (burstInfo->phChType == UMTS_PHYSICAL_DPDCH)
            {
                // out loop power control
                // currently SIRtarget is set to a minimum value
                // check power control
                UmtsMacPhyNodeBCheckPowerControl(
                    node,
                    interfaceIndex,
                    phyUmts,
                    phyDataNodeB,
                    burstInfo->scCodeIndex,
                    burstInfo->cinr,
                    burstInfo->burstPower_mW,
                    burstInfo->spreadFactor);
            }
            MESSAGE_Free(node, tmpMsg);
        }
        else
        {
            switch(burstInfo->phChType)
            {
                case UMTS_PHYSICAL_PRACH:
                {
                    if (DEBUG_MAC_PHY_RX_TX)
                    {
                        printf("\n\trcvs a burst from PRACH\n");
                    }
                    // check if the scrabmle code is in BodeB's group
                    // yes, continue to handle, otherwise free it.
                    // preamble  ,set the AICH
                    // UmtsMacPhyNodeBHandlePrachPreamble
                    // msg part pass to RLC
                    if (burstInfo->prachPreamble)
                    {
                        if (DEBUG_MAC_PHY_RACH)
                        {
                            printf("node%d: rcvd a preamble with sig %d "
                                "Preslot %d in slot%d\n",
                                node->nodeId, burstInfo->preSig,
                                burstInfo->preSlot, phyUmts->slotIndex);
                        }

                        if (!phyDataNodeB->
                            prachAichInfo.aichMsgNeeded)
                        {
                            phyDataNodeB->
                                prachAichInfo.aichMsgNeeded = TRUE;
                            phyDataNodeB->
                                prachAichInfo.rcvdSlotIndex = 
                                burstInfo->preSlot;
                                // TODO: Have to make sure the preamble 
                                // from all UE with the same slot
                                // should arrive at nodeB at the same slot
                        }
                        // preamble part
                        if (phyDataNodeB->
                            prachAichInfo.sigRcvInfo[burstInfo->preSig])
                        {
                            // not the first time rcvd this sig in this slot
                            phyDataNodeB->
                                prachAichInfo.sigRcvInfo[burstInfo->preSig] =
                                PHY_UMTS_SIG_COLLISION;
                        }
                        else
                        {
                           phyDataNodeB->
                               prachAichInfo.sigRcvInfo[burstInfo->preSig] =
                               PHY_UMTS_SIG_SUCC;
                        }
                        MESSAGE_Free(node, tmpMsg);
                    }
                    else
                    {
                        std::list<Message*> msgList;
                        UmtsUnpackMessage(
                            node, tmpMsg, FALSE, TRUE, msgList);
                        UmtsRlcReceivePduFromMac(
                            node, interfaceIndex, UMTS_CCCH_RB_ID,
                            msgList, FALSE, node->nodeId);
                    }
                    break;
                }
                case UMTS_PHYSICAL_DPDCH:
                {
                    std::list<Message*> msgList;

                    if (DEBUG_MAC_PHY_RX_TX)
                    {
                        printf("\n\tNode: %u rcvs a burst from "
                            " DPDCH UL SIR is %f\n",
                            node->nodeId, IN_DB(burstInfo->cinr));
                    }

                    // check power control
                    UmtsMacPhyNodeBCheckPowerControl(
                        node,
                        interfaceIndex,
                        phyUmts,
                        phyDataNodeB,
                        burstInfo->scCodeIndex,
                        burstInfo->cinr,
                        burstInfo->burstPower_mW,
                        burstInfo->spreadFactor);

                    // check if it is for this nodeB
                    UmtsUnpackMessage(node, tmpMsg, FALSE, TRUE, msgList);

                    BOOL errorIndi = FALSE;
                    //TODO check if there is a missing packet in the list
                    UmtsRlcReceivePduFromMac(
                        node,
                        interfaceIndex,
                        msgList,
                        errorIndi);

                    break;
                }
                case UMTS_PHYSICAL_DPCCH:
                {
                    if (DEBUG_MAC_PHY_RX_TX)
                    {
                        printf("\n\trcvs a burst from UL DPCCH\n");
                    }
                    // handle DL power control command
                    // check if it is for this nodeB
                    MESSAGE_Free(node, tmpMsg);

                    break;
                }
                case UMTS_PHYSICAL_HSDPCCH:
                {
                    if (DEBUG_MAC_PHY_RX_TX || 0)
                    {
                        printf("\n\trcvs a burst from UL HS-DPCCH\n");
                    }
                    UmtsMacPhyNodeBHandleHsdpcchMsg(
                        node,
                        interfaceIndex,
                        phyUmts,
                        phyDataNodeB,
                        burstInfo->scCodeIndex,
                        burstInfo->cqiVal);

                    MESSAGE_Free(node, tmpMsg);

                    break;
                }
                default:
                {
                    // TODO: forward the tmMsg to RLC
                    MESSAGE_Free(node, tmpMsg);
                }

            } // switch(burstInfo->phChType)
        } // burstInfo->inError
    } // while
    // 1. get the PHY layer related info, power control...
    // 2 .process those msgs from SCH, PICH,...AICH without Tr CH mapping
    // 3 .then pass the rest to the RLC
    // get the PHY layer related info
    // process those msgs from PRACH, to set the AICH
    // then pass the rest to the RLC
}

//  /**
// FUNCITON   :: UmtsMacPhyNodeBConfigTrCh
// LAYER      :: UMTS PHY
// PURPOSE    :: Handle Interlayer CPHY-TrCh-CONFIG-REQ at NodeB
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + nodeBMacVar      : UmtsMacNodeBData*    : NodeB mac var
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyNodeBConfigTrCh(Node* node,
                               UInt32 interfaceIndex,
                               UmtsMacData* umtsMacData,
                               UmtsMacNodeBData* nodeBMacVar,
                               void* cmd)
{
    UmtsCphyTrChConfigReqInfo* trChCfgInfo;
    trChCfgInfo = (UmtsCphyTrChConfigReqInfo*) cmd;

    if (trChCfgInfo->trChDir != UMTS_CH_DOWNLINK)
    {
        // only enlist downlink trCh
        return;
    }

    // create transport channel object
    UmtsTransChInfo* trChInfo = NULL;
    trChInfo = UmtsMacGetTrChInfoFromList(
                   node,
                   nodeBMacVar->nodeBTrChList,
                   trChCfgInfo->trChType,
                   trChCfgInfo->trChId);
    if (!trChInfo)
    {
        trChInfo = new UmtsTransChInfo(trChCfgInfo->ueId,
                                       trChCfgInfo->trChType,
                                       trChCfgInfo->trChDir,
                                       trChCfgInfo->trChId,
                                       trChCfgInfo->transFormat.TTI,
                                       trChCfgInfo->logChPrio);

        nodeBMacVar->nodeBTrChList->push_back(trChInfo);
        if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
        {
            printf("\tcreate a new TrInfo and add to list with "
                "type %d, chDir %d, chId %d TTI %d\n",
                trChInfo->trChType, trChInfo->trChDir,
                trChInfo->trChId, trChInfo->TTI);
        }
    }
    else
    {
        // reconfiguration
        // reconfigurate the transport format
    }
}

//  /**
// FUNCITON   :: UmtsMacPhyNodeBReleaseTrCh
// LAYER      :: UMTS PHY
// PURPOSE    :: Handle Interlayer CPHY-TrCh-RELEASE-REQ at Ue
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + nodeBMacVar      : UmtsMacNodeBData*    : NodeB mac var
// + cmd              : void*             : cmd to handle
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyNodeBReleaseTrCh(Node* node,
                                UInt32 interfaceIndex,
                                UmtsMacData* umtsMacData,
                                UmtsMacNodeBData* nodeBMacVar,
                                void* cmd)
{
    UmtsCphyTrChReleaseReqInfo* trChRelInfo;
    trChRelInfo = (UmtsCphyTrChReleaseReqInfo*) cmd;

    if (trChRelInfo->trChDir != UMTS_CH_DOWNLINK)
    {
        // only enlist downlink trCh
        return;
    }

    std::list<UmtsTransChInfo*>* chList;
    chList = nodeBMacVar->nodeBTrChList;

    std::list<UmtsTransChInfo*>::iterator it;
    for (it = chList->begin();
        it != chList->end();
        it ++)
    {
        if ((*it)->trChType == trChRelInfo->trChType &&
            (*it)->trChId == trChRelInfo->trChId)
        {
            break;
        }
    }

    if (it != chList->end())
    {
        if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
        {
            printf("\tnode %d (MAC_PHY):delete TrInfo from "
                "list with type %d, chDir %d, chId %d TTI %d\n",
                node->nodeId, (*it)->trChType, (*it)->trChDir,
                (*it)->trChId, (*it)->TTI);
        }
        delete (*it);
        chList->erase(it);
    }

    // TODO: send RELEASE-CNF to L3
}

// /**
// FUNCITON   :: UmtsMacPhyNodeBRlSetupModifyReq
// LAYER      :: UMTS PHY
// PURPOSE    :: Handle Interlayer CPHY-RL-SETUP-REQ at NodeB
//                              or CPHY-RL-MODIFY_REQ
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + nodeBMacVar      : UmtsMacNodeBData* : NodeB mac var
// + cmd              : void*             : cmd to handle
// + isRlSetup        : BOOL              : IS RL setup
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyNodeBRlSetupModifyReq(Node* node,
                                     UInt32 interfaceIndex,
                                     UmtsMacData* umtsMacData,
                                     UmtsMacNodeBData* nodeBMacVar,
                                     void* cmd,
                                     BOOL isRlSetup)
{
    UmtsCphyRadioLinkSetupReqInfo* rlReqInfo;
    std::list <PhyUmtsChannelData*>* phyChList;

    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsNodeBData* phyDataNodeB = phyUmts->phyDataNodeB;

    // by default QPSK, CONV_2 coding
    UmtsCodingType codingType = UMTS_CONV_2;

    rlReqInfo =  (UmtsCphyRadioLinkSetupReqInfo*) cmd;

    if (rlReqInfo->phChDir != UMTS_CH_DOWNLINK)
    {
        // only enlist downlink trCh
        // send L3  setup-CNF to L3
        return;
    }
    phyChList = phyDataNodeB->phyChList;
    PhyUmtsChannelData* phChInfo;

    phChInfo = UmtsMacGetPhChInfoFromList(
                    node,
                    phyChList,
                    rlReqInfo->phChType,
                    rlReqInfo->phChId);

    if (rlReqInfo->phChType == UMTS_PHYSICAL_PSCH)
    {
        UmtsCphyChDescPSCH* pschDesc;

        pschDesc = (UmtsCphyChDescPSCH*) rlReqInfo->phChDesc;

        if (!phChInfo)
        {
            phChInfo = new PhyUmtsChannelData(
                                rlReqInfo->phChId,
                                rlReqInfo->phChType,
                                UMTS_CH_DOWNLINK,
                                UMTS_SF_256,
                                0, // no meaning for PSCH
                                0, // no meaning for PSCH
                                UMTS_PHY_CH_BRANCH_IQ,
                                1,
                                UMTS_MOD_QPSK,
                                UMTS_CONV_2);

            phChInfo->moreChInfo = (UmtsCphyChDescSSCH*)
                                    MEM_malloc(sizeof(UmtsCphyChDescPSCH));
            phyDataNodeB->phyChList->push_back(phChInfo);

            memcpy(phChInfo->moreChInfo,
                   rlReqInfo->phChDesc,
                   sizeof(UmtsCphyChDescPSCH));
            if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
            {
                printf("\tNode %d(MAC-PHY): add new PSCH\n",
                    node->nodeId);
            }
        }
        else
        {
            // TODO
            memcpy(phChInfo->moreChInfo,
                   rlReqInfo->phChDesc,
                   sizeof(UmtsCphyChDescPSCH));

        }
    }
    else if (rlReqInfo->phChType == UMTS_PHYSICAL_SSCH)
    {
        UmtsCphyChDescSSCH* pschDesc;

        pschDesc = (UmtsCphyChDescSSCH*) rlReqInfo->phChDesc;
        if (!phChInfo)
        {
            phChInfo = new PhyUmtsChannelData(
                                rlReqInfo->phChId,
                                rlReqInfo->phChType,
                                UMTS_CH_DOWNLINK,
                                UMTS_SF_256,
                                0, // no meaning for SSCH
                                0, // no meaning for SSCH
                                UMTS_PHY_CH_BRANCH_IQ,
                                1,
                                UMTS_MOD_QPSK,
                                UMTS_CONV_2);

            phChInfo->moreChInfo = (UmtsCphyChDescSSCH*)
                                   MEM_malloc(sizeof(UmtsCphyChDescSSCH));
            // TODO
            memcpy(phChInfo->moreChInfo,
                   rlReqInfo->phChDesc,
                   sizeof(UmtsCphyChDescSSCH));

            phyDataNodeB->phyChList->push_back(phChInfo);
            if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
            {
                printf("\tNode %d(MAC-PHY): add new SSCH\n",
                    node->nodeId);
            }
        }
        else
        {
            // TODO
            memcpy(phChInfo->moreChInfo,
                   rlReqInfo->phChDesc,
                   sizeof(UmtsCphyChDescSSCH));
        }
    }
    else if (rlReqInfo->phChType == UMTS_PHYSICAL_CPICH)
    {
        if (!phChInfo)
        {
            phChInfo = new PhyUmtsChannelData(
                                rlReqInfo->phChId,
                                rlReqInfo->phChType,
                                UMTS_CH_DOWNLINK,
                                UMTS_SF_256,
                                0, // Cch, 256, 0
                                phyDataNodeB->pscCodeIndex,
                                UMTS_PHY_CH_BRANCH_IQ,
                                1,
                                UMTS_MOD_QPSK,
                                UMTS_CONV_2);

            phyDataNodeB->phyChList->push_back(phChInfo);
            if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
            {
                printf("\tNode %d(MAC-PHY): add new CPICH\n",
                    node->nodeId);
            }
        }
        else
        {
            // do nothing
        }
    }
    else if (rlReqInfo->phChType == UMTS_PHYSICAL_PCCPCH)
    {
        UmtsCphyChDescPCCPCH* pccpchDesc;
        pccpchDesc = (UmtsCphyChDescPCCPCH*) rlReqInfo->phChDesc;

        if (!phChInfo)
        {
            phChInfo = new PhyUmtsChannelData(
                                rlReqInfo->phChId,
                                rlReqInfo->phChType,
                                UMTS_CH_DOWNLINK,
                                UMTS_SF_256,
                                1, // Cch, 256, 1
                                phyDataNodeB->pscCodeIndex,
                                UMTS_PHY_CH_BRANCH_IQ,
                                1,
                                UMTS_MOD_QPSK,
                                UMTS_CONV_2);

            phChInfo->moreChInfo = (UmtsCphyChDescPCCPCH*)
                                  MEM_malloc(sizeof(UmtsCphyChDescPCCPCH));

            memcpy(phChInfo->moreChInfo,
                   pccpchDesc,
                   sizeof(UmtsCphyChDescPCCPCH));
            phyDataNodeB->phyChList->push_back(phChInfo);
            if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
            {
                printf("\tNode %d(MAC-PHY): add new PCCPCH\n",
                    node->nodeId);
            }
        }
        else
        {
            // TODO
            memcpy(phChInfo->moreChInfo,
                   pccpchDesc,
                   sizeof(UmtsCphyChDescPCCPCH));
        }
    }
    else if (rlReqInfo->phChType == UMTS_PHYSICAL_SCCPCH)
    {
        UmtsCphyChDescSCCPCH* sccpchDesc;
        sccpchDesc = (UmtsCphyChDescSCCPCH*) rlReqInfo->phChDesc;

        if (!phChInfo)
        {
            phChInfo = new PhyUmtsChannelData(
                                rlReqInfo->phChId,
                                rlReqInfo->phChType,
                                UMTS_CH_DOWNLINK,
                                sccpchDesc->sfFactor,
                                sccpchDesc->chCode,
                                sccpchDesc->dlScCode,
                                UMTS_PHY_CH_BRANCH_IQ,
                                1,
                                UMTS_MOD_QPSK,
                                UMTS_CONV_2);

            phChInfo->moreChInfo = (UmtsCphyChDescSCCPCH*)
                                  MEM_malloc(sizeof(UmtsCphyChDescSCCPCH));

            memcpy(phChInfo->moreChInfo,
                   sccpchDesc,
                   sizeof(UmtsCphyChDescSCCPCH));
            phyDataNodeB->phyChList->push_back(phChInfo);
            if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
            {
                printf("\tNode %d(MAC-PHY): add new SCCPCH\n",
                    node->nodeId);
            }
        }
        else
        {
            // TODO
            memcpy(phChInfo->moreChInfo,
                   sccpchDesc,
                   sizeof(UmtsCphyChDescSCCPCH));
        }
    }
    else if (rlReqInfo->phChType == UMTS_PHYSICAL_DPDCH ||
             rlReqInfo->phChType == UMTS_PHYSICAL_DPCCH)
    {
        UmtsCphyChDescDlDPCH* dpchDesc;
        dpchDesc = (UmtsCphyChDescDlDPCH*) rlReqInfo->phChDesc;

        codingType = UMTS_TURBO_3;
        if (!phChInfo)
        {
            phChInfo = new PhyUmtsChannelData(
                                rlReqInfo->phChId,
                                rlReqInfo->phChType,
                                UMTS_CH_DOWNLINK,
                                dpchDesc->sfFactor,
                                dpchDesc->dlChCode,
                                dpchDesc->dlScCode,
                                UMTS_PHY_CH_BRANCH_IQ,
                                1,
                                UMTS_MOD_QPSK,
                                codingType);

            phChInfo->moreChInfo = (UmtsCphyChDescDlDPCH*)
                                  MEM_malloc(sizeof(UmtsCphyChDescDlDPCH));

            memcpy(phChInfo->moreChInfo,
                   dpchDesc,
                   sizeof(UmtsCphyChDescDlDPCH));

            // update the slotFormat
            phChInfo->slotFormatInUse = dpchDesc->slotFormatIndex;

            phyDataNodeB->phyChList->push_back(phChInfo);
            if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
            {
                printf("\tNode %d(MAC-PHY): add new DL DPDCH or DPCCH "
                    "sfFactor%d ChCode %d slotFormat %d\n",
                    node->nodeId, phChInfo->spreadFactor,
                    phChInfo->chCodeIndex,
                    phChInfo->slotFormatInUse);
            }
        }
        else
        {
            memcpy(phChInfo->moreChInfo,
                    dpchDesc,
                    sizeof(UmtsCphyChDescDlDPCH));

            // update the slotFormat
            phChInfo->slotFormatInUse = dpchDesc->slotFormatIndex;
        }
    }
    else if (rlReqInfo->phChType == UMTS_PHYSICAL_PICH)
    {
        UmtsCphyChDescPICH* pichDesc;
        pichDesc = (UmtsCphyChDescPICH*) rlReqInfo->phChDesc;

        if (!phChInfo)
        {
            phChInfo = new PhyUmtsChannelData(
                                rlReqInfo->phChId,
                                rlReqInfo->phChType,
                                UMTS_CH_DOWNLINK,
                                pichDesc->sfFactor,
                                pichDesc->chCode,
                                pichDesc->scCode,
                                UMTS_PHY_CH_BRANCH_IQ,
                                1,
                                UMTS_MOD_QPSK,
                                UMTS_CONV_2);

            phChInfo->moreChInfo = (UmtsCphyChDescPICH*)
                                  MEM_malloc(sizeof(UmtsCphyChDescPICH));

            memcpy(phChInfo->moreChInfo,
                   pichDesc,
                   sizeof(UmtsCphyChDescPICH));
            phyDataNodeB->phyChList->push_back(phChInfo);
            if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
            {
                printf("\tNode %d(MAC-PHY): add new PICH\n",
                    node->nodeId);
            }
        }
        else
        {
            memcpy(phChInfo->moreChInfo,
                   pichDesc,
                   sizeof(UmtsCphyChDescPICH));
        }

    }
    else if (rlReqInfo->phChType == UMTS_PHYSICAL_AICH)
    {
        UmtsCphyChDescAICH* aichDesc;
        aichDesc = (UmtsCphyChDescAICH*) rlReqInfo->phChDesc;

        if (!phChInfo)
        {
            phChInfo = new PhyUmtsChannelData(
                                rlReqInfo->phChId,
                                rlReqInfo->phChType,
                                UMTS_CH_DOWNLINK,
                                aichDesc->sfFactor,
                                aichDesc->chCode,
                                aichDesc->scCode,
                                UMTS_PHY_CH_BRANCH_IQ,
                                1,
                                UMTS_MOD_QPSK,
                                UMTS_CONV_2);

            phChInfo->moreChInfo = (UmtsCphyChDescAICH*)
                                  MEM_malloc(sizeof(UmtsCphyChDescAICH));

            memcpy(phChInfo->moreChInfo,
                   aichDesc,
                   sizeof(UmtsCphyChDescAICH));
            phyDataNodeB->phyChList->push_back(phChInfo);
            if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
            {
                printf("\tNode %d(MAC-PHY): add new AICH\n",
                    node->nodeId);
            }
        }
        else
        {
            memcpy(phChInfo->moreChInfo,
                   aichDesc,
                   sizeof(UmtsCphyChDescAICH));
        }
    }
    else if (rlReqInfo->phChType == UMTS_PHYSICAL_HSPDSCH)
    {
        UmtsCphyChDescHspdsch* dpschDesc;
        dpschDesc = (UmtsCphyChDescHspdsch*) rlReqInfo->phChDesc;

        if (!phChInfo)
        {
            phChInfo = new PhyUmtsChannelData(
                                rlReqInfo->phChId,
                                rlReqInfo->phChType,
                                UMTS_CH_DOWNLINK,
                                dpschDesc->sfFactor,
                                dpschDesc->chCode,
                                dpschDesc->scCode,
                                UMTS_PHY_CH_BRANCH_IQ,
                                1,
                                UMTS_MOD_16QAM,
                                UMTS_TURBO_3);

            phChInfo->moreChInfo = (UmtsCphyChDescHspdsch*)
                                  MEM_malloc(sizeof(UmtsCphyChDescHspdsch));

            memcpy(phChInfo->moreChInfo,
                   dpschDesc,
                   sizeof(UmtsCphyChDescHspdsch));

            // for HS-PDSCH, channels are put in a reverse order
            // to keep the PDUs in order when packing
            phyDataNodeB->phyChList->push_front(phChInfo);
            if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
            {
                printf("\tNode %d(MAC-PHY): add new HS-DPSCH\n",
                    node->nodeId);
            }
        }
        else
        {
            memcpy(phChInfo->moreChInfo,
                   dpschDesc,
                   sizeof(UmtsCphyChDescHspdsch));
        }
    }
    else if (rlReqInfo->phChType == UMTS_PHYSICAL_HSSCCH)
    {
        UmtsCphyChDescHsscch* hsscchDesc;

        hsscchDesc = (UmtsCphyChDescHsscch*) rlReqInfo->phChDesc;
        if (!phChInfo)
        {
            phChInfo = new PhyUmtsChannelData(
                                rlReqInfo->phChId,
                                rlReqInfo->phChType,
                                UMTS_CH_DOWNLINK,
                                hsscchDesc->sfFactor,
                                hsscchDesc->chCode,
                                hsscchDesc->scCode,
                                UMTS_PHY_CH_BRANCH_IQ,
                                1,
                                UMTS_MOD_QPSK,
                                UMTS_CONV_3);

            phChInfo->moreChInfo = (UmtsCphyChDescHsscch*)
                                  MEM_malloc(sizeof(UmtsCphyChDescHsscch));

            memcpy(phChInfo->moreChInfo,
                   hsscchDesc,
                   sizeof(UmtsCphyChDescHsscch));
            phyDataNodeB->phyChList->push_back(phChInfo);
            if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
            {
                printf("\tNode %d(MAC-PHY): add new HS-SCCH\n",
                    node->nodeId);
            }
        }
        else
        {
           memcpy(phChInfo->moreChInfo,
                   hsscchDesc,
                   sizeof(UmtsCphyChDescHsscch));
        }
    }
    else
    {
        // TODO
    }
}

// /**
// FUNCITON   :: UmtsMacPhyNodeBRlReleaseReq
// LAYER      :: UMTS PHY
// PURPOSE    :: Handle Interlayer CPHY-RL-RELEASE-REQ at Ue
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + nodeBMacVar      : UmtsMacNodeBData* : NodeB mac var
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyNodeBRlReleaseReq(Node* node,
                                 UInt32 interfaceIndex,
                                 UmtsMacData* umtsMacData,
                                 UmtsMacNodeBData* nodeBMacVar,
                                 void* cmd)
{
    UmtsCphyRadioLinkRelReqInfo* rlReqInfo = NULL;
    std::list <PhyUmtsChannelData*>* phyChList = NULL;

    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsNodeBData* phyDataNodeB = phyUmts->phyDataNodeB;

    rlReqInfo =  (UmtsCphyRadioLinkRelReqInfo*) cmd;

    if (rlReqInfo->phChDir != UMTS_CH_DOWNLINK)
    {
        // only enlist downlink trCh
        // send L3  setup-CNF to L3
        return;
    }

    phyChList = phyDataNodeB->phyChList;

    std::list<PhyUmtsChannelData*>::iterator it;
    for (it = phyChList->begin();
        it != phyChList->end();
        it ++)
    {
        if ((*it)->phChType == rlReqInfo->phChType &&
            (*it)->channelIndex == rlReqInfo->phChId)
        {
            break;
        }
    }

    if (it != phyChList->end())
    {
        if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
        {
            printf("\tnode %d (MAC_PHY):delete a PhCh from "
                "list with type %d, chDir %d, chId %d SF %d code %d\n",
                node->nodeId, (*it)->phChType, (*it)->chDirection,
                (*it)->channelIndex,
                (*it)->spreadFactor, (*it)->chCodeIndex);
        }
        
        std::list<Message*>::iterator itMsg;

        for (itMsg = (*it)->txMsgList->begin();
            itMsg != (*it)->txMsgList->end();
            itMsg ++)
        {
            if (*itMsg)
            {
                MESSAGE_Free(node, (*itMsg));
            }
        }

        delete (*it);
        phyChList->erase(it);
        // TODO: send L3 CFN
    }
}

//  /**
// FUNCITON   :: UmtsMacPhyNodeBHandleInterLayerCommand
// LAYER      :: UMTS L1 PHY
// PURPOSE    :: Handle Interlayer command at NodeB (CPHY-)
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + cmdType          : UmtsInterlayerCommandType : command type
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyNodeBHandleInterLayerCommand(
            Node* node,
            UInt32 interfaceIndex,
            UmtsInterlayerCommandType cmdType,
            void* cmd)
{
    MacCellularData* macCellularData;
    UmtsMacData* umtsMacData;
    UmtsMacNodeBData* nodeBMacVar;

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;
    umtsMacData =
        (UmtsMacData*)macCellularData->cellularUmtsL2Data->umtsMacData;

    nodeBMacVar = (UmtsMacNodeBData*)umtsMacData->nodeBMacVar;

    if (DEBUG_MAC_PHY_INTERLAYER)
    {
        UmtsLayer2PrintRunTimeInfo(
            node, interfaceIndex, UMTS_LAYER2_SUBLAYER_NONE);
        printf("\t(MAC-PHY): rcvd a interlayer cmd type %d\n",
               cmdType);
    }
    switch(cmdType)
    {
        case UMTS_CPHY_TrCH_CONFIG_REQ:
        {
            UmtsMacPhyNodeBConfigTrCh(node,
                                      interfaceIndex,
                                      umtsMacData,
                                      nodeBMacVar,
                                      cmd);
            break;
        }
        case UMTS_CPHY_TrCH_RELEASE_REQ:
        {
            UmtsMacPhyNodeBReleaseTrCh(node,
                                       interfaceIndex,
                                       umtsMacData,
                                       nodeBMacVar,
                                       cmd);
            break;
        }
        case UMTS_CPHY_RL_SETUP_REQ:
        {
            UmtsMacPhyNodeBRlSetupModifyReq(node,
                                            interfaceIndex,
                                            umtsMacData,
                                            nodeBMacVar,
                                            cmd,
                                            TRUE);

            break;
        }
        case UMTS_CPHY_RL_RELEASE_REQ:
        {
            UmtsMacPhyNodeBRlReleaseReq(node,
                                        interfaceIndex,
                                        umtsMacData,
                                        nodeBMacVar,
                                        cmd);

            break;
        }
        case UMTS_CPHY_RL_MODIFY_REQ:
        {
            break;
        }
        case UMTS_CPHY_COMMIT_REQ:
        {
            break;
        }
        case UMTS_CPHY_OUT_OF_SYNC_CONFIG_REQ:
        {
            break;
        }
        case UMTS_CPHY_MBMS_CONFIG_REQ:
        {
            break;
        }
        default:
        {
            break;
        }
    }
}

// /**
// FUNCTION   :: UmtsMacNodeBGetUeInfoFromPhChInfo
// LAYER      :: UMTS LAYER2 MAC sublayer at UE
// PURPOSE    :: Handle timers out messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + nodeBMacVar      : UmtsMacNodeBData* : NodeB Mac Data
// + phyType          : UmtsPhysicalChannelType : Physical channel type
// + chIndex          : unsigned chat     : channel index
// RETURN     :: void : NULL
// **/
static
PhyUmtsNodeBUeInfo* UmtsMacNodeBGetUeInfoFromPhChInfo
                        (Node* node,
                         UInt32 interfaceIndex,
                         UmtsMacNodeBData* nodeBMacVar,
                         UmtsPhysicalChannelType phType,
                         unsigned char chIndex)
{
    BOOL found = FALSE;

    std::list<UmtsMacNodeBUeInfo*>::iterator ueMacInfoIt;
    std::list <UmtsRbLogTrPhMapping*>::iterator mapListIt;
    PhyUmtsNodeBUeInfo* phyUeInfo = NULL;

    for (ueMacInfoIt = nodeBMacVar->macNodeBUeInfo->begin();
        ueMacInfoIt != nodeBMacVar->macNodeBUeInfo->end();
        ueMacInfoIt ++)
    {
        for (mapListIt = (*ueMacInfoIt)->ueLogTrPhMap->begin();
            mapListIt != (*ueMacInfoIt)->ueLogTrPhMap->end();
            mapListIt ++)
        {
            UmtsRbLogTrPhMapping* mapping;
            mapping = (*mapListIt);
            if (mapping->dlPhChId == chIndex &&
                mapping->dlPhChType == phType)
            {
                found  = TRUE;
                break;
            }
        }
        if (found)
        {
            break;
        }
    }
    if (found == TRUE)
    {

        phyUeInfo = UmtsMacPhyNodeBFindUeInfo(
                        node,
                        interfaceIndex,
                        (*ueMacInfoIt)->priSc);
    }

    return phyUeInfo;
}

// /**
// FUNCTION   :: UmtsMacNodeBTransmitMacBurst
// LAYER      :: UMTS LAYER2 MAC sublayer at UE
// PURPOSE    :: Handle timers out messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + mac              : UmtsMacData*      : Mac Data
// + msg              : Message*          : msg List to send to phy
// RETURN     :: void : NULL
// **/
void UmtsMacNodeBTransmitMacBurst(Node* node,
                                  UInt32 interfaceIndex,
                                  UmtsMacData* mac,
                                  Message* msg)
{
    Message* firstMsg = NULL;
    Message* pschMsg = NULL;
    Message* sschMsg  = NULL;
    Message* cpichMsg = NULL;
    clocktype duration = 0;


    PhyUmtsCdmaBurstInfo burstInfo;
    std::list <PhyUmtsChannelData*>* phyChList;
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    clocktype currentTime = node->getNodeTime();

    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;

    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsNodeBData* phyDataNodeB =
        (PhyUmtsNodeBData*) phyUmts->phyDataNodeB;


    if (DEBUG_MAC_PHY_RX_TX)
    {
        printf("Node %d MAC-PHY: node B  TX a msg to Phy\n", node->nodeId);
    }

    if (msg != NULL)
    {
        firstMsg = msg;
    }

    // create P-SCH signal
    pschMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node,
                        pschMsg,
                        sizeof(UInt16)* 16,
                        TRACE_CELLULAR);
    memset((UInt16*)MESSAGE_ReturnPacket(pschMsg),
            UMTS_PSCH_CODE,
            16);
    
    // For simplicity, we use nodeId here to distigush
    // the physical signal, in stead of simulating the
    // match filter

    phyChList = phyDataNodeB->phyChList;

    memset(&burstInfo, 0, sizeof(PhyUmtsCdmaBurstInfo));

    UmtsPhyCreateCdmaBurstInfo(node,
                               interfaceIndex,
                               phyChList,
                               UMTS_PHYSICAL_PSCH,
                               1,
                               currentTime,
                               currentTime + mac->chipDuration * 256, 
                                                          // if 3.84M
                               &burstInfo);

    UmtsPhyAddCdmaBurstInfo(node, pschMsg, &burstInfo);

    pschMsg->next = firstMsg;
    firstMsg = pschMsg;

    duration = mac->slotDuration; // TODO

    // create S-SCH signal
    sschMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node,
                        sschMsg,
                        sizeof(UInt16)* 16,
                        TRACE_CELLULAR);

    // for simiplicity use SSC index instead of concrete bit sequence
    memset((UInt16*)MESSAGE_ReturnPacket(pschMsg),
            phyDataNodeB->sscCodeSeq[phyUmts->slotIndex],
            16);

    memset(&burstInfo, 0, sizeof(PhyUmtsCdmaBurstInfo));

    UmtsPhyCreateCdmaBurstInfo(node,
                               interfaceIndex,
                               phyChList,
                               UMTS_PHYSICAL_SSCH,
                               1,
                               currentTime,
                               currentTime + mac->chipDuration * 256,
                               &burstInfo);

    UmtsPhyAddCdmaBurstInfo(node, sschMsg, &burstInfo);

    sschMsg->next = firstMsg;
    firstMsg = sschMsg;

    // create CPICH signal
    cpichMsg = MESSAGE_Alloc(node, 0, 0, 0);
    // no packet field  is allocated in CPICH

    memset(&burstInfo, 0, sizeof(PhyUmtsCdmaBurstInfo));

    UmtsPhyCreateCdmaBurstInfo(node,
                               interfaceIndex,
                               phyChList,
                               UMTS_PHYSICAL_CPICH,
                               1,
                               currentTime,
                               currentTime + mac->slotDuration,
                               &burstInfo);

    UmtsPhyAddCdmaBurstInfo(node, cpichMsg, &burstInfo);

    cpichMsg->next = firstMsg;
    firstMsg = cpichMsg;

    // create AICH msg if needed
    if (phyDataNodeB->prachAichInfo.aichMsgNeeded)
    {
        int i;
        Message* aichMsg;
        char aichInd[16];

        // process the sigRcvInfo
        for (i = 0; i < 16; i ++)
        {
            if (phyDataNodeB->prachAichInfo.sigRcvInfo[i] ==
                PHY_UMTS_SIG_SUCC)
            {
                aichInd[i] = 1;
            }
            else if (phyDataNodeB->prachAichInfo.sigRcvInfo[i] ==
                     PHY_UMTS_SIG_ERROR)
            {
                aichInd[i] = -1;
            }
            else
            {
                aichInd[i] = 0;
            }
        }

        aichMsg = MESSAGE_Alloc(node, 0, 0, 0);
        MESSAGE_PacketAlloc(node,
                            aichMsg,
                            sizeof(char)* 16,
                            TRACE_CELLULAR);

        memcpy((char*)MESSAGE_ReturnPacket(aichMsg),
               &(aichInd[0]),
               16);

        memset(&burstInfo, 0, sizeof(PhyUmtsCdmaBurstInfo));

        UmtsPhyCreateCdmaBurstInfo(node,
                                   interfaceIndex,
                                   phyChList,
                                   UMTS_PHYSICAL_AICH,
                                   1,
                                   currentTime,
                                   currentTime + mac->slotDuration,
                                   &burstInfo);

        // TODO
        burstInfo.preSlot = phyDataNodeB->prachAichInfo.rcvdSlotIndex;
        UmtsPhyAddCdmaBurstInfo(node, aichMsg, &burstInfo);
        
        if (DEBUG_MAC_PHY_RACH)
        {
            printf("node %d: send aich in slot %d\n",
                node->nodeId, phyUmts->slotIndex);
        }

        aichMsg->next = firstMsg;
        firstMsg = aichMsg;
    }

    //  create other PHY Ch msgs and add to the msg list;
    std::list<PhyUmtsChannelData*>::iterator it;
    for (it = phyChList->begin();
         it != phyChList->end();
         it ++)
    {
        if ((*it)->phChType != UMTS_PHYSICAL_PSCH &&
            (*it)->phChType != UMTS_PHYSICAL_SSCH &&
            (*it)->phChType != UMTS_PHYSICAL_CPICH &&
            (*it)->phChType != UMTS_PHYSICAL_AICH)
        {
            BOOL ulPowCtrlCmdNeeded = FALSE;
            PhyUmtsPowerControlCommand powCtrlCmd = 
                UMTS_POWER_CONTROL_CMD_INVALID;

            PhyUmtsNodeBUeInfo* ueInfo = NULL;

            if((*it)->phChType == UMTS_PHYSICAL_DPDCH)
            {
                ueInfo = UmtsMacNodeBGetUeInfoFromPhChInfo
                              (node, interfaceIndex, mac->nodeBMacVar,
                              (*it)->phChType, (*it)->channelIndex);

                if (ueInfo && ueInfo->needUlPowerControl)
                {
                    ulPowCtrlCmdNeeded = TRUE;
                    powCtrlCmd = ueInfo->ulPowCtrlCmd;
                    ueInfo->needUlPowerControl = FALSE;
                    if (DEBUG_POWER)
                    {
                        UmtsLayer2PrintRunTimeInfo(
                            node,
                            interfaceIndex,
                            UMTS_LAYER2_SUBLAYER_NONE);
                        printf("\n\tneed to request UE %d adjust "
                            "UL Tx Power \n", ueInfo->ueId);
                    }
                }
            }
            if (!(*it)->txMsgList->empty() ||
                (((*it)->phChType == UMTS_PHYSICAL_DPDCH &&
                ulPowCtrlCmdNeeded) ||
                (*it)->phChType == UMTS_PHYSICAL_HSSCCH ))
            {
                    Message* otherMsg = NULL;
                    BOOL dpcchOnly = FALSE;

                    if (!(*it)->txMsgList->empty())
                    {
                        otherMsg = UmtsPackMessage(
                                   node,
                                   *((*it)->txMsgList),
                                   TRACE_CELLULAR);

                        (*it)->txMsgList->clear();
                    }
                    else if ((*it)->phChType == UMTS_PHYSICAL_DPDCH &&
                            ulPowCtrlCmdNeeded)
                    {
                        otherMsg = MESSAGE_Alloc(node, 0, 0, 0);
                        MESSAGE_PacketAlloc(
                            node, otherMsg, 0, TRACE_UMTS_PHY);
                        dpcchOnly = TRUE;
                    }
                    else if ((*it)->phChType == UMTS_PHYSICAL_HSSCCH)
                    {
                        // create a HS-SCCH signal
                        otherMsg = MESSAGE_Alloc(node, 0, 0, 0);
                        MESSAGE_PacketAlloc(
                            node, otherMsg, 0, TRACE_UMTS_PHY);

                        if (DEBUG_MAC_PHY_HSDPA && 0)
                        {
                            printf("\n\t create HS_SCCH packets\n");
                        }
                    }
                    else
                    {
                        // no more
                    }

                    memset(&burstInfo, 0, sizeof(PhyUmtsCdmaBurstInfo));

                    UmtsPhyCreateCdmaBurstInfo(node,
                               interfaceIndex,
                               phyChList,
                               (*it)->phChType,
                               (*it)->channelIndex,
                               currentTime,
                               currentTime + mac->slotDuration,
                               &burstInfo);

                    if (ulPowCtrlCmdNeeded)
                    {
                        if (powCtrlCmd == UMTS_POWER_CONTROL_CMD_DEC)
                        {
                            burstInfo.pcCmd = 
                                (signed char)(-1 * ueInfo->ulPowCtrlSteps);
                        }
                        else if (powCtrlCmd == UMTS_POWER_CONTROL_CMD_INC)
                        {
                            burstInfo.pcCmd = 
                                (signed char)(1 * ueInfo->ulPowCtrlSteps);
                        }
                        if (DEBUG_POWER)
                        {
                            printf("\n\tUE %d need %d power "
                                "UL Tx Power \n",
                                ueInfo->ueId, burstInfo.pcCmd);
                        }
                    }
                    if (dpcchOnly)
                    {
                        burstInfo.dlDpcchOnly = TRUE;
                        if (DEBUG_POWER)
                        {
                            printf("\n\t this PowCtrl for UE %d is "
                                "via a DL DPCCH only\n",
                                ueInfo->ueId);
                        }
                    }

                    // for HSPDSCH set UeId
                    if ((*it)->phChType == UMTS_PHYSICAL_HSPDSCH)
                    {
                        burstInfo.hsdpaUeId = (*it)->targetNodeId;
                        if (DEBUG_MAC_PHY_HSDPA)
                        {
                            printf("\n\t HS-PDSCH Tx to UE %d chCode %d\n",
                                burstInfo.hsdpaUeId, burstInfo.chCodeIndex);
                        }
                    }

                    UmtsPhyAddCdmaBurstInfo(
                        node,
                        otherMsg,
                        &burstInfo);
                    ERROR_Assert(otherMsg != NULL, "msg is NULL");

                    otherMsg->next = firstMsg;
                    firstMsg = otherMsg;

                    // TODO: update the duration to the max one
            }
        }
    }

    // pack the msg list into one msg, add phy header
    firstMsg = UmtsPackMessage(node, firstMsg, TRACE_CELLULAR);

    // set transmission channel
    PHY_SetTransmissionChannel(
        node,
        node->macData[interfaceIndex]->phyNumber,
        mac->currentDLChannel);

    // transmit the signal
    // it should be set as the logest burst duration

    PHY_StartTransmittingSignal(
            node,
            node->macData[interfaceIndex]->phyNumber,
            firstMsg,
            duration,
            TRUE,
            UMTS_DELAY_UNTIL_SIGNAL_AIRBORN);

    // reset active HSDPA ch
    if (phyDataNodeB->hsdpaConfig.numActiveCh)
    {
        phyDataNodeB->hsdpaConfig.numActiveCh = 0;
        if (0)
        {
            UmtsLayer2PrintRunTimeInfo(
                node, interfaceIndex, UMTS_LAYER2_SUBLAYER_MAC);
            printf("\n\t tx HSDPA data\n");
        }
    }

    // increase the slot index
    phyUmts->slotIndex = (phyUmts->slotIndex + 1 ) %
                          UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME;

    // update PRACH/AICH
    if (phyDataNodeB->prachAichInfo.aichMsgNeeded)
    {
        memset((void*)&phyDataNodeB->prachAichInfo,
               0,
               sizeof(PhyUmtsNodeBPrachAichInfo));
    }
}
//-------------------------------------------------------------------------
// handle function for Ue
//-------------------------------------------------------------------------
// /**
// FUNCTION   :: UmtsMacPhyUeHandlePschMsg
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer.
//               The PHY sublayer will first handle it
// PARAMETERS ::
// + node             : Node*          : Pointer to node.
// + phyDataUe        : PhyUmtsUeData* : UE PHY data
// + msg              : Message*       : PSCH Packet received from PHY
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyUeHandlePschMsg(Node* node,
                            PhyUmtsUeData* phyDataUe,
                            Message* msg)
{
    UInt16* psscSeq;
    PhyUmtsCdmaBurstInfo* burstInfo;
    burstInfo = (PhyUmtsCdmaBurstInfo*) MESSAGE_ReturnInfo(msg);
    psscSeq = (UInt16*)MESSAGE_ReturnPacket(msg);

    NodeAddress syncTimeInfo;
    syncTimeInfo = burstInfo->syncTimeInfo;


    if (!phyDataUe->ueCellSearch.signalLocked &&
        phyDataUe->cellSearchNeeded)
    {
        // determine whether this sync signal has been detected before
        // update detect list, active lsit and monitor list
        std::vector <NodeAddress>::iterator it;

        for(it = phyDataUe->syncTimeInfo->begin();
            it != phyDataUe->syncTimeInfo->end();
            it ++)
        {
            if (*it == syncTimeInfo)
            {
                break;
            }
        }
        if (it == phyDataUe->syncTimeInfo->end())
        {
            NodeAddress newInfo;
            newInfo = syncTimeInfo;
            phyDataUe->syncTimeInfo->push_back(newInfo);
            phyDataUe->ueCellSearch.signalLocked = TRUE;
            phyDataUe->ueCellSearch.syncTimeInfo = syncTimeInfo;
            if (DEBUG_MAC_PHY_RX_TX)
            {
                printf("node %d (MAC-PHY): detect a phy signal with new "
                    "synchronization info %d\n",
                    node->nodeId,  newInfo);
            }
        }
    }

    if (phyDataUe->cellSearchNeeded &&
        phyDataUe->ueCellSearch.signalLocked &&
        phyDataUe->ueCellSearch.syncTimeInfo == syncTimeInfo &&
        !phyDataUe->ueCellSearch.ueSync)
    {
        phyDataUe->ueCellSearch.slotCount =
            (phyDataUe->ueCellSearch.slotCount + 1) %
            UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME;

        // after recv 15 slot
        if (phyDataUe->ueCellSearch.slotCount == 0)
        {
            phyDataUe->ueCellSearch.ueSync = TRUE;
            if (DEBUG_MAC_PHY_RX_TX)
            {
                printf("node %d (MAC-PHY): synchronized with syncInfo %d\n",
                    node->nodeId, syncTimeInfo);
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsMacPhyUeHandleAichMsg
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer.
//               The PHY sublayer will first handle it
// PARAMETERS ::
// + node             : Node*          : Pointer to node.
// + interfaceIndex   : UInt32         : interface Index
// + phyDataUe        : PhyUmtsUeData* : UE PHY data
// + msg              : Message*       : CPICH Packet received from PHY
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyUeHandleAichMsg(Node* node,
                               UInt32 interfaceIndex,
                               PhyUmtsUeData* phyDataUe,
                               Message* msg)
{
    PhyUmtsCdmaBurstInfo* burstInfo;
    char* aichInd;
    BOOL needIndMac = FALSE;
    UmtsPhyAccessCnfInfo cnfInfo;

    burstInfo = (PhyUmtsCdmaBurstInfo*) MESSAGE_ReturnInfo(msg);

    aichInd = (char*)MESSAGE_ReturnPacket(msg);
    if (phyDataUe->raInfo &&
        burstInfo->preSlot == phyDataUe->raInfo->preambleSlot)
    {
        if(aichInd[phyDataUe->raInfo->preambleSig] == 1)
        {
           cnfInfo.cnfType = UMTS_PHY_ACCESS_CFN_ACK;
           phyDataUe->prachSucPower =
               phyDataUe->raInfo->cmdPrePower;
           needIndMac = TRUE;
           phyDataUe->raInfo->waitForAich = FALSE;

           if (DEBUG_MAC_PHY_RACH)
           {
               printf("node %d: rcvd aich with sig %d slot %d SUCC\n",
                   node->nodeId, phyDataUe->raInfo->preambleSig,
                   burstInfo->preSlot);
           }

        }
        else if(aichInd[phyDataUe->raInfo->preambleSig] != 1)
        {
            // Original is for case -1, did not consider case 0
            cnfInfo.cnfType = UMTS_PHY_ACCESS_CFN_NACK;
            needIndMac = TRUE;
            phyDataUe->raInfo->waitForAich = FALSE;
            if (DEBUG_MAC_PHY_RACH)
            {
               printf("node %d: rcvd aich with sig %d slot %d UNSUC\n",
                   node->nodeId, phyDataUe->raInfo->preambleSig,
                   burstInfo->preSlot);
            }

            // exit the procedure
            if (phyDataUe->raInfo)
            {
                MEM_free(phyDataUe->raInfo);
                phyDataUe->raInfo = NULL;
            }
        }
    }

    if (needIndMac)
    {
        if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_RACH)
        {
            printf("node %d send a PHY ACCESS_CFN to MAC layer\n",
                node->nodeId);
        }

        UmtsMacHandleInterLayerCommand(
            node, interfaceIndex, UMTS_PHY_ACCESS_CNF, (void*)&cnfInfo);
    }
}

// /**
// FUNCTION   :: UmtsMacPhyUeHandleCpichMsg
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer.
//               The PHY sublayer will first handle it
// PARAMETERS ::
// + node             : Node*          : Pointer to node.
// + interfaceIndex   : UInt32         : interface Index
// + phyDataUe        : PhyUmtsUeData* : UE PHY data
// + msg              : Message*       : CPICH Packet received from PHY
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyUeHandleCpichMsg(Node* node,
                                UInt32 interfaceIndex,
                                PhyUmtsUeData* phyDataUe,
                                Message* msg)
{
    PhyUmtsCdmaBurstInfo* burstInfo;
    PhyUmtsUeNodeBInfo* ueNodeBInfo = NULL;
    BOOL reportL3 = FALSE;

    burstInfo = (PhyUmtsCdmaBurstInfo*) MESSAGE_ReturnInfo(msg);

    // update detect list, active lsit and monitor list
    std::list<PhyUmtsUeNodeBInfo*>::iterator it;

    for(it = phyDataUe->ueNodeBInfo->begin();
        it != phyDataUe->ueNodeBInfo->end();
        it ++)
    {
        if((*it)->primaryScCode == burstInfo->scCodeIndex)
        {
            // has already been detected
            if (DEBUG_MAC_PHY_RX_TX)
            {
                printf("Node %d (MAC-PHY): It is a detected cell "
                       "with pri SC code %d in code group %d\n",
                       node->nodeId,
                       (*it)->primaryScCode,
                       (*it)->sscCodeGroupIndex);
            }
            ueNodeBInfo = *it;
            break;
        }
    }

    // include the one for cell search
    if ((phyDataUe->cellSearchNeeded &&
        phyDataUe->ueCellSearch.ueSync &&
        phyDataUe->ueCellSearch.scGroupDetected &&
        phyDataUe->ueCellSearch.signalLocked &&
        burstInfo->syncTimeInfo == phyDataUe->ueCellSearch.syncTimeInfo  &&
        !phyDataUe->ueCellSearch.priScDetected))
    {
        // THis is to detect new cell
        phyDataUe->ueCellSearch.priScDetected = TRUE;

        // reset the cell search info for next detected NodeB
        memset(&phyDataUe->ueCellSearch, 0, sizeof (UeCellSearchInfo));

        if (it == phyDataUe->ueNodeBInfo->end())
        {
            // a new detected NodeB, cell
            ueNodeBInfo = new PhyUmtsUeNodeBInfo(burstInfo->scCodeIndex);
            ueNodeBInfo->cpichRscp = IN_DB(burstInfo->burstPower_mW);
            ueNodeBInfo->cpichEcNo = burstInfo->cinr;
            ueNodeBInfo->cellStatus = UMTS_CELL_DETECTED_SET;
            ueNodeBInfo->synchronized = TRUE;
            phyDataUe->ueNodeBInfo->push_back(ueNodeBInfo);
            reportL3 = TRUE;
            ueNodeBInfo->lastReportTime = node->getNodeTime();
            if (DEBUG_MAC_PHY_RX_TX)
            {
                printf("Node %d (MAC-PHY): detect a new cell "
                       "with pri SC code %d in code group %d\n",
                       node->nodeId,
                       ueNodeBInfo->primaryScCode,
                       ueNodeBInfo->sscCodeGroupIndex);
            }
        }
    }
    else if (ueNodeBInfo)
    {
        ueNodeBInfo->cpichRscp = IN_DB(burstInfo->burstPower_mW);
        ueNodeBInfo->cpichEcNo = burstInfo->cinr;

        if (ueNodeBInfo->synchronized &&
            (ueNodeBInfo->lastReportTime + 1 * SECOND) <  node->getNodeTime())
        {
            // for cell already report before
            // which means time/fram/freq syncornized has been done
            // report signal strength to L3 if event trigger or periodical
            reportL3 = TRUE;
            ueNodeBInfo->lastReportTime = node->getNodeTime();
        }
    }

    if (reportL3)
    {
        UmtsMeasCpichRscp rscp;
        UmtsCphyMeasIndInfo measInd;
        rscp.measVal = IN_DB(burstInfo->burstPower_mW);
        rscp.priSc = burstInfo->scCodeIndex;
        measInd.measType = UMTS_MEASURE_L1_CPICH_RSCP;
        measInd.measInfo = (void*)(&rscp);
        UmtsLayer3HandleInterLayerCommand(node,
                                          interfaceIndex,
                                          UMTS_CPHY_MEASUREMENT_IND,
                                          (void*)(&measInd));
        
        //  report EcNo as well, be sure to put Ec_No after
        //  RSCP
        rscp.measVal = burstInfo->cinr;
        measInd.measType = UMTS_MEASURE_L1_CPICH_Ec_No;
        UmtsLayer3HandleInterLayerCommand(node,
                                          interfaceIndex,
                                          UMTS_CPHY_MEASUREMENT_IND,
                                          (void*)(&measInd));
    }
}

// /**
// FUNCTION   :: UmtsMacPhyUeHandlePowerControlCmd
// LAYER      :: MAC-PHY
// PURPOSE    :: Receive a power control command from network
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + interfaceIndex   : int          : Interface Index
// + phyDataUe        : PhyUmtsUeData* : Phy Data at UE
// + priSc            : unsigned int : primary scrambiling code
// + pcCmd            : signed char  : power control cmd
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyUeHandlePowerControlCmd(Node* node,
                                       UInt32 interfaceIndex,
                                       PhyUmtsUeData* phyDataUe,
                                       unsigned int nodeBPriSc,
                                       signed char pcCmd)
{
    PhyUmtsUeNodeBInfo* ueNodeBInfo = NULL;

    std::list<PhyUmtsUeNodeBInfo*>::iterator it;

    for(it = phyDataUe->ueNodeBInfo->begin();
        it != phyDataUe->ueNodeBInfo->end();
        it ++)
    {
        if((*it)->primaryScCode == nodeBPriSc)
        {
            ueNodeBInfo = *it;
            break;
        }
    }
    ERROR_Assert(it != phyDataUe->ueNodeBInfo->end(),
                 " no UeNodeBInfo link with this priSc");

    if (!ueNodeBInfo->ulPowCtrl->needAdjustPower)
    {
        ueNodeBInfo->ulPowCtrl->needAdjustPower = TRUE;
        ueNodeBInfo->ulPowCtrl->deltaPower =
            ueNodeBInfo->ulPowCtrl->stepSize * pcCmd;
        if (DEBUG_POWER )
        {
            UmtsLayer2PrintRunTimeInfo(
                node, interfaceIndex, UMTS_LAYER2_SUBLAYER_NONE);
            printf("\n\tHandle PowCtrl Cmd from cell(peisc) %d,"
                "DeltaPower is  %d\n",
                nodeBPriSc, ueNodeBInfo->ulPowCtrl->deltaPower);
        }
    }
}

// /**
// FUNCTION   :: UmtsMacPhyUeReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer.
//               The PHY sublayer will first handle it
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + interfaceIndex   : int          : Interface Index
// + msg              : Message*     : Packet received from PHY
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyUeReceivePacketFromPhy(Node* node,
                                      int interfaceIndex,
                                      Message* msg)
{
    PhyUmtsCdmaBurstInfo* burstInfo;
    Message* tmpMsg;

    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsUeData* phyDataUe = phyUmts->phyDataUe;

    if (DEBUG_MAC_PHY_RX_TX)
    {
        UmtsLayer2PrintRunTimeInfo(
            node, interfaceIndex, UMTS_LAYER2_SUBLAYER_NONE);
        printf("\n");
    }
    while(msg != NULL)
    {
        tmpMsg = msg;
        msg = msg->next;

        burstInfo = (PhyUmtsCdmaBurstInfo*) MESSAGE_ReturnInfo(tmpMsg);

        if (!UmtsMacPhyIsThisBurstForMe(
                     node,
                     interfaceIndex,
                     (void*)phyDataUe,
                     burstInfo->phChType,
                     burstInfo->scCodeIndex,
                     burstInfo->chCodeIndex,
                     burstInfo->spreadFactor))
        {
            if (DEBUG_MAC_PHY_RX_TX)
            {
                if (burstInfo->phChType == UMTS_PHYSICAL_PCCPCH)
                {
                    printf("UE: %d rcvs a PCCPCH burst not for me\n",
                        node->nodeId);
                }
            }

            MESSAGE_Free(node, tmpMsg);
        }
        else if (burstInfo->inError)
        {
            if ( DEBUG_MAC_PHY_RX_TX )
            {
                if (burstInfo->phChType == UMTS_PHYSICAL_DPDCH)
                {
                    printf("UE: %d rcvs a DPDCH burst in error\n",
                        node->nodeId);
                }
            }
            if (burstInfo->phChType == UMTS_PHYSICAL_DPDCH)
            {
                // out loop power control
                // currently SIRtarget is set to a minimum value
                // adjust SIR DL target
                // phyDataUe->targetDlSir decrease

            }
            if ( DEBUG_MAC_PHY_RX_TX || DEBUG_MAC_PHY_HSDPA)
            {
                if (burstInfo->phChType == UMTS_PHYSICAL_HSPDSCH)
                {
                    if (burstInfo->hsdpaUeId == node->nodeId)
                    printf("UE: %d rcvd a HS-PDSCH burst with "
                        "chCode %d for me in error\n",
                        node->nodeId, burstInfo->chCodeIndex );
                }
            } 
            MESSAGE_Free(node, tmpMsg);
        }
        else
        {
            switch(burstInfo->phChType)
            {
                case UMTS_PHYSICAL_PSCH:
                {
                    UmtsMacPhyUeHandlePschMsg(node, phyDataUe, tmpMsg);
                    MESSAGE_Free(node, tmpMsg);

                    break;
                }
                case UMTS_PHYSICAL_SSCH:
                {
                    // do nothing right now
                    UInt16* sscSeq;
                    sscSeq = (UInt16*)MESSAGE_ReturnPacket(tmpMsg);

                    NodeAddress syncTimeInfo;
                    syncTimeInfo = burstInfo->syncTimeInfo;

                    if (phyDataUe->cellSearchNeeded &&
                        phyDataUe->ueCellSearch.signalLocked &&
                        phyDataUe->ueCellSearch.syncTimeInfo ==
                            syncTimeInfo &&
                        phyDataUe->ueCellSearch.ueSync &&
                        !phyDataUe->ueCellSearch.scGroupDetected)
                    {
                        phyDataUe->ueCellSearch.secScInfo[phyDataUe->
                            ueCellSearch.slotCount %
                            UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME] =
                            sscSeq[0];

                        phyDataUe->ueCellSearch.slotCount =
                            (phyDataUe->ueCellSearch.slotCount + 1) %
                            UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME;

                        if (phyDataUe->ueCellSearch.slotCount == 0)
                        {
                            // to see if we can detect the group
                            // for similicity, skip here
                            phyDataUe->ueCellSearch.scGroupDetected = TRUE;
                            if (DEBUG_MAC_PHY_RX_TX)
                            {
                                printf("Node %d (MAC-PHY):the group index "
                                       "is decoded for timeSynInfo %d\n",
                                       node->nodeId, syncTimeInfo);
                            }
                        }
                    }
                    MESSAGE_Free(node, tmpMsg);

                    break;
                }
                case UMTS_PHYSICAL_AICH:
                {
                    if (phyDataUe->raInfo && phyDataUe->raInfo->waitForAich)
                    {

                        if (DEBUG_MAC_PHY_RACH)
                        {
                            printf("node %d: rcvd AICH in slot %d\n",
                                node->nodeId, phyUmts->slotIndex);
                        }

                        UmtsMacPhyUeHandleAichMsg(
                            node, interfaceIndex, phyDataUe, tmpMsg);
                        MESSAGE_Free(node, tmpMsg);

                        break;
                    }
                    else
                    {
                        MESSAGE_Free(node, tmpMsg);

                        break;
                    }
                }
                case UMTS_PHYSICAL_CPICH:
                {
                    UmtsMacPhyUeHandleCpichMsg(
                        node, interfaceIndex, phyDataUe, tmpMsg);
                    MESSAGE_Free(node, tmpMsg);

                    break;
                }
                case UMTS_PHYSICAL_PCCPCH:
                {
                    std::list<Message*> msgList;

                    if (DEBUG_MAC_PHY_RX_TX)
                    {
                        printf("rcvs a burst from PCCPCH\n");
                    }
                    UmtsUnpackMessage(node, tmpMsg, FALSE, TRUE, msgList);
                    UmtsRlcReceivePduFromMac(
                        node, interfaceIndex, UMTS_BCCH_RB_ID,
                        msgList, FALSE);

                    break;
                }
                case UMTS_PHYSICAL_SCCPCH:
                {
                    std::list<Message*> msgList;

                    // could be PCCH_RB on PCCH or CCCH_RB on FACH
                    if (DEBUG_MAC_PHY_RX_TX )
                    {
                        printf("node: %d rcvs a burst from SCCPCH\n", 
                            node->nodeId);
                    }
                    UmtsUnpackMessage(node, tmpMsg, FALSE, TRUE, msgList);
                    UmtsRlcReceivePduFromMac(
                        node, interfaceIndex,
                        msgList, FALSE);

                    break;
                }
                case UMTS_PHYSICAL_DPDCH:
                {
                    std::list<Message*> msgList;

                    if (DEBUG_MAC_PHY_RX_TX )
                    {
                        printf("UE: %d rcvs a burst from DPDCH\n",
                            node->nodeId);
                    }
                    // handle power control info first
                    if (burstInfo->pcCmd != 0)
                    {
                        UmtsMacPhyUeHandlePowerControlCmd(
                            node,
                            interfaceIndex,
                            phyDataUe,
                            burstInfo->scCodeIndex,
                            burstInfo->pcCmd);
                    }
                    if (burstInfo->dlDpcchOnly)
                    {
                        MESSAGE_Free(node, tmpMsg);
                    }
                    else
                    {
                        BOOL errorIndi = FALSE;

                        // check if it is for this Ue
                        UmtsUnpackMessage(
                            node, tmpMsg, FALSE, TRUE, msgList);

                        //check if there is a missing packet in the list
                        UmtsRlcReceivePduFromMac(
                            node,
                            interfaceIndex,
                            msgList,
                            errorIndi);
                    }
                    break;
                }
                case UMTS_PHYSICAL_DPCCH:
                {
                    if (DEBUG_MAC_PHY_RX_TX)
                    {
                        printf("rcvs a burst from DPCCH\n");
                    }

                    // handle power control info first
                    if (burstInfo->pcCmd != 0)
                    {
                        UmtsMacPhyUeHandlePowerControlCmd(
                            node,
                            interfaceIndex,
                            phyDataUe,
                            burstInfo->scCodeIndex,
                            burstInfo->pcCmd);
                    }

                    MESSAGE_Free(node, tmpMsg);

                    break;
                }
                case UMTS_PHYSICAL_HSPDSCH:
                {
                    std::list<Message*> msgList;

                    if (burstInfo->hsdpaUeId == node->nodeId)
                    {
                        unsigned int priScCode;

                        if (DEBUG_MAC_PHY_RX_TX ||
                            DEBUG_MAC_PHY_HSDPA || 0)
                        {
                            printf("\n\trcvs a burst from HS-PDSCH "
                                "for me from chCode%d\n",
                                burstInfo->chCodeIndex);
                        }

                        priScCode = 
                            ((unsigned int)(burstInfo->scCodeIndex /
                                            16)) * 16;
                        
                        // update serving NodeB priScCode
                        if (!phyDataUe->hsdpaInfo.rcvdHspdschSig ||
                            phyDataUe->hsdpaInfo.ServHsdpaNodeBPriScCode !=
                            priScCode)
                        {
                            // for the first HSDPSCH signal or
                            // from  new serving nodeB
                            if (!phyDataUe->hsdpaInfo.rcvdHspdschSig)
                            {
                               phyDataUe->hsdpaInfo.rcvdHspdschSig = TRUE;
                            }
                            phyDataUe->hsdpaInfo.ServHsdpaNodeBPriScCode =
                                priScCode;
                        }

                        // check if it is for this Ue
                        UmtsUnpackMessage(
                            node, tmpMsg, FALSE, TRUE, msgList);

                        BOOL errorIndi = FALSE;
                       
                        //check if there is a missing packet in the list
                        UmtsRlcReceivePduFromMac(
                            node,
                            interfaceIndex,
                            msgList,
                            errorIndi);
                    }
                    else
                    {
                        if (DEBUG_MAC_PHY_RX_TX ||
                            (0 && DEBUG_MAC_PHY_HSDPA))
                        {
                            printf("rcvs a burst from "
                                "HS-PDSCH not for me\n");
                        }
                        MESSAGE_Free(node, tmpMsg);
                    }

                    break;
                }
                case UMTS_PHYSICAL_HSSCCH:
                {
                    if (DEBUG_MAC_PHY_RX_TX ||
                        (0 && DEBUG_MAC_PHY_HSDPA))
                    {
                        printf("rcvs a burst from HS-SCCH\n");
                    }

                    MESSAGE_Free(node, tmpMsg);

                    break;
                }
                default:
                {
                    // TODO: forward the tmMsg to RLC
                    MESSAGE_Free(node, tmpMsg);
                }
            } // switch(burstInfo->phChType)
        } // if (!UmtsMacPhyIsThisBurstForMe)
    } // while
    // 1. get the PHY layer related info, power control...
    // 2 .process those msgs from SCH, PICH,...AICH without Tr CH mapping
    // 3 .then pass the rest to the RLC
}

//  /**
// FUNCITON   :: UmtsMacPhyUeHandleAccessReq
// LAYER      :: UMTS PHY
// PURPOSE    :: Handle Interlayer CPHY-TrCh-CONFIG-REQ at Ue
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + ueMacVar         : UmtsMacUeData*    : ue mac var
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyUeHandleAccessReq(Node* node,
                                 UInt32 interfaceIndex,
                                 UmtsMacData* umtsMacData,
                                 UmtsMacUeData* ueMacVar,
                                 void* cmd)
{
    UmtsPhyAccessReqInfo* accessReqInfo;

    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsUeData* phyDataUe = phyUmts->phyDataUe;

    phyDataUe->raInfo =
        (UmtsPhyRandomAccessInfo*)
         MEM_malloc(sizeof(UmtsPhyRandomAccessInfo));

    memset(phyDataUe->raInfo, 0, sizeof(UmtsPhyRandomAccessInfo));

    accessReqInfo = (UmtsPhyAccessReqInfo*)cmd;

    phyDataUe->raInfo->ascIndex = accessReqInfo->ascIndex;
    phyDataUe->raInfo->assignedSubCh = accessReqInfo->subChAssigned;
    phyDataUe->raInfo->retryCount = accessReqInfo->maxRetry;
    phyDataUe->raInfo->retryMax = accessReqInfo->maxRetry;
    phyDataUe->raInfo->sigStartIndex = accessReqInfo->sigStartIndex;
    phyDataUe->raInfo->sigEndIndex = accessReqInfo->sigEndIndex;
    phyDataUe->raInfo->cmdPrePower = PHY_UMTS_DEFAULT_TX_POWER_dBm;

    RandomDistribution<Int32> randomInt;
    randomInt.setSeed(phyCellular->randSeed[1]);
    randomInt.setDistributionUniformInteger(
        phyDataUe->raInfo->sigStartIndex,
        phyDataUe->raInfo->sigEndIndex);
    phyDataUe->raInfo->preambleSig = 
        (unsigned char)(randomInt.getRandomNumber());

    // TODO: get the slot index in the frame
    phyDataUe->raInfo->preambleSlot =
        (unsigned char)(RANDOM_erand(phyCellular->randSeed) *
        UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME);

    // FOR PRACH/AICH, one lsot = 2 regular slot
    if (phyDataUe->raInfo->preambleSlot > 7 ||
        (phyDataUe->raInfo->preambleSlot <= 7 &&
        (phyUmts->slotIndex % UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME
        < phyDataUe->raInfo->preambleSlot * 2)))
    {
        phyDataUe->raInfo->rachPreambleWaitTime =
            phyDataUe->raInfo->preambleSlot * 2 -
            (phyUmts->slotIndex % UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME);

    }
    else
    {
        // have to send in next frame
        phyDataUe->raInfo->rachPreambleWaitTime =
            phyDataUe->raInfo->preambleSlot * 2 +
            (UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME -
            phyUmts->slotIndex % UMTS_DEFAULT_SLOT_NUM_PER_RADIO_FRAME);
    }

    phyDataUe->raInfo->needSendRachPreamble = TRUE;
    phyDataUe->raInfo->waitForAich = FALSE;

    if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_RACH)
    {
        UmtsLayer2PrintRunTimeInfo(
            node, interfaceIndex, UMTS_LAYER2_SUBLAYER_NONE);
        printf("rcvd access req, need to send RACH "
               "preamble at select slot %d with sig %d, "
               "curSlotIndex %d need to wait %d slot\n",
               phyDataUe->raInfo->preambleSlot,
               phyDataUe->raInfo->preambleSig,
               phyUmts->slotIndex,
               phyDataUe->raInfo->rachPreambleWaitTime);
    }
}

//  /**
// FUNCITON   :: UmtsMacUeConfigTrCh
// LAYER      :: UMTS PHY
// PURPOSE    :: Handle Interlayer CPHY-TrCh-CONFIG-REQ at Ue
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + ueMacVar         : UmtsMacUeData*    : ue mac var
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyUeConfigTrCh(Node* node,
                            UInt32 interfaceIndex,
                            UmtsMacData* umtsMacData,
                            UmtsMacUeData* ueMacVar,
                            void* cmd)
{
    UmtsCphyTrChConfigReqInfo* trChCfgInfo;
    trChCfgInfo = (UmtsCphyTrChConfigReqInfo*) cmd;

    if (trChCfgInfo->trChDir != UMTS_CH_UPLINK)
    {
        // only enlist downlink trCh
        // send  RELEASE-CNF to L3
        return;
    }

    // create transport channel object
    UmtsTransChInfo* trChInfo = NULL;
    trChInfo = UmtsMacGetTrChInfoFromList(
                   node,
                   ueMacVar->ueTrChList,
                   trChCfgInfo->trChType,
                   trChCfgInfo->trChId);
    if (!trChInfo)
    {
        UmtsTransportFormat format;

        trChInfo = new UmtsTransChInfo(trChCfgInfo->ueId,
                                       trChCfgInfo->trChType,
                                       trChCfgInfo->trChDir,
                                       trChCfgInfo->trChId,
                                       trChCfgInfo->transFormat.TTI,
                                       trChCfgInfo->logChPrio);

        ueMacVar->ueTrChList->push_back(trChInfo);
        if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
        {
            printf("\tnode %d (MAC_PHY):create a new TrInfo and "
                "add to list with type %d, chDir %d, chId %d TTI %d\n",
                node->nodeId, trChInfo->trChType, trChInfo->trChDir,
                trChInfo->trChId, trChInfo->TTI);
        }
    }
    else
    {
        // reconfiguration
        // reconfigurate the transport format
    }
}

//  /**
// FUNCITON   :: UmtsMacPhyUeReleaseTrCh
// LAYER      :: UMTS PHY
// PURPOSE    :: Handle Interlayer CPHY-TrCh-RELEASE-REQ at Ue
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + ueMacVar         : UmtsMacUeData*    : ue mac var
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
static
void UmtsMacPhyUeReleaseTrCh(Node* node,
                             UInt32 interfaceIndex,
                             UmtsMacData* umtsMacData,
                             UmtsMacUeData* ueMacVar,
                             void* cmd)
{
    UmtsCphyTrChReleaseReqInfo* trChRelInfo;
    //BOOL relSuc = FALSE;
    trChRelInfo = (UmtsCphyTrChReleaseReqInfo*) cmd;

    if (trChRelInfo->trChDir != UMTS_CH_UPLINK)
    {
        // only enlist downlink trCh
        // send L3  RELEASE-CNF to L3
        return;
    }

    std::list<UmtsTransChInfo*>* chList;
    chList = ueMacVar->ueTrChList;

    std::list<UmtsTransChInfo*>::iterator it;
    for (it = chList->begin();
        it != chList->end();
        it ++)
    {
        if ((*it)->trChType == trChRelInfo->trChType &&
            (*it)->trChId == trChRelInfo->trChId)
        {
            break;
        }
    }

    if (it != chList->end())
    {
        if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
        {
            printf("\tnode %d (MAC_PHY):delete TrInfo from "
                "list with type %d, chDir %d, chId %d TTI %d\n",
                node->nodeId, (*it)->trChType, (*it)->trChDir,
                (*it)->trChId, (*it)->TTI);
        }
        delete (*it);
        chList->erase(it);
    }

    // TODO: send RELEASE-CNF to L3
}

//  /**
// FUNCITON   :: UmtsMacPhyUeRlSetupModifyReq
// LAYER      :: UMTS PHY
// PURPOSE    :: Handle Interlayer CPHY-RL-SETUP-REQ at Ue
//                              or CPHY-RL-MODIFY_REQ
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + ueMacVar         : UmtsMacUeData*    : ue mac var
// + cmd              : void*             : cmd to handle
// + isRlSetup        : BOOL              : RL Setup or not
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyUeRlSetupModifyReq(Node* node,
                                  UInt32 interfaceIndex,
                                  UmtsMacData* umtsMacData,
                                  UmtsMacUeData* ueMacVar,
                                  void* cmd,
                                  BOOL isRlSetup)
{
    UmtsCphyRadioLinkSetupReqInfo* rlReqInfo;
    std::list <PhyUmtsChannelData*>* phyChList;

    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsUeData* phyDataUe = phyUmts->phyDataUe;
    //UmtsModulationType modType = UMTS_MOD_QPSK;
    UmtsCodingType codingType = UMTS_CONV_2;

    rlReqInfo =  (UmtsCphyRadioLinkSetupReqInfo*) cmd;

    if (rlReqInfo->phChDir != UMTS_CH_UPLINK)
    {
        // only enlist downlink trCh
        // send L3  setup-CNF to L3
        return;
    }

    if (rlReqInfo->phChType == UMTS_PHYSICAL_PRACH)
    {
        PhyUmtsChannelData* phChInfo;
        PhyUmtsUePrachInfo* prachDesc;

        prachDesc = (PhyUmtsUePrachInfo*) rlReqInfo->phChDesc;

        phyDataUe->prachSucPower = prachDesc->initTxPower;
        phyChList = phyDataUe->phyChList;
        phChInfo = UmtsMacGetPhChInfoFromList(
                        node,
                        phyChList,
                        rlReqInfo->phChType,
                        rlReqInfo->phChId);
        if (!phChInfo)
        {
            // add PRACH
            phChInfo = new PhyUmtsChannelData(
                                1,
                                UMTS_PHYSICAL_PRACH,
                                UMTS_CH_UPLINK,
                                UMTS_SF_256, // no meaning
                                0,
                                prachDesc->preScCode,
                                UMTS_PHY_CH_BRANCH_Q, // no meaning
                                1,
                                UMTS_MOD_QPSK,
                                UMTS_CONV_2);

            phChInfo->moreChInfo  =
                (PhyUmtsUePrachInfo*)
                 MEM_malloc(sizeof(PhyUmtsUePrachInfo));
            memcpy((void*)phChInfo->moreChInfo,
                   (void*)prachDesc,
                   sizeof(PhyUmtsUePrachInfo));

            // update the slotFormat
            phChInfo->slotFormatInUse = prachDesc->slotFormatIndex;

            phyDataUe->phyChList->push_back(phChInfo);

            // change the nodeB satus to activeSet
            // update detect list, active lsit and monitor list
            std::list<PhyUmtsUeNodeBInfo*>::iterator it;
            unsigned int priScCode;

            priScCode = ((int)(prachDesc->preScCode / 16)) * 16;
            for(it = phyDataUe->ueNodeBInfo->begin();
                it != phyDataUe->ueNodeBInfo->end();
                it ++)
            {
                if (priScCode == (*it)->primaryScCode)
                {
                    break;
                }
            }

            ERROR_Assert(it != phyDataUe->ueNodeBInfo->end(),
                         "try to set up a PRACH with scrambling"
                         "code belongs to a nodeB Ue did not detect");
        }

        if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
        {
            printf("\tNode %d(MAC-PHY): add new PRACH or modify a PRACH\n",
                node->nodeId);
        }
        // TODO send L3 CFN
    }
    else if (rlReqInfo->phChType == UMTS_PHYSICAL_DPDCH ||
        rlReqInfo->phChType == UMTS_PHYSICAL_DPCCH)
    {
        phyChList = phyDataUe->phyChList;
        PhyUmtsChannelData* phChInfo;
        UmtsCphyChDescUlDPCH* dpchDesc;

        dpchDesc = (UmtsCphyChDescUlDPCH*) rlReqInfo->phChDesc;

        phChInfo = UmtsMacGetPhChInfoFromList(
                        node,
                        phyChList,
                        rlReqInfo->phChType,
                        rlReqInfo->phChId);
        if (!phChInfo && rlReqInfo->phChType == UMTS_PHYSICAL_DPCCH)
        {
            // add DPCCH
            phChInfo = new PhyUmtsChannelData(
                                rlReqInfo->phChId,
                                rlReqInfo->phChType,
                                UMTS_CH_UPLINK,
                                UMTS_SF_256,
                                0,
                                dpchDesc->ulScCode,
                                UMTS_PHY_CH_BRANCH_Q,
                                1,
                                UMTS_MOD_QPSK,
                                UMTS_CONV_2);
            phChInfo->moreChInfo  =
                (UmtsCphyChDescUlDPCH*)
                 MEM_malloc(sizeof(UmtsCphyChDescUlDPCH));
            memcpy((void*)phChInfo->moreChInfo,
                   (void*)dpchDesc,
                   sizeof(UmtsCphyChDescUlDPCH));
            phyDataUe->phyChList->push_back(phChInfo);
        }
        else if (!phChInfo && rlReqInfo->phChType == UMTS_PHYSICAL_DPDCH)
        {
            unsigned int chCodeIndex = 1;
            UmtsPhyChRelativePhase phase;

            codingType = UMTS_TURBO_3;

            phyDataUe->ulDediPhChConfig.numDPDCH ++;

            // add DPDCH
            if (phyDataUe->ulDediPhChConfig.numDPDCH == 1)
            {
                // only one DPDCH
                // Reference: TS25.213 v700 p.16
                chCodeIndex = phyDataUe->dpchSfInUse / 4;
            }
            else
            {   if (phyDataUe->ulDediPhChConfig.numDPDCH == 2)
                {
                    // change the first dpdch 's SF tp 4
                    //PhyUmtsChannelData* firstPhChInfo;
                    std::list<PhyUmtsChannelData*>::iterator phChIter;
                    for (phChIter = phyChList->begin();
                         phChIter != phyChList->end();
                         ++phChIter)
                    {
                        if ((*phChIter)->phChType == UMTS_PHYSICAL_DPDCH)
                        {
                            (*phChIter)->spreadFactor = UMTS_SF_4;
                            (*phChIter)->chCodeIndex = 1;
                        }
                    }
                    chCodeIndex = 1;// CH # 1,2
                }
                else if (phyDataUe->ulDediPhChConfig.numDPDCH < 5)
                {
                    chCodeIndex = 3; // CH #3,4
                }
                else if (phyDataUe->ulDediPhChConfig.numDPDCH < 7)
                {
                    chCodeIndex = 2; // CH #5,6
                }
                else
                {
                    ERROR_ReportError("Max DPDCH support is 6");
                }
            }

            if (phyDataUe->ulDediPhChConfig.numDPDCH % 2 == 1)
            {
                // CH # 1, 3, 5
                phase = UMTS_PHY_CH_BRANCH_I;
            }
            else
            {
                // CH # 2, 4, 6
                phase = UMTS_PHY_CH_BRANCH_Q;
            }

            phChInfo = new PhyUmtsChannelData(
                                 rlReqInfo->phChId,
                                 rlReqInfo->phChType,
                                 UMTS_CH_UPLINK,
                                 phyDataUe->dpchSfInUse,
                                 chCodeIndex,
                                 dpchDesc->ulScCode,
                                 phase,
                                 1,
                                 UMTS_MOD_QPSK,
                                 codingType);

            phChInfo->moreChInfo  =
                (UmtsCphyChDescUlDPCH*)
                 MEM_malloc(sizeof(UmtsCphyChDescUlDPCH));
            memcpy((void*)phChInfo->moreChInfo,
                   (void*)dpchDesc,
                   sizeof(UmtsCphyChDescUlDPCH));

            phyDataUe->phyChList->push_back(phChInfo);

            if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
            {
                printf("\tNode %d(MAC-PHY): add new UL DPDCH "
                    "sfFactor%d ChCode %d slotFormat %d\n",
                    node->nodeId, phChInfo->spreadFactor,
                    phChInfo->chCodeIndex,
                    phChInfo->slotFormatInUse);
            }
        }
        else
        {
            // modify the RL
            memcpy((void*)phChInfo->moreChInfo,
                   (void*)dpchDesc,
                   sizeof(UmtsCphyChDescUlDPCH));

            // update slotFormat
            phChInfo->slotFormatInUse = (unsigned char)-1;
            UmtsLayer3GetUlPhChDataBitRate(
                phyDataUe->dpchSfInUse,
                (signed char*)&(phChInfo->slotFormatInUse));
        }
        if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
        {
            printf("\tNode %d(MAC-PHY): add new UL PDCH or modify a PDCH,"
                "current sfInUse %d\n",
                node->nodeId, phyDataUe->dpchSfInUse);
        }
        // TODO send L3 CFN
    }
    else if (rlReqInfo->phChType == UMTS_PHYSICAL_HSDPCCH)
    {
        phyChList = phyDataUe->phyChList;
        PhyUmtsChannelData* phChInfo;
        UmtsCphyChDescUlHsdpcch* hsdpcchDesc;

        hsdpcchDesc = (UmtsCphyChDescUlHsdpcch*) rlReqInfo->phChDesc;

        phChInfo = UmtsMacGetPhChInfoFromList(
                                node,
                                phyChList,
                                rlReqInfo->phChType,
                                rlReqInfo->phChId);
        if (!phChInfo)
        {
            // add HSDPCCH

            phChInfo = new PhyUmtsChannelData(
                                1,
                                UMTS_PHYSICAL_HSDPCCH,
                                UMTS_CH_UPLINK,
                                hsdpcchDesc->sfFactor,
                                hsdpcchDesc->chCode,
                                hsdpcchDesc->ulScCode,
                                UMTS_PHY_CH_BRANCH_I,
                                1,
                                UMTS_MOD_QPSK,
                                UMTS_CONV_2);
            phChInfo->moreChInfo  =
                (UmtsCphyChDescUlHsdpcch*)
                 MEM_malloc(sizeof(UmtsCphyChDescUlHsdpcch));
            memcpy((void*)phChInfo->moreChInfo,
                   (void*)hsdpcchDesc,
                   sizeof(UmtsCphyChDescUlHsdpcch));

            // update the slotFormat
            phChInfo->slotFormatInUse = hsdpcchDesc->slotFormatIndex;

            phyDataUe->phyChList->push_back(phChInfo);

            phyDataUe->ulDediPhChConfig.numHsDPCCH ++;

            if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
            {
                printf("\tNode %d(MAC-PHY): add HS-DPCCH\n",
                    node->nodeId);
            }
        }
        else
        {
            // more to here
        }
    }
}

// /**
// FUNCITON   :: UmtsMacPhyUeRlReleaseReq
// LAYER      :: UMTS PHY
// PURPOSE    :: Handle Interlayer CPHY-RL-RELEASE-REQ at Ue
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + umtsMacData      : UmtsMacData*      : Umts Mac Data
// + ueMacVar         : UmtsMacUeData*    : ue mac var
// + cmd              : void*             : cmd to handle
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyUeRlReleaseReq(Node* node,
                              UInt32 interfaceIndex,
                              UmtsMacData* umtsMacData,
                              UmtsMacUeData* ueMacVar,
                              void* cmd)
{
    UmtsCphyRadioLinkRelReqInfo* rlReqInfo = NULL;
    std::list <PhyUmtsChannelData*>* phyChList = NULL;

    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsUeData* phyDataUe = phyUmts->phyDataUe;

    rlReqInfo =  (UmtsCphyRadioLinkRelReqInfo*) cmd;

    if (rlReqInfo->phChDir != UMTS_CH_UPLINK)
    {
        // only enlist downlink trCh
        // send L3  setup-CNF to L3
        return;
    }

    phyChList = phyDataUe->phyChList;

    std::list<PhyUmtsChannelData*>::iterator it;
    for (it = phyChList->begin();
        it != phyChList->end();
        it ++)
    {
        if ((*it)->phChType == rlReqInfo->phChType &&
            (*it)->channelIndex == rlReqInfo->phChId)
        {
            break;
        }
    }

    if (it != phyChList->end())
    {
        if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
        {
            printf("\tnode %d (MAC_PHY):delete a PhCh from "
                "list with type %d, chDir %d, chId %d Sf %d codeIndex %d\n",
                node->nodeId, (*it)->phChType, (*it)->chDirection,
                (*it)->channelIndex,
                (*it)->spreadFactor, (*it)->chCodeIndex);
        }

        if ((*it)->phChType == UMTS_PHYSICAL_DPDCH)
        {
            phyDataUe->ulDediPhChConfig.numDPDCH --;
        }
        else if ((*it)->phChType == UMTS_PHYSICAL_HSDPCCH)
        {
            phyDataUe->ulDediPhChConfig.numHsDPCCH --;
        }
        else if ((*it)->phChType == UMTS_PHYSICAL_PRACH)
        {
            if (phyDataUe->raInfo)
            {
                MEM_free(phyDataUe->raInfo);
                phyDataUe->raInfo = NULL;
            }
        }
        else
        {
            // do nothing
        }
        
        std::list<Message*>::iterator itMsg;

        for (itMsg = (*it)->txMsgList->begin();
            itMsg != (*it)->txMsgList->end();
            itMsg ++)
        {
            if (*itMsg)
            {
                MESSAGE_Free(node, (*itMsg));
            }
        }

        delete (*it);
        phyChList->erase(it);
        // TODO send L3 CFN
    }
}

// /**
// FUNCITON   :: UmtsMacPhyUeHandleInterLayerCommand
// LAYER      :: UMTS L1 PHY
// PURPOSE    :: Handle Interlayer command at Ue
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface index
// + cmdType          : UmtsInterlayerCommandType : command type
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
// **/
static
void UmtsMacPhyUeHandleInterLayerCommand(
            Node* node,
            UInt32 interfaceIndex,
            UmtsInterlayerCommandType cmdType,
            void* cmd)
{
    MacCellularData* macCellularData;
    UmtsMacData* umtsMacData;
    UmtsMacUeData* ueMacVar;

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;
    umtsMacData =
        (UmtsMacData*)macCellularData->cellularUmtsL2Data->umtsMacData;

    ueMacVar = (UmtsMacUeData*)umtsMacData->ueMacVar;

    if (DEBUG_MAC_PHY_INTERLAYER)
    {
        UmtsLayer2PrintRunTimeInfo(
            node, interfaceIndex, UMTS_LAYER2_SUBLAYER_NONE);
        printf("\t(MAC-PHY): rcvd a interlayer cmd type %d\n",
               cmdType);
    }
    switch(cmdType)
    {
        case UMTS_CPHY_TrCH_CONFIG_REQ:
        {
            UmtsMacPhyUeConfigTrCh(node,
                                   interfaceIndex,
                                   umtsMacData,
                                   ueMacVar,
                                   cmd);
            break;
        }
        case UMTS_CPHY_TrCH_RELEASE_REQ:
        {
            UmtsMacPhyUeReleaseTrCh(node,
                                    interfaceIndex,
                                    umtsMacData,
                                    ueMacVar,
                                    cmd);
            break;
        }
        case UMTS_CPHY_RL_SETUP_REQ:
        {
            UmtsMacPhyUeRlSetupModifyReq(node,
                                         interfaceIndex,
                                         umtsMacData,
                                         ueMacVar,
                                         cmd,
                                         TRUE);

            break;
        }
        case UMTS_CPHY_RL_RELEASE_REQ:
        {
            UmtsMacPhyUeRlReleaseReq(node,
                                     interfaceIndex,
                                     umtsMacData,
                                     ueMacVar,
                                     cmd);
            break;
        }
        case UMTS_CPHY_RL_MODIFY_REQ:
        {
            UmtsMacPhyUeRlSetupModifyReq(node,
                                         interfaceIndex,
                                         umtsMacData,
                                         ueMacVar,
                                         cmd,
                                         FALSE);
            break;
        }
        case UMTS_CPHY_COMMIT_REQ:
        {
            break;
        }
        case UMTS_CPHY_OUT_OF_SYNC_CONFIG_REQ:
        {
            break;
        }
        case UMTS_CPHY_MBMS_CONFIG_REQ:
        {
            break;
        }
        case UMTS_PHY_ACCESS_REQ:
        {

            UmtsMacPhyUeHandleAccessReq(node,
                                        interfaceIndex,
                                        umtsMacData,
                                        ueMacVar,
                                        cmd);

            break;
        }
        default:
        {
            break;
        }
    }
}

//-------------------------------------------------------------------------
// API functions
//-------------------------------------------------------------------------

// /**
// FUNCTION   :: UmtsMacPhyStartListeningToChannel
// LAYER      :: UMTS PHY
// PURPOSE    :: Start listening to a channel
// PARAMETERS ::
// + node : Node* : Pointer to node
// + phyNumber : int :PHY number
// + channelIndex : int : channel to listen to
// RETURN     :: void : NULL
// **/
void UmtsMacPhyStartListeningToChannel(Node* node,
                                       int phyNumber,
                                       int channelIndex)
{
    if (!PHY_IsListeningToChannel(node, phyNumber, channelIndex))
    {
        if (DEBUG_MAC_PHY_CHANNEL)
        {
            char timeStr[20];
            TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        
            printf("at %s Node%u starts listening to channel %d\n",
                    timeStr, node->nodeId, channelIndex);
        }

        PHY_StartListeningToChannel(node, phyNumber, channelIndex);
    }
}

// /**
// FUNCTION   :: UmtsMacPhyStopListeningToChannel
// LAYER      :: PHY
// PURPOSE    :: Stop listening to a channel
// PARAMETERS ::
// + node : Node* : Pointer to node
// + phyNumber : int : PHY number of the interface
// + channelIndex : int : channel to stop listening to
// RETURN     :: void : NULL
// **/
void UmtsMacPhyStopListeningToChannel(Node* node,
                                      int phyNumber,
                                      int channelIndex)
{
    if (PHY_IsListeningToChannel(node, phyNumber, channelIndex))
    {
        if (DEBUG_MAC_PHY_CHANNEL)
        {
            char timeStr[20];
            TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
            printf("at %s Node%u stops listening to channel %d\n",
                   timeStr, node->nodeId, channelIndex);
        }

        PHY_StopListeningToChannel(node, phyNumber, channelIndex);
    }
}

// /**
// FUNCTION   :: UmtsMacReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer.
//               The PHY sublayer will first handle it
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + interfaceIndex   : int          : Interface Index
// + msg              : Message*     : Packet received from PHY
// RETURN     :: void : NULL
// **/
void UmtsMacReceivePacketFromPhy(Node* node,
                                 int interfaceIndex,
                                 Message* msg)
{
    if (DEBUG_MAC_PHY_RX_TX && 0)
    {
        printf("Node %d MAC-PHY: rcvd a msg from Phy\n", node->nodeId);
    }
    if (UmtsGetNodeType(node) == CELLULAR_UE)
    {
        UmtsMacPhyUeReceivePacketFromPhy(node, interfaceIndex, msg);
    }
    else if (UmtsGetNodeType(node) == CELLULAR_NODEB)
    {
        UmtsMacPhyNodeBReceivePacketFromPhy(node, interfaceIndex, msg);
    }
}

//  /**
// FUNCITON   :: UmtsMacHandleInterLayerCommand
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Handle Interlayer command CPHY-
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + cmdType          : UmtsInterlayerCommandType : command type
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
// **/
void UmtsMacPhyHandleInterLayerCommand(
            Node* node,
            UInt32 interfaceIndex,
            UmtsInterlayerCommandType cmdType,
            void* cmd)
{
    if (UmtsIsNodeB(node))
    {
        UmtsMacPhyNodeBHandleInterLayerCommand(
            node,
            interfaceIndex,
            cmdType,
            cmd);
    }
    else if (UmtsIsUe(node))
    {
        UmtsMacPhyUeHandleInterLayerCommand(
            node,
            interfaceIndex,
            cmdType,
            cmd);
    }
}

//  /**
// FUNCITON   :: UmtsMacPhyConfigRcvPhCh
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Config receiving side DPDCH information 
//               (channelization code)
// PARAMETERS ::
// + node             : Node*                   : Pointer to node.
// + interfaceIndex   : UInt32                  : interface Index
// + priSc            : UInt32                  : primary scrambling code
// + phChInfo         : const UmtsRcvPhChInfo*  : receiving physical channel
//                                                information
// RETURN     :: void : NULL
// **/
void UmtsMacPhyConfigRcvPhCh(
    Node* node,
    UInt32 interfaceIndex,
    UInt32 priSc,
    const UmtsRcvPhChInfo* phChInfo)
{
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;

    if (UmtsGetNodeType(node) == CELLULAR_UE)
    {
        PhyUmtsUeData* phyUmtsUe = (PhyUmtsUeData*)phyUmts->phyDataUe;
        std::list <PhyUmtsUeNodeBInfo*>::iterator it;
        for (it = phyUmtsUe->ueNodeBInfo->begin();
             it != phyUmtsUe->ueNodeBInfo->end();
             it ++)
        {
            if (priSc == (*it)->primaryScCode)
            {
                (*it)->cellStatus = UMTS_CELL_ACTIVE_SET;
#if 0
                if ((*it)->dlPhChInfoList->end() ==
                    std::find_if((*it)->dlPhChInfoList->begin(),
                                 (*it)->dlPhChInfoList->end(),
                                 UmtsFindItemByOvsf<UmtsRcvPhChInfo>
                                    (phChInfo->chCode,
                                    phChInfo->sfFactor)))
#endif // 0
                //alow duplicate items in the list, so that multiple
                //configuration on the same receiving channels are allowed
                {
                    UmtsRcvPhChInfo* info = (UmtsRcvPhChInfo*)
                                        MEM_malloc(sizeof(UmtsRcvPhChInfo));
                    memcpy(info, phChInfo, sizeof(UmtsRcvPhChInfo));
                    (*it)->dlPhChInfoList->push_back(info);
                }
                break;
            } // if (priSc == (*it)->primaryScCode)
        } // for (it)
    } // if (UmtsGetNodeType(node) == CELLULAR_UE)
    //TODO, add Uplink info for NodeB
}

//  /**
// FUNCITON   :: UmtsMacPhyReleaseRcvPhCh
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Release receiving side DPDCH information 
//               (channelization code)
// PARAMETERS ::
// + node             : Node*                   : Pointer to node.
// + interfaceIndex   : UInt32                  : interface Index
// + priSc            : UInt32                  : primary scrambling code
// + phChInfo         : const UmtsRcvPhChInfo*  : receiving physical channel
//                                                information
// RETURN     :: void : NULL
// **/
void UmtsMacPhyReleaseRcvPhCh(
    Node* node,
    UInt32 interfaceIndex,
    UInt32 priSc,
    const UmtsRcvPhChInfo* phChInfo)
{
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;

    if (UmtsGetNodeType(node) == CELLULAR_UE)
    {
        PhyUmtsUeData* phyUmtsUe = (PhyUmtsUeData*)phyUmts->phyDataUe;
        std::list <PhyUmtsUeNodeBInfo*>::iterator it;
        for (it = phyUmtsUe->ueNodeBInfo->begin();
             it != phyUmtsUe->ueNodeBInfo->end();
             ++it )
        {
            if (priSc == (*it)->primaryScCode)
            {
                if (phChInfo == NULL)
                {
                    std::for_each((*it)->dlPhChInfoList->begin(),
                                  (*it)->dlPhChInfoList->end(),
                                  UmtsFreeStlItem<UmtsRcvPhChInfo>());
                    (*it)->dlPhChInfoList->clear();
                    (*it)->cellStatus = UMTS_CELL_DETECTED_SET;
                }
                else
                {
                    std::list<UmtsRcvPhChInfo*>::iterator dlPhChIt;
                    dlPhChIt = std::find_if(
                                    (*it)->dlPhChInfoList->begin(),
                                    (*it)->dlPhChInfoList->end(),
                                    UmtsFindItemByOvsf<UmtsRcvPhChInfo>
                                        ((unsigned char)phChInfo->chCode,
                                        phChInfo->sfFactor));
                    if (dlPhChIt != (*it)->dlPhChInfoList->end())
                    {
                        MEM_free((*dlPhChIt));
                        (*it)->dlPhChInfoList->erase(dlPhChIt);
                    }
                } // if (phChInfo == NULL)
            } // if (priSc == (*it)->primaryScCode)
        } // for (it)
    } //  if (UmtsGetNodeType(node) == CELLULAR_UE)
}

//  /**
// FUNCITON   :: UmtsMacPhyMappingTrChPhCh
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Handle Interlayer command CPHY-
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + mappingInfo      : UmtsTrCh2PhChMappingInfo* : Mapping Info
// RETURN     :: void : NULL
void UmtsMacPhyMappingTrChPhCh(
            Node* node,
            UInt32 interfaceIndex,
            UmtsTrCh2PhChMappingInfo* mappingInfo)
{
    MacCellularData* macCellularData;
    UmtsMacData* umtsMacData;
    UmtsMacUeData* ueMacVar = NULL;
    UmtsMacNodeBData* nodebMacVar = NULL;
    std::list <UmtsRbLogTrPhMapping*>* mapList = NULL;

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;
    umtsMacData =
        (UmtsMacData*)macCellularData->cellularUmtsL2Data->umtsMacData;

    if(UmtsIsUe(node))
    {
        ueMacVar = (UmtsMacUeData*)umtsMacData->ueMacVar;
        mapList = ueMacVar->ueLogTrPhMap;
    }
    else if(UmtsIsNodeB(node))
    {
        nodebMacVar = (UmtsMacNodeBData*)umtsMacData->nodeBMacVar;
        if (mappingInfo->ueId == node->nodeId)
        {
            // common channels
            mapList = nodebMacVar->nodeBLocalMap;
        }
        else
        {
            UmtsMacNodeBUeInfo* ueInfo;
            ueInfo = UmtsMacPhyNodeBGetUeInfoFromList(
                         node,
                         nodebMacVar->macNodeBUeInfo,
                         mappingInfo->priSc);

            ERROR_Assert(ueInfo, "no ueInfo link with this mapping info");

            mapList = ueInfo->ueLogTrPhMap;
        }
    }

    ERROR_Assert(mapList != NULL, "mapList is NULL");

    std::list<UmtsRbLogTrPhMapping*>::iterator it;

    for (it = mapList->begin();
            it != mapList->end();
            it ++)
    {
        if ((*it)->rbId == mappingInfo->rbId)
        {
            break;
        }
    }

    ERROR_Assert(it != mapList->end(), " Cannot find the mappingInfo");

    if (mappingInfo->chDir == UMTS_CH_UPLINK)
    {
        (*it)->ulPhChId = (unsigned char)mappingInfo->phChId;
        (*it)->ulPhChType = mappingInfo->phChType;
        if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
        {
            printf("\tnode %d (MAC-PHY): "
                   "Mapping %d UL direction trChType %d "
                   "trChId %d, to PhCh type %d phCh %d \n",
                   node->nodeId, mappingInfo->chDir,
                   (*it)->ulTrChType, (*it)->ulTrChId,
                   (*it)->ulPhChType, (*it)->ulPhChId);
        }
    }
    else if (mappingInfo->chDir == UMTS_CH_DOWNLINK)
    {
        (*it)->dlPhChId = (unsigned char)mappingInfo->phChId;
        (*it)->dlPhChType = mappingInfo->phChType;
        if (DEBUG_MAC_PHY_INTERLAYER || DEBUG_MAC_PHY_TR_RL_CH)
        {
            printf("\tnode %d (MAC-PHY): "
                   "Mapping %d DL direction trChType %d "
                   "trChId %d, to PhCh type %d phCh %d \n",
                   node->nodeId,mappingInfo->chDir,
                   (*it)->dlTrChType, (*it)->dlTrChId,
                   (*it)->dlPhChType, (*it)->dlPhChId);
        }
    }

    // update the transport format
    memcpy(&((*it)->transFormat), &(mappingInfo->transFormat),
        sizeof(UmtsTransportFormat));

    if (DEBUG_TRANSPORT_FORMAT)
    {
        UmtsLayer2PrintRunTimeInfo(
            node, interfaceIndex, UMTS_LAYER2_SUBLAYER_NONE);
        printf("Current Transport Format is \n");
        printf("\t transBlkSize %d transBlkSetSize %15" TYPES_64BITFMT"d\n",
            (*it)->transFormat.transBlkSize,
               (*it)->transFormat.transBlkSetSize);
        printf("\t TTI %d modType %d \n", (*it)->transFormat.TTI,
               (*it)->transFormat.modType);
        printf("\t coding Type %d crcSize %d\n",
               (*it)->transFormat.codingType,
               (*it)->transFormat.crcSize);
    }
}

// /**
// FUNCTION   :: UmtsMacPhyNodeBEnqueuePacketToPhChTxBuffer
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: enqueue packet to PhCh txBuffer
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// chType      : UmtsPhysicalChannelType   : physical channel type
// chId        : unsigned int              : channel Idetifier
// pduList     : std::list<Message*>&      : pointer to msg list
//                                           to be enqueued
// RETURN     :: void : NULL
// **/
void UmtsMacPhyNodeBEnqueuePacketToPhChTxBuffer(
                     Node* node,
                     UInt32 interfaceIndex,
                     UmtsPhysicalChannelType chType,
                     unsigned int chId,
                     std::list<Message*>& pduList)
{
    // get Phy data
    std::list<Message*>::iterator it;
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsNodeBData* phyDataNodeB = phyUmts->phyDataNodeB;

    PhyUmtsChannelData* phChData;
    phChData = UmtsMacGetPhChInfoFromList(
                   node,
                   phyDataNodeB->phyChList,
                   chType,
                   chId);

    ERROR_Assert(phChData, "no phData link with this chType and chId");

    // put it into the Tx queue
    it = phChData->txMsgList->end();
    phChData->txMsgList->insert(it, pduList.begin(), pduList.end());
}

// /**
// FUNCTION   :: UmtsMacPhyUeEnqueuePacketToPhChTxBuffer
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: enqueue packet to PhCh txBuffer
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// chType      : UmtsPhysicalChannelType   : physical channel type
// chId        : unsigned int              : channel Idetifier
// pduList     : std::list<Message*>&      : pointer to msg list 
//                                           to be enqueued
// RETURN     :: void : NULL
// **/
void UmtsMacPhyUeEnqueuePacketToPhChTxBuffer(
                     Node* node,
                     UInt32 interfaceIndex,
                     UmtsPhysicalChannelType chType,
                     unsigned int chId,
                     std::list<Message*>& pduList)
{
    // get Phy data
    std::list<Message*>::iterator it;
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsUeData* phyDataUe = phyUmts->phyDataUe;

    PhyUmtsChannelData* phChData;
    phChData = UmtsMacGetPhChInfoFromList(
                   node,
                   phyDataUe->phyChList,
                   chType,
                   chId);

    ERROR_Assert(phChData, "no phData link with this chType and chId");

    // put it into the Tx queue
    it = phChData->txMsgList->end();
    phChData->txMsgList->insert(it, pduList.begin(), pduList.end());

    if (DEBUG_MAC_PHY_DATA &&
        chType == UMTS_PHYSICAL_DPDCH)
    {
        printf ("Node: %d is enquing %u MAC message on DPDCH. \n",
                    node->nodeId, (unsigned int)pduList.size());
        UmtsPrintMessage(std::cout, pduList.front());
    }

    if (phChData->phChType == UMTS_PHYSICAL_PRACH &&
        phyDataUe->raInfo)
    {
        phyDataUe->raInfo->needSendRachData = TRUE;
        phyDataUe->raInfo->rachDataWaitTime = 3;

        // assume 3 slot later to send data
    }
}

// /**
// FUNCTION   :: UmtsMacPhyGetPccpchSfn
// LAYER      :: Layer 2
// PURPOSE    :: return the PCCPCH SFN at nodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + priSc     : unsigned int : primary sc code of the nodeB
// RETURN     :: unsigned int   : PCCPCH SFN index of the nodeB
// **/
unsigned int UmtsMacPhyGetPccpchSfn(Node*node,
                                     int interfaceIndex,
                                     unsigned int priSc)
{
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;

    if (UmtsGetNodeType(node) == CELLULAR_NODEB)
    {
        PhyUmtsNodeBData* phyUmtsNodeb =
            (PhyUmtsNodeBData*)phyUmts->phyDataNodeB;

        return phyUmtsNodeb->sfnPccpch;
    }
    else if (UmtsGetNodeType(node) == CELLULAR_UE)
    {
        PhyUmtsUeData* phyUmtsUe = (PhyUmtsUeData*)phyUmts->phyDataUe;

        std::list <PhyUmtsUeNodeBInfo*>::iterator it;
        for (it = phyUmtsUe->ueNodeBInfo->begin();
            it != phyUmtsUe->ueNodeBInfo->end();
            it ++)
        {
            if (priSc == (*it)->primaryScCode)
            {
                break;
            }
        }

        ERROR_Assert(it != phyUmtsUe->ueNodeBInfo->end(),
                     "No ueNodeBinfo link with this priSc");

        return ((*it)->sfnPccpch);
    }
    else
    {
        ERROR_ReportError(
            "UMTS PHY can only be configured at UE or NodeB!");
        return 0;
    }
}

// /**
// FUNCTION   :: UmtsMacPhySetPccpchSfn
// LAYER      :: Layer 2
// PURPOSE    :: return the active ul channel index 
//               of the interface at UE/NodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + pccpchSfn : unsigned int : PCCPCH FSN
// + priSc     : unsigned int : primary sc code of the nodeB
// RETURN     :: void         : NULL
// **/
void UmtsMacPhySetPccpchSfn(Node*node,
                             int interfaceIndex,
                             unsigned int pccpchSfn,
                             unsigned int priSc)
{
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;

    if (UmtsGetNodeType(node) == CELLULAR_NODEB)
    {
        PhyUmtsNodeBData* phyUmtsNodeb =
            (PhyUmtsNodeBData*)phyUmts->phyDataNodeB;

        phyUmtsNodeb->sfnPccpch = pccpchSfn;
    }
    else if (UmtsGetNodeType(node) == CELLULAR_UE)
    {
        PhyUmtsUeData* phyUmtsUe = (PhyUmtsUeData*)phyUmts->phyDataUe;

        std::list <PhyUmtsUeNodeBInfo*>::iterator it;
        for (it = phyUmtsUe->ueNodeBInfo->begin();
            it != phyUmtsUe->ueNodeBInfo->end();
            it ++)
        {
            if (priSc == (*it)->primaryScCode)
            {
                break;
            }
        }

        ERROR_Assert(it != phyUmtsUe->ueNodeBInfo->end(),
                     "No ueNodeBinfo link with this priSc");

        (*it)->sfnPccpch = pccpchSfn;
    }
}

// /**
// FUNCTION   :: UmtsMacPhyNodeBAddUeInfo
// LAYER      :: Layer 2
// PURPOSE    :: return the active ul channel index 
//               of the interface at UE/NodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + ueId      : NodeAddress : UE Node Id
// + priSc     : unsigned int : primary sc code of the nodeB
// + selfPrimCell : BOOL  : Is it a priCell for UE
// RETURN     :: void   : NULL
// **/
void UmtsMacPhyNodeBAddUeInfo(Node*node,
                              int interfaceIndex,
                              NodeAddress ueId,
                              unsigned int priSc,
                              BOOL selfPrimCell)
{
    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsNodeBData* phyDataNodeB = phyUmts->phyDataNodeB;

    PhyUmtsNodeBUeInfo* ueInfo;
    ueInfo = new PhyUmtsNodeBUeInfo(ueId, priSc, selfPrimCell);
    ueInfo->cqiInfo.repVal = 1; // the initial value
    phyDataNodeB->ueInfo->push_back(ueInfo);
}

// /**
// FUNCTION   :: UmtsMacPhyNodeBRemoveUeInfo
// LAYER      :: Layer 2
// PURPOSE    :: return the active ul channel index 
//               of the interface at UE/NodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + ueId      : NodeAddress : UE Node Id
// + priSc     : unsigned int : primary sc code of the nodeB
// RETURN     :: void   : NULL
// **/
void UmtsMacPhyNodeBRemoveUeInfo(Node*node,
                                 int interfaceIndex,
                                 NodeAddress ueId,
                                 unsigned int priSc)
{
    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsNodeBData* phyDataNodeB = phyUmts->phyDataNodeB;

    std::list<PhyUmtsNodeBUeInfo*>::iterator it;
    for ( it = phyDataNodeB->ueInfo->begin();
          it != phyDataNodeB->ueInfo->end();
          it ++)
    {
        if ((*it)->priUlScAssigned == priSc)
        {
            break;
        }
    }

    ERROR_Assert(it != phyDataNodeB->ueInfo->end(),
                 " no Ue info link with this priSc");

    delete (*it);
    phyDataNodeB->ueInfo->erase(it);
}

// /**
// FUNCTION   :: UmtsMacPhyUeCheckUlPowerControlAdjustment
// LAYER      :: Layer 2
// PURPOSE    :: return the active ul channel index 
//               of the interface at UE/NodeB
// PARAMETERS ::
// + node           : Node *          : Pointer to node.
// + interfaceIndex : int             : interface index
// RETURN     :: void : NULL
// **/
void UmtsMacPhyUeCheckUlPowerControlAdjustment(Node* node,
                                               UInt32 interfaceIndex)
{
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsUeData* phyDataUe = phyUmts->phyDataUe;

    std::list<PhyUmtsUeNodeBInfo*>::iterator it;
    signed char pcCmd;
    int numActiveSetPowCmd = 0;
    int numInc = 0;
    int numDec = 0;
    int numSteps = 0;

    for(it = phyDataUe->ueNodeBInfo->begin();
        it != phyDataUe->ueNodeBInfo->end();
        it ++)
    {
        if ((*it)->cellStatus == UMTS_CELL_ACTIVE_SET &&
            (*it)->ulPowCtrl->needAdjustPower)
        {
            numActiveSetPowCmd ++;
            if ((*it)->ulPowCtrl->deltaPower > 0)
            {
                numInc ++;
            }
            else if ((*it)->ulPowCtrl->deltaPower < 0)
            {
                numDec ++;
            }

            if (abs((*it)->ulPowCtrl->deltaPower) > numSteps)
            {
                numSteps = abs((*it)->ulPowCtrl->deltaPower);
            }

            (*it)->ulPowCtrl->needAdjustPower = FALSE;
        }
    }
    if (numActiveSetPowCmd)
    {
        signed char deltaPower = 0;
        double curTxPow = 0;

        // need power control
        if (phyDataUe->pca == UMTS_POWER_CONTROL_ALGO_1)
        {
            if (numInc == numActiveSetPowCmd)
            {
                pcCmd = 1;
            }
            else
            {
                pcCmd = -1;
            }
            deltaPower = 
                (signed char)(pcCmd * phyDataUe->stepSize * numSteps);
        }
        else
        {
            // other power control algorithm
        }

        PhyUmtsGetTransmitPower(node, phyIndex, &curTxPow);

        curTxPow = IN_DB(curTxPow);

        // set the new trnasmission power
        PhyUmtsSetTransmitPower(
                    node,
                    phyIndex,
                    NON_DB(curTxPow + deltaPower));
        if (DEBUG_POWER)
        {
            UmtsLayer2PrintRunTimeInfo(
                node, interfaceIndex, UMTS_LAYER2_SUBLAYER_NONE);
            if (pcCmd == -1)
            {
                printf("\n\tDecrease power for %d(dB) from %f to %f\n",
                    phyDataUe->stepSize, curTxPow, curTxPow + deltaPower);
            }
            else
            {
                 printf("\n\tIncrease power for %d(dB) from %f to %f\n",
                    phyDataUe->stepSize, curTxPow, curTxPow + deltaPower);
            }
        }
    } // if (numActiveSetPowCmd)
}

// /**
// FUNCTION   :: UmtsMacPhyNodeBEnableSelfPrimCell
// LAYER      :: Layer 2
// PURPOSE    :: Enable self as the prim cell for this UE
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + priSc     : unsigned int : Ue Primary Scrambling code
// RETURN     :: void : NULL
// **/
void UmtsMacPhyNodeBEnableSelfPrimCell(
    Node*node,
    int interfaceIndex,
    unsigned int priSc)
{
    PhyUmtsNodeBUeInfo* nodebUeInfo =
        UmtsMacPhyNodeBFindUeInfo(
            node, interfaceIndex, priSc);
    ERROR_Assert(nodebUeInfo->isSelfPrimCell == FALSE,
        "It's already the primary cell of the UE.");
    nodebUeInfo->isSelfPrimCell = TRUE;

    // update the power control after cell switch option
    nodebUeInfo->powCtrlAftCellSwitch = TRUE;
}

// /**
// FUNCTION   :: UmtsMacPhyNodeBDisableSelfPrimCell
// LAYER      :: Layer 2
// PURPOSE    :: Enable self as the prim cell for this UE
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + priSc     : unsigned int : Ue Primary Scrambling code
// RETURN     :: void   : NULL
// **/
void UmtsMacPhyNodeBDisableSelfPrimCell(
    Node*node,
    int interfaceIndex,
    unsigned int priSc)
{
    PhyUmtsNodeBUeInfo* nodebUeInfo =
        UmtsMacPhyNodeBFindUeInfo(
            node, interfaceIndex, priSc);
    if (nodebUeInfo)
    {
        nodebUeInfo->isSelfPrimCell = FALSE;
    }
}

// /**
// FUNCTION   :: UmtsMacPhyUeRelAllDedchPhCh
// LAYER      :: Layer 2
// PURPOSE    :: release  dedicated physical channel
// PARAMETERS ::
// + node           : Node *          : Pointer to node.
// + interfaceIndex : int             : interface index
// RETURN     :: void : NULL
// **/
void UmtsMacPhyUeRelAllDedctPhCh(Node* node,
                                 UInt32 interfaceIndex)
{
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsUeData* phyDataUe = phyUmts->phyDataUe;

    std::list<PhyUmtsChannelData*>::iterator itCh;

    // delete the channel list
    for (itCh = phyDataUe->phyChList->begin();
         itCh != phyDataUe->phyChList->end();
         itCh ++)
    {
        std::list<Message*>::iterator it;

        if ((*itCh)->phChType == UMTS_PHYSICAL_DPDCH ||
            (*itCh)->phChType == UMTS_PHYSICAL_DPCCH)
        {
            for (it = (*itCh)->txMsgList->begin();
                it != (*itCh)->txMsgList->end();
                it ++)
            {
                if (*it)
                {
                    MESSAGE_Free(node, (*it));
                }
            }
            delete (*itCh);
            (*itCh) = NULL;
        }
    }

    memset(&(phyDataUe->ulDediPhChConfig),
           0,
           sizeof(UeULDediPhChConfig));

    phyDataUe->phyChList->remove(NULL);
}

// /**
// FUNCTION   :: UmtsMacPhyUeReleaseAllDch
// LAYER      :: Layer 2
// PURPOSE    :: release all  dedicated Dedicated Transport channels
// PARAMETERS ::
// + node           : Node *          : Pointer to node.
// + interfaceIndex : int             : interface index
// RETURN     :: void : NULL
// **/
void UmtsMacPhyUeRelAllDch(Node* node,
                           UInt32 interfaceIndex)
{
    MacCellularData* macCellularData;
    UmtsMacData* umtsMacData;
    UmtsMacUeData* ueMacVar;

    macCellularData = (MacCellularData*)
                      node->macData[interfaceIndex]->macVar;
    umtsMacData = (UmtsMacData*)
                  macCellularData->cellularUmtsL2Data->umtsMacData;
    ueMacVar = (UmtsMacUeData*)umtsMacData->ueMacVar;

    std::list<UmtsTransChInfo*>* chList = ueMacVar->ueTrChList;

    std::list<UmtsTransChInfo*>::iterator it;
    for (it = chList->begin(); it != chList->end(); ++it)
    {
        if ((*it)->trChType == UMTS_TRANSPORT_DCH)
        {
            delete (*it);
            (*it) = NULL;
        }
    }
    chList->remove(NULL);
}

// /**
// FUNCTION   :: UmtsMacPhyUeSetDataBitSizeInBuffer
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: Set the number of data bit in BUffer
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// numDataBitInBuffer : int                : number of data bit in buffer
// RETURN     :: void : NULL
// **/
void UmtsMacPhyUeSetDataBitSizeInBuffer(
                     Node* node,
                     UInt32 interfaceIndex,
                     int numDataBitInBuffer)
{
    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsUeData* phyDataUe = phyUmts->phyDataUe;

    phyDataUe->numDataBitsInBuffer = numDataBitInBuffer;
}

// /**
// FUNCTION   :: UmtsMacPhyUeGetDataBitSizeInBuffer
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: Get the number of data bit in BUffer
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// RETURN     :: int : numDataBitInBuffer
// **/
int UmtsMacPhyUeGetDataBitSizeInBuffer(
                     Node* node,
                     UInt32 interfaceIndex)
{
    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsUeData* phyDataUe = phyUmts->phyDataUe;

    return phyDataUe->numDataBitsInBuffer;
}

// /**
// FUNCTION   :: UmtsMacPhyUeGetDpdchNumber
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: Get the number of DPDCH channels
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// RETURN     :: unsigned char : number of dedicated DPDCH
// **/
unsigned char UmtsMacPhyUeGetDpdchNumber(
                     Node* node,
                     UInt32 interfaceIndex)
{
    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsUeData* phyDataUe = phyUmts->phyDataUe;

    return phyDataUe->ulDediPhChConfig.numDPDCH;
}

// /**
// FUNCTION   :: UmtsMacPhyUeSetDpdchNumber
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: Get the number of data bit in BUffer
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// numUlDpdch  : unsigned char             : umber of dedicated DPDCH       
// RETURN     :: void : NULL
// **/
void UmtsMacPhyUeSetDpdchNumber(
                     Node* node,
                     UInt32 interfaceIndex,
                     unsigned char numUlDpdch)
{
    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsUeData* phyDataUe = phyUmts->phyDataUe;

    phyDataUe->ulDediPhChConfig.numDPDCH = numUlDpdch;

}

// /**
// FUNCTION   :: UmtsMacPhyUeSetHsdpaCqiReportInd
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: Set the hsdpa Cqi report request indicator
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// RETURN     :: void : NULL
// **/
void UmtsMacPhyUeSetHsdpaCqiReportInd(
                     Node* node,
                     UInt32 interfaceIndex)
{
    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsUeData* phyDataUe = phyUmts->phyDataUe;

    if (phyDataUe->hsdpaInfo.rcvdHspdschSig)
    {
        // only after rcvd first hsdpa  signal
        // need to report CQI
        // need consider more flexible report scheme
        phyDataUe->hsdpaInfo.repCqiNeeded = TRUE;
    }
}

// /**
// FUNCTION   :: UmtsMacPhyNodeBSetHsdpaConfig
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: Set the hsdpa configuration
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// numHspdsch  : unsigned char             : numer of HS-PDSCH
// hspdschId   : unsigned int*             : channel Id
// numHsscch   : unsigned char             : number of HS-SCCH
// hsscchId    : unsigned int*             : channel Id
// RETURN     :: void : NULL
// **/
void UmtsMacPhyNodeBSetHsdpaConfig(
                     Node* node,
                     UInt32 interfaceIndex,
                     unsigned char numHspdsch,
                     unsigned int* hspdschId,
                     unsigned char numHsscch,
                     unsigned int* hsscchId)
{
    int i;

    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsNodeBData* phyDataNodeB = phyUmts->phyDataNodeB;

    phyDataNodeB->hsdpaConfig.numHspdschAlloc = numHspdsch;
    for (i = 0; i < numHspdsch; i ++)
    {
        phyDataNodeB->hsdpaConfig.hspdschChId[i] = hspdschId[i];
    }

    phyDataNodeB->hsdpaConfig.numHsscchAlloc = numHsscch;
    for (i = 0; i < numHsscch; i ++)
    {
        phyDataNodeB->hsdpaConfig.hsscchChId[i] = hsscchId[i];
    }
}

// /**
// FUNCTION   :: UmtsMacPhyNodeBGetHsdpaConfig
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: Get the hsdpa configuration
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// numHspdsch  : unsigned char*             : numer of HS-PDSCH
// hspdschId   : unsigned int*             : channel Id
// numHsscch   : unsigned char*             : number of HS-SCCH
// hsscchId    : unsigned int*             : channel Id
// RETURN     :: void : NULL
// **/
void UmtsMacPhyNodeBGetHsdpaConfig(
                     Node* node,
                     UInt32 interfaceIndex,
                     unsigned char* numHspdsch,
                     unsigned int* hspdschId,
                     unsigned char* numHsscch,
                     unsigned int* hsscchId)
{
    int i;

    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsNodeBData* phyDataNodeB = phyUmts->phyDataNodeB;

    *numHspdsch = phyDataNodeB->hsdpaConfig.numHspdschAlloc;
    for (i = 0; i < *numHspdsch; i ++)
    {
        hspdschId[i] = phyDataNodeB->hsdpaConfig.hspdschChId[i] ;
    }

    *numHsscch = phyDataNodeB->hsdpaConfig.numHsscchAlloc;
    for (i = 0; i < *numHsscch; i ++)
    {
        hsscchId[i] = phyDataNodeB->hsdpaConfig.hsscchChId[i];
    }
}

// /**
// FUNCTION   :: UmtsMacPhyNodeBGetActiveHsdpaChNum
// LAYER      :: Layer 2
// PURPOSE    :: return the active HSDPA channels
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + numActiveCh : unsigned int* : Number of active HSDPA Channels
// RETURN     :: void : NULL
// **/
void UmtsMacPhyNodeBGetActiveHsdpaChNum(Node*node,
                                        UInt32 interfaceIndex,
                                        unsigned int* numActiveCh)
{
    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsNodeBData* phyDataNodeB = phyUmts->phyDataNodeB;

    *numActiveCh = phyDataNodeB->hsdpaConfig.numActiveCh;
}

// /**
// FUNCTION   :: UmtsMacPhyNodeBSetActiveHsdpaChNum
// LAYER      :: Layer 2
// PURPOSE    :: set the active HSDPA channels
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + numActiveCh : unsigned int : Number of active HSDPA Channels
// RETURN     :: void : NULL
// **/
void UmtsMacPhyNodeBSetActiveHsdpaChNum(Node*node,
                                 UInt32 interfaceIndex,
                                 unsigned int numActiveCh)
{
    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsNodeBData* phyDataNodeB = phyUmts->phyDataNodeB;

    phyDataNodeB->hsdpaConfig.numActiveCh = numActiveCh;

    // GUI
    if (node->guiOption)
    {
        GUI_SendIntegerData(node->nodeId,
                            phyDataNodeB->stats.numHsdpaChInUseGuiId,
                            numActiveCh,
                            node->getNodeTime());
    }
}

// /**
// FUNCTION   :: UmtsMacPhyNodeBGetUeCqiInfo
// LAYER      :: Layer 2
// PURPOSE    :: return the CQI value at UE/NodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + priSc : unsigned int : primry sc code
// + cqiVal : unsigned chr* : cqivl reproted from UE
// RETURN     :: void : NULL
// **/
void UmtsMacPhyNodeBGetUeCqiInfo(Node*node,
                                 UInt32 interfaceIndex,
                                 unsigned int priSc,
                                 unsigned char* cqiVal)
{
    PhyUmtsNodeBUeInfo* ueInfo = NULL;
    *cqiVal = 0;

    ueInfo = UmtsMacPhyNodeBFindUeInfo(node,
                              interfaceIndex,
                              priSc);

    if (ueInfo)
    {
        *cqiVal = ueInfo->cqiInfo.repVal;
    }
}

// /**
// FUNCTION   :: UmtsMacPhyNodeBUpdatePhChInfo
// LAYER      :: Layer 2
// PURPOSE    :: Update Physical channel info
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + trChType    : UmtsPhysicalChannelType   : physical channel type
// + chId        : unsigned int              : channel Identifier
// + upInfo      : UmtsPhChUpdateInfo*       : updated info
// RETURN     :: void : NULL
// **/ 
BOOL UmtsMacPhyNodeBUpdatePhChInfo(
                     Node* node,
                     UInt32 interfaceIndex,
                     UmtsPhysicalChannelType chType,
                     unsigned int chId,
                     UmtsPhChUpdateInfo*  upInfo)
{
    // get Phy data
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    std::list <PhyUmtsChannelData*>* phyChList;
    PhyUmtsChannelData* phChInfo;
    PhyCellularData* phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    PhyUmtsData* phyUmts = (PhyUmtsData *)phyCellular->phyUmtsData;
    PhyUmtsNodeBData* phyDataNodeB = phyUmts->phyDataNodeB;

    phyChList = phyDataNodeB->phyChList;


    phChInfo = UmtsMacGetPhChInfoFromList(
                    node,
                    phyChList,
                    chType,
                    chId);
    if (!phChInfo)
    {
        // REPORT ERROR
        ERROR_ReportError("No PhChInfo link with this phType and PhChId");
        return FALSE;
    }

    // update the channel info
    if (upInfo->modType)
    {
        phChInfo->moduType = *(upInfo->modType);
    }
    if (upInfo->gainFactor)
    {
        phChInfo->gainFactor = *(upInfo->gainFactor);
    }
    if (upInfo->nodeId)
    {
        phChInfo->targetNodeId = *(upInfo->nodeId);
    }

    return TRUE;
}

// /**
// FUNCTION   :: UmtsMacPhyGetSignalBitErrorRate
// LAYER      :: Layer 2
// PURPOSE    :: return the CQI value at UE/NodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + codingType : UmtsCodingType  : Coding type 
// + moduType   : UmtsModulationType : Modulation type
// + sinr       : double : sinr
// RETURN     :: double : BER
// **/
double UmtsMacPhyGetSignalBitErrorRate(
                     Node* node,
                     UInt32 interfaceIndex,
                     UmtsCodingType codingType,
                     UmtsModulationType moduType,
                     double sinr)
{
    int phyIndex = node->macData[interfaceIndex]->phyNumber;
    double codeRate = 0;
    int berTableIndex = 0;
    PhyData *thisPhy  = node->phyData[phyIndex];

    berTableIndex = PhyUmtsGetModCodeType(
                        codingType,
                        moduType,
                        & codeRate);

    return PHY_BER(thisPhy, berTableIndex, sinr);
}
