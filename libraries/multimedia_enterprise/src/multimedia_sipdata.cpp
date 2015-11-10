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
// COMMENTS     :: This is implementation of SIP 2.0 as per RFC 3261
// **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "app_util.h"
#include "network_ip.h"
#include "multimedia_sipdata.h"
#include "app_dns.h"
// Pseudo traffic sender layer
#include "app_trafficSender.h"

#define  DEBUG      0
#define  DEBUG_L2   0

// /**
// FUNCTION   :: SipData
// LAYER      :: APPLICATION
// PURPOSE    :: Constructor of the SipData class
// PARAMETERS ::
// + hostType  : SipHostType : host type of the node
// RETURN     :: NULL
// **/
SipData :: SipData(Node* node,
                   SipHostType hostType)
{
    hostBlockList.hostBlockHead = NULL;
    hostBlockList.hostBlockTail = NULL;

    ownAliasAddr = NULL;
    proxyId = ANY_NODEID;
    proxyIp = INVALID_ADDRESS;

    if (hostType == SIP_PROXY)
    {
        SipAllocateNextBlock();
    }

    branchCounter = 1;
    tagCounter = 100;
    callId = 1;
    seqCounter = 1;
    callSignalPort = 0;
    sipMsgList = NULL;
    memset(&stat, 0, sizeof(SipStat));
    domainList = NULL;

    RANDOM_SetSeed(seed,
                   node->globalSeed,
                   node->nodeId,
                   APP_SIP);
    RANDOM_SetSeed(tagSeed,
                   node->globalSeed,
                   node->nodeId,
                   APP_SIP,
                   1);

    // For unique 32 bit number generation
    ruleVector = NULL;
    seedState = NULL;
    nextState = NULL;
    sizeOfCA = 0;
    count = 0;
}


// /**
// FUNCTION   :: SipData
// LAYER      :: APPLICATION
// PURPOSE    :: Destructor of the SipData class
// PARAMETERS :: NIL
// RETURN     :: NULL
// **/
SipData :: ~SipData()
{
    if (domainList)
    {
        // NULL is passed as node is not reqd here
        ListFree(NULL, domainList, false);
    }
}


// /**
// FUNCTION   :: SipPrintAliasInfo
// LAYER      :: APPLICATION
// PURPOSE    :: print node details read from the alias file
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// RETURN     :: void : NULL
// **/
void SipData :: SipPrintAliasInfo(Node* node)
{
    char ipAddr[MAX_ADDRESS_STRING_LENGTH];
    printf("\nDetails of Node %d\n", node->nodeId);
    if (transProto == UDP)
    {
        printf("Transport Protocol : UDP\n");
    }
    else if (transProto == TCP)
    {
        printf("Transport Protocol : TCP\n");
    }

    if (callModel == SipDirect)
    {
        printf("Call model : DIRECT\n");
    }
    else if (callModel == SipProxyRouted)
    {
        printf("Call model : PROXY-ROUTED\n");
    }


    if (isSipStatEnabled)
        printf("SipStatistis : YES\n");
    else
        printf("SipStatistis : NO\n");

    if (hostType == SIP_UA)
    {
        printf("HostType : UA\n");
    }
    else if (hostType == SIP_PROXY)
    {
        printf("HostType : PROXY\n");
    }

    printf("Alias Address : %s\n", ownAliasAddr);
    printf("Proxy node : %d\n", proxyId);

    IO_ConvertIpAddressToString(proxyIp, ipAddr);
    printf("Proxy IP : %s\n", ipAddr);

    SipHostBlock* pBlock = NULL;

    if (hostType == SIP_PROXY)
    {
        int i = 0;
        pBlock = hostBlockList.hostBlockHead;
        while (pBlock != NULL)
        {
            for (i = 0; i < pBlock->hostCount; i++)
            {
                IO_ConvertIpAddressToString(pBlock->hostList[i].hostIp,
                    ipAddr);
                printf("HostId: %d  HostIP: %s HostAliasName: %s\n",
                    pBlock->hostList[i].hostId,
                    ipAddr,
                    pBlock->hostList[i].hostName);
            }
            pBlock = pBlock->next;
        }
    }
    printf("\n***********************************\n");
}


// /**
// FUNCTION   :: SipStatsInit
// LAYER      :: APPLICATION
// PURPOSE    :: Read statistics related configuration data
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + nodeInput : NodeInput* : Pointer to the input config data
// RETURN     :: void : NULL
// **/
void SipData :: SipStatsInit(Node* node, const NodeInput* nodeInput)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];

    IO_ReadString(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "VOIP-SIGNALLING-STATISTICS",
                  &retVal,
                  buf);

    if (retVal == TRUE)
    {
        if (!strcmp(buf, "YES")){
            isSipStatEnabled = true;
        }
        else if (!strcmp(buf, "NO")){
            isSipStatEnabled = false;
        }
        else{
            char errorBuf[MAX_STRING_LENGTH];
            sprintf(errorBuf,
                "VOIP-SIGNALLING-STATISTICS unknown setting %s\n", buf);
            ERROR_ReportError(errorBuf);
        }
    }
    else
    {
        // Let us assume FALSE if not specified
        isSipStatEnabled = false;
    }
}


