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

// This file contains initialization function, message processing
// function, and finalize function used by mobile ip.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "partition.h"
#include "transport_udp.h"
#include "network_ip.h"
#include "network_icmp.h"
#include "message.h"
#include "network_mobileip.h"

#define MOBILE_IP_DEBUG                     0
#define MOBILE_IP_DEBUG_DETAIL              0
#define MOBILE_IP_DEBUG_OUTPUT              0
#define MOBILE_IP_DEBUG_FORWARDING_TABLE    0
#define SIZE_REGISTRATIONREPLY    44

static
BOOL MobileIpValidityCheck(
    Node* ,
    Message* ,
    MobileIpRegMessageType ,
    NodeAddress ,
    NodeAddress ,
    int , MobileIpRegistrationReply* replyPktStruct = NULL);

// FUNCTION:   MobileIpPrintBindingList()
// PURPOSE:
//
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
static
void MobileIpPrintBindingList(Node* node)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
            ipLayer->mobileIpStruct;

    LinkedList* infoList;
    ListItem* item;
    HomeRegistrationList* regList;

    // Binding List maintains only for HomeAgent or BothAgent.
    if (!(mobileIp->agent == BothAgent || mobileIp->agent == HomeAgent))
    {
        return;
    }

    infoList = (LinkedList*) mobileIp->homeAgentInfoList;
    item = (ListItem*)infoList->first;

    printf("\n ____________________________________________________________"
           "______\n");
    printf("|\t\tMobility Binding list of node %d:\t\t   |\n", node->nodeId);
    printf("|______________________________________________________________"
           "____|\n");
    printf("|\tHomeAddess\t|\tCareOfAddress\t|\tLifeTime   |\n");
    printf("|______________________________________________________________"
           "____|\n");

    while (item != NULL)
    {
        regList = (HomeRegistrationList*) item->data;

        printf("|\t%x\t\t|\t%x\t\t|\t%d\t   |\n",
            regList->homeAddress,
            regList->careOfAddress,
            regList->remainingLifetime);

        item = item->next;
    }
    printf("|______________________________________________________________"
           "____|\n\n");
}


// FUNCTION:   MobileIpPrintVisitorList()
// PURPOSE:
//
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
static
void MobileIpPrintVisitorList(Node* node)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
            ipLayer->mobileIpStruct;

    ListItem* item;
    LinkedList* infoList = mobileIp->foreignAgentInfoList;
    ForeignRegistrationList* regList;

    // VisitorList List maintains only for ForeignAgent or BothAgent.
    if (!(mobileIp->agent == BothAgent || mobileIp->agent == ForeignAgent))
    {
        return;
    }

    item = (ListItem*) infoList->first;

    printf("\n ____________________________________________________________"
           "___\n");
    printf("|\t\tMobility Visitor list of node %d:\t\t|\n", node->nodeId);
    printf("|______________________________________________________________"
           "_|\n");
    while (item != NULL)
    {
        char clockStr[MAX_CLOCK_STRING_LENGTH];
        regList = (ForeignRegistrationList*) item->data;
        ctoa(regList->identNumber, clockStr);

        printf("|__________________________________________________________"
               "_____|\n");
        printf("|\t sourceAddress\t\t->\t\t%x\t\t|\n"
            "|\t destinationAddress\t->\t\t%x\t\t|\n"
            "|\t homeAgent\t\t->\t\t%x\t\t|\n"
            "|\t udpSourcePort\t\t->\t\t%d\t\t|\n"
            "|\t identNumber\t\t->\t\t%s\t|\n"
            "|\t registrationLifetime\t->\t\t%d\t\t|\n"
            "|\t remainingLifetime\t->\t\t%d\t\t|\n",
            regList->sourceAddress,
            regList->destinationAddress,
            regList->homeAgent,
            regList->udpSourcePort,
            clockStr,
            regList->registrationLifetime,
            regList->remainingLifetime);
        printf("|__________________________________________________________"
               "_____|\n");
        item = item->next;
    }
    printf("|______________________________________________________________"
           "_|\n\n");
}


// FUNCTION:   MobileIpSetDefaultRoute()
// PURPOSE:
//
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
static
void MobileIpUpdateForwardingTable(
    Node *node,
    NodeAddress destAddress,
    NodeAddress destAddressMask,
    NodeAddress nextHopAddress,
    int outgoingInterfaceIndex,
    int cost,
    NetworkRoutingProtocolType type)
{
    NetworkUpdateForwardingTable(
        node,
        destAddress,
        destAddressMask,
        nextHopAddress,
        outgoingInterfaceIndex,
        cost,
        type);
}


// FUNCTION:   MobileIpStatsInit()
// PURPOSE:    This function is called from MobileIpInit() function to
//             initialize all the statistics variables/flags mobile IP.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
//             nodeInput:     Structure containing contents of input file.
static
void MobileIpStatsInit(Node* node, const NodeInput* nodeInput)
{
    BOOL retVal;
    char buf[80];

    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    MobileIpStatistics* mobileIpStatistics = (MobileIpStatistics*)
        MEM_malloc (sizeof (MobileIpStatistics));

    mobileIp->mobileIpStat = mobileIpStatistics;

    IO_ReadString(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "MOBILE-IP-STATISTICS",
                  &retVal,
                  buf);

    if ((retVal == FALSE) || (strcmp(buf, "NO") == 0))
    {
        mobileIp->collectStatistics = FALSE;
    }
    else if (strcmp(buf, "YES") == 0)
    {
        mobileIp->collectStatistics = TRUE;
    }

    memset(mobileIp->mobileIpStat, 0, sizeof(MobileIpStatistics));
}


// FUNCTION:   MobileIpInitializeMobileIp()
// PURPOSE:    This function is called from MobileIpReadAgentType()
//             function to initialize mobile IP Structure.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
//             nodeInput:     Structure containing contents of input file.
static
void MobileIpInitializeMobileIp(
    Node* node,
    const NodeInput* nodeInput,
    AgentType agentType)
{
    NetworkDataMobileIp* mobileIpStruct;
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;

    if (ipLayer->mobileIpStruct != NULL)
    {
        ERROR_Assert(FALSE, "Invalid MEM_malloc for mobileIpStruct,"
            " program terminated");
    }

    mobileIpStruct = (NetworkDataMobileIp*)
            MEM_malloc (sizeof (NetworkDataMobileIp));

    // Set mobile IP flag and prepare the structure for mobile ip
    ipLayer->mobileIpStruct = mobileIpStruct;

    // Initialize sequenceNumber, agentType and foreignAgentType
    mobileIpStruct->sequenceNumber = 0;
    mobileIpStruct->agent = agentType;
    mobileIpStruct->foreignAgentType = None;

    // Initialize random seed.
    RANDOM_SetSeed(mobileIpStruct->seed,
                   node->globalSeed,
                   node->nodeId,
                   NETWORK_PROTOCOL_MOBILE_IP);

    // Initialize all the state variables for statistics
    MobileIpStatsInit(node, nodeInput);
}


// FUNCTION:   MobileIpParseInputString()
// PURPOSE:    This function is called from MobileIpReadAgentType()
//             function to determine the node is a mobile node or a home
//             agent or foreign agent.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
//             nodeInput:     Structure containing contents of input file.
//             index          line index where matches the keyword
static
BOOL MobileIpParseInputString(Node* node, char* currentLine)
{
    // For IO_GetDelimitedToken
    char iotoken[MAX_STRING_LENGTH];
    char* next;
    int i;
    unsigned int num;
    unsigned int maxNodeId;
    char* token;
    char* p;
    char mobileIpString[MAX_STRING_LENGTH];
    const char* delims = "{,} \n";

    for (i = 0; i < MAX_STRING_LENGTH; i++)
    {
        if (currentLine[i] != '}')
        {
            mobileIpString[i] = currentLine[i];
        }
        else
        {
            mobileIpString[i] = '}';
            mobileIpString[i + 1] = '\0';
            break;
        }
    }

    p = mobileIpString;
    p = strchr(p, '{');

    if (p == NULL)
    {
        char errorBuf[MAX_STRING_LENGTH];
        sprintf(errorBuf, "Could not find '{' character:\n in %s\n",
                           mobileIpString);
        ERROR_Assert(FALSE, errorBuf);
    }
    else
    {
        token = IO_GetDelimitedToken(iotoken, p, delims, &next);

        if (!token)
        {
            char errorBuf[MAX_STRING_LENGTH];
            sprintf(errorBuf, "Can't find nodeId list,"
                " e.g. { 1, 2, ... }:in \n  %s\n", mobileIpString);
            ERROR_Assert(FALSE, errorBuf);
        }
        else
        {
            while (1)
            {
                num = atoi(token);
                if (num == node->nodeId)
                {
                    return TRUE;
                }//if//

                token = IO_GetDelimitedToken(
                            iotoken, next, delims, &next);

                if (!token)
                {
                    break;
                }

                if (IO_CaseInsensitiveStringsAreEqual(
                        "thru", token, 4))
                {
                    token = IO_GetDelimitedToken(
                            iotoken, next, delims, &next);

                    maxNodeId = atoi(token);
                    num++;

                    while (num <= maxNodeId)
                    {
                        if (num == node->nodeId)
                        {
                            return TRUE;
                        }//if//

                        num++;

                    }//while//

                      token = IO_GetDelimitedToken(
                              iotoken, next, delims, &next);

                    if (!token)
                    {
                        break;
                    }
                }//if//
            }//while//
        }//else//
    }//else//

    return FALSE;
}


// FUNCTION:   MobileIpAssignHomeAgentForMobileNode()
// PURPOSE:
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
//             nodeInput:     Structure containing contents of input file.
static
NodeAddress MobileIpAssignHomeAgentForMobileNode(
    Node* node,
    char* currentLine)
{
    // For IO_GetDelimitedToken
    char iotoken[MAX_STRING_LENGTH];
    char* next;
    int i;
    unsigned int homeNodeId = 0;
    char* token;
    char* p;
    char mobileIpString[MAX_STRING_LENGTH];
    const char* delims = "{,} \n";

    for (i = 0; i < MAX_STRING_LENGTH; i++)
    {
        if (currentLine[i] != '}')
        {
            mobileIpString[i] = currentLine[i];
        }
        else
        {
            mobileIpString[i] = '}';
            mobileIpString[i + 1] = '\0';
            break;
        }
    }

    p = mobileIpString;
    p = strchr(p, '{');


    if (p == NULL)
    {
        char errorBuf[MAX_STRING_LENGTH];
        sprintf(errorBuf, "Could not find '{' character:\n in %s\n",
                           mobileIpString);
        ERROR_Assert(FALSE, errorBuf);
    }
    else
    {
        token = IO_GetDelimitedToken(iotoken, p, delims, &next);

        if (!token)
        {
            char errorBuf[MAX_STRING_LENGTH];
            sprintf(errorBuf, "Can't find nodeId list,"
                " e.g. { 1, 2, ... }:in \n  %s\n", mobileIpString);
            ERROR_Assert(FALSE, errorBuf);
        }
        else
        {
            homeNodeId = atoi(token);
        }
    }

    return MAPPING_GetInterfaceAddressForInterface(node,homeNodeId,DEFAULT_INTERFACE);


}


// FUNCTION:   MobileIpReadCurrentLine()
// PURPOSE:
//
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
//             nodeInput:     Structure containing contents of input file.
static
BOOL MobileIpReadCurrentLine(
    Node* node,
    const NodeInput* nodeInput,
    const char *parameterName,
    BOOL *wasFound,
    char *parameterValue)
{
    int i;
    for (i = 0; i < node->numberInterfaces; i++)
    {

        IO_ReadLine(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, i),
            nodeInput,
            parameterName,
            wasFound,
            parameterValue);

        if (*wasFound)
        {
            return TRUE;
        }
    }
    return FALSE;
}


// FUNCTION:   MobileIpReadAgentType()
// PURPOSE:    This function is called from MobileIpInit()
//             function to determine the node is a mobile node or a home
//             agent or foreign agent.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
//             nodeInput:     Structure containing contents of input file.
static
AgentType MobileIpReadAgentType(
    Node* node,
    const NodeInput* nodeInput,
    NodeAddress* homeAgentAdder)
{
    BOOL isIcmpRouterAlso = FALSE;
    BOOL retVal = FALSE;
    char icmpRouterString[MAX_STRING_LENGTH];
    char currentLine[MAX_STRING_LENGTH];
    AgentType agentType = Others;

    if (MobileIpReadCurrentLine(
            node,
            nodeInput,
            "HOME-AGENT",
            &retVal,
            currentLine))
    {
        if (MobileIpParseInputString(node, currentLine))
        {
            // node is a home agent
            agentType = HomeAgent;

        }
    }

    //Multiple foreign agent in one subnet is not handelled.
    if (MobileIpReadCurrentLine(
            node,
            nodeInput,
            "FOREIGN-AGENT",
            &retVal,
            currentLine))
    {
        if (MobileIpParseInputString(node, currentLine))
        {
            if (agentType == Others)
            {
                // node is a foreign agent
                agentType = ForeignAgent;
            }
            else if (agentType == HomeAgent)
            {
                agentType = BothAgent;
            }
            else
            {
                char errorBuf[MAX_STRING_LENGTH];
                sprintf(errorBuf, "Node %u: Agent can't be mobile\n",
                                   node->nodeId);
                ERROR_ReportError(errorBuf);
            }
        }
    }

    if (MobileIpReadCurrentLine(
            node,
            nodeInput,
            "MOBILE-NODE",
            &retVal,
            currentLine))
    {
        if (MobileIpParseInputString(node, currentLine))
        {
            if (agentType != Others)
            {
                char errorBuf[MAX_STRING_LENGTH];
                sprintf(errorBuf, "Node %u: Agent can't be mobile\n",
                                   node->nodeId);
                ERROR_ReportError(errorBuf);
            }
            else
            {
                // node is a mobile node
                agentType = MobileNode;

                // Multiple home agent in one subnet is not supported.
                // User can specyfy. Mobile node allyase take first specified
                // node as default home agent

                if (MobileIpReadCurrentLine(
                        node,
                        nodeInput,
                        "HOME-AGENT",
                        &retVal,
                        currentLine))
                {
                    *homeAgentAdder = MobileIpAssignHomeAgentForMobileNode(
                        node,
                        currentLine);
                }
                else
                {
                    char errorBuf[MAX_STRING_LENGTH];

                    sprintf(errorBuf, "Can not find home agent for mobile node %d\n",
                        node->nodeId);

                    ERROR_ReportWarning(errorBuf);
                }
            }
        }
    }

   // If the node is a mobility agent then check existence of the node in
   // ICMP router list
    if (agentType != Others)
    {
        MobileIpInitializeMobileIp(
            node,
            nodeInput,
            agentType);

        if (MOBILE_IP_DEBUG || MOBILE_IP_DEBUG_DETAIL)
        {
            const char* agentArr[5] = {"BothAgent",
                                 "Home Agent",
                                 "Foreign Agent",
                                 "Mobile Node",
                                 "Non_mobile Node"};

            printf("Node %u is a %s\n", node->nodeId,
                agentArr[agentType]);
        }

        IO_ReadLine(node->nodeId,
                    ANY_ADDRESS,
                    nodeInput,
                    "ICMP-ROUTER-LIST",
                    &retVal,
                    icmpRouterString);

        if (retVal)
        {
            if (MobileIpParseInputString(node, icmpRouterString))
            {
                // Node is in the ICMP router list also
                isIcmpRouterAlso = TRUE;
            }
        }

        if ((agentType != MobileNode) && (!isIcmpRouterAlso))
        {
            char errorBuf[MAX_STRING_LENGTH];
            sprintf(errorBuf, "Node %u is invalid Foreign or Home agent\n",
                node->nodeId);
            ERROR_ReportError(errorBuf);
        }
    }

    return agentType;
}


