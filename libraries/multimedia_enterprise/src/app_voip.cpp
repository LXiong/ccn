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

//--------------------------------------------------------------------------
// VOIP performance issue:
//--------------------------------------------------------------------------
// Application must pay close attention to packet size, buffer size, packet
// loss and packet latency. Larger packet loss introduce degradation in
// audio quality. In addition large packet size and buffer size increase
// delay. Studies have shown that losing traffic that is greater than 32 ms
// in duration is problematic for listeners since speech phonemes are lost.
// However, traffic losses between 4-16 ms are not noticeable to the
// listener. The important thing for any VOIP application, from a
// performance perspective, is to minimize end-to-end delay. Studies have
// shown, the overall delay of packet transmissions should not exceed
// 200 ms; however, delays of less than 150 ms are desirable. Typically,
// when delay reaches 800 ms, normal conversation is impeded
//--------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "api.h"
#include "app_util.h"
#include "app_voip.h"
#include "multimedia_h323.h"
#include "transport_rtp.h"
#include "multimedia_sip.h"
#include "transport_udp.h"
#include "network_ip.h"
// Pseudo traffic sender layer
#include "app_trafficSender.h"

#ifdef ADDON_DB
#include "db.h"
#include "dbapi.h"
#endif // ADDON_DB

#define DEBUG_VOIP 0
#define DEBUG_VOIP_Err 0

#ifdef DEBUG_VOIP
#include "partition.h"
void
diag_reportFoundValue (const char * called,
    VoipHostData* voipHost, Node * node)
{
    if (voipHost)
    {
        printf ("Node %d calling %s has found data\n    %s to %s\n"
            "sessionStart %" TYPES_64BITFMT "d, end %" TYPES_64BITFMT "d\n",
            node->nodeId, called, voipHost->sourceAliasAddr,
            voipHost->destAliasAddr, voipHost->sessionInitiate,
            voipHost->endTime);
    }
    else
    {
        printf ("Node %d calling %s at %" TYPES_64BITFMT
                "d has found NULL data\n",
                node->nodeId, called, node->getNodeTime());
    }
}
#endif

//-----------------------------------------------------------
// NAME         : VoipGetEncodingSchemeDetails()
// PURPOSE      : get Encoding Scheme Details
// ASSUMPTION   : None.
// RETURN VALUE : None.
//-----------------------------------------------------------
void VoipGetEncodingSchemeDetails(char* codecScheme,
                           clocktype& packetizationInterval,
                           unsigned int& bitRatePerSecond)
{
    int index;
    char intervalMS[MAX_STRING_LENGTH];

    VoipEncodingScheme encodingScheme[] =
           {
               {"G.711", 64000, "20MS"},
               {"G.729", 8000, "20MS"},
               {"G.723.1ar6.3", 6300, "30MS"},
               {"G.723.1ar5.3", 5300, "30MS"},
               {"G.726ar32", 32000, "20MS"},
               {"G.726ar24", 24000, "20MS"},
               {"G.728ar16", 16000, "30MS"}
            };

    int upperIndex = sizeof(encodingScheme) / sizeof(VoipEncodingScheme);

    for (index = 0; index < upperIndex ;index++)
    {
        //Found Encoding Scheme
        if (strcmp (encodingScheme[index].codecScheme, codecScheme) == 0)
        {
            if (packetizationInterval == 0)
            {
                strcpy(intervalMS,
                   encodingScheme[index].packetizationIntervalInMilliSec);
                packetizationInterval = TIME_ConvertToClock(intervalMS);
            }
            bitRatePerSecond = encodingScheme[index].bitRatePerSecond;
            break;
        }
    }

    if (index == upperIndex)
    {
        //Assign Default Encoding Scheme Details
        strcpy(intervalMS, encodingScheme[0].packetizationIntervalInMilliSec);
        packetizationInterval = TIME_ConvertToClock(intervalMS);
        bitRatePerSecond = encodingScheme[0].bitRatePerSecond;
    }

    if (DEBUG_VOIP)
    {
        if (index < upperIndex)
        {
            strcpy(encodingScheme[0].codecScheme, codecScheme);
        }
        printf(" encoding scheme %s, "
               "Packetization Interval ""%" TYPES_64BITFMT "d",
                                encodingScheme[0].codecScheme,
                                packetizationInterval);
    }
}


//-----------------------------------------------------------
// NAME         : VoipReadConfiguration()
// PURPOSE      : Read VOIP related data
// ASSUMPTION   : None.
// RETURN VALUE : None.
//-----------------------------------------------------------

void VoipReadConfiguration(Node* srcNode,
                      Node* destNode,
                      const NodeInput *nodeInput,
                      char* codecScheme,
                      clocktype& packetizationInterval,
                      unsigned int& bitRatePerSecond)
{
    double readTotalLossProbablityVal;
    BOOL retVal;
    AppMultimedia* srcMultimedia = NULL;
    AppMultimedia* destMultimedia = NULL;
    if (srcNode) {
        srcMultimedia = srcNode->appData.multimedia;
    }
    if (destNode) {
        destMultimedia = destNode->appData.multimedia;
    }

    if ((!srcMultimedia) && (!destMultimedia))
    {
        ERROR_ReportError("Incorrect usage: set "
            "MULTIMEDIA-SIGNALLING-PROTOCOL as SIP or H323");
    }

    BOOL wasFound;
    clocktype time;

    // Initialize connectionDelay for the node
    // If not specified, default value is assigned
    if (srcNode != NULL)
    {
        IO_ReadTime(
            srcNode->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "VOIP-CONNECTION-DELAY",
        &wasFound,
        &time);

        if (wasFound)
        {
            if (time <= 0)
            {
                // invalid entry in config
                ERROR_ReportError("Invalid VOIP-CONNECTION-DELAY value\n");
            }

            if (srcMultimedia) {
                srcMultimedia->connectionDelay = time;
            }
        }
        else
        {
            if (srcMultimedia) {
                srcMultimedia->connectionDelay =
                       VOIP_DEFAULT_CONNECTION_DELAY;
            }
        }
    }
    if (destNode != NULL)
    {
        IO_ReadTime(destNode->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "VOIP-CONNECTION-DELAY",
        &wasFound,
        &time);

        if (wasFound)
        {
            if (time <= 0)
            {
                // invalid entry in config
                ERROR_ReportError("Invalid VOIP-CONNECTION-DELAY value\n");
            }

            if (destMultimedia) {
                destMultimedia->connectionDelay = time;
            }
        }
        else
        {
            if (destMultimedia) {
                destMultimedia->connectionDelay =
                    VOIP_DEFAULT_CONNECTION_DELAY;
            }
        }
    }

    // Initialize callTimeout for the node
    // If not specified, default value is assigned
    if (srcNode != NULL)
    {
        IO_ReadTime(srcNode->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "VOIP-CALL-TIMEOUT",
        &wasFound,
        &time);

        if (wasFound)
        {
            if (time <= 0)
            {
                // invalid entry in config
                ERROR_ReportError("Invalid VOIP-CALL-TIMEOUT value\n");
            }

            if (srcMultimedia) {
                srcMultimedia->callTimeout = time;
            }
        }
        else
        {
            if (srcMultimedia) {
                srcMultimedia->callTimeout = VOIP_DEFAULT_CALL_TIMEOUT;
            }
        }
    }
    if (destNode != NULL)
    {
        IO_ReadTime(destNode->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "VOIP-CALL-TIMEOUT",
        &wasFound,
        &time);

        if (wasFound)
        {
            if (time <= 0)
            {
                // invalid entry in config
                ERROR_ReportError("Invalid VOIP-CALL-TIMEOUT value\n");
            }

            if (destMultimedia) {
                destMultimedia->callTimeout = time;
            }
        }
        else
        {
            if (destMultimedia) {
                destMultimedia->callTimeout = VOIP_DEFAULT_CALL_TIMEOUT;
            }
        }
    }
    if (srcNode != NULL)
    {
        //read the Total Loss Probability from Config file
        IO_ReadDouble(
        srcNode->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "VOIP-TOTAL-LOSS-PROBABILITY",
        &retVal,
        &readTotalLossProbablityVal);

        // Zero nos of Packet is not permitted
        if (retVal && readTotalLossProbablityVal > 0.0)
        {
            if (srcMultimedia) {
                srcMultimedia->totalLossProbablity
                                    = readTotalLossProbablityVal / 100.0;
            }
        }
        else
        {
            if (srcMultimedia) {
                srcMultimedia->totalLossProbablity
                                = DEFAULT_TOTAL_LOSS_PROBABILITY / 100.0;
            }
        }
    }
    if (destNode != NULL)
    {
        IO_ReadDouble(destNode->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "VOIP-TOTAL-LOSS-PROBABILITY",
        &retVal,
        &readTotalLossProbablityVal);

        // Zero nos of Packet is not permitted
        if (retVal && readTotalLossProbablityVal > 0.0)
        {
            if (destMultimedia) {
                destMultimedia->totalLossProbablity
                                    = readTotalLossProbablityVal / 100.0;
            }
        }
        else
        {
            if (destMultimedia) {
                destMultimedia->totalLossProbablity
                                = DEFAULT_TOTAL_LOSS_PROBABILITY / 100.0;
            }
        }
    }
    VoipGetEncodingSchemeDetails(codecScheme,
                                 packetizationInterval,
                                 bitRatePerSecond);
}


//--------------------------------------------------------------------------
// NAME         : VoipConnectionEstablished()
// PURPOSE      : H323 indicate that connectio has been established.
// ASSUMPTION   : None.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void VoipConnectionEstablished(Node* node, VoipHostData* voip)
{
    voip->status = VOIP_SESSION_ESTABLISHED;
    voip->sessionEstablished = node->getNodeTime();

    if (DEBUG_VOIP)
    {
        printf("Initiating RTP session on terminating side\n");
    }
    VOIPSendRtpInitiateNewSession (
            node,
            voip->localAddr,
            voip->sourceAliasAddr,
            voip->remoteAddr,
            voip->packetizationInterval,
            voip->initiatorPort,
            voip->initiator,
            (void*) &VoipReceiveDataFromRtp);
}


//--------------------------------------------------------------------------
// NAME         : VoipCloseSession()
// PURPOSE      : Close voip session.
// ASSUMPTION   : None.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void VoipCloseSession(Node* node, VoipHostData* voip)
{
    if (voip->status == VOIP_SESSION_ESTABLISHED)
    {
        voip->sessionFinish = node->getNodeTime();
        voip->sessionIsClosed = TRUE;

        if (node->appData.appStats)
        {
            voip->stats->SessionFinish(node);
        }
        voip->status = VOIP_SESSION_CLOSED;

        VOIPSendRtpTerminateSession(
                node,
                voip->remoteAddr,
                voip->initiatorPort);
    }
}


//--------------------------------------------------------------------------
// NAME         : VoipGetCallInitiatorData()
// PURPOSE      : search for a voip call initiator data structure.
// ASSUMPTION   : None.
// RETURN VALUE : The pointer to the desired data structure,
//                NULL if nothing found.
//--------------------------------------------------------------------------
VoipHostData* VoipGetCallInitiatorData(
    Node* node,
    NodeAddress srcAddr,
    unsigned short initiatorPort)
{
    AppInfo *appList = node->appData.appPtr;
    VoipHostData* voipHost;

    while (appList)
    {
        if (appList->appType == APP_VOIP_CALL_INITIATOR)
        {
            voipHost = (VoipHostData*) appList->appDetail;

            if ((voipHost->initiatorPort == initiatorPort) &&
                (voipHost->localAddr == srcAddr))
            {
                return voipHost;
            }
        }
        appList = appList->appNext;
    }
    return NULL;
}


