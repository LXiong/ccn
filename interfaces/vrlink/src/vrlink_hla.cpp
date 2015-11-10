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


#include "vrlink_shared.h"
#include "vrlink_hla.h"

// /**
// FUNCTION :: VRLinkHLA
// PURPOSE :: Initializing function for HLA.
// PARAMETERS ::
// + ifacePtr : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
// **/
VRLinkHLA::VRLinkHLA(
    EXTERNAL_Interface* ifacePtr,
    NodeInput* nodeInput,
    VRLinkSimulatorInterface *simIface) : VRLink(ifacePtr, nodeInput, simIface)
{
    InitVariables(ifacePtr, nodeInput);
}

// /**
// FUNCTION :: VRLinkHLA13
// PURPOSE :: Initializing function for HLA13.
// PARAMETERS ::
// + ifacePtr : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
// **/
VRLinkHLA13::VRLinkHLA13(
    EXTERNAL_Interface* ifacePtr,
    NodeInput* nodeInput,
    VRLinkSimulatorInterface *simIface) : VRLinkHLA(ifacePtr, nodeInput, simIface)
{
}

// /**
// FUNCTION :: VRLinkHLA1516
// PURPOSE :: Initializing function for HLA1516.
// PARAMETERS ::
// + ifacePtr : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
// **/
VRLinkHLA1516::VRLinkHLA1516(
    EXTERNAL_Interface* ifacePtr,
    NodeInput* nodeInput,
    VRLinkSimulatorInterface *simIface) : VRLinkHLA(ifacePtr, nodeInput, simIface)
{
}

// /**
//FUNCTION :: InitVariables
//PURPOSE :: To initialize the HLA specific variables with default values.
//PARAMETERS ::
// + ifacePtr : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
//RETURN :: void : NULL
// **/
void VRLinkHLA::InitVariables(
    EXTERNAL_Interface* ifacePtr,
    NodeInput* nodeInput)
{
    iface = ifacePtr;
    
    if (simIface->GetPartitionIdForIface(iface) == 0)
    {
        rprFomVersion = 0;
        
        tickInterval = 0;
        dynamicStats = false;
        verboseMetricUpdates = false;
        sendNodeIdDescriptions = false;
        sendMetricDefinitions = false;
        //init the metric update buffer and size
        metricUpdateBuf[0] = 0;
        metricUpdateSize = 1;
        checkMetricUpdateInterval = 1 * SECOND;
        endProgram = false;

        //COMMENTED OUT NAWC GATEWAY CODE, currently unsupported in vrlink
        //nawcGatewayCompatibility = false;
        //attributeUpdateRequestDelay = 0;
        //attributeUpdateRequestInterval = 0;
        //maxAttributeUpdateRequests = 0;
        //numAttributeUpdateRequests = 0;
    }
}

// /**
//FUNCTION :: CreateFederation
//PURPOSE :: To make QualNet join the federation. If the federation does not
//           exist, the exconn API creates a new federation first, and then
//           register QualNet with it.
//PARAMETERS :: None
//RETURN :: void : NULL
// **/
void VRLinkHLA::CreateFederation()
{
    try
    {
        DtExerciseConnInitializer initializer;
        initializer.setExecName((DtString)execName.c_str());
        initializer.setFedFileName((DtString)fedPath.c_str());
        initializer.setRprFomVersion(rprFomVersion);
        initializer.setFederateType((DtString)fedName.c_str());
        DtExerciseConn::InitializationStatus status;

        cout << "VRLINK: Trying to create federation "<<execName<< " ... \n";

        exConn = new DtExerciseConn(initializer, &status);

        if (status != 0)
        {
            VRLinkReportError("VRLINK: Unable to join federation");
        }

        // Initialize VR-Link time.
        SetSimulationTime();
    }
    DtCATCH_AND_WARN(cout);

    cout << "VRLINK: Successfully joined federation." << endl;
}

// /**
//FUNCTION :: ScheduleTasksIfAny
//PURPOSE :: If dynamic stats are enabled, schedule corresponding tasks.
//PARAMETERS :: None
//RETURN :: void : NULL
// **/
void VRLinkHLA::ScheduleTasksIfAny()
{
    if (dynamicStats)
    {
        if (sendNodeIdDescriptions)
        {
            SendNodeIdDescriptionNotifications();
        }

        if (sendMetricDefinitions)
        {
            SendMetricDefinitionNotifications();
        }

        ScheduleCheckMetricUpdate();
    }
}