// FUNCTION:   MobileIpHomeAgentInit()
// PURPOSE:    This function is called from MobileIpInit()
//             function to initialize a home or foreign agent
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
//             nodeInput:     Structure containing contents of input file.
static
void MobileIpHomeAgentInit(Node* node, const NodeInput* nodeInput)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    // Initialize the list for registration list
    ListInit(node, &mobileIp->homeAgentInfoList);

    // Initialize the list for mobile node information
    ListInit(node, &mobileIp->homeAgent);

    // MobileIP agent must support All mobility agent multicast address (224.0.0.11)
    NetworkIpAddToMulticastGroupList(node, ALL_MOBILITY_AGENT_MULTICAST_ADDRESS);
}


// FUNCTION:   MobileIpForeignAgentInit()
// PURPOSE:    This function is called from MobileIpInit()
//             function to initialize a home or foreign agent
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
//             nodeInput:     Structure containing contents of input file.
static
void MobileIpForeignAgentInit(Node* node, const NodeInput* nodeInput)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
            ipLayer->mobileIpStruct;

    // Initialize the list for registration list
    ListInit(node, &mobileIp->foreignAgentInfoList);

    // MobileIP agent must support All mobility agent multicast address (224.0.0.11)
    NetworkIpAddToMulticastGroupList(node, ALL_MOBILITY_AGENT_MULTICAST_ADDRESS);
}


// FUNCTION:   MobileIpMobileNodeStoreFields()
// PURPOSE:    This function stores the agent advertisement information in
//             the mobile node.
// RETURN:     NULL
// ASSUMPTION: None
// PARAMETERS: node:               node for which info. is stored
//             sourceAddress:      source address for the receiving packet
//             destinationAddress: dest. address for the receiving packet
//             interfaceId:        interface id at which message comes
static
void MobileIpMobileNodeStoreFields(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int interfaceId)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    MobileNodeInfoList* mobileNode = mobileIp->mobileNodeInfoList;

    // Set registration lifetime with value from the Adv. Message
    // extension set mobile node home address in MobileNodeInfoList structure
    // set identification Number for agent advertisement in the above structure
    IcmpData* icmpData = (IcmpData*) MESSAGE_ReturnPacket(msg);
    AgentAdvertisementExtension* advExt = (AgentAdvertisementExtension*)
        (((char*) icmpData) + sizeof(IcmpData));

    mobileIp->sequenceNumber = advExt->sequenceNumber;
    mobileNode->homeAddress = NetworkIpGetInterfaceAddress(
                                  node, interfaceId);

    // store the registration lifetime, if it is more than 30 Seconds then set
    // 30 Seconds.
    if (advExt->registrationLifetime > REGISTRATION_LIFETIME)
    {
        mobileNode->registrationLifetime = REGISTRATION_LIFETIME;
    }
    else
    {
        mobileNode->registrationLifetime = advExt->registrationLifetime;
    }

    if (AgentAdvertisementExtensionGetHAgent(advExt->agentAdvExt))
    {
        // mobile node receives advertisement from home, which was previously
        // in the foreign network. That means mobile node now moves to the home
        // network, then ask for de-registration from the foreign network
        if (mobileIp->foreignAgentType != None)
        {
            // go for de-registration, lifetime set 0
            mobileNode->registrationLifetime = DEREGISTRATION_LIFETIME;

            // de-registration of the node, set care of address with
            // home address
            mobileNode->careOfAddress = mobileNode->homeAddress;
        }
        else
        {
            mobileNode->careOfAddress = 0; //CHECK
        }

       // set Home agent address in MobileNodeInfoList with source address of
       // Advertisement message set destination address in MobileNodeInfoList
       // with source address of Advertisement message
        //mobileNode->homeAgent = sourceAddress;

        // mobile node treated as in the home network
        mobileIp->foreignAgentType = None;
    }
    else if (AgentAdvertisementExtensionGetFAgent(advExt->agentAdvExt))
    {
        // set the care of address in MobileNodeInfoList with source
        // address of  Adv. Message
        if ((!mobileNode->homeAgent) ||
            (mobileNode->careOfAddress != advExt->careOfAddress))
        {
            //mobileNode->homeAgent = ANY_DEST;
        }

        mobileNode->careOfAddress = advExt->careOfAddress;

        if (AgentAdvertisementExtensionGetRegReq(advExt->agentAdvExt))
        {
            // mobile node in foreign network with foreign agent address
            mobileIp->foreignAgentType = ForeignAgentAddress;
        }
        else
        {
            // mobile node in foreign network with collocated foreign agent
            mobileIp->foreignAgentType = ColocatedAddress;
        }
    }

    // remaining lifetime initially set to the current lifetime
    mobileNode->remainingLifetime =
    (unsigned short) (node->getNodeTime() / SECOND) +
    mobileNode->registrationLifetime;

    // set destination address in MobileNodeInfoList with source address of
    // Advertisement message
    mobileNode->destinationAddress = sourceAddress;
}


// FUNCTION:   MobileIpCreateAgntAdvTimerMsg()
// PURPOSE:
//
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
static
void MobileIpCreateAgntAdvTimerMsg(
    Node* node,
    unsigned short agentAdvLifeTime,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int interfaceId)
{
    Message* agentAdvMsg;
    AgentTimerInfo* agentAdv;

    agentAdvMsg = MESSAGE_Alloc(node,
                                NETWORK_LAYER,
                                NETWORK_PROTOCOL_MOBILE_IP,
                                MSG_NETWORK_AgentAdvertisementRefreshTimer);

    MESSAGE_InfoAlloc(node, agentAdvMsg, sizeof(AgentTimerInfo));

    agentAdv = (AgentTimerInfo*) MESSAGE_ReturnInfo(agentAdvMsg);

    // set remaining lifetime as current time with registration lifetime
    agentAdv->remainingLifetime =
    (unsigned short) (node->getNodeTime() / SECOND) + agentAdvLifeTime;

    // agent advertisement lifetime is sent for next use
    agentAdv->advMsgTimerVal = agentAdvLifeTime;

    //MESSAGE_Send(node, agentAdvMsg, ICMP_AD_LIFE_TIME);
    MESSAGE_Send(node,agentAdvMsg,agentAdv->advMsgTimerVal * SECOND);
}


// FUNCTION:   MobileIpCreateAndSendRegReq()
// PURPOSE:    Mobile node would send registration request to Home Agent or
//             Foreign Agent when accepting advertisement message from that
//             agent. This function has been set for each mobile node at the
//             time of initialization. It is called from ICMP code when a
//             mobile node receives agent advertisement.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:              node which is allocating message
//             nodeInput:         Structure containing contents of input file.
//             sourceAddress:     source address of this message
//             destinationAddress:Ip address in which interface this message
//                                has been come.
static
void MobileIpCreateAndSendRegReq(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int interfaceId)
{
    Message* requestMsg;
    int numHostBits;
    NodeAddress networkAddress;
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    SrcDestAddrInfo* sdAddr;
    IcmpData* icmpData = (IcmpData*) MESSAGE_ReturnPacket(msg);

    AgentAdvertisementExtension* advExt = (AgentAdvertisementExtension*)
        (((char*) icmpData) + sizeof(IcmpData));

    int index = 0;
    BOOL isHomeNetwork = FALSE;
    NodeAddress agentId;
    Node* agentNode = NULL;
    NodeAddress aNetworkAddress;
    IdToNodePtrMap* nodeHash = node->partitionData->nodeIdHash;
    NodeAddress agentAddress = icmpData->RouterAdvertisementData.routerAddress;

    // ICMP router advertisement comes from a ICMP router which does not
    // support mobile IP.
    if (sizeof(IcmpData) == MESSAGE_ReturnPacketSize(msg))
    {
        MESSAGE_Free(node, msg);
        return;
    }

    agentId = MAPPING_GetNodeIdFromInterfaceAddress(node, agentAddress);
    agentNode = MAPPING_GetNodePtrFromHash(nodeHash, agentId);

#ifdef PARALLEL //Parallel 
    if (agentNode == NULL) {
        agentNode = MAPPING_GetNodePtrFromHash(
            node->partitionData->remoteNodeIdHash,
            agentId);
        assert(agentNode != NULL);
    }
#else  //elseParallel
    ERROR_Assert((agentId != NULL),
                 "Agent node doesn't exist");
#endif //endParallel


    for (; index < agentNode->numberInterfaces; index++)
    {
        int aNumHostBits = MAPPING_GetNumHostBitsForInterface(node, agentId, index);
        aNetworkAddress = NetworkIpGetInterfaceNetworkAddress(
                                         node, DEFAULT_INTERFACE);
        NodeAddress address = MAPPING_GetInterfaceAddressForInterface(
                                node,
                                agentId,
                                index);

        // When mobile node is in Home subnet and ICMP router
        // advertisement comes from a mobility agent which serves
        // home as well as foreign agent.
        if (IsIpAddressInSubnet(
                address,
                aNetworkAddress,
                aNumHostBits))
        {
            isHomeNetwork = TRUE;
        }
    }

    numHostBits = NetworkIpGetInterfaceNumHostBits(node, interfaceId);

    networkAddress = NetworkIpGetInterfaceNetworkAddress(
                                     node, interfaceId);

    // ICMP router advertisement comes from a mobility agent which serves
    // home as well as foreign agent.
    if (AgentAdvertisementExtensionGetHAgent(advExt->agentAdvExt)
        && AgentAdvertisementExtensionGetFAgent(advExt->agentAdvExt))
    {
        // When mobile node is in Home subnet and ICMP router
        // advertisement comes from a mobility agent which serves
        // home as well as foreign agent.
        if (isHomeNetwork)
        {
            if (IsIpAddressInSubnet(
                    icmpData->RouterAdvertisementData.routerAddress,
                    networkAddress,
                    numHostBits))
            {
                AgentAdvertisementExtensionSetFAgent(&(advExt->agentAdvExt),
                    0);
                AgentAdvertisementExtensionSetRegReq(&(advExt->agentAdvExt),
                    0);
                advExt->careOfAddress = 0;

                MobileIpUpdateForwardingTable(node,
                                             0,
                                             0,
                                             sourceAddress,
                                             DEFAULT_INTERFACE,
                                             ROUTING_ADMIN_DISTANCE_DEFAULT,
                                             ROUTING_PROTOCOL_DEFAULT);

                if (MOBILE_IP_DEBUG_DETAIL)
                {
                    printf(" Node %u set %x as home agent\n",
                        node->nodeId, sourceAddress);
                }

                if (MOBILE_IP_DEBUG_FORWARDING_TABLE)
                {
                    NetworkPrintForwardingTable(node);
                }
            }
            else
            {
                MESSAGE_Free(node, msg);
                return;
            }
        }
        // When mobile node is in Foreign subnet and ICMP router
        // advertisement comes from a mobility agent which serves
        // home as well as foreign agent.
        else
        {
            AgentAdvertisementExtensionSetHAgent(&(advExt->agentAdvExt), 0);

            MobileIpUpdateForwardingTable(node,
                                         0,
                                         0,
                                         sourceAddress,
                                         DEFAULT_INTERFACE,
                                         ROUTING_ADMIN_DISTANCE_DEFAULT,
                                         ROUTING_PROTOCOL_DEFAULT);
            if (MOBILE_IP_DEBUG_DETAIL)
            {
                printf(" Node %u set %x as foreign agent\n",
                    node->nodeId, sourceAddress);
            }

            if (MOBILE_IP_DEBUG_FORWARDING_TABLE)
            {
                NetworkPrintForwardingTable(node);
            }
        }
    }
    else if (AgentAdvertisementExtensionGetHAgent(advExt->agentAdvExt))
    // Mobile node discards all ICMP router advertisement comes from foreign
    // agent when it is in home subnet. On the other hand it discards all
    // ICMP router advertisement comes from home agent when it is in foreign
    // subnet.
    {
        if ((!isHomeNetwork) || (!IsIpAddressInSubnet(
                icmpData->RouterAdvertisementData.routerAddress,
                networkAddress,
                numHostBits)))
        {
            MESSAGE_Free(node, msg);
            return;
        }

        MobileIpUpdateForwardingTable(node,
                                     0,
                                     0,
                                     sourceAddress,
                                     DEFAULT_INTERFACE,
                                     ROUTING_ADMIN_DISTANCE_DEFAULT,
                                     ROUTING_PROTOCOL_DEFAULT);

        if (MOBILE_IP_DEBUG_DETAIL)
        {
            printf(" Node %u set %x as home agent\n",
                node->nodeId, sourceAddress);
        }

        if (MOBILE_IP_DEBUG_FORWARDING_TABLE)
        {
            NetworkPrintForwardingTable(node);
        }
    }
    else if (AgentAdvertisementExtensionGetFAgent(advExt->agentAdvExt))
    {
        if (isHomeNetwork || IsIpAddressInSubnet(
                icmpData->RouterAdvertisementData.routerAddress,
                networkAddress,
                numHostBits))
        {
            MESSAGE_Free(node, msg);
            return;
        }

        MobileIpUpdateForwardingTable(node,
                                     0,
                                     0,
                                     sourceAddress,
                                     DEFAULT_INTERFACE,
                                     ROUTING_ADMIN_DISTANCE_DEFAULT,
                                     ROUTING_PROTOCOL_DEFAULT);

        if (MOBILE_IP_DEBUG_DETAIL)
        {
            printf("Node %u set %x as foreign agent\n",
                node->nodeId, sourceAddress);
        }

        if (MOBILE_IP_DEBUG_FORWARDING_TABLE)
        {
            NetworkPrintForwardingTable(node);
        }
    }

    if ((mobileIp->sequenceNumber < advExt->sequenceNumber &&
        advExt->sequenceNumber > 256))
    {
        MESSAGE_Free(node, msg);
        mobileIp->sequenceNumber = advExt->sequenceNumber;
        return;
    }

    mobileIp->sequenceNumber = advExt->sequenceNumber;

    requestMsg = MESSAGE_Alloc(node,
                               NETWORK_LAYER,
                               NETWORK_PROTOCOL_MOBILE_IP,
                               MSG_NETWORK_RegistrationRequest);

   // Store important fields of advertisement messages in the
   // MobileNodeInfoList. This will be used when preparing the
   // request message.

    MobileIpMobileNodeStoreFields(
        node, msg, sourceAddress, destinationAddress, interfaceId);

   // send the source and destination address in a structure
   // with the registration request event
    MESSAGE_InfoAlloc(node, requestMsg, sizeof(SrcDestAddrInfo));

    sdAddr = (SrcDestAddrInfo*) MESSAGE_ReturnInfo(requestMsg);
    sdAddr->sourceAddress = destinationAddress;
    sdAddr->destinationAddress = sourceAddress;
    sdAddr->registrationSeqNo = mobileIp->sequenceNumber;

    if (MOBILE_IP_DEBUG_DETAIL)
    {
        printf("Event MSG_NETWORK_RegistrationRequest is sent for Node %u"
               " (source %x, destination %x\n",
               node->nodeId, sdAddr->sourceAddress,
               sdAddr->destinationAddress);
    }

    MESSAGE_Send(node, requestMsg, 0);

    // this function sets the agent advertisement lifetime
    MobileIpCreateAgntAdvTimerMsg(
        node,
        advExt->registrationLifetime,
        sourceAddress,
        destinationAddress,
        interfaceId);

    MESSAGE_Free(node, msg);
}