// /**
// FUNCTION   :: SipAllocateNextBlock
// LAYER      :: APPLICATION
// PURPOSE    :: Allocate the next block for storage of alias data
// PARAMETERS :: NIL
// RETURN     :: void : NULL
// **/
void SipData :: SipAllocateNextBlock()
{
    SipHostBlock*  temp = new SipHostBlock;
    ERROR_Assert(temp != NULL, "Memory Allocation Error for SipHostBlock");

    temp->next = NULL;
    temp->hostCount = 0;
    memset(temp->hostList, 0,
        (MAX_HOSTS_PER_PROXYBLOCK * sizeof(SipHostNode)));

    if (!hostBlockList.hostBlockHead)
    {
        hostBlockList.hostBlockHead = temp;
    }
    else
    {
        hostBlockList.hostBlockTail->next = temp;
    }
    hostBlockList.hostBlockTail = temp;
};


// /**
// FUNCTION   :: SipInsertToHostBlock
// LAYER      :: APPLICATION
// PURPOSE    :: Insert a node into a HostBlock(External Interface 1)
// PARAMETERS ::
// + pHost     : SipHostNode* : Pointer to sip node
// RETURN     :: void : NULL
// **/
bool SipData :: SipInsertToHostBlock(SipHostNode* pHost)
{
    if (!hostBlockList.hostBlockHead)
    {
        ERROR_ReportError("No host block exists in the Proxy Database");
        return false;
    }

    SipHostBlock* pTmp = hostBlockList.hostBlockTail;
    pTmp->hostList[pTmp->hostCount] = *pHost;
    pTmp->hostCount++;

    // handle block overflow
    if ((pTmp->hostCount) == MAX_HOSTS_PER_PROXYBLOCK)
    {
        SipAllocateNextBlock();
    }
    return true;
};


// /**
// FUNCTION   :: SipGetHostIp
// LAYER      :: APPLICATION
// PURPOSE    :: Read hostIp from the hostAlias name(External Interface 2)
// PARAMETERS ::
// + hostName  : char* : host name
// + hostIP    : unsigned : host IP address
// RETURN     :: void : NULL
// **/
void SipData :: SipGetHostIp(char* hostName, unsigned& hostIP)
{
    unsigned short i ;
    SipHostBlock* tmp = NULL;
    hostIP = 0;

    for (tmp = hostBlockList.hostBlockHead; tmp != NULL; tmp = tmp->next)
    {
        for (i = 0; i < tmp->hostCount; i++)
        {
            if (!strcmp(hostName, tmp->hostList[i].hostName))
            {
                hostIP = tmp->hostList[i].hostIp;
                return;
            }
        }
    }
}

//---------------------------------------------------------------------------
// FUNCTION   :: SipGetHostId
// LAYER      :: APPLICATION
// PURPOSE    :: Read hostId from the hostAlias name(External Interface 2)
// PARAMETERS ::
// + hostName  : char* : host name
// + hostId    : unsigned : host Id
// RETURN     :: void : NULL
//---------------------------------------------------------------------------
void SipData :: SipGetHostId(char* hostName, unsigned& hostId)
{
    unsigned short i ;
    SipHostBlock* tmp = NULL;
    hostId = 0;

    for (tmp = hostBlockList.hostBlockHead; tmp != NULL; tmp = tmp->next)
    {
        for (i = 0; i < tmp->hostCount; i++)
        {
            if (!strcmp(hostName, tmp->hostList[i].hostName))
            {
                hostId = tmp->hostList[i].hostId;
                return;
            }
        }
    }
}
// /**
// FUNCTION   :: SipReadConfigFile
// LAYER      :: APPLICATION
// PURPOSE    :: Read relevant details from the input config
//               file supplied as nodeinput
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + nodeInput : NodeInput* : Input config file
// RETURN     :: void : NULL
// **/
void SipData :: SipReadConfigFile(Node* node, const NodeInput* nodeInput)
{
    BOOL retVal;
    char readBuf[MAX_STRING_LENGTH];

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SIP-CALL-MODEL",
        &retVal,
        readBuf);

    SipData* sip = (SipData*)node->appData.multimedia->sigPtr;

    if (retVal == TRUE)
    {
        if (strcmp(readBuf, "DIRECT") == 0)
        {
            sip->callModel = SipDirect;
        }
        else if (strcmp(readBuf, "PROXY-ROUTED") == 0)
        {
            sip->callModel = SipProxyRouted;
        }
        else
        {
            // invalid entry in config
            char errorBuf[MAX_STRING_LENGTH];
            sprintf(errorBuf,
                "Invalid call model (%s) for SIP\n", readBuf);
            ERROR_ReportError(errorBuf);
        }
    }
    else
    {
        // if not specified defaulted as SipDirect
        sip->callModel = SipDirect;
    }

    IO_ReadString(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "SIP-TRANSPORT-LAYER-PROTOCOL",
                  &retVal,
                  readBuf);

    if (retVal == TRUE)
    {
        if (!strcmp(readBuf, "TCP"))
        {
            sip->transProto = TCP;
        }
        else if (!strcmp(readBuf, "UDP"))
        {
            sip->transProto = UDP;
        }
        else
        {
            // invalid entry in config
            char errorBuf[MAX_STRING_LENGTH];
            sprintf(errorBuf,
                    "Invalid Transport layer protocol %s\n", readBuf);
            ERROR_ReportError(errorBuf);
        }
    }
    else
    {
        // default value
        sip->transProto = TCP;
    }
}


