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

#include "mac_dot16.h"
#include "mac_dot16_bs.h"
#include "mac_dot16_ss.h"
#include "mac_dot16_phy.h"
#include "mac_dot16_sch.h"
#include "phy_dot16.h"

#ifdef PARALLEL //Parallel
#include "parallel.h"
#endif //endParallel

#define DEBUG              0
#define DEBUG_PACKET       0
#define DEBUG_CDMA_RANGING 0

// /**
// FUNCTION   :: MacDot16PhyPrintStats
// LAYER      :: MAC
// PURPOSE    :: Print out statistics
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to 802.16 data structure
// + phyStats : MacDot16PhyStats* : Pointer to stats structure
// RETURN     :: void : NULL
// **/
static
void MacDot16PhyPrintStats(Node* node,
                           MacDataDot16* dot16,
                           MacDot16PhyStats* phyStats)
{

}

//--------------------------------------------------------------------------
// OFDMA PHY related functions
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// Generial API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: MacDot16PhyAddPhySyncField
// LAYER      :: MAC
// PURPOSE    :: Add the PHY synchronization field
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to 802.16 data structure
// + macFrame  : unsigned char*: Pointer to the MAC frame
// + startIndex: int           : starting position in MAC frame
// + frameLengh: int           : total length of the DL part of the frame
// RETURN     :: int : Number of bytes added
// **/
int MacDot16PhyAddPhySyncField(Node* node,
                               MacDataDot16* dot16,
                               unsigned char* macFrame,
                               int startIndex,
                               int frameLength)
{
    int index;

    index = startIndex;

    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        MacDot16PhyOfdma* dot16Ofdma = (MacDot16PhyOfdma*) dot16->phyData;
        MacDot16PhyOfdmaSyncField syncField;

        syncField.durationCode = dot16Ofdma->frameDurationCode;
        syncField.frameNumber = dot16Ofdma->frameNumber;
        memcpy((unsigned char*) &(macFrame[index]),
               (unsigned char*) &syncField,
               sizeof(MacDot16PhyOfdmaSyncField));

        dot16Ofdma->frameNumber ++;
        dot16Ofdma->frameNumber &= 0x00FFFFFF; // modulo 2^24

        index += sizeof(syncField);
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16PhyGetFrameNumber
// LAYER      :: MAC
// PURPOSE    :: Get frame number from PHY synchronization field
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to 802.16 data structure
// + phySyncField  : unsigned char*: Pointer to PHY sync field
// RETURN     :: int : return frame number in PHY sync field
// **/
int MacDot16PhyGetFrameNumber(Node* node,
                                      MacDataDot16* dot16,
                                      unsigned char* macFrame)
{
    int frameNumber = 0;

    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        MacDot16PhyOfdmaSyncField syncField;

        memcpy((unsigned char*) &syncField,
               (unsigned char*) macFrame,
               sizeof(MacDot16PhyOfdmaSyncField));
        frameNumber = syncField.frameNumber;
    }
    else
    {
        // PHY model not supported
        ERROR_ReportError("MAC 802.16: Wrong PHY model, only OFDMA "
                          "supported!");
    }

    return frameNumber;
}

// /**
// FUNCTION   :: MacDot16PhyGetFrameDuration
// LAYER      :: MAC
// PURPOSE    :: Get frame duration from PHY synchronization field
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to 802.16 data structure
// + phySyncField  : unsigned char*: Pointer to PHY sync field
// RETURN     :: clocktype : return frame duration in PHY sync field
// **/
clocktype MacDot16PhyGetFrameDuration(Node* node,
                                      MacDataDot16* dot16,
                                      unsigned char* macFrame)
{
    clocktype frameDuration = 0;
    int durationCode;

    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        MacDot16PhyOfdma* dot16Ofdma = (MacDot16PhyOfdma*) dot16->phyData;
        MacDot16PhyOfdmaSyncField syncField;

        memcpy((unsigned char*) &syncField,
               (unsigned char*) macFrame,
               sizeof(MacDot16PhyOfdmaSyncField));
        durationCode = syncField.durationCode;
        frameDuration = dot16Ofdma->frameDurationList[durationCode];
    }
    else
    {
        // PHY model not supported
        ERROR_ReportError("MAC 802.16: Wrong PHY model, only OFDMA "
                          "supported!");
    }

    return frameDuration;
}