// FUNCTION:   MobileIpMobileNodeInit()
// PURPOSE:    This function is called from MobileIpInit()
//             function to initialize a mobile node
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
//             nodeInput:     Structure containing contents of input file.
static
void MobileIpMobileNodeInit(
    Node* node,
    const NodeInput* nodeInput,
    NodeAddress homeAgentAdder)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    ERROR_Assert(ipLayer->interfaceInfo[DEFAULT_INTERFACE]->routingProtocol == NULL,
                 "\nMOBILEIP: Mobile node cannot initiate routing protocol\n");

    MobileNodeInfoList* infoList = (MobileNodeInfoList*)
        MEM_malloc(sizeof(MobileNodeInfoList));

    mobileIp->mobileNodeInfoList = infoList;

    infoList->homeAddress = 0;
    infoList->careOfAddress = 0;
    infoList->homeAgent = homeAgentAdder;
    infoList->unicastHome = 0;
    infoList->identNumber = 0;
    infoList->destinationAddress = 0;   // home/foreign agent IP address
    infoList->registrationLifetime = 0;
    infoList->remainingLifetime = 0;
    infoList->retransmissionLifetime = MIN_RETRANSMISSION_LIFETIME;
    infoList->isRegReplyRecv = FALSE;

    // Set router discovery function for the mobile node. This function will
    // be called when a mobile node receives the advertisement message. The
    // function creates the registration packet and send it for registration.
    NetworkIcmpSetRouterDiscoveryFunction(
        node,
        (NetworkIcmpRouterDiscoveryFunctionType)
        &MobileIpCreateAndSendRegReq);
}


// FUNCTION:   MobileIpSendRequestPacketToIp()
// PURPOSE:    This function sends the request message to Ip layer for
//             another node.
// RETURN:     NULL
// ASSUMPTION: None
// PARAMETERS: node:      node which is allocating message
//             reqPacket: registration request message
static
void MobileIpSendRequestPacketToIp(Node* node, Message* requestMsg)
{
    NodeAddress sourceAddress;
    NodeAddress destinationAddress;
    TransportUdpHeader* udpHdr;
    int ttl = IPDEFTTL;

    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    MobileNodeInfoList* mobileNode = mobileIp->mobileNodeInfoList;

    MESSAGE_AddHeader(node,
                      requestMsg,
                      sizeof(TransportUdpHeader),
                      TRACE_MOBILE_IP);

    udpHdr = (TransportUdpHeader*) MESSAGE_ReturnPacket(requestMsg);

    udpHdr->sourcePort = node->appData.nextPortNum;
    node->appData.nextPortNum++;
    udpHdr->destPort = MOBILE_IP_DESTINATION_PORT;
    udpHdr->length = (unsigned short) requestMsg->packetSize;
    udpHdr->checksum = 0;      // checksum not calculated

    // for collocated address only source address will be the care of address
    // for all other cases it is the mobile node home address
    if (mobileIp->foreignAgentType == ColocatedAddress)
    {
        sourceAddress = mobileNode->careOfAddress;
    }
    else
    {
        sourceAddress = mobileNode->homeAddress;
    }

   // If IP address of the agent is not known use "All mobility agents"
   // multicast address in the destination address of the registration
   // Agent type None is considered as the agent is a home agent
   // When registration to home agent set home agent address to the
   // destination address. subnet directed broadcast address is already
   // considered in the home agent address. For all other cases the agent
   // advertisement source address will be the destination address.
    if (!mobileNode->destinationAddress)
    {
        ttl = 1;
        destinationAddress = ALL_MOBILITY_AGENT_MULTICAST_ADDRESS;
    }
    else if (mobileIp->foreignAgentType == None)
    {
        destinationAddress = mobileNode->homeAgent;
    }
    else
    {
        destinationAddress = mobileNode->destinationAddress;
    }

    if (MOBILE_IP_DEBUG)
    {
        printf("Node %u send request packet "
            "\nsource = %x, "
            "\ndestination = %x,"
            "\nCareOfAddress = %x,"
            "\nHomeAddress = %x, "
            "\nHomeAgent = %x\n",
               node->nodeId,
               sourceAddress,
               destinationAddress,
               mobileNode->careOfAddress,
               mobileNode->homeAddress,
               mobileNode->homeAgent);
    }

    // send req message to mobility agent
    NetworkIpSendRawMessageToMacLayer(
        node,
        requestMsg,
        sourceAddress,
        destinationAddress,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_MOBILE_IP,
        ttl,
        NetworkIpGetInterfaceIndexFromAddress(node, sourceAddress),
        destinationAddress);
}


// FUNCTION:   MobileIpGetProperAgentType()
// PURPOSE:
//
// RETURN:     AgentType
// ASSUMPTION: None
static
AgentType MobileIpGetProperAgentType(
    Node* node,
    AgentType mobileAgent,
    NodeAddress homeAddress)
{
    if (mobileAgent == BothAgent)
    {
        int index = 0;

        for (; index < node->numberInterfaces; index++)
        {
            int numHostBits = NetworkIpGetInterfaceNumHostBits(node, index);

            NodeAddress networkAddress = NetworkIpGetInterfaceNetworkAddress(
                                             node, index);

            if (IsIpAddressInSubnet(homeAddress,
                                    networkAddress,
                                    numHostBits))
            {
                return HomeAgent;
            }
        }
        return ForeignAgent;
    }
    return mobileAgent;
}


// FUNCTION:   MobileIpCreateReplyPacket()
// PURPOSE:    This function will be called when a home agent receives
//             MSG_NETWORK_RegistrationReply. This function creates the
//             reply packet.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:      mobile node which is creating request packet
//             msg:       MSG_NETWORK_RegistrationRequest message
//             code:      code for reply packet
static
Message* MobileIpCreateReplyPacket(
    Node* node,
    Message* reqMsg,
    MobileIpRegReplyType code)
{
    char* replyPacket;
    unsigned char * codeptr;
    NodeAddress homeUnicastAdr;
    AgentType agentType;
    unsigned short * lifetime;
    NodeAddress *homeagent,*homeaddress;
    clocktype *identification;
    unsigned int *securityParameterIndex;

    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(reqMsg);
    MobileIpRegistrationRequest* reqPacket = (MobileIpRegistrationRequest*)
        (((char*) ipHeader) + sizeof(IpHeaderType)
        + sizeof(TransportUdpHeader));

    int interfaceId = NetworkGetInterfaceIndexForDestAddress(
                          node, ipHeader->ip_src);

    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    // Create reply message and allocate memory for the reply packet
    Message* replyMsg = MESSAGE_Alloc(node,
                                      NETWORK_LAYER,
                                      NETWORK_PROTOCOL_MOBILE_IP,
                                      MSG_NETWORK_MobileIpData);

    MESSAGE_PacketAlloc(node,
                        replyMsg,
                        SIZE_REGISTRATIONREPLY,
                        TRACE_MOBILE_IP);

    replyPacket = (char* )MESSAGE_ReturnPacket(replyMsg);

    memset(replyPacket, 0, SIZE_REGISTRATIONREPLY);

    *replyPacket = (unsigned char) REGISTRATION_REPLY;
    replyPacket = replyPacket + sizeof(unsigned char);
    codeptr = (unsigned char *)replyPacket;
    *replyPacket = (unsigned char) code;
    replyPacket = replyPacket + sizeof(unsigned char);
    lifetime = (unsigned short *) replyPacket;

    // if requested lifetime exceeds the allowed lifetime of the home agent
    // then set the maximum allowed, else copy lifetime from request message
    if (reqPacket->lifetime > REGISTRATION_LIFETIME)
    {
        *lifetime = REGISTRATION_LIFETIME;
    }
    else
    {
        *lifetime = reqPacket->lifetime;
    }

    replyPacket = replyPacket + sizeof(unsigned short);

    agentType = MobileIpGetProperAgentType(node,
                                           mobileIp->agent,
                                           reqPacket->homeAddress);

    homeagent = (NodeAddress *)replyPacket;

    switch (agentType)
    {
        case HomeAgent:
        {
           // if home agent address is unicast address of home agent, copy it
           // from the request message. Otherwise it is all mobility multicast
           // address. Then set home agent unicast address to the home agent
           // address of reply message and set the code to error (136)

            homeUnicastAdr = NetworkIpGetInterfaceAddress(node, interfaceId);

            if (homeUnicastAdr == reqPacket->homeAgent)
            {
                *homeagent = reqPacket->homeAgent;
                replyPacket = replyPacket + sizeof(NodeAddress);
            }
            else
            {
                *homeagent = homeUnicastAdr;
                replyPacket = replyPacket + sizeof(NodeAddress);
                *codeptr = mobileIPRegDenyHAUnknownHAAddress;
            }
            break;
        }

        case ForeignAgent:
        {

            *homeagent = reqPacket->homeAgent;
            replyPacket = replyPacket + sizeof(NodeAddress);
            break;
        }

        default:
        {
            replyPacket = replyPacket + sizeof(NodeAddress);
            break;
        }
    }

    homeaddress = (NodeAddress *)replyPacket;
    // copy home address from request message to reply message
    *homeaddress = reqPacket->homeAddress;
    replyPacket = replyPacket + sizeof(NodeAddress);
    identification = (clocktype *)replyPacket;
    // identification copied from request packet
    *identification = reqPacket->identification;
    replyPacket = replyPacket + sizeof(clocktype);
    *replyPacket = mobileHome;
    replyPacket = replyPacket + sizeof(unsigned char);
    *replyPacket = 4 + AUTHENTICATOR_SIZE;
    replyPacket = replyPacket + sizeof(unsigned char);
    securityParameterIndex = (unsigned int *)replyPacket;
    *securityParameterIndex = 255 + RANDOM_nrand(mobileIp->seed);

    // not used right now. set to 0
    memset(reqPacket->mobileHomeExtension.authenticator,
           0,
           AUTHENTICATOR_SIZE);

    return replyMsg;
}


// FUNCTION:   MobileIpSendReplyPacketToIp()
// PURPOSE:    This function sends the reply message to Ip layer for
//             another node.
// RETURN:     NULL
// ASSUMPTION: None
// PARAMETERS: node:         node which is allocating message
//             reqPacket:    registration request message
//             replyPacket:  registration reply message
static
void MobileIpSendReplyPacketToIp(
    Node* node,
    Message* reqMsg,
    Message* replyMsg)
{
    int ttl = IPDEFTTL;
    NodeAddress sourceAddress;
    NodeAddress destinationAddress;
    TransportUdpHeader* udpHdr;
    NodeAddress homeUnicastAdr;
    int interfaceId;
    MobileIpRegistrationReply* replyPkt;
    AgentType agentType;

    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    // get IP and UDP header for the request message
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(reqMsg);
    TransportUdpHeader* regUdpHeader = (TransportUdpHeader*)
        (((char*) ipHeader) + sizeof(IpHeaderType));

    // Add UDP header to the reply message
    MESSAGE_AddHeader(node,
                      replyMsg,
                      sizeof(TransportUdpHeader),
                      TRACE_MOBILE_IP);

    udpHdr = (TransportUdpHeader*) MESSAGE_ReturnPacket(replyMsg);
    replyPkt = (MobileIpRegistrationReply*)
        (((char*) udpHdr) + sizeof(TransportUdpHeader));

    // destination port set to the source port of request message
    udpHdr->destPort = regUdpHeader->sourcePort;
    udpHdr->length = (unsigned short) replyMsg->packetSize;
    udpHdr->checksum = 0;      // checksum not calculated

    // destination address set to source address of request message
    destinationAddress = ipHeader->ip_src;

    interfaceId = NetworkGetInterfaceIndexForDestAddress(
                        node, destinationAddress);

     // If destination address of request message is a broadcast or multicast
     // set source address of reply message to the unicast address of home agent
     // Otherwise use the request message destination address as the source
     // address of reply packet.

    homeUnicastAdr = NetworkIpGetInterfaceAddress(node, interfaceId);

    if (ipHeader->ip_dst == ANY_DEST)
    {
        sourceAddress = homeUnicastAdr;
    }
    else
    {
        sourceAddress = ipHeader->ip_dst;
    }

    agentType = MobileIpGetProperAgentType(node,
                                                  mobileIp->agent,
                                                  replyPkt->homeAddress);

    switch (agentType)
    {
        case HomeAgent:
        {
            // Set source port to the destination port of request message
            udpHdr->sourcePort = regUdpHeader->destPort;
            break;
        }

        case ForeignAgent:
        {
            // source port set 434
            udpHdr->sourcePort = MOBILE_IP_DESTINATION_PORT;
            break;
        }

        default:
        {
            break;
        }
    }

    if (MOBILE_IP_DEBUG)
    {

        printf("Node %u sends reply packet "
            "\nsource = %x, destination = %x,"
            "HomeAddress = %x, HomeAgent = %x\n",
            node->nodeId,
            sourceAddress,
            destinationAddress,
            replyPkt->homeAddress,
            replyPkt->homeAgent);
    }

    // send reply message to the mobile node by sending it to the IP layer
    NetworkIpSendRawMessage(
        node,
        replyMsg,
        sourceAddress,
        destinationAddress,
        NetworkIpGetInterfaceIndexFromAddress(node, sourceAddress),
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_MOBILE_IP,
        ttl);
}


