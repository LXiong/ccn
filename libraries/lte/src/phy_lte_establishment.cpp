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
#include <string.h>
#include <math.h>

#include "api.h"
#include "antenna.h"
#include "propagation.h"
#include "message.h"
#include "phy.h"

#ifdef LTE_LIB_LOG
#include "log_lte.h"
#endif

#include "phy_lte.h"
#include "layer2_lte.h"
#include "layer3_lte_filtering.h"
#include "phy_lte_establishment.h"

#ifdef ADDON_DB
#include "stats_phy.h"
#include "dbapi.h"
#endif

#define NEXT_FRAME  (2)

// select RSRP of the index 0
//#define SELECT_RSRP_INDEX0

// /**
// FUNCTION   :: PhyLteSetMessageRaPreambleInfo
// LAYER      :: PHY
// PURPOSE    :: Append the information of the preamble
// PARAMETERS ::
// + node     : Node*            : Pointer to node.
// + phyIndex : int              : Index of the PHY
// + preamble : PhyLteRaPreamble : Information of the RA preamble to append.
// + msg      : Message          : Tx Message
// RETURN     :: void : NULL
// **/
//
void PhyLteSetMessageRaPreambleInfo(
                              Node* node,
                              int phyIndex,
                              PhyLteRaPreamble* preamble,
                              Message* msg)
{
    MESSAGE_AppendInfo(node,
                       msg,
                       sizeof(PhyLteRaPreamble),
                       (int)INFO_TYPE_LtePhyRandamAccessTransmitPreamble);

    PhyLteRaPreamble* info = (PhyLteRaPreamble*)MESSAGE_ReturnInfo(
                              msg,
                              INFO_TYPE_LtePhyRandamAccessTransmitPreamble);

    memcpy(info, preamble, sizeof(PhyLteRaPreamble));

    return;
}

