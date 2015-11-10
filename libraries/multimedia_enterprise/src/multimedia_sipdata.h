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

#ifndef SIP_DATA_H
#define SIP_DATA_H

#include "list.h"
#include "multimedia_sipsession.h"
#include "multimedia_sipmsg.h"
#include "circularbuffer.h"
#include <vector>


// /**
// CONSTANT    :: RULE_IDX1                 :   17
// DESCRIPTION :: value of rule index1 for random no generation
// **/
#define RULE_IDX1                               17

// /**
// CONSTANT    :: RULE_IDX2                 :   31
// DESCRIPTION :: value of rule index2 for random no generation
// **/
#define RULE_IDX2                               31

// /**
// CONSTANT    :: RULE_VECTOR_VAL1          :   90
// DESCRIPTION :: Rule vector1 value for CA size 32
// **/
#define RULE_VECTOR_VAL1                        90

// /**
// CONSTANT    :: RULE_VECTOR_VAL2          :   150
// DESCRIPTION :: Rule vector2 value for CA size 32
// **/
#define RULE_VECTOR_VAL2                        150

// /**
// CONSTANT    :: CA_SIZE                   :   32
// DESCRIPTION :: Number of bits of the generated output
// **/
#define CA_SIZE                                 32

// /**
// CONSTANT    :: RAND_MULT                 :   1000
// DESCRIPTION :: Multiplier used for getting short values
// **/
#define RAND_MULT                               1000

// /**
// CONSTANT    :: SIP_CIR_BUF_SIZE          :  2048
// DESCRIPTION :: Circular buffer size used for SIP
// **/
#define SIP_CIR_BUF_SIZE                        2048


// /**
// STRUCT      :: SipHostNode
// DESCRIPTION :: structure to hold details of a sip node
// **/
struct SipHostNode
{
    unsigned hostId;
    unsigned hostIp;
    Int16 hostInterfaceIndex;
    char     hostName[SIP_STR_SIZE32];
};

// /**
// STRUCT      :: SipHostBlock
// DESCRIPTION :: structure for holding a block of sip nodes
// **/
struct SipHostBlock
{
    SipHostNode     hostList[MAX_HOSTS_PER_PROXYBLOCK];
    // 0 based(for a single host,hostCount is 1)
    unsigned short  hostCount;
    SipHostBlock*   next;
};

// /**
// STRUCT      :: SipHostBlockList
// DESCRIPTION :: structure holding a linked list of sipnode blocks
//                (one per proxy node, 0 for UA node)
// **/
struct SipHostBlockList
{
    SipHostBlock*  hostBlockHead;
    SipHostBlock*  hostBlockTail;
};

// /**
// STRUCT      :: SipDomain
// DESCRIPTION :: structure to hold domain details
// **/
struct SipDomain
{
    char        domainName[SIP_STR_SIZE64]; // one proxy per domain
    NodeAddress proxyIp;
    std::string* proxyUrl;
};

// /**
// STRUCT      :: SipStat
// DESCRIPTION :: structure for recording the sip statistics
// **/
struct SipStat
{
    // UAC part
    unsigned short  inviteSent;
    unsigned short  ackSent;
    unsigned short  byeSent;
    unsigned short  optSent;
    unsigned short  cancelSent;
    unsigned short  regnSent;
    unsigned        totReqSent;

    unsigned short  tryingRecd;
    unsigned short  ringingRecd ;
    unsigned short  okRecd ;
    unsigned short  cliErrRecd ;
    unsigned short  srvErrRecd;
    unsigned short  gblErrRecd ;
    unsigned        totRespRecd;

    // UAS part
    unsigned short  inviteRecd;
    unsigned short  ackRecd;
    unsigned short  byeRecd;
    unsigned short  optRecd;
    unsigned short  cancelRecd;
    unsigned short  regnRecd;
    unsigned        totReqRecd;

    unsigned short  ringingSent ;
    unsigned short  okSent ;
    unsigned short  cliErrSent ;
    unsigned short  srvErrSent;
    unsigned short  gblErrSent ;
    unsigned        totRespSent;

    // proxy part
    unsigned short  reqForwarded;
    unsigned short  respForwarded;
    unsigned short  reqRedirected;
    unsigned short  reqDropped;
    unsigned short  respDropped;
    unsigned short  tryingSent;

