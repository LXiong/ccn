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

#ifndef _MOBILE_IP_H_
#define _MOBILE_IP_H_

#include "main.h"

#define MAX_ADVERTISED_NO    1

#define AUTHENTICATOR_SIZE   16  /* 128(16*8) bits for keyed MD5 algorithm */

// Different solicitation interval

#define MOBILE_IP_SOLICITATION_INTERVAL (1 * SECOND)
#define MOBILE_IP_MAX_SOLITATION_INTERVAL (1 * MINUTE)

#define REGISTRATION_LIFETIME     200
#define DEREGISTRATION_LIFETIME 0
#define MIN_RETRANSMISSION_LIFETIME 1

// All mobility agent multicast address (224.0.0.11)
#define ALL_MOBILITY_AGENT_MULTICAST_ADDRESS 0xE000000B

// UDP destination port
#define MOBILE_IP_DESTINATION_PORT 434
#define MOBILE_IP_MAX_JITTER (100 * MILLI_SECOND)

// Structure of agent advertisement extension
typedef struct agent_advertisement_extension
{
    unsigned char  extensionType;
    unsigned char  extensionLength;
    unsigned short sequenceNumber;
    unsigned short registrationLifetime;
    UInt16 agentAdvExt;//registrationRequired:1,
                   //busy:1,
                   //homeAgent:1,
                   //foreignAgent:1,
                   //minimalEncapsulation:1,
                   //greEncapsulation:1,
                   //jacobsonHeaderCompression:1,
                   //reserved:9;
    NodeAddress    careOfAddress; // assume 1 care of address for a foreign
} AgentAdvertisementExtension;

// FUNCTION:     AgentAdvertisementExtensionSetRegReq()
// PURPOSE:       Set the value of registrationRequired for
//                AgentAdvertisementExtension
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    agentAdvExt:    The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
//                registrationRequired: Input value for set operation
static void AgentAdvertisementExtensionSetRegReq(UInt16 *agentAdvExt,
                                                 BOOL registrationRequired)
{
    UInt16 x = (UInt16) registrationRequired;

    //masks registrationRequired within boundry range
    x = x & maskShort(16, 16);

    //clears the first bit
    *agentAdvExt = *agentAdvExt & (~(maskShort(1, 1)));

    //setting value of x in agentAdvExt
    *agentAdvExt = *agentAdvExt | LshiftShort(x, 1);
}


// FUNCTION:      AgentAdvertisementExtensionSetBusy()
// PURPOSE:       Set the value of busy for AgentAdvertisementExtension
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    agentAdvExt:    The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
//                busy:          Input value for set operation
static void AgentAdvertisementExtensionSetBusy(UInt16 *agentAdvExt,
                                               BOOL busy)
{
    UInt16 x = (UInt16) busy;

    //masks busy within boundry range
    x = x & maskShort(16, 16);

    //clears the second bit
    *agentAdvExt = *agentAdvExt & (~(maskShort(2, 2)));

    //setting value of x in agentAdvExt
    *agentAdvExt = *agentAdvExt | LshiftShort(x, 2);
}


// FUNCTION:      AgentAdvertisementExtensionSetHAgent()
// PURPOSE:       Set the value of homeAgent for
//                AgentAdvertisementExtension
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    agentAdvExt:    The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
//               homeAgent:      Input value for set operation
static void AgentAdvertisementExtensionSetHAgent(UInt16 *agentAdvExt,
                                                 BOOL homeAgent)
{
    UInt16 x = (UInt16) homeAgent;

    //masks homeAgent within boundry range
    x = x & maskShort(16, 16);

    //clears the third bit
    *agentAdvExt = *agentAdvExt & (~(maskShort(3, 3)));

    //setting value of x in agentAdvExt
    *agentAdvExt = *agentAdvExt | LshiftShort(x, 3);
}


