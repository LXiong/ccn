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
// PROTOCOL     :: SIP
// LAYER        :  APPLICATION
// REFERENCES   ::
// + whatissip.pdf  : www.sipcenter.com
// + SIP-2003-Future_of_SIP_and_Presence.pdf
// + siptutorial.pdf
// COMMENTS     :: This is implementation of SIP 2.0 as per RFC 3261
// **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "api.h"
#include "app_util.h"
#include "partition.h"
#include "transport_tcp.h"
#include "network_ip.h"
#include "multimedia_sip.h"
#include "app_voip.h"
#include "transport_rtp.h"

#include "app_dns.h"
// Pseudo traffic sender layer
#include "app_trafficSender.h"

#ifdef MUOS_LIB
#include "muos_sip.h"
#endif

#define  DEBUG     0
#define  DEBUG_APP 0


// VOIP->proxyConnectionId -> applicable for proxy routed connection establishement
// VOIP->connectionId -> for all direct call and direct path of proxy routed call
//
// UA:
// SipMsg->proxyConnectionId -> applicable for proxy routed connection establishement
// SipMsg->connectionId -> for all direct call and direct path of proxy routed call
//
// PROXY:
// SipMsg->connectionId -> Proxy server part connection with UAC/ Proxy client part
// SipMsg->proxyConnectionId -> Proxy client part connection with UAS/ other proxy server part


#define SipCheckSession(sipSession, funcName, nodeId) { if (!sipSession) { \
        char errStr[MAX_STRING_LENGTH]; \
        sprintf(errStr,"\nIn Function %s(), at node %d : Sip Session " \
                       "must exist", __FUNCTION__, nodeId); \
        ERROR_Assert(FALSE, errStr); } }

// /**
// FUNCTION   :: SipAllocateCirBufInVector
// LAYER      :: APPLICATION
// PURPOSE    :: Allocate memory for circular buffer in Circular Buff Vector
// PARAMETERS ::
// + sip       : SipData* : Pointer to the SipData instance
// + connId    : unsigned short : Connection Id for which Cir. Buf to be
//               alloted.
// RETURN     :: void : NIL
// **/
void SipAllocateCirBufInVector(SipData* sip, unsigned short connId)
{
    unsigned int idx;
    CircularBuffer* cirBuff = NULL;
    CirBufVector* cbVec = sip->SipGetCirBufVector();

    for (idx = 0; idx < cbVec->size(); idx++)
    {
        cirBuff = (*cbVec)[idx];
        if (connId == cirBuff->getIndex())
        {
            //connection Id Already exist
            return;
        }
    }

    cirBuff = new CircularBuffer(connId, SIP_CIR_BUF_SIZE);
    cbVec->push_back(cirBuff);
}


// /**
// FUNCTION   :: SipGetCirBufForConnId
// LAYER      :: APPLICATION
// PURPOSE    :: To get the cicular buffer corresponding to a connection ID
//               note: there is one circular buffer for each conn.ID.
// PARAMETERS ::
// + sip       : SipData* : Pointer to the SipData instance
// + connId    : unsigned short : Connection ID of the TCP  connection
// RETURN     :: CircularBuffer* : Pointer to the desired circular Buffer
// **/
CircularBuffer* SipGetCirBufForConnId(SipData* sip,
                                      unsigned short connId)
{
    unsigned int idx;
    CircularBuffer* cirBuf;
    CirBufVector* cbVec = sip->SipGetCirBufVector();

    ERROR_Assert(connId, "Invalid connectionId passed\n");

    for (idx = 0; idx < cbVec->size(); idx++)
    {
        cirBuf = (*cbVec)[idx];
        if (connId == cirBuf->getIndex())
        {
            return cirBuf;
        }
    }
    return NULL;
}


// /**
// FUNCTION   :: SipDeleteCirBuf
// LAYER      :: APPLICATION
// PURPOSE    :: Delete the circular buffer with conn. ID from recd message
// PARAMETERS ::
// + sip       : SipData* : Pointer to the SipData instance
// + connId    : int      : ConnectionId of the cir.buffer to be deleted
// RETURN     :: void : NIL
// **/
void SipDeleteCirBuf(SipData* sip, int connId)
{
    cbVectorIterator cbVecItr;
    CirBufVector* cbVec = sip->SipGetCirBufVector();

    for (cbVecItr = cbVec->begin(); cbVecItr != cbVec->end(); cbVecItr++)
    {
        if (((CircularBuffer*)*cbVecItr)->getIndex() == connId)
        {
            delete ((CircularBuffer*)*cbVecItr);
            cbVec->erase(cbVecItr);
            break;
        }
    }
}


// /**
// FUNCTION   :: SipDeleteCirBufVector
// LAYER      :: APPLICATION
// PURPOSE    :: Delete the entire circular buffer vector for this SipData
// PARAMETERS ::
// + sip       : SipData* : Pointer to the SipData instance
// RETURN     :: void : NIL
// **/
void SipDeleteCirBufVector(SipData* sip)
{
    unsigned int idx;
    CircularBuffer* cirBuf = NULL;

    CirBufVector* cbVec = sip->SipGetCirBufVector();
    for (idx = 0; idx < cbVec->size(); idx++)
    {
        cirBuf = (*cbVec)[idx];
        if (cirBuf)
        {
            delete cirBuf;
        }
    }
    cbVec->clear();
}


// /**
// FUNCTION   :: SipWriteToCirBufVector
// LAYER      :: APPLICATION
// PURPOSE    :: To write the contents of incoming packet from transport
//               layer to the circular buffer
// PARAMETERS ::
// + node      : Node* : Pointer to the node that receives the event
// + msg       : Message* : Pointer to the received message
// RETURN     :: void : NIL
// **/
void SipWriteToCirBufVector(Node* node, Message* msg)
{
    SipData* sip = (SipData*)node->appData.multimedia->sigPtr;
    TransportToAppDataReceived *dataRecvd =
     (TransportToAppDataReceived *)MESSAGE_ReturnInfo(msg);

    unsigned char* pktRecd = (unsigned char*)MESSAGE_ReturnPacket(msg);
    int pktLen = MESSAGE_ReturnPacketSize(msg);

    CircularBuffer* cirBuf =
        SipGetCirBufForConnId(sip,(unsigned short)dataRecvd->connectionId);

    if (!cirBuf)
    {
        ERROR_ReportWarning("Invalid Circular buffer\n");
        return;
    }
    cirBuf->write(pktRecd, pktLen);
}


// /**
// FUNCTION   :: SipIsCompleteMsg
// LAYER      :: APPLICATION
// PURPOSE    :: check if the passed buffer contains a complete message
// PARAMETERS ::
// + inputBuf  : unsigned char* : Pointer to the input buffer
// + pktSize   : long : size of the above buffer
// RETURN     :: unsigned short: Actual message length, zero if incomplete
// **/
unsigned short SipIsCompleteMsg(unsigned char* inBuf, unsigned short pktSize)
{
    ERROR_Assert(inBuf != NULL && *inBuf != '\0',
            "SipIsCompleteMsg: No data available for parsing");

    unsigned short i = 1;
    unsigned char* tmp = inBuf;
    int headerSize = 0;

    while (i < pktSize)
    {
        if (*tmp == '\n')
        {
            if (*(tmp + 1) == '\r' &&
                *(tmp + 2) == '\n')
            {
                // empty line at the end of the headers
                headerSize = i + 2;
                break;
            }
        }
        i++;
        tmp++;
    }
    // Check for no empty line found
    if (headerSize == 0)
    {
        return 0;
    }

    // Search for "Content-Length: "
    int contentLength = 0;
    char *cl = strstr((char*) inBuf, (char*) "Content-Length: ");
    if (cl != NULL)
    {
        contentLength = atoi(cl + strlen((char*) "Content-Length: "));
    }

    if (headerSize + contentLength <= pktSize)
    {
        return headerSize + contentLength;
    }
    else
    {
        return 0;
    }
}