// /**
// FUNCTION   :: SipReadDNSFile
// LAYER      :: APPLICATION
// PURPOSE    :: Read the input domain name file for domain details
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + nodeInput : NodeInput* : Input config file
// RETURN     :: void : NULL
// **/
void SipData :: SipReadDNSFile(Node* node, const NodeInput* nodeInput)
{
    NodeInput dnsInput;
    BOOL retVal;
    int i;

    NodeAddress proxyId;
    int numHostBits;
    BOOL isNodeId;
    SipDomain* pDomain;

    IO_ReadCachedFile(node->nodeId,
                      ANY_ADDRESS,
                      nodeInput,
                      "DNS-ADDRESS-FILE",
                      &retVal,
                      &dnsInput);

    if (!retVal)
    {
        return;
    }

    ListInit(node, &domainList);

    for (i = 0; i < dnsInput.numLines; i++)
    {
        char domainName[MAX_STRING_LENGTH];
        char proxyIPAddrStr[MAX_ADDRESS_STRING_LENGTH];
        NodeAddress proxyIPAddr;
        int numValues;
        pDomain = NULL;

        numValues = sscanf(dnsInput.inputStrings[i], "%u %s %s",
                           &proxyId, domainName, proxyIPAddrStr);

        if (numValues < 3)
        {
            ERROR_ReportError("Invalid format in DNS address file\n");
        }

        if (node->nodeId == proxyId)
        {
            pDomain = (SipDomain*) MEM_malloc(sizeof(SipDomain));
            memset(pDomain, 0, sizeof(SipDomain));
            memcpy(pDomain->domainName, domainName, strlen(domainName));
            pDomain->proxyUrl = new std::string();
            pDomain->proxyUrl->clear();
            if (ADDR_IsUrl(proxyIPAddrStr))
            {
                  // setting the proxyIPaddr as INVALID as till now it does
                  // not get resolved and once it will get resolved it will be 
                  // used for further VOIP calls also
                 pDomain->proxyIp = INVALID_ADDRESS;
                 if (node->appData.appTrafficSender->ifUrlLocalHost(proxyIPAddrStr) == FALSE)
                 {
                     node->appData.appTrafficSender->appClientSetDnsInfo(
                                        node,
                                        (const char*)proxyIPAddrStr,
                                        pDomain->proxyUrl);
                }
            }
            else //If IP address is present
            {

                // find ip address for this host
                IO_ParseNodeIdHostOrNetworkAddress(proxyIPAddrStr,
                                                   &proxyIPAddr,
                                                   &numHostBits,
                                                   &isNodeId);

                pDomain->proxyIp = proxyIPAddr;
            }
            ListInsert(node, domainList, node->getNodeTime(), (void*)pDomain);
        }
    }
}


// /**
// FUNCTION   :: SipPrintDNSData
// LAYER      :: APPLICATION
// PURPOSE    :: Print DNS information read from the input DNS file
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// RETURN     :: void : NULL
// **/
void SipData::SipPrintDNSData(Node* node)
{
    ListItem* item = domainList->first;
    printf("Domain Name list for Node %u :\n", node->nodeId);
    while (item != NULL)
    {
       SipDomain* sipDomain = (SipDomain*)item->data;
       printf("domain Name = #%s#, ProxyIP Address = %u\n",
       sipDomain->domainName, sipDomain->proxyIp);
       item = item->next;
    }
}


