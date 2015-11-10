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


#ifndef SIP_MSG_H
#define SIP_MSG_H

#include "app_voip.h"
#include "multimedia_sipsession.h"


// /**
// CONSTANT    :: SIP_MAX_FORWARDS        : 70
// DESCRIPTION :: Maximum number of hops a sip packet can travel
// **/
#define     SIP_MAX_FORWARDS                70

// /**
// CONSTANT    :: MAX_CONCURRENT_DLGS_PER_PROXY : 20
// DESCRIPTION :: Maximum number of simultaneous dialogs per proxy
// **/
#define     MAX_CONCURRENT_DLGS_PER_PROXY   20

// /**
// CONSTANT    :: NUMBER_OF_HOSTBITS      : 32
// DESCRIPTION :: Number of hostbits
// **/
#define     NUMBER_OF_HOSTBITS              32

// /**
// CONSTANT    :: SIG_TYPE_SIP   :          2
// DESCRIPTION :: Signalling protocol type definition for SIP
// **/
#define     SIG_TYPE_SIP                    2

// /**
// CONSTANT    :: SIP_STR_SIZE16          : 16
// DESCRIPTION :: string size of 16 bits
// **/
#define     SIP_STR_SIZE16                  16

// /**
// CONSTANT    :: IP_ADDRESS_LENGTH        : 16
// DESCRIPTION :: IP address length of dotted decimal format a.b.c.d
// **/
#define     IP_ADDRESS_LENGTH               16

// /**
// CONSTANT    :: SIP_STR_SIZE32          : 32
// DESCRIPTION :: string size of 32 bits
// **/
#define     SIP_STR_SIZE32                  32

// /**
// CONSTANT    :: SIP_STR_SIZE64          : 64
// DESCRIPTION :: string size of 64 bits
// **/
#define     SIP_STR_SIZE64                  64

// /**
// CONSTANT    :: MAX_HOSTS_PER_PROXYBLOCK: 10
// DESCRIPTION :: Max no.of UA nodes alloted per one block
// **/
#define     MAX_HOSTS_PER_PROXYBLOCK        10

// /**
// CONSTANT    :: SIP_TRANS_PROTOCOL      : TCP
// DESCRIPTION :: Transport protocol used by sip
// **/
#define     SIP_TRANS_PROTOCOL              TCP

// /**
// CONSTANT    :: INVALID_ID              : -1
// DESCRIPTION :: Initializer used to init connectionId
// **/
#define     INVALID_ID                      -1

// /**
// CONSTANT    :: MAX_SIP_PACKET_SIZE     : 2560
// DESCRIPTION :: Maximum sip packet size
// **/
#define     MAX_SIP_PACKET_SIZE             2560

// /**
// CONSTANT    :: SIP_VIA_COUNT           : 4
// DESCRIPTION :: Count of number of vias
// **/
#define     SIP_VIA_COUNT                   4

// /**
// CONSTANT    :: MIN_SIP_HDR_SIZE        : 50
// DESCRIPTION :: Min. sip message length
// **/
#define     MIN_SIP_HDR_SIZE                50


// /**
// STRUCT      :: SipVia
// DESCRIPTION :: structure holding SipVia field
// **/
struct SipVia
{
    char viaProxy[SIP_STR_SIZE32];
    char branch[SIP_STR_SIZE16];
    char receivedIp[SIP_STR_SIZE16];
    bool isReceivedIp;
};


// /**
// ENUM        :: SipMsgType
// DESCRIPTION :: Type of sip message
// **/
enum SipMsgType
{
    SIP_INVALID_MSG,
    SIP_REQUEST,
    SIP_RESPONSE
};

// /**
// ENUM        :: SipReqType
// DESCRIPTION :: Type of sip request
// **/
enum SipReqType
{
    INVALID_REQ,
    INV,
    ACK,
    BYE,
    CAN,
    OPT,
    REG
};

// /**
// ENUM        :: SipAckType
// DESCRIPTION :: Type of sip acknowledgement
// **/
enum SipAckType
{
    SIP_ACK_INVALID,
    SIP_ACK_2XX,
    SIP_ACK_NON2XX
};

