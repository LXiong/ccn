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

#include "api.h"
#include "app_util.h"
#include "multimedia_sipmsg.h"
#include "multimedia_sipdata.h"


#define  DEBUG     0

// /**
// FUNCTION   :: SipMsg
// LAYER      :: APPLICATION
// PURPOSE    :: Constructor of the SipMsg class
// PARAMETERS ::
// RETURN     :: NULL
// **/
SipMsg :: SipMsg()
{
    msgType = SIP_INVALID_MSG;
    requestType = INVALID_REQ;
    responseType = INVALID_RES;
    contentLength = 0;
    pBuffer = NULL;
    pLength = 0;
    totHdrLen = 0;
    maxForwards = SIP_MAX_FORWARDS;
    ackType = SIP_ACK_INVALID;

    connectionId = INVALID_ID;
    proxyConnectionId = INVALID_ID;

    fromIp = INVALID_ADDRESS;
    targetIp = INVALID_ADDRESS;

    memset(targetUrl , 0, sizeof(targetUrl));

    via = NULL;
    maxViaCount = SIP_VIA_COUNT;
    viaCount = 0;
    next = NULL;

    memset(from, 0, sizeof(from));
    memset(to, 0, sizeof(to));
    memset(callId, 0, sizeof(callId));
    memset(tagTo, 0, sizeof(tagTo));
    memset(tagFrom, 0, sizeof(tagFrom));
    memset(cSeq, 0, sizeof(cSeq));
    memset(cSeqMsg, 0, sizeof(cSeqMsg));
    memset(contact, 0, sizeof(contact));
    memset(contentType, 0, sizeof(contentType));
    memset(domainName, 0, sizeof(domainName));
    memset(transProto, 0, sizeof(transProto));
    memset(callInfo, 0, sizeof(callInfo));
}


// /**
// FUNCTION   :: SipMsg
// LAYER      :: APPLICATION
// PURPOSE    :: Destructor of the SipMsg class
// PARAMETERS ::
// RETURN     :: NULL
// **/
SipMsg :: ~SipMsg()
{
    if (via != NULL)
    {
        MEM_free(via);
    }
    via = NULL;
}


// /**
// FUNCTION   :: SipGenerateVia
// LAYER      :: APPLICATION
// PURPOSE    :: Generate the Via field
// PARAMETERS ::
// +viaGen     : char* : Field to be filled up
// +start      : int   : starting address
// +isBye      : bool  : true for a BYE message
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipGenerateVia(char* viaGen, int start, bool isBye)
{
    // count refers to no of vias
    int count = start;
    int storedCount = 0;
    int offset = 0;

    while (count < viaCount)
    {
        if (via[count].isReceivedIp && !isBye)
        {
            storedCount = sprintf(viaGen + offset,
                 "Via: SIP/2.0/%s %s;branch=%s;received=%s\r\n",
                        transProto, via[count].viaProxy, via[count].branch,
                            via[count].receivedIp);
            offset += storedCount;
        }
        else
        {
            storedCount = sprintf(viaGen + offset,
                "Via: SIP/2.0/%s %s;branch=%s\r\n",
                    transProto, via[count].viaProxy, via[count].branch);
            offset += storedCount;
        }
        count++;
    }
}


// /**
// FUNCTION   :: SipGenViaWithOwnVia
// LAYER      :: APPLICATION
// PURPOSE    :: Generate the Via field
// PARAMETERS ::
// +localVia   : char* : Field to be filled up
// +sip        : SipData*  : Pointer to SipData
// RETURN     :: void  : NULL
// **/
void  SipMsg :: SipGenViaWithOwnVia(char* localVia, SipData* sip)
{
    char branch[SIP_STR_SIZE16];
    char via[SIP_VIA_COUNT * MAX_STRING_LENGTH];

    ERROR_Assert(localVia, "Via field cannot be NULL\n");

    sip->SipGenerateBranch(branch);

    sprintf(via, "Via: SIP/2.0/%s %s;branch=%s\r\n",
        transProto, sip->SipGetFQDN(), branch);

    SipGenerateVia(via + strlen(via));
    memcpy(localVia, via, strlen(via));
}


// /**
// FUNCTION   :: SipGenViaWithoutTopVia
// LAYER      :: APPLICATION
// PURPOSE    :: Generate the Via field
// PARAMETERS ::
// +localVia   : char* : Field to be filled up
// RETURN     :: void  : NULL
// **/
void  SipMsg :: SipGenViaWithoutTopVia(char* localVia)
{
    // routine assumes via parsing is already done and
    // therefore viaCount contains the numb of vias present

    ERROR_Assert(localVia, "localVia field cannot be NULL\n");
    SipGenerateVia(localVia, 1);
}


// /**
// FUNCTION   :: SipUpdateViaWithReceivedIp
// LAYER      :: APPLICATION
// PURPOSE    :: Update via field with the received Ip
// PARAMETERS ::
// RETURN     :: void  : NULL
// **/
void  SipMsg :: SipUpdateViaWithReceivedIp()
{
    char recdIpStr[SIP_STR_SIZE16];
    if (viaCount < 1)
    {
        // no via available
        return;
    }
    IO_ConvertIpAddressToString(fromIp, recdIpStr);
    strcpy(via[0].receivedIp, recdIpStr);
    via[0].isReceivedIp = true;
}


