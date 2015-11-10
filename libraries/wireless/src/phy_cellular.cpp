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
#include "antenna_switched.h"
#include "antenna_steerable.h"

#include "cellular.h"
#include "phy_cellular.h"

#ifdef UMTS_LIB
#include "cellular_umts.h"
#include "phy_umts.h"
#endif

#ifdef MUOS_LIB
#include "cellular_muos.h"
#include "phy_muos.h"
#endif

#define DEBUG 0
// /**
// FUNCTION   :: PhyCellularInit
// LAYER      :: PHY
// PURPOSE    :: Initialize the Cellular PHY
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// + nodeInput : const NodeInput* : Pointer to the node input
// RETURN     :: void             : NULL
// **/
void PhyCellularInit(Node *node,
                  const int phyIndex,
                  const NodeInput *nodeInput)
{
    Address interfaceAddress;
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;

    PhyCellularData *phyCellular =
        (PhyCellularData*) MEM_malloc(sizeof(PhyCellularData));

    phyCellular->thisPhy = node->phyData[phyIndex];
    node->phyData[phyIndex]->phyVar = ( void* )phyCellular;

    // generate initial seed
    RANDOM_SetSeed(phyCellular->randSeed,
                   node->globalSeed,
                   node->nodeId,
                   PHY_CELLULAR,
                   phyIndex);

    if (DEBUG)
    {
        printf("node %d: Cellular Phy Init\n", node->nodeId);
    }

    NetworkGetInterfaceInfo(node,
                            phyCellular->thisPhy->macInterfaceIndex,
                            &interfaceAddress);
    IO_ReadString(
        node,
        node->nodeId,
        phyCellular->thisPhy->macInterfaceIndex,
        nodeInput,
        "CELLULAR-PHY-MODEL",
        &retVal,
        buf);
    if (retVal == FALSE)
    {
        ERROR_ReportError("CELLULAR-PHY-MODEL parameter"
            " was not found for node");
    }

    if (strcmp(buf, "PHY-ABSTRACT-CELLULAR") == 0)
    {
        if (DEBUG)
        {
            printf("node %d: Calling Abstract Cellular Phy Init\n",
                node->nodeId);
        }
        phyCellular->cellularPhyProtocolType =
            Cellular_ABSTRACT_Phy;

    }
    else if (strcmp(buf, "PHY-GSM") == 0)
    {
        if (DEBUG)
        {
            printf("node %d: GSM Cellular Phy init\n",node->nodeId);
        }
        phyCellular->cellularPhyProtocolType =
            Cellular_GSM_Phy;
    }
    else if (strcmp(buf, "PHY-GPRS") == 0)
    {
        if (DEBUG)
        {
            printf("node %d: GPRS Cellular Phy init\n",node->nodeId);
        }
        phyCellular->cellularPhyProtocolType =
            Cellular_GPRS_Phy;
    }
    else if (strcmp(buf, "PHY-UMTS") == 0)
    {
#if defined(UMTS_LIB)
        phyCellular->cellularPhyProtocolType = Cellular_UMTS_Phy;
        PhyUmtsInit(node, phyIndex, nodeInput);
#else
        ERROR_ReportMissingLibrary("PHY-UMTS", "UMTS");
#endif
    }
    else if (strcmp(buf, "PHY-MUOS") == 0)
    {
#if defined(MUOS_LIB)
        phyCellular->cellularPhyProtocolType = Cellular_MUOS_Phy;
        PhyMuosInit(node, phyIndex, nodeInput);
#else
        ERROR_ReportMissingLibrary("PHY-MUOS", "MUOS");
#endif
    }
    else if (strcmp(buf, "PHY-CDMA2000") == 0)
    {
        if (DEBUG)
        {
            printf("node %d: CDMA2000 Cellular PHY init\n",node->nodeId);
        }
        phyCellular->cellularPhyProtocolType =
            Cellular_CDMA2000_Phy;
    }
    else if (strcmp(buf, "PHY-WIMAX") == 0)
    {
        if (DEBUG)
        {
            printf("node %d: WIMAX Cellular PHY init\n",node->nodeId);
        }
        phyCellular->cellularPhyProtocolType =
            Cellular_WIMAX_Phy;
    }
    else
    {
        ERROR_ReportError(
            "CELLULAR-PHY-MODEL parameter must be ABSTRACT-CELLULAR-PHY,"
            " PHY-GSM, PHY-GPRS, PHY-UMTS, PHY-CDMA2000"
            " or PHY-WIMAX");
    }

    IO_ReadString(
                node->nodeId,
                ANY_DEST,
                nodeInput,
                "CELLULAR-STATISTICS",
                &retVal,
                buf);

    if ((retVal == FALSE) || (strcmp(buf, "YES") == 0))
    {
        phyCellular->collectStatistics = TRUE;
    }
    else if (retVal && strcmp(buf, "NO") == 0)
    {
        phyCellular->collectStatistics = FALSE;
    }
    else
    {
        ERROR_ReportWarning(
            "Value of CELLULAR-STATISTICS should be YES or NO! "
            "Default value YES is used.");
        phyCellular->collectStatistics = TRUE;
    }
}