// /**
// FUNCTION   :: PhyLteCalculateRsrpRsrq
// LAYER      :: PHY
// PURPOSE    :: Calculate RSRP and RSRQ
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyLteCalculateRsrpRsrq(
                                    Node* node,
                                    int phyIndex,
                                    int channelIndex,
                                    PropRxInfo* propRxInfo)
{
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    // rxRnti
    Layer2DataLte* rxLteLyer2
                = LteLayer2GetLayer2DataLte(
                node,
                node->phyData[phyIndex]->macInterfaceIndex);
    LteRnti rxRnti = rxLteLyer2->myRnti;

    // rxPhyLte
    PhyDataLte* rxPhyLte = (PhyDataLte*)node->phyData[phyIndex]->phyVar;

    // txPhyLte
    PhyLteTxInfo* lteTxInfo = PhyLteGetMessageTxInfo(node,
                                                     phyIndex,
                                                     propRxInfo->txMsg);

    // propTxInfo
    PropTxInfo* propTxInfo
        = (PropTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg);

    // txNode
    Node* txNode = propTxInfo->txNode;

    // txPhyLte
    PhyDataLte* txPhyLte
        = (PhyDataLte*)txNode->phyData[propTxInfo->phyIndex]->phyVar;

    // rrcConfig
    LteRrcConfig* rrcConfig
        =  (LteRrcConfig*)MESSAGE_ReturnInfo(
                propRxInfo->txMsg,
                (unsigned short)INFO_TYPE_LtePhyPss);

    // Pass the filtering module RSRP
    LteLayer3Filtering* lteFiltering
        = LteLayer2GetLayer3Filtering(
                node,
                node->phyData[phyIndex]->macInterfaceIndex);

    /**
     *  Calculate the channel matrix
     */
    // pathloss
    double pathloss_dB
            = PhyLteGetPathloss_dB(node, phyIndex, lteTxInfo, propRxInfo);

    int matrixSize = rxPhyLte->numRxAntennas * lteTxInfo->numTxAnntena;
    Cmat channelMatrix(LTE_DEFAULT_DCOMP, matrixSize);
    PhyLteGetChannelMatrix(node,
                            phyIndex,
                            rxPhyLte,
                            propTxInfo,
                            lteTxInfo,
                            pathloss_dB,
                            channelMatrix);

    /**
     * Calculate RSRP
     */
    UInt8 numScPerRb
        = PhyLteGetNumSubcarriersPerRb(propTxInfo->txNode,
                                       propTxInfo->phyIndex);
    UInt8 txNumRb
            = PhyLteGetNumResourceBlocks(propTxInfo->txNode,
                                         propTxInfo->phyIndex);
    UInt8 rxNumRb
            = PhyLteGetNumResourceBlocks(node,
                                         phyIndex);
    int numRsResourceElementsInRb
            = PhyLteGetRsResourceElementsInRb(propTxInfo->txNode,
                                        propTxInfo->phyIndex);
    std::vector < double > rsrp_mW(rxPhyLte->numRxAntennas);
    double Pre_rs_mW = NON_DB(txPhyLte->maxTxPower_dBm)
                     / (txNumRb * numScPerRb);
    int txAntennaIndex = 0;

    Layer2DataLte* txLteLayer2
        = LteLayer2GetLayer2DataLte(
                txNode,
                txNode->phyData[propTxInfo->phyIndex]->macInterfaceIndex);
    LteRnti oppositeRnti = txLteLayer2->myRnti;

    double currentRsrp;
    BOOL rsrpExists =
            lteFiltering->get(oppositeRnti,
                              RSRP,
                              &currentRsrp);

    for (int i = 0; i < rxPhyLte->numRxAntennas; i++){
        int index = MATRIX_INDEX(i,txAntennaIndex,lteTxInfo->numTxAnntena);

        // If this is the first time to
        // register RSRP/RSRQ to filtering module,
        // Registered RSRP/RSRQ are calculated with ideal average pathloss.
        double g;
        if (rsrpExists == TRUE)
        {
            g = norm(channelMatrix[index]);
        }else
        {
            // First time
            g = 1.0 / NON_DB(pathloss_dB);
        }

        rsrp_mW[i] = g * Pre_rs_mW * numRsResourceElementsInRb;
    }

    // Select max of RSRPs
    double maxRsrp_mW  = rsrp_mW[0];
    int    maxIndex = 0;
#ifndef SELECT_RSRP_INDEX0
    for (int i = 1; i < rxPhyLte->numRxAntennas; i++){
        if (rsrp_mW[i] > maxRsrp_mW){
            maxRsrp_mW  = rsrp_mW[i];
            maxIndex = i;
        }
    }
#endif

    if (rrcConfig != NULL)
    {
        lteFiltering->update(
            (const LteRnti)oppositeRnti, RSRP,
            IN_DB(maxRsrp_mW), rrcConfig->filterCoefficient);
    }
    else
    {
        lteFiltering->update(
            (const LteRnti)oppositeRnti, RSRP, IN_DB(maxRsrp_mW));
    }

#ifdef LTE_LIB_LOG
    double filteredRsrp_dBm;
    lteFiltering->get(oppositeRnti, RSRP, &filteredRsrp_dBm);
    lte::LteLog::DebugFormat(
        node, node->phyData[phyIndex]->macInterfaceIndex,
        LTE_STRING_LAYER_TYPE_PHY,
        "%s,%s=,"LTE_STRING_FORMAT_RNTI",Instant=,%e,Filtered=,%e",
        LTE_STRING_CATEGORY_TYPE_RSRP,
        LTE_STRING_STATION_TYPE_ENB,
        oppositeRnti.nodeId, oppositeRnti.interfaceIndex,
        IN_DB(maxRsrp_mW),
        filteredRsrp_dBm);
#endif

    /**
     * Calculate RSRQ
     */
    double thermalNoise     = rxPhyLte->rbNoisePower_mW;
    double Pthermal_mW      = rxNumRb * thermalNoise;
    double Preceive_mW      = 0.0;
    double Pinterference_mW = 0.0;
    std::vector < double > rsrq(rxPhyLte->numRxAntennas);

    // Total interference power
    for (UInt8 i = 0; i < rxNumRb; i++)
    {
        Pinterference_mW += rxPhyLte->interferencePower_mW[i];
    }

    // Add the received power(if the UE is assigned RBs and has the TB)
    PhyLteRxMsgInfo* rxMsgInfo = NULL;
    UInt8 usedRB_list[PHY_LTE_MAX_NUM_RB];
    memset(usedRB_list, 0, sizeof(usedRB_list));
    int numRb = 0;
    std::list < PhyLteRxMsgInfo* > ::iterator it;
    for (it = rxPhyLte->rxPackMsgList->begin();
         it != rxPhyLte->rxPackMsgList->end();
         it ++)
    {
        rxMsgInfo = (*it);
        Message* rfMsg = rxMsgInfo->rxMsg;

        BOOL dciExists =
            PhyLteGetResourceAllocationInfo(
            node, phyIndex, rfMsg, usedRB_list, &numRb);

        if (dciExists == FALSE)
        {
            continue;
        }

        double rbPower_dBm = (*it)->lteTxInfo.txPower_dBm
                           - IN_DB(1.0/(*it)->geometry);
        Preceive_mW += NON_DB(rbPower_dBm) * numRb;
    }

    for (int i = 0; i < rxPhyLte->numRxAntennas; i++) {
        rsrq[i] = (txNumRb * rsrp_mW[i])
                / (Pthermal_mW + Pinterference_mW + Preceive_mW
                  + txNumRb * rsrp_mW[i]);
    }

    // Select max of RSRQs
    double maxRsrq  = rsrq[0];
    maxIndex = 0;
    for (int i = 1; i < rxPhyLte->numRxAntennas; i++){
        if (rsrq[i] > maxRsrq){
            maxRsrq  = rsrq[i];
            maxIndex = i;
        }
    }

    // Pass the filtering module RSRQ
    if (rrcConfig != NULL)
    {
        lteFiltering->update(
            oppositeRnti,
            RSRQ,
            IN_DB(maxRsrq),
            rrcConfig->filterCoefficient);
    }
    else
    {
        lteFiltering->update(
            oppositeRnti, RSRQ, IN_DB(maxRsrq));
    }

#ifdef LTE_LIB_LOG
    double filteredRsrq_dB;
    lteFiltering->get(oppositeRnti, RSRQ, &filteredRsrq_dB);
    lte::LteLog::DebugFormat(
        node, node->phyData[phyIndex]->macInterfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        "%s,%s=,"LTE_STRING_FORMAT_RNTI",Instant=,%e,Filtered=,%e",
        LTE_STRING_CATEGORY_TYPE_RSRQ,
        LTE_STRING_STATION_TYPE_ENB,
        oppositeRnti.nodeId, oppositeRnti.interfaceIndex,
        IN_DB(maxRsrq),
        filteredRsrq_dB);
#endif


    /* measurement for handover ********************************/
    // DlTtiInfo
    DlTtiInfo* ttiInfo =(DlTtiInfo*)MESSAGE_ReturnInfo(
                                propRxInfo->txMsg, 
                                (unsigned short)INFO_TYPE_LtePhyDlTtiInfo);
    // PhyLteEstablishmentData
    PhyLteEstablishmentData* establishmentData = rxPhyLte->establishmentData;
    //ERROR_Assert(
    //    ttiInfo == NULL,
    //    "unexpected error (PHY couldn't retrieve ttiNumber)");
    if (rxPhyLte->stationType == LTE_STATION_TYPE_UE
        && ttiInfo != NULL)
    {
        /** 
         * intra-frequency
         */
        // check whether measurement is needed at this subframe
        if (rxPhyLte->dlChannelNo == channelIndex
            && establishmentData->measureIntraFreq)
        {
            /**
             * RSRP_FOR_HO
             */
            {
                /* [S] duplicated from RSRP calcuration ********************/
                UInt8 numScPerRb 
                    = PhyLteGetNumSubcarriersPerRb(propTxInfo->txNode,
                                                   propTxInfo->phyIndex);
                UInt8 txNumRb
                        = PhyLteGetNumResourceBlocks(propTxInfo->txNode,
                                                     propTxInfo->phyIndex);
                int numRsResourceElementsInRb
                        = PhyLteGetRsResourceElementsInRb(propTxInfo->txNode,
                                                    propTxInfo->phyIndex);
                std::vector<double> rsrp_mW(rxPhyLte->numRxAntennas);
                double Pre_rs_mW = NON_DB(txPhyLte->maxTxPower_dBm)
                                 / (txNumRb * numScPerRb);
                int txAntennaIndex = 0;

                Layer2DataLte* txLteLayer2
                    = LteLayer2GetLayer2DataLte(
                        txNode, txNode->phyData[propTxInfo->phyIndex]
                            ->macInterfaceIndex);
                LteRnti oppositeRnti = txLteLayer2->myRnti;

                double currentRsrp;
                BOOL rsrpExists =
                        lteFiltering->get(oppositeRnti,
                                          RSRP_FOR_HO,
                                          &currentRsrp);

                for (int i=0; i<rxPhyLte->numRxAntennas; i++){
                    int index = MATRIX_INDEX(i,txAntennaIndex,
                        lteTxInfo->numTxAnntena);

                    // If this is the first time to register RSRP/RSRQ to
                    // filtering module, Registered RSRP/RSRQ are calculated
                    // with ideal average pathloss.
                    double g;
                    if (rsrpExists == TRUE )
                    {
                        g = norm(channelMatrix[index]);
                    }else
                    {
                        // First time
                        g = 1.0 / NON_DB( pathloss_dB );
                    }

                    rsrp_mW[i] = g * Pre_rs_mW * numRsResourceElementsInRb;
                }

                // Select max of RSRPs
                double maxRsrp_mW  = rsrp_mW[0];
                int    maxIndex = 0;
#ifndef SELECT_RSRP_INDEX0
                for (int i=1; i<rxPhyLte->numRxAntennas; i++){
                    if (rsrp_mW[i] > maxRsrp_mW){
                        maxRsrp_mW  = rsrp_mW[i];
                        maxIndex = i;
                    }
                }
#endif
                /* [E] duplicated from RSRP calcuration ********************/

                // L3F updated!
                lteFiltering->update(
                    (const LteRnti)oppositeRnti, RSRP_FOR_HO,
                    IN_DB(maxRsrp_mW), establishmentData->filterCoefRSRP);

                // register freq and rnti
                lteFiltering->registerFreqInfo(
                    RSRP_FOR_HO,
                    channelIndex,
                    &oppositeRnti);
            }

            /**
             * RSRQ_FOR_HO
             */
            {
                /* [S] duplicated from RSRS calcuration ********************/
                /**
                 * Calculate RSRQ
                 */
                double thermalNoise     = rxPhyLte->rbNoisePower_mW;
                double Pthermal_mW      = rxNumRb * thermalNoise;
                double Preceive_mW      = 0.0;
                double Pinterference_mW = 0.0;
                std::vector < double > rsrq(rxPhyLte->numRxAntennas);

                // Total interference power
                for (UInt8 i = 0; i < rxNumRb; i++)
                {
                    Pinterference_mW += rxPhyLte->interferencePower_mW[i];
                }

                // Add the received power
                // (if the UE is assigned RBs and has the TB)
                PhyLteRxMsgInfo* rxMsgInfo = NULL;
                UInt8 usedRB_list[PHY_LTE_MAX_NUM_RB];
                memset(usedRB_list, 0, sizeof(usedRB_list));
                int numRb = 0;
                std::list < PhyLteRxMsgInfo* > ::iterator it;
                for (it = rxPhyLte->rxPackMsgList->begin();
                     it != rxPhyLte->rxPackMsgList->end();
                     it ++)
                {
                    rxMsgInfo = (*it);
                    Message* rfMsg = rxMsgInfo->rxMsg;

                    BOOL dciExists =
                        PhyLteGetResourceAllocationInfo(
                        node, phyIndex, rfMsg, usedRB_list, &numRb);

                    if (dciExists == FALSE)
                    {
                        continue;
                    }

                    double rbPower_dBm = (*it)->lteTxInfo.txPower_dBm
                                       - IN_DB(1.0/(*it)->geometry);
                    Preceive_mW += NON_DB(rbPower_dBm) * numRb;
                }

                for (int i = 0; i < rxPhyLte->numRxAntennas; i++) {
                    rsrq[i] = (txNumRb * rsrp_mW[i])
                            / (Pthermal_mW + Pinterference_mW + Preceive_mW
                              + txNumRb * rsrp_mW[i]);
                }

                // Select max of RSRQs
                double maxRsrq  = rsrq[0];
                maxIndex = 0;
                for (int i = 1; i < rxPhyLte->numRxAntennas; i++){
                    if (rsrq[i] > maxRsrq){
                        maxRsrq  = rsrq[i];
                        maxIndex = i;
                    }
                }
                /* [E] duplicated from RSRQ calcuration ********************/

                // L3F updated!
                lteFiltering->update(
                    (const LteRnti)oppositeRnti, RSRQ_FOR_HO,
                    IN_DB(maxRsrq), establishmentData->filterCoefRSRQ);

                // register freq and rnti
                lteFiltering->registerFreqInfo(
                    RSRQ_FOR_HO,
                    channelIndex,
                    &oppositeRnti);
            }

            if (ttiInfo->ttiNumber %
                    establishmentData->intervalSubframeNum_intraFreq
                        == establishmentData->offsetSubframe_intraFreq)
            {
#ifdef LTE_LIB_LOG
                double filteredValue = 0;
                lteFiltering->get((const LteRnti)oppositeRnti,
                    RSRP_FOR_HO, &filteredValue);
                lte::LteLog::InfoFormat(
                    node, node->phyData[phyIndex]->macInterfaceIndex,
                    LTE_STRING_LAYER_TYPE_RRC,
                    LAYER3_LTE_MEAS_CAT_MEASUREMENT","
                    LTE_STRING_FORMAT_RNTI","
                    "[L3F updated],"
                    "measType,%d,value (instantaneous),%f,"
                    "value (filterd),%f,serving,"LTE_STRING_FORMAT_RNTI,
                    oppositeRnti.nodeId,
                    oppositeRnti.interfaceIndex,
                    RSRP_FOR_HO, IN_DB(maxRsrp_mW), filteredValue,
                    rxPhyLte->establishmentData->selectedRntieNB.nodeId,
                    rxPhyLte->establishmentData->selectedRntieNB.interfaceIndex);
                lteFiltering->get((const LteRnti)oppositeRnti,
                    RSRQ_FOR_HO, &filteredValue);
                lte::LteLog::InfoFormat(
                    node, node->phyData[phyIndex]->macInterfaceIndex,
                    LTE_STRING_LAYER_TYPE_RRC,
                    LAYER3_LTE_MEAS_CAT_MEASUREMENT","
                    LTE_STRING_FORMAT_RNTI","
                    "[L3F updated],"
                    "measType,%d,value (instantaneous),%f,"
                    "value (filterd),%f,serving,"LTE_STRING_FORMAT_RNTI,
                    oppositeRnti.nodeId,
                    oppositeRnti.interfaceIndex,
                    RSRQ_FOR_HO, IN_DB(maxRsrq), filteredValue,
                    rxPhyLte->establishmentData->selectedRntieNB.nodeId,
                    rxPhyLte->establishmentData->selectedRntieNB.interfaceIndex);
#endif
                // notify to RRC about measurement result updated
                Layer3LteIFHPNotifyL3FilteringUpdated(node, interfaceIndex);
            }
        }
        /** 
         * inter-frequency
         */
        if (rxPhyLte->dlChannelNo != channelIndex
            && rxPhyLte->rxState != PHY_LTE_CELL_SELECTION_MONITORING
            && rxPhyLte->monitoringState ==
                PHY_LTE_NON_SERVING_CELL_MONITORING)
        {
            /**
             * RSRP_FOR_HO
             */
            {
                /* [S] duplicated from RSRP calcuration ********************/
                UInt8 numScPerRb 
                    = PhyLteGetNumSubcarriersPerRb(propTxInfo->txNode,
                                                   propTxInfo->phyIndex);
                UInt8 txNumRb
                        = PhyLteGetNumResourceBlocks(propTxInfo->txNode,
                                                     propTxInfo->phyIndex);
                int numRsResourceElementsInRb
                        = PhyLteGetRsResourceElementsInRb(propTxInfo->txNode,
                                                    propTxInfo->phyIndex);
                std::vector<double> rsrp_mW(rxPhyLte->numRxAntennas);
                double Pre_rs_mW = NON_DB(txPhyLte->maxTxPower_dBm)
                                 / (txNumRb * numScPerRb);
                int txAntennaIndex = 0;

                Layer2DataLte* txLteLayer2
                    = LteLayer2GetLayer2DataLte(
                            txNode,
                            txNode->phyData[propTxInfo->phyIndex]
                                ->macInterfaceIndex);
                LteRnti oppositeRnti = txLteLayer2->myRnti;

                double currentRsrp;
                BOOL rsrpExists =
                        lteFiltering->get(oppositeRnti,
                                          RSRP_FOR_HO,
                                          &currentRsrp);

                for (int i=0; i<rxPhyLte->numRxAntennas; i++){
                    int index = MATRIX_INDEX(i,txAntennaIndex,
                        lteTxInfo->numTxAnntena);

                    // If this is the first time to register RSRP/RSRQ to
                    // filtering module, Registered RSRP/RSRQ are calculated
                    // with ideal average pathloss.
                    double g;
                    if (rsrpExists == TRUE )
                    {
                        g = norm(channelMatrix[index]);
                    }else
                    {
                        // First time
                        g = 1.0 / NON_DB( pathloss_dB );
                    }

                    rsrp_mW[i] = g * Pre_rs_mW * numRsResourceElementsInRb;
                }

                // Select max of RSRPs
                double maxRsrp_mW  = rsrp_mW[0];
                int    maxIndex = 0;
    #ifndef SELECT_RSRP_INDEX0
                for (int i=1; i<rxPhyLte->numRxAntennas; i++){
                    if (rsrp_mW[i] > maxRsrp_mW){
                        maxRsrp_mW  = rsrp_mW[i];
                        maxIndex = i;
                    }
                }
    #endif
                /* [E] duplicated from RSRP calcuration ********************/

                // L3F updated!
                lteFiltering->update(
                    (const LteRnti)oppositeRnti, RSRP_FOR_HO,
                    IN_DB(maxRsrp_mW), establishmentData->filterCoefRSRP);
#ifdef LTE_LIB_LOG
        double filteredValue = 0;
        lteFiltering->get((const LteRnti)oppositeRnti,
            RSRP_FOR_HO, &filteredValue);
        lte::LteLog::InfoFormat(
            node, node->phyData[phyIndex]->macInterfaceIndex,
            LTE_STRING_LAYER_TYPE_RRC,
            LAYER3_LTE_MEAS_CAT_MEASUREMENT","
            LTE_STRING_FORMAT_RNTI","
            "[L3F updated],"
            "measType,%d,value (instantaneous),%f,"
            "value (filterd),%f,serving,"LTE_STRING_FORMAT_RNTI,
            oppositeRnti.nodeId,
            oppositeRnti.interfaceIndex,
            RSRP_FOR_HO, IN_DB(maxRsrp_mW), filteredValue,
            rxPhyLte->establishmentData->selectedRntieNB.nodeId,
            rxPhyLte->establishmentData->selectedRntieNB.interfaceIndex);
#endif

                // register freq and rnti
                lteFiltering->registerFreqInfo(
                    RSRP_FOR_HO,
                    channelIndex,
                    &oppositeRnti);
            }
            // notify to RRC about measurement result updated
            Layer3LteIFHPNotifyL3FilteringUpdated(node, interfaceIndex);
        }

    }
}