// /**
// FUNCTION   :: SipReadAliasFileForHost
// LAYER      :: APPLICATION
// PURPOSE    :: Read the input alias file for UA node
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + nodeInput : NodeInput* : Input config file
// RETURN     :: void : NULL
// **/
//  Alias file reading for UA's  proxy info requirement
void SipData :: SipReadAliasFileForHost(Node* node,
                                        const NodeInput* nodeInput)
{
    NodeInput aliasInput;
    BOOL retVal;
    int i;

    IO_ReadCachedFile(node->nodeId,
                      ANY_ADDRESS,
                      nodeInput,
                      "TERMINAL-ALIAS-ADDRESS-FILE",
                      &retVal,
                      &aliasInput);

    if (!retVal)
    {
        ERROR_ReportError("Invalid SIP alias file\n");
    }

    for (i = 0; i < aliasInput.numLines; i++)
    {
        NodeAddress hostId;
        NodeAddress proxyId;
        NodeAddress proxyIPAddr = INVALID_ADDRESS;;
        int numHostBits;
        BOOL isNodeId;

        char aliasName[MAX_STRING_LENGTH];
        char domainName[MAX_STRING_LENGTH];
        char hostIpStr[MAX_ADDRESS_STRING_LENGTH];
        char proxyIPAddrStr[MAX_ADDRESS_STRING_LENGTH];
        char aliasDomainAddr[MAX_STRING_LENGTH];
        char fqdn[MAX_STRING_LENGTH];
        memset(aliasDomainAddr, 0, MAX_STRING_LENGTH);
        int numValues;
        //modified for removing SIP proxy list
        char buf[MAX_STRING_LENGTH];

        numValues = sscanf(aliasInput.inputStrings[i], "%u %s %s %s %u %s",
                           &hostId, hostIpStr, aliasName, domainName,
                           &proxyId, proxyIPAddrStr);

        if (numValues < 6)
        {
            ERROR_ReportError("Invalid format in SIP alias file\n");
        }

        if (node->nodeId == hostId)
        {
            proxyURL = new std::string();
            proxyURL->clear();
            //modified for removing SIP proxy list
            //Check if Proxy id is valid proxy or not
            IO_ReadString(
                 proxyId,
                 ANY_ADDRESS,
                 nodeInput,
                 "SIP-PROXY",
                 &retVal,
                 buf);
            if (!retVal || (retVal && !strcmp(buf,"NO")))
            {
                 char error[MAX_STRING_LENGTH];
                 sprintf(error, "\nEither SIP-PROXY not present or proxy id"
                                 " not configured as proxy for node %d",
                                 node->nodeId);
                 ERROR_ReportError(error);
            }
            else
            {
                // set proxy ID for this host
                this->proxyId = proxyId;
            }

            // create alias address from alias name and domain
            sprintf(aliasDomainAddr, "sip:%s@%s", aliasName, domainName);
            SipSetOwnAliasAddr(aliasDomainAddr);
            SipSetDomainName(domainName);

            // set FQDN
            sprintf(fqdn, "%s.%s", aliasName, domainName);
            SipSetFQDN(fqdn);
            // if proxy server is specified with its URL
            if (ADDR_IsUrl(proxyIPAddrStr))
            {
               if (node->appData.appTrafficSender->ifUrlLocalHost(proxyIPAddrStr) == FALSE)
               {
                    node->appData.appTrafficSender->appClientSetDnsInfo(
                                        node,
                                        (const char*)proxyIPAddrStr,
                                        proxyURL);
               }
               // setting the proxyIPaddr as INVALID as till now it does
               // not get resolved and once it will get resolved it will be 
               // used for further VOIP calls also
                proxyIPAddr = INVALID_ADDRESS ;
            }
            else //if IP address is specified
            {
                // set proxy ip address for this host
                IO_ParseNodeIdHostOrNetworkAddress(
                                        proxyIPAddrStr,
                                        &proxyIPAddr,
                                        &numHostBits,
                                        &isNodeId);

                proxyIp = proxyIPAddr;

                // If IP address is present then proxyURL field should 
                // be empty
                proxyURL->clear();
            }
            
            break;
        }
    }
}

static void
SIP_parseAliasLine (char * aliasLine,
    NodeAddress * hostIdPtr, char * hostIpStr, char * aliasName,
    char * domainName, NodeAddress * proxyIdPtr, char * proxyIPAddrStr)
{
    int numValues;
    numValues = sscanf(aliasLine, "%u %s %s %s %u %s",
                       hostIdPtr, hostIpStr, aliasName, domainName,
                       proxyIdPtr, proxyIPAddrStr);

    if (numValues < 6)
    {
        ERROR_ReportError("Invalid format in SIP alias file\n");
    }
}