// /**
// ENUM        :: SipResType
// DESCRIPTION :: Type of sip response(For details refer sec.21 of RFC3261)
// **/
enum SipResType
{
    INVALID_RES             = 0,

    TRYING                  = 100,
    RINGING                 = 180,
    CALL_FORWARDED          = 181,
    CALL_QUEUED             = 182,
    CALL_PROGRESS           = 183,

    OK                      = 200,

    MULTIPLE_CHOICE         = 300,
    MOVED_PERMANENTLY       = 301,
    MOVED_TEMPORARILY       = 302,
    USE_PROXY               = 305,
    ALTERNATE_SERVICE       = 380,

    BAD_REQUEST             = 400,
    UNAUTHORIZED_ACCESS     = 401,
    PAYMENT_REQUIRED        = 402,
    FORBIDDEN               = 403,
    CALLED_PARTY_NOT_FOUND  = 404,
    REQUEST_NOT_ALLOWED     = 405,
    NOT_ACCEPTABLE          = 406,
    PROXY_AUTHENTICN_REQD   = 407,
    REQUEST_TIME_OUT        = 408,
    GONE                    = 410,
    REQ_ENTITY_LARGE        = 413,
    REQ_URI_LONG            = 414,
    UNSUPPORTED_MEDTYPE     = 415,
    UNSUPPORTED_URI_SCHEME  = 416,
    BAD_PROTOCOL_EXTENSION  = 420,
    TOO_BRIEF_INTERVAL      = 423,
    TEMP_UNAVAILABLE        = 480,
    NO_MATCHING_TRANSACTION = 481,
    SIP_LOOP_DETECTED       = 482,
    TOO_MANY_HOPS           = 483,
    ADDRESS_INCOMPLETE      = 484,
    AMBIGUOUS               = 485,
    BUSY_HERE               = 486,
    REQUEST_TERMINATED      = 487,
    NOT_ACCEPTABLE_HERE     = 488,
    REQUEST_PENDING         = 491,
    UNDECIPHERABLE          = 493,

    SERVER_INTERNAL_ERR     = 500,
    NOT_IMPLEMENTED         = 501,
    BAD_GATEWAY             = 502,
    SERVICE_UNAVILABLE      = 503,
    SERVER_TIME_OUT         = 504,
    VERSION_NOT_SUPPORTED   = 505,
    MESSAGE_TOO_LARGE       = 513,

    BUSY_EVERYWHERE         = 600,
    DECLINE                 = 603,
    USER_DOESNOT_EXIST      = 604,
    GLO_NOT_ACCEPTABLE      = 606
};


class SipData;


// /**
// CLASS       :: SipMsg
// DESCRIPTION :: Represents generic sip message structure
// **/
class SipMsg
{
    char*       pBuffer;
    unsigned    pLength;   // length of the full packet
    unsigned    totHdrLen; // length of the header fields only
    SipMsgType  msgType;         // i.e. pLength - firstLine-blankline
    SipReqType  requestType;
    SipResType  responseType;
    SipAckType  ackType;

    NodeAddress targetIp;

    // As corresponding to targetIp there can be URL in DNS enabled system
    char        targetUrl[MAX_STRING_LENGTH]; 

    unsigned short fromPort;

    NodeAddress fromIp;

    int         connectionId;
    int         proxyConnectionId;

    int         viaCount;
    int         maxViaCount;

    // Main SipMessage components
    char        transProto[SIP_STR_SIZE16];
    char        domainName[SIP_STR_SIZE64];
    SipVia*     via;
    char        from[SIP_STR_SIZE64];
    char        to[SIP_STR_SIZE64];
    char        callId[SIP_STR_SIZE64];
    char        tagTo[SIP_STR_SIZE16];
    char        tagFrom[SIP_STR_SIZE16];
    char        cSeq[SIP_STR_SIZE16];
    char        cSeqMsg[SIP_STR_SIZE16];
    char        contact[SIP_STR_SIZE64];
    char        callInfo[SIP_STR_SIZE64];
    char        contentType[SIP_STR_SIZE64];
    unsigned    contentLength;
    unsigned    maxForwards;

public:

    SipMsg*     next;