// /**
// FUNCTION   :: PhyLteSignalArrivalFromChannelInEstablishment
// LAYER      :: PHY
// PURPOSE    :: Handle signal arrival from the channel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: BOOL : TRUE: some message info are appended
// **/
BOOL PhyLteSignalArrivalFromChannelInEstablishment(
                                    Node* node,
                                    int phyIndex,
                                    int channelIndex,
                                    PropRxInfo* propRxInfo)
{
    BOOL appendInfo = FALSE;
#ifdef ADDON_DB
    BOOL needToLogCtrlMsgForDB = FALSE;
#endif
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    PhyLteEstablishmentData* establishmentData = phyLte->establishmentData;

    PropTxInfo* propTxInfo
                = (PropTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg);
    PhyData* txThisPhy
                = propTxInfo->txNode->phyData[propTxInfo->phyIndex];
    PhyDataLte* txPhyLte = (PhyDataLte*)txThisPhy->phyVar;

    LteRnti rxRnti = LteRnti(
                        node->nodeId,
                        node->phyData[phyIndex]->macInterfaceIndex);
    PhyLteTxInfo* lteTxInfo = PhyLteGetMessageTxInfo(node,
                                                     phyIndex,
                                                     propRxInfo->txMsg);
    // DL
    if (phyLte->stationType == LTE_STATION_TYPE_UE)
    {
        // if TX Node is UE, return from this function
        if (txPhyLte->stationType == LTE_STATION_TYPE_UE)
        {
            appendInfo = FALSE;
            return appendInfo;
        }

        // Cell selection(Network disconnected/Network connected)
        if ((phyLte->rxState == PHY_LTE_CELL_SELECTION_MONITORING) ||
        (PhyLteIsInStationaryState(node, phyIndex) == true) &&
         (phyLte->monitoringState == PHY_LTE_NON_SERVING_CELL_MONITORING))
        {
            // Get the RRC config
            LteRrcConfig* info
                        =  (LteRrcConfig*)MESSAGE_ReturnInfo(
                                    propRxInfo->txMsg,
                                    (unsigned short)INFO_TYPE_LtePhyPss);

            if ((txPhyLte->stationType == LTE_STATION_TYPE_ENB) &&
                (info != NULL))
            {
                appendInfo = TRUE;

                // Store the information of the RRC config
                Node* txNode = propTxInfo->txNode;
                Layer2DataLte* txLteLayer2
                    = LteLayer2GetLayer2DataLte(
                                    txNode,
                                    txNode->phyData[propTxInfo->phyIndex]
                                          ->macInterfaceIndex);
                LteRnti txRnti = txLteLayer2->myRnti;

                // Rewrite the information if the List has one about txRnti.
                std::map < LteRnti, LteRrcConfig > ::iterator it;
                it = establishmentData->rxRrcConfigList->find(txRnti);

                if (it != establishmentData->rxRrcConfigList->end())
                {
                    establishmentData->rxRrcConfigList->erase(it);
                }

                establishmentData->rxRrcConfigList->insert(
                    std::pair < LteRnti, LteRrcConfig > (txRnti, *info));
#ifdef ADDON_DB
                needToLogCtrlMsgForDB = TRUE;
#endif
            }
        }

        // Measure propagation delay from the RA Grant message
        if (phyLte->rxState == PHY_LTE_RA_GRANT_WAITING)
        {
            // Take the information of the RA grant from the message
            int infoSize = MESSAGE_ReturnInfoSize(propRxInfo->txMsg,
                        (unsigned short)INFO_TYPE_LtePhyRandamAccessGrant);
            int numGrant = infoSize / sizeof(LteRaGrant);
            LteRaGrant* grantArray
                    = (LteRaGrant*)MESSAGE_ReturnInfo(propRxInfo->txMsg,
                        (unsigned short)INFO_TYPE_LtePhyRandamAccessGrant);
            LteRnti myRnti = LteRnti(node->nodeId,
                                node->phyData[phyIndex]->macInterfaceIndex);

            for (int i = 0; i < numGrant; i++)
            {
                if (grantArray[i].ueRnti == myRnti)
                {
                    // Set the propagation delay
                     PhyLteSetPropagationDelay(node, phyIndex, propTxInfo);

                    // Notify reception of RA Grant
                    MacLteNotifyReceivedRaGrant(
                            node,
                            node->phyData[phyIndex]->macInterfaceIndex,
                            grantArray[i]);

                    // Set RRC config
                    SetLteRrcConfig(
                            node,
                            node->phyData[phyIndex]->macInterfaceIndex,
                            establishmentData->selectedRrcConfig);

#ifdef ADDON_DB
                    needToLogCtrlMsgForDB = TRUE;
#endif
                }
            }
        }
    }
    // UL
    else
    {
        // if TX Node is eNB, return from this function
        if (txPhyLte->stationType == LTE_STATION_TYPE_ENB)
        {
            appendInfo = FALSE;
            return appendInfo;
        }

        // Take the information of the preamble from the message
        PhyLteRaPreamble* info
            = (PhyLteRaPreamble*)MESSAGE_ReturnInfo(
                            propRxInfo->txMsg,
                            INFO_TYPE_LtePhyRandamAccessTransmitPreamble);

        if ((info != NULL) &&
            (rxRnti == info->targetEnbRnti))
        {
            appendInfo = TRUE;

#ifdef LTE_LIB_LOG
        lte::LteLog::InfoFormat(
            node, node->phyData[phyIndex]->macInterfaceIndex,
            LTE_STRING_LAYER_TYPE_PHY,
            "%s,Rx,%s,UE=,"LTE_STRING_FORMAT_RNTI,
            LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
            LTE_STRING_MESSAGE_TYPE_RA_PREAMBLE,
            propRxInfo->txNodeId, propRxInfo->txPhyIndex);
#endif
            // Calculate the received power
            double pathloss_dB
                    = PhyLteGetPathloss_dB(node,
                                           phyIndex,
                                           lteTxInfo,
                                           propRxInfo);
            int matrixSize
                    = phyLte->numRxAntennas * lteTxInfo->numTxAnntena;
            Cmat channelMatrix(LTE_DEFAULT_DCOMP, matrixSize);
            PhyLteGetChannelMatrix(node,
                                    phyIndex,
                                    phyLte,
                                    propTxInfo,
                                    lteTxInfo,
                                    pathloss_dB,
                                    channelMatrix);
            double rxPower_mW
                    = norm(channelMatrix[0]) * info->txPower_mW
                    * PHY_LTE_PRACH_RB_NUM;

            // Set the information of the preamble receiving currently
            PhyLteReceivingRaPreamble rxPreamble;
            memcpy(&rxPreamble.raPreamble, info, sizeof(PhyLteRaPreamble));
            rxPreamble.txMsg           = propRxInfo->txMsg;
            rxPreamble.collisionFlag   = FALSE;
            rxPreamble.arrivalTime     = node->getNodeTime();
            rxPreamble.duration        = propRxInfo->duration;
            rxPreamble.rxPower_mW      = rxPower_mW;

            // Detect collisions
            std::list < PhyLteReceivingRaPreamble > ::iterator it;
            for (it = establishmentData->rxPreambleList->begin();
                it != establishmentData->rxPreambleList->end();
                it ++)
            {
                // assigned resource is overlapped
                if ((rxPreamble.raPreamble.preambleIndex ==
                            (*it).raPreamble.preambleIndex) &&
                     (((rxPreamble.raPreamble.prachResourceStart >=
                            (*it).raPreamble.prachResourceStart) &&
                       (rxPreamble.raPreamble.prachResourceStart <=
                            (*it).raPreamble.prachResourceStart
                                                + PHY_LTE_PRACH_RB_NUM)) ||
                      (((*it).raPreamble.prachResourceStart >=
                            rxPreamble.raPreamble.prachResourceStart) &&
                       ((*it).raPreamble.prachResourceStart <=
                            rxPreamble.raPreamble.prachResourceStart
                                            + PHY_LTE_PRACH_RB_NUM))) &&
                     (((rxPreamble.arrivalTime >=
                       (*it).arrivalTime) &&
                       (rxPreamble.arrivalTime <=
                            (*it).arrivalTime + (*it).duration)) ||
                      (((*it).arrivalTime >=
                            rxPreamble.arrivalTime) &&
                       ((*it).arrivalTime <=
                            rxPreamble.arrivalTime
                                            + rxPreamble.duration))))
                {
                    rxPreamble.collisionFlag = TRUE;
                    (*it).collisionFlag = TRUE;
                }
            }
            // Insert the information of the preamble
            establishmentData->rxPreambleList->push_back(rxPreamble);
#ifdef ADDON_DB
            needToLogCtrlMsgForDB = TRUE;
#endif
        }
    }
#ifdef ADDON_DB
    if (needToLogCtrlMsgForDB
        && phyLte->thisPhy->phyStats
        && (((channelIndex != phyLte->ulChannelNo)
              && (phyLte->stationType == LTE_STATION_TYPE_ENB))
           || ((channelIndex != phyLte->dlChannelNo)
              && (phyLte->stationType == LTE_STATION_TYPE_UE))))
    {
        phyLte->thisPhy->stats->AddSignalLockedDataPoints(
            node,
            propRxInfo,
            phyLte->thisPhy,
            channelIndex,
            NON_DB(lteTxInfo->txPower_dBm
                    - PhyLteGetPathloss_dB(node,
                                           phyIndex,
                                           lteTxInfo,
                                           propRxInfo)));
    }
#endif
    return appendInfo;
}