// /**
// FUNCTION   :: SipIsHostClient
// LAYER      :: APPLICATION
// PURPOSE    :: Check if the Sip node is working as Client
// PARAMETERS ::
// + node      : Node* : Pointer to the node to be checked
// RETURN     :: BOOL : True if the node is a client
// **/
BOOL SipIsHostClient(
    Node* node,
    short localPort)
{
    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;
    //SipSession* sipSession = sip->SipGetSipSessionByLocalPort(localPort);
    // changed by Li
    SipSession* sipSession = sip->SipGetSipSessionByRtpPort(localPort);


    SipCheckSession(sipSession, __FUNCTION__, node->nodeId);

    if (sipSession->SipGetTransactionState() == SIP_CLIENT)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION   :: SipIsHostServer
// LAYER      :: APPLICATION
// PURPOSE    :: Check if the Sip node is working as server
// PARAMETERS ::
// + node      : Node* : Pointer to the node to be checked
// RETURN     :: BOOL : True if the node is a server
// **/
BOOL SipIsHostServer(
    Node* node,
    short localPort)
{
    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;
    //SipSession* sipSession = sip->SipGetSipSessionByLocalPort(localPort);
    // changed by Li - 1/20 - begin
    SipSession* sipSession = sip->SipGetSipSessionByRtpPort(localPort);
    // changed by Li - 1/20 - end

    if (!sipSession)
        printf("node %d \n", node->nodeId);
    SipCheckSession(sipSession, __FUNCTION__, node->nodeId);

    if (sipSession->SipGetTransactionState() == SIP_SERVER)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


// /**
// FUNCTION   :: SipInit
// LAYER      :: APPLICATION
// PURPOSE    :: Initializes the SIP-related structures of the sip node
// PARAMETERS ::
// + node      : Node* : Pointer to the node to be initialized
// + nodeInput : NodeInput* : Pointer to input configuration file
// + hostType  : SipHostType : Type of Sip Host
// RETURN     :: void : NULL
// **/
void SipInit(Node* node,
             const NodeInput *nodeInput,
             SipHostType hostType,
             clocktype waitTime)
{
    AppMultimedia* multimedia = node->appData.multimedia;
    NodeAddress homeUnicastAdr;
    Address sipAddr;
    memset(&sipAddr, 0, sizeof(Address));
    SipData* sip = new SipData(node, hostType);
    int i;
    if (sip == NULL){
        ERROR_ReportError("SipData memory allocation error");
        return ;
    }

    // Li: temporarily put here, to be changed - begin
    sip->setSignalAddr( MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(
                            node,
                            node->nodeId));
    /*if (node->nodeId == 42)
    {
        char sipAddrStr[256] ;

        IO_ConvertIpAddressToString(&sip->getSignalAddr(), sipAddrStr);

        printf("node %d has sip signal address %s \n", node->nodeId,
            sipAddrStr);
    }*/
    // Li: temporarily put here, to be changed -end

//    sip->SipSetCallSignalPort(APP_SIP);
    sip->SipSetCallSignalPort(node->appData.nextPortNum++);

    // Set void pointer in node data structure to this SipData instance
    multimedia->sigPtr = (void *) sip;

    multimedia->initiateFunction  = &SipInitiateConnection;
    multimedia->terminateFunction = &SipTerminateConnection;

    multimedia->hostCallingFunction = &SipIsHostClient;
    multimedia->hostCalledFunction  = &SipIsHostServer;

    sip->SipSetHostType(hostType);
    sip->SipReadConfigFile(node, nodeInput);
    if (hostType == SIP_PROXY && SipIsDHCPClient(node , nodeInput))
    {
        ERROR_ReportError("\nSip Proxy Can't be DHCP Client");
    }
    if (sip->SipGetTransProtocolType() == UDP)
    {
        printf("UDP protocol is not supported in the current version!\n");
        return;
    }

    if (hostType == SIP_UA)
    {
        sip->SipReadAliasFileForHost(node, nodeInput);
    }
    else if (hostType == SIP_PROXY)
    {
        sip->SipReadAliasFileForProxy(node, nodeInput);
        sip->SipReadDNSFile(node, nodeInput);
    }
    else
    {
        ERROR_ReportError("Currently only UserAgent and"
            "Proxy node are allowed\n");
    }
    node->appData.appTrafficSender->SipDnsInit(
                                            node,
                                            &SipUrlSessionStartCallBack);

    sip->SipStatsInit(node, nodeInput);
    sip->SipSessionListInit(node);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        // Delay TCP server listening by start time
        // such that server may acquire an address by that time
        // to ensure that server listens earlier than TCP
        // open; therefore listen timer should be set at unit 
        // time (1 ns) before Open. SIP Proxy's cannot be DHCP clients,
        // so tcp listen will not be delayed for them.

        clocktype listenTime = 0;
        if (hostType == SIP_UA)
        {
            listenTime = waitTime - 1 * NANO_SECOND;
        }
        sipAddr.networkType = NETWORK_IPV4;
        sipAddr.interfaceAddr.ipv4 = NetworkIpGetInterfaceAddress(node, i);
        AppStartTcpAppSessionServerListenTimer(
                                            node,
                                            APP_SIP,
                                            sipAddr,
                                            NULL,
                                            listenTime);

    }
};
//---------------------------------------------------------------------------
// NAME:        SipIsDHCPClient()
// PURPOSE:     find whether the node is DHCP client at the interface index
// PARAMETERS:
// + node       : Node* : Node which is to be checked for DHCP client
// + nodeInput  : NodeInput* : The input file
// RETURN:
// BOOL.    : TRUE if DHCP client FALSE otherwise
//---------------------------------------------------------------------------
BOOL SipIsDHCPClient(Node* node, const NodeInput* nodeInput) 
{
    BOOL retVal;
    Int32 matchType = 0;
    Int32 i = 0;
    
    char parameterValue[MAX_STRING_LENGTH];

    for (i = 0; i < node->numberInterfaces; i++)
    {
       IO_ReadString(node->nodeId,
                      i,
                      MAPPING_GetInterfaceAddrForNodeIdAndIntfId
                      (node,node->nodeId,i),
                      NULL,
                      nodeInput,
                      "DHCP-CLIENT",
                      parameterValue,
                      retVal,
                      matchType);

        if (retVal == 0 || (!strcmp(parameterValue,"NO")))
        {
            return FALSE;
        }
        if (retVal == 1 && (!strcmp(parameterValue,"YES")))
        {
            return TRUE;
        }
    }
    return FALSE;
}

//---------------------------------------------------------------------------
// NAME:        SIPUAInit.
// PURPOSE:     listen on SIP port.
// PARAMETERS:
// + node       : Node*     : pointer to the node
// + serverAddr : Address   : server address
// RETURN:      NONE
//---------------------------------------------------------------------------
void
SIPUAInit(Node* node, Address serverAddr)
{
    APP_TcpServerListen(node,
                        APP_SIP,
                        serverAddr,
                        (short) APP_SIP);
}


// /**
// FUNCTION   :: SipProxyInit
// LAYER      :: APPLICATION
// PURPOSE    :: Reads the list of proxy nodes, and initializes each
//               of these SIP proxy nodes
// PARAMETERS ::
// + firstNode : Node* : Pointer to the first node
// + nodeInput : NodeInput* : Pointer to input configuration file
// RETURN     :: void : NULL
// **/
void SipProxyInit(Node* firstNode, const NodeInput* nodeInput)
{
    //modified for removing SIP proxy list
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    IO_ReadString(
                firstNode->nodeId,
                ANY_ADDRESS,
                nodeInput,
                "SIP-PROXY",
                &retVal,
                buf);
    if (retVal && !strcmp(buf,"YES"))
    {
        SipInit(
            firstNode,
            nodeInput,
            SIP_PROXY,
            (clocktype)0);
    }
    if (retVal && strcmp(buf,"YES") && strcmp(buf,"NO"))
    {
        ERROR_ReportError("\nValue for SIP-PROXY should be YES/NO");
    }
}

// /**
// FUNCTION   :: SipFinalize
// LAYER      :: APPLICATION
// PURPOSE    :: Prints the statistics data from simulation for a node
// PARAMETERS ::
// + node      : Node* : Pointer to the node getting finalized
// RETURN     :: void : NULL
// **/
void SipFinalize(Node* node)
{
    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;

    // sip valid only for participating nodes
    if (!sip)
    {
        return;
    }

    if (node->appData.appStats == FALSE)
    {
        return;
    }

    // Delete CircularBuffer Vector
    // Uncommented following line if we need delete the buffer
    // at the end of simulation
    // SipDeleteCirBufVector(sip);

    if (!sip->SipGetIsSipStatEnabled())
    {
        return;
    }

    SipStat stat = sip->SipGetStat();

    // print the output statistics
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "Number Of Invite Requests Sent = %d", stat.inviteSent);
    IO_PrintStat(
        node,
        "Application",
        "SIP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of Ack Requests Sent = %d", stat.ackSent);
    IO_PrintStat(
        node,
        "Application",
        "SIP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of Bye Requests Sent = %d", stat.byeSent);
    IO_PrintStat(
         node,
         "Application",
         "SIP",
         ANY_DEST,
         -1 /* instance Id */,
         buf);

    sprintf(buf, "Number Of Trying Responses Received = %d", stat.tryingRecd);
    IO_PrintStat(
          node,
          "Application",
          "SIP",
          ANY_DEST,
          -1 /* instance Id */,
          buf);

    sprintf(buf, "Number Of Ringing Responses Received = %d", stat.ringingRecd);
    IO_PrintStat(
       node,
       "Application",
       "SIP",
       ANY_DEST,
       -1 /* instance Id */,
       buf);

    sprintf(buf, "Number Of Ok Responses Received = %d", stat.okRecd);
    IO_PrintStat(
       node,
       "Application",
       "SIP",
       ANY_DEST,
       -1 /* instance Id */,
       buf);

    // UAS
    sprintf(buf, "Number Of Invite Requests Received = %d", stat.inviteRecd);
    IO_PrintStat(
       node,
       "Application",
       "SIP",
       ANY_DEST,
       -1 /* instance Id */,
       buf);

    sprintf(buf, "Number Of Ack Requests Received = %d", stat.ackRecd);
    IO_PrintStat(
        node,
        "Application",
        "SIP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of Bye Requests Received = %d", stat.byeRecd);
    IO_PrintStat(
         node,
         "Application",
         "SIP",
         ANY_DEST,
         -1 /* instance Id */,
         buf);

    sprintf(buf, "Number Of Ringing Responses Sent = %d", stat.ringingSent);
    IO_PrintStat(
        node,
        "Application",
        "SIP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of Ok Responses Sent = %d", stat.okSent);
    IO_PrintStat(
        node,
        "Application",
        "SIP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    // proxy
    sprintf(buf, "Number Of Requests Forwarded = %d", stat.reqForwarded);
    IO_PrintStat(
        node,
        "Application",
        "SIP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of Responses Forwarded = %d", stat.respForwarded);
    IO_PrintStat(
        node,
        "Application",
        "SIP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of Requests Dropped = %d", stat.reqDropped);
    IO_PrintStat(
        node,
        "Application",
        "SIP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of Responses Dropped = %d", stat.respDropped);
    IO_PrintStat(
        node,
        "Application",
        "SIP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of Trying Responses Sent = %d", stat.tryingSent);
    IO_PrintStat(
        node,
        "Application",
        "SIP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of Attempted Calls = %d", stat.callAttempted);
    IO_PrintStat(
        node,
        "Application",
        "SIP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of Attempted Calls Successful = %d", stat.callConnected);
    IO_PrintStat(
        node,
        "Application",
        "SIP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of Calls Received = %d", stat.ackRecd);
    IO_PrintStat(
        node,
        "Application",
        "SIP",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    sprintf(buf, "Number Of Calls Rejected = %d", stat.callRejected);
    IO_PrintStat(
         node,
         "Application",
         "SIP",
         ANY_DEST,
         -1 /* instance Id */,
         buf);

};


// /**
// FUNCTION   :: SipHandleConnectionDelayTimer
// LAYER      :: APPLICATION
// PURPOSE    :: Handles the Connection delay timer event
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + msg       : Message* : Pointer to the received message
// RETURN     :: void : NULL
// **/
void SipHandleConnectionDelayTimer(Node* node, Message* msg)
{
    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;
    SipCallInfo* sipCallInfo = (SipCallInfo*)(MESSAGE_ReturnInfo(msg));
    VoipHostData* voip = NULL;
    SipSession* sipSession = sip->SipGetSipSessionByToFromCallId(
                                sipCallInfo->to,
                                sipCallInfo->from,
                                sipCallInfo->callId);

    SipCheckSession(sipSession, __FUNCTION__, node->nodeId);

    if (sipSession->SipGetConnDelayTimerRunning() == false)
    {
        if (DEBUG)
        {
            printf("\nIn Function %s(), at node %d : Connection delay "
                   "timer has been cancelled for sip session : "
                   "To     : %s\n"
                   "From   : %s\n"
                   "CallId : %s\n",
                   __FUNCTION__, node->nodeId, sipSession->SipGetTo(),
                   sipSession->SipGetFrom(), sipSession->SipGetCallId());
        }

        return;
    }

    SipMsg* sipMsg = sip->SipFindMsg(sipCallInfo->connectionId);

    if (!sipMsg)
    {
        char errStr[MAX_STRING_LENGTH];
        sprintf(errStr,"\nIn Function %s(), at node %d : Invalid Sip Msg "
                       "returned", __FUNCTION__, node->nodeId);
        ERROR_Assert(FALSE, errStr);
    }

    if (!sipMsg)
    {
        sipSession->SipSetCallState(SIP_IDLE);
        return;
    }

    if (DEBUG)
    {
        printf("ConnectionDelay event received by node %d\n", node->nodeId);
    }

    if (sip->SipGetHostType() == SIP_UA)
    {
        //printf("node %d sent ok response \n", node->nodeId);

        if (sip->SipGetCallModel() == SipProxyRouted )
        {
           voip = VoipCallReceiverGetDataForProxyConnectionId(node,
                       sipCallInfo->connectionId);
        }
        else
        {
           voip = VoipCallReceiverGetDataForConnectionId(node,
                                                         sipCallInfo->connectionId);
        }
        
        if (!voip)
        {
            char errBuf[MAX_STRING_LENGTH];
            sprintf(errBuf, "Connection Failure, check proper Proxy"
                " node specification\n");
            ERROR_ReportError(errBuf);
        }

        sipMsg->SipCreate2xxOkResponsePkt(node, sip, voip);

        if (sip->SipGetCallModel() == SipDirect)
        {
            Message *tcpmsg = APP_TcpCreateMessage(
                node,
                sipMsg->SipGetConnectionId(),
                TRACE_SIP);

            APP_AddPayload(
                node,
                tcpmsg,
                sipMsg->SipGetBuffer(),
                sipMsg->SipGetLength());

            APP_TcpSend(node, tcpmsg);
        }
        else if (sip->SipGetCallModel() == SipProxyRouted)
        {
            Message *tcpmsg = APP_TcpCreateMessage(
                node,
                sipMsg->SipGetProxyConnectionId(),
                TRACE_SIP);

            APP_AddPayload(
                node,
                tcpmsg,
                sipMsg->SipGetBuffer(),
                sipMsg->SipGetLength());

            APP_TcpSend(node, tcpmsg);
        }

        MEM_free(sipMsg->SipGetBuffer());
        sipSession->SipSetCallState(SIP_OK);
        sip->SipIncrementOkSent();
    }
}

RtpSession* RtpGetSessionPtrByDestAddr(RtpData* rtp,
                                       Address destAddr,
                                       unsigned short initiatorPort,
                                       BOOL isRtpProcessed);
// /**
// FUNCTION   :: SipProcessActiveOpenResultUA
// LAYER      :: APPLICATION
// PURPOSE    :: Handles successful TCP connection establishment event
//               at the client(UA) side.
// PARAMETERS ::
// + node      : Node* : Pointer to the node initiating TCP connection
// + sip       : SipData* : Pointer to SipData
// + openResult: TransportToAppOpenResult* : Pointer to
//               TransportToAppOpenResult structure
// RETURN     :: void : NULL
// **/
void SipProcessActiveOpenResultUA(
    Node* node,
    SipData* sip,
    TransportToAppOpenResult* openResult)
{

    // Get voip pointer
    VoipHostData* voip = VoipGetInitiatorDataForCallSignal(
                             node,
                             openResult->localPort);

    // added by Li -begin
    SipSession* sipSession = sip->SipGetSipSessionByLocalPort(
        openResult->localPort);

    sip->SipSetCallSignalAddr(openResult->localAddr.interfaceAddr.ipv4);

    sipSession->SipSetRtpPort(voip->initiatorPort);
    // added by Li -end

    SipCheckSession(sipSession, __FUNCTION__, node->nodeId);

    if ((sipSession->SipGetCallState() != SIP_IDLE) &&
        (sipSession->SipGetCallState() != SIP_OK))
    {
        ERROR_ReportWarning("UA is busy. Call cannot be established\n");
        return;
    }

#ifdef MUOS_LIB
    // should wait for muos allocate services complete
    if (node->networkData.networkProtocol == CELLULAR)
    {
        if (sip->SipGetCallModel() == SipDirect)
        {
            // don't send the invite here
            return MUOS_AllocateConnection(node,
                sip, voip, openResult);
        } else if (sip->SipGetCallModel() == SipProxyRouted &&
            sipSession->SipGetCallState() == SIP_IDLE)
        {
            // don't send the invite here
            return MUOS_AllocateConnection(node,
                sip, voip, openResult);
        }
    }
#endif

    SipMsg* sipMsg = NULL;
    bool invSent = false;

    if (sip->SipGetCallModel() == SipDirect)
    {
        // set the connectionId in voip
        voip->connectionId = openResult->connectionId;
        VoipUpdateInitiator(node, voip, openResult, TRUE);

        sipMsg = new SipMsg();

        // set the connectionId in SipMsg
        sipMsg->SipSetConnectionId(openResult->connectionId);
        sipSession->SipSetConnectionId(openResult->connectionId);

        sipSession->SipSetTransactionState(SIP_CLIENT);
        sipMsg->SipCreateInviteRequestPkt(node, sip, voip);

        sipSession->SipSetCallState(SIP_CALLING);
        sip->SipInsertMsg(sipMsg);
        invSent = true;
    }
    else if (sip->SipGetCallModel() == SipProxyRouted)
    {
        if (sipSession->SipGetCallState() == SIP_IDLE)
        {
            // during INVITE messaging TCP connection
            // set the proxyConnectionId in voip
            voip->proxyConnectionId = openResult->connectionId;
            VoipUpdateInitiator(node, voip, openResult, FALSE);

            sipMsg = new SipMsg();

            // set the proxyConnectionId in SipMsg
            sipMsg->SipSetProxyConnectionId(openResult->connectionId);
            sipSession->SipSetProxyConnectionId(openResult->connectionId);
            sipMsg->SipCreateInviteRequestPkt(node, sip, voip);

            sipSession->SipSetTransactionState(SIP_CLIENT);
            sipSession->SipSetCallState(SIP_CALLING);
            sip->SipInsertMsg(sipMsg);
            invSent = true;
        }
        else
        {
            // post INV message direct tcp connection
            sipMsg = sip->SipFindMsg(voip->proxyConnectionId);

            ERROR_Assert(sipMsg, "Invalid SipMsg returned\n");

            // set the connectionId in voip
            voip->connectionId = openResult->connectionId;

            // storing the connectionId in SipMsg
            sipMsg->SipSetConnectionId(openResult->connectionId);

            sipSession->SipSetConnectionId(openResult->connectionId);//???

            sipMsg->SipCreate2xxAckRequestPkt(node, sip);

            sipSession->SipSetCallState(SIP_COMPLETED);
        }
    }

    sipSession->SipSetTo(sipMsg->SipGetTo());
    sipSession->SipSetFrom(sipMsg->SipGetFrom());
    sipSession->SipSetCallId(sipMsg->SipGetCallId());

            Message *tcpmsg = APP_TcpCreateMessage(
                node,
                openResult->connectionId,
                TRACE_SIP);

            APP_AddPayload(
                node,
                tcpmsg,
                sipMsg->SipGetBuffer(),
                sipMsg->SipGetLength());

            APP_TcpSend(node, tcpmsg);

    MEM_free(sipMsg->SipGetBuffer());

    if (invSent)
    {

        // start the callTimeOut timer
        AppMultimedia* multimedia = node->appData.multimedia;

        Message* tmOutMsg = MESSAGE_Alloc(node,
                                 APP_LAYER,
                                 APP_SIP,
                                 MSG_APP_SipCallTimeOut);

        MESSAGE_InfoAlloc(node, tmOutMsg, sizeof(int));

        *((int*)MESSAGE_ReturnInfo(tmOutMsg)) = openResult->connectionId;


        MESSAGE_Send(node, tmOutMsg, multimedia->callTimeout);
    }


}


// /**
// FUNCTION   :: SipProcessActiveOpenResultProxy
// LAYER      :: APPLICATION
// PURPOSE    :: Handles successful TCP connection establishment event
//               at the client(proxy) side.
// PARAMETERS ::
// + node      : Node* : Pointer to the node initiating TCP connection
// + sip       : SipData* : Pointer to SipData
// + openResult: TransportToAppOpenResult* : Pointer to
//               TransportToAppOpenResult structure
// RETURN     :: void : NULL
// **/
void SipProcessActiveOpenResultProxy(
    Node* node,
    SipData* sip,
    TransportToAppOpenResult* openResult)
{
    SipMsg* sipMsg = sip->SipFindMsg((unsigned short) openResult->localPort);

    if (!sipMsg)
    {
        ERROR_Assert(FALSE, "No sip message available at proxy node\n");
    }

    sipMsg->SipSetTargetIp(openResult->remoteAddr.interfaceAddr.ipv4);
    sipMsg->SipSetProxyConnectionId(openResult->connectionId);

    // forward the packet if maxFwd > 0
    int maxFwd = sipMsg->SipGetMaxForwards();
    if (maxFwd <= 0)
    {
        sip->SipIncrementReqDropped();
        sipMsg->SipCreateResponsePkt(node, sip, TOO_MANY_HOPS, "INVITE");

        // send in reverse direction
        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            sipMsg->SipGetConnectionId(),
            TRACE_SIP);

        APP_AddPayload(
            node,
            tcpmsg,
            sipMsg->SipGetBuffer(),
            sipMsg->SipGetLength());

        APP_TcpSend(node, tcpmsg);

        MEM_free(sipMsg->SipGetBuffer());
        return;
    }

    sipMsg->SipForwardInviteReqPkt(node);
            Message *tcpmsg = APP_TcpCreateMessage(
                node,
                openResult->connectionId,
                TRACE_SIP);

            APP_AddPayload(
                node,
                tcpmsg,
                sipMsg->SipGetBuffer(),
                sipMsg->SipGetLength());

            APP_TcpSend(node, tcpmsg);

    MEM_free(sipMsg->SipGetBuffer());
}


// /**
// FUNCTION   :: SipProcessActiveOpenResult
// LAYER      :: APPLICATION
// PURPOSE    :: Handles successful TCP connection establishment event
//               at the client side.
// PARAMETERS ::
// + node      : Node* : Pointer to the node initiating TCP connection
// + sip       : SipData* : Pointer to SipData
// + openResult: TransportToAppOpenResult* : Pointer to
//               TransportToAppOpenResult structure
// RETURN     :: void : NULL
// **/
void SipProcessActiveOpenResult(Node* node,
                                TransportToAppOpenResult* openResult)
{
    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;

    if (sip->SipGetHostType() == SIP_UA)
    {
        SipProcessActiveOpenResultUA(node, sip, openResult);
    }
    else if (sip->SipGetHostType() == SIP_PROXY)
    {
        SipProcessActiveOpenResultProxy(node, sip, openResult);
    }
}


// /**
// FUNCTION   :: SipProcessPassiveOpenResult
// LAYER      :: APPLICATION
// PURPOSE    :: Handles successful TCP connection establishment event
//               at the server side.
// PARAMETERS ::
// + node      : Node* : Pointer to the node initiating TCP connection
// + openResult: TransportToAppOpenResult* : Pointer to
//               TransportToAppOpenResult structure
// RETURN     :: void : NULL
// **/
void SipProcessPassiveOpenResult(Node* node,
        TransportToAppOpenResult* openResult)
{
    SipData* sip = (SipData*)node->appData.multimedia->sigPtr;
    SipCallModel callModel = sip->SipGetCallModel();

    if (sip->SipGetHostType() == SIP_UA)
    {
        VoipHostData* voipReceiver =
            VoipGetReceiverDataForCallSignal(
                node,
                openResult->remotePort);

        if (callModel == SipDirect && voipReceiver)
        {
            // connection id is set here for direct model and direct path
            // of proxy routed model, for proxy routed path, it will be set
            // on the receipt of INVITE message at receiver
            voipReceiver->connectionId = openResult->connectionId;
            VoipUpdateInitiator(node, voipReceiver, openResult, TRUE);

        }

        // for storage and later retrieval of the fromIp at server end
        // of the proxyRouted path (voip Not null already chked)
        if (callModel == SipProxyRouted)
        {
            SipSession* sipSession = sip->SipGetSipSessionByLocalPort(
                openResult->remotePort);

            if (!sipSession || sipSession->SipGetCallState() == SIP_IDLE)
            {
                SipMsg* sipMsg = new SipMsg();
                sipMsg->SipSetProxyConnectionId(openResult->connectionId);
                sipMsg->SipSetFromIp(GetIPv4Address(openResult->remoteAddr));
                sip->SipInsertMsg(sipMsg);
            }

            if (sipSession)
            {
                sipSession->SipSetConnectionId(openResult->connectionId);
            }

            if (voipReceiver)
            {
                SipSession* sipSession = sip->SipGetSipSessionByLocalPort(
                    openResult->remotePort);
                voipReceiver->connectionId = openResult->connectionId;

            }
        }

    }
    else if (sip->SipGetHostType() == SIP_PROXY)
    {
        NodeAddress hostId;
        Int16 interfaceIndex;

        Address hostIPAddr;
        Int32 found = FALSE;
        hostId = MAPPING_GetNodeIdFromInterfaceAddress(
                                            node,
                                            openResult->remoteAddr);
        interfaceIndex = 
                (short)MAPPING_GetInterfaceIndexFromInterfaceAddress(
                                            node,
                                            openResult->remoteAddr);

        unsigned short i ;
        SipHostBlockList hostBlockList ;
        hostBlockList = sip->SipGetHostBlockList();

        SipHostBlock * hostBlock = NULL;
        hostBlock = hostBlockList.hostBlockHead;

        for (;hostBlock != NULL; hostBlock = hostBlock->next)
        {
            for (i = 0; i < hostBlock->hostCount; i++)
            {
                if (hostId == hostBlock->hostList[i].hostId)
                {
                    hostIPAddr.interfaceAddr.ipv4 = 
                        MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
                                            node,
                                            hostId,
                                            interfaceIndex);

                    hostBlock->hostList[i].hostIp = hostIPAddr.interfaceAddr.ipv4;
                    found = TRUE;
                    break;
                }
            }
            if (found)
            {
                break;
            }
        }

        // for storage and later retrieval of the fromIp
        SipMsg* sipMsg = new SipMsg();
        sipMsg->SipSetConnectionId(openResult->connectionId);
        sipMsg->SipSetFromIp(GetIPv4Address(openResult->remoteAddr));
        sip->SipInsertMsg(sipMsg);
    }
}


