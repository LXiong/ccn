// for dependency
#include "api.h"
#include "phy_lte.h"
#include "phy_lte_establishment.h"
#include "layer3_lte_filtering.h"
#include "layer3_lte.h"

#include "layer3_lte_measurement.h"

// /**
// FUNCTION   :: Layer3LteMeasProcessEvent
// LAYER      :: RRC
// PURPOSE    :: Process Event.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message           : Event message
// RETURN     :: void : NULL
// **/
void Layer3LteMeasProcessEvent(
    Node* node, UInt32 interfaceIndex, Message* msg)
{
    switch (msg->eventType)
    {
        // periodical report timer expired
        case MSG_RRC_LTE_PeriodicalReportTimerExpired:
        {
            // corresponding measId
            MeasId* measId = (MeasId*)MESSAGE_ReturnInfo(msg);
            ERROR_Assert(measId, "unexpected error.");
            // resend report
            Layer3LteMeasSendMeasReport(node, interfaceIndex, *measId);
            break;
        }
        default:
        {
            char errBuf[MAX_STRING_LENGTH] = {0};
            sprintf(errBuf,
                "Event Type(%d) is not supported "
                "by LTE RRC.", msg->eventType);
            ERROR_ReportError(errBuf);
            break;
        }
    }
    MESSAGE_Free(node, msg);
}

// /**
// FUNCTION   :: Layer3LteMeasCancelAllTimer
// LAYER      :: RRC
// PURPOSE    :: cancel all the measurement timers
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message           : Event message
// RETURN     :: void : NULL
// **/
void Layer3LteMeasCancelAllTimer(
    Node* node, UInt32 interfaceIndex)
{
    Layer3DataLte* layer3Data = LteLayer2GetLayer3DataLte(
        node, interfaceIndex);
    
    for (MapMeasPeriodicalTimer::iterator it = 
        layer3Data->mapMeasPeriodicalTimer.begin();
        it != layer3Data->mapMeasPeriodicalTimer.end(); ++it)
    {
        MESSAGE_CancelSelfMsg(node, it->second);
        it->second = NULL;
    }
    layer3Data->mapMeasPeriodicalTimer.clear();
}