//--------------------------------------------------------------------------
// NAME         : VoipGetCallReceiverData()
// PURPOSE      : search for a voip call receiver data structure.
// ASSUMPTION   : None.
// RETURN VALUE : The pointer to the desired data structure,
//                NULL if nothing found.
//--------------------------------------------------------------------------
VoipHostData* VoipGetCallReceiverData(
    Node* node,
    NodeAddress srcAddr,
    unsigned short initiatorPort)
{
    AppInfo *appList = node->appData.appPtr;
    VoipHostData* voipHost;

    while (appList)
    {
        if (appList->appType == APP_VOIP_CALL_RECEIVER)
        {
            voipHost = (VoipHostData*) appList->appDetail;

            if (voipHost->remoteAddr == srcAddr
                && voipHost->initiatorPort == initiatorPort)
            {
                return voipHost;
            }
        }
        appList = appList->appNext;
    }
    return NULL;
}


VoipHostData* VoipGetReceiverDataForTargetAlias(Node* node, char* target)
{
    AppInfo *appList = node->appData.appPtr;
    VoipHostData* voipHost;

    while (appList)
    {
        if (appList->appType == APP_VOIP_CALL_RECEIVER)
        {
            voipHost = (VoipHostData*) appList->appDetail;

            if (!strcmp(voipHost->sourceAliasAddr, target))
            {
                return voipHost;
            }
        }
        appList = appList->appNext;
    }
    return NULL;
}


//--------------------------------------------------------------------------
// NAME         : VoipUpdateInitiator()
// PURPOSE      : update voip initiator data
// PARAMETERS   : 
// + node       : Node*         : pointer to node
// + voip       : VoipHostData* : voip host data
// + openResult : TransportToAppOpenResult* : open result data
// + isDirect   : bool          : TRUE if direct call FALSE otherwise
// RETURN :: NONE
//--------------------------------------------------------------------------
void
VoipUpdateInitiator(Node* node,
                    VoipHostData* voip,
                    TransportToAppOpenResult* openResult,
                    bool isDirect)
{
    AppInfo *appList = node->appData.appPtr;
    /*
     * fill in connection id, etc.
     */
    voip->connectionId = openResult->connectionId;
    voip->localAddr = openResult->localAddr.interfaceAddr.ipv4;
    if (isDirect)
    {
        voip->remoteAddr = openResult->remoteAddr.interfaceAddr.ipv4;
    }
    voip->callSignalAddr = openResult->localAddr.interfaceAddr.ipv4;
}


//--------------------------------------------------------------------------
// NAME         : VoipGetInitiatorDataForCallSignal()
// PURPOSE      : search for a voip call initiator data structure.
// ASSUMPTION   : None.
// RETURN VALUE : The pointer to the desired data structure,
//                NULL if nothing found.
//--------------------------------------------------------------------------
VoipHostData* VoipGetInitiatorDataForCallSignal(
    Node* node,
    unsigned short callSignalPort)
{
    AppInfo *appList = node->appData.appPtr;
    VoipHostData* voipHost;

    while (appList)
    {
        if (appList->appType == APP_VOIP_CALL_INITIATOR)
        {
            voipHost = (VoipHostData*) appList->appDetail;

            if (voipHost->callSignalPort == callSignalPort)
            {
                return voipHost;
            }
        }
        appList = appList->appNext;
    }
    return NULL;
}


//--------------------------------------------------------------------------
// NAME         : VoipGetReceiverDataForCallSignal()
// PURPOSE      : search for a voip call receiver data structure.
// ASSUMPTION   : None.
// RETURN VALUE : The pointer to the desired data structure,
//                NULL if nothing found.
//--------------------------------------------------------------------------
VoipHostData* VoipGetReceiverDataForCallSignal(
    Node* node,
    unsigned short callSignalSourcePort)
{
    AppInfo *appList = node->appData.appPtr;
    VoipHostData* voipHost;

    while (appList)
    {
        if (appList->appType == APP_VOIP_CALL_RECEIVER)
        {
            voipHost = (VoipHostData*) appList->appDetail;

            if (voipHost->callSignalPort == callSignalSourcePort)
            {
                return voipHost;
            }
        }
        appList = appList->appNext;
    }
    return NULL;
}


// added
//--------------------------------------------------------------------------
// NAME         : VoipGetReceiverDataForCallSignalPort()
// PURPOSE      : search for a voip call receiver data structure.
// ASSUMPTION   : None.
// RETURN VALUE : The pointer to the desired data structure,
//                NULL if nothing found.
//--------------------------------------------------------------------------
VoipHostData* VoipGetReceiverDataForCallSignal2(
    Node* node,
    unsigned short callSignalSourcePort)
{
    AppInfo *appList = node->appData.appPtr;
    VoipHostData* voipHost;

    while (appList)
    {
        if (appList->appType == APP_VOIP_CALL_RECEIVER)
        {
            voipHost = (VoipHostData*) appList->appDetail;

            if (voipHost->callSignalPort == callSignalSourcePort)
            {
                return voipHost;
            }
        }
        appList = appList->appNext;
    }
    return NULL;
}


//--------------------------------------------------------------------------
// NAME         : VoipGetHostDataForConnectionId()
// PURPOSE      : Search for a voip host data structure by connectionId.
// RETURN VALUE : The pointer to the desired data structure,
//                NULL if nothing found.
//--------------------------------------------------------------------------
static
VoipHostData* VoipGetHostDataForConnectionId(
    Node *node,
    int connectionId,
    AppType appType)
{
    AppInfo *appList = node->appData.appPtr;
    VoipHostData* voipHostData;

    while (appList)
    {
        if (appList->appType == appType)
        {
            voipHostData = (VoipHostData*) appList->appDetail;

            if (voipHostData->connectionId == connectionId)
            {
                return voipHostData;
            }
        }

        appList = appList->appNext;
    }

    return NULL;
}


//--------------------------------------------------------------------------
// NAME         : VoipCallInitiatorGetDataForConnectionId()
// PURPOSE      : Search for a voip call initator data structure by
//                connectionId.
// RETURN VALUE : The pointer to the desired data structure,
//                NULL if nothing found.
//--------------------------------------------------------------------------
VoipHostData* VoipCallInitiatorGetDataForConnectionId(
    Node *node,
    int connectionId)
{
    VoipHostData* voipHostData = NULL;

    voipHostData = VoipGetHostDataForConnectionId(
                            node,
                            connectionId,
                            APP_VOIP_CALL_INITIATOR);

    return voipHostData;
}


//--------------------------------------------------------------------------
// NAME         : VoipCallReceiverGetDataForConnectionId()
// PURPOSE      : Search for a voip call receiver data structure by
//                connectionId.
// RETURN VALUE : The pointer to the desired data structure,
//                NULL if nothing found.
//--------------------------------------------------------------------------
VoipHostData* VoipCallReceiverGetDataForConnectionId(
    Node *node,
    int connectionId)
{
    VoipHostData* voipHostData = NULL;

    voipHostData = VoipGetHostDataForConnectionId(
                            node,
                            connectionId,
                            APP_VOIP_CALL_RECEIVER);

    return voipHostData;
}


//--------------------------------------------------------------------------
// NAME         : VoipGetHostDataForProxyConnectionId()
// PURPOSE      : Search for a voip host data structure by proxy
//                routed connectionId.
// RETURN VALUE : The pointer to the desired data structure,
//                NULL if nothing found.
//--------------------------------------------------------------------------
static
VoipHostData* VoipGetHostDataForProxyConnectionId(
    Node *node,
    int connectionId,
    AppType appType)
{
    AppInfo *appList = node->appData.appPtr;  //prob
    VoipHostData* voipHostData;

    while (appList)
    {
        if (appList->appType == appType)
        {
            voipHostData = (VoipHostData*) appList->appDetail;

            if (voipHostData->proxyConnectionId == connectionId)
            {
                return voipHostData;
            }
        }

        appList = appList->appNext;
    }

    return NULL;
}


//--------------------------------------------------------------------------
// NAME         : VoipCallInitiatorGetDataForProxyConnectionId()
// PURPOSE      : Search for a voip call initator data structure by
//                connectionId for proxy routed connection.
// RETURN VALUE : The pointer to the desired data structure,
//                NULL if nothing found.
//--------------------------------------------------------------------------
VoipHostData* VoipCallInitiatorGetDataForProxyConnectionId(
    Node *node,
    int connectionId)
{
    VoipHostData* voipHostData = NULL;

    voipHostData = VoipGetHostDataForProxyConnectionId(
                            node,
                            connectionId,
                            APP_VOIP_CALL_INITIATOR);

    return voipHostData;
}


//--------------------------------------------------------------------------
// NAME         : VoipCallReceiverGetDataForProxyConnectionId()
// PURPOSE      : Search for a voip call receiver data structure by
//                connectionId for proxy routed connection.
// RETURN VALUE : The pointer to the desired data structure,
//                NULL if nothing found.
//--------------------------------------------------------------------------
VoipHostData* VoipCallReceiverGetDataForProxyConnectionId(
    Node *node,
    int connectionId)
{
    VoipHostData* voipHostData = NULL;

    voipHostData = VoipGetHostDataForProxyConnectionId(
                            node,
                            connectionId,
                            APP_VOIP_CALL_RECEIVER);

    return voipHostData;
}