// FUNCTION:   MobileIpCreateRequestPacket()
// PURPOSE:    This function will be called when a mobile node receives
//             MSG_NETWORK_RegistrationRequest. This function creates the
//             request packet for registration.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:   mobile node which is creating request packet
//             msg:    MSG_NETWORK_RegistrationRequest message
static
Message* MobileIpCreateRequestPacket(Node* node, Message* msg)
{
    MobileIpRegistrationRequest* reqPacket;

    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    MobileNodeInfoList* infoList = mobileIp->mobileNodeInfoList;

    // Create request message and allocate memory for the request packet
    Message* reqMsg = MESSAGE_Alloc(node,
                                    NETWORK_LAYER,
                                    NETWORK_PROTOCOL_MOBILE_IP,
                                    MSG_NETWORK_MobileIpData);

    MESSAGE_PacketAlloc(node,
                        reqMsg,
                        sizeof(MobileIpRegistrationRequest),
                        TRACE_MOBILE_IP);

    reqPacket = (MobileIpRegistrationRequest*) MESSAGE_ReturnPacket(reqMsg);

    memset(reqPacket, 0, sizeof(MobileIpRegistrationRequest));

    // registration request type is 1
    reqPacket->type = (unsigned char) REGISTRATION_REQUEST;
    // no simultaneous mobility binding
    MobileIpRegistrationRequestSetS(&(reqPacket->mobIpRr), 0);
    // assume no broadcasting
    MobileIpRegistrationRequestSetB(&(reqPacket->mobIpRr), 0);
    // decapsulation not done by mobile node
    MobileIpRegistrationRequestSetD(&(reqPacket->mobIpRr), 0);

    // No special encapsulation, only IP in IP encapsulation used
    MobileIpRegistrationRequestSetM(&(reqPacket->mobIpRr), 0);
    MobileIpRegistrationRequestSetG(&(reqPacket->mobIpRr), 0);
    MobileIpRegistrationRequestSetV(&(reqPacket->mobIpRr), 0);
    MobileIpRegistrationRequestSetResv(&(reqPacket->mobIpRr), 0);

    // registration lifetime retrieved from mobile node structure
    reqPacket->lifetime = infoList->registrationLifetime;

    // home agent address, care of address and home address set
    reqPacket->homeAgent = infoList->homeAgent;
    reqPacket->homeAddress = infoList->homeAddress;
    reqPacket->careOfAddress = infoList->careOfAddress;

    // identification value set
    reqPacket->identification = node->getNodeTime();

    // compulsory mobile home extension
    reqPacket->mobileHomeExtension.extensionType = mobileHome;
    reqPacket->mobileHomeExtension.extensionLength = 4 + AUTHENTICATOR_SIZE;
    reqPacket->mobileHomeExtension.securityParameterIndex = 255 +
            RANDOM_nrand(mobileIp->seed);

    // not used right now, set to 0
    memset(reqPacket->mobileHomeExtension.authenticator,
           0,
           AUTHENTICATOR_SIZE);

    return reqMsg;
}


// FUNCTION:   MobileIpDeregisterFromBindingList()
// PURPOSE:    This function de-registers a mobile node from the mobility
//             binding list
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
//             homeAddr:   home address to be de-registered
//             careOfAddr: corresponding care of address
static
void MobileIpDeregisterFromBindingList(
    Node* node,
    NodeAddress homeAddr,
    NodeAddress careOfAddr)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
            ipLayer->mobileIpStruct;

    LinkedList* infoList = mobileIp->homeAgentInfoList;

    ListItem* item = infoList->first;
    HomeRegistrationList* regList;

    while (item != NULL)
    {
        BOOL isItemToBeDeleted = FALSE;
        regList = (HomeRegistrationList*) item->data;

        if (regList->homeAddress == homeAddr)
        {
            // if careOfAdress is null, de-register all entries of mobile node
            if (careOfAddr == 0)
            {
                isItemToBeDeleted = TRUE;
            }
            else
            {
                if (regList->careOfAddress == careOfAddr)
                {
                    // de-register only current pair.
                   isItemToBeDeleted = TRUE;
                }
            }
        }
        if (isItemToBeDeleted)
        {
            ListGet(node, infoList, item, TRUE, FALSE);
            item = infoList->first;
        }
        else
        {
            item = item->next;
        }
    }
}


// FUNCTION:   MobileIpUpdateBindingList()
// PURPOSE:    This function updates Binding list of the home agent from
//             request message.
// RETURN:     NULL
// ASSUMPTION: None
// PARAMETERS: node:               node for which info. is stored
static
void MobileIpUpdateBindingList(
    Node* node,
    clocktype identification,
    unsigned short lifetime)
{
    // update Binding list of the home agent
    Message* bindingListUpdateMsg;
    bindingListUpdateMsg = MESSAGE_Alloc(
                               node,
                               NETWORK_LAYER,
                               NETWORK_PROTOCOL_MOBILE_IP,
                               MSG_NETWORK_BindingListRefreshTimer);

    MESSAGE_InfoAlloc(node,
                      bindingListUpdateMsg,
                      sizeof(clocktype));

    *((clocktype*) MESSAGE_ReturnInfo(bindingListUpdateMsg)) = identification;

    MESSAGE_Send(node, bindingListUpdateMsg, lifetime * SECOND);
}


// FUNCTION:   MobileIpRegisterInBindingList()
// PURPOSE:    This function inserts a new registration in the mobility
//             binding list
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
//             homeAddr:      home address to be de-registered
//             careOfAddr:    corresponding care of address
//             lifetime:      lifetime of the request remaining
//             simulMobility: simultaneous mobility binding flag
static
void MobileIpRegisterInBindingList(
    Node* node,
    NodeAddress homeAddr,
    NodeAddress careOfAddr,
    unsigned short lifetime,
    BOOL simulMobility)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    LinkedList* infoList = mobileIp->homeAgentInfoList;

    HomeRegistrationList* newRegList;

    if (!simulMobility)
    {
        // if home agent does not support simultaneous mobility binding, remove
        // all other entries in the binding list for that node before inserting
        // new one
        MobileIpDeregisterFromBindingList(node, homeAddr, 0);
    }

    newRegList = (HomeRegistrationList*)
        MEM_malloc(sizeof (HomeRegistrationList));

    newRegList->homeAddress = homeAddr;
    newRegList->careOfAddress = careOfAddr;
    newRegList->identNumber = node->getNodeTime();
    newRegList->remainingLifetime = lifetime;

    ListInsert(node, infoList, node->getNodeTime(), newRegList);

    // set the binding list refresh timer
    MobileIpUpdateBindingList(node,
                              newRegList->identNumber,
                              newRegList->remainingLifetime);
}


// FUNCTION:   MobileIpSendPacketToIp()
// PURPOSE:    This function sends Msg to Ip layer
// RETURN:     NULL
// ASSUMPTION: None
// PARAMETERS: node:         node which is allocating message
static
void MobileIpSendPacketToIp(
    Node * node,
    Message *registrationMsg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int outgoingInterface,
    int ttl)
{
    int priority = IPTOS_PREC_INTERNETCONTROL;
    unsigned char protocol = IPPROTO_MOBILE_IP;

    // this is for adding IpHeader & Routing packet
    NetworkIpSendRawMessage(
        node,
        registrationMsg,
        sourceAddress,
        destinationAddress,
        outgoingInterface,
        priority,
        protocol,
        ttl);
}

// FUNCTION:   MobileIpHomeSendMobileIpProxeyRequest()
// PURPOSE:    Quick solution of proxey ARP
// RETURN:     NULL
// ASSUMPTION: None
// PARAMETERS:
static
void MobileIpHomeSendMobileIpProxeyRequest(
    Node* node,
    NodeAddress mobileAddr,
    NodeAddress homeAgent)
{
    int ttl = 1;
    Message* proxeyMsg;
    MobileIpProxeyRequest* proxeyPkt;
    NodeAddress broadcastAddr;
    NodeAddress sourceAddress;
    int interfaceIndex;

    proxeyMsg = MESSAGE_Alloc(node,
                NETWORK_LAYER,
                NETWORK_PROTOCOL_MOBILE_IP,
                MSG_NETWORK_MobileIpData);


    MESSAGE_PacketAlloc(node,
                        proxeyMsg,
                        sizeof(MobileIpProxeyRequest),
                        TRACE_MOBILE_IP);

    proxeyPkt = (MobileIpProxeyRequest*)
                      MESSAGE_ReturnPacket(proxeyMsg);

    interfaceIndex = NetworkGetInterfaceIndexForDestAddress(
                node,
                mobileAddr);

    broadcastAddr = NetworkIpGetInterfaceBroadcastAddress(node, interfaceIndex);

    sourceAddress = NetworkIpGetInterfaceAddress(node, interfaceIndex);

    proxeyPkt->type =  (unsigned char) PROXY_REQUEST;
    proxeyPkt->homeAgentAddress = sourceAddress;
    proxeyPkt->mobileNodeAddress = mobileAddr;

    //dummy udp header
    MESSAGE_AddHeader(node,
                      proxeyMsg,
                      sizeof(TransportUdpHeader),
                      TRACE_MOBILE_IP);

    if (MOBILE_IP_DEBUG)
    {
        printf("Node %u send Proxey Request packet "
            "\nsource = %x, "
            "\ninterfaceIndex = %d,"
            "\ndestination = %x,"
            "\nmobileAddr = %x, "
            "\nHomeAgent = %x\n",
               node->nodeId,
               sourceAddress,
               interfaceIndex,
               broadcastAddr,
               mobileAddr,
               homeAgent);
    }

    MobileIpSendPacketToIp(
        node,
        proxeyMsg,
        sourceAddress,
        broadcastAddr,
        interfaceIndex,
        ttl);

}


// FUNCTION:   MobileIpRegisterMobileNode()
// PURPOSE:    This function inserts a new registration
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS:
static
void MobileIpRegisterMobileNode(
    Node* node,
    NodeAddress mobileAddr,
    NodeAddress homeAgent)
{
    HomeList* newRegList;
    BOOL isMobileRegistered = FALSE;

    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    LinkedList* infoList = mobileIp->homeAgent;

    ListItem* item = infoList->first;
    // Search the destination address in its binding list
    while (item)
    {
        HomeList* regList = (HomeList*) item->data;

        if (regList->homeAddress == mobileAddr)
        {
            if(regList->isDummyProxySend)
            {
                return;
            }
            isMobileRegistered = TRUE;
        }
        item = item->next;
    }

    if(!isMobileRegistered)
    {
        newRegList = (HomeList*)
            MEM_malloc(sizeof (HomeList));

        newRegList->homeAddress = mobileAddr;
        newRegList->homeAgentAddress = homeAgent;
        newRegList->isDummyProxySend = TRUE;

        ListInsert(node, infoList, node->getNodeTime(), newRegList);
    }

    MobileIpHomeSendMobileIpProxeyRequest(
        node,
        mobileAddr,
        homeAgent);
}


// FUNCTION:   MobileIpHomeStoreFields()
// PURPOSE:    This function stores information from request message
//             into its data structure
// RETURN:     NULL
// ASSUMPTION: None
// PARAMETERS: node:               node for which info. is stored
//             sourceAddress:      source address of the receiving packet
//             destinationAddress: dest. address of the receiving packet
//             interfaceId:        interface id at which message comes
static
void MobileIpHomeStoreFields(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress)
{
    // Update mobility binding list for the mobile node
    MobileIpRegistrationRequest* regReq = (MobileIpRegistrationRequest*)
        (((char*) MESSAGE_ReturnPacket(msg)) + sizeof(TransportUdpHeader)
        + sizeof(IpHeaderType));

    // check for de-registration
    if (regReq->lifetime == DEREGISTRATION_LIFETIME)
    {
        if (MOBILE_IP_DEBUG)
        {
            printf("Remove entry for mobile node %x from the binding"
                    " list of node %u for de-registration\n",
                    regReq->homeAddress,
                    node->nodeId);
        }

        if (regReq->careOfAddress == regReq->homeAddress)
        {
            // remove mobility binding for all
            MobileIpDeregisterFromBindingList(
                node, regReq->homeAddress, 0);
        }
        else
        {
            // remove mobility binding for current care of address
            MobileIpDeregisterFromBindingList(node,
                                              regReq->homeAddress,
                                              regReq->careOfAddress);
        }
    }
    else
    {
        if (MOBILE_IP_DEBUG)
        {
            printf("Check for entry in binding list of node %u for the mobile"
                    " node %x with careof Addr %x\n",
                    node->nodeId,
                    regReq->homeAddress,
                    regReq->careOfAddress);
        }

        if (regReq->careOfAddress != 0)
        {
            MobileIpRegisterInBindingList(node,
                                          regReq->homeAddress,
                                          regReq->careOfAddress,
                                          regReq->lifetime,
                                          MobileIpRegistrationRequestGetS(
                                          regReq->mobIpRr));

            MobileIpUpdateForwardingTable(
                                         node,
                                         regReq->homeAddress,
                                         ANY_DEST,
                                         (unsigned) NETWORK_UNREACHABLE,
                                         DEFAULT_INTERFACE,
                                         ROUTING_ADMIN_DISTANCE_DEFAULT,
                                         ROUTING_PROTOCOL_STATIC);

            if (MOBILE_IP_DEBUG_FORWARDING_TABLE)
            {
                NetworkPrintForwardingTable(node);
            }

            if (MOBILE_IP_DEBUG)
            {
                printf("Make an entry in binding list of node %u for the mobile"
                        " node %x\n",
                        node->nodeId,
                        regReq->homeAddress);
            }

            if (MOBILE_IP_DEBUG_OUTPUT)
            {
                MobileIpPrintBindingList(node);

            }
        }
        else
        {
            MobileIpUpdateForwardingTable(node,
                                         regReq->homeAddress,
                                         ANY_DEST,
                                         regReq->homeAddress,
                                         DEFAULT_INTERFACE,
                                         ROUTING_ADMIN_DISTANCE_DEFAULT,
                                         ROUTING_PROTOCOL_STATIC);

            if (MOBILE_IP_DEBUG_FORWARDING_TABLE)
            {
                NetworkPrintForwardingTable(node);
            }
        }

        MobileIpRegisterMobileNode(
            node,
            regReq->homeAddress,
            regReq->homeAgent);
    }
}


// FUNCTION:   MobileIpCheckInVisitorList()
// PURPOSE:
//
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
static BOOL
MobileIpCheckInVisitorList(Node* node,
                           MobileIpRegistrationReply* replyPktStruct)
{

    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
            ipLayer->mobileIpStruct;

    LinkedList* infoList = mobileIp->foreignAgentInfoList;

    ListItem* item = infoList->first;
    ForeignRegistrationList* regList;

    // Search the visitor list for the current reply message.
    while (item != NULL)
    {
        regList = (ForeignRegistrationList*) item->data;
        // If source address of current reply message is equal to a mobile node
        // home address and the identification number is same. then relays it

        if (regList->sourceAddress == replyPktStruct->homeAddress &&
                regList->identNumber == replyPktStruct->identification)
        {
            // proper entry found in visitor list
            return 0;
        }
        item = item->next;
    }
    // an entry not found in visitor list for the current reply message
    return 1;
}