// FUNCTION:      AgentAdvertisementExtensionSetFAgent()
// PURPOSE:       Set the value of foreignAgent for
//                AgentAdvertisementExtension
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    agentAdvExt:    The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
//               foreignAgent:    Input value for set operation
static void AgentAdvertisementExtensionSetFAgent(UInt16 *agentAdvExt,
                                                 BOOL foreignAgent)
{
    UInt16 x = (UInt16) foreignAgent;

    //masks foreignAgent within boundry range
    x = x & maskShort(16, 16);

    //clears the fourth bit
    *agentAdvExt = *agentAdvExt & (~(maskShort(4, 4)));

    //setting value of x in agentAdvExt
    *agentAdvExt = *agentAdvExt | LshiftShort(x, 4);
}


// FUNCTION:      AgentAdvertisementExtensionSetMinEncap()
// PURPOSE:       Set the value of minimalEncapsulation for
//                AgentAdvertisementExtension
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    agentAdvExt:    The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
//                minimalEncapsulation: Input value for set operation
static void AgentAdvertisementExtensionSetMinEncap(UInt16 *agentAdvExt,
                                                   BOOL minimalEncapsulation)
{
    UInt16 x = (UInt16) minimalEncapsulation;

    //masks minimalEncapsulation within boundry range
    x = x & maskShort(16, 16);

    //clears the fifth bit
    *agentAdvExt = *agentAdvExt & (~(maskShort(5, 5)));

    //setting value of x in agentAdvExt
    *agentAdvExt = *agentAdvExt | LshiftShort(x, 5);
}


// FUNCTION:      AgentAdvertisementExtensionSetGreEncap()
// PURPOSE:       Set the value of greEncapsulation for
//                AgentAdvertisementExtension
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    agentAdvExt:    The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
//                greEncapsulation: Input value for set operation
static void AgentAdvertisementExtensionSetGreEncap(UInt16 *agentAdvExt,
                                                   BOOL greEncapsulation)
{
    UInt16 x = (UInt16) greEncapsulation;

    //masks greEncapsulation within boundry range
    x = x & maskShort(16, 16);

    //clears the sixth bit
    *agentAdvExt = *agentAdvExt & (~(maskShort(6, 6)));

    //setting value of x in agentAdvExt
    *agentAdvExt = *agentAdvExt | LshiftShort(x, 6);
}


// FUNCTION:      AgentAdvertisementExtensionSetJHdrCmpression()
// PURPOSE:       Set the value of jacobsonHeaderCompression for
//                AgentAdvertisementExtension
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    agentAdvExt:    The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
//                jacobsonHeaderCompression: the value that is supposed to
//                                            be set
static void AgentAdvertisementExtensionSetJHdrCmpression(UInt16 *agentAdvExt,
                                              BOOL jacobsonHeaderCompression)
{
    UInt16 x = (UInt16) jacobsonHeaderCompression;

    //masks jacobsonHeaderCompression within boundry range
    x = x & maskShort(16, 16);

    //clears the seventh bit
    *agentAdvExt = *agentAdvExt & (~(maskShort(7, 7)));

    //setting value of x in agentAdvExt
    *agentAdvExt = *agentAdvExt | LshiftShort(x, 7);
}


// FUNCTION:      AgentAdvertisementExtensionSetReserved()
// PURPOSE:       Set the value of reserved for AgentAdvertisementExtension
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    agentAdvExt:    The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
//                 reserved:      Input value for set operation
static void AgentAdvertisementExtensionSetReserved(UInt16 *agentAdvExt,
                                                   UInt16 reserved)
{
    //masks reserved within boundry range
    reserved = reserved & maskShort(8, 16);

    //clears the last 9 bits
    *agentAdvExt = *agentAdvExt & maskShort(1, 7);

    //setting value of reserved in agentAdvExt
    *agentAdvExt = *agentAdvExt | reserved;
}


// FUNCTION:      AgentAdvertisementExtensionGetRegReq()
// PURPOSE:       Returns the value of registrationRequired for
//                AgentAdvertisementExtension
// RETURN:        BOOL
// ASSUMPTION:    None
// PARAMETERS:    agentAdvExt:    The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
static BOOL AgentAdvertisementExtensionGetRegReq(UInt16 agentAdvExt)
{
    UInt16 registrationRequired = agentAdvExt;

    //clears all the bits except the first bit
    registrationRequired = registrationRequired & maskShort(1, 1);

    //right shifts so that last bit represents registrationRequired
    registrationRequired = RshiftShort(registrationRequired, 1);

    return (BOOL)registrationRequired;
}