// /**
// FUNCTION   :: LteSignalEndFromChannelInEstablishment
// LAYER      :: PHY
// PURPOSE    :: Handle signal end from a channel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyLteSignalEndFromChannelInEstablishment(
                                Node* node,
                                int phyIndex,
                                int channelIndex,
                                PropRxInfo* propRxInfo)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    PhyLteEstablishmentData* establishmentData = phyLte->establishmentData;
    LteRnti rxRnti = LteRnti(
                        node->nodeId,
                        node->phyData[phyIndex]->macInterfaceIndex);
    PropTxInfo* propTxInfo
                    = (PropTxInfo*)MESSAGE_ReturnInfo(propRxInfo->txMsg);
    PhyDataLte* txPhyLte = (PhyDataLte*)
        (propTxInfo->txNode->phyData[propTxInfo->phyIndex]->phyVar);
#ifdef ADDON_DB
    BOOL needToLogCtrlMsgForDB = FALSE;
#endif

    // DL
    if (phyLte->stationType == LTE_STATION_TYPE_UE)
    {
        // if TX Node is UE, return from this function
        if (txPhyLte->stationType == LTE_STATION_TYPE_UE)
        {
            return;
        }

#ifdef ADDON_DB
        // first check for RRC Config info
        LteRrcConfig* rrcInfo =  (LteRrcConfig*)MESSAGE_ReturnInfo(
                                                propRxInfo->txMsg,
                                                (UInt16)INFO_TYPE_LtePhyPss);
        if (rrcInfo)
        {
            needToLogCtrlMsgForDB = TRUE;
        }
#endif
        // Calculate RSRP, RSRQ
        PhyLteCalculateRsrpRsrq(node,
                                phyIndex,
                                channelIndex,
                                propRxInfo);

        if (phyLte->rxState == PHY_LTE_RA_GRANT_WAITING)
        {
            // Take the information of the RA grant from the message
            int infoSize = MESSAGE_ReturnInfoSize(propRxInfo->txMsg,
                        (unsigned short)INFO_TYPE_LtePhyRandamAccessGrant);
            int numGrant = infoSize / sizeof(LteRaGrant);
            LteRaGrant* grantArray
                    = (LteRaGrant*)MESSAGE_ReturnInfo(propRxInfo->txMsg,
                        (unsigned short)INFO_TYPE_LtePhyRandamAccessGrant);
            LteRnti myRnti = LteRnti(node->nodeId,
                                node->phyData[phyIndex]->macInterfaceIndex);
            Int32 i;
            for (i = 0; i < numGrant; i++)
            {
                if (grantArray[i].ueRnti == myRnti)
                {
#ifdef LTE_LIB_LOG
                    lte::LteLog::InfoFormat(
                        node, node->phyData[phyIndex]->macInterfaceIndex,
                        LTE_STRING_LAYER_TYPE_PHY,
                        "%s,Rx,%s,eNB=,"LTE_STRING_FORMAT_RNTI,
                        LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
                        LTE_STRING_MESSAGE_TYPE_RA_GRANT,
                        propRxInfo->txNodeId, propRxInfo->txPhyIndex);
#endif
#ifdef ADDON_DB
                    needToLogCtrlMsgForDB = TRUE;
#endif
                    // Change state
                    PhyLteChangeState(node,
                                      phyIndex,
                                      phyLte,
                                      PHY_LTE_DL_IDLE);
                    PhyLteChangeState(node,
                                      phyIndex,
                                      phyLte,
                                      PHY_LTE_UL_IDLE);
#ifdef LTE_LIB_USE_ONOFF
                    // Set a NonServingCellMeasurementInterval timer
                    PhyLteSetNonServingCellMeasurementIntervalTimer(
                                                                node,
                                                                phyIndex);
#endif
                }
            }
        }
    }
    // UL
    else
    {
        // if TX Node is eNB, return from this function
        if (txPhyLte->stationType == LTE_STATION_TYPE_ENB)
        {
            return;
        }

        // Take the information of the preamble from the message
        PhyLteRaPreamble* info
            = (PhyLteRaPreamble*)MESSAGE_ReturnInfo(
                            propRxInfo->txMsg,
                            INFO_TYPE_LtePhyRandamAccessTransmitPreamble);

        // Accept the preamble
#ifdef LTE_LIB_LOG
        std::ostringstream rxString;
        BOOL accept = FALSE;
#endif
        LteRnti ueRnti;
        double rxPower_mW;
        if ((info != NULL) &&
            (rxRnti == info->targetEnbRnti))
        {
            std::list<PhyLteReceivingRaPreamble>::iterator it;

            for (it = establishmentData->rxPreambleList->begin();
                   it != establishmentData->rxPreambleList->end();
                   it++)
            {
                if (propRxInfo->txMsg == (*it).txMsg)
                {
                    ueRnti      = (*it).raPreamble.raRnti;
                    rxPower_mW  = (*it).rxPower_mW;
                    double prachRxPower_dBm
                        = IN_DB((*it).rxPower_mW);
#ifdef LTE_LIB_LOG
                    rxString << "Rnti=,[" << ueRnti.nodeId << " "
                             << ueRnti.interfaceIndex << "]"
                             << ",dBm=," << prachRxPower_dBm
                             << ",collisionFlag=,"
                             << ((*it).collisionFlag ? "TRUE" : "FALSE");
#endif
                    if (((*it).collisionFlag == FALSE) &&
                        (prachRxPower_dBm >=
                            phyLte->raDetectionThreshold_dBm))
                    {
                        // #3
                        // If requested preamble is from the UE existing in
                        // the phyLte->ueInfoList, RA shall be denied until
                        // eNB detects lost of the UE spontaneously.
                        if (phyLte->ueInfoList->find((*it).raPreamble.raRnti)
                            != phyLte->ueInfoList->end())
                        {

#ifdef LTE_LIB_LOG
                            LteRnti ueRnti = (*it).raPreamble.raRnti;
                            char buf[MAX_STRING_LENGTH];

                            lte::LteLog::WarnFormat(
                                node, node->phyData[phyIndex]->macInterfaceIndex,
                                LTE_STRING_LAYER_TYPE_PHY,
                                "UE %s already exists in phyLte->ueInfoList."
                                " Random access is denied.",
                                STR_RNTI(buf, ueRnti));
#endif

                        }else
                        {
#ifdef LTE_LIB_LOG
                            accept = TRUE;
#endif // LTE_LIB_LOG

                            MacLteNotifyReceivedRaPreamble(
                                node,
                                node->phyData[phyIndex]->macInterfaceIndex,
                                (*it).raPreamble.raRnti,
                                (*it).raPreamble.isHandingOverRa);
                        }

                    }

                    establishmentData->rxPreambleList->erase(it);

                    // No need to seek more.
                    break;
                }
            }
#ifdef ADDON_DB
            needToLogCtrlMsgForDB = TRUE;
#endif
#ifdef LTE_LIB_LOG
            lte::LteLog::DebugFormat(
                node, node->phyData[phyIndex]->macInterfaceIndex,
                LTE_STRING_LAYER_TYPE_PHY,
                "%s,%s,%s",
                LTE_STRING_CATEGORY_TYPE_RA_PREAMBLE_RX,
                rxString.str().c_str(),
                (accept ? "ACCEPT" : "DISCARD"));
#endif // LTE_LIB_LOG
        }
    }
#ifdef ADDON_DB
    if (needToLogCtrlMsgForDB
        && (((channelIndex != phyLte->ulChannelNo)
              && (phyLte->stationType == LTE_STATION_TYPE_ENB))
           || ((channelIndex != phyLte->dlChannelNo)
              && (phyLte->stationType == LTE_STATION_TYPE_UE))))
    {
        PhyLteUpdateEventsTable(node,
                                phyIndex,
                                channelIndex,
                                propRxInfo,
                                NULL,
                                "PhyReceiveSignal");

        if (phyLte->thisPhy->phyStats)
        {
            phyLte->thisPhy->stats->AddSignalUnLockedDataPoints(
                                            node,
                                            propRxInfo->txMsg);
        }
    }
#endif
}

// /**
// FUNCTION   :: PhyLteStartTransmittingSignalInEstablishment
// LAYER      :: PHY
// PURPOSE    :: Start transmitting a frame
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : int   : Index of the PHY
// + rxmsg     : Message* : Message associated with start transmitting event.
// RETURN     :: void : NULL
// **/
void PhyLteStartTransmittingSignalInEstablishment(
                                Node* node,
                                int phyIndex,
                                Message* rxmsg)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