//--------------------------------------------------------------------------
// NAME         : VoipSendDataToRtp()
// PURPOSE      : Send data to RTP. Data types have the following meaning
//                m --> More
//                o --> Over for now
//                e --> End session
// RETURN VALUE : None
//--------------------------------------------------------------------------
static
void VoipSendDataToRtp(
    Node* node,
    VoipHostData* voipHostData,
    BOOL overNow,
    BOOL endSession)
{
    int payloadSize;
    char* payload;
    VoipData data;
    clocktype samplingTime;
    double pps;
    int bitsPerInterval;




    // bitRate = 64000bps (for G.711
    // pps = packetizationInterval in Sec (i.e. 1000/20 for G.711)
    // Payload Size = (bitRate * pps) / 8 = 160 byte

    pps = (double) voipHostData->packetizationInterval / SECOND;

    // number of bits sent during each packetisation interval
    bitsPerInterval = (int) (pps * (voipHostData->bitRate));

    // calculate the payload size in terms of octet
    payloadSize = bitsPerInterval / 8;

    if ((unsigned int)payloadSize < sizeof(VoipData) )
    {
        char buf[MAX_STRING_LENGTH];
        sprintf(buf, "\nEncoding Scheme with Packetization "
            "Interval both are not match. Increase Packetization "
            "Interval value. For Node ID = %d\n", node->nodeId);
        ERROR_Assert(FALSE, buf);
    }
    payload = (char*) MEM_malloc(payloadSize);
    memset(payload, 0, payloadSize);

    if (DEBUG_VOIP_Err)
    {
        printf("VOIP: Node %u prepare to send data to Rtp\n"
               "      Payload size = %d\n",
               node->nodeId, payloadSize);
    }

    memset(&data, 0, sizeof(VoipData));
    if (endSession)
    {
        data.type = 'e';
    }
    else if (overNow)
    {
        data.type = 'o';
    }
    else
    {
        data.type = 'm';
    }

    if (voipHostData->initiator)
    {
        data.srcAddr = voipHostData->localAddr;
    }
    else
    {
        data.srcAddr = voipHostData->remoteAddr;
    }

    samplingTime = (node->getNodeTime()
                        - voipHostData->packetizationInterval);

    data.initiatorPort = voipHostData->initiatorPort;
    data.txTime = samplingTime;

    memcpy(payload, &data, sizeof(VoipData));

    if (DEBUG_VOIP_Err)
    {
        char clockStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(samplingTime, clockStr);
        printf("    Sampling Time (1st octet) = %s\n", clockStr);
        printf("    initiatorPort = %u\n", data.initiatorPort);
        printf("    type = %c\n", data.type);
        TIME_PrintClockInSecond(data.txTime, clockStr);
        printf("    txTime = %s\n", clockStr);
    }

    if (node->appData.appStats)
    {
        if (!voipHostData->stats)
        {
            Address sourceAddr;
            Address destAddr;
            SetIPv4AddressInfo(&sourceAddr, voipHostData->localAddr);
            SetIPv4AddressInfo(&destAddr, voipHostData->remoteAddr);

            voipHostData->sessionId = voipHostData->uniqueId;

            std::string customName;
            if (voipHostData->applicationName->empty())
            {
                if (voipHostData->initiator == FALSE)
                {
                    customName = "VOIP Receiver";
                }
                else
                {
                   customName =  "VOIP Initiator";
                }
            }
            else
            {
                customName = *voipHostData->applicationName;
            }
            if (voipHostData->initiator == FALSE)
            {
                voipHostData->stats =
                    new STAT_AppStatistics(
                                node,
                                "voipReceiver",
                                STAT_Unicast,
                                STAT_AppSenderReceiver,
                                customName.c_str());
            }
            else
            {
                voipHostData->stats =
                    new STAT_AppStatistics(
                                node,
                                "voipInitiator",
                                STAT_Unicast,
                                STAT_AppSenderReceiver,
                                customName.c_str());
            }

            voipHostData->stats->Initialize(
                               node,
                               sourceAddr,
                               destAddr,
                               (STAT_SessionIdType)voipHostData->sessionId,
                               voipHostData->uniqueId);
            voipHostData->stats->setTos(voipHostData->tos);
            voipHostData->stats->SessionStart(node);
        }
    }

#ifdef ADDON_DB
    StatsDBAppEventParam appParam;
    Address destAddr;
    SetIPv4AddressInfo(&destAddr, voipHostData->remoteAddr);
    appParam.m_SessionInitiator = node->nodeId;
    appParam.m_ReceiverId = voipHostData->receiverId;
    appParam.SetAppType("VOIP");
    appParam.SetFragNum(0);

    if (!voipHostData->applicationName->empty())
    {
        appParam.SetAppName(
            voipHostData->applicationName->c_str());
    }
    appParam.SetReceiverAddr(&destAddr);
    appParam.SetPriority(voipHostData->tos);
    appParam.SetSessionId(voipHostData->sessionId);
    appParam.SetMsgSize(payloadSize);
    appParam.m_TotalMsgSize = payloadSize;
    appParam.m_fragEnabled = FALSE;
#endif // ADDON_DB

    Address localAddr;
    Address remoteAddr;
    UInt32 samplingTimeInMilliSec = (UInt32)(samplingTime / MILLI_SECOND);

    localAddr.networkType = NETWORK_IPV4;
    localAddr.interfaceAddr.ipv4 = voipHostData->localAddr;
    remoteAddr.networkType = NETWORK_IPV4;
    remoteAddr.interfaceAddr.ipv4 = voipHostData->remoteAddr;

    Message* sentMsg = RtpHandleDataFromApp(node,
                                            localAddr,
                                            remoteAddr,
                                            payload,
                                            payloadSize,
                                            (UInt8)VOIP_PAYLOAD_TYPE,
                                            samplingTimeInMilliSec,
                                            voipHostData->tos,
                                            voipHostData->initiatorPort
#ifdef ADDON_DB
                                            ,
                                            &appParam
#endif
                                            );


    if (node->appData.appStats && sentMsg)
    {
        if (!voipHostData->stats->IsSessionStarted(STAT_Unicast))
        {
            voipHostData->stats->SessionStart(node);
        }
        voipHostData->stats->AddFragmentSentDataPoints(node,
                                                       payloadSize,
                                                       STAT_Unicast);
        voipHostData->stats->AddMessageSentDataPoints(node,
                                                      sentMsg,
                                                      0,
                                                      payloadSize,
                                                      0,
                                                      STAT_Unicast);
    }

    voipHostData->sessionLastSent = node->getNodeTime();

    if (endSession)
    {
        voipHostData->sessionFinish = node->getNodeTime();
        voipHostData->sessionIsClosed = TRUE;
        if (node->appData.appStats)
        {
            voipHostData->stats->SessionFinish(node);
        }
        voipHostData->status = VOIP_SESSION_CLOSED;
    }
    MEM_free(payload);
}