// FUNCTION:       AgentAdvertisementExtensionGetBusy()
// PURPOSE:        Returns the value of busy for AgentAdvertisementExtension
// RETURN:         BOOL
// ASSUMPTION:     None
// PARAMETERS:     agentAdvExt:   The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
static BOOL AgentAdvertisementExtensionGetBusy(UInt16 agentAdvExt)
{
    UInt16 busy = agentAdvExt;

    //clears all the bits except the second bit
    busy = busy & maskShort(2, 2);

    //right shifts so that last bit represents busy
    busy = RshiftShort(busy, 2);

    return (BOOL)busy;
}


// FUNCTION:      AgentAdvertisementExtensionGetHAgent()
// PURPOSE:       Returns the value of Get_home_agent for
//                AgentAdvertisementExtension
// RETURN:        BOOL
// ASSUMPTION:    None
// PARAMETERS:    agentAdvExt:    The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
static BOOL AgentAdvertisementExtensionGetHAgent(UInt16 agentAdvExt)
{
    UInt16 homeAgent = agentAdvExt;

    //clears all the bits except the third bit
    homeAgent = homeAgent & maskShort(3, 3);

    //right shifts so that last bit represents homeAgent
    homeAgent = RshiftShort(homeAgent, 3);

    return (BOOL)homeAgent;
}


// FUNCTION:      AgentAdvertisementExtensionGetFAgent()
// PURPOSE:       Returns the value of foreignAgent for
//                AgentAdvertisementExtension
// RETURN:        BOOL
// ASSUMPTION:    None
// PARAMETERS:    agentAdvExt:    The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
static BOOL AgentAdvertisementExtensionGetFAgent(UInt16 agentAdvExt)
{
    UInt16 foreignAgent = agentAdvExt;

    //clears all the bits except the fourth bit
    foreignAgent = foreignAgent & maskShort(4, 4);

    //right shifts so that last bit represents foreignAgent
    foreignAgent = RshiftShort(foreignAgent, 4);

    return (BOOL)foreignAgent;
}


// FUNCTION:      AgentAdvertisementExtensionGetMinEncap()
// PURPOSE:       Returns the value of minimalEncapsulation for
//                AgentAdvertisementExtension
// RETURN:        BOOL
// ASSUMPTION:    None
// PARAMETERS:    agentAdvExt:    The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
static BOOL AgentAdvertisementExtensionGetMinEncap(UInt16 agentAdvExt)
{
    UInt16 minimalEncapsulation = agentAdvExt;

    //clears all the bits except the fifth bit
    minimalEncapsulation = minimalEncapsulation & maskShort(5, 5);

    //right shifts so that last bit represents minimalEncapsulation
    minimalEncapsulation = RshiftShort(minimalEncapsulation, 5);

    return (BOOL)minimalEncapsulation;
}


// FUNCTION:      AgentAdvertisementExtensionGetGreEncap()
// PURPOSE:       Returns the value of greEncapsulation for
//                AgentAdvertisementExtension
// RETURN:        BOOL
// ASSUMPTION:    None
// PARAMETERS:    agentAdvExt:    The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
static BOOL AgentAdvertisementExtensionGetGreEncap(UInt16 agentAdvExt)
{
    UInt16 greEncapsulation = agentAdvExt;

    //clears all the bits except the sixth bit
    greEncapsulation = greEncapsulation & maskShort(6, 6);

    //right shifts so that last bit represents greEncapsulation
    greEncapsulation = RshiftShort(greEncapsulation, 6);

    return (BOOL)greEncapsulation;
}