#ifdef LTE_LIB_LOG
    PhyLteEstablishmentData* establishmentData = phyLte->establishmentData;

    lte::LteLog::InfoFormat(
        node, node->phyData[phyIndex]->macInterfaceIndex,
        LTE_STRING_LAYER_TYPE_PHY,
        "%s,Tx,%s,eNB=,"LTE_STRING_FORMAT_RNTI,
        LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
        LTE_STRING_MESSAGE_TYPE_RA_PREAMBLE,
        establishmentData->selectedRntieNB.nodeId,
        establishmentData->selectedRntieNB.interfaceIndex);
#endif

    // Message info: delay
    clocktype* delayUntilAirborne =
            (clocktype*)MESSAGE_ReturnInfo(
            rxmsg,
            INFO_TYPE_LtePhyRandamAccessTransmitPreambleTimerDelay);

    // Message info: preamble info
    PhyLteRaPreamble* preambleInfo =
            (PhyLteRaPreamble*)MESSAGE_ReturnInfo(
            rxmsg,
            INFO_TYPE_LtePhyRandamAccessTransmitPreambleInfo);

#ifdef LTE_LIB_LOG
            lte::LteLog::DebugFormat(
                node, node->phyData[phyIndex]->macInterfaceIndex,
                LTE_STRING_LAYER_TYPE_PHY,
                "%s,%s=,"LTE_STRING_FORMAT_RNTI
                ",mW=,%e,PreambleId=,%d,PrachStart=,%d",
                LTE_STRING_CATEGORY_TYPE_RA_PREAMBLE_TX,
                LTE_STRING_STATION_TYPE_ENB,
                preambleInfo->targetEnbRnti.nodeId,
                preambleInfo->targetEnbRnti.interfaceIndex,
                preambleInfo->txPower_mW,
                preambleInfo->preambleIndex,
                preambleInfo->prachResourceStart);
#endif

    // Transmit the preamble immediately.
    int channelIndex            = 0;
    clocktype duration          = PHY_LTE_PREAMBLE_DURATION;

    Message* msg;

    msg = MESSAGE_Alloc(node, PHY_LAYER, 0, 0);

    PhyLteSetMessageRaPreambleInfo(node,
                                 phyIndex,
                                 preambleInfo,
                                 msg);

#ifdef ADDON_DB
    LteMacStatsDBCheckForMsgSequenceNum(node, msg);
#endif

    double txPower_dBm = IN_DB(preambleInfo->txPower_mW);
    PhyLteSetMessageTxInfo(node, phyIndex, phyLte, txPower_dBm, msg);

    PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);

#ifdef ADDON_DB
    if (phyLte->thisPhy->phyStats)
    {
        phyLte->thisPhy->stats->AddSignalTransmittedDataPoints(node,
                                                               duration);
    }
#endif

    PROP_ReleaseSignal(node,
                       msg,
                       phyIndex,
                       channelIndex,
                       (float)(IN_DB(preambleInfo->txPower_mW)),
                       duration,
                       *delayUntilAirborne);

    // Change state
    PhyLteChangeState(node,
                      phyIndex,
                      phyLte,
                      PHY_LTE_RA_PREAMBLE_TRANSMITTING);

#ifdef LTE_LIB_LOG
    lte::LteLog::Debug2Format(
        node,
        node->phyData[phyIndex]->macInterfaceIndex,
        "PHY:DEBUG:PhyLteStartTransmittingSignalInEstablishment:",
        "Preamble");
#endif // LTE_LIB_LOG

    phyLte->eventMsgList->erase(
                        MSG_PHY_LTE_StartTransmittingSignalInEstablishment);

    // Set the transmissionEnd timer
    Message* endMsg;

    endMsg = MESSAGE_Alloc(node,
                        PHY_LAYER,
                        0,
                        MSG_PHY_LTE_TransmissionEndInEstablishment);
    MESSAGE_SetInstanceId(endMsg, (short) phyIndex);

    phyLte->numTxMsgs++;
    phyLte->txEndMsgList->push_back(endMsg);

    MESSAGE_Send(node, endMsg, *delayUntilAirborne + duration);
}

// /**
// FUNCTION   :: PhyLteTransmissionEndInEstablishment
// LAYER      :: PHY
// PURPOSE    :: End of the transmission
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// + msg       : Message* : Tx End event
// RETURN     :: void  : NULL
// **/
void PhyLteTransmissionEndInEstablishment(
                                Node* node,
                                int phyIndex,
                                Message* msg)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    // remove from the message list kept for cancel purpose
    std::list < Message* > ::iterator it;
    for (it = phyLte->txEndMsgList->begin();
         it != phyLte->txEndMsgList->end();
         it++)
    {
        if ((*it) == msg)
        {
            phyLte->txEndMsgList->erase(it);
            break;
        }
    }
    phyLte->numTxMsgs--;

    // Change state
    if (phyLte->txState == PHY_LTE_RA_PREAMBLE_TRANSMITTING)
    {
        PhyLteChangeState(node,
                          phyIndex,
                          phyLte,
                          PHY_LTE_RA_GRANT_WAITING);
    }
}

// /**
// FUNCTION   :: PhyLteSetNonServingCellMeasurementIntervalTimer
// LAYER      :: PHY
// PURPOSE    :: Set a NonServingCellMeasurementInterval Timer
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteSetNonServingCellMeasurementIntervalTimer(
                                Node* node,
                                int phyIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    Message* newMsg;
    newMsg = MESSAGE_Alloc(node,
                           PHY_LAYER,
                           0,
                           MSG_PHY_LTE_NonServingCellMeasurementInterval);

    MESSAGE_SetInstanceId(newMsg, (short) phyIndex);

    phyLte->eventMsgList->insert(
            std::pair < int, Message* > (
            MSG_PHY_LTE_NonServingCellMeasurementInterval, newMsg));

    MESSAGE_Send(node, newMsg, phyLte->nonServingCellMeasurementInterval);
}

// /**
// FUNCTION   :: PhyLteSetNonServingCellMeasurementPeriodTimer
// LAYER      :: PHY
// PURPOSE    :: Set a NonServingCellMeasurementPeriod Timer
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteSetNonServingCellMeasurementPeriodTimer(
                                Node* node,
                                int phyIndex,
                                BOOL isForHoMeasurement)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    Message* newMsg;
    newMsg = MESSAGE_Alloc(node,
                           PHY_LAYER,
                           0,
                           MSG_PHY_LTE_NonServingCellMeasurementPeriod);

    MESSAGE_SetInstanceId(newMsg, (short) phyIndex);

    phyLte->eventMsgList->insert(
            std::pair < int, Message* > (
            MSG_PHY_LTE_NonServingCellMeasurementPeriod, newMsg));

    clocktype delay = isForHoMeasurement ? 
            phyLte->nonServingCellMeasurementForHoPeriod :
            phyLte->nonServingCellMeasurementPeriod;
    MESSAGE_Send(node, newMsg, delay);
}

// /**
// FUNCTION   :: PhyLteSetTransmissionPreambleTimerMessageInfo
// LAYER      :: PHY
// PURPOSE    :: Append the information of the delay
// PARAMETERS ::
// + node               : Node*            : Pointer to node.
// + phyIndex           : int              : Index of the PHY
// + txPreamble         : PhyLteRaPreamble : RA preamble to transmit
// + delayUntilAirborne : clocktype        : Delay until airborne
// + msg                : Message*         : Tx Message
// RETURN     :: void : NULL
// **/
//
void PhyLteSetTransmissionPreambleTimerMessageInfo(
                              Node* node,
                              int phyIndex,
                              PhyLteRaPreamble txPreamble,
                              clocktype delayUntilAirborne,
                              Message* msg)
{
    // preamble info
    MESSAGE_AppendInfo(
            node,
            msg,
            sizeof(PhyLteRaPreamble),
            (int)INFO_TYPE_LtePhyRandamAccessTransmitPreambleInfo);

    PhyLteRaPreamble* preambleInfo =
                (PhyLteRaPreamble*)MESSAGE_ReturnInfo(
                msg,
                INFO_TYPE_LtePhyRandamAccessTransmitPreambleInfo);

    memcpy(preambleInfo, &txPreamble, sizeof(PhyLteRaPreamble));

    // delay
    MESSAGE_AppendInfo(
            node,
            msg,
            sizeof(clocktype),
            (int)INFO_TYPE_LtePhyRandamAccessTransmitPreambleTimerDelay);

    clocktype* delayInfo =
                (clocktype*)MESSAGE_ReturnInfo(
                msg,
                INFO_TYPE_LtePhyRandamAccessTransmitPreambleTimerDelay);

    *delayInfo = delayUntilAirborne;

    return;
}

// /**
// FUNCTION   :: PhyLteSetNextTrannsmissionPreambleTimer
// LAYER      :: PHY
// PURPOSE    :: Set a NonServingCellMeasurementPeriod Timer
// PARAMETERS ::
// + node               : Node*  : Pointer to node.
// + phyIndex           : int    : Index of the PHY
// + timerDuration      : clocktype          : Timer duration
// + txPreamble         : PhyLteRaPreamble   : RA Preamble to transmit
// + delayUntilAirborne : delayUntilAirborne :
//                          Delay until airborne of RA preamble.
// RETURN     :: void  : NULL
// **/
void PhyLteSetNextTrannsmissionPreambleTimer(
                                Node* node,
                                int phyIndex,
                                clocktype timerDuration,
                                PhyLteRaPreamble txPreamble,
                                clocktype delayUntilAirborne)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    Message* newMsg;
    newMsg = MESSAGE_Alloc(
                    node,
                    PHY_LAYER,
                    0,
                    (int)MSG_PHY_LTE_StartTransmittingSignalInEstablishment);

    PhyLteSetTransmissionPreambleTimerMessageInfo(
                                                  node,
                                                  phyIndex,
                                                  txPreamble,
                                                  delayUntilAirborne,
                                                  newMsg);

    MESSAGE_SetInstanceId(newMsg, (short) phyIndex);

    phyLte->eventMsgList->insert(
                std::pair < int, Message* > (
                MSG_PHY_LTE_StartTransmittingSignalInEstablishment, newMsg));

    MESSAGE_Send(node, newMsg, timerDuration);
}