// /**
//FUNCTION :: InitConfigurableParameters
//PURPOSE :: To initialize the user configurable HLA parameters or initialize
//           the corresponding variables with the default values.
//PARAMETERS ::
// + nodeInput : NodeInput* : Pointer to cached file.
//RETURN :: void : NULL
// **/
void VRLinkHLA::InitConfigurableParameters(NodeInput* nodeInput)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];

    simIface->IO_ReadString(ANY_NODEID,
                            ANY_ADDRESS,
                            nodeInput,
                            "HLA",
                            &retVal,
                            buf);

    if (retVal && strcmp(buf, "YES") == 0)
    {   
        ReadLegacyParameters(nodeInput);
        return;
    }   

    VRLink::InitConfigurableParameters(nodeInput);

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DEBUG",
                &retVal,
                buf);

    cout << endl
         << "VRLINK debugging output is ";

    if (retVal && strcmp(buf, "YES") == 0)
    {
        debug = true;
        cout << "on." << endl;
    }
    else
    {
        debug = false;
        cout << "off." << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DEBUG-2",
                &retVal,
                buf);

    cout << "VRLINK debugging output level 2 is ";

    if (retVal && strcmp(buf, "YES") == 0)
    {
        debug2 = true;
        cout << "on." << endl;
    }
    else
    {
        debug2 = false;
        cout << "off." << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-FEDERATION-NAME",
                &retVal,
                buf);

    VRLinkVerify(retVal == TRUE,
                "Can't find VRLINK-FEDERATION-NAME parameter, .config file");

    execName = buf;

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-FED-FILE-PATH",
                &retVal,
                buf);

    VRLinkVerify(retVal == TRUE,
                 "Can't find VRLINK-FED-FILE-PATH parameter",
                 ".config file");

    fedPath = buf;

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-FEDERATE-NAME",
                &retVal,
                buf);

    VRLinkVerify(retVal == TRUE,
                 "Can't find VRLINK-FEDERATE-NAME parameter",
                  ".config file");
    fedName = buf;

    simIface->ReadDouble(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-RPR-FOM-VERSION",
                &retVal,
                &rprFomVersion);

    if (retVal == FALSE)
    {
        rprFomVersion = 1.0;
    }

    VRLinkVerify(rprFomVersion == 1.0 ||
                 rprFomVersion == 2.0017,
                 "Invalid RPR-FOM version (valid values: 1.0, 2.0017)");

    cout.precision(1);

    cout << "Federation Object Model is RPR FOM "
            << rprFomVersion << "." << endl;  

    /*//COMMENTED OUT NAWC GATEWAY CODE, currently unsupported in vrlink
    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-NAWC-GATEWAY-COMPATIBILITY",
                &retVal,
                buf);

    cout << "NAWC Gateway compatibility is ";

    if (retVal && strcmp(buf, "YES") == 0)
    {
        cout << "on." << endl;
        nawcGatewayCompatibility       = true;
        attributeUpdateRequestDelay    = 10 * SECOND;
        attributeUpdateRequestInterval = 10 * SECOND;
        maxAttributeUpdateRequests     = 3;
    }
    else
    {
        cout << "off." << endl;
        nawcGatewayCompatibility       = false;
        attributeUpdateRequestDelay    = 2 * SECOND;
        attributeUpdateRequestInterval = 2 * SECOND;
        maxAttributeUpdateRequests     = 1;
    }*/

    simIface->ReadTime(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-TICK-INTERVAL",
                &retVal,
                &tickInterval);

    if (retVal == FALSE)
    {
        tickInterval = 200 * MILLI_SECOND;
    }

    VRLinkVerify(tickInterval >= 0, "Can't use negative time");

    char tickIntervalString[MAX_STRING_LENGTH];
    simIface->PrintClockInSecond(tickInterval, tickIntervalString);

    cout << "VRLINK tick() interval = " << tickIntervalString << " second(s)"
            << endl;

    simIface->SetExternalReceiveDelay(iface, tickInterval);

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-HLA-DYNAMIC-STATISTICS",
                &retVal,
                buf);

    cout << endl
         << "VRLINK dynamic statistics (via Comment interaction)         = ";

    if (retVal && strcmp(buf, "NO") == 0)
    {
        dynamicStats = false;
        cout << "Off" << endl;
    }
    else
    {
        dynamicStats = true;
        cout << "On" << endl;
    }

    if (dynamicStats)
    {
        simIface->ReadString(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "VRLINK-HLA-DYNAMIC-STATISTICS-METRIC-UPDATE-MODE",
                    &retVal,
                    buf);

        cout << "VRLINK dynamic statistics, "
             << "Metric Update notifications      = ";

        if (!retVal || strcmp(buf, "BRIEF") != 0)
        {
            verboseMetricUpdates = true;
            cout << "Verbose" << endl;
        }
        else
        {
            verboseMetricUpdates = false;
            cout << "Brief" << endl;
        }

        simIface->ReadString(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "VRLINK-HLA-DYNAMIC-STATISTICS-SEND-NODEID-DESCRIPTIONS",
                    &retVal,
                    buf);

        cout << "VRLINK dynamic statistics, "
             << "nodeId Description notifications = ";

        if (retVal && strcmp(buf, "YES") == 0)
        {
            sendNodeIdDescriptions = true;
            cout << "On" << endl;
        }
        else
        {
            sendNodeIdDescriptions = false;
            cout << "Off" << endl;
        }

        simIface->ReadString(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "VRLINK-HLA-DYNAMIC-STATISTICS-SEND-METRIC-DEFINITIONS",
                    &retVal,
                    buf);

        cout << "VRLINK dynamic statistics, "
             << "Metric Definition notifications  = ";

        if (retVal && strcmp(buf, "YES") == 0)
        {
            sendMetricDefinitions = true;
            cout << "On" << endl;
        }
        else
        {
            sendMetricDefinitions = false;
            cout << "Off" << endl;
        }
    }//if//

    cout << endl;
}

//FUNCTION :: ReadLegacyParameters
//PURPOSE :: To initialize the user configurable HLA parameters or initialize
//           the corresponding variables with the default values.
//PARAMETERS ::
// + nodeInput : NodeInput* : Pointer to cached file.
//RETURN :: void : NULL
// **/
void VRLinkHLA::ReadLegacyParameters(NodeInput* nodeInput)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "HLA-DEBUG",
                &retVal,
                buf);

    cout << endl
         << "VRLINK debugging output is ";

    if (retVal && strcmp(buf, "YES") == 0)
    {
        debug = true;
        cout << "on." << endl;
    }
    else
    {
        debug = false;
        cout << "off." << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "HLA-DEBUG-2",
                &retVal,
                buf);

    cout << "VRLINK debugging output level 2 is ";

    if (retVal && strcmp(buf, "YES") == 0)
    {
        debug2 = true;
        cout << "on." << endl;
    }
    else
    {
        debug2 = false;
        cout << "off." << endl;
    }

    if (simIface->IsMilitaryLibraryEnabled())
    {
        simIface->ReadString(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "HLA-DEBUG-PRINT-RTSS",
                    &retVal,
                    buf);

        cout << "VRLINK debugging output (Ready To Send Signal interaction) is ";

        if (retVal && strcmp(buf, "NO") == 0)
        {
            debugPrintRtss = false;
            cout << "off." << endl;
        }
        else
        {
            debugPrintRtss = true;
            cout << "on." << endl;
        }
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "HLA-FEDERATION-NAME",
                &retVal,
                buf);

    VRLinkVerify(retVal == TRUE,
                "Can't find HLA-FEDERATION-NAME parameter, .config file");

    execName = buf;

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "HLA-FED-FILE-PATH",
                &retVal,
                buf);

    VRLinkVerify(retVal == TRUE,
                 "Can't find HLA-FED-FILE-PATH parameter",
                 ".config file");

    fedPath = buf;

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "HLA-FEDERATE-NAME",
                &retVal,
                buf);

    VRLinkVerify(retVal == TRUE,
                 "Can't find HLA-FEDERATE-NAME parameter",
                  ".config file");
    fedName = buf;

    simIface->ReadDouble(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "HLA-RPR-FOM-VERSION",
                &retVal,
                &rprFomVersion);

    if (retVal == FALSE)
    {
        rprFomVersion = 1.0;
    }

    VRLinkVerify(rprFomVersion == 1.0 || rprFomVersion == 2.0017,
                 "Invalid RPR-FOM version (valid values: 1.0, 2.0017)");

    cout.precision(1);

    cout << "Federation Object Model is RPR FOM "
            << rprFomVersion << "." << endl;


    /*//COMMENTED OUT NAWC GATEWAY CODE, currently unsupported in vrlink
    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "HLA-NAWC-GATEWAY-COMPATIBILITY",
                &retVal,
                buf);

    cout << "NAWC Gateway compatibility is ";

    if (retVal && strcmp(buf, "YES") == 0)
    {
        cout << "on." << endl;
        nawcGatewayCompatibility       = true;
        attributeUpdateRequestDelay    = 10 * SECOND;
        attributeUpdateRequestInterval = 10 * SECOND;
        maxAttributeUpdateRequests     = 3;
    }
    else
    {
        cout << "off." << endl;
        nawcGatewayCompatibility       = false;
        attributeUpdateRequestDelay    = 2 * SECOND;
        attributeUpdateRequestInterval = 2 * SECOND;
        maxAttributeUpdateRequests     = 1;
    }*/

    simIface->ReadDouble(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "HLA-XYZ-EPSILON",
                &retVal,
                &xyzEpsilon);

    if (retVal == FALSE)
    {
        xyzEpsilon = 0.5;
    }

    VRLinkVerify(xyzEpsilon >= 0.0, "Can't use negative epsilon value");

    cout << "GCC (x,y,z) epsilons are ("
            << xyzEpsilon << ","
            << xyzEpsilon << ","
            << xyzEpsilon << " meter(s))" << endl
         << "  For movement to be reflected in QualNet, change in position"
            " must be" << endl
         << "  >= any one of these values." << endl;

    simIface->ReadTime(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "HLA-TICK-INTERVAL",
                &retVal,
                &tickInterval);

    if (retVal == FALSE)
    {
        tickInterval = 200 * MILLI_SECOND;
    }

    VRLinkVerify(tickInterval >= 0, "Can't use negative time");

    char tickIntervalString[MAX_STRING_LENGTH];
    simIface->PrintClockInSecond(tickInterval, tickIntervalString);

    cout << "VRLINK tick() interval = " << tickIntervalString << " second(s)"
            << endl;

    simIface->SetExternalReceiveDelay(iface, tickInterval);

    simIface->ReadTime(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "HLA-MOBILITY-INTERVAL",
                &retVal,
                &mobilityInterval);

    if (retVal == FALSE)
    {
        mobilityInterval = 500 * MILLI_SECOND;
    }

    VRLinkVerify(mobilityInterval >= 0, "Can't use negative time");

    char mobilityIntervalString[MAX_STRING_LENGTH];
    simIface->PrintClockInSecond(mobilityInterval, mobilityIntervalString);

    cout << "VRLINK mobility interval = " << mobilityIntervalString
         << " second(s)" << endl;

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "HLA-DYNAMIC-STATISTICS",
                &retVal,
                buf);

    cout << endl
         << "VRLINK dynamic statistics (via Comment interaction)         = ";

    if (retVal && strcmp(buf, "NO") == 0)
    {
        dynamicStats = false;
        cout << "Off" << endl;
    }
    else
    {
        dynamicStats = true;
        cout << "On" << endl;
    }

    if (dynamicStats)
    {
        simIface->ReadString(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "HLA-DYNAMIC-STATISTICS-METRIC-UPDATE-MODE",
                    &retVal,
                    buf);

        cout << "VRLINK dynamic statistics, "
             << "Metric Update notifications      = ";

        if (!retVal || strcmp(buf, "BRIEF") != 0)
        {
            verboseMetricUpdates = true;
            cout << "Verbose" << endl;
        }
        else
        {
            verboseMetricUpdates = false;
            cout << "Brief" << endl;
        }

        simIface->ReadString(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "HLA-DYNAMIC-STATISTICS-SEND-NODEID-DESCRIPTIONS",
                    &retVal,
                    buf);

        cout << "VRLINK dynamic statistics, "
             << "nodeId Description notifications = ";

        if (retVal && strcmp(buf, "YES") == 0)
        {
            sendNodeIdDescriptions = true;
            cout << "On" << endl;
        }
        else
        {
            sendNodeIdDescriptions = false;
            cout << "Off" << endl;
        }

        simIface->ReadString(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "HLA-DYNAMIC-STATISTICS-SEND-METRIC-DEFINITIONS",
                    &retVal,
                    buf);

        cout << "VRLINK dynamic statistics, "
             << "Metric Definition notifications  = ";

        if (retVal && strcmp(buf, "YES") == 0)
        {
            sendMetricDefinitions = true;
            cout << "On" << endl;
        }
        else
        {
            sendMetricDefinitions = false;
            cout << "Off" << endl;
        }
    }//if//
    char path[g_PathBufSize];

    simIface->IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "HLA-ENTITIES-FILE-PATH",
        &retVal,
        path);

    VRLinkVerify(
        retVal == TRUE,
        "HLA-ENTITIES-FILE-PATH must be specified",
        __FILE__, __LINE__);

    entitiesPath = path;

    simIface->IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "HLA-RADIOS-FILE-PATH",
        &retVal,
        path);

    VRLinkVerify(
        retVal == TRUE,
        "HLA-RADIOS-FILE-PATH must be specified",
        __FILE__, __LINE__);

    radiosPath = path;

    simIface->IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "HLA-NETWORKS-FILE-PATH",
        &retVal,
        path);

    VRLinkVerify(
        retVal == TRUE,
        "HLA-NETWORKS-FILE-PATH must be specified",
        __FILE__, __LINE__);

    networksPath = path;


    cout << endl;
}