// FUNCTION:       AgentAdvertisementExtensionGetJHdrCmpression()
// PURPOSE:        Returns the value of jacobsonHeaderCompression for
//                 AgentAdvertisementExtension
// RETURN:         BOOL
// ASSUMPTION:     None
// PARAMETERS:     agentAdvExt:   The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
static BOOL AgentAdvertisementExtensionGetJHdrCmpression(UInt16 agentAdvExt)
{
    UInt16 jacobsonHeaderCompression = agentAdvExt;

    //clears all the bits except the seventh bit
    jacobsonHeaderCompression = jacobsonHeaderCompression & maskShort(7, 7);

    //right shifts so that last bit represents jacobsonHeaderCompression
    jacobsonHeaderCompression = RshiftShort(jacobsonHeaderCompression, 7);

    return (BOOL)jacobsonHeaderCompression;
}


// FUNCTION:      AgentAdvertisementExtensionGetReserved()
// PURPOSE:       Returns the value of reserved for
//                AgentAdvertisementExtension
// RETURN:        UInt16
// ASSUMPTION:    None
// PARAMETERS:    agentAdvExt:    The variable containing the value of
//                                registrationRequired,busy,homeAgent,
//                                foreignAgent,minimalEncapsulation,
//                                greEncapsulation,jacobsonHeaderCompression
//                                and reserved
static UInt16 AgentAdvertisementExtensionGetReserved(UInt16 agentAdvExt)
{
    UInt16 reserved = agentAdvExt;

    //clears the first seven bits
    reserved = reserved & maskShort(8, 16);

    return reserved;
}



// Different Agent types. 'Others' indicates the node is neither a mobility
// agent nor mobile node
typedef enum
{
    BothAgent,
    HomeAgent,
    ForeignAgent,
    MobileNode,
    Others
}AgentType;

// Foreign agent type enumeration
typedef enum
{
    ColocatedAddress,
    ForeignAgentAddress,
    None
}ForeignAgentType;

// Enumeration of different registration message types
typedef enum{
    REGISTRATION_REQUEST = 1,
    REGISTRATION_REPLY   = 3,
    PROXY_REQUEST        = 4
}MobileIpRegMessageType;

// Different authentication extension types for control
// messages enumeration
typedef enum{
    mobileHome = 32,
    mobileForeign,
    foreignHome
}ControlMessageExtType;

// Enumeration of different cases when the registration has been accepted or
// denied by the home or foreign agent.
typedef enum{
    mobileIPRegAccept,
    mobileIPRegAcceptNoMobileBinding,

    // Rejections by the Foreign Agent.
    mobileIPRegDenyFAReasonUnspecified = 64,
    mobileIPRegDenyFAAdminProhibit,
    mobileIPRegDenyFAInsuffResources,
    mobileIPRegDenyFAMNFailedAuth,
    mobileIPRegDenyFAHAFailedAuth,
    mobileIPRegDenyFAReqLifetimeLong,
    mobileIPRegDenyFAPoorRequest,
    mobileIPRegDenyFAPoorReply,
    mobileIPRegDenyFAReqEncapUnavailable,
    mobileIPRegDenyFAVanJacobsonCompression,
    mobileIPRegDenyFAHomeNetworkUnreachable = 80,
    mobileIPRegDenyFAHAHostUnreachable,
    mobileIPRegDenyFAHAPortUnreachable,
    mobileIPRegDenyFAHAUnreachable = 88,

    // Rejections by the Home Agent.
    mobileIPRegDenyHAReasonUnspecified = 128,
    mobileIPRegDenyHAAdminProhibit,
    mobileIPRegDenyHAInsuffResources,
    mobileIPRegDenyHAMNFailedAuth,
    mobileIPRegDenyHAFAFailedAuth,
    mobileIPRegDenyHARegIdentMismatch,
    mobileIPRegDenyHAPoorReq,
    mobileIPRegDenyHATooManyMobilityBindings,
    mobileIPRegDenyHAUnknownHAAddress
}MobileIpRegReplyType;

// Registration authentication extension structure
typedef struct registration_authentication_extension
{
    unsigned char extensionType;
    unsigned char extensionLength;
    unsigned int  securityParameterIndex;
    unsigned char authenticator[AUTHENTICATOR_SIZE];
}RegistrationAuthenticationExtension;