// /**
// FUNCTION   :: MacDot16PhySetFrameDuration
// LAYER      :: MAC
// PURPOSE    :: Set and validate the frame duration according to PHY type
// PARAMETERS ::
// + node          : Node*         : Pointer to node
// + dot16         : MacDataDot16* : Pointer to 802.16 data structure
// + frameDuration : clocktype     : Duration of the frame
// RETURN     :: void : NULL
// **/
void MacDot16PhySetFrameDuration(Node* node,
                                 MacDataDot16* dot16,
                                 clocktype frameDuration)
{
    int i;

    // get frame duration code according to configured frame duration
    if (dot16->stationType == DOT16_BS)
    {
        if (dot16->phyType == DOT16_PHY_OFDMA)
        {
            MacDot16PhyOfdma* dot16Ofdma = (MacDot16PhyOfdma*)
                                           dot16->phyData;
            MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
            ERROR_Assert(dot16Bs != NULL,
                         "MAC 802.16: BS data struct needs to be inited "
                         "before initiating PHY sublayer!");

            for (i = 0; i < DOT16_OFDMA_NUM_FRAME_DURATION; i ++)
            {
                if (dot16Ofdma->frameDurationList[i] ==
                    dot16Bs->para.frameDuration)
                {
                    dot16Ofdma->frameDurationCode = (UInt8) i;
                    break;
                }
            }

            if (i == DOT16_OFDMA_NUM_FRAME_DURATION)
            {
                ERROR_ReportError("MAC 802.16: Wrong frame duration! For "
                                  "OFDMA PHY, valid frame durations are: "
                                  "2MS, 2500US, 4MS, 5MS, 8MS, 10MS, "
                                  "12500US, and 20MS");
            }
        }
        else
        {
            ERROR_ReportError("MAC 802.16: Can only set frame duration "
                              "for BS!\n");
        }
    }
    else
    {
        // PHY model not supported
        ERROR_ReportError("MAC 802.16: Wrong PHY model, only OFDMA "
                          "supported!");
    }
}

// /**
// FUNCTION   :: MacDot16PhyAddDlMapIE
// LAYER      :: MAC
// PURPOSE    :: Add DL MAP IE for one  burst
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + dot16     : MacDataDot16*  : Pointer to DOT16 data struct
// + macFrame  : unsigned char* : Pointer to the MAC frame
// + startIndex : int           : starting position in MAC frame
// + cid       :                : CID for this  burst
// + diuc      :                : diuc used in the busrt
// + burstInfo : Dot16BurstInfo : busrt info
// RETURN     :: int : Number of bytes added
// **/
int MacDot16PhyAddDlMapIE(Node* node,
                          MacDataDot16* dot16,
                          unsigned char* macFrame,
                          int startIndex,
                          Dot16CIDType cid,
                          unsigned char diuc,
                          Dot16BurstInfo burstInfo)
{
    int index;

    index = startIndex;
    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        MacDot16PhyOfdmaDlMapIE dlMapIE;

        // only the basic IE is supported right now
        dlMapIE.diuc = diuc;
        dlMapIE.numOfdmaSymbols = burstInfo.numSymbols;
        dlMapIE.numSubchannels = burstInfo.numSubchannels;
        dlMapIE.ofdmaSymbolOffset = burstInfo.symbolOffset;
        dlMapIE.subchannelOffset = burstInfo.subchannelOffset;
        dlMapIE.repCodingIndication = 0; // Give the right vlaue
        dlMapIE.boosting = 0; // give the right vlaue

        //copy the contents to macFrame
        memcpy((unsigned char*) &(macFrame[index]),
                 (unsigned char*) &dlMapIE,
                  sizeof(MacDot16PhyOfdmaDlMapIE));

        index += sizeof(MacDot16PhyOfdmaDlMapIE);
    }
    else
    {
        ERROR_ReportError("MAC 802.16: Only OFDMA PHY is supported!");
    }

    return index - startIndex;

}

