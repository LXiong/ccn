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

#ifndef SIP_SESSION_H
#define SIP_SESSION_H

#include "string.h"

// /**
// CONSTANT    :: SIP_STR_SIZE16          : 16
// DESCRIPTION :: string size of 16 bits
// **/
#define     SIP_STR_SIZE16                  16

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
// ENUM        :: SipTransactionState
// DESCRIPTION :: Transaction state of sip node
// **/
enum SipTransactionState
{
    SIP_NORMAL,
    SIP_CLIENT,
    SIP_SERVER
};

// /**
// ENUM        :: SipCallState
// DESCRIPTION :: enumerates Calling state of a sip node
// **/
enum SipCallState
{
    SIP_INVALID = 0,
    SIP_IDLE = 1,       // The initial idle state, comes to this state from SIP_TERMINATED
                        // on sending/ recpt of OK for BYE (client/server)
    SIP_CALLING = 2,    // after sending INV(client)/ after recpt of INV(Server)
    SIP_TRYING = 3,     // after recpt of TRYING(client part only)
    SIP_RINGING = 4,    // after recpt of RINGING(client)/ after sending of RINGING
    SIP_OK = 5,         // after recpt of OK for INV(client)/ after sending of OK for INV,
                        // just establishing direct connection for proxy routed model
    SIP_COMPLETED = 6,  // after sending of ACK(client)/recpt of ACK (server)
                        // implies signalling completed, connection established
    SIP_TERMINATED = 7  // after sending/recpt of BYE (client/server)
};

//
// /**
// CLASS       :: SipSession
// DESCRIPTION :: Main Protocol Data structure required for each sip session
// **/
class SipSession
{
    SipCallState            callState;
    SipTransactionState     transactionState;
    int                     connectionId;
    short                   localPort; // sip's port
    unsigned short          rtpPort; // rtp's port
    char                    from[SIP_STR_SIZE64];
    char                    to[SIP_STR_SIZE64];
    char                    callId[SIP_STR_SIZE64];
    int                     proxyConnectionId;
    bool                    connDelayTimerRunning;

    void *                  sipMsg; // temporary holding place for a SIP message
public:

    void                    SipSetSipMessage(void* m) { sipMsg = m; }
    void *                  SipGetSipMessage() { return sipMsg; }

    SipSession(short portNum,
               SipTransactionState state = SIP_NORMAL);
    ~SipSession();

    void        SipSetCallState(SipCallState state) {callState = state;}
    void        SipSetLocalPort(int portNum) {localPort = portNum;}
    void        SipSetConnectionId(int connId) {connectionId = connId;}
    void        SipSetProxyConnectionId(int connId)
                    {proxyConnectionId = connId;}

    void        SipSetTo(char* toStr)
                {strcpy(to, toStr);}
    void        SipSetFrom(char* fromStr)
                {strcpy(from, fromStr);}
    void        SipSetCallId(char* callIdStr)
                {strcpy(callId, callIdStr);}

    void        SipSetTransactionState(SipTransactionState state)
    {transactionState = state;}

    void        SipSetConnDelayTimerRunning(bool flag)
    {connDelayTimerRunning = flag;}

    SipCallState        SipGetCallState() {return callState;}
    short               SipGetLocalPort() {return localPort;}
    int                 SipGetConnectionId() {return connectionId;}
    int                 SipGetProxyConnectionId()
                        {return proxyConnectionId;}

    SipTransactionState SipGetTransactionState()
    {return transactionState;}
    char*               SipGetTo() {return to;}
    char*               SipGetFrom() {return from;}
    char*               SipGetCallId() {return callId;}

    bool                SipGetConnDelayTimerRunning()
    {return connDelayTimerRunning;}

    unsigned short SipGetRtpPort(){return rtpPort;}
    void SipSetRtpPort(unsigned short port) {
        rtpPort = port;        
    }
};

#endif  /* SIP_DATA_H */