// /**
// FUNCTION   :: Layer3LteGetMeasEventVar
// LAYER      :: Layer3
// PURPOSE    :: get measurement event variable
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// + nodeInput      : const NodeInput*  : Pointer to node input.
// + measEventType  : MeasEventType     : Measurement Event Type
// + measType       : LteMeasurementType: Measurement Type
// + eventVar       : MeasEventVar*     : Pointer to return value
// + hysteresis     : MeasHysteresis*   : Pointer to return value
// RETURN     :: void : NULL
// **/
void Layer3LteMeasGetMeasEventVar(
    Node* node, int interfaceIndex, const NodeInput* nodeInput,
    MeasEventType measEventType, LteMeasurementType measType,
    MeasEventVar* eventVar, MeasHysteresis* hysteresis)
{
    BOOL wasFound = false;

    NodeAddress subnetAddress;
    subnetAddress = MAPPING_GetSubnetAddressForInterface(
                                node,
                                node->nodeId,
                                interfaceIndex);

    switch (measEventType)
    { 
    case MEAS_EVENT_A1:
        if (measType == RSRP_FOR_HO)
        {
            eventVar->eventA1.threshold = 
                LAYER3_LTE_MEAS_DEFAULT_EVENT_A1_RSRP_TH;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A1_RSRP_TH,
                &wasFound,
                &eventVar->eventA1.threshold);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A1_RSRP_TH,
                eventVar->eventA1.threshold);

            *hysteresis = LAYER3_LTE_MEAS_DEFAULT_EVENT_A1_RSRP_HYS;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A1_RSRP_HYS,
                &wasFound,
                hysteresis);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A1_RSRP_HYS,
                *hysteresis);
        }
        else
        {
            eventVar->eventA1.threshold = 
                LAYER3_LTE_MEAS_DEFAULT_EVENT_A1_RSRQ_TH;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A1_RSRQ_TH,
                &wasFound,
                &eventVar->eventA1.threshold);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A1_RSRQ_TH,
                eventVar->eventA1.threshold);

            *hysteresis = LAYER3_LTE_MEAS_DEFAULT_EVENT_A1_RSRQ_HYS;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A1_RSRQ_HYS,
                &wasFound,
                hysteresis);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A1_RSRQ_HYS,
                *hysteresis);
        }
        break;
    case MEAS_EVENT_A2:
        if (measType == RSRP_FOR_HO)
        {
            eventVar->eventA2.threshold = 
                LAYER3_LTE_MEAS_DEFAULT_EVENT_A2_RSRP_TH;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A2_RSRP_TH,
                &wasFound,
                &eventVar->eventA2.threshold);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A2_RSRP_TH,
                eventVar->eventA2.threshold);

            *hysteresis = LAYER3_LTE_MEAS_DEFAULT_EVENT_A2_RSRP_HYS;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A2_RSRP_HYS,
                &wasFound,
                hysteresis);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A2_RSRP_HYS,
                *hysteresis);
        }
        else
        {
            eventVar->eventA2.threshold = 
                LAYER3_LTE_MEAS_DEFAULT_EVENT_A2_RSRQ_TH;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A2_RSRQ_TH,
                &wasFound,
                &eventVar->eventA2.threshold);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A2_RSRQ_TH,
                eventVar->eventA2.threshold);

            *hysteresis = LAYER3_LTE_MEAS_DEFAULT_EVENT_A2_RSRQ_HYS;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A2_RSRQ_HYS,
                &wasFound,
                hysteresis);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A2_RSRQ_HYS,
                *hysteresis);
        }
        break;
    case MEAS_EVENT_A3:
        if (measType == RSRP_FOR_HO)
        {
            eventVar->eventA3.offset = 
                LAYER3_LTE_MEAS_DEFAULT_EVENT_A3_RSRP_OFF;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A3_RSRP_OFF,
                &wasFound,
                &eventVar->eventA3.offset);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A3_RSRP_OFF,
                eventVar->eventA3.offset);

            *hysteresis = LAYER3_LTE_MEAS_DEFAULT_EVENT_A3_RSRP_HYS;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A3_RSRP_HYS,
                &wasFound,
                hysteresis);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A3_RSRP_HYS,
                *hysteresis);
        }
        else
        {
            eventVar->eventA3.offset = 
                LAYER3_LTE_MEAS_DEFAULT_EVENT_A3_RSRQ_OFF;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A3_RSRQ_OFF,
                &wasFound,
                &eventVar->eventA3.offset);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A3_RSRQ_OFF,
                eventVar->eventA3.offset);

            *hysteresis = LAYER3_LTE_MEAS_DEFAULT_EVENT_A3_RSRQ_HYS;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A3_RSRQ_HYS,
                &wasFound,
                hysteresis);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A3_RSRQ_HYS,
                *hysteresis);
        }
        break;
    case MEAS_EVENT_A4:
        if (measType == RSRP_FOR_HO)
        {
            eventVar->eventA4.threshold = 
                LAYER3_LTE_MEAS_DEFAULT_EVENT_A4_RSRP_TH;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A4_RSRP_TH,
                &wasFound,
                &eventVar->eventA4.threshold);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A4_RSRP_TH,
                eventVar->eventA4.threshold);

            *hysteresis = LAYER3_LTE_MEAS_DEFAULT_EVENT_A4_RSRP_HYS;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A4_RSRP_HYS,
                &wasFound,
                hysteresis);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A4_RSRP_HYS,
                *hysteresis);
        }
        else
        {
            eventVar->eventA4.threshold = 
                LAYER3_LTE_MEAS_DEFAULT_EVENT_A4_RSRQ_TH;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A4_RSRQ_TH,
                &wasFound,
                &eventVar->eventA4.threshold);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A4_RSRQ_TH,
                eventVar->eventA4.threshold);

            *hysteresis = LAYER3_LTE_MEAS_DEFAULT_EVENT_A4_RSRQ_HYS;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A4_RSRQ_HYS,
                &wasFound,
                hysteresis);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A4_RSRQ_HYS,
                *hysteresis);
        }
        break;
    case MEAS_EVENT_A5:
        if (measType == RSRP_FOR_HO)
        {
            eventVar->eventA5.threshold1 = 
                LAYER3_LTE_MEAS_DEFAULT_EVENT_A5_RSRP_TH1;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A5_RSRP_TH1,
                &wasFound,
                &eventVar->eventA5.threshold1);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A5_RSRP_TH1,
                eventVar->eventA5.threshold1);

            eventVar->eventA5.threshold2 = 
                LAYER3_LTE_MEAS_DEFAULT_EVENT_A5_RSRP_TH2;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A5_RSRP_TH2,
                &wasFound,
                &eventVar->eventA5.threshold2);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A5_RSRP_TH2,
                eventVar->eventA5.threshold2);

            *hysteresis = LAYER3_LTE_MEAS_DEFAULT_EVENT_A5_RSRP_HYS;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A5_RSRP_HYS,
                &wasFound,
                hysteresis);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A5_RSRP_HYS,
                *hysteresis);
        }
        else
        {
            eventVar->eventA5.threshold1 = 
                LAYER3_LTE_MEAS_DEFAULT_EVENT_A5_RSRQ_TH1;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A5_RSRQ_TH1,
                &wasFound,
                &eventVar->eventA5.threshold1);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A5_RSRQ_TH1,
                eventVar->eventA5.threshold1);

            eventVar->eventA5.threshold2 = 
                LAYER3_LTE_MEAS_DEFAULT_EVENT_A5_RSRQ_TH2;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A5_RSRQ_TH2,
                &wasFound,
                &eventVar->eventA5.threshold2);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A5_RSRQ_TH2,
                eventVar->eventA5.threshold2);

            *hysteresis = LAYER3_LTE_MEAS_DEFAULT_EVENT_A5_RSRQ_HYS;
            IO_ReadDouble(
                node->nodeId,
                subnetAddress,
                nodeInput,
                RRC_LTE_STRING_MEAS_EVENT_A5_RSRQ_HYS,
                &wasFound,
                hysteresis);
            LAYER3_LTE_MEAS_CHECK_EVENT_DB_PARAMETER(
                RRC_LTE_STRING_MEAS_EVENT_A5_RSRQ_HYS,
                *hysteresis);
        }
        break;
    default:
        ERROR_Assert(false, "unexpected error");
        break;
    }
}