// /**
// FUNCTION   :: SipProcessOpenResult
// LAYER      :: APPLICATION
// PURPOSE    :: Handles successful TCP connection establishment event
// PARAMETERS ::
// + node      : Node* : Pointer to the node initiating TCP connection
// + msg       : Message* : Pointer to event message
//               TransportToAppOpenResult structure
// RETURN     :: void : NULL
// **/
void SipProcessOpenResult(Node* node, Message* msg)
{
    TransportToAppOpenResult* openResult;
    openResult = (TransportToAppOpenResult*) MESSAGE_ReturnInfo(msg);
    SipData* sip = (SipData*)node->appData.multimedia->sigPtr;

    if (DEBUG)
    {
        char timeStr[MAX_STRING_LENGTH];
        char localIpAddr[MAX_STRING_LENGTH];
        char remoteIpAddr[MAX_STRING_LENGTH];

        ctoa(node->getNodeTime(), timeStr);
        IO_ConvertIpAddressToString(
            GetIPv4Address(openResult->localAddr),
            localIpAddr);
        IO_ConvertIpAddressToString(
            GetIPv4Address(openResult->remoteAddr),
            remoteIpAddr);

        printf(" localIP      = %s, localPort    = %u\n"
               " remoteIP     = %s, remotePort   = %u\n"
               " connectionId = %d, TimeofConn   = %s\n",
               localIpAddr, openResult->localPort,
               remoteIpAddr, openResult->remotePort,
               openResult->connectionId, timeStr);
    }

    if (openResult->connectionId < 0)
    {
        // wrong connId may be due to wrong node specified as proxy
        if (sip->SipGetCallModel() == SipProxyRouted)
        {
            char errBuf[MAX_STRING_LENGTH];
            sprintf(errBuf, "Node %d : TCP connection failure, check proper"
                            "Proxy node specification\n", node->nodeId);
            ERROR_ReportWarning(errBuf);
        }
        else
        {
            char errBuf[MAX_STRING_LENGTH];
            sprintf(errBuf, "Node %d : Error in TCP connection !\n",
                    node->nodeId);
            ERROR_ReportWarning(errBuf);
        }
        return;
    }

    // allocate cirBuf in vector for this connection Id
    SipAllocateCirBufInVector(sip, (unsigned short)openResult->connectionId);

    if (openResult->type == TCP_CONN_ACTIVE_OPEN)
    {
        if (DEBUG)
        {
            printf("Active connection made for node %d with "
             "connection ID: %d\n", node->nodeId, openResult->connectionId);
        }
        SipProcessActiveOpenResult(node, openResult);
    }
    else if (openResult->type == TCP_CONN_PASSIVE_OPEN)
    {
        if (DEBUG)
        {
            printf("Passive connection made for node %d with "
             "connection ID: %d\n", node->nodeId, openResult->connectionId);
        }
        SipProcessPassiveOpenResult(node, openResult);
    }
}