    // overall
    unsigned short  callAttempted;
    unsigned short  callConnected;
    unsigned short  callRejected;
};


// /**
// ENUM        :: SipHostType
// DESCRIPTION :: Type of sip node
// **/
enum SipHostType
{
    SIP_UA,
    SIP_PROXY,
    SIP_REDIRECT,
    SIP_REGISTER
};


// /**
// ENUM        :: SipTransProtocolType
// DESCRIPTION :: Type of transport layer protocol used by sip
// **/
enum SipTransProtocolType
{
    TCP = 1,
    UDP = 2,
    OTHERS = 3
};

// /**
// ENUM        :: SipCallModel
// DESCRIPTION :: Mode of Sip call
// **/
enum SipCallModel
{
    SipDirect,
    SipProxyRouted
};


typedef std::vector<CircularBuffer*> CirBufVector;
typedef std::vector<CircularBuffer*>::iterator cbVectorIterator;

// /**
// CLASS       :: SipData
// DESCRIPTION :: Main Protocol Data structure required at each node
// **/
class SipData
{
    SipTransProtocolType    transProto;
    SipHostType             hostType;
    SipHostBlockList        hostBlockList;
    SipCallModel            callModel;
    LinkedList*             domainList;
    CirBufVector            cbVector;

    Address                 callSignalAddress; // temporarily added by Li   
    unsigned short          callSignalPort;
    char*                   ownAliasAddr;
    char*                   domainName;
    char*                   FQDN;

    unsigned                proxyId;
    unsigned                proxyIp;
    std::string*            proxyURL;
    BOOL                    isSipStatEnabled;
    SipStat                 stat;

    unsigned                branchCounter;
    unsigned                tagCounter;
    RandomSeed              seed;    // used to set SeedState
    RandomSeed              tagSeed; // used for generating tags
    unsigned                callId;
    unsigned                seqCounter;
    SipMsg*                 sipMsgList;

    void                    SipAllocateNextBlock();

    // For unique 32 bit number generation
    int*                    ruleVector;
    unsigned int*           seedState;
    int*                    nextState;
    int                     sizeOfCA;
    unsigned int            count;

    unsigned int            SipFindNextState(Node*);
    void                    SipInitRuleVectorAndSeedState
                                (int, int*, unsigned int*, Node*);
    unsigned int            SipBinToDec(int, int*);

    LinkedList* sipSessionList;
    // Dynamic Address
    NodeId destNodeId; // destination node id for this app session 
    Int16 clientInterfaceIndex; // client interface index for this app 
                                // session
    Int16 destInterfaceIndex; // destination interface index for this app
                              // session
    NodeAddress             callSignalAddr;

public:
                SipData(Node* node, SipHostType hostType);
                ~SipData();
    // UAC part
    void        SipIncrementInviteSent(){stat.inviteSent++;}
    void        SipIncrementAckSent(){stat.ackSent++;}
    void        SipIncrementByeSent(){stat.byeSent++;}
    void        SipIncrementTryingRecd(){stat.tryingRecd++;}
    void        SipIncrementRingingRecd(){stat.ringingRecd++;}
    void        SipIncrementOkRecd(){stat.okRecd++;}

    // UAS part
    void        SipIncrementInviteRecd(){stat.inviteRecd++;}
    void        SipIncrementAckRecd(){stat.ackRecd++;}
    void        SipIncrementByeRecd(){stat.byeRecd++;}
    void        SipIncrementRingingSent(){stat.ringingSent++;}
    void        SipIncrementOkSent(){stat.okSent++;}

    // proxy part
    void        SipIncrementReqForwarded(){stat.reqForwarded++;}
    void        SipIncrementRespForwarded(){stat.respForwarded++;}
    void        SipIncrementReqRedirected(){stat.reqRedirected++;}
    void        SipIncrementReqDropped(){stat.reqDropped++;}
    void        SipIncrementRespDropped(){stat.respDropped++;}
    void        SipIncrementTryingSent(){stat.tryingSent++;}

    // overall
    void        SipIncrementCallAttempted(){stat.callAttempted++;}
    void        SipIncrementCallConnected(){stat.callConnected++;}
    void        SipIncrementCallRejected(){stat.callRejected++;}