// /**
// FUNCTION   :: PhyLteStartCellSearch
// LAYER      :: PHY
// PURPOSE    :: Start cell search
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteStartCellSearch(Node* node,
                           int phyIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    ERROR_Assert(phyLte->stationType == LTE_STATION_TYPE_UE, "");

    if (phyLte->rxState == PHY_LTE_IDLE_CELL_SELECTION ||
        phyLte->rxState == PHY_LTE_IDLE_RANDOM_ACCESS)
    {
        // Change state
        PhyLteChangeState(node,
                          phyIndex,
                          phyLte,
                          PHY_LTE_CELL_SELECTION_MONITORING);

#ifdef LTE_LIB_LOG
        lte::LteLog::InfoFormat(
            node, node->phyData[phyIndex]->macInterfaceIndex,
            LTE_STRING_LAYER_TYPE_PHY,
            "%s,%s",
            LTE_STRING_CATEGORY_TYPE_CELL_SELECTION,
            LTE_STRING_PROCESS_TYPE_START);
#endif

        // Set all of the listening channel mask flags
        int numChannels = PROP_NumberChannels(node);
        for (int i = 0; i < numChannels; i++) {


            if ((PHY_CanListenToChannel(node, phyIndex, i)) &&
                (!PHY_IsListeningToChannel(node, phyIndex, i)))
            {
                PHY_StartListeningToChannel(node, phyIndex, i);
            }
        }

        // Start measuring of RSRP and RSRQ
        phyLte->monitoringState = PHY_LTE_NON_SERVING_CELL_MONITORING;

        // Set a NonServingCellMeasurementPeriod timer
        PhyLteSetNonServingCellMeasurementPeriodTimer(node, phyIndex, FALSE);
    }
}

// /**
// FUNCTION   :: PhyLteNonServingCellMeasurementIntervalExpired
// LAYER      :: PHY
// PURPOSE    :: End of the measurement interval
// NOTE       :: Note that this function is never called
//               if LTE_LIB_USE_ONOFF is not defined.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteNonServingCellMeasurementIntervalExpired(Node* node,
                                                    int phyIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    PhyLteEstablishmentData* establishmentData = phyLte->establishmentData;

    if (!establishmentData->measureInterFreq)
    {
        return;
    }

    // Change the state and start measuring of RSRP and RSRQ
    phyLte->monitoringState = PHY_LTE_NON_SERVING_CELL_MONITORING;

    phyLte->eventMsgList->erase(
                            MSG_PHY_LTE_NonServingCellMeasurementInterval);

    // Set a NonServingCellMeasurementInterval timer
    PhyLteSetNonServingCellMeasurementIntervalTimer(node, phyIndex);

    // Set all of the listening channel flags
    int numChannels = PROP_NumberChannels(node);
    for (int i = 0; i < numChannels; i++) {
        if ((PHY_CanListenToChannel(node, phyIndex, i)) &&
           (!PHY_IsListeningToChannel(node, phyIndex, i)))
        {
            PHY_StartListeningToChannel(node, phyIndex, i);
        }
    }

    // Set a NonServingCellMeasurementPeriod timer
    PhyLteSetNonServingCellMeasurementPeriodTimer(node, phyIndex, TRUE);
}

// /**
// FUNCTION   :: PhyLteNonServingCellMeasurementPeriodExpired
// LAYER      :: PHY
// PURPOSE    :: Callback of end of the measurement period
// NOTE       :: Note that this function is called only one time
//               if LTE_LIB_USE_ONOFF is not defined.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteNonServingCellMeasurementPeriodExpired(Node* node,
                                                  int phyIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    PhyLteEstablishmentData* establishmentData = phyLte->establishmentData;

    phyLte->eventMsgList->erase(MSG_PHY_LTE_NonServingCellMeasurementPeriod);

    // Change the state and stop measuring of RSRP and RSRQ
    phyLte->monitoringState = PHY_LTE_NON_SERVING_CELL_MONITORING_IDLE;

    // if RRC_CONNECTED, don't go to cell selection proccess.
    Layer3DataLte* layer3Data = LteLayer2GetLayer3DataLte(
        node,  node->phyData[phyIndex]->macInterfaceIndex);
    if (layer3Data->layer3LteState == LAYER3_LTE_RRC_CONNECTED)
    {
        int numChannels = PROP_NumberChannels(node);
        for (int i = 0; i < numChannels; i++)
        {
            if ((phyLte->dlChannelNo != i) &&
                (PHY_IsListeningToChannel(node, phyIndex, i)))
            {
                // stop listening to non-serving cell channel
                PHY_StopListeningToChannel(node, phyIndex, i);
            }
        }
        return;
    }

    /**
     * Select a Cell
     */
    // Calculate Srxlev and Squal
    std::map < LteRnti, LteRrcConfig > ::iterator it;
    it = establishmentData->rxRrcConfigList->begin();

    LteRrcConfig selectedRrcConfig;
    LteRnti      selectedRntieNB  = LTE_INVALID_RNTI;
    double       selectedRsrp_dBm = LTE_INFINITY_POWER_dBm;
    BOOL         selected = FALSE;
#ifdef LTE_LIB_LOG
    std::ostringstream candidateString;
#endif // LTE_LIB_LOG

    while (it != establishmentData->rxRrcConfigList->end())
    {
        LteLayer3Filtering* lteFiltering
                            = LteLayer2GetLayer3Filtering(
                                node,
                                node->phyData[phyIndex]->macInterfaceIndex);
        const LteRnti oppositeRnti = it->first;

        double Srxlev_dBm;
        double Squal_dB;
        double Qrxlevmeas_dBm  = LTE_INFINITY_POWER_dBm;
        double Qqualmeas_dB    = LTE_INFINITY_POWER_dBm;
        double Qrxlevmin_dBm
            = it->second.ltePhyConfig.cellSelectionRxLevelMin_dBm;
        double Qqualmin_dB
            = it->second.ltePhyConfig.cellSelectionQualLevelMin_dB;
        double Qrxlevminoffset_dB
            = it->second.ltePhyConfig.cellSelectionRxLevelMinOff_dBm;
        double Qqualminoffset_dB
            = it->second.ltePhyConfig.cellSelectionQualLevelMinOffset_dB;
        // Pcompensation_dB is always 0
        double Pcompensation_dB   = 0.0;

        // Get RSRP and RSRQ
        lteFiltering->get(oppositeRnti, RSRP, &Qrxlevmeas_dBm);
        lteFiltering->get(oppositeRnti, RSRQ, &Qqualmeas_dB);

        Srxlev_dBm = Qrxlevmeas_dBm - (Qrxlevmin_dBm + Qrxlevminoffset_dB)
                   - Pcompensation_dB;
        Squal_dB   = Qqualmeas_dB - (Qqualmin_dB + Qqualminoffset_dB);

#ifdef LTE_LIB_LOG
        candidateString << "candidate=,[" << oppositeRnti.nodeId << " "
                        << oppositeRnti.interfaceIndex << "],"
                        << "RSRP=," << Qrxlevmeas_dBm << ","
                        << "RSRQ=," << Qqualmeas_dB << ",";
#endif

#if PHY_LTE_APPLY_SENSITIVITY_FOR_CELL_SELECTION

        double rxRxPowerRb_dBm = IN_DB(NON_DB(Qrxlevmeas_dBm) /
                PhyLteGetRsResourceElementsInRb(node, phyIndex) *
                PhyLteGetNumSubcarriersPerRb(node, phyIndex));

        if (rxRxPowerRb_dBm < IN_DB(phyLte->rxSensitivity_mW[0]))
        {
#ifdef LTE_LIB_LOG
            std::stringstream ss;
            ss << "Cell selection skipped due to "
               << "the rx sensitivity condition,"
               << "rxRxPowerRb_dBm=," << rxRxPowerRb_dBm << ","
               << "rxSensitivity_dBm[0]=,"
               << IN_DB(phyLte->rxSensitivity_mW[0]) << ",";
            ERROR_ReportWarning(ss.str().c_str());
#endif
            it++;
            continue;
        }

#endif

        // Find cells satisfied conditions.
        // If multiple eNB satisfies cell selection condition
        // maximum RSRP eNB is selected.
        if (((Srxlev_dBm > 0.0) && (Squal_dB > 0.0) &&
                (selectedRsrp_dBm == LTE_INFINITY_POWER_dBm)) ||
            ((Srxlev_dBm > 0.0) && (Squal_dB > 0.0) &&
                (selectedRsrp_dBm != LTE_INFINITY_POWER_dBm) &&
                (Qrxlevmeas_dBm > selectedRsrp_dBm)))
        {
            selectedRntieNB   = it->first;
            selectedRrcConfig = it->second;
            selectedRsrp_dBm  = Qrxlevmeas_dBm;
            selected          = TRUE;
        }
        it++;
    }

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    if (selected == TRUE && lte::LteLog::validationPatternEnabled())
    {
        std::vector < lte::ConnectionInfo > connectionInfos;

        lte::LteLog::getConnectionInfoConnectingWithMe(
                                    node->nodeId, false, connectionInfos);
        ERROR_Assert(connectionInfos.size() == 1,
                "Invalid connection info for validation. "
                "Please check the environment variable of "
                "validation pattern.");
        const lte::ConnectionInfo& ci = connectionInfos[0];

        // If selected eNB is not the intended eNB for validation,
        // let the cell selection be failed.
        if (ci.nodeId != selectedRntieNB.nodeId)
        {
            std::stringstream ss;
            ss << "Cell selection canceled because "
               << "a non-intended eNB is selected. "
               << "UE=" << node->nodeId << ", "
               << "selected eNB=" << selectedRntieNB.nodeId << ", "
               << "intended eNB=" << ci.nodeId;
            ERROR_ReportWarning(ss.str().c_str());

            selected = FALSE;
            selectedRntieNB  = LTE_INVALID_RNTI;
        }
    }