// /**
// FUNCTION   :: SipHandleInviteRequestUA
// LAYER      :: APPLICATION
// PURPOSE    :: Handles Invite requests at UA node
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the request
// + sip       : SipData* : Pointer to SipData
// + sipMsg    : SipMsg* : Pointer to SipMsg
// RETURN     :: void : NULL
// **/
void SipHandleInviteRequestUA(Node* node, SipData* sip, SipMsg* sipMsg)
{
    SipSession* sipSession = new SipSession(atoi(sipMsg->SipGetCallInfo()),
                                            SIP_SERVER);//?????????

    ListInsert(node, sip->SipGetSipSessionList(), node->getNodeTime(),
        (void*)sipSession);

    sipSession->SipSetTo(sipMsg->SipGetTo());
    sipSession->SipSetFrom(sipMsg->SipGetFrom());
    sipSession->SipSetCallId(sipMsg->SipGetCallId());
    // set node transaction state
    sipSession->SipSetTransactionState(SIP_SERVER);//??????????

    AppMultimedia* multimedia = node->appData.multimedia;

    VoipHostData* voip = NULL;
    int numHostBits = NUMBER_OF_HOSTBITS;
    BOOL isNodeId = FALSE;
    SipCallModel callModel = sip->SipGetCallModel();

    SipCheckSession(sipSession, __FUNCTION__, node->nodeId);
    char* tmp = NULL;
    NodeAddress addr;
    memset(&addr , 0 , sizeof(NodeAddress));

    char contactHeader[MAX_STRING_LENGTH];
    memset(contactHeader, 0 , MAX_STRING_LENGTH);

    char ipAddrInContactHeader[IP_ADDRESS_LENGTH];
    memset(&ipAddrInContactHeader[0] , 0 , sizeof(ipAddrInContactHeader));

    ERROR_Assert(!strcmp(sip->SipGetOwnAliasAddr(), sipMsg->SipGetTo()),
                "Invalid destination address\n");

    sipSession->SipSetTransactionState(SIP_SERVER);

    if (sipSession->SipGetCallState() != SIP_IDLE)
    {
        ERROR_ReportWarning("UA busy. call cannot be established!\n");
        if (callModel == SipDirect)
        {
            APP_TcpCloseConnection(node, sipMsg->SipGetConnectionId());
        }
        else if (callModel == SipProxyRouted)
        {
            APP_TcpCloseConnection(node, sipMsg->SipGetProxyConnectionId());
        }
        sip->SipIncrementCallRejected();
        return;
    }

    // change call state to CALLING
    sipSession->SipSetCallState(SIP_CALLING);

    // increase the branchCounter at server
    sip->SipIncrementBranchCounter();

    strcpy(contactHeader , sipMsg->SipGetContact());

    tmp = strchr(contactHeader , '@' );

    strcpy( ipAddrInContactHeader , tmp+1 );

    IO_ParseNodeIdOrHostAddress(
        ipAddrInContactHeader, &addr, &isNodeId);
    // for direct call model get the voip for this node and
    // check it if NULL to detect REJECT setting
    if (callModel == SipDirect)
    {
        voip = VoipCallReceiverGetDataForConnectionId(node,
                        sipMsg->SipGetConnectionId());
        if (!voip)
        {
            // if voip not found, that indicates call cannot be established
            // one reason may be REJECT is selected during configuration
            if (DEBUG)
            {
                printf("Call is rejected. Connection closed.\n");
                printf("Node %u sends TCP close\n", node->nodeId);
            }

            sip->SipIncrementCallRejected();
            sipSession->SipSetTransactionState(SIP_NORMAL);
            sipSession->SipSetCallState(SIP_IDLE);
            APP_TcpCloseConnection(node, sipMsg->SipGetConnectionId());
            sip->SipDeleteMsg(sipMsg->SipGetConnectionId());
            return;
        }
        // knowing the initiator's IP Address from Contact header as 
        // per RFC : 3261
        voip->remoteAddr = addr;
        voip->callSignalAddr = addr;
        sipSession->SipSetConnectionId(sipMsg->SipGetConnectionId());

        // added by Li -begin

        sipSession->SipSetRtpPort(voip->initiatorPort);
        // added by Li -end
    }

    // to set voip's connID for the ProxyRouted Mode
    // for direct callMode, it is already set at passive open

    if (callModel == SipProxyRouted)
    {
        SipVia* tmpVia = sipMsg->SipGetSipVia();
        int numVia = sipMsg->SipGetViaCount();

        NodeAddress clientAddr = 0;
        unsigned short clientPort =
            (unsigned short)atoi(sipMsg->SipGetCallInfo());

        ERROR_Assert(tmpVia[numVia-1].isReceivedIp,"ClientIp not recd\n");

        IO_ParseNodeIdHostOrNetworkAddress(
            tmpVia[numVia - 1].receivedIp,
            &clientAddr,
            &numHostBits,
            &isNodeId);

        voip = VoipGetReceiverDataForCallSignal(node, clientPort);
        if (!voip)
        {
            // if voip not found, that indicates call cannot be established
            // one reason may be REJECT is selected during configuration

            if (DEBUG)
            {
                printf("Call is rejected. Connection closed.\n");
                printf("Node %u sends TCP close\n", node->nodeId);
            }

            sip->SipIncrementCallRejected();
            sipSession->SipSetTransactionState(SIP_NORMAL);
            sipSession->SipSetCallState(SIP_IDLE);
            APP_TcpCloseConnection(node, sipMsg->SipGetProxyConnectionId());
            sip->SipDeleteMsg(sipMsg->SipGetProxyConnectionId());
            return;
        }
        voip->proxyConnectionId = sipMsg->SipGetProxyConnectionId();

        sipSession->SipSetProxyConnectionId(sipMsg->SipGetProxyConnectionId());
        // knowing the initiator's IP Address from Contact header as 
        // per RFC : 3261
        voip->remoteAddr = addr;
        voip->callSignalAddr = addr;

        // added by Li -begin

        sipSession->SipSetRtpPort(voip->initiatorPort);

        // added by Li -end

    }

    // indicate to muos waveform
    // waiting for response to allocate pdp context

    // should wait for muos allocate services complete
#ifdef MUOS_LIB
    if (node->networkData.networkProtocol == CELLULAR)
    {

        if (sip->SipGetCallModel() == SipDirect)
        {
            // don't send the invite response here
            return MUOS_AllocateConnection(node,
                sip, voip, sipMsg, sipSession);
        } else if (sip->SipGetCallModel() == SipProxyRouted &&
            sipSession->SipGetCallState() == SIP_CALLING)
        {
            // don't send the invite response here
            return MUOS_AllocateConnection(node,
                sip, voip, sipMsg, sipSession);
        }
    }
#endif

    sipSession->SipSetTransactionState(SIP_SERVER);
    sipMsg->SipCreateResponsePkt(node, sip, RINGING, "INVITE");

    if (callModel == SipProxyRouted)
    {
        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            sipMsg->SipGetProxyConnectionId(),
            TRACE_SIP);

            APP_AddPayload(
                node,
                tcpmsg,
                sipMsg->SipGetBuffer(),
                sipMsg->SipGetLength());

            APP_TcpSend(node, tcpmsg);
    }
    else if (callModel == SipDirect)
    {
        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            sipMsg->SipGetConnectionId(),
            TRACE_SIP);

        APP_AddPayload(
            node,
            tcpmsg,
            sipMsg->SipGetBuffer(),
            sipMsg->SipGetLength());

        APP_TcpSend(node, tcpmsg);
    }

    MEM_free(sipMsg->SipGetBuffer());

    // change call state to RINGING
    sipSession->SipSetCallState(SIP_RINGING);

    sip->SipIncrementRingingSent();


    // The terminal receiving invite message fires a self timer
    // to send OK message after ConnectionDelay set in config file

    SipCallInfo sipCallInfo;
    strcpy(sipCallInfo.to, sipMsg->SipGetTo());
    strcpy(sipCallInfo.from, sipMsg->SipGetFrom());
    strcpy(sipCallInfo.callId, sipMsg->SipGetCallId());

    Message* delayMsg;
    delayMsg = MESSAGE_Alloc(node,
                             APP_LAYER,
                             APP_SIP,
                             MSG_APP_SipConnectionDelay);

    MESSAGE_InfoAlloc(node, delayMsg, sizeof(SipCallInfo));

    if (callModel == SipProxyRouted)
    {
        sipCallInfo.connectionId = sipMsg->SipGetProxyConnectionId();
    }
    else if (callModel == SipDirect)
    {
        sipCallInfo.connectionId = sipMsg->SipGetConnectionId();
    }

    memcpy(MESSAGE_ReturnInfo(delayMsg),
           &sipCallInfo,
           sizeof(SipCallInfo));
    sipSession->SipSetConnDelayTimerRunning(true);
    MESSAGE_Send(node, delayMsg, multimedia->connectionDelay);
}