    CirBufVector* SipGetCirBufVector(){ return &cbVector;}

    void        SipStatsInit(Node* node, const NodeInput* nodeInput);
    bool        SipInsertToHostBlock(SipHostNode* phost);

    void        SipReadConfigFile(Node* node, const NodeInput* nodeInput);
    void        SipReadAliasFileForHost(Node* node,
                    const NodeInput* nodeInput);
    void        SipReadAliasFileForProxy(Node* node,
                    const NodeInput* nodeInput);
    void        SipPrintAliasInfo(Node* node);

    void        SipReadDNSFile(Node* node, const NodeInput* nodeInput);
    void        SipPrintDNSData(Node* node);

    void        SipIncrementBranchCounter(){branchCounter++;}

    void        SipGenerateBranch(char* branch)
                {sprintf(branch, "z9hG4bK%d", branchCounter++);}

    int         SipGenerateSeqCounter() {return seqCounter++;}

                // TBD: we may need to generate tag based on some encoding tech.
    void        SipGenerateTag(Node* node, char* tag)
                {
                    double d = RANDOM_erand(tagSeed);
                    unsigned n = (unsigned) d * RAND_MULT;
                    if (n == 0) n = 111;
                    sprintf(tag, "%d%u", node->nodeId,n);
                }

    void        SipGenerateCallIdPfx(Node* node, char* callIdPrefix)
                {
                    unsigned n = SipFindNextState(node);
                    sprintf(callIdPrefix, "%u@%s", n, FQDN);
                }


    void        SipInsertMsg(SipMsg* msg);
    void        SipDeleteMsg(int connectionId);
    SipMsg*     SipFindMsg(int connectionId);
    SipMsg*     SipFindMsg(unsigned short fromPort);
    SipMsg*     SipFindMsg(SipMsg* sipMsg);
    void        SipRemoveAllMsg();

    char*       SipGetFQDN(){return FQDN;}
    void        SipSetFQDN(char* fqdn)
                { FQDN = (char*) MEM_malloc(strlen(fqdn) + 1);
                  memset(FQDN, 0, strlen(fqdn) + 1);
                  memcpy(FQDN, fqdn, strlen(fqdn));
                }

    SipStat     SipGetStat(){return stat;}

    int         SipGetMirrorConnectionId(int connectionId);

    SipHostType SipGetHostType() {return hostType;}

    void        SipGetHostIp(char* hostName, unsigned& hostIp);

//---------------------------------------------------------------------------
// FUNCTION   :: SipGetHostId
// LAYER      :: APPLICATION
// PURPOSE    :: Read hostId from the hostAlias name(External Interface 2)
// PARAMETERS ::
// + hostName  : char* : host name
// + hostId    : unsigned : host Id
// RETURN     :: void : NULL
//---------------------------------------------------------------------------
    void        SipGetHostId(char* hostName, unsigned& hostId);

    void        SipGetDestProxyIp(char* sipTo, unsigned& proxyIp);
//---------------------------------------------------------------------------
// FUNCTION   :: SipGetDestProxyUrl
// LAYER      :: APPLICATION
// PURPOSE    :: Get Destination Proxy Url address
// PARAMETERS ::
// + domainName: char* : domain name
// + proxyUrl   : char*: proxy Url address
// RETURN     :: unsigned int : decimal number required
//---------------------------------------------------------------------------
    void        SipGetDestProxyUrl(char* sipTo, char* proxyUrl);

    char*       SipGetDomainName() { return domainName;}

    unsigned    SipGetCallSignalPort(){return callSignalPort++;}

    char*       SipGetOwnAliasAddr() { return ownAliasAddr;}

    SipHostBlockList   SipGetHostBlockList() { return hostBlockList;}

    unsigned    SipGetProxyIp() { return proxyIp;}

    std::string    SipGetProxyUrl() { return *proxyURL;}

    unsigned    SipRetrieveCallSignalPort(){return callSignalPort - 1;}

    SipCallModel         SipGetCallModel() {return callModel;}

    SipTransProtocolType SipGetTransProtocolType() { return transProto;}
    BOOL        SipGetIsSipStatEnabled() { return isSipStatEnabled;}

    LinkedList* SipGetSipSessionList()
    {
        return sipSessionList;
    }