#endif
#endif

    BOOL foundBetterCell = FALSE;
    if (selected == TRUE)
    {
        if ((establishmentData->selectedRntieNB != selectedRntieNB))
        {
            foundBetterCell = TRUE;
        }

        // Store the RRC config of serving eNB.
        establishmentData->selectedRntieNB   = selectedRntieNB;
        establishmentData->selectedRrcConfig = selectedRrcConfig;

        // Set the selected time
        phyLte->phySelectedTime = node->getNodeTime();

#ifdef LTE_LIB_LOG
        lte::LteLog::DebugFormat(
            node, node->phyData[phyIndex]->macInterfaceIndex,
            LTE_STRING_LAYER_TYPE_PHY,
            "%s,%sFound=,%s,selected/serving=,"LTE_STRING_FORMAT_RNTI,
            LTE_STRING_CATEGORY_TYPE_CELL_SELECTION,
            candidateString.str().c_str(),
            foundBetterCell ? "TRUE" : "FALSE",
            establishmentData->selectedRntieNB.nodeId,
            establishmentData->selectedRntieNB.interfaceIndex);
#endif
        if (PhyLteIsInStationaryState(node, phyIndex) == true)
        {
            clocktype elapsedTime
                    = node->getNodeTime() - phyLte->phySelectedTime;
            if (elapsedTime >= phyLte->cellSelectionMinServingDuration
                && foundBetterCell == TRUE)
            {
                // Notify the better cell detection
                RrcLteNotifyDetectingBetterCell(
                    node,
                    node->phyData[phyIndex]->macInterfaceIndex,
                    selectedRntieNB);
            }
        }
        else
        {
            // Change state
            PhyLteChangeState(node,
                              phyIndex,
                              phyLte,
                              PHY_LTE_IDLE_RANDOM_ACCESS);

#ifdef LTE_LIB_LOG
            lte::LteLog::InfoFormat(
                node, node->phyData[phyIndex]->macInterfaceIndex,
                LTE_STRING_LAYER_TYPE_PHY,
                "%s,%s,%s=,"LTE_STRING_FORMAT_RNTI,
                LTE_STRING_CATEGORY_TYPE_CELL_SELECTION,
                LTE_STRING_RESULT_TYPE_SUCCESS,
                LTE_STRING_STATION_TYPE_ENB,
                selectedRntieNB.nodeId,
                selectedRntieNB.interfaceIndex);
#endif

            // Notify the cell selection result
            RrcLteNotifyCellSelectionResults(
                            node,
                            node->phyData[phyIndex]->macInterfaceIndex,
                            TRUE,
                            selectedRntieNB);

            // Set CheckingConnection timer
            PhyLteSetCheckingConnectionTimer(node, phyIndex);
        }

        // Terminate receiving/interference signals if
        // listening channel is changed.
        // Because signal end of these signals will never happen.
        if (phyLte->dlChannelNo !=
            selectedRrcConfig.ltePhyConfig.dlChannelNo)
        {
            PhyLteTerminateCurrentReceive(node,phyIndex);
        }

        // Set the listening channel
        phyLte->dlChannelNo = selectedRrcConfig.ltePhyConfig.dlChannelNo;
        phyLte->ulChannelNo = selectedRrcConfig.ltePhyConfig.ulChannelNo;

        int numChannels = PROP_NumberChannels(node);
        for (int i = 0; i < numChannels; i++)
        {
            if ((phyLte->dlChannelNo != i) &&
                (PHY_IsListeningToChannel(node, phyIndex, i)))
            {
                PHY_StopListeningToChannel(node, phyIndex, i);
            }
        }
        PHY_SetTransmissionChannel(node, phyIndex, phyLte->ulChannelNo);
    }
    else
    {
#ifdef LTE_LIB_LOG
        lte::LteLog::DebugFormat(
            node, node->phyData[phyIndex]->macInterfaceIndex,
            LTE_STRING_LAYER_TYPE_PHY,
            "%s,%sFound=,%s",
            LTE_STRING_CATEGORY_TYPE_CELL_SELECTION,
            candidateString.str().c_str(),
            foundBetterCell ? "TRUE" : "FALSE");
#endif

        if (phyLte->rxState == PHY_LTE_CELL_SELECTION_MONITORING)
        {
            // Change state
            PhyLteChangeState(node,
                              phyIndex,
                              phyLte,
                              PHY_LTE_IDLE_CELL_SELECTION);

            // Notify the cell selection result
            RrcLteNotifyCellSelectionResults(
                            node,
                            node->phyData[phyIndex]->macInterfaceIndex,
                            FALSE,
                            selectedRntieNB);

#ifdef LTE_LIB_LOG
            lte::LteLog::InfoFormat(
                node, node->phyData[phyIndex]->macInterfaceIndex,
                LTE_STRING_LAYER_TYPE_PHY,
                "%s,%s,%s",
                LTE_STRING_CATEGORY_TYPE_CELL_SELECTION,
                LTE_STRING_RESULT_TYPE_FAILURE,
                "Doesn't meat the power requirements.");
#endif
        }
    }

    // Erase the information except one about
    // the selected(or connecting currently) cell
    std::map < LteRnti, LteRrcConfig > ::iterator eraseit;
    eraseit = establishmentData->rxRrcConfigList->begin();

    while (eraseit != establishmentData->rxRrcConfigList->end())
    {
        if (establishmentData->selectedRntieNB != eraseit->first)
        {
#if 0
            std::map < LteRnti, LteRrcConfig > ::iterator tmp = eraseit;
            eraseit++;
            establishmentData->rxRrcConfigList->erase(tmp);
#else
            establishmentData->rxRrcConfigList->erase(eraseit++);
#endif
        }
        else
        {
            eraseit++;
        }
    }
}

// /**
// FUNCTION   :: PhyLteIsInStationaryState
// LAYER      :: PHY
// PURPOSE    :: Get whether the PHY state is a stationary state.
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : int   : Index of the PHY
// RETURN     :: bool  : true  : PHY state is a stationary state
// **/
bool PhyLteIsInStationaryState(
                                Node* node,
                                int phyIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    if (phyLte->txState > PHY_LTE_RA_GRANT_WAITING
     && phyLte->rxState > PHY_LTE_RA_GRANT_WAITING)
    {
        return true;
    }
    ERROR_Assert(phyLte->txState==phyLte->rxState,
        "Tx state and Rx state are different state.");
    return false;
}

// /**
// FUNCTION   :: PhyLteRaGrantTransmissionIndication
// LAYER      :: PHY
// PURPOSE    :: Sending RA Grant
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : int   : Index of the PHY
// + ueRnti                    : Rnti of UE which receives RA Grant
// RETURN     :: void  : NULL
// **/
void PhyLteRaGrantTransmissionIndication(
                                Node* node,
                                int phyIndex,
                                LteRnti ueRnti)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    ERROR_Assert(phyLte->raGrantsToSend->find(ueRnti) ==
                                            phyLte->raGrantsToSend->end(),
            "Multiple RA grant transmission for the same UE is requested");

    phyLte->raGrantsToSend->insert(ueRnti);
}

// /**
// FUNCTION   :: PhyLteRaGrantWaitingTimerTimeoutNotification
// LAYER      :: PHY
// PURPOSE    :: RA grant waiting timer time out notification from MAC Layer
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteRaGrantWaitingTimerTimeoutNotification(
                                Node* node,
                                int phyIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    // Change state
    PhyLteChangeState(node,
                      phyIndex,
                      phyLte,
                      PHY_LTE_IDLE_RANDOM_ACCESS);
}

// /**
// FUNCTION   :: PhyLteGetMacConfigInEstablishment
// LAYER      :: PHY
// PURPOSE    :: Pass the mac config to MAC Layer during establishment period
// PARAMETERS ::
// + node                      : Node* : Pointer to node.
// + phyIndex                  : int   : Index of the PHY
// RETURN     :: LteMacConfig* : Pointer of LteMacConfig
// **/
LteMacConfig* PhyLteGetMacConfigInEstablishment(
                                Node* node,
                                int phyIndex)
{
    LteMacConfig* retMacConfig = NULL;
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;

    if ((phyLte->rxState >= PHY_LTE_IDLE_RANDOM_ACCESS) &&
        (PhyLteIsInStationaryState(node, phyIndex) == false))
    {
        PhyLteEstablishmentData* establishmentData =
            phyLte->establishmentData;
        retMacConfig = &(establishmentData->selectedRrcConfig.lteMacConfig);
    }
    return retMacConfig;
}

// /**
// FUNCTION   :: PhyLteRaPreambleTransmissionIndication
// LAYER      :: PHY
// PURPOSE    :: End of the transmission
// PARAMETERS ::
// + node               : Node*         : Pointer to node.
// + phyIndex           : int           : Index of the PHY
// + useMacLayerSpecifiedDelay : BOOL  : Use MAC layer specified delay or not
// + initDelayUntilAirborne : clocktype : Delay until airborne
// + preambleInfoFromMac: LteRaPreamble*: Random Access Preamble Info
// RETURN     :: void  : NULL
// **/
void PhyLteRaPreambleTransmissionIndication(
                                Node* node,
                                int phyIndex,
                                BOOL useMacLayerSpecifiedDelay,
                                clocktype initDelayUntilAirborne,
                                const LteRaPreamble* preambleInfoFromMac)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    PhyLteEstablishmentData* establishmentData = phyLte->establishmentData;
    int interfaceIndex =
        LteGetMacInterfaceIndexFromPhyIndex(node, phyIndex);

    clocktype delayUntilAirborne = initDelayUntilAirborne;
    if (!useMacLayerSpecifiedDelay)
    {
        delayUntilAirborne = PHY_LTE_RX_TX_TURNAROUND_TIME;
    }

    // PRACH Configuration Index
    int prachConfigIndex = establishmentData->
                           selectedRrcConfig.lteMacConfig.raPrachConfigIndex;

    // PRACH Frequency offset
    int prachFreqOffset = establishmentData->
                         selectedRrcConfig.ltePhyConfig.raPrachFreqOffset;

    // current subframe index
    clocktype subframeDuration  = 1 * MILLI_SECOND;
    clocktype frameDuration     = PHY_LTE_MAX_NUM_AVAILABLE_SUBFRAME
                                  * subframeDuration;
    clocktype currentTime       = node->getNodeTime();
    int currentFrameIndex       = (int)(currentTime / frameDuration);
    int currentSubframeIndex    = (int)((currentTime -
                                  currentFrameIndex * frameDuration)
                                  / subframeDuration);

    // Decide next timing for transmission
    int nextTransmissionFrameIndex      = currentFrameIndex;
    int nextTransmissionSubframeIndex   = currentSubframeIndex;
    clocktype nextTransmissionTiming    = 0;
    
    for (int i = 0; i < NEXT_FRAME; i++)
    {
        if (nextTransmissionTiming >= currentTime)
        {
            break;
        }

        nextTransmissionFrameIndex += i;
        for (int j = 0; j < PHY_LTE_MAX_NUM_AVAILABLE_SUBFRAME; j++)
        {
            nextTransmissionSubframeIndex
                = PHY_LTE_PRACH_SUBFRAME_NUMBER_TABLE[prachConfigIndex][j];

            // -1: not available
            if (nextTransmissionSubframeIndex == -1)
            {
                continue;
            }

            nextTransmissionTiming
                    = nextTransmissionFrameIndex * frameDuration
                    + nextTransmissionSubframeIndex * subframeDuration;

            if (nextTransmissionTiming >= currentTime)
            {
                break;
            }
        }
    }

    clocktype timerDuration
                    = (nextTransmissionFrameIndex * frameDuration
                    + nextTransmissionSubframeIndex * subframeDuration)
                    - currentTime;

    // Calculate tx power
    LteLayer3Filtering* lteFiltering
                            = LteLayer2GetLayer3Filtering(
                              node,
                              node->phyData[phyIndex]->macInterfaceIndex);
    int numOfdmsymbolRsInRb
            = PhyLteGetRsResourceElementsInRb(node, phyIndex);
    UInt8 numScPerRb
            = PhyLteGetNumSubcarriersPerRb(node, phyIndex);
    UInt8 numRb
            = PhyLteGetNumResourceBlocks(node, phyIndex);

    double receivedTargetPower_dBm
                = preambleInfoFromMac->preambleReceivedTargetPower;
    double rsrp_dBm;
    lteFiltering->get(establishmentData->selectedRntieNB,
                        RSRP,
                        &rsrp_dBm);
    double maxTxPower_dBm
        = establishmentData->selectedRrcConfig.ltePhyConfig.maxTxPower_dBm;

    double Pre_rs_mW
            = NON_DB(maxTxPower_dBm) / (numRb * numScPerRb);

    double pathloss_dB
            = -1.0 * (IN_DB(NON_DB(rsrp_dBm) / numOfdmsymbolRsInRb)
              - IN_DB(Pre_rs_mW));

    double txPower_dBm = MIN(phyLte->maxTxPower_dBm,
                             receivedTargetPower_dBm + pathloss_dB);
    double txPower_mW  = NON_DB(txPower_dBm) / PHY_LTE_PRACH_RB_NUM;

    // Change state
    PhyLteChangeState(node,
                      phyIndex,
                      phyLte,
                      PHY_LTE_RA_PREAMBLE_TRANSMISSION_RESERVED);

    // Set a timer
    PhyLteRaPreamble txPreamble;
    txPreamble.raRnti           = preambleInfoFromMac->ueRnti;

    txPreamble.targetEnbRnti        = establishmentData->selectedRntieNB;
    txPreamble.prachResourceStart   = prachFreqOffset;
    txPreamble.preambleIndex        = preambleInfoFromMac->raPreambleIndex;
    txPreamble.txPower_mW           = txPower_mW;
    LteConnectionInfo* handoverInfo =
        Layer3LteGetHandoverInfo(node, interfaceIndex);
    txPreamble.isHandingOverRa = (handoverInfo != NULL);
    PhyLteSetNextTrannsmissionPreambleTimer(node,
                                            phyIndex,
                                            timerDuration,
                                            txPreamble,
                                            delayUntilAirborne);