// /**
// FUNCTION   :: Layer3LteMeasCheckEventToReport
// LAYER      :: Layer3
// PURPOSE    :: Check Event To Report
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// + measInfo       : MeasInfo*         : Measurement information
// RETURN     :: void : NULL
// **/
void Layer3LteMeasCheckEventToReport(Node* node, 
                                     int interfaceIndex, 
                                     MeasInfo* measInfo)
{
    // RRC instatnce
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    // MeasEventConditionTable
    MeasEventConditionTable* eventCondTable = 
        &layer3Data->measEventConditionTable;
    // serving RNTI
    const LteRnti& servingRnti =
        Layer3LteGetConnectedEnb(node, interfaceIndex);

    // measurement event
    MeasEvent* measEvent = &(measInfo->reportConfig->measEvent);
    // event type
    MeasEventType eventType = measEvent->eventType;
    // MapRntiMeasEventConditionInfo
    MapRntiMeasEventConditionInfo* eventCondInfo = 
        &(eventCondTable->find(measInfo->id)->second);

    switch (eventType)
    {
    case MEAS_EVENT_A1:
    case MEAS_EVENT_A2:
    {
        // corresponding condition entry
        MapRntiMeasEventConditionInfo::iterator it = 
            eventCondInfo->find(servingRnti);
        if (it == eventCondInfo->end())
        {
            break;
        }
        MeasEventConditionInfo* condInfo = &(it->second);

        // collect RNTIs which meet the condition
        // (list contains only serving cell or is empty)
        CellsTriggeredList relatedCells;
        BOOL enterCondition =
            (condInfo->condition == MEAS_EVENT_CONDITION_ENTER
            && condInfo->changedTimeStamp > 0
            && node->getNodeTime() >= condInfo->changedTimeStamp
                + measInfo->reportConfig->measEvent.timeToTrigger);
        if (enterCondition) condInfo->reportingPeriod = TRUE;

        BOOL notYetLeaveCondition =
            (condInfo->condition == MEAS_EVENT_CONDITION_LEAVE
            && condInfo->changedTimeStamp > 0
            && node->getNodeTime() < condInfo->changedTimeStamp
                + measInfo->reportConfig->measEvent.timeToTrigger);

        BOOL alreadyLeftCondition =
            (condInfo->condition == MEAS_EVENT_CONDITION_LEAVE
            && condInfo->changedTimeStamp > 0
            && node->getNodeTime() >= condInfo->changedTimeStamp
                + measInfo->reportConfig->measEvent.timeToTrigger);
        if (alreadyLeftCondition) condInfo->reportingPeriod = FALSE;

        if (enterCondition || (condInfo->reportingPeriod && notYetLeaveCondition))
        {
            relatedCells.insert(it->first);
    
#ifdef LTE_LIB_LOG
            LteMeasurementType measType = measEvent->quantityType;

            lte::LteLog::InfoFormat(node, interfaceIndex,
                LTE_STRING_LAYER_TYPE_RRC,
                LAYER3_LTE_MEAS_CAT_MEASUREMENT","
                LTE_STRING_FORMAT_RNTI","
                "[enter event],"
                "measType,%d,eventType,%d",
                servingRnti.nodeId, servingRnti.interfaceIndex,
                measType, eventType);
#endif
        }

        // report process
        Layer3LteMeasProcessReport(node, interfaceIndex,
            measInfo, relatedCells);

        break;
    }
    case MEAS_EVENT_A3:
    case MEAS_EVENT_A4:
    case MEAS_EVENT_A5:
    {
        MapRntiMeasEventConditionInfo enteringNeighborList;
        // collect RNTIs which meet the condition
        CellsTriggeredList relatedCells;
        // iterate by neighbor cells
        for (MapRntiMeasEventConditionInfo::iterator it = 
            eventCondInfo->begin();
            it != eventCondInfo->end(); it++)
        {
            // skip serving cell
            if (it->first == servingRnti)
            {
                continue;
            }

            BOOL enterCondition =
                (it->second.condition == MEAS_EVENT_CONDITION_ENTER
                && it->second.changedTimeStamp > 0
                && node->getNodeTime() >= it->second.changedTimeStamp
                    + measInfo->reportConfig->measEvent.timeToTrigger);
            if (enterCondition) it->second.reportingPeriod = TRUE;

            BOOL notYetLeaveCondition =
                (it->second.condition == MEAS_EVENT_CONDITION_LEAVE
                && it->second.changedTimeStamp > 0
                && node->getNodeTime() < it->second.changedTimeStamp
                    + measInfo->reportConfig->measEvent.timeToTrigger);

            BOOL alreadyLeftCondition =
                (it->second.condition == MEAS_EVENT_CONDITION_LEAVE
                && it->second.changedTimeStamp > 0
                && node->getNodeTime() >= it->second.changedTimeStamp
                    + measInfo->reportConfig->measEvent.timeToTrigger);
            if (alreadyLeftCondition) it->second.reportingPeriod = FALSE;

            if (enterCondition || (it->second.reportingPeriod && notYetLeaveCondition))
            {
                // add
                relatedCells.insert(it->first);
    
#ifdef LTE_LIB_LOG
                LteMeasurementType measType = measEvent->quantityType;

                lte::LteLog::InfoFormat(node, interfaceIndex,
                    LTE_STRING_LAYER_TYPE_RRC,
                    LAYER3_LTE_MEAS_CAT_MEASUREMENT","
                    LTE_STRING_FORMAT_RNTI","
                    "[enter event],"
                    "measType,%d,eventType,%d",
                    it->first.nodeId, it->first.interfaceIndex,
                    measType, eventType);
#endif
            }
        }

        // report process
        Layer3LteMeasProcessReport(node, interfaceIndex,
            measInfo, relatedCells);

        break;
    }
    default:
        ERROR_Assert(FALSE, "unexpected event type");
    }

}