// /**
// FUNCTION   :: SipHandleInviteRequestProxy
// LAYER      :: APPLICATION
// PURPOSE    :: Handles Invite requests at Proxy node
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the request
// + sip       : SipData* : Pointer to SipData
// + sipMsg    : SipMsg* : Pointer to SipMsg
// RETURN     :: void : NULL
// **/
void SipHandleInviteRequestProxy(Node* node, SipData* sip, SipMsg* sipMsg)
{
    SipSession* sipSession = new SipSession(atoi(sipMsg->SipGetCallInfo()));//?????????

    ListInsert(node, sip->SipGetSipSessionList(), node->getNodeTime(),
        (void*)sipSession);

    sipSession->SipSetTo(sipMsg->SipGetTo());
    sipSession->SipSetFrom(sipMsg->SipGetFrom());
    sipSession->SipSetCallId(sipMsg->SipGetCallId());
    // set node transaction state
    sipSession->SipSetTransactionState(SIP_SERVER);//??????????

    unsigned int targetIP = INVALID_ADDRESS;
    unsigned short srcPort;

    unsigned int targetId = 0;
    // send trying response to the caller
    sipMsg->SipCreateResponsePkt(node, sip, TRYING, "INVITE");
        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            sipMsg->SipGetConnectionId(),
            TRACE_SIP);

        APP_AddPayload(
            node,
            tcpmsg,
            sipMsg->SipGetBuffer(),
            sipMsg->SipGetLength());

        APP_TcpSend(node, tcpmsg);

    MEM_free(sipMsg->SipGetBuffer());
    sip->SipIncrementTryingSent();

    // ensure called node is not a proxy
    if ((sip->SipGetOwnAliasAddr())&&
        (!strcmp(sip->SipGetOwnAliasAddr(), sipMsg->SipGetTo())))
    {
        char err[MAX_STRING_LENGTH];
        sprintf(err, "called party %s not an END NODE/USER AGENT\n",
                sipMsg->SipGetTo());
        ERROR_ReportWarning(err);

        if (DEBUG)
        {
            printf("Node %u sends TCP close\n", node->nodeId);
        }

        // we cant init states, for a proxy
        APP_TcpCloseConnection(node, sipMsg->SipGetConnectionId());
        sip->SipDeleteMsg(sipMsg->SipGetConnectionId());
        return;
    }

    // search location service of this node to find callee
    // getting the callers id(not ip addr) on the basis of its domain name
    sip->SipGetHostId(sipMsg->SipGetTo(), targetId);

    if (targetId) // target host under this proxy
    {
        NodeAddress hostId;
        int found = FALSE;
        hostId = targetId;

        unsigned short i ;
        SipHostBlockList hostBlockList = sip->SipGetHostBlockList();
        SipHostBlock * hostBlock = hostBlockList.hostBlockHead;

        for (;hostBlock != NULL; hostBlock = hostBlock->next)
        {
            for (i = 0; i < hostBlock->hostCount; i++)
            {
                if (hostId == hostBlock->hostList[i].hostId)
                {
                    hostBlock->hostList[i].hostIp = 
                    MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
                                node,
                                hostBlock->hostList[i].hostId,
                                hostBlock->hostList[i].hostInterfaceIndex);

                    targetIP = hostBlock->hostList[i].hostIp;
                    found = TRUE;
                    break;
                }
            }

            if (found)
            {
                break;
            }
        }

        int interfaceId = NetworkGetInterfaceIndexForDestAddress(node,
                                                                 targetIP);

        NodeAddress proxyAddress = NetworkIpGetInterfaceAddress(node,
                                                                interfaceId);
        srcPort =(unsigned short)(sip->SipGetCallSignalPort());


        sipMsg->SipSetTargetIp(targetIP);
        sipMsg->SipSetFromPort(srcPort);

        Address localAddr;
        memset(&localAddr, 0, sizeof(Address));
        localAddr.networkType = NETWORK_IPV4;
        localAddr.interfaceAddr.ipv4 = proxyAddress;

        Address remoteAddr;
        memset(&remoteAddr, 0, sizeof(Address));
        remoteAddr.networkType = NETWORK_IPV4;
        remoteAddr.interfaceAddr.ipv4 = targetIP;
        std::string url;
        url.clear();
        node->appData.appTrafficSender->appTcpOpenConnection(
                                                    node,
                                                    APP_SIP,
                                                    localAddr,
                                                    srcPort,
                                                    remoteAddr,
                                                    (UInt16) APP_SIP,
                                                    srcPort,
                                                    APP_DEFAULT_TOS,
                                                    ANY_INTERFACE,
                                                    url,
                                                    0,
                                                    ANY_NODEID,
                                                    ANY_INTERFACE,
                                                    ANY_INTERFACE);

    }
    else // target host not under this proxy
    {
        // query the redirect server part for respective proxy IP
        sip->SipGetDestProxyIp(sipMsg->SipGetTo(), targetIP);

        char targetUrl[MAX_STRING_LENGTH];
        sip->SipGetDestProxyUrl(sipMsg->SipGetTo(), targetUrl);

        if (targetIP == INVALID_ADDRESS && targetUrl[0] == '\0')
        {
            char err[MAX_STRING_LENGTH];
            sprintf(err, "No route to the remote target Host!"
                "destination domain unknown: %s\n", sipMsg->SipGetTo());
            ERROR_ReportError(err);
        }
        if (targetIP == INVALID_ADDRESS && targetUrl[0] != '\0')//ProxyUrl is present
        {
            srcPort = (unsigned short)(sip->SipGetCallSignalPort());

            sipMsg->SipSetTargetUrl(targetUrl);
            sipMsg->SipSetTargetIp(targetIP); //its INVALID at this
                                             //point of time
            sipMsg->SipSetFromPort(srcPort);

            Address homeUnicastAdr;
            memset(&homeUnicastAdr,0,sizeof(Address));
            homeUnicastAdr.networkType = NETWORK_IPV4;
            homeUnicastAdr.interfaceAddr.ipv4 = sip->SipGetProxyIp();

            std::string url;
            url.assign(targetUrl);
            // Get the IP address of this proxy for the node who have 
            // initaited the call
            Address localAddr;
            memset(&localAddr, 0, sizeof(Address));
            localAddr.networkType = NETWORK_IPV4;
            localAddr.interfaceAddr.ipv4 = homeUnicastAdr.interfaceAddr.ipv4;

            Address remoteAddr;
            memset(&remoteAddr, 0, sizeof(Address));
            remoteAddr.networkType = NETWORK_IPV4;
            remoteAddr.interfaceAddr.ipv4 = INVALID_ADDRESS;
            remoteAddr.networkType = NETWORK_INVALID;

            node->appData.appTrafficSender->appTcpOpenConnection(
                                                node,
                                                APP_SIP,
                                                localAddr,
                                                srcPort,
                                                remoteAddr,
                                                (UInt16) APP_SIP,
                                                srcPort,
                                                APP_DEFAULT_TOS,
                                                ANY_INTERFACE,
                                                url,
                                                0,
                                                ANY_NODEID,
                                                ANY_INTERFACE,
                                                ANY_INTERFACE);

        }
        else //targetIP is present 
        {

        int interfaceId =
            NetworkGetInterfaceIndexForDestAddress(node, targetIP);
        NodeAddress proxyAdd = NetworkIpGetInterfaceAddress
                                                (node, interfaceId);

        unsigned short srcPort =
            (unsigned short)(sip->SipGetCallSignalPort());

        sipMsg->SipSetTargetIp(targetIP);
        sipMsg->SipSetFromPort(srcPort);

        Address localAddr;
        memset(&localAddr, 0, sizeof(Address));
        localAddr.networkType = NETWORK_IPV4;
        localAddr.interfaceAddr.ipv4 = proxyAdd;

        Address remoteAddr;
        memset(&remoteAddr, 0, sizeof(Address));
        remoteAddr.networkType = NETWORK_IPV4;
        remoteAddr.interfaceAddr.ipv4 = targetIP;
        std::string url;
        url.clear();
        node->appData.appTrafficSender->appTcpOpenConnection(
                                                        node,
                                                        APP_SIP,
                                                        localAddr,
                                                        srcPort,
                                                        remoteAddr,
                                                        (UInt16) APP_SIP,
                                                        srcPort,
                                                        APP_DEFAULT_TOS,
                                                        ANY_INTERFACE,
                                                        url,
                                                        0,
                                                        ANY_NODEID,
                                                        ANY_INTERFACE,
                                                        ANY_INTERFACE);
        }
    }
}


// /**
// FUNCTION   :: SipHandleInviteRequest
// LAYER      :: APPLICATION
// PURPOSE    :: Handles Invite request
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the request
// + sipMsg    : SipMsg* : Pointer to SipMsg
// RETURN     :: void : NULL
// **/
void SipHandleInviteRequest(Node* node, SipMsg* sipMsg)
{
    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;

    sip->SipIncrementInviteRecd();

    if (sip->SipGetHostType() == SIP_PROXY)
    {
        SipHandleInviteRequestProxy(node, sip, sipMsg);
    }
    else if (sip->SipGetHostType() == SIP_UA)
    {
        SipHandleInviteRequestUA(node, sip, sipMsg);
    }
}


// /**
// FUNCTION   :: SipHandleAckRequest
// LAYER      :: APPLICATION
// PURPOSE    :: Handles Ack request
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the request
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the request
// RETURN     :: void : NULL
// **/
void SipHandleAckRequest(Node* node, SipMsg* sipMsg)
{
    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;
    VoipHostData* voip = NULL;

    // ack handler must be a user agent
    ERROR_Assert(sip->SipGetHostType() == SIP_UA, "Invalid Ack handler\n");

    SipSession* sipSession = sip->SipGetSipSessionByToFromCallId(
                                sipMsg->SipGetTo(),
                                sipMsg->SipGetFrom(),
                                sipMsg->SipGetCallId());

    SipCheckSession(sipSession, __FUNCTION__, node->nodeId);

    if (sipSession->SipGetCallState() != SIP_OK)
    {
        ERROR_ReportWarning("UA is busy. Call cannot be established\n");
        APP_TcpCloseConnection(node, sipMsg->SipGetConnectionId());
        return;
    }

    sipSession->SipSetCallState(SIP_COMPLETED);
    sip->SipIncrementAckRecd();

    // get voip structure for data transmission
    voip = VoipCallReceiverGetDataForConnectionId(node,
                        sipMsg->SipGetConnectionId());
    ERROR_Assert(voip, "Invalid voip pointer in AckHandler");

    if (DEBUG)
    {
        printf("Callee node: %d,received ACK packet to OK \n"
               "STARTING CONVERSATION\n\n", node->nodeId);
    }
    VoipStartTransmission(node, voip);

    if (sip->SipGetCallModel() == SipProxyRouted)
    {
        // increase the branchCounter only for 2XXACK(TBD: only for 2XXACK)
        sip->SipIncrementBranchCounter();

        if (DEBUG)
        {
            printf("node: %u Closing down Proxy Path TCP connections for"
              " connectionId: %d\n", node->nodeId, voip->proxyConnectionId);
        }
        APP_TcpCloseConnection(node, voip->proxyConnectionId);

    }
}


// /**
// FUNCTION   :: SipHandleByeRequest
// LAYER      :: APPLICATION
// PURPOSE    :: Handles Bye request
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the request
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the request
// RETURN     :: void : NULL
// **/
void SipHandleByeRequest(Node* node, SipMsg* sipMsg)
{
    SipData* sip = (SipData*)node->appData.multimedia->sigPtr;

    SipSession* sipSession = sip->SipGetSipSessionByToFromCallId(
                                sipMsg->SipGetTo(),
                                sipMsg->SipGetFrom(),
                                sipMsg->SipGetCallId());

    SipCheckSession(sipSession, __FUNCTION__, node->nodeId);

    ERROR_Assert(sipSession->SipGetCallState() == SIP_COMPLETED,
                                    "Invalid call state\n");

    sipSession->SipSetCallState(SIP_TERMINATED);

    sip->SipIncrementByeRecd();

    // increase the branchCounter
    sip->SipIncrementBranchCounter();

    sipMsg->SipCreateResponsePkt(node, sip, OK, "BYE");

    Message *tcpmsg = APP_TcpCreateMessage(
        node,
        sipMsg->SipGetConnectionId(),
        TRACE_SIP);

    APP_AddPayload(
        node,
        tcpmsg,
        sipMsg->SipGetBuffer(),
        sipMsg->SipGetLength());

    APP_TcpSend(node, tcpmsg);

    MEM_free(sipMsg->SipGetBuffer());
}


// /**
// FUNCTION   :: SipHandleTryingResponse
// LAYER      :: APPLICATION
// PURPOSE    :: Handles Trying Response
// PARAMETERS ::
// + node      : Node* : Pointer to node receiving the response
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the response
// RETURN     :: void : NULL
// **/
void SipHandleTryingResponse(Node* node, SipMsg* sipMsg)
{
    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;

    sip->SipIncrementTryingRecd();

    SipSession* sipSession = sip->SipGetSipSessionByToFromCallId(
                                sipMsg->SipGetTo(),
                                sipMsg->SipGetFrom(),
                                sipMsg->SipGetCallId());

    SipCheckSession(sipSession, __FUNCTION__, node->nodeId);

    if (sip->SipGetHostType() == SIP_PROXY)
    {
        // TBD: To stop retransmission in case of UDP
    }
    else if (sip->SipGetHostType() == SIP_UA)
    {
        if (sipSession->SipGetCallState() != SIP_CALLING)
        {
            ERROR_ReportWarning("UA not in CALLING state."
            "Call can't proceed\n");
            APP_TcpCloseConnection(node, sipMsg->SipGetConnectionId());
            return;
        }
        sipSession->SipSetCallState(SIP_TRYING);
        // TBD: To stop retransmission in case of UDP
    }
}