    SipSession*
    SipGetSipSessionByLocalPort(short portNum)
    {
        ListItem* listItem = sipSessionList->first;
        SipSession* sipSession = NULL;

        while (listItem)
        {
            sipSession = (SipSession*)listItem->data;

            if (sipSession->SipGetLocalPort() == portNum)           
            {
                return sipSession;
            }

            listItem = listItem->next;
        }

        return NULL;
    }

    SipSession*
    SipGetSipSessionByRtpPort(short portNum)
    {
        ListItem* listItem = sipSessionList->first;
        SipSession* sipSession = NULL;

        while (listItem)
        {
            sipSession = (SipSession*)listItem->data;

            //if (sipSession->SipGetLocalPort() == portNum)
            if ((short)(sipSession->SipGetRtpPort()) == portNum)
            {
                return sipSession;
            }
            listItem = listItem->next;
        }

        return NULL;
    }

    SipSession*
    SipGetSipSessionByConnectionId(int connId)
    {
        ListItem* listItem = sipSessionList->first;
        SipSession* sipSession = NULL;

        while (listItem)
        {
            sipSession = (SipSession*)listItem->data;

            if (sipSession->SipGetConnectionId() == connId
                ||
                sipSession->SipGetProxyConnectionId() == connId)
            {
                return sipSession;
            }

            listItem = listItem->next;
        }

        return NULL;
    }

    SipSession*
    SipGetSipSessionByToFromCallId(
        char* toStr,
        char* fromStr,
        char* callIdStr)
    {
        ListItem* listItem = sipSessionList->first;
        SipSession* sipSession = NULL;

        while (listItem)
        {
            sipSession = (SipSession*)listItem->data;

            if ((strcmp(sipSession->SipGetTo(), toStr) == 0
                    &&
                    strcmp(sipSession->SipGetFrom(), fromStr) == 0
                    &&
                    strcmp(sipSession->SipGetCallId(), callIdStr) == 0)
                ||
                (strcmp(sipSession->SipGetTo(), fromStr) == 0
                    &&
                    strcmp(sipSession->SipGetFrom(), toStr) == 0
                    &&
                    strcmp(sipSession->SipGetCallId(), callIdStr) == 0))
            {
                return sipSession;
            }
            listItem = listItem->next;
        }

        return NULL;
    }

    void        SipSetProxyId(unsigned proxyID) { proxyId = proxyID;}

    void        SipSetProxyIp(unsigned proxyIP) {proxyIp = proxyIP;}

    void        SipSetHostType(SipHostType hType) {hostType = hType;}

    void        SipSetCallSignalPort(unsigned short signalPort)
                { callSignalPort = signalPort;}

    void        SipSetCallSignalAddr(NodeAddress signalAddr)
                { callSignalAddr = signalAddr;}

    void        SipSetIsSipStatEnabled(BOOL value)
                { isSipStatEnabled = value;}

    void        SipSetCallModel(SipCallModel inpCallModel)
                { callModel = inpCallModel;}

    void        SipSetOwnAliasAddr(char* ownAliasAndDomain)
                {   ownAliasAddr = (char*) MEM_malloc(strlen
                     (ownAliasAndDomain) + 1);
                    memset(ownAliasAddr, 0, strlen(ownAliasAndDomain) + 1);
                    memcpy(ownAliasAddr, ownAliasAndDomain,
                    strlen(ownAliasAndDomain));
                }

    void        SipSetDomainName(char* dName)
                { domainName = (char*) MEM_malloc(strlen(dName) + 1);
                  memset(domainName, 0, strlen(dName) + 1);
                  memcpy(domainName, dName, strlen(dName));
                }

    void SipSessionListInit(Node* node)
    {
        ListInit(node, &sipSessionList);
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
     void SipUpdateTargetIp(unsigned short sourcePort,
                      NodeAddress resolvedAddr,
                      char* resolvedUrl );


    void setSignalAddr(const Address & addr) {callSignalAddress = addr;}
    Address getSignalAddr(){return callSignalAddress;}
};

bool
SIPDATA_fillAliasAddrForNodeId (NodeAddress destNodeId,
    const NodeInput * nodeInput,
    char * destAliasAddrBuf);

#endif  /* SIP_DATA_H */