// /**
// FUNCTION   :: PhyCellularFinalize
// LAYER      :: PHY
// PURPOSE    :: Finalize the Cellular PHY, print out statistics
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + phyIndex  : const int : Index of the PHY
// RETURN     :: void      : NULL
// **/
void PhyCellularFinalize(Node *node, const int phyIndex)
{
    if (DEBUG)
    {
        printf("node %d: Cellular Phy Finalize\n",node->nodeId);
    }

    PhyCellularData *phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    switch(phyCellular->cellularPhyProtocolType)
    {
        case Cellular_ABSTRACT_Phy:
        {

            break;
        }
        case Cellular_GSM_Phy:
        {
            break;
        }
        case Cellular_GPRS_Phy:
        {
            break;
        }
        case Cellular_UMTS_Phy:
        {
#ifdef UMTS_LIB
            PhyUmtsFinalize(node, phyIndex);
            break;
#else
            ERROR_ReportMissingLibrary("PHY-UMTS", "UMTS");
#endif
        }
       case Cellular_MUOS_Phy:
        {
#ifdef MUOS_LIB
            PhyMuosFinalize(node, phyIndex);
            break;
#else
            ERROR_ReportMissingLibrary("PHY-MUOS", "MUOS");
#endif
        }
        case Cellular_CDMA2000_Phy:
        {
            break;
        }
        case Cellular_WIMAX_Phy:
        {
            break;
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-PHY-PROTOCOL parameter must be"
                " ABSTRACT-CELLULAR-PHY,"
                " GSM-PHY, GPRS-PHY, UMTS-PHY, CDMA2000-PHY"
                " or WIMAX-PHY");
        }
    }

    MEM_free(phyCellular);
    node->phyData[phyIndex]->phyVar = NULL;
}

// /**
// FUNCTION   :: PhyCellularStartTransmittingSignal
// LAYER      :: PHY
// PURPOSE    :: Start transmitting a frame
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : int   : Index of the PHY
// + packet : Message* : Frame to be transmitted
// + clocktype : duration : Duration of the transmission
// + useMacLayerSpecifiedDelay : Use MAC specified delay or calculate it
// + initDelayUntilAirborne    : clocktype : The MAC specified delay
// RETURN     :: void : NULL
// **/
void PhyCellularStartTransmittingSignal(
         Node* node,
         int phyIndex,
         Message* packet,
         clocktype duration,
         BOOL useMacLayerSpecifiedDelay,
         clocktype initDelayUntilAirborne)
{
    if (DEBUG)
    {
        printf("node %d: Cellular Phy start TXing\n",node->nodeId);
    }

    PhyCellularData *phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    switch(phyCellular->cellularPhyProtocolType)
    {
        case Cellular_ABSTRACT_Phy:
        {

            break;
        }
        case Cellular_GSM_Phy:
        {
            break;
        }
        case Cellular_GPRS_Phy:
        {
            break;
        }
        case Cellular_UMTS_Phy:
        {
#ifdef UMTS_LIB
            PhyUmtsStartTransmittingSignal(
                node,
                phyIndex,
                packet,
                duration,
                useMacLayerSpecifiedDelay,
                initDelayUntilAirborne);
            break;
#else
            ERROR_ReportMissingLibrary("PHY-UMTS", "UMTS");
#endif
        }
        case Cellular_MUOS_Phy:
        {
#ifdef MUOS_LIB
            PhyMuosStartTransmittingSignal(
                node,
                phyIndex,
                packet,
                duration,
                useMacLayerSpecifiedDelay,
                initDelayUntilAirborne);

            break;
#else
            ERROR_ReportMissingLibrary("PHY-MUOS", "MUOS");
#endif
        }
        case Cellular_CDMA2000_Phy:
        {
            break;
        }
        case Cellular_WIMAX_Phy:
        {
            break;
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-PHY-PROTOCOL parameter must be"
                " ABSTRACT-CELLULAR-PHY,"
                " GSM-PHY, GPRS-PHY, UMTS-PHY, CDMA2000-PHY"
                " or WIMAX-PHY");
        }
    }
}