//COMMENTED OUT NAWC GATEWAY CODE, currently unsupported in vrlink
// /**
// FUNCTION :: GetNawcGatewayCompatibility
// PURPOSE :: Selector for returning whether NAWC gateway compatibility is
//            enabled or not.
// PARAMETERS :: None
// RETURN :: bool
// **/
/*bool VRLinkHLA::GetNawcGatewayCompatibility()
{
    return nawcGatewayCompatibility;
}*/

// /**
// FUNCTION :: GetMetricUpdateSize
// PURPOSE :: Selector for returning the metric update size.
// PARAMETERS :: None
// RETURN :: unsigned int
// **/
unsigned int VRLinkHLA::GetMetricUpdateSize()
{
    return metricUpdateSize;
}

// /**
// FUNCTION :: GetMetricUpdateBufSize
// PURPOSE :: Selector for returning the metric update buffer size.
// PARAMETERS :: None
// RETURN :: unsigned int
// **/
unsigned int VRLinkHLA::GetMetricUpdateBufSize()
{
    return sizeof(metricUpdateBuf);
}

// /**
// FUNCTION :: IsVerboseMetricUpdates
// PURPOSE :: Returns whether verbose metric updates are enabled or not.
// PARAMETERS :: None
// RETURN :: bool
// **/
bool VRLinkHLA::IsVerboseMetricUpdates()
{
    return verboseMetricUpdates;
}

// /**
// FUNCTION :: SetMetricUpdateSize
// PURPOSE :: Selector for setting the value of the next pointer.
// PARAMETERS ::
// + size : unsigned int : Size of the update metric to be set.
// RETURN :: void : NULL
// **/
void VRLinkHLA::SetMetricUpdateSize(unsigned int size)
{
    metricUpdateSize = size;
}

// /**
// FUNCTION :: AppendMetricUpdateBuf
// PURPOSE :: To append information to the metric update buffer.
// PARAMETERS ::
// + line : const char* : Size of the update metric to be set.
// RETURN :: void : NULL
// **/
void VRLinkHLA::AppendMetricUpdateBuf(const char* line)
{
    strcat(metricUpdateBuf, line);
}