// Registration request message structure
typedef struct registration_request{
    unsigned char  type;
    unsigned char  mobIpRr;//mobIp_s:1,
                   //mobIp_b:1,
                   //mobIp_d:1,
                   //mobIp_m:1,
                   //mobIp_g:1,
                   //mobIp_v:1,
                   //mobIp_resv:2;
    unsigned short lifetime;
    NodeAddress    homeAddress;
    NodeAddress    homeAgent;
    NodeAddress    careOfAddress;
    clocktype      identification;      // 8 bytes
    RegistrationAuthenticationExtension mobileHomeExtension;
}MobileIpRegistrationRequest;

// FUNCTION:      MobileIpRegistrationRequestSetS()
// PURPOSE:       Set the value of mobIp_s for MobileIpRegistrationRequest
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    mobIpRr:   The variable containing the value of
//                           mobIp_s,mobIp_b,mobIp_d,mobIp_m,mobIp_g,mobIp_v
//                           and mobIp_resv
//                s:         Input value for set operation
static void MobileIpRegistrationRequestSetS(unsigned char *mobIpRr,
                                            BOOL s)
{
    unsigned char x = (unsigned char) s;

    //masks mobIp_s within boundry range
    x = x & maskChar(8, 8);

    //clears the first bit
    *mobIpRr = *mobIpRr & (~(maskChar(1, 1)));

    //setting the value of x in mobIpRr
    *mobIpRr = *mobIpRr | LshiftChar(x, 1);
}


// FUNCTION:      MobileIpRegistrationRequestSetB()
// PURPOSE:       Set the value of mobIp_b for MobileIpRegistrationRequest
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    mobIpRr:  The variable containing the value of
//                          mobIp_s,mobIp_b,mobIp_d,mobIp_m,mobIp_g,mobIp_v
//                          and mobIp_resv
//                b:        Input value for set operation
static void MobileIpRegistrationRequestSetB(unsigned char *mobIpRr,
                                            BOOL b)
{
    unsigned char x = (unsigned char) b;

    //masks mobIp_b within boundry range
    x = x & maskChar(8, 8);

    //clears the second bit
    *mobIpRr = *mobIpRr & (~(maskChar(2, 2)));

    //setting the value of x in mobIpRr
    *mobIpRr = *mobIpRr | LshiftChar(x, 2);
}


// FUNCTION:      MobileIpRegistrationRequestSetD()
// PURPOSE:       Set the value of mobIp_d for MobileIpRegistrationRequest
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    mobIpRr:  The variable containing the value of
//                          mobIp_s,mobIp_b,mobIp_d,mobIp_m,mobIp_g,mobIp_v
//                          and mobIp_resv
//                d:        Input value for set operation
static void MobileIpRegistrationRequestSetD(unsigned char *mobIpRr,
                                            BOOL d)
{
    unsigned char x = (unsigned char) d;

    //masks mobIp_d within boundry range
    x = x & maskChar(8, 8);

    //clears the third bit
    *mobIpRr = *mobIpRr & (~(maskChar(3, 3)));

    //setting the value of x in mobIpRr
    *mobIpRr = *mobIpRr | LshiftChar(x, 3);
}


// FUNCTION:      MobileIpRegistrationRequestSetM()
// PURPOSE:       Set the value of mobIp_m for MobileIpRegistrationRequest
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    mobIpRr:  The variable containing the value of
//                          mobIp_s,mobIp_b,mobIp_d,mobIp_m,mobIp_g,mobIp_v
//                          and mobIp_resv
//                m:        Input value for set operation
static void MobileIpRegistrationRequestSetM(unsigned char *mobIpRr,
                                            BOOL m)
{
    unsigned char x = (unsigned char) m;

    //masks mobIp_m within boundry range
    x = x & maskChar(8, 8);

    //clears the fourth bit
    *mobIpRr = *mobIpRr & (~(maskChar(4, 4)));

    //setting the value of x in mobIpRr
    *mobIpRr = *mobIpRr | LshiftChar(x, 4);
}