// /**
// FUNCTION   :: PhyCellularTransmissionEnd
// LAYER      :: PHY
// PURPOSE    :: End of the transmission
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyCellularTransmissionEnd(
         Node* node,
         int phyIndex)
{
    if (DEBUG)
    {
        printf("node %d: Cellular Phy Transmission End\n",node->nodeId);
    }

    PhyCellularData *phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    switch(phyCellular->cellularPhyProtocolType)
    {
        case Cellular_ABSTRACT_Phy:
        {

            break;
        }
        case Cellular_GSM_Phy:
        {
            break;
        }
        case Cellular_GPRS_Phy:
        {
            break;
        }
        case Cellular_UMTS_Phy:
        {
#ifdef UMTS_LIB
            PhyUmtsTransmissionEnd(node, phyIndex);
            break;
#else
            ERROR_ReportMissingLibrary("PHY-UMTS", "UMTS");
#endif
        }
        case Cellular_MUOS_Phy:
        {
#ifdef MUOS_LIB
            PhyMuosTransmissionEnd(node, phyIndex);
            break;
#else
            ERROR_ReportMissingLibrary("PHY-MUOS", "MUOS");
#endif
        }
        case Cellular_CDMA2000_Phy:
        {
            break;
        }
        case Cellular_WIMAX_Phy:
        {
            break;
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-PHY-PROTOCOL parameter must be"
                " ABSTRACT-CELLULAR-PHY,"
                " GSM-PHY, GPRS-PHY, UMTS-PHY, CDMA2000-PHY"
                " or WIMAX-PHY");
        }
    }
}

// /**
// FUNCTION   :: PhyCellularSignalArrivalFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal arrival from the chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyCellularSignalArrivalFromChannel(
         Node* node,
         int phyIndex,
         int channelIndex,
         PropRxInfo* propRxInfo)
{
    if (DEBUG)
    {
        printf("node %d: Cellular Phy Signal Arrival From Channel\n",
                node->nodeId);
    }

    PhyCellularData *phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    switch(phyCellular->cellularPhyProtocolType)
    {
        case Cellular_ABSTRACT_Phy:
        {

            break;
        }
        case Cellular_GSM_Phy:
        {
            break;
        }
        case Cellular_GPRS_Phy:
        {
            break;
        }
        case Cellular_UMTS_Phy:
        {
#ifdef UMTS_LIB
            PhyUmtsSignalArrivalFromChannel(
                node,
                phyIndex,
                channelIndex,
                propRxInfo);
            break;
#else
            ERROR_ReportMissingLibrary("PHY-UMTS", "UMTS");
#endif
        }
       case Cellular_MUOS_Phy:
        {
#ifdef MUOS_LIB
            PhyMuosSignalArrivalFromChannel(
                node,
                phyIndex,
                channelIndex,
                propRxInfo);
            break;
#else
            ERROR_ReportMissingLibrary("PHY-MUOS", "MUOS");
#endif
        }
        case Cellular_CDMA2000_Phy:
        {
            break;
        }
        case Cellular_WIMAX_Phy:
        {
            break;
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-PHY-PROTOCOL parameter must be"
                " ABSTRACT-CELLULAR-PHY,"
                " GSM-PHY, GPRS-PHY, UMTS-PHY, CDMA2000-PHY"
                " or WIMAX-PHY");
        }
    }
}

// /**
// FUNCTION   :: PhyCellularSignalEndFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal end from a chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyCellularSignalEndFromChannel(
         Node* node,
         int phyIndex,
         int channelIndex,
         PropRxInfo* propRxInfo)
{
    if (DEBUG)
    {
        printf("node %d: Cellular Phy Signal End From Channel\n",
                node->nodeId);
    }

    PhyCellularData *phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    switch(phyCellular->cellularPhyProtocolType)
    {
        case Cellular_ABSTRACT_Phy:
        {
            break;
        }
        case Cellular_GSM_Phy:
        {
            break;
        }
        case Cellular_GPRS_Phy:
        {
            break;
        }
        case Cellular_UMTS_Phy:
        {
#ifdef UMTS_LIB
            PhyUmtsSignalEndFromChannel(
                node,
                phyIndex,
                channelIndex,
                propRxInfo);
            break;
#else
            ERROR_ReportMissingLibrary("PHY-UMTS", "UMTS");
#endif
        }
        case Cellular_MUOS_Phy:
        {
#ifdef MUOS_LIB
            PhyMuosSignalEndFromChannel(
                node,
                phyIndex,
                channelIndex,
                propRxInfo);
            break;
#else
            ERROR_ReportMissingLibrary("PHY-MUOS", "MUOS");
#endif
        }
        case Cellular_CDMA2000_Phy:
        {
            break;
        }
        case Cellular_WIMAX_Phy:
        {
            break;
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-PHY-PROTOCOL parameter must be"
                " ABSTRACT-CELLULAR-PHY,"
                " GSM-PHY, GPRS-PHY, UMTS-PHY, CDMA2000-PHY"
                " or WIMAX-PHY");
        }
    }
}