// /**
// FUNCTION   :: MacDot16PhyGetDlMapIE
// LAYER      :: MAC
// PURPOSE    :: Get one DL-MAP_IE
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16*       : Pointer to 802.16 data structure
// + macFrame  : unsigned char*  : Pointer to the MAC frame
// + startIndex: int             : starting position in MAC frame
// + cid       : Dot16CIDType*   : CID that the IE assigned to
// + diuc      : unsigned char*  : DIUC value
// + burstInfo : Dot16BurstInfo* : Pointer to the burst Info
// RETURN     :: int : Number of bytes processed
// **/
int MacDot16PhyGetDlMapIE(Node* node,
                          MacDataDot16* dot16,
                          unsigned char* macFrame,
                          int startIndex,
                          Dot16CIDType* cid,
                          unsigned char* diuc,
                          Dot16BurstInfo* burstInfo)
{
    int index;

    index = startIndex;

    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        MacDot16PhyOfdmaDlMapIE dlMapIE;

        // only the basic IE is supported right now
        memcpy((unsigned char*) &dlMapIE,
               (unsigned char*) &(macFrame[index]),
               sizeof(MacDot16PhyOfdmaDlMapIE));

        *diuc = dlMapIE.diuc;
        burstInfo->numSymbols = dlMapIE.numOfdmaSymbols;
        burstInfo->numSubchannels = dlMapIE.numSubchannels;
        burstInfo->symbolOffset = dlMapIE.ofdmaSymbolOffset;
        burstInfo->subchannelOffset = dlMapIE.subchannelOffset;

        // more ....

        index += sizeof(MacDot16PhyOfdmaDlMapIE);
    }
    else
    {
        ERROR_ReportError("MAC 802.16: Only OFDMA PHY is supported!");
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16PhyAddUlMapIE
// LAYER      :: MAC
// PURPOSE    :: Add one UL-MAP_IE
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to 802.16 data structure
// + macFrame  : unsigned char*: Pointer to the MAC frame
// + startIndex: int           : starting position in MAC frame
// + frameLengh: int           : total length of the DL part of the frame
// + cid       : Dot16CIDType  : CID that the IE assigned to
// + uiuc      : unsigned char : UIUC value
// + duration  : int           : Duration of the burst
// RETURN     :: int : Number of bytes added
// **/
int MacDot16PhyAddUlMapIE(Node* node,
                          MacDataDot16* dot16,
                          unsigned char* macFrame,
                          int startIndex,
                          int frameLength,
                          Dot16CIDType cid,
                          unsigned char uiuc,
                          int duration,
                          unsigned char flag)
{
    int index;
    //MacDot16GenericPhyOfdmaUlMapIE genricULMapIE;
    MacDot16Bs* dot16Bs = (MacDot16Bs*) dot16->bsData;
    index = startIndex;

    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
// Note: based on the UIUC value perform the action
// Refer page no. 532-533
        if(uiuc == DOT16_UIUC_CDMA_RANGE)
        {
            MacDot16PhyOfdmaUlMapIEuiuc12 ulMapIE;
            ulMapIE.cid = cid;
            ulMapIE.uiuc = uiuc;
            clocktype symbolDuration;
            symbolDuration = (clocktype) PhyDot16GetOfdmSymbolDuration(
                                             node,
                                             dot16->myMacData->phyNumber);
            ulMapIE.paddingNibble = 0;
            ulMapIE.noOfOfdmaSymbols = (int)((dot16Bs->para.frameDuration 
                                             - dot16Bs->para.rtg -
                                            dot16Bs->para.tddDlDuration) / 
                                            symbolDuration);
            ulMapIE.noOfSubchannels = DOT16_CDMA_ALLOCATE_RANGEING_CHANNEL;
            ulMapIE.ofmaSynbolOffset = 0;
            ulMapIE.rangingIndicator = 0;
            ulMapIE.rangingMethod = flag;

            ulMapIE.subchannelOffset =
                MacDot16PhyGetUplinkNumSubchannels(node, dot16);

            memcpy((unsigned char*) &(macFrame[index]),
                   (unsigned char*) &ulMapIE,
                    sizeof(MacDot16PhyOfdmaUlMapIEuiuc12));
            if(DEBUG_CDMA_RANGING)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("Send CDMA UL map pdu\n");
            }

            index += sizeof(MacDot16PhyOfdmaUlMapIEuiuc12);
        }

        else if(uiuc == DOT16_UIUC_CDMA_ALLOCATION_IE)
        {
            MacDot16PhyOfdmaUlMapIEuiuc14 ulMapIE;
            MacDot16RngCdmaInfoList* cdmaInfo = dot16Bs->recCDMARngList;
            MacDot16RngCdmaInfoList* tempCdmaInfo = cdmaInfo;
            ulMapIE.cid = cid;
            ulMapIE.uiuc = uiuc;

            if(cdmaInfo == NULL)
            {
                return index - startIndex;
            }
            while(tempCdmaInfo != NULL &&
                (tempCdmaInfo->rngRspSentFlag == TRUE
                || tempCdmaInfo->codetype != DOT16_CDMA_INITIAL_RANGING_CODE)
                && (duration > 0))
            {
                ulMapIE.cid = cid;
                ulMapIE.uiuc_Transmission = uiuc;
                // 0 for initial ranging and 1 for Bw req
                ulMapIE.bwRequest = 0;

                //allocate smaller duration for bandwidth request
                if(cdmaInfo->codetype == DOT16_CDMA_BWREQ_CODE)
                {
                    ulMapIE.duration = dot16Bs->para.requestOppSize
                        + dot16Bs->para.sstgInPs;
                }
                else
                {
                ulMapIE.duration = dot16Bs->para.rangeOppSize
                    + dot16Bs->para.sstgInPs;
                }

                ulMapIE.repCodingIndication = 0;
                ulMapIE.frameNumber = tempCdmaInfo->cdmaInfo.ofdmaFrame;
                ulMapIE.rangingCode = tempCdmaInfo->cdmaInfo.rangingCode;
                ulMapIE.rangingSubchannel =
                    tempCdmaInfo->cdmaInfo.ofdmaSubchannel;
                ulMapIE.rangingSymbol = tempCdmaInfo->cdmaInfo.ofdmaSymbol;
                ulMapIE.paddingNibble = 0;
                memcpy((unsigned char*) &(macFrame[index]),
                       (unsigned char*) &ulMapIE,
                        sizeof(MacDot16PhyOfdmaUlMapIEuiuc14));
                index += sizeof(MacDot16PhyOfdmaUlMapIEuiuc14);

                cdmaInfo = cdmaInfo->next;
                MEM_free(tempCdmaInfo);
                tempCdmaInfo = cdmaInfo;
                duration -= ulMapIE.duration;
            }
            dot16Bs->recCDMARngList = tempCdmaInfo;
            if(DEBUG_CDMA_RANGING)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("Send CDMA Allocation IE map pdu\n");
            }
        }
        else
        {
            MacDot16PhyOfdmaUlMapIE ulMapIE;

            ulMapIE.cid = cid;
            ulMapIE.uiuc = uiuc;
            ulMapIE.duration = duration;
            ulMapIE.repCodingIndication = 0;

            memcpy((unsigned char*) &(macFrame[index]),
                   (unsigned char*) &ulMapIE,
                    sizeof(MacDot16PhyOfdmaUlMapIE));

            index += sizeof(MacDot16PhyOfdmaUlMapIE);
        }
    }
    else
    {
        ERROR_ReportError("MAC 802.16: Only OFDMA PHY is supported!");
    }

    return index - startIndex;
}