// FUNCTION:   MobileIpRepairRegistrationMessage()
// PURPOSE:
//
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
static
void MobileIpRepairRegistrationMessage(
    Node* node,
    MobileIpRegistrationReply* replyPktStruct)
{
    // If mobile node receives the reply message with some error, it can
    // repair the error and re-send the request again. This has been handled
    // here. The reply message errors are properly handles for statistics
    // collection

    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    MobileNodeInfoList* infoList = mobileIp->mobileNodeInfoList;


    // all error recovery can be handled within the following switch statement.
    // Among them only mobileIPRegDenyHAUnknownHAAddress is considered here.
    switch (replyPktStruct->code)
    {
        // this error appears when mobile node does not know home agent addr
        case mobileIPRegDenyHAUnknownHAAddress:
        {
            Message* requestMsg;
            SrcDestAddrInfo* sdAddr;

            infoList->homeAgent = replyPktStruct->homeAgent;
            //infoList->unicastHome = replyPkt->homeAgent;
            infoList->remainingLifetime = (unsigned short)(node->getNodeTime() / SECOND)
                                          + infoList->registrationLifetime;

            requestMsg = MESSAGE_Alloc(node,
                                       NETWORK_LAYER,
                                       NETWORK_PROTOCOL_MOBILE_IP,
                                       MSG_NETWORK_RegistrationRequest);

            // send the source and destination address in a structure
            // with the registration request event
            MESSAGE_InfoAlloc(node, requestMsg, sizeof(SrcDestAddrInfo));
            sdAddr = (SrcDestAddrInfo*) MESSAGE_ReturnInfo(requestMsg);
            sdAddr->sourceAddress = infoList->homeAddress;
            sdAddr->destinationAddress = infoList->destinationAddress;
            sdAddr->registrationSeqNo = mobileIp->sequenceNumber;

            if (MOBILE_IP_DEBUG_DETAIL)
            {
                printf("Event MSG_NETWORK_RegistrationRequest is sent for"
                       " Node %u (source %x, destination %x\n",
                       node->nodeId,
                       sdAddr->sourceAddress,
                       sdAddr->destinationAddress);
            }

            MESSAGE_Send(node, requestMsg, 0);
            break;
        }
        default:
        {
            // some eror come.
            char errorBuf[MAX_STRING_LENGTH];
            sprintf(
                errorBuf,
                "Error code %d received from the reply message",
                replyPktStruct->code);

            ERROR_Assert(FALSE, errorBuf);

            break;
        }
    }
}


// FUNCTION:   MobileIpValidityCheck()
// PURPOSE:    This function is used to validate the registration messages in
//             mobile node and mobility agent.
//
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
///
//
// This function is used to validate the registration messages in mobile node
// and mobility agent.
static
BOOL MobileIpValidityCheck(
    Node* node,
    Message* msg,
    MobileIpRegMessageType regMessage,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int interfaceId,
    MobileIpRegistrationReply* replyPktStruct)
{

   // In Qualnet UDP checksum always set to 0, UDp checksum not tested
   // for validity
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    MobileIpRegistrationRequest* requestPkt;
    Message* newMsg;

    NodeAddress homeUnicastAdr;

    switch (regMessage)
    {
        case REGISTRATION_REQUEST:
        {
             // Validation check for registration request in both foreign and
             // home agent
            AgentType agentType;
            requestPkt = (MobileIpRegistrationRequest*)
                    (((char*) MESSAGE_ReturnPacket(msg))
                    + sizeof(TransportUdpHeader));

            agentType = MobileIpGetProperAgentType(
                            node,
                            mobileIp->agent,
                            requestPkt->homeAddress);

            switch (agentType)
            {
                case ForeignAgent:
                {
                    // Validation in Foreign agent for registration request
                    // message

                    // reserved bit test - must be 0
                    if (MobileIpRegistrationRequestGetResv(
                        requestPkt->mobIpRr) != 0)
                    {
                       // Create and send reply packet from foreign
                       // agent for error
                        newMsg = MobileIpCreateReplyPacket(
                                   node, msg, mobileIPRegDenyFAPoorRequest);

                        MobileIpSendReplyPacketToIp(
                            node, msg, newMsg);

                        mobileIp->mobileIpStat->
                            mobileIPRegDenyFAPoorRequest++;
                        return 1;
                    }
                    // maximum lifetime allowed test
                    if (requestPkt->lifetime > REGISTRATION_LIFETIME)
                    {
                        // Set current allowed lifetime and send reply message
                        // to mobile node with error 69
                        newMsg = MobileIpCreateReplyPacket(
                                     node,
                                     msg,
                                     mobileIPRegDenyFAReqLifetimeLong);

                        MobileIpSendReplyPacketToIp(
                            node, msg, newMsg);

                        mobileIp->mobileIpStat->
                                mobileIPRegDenyFAReqLifetimeLong++;
                        return 1;
                    }
                    break;
                }
                case HomeAgent:
                {
                    // Validation in home agent for registration request
                    // message
                    homeUnicastAdr = NetworkIpGetInterfaceAddress(
                                         node, interfaceId);

                    if (MOBILE_IP_DEBUG)
                    {
                        printf("Node %u receives request packet from %x "
                                "having destination address %x and unicast"
                                " home %x\n",
                                node->nodeId,
                                sourceAddress,
                                destinationAddress,
                                homeUnicastAdr);
                    }

                    if (homeUnicastAdr != destinationAddress)
                    {
                        // Create and send reply packet from foreign
                        // agent for error
                        newMsg = MobileIpCreateReplyPacket(
                                     node,
                                     msg,
                                     mobileIPRegDenyHAUnknownHAAddress);

                        MobileIpSendReplyPacketToIp(
                            node, msg, newMsg);

                        mobileIp->mobileIpStat->
                                mobileIPRegDenyHAUnknownHAAddress++;
                        return 1;
                    }
                    // reserved bit test - must be 0
                    if (MobileIpRegistrationRequestGetResv(
                        requestPkt->mobIpRr) != 0)
                    {
                        // Create and send reply packet from foreign
                        // agent for error
                        newMsg = MobileIpCreateReplyPacket(
                                     node,
                                     msg,
                                     mobileIPRegDenyHAPoorReq);

                        MobileIpSendReplyPacketToIp(
                            node, msg, newMsg);

                        mobileIp->mobileIpStat->mobileIPRegDenyHAPoorReq++;
                        return 1;
                    }
                    break;
                }

                default:
                {
                    // invalid receipt of registration request
                    return 1;
                }
            }
            break;
        }
        case REGISTRATION_REPLY:
        {
           // Reply message in foreign agent and mobile node
           // Check the lower 32 bit of the identification number. If it is not
           // equal to the number from request message, reply is discarded.

            AgentType agentType;
            assert(replyPktStruct!=NULL);

            agentType = MobileIpGetProperAgentType(
                         node, mobileIp->agent, replyPktStruct->homeAddress);

            switch (agentType)
            {
                case MobileNode:
                {

                    // check code in reply packet whether accepted or rejected
                    if (replyPktStruct->code == mobileIPRegAccept ||
                        replyPktStruct->code ==
                        mobileIPRegAcceptNoMobileBinding)
                    {
                        // Registration accepted by home agent
                        return 0;
                    }
                    else
                    {
                        // try to repair registration message and send it
                        MobileIpRepairRegistrationMessage(
                            node, replyPktStruct);

                        return 1;
                    }
                    break;
                }

                case ForeignAgent:
                {
                    // Check visitor list for the mobile node for which the reply
                    // message comes
                    if (MobileIpCheckInVisitorList(node, replyPktStruct))
                    {
                       // Discard the message. if there is not entry
                       return 1;
                    }
                    break;
                }
                default:
                {
                    // invalid receipt of registration reply
                    return 1;
                }
            }
            break;
        }

        default:
        {
            break;
        }
    }
    return 0;
}


// FUNCTION:   MobileIpHomeAgentCreateAndSendRegReply()
// PURPOSE:    This function is called when an agent receives registration
//             request. If receiving agent is a foreign agent then it only
//             relays the request to the home agent. If receiving agent is a
//             home agent, it sends Registration reply message. If request is
//             not accepted, reply message is prepared with proper error code.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:              node which is allocating message
//             nodeInput:         Structure containing contents of input file.
//             sourceAddress:     source address of this message
//             destinationAddress:Ip address in which interface this message
//                                has been come.
//             interfaceIndex:    interface at which request message comes
static
void MobileIpHomeAgentCreateAndSendRegReply(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int interfaceIndex)
{
    Message* registrationPkt;
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
            ipLayer->mobileIpStruct;

    // Foreign agent broadcast registration request comes from mobile
    // node. Only home agent which situated in mobile nodes own subnet
    // accepts request packet. all other home agent discards it.
    int index;
    MobileIpRegistrationRequest* regReq =
        (MobileIpRegistrationRequest*)
        (MESSAGE_ReturnPacket(msg) + sizeof(TransportUdpHeader));

    for (index = 0; index < node->numberInterfaces; index++)
    {
        NodeAddress networkAddress = NetworkIpGetInterfaceNetworkAddress(
                                         node, index);

        int numHostBits = NetworkIpGetInterfaceNumHostBits(node, index);

        if (IsIpAddressInSubnet(regReq->homeAddress,
                                networkAddress,
                                numHostBits))
        {
            // Registration request received by home agent.
            // Send reply message

            if (MOBILE_IP_DEBUG)
            {
                printf("Request message received by home agent %u\n",
                        node->nodeId);
            }

            mobileIp->mobileIpStat->mobileIpRegRequestReceivedByHome++;

            // First it checks validity. If following function return 0, then
            // valid request, otherwise error handling

            // add IP header with the request packet and send it with
            // reply message, because all the source and destination
            // address will be required when preparing the reply packet.

            NetworkIpAddHeader(node,
                        msg,
                        sourceAddress,
                        destinationAddress,
                        0,
                        IPPROTO_MOBILE_IP,
                        IPDEFTTL);

            if (!MobileIpValidityCheck(node,
                                       msg,
                                       REGISTRATION_REQUEST,
                                       sourceAddress,
                                       destinationAddress,
                                       interfaceIndex))
            {
                // Store important fields of advertisement messages in the
                // HomeAgentInfoList.

                MobileIpHomeStoreFields(node,
                                        msg,
                                        sourceAddress,
                                        destinationAddress);

                // Create registration reply packet
                registrationPkt = MobileIpCreateReplyPacket(
                                      node,
                                      msg,
                                      mobileIPRegAcceptNoMobileBinding);

                MobileIpSendReplyPacketToIp(node,
                                            msg,
                                            registrationPkt);

                mobileIp->mobileIpStat->mobileIpRegReplied++;
            }

            MESSAGE_Free(node, msg);
            return;
        }
    }
    MESSAGE_Free(node, msg);
}


// FUNCTION:   MobileIpUpdateVisitorList()
// PURPOSE:
//
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node: node which is allocating message
static
void MobileIpUpdateVisitorList(
    Node* node,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
            ipLayer->mobileIpStruct;

    ListItem* item;
    LinkedList* infoList = mobileIp->foreignAgentInfoList;
    ForeignRegistrationList* regList;

    // VisitorList List maintains only for ForeignAgent or BothAgent.
    if (!(mobileIp->agent == BothAgent || mobileIp->agent == ForeignAgent))
    {
        // assert
    }

    item = (ListItem*) infoList->first;

    while (item != NULL)
    {
        ListItem* temp;
        regList = (ForeignRegistrationList*) item->data;

        if (regList->sourceAddress == sourceAddress &&
            regList->destinationAddress == destinationAddress)
        {
            ListGet(node, infoList, item, FALSE, FALSE);
        }
        if (item->data!=NULL)
        {
            MEM_free (item->data);
        }
        temp = item;
        item = item->next;
        MEM_free(temp);
    }
}


// FUNCTION:   MobileIpForeignStoreFields()
// PURPOSE:    This function stores information from request message
//             into its data structure
// RETURN:     NULL
// ASSUMPTION: None
// PARAMETERS: node:               node for which info. is stored
//             sourceAddress:      source address of the receiving packet
//             destinationAddress: dest. address of the receiving packet
//             interfaceId:        interface id at which message comes
static
void MobileIpForeignStoreFields(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress)
{
   // update visitor list of the foreign agent with the new registration
   // requested by the mobile node. If there is an entry in the list for the
   // same mobile node, it also records another entry for that request.

    TransportUdpHeader* udpHeader =
        (TransportUdpHeader*) MESSAGE_ReturnPacket(msg);

    MobileIpRegistrationRequest* requestPkt = (MobileIpRegistrationRequest*)
        (((char*) udpHeader) + sizeof(TransportUdpHeader));

    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    LinkedList* infoList = mobileIp->foreignAgentInfoList;
    ForeignRegistrationList* newRegList;

    newRegList = (ForeignRegistrationList*) MEM_malloc(
        sizeof (ForeignRegistrationList));

    newRegList->sourceAddress = sourceAddress;
    newRegList->destinationAddress = destinationAddress;
    newRegList->homeAgent = requestPkt->homeAgent;
    newRegList->udpSourcePort = udpHeader->sourcePort;
    newRegList->identNumber = requestPkt->identification;
    newRegList->registrationLifetime = requestPkt->lifetime;
    newRegList->remainingLifetime = requestPkt->lifetime;


    MobileIpUpdateVisitorList(
        node,
        sourceAddress,
        destinationAddress);

    MobileIpUpdateForwardingTable(node,
                                 requestPkt->homeAddress,
                                 ANY_DEST,
                                 requestPkt->homeAddress,
                                 DEFAULT_INTERFACE,
                                 ROUTING_ADMIN_DISTANCE_DEFAULT,
                                 ROUTING_PROTOCOL_STATIC);

    if (MOBILE_IP_DEBUG_FORWARDING_TABLE)
    {
        NetworkPrintForwardingTable(node);
    }

    ListInsert(node, infoList, node->getNodeTime(), newRegList);

    if (MOBILE_IP_DEBUG)
    {
        char clockStr[100];
        ctoa(newRegList->identNumber, clockStr);

        printf("Node %u creates visitor list inserting member with "
               "identNumber %s\n",
               node->nodeId,
               clockStr);
    }

    if (MOBILE_IP_DEBUG_OUTPUT)
    {
        MobileIpPrintVisitorList(node);
    }
}



// FUNCTION:   MobileIpRelayRequestPacketToIp()
// PURPOSE:    This function relays the request message to Ip layer for
//             for home agent
// RETURN:     NULL
// ASSUMPTION: None
// PARAMETERS: node:         node which is allocating message
//             reqMsg:       registration request message to be relayed
static
void MobileIpRelayRequestPacketToIp(Node* node, Message* reqMsg)
{
    int ttl = IPDEFTTL;
    NodeAddress sourceAddress;
    NodeAddress destinationAddress;

    int interfaceId;
    int index;
    int priority = IPTOS_PREC_INTERNETCONTROL;
    unsigned char protocol = IPPROTO_MOBILE_IP;
    clocktype delay;

    TransportUdpHeader* udpHeader =
        (TransportUdpHeader*) MESSAGE_ReturnPacket(reqMsg);
    MobileIpRegistrationRequest* requestPkt = (MobileIpRegistrationRequest*)
            (((char*)udpHeader) + sizeof(TransportUdpHeader));

    // source port set. destination port is same (434)
    udpHeader->sourcePort = node->appData.nextPortNum;
    node->appData.nextPortNum++;

    destinationAddress = requestPkt->homeAgent;

    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
             ipLayer->mobileIpStruct;


    if (MOBILE_IP_DEBUG)
    {
        printf("Node %u is relaying request packet\n", node->nodeId);
    }

    if (requestPkt->homeAgent == ANY_DEST)
    {
        for (index = 0; index < node->numberInterfaces; index++)
        {
            sourceAddress = NetworkIpGetInterfaceAddress(node, index);

            delay = RANDOM_nrand(mobileIp->seed) % MOBILE_IP_MAX_JITTER;

            NetworkIpSendRawMessageToMacLayerWithDelay(
                node,
                MESSAGE_Duplicate(node, reqMsg),
                sourceAddress,
                destinationAddress,
                priority,
                protocol,
                ttl,
                index,
                destinationAddress,
                delay);
         }

         MESSAGE_Free(node, reqMsg);
    }
    else
    {
        // get interface address of foreign agent through which message
        // is relayed and set it to source address of relayed packet.
        interfaceId = NetworkGetInterfaceIndexForDestAddress(
                          node, requestPkt->homeAgent);

        sourceAddress = NetworkIpGetInterfaceAddress(
                            node, interfaceId);

        requestPkt->careOfAddress = sourceAddress;

        NetworkIpSendRawMessage(
            node,
            reqMsg,
            sourceAddress,
            destinationAddress,
            interfaceId,
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_MOBILE_IP,
            ttl);
    }
}