//--------------------------------------------------------------------------
// NAME         : VoipStartTalking()
// PURPOSE      : This node now start talking.
// RETURN VALUE : None
//--------------------------------------------------------------------------
static
BOOL VoipStartTalking(
    Node* node,
    VoipHostData* voipHostData)
{
    Message* newMsg;
    VoipTimerData* timerData;
    clocktype talkTime;
    clocktype now;
    int numPkt;
    BOOL endSession;
    NodeAddress addr;

    RandomDistribution<clocktype> randomClocktype;

    now = node->getNodeTime();

    // Not sure this is the best way to a set a seed, but we have to use
    // start time or session number to avoid having all this node's
    // calls have the same talkTime.  Maybe VoipHostData should have a seed.
    randomClocktype.setSeed(node->globalSeed, node->nodeId, (int) now);
    randomClocktype.setDistributionExponential(
        (clocktype)(voipHostData->interval * VOIP_MEAN_SPURT_GAP_RATIO));
    talkTime = randomClocktype.getRandomNumber();

    if (talkTime < voipHostData->packetizationInterval)
    {
        talkTime = 2 * voipHostData->packetizationInterval;
    }

    if (DEBUG_VOIP_Err)
    {
        char clockStr[MAX_STRING_LENGTH];
        char aStr[MAX_STRING_LENGTH],aStr1[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(talkTime, aStr);

        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        TIME_PrintClockInSecond(voipHostData->packetizationInterval, aStr1);
        printf("Node %u: Talking time = %s starting from = %s, pp %s\n",
            node->nodeId, aStr, clockStr,aStr1);
    }

    if (now + talkTime < voipHostData->endTime)
    {
        numPkt = (int) (talkTime / voipHostData->packetizationInterval);
        endSession = FALSE;
    }
    else
    {
        numPkt = (int) ((voipHostData->endTime - now - CALL_TERMINATING_DELAY)
                        / voipHostData->packetizationInterval);
        if (DEBUG_VOIP)
        {
            printf("VOIP:Setting endSession to true as talktime+simtime has exceeded voip call time\n");
        }
        endSession = TRUE;
    }

    Message* msg = MESSAGE_Alloc(node, APP_LAYER, APP_RTP,
                                MSG_RTP_SetJitterBuffFirstPacketAsTrue);
    MESSAGE_InfoAlloc(node, msg, sizeof(NodeAddress) + sizeof(unsigned short));
    char* msgInfo = MESSAGE_ReturnInfo(msg);
    memcpy(msgInfo, &(voipHostData->remoteAddr), sizeof(NodeAddress));
    msgInfo += sizeof(NodeAddress);
    memcpy(msgInfo, &(voipHostData->initiatorPort), sizeof(unsigned short));
    MESSAGE_Send(node, msg, (clocktype) 0);

    newMsg = MESSAGE_Alloc(node, 0, 0, MSG_APP_TimerExpired);
    if (voipHostData->initiator)
    {
        MESSAGE_SetLayer(newMsg, APP_LAYER, APP_VOIP_CALL_INITIATOR);
        addr = voipHostData->localAddr;
    }
    else
    {
        MESSAGE_SetLayer(newMsg, APP_LAYER, APP_VOIP_CALL_RECEIVER);
        addr = voipHostData->remoteAddr;
    }


    voipHostData->totalTalkingTime += talkTime;

    MESSAGE_InfoAlloc(node, newMsg, sizeof(VoipTimerData));

    timerData = (VoipTimerData*) MESSAGE_ReturnInfo(newMsg);
    timerData->type = VOIP_SEND_DATA;
    timerData->initiatorPort = voipHostData->initiatorPort;
    timerData->srcAddr = addr;
    timerData->remainingPkt = numPkt;
    timerData->endSession = endSession;

    if (!voipHostData->initiator)
    {
        timerData->srcAddr = voipHostData->remoteAddr;
    }

    MESSAGE_Send(node, newMsg, voipHostData->packetizationInterval);

    if (DEBUG_VOIP) 
    {
        char currentTimeStr[100];
        char talkingTimeStr[100];
        char addrStr[20];

        TIME_PrintClockInSecond(node->getNodeTime(), currentTimeStr);
        TIME_PrintClockInSecond(talkTime, talkingTimeStr);
        IO_ConvertIpAddressToString(voipHostData->remoteAddr, addrStr);

        printf("VOIP: Node %u now talking to node %s\n"
               "      Current time = %s.  Talking duration = %s, total pkts to be sent %d\n",
                node->nodeId, addrStr, currentTimeStr, talkingTimeStr,numPkt);
    }

    return TRUE;
}


//--------------------------------------------------------------------------
// NAME         : VoipTerminateSession()
// PURPOSE      : This node now terminate session.
// RETURN VALUE : None
//--------------------------------------------------------------------------
static
void VoipTerminateSession(
    Node* node,
    VoipHostData* voipHostData)
{
    Message* newMsg;
    VoipTimerData* timerData;
    NodeAddress addr;

    if (DEBUG_VOIP)
    {
        char clockStr[MAX_STRING_LENGTH];
        char addrStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        IO_ConvertIpAddressToString(voipHostData->remoteAddr, addrStr);

        printf("VOIP: Node %u terminating session with node %s at %s\n",
                node->nodeId, addrStr, clockStr);
    }

    newMsg = MESSAGE_Alloc(node, 0, 0, MSG_APP_TimerExpired);

    if (voipHostData->initiator)
    {
        MESSAGE_SetLayer(newMsg, APP_LAYER, APP_VOIP_CALL_INITIATOR);
        addr = voipHostData->localAddr;
    }
    else
    {
        MESSAGE_SetLayer(newMsg, APP_LAYER, APP_VOIP_CALL_RECEIVER);
        addr = voipHostData->remoteAddr;
    }

    MESSAGE_InfoAlloc(node, newMsg, sizeof(VoipTimerData));

    timerData = (VoipTimerData*) MESSAGE_ReturnInfo(newMsg);
    timerData->type = VOIP_TERMINATE_CALL;
    timerData->initiatorPort = voipHostData->initiatorPort;
    timerData->srcAddr = addr;

    if (!voipHostData->initiator)
    {
        timerData->srcAddr = voipHostData->remoteAddr;
    }

    MESSAGE_Send(node, newMsg, VOIP_CALL_TERMINATING_DELAY);

    VOIPSendRtpTerminateSession(node, voipHostData->remoteAddr, voipHostData->initiatorPort);
}


//--------------------------------------------------------------------------
// NAME         : VoipReceiveDataFromRtp()
// PURPOSE      : Handle data from rtp.
// RETURN VALUE : None
//--------------------------------------------------------------------------
void VoipReceiveDataFromRtp(
    Node* node,
    Message* msg,
    Address srcAddr,
    int payloadSize,
    BOOL initiator)
{
    VoipHostData* voipHostData = NULL;
    VoipData data;
    AppMultimedia* multimedia = node->appData.multimedia;
    initiator = initiator; //To Avoid Warnings
    char* payload = (char*)MESSAGE_ReturnPacket(msg);

    memcpy(&data, payload, sizeof(VoipData));

    //if ((multimedia->hostCallingFunction) (node, data.initiatorPort - 1))
    if ((multimedia->hostCallingFunction) (node, data.initiatorPort))
    {
        voipHostData = VoipGetCallInitiatorData(
                        node, data.srcAddr, data.initiatorPort);
    }
    //else if ((multimedia->hostCalledFunction) (node, data.initiatorPort - 1))
    else if ((multimedia->hostCalledFunction) (node, data.initiatorPort))
    {
        voipHostData = VoipGetCallReceiverData(
                                node,
                                srcAddr.interfaceAddr.ipv4,
                                data.initiatorPort);
    }
    else
    {
        ERROR_Assert(FALSE, "Host state should be either receiver or "
            "initiator for voip conversation\n");
    }

    ERROR_Assert(voipHostData, "Receiver structure should initializes "
            "before getting data packet\n");


    if (node->appData.appStats)
    {
        if (!voipHostData->stats)
        {
            Address sourceAddr;
            Address destAddr;
            SetIPv4AddressInfo(&sourceAddr, voipHostData->localAddr);
            SetIPv4AddressInfo(&destAddr, voipHostData->remoteAddr);

            if (voipHostData->initiator == FALSE)
            {
                voipHostData->stats = new STAT_AppStatistics(
                    node,
                    "voipReceiver",
                    STAT_Unicast,
                    STAT_AppSenderReceiver,
                    "VOIP Receiver");
            }
            else
            {
                voipHostData->stats = new STAT_AppStatistics(
                    node,
                    "voipInitiator",
                    STAT_Unicast,
                    STAT_AppSenderReceiver,
                    "VOIP Initiator");
            }
            voipHostData->stats->Initialize(
                node,
                sourceAddr,
                destAddr,
                STAT_AppStatistics::GetSessionId(msg),
                voipHostData->uniqueId);
            voipHostData->stats->SessionStart(node);
        }
    }
#ifdef ADDON_DB
    StatsDb* db = node->partitionData->statsDb;
    if (db != NULL)
    {
        clocktype delay = node->getNodeTime() - data.txTime;

        if (node->appData.appStats)
        {
            if (voipHostData->stats->
                GetMessagesReceived().GetValue(node->getNodeTime()) == 1)
            {
                // Only need this info once
                StatsDBAppEventParam* appParamInfo = NULL;
                appParamInfo = (StatsDBAppEventParam*)
                    MESSAGE_ReturnInfo(msg, INFO_TYPE_AppStatsDbContent);
                if (appParamInfo == NULL)
                {
                    ERROR_ReportWarning("Voip: Unable to find Application "
                        "Layer Stats Info Field\n");
                    return;
                }
                voipHostData->sessionId = appParamInfo->m_SessionId;
                voipHostData->sessionInitiator =
                    appParamInfo->m_SessionInitiator;
            }

            SequenceNumber::Status seqStatus =
                APP_ReportStatsDbReceiveEvent(
                    node,
                    msg,
                    NULL,
                    0,
                    delay,
                    (clocktype)voipHostData->stats->
                        GetJitter().GetValue(node->getNodeTime()),
                    MESSAGE_ReturnPacketSize(msg),
                    (Int32)voipHostData->stats->
                        GetMessagesReceived().GetValue(node->getNodeTime()),
                    APP_MSG_NEW);
        }
    }
#endif // ADDON_DB

    if (DEBUG_VOIP_Err)
    {
        printf("VOIP: Node %d receive data from Rtp\n"
               "      Payload size = %d\n"
               "      port = %u\n"
               "      type = %c\n",
               node->nodeId, payloadSize, data.initiatorPort, data.type);
    }

    voipHostData->sessionLastReceived = node->getNodeTime();

    if (node->appData.appStats)
    {
        voipHostData->stats->AddFragmentReceivedDataPoints(
            node,
            msg,
            MESSAGE_ReturnPacketSize(msg),
            STAT_Unicast);

        voipHostData->stats->AddMessageReceivedDataPoints(
            node,
            msg,
            0,
            MESSAGE_ReturnPacketSize(msg),
            0,
            STAT_Unicast);
    }

    clocktype currRtpTs = node->getNodeTime();
    if (DEBUG_VOIP_Err)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(currRtpTs, clockStr);
        printf("    Receive in VOIP = %s\n", clockStr);
    }

    clocktype oneWayDelay = currRtpTs - data.txTime;
    voipHostData->totalOneWayDelay += oneWayDelay;
    if (voipHostData->maxOneWayDelay < oneWayDelay)
    {
        voipHostData->maxOneWayDelay = oneWayDelay;
    }

    voipHostData->lastInterArrivalInterval = oneWayDelay;

    if (voipHostData->minOneWayDelay == -1.0)
    {
        voipHostData->minOneWayDelay = oneWayDelay;
    }
    else if (voipHostData->minOneWayDelay > oneWayDelay)
    {
        voipHostData->minOneWayDelay = oneWayDelay;
    }

    VOIPGetMeanOpinionSquareScore(node, voipHostData, oneWayDelay);

    if (data.type == 'm')
    {
        // More packets are remaining.
    }
    else if (data.type == 'o')
    {
        if (node->getNodeTime() >= voipHostData->endTime)
        {
            voipHostData->sessionFinish = node->getNodeTime();
            voipHostData->sessionIsClosed = TRUE;
            if (node->appData.appStats)
            {
                voipHostData->stats->SessionFinish(node);
            }
            voipHostData->status = VOIP_SESSION_CLOSED;
            if (DEBUG_VOIP)
            {
                char clockStr[MAX_STRING_LENGTH];
                char addrStr[MAX_STRING_LENGTH];

                TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                IO_ConvertIpAddressToString(voipHostData->remoteAddr, addrStr);

                printf("VOIP: Node %u terminating session with node %s at %s "
               "as Sim time exceeded endtime\n",
                       node->nodeId, addrStr, clockStr);
            }

            VoipTerminateSession(node, voipHostData);
        }
        else
        {
            if (!VoipStartTalking(node, voipHostData))
            {
                voipHostData->sessionFinish = node->getNodeTime();
                voipHostData->sessionIsClosed = TRUE;
                if (node->appData.appStats)
                {
                    voipHostData->stats->SessionFinish(node);
                }
                voipHostData->status = VOIP_SESSION_CLOSED;
                if (DEBUG_VOIP)
                {
                    char clockStr[MAX_STRING_LENGTH];
                    char addrStr[MAX_STRING_LENGTH];

                    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                    IO_ConvertIpAddressToString(voipHostData->remoteAddr, addrStr);

                    printf("VOIP: Node %u terminating session with node %s at %s "
                           "as no packets to send\n",
                           node->nodeId, addrStr, clockStr);
                }

                VoipTerminateSession(node, voipHostData);
            }
        }
    }
    else if (data.type == 'e')
    {
        if (DEBUG_VOIP)
        {
            char clockStr[MAX_STRING_LENGTH];
            char addrStr[MAX_STRING_LENGTH];

            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            IO_ConvertIpAddressToString(voipHostData->remoteAddr, addrStr);

            printf("VOIP: Node %u terminating session with node %s at %s\n",
                    node->nodeId, addrStr, clockStr);
        }

        voipHostData->sessionFinish = node->getNodeTime();
        voipHostData->sessionIsClosed = TRUE;
        if (node->appData.appStats)
        {
            voipHostData->stats->SessionFinish(node);
        }
        voipHostData->status = VOIP_SESSION_CLOSED;

        VOIPSendRtpTerminateSession(
                node,
                voipHostData->remoteAddr,
                voipHostData->initiatorPort);
    }
    else
    {
        ERROR_Assert(FALSE, "Invalid voip data\n");
    }
    MESSAGE_Free(node, msg);
}


//-------------------------------------------------------------------------
// NAME         : VOIPGetMeanOpinionSquareScore()
// PURPOSE      : Get Mean Opinion Square (MOS) Score
// RETURNS      : void
// PARAMETERS   :
//         node : the Node
//        voip  : A Pointer of VoipHostData
// oneWayDelay  : One Way delay
//-------------------------------------------------------------------------
void VOIPGetMeanOpinionSquareScore(Node *node,
                                  VoipHostData* voipHostData,
                                  clocktype oneWayDelay)
{
    //Calculate Transmission rating Factor R Factor
    //  Using the formula R = 93.2 - Id - Ief
    //  Where Id is Delay Impairment Factor, caused by delay in network.
    //        Ief is Equipment impairment factor, caused by low bit rate
    //               coders,the effect of frame loss on the coder.
    //      Id = 0.024d + 0.11(d - 177.3) H(d - 177.3)
    //   Where d is one way delay (coding + network + de-jitter delay)(ms)
    //  And H(x) = 0 for x<0
    //      H(x) = 1 for x>=0
    double functionOfHx = 0.0;
    double functionOfHx1 = 0.0;

    double delayImpairmentFactor;
    double equipmentImpairmentFactor;
    double transmissionRatingFactorR;

    AppMultimedia* multimedia = node->appData.multimedia;
    double totalLossProbablity = multimedia->totalLossProbablity;

    double scoreMOS = 0.0;
    double oneWayDelayinMS;

    oneWayDelayinMS = (double)(oneWayDelay / MILLI_SECOND);

    if ((oneWayDelayinMS - 177.3) < 0.0)
    {
        functionOfHx = 0.0;
    }
    else if ((oneWayDelayinMS - 177.3) >= 0.0)
    {
        functionOfHx = 1.0;
    }

    delayImpairmentFactor = (0.024 * oneWayDelayinMS)
                    + (0.11 * (oneWayDelayinMS - 177.3) * functionOfHx);

    // Ief = 30 ln(1+15e) H(0.04-e)+19ln(1+70e) H(e-0.04)
    if ((0.04 - totalLossProbablity) < 0)
    {
        functionOfHx = 0.0;
    }
    else if ((0.04 - totalLossProbablity) >= 0)
    {
        functionOfHx = 1.0;
    }

    if ((totalLossProbablity - 0.04) < 0)
    {
        functionOfHx1 = 0.0;
    }
    else if ((totalLossProbablity - 0.04) >= 0)
    {
        functionOfHx1 = 1.0;
    }
    equipmentImpairmentFactor
         = ((30.0 * log((double)(1.0 + 15.0 * totalLossProbablity))
                                    * functionOfHx)
           + (19.0 * log((double)(1.0 + 70.0 * totalLossProbablity))
                                    * functionOfHx1));

    transmissionRatingFactorR = 93.2 - delayImpairmentFactor
                                     - equipmentImpairmentFactor;

    if (transmissionRatingFactorR < 0.0)
    {
        scoreMOS = 1.0;
    }
    else if ((transmissionRatingFactorR >= 0.0)
                && (transmissionRatingFactorR <= 100.0))
    {
        scoreMOS = 1.0 + 0.035 * transmissionRatingFactorR
                        + 7.0 * transmissionRatingFactorR
                        * (transmissionRatingFactorR - 60.0)
                        * (100.0 - transmissionRatingFactorR) * 1.0e-6;
    }
    else if (transmissionRatingFactorR > 100.0)
    {
        scoreMOS = 4.5;
    }
    voipHostData->scoreMOS += scoreMOS;
    if (voipHostData->maxMOS < scoreMOS)
    {
        voipHostData->maxMOS = scoreMOS;
    }
    if (voipHostData->minMOS > scoreMOS)
    {
        voipHostData->minMOS = scoreMOS;
    }
}