bool
SIPDATA_fillAliasAddrForNodeId (NodeAddress destNodeId,
    const NodeInput * nodeInput,
    char * destAliasAddrBuf)
    // This function is derived from SipData :: SipReadAliasFileForHost()
    // This funciton only works for hostType of SIP_UA (not SIP_PROXY)
{
    NodeInput aliasInput;
    BOOL retVal;
    int i;


    IO_ReadCachedFile(destNodeId,
                      ANY_ADDRESS,
                      nodeInput,
                      "TERMINAL-ALIAS-ADDRESS-FILE",
                      &retVal,
                      &aliasInput);

    if (!retVal)
    {
        ERROR_ReportError("Invalid SIP alias file\n");
    }

    for (i = 0; i < aliasInput.numLines; i++)
    {
        NodeAddress hostId;
        NodeAddress proxyId;

        char aliasName[MAX_STRING_LENGTH];
        char domainName[MAX_STRING_LENGTH];
        char hostIpStr[MAX_ADDRESS_STRING_LENGTH];
        char proxyIPAddrStr[MAX_ADDRESS_STRING_LENGTH];
        char aliasDomainAddr[MAX_STRING_LENGTH];
        memset(aliasDomainAddr, 0, MAX_STRING_LENGTH);

        SIP_parseAliasLine (aliasInput.inputStrings[i],
                           &hostId, hostIpStr, aliasName, domainName,
                           &proxyId, proxyIPAddrStr);
        if (destNodeId == hostId)
        {
            // create alias address from alias name and domain
            sprintf (destAliasAddrBuf, "sip:%s@%s", aliasName, domainName);
            return true;
        }
    }
    return false;
}


// /**
// FUNCTION   :: SipReadAliasFileForProxy
// LAYER      :: APPLICATION
// PURPOSE    :: Read the alias file for Proxy node
// PARAMETERS ::
// + node      : Node* : Pointer to the node
// + nodeInput : NodeInput* : Input config file
// RETURN     :: void : NULL
// **/
//  Reading alias file for PROXY's  useAgent info requirement
void SipData::SipReadAliasFileForProxy(Node* node,
                                       const NodeInput* nodeInput)
{
    NodeInput aliasInput;
    BOOL retVal;
    int i;

    proxyURL = new std::string();
    proxyURL->clear();
    IO_ReadCachedFile(node->nodeId,
                      ANY_ADDRESS,
                      nodeInput,
                      "TERMINAL-ALIAS-ADDRESS-FILE",
                      &retVal,
                      &aliasInput);

    if (!retVal)
    {
        ERROR_ReportError("Invalid SIP alias file\n");
    }

    for (i = 0; i < aliasInput.numLines; i++)
    {
        NodeAddress hostId;
        NodeAddress proxyId;
        NodeAddress hostIPAddr;
        NodeAddress proxyInterfaceAdd;
        int numHostBits;
        BOOL isNodeId;

        char aliasName[MAX_STRING_LENGTH];
        char domainName[MAX_STRING_LENGTH];
        char hostIpStr[MAX_ADDRESS_STRING_LENGTH];
        char proxyIPAddrStr[MAX_ADDRESS_STRING_LENGTH];
        char aliasDomainAddr[MAX_STRING_LENGTH];
        char fqdn[MAX_STRING_LENGTH];

        SIP_parseAliasLine (aliasInput.inputStrings[i],
                           &hostId, hostIpStr, aliasName, domainName,
                           &proxyId, proxyIPAddrStr);
        // If proxy server is present with the URL then there is no need to
        // get the proxyId on the basis of IP address as now there is no 
        // IP address
        if (!ADDR_IsUrl(proxyIPAddrStr))
        {
            // cross validation of proxyIP and corresponding proxy IP address
            IO_ParseNodeIdHostOrNetworkAddress(
                                            proxyIPAddrStr,
                                            &proxyInterfaceAdd,
                                            &numHostBits,
                                            &isNodeId);
            NodeAddress proxyNodeId =
                MAPPING_GetNodeIdFromInterfaceAddress(
                                                node,
                                                proxyInterfaceAdd);

            if (proxyNodeId != proxyId)
            {
                ERROR_ReportError("Error in default.sip: Proxy nodeId and"
                            " Proxy IP address mismatch\n");
            }
        }


        if (node->nodeId == proxyId)
        {
            // add this user agent info to the proxy block
            SipHostNode hostNode;
            Address ipAddr;

            memset(&hostNode, 0, sizeof(SipHostNode));
            hostNode.hostId = hostId;

            // set host ip address for this host
            IO_ParseNodeIdHostOrNetworkAddress(
                hostIpStr,
                &hostIPAddr,
                &numHostBits,
                &isNodeId);

            hostNode.hostIp = hostIPAddr;

            // create aliasDomain address from alias name and domain
            sprintf(aliasDomainAddr, "sip:%s@%s", aliasName, domainName);
            memcpy(hostNode.hostName, aliasDomainAddr,
                   strlen(aliasDomainAddr));

            // create fqdn
            sprintf(fqdn, "%s.%s", aliasName, domainName);
            ipAddr.networkType = NETWORK_IPV4;
            ipAddr.interfaceAddr.ipv4 = hostIPAddr;

            hostNode.hostInterfaceIndex = 
                    (short)MAPPING_GetInterfaceIndexFromInterfaceAddress(
                                                            node,
                                                            ipAddr);

            if (hostId == proxyId)
            {
               // set proxy ID for this host
                this->proxyId = proxyId;

                // set alias address
                SipSetOwnAliasAddr(aliasDomainAddr);

                // set fqdn
                SipSetFQDN(fqdn);

                SipSetDomainName(domainName);

                // set proxy ip address for this host
                proxyIp = hostIPAddr;
            }
            else
            {
                proxyIp = NetworkIpGetInterfaceAddress(node,
                                                       DEFAULT_INTERFACE);
                // set fqdn
                SipSetFQDN(proxyIPAddrStr);

            }

            if (!SipInsertToHostBlock(&hostNode))
            {
                ERROR_ReportError("Insertion into the "
                    "hostBlock not successful\n");
            }
        }
    }
}