// /**
// FUNCTION :: ProcessEvent
// PURPOSE :: Handles all the protocol events for HLA.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg : Message* : Pointer to message.
// RETURN :: void : NULL
// **/
void VRLinkHLA::ProcessEvent(Node* node, Message* msg)
{
    switch (simIface->GetMessageEventTypesUsedInVRLink(
        simIface->ReturnEventTypeFromMsg(msg)))
    {
        case EXTERNAL_VRLINK_HierarchyMobility:
            {
                AppProcessMoveHierarchy(node, msg);
                break;
            }
        case EXTERNAL_VRLINK_CheckMetricUpdate:
            {
                AppProcessCheckMetricUpdateEvent(node, msg);
                break;
            }
        case EXTERNAL_VRLINK_AckTimeout:
            {
                AppProcessTimeoutEvent(msg);
                break;
            }
        case EXTERNAL_VRLINK_SendRtss:
            {
                // The VRLINK Federation has created this message
                // in the method ScheduleRtssNotification by way of
                // ReflectRadioAttributes ()
                if (simIface->IsMilitaryLibraryEnabled())
                {
                    Node* rtssNode;
                    RtssForwardedInfo* rtssForwardedInfo = (RtssForwardedInfo*)
                                              simIface->ReturnInfoFromMsg(msg);

                    //You are on the partition with the node so get node* from local node hash
                    rtssNode = simIface->GetNodePtrFromOtherNodesHash(
                            node, rtssForwardedInfo->nodeId, false);

                    AppProcessSendRtssEvent(rtssNode);
                    break;
                }
            }
        case EXTERNAL_VRLINK_SendRtssForwarded:
            {
                // See AppProcessSendRtssEvent() for sender...
                // This messages was MSG_EXTERNAL_VRLINK_SendRtss, but
                // we are receiving this notification from a remote
                // partition and thus have to locate the node by nodeId.
                if (simIface->IsMilitaryLibraryEnabled())
                {                    
                    Node* rtssNode;
                    RtssForwardedInfo* rtssForwardedInfo = (RtssForwardedInfo*)
                                              simIface->ReturnInfoFromMsg(msg);
                    rtssNode = simIface->GetNodePtrFromOtherNodesHash(
                            node, rtssForwardedInfo->nodeId, true);
                    SendRtssNotification(rtssNode);
                    break;
                }
            }
        case EXTERNAL_VRLINK_StartMessengerForwarded:
            {
                // See method SendSimulatedMsgUsingMessenger() for sender
                EXTERNAL_VRLinkStartMessenegerForwardedInfo* startInfo =
                    (EXTERNAL_VRLinkStartMessenegerForwardedInfo*)
                    simIface->ReturnInfoFromMsg(msg);
                Node* srcNode;
                srcNode = simIface->GetNodePtrFromOtherNodesHash(
                        node, startInfo->srcNodeId, false);

                StartSendSimulatedMsgUsingMessenger(
                    srcNode,
                    startInfo->srcNetworkAddress,
                    startInfo->destNodeId,
                    startInfo->smInfo,
                    startInfo->requestedDataRate,
                    startInfo->dataMsgSize,
                    startInfo->voiceMsgDuration,
                    startInfo->isVoice,
                    startInfo->timeoutDelay,
                    startInfo->unicast,
                    startInfo->sendDelay,
                    startInfo->isLink11,
                    startInfo->isLink16);
                    break;
            }
        case EXTERNAL_VRLINK_CompletedMessengerForwarded:
            {
                EXTERNAL_VRLinkCompletedMessenegerForwardedInfo*
                    completedInfo =
                           (EXTERNAL_VRLinkCompletedMessenegerForwardedInfo*)
                            simIface->ReturnInfoFromMsg(msg);

               
                    Node* destNode = NULL;
                    destNode = simIface->GetNodePtrFromOtherNodesHash(
                            node, completedInfo->destNodeId, true);
                    if (!destNode)
                    {
                        destNode = simIface->GetNodePtrFromOtherNodesHash(
                                node, completedInfo->destNodeId, false);
                    }
                    AppMessengerResultCompleted(destNode,
                                                completedInfo->smInfo,
                                                completedInfo->success);
                
                break;
            }
        case EXTERNAL_VRLINK_ChangeMaxTxPower:
            {
                AppProcessChangeMaxTxPowerEvent(msg);
                break;
            }
        case EXTERNAL_VRLINK_InitialPhyMaxTxPowerRequest:
            {
                AppProcessGetInitialPhyTxPowerRequest(node, msg);
                break;
            }
        case EXTERNAL_VRLINK_InitialPhyMaxTxPowerResponse:
            {
                AppProcessGetInitialPhyTxPowerResponse(msg);
                break;
            }
        default:
            VRLinkReportError(
                "Unknown message type encountered",
                __FILE__, __LINE__);
    }//end of switch

    simIface->FreeMessage(node, msg);
}

// /**
// FUNCTION :: AppProcessCheckMetricUpdateEvent
// PURPOSE :: Process the HLA event MSG_EXTERNAL_VRLINK_CheckMetricUpdate.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg : Message* : Pointer to message.
// RETURN :: void : NULL
// **/
void VRLinkHLA::AppProcessCheckMetricUpdateEvent(Node* node, Message* msg)
{
    if (metricUpdateSize > 1)
    {
        SendMetricUpdateNotification();
    }
    ScheduleCheckMetricUpdate();
}

// /**
// FUNCTION :: SendMetricUpdateNotification
// PURPOSE :: Sends the metric update notification to another federate.
// PARAMETERS :: None.
// RETURN :: void : NULL
// **/
void VRLinkHLA::SendMetricUpdateNotification()
{
    VRLinkVariableDatumSetInfo vdsInfo;
    memset(&vdsInfo, 0, sizeof(VRLinkVariableDatumSetInfo));

    VRLinkVariableDatumInfo & vdInfo = vdsInfo.variableDatumsInfo[0];

    // NumberOfVariableDatums, DatumID.
    vdsInfo.numberOfVariableDatums = 1;

    vdInfo.datumId = metricUpdateNotificationDatumId;

    if (strlen(metricUpdateBuf) > sizeof(vdInfo.datumValue))
    {
        char errString[MAX_STRING_LENGTH];

        sprintf(errString,
                "Size of the metric update buffer is more than the defined "
                "datumValue size by %d. Either increase the buffer size or "
                "check for the merticUpdateBuf.",
                strlen(metricUpdateBuf)-sizeof(vdInfo.datumValue));

        VRLinkReportError(errString, __FILE__, __LINE__);
    }

    strcpy((char*) vdInfo.datumValue, metricUpdateBuf);

    // Comment data is almost ready, except the DatumLength
    // field still needs to be set. Set it, and transmit the Comment.

    unsigned datumLengthInBytes = strlen((char*) vdInfo.datumValue) + 1;

    if (datumLengthInBytes > g_maxOutgoingCommentIxnDatumValueSize)
    {
        char errString[MAX_STRING_LENGTH];

        sprintf(errString,
                "Size of the outgoing comment interaction datum value is "
                " more than the allowed size %d.",
                g_maxOutgoingCommentIxnDatumValueSize);

        VRLinkReportError(errString, __FILE__, __LINE__);
    }

    //assigning size in bit length
    vdInfo.datumLength = datumLengthInBytes * 8;

    SendCommentIxn(&vdsInfo);

    //re-init the metric update buffer and size
    metricUpdateBuf[0] = 0;
    metricUpdateSize = 1;
}

// /**
// FUNCTION :: ScheduleCheckMetricUpdate
// PURPOSE :: Schedules the HLA event MSG_EXTERNAL_VRLINK_CheckMetricUpdate.
// PARAMETERS :: None.
// RETURN :: void : NULL
// **/
void VRLinkHLA::ScheduleCheckMetricUpdate()
{
    Node* node = simIface->GetFirstNodeOnPartition(iface);

    Message* newMsg = simIface->AllocateMessage(node,
                                      simIface->GetExternalLayerId(),
                                      simIface->GetVRLinkExternalInterfaceType(),
                                      simIface->GetMessageEventType(EXTERNAL_VRLINK_CheckMetricUpdate));

    if (newMsg != NULL)
    {
        simIface->SendMessageWithDelay(node,
                                       newMsg,
                                       checkMetricUpdateInterval);
    }
}