// /**
// FUNCTION   :: SipHandleRingingResponse
// LAYER      :: APPLICATION
// PURPOSE    :: Handles Ringing Response
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the response
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the response
// RETURN     :: void : NULL
// **/
void SipHandleRingingResponse(Node* node, SipMsg* sipMsg)
{
    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;
    sip->SipIncrementRingingRecd();

    SipSession* sipSession = sip->SipGetSipSessionByToFromCallId(
                                sipMsg->SipGetTo(),
                                sipMsg->SipGetFrom(),
                                sipMsg->SipGetCallId());

    SipCheckSession(sipSession, __FUNCTION__, node->nodeId);

    if (sip->SipGetHostType() == SIP_PROXY)
    {
        int maxFwd = sipMsg->SipGetMaxForwards();

        // drop packet and send back error response
        if (maxFwd <= 0)
        {
            sip->SipIncrementRespDropped();
            sipMsg->SipCreateResponsePkt(node, sip, TOO_MANY_HOPS, "INVITE");

            // send in reverse direction
            Message *tcpmsg = APP_TcpCreateMessage(
                node,
                sipMsg->SipGetProxyConnectionId(),
                TRACE_SIP);

            APP_AddPayload(
                node,
                tcpmsg,
                sipMsg->SipGetBuffer(),
                sipMsg->SipGetLength());

            APP_TcpSend(node, tcpmsg);

            MEM_free(sipMsg->SipGetBuffer());
            return;
        }

        // forward to the prev hop thr existing TCP link
        sipMsg->SipForwardResponsePkt(node, RINGING);

        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            sipMsg->SipGetConnectionId(),
            TRACE_SIP);

        APP_AddPayload(
            node,
            tcpmsg,
            sipMsg->SipGetBuffer(),
            sipMsg->SipGetLength());

        APP_TcpSend(node, tcpmsg);

        MEM_free(sipMsg->SipGetBuffer());

    }
    else if (sip->SipGetHostType() == SIP_UA)
    {
        if ((sipSession->SipGetCallState() != SIP_CALLING) &&
            (sipSession->SipGetCallState() != SIP_TRYING))
        {
            ERROR_ReportWarning("UA is neither in CALLING/TRYING state, "
            "Call cannot proceed further\n");
            APP_TcpCloseConnection(node, sipMsg->SipGetProxyConnectionId());
            return;
        }
        sipSession->SipSetCallState(SIP_RINGING);
    }
}


// /**
// FUNCTION   :: SipHandleOKResponseProxy
// LAYER      :: APPLICATION
// PURPOSE    :: Handles OK response(Proxy node)
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the response
// + sip       : SipData* : Pointer to SipData
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the response
// RETURN     :: void : NULL
// **/
void SipHandleOKResponseProxy(Node* node, SipData* sip, SipMsg* sipMsg)
{
    int maxFwd = sipMsg->SipGetMaxForwards();

    // drop packet and send back error response
    if (maxFwd <= 0)
    {
        sip->SipIncrementRespDropped();
        sipMsg->SipCreateResponsePkt(node, sip, TOO_MANY_HOPS, "INVITE");

        // send in reverse direction
        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            sipMsg->SipGetProxyConnectionId(),
            TRACE_SIP);

        APP_AddPayload(
            node,
            tcpmsg,
            sipMsg->SipGetBuffer(),
            sipMsg->SipGetLength());

        APP_TcpSend(node, tcpmsg);

        MEM_free(sipMsg->SipGetBuffer());
        return;
    }
    // forward to the next hop thr existing TCP link
    sipMsg->SipForwardResponsePkt(node, OK);

    Message *tcpmsg = APP_TcpCreateMessage(
        node,
        sipMsg->SipGetConnectionId(),
        TRACE_SIP);

    APP_AddPayload(
        node,
        tcpmsg,
        sipMsg->SipGetBuffer(),
        sipMsg->SipGetLength());

    APP_TcpSend(node, tcpmsg);

    MEM_free(sipMsg->SipGetBuffer());
}


// /**
// FUNCTION   :: SipHandleOKResponseUADirect
// LAYER      :: APPLICATION
// PURPOSE    :: Handles OK response(UA node)
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the response
// + sip       : SipData* : Pointer to SipData
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the response
// RETURN     :: void : NULL
// **/
void SipHandleOKResponseUADirect(Node* node, SipData* sip, SipMsg* sipMsg)
{
    VoipHostData* voip = NULL;

    SipSession* sipSession = sip->SipGetSipSessionByToFromCallId(
                                sipMsg->SipGetTo(),
                                sipMsg->SipGetFrom(),
                                sipMsg->SipGetCallId());

    SipCheckSession(sipSession, __FUNCTION__, node->nodeId);

    if (!strcmp(sipMsg->SipGetCSeqMsg(), "INVITE"))
    {
        if (sipSession->SipGetCallState() != SIP_RINGING)
        {
            ERROR_ReportWarning("UA not in RINGING state, "
             "Call cannot proceed further\n");
            sipSession->SipSetTransactionState(SIP_NORMAL);
            sipSession->SipSetCallState(SIP_IDLE);
            APP_TcpCloseConnection(node, sipMsg->SipGetConnectionId());
            return;
        }
        char* tmp = NULL;
        BOOL isNodeId = FALSE ; 
        NodeAddress addr;
        memset(&addr , 0 , sizeof(NodeAddress));

        char contactHeader[MAX_STRING_LENGTH];
        memset(contactHeader, 0 , MAX_STRING_LENGTH);

        char ipAddrInContactHeader[IP_ADDRESS_LENGTH];
        memset(&ipAddrInContactHeader[0] , 0 , sizeof(ipAddrInContactHeader));

        strcpy(contactHeader , sipMsg->SipGetContact());

        tmp = strchr(contactHeader , '@' );

        strcpy( ipAddrInContactHeader , tmp + 1 );

        IO_ParseNodeIdOrHostAddress(
                                    ipAddrInContactHeader,
                                    &addr,
                                    &isNodeId);

        voip = VoipCallInitiatorGetDataForConnectionId(
                                node, sipMsg->SipGetConnectionId());
        ERROR_Assert(voip, "Invalid voip structure\n");

        if (voip->endTime < node->getNodeTime())
        {
            ERROR_ReportWarning("Call already terminated by Initiator\n");
            VoipCloseSession(node, voip);
            voip->connectionId = INVALID_ID;
            sipSession->SipSetTransactionState(SIP_NORMAL);
            sipSession->SipSetCallState(SIP_IDLE);
            APP_TcpCloseConnection(node, sipMsg->SipGetConnectionId());
            return;
        }
        // knowing the receiver's IP Address from Contact header as 
        // per RFC : 3261
        voip->remoteAddr = addr;
        sipSession->SipSetCallState(SIP_OK);
        sipMsg->SipCreate2xxAckRequestPkt(node, sip);

        Message *tcpmsg = APP_TcpCreateMessage(
            node,
            sipMsg->SipGetConnectionId(),
            TRACE_SIP);

        APP_AddPayload(
            node,
            tcpmsg,
            sipMsg->SipGetBuffer(),
            sipMsg->SipGetLength());

        APP_TcpSend(node, tcpmsg);

        MEM_free(sipMsg->SipGetBuffer());

        sipSession->SipSetCallState(SIP_COMPLETED);

        if (DEBUG)
        {
            printf("Informing voip application of Successful\n"
             "Call Establishment by VoipConnectionEstablished()\n\n");
        }
        VoipConnectionEstablished(node, voip);
    }
    else if (!strcmp(sipMsg->SipGetCSeqMsg(), "BYE"))
    {
        if (DEBUG)
        {
            printf("Node: %u Closing down the TCP connectionID: %d\n\n",
                    node->nodeId, sipMsg->SipGetConnectionId());
        }

        if (sipSession->SipGetTransactionState() == SIP_CLIENT)
        {
            voip = VoipCallInitiatorGetDataForConnectionId(
                    node, sipMsg->SipGetConnectionId());
        }
        else if (sipSession->SipGetTransactionState() == SIP_SERVER)
        {
            voip = VoipCallReceiverGetDataForConnectionId(
                    node, sipMsg->SipGetConnectionId());
        }

        VoipCloseSession(node, voip);
        voip->connectionId = INVALID_ID;
        sipSession->SipSetTransactionState(SIP_NORMAL);
        sipSession->SipSetCallState(SIP_IDLE);
        APP_TcpCloseConnection(node, sipMsg->SipGetConnectionId());
    }
}


// /**
// FUNCTION   :: SipHandleOKResponseUAProxyRouted
// LAYER      :: APPLICATION
// PURPOSE    :: Handles OK response(UA node) for Proxy routed call
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the response
// + sip       : SipData* : Pointer to SipData
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the response
// RETURN     :: void : NULL
// **/
void SipHandleOKResponseUAProxyRouted(Node* node, SipData* sip,
                                      SipMsg* sipMsg)
{
    VoipHostData* voip = NULL;

    SipSession* sipSession = sip->SipGetSipSessionByToFromCallId(
                                sipMsg->SipGetTo(),
                                sipMsg->SipGetFrom(),
                                sipMsg->SipGetCallId());

    SipCheckSession(sipSession, __FUNCTION__, node->nodeId);

    if (!strcmp(sipMsg->SipGetCSeqMsg(), "INVITE"))
    {
        voip = VoipCallInitiatorGetDataForProxyConnectionId(
                         node, sipMsg->SipGetProxyConnectionId());
        ERROR_Assert(voip, "Invalid voip structure\n");

        if (voip->endTime < node->getNodeTime())
        {
            ERROR_ReportWarning("Call already terminated by Initiator\n");
            VoipCloseSession(node, voip);
            voip->connectionId = INVALID_ID;
            sipSession->SipSetTransactionState(SIP_NORMAL);
            sipSession->SipSetCallState(SIP_IDLE);
            APP_TcpCloseConnection(node, sipMsg->SipGetProxyConnectionId());
            return;
        }
        char* tmp = NULL;
        BOOL isNodeId = FALSE;

        NodeAddress addr;
        memset(&addr , 0 , sizeof(NodeAddress));

        char contactHeader[MAX_STRING_LENGTH];
        memset(contactHeader, 0 , MAX_STRING_LENGTH);

        char ipAddrInContactHeader[IP_ADDRESS_LENGTH];
        memset(ipAddrInContactHeader , 0 , sizeof(ipAddrInContactHeader));

        strcpy(contactHeader , sipMsg->SipGetContact());
        tmp = strchr( contactHeader , '@' );

        strcpy( ipAddrInContactHeader , tmp + 1);

        IO_ParseNodeIdOrHostAddress(ipAddrInContactHeader,
                                    &addr,
                                    &isNodeId);

        // knowing the receiver's IP Address from Contact header as per
        // RFC : 3261
        voip->remoteAddr = addr;

        Address localAddr;
        memset(&localAddr, 0, sizeof(Address));
        localAddr.networkType = NETWORK_IPV4;
        localAddr.interfaceAddr.ipv4 = voip->callSignalAddr;

        Address remoteAddr;
        memset(&remoteAddr, 0, sizeof(Address));
        remoteAddr.networkType = NETWORK_IPV4;
        remoteAddr.interfaceAddr.ipv4 = voip->remoteAddr;
        std::string url;
        url.clear();
        node->appData.appTrafficSender->appTcpOpenConnection(
                                                node,
                                                APP_SIP,
                                                localAddr,
                                                voip->callSignalPort,
                                                remoteAddr,
                                                (UInt16) APP_SIP,
                                                voip->callSignalPort,
                                                APP_DEFAULT_TOS,
                                                ANY_INTERFACE,
                                                url,
                                                0,
                                                ANY_NODEID,
                                                ANY_INTERFACE,
                                                ANY_INTERFACE);

        AppVoipClientAddAddressInformation(node,
                                           voip,
                                           voip->remoteAddr);

        sipSession->SipSetCallState(SIP_OK);
        VoipConnectionEstablished(node, voip);
    }
    else if (!strcmp(sipMsg->SipGetCSeqMsg(), "BYE"))
    {
        if (sip->SipGetTransProtocolType() == TCP)
        {
            // get access to voip
            if (sipSession->SipGetTransactionState() == SIP_CLIENT)
            {
                voip = VoipCallInitiatorGetDataForConnectionId(
                        node, sipMsg->SipGetConnectionId());
            }
            else if (sipSession->SipGetTransactionState() == SIP_SERVER)
            {
                voip = VoipCallReceiverGetDataForConnectionId(
                        node, sipMsg->SipGetConnectionId());
            }
            ERROR_Assert(voip, "Invalid voip structure\n");

            // set the voip connectionId invalid
            voip->connectionId = INVALID_ID;
            voip->proxyConnectionId = INVALID_ID;
            VoipCloseSession(node, voip);

            sipSession->SipSetTransactionState(SIP_NORMAL);
            sipSession->SipSetCallState(SIP_IDLE);
            APP_TcpCloseConnection(node, sipMsg->SipGetConnectionId());
            sip->SipDeleteMsg(sipMsg->SipGetConnectionId());

            if (DEBUG)
            {
                printf("Node: %u Closing down the TCP "
                       "connectionID: %d\n\n",
                        node->nodeId, sipMsg->SipGetConnectionId());
            }
        }
        else if (sip->SipGetTransProtocolType() == UDP)
        {
            // do necessary handling if any
        }
    }
}