// FUNCTION:      MobileIpRegistrationRequestSetG()
// PURPOSE:       Set the value of mobIp_g for MobileIpRegistrationRequest
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    mobIpRr:  The variable containing the value of
//                          mobIp_s,mobIp_b,mobIp_d,mobIp_m,mobIp_g,mobIp_v
//                          and mobIp_resv
//                g:        Input value for set operation
static void MobileIpRegistrationRequestSetG(unsigned char *mobIpRr,
                                            BOOL g)
{
    unsigned char x = (unsigned char) g;

    //masks mobIp_g within boundry range
    x = x & maskChar(8, 8);

    //clears the fifth bit
    *mobIpRr = *mobIpRr & (~(maskChar(5, 5)));

    //setting the value of x in mobIpRr
    *mobIpRr = *mobIpRr | LshiftChar(x, 5);
}


// FUNCTION:      MobileIpRegistrationRequestSetV()
// PURPOSE:       Set the value of v for MobileIpRegistrationRequest
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    mobIpRr:  The variable containing the value of
//                          mobIp_s,mobIp_b,mobIp_d,mobIp_m,mobIp_g,mobIp_v
//                          and mobIp_resv
//                v:        Input value for set operation
static void MobileIpRegistrationRequestSetV(unsigned char *mobIpRr,
                                            BOOL v)
{
    unsigned char x = (unsigned char) v;

    //masks mobIp_v within boundry range
    x = x & maskChar(8, 8);

    //clears the sixth bit
    *mobIpRr = *mobIpRr & (~(maskChar(6, 6)));

    //setting the value of x in mobIpRr
    *mobIpRr = *mobIpRr | LshiftChar(x, 6);
}


// FUNCTION:      MobileIpRegistrationRequestSetResv()
// PURPOSE:       Set the value of mobIp_resv for MobileIpRegistrationRequest
// RETURN:        void
// ASSUMPTION:    None
// PARAMETERS:    mobIpRr:   The variable containing the value of
//                           mobIp_s,mobIp_b,mobIp_d,mobIp_m,mobIp_g,mobIp_v
//                           and mobIp_resv
//                resv: Input value for set operation
static void MobileIpRegistrationRequestSetResv(unsigned char *mobIpRr,
                                               unsigned char resv)
{
    //masks mobIp_resv within boundry range
     resv = resv & maskChar(7, 8);

    //clears the last two bits
    *mobIpRr = *mobIpRr & maskChar(1, 6);

    //setting the value of reserved in mobIpRr
    *mobIpRr = *mobIpRr | resv;
}


// FUNCTION:      MobileIpRegistrationRequestGetS()
// PURPOSE:       Returns the value of s for MobileIpRegistrationRequest
// RETURN:        BOOL
// ASSUMPTION:    None
// PARAMETERS:    mobIpRr:   The variable containing the value of
//                           mobIp_s,mobIp_b,mobIp_d,mobIp_m,mobIp_g,
//                           mobIp_v,mobIp_resv
static BOOL MobileIpRegistrationRequestGetS(unsigned char mobIpRr)
{
    unsigned char s = mobIpRr;

    //clears all the bits except the first bit
    s = s & maskChar(1, 1);

    //right shifts so that last bit represents s
    s = RshiftChar(s, 1);

    return (BOOL)s;
}


// FUNCTION:      MobileIpRegistrationRequestGetB()
// PURPOSE:       Returns the value of b for MobileIpRegistrationRequest
// RETURN:        BOOL
// ASSUMPTION:    None
// PARAMETERS:    mobIpRr:   The variable containing the value of
//                           mobIp_s,mobIp_b,mobIp_d,mobIp_m,mobIp_g,
//                           mobIp_v,mobIp_resv
static BOOL MobileIpRegistrationRequestGetB(unsigned char mobIpRr)
{
    unsigned char b = mobIpRr;

    //clears all the bits except the second bit
    b = b & maskChar(2, 2);

    //right shifts so that last bit represents b
    b = RshiftChar(b, 2);

    return (BOOL)b;
}


// FUNCTION:      MobileIpRegistrationRequestGetD()
// PURPOSE:       Returns the value of d for MobileIpRegistrationRequest
// RETURN:        BOOL
// ASSUMPTION:    None
// PARAMETERS:    mobIpRr:   The variable containing the value of
//                           mobIp_s,mobIp_b,mobIp_d,mobIp_m,mobIp_g,
//                           mobIp_v,mobIp_resv
static BOOL MobileIpRegistrationRequestGetD(unsigned char mobIpRr)
{
    unsigned char d = mobIpRr;

    //clears all the bits except the third bit
    d = d & maskChar(3, 3);

    //right shifts so that last bit represents d
    d = RshiftChar(d, 3);

    return (BOOL)d;
}