// /**
// FUNCTION   :: SipGetResponseString
// LAYER      :: APPLICATION
// PURPOSE    :: Get the respone string corresponding to an enum
// PARAMETERS ::
// +responseType    : SipResType : Response type
// +responseStr     : char*      : string to be filled up
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipGetResponseString(SipResType responseType,
                                    char* responseStr)
{
    switch (responseType)
    {
        case TRYING:
            strcpy(responseStr, "Trying");
            break;

        case RINGING:
            strcpy(responseStr, "Ringing");
            break;

        case OK:
            strcpy(responseStr, "OK");
            break;

        default:
            strcpy(responseStr, "");
    }
}


// /**
// FUNCTION   :: SipCreateInviteRequestPkt
// LAYER      :: APPLICATION
// PURPOSE    :: Create Sip Invite request packet
// PARAMETERS ::
// +node       : Node*        : Pointer to the node
// +sip        : SipData*     : Pointer to SipData
// +voip       : VoipHostData*: Pointer to VoipHostData
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipCreateInviteRequestPkt(Node* node,
                                         SipData* sip,
                                         VoipHostData* voip)
{
    char protoBuf[SIP_STR_SIZE16];
    char branch[MAX_STRING_LENGTH];
    char callId[SIP_STR_SIZE64];
    char tag[SIP_STR_SIZE16];

    char myIPAddress[MAX_STRING_LENGTH];
    char aliasAddr[MAX_ALIAS_ADDR_SIZE];

    Address addr;
    Int32 i = -1;

    if (sip->SipGetTransProtocolType() == TCP)
    {
        strcpy(protoBuf, "TCP");
    }
    else if (sip->SipGetTransProtocolType() == UDP)
    {
        strcpy(protoBuf, "UDP");
    }

    pBuffer = (char*) MEM_malloc(MAX_SIP_PACKET_SIZE);
    memset(pBuffer, 0, MAX_SIP_PACKET_SIZE);
    do
    {
       i++;
       aliasAddr[i] = voip->sourceAliasAddr[i];
    }while (voip->sourceAliasAddr[i] != '@');

    aliasAddr[++i] = '\0';

    addr.networkType = NETWORK_IPV4;
    addr.interfaceAddr.ipv4 = voip->localAddr;
    IO_ConvertIpAddressToString(&addr , myIPAddress);

    sip->SipGenerateBranch(branch);
    sip->SipGenerateTag(node, tag);
    sip->SipGenerateCallIdPfx(node, callId);


    // Add a sample SDP for audio in SIP request msg body
    // Note: 1. include only the required items, and must be in order;
    //       2. most are hardcoded for simplicity---just as placeholders in simulation and not parsed;
    //       3. use fixed RTP port for simplicity---same src/dst port for RTP, so as to let wireshark recognize RTP by port
    char sdpRequestBuffer[BIG_STRING_LENGTH]; // buffer size 512, enough for the following SIP msg body: 116+4(port#)+FQDN
    memset(sdpRequestBuffer, 0, BIG_STRING_LENGTH);
    // This works for node with single interface---should use the sending interface IP if multiple
    NodeAddress connAddr = MAPPING_GetDefaultInterfaceAddressFromNodeId(node, node->nodeId);
    char connAddrStr[MAX_STRING_LENGTH];
    memset(connAddrStr, 0, MAX_STRING_LENGTH);
    IO_ConvertIpAddressToString(connAddr, connAddrStr);
    sprintf(sdpRequestBuffer, 
            "v=0\r\n"
            "o=user 8000 8000 IN IP4 %s\r\n"
            "s=SIP Call\r\n"
            "c=IN IP4 %s\r\n"
            "t=0 0\r\n"
            "m=audio %d RTP/AVP 0\r\n"
            "a=sendrecv\r\n"
            "a=rtpmap:0 PCMU/8000\r\n"
            "a=ptime:20\r\n",
            sip->SipGetFQDN(),
            connAddrStr,
            APP_RTP);
    unsigned int sdpRequestLength = (unsigned int)strlen(sdpRequestBuffer); 
    

    sprintf(pBuffer,
            "INVITE %s SIP/2.0\r\n"
            "Via: SIP/2.0/%s %s;branch=%s\r\n"
            "Max-Forwards: %d\r\n"
            "To: <%s>\r\n"
            "From: <%s>;tag=%s\r\n"
            "Call-ID: %s\r\n"
            "CSeq: %d INVITE\r\n"
            "Contact: <%s>\r\n"
            "Call-Info: %d\r\n"
            "Content-Type: application/sdp\r\n"
            "Content-Length: %d\r\n\r\n"
            "%s",  // Add SDP msg body
            voip->destAliasAddr,
            protoBuf, sip->SipGetFQDN(), branch,
            SIP_MAX_FORWARDS,
            voip->destAliasAddr,
            voip->sourceAliasAddr, tag,
            callId,
            sip->SipGenerateSeqCounter(),
            strcat(aliasAddr,myIPAddress),
            voip->callSignalPort,
            sdpRequestLength, // SDP msg body length
            sdpRequestBuffer);// SDP msg body str

    pLength = (unsigned int)strlen(pBuffer);

    // stats incremented
    sip->SipIncrementInviteSent();
    sip->SipIncrementCallAttempted();

    SipFillComponents();

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        char msgBuf[MAX_SIP_PACKET_SIZE] = {0};
        memcpy(msgBuf, pBuffer, pLength);
        printf("Node: %u sends following message at: %s - \n%s\n",
            node->nodeId, clockStr, msgBuf);

        printf("Length of forwarded sip - pBuffer: %d\n", pLength);
    }
}