// /**
// FUNCTION   :: Layer3LteMeasSendMeasReport
// LAYER      :: Layer3
// PURPOSE    :: Send Measurement Report
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// + measId         : MeasId            : Measurement Id
// RETURN     :: void : NULL
// **/
void Layer3LteMeasSendMeasReport(Node* node,
                                 int interfaceIndex,
                                 MeasId measId)
{
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);
    // phy
    PhyDataLte* phyLte = (PhyDataLte*)node->phyData[phyIndex]->phyVar;

    // RRC instatnce
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    // VarMeasConfig
    VarMeasConfig* varMeasConfig = &layer3Data->varMeasConfig;
    // MeasInfo
    MeasInfo* measInfo = NULL;
    ListMeasInfo::iterator it =
        Layer3LteMeasGetMeasInfo(varMeasConfig->measInfoList, measId);
    if (it != varMeasConfig->measInfoList.end())
    {
        measInfo = &(*it);
    }
    ERROR_Assert(measInfo != NULL, "unexpected error");
    // VarMeasReportList
    VarMeasReportList* varMeasReportList = &layer3Data->varMeasReportList;
    // reportInfo
    VarMeasReport* reportInfo = NULL;
    VarMeasReportList::iterator it_reportInfo =
        Layer3LteMeasGetReportInfo(*varMeasReportList, measId);
    if (it_reportInfo != varMeasReportList->end())
    {
        reportInfo = &(*it_reportInfo);
    }
    ERROR_Assert(reportInfo != NULL, "unexpected error");

    // generate new measurement report
    MeasurementReport newReport;
    Layer3LteMeasGenMeasurementReport(node, interfaceIndex, measInfo,
        &reportInfo->cellsTriggeredList, &newReport);
    phyLte->rrcMeasurementReportList->push_back(newReport);
#ifdef LTE_LIB_LOG
    Layer2DataLte* lteLayer2
        = LteLayer2GetLayer2DataLte(
                node, interfaceIndex);
    LteRnti rnti = lteLayer2->myRnti;
    lte::LteLog::InfoFormat(node, interfaceIndex,
        LTE_STRING_LAYER_TYPE_RRC,
        LAYER3_LTE_MEAS_CAT_MEASUREMENT","
        LTE_STRING_FORMAT_RNTI","
        "[send report],"
        "%s,",
        rnti.nodeId,
        rnti.interfaceIndex,
        newReport.ToString().c_str()
        );
#endif

    // increment number of reports sent
    reportInfo->numberOfReportsSent++;

    // specified amount of reports haven't sent yet.
    if (reportInfo->numberOfReportsSent
            < measInfo->reportConfig->reportAmount)
    {
        // set timer for the next periodical report
        Layer3LteMeasSetPeriodicalReportTimer(
            node, interfaceIndex,
            measInfo->reportConfig->reportInterval,
            measId);
    }
}

// /**
// FUNCTION   :: Layer3LteMeasStopPeriodicalTimer
// LAYER      :: Layer3
// PURPOSE    :: Stop Periodical Timer if neccesary
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// + measId         : MeasId            : Measurement Id
// RETURN     :: void : NULL
// **/
void Layer3LteMeasStopPeriodicalTimer(
    Node* node, int interfaceIndex, MeasId measId)
{
    // RRC instatnce
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);

    MapMeasPeriodicalTimer::iterator it_timer =
        layer3Data->mapMeasPeriodicalTimer.find(measId);
    if (it_timer != layer3Data->mapMeasPeriodicalTimer.end())
    {
        // cancel the event
        MESSAGE_CancelSelfMsg(node, it_timer->second);
        // erase from map
        layer3Data->mapMeasPeriodicalTimer.erase(it_timer);
    }
}

// /**
// FUNCTION   :: Layer3LteMeasSetPeriodicalReportTimer
// LAYER      :: RRC
// PURPOSE    :: Set periodical report timer message
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + delay           : clocktype : delay before timer expired
// + measId          : MeasId    : measurement id
// RETURN     :: void : NULL
// **/
void Layer3LteMeasSetPeriodicalReportTimer(
    Node* node, int interfaceIndex, clocktype delay, MeasId measId)
{
    // RRC instatnce
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    
    // if the timer with the same MeasId is already working, stop it.
    Layer3LteMeasStopPeriodicalTimer(node, interfaceIndex, measId);

    Message* timerMsg = 
        MESSAGE_Alloc(
            node, MAC_LAYER, MAC_PROTOCOL_LTE,
            MSG_RRC_LTE_PeriodicalReportTimerExpired);
    MESSAGE_SetInstanceId(timerMsg, interfaceIndex);
    MESSAGE_Send(node, timerMsg, delay);

    *((MeasId*)MESSAGE_InfoAlloc(node, timerMsg, sizeof(MeasId)))
        = measId;

    // register to map
    layer3Data->mapMeasPeriodicalTimer[measId] = timerMsg;
}