// /**
// FUNCTION   :: MacDot16PhyGetUlMapIE
// LAYER      :: MAC
// PURPOSE    :: Get one UL-MAP_IE
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to 802.16 data structure
// + macFrame  : unsigned char*: Pointer to the MAC frame
// + startIndex: int           : starting position in MAC frame
// + cid       : Dot16CIDType* : CID that the IE assigned to
// + uiuc      : unsigned char*: UIUC value
// + duration  : clocktype*    : Duration of the burst
// RETURN     :: int : Number of bytes processed
// **/
int MacDot16PhyGetUlMapIE(Node* node,
                          MacDataDot16* dot16,
                          unsigned char* macFrame,
                          int startIndex,
                          Dot16CIDType* cid,
                          unsigned char* uiuc,
                          clocktype* duration,
                          MacDot16GenericPhyOfdmaUlMapIE* cdmaInfo)
{
    int index;

    index = startIndex;
// Note: based on the UIUC value perform the action
// Refer page no. 532-533

    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        MacDot16PhyOfdmaUlMapIE ulMapIE;

        memcpy((unsigned char*) &ulMapIE,
               (unsigned char*) &(macFrame[index]),
               sizeof(MacDot16PhyOfdmaUlMapIE));

        *cid = ulMapIE.cid;
        *uiuc = ulMapIE.uiuc;

        if(*uiuc == DOT16_UIUC_CDMA_RANGE)
        {
            //MacDot16PhyOfdmaUlMapIEuiuc12 ulMapIE;
            memcpy((unsigned char*) &cdmaInfo->ulMapIE12,
                   (unsigned char*) &(macFrame[index]),
                   sizeof(MacDot16PhyOfdmaUlMapIEuiuc12));

            *duration = 0;
            index += sizeof(MacDot16PhyOfdmaUlMapIEuiuc12);
            if(DEBUG_CDMA_RANGING)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("Received CDMA UL map pdu\n");
            }
        }
        else if(*uiuc == DOT16_UIUC_CDMA_ALLOCATION_IE)
        {
            //MacDot16PhyOfdmaUlMapIEuiuc12 ulMapIE;
            memcpy((unsigned char*) &cdmaInfo->ulMapIE14,
                   (unsigned char*) &(macFrame[index]),
                   sizeof(MacDot16PhyOfdmaUlMapIEuiuc14));

            *duration = cdmaInfo->ulMapIE14.duration;
            index += sizeof(MacDot16PhyOfdmaUlMapIEuiuc14);
            if(DEBUG_CDMA_RANGING)
            {
                MacDot16PrintRunTimeInfo(node, dot16);
                printf("Received CDMA allocation UL map pdu\n");
            }
        }
        else
        {
            *duration = (clocktype) ulMapIE.duration;
            index += sizeof(MacDot16PhyOfdmaUlMapIE);
        }
    }
    else
    {
        ERROR_ReportError("MAC 802.16: Only OFDMA PHY is supported!");
    }

    return index - startIndex;
}