#ifdef LTE_LIB_LOG
    lte::LteLog::InfoFormat(
        node, node->phyData[phyIndex]->macInterfaceIndex,
        LTE_STRING_LAYER_TYPE_PHY,
        "%s,%s",
        LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS,
        LTE_STRING_PROCESS_TYPE_START);
#endif // LTE_LIB_LOG
}



// /**
// FUNCTION   :: PhyLteIFPHConfigureMeasConfig
// LAYER      :: PHY
// PURPOSE    :: IF for RRC to setup intra-freq meas config
// PARAMETERS ::
// + node                 : Node* : Pointer to node.
// + phyIndex             : int   : Index of the PHY
// + intervalSubframeNum  : int   : interval of measurement
// + offsetSubframe       : int   : offset of measurement subframe
// + filterCoefRSRP       : int   : filter Coefficient RSRP
// + filterCoefRSRQ       : int   : filter Coefficient RSRQ
// + gapInterval          : clocktype : non Serving Cell Measurement Interval
// RETURN     :: void  : NULL
// **/
void PhyLteIFPHConfigureMeasConfig(
                            Node *node,
                            int phyIndex,
                            int intervalSubframeNum,
                            int offsetSubframe,
                            int filterCoefRSRP,
                            int filterCoefRSRQ,
                            clocktype gapInterval)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte *)thisPhy->phyVar;
    PhyLteEstablishmentData* establishmentData = phyLte->establishmentData;

    // set
    establishmentData->intervalSubframeNum_intraFreq = intervalSubframeNum;
    establishmentData->offsetSubframe_intraFreq = offsetSubframe;
    establishmentData->filterCoefRSRP = filterCoefRSRP;
    establishmentData->filterCoefRSRQ = filterCoefRSRQ;

    // gap config
    phyLte->nonServingCellMeasurementInterval = gapInterval;
}
// /**
// FUNCTION   :: PhyLteIFPHStartMeasIntraFreq
// LAYER      :: PHY
// PURPOSE    :: IF for RRC to start intra-freq measurement
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteIFPHStartMeasIntraFreq(
                            Node *node,
                            int phyIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte *)thisPhy->phyVar;
    PhyLteEstablishmentData* establishmentData = phyLte->establishmentData;

    ERROR_Assert(establishmentData->measureIntraFreq == FALSE,
        "intra-freq measurement has already started.");

    // set
    establishmentData->measureIntraFreq = TRUE;
}
// /**
// FUNCTION   :: PhyLteIFPHStopMeasIntraFreq
// LAYER      :: PHY
// PURPOSE    :: IF for RRC to stop intra-freq measurement
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteIFPHStopMeasIntraFreq(
                            Node *node,
                            int phyIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte *)thisPhy->phyVar;
    PhyLteEstablishmentData* establishmentData = phyLte->establishmentData;

    //ERROR_Assert(establishmentData->measureIntraFreq == TRUE,
    //    "intra-freq measurement has already stop.");
    if (establishmentData->measureIntraFreq != TRUE) return;

    //// clear layer3 filtering freq info
    //LteLayer3Filtering* lteFiltering 
    //                        = LteLayer2GetLayer3Filtering(
    //                          node, 
    //                          node->phyData[phyIndex]->macInterfaceIndex);

    //// clear meas result (for H.O.) and freq info (type-freq-rnti dictionary)
    //lteFiltering->clearHOMeasIntraFreq(phyLte->dlChannelNo);

    // set
    establishmentData->measureIntraFreq = FALSE;

}
// /**
// FUNCTION   :: PhyLteIFPHStartMeasInterFreq
// LAYER      :: PHY
// PURPOSE    :: IF for RRC to start inter-freq measurement
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteIFPHStartMeasInterFreq(
                            Node *node,
                            int phyIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte *)thisPhy->phyVar;
    PhyLteEstablishmentData* establishmentData = phyLte->establishmentData;

    ERROR_Assert(establishmentData->measureInterFreq == FALSE,
        "inter-freq measurement has already started.");

    // set non serving cell measurement timer
    PhyLteSetNonServingCellMeasurementIntervalTimer(node, phyIndex);

    // set
    establishmentData->measureInterFreq = TRUE;
}
// /**
// FUNCTION   :: PhyLteIFPHStopMeasInterFreq
// LAYER      :: PHY
// PURPOSE    :: IF for RRC to stop inter-freq measurement
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteIFPHStopMeasInterFreq(
                            Node *node,
                            int phyIndex)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte *)thisPhy->phyVar;
    PhyLteEstablishmentData* establishmentData = phyLte->establishmentData;

    //ERROR_Assert(establishmentData->measureInterFreq == TRUE,
    //    "intra-freq measurement has already stop.");
    if (establishmentData->measureInterFreq != TRUE) return;

    //// clear layer3 filtering freq info
    //LteLayer3Filtering* lteFiltering 
    //                        = LteLayer2GetLayer3Filtering(
    //                          node, 
    //                          node->phyData[phyIndex]->macInterfaceIndex);

    //// clear meas result (for H.O.) and freq info (type-freq-rnti dictionary)
    //lteFiltering->clearHOMeasInterFreq(phyLte->dlChannelNo);

    // set
    establishmentData->measureInterFreq = FALSE;

    // cancel timer
    std::map<int, Message*>::iterator it = 
        phyLte->eventMsgList->find(
        MSG_PHY_LTE_NonServingCellMeasurementInterval);
    if (it != phyLte->eventMsgList->end())
    {
        phyLte->eventMsgList->erase(it);
    }
}

// /**
// FUNCTION   :: PhyLtePrepareForHandoverExecution
// LAYER      :: PHY
// PURPOSE    :: clear information for H.O. execution
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// + selectedRntieNB: const LteRnti&: target eNB's RNTI
// + selectedRrcConfig: const LteRrcConfig&: rrcConfig to connect to target
// RETURN     :: void  : NULL
// **/
void PhyLtePrepareForHandoverExecution(
    Node* node,
    int phyIndex,
    const LteRnti& selectedRntieNB,
    const LteRrcConfig& selectedRrcConfig)
{
    PhyData* thisPhy   = node->phyData[phyIndex];
    PhyDataLte* phyLte = (PhyDataLte*)thisPhy->phyVar;
    PhyLteEstablishmentData* establishmentData = phyLte->establishmentData;

    phyLte->monitoringState = PHY_LTE_NON_SERVING_CELL_MONITORING_IDLE;

    // Store the RRC config of serving eNB.
    establishmentData->selectedRntieNB   = selectedRntieNB;
    establishmentData->selectedRrcConfig = selectedRrcConfig;

    // Set the selected time
    phyLte->phySelectedTime = node->getNodeTime();

    {
        // Set CheckingConnection timer
        PhyLteSetCheckingConnectionTimer(node, phyIndex);
    }

    // Set the listening channel
    phyLte->dlChannelNo = selectedRrcConfig.ltePhyConfig.dlChannelNo;
    phyLte->ulChannelNo = selectedRrcConfig.ltePhyConfig.ulChannelNo;

    // reset channels
    int numChannels = PROP_NumberChannels(node);
    for (int i = 0; i < numChannels; i++)
    {
        if ((phyLte->dlChannelNo != i) &&
            (PHY_IsListeningToChannel(node, phyIndex, i)))
        {
            PHY_StopListeningToChannel(node, phyIndex, i);
        }
        if ((phyLte->dlChannelNo == i) &&
            (!PHY_IsListeningToChannel(node, phyIndex, i)))
        {
            PHY_StartListeningToChannel(node, phyIndex, i);
        }
    }
    PHY_SetTransmissionChannel(node, phyIndex, phyLte->ulChannelNo);

}