//--------------------------------------------------------------------------
// NAME         : VoipStartTransmission()
// PURPOSE      : H.323 has indicated that chanell has been established. So
//                start sending data packet.
// RETURN VALUE : None
//--------------------------------------------------------------------------
void VoipStartTransmission(Node* node, VoipHostData* voip)
{
    if (!voip)
    {
        // connection failed
        return;
    }

    VOIPSendRtpInitiateNewSession(
            node,
            voip->localAddr,
            voip->sourceAliasAddr,
            voip->remoteAddr,
            voip->packetizationInterval,
            voip->initiatorPort,
            voip->initiator,
            (void*) &VoipReceiveDataFromRtp);

    if (DEBUG_VOIP)
    {
        char clockStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        printf("VOIP: Node %u got StartTransmission from H.323 at %s\n",
                node->nodeId, clockStr);
    }

    if (node->getNodeTime() >= voip->endTime)
    {
        if (DEBUG_VOIP_Err)
        {
            printf("VOIP: Terminating session as current sim time has exceeded the call end time\n");
        }

        VoipTerminateSession(node, voip);
    }
    else
    {
        voip->status = VOIP_SESSION_ESTABLISHED;
        voip->sessionEstablished = node->getNodeTime();

        if (!VoipStartTalking(node, voip))
        {
            ERROR_Assert(FALSE, "");
        }
    }
}


//--------------------------------------------------------------------------
// NAME         : VoipInitiatorCreateNewSession()
// PURPOSE      : create a new voip initiator data structure, place it
//                at the beginning of the application list.
// RETURN VALUE : The pointer to the created data structure,
//                NULL if no data structure allocated.
//--------------------------------------------------------------------------
static
VoipHostData* VoipInitiatorCreateNewSession(
    Node* node,
    NodeAddress localAddr,
    NodeAddress remoteAddr,
    unsigned short callSignalLocalPort,
    unsigned short srcPort,
    char* sourceAliasAddr,
    char* destAliasAddr,
    char* appName,
    clocktype interval,
    clocktype startTime,
    clocktype endTime,
    clocktype packetizationInterval,
    unsigned int bitRatePerSecond,
    TosType tos)
{
    VoipHostData* voipInitiator;

    voipInitiator = (VoipHostData*) MEM_malloc(sizeof(VoipHostData));
    memset(voipInitiator, 0, sizeof(VoipHostData));

    voipInitiator->localAddr = localAddr;
    voipInitiator->remoteAddr = remoteAddr;

    strcpy(voipInitiator->sourceAliasAddr, sourceAliasAddr);
    strcpy(voipInitiator->destAliasAddr, destAliasAddr);

    voipInitiator->initiator = TRUE;
    voipInitiator->interval = interval;
    voipInitiator->sessionInitiate = startTime;
    voipInitiator->endTime = endTime;
    voipInitiator->sessionIsClosed = FALSE;

    voipInitiator->initiatorPort = srcPort;

    RANDOM_SetSeed(voipInitiator->seed,
                   node->globalSeed,
                   node->nodeId,
                   APP_VOIP_CALL_INITIATOR,
                   voipInitiator->initiatorPort);

    voipInitiator->callSignalAddr = localAddr;
    voipInitiator->callSignalPort = callSignalLocalPort;

    voipInitiator->status = VOIP_SESSION_NOT_ESTABLISHED;

    voipInitiator->scoreMOS = 0.0;
    voipInitiator->maxMOS = 0.0;
    voipInitiator->minMOS = VOIP_MOS_MAX;
    voipInitiator->averageMOS = 0.0;

    voipInitiator->totalOneWayDelay = 0;
    voipInitiator->maxOneWayDelay = 0;
    voipInitiator->minOneWayDelay = VOIP_MIN_ONEWAY_DELAY;
    voipInitiator->averageOneWayDelay = 0;
    voipInitiator->lastInterArrivalInterval = 0;

    // voip set Packetization Interval & bit rate
    voipInitiator->bitRate = bitRatePerSecond;
    voipInitiator->packetizationInterval = packetizationInterval;

    voipInitiator->tos = tos;
    voipInitiator->stats = NULL;
    if (appName)
    {
        voipInitiator->applicationName = new std::string(appName);
    }
    else
    {
        voipInitiator->applicationName = new std::string();
    }

    voipInitiator->uniqueId = node->appData.uniqueId++;
    voipInitiator->sessionId = -1;
#ifdef ADDON_DB
    Address remoteAddress;
    SetIPv4AddressInfo(&remoteAddress, remoteAddr);

    if (Address_IsAnyAddress(&remoteAddress) ||
        Address_IsMulticastAddress(&remoteAddress))
    {
        voipInitiator->receiverId = 0;
    }
    else
    {
        voipInitiator->receiverId =
            MAPPING_GetNodeIdFromInterfaceAddress(node, remoteAddress);
    }
#endif // ADDON_DB

    if (DEBUG_VOIP_Err)
    {
        char clockStr[MAX_STRING_LENGTH];
        char localAddr[MAX_STRING_LENGTH];
        char remoteAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(voipInitiator->localAddr, localAddr);
        IO_ConvertIpAddressToString(voipInitiator->remoteAddr, remoteAddr);

        printf("VOIP Initiator: Node %u created new voip "
                "initiator structure\n",
                node->nodeId);
        printf("    localAddr = %s\n", localAddr);
        printf("    remoteAddr = %s\n", remoteAddr);
        ctoa(voipInitiator->interval, clockStr);
        printf("    interval = %s\n", clockStr);
        ctoa(voipInitiator->sessionInitiate, clockStr);
        printf("    sessionInitiate = %s\n", clockStr);
        printf("    initiatorPort = %d\n", voipInitiator->initiatorPort);

        TIME_PrintClockInSecond(voipInitiator->packetizationInterval,
                                clockStr);
        printf("    packetizationInterval = %s\n", clockStr);
    }
    // dynamic address
    voipInitiator->destNodeId = ANY_NODEID;
    voipInitiator->clientInterfaceIndex = ANY_INTERFACE;
    voipInitiator->destInterfaceIndex = ANY_INTERFACE;
    voipInitiator->proxyNodeId = ANY_NODEID;
    voipInitiator->proxyInterfaceIndex = ANY_INTERFACE;

    APP_RegisterNewApp(node, APP_VOIP_CALL_INITIATOR, voipInitiator);
    if (DEBUG_VOIP)
    {
        printf("VOIP:Created voip initiator and registered with APP "
            "layer the traffic generator\n");
    }

    return voipInitiator;
}


//--------------------------------------------------------------------------
// NAME         : VoipCallInitiatorHandleEvent()
// PURPOSE      : Handle event on caller side.
// ASSUMPTION   : None.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void VoipCallInitiatorHandleEvent(Node* node, Message* msg)
{
    char error[MAX_STRING_LENGTH];
    VoipHostData* callInitiatorPtr;

    switch (msg->eventType)
    {
        case MSG_APP_TimerExpired:
        {
            VoipTimerData* timerData;

            timerData = (VoipTimerData*) MESSAGE_ReturnInfo(msg);

            callInitiatorPtr = VoipGetCallInitiatorData(
                node, timerData->srcAddr, timerData->initiatorPort);

            if (!callInitiatorPtr)
            {
                sprintf(error, "VOIP Call Initiator: Node %u cannot "
                        "find voip call initiator\n",
                        node->nodeId);

                ERROR_ReportError(error);
            }

            switch (timerData->type)
            {
                case VOIP_INITIATE_CALL:
                {
                    AppMultimedia* multimedia = node->appData.multimedia;
                    if (DEBUG_VOIP)
                    {
                        char clockStr[MAX_STRING_LENGTH];
                        char addrStr[MAX_STRING_LENGTH];

                        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                        IO_ConvertIpAddressToString(
                                    callInitiatorPtr->remoteAddr,
                                    addrStr);

                        printf("VOIP: Node %u request to establish "
                               "channel with node %s at %s\n",
                                node->nodeId, addrStr, clockStr);
                    }                   

                    if (multimedia)
                    {
                        (multimedia->initiateFunction)
                                        (node, (void*) callInitiatorPtr);
                    }
                    break;
                }
                case VOIP_SEND_DATA:
                {
                    BOOL overNow;
                    BOOL endSession;

                    timerData->remainingPkt -= 1;

                    if (timerData->remainingPkt > 0)
                    {
                        overNow = FALSE;
                        endSession = FALSE;
                    }
                    else
                    {
                        overNow = TRUE;
                        endSession = timerData->endSession;
                    }

                    if (timerData->remainingPkt >= 0)
                    {
                        VoipSendDataToRtp(
                            node,
                            callInitiatorPtr,
                            overNow,
                            endSession);

                        if (DEBUG_VOIP)
                        {
                            char clockStr[MAX_STRING_LENGTH];
                            char addrStr[MAX_STRING_LENGTH];

                            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
                            IO_ConvertIpAddressToString(
                                        callInitiatorPtr->remoteAddr,
                                        addrStr);

                            printf("VOIP: Node %u request to send "
                                   "packet node %s at %s\n",
                                    node->nodeId, addrStr, clockStr);
                        }   
                    }

                    if (timerData->remainingPkt > 0)
                    {
                        Message* newMsg;

                        newMsg = MESSAGE_Duplicate(node, msg);

                        MESSAGE_Send(
                            node,
                            newMsg,
                            callInitiatorPtr->packetizationInterval);
                    }

                    if (endSession)
                    {
                        if (DEBUG_VOIP_Err)
                        {
                            printf("VOIP: Terminating session as current simu"
                            "time has exceeded the call end time\n");
                        }

                        VoipTerminateSession(node, callInitiatorPtr);
                    }

                    break;
                }

                case VOIP_TERMINATE_CALL:
                {
                    AppMultimedia* multimedia = node->appData.multimedia;

                    if (multimedia)
                    {
                        (multimedia->terminateFunction)
                                        (node, (void*) callInitiatorPtr);
                    }
                    break;
                }

                default:
                {
                    sprintf(error, "Node %u (VOIP Call Initiator): "
                            "VOIP unknown event\n",
                            node->nodeId);
                    ERROR_Assert(FALSE, error);
                }
            }

            break;
        }
        default:
        {
            sprintf(error, "Node %u (VOIP Call Initiator): "
                    "Receive unknown event\n",
                    node->nodeId);
#ifndef EXATA
            ERROR_Assert(FALSE, error);
#endif
        }
    }

    MESSAGE_Free(node, msg);
}