PhyStatusType
PhyCellularGetStatus(
    Node* node,
    int phyNum)
{
    return (PhyStatusType)1;
}


void PhyCellularSetTransmitPower(
         Node* node,
         PhyData* thisPhy,
         double newTxPower_mW)
{
}


void PhyCellularGetTransmitPower(
         Node* node,
         PhyData* thisPhy,
         double* txPower_mW)
{
}


clocktype PhyCellularGetTransmissionDuration(
              Node* node,
              int size,
              int dataRate)
{
    return (clocktype)1;
}

int PhyCellularGetDataRate(
        Node* node,
        PhyData* thisPhy)
{
    return 1;
}

float PhyCellularGetRxLevel(
          Node* node,
          int phyIndex)
{
    return 1.0;
}

double PhyCellularGetRxQuality(
           Node* node,
           int phyIndex)
{
    return 1.0;
}

// /**
// FUNCTION   :: PhyCellularInterferenceArrivalFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal arrival from the chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// + sigPower_mW  : double      : The inband interference signal power
// RETURN     :: void : NULL
// **/
void PhyCellularInterferenceArrivalFromChannel(
         Node* node,
         int phyIndex,
         int channelIndex,
         PropRxInfo* propRxInfo,
         double sigPower_mW)
{

    PhyCellularData *phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;
    switch(phyCellular->cellularPhyProtocolType)
    {
        case Cellular_ABSTRACT_Phy:
        {

            break;
        }
        case Cellular_GSM_Phy:
        {
            break;
        }
        case Cellular_GPRS_Phy:
        {
            break;
        }
        case Cellular_UMTS_Phy:
        {
#ifdef UMTS_LIB
            PhyUmtsInterferenceArrivalFromChannel(
                node,
                phyIndex,
                channelIndex,
                propRxInfo,
                sigPower_mW);

            break;
#else
            ERROR_ReportMissingLibrary("PHY-UMTS", "UMTS");
#endif
        }
        case Cellular_CDMA2000_Phy:
        {
            break;
        }
        case Cellular_WIMAX_Phy:
        {
            break;
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-PHY-PROTOCOL parameter must be"
                " ABSTRACT-CELLULAR-PHY,"
                " GSM-PHY, GPRS-PHY, UMTS-PHY, CDMA2000-PHY"
                " or WIMAX-PHY");
        }
    }
}

// /**
// FUNCTION   :: PhyCellularInterferenceEndFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal end from a chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyCellularInterferenceEndFromChannel(
         Node* node,
         int phyIndex,
         int channelIndex,
         PropRxInfo* propRxInfo)
{

    PhyCellularData *phyCellular =
        (PhyCellularData*)node->phyData[phyIndex]->phyVar;

    switch(phyCellular->cellularPhyProtocolType)
    {
        case Cellular_ABSTRACT_Phy:
        {
            break;
        }
        case Cellular_GSM_Phy:
        {
            break;
        }
        case Cellular_GPRS_Phy:
        {
            break;
        }
        case Cellular_UMTS_Phy:
        {
#ifdef UMTS_LIB
            PhyUmtsInterferenceEndFromChannel(
                node,
                phyIndex,
                channelIndex,
                propRxInfo);

            break;
#else
            ERROR_ReportMissingLibrary("PHY-UMTS", "UMTS");
#endif
        }
        case Cellular_CDMA2000_Phy:
        {
            break;
        }
        case Cellular_WIMAX_Phy:
        {
            break;
        }
        default:
        {
            ERROR_ReportError(
                "CELLULAR-PHY-PROTOCOL parameter must be"
                " ABSTRACT-CELLULAR-PHY,"
                " GSM-PHY, GPRS-PHY, UMTS-PHY, CDMA2000-PHY"
                " or WIMAX-PHY");
        }
    }
}