// FUNCTION:   MobileIpForeignAgentRelayedRegReq()
// PURPOSE:    This function is called when an agent receives registration
//             request. If receiving agent is a foreign agent then it only
//             relays the request to the home agent. If receiving agent is a
//             home agent, it sends Registration reply message. If request is
//             not accepted, reply message is prepared with proper error code.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:              node which is allocating message
//             nodeInput:         Structure containing contents of input file.
//             sourceAddress:     source address of this message
//             destinationAddress:Ip address in which interface this message
//                                has been come.
//             interfaceIndex:    interface at which request message comes
static
void MobileIpForeignAgentRelayedRegReq(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int interfaceIndex)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
            ipLayer->mobileIpStruct;

   // Registration request received by foreign agent and relayed that
   // message to home agent
    if (MOBILE_IP_DEBUG)
    {
        printf("Request message received by foreign agent %u\n",
                node->nodeId);
    }

   // Store important fields of request message received from the
   // mobile node in the ForeignAgentInfoList. This will be used
   // when it gets the reply back from the home agent and checks the
   // validity of the reply message
    MobileIpForeignStoreFields(node,
                               msg,
                               sourceAddress,
                               destinationAddress);

    mobileIp->mobileIpStat->mobileIpRegRequestReceivedByForeign++;

   // First it checks the validity. If following function return 0,
   // then valid request, otherwise error handling

    if (!MobileIpValidityCheck(node,
                               msg,
                               REGISTRATION_REQUEST,
                               sourceAddress,
                               destinationAddress,
                               interfaceIndex))
    {
        mobileIp->mobileIpStat->mobileIpRegRequestRelayedByForeign++;

        // Relay request message to the home agent
        MobileIpRelayRequestPacketToIp(node, msg);
    }
}


// FUNCTION:   MobileIpCreateAndSendRegReply()
// PURPOSE:    This function is called when an agent receives registration
//             request. If receiving agent is a foreign agent then it only
//             relays the request to the home agent. If receiving agent is a
//             home agent, it sends Registration reply message. If request is
//             not accepted, reply message is prepared with proper error code.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:              node which is allocating message
//             nodeInput:         Structure containing contents of input file.
//             sourceAddress:     source address of this message
//             destinationAddress:Ip address in which interface this message
//                                has been come.
//             interfaceIndex:    interface at which request message comes
static
void MobileIpCreateAndSendRegReply(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int interfaceIndex)
{
    AgentType agentType;
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    MobileIpRegistrationRequest* regReq = (MobileIpRegistrationRequest*)
        (MESSAGE_ReturnPacket(msg) + sizeof(TransportUdpHeader));

    if (mobileIp == NULL)
    {
        MESSAGE_Free(node, msg);
        return;
    }

    agentType = MobileIpGetProperAgentType(node,
                                           mobileIp->agent,
                                           regReq->homeAddress);

    switch (agentType)
    {
        case ForeignAgent:
        {
            // Registration request received by foreign agent and relayed that
            // message to again foreign agent should discard.
            if (regReq->homeAddress != sourceAddress)
            {
                MESSAGE_Free(node, msg);
                return;
            }

            // Registration request received by foreign agent and relayed that
            // message to home agent
            MobileIpForeignAgentRelayedRegReq(node,
                                              msg,
                                              sourceAddress,
                                              destinationAddress,
                                              interfaceIndex);

            break;
        }

        case HomeAgent:
        {
            MobileIpHomeAgentCreateAndSendRegReply(node,
                                                   msg,
                                                   sourceAddress,
                                                   destinationAddress,
                                                   interfaceIndex);

           break;
        }
        default:
        {
            MESSAGE_Free(node, msg);
            // Invalid request
            break;
        }
    }
}


// FUNCTION:   MobileIpForeignRefreshVisitorList()
// PURPOSE:    This function updates visitor list of the foreign agent from
//             from reply message.
// RETURN:     NULL
// ASSUMPTION: None
// PARAMETERS: node:               node for which info. is stored
static
void MobileIpForeignRefreshVisitorList(Node* node, MobileIpRegistrationReply*
                                       replyPktStruct)
{
    // update visitor list of the foreign agent
    Message* visitorListUpdateMsg;
    clocktype delay;

    visitorListUpdateMsg = MESSAGE_Alloc(
                               node,
                               NETWORK_LAYER,
                               NETWORK_PROTOCOL_MOBILE_IP,
                               MSG_NETWORK_VisitorListRefreshTimer);

    MESSAGE_InfoAlloc(node,
                      visitorListUpdateMsg,
                      sizeof(clocktype));

   *((clocktype*) MESSAGE_ReturnInfo(visitorListUpdateMsg)) =
       replyPktStruct->identification;

    if (replyPktStruct->lifetime == DEREGISTRATION_LIFETIME)
    {
        delay = DEREGISTRATION_LIFETIME;
    }
    else
    {
        // accepted lifetime without the delay to receive reply message
        // from the request message
        delay = replyPktStruct->lifetime * SECOND - (node->getNodeTime()
            - replyPktStruct->identification);
    }
    MESSAGE_Send(node, visitorListUpdateMsg, delay);
}


// FUNCTION:   MobileIpRelayReplyPacketToIp()
// PURPOSE:    This function relays the reply message to Ip layer to
//             send it for mobile node
// RETURN:     NULL
// ASSUMPTION: None
// PARAMETERS: node:         node which is allocating message
//             reqMsg:       registration request message to be relayed
static
void MobileIpRelayReplyPacketToIp(Node* node, Message* replyMsg,
                                  MobileIpRegistrationReply* replyPktStruct)
{
    int ttl = IPDEFTTL;
    NodeAddress sourceAddress = 0;
    NodeAddress destinationAddress = 0;
    int interfaceId;

    assert(replyPktStruct!=NULL);

    TransportUdpHeader* udpHeader =
        (TransportUdpHeader*) MESSAGE_ReturnPacket(replyMsg);

    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    LinkedList* infoList = mobileIp->foreignAgentInfoList;

    ListItem* item = infoList->first;
    ForeignRegistrationList* regList = NULL;
    BOOL isVisitorEntryFound = FALSE;

    // Search the visitor list for the current reply message
    while (item != NULL && !isVisitorEntryFound)
    {
        regList = (ForeignRegistrationList*) item->data;
       // If source address of current reply message is equal to a mobile node
       // home address and the identification number is same. then relays it

        if (regList->sourceAddress == replyPktStruct->homeAddress &&
            regList->identNumber == replyPktStruct->identification)
        {
            // proper entry found in visitor list
            isVisitorEntryFound = TRUE;
        }
        item = item->next;
    }

    // prepare reply message header for relaying
    if (isVisitorEntryFound)
    {
        // destination port set to the source port of original request message
        // source port is same as 434
        udpHeader->destPort = regList->udpSourcePort;

        // destination address is the source address of request message
        destinationAddress = regList->sourceAddress;

        // if destination address of request packet is "All mobility agent"
        // then find the unicast address in the proper interface of the foreign
        // agent ans set it as the source address of reply message. otherwise
        // source address will be the destination address of the request packet.

        if (regList->destinationAddress ==
                ALL_MOBILITY_AGENT_MULTICAST_ADDRESS)
        {
            interfaceId = NetworkGetInterfaceIndexForDestAddress(
                          node, destinationAddress);

            sourceAddress = NetworkIpGetInterfaceAddress(
                            node, interfaceId);
        }
        else
        {
            sourceAddress = regList->destinationAddress;
        }

        if (replyPktStruct->lifetime == 0)
        {
           // de-registration reply message from home agent. Before relaying
           // mobile node, delete all the visitor list entry for that
           // mobile node

            item = infoList->first;

            // Search the visitor list for the current reply message.
            while (item != NULL)
            {
                regList = (ForeignRegistrationList*) item->data;

                // delete all entries from visitor list for current m/node

                if (regList->sourceAddress == replyPktStruct->homeAddress)
                {
                    ListGet(node, infoList, item, TRUE, FALSE);
                }
                item = item->next;
            }
        }
        else
        {
            // granted lifetime by home is set and remaining lifetime is also
            // adjusted
            regList->remainingLifetime = regList->remainingLifetime -
                  (regList->registrationLifetime - replyPktStruct->lifetime);
            regList->registrationLifetime = replyPktStruct->lifetime;
        }
    }
    else
    {
        // visitor entry not found simply discard the reply message
    }

    // relays reply message to the mobile node by sending it to the IP layer
    NetworkIpSendRawMessageToMacLayer(
        node,
        replyMsg,
        sourceAddress,
        destinationAddress,
        IPTOS_PREC_INTERNETCONTROL,
        IPPROTO_MOBILE_IP,
        ttl,
        NetworkIpGetInterfaceIndexFromAddress(node, sourceAddress),
        destinationAddress);
}


// FUNCTION:   MobileIpReceiveRegReply()
// PURPOSE:    This function will be called when a mobile node or a foreign
//             agent receives a registration reply message. If receiving agent
//             type is foreign agent then it relays the message to the mobile
//             node. If the receiving agent is mobile node the registration is
//             completed.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:              node which is creating reply message
//             msg:               reply message for the node
//             sourceAddress:     source address of this message
//             destinationAddress:Ip address in which interface this message
//                                has been come.
//             interfaceId:       interface at which request message comes
static
void MobileIpReceiveRegReply(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int interfaceId)
{
    AgentType agentType;
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    MobileNodeInfoList* mobileNode = mobileIp->mobileNodeInfoList;


    char* replyPkt = (char*) MESSAGE_ReturnPacket(msg) +
                        sizeof(TransportUdpHeader);

    MobileIpRegistrationReply replyPktStruct;

    replyPktStruct.type = *((unsigned char *) replyPkt) ;
    replyPkt = replyPkt + sizeof(unsigned char);
    replyPktStruct.code = *((unsigned char *) replyPkt) ;
    replyPkt = replyPkt + sizeof(unsigned char);
    replyPktStruct.lifetime = *((unsigned short *) replyPkt) ;
    replyPkt = replyPkt + sizeof(unsigned short);
    replyPktStruct.homeAgent = *((NodeAddress *) replyPkt) ;
    replyPkt = replyPkt + sizeof(NodeAddress);
    replyPktStruct.homeAddress = *((NodeAddress *) replyPkt) ;
    replyPkt = replyPkt + sizeof(NodeAddress);
    replyPktStruct.identification = *((clocktype *) replyPkt) ;
    replyPkt = replyPkt + sizeof(clocktype);
    replyPktStruct.mobileHomeExtension.extensionType = *((unsigned char *)
                                                        replyPkt) ;
    replyPkt = replyPkt + sizeof(unsigned char);
    replyPktStruct.mobileHomeExtension.extensionLength = *((unsigned char *)
                                                          replyPkt) ;
    replyPkt = replyPkt + sizeof(unsigned char);
    replyPktStruct.mobileHomeExtension.securityParameterIndex =
        *((unsigned int *) replyPkt) ;

    agentType = MobileIpGetProperAgentType(node, mobileIp->agent,
                                                 replyPktStruct.homeAddress);

    switch (agentType)
    {
        case ForeignAgent:
        {
            if (MOBILE_IP_DEBUG)
            {
                printf("Reply message received by foreign agent %u\n",
                        node->nodeId);
            }

             // Reply message received by a foreign agent. Registration reply
             // relayed to mobile node

             mobileIp->mobileIpStat->mobileIpRegReplyReceivedByForeign++;

             // First it checks the validity. If following function return 0,
             // then valid reply message, otherwise error handling

            if (!MobileIpValidityCheck(node,
                                              msg,
                                              REGISTRATION_REPLY,
                                              sourceAddress,
                                              destinationAddress,
                                              interfaceId,&replyPktStruct))
            {
                MobileIpForeignRefreshVisitorList(node, &replyPktStruct);

                mobileIp->mobileIpStat->mobileIpRegReplyRelayedByForeign++;

                // Relay request message to the home agent
                MobileIpRelayReplyPacketToIp(node, msg, &replyPktStruct);
            }
            break;
        }

        case MobileNode:
        {
            if (MOBILE_IP_DEBUG)
            {
                printf("Reply message received by mobile node %u\n",
                        node->nodeId);
            }

            mobileNode->retransmissionLifetime = MIN_RETRANSMISSION_LIFETIME;
            mobileNode->isRegReplyRecv = TRUE;

           // Reply message received by mobile node
           // First it checks the validity. If following function return 0,
           // then valid reply message, otherwise error handling

            if (!MobileIpValidityCheck(node,
                                       msg,
                                       REGISTRATION_REPLY,
                                       sourceAddress,
                                       destinationAddress,
                                       interfaceId,&replyPktStruct))
            {
                mobileIp->mobileIpStat->mobileIpRegAccepted++;
            }
            MESSAGE_Free(node, msg);
            break;
        }

        default:
        {
            MESSAGE_Free(node, msg);
            // Invalid request, log error message
            ERROR_Assert(FALSE, "Invalid node");
            break;
        }
    }
}


// FUNCTION:   MobileIpCreateAgentAdvExt()
// PURPOSE:    This function prepares the agent advertisement extension message
//             for a mobility agent
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:       node which is allocating message
//             routerAddr: Advertised router's interface address
//             advExt:     Pointer to the Advertisement extension part of
//                         advertisement message
void MobileIpCreateAgentAdvExt(
    Node* node,
    NodeAddress routerAddr,
    Message* msg)
{
   // Assign proper value in the extension message, which is already allocated
   // Store the sequenceNumber in AgentInfoList structure for future use. Next
   // time this number is incremented to 1, until 0xffff

    AgentAdvertisementExtension* advExt = (AgentAdvertisementExtension*)
            (((char*) MESSAGE_ReturnPacket(msg)) + sizeof(IcmpData));

    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
            ipLayer->mobileIpStruct;

    if (MOBILE_IP_DEBUG_DETAIL)
    {
        printf("Create agent advertisement(from %x) extension for node = %u\n",
            routerAddr, node->nodeId);
    }

    // Agent advertisement extension
    advExt->extensionType = 16;
    advExt->registrationLifetime = REGISTRATION_LIFETIME;
    AgentAdvertisementExtensionSetReserved(&(advExt->agentAdvExt), 0);

    if (mobileIp->agent == BothAgent)
    {
        AgentAdvertisementExtensionSetHAgent(&(advExt->agentAdvExt), 1);
        AgentAdvertisementExtensionSetFAgent(&(advExt->agentAdvExt), 1);
        advExt->extensionLength = 6 + 4 * MAX_ADVERTISED_NO;
        advExt->sequenceNumber = mobileIp->sequenceNumber;
        AgentAdvertisementExtensionSetRegReq(&(advExt->agentAdvExt), 1);
        advExt->careOfAddress = routerAddr;
    }
    else if (mobileIp->agent == ForeignAgent)
    {
        AgentAdvertisementExtensionSetHAgent(&(advExt->agentAdvExt), 0);
        AgentAdvertisementExtensionSetFAgent(&(advExt->agentAdvExt), 1);
        advExt->extensionLength = 6 + 4 * MAX_ADVERTISED_NO;
        advExt->sequenceNumber = mobileIp->sequenceNumber;
        AgentAdvertisementExtensionSetRegReq(&(advExt->agentAdvExt), 1);
        advExt->careOfAddress = routerAddr;
    }
    else
    {
        AgentAdvertisementExtensionSetHAgent(&(advExt->agentAdvExt), 1);
        AgentAdvertisementExtensionSetFAgent(&(advExt->agentAdvExt), 0);
        AgentAdvertisementExtensionSetRegReq(&(advExt->agentAdvExt), 0);

        // for home agent no care-of-address
        advExt->extensionLength = 6;
        advExt->sequenceNumber = mobileIp->sequenceNumber;
    }
    // Next sequence number incremented. If it is greater than 0xffff,
    // then reset to 256
    mobileIp->sequenceNumber++;
    if (mobileIp->sequenceNumber == 0)
    {
        mobileIp->sequenceNumber = 256;
    }
}