//--------------------------------------------------------------------------
// NAME         : VoipCallInitiatorInit()
// PURPOSE      : Initialize a VOIP caller session.
// ASSUMPTION   : None.
// RETURN VALUE : Source port number allocated to this application.
//--------------------------------------------------------------------------
unsigned short VoipCallInitiatorInit(
    Node* node,
    NodeAddress srcAddr,
    NodeAddress destAddr,
    unsigned short callSignalPort,
    unsigned short srcPort,
    char* sourceAliasAddr,
    char* destAliasAddr,
    char* appName,
    clocktype interval,
    clocktype startTime,
    clocktype endTime,
    clocktype packetizationInterval,
    unsigned int bitRatePerSecond,
    unsigned tos)
{
    VoipHostData* voipInitiator;
    Message* newMsg;
    VoipTimerData* timerData;
    char error[MAX_STRING_LENGTH];
    Float64 packetizationIntervalInSeconds = 0.0;
    Float64 bitsPerInterval = 0.0;
    Float64 packetSize = 0.0;

    /* Make sure interval is valid. */
    if (interval <= 0)
    {
        sprintf(error, "VOIP Initiator node %u: Average talking time should"
            " be > 0.\n", node->nodeId);
        ERROR_ReportError(error);
    }

    /* Make sure start time is valid. */
    if (startTime < 0)
    {
        sprintf(error, "VOIP Initiator node %u: Start time should be >= 0\n",
                node->nodeId);
        ERROR_ReportError(error);
    }

    /* Check to make sure the end time is a correct value. */
    if (startTime >= endTime)
    {
        sprintf(error, "Initiator node %u: Start time should less than "
            "end time \n", node->nodeId);

        ERROR_ReportError(error);
    }

    if (tos > 255)
    {
        sprintf(error, "VOIP: Node %d should have tos value "
            "within the range 0 to 255.\n",
            node->nodeId);
        ERROR_ReportError(error);
    }

    packetizationIntervalInSeconds = (Float64)packetizationInterval / SECOND;

    // Number of bits sent during each packetisation interval.
    bitsPerInterval = packetizationIntervalInSeconds * bitRatePerSecond;

    /* Calculate the total packet size at network layer taking into
       consideration RTP, UDP and IP headers. */

    packetSize = bitsPerInterval / 8 +
                 sizeof(RtpPacket) +
                 sizeof(TransportUdpHeader) +
                 sizeof(IpHeaderType);

    // Total packet size should not be greter than IP_MAXPACKET.
    if (ceil(packetSize) > (Float64)IP_MAXPACKET)
    {
        Float64 maxAllowedPacketSize = 0.0;
        Int64 maxAllowedPacketizationInterval = 0;
        NodeAddress destNodeId = 0;

        maxAllowedPacketSize = IP_MAXPACKET -
                               (sizeof(RtpPacket) +
                                sizeof(TransportUdpHeader) +
                                sizeof(IpHeaderType));

        maxAllowedPacketizationInterval = 
                (Int64)(((maxAllowedPacketSize * 8) /
                    bitRatePerSecond) * (Float64)SECOND);

        destNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node, destAddr);

        ERROR_ReportErrorArgs("The packet size for voip application configured"
                              " between node %u and node %u has exceeded the "
                              "maximum packet size supported at network layer "
                              "(65535 bytes). Please configure the "
                              "packetization interval for this application "
                              "less than %" TYPES_64BITFMT "d nanoseconds.\n ",
                              node->nodeId,
                              destNodeId,
                              maxAllowedPacketizationInterval);
    }

    voipInitiator = VoipInitiatorCreateNewSession(
                        node,
                        srcAddr,
                        destAddr,
                        callSignalPort,
                        srcPort,
                        sourceAliasAddr,
                        destAliasAddr,
                        appName,
                        interval,
                        startTime,
                        endTime,
                        packetizationInterval,
                        bitRatePerSecond,
                        (TosType) tos);


    newMsg = MESSAGE_Alloc(node, APP_LAYER, APP_VOIP_CALL_INITIATOR,
                    MSG_APP_TimerExpired);

    MESSAGE_InfoAlloc(node, newMsg, sizeof(VoipTimerData));

    timerData = (VoipTimerData*) MESSAGE_ReturnInfo(newMsg);
    timerData->type = VOIP_INITIATE_CALL;
    timerData->initiatorPort = voipInitiator->initiatorPort;
    timerData->srcAddr = voipInitiator->localAddr;

    MESSAGE_Send(node, newMsg, startTime);
    AppVoipClientAddAddressInformation(node, voipInitiator, voipInitiator->remoteAddr);
    AppMultimedia* multimedia = node->appData.multimedia;
    if (multimedia && multimedia->sigType == SIG_TYPE_SIP)
    {
        SipData* sip;
        sip = (SipData*) multimedia->sigPtr;
        if (sip && sip->SipGetCallModel() == SipProxyRouted)
        {
            AppVoipClientAddAddressInformation(node,
                                               voipInitiator,
                                               sip->SipGetProxyIp(),
                                               TRUE);
        }
    }
    return voipInitiator->initiatorPort;
}


//--------------------------------------------------------------------------
// NAME         : VoipAddNewCallToList()
// PURPOSE      : Add new entry in a list which will be accepted.
// ASSUMPTION   : None.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void VoipAddNewCallToList(
    Node* node,
    NodeAddress srcAddr,
    unsigned short initiatorPort,
    clocktype interval,
    clocktype startTime,
    clocktype endTime,
    clocktype packetizationInterval)
{
    VoipCallAcceptanceList* callAcceptanceList = NULL;
    VoipCallAcceptanceListEntry* newListItem = NULL;

    callAcceptanceList = (VoipCallAcceptanceList*)
                                node->appData.voipCallReceiptList;
    if (DEBUG_VOIP_Err)
    {
        printf("VOIP:Adding a call to acceptance list\n");
    }

    if (!callAcceptanceList)
    {
        callAcceptanceList = (VoipCallAcceptanceList*)
                MEM_malloc(sizeof(VoipCallAcceptanceList));

        callAcceptanceList->head = NULL;
        callAcceptanceList->rear = NULL;
        callAcceptanceList->count = 0;

        node->appData.voipCallReceiptList = (void*) callAcceptanceList;
    }

    newListItem = (VoipCallAcceptanceListEntry*)
            MEM_malloc(sizeof(VoipCallAcceptanceListEntry));
    memset(newListItem, 0, sizeof(VoipCallAcceptanceListEntry));

    newListItem->srcAddr = srcAddr;
    newListItem->initiatorPort = initiatorPort;
    newListItem->interval = interval;
    newListItem->startTime = startTime;
    newListItem->endTime = endTime;
    newListItem->packetizationInterval = packetizationInterval;

    newListItem->next = NULL;

    if (callAcceptanceList->rear)
    {
        callAcceptanceList->rear->next = newListItem;
        callAcceptanceList->rear = newListItem;
    }
    else
    {
        callAcceptanceList->head = newListItem;
        callAcceptanceList->rear = callAcceptanceList->head;
    }
}


//--------------------------------------------------------------------------
// NAME         : VoipCheckCallList()
// PURPOSE      : Check if this entry is listed. This function will be used
//                to determine whether the call will be accepted or not.
// ASSUMPTION   : None.
// RETURN VALUE : pointer to entry found, NULL otherwise.
//--------------------------------------------------------------------------
static
VoipCallAcceptanceListEntry* VoipCheckCallList(
    Node* node,
    NodeAddress srcAddr,
    unsigned short initiatorPort)
{
    VoipCallAcceptanceList* callAcceptanceList = NULL;
    VoipCallAcceptanceListEntry* currentListItem = NULL;

    callAcceptanceList = (VoipCallAcceptanceList*)
                                node->appData.voipCallReceiptList;

    if (!callAcceptanceList)
    {
        if (DEBUG_VOIP_Err)
        {
            printf("VOIP:call acceptance list is empty so returning"
                " and not creating the voice reciever data instance\n");
        }

        return NULL;
    }

    currentListItem = callAcceptanceList->head;

    while (currentListItem)
    {
        if (currentListItem->srcAddr == srcAddr
            && currentListItem->initiatorPort == initiatorPort)
        {
            return currentListItem;
        }

        currentListItem = currentListItem->next;
    }

    return NULL;
}


//--------------------------------------------------------------------------
// NAME         : VoipCallReceiverHandleEvent()
// PURPOSE      : Handle event on receiver side.
// ASSUMPTION   : None.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void VoipCallReceiverHandleEvent(Node* node, Message* msg)
{
    char error[MAX_STRING_LENGTH];
    VoipHostData* callReceiverPtr;

    switch (msg->eventType)
    {
        case MSG_APP_TimerExpired:
        {
            VoipTimerData* timerData;

            timerData = (VoipTimerData*) MESSAGE_ReturnInfo(msg);

            callReceiverPtr = VoipGetCallReceiverData(node,
                                        timerData->srcAddr,
                                        timerData->initiatorPort);

            if (!callReceiverPtr)
            {
                sprintf(error, "VOIP Call Receiver: Node %u cannot "
                        "find voip call receiver\n",
                        node->nodeId);

                ERROR_ReportError(error);
            }

            switch (timerData->type)
            {
                case VOIP_SEND_DATA:
                {
                    BOOL overNow;
                    BOOL endSession;

                    timerData->remainingPkt -= 1;

                    if (timerData->remainingPkt > 0)
                    {
                        overNow = FALSE;
                        endSession = FALSE;
                    }
                    else
                    {
                        overNow = TRUE;
                        endSession = timerData->endSession;
                    }

                    if (timerData->remainingPkt >= 0)
                    {
                         VoipSendDataToRtp(
                            node,
                            callReceiverPtr,
                            overNow,
                            endSession);
                    }

                    if (timerData->remainingPkt > 0)
                    {
                        Message* newMsg;

                        newMsg = MESSAGE_Duplicate(node, msg);

                        MESSAGE_Send(
                            node,
                            newMsg,
                            callReceiverPtr->packetizationInterval);
                    }

                    if (endSession)
                    {
                        if (DEBUG_VOIP_Err)
                        {
                            printf("VOIP: Terminating session as current sim"
                            "time has exceeded the call end time\n");
                        }
                        VoipTerminateSession(node, callReceiverPtr);
                    }

                    break;
                }

                case VOIP_TERMINATE_CALL:
                {
                    AppMultimedia* multimedia = node->appData.multimedia;

                    if (multimedia)
                    {
                        (multimedia->terminateFunction)
                                        (node, (void*) callReceiverPtr);
                    }
                    break;
                }

                default:
                {
                    sprintf(error, "Node %u (VOIP Call Receiver): "
                            "VOIP unknown event\n",
                            node->nodeId);
                    ERROR_Assert(FALSE, error);
                }
            }

            break;
        }
        default:
        {
            sprintf(error, "Node %u (VOIP Call Receiver): "
                    "Receive unknown event\n",
                    node->nodeId);
#ifndef EXATA
            ERROR_Assert(FALSE, error);
#endif
        }
    }

    MESSAGE_Free(node, msg);
}