// /**
// FUNCTION   :: SipInsertMsg
// LAYER      :: APPLICATION
// PURPOSE    :: Insert a sip message into the list
// PARAMETERS ::
// + msg       : SipMsg* : Pointer to the sip message
// RETURN     :: void : NULL
// **/
void SipData :: SipInsertMsg(SipMsg* msg)
{
    if (sipMsgList == (SipMsg*) NULL)
    {
        sipMsgList = msg;
    }
    else
    {
        SipMsg* prevMsg = sipMsgList;
        SipMsg* temp = sipMsgList->next;
        while (temp)
        {
            if (temp->SipGetConnectionId() == msg->SipGetConnectionId()
                &&
                temp->SipGetProxyConnectionId() ==
                        msg->SipGetProxyConnectionId())
            {
                // already message stored, don't need to restore it
                return;
            }
            prevMsg = temp;
            temp = temp->next;
        }
        prevMsg->next = msg;
    }
}


// /**
// FUNCTION   :: SipDeleteMsg
// LAYER      :: APPLICATION
// PURPOSE    :: To delete a message from the list
// PARAMETERS ::
// + connectionId      : int : TCP connection Id
// RETURN     :: void : NULL
// **/
void SipData :: SipDeleteMsg(int connectionId)
{
    SipMsg* prevMsg = NULL;
    SipMsg* temp = sipMsgList;
    while (temp)
    {
        if ((temp->SipGetConnectionId() == connectionId) ||
            (temp->SipGetProxyConnectionId() == connectionId))

        {
            // if only one SipMsg
            if (temp == sipMsgList)
            {
                // now SipMsg will be null
                sipMsgList = temp->next;
            }
            else
            {
                prevMsg->next = temp->next;
            }
            delete temp;
            return;
        }
        prevMsg = temp;
        temp = temp->next;
    }
}


// /**
// FUNCTION   :: SipGetMirrorConnectionId
// LAYER      :: APPLICATION
// PURPOSE    :: Get the mirror connction Id for a node
// PARAMETERS ::
// + connectionId     : int  : TCP connection Id
// RETURN     :: int  : mirror connection Id
// **/
int SipData :: SipGetMirrorConnectionId(int connectionId)
{
    SipMsg* temp = sipMsgList;
    while (temp)
    {
        if (temp->SipGetConnectionId() == connectionId)
        {
            return temp->SipGetProxyConnectionId();
        }
        else if (temp->SipGetProxyConnectionId() == connectionId)
        {
            return temp->SipGetConnectionId();
        }
        temp = temp->next;
    }
    return -1;
}