// /**
// FUNCTION   :: SipCreate2xxAckRequestPkt
// LAYER      :: APPLICATION
// PURPOSE    :: create a sip Acknowledge request packet for 2xx response
// PARAMETERS ::
// +node       : Node*        : Pointer to the node
// +sip        : SipData*     : Pointer to SipData
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipCreate2xxAckRequestPkt(Node* node, SipData* sip)
{
    char branch[MAX_STRING_LENGTH];
    char protoBuf[SIP_STR_SIZE16];

    if (sip->SipGetTransProtocolType() == TCP)
    {
        strcpy(protoBuf, "TCP");
    }
    else if (sip->SipGetTransProtocolType() == UDP)
    {
        strcpy(protoBuf, "UDP");
    }

    pBuffer = (char*) MEM_malloc(MAX_SIP_PACKET_SIZE);
    memset(pBuffer, 0, MAX_SIP_PACKET_SIZE);

    sip->SipGenerateBranch(branch);

    sprintf(pBuffer,
            "ACK %s SIP/2.0\r\n"
            "Via: SIP/2.0/%s %s;branch=%s\r\n"
            "Max-Forwards: %d\r\n"
            "To: <%s>;tag=%s\r\n"
            "From: <%s>;tag=%s\r\n"
            "Call-ID: %s\r\n"
            "CSeq: %s ACK\r\n"
            "Content-Length: %d\r\n\r\n",

            to,
            protoBuf, sip->SipGetFQDN(), branch,
            SIP_MAX_FORWARDS,
            to, tagTo,
            from, tagFrom,
            callId,
            cSeq,  // Number must be same as the cseq# of invite being ACK
            0);

    pLength = (unsigned int)strlen(pBuffer);

    sip->SipIncrementAckSent();
    sip->SipIncrementCallConnected();

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        char msgBuf[MAX_SIP_PACKET_SIZE] = {0};
        memcpy(msgBuf, pBuffer, pLength);
        printf("Node: %u sends following message at: %s - \n%s\n",
            node->nodeId, clockStr, msgBuf);

        printf("Length of forwarded sip - pBuffer: %d\n", pLength);
    }
}


// /**
// FUNCTION   :: SipCreateNon2xxAckRequestPkt
// LAYER      :: APPLICATION
// PURPOSE    :: create a sip Acknowledge request packet for Non2xx response
//               (This ack is sent for final responses between 300 and 699)
// PARAMETERS ::
// +node       : Node*        : Pointer to the node
// +sip        : SipData*     : Pointer to SipData
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipCreateNon2xxAckRequestPkt(Node* node, SipData* sip)
{
    char branch[MAX_STRING_LENGTH];
    char protoBuf[SIP_STR_SIZE16];

    if (sip->SipGetTransProtocolType() == TCP)
    {
        strcpy(protoBuf, "TCP");
    }
    else if (sip->SipGetTransProtocolType() == UDP)
    {
        strcpy(protoBuf, "UDP");
    }

    // Note: This ack must contain a single via Header,i.e.top level via.
    // Ref: SEC 17.1.1.3

    pBuffer = (char*) MEM_malloc(MAX_SIP_PACKET_SIZE);
    memset(pBuffer, 0, MAX_SIP_PACKET_SIZE);

    sip->SipGenerateBranch(branch);

    sprintf(pBuffer,
            "ACK %s SIP/2.0\r\n"
            "Via: SIP/2.0/%s %s;branch=%s\r\n"
            "Max-Forwards: %d\r\n"
            "To: <%s>;tag=%s\r\n"
            "From: <%s>;tag=%s\r\n"
            "Call-ID: %s\r\n"
            "CSeq: %s ACK\r\n\r\n",

            to,
            protoBuf, sip->SipGetFQDN(), branch,
            SIP_MAX_FORWARDS,
            to, tagTo,
            from, tagFrom,
            callId,
            cSeq);

    pLength = (unsigned int)strlen(pBuffer);

    sip->SipIncrementAckSent();

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        char msgBuf[MAX_SIP_PACKET_SIZE] = {0};
        memcpy(msgBuf, pBuffer, pLength);
        printf("Node: %u sends following message at: %s -\n%s\n",
            node->nodeId, clockStr, msgBuf);
    }
}