// FUNCTION:      MobileIpRegistrationRequestGetM()
// PURPOSE:       Returns the value of m for MobileIpRegistrationRequest
// RETURN:        BOOL
// ASSUMPTION:    None
// PARAMETERS:    mobIpRr:   The variable containing the value of
//                           mobIp_s,mobIp_b,mobIp_d,mobIp_m,mobIp_g,
//                           mobIp_v,mobIp_resv
static BOOL MobileIpRegistrationRequestGetM(unsigned char mobIpRr)
{
    unsigned char m = mobIpRr;

    //clears all the bits except the fourth bit
    m = m & maskChar(4, 4);

    //right shifts so that last bit represents m
    m = RshiftChar(m, 4);

    return (BOOL)m;
}


// FUNCTION:      MobileIpRegistrationRequestGetG()
// PURPOSE:       Returns the value of g for MobileIpRegistrationRequest
// RETURN:        BOOL
// ASSUMPTION:    None
// PARAMETERS:    mobIpRr:   The variable containing the value of
//                           mobIp_s,mobIp_b,mobIp_d,mobIp_m,mobIp_g,
//                           mobIp_v,mobIp_resv
static BOOL MobileIpRegistrationRequestGetG(unsigned char mobIpRr)
{
    unsigned char g = mobIpRr;

    //clears all the bits except the fifth bit
    g = g & maskChar(5, 5);

    //right shifts so that last bit represents g
    g = RshiftChar(g, 5);

    return (BOOL)g;
}


// FUNCTION:      MobileIpRegistrationRequestGetV()
// PURPOSE:       Returns the value of s for MobileIpRegistrationRequest
// RETURN:        BOOL
// ASSUMPTION:    None
// PARAMETERS:    mobIpRr:   The variable containing the value of
//                           mobIp_s,mobIp_b,mobIp_d,mobIp_m,mobIp_g,
//                           mobIp_v,mobIp_resv
static BOOL MobileIpRegistrationRequestGetV(unsigned char mobIpRr)
{
    unsigned char v = mobIpRr;

    //clears all the bits except the sixth bit
    v = v & maskChar(6, 6);

    //right shifts so that last bit represents v
    v = RshiftChar(v, 6);

    return (BOOL)v;
}


// FUNCTION:      MobileIpRegistrationRequestGetResv()
// PURPOSE:       Returns the value of resv for MobileIpRegistrationRequest
// RETURN:        BOOL
// ASSUMPTION:    None
// PARAMETERS:    mobIpRr:   The variable containing the value of
//                           mobIp_s,mobIp_b,mobIp_d,mobIp_m,mobIp_g,
//                           mobIp_v,mobIp_resv
static unsigned char MobileIpRegistrationRequestGetResv
(unsigned char mobIpRr)
{
    unsigned char resv = mobIpRr;

    //clears all the bits except 7-8
    resv = resv & maskChar(7, 8);

    return resv;
}


// Registration reply message structure
typedef struct registration_reply{
    unsigned char  type;
    unsigned char  code;
    unsigned short lifetime;
    NodeAddress    homeAddress;
    NodeAddress    homeAgent;
    clocktype      identification;     // 8 bytes
    RegistrationAuthenticationExtension mobileHomeExtension;
}MobileIpRegistrationReply;