// /**
// FUNCTION   :: SipFindMsg
// LAYER      :: APPLICATION
// PURPOSE    :: search for a sip message in the list
// PARAMETERS ::
// + connectionId : int  : TCP connection Id
// RETURN     :: SipMsg* : Pointer to returned sip message
// **/
SipMsg* SipData :: SipFindMsg(int connectionId)
{
    SipMsg* temp = sipMsgList;
    while (temp)
    {
        if (DEBUG_L2)
        {
            printf("Inside SipData :: SipFindMsg\n");
            printf("input connId: %d, SipMsg ConnId: %d, ProxyConnId: %d\n",
                   connectionId, temp->SipGetConnectionId(),
                   temp->SipGetProxyConnectionId());
        }
        if (temp->SipGetConnectionId() == connectionId)
        {
            return temp;
        }
        else if (temp->SipGetProxyConnectionId() == connectionId)
        {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}


// /**
// FUNCTION   :: SipFindMsg
// LAYER      :: APPLICATION
// PURPOSE    :: search for a sip message in the list
// PARAMETERS ::
// + fromPort   : unsigned short : from port
// RETURN     :: SipMsg* : Pointer to sip message
// **/
SipMsg* SipData :: SipFindMsg(unsigned short fromPort)
{
    SipMsg* temp = sipMsgList;
    while (temp)
    {
        if (temp->SipGetFromPort() == fromPort)
        {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}


// /**
// FUNCTION   :: SipFindMsg
// LAYER      :: APPLICATION
// PURPOSE    :: search for a sip message in the list
// PARAMETERS ::
// + newSipMsg : SipMsg* : Pointer to sip message
// RETURN     :: SipMsg* : Pointer to sip message
// **/
SipMsg* SipData :: SipFindMsg(SipMsg* newSipMsg)
{
    char* to = newSipMsg->SipGetTo();
    char* from = newSipMsg->SipGetFrom();
    char* callId = newSipMsg->SipGetCallId();

    SipMsg* tmpMsg = sipMsgList;

    while (tmpMsg)
    {
        // only compare fromTag and callId : to chk impact when extended
        if (!strcmp(tmpMsg->SipGetFrom(), from) &&
            !strcmp(tmpMsg->SipGetCallId(), callId) &&
            (tmpMsg->SipGetTo() == NULL || to == NULL))
        {
            return tmpMsg;
        }
        // compare all three dialogId fields
        else if (!strcmp(tmpMsg->SipGetTo(), to) &&
                 !strcmp(tmpMsg->SipGetFrom(), from) &&
                 !strcmp(tmpMsg->SipGetCallId(), callId))
        {
            return tmpMsg;
        }
        tmpMsg = tmpMsg->next;
    }
    return NULL;
}


// /**
// FUNCTION   :: SipRemoveAllMsg
// LAYER      :: APPLICATION
// PURPOSE    :: Remove all messages
// PARAMETERS ::
// RETURN     :: void : NULL
// **/
void SipData :: SipRemoveAllMsg()
{
    SipMsg* prevMsg;
    SipMsg* temp = sipMsgList;
    while (temp)
    {
        prevMsg = temp;
        temp = temp->next;
        delete prevMsg;
    }
}


// /**
// FUNCTION   :: SipFindNextState
// LAYER      :: APPLICATION
// PURPOSE    :: Return a unique number
// PARAMETERS ::
// + node      : Node*        : Pointer to the node
// RETURN     :: unsigned int : generated number
// **/
unsigned int SipData :: SipFindNextState(Node* node)
{
    int cell;

    if (ruleVector == NULL && seedState == NULL && count == 0)
    {
        sizeOfCA = CA_SIZE;
        ruleVector = (int *)MEM_malloc(sizeOfCA * sizeof(int));
        seedState  = (unsigned int *)MEM_malloc(sizeOfCA * sizeof(int));
        nextState  = (int *)MEM_malloc(sizeOfCA * sizeof(int));

        SipInitRuleVectorAndSeedState(sizeOfCA, ruleVector, seedState,node);
    }

    if (count == 0)
    {
        for (cell = 0; cell < sizeOfCA; cell++)
        {
            nextState[cell] = seedState[cell];
        }
        count++;
        return (SipBinToDec(CA_SIZE,nextState));
    }
    else if ((count == (unsigned int)(((unsigned int)~0) >>
            (8 * sizeof(unsigned int) - sizeOfCA))))
    {
        ERROR_ReportError("Max state already generated, repeat ahead\n");
    }

    count++; // Increment the state counter

    for (cell = 0; cell < sizeOfCA; cell++)
    {
        int leftCell;
        int currentCell;
        int rightCell;
        int position;

        if (cell == 0)
        {
            rightCell = 0;
        }
        else
        {
            rightCell = seedState[cell-1];
        }

        currentCell = seedState[cell];

        if (cell == (sizeOfCA-1))
        {
            leftCell = 0;
        }
        else
        {
            leftCell = seedState[cell + 1];
        }

        position = rightCell + 2 * currentCell + 4 * leftCell;

        nextState[cell] = (ruleVector[cell] & (1<<position))>>position;

        if ((nextState[cell] != 0) && (nextState[cell] != 1))
        {
            ERROR_ReportError("Incorrect state value generated\n");
        }
    }

    for (cell = 0; cell < sizeOfCA; cell++)
    {
        seedState[cell] = nextState[cell];
    }
    return (SipBinToDec(CA_SIZE,nextState));
}


// /**
// FUNCTION   :: SipInitRuleVectorAndSeedState
// LAYER      :: APPLICATION
// PURPOSE    :: Seed and initialize the generator
// PARAMETERS ::
// + noOfCell  : int : Number of cells in the generator
// + ruleVector: int*: Rule vector pointer to be passed
// + seedState : unsigned * : State of the seed
// + node      : Node*      : Pointer to the node
// RETURN     :: void : NULL
// **/
void SipData :: SipInitRuleVectorAndSeedState(int noOfCell,
                                              int *ruleVector,
                                              unsigned *seedState,
                                              Node* node)
{
    while (noOfCell--)
    {
        ruleVector[noOfCell] = RULE_VECTOR_VAL1;
        seedState[noOfCell] = RANDOM_nrand(seed) % 2;

        //for 32 bit maximum length CA
        ruleVector[17] = RULE_VECTOR_VAL2;
        ruleVector[31] = RULE_VECTOR_VAL2;
    }

}


// /**
// FUNCTION   :: SipBinToDec
// LAYER      :: APPLICATION
// PURPOSE    :: Convert binary to decimal number
// PARAMETERS ::
// + size      : int : size of the generated number
// + bin       : int*: input binary number
// RETURN     :: unsigned int : decimal number required
// **/
unsigned int SipData :: SipBinToDec(int size, int *bin)
{
    unsigned int dec = 0;
    int i;

    for (i = 0; i < size; i++)
    {
        if (bin[i] == 0 || bin[i] == 1)
        {
            dec += (1<<i) * bin[i];
        }
        else
        {
            ERROR_ReportError("Invalid bit Value\n");
        }
    }
    return (dec);
}


// /**
// FUNCTION   :: SipGetDestProxyIp
// LAYER      :: APPLICATION
// PURPOSE    :: Get Destination Proxy Ip address
// PARAMETERS ::
// + domainName: char* : domain name
// + proxyIp   : unsigned: proxy IP address
// RETURN     :: unsigned int : decimal number required
// **/
void SipData :: SipGetDestProxyIp(char* sipTo, NodeAddress& proxyIp)
{
    proxyIp = INVALID_ADDRESS;
    char dName[MAX_STRING_LENGTH];
    ListItem* item;
    char* tmp = strchr(sipTo, '@');

    if (!tmp)
    {
        ERROR_ReportError("Invalid Domain name !\n");
    }
    else
    {
        strcpy(dName, tmp + 1);
    }

    if (!domainList)
    {
        return;
    }
    item = domainList->first;
    while (item != NULL)
    {
       SipDomain* sipDomain = (SipDomain*) item->data;
       if (!strcmp(sipDomain->domainName, dName))
       {
            proxyIp = sipDomain->proxyIp;
            break;
       }
       item = item->next;
    }
}
//---------------------------------------------------------------------------
// FUNCTION   :: SipGetDestProxyUrl
// LAYER      :: APPLICATION
// PURPOSE    :: Get Destination Proxy Url address
// PARAMETERS ::
// + domainName: char* : domain name
// + proxyUrl   : char*: proxy Url address
// RETURN     :: unsigned int : decimal number required
//---------------------------------------------------------------------------
void SipData :: SipGetDestProxyUrl(char* sipTo, char* proxyUrl)
{
    proxyUrl[0] = '\0';
    char dName[MAX_STRING_LENGTH];
    ListItem* item;
    char* tmp = strchr(sipTo, '@');

    if (!tmp)
    {
        ERROR_ReportError("Invalid Domain name !\n");
    }
    else
    {
        strcpy(dName, tmp + 1);
    }

    if (!domainList)
    {
        return;
    }

    item = domainList->first;
    while (item != NULL)
    {
        SipDomain* sipDomain = (SipDomain*) item->data;
        if (!strcmp(sipDomain->domainName, dName))
        {
            strcpy(proxyUrl , sipDomain->proxyUrl->c_str() );
            break;
        }
        item = item->next;
    }
}

//---------------------------------------------------------------------------
// FUNCTION   :: SipUpdateTargetIp
// LAYER      :: APPLICATION
// PURPOSE    :: update target ip
// PARAMETERS ::
// + fromPort   : unsigned short: from port
// + targetIp   : NodeAddress   : target ip
// + targetUrl  : char*         : target url
// RETURN     :: NONE
//---------------------------------------------------------------------------
void SipData :: SipUpdateTargetIp(unsigned short fromPort,
                                  NodeAddress targetIp,
                                  char* targetUrl )
{
    SipMsg* temp = sipMsgList;
    while (temp)
    {
        if (!strcmp(temp->SipGetTargetUrl() , targetUrl) &&
            temp->SipGetTargetIp() == INVALID_ADDRESS &&
            temp->SipGetFromPort() == fromPort)
        {
            temp->SipSetTargetIp( targetIp ) ;
        }
        temp = temp->next;
    }
}