// /**
// FUNCTION   :: SipCreateByeRequestPkt
// LAYER      :: APPLICATION
// PURPOSE    :: Create a Bye request packet
// PARAMETERS ::
// +node       : Node*    : Pointer to the node
// +sip        : SipData* : Pointer to SipData
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipCreateByeRequestPkt(Node* node, SipData* sip,
                                      SipTransactionState state)
{
    char branch[MAX_STRING_LENGTH];
    sip->SipGenerateBranch(branch);

    pBuffer = (char*) MEM_malloc(MAX_SIP_PACKET_SIZE);
    memset(pBuffer, 0, MAX_SIP_PACKET_SIZE);

    // Note that sender of BYE request creates a new transaction
    // within the scope of current dialogue. So it genereates and
    // uses a new CSeq number and if it is the same node that has
    // issued the Invite request then from and to field values remain
    // unaltered else if the called node hangs up first then it
    // is the node that issues the BYE request and hence from and to
    // field aliases including tags are swapped for creation of this req.
    // Call Id remains the same as it is the same dlg.

    if (state == SIP_CLIENT)
    {
        sprintf(pBuffer,
                "BYE %s SIP/2.0\r\n"
                "Via: SIP/2.0/%s %s;branch=%s\r\n"
                "Max-Forwards: %d\r\n"
                "To: <%s>;tag=%s\r\n"
                "From: <%s>;tag=%s\r\n"
                "Call-ID: %s\r\n"
                "CSeq: %d BYE\r\n"
                "Content-Length: %d\r\n\r\n",

                to,
                transProto, sip->SipGetFQDN(), branch,
                SIP_MAX_FORWARDS,
                to, tagTo,
                from, tagFrom,
                callId,
                sip->SipGenerateSeqCounter(),
                0);
    }
    else if (state == SIP_SERVER)
    {
       // must swap from and to fields
       sprintf(pBuffer,
               "BYE %s SIP/2.0\r\n"
               "Via: SIP/2.0/%s %s;branch=%s\r\n"
               "Max-Forwards: %d\r\n"
               "To: <%s>;tag=%s\r\n"
               "From: <%s>;tag=%s\r\n"
               "Call-ID: %s\r\n"
               "CSeq: %d BYE\r\n"
               "Content-Length: %d\r\n\r\n",

               from,
               transProto, sip->SipGetFQDN(), branch,
               SIP_MAX_FORWARDS,
               from, tagFrom,
               to, tagTo,
               callId,
               sip->SipGenerateSeqCounter(),
               0);
    }

    pLength = (unsigned int)strlen(pBuffer);

    sip->SipIncrementByeSent();

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        char msgBuf[MAX_SIP_PACKET_SIZE] = {0};
        memcpy(msgBuf, pBuffer, pLength);
        printf("Node: %u sends following message at: %s -\n%s\n",
            node->nodeId, clockStr, msgBuf);

        printf("Length of sip - pBuffer being sent: %d\n", pLength);
    }
}


// /**
// FUNCTION   :: SipCreateResponsePkt
// LAYER      :: APPLICATION
// PURPOSE    :: Create a response packet
// PARAMETERS ::
// +node       : Node*    : Pointer to the node
// +sip        : SipData* : Pointer to SipData
// +responseType: SipResType : Sip response type
// +requestType : char*   : request type
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipCreateResponsePkt(Node* node,
                                    SipData* sip,
                                    SipResType responseType,
                                    const char* requestType)
{
    char via[SIP_VIA_COUNT * SIP_STR_SIZE64];
    char responseStr[SIP_STR_SIZE16];
    char toBuf[SIP_STR_SIZE64];

    pBuffer = (char*) MEM_malloc(MAX_SIP_PACKET_SIZE);
    memset(pBuffer, 0, MAX_SIP_PACKET_SIZE);
    memset(via, 0, SIP_VIA_COUNT * SIP_STR_SIZE64);

    SipGetResponseString(responseType, responseStr);

    if (sip->SipGetCallModel() == SipProxyRouted)
    {
        SipUpdateViaWithReceivedIp();
    }


    switch (responseType)
    {
        case RINGING:
        {
            SipGenerateVia(via);
            sip->SipGenerateTag(node, tagTo);
            sprintf(toBuf, "To: <%s>;tag=%s\r\n", to, tagTo);
            break;
        }

        case OK:
        {
            if (!strcmp(requestType, "BYE"))
            {
                SipGenerateVia(via, 0, true);
            }
            else
            {
                SipGenerateVia(via);
            }
            sprintf(toBuf, "To: <%s>;tag=%s\r\n", to, tagTo);
            break;
        }

        case TRYING:
        {
            SipGenerateVia(via);
            sprintf(toBuf, "To: <%s>\r\n", to);
            break;
        }

        default:
            break;
    }

    // add msg body for 200 OK response to INVITE
    if (responseType == OK && !strcmp(requestType, "INVITE")) 
    {
        char sdpResponseBuffer[BIG_STRING_LENGTH]; // buffer size 512, enough for the following SIP msg body: 127+4(port#)+FQDN+IP
        memset(sdpResponseBuffer, 0, BIG_STRING_LENGTH);

        // This works for node with single interface---should use the sending interface IP if multiple
        NodeAddress connAddr = MAPPING_GetDefaultInterfaceAddressFromNodeId(node, node->nodeId);
        char connAddrStr[MAX_STRING_LENGTH];
        memset(connAddrStr, 0, MAX_STRING_LENGTH);
        IO_ConvertIpAddressToString(connAddr, connAddrStr);

        sprintf(sdpResponseBuffer, 
                "v=0\r\n"
                "o=user 8000 8000 IN IP4 %s\r\n"
                "s=SIP Call\r\n"
                "c=IN IP4 %s\r\n"
                "t=0 0\r\n"
                "m=audio %d RTP/AVP 0\r\n"
                "a=sendrecv\r\n"
                "a=rtpmap:0 PCMU/8000\r\n"
                "a=ptime:20\r\n",
                sip->SipGetFQDN(),
                connAddrStr, // own IP address
                APP_RTP);

        unsigned int sdpResponseLength = (unsigned int)strlen(sdpResponseBuffer);

        sprintf(pBuffer,
                "SIP/2.0 %d %s\r\n"
                "%s"
                "%s"
                "From: <%s>;tag=%s\r\n"
                "Call-ID: %s\r\n"
                "CSeq: %s %s\r\n"
                "Content-Type: application/sdp\r\n"
                "Content-Length: %d\r\n\r\n"
                "%s",

                responseType, responseStr,
                via,
                toBuf,
                from, tagFrom,
                callId,
                cSeq, requestType,
                sdpResponseLength,
                sdpResponseBuffer);        
    }
    else
    {
        sprintf(pBuffer,
                "SIP/2.0 %d %s\r\n"
                "%s"
                "%s"
                "From: <%s>;tag=%s\r\n"
                "Call-ID: %s\r\n"
                "CSeq: %s %s\r\n"
                "Content-Length: %d\r\n\r\n",

                responseType, responseStr,
                via,
                toBuf,
                from, tagFrom,
                callId,
                cSeq, requestType,
                0);
    }

    pLength = (unsigned int)strlen(pBuffer);

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        char msgBuf[MAX_SIP_PACKET_SIZE] = {0};
        memcpy(msgBuf, pBuffer, pLength);
        printf("Node: %u sends following message at: %s -\n%s\n",
            node->nodeId, clockStr, msgBuf);

        printf("Length of sip - pBuffer being sent: %d\n", pLength);
    }
}