// /**
// FUNCTION   :: Layer3LteMeasGenMeasurementReport
// LAYER      :: Layer3
// PURPOSE    :: generate measurement report
// PARAMETERS ::
// + node               : Node*               : Pointer to node.
// + interfaceIndex     : int                 : Interface Index
// + measId             : MeasId              : Measurement Id
// + cellsTriggeredList : CellsTriggeredList* : cells triggered list
// + newReport          : MeasurementReport   : Generated report
// RETURN     :: void : NULL
// **/
void Layer3LteMeasGenMeasurementReport(Node* node,
                                   int interfaceIndex,
                                   MeasInfo* measInfo,
                                   CellsTriggeredList* cellsTriggeredList,
                                   MeasurementReport* newReport)
{
    // Layer3Filtering
    LteLayer3Filtering* layer3Filtering =
        LteLayer2GetLayer3Filtering(node, interfaceIndex);
    // MeasEventType
    MeasEventType eventType = measInfo->reportConfig->measEvent.eventType;
    // serving RNTI
    const LteRnti& servingRnti =
        Layer3LteGetConnectedEnb(node, interfaceIndex);

    BOOL l3fGetResult = TRUE;
    MeasResults* store = &newReport->measResults;

    // common fields
    store->measId = measInfo->id;
    store->measResultServCell.rnti = servingRnti;
    l3fGetResult = layer3Filtering->get(servingRnti, RSRP_FOR_HO,
        &store->measResultServCell.rsrpResult);
    if (!l3fGetResult)
        store->measResultServCell.rsrpResult = LTE_INVALID_RSRP_dBm;
    l3fGetResult = layer3Filtering->get(servingRnti, RSRQ_FOR_HO,
        &store->measResultServCell.rsrqResult);
    if (!l3fGetResult)
        store->measResultServCell.rsrqResult = LTE_INVALID_RSRQ_dB;

    // set neighbor information for A3~A5
    if (eventType == MEAS_EVENT_A3 || eventType == MEAS_EVENT_A4
        || eventType == MEAS_EVENT_A5)
    {
        for (CellsTriggeredList::iterator it = cellsTriggeredList->begin();
            it != cellsTriggeredList->end(); it++)
        {
            MeasResultInfo newInfo;
            newInfo.rnti = *it;
            l3fGetResult = layer3Filtering->get(*it,
                RSRP_FOR_HO, &newInfo.rsrpResult);
            if (!l3fGetResult) newInfo.rsrpResult =LTE_INVALID_RSRP_dBm;
            l3fGetResult = layer3Filtering->get(*it,
                RSRQ_FOR_HO, &newInfo.rsrqResult);
            if (!l3fGetResult) newInfo.rsrqResult =LTE_INVALID_RSRQ_dB;
            store->measResultNeighCells.push_back(newInfo);
        }
        //store->measResultNeighCellsNum = cellsTriggeredList->size();
    }

    // assure that all the related l3f result were retrieved
    //ERROR_Assert(l3fGetResult, "unexpected error
    //  (couldn't retrieve layer3 filtering result for measurement report)");

}

// /**
// FUNCTION   :: Layer3LteMeasProcessReport
// LAYER      :: Layer3
// PURPOSE    :: process report
// PARAMETERS ::
// + node               : Node*               : Pointer to node.
// + interfaceIndex     : int                 : Interface Index
// + measInfo           : MeasInfo*           : Measurement information
// + cellsTriggeredList : CellsTriggeredList* : cells triggered list
// RETURN     :: void : NULL
// **/
void Layer3LteMeasProcessReport(Node* node,
                                int interfaceIndex,
                                MeasInfo* measInfo,
                                CellsTriggeredList& relatedCells)
{
    // RRC instatnce
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    // VarMeasReportList
    VarMeasReportList* varMeasReportList = &layer3Data->varMeasReportList;

    // retrieve corresponding report info
    VarMeasReport* reportInfo = NULL;
    VarMeasReportList::iterator it_reportInfo =
        Layer3LteMeasGetReportInfo(*varMeasReportList, measInfo->id);
    if (it_reportInfo != varMeasReportList->end())
    {
        reportInfo = &(*it_reportInfo);
    }

    if (reportInfo == NULL)
    {
        // the first time to send

        // need not to send if relatedCells is empty
        if (relatedCells.size() < 1)
        {
            return;
        }

        // new entry
        VarMeasReport newVarMeasReport;
        newVarMeasReport.measId = measInfo->id;
        newVarMeasReport.numberOfReportsSent = 0;
        newVarMeasReport.cellsTriggeredList = relatedCells;     // copy
        // add
        varMeasReportList->push_back(newVarMeasReport);

        // send
        Layer3LteMeasSendMeasReport(node, interfaceIndex,
            newVarMeasReport.measId);
    }
    else
    {
        if (reportInfo->cellsTriggeredList.size() > 0)
        {
            // check diffrence of list
            BOOL containedInOldList = Layer3LteMeasContainedInOldList(
                relatedCells, reportInfo->cellsTriggeredList);
            if (containedInOldList)
            {
                // replace cellsTriggeredList
                reportInfo->cellsTriggeredList = relatedCells;
                // need not to re-report (report timer has already set)
            }
            else
            {
                // reset report info
                reportInfo->numberOfReportsSent = 0;
                reportInfo->cellsTriggeredList = relatedCells;    // copy
                // send
                Layer3LteMeasSendMeasReport(node, interfaceIndex,
                    reportInfo->measId);
            }
        }
        else
        {
            // need not to report no longer
            // erase from list
            varMeasReportList->erase(it_reportInfo);

            // stop timer if necessary
            Layer3LteMeasStopPeriodicalTimer(node, interfaceIndex,
                measInfo->id);
        }
    }
}