// /**
// FUNCTION   :: SipHandleOKResponseUA
// LAYER      :: APPLICATION
// PURPOSE    :: Handles OK response(UA node)
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the response
// + sip       : SipData* : Pointer to SipData
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the response
// RETURN     :: void : NULL
// **/
void SipHandleOKResponseUA(Node* node, SipData* sip, SipMsg* sipMsg)
{
    if (sip->SipGetCallModel() == SipDirect)
    {
        SipHandleOKResponseUADirect(node, sip, sipMsg);
    }
    else if (sip->SipGetCallModel() == SipProxyRouted)
    {
        SipHandleOKResponseUAProxyRouted(node, sip, sipMsg);
    }
}


// /**
// FUNCTION   :: SipHandleOKResponse
// LAYER      :: APPLICATION
// PURPOSE    :: Handles OK response
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the response
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the response
// RETURN     :: void : NULL
// **/
void SipHandleOKResponse(Node* node, SipMsg* sipMsg)
{
    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;

    sip->SipIncrementOkRecd();

    if (sip->SipGetHostType() == SIP_PROXY)
    {
        SipHandleOKResponseProxy(node, sip, sipMsg);
    }
    else if (sip->SipGetHostType() == SIP_UA)
    {
        SipHandleOKResponseUA(node, sip, sipMsg);
    }
}


// /**
// FUNCTION   :: SipHandleTooManyHopResponse
// LAYER      :: APPLICATION
// PURPOSE    :: Handles TooManyHops response
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the response
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the response
// RETURN     :: void : NULL
// **/
void SipHandleTooManyHopResponse(Node* node, SipMsg* sipMsg)
{
    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;
    ERROR_Assert(sip->SipGetCallModel() == SipProxyRouted,
        "Error, Control must not come here for direct call\n");

    if (sip->SipGetHostType() == SIP_UA)
    {
        APP_TcpCloseConnection(node, sipMsg->SipGetProxyConnectionId());
    }
    else if (sip->SipGetHostType() == SIP_PROXY)
    {
        // TBD: When extended for multiple hops, first forward the packet
        // thr the mirConnId and then close the connId through which the
        // msg had been received.
    }
}


// /**
// FUNCTION   :: SipHandleRequestPacket
// LAYER      :: APPLICATION
// PURPOSE    :: Handles Request packet receipt
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the packet
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the packet
// RETURN     :: void : NULL
// **/
void SipHandleRequestPacket(Node* node, SipMsg* sipMsg)
{
    switch (sipMsg->SipGetReqType())
    {
        case INV:
            SipHandleInviteRequest(node, sipMsg);
            break;

        case ACK:
            SipHandleAckRequest(node, sipMsg);
            break;

        case BYE:
            SipHandleByeRequest(node, sipMsg);
            break;

        case OPT:
            // To be incorporated in future.
            break;

        case CAN:
            // To be incorporated in future.
            break;

        case REG:
            // To be incorporated in future.
            break;

        default:
            break;
    }
}


// /**
// FUNCTION   :: SipHandleResponsePacket
// LAYER      :: APPLICATION
// PURPOSE    :: Handles Response packet receipt
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the packet
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the packet
// RETURN     :: void : NULL
// **/
void SipHandleResponsePacket(Node* node, SipMsg* sipMsg)
{
    switch (sipMsg->SipGetResType())
    {
        case TRYING:
            SipHandleTryingResponse(node, sipMsg);
            break;

        case RINGING:
            SipHandleRingingResponse(node, sipMsg);
            break;

        case OK:
            SipHandleOKResponse(node, sipMsg);
            break;

        case TOO_MANY_HOPS:
            SipHandleTooManyHopResponse(node, sipMsg);
            break;

        default:
            break;
    }
}


// /**
// FUNCTION   :: SipHandleReceivedPacket
// LAYER      :: APPLICATION
// PURPOSE    :: Handles packet receipt
// PARAMETERS ::
// + node      : Node* : Pointer to the node receiving the packet
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the packet
// RETURN     :: void : NULL
// **/
void SipHandleReceivedPacket(Node* node, Message* msg)
{
    if (DEBUG)
    {
        printf("\nNode - %d,Recd packetLength: %d\n",node->nodeId,
        MESSAGE_ReturnPacketSize(msg));
    }

    SipData* sip = (SipData*)node->appData.multimedia->sigPtr;
    TransportToAppDataReceived *dataRecvd =
     (TransportToAppDataReceived *)MESSAGE_ReturnInfo(msg);

    CircularBuffer* cirBuf =
        SipGetCirBufForConnId(sip,(unsigned short)dataRecvd->connectionId);

    Int32 pktSize = 0;
    if (!cirBuf || (!cirBuf->getCount(pktSize, idRead)))
    {
        return;
    }

    unsigned char* localBuf = (unsigned char*) MEM_malloc(pktSize);
    memset(localBuf, 0, pktSize);
    if (cirBuf->read(localBuf) == FALSE)
    {
         MEM_free(localBuf);
         return;
    }

    // dataLen zero implies an incomplete message
    unsigned dataLen = SipIsCompleteMsg(localBuf,(unsigned short)pktSize);
    if (!dataLen)
    {
        MEM_free(localBuf);
        return;
    }

#if DEBUG
    printf("%" TYPES_64BITFMT "d Node %d receiving SIP\n", node->getNodeTime(), node->nodeId);
    printf("SIP buffer is\n");
    cirBuf->dumpToStdout();
#endif

    unsigned char* data = (unsigned char*) MEM_malloc(dataLen);
    memset(data, 0, dataLen);
    cirBuf->readFromBuffer(data, dataLen);

#if DEBUG
    printf("Read is\n");
    for (int i = 0; i < dataLen; i++) printf("%c", data[i]);
    printf("\n");
    printf("SIP buffer is\n");
    cirBuf->dumpToStdout();
    printf("\n"); fflush(stdout);
#endif

    SipMsg* sipMsg = new SipMsg();
    sipMsg->SipParseMsg((char*)data, dataLen);

    SipMsg* oldMsg = sip->SipFindMsg(sipMsg);

    // for cases where the oldmsg doesn't have fromTag, toTag, callId
    // such as where sipMsg acts as a carrier of fromIp

    if (!oldMsg)
    {
        oldMsg = sip->SipFindMsg(dataRecvd->connectionId);
    }

    sipMsg->SipSetProperConnectionId(sip, dataRecvd->connectionId, oldMsg);
    if (oldMsg)
    {
        if (oldMsg->SipGetConnectionId() > 0)
        {
            sip->SipDeleteMsg(oldMsg->SipGetConnectionId());
        }
        else if (oldMsg->SipGetProxyConnectionId() > 0)
        {
            sip->SipDeleteMsg(oldMsg->SipGetProxyConnectionId());
        }
    }
    sip->SipInsertMsg(sipMsg);

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        char msgBuf[MAX_SIP_PACKET_SIZE] = {0};
        memcpy(msgBuf, sipMsg->SipGetBuffer(), sipMsg->SipGetLength());
        printf("Node: %d receives following message at: %s -\n%s\n",
            node->nodeId, clockStr, msgBuf);

        printf("\n");
    }

    switch (sipMsg->SipGetMsgType())
    {
        case SIP_REQUEST:
            SipHandleRequestPacket(node, sipMsg);
            break;

        case SIP_RESPONSE:
            SipHandleResponsePacket(node, sipMsg);
            break;

        default:
            ERROR_Assert(FALSE, "Invalid message type\n");
    }
    MEM_free(localBuf);
    MEM_free(data);

    if (cirBuf->getCount(pktSize, idRead))
    {
        SipHandleReceivedPacket(node, msg);
    }
}


// /**
// FUNCTION   :: SipHandleCloseResult
// LAYER      :: APPLICATION
// PURPOSE    :: Handles TCP connection close event
// PARAMETERS ::
// + node      : Node* : Pointer to the node closing the connection
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the event
// RETURN     :: void : NULL
// **/
void SipHandleCloseResult(Node* node, Message* msg)
{
    SipData* sip = (SipData*)node->appData.multimedia->sigPtr;
    SipHostType hostType = sip->SipGetHostType();

    TransportToAppCloseResult* closeResult =
            (TransportToAppCloseResult *) MESSAGE_ReturnInfo(msg);



    if (DEBUG)
    {
        char buf[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(), buf);

        char buf2[MAX_STRING_LENGTH];
        if (hostType == SIP_UA)
        {
            strcpy(buf2, "UA");
        }
        else if (hostType == SIP_PROXY)
        {
            strcpy(buf2, "PROXY");
        }

        printf("\n%s Node: %u Got CloseResult call at %s on closing down TCP"
        " ConnID: %d\n", buf2,node->nodeId,buf, closeResult->connectionId);
    }

    if (hostType == SIP_UA)
    {
        VoipHostData* voip = NULL;

        SipSession* sipSession = sip->SipGetSipSessionByConnectionId(
            closeResult->connectionId);

        if (!sipSession)
        {
            // session will not exist in case of REJECT calls on the
            // receiver, so just return
            return;
        }
        else
        {
            SipCheckSession(sipSession, __FUNCTION__, node->nodeId);
        }

        SipTransactionState transState = sipSession->SipGetTransactionState();

        // find voip for this node
        if (transState == SIP_CLIENT)
        {
            voip = VoipCallInitiatorGetDataForConnectionId(
                        node, closeResult->connectionId);
        }
        else if (transState == SIP_SERVER)
        {
            voip = VoipCallReceiverGetDataForConnectionId(
                        node, closeResult->connectionId);
        }

        if (voip)
        {
            // this is the case of actual closing of direct model
            // or direct path of proxy routed model

            voip->connectionId = INVALID_ID;
            VoipCloseSession(node, voip);
            sip->SipDeleteMsg(closeResult->connectionId);
            sipSession->SipSetTransactionState(SIP_NORMAL);
            sipSession->SipSetCallState(SIP_IDLE);

            //cancel the connection delay timer running after
            //its expiry, the 200 OK will be sent
            sipSession->SipSetConnDelayTimerRunning(false);
        }
        else
        {
            // this is the case of normal closing of proxy routed path
            // after the establishment of direct path(No state change here)

            if (transState == SIP_CLIENT)
            {
                voip = VoipCallInitiatorGetDataForProxyConnectionId(
                            node, closeResult->connectionId);
            }
            else if (transState == SIP_SERVER)
            {
                voip = VoipCallReceiverGetDataForProxyConnectionId(
                            node, closeResult->connectionId);
            }

            if (voip)
            {
                voip->proxyConnectionId = INVALID_ID;

                // for any errorneous condition before call establishment,
                // the node's state also must be initialised for proxy
                // routed close

                if (voip->status == VOIP_SESSION_NOT_ESTABLISHED)
                {
                    sipSession->SipSetTransactionState(SIP_NORMAL);
                    sipSession->SipSetCallState(SIP_IDLE);
                    sip->SipDeleteMsg(closeResult->connectionId);

                    //cancel the connection delay timer running after
                    //its expiry, the 200 OK will be sent
                    sipSession->SipSetConnDelayTimerRunning(false);

                }
            }
        }
    }
    else if (hostType == SIP_PROXY)
    {
        // needs to close the other side in case of proxy routed
        int mirConnId = sip->SipGetMirrorConnectionId(
                        closeResult->connectionId);

        // to handle cases in which proxy decides not to proceed
        // s.a. toomanyhops
        if (mirConnId > 0)
        {
            APP_TcpCloseConnection(node, mirConnId);
        }
        sip->SipDeleteMsg(closeResult->connectionId);
    }

    // whatever may be the scenario, if the TCP connection is
    // closed, the cir. buffer associated must also be close.
    SipDeleteCirBuf(sip, closeResult->connectionId);

    // if the TCP connection is passive close, then we need to initiate
    // the close conection process from our side too
    if (closeResult->type == TCP_CONN_PASSIVE_CLOSE)
    {
        APP_TcpCloseConnection(node, closeResult->connectionId);
    }
}