//---------------------------------------------------------------------------
// FUNCTION:    SipCreate2xxOkResponsePkt()
// PURPOSE:     Create a 200 OK response packet
// PARAMETERS:  node::Pointer to the node
//              sip::Pointer to SipData
//              voip::Pointer to VoipHostData
// RETURN:      None.
//---------------------------------------------------------------------------
void SipMsg :: SipCreate2xxOkResponsePkt(Node* node,
                                    SipData* sip,
                                    VoipHostData* voip)
{
    char via[SIP_VIA_COUNT * SIP_STR_SIZE64];
    char responseStr[SIP_STR_SIZE16];
    char toBuf[SIP_STR_SIZE64];

    SipResType responseType = OK;
    const char* requestType = "INVITE";

    char myIPAddress[MAX_STRING_LENGTH];
    char aliasAddr[MAX_ALIAS_ADDR_SIZE];

    SipGetResponseString(responseType, responseStr);

    Address addr;
    Int32 i = -1;

    pBuffer = (char*) MEM_malloc(MAX_SIP_PACKET_SIZE);
    memset(pBuffer, 0, MAX_SIP_PACKET_SIZE);
    memset(via, 0, SIP_VIA_COUNT * SIP_STR_SIZE64);

    if (sip->SipGetCallModel() == SipProxyRouted)
    {
        SipUpdateViaWithReceivedIp();
    }

    do
    {
       i++;
       aliasAddr[i] = voip->sourceAliasAddr[i];
    }while (voip->sourceAliasAddr[i] != '@');

    aliasAddr[++i] = '\0';

    addr.networkType = NETWORK_IPV4;

    //taking the fresh IP Address
    voip->localAddr = MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
                                node,
                                node->nodeId,
                                voip->clientInterfaceIndex);

    addr.interfaceAddr.ipv4 = voip->localAddr;
    IO_ConvertIpAddressToString(&addr , myIPAddress);

   //OK Response
    SipGenerateVia(via);

    sprintf(toBuf, "To: <%s>;tag=%s\r\n", to, tagTo);

    sprintf(pBuffer,
            "SIP/2.0 %d %s\r\n"
            "%s"
            "%s"
            "From: <%s>;tag=%s\r\n"
            "Call-ID: %s\r\n"
            "Contact: <%s>\r\n"
            "CSeq: %s %s\r\n"
            "Content-Length: %d\r\n\r\n",

            responseType, responseStr,
            via,
            toBuf,
            from, tagFrom,
            callId,
            strcat(aliasAddr , myIPAddress),
            cSeq, requestType,
            0);

    pLength = (unsigned int)strlen(pBuffer);

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        char msgBuf[MAX_SIP_PACKET_SIZE] = {0};
        memcpy(msgBuf, pBuffer, pLength);
        printf("Node: %u sends following message at: %s -\n%s\n",
            node->nodeId, clockStr, msgBuf);

        printf("Length of sip - pBuffer being sent: %d\n", pLength);
    }
}

// /**
// FUNCTION   :: SipIsRequestMsg
// LAYER      :: APPLICATION
// PURPOSE    :: Check the type of received sip message
// PARAMETERS ::
// +token1     : char* : Input buffer
// RETURN     :: SipMsgType  : Type of Sip message
// **/
SipMsgType SipMsg :: SipIsRequestMsg(char* token1)
{
    if (!strcmp(token1, "INVITE"))
    {
        requestType = INV;
    }
    else if (!strcmp(token1, "ACK"))
    {
        requestType = ACK;
    }
    else if (!strcmp(token1, "BYE"))
    {
        requestType = BYE;
    }
    else if (!strcmp(token1, "OPTIONS"))
    {
        requestType = OPT;
    }
    else if (!strcmp(token1, "REGISTER"))
    {
        requestType = REG;
    }
    else if (!strcmp(token1, "CANCEL"))
    {
        requestType = CAN;
    }
    else
    {
        requestType = INVALID_REQ;
    }

    if (requestType == INVALID_REQ)
    {
        msgType = SIP_INVALID_MSG;
    }
    else
    {
        msgType = SIP_REQUEST;
    }
    return msgType;
}


// /**
// FUNCTION   :: SipIsResponseMsg
// LAYER      :: APPLICATION
// PURPOSE    :: Check the type of received sip message
// PARAMETERS ::
// +responseToken  : char* : Input buffer
// +resString      : char* : response string
// RETURN     :: SipMsgType  : Type of Sip message
// **/
SipMsgType SipMsg :: SipIsResponseMsg(char* responseToken, char* resString)
{
    switch (atoi(responseToken))
    {
        case TRYING:
            if (strcmp(resString, "Trying"))
            {
                return SIP_INVALID_MSG;
            }
            responseType = TRYING;
            break;

        case RINGING:
            if (strcmp(resString, "Ringing"))
            {
                return SIP_INVALID_MSG;
            }
            responseType = RINGING;
            break;

        case OK:
            if (strcmp(resString, "OK"))
            {
                return SIP_INVALID_MSG;
            }
            responseType = OK;
            break;

        default:
           responseType = INVALID_RES;
           break;
    }

    if (responseType == INVALID_RES)
    {
        msgType = SIP_INVALID_MSG;
    }
    else
    {
        msgType = SIP_RESPONSE;
    }
    return msgType;
}