// /**
// FUNCTION   :: MacDot16PhyInit
// LAYER      :: MAC
// PURPOSE    :: Initialize DOT16 PHY sublayer at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacDot16PhyInit(Node* node,
                     int interfaceIndex,
                     const NodeInput* nodeInput)
{
    MacDataDot16* dot16 = (MacDataDot16*)
                          node->macData[interfaceIndex]->macVar;

    ERROR_Assert(dot16 != NULL, "MAC 802.16: PHY sublayer should be inited"
                 "after other MAC components");

    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        MacDot16PhyOfdma* dot16Ofdma;

        dot16Ofdma = (MacDot16PhyOfdma*)
                     MEM_malloc(sizeof(MacDot16PhyOfdma));
        ERROR_Assert(dot16Ofdma != NULL, "MAC 802.16: Out of memory!");
        memset((char*)dot16Ofdma, 0, sizeof(MacDot16PhyOfdma));

        dot16->phyData = (void*) dot16Ofdma;

        dot16Ofdma->frameNumber = 0;
        dot16Ofdma->numSubchannels = 1;

        dot16Ofdma->frameDurationList[0] = 0; // N/A
        dot16Ofdma->frameDurationList[1] = 2 * MILLI_SECOND;
        dot16Ofdma->frameDurationList[2] = 2500 * MICRO_SECOND;
        dot16Ofdma->frameDurationList[3] = 4 * MILLI_SECOND;
        dot16Ofdma->frameDurationList[4] = 5 * MILLI_SECOND;
        dot16Ofdma->frameDurationList[5] = 8 * MILLI_SECOND;
        dot16Ofdma->frameDurationList[6] = 10 * MILLI_SECOND;
        dot16Ofdma->frameDurationList[7] = 12500 * MICRO_SECOND;
        dot16Ofdma->frameDurationList[8] = 20 * MILLI_SECOND;
    }
    else
    {
        ERROR_ReportError("MAC 802.16: Only OFDMA is supported!");
    }

    int phyIndex = dot16->myMacData->phyNumber;
    if (node->phyData[phyIndex]->phyModel == PHY802_16)
    {
        PhyDot16SetStationType(node, phyIndex, (char) dot16->stationType);
    }
    else
    {
        // MAC 802.16 requires PHY802.16
        ERROR_ReportError("MAC802.16 requires PHY802.16 PHY model!\n");
    }

    #ifdef PARALLEL //Parallel
        PARALLEL_SetProtocolIsNotEOTCapable(node);
        PARALLEL_SetMinimumLookaheadForInterface(
            node,
            DOT16_PHY_RADIO_TURNAROUND_TIME);
    #endif //endParallel
}

// /**
// FUNCTION   :: MacDot16PhyFinalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacDot16PhyFinalize(Node *node, int interfaceIndex)
{
    MacDataDot16* dot16 = (MacDataDot16 *)
                          node->macData[interfaceIndex]->macVar;

    MacDot16PhyStats* phyStats = NULL;

    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        MacDot16PhyOfdma* dot16Ofdma;

        dot16Ofdma = (MacDot16PhyOfdma*) dot16->phyData;
        phyStats = &(dot16Ofdma->stats);
    }

    if (phyStats != NULL)
    {
        MacDot16PhyPrintStats(node, dot16, phyStats);
    }
}