//--------------------------------------------------------------------------
// NAME         : VoipReceiverCreateNewSession()
// PURPOSE      : create a new voip receiver data structure, place it
//                at the beginning of the application list.
// RETURN VALUE : The pointer to the created data structure,
//                NULL if no data structure allocated.
//--------------------------------------------------------------------------
static
VoipHostData* VoipReceiverCreateNewSession(
    Node* node,
    NodeAddress localAddr,
    NodeAddress remoteAddr,
    unsigned short remotePort,
    char* sourceAliasAddr,
    char* appName,
    unsigned short callSignalRemotePort,
    int connectionId,
    clocktype packetizationInterval,
    unsigned int bitRatePerSecond,
    TosType tos)
{
    VoipHostData* voipReceiver;

    voipReceiver = (VoipHostData*) MEM_malloc(sizeof(VoipHostData));
    memset(voipReceiver, 0, sizeof(VoipHostData));

    voipReceiver->localAddr = localAddr;
    voipReceiver->remoteAddr = remoteAddr;
    voipReceiver->initiator = FALSE;
    voipReceiver->initiatorPort = remotePort;

    RANDOM_SetSeed(voipReceiver->seed,
                   node->globalSeed,
                   node->nodeId,
                   APP_VOIP_CALL_RECEIVER,
                   remoteAddr);

    strcpy(voipReceiver->sourceAliasAddr, sourceAliasAddr);

    voipReceiver->callSignalAddr = remoteAddr;
    voipReceiver->callSignalPort = callSignalRemotePort;

    voipReceiver->connectionId = connectionId;

    voipReceiver->tos = tos;

    // voip set Packetization Interval & bit rate
    voipReceiver->bitRate = bitRatePerSecond;
    voipReceiver->packetizationInterval = packetizationInterval;

    voipReceiver->scoreMOS = 0.0;
    voipReceiver->maxMOS = 0.0;
    voipReceiver->minMOS = VOIP_MOS_MAX;
    voipReceiver->averageMOS = 0.0;

    voipReceiver->totalOneWayDelay = 0;
    voipReceiver->maxOneWayDelay = 0;
    voipReceiver->minOneWayDelay = VOIP_MIN_ONEWAY_DELAY;
    voipReceiver->averageOneWayDelay = 0;
    voipReceiver->lastInterArrivalInterval = 0;

    voipReceiver->stats = NULL;
    if (appName)
    {
        voipReceiver->applicationName = new std::string(appName);
    }
    else
    {
        voipReceiver->applicationName = new std::string();
    }

    voipReceiver->uniqueId = node->appData.uniqueId++;
    voipReceiver->sessionId = -1;
#ifdef ADDON_DB
    Address remoteAddress;
    SetIPv4AddressInfo(&remoteAddress, remoteAddr);

    if (Address_IsAnyAddress(&remoteAddress) ||
        Address_IsMulticastAddress(&remoteAddress))
    {
        voipReceiver->receiverId = 0;
    }
    else
    {
        voipReceiver->receiverId =
            MAPPING_GetNodeIdFromInterfaceAddress(node, remoteAddress);
    }
#endif // ADDON_DB

    if (DEBUG_VOIP_Err)
    {
        char localAddr[MAX_STRING_LENGTH];
        char remoteAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(voipReceiver->localAddr, localAddr);
        IO_ConvertIpAddressToString(voipReceiver->remoteAddr, remoteAddr);

        printf("VOIP Receiver: Node %u created new voip "
                "receiver structure\n",
                node->nodeId);
        printf("    localAddr = %s\n", localAddr);
        printf("    remoteAddr = %s\n", remoteAddr);
    }

    APP_RegisterNewApp(node, APP_VOIP_CALL_RECEIVER, voipReceiver);
    if (DEBUG_VOIP_Err)
    {
        printf("VOIP:Created voip reciever and registered with APP "
            "layer the traffic generator\n");
    }

    return voipReceiver;
}


//--------------------------------------------------------------------------
// NAME         : VoipCallReceiverInit()
// PURPOSE      : Initialize a VOIP call receiver session.
// ASSUMPTION   : None.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void VoipCallReceiverInit(
    Node* node,
    NodeAddress localAddr,
    NodeAddress remoteAddr,
    unsigned short remotePort,
    char* sourceAliasAddr,
    char* appName,
    unsigned short callSignalPort,
    int connectionId,
    clocktype packetizationInterval,
    unsigned int bitRatePerSecond,
    unsigned tos)
{
    VoipHostData* voipReceiver = NULL;
    VoipCallAcceptanceListEntry* callListEntry = NULL;
    char error[MAX_STRING_LENGTH];

    if (tos > 255)
    {
        sprintf(error, "VOIP: Node %d should have tos value "
            "within the range 0 to 255.\n",
            node->nodeId);
        ERROR_ReportError(error);
    }

    callListEntry = VoipCheckCallList(node, remoteAddr, remotePort);

    if (callListEntry)
    {
        voipReceiver = VoipReceiverCreateNewSession(
                            node,
                            localAddr,
                            remoteAddr,
                            remotePort,
                            sourceAliasAddr,
                            appName,
                            callSignalPort,
                            connectionId,
                            packetizationInterval,
                            bitRatePerSecond,
                            (TosType) tos);

        voipReceiver->interval = callListEntry->interval;
        voipReceiver->endTime = callListEntry->endTime;

        voipReceiver->packetizationInterval =
                callListEntry->packetizationInterval;
        AppVoipClientAddAddressInformation(node, voipReceiver, voipReceiver->remoteAddr);

    }
}


//--------------------------------------------------------------------------
// NAME         : VoipAskForConnectionEstablished()
// PURPOSE      : The function set a self timer. This is the time after when
//                receiver acknowledge the call. On expiry of this timer
//                receiver should send Connect message
// ASSUMPTION   : None.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void VoipAskForConnectionEstablished(
    Node* node,
    int connectionId)
{
    Message* connectTimerMsg;
    VoipHostData* voipReceiver = NULL;

    AppMultimedia* multimedia = node->appData.multimedia;

    voipReceiver = VoipCallReceiverGetDataForConnectionId(
                                        node,
                                        connectionId);

    if (voipReceiver) //prob
    {
        clocktype delay;

        connectTimerMsg = MESSAGE_Alloc(node,
                            APP_LAYER,
                            APP_H323,
                            MSG_APP_H323_Connect_Timer);

        // connection ID is passed through info field
        MESSAGE_InfoAlloc(node, connectTimerMsg, sizeof(int));
        *((int*)MESSAGE_ReturnInfo(connectTimerMsg)) = connectionId;

        delay = (clocktype) (multimedia->connectionDelay *
                RANDOM_erand(voipReceiver->seed));

        MESSAGE_Send(node, connectTimerMsg, delay);
    }
}