// /**
// FUNCTION   :: SipParseTo
// LAYER      :: APPLICATION
// PURPOSE    :: Parse the input field
// PARAMETERS ::
// +toField    : char* : Field to be parsed
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipParseTo(char* toField)
{
    char* next;
    const char* delims = {" ;"};
    char* token;

    char  field1[SIP_STR_SIZE16];
    char  field2[SIP_STR_SIZE64];
    //Incresed Array size to 32 to fix memory corruption.
    char  field3[SIP_STR_SIZE32];

    memset(field1, 0, sizeof(field1));
    memset(field2, 0, sizeof(field2));
    memset(field3, 0, sizeof(field3));

    IO_GetDelimitedToken(field1, toField, delims, &next);
    IO_GetDelimitedToken(field2, next, delims, &next);
    token = IO_GetDelimitedToken(field3, next, delims, &next);

    // strip off < and >
    memcpy(to, field2 + 1, strlen(field2) - 2);

    if (token)
    {
        sscanf(field3, "tag=%s", tagTo);
    }
}


// /**
// FUNCTION   :: SipParseVia
// LAYER      :: APPLICATION
// PURPOSE    :: Parse the input field
// PARAMETERS ::
// +currVia    : char* : Field to be parsed
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipParseVia(char* currVia)
{
    char token1[SIP_STR_SIZE64];

    const char* delims = ";= \r\n";
    char* next;

    if (viaCount >= maxViaCount)
    {
        maxViaCount += SIP_VIA_COUNT;
        SipVia* newVia = (SipVia*) MEM_malloc(sizeof(SipVia) * maxViaCount);
        memcpy(newVia, via, sizeof(SipVia) * viaCount);
        MEM_free(via);
        via = newVia;
    }

    // ignore version
    IO_GetDelimitedToken(token1, currVia, delims, &next);
    IO_GetDelimitedToken(token1, next, delims, &next);
    strcpy(transProto, token1 + strlen(token1) - 3);

    // get domain name
    IO_GetDelimitedToken(token1, next, delims, &next);
    strcpy(via[viaCount].viaProxy, token1);

    // get branch keyword
    IO_GetDelimitedToken(token1, next, delims, &next);

    if (!strcmp(token1, "branch"))
    {
        // get branch value
        IO_GetDelimitedToken(token1, next, delims, &next);
        strcpy(via[viaCount].branch, token1);
    }

    // get received keyword if any
    IO_GetDelimitedToken(token1, next, delims, &next);

    if (!strcmp(token1, "received"))
    {
        // get received value
        IO_GetDelimitedToken(token1, next, delims, &next);
        via[viaCount].isReceivedIp = true;
        strcpy(via[viaCount].receivedIp, token1);
        IO_GetDelimitedToken(token1, next, delims, &next);
    }
    else
    {
        via[viaCount].isReceivedIp = false;
    }
    viaCount++;
}


// /**
// FUNCTION   :: SipAllocateVia
// LAYER      :: APPLICATION
// PURPOSE    :: Init and allocate memory for the Via field
// PARAMETERS ::
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipAllocateVia()
{
    if (via)
    {
        viaCount = 0;
        MEM_free(via);
        via = NULL;
    }

    via = (SipVia*) MEM_malloc(sizeof(SipVia) * SIP_VIA_COUNT);
    memset(via, 0, sizeof(SipVia) * SIP_VIA_COUNT);
}


// /**
// FUNCTION   :: SipParseField
// LAYER      :: APPLICATION
// PURPOSE    :: Parse for fields of the sip message
// PARAMETERS ::
// +sipField   : char* : Input buffer supplied for parsing
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipParseField(char* sipField)
{
    ERROR_Assert(sipField != NULL && *sipField != '\0',
            "SipParseField: No data available for parsing");

    // For IO_GetDelimitedToken
    char* next;
    char* token;
    char delims[] = {"\r\n"};
    char headerField[MAX_STRING_LENGTH];
    char headerName[SIP_STR_SIZE16];
    int  len = totHdrLen;

    SipAllocateVia();

    memset(headerField, 0, sizeof(headerField));
    memset(headerName, 0, sizeof(headerName));

    token = IO_GetDelimitedToken(headerField, sipField, delims, &next);
    while (token && len > 0)
    {
        sscanf(headerField, "%s", headerName);
        len -= (int)strlen(headerField) + 2;

        if (!strcmp(headerName, "Via:"))
        {
            SipParseVia(headerField);
        }
        else if (!strcmp(headerName, "Max-Forwards:"))
        {
            sscanf(headerField,"Max-Forwards: %d", &maxForwards);
        }
        else if (!strcmp(headerName, "To:"))
        {
            SipParseTo(headerField);
        }
        else if (!strcmp(headerName, "From:"))
        {
            SipParseFrom(headerField);
        }
        else if (!strcmp(headerName, "CSeq:"))
        {
            sscanf(headerField,"CSeq: %s %s", cSeq, cSeqMsg);
        }
        else if (!strcmp(headerName, "Contact:"))
        {
            SipParseContact(headerField);
        }
        else if (!strcmp(headerName, "Call-ID:"))
        {
            sscanf(headerField,"Call-ID: %s", callId);
        }
        else if (!strcmp(headerName, "Call-Info:"))
        {
            sscanf(headerField,"Call-Info: %s", callInfo);
        }
        else if (!strcmp(headerName, "Content-Type:"))
        {
            sscanf(headerField,"Content-Type: %s", contentType);
        }
        else if (!strcmp(headerName, "Content-Length:"))
        {
            sscanf(headerField,"Content-Length: %d", &contentLength);
        }
        memset(headerField, 0, sizeof(headerField));
        if (len <= 0)
        {
            break;
        }
        token = IO_GetDelimitedToken(headerField, next, delims, &next);
    }
}