// FUNCTION:   MobileIpEncapsulateDatagram()
// PURPOSE:    This function is written in mobileip.c to encapsulate
//             the incoming datagram.
// RETURN:     BOOL
// ASSUMPTION: None
// PARAMETERS: node:               node for which info. is stored
void MobileIpEncapsulateDatagram(Node* node, Message* msg)
{
    int interfaceId;
    NodeAddress sourceAddress;
    ListItem* item;
    HomeRegistrationList* regList;

    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
            ipLayer->mobileIpStruct;

    LinkedList* infoList = mobileIp->homeAgentInfoList;

    AgentType agentType = MobileIpGetProperAgentType(
                              node, mobileIp->agent, ipHeader->ip_dst);

    // Encapsulate will be considered for home agent only.
    // If already encapsulated then do nothing
    // If the packet is a control packet of mobileip then do nothing.
    if (agentType != HomeAgent)
    {
        return;
    }

    item = infoList->first;

    // Search the destination address in its binding list
    while (item)
    {
        regList = (HomeRegistrationList*) item->data;

        if (regList->homeAddress == ipHeader->ip_dst)
        {
            // Encapsulates the original datagram with another IP header with
            // care of address as the destination address
            interfaceId = NetworkGetInterfaceIndexForDestAddress(
                          node, regList->careOfAddress);

            sourceAddress = NetworkIpGetInterfaceAddress(
                            node, interfaceId);

            // When encapsulating a datagram, the TTL in the inner IP header is
            // decremented by one if the tunneling is being done as part of
            // forwarding the datagram
            ipHeader->ip_ttl = ipHeader->ip_ttl - 1;

            // An encapsulator MUST NOT encapsulate a datagram with TTL = 0.
            if (ipHeader->ip_ttl <= 0)
            {
                return;
            }

            mobileIp->mobileIpStat->mobileIPDataPktEncapHome++;

            if (MOBILE_IP_DEBUG)
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);
                printf("Node %u Encapsulate Datagram at time %s "
                        "(source = %x & destination = %x)\n",
                        node->nodeId,
                        clockStr,
                        sourceAddress,
                        regList->careOfAddress);
            }

            NetworkIpAddHeader(node,
                        msg,
                        sourceAddress,
                        regList->careOfAddress,
                        0,
                        IPPROTO_IPIP,
                        IPDEFTTL);

            break;
        }
        item = item->next;
    }
}


// FUNCTION:   MobileIpDecapsulateDatagram()
// PURPOSE:    This function is written to decapsulate
//             the incoming datagram.
// RETURN:     NULL
// ASSUMPTION: None
// PARAMETERS: node:               node for which info. is stored
void MobileIpDecapsulateDatagram(Node* node, Message* msg)
{
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
        ipLayer->mobileIpStruct;

    LinkedList* infoList = mobileIp->foreignAgentInfoList;
    ListItem* item = infoList->first;
    ForeignRegistrationList* regList;

    // Search the destination address in its visitor list
    while (item != NULL)
    {
        regList = (ForeignRegistrationList*) item->data;

        if (regList->sourceAddress == ipHeader->ip_dst)
        {
            if (MOBILE_IP_DEBUG)
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);

                printf("Node %u Decapsulate Datagram "
                       "at time %s\n",
                       node->nodeId,
                       clockStr);
            }

            mobileIp->mobileIpStat->mobileIPDataPktDcapForeign++;

            // proper entry found in visitor list, so forward packet to that
            NetworkIpSendPacketToMacLayer(
                node,
                msg,
                NetworkIpGetInterfaceIndexFromAddress(
                    node,
                    regList->destinationAddress),
                regList->sourceAddress);

            return;
        }
        item = item->next;
    }
    MESSAGE_Free(node, msg);
}


// FUNCTION:   MobileIpFindNextInterval()
// PURPOSE:    Calculate the next solicitation interval. For the first 3
//             cases normal mobile IP solicitation interval has been returned.
//             After the first 3 transmission, each time interval is made
//             doubled as the previous one. The maximum limit is set to
//             the allowed maximum limit.
// RETURN:     Next solicitation interval
// ASSUMTIONS: None
// PARAMETERS: numberOfSolicitation: Number of solicitation already made
//             maxSolicitation     : Maximum ICMP solicitation allowed
clocktype MobileIpFindNextInterval(
    unsigned int numberOfSolicit,
    unsigned int maxSolicitation)
{
    clocktype nextInterval;
    if (numberOfSolicit <= maxSolicitation)
    {
        nextInterval = MOBILE_IP_SOLICITATION_INTERVAL;
    }
    else
    {
        nextInterval = MOBILE_IP_SOLICITATION_INTERVAL
                       * (numberOfSolicit - maxSolicitation + 1);
    }

    if (nextInterval > MOBILE_IP_MAX_SOLITATION_INTERVAL)
    {
        nextInterval = MOBILE_IP_MAX_SOLITATION_INTERVAL;
    }
    return nextInterval;
}


// FUNCTION:   MobileIpInit()
// PURPOSE:    This function is called from NetworkIpInit() function to
//             initialize all mobile IP related data fields.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
//             nodeInput:     Structure containing contents of input file.
void MobileIpInit(
    Node* node,
    const NodeInput* nodeInput)
{
    BOOL retVal = FALSE;
    char protocolString[MAX_STRING_LENGTH];
    NodeAddress homeAgentAdder = 0;
    BOOL flag = FALSE;
    int i;

    for (i = 0; i < node->numberInterfaces; i++)
    {

        IO_ReadString(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, i),
            nodeInput,
            "MOBILE-IP",
            &retVal,
            protocolString);

        if (retVal)
        {
            if (strcmp(protocolString, "YES") == 0)
            {
                flag = TRUE;
                break;
            }
        }
    }

    if (flag)
    {
        // Determine whether node is mobile node or home agent or foreign agent
        AgentType agentType = (AgentType) MobileIpReadAgentType(
                                  node, nodeInput, &homeAgentAdder);

        switch (agentType)
        {
            case MobileNode:
                MobileIpMobileNodeInit(node, nodeInput, homeAgentAdder);
                break;

            case BothAgent:
                MobileIpHomeAgentInit(node, nodeInput);
                MobileIpForeignAgentInit(node, nodeInput);
                break;

            case HomeAgent:
                MobileIpHomeAgentInit(node, nodeInput);
                break;

            case ForeignAgent:
                MobileIpForeignAgentInit(node, nodeInput);
                break;

            case Others:
                break;

            default:
                ERROR_Assert(FALSE, "Invalid node, program terminated");
                break;
        }
    }
}


// FUNCTION:   MobileIpLayer()
// PURPOSE:    This function is called  from NetworkIpLayer function when
//             protocol type is NETWORK_PROTOCOL_MOBILE_IP.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:  node which is allocating message
//             msg:   message to the node
void
MobileIpLayer(Node* node, Message* msg)
{
    Message* registrationPkt;
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
            ipLayer->mobileIpStruct;

    MobileNodeInfoList* mobileNode = (MobileNodeInfoList*)
        mobileIp->mobileNodeInfoList;

    Message* reqTimeoutMsg;
    Message* requestMsg;

    SrcDestAddrInfo* sdAddr;
    SrcDestAddrInfo* sdTimeoutAddr;
    SrcDestAddrInfo* sdRexmtAddr;

    switch (MESSAGE_GetEvent(msg))
    {
        case MSG_NETWORK_RegistrationRequest:
        {
            // network layer receives this message for creating
            // REGISTRATION_REQUEST

            sdAddr = (SrcDestAddrInfo*) MESSAGE_ReturnInfo(msg);

            if (sdAddr->registrationSeqNo < mobileIp->sequenceNumber)
            {
                // remove that entry from visitor list //?
                break;
            }
           // if (mobileNode->destinationAddress == sdAddr->destinationAddress)
            {
                // checks whether it receive of Agent Adv from same Agent
                // or different agent
                if (MOBILE_IP_DEBUG_DETAIL)
                {
                    char clockStr[100];
                    ctoa(node->getNodeTime(), clockStr);

                    printf("Node %u got MSG_NETWORK_RegistrationRequest"
                           " event at time %s\n", node->nodeId, clockStr);
                }

                // Create registration request packet. Add Mobile-Home
                // authentication extension and add other authentication
                // extension if necessary. Add UDP header and send it to
                // IP layer.
                registrationPkt = MobileIpCreateRequestPacket(
                                      node, msg);

                MobileIpSendRequestPacketToIp(node,
                                                     registrationPkt);

                mobileIp->mobileIpStat->mobileIpRegRequested++;

                // When mobile node is deregistered while coming back to home
                // network, the following self messages are not required
                if (mobileNode->registrationLifetime ==
                        DEREGISTRATION_LIFETIME)
                {
                    break;
                }

                // Send self message MSG_NETWORK_RegistrationRequest with a
                // delay of Registration lifetime, so that after the lifetime
                // expires it again ask for the re-registration if there is
                // no other advertisement message  But getting granted
                // lifetime from Home agent, it should update the remaining
                // lifetime later.

                reqTimeoutMsg = MESSAGE_Alloc(
                                    node,
                                    NETWORK_LAYER,
                                    NETWORK_PROTOCOL_MOBILE_IP,
                                    MSG_NETWORK_RegistrationRequest);

                // send the source and destination address in a structure
                // with the registration request event
                MESSAGE_InfoAlloc(
                    node,
                    reqTimeoutMsg,
                    sizeof(SrcDestAddrInfo));

                sdTimeoutAddr = (SrcDestAddrInfo*)
                                MESSAGE_ReturnInfo(reqTimeoutMsg);

                sdTimeoutAddr->sourceAddress = sdAddr->sourceAddress;
                sdTimeoutAddr->destinationAddress =
                    sdAddr->destinationAddress;

                sdTimeoutAddr->registrationSeqNo = mobileIp->sequenceNumber;
                MESSAGE_Send(node,
                             reqTimeoutMsg,
                             mobileNode->registrationLifetime * SECOND);

                // Send self message MSG_NETWORK_RetransmissionRequired to
                // the network layer with a delay less than the value of
                // request lifetime. Within that period if mobile node does
                // not accept reply, then it retransmits the request

                if (mobileNode->retransmissionLifetime <=
                    mobileNode->registrationLifetime)
                {
                    requestMsg = MESSAGE_Alloc(
                                     node,
                                     NETWORK_LAYER,
                                     NETWORK_PROTOCOL_MOBILE_IP,
                                     MSG_NETWORK_RetransmissionRequired);

                    // send the source and destination address in a structure
                    // with the registration request event
                    MESSAGE_InfoAlloc(node,
                                      requestMsg,
                                      sizeof(SrcDestAddrInfo));

                    sdRexmtAddr = (SrcDestAddrInfo*)
                                  MESSAGE_ReturnInfo(requestMsg);

                    sdRexmtAddr->sourceAddress = sdAddr->sourceAddress;
                    sdRexmtAddr->destinationAddress =
                                            sdAddr->destinationAddress;

                    mobileNode->isRegReplyRecv = FALSE;

                    MESSAGE_Send(
                        node,
                        requestMsg,
                        mobileNode->retransmissionLifetime * SECOND);

                    mobileNode->retransmissionLifetime *= 2;
                }
            }
            break;
        }

        case MSG_NETWORK_RetransmissionRequired:
        {
            // network layer receives this message for re-transmitting the
            // REGISTRATION_REQUEST

            // if mobile node already receives reply message, then ginored
            if (mobileNode->isRegReplyRecv)
            {
                break;
            }

            if (MOBILE_IP_DEBUG_DETAIL)
            {
                char clockStr[100];
                ctoa(node->getNodeTime(), clockStr);

                printf("Node %u got MSG_NETWORK_RetransmissionRequired"
                       " event at time %s\n",
                       node->nodeId,
                       clockStr);
            }

            mobileIp->mobileIpStat->mobileIpRegRetransmitted++;

            // Send self message MSG_NETWORK_RegistrationRequest with a
            // delay of Registration lifetime, so that after the lifetime
            // expires it again ask for the re-registration if there is no
            // other advertisement message  But getting granted lifetime from
            // Home agent, it should update the remaining lifetime later.

            reqTimeoutMsg = MESSAGE_Alloc(node,
                                          NETWORK_LAYER,
                                          NETWORK_PROTOCOL_MOBILE_IP,
                                          MSG_NETWORK_RegistrationRequest);

            // send the source and destination address in a structure
            // with the registration request event
            MESSAGE_InfoAlloc(node, reqTimeoutMsg, sizeof(SrcDestAddrInfo));

            sdAddr = (SrcDestAddrInfo*) MESSAGE_ReturnInfo(msg);
            sdTimeoutAddr = (SrcDestAddrInfo*)
                            MESSAGE_ReturnInfo(reqTimeoutMsg);
            sdTimeoutAddr->sourceAddress = sdAddr->sourceAddress;
            sdTimeoutAddr->destinationAddress = sdAddr->destinationAddress;

            MESSAGE_Send(node, reqTimeoutMsg, 0);

            break;
        }

        case MSG_NETWORK_AgentAdvertisementRefreshTimer:
        {

            // Move detection is handled by this event. The event is expired
            // after agent advertisement lifetime. If another agent adv.
            // comes within this lifetime, then mobile node is connected with
            // the same mobility agent. Otherwise it losts connection with
            // that mobility agent and then request solicitation for another
            // mobility agent binding
            AgentTimerInfo* agentAdv = (AgentTimerInfo*)
                                       MESSAGE_ReturnInfo(msg);
            AgentTimerInfo* newAgentAdv;

            if (mobileNode->remainingLifetime > agentAdv->remainingLifetime)
            {
                // still with the same mobility agent. Resend the self timer
                // for the next check
                Message* agentAdvMsg;
                agentAdvMsg = MESSAGE_Alloc(
                              node,
                              NETWORK_LAYER,
                              NETWORK_PROTOCOL_MOBILE_IP,
                              MSG_NETWORK_AgentAdvertisementRefreshTimer);

                MESSAGE_InfoAlloc(node,
                                  agentAdvMsg,
                                  sizeof(AgentTimerInfo));

                newAgentAdv = (AgentTimerInfo*) MESSAGE_ReturnInfo(agentAdvMsg);

                // set remaining lifetime
                newAgentAdv->remainingLifetime =
                (unsigned short) (node->getNodeTime() / SECOND) +
                agentAdv->advMsgTimerVal;

                // agent advertisement lifetime is sent for next use
                newAgentAdv->advMsgTimerVal = agentAdv->advMsgTimerVal;

                MESSAGE_Send(node,
                             agentAdvMsg,
                             agentAdv->advMsgTimerVal * SECOND);
            }
            else if (mobileNode->remainingLifetime < agentAdv->remainingLifetime)
            {
                // mobile node lost connection with current mobility agent.
                // Ask for another solicitation, to discover new mobility agent

                int index;
                NetworkDataIcmp* icmp = (NetworkDataIcmp*)
                    ipLayer->icmpStruct;

                // reset mobile node structure and ready to get new arrival
                mobileNode->homeAddress = 0;
                mobileNode->careOfAddress = 0;
                mobileNode->identNumber = 0;
                mobileNode->destinationAddress = 0;
                mobileNode->registrationLifetime = 0;
                mobileNode->remainingLifetime = 0;
                mobileNode->retransmissionLifetime =
                                    MIN_RETRANSMISSION_LIFETIME;
                mobileNode->isRegReplyRecv = TRUE;     //do not send regreq

                // increment seq no to ignore all the previous registration
                // request message
                mobileIp->sequenceNumber++;

                // send solicitation for another discovery

                for (index = 0; index < node->numberInterfaces; index++)
                {
                    Message* routerSolicitationMsg;
                    HostInterfaceStructure* hostInterfaceStructure =
                                   (HostInterfaceStructure*)
                                   icmp->interfaceStruct[index];

                    hostInterfaceStructure->
                        alreadyGotRouterSolicitationMessage = FALSE;
                    hostInterfaceStructure->solicitationNumber = 0;

                    routerSolicitationMsg =
                        MESSAGE_Alloc(
                            node,
                            NETWORK_LAYER,
                            NETWORK_PROTOCOL_ICMP,
                            MSG_NETWORK_IcmpRouterSolicitation);

                    MESSAGE_InfoAlloc(node,
                                      routerSolicitationMsg,
                                      sizeof(int));

                   *((int*) MESSAGE_ReturnInfo(routerSolicitationMsg)) = index;

                    MESSAGE_Send(node, routerSolicitationMsg, 0);

                    if (MOBILE_IP_DEBUG_DETAIL)
                    {
                        char clockStr[100];
                        ctoa(node->getNodeTime(), clockStr);

                        printf("Node %u sends "
                               "MSG_NETWORK_IcmpRouterSolicitation event "
                               "at time %s\n",
                               node->nodeId,
                               clockStr);
                    }
                }
            }
            break;
        }

        case MSG_NETWORK_VisitorListRefreshTimer:
        {
            // This timer expires after the accepted registration lifetime.
            // When foreign agent receives reply message from home, it starts
            // this timer with the lifetime accepted by home agent. On
            // expiration the corresponding visitor list entry will be removed.
            ForeignRegistrationList* regList;
            LinkedList* infoList = mobileIp->foreignAgentInfoList;

            ListItem* item = (ListItem*) infoList->first;
            clocktype identification =* ((clocktype*) MESSAGE_ReturnInfo(msg));

            // Search the visitor list
            while (item != NULL)
            {
                regList = (ForeignRegistrationList*) item->data;

                if (regList->identNumber == identification)
                {
                    // proper entry found in visitor list and remove it

                    if (MOBILE_IP_DEBUG)
                    {
                        char clockStr[100];
                        ctoa(regList->identNumber, clockStr);

                        printf("Node %u updates visitor list deleting"
                               " member with identNumber %s\n",
                               node->nodeId,
                               clockStr);
                    }

                    MobileIpUpdateForwardingTable(
                                   node,
                                   regList->sourceAddress,
                                   ANY_DEST,
                                   (unsigned) NETWORK_UNREACHABLE,
                                   DEFAULT_INTERFACE,
                                   ROUTING_ADMIN_DISTANCE_DEFAULT,
                                   ROUTING_PROTOCOL_STATIC);

                    if (MOBILE_IP_DEBUG_FORWARDING_TABLE)
                    {
                        NetworkPrintForwardingTable(node);
                    }

                    ListGet(node, infoList, item, TRUE, FALSE);
                    break;
                }
                item = item->next;
            }
            break;
        }

        case MSG_NETWORK_BindingListRefreshTimer:
        {
            // This timer expires after the remaining lifetime. When home agent
            // receives request message from foreign, it starts this timer with
            // the remaining lifetime. On expiration the corresponding binding
            // list entry will be removed here.

            NetworkDataIp* ipLayer = (NetworkDataIp*)
                    node->networkData.networkVar;

            NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
                    ipLayer->mobileIpStruct;

            LinkedList* infoList = mobileIp->homeAgentInfoList;

            ListItem* item = (ListItem*) infoList->first;
            HomeRegistrationList* regList;
            clocktype identification =* ((clocktype*) MESSAGE_ReturnInfo(msg));

            while (item != NULL)
            {
                regList = (HomeRegistrationList*) item->data;

                // check the entry from binding table to be deleted
                if (regList->identNumber == identification)
                {
                    MobileIpUpdateForwardingTable(node,
                                                 regList->homeAddress,
                                                 ANY_DEST,
                                                 regList->homeAddress,
                                                 DEFAULT_INTERFACE,
                                                 ROUTING_ADMIN_DISTANCE_DEFAULT,
                                                 ROUTING_PROTOCOL_STATIC);

                    if (MOBILE_IP_DEBUG_FORWARDING_TABLE)
                    {
                        NetworkPrintForwardingTable(node);
                    }

                    ListGet(node, infoList, item, TRUE, FALSE);
                    break;
                }
                else
                {
                    item = item->next;
                }
            }
            break;
        }

        default:
            ERROR_Assert(FALSE, "Invalid mobile IP event.");
    }
    MESSAGE_Free(node, msg);
}