// /**
// FUNCTION   :: Layer3LteMeasGetEventConditionInfo
// LAYER      :: Layer3
// PURPOSE    :: get corresponding condition entry
// PARAMETERS ::
// + node               : Node*               : Pointer to node.
// + interfaceIndex     : int                 : Interface Index
// + measInfo           : MeasInfo*           : Measurement information
// + cond               : MeasEventConditionInfo**: corresponding entry
// + createdNewEntry    : BOOL*               : new entry was created or not
// RETURN     :: void : NULL
// **/
void Layer3LteMeasGetEventConditionInfo(
    Node* node,
   int interfaceIndex,
   const LteRnti& keyRnti,
   MeasInfo* measInfo,
   MeasEventConditionInfo** cond,
   BOOL* createdNewEntry)
{
    // RRC instatnce
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);
    // MeasEventConditionTable
    MeasEventConditionTable* eventCondTable =
        &layer3Data->measEventConditionTable;
    // MapRntiMeasEventConditionInfo
    MapRntiMeasEventConditionInfo* eventCondInfo = 
        &(eventCondTable->find(measInfo->id)->second);

    // corresponding condition entry
    MapRntiMeasEventConditionInfo::iterator it =
        eventCondInfo->find(keyRnti);
    if (it == eventCondInfo->end())
    {
        MeasEventConditionInfo newEntry;
        std::pair<MapRntiMeasEventConditionInfo::iterator, bool>
            insertResult = eventCondInfo->insert(
                MapRntiMeasEventConditionInfo::value_type(
                keyRnti, newEntry));
        *createdNewEntry = TRUE;
        *cond = &(insertResult.first->second);
    }
    else
    {
        *createdNewEntry = FALSE;
        *cond = &(it->second);
    }
}

// /**
// FUNCTION   :: Layer3LteMeasUpdateConditionInfoEntry
// LAYER      :: Layer3
// PURPOSE    :: update entry of condition table
// PARAMETERS ::
// + node               : Node*               : Pointer to node.
// + interfaceIndex     : int                 : Interface Index
// + cond               : MeasEventConditionInfo*: corresponding entry
// + enterCondition     : BOOL                : meet enter condition or not
// + leaveCondition     : BOOL                : meet leave condition or not
// + createdNewEntry    : BOOL*               : new entry was created or not
// RETURN     :: void : NULL
// **/
void Layer3LteMeasUpdateConditionInfoEntry(
    Node* node,
   int interfaceIndex,
   MeasEventConditionInfo* cond,
   BOOL enterCondition,
   BOOL leaveCondition,
   BOOL createdNewEntry)
{
    if (enterCondition)
    {
        if (cond->condition != MEAS_EVENT_CONDITION_ENTER)
        {
            cond->condition = MEAS_EVENT_CONDITION_ENTER;
            cond->changedTimeStamp = node->getNodeTime();
        }
    }
    else if (leaveCondition)
    {
        if (cond->condition != MEAS_EVENT_CONDITION_LEAVE)
        {
            cond->condition = MEAS_EVENT_CONDITION_LEAVE;
            if (createdNewEntry == TRUE)
            {
                cond->changedTimeStamp = -1;    // as invalid time
            }
            else
            {
                cond->changedTimeStamp = node->getNodeTime();
            }
        }
    }
    else
    {
        ;    // do nothing
    }
}