                SipMsg();
                ~SipMsg();
    char*       SipGetTo() {return to;}
    char*       SipGetTagTo() {return tagTo;}
    char*       SipGetFrom() {return from;}
    char*       SipGetTagFrom() {return tagFrom;}
    char*       SipGetCallId() {return callId;}
    char*       SipGetCSeqMsg() {return cSeqMsg;}
    char*       SipGetCSeq() {return cSeq;}
    char*       SipGetTransProto() {return transProto;}
    char*       SipGetCallInfo(){return callInfo;}
    char*       SipGetContact() {return contact;}
    SipVia*     SipGetSipVia(){return via;}
    int         SipGetViaCount(){return viaCount;}

    int         SipGetConnectionId() {return connectionId;}
    void        SipSetConnectionId(int connId) {connectionId = connId;}

    int         SipGetProxyConnectionId() {return proxyConnectionId;}
    void        SipSetProxyConnectionId(int connId)
                    {proxyConnectionId = connId;}

    NodeAddress SipGetTargetIp() {return targetIp;}
    void        SipSetTargetIp(NodeAddress toAddr) {targetIp = toAddr;}

    unsigned short SipGetFromPort() {return fromPort;}
    void        SipSetFromPort(unsigned short port) {fromPort = port;}

    NodeAddress SipGetFromIp() {return fromIp;}
    void        SipSetFromIp(NodeAddress frIp) {fromIp = frIp;}

    int         SipGetMaxForwards(){return --maxForwards;}

    void        SipSetProperConnectionId(
                    SipData* sip, int connectionId, SipMsg* oldMsg);
    char*       SipGetTargetUrl(){return targetUrl;}
    void        SipSetTargetUrl(char* url){strcpy(targetUrl , url);}

    SipMsgType  SipGetMsgType() {return msgType;}
    SipReqType  SipGetReqType() {return requestType;}
    SipResType  SipGetResType() {return responseType;}
    SipAckType  SipGetAckType() {return ackType;}
    char*       SipGetBuffer() {return pBuffer;}
    unsigned    SipGetLength() {return pLength;}

    void        SipParseMsg(char* msgBuf, unsigned len);
    void        SipFillComponents();
    void        SipParseField(char* sipField);
    void        SipParseTo(char* toField);
    void        SipParseFrom(char* fromField);
    void        SipParseContact(char* contactField);

    void        SipAllocateVia();
    void        SipParseVia(char* locationPtr);

    void        SipCreateInviteRequestPkt(Node* node, SipData* sip,
                        VoipHostData* voip);

    void        SipCreate2xxAckRequestPkt(Node* node, SipData* sip);

    void        SipCreateNon2xxAckRequestPkt(Node* node, SipData* sip);

    void        SipCreateByeRequestPkt(Node* node, SipData* sip,
                                       SipTransactionState state);

    void        SipCreateResponsePkt(Node* node, SipData* sip,
                        SipResType responseType, const char* requestMsg);

    void        SipGetResponseString(SipResType responseType,
                        char* responseStr);

    void        SipGetClientBranch(char* clBranch)
                    { strcpy(clBranch, via[viaCount - 1].branch);}

    SipMsgType  SipIsRequestMsg(char* startLine);

    SipMsgType  SipIsResponseMsg(char* responseToken, char* resString);


    void        SipGenerateVia(char* viaGen, int start = 0,
                    bool isBye = false);

    void        SipGenViaWithOwnVia(char* currVia, SipData* sip);

    void        SipGenViaWithoutTopVia(char* localVia);

    void        SipUpdateViaWithReceivedIp();

    void        SipForwardInviteReqPkt(Node* node);

    void        SipForwardResponsePkt(Node* node, SipResType resp);

//---------------------------------------------------------------------------
// FUNCTION:    SipCreate2xxOkResponsePkt()
// PURPOSE:     Create a 200 OK response packet
// PARAMETERS:  node::Pointer to the node
//              sip::Pointer to SipData
//              voip::Pointer to VoipHostData
// RETURN:      None.
//---------------------------------------------------------------------------
    void        SipCreate2xxOkResponsePkt(Node* node,
                                          SipData* sip,
                                          VoipHostData* voip);

};


#endif  /* SIP_MSG_H */