//--------------------------------------------------------------------------
// NAME         : VoipPrintStats()
// PURPOSE      : Print statistics for a voip session.
// ASSUMPTION   : None.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
static
void VoipPrintStats(Node* node, VoipHostData* voipHostData)
{
    char clockStr[MAX_STRING_LENGTH];
    char initiateStr[MAX_STRING_LENGTH];
    char sessionStatusStr[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char remoteAddr[MAX_STRING_LENGTH];
    char appType[MAX_STRING_LENGTH];
    char talkTimeStr[MAX_STRING_LENGTH];
    UInt32 numPktsRecvd = 0;

    if (voipHostData->initiator)
    {
        sprintf(appType, "VOIP Initiator");
    }
    else
    {
        sprintf(appType, "VOIP Receiver");
    }

    TIME_PrintClockInSecond(voipHostData->sessionInitiate, initiateStr);
    TIME_PrintClockInSecond(voipHostData->totalTalkingTime, talkTimeStr);

    if (voipHostData->sessionIsClosed == FALSE)
    {
        voipHostData->sessionFinish = node->getNodeTime();

        if (voipHostData->stats)
        {
            voipHostData->stats->SessionFinish(node);
        }
    }

    if (voipHostData->status == VOIP_SESSION_CLOSED)
    {
        sprintf(sessionStatusStr, "Closed");
    }
    else if (voipHostData->status == VOIP_SESSION_ESTABLISHED)
    {
        sprintf(sessionStatusStr, "Not Closed");
    }
    else
    {
        sprintf(sessionStatusStr, "Not Established");
    }

    IO_ConvertIpAddressToString(voipHostData->remoteAddr, remoteAddr);

    if (voipHostData->initiator)
    {
        sprintf(buf, "Receiver Address = %s", remoteAddr);
    }
    else
    {
        sprintf(buf, "Initiator Address = %s", remoteAddr);
    }


    IO_PrintStat(
        node,
        "Application",
        appType,
        ANY_DEST,
        voipHostData->initiatorPort,
        buf);


    sprintf(buf, "Session Status = %s", sessionStatusStr);
    IO_PrintStat(
        node,
        "Application",
        appType,
        ANY_DEST,
        voipHostData->initiatorPort,
        buf);

    if (voipHostData->initiator)
    {
        sprintf(buf, "Session Initiated At (s) = %s", initiateStr);
        IO_PrintStat(
            node,
            "Application",
            appType,
            ANY_DEST,
            voipHostData->initiatorPort,
            buf);
    }

    if (voipHostData->status == VOIP_SESSION_NOT_ESTABLISHED)
    {
        // We don't need to print rest.
        return;
    }

    if (voipHostData->stats)
    {
        voipHostData->stats->Print(node,
                                   "Application",
                                   appType,
                                   ANY_DEST,
                                   voipHostData->initiatorPort);
    }

    sprintf(buf, "Talking Time = %s", talkTimeStr);
    IO_PrintStat(
        node,
        "Application",
        appType,
        ANY_DEST,
        voipHostData->initiatorPort,
        buf);

    TIME_PrintClockInSecond(voipHostData->maxOneWayDelay, clockStr);
    sprintf(buf, "Maximum One Way Delay (s) = %s", clockStr);

    IO_PrintStat(
        node,
        "Application",
        appType,
        ANY_DEST,
        voipHostData->initiatorPort,
        buf);

    if (voipHostData->minOneWayDelay == VOIP_MIN_ONEWAY_DELAY)
    {
        voipHostData->minOneWayDelay = voipHostData->maxOneWayDelay;
    }

    TIME_PrintClockInSecond(voipHostData->minOneWayDelay, clockStr);
    sprintf(buf, "Minimum One Way Delay (s) = %s", clockStr);
    IO_PrintStat(
        node,
        "Application",
        appType,
        ANY_DEST,
        voipHostData->initiatorPort,
        buf);

    if (voipHostData->stats)
    {
        numPktsRecvd = (UInt32)voipHostData->stats->
                        GetMessagesReceived().GetValue(node->getNodeTime());
    }
    if (numPktsRecvd == 0)
    {
        voipHostData->averageOneWayDelay = 0;
    }
    else
    {
        voipHostData->averageOneWayDelay = voipHostData->totalOneWayDelay
                                              / numPktsRecvd;
    }
    TIME_PrintClockInSecond(voipHostData->averageOneWayDelay, clockStr);
    sprintf(buf, "Average One Way Delay (s) = %s", clockStr);
    IO_PrintStat(
        node,
        "Application",
        appType,
        ANY_DEST,
        voipHostData->initiatorPort,
        buf);

    sprintf(buf, "Maximum MOS = %f", voipHostData->maxMOS);
    IO_PrintStat(
        node,
        "Application",
        appType,
        ANY_DEST,
        voipHostData->initiatorPort,
        buf);

    if (voipHostData->minMOS == VOIP_MOS_MAX)
    {
        voipHostData->minMOS = voipHostData->maxMOS;
    }

    sprintf(buf, "Minimum MOS = %f", voipHostData->minMOS);
    IO_PrintStat(
        node,
        "Application",
        appType,
        ANY_DEST,
        voipHostData->initiatorPort,
        buf);

    if (numPktsRecvd == 0)
    {
        voipHostData->averageMOS = 0;
    }
    else
    {
        voipHostData->averageMOS = voipHostData->scoreMOS
                                        / numPktsRecvd;
    }
    sprintf(buf, "Average MOS = %f", voipHostData->averageMOS);
    IO_PrintStat(
        node,
        "Application",
        appType,
        ANY_DEST,
        voipHostData->initiatorPort,
        buf);
}


//--------------------------------------------------------------------------
// NAME         : AppVoipInitiatorFinalize()
// PURPOSE      : Collect statistics of initiator.
// ASSUMPTION   : None.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void AppVoipInitiatorFinalize(Node* node, AppInfo* appInfo)
{
    VoipHostData* voipHostData = (VoipHostData*) appInfo->appDetail;

    if (node->appData.appStats == TRUE)
    {
        VoipPrintStats(node, voipHostData);
    }
}


//--------------------------------------------------------------------------
// NAME         : AppVoipReceiverFinalize()
// PURPOSE      : Collect statistics of receiver.
// ASSUMPTION   : None.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void AppVoipReceiverFinalize(Node* node, AppInfo* appInfo)
{
    VoipHostData* voipHostData = (VoipHostData*) appInfo->appDetail;

    if (node->appData.appStats == TRUE)
    {
        VoipPrintStats(node, voipHostData);
    }
}


//--------------------------------------------------------------------------
// NAME         : VOIPgetOption()
// PURPOSE      : Get VOIP optional Parameter
// PARAMETERS   : token - String Parameter
// RETURN VALUE : Boolean
//--------------------------------------------------------------------------

int VOIPgetOption(char* token)
{
    int optionValue = 0;
    if (!strcmp(token, "CALL-STATUS"))
    {
        optionValue = VOIP_CALL_STATUS;
    }
    else if (!strcmp(token, "ENCODING"))
    {
        optionValue = VOIP_ENCODING_SCHEME;
    }
    else if (!strcmp(token, "PACKETIZATION-INTERVAL"))
    {
        optionValue = VOIP_PACKETIZATION_INTERVAL;
    }
    else if (!strcmp(token, "DSCP"))
    {
        optionValue = VOIP_DSCP;
    }
    else if (!strcmp(token, "PRECEDENCE"))
    {
        optionValue = VOIP_PRECEDENCE;
    }
    else if (!strcmp(token, "TOS"))
    {
        optionValue = VOIP_TOS;
    }
    return optionValue;
}

//--------------------------------------------------------------------------
// NAME         : VOIPsetCallStatus()
// PURPOSE      : set VOIP Call Status whether Accept or Reject
// PARAMETERS   : optionStr - String Parameter
// RETURN VALUE : Boolean
//--------------------------------------------------------------------------

BOOL VOIPsetCallStatus(char* optionStr)
{
    if (!strcmp(optionStr, "ACCEPT"))
    {
        return TRUE;
    }
    else if (!strcmp(optionStr, "REJECT"))
    {
        return FALSE;
    }
    else
    {
        char errStr[MAX_STRING_LENGTH];
        sprintf(errStr,"Invalid option: %s\n", optionStr);
        ERROR_ReportError(errStr);
        return FALSE;
    }
}
void VOIPSendRtpTerminateSession(Node* node, NodeAddress remoteAddr,
                                 unsigned short initiatorPort)
{
    Address  newrem_adr;
    Message* msg = MESSAGE_Alloc(node, APP_LAYER, APP_RTP,
                                 MSG_RTP_TerminateSession);
    MESSAGE_InfoAlloc(node, msg,
        sizeof(Address) + sizeof(unsigned short));
    char* msgInfo = MESSAGE_ReturnInfo(msg);

    newrem_adr.networkType = NETWORK_IPV4;
    newrem_adr.interfaceAddr.ipv4 = remoteAddr;

    memcpy(msgInfo, &newrem_adr, sizeof(Address));
    msgInfo += sizeof(Address);
    memcpy(msgInfo, &initiatorPort, sizeof(unsigned short));

    MESSAGE_Send(node, msg, (clocktype) 0);
}

void VOIPSendRtpInitiateNewSession(Node* node,
    NodeAddress localAddr,
    char* srcAliasAddress,
    NodeAddress remoteAddr,
    clocktype packetizationInterval,
    unsigned short localPort,
    BOOL           initiator,
    void*          RTPSendFunc)
{
    Address  local_v6,remote_v6;
    Message* newMsg = MESSAGE_Alloc(node, APP_LAYER, APP_RTP,
                                    MSG_RTP_InitiateNewSession);
    unsigned short voipPayloadType = VOIP_PAYLOAD_TYPE;
    MESSAGE_InfoAlloc(node, newMsg, 2 * sizeof(Address)
                                    + MAX_STRING_LENGTH
                                    + sizeof(clocktype)
                                    + 2 * sizeof(unsigned short)
                                    + sizeof(int)
                                    + sizeof(void*));
    char* newMsgInfo = MESSAGE_ReturnInfo(newMsg);

    local_v6.networkType = NETWORK_IPV4;
    local_v6.interfaceAddr.ipv4 = localAddr;
    remote_v6.networkType = NETWORK_IPV4;
    remote_v6.interfaceAddr.ipv4 = remoteAddr;

    memcpy(newMsgInfo, &local_v6, sizeof(Address));
    newMsgInfo += sizeof(Address);
    memcpy(newMsgInfo, srcAliasAddress, MAX_STRING_LENGTH);
    newMsgInfo += MAX_STRING_LENGTH;
    memcpy(newMsgInfo, &remote_v6, sizeof(Address));
    newMsgInfo += sizeof(Address);
    memcpy(newMsgInfo, &packetizationInterval, sizeof(clocktype));
    newMsgInfo += sizeof(clocktype);
    memcpy(newMsgInfo, &localPort, sizeof(unsigned short));
    newMsgInfo += sizeof(unsigned short);
    memcpy(newMsgInfo, &voipPayloadType, sizeof(unsigned short));
    newMsgInfo += sizeof(unsigned short);
    memcpy(newMsgInfo, &initiator, sizeof(int));
    newMsgInfo += sizeof(int);
    memcpy(newMsgInfo, &RTPSendFunc, sizeof(void*));

    MESSAGE_Send(node, newMsg, (clocktype) 0);
}
//---------------------------------------------------------------------------
// FUNCTION             : AppVoipClientAddAddressInformation.
// PURPOSE              : store client interface index, destination 
//                        interface index destination node id to get the 
//                        correct address when application starts
// PARAMETERS:
// + node               : Node*             : pointer to the node
// + clientPtr          : VoipHostData* : pointer to client data
// + remoteAddr         : NodeAddress   : Remote proxy/Destn addr
// + isProxy            : bool : flag to check if proxy d/b needs an update
// RETRUN               : NONE
//---------------------------------------------------------------------------
void
AppVoipClientAddAddressInformation(Node* node,
                                VoipHostData* clientPtr,
                                NodeAddress remoteAddr,
                                bool isProxy)
{
    // Store the client and destination interface index such that we can get
    // the correct address when the application starts
    NodeId destNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                                                node,
                                                remoteAddr);

    if (destNodeId != INVALID_MAPPING)
    {
        if (isProxy)
        {
            clientPtr->proxyNodeId = destNodeId;
            clientPtr->proxyInterfaceIndex = 
                (Int16)MAPPING_GetInterfaceIdForDestAddress(
                                                node,
                                                destNodeId,
                                                remoteAddr);

        }
        else
        {
            clientPtr->destNodeId = destNodeId;
            clientPtr->destInterfaceIndex = 
                (Int16)MAPPING_GetInterfaceIdForDestAddress(
                                                node,
                                                destNodeId,
                                                remoteAddr);
        }
    }
    Address rmtAddr;
    memset(&rmtAddr, 0, sizeof(Address));
    rmtAddr.networkType = NETWORK_IPV4;
    rmtAddr.interfaceAddr.ipv4 = remoteAddr;
    // Handle loopback address in destination
    if (destNodeId == INVALID_MAPPING)
    {
        if (NetworkIpCheckIfAddressLoopBack(node, rmtAddr))
        {
            clientPtr->destNodeId = node->nodeId;
            clientPtr->destInterfaceIndex = DEFAULT_INTERFACE;
        }
    }
    clientPtr->clientInterfaceIndex = 
        (Int16)MAPPING_GetInterfaceIndexFromInterfaceAddress(
                                            node,
                                            clientPtr->localAddr);
}
//---------------------------------------------------------------------------
// FUNCTION             :AppVoipClientGetSessionAddressState.
// PURPOSE              :get the current address sate of client and server 
// PARAMETERS:
// + node               : Node* : pointer to the node
// + clientPtr          : VoipHostData* : pointer to VOIP client data
// RETRUN:     
// addressState         : AppAddressState :
//                        ADDRESS_FOUND : if both client and server
//                                        are having valid address
//                        ADDRESS_INVALID : if either of them are in 
//                                          invalid address state
//---------------------------------------------------------------------------
AppAddressState
AppVoipClientGetSessionAddressState(Node* node,
                                   VoipHostData* clientPtr)
{
    // variable to determine the server current address state
    Int32 serverAddresState = 0; 
    // variable to determine the client current address state
    Int32 clientAddressState = 0; 
    Address localAddr, remoteAddr;
    memset(&localAddr, 0 ,sizeof(Address));
    memset(&remoteAddr, 0 ,sizeof(Address));

    // Get the current client and destination address if the
    // session is starting
    // Address state is checked only at the session start; if this is not
    // starting of session then return FOUND_ADDRESS
    clientAddressState =
        MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
                                node,
                                node->nodeId,
                                clientPtr->clientInterfaceIndex,
                                NETWORK_IPV4,
                                &localAddr);
    clientPtr->localAddr = localAddr.interfaceAddr.ipv4;
    clientPtr->callSignalAddr = clientPtr->localAddr;
    remoteAddr.interfaceAddr.ipv4 = clientPtr->remoteAddr;
    remoteAddr.networkType = NETWORK_IPV4;

    if (NetworkIpCheckIfAddressLoopBack(node, remoteAddr))
    {
        serverAddresState = clientAddressState;
    }
    else if (clientPtr->destNodeId != ANY_NODEID &&
        clientPtr->destInterfaceIndex != ANY_INTERFACE)
    {
        serverAddresState =
            MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
                                node,
                                clientPtr->destNodeId,
                                clientPtr->destInterfaceIndex,
                                NETWORK_IPV4,
                                &remoteAddr);
        clientPtr->remoteAddr = remoteAddr.interfaceAddr.ipv4;

    }

    // if either client or server are not in valid address state
    // then mapping will return INVALID_MAPPING
    if (clientAddressState == INVALID_MAPPING ||
        serverAddresState == INVALID_MAPPING)
    {
        return ADDRESS_INVALID;
    }
    return ADDRESS_FOUND;
}