// /**
// FUNCTION   :: MacDot16PhyReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer.
//               The PHY sublayer will first handle it
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// + msg              : Message*     : Packet received from PHY
// RETURN     :: void : NULL
// **/
void MacDot16PhyReceivePacketFromPhy(Node* node,
                                     MacDataDot16* dot16,
                                     Message* msg)
{
    if (dot16->stationType == DOT16_SS)
    {
        MacDot16SsReceivePacketFromPhy(node, dot16, msg);
    }
    else
    {
        // PDU from SS
        MacDot16BsReceivePacketFromPhy(node, dot16, msg);
    }
}

// /**
// FUNCTION   :: MacDot16PhyTransmitMacFrame
// LAYER      :: MAC
// PURPOSE    :: Transmit a MAC frame
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// + msg              : Message*     : Packet received from PHY
// + dlMapLength      : unsigned char: Length of the DL-MAP
// RETURN     :: void : NULL
// **/
void MacDot16PhyTransmitMacFrame(Node* node,
                                 MacDataDot16* dot16,
                                 Message* msg,
                                 unsigned char dlMapLength,
                                 clocktype duration)
{
    Message* newMsg;
    PhyDot16TxInfo* txInfo;

    if (DEBUG_PACKET)
    {
        printf("Node%d transmit packet on DL channel, packet size= %d\n",
               node->nodeId,
               MESSAGE_ReturnPacketSize(msg));
    }

    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        if (dot16->stationType == DOT16_BS)
        {
            // add FCH to the DL mac frame
            MacDot16PhyOfdmaFch* fch;

            MESSAGE_AddHeader(node,
                              msg,
                              sizeof(MacDot16PhyOfdmaFch),
                              TRACE_DOT16);
            fch = (MacDot16PhyOfdmaFch*) MESSAGE_ReturnPacket(msg);

            fch->preamble = 0xFF;
            fch->usedSubchannelMap = 0x3F; // all subchannels are used
            fch->rangeChangeIndication = 0; // no change
            fch->repCodingIndication = 0;
            fch->codingIndication = 0;
            fch->dlMapLength = dlMapLength;
        }

        // start transmitting the frame on the PHY
        newMsg = MESSAGE_PackMessage(node, msg, TRACE_DOT16, NULL);
        MESSAGE_InfoAlloc(node, newMsg, sizeof(PhyDot16TxInfo));

        txInfo = (PhyDot16TxInfo*) MESSAGE_ReturnInfo(newMsg);
        txInfo->duration = duration;

        PHY_StartTransmittingSignal(
            node,
            dot16->myMacData->phyNumber,
            newMsg,
            duration,
            TRUE,
            DOT16_PHY_RADIO_TURNAROUND_TIME);
    }
    else
    {
        // other PHYs are not supported yet
        MESSAGE_FreeList(node, msg);
    }
}

// /**
// FUNCTION   :: MacDot16PhyTransmitUlBurst
// LAYER      :: MAC
// PURPOSE    :: Transmit a Uplink burst
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// + msg              : Message*     : Packet received from PHY
// + channelIndex     : int          : Channel for transmission
// + delayInPs        : int          : Delay in PS for transmitting the burst
// RETURN     :: void : NULL
// **/
void MacDot16PhyTransmitUlBurst(Node* node,
                                MacDataDot16* dot16,
                                Message* msg,
                                int channelIndex,
                                int delayInPs)
{
    clocktype delay;
    Message* newMsg;
    PhyDot16TxInfo* txInfo;

    if (DEBUG_PACKET)
    {
        printf("Node%d transmit packet on UL channel %d, packet size= %d,"
               "duration in PS = %d\n",
               node->nodeId, channelIndex,
               MESSAGE_ReturnPacketSize(msg), delayInPs);
    }

    PHY_SetTransmissionChannel(
        node,
        dot16->myMacData->phyNumber,
        channelIndex);

    // calculate the actual transmission delay
    delay = delayInPs * dot16->ulPsDuration;

    // start transmitting the frame on the PHY
    newMsg = MESSAGE_PackMessage(node, msg, TRACE_DOT16, NULL);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(PhyDot16TxInfo));
    txInfo = (PhyDot16TxInfo*) MESSAGE_ReturnInfo(newMsg);
    txInfo->duration = delay;

    PHY_StartTransmittingSignal(
        node,
        dot16->myMacData->phyNumber,
        newMsg,
        delay,
        TRUE,
        DOT16_PHY_RADIO_TURNAROUND_TIME);
}

// /**
// FUNCTION   :: MacDot16PhyGetFrameOverhead
// LAYER      :: MAC
// PURPOSE    :: Get the additional overhead that PHY adds to the beginning
//               of each MAC frame in terms of number of bytes
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// RETURN     :: int : PHY overhead in terms of number of bytes
// **/
int MacDot16PhyGetFrameOverhead(Node* node,
                                MacDataDot16* dot16)
{
    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        return sizeof(MacDot16PhyOfdmaFch);
    }
    else
    {
        return 0;
    }
}