// /**
// FUNCTION   :: Layer3LteMeasUpdateConditionTable
// LAYER      :: Layer3
// PURPOSE    :: update condition table
// PARAMETERS ::
// + node               : Node*               : Pointer to node.
// + interfaceIndex     : int                 : Interface Index
// + measInfo           : MeasInfo*           : Measurement information
// RETURN     :: void : NULL
// **/
void Layer3LteMeasUpdateConditionTable(Node* node,
                                       int interfaceIndex,
                                       MeasInfo* measInfo)
{
    int phyIndex =
        LteGetPhyIndexFromMacInterfaceIndex(node, interfaceIndex);
    // phy
    PhyDataLte* phyLte = (PhyDataLte*)node->phyData[phyIndex]->phyVar;

    // Layer3Filtering
    LteLayer3Filtering* layer3Filtering =
        LteLayer2GetLayer3Filtering(node, interfaceIndex);
    // serving RNTI
    const LteRnti& servingRnti =
        Layer3LteGetConnectedEnb(node, interfaceIndex);
    // serving channel (DL)
    int servingCh = phyLte->dlChannelNo;

    // measurement event
    MeasEvent* measEvent = &(measInfo->reportConfig->measEvent);
    // event type
    MeasEventType eventType = measEvent->eventType;
    // measurement type
    LteMeasurementType measType = measEvent->quantityType;

    switch (eventType)
    {
    case MEAS_EVENT_A1:
    {
        // check only serving cell
        if (measInfo->measObject->channelNo != servingCh)
        {
            break;
        }

        // get serving L3 filtering result
        double value = 0;
        if (!layer3Filtering->get(servingRnti, measType, &value))
        {
            break;
        }

        LteRnti keyRnti = servingRnti;
        // corresponding condition entry
        MeasEventConditionInfo* cond;
        BOOL createdNewEntry = FALSE;
        Layer3LteMeasGetEventConditionInfo(node, interfaceIndex, keyRnti,
            measInfo, &cond, &createdNewEntry);

        // whether "entry" or "leave"
        BOOL enterCondition = value - measEvent->hysteresis >
            measEvent->eventVar.eventA1.threshold;
        BOOL leaveCondition = value + measEvent->hysteresis <
            measEvent->eventVar.eventA1.threshold;
        // update entry
        Layer3LteMeasUpdateConditionInfoEntry(node, interfaceIndex, cond,
            enterCondition, leaveCondition, createdNewEntry);

        break;
    }
    case MEAS_EVENT_A2:
    {
        // check only serving cell
        if (measInfo->measObject->channelNo != servingCh)
        {
            break;
        }

        // get serving L3 filtering result
        double value = 0;
        if (!layer3Filtering->get(servingRnti, measType, &value))
        {
            break;
        }

        LteRnti keyRnti = servingRnti;
        // corresponding condition entry
        MeasEventConditionInfo* cond;
        BOOL createdNewEntry = FALSE;
        Layer3LteMeasGetEventConditionInfo(node, interfaceIndex, keyRnti,
            measInfo, &cond, &createdNewEntry);

        // whether "entry" or "leave"
        BOOL enterCondition = value + measEvent->hysteresis <
            measEvent->eventVar.eventA2.threshold;
        BOOL leaveCondition = value - measEvent->hysteresis >
            measEvent->eventVar.eventA2.threshold;
        // update entry
        Layer3LteMeasUpdateConditionInfoEntry(node, interfaceIndex, cond,
            enterCondition, leaveCondition, createdNewEntry);

        break;
    }
    case MEAS_EVENT_A3:
    {
        // get serving L3 filtering result
        double servingValue = 0;
        if (!layer3Filtering->get(servingRnti, measType, &servingValue))
        {
            break;
        }
        // iterate condition entry
        SetLteRnti* rntiList =
            layer3Filtering->getRntiList(
            measType, measInfo->measObject->channelNo);
        if (rntiList == NULL)
        {
            break;
        }

        for (SetLteRnti::iterator it_rnti = rntiList->begin();
            it_rnti != rntiList->end(); it_rnti++)
        {
            LteRnti neighborRnti = *it_rnti;
            if (neighborRnti == servingRnti)
            {
                continue;
            }
            // get neighbor L3 filtering result
            double neighborValue = 0;
            if (!layer3Filtering->get(neighborRnti,
                measType, &neighborValue))
            {
                continue;
            }

            LteRnti keyRnti = neighborRnti;
            // corresponding condition entry
            MeasEventConditionInfo* cond;
            BOOL createdNewEntry = FALSE;
            Layer3LteMeasGetEventConditionInfo(node, interfaceIndex, keyRnti,
                measInfo, &cond, &createdNewEntry);

            // compare serving and neighbor
            BOOL enterCondition = neighborValue - measEvent->hysteresis >
                servingValue + measEvent->eventVar.eventA3.offset;
            BOOL leaveCondition = neighborValue + measEvent->hysteresis <
                servingValue +measEvent->eventVar.eventA3.offset;
            // update entry
            Layer3LteMeasUpdateConditionInfoEntry(node, interfaceIndex, cond,
                enterCondition, leaveCondition, createdNewEntry);
        }
        break;
    }
    case MEAS_EVENT_A4:
    {
        // iterate condition entry
        SetLteRnti* rntiList = layer3Filtering->getRntiList(
            measType, measInfo->measObject->channelNo);
        if (rntiList == NULL)
        {
            break;
        }

        for (SetLteRnti::iterator it_rnti = rntiList->begin();
            it_rnti != rntiList->end(); it_rnti++)
        {
            LteRnti neighborRnti = *it_rnti;
            if (neighborRnti == servingRnti)
            {
                continue;
            }
            // get neighbor L3 filtering result
            double neighborValue = 0;
            if (!layer3Filtering->get(neighborRnti,
                measType, &neighborValue))
            {
                continue;
            }

            LteRnti keyRnti = neighborRnti;
            // corresponding condition entry
            MeasEventConditionInfo* cond;
            BOOL createdNewEntry = FALSE;
            Layer3LteMeasGetEventConditionInfo(node, interfaceIndex, keyRnti,
                measInfo, &cond, &createdNewEntry);

            // compare neighbor and threshold
            BOOL enterCondition = neighborValue - measEvent->hysteresis >
                measEvent->eventVar.eventA4.threshold;
            BOOL leaveCondition = neighborValue + measEvent->hysteresis <
                measEvent->eventVar.eventA4.threshold;
            // update entry
            Layer3LteMeasUpdateConditionInfoEntry(node, interfaceIndex, cond,
                enterCondition, leaveCondition, createdNewEntry);
        }
        break;
    }
    case MEAS_EVENT_A5:
    {
        // get serving L3 filtering result
        double servingValue = 0;
        if (!layer3Filtering->get(servingRnti, measType, &servingValue))
        {
            break;
        }
        // iterate condition entry
        SetLteRnti* rntiList = layer3Filtering->getRntiList(
            measType, measInfo->measObject->channelNo);
        if (rntiList == NULL)
        {
            break;
        }

        for (SetLteRnti::iterator it_rnti = rntiList->begin();
            it_rnti != rntiList->end(); it_rnti++)
        {
            LteRnti neighborRnti = *it_rnti;
            if (neighborRnti == servingRnti)
            {
                continue;
            }
            // get neighbor L3 filtering result
            double neighborValue = 0;
            if (!layer3Filtering->get(neighborRnti,
                measType, &neighborValue))
            {
                continue;
            }

            LteRnti keyRnti = neighborRnti;
            // corresponding condition entry
            MeasEventConditionInfo* cond;
            BOOL createdNewEntry = FALSE;
            Layer3LteMeasGetEventConditionInfo(node, interfaceIndex, keyRnti,
                measInfo, &cond, &createdNewEntry);

            // compare serving and threshold1
            // and compare neighbor and threshold2
            BOOL enterCondition = servingValue + measEvent->hysteresis <
                    measEvent->eventVar.eventA5.threshold1
                && neighborValue - measEvent->hysteresis >
                    measEvent->eventVar.eventA5.threshold2;
            BOOL leaveCondition = servingValue - measEvent->hysteresis >
                    measEvent->eventVar.eventA5.threshold1
                && neighborValue + measEvent->hysteresis <
                    measEvent->eventVar.eventA5.threshold2;
            // update entry
            Layer3LteMeasUpdateConditionInfoEntry(node, interfaceIndex, cond,
                enterCondition, leaveCondition, createdNewEntry);
        }
        break;
    }
    default:
        ERROR_Assert(FALSE, "unexpected event type");

    }
}