// /**
// FUNCTION   :: SipParseFrom
// LAYER      :: APPLICATION
// PURPOSE    :: Parse the input field
// PARAMETERS ::
// +fromField  : char* : Field to be parsed
// RETURN     :: void  : NULL
// **/
void  SipMsg :: SipParseFrom(char* fromField)
{
    char* next;
    const char* delims = " ;";

    char token1[SIP_STR_SIZE32];
    //Incresed Array size to 64 to fix memory corruption.
    char token2[SIP_STR_SIZE64];
    char token3[SIP_STR_SIZE32];

    IO_GetDelimitedToken(token1, fromField, delims, &next);
    IO_GetDelimitedToken(token2, next, delims, &next);
    IO_GetDelimitedToken(token3, next, delims, &next);

    // strip off "< and >"
    memcpy(from, token2 + 1, strlen(token2) - 2);

    // get fromTag
    if (token3)
    {
        sscanf(token3, "tag=%s", tagFrom);
    }
}


// /**
// FUNCTION   :: SipParseContact
// LAYER      :: APPLICATION
// PURPOSE    :: Parse the input field
// PARAMETERS ::
// +contactField  : char* : Field to be parsed
// RETURN     :: void  : NULL
// **/
void  SipMsg :: SipParseContact(char* contactField)
{
    char tmpContact[MAX_STRING_LENGTH];

    memset(tmpContact, 0, MAX_STRING_LENGTH);
    sscanf(contactField,"Contact: %s", tmpContact);
    memcpy(contact, tmpContact + 1, strlen(tmpContact) - 2);
}


// /**
// FUNCTION   :: SipParseFrom
// LAYER      :: APPLICATION
// PURPOSE    :: Parse the input field
// PARAMETERS ::
// +fromField  : char* : Field to be parsed
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipParseMsg(char* msgBuf, unsigned len)
{
    char startLine[MAX_STRING_LENGTH];
    char token1[SIP_STR_SIZE16];
    char token2[SIP_STR_SIZE64];
    char token3[SIP_STR_SIZE16];

    const char* delims = {"\r\n"};
    const char* delims2 = {" "};

    char* next = NULL;
    char* next2 = NULL;

    pBuffer = msgBuf;
    pLength = len;

    IO_GetDelimitedToken(startLine, pBuffer, delims, &next);
    ERROR_Assert(next, "SipParseMsg(): End of Line is missing in SIP msg");

    // Use empty line between headers and body to extract headers
    const char* emptyLine = {"\r\n\r\n"}; 
    char* endOfHeader = NULL;
    endOfHeader = strstr(pBuffer, emptyLine);
    ERROR_Assert(endOfHeader, "The empty Line after SIP headers is missing\n");
    totHdrLen = (unsigned)(endOfHeader - next);

    IO_GetDelimitedToken(token1, startLine, delims2, &next2);
    IO_GetDelimitedToken(token2, next2, delims2, &next2);
    IO_GetDelimitedToken(token3, next2, delims2, &next2);

    if ((SipIsRequestMsg(token1) == SIP_REQUEST) ||
         (SipIsResponseMsg(token2, token3) == SIP_RESPONSE))
    {
        char headers[BIG_STRING_LENGTH];
        char* startOfHeader = next + 2;
        strncpy(headers, startOfHeader, totHdrLen);
        headers[totHdrLen] = '\0'; // manually null terminated for c str
        SipParseField(headers);
    }
}


// /**
// FUNCTION   :: SipFillComponents
// LAYER      :: APPLICATION
// PURPOSE    :: Parse sip message to fill components
// PARAMETERS ::
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipFillComponents()
{
    ERROR_Assert(pBuffer, "Invalid data in Buffer");

    // Use empty line between headers and body to extract headers
    char headers[BIG_STRING_LENGTH];
    const char* endOfLine = {"\r\n"};
    const char* emptyLine = {"\r\n\r\n"};   
    char* endOfStartLine = NULL;
    char* endOfHeader = NULL;
    char* startOfHeader = NULL;

    endOfStartLine = strstr(pBuffer, endOfLine);
    ERROR_Assert(endOfStartLine, "SipFillComponents(): End of Line is missing in SIP msg");
    endOfHeader = strstr(pBuffer, emptyLine);
    ERROR_Assert(endOfHeader, "The empty Line after SIP headers is missing\n");

    totHdrLen = (unsigned)(endOfHeader - endOfStartLine);

    startOfHeader = endOfStartLine + 2;
    strncpy(headers, startOfHeader, totHdrLen);
    headers[totHdrLen] = '\0'; // manually null terminated for c str
    SipParseField(headers);
}