// /**
// FUNCTION   :: MacDot16PhyGetPsDuration
// LAYER      :: MAC
// PURPOSE    :: Get the duration of a physical slot.
//               The physical slot duration is different for
//               different 802.16 PHY models. It may be different for
//               DL and UL.
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + dot16        : MacDataDot16*: Pointer to DOT16 structure
// + subframeType : MacDot16SubframeType : Indicate DL or UL
// RETURN     :: clocktype : Duration of a physical slot.
// **/
clocktype MacDot16PhyGetPsDuration(Node* node,
                                   MacDataDot16* dot16,
                                   MacDot16SubframeType subframeType)
{
    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        clocktype symbolDuration;

        symbolDuration = (clocktype) PhyDot16GetOfdmSymbolDuration(
                                         node,
                                         dot16->myMacData->phyNumber);

        // check this is downlink (BS) or uplink (SS)
        if (subframeType == DOT16_DL)
        {
            return symbolDuration * DOT16_OFDMA_NUM_SYMBOLS_PER_DL_PS;
        }
        else
        {
            return symbolDuration * DOT16_OFDMA_NUM_SYMBOLS_PER_UL_PS;
        }
    }
    else
    {
        return 0;
    }
}

// /**
// FUNCTION   :: MacDot16PhyBitsPerPs
// LAYER      :: MAC
// PURPOSE    :: Get the number of Bits per PS with given burst profile
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + dot16        : MacDataDot16*: Pointer to DOT16 structure
// + burstProfile : void*        : Point to burst profile
// + subframeType : MacDot16SubframeType : Indicate DL or UL
// RETURN     :: int : number of bits per PS
// **/
int MacDot16PhyBitsPerPs(Node* node,
                         MacDataDot16* dot16,
                         void* burstProfile,
                         MacDot16SubframeType subframeType)
{
    int bitsPerPs;
    int codeModuType;

    if (subframeType == DOT16_DL)
    {
        codeModuType =
            ((MacDot16DlBurstProfile*)burstProfile)->ofdma.fecCodeModuType;
    }
    else
    {
        codeModuType =
            ((MacDot16UlBurstProfile*)burstProfile)->ofdma.fecCodeModuType;
    }

    bitsPerPs = PhyDot16GetDataBitsPayloadPerSubchannel(
                     node,
                     dot16->myMacData->phyNumber,
                     codeModuType,
                     subframeType);

    if (subframeType == DOT16_DL)
    {
        bitsPerPs *= DOT16_OFDMA_NUM_SYMBOLS_PER_DL_PS;
    }
    else
    {
        bitsPerPs *= DOT16_OFDMA_NUM_SYMBOLS_PER_UL_PS;
    }

    return bitsPerPs;
}

// /**
// FUNCTION   :: MacDot16PhyBytesToPs
// LAYER      :: MAC
// PURPOSE    :: Get the number of physical slots needed to transmit
//               the given number of bytes
// PARAMETERS ::
// + node         : Node*        : Pointer to node.
// + dot16        : MacDataDot16*: Pointer to DOT16 structure
// + size         : int          : Size in bytes
// + burstProfile : void*        : Point to burst profile
// + subframeType : MacDot16SubframeType : Indicate DL or UL
// RETURN     :: int : # of physical slots needed
// **/
int MacDot16PhyBytesToPs(Node* node,
                         MacDataDot16* dot16,
                         int size,
                         void* burstProfile,
                         MacDot16SubframeType subframeType)
{
    int numPs;
    int sizeInBits;
    int bitsPerPs;
    int codeModuType;

    if (subframeType == DOT16_DL)
    {
        codeModuType =
            ((MacDot16DlBurstProfile*)burstProfile)->ofdma.fecCodeModuType;
    }
    else
    {
        codeModuType =
            ((MacDot16UlBurstProfile*)burstProfile)->ofdma.fecCodeModuType;
    }

    bitsPerPs = PhyDot16GetDataBitsPayloadPerSubchannel(
                    node,
                    dot16->myMacData->phyNumber,
                    codeModuType,
                    subframeType);

    if (subframeType == DOT16_DL)
    {
        bitsPerPs *= DOT16_OFDMA_NUM_SYMBOLS_PER_DL_PS;
    }
    else
    {
        bitsPerPs *= DOT16_OFDMA_NUM_SYMBOLS_PER_UL_PS;
    }

    sizeInBits = size * 8;
    numPs = sizeInBits / bitsPerPs + ((sizeInBits % bitsPerPs) != 0);

    return numPs;
}