// FUNCTION:   MobileIpHandleProtocolPacket()
// PURPOSE:    This function will be called from ProcessPacketForMeFromMac
//             function when protocol type is mobile IP. Here mobile IP
//             specific message are received and action made properly for any
//             such event.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:              node which is allocating message
//             nodeInput:         Structure containing contents of input file.
//             sourceAddress:     source address of this message
//             destinationAddress:Ip address in which interface this message
//                                has been come.
//             interfaceIndex:    interface at which request message comes
void MobileIpHandleProtocolPacket(
    Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    int interfaceIndex)
{
    MobileIpRegistrationRequest* regPacket = (MobileIpRegistrationRequest*)
            (MESSAGE_ReturnPacket(msg) + sizeof(TransportUdpHeader));

    switch (regPacket->type)
    {
        case REGISTRATION_REQUEST:
        {
            // send self message MSG_NETWORK_RegistrationReply to network
            // layer when node is home agent. Relayed the registration
            // message to the home agent when the node is foreign agent
            MobileIpCreateAndSendRegReply(node,
                                         msg,
                                         sourceAddress,
                                         destinationAddress,
                                         interfaceIndex);

            break;
        }

        case REGISTRATION_REPLY:
        {
            // Relayed the registration reply message to the mobile node when
            // the node is foreign agent. If mobile node receives reply
            // message the registration process is completed

            MobileIpReceiveRegReply(node,
                                    msg,
                                    sourceAddress,
                                    destinationAddress,
                                    interfaceIndex);

            break;
        }
        case PROXY_REQUEST:
        {
            MobileIpProxeyRequest* proxy = (MobileIpProxeyRequest*)regPacket;
            NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
            NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
                    ipLayer->mobileIpStruct;

            if (mobileIp == NULL ||
                mobileIp->agent == ForeignAgent ||
                mobileIp->agent == Others)
            {
                MobileIpUpdateForwardingTable(node,
                                             proxy->mobileNodeAddress,
                                             ANY_DEST,
                                             proxy->homeAgentAddress,
                                             DEFAULT_INTERFACE,
                                             ROUTING_ADMIN_DISTANCE_DEFAULT,
                                             ROUTING_PROTOCOL_STATIC);
            }

            MESSAGE_Free(node, msg);
            break;
        }
        default:
            break;
    }
}


// FUNCTION:   MobileIpPrintMobileNodeStat()
// PURPOSE:
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
static
void MobileIpPrintMobileNodeStat(
    Node* node,
    NetworkDataMobileIp* mobileIp)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf,
            "Registration requested = %u",
            mobileIp->mobileIpStat->mobileIpRegRequested);

    IO_PrintStat(node,
                 "Network",
                 "MobileIp",
                 ANY_DEST,
                 -1,     // instance Id
                 buf);


    sprintf(buf,
            "Registration reply accepted = %u",
            mobileIp->mobileIpStat->mobileIpRegAccepted);

    IO_PrintStat(node,
                 "Network",
                 "MobileIp",
                 ANY_DEST,
                 -1,     // instance Id
                 buf);
}


// FUNCTION:   MobileIpPrintBothAgentStat()
// PURPOSE:
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
static
void MobileIpPrintBothAgentStat(
    Node* node,
    NetworkDataMobileIp* mobileIp)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf,
            "Registration request relayed by Foreign Agent = %u",
            mobileIp->mobileIpStat->mobileIpRegRequestRelayedByForeign);

    IO_PrintStat(node,
                 "Network",
                 "MobileIp",
                 ANY_DEST,
                 -1,     // instance Id
                 buf);

    sprintf(buf,
            "Registration request received by Home Agent = %u",
            mobileIp->mobileIpStat->mobileIpRegRequestReceivedByHome);

    IO_PrintStat(node,
                 "Network",
                 "MobileIp",
                 ANY_DEST,
                 -1,     // instance Id
                 buf);

    sprintf(buf,
            "Registration replied by Home Agent = %u",
            mobileIp->mobileIpStat->mobileIpRegReplied);

    IO_PrintStat(node,
                 "Network",
                 "MobileIp",
                 ANY_DEST,
                 -1, // instance Id
                 buf);

    sprintf(buf,
            "Registration reply relayed by Foreign Agent = %u",
            mobileIp->mobileIpStat->mobileIpRegReplyRelayedByForeign);

    IO_PrintStat(node,
                 "Network",
                 "MobileIp",
                 ANY_DEST,
                 -1,     // instance Id
                 buf);

    sprintf(buf,
            "Number of Datagrams Encapsulate by Home Agent = %u",
            mobileIp->mobileIpStat->mobileIPDataPktEncapHome);

    IO_PrintStat(node,
                 "Network",
                 "MobileIp",
                 ANY_DEST,
                 -1,     // instance Id
                 buf);

    sprintf(buf,
            "Number of Datagrams Decapsulate by Foreign Agent = %u",
            mobileIp->mobileIpStat->mobileIPDataPktDcapForeign);

    IO_PrintStat(node,
                 "Network",
                 "MobileIp",
                 ANY_DEST,
                 -1,     // instance Id
                 buf);
}

// FUNCTION:   MobileIpPrintForeignAgentStat()
// PURPOSE:
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
static
void MobileIpPrintForeignAgentStat(
    Node* node,
    NetworkDataMobileIp* mobileIp)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf,
            "Registration request relayed by Foreign Agent = %u",
            mobileIp->mobileIpStat->mobileIpRegRequestRelayedByForeign);

    IO_PrintStat(node,
                 "Network",
                 "MobileIp",
                 ANY_DEST,
                 -1,     // instance Id
                 buf);

    sprintf(buf,
            "Registration reply relayed by Foreign Agent = %u",
            mobileIp->mobileIpStat->mobileIpRegReplyRelayedByForeign);

    IO_PrintStat(node,
                 "Network",
                 "MobileIp",
                 ANY_DEST,
                 -1,     // instance Id
                 buf);

    sprintf(buf,
            "Number of Datagrams Decapsulate by Foreign Agent = %u",
            mobileIp->mobileIpStat->mobileIPDataPktDcapForeign);

    IO_PrintStat(node,
                 "Network",
                 "MobileIp",
                 ANY_DEST,
                 -1,     // instance Id
                 buf);
}


// FUNCTION:   MobileIpPrintHomeAgentStat()
// PURPOSE:
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
static
void MobileIpPrintHomeAgentStat(
    Node* node,
    NetworkDataMobileIp* mobileIp)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf,
            "Registration request received by Home Agent = %u",
            mobileIp->mobileIpStat->mobileIpRegRequestReceivedByHome);

    IO_PrintStat(node,
                 "Network",
                 "MobileIp",
                 ANY_DEST,
                 -1,     // instance Id
                 buf);

    sprintf(buf,
            "Registration replied by Home Agent = %u",
            mobileIp->mobileIpStat->mobileIpRegReplied);

    IO_PrintStat(node,
                 "Network",
                 "MobileIp",
                 ANY_DEST,
                 -1, // instance Id
                 buf);

    sprintf(buf,
            "Number of Datagrams Encapsulate by Home Agent = %u",
            mobileIp->mobileIpStat->mobileIPDataPktEncapHome);

    IO_PrintStat(node,
                 "Network",
                 "MobileIp",
                 ANY_DEST,
                 -1,     // instance Id
                 buf);
}


// FUNCTION:   MobileIpFinalize()
// PURPOSE:    This function is called from NetworkIpInit() function to
//             initialize all mobile IP related data fields.
// RETURN:     NULL
// ASSUMTIONS: None
// PARAMETERS: node:          node which is allocating message
//             nodeInput:     Structure containing contents of input file.
void MobileIpFinalize(Node* node)
{
    NetworkDataIp* ipLayer = (NetworkDataIp*) node->networkData.networkVar;
    NetworkDataMobileIp* mobileIp = (NetworkDataMobileIp*)
            ipLayer->mobileIpStruct;

    if (MOBILE_IP_DEBUG_OUTPUT)
    {
        MobileIpPrintBindingList(node);

        MobileIpPrintVisitorList(node);
    }

    if (MOBILE_IP_DEBUG_FORWARDING_TABLE)
    {
        NetworkPrintForwardingTable(node);
    }

    if (!mobileIp->collectStatistics)
    {
        return;
    }

    // Determine whether node is mobile node or home agent or foreign agent
    AgentType agentType = mobileIp->agent;

    switch (agentType)
    {
        case MobileNode:
            MobileIpPrintMobileNodeStat(node, mobileIp);
            break;

        case BothAgent:
            MobileIpPrintBothAgentStat(node, mobileIp);
            break;

        case ForeignAgent:
            MobileIpPrintForeignAgentStat(node, mobileIp);
            break;

        case HomeAgent:
            MobileIpPrintHomeAgentStat(node, mobileIp);
            break;

        default:
            ERROR_Assert(FALSE, "Invalid node, program terminated");
            break;
    }
}