// /**
// FUNCTION :: SendNodeIdDescriptionNotifications
// PURPOSE :: Sends the nodeId description notification to another federate.
// PARAMETERS :: None.
// RETURN :: void : NULL
// **/
void VRLinkHLA::SendNodeIdDescriptionNotifications()
{
    if (radios.empty())
    {
        VRLinkReportWarning("No radios found", __FILE__, __LINE__);
        return;
    }

    VRLinkVariableDatumSetInfo vdsInfo;
    memset(&vdsInfo, 0, sizeof(VRLinkVariableDatumSetInfo));

    VRLinkVariableDatumInfo& vdInfo = vdsInfo.variableDatumsInfo[0];

    // NumberOfVariableDatums, DatumID.
    vdsInfo.numberOfVariableDatums = 1;

    vdInfo.datumId = nodeIdDescriptionNotificationDatumId;

    char line[g_maxOutgoingCommentIxnDatumValueSize];
    hash_map <NodeId, VRLinkRadio*>::iterator radiosIter;

    // Initial size of DatumValue field includes the space for a terminating
    // null character.

    unsigned dvOffset = 0;
    unsigned currentDatumValueSizeInBytes = 1;

    for (radiosIter = radios.begin();radiosIter != radios.end();radiosIter++)
    {
        VRLinkRadio* radio = radiosIter->second;

        VRLinkVerify(
                (radio->GetEntityPtr() != NULL),
                "Radio not associated with any entity",
                __FILE__, __LINE__);

        sprintf(
            line,
            "%u \"%s\" %u\n",
            simIface->GetNodeId(radio->GetNode()),
            radio->GetEntityPtr()->GetMarkingData().c_str(),
            radio->GetRadioIndex());

        unsigned lineLength = strlen(line);

        VRLinkVerify(
            (lineLength < sizeof(line)),
            "Invalid length of line",
            __FILE__, __LINE__);

        unsigned potentialDatumValueSizeInBytes
            = currentDatumValueSizeInBytes + lineLength;

        // At least one record should fit in the notification.

        if (currentDatumValueSizeInBytes == 1)
        {
            VRLinkVerify(
                (potentialDatumValueSizeInBytes
                               <= g_maxOutgoingCommentIxnDatumValueSize),
                "Potential datum value size cannot exceed the maximum" \
                " outgoing comment ineraction datum value size",
                __FILE__, __LINE__);
        }

        if (potentialDatumValueSizeInBytes
                                     > g_maxOutgoingCommentIxnDatumValueSize)
        {
            // Null-terminate string, send notification, and reset indexes to
            // DatumValue.
            vdInfo.datumValue[dvOffset] = 0;

            dvOffset = 0;
            unsigned datumLengthInBytes = strlen((char*) vdInfo.datumValue)
                                            + 1;

            if (datumLengthInBytes > g_maxOutgoingCommentIxnDatumValueSize)
            {
                char errString[MAX_STRING_LENGTH];

                sprintf(errString,
                        "Size of the outgoing comment interaction"
                        " datum value is "
                        " more than the allowed size %d.",
                        g_maxOutgoingCommentIxnDatumValueSize);

                VRLinkReportError(errString, __FILE__, __LINE__);
            }
            //assigning size in bit length
            vdInfo.datumLength = datumLengthInBytes * 8;

            //send comment Ixn
            SendCommentIxn(&vdsInfo);

            currentDatumValueSizeInBytes = 1;

            potentialDatumValueSizeInBytes
                = currentDatumValueSizeInBytes + lineLength;

            VRLinkVerify(
                (potentialDatumValueSizeInBytes
                               <= g_maxOutgoingCommentIxnDatumValueSize),
                "Potential datum value size cannot exceed the maximum" \
                " outgoing comment ineraction datum value size",
                __FILE__, __LINE__);
        }//if//

        CopyToOffset(
            vdInfo.datumValue,
            dvOffset,
            line,
            lineLength);

        currentDatumValueSizeInBytes = potentialDatumValueSizeInBytes;
    }//for//

    // Done loading all records.
    // If there's untransmitted data in DatumValue, transmit it now.

    if (currentDatumValueSizeInBytes > 1)
    {
        // Null-terminate string, send notification, and reset indexes to
        // DatumValue.

        vdInfo.datumValue[dvOffset] = 0;
        unsigned datumLengthInBytes = strlen((char*) vdInfo.datumValue) + 1;

        if (datumLengthInBytes > g_maxOutgoingCommentIxnDatumValueSize)
        {
            char errString[MAX_STRING_LENGTH];

            sprintf(errString,
                   "Size of the outgoing comment interaction datum value is "
                   " more than the allowed size %d.",
                   g_maxOutgoingCommentIxnDatumValueSize);

            VRLinkReportError(errString, __FILE__, __LINE__);
        }
        //assigning size in bit length
        vdInfo.datumLength = datumLengthInBytes * 8;

        //send comment Ixn
        SendCommentIxn(&vdsInfo);

        dvOffset = 0;
        currentDatumValueSizeInBytes = 1;
    }
}

// /**
// FUNCTION :: SendMetricDefinitionNotifications
// PURPOSE :: Sends the metric definition notification to another federate.
// PARAMETERS :: None.
// RETURN :: void : NULL
// **/
void VRLinkHLA::SendMetricDefinitionNotifications()
{
    VRLinkVariableDatumSetInfo vdsInfo;
    memset(&vdsInfo, 0, sizeof(VRLinkVariableDatumSetInfo));

    VRLinkVariableDatumInfo& vdInfo = vdsInfo.variableDatumsInfo[0];

    // NumberOfVariableDatums, DatumID.
    vdsInfo.numberOfVariableDatums = 1;

    vdInfo.datumId = metricDefinitionNotificationDatumId;

    char line[g_maxOutgoingCommentIxnDatumValueSize];

    // Initial size of DatumValue field includes the space for a terminating
    // null character.

    unsigned dvOffset = 0;
    unsigned currentDatumValueSizeInBytes = 1;
    unsigned i;

    for (i = 0; i < MAX_LAYERS; i++)
    {
        const MetricLayerData& metricDataForLayer = simIface->getMetricLayerData(i);

        VRLinkVerify(
            (metricDataForLayer.numberUsed >= 0),
            "Metric data number used cannot be negative",
            __FILE__, __LINE__);

        unsigned j;

        for (j = 0; j < (unsigned) metricDataForLayer.numberUsed; j++)
        {
            VRLinkVerify(
                (metricDataForLayer.metricList != NULL),
                "Metric list not specified",
                __FILE__, __LINE__);

            MetricData& metric = metricDataForLayer.metricList[j];

            sprintf(
                line,
                "%d \"%s\" %d %d\n",
                metric.metricID,
                metric.metricName,
                (int) i,
                metric.metricDataType);

            unsigned lineLength = strlen(line);

            VRLinkVerify(
                (lineLength < sizeof(line)),
                "Invalid length of line",
                __FILE__, __LINE__);

            unsigned potentialDatumValueSizeInBytes
                                 = currentDatumValueSizeInBytes + lineLength;

            // At least one record should fit in the notification.

            if (currentDatumValueSizeInBytes == 1)
            {
                VRLinkVerify(
                    (potentialDatumValueSizeInBytes
                                   <= g_maxOutgoingCommentIxnDatumValueSize),
                    "Potential datum value size cannot exceed the maximum" \
                    " outgoing comment ineraction datum value size",
                    __FILE__, __LINE__);
            }

            if (potentialDatumValueSizeInBytes
                                     > g_maxOutgoingCommentIxnDatumValueSize)
            {
                // Null-terminate string, send notification,
                // and reset indexes to DatumValue.

                vdInfo.datumValue[dvOffset] = 0;
                unsigned datumLengthInBytes =
                                       strlen((char*) vdInfo.datumValue) + 1;

                if (datumLengthInBytes >
                    g_maxOutgoingCommentIxnDatumValueSize)
                {
                    char errString[MAX_STRING_LENGTH];

                    sprintf(errString,
                            "Size of the outgoing comment interaction "
                            " datum value is "
                            " more than the allowed size %d.",
                            g_maxOutgoingCommentIxnDatumValueSize);

                    VRLinkReportError(errString, __FILE__, __LINE__);
                }
                //assigning size in bit length
                vdInfo.datumLength = datumLengthInBytes * 8;

                //send comment Ixn
                SendCommentIxn(&vdsInfo);

                dvOffset = 0;
                currentDatumValueSizeInBytes = 1;

                potentialDatumValueSizeInBytes
                    = currentDatumValueSizeInBytes + lineLength;

                VRLinkVerify(
                    (potentialDatumValueSizeInBytes
                                   <= g_maxOutgoingCommentIxnDatumValueSize),
                    "Potential datum value size cannot exceed the maximum" \
                    " outgoing comment ineraction datum value size",
                    __FILE__, __LINE__);
            }//if//

            CopyToOffset(
                vdInfo.datumValue,
                dvOffset,
                line,
                lineLength);

            currentDatumValueSizeInBytes = potentialDatumValueSizeInBytes;
        }//for//
    }//for//

    // Done loading all records.
    // If there's untransmitted data in DatumValue, transmit it now.

    if (currentDatumValueSizeInBytes > 1)
    {
        // Null-terminate string, send notification, and reset indexes to
        // DatumValue.

        vdInfo.datumValue[dvOffset] = 0;
        unsigned datumLengthInBytes = strlen((char*) vdInfo.datumValue) + 1;

        if (datumLengthInBytes > g_maxOutgoingCommentIxnDatumValueSize)
        {
            char errString[MAX_STRING_LENGTH];

            sprintf(errString,
                   "Size of the outgoing comment interaction datum value is "
                   " more than the allowed size %d.",
                   g_maxOutgoingCommentIxnDatumValueSize);

            VRLinkReportError(errString, __FILE__, __LINE__);
        }
        //assigning size in bit length
        vdInfo.datumLength = datumLengthInBytes * 8;

        //send comment Ixn
        SendCommentIxn(&vdsInfo);

        dvOffset = 0;
        currentDatumValueSizeInBytes = 1;
    }
}