// /**
// FUNCTION   :: SipHandleCallTimeOut
// LAYER      :: APPLICATION
// PURPOSE    :: Handles Call Timeout event
// PARAMETERS ::
// + node      : Node* : Pointer to the node at which call times out
// + sipMsg    : SipMsg* : Pointer to SipMsg carrying the event
// RETURN     :: void : NULL
// **/
void SipHandleCallTimeOut(Node* node, Message* msg)
{
    VoipHostData* voip = NULL;
    int connId = *((int*)(MESSAGE_ReturnInfo(msg)));

    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;

    SipSession* sipSession = sip->SipGetSipSessionByConnectionId(connId);

    SipCheckSession(sipSession, __FUNCTION__, node->nodeId);

    SipCallState callState = sipSession->SipGetCallState();

    if (!(callState == SIP_CALLING || callState == SIP_TRYING  ||
          callState == SIP_RINGING))
    {
        // nothing to do
        return;
    }

    if (sip->SipGetCallModel() == SipDirect)
    {
        voip = VoipCallInitiatorGetDataForConnectionId(
                        node, connId);
    }
    else if (sip->SipGetCallModel() == SipProxyRouted)
    {
        voip = VoipCallInitiatorGetDataForProxyConnectionId(
                        node, connId);
    }

    if (voip)
    {
        // genuine case of timeout, release this call alongwith associated
        // tcp connections, inform human interface

        char buf[MAX_STRING_LENGTH];
        sprintf(buf, "NODE-%d:NO ANSWER FROM REMOTE END, CALL RELEASED"
            " ON TIMEOUT\n", node->nodeId);

        ERROR_ReportWarning(buf);
        if (DEBUG)
        {
            printf("calling nodeId: %d, called party: %s\n",
                        node->nodeId, voip->destAliasAddr);
        }
        sip->SipIncrementCallRejected();
        voip->connectionId = INVALID_ID;
        VoipCloseSession(node, voip);
        sip->SipDeleteMsg(connId);
        sipSession->SipSetTransactionState(SIP_NORMAL);
        sipSession->SipSetCallState(SIP_IDLE);
        APP_TcpCloseConnection(node, connId);
    }
}

// /**
// FUNCTION   :: SipProcessEvent
// LAYER      :: APPLICATION
// PURPOSE    :: Handles the SIP related events generated during simulation
// PARAMETERS ::
// + node      : Node* : Pointer to the node that receives the event
// + msg       : Message* : Pointer to the received message
// RETURN     :: void : NULL
// **/
void SipProcessEvent(Node* node, Message* msg)
{
    switch (MESSAGE_GetEvent(msg))
    {
        case MSG_APP_CELLULAR_SipCallRejected:
        case MSG_APP_CELLULAR_ServResourcesAlloc:
        {
#ifdef MUOS_LIB
            MUOS_SipVoiceCallControllerEvent(node, msg);
#endif
            break;
        }
        case MSG_APP_SipConnectionDelay:
        {
            SipHandleConnectionDelayTimer(node, msg);
            break;
        }
        case MSG_APP_SipCallTimeOut:
        {
            SipHandleCallTimeOut(node, msg);
            break;
        }
        case MSG_APP_FromTransOpenResult:
        {
             // TCP connection successful
            SipProcessOpenResult(node, msg);
            break;
        }
        case MSG_APP_FromTransDataSent:
        {
            break;
        }
        case MSG_APP_FromTransDataReceived:
        {
            // data packet recd from transport layer
            SipWriteToCirBufVector(node, msg);
            SipHandleReceivedPacket(node, msg);
            break;
        }
        case MSG_APP_FromTransCloseResult:
        {
            // TCP close connection msg received by SIP
            SipHandleCloseResult(node, msg);
            break;
        }
        case MSG_APP_FromTransListenResult:
        {
            break;
        }
        case MSG_APP_TimerExpired:
        {
            // now start listening at the updated address if
            // in valid address state
            Address serverAddress;
            if (AppTcpValidServerAddressState(node, msg, &serverAddress))
            {
                SIPUAInit(node, serverAddress);
            }
            break;
        }
        default:
        {
#ifndef EXATA
            ERROR_Assert(FALSE, "Unknown message\n");
#endif
            break;
        }
    }
    MESSAGE_Free(node, msg);
}


// /**
// FUNCTION   :: SipInitiateConnection
// LAYER      :: APPLICATION
// PURPOSE    :: Interface, for the third party application(now VoIP)
// using which a SIP node is informed of desire for call initiation
// PARAMETERS ::
// + node      : Node* : Pointer to the calling node
// + voipData  : void* : Pointer to calling application's data struct.
// RETURN     :: void : NULL
// **/
void SipInitiateConnection(Node* node, void* voip)
{
    NodeId destNodeId = 0;
    Int16 clientInterfaceIndex = ANY_INTERFACE;
    Int16 destInterfaceIndex = ANY_INTERFACE; 

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("\nnode: %u recd SipInitiateConnection from voip at %s\n",
               node->nodeId, clockStr);
    }

    VoipHostData* voipData = (VoipHostData*) voip;
    ERROR_Assert(voipData, "Invalid call initiator\n");

    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;

    // Exit if the initiator is a proxy node
    if (sip->SipGetHostType() == SIP_PROXY)
    {
        ERROR_ReportError("Proxy node cannot initiate a call\n");
    }

    SipSession* sipSession = new SipSession(voipData->callSignalPort,
                                            SIP_CLIENT);
    ListInsert(node, sip->SipGetSipSessionList(), node->getNodeTime(),
        (void*)sipSession);


    if (sip->SipGetCallModel() == SipDirect)
    {
        if (sip->SipGetTransProtocolType() == TCP)
        {
            Address localAddr;
            memset(&localAddr, 0, sizeof(Address));
            localAddr.networkType = NETWORK_IPV4;
            localAddr.interfaceAddr.ipv4 = voipData->callSignalAddr;
            sip->SipSetCallSignalAddr(voipData->callSignalAddr);

            Address remoteAddr;
            memset(&remoteAddr, 0, sizeof(Address));
            remoteAddr.networkType = NETWORK_IPV4;
            remoteAddr.interfaceAddr.ipv4 = voipData->remoteAddr;
            std::string url;
            url.clear();
            node->appData.appTrafficSender->appTcpOpenConnection(
                  node,
                  APP_SIP,
                    localAddr,
                    voipData->callSignalPort,
                    remoteAddr,
                    (UInt16) APP_SIP,
                    voipData->callSignalPort,
                    APP_DEFAULT_TOS,
                    ANY_INTERFACE,
                    url,
                    0,
                    voipData->destNodeId,
                    voipData->clientInterfaceIndex,
                    voipData->destInterfaceIndex);

        }
        else if (sip->SipGetTransProtocolType() == UDP)
        {
            // handle later
        }
    }
    else if (sip->SipGetCallModel() == SipProxyRouted)
    {
         if (sip->SipGetTransProtocolType() == TCP)
         {
             if (DEBUG_APP)
             {
             char srcAddrStr[MAX_STRING_LENGTH];
             IO_ConvertIpAddressToString((voipData->callSignalAddr), srcAddrStr);
             char dstAddrStr[MAX_STRING_LENGTH];
             IO_ConvertIpAddressToString((sip->SipGetProxyIp()), dstAddrStr);
             printf("connection to open for %s [%d] -> %s [%d] \n",
                 srcAddrStr, voipData->callSignalPort,
                 dstAddrStr, APP_SIP);
             }

            if (sip->SipGetCallModel() == SipProxyRouted)
            {
                destNodeId = voipData->proxyNodeId;
                destInterfaceIndex = voipData->proxyInterfaceIndex;
                SipSession* sipSession = sip->SipGetSipSessionByLocalPort(
                                                voipData->callSignalPort);

                if (sipSession && (sipSession->SipGetCallState() == SIP_OK))
                {
                    destNodeId = voipData->destNodeId;
                    destInterfaceIndex = voipData->destInterfaceIndex;
                }
            }
            clientInterfaceIndex = voipData->clientInterfaceIndex;

            Address localAddr;
            memset(&localAddr, 0, sizeof(Address));
            localAddr.networkType = NETWORK_IPV4;
            localAddr.interfaceAddr.ipv4 = voipData->callSignalAddr;
            sip->SipSetCallSignalAddr(voipData->callSignalAddr);

            Address remoteAddr;
            memset(&remoteAddr, 0, sizeof(Address));
            remoteAddr.networkType = NETWORK_IPV4;
            remoteAddr.interfaceAddr.ipv4 = sip->SipGetProxyIp();
            if (remoteAddr.interfaceAddr.ipv4 == INVALID_ADDRESS)
            {
                remoteAddr.networkType = NETWORK_INVALID;
            }
            node->appData.appTrafficSender->appTcpOpenConnection(
                                                    node,
                                                    APP_SIP,
                                                    localAddr,
                                                    voipData->callSignalPort,
                                                    remoteAddr,
                                                    (UInt16) APP_SIP,
                                                    voipData->callSignalPort,
                                                    APP_DEFAULT_TOS,
                                                    ANY_INTERFACE,
                                                    sip->SipGetProxyUrl(),
                                                    0,
                                                    destNodeId,
                                                    clientInterfaceIndex,
                                                    destInterfaceIndex);
          }
          else if (sip->SipGetTransProtocolType() == UDP)
          {
            // TBD
          }
    }
}


// /**
// FUNCTION   :: SipTerminateConnection
// LAYER      :: APPLICATION
// PURPOSE    :: Interface, for the third party application(now VoIP)
// using which a SIP node is informed of desire for call termination.
// PARAMETERS ::
// + node      : Node* : Pointer to the node wishing to terminate call
// + voipData  : void* : Pointer to calling application's data struct.
// RETURN     :: void : NULL
// **/
void SipTerminateConnection(Node* node, void* voip)
{
    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;

    VoipHostData* voipData = (VoipHostData*) voip;
    ERROR_Assert(voipData, "Invalid call initiator\n");

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("\nNode: %u receives SipTerminateConnection at: %s\n\n",
                node->nodeId, clockStr);
    }

    // Send a cancel/bye request to the peer
    SipMsg* sipMsg = sip->SipFindMsg(voipData->connectionId);

    if (!sipMsg)
    {
       ERROR_ReportError("TerminateConnection: Invalid SipMsg Returned\n");
    }

    SipSession* sipSession = sip->SipGetSipSessionByLocalPort(
        voipData->callSignalPort);

    SipCheckSession(sipSession, __FUNCTION__, node->nodeId);

    SipCallState state = sipSession->SipGetCallState();
    if (state == SIP_COMPLETED)
    {
        sipMsg->SipCreateByeRequestPkt(node, sip,
            sipSession->SipGetTransactionState());
    }
    else if (state != SIP_INVALID && state < 6)
    {
        printf("TerminateConnection: error, premature call rejection\n");
        return;
    }
    Message *tcpmsg = APP_TcpCreateMessage(
        node,
        voipData->connectionId,
        TRACE_SIP);

    APP_AddPayload(
        node,
        tcpmsg,
        sipMsg->SipGetBuffer(),
        sipMsg->SipGetLength());

    APP_TcpSend(node, tcpmsg);

    MEM_free(sipMsg->SipGetBuffer());
}

//--------------------------------------------------------------------------
// FUNCTION    :: SipUrlSessionStartCallBack
// PURPOSE     :: To update the client when URL is resolved for TCP based
//                SIP. This is a dummy function for SIP as we do not need to
//                update any data structure after the URL is resolved in this
//                case. This function has been kept only for consistency.
// PARAMETERS   ::
// + node       : Node*          : pointer to the node
// + serverAddr : Address*       : server address
// + sourcePort : UInt16         : source port
// + uniqueId   : Int32          : connection id
// +interfaceId : Int16          : interface index,
// +serverUrl   : std::string    : server URL
// +packetSendingInfo : AppUdpPacketSendingInfo* : information required to 
//                                                send buffered application 
//                                                packets in case of UDP 
//                                                based applications; not
//                                                used by SIP
// RETURN       :
// bool         : TRUE if application packet can be sent; FALSE otherwise
//                TCP based application will always return FALSE as 
//                this callback will initiate TCP Open request and not send 
//                packet
//---------------------------------------------------------------------------
bool
SipUrlSessionStartCallBack(
                     Node* node,
                     Address* serverAddr,
                     UInt16 sourcePort,
                     Int32 uniqueId,
                     Int16 interfaceId,
                     std::string serverUrl,
                     AppUdpPacketSendingInfo* packetSendingInfo)
{
    return (TRUE);
}