// /**
// FUNCTION   :: Layer3LteMeasGetReportInfo
// LAYER      :: Layer3
// PURPOSE    :: get report info
// PARAMETERS ::
// + measReportList     : VarMeasReportList&  : report list
// + measId             : MeasId              : Measurement ID
// RETURN     :: VarMeasReportList::iterator  : iterator of retrieved info
// **/
VarMeasReportList::iterator Layer3LteMeasGetReportInfo(
    VarMeasReportList& measReportList, MeasId measId)
{
    for (VarMeasReportList::iterator it = measReportList.begin();
        it != measReportList.end(); it++)
    {
        if (it->measId == measId)
        {
            return it;
        }
    }
    return measReportList.end();
}

// /**
// FUNCTION   :: Layer3LteMeasContainedInOldList
// LAYER      :: Layer3
// PURPOSE    :: compare CellsTriggeredLists
// PARAMETERS ::
// + measReportList     : VarMeasReportList&  : report list
// + list1              : CellsTriggeredList  : listNew
// + list2              : CellsTriggeredList  : listOld
// RETURN     :: BOOL   : listNew is containd in listOld: TRUE
//                        otherwise                     : FALSE
// **/
BOOL Layer3LteMeasContainedInOldList(
    CellsTriggeredList& listNew, CellsTriggeredList& listOld)
{
    for (CellsTriggeredList::iterator it = listNew.begin();
        it != listNew.end(); it++)
    {
        if (listOld.find(*it) == listOld.end())
        {
            return FALSE;
        }
    }

    return TRUE;
}

// /**
// FUNCTION   :: Layer3LteMeasGetMeasInfo
// LAYER      :: Layer3
// PURPOSE    :: get measurement information
// PARAMETERS ::
// + measInfoList       : ListMeasInfo&       : measurement information list
// + measId             : MeasId              : Measurement ID
// RETURN     :: ListMeasInfo::iterator  : iterator of retrieved info
// **/
ListMeasInfo::iterator Layer3LteMeasGetMeasInfo(
    ListMeasInfo& measInfoList, MeasId measId)
{
    for (ListMeasInfo::iterator it = measInfoList.begin();
        it != measInfoList.end(); it++)
    {
        if (it->id == measId)
        {
            return it;
        }
    }
    return measInfoList.end();
}

// /**
// FUNCTION   :: Layer3LteMeasResetMeasurementValues
// LAYER      :: Layer3
// PURPOSE    :: get measurement information
// PARAMETERS ::
// + node               : Node*               : Pointer to node.
// + interfaceIndex     : int                 : Interface Index
// RETURN     :: void  : NULL
// **/
void Layer3LteMeasResetMeasurementValues(Node* node, int interfaceIndex)
{
    // RRC instatnce
    Layer3DataLte* layer3Data =
        LteLayer2GetLayer3DataLte(node, interfaceIndex);

    // cancel all timer
    Layer3LteMeasCancelAllTimer(node, interfaceIndex);

    // clear event table
    MeasEventConditionTable* condTable =
        &layer3Data->measEventConditionTable;
    for (MeasEventConditionTable::iterator it = condTable->begin();
        it != condTable->end(); it++)
    {
        it->second.clear();
    }

    // clear report field
    layer3Data->varMeasReportList.clear();

}