// /**
// FUNCTION :: SendCommentIxn
// PURPOSE :: Sends comment interaction to another federate.
// PARAMETERS ::
// + vdsInfo : VRLinkVariableDatumSetInfo* : Variable datum set infomation to
//                                           be sent.
// RETURN :: void : NULL
// **/
void VRLinkHLA::SendCommentIxn(VRLinkVariableDatumSetInfo* vdsInfo)
{
    DtCommentInteraction commentIxn;
    DtDatumRetCode returnCode;
    VRLinkVariableDatumInfo& vdInfo = vdsInfo->variableDatumsInfo[0];

    unsigned datumLengthInBytes = 0;

    // The Comment interaction sent contains only the parameters specified
    // in the ICD and those parameters required for RPR FOM 1.0.

    // OriginatingEntity and ReceivingEntity parameter.
    // Zero values are sent, since these parameters are not used in the ICD.
    // These fields are nonetheless required for RPR FOM 1.0.

    try
    {
        commentIxn.setSenderId(DtEntityIdentifier(0,0,0));
        commentIxn.setReceiverId(DtEntityIdentifier(0,0,0));
        commentIxn.setNumFixedFields(0);

        VRLinkVerify(
            (vdsInfo->numberOfVariableDatums == 1),
            "Currently, only a single variable datums are supported",
            __FILE__, __LINE__);

        commentIxn.setNumVarFields(vdsInfo->numberOfVariableDatums);
        commentIxn.setVarDatumId(1, vdInfo.datumId, &returnCode);

        if (returnCode == DtDatumBadIndex)
        {
            VRLinkReportError(
             "Invalid datum index or data",
             __FILE__, __LINE__);
        }

        VRLinkVerify(
            (vdInfo.datumLength % 8 == 0),
            "Invalid datum length",
            __FILE__, __LINE__);

        datumLengthInBytes = vdInfo.datumLength / 8;
        commentIxn.setVarDataBytes(1, datumLengthInBytes, &returnCode);

        if (returnCode == DtDatumBadIndex)
        {
            VRLinkReportError(
             "Invalid datum index or data",
             __FILE__, __LINE__);
        }

        commentIxn.setDatumValByteArray(
                       DtVar,
                       1,
                       (const char *)vdInfo.datumValue,
                       &returnCode);

        if (returnCode == DtDatumBadIndex)
        {
            VRLinkReportError(
             "Invalid datum index or data",
             __FILE__, __LINE__);
        }

        exConn->sendStamped(commentIxn);
    }
    DtCATCH_AND_WARN(cout);
}

// /**
// FUNCTION :: GetRadioIdKey
// PURPOSE :: Get the RadioIdKey for signal interaction.
// PARAMETERS ::
// + dataIxnInfo : DtSignalInteraction* : Pointer to signal interaction
// RETURN :: RadioIdKey : EntityIdentifierKey and int pair
// **/
RadioIdKey VRLinkHLA::GetRadioIdKey(DtSignalInteraction* inter)

{
    return inter->radioId();
}

// /**
// FUNCTION :: GetRadioIdKey
// PURPOSE :: Get the RadioIdKey for radio transmitter repository.
// PARAMETERS ::
// + dataIxnInfo : DtRadioTransmitterRepository* : Pointer to radio 
//                  transmitter repository
// RETURN :: RadioIdKey : EntityIdentifierKey and int pair
// **/
RadioIdKey VRLinkHLA::GetRadioIdKey(DtRadioTransmitterRepository* radioTsr)
{
    return radioTsr->globalId();
}

// /**
// FUNCTION :: SendDataIxn
// PURPOSE :: Sends data interaction to another federate.
// PARAMETERS ::
// + dataIxnInfo : VRLinkDataIxnInfo* : Pointer to data interaction
//                                      information to be sent.
// RETURN :: void : NULL
// **/
void VRLinkHLA::SendDataIxn(VRLinkDataIxnInfo& dataIxnInfo)
{
    DtDataInteraction dataIxn;
    DtDatumRetCode returnCode;
    VRLinkVariableDatumSetInfo& vdsInfo = dataIxnInfo.variableDatumSetInfo;
    VRLinkVariableDatumInfo& vdInfo     = vdsInfo.variableDatumsInfo[0];

    unsigned datumLengthInBytes = vdInfo.datumLength / 8;

    // The Data interaction sent contains only the parameters specified in
    // the ICD and those parameters required for RPR FOM 1.0.

    try
    {
        // OriginatingEntity parameter.
        dataIxn.setSenderId(dataIxnInfo.originatingEntity);

        // ReceivingEntity parameter.
        // Zero values are sent, since this parameter is not used in the ICD.
        // This field is nonetheless required for RPR FOM 1.0.

        dataIxn.setReceiverId(DtEntityIdentifier(0, 0, 0));

        // RequestIdentifier parameter.
        // Zero values are sent, since this parameter is not used in the ICD.
        // This field is nonetheless required for RPR FOM 1.0.

        unsigned nboRequestIdentifier = 0;

        dataIxn.setRequestId(nboRequestIdentifier);
        dataIxn.setNumFixedFields(0);

        // VariableDatumSet parameter.
        // NumberofVariableDatums, DatumId, DatumLength, DatumValue

        VRLinkVerify(
            (vdsInfo.numberOfVariableDatums == 1),
            "Currently, only a single variable datums are supported",
            __FILE__, __LINE__);

        dataIxn.setNumVarFields(vdsInfo.numberOfVariableDatums);
        dataIxn.setVarDatumId(1, vdInfo.datumId, &returnCode);

        if (returnCode == DtDatumBadIndex)
        {
            VRLinkReportError(
             "Invalid datum index or data",
             __FILE__, __LINE__);
        }

        VRLinkVerify(
            (vdInfo.datumLength % 8 == 0),
            "Invalid datum length",
            __FILE__, __LINE__);

        datumLengthInBytes = vdInfo.datumLength / 8;

        VRLinkVerify(
            (datumLengthInBytes <= g_maxOutgoingDataIxnDatumValueSize),
            "Datum length cannot exceed the maximum outgoing data" \
            " interaction length",
            __FILE__, __LINE__);

        dataIxn.setVarDataBytes(1, datumLengthInBytes, &returnCode);

        if (returnCode == DtDatumBadIndex)
        {
            VRLinkReportError(
             "Invalid datum index or data",
             __FILE__, __LINE__);
        }

        dataIxn.setDatumValByteArray(
                    DtVar,
                    1,
                    (const char *)vdInfo.datumValue,
                    &returnCode);

        if (returnCode == DtDatumBadIndex)
        {
            VRLinkReportError(
             "Invalid datum index or data",
             __FILE__, __LINE__);
        }

        exConn->sendStamped(dataIxn);
    }
    DtCATCH_AND_WARN(cout);
}