// /**
// FUNCTION   :: MacDot16PhyGetNumSubchannels
// LAYER      :: MAC
// PURPOSE    :: Get number of subchannels supported by PHY layer
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// RETURN     :: int  : Number of subcahnnels supported
// **/
int MacDot16PhyGetNumSubchannels(Node* node,
                                 MacDataDot16* dot16,
                                 MacDot16SubframeType subChannelType)
{
    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        int noOfChannelAllocateForRanging = 0;
        BOOL isCdmaRangingEnabled = FALSE;
        MacDot16Bs* dot16Bs;
        MacDot16Ss* dot16Ss;
        if(dot16->stationType == DOT16_BS)
        {
            dot16Bs = (MacDot16Bs*) dot16->bsData;
            if(dot16Bs->rngType == DOT16_CDMA ||
               dot16Bs->bwReqType == DOT16_BWReq_CDMA)
            {
                isCdmaRangingEnabled = TRUE;
            }
        }
        else
        {
            dot16Ss = (MacDot16Ss*) dot16->ssData;
            if(dot16Ss->rngType == DOT16_CDMA ||
               dot16Ss->bwReqType == DOT16_BWReq_CDMA)
            {
                isCdmaRangingEnabled = TRUE;
            }
        }

        if(isCdmaRangingEnabled && (subChannelType == DOT16_UL))
        {
            int temp;
            noOfChannelAllocateForRanging =
                DOT16_CDMA_ALLOCATE_RANGEING_CHANNEL;
            temp = PhyDot16GetNumberSubchannels(
                    node,
                    dot16->myMacData->phyNumber)
                    - noOfChannelAllocateForRanging;
            if(temp <= 0)
            {
                ERROR_ReportError(
                    "Total No of subchannel is less than"
                    " DOT16_CDMA_ALLOCATE_RANGEING_CHANNEL\n");
            }
        }
        return (PhyDot16GetNumberSubchannels(
                    node,
                    dot16->myMacData->phyNumber)
                    - noOfChannelAllocateForRanging);
    }
    else
    {
        return 1;
    }
}
// /**
// FUNCTION   :: MacDot16PhyGetUplinkNumSubchannels
// LAYER      :: MAC
// PURPOSE    :: Get uplink number of subchannels supported by PHY layer
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + dot16            : MacDataDot16*: Pointer to DOT16 structure
// RETURN     :: int  : Number of subcahnnels supported
// **/
int MacDot16PhyGetUplinkNumSubchannels(Node* node, MacDataDot16* dot16)
{
    if (dot16->phyType == DOT16_PHY_OFDMA)
    {
        //do not count ranging channel
        int noOfChannelAllocateForRanging = 0;
        BOOL isCdmaRangingEnabled = FALSE;
        MacDot16Bs* dot16Bs;
        MacDot16Ss* dot16Ss;
        if(dot16->stationType == DOT16_BS)
        {
            dot16Bs = (MacDot16Bs*) dot16->bsData;
            if(dot16Bs->rngType == DOT16_CDMA ||
               dot16Bs->bwReqType == DOT16_BWReq_CDMA)
            {
                isCdmaRangingEnabled = TRUE;
            }
        }
        else
        {
            dot16Ss = (MacDot16Ss*) dot16->ssData;
            if(dot16Ss->rngType == DOT16_CDMA ||
               dot16Ss->bwReqType == DOT16_BWReq_CDMA)
            {
                isCdmaRangingEnabled = TRUE;
            }
        }

        if(isCdmaRangingEnabled)
        {
            int temp;
            noOfChannelAllocateForRanging =
                DOT16_CDMA_ALLOCATE_RANGEING_CHANNEL;
            temp = PhyDot16GetUplinkNumberSubchannels(
                    node,
                    dot16->myMacData->phyNumber)
                    - noOfChannelAllocateForRanging;
            if(temp <= 0)
            {
                ERROR_ReportError(
                    "Total No of subchannel is less than"
                    " DOT16_CDMA_ALLOCATE_RANGEING_CHANNEL\n");
            }
        }

        return (PhyDot16GetUplinkNumberSubchannels(
                    node,
                    dot16->myMacData->phyNumber)
                    - noOfChannelAllocateForRanging);
    }
    else
    {
        return 1;
    }
}