// /**
// FUNCTION   :: SipForwardResponsePkt
// LAYER      :: APPLICATION
// PURPOSE    :: create response packet for forwarding
// PARAMETERS ::
// +node       : Node* : Pointer to the node
// +resp       : SipResType : Type of response
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipForwardResponsePkt(Node* node, SipResType resp)
{
    char responseStr[SIP_STR_SIZE16];
    char currVia[SIP_VIA_COUNT * MAX_STRING_LENGTH];

    memset(currVia, 0, sizeof(currVia));

    SipData* sip = (SipData*)node->appData.multimedia->sigPtr;

    pBuffer = (char*) MEM_malloc(MAX_SIP_PACKET_SIZE);
    memset(pBuffer, 0, MAX_SIP_PACKET_SIZE);

    SipGetResponseString(resp, responseStr);

    SipGenViaWithoutTopVia(currVia);

    sprintf(pBuffer,
            "SIP/2.0 %d %s\r\n"
            "%s"
            "To: <%s>;tag=%s\r\n"
            "From: <%s>;tag=%s\r\n"
            "Call-ID: %s\r\n"
            "CSeq: %s %s\r\n"
            "Contact: <%s>\r\n"
            "Content-Length: %d\r\n\r\n",

            responseType, responseStr,
            currVia,
            to, tagTo,
            from, tagFrom,
            callId,
            cSeq, cSeqMsg,
            contact,
            0);

    pLength = (unsigned int)strlen(pBuffer);

    sip->SipIncrementRespForwarded();

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        char msgBuf[MAX_SIP_PACKET_SIZE] = {0};
        memcpy(msgBuf, pBuffer, pLength);
        printf("Node: %d forwards following message at: %s -\n%s\n",
            node->nodeId, clockStr, msgBuf);

        printf("Length of forwarded sip - pBuffer: %d\n", pLength);
    }
}


// /**
// FUNCTION   :: SipForwardInviteReqPkt
// LAYER      :: APPLICATION
// PURPOSE    :: Create Invite request packet for forwarding
// PARAMETERS ::
// +node       : Node* : Pointer to the node
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipForwardInviteReqPkt(Node* node)
{
    char currVia[SIP_VIA_COUNT * MAX_STRING_LENGTH];
    memset(currVia, 0, sizeof(currVia));

    SipData* sip = (SipData*) node->appData.multimedia->sigPtr;

    pBuffer = (char*) MEM_malloc(MAX_SIP_PACKET_SIZE);
    memset(pBuffer, 0, MAX_SIP_PACKET_SIZE);

    // recdIp added during trying message
    // add own via and forward

    SipGenViaWithOwnVia(currVia, sip);
    sprintf(pBuffer,
       "INVITE %s SIP/2.0\r\n"
       "%s"
       "Max-Forwards: %d\r\n"
       "To: <%s>\r\n"
       "From: <%s>;tag=%s\r\n"
       "Call-ID: %s\r\n"
       "CSeq: %s INVITE\r\n"
       "Contact: <%s>\r\n"
       "Call-Info: %s\r\n"
       "Content-Type: application/sdp\r\n"
       "Content-Length: %d\r\n\r\n",

       to,
       currVia,
       maxForwards,
       to,
       from, tagFrom,
       callId,
       cSeq,
       contact,
       callInfo,
       0);

    pLength = (unsigned int)strlen(pBuffer);

    // stats incremented
    sip->SipIncrementReqForwarded();

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

        char msgBuf[MAX_SIP_PACKET_SIZE] = {0};
        memcpy(msgBuf, pBuffer, pLength);
        printf("Node: %d forwards following message at: %s - \n%s\n",
            node->nodeId, clockStr, msgBuf);

        printf("Length of forwarded sip-pBuffer: %d\n", pLength);
    }
}


// /**
// FUNCTION   :: SipSetProperConnectionId
// LAYER      :: APPLICATION
// PURPOSE    :: Set proper connection Id in the sip message
// PARAMETERS ::
// +node       : Node* : Pointer to the node
// +connectionId       : int : connection Id
// +oldMsg     : SipMsg* : Pointer to the sip message
// RETURN     :: void  : NULL
// **/
void SipMsg :: SipSetProperConnectionId(SipData* sip, int connectionId,
               SipMsg* oldMsg)
{
    SipReqType reqType = INVALID_REQ;
    SipResType resType = INVALID_RES;

    switch (SipGetMsgType())
    {
        case SIP_REQUEST:
            reqType = SipGetReqType();
            break;

        case SIP_RESPONSE:
            resType = SipGetResType();
            break;
    }

    if (sip->SipGetHostType() == SIP_PROXY)
    {
        if (SipGetMsgType() == SIP_REQUEST)
        {
            SipSetConnectionId(connectionId);
            if (oldMsg)
            {
                SipSetProxyConnectionId(oldMsg->SipGetProxyConnectionId());
                SipSetFromIp(oldMsg->SipGetFromIp());
            }
        }
        else
        {
            SipSetProxyConnectionId(connectionId);
            if (oldMsg)
            {
                SipSetConnectionId(oldMsg->SipGetConnectionId());
            }
        }
    }
    else if (sip->SipGetHostType() == SIP_UA)
    {
        if (sip->SipGetCallModel() == SipProxyRouted)
        {
            if ((reqType == INV) ||
                (resType == TRYING) ||
                (resType == RINGING) ||
                (resType == OK))
            {
                SipSetProxyConnectionId(connectionId);

                if (oldMsg)
                {
                    SipSetConnectionId(oldMsg->SipGetConnectionId());
                    SipSetFromIp(oldMsg->SipGetFromIp());
                }
            }
            else
            {
                SipSetConnectionId(connectionId);
            }
        }
        else // Direct
        {
            SipSetConnectionId(connectionId);
        }
    }
}