// /**
// FUNCTION :: ProcessSignalIxn
// PURPOSE :: Processes the signal interaction received from an outside
//            federate.
// PARAMETERS ::
// + inter : DtSignalInteraction* : Received interaction.
// RETURN :: void : NULL
// **/
void VRLinkHLA::ProcessSignalIxn(DtSignalInteraction* inter)
{
    if ((inter->protocolId() == cerUserProtocolId) &&
                          (inter->encodingClass() ==  DtApplicationSpecific))
    {
        if (debug)
        {
            cout << "VRLINK: Received ApplicationSpecificRadioSignal "
                 << "interaction"<<endl;
        }

        ProcessCommEffectsRequest(inter);
    }
    else
    {
        if (debug)
        {
            cout << "VRLINK: Received non-subscribed interaction ";
        }

        if (debug2)
        {
            cout <<"VRLINK: Ignoring ApplicationSpecificRadioSignal interaction"
                    " with UserProtocolID " <<inter->protocolId()<< endl;
        }
    }
}

// /**
// FUNCTION :: RegisterProtocolSpecificCallbacks
// PURPOSE :: Registers HLA specific callbacks.
// PARAMETERS :: None.
// RETURN :: void : NULL
// **/
void VRLinkHLA::RegisterProtocolSpecificCallbacks()
{
    DtDataInteraction::addCallback(
                           exConn,
                           VRLinkHLA::CbDataIxnReceived,
                           this);
}

// /**
// FUNCTION :: CbDataIxnReceived
// PURPOSE :: Callback to receive data interaction.
// PARAMETERS ::
// + inter : DtDataInteraction* : Received data interaction.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLinkHLA::CbDataIxnReceived(DtDataInteraction* inter, void* usr)
{
    VRLinkHLA* hla = (VRLinkHLA*)usr;

    bool debug = hla->GetDebug();   
    bool debug2 = hla->GetDebug2();

    if (debug2)
    {
        cout << "VRLINK: Received Data interaction" << endl;
    }

    if (inter->numVarFields() != 1)
    {
        if (debug2)
        {
            cout << "VRLINK: Ignoring Data interaction where"
                    " NumberOfVariableDatums != 1" << endl;
        }
        return;
    }

    //Ensure that this check is really required
    if (inter->datumSizeBits(DtVar, 1) % 8 != 0)
    {
        if (debug)
        {
            cout << "VRLINK: Ignoring Data interaction where"
                    " DatumLength is not a multiple of 8" << endl;
        }
        return;
    }

    //Ensure that this check is really required
    if ((unsigned)(inter->datumSizeBytes(DtVar, 1)) > g_DatumValueBufSize)
    {
        if (debug)
        {
            cout << "VRLINK: Ignoring Data interaction containing"
                    " large DatumValue field" << endl;
        }
        return;
    }

    DtDatumParam varDatumId = inter->datumId(DtVar, 1);

    if (varDatumId != DtDatumParam(0))
    {
        if (varDatumId == terminateCesNotificationDatumId)
        {
            hla->ProcessTerminateCesRequest();
        }
        else
        {
            if (debug2)
            {
                cout << "VRLINK: Ignoring Data interaction with DatumID "
                        << varDatumId << endl;
            }
        }
    }
    else
    {
        if (debug2)
        {
            cout << "VRLINK: Ignoring Data interaction with invalid DatumID\n";
        }
    }
}
// /**
// FUNCTION :: DeregisterCallbacks
// PURPOSE :: Removed callback registered with exercise connection
// PARAMETERS :: None.
// **/
void VRLinkHLA::DeregisterCallbacks()
{
    DtDataInteraction::removeCallback(
                           exConn,
                           VRLinkHLA::CbDataIxnReceived,
                           this);
}
// /**
// FUNCTION :: ~VRLinkHLA
// PURPOSE :: Destructor of Class VRLinkHLA.
// PARAMETERS :: None.
// **/
VRLinkHLA::~VRLinkHLA()
{
    //Clean up vrlink exercise connection: resign from rti and destroy federation
    char errorString[200];
    std::string except;
    try {

#if DtHLA_1516
         exConn->rtiAmb()->resignFederationExecution(RTI::CANCEL_THEN_DELETE_THEN_DIVEST);
#else 
         exConn->rtiAmb()->resignFederationExecution(RTI::DELETE_OBJECTS_AND_RELEASE_ATTRIBUTES);
#endif
    }
    catch (const RTI::Exception& e)
    {
#if DtHLA_1516
        except = DtToString(e.what());
        sprintf(errorString, "RTI: Exception in resignFederationExecution: %s\n", except.c_str());
#else
        sprintf(errorString, "RTI: Exception in resignFederationExecution: %s: %s\n", e._name, e._reason);
#endif
        VRLinkReportError(errorString, __FILE__, __LINE__);
    }
      
    if (exConn->destroyFedExecFlag())
    {
        try 
        { 
#if DtHLA_1516
            exConn->rtiAmb()->destroyFederationExecution(DtToWString(exConn->execName())); 
#else
            exConn->rtiAmb()->destroyFederationExecution(exConn->execName().string()); 
#endif
        }
        catch(RTI::FederatesCurrentlyJoined&) 
        {
            if (debug2)
            {
                sprintf(errorString, "RTI: Exception in destroyFederationExecution: Other federates are still connected");
                VRLinkReportWarning(errorString, __FILE__, __LINE__);
            }
        }
        catch (const RTI::Exception& ex) 
        {
#if DtHLA_1516
            except = DtToString(ex.what());
            sprintf(errorString, "RTI: Exception in destroyFederationExecution: %s\n", except.c_str());
#else
            sprintf(errorString, "RTI: Exception in destroyFederationExecution: %s: %s\n", ex._name, ex._reason);
#endif
            VRLinkReportError(errorString, __FILE__, __LINE__);
        }
    }
}