// Statistics Structure for mobile IP
typedef struct mobile_ip_statistics
{
    unsigned short mobileIpRegRequested;

    unsigned short mobileIpRegRequestReceivedByForeign;
    unsigned short mobileIpRegRequestRelayedByForeign;
    unsigned short mobileIpRegReplyReceivedByForeign;
    unsigned short mobileIpRegReplyRelayedByForeign;

    unsigned short mobileIpRegRequestReceivedByHome;
    unsigned short mobileIpRegReplied;

    unsigned short mobileIpRegRetransmitted;
    unsigned short mobileIpRegDeregistered;
    unsigned short mobileIpRegDiscarded;

    unsigned short mobileIpRegAccepted;

    unsigned short mobileIPRegDenyFAPoorRequest;
    unsigned short mobileIPRegDenyFAReqLifetimeLong;
    unsigned short mobileIPRegDenyHAUnknownHAAddress;
    unsigned short mobileIPRegDenyHAPoorReq;

    unsigned int   mobileIPDataPktEncapHome;
    unsigned int   mobileIPDataPktDcapForeign;
} MobileIpStatistics;

// Mobile node information list structure
typedef struct mobile_node_info_list
{
    NodeAddress      homeAddress;
    NodeAddress      careOfAddress;
    NodeAddress      homeAgent;
    NodeAddress      unicastHome;
    clocktype        identNumber;
    NodeAddress      destinationAddress;   // home/foreign agent IP address
    unsigned short   registrationLifetime;
    unsigned short   remainingLifetime;
    BOOL             isRegReplyRecv;
    unsigned short   retransmissionLifetime;
} MobileNodeInfoList;

// Foreign agent visitor list structure
typedef struct foreign_registration_list
{
    NodeAddress    sourceAddress;
    NodeAddress    destinationAddress;
    NodeAddress    homeAgent;
    short          udpSourcePort;
    clocktype      identNumber;
    unsigned short registrationLifetime;
    unsigned short remainingLifetime;
} ForeignRegistrationList;


// home agent mobility-binding structure
typedef struct home_registration_list
{
    NodeAddress    homeAddress;
    NodeAddress    careOfAddress;
    clocktype      identNumber;
    unsigned short remainingLifetime;
} HomeRegistrationList;

// Home Agent information structure
typedef struct home_list
{
    NodeAddress    homeAddress;
    NodeAddress    homeAgentAddress;
    BOOL           isDummyProxySend;
} HomeList;

// Registration request message structure
typedef struct proxey_request{
    unsigned char  type;
    NodeAddress    homeAgentAddress;
    NodeAddress    mobileNodeAddress;
}MobileIpProxeyRequest;

// NetworkDataMobileIp structure.
typedef struct struct_network_mobile_ip
{
    AgentType            agent;
    unsigned short       sequenceNumber;
    ForeignAgentType     foreignAgentType;
    MobileNodeInfoList   *mobileNodeInfoList;       // Mobile Node information
                                                    // structure

    LinkedList           *homeAgentInfoList;        // mobility-binding structure
    LinkedList           *homeAgent;                // Home Agent information
                                                    // structure

    LinkedList           *foreignAgentInfoList;     // visitor list structure

    BOOL                 collectStatistics;
    MobileIpStatistics   *mobileIpStat;
    RandomSeed           seed;
} NetworkDataMobileIp;

typedef struct source_destination_address
{
    NodeAddress     sourceAddress;
    NodeAddress     destinationAddress;
    int             interfaceIndex;
    unsigned short  registrationSeqNo;
} SrcDestAddrInfo;

typedef struct agent_adv_timer_info
{
    unsigned short remainingLifetime;
    unsigned short advMsgTimerVal;
} AgentTimerInfo;

void MobileIpInit(Node *node,
                         const NodeInput *nodeInput);

void MobileIpHandleProtocolPacket(Node *node,
                                         Message *msg,
                                         NodeAddress sourceAddress,
                                         NodeAddress destinationAddress,
                                         int interfaceIndex);

void MobileIpLayer(Node *node, Message *msg);

clocktype MobileIpFindNextInterval(unsigned int numberOfSolicitation,
                                          unsigned int maxSolicitation);

// Protypes for handling IP in IP Encapsulation & Decapsulation technique
void MobileIpEncapsulateDatagram(Node *node, Message *msg);

void MobileIpDecapsulateDatagram(Node *node, Message *msg);

void MobileIpFinalize(Node* node);

void MobileIpCreateAgentAdvExt(
    Node* node,
    NodeAddress routerAddr,
    Message* msg);

#endif    /* _MOBILE_IP_H_ */