// /**
// FUNCTION :: AppProcessSendRtssEvent
// PURPOSE :: Handles the event type MSG_EXTERNAL_VRLINK_SendRtss.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN :: void : NULL
// **/
void VRLinkHLA::AppProcessSendRtssEvent(Node* node)
{
    if (simIface->IsMilitaryLibraryEnabled())
    {
        if (simIface->GetLocalPartitionIdForNode(node) != 0)
        {
            // We currently are not on p0, this is a remote node processing this;
            // forward the processing of the ready to send event to p0
            // We'd like to use MESSAGE_RemoteSend (node, destNodeId, msg, -1);
            // but we can't becuase that looks up which partition the node
            // lives on, and we actually need to send to p0 proxy node.
            // We'd like to use EXTERNAL_MESSAGE_SendAnyNode () but again we
            // can't because that schedules on the partition where the node
            // really lives.
            // 1.) Allocate an RtssEventNotification message

            Message * rtssForwardedMsg = simIface->AllocateMessage(
                node,
                simIface->GetExternalLayerId(),       // special layer
                simIface->GetVRLinkExternalInterfaceType(),      // protocol
                simIface->GetMessageEventType(EXTERNAL_VRLINK_SendRtssForwarded));
            // 2.) Set info - info is the node's nodeId

            simIface->AllocateInfoInMsg (
                node,
                rtssForwardedMsg,
                sizeof(RtssForwardedInfo));

            RtssForwardedInfo * rtssForwardedInfo = (RtssForwardedInfo *)
                simIface->ReturnInfoFromMsg (rtssForwardedMsg);

            rtssForwardedInfo->nodeId = simIface->GetNodeId(node);

            // 3.) Forward the message to p0 - vrlink interface on p0 will then
            // be able complete handling via SendRtssNotification()

            EXTERNAL_Interface* iface =
                                   simIface->GetExternalInterfaceForVRLink(node);

            simIface->RemoteExternalSendMessage (
                          iface,
                          0,
                          rtssForwardedMsg,
                          0,
                          simIface->GetExternalScheduleSafeType());
        }
        else
        {
            SendRtssNotification(node);
        }
    }
}

// /**
//FUNCTION :: UpdateMetric
//PURPOSE :: Updates values of stored metrics sent by the GUI.
//PARAMETERS ::
// + unsigned : NodeId : node id of metric
// + metric : const MetricData : definition of metric being updated
// + metricValue : void* : pointer to new value of the metric
// + updateTime : clocktype : when this value update occurred
//RETURN :: void : NULL
// **/
void VRLinkHLA::UpdateMetric(unsigned nodeId,
        const MetricData& metric,
        void* metricValue,
        clocktype updateTime)
{
    if (!dynamicStats)
    {
        return;
    }

    //UpdateMetric is called from series of calls originating form gui.cpp as it sends stats updates
    
    VRLinkVerify(
        (metricUpdateSize <= GetMetricUpdateBufSize()),
        "Metric update size cannot exceed the buffer size",
        __FILE__, __LINE__);

    VRLinkVerify(
        (updateTime >= 0),
        "Update time cannot be zero",
        __FILE__, __LINE__);

    char metricValueString[g_metricUpdateBufSize];

    MetricDataTypesUsedInVRLink typeUsed = simIface->GetMetricDataTypesUsedInVRLink(metric.metricDataType);
    switch (typeUsed)
    {
        case IntegerType:
            sprintf(metricValueString, "%d", *((int*) metricValue));
            break;
        case DoubleType:
            sprintf(metricValueString, "%f", *((double*) metricValue));
            break;
        case UnsignedType:
            sprintf(metricValueString, "%u", *((unsigned*) metricValue));
            break;
        default:
            VRLinkReportError("Invalid datum index or data", __FILE__, __LINE__);
    }

    char updateTimeString[g_ClocktypeStringBufSize];
    ctoa(updateTime, updateTimeString);

    char line[g_metricUpdateBufSize];

    if (verboseMetricUpdates)
    {
        VRLinkVerify(
            (simIface->GetPartitionForIface(GetExtIfacePtr()) != NULL),
             "Partition data for interface not found",
             __FILE__, __LINE__);

        const hash_map<NodeId, VRLinkData*>& nodeIdToPerNodeDataHashPtr =
                                        GetNodeIdToPerNodeDataHash();

        hash_map<NodeId, VRLinkData*>::const_iterator
                                                nodeIdToPerNodeDataHashIter;

        nodeIdToPerNodeDataHashIter =
                               nodeIdToPerNodeDataHashPtr.find((NodeId)nodeId);

        VRLinkVerify(
            (nodeIdToPerNodeDataHashIter != nodeIdToPerNodeDataHashPtr.end()),
            "Entry not found in nodeIdToPerNodeDataHash",
            __FILE__, __LINE__);

        const VRLinkData* vrlinkData = nodeIdToPerNodeDataHashIter->second;
        const VRLinkRadio* radio  = vrlinkData->GetRadioPtr();

        if (radio != NULL)
        {
            VRLinkVerify(
                (radio->GetEntityPtr() != NULL),
                "Radio not associated with any entity",
                __FILE__, __LINE__);

            const VRLinkEntity* entity = radio->GetEntityPtr();

            if (!IsEntityMapped(entity))
            {
                return;
            }

            sprintf(
                line,
                "%u \"%s\" %s %u %d \"%s\" %d %d %s %s\n",
                nodeId,
                entity->GetMarkingData().c_str(),
                entity->GetEntityIdString(),
                radio->GetRadioIndex(),
                metric.metricID,
                metric.metricName,
                metric.metricLayerID,
                metric.metricDataType,
                metricValueString,
                updateTimeString);
        }
        else
        {
            sprintf(
                line,
                "%u \"\" -1 -1 %d \"%s\" %d %d %s %s\n",
                nodeId,
                metric.metricID,
                metric.metricName,
                metric.metricLayerID,
                metric.metricDataType,
                metricValueString,
                updateTimeString);
        }
    }
    else
    {
        sprintf(
            line,
            "%u %d %s %s\n",
            nodeId,
            metric.metricID,
            metricValueString,
            updateTimeString);
    }
    
    unsigned lineLength = strlen(line);

    VRLinkVerify(
        (lineLength < sizeof(line)),
        "Invalid length of line",
        __FILE__, __LINE__);

    unsigned potentialMetricUpdateSize = metricUpdateSize + lineLength;

    if (metricUpdateSize == 1)
    {
        VRLinkVerify(
            (potentialMetricUpdateSize <= GetMetricUpdateBufSize()),
            "Potential metric update size cannot exceed the buffer size",
            __FILE__, __LINE__);
    }

    if (potentialMetricUpdateSize <= GetMetricUpdateBufSize())
    {
        AppendMetricUpdateBuf(line);
        SetMetricUpdateSize(potentialMetricUpdateSize);
        return;
    }

    SendMetricUpdateNotification();
    potentialMetricUpdateSize = metricUpdateSize + lineLength;

    VRLinkVerify(
        (potentialMetricUpdateSize <= GetMetricUpdateBufSize()),
        "Potential metric update size cannot exceed the buffer size",
        __FILE__, __LINE__);

    AppendMetricUpdateBuf(line);
    SetMetricUpdateSize(potentialMetricUpdateSize);
}
